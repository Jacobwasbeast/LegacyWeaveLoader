#pragma once
#include <cstdint>

namespace PdbParser
{
    bool Open(const char* pdbPath);

    // Returns the RVA for a decorated symbol name, or 0 on failure.
    uint32_t FindSymbolRVA(const char* decoratedName);

    // Returns the RVA for an exact procedure/data name as stored in module/global
    // symbol streams (for example "Tile::onPlace"), or 0 on failure.
    uint32_t FindSymbolRVAByName(const char* exactName);

    // Logs all symbols whose name contains the given substring (for debugging).
    void DumpMatching(const char* substring);

    // Builds a sorted index of all symbols for reverse RVA->name lookup.
    // Must be called while PDB is open. The index survives Close().
    void BuildAddressIndex();

    // Reverse lookup: given an RVA, find the nearest symbol at or before it.
    // Returns true if found. outName receives the symbol name, outOffset
    // the byte distance from the symbol's start address.
    bool FindNameByRVA(uint32_t rva, char* outName, size_t nameSize, uint32_t* outOffset);
    void Close();
}
