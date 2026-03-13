using System.Runtime.InteropServices;

namespace WeaveLoader.API.Block;

/// <summary>
/// Base class for managed custom blocks.
/// Mods can inherit and override callbacks for block behavior.
/// </summary>
public abstract class Block
{
    /// <summary>The namespaced ID used during registration.</summary>
    public Identifier? Id { get; internal set; }

    /// <summary>The numeric runtime ID allocated by the game.</summary>
    public int NumericId { get; internal set; } = -1;

    /// <summary>Optional override for the block to drop when broken.</summary>
    public Identifier? DropAsBlockId { get; init; }

    /// <summary>Optional override for the block to clone via pick-block.</summary>
    public Identifier? CloneAsBlockId { get; init; }

    /// <summary>
    /// Called when the block is placed in the world.
    /// </summary>
    public virtual void OnPlace(BlockUpdateContext context)
    {
    }

    /// <summary>
    /// Called when a neighboring block changes.
    /// </summary>
    public virtual void OnNeighborChanged(BlockNeighborChangedContext context)
    {
    }

    /// <summary>
    /// Called when a scheduled tick for this block fires.
    /// </summary>
    public virtual void OnScheduledTick(BlockTickContext context)
    {
    }

    /// <summary>
    /// Called when the block is used (right-click).
    /// Return <see cref="BlockActionResult.ContinueVanilla"/> to run vanilla logic,
    /// or <see cref="BlockActionResult.CancelVanilla"/> to skip vanilla handling.
    /// </summary>
    public virtual BlockActionResult OnUse(BlockUseContext context) => BlockActionResult.ContinueVanilla;

    /// <summary>
    /// Called when an entity steps on the block.
    /// </summary>
    public virtual void OnStepOn(BlockEntityContext context)
    {
    }

    /// <summary>
    /// Called when an entity is inside the block's volume.
    /// </summary>
    public virtual void OnEntityInsideTile(BlockEntityContext context)
    {
    }

    /// <summary>
    /// Called when an entity falls onto the block.
    /// </summary>
    public virtual void OnFallOn(BlockFallContext context)
    {
    }

    /// <summary>
    /// Called before the block is removed from the world.
    /// </summary>
    public virtual void OnRemoving(BlockRemovingContext context)
    {
    }

    /// <summary>
    /// Called after the block is removed from the world.
    /// </summary>
    public virtual void OnRemoved(BlockRemoveContext context)
    {
    }

    /// <summary>
    /// Called when the block is destroyed by the world (non-player).
    /// </summary>
    public virtual void OnDestroyed(BlockDestroyContext context)
    {
    }

    /// <summary>
    /// Called when the block is destroyed by a player.
    /// </summary>
    public virtual void OnPlayerDestroy(BlockPlayerDestroyContext context)
    {
    }

    /// <summary>
    /// Called just before a player destroys the block.
    /// </summary>
    public virtual void OnPlayerWillDestroy(BlockPlayerWillDestroyContext context)
    {
    }

    /// <summary>
    /// Called when the block is placed by a player with an item.
    /// </summary>
    public virtual void OnPlacedBy(BlockPlacedByContext context)
    {
    }

}

/// <summary>Managed falling block base class.</summary>
public class FallingBlock : Block
{
}

/// <summary>Managed slab block base class.</summary>
public class SlabBlock : Block
{
}

/// <summary>
/// Result of managed block action callbacks.
/// </summary>
public enum BlockActionResult
{
    ContinueVanilla = 0,
    CancelVanilla = 1
}

/// <summary>
/// Runtime context for block update callbacks.
/// </summary>
public readonly struct BlockUpdateContext
{
    public int BlockId { get; }
    public bool IsClientSide { get; }
    public nint NativeLevelPtr { get; }
    public int X { get; }
    public int Y { get; }
    public int Z { get; }

    internal BlockUpdateContext(int blockId, bool isClientSide, nint nativeLevelPtr, int x, int y, int z)
    {
        BlockId = blockId;
        IsClientSide = isClientSide;
        NativeLevelPtr = nativeLevelPtr;
        X = x;
        Y = y;
        Z = z;
    }

    public bool HasNeighborSignal()
    {
        if (NativeLevelPtr == 0)
            return false;

        return NativeInterop.native_level_has_neighbor_signal(NativeLevelPtr, X, Y, Z) != 0;
    }

    /// <summary>
    /// Set the block at this context's position.
    /// </summary>
    public bool SetBlock(Identifier id, int data = 0, int flags = 2)
    {
        int numericId = IdHelper.GetBlockNumericId(id);
        if (numericId < 0)
            return false;

        return SetBlock(numericId, data, flags);
    }

    /// <summary>
    /// Set the block at this context's position using a numeric ID.
    /// </summary>
    public bool SetBlock(int numericBlockId, int data = 0, int flags = 2)
    {
        if (NativeLevelPtr == 0 || numericBlockId < 0)
            return false;

        return NativeInterop.native_level_set_tile(NativeLevelPtr, X, Y, Z, numericBlockId, data, flags) != 0;
    }

    /// <summary>
    /// Schedule a tick for this block after a delay (in ticks).
    /// </summary>
    public bool ScheduleTick(int delay)
    {
        if (NativeLevelPtr == 0 || delay < 0)
            return false;

        return NativeInterop.native_level_schedule_tick(NativeLevelPtr, X, Y, Z, BlockId, delay) != 0;
    }

    /// <summary>
    /// Read a block ID relative to this context's position.
    /// </summary>
    public int GetBlockId(int offsetX = 0, int offsetY = 0, int offsetZ = 0)
    {
        if (NativeLevelPtr == 0)
            return -1;

        return NativeInterop.native_level_get_tile(NativeLevelPtr, X + offsetX, Y + offsetY, Z + offsetZ);
    }
}

