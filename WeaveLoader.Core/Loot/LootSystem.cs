using System.Text.Json;
using WeaveLoader.API;
using WeaveLoader.API.Loot;

namespace WeaveLoader.Core.Loot;

internal sealed class LootSystem
{
    private readonly LootIndex _index;
    private readonly LootManager _lootManager;
    private readonly LootResourceManager _resourceManager;
    private readonly Random _random = new();
    private readonly object _lock = new();
    private readonly string _modsPath;
    private static readonly object _logLock = new();
    private static readonly HashSet<string> _loggedTableIds = new(StringComparer.OrdinalIgnoreCase);
    private static readonly List<LootDrop> _emptyDrops = new();
    private static readonly object _tableCacheLock = new();
    private static readonly Dictionary<string, CachedTable> _tableCache = new(StringComparer.OrdinalIgnoreCase);

    public LootSystem(string modsPath)
    {
        _modsPath = modsPath;
        _index = LootIndex.Build(modsPath);
        _lootManager = new LootManager();
        _resourceManager = new LootResourceManager();
    }

    public LootResult GetEntityLoot(Identifier entityId)
    {
        Identifier tableId = new(entityId.Namespace, $"entities/{entityId.Path}");
        return GetLoot(tableId);
    }

    public LootResult GetBlockLoot(Identifier blockId)
    {
        Identifier tableId = new(blockId.Namespace, $"blocks/{blockId.Path}");
        return GetLoot(tableId);
    }

    private LootResult GetLoot(Identifier tableId)
    {
        var tables = _index.FindTables(tableId);
        LogTablesOnce(tableId, tables);

        if (tables.Count == 0 && !LootTableEvents.MODIFY.HasHandlers)
            return new LootResult(_emptyDrops, false);

        bool overrideVanilla = false;
        var builder = new LootTableBuilder();

        foreach (var table in tables)
        {
            if (table.Namespace == "minecraft")
                overrideVanilla = true;

            MergeTableIntoBuilder(builder, table.Path);
        }

        LootTableSource source = tableId.Namespace == "minecraft" ? LootTableSource.Vanilla : LootTableSource.Mod;
        LootTableEvents.MODIFY.Fire(_resourceManager, _lootManager, tableId, builder, source);

        List<LootDrop> drops;
        lock (_lock)
        {
            drops = LootEvaluator.Evaluate(builder, _random);
        }

        return new LootResult(drops, overrideVanilla);
    }

    private void LogTablesOnce(Identifier tableId, List<TableRef> tables)
    {
        string key = tableId.ToString();
        lock (_logLock)
        {
            if (_loggedTableIds.Contains(key))
                return;
            _loggedTableIds.Add(key);
        }

        if (tables.Count == 0)
        {
            Logger.Info($"Loot tables not found for {tableId} (modsPath={_modsPath})");
            return;
        }

        string list = string.Join(", ", tables.Select(t => $"{t.Namespace}:{t.Path}"));
        Logger.Info($"Loot tables for {tableId}: {list}");
    }

    public static Identifier NormalizeEntityId(string encodeId)
    {
        if (string.IsNullOrWhiteSpace(encodeId))
            return new Identifier("minecraft", "pig");

        string trimmed = encodeId.Trim();
        if (trimmed.Contains(':'))
            return new Identifier(trimmed.ToLowerInvariant());

        if (LegacyEntityIdMap.TryGetValue(trimmed, out string? mapped))
            return new Identifier(mapped);

        string snake = ToSnakeCase(trimmed);
        return new Identifier("minecraft", snake.ToLowerInvariant());
    }

    private static void MergeTableIntoBuilder(LootTableBuilder builder, string path)
    {
        CachedTable table;
        lock (_tableCacheLock)
        {
            if (!_tableCache.TryGetValue(path, out table!))
            {
                table = ParseTable(path);
                _tableCache[path] = table;
            }
        }

        foreach (CachedPool cachedPool in table.Pools)
        {
            var poolBuilder = LootPool.builder();

            if (cachedPool.Rolls.HasValue)
                poolBuilder.rolls(ConstantLootNumberProvider.create(cachedPool.Rolls.Value));

            if (cachedPool.RandomChance.HasValue)
                poolBuilder.conditionally(RandomChanceLootCondition.builder(cachedPool.RandomChance.Value));

            foreach (CachedEntry entry in cachedPool.Entries)
            {
                if (entry.IsEmpty)
                {
                    poolBuilder.with(new EmptyEntry(entry.Weight));
                    continue;
                }

                var itemBuilder = ItemEntry.builder(entry.ItemId);
                itemBuilder.weight(entry.Weight);
                if (entry.Count.HasValue)
                    itemBuilder.apply(new SetCountLootFunction(entry.Count.Value));
                poolBuilder.with(itemBuilder);
            }

            builder.pool(poolBuilder);
        }
    }

