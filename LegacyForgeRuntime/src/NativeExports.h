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
        int lightBlock);

    __declspec(dllexport) int native_register_item(
        const char* namespacedId,
        int maxStackSize,
        int maxDamage);

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

    __declspec(dllexport) void native_subscribe_event(
        const char* eventName,
        void* managedFnPtr);
}