/// <summary>
/// Runtime context for block neighbor-change callback.
/// </summary>
public readonly struct BlockNeighborChangedContext
{
    public BlockUpdateContext Block { get; }
    public int NeighborBlockId { get; }

    internal BlockNeighborChangedContext(BlockUpdateContext block, int neighborBlockId)
    {
        Block = block;
        NeighborBlockId = neighborBlockId;
    }
}

/// <summary>
/// Runtime context for scheduled tick callback.
/// </summary>
public readonly struct BlockTickContext
{
    public BlockUpdateContext Block { get; }

    internal BlockTickContext(BlockUpdateContext block)
    {
        Block = block;
    }
}

/// <summary>
/// Runtime context for block use callback.
/// </summary>
public readonly struct BlockUseContext
{
    public BlockUpdateContext Block { get; }
    public nint NativePlayerPtr { get; }
    public int Face { get; }
    public float ClickX { get; }
    public float ClickY { get; }
    public float ClickZ { get; }
    public bool SoundOnly { get; }

    internal BlockUseContext(BlockUpdateContext block, nint nativePlayerPtr, int face, float clickX, float clickY, float clickZ, bool soundOnly)
    {
        Block = block;
        NativePlayerPtr = nativePlayerPtr;
        Face = face;
        ClickX = clickX;
        ClickY = clickY;
        ClickZ = clickZ;
        SoundOnly = soundOnly;
    }
}

/// <summary>
/// Runtime context for entity-on-block callbacks.
/// </summary>
public readonly struct BlockEntityContext
{
    public BlockUpdateContext Block { get; }
    public nint NativeEntityPtr { get; }

    internal BlockEntityContext(BlockUpdateContext block, nint nativeEntityPtr)
    {
        Block = block;
        NativeEntityPtr = nativeEntityPtr;
    }
}

/// <summary>
/// Runtime context for block fall-on callback.
/// </summary>
public readonly struct BlockFallContext
{
    public BlockUpdateContext Block { get; }
    public nint NativeEntityPtr { get; }
    public float FallDistance { get; }

    internal BlockFallContext(BlockUpdateContext block, nint nativeEntityPtr, float fallDistance)
    {
        Block = block;
        NativeEntityPtr = nativeEntityPtr;
        FallDistance = fallDistance;
    }
}

/// <summary>
/// Runtime context for block removing callback.
/// </summary>
public readonly struct BlockRemovingContext
{
    public BlockUpdateContext Block { get; }
    public int BlockData { get; }

    internal BlockRemovingContext(BlockUpdateContext block, int blockData)
    {
        Block = block;
        BlockData = blockData;
    }
}

/// <summary>
/// Runtime context for block removed callback.
/// </summary>
public readonly struct BlockRemoveContext
{
    public BlockUpdateContext Block { get; }
    public int RemovedBlockId { get; }
    public int RemovedBlockData { get; }

    internal BlockRemoveContext(BlockUpdateContext block, int removedBlockId, int removedBlockData)
    {
        Block = block;
        RemovedBlockId = removedBlockId;
        RemovedBlockData = removedBlockData;
    }
}

/// <summary>
/// Runtime context for block destroyed callback.
/// </summary>
public readonly struct BlockDestroyContext
{
    public BlockUpdateContext Block { get; }
    public int BlockData { get; }

    internal BlockDestroyContext(BlockUpdateContext block, int blockData)
    {
        Block = block;
        BlockData = blockData;
    }
}

