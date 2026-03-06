namespace LegacyForge.API.Block;

/// <summary>
/// Represents a block that has been registered with the game engine.
/// </summary>
public class RegisteredBlock
{
    /// <summary>The namespaced string ID (e.g. "mymod:ruby_ore").</summary>
    public Identifier StringId { get; }

    /// <summary>The numeric ID allocated by the engine.</summary>
    public int NumericId { get; }

    internal RegisteredBlock(Identifier id, int numericId)
    {
        StringId = id;
        NumericId = numericId;
    }
}

/// <summary>
/// Block registration via the LegacyForge registry.
/// Accessed through <see cref="Registry.Block"/>.
/// </summary>
public static class BlockRegistry
{
    /// <summary>
    /// Register a new block with the game engine.
    /// </summary>
    /// <param name="id">Namespaced identifier (e.g. "mymod:ruby_ore").</param>
    /// <param name="properties">Block properties built with <see cref="BlockProperties"/>.</param>
    /// <returns>A handle to the registered block.</returns>
    public static RegisteredBlock Register(Identifier id, BlockProperties properties)
    {
        int numericId = NativeInterop.native_register_block(
            id.ToString(),
            (int)properties.MaterialValue,
            properties.HardnessValue,
            properties.ResistanceValue,
            (int)properties.SoundValue,
            properties.IconValue,
            properties.LightEmissionValue,
            properties.LightBlockValue);

        if (numericId < 0)
            throw new InvalidOperationException($"Failed to register block '{id}'. No free IDs or invalid parameters.");

        Logger.Debug($"Registered block '{id}' -> numeric ID {numericId}");
        return new RegisteredBlock(id, numericId);
    }
}
