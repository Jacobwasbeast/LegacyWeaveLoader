#include "CustomBlockRegistry.h"
#include <unordered_map>

namespace
{
    std::unordered_map<int, CustomBlockRegistry::Definition> g_definitions;
}

namespace CustomBlockRegistry
{
    void Register(int blockId, int requiredHarvestLevel, int requiredTool, bool acceptsRedstonePower)
    {
        Definition def;
        def.requiredHarvestLevel = requiredHarvestLevel;
        def.requiredTool = static_cast<ToolType>(requiredTool);
        def.acceptsRedstonePower = acceptsRedstonePower;
        g_definitions[blockId] = def;
    }

    const Definition* Find(int blockId)
    {
        const auto it = g_definitions.find(blockId);
        if (it == g_definitions.end())
            return nullptr;
        return &it->second;
    }
}
