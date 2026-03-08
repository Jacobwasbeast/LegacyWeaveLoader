#include "NativeExports.h"
#include "IdRegistry.h"
#include "CreativeInventory.h"
#include "GameObjectFactory.h"
#include "FurnaceRecipeRegistry.h"
#include "GameHooks.h"
#include "CustomPickaxeRegistry.h"
#include "CustomToolMaterialRegistry.h"
#include "CustomBlockRegistry.h"
#include "CustomSlabRegistry.h"
#include "ManagedBlockRegistry.h"
#include "ModStrings.h"
#include "LogUtil.h"
#include <Windows.h>
#include <cstring>
#include <string>

namespace
{
    using LevelHasNeighborSignal_fn = bool (__fastcall *)(void* thisPtr, int x, int y, int z);
    using LevelSetTileAndData_fn = bool (__fastcall *)(void* thisPtr, int x, int y, int z, int blockId, int data, int flags);
    using LevelAddToTickNextTick_fn = void (__fastcall *)(void* thisPtr, int x, int y, int z, int blockId, int delay);
    using LevelGetTile_fn = int (__fastcall *)(void* thisPtr, int x, int y, int z);

    LevelHasNeighborSignal_fn s_levelHasNeighborSignal = nullptr;
    LevelSetTileAndData_fn s_levelSetTileAndData = nullptr;
    LevelAddToTickNextTick_fn s_levelAddToTickNextTick = nullptr;
    LevelGetTile_fn s_levelGetTile = nullptr;
}

void NativeExports::SetLevelInteropSymbols(void* hasNeighborSignal, void* setTileAndData, void* addToTickNextTick, void* getTile)
{
    s_levelHasNeighborSignal = reinterpret_cast<LevelHasNeighborSignal_fn>(hasNeighborSignal);
    s_levelSetTileAndData = reinterpret_cast<LevelSetTileAndData_fn>(setTileAndData);
    s_levelAddToTickNextTick = reinterpret_cast<LevelAddToTickNextTick_fn>(addToTickNextTick);
    s_levelGetTile = reinterpret_cast<LevelGetTile_fn>(getTile);
}

static std::wstring Utf8ToWide(const char* utf8)
{
    if (!utf8 || !utf8[0]) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (len <= 0) return std::wstring();
    std::wstring result(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &result[0], len);
    return result;
}

static int ResolveRecipeId(const char* namespacedId, bool preferItem)
{
    if (!namespacedId || !namespacedId[0]) return -1;

    int itemId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Item, namespacedId);
    int blockId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, namespacedId);

    if (preferItem)
        return (itemId >= 0) ? itemId : blockId;

    return (blockId >= 0) ? blockId : itemId;
}

static bool PrepareBlockRegistration(
    const char* namespacedId,
    const char* iconName,
    const char* displayName,
    int* outId,
    int* outDescId,
    std::wstring* outIcon)
{
    if (!namespacedId || !outId || !outDescId || !outIcon)
        return false;

    *outId = IdRegistry::Instance().Register(IdRegistry::Type::Block, namespacedId);
    if (*outId < 0)
    {
        LogUtil::Log("[WeaveLoader] Failed to allocate block ID for '%s'", namespacedId);
        return false;
    }

    *outIcon = Utf8ToWide(iconName);
    *outDescId = -1;
    if (displayName && displayName[0])
    {
        *outDescId = ModStrings::AllocateId();
        std::wstring wName = Utf8ToWide(displayName);
        ModStrings::Register(*outDescId, wName.c_str());
    }

    return true;
}

