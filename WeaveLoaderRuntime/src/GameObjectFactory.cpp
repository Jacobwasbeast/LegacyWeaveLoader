#include "GameObjectFactory.h"
#include "CustomSlabRegistry.h"
#include "SymbolResolver.h"
#include "PdbParser.h"
#include "LogUtil.h"
#include <Windows.h>
#include <cstring>
#include <string>
#include <unordered_map>

// Tile::Tile(int id, Material* material, bool isSolidRender) — protected ctor
typedef void (__fastcall *TileCtor_fn)(void* thisPtr, int id, void* material, bool isSolidRender);
// Tile* Tile::setDestroyTime(float) — protected virtual
typedef void* (__fastcall *TileSetFloat_fn)(void* thisPtr, float val);
// Tile* Tile::setSoundType(const SoundType*) — protected virtual
typedef void* (__fastcall *TileSetSoundType_fn)(void* thisPtr, const void* soundType);
// Tile* Tile::setIconName(const std::wstring&) — protected virtual
typedef void* (__fastcall *TileSetIconName_fn)(void* thisPtr, const std::wstring& name);
// Tile* Tile::setDescriptionId(unsigned int) — public virtual
typedef void* (__fastcall *TileSetDescriptionId_fn)(void* thisPtr, unsigned int id);
typedef void* (__fastcall *TileSetLightBlock_fn)(void* thisPtr, int value);
typedef void* (__fastcall *TileSetLightEmission_fn)(void* thisPtr, float value);

// TileItem::TileItem(int id)
typedef void (__fastcall *TileItemCtor_fn)(void* thisPtr, int id);
typedef void (__fastcall *HeavyTileCtor_fn)(void* thisPtr, int id, bool isSolidRender);

// Item::Item(int id) — protected ctor
typedef void (__fastcall *ItemCtor_fn)(void* thisPtr, int id);
// PickaxeItem::PickaxeItem(int id, const Item::Tier* tier)
typedef void (__fastcall *PickaxeCtor_fn)(void* thisPtr, int id, const void* tier);
typedef void (__fastcall *ShovelCtor_fn)(void* thisPtr, int id, const void* tier);
typedef void (__fastcall *HoeCtor_fn)(void* thisPtr, int id, const void* tier);
typedef void (__fastcall *HatchetCtor_fn)(void* thisPtr, int id, const void* tier);
typedef void (__fastcall *WeaponCtor_fn)(void* thisPtr, int id, const void* tier);
typedef void (__fastcall *StoneSlabCtor_fn)(void* thisPtr, int id, bool fullSize);
typedef void (__fastcall *WoodSlabCtor_fn)(void* thisPtr, int id, bool fullSize);
typedef void (__fastcall *StoneSlabItemCtor_fn)(void* thisPtr, int id, void* halfTile, void* fullTile, bool full);
// Item* Item::setIconName(const std::wstring&)
typedef void* (__fastcall *ItemSetIconName_fn)(void* thisPtr, const std::wstring& name);
typedef void* (__fastcall *ItemSetUseDescriptionId_fn)(void* thisPtr, unsigned int id);
// Item::getDescriptionId(int) — used to extract the descriptionId field offset
typedef unsigned int (__fastcall *ItemGetDescriptionId_fn)(void* thisPtr, int auxData);

static TileCtor_fn        fnTileCtor       = nullptr;
static TileSetFloat_fn    fnSetDestroyTime = nullptr;
static TileSetFloat_fn    fnSetExplodeable = nullptr;
static TileSetSoundType_fn fnSetSoundType  = nullptr;
static TileSetIconName_fn fnTileSetIconName= nullptr;
static TileSetDescriptionId_fn fnTileSetDescriptionId = nullptr;
static TileSetLightBlock_fn fnTileSetLightBlock = nullptr;
static TileSetLightEmission_fn fnTileSetLightEmission = nullptr;

static TileItemCtor_fn    fnTileItemCtor   = nullptr;
static HeavyTileCtor_fn   fnHeavyTileCtor  = nullptr;

