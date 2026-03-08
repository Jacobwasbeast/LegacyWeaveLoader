#pragma once
#include <Windows.h>

class SymbolResolver
{
public:
    bool Initialize();
    bool ResolveGameFunctions();
    void Cleanup();

    void* Resolve(const char* decoratedName);

    void* pRunStaticCtors = nullptr;       // MinecraftWorld_RunStaticCtors
    void* pMinecraftTick = nullptr;        // Minecraft::tick(bool, bool)
    void* pMinecraftInit = nullptr;        // Minecraft::init()
    void* pExitGame = nullptr;             // CConsoleMinecraftApp::ExitGame()
    void* pCreativeStaticCtor = nullptr;   // IUIScene_CreativeMenu::staticCtor()
    void* pMainMenuCustomDraw = nullptr;   // UIScene_MainMenu::customDraw()
    void* pPresent = nullptr;              // C4JRender::Present()
    void* pGetString = nullptr;            // CMinecraftApp::GetString(int)
    void* pGetResourceAsStream = nullptr;  // InputStream::getResourceAsStream(wstring)
    void* pLoadUVs = nullptr;              // PreStitchedTextureMap::loadUVs()
    void* pSimpleIconCtor = nullptr;       // SimpleIcon::SimpleIcon(wstring,wstring,float*4)
    void* pOperatorNew = nullptr;          // global operator new(size_t) - for texture injection
    void* pRegisterIcon = nullptr;         // PreStitchedTextureMap::registerIcon(const wstring&)
    void* pItemInstanceGetIcon = nullptr;  // ItemInstance::getIcon()
    void* pEntityRendererBindTextureResource = nullptr; // EntityRenderer::bindTexture(ResourceLocation*)
    void* pItemRendererRenderItemBillboard = nullptr; // ItemRenderer::renderItemBillboard(shared_ptr<ItemEntity>,Icon*,...)
    void* pCompassTextureCycleFrames = nullptr; // CompassTexture::cycleFrames()
    void* pClockTextureCycleFrames = nullptr; // ClockTexture::cycleFrames()
    void* pCompassTextureGetSourceWidth = nullptr; // CompassTexture::getSourceWidth() const
    void* pCompassTextureGetSourceHeight = nullptr; // CompassTexture::getSourceHeight() const
    void* pClockTextureGetSourceWidth = nullptr; // ClockTexture::getSourceWidth() const
    void* pClockTextureGetSourceHeight = nullptr; // ClockTexture::getSourceHeight() const
    void* pItemInstanceMineBlock = nullptr; // ItemInstance::mineBlock(Level*,int,int,int,int,shared_ptr<Player>)
    void* pItemMineBlock = nullptr;        // Item::mineBlock(shared_ptr<ItemInstance>,Level*,int,int,int,int,shared_ptr<LivingEntity>)
    void* pDiggerItemMineBlock = nullptr;  // DiggerItem::mineBlock(shared_ptr<ItemInstance>,Level*,int,int,int,int,shared_ptr<LivingEntity>)
    void* pPickaxeItemGetDestroySpeed = nullptr; // PickaxeItem::getDestroySpeed(shared_ptr<ItemInstance>,Tile*)
    void* pPickaxeItemCanDestroySpecial = nullptr; // PickaxeItem::canDestroySpecial(Tile*)
    void* pShovelItemGetDestroySpeed = nullptr; // ShovelItem::getDestroySpeed(shared_ptr<ItemInstance>,Tile*)
    void* pShovelItemCanDestroySpecial = nullptr; // ShovelItem::canDestroySpecial(Tile*)
    void* pPlayerCanDestroy = nullptr;             // Player::canDestroy(Tile*)
    void* pServerPlayerGameModeUseItem = nullptr; // ServerPlayerGameMode::useItem(shared_ptr<Player>,Level*,shared_ptr<ItemInstance>,bool)
    void* pMultiPlayerGameModeUseItem = nullptr;  // MultiPlayerGameMode::useItem(shared_ptr<Player>,Level*,shared_ptr<ItemInstance>,bool)
    void* pTexturesBindTextureResource = nullptr; // Textures::bindTexture(ResourceLocation*)
    void* pTexturesLoadTextureByName = nullptr;   // Textures::loadTexture(TEXTURE_NAME,const wstring&)
    void* pTexturesLoadTextureByIndex = nullptr;  // Textures::loadTexture(int)
    void* pStitchedGetU0 = nullptr;       // StitchedTexture::getU0(bool) const
    void* pStitchedGetU1 = nullptr;       // StitchedTexture::getU1(bool) const
    void* pStitchedGetV0 = nullptr;       // StitchedTexture::getV0(bool) const
    void* pStitchedGetV1 = nullptr;       // StitchedTexture::getV1(bool) const
    void* pMinecraftSetLevel = nullptr;   // Minecraft::setLevel(MultiPlayerLevel*,int,shared_ptr<Player>,bool,bool)
    void* pLevelAddEntity = nullptr;      // Level::addEntity(shared_ptr<Entity>)
    void* pEntityIONewById = nullptr;     // EntityIO::newById(int,Level*)
    void* pEntityMoveTo = nullptr;        // Entity::moveTo(double,double,double,float,float)
    void* pEntitySetPos = nullptr;        // Entity::setPos(double,double,double)
    void* pEntityGetLookAngle = nullptr;  // Entity::getLookAngle()
    void* pLivingEntityGetViewVector = nullptr; // LivingEntity::getViewVector(float)
    void* pEntityLerpMotion = nullptr;    // Entity::lerpMotion(double,double,double)
    void* pInventoryRemoveResource = nullptr; // Inventory::removeResource(int)
    void* pInventoryVtable = nullptr;      // Inventory vftable
    void* pItemInstanceHurtAndBreak = nullptr; // ItemInstance::hurtAndBreak(int,shared_ptr<LivingEntity>)
    void* pAbstractContainerMenuBroadcastChanges = nullptr; // AbstractContainerMenu::broadcastChanges()
    void* pTextureAtlasLocationBlocks = nullptr; // TextureAtlas::LOCATION_BLOCKS
    void* pTextureAtlasLocationItems = nullptr;  // TextureAtlas::LOCATION_ITEMS
    void* pTileTiles = nullptr;                  // Tile::tiles (Tile*[]) for tile id lookup

private:
    uintptr_t m_moduleBase = 0;
    bool m_initialized = false;
};
