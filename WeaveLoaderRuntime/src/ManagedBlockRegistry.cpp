#include "ManagedBlockRegistry.h"
#include <unordered_map>

namespace
{
    std::unordered_map<int, ManagedBlockRegistry::Definition> g_definitions;
}

namespace ManagedBlockRegistry
{
    void Register(int blockId)
    {
        g_definitions.try_emplace(blockId);
    }

    void Configure(int blockId, int dropBlockId, int cloneBlockId)
    {
        Definition& def = g_definitions[blockId];
        def.dropBlockId = dropBlockId;
        def.cloneBlockId = cloneBlockId;
    }

    const Definition* Find(int blockId)
    {
        const auto it = g_definitions.find(blockId);
        if (it == g_definitions.end())
            return nullptr;
        return &it->second;
    }

    bool IsManaged(int blockId)
    {
        return g_definitions.find(blockId) != g_definitions.end();
    }
}
