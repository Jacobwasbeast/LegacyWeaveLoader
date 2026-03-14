using WeaveLoader.API;

namespace WeaveLoader.API.Loot;

public interface ILootFunction
{
    void Apply(ref LootDrop drop, Random random);
}

public sealed class SetCountLootFunction : ILootFunction
{
    private readonly int _count;

    public SetCountLootFunction(int count)
    {
        _count = count;
    }

    public void Apply(ref LootDrop drop, Random random)
    {
        drop = drop with { Count = _count };
    }
}

public readonly record struct LootDrop(Identifier ItemId, int Count, int Aux);
