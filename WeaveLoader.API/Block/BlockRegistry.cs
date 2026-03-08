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

public class RegisteredSlabBlock : RegisteredBlock
{
    public Identifier DoubleStringId { get; }
    public int DoubleNumericId { get; }

    internal RegisteredSlabBlock(Identifier id, Identifier doubleId, int numericId, int doubleNumericId)
        : base(id, numericId)
    {
        DoubleStringId = doubleId;
        DoubleNumericId = doubleNumericId;
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
            properties.NameValue ?? "",
            properties.RequiredHarvestLevelValue,
            (int)properties.RequiredToolValue,
            properties.AcceptsRedstonePowerValue ? 1 : 0);

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

    public static RegisteredBlock Register(Identifier id, Block managedBlock, BlockProperties properties)
    {
        if (managedBlock is SlabBlock)
            return RegisterSlab(id, properties);

        if (managedBlock is FallingBlock)
            return RegisterFalling(id, managedBlock, properties);

        int numericId = NativeInterop.native_register_managed_block(
            id.ToString(),
            (int)properties.MaterialValue,
            properties.HardnessValue,
            properties.ResistanceValue,
            (int)properties.SoundValue,
            properties.IconValue,
            properties.LightEmissionValue,
            properties.LightBlockValue,
            properties.NameValue ?? "",
            properties.RequiredHarvestLevelValue,
            (int)properties.RequiredToolValue,
            properties.AcceptsRedstonePowerValue ? 1 : 0);

        if (numericId < 0)
            throw new InvalidOperationException($"Failed to register managed block '{id}'.");

        if (properties.CreativeTabValue != CreativeTab.None)
        {
            NativeInterop.native_add_to_creative(numericId, 1, 0, (int)properties.CreativeTabValue);
        }

        ManagedBlockDispatcher.RegisterBlock(id, numericId, managedBlock);

        int dropNumericId = -1;
        if (managedBlock.DropAsBlockId is Identifier dropId)
            dropNumericId = IdHelper.GetBlockNumericId(dropId);

        int cloneNumericId = -1;
        if (managedBlock.CloneAsBlockId is Identifier cloneId)
            cloneNumericId = IdHelper.GetBlockNumericId(cloneId);
        NativeInterop.native_configure_managed_block(numericId, dropNumericId, cloneNumericId);

        lock (s_lock)
        {
            s_idByNumeric[numericId] = id;
        }

        return new RegisteredBlock(id, numericId);
    }

    public static RegisteredBlock RegisterFalling(Identifier id, BlockProperties properties)
        => RegisterFalling(id, null, properties);

    private static RegisteredBlock RegisterFalling(Identifier id, Block? managedBlock, BlockProperties properties)
    {
        int numericId = NativeInterop.native_register_falling_block(
            id.ToString(),
            (int)properties.MaterialValue,
            properties.HardnessValue,
            properties.ResistanceValue,
            (int)properties.SoundValue,
            properties.IconValue,
            properties.LightEmissionValue,
            properties.LightBlockValue,
            properties.NameValue ?? "",
            properties.RequiredHarvestLevelValue,
            (int)properties.RequiredToolValue,
            properties.AcceptsRedstonePowerValue ? 1 : 0);

        if (numericId < 0)
            throw new InvalidOperationException($"Failed to register falling block '{id}'.");

        if (properties.CreativeTabValue != CreativeTab.None)
        {
            NativeInterop.native_add_to_creative(numericId, 1, 0, (int)properties.CreativeTabValue);
        }

        if (managedBlock != null)
        {
            ManagedBlockDispatcher.RegisterBlock(id, numericId, managedBlock);

            int dropNumericId = -1;
            if (managedBlock.DropAsBlockId is Identifier dropId)
                dropNumericId = IdHelper.GetBlockNumericId(dropId);

            int cloneNumericId = -1;
            if (managedBlock.CloneAsBlockId is Identifier cloneId)
                cloneNumericId = IdHelper.GetBlockNumericId(cloneId);

            NativeInterop.native_configure_managed_block(numericId, dropNumericId, cloneNumericId);
        }

        lock (s_lock)
        {
            s_idByNumeric[numericId] = id;
        }
        return new RegisteredBlock(id, numericId);
    }

    public static RegisteredSlabBlock RegisterSlab(Identifier id, BlockProperties properties)
    {
        Identifier doubleId = new($"{id}_double");
        int numericId = NativeInterop.native_register_slab_block(
            id.ToString(),
            (int)properties.MaterialValue,
            properties.HardnessValue,
            properties.ResistanceValue,
            (int)properties.SoundValue,
            properties.IconValue,
            properties.LightEmissionValue,
            properties.LightBlockValue,
            properties.NameValue ?? "",
            properties.RequiredHarvestLevelValue,
            (int)properties.RequiredToolValue,
            properties.AcceptsRedstonePowerValue ? 1 : 0,
            out int doubleNumericId);

        if (numericId < 0)
            throw new InvalidOperationException($"Failed to register slab block '{id}'.");
        if (doubleNumericId < 0)
            throw new InvalidOperationException($"Failed to resolve generated slab pair '{doubleId}'.");

        if (properties.CreativeTabValue != CreativeTab.None)
        {
            NativeInterop.native_add_to_creative(numericId, 1, 0, (int)properties.CreativeTabValue);
        }

        lock (s_lock)
        {
            s_idByNumeric[numericId] = id;
            s_idByNumeric[doubleNumericId] = doubleId;
        }

        return new RegisteredSlabBlock(id, doubleId, numericId, doubleNumericId);
    }
    internal static bool TryGetIdentifier(int numericId, out Identifier id)
    {
        lock (s_lock)
        {
            return s_idByNumeric.TryGetValue(numericId, out id);
        }
    }
}
