#pragma once

#include <unordered_map>
namespace CustomPickaxeRegistry
{
    struct Definition
    {
        int harvestLevel = 0;
        float destroySpeed = 1.0f;
    };

    void Register(int itemId, int harvestLevel, float destroySpeed);
    const Definition* Find(int itemId);
}
