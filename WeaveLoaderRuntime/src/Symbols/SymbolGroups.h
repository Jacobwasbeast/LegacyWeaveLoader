#pragma once

class SymbolResolver;

struct CoreSymbols
{
    void* pRunStaticCtors = nullptr;
    void* pMinecraftTick = nullptr;
    void* pMinecraftInit = nullptr;
    void* pMinecraftSetLevel = nullptr;
    void* pExitGame = nullptr;
    void* pPresent = nullptr;
    void* pMinecraftApp = nullptr;
    void* pGetMinecraftLanguage = nullptr;
    void* pGetMinecraftLocale = nullptr;

    bool Resolve(SymbolResolver& resolver);
    void Log() const;
    bool HasCritical() const;
};

struct UiSymbols
{
    void* pCreativeStaticCtor = nullptr;
    void* pMainMenuCustomDraw = nullptr;
    void* pGetString = nullptr;

    bool Resolve(SymbolResolver& resolver);
    void Log() const;
};

struct ResourceSymbols
{
    void* pGetResourceAsStream = nullptr;
    void* pAbstractTexturePackGetImageResource = nullptr;
    void* pDLCTexturePackGetImageResource = nullptr;

    bool Resolve(SymbolResolver& resolver);
    void Log() const;
};

struct TextureSymbols
{
    void* pLoadUVs = nullptr;
    void* pPreStitchedTextureMapStitch = nullptr;
    void* pSimpleIconCtor = nullptr;
    void* pOperatorNew = nullptr;
    void* pRegisterIcon = nullptr;
    void* pEntityRendererBindTextureResource = nullptr;
    void* pItemRendererRenderItemBillboard = nullptr;
    void* pCompassTextureCycleFrames = nullptr;
    void* pClockTextureCycleFrames = nullptr;
    void* pCompassTextureGetSourceWidth = nullptr;
    void* pCompassTextureGetSourceHeight = nullptr;
    void* pClockTextureGetSourceWidth = nullptr;
    void* pClockTextureGetSourceHeight = nullptr;
    void* pTexturesBindTextureResource = nullptr;
    void* pTexturesLoadTextureByName = nullptr;
    void* pTexturesLoadTextureByIndex = nullptr;
    void* pTexturesReadImage = nullptr;
    void* pStitchedGetU0 = nullptr;
    void* pStitchedGetU1 = nullptr;
    void* pStitchedGetV0 = nullptr;
    void* pStitchedGetV1 = nullptr;
    void* pBufferedImageCtorFile = nullptr;
    void* pBufferedImageCtorDLCPack = nullptr;
    void* pTextureManagerCreateTexture = nullptr;
    void* pTextureTransferFromImage = nullptr;
    void* pTextureAtlasLocationBlocks = nullptr;
    void* pTextureAtlasLocationItems = nullptr;

    bool Resolve(SymbolResolver& resolver);
    void Log() const;
};

struct ItemSymbols
{
    void* pItemInstanceGetIcon = nullptr;
    void* pItemInstanceMineBlock = nullptr;
    void* pItemInstanceSave = nullptr;
    void* pItemInstanceLoad = nullptr;
    void* pItemInstanceHurtAndBreak = nullptr;
    void* pItemMineBlock = nullptr;
    void* pDiggerItemMineBlock = nullptr;
    void* pPickaxeItemGetDestroySpeed = nullptr;
    void* pPickaxeItemCanDestroySpecial = nullptr;
    void* pShovelItemGetDestroySpeed = nullptr;
    void* pShovelItemCanDestroySpecial = nullptr;

    bool Resolve(SymbolResolver& resolver);
    void Log() const;
};

struct TileSymbols
{
    void* pTileOnPlace = nullptr;
    void* pTileNeighborChanged = nullptr;
    void* pTileTick = nullptr;
    void* pTileGetResource = nullptr;
    void* pTileCloneTileId = nullptr;
    void* pTileGetTextureFaceData = nullptr;
    void* pStoneSlabGetTexture = nullptr;
    void* pWoodSlabGetTexture = nullptr;
    void* pStoneSlabGetResource = nullptr;
    void* pWoodSlabGetResource = nullptr;
    void* pStoneSlabGetDescriptionId = nullptr;
    void* pWoodSlabGetDescriptionId = nullptr;
    void* pStoneSlabGetAuxName = nullptr;
    void* pWoodSlabGetAuxName = nullptr;
    void* pStoneSlabRegisterIcons = nullptr;
    void* pWoodSlabRegisterIcons = nullptr;
    void* pStoneSlabItemGetIcon = nullptr;
    void* pStoneSlabItemGetDescriptionId = nullptr;
    void* pHalfSlabCloneTileId = nullptr;
    void* pTileTiles = nullptr;

    bool Resolve(SymbolResolver& resolver);
    void Log() const;
};

struct LevelSymbols
{
    void* pLevelUpdateNeighborsAt = nullptr;
    void* pServerLevelTickPendingTicks = nullptr;
    void* pLevelGetTile = nullptr;
    void* pLevelSetData = nullptr;
    void* pMcRegionChunkStorageLoad = nullptr;
    void* pMcRegionChunkStorageSave = nullptr;
    void* pLevelSetTileAndData = nullptr;
    void* pLevelHasNeighborSignal = nullptr;
    void* pLevelAddToTickNextTick = nullptr;
    void* pServerLevelAddToTickNextTick = nullptr;
    void* pLevelChunkGetTile = nullptr;
    void* pLevelChunkSetTile = nullptr;
    void* pLevelChunkGetPos = nullptr;
    void* pLevelChunkGetHighestNonEmptyY = nullptr;
    void* pCompressedTileStorageSet = nullptr;

    bool Resolve(SymbolResolver& resolver);
    void Log() const;
};

struct EntitySymbols
{
    void* pPlayerCanDestroy = nullptr;
    void* pServerPlayerGameModeUseItem = nullptr;
    void* pMultiPlayerGameModeUseItem = nullptr;
    void* pLevelAddEntity = nullptr;
    void* pEntityIONewById = nullptr;
    void* pEntityMoveTo = nullptr;
    void* pEntitySetPos = nullptr;
    void* pEntityGetLookAngle = nullptr;
    void* pLivingEntityGetViewVector = nullptr;
    void* pEntityLerpMotion = nullptr;

    bool Resolve(SymbolResolver& resolver);
    void Log() const;
};

struct InventorySymbols
{
    void* pInventoryRemoveResource = nullptr;
    void* pInventoryVtable = nullptr;
    void* pAbstractContainerMenuBroadcastChanges = nullptr;

    bool Resolve(SymbolResolver& resolver);
    void Log() const;
};

struct NbtSymbols
{
    void* pTagNewTag = nullptr;

    bool Resolve(SymbolResolver& resolver);
    void Log() const;
};