static ItemCtor_fn        fnItemCtor       = nullptr;
static PickaxeCtor_fn     fnPickaxeCtor    = nullptr;
static ShovelCtor_fn      fnShovelCtor     = nullptr;
static HoeCtor_fn         fnHoeCtor        = nullptr;
static HatchetCtor_fn     fnHatchetCtor    = nullptr;
static WeaponCtor_fn      fnWeaponCtor     = nullptr;
static StoneSlabCtor_fn   fnStoneSlabCtor  = nullptr;
static WoodSlabCtor_fn    fnWoodSlabCtor   = nullptr;
static StoneSlabItemCtor_fn fnStoneSlabItemCtor = nullptr;
static ItemSetIconName_fn fnItemSetIconName= nullptr;
static ItemSetUseDescriptionId_fn fnItemSetUseDescriptionId = nullptr;
static int s_itemDescIdOffset = -1; // offset of descriptionId field in Item, extracted from getDescriptionId

// Store ADDRESSES of Material*/SoundType* statics so we can dereference lazily
// (they're NULL at resolve time because staticCtor hasn't run yet).
static void** s_materialAddrs[16] = {};
static void** s_soundAddrs[10] = {};
static void** s_tierAddrs[5] = {};

static const int TILE_ALLOC_SIZE = 1024;
static const int ITEM_ALLOC_SIZE = 1024;
static const int TILEITEM_ALLOC_SIZE = 1024;
static const int STONE_SLAB_TILE_ALLOC_SIZE = 128;
static const int WOOD_SLAB_TILE_ALLOC_SIZE = 120;
static const int STONE_SLAB_TILE_ITEM_ALLOC_SIZE = 192;

static bool s_resolved = false;
static std::unordered_map<int, void*> s_createdItems;

static int MapTierMaterial(int tier)
{
    switch (tier)
    {
        case 0: return 1; // wood
        case 1: return 2; // stone
        case 2: return 3; // iron
        case 3: return 5; // diamond
        case 4: return 4; // gold
        default: return 5;
    }
}

static void* GetMaterial(int idx)
{
    if (idx < 0 || idx >= 16 || !s_materialAddrs[idx]) return nullptr;
    return *s_materialAddrs[idx];
}

static void* GetSound(int idx)
{
    if (idx < 0 || idx >= 10 || !s_soundAddrs[idx]) return nullptr;
    return *s_soundAddrs[idx];
}

static const void* GetTier(int idx)
{
    if (idx < 0 || idx >= 5 || !s_tierAddrs[idx]) return nullptr;
    return *s_tierAddrs[idx];
}