extern "C"
{

int native_register_block(
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
    int acceptsRedstonePower)
{
    if (!namespacedId) return -1;

    int id = -1;
    int descId = -1;
    std::wstring wIcon;
    if (!PrepareBlockRegistration(namespacedId, iconName, displayName, &id, &descId, &wIcon))
        return -1;

    LogUtil::Log("[WeaveLoader] Registered block '%s' -> ID %d (hardness=%.1f, resistance=%.1f)",
                 namespacedId, id, hardness, resistance);

    if (!GameObjectFactory::CreateTile(id, materialId, hardness, resistance,
                                       soundType, wIcon.empty() ? nullptr : wIcon.c_str(), lightEmission, lightBlock, descId))
    {
        LogUtil::Log("[WeaveLoader] Warning: failed to create game Tile for block '%s' id=%d", namespacedId, id);
    }

    if (requiredHarvestLevel >= 0 || requiredTool != 0 || acceptsRedstonePower != 0)
    {
        CustomBlockRegistry::Register(id, requiredHarvestLevel, requiredTool, acceptsRedstonePower != 0);
    }

    return id;
}

int native_register_managed_block(
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
    int acceptsRedstonePower)
{
    if (!namespacedId) return -1;

    int id = -1;
    int descId = -1;
    std::wstring wIcon;
    if (!PrepareBlockRegistration(namespacedId, iconName, displayName, &id, &descId, &wIcon))
        return -1;

    if (!GameObjectFactory::CreateManagedTile(id, materialId, hardness, resistance,
            soundType, wIcon.empty() ? nullptr : wIcon.c_str(), lightEmission, lightBlock, descId))
    {
        LogUtil::Log("[WeaveLoader] Warning: failed to create managed Tile for '%s' id=%d", namespacedId, id);
    }

    ManagedBlockRegistry::Register(id);

    if (requiredHarvestLevel >= 0 || requiredTool != 0 || acceptsRedstonePower != 0)
    {
        CustomBlockRegistry::Register(id, requiredHarvestLevel, requiredTool, acceptsRedstonePower != 0);
    }

    return id;
}

int native_register_falling_block(
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
    int acceptsRedstonePower)
{
    int id = -1;
    int descId = -1;
    std::wstring wIcon;
    if (!PrepareBlockRegistration(namespacedId, iconName, displayName, &id, &descId, &wIcon))
        return -1;

    if (!GameObjectFactory::CreateFallingTile(id, materialId, hardness, resistance,
            soundType, wIcon.empty() ? nullptr : wIcon.c_str(), lightEmission, lightBlock, descId))
    {
        LogUtil::Log("[WeaveLoader] Warning: failed to create falling Tile for '%s' id=%d", namespacedId, id);
    }

    ManagedBlockRegistry::Register(id);

    if (requiredHarvestLevel >= 0 || requiredTool != 0 || acceptsRedstonePower != 0)
        CustomBlockRegistry::Register(id, requiredHarvestLevel, requiredTool, acceptsRedstonePower != 0);

    return id;
}

int native_register_slab_block(
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
    int* outDoubleBlockNumericId)
{
    if (!namespacedId) return -1;

    int halfId = -1;
    int descId = -1;
    std::wstring wIcon;
    if (!PrepareBlockRegistration(namespacedId, iconName, displayName, &halfId, &descId, &wIcon))
        return -1;

    std::string fullName = std::string(namespacedId) + "_double";
    int fullId = IdRegistry::Instance().Register(IdRegistry::Type::Block, fullName.c_str());
    if (fullId < 0)
    {
        LogUtil::Log("[WeaveLoader] Failed to allocate double slab block ID for '%s'", namespacedId);
        return -1;
    }

    if (outDoubleBlockNumericId)
        *outDoubleBlockNumericId = fullId;

    if (!GameObjectFactory::CreateSlabPair(halfId, fullId, materialId, hardness, resistance,
            soundType, wIcon.empty() ? nullptr : wIcon.c_str(), lightEmission, lightBlock, descId))
    {
        LogUtil::Log("[WeaveLoader] Warning: failed to create slab pair for '%s' half=%d full=%d", namespacedId, halfId, fullId);
    }

    if (requiredHarvestLevel >= 0 || requiredTool != 0 || acceptsRedstonePower != 0)
    {
        CustomBlockRegistry::Register(halfId, requiredHarvestLevel, requiredTool, acceptsRedstonePower != 0);
        CustomBlockRegistry::Register(fullId, requiredHarvestLevel, requiredTool, acceptsRedstonePower != 0);
    }

    return halfId;
}

void native_configure_managed_block(int numericBlockId, int dropNumericBlockId, int cloneNumericBlockId)
{
    if (numericBlockId < 0)
        return;
    ManagedBlockRegistry::Configure(numericBlockId, dropNumericBlockId, cloneNumericBlockId);
}

int native_register_item(
    const char* namespacedId,
    int maxStackSize,
    int maxDamage,
    const char* iconName,
    const char* displayName)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Item, namespacedId);
    if (id < 0)
    {
        LogUtil::Log("[WeaveLoader] Failed to allocate item ID for '%s'", namespacedId);
        return -1;
    }

    LogUtil::Log("[WeaveLoader] Registered item '%s' -> ID %d (stack=%d, durability=%d)",
                 namespacedId, id, maxStackSize, maxDamage);

    std::wstring wIcon = Utf8ToWide(iconName);

    int descId = -1;
    if (displayName && displayName[0])
    {
        descId = ModStrings::AllocateId();
        std::wstring wName = Utf8ToWide(displayName);
        ModStrings::Register(descId, wName.c_str());
    }

    if (!GameObjectFactory::CreateItem(id, maxStackSize, maxDamage,
                                       wIcon.empty() ? nullptr : wIcon.c_str(), descId))
    {
        LogUtil::Log("[WeaveLoader] Warning: failed to create game Item for '%s' id=%d", namespacedId, id);
    }

    return id;
}

