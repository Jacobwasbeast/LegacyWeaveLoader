#include "SymbolResolver.h"
#include "PdbParser.h"
#include "LogUtil.h"
#include <cstdio>
#include <cstring>
#include <string>

static const char* SYM_RUN_STATIC_CTORS     = "?MinecraftWorld_RunStaticCtors@@YAXXZ";
static const char* SYM_MINECRAFT_TICK       = "?tick@Minecraft@@QEAAX_N0@Z";
static const char* SYM_MINECRAFT_INIT       = "?init@Minecraft@@QEAAXXZ";
static const char* SYM_EXIT_GAME            = "?ExitGame@CConsoleMinecraftApp@@UEAAXXZ";
static const char* SYM_CREATIVE_STATIC_CTOR = "?staticCtor@IUIScene_CreativeMenu@@SAXXZ";
static const char* SYM_MAINMENU_CUSTOMDRAW  = "?customDraw@UIScene_MainMenu@@UEAAXPEAUIggyCustomDrawCallbackRegion@@@Z";
static const char* SYM_PRESENT              = "?Present@C4JRender@@QEAAXXZ";
static const char* SYM_GET_STRING           = "?GetString@CMinecraftApp@@SAPEB_WH@Z";
static const char* SYM_GET_RESOURCE_AS_STREAM = "?getResourceAsStream@InputStream@@SAPEAV1@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z";
static const char* SYM_LOAD_UVS = "?loadUVs@PreStitchedTextureMap@@AEAAXXZ";
static const char* SYM_SIMPLE_ICON_CTOR = "??0SimpleIcon@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@0MMMM@Z";
static const char* SYM_OPERATOR_NEW = "??2@YAPEAX_K@Z";
static const char* SYM_REGISTER_ICON = "?registerIcon@PreStitchedTextureMap@@UEAAPEAVIcon@@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z";
static const char* SYM_ITEMINSTANCE_GETICON = "?getIcon@ItemInstance@@QEAAPEAVIcon@@XZ";
static const char* SYM_ENTITYRENDERER_BINDTEXTURE_RESOURCE = "?bindTexture@EntityRenderer@@MEAAXPEAVResourceLocation@@@Z";
static const char* SYM_ITEMRENDERER_RENDERITEMBILLBOARD = "?renderItemBillboard@ItemRenderer@@EEAAXV?$shared_ptr@VItemEntity@@@std@@PEAVIcon@@HMMMM@Z";
static const char* SYM_COMPASS_CYCLEFRAMES = "?cycleFrames@CompassTexture@@UEAAXXZ";
static const char* SYM_CLOCK_CYCLEFRAMES = "?cycleFrames@ClockTexture@@UEAAXXZ";
static const char* SYM_COMPASS_GETSOURCEWIDTH = "?getSourceWidth@CompassTexture@@UEBAHXZ";
static const char* SYM_COMPASS_GETSOURCEHEIGHT = "?getSourceHeight@CompassTexture@@UEBAHXZ";
static const char* SYM_CLOCK_GETSOURCEWIDTH = "?getSourceWidth@ClockTexture@@UEBAHXZ";
static const char* SYM_CLOCK_GETSOURCEHEIGHT = "?getSourceHeight@ClockTexture@@UEBAHXZ";
static const char* SYM_ITEMINSTANCE_MINEBLOCK = "?mineBlock@ItemInstance@@QEAAXPEAVLevel@@HHHHV?$shared_ptr@VPlayer@@@std@@@Z";
static const char* SYM_ITEMINSTANCE_SAVE = "?save@ItemInstance@@QEAAPEAVCompoundTag@@PEAV2@@Z";
static const char* SYM_ITEMINSTANCE_LOAD = "?load@ItemInstance@@QEAAXPEAVCompoundTag@@@Z";
static const char* SYM_TAG_NEWTAG = "?newTag@Tag@@SAPEAV1@EAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z";
static const char* SYM_ITEM_MINEBLOCK = "?mineBlock@Item@@UEAA_NV?$shared_ptr@VItemInstance@@@std@@PEAVLevel@@HHHHV?$shared_ptr@VLivingEntity@@@3@@Z";
static const char* SYM_DIGGERITEM_MINEBLOCK = "?mineBlock@DiggerItem@@UEAA_NV?$shared_ptr@VItemInstance@@@std@@PEAVLevel@@HHHHV?$shared_ptr@VLivingEntity@@@3@@Z";
static const char* SYM_PICKAXEITEM_GETDESTROYSPEED = "?getDestroySpeed@PickaxeItem@@UEAAMV?$shared_ptr@VItemInstance@@@std@@PEAVTile@@@Z";
static const char* SYM_PICKAXEITEM_CANDESTROYSPECIAL = "?canDestroySpecial@PickaxeItem@@UEAA_NPEAVTile@@@Z";
static const char* SYM_SHOVELITEM_GETDESTROYSPEED = "?getDestroySpeed@ShovelItem@@UEAAMV?$shared_ptr@VItemInstance@@@std@@PEAVTile@@@Z";
static const char* SYM_SHOVELITEM_CANDESTROYSPECIAL = "?canDestroySpecial@ShovelItem@@UEAA_NPEAVTile@@@Z";
static const char* SYM_TILE_ONPLACE = "?onPlace@Tile@@UEAAXPEAVLevel@@HHH@Z";
static const char* SYM_TILE_NEIGHBORCHANGED = "?neighborChanged@Tile@@UEAAXPEAVLevel@@HHHH@Z";
static const char* SYM_TILE_TICK = "?tick@Tile@@UEAAXPEAVLevel@@HHHPEAVRandom@@@Z";
static const char* SYM_LEVEL_UPDATE_NEIGHBORS_AT = "?updateNeighborsAt@Level@@QEAAXHHHH@Z";
static const char* SYM_SERVERLEVEL_TICKPENDINGTICKS = "?tickPendingTicks@ServerLevel@@UEAA_N_N@Z";
static const char* SYM_LEVEL_GETTILE = "?getTile@Level@@UEAAHHHH@Z";
static const char* SYM_LEVEL_SETDATA = "?setData@Level@@UEAA_NHHHHH_N@Z";
static const char* SYM_MCREGIONCHUNKSTORAGE_LOAD = "?load@McRegionChunkStorage@@UEAAPEAVLevelChunk@@PEAVLevel@@HH@Z";
static const char* SYM_MCREGIONCHUNKSTORAGE_SAVE = "?save@McRegionChunkStorage@@UEAAXPEAVLevel@@PEAVLevelChunk@@@Z";
static const char* SYM_TILE_GETRESOURCE = "?getResource@Tile@@UEAAHHPEAVRandom@@H@Z";
static const char* SYM_TILE_CLONETILEID = "?cloneTileId@Tile@@UEAAHPEAVLevel@@HHH@Z";
static const char* SYM_TILE_GETTEXTURE_FACEDATA = "?getTexture@Tile@@UEAAPEAVIcon@@HH@Z";
static const char* SYM_STONESLAB_GETTEXTURE = "?getTexture@StoneSlabTile@@UEAAPEAVIcon@@HH@Z";
static const char* SYM_WOODSLAB_GETTEXTURE = "?getTexture@WoodSlabTile@@UEAAPEAVIcon@@HH@Z";
static const char* SYM_STONESLAB_GETRESOURCE = "?getResource@StoneSlabTile@@UEAAHHPEAVRandom@@H@Z";
static const char* SYM_WOODSLAB_GETRESOURCE = "?getResource@WoodSlabTile@@UEAAHHPEAVRandom@@H@Z";
static const char* SYM_STONESLAB_GETDESCRIPTIONID = "?getDescriptionId@StoneSlabTile@@UEAAIH@Z";
static const char* SYM_WOODSLAB_GETDESCRIPTIONID = "?getDescriptionId@WoodSlabTile@@UEAAIH@Z";
static const char* SYM_STONESLAB_GETAUXNAME = "?getAuxName@StoneSlabTile@@UEAAHH@Z";
static const char* SYM_WOODSLAB_GETAUXNAME = "?getAuxName@WoodSlabTile@@UEAAHH@Z";
static const char* SYM_STONESLAB_REGISTERICONS = "?registerIcons@StoneSlabTile@@UEAAXPEAVIconRegister@@@Z";
static const char* SYM_WOODSLAB_REGISTERICONS = "?registerIcons@WoodSlabTile@@UEAAXPEAVIconRegister@@@Z";
static const char* SYM_STONESLABITEM_GETICON = "?getIcon@StoneSlabTileItem@@UEAAPEAVIcon@@H@Z";
static const char* SYM_STONESLABITEM_GETDESCRIPTIONID = "?getDescriptionId@StoneSlabTileItem@@UEAAIV?$shared_ptr@VItemInstance@@@std@@@Z";
static const char* SYM_HALFSLAB_CLONETILEID = "?cloneTileId@HalfSlabTile@@UEAAHPEAVLevel@@HHH@Z";
static const char* SYM_PLAYER_CANDESTROY = "?canDestroy@Player@@QEAA_NPEAVTile@@@Z";
static const char* SYM_SERVER_PLAYER_GAMEMODE_USEITEM = "?useItem@ServerPlayerGameMode@@QEAA_NV?$shared_ptr@VPlayer@@@std@@PEAVLevel@@V?$shared_ptr@VItemInstance@@@3@_N@Z";
static const char* SYM_MULTI_PLAYER_GAMEMODE_USEITEM = "?useItem@MultiPlayerGameMode@@UEAA_NV?$shared_ptr@VPlayer@@@std@@PEAVLevel@@V?$shared_ptr@VItemInstance@@@3@_N@Z";
static const char* SYM_TEXTURES_BIND_RESOURCE = "?bindTexture@Textures@@QEAAXPEAVResourceLocation@@@Z";
static const char* SYM_TEXTURES_LOAD_BY_NAME = "?loadTexture@Textures@@AEAAHW4_TEXTURE_NAME@@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z";
static const char* SYM_TEXTURES_LOAD_BY_INDEX_PUBLIC = "?loadTexture@Textures@@QEAAHH@Z";
static const char* SYM_TEXTURES_LOAD_BY_INDEX_PRIVATE = "?loadTexture@Textures@@AEAAHH@Z";
static const char* SYM_STITCHED_GETU0 = "?getU0@StitchedTexture@@UEBAM_N@Z";
static const char* SYM_STITCHED_GETU1 = "?getU1@StitchedTexture@@UEBAM_N@Z";
static const char* SYM_STITCHED_GETV0 = "?getV0@StitchedTexture@@UEBAM_N@Z";
static const char* SYM_STITCHED_GETV1 = "?getV1@StitchedTexture@@UEBAM_N@Z";
static const char* SYM_MINECRAFT_SETLEVEL = "?setLevel@Minecraft@@QEAAXPEAVMultiPlayerLevel@@HV?$shared_ptr@VPlayer@@@std@@_N2@Z";
static const char* SYM_LEVEL_ADDENTITY = "?addEntity@Level@@UEAA_NV?$shared_ptr@VEntity@@@std@@@Z";
static const char* SYM_ENTITYIO_NEWBYID = "?newById@EntityIO@@SA?AV?$shared_ptr@VEntity@@@std@@HPEAVLevel@@@Z";
static const char* SYM_ENTITY_MOVETO = "?moveTo@Entity@@QEAAXNNNMM@Z";
static const char* SYM_ENTITY_SETPOS = "?setPos@Entity@@QEAAXNNN@Z";
static const char* SYM_LIVINGENTITY_GETLOOKANGLE = "?getLookAngle@LivingEntity@@UEAAPEAVVec3@@XZ";
static const char* SYM_LIVINGENTITY_GETVIEWVECTOR = "?getViewVector@LivingEntity@@UEAAPEAVVec3@@M@Z";
static const char* SYM_ENTITY_GETLOOKANGLE = "?getLookAngle@Entity@@UEAAPEAVVec3@@XZ";
static const char* SYM_ENTITY_LERPMOTION = "?lerpMotion@Entity@@UEAAXNNN@Z";
static const char* SYM_INVENTORY_REMOVERESOURCE = "?removeResource@Inventory@@QEAA_NH@Z";
static const char* SYM_INVENTORY_VFTABLE = "??_7Inventory@@6B@";
static const char* SYM_ITEMINSTANCE_HURTANDBREAK = "?hurtAndBreak@ItemInstance@@QEAAXHV?$shared_ptr@VLivingEntity@@@std@@@Z";
static const char* SYM_ABSTRACTCONTAINERMENU_BROADCASTCHANGES = "?broadcastChanges@AbstractContainerMenu@@UEAAXXZ";
static const char* SYM_TEXATLAS_BLOCKS = "?LOCATION_BLOCKS@TextureAtlas@@2VResourceLocation@@A";
static const char* SYM_TEXATLAS_ITEMS = "?LOCATION_ITEMS@TextureAtlas@@2VResourceLocation@@A";
static const char* SYM_TILE_TILES = "?tiles@Tile@@2PEAPEAV1@EA";
static const char* SYM_LEVEL_HASNEIGHBORSIGNAL = "?hasNeighborSignal@Level@@QEAA_NHHH@Z";
static const char* SYM_LEVEL_SETTILEANDDATA = "?setTileAndData@Level@@UEAA_NHHHHHH@Z";
static const char* SYM_LEVEL_ADDTOTICKNEXTTICK = "?addToTickNextTick@Level@@UEAAXHHHHH@Z";
static const char* SYM_SERVERLEVEL_ADDTOTICKNEXTTICK = "?addToTickNextTick@ServerLevel@@UEAAXHHHHH@Z";

