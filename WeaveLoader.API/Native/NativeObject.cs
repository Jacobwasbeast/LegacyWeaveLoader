using System;
using System.Collections.Generic;

namespace WeaveLoader.API.Native;

public sealed class NativeObject
{
    public string ClassName { get; }
    public nint Pointer { get; }
    private readonly Dictionary<string, nint> _methodCache = new();

    public NativeObject(string className, nint pointer)
    {
        ClassName = className;
        Pointer = pointer;
    }

    public bool HasMethod(string method)
        => Resolve(BuildFullName(method)) != 0;

    public bool CallVoid(string method, params object[] args)
    {
        string fullName = BuildFullName(method);
        nint fn = Resolve(fullName);
        if (fn == 0)
            return false;
        if (!TryBuildArgs(args, out var nativeArgs))
            return false;
        return NativeInvoker.TryInvoke(fn, Pointer, true, NativeType.I32, nativeArgs, out _);
    }

    public T Call<T>(string method, params object[] args)
    {
        string fullName = BuildFullName(method);
        nint fn = Resolve(fullName);
        if (fn == 0)
            return default!;
        if (!TryBuildArgs(args, out var nativeArgs))
            return default!;
        NativeType retType = GetReturnType(typeof(T));
        if (!NativeInvoker.TryInvoke(fn, Pointer, true, retType, nativeArgs, out var ret))
            return default!;
        return ConvertReturn<T>(ret);
    }

    private string BuildFullName(string method)
        => method.Contains("::") ? method : $"{ClassName}::{method}";

    private nint Resolve(string fullName)
    {
        if (_methodCache.TryGetValue(fullName, out var cached))
            return cached;
        nint fn = NativeSymbol.Find(fullName);
        _methodCache[fullName] = fn;
        return fn;
    }

    internal static bool TryBuildArgs(object[] args, out NativeArg[] nativeArgs)
    {
        nativeArgs = new NativeArg[args.Length];
        for (int i = 0; i < args.Length; ++i)
        {
            object arg = args[i];
            switch (arg)
            {
            case int v:
                nativeArgs[i] = NativeArg.FromInt(v);
                break;
            case long v:
                nativeArgs[i] = NativeArg.FromLong(v);
                break;
            case bool v:
                nativeArgs[i] = NativeArg.FromBool(v);
                break;
            case nint v:
                nativeArgs[i] = NativeArg.FromPtr(v);
                break;
            default:
                return false;
            }
        }
        return true;
    }

    internal static NativeType GetReturnType(Type t)
    {
        if (t == typeof(int))
            return NativeType.I32;
        if (t == typeof(long))
            return NativeType.I64;
        if (t == typeof(bool))
            return NativeType.Bool;
        if (t == typeof(nint) || t == typeof(IntPtr))
            return NativeType.Ptr;
        return NativeType.I32;
    }

    internal static T ConvertReturn<T>(NativeRet ret)
    {
        object value = ret.Type switch
        {
            NativeType.Bool => ret.AsBool(),
            NativeType.I64 => ret.AsLong(),
            NativeType.Ptr => typeof(T) == typeof(IntPtr) ? (object)(IntPtr)ret.AsPtr() : ret.AsPtr(),
            _ => ret.AsInt()
        };
        return (T)value;
    }
}
