using System.Runtime.InteropServices;

namespace WeaveLoader.API.Native;

public enum NativeType : byte
{
    I32 = 1,
    I64 = 2,
    F32 = 3,
    F64 = 4,
    Ptr = 5,
    Bool = 6
}

[StructLayout(LayoutKind.Sequential)]
public struct NativeArg
{
    public NativeType Type;
    public ulong Value;

    public static NativeArg FromInt(int value) => new() { Type = NativeType.I32, Value = unchecked((ulong)value) };
    public static NativeArg FromLong(long value) => new() { Type = NativeType.I64, Value = unchecked((ulong)value) };
    public static NativeArg FromBool(bool value) => new() { Type = NativeType.Bool, Value = value ? 1UL : 0UL };
    public static NativeArg FromPtr(nint value) => new() { Type = NativeType.Ptr, Value = unchecked((ulong)value) };
}

[StructLayout(LayoutKind.Sequential)]
public struct NativeRet
{
    public NativeType Type;
    public ulong Value;

    public int AsInt() => unchecked((int)Value);
    public long AsLong() => unchecked((long)Value);
    public bool AsBool() => Value != 0;
    public nint AsPtr() => unchecked((nint)Value);
}
