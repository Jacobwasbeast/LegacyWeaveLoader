#include <Windows.h>
#include <cstdio>
#include <string>
#include "LogUtil.h"
#include "CrashHandler.h"
#include "PdbParser.h"
#include "SymbolResolver.h"
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
    LogUtil::Log("[LegacyForge] InitThread started (module=%p)", g_hModule);

    std::string baseDir = GetDllDirectory(g_hModule);
    LogUtil::Log("[LegacyForge] Runtime DLL directory: %s", baseDir.c_str());

    char cwd[MAX_PATH] = {0};
    GetCurrentDirectoryA(MAX_PATH, cwd);
    LogUtil::Log("[LegacyForge] Game working directory: %s", cwd);

    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    LogUtil::Log("[LegacyForge] Host executable: %s", exePath);

    SymbolResolver symbols;
    if (!symbols.Initialize())
    {
        LogUtil::Log("[LegacyForge] ERROR: Failed to initialize symbol resolver. Is the PDB present?");
        return 1;
    }
    LogUtil::Log("[LegacyForge] Symbol resolver initialized");

    if (!symbols.ResolveGameFunctions())
    {
        LogUtil::Log("[LegacyForge] ERROR: Failed to resolve critical game functions.");
        return 1;
    }
    LogUtil::Log("[LegacyForge] Game functions resolved from PDB");

    if (!HookManager::Install(symbols))
    {
        LogUtil::Log("[LegacyForge] ERROR: Failed to install hooks");
        symbols.Cleanup();
        return 1;
    }
    LogUtil::Log("[LegacyForge] Hooks installed");

    // Build the RVA->name index before releasing the PDB.
    // This index survives PdbParser::Close() and is used by the crash handler.
    PdbParser::BuildAddressIndex();
    CrashHandler::SetGameBase(reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)));

    symbols.Cleanup();

    if (!DotNetHost::Initialize())
    {
        LogUtil::Log("[LegacyForge] ERROR: Failed to initialize .NET host");
        return 1;
    }
    LogUtil::Log("[LegacyForge] .NET runtime initialized");

    std::string modsPath = baseDir + "mods";
    LogUtil::Log("[LegacyForge] Mods directory: %s", modsPath.c_str());

    DotNetHost::CallManagedInit();
    int modCount = DotNetHost::CallDiscoverMods(modsPath.c_str());
    MainMenuOverlay::SetModCount(modCount);
    LogUtil::Log("[LegacyForge] Mod discovery complete (%d mods). Ready.", modCount);

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
