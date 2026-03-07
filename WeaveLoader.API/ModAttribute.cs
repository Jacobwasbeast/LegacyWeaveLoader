namespace WeaveLoader.API;

/// <summary>
/// Marks a class as a WeaveLoader mod and provides metadata.
/// The class must also implement <see cref="IMod"/>.
/// </summary>
[AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
public sealed class ModAttribute : Attribute
{
    /// <summary>
    /// The unique mod identifier (e.g. "examplemod"). Used as the default namespace
    /// for content registered by this mod.
    /// </summary>
    public string Id { get; }

    /// <summary>
    /// Human-readable display name.
    /// </summary>
    public string Name { get; set; } = "";

    /// <summary>
    /// Semantic version string (e.g. "1.0.0").
    /// </summary>
    public string Version { get; set; } = "1.0.0";

    /// <summary>
    /// Mod author(s).
    /// </summary>
    public string Author { get; set; } = "";

    /// <summary>
    /// Short description of the mod.
    /// </summary>
    public string Description { get; set; } = "";

    public ModAttribute(string id)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(id);
        Id = id;
    }
}
