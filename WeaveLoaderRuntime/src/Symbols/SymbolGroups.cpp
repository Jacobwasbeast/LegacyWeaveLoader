#include "Symbols/SymbolGroups.h"
#include "SymbolResolver.h"
#include "PdbParser.h"
#include "LogUtil.h"
#include <Windows.h>

namespace
{
    static void LogSym(const char* name, void* ptr)
    {
        if (ptr)
            LogUtil::Log("[WeaveLoader] %-25s @ %p", name, ptr);
        else
            LogUtil::Log("[WeaveLoader] MISSING: %s", name);
    }

    static void LogSymOptional(const char* name, void* ptr)
    {
        if (ptr)
            LogUtil::Log("[WeaveLoader] %-25s @ %p", name, ptr);
    }

    static const char* SYM_RUN_STATIC_CTORS = "?MinecraftWorld_RunStaticCtors@@YAXXZ";
    static const char* SYM_MINECRAFT_TICK = "?tick@Minecraft@@QEAAX_N0@Z";
    static const char* SYM_MINECRAFT_INIT = "?init@Minecraft@@QEAAXXZ";
    static const char* SYM_EXIT_GAME = "?ExitGame@CConsoleMinecraftApp@@UEAAXXZ";
    static const char* SYM_MINECRAFT_SETLEVEL = "?setLevel@Minecraft@@QEAAXPEAVMultiPlayerLevel@@HV?$shared_ptr@VPlayer@@@std@@_N2@Z";
    static const char* SYM_PRESENT = "?Present@C4JRender@@QEAAXXZ";
    static const char* SYM_GET_MINECRAFT_LANGUAGE = "?GetMinecraftLanguage@CMinecraftApp@@QEAAEH@Z";
    static const char* SYM_GET_MINECRAFT_LOCALE = "?GetMinecraftLocale@CMinecraftApp@@QEAAEH@Z";

    static const char* SYM_CREATIVE_STATIC_CTOR = "?staticCtor@IUIScene_CreativeMenu@@SAXXZ";
    static const char* SYM_MAINMENU_CUSTOMDRAW = "?customDraw@UIScene_MainMenu@@UEAAXPEAUIggyCustomDrawCallbackRegion@@@Z";
    static const char* SYM_GET_STRING = "?GetString@CMinecraftApp@@SAPEB_WH@Z";
    static const char* SYM_GET_STRING_CONSOLE = "?GetString@CConsoleMinecraftApp@@SAPEB_WH@Z";

    static const char* SYM_GET_RESOURCE_AS_STREAM = "?getResourceAsStream@InputStream@@SAPEAV1@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z";
    static const char* SYM_ABSTRACT_TEXPACK_GETIMAGE = "?getImageResource@AbstractTexturePack@@UEAAPEAVBufferedImage@@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@_N10@Z";
    static const char* SYM_DLC_TEXPACK_GETIMAGE = "?getImageResource@DLCTexturePack@@UEAAPEAVBufferedImage@@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@_N10@Z";

    static const char* SYM_LOAD_UVS = "?loadUVs@PreStitchedTextureMap@@AEAAXXZ";
    static const char* SYM_PRESTITCHED_STITCH = "?stitch@PreStitchedTextureMap@@QEAAXXZ";
    static const char* SYM_SIMPLE_ICON_CTOR = "??0SimpleIcon@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@0MMMM@Z";
    static const char* SYM_OPERATOR_NEW = "??2@YAPEAX_K@Z";
    static const char* SYM_REGISTER_ICON = "?registerIcon@PreStitchedTextureMap@@UEAAPEAVIcon@@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z";
    static const char* SYM_ENTITYRENDERER_BINDTEXTURE_RESOURCE = "?bindTexture@EntityRenderer@@MEAAXPEAVResourceLocation@@@Z";
    static const char* SYM_ITEMRENDERER_RENDERITEMBILLBOARD = "?renderItemBillboard@ItemRenderer@@EEAAXV?$shared_ptr@VItemEntity@@@std@@PEAVIcon@@HMMMM@Z";
    static const char* SYM_COMPASS_CYCLEFRAMES = "?cycleFrames@CompassTexture@@UEAAXXZ";
    static const char* SYM_CLOCK_CYCLEFRAMES = "?cycleFrames@ClockTexture@@UEAAXXZ";
    static const char* SYM_COMPASS_GETSOURCEWIDTH = "?getSourceWidth@CompassTexture@@UEBAHXZ";
    static const char* SYM_COMPASS_GETSOURCEHEIGHT = "?getSourceHeight@CompassTexture@@UEBAHXZ";
    static const char* SYM_CLOCK_GETSOURCEWIDTH = "?getSourceWidth@ClockTexture@@UEBAHXZ";
    static const char* SYM_CLOCK_GETSOURCEHEIGHT = "?getSourceHeight@ClockTexture@@UEBAHXZ";
    static const char* SYM_TEXTURES_BIND_RESOURCE = "?bindTexture@Textures@@QEAAXPEAVResourceLocation@@@Z";
    static const char* SYM_TEXTURES_LOAD_BY_NAME = "?loadTexture@Textures@@AEAAHW4_TEXTURE_NAME@@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z";
    static const char* SYM_TEXTURES_LOAD_BY_INDEX_PUBLIC = "?loadTexture@Textures@@QEAAHH@Z";
    static const char* SYM_TEXTURES_LOAD_BY_INDEX_PRIVATE = "?loadTexture@Textures@@AEAAHH@Z";
    static const char* SYM_TEXTURES_READIMAGE = "?readImage@Textures@@QEAAPEAVBufferedImage@@W4_TEXTURE_NAME@@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z";
    static const char* SYM_STITCHED_GETU0 = "?getU0@StitchedTexture@@UEBAM_N@Z";
    static const char* SYM_STITCHED_GETU1 = "?getU1@StitchedTexture@@UEBAM_N@Z";
    static const char* SYM_STITCHED_GETV0 = "?getV0@StitchedTexture@@UEBAM_N@Z";
    static const char* SYM_STITCHED_GETV1 = "?getV1@StitchedTexture@@UEBAM_N@Z";
    static const char* SYM_BUFFEREDIMAGE_CTOR_FILE = "??0BufferedImage@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@_N10@Z";
    static const char* SYM_BUFFEREDIMAGE_CTOR_DLC = "??0BufferedImage@@QEAA@PEAVDLCPack@@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@_N@Z";
    static const char* SYM_TEXTUREMANAGER_CREATETEXTURE = "?createTexture@TextureManager@@QEAAPEAVTexture@@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@HHHHHHH_NPEAVBufferedImage@@@Z";
    static const char* SYM_TEXTURE_TRANSFERFROMIMAGE = "?transferFromImage@Texture@@QEAAXPEAVBufferedImage@@@Z";
    static const char* SYM_TEXATLAS_BLOCKS = "?LOCATION_BLOCKS@TextureAtlas@@2VResourceLocation@@A";
    static const char* SYM_TEXATLAS_ITEMS = "?LOCATION_ITEMS@TextureAtlas@@2VResourceLocation@@A";