int native_register_pickaxe_item(
    const char* namespacedId,
    int tier,
    int maxDamage,
    const char* iconName,
    const char* displayName)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Item, namespacedId);
    if (id < 0)
    {
        LogUtil::Log("[WeaveLoader] Failed to allocate pickaxe item ID for '%s'", namespacedId);
        return -1;
    }

    LogUtil::Log("[WeaveLoader] Registered pickaxe item '%s' -> ID %d (tier=%d, durability=%d)",
                 namespacedId, id, tier, maxDamage);

    std::wstring wIcon = Utf8ToWide(iconName);

    int descId = -1;
    if (displayName && displayName[0])
    {
        descId = ModStrings::AllocateId();
        std::wstring wName = Utf8ToWide(displayName);
        ModStrings::Register(descId, wName.c_str());
    }

    if (!GameObjectFactory::CreatePickaxeItem(id, tier, maxDamage,
                                              wIcon.empty() ? nullptr : wIcon.c_str(), descId))
    {
        LogUtil::Log("[WeaveLoader] Warning: failed to create native PickaxeItem for '%s' id=%d", namespacedId, id);
    }

    return id;
}

int native_register_shovel_item(
    const char* namespacedId,
    int tier,
    int maxDamage,
    const char* iconName,
    const char* displayName)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Item, namespacedId);
    if (id < 0) return -1;

    std::wstring wIcon = Utf8ToWide(iconName);
    int descId = -1;
    if (displayName && displayName[0])
    {
        descId = ModStrings::AllocateId();
        std::wstring wName = Utf8ToWide(displayName);
        ModStrings::Register(descId, wName.c_str());
    }

    if (!GameObjectFactory::CreateShovelItem(id, tier, maxDamage, wIcon.empty() ? nullptr : wIcon.c_str(), descId))
        LogUtil::Log("[WeaveLoader] Warning: failed to create native ShovelItem for '%s' id=%d", namespacedId, id);

    return id;
}

int native_register_hoe_item(
    const char* namespacedId,
    int tier,
    int maxDamage,
    const char* iconName,
    const char* displayName)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Item, namespacedId);
    if (id < 0) return -1;

    std::wstring wIcon = Utf8ToWide(iconName);
    int descId = -1;
    if (displayName && displayName[0])
    {
        descId = ModStrings::AllocateId();
        std::wstring wName = Utf8ToWide(displayName);
        ModStrings::Register(descId, wName.c_str());
    }

    if (!GameObjectFactory::CreateHoeItem(id, tier, maxDamage, wIcon.empty() ? nullptr : wIcon.c_str(), descId))
        LogUtil::Log("[WeaveLoader] Warning: failed to create native HoeItem for '%s' id=%d", namespacedId, id);

    return id;
}

int native_register_axe_item(
    const char* namespacedId,
    int tier,
    int maxDamage,
    const char* iconName,
    const char* displayName)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Item, namespacedId);
    if (id < 0) return -1;

    std::wstring wIcon = Utf8ToWide(iconName);
    int descId = -1;
    if (displayName && displayName[0])
    {
        descId = ModStrings::AllocateId();
        std::wstring wName = Utf8ToWide(displayName);
        ModStrings::Register(descId, wName.c_str());
    }

    if (!GameObjectFactory::CreateAxeItem(id, tier, maxDamage, wIcon.empty() ? nullptr : wIcon.c_str(), descId))
        LogUtil::Log("[WeaveLoader] Warning: failed to create native HatchetItem for '%s' id=%d", namespacedId, id);

    return id;
}

int native_register_sword_item(
    const char* namespacedId,
    int tier,
    int maxDamage,
    const char* iconName,
    const char* displayName)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Item, namespacedId);
    if (id < 0) return -1;

    std::wstring wIcon = Utf8ToWide(iconName);
    int descId = -1;
    if (displayName && displayName[0])
    {
        descId = ModStrings::AllocateId();
        std::wstring wName = Utf8ToWide(displayName);
        ModStrings::Register(descId, wName.c_str());
    }

    if (!GameObjectFactory::CreateSwordItem(id, tier, maxDamage, wIcon.empty() ? nullptr : wIcon.c_str(), descId))
        LogUtil::Log("[WeaveLoader] Warning: failed to create native WeaponItem for '%s' id=%d", namespacedId, id);

    return id;
}

