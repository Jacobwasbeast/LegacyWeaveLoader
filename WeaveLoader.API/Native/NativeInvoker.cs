namespace WeaveLoader.API.Native;

internal static class NativeInvoker
{
    internal static bool TryInvoke(string fullName, nint thisPtr, bool hasThis, NativeType retType, NativeArg[] args, out NativeRet ret)
    {
        ret = new NativeRet { Type = retType };
        nint fn = NativeSymbol.Find(fullName);
        if (fn == 0)
            return false;
        int ok = NativeInterop.native_invoke(fn, thisPtr, hasThis ? 1 : 0, args, args.Length, ref ret);
        return ok != 0;
    }

    internal static bool TryInvoke(nint fn, nint thisPtr, bool hasThis, NativeType retType, NativeArg[] args, out NativeRet ret)
    {
        ret = new NativeRet { Type = retType };
        if (fn == 0)
            return false;
        int ok = NativeInterop.native_invoke(fn, thisPtr, hasThis ? 1 : 0, args, args.Length, ref ret);
        return ok != 0;
    }
}
