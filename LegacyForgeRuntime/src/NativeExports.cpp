#include "NativeExports.h"
#include "IdRegistry.h"
#include <cstdio>
#include <cstring>

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
    int lightBlock)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Block, namespacedId);
    if (id < 0)
    {
        printf("[LegacyForge] Failed to allocate block ID for '%s'\n", namespacedId);
        return -1;
    }

    // TODO: Once we have access to game headers, create a GenericTile instance
    // and insert it into Tile::tiles[id]. For now we just allocate the ID.
    printf("[LegacyForge] Registered block '%s' -> ID %d (hardness=%.1f, resistance=%.1f)\n",
           namespacedId, id, hardness, resistance);

    return id;
}

int native_register_item(
    const char* namespacedId,
    int maxStackSize,
    int maxDamage)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Item, namespacedId);
    if (id < 0)
    {
        printf("[LegacyForge] Failed to allocate item ID for '%s'\n", namespacedId);
        return -1;
    }

    // TODO: Create GenericItem and insert into Item::items[256 + id]
    printf("[LegacyForge] Registered item '%s' -> ID %d (stack=%d, durability=%d)\n",
           namespacedId, id, maxStackSize, maxDamage);

    return id;
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
        printf("[LegacyForge] Failed to allocate entity ID for '%s'\n", namespacedId);
        return -1;
    }

    // TODO: Register with EntityIO via resolved PDB symbols
    printf("[LegacyForge] Registered entity '%s' -> ID %d (%.1fx%.1f)\n",
           namespacedId, id, width, height);

    return id;
}

void native_add_shaped_recipe(
    const char* resultId,
    int resultCount,
    const char* pattern,
    const char* ingredientIds)
{
    // TODO: Parse pattern and ingredients, call game's recipe registration
    printf("[LegacyForge] Added shaped recipe: %dx %s\n", resultCount, resultId);
}

void native_add_furnace_recipe(
    const char* inputId,
    const char* outputId,
    float xp)
{
    // TODO: Call game's FurnaceRecipes::addRecipe via resolved symbols
    printf("[LegacyForge] Added furnace recipe: %s -> %s (%.1f xp)\n", inputId, outputId, xp);
}

void native_log(const char* message, int level)
{
    if (message)
        printf("%s\n", message);
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

void native_subscribe_event(const char* eventName, void* managedFnPtr)
{
    // TODO: Store managed callback pointers and invoke from game hooks
    printf("[LegacyForge] Event subscription: %s\n", eventName ? eventName : "(null)");
}

} // extern "C"