int native_configure_custom_pickaxe_item(
    int numericItemId,
    int harvestLevel,
    float destroySpeed)
{
    return native_configure_custom_tool_item(
        numericItemId,
        static_cast<int>(CustomToolMaterialRegistry::ToolKind::Pickaxe),
        harvestLevel,
        destroySpeed,
        0.0f);
}

int native_configure_custom_tool_item(
    int numericItemId,
    int toolKind,
    int harvestLevel,
    float destroySpeed,
    float attackDamage)
{
    if (numericItemId < 0 || harvestLevel < 0 || destroySpeed <= 0.0f)
        return 0;

    const auto kind = static_cast<CustomToolMaterialRegistry::ToolKind>(toolKind);
    CustomToolMaterialRegistry::Register(numericItemId, kind, harvestLevel, destroySpeed, attackDamage);
    if (kind == CustomToolMaterialRegistry::ToolKind::Pickaxe)
    {
        CustomPickaxeRegistry::Register(numericItemId, harvestLevel, destroySpeed);
    }

    void* itemPtr = GameObjectFactory::FindItem(numericItemId);
    if (itemPtr)
    {
        switch (kind)
        {
        case CustomToolMaterialRegistry::ToolKind::Pickaxe:
        case CustomToolMaterialRegistry::ToolKind::Shovel:
        case CustomToolMaterialRegistry::ToolKind::Axe:
            if (destroySpeed > 0.0f)
                *reinterpret_cast<float*>(static_cast<char*>(itemPtr) + 0xA0) = destroySpeed;
            if (attackDamage > 0.0f)
                *reinterpret_cast<float*>(static_cast<char*>(itemPtr) + 0xA4) = attackDamage;
            break;
        case CustomToolMaterialRegistry::ToolKind::Sword:
            if (attackDamage > 0.0f)
                *reinterpret_cast<float*>(static_cast<char*>(itemPtr) + 0x98) = attackDamage;
            break;
        case CustomToolMaterialRegistry::ToolKind::Hoe:
            break;
        }
    }

    LogUtil::Log("[WeaveLoader] Configured custom tool item id=%d (kind=%d harvest=%d speed=%.2f attack=%.2f)",
                 numericItemId, toolKind, harvestLevel, destroySpeed, attackDamage);
    return 1;
}

int native_allocate_description_id()
{
    return ModStrings::AllocateId();
}

void native_register_string(int descriptionId, const char* displayName)
{
    if (!displayName) return;
    std::wstring wName = Utf8ToWide(displayName);
    ModStrings::Register(descriptionId, wName.c_str());
}

int native_register_entity(
    const char* namespacedId,
    float width,
    float height,
    int trackingRange)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Entity, namespacedId);
    if (id < 0)
    {
        LogUtil::Log("[WeaveLoader] Failed to allocate entity ID for '%s'", namespacedId);
        return -1;
    }

    LogUtil::Log("[WeaveLoader] Registered entity '%s' -> ID %d (%.1fx%.1f)",
                 namespacedId, id, width, height);

    return id;
}

void native_add_shaped_recipe(
    const char* resultId,
    int resultCount,
    const char* pattern,
    const char* ingredientIds)
{
    LogUtil::Log("[WeaveLoader] Added shaped recipe: %dx %s", resultCount, resultId);
}

void native_add_furnace_recipe(
    const char* inputId,
    const char* outputId,
    float xp)
{
    int inputNumeric = ResolveRecipeId(inputId, false);
    int outputNumeric = ResolveRecipeId(outputId, true);

    if (inputNumeric < 0)
    {
        LogUtil::Log("[WeaveLoader] Failed furnace recipe: unknown input '%s'", inputId ? inputId : "(null)");
        return;
    }

    if (outputNumeric < 0)
    {
        LogUtil::Log("[WeaveLoader] Failed furnace recipe: unknown output '%s'", outputId ? outputId : "(null)");
        return;
    }

    if (!FurnaceRecipeRegistry::AddRecipe(inputNumeric, outputNumeric, xp))
    {
        LogUtil::Log("[WeaveLoader] Failed furnace recipe: %s -> %s (%.1f xp)", inputId, outputId, xp);
        return;
    }

    LogUtil::Log("[WeaveLoader] Added furnace recipe: %s[%d] -> %s[%d] (%.1f xp)",
                 inputId, inputNumeric, outputId, outputNumeric, xp);
}

