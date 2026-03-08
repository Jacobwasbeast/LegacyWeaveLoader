using WeaveLoader.API.Block;
using WeaveLoader.API.Item;
using WeaveLoader.API.Entity;
using WeaveLoader.API.Recipe;
using WeaveLoader.API.Assets;

namespace WeaveLoader.API;

/// <summary>
/// Central access point for all WeaveLoader registries.
/// Use Registry.Block, Registry.Item, Registry.Entity, Registry.Recipe, or Registry.Assets.
/// </summary>
public static class Registry
{
    /// <summary>Block registration. Call Register() with a namespaced ID and BlockProperties.</summary>
    public static class Block
    {
        public static RegisteredBlock Register(Identifier id, BlockProperties properties)
            => BlockRegistry.Register(id, properties);

        public static RegisteredBlock Register(Identifier id, WeaveLoader.API.Block.Block block, BlockProperties properties)
            => BlockRegistry.Register(id, block, properties);

        public static RegisteredBlock RegisterFalling(Identifier id, BlockProperties properties)
            => BlockRegistry.Register(id, new FallingBlock(), properties);

        public static RegisteredSlabBlock RegisterSlab(Identifier id, BlockProperties properties)
            => (RegisteredSlabBlock)BlockRegistry.Register(id, new SlabBlock(), properties);
    }

    /// <summary>Item registration. Call Register() with a namespaced ID and ItemProperties.</summary>
    public static class Item
    {
        public static RegisteredItem Register(Identifier id, ItemProperties properties)
            => ItemRegistry.Register(id, properties);

        public static RegisteredItem Register(Identifier id, WeaveLoader.API.Item.Item item, ItemProperties properties)
            => ItemRegistry.Register(id, item, properties);

        public static RegisteredToolMaterial RegisterToolMaterial(Identifier id, ToolMaterialDefinition definition)
            => ToolMaterialRegistry.Register(id, definition);

        public static RegisteredPickaxeTier RegisterPickaxeTier(Identifier id, PickaxeTierDefinition definition)
            => PickaxeTierRegistry.Register(id, definition);
    }

    /// <summary>Entity registration. Call Register() with a namespaced ID and EntityDefinition.</summary>
    public static class Entity
    {
        public static RegisteredEntity Register(Identifier id, EntityDefinition definition)
            => EntityRegistry.Register(id, definition);

        public static bool Summon(Identifier id, double x, double y, double z)
            => EntityRegistry.Summon(id, x, y, z);

        public static bool Summon(int numericId, double x, double y, double z)
            => EntityRegistry.Summon(numericId, x, y, z);
    }

    /// <summary>Recipe registration for crafting and smelting.</summary>
    public static class Recipe
    {
        public static void AddShaped(Identifier result, int count, string[] pattern,
                                      params (char key, Identifier ingredient)[] keys)
            => RecipeRegistry.AddShaped(result, count, pattern, keys);

        public static void AddFurnace(Identifier input, Identifier output, float xp)
            => RecipeRegistry.AddFurnace(input, output, xp);
    }

    /// <summary>Asset registration for language strings and (future) textures.</summary>
    public static class Assets
    {
        public static void RegisterString(int descriptionId, string displayName)
            => AssetRegistry.RegisterString(descriptionId, displayName);
        public static int AllocateDescriptionId()
            => AssetRegistry.AllocateDescriptionId();
    }
}
