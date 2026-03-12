using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;

namespace WeaveLoader.API.Assets;

internal static class ModelResolver
{
    private sealed class ModelData
    {
        public string IconName = "";
        public List<ModelBox> Boxes = new();
    }

    private enum ModelKind
    {
        Block,
        Item
    }

    internal static void ApplyBlockModel(Identifier id, Block.BlockProperties properties)
    {
        if (!ShouldResolveModel(properties.ModelValue, properties.IconValue, ModelKind.Block))
            return;

        if (TryLoadModel(id, properties.ModelValue, ModelKind.Block, out ModelData? model))
        {
            if (!string.IsNullOrWhiteSpace(model.IconName))
            {
                properties.IconValue = model.IconName;
                Logger.Debug($"Model resolved for block '{id}' -> icon '{model.IconName}'");
            }
            if (model.Boxes.Count > 0)
            {
                properties.ModelBoxes = model.Boxes;
                properties.ModelIsFullCube = IsFullCube(model.Boxes);
            }
        }
        else if (!string.IsNullOrWhiteSpace(properties.ModelValue))
        {
            Logger.Warning($"Model not found for block '{id}' (model '{properties.ModelValue}')");
        }
    }

    internal static void ApplyBlockStates(Identifier id, Block.BlockProperties properties)
    {
        if (string.IsNullOrWhiteSpace(ModContext.ModFolder))
            return;

        if (!TryGetBlockStateFilePath(id, properties.BlockStateValue, out string blockStatePath))
            return;
        if (!File.Exists(blockStatePath))
            return;

        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllText(blockStatePath));
            if (!doc.RootElement.TryGetProperty("variants", out JsonElement variants) ||
                variants.ValueKind != JsonValueKind.Object)
            {
                return;
            }

