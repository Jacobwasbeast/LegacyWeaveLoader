namespace WeaveLoader.API.Loot;

public interface ILootCondition
{
    bool Test(Random random);
}

public sealed class RandomChanceLootCondition : ILootCondition
{
    private readonly float _chance;

    private RandomChanceLootCondition(float chance)
    {
        _chance = chance;
    }

    public static RandomChanceLootCondition builder(float chance)
    {
        return new RandomChanceLootCondition(chance);
    }

    public bool Test(Random random)
    {
        if (random == null)
            return false;
        return random.NextDouble() < _chance;
    }
}
