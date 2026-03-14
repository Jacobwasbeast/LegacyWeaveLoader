using WeaveLoader.API;

namespace WeaveLoader.API.Loot;

public abstract class LootEntry
{
    public int Weight { get; internal set; } = 1;
    internal List<ILootFunction> Functions { get; } = new();
}

public abstract class LootEntryBuilder
{
    internal abstract LootEntry Build();
}

public sealed class ItemEntry : LootEntry
{
    public Identifier ItemId { get; }

    internal ItemEntry(Identifier itemId)
    {
        ItemId = itemId;
    }

    public static ItemEntryBuilder builder(Identifier itemId)
    {
        return new ItemEntryBuilder(itemId);
    }

    public sealed class ItemEntryBuilder : LootEntryBuilder
    {
        private readonly Identifier _itemId;
        private int _weight = 1;
        private readonly List<ILootFunction> _functions = new();

        internal ItemEntryBuilder(Identifier itemId)
        {
            _itemId = itemId;
        }

        public ItemEntryBuilder weight(int weight)
        {
            _weight = weight;
            return this;
        }

        public ItemEntryBuilder apply(ILootFunction function)
        {
            _functions.Add(function);
            return this;
        }

        internal override LootEntry Build()
        {
            var entry = new ItemEntry(_itemId) { Weight = _weight };
            entry.Functions.AddRange(_functions);
            return entry;
        }
    }
}

public sealed class EmptyEntry : LootEntry
{
    internal EmptyEntry(int weight)
    {
        Weight = weight;
    }
}
