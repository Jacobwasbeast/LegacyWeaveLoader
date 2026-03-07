namespace WeaveLoader.API.Assets;

/// <summary>
/// Asset registration for mods. Use for language strings and (future) texture paths.
/// Block and item display names are typically set via BlockProperties.Name() and ItemProperties.Name().
/// Use this registry for additional strings (e.g. GUI labels, messages).
/// </summary>
public static class AssetRegistry
{
    /// <summary>
    /// Register a display string for a custom description ID.
    /// Use native_allocate_description_id() to get an ID, then wire it to your custom UI/entity.
    /// </summary>
    public static void RegisterString(int descriptionId, string displayName)
    {
        NativeInterop.native_register_string(descriptionId, displayName ?? "");
    }

    /// <summary>
    /// Allocate a new description ID for custom strings.
    /// IDs are in the mod range (10000+) and are looked up via the GetString hook.
    /// </summary>
    public static int AllocateDescriptionId()
    {
        return NativeInterop.native_allocate_description_id();
    }
}