    static const char* SYM_ITEMINSTANCE_GETICON = "?getIcon@ItemInstance@@QEAAPEAVIcon@@XZ";
    static const char* SYM_ITEMINSTANCE_MINEBLOCK = "?mineBlock@ItemInstance@@QEAAXPEAVLevel@@HHHHV?$shared_ptr@VPlayer@@@std@@@Z";
    static const char* SYM_ITEMINSTANCE_USEON = "?useOn@ItemInstance@@QEAA_NV?$shared_ptr@VPlayer@@@std@@PEAVLevel@@HHHHMMM_N@Z";
    static const char* SYM_ITEMINSTANCE_SAVE = "?save@ItemInstance@@QEAAPEAVCompoundTag@@PEAV2@@Z";
    static const char* SYM_ITEMINSTANCE_LOAD = "?load@ItemInstance@@QEAAXPEAVCompoundTag@@@Z";
    static const char* SYM_ITEMINSTANCE_HURTANDBREAK = "?hurtAndBreak@ItemInstance@@QEAAXHV?$shared_ptr@VLivingEntity@@@std@@@Z";
    static const char* SYM_ITEMINSTANCE_INVENTORYTICK = "?inventoryTick@ItemInstance@@QEAAXPEAVLevel@@V?$shared_ptr@VEntity@@@std@@H_N@Z";
    static const char* SYM_ITEMINSTANCE_ONCRAFTEDBY = "?onCraftedBy@ItemInstance@@QEAAXPEAVLevel@@V?$shared_ptr@VPlayer@@@std@@H@Z";
    static const char* SYM_ITEMINSTANCE_INTERACTENEMY = "?interactEnemy@ItemInstance@@QEAA_NV?$shared_ptr@VPlayer@@@std@@V?$shared_ptr@VLivingEntity@@@3@@Z";
    static const char* SYM_ITEMINSTANCE_HURTENEMY = "?hurtEnemy@ItemInstance@@QEAAXV?$shared_ptr@VLivingEntity@@@std@@V?$shared_ptr@VPlayer@@@3@@Z";
    // Not present in this build's mapping.json/PDB. Keep null so we don't spam missing-symbol logs.
    static const char* SYM_ITEMINSTANCE_SETAUXVALUE = nullptr;
    static const char* SYM_ITEMINSTANCE_SHAREDPTR_DTOR = "??1?$shared_ptr@VItemInstance@@@std@@QEAA@XZ";
    static const char* SYM_ITEMENTITY_SHAREDPTR_DTOR = "??1?$shared_ptr@VItemEntity@@@std@@QEAA@XZ";
    static const char* SYM_ITEMENTITY_SETITEM = "?setItem@ItemEntity@@QEAAXV?$shared_ptr@VItemInstance@@@std@@@Z";
    static const char* SYM_ITEMINSTANCE_CTOR_INT = "??0ItemInstance@@QEAA@HHH@Z";
    static const char* SYM_ITEMINSTANCE_SHAREDPTR_CTOR = "??$?0VItemInstance@@$0A@@?$shared_ptr@VItemInstance@@@std@@QEAA@PEAVItemInstance@@@Z";
    static const char* SYM_ENTITY_SHAREDPTR_CTOR = "??$?0VEntity@@$0A@@?$shared_ptr@VEntity@@@std@@QEAA@PEAVEntity@@@Z";
    static const char* SYM_ENTITY_SHAREDPTR_DTOR = "??1?$shared_ptr@VEntity@@@std@@QEAA@XZ";
    static const char* SYM_ITEM_MINEBLOCK = "?mineBlock@Item@@UEAA_NV?$shared_ptr@VItemInstance@@@std@@PEAVLevel@@HHHHV?$shared_ptr@VLivingEntity@@@3@@Z";
    static const char* SYM_DIGGERITEM_MINEBLOCK = "?mineBlock@DiggerItem@@UEAA_NV?$shared_ptr@VItemInstance@@@std@@PEAVLevel@@HHHHV?$shared_ptr@VLivingEntity@@@3@@Z";
    static const char* SYM_PICKAXEITEM_GETDESTROYSPEED = "?getDestroySpeed@PickaxeItem@@UEAAMV?$shared_ptr@VItemInstance@@@std@@PEAVTile@@@Z";
    static const char* SYM_PICKAXEITEM_CANDESTROYSPECIAL = "?canDestroySpecial@PickaxeItem@@UEAA_NPEAVTile@@@Z";
    static const char* SYM_SHOVELITEM_GETDESTROYSPEED = "?getDestroySpeed@DiggerItem@@UEAAMV?$shared_ptr@VItemInstance@@@std@@PEAVTile@@@Z";
    static const char* SYM_SHOVELITEM_CANDESTROYSPECIAL = "?canDestroySpecial@ShovelItem@@UEAA_NPEAVTile@@@Z";
    static const char* SYM_ITEMENTITY_GETITEM = "?getItem@ItemEntity@@QEAA?AV?$shared_ptr@VItemInstance@@@std@@XZ";
    static const char* SYM_ITEMENTITY_CTOR_WITH_ITEM = "??0ItemEntity@@QEAA@PEAVLevel@@NNNV?$shared_ptr@VItemInstance@@@std@@@Z";
    static const char* SYM_ITEMENTITY_MAKE_SHARED = "??$make_shared@VItemEntity@@AEAPEAVLevel@@AEANNAEANAEAV?$shared_ptr@VItemInstance@@@std@@@std@@YA?AV?$shared_ptr@VItemEntity@@@0@AEAPEAVLevel@@AEAN$$QEAN1AEAV?$shared_ptr@VItemInstance@@@0@@Z";
    static const char* SYM_ITEMRENDERER_RENDERGUIITEM = "?renderGuiItem@ItemRenderer@@QEAAXPEAVFont@@PEAVTextures@@V?$shared_ptr@VItemInstance@@@std@@MMMMM_N@Z";
    static const char* SYM_ITEMINHANDRENDERER_RENDER = "?render@ItemInHandRenderer@@QEAAXM@Z";
    static const char* SYM_ITEMINHANDRENDERER_RENDERITEM = "?renderItem@ItemInHandRenderer@@QEAAXV?$shared_ptr@VLivingEntity@@@std@@V?$shared_ptr@VItemInstance@@@3@H_N@Z";
    static const char* SYM_ENTITY_SPAWNATLOCATION_INT = nullptr;

    static const char* SYM_TILE_ONPLACE = "?onPlace@Tile@@UEAAXPEAVLevel@@HHH@Z";
    static const char* SYM_TILE_NEIGHBORCHANGED = "?neighborChanged@Tile@@UEAAXPEAVLevel@@HHHH@Z";
    static const char* SYM_TILE_TICK = "?tick@Tile@@UEAAXPEAVLevel@@HHHPEAVRandom@@@Z";
    static const char* SYM_TILE_USE = "?use@Tile@@UEAA_NPEAVLevel@@HHHV?$shared_ptr@VPlayer@@@std@@HMMM_N@Z";
    static const char* SYM_TILE_STEPON = "?stepOn@Tile@@UEAAXPEAVLevel@@HHHV?$shared_ptr@VEntity@@@std@@@Z";
    static const char* SYM_TILE_ENTITYINSIDE = "?entityInside@Tile@@UEAAXPEAVLevel@@HHHV?$shared_ptr@VEntity@@@std@@@Z";
    static const char* SYM_TILE_FALLON = "?fallOn@Tile@@UEAAXPEAVLevel@@HHHV?$shared_ptr@VEntity@@@std@@M@Z";
    static const char* SYM_TILE_ONREMOVING = "?onRemoving@Tile@@UEAAXPEAVLevel@@HHHH@Z";
    static const char* SYM_TILE_ONREMOVE = "?onRemove@Tile@@UEAAXPEAVLevel@@HHHHH@Z";
    static const char* SYM_TILE_DESTROY = "?destroy@Tile@@UEAAXPEAVLevel@@HHHH@Z";
    static const char* SYM_TILE_PLAYERDESTROY = "?playerDestroy@Tile@@UEAAXPEAVLevel@@V?$shared_ptr@VPlayer@@@std@@HHHH@Z";
    static const char* SYM_TILE_PLAYERWILLDESTROY = "?playerWillDestroy@Tile@@UEAAXPEAVLevel@@HHHHV?$shared_ptr@VPlayer@@@std@@@Z";
    static const char* SYM_TILE_SETPLACEDBY = "?setPlacedBy@Tile@@UEAAXPEAVLevel@@HHHV?$shared_ptr@VLivingEntity@@@std@@V?$shared_ptr@VItemInstance@@@4@@Z";
    static const char* SYM_TILE_GETRESOURCE = "?getResource@Tile@@UEAAHHPEAVRandom@@H@Z";
    static const char* SYM_TILE_GETPLACEDONFACEDATAVALUE = "?getPlacedOnFaceDataValue@Tile@@UEAAHPEAVLevel@@HHHHMMMH@Z";
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
    static const char* SYM_TILE_TILES = "?tiles@Tile@@2PEAPEAV1@EA";
    static const char* SYM_TILE_ADDAABBS = "?addAABBs@Tile@@UEAAXPEAVLevel@@HHHPEAVAABB@@PEAV?$vector@PEAVAABB@@V?$allocator@PEAVAABB@@@std@@@std@@V?$shared_ptr@VEntity@@@5@@Z";
    static const char* SYM_TILE_UPDATEDEFAULTSHAPE = "?updateDefaultShape@Tile@@UEAAXXZ";
    static const char* SYM_TILE_SET_SHAPE = "?setShape@Tile@@UEAAXMMMMMM@Z";
    static const char* SYM_AABB_NEWTEMP = "?newTemp@AABB@@SAPEAV1@NNNNNN@Z";
    static const char* SYM_AABB_CLIP = "?clip@AABB@@QEAAPEAVHitResult@@PEAVVec3@@0@Z";
    static const char* SYM_TILE_ISSOLIDRENDER = "?isSolidRender@Tile@@UEAA_N_N@Z";
    static const char* SYM_TILE_ISCUBESHAPED = "?isCubeShaped@Tile@@UEAA_NXZ";
    static const char* SYM_TILE_CLIP = "?clip@Tile@@UEAAPEAVHitResult@@PEAVLevel@@HHHPEAVVec3@@1@Z";
    static const char* SYM_VEC3_NEWTEMP = "?newTemp@Vec3@@SAPEAV1@NNN@Z";
    static const char* SYM_HITRESULT_CTOR = "??0HitResult@@QEAA@HHHHPEAVVec3@@@Z";
    static const char* SYM_TILERENDERER_TESSELLATE_IN_WORLD = "?tesselateInWorld@TileRenderer@@QEAA_NPEAVTile@@HHHHV?$shared_ptr@VTileEntity@@@std@@@Z";
    static const char* SYM_TILERENDERER_TESSELLATE_BLOCK_IN_WORLD = "?tesselateBlockInWorld@TileRenderer@@QEAA_NPEAVTile@@HHH@Z";
    static const char* SYM_TILERENDERER_SET_SHAPE = "?setShape@TileRenderer@@QEAAXMMMMMM@Z";
    static const char* SYM_TILERENDERER_SET_SHAPE_TILE = "?setShape@TileRenderer@@QEAAXPEAVTile@@@Z";
    static const char* SYM_TILERENDERER_RENDER_TILE = "?renderTile@TileRenderer@@QEAAXPEAVTile@@HMM_N@Z";

