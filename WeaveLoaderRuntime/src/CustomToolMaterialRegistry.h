#pragma once

#include <unordered_map>

namespace CustomToolMaterialRegistry
{
    enum class ToolKind : int
    {
        Pickaxe = 1,
        Shovel = 2,
        Hoe = 3,
        Sword = 4,
        Axe = 5,
    };

    struct Definition
    {
        ToolKind toolKind = ToolKind::Pickaxe;
        int harvestLevel = 0;
        float destroySpeed = 1.0f;
        float attackDamage = 0.0f;
    };

    void Register(int itemId, ToolKind toolKind, int harvestLevel, float destroySpeed, float attackDamage);
    const Definition* Find(int itemId);
}