static void* ResolveExactProcName(uintptr_t moduleBase, const char* exactName)
{
    uint32_t rva = PdbParser::FindSymbolRVAByName(exactName);
    if (rva == 0)
        return nullptr;
    return reinterpret_cast<void*>(moduleBase + rva);
}

static bool IsStub31000(uintptr_t moduleBase, void* ptr)
{
    return ptr == reinterpret_cast<void*>(moduleBase + 0x31000u);
}

bool SymbolResolver::Initialize()
{
    m_moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    if (!m_moduleBase)
    {
        LogUtil::Log("[WeaveLoader] Failed to get module base address");
        return false;
    }

    // Derive PDB path from executable path: replace .exe with .pdb
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string pdbPath(exePath);
    size_t dotPos = pdbPath.rfind('.');
    if (dotPos != std::string::npos)
        pdbPath = pdbPath.substr(0, dotPos) + ".pdb";
    else
        pdbPath += ".pdb";

    LogUtil::Log("[WeaveLoader] PDB path: %s", pdbPath.c_str());
    LogUtil::Log("[WeaveLoader] Module base: %p", reinterpret_cast<void*>(m_moduleBase));

    if (!PdbParser::Open(pdbPath.c_str()))
    {
        LogUtil::Log("[WeaveLoader] ERROR: Failed to open PDB file");
        return false;
    }

    m_initialized = true;
    return true;
}

