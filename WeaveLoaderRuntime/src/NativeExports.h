#pragma once

/// Exported C functions callable from C# via P/Invoke.
/// All registration functions accept namespaced string IDs and delegate
/// to IdRegistry for numeric ID allocation.
extern "C"
{
    __declspec(dllexport) int native_register_block(
        const char* namespacedId,
        int materialId,
        float hardness,
        float resistance,
        int soundType,
        const char* iconName,
        float lightEmission,
        int lightBlock,
        const char* displayName,
        int requiredHarvestLevel,
        int requiredTool);

    __declspec(dllexport) int native_register_item(
        const char* namespacedId,
        int maxStackSize,
        int maxDamage,
        const char* iconName,
        const char* displayName);

    __declspec(dllexport) int native_register_pickaxe_item(
        const char* namespacedId,
        int tier,
        int maxDamage,
        const char* iconName,
        const char* displayName);
    __declspec(dllexport) int native_register_shovel_item(
        const char* namespacedId,
        int tier,
        int maxDamage,
        const char* iconName,
        const char* displayName);
    __declspec(dllexport) int native_register_hoe_item(
        const char* namespacedId,
        int tier,
        int maxDamage,
        const char* iconName,
        const char* displayName);
    __declspec(dllexport) int native_register_axe_item(
        const char* namespacedId,
        int tier,
        int maxDamage,
        const char* iconName,
        const char* displayName);
    __declspec(dllexport) int native_register_sword_item(
        const char* namespacedId,
        int tier,
        int maxDamage,
        const char* iconName,
        const char* displayName);

    __declspec(dllexport) int native_configure_custom_pickaxe_item(
        int numericItemId,
        int harvestLevel,
        float destroySpeed);
    __declspec(dllexport) int native_configure_custom_tool_item(
        int numericItemId,
        int toolKind,
        int harvestLevel,
        float destroySpeed,
        float attackDamage);

    __declspec(dllexport) int native_allocate_description_id();
    __declspec(dllexport) void native_register_string(int descriptionId, const char* displayName);

    __declspec(dllexport) int native_register_entity(
        const char* namespacedId,
        float width,
        float height,
        int trackingRange);

    __declspec(dllexport) void native_add_shaped_recipe(
        const char* resultId,
        int resultCount,
        const char* pattern,
        const char* ingredientIds);

    __declspec(dllexport) void native_add_furnace_recipe(
        const char* inputId,
        const char* outputId,
        float xp);

    __declspec(dllexport) void native_log(
        const char* message,
        int level);

    __declspec(dllexport) int native_get_block_id(const char* namespacedId);
    __declspec(dllexport) int native_get_item_id(const char* namespacedId);
    __declspec(dllexport) int native_get_entity_id(const char* namespacedId);
    __declspec(dllexport) int native_consume_item_from_player(void* playerPtr, int numericItemId, int count);
    __declspec(dllexport) int native_damage_item_instance(void* itemInstancePtr, int amount, void* ownerSharedPtr);
    __declspec(dllexport) int native_spawn_entity_from_player_look(void* playerPtr, void* playerSharedPtr, int numericEntityId, double speed, double spawnForward, double spawnUp);
    __declspec(dllexport) int native_summon_entity(const char* namespacedId, double x, double y, double z);
    __declspec(dllexport) int native_summon_entity_by_id(int numericEntityId, double x, double y, double z);

    __declspec(dllexport) void native_subscribe_event(
        const char* eventName,
        void* managedFnPtr);

    __declspec(dllexport) void native_add_to_creative(
        int numericId,
        int count,
        int auxValue,
        int groupIndex);
}
