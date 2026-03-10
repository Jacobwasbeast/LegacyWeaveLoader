namespace WeaveLoader.API.Native;

public static class NativeLevel
{
    public static int GetTile(nint levelPtr, int x, int y, int z)
        => NativeInterop.native_level_get_tile(levelPtr, x, y, z);

    public static bool SetTile(nint levelPtr, int x, int y, int z, int blockId, int data = 0, int flags = 2)
        => NativeInterop.native_level_set_tile(levelPtr, x, y, z, blockId, data, flags) != 0;
}
