#include "HookManager.h"
#include "GameHooks.h"
#include "ModAtlas.h"
#include "ModStrings.h"
#include "SymbolResolver.h"
#include "CreativeInventory.h"
#include "MainMenuOverlay.h"
#include "GameObjectFactory.h"
#include "FurnaceRecipeRegistry.h"
#include "NativeExports.h"
#include "WorldIdRemap.h"
#include "LogUtil.h"
#include <Windows.h>
#include <MinHook.h>
#include <unordered_map>

namespace
{
    static bool IsExecutablePtr(void* ptr)
    {
        if (!ptr)
            return false;
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(ptr, &mbi, sizeof(mbi)))
            return false;
        if (mbi.State != MEM_COMMIT)
            return false;
        const DWORD protect = mbi.Protect & 0xFF;
        return protect == PAGE_EXECUTE
            || protect == PAGE_EXECUTE_READ
            || protect == PAGE_EXECUTE_READWRITE
            || protect == PAGE_EXECUTE_WRITECOPY;
    }

    static MH_STATUS CreateHookChecked(const SymbolResolver& symbols, void* target, void* detour, void** original)
    {
        if (!target)
            return MH_ERROR_NOT_EXECUTABLE;
        if (symbols.IsStub(target))
            return MH_ERROR_NOT_EXECUTABLE;
        if (!IsExecutablePtr(target))
            return MH_ERROR_NOT_EXECUTABLE;
        return MH_CreateHook(target, detour, original);
    }
}

