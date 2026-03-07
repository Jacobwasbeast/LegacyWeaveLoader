using System.Reflection;
using System.Runtime.Loader;
using WeaveLoader.API;

namespace WeaveLoader.Core;

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

        // Count WeaveLoader.API as a mod when its folder exists (mods/WeaveLoader.API/)
        var apiFolder = Path.Combine(modsPath, "WeaveLoader.API");
        if (Directory.Exists(apiFolder))
        {
            var apiMod = new WeaveLoaderApiMod();
            var attr = typeof(WeaveLoaderApiMod).GetCustomAttribute<ModAttribute>()!;
            mods.Add(new DiscoveredMod(apiMod, attr, typeof(ModDiscovery).Assembly));
            Logger.Info($"Discovered mod: {attr.Name} v{attr.Version} by {attr.Author} (mods/WeaveLoader.API/)");
        }

        // Scan each mod folder: mods/ExampleMod/, mods/SomeMod/, etc.
        // Each subfolder may contain one or more mod DLLs (we skip WeaveLoader.API.dll)
        var modFolders = Directory.GetDirectories(modsPath);
        foreach (var folder in modFolders)
        {
            var folderName = Path.GetFileName(folder);
            var dllFiles = Directory.GetFiles(folder, "*.dll", SearchOption.TopDirectoryOnly);

            foreach (var dllPath in dllFiles)
            {
                string fileName = Path.GetFileName(dllPath);

                // Skip the API assembly -- it's in its own folder and counted above
                if (fileName.Equals("WeaveLoader.API.dll", StringComparison.OrdinalIgnoreCase))
                    continue;

                try
                {
                    var discovered = LoadModAssembly(dllPath);
                    mods.AddRange(discovered);
                }
                catch (Exception ex)
                {
                    Logger.Error($"Failed to load mod from {fileName}: {ex.Message}");
                }
            }
        }

        Logger.Info($"Scanning {modsPath} -- found {mods.Count} mod(s) total");
        return mods;
    }

    private static List<DiscoveredMod> LoadModAssembly(string dllPath)
    {
        var results = new List<DiscoveredMod>();
        var fileName = Path.GetFileName(dllPath);
        var fullPath = Path.GetFullPath(dllPath);

        // Load into the SAME ALC that WeaveLoader.Core lives in (the hostfxr component context).
        // This ensures WeaveLoader.API types (IMod, ModAttribute, etc.) have the same identity.
        var coreContext = AssemblyLoadContext.GetLoadContext(typeof(ModDiscovery).Assembly)
                          ?? AssemblyLoadContext.Default;
        var assembly = coreContext.LoadFromAssemblyPath(fullPath);

        var allTypes = assembly.GetTypes();
        Logger.Debug($"{fileName}: {allTypes.Length} type(s), checking for IMod implementations...");

        var modTypes = allTypes
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
                results.Add(new DiscoveredMod(instance, attr, assembly));

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
