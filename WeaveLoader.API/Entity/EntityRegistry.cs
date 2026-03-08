using System.Collections.Generic;

namespace WeaveLoader.API.Entity;

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
/// Entity registration via the WeaveLoader registry.
/// Accessed through <see cref="Registry.Entity"/>.
/// </summary>
public static class EntityRegistry
{
    private static readonly object s_lock = new();
    private static readonly Dictionary<int, Identifier> s_idByNumeric = new();

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
        lock (s_lock)
        {
            s_idByNumeric[numericId] = id;
        }
        return new RegisteredEntity(id, numericId);
    }

    public static bool Summon(Identifier id, double x, double y, double z)
    {
        int numericId = IdHelper.GetEntityNumericId(id);
        if (numericId < 0)
            return false;

        return Summon(numericId, x, y, z);
    }

    public static bool Summon(int numericId, double x, double y, double z)
    {
        int ok = NativeInterop.native_summon_entity_by_id(numericId, x, y, z);
        return ok != 0;
    }

    internal static bool TryGetIdentifier(int numericId, out Identifier id)
    {
        lock (s_lock)
        {
            return s_idByNumeric.TryGetValue(numericId, out id);
        }
    }
}
