using System.Collections.Generic;
using WeaveLoader.API;

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
/// Rotation/profile mapping used when resolving blockstate variants.
/// </summary>
public enum BlockRotationProfile
{
    None = 0,
    Facing = 1,
    WallSign = 2,
    StandingSign = 3,
    Trapdoor = 4,
    Door = 5
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
    internal string? ModelValue;
    internal float LightEmissionValue = 0.0f;
    internal int LightBlockValue = 255;
    internal bool LightBlockExplicit;
    internal CreativeTab CreativeTabValue = CreativeTab.None;
    internal CreativePlacement? CreativePlacementValue;
    internal Text? NameValue;
    internal int RequiredHarvestLevelValue = -1;
    internal ToolType RequiredToolValue = ToolType.None;
    internal bool AcceptsRedstonePowerValue;
    internal List<Assets.ModelBox>? ModelBoxes;
    internal Dictionary<string, List<Assets.ModelBox>>? ModelVariants;
    internal bool ModelIsFullCube;
    internal BlockRotationProfile RotationProfileValue = BlockRotationProfile.None;
    internal string? BlockStateValue;

    public BlockProperties Material(MaterialType material) { MaterialValue = material; return this; }
    public BlockProperties Hardness(float hardness) { HardnessValue = hardness; return this; }
    public BlockProperties Resistance(float resistance) { ResistanceValue = resistance; return this; }
    public BlockProperties Sound(SoundType sound) { SoundValue = sound; return this; }
    /// <summary>
    /// Icon name in the terrain atlas. Use Java-style names like "examplemod:block/ruby_ore"
    /// from assets/examplemod/textures/block/ruby_ore.png, or vanilla names like "stone", "gold_ore".
    /// </summary>
    public BlockProperties Icon(string iconName) { IconValue = iconName; return this; }
    /// <summary>
    /// Optional Java-style model name (e.g. "examplemod:block/ruby_ore").
    /// When provided, WeaveLoader will read assets/&lt;namespace&gt;/models/block/&lt;name&gt;.json
    /// and use its texture for the block icon.
    /// </summary>
    public BlockProperties Model(string modelName) { ModelValue = modelName; return this; }
    public BlockProperties LightLevel(float level) { LightEmissionValue = level; return this; }
    public BlockProperties LightBlocking(int level) { LightBlockValue = level; LightBlockExplicit = true; return this; }
    public BlockProperties Indestructible() { HardnessValue = -1.0f; ResistanceValue = 6000000f; return this; }
    public BlockProperties InCreativeTab(CreativeTab tab) { CreativeTabValue = tab; return this; }
    public BlockProperties CreativePlacement(CreativePlacement placement) { CreativePlacementValue = placement; return this; }
    public BlockProperties Prepend() { CreativePlacementValue = global::WeaveLoader.API.CreativePlacement.Prepend(); return this; }
    /// <summary>Display name shown in-game (e.g. "Ruby Ore"). Used for localization.</summary>
    public BlockProperties Name(string displayName) { NameValue = Text.Literal(displayName); return this; }
    /// <summary>Localized display name using a language key (e.g. "block.examplemod.ruby_ore").</summary>
    public BlockProperties Name(Text text) { NameValue = text; return this; }
    /// <summary>Minimum harvest level required to properly mine this block (e.g. 3 for obsidian). -1 means no requirement.</summary>
    public BlockProperties RequiredHarvestLevel(int level) { RequiredHarvestLevelValue = level; return this; }
    /// <summary>Tool type required to harvest this block (e.g. Pickaxe for stone-like blocks).</summary>
    public BlockProperties RequiredTool(ToolType tool) { RequiredToolValue = tool; return this; }
    /// <summary>Marks the block as one that can receive redstone power. Stored for future block callbacks.</summary>
    public BlockProperties AcceptsRedstonePower(bool accepts = true) { AcceptsRedstonePowerValue = accepts; return this; }
    /// <summary>Optional blockstate JSON name (e.g. "examplemod:ruby_chair").</summary>
    public BlockProperties BlockState(string blockStateName) { BlockStateValue = blockStateName; return this; }
    /// <summary>Rotation/profile mapping for blockstate variants.</summary>
    public BlockProperties RotationProfile(BlockRotationProfile profile) { RotationProfileValue = profile; return this; }
}
