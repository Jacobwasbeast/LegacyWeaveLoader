using System.Reflection;
using System.Runtime.InteropServices;
using System.Text.Json;
using WeaveLoader.API;
using WeaveLoader.API.Mixins;

namespace WeaveLoader.Core;

internal static class MixinLoader
{
    private sealed class MixinConfig
    {
        public bool Required { get; set; } = false;
        public string? Package { get; set; }
        public string[]? Mixins { get; set; }
        public string[]? Client { get; set; }
        public string[]? Server { get; set; }
        public InjectorConfig? Injectors { get; set; }
    }

    private sealed class InjectorConfig
    {
        public int DefaultRequire { get; set; } = 0;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct MixinContextNative
    {
        public nint ThisPtr;
        public nint Args;
        public int ArgCount;
        public nint Ret;
        public int Cancel;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void MixinCallback(ref MixinContextNative ctx);

    private sealed class CallbackWrapper
    {
        private readonly MethodInfo _method;

        public CallbackWrapper(MethodInfo method)
        {
            _method = method;
        }

        public unsafe void Invoke(ref MixinContextNative ctx)
        {
            object?[]? args = null;
            if (_method.GetParameters().Length == 1)
            {
                fixed (MixinContextNative* pCtx = &ctx)
                {
                    args = new object?[] { new MixinContext((nint)pCtx) };
                }
            }
            _method.Invoke(null, args);
        }
    }

    private static readonly List<Delegate> s_callbacks = new();
    private static readonly JsonSerializerOptions s_jsonOptions = new()
    {
        PropertyNameCaseInsensitive = true
    };

    internal static void LoadMixins(IEnumerable<ModDiscovery.DiscoveredMod> mods)
    {
        foreach (var mod in mods)
            LoadMixinsForMod(mod);
    }

    private static void LoadMixinsForMod(ModDiscovery.DiscoveredMod mod)
    {
        if (string.IsNullOrWhiteSpace(mod.Folder))
            return;

        string configPath = Path.Combine(mod.Folder, "weave.mixins.json");
        if (!File.Exists(configPath))
            return;

        MixinConfig? config;
        try
        {
            config = JsonSerializer.Deserialize<MixinConfig>(File.ReadAllText(configPath), s_jsonOptions);
        }
        catch (Exception ex)
        {
            Logger.Error($"Failed to parse mixin config for {mod.Metadata.Id}: {ex.Message}");
            return;
        }

        if (config == null || string.IsNullOrWhiteSpace(config.Package))
            return;

        Logger.Info($"Loading mixins for {mod.Metadata.Id} from {configPath}");

        var mixinTypes = new List<string>();
        if (config.Mixins != null) mixinTypes.AddRange(config.Mixins);
        if (config.Client != null) mixinTypes.AddRange(config.Client);
        if (config.Server != null) mixinTypes.AddRange(config.Server);

        foreach (string mixinName in mixinTypes)
        {
            string fullTypeName = $"{config.Package}.{mixinName}";
            Type? type = mod.Assembly.GetType(fullTypeName, false);
            if (type == null)
            {
                Logger.Warning($"Mixin type not found: {fullTypeName}");
                continue;
            }

            var mixinAttr = type.GetCustomAttribute<MixinAttribute>();
            if (mixinAttr == null)
            {
                Logger.Warning($"Mixin missing [Mixin] attribute: {fullTypeName}");
                continue;
            }

            Logger.Info($"Processing mixin: {fullTypeName} -> {mixinAttr.Target}");

            foreach (MethodInfo method in type.GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static))
            {
                var inject = method.GetCustomAttribute<InjectAttribute>();
                if (inject == null)
                    continue;

                if (!method.IsStatic)
                {
                    Logger.Warning($"Mixin method must be static: {method.DeclaringType?.FullName}.{method.Name}");
                    continue;
                }

                if (method.GetParameters().Length > 1)
                {
                    Logger.Warning($"Mixin method has too many parameters: {method.DeclaringType?.FullName}.{method.Name}");
                    continue;
                }

                if (method.GetParameters().Length == 1 &&
                    method.GetParameters()[0].ParameterType != typeof(MixinContext))
                {
                    Logger.Warning($"Mixin method parameter must be MixinContext: {method.DeclaringType?.FullName}.{method.Name}");
                    continue;
                }

                string targetName = mixinAttr.Target;
                string fullName = inject.Method.Contains("::") ? inject.Method : $"{targetName}::{inject.Method}";
                int require = inject.Require > 0 ? inject.Require : (config.Injectors?.DefaultRequire ?? 0);

                if (NativeInterop.native_has_symbol(fullName) == 0)
                {
                    Logger.Warning($"Mixin target not found in mapping: {fullName}");
                }

                var wrapper = new CallbackWrapper(method);
                MixinCallback callback = wrapper.Invoke;
                nint fnPtr = Marshal.GetFunctionPointerForDelegate(callback);
                s_callbacks.Add(callback);

                int ok = NativeInterop.native_mixin_add(fullName, (int)inject.At, fnPtr, require);
                if (ok == 0)
                    Logger.Warning($"Mixin hook failed: {fullName}");
                else
                    Logger.Info($"Mixin hook installed: {fullName} ({inject.At})");
            }
        }
    }
}
