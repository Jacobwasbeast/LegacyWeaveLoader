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

    WorldIdRemap::SetTagNewTagSymbol(symbols.pTagNewTag);
    WorldIdRemap::SetLevelChunkTileSymbols(
        symbols.pLevelChunkGetTile,
        symbols.pLevelChunkSetTile,
        symbols.pLevelChunkGetPos,
        symbols.pLevelChunkGetHighestNonEmptyY);

    if (symbols.pRunStaticCtors)
    {
        if (MH_CreateHook(symbols.pRunStaticCtors,
                          reinterpret_cast<void*>(&GameHooks::Hooked_RunStaticCtors),
                          reinterpret_cast<void**>(&GameHooks::Original_RunStaticCtors)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Failed to hook RunStaticCtors");
            return false;
        }
        LogUtil::Log("[WeaveLoader] Hooked RunStaticCtors");
    }

    if (symbols.pMinecraftTick)
    {
        if (MH_CreateHook(symbols.pMinecraftTick,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftTick),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftTick)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Failed to hook Minecraft::tick");
            return false;
        }
        LogUtil::Log("[WeaveLoader] Hooked Minecraft::tick");
    }

    if (symbols.pMinecraftInit)
    {
        if (MH_CreateHook(symbols.pMinecraftInit,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftInit),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftInit)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Failed to hook Minecraft::init");
            return false;
        }
        LogUtil::Log("[WeaveLoader] Hooked Minecraft::init");
    }

    if (symbols.pMinecraftSetLevel)
    {
        if (MH_CreateHook(symbols.pMinecraftSetLevel,
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

    if (symbols.pItemInstanceMineBlock)
    {
        if (MH_CreateHook(symbols.pItemInstanceMineBlock,
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

    if (symbols.pItemInstanceSave)
    {
        if (MH_CreateHook(symbols.pItemInstanceSave,
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

    if (symbols.pItemInstanceLoad)
    {
        if (MH_CreateHook(symbols.pItemInstanceLoad,
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

    if (symbols.pItemInstanceGetIcon)
    {
        if (MH_CreateHook(symbols.pItemInstanceGetIcon,
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

    if (symbols.pEntityRendererBindTextureResource)
    {
        if (MH_CreateHook(symbols.pEntityRendererBindTextureResource,
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

    if (symbols.pItemRendererRenderItemBillboard)
    {
        if (MH_CreateHook(symbols.pItemRendererRenderItemBillboard,
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

    if (symbols.pCompassTextureCycleFrames)
    {
        if (MH_CreateHook(symbols.pCompassTextureCycleFrames,
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

    if (symbols.pClockTextureCycleFrames)
    {
        if (MH_CreateHook(symbols.pClockTextureCycleFrames,
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

    if (symbols.pCompassTextureGetSourceWidth)
    {
        if (MH_CreateHook(symbols.pCompassTextureGetSourceWidth,
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

    if (symbols.pCompassTextureGetSourceHeight)
    {
        if (MH_CreateHook(symbols.pCompassTextureGetSourceHeight,
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

    if (symbols.pClockTextureGetSourceWidth)
    {
        if (MH_CreateHook(symbols.pClockTextureGetSourceWidth,
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

    if (symbols.pClockTextureGetSourceHeight)
    {
        if (MH_CreateHook(symbols.pClockTextureGetSourceHeight,
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

    if (symbols.pItemMineBlock)
    {
        if (MH_CreateHook(symbols.pItemMineBlock,
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

    if (symbols.pDiggerItemMineBlock)
    {
        if (MH_CreateHook(symbols.pDiggerItemMineBlock,
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

    if (symbols.pPickaxeItemGetDestroySpeed)
    {
        if (MH_CreateHook(symbols.pPickaxeItemGetDestroySpeed,
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

    if (symbols.pPickaxeItemCanDestroySpecial)
    {
        if (MH_CreateHook(symbols.pPickaxeItemCanDestroySpecial,
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

    if (symbols.pShovelItemGetDestroySpeed)
    {
        if (MH_CreateHook(symbols.pShovelItemGetDestroySpeed,
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

    if (symbols.pShovelItemCanDestroySpecial)
    {
        if (MH_CreateHook(symbols.pShovelItemCanDestroySpecial,
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

    if (symbols.pLevelSetTileAndData)
    {
        if (MH_CreateHook(symbols.pLevelSetTileAndData,
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

    if (symbols.pLevelSetData)
    {
        if (MH_CreateHook(symbols.pLevelSetData,
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

    if (symbols.pLevelUpdateNeighborsAt)
    {
        if (MH_CreateHook(symbols.pLevelUpdateNeighborsAt,
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

    if (symbols.pServerLevelTickPendingTicks)
    {
        if (MH_CreateHook(symbols.pServerLevelTickPendingTicks,
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

    if (symbols.pMcRegionChunkStorageLoad)
    {
        if (MH_CreateHook(symbols.pMcRegionChunkStorageLoad,
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

    if (symbols.pMcRegionChunkStorageSave)
    {
        if (MH_CreateHook(symbols.pMcRegionChunkStorageSave,
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

    if (symbols.pTileGetResource)
    {
        if (MH_CreateHook(symbols.pTileGetResource,
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

    if (symbols.pTileCloneTileId)
    {
        if (MH_CreateHook(symbols.pTileCloneTileId,
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

    if (symbols.pStoneSlabGetTexture)
    {
        if (MH_CreateHook(symbols.pStoneSlabGetTexture,
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

    if (symbols.pWoodSlabGetTexture)
    {
        if (MH_CreateHook(symbols.pWoodSlabGetTexture,
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

    if (symbols.pStoneSlabGetResource)
    {
        if (MH_CreateHook(symbols.pStoneSlabGetResource,
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

    if (symbols.pWoodSlabGetResource)
    {
        if (MH_CreateHook(symbols.pWoodSlabGetResource,
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

    if (symbols.pStoneSlabGetDescriptionId)
    {
        if (MH_CreateHook(symbols.pStoneSlabGetDescriptionId,
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

    if (symbols.pWoodSlabGetDescriptionId)
    {
        if (MH_CreateHook(symbols.pWoodSlabGetDescriptionId,
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

    if (symbols.pStoneSlabGetAuxName)
    {
        if (MH_CreateHook(symbols.pStoneSlabGetAuxName,
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

    if (symbols.pWoodSlabGetAuxName)
    {
        if (MH_CreateHook(symbols.pWoodSlabGetAuxName,
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

    if (symbols.pStoneSlabRegisterIcons)
    {
        if (MH_CreateHook(symbols.pStoneSlabRegisterIcons,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StoneSlabRegisterIcons),
                          reinterpret_cast<void**>(&GameHooks::Original_StoneSlabRegisterIcons)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StoneSlabTile::registerIcons");
        }
        else
        {
            MH_EnableHook(symbols.pStoneSlabRegisterIcons);
            LogUtil::Log("[WeaveLoader] Hooked StoneSlabTile::registerIcons (custom slabs)");
        }
    }

    if (symbols.pWoodSlabRegisterIcons)
    {
        if (MH_CreateHook(symbols.pWoodSlabRegisterIcons,
                          reinterpret_cast<void*>(&GameHooks::Hooked_WoodSlabRegisterIcons),
                          reinterpret_cast<void**>(&GameHooks::Original_WoodSlabRegisterIcons)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook WoodSlabTile::registerIcons");
        }
        else
        {
            MH_EnableHook(symbols.pWoodSlabRegisterIcons);
            LogUtil::Log("[WeaveLoader] Hooked WoodSlabTile::registerIcons (custom slabs)");
        }
    }

    if (symbols.pHalfSlabCloneTileId)
    {
        if (MH_CreateHook(symbols.pHalfSlabCloneTileId,
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

    if (symbols.pStoneSlabItemGetDescriptionId)
    {
        if (MH_CreateHook(symbols.pStoneSlabItemGetDescriptionId,
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

    if (symbols.pStoneSlabItemGetIcon)
    {
        if (MH_CreateHook(symbols.pStoneSlabItemGetIcon,
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

    if (symbols.pPlayerCanDestroy)
    {
        if (MH_CreateHook(symbols.pPlayerCanDestroy,
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

    if (symbols.pServerPlayerGameModeUseItem)
    {
        if (MH_CreateHook(symbols.pServerPlayerGameModeUseItem,
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

    if (symbols.pMultiPlayerGameModeUseItem)
    {
        if (MH_CreateHook(symbols.pMultiPlayerGameModeUseItem,
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

    GameHooks::SetAtlasLocationPointers(symbols.pTextureAtlasLocationBlocks, symbols.pTextureAtlasLocationItems);
    GameHooks::SetTileTilesArray(symbols.pTileTiles);
    GameHooks::SetBlockHelperSymbols(symbols.pTileGetTextureFaceData);
    GameHooks::SetManagedBlockDispatchSymbols(symbols.pLevelGetTile);
    NativeExports::SetLevelInteropSymbols(
        symbols.pLevelHasNeighborSignal,
        symbols.pLevelSetTileAndData,
        symbols.pServerLevelAddToTickNextTick ? symbols.pServerLevelAddToTickNextTick
                                              : symbols.pLevelAddToTickNextTick,
        symbols.pLevelGetTile);

    if (symbols.pTexturesBindTextureResource)
    {
        if (MH_CreateHook(symbols.pTexturesBindTextureResource,
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

    if (symbols.pTexturesLoadTextureByName)
    {
        if (MH_CreateHook(symbols.pTexturesLoadTextureByName,
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

    if (symbols.pTexturesLoadTextureByIndex)
    {
        if (MH_CreateHook(symbols.pTexturesLoadTextureByIndex,
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

    if (symbols.pStitchedGetU0)
    {
        if (MH_CreateHook(symbols.pStitchedGetU0,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetU0),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetU0)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getU0");
        }
    }
    if (symbols.pStitchedGetU1)
    {
        if (MH_CreateHook(symbols.pStitchedGetU1,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetU1),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetU1)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getU1");
        }
    }
    if (symbols.pStitchedGetV0)
    {
        if (MH_CreateHook(symbols.pStitchedGetV0,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetV0),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetV0)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getV0");
        }
    }
    if (symbols.pStitchedGetV1)
    {
        if (MH_CreateHook(symbols.pStitchedGetV1,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetV1),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetV1)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getV1");
        }
    }

    if (symbols.pExitGame)
    {
        if (MH_CreateHook(symbols.pExitGame,
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
        symbols.pLevelAddEntity,
        symbols.pEntityIONewById,
        symbols.pEntityMoveTo,
        symbols.pEntitySetPos);
    GameHooks::SetUseActionSymbols(
        symbols.pInventoryRemoveResource,
        symbols.pInventoryVtable,
        symbols.pItemInstanceHurtAndBreak,
        symbols.pAbstractContainerMenuBroadcastChanges,
        symbols.pEntityGetLookAngle,
        symbols.pLivingEntityGetViewVector,
        symbols.pEntityLerpMotion,
        symbols.pEntitySetPos);

    if (symbols.pLoadUVs && symbols.pSimpleIconCtor && symbols.pOperatorNew)
    {
        ModAtlas::SetInjectSymbols(symbols.pSimpleIconCtor, symbols.pOperatorNew);
        if (MH_CreateHook(symbols.pLoadUVs,
                          reinterpret_cast<void*>(&GameHooks::Hooked_LoadUVs),
                          reinterpret_cast<void**>(&GameHooks::Original_LoadUVs)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook PreStitchedTextureMap::loadUVs (mod textures may not appear)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked PreStitchedTextureMap::loadUVs (mod texture injection)");
        }

        if (symbols.pRegisterIcon)
        {
            if (MH_CreateHook(symbols.pRegisterIcon,
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
    else if (symbols.pLoadUVs)
    {
        LogUtil::Log("[WeaveLoader] Mod texture injection unavailable: SimpleIcon/operator new not resolved");
    }

    if (symbols.pCreativeStaticCtor)
    {
        CreativeInventory::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

        if (MH_CreateHook(symbols.pCreativeStaticCtor,
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

    if (symbols.pMainMenuCustomDraw)
    {
        if (MH_CreateHook(symbols.pMainMenuCustomDraw,
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

    if (symbols.pPresent)
    {
        MainMenuOverlay::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

        if (MH_CreateHook(symbols.pPresent,
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

    if (symbols.pGetString)
    {
        // Read GetString prologue bytes BEFORE MinHook overwrites them.
        ModStrings::CaptureStringTableRef(symbols.pGetString);

        if (MH_CreateHook(symbols.pGetString,
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


    if (symbols.pGetResourceAsStream)
    {
        if (MH_CreateHook(symbols.pGetResourceAsStream,
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
