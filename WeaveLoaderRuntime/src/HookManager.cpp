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
#include <MinHook.h>

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
        if (MH_CreateHook(symbols.Core.pRunStaticCtors,
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
        if (MH_CreateHook(symbols.Core.pMinecraftTick,
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
        if (MH_CreateHook(symbols.Core.pMinecraftInit,
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
        if (MH_CreateHook(symbols.Core.pMinecraftSetLevel,
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
        if (MH_CreateHook(symbols.Item.pItemInstanceMineBlock,
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
        if (MH_CreateHook(symbols.Entity.pServerPlayerGameModeUseItemOn,
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
        if (MH_CreateHook(symbols.Entity.pMultiPlayerGameModeUseItemOn,
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
        if (MH_CreateHook(symbols.Item.pItemInstanceUseOn,
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
        if (MH_CreateHook(symbols.Item.pItemInstanceSave,
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
        if (MH_CreateHook(symbols.Item.pItemInstanceLoad,
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

    if (symbols.Item.pItemInstanceGetIcon)
    {
        if (MH_CreateHook(symbols.Item.pItemInstanceGetIcon,
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
        if (MH_CreateHook(symbols.Texture.pEntityRendererBindTextureResource,
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
        if (MH_CreateHook(symbols.Texture.pItemRendererRenderItemBillboard,
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

    if (symbols.Texture.pCompassTextureCycleFrames)
    {
        if (MH_CreateHook(symbols.Texture.pCompassTextureCycleFrames,
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
        if (MH_CreateHook(symbols.Texture.pClockTextureCycleFrames,
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
        if (MH_CreateHook(symbols.Texture.pCompassTextureGetSourceWidth,
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
        if (MH_CreateHook(symbols.Texture.pCompassTextureGetSourceHeight,
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
        if (MH_CreateHook(symbols.Texture.pClockTextureGetSourceWidth,
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
        if (MH_CreateHook(symbols.Texture.pClockTextureGetSourceHeight,
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
        if (MH_CreateHook(symbols.Item.pItemMineBlock,
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
        if (MH_CreateHook(symbols.Item.pDiggerItemMineBlock,
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
        if (MH_CreateHook(symbols.Item.pPickaxeItemGetDestroySpeed,
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
        if (MH_CreateHook(symbols.Item.pPickaxeItemCanDestroySpecial,
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
        if (MH_CreateHook(symbols.Item.pShovelItemGetDestroySpeed,
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
        if (MH_CreateHook(symbols.Item.pShovelItemCanDestroySpecial,
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
        if (MH_CreateHook(symbols.Level.pLevelSetTileAndData,
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
        if (MH_CreateHook(symbols.Level.pLevelSetData,
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
        if (MH_CreateHook(symbols.Level.pLevelUpdateNeighborsAt,
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

    if (symbols.Level.pServerLevelTickPendingTicks)
    {
        if (MH_CreateHook(symbols.Level.pServerLevelTickPendingTicks,
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
        if (MH_CreateHook(symbols.Level.pMcRegionChunkStorageLoad,
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
        if (MH_CreateHook(symbols.Level.pMcRegionChunkStorageSave,
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
        if (MH_CreateHook(symbols.Tile.pTileGetResource,
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

    if (symbols.Tile.pTileCloneTileId)
    {
        if (MH_CreateHook(symbols.Tile.pTileCloneTileId,
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
        if (MH_CreateHook(symbols.Tile.pTileAddAABBs,
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
        if (MH_CreateHook(symbols.Tile.pTileUpdateDefaultShape,
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
        if (MH_CreateHook(symbols.Tile.pTileIsSolidRender,
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
        if (MH_CreateHook(symbols.Tile.pTileIsCubeShaped,
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
        if (MH_CreateHook(symbols.Tile.pTileClip,
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
        if (MH_CreateHook(symbols.Level.pLevelClip,
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
        if (MH_CreateHook(symbols.Tile.pTileRendererTesselateInWorld,
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
        if (MH_CreateHook(symbols.Tile.pTileRendererRenderTile,
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
        if (MH_CreateHook(symbols.Tile.pStoneSlabGetTexture,
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
        if (MH_CreateHook(symbols.Tile.pWoodSlabGetTexture,
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
        if (MH_CreateHook(symbols.Tile.pStoneSlabGetResource,
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
        if (MH_CreateHook(symbols.Tile.pWoodSlabGetResource,
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
        if (MH_CreateHook(symbols.Tile.pStoneSlabGetDescriptionId,
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
        if (MH_CreateHook(symbols.Tile.pWoodSlabGetDescriptionId,
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
        if (MH_CreateHook(symbols.Tile.pStoneSlabGetAuxName,
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
        if (MH_CreateHook(symbols.Tile.pWoodSlabGetAuxName,
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
        if (MH_CreateHook(symbols.Tile.pStoneSlabRegisterIcons,
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
        if (MH_CreateHook(symbols.Tile.pWoodSlabRegisterIcons,
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
        if (MH_CreateHook(symbols.Tile.pHalfSlabCloneTileId,
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
        if (MH_CreateHook(symbols.Tile.pStoneSlabItemGetDescriptionId,
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
        if (MH_CreateHook(symbols.Tile.pStoneSlabItemGetIcon,
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
        if (MH_CreateHook(symbols.Entity.pPlayerCanDestroy,
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
        if (MH_CreateHook(symbols.Entity.pServerPlayerGameModeUseItem,
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
        if (MH_CreateHook(symbols.Entity.pMultiPlayerGameModeUseItem,
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
        if (MH_CreateHook(symbols.Texture.pTexturesBindTextureResource,
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
        if (MH_CreateHook(symbols.Texture.pTexturesLoadTextureByName,
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
        if (MH_CreateHook(symbols.Texture.pTexturesLoadTextureByIndex,
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
        if (MH_CreateHook(symbols.Texture.pTexturesReadImage,
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
        if (MH_CreateHook(symbols.Texture.pTextureManagerCreateTexture,
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
        if (MH_CreateHook(symbols.Texture.pTextureTransferFromImage,
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
        if (MH_CreateHook(symbols.Resource.pAbstractTexturePackGetImageResource,
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
        if (MH_CreateHook(symbols.Resource.pDLCTexturePackGetImageResource,
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
        if (MH_CreateHook(symbols.Texture.pStitchedGetU0,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetU0),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetU0)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getU0");
        }
    }
    if (symbols.Texture.pStitchedGetU1)
    {
        if (MH_CreateHook(symbols.Texture.pStitchedGetU1,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetU1),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetU1)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getU1");
        }
    }
    if (symbols.Texture.pStitchedGetV0)
    {
        if (MH_CreateHook(symbols.Texture.pStitchedGetV0,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetV0),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetV0)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getV0");
        }
    }
    if (symbols.Texture.pStitchedGetV1)
    {
        if (MH_CreateHook(symbols.Texture.pStitchedGetV1,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetV1),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetV1)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getV1");
        }
    }

    if (symbols.Core.pExitGame)
    {
        if (MH_CreateHook(symbols.Core.pExitGame,
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
        if (MH_CreateHook(symbols.Entity.pLivingEntityPick,
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
        if (MH_CreateHook(symbols.Texture.pPreStitchedTextureMapStitch,
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
        if (MH_CreateHook(symbols.Texture.pLoadUVs,
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
            if (MH_CreateHook(symbols.Texture.pRegisterIcon,
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

        if (MH_CreateHook(symbols.Ui.pCreativeStaticCtor,
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
        if (MH_CreateHook(symbols.Ui.pMainMenuCustomDraw,
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

        if (MH_CreateHook(symbols.Core.pPresent,
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

        if (MH_CreateHook(symbols.Ui.pGetString,
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
        if (MH_CreateHook(symbols.Resource.pGetResourceAsStream,
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
            if (MH_CreateHook(pOutputDbgStr,
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
