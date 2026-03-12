#pragma once

#include <string>

namespace WorldIdRemap
{
    void SetTagNewTagSymbol(void* fnPtr);
    void SetTileArraySymbol(void* tileArrayPtr);
    void SetLevelChunkTileSymbols(void* getTileFn, void* setTileFn, void* getDataFn, void* setTileAndDataFn, void* getPosFn, void* getHighestNonEmptyYFn);
    void SetCompressedTileStorageSetSymbol(void* setFn);
    void EnsureMissingPlaceholders();
    int RemapChunkBlockIds(void* chunkStoragePtr, void* levelChunkPtr, int chunkX, int chunkZ);
    void SaveChunkBlockNamespaces(void* chunkStoragePtr, void* levelChunkPtr);
    void MarkChunkDirtyByBlockUpdate(int x, int z, int oldBlockId, int newBlockId);
    void TagModdedItemInstance(void* itemInstancePtr, void* compoundTagPtr);
    void RemapItemInstanceFromTag(void* itemInstancePtr, void* compoundTagPtr);
}