bool HookManager::Install(const SymbolResolver& symbols)
{
    if (MH_Initialize() != MH_OK)
    {
        LogUtil::Log("[WeaveLoader] MH_Initialize failed");
        return false;
    }

    WorldIdRemap::SetTagNewTagSymbol(symbols.Nbt.pTagNewTag);
    WorldIdRemap::SetTileArraySymbol(symbols.Tile.pTileTiles);
    WorldIdRemap::SetLevelChunkTileSymbols(
        symbols.Level.pLevelChunkGetTile,
        symbols.Level.pLevelChunkSetTile,
        symbols.Level.pLevelChunkGetData,
        symbols.Level.pLevelChunkSetTileAndData,
        symbols.Level.pLevelChunkGetPos,
        symbols.Level.pLevelChunkGetHighestNonEmptyY);
    WorldIdRemap::SetCompressedTileStorageSetSymbol(symbols.Level.pCompressedTileStorageSet);

    GameHooks::TileRenderer_TesselateBlockInWorld =
        reinterpret_cast<TileRendererTesselateBlockInWorld_fn>(symbols.Tile.pTileRendererTesselateBlockInWorld);
    GameHooks::TileRenderer_SetShape =
        reinterpret_cast<TileRendererSetShape_fn>(symbols.Tile.pTileRendererSetShape);
    GameHooks::TileRenderer_SetShapeTile =
        reinterpret_cast<TileRendererSetShapeTile_fn>(symbols.Tile.pTileRendererSetShapeTile);
    GameHooks::Tile_SetShape =
        reinterpret_cast<TileSetShape_fn>(symbols.Tile.pTileSetShape);
    GameHooks::AABB_NewTemp =
        reinterpret_cast<AABBNewTemp_fn>(symbols.Tile.pAABBNewTemp);
    GameHooks::AABB_Clip =
        reinterpret_cast<AABBClip_fn>(symbols.Tile.pAABBClip);
    GameHooks::Vec3_NewTemp =
        reinterpret_cast<Vec3NewTemp_fn>(symbols.Tile.pVec3NewTemp);
    GameHooks::HitResult_Ctor =
        reinterpret_cast<HitResultCtor_fn>(symbols.Tile.pHitResultCtor);
    GameHooks::Level_GetData =
        reinterpret_cast<LevelGetData_fn>(symbols.Level.pLevelGetData);

    if (symbols.Core.pRunStaticCtors)
    {
        if (CreateHookChecked(symbols, symbols.Core.pRunStaticCtors,
                          reinterpret_cast<void*>(&GameHooks::Hooked_RunStaticCtors),
                          reinterpret_cast<void**>(&GameHooks::Original_RunStaticCtors)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Failed to hook RunStaticCtors");
            return false;
        }
        LogUtil::Log("[WeaveLoader] Hooked RunStaticCtors");
    }

    if (symbols.Core.pMinecraftTick)
    {
        if (CreateHookChecked(symbols, symbols.Core.pMinecraftTick,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftTick),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftTick)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Failed to hook Minecraft::tick");
            return false;
        }
        LogUtil::Log("[WeaveLoader] Hooked Minecraft::tick");
    }

    if (symbols.Core.pMinecraftInit)
    {
        if (CreateHookChecked(symbols, symbols.Core.pMinecraftInit,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftInit),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftInit)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Failed to hook Minecraft::init");
            return false;
        }
        LogUtil::Log("[WeaveLoader] Hooked Minecraft::init");
    }

    if (symbols.Core.pMinecraftSetLevel)
    {
        if (CreateHookChecked(symbols, symbols.Core.pMinecraftSetLevel,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftSetLevel),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftSetLevel)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Minecraft::setLevel");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Minecraft::setLevel (active level tracking)");
        }
    }

    if (symbols.Item.pItemInstanceMineBlock)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInstanceMineBlock,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceMineBlock),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceMineBlock)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::mineBlock");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::mineBlock (managed item callbacks)");
        }
    }

    if (symbols.Entity.pServerPlayerGameModeUseItemOn)
    {
        if (CreateHookChecked(symbols, symbols.Entity.pServerPlayerGameModeUseItemOn,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ServerPlayerGameModeUseItemOn),
                          reinterpret_cast<void**>(&GameHooks::Original_ServerPlayerGameModeUseItemOn)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ServerPlayerGameMode::useItemOn");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ServerPlayerGameMode::useItemOn (placement tracking)");
        }
    }

    if (symbols.Entity.pMultiPlayerGameModeUseItemOn)
    {
        if (CreateHookChecked(symbols, symbols.Entity.pMultiPlayerGameModeUseItemOn,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MultiPlayerGameModeUseItemOn),
                          reinterpret_cast<void**>(&GameHooks::Original_MultiPlayerGameModeUseItemOn)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook MultiPlayerGameMode::useItemOn");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked MultiPlayerGameMode::useItemOn (placement tracking)");
        }
    }

    if (symbols.Item.pItemInstanceUseOn)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInstanceUseOn,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceUseOn),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceUseOn)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::useOn");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::useOn (placement tracking)");
        }
    }

    if (symbols.Item.pItemInstanceSave)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInstanceSave,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceSave),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceSave)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::save");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::save (namespace remap)");
        }
    }

    if (symbols.Item.pItemInstanceLoad)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInstanceLoad,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceLoad),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceLoad)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::load");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::load (namespace remap)");
        }
    }

    if (symbols.Item.pItemInstanceInventoryTick)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInstanceInventoryTick,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceInventoryTick),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceInventoryTick)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::inventoryTick");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::inventoryTick (managed item callbacks)");
        }
    }

    if (symbols.Item.pItemInstanceOnCraftedBy)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInstanceOnCraftedBy,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceOnCraftedBy),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceOnCraftedBy)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::onCraftedBy");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::onCraftedBy (managed item callbacks)");
        }
    }

    if (symbols.Item.pItemInstanceInteractEnemy)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInstanceInteractEnemy,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceInteractEnemy),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceInteractEnemy)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::interactEnemy");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::interactEnemy (managed item callbacks)");
        }
    }

    if (symbols.Item.pItemInstanceHurtEnemy)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInstanceHurtEnemy,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceHurtEnemy),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceHurtEnemy)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::hurtEnemy");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::hurtEnemy (managed item callbacks)");
        }
    }

    if (symbols.Entity.pEntityPlayStepSound)
    {
        if (CreateHookChecked(symbols, symbols.Entity.pEntityPlayStepSound,
                          reinterpret_cast<void*>(&GameHooks::Hooked_EntityPlayStepSound),
                          reinterpret_cast<void**>(&GameHooks::Original_EntityPlayStepSound)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Entity::playStepSound");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Entity::playStepSound (step-on callbacks)");
        }
    }

    if (symbols.Entity.pEntityCheckInsideTiles)
    {
        if (CreateHookChecked(symbols, symbols.Entity.pEntityCheckInsideTiles,
                          reinterpret_cast<void*>(&GameHooks::Hooked_EntityCheckInsideTiles),
                          reinterpret_cast<void**>(&GameHooks::Original_EntityCheckInsideTiles)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Entity::checkInsideTiles");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Entity::checkInsideTiles (entity-inside callbacks)");
        }
    }

    std::unordered_map<void*, const char*> installedDropLootTargets;
    auto hookDropLoot = [&](void* target, void* hookFn, void** originalFn, const char* name)
    {
        if (!target)
            return;
        if (target == symbols.Tile.pTileOnPlace)
        {
            LogUtil::Log("[WeaveLoader] Hook coverage: skipping %s (shared stub address)", name);
            return;
        }
        auto it = installedDropLootTargets.find(target);
        if (it != installedDropLootTargets.end())
        {
            LogUtil::Log("[WeaveLoader] Hook coverage: %s shares address with %s", name, it->second);
            return;
        }

        const MH_STATUS status = CreateHookChecked(symbols, target, hookFn, originalFn);
        if (status != MH_OK)
        {
            if (status == MH_ERROR_ALREADY_CREATED)
            {
                LogUtil::Log("[WeaveLoader] Hook coverage: %s already hooked elsewhere", name);
                return;
            }

            LogUtil::Log("[WeaveLoader] Warning: Failed to hook %s (status=%d)", name, static_cast<int>(status));
        }
        else
        {
            installedDropLootTargets[target] = name;
            LogUtil::Log("[WeaveLoader] Hooked %s (loot tables)", name);
        }
    };

    hookDropLoot(symbols.Entity.pMobDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_MobDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_MobDropDeathLoot),
                 "Mob::dropDeathLoot");
    hookDropLoot(symbols.Entity.pChickenDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_ChickenDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_ChickenDropDeathLoot),
                 "Chicken::dropDeathLoot");
    hookDropLoot(symbols.Entity.pCowDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_CowDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_CowDropDeathLoot),
                 "Cow::dropDeathLoot");
    hookDropLoot(symbols.Entity.pPigDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_PigDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_PigDropDeathLoot),
                 "Pig::dropDeathLoot");
    hookDropLoot(symbols.Entity.pSheepDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_SheepDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_SheepDropDeathLoot),
                 "Sheep::dropDeathLoot");
    hookDropLoot(symbols.Entity.pSquidDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_SquidDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_SquidDropDeathLoot),
                 "Squid::dropDeathLoot");
    hookDropLoot(symbols.Entity.pOcelotDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_OcelotDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_OcelotDropDeathLoot),
                 "Ocelot::dropDeathLoot");
    hookDropLoot(symbols.Entity.pSnowManDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_SnowManDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_SnowManDropDeathLoot),
                 "SnowMan::dropDeathLoot");
    hookDropLoot(symbols.Entity.pVillagerGolemDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_VillagerGolemDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_VillagerGolemDropDeathLoot),
                 "VillagerGolem::dropDeathLoot");
    hookDropLoot(symbols.Entity.pPigZombieDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_PigZombieDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_PigZombieDropDeathLoot),
                 "PigZombie::dropDeathLoot");
    hookDropLoot(symbols.Entity.pSpiderDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_SpiderDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_SpiderDropDeathLoot),
                 "Spider::dropDeathLoot");
    hookDropLoot(symbols.Entity.pSkeletonDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_SkeletonDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_SkeletonDropDeathLoot),
                 "Skeleton::dropDeathLoot");
    hookDropLoot(symbols.Entity.pWitchDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_WitchDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_WitchDropDeathLoot),
                 "Witch::dropDeathLoot");
    hookDropLoot(symbols.Entity.pBlazeDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_BlazeDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_BlazeDropDeathLoot),
                 "Blaze::dropDeathLoot");
    hookDropLoot(symbols.Entity.pEnderManDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_EnderManDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_EnderManDropDeathLoot),
                 "EnderMan::dropDeathLoot");
    hookDropLoot(symbols.Entity.pGhastDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_GhastDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_GhastDropDeathLoot),
                 "Ghast::dropDeathLoot");
    hookDropLoot(symbols.Entity.pLavaSlimeDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_LavaSlimeDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_LavaSlimeDropDeathLoot),
                 "LavaSlime::dropDeathLoot");
    hookDropLoot(symbols.Entity.pWitherBossDropDeathLoot,
                 reinterpret_cast<void*>(&GameHooks::Hooked_WitherBossDropDeathLoot),
                 reinterpret_cast<void**>(&GameHooks::Original_WitherBossDropDeathLoot),
                 "WitherBoss::dropDeathLoot");


    if (symbols.Item.pItemInstanceGetIcon)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInstanceGetIcon,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceGetIcon),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceGetIcon)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::getIcon");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::getIcon (atlas page routing)");
        }
    }

    if (symbols.Texture.pEntityRendererBindTextureResource)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pEntityRendererBindTextureResource,
                          reinterpret_cast<void*>(&GameHooks::Hooked_EntityRendererBindTextureResource),
                          reinterpret_cast<void**>(&GameHooks::Original_EntityRendererBindTextureResource)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook EntityRenderer::bindTexture(ResourceLocation)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked EntityRenderer::bindTexture(ResourceLocation) (dropped item atlas routing)");
        }
    }

    if (symbols.Texture.pItemRendererRenderItemBillboard)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pItemRendererRenderItemBillboard,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemRendererRenderItemBillboard),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemRendererRenderItemBillboard)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemRenderer::renderItemBillboard");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemRenderer::renderItemBillboard (dropped item atlas routing)");
        }
    }

    if (symbols.Item.pItemRendererRenderGuiItem)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemRendererRenderGuiItem,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemRendererRenderGuiItem),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemRendererRenderGuiItem)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemRenderer::renderGuiItem");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemRenderer::renderGuiItem (item display transforms)");
        }
    }

    if (symbols.Item.pItemInHandRendererRender)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInHandRendererRender,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInHandRendererRender),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInHandRendererRender)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInHandRenderer::render");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInHandRenderer::render (first-person context)");
        }
    }

    if (symbols.Item.pItemInHandRendererRenderItem)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemInHandRendererRenderItem,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInHandRendererRenderItem),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInHandRendererRenderItem)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInHandRenderer::renderItem");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInHandRenderer::renderItem (item display transforms)");
        }
    }

    if (symbols.Texture.pCompassTextureCycleFrames)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pCompassTextureCycleFrames,
                          reinterpret_cast<void*>(&GameHooks::Hooked_CompassTextureCycleFrames),
                          reinterpret_cast<void**>(&GameHooks::Original_CompassTextureCycleFrames)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook CompassTexture::cycleFrames");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked CompassTexture::cycleFrames (texture pack crash guard)");
        }
    }

    if (symbols.Texture.pClockTextureCycleFrames)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pClockTextureCycleFrames,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ClockTextureCycleFrames),
                          reinterpret_cast<void**>(&GameHooks::Original_ClockTextureCycleFrames)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ClockTexture::cycleFrames");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ClockTexture::cycleFrames (texture pack crash guard)");
        }
    }

    if (symbols.Texture.pCompassTextureGetSourceWidth)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pCompassTextureGetSourceWidth,
                          reinterpret_cast<void*>(&GameHooks::Hooked_CompassTextureGetSourceWidth),
                          reinterpret_cast<void**>(&GameHooks::Original_CompassTextureGetSourceWidth)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook CompassTexture::getSourceWidth");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked CompassTexture::getSourceWidth (texture pack crash guard)");
        }
    }

    if (symbols.Texture.pCompassTextureGetSourceHeight)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pCompassTextureGetSourceHeight,
                          reinterpret_cast<void*>(&GameHooks::Hooked_CompassTextureGetSourceHeight),
                          reinterpret_cast<void**>(&GameHooks::Original_CompassTextureGetSourceHeight)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook CompassTexture::getSourceHeight");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked CompassTexture::getSourceHeight (texture pack crash guard)");
        }
    }

    if (symbols.Texture.pClockTextureGetSourceWidth)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pClockTextureGetSourceWidth,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ClockTextureGetSourceWidth),
                          reinterpret_cast<void**>(&GameHooks::Original_ClockTextureGetSourceWidth)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ClockTexture::getSourceWidth");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ClockTexture::getSourceWidth (texture pack crash guard)");
        }
    }

    if (symbols.Texture.pClockTextureGetSourceHeight)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pClockTextureGetSourceHeight,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ClockTextureGetSourceHeight),
                          reinterpret_cast<void**>(&GameHooks::Original_ClockTextureGetSourceHeight)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ClockTexture::getSourceHeight");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ClockTexture::getSourceHeight (texture pack crash guard)");
        }
    }

    if (symbols.Item.pItemMineBlock)
    {
        if (CreateHookChecked(symbols, symbols.Item.pItemMineBlock,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemMineBlock),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemMineBlock)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Item::mineBlock");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Item::mineBlock (managed item callbacks)");
        }
    }

    if (symbols.Item.pDiggerItemMineBlock)
    {
        if (CreateHookChecked(symbols, symbols.Item.pDiggerItemMineBlock,
                          reinterpret_cast<void*>(&GameHooks::Hooked_DiggerItemMineBlock),
                          reinterpret_cast<void**>(&GameHooks::Original_DiggerItemMineBlock)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook DiggerItem::mineBlock");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked DiggerItem::mineBlock (managed item callbacks)");
        }
    }

    if (symbols.Item.pPickaxeItemGetDestroySpeed)
    {
        if (CreateHookChecked(symbols, symbols.Item.pPickaxeItemGetDestroySpeed,
                          reinterpret_cast<void*>(&GameHooks::Hooked_PickaxeItemGetDestroySpeed),
                          reinterpret_cast<void**>(&GameHooks::Original_PickaxeItemGetDestroySpeed)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook PickaxeItem::getDestroySpeed");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked PickaxeItem::getDestroySpeed (custom pickaxe tiers)");
        }
    }

    if (symbols.Item.pPickaxeItemCanDestroySpecial)
    {
        if (CreateHookChecked(symbols, symbols.Item.pPickaxeItemCanDestroySpecial,
                          reinterpret_cast<void*>(&GameHooks::Hooked_PickaxeItemCanDestroySpecial),
                          reinterpret_cast<void**>(&GameHooks::Original_PickaxeItemCanDestroySpecial)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook PickaxeItem::canDestroySpecial");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked PickaxeItem::canDestroySpecial (custom pickaxe tiers)");
        }
    }

    if (symbols.Item.pShovelItemGetDestroySpeed)
    {
        if (CreateHookChecked(symbols, symbols.Item.pShovelItemGetDestroySpeed,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ShovelItemGetDestroySpeed),
                          reinterpret_cast<void**>(&GameHooks::Original_ShovelItemGetDestroySpeed)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ShovelItem::getDestroySpeed");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ShovelItem::getDestroySpeed (custom tool materials)");
        }
    }

    if (symbols.Item.pShovelItemCanDestroySpecial)
    {
        if (CreateHookChecked(symbols, symbols.Item.pShovelItemCanDestroySpecial,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ShovelItemCanDestroySpecial),
                          reinterpret_cast<void**>(&GameHooks::Original_ShovelItemCanDestroySpecial)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ShovelItem::canDestroySpecial");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ShovelItem::canDestroySpecial (custom tool materials)");
        }
    }

    if (symbols.Level.pLevelSetTileAndData)
    {
        if (CreateHookChecked(symbols, symbols.Level.pLevelSetTileAndData,
                          reinterpret_cast<void*>(&GameHooks::Hooked_LevelSetTileAndData),
                          reinterpret_cast<void**>(&GameHooks::Original_LevelSetTileAndData)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Level::setTileAndData");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Level::setTileAndData (managed block callbacks)");
        }
    }

    if (symbols.Level.pLevelSetData)
    {
        if (CreateHookChecked(symbols, symbols.Level.pLevelSetData,
                          reinterpret_cast<void*>(&GameHooks::Hooked_LevelSetData),
                          reinterpret_cast<void**>(&GameHooks::Original_LevelSetData)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Level::setData");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Level::setData (managed block callbacks)");
        }
    }

    if (symbols.Level.pLevelUpdateNeighborsAt)
    {
        if (CreateHookChecked(symbols, symbols.Level.pLevelUpdateNeighborsAt,
                          reinterpret_cast<void*>(&GameHooks::Hooked_LevelUpdateNeighborsAt),
                          reinterpret_cast<void**>(&GameHooks::Original_LevelUpdateNeighborsAt)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Level::updateNeighborsAt");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Level::updateNeighborsAt (managed block callbacks)");
        }
    }

    if (symbols.Tile.pTileUse)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileUse,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileUse),
                          reinterpret_cast<void**>(&GameHooks::Original_TileUse)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::use");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::use (managed block callbacks)");
        }
    }


    void* sharedActionTarget = symbols.Tile.pTileStepOn
        ? symbols.Tile.pTileStepOn
        : symbols.Tile.pTileFallOn;
    int sharedActionCount = 0;
    if (sharedActionTarget)
    {
        if (symbols.Tile.pTileStepOn == sharedActionTarget) sharedActionCount++;
        if (symbols.Tile.pTileFallOn == sharedActionTarget) sharedActionCount++;
    }

    const bool useSharedActionHook = sharedActionTarget && sharedActionCount >= 2;
    if (useSharedActionHook)
    {
        if (CreateHookChecked(symbols, sharedActionTarget,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileSharedAction),
                          reinterpret_cast<void**>(&GameHooks::Original_TileSharedAction)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook shared Tile action stub");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked shared Tile action stub (stepOn/fallOn)");
        }
    }
    else
    {
        if (symbols.Tile.pTileStepOn)
        {
            if (CreateHookChecked(symbols, symbols.Tile.pTileStepOn,
                              reinterpret_cast<void*>(&GameHooks::Hooked_TileStepOn),
                              reinterpret_cast<void**>(&GameHooks::Original_TileStepOn)) != MH_OK)
            {
                LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::stepOn");
            }
            else
            {
                LogUtil::Log("[WeaveLoader] Hooked Tile::stepOn (managed block callbacks)");
            }
        }

        if (symbols.Tile.pTileFallOn)
        {
            if (CreateHookChecked(symbols, symbols.Tile.pTileFallOn,
                              reinterpret_cast<void*>(&GameHooks::Hooked_TileFallOn),
                              reinterpret_cast<void**>(&GameHooks::Original_TileFallOn)) != MH_OK)
            {
                LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::fallOn");
            }
            else
            {
                LogUtil::Log("[WeaveLoader] Hooked Tile::fallOn (managed block callbacks)");
            }
        }
    }

    void* sharedLifecycleTarget = symbols.Tile.pTileDestroy
        ? symbols.Tile.pTileDestroy
        : symbols.Tile.pTileTick;
    int sharedLifecycleCount = 0;
    if (sharedLifecycleTarget)
    {
        if (symbols.Tile.pTileDestroy == sharedLifecycleTarget) sharedLifecycleCount++;
        if (symbols.Tile.pTileOnRemoving == sharedLifecycleTarget) sharedLifecycleCount++;
        if (symbols.Tile.pTileOnRemove == sharedLifecycleTarget) sharedLifecycleCount++;
        if (symbols.Tile.pTileTick == sharedLifecycleTarget) sharedLifecycleCount++;
    }

    const bool useSharedLifecycleHook = sharedLifecycleTarget && sharedLifecycleCount >= 2;
    if (useSharedLifecycleHook)
    {
        if (CreateHookChecked(symbols, sharedLifecycleTarget,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileSharedLifecycle),
                          reinterpret_cast<void**>(&GameHooks::Original_TileSharedLifecycle)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook shared Tile lifecycle stub");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked shared Tile lifecycle stub (destroy)");
        }
    }
    else
    {
        if (symbols.Tile.pTileDestroy)
        {
            if (CreateHookChecked(symbols, symbols.Tile.pTileDestroy,
                              reinterpret_cast<void*>(&GameHooks::Hooked_TileDestroy),
                              reinterpret_cast<void**>(&GameHooks::Original_TileDestroy)) != MH_OK)
            {
                LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::destroy");
            }
            else
            {
                LogUtil::Log("[WeaveLoader] Hooked Tile::destroy (managed block callbacks)");
            }
        }

    }

    if (symbols.Tile.pTilePlayerDestroy)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTilePlayerDestroy,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TilePlayerDestroy),
                          reinterpret_cast<void**>(&GameHooks::Original_TilePlayerDestroy)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::playerDestroy");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::playerDestroy (managed block callbacks)");
        }
    }

    if (symbols.Tile.pTilePlayerWillDestroy)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTilePlayerWillDestroy,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TilePlayerWillDestroy),
                          reinterpret_cast<void**>(&GameHooks::Original_TilePlayerWillDestroy)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::playerWillDestroy");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::playerWillDestroy (managed block callbacks)");
        }
    }

    if (symbols.Tile.pTileSetPlacedBy)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileSetPlacedBy,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileSetPlacedBy),
                          reinterpret_cast<void**>(&GameHooks::Original_TileSetPlacedBy)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::setPlacedBy");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::setPlacedBy (managed block callbacks)");
        }
    }

    if (symbols.Level.pServerLevelTickPendingTicks)
    {
        if (CreateHookChecked(symbols, symbols.Level.pServerLevelTickPendingTicks,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ServerLevelTickPendingTicks),
                          reinterpret_cast<void**>(&GameHooks::Original_ServerLevelTickPendingTicks)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ServerLevel::tickPendingTicks");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ServerLevel::tickPendingTicks (managed block callbacks)");
        }
    }

    if (symbols.Level.pMcRegionChunkStorageLoad)
    {
        if (CreateHookChecked(symbols, symbols.Level.pMcRegionChunkStorageLoad,
                          reinterpret_cast<void*>(&GameHooks::Hooked_McRegionChunkStorageLoad),
                          reinterpret_cast<void**>(&GameHooks::Original_McRegionChunkStorageLoad)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook McRegionChunkStorage::load");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked McRegionChunkStorage::load (block id remap)");
        }
    }

    if (symbols.Level.pMcRegionChunkStorageSave)
    {
        if (CreateHookChecked(symbols, symbols.Level.pMcRegionChunkStorageSave,
                          reinterpret_cast<void*>(&GameHooks::Hooked_McRegionChunkStorageSave),
                          reinterpret_cast<void**>(&GameHooks::Original_McRegionChunkStorageSave)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook McRegionChunkStorage::save");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked McRegionChunkStorage::save (block namespace persistence)");
        }
    }

    if (symbols.Tile.pTileGetResource)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileGetResource,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileGetResource),
                          reinterpret_cast<void**>(&GameHooks::Original_TileGetResource)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::getResource");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::getResource (managed block drops)");
        }
    }

    if (symbols.Tile.pTileGetPlacedOnFaceDataValue)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileGetPlacedOnFaceDataValue,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileGetPlacedOnFaceDataValue),
                          reinterpret_cast<void**>(&GameHooks::Original_TileGetPlacedOnFaceDataValue)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::getPlacedOnFaceDataValue");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::getPlacedOnFaceDataValue (rotation placement)");
        }
    }

    if (symbols.Tile.pTileCloneTileId)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileCloneTileId,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileCloneTileId),
                          reinterpret_cast<void**>(&GameHooks::Original_TileCloneTileId)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::cloneTileId");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::cloneTileId (managed block pick-block)");
        }
    }

    if (symbols.Tile.pTileAddAABBs)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileAddAABBs,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileAddAABBs),
                          reinterpret_cast<void**>(&GameHooks::Original_TileAddAABBs)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::addAABBs");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::addAABBs (block model collisions)");
        }
    }

    if (symbols.Tile.pTileUpdateDefaultShape)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileUpdateDefaultShape,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileUpdateDefaultShape),
                          reinterpret_cast<void**>(&GameHooks::Original_TileUpdateDefaultShape)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::updateDefaultShape");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::updateDefaultShape (block model bounds)");
        }
    }

    if (symbols.Tile.pTileIsSolidRender)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileIsSolidRender,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileIsSolidRender),
                          reinterpret_cast<void**>(&GameHooks::Original_TileIsSolidRender)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::isSolidRender");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::isSolidRender (block model culling)");
        }
    }

    if (symbols.Tile.pTileIsCubeShaped)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileIsCubeShaped,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileIsCubeShaped),
                          reinterpret_cast<void**>(&GameHooks::Original_TileIsCubeShaped)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::isCubeShaped");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::isCubeShaped (block model shape)");
        }
    }

    if (symbols.Tile.pTileClip)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileClip,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileClip),
                          reinterpret_cast<void**>(&GameHooks::Original_TileClip)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Tile::clip");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Tile::clip (block model picking)");
        }
    }

    if (symbols.Level.pLevelClip)
    {
        if (CreateHookChecked(symbols, symbols.Level.pLevelClip,
                          reinterpret_cast<void*>(&GameHooks::Hooked_LevelClip),
                          reinterpret_cast<void**>(&GameHooks::Original_LevelClip)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Level::clip");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Level::clip (block model picking)");
        }
    }
    else
    {
        LogUtil::Log("[WeaveLoader] Warning: Level::clip symbol not found; model picking disabled");
    }

    if (symbols.Tile.pTileRendererTesselateInWorld)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileRendererTesselateInWorld,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileRendererTesselateInWorld),
                          reinterpret_cast<void**>(&GameHooks::Original_TileRendererTesselateInWorld)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook TileRenderer::tesselateInWorld");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked TileRenderer::tesselateInWorld (block models)");
        }
    }

    if (symbols.Tile.pTileRendererRenderTile)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pTileRendererRenderTile,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TileRendererRenderTile),
                          reinterpret_cast<void**>(&GameHooks::Original_TileRendererRenderTile)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook TileRenderer::renderTile");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked TileRenderer::renderTile (block models in inventory)");
        }
    }

    if (symbols.Tile.pStoneSlabGetTexture)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pStoneSlabGetTexture,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StoneSlabGetTexture),
                          reinterpret_cast<void**>(&GameHooks::Original_StoneSlabGetTexture)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StoneSlabTile::getTexture");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked StoneSlabTile::getTexture (custom slabs)");
        }
    }

    if (symbols.Tile.pWoodSlabGetTexture)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pWoodSlabGetTexture,
                          reinterpret_cast<void*>(&GameHooks::Hooked_WoodSlabGetTexture),
                          reinterpret_cast<void**>(&GameHooks::Original_WoodSlabGetTexture)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook WoodSlabTile::getTexture");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked WoodSlabTile::getTexture (custom slabs)");
        }
    }

    if (symbols.Tile.pStoneSlabGetResource)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pStoneSlabGetResource,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StoneSlabGetResource),
                          reinterpret_cast<void**>(&GameHooks::Original_StoneSlabGetResource)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StoneSlabTile::getResource");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked StoneSlabTile::getResource (custom slabs)");
        }
    }

    if (symbols.Tile.pWoodSlabGetResource)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pWoodSlabGetResource,
                          reinterpret_cast<void*>(&GameHooks::Hooked_WoodSlabGetResource),
                          reinterpret_cast<void**>(&GameHooks::Original_WoodSlabGetResource)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook WoodSlabTile::getResource");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked WoodSlabTile::getResource (custom slabs)");
        }
    }

    if (symbols.Tile.pStoneSlabGetDescriptionId)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pStoneSlabGetDescriptionId,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StoneSlabGetDescriptionId),
                          reinterpret_cast<void**>(&GameHooks::Original_StoneSlabGetDescriptionId)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StoneSlabTile::getDescriptionId");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked StoneSlabTile::getDescriptionId (custom slabs)");
        }
    }

    if (symbols.Tile.pWoodSlabGetDescriptionId)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pWoodSlabGetDescriptionId,
                          reinterpret_cast<void*>(&GameHooks::Hooked_WoodSlabGetDescriptionId),
                          reinterpret_cast<void**>(&GameHooks::Original_WoodSlabGetDescriptionId)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook WoodSlabTile::getDescriptionId");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked WoodSlabTile::getDescriptionId (custom slabs)");
        }
    }

    if (symbols.Tile.pStoneSlabGetAuxName)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pStoneSlabGetAuxName,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StoneSlabGetAuxName),
                          reinterpret_cast<void**>(&GameHooks::Original_StoneSlabGetAuxName)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StoneSlabTile::getAuxName");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked StoneSlabTile::getAuxName (custom slabs)");
        }
    }

    if (symbols.Tile.pWoodSlabGetAuxName)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pWoodSlabGetAuxName,
                          reinterpret_cast<void*>(&GameHooks::Hooked_WoodSlabGetAuxName),
                          reinterpret_cast<void**>(&GameHooks::Original_WoodSlabGetAuxName)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook WoodSlabTile::getAuxName");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked WoodSlabTile::getAuxName (custom slabs)");
        }
    }

    if (symbols.Tile.pStoneSlabRegisterIcons)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pStoneSlabRegisterIcons,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StoneSlabRegisterIcons),
                          reinterpret_cast<void**>(&GameHooks::Original_StoneSlabRegisterIcons)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StoneSlabTile::registerIcons");
        }
        else
        {
            MH_EnableHook(symbols.Tile.pStoneSlabRegisterIcons);
            LogUtil::Log("[WeaveLoader] Hooked StoneSlabTile::registerIcons (custom slabs)");
        }
    }

    if (symbols.Tile.pWoodSlabRegisterIcons)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pWoodSlabRegisterIcons,
                          reinterpret_cast<void*>(&GameHooks::Hooked_WoodSlabRegisterIcons),
                          reinterpret_cast<void**>(&GameHooks::Original_WoodSlabRegisterIcons)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook WoodSlabTile::registerIcons");
        }
        else
        {
            MH_EnableHook(symbols.Tile.pWoodSlabRegisterIcons);
            LogUtil::Log("[WeaveLoader] Hooked WoodSlabTile::registerIcons (custom slabs)");
        }
    }

    if (symbols.Tile.pHalfSlabCloneTileId)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pHalfSlabCloneTileId,
                          reinterpret_cast<void*>(&GameHooks::Hooked_HalfSlabCloneTileId),
                          reinterpret_cast<void**>(&GameHooks::Original_HalfSlabCloneTileId)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook HalfSlabTile::cloneTileId");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked HalfSlabTile::cloneTileId (custom slabs)");
        }
    }

    if (symbols.Tile.pStoneSlabItemGetDescriptionId)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pStoneSlabItemGetDescriptionId,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StoneSlabItemGetDescriptionId),
                          reinterpret_cast<void**>(&GameHooks::Original_StoneSlabItemGetDescriptionId)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StoneSlabTileItem::getDescriptionId");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked StoneSlabTileItem::getDescriptionId (custom slabs)");
        }
    }

    if (symbols.Tile.pStoneSlabItemGetIcon)
    {
        if (CreateHookChecked(symbols, symbols.Tile.pStoneSlabItemGetIcon,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StoneSlabItemGetIcon),
                          reinterpret_cast<void**>(&GameHooks::Original_StoneSlabItemGetIcon)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StoneSlabTileItem::getIcon");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked StoneSlabTileItem::getIcon (custom slabs)");
        }
    }

    if (symbols.Entity.pPlayerCanDestroy)
    {
        if (CreateHookChecked(symbols, symbols.Entity.pPlayerCanDestroy,
                          reinterpret_cast<void*>(&GameHooks::Hooked_PlayerCanDestroy),
                          reinterpret_cast<void**>(&GameHooks::Original_PlayerCanDestroy)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Player::canDestroy");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Player::canDestroy (block harvest enforcement)");
        }
    }

    if (symbols.Entity.pServerPlayerGameModeUseItem)
    {
        if (CreateHookChecked(symbols, symbols.Entity.pServerPlayerGameModeUseItem,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ServerPlayerGameModeUseItem),
                          reinterpret_cast<void**>(&GameHooks::Original_ServerPlayerGameModeUseItem)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ServerPlayerGameMode::useItem");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ServerPlayerGameMode::useItem (managed item callbacks)");
        }
    }

    if (symbols.Entity.pMultiPlayerGameModeUseItem)
    {
        if (CreateHookChecked(symbols, symbols.Entity.pMultiPlayerGameModeUseItem,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MultiPlayerGameModeUseItem),
                          reinterpret_cast<void**>(&GameHooks::Original_MultiPlayerGameModeUseItem)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook MultiPlayerGameMode::useItem");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked MultiPlayerGameMode::useItem (managed item callbacks)");
        }
    }

    GameHooks::SetAtlasLocationPointers(symbols.Texture.pTextureAtlasLocationBlocks, symbols.Texture.pTextureAtlasLocationItems);
    GameHooks::SetTileTilesArray(symbols.Tile.pTileTiles);
    GameHooks::SetBlockHelperSymbols(symbols.Tile.pTileGetTextureFaceData);
    GameHooks::SetManagedBlockDispatchSymbols(symbols.Level.pLevelGetTile);
    NativeExports::SetLevelInteropSymbols(
        symbols.Level.pLevelHasNeighborSignal,
        symbols.Level.pLevelSetTileAndData,
        symbols.Level.pServerLevelAddToTickNextTick ? symbols.Level.pServerLevelAddToTickNextTick
                                              : symbols.Level.pLevelAddToTickNextTick,
        symbols.Level.pLevelGetTile);
    NativeExports::SetLocalizationSymbols(
        symbols.Core.pMinecraftApp,
        symbols.Core.pGetMinecraftLanguage,
        symbols.Core.pGetMinecraftLocale);

    if (symbols.Texture.pTexturesBindTextureResource)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pTexturesBindTextureResource,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TexturesBindTextureResource),
                          reinterpret_cast<void**>(&GameHooks::Original_TexturesBindTextureResource)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Textures::bindTexture(ResourceLocation)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Textures::bindTexture(ResourceLocation) (atlas page routing)");
        }
    }

    if (symbols.Texture.pTexturesLoadTextureByName)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pTexturesLoadTextureByName,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TexturesLoadTextureByName),
                          reinterpret_cast<void**>(&GameHooks::Original_TexturesLoadTextureByName)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Textures::loadTexture(TEXTURE_NAME,wstring)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Textures::loadTexture(TEXTURE_NAME,wstring) (virtual atlas remap)");
        }
    }

    if (symbols.Texture.pTexturesLoadTextureByIndex)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pTexturesLoadTextureByIndex,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TexturesLoadTextureByIndex),
                          reinterpret_cast<void**>(&GameHooks::Original_TexturesLoadTextureByIndex)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Textures::loadTexture(int)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Textures::loadTexture(int) (terrain atlas page routing)");
        }
    }

    if (symbols.Texture.pTexturesReadImage)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pTexturesReadImage,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TexturesReadImage),
                          reinterpret_cast<void**>(&GameHooks::Original_TexturesReadImage)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Textures::readImage");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Textures::readImage");
        }
    }

    if (symbols.Texture.pTextureManagerCreateTexture)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pTextureManagerCreateTexture,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TextureManagerCreateTexture),
                          reinterpret_cast<void**>(&GameHooks::Original_TextureManagerCreateTexture)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook TextureManager::createTexture");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked TextureManager::createTexture");
        }
    }

    if (symbols.Texture.pTextureTransferFromImage)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pTextureTransferFromImage,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TextureTransferFromImage),
                          reinterpret_cast<void**>(&GameHooks::Original_TextureTransferFromImage)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Texture::transferFromImage");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Texture::transferFromImage");
        }
    }

    if (symbols.Resource.pAbstractTexturePackGetImageResource)
    {
        if (CreateHookChecked(symbols, symbols.Resource.pAbstractTexturePackGetImageResource,
                          reinterpret_cast<void*>(&GameHooks::Hooked_AbstractTexturePackGetImageResource),
                          reinterpret_cast<void**>(&GameHooks::Original_AbstractTexturePackGetImageResource)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook AbstractTexturePack::getImageResource");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked AbstractTexturePack::getImageResource");
        }
    }

    if (symbols.Resource.pDLCTexturePackGetImageResource)
    {
        if (CreateHookChecked(symbols, symbols.Resource.pDLCTexturePackGetImageResource,
                          reinterpret_cast<void*>(&GameHooks::Hooked_DLCTexturePackGetImageResource),
                          reinterpret_cast<void**>(&GameHooks::Original_DLCTexturePackGetImageResource)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook DLCTexturePack::getImageResource");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked DLCTexturePack::getImageResource");
        }
    }

    // BufferedImage constructor hooks disabled: the work is now handled in
    // Textures::readImage for stability during boot.

    if (symbols.Texture.pStitchedGetU0)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pStitchedGetU0,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetU0),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetU0)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getU0");
        }
    }
    if (symbols.Texture.pStitchedGetU1)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pStitchedGetU1,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetU1),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetU1)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getU1");
        }
    }
    if (symbols.Texture.pStitchedGetV0)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pStitchedGetV0,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetV0),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetV0)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getV0");
        }
    }
    if (symbols.Texture.pStitchedGetV1)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pStitchedGetV1,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetV1),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetV1)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getV1");
        }
    }

    if (symbols.Core.pExitGame)
    {
        if (CreateHookChecked(symbols, symbols.Core.pExitGame,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ExitGame),
                          reinterpret_cast<void**>(&GameHooks::Original_ExitGame)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ExitGame (shutdown hook unavailable)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ExitGame");
        }
    }

    GameObjectFactory::ResolveSymbols(const_cast<SymbolResolver&>(symbols));
    FurnaceRecipeRegistry::ResolveSymbols(const_cast<SymbolResolver&>(symbols));
    GameHooks::SetSummonSymbols(
        symbols.Entity.pLevelAddEntity,
        symbols.Entity.pEntityIONewById,
        symbols.Entity.pEntityMoveTo,
        symbols.Entity.pEntitySetPos);
    GameHooks::SetItemRenderSymbols(symbols.Item.pItemEntityGetItem);
    GameHooks::SetLootSymbols(
        symbols.Texture.pOperatorNew,
        symbols.Item.pItemInstanceCtor,
        symbols.Item.pItemInstanceSharedPtrCtor,
        symbols.Item.pItemInstanceSharedPtrDtor,
        symbols.Item.pEntitySharedPtrCtor,
        symbols.Item.pEntitySharedPtrDtor,
        symbols.Item.pItemEntitySharedPtrDtor,
        symbols.Item.pItemEntityCtorWithItem,
        symbols.Item.pItemEntitySetItem,
        symbols.Item.pItemEntityMakeShared,
        symbols.Entity.pEntitySpawnAtLocation,
        symbols.Entity.pEntitySpawnAtLocationInt,
        symbols.Item.pItemInstanceSetAuxValue,
        symbols.Entity.pEntityGetEncodeId,
        symbols.Entity.pEntityGetEncodeIdById,
        symbols.Entity.pEntityGetNetworkName);
    GameHooks::SetUseActionSymbols(
        symbols.Inventory.pInventoryRemoveResource,
        symbols.Inventory.pInventoryVtable,
        symbols.Item.pItemInstanceHurtAndBreak,
        symbols.Inventory.pAbstractContainerMenuBroadcastChanges,
        symbols.Entity.pEntityGetLookAngle,
        symbols.Entity.pLivingEntityGetPos,
        symbols.Entity.pLivingEntityGetViewVector,
        symbols.Entity.pEntityLerpMotion,
        symbols.Entity.pEntitySetPos);

    if (symbols.Entity.pLivingEntityPick)
    {
        if (CreateHookChecked(symbols, symbols.Entity.pLivingEntityPick,
                          reinterpret_cast<void*>(&GameHooks::Hooked_LivingEntityPick),
                          reinterpret_cast<void**>(&GameHooks::Original_LivingEntityPick)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook LivingEntity::pick");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked LivingEntity::pick (block model picking)");
        }
    }

    if (symbols.Texture.pPreStitchedTextureMapStitch)
    {
        if (CreateHookChecked(symbols, symbols.Texture.pPreStitchedTextureMapStitch,
                          reinterpret_cast<void*>(&GameHooks::Hooked_PreStitchedTextureMapStitch),
                          reinterpret_cast<void**>(&GameHooks::Original_PreStitchedTextureMapStitch)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook PreStitchedTextureMap::stitch");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked PreStitchedTextureMap::stitch (atlas type tracking)");
        }
    }

    if (symbols.Texture.pLoadUVs && symbols.Texture.pSimpleIconCtor && symbols.Texture.pOperatorNew)
    {
        ModAtlas::SetInjectSymbols(symbols.Texture.pSimpleIconCtor, symbols.Texture.pOperatorNew);
        if (CreateHookChecked(symbols, symbols.Texture.pLoadUVs,
                          reinterpret_cast<void*>(&GameHooks::Hooked_LoadUVs),
                          reinterpret_cast<void**>(&GameHooks::Original_LoadUVs)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook PreStitchedTextureMap::loadUVs (mod textures may not appear)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked PreStitchedTextureMap::loadUVs (mod texture injection)");
        }

        if (symbols.Texture.pRegisterIcon)
        {
            if (CreateHookChecked(symbols, symbols.Texture.pRegisterIcon,
                              reinterpret_cast<void*>(&GameHooks::Hooked_RegisterIcon),
                              reinterpret_cast<void**>(&GameHooks::Original_RegisterIcon)) != MH_OK)
            {
                LogUtil::Log("[WeaveLoader] Warning: Failed to hook PreStitchedTextureMap::registerIcon");
            }
            else
            {
                LogUtil::Log("[WeaveLoader] Hooked PreStitchedTextureMap::registerIcon (mod icon lookup)");
                // Pass the trampoline to ModAtlas so it can look up vanilla icons
                // for copying internal state (field_0x48 source image pointer).
                ModAtlas::SetRegisterIconFn(GameHooks::Original_RegisterIcon);
            }
        }
    }
    else if (symbols.Texture.pLoadUVs)
    {
        LogUtil::Log("[WeaveLoader] Mod texture injection unavailable: SimpleIcon/operator new not resolved");
    }

    if (symbols.Ui.pCreativeStaticCtor)
    {
        CreativeInventory::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

        if (CreateHookChecked(symbols, symbols.Ui.pCreativeStaticCtor,
                          reinterpret_cast<void*>(&GameHooks::Hooked_CreativeStaticCtor),
                          reinterpret_cast<void**>(&GameHooks::Original_CreativeStaticCtor)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook CreativeStaticCtor");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked CreativeStaticCtor");
        }
    }

    if (symbols.Ui.pMainMenuCustomDraw)
    {
        if (CreateHookChecked(symbols, symbols.Ui.pMainMenuCustomDraw,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MainMenuCustomDraw),
                          reinterpret_cast<void**>(&GameHooks::Original_MainMenuCustomDraw)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook MainMenuCustomDraw");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked MainMenuCustomDraw");
        }
    }

    if (symbols.Core.pPresent)
    {
        MainMenuOverlay::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

        if (CreateHookChecked(symbols, symbols.Core.pPresent,
                          reinterpret_cast<void*>(&GameHooks::Hooked_Present),
                          reinterpret_cast<void**>(&GameHooks::Original_Present)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook C4JRender::Present");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked C4JRender::Present");
        }
    }

    if (symbols.Ui.pGetString)
    {
        // Read GetString prologue bytes BEFORE MinHook overwrites them.
        ModStrings::CaptureStringTableRef(symbols.Ui.pGetString);

        if (CreateHookChecked(symbols, symbols.Ui.pGetString,
                          reinterpret_cast<void*>(&GameHooks::Hooked_GetString),
                          reinterpret_cast<void**>(&GameHooks::Original_GetString)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook CMinecraftApp::GetString (mod names unavailable)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked CMinecraftApp::GetString (mod localization)");
        }
    }


    if (symbols.Resource.pGetResourceAsStream)
    {
        if (CreateHookChecked(symbols, symbols.Resource.pGetResourceAsStream,
                          reinterpret_cast<void*>(&GameHooks::Hooked_GetResourceAsStream),
                          reinterpret_cast<void**>(&GameHooks::Original_GetResourceAsStream)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook InputStream::getResourceAsStream (mod atlas unavailable)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked InputStream::getResourceAsStream (mod textures)");
        }
    }

    {
        void* pOutputDbgStr = reinterpret_cast<void*>(
            GetProcAddress(GetModuleHandleA("kernel32.dll"), "OutputDebugStringA"));
        if (pOutputDbgStr)
        {
            if (CreateHookChecked(symbols, pOutputDbgStr,
                              reinterpret_cast<void*>(&GameHooks::Hooked_OutputDebugStringA),
                              reinterpret_cast<void**>(&GameHooks::Original_OutputDebugStringA)) != MH_OK)
            {
                LogUtil::Log("[WeaveLoader] Warning: Failed to hook OutputDebugStringA");
            }
            else
            {
                LogUtil::Log("[WeaveLoader] Hooked OutputDebugStringA (game log capture)");
            }
        }
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        LogUtil::Log("[WeaveLoader] MH_EnableHook(MH_ALL_HOOKS) failed");
        return false;
    }

    LogUtil::Log("[WeaveLoader] All hooks installed and enabled");
    return true;
}

void HookManager::Cleanup()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
