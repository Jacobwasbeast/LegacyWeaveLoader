#include "CustomToolMaterialRegistry.h"

namespace
{
    std::unordered_map<int, CustomToolMaterialRegistry::Definition> g_definitions;
}

namespace CustomToolMaterialRegistry
{
    void Register(int itemId, ToolKind toolKind, int harvestLevel, float destroySpeed, float attackDamage)
    {
        Definition definition;
        definition.toolKind = toolKind;
        definition.harvestLevel = harvestLevel;
        definition.destroySpeed = destroySpeed;
        definition.attackDamage = attackDamage;
        g_definitions[itemId] = definition;
    }

    const Definition* Find(int itemId)
    {
        const auto it = g_definitions.find(itemId);
        if (it == g_definitions.end())
            return nullptr;
        return &it->second;
    }
}
