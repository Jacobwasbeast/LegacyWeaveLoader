using System;
using System.Runtime.InteropServices;
using WeaveLoader.API.Native;

namespace WeaveLoader.API.Mixins;

[StructLayout(LayoutKind.Sequential)]
internal struct MixinContextNative
{
    public nint ThisPtr;
    public nint Args;
    public int ArgCount;
    public nint Ret;
    public int Cancel;
}

public sealed class MixinContext
{
    private readonly nint _ctxPtr;

    internal MixinContext(nint ctxPtr)
    {
        _ctxPtr = ctxPtr;
    }

    public nint ThisPtr => Read().ThisPtr;
    public int ArgCount => Read().ArgCount;

    public NativeArg GetArg(int index)
    {
        var ctx = Read();
        int size = Marshal.SizeOf<NativeArg>();
        nint ptr = ctx.Args + index * size;
        return Marshal.PtrToStructure<NativeArg>(ptr);
    }

    public void SetArg(int index, NativeArg arg)
    {
        var ctx = Read();
        int size = Marshal.SizeOf<NativeArg>();
        nint ptr = ctx.Args + index * size;
        Marshal.StructureToPtr(arg, ptr, false);
    }

    public NativeRet GetReturn()
    {
        var ctx = Read();
        return Marshal.PtrToStructure<NativeRet>(ctx.Ret);
    }

    public void SetReturn(NativeRet ret)
    {
        var ctx = Read();
        Marshal.StructureToPtr(ret, ctx.Ret, false);
    }

    public void Cancel()
    {
        var ctx = Read();
        ctx.Cancel = 1;
        Marshal.StructureToPtr(ctx, _ctxPtr, false);
    }

    private MixinContextNative Read()
    {
        return Marshal.PtrToStructure<MixinContextNative>(_ctxPtr);
    }
}
