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
static const char* SYM_ITEMINSTANCE_MINEBLOCK = "?mineBlock@ItemInstance@@QEAAXPEAVLevel@@HHHHV?$shared_ptr@VPlayer@@@std@@@Z";
static const char* SYM_ITEM_MINEBLOCK = "?mineBlock@Item@@UEAA_NV?$shared_ptr@VItemInstance@@@std@@PEAVLevel@@HHHHV?$shared_ptr@VLivingEntity@@@3@@Z";
static const char* SYM_DIGGERITEM_MINEBLOCK = "?mineBlock@DiggerItem@@UEAA_NV?$shared_ptr@VItemInstance@@@std@@PEAVLevel@@HHHHV?$shared_ptr@VLivingEntity@@@3@@Z";
static const char* SYM_TEXTURES_BIND_RESOURCE = "?bindTexture@Textures@@QEAAXPEAVResourceLocation@@@Z";
static const char* SYM_TEXTURES_LOAD_BY_NAME = "?loadTexture@Textures@@AEAAHW4_TEXTURE_NAME@@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z";
static const char* SYM_TEXTURES_LOAD_BY_INDEX_PUBLIC = "?loadTexture@Textures@@QEAAHH@Z";
static const char* SYM_TEXTURES_LOAD_BY_INDEX_PRIVATE = "?loadTexture@Textures@@AEAAHH@Z";
static const char* SYM_STITCHED_GETU0 = "?getU0@StitchedTexture@@UEBAM_N@Z";
static const char* SYM_STITCHED_GETU1 = "?getU1@StitchedTexture@@UEBAM_N@Z";
static const char* SYM_STITCHED_GETV0 = "?getV0@StitchedTexture@@UEBAM_N@Z";
static const char* SYM_STITCHED_GETV1 = "?getV1@StitchedTexture@@UEBAM_N@Z";
static const char* SYM_TEXATLAS_BLOCKS = "?LOCATION_BLOCKS@TextureAtlas@@2VResourceLocation@@A";
static const char* SYM_TEXATLAS_ITEMS = "?LOCATION_ITEMS@TextureAtlas@@2VResourceLocation@@A";

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
    pItemInstanceMineBlock = Resolve(SYM_ITEMINSTANCE_MINEBLOCK);
    pItemMineBlock = Resolve(SYM_ITEM_MINEBLOCK);
    pDiggerItemMineBlock = Resolve(SYM_DIGGERITEM_MINEBLOCK);
    pTexturesBindTextureResource = Resolve(SYM_TEXTURES_BIND_RESOURCE);
    pTexturesLoadTextureByName = Resolve(SYM_TEXTURES_LOAD_BY_NAME);
    pTexturesLoadTextureByIndex = Resolve(SYM_TEXTURES_LOAD_BY_INDEX_PUBLIC);
    if (!pTexturesLoadTextureByIndex)
        pTexturesLoadTextureByIndex = Resolve(SYM_TEXTURES_LOAD_BY_INDEX_PRIVATE);
    pStitchedGetU0 = Resolve(SYM_STITCHED_GETU0);
    pStitchedGetU1 = Resolve(SYM_STITCHED_GETU1);
    pStitchedGetV0 = Resolve(SYM_STITCHED_GETV0);
    pStitchedGetV1 = Resolve(SYM_STITCHED_GETV1);
    pTextureAtlasLocationBlocks = Resolve(SYM_TEXATLAS_BLOCKS);
    pTextureAtlasLocationItems = Resolve(SYM_TEXATLAS_ITEMS);
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
    logSym("ItemInstance::mineBlock", pItemInstanceMineBlock);
    logSym("Item::mineBlock", pItemMineBlock);
    logSym("DiggerItem::mineBlock", pDiggerItemMineBlock);
    logSym("Textures::bindTexture(ResourceLocation)", pTexturesBindTextureResource);
    logSym("Textures::loadTexture(TEXTURE_NAME,wstring)", pTexturesLoadTextureByName);
    logSym("Textures::loadTexture(int)", pTexturesLoadTextureByIndex);
    logSym("StitchedTexture::getU0", pStitchedGetU0);
    logSym("StitchedTexture::getU1", pStitchedGetU1);
    logSym("StitchedTexture::getV0", pStitchedGetV0);
    logSym("StitchedTexture::getV1", pStitchedGetV1);
    logSym("TextureAtlas::LOCATION_BLOCKS", pTextureAtlasLocationBlocks);
    logSym("TextureAtlas::LOCATION_ITEMS", pTextureAtlasLocationItems);

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
