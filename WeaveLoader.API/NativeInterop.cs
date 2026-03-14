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
        string displayName,
        int requiredHarvestLevel,
        int requiredTool,
        int acceptsRedstonePower);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_managed_block(
        string namespacedId,
        int materialId,
        float hardness,
        float resistance,
        int soundType,
        string iconName,
        float lightEmission,
        int lightBlock,
        string displayName,
        int requiredHarvestLevel,
        int requiredTool,
        int acceptsRedstonePower);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_falling_block(
        string namespacedId,
        int materialId,
        float hardness,
        float resistance,
        int soundType,
        string iconName,
        float lightEmission,
        int lightBlock,
        string displayName,
        int requiredHarvestLevel,
        int requiredTool,
        int acceptsRedstonePower);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_slab_block(
        string namespacedId,
        int materialId,
        float hardness,
        float resistance,
        int soundType,
        string iconName,
        float lightEmission,
        int lightBlock,
        string displayName,
        int requiredHarvestLevel,
        int requiredTool,
        int acceptsRedstonePower,
        out int doubleNumericBlockId);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void native_register_block_model(
        int blockId,
        [In] Assets.ModelBox[] boxes,
        int count);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_register_block_model_variant(
        int blockId,
        string key,
        [In] Assets.ModelBox[] boxes,
        int count);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void native_register_block_rotation_profile(
        int blockId,
        int profile);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_item(
        string namespacedId,
        int maxStackSize,
        int maxDamage,
        string iconName,
        string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void native_register_item_display_transform(
        int numericItemId,
        int context,
        Item.ItemDisplayTransform transform);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void native_register_item_renderer(
        int numericItemId,
        nint rendererFnPtr);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void native_set_item_hand_equipped(
        int numericItemId,
        int isHandEquipped);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_pickaxe_item(
        string namespacedId,
        int tier,
        int maxDamage,
        string iconName,
        string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_shovel_item(
        string namespacedId,
        int tier,
        int maxDamage,
        string iconName,
        string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_hoe_item(
        string namespacedId,
        int tier,
        int maxDamage,
        string iconName,
        string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_axe_item(
        string namespacedId,
        int tier,
        int maxDamage,
        string iconName,
        string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_sword_item(
        string namespacedId,
        int tier,
        int maxDamage,
        string iconName,
        string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_configure_custom_pickaxe_item(
        int numericItemId,
        int harvestLevel,
        float destroySpeed);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void native_configure_managed_block(
        int numericBlockId,
        int dropNumericBlockId,
        int cloneNumericBlockId);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_configure_custom_tool_item(
        int numericItemId,
        int toolKind,
        int harvestLevel,
        float destroySpeed,
        float attackDamage);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_allocate_description_id();

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_register_string(int descriptionId, string displayName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_get_minecraft_language();

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_get_minecraft_locale();

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint native_get_mods_path();

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
    internal static extern int native_spawn_item_from_entity(nint entityPtr, int numericItemId, int count, int aux);

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

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_level_has_neighbor_signal(nint levelPtr, int x, int y, int z);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_level_set_tile(nint levelPtr, int x, int y, int z, int blockId, int data, int flags);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_level_schedule_tick(nint levelPtr, int x, int y, int z, int blockId, int delay);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_level_get_tile(nint levelPtr, int x, int y, int z);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_subscribe_event(string eventName, IntPtr managedFnPtr);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void native_add_to_creative(int numericId, int count, int auxValue, int groupIndex);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void native_add_to_creative_ex(int numericId, int count, int auxValue, int groupIndex, int insertMode, int anchorId, int anchorAux);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern nint native_find_symbol(string fullName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_has_symbol(string fullName);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_get_signature_key(string fullName, System.Text.StringBuilder outKey, int outLen);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int native_invoke(nint fn, nint thisPtr, int hasThis, Native.NativeArg[] args, int argCount, ref Native.NativeRet ret);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_mixin_add(string fullName, int at, nint managedCallback, int require);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_mixin_remove(string fullName, nint managedCallback);
}
