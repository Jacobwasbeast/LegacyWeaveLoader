namespace WeaveLoader.API.Entity;

/// <summary>
/// Fluent builder for defining entity properties.
/// </summary>
public class EntityDefinition
{
    internal float WidthValue = 0.6f;
    internal float HeightValue = 1.8f;
    internal int TrackingRangeValue = 80;

    public EntityDefinition Width(float width) { WidthValue = width; return this; }
    public EntityDefinition Height(float height) { HeightValue = height; return this; }
    public EntityDefinition TrackingRange(int range) { TrackingRangeValue = range; return this; }
    public EntityDefinition Size(float width, float height) { WidthValue = width; HeightValue = height; return this; }
}