namespace GameObjectFactory
{

bool ResolveSymbols(SymbolResolver& resolver)
{
    LogUtil::Log("[WeaveLoader] GameObjectFactory: resolving symbols...");

    // Tile constructor — protected (IEAA not QEAA)
    fnTileCtor = (TileCtor_fn)resolver.Resolve("??0Tile@@IEAA@HPEAVMaterial@@_N@Z");

    // Tile setters — protected virtual (MEAA not UEAA)
    fnSetDestroyTime = (TileSetFloat_fn)resolver.Resolve(
        "?setDestroyTime@Tile@@MEAAPEAV1@M@Z");
    fnSetExplodeable = (TileSetFloat_fn)resolver.Resolve(
        "?setExplodeable@Tile@@MEAAPEAV1@M@Z");
    fnSetSoundType = (TileSetSoundType_fn)resolver.Resolve(
        "?setSoundType@Tile@@MEAAPEAV1@PEBVSoundType@1@@Z");
    fnTileSetIconName = (TileSetIconName_fn)resolver.Resolve(
        "?setIconName@Tile@@MEAAPEAV1@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z");
    fnTileSetDescriptionId = (TileSetDescriptionId_fn)resolver.Resolve(
        "?setDescriptionId@Tile@@UEAAPEAV1@I@Z");
    fnTileSetLightBlock = (TileSetLightBlock_fn)resolver.Resolve(
        "?setLightBlock@Tile@@MEAAPEAV1@H@Z");
    fnTileSetLightEmission = (TileSetLightEmission_fn)resolver.Resolve(
        "?setLightEmission@Tile@@MEAAPEAV1@M@Z");

    // TileItem constructor
    fnTileItemCtor = (TileItemCtor_fn)resolver.Resolve("??0TileItem@@QEAA@H@Z");
    fnHeavyTileCtor = (HeavyTileCtor_fn)resolver.Resolve("??0HeavyTile@@QEAA@H_N@Z");

    // Item constructor — protected (IEAA not QEAA)
    fnItemCtor = (ItemCtor_fn)resolver.Resolve("??0Item@@IEAA@H@Z");
    fnPickaxeCtor = (PickaxeCtor_fn)resolver.Resolve("??0PickaxeItem@@QEAA@HPEBVTier@Item@@@Z");
    fnShovelCtor = (ShovelCtor_fn)resolver.Resolve("??0ShovelItem@@QEAA@HPEBVTier@Item@@@Z");
    fnHoeCtor = (HoeCtor_fn)resolver.Resolve("??0HoeItem@@QEAA@HPEBVTier@Item@@@Z");
    fnHatchetCtor = (HatchetCtor_fn)resolver.Resolve("??0HatchetItem@@QEAA@HPEBVTier@Item@@@Z");
    fnWeaponCtor = (WeaponCtor_fn)resolver.Resolve("??0WeaponItem@@QEAA@HPEBVTier@Item@@@Z");
    fnStoneSlabCtor = (StoneSlabCtor_fn)resolver.Resolve("??0StoneSlabTile@@QEAA@H_N@Z");
    fnWoodSlabCtor = (WoodSlabCtor_fn)resolver.Resolve("??0WoodSlabTile@@QEAA@H_N@Z");
    fnStoneSlabItemCtor = (StoneSlabItemCtor_fn)resolver.Resolve("??0StoneSlabTileItem@@QEAA@HPEAVHalfSlabTile@@0_N@Z");

    // Item::setIconName
    fnItemSetIconName = (ItemSetIconName_fn)resolver.Resolve(
        "?setIconName@Item@@QEAAPEAV1@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z");
    fnItemSetUseDescriptionId = (ItemSetUseDescriptionId_fn)resolver.Resolve(
        "?setUseDescriptionId@Item@@QEAAPEAV1@I@Z");
    // Item::setDescriptionId is inlined — extract the field offset from getDescriptionId instead.
    // getDescriptionId(int) is "mov eax, [rcx+offset]; ret" so we parse the offset from its opcodes.
    void* fnItemGetDescId = resolver.Resolve("?getDescriptionId@Item@@UEAAIH@Z");
    if (fnItemGetDescId)
    {
        const uint8_t* code = static_cast<const uint8_t*>(fnItemGetDescId);
        if (code[0] == 0x8B && code[1] == 0x41)
        {
            // mov eax, [rcx+disp8]  —  8B 41 XX
            s_itemDescIdOffset = static_cast<int>(code[2]);
            LogUtil::Log("[WeaveLoader] Item descriptionId offset = 0x%X (from getDescriptionId disp8)", s_itemDescIdOffset);
        }
        else if (code[0] == 0x8B && code[1] == 0x81)
        {
            // mov eax, [rcx+disp32]  —  8B 81 XX XX XX XX
            s_itemDescIdOffset = *reinterpret_cast<const int*>(code + 2);
            LogUtil::Log("[WeaveLoader] Item descriptionId offset = 0x%X (from getDescriptionId disp32)", s_itemDescIdOffset);
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Item::getDescriptionId has unexpected opcode pattern: %02X %02X %02X",
                         code[0], code[1], code[2]);
        }
    }
    else
    {
        LogUtil::Log("[WeaveLoader] MISSING: Item::getDescriptionId — cannot set item display names");
    }