void native_log(const char* message, int level)
{
    if (message)
        LogUtil::Log("%s", message);
}

int native_get_block_id(const char* namespacedId)
{
    if (!namespacedId) return -1;
    return IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, namespacedId);
}

int native_get_item_id(const char* namespacedId)
{
    if (!namespacedId) return -1;
    return IdRegistry::Instance().GetNumericId(IdRegistry::Type::Item, namespacedId);
}

int native_get_entity_id(const char* namespacedId)
{
    if (!namespacedId) return -1;
    return IdRegistry::Instance().GetNumericId(IdRegistry::Type::Entity, namespacedId);
}

int native_consume_item_from_player(void* playerPtr, int numericItemId, int count)
{
    if (numericItemId < 0 || count <= 0)
        return 0;
    return GameHooks::ConsumePlayerResource(playerPtr, numericItemId, count) ? 1 : 0;
}

int native_damage_item_instance(void* itemInstancePtr, int amount, void* ownerSharedPtr)
{
    if (amount <= 0)
        return 0;
    return GameHooks::DamageItemInstance(itemInstancePtr, amount, ownerSharedPtr) ? 1 : 0;
}

int native_spawn_entity_from_player_look(void* playerPtr, void* playerSharedPtr, int numericEntityId, double speed, double spawnForward, double spawnUp)
{
    if (numericEntityId < 0)
        return 0;
    return GameHooks::SummonEntityFromPlayerLook(playerPtr, playerSharedPtr, numericEntityId, speed, spawnForward, spawnUp) ? 1 : 0;
}

int native_summon_entity_by_id(int numericEntityId, double x, double y, double z)
{
    if (!GameHooks::SummonEntityByNumericId(numericEntityId, x, y, z))
    {
        LogUtil::Log("[WeaveLoader] Summon failed: entity=%d at (%.2f, %.2f, %.2f)",
                     numericEntityId, x, y, z);
        return 0;
    }

    LogUtil::Log("[WeaveLoader] Summoned entity=%d at (%.2f, %.2f, %.2f)",
                 numericEntityId, x, y, z);
    return 1;
}

int native_level_has_neighbor_signal(void* levelPtr, int x, int y, int z)
{
    return (levelPtr && s_levelHasNeighborSignal && s_levelHasNeighborSignal(levelPtr, x, y, z)) ? 1 : 0;
}

int native_level_set_tile(void* levelPtr, int x, int y, int z, int blockId, int data, int flags)
{
    return (levelPtr && s_levelSetTileAndData && s_levelSetTileAndData(levelPtr, x, y, z, blockId, data, flags)) ? 1 : 0;
}

int native_level_schedule_tick(void* levelPtr, int x, int y, int z, int blockId, int delay)
{
    if (!levelPtr)
        return 0;
    if (ManagedBlockRegistry::IsManaged(blockId))
    {
        GameHooks::EnqueueManagedBlockTick(levelPtr, x, y, z, blockId, delay);
        return 1;
    }
    if (!s_levelAddToTickNextTick)
        return 0;
    s_levelAddToTickNextTick(levelPtr, x, y, z, blockId, delay);
    return 1;
}

int native_level_get_tile(void* levelPtr, int x, int y, int z)
{
    if (!levelPtr || !s_levelGetTile)
        return -1;
    return s_levelGetTile(levelPtr, x, y, z);
}

int native_summon_entity(const char* namespacedId, double x, double y, double z)
{
    if (!namespacedId || !namespacedId[0])
        return 0;

    const int entityNumericId =
        IdRegistry::Instance().GetNumericId(IdRegistry::Type::Entity, namespacedId);
    if (entityNumericId < 0)
    {
        LogUtil::Log("[WeaveLoader] Summon failed: unknown entity id '%s'", namespacedId);
        return 0;
    }

    return native_summon_entity_by_id(entityNumericId, x, y, z);
}

void native_subscribe_event(const char* eventName, void* managedFnPtr)
{
    LogUtil::Log("[WeaveLoader] Event subscription: %s", eventName ? eventName : "(null)");
}

void native_add_to_creative(int numericId, int count, int auxValue, int groupIndex)
{
    CreativeInventory::AddPending(numericId, count, auxValue, groupIndex);
}

} // extern "C"
