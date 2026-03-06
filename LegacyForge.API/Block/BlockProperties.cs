namespace LegacyForge.API.Block;

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

    public BlockProperties Material(MaterialType material) { MaterialValue = material; return this; }
    public BlockProperties Hardness(float hardness) { HardnessValue = hardness; return this; }
    public BlockProperties Resistance(float resistance) { ResistanceValue = resistance; return this; }
    public BlockProperties Sound(SoundType sound) { SoundValue = sound; return this; }
    public BlockProperties Icon(string iconName) { IconValue = iconName; return this; }
    public BlockProperties LightLevel(float level) { LightEmissionValue = level; return this; }
    public BlockProperties LightBlocking(int level) { LightBlockValue = level; return this; }
    public BlockProperties Indestructible() { HardnessValue = -1.0f; ResistanceValue = 6000000f; return this; }
}
