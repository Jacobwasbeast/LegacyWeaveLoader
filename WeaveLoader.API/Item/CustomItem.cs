using System.Runtime.InteropServices;

namespace WeaveLoader.API.Item;

/// <summary>
/// Base class for managed custom items.
/// Mods can inherit and override callbacks for item behavior.
/// </summary>
public abstract class Item
{
    /// <summary>The namespaced ID used during registration.</summary>
    public Identifier? Id { get; internal set; }

    /// <summary>The numeric runtime ID allocated by the game.</summary>
    public int NumericId { get; internal set; } = -1;

    /// <summary>
    /// Called when this item is used to mine a block.
    /// Return <see cref="MineBlockResult.ContinueVanilla"/> to run vanilla logic (equivalent to calling super),
    /// or <see cref="MineBlockResult.CancelVanilla"/> to skip vanilla handling.
    /// </summary>
    public virtual MineBlockResult OnMineBlock(MineBlockContext context) => MineBlockResult.ContinueVanilla;
}

/// <summary>
/// Result of managed mine-block callback.
/// </summary>
public enum MineBlockResult
{
    ContinueVanilla = 0,
    CancelVanilla = 1
}

/// <summary>
/// Tool tier used by native tool constructors.
/// </summary>
public enum ToolTier
{
    Wood = 0,
    Stone = 1,
    Iron = 2,
    Diamond = 3,
    Gold = 4
}

/// <summary>
/// Managed pickaxe base class.
/// Override callbacks to customize behavior.
/// </summary>
public class PickaxeItem : Item
{
    public ToolTier Tier { get; init; } = ToolTier.Diamond;
}

/// <summary>
/// Runtime context for item mine-block callback.
/// </summary>
public readonly struct MineBlockContext
{
    public int ItemId { get; }
    public int TileId { get; }
    public int X { get; }
    public int Y { get; }
    public int Z { get; }

    internal MineBlockContext(int itemId, int tileId, int x, int y, int z)
    {
        ItemId = itemId;
        TileId = tileId;
        X = x;
        Y = y;
        Z = z;
    }
}

[StructLayout(LayoutKind.Sequential)]
internal struct MineBlockNativeArgs
{
    public int ItemId;
    public int TileId;
    public int X;
    public int Y;
    public int Z;
}

internal static class ManagedItemDispatcher
{
    private static readonly object s_lock = new();
    private static readonly Dictionary<int, Item> s_items = new();

    internal static void RegisterItem(Identifier id, int numericId, Item item)
    {
        item.Id = id;
        item.NumericId = numericId;

        lock (s_lock)
        {
            s_items[numericId] = item;
        }
    }

    internal static int HandleMineBlock(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<MineBlockNativeArgs>())
            return 0;

        MineBlockNativeArgs nativeArgs = Marshal.PtrToStructure<MineBlockNativeArgs>(args);

        Item? item;
        lock (s_lock)
        {
            s_items.TryGetValue(nativeArgs.ItemId, out item);
        }

        if (item == null)
            return 0;

        var result = item.OnMineBlock(new MineBlockContext(
            nativeArgs.ItemId,
            nativeArgs.TileId,
            nativeArgs.X,
            nativeArgs.Y,
            nativeArgs.Z));

        // 0 = no managed item, 1 = continue vanilla, 2 = cancel vanilla.
        return result == MineBlockResult.CancelVanilla ? 2 : 1;
    }
}
