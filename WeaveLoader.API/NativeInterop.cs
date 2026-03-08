using System.Runtime.InteropServices;

namespace WeaveLoader.API;

/// <summary>
/// Internal P/Invoke bindings to WeaveLoaderRuntime.dll native exports.
/// Mod authors should use the Registry and Logger classes instead of calling these directly.
/// </summary>
internal static class NativeInterop
{
    private const string RuntimeDll = "WeaveLoaderRuntime";

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_block(
        string namespacedId,
        int materialId,
        float hardness,
        float resistance,
        int soundType,
        string iconName,
        float lightEmission,
        int lightBlock,
        string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_item(
        string namespacedId,
        int maxStackSize,
        int maxDamage,
        string iconName,
        string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_pickaxe_item(
        string namespacedId,
        int tier,
        int maxDamage,
        string iconName,
        string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_allocate_description_id();

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_register_string(int descriptionId, string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_entity(
        string namespacedId,
        float width,
        float height,
        int trackingRange);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_add_shaped_recipe(
        string resultId,
        int resultCount,
        string pattern,
        string ingredientIds);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_add_furnace_recipe(
        string inputId,
        string outputId,
        float xp);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_log(string message, int level);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_get_block_id(string namespacedId);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_get_item_id(string namespacedId);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_get_entity_id(string namespacedId);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_summon_entity(string namespacedId, double x, double y, double z);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_summon_entity_by_id(int numericEntityId, double x, double y, double z);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_consume_item_from_player(nint playerPtr, int numericItemId, int count);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_damage_item_instance(nint itemInstancePtr, int amount, nint ownerSharedPtr);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_spawn_entity_from_player_look(
        nint playerPtr,
        nint playerSharedPtr,
        int numericEntityId,
        double speed,
        double spawnForward,
        double spawnUp);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_subscribe_event(string eventName, IntPtr managedFnPtr);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void native_add_to_creative(int numericId, int count, int auxValue, int groupIndex);
}
