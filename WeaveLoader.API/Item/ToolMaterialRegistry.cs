namespace WeaveLoader.API.Item;

public sealed class ToolMaterialDefinition
{
    public ToolTier BaseTierValue { get; private set; } = ToolTier.Diamond;
    public int HarvestLevelValue { get; private set; } = 3;
    public float DestroySpeedValue { get; private set; } = 8.0f;

    public ToolMaterialDefinition BaseTier(ToolTier tier)
    {
        BaseTierValue = tier;
        return this;
    }

    public ToolMaterialDefinition HarvestLevel(int harvestLevel)
    {
        if (harvestLevel < 0)
            throw new ArgumentOutOfRangeException(nameof(harvestLevel));

        HarvestLevelValue = harvestLevel;
        return this;
    }

    public ToolMaterialDefinition DestroySpeed(float destroySpeed)
    {
        if (destroySpeed <= 0.0f)
            throw new ArgumentOutOfRangeException(nameof(destroySpeed));

        DestroySpeedValue = destroySpeed;
        return this;
    }

}

public sealed class RegisteredToolMaterial
{
    public Identifier StringId { get; }

    internal RegisteredToolMaterial(Identifier stringId)
    {
        StringId = stringId;
    }
}

public static class ToolMaterialRegistry
{
    private static readonly object s_lock = new();
    private static readonly Dictionary<Identifier, ToolMaterialDefinition> s_materials = new();

    public static RegisteredToolMaterial Register(Identifier id, ToolMaterialDefinition definition)
    {
        ArgumentNullException.ThrowIfNull(definition);

        lock (s_lock)
        {
            if (s_materials.ContainsKey(id))
                throw new InvalidOperationException($"Tool material '{id}' is already registered.");

            s_materials[id] = definition;
        }

        return new RegisteredToolMaterial(id);
    }

    internal static bool TryGetDefinition(Identifier id, out ToolMaterialDefinition? definition)
    {
        lock (s_lock)
        {
            return s_materials.TryGetValue(id, out definition);
        }
    }
}