    static const char* SYM_LEVEL_UPDATE_NEIGHBORS_AT = "?updateNeighborsAt@Level@@QEAAXHHHH@Z";
    static const char* SYM_SERVERLEVEL_TICKPENDINGTICKS = "?tickPendingTicks@ServerLevel@@UEAA_N_N@Z";
    static const char* SYM_LEVEL_GETTILE = "?getTile@Level@@UEAAHHHH@Z";
    static const char* SYM_LEVEL_GETDATA = "?getData@Level@@UEAAHHHH@Z";
    static const char* SYM_LEVEL_SETDATA = "?setData@Level@@UEAA_NHHHHH_N@Z";
    static const char* SYM_LEVEL_CLIP = "?clip@Level@@QEAAPEAVHitResult@@PEAVVec3@@0_N1@Z";
    static const char* SYM_MCREGIONCHUNKSTORAGE_LOAD = "?load@McRegionChunkStorage@@UEAAPEAVLevelChunk@@PEAVLevel@@HH@Z";
    static const char* SYM_MCREGIONCHUNKSTORAGE_SAVE = "?save@McRegionChunkStorage@@UEAAXPEAVLevel@@PEAVLevelChunk@@@Z";
    static const char* SYM_LEVEL_SETTILEANDDATA = "?setTileAndData@Level@@UEAA_NHHHHHH@Z";
    static const char* SYM_LEVEL_HASNEIGHBORSIGNAL = "?hasNeighborSignal@Level@@QEAA_NHHH@Z";
    static const char* SYM_LEVEL_ADDTOTICKNEXTTICK = "?addToTickNextTick@Level@@UEAAXHHHHH@Z";
    static const char* SYM_SERVERLEVEL_ADDTOTICKNEXTTICK = "?addToTickNextTick@ServerLevel@@UEAAXHHHHH@Z";
    static const char* SYM_LEVELCHUNK_GETDATA = "?getData@LevelChunk@@UEAAHHHH@Z";
    static const char* SYM_LEVELCHUNK_SETTILEANDDATA = "?setTileAndData@LevelChunk@@UEAA_NHHHHH@Z";

    static const char* SYM_PLAYER_CANDESTROY = "?canDestroy@Player@@QEAA_NPEAVTile@@@Z";
    static const char* SYM_SERVER_PLAYER_GAMEMODE_USEITEM = "?useItem@ServerPlayerGameMode@@QEAA_NV?$shared_ptr@VPlayer@@@std@@PEAVLevel@@V?$shared_ptr@VItemInstance@@@3@_N@Z";
    static const char* SYM_MULTI_PLAYER_GAMEMODE_USEITEM = "?useItem@MultiPlayerGameMode@@UEAA_NV?$shared_ptr@VPlayer@@@std@@PEAVLevel@@V?$shared_ptr@VItemInstance@@@3@_N@Z";
    static const char* SYM_SERVER_PLAYER_GAMEMODE_USEITEMON = "?useItemOn@ServerPlayerGameMode@@QEAA_NV?$shared_ptr@VPlayer@@@std@@PEAVLevel@@V?$shared_ptr@VItemInstance@@@3@HHHHMMM_NPEA_N@Z";
    static const char* SYM_MULTI_PLAYER_GAMEMODE_USEITEMON = "?useItemOn@MultiPlayerGameMode@@UEAA_NV?$shared_ptr@VPlayer@@@std@@PEAVLevel@@V?$shared_ptr@VItemInstance@@@3@HHHHPEAVVec3@@_NPEA_N@Z";
    static const char* SYM_LEVEL_ADDENTITY = "?addEntity@Level@@UEAA_NV?$shared_ptr@VEntity@@@std@@@Z";
    static const char* SYM_ENTITYIO_NEWBYID = "?newById@EntityIO@@SA?AV?$shared_ptr@VEntity@@@std@@HPEAVLevel@@@Z";
    static const char* SYM_ENTITY_MOVETO = "?moveTo@Entity@@QEAAXNNNMM@Z";
    static const char* SYM_ENTITY_CHECKINSIDETILES = "?checkInsideTiles@Entity@@MEAAXXZ";
    static const char* SYM_ENTITY_SETPOS = "?setPos@Entity@@QEAAXNNN@Z";
    static const char* SYM_ENTITY_PLAYSTEPSOUND = "?playStepSound@Entity@@MEAAXHHHH@Z";
    static const char* SYM_LIVINGENTITY_GETLOOKANGLE = "?getLookAngle@LivingEntity@@UEAAPEAVVec3@@XZ";
    static const char* SYM_ENTITY_GETLOOKANGLE = "?getLookAngle@Entity@@UEAAPEAVVec3@@XZ";
    static const char* SYM_LIVINGENTITY_GETVIEWVECTOR = "?getViewVector@LivingEntity@@UEAAPEAVVec3@@M@Z";
    static const char* SYM_LIVINGENTITY_GETPOS = "?getPos@LivingEntity@@UEAAPEAVVec3@@M@Z";
    static const char* SYM_LIVINGENTITY_GETPOS_V = "?getPos@LivingEntity@@UEAAPEAVVec3@@M@Z";
    static const char* SYM_LIVINGENTITY_PICK = "?pick@LivingEntity@@UEAAPEAVHitResult@@NM@Z";
    static const char* SYM_ENTITY_LERPMOTION = "?lerpMotion@Entity@@UEAAXNNN@Z";
    static const char* SYM_ENTITY_SPAWNATLOCATION = "?spawnAtLocation@Entity@@QEAA?AV?$shared_ptr@VItemEntity@@@std@@V?$shared_ptr@VItemInstance@@@3@M@Z";
    static const char* SYM_ENTITY_GETENCODEID = "?getEncodeId@EntityIO@@SA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@V?$shared_ptr@VEntity@@@3@@Z";
    static const char* SYM_ENTITY_GETENCODEID_BYID = "?getEncodeId@EntityIO@@SA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@H@Z";
    static const char* SYM_ENTITY_GETNETWORKNAME = "?getNetworkName@Entity@@UEAA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@XZ";
    static const char* SYM_LIVINGENTITY_TICKDEATH = "?tickDeath@LivingEntity@@MEAAXXZ";
    static const char* SYM_LIVINGENTITY_DROPDEATHLOOT = "?dropDeathLoot@LivingEntity@@MEAAX_NH@Z";
    static const char* SYM_MOB_DROPDEATHLOOT = "?dropDeathLoot@Mob@@MEAAX_NH@Z";
    static const char* SYM_CHICKEN_DROPDEATHLOOT = "?dropDeathLoot@Chicken@@MEAAX_NH@Z";
    static const char* SYM_COW_DROPDEATHLOOT = "?dropDeathLoot@Cow@@MEAAX_NH@Z";
    static const char* SYM_PIG_DROPDEATHLOOT = "?dropDeathLoot@Pig@@MEAAX_NH@Z";
    static const char* SYM_SHEEP_DROPDEATHLOOT = "?dropDeathLoot@Sheep@@UEAAX_NH@Z";
    static const char* SYM_SQUID_DROPDEATHLOOT = "?dropDeathLoot@Squid@@MEAAX_NH@Z";
    static const char* SYM_OCELOT_DROPDEATHLOOT = "?dropDeathLoot@Ocelot@@MEAAX_NH@Z";
    static const char* SYM_SNOWMAN_DROPDEATHLOOT = "?dropDeathLoot@SnowMan@@MEAAX_NH@Z";
    static const char* SYM_VILLAGERGOLEM_DROPDEATHLOOT = "?dropDeathLoot@VillagerGolem@@MEAAX_NH@Z";
    static const char* SYM_PIGZOMBIE_DROPDEATHLOOT = "?dropDeathLoot@PigZombie@@MEAAX_NH@Z";
    static const char* SYM_SPIDER_DROPDEATHLOOT = "?dropDeathLoot@Spider@@MEAAX_NH@Z";
    static const char* SYM_SKELETON_DROPDEATHLOOT = "?dropDeathLoot@Skeleton@@MEAAX_NH@Z";
    static const char* SYM_WITCH_DROPDEATHLOOT = "?dropDeathLoot@Witch@@MEAAX_NH@Z";
    static const char* SYM_BLAZE_DROPDEATHLOOT = "?dropDeathLoot@Blaze@@MEAAX_NH@Z";
    static const char* SYM_ENDERMAN_DROPDEATHLOOT = "?dropDeathLoot@EnderMan@@MEAAX_NH@Z";
    static const char* SYM_GHAST_DROPDEATHLOOT = "?dropDeathLoot@Ghast@@MEAAX_NH@Z";
    static const char* SYM_LAVASLIME_DROPDEATHLOOT = "?dropDeathLoot@LavaSlime@@MEAAX_NH@Z";
    static const char* SYM_WITHERBOSS_DROPDEATHLOOT = "?dropDeathLoot@WitherBoss@@MEAAX_NH@Z";

