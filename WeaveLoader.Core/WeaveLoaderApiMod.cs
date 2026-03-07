using WeaveLoader.API;

namespace WeaveLoader.Core;

/// <summary>
/// Built-in mod representing the WeaveLoader API. Counts in the mod list and appears
/// when mods/WeaveLoader.API/ exists. Does not run lifecycle hooks.
/// </summary>
[Mod("weaveloader.api", Name = "WeaveLoader API", Version = "1.0.0", Author = "WeaveLoader",
     Description = "Mod API and shared types")]
internal sealed class WeaveLoaderApiMod : IMod
{
    public void OnInitialize() { }
}
