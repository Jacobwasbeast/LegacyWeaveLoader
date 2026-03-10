using System.Runtime.InteropServices;
using WeaveLoader.API;
using WeaveLoader.API.Events;
using WeaveLoader.API.Block;
using WeaveLoader.API.Item;

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
            MixinLoader.LoadMixins(discovered);
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

    public static int OnItemMineBlock(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedItemDispatcher.HandleMineBlock(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnItemMineBlock EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnItemUse(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedItemDispatcher.HandleUseItem(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnItemUse EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockPlace(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandlePlace(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockPlace EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockNeighborChanged(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandleNeighborChanged(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockNeighborChanged EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockTick(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandleScheduledTick(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockTick EXCEPTION: {ex}");
            return 0;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct WorldLoadedNativeArgs
    {
        public IntPtr LevelPtr;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct EntitySummonedNativeArgs
    {
        public int EntityNumericId;
        public float X;
        public float Y;
        public float Z;
    }

    public static int OnWorldLoaded(IntPtr args, int sizeBytes)
    {
        try
        {
            var native = Marshal.PtrToStructure<WorldLoadedNativeArgs>(args);
            GameEvents.FireWorldLoaded(new WorldLoadedEventArgs
            {
                NativeLevelPointer = native.LevelPtr
            });
            return 0;
        }
        catch (Exception ex)
        {
            Logger.Error($"OnWorldLoaded EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnWorldUnloaded(IntPtr args, int sizeBytes)
    {
        try
        {
            GameEvents.FireWorldUnloaded(new WorldUnloadedEventArgs());
            return 0;
        }
        catch (Exception ex)
        {
            Logger.Error($"OnWorldUnloaded EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnEntitySummoned(IntPtr args, int sizeBytes)
    {
        try
        {
            var native = Marshal.PtrToStructure<EntitySummonedNativeArgs>(args);
            GameEvents.FireEntitySpawn(new EntitySpawnEventArgs
            {
                EntityId = $"entity:{native.EntityNumericId}",
                X = native.X,
                Y = native.Y,
                Z = native.Z
            });
            return 0;
        }
        catch (Exception ex)
        {
            Logger.Error($"OnEntitySummoned EXCEPTION: {ex}");
            return 0;
        }
    }
}