    // Resolve Material* static pointer ADDRESSES (values are NULL until staticCtor runs)
    auto resolveMat = [&](int idx, const char* sym) {
        s_materialAddrs[idx] = (void**)resolver.Resolve(sym);
    };
    resolveMat(0,  "?air@Material@@2PEAV1@EA");
    resolveMat(1,  "?stone@Material@@2PEAV1@EA");
    resolveMat(2,  "?wood@Material@@2PEAV1@EA");
    resolveMat(3,  "?cloth@Material@@2PEAV1@EA");
    resolveMat(4,  "?plant@Material@@2PEAV1@EA");
    resolveMat(5,  "?dirt@Material@@2PEAV1@EA");
    resolveMat(6,  "?sand@Material@@2PEAV1@EA");
    resolveMat(7,  "?glass@Material@@2PEAV1@EA");
    resolveMat(8,  "?water@Material@@2PEAV1@EA");
    resolveMat(9,  "?lava@Material@@2PEAV1@EA");
    resolveMat(10, "?ice@Material@@2PEAV1@EA");
    resolveMat(11, "?metal@Material@@2PEAV1@EA");
    resolveMat(12, "?snow@Material@@2PEAV1@EA");
    resolveMat(13, "?clay@Material@@2PEAV1@EA");
    resolveMat(14, "?explosive@Material@@2PEAV1@EA");
    resolveMat(15, "?web@Material@@2PEAV1@EA");

    // Resolve SoundType* static pointer ADDRESSES
    auto resolveSound = [&](int idx, const char* sym) {
        s_soundAddrs[idx] = (void**)resolver.Resolve(sym);
    };
    resolveSound(0, "?SOUND_NORMAL@Tile@@2PEAVSoundType@1@EA");
    resolveSound(1, "?SOUND_STONE@Tile@@2PEAVSoundType@1@EA");
    resolveSound(2, "?SOUND_WOOD@Tile@@2PEAVSoundType@1@EA");
    resolveSound(3, "?SOUND_GRAVEL@Tile@@2PEAVSoundType@1@EA");
    resolveSound(4, "?SOUND_GRASS@Tile@@2PEAVSoundType@1@EA");
    resolveSound(5, "?SOUND_METAL@Tile@@2PEAVSoundType@1@EA");
    resolveSound(6, "?SOUND_GLASS@Tile@@2PEAVSoundType@1@EA");
    resolveSound(7, "?SOUND_CLOTH@Tile@@2PEAVSoundType@1@EA");
    resolveSound(8, "?SOUND_SAND@Tile@@2PEAVSoundType@1@EA");
    resolveSound(9, "?SOUND_SNOW@Tile@@2PEAVSoundType@1@EA");

    // Resolve Item::Tier static pointer ADDRESSES
    s_tierAddrs[0] = (void**)resolver.Resolve("?WOOD@Tier@Item@@2PEBV12@EB");
    s_tierAddrs[1] = (void**)resolver.Resolve("?STONE@Tier@Item@@2PEBV12@EB");
    s_tierAddrs[2] = (void**)resolver.Resolve("?IRON@Tier@Item@@2PEBV12@EB");
    s_tierAddrs[3] = (void**)resolver.Resolve("?DIAMOND@Tier@Item@@2PEBV12@EB");
    s_tierAddrs[4] = (void**)resolver.Resolve("?GOLD@Tier@Item@@2PEBV12@EB");

    auto logSym = [](const char* name, void* ptr) {
        if (ptr) LogUtil::Log("[WeaveLoader] GOF %-20s @ %p", name, ptr);
        else     LogUtil::Log("[WeaveLoader] GOF MISSING: %s", name);
    };

    logSym("Tile::Tile",         (void*)fnTileCtor);
    logSym("setDestroyTime",     (void*)fnSetDestroyTime);
    logSym("setExplodeable",     (void*)fnSetExplodeable);
    logSym("setSoundType",       (void*)fnSetSoundType);
    logSym("Tile::setIconName",  (void*)fnTileSetIconName);
    logSym("Tile::setLightBlock", (void*)fnTileSetLightBlock);
    logSym("Tile::setLightEmission", (void*)fnTileSetLightEmission);
    logSym("TileItem::TileItem", (void*)fnTileItemCtor);
    logSym("HeavyTile::HeavyTile", (void*)fnHeavyTileCtor);
    logSym("Item::Item",         (void*)fnItemCtor);
    logSym("PickaxeItem::PickaxeItem", (void*)fnPickaxeCtor);
    logSym("ShovelItem::ShovelItem", (void*)fnShovelCtor);
    logSym("HoeItem::HoeItem", (void*)fnHoeCtor);
    logSym("HatchetItem::HatchetItem", (void*)fnHatchetCtor);
    logSym("WeaponItem::WeaponItem", (void*)fnWeaponCtor);
    logSym("StoneSlabTile::StoneSlabTile", (void*)fnStoneSlabCtor);
    logSym("WoodSlabTile::WoodSlabTile", (void*)fnWoodSlabCtor);
    logSym("StoneSlabTileItem::StoneSlabTileItem", (void*)fnStoneSlabItemCtor);
    logSym("Item::setIconName",  (void*)fnItemSetIconName);
    logSym("Item::setUseDescriptionId", (void*)fnItemSetUseDescriptionId);
    logSym("Material::stone addr", (void*)s_materialAddrs[1]);
    logSym("SOUND_STONE addr",     (void*)s_soundAddrs[1]);

