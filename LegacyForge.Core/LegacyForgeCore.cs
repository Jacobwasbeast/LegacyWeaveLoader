using System.Runtime.InteropServices;
using LegacyForge.API;

namespace LegacyForge.Core;

/// <summary>
/// Entry point class loaded by the C++ DotNetHost via hostfxr.
/// All public static methods here are resolved as function pointers from native code.
/// Method signatures must match the component_entry_point_fn delegate:
///     public delegate int ComponentEntryPoint(IntPtr args, int sizeBytes);
/// </summary>
public static class LegacyForgeCore
{
    private static ModManager? _modManager;
    private static bool _initialized;

    /// <summary>
    /// Called once by C++ to initialize the managed runtime.
    /// Sets up the native log handler and prepares the mod manager.
    /// </summary>
    public static int Initialize(IntPtr args, int sizeBytes)
    {
        if (_initialized) return 0;
        _initialized = true;

        Logger.LogHandler = (message, level) =>
        {
            string formatted = $"[LegacyForge/{level}] {message}";
            try
            {
                NativeInterop.native_log(formatted, (int)level);
            }
            catch
            {
                Console.WriteLine(formatted);
            }
        };

        Logger.Info("LegacyForge Core initialized");
        _modManager = new ModManager();
        return 0;
    }

    /// <summary>
    /// Called by C++ to discover and load mod assemblies from the mods/ directory.
    /// The mods path is passed as a UTF-8 string pointer.
    /// </summary>
    public static int DiscoverMods(IntPtr args, int sizeBytes)
    {
        string modsPath;
        if (args != IntPtr.Zero && sizeBytes > 0)
            modsPath = Marshal.PtrToStringUTF8(args, sizeBytes) ?? "mods";
        else
            modsPath = "mods";

        Logger.Info($"Discovering mods in: {modsPath}");
        var discovered = ModDiscovery.DiscoverMods(modsPath);
        _modManager?.AddMods(discovered);
        Logger.Info($"Loaded {discovered.Count} mod(s)");
        return discovered.Count;
    }

    /// <summary>Called before MinecraftWorld_RunStaticCtors.</summary>
    public static int PreInit(IntPtr args, int sizeBytes)
    {
        _modManager?.PreInit();
        return 0;
    }

    /// <summary>Called after MinecraftWorld_RunStaticCtors.</summary>
    public static int Init(IntPtr args, int sizeBytes)
    {
        _modManager?.Init();
        return 0;
    }

    /// <summary>Called after Minecraft::init completes.</summary>
    public static int PostInit(IntPtr args, int sizeBytes)
    {
        _modManager?.PostInit();
        return 0;
    }

    /// <summary>Called each game tick from the Minecraft::tick hook.</summary>
    public static int Tick(IntPtr args, int sizeBytes)
    {
        _modManager?.Tick();
        return 0;
    }

    /// <summary>Called from the Minecraft::destroy hook during shutdown.</summary>
    public static int Shutdown(IntPtr args, int sizeBytes)
    {
        _modManager?.Shutdown();
        Logger.Info("LegacyForge shut down.");
        return 0;
    }
}
