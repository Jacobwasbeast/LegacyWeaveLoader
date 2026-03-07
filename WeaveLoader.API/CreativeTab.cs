namespace WeaveLoader.API;

/// <summary>
/// Creative inventory tabs matching the game's internal group indices.
/// Use with <see cref="Block.BlockProperties.CreativeTab"/> or
/// <see cref="Item.ItemProperties.CreativeTab"/> to make content
/// appear in the creative menu.
/// </summary>
public enum CreativeTab
{
    None = -1,
    BuildingBlocks = 0,
    Decoration = 1,
    Redstone = 2,
    Transport = 3,
    Materials = 4,
    Food = 5,
    ToolsAndWeapons = 6,
    Brewing = 7,
    Misc = 12
}
