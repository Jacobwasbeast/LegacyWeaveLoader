namespace LegacyForge.API;

/// <summary>
/// The main interface all LegacyForge mods must implement.
/// Default interface methods allow mods to only override what they need.
/// </summary>
public interface IMod
{
    /// <summary>
    /// Called before vanilla registries are populated.
    /// Use for very early setup that must happen before any game content loads.
    /// </summary>
    void OnPreInit() { }

    /// <summary>
    /// Called after vanilla registries are populated.
    /// This is the main initialization point -- register your blocks, items,
    /// entities, recipes, and event handlers here.
    /// </summary>
    void OnInitialize();

    /// <summary>
    /// Called after the game client has finished its own initialization.
    /// Use for client-side setup like custom renderers or UI.
    /// </summary>
    void OnPostInitialize() { }

    /// <summary>
    /// Called once per game tick (20 times per second).
    /// Use for ongoing mod logic.
    /// </summary>
    void OnTick() { }

    /// <summary>
    /// Called when the game is shutting down.
    /// Use for cleanup and saving mod state.
    /// </summary>
    void OnShutdown() { }
}
