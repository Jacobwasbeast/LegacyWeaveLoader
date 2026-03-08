#include "CustomSlabRegistry.h"

#include <unordered_set>
#include <unordered_map>

namespace
{
    std::unordered_map<int, CustomSlabRegistry::Definition> g_definitions;
}

namespace CustomSlabRegistry
{
    void Register(int halfBlockId, int fullBlockId, int descriptionId, bool woodFamily, const wchar_t* iconName)
    {
        Definition def;
        def.halfBlockId = halfBlockId;
        def.fullBlockId = fullBlockId;
        def.descriptionId = descriptionId;
        def.woodFamily = woodFamily;
        if (iconName)
            def.iconName = iconName;
        g_definitions[halfBlockId] = def;
        g_definitions[fullBlockId] = def;
    }

    const Definition* Find(int blockId)
    {
        const auto it = g_definitions.find(blockId);
        if (it == g_definitions.end())
            return nullptr;
        return &it->second;
    }

    void ForEachUnique(void (*fn)(const Definition&, void*), void* userData)
    {
        if (!fn)
            return;

        std::unordered_set<int> seenHalfIds;
        for (const auto& entry : g_definitions)
        {
            const Definition& def = entry.second;
            if (def.halfBlockId < 0)
                continue;
            if (!seenHalfIds.insert(def.halfBlockId).second)
                continue;
            fn(def, userData);
        }
    }
}
