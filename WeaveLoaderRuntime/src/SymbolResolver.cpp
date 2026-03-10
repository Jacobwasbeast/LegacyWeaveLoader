#include "SymbolResolver.h"
#include "PdbParser.h"
#include "LogUtil.h"
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static bool HasExtensiveSymbolScanArg()
{
    const char* cmdLine = GetCommandLineA();
    return cmdLine && std::strstr(cmdLine, "--extensive-symbol-scan") != nullptr;
}

static std::vector<std::string> g_pendingExtensiveSymbolScans;

static void QueueExtensiveSymbolScan(const char* missingName)
{
    if (!HasExtensiveSymbolScanArg() || !missingName || !missingName[0])
        return;

    for (const std::string& existing : g_pendingExtensiveSymbolScans)
    {
        if (existing == missingName)
            return;
    }

    g_pendingExtensiveSymbolScans.push_back(missingName);
}

static void RunQueuedExtensiveSymbolScanAndExit()
{
    if (!HasExtensiveSymbolScanArg())
        return;

    if (g_pendingExtensiveSymbolScans.empty())
    {
        LogUtil::Log("[WeaveLoader] Extensive symbol scan requested, but no unresolved symbols were queued");
        return;
    }

    const char* logsDir = LogUtil::GetLogsDir();
    std::string logPath = (logsDir && logsDir[0])
        ? std::string(logsDir) + "similar_dumped_addresses.log"
        : "similar_dumped_addresses.log";
    LogUtil::Log("[WeaveLoader] Extensive symbol scan requested for %zu missing symbol(s)", g_pendingExtensiveSymbolScans.size());
    for (const std::string& missingName : g_pendingExtensiveSymbolScans)
        PdbParser::DumpSimilarFull(missingName.c_str(), logPath.c_str());
    LogUtil::Log("[WeaveLoader] Extensive symbol scan complete, terminating process");
    ExitProcess(0);
}

bool SymbolResolver::Initialize()
{
    m_moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    if (!m_moduleBase)
    {
        LogUtil::Log("[WeaveLoader] Failed to get module base address");
        return false;
    }

    // Derive PDB path from executable path: replace .exe with .pdb
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string pdbPath(exePath);
    size_t dotPos = pdbPath.rfind('.');
    if (dotPos != std::string::npos)
        pdbPath = pdbPath.substr(0, dotPos) + ".pdb";
    else
        pdbPath += ".pdb";

    LogUtil::Log("[WeaveLoader] PDB path: %s", pdbPath.c_str());
    LogUtil::Log("[WeaveLoader] Module base: %p", reinterpret_cast<void*>(m_moduleBase));

    if (!PdbParser::Open(pdbPath.c_str()))
    {
        LogUtil::Log("[WeaveLoader] ERROR: Failed to open PDB file");
        return false;
    }

    m_initialized = true;
    return true;
}

void* SymbolResolver::Resolve(const char* decoratedName)
{
    if (!m_initialized) return nullptr;

    uint32_t rva = PdbParser::FindSymbolRVA(decoratedName);
    if (rva == 0)
    {
        LogUtil::Log("[WeaveLoader] Symbol not found in PDB: '%s'", decoratedName);
        QueueExtensiveSymbolScan(decoratedName);
        PdbParser::DumpSimilar(decoratedName);
        return nullptr;
    }

    return reinterpret_cast<void*>(m_moduleBase + rva);
}

void* SymbolResolver::ResolveExact(const char* exactName)
{
    if (!m_initialized) return nullptr;

    uint32_t rva = PdbParser::FindSymbolRVAByName(exactName);
    if (rva == 0)
    {
        LogUtil::Log("[WeaveLoader] Exact symbol not found in PDB: '%s'", exactName);
        QueueExtensiveSymbolScan(exactName);
        PdbParser::DumpSimilar(exactName);
        return nullptr;
    }

    return reinterpret_cast<void*>(m_moduleBase + rva);
}

bool SymbolResolver::IsStub(void* ptr) const
{
    return ptr == reinterpret_cast<void*>(m_moduleBase + 0x31000u);
}

bool SymbolResolver::ResolveGameFunctions()
{
    LogUtil::Log("[WeaveLoader] Resolving game functions via raw PDB parser...");

    Core.Resolve(*this);
    Ui.Resolve(*this);
    Resource.Resolve(*this);
    Texture.Resolve(*this);
    Item.Resolve(*this);
    Tile.Resolve(*this);
    Level.Resolve(*this);
    Entity.Resolve(*this);
    Inventory.Resolve(*this);
    Nbt.Resolve(*this);

    if (!Texture.pSimpleIconCtor) PdbParser::DumpMatching("??0SimpleIcon@@");
    if (!Texture.pBufferedImageCtorFile && !Texture.pBufferedImageCtorDLCPack) PdbParser::DumpMatching("??0BufferedImage@@");
    if (!Texture.pTextureManagerCreateTexture) PdbParser::DumpMatching("createTexture@TextureManager");
    if (!Texture.pTextureTransferFromImage) PdbParser::DumpMatching("transferFromImage@Texture");
    if (!Resource.pAbstractTexturePackGetImageResource) PdbParser::DumpMatching("getImageResource@AbstractTexturePack");
    if (!Resource.pDLCTexturePackGetImageResource) PdbParser::DumpMatching("getImageResource@DLCTexturePack");
    if (!Texture.pLoadUVs) PdbParser::DumpMatching("loadUVs@PreStitchedTextureMap");
    if (!Texture.pPreStitchedTextureMapStitch) PdbParser::DumpMatching("stitch@PreStitchedTextureMap");

    Core.Log();
    Ui.Log();
    Resource.Log();
    Texture.Log();
    Item.Log();
    Tile.Log();
    Level.Log();
    Entity.Log();
    Inventory.Log();
    Nbt.Log();

    bool ok = Core.HasCritical();
    if (ok)
        LogUtil::Log("[WeaveLoader] All critical symbols resolved (via raw PDB parser)");
    else
        LogUtil::Log("[WeaveLoader] CRITICAL symbols missing - hooks will not be installed");

    RunQueuedExtensiveSymbolScanAndExit();

    return ok;
}

void SymbolResolver::Cleanup()
{
    PdbParser::Close();
    m_initialized = false;
}
