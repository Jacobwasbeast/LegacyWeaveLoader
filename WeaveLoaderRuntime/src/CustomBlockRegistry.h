#pragma once

namespace CustomBlockRegistry
{
    enum class ToolType : int
    {
        None = 0,
        Pickaxe = 1,
        Axe = 2,
        Shovel = 3,
    };

    struct Definition
    {
        int requiredHarvestLevel = -1;
        ToolType requiredTool = ToolType::None;
    };

    void Register(int blockId, int requiredHarvestLevel, int requiredTool);
    const Definition* Find(int blockId);
}
