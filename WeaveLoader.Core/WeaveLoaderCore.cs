using System.Runtime.InteropServices;
using WeaveLoader.API;

namespace WeaveLoader.Core;

public static class WeaveLoaderCore
{
    private static ModManager? _modManager;
    private static bool _initialized;

    public static int Initialize(IntPtr args, int sizeBytes)
    {
        if (_initialized) return 0;
        _initialized = true;

        Logger.SetLogHandler((message, level) =>
        {
            string formatted = $"[WeaveLoader/{level}] {message}";
            try
            {
                NativeInterop.native_log(formatted, (int)level);
            }
            catch
            {
                Console.WriteLine(formatted);
            }
        });

        Logger.Info("WeaveLoader Core initialized");
        _modManager = new ModManager();
        return 0;
    }

    public static int DiscoverMods(IntPtr args, int sizeBytes)
    {
        try
        {
            string modsPath;
            if (args != IntPtr.Zero && sizeBytes > 0)
                modsPath = Marshal.PtrToStringUTF8(args, sizeBytes) ?? "mods";
            else
                modsPath = "mods";

            Logger.Info($"Discovering mods in: {modsPath}");
            Logger.Info($"Directory exists: {Directory.Exists(modsPath)}");

            if (Directory.Exists(modsPath))
            {
                var files = Directory.GetFiles(modsPath, "*.dll");
                Logger.Info($"DLL files found: {string.Join(", ", files.Select(Path.GetFileName))}");
            }

            var discovered = ModDiscovery.DiscoverMods(modsPath);
            _modManager?.AddMods(discovered);
            Logger.Info($"Loaded {discovered.Count} mod(s)");
            return discovered.Count;
        }
        catch (Exception ex)
        {
            Logger.Error($"DiscoverMods EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int PreInit(IntPtr args, int sizeBytes)
    {
        _modManager?.PreInit();
        return 0;
    }

    public static int Init(IntPtr args, int sizeBytes)
    {
        _modManager?.Init();
        return 0;
    }

    public static int PostInit(IntPtr args, int sizeBytes)
    {
        _modManager?.PostInit();
        return 0;
    }

    public static int Tick(IntPtr args, int sizeBytes)
    {
        _modManager?.Tick();
        return 0;
    }

    public static int Shutdown(IntPtr args, int sizeBytes)
    {
        _modManager?.Shutdown();
        Logger.Info("WeaveLoader shut down.");
        return 0;
    }
}