/// <summary>
/// Runtime context for player-destroyed callback.
/// </summary>
public readonly struct BlockPlayerDestroyContext
{
    public BlockUpdateContext Block { get; }
    public nint NativePlayerPtr { get; }
    public int BlockData { get; }

    internal BlockPlayerDestroyContext(BlockUpdateContext block, nint nativePlayerPtr, int blockData)
    {
        Block = block;
        NativePlayerPtr = nativePlayerPtr;
        BlockData = blockData;
    }
}

/// <summary>
/// Runtime context for player-will-destroy callback.
/// </summary>
public readonly struct BlockPlayerWillDestroyContext
{
    public BlockUpdateContext Block { get; }
    public nint NativePlayerPtr { get; }
    public int BlockData { get; }

    internal BlockPlayerWillDestroyContext(BlockUpdateContext block, nint nativePlayerPtr, int blockData)
    {
        Block = block;
        NativePlayerPtr = nativePlayerPtr;
        BlockData = blockData;
    }
}

/// <summary>
/// Runtime context for block placed-by callback.
/// </summary>
public readonly struct BlockPlacedByContext
{
    public BlockUpdateContext Block { get; }
    public nint NativePlacerPtr { get; }
    public nint NativeItemInstancePtr { get; }

    internal BlockPlacedByContext(BlockUpdateContext block, nint nativePlacerPtr, nint nativeItemInstancePtr)
    {
        Block = block;
        NativePlacerPtr = nativePlacerPtr;
        NativeItemInstancePtr = nativeItemInstancePtr;
    }
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockUpdateNativeArgs
{
    public int BlockId;
    public int IsClientSide;
    public nint LevelPtr;
    public int X;
    public int Y;
    public int Z;
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockNeighborChangedNativeArgs
{
    public BlockUpdateNativeArgs Block;
    public int NeighborBlockId;
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockUseNativeArgs
{
    public BlockUpdateNativeArgs Block;
    public nint PlayerPtr;
    public int Face;
    public float ClickX;
    public float ClickY;
    public float ClickZ;
    public int SoundOnly;
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockEntityNativeArgs
{
    public BlockUpdateNativeArgs Block;
    public nint EntityPtr;
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockFallNativeArgs
{
    public BlockUpdateNativeArgs Block;
    public nint EntityPtr;
    public float FallDistance;
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockRemovingNativeArgs
{
    public BlockUpdateNativeArgs Block;
    public int BlockData;
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockRemoveNativeArgs
{
    public BlockUpdateNativeArgs Block;
    public int RemovedBlockId;
    public int RemovedBlockData;
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockDestroyNativeArgs
{
    public BlockUpdateNativeArgs Block;
    public int BlockData;
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockPlayerDestroyNativeArgs
{
    public BlockUpdateNativeArgs Block;
    public nint PlayerPtr;
    public int BlockData;
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockPlayerWillDestroyNativeArgs
{
    public BlockUpdateNativeArgs Block;
    public nint PlayerPtr;
    public int BlockData;
}

[StructLayout(LayoutKind.Sequential)]
internal struct BlockPlacedByNativeArgs
{
    public BlockUpdateNativeArgs Block;
    public nint PlacerPtr;
    public nint ItemInstancePtr;
}

internal static class ManagedBlockDispatcher
{
    private static readonly object s_lock = new();
    private static readonly Dictionary<int, Block> s_blocks = new();

    internal static void RegisterBlock(Identifier id, int numericId, Block block)
    {
        block.Id = id;
        block.NumericId = numericId;

        lock (s_lock)
        {
            s_blocks[numericId] = block;
        }
    }

    internal static int HandlePlace(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockUpdateNativeArgs>())
            return 0;

        BlockUpdateNativeArgs nativeArgs = Marshal.PtrToStructure<BlockUpdateNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.BlockId, out block);
        }

        if (block == null)
            return 0;

        block.OnPlace(new BlockUpdateContext(
            nativeArgs.BlockId,
            nativeArgs.IsClientSide != 0,
            nativeArgs.LevelPtr,
            nativeArgs.X,
            nativeArgs.Y,
            nativeArgs.Z));
        return 1;
    }

    internal static int HandleNeighborChanged(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockNeighborChangedNativeArgs>())
            return 0;

        BlockNeighborChangedNativeArgs nativeArgs = Marshal.PtrToStructure<BlockNeighborChangedNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);
        block.OnNeighborChanged(new BlockNeighborChangedContext(update, nativeArgs.NeighborBlockId));
        return 1;
    }

    internal static int HandleScheduledTick(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockUpdateNativeArgs>())
            return 0;

        BlockUpdateNativeArgs nativeArgs = Marshal.PtrToStructure<BlockUpdateNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.BlockId, out block);
        }

        if (block == null)
            return 0;

        block.OnScheduledTick(new BlockTickContext(new BlockUpdateContext(
            nativeArgs.BlockId,
            nativeArgs.IsClientSide != 0,
            nativeArgs.LevelPtr,
            nativeArgs.X,
            nativeArgs.Y,
            nativeArgs.Z)));
        return 1;
    }

    internal static int HandleUse(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockUseNativeArgs>())
            return 0;

        BlockUseNativeArgs nativeArgs = Marshal.PtrToStructure<BlockUseNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);

        var result = block.OnUse(new BlockUseContext(
            update,
            nativeArgs.PlayerPtr,
            nativeArgs.Face,
            nativeArgs.ClickX,
            nativeArgs.ClickY,
            nativeArgs.ClickZ,
            nativeArgs.SoundOnly != 0));

        return result == BlockActionResult.CancelVanilla ? 2 : 1;
    }

    internal static int HandleStepOn(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockEntityNativeArgs>())
            return 0;

        BlockEntityNativeArgs nativeArgs = Marshal.PtrToStructure<BlockEntityNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);
        block.OnStepOn(new BlockEntityContext(update, nativeArgs.EntityPtr));
        return 1;
    }

    internal static int HandleEntityInsideTile(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockEntityNativeArgs>())
            return 0;

        BlockEntityNativeArgs nativeArgs = Marshal.PtrToStructure<BlockEntityNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);
        block.OnEntityInsideTile(new BlockEntityContext(update, nativeArgs.EntityPtr));
        return 1;
    }

    internal static int HandleFallOn(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockFallNativeArgs>())
            return 0;

        BlockFallNativeArgs nativeArgs = Marshal.PtrToStructure<BlockFallNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);
        block.OnFallOn(new BlockFallContext(update, nativeArgs.EntityPtr, nativeArgs.FallDistance));
        return 1;
    }

    internal static int HandleRemoving(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockRemovingNativeArgs>())
            return 0;

        BlockRemovingNativeArgs nativeArgs = Marshal.PtrToStructure<BlockRemovingNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);
        block.OnRemoving(new BlockRemovingContext(update, nativeArgs.BlockData));
        return 1;
    }

    internal static int HandleRemoved(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockRemoveNativeArgs>())
            return 0;

        BlockRemoveNativeArgs nativeArgs = Marshal.PtrToStructure<BlockRemoveNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);
        block.OnRemoved(new BlockRemoveContext(update, nativeArgs.RemovedBlockId, nativeArgs.RemovedBlockData));
        return 1;
    }

    internal static int HandleDestroyed(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockDestroyNativeArgs>())
            return 0;

        BlockDestroyNativeArgs nativeArgs = Marshal.PtrToStructure<BlockDestroyNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);
        block.OnDestroyed(new BlockDestroyContext(update, nativeArgs.BlockData));
        return 1;
    }

    internal static int HandlePlayerDestroy(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockPlayerDestroyNativeArgs>())
            return 0;

        BlockPlayerDestroyNativeArgs nativeArgs = Marshal.PtrToStructure<BlockPlayerDestroyNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);
        block.OnPlayerDestroy(new BlockPlayerDestroyContext(update, nativeArgs.PlayerPtr, nativeArgs.BlockData));
        return 1;
    }

    internal static int HandlePlayerWillDestroy(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockPlayerWillDestroyNativeArgs>())
            return 0;

        BlockPlayerWillDestroyNativeArgs nativeArgs = Marshal.PtrToStructure<BlockPlayerWillDestroyNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);
        block.OnPlayerWillDestroy(new BlockPlayerWillDestroyContext(update, nativeArgs.PlayerPtr, nativeArgs.BlockData));
        return 1;
    }

    internal static int HandlePlacedBy(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < Marshal.SizeOf<BlockPlacedByNativeArgs>())
            return 0;

        BlockPlacedByNativeArgs nativeArgs = Marshal.PtrToStructure<BlockPlacedByNativeArgs>(args);
        Block? block;
        lock (s_lock)
        {
            s_blocks.TryGetValue(nativeArgs.Block.BlockId, out block);
        }

        if (block == null)
            return 0;

        var update = new BlockUpdateContext(
            nativeArgs.Block.BlockId,
            nativeArgs.Block.IsClientSide != 0,
            nativeArgs.Block.LevelPtr,
            nativeArgs.Block.X,
            nativeArgs.Block.Y,
            nativeArgs.Block.Z);
        block.OnPlacedBy(new BlockPlacedByContext(update, nativeArgs.PlacerPtr, nativeArgs.ItemInstancePtr));
        return 1;
    }


}
