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

    /// <summary>
    /// Called when this item is actively used by the player (right-click use path).
    /// Return <see cref="UseItemResult.ContinueVanilla"/> to run vanilla logic,
    /// or <see cref="UseItemResult.CancelVanilla"/> to skip vanilla handling.
    /// </summary>
    public virtual UseItemResult OnUseItem(UseItemContext context) => UseItemResult.ContinueVanilla;
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
/// Result of managed use-item callback.
/// </summary>
public enum UseItemResult
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
public abstract class ToolItem : Item
{
    public ToolTier Tier { get; init; } = ToolTier.Diamond;
    public Identifier? CustomMaterialId { get; init; }
}

public class PickaxeItem : ToolItem
{
    public Identifier? CustomTierId { get; init; }
}

public class ShovelItem : ToolItem
{
}

public class HoeItem : ToolItem
{
}

public class AxeItem : ToolItem
{
}

public class SwordItem : ToolItem
{
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

/// <summary>
/// Runtime context for item use callback.
/// </summary>
public readonly struct UseItemContext
{
    public int ItemId { get; }
    public bool IsTestUseOnly { get; }
    public bool IsClientSide { get; }
    public nint NativeItemInstancePtr { get; }
    public nint NativePlayerPtr { get; }
    public nint NativePlayerSharedPtr { get; }

    internal UseItemContext(int itemId, bool isTestUseOnly, bool isClientSide, nint nativeItemInstancePtr, nint nativePlayerPtr, nint nativePlayerSharedPtr)
    {
        ItemId = itemId;
        IsTestUseOnly = isTestUseOnly;
        IsClientSide = isClientSide;
        NativeItemInstancePtr = nativeItemInstancePtr;
        NativePlayerPtr = nativePlayerPtr;
        NativePlayerSharedPtr = nativePlayerSharedPtr;
    }

    public bool ConsumeInventoryItem(Identifier id, int count = 1)
    {
        if (NativePlayerPtr == 0 || count <= 0)
            return false;

        int numericId = IdHelper.GetItemNumericId(id);
        if (numericId < 0)
            return false;

        return NativeInterop.native_consume_item_from_player(NativePlayerPtr, numericId, count) != 0;
    }

    public bool DamageItem(int amount)
    {
        if (NativeItemInstancePtr == 0 || NativePlayerSharedPtr == 0 || amount <= 0)
            return false;

        return NativeInterop.native_damage_item_instance(NativeItemInstancePtr, amount, NativePlayerSharedPtr) != 0;
    }

    public bool SpawnEntityFromLook(Identifier id, double speed = 1.4, double spawnForward = 1.0, double spawnUp = 1.2)
    {
        int numericEntityId = IdHelper.GetEntityNumericId(id);
        if (numericEntityId < 0)
            return false;

        return SpawnEntityFromLook(numericEntityId, speed, spawnForward, spawnUp);
    }

    public bool SpawnEntityFromLook(int numericEntityId, double speed = 1.4, double spawnForward = 1.0, double spawnUp = 1.2)
    {
        if (NativePlayerPtr == 0 || numericEntityId < 0)
            return false;

        return NativeInterop.native_spawn_entity_from_player_look(
            NativePlayerPtr,
            NativePlayerSharedPtr,
            numericEntityId,
            speed,
            spawnForward,
            spawnUp) != 0;
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

[StructLayout(LayoutKind.Sequential)]
internal struct UseItemNativeArgs
{
    public int ItemId;
    public int IsTestUseOnly;
    public int IsClientSide;
    public nint ItemInstancePtr;
    public nint PlayerPtr;
    public nint PlayerSharedPtr;
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

    internal static int HandleUseItem(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<UseItemNativeArgs>())
            return 0;

        UseItemNativeArgs nativeArgs = Marshal.PtrToStructure<UseItemNativeArgs>(args);

        Item? item;
        lock (s_lock)
        {
            s_items.TryGetValue(nativeArgs.ItemId, out item);
        }

        if (item == null)
            return 0;

        var result = item.OnUseItem(new UseItemContext(
            nativeArgs.ItemId,
            nativeArgs.IsTestUseOnly != 0,
            nativeArgs.IsClientSide != 0,
            nativeArgs.ItemInstancePtr,
            nativeArgs.PlayerPtr,
            nativeArgs.PlayerSharedPtr));

        // 0 = no managed item, 1 = continue vanilla, 2 = cancel vanilla.
        return result == UseItemResult.CancelVanilla ? 2 : 1;
    }
}