    static const char* SYM_INVENTORY_REMOVERESOURCE = "?removeResource@Inventory@@QEAA_NH@Z";
    static const char* SYM_INVENTORY_VFTABLE = "??_7Inventory@@6B@";
    static const char* SYM_ABSTRACTCONTAINERMENU_BROADCASTCHANGES = "?broadcastChanges@AbstractContainerMenu@@UEAAXXZ";

    static const char* SYM_TAG_NEWTAG = "?newTag@Tag@@SAPEAV1@EAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z";
}

bool CoreSymbols::Resolve(SymbolResolver& resolver)
{
    pRunStaticCtors = resolver.Resolve(SYM_RUN_STATIC_CTORS);
    pMinecraftTick = resolver.Resolve(SYM_MINECRAFT_TICK);
    pMinecraftInit = resolver.Resolve(SYM_MINECRAFT_INIT);
    pMinecraftSetLevel = resolver.Resolve(SYM_MINECRAFT_SETLEVEL);
    pExitGame = resolver.Resolve(SYM_EXIT_GAME);
    pPresent = resolver.Resolve(SYM_PRESENT);
    pMinecraftApp = resolver.ResolveOptional("?app@@3VCMinecraftApp@@A");
    if (!pMinecraftApp)
        pMinecraftApp = resolver.ResolveExactOptional("app");
    pGetMinecraftLanguage = resolver.ResolveOptional(SYM_GET_MINECRAFT_LANGUAGE);
    if (!pGetMinecraftLanguage)
        pGetMinecraftLanguage = resolver.ResolveExactOptional("CMinecraftApp::GetMinecraftLanguage");
    pGetMinecraftLocale = resolver.ResolveOptional(SYM_GET_MINECRAFT_LOCALE);
    if (!pGetMinecraftLocale)
        pGetMinecraftLocale = resolver.ResolveExactOptional("CMinecraftApp::GetMinecraftLocale");
    return HasCritical();
}

void CoreSymbols::Log() const
{
    LogSym("RunStaticCtors", pRunStaticCtors);
    LogSym("Minecraft::tick", pMinecraftTick);
    LogSym("Minecraft::init", pMinecraftInit);
    LogSym("Minecraft::setLevel", pMinecraftSetLevel);
    LogSym("ExitGame", pExitGame);
    LogSym("C4JRender::Present", pPresent);
    LogSymOptional("app (CMinecraftApp)", pMinecraftApp);
    LogSymOptional("CMinecraftApp::GetMinecraftLanguage", pGetMinecraftLanguage);
    LogSymOptional("CMinecraftApp::GetMinecraftLocale", pGetMinecraftLocale);
}

bool CoreSymbols::HasCritical() const
{
    return pRunStaticCtors && pMinecraftTick && pMinecraftInit;
}

bool UiSymbols::Resolve(SymbolResolver& resolver)
{
    pCreativeStaticCtor = resolver.Resolve(SYM_CREATIVE_STATIC_CTOR);
    pMainMenuCustomDraw = resolver.Resolve(SYM_MAINMENU_CUSTOMDRAW);
    pGetString = resolver.Resolve(SYM_GET_STRING);
    if (!pGetString)
        pGetString = resolver.Resolve(SYM_GET_STRING_CONSOLE);
    return true;
}

void UiSymbols::Log() const
{
    LogSym("CreativeStaticCtor", pCreativeStaticCtor);
    LogSym("MainMenuCustomDraw", pMainMenuCustomDraw);
    LogSym("CMinecraftApp::GetString", pGetString);
}

bool ResourceSymbols::Resolve(SymbolResolver& resolver)
{
    pGetResourceAsStream = resolver.Resolve(SYM_GET_RESOURCE_AS_STREAM);
    pAbstractTexturePackGetImageResource = resolver.Resolve(SYM_ABSTRACT_TEXPACK_GETIMAGE);
    if (!pAbstractTexturePackGetImageResource)
        pAbstractTexturePackGetImageResource = resolver.ResolveExact("AbstractTexturePack::getImageResource");
    pDLCTexturePackGetImageResource = resolver.Resolve(SYM_DLC_TEXPACK_GETIMAGE);
    if (!pDLCTexturePackGetImageResource)
        pDLCTexturePackGetImageResource = resolver.ResolveExact("DLCTexturePack::getImageResource");
    return true;
}

void ResourceSymbols::Log() const
{
    LogSym("InputStream::getResourceAsStream", pGetResourceAsStream);
    LogSym("AbstractTexturePack::getImageResource", pAbstractTexturePackGetImageResource);
    LogSym("DLCTexturePack::getImageResource", pDLCTexturePackGetImageResource);
}

bool TextureSymbols::Resolve(SymbolResolver& resolver)
{
    pLoadUVs = resolver.Resolve(SYM_LOAD_UVS);
    pPreStitchedTextureMapStitch = resolver.Resolve(SYM_PRESTITCHED_STITCH);
    if (!pPreStitchedTextureMapStitch)
        pPreStitchedTextureMapStitch = resolver.ResolveExact("PreStitchedTextureMap::stitch");
    pSimpleIconCtor = resolver.Resolve(SYM_SIMPLE_ICON_CTOR);
    pOperatorNew = resolver.Resolve(SYM_OPERATOR_NEW);
    pRegisterIcon = resolver.Resolve(SYM_REGISTER_ICON);
    pEntityRendererBindTextureResource = resolver.Resolve(SYM_ENTITYRENDERER_BINDTEXTURE_RESOURCE);
    pItemRendererRenderItemBillboard = resolver.Resolve(SYM_ITEMRENDERER_RENDERITEMBILLBOARD);
    pCompassTextureCycleFrames = resolver.Resolve(SYM_COMPASS_CYCLEFRAMES);
    pClockTextureCycleFrames = resolver.Resolve(SYM_CLOCK_CYCLEFRAMES);
    pCompassTextureGetSourceWidth = resolver.Resolve(SYM_COMPASS_GETSOURCEWIDTH);
    pCompassTextureGetSourceHeight = resolver.Resolve(SYM_COMPASS_GETSOURCEHEIGHT);
    pClockTextureGetSourceWidth = resolver.Resolve(SYM_CLOCK_GETSOURCEWIDTH);
    pClockTextureGetSourceHeight = resolver.Resolve(SYM_CLOCK_GETSOURCEHEIGHT);
    pTexturesBindTextureResource = resolver.Resolve(SYM_TEXTURES_BIND_RESOURCE);
    pTexturesLoadTextureByName = resolver.Resolve(SYM_TEXTURES_LOAD_BY_NAME);
    pTexturesLoadTextureByIndex = resolver.Resolve(SYM_TEXTURES_LOAD_BY_INDEX_PUBLIC);
    if (!pTexturesLoadTextureByIndex)
        pTexturesLoadTextureByIndex = resolver.Resolve(SYM_TEXTURES_LOAD_BY_INDEX_PRIVATE);
    pTexturesReadImage = resolver.Resolve(SYM_TEXTURES_READIMAGE);
    pStitchedGetU0 = resolver.Resolve(SYM_STITCHED_GETU0);
    pStitchedGetU1 = resolver.Resolve(SYM_STITCHED_GETU1);
    pStitchedGetV0 = resolver.Resolve(SYM_STITCHED_GETV0);
    pStitchedGetV1 = resolver.Resolve(SYM_STITCHED_GETV1);
    pBufferedImageCtorFile = resolver.Resolve(SYM_BUFFEREDIMAGE_CTOR_FILE);
    pBufferedImageCtorDLCPack = resolver.Resolve(SYM_BUFFEREDIMAGE_CTOR_DLC);
    pTextureManagerCreateTexture = resolver.Resolve(SYM_TEXTUREMANAGER_CREATETEXTURE);
    pTextureTransferFromImage = resolver.Resolve(SYM_TEXTURE_TRANSFERFROMIMAGE);
    pTextureAtlasLocationBlocks = resolver.Resolve(SYM_TEXATLAS_BLOCKS);
    pTextureAtlasLocationItems = resolver.Resolve(SYM_TEXATLAS_ITEMS);

    if (!pOperatorNew)
        pOperatorNew = GetProcAddress(GetModuleHandleA("vcruntime140.dll"), SYM_OPERATOR_NEW);
    if (!pOperatorNew)
        pOperatorNew = GetProcAddress(GetModuleHandleA("vcruntime140d.dll"), SYM_OPERATOR_NEW);
    if (!pOperatorNew)
        pOperatorNew = GetProcAddress(GetModuleHandle(nullptr), SYM_OPERATOR_NEW);

    return true;
}

