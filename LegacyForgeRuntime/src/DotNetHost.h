#pragma once
#include <cstdint>

/// Hosts the .NET CoreCLR runtime inside the game process using the hostfxr API.
/// Loads LegacyForge.Core.dll and resolves managed entry point methods.
namespace DotNetHost
{
    bool Initialize();
    void Cleanup();

    void CallManagedInit();
    void CallDiscoverMods(const char* modsPath);
    void CallPreInit();
    void CallInit();
    void CallPostInit();
    void CallTick();
    void CallShutdown();
}