    // Diagnostics for missing symbols
    if (!fnTileCtor)        PdbParser::DumpMatching("??0Tile@@");
    if (!fnTileItemCtor)    PdbParser::DumpMatching("??0TileItem@@");
    if (!fnItemCtor)        PdbParser::DumpMatching("??0Item@@");
    if (!fnSetDestroyTime)  PdbParser::DumpMatching("setDestroyTime@Tile");
    if (!fnSetExplodeable)  PdbParser::DumpMatching("setExplodeable@Tile");
    if (!fnSetSoundType)    PdbParser::DumpMatching("setSoundType@Tile");
    if (!fnTileSetIconName) PdbParser::DumpMatching("setIconName@Tile");
    if (!fnItemSetIconName) PdbParser::DumpMatching("setIconName@Item");
    if (!s_materialAddrs[1]) PdbParser::DumpMatching("stone@Material");
    if (!s_soundAddrs[1])    PdbParser::DumpMatching("SOUND_STONE@Tile");

    s_resolved = fnTileCtor && fnTileItemCtor && fnItemCtor;

    if (s_resolved)
        LogUtil::Log("[WeaveLoader] GameObjectFactory: core symbols resolved OK");
    else
        LogUtil::Log("[WeaveLoader] GameObjectFactory: MISSING core symbols -- block/item creation disabled");

    return s_resolved;
}

static void ApplyTileCommon(
    void* tile,
    float hardness,
    float resistance,
    int soundType,
    const wchar_t* iconName,
    float lightEmission,
    int lightBlock,
    int descriptionId)
{
    if (fnSetDestroyTime)
        fnSetDestroyTime(tile, hardness);

    if (fnSetExplodeable)
        fnSetExplodeable(tile, resistance);

    void* sound = GetSound(soundType);
    if (fnSetSoundType && sound)
        fnSetSoundType(tile, sound);

    if (fnTileSetLightEmission)
        fnTileSetLightEmission(tile, lightEmission);

    if (fnTileSetLightBlock)
        fnTileSetLightBlock(tile, lightBlock);

    if (fnTileSetIconName && iconName)
    {
        std::wstring name(iconName);
        fnTileSetIconName(tile, name);
    }

    if (fnTileSetDescriptionId && descriptionId >= 0)
        fnTileSetDescriptionId(tile, static_cast<unsigned int>(descriptionId));
}

bool CreateTile(int tileId, int materialType, float hardness, float resistance,
                int soundType, const wchar_t* iconName, float lightEmission, int lightBlock, int descriptionId)
{
    if (!s_resolved || !fnTileCtor)
    {
        LogUtil::Log("[WeaveLoader] CreateTile: symbols not resolved");
        return false;
    }

    // Read material pointer lazily (staticCtor has run by the time mods call this)
    void* mat = GetMaterial(materialType);
    if (!mat)
        mat = GetMaterial(1); // stone fallback

    if (!mat)
    {
        LogUtil::Log("[WeaveLoader] CreateTile: no material pointer for type %d", materialType);
        return false;
    }

    void* tile = ::operator new(TILE_ALLOC_SIZE);
    memset(tile, 0, TILE_ALLOC_SIZE);
    fnTileCtor(tile, tileId, mat, true);
    ApplyTileCommon(tile, hardness, resistance, soundType, iconName, lightEmission, lightBlock, descriptionId);

    LogUtil::Log("[WeaveLoader] Created Tile id=%d (material=%d, icon=%ls, descId=%d)", tileId, materialType,
                 iconName ? iconName : L"<none>", descriptionId);

    // Create the corresponding TileItem so the block can appear in inventory.
    // TileItem(tileId - 256) -> Item::Item(tileId - 256) -> id = tileId, items[tileId] = this
    if (fnTileItemCtor)
    {
        void* tileItem = ::operator new(TILEITEM_ALLOC_SIZE);
        memset(tileItem, 0, TILEITEM_ALLOC_SIZE);
        fnTileItemCtor(tileItem, tileId - 256);
        LogUtil::Log("[WeaveLoader] Created TileItem for tile %d", tileId);
    }

    return true;
}