void TextureSymbols::Log() const
{
    LogSym("PreStitchedTextureMap::loadUVs", pLoadUVs);
    LogSym("PreStitchedTextureMap::stitch", pPreStitchedTextureMapStitch);
    LogSym("SimpleIcon::SimpleIcon", pSimpleIconCtor);
    LogSym("operator new", pOperatorNew);
    LogSym("registerIcon", pRegisterIcon);
    LogSym("EntityRenderer::bindTexture(ResourceLocation)", pEntityRendererBindTextureResource);
    LogSym("ItemRenderer::renderItemBillboard", pItemRendererRenderItemBillboard);
    LogSym("CompassTexture::cycleFrames", pCompassTextureCycleFrames);
    LogSym("ClockTexture::cycleFrames", pClockTextureCycleFrames);
    LogSym("CompassTexture::getSourceWidth", pCompassTextureGetSourceWidth);
    LogSym("CompassTexture::getSourceHeight", pCompassTextureGetSourceHeight);
    LogSym("ClockTexture::getSourceWidth", pClockTextureGetSourceWidth);
    LogSym("ClockTexture::getSourceHeight", pClockTextureGetSourceHeight);
    LogSym("Textures::bindTexture(ResourceLocation)", pTexturesBindTextureResource);
    LogSym("Textures::loadTexture(TEXTURE_NAME,wstring)", pTexturesLoadTextureByName);
    LogSym("Textures::loadTexture(int)", pTexturesLoadTextureByIndex);
    LogSym("Textures::readImage(TEXTURE_NAME,wstring)", pTexturesReadImage);
    LogSym("StitchedTexture::getU0", pStitchedGetU0);
    LogSym("StitchedTexture::getU1", pStitchedGetU1);
    LogSym("StitchedTexture::getV0", pStitchedGetV0);
    LogSym("StitchedTexture::getV1", pStitchedGetV1);
    LogSym("BufferedImage::BufferedImage(file)", pBufferedImageCtorFile);
    LogSym("BufferedImage::BufferedImage(DLCPack)", pBufferedImageCtorDLCPack);
    LogSym("TextureManager::createTexture", pTextureManagerCreateTexture);
    LogSym("Texture::transferFromImage", pTextureTransferFromImage);
    LogSym("TextureAtlas::LOCATION_BLOCKS", pTextureAtlasLocationBlocks);
    LogSym("TextureAtlas::LOCATION_ITEMS", pTextureAtlasLocationItems);
}

bool ItemSymbols::Resolve(SymbolResolver& resolver)
{
    pItemInstanceGetIcon = resolver.Resolve(SYM_ITEMINSTANCE_GETICON);
    pItemInstanceMineBlock = resolver.Resolve(SYM_ITEMINSTANCE_MINEBLOCK);
    pItemInstanceUseOn = resolver.Resolve(SYM_ITEMINSTANCE_USEON);
    pItemInstanceSave = resolver.Resolve(SYM_ITEMINSTANCE_SAVE);
    pItemInstanceLoad = resolver.Resolve(SYM_ITEMINSTANCE_LOAD);
    pItemInstanceHurtAndBreak = resolver.Resolve(SYM_ITEMINSTANCE_HURTANDBREAK);
    pItemInstanceInventoryTick = resolver.Resolve(SYM_ITEMINSTANCE_INVENTORYTICK);
    pItemInstanceOnCraftedBy = resolver.Resolve(SYM_ITEMINSTANCE_ONCRAFTEDBY);
    pItemInstanceInteractEnemy = resolver.Resolve(SYM_ITEMINSTANCE_INTERACTENEMY);
    pItemInstanceHurtEnemy = resolver.Resolve(SYM_ITEMINSTANCE_HURTENEMY);
    if (SYM_ITEMINSTANCE_SETAUXVALUE)
        pItemInstanceSetAuxValue = resolver.Resolve(SYM_ITEMINSTANCE_SETAUXVALUE);
    pItemInstanceCtor = resolver.Resolve(SYM_ITEMINSTANCE_CTOR_INT);
    pItemInstanceSharedPtrCtor = resolver.Resolve(SYM_ITEMINSTANCE_SHAREDPTR_CTOR);
    pItemInstanceSharedPtrDtor = resolver.Resolve(SYM_ITEMINSTANCE_SHAREDPTR_DTOR);
    pEntitySharedPtrCtor = resolver.Resolve(SYM_ENTITY_SHAREDPTR_CTOR);
    pEntitySharedPtrDtor = resolver.Resolve(SYM_ENTITY_SHAREDPTR_DTOR);
    pItemEntitySharedPtrDtor = resolver.Resolve(SYM_ITEMENTITY_SHAREDPTR_DTOR);
    pItemEntityCtorWithItem = resolver.Resolve(SYM_ITEMENTITY_CTOR_WITH_ITEM);
    pItemEntitySetItem = resolver.Resolve(SYM_ITEMENTITY_SETITEM);
    pItemEntityMakeShared = resolver.Resolve(SYM_ITEMENTITY_MAKE_SHARED);
    pItemMineBlock = resolver.Resolve(SYM_ITEM_MINEBLOCK);
    pDiggerItemMineBlock = resolver.Resolve(SYM_DIGGERITEM_MINEBLOCK);
    pPickaxeItemGetDestroySpeed = resolver.Resolve(SYM_PICKAXEITEM_GETDESTROYSPEED);
    pPickaxeItemCanDestroySpecial = resolver.Resolve(SYM_PICKAXEITEM_CANDESTROYSPECIAL);
    pShovelItemGetDestroySpeed = resolver.Resolve(SYM_SHOVELITEM_GETDESTROYSPEED);
    pShovelItemCanDestroySpecial = resolver.Resolve(SYM_SHOVELITEM_CANDESTROYSPECIAL);
    pItemEntityGetItem = resolver.Resolve(SYM_ITEMENTITY_GETITEM);
    pItemRendererRenderGuiItem = resolver.Resolve(SYM_ITEMRENDERER_RENDERGUIITEM);
    pItemInHandRendererRender = resolver.Resolve(SYM_ITEMINHANDRENDERER_RENDER);
    pItemInHandRendererRenderItem = resolver.Resolve(SYM_ITEMINHANDRENDERER_RENDERITEM);
    if (!pShovelItemGetDestroySpeed)
        pShovelItemGetDestroySpeed = resolver.ResolveExact("DiggerItem::getDestroySpeed");
    return true;
}

void ItemSymbols::Log() const
{
    LogSym("ItemInstance::getIcon", pItemInstanceGetIcon);
    LogSym("ItemInstance::mineBlock", pItemInstanceMineBlock);
    LogSym("ItemInstance::useOn", pItemInstanceUseOn);
    LogSym("ItemInstance::save", pItemInstanceSave);
    LogSym("ItemInstance::load", pItemInstanceLoad);
    LogSym("ItemInstance::hurtAndBreak", pItemInstanceHurtAndBreak);
    LogSym("ItemInstance::inventoryTick", pItemInstanceInventoryTick);
    LogSym("ItemInstance::onCraftedBy", pItemInstanceOnCraftedBy);
    LogSym("ItemInstance::interactEnemy", pItemInstanceInteractEnemy);
    LogSym("ItemInstance::hurtEnemy", pItemInstanceHurtEnemy);
    LogSymOptional("ItemInstance::setAuxValue", pItemInstanceSetAuxValue);
    LogSym("ItemInstance::ItemInstance(int,int,int)", pItemInstanceCtor);
    LogSym("std::shared_ptr<ItemInstance>::shared_ptr(ItemInstance*)", pItemInstanceSharedPtrCtor);
    LogSym("std::shared_ptr<ItemInstance>::~shared_ptr", pItemInstanceSharedPtrDtor);
    LogSym("std::shared_ptr<Entity>::shared_ptr(Entity*)", pEntitySharedPtrCtor);
    LogSym("std::shared_ptr<Entity>::~shared_ptr", pEntitySharedPtrDtor);
    LogSym("std::shared_ptr<ItemEntity>::~shared_ptr", pItemEntitySharedPtrDtor);
    LogSym("ItemEntity::ItemEntity(Level,double,double,double,shared_ptr<ItemInstance>)", pItemEntityCtorWithItem);
    LogSym("ItemEntity::setItem", pItemEntitySetItem);
    LogSym("std::make_shared<ItemEntity>", pItemEntityMakeShared);
    LogSym("Item::mineBlock", pItemMineBlock);
    LogSym("DiggerItem::mineBlock", pDiggerItemMineBlock);
    LogSym("PickaxeItem::getDestroySpeed", pPickaxeItemGetDestroySpeed);
    LogSym("PickaxeItem::canDestroySpecial", pPickaxeItemCanDestroySpecial);
    LogSym("DiggerItem::getDestroySpeed", pShovelItemGetDestroySpeed);
    LogSym("ShovelItem::canDestroySpecial", pShovelItemCanDestroySpecial);
    LogSym("ItemEntity::getItem", pItemEntityGetItem);
    LogSym("ItemRenderer::renderGuiItem", pItemRendererRenderGuiItem);
    LogSym("ItemInHandRenderer::render", pItemInHandRendererRender);
    LogSym("ItemInHandRenderer::renderItem", pItemInHandRendererRenderItem);
}

