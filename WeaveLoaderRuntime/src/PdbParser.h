#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace PdbParser
{
    struct SymbolInfo
    {
        uint32_t rva = 0;
        std::string name;
        bool isProc = false;
        uint8_t source = 0;
    };

    bool Open(const char* pdbPath);

    // Returns the RVA for a decorated symbol name, or 0 on failure.
    uint32_t FindSymbolRVA(const char* decoratedName);

    // Returns the RVA for an exact procedure/data name as stored in module/global
    // symbol streams (for example "Tile::onPlace"), or 0 on failure.
    uint32_t FindSymbolRVAByName(const char* exactName);

    // Logs all symbols whose name contains the given substring (for debugging).
    void DumpMatching(const char* substring);

    // Logs a short list of likely DumpMatching() patterns for a missing symbol.
    void DumpSimilar(const char* missingName);

    // Writes a ranked full similarity dump to a file for a missing symbol.
    void DumpSimilarFull(const char* missingName, const char* logPath, size_t maxResults = 200);

    // Builds a sorted index of all symbols for reverse RVA->name lookup.
    // Must be called while PDB is open. The index survives Close().
    void BuildAddressIndex();

    // Reverse lookup: given an RVA, find the nearest symbol at or before it.
    // Returns true if found. outName receives the symbol name, outOffset
    // the byte distance from the symbol's start address.
    bool FindNameByRVA(uint32_t rva, char* outName, size_t nameSize, uint32_t* outOffset);
    bool EnumerateSymbols(std::vector<SymbolInfo>& outSymbols);
    void Close();
}