bool CreateManagedTile(int tileId, int materialType, float hardness, float resistance,
                       int soundType, const wchar_t* iconName, float lightEmission, int lightBlock, int descriptionId)
{
    return CreateTile(tileId, materialType, hardness, resistance, soundType, iconName, lightEmission, lightBlock, descriptionId);
}

bool CreateFallingTile(int tileId, int materialType, float hardness, float resistance,
                       int soundType, const wchar_t* iconName, float lightEmission, int lightBlock, int descriptionId)
{
    if (!s_resolved || !fnHeavyTileCtor)
    {
        LogUtil::Log("[WeaveLoader] CreateFallingTile: symbols not resolved");
        return false;
    }

    void* tile = ::operator new(TILE_ALLOC_SIZE);
    memset(tile, 0, TILE_ALLOC_SIZE);
    fnHeavyTileCtor(tile, tileId, true);
    ApplyTileCommon(tile, hardness, resistance, soundType, iconName, lightEmission, lightBlock, descriptionId);

    if (fnTileItemCtor)
    {
        void* tileItem = ::operator new(TILEITEM_ALLOC_SIZE);
        memset(tileItem, 0, TILEITEM_ALLOC_SIZE);
        fnTileItemCtor(tileItem, tileId - 256);
    }

    LogUtil::Log("[WeaveLoader] Created FallingTile id=%d (icon=%ls, descId=%d)",
        tileId, iconName ? iconName : L"<none>", descriptionId);
    return true;
}

bool CreateSlabPair(int halfTileId, int fullTileId, int materialType, float hardness, float resistance,
                    int soundType, const wchar_t* iconName, float lightEmission, int lightBlock, int descriptionId)
{
    if (!s_resolved || !fnStoneSlabItemCtor || (!fnStoneSlabCtor && !fnWoodSlabCtor))
    {
        LogUtil::Log("[WeaveLoader] CreateSlabPair: symbols not resolved");
        return false;
    }

    const bool woodFamily = (materialType == 2);
    if ((woodFamily && !fnWoodSlabCtor) || (!woodFamily && !fnStoneSlabCtor))
    {
        LogUtil::Log("[WeaveLoader] CreateSlabPair: missing %s slab ctor", woodFamily ? "wood" : "stone");
        return false;
    }
    const int tileAllocSize = woodFamily ? WOOD_SLAB_TILE_ALLOC_SIZE : STONE_SLAB_TILE_ALLOC_SIZE;

    void* halfTile = ::operator new(tileAllocSize);
    void* fullTile = ::operator new(tileAllocSize);
    memset(halfTile, 0, tileAllocSize);
    memset(fullTile, 0, tileAllocSize);

    if (woodFamily)
    {
        fnWoodSlabCtor(halfTile, halfTileId, false);
        fnWoodSlabCtor(fullTile, fullTileId, true);
    }
    else
    {
        fnStoneSlabCtor(halfTile, halfTileId, false);
        fnStoneSlabCtor(fullTile, fullTileId, true);
    }

    ApplyTileCommon(halfTile, hardness, resistance, soundType, iconName, lightEmission, lightBlock, descriptionId);
    ApplyTileCommon(fullTile, hardness, resistance, soundType, iconName, lightEmission, lightBlock, descriptionId);

    void* slabItem = ::operator new(STONE_SLAB_TILE_ITEM_ALLOC_SIZE);
    memset(slabItem, 0, STONE_SLAB_TILE_ITEM_ALLOC_SIZE);
    fnStoneSlabItemCtor(slabItem, halfTileId - 256, halfTile, fullTile, false);

    if (fnItemSetIconName)
    {
        std::wstring name = (iconName && iconName[0]) ? iconName : L"MISSING_ICON_ITEM";
        fnItemSetIconName(slabItem, name);
    }

    if (s_itemDescIdOffset > 0 && descriptionId >= 0)
    {
        *reinterpret_cast<unsigned int*>(static_cast<char*>(slabItem) + s_itemDescIdOffset) =
            static_cast<unsigned int>(descriptionId);
    }

    if (fnItemSetUseDescriptionId && descriptionId >= 0)
    {
        fnItemSetUseDescriptionId(slabItem, static_cast<unsigned int>(descriptionId));
    }

    CustomSlabRegistry::Register(halfTileId, fullTileId, descriptionId, woodFamily, iconName);

    LogUtil::Log("[WeaveLoader] Created SlabPair half=%d full=%d family=%s (icon=%ls, descId=%d)",
        halfTileId, fullTileId, woodFamily ? "wood" : "stone", iconName ? iconName : L"<none>", descriptionId);
    return true;
}