bool TileSymbols::Resolve(SymbolResolver& resolver)
{
    pTileOnPlace = resolver.Resolve(SYM_TILE_ONPLACE);
    pTileNeighborChanged = resolver.Resolve(SYM_TILE_NEIGHBORCHANGED);
    pTileTick = resolver.Resolve(SYM_TILE_TICK);
    pTileUse = resolver.Resolve(SYM_TILE_USE);
    pTileStepOn = resolver.Resolve(SYM_TILE_STEPON);
    pTileEntityInside = resolver.Resolve(SYM_TILE_ENTITYINSIDE);
    pTileFallOn = resolver.Resolve(SYM_TILE_FALLON);
    pTileOnRemoving = resolver.Resolve(SYM_TILE_ONREMOVING);
    pTileOnRemove = resolver.Resolve(SYM_TILE_ONREMOVE);
    pTileDestroy = resolver.Resolve(SYM_TILE_DESTROY);
    pTilePlayerDestroy = resolver.Resolve(SYM_TILE_PLAYERDESTROY);
    pTilePlayerWillDestroy = resolver.Resolve(SYM_TILE_PLAYERWILLDESTROY);
    pTileSetPlacedBy = resolver.Resolve(SYM_TILE_SETPLACEDBY);
    pTileGetResource = resolver.Resolve(SYM_TILE_GETRESOURCE);
    pTileGetPlacedOnFaceDataValue = resolver.Resolve(SYM_TILE_GETPLACEDONFACEDATAVALUE);
    pTileCloneTileId = resolver.Resolve(SYM_TILE_CLONETILEID);
    pTileGetTextureFaceData = resolver.Resolve(SYM_TILE_GETTEXTURE_FACEDATA);
    pStoneSlabGetTexture = resolver.Resolve(SYM_STONESLAB_GETTEXTURE);
    pWoodSlabGetTexture = resolver.Resolve(SYM_WOODSLAB_GETTEXTURE);
    pStoneSlabGetResource = resolver.Resolve(SYM_STONESLAB_GETRESOURCE);
    pWoodSlabGetResource = resolver.Resolve(SYM_WOODSLAB_GETRESOURCE);
    pStoneSlabGetDescriptionId = resolver.Resolve(SYM_STONESLAB_GETDESCRIPTIONID);
    pWoodSlabGetDescriptionId = resolver.Resolve(SYM_WOODSLAB_GETDESCRIPTIONID);
    pStoneSlabGetAuxName = resolver.Resolve(SYM_STONESLAB_GETAUXNAME);
    pWoodSlabGetAuxName = resolver.Resolve(SYM_WOODSLAB_GETAUXNAME);
    pStoneSlabRegisterIcons = resolver.Resolve(SYM_STONESLAB_REGISTERICONS);
    pWoodSlabRegisterIcons = resolver.Resolve(SYM_WOODSLAB_REGISTERICONS);
    pStoneSlabItemGetIcon = resolver.Resolve(SYM_STONESLABITEM_GETICON);
    pStoneSlabItemGetDescriptionId = resolver.Resolve(SYM_STONESLABITEM_GETDESCRIPTIONID);
    pHalfSlabCloneTileId = resolver.Resolve(SYM_HALFSLAB_CLONETILEID);
    pTileTiles = resolver.Resolve(SYM_TILE_TILES);
    pTileAddAABBs = resolver.Resolve(SYM_TILE_ADDAABBS);
    pTileUpdateDefaultShape = resolver.Resolve(SYM_TILE_UPDATEDEFAULTSHAPE);
    pTileSetShape = resolver.Resolve(SYM_TILE_SET_SHAPE);
    pAABBNewTemp = resolver.Resolve(SYM_AABB_NEWTEMP);
    pAABBClip = resolver.Resolve(SYM_AABB_CLIP);
    pTileIsSolidRender = resolver.Resolve(SYM_TILE_ISSOLIDRENDER);
    pTileIsCubeShaped = resolver.Resolve(SYM_TILE_ISCUBESHAPED);
    pTileClip = resolver.Resolve(SYM_TILE_CLIP);
    pVec3NewTemp = resolver.Resolve(SYM_VEC3_NEWTEMP);
    pHitResultCtor = resolver.Resolve(SYM_HITRESULT_CTOR);
    pTileRendererTesselateInWorld = resolver.Resolve(SYM_TILERENDERER_TESSELLATE_IN_WORLD);
    pTileRendererTesselateBlockInWorld = resolver.Resolve(SYM_TILERENDERER_TESSELLATE_BLOCK_IN_WORLD);
    pTileRendererSetShape = resolver.Resolve(SYM_TILERENDERER_SET_SHAPE);
    pTileRendererSetShapeTile = resolver.Resolve(SYM_TILERENDERER_SET_SHAPE_TILE);
    pTileRendererRenderTile = resolver.Resolve(SYM_TILERENDERER_RENDER_TILE);

    if (resolver.IsStub(pTileOnPlace))
        pTileOnPlace = resolver.ResolveExact("Tile::onPlace");
    if (resolver.IsStub(pTileNeighborChanged))
        pTileNeighborChanged = resolver.ResolveExact("Tile::neighborChanged");
    if (resolver.IsStub(pTileTick))
        pTileTick = resolver.ResolveExact("Tile::tick");
    if (resolver.IsStub(pTileUse))
        pTileUse = resolver.ResolveExact("Tile::use");
    if (resolver.IsStub(pTileStepOn))
        pTileStepOn = resolver.ResolveExact("Tile::stepOn");
    if (resolver.IsStub(pTileEntityInside))
        pTileEntityInside = resolver.ResolveExact("Tile::entityInside");
    if (resolver.IsStub(pTileFallOn))
        pTileFallOn = resolver.ResolveExact("Tile::fallOn");
    if (resolver.IsStub(pTileOnRemoving))
        pTileOnRemoving = resolver.ResolveExact("Tile::onRemoving");
    if (resolver.IsStub(pTileOnRemove))
        pTileOnRemove = resolver.ResolveExact("Tile::onRemove");
    if (resolver.IsStub(pTileDestroy))
        pTileDestroy = resolver.ResolveExact("Tile::destroy");
    if (resolver.IsStub(pTilePlayerDestroy))
        pTilePlayerDestroy = resolver.ResolveExact("Tile::playerDestroy");
    if (resolver.IsStub(pTilePlayerWillDestroy))
        pTilePlayerWillDestroy = resolver.ResolveExact("Tile::playerWillDestroy");
    if (resolver.IsStub(pTileSetPlacedBy))
        pTileSetPlacedBy = resolver.ResolveExact("Tile::setPlacedBy");
    if (resolver.IsStub(pWoodSlabRegisterIcons))
        pWoodSlabRegisterIcons = resolver.ResolveExact("WoodSlabTile::registerIcons");
    if (resolver.IsStub(pTileClip))
        pTileClip = resolver.ResolveExact("Tile::clip");
    return true;
}