    private static CachedTable ParseTable(string path)
    {
        var cached = new CachedTable();
        try
        {
            string json = File.ReadAllText(path);
            using JsonDocument doc = JsonDocument.Parse(json);
            if (!doc.RootElement.TryGetProperty("pools", out JsonElement pools) || pools.ValueKind != JsonValueKind.Array)
                return cached;

            foreach (JsonElement poolElem in pools.EnumerateArray())
            {
                var cachedPool = new CachedPool();

                if (poolElem.TryGetProperty("rolls", out JsonElement rollsElem) &&
                    rollsElem.ValueKind == JsonValueKind.Number &&
                    rollsElem.TryGetInt32(out int rolls))
                {
                    cachedPool.Rolls = rolls;
                }

                if (poolElem.TryGetProperty("conditions", out JsonElement conditionsElem) &&
                    conditionsElem.ValueKind == JsonValueKind.Array)
                {
                    foreach (JsonElement cond in conditionsElem.EnumerateArray())
                        ParseCondition(cond, cachedPool);
                }

                if (poolElem.TryGetProperty("entries", out JsonElement entriesElem) &&
                    entriesElem.ValueKind == JsonValueKind.Array)
                {
                    foreach (JsonElement entry in entriesElem.EnumerateArray())
                        ParseEntry(entry, cachedPool);
                }

                cached.Pools.Add(cachedPool);
            }
        }
        catch (Exception ex)
        {
            Logger.Error($"Loot table parse failed for {path}: {ex.Message}");
        }
        return cached;
    }

    private static void ParseCondition(JsonElement element, CachedPool pool)
    {
        if (!element.TryGetProperty("condition", out JsonElement typeElem))
            return;

        string? type = typeElem.GetString();
        if (type == "minecraft:random_chance" && element.TryGetProperty("chance", out JsonElement chanceElem))
        {
            if (chanceElem.ValueKind == JsonValueKind.Number && chanceElem.TryGetSingle(out float chance))
            {
                pool.RandomChance = chance;
            }
        }
    }

    private static void ParseEntry(JsonElement entry, CachedPool pool)
    {
        if (!entry.TryGetProperty("type", out JsonElement typeElem))
            return;

        string? type = typeElem.GetString();
        int weight = 1;
        if (entry.TryGetProperty("weight", out JsonElement weightElem) && weightElem.ValueKind == JsonValueKind.Number)
            weightElem.TryGetInt32(out weight);

        if (type == "minecraft:empty")
        {
            pool.Entries.Add(new CachedEntry
            {
                IsEmpty = true,
                Weight = weight
            });
            return;
        }

        if (type == "minecraft:item" && entry.TryGetProperty("name", out JsonElement nameElem))
        {
            string? name = nameElem.GetString();
            if (string.IsNullOrWhiteSpace(name))
                return;

            var cachedEntry = new CachedEntry
            {
                IsEmpty = false,
                ItemId = new Identifier(name),
                Weight = weight,
                Count = null
            };

            if (entry.TryGetProperty("functions", out JsonElement funcsElem) && funcsElem.ValueKind == JsonValueKind.Array)
            {
                foreach (JsonElement func in funcsElem.EnumerateArray())
                {
                    if (!func.TryGetProperty("function", out JsonElement fnTypeElem))
                        continue;
                    string? fnType = fnTypeElem.GetString();
                    if (fnType == "minecraft:set_count" && func.TryGetProperty("count", out JsonElement countElem))
                    {
                        if (countElem.ValueKind == JsonValueKind.Number && countElem.TryGetInt32(out int count))
                            cachedEntry.Count = count;
                    }
                }
            }

            pool.Entries.Add(cachedEntry);
        }
    }

    private sealed class CachedTable
    {
        public readonly List<CachedPool> Pools = new();
    }

    private sealed class CachedPool
    {
        public int? Rolls;
        public float? RandomChance;
        public readonly List<CachedEntry> Entries = new();
    }

    private sealed class CachedEntry
    {
        public bool IsEmpty;
        public Identifier ItemId = default!;
        public int Weight;
        public int? Count;
    }

    private static string ToSnakeCase(string input)
    {
        if (string.IsNullOrEmpty(input))
            return input;

        var result = new System.Text.StringBuilder(input.Length + 8);
        for (int i = 0; i < input.Length; i++)
        {
            char c = input[i];
            if (char.IsUpper(c))
            {
                if (i > 0 && (char.IsLower(input[i - 1]) || char.IsDigit(input[i - 1])))
                    result.Append('_');
                result.Append(char.ToLowerInvariant(c));
            }
            else
            {
                result.Append(c);
            }
        }
        return result.ToString();
    }

