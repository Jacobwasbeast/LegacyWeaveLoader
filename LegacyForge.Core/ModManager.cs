using LegacyForge.API;

namespace LegacyForge.Core;

/// <summary>
/// Manages the lifecycle of all loaded mods.
/// Catches exceptions from individual mods to prevent one broken mod from crashing the game.
/// </summary>
internal class ModManager
{
    private readonly List<ModDiscovery.DiscoveredMod> _mods = new();

    internal int ModCount => _mods.Count;

    internal void AddMods(IEnumerable<ModDiscovery.DiscoveredMod> mods)
    {
        _mods.AddRange(mods);
    }

    internal void PreInit()
    {
        Logger.Info("--- PreInit phase ---");
        foreach (var mod in _mods)
            SafeCall(mod, "OnPreInit", () => mod.Instance.OnPreInit());
    }

    internal void Init()
    {
        Logger.Info("--- Initialize phase ---");
        foreach (var mod in _mods)
            SafeCall(mod, "OnInitialize", () => mod.Instance.OnInitialize());
    }

    internal void PostInit()
    {
        Logger.Info("--- PostInitialize phase ---");
        foreach (var mod in _mods)
            SafeCall(mod, "OnPostInitialize", () => mod.Instance.OnPostInitialize());
    }

    internal void Tick()
    {
        foreach (var mod in _mods)
        {
            try
            {
                mod.Instance.OnTick();
            }
            catch (Exception ex)
            {
                Logger.Error($"[{mod.Metadata.Id}] OnTick error: {ex.Message}");
            }
        }
    }

    internal void Shutdown()
    {
        Logger.Info("--- Shutdown phase ---");
        foreach (var mod in _mods)
            SafeCall(mod, "OnShutdown", () => mod.Instance.OnShutdown());
    }

    private static void SafeCall(ModDiscovery.DiscoveredMod mod, string phase, Action action)
    {
        try
        {
            action();
        }
        catch (Exception ex)
        {
            Logger.Error($"[{mod.Metadata.Id}] {phase} failed: {ex.Message}");
            Logger.Debug(ex.StackTrace ?? "");
        }
    }
}
