namespace WeaveLoader.API.Loot;

public interface ILootNumberProvider
{
    int NextInt(Random random);
}

public sealed class ConstantLootNumberProvider : ILootNumberProvider
{
    private readonly int _value;

    private ConstantLootNumberProvider(int value)
    {
        _value = value;
    }

    public static ConstantLootNumberProvider create(int value)
    {
        return new ConstantLootNumberProvider(value);
    }

    public int NextInt(Random random) => _value;
}