bool CreateItem(int itemId, int maxStackSize, int maxDamage, const wchar_t* iconName, int descriptionId)
{
    if (!s_resolved || !fnItemCtor)
    {
        LogUtil::Log("[WeaveLoader] CreateItem: symbols not resolved");
        return false;
    }

    int ctorParam = itemId - 256;

    void* item = ::operator new(ITEM_ALLOC_SIZE);
    memset(item, 0, ITEM_ALLOC_SIZE);
    fnItemCtor(item, ctorParam);

    // Verified from Item::Item disassembly:
    // +0x24 = maxStackSize, +0x28 = maxDamage.
    if (maxStackSize > 0)
    {
        *reinterpret_cast<int*>(static_cast<char*>(item) + 0x24) = maxStackSize;
    }
    if (maxDamage > 0)
    {
        *reinterpret_cast<int*>(static_cast<char*>(item) + 0x28) = maxDamage;
    }

    // The game calls __debugbreak() if registerIcons is called with an empty
    // m_textureName, so always set a non-empty icon name.
    if (fnItemSetIconName)
    {
        std::wstring name = (iconName && iconName[0]) ? iconName : L"MISSING_ICON_ITEM";
        fnItemSetIconName(item, name);
    }

    if (s_itemDescIdOffset > 0 && descriptionId >= 0)
    {
        *reinterpret_cast<unsigned int*>(static_cast<char*>(item) + s_itemDescIdOffset) =
            static_cast<unsigned int>(descriptionId);
    }

    LogUtil::Log("[WeaveLoader] Created Item id=%d (ctorParam=%d, stack=%d, damage=%d, icon=%ls, descId=%d)",
                 itemId, ctorParam, maxStackSize, maxDamage,
                 iconName ? iconName : L"<none>", descriptionId);

    s_createdItems[itemId] = item;
    return true;
}

bool CreatePickaxeItem(int itemId, int tier, int maxDamage, const wchar_t* iconName, int descriptionId)
{
    if (!s_resolved || !fnPickaxeCtor)
    {
        LogUtil::Log("[WeaveLoader] CreatePickaxeItem: symbols not resolved");
        return false;
    }

    const void* tierPtr = GetTier(tier);
    if (!tierPtr)
    {
        LogUtil::Log("[WeaveLoader] CreatePickaxeItem: invalid tier %d", tier);
        return false;
    }

    int ctorParam = itemId - 256;
    void* item = ::operator new(ITEM_ALLOC_SIZE);
    memset(item, 0, ITEM_ALLOC_SIZE);
    fnPickaxeCtor(item, ctorParam, tierPtr);

    // Ensure pickaxe category/material for crafting menus:
    // baseType=pickaxe(3), material depends on tier.
    *reinterpret_cast<int*>(static_cast<char*>(item) + 0x38) = 3;
    *reinterpret_cast<int*>(static_cast<char*>(item) + 0x3C) = MapTierMaterial(tier);

    // Tools should always stack to 1.
    *reinterpret_cast<int*>(static_cast<char*>(item) + 0x24) = 1;

    // Optionally override tier durability.
    if (maxDamage > 0)
    {
        *reinterpret_cast<int*>(static_cast<char*>(item) + 0x28) = maxDamage;
    }

    if (fnItemSetIconName)
    {
        std::wstring name = (iconName && iconName[0]) ? iconName : L"MISSING_ICON_ITEM";
        fnItemSetIconName(item, name);
    }

    if (s_itemDescIdOffset > 0 && descriptionId >= 0)
    {
        *reinterpret_cast<unsigned int*>(static_cast<char*>(item) + s_itemDescIdOffset) =
            static_cast<unsigned int>(descriptionId);
    }

    LogUtil::Log("[WeaveLoader] Created PickaxeItem id=%d (ctorParam=%d, tier=%d, damage=%d, icon=%ls, descId=%d)",
                 itemId, ctorParam, tier, maxDamage, iconName ? iconName : L"<none>", descriptionId);
    s_createdItems[itemId] = item;
    return true;
}

