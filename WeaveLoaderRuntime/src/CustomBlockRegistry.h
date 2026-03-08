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
        bool acceptsRedstonePower = false;
    };

    void Register(int blockId, int requiredHarvestLevel, int requiredTool, bool acceptsRedstonePower);
    const Definition* Find(int blockId);
}
