namespace LegacyForge.API.Item;

/// <summary>
/// Fluent builder for defining item properties.
/// </summary>
public class ItemProperties
{
    internal int MaxStackSizeValue = 64;
    internal int MaxDamageValue = 0;

    public ItemProperties MaxStackSize(int size) { MaxStackSizeValue = size; return this; }

    /// <summary>
    /// Set max damage for a tool/armor item. Setting this to a positive value
    /// makes the item damageable with a durability bar.
    /// </summary>
    public ItemProperties MaxDamage(int damage) { MaxDamageValue = damage; MaxStackSizeValue = 1; return this; }
}
