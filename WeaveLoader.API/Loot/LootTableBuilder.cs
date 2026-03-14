namespace WeaveLoader.API.Loot;

public sealed class LootTableBuilder
{
    internal List<LootPool> Pools { get; } = new();

    public LootTableBuilder pool(LootPool.Builder builder)
    {
        Pools.Add(builder.Build());
        return this;
    }
}