void TileSymbols::Log() const
{
    LogSym("Tile::onPlace", pTileOnPlace);
    LogSym("Tile::neighborChanged", pTileNeighborChanged);
    LogSym("Tile::tick", pTileTick);
    LogSym("Tile::use", pTileUse);
    LogSym("Tile::stepOn", pTileStepOn);
    LogSym("Tile::entityInside", pTileEntityInside);
    LogSym("Tile::fallOn", pTileFallOn);
    LogSym("Tile::onRemoving", pTileOnRemoving);
    LogSym("Tile::onRemove", pTileOnRemove);
    LogSym("Tile::destroy", pTileDestroy);
    LogSym("Tile::playerDestroy", pTilePlayerDestroy);
    LogSym("Tile::playerWillDestroy", pTilePlayerWillDestroy);
    LogSym("Tile::setPlacedBy", pTileSetPlacedBy);
    LogSym("Tile::getResource", pTileGetResource);
    LogSym("Tile::getPlacedOnFaceDataValue", pTileGetPlacedOnFaceDataValue);
    LogSym("Tile::cloneTileId", pTileCloneTileId);
    LogSym("Tile::getTexture(face,data)", pTileGetTextureFaceData);
    LogSym("StoneSlabTile::getTexture", pStoneSlabGetTexture);
    LogSym("WoodSlabTile::getTexture", pWoodSlabGetTexture);
    LogSym("StoneSlabTile::getResource", pStoneSlabGetResource);
    LogSym("WoodSlabTile::getResource", pWoodSlabGetResource);
    LogSym("StoneSlabTile::getDescriptionId", pStoneSlabGetDescriptionId);
    LogSym("WoodSlabTile::getDescriptionId", pWoodSlabGetDescriptionId);
    LogSym("StoneSlabTile::getAuxName", pStoneSlabGetAuxName);
    LogSym("WoodSlabTile::getAuxName", pWoodSlabGetAuxName);
    LogSym("StoneSlabTile::registerIcons", pStoneSlabRegisterIcons);
    LogSym("WoodSlabTile::registerIcons", pWoodSlabRegisterIcons);
    LogSym("StoneSlabTileItem::getIcon", pStoneSlabItemGetIcon);
    LogSym("StoneSlabTileItem::getDescriptionId", pStoneSlabItemGetDescriptionId);
    LogSym("HalfSlabTile::cloneTileId", pHalfSlabCloneTileId);
    LogSym("Tile::tiles", pTileTiles);
    LogSym("Tile::addAABBs", pTileAddAABBs);
    LogSym("Tile::updateDefaultShape", pTileUpdateDefaultShape);
    LogSym("Tile::setShape", pTileSetShape);
    LogSym("AABB::newTemp", pAABBNewTemp);
    LogSym("AABB::clip", pAABBClip);
    LogSym("Tile::isSolidRender", pTileIsSolidRender);
    LogSym("Tile::isCubeShaped", pTileIsCubeShaped);
    LogSym("Tile::clip", pTileClip);
    LogSym("Vec3::newTemp", pVec3NewTemp);
    LogSym("HitResult::HitResult", pHitResultCtor);
    LogSym("TileRenderer::tesselateInWorld", pTileRendererTesselateInWorld);
    LogSym("TileRenderer::tesselateBlockInWorld", pTileRendererTesselateBlockInWorld);
    LogSym("TileRenderer::setShape(float)", pTileRendererSetShape);
    LogSym("TileRenderer::setShape(Tile)", pTileRendererSetShapeTile);
    LogSym("TileRenderer::renderTile", pTileRendererRenderTile);
}

bool LevelSymbols::Resolve(SymbolResolver& resolver)
{
    pLevelUpdateNeighborsAt = resolver.Resolve(SYM_LEVEL_UPDATE_NEIGHBORS_AT);
    pServerLevelTickPendingTicks = resolver.Resolve(SYM_SERVERLEVEL_TICKPENDINGTICKS);
    pLevelGetTile = resolver.Resolve(SYM_LEVEL_GETTILE);
    pLevelGetData = resolver.Resolve(SYM_LEVEL_GETDATA);
    pLevelSetData = resolver.Resolve(SYM_LEVEL_SETDATA);
    pLevelClip = resolver.Resolve(SYM_LEVEL_CLIP);
    pMcRegionChunkStorageLoad = resolver.Resolve(SYM_MCREGIONCHUNKSTORAGE_LOAD);
    if (!pMcRegionChunkStorageLoad)
        pMcRegionChunkStorageLoad = resolver.ResolveExact("McRegionChunkStorage::load");
    pMcRegionChunkStorageSave = resolver.Resolve(SYM_MCREGIONCHUNKSTORAGE_SAVE);
    if (!pMcRegionChunkStorageSave)
        pMcRegionChunkStorageSave = resolver.ResolveExact("McRegionChunkStorage::save");
    pLevelSetTileAndData = resolver.Resolve(SYM_LEVEL_SETTILEANDDATA);
    pLevelHasNeighborSignal = resolver.Resolve(SYM_LEVEL_HASNEIGHBORSIGNAL);
    pLevelAddToTickNextTick = resolver.Resolve(SYM_LEVEL_ADDTOTICKNEXTTICK);
    pServerLevelAddToTickNextTick = resolver.Resolve(SYM_SERVERLEVEL_ADDTOTICKNEXTTICK);
    pLevelChunkGetTile = resolver.ResolveExact("LevelChunk::getTile");
    pLevelChunkSetTile = resolver.ResolveExact("LevelChunk::setTile");
    pLevelChunkGetData = resolver.Resolve(SYM_LEVELCHUNK_GETDATA);
    if (!pLevelChunkGetData)
        pLevelChunkGetData = resolver.ResolveExact("LevelChunk::getData");
    pLevelChunkSetTileAndData = resolver.Resolve(SYM_LEVELCHUNK_SETTILEANDDATA);
    if (!pLevelChunkSetTileAndData)
        pLevelChunkSetTileAndData = resolver.ResolveExact("LevelChunk::setTileAndData");
    pLevelChunkGetPos = resolver.ResolveExactOptional("LevelChunk::getPos");
    pLevelChunkGetHighestNonEmptyY = resolver.ResolveExactOptional("LevelChunk::getHighestNonEmptyY");
    pCompressedTileStorageSet = resolver.ResolveExact("CompressedTileStorage::set");
    return true;
}

void LevelSymbols::Log() const
{
    LogSym("Level::updateNeighborsAt", pLevelUpdateNeighborsAt);
    LogSym("ServerLevel::tickPendingTicks", pServerLevelTickPendingTicks);
    LogSym("Level::getTile", pLevelGetTile);
    LogSym("Level::getData", pLevelGetData);
    LogSym("Level::setData", pLevelSetData);
    LogSym("Level::clip", pLevelClip);
    LogSym("McRegionChunkStorage::load", pMcRegionChunkStorageLoad);
    LogSym("McRegionChunkStorage::save", pMcRegionChunkStorageSave);
    LogSym("Level::setTileAndData", pLevelSetTileAndData);
    LogSym("Level::hasNeighborSignal", pLevelHasNeighborSignal);
    LogSym("Level::addToTickNextTick", pLevelAddToTickNextTick);
    LogSym("ServerLevel::addToTickNextTick", pServerLevelAddToTickNextTick);
    LogSym("LevelChunk::getTile", pLevelChunkGetTile);
    LogSym("LevelChunk::setTile", pLevelChunkSetTile);
    LogSym("LevelChunk::getData", pLevelChunkGetData);
    LogSym("LevelChunk::setTileAndData", pLevelChunkSetTileAndData);
    LogSymOptional("LevelChunk::getPos", pLevelChunkGetPos);
    LogSymOptional("LevelChunk::getHighestNonEmptyY", pLevelChunkGetHighestNonEmptyY);
    LogSym("CompressedTileStorage::set", pCompressedTileStorageSet);
}

