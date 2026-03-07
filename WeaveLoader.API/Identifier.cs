namespace WeaveLoader.API;

/// <summary>
/// A namespaced identifier in the form "namespace:path" (e.g. "minecraft:stone", "mymod:ruby_ore").
/// </summary>
public readonly record struct Identifier
{
    public string Namespace { get; }
    public string Path { get; }

    public Identifier(string ns, string path)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(ns);
        ArgumentException.ThrowIfNullOrWhiteSpace(path);
        Namespace = ns;
        Path = path;
    }

    public Identifier(string id)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(id);

        var colonIndex = id.IndexOf(':');
        if (colonIndex < 0)
        {
            Namespace = "minecraft";
            Path = id;
        }
        else
        {
            Namespace = id[..colonIndex];
            Path = id[(colonIndex + 1)..];
        }
    }

    public override string ToString() => $"{Namespace}:{Path}";

    public static implicit operator Identifier(string id) => new(id);
}
