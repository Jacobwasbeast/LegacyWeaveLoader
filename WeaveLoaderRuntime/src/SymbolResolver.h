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
    void* pItemInstanceMineBlock = nullptr; // ItemInstance::mineBlock(Level*,int,int,int,int,shared_ptr<Player>)
    void* pItemMineBlock = nullptr;        // Item::mineBlock(shared_ptr<ItemInstance>,Level*,int,int,int,int,shared_ptr<LivingEntity>)
    void* pDiggerItemMineBlock = nullptr;  // DiggerItem::mineBlock(shared_ptr<ItemInstance>,Level*,int,int,int,int,shared_ptr<LivingEntity>)
    void* pTexturesBindTextureResource = nullptr; // Textures::bindTexture(ResourceLocation*)
    void* pTexturesLoadTextureByName = nullptr;   // Textures::loadTexture(TEXTURE_NAME,const wstring&)
    void* pTexturesLoadTextureByIndex = nullptr;  // Textures::loadTexture(int)
    void* pStitchedGetU0 = nullptr;       // StitchedTexture::getU0(bool) const
    void* pStitchedGetU1 = nullptr;       // StitchedTexture::getU1(bool) const
    void* pStitchedGetV0 = nullptr;       // StitchedTexture::getV0(bool) const
    void* pStitchedGetV1 = nullptr;       // StitchedTexture::getV1(bool) const
    void* pTextureAtlasLocationBlocks = nullptr; // TextureAtlas::LOCATION_BLOCKS
    void* pTextureAtlasLocationItems = nullptr;  // TextureAtlas::LOCATION_ITEMS

private:
    uintptr_t m_moduleBase = 0;
    bool m_initialized = false;
};
