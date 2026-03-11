# API Reference (Short)

## Registry.Block

```csharp
RegisteredBlock Register(string id, BlockProperties properties)
```

Creates a block and its inventory item.

## Registry.Item

```csharp
RegisteredItem Register(string id, ItemProperties properties)
```

Creates a new item.

## Registry.Recipe

```csharp
void AddFurnace(string inputId, string outputId, float xp)
```

Adds a furnace recipe.

## GameEvents

```csharp
event EventHandler<BlockBreakEventArgs> OnBlockBreak;
event EventHandler<BlockPlaceEventArgs> OnBlockPlace;
event EventHandler<ChatEventArgs> OnChat;
event EventHandler<EntitySpawnEventArgs> OnEntitySpawn;
event EventHandler<PlayerJoinEventArgs> OnPlayerJoin;
```

## Logger

```csharp
Logger.Debug(string message)
Logger.Info(string message)
Logger.Warning(string message)
Logger.Error(string message)
```

## Identifier

Namespaced IDs use `"namespace:path"` (for example, `"mymod:ruby_ore"`). If no namespace is provided, `"minecraft"` is assumed.