void* SymbolResolver::Resolve(const char* decoratedName)
{
    if (!m_initialized) return nullptr;

    uint32_t rva = PdbParser::FindSymbolRVA(decoratedName);
    if (rva == 0)
    {
        LogUtil::Log("[WeaveLoader] Symbol not found in PDB: '%s'", decoratedName);
        return nullptr;
    }

    return reinterpret_cast<void*>(m_moduleBase + rva);
}

bool SymbolResolver::ResolveGameFunctions()
{
    LogUtil::Log("[WeaveLoader] Resolving game functions via raw PDB parser...");

    pRunStaticCtors     = Resolve(SYM_RUN_STATIC_CTORS);
    pMinecraftTick      = Resolve(SYM_MINECRAFT_TICK);
    pMinecraftInit      = Resolve(SYM_MINECRAFT_INIT);
    pExitGame           = Resolve(SYM_EXIT_GAME);
    pCreativeStaticCtor = Resolve(SYM_CREATIVE_STATIC_CTOR);
    pMainMenuCustomDraw = Resolve(SYM_MAINMENU_CUSTOMDRAW);
    pPresent            = Resolve(SYM_PRESENT);
    pGetString          = Resolve(SYM_GET_STRING);
    if (!pGetString)
    {
        pGetString = Resolve("?GetString@CConsoleMinecraftApp@@SAPEB_WH@Z");
        if (!pGetString)
            PdbParser::DumpMatching("GetString");
    }
    pGetResourceAsStream = Resolve(SYM_GET_RESOURCE_AS_STREAM);
    pLoadUVs             = Resolve(SYM_LOAD_UVS);
    pSimpleIconCtor      = Resolve(SYM_SIMPLE_ICON_CTOR);
    pOperatorNew         = Resolve(SYM_OPERATOR_NEW);
    pRegisterIcon        = Resolve(SYM_REGISTER_ICON);
    pItemInstanceGetIcon = Resolve(SYM_ITEMINSTANCE_GETICON);
    pEntityRendererBindTextureResource = Resolve(SYM_ENTITYRENDERER_BINDTEXTURE_RESOURCE);
    pItemRendererRenderItemBillboard = Resolve(SYM_ITEMRENDERER_RENDERITEMBILLBOARD);
    pCompassTextureCycleFrames = Resolve(SYM_COMPASS_CYCLEFRAMES);
    pClockTextureCycleFrames = Resolve(SYM_CLOCK_CYCLEFRAMES);
    pCompassTextureGetSourceWidth = Resolve(SYM_COMPASS_GETSOURCEWIDTH);
    pCompassTextureGetSourceHeight = Resolve(SYM_COMPASS_GETSOURCEHEIGHT);
    pClockTextureGetSourceWidth = Resolve(SYM_CLOCK_GETSOURCEWIDTH);
    pClockTextureGetSourceHeight = Resolve(SYM_CLOCK_GETSOURCEHEIGHT);
    pItemInstanceMineBlock = Resolve(SYM_ITEMINSTANCE_MINEBLOCK);
    pItemInstanceSave = Resolve(SYM_ITEMINSTANCE_SAVE);
    pItemInstanceLoad = Resolve(SYM_ITEMINSTANCE_LOAD);
    pTagNewTag = Resolve(SYM_TAG_NEWTAG);
    if (!pTagNewTag)
        pTagNewTag = ResolveExactProcName(m_moduleBase, "Tag::newTag");
    pItemMineBlock = Resolve(SYM_ITEM_MINEBLOCK);
    pDiggerItemMineBlock = Resolve(SYM_DIGGERITEM_MINEBLOCK);
    pPickaxeItemGetDestroySpeed = Resolve(SYM_PICKAXEITEM_GETDESTROYSPEED);
    pPickaxeItemCanDestroySpecial = Resolve(SYM_PICKAXEITEM_CANDESTROYSPECIAL);
    pShovelItemGetDestroySpeed = Resolve(SYM_SHOVELITEM_GETDESTROYSPEED);
    pShovelItemCanDestroySpecial = Resolve(SYM_SHOVELITEM_CANDESTROYSPECIAL);
    pTileOnPlace = Resolve(SYM_TILE_ONPLACE);
    pTileNeighborChanged = Resolve(SYM_TILE_NEIGHBORCHANGED);
    pTileTick = Resolve(SYM_TILE_TICK);
    pLevelUpdateNeighborsAt = Resolve(SYM_LEVEL_UPDATE_NEIGHBORS_AT);
    pServerLevelTickPendingTicks = Resolve(SYM_SERVERLEVEL_TICKPENDINGTICKS);
    pLevelGetTile = Resolve(SYM_LEVEL_GETTILE);
    pLevelSetData = Resolve(SYM_LEVEL_SETDATA);
    pMcRegionChunkStorageLoad = Resolve(SYM_MCREGIONCHUNKSTORAGE_LOAD);
    if (!pMcRegionChunkStorageLoad)
        pMcRegionChunkStorageLoad = ResolveExactProcName(m_moduleBase, "McRegionChunkStorage::load");
    pMcRegionChunkStorageSave = Resolve(SYM_MCREGIONCHUNKSTORAGE_SAVE);
    if (!pMcRegionChunkStorageSave)
        pMcRegionChunkStorageSave = ResolveExactProcName(m_moduleBase, "McRegionChunkStorage::save");
    pTileGetResource = Resolve(SYM_TILE_GETRESOURCE);
    pTileCloneTileId = Resolve(SYM_TILE_CLONETILEID);
    pTileGetTextureFaceData = Resolve(SYM_TILE_GETTEXTURE_FACEDATA);
    pStoneSlabGetTexture = Resolve(SYM_STONESLAB_GETTEXTURE);
    pWoodSlabGetTexture = Resolve(SYM_WOODSLAB_GETTEXTURE);
    pStoneSlabGetResource = Resolve(SYM_STONESLAB_GETRESOURCE);
    pWoodSlabGetResource = Resolve(SYM_WOODSLAB_GETRESOURCE);
    pStoneSlabGetDescriptionId = Resolve(SYM_STONESLAB_GETDESCRIPTIONID);
    pWoodSlabGetDescriptionId = Resolve(SYM_WOODSLAB_GETDESCRIPTIONID);
    pStoneSlabGetAuxName = Resolve(SYM_STONESLAB_GETAUXNAME);
    pWoodSlabGetAuxName = Resolve(SYM_WOODSLAB_GETAUXNAME);
    pStoneSlabRegisterIcons = Resolve(SYM_STONESLAB_REGISTERICONS);
    pWoodSlabRegisterIcons = Resolve(SYM_WOODSLAB_REGISTERICONS);
    pStoneSlabItemGetIcon = Resolve(SYM_STONESLABITEM_GETICON);
    pStoneSlabItemGetDescriptionId = Resolve(SYM_STONESLABITEM_GETDESCRIPTIONID);
    pHalfSlabCloneTileId = Resolve(SYM_HALFSLAB_CLONETILEID);
    pPlayerCanDestroy = Resolve(SYM_PLAYER_CANDESTROY);
    pServerPlayerGameModeUseItem = Resolve(SYM_SERVER_PLAYER_GAMEMODE_USEITEM);
    pMultiPlayerGameModeUseItem = Resolve(SYM_MULTI_PLAYER_GAMEMODE_USEITEM);
    pTexturesBindTextureResource = Resolve(SYM_TEXTURES_BIND_RESOURCE);
    pTexturesLoadTextureByName = Resolve(SYM_TEXTURES_LOAD_BY_NAME);
    pTexturesLoadTextureByIndex = Resolve(SYM_TEXTURES_LOAD_BY_INDEX_PUBLIC);
    if (!pTexturesLoadTextureByIndex)
        pTexturesLoadTextureByIndex = Resolve(SYM_TEXTURES_LOAD_BY_INDEX_PRIVATE);
    pStitchedGetU0 = Resolve(SYM_STITCHED_GETU0);
    pStitchedGetU1 = Resolve(SYM_STITCHED_GETU1);
    pStitchedGetV0 = Resolve(SYM_STITCHED_GETV0);
    pStitchedGetV1 = Resolve(SYM_STITCHED_GETV1);
    pMinecraftSetLevel = Resolve(SYM_MINECRAFT_SETLEVEL);
    pLevelAddEntity = Resolve(SYM_LEVEL_ADDENTITY);
    pEntityIONewById = Resolve(SYM_ENTITYIO_NEWBYID);
    pEntityMoveTo = Resolve(SYM_ENTITY_MOVETO);
    pEntitySetPos = Resolve(SYM_ENTITY_SETPOS);
    pEntityGetLookAngle = Resolve(SYM_LIVINGENTITY_GETLOOKANGLE);
    pLivingEntityGetViewVector = Resolve(SYM_LIVINGENTITY_GETVIEWVECTOR);
    if (!pEntityGetLookAngle)
        pEntityGetLookAngle = Resolve(SYM_ENTITY_GETLOOKANGLE);
    pEntityLerpMotion = Resolve(SYM_ENTITY_LERPMOTION);
    pInventoryRemoveResource = Resolve(SYM_INVENTORY_REMOVERESOURCE);
    pInventoryVtable = Resolve(SYM_INVENTORY_VFTABLE);
    pItemInstanceHurtAndBreak = Resolve(SYM_ITEMINSTANCE_HURTANDBREAK);
    pAbstractContainerMenuBroadcastChanges = Resolve(SYM_ABSTRACTCONTAINERMENU_BROADCASTCHANGES);
    pTextureAtlasLocationBlocks = Resolve(SYM_TEXATLAS_BLOCKS);
    pTextureAtlasLocationItems = Resolve(SYM_TEXATLAS_ITEMS);
    pTileTiles = Resolve(SYM_TILE_TILES);
    pLevelHasNeighborSignal = Resolve(SYM_LEVEL_HASNEIGHBORSIGNAL);
    pLevelSetTileAndData = Resolve(SYM_LEVEL_SETTILEANDDATA);
    pLevelAddToTickNextTick = Resolve(SYM_LEVEL_ADDTOTICKNEXTTICK);
    pServerLevelAddToTickNextTick = Resolve(SYM_SERVERLEVEL_ADDTOTICKNEXTTICK);
    pLevelChunkGetTile = ResolveExactProcName(m_moduleBase, "LevelChunk::getTile");
    pLevelChunkSetTile = ResolveExactProcName(m_moduleBase, "LevelChunk::setTile");
    pLevelChunkGetPos = ResolveExactProcName(m_moduleBase, "LevelChunk::getPos");
    pLevelChunkGetHighestNonEmptyY = ResolveExactProcName(m_moduleBase, "LevelChunk::getHighestNonEmptyY");
    pCompressedTileStorageSet = ResolveExactProcName(m_moduleBase, "CompressedTileStorage::set");

    // Some public symbols in this build resolve to stub bodies. Prefer exact
    // module procedure names from the PDB where those exist.
    if (pShovelItemGetDestroySpeed == nullptr)
        pShovelItemGetDestroySpeed = ResolveExactProcName(m_moduleBase, "DiggerItem::getDestroySpeed");
    if (IsStub31000(m_moduleBase, pTileOnPlace))
        pTileOnPlace = ResolveExactProcName(m_moduleBase, "Tile::onPlace");
    if (IsStub31000(m_moduleBase, pTileNeighborChanged))
        pTileNeighborChanged = ResolveExactProcName(m_moduleBase, "Tile::neighborChanged");
    if (IsStub31000(m_moduleBase, pTileTick))
        pTileTick = ResolveExactProcName(m_moduleBase, "Tile::tick");
    if (IsStub31000(m_moduleBase, pWoodSlabRegisterIcons))
        pWoodSlabRegisterIcons = ResolveExactProcName(m_moduleBase, "WoodSlabTile::registerIcons");

    if (!pOperatorNew)   pOperatorNew = GetProcAddress(GetModuleHandleA("vcruntime140.dll"), SYM_OPERATOR_NEW);
    if (!pOperatorNew)   pOperatorNew = GetProcAddress(GetModuleHandleA("vcruntime140d.dll"), SYM_OPERATOR_NEW);
    if (!pOperatorNew)   pOperatorNew = GetProcAddress(GetModuleHandle(nullptr), SYM_OPERATOR_NEW);
    if (!pSimpleIconCtor) PdbParser::DumpMatching("??0SimpleIcon@@");
    if (!pLoadUVs)        PdbParser::DumpMatching("loadUVs@PreStitchedTextureMap");

    auto logSym = [](const char* name, void* ptr) {
        if (ptr)
            LogUtil::Log("[WeaveLoader] %-25s @ %p", name, ptr);
        else
            LogUtil::Log("[WeaveLoader] MISSING: %s", name);
    };

    logSym("RunStaticCtors",     pRunStaticCtors);
    logSym("Minecraft::tick",    pMinecraftTick);
    logSym("Minecraft::init",    pMinecraftInit);
    logSym("ExitGame",           pExitGame);
    logSym("CreativeStaticCtor", pCreativeStaticCtor);
    logSym("MainMenuCustomDraw", pMainMenuCustomDraw);
    logSym("C4JRender::Present", pPresent);
    logSym("CMinecraftApp::GetString", pGetString);
    logSym("InputStream::getResourceAsStream", pGetResourceAsStream);
    logSym("PreStitchedTextureMap::loadUVs", pLoadUVs);
    logSym("SimpleIcon::SimpleIcon", pSimpleIconCtor);
    logSym("operator new", pOperatorNew);
    logSym("registerIcon", pRegisterIcon);
    logSym("ItemInstance::getIcon", pItemInstanceGetIcon);
    logSym("EntityRenderer::bindTexture(ResourceLocation)", pEntityRendererBindTextureResource);
    logSym("ItemRenderer::renderItemBillboard", pItemRendererRenderItemBillboard);
    logSym("CompassTexture::cycleFrames", pCompassTextureCycleFrames);
    logSym("ClockTexture::cycleFrames", pClockTextureCycleFrames);
    logSym("CompassTexture::getSourceWidth", pCompassTextureGetSourceWidth);
    logSym("CompassTexture::getSourceHeight", pCompassTextureGetSourceHeight);
    logSym("ClockTexture::getSourceWidth", pClockTextureGetSourceWidth);
    logSym("ClockTexture::getSourceHeight", pClockTextureGetSourceHeight);
    logSym("ItemInstance::mineBlock", pItemInstanceMineBlock);
    logSym("ItemInstance::save", pItemInstanceSave);
    logSym("ItemInstance::load", pItemInstanceLoad);
    logSym("Tag::newTag", pTagNewTag);
    logSym("Item::mineBlock", pItemMineBlock);
    logSym("DiggerItem::mineBlock", pDiggerItemMineBlock);
    logSym("PickaxeItem::getDestroySpeed", pPickaxeItemGetDestroySpeed);
    logSym("PickaxeItem::canDestroySpecial", pPickaxeItemCanDestroySpecial);
    logSym("ShovelItem::getDestroySpeed", pShovelItemGetDestroySpeed);
    logSym("ShovelItem::canDestroySpecial", pShovelItemCanDestroySpecial);
    logSym("Tile::onPlace", pTileOnPlace);
    logSym("Tile::neighborChanged", pTileNeighborChanged);
    logSym("Tile::tick", pTileTick);
    logSym("Level::updateNeighborsAt", pLevelUpdateNeighborsAt);
    logSym("ServerLevel::tickPendingTicks", pServerLevelTickPendingTicks);
    logSym("Level::getTile", pLevelGetTile);
    logSym("Level::setData", pLevelSetData);
    logSym("McRegionChunkStorage::load", pMcRegionChunkStorageLoad);
    logSym("McRegionChunkStorage::save", pMcRegionChunkStorageSave);
    logSym("Tile::getResource", pTileGetResource);
    logSym("Tile::cloneTileId", pTileCloneTileId);
    logSym("Tile::getTexture(face,data)", pTileGetTextureFaceData);
    logSym("StoneSlabTile::getTexture", pStoneSlabGetTexture);
    logSym("WoodSlabTile::getTexture", pWoodSlabGetTexture);
    logSym("StoneSlabTile::getResource", pStoneSlabGetResource);
    logSym("WoodSlabTile::getResource", pWoodSlabGetResource);
    logSym("StoneSlabTile::getDescriptionId", pStoneSlabGetDescriptionId);
    logSym("WoodSlabTile::getDescriptionId", pWoodSlabGetDescriptionId);
    logSym("StoneSlabTile::getAuxName", pStoneSlabGetAuxName);
    logSym("WoodSlabTile::getAuxName", pWoodSlabGetAuxName);
    logSym("StoneSlabTile::registerIcons", pStoneSlabRegisterIcons);
    logSym("WoodSlabTile::registerIcons", pWoodSlabRegisterIcons);
    logSym("StoneSlabTileItem::getIcon", pStoneSlabItemGetIcon);
    logSym("StoneSlabTileItem::getDescriptionId", pStoneSlabItemGetDescriptionId);
    logSym("HalfSlabTile::cloneTileId", pHalfSlabCloneTileId);
    logSym("Player::canDestroy", pPlayerCanDestroy);
    logSym("ServerPlayerGameMode::useItem", pServerPlayerGameModeUseItem);
    logSym("MultiPlayerGameMode::useItem", pMultiPlayerGameModeUseItem);
    logSym("Textures::bindTexture(ResourceLocation)", pTexturesBindTextureResource);
    logSym("Textures::loadTexture(TEXTURE_NAME,wstring)", pTexturesLoadTextureByName);
    logSym("Textures::loadTexture(int)", pTexturesLoadTextureByIndex);
    logSym("StitchedTexture::getU0", pStitchedGetU0);
    logSym("StitchedTexture::getU1", pStitchedGetU1);
    logSym("StitchedTexture::getV0", pStitchedGetV0);
    logSym("StitchedTexture::getV1", pStitchedGetV1);
    logSym("Minecraft::setLevel", pMinecraftSetLevel);
    logSym("Level::addEntity", pLevelAddEntity);
    logSym("EntityIO::newById", pEntityIONewById);
    logSym("Entity::moveTo", pEntityMoveTo);
    logSym("Entity::setPos", pEntitySetPos);
    logSym("LivingEntity/Entity::getLookAngle", pEntityGetLookAngle);
    logSym("LivingEntity::getViewVector", pLivingEntityGetViewVector);
    logSym("Entity::lerpMotion", pEntityLerpMotion);
    logSym("Inventory::removeResource", pInventoryRemoveResource);
    logSym("Inventory::vftable", pInventoryVtable);
    logSym("ItemInstance::hurtAndBreak", pItemInstanceHurtAndBreak);
    logSym("AbstractContainerMenu::broadcastChanges", pAbstractContainerMenuBroadcastChanges);
    logSym("TextureAtlas::LOCATION_BLOCKS", pTextureAtlasLocationBlocks);
    logSym("TextureAtlas::LOCATION_ITEMS", pTextureAtlasLocationItems);
    logSym("Tile::tiles", pTileTiles);
    logSym("Level::hasNeighborSignal", pLevelHasNeighborSignal);
    logSym("Level::setTileAndData", pLevelSetTileAndData);
    logSym("Level::addToTickNextTick", pLevelAddToTickNextTick);
    logSym("ServerLevel::addToTickNextTick", pServerLevelAddToTickNextTick);
    logSym("LevelChunk::getTile", pLevelChunkGetTile);
    logSym("LevelChunk::setTile", pLevelChunkSetTile);
    logSym("LevelChunk::getPos", pLevelChunkGetPos);
    logSym("LevelChunk::getHighestNonEmptyY", pLevelChunkGetHighestNonEmptyY);
    logSym("CompressedTileStorage::set", pCompressedTileStorageSet);

    bool ok = pRunStaticCtors && pMinecraftTick && pMinecraftInit;
    if (ok)
        LogUtil::Log("[WeaveLoader] All critical symbols resolved (via raw PDB parser)");
    else
        LogUtil::Log("[WeaveLoader] CRITICAL symbols missing - hooks will not be installed");

    return ok;
}

void SymbolResolver::Cleanup()
{
    PdbParser::Close();
    m_initialized = false;
}
