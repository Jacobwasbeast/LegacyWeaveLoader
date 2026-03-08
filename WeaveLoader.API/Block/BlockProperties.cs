namespace WeaveLoader.API.Block;

/// <summary>
/// Tool type required to harvest a block. Used with <see cref="BlockProperties.RequiredTool"/>.
/// </summary>
public enum ToolType
{
    /// <summary>No specific tool required; any tool or hand can harvest.</summary>
    None = 0,
    /// <summary>Requires a pickaxe.</summary>
    Pickaxe = 1,
    /// <summary>Requires an axe.</summary>
    Axe = 2,
    /// <summary>Requires a shovel.</summary>
    Shovel = 3,
}

public enum MaterialType
{
    Air = 0,
    Stone = 1,
    Wood = 2,
    Cloth = 3,
    Plant = 4,
    Dirt = 5,
    Sand = 6,
    Glass = 7,
    Water = 8,
    Lava = 9,
    Ice = 10,
    Metal = 11,
    Snow = 12,
    Clay = 13,
    Explosive = 14,
    Web = 15
}

public enum SoundType
{
    None = 0,
    Stone = 1,
    Wood = 2,
    Gravel = 3,
    Grass = 4,
    Metal = 5,
    Glass = 6,
    Cloth = 7,
    Sand = 8,
    Snow = 9
}

/// <summary>
/// Fluent builder for defining block properties.
/// </summary>
public class BlockProperties
{
    internal MaterialType MaterialValue = MaterialType.Stone;
    internal float HardnessValue = 1.0f;
    internal float ResistanceValue = 5.0f;
    internal SoundType SoundValue = SoundType.Stone;
    internal string IconValue = "stone";
    internal float LightEmissionValue = 0.0f;
    internal int LightBlockValue = 255;
    internal CreativeTab CreativeTabValue = CreativeTab.None;
    internal string? NameValue;
    internal int RequiredHarvestLevelValue = -1;
    internal ToolType RequiredToolValue = ToolType.None;

    public BlockProperties Material(MaterialType material) { MaterialValue = material; return this; }
    public BlockProperties Hardness(float hardness) { HardnessValue = hardness; return this; }
    public BlockProperties Resistance(float resistance) { ResistanceValue = resistance; return this; }
    public BlockProperties Sound(SoundType sound) { SoundValue = sound; return this; }
    /// <summary>Icon name in the terrain atlas. Use namespaced ID for mod textures (e.g. "examplemod:ruby_ore" from assets/blocks/ruby_ore.png), or vanilla names like "stone", "gold_ore".</summary>
    public BlockProperties Icon(string iconName) { IconValue = iconName; return this; }
    public BlockProperties LightLevel(float level) { LightEmissionValue = level; return this; }
    public BlockProperties LightBlocking(int level) { LightBlockValue = level; return this; }
    public BlockProperties Indestructible() { HardnessValue = -1.0f; ResistanceValue = 6000000f; return this; }
    public BlockProperties InCreativeTab(CreativeTab tab) { CreativeTabValue = tab; return this; }
    /// <summary>Display name shown in-game (e.g. "Ruby Ore"). Used for localization.</summary>
    public BlockProperties Name(string displayName) { NameValue = displayName; return this; }
    /// <summary>Minimum harvest level required to properly mine this block (e.g. 3 for obsidian). -1 means no requirement.</summary>
    public BlockProperties RequiredHarvestLevel(int level) { RequiredHarvestLevelValue = level; return this; }
    /// <summary>Tool type required to harvest this block (e.g. Pickaxe for stone-like blocks).</summary>
    public BlockProperties RequiredTool(ToolType tool) { RequiredToolValue = tool; return this; }
}
