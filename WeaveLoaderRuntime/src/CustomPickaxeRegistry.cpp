#include "CustomPickaxeRegistry.h"

namespace
{
    std::unordered_map<int, CustomPickaxeRegistry::Definition> g_definitions;
}

namespace CustomPickaxeRegistry
{
    void Register(int itemId, int harvestLevel, float destroySpeed)
    {
        Definition definition;
        definition.harvestLevel = harvestLevel;
        definition.destroySpeed = destroySpeed;

        g_definitions[itemId] = std::move(definition);
    }

    const Definition* Find(int itemId)
    {
        const auto it = g_definitions.find(itemId);
        if (it == g_definitions.end())
            return nullptr;
        return &it->second;
    }
}
