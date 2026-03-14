using System.Runtime.InteropServices;
using WeaveLoader.API;
using WeaveLoader.API.Events;
using WeaveLoader.API.Block;
using WeaveLoader.API.Item;
using WeaveLoader.Core.Loot;

namespace WeaveLoader.Core;

public static class WeaveLoaderCore
{
    private static readonly object _lootLogLock = new();
    private static readonly HashSet<string> _loggedEntityLoot = new(StringComparer.OrdinalIgnoreCase);
    private static readonly object _lootResultLock = new();
    private static readonly HashSet<string> _loggedEntityLootResult = new(StringComparer.OrdinalIgnoreCase);
    private static readonly object _lootMissingItemLock = new();
    private static readonly HashSet<string> _loggedMissingLootItems = new(StringComparer.OrdinalIgnoreCase);
    private static int _lootSpawnLogCount;
    private static ModManager? _modManager;
    private static bool _initialized;
    private static string _modsPath = "mods";
    private static LootSystem? _lootSystem;

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

            _modsPath = modsPath;
            _lootSystem = null;
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

    public static int OnItemUseOn(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedItemDispatcher.HandleUseOn(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnItemUseOn EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnItemInteractEntity(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedItemDispatcher.HandleInteractEntity(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnItemInteractEntity EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnItemHurtEntity(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedItemDispatcher.HandleHurtEntity(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnItemHurtEntity EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnItemInventoryTick(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedItemDispatcher.HandleInventoryTick(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnItemInventoryTick EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnItemCraftedBy(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedItemDispatcher.HandleCraftedBy(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnItemCraftedBy EXCEPTION: {ex}");
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

    public static int OnBlockUse(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandleUse(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockUse EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockStepOn(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandleStepOn(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockStepOn EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockEntityInsideTile(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandleEntityInsideTile(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockEntityInsideTile EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockFallOn(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandleFallOn(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockFallOn EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockRemoving(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandleRemoving(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockRemoving EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockRemoved(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandleRemoved(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockRemoved EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockDestroyed(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandleDestroyed(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockDestroyed EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockPlayerDestroy(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandlePlayerDestroy(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockPlayerDestroy EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockPlayerWillDestroy(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandlePlayerWillDestroy(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockPlayerWillDestroy EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockPlacedBy(IntPtr args, int sizeBytes)
    {
        try
        {
            return ManagedBlockDispatcher.HandlePlacedBy(args, sizeBytes);
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockPlacedBy EXCEPTION: {ex}");
            return 0;
        }
    }

    public static int OnBlockLoot(IntPtr args, int sizeBytes)
    {
        try
        {
            if (args == IntPtr.Zero || sizeBytes <= 0)
                return -1;

            var native = Marshal.PtrToStructure<BlockLootNativeArgs>(args);
            if (native.BlockNumericId < 0)
                return -1;

            if (!IdHelper.TryGetBlockIdentifier(native.BlockNumericId, out Identifier blockId))
                return -1;

            _lootSystem ??= new LootSystem(_modsPath);
            var result = _lootSystem.GetBlockLoot(blockId);
            if (result.Drops.Count == 0)
                return -1;

            Identifier dropId = result.Drops[0].ItemId;
            int numericId = IdHelper.GetItemNumericId(dropId);
            if (numericId < 0)
                numericId = IdHelper.GetBlockNumericId(dropId);

            return numericId;
        }
        catch (Exception ex)
        {
            Logger.Error($"OnBlockLoot EXCEPTION: {ex}");
            return -1;
        }
    }

    public static int OnEntityLoot(IntPtr args, int sizeBytes)
    {
        try
        {
            if (args == IntPtr.Zero || sizeBytes <= 0)
                return 0;

            var native = Marshal.PtrToStructure<EntityLootNativeArgs>(args);
            Identifier entityId;
            string? encodeId = Marshal.PtrToStringUTF8(native.EntityId);
            if (!string.IsNullOrWhiteSpace(encodeId))
            {
                entityId = LootSystem.NormalizeEntityId(encodeId);
            }
            else if (native.EntityNumericId >= 0 &&
                     IdHelper.TryGetEntityIdentifier(native.EntityNumericId, out Identifier numericEntityId))
            {
                entityId = numericEntityId;
            }
            else
            {
                return 0;
            }

            LogEntityLootOnce(entityId, encodeId, native.EntityNumericId);

            _lootSystem ??= new LootSystem(_modsPath);
            var result = _lootSystem.GetEntityLoot(entityId);
            LogEntityLootResultOnce(entityId, result);

            int spawnCount = 0;
            foreach (var drop in result.Drops)
            {
                int numericId = IdHelper.GetItemNumericId(drop.ItemId);
                if (numericId < 0)
                    numericId = IdHelper.GetBlockNumericId(drop.ItemId);
                if (numericId < 0)
                {
                    LogMissingLootItemOnce(entityId, drop.ItemId);
                    continue;
                }

                int logIndex = System.Threading.Interlocked.Increment(ref _lootSpawnLogCount);
                if (logIndex <= 100)
                {
                    Logger.Info($"Loot spawn attempt entity={entityId} item={drop.ItemId} numericId={numericId} count={drop.Count} aux={drop.Aux}");
                }

                int ok = NativeInterop.native_spawn_item_from_entity(
                    native.EntityPtr, numericId, drop.Count, drop.Aux);
                if (ok != 0)
                    spawnCount++;
            }

            if (result.OverrideVanilla)
                return 2;
            return spawnCount == 0 ? 0 : 1;
        }
        catch (Exception ex)
        {
            Logger.Error($"OnEntityLoot EXCEPTION: {ex}");
            return 0;
        }
    }

    private static void LogEntityLootOnce(Identifier entityId, string? encodeId, int numericId)
    {
        lock (_lootLogLock)
        {
            if (_loggedEntityLoot.Contains(entityId.ToString()))
                return;
            _loggedEntityLoot.Add(entityId.ToString());
        }

        string enc = string.IsNullOrWhiteSpace(encodeId) ? "<empty>" : encodeId!;
        Logger.Info($"EntityLoot resolved id={entityId} encodeId={enc} numericId={numericId}");
    }

    private static void LogEntityLootResultOnce(Identifier entityId, LootSystem.LootResult result)
    {
        lock (_lootResultLock)
        {
            if (_loggedEntityLootResult.Contains(entityId.ToString()))
                return;
            _loggedEntityLootResult.Add(entityId.ToString());
        }

        Logger.Info($"EntityLoot result id={entityId} drops={result.Drops.Count} overrideVanilla={result.OverrideVanilla}");
    }

    private static void LogMissingLootItemOnce(Identifier entityId, Identifier itemId)
    {
        string key = $"{entityId}->{itemId}";
        lock (_lootMissingItemLock)
        {
            if (_loggedMissingLootItems.Contains(key))
                return;
            _loggedMissingLootItems.Add(key);
        }

        Logger.Info($"Loot drop item id not found: entity={entityId} item={itemId}");
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

    [StructLayout(LayoutKind.Sequential)]
    private struct EntityLootNativeArgs
    {
        public int EntityNumericId;
        public IntPtr EntityId;
        public IntPtr EntityPtr;
        public int WasKilledByPlayer;
        public int PlayerBonusLevel;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct BlockLootNativeArgs
    {
        public int BlockNumericId;
        public int BlockData;
        public int PlayerBonusLevel;
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
