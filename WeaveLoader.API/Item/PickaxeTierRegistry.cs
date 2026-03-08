namespace WeaveLoader.API.Item;

public sealed class PickaxeTierDefinition
{
    private readonly ToolMaterialDefinition _inner = new();

    internal ToolMaterialDefinition ToToolMaterialDefinition() => _inner;

    public PickaxeTierDefinition BaseTier(ToolTier tier)
    {
        _inner.BaseTier(tier);
        return this;
    }

    public PickaxeTierDefinition HarvestLevel(int harvestLevel)
    {
        if (harvestLevel < 0)
            throw new ArgumentOutOfRangeException(nameof(harvestLevel));

        _inner.HarvestLevel(harvestLevel);
        return this;
    }

    public PickaxeTierDefinition DestroySpeed(float destroySpeed)
    {
        if (destroySpeed <= 0.0f)
            throw new ArgumentOutOfRangeException(nameof(destroySpeed));

        _inner.DestroySpeed(destroySpeed);
        return this;
    }
}

public sealed class RegisteredPickaxeTier
{
    public Identifier StringId { get; }

    internal RegisteredPickaxeTier(Identifier stringId)
    {
        StringId = stringId;
    }
}

public static class PickaxeTierRegistry
{
    public static RegisteredPickaxeTier Register(Identifier id, PickaxeTierDefinition definition)
    {
        ArgumentNullException.ThrowIfNull(definition);
        ToolMaterialRegistry.Register(id, definition.ToToolMaterialDefinition());

        return new RegisteredPickaxeTier(id);
    }

    internal static bool TryGetDefinition(Identifier id, out PickaxeTierDefinition? definition)
    {
        definition = null;
        return false;
    }
}
