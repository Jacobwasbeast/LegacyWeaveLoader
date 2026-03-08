namespace WeaveLoader.API.Item;

/// <summary>
/// Fluent builder for defining item properties.
/// </summary>
public class ItemProperties
{
    internal int MaxStackSizeValue = 64;
    internal int MaxDamageValue = 0;
    internal float AttackDamageValue = 0.0f;
    internal string IconValue = "";
    internal CreativeTab CreativeTabValue = CreativeTab.None;
    internal string? NameValue;

    public ItemProperties MaxStackSize(int size) { MaxStackSizeValue = size; return this; }
    /// <summary>Icon name in the items atlas. Use namespaced ID for mod textures (e.g. "examplemod:ruby" from assets/items/ruby.png), or vanilla names like "diamond", "ingotIron".</summary>
    public ItemProperties Icon(string iconName) { IconValue = iconName; return this; }

    /// <summary>
    /// Set max damage for a tool/armor item. Setting this to a positive value
    /// makes the item damageable with a durability bar.
    /// </summary>
    public ItemProperties MaxDamage(int damage) { MaxDamageValue = damage; MaxStackSizeValue = 1; return this; }
    /// <summary>Override the native attack damage value for tool items.</summary>
    public ItemProperties AttackDamage(float damage) { AttackDamageValue = damage; return this; }
    public ItemProperties InCreativeTab(CreativeTab tab) { CreativeTabValue = tab; return this; }
    /// <summary>Display name shown in-game (e.g. "Ruby"). Used for localization.</summary>
    public ItemProperties Name(string displayName) { NameValue = displayName; return this; }
}
