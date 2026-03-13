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
    int  CallItemUseOn(const void* args, int sizeBytes);
    int  CallItemInteractEntity(const void* args, int sizeBytes);
    int  CallItemHurtEntity(const void* args, int sizeBytes);
    int  CallItemInventoryTick(const void* args, int sizeBytes);
    int  CallItemCraftedBy(const void* args, int sizeBytes);
    int  CallBlockOnPlace(const void* args, int sizeBytes);
    int  CallBlockNeighborChanged(const void* args, int sizeBytes);
    int  CallBlockTick(const void* args, int sizeBytes);
    int  CallBlockUse(const void* args, int sizeBytes);
    int  CallBlockStepOn(const void* args, int sizeBytes);
    int  CallBlockEntityInsideTile(const void* args, int sizeBytes);
    int  CallBlockFallOn(const void* args, int sizeBytes);
    int  CallBlockRemoving(const void* args, int sizeBytes);
    int  CallBlockRemoved(const void* args, int sizeBytes);
    int  CallBlockDestroyed(const void* args, int sizeBytes);
    int  CallBlockPlayerDestroy(const void* args, int sizeBytes);
    int  CallBlockPlayerWillDestroy(const void* args, int sizeBytes);
    int  CallBlockPlacedBy(const void* args, int sizeBytes);
    void CallEntitySummoned(int entityNumericId, float x, float y, float z);
}
