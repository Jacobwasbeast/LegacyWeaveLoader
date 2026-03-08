#pragma once
#include <string>

namespace CustomSlabRegistry
{
    struct Definition
    {
        int halfBlockId = -1;
        int fullBlockId = -1;
        int descriptionId = -1;
        bool woodFamily = false;
        std::wstring iconName;
    };

    void Register(int halfBlockId, int fullBlockId, int descriptionId, bool woodFamily, const wchar_t* iconName);
    const Definition* Find(int blockId);
    void ForEachUnique(void (*fn)(const Definition&, void*), void* userData);
}
