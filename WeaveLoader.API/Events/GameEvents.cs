namespace WeaveLoader.API.Events;

public class TickEventArgs : EventArgs { }

public class BlockBreakEventArgs : EventArgs
{
    public string BlockId { get; init; } = "";
    public int X { get; init; }
    public int Y { get; init; }
    public int Z { get; init; }
    public int PlayerId { get; init; }
}

public class BlockPlaceEventArgs : EventArgs
{
    public string BlockId { get; init; } = "";
    public int X { get; init; }
    public int Y { get; init; }
    public int Z { get; init; }
    public int PlayerId { get; init; }
}

public class ChatEventArgs : EventArgs
{
    public string Message { get; init; } = "";
    public int PlayerId { get; init; }
}

public class EntitySpawnEventArgs : EventArgs
{
    public string EntityId { get; init; } = "";
    public float X { get; init; }
    public float Y { get; init; }
    public float Z { get; init; }
}

public class PlayerJoinEventArgs : EventArgs
{
    public int PlayerId { get; init; }
    public string PlayerName { get; init; } = "";
}

/// <summary>
/// Global game event subscriptions. Subscribe to these in your mod's OnInitialize().
/// Events are fired from the game's main thread via hooks in WeaveLoaderRuntime.
/// </summary>
public static class GameEvents
{
    public static event EventHandler<BlockBreakEventArgs>? OnBlockBreak;
    public static event EventHandler<BlockPlaceEventArgs>? OnBlockPlace;
    public static event EventHandler<ChatEventArgs>? OnChat;
    public static event EventHandler<EntitySpawnEventArgs>? OnEntitySpawn;
    public static event EventHandler<PlayerJoinEventArgs>? OnPlayerJoin;

    internal static void FireBlockBreak(BlockBreakEventArgs e) => OnBlockBreak?.Invoke(null, e);
    internal static void FireBlockPlace(BlockPlaceEventArgs e) => OnBlockPlace?.Invoke(null, e);
    internal static void FireChat(ChatEventArgs e) => OnChat?.Invoke(null, e);
    internal static void FireEntitySpawn(EntitySpawnEventArgs e) => OnEntitySpawn?.Invoke(null, e);
    internal static void FirePlayerJoin(PlayerJoinEventArgs e) => OnPlayerJoin?.Invoke(null, e);
}
