#pragma once
#include <cstdint>

/// Hosts the .NET CoreCLR runtime inside the game process using the hostfxr API.
/// Loads WeaveLoader.Core.dll and resolves managed entry point methods.
namespace DotNetHost
{
    bool Initialize();
    void Cleanup();

    void CallManagedInit();
    int  CallDiscoverMods(const char* modsPath);
    void CallPreInit();
    void CallInit();
    void CallPostInit();
    void CallTick();
    void CallShutdown();
    int  CallItemMineBlock(const void* args, int sizeBytes);
    int  CallItemUse(const void* args, int sizeBytes);
    void CallEntitySummoned(int entityNumericId, float x, float y, float z);
}
