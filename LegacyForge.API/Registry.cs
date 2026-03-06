using LegacyForge.API.Block;
using LegacyForge.API.Item;
using LegacyForge.API.Entity;
using LegacyForge.API.Recipe;

namespace LegacyForge.API;

/// <summary>
/// Central access point for all LegacyForge registries.
/// Use Registry.Block, Registry.Item, Registry.Entity, or Registry.Recipe to register content.
/// </summary>
public static class Registry
{
    /// <summary>Block registration. Call Register() with a namespaced ID and BlockProperties.</summary>
    public static class Block
    {
        public static RegisteredBlock Register(Identifier id, BlockProperties properties)
            => BlockRegistry.Register(id, properties);
    }

    /// <summary>Item registration. Call Register() with a namespaced ID and ItemProperties.</summary>
    public static class Item
    {
        public static RegisteredItem Register(Identifier id, ItemProperties properties)
            => ItemRegistry.Register(id, properties);
    }

    /// <summary>Entity registration. Call Register() with a namespaced ID and EntityDefinition.</summary>
    public static class Entity
    {
        public static RegisteredEntity Register(Identifier id, EntityDefinition definition)
            => EntityRegistry.Register(id, definition);
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
}
