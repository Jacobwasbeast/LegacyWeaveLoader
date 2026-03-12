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
        Assets.ModelResolver.ApplyBlockModel(id, properties);
        Assets.ModelResolver.ApplyBlockStates(id, properties);
        ApplyModelLightDefaults(properties);
        int numericId = NativeInterop.native_register_block(
            id.ToString(),
            (int)properties.MaterialValue,
            properties.HardnessValue,
            properties.ResistanceValue,
            (int)properties.SoundValue,
            properties.IconValue,
            properties.LightEmissionValue,
            properties.LightBlockValue,
            properties.NameValue?.Resolve() ?? "",
            properties.RequiredHarvestLevelValue,
            (int)properties.RequiredToolValue,
            properties.AcceptsRedstonePowerValue ? 1 : 0);

        if (numericId < 0)
        {
            Logger.Error($"Failed to register block '{id}'. No free IDs or invalid parameters.");
            throw new InvalidOperationException($"Failed to register block '{id}'. No free IDs or invalid parameters.");
        }

        RegisterBlockModels(numericId, properties);

        AddToCreative(id, numericId, properties);

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

        Assets.ModelResolver.ApplyBlockModel(id, properties);
        Assets.ModelResolver.ApplyBlockStates(id, properties);
        ApplyModelLightDefaults(properties);
        int numericId = NativeInterop.native_register_managed_block(
            id.ToString(),
            (int)properties.MaterialValue,
            properties.HardnessValue,
            properties.ResistanceValue,
            (int)properties.SoundValue,
            properties.IconValue,
            properties.LightEmissionValue,
            properties.LightBlockValue,
            properties.NameValue?.Resolve() ?? "",
            properties.RequiredHarvestLevelValue,
            (int)properties.RequiredToolValue,
            properties.AcceptsRedstonePowerValue ? 1 : 0);

        if (numericId < 0)
        {
            Logger.Error($"Failed to register managed block '{id}'.");
            throw new InvalidOperationException($"Failed to register managed block '{id}'.");
        }

        RegisterBlockModels(numericId, properties);

        AddToCreative(id, numericId, properties);

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
        Assets.ModelResolver.ApplyBlockModel(id, properties);
        Assets.ModelResolver.ApplyBlockStates(id, properties);
        ApplyModelLightDefaults(properties);
        int numericId = NativeInterop.native_register_falling_block(
            id.ToString(),
            (int)properties.MaterialValue,
            properties.HardnessValue,
            properties.ResistanceValue,
            (int)properties.SoundValue,
            properties.IconValue,
            properties.LightEmissionValue,
            properties.LightBlockValue,
            properties.NameValue?.Resolve() ?? "",
            properties.RequiredHarvestLevelValue,
            (int)properties.RequiredToolValue,
            properties.AcceptsRedstonePowerValue ? 1 : 0);

        if (numericId < 0)
        {
            Logger.Error($"Failed to register falling block '{id}'.");
            throw new InvalidOperationException($"Failed to register falling block '{id}'.");
        }

        RegisterBlockModels(numericId, properties);

        AddToCreative(id, numericId, properties);

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
        Assets.ModelResolver.ApplyBlockModel(id, properties);
        Assets.ModelResolver.ApplyBlockStates(id, properties);
        ApplyModelLightDefaults(properties);
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
            properties.NameValue?.Resolve() ?? "",
            properties.RequiredHarvestLevelValue,
            (int)properties.RequiredToolValue,
            properties.AcceptsRedstonePowerValue ? 1 : 0,
            out int doubleNumericId);

        if (numericId < 0)
        {
            Logger.Error($"Failed to register slab block '{id}'.");
            throw new InvalidOperationException($"Failed to register slab block '{id}'.");
        }
        if (doubleNumericId < 0)
        {
            Logger.Error($"Failed to resolve generated slab pair '{doubleId}'.");
            throw new InvalidOperationException($"Failed to resolve generated slab pair '{doubleId}'.");
        }

        RegisterBlockModels(numericId, properties);

        AddToCreative(id, numericId, properties);

        lock (s_lock)
        {
            s_idByNumeric[numericId] = id;
            s_idByNumeric[doubleNumericId] = doubleId;
        }

        return new RegisteredSlabBlock(id, doubleId, numericId, doubleNumericId);
    }

    private static void ApplyModelLightDefaults(BlockProperties properties)
    {
        if (properties.ModelBoxes == null || properties.ModelBoxes.Count == 0)
            return;

        if (properties.LightBlockExplicit)
            return;

        if (!properties.ModelIsFullCube)
            properties.LightBlockValue = 0;
    }

    private static void RegisterBlockModels(int numericId, BlockProperties properties)
    {
        if (properties.ModelBoxes != null && properties.ModelBoxes.Count > 0)
        {
            NativeInterop.native_register_block_model(numericId, properties.ModelBoxes.ToArray(), properties.ModelBoxes.Count);
        }

        if (properties.ModelVariants != null)
        {
            foreach (var variant in properties.ModelVariants)
            {
                var boxes = variant.Value;
                if (boxes.Count == 0)
                    continue;
                NativeInterop.native_register_block_model_variant(numericId, variant.Key, boxes.ToArray(), boxes.Count);
            }
        }

        NativeInterop.native_register_block_rotation_profile(numericId, (int)properties.RotationProfileValue);
    }
    internal static bool TryGetIdentifier(int numericId, out Identifier id)
    {
        lock (s_lock)
        {
            return s_idByNumeric.TryGetValue(numericId, out id);
        }
    }

    private static void AddToCreative(Identifier id, int numericId, BlockProperties properties)
    {
        if (properties.CreativeTabValue == CreativeTab.None)
        {
            Logger.Debug($"Block '{id}' not added to creative (CreativeTab.None)");
            return;
        }

        bool added = false;
        if (properties.CreativePlacementValue.HasValue)
        {
            CreativePlacement placement = properties.CreativePlacementValue.Value;
            if (placement.Insert == CreativeInsert.Prepend)
            {
                try
                {
                    NativeInterop.native_add_to_creative_ex(
                        numericId, 1, 0, (int)properties.CreativeTabValue,
                        (int)placement.Insert, -1, -1);
                }
                catch (DllNotFoundException e)
                {
                    Logger.Error($"Creative add failed for block '{id}': {e.Message}. Check that WeaveLoaderRuntime is present.");
                    throw;
                }
                catch (EntryPointNotFoundException e)
                {
                    Logger.Error($"Creative add failed for block '{id}': {e.Message}. API/runtime mismatch.");
                    throw;
                }
                added = true;
            }
        }

        if (!added)
        {
            try
            {
                NativeInterop.native_add_to_creative(numericId, 1, 0, (int)properties.CreativeTabValue);
            }
            catch (DllNotFoundException e)
            {
                Logger.Error($"Creative add failed for block '{id}': {e.Message}. Check that WeaveLoaderRuntime is present.");
                throw;
            }
            catch (EntryPointNotFoundException e)
            {
                Logger.Error($"Creative add failed for block '{id}': {e.Message}. API/runtime mismatch.");
                throw;
            }
        }

        Logger.Debug($"Block '{id}' added to creative tab {properties.CreativeTabValue}");
    }
}
