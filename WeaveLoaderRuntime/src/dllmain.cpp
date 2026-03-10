#include <Windows.h>
#include <cstdio>
#include <string>
#include "LogUtil.h"
#include "CrashHandler.h"
#include "PdbParser.h"
#include "SymbolResolver.h"
#include "SymbolRegistry.h"
#include "HookManager.h"
#include "DotNetHost.h"
#include "MainMenuOverlay.h"

static HMODULE g_hModule = nullptr;

static std::string GetDllDirectory(HMODULE hModule)
{
    char path[MAX_PATH] = {0};
    GetModuleFileNameA(hModule, path, MAX_PATH);
    std::string s(path);
    size_t pos = s.find_last_of("\\/");
    if (pos != std::string::npos)
        return s.substr(0, pos + 1);
    return ".\\";
}

DWORD WINAPI InitThread(LPVOID lpParam)
{
    LogUtil::Log("[WeaveLoader] InitThread started (module=%p)", g_hModule);
#ifdef WEAVELOADER_DEBUG_BUILD
    LogUtil::Log("[WeaveLoader] Loader is running in debug mode");
#endif

    std::string baseDir = GetDllDirectory(g_hModule);
    LogUtil::Log("[WeaveLoader] Runtime DLL directory: %s", baseDir.c_str());

    std::string mappingPath = baseDir + "metadata\\mapping.json";
    if (!SymbolRegistry::Instance().LoadFromFile(mappingPath.c_str()))
    {
        std::string fallbackPath = baseDir + "mapping.json";
        SymbolRegistry::Instance().LoadFromFile(fallbackPath.c_str());
    }

    char cwd[MAX_PATH] = {0};
    GetCurrentDirectoryA(MAX_PATH, cwd);
    LogUtil::Log("[WeaveLoader] Game working directory: %s", cwd);

    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    LogUtil::Log("[WeaveLoader] Host executable: %s", exePath);

    SymbolResolver symbols;
    if (!symbols.Initialize())
    {
        LogUtil::Log("[WeaveLoader] ERROR: Failed to initialize symbol resolver. Is the PDB present?");
        return 1;
    }
    LogUtil::Log("[WeaveLoader] Symbol resolver initialized");

    if (!symbols.ResolveGameFunctions())
    {
        LogUtil::Log("[WeaveLoader] ERROR: Failed to resolve critical game functions.");
        return 1;
    }
    LogUtil::Log("[WeaveLoader] Game functions resolved from PDB");

    if (!HookManager::Install(symbols))
    {
        LogUtil::Log("[WeaveLoader] ERROR: Failed to install hooks");
        symbols.Cleanup();
        return 1;
    }
    LogUtil::Log("[WeaveLoader] Hooks installed");

    // Build the RVA->name index before releasing the PDB.
    // This index survives PdbParser::Close() and is used by the crash handler.
    PdbParser::BuildAddressIndex();
    CrashHandler::SetGameBase(reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)));

    symbols.Cleanup();

    if (!DotNetHost::Initialize())
    {
        LogUtil::Log("[WeaveLoader] ERROR: Failed to initialize .NET host");
        return 1;
    }
    LogUtil::Log("[WeaveLoader] .NET runtime initialized");

    std::string modsPath = baseDir + "mods";
    LogUtil::Log("[WeaveLoader] Mods directory: %s", modsPath.c_str());

    DotNetHost::CallManagedInit();
    int modCount = DotNetHost::CallDiscoverMods(modsPath.c_str());
    MainMenuOverlay::SetModCount(modCount);
    LogUtil::Log("[WeaveLoader] Mod discovery complete (%d mods). Ready.", modCount);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        // Set up logging and crash handler BEFORE anything else.
        // These must work immediately so we catch crashes during init.
        {
            std::string baseDir = GetDllDirectory(hModule);
            LogUtil::SetBaseDir(baseDir.c_str());
            CrashHandler::Install(hModule);
        }

        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        HookManager::Cleanup();
        break;
    }
    return TRUE;
}