    private static readonly Dictionary<string, string> LegacyEntityIdMap = new(StringComparer.OrdinalIgnoreCase)
    {
        ["PigZombie"] = "minecraft:zombie_pigman",
        ["LavaSlime"] = "minecraft:magma_cube",
        ["VillagerGolem"] = "minecraft:iron_golem",
        ["SnowMan"] = "minecraft:snow_golem",
        ["Ozelot"] = "minecraft:ocelot",
        ["EnderMan"] = "minecraft:enderman",
        ["WitherBoss"] = "minecraft:wither",
        ["MushroomCow"] = "minecraft:mooshroom",
        ["Giant"] = "minecraft:giant",
        ["CaveSpider"] = "minecraft:cave_spider",
        ["MinecartRideable"] = "minecraft:minecart",
        ["MinecartChest"] = "minecraft:chest_minecart",
        ["MinecartFurnace"] = "minecraft:furnace_minecart",
        ["MinecartTNT"] = "minecraft:tnt_minecart",
        ["MinecartHopper"] = "minecraft:hopper_minecart",
        ["MinecartSpawner"] = "minecraft:spawner_minecart",
        ["EyeOfEnderSignal"] = "minecraft:eye_of_ender_signal",
        ["ThrownEnderpearl"] = "minecraft:ender_pearl",
        ["ThrownExpBottle"] = "minecraft:xp_bottle",
        ["ThrownPotion"] = "minecraft:potion",
        ["FireworksRocketEntity"] = "minecraft:fireworks_rocket",
        ["PrimedTnt"] = "minecraft:primed_tnt",
        ["FallingSand"] = "minecraft:falling_block",
        ["XPOrb"] = "minecraft:xp_orb"
    };

    private sealed record TableRef(string Namespace, string Path);

    private sealed class LootIndex
    {
        private readonly Dictionary<Identifier, List<TableRef>> _tables = new();
        private readonly Dictionary<string, List<TableRef>> _tablesByPath = new(StringComparer.OrdinalIgnoreCase);
        private static readonly List<TableRef> _emptyTables = new();

        public static LootIndex Build(string modsPath)
        {
            var index = new LootIndex();
            if (!Directory.Exists(modsPath))
                return index;

            foreach (string modFolder in Directory.GetDirectories(modsPath))
            {
                string dataDir = Path.Combine(modFolder, "data");
                if (!Directory.Exists(dataDir))
                    continue;

                foreach (string nsDir in Directory.GetDirectories(dataDir))
                {
                    string ns = Path.GetFileName(nsDir).ToLowerInvariant();
                    string lootDir = Path.Combine(nsDir, "loot_tables");
                    if (!Directory.Exists(lootDir))
                        continue;

                    foreach (string file in Directory.GetFiles(lootDir, "*.json", SearchOption.AllDirectories))
                    {
                        string rel = Path.GetRelativePath(lootDir, file).Replace('\\', '/');
                        if (rel.EndsWith(".json", StringComparison.OrdinalIgnoreCase))
                            rel = rel[..^5];
                        var id = new Identifier(ns, rel.ToLowerInvariant());

                        if (!index._tables.TryGetValue(id, out var list))
                        {
                            list = new List<TableRef>();
                            index._tables[id] = list;
                        }
                        list.Add(new TableRef(ns, file));

                        if (!index._tablesByPath.TryGetValue(id.Path, out var pathList))
                        {
                            pathList = new List<TableRef>();
                            index._tablesByPath[id.Path] = pathList;
                        }
                        pathList.Add(new TableRef(ns, file));
                    }
                }
            }

            return index;
        }

        public List<TableRef> FindTables(Identifier id)
        {
            if (id.Namespace == "minecraft")
            {
                if (_tablesByPath.TryGetValue(id.Path, out var byPath))
                    return byPath;
            }
            if (_tables.TryGetValue(id, out var list))
                return list;
            return _emptyTables;
        }
    }

    public readonly record struct LootResult(List<LootDrop> Drops, bool OverrideVanilla);

    private static class LootEvaluator
    {
        public static List<LootDrop> Evaluate(LootTableBuilder builder, Random random)
        {
            var drops = new List<LootDrop>();
            foreach (LootPool pool in builder.Pools)
            {
                if (!CheckConditions(pool, random))
                    continue;

                int rolls = pool.Rolls.NextInt(random);
                if (rolls < 1)
                    continue;

                for (int i = 0; i < rolls; i++)
                {
                    LootEntry? entry = ChooseEntry(pool.Entries, random);
                    if (entry is ItemEntry itemEntry)
                    {
                        var drop = new LootDrop(itemEntry.ItemId, 1, 0);
                        foreach (ILootFunction fn in itemEntry.Functions)
                            fn.Apply(ref drop, random);
                        if (drop.Count > 0)
                            drops.Add(drop);
                    }
                }
            }

            return drops;
        }

        private static bool CheckConditions(LootPool pool, Random random)
        {
            foreach (ILootCondition cond in pool.Conditions)
            {
                if (!cond.Test(random))
                    return false;
            }
            return true;
        }

        private static LootEntry? ChooseEntry(List<LootEntry> entries, Random random)
        {
            if (entries.Count == 0)
                return null;

            int totalWeight = 0;
            foreach (LootEntry entry in entries)
                totalWeight += entry.Weight > 0 ? entry.Weight : 0;

            if (totalWeight <= 0)
                return null;

            int roll = random.Next(totalWeight);
            int accum = 0;
            foreach (LootEntry entry in entries)
            {
                int weight = entry.Weight > 0 ? entry.Weight : 0;
                accum += weight;
                if (roll < accum)
                    return entry;
            }

            return entries[0];
        }
    }
}
