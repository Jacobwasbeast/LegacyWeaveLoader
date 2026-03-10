using System.Collections.Generic;

namespace WeaveLoader.API.Native;

public sealed class NativeClass
{
    public string Name { get; }
    private readonly Dictionary<string, nint> _methodCache = new();

    public NativeClass(string name)
    {
        Name = name;
    }

    public bool HasMethod(string method)
        => Resolve(BuildFullName(method)) != 0;

    public bool CallVoid(string method, params object[] args)
    {
        string fullName = BuildFullName(method);
        nint fn = Resolve(fullName);
        if (fn == 0)
            return false;
        if (!NativeObject.TryBuildArgs(args, out var nativeArgs))
            return false;
        return NativeInvoker.TryInvoke(fn, 0, false, NativeType.I32, nativeArgs, out _);
    }

    public T Call<T>(string method, params object[] args)
    {
        string fullName = BuildFullName(method);
        nint fn = Resolve(fullName);
        if (fn == 0)
            return default!;
        if (!NativeObject.TryBuildArgs(args, out var nativeArgs))
            return default!;
        NativeType retType = NativeObject.GetReturnType(typeof(T));
        if (!NativeInvoker.TryInvoke(fn, 0, false, retType, nativeArgs, out var ret))
            return default!;
        return NativeObject.ConvertReturn<T>(ret);
    }

    private string BuildFullName(string method)
        => method.Contains("::") ? method : $"{Name}::{method}";

    private nint Resolve(string fullName)
    {
        if (_methodCache.TryGetValue(fullName, out var cached))
            return cached;
        nint fn = NativeSymbol.Find(fullName);
        _methodCache[fullName] = fn;
        return fn;
    }
}
