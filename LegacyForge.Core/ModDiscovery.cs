using System.Reflection;
using System.Runtime.Loader;
using LegacyForge.API;

namespace LegacyForge.Core;

/// <summary>
/// Discovers and loads mod assemblies from the mods/ directory.
/// Each mod is loaded into its own AssemblyLoadContext for isolation.
/// </summary>
internal static class ModDiscovery
{
    internal record DiscoveredMod(
        IMod Instance,
        ModAttribute Metadata,
        Assembly Assembly);

    internal static List<DiscoveredMod> DiscoverMods(string modsPath)
    {
        var mods = new List<DiscoveredMod>();

        if (!Directory.Exists(modsPath))
        {
            Logger.Warning($"Mods directory not found: {modsPath}");
            return mods;
        }

        var dllFiles = Directory.GetFiles(modsPath, "*.dll");
        Logger.Info($"Scanning {modsPath} -- found {dllFiles.Length} DLL(s)");

        foreach (var dllPath in dllFiles)
        {
            try
            {
                var discovered = LoadModAssembly(dllPath);
                mods.AddRange(discovered);
            }
            catch (Exception ex)
            {
                Logger.Error($"Failed to load mod from {Path.GetFileName(dllPath)}: {ex.Message}");
            }
        }

        return mods;
    }

    private static List<DiscoveredMod> LoadModAssembly(string dllPath)
    {
        var results = new List<DiscoveredMod>();
        var fileName = Path.GetFileName(dllPath);

        var loadContext = new AssemblyLoadContext(fileName, isCollectible: false);
        var assembly = loadContext.LoadFromAssemblyPath(Path.GetFullPath(dllPath));

        var modTypes = assembly.GetTypes()
            .Where(t => t.IsClass && !t.IsAbstract && typeof(IMod).IsAssignableFrom(t));

        foreach (var type in modTypes)
        {
            var attr = type.GetCustomAttribute<ModAttribute>();
            if (attr == null)
            {
                Logger.Warning($"Class {type.FullName} in {fileName} implements IMod but is missing [Mod] attribute -- skipping");
                continue;
            }

            try
            {
                var instance = (IMod)Activator.CreateInstance(type)!;
                results.Add(new ModDiscovery.DiscoveredMod(instance, attr, assembly));

                string name = string.IsNullOrEmpty(attr.Name) ? attr.Id : attr.Name;
                Logger.Info($"Discovered mod: {name} v{attr.Version} by {attr.Author} ({fileName})");
            }
            catch (Exception ex)
            {
                Logger.Error($"Failed to instantiate mod {type.FullName}: {ex.Message}");
            }
        }

        if (results.Count == 0)
            Logger.Debug($"No IMod implementations found in {fileName}");

        return results;
    }
}
