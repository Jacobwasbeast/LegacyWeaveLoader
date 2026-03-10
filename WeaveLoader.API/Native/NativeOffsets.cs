using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;

namespace WeaveLoader.API.Native;

public static class NativeOffsets
{
    private static Dictionary<string, Dictionary<string, int>>? s_offsets;
    private static bool s_loaded;
    private static bool s_logged;

    static NativeOffsets()
    {
        TryLoadFromMetadata();
    }

    public static class Entity
    {
        public static int X = 0x78;
        public static int Y = 0x80;
        public static int Z = 0x88;
        public static int Removed = 0xC7;
    }

    public static bool HasData => s_loaded;

    public static bool TryLoadFromFile(string path)
    {
        if (string.IsNullOrWhiteSpace(path) || !File.Exists(path))
            return false;

        try
        {
            var json = File.ReadAllText(path);
            var root = JsonSerializer.Deserialize<Dictionary<string, Dictionary<string, int>>>(json);
            if (root == null)
                return false;

            s_offsets = NormalizeOffsets(root);
            s_loaded = true;

            if (s_offsets.TryGetValue("entity", out var entity))
            {
                if (entity.TryGetValue("x", out var x)) Entity.X = x;
                if (entity.TryGetValue("y", out var y)) Entity.Y = y;
                if (entity.TryGetValue("z", out var z)) Entity.Z = z;
                if (entity.TryGetValue("removed", out var removed)) Entity.Removed = removed;
            }
            return true;
        }
        catch (Exception ex)
        {
            try
            {
                Logger.Warning($"Offsets load failed for {path}: {ex.Message}");
            }
            catch
            {
            }
            return false;
        }
    }

    public static bool TryGet(string typeName, string fieldName, out int offset)
    {
        offset = 0;
        if (s_offsets == null)
            return false;
        string typeKey = NormalizeName(typeName);
        string fieldKey = NormalizeFieldName(fieldName);
        if (!s_offsets.TryGetValue(typeKey, out var fields))
            return false;
        return fields.TryGetValue(fieldKey, out offset);
    }

    private static void TryLoadFromMetadata()
    {
        if (s_logged)
            return;
        s_logged = true;

        var candidates = new List<string>(GetCandidates());
        foreach (var candidate in candidates)
        {
            if (TryLoadFromFile(candidate))
                return;
        }

        try
        {
            Logger.Warning("Offsets not loaded. offsets.json not found in metadata.");
        }
        catch
        {
        }
    }

    private static IEnumerable<string> GetCandidates()
    {
        var list = new List<string>();
        var baseDir = AppContext.BaseDirectory;
        list.Add(Path.Combine(baseDir, "metadata", "offsets.json"));
        list.Add(Path.Combine(baseDir, "offsets.json"));

        var modsPath = GetNativeModsPath();
        if (!string.IsNullOrWhiteSpace(modsPath))
        {
            var root = Path.GetDirectoryName(modsPath) ?? "";
            if (!string.IsNullOrWhiteSpace(root))
            {
                list.Add(Path.Combine(root, "metadata", "offsets.json"));
                list.Add(Path.Combine(root, "offsets.json"));
            }
        }

        var cwd = Directory.GetCurrentDirectory();
        list.Add(Path.Combine(cwd, "metadata", "offsets.json"));
        list.Add(Path.Combine(cwd, "offsets.json"));

        try
        {
            string loaderBuild = Path.Combine(@"Z:\home\jacobwasbeast\MinecraftLegacyEdition\ModLoader\build", "metadata", "offsets.json");
            list.Add(loaderBuild);
        }
        catch
        {
        }

        return list;
    }

    private static string? GetNativeModsPath()
    {
        try
        {
            var ptr = NativeInterop.native_get_mods_path();
            if (ptr == nint.Zero)
                return null;
            return Marshal.PtrToStringAnsi(ptr);
        }
        catch
        {
            return null;
        }
    }

    private static Dictionary<string, Dictionary<string, int>> NormalizeOffsets(
        Dictionary<string, Dictionary<string, int>> raw)
    {
        var result = new Dictionary<string, Dictionary<string, int>>();
        foreach (var (typeName, fields) in raw)
        {
            string typeKey = NormalizeName(typeName);
            var fieldMap = new Dictionary<string, int>();
            foreach (var (fieldName, offset) in fields)
            {
                string fieldKey = NormalizeFieldName(fieldName);
                if (!fieldMap.ContainsKey(fieldKey))
                    fieldMap[fieldKey] = offset;
            }
            result[typeKey] = fieldMap;
        }
        return result;
    }

    private static string NormalizeName(string name)
    {
        if (string.IsNullOrWhiteSpace(name))
            return "";
        return name.Trim().ToLowerInvariant();
    }

    private static string NormalizeFieldName(string name)
    {
        if (string.IsNullOrWhiteSpace(name))
            return "";
        string trimmed = name.Trim();
        if (trimmed.StartsWith("m_", StringComparison.OrdinalIgnoreCase))
            trimmed = trimmed.Substring(2);
        if (trimmed.StartsWith("_", StringComparison.OrdinalIgnoreCase))
            trimmed = trimmed.Substring(1);
        if (trimmed.StartsWith("m", StringComparison.OrdinalIgnoreCase) && trimmed.Length > 1 && char.IsUpper(trimmed[1]))
            trimmed = trimmed.Substring(1);
        return trimmed.ToLowerInvariant();
    }
}
