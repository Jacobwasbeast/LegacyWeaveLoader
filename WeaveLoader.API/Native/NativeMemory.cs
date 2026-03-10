using System;
using System.Runtime.InteropServices;

namespace WeaveLoader.API.Native;

public static class NativeMemory
{
    public static byte ReadByte(nint basePtr, int offset)
        => Marshal.ReadByte(basePtr, offset);

    public static int ReadInt32(nint basePtr, int offset)
        => Marshal.ReadInt32(basePtr, offset);

    public static long ReadInt64(nint basePtr, int offset)
        => Marshal.ReadInt64(basePtr, offset);

    public static double ReadDouble(nint basePtr, int offset)
        => BitConverter.Int64BitsToDouble(ReadInt64(basePtr, offset));

    public static bool ReadBool(nint basePtr, int offset)
        => ReadByte(basePtr, offset) != 0;

    public static nint ReadPtr(nint basePtr, int offset)
        => Marshal.ReadIntPtr(basePtr, offset);
}
