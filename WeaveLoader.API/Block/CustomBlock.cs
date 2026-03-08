using System.Runtime.InteropServices;

namespace WeaveLoader.API.Block;

public abstract class Block
{
    public Identifier? Id { get; internal set; }
    public int NumericId { get; internal set; } = -1;

    public Identifier? DropAsBlockId { get; init; }
    public Identifier? CloneAsBlockId { get; init; }

    public virtual void OnPlace(BlockUpdateContext context)
    {
    }

    public virtual void OnNeighborChanged(BlockNeighborChangedContext context)
    {
    }

    public virtual void OnScheduledTick(BlockTickContext context)
    {
    }
}

public class FallingBlock : Block
{
}

public class SlabBlock : Block
{
}

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

    public bool SetBlock(Identifier id, int data = 0, int flags = 2)
    {
        int numericId = IdHelper.GetBlockNumericId(id);
        if (numericId < 0)
            return false;

        return SetBlock(numericId, data, flags);
    }

    public bool SetBlock(int numericBlockId, int data = 0, int flags = 2)
    {
        if (NativeLevelPtr == 0 || numericBlockId < 0)
            return false;

        return NativeInterop.native_level_set_tile(NativeLevelPtr, X, Y, Z, numericBlockId, data, flags) != 0;
    }

    public bool ScheduleTick(int delay)
    {
        if (NativeLevelPtr == 0 || delay < 0)
            return false;

        return NativeInterop.native_level_schedule_tick(NativeLevelPtr, X, Y, Z, BlockId, delay) != 0;
    }

    public int GetBlockId(int offsetX = 0, int offsetY = 0, int offsetZ = 0)
    {
        if (NativeLevelPtr == 0)
            return -1;

        return NativeInterop.native_level_get_tile(NativeLevelPtr, X + offsetX, Y + offsetY, Z + offsetZ);
    }
}

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

public readonly struct BlockTickContext
{
    public BlockUpdateContext Block { get; }

    internal BlockTickContext(BlockUpdateContext block)
    {
        Block = block;
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
}
