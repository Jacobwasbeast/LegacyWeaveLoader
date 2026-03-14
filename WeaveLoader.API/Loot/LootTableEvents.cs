using System;
using WeaveLoader.API;

namespace WeaveLoader.API.Loot;

public delegate void LootTableModifyHandler(
    LootResourceManager resourceManager,
    LootManager lootManager,
    Identifier id,
    LootTableBuilder tableBuilder,
    LootTableSource source);

public sealed class LootTableModifyEvent
{
    private event LootTableModifyHandler? _handlers;
    internal bool HasHandlers => _handlers != null;

    public void Register(LootTableModifyHandler handler)
    {
        _handlers += handler;
    }

    public void register(LootTableModifyHandler handler)
    {
        Register(handler);
    }

    internal void Fire(LootResourceManager resourceManager,
                       LootManager lootManager,
                       Identifier id,
                       LootTableBuilder tableBuilder,
                       LootTableSource source)
    {
        _handlers?.Invoke(resourceManager, lootManager, id, tableBuilder, source);
    }
}

public static class LootTableEvents
{
    public static LootTableModifyEvent MODIFY { get; } = new();
}
