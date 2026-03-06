namespace LegacyForge.API.Recipe;

/// <summary>
/// Recipe registration via the LegacyForge registry.
/// Accessed through <see cref="Registry.Recipe"/>.
/// </summary>
public static class RecipeRegistry
{
    /// <summary>
    /// Add a shaped crafting recipe.
    /// </summary>
    /// <param name="result">The output item identifier.</param>
    /// <param name="resultCount">Number of items produced.</param>
    /// <param name="pattern">Crafting grid pattern rows (e.g. "XXX", " | ", " | " for a pickaxe).</param>
    /// <param name="keys">Character-to-ingredient mappings.</param>
    public static void AddShaped(Identifier result, int resultCount, string[] pattern,
                                  params (char key, Identifier ingredient)[] keys)
    {
        string patternStr = string.Join(";", pattern);
        string ingredientStr = string.Join(";",
            keys.Select(k => $"{k.key}={k.ingredient}"));

        NativeInterop.native_add_shaped_recipe(
            result.ToString(), resultCount, patternStr, ingredientStr);

        Logger.Debug($"Added shaped recipe -> {resultCount}x {result}");
    }

    /// <summary>
    /// Add a furnace/smelting recipe.
    /// </summary>
    /// <param name="input">The input item/block identifier.</param>
    /// <param name="output">The output item identifier.</param>
    /// <param name="xp">Experience granted per smelt.</param>
    public static void AddFurnace(Identifier input, Identifier output, float xp)
    {
        NativeInterop.native_add_furnace_recipe(input.ToString(), output.ToString(), xp);
        Logger.Debug($"Added furnace recipe: {input} -> {output} ({xp} xp)");
    }
}
