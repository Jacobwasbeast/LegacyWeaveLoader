#pragma once

#include <string>
#include <cstdint>

namespace NativeExports
{
    void SetLevelInteropSymbols(void* hasNeighborSignal, void* setTileAndData, void* addToTickNextTick, void* getTile);
    void SetLocalizationSymbols(void* appPtr, void* getLanguage, void* getLocale);
    void SetModsPath(const std::string& modsPath);
}

/// Exported C functions callable from C# via P/Invoke.
/// All registration functions accept namespaced string IDs and delegate
/// to IdRegistry for numeric ID allocation.
extern "C"
{
    enum NativeType : uint8_t
    {
        NativeType_I32 = 1,
        NativeType_I64 = 2,
        NativeType_F32 = 3,
        NativeType_F64 = 4,
        NativeType_Ptr = 5,
        NativeType_Bool = 6
    };

    struct NativeArg
    {
        NativeType type;
        uint64_t value;
    };

    struct NativeRet
    {
        NativeType type;
        uint64_t value;
    };

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
        int requiredTool,
        int acceptsRedstonePower);
    __declspec(dllexport) int native_register_managed_block(
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
        int requiredTool,
        int acceptsRedstonePower);
    __declspec(dllexport) int native_register_falling_block(
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
        int requiredTool,
        int acceptsRedstonePower);
    __declspec(dllexport) int native_register_slab_block(
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
        int requiredTool,
        int acceptsRedstonePower,
        int* outDoubleBlockNumericId);

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
    __declspec(dllexport) void native_configure_managed_block(
        int numericBlockId,
        int dropNumericBlockId,
        int cloneNumericBlockId);
    __declspec(dllexport) int native_configure_custom_tool_item(
        int numericItemId,
        int toolKind,
        int harvestLevel,
        float destroySpeed,
        float attackDamage);

    __declspec(dllexport) int native_allocate_description_id();
    __declspec(dllexport) void native_register_string(int descriptionId, const char* displayName);
    __declspec(dllexport) int native_get_minecraft_language();
    __declspec(dllexport) int native_get_minecraft_locale();
    __declspec(dllexport) const char* native_get_mods_path();

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
    __declspec(dllexport) int native_level_has_neighbor_signal(void* levelPtr, int x, int y, int z);
    __declspec(dllexport) int native_level_set_tile(void* levelPtr, int x, int y, int z, int blockId, int data, int flags);
    __declspec(dllexport) int native_level_schedule_tick(void* levelPtr, int x, int y, int z, int blockId, int delay);
    __declspec(dllexport) int native_level_get_tile(void* levelPtr, int x, int y, int z);

    __declspec(dllexport) void native_subscribe_event(
        const char* eventName,
        void* managedFnPtr);

    __declspec(dllexport) void native_add_to_creative(
        int numericId,
        int count,
        int auxValue,
        int groupIndex);

    __declspec(dllexport) void* native_find_symbol(const char* fullName);
    __declspec(dllexport) int native_has_symbol(const char* fullName);
    __declspec(dllexport) int native_get_signature_key(const char* fullName, char* outKey, int outLen);
    __declspec(dllexport) int native_invoke(void* fn, void* thisPtr, int hasThis, const NativeArg* args, int argCount, NativeRet* outRet);

    __declspec(dllexport) int native_mixin_add(const char* fullName, int at, void* managedCallback, int require);
    __declspec(dllexport) int native_mixin_remove(const char* fullName, void* managedCallback);
}
