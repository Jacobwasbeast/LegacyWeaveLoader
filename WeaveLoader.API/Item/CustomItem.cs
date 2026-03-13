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

    /// <summary>
    /// Called when this item is used on a block.
    /// Return <see cref="ItemActionResult.ContinueVanilla"/> to run vanilla logic,
    /// or <see cref="ItemActionResult.CancelVanilla"/> to skip vanilla handling.
    /// </summary>
    public virtual ItemActionResult OnUseOn(UseOnItemContext context) => ItemActionResult.ContinueVanilla;

    /// <summary>
    /// Called when this item interacts with an entity (right-click interaction).
    /// Return <see cref="ItemActionResult.ContinueVanilla"/> to run vanilla logic,
    /// or <see cref="ItemActionResult.CancelVanilla"/> to skip vanilla handling.
    /// </summary>
    public virtual ItemActionResult OnInteractEntity(ItemEntityInteractionContext context) => ItemActionResult.ContinueVanilla;

    /// <summary>
    /// Called when this item hurts an entity.
    /// Return <see cref="ItemActionResult.ContinueVanilla"/> to run vanilla logic,
    /// or <see cref="ItemActionResult.CancelVanilla"/> to skip vanilla handling.
    /// </summary>
    public virtual ItemActionResult OnHurtEntity(ItemEntityInteractionContext context) => ItemActionResult.ContinueVanilla;

    /// <summary>
    /// Called each tick while this item is in an inventory.
    /// </summary>
    public virtual void OnInventoryTick(ItemInventoryTickContext context)
    {
    }

    /// <summary>
    /// Called when this item is crafted by a player.
    /// </summary>
    public virtual void OnCraftedBy(ItemCraftedByContext context)
    {
    }
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
/// Result of managed item action callbacks.
/// </summary>
public enum ItemActionResult
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
    public bool IsClientSide { get; }
    public nint NativeItemInstancePtr { get; }
    public nint NativePlayerPtr { get; }
    public nint NativePlayerSharedPtr { get; }

    internal UseItemContext(int itemId, bool isClientSide, nint nativeItemInstancePtr, nint nativePlayerPtr, nint nativePlayerSharedPtr)
    {
        ItemId = itemId;
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

/// <summary>
/// Runtime context for item use-on-block callback.
/// </summary>
public readonly struct UseOnItemContext
{
    public int ItemId { get; }
    public bool IsClientSide { get; }
    public nint NativeItemInstancePtr { get; }
    public nint NativePlayerPtr { get; }
    public nint NativePlayerSharedPtr { get; }
    public nint NativeLevelPtr { get; }
    public int X { get; }
    public int Y { get; }
    public int Z { get; }
    public int Face { get; }
    public float ClickX { get; }
    public float ClickY { get; }
    public float ClickZ { get; }

    internal UseOnItemContext(
        int itemId,
        bool isClientSide,
        nint nativeItemInstancePtr,
        nint nativePlayerPtr,
        nint nativePlayerSharedPtr,
        nint nativeLevelPtr,
        int x,
        int y,
        int z,
        int face,
        float clickX,
        float clickY,
        float clickZ)
    {
        ItemId = itemId;
        IsClientSide = isClientSide;
        NativeItemInstancePtr = nativeItemInstancePtr;
        NativePlayerPtr = nativePlayerPtr;
        NativePlayerSharedPtr = nativePlayerSharedPtr;
        NativeLevelPtr = nativeLevelPtr;
        X = x;
        Y = y;
        Z = z;
        Face = face;
        ClickX = clickX;
        ClickY = clickY;
        ClickZ = clickZ;
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

/// <summary>
/// Runtime context for item/entity interactions.
/// </summary>
public readonly struct ItemEntityInteractionContext
{
    public int ItemId { get; }
    public nint NativeItemInstancePtr { get; }
    public nint NativePlayerPtr { get; }
    public nint NativePlayerSharedPtr { get; }
    public nint NativeTargetEntityPtr { get; }

    internal ItemEntityInteractionContext(
        int itemId,
        nint nativeItemInstancePtr,
        nint nativePlayerPtr,
        nint nativePlayerSharedPtr,
        nint nativeTargetEntityPtr)
    {
        ItemId = itemId;
        NativeItemInstancePtr = nativeItemInstancePtr;
        NativePlayerPtr = nativePlayerPtr;
        NativePlayerSharedPtr = nativePlayerSharedPtr;
        NativeTargetEntityPtr = nativeTargetEntityPtr;
    }

    public bool DamageItem(int amount)
    {
        if (NativeItemInstancePtr == 0 || NativePlayerSharedPtr == 0 || amount <= 0)
            return false;

        return NativeInterop.native_damage_item_instance(NativeItemInstancePtr, amount, NativePlayerSharedPtr) != 0;
    }
}

/// <summary>
/// Runtime context for inventory tick callbacks.
/// </summary>
public readonly struct ItemInventoryTickContext
{
    public int ItemId { get; }
    public nint NativeItemInstancePtr { get; }
    public nint NativeLevelPtr { get; }
    public nint NativeOwnerEntityPtr { get; }
    public int Slot { get; }
    public bool IsSelected { get; }
    public bool IsClientSide { get; }

    internal ItemInventoryTickContext(
        int itemId,
        nint nativeItemInstancePtr,
        nint nativeLevelPtr,
        nint nativeOwnerEntityPtr,
        int slot,
        bool isSelected,
        bool isClientSide)
    {
        ItemId = itemId;
        NativeItemInstancePtr = nativeItemInstancePtr;
        NativeLevelPtr = nativeLevelPtr;
        NativeOwnerEntityPtr = nativeOwnerEntityPtr;
        Slot = slot;
        IsSelected = isSelected;
        IsClientSide = isClientSide;
    }
}

/// <summary>
/// Runtime context for crafted-by callbacks.
/// </summary>
public readonly struct ItemCraftedByContext
{
    public int ItemId { get; }
    public nint NativeItemInstancePtr { get; }
    public nint NativeLevelPtr { get; }
    public nint NativePlayerPtr { get; }
    public nint NativePlayerSharedPtr { get; }
    public int Amount { get; }
    public bool IsClientSide { get; }

    internal ItemCraftedByContext(
        int itemId,
        nint nativeItemInstancePtr,
        nint nativeLevelPtr,
        nint nativePlayerPtr,
        nint nativePlayerSharedPtr,
        int amount,
        bool isClientSide)
    {
        ItemId = itemId;
        NativeItemInstancePtr = nativeItemInstancePtr;
        NativeLevelPtr = nativeLevelPtr;
        NativePlayerPtr = nativePlayerPtr;
        NativePlayerSharedPtr = nativePlayerSharedPtr;
        Amount = amount;
        IsClientSide = isClientSide;
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
    public int IsClientSide;
    public nint ItemInstancePtr;
    public nint PlayerPtr;
    public nint PlayerSharedPtr;
}

[StructLayout(LayoutKind.Sequential)]
internal struct UseOnItemNativeArgs
{
    public int ItemId;
    public int IsClientSide;
    public nint ItemInstancePtr;
    public nint PlayerPtr;
    public nint PlayerSharedPtr;
    public nint LevelPtr;
    public int X;
    public int Y;
    public int Z;
    public int Face;
    public float ClickX;
    public float ClickY;
    public float ClickZ;
}

[StructLayout(LayoutKind.Sequential)]
internal struct ItemEntityInteractionNativeArgs
{
    public int ItemId;
    public nint ItemInstancePtr;
    public nint PlayerPtr;
    public nint PlayerSharedPtr;
    public nint TargetEntityPtr;
}

[StructLayout(LayoutKind.Sequential)]
internal struct ItemInventoryTickNativeArgs
{
    public int ItemId;
    public nint ItemInstancePtr;
    public nint LevelPtr;
    public nint OwnerEntityPtr;
    public int Slot;
    public int IsSelected;
    public int IsClientSide;
}

[StructLayout(LayoutKind.Sequential)]
internal struct ItemCraftedByNativeArgs
{
    public int ItemId;
    public nint ItemInstancePtr;
    public nint LevelPtr;
    public nint PlayerPtr;
    public nint PlayerSharedPtr;
    public int Amount;
    public int IsClientSide;
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
            nativeArgs.IsClientSide != 0,
            nativeArgs.ItemInstancePtr,
            nativeArgs.PlayerPtr,
            nativeArgs.PlayerSharedPtr));

        // 0 = no managed item, 1 = continue vanilla, 2 = cancel vanilla.
        return result == UseItemResult.CancelVanilla ? 2 : 1;
    }

    internal static int HandleUseOn(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<UseOnItemNativeArgs>())
            return 0;

        UseOnItemNativeArgs nativeArgs = Marshal.PtrToStructure<UseOnItemNativeArgs>(args);

        Item? item;
        lock (s_lock)
        {
            s_items.TryGetValue(nativeArgs.ItemId, out item);
        }

        if (item == null)
            return 0;

        var result = item.OnUseOn(new UseOnItemContext(
            nativeArgs.ItemId,
            nativeArgs.IsClientSide != 0,
            nativeArgs.ItemInstancePtr,
            nativeArgs.PlayerPtr,
            nativeArgs.PlayerSharedPtr,
            nativeArgs.LevelPtr,
            nativeArgs.X,
            nativeArgs.Y,
            nativeArgs.Z,
            nativeArgs.Face,
            nativeArgs.ClickX,
            nativeArgs.ClickY,
            nativeArgs.ClickZ));

        return result == ItemActionResult.CancelVanilla ? 2 : 1;
    }

    internal static int HandleInteractEntity(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<ItemEntityInteractionNativeArgs>())
            return 0;

        ItemEntityInteractionNativeArgs nativeArgs = Marshal.PtrToStructure<ItemEntityInteractionNativeArgs>(args);

        Item? item;
        lock (s_lock)
        {
            s_items.TryGetValue(nativeArgs.ItemId, out item);
        }

        if (item == null)
            return 0;

        var result = item.OnInteractEntity(new ItemEntityInteractionContext(
            nativeArgs.ItemId,
            nativeArgs.ItemInstancePtr,
            nativeArgs.PlayerPtr,
            nativeArgs.PlayerSharedPtr,
            nativeArgs.TargetEntityPtr));

        return result == ItemActionResult.CancelVanilla ? 2 : 1;
    }

    internal static int HandleHurtEntity(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<ItemEntityInteractionNativeArgs>())
            return 0;

        ItemEntityInteractionNativeArgs nativeArgs = Marshal.PtrToStructure<ItemEntityInteractionNativeArgs>(args);

        Item? item;
        lock (s_lock)
        {
            s_items.TryGetValue(nativeArgs.ItemId, out item);
        }

        if (item == null)
            return 0;

        var result = item.OnHurtEntity(new ItemEntityInteractionContext(
            nativeArgs.ItemId,
            nativeArgs.ItemInstancePtr,
            nativeArgs.PlayerPtr,
            nativeArgs.PlayerSharedPtr,
            nativeArgs.TargetEntityPtr));

        return result == ItemActionResult.CancelVanilla ? 2 : 1;
    }

    internal static int HandleInventoryTick(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<ItemInventoryTickNativeArgs>())
            return 0;

        ItemInventoryTickNativeArgs nativeArgs = Marshal.PtrToStructure<ItemInventoryTickNativeArgs>(args);

        Item? item;
        lock (s_lock)
        {
            s_items.TryGetValue(nativeArgs.ItemId, out item);
        }

        if (item == null)
            return 0;

        item.OnInventoryTick(new ItemInventoryTickContext(
            nativeArgs.ItemId,
            nativeArgs.ItemInstancePtr,
            nativeArgs.LevelPtr,
            nativeArgs.OwnerEntityPtr,
            nativeArgs.Slot,
            nativeArgs.IsSelected != 0,
            nativeArgs.IsClientSide != 0));
        return 1;
    }

    internal static int HandleCraftedBy(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<ItemCraftedByNativeArgs>())
            return 0;

        ItemCraftedByNativeArgs nativeArgs = Marshal.PtrToStructure<ItemCraftedByNativeArgs>(args);

        Item? item;
        lock (s_lock)
        {
            s_items.TryGetValue(nativeArgs.ItemId, out item);
        }

        if (item == null)
            return 0;

        item.OnCraftedBy(new ItemCraftedByContext(
            nativeArgs.ItemId,
            nativeArgs.ItemInstancePtr,
            nativeArgs.LevelPtr,
            nativeArgs.PlayerPtr,
            nativeArgs.PlayerSharedPtr,
            nativeArgs.Amount,
            nativeArgs.IsClientSide != 0));
        return 1;
    }
}