static bool CreateTieredItem(
    const char* typeName,
    void* ctorRaw,
    int baseType,
    int itemId,
    int tier,
    int maxDamage,
    const wchar_t* iconName,
    int descriptionId)
{
    if (!s_resolved || !ctorRaw)
    {
        LogUtil::Log("[WeaveLoader] %s: symbols not resolved", typeName);
        return false;
    }

    const void* tierPtr = GetTier(tier);
    if (!tierPtr)
    {
        LogUtil::Log("[WeaveLoader] %s: invalid tier %d", typeName, tier);
        return false;
    }

    int ctorParam = itemId - 256;
    void* item = ::operator new(ITEM_ALLOC_SIZE);
    memset(item, 0, ITEM_ALLOC_SIZE);
    reinterpret_cast<PickaxeCtor_fn>(ctorRaw)(item, ctorParam, tierPtr);

    *reinterpret_cast<int*>(static_cast<char*>(item) + 0x38) = baseType;
    *reinterpret_cast<int*>(static_cast<char*>(item) + 0x3C) = MapTierMaterial(tier);
    *reinterpret_cast<int*>(static_cast<char*>(item) + 0x24) = 1;

    if (maxDamage > 0)
        *reinterpret_cast<int*>(static_cast<char*>(item) + 0x28) = maxDamage;

    if (fnItemSetIconName)
    {
        std::wstring name = (iconName && iconName[0]) ? iconName : L"MISSING_ICON_ITEM";
        fnItemSetIconName(item, name);
    }

    if (s_itemDescIdOffset > 0 && descriptionId >= 0)
    {
        *reinterpret_cast<unsigned int*>(static_cast<char*>(item) + s_itemDescIdOffset) =
            static_cast<unsigned int>(descriptionId);
    }

    LogUtil::Log("[WeaveLoader] Created %s id=%d (ctorParam=%d, tier=%d, damage=%d, icon=%ls, descId=%d)",
                 typeName, itemId, ctorParam, tier, maxDamage, iconName ? iconName : L"<none>", descriptionId);
    s_createdItems[itemId] = item;
    return true;
}

bool CreateShovelItem(int itemId, int tier, int maxDamage, const wchar_t* iconName, int descriptionId)
{
    return CreateTieredItem("ShovelItem", fnShovelCtor, 2, itemId, tier, maxDamage, iconName, descriptionId);
}

bool CreateHoeItem(int itemId, int tier, int maxDamage, const wchar_t* iconName, int descriptionId)
{
    return CreateTieredItem("HoeItem", fnHoeCtor, 5, itemId, tier, maxDamage, iconName, descriptionId);
}

bool CreateAxeItem(int itemId, int tier, int maxDamage, const wchar_t* iconName, int descriptionId)
{
    return CreateTieredItem("HatchetItem", fnHatchetCtor, 4, itemId, tier, maxDamage, iconName, descriptionId);
}

bool CreateSwordItem(int itemId, int tier, int maxDamage, const wchar_t* iconName, int descriptionId)
{
    return CreateTieredItem("WeaponItem", fnWeaponCtor, 1, itemId, tier, maxDamage, iconName, descriptionId);
}

void* FindItem(int itemId)
{
    const auto it = s_createdItems.find(itemId);
    if (it == s_createdItems.end())
        return nullptr;
    return it->second;
}

} // namespace GameObjectFactory