bool EntitySymbols::Resolve(SymbolResolver& resolver)
{
    auto resolveWithExactOptional = [&resolver](const char* decoratedName, const char* exactNameA, const char* exactNameB = nullptr) -> void*
    {
        void* ptr = resolver.ResolveOptional(decoratedName);
        if (!ptr && exactNameA && exactNameA[0])
            ptr = resolver.ResolveExactOptional(exactNameA);
        if (!ptr && exactNameB && exactNameB[0])
            ptr = resolver.ResolveExactOptional(exactNameB);
        return ptr;
    };

    pPlayerCanDestroy = resolver.Resolve(SYM_PLAYER_CANDESTROY);
    pServerPlayerGameModeUseItem = resolver.Resolve(SYM_SERVER_PLAYER_GAMEMODE_USEITEM);
    pMultiPlayerGameModeUseItem = resolver.Resolve(SYM_MULTI_PLAYER_GAMEMODE_USEITEM);
    pServerPlayerGameModeUseItemOn = resolver.Resolve(SYM_SERVER_PLAYER_GAMEMODE_USEITEMON);
    pMultiPlayerGameModeUseItemOn = resolver.Resolve(SYM_MULTI_PLAYER_GAMEMODE_USEITEMON);
    pLevelAddEntity = resolver.Resolve(SYM_LEVEL_ADDENTITY);
    pEntityPlayStepSound = resolver.Resolve(SYM_ENTITY_PLAYSTEPSOUND);
    pEntityIONewById = resolver.Resolve(SYM_ENTITYIO_NEWBYID);
    pEntityMoveTo = resolver.Resolve(SYM_ENTITY_MOVETO);
    pEntityCheckInsideTiles = resolver.Resolve(SYM_ENTITY_CHECKINSIDETILES);
    pEntitySetPos = resolver.Resolve(SYM_ENTITY_SETPOS);
    pEntityGetLookAngle = resolver.Resolve(SYM_LIVINGENTITY_GETLOOKANGLE);
    pLivingEntityGetPos = resolver.Resolve(SYM_LIVINGENTITY_GETPOS);
    if (!pLivingEntityGetPos)
        pLivingEntityGetPos = resolver.Resolve(SYM_LIVINGENTITY_GETPOS_V);
    pLivingEntityGetViewVector = resolver.Resolve(SYM_LIVINGENTITY_GETVIEWVECTOR);
    pLivingEntityPick = resolver.Resolve(SYM_LIVINGENTITY_PICK);
    if (!pEntityGetLookAngle)
        pEntityGetLookAngle = resolver.Resolve(SYM_ENTITY_GETLOOKANGLE);
    pEntityLerpMotion = resolver.Resolve(SYM_ENTITY_LERPMOTION);
    pEntitySpawnAtLocation = resolver.Resolve(SYM_ENTITY_SPAWNATLOCATION);
    if (SYM_ENTITY_SPAWNATLOCATION_INT)
        pEntitySpawnAtLocationInt = resolver.Resolve(SYM_ENTITY_SPAWNATLOCATION_INT);
    pEntityGetEncodeId = resolver.Resolve(SYM_ENTITY_GETENCODEID);
    pEntityGetEncodeIdById = resolver.ResolveOptional(SYM_ENTITY_GETENCODEID_BYID);
    pEntityGetNetworkName = resolver.Resolve(SYM_ENTITY_GETNETWORKNAME);
    pLivingEntityTickDeath = resolveWithExactOptional(SYM_LIVINGENTITY_TICKDEATH, "LivingEntity::tickDeath");
    pLivingEntityDropDeathLoot = resolveWithExactOptional(SYM_LIVINGENTITY_DROPDEATHLOOT, "LivingEntity::dropDeathLoot");
    pMobDropDeathLoot = resolveWithExactOptional(SYM_MOB_DROPDEATHLOOT, "Mob::dropDeathLoot");
    pChickenDropDeathLoot = resolveWithExactOptional(SYM_CHICKEN_DROPDEATHLOOT, "Chicken::dropDeathLoot");
    pCowDropDeathLoot = resolveWithExactOptional(SYM_COW_DROPDEATHLOOT, "Cow::dropDeathLoot");
    pPigDropDeathLoot = resolveWithExactOptional(SYM_PIG_DROPDEATHLOOT, "Pig::dropDeathLoot");
    pSheepDropDeathLoot = resolveWithExactOptional(SYM_SHEEP_DROPDEATHLOOT, "Sheep::dropDeathLoot");
    pSquidDropDeathLoot = resolveWithExactOptional(SYM_SQUID_DROPDEATHLOOT, "Squid::dropDeathLoot");
    pOcelotDropDeathLoot = resolveWithExactOptional(SYM_OCELOT_DROPDEATHLOOT, "Ocelot::dropDeathLoot", "Ozelot::dropDeathLoot");
    pSnowManDropDeathLoot = resolveWithExactOptional(SYM_SNOWMAN_DROPDEATHLOOT, "SnowMan::dropDeathLoot");
    pVillagerGolemDropDeathLoot = resolveWithExactOptional(SYM_VILLAGERGOLEM_DROPDEATHLOOT, "VillagerGolem::dropDeathLoot");
    pPigZombieDropDeathLoot = resolveWithExactOptional(SYM_PIGZOMBIE_DROPDEATHLOOT, "PigZombie::dropDeathLoot");
    pSpiderDropDeathLoot = resolveWithExactOptional(SYM_SPIDER_DROPDEATHLOOT, "Spider::dropDeathLoot");
    pSkeletonDropDeathLoot = resolveWithExactOptional(SYM_SKELETON_DROPDEATHLOOT, "Skeleton::dropDeathLoot");
    pWitchDropDeathLoot = resolveWithExactOptional(SYM_WITCH_DROPDEATHLOOT, "Witch::dropDeathLoot");
    pBlazeDropDeathLoot = resolveWithExactOptional(SYM_BLAZE_DROPDEATHLOOT, "Blaze::dropDeathLoot");
    pEnderManDropDeathLoot = resolveWithExactOptional(SYM_ENDERMAN_DROPDEATHLOOT, "EnderMan::dropDeathLoot");
    pGhastDropDeathLoot = resolveWithExactOptional(SYM_GHAST_DROPDEATHLOOT, "Ghast::dropDeathLoot");
    pLavaSlimeDropDeathLoot = resolveWithExactOptional(SYM_LAVASLIME_DROPDEATHLOOT, "LavaSlime::dropDeathLoot");
    pWitherBossDropDeathLoot = resolveWithExactOptional(SYM_WITHERBOSS_DROPDEATHLOOT, "WitherBoss::dropDeathLoot");
    return true;
}

void EntitySymbols::Log() const
{
    LogSym("Player::canDestroy", pPlayerCanDestroy);
    LogSym("ServerPlayerGameMode::useItem", pServerPlayerGameModeUseItem);
    LogSym("MultiPlayerGameMode::useItem", pMultiPlayerGameModeUseItem);
    LogSym("ServerPlayerGameMode::useItemOn", pServerPlayerGameModeUseItemOn);
    LogSym("MultiPlayerGameMode::useItemOn", pMultiPlayerGameModeUseItemOn);
    LogSym("Level::addEntity", pLevelAddEntity);
    LogSym("Entity::playStepSound", pEntityPlayStepSound);
    LogSym("EntityIO::newById", pEntityIONewById);
    LogSym("Entity::moveTo", pEntityMoveTo);
    LogSym("Entity::checkInsideTiles", pEntityCheckInsideTiles);
    LogSym("Entity::setPos", pEntitySetPos);
    LogSym("LivingEntity/Entity::getLookAngle", pEntityGetLookAngle);
    LogSym("LivingEntity::getPos", pLivingEntityGetPos);
    LogSym("LivingEntity::getViewVector", pLivingEntityGetViewVector);
    LogSym("LivingEntity::pick", pLivingEntityPick);
    LogSym("Entity::lerpMotion", pEntityLerpMotion);
    LogSym("Entity::spawnAtLocation", pEntitySpawnAtLocation);
    LogSymOptional("Entity::spawnAtLocation(int,int,float)", pEntitySpawnAtLocationInt);
    if (SYM_ENTITY_SPAWNATLOCATION_INT && !pEntitySpawnAtLocationInt)
    {
        // Provide candidates when the int overload is missing.
        PdbParser::DumpMatching("spawnAtLocation@Entity@@");
    }
    LogSym("EntityIO::getEncodeId", pEntityGetEncodeId);
    LogSym("EntityIO::getEncodeId(int)", pEntityGetEncodeIdById);
    LogSym("Entity::getNetworkName", pEntityGetNetworkName);
    LogSym("LivingEntity::tickDeath", pLivingEntityTickDeath);
    LogSym("LivingEntity::dropDeathLoot", pLivingEntityDropDeathLoot);
    LogSym("Mob::dropDeathLoot", pMobDropDeathLoot);
    LogSym("Chicken::dropDeathLoot", pChickenDropDeathLoot);
    LogSym("Cow::dropDeathLoot", pCowDropDeathLoot);
    LogSym("Pig::dropDeathLoot", pPigDropDeathLoot);
    LogSym("Sheep::dropDeathLoot", pSheepDropDeathLoot);
    LogSym("Squid::dropDeathLoot", pSquidDropDeathLoot);
    LogSym("Ocelot::dropDeathLoot", pOcelotDropDeathLoot);
    LogSym("SnowMan::dropDeathLoot", pSnowManDropDeathLoot);
    LogSym("VillagerGolem::dropDeathLoot", pVillagerGolemDropDeathLoot);
    LogSym("PigZombie::dropDeathLoot", pPigZombieDropDeathLoot);
    LogSym("Spider::dropDeathLoot", pSpiderDropDeathLoot);
    LogSym("Skeleton::dropDeathLoot", pSkeletonDropDeathLoot);
    LogSym("Witch::dropDeathLoot", pWitchDropDeathLoot);
    LogSym("Blaze::dropDeathLoot", pBlazeDropDeathLoot);
    LogSym("EnderMan::dropDeathLoot", pEnderManDropDeathLoot);
    LogSym("Ghast::dropDeathLoot", pGhastDropDeathLoot);
    LogSym("LavaSlime::dropDeathLoot", pLavaSlimeDropDeathLoot);
    LogSym("WitherBoss::dropDeathLoot", pWitherBossDropDeathLoot);
}

bool InventorySymbols::Resolve(SymbolResolver& resolver)
{
    pInventoryRemoveResource = resolver.Resolve(SYM_INVENTORY_REMOVERESOURCE);
    pInventoryVtable = resolver.Resolve(SYM_INVENTORY_VFTABLE);
    pAbstractContainerMenuBroadcastChanges = resolver.Resolve(SYM_ABSTRACTCONTAINERMENU_BROADCASTCHANGES);
    return true;
}

void InventorySymbols::Log() const
{
    LogSym("Inventory::removeResource", pInventoryRemoveResource);
    LogSym("Inventory::vftable", pInventoryVtable);
    LogSym("AbstractContainerMenu::broadcastChanges", pAbstractContainerMenuBroadcastChanges);
}

bool NbtSymbols::Resolve(SymbolResolver& resolver)
{
    pTagNewTag = resolver.Resolve(SYM_TAG_NEWTAG);
    if (!pTagNewTag)
        pTagNewTag = resolver.ResolveExact("Tag::newTag");
    return true;
}

void NbtSymbols::Log() const
{
    LogSym("Tag::newTag", pTagNewTag);
}
