#include "FurnaceRecipeRegistry.h"
#include "SymbolResolver.h"
#include "PdbParser.h"
#include "LogUtil.h"
#include <unordered_map>
#include <cstring>

namespace FurnaceRecipeRegistry
{

typedef void (__fastcall *ItemInstanceCtor_fn)(void* thisPtr, int id, int count, int auxValue);

struct FurnaceRecipesLayout
{
    std::unordered_map<int, void*> recipies;
    std::unordered_map<int, float> recipeValue;
};

static void** s_furnaceInstanceAddr = nullptr;
static ItemInstanceCtor_fn s_itemInstanceCtor = nullptr;

static const int ITEMINSTANCE_ALLOC_SIZE = 256;

bool ResolveSymbols(SymbolResolver& resolver)
{
    s_furnaceInstanceAddr = reinterpret_cast<void**>(
        resolver.Resolve("?instance@FurnaceRecipes@@0PEAV1@EA"));

    s_itemInstanceCtor = reinterpret_cast<ItemInstanceCtor_fn>(
        resolver.Resolve("??0ItemInstance@@QEAA@HHH@Z"));

    if (!s_furnaceInstanceAddr) PdbParser::DumpMatching("instance@FurnaceRecipes");
    if (!s_itemInstanceCtor) PdbParser::DumpMatching("??0ItemInstance@@QEAA@HHH@Z");

    if (s_furnaceInstanceAddr)
        LogUtil::Log("[WeaveLoader] FurnaceRecipes::instance addr @ %p", s_furnaceInstanceAddr);
    else
        LogUtil::Log("[WeaveLoader] MISSING: FurnaceRecipes::instance addr");

    if (s_itemInstanceCtor)
        LogUtil::Log("[WeaveLoader] ItemInstance::ItemInstance(int,int,int) @ %p", reinterpret_cast<void*>(s_itemInstanceCtor));
    else
        LogUtil::Log("[WeaveLoader] MISSING: ItemInstance::ItemInstance(int,int,int)");

    return s_furnaceInstanceAddr && s_itemInstanceCtor;
}

bool AddRecipe(int inputId, int outputId, float xp)
{
    if (!s_furnaceInstanceAddr || !s_itemInstanceCtor)
    {
        LogUtil::Log("[WeaveLoader] Cannot add furnace recipe: symbols not resolved");
        return false;
    }

    void* instance = *s_furnaceInstanceAddr;
    if (!instance)
    {
        LogUtil::Log("[WeaveLoader] Cannot add furnace recipe: FurnaceRecipes::instance is null");
        return false;
    }

    void* resultItem = ::operator new(ITEMINSTANCE_ALLOC_SIZE);
    memset(resultItem, 0, ITEMINSTANCE_ALLOC_SIZE);
    s_itemInstanceCtor(resultItem, outputId, 1, 0);

    auto* recipes = reinterpret_cast<FurnaceRecipesLayout*>(instance);
    recipes->recipies[inputId] = resultItem;
    recipes->recipeValue[outputId] = xp;

    LogUtil::Log("[WeaveLoader] Added furnace recipe to vanilla table: %d -> %d (%.1f xp)",
                 inputId, outputId, xp);
    return true;
}

} // namespace FurnaceRecipeRegistry
