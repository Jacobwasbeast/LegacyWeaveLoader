using System.Collections.Generic;

namespace WeaveLoader.API.Block;

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
/// Block registration via the WeaveLoader registry.
/// Accessed through <see cref="Registry.Block"/>.
/// </summary>
public static class BlockRegistry
{
    private static readonly object s_lock = new();
    private static readonly Dictionary<int, Identifier> s_idByNumeric = new();

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
            properties.LightBlockValue,
            properties.NameValue ?? "");

        if (numericId < 0)
            throw new InvalidOperationException($"Failed to register block '{id}'. No free IDs or invalid parameters.");

        if (properties.CreativeTabValue != CreativeTab.None)
        {
            NativeInterop.native_add_to_creative(numericId, 1, 0, (int)properties.CreativeTabValue);
            Logger.Debug($"Block '{id}' added to creative tab {properties.CreativeTabValue}");
        }

        Logger.Debug($"Registered block '{id}' -> numeric ID {numericId}");
        lock (s_lock)
        {
            s_idByNumeric[numericId] = id;
        }
        return new RegisteredBlock(id, numericId);
    }

    internal static bool TryGetIdentifier(int numericId, out Identifier id)
    {
        lock (s_lock)
        {
            return s_idByNumeric.TryGetValue(numericId, out id);
        }
    }
}