            var variantProps = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var variant in variants.EnumerateObject())
            {
                string key = NormalizeVariantKey(variant.Name);
                CollectVariantProps(variant.Name, variantProps);
                JsonElement entry = variant.Value;
                if (entry.ValueKind == JsonValueKind.Array)
                {
                    if (entry.GetArrayLength() == 0)
                        continue;
                    entry = entry[0];
                }
                if (entry.ValueKind != JsonValueKind.Object)
                    continue;

                if (!entry.TryGetProperty("model", out JsonElement modelEl) ||
                    modelEl.ValueKind != JsonValueKind.String)
                    continue;

                string? modelName = modelEl.GetString();
                if (string.IsNullOrWhiteSpace(modelName))
                    continue;

                int xRot = 0;
                int yRot = 0;
                if (entry.TryGetProperty("x", out JsonElement xEl) && xEl.ValueKind == JsonValueKind.Number)
                    xRot = xEl.GetInt32();
                if (entry.TryGetProperty("y", out JsonElement yEl) && yEl.ValueKind == JsonValueKind.Number)
                    yRot = yEl.GetInt32();

                if (!TryLoadModelFromValue(id, modelName!, ModelKind.Block, out ModelData? model) || model == null)
                    continue;

                if (string.IsNullOrWhiteSpace(properties.IconValue) && !string.IsNullOrWhiteSpace(model.IconName))
                    properties.IconValue = model.IconName;

                var boxes = model.Boxes.Count > 0
                    ? RotateBoxes(model.Boxes, xRot, yRot)
                    : new List<ModelBox>();

                properties.ModelVariants ??= new Dictionary<string, List<ModelBox>>(StringComparer.OrdinalIgnoreCase);
                properties.ModelVariants[key] = boxes;
            }

            if (properties.RotationProfileValue == Block.BlockRotationProfile.None)
            {
                var inferred = InferRotationProfile(variantProps, blockStatePath);
                if (inferred != Block.BlockRotationProfile.None)
                    properties.RotationProfileValue = inferred;
            }
        }
        catch (Exception ex)
        {
            Logger.Warning($"Failed to parse blockstate JSON '{blockStatePath}': {ex.Message}");
        }
    }

    internal static void ApplyItemModel(Identifier id, Item.ItemProperties properties)
    {
        if (!ShouldResolveModel(properties.ModelValue, properties.IconValue, ModelKind.Item))
            return;

        if (TryLoadModel(id, properties.ModelValue, ModelKind.Item, out ModelData? model))
        {
            if (!string.IsNullOrWhiteSpace(model.IconName))
            {
                properties.IconValue = model.IconName;
                Logger.Debug($"Model resolved for item '{id}' -> icon '{model.IconName}'");
            }
        }
        else if (!string.IsNullOrWhiteSpace(properties.ModelValue))
        {
            Logger.Warning($"Model not found for item '{id}' (model '{properties.ModelValue}')");
        }
    }

    private static bool ShouldResolveModel(string? modelValue, string iconValue, ModelKind kind)
    {
        if (!string.IsNullOrWhiteSpace(modelValue))
            return true;

        // Only auto-resolve if icon was not provided.
        if (kind == ModelKind.Item && string.IsNullOrWhiteSpace(iconValue))
            return true;

        return false;
    }

    private static bool TryLoadModel(Identifier id, string? modelValue, ModelKind kind, out ModelData? model)
    {
        model = null;
        if (string.IsNullOrWhiteSpace(ModContext.ModFolder))
            return false;

        string modelPath;
        string modelNamespace;
        ModelKind effectiveKind = kind;

        if (kind == ModelKind.Item)
        {
            if (TryGetModelFilePath(id, modelValue, ModelKind.Block, out string blockModelPath, out string blockNamespace) &&
                File.Exists(blockModelPath))
            {
                modelPath = blockModelPath;
                modelNamespace = blockNamespace;
                effectiveKind = ModelKind.Block;
            }
            else if (TryGetModelFilePath(id, modelValue, ModelKind.Item, out string itemModelPath, out string itemNamespace) &&
                     File.Exists(itemModelPath))
            {
                modelPath = itemModelPath;
                modelNamespace = itemNamespace;
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (!TryGetModelFilePath(id, modelValue, kind, out modelPath, out modelNamespace))
                return false;
            if (!File.Exists(modelPath))
                return false;
        }

        if (!TryParseTextures(modelPath, out Dictionary<string, string> textures))
            return false;

        var data = new ModelData();

        string? texture = SelectTexture(textures, effectiveKind);
        if (!string.IsNullOrWhiteSpace(texture))
        {
            texture = ResolveTextureReference(texture, textures);
            if (!string.IsNullOrWhiteSpace(texture))
                data.IconName = NormalizeTextureName(texture, modelNamespace);
        }

        TryParseElements(modelPath, data.Boxes);
        model = data;
        return !string.IsNullOrWhiteSpace(data.IconName) || data.Boxes.Count > 0;
    }

    private static bool TryLoadModelFromValue(Identifier id, string modelValue, ModelKind kind, out ModelData? model)
    {
        return TryLoadModel(id, modelValue, kind, out model);
    }

    private static bool TryGetModelFilePath(Identifier id, string? modelValue, ModelKind kind, out string modelPath, out string modelNamespace)
    {
        modelPath = "";
        modelNamespace = "";

        string ns = id.Namespace;
        string rel = id.Path;

        if (!string.IsNullOrWhiteSpace(modelValue))
        {
            string raw = modelValue!;
            int colon = raw.IndexOf(':');
            if (colon >= 0)
            {
                ns = raw[..colon];
                rel = raw[(colon + 1)..];
            }
            else
            {
                rel = raw;
            }
        }

        rel = rel.Replace('\\', '/');
        if (rel.StartsWith("models/", StringComparison.OrdinalIgnoreCase))
            rel = rel["models/".Length..];
        if (!rel.StartsWith("block/", StringComparison.OrdinalIgnoreCase) &&
            !rel.StartsWith("item/", StringComparison.OrdinalIgnoreCase))
        {
            rel = (kind == ModelKind.Block ? "block/" : "item/") + rel;
        }

        string modRoot = ModContext.ModFolder ?? "";
        if (string.IsNullOrWhiteSpace(modRoot))
            return false;

        string file = Path.Combine(modRoot, "assets", ns, "models", rel.Replace('/', Path.DirectorySeparatorChar) + ".json");
        modelPath = file;
        modelNamespace = ns;
        return true;
    }

    private static bool TryGetBlockStateFilePath(Identifier id, string? blockStateValue, out string blockStatePath)
    {
        blockStatePath = "";
        string ns = id.Namespace;
        string rel = id.Path;

        if (!string.IsNullOrWhiteSpace(blockStateValue))
        {
            string raw = blockStateValue!;
            int colon = raw.IndexOf(':');
            if (colon >= 0)
            {
                ns = raw[..colon];
                rel = raw[(colon + 1)..];
            }
            else
            {
                rel = raw;
            }
        }

        rel = rel.Replace('\\', '/');
        if (rel.StartsWith("blockstates/", StringComparison.OrdinalIgnoreCase))
            rel = rel["blockstates/".Length..];

        string modRoot = ModContext.ModFolder ?? "";
        if (string.IsNullOrWhiteSpace(modRoot))
            return false;

        blockStatePath = Path.Combine(modRoot, "assets", ns, "blockstates", rel.Replace('/', Path.DirectorySeparatorChar) + ".json");
        return true;
    }

    private static void CollectVariantProps(string rawKey, HashSet<string> props)
    {
        if (string.IsNullOrWhiteSpace(rawKey))
            return;

        foreach (string part in rawKey.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
        {
            int eq = part.IndexOf('=');
            if (eq <= 0)
                continue;
            props.Add(part[..eq]);
        }
    }

    private static Block.BlockRotationProfile InferRotationProfile(HashSet<string> props, string blockStatePath)
    {
        bool hasFacing = props.Contains("facing");
        bool hasHalf = props.Contains("half");
        bool hasOpen = props.Contains("open");
        bool hasHinge = props.Contains("hinge");
        bool hasRotation = props.Contains("rotation");

        if (hasFacing && hasHalf && hasOpen && hasHinge)
            return Block.BlockRotationProfile.Door;
        if (hasFacing && hasHalf && hasOpen)
            return Block.BlockRotationProfile.Trapdoor;
        if (hasRotation)
            return Block.BlockRotationProfile.StandingSign;
        if (hasFacing)
            return Block.BlockRotationProfile.Facing;

        string file = Path.GetFileNameWithoutExtension(blockStatePath);
        if (file.EndsWith("wall_sign", StringComparison.OrdinalIgnoreCase) ||
            file.EndsWith("wall_hanging_sign", StringComparison.OrdinalIgnoreCase))
            return Block.BlockRotationProfile.WallSign;
        if (file.EndsWith("sign", StringComparison.OrdinalIgnoreCase) ||
            file.EndsWith("hanging_sign", StringComparison.OrdinalIgnoreCase))
            return Block.BlockRotationProfile.StandingSign;

        return Block.BlockRotationProfile.None;
    }

    private static bool TryParseTextures(string modelPath, out Dictionary<string, string> textures)
    {
        textures = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllText(modelPath));
            if (!doc.RootElement.TryGetProperty("textures", out JsonElement texElem))
                return false;
            if (texElem.ValueKind != JsonValueKind.Object)
                return false;

            foreach (var prop in texElem.EnumerateObject())
            {
                if (prop.Value.ValueKind != JsonValueKind.String)
                    continue;
                string? value = prop.Value.GetString();
                if (string.IsNullOrWhiteSpace(value))
                    continue;
                textures[prop.Name] = value!;
            }
            return textures.Count > 0;
        }
        catch (Exception ex)
        {
            Logger.Warning($"Failed to parse model JSON '{modelPath}': {ex.Message}");
            return false;
        }
    }

    private static void TryParseElements(string modelPath, List<ModelBox> boxes)
    {
        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllText(modelPath));
            if (!doc.RootElement.TryGetProperty("elements", out JsonElement elements) ||
                elements.ValueKind != JsonValueKind.Array)
                return;

            foreach (var elem in elements.EnumerateArray())
            {
                if (!elem.TryGetProperty("from", out JsonElement fromEl) ||
                    !elem.TryGetProperty("to", out JsonElement toEl))
                    continue;

                if (fromEl.ValueKind != JsonValueKind.Array || toEl.ValueKind != JsonValueKind.Array)
                    continue;

                if (fromEl.GetArrayLength() < 3 || toEl.GetArrayLength() < 3)
                    continue;

                float fx0 = (float)fromEl[0].GetDouble();
                float fy0 = (float)fromEl[1].GetDouble();
                float fz0 = (float)fromEl[2].GetDouble();
                float fx1 = (float)toEl[0].GetDouble();
                float fy1 = (float)toEl[1].GetDouble();
                float fz1 = (float)toEl[2].GetDouble();

                float x0 = MathF.Min(fx0, fx1) / 16.0f;
                float y0 = MathF.Min(fy0, fy1) / 16.0f;
                float z0 = MathF.Min(fz0, fz1) / 16.0f;
                float x1 = MathF.Max(fx0, fx1) / 16.0f;
                float y1 = MathF.Max(fy0, fy1) / 16.0f;
                float z1 = MathF.Max(fz0, fz1) / 16.0f;

                if (x1 <= x0 || y1 <= y0 || z1 <= z0)
                    continue;

                boxes.Add(new ModelBox
                {
                    X0 = x0,
                    Y0 = y0,
                    Z0 = z0,
                    X1 = x1,
                    Y1 = y1,
                    Z1 = z1
                });
            }
        }
        catch (Exception ex)
        {
            Logger.Warning($"Failed to parse model elements in '{modelPath}': {ex.Message}");
        }
    }

    private static bool IsFullCube(List<ModelBox> boxes)
    {
        if (boxes.Count != 1)
            return false;
        ModelBox box = boxes[0];
        const float eps = 0.0001f;
        return box.X0 <= 0.0f + eps && box.Y0 <= 0.0f + eps && box.Z0 <= 0.0f + eps &&
               box.X1 >= 1.0f - eps && box.Y1 >= 1.0f - eps && box.Z1 >= 1.0f - eps;
    }

    private static string NormalizeVariantKey(string raw)
    {
        if (string.IsNullOrWhiteSpace(raw))
            return "";
        string[] parts = raw.Split(',', StringSplitOptions.RemoveEmptyEntries);
        var pairs = new List<(string key, string value)>();
        foreach (string part in parts)
        {
            int eq = part.IndexOf('=');
            if (eq <= 0 || eq >= part.Length - 1)
                continue;
            string key = part[..eq].Trim().ToLowerInvariant();
            string value = part[(eq + 1)..].Trim().ToLowerInvariant();
            if (key.Length == 0 || value.Length == 0)
                continue;
            pairs.Add((key, value));
        }
        if (pairs.Count == 0)
            return "";
        pairs.Sort((a, b) => string.CompareOrdinal(a.key, b.key));
        return string.Join(",", pairs.Select(p => $"{p.key}={p.value}"));
    }

    private static List<ModelBox> RotateBoxes(List<ModelBox> boxes, int xRot, int yRot)
    {
        int xr = ((xRot % 360) + 360) % 360;
        int yr = ((yRot % 360) + 360) % 360;
        if (xr == 0 && yr == 0)
            return new List<ModelBox>(boxes);

        var result = new List<ModelBox>(boxes.Count);
        foreach (var box in boxes)
        {
            float x0 = MathF.Min(box.X0, box.X1);
            float y0 = MathF.Min(box.Y0, box.Y1);
            float z0 = MathF.Min(box.Z0, box.Z1);
            float x1 = MathF.Max(box.X0, box.X1);
            float y1 = MathF.Max(box.Y0, box.Y1);
            float z1 = MathF.Max(box.Z0, box.Z1);

            Span<(float x, float y, float z)> pts = stackalloc (float, float, float)[8];
            int i = 0;
            for (int xi = 0; xi < 2; xi++)
            for (int yi = 0; yi < 2; yi++)
            for (int zi = 0; zi < 2; zi++)
            {
                float x = xi == 0 ? x0 : x1;
                float y = yi == 0 ? y0 : y1;
                float z = zi == 0 ? z0 : z1;
                (x, y, z) = RotatePoint(x, y, z, xr, yr);
                pts[i++] = (x, y, z);
            }

            float rx0 = pts[0].x, ry0 = pts[0].y, rz0 = pts[0].z;
            float rx1 = pts[0].x, ry1 = pts[0].y, rz1 = pts[0].z;
            for (int j = 1; j < pts.Length; j++)
            {
                var p = pts[j];
                if (p.x < rx0) rx0 = p.x;
                if (p.y < ry0) ry0 = p.y;
                if (p.z < rz0) rz0 = p.z;
                if (p.x > rx1) rx1 = p.x;
                if (p.y > ry1) ry1 = p.y;
                if (p.z > rz1) rz1 = p.z;
            }

            result.Add(new ModelBox { X0 = rx0, Y0 = ry0, Z0 = rz0, X1 = rx1, Y1 = ry1, Z1 = rz1 });
        }

        return result;
    }

    private static (float x, float y, float z) RotatePoint(float x, float y, float z, int xRot, int yRot)
    {
        float px = x;
        float py = y;
        float pz = z;

        switch (xRot)
        {
            case 90:
                (py, pz) = (1.0f - pz, py);
                break;
            case 180:
                py = 1.0f - py;
                pz = 1.0f - pz;
                break;
            case 270:
                (py, pz) = (pz, 1.0f - py);
                break;
        }

        switch (yRot)
        {
            case 90:
                (px, pz) = (pz, 1.0f - px);
                break;
            case 180:
                px = 1.0f - px;
                pz = 1.0f - pz;
                break;
            case 270:
                (px, pz) = (1.0f - pz, px);
                break;
        }

        return (px, py, pz);
    }

    private static string? SelectTexture(Dictionary<string, string> textures, ModelKind kind)
    {
        string[] keys = kind == ModelKind.Item
            ? new[] { "layer0", "layer1" }
            : new[] { "all", "side", "top", "bottom", "particle" };

        foreach (string key in keys)
        {
            if (textures.TryGetValue(key, out string value))
                return value;
        }

        foreach (var kvp in textures)
            return kvp.Value;

        return null;
    }

    private static string ResolveTextureReference(string texture, Dictionary<string, string> textures)
    {
        string current = texture;
        for (int i = 0; i < 8; i++)
        {
            if (!current.StartsWith("#", StringComparison.Ordinal))
                return current;
            string key = current[1..];
            if (!textures.TryGetValue(key, out string next) || string.IsNullOrWhiteSpace(next))
                return "";
            current = next;
        }
        return "";
    }

    private static string NormalizeTextureName(string texture, string modelNamespace)
    {
        string value = texture.Replace('\\', '/');
        if (value.Contains(':'))
            return value;
        if (string.IsNullOrWhiteSpace(modelNamespace))
            return value;
        return $"{modelNamespace}:{value}";
    }
}
