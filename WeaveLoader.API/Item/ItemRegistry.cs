namespace WeaveLoader.API.Item;

/// <summary>
/// Represents an item that has been registered with the game engine.
/// </summary>
public class RegisteredItem
{
    /// <summary>The namespaced string ID (e.g. "mymod:ruby").</summary>
    public Identifier StringId { get; }

    /// <summary>The numeric ID allocated by the engine.</summary>
    public int NumericId { get; }

    internal RegisteredItem(Identifier id, int numericId)
    {
        StringId = id;
        NumericId = numericId;
    }
}

/// <summary>
/// Item registration via the WeaveLoader registry.
/// Accessed through <see cref="Registry.Item"/>.
/// </summary>
public static class ItemRegistry
{
    /// <summary>
    /// Register a new item with the game engine.
    /// </summary>
    /// <param name="id">Namespaced identifier (e.g. "mymod:ruby").</param>
    /// <param name="properties">Item properties built with <see cref="ItemProperties"/>.</param>
    /// <returns>A handle to the registered item.</returns>
    public static RegisteredItem Register(Identifier id, ItemProperties properties)
    {
        int numericId = NativeInterop.native_register_item(
            id.ToString(),
            properties.MaxStackSizeValue,
            properties.MaxDamageValue,
            properties.IconValue,
            properties.NameValue ?? "");

        if (numericId < 0)
            throw new InvalidOperationException($"Failed to register item '{id}'. No free IDs or invalid parameters.");

        if (properties.CreativeTabValue != CreativeTab.None)
        {
            NativeInterop.native_add_to_creative(numericId, 1, 0, (int)properties.CreativeTabValue);
            Logger.Debug($"Item '{id}' added to creative tab {properties.CreativeTabValue}");
        }

        Logger.Debug($"Registered item '{id}' -> numeric ID {numericId}");
        return new RegisteredItem(id, numericId);
    }
}
