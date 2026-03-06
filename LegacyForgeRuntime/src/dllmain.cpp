#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "SymbolResolver.h"
#include "HookManager.h"
#include "DotNetHost.h"

static HMODULE g_hModule = nullptr;

static void LogToFile(const char* msg)
{
    FILE* f = nullptr;
    fopen_s(&f, "legacyforge.log", "a");
    if (f)
    {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

DWORD WINAPI InitThread(LPVOID lpParam)
{
    LogToFile("[LegacyForge] InitThread started");

    SymbolResolver symbols;
    if (!symbols.Initialize())
    {
        LogToFile("[LegacyForge] ERROR: Failed to initialize symbol resolver. Is the PDB present?");
        return 1;
    }
    LogToFile("[LegacyForge] Symbol resolver initialized");

    if (!symbols.ResolveGameFunctions())
    {
        LogToFile("[LegacyForge] ERROR: Failed to resolve one or more game functions");
        return 1;
    }
    LogToFile("[LegacyForge] Game functions resolved from PDB");

    if (!HookManager::Install(symbols))
    {
        LogToFile("[LegacyForge] ERROR: Failed to install hooks");
        return 1;
    }
    LogToFile("[LegacyForge] Hooks installed");

    if (!DotNetHost::Initialize())
    {
        LogToFile("[LegacyForge] ERROR: Failed to initialize .NET host");
        return 1;
    }
    LogToFile("[LegacyForge] .NET runtime initialized");

    DotNetHost::CallManagedInit();
    DotNetHost::CallDiscoverMods("mods");
    LogToFile("[LegacyForge] Mod discovery complete. Ready.");

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        HookManager::Cleanup();
        break;
    }
    return TRUE;
}
