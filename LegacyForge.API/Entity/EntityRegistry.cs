namespace LegacyForge.API.Entity;

/// <summary>
/// Represents an entity type that has been registered with the game engine.
/// </summary>
public class RegisteredEntity
{
    public Identifier StringId { get; }
    public int NumericId { get; }

    internal RegisteredEntity(Identifier id, int numericId)
    {
        StringId = id;
        NumericId = numericId;
    }
}

/// <summary>
/// Entity registration via the LegacyForge registry.
/// Accessed through <see cref="Registry.Entity"/>.
/// </summary>
public static class EntityRegistry
{
    public static RegisteredEntity Register(Identifier id, EntityDefinition definition)
    {
        int numericId = NativeInterop.native_register_entity(
            id.ToString(),
            definition.WidthValue,
            definition.HeightValue,
            definition.TrackingRangeValue);

        if (numericId < 0)
            throw new InvalidOperationException($"Failed to register entity '{id}'.");

        Logger.Debug($"Registered entity '{id}' -> numeric ID {numericId}");
        return new RegisteredEntity(id, numericId);
    }
}
