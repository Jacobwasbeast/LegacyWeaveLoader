namespace WeaveLoader.API.Loot;

public sealed class LootPool
{
    internal ILootNumberProvider Rolls { get; }
    internal List<ILootCondition> Conditions { get; }
    internal List<LootEntry> Entries { get; }

    private LootPool(ILootNumberProvider rolls, List<ILootCondition> conditions, List<LootEntry> entries)
    {
        Rolls = rolls;
        Conditions = conditions;
        Entries = entries;
    }

    public static Builder builder() => new();

    public sealed class Builder
    {
        private ILootNumberProvider? _rolls;
        private readonly List<ILootCondition> _conditions = new();
        private readonly List<LootEntry> _entries = new();

        public Builder rolls(ILootNumberProvider rolls)
        {
            _rolls = rolls;
            return this;
        }

        public Builder conditionally(ILootCondition condition)
        {
            _conditions.Add(condition);
            return this;
        }

        public Builder with(LootEntryBuilder entryBuilder)
        {
            _entries.Add(entryBuilder.Build());
            return this;
        }

        public Builder with(LootEntry entry)
        {
            _entries.Add(entry);
            return this;
        }

        internal LootPool Build()
        {
            ILootNumberProvider rolls = _rolls ?? ConstantLootNumberProvider.create(1);
            return new LootPool(rolls, _conditions, _entries);
        }
    }
}
