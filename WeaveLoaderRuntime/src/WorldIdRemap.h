#pragma once

#include <string>

namespace WorldIdRemap
{
    void SetTagNewTagSymbol(void* fnPtr);
    void SetLevelChunkTileSymbols(void* getTileFn, void* setTileFn, void* getPosFn, void* getHighestNonEmptyYFn);
    void EnsureMissingPlaceholders();
    int RemapChunkBlockIds(void* chunkStoragePtr, void* levelChunkPtr, int chunkX, int chunkZ);
    void SaveChunkBlockNamespaces(void* chunkStoragePtr, void* levelChunkPtr);
    void TagModdedItemInstance(void* itemInstancePtr, void* compoundTagPtr);
    void RemapItemInstanceFromTag(void* itemInstancePtr, void* compoundTagPtr);
}
