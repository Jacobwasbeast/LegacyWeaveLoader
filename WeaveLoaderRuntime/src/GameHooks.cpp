#include "GameHooks.h"
#include "DotNetHost.h"
#include "CreativeInventory.h"
#include "MainMenuOverlay.h"
#include "ModStrings.h"
#include "ModAtlas.h"
#include "NativeExports.h"
#include "CustomPickaxeRegistry.h"
#include "CustomToolMaterialRegistry.h"
#include "CustomBlockRegistry.h"
#include "ManagedBlockRegistry.h"
#include "CustomSlabRegistry.h"
#include "LogUtil.h"
#include "WorldIdRemap.h"
#include <Windows.h>
#include <string>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <cctype>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <cstddef>
#include <vector>

namespace GameHooks
{
    RunStaticCtors_fn     Original_RunStaticCtors = nullptr;
    MinecraftTick_fn      Original_MinecraftTick = nullptr;
    MinecraftInit_fn      Original_MinecraftInit = nullptr;
    ExitGame_fn           Original_ExitGame = nullptr;
    CreativeStaticCtor_fn Original_CreativeStaticCtor = nullptr;
    MainMenuCustomDraw_fn Original_MainMenuCustomDraw = nullptr;
    Present_fn            Original_Present = nullptr;
    OutputDebugStringA_fn Original_OutputDebugStringA = nullptr;
    GetString_fn          Original_GetString = nullptr;
    GetResourceAsStream_fn Original_GetResourceAsStream = nullptr;
    LoadUVs_fn             Original_LoadUVs = nullptr;
    PreStitchedTextureMapStitch_fn Original_PreStitchedTextureMapStitch = nullptr;
    RegisterIcon_fn        Original_RegisterIcon = nullptr;
    ItemInstanceGetIcon_fn Original_ItemInstanceGetIcon = nullptr;
    EntityRendererBindTextureResource_fn Original_EntityRendererBindTextureResource = nullptr;
    ItemRendererRenderItemBillboard_fn Original_ItemRendererRenderItemBillboard = nullptr;
    AnimatedTextureCycleFrames_fn Original_CompassTextureCycleFrames = nullptr;
    AnimatedTextureCycleFrames_fn Original_ClockTextureCycleFrames = nullptr;
    TextureGetSourceDim_fn Original_CompassTextureGetSourceWidth = nullptr;
    TextureGetSourceDim_fn Original_CompassTextureGetSourceHeight = nullptr;
    TextureGetSourceDim_fn Original_ClockTextureGetSourceWidth = nullptr;
    TextureGetSourceDim_fn Original_ClockTextureGetSourceHeight = nullptr;
    ItemInstanceMineBlock_fn Original_ItemInstanceMineBlock = nullptr;
    ItemInstanceSave_fn Original_ItemInstanceSave = nullptr;
    ItemInstanceLoad_fn Original_ItemInstanceLoad = nullptr;
    ItemMineBlock_fn       Original_ItemMineBlock = nullptr;
    ItemMineBlock_fn       Original_DiggerItemMineBlock = nullptr;
    PickaxeGetDestroySpeed_fn Original_PickaxeItemGetDestroySpeed = nullptr;
    PickaxeCanDestroySpecial_fn Original_PickaxeItemCanDestroySpecial = nullptr;
    PickaxeGetDestroySpeed_fn Original_ShovelItemGetDestroySpeed = nullptr;
    PickaxeCanDestroySpecial_fn Original_ShovelItemCanDestroySpecial = nullptr;
    TileOnPlace_fn Original_TileOnPlace = nullptr;
    TileNeighborChanged_fn Original_TileNeighborChanged = nullptr;
    TileTick_fn Original_TileTick = nullptr;
    LevelSetTileAndDataDispatch_fn Original_LevelSetTileAndData = nullptr;
    LevelSetDataDispatch_fn Original_LevelSetData = nullptr;
    LevelUpdateNeighborsAtDispatch_fn Original_LevelUpdateNeighborsAt = nullptr;
    ServerLevelTickPendingTicks_fn Original_ServerLevelTickPendingTicks = nullptr;
    TileGetResource_fn Original_TileGetResource = nullptr;
    McRegionChunkStorageLoad_fn Original_McRegionChunkStorageLoad = nullptr;
    McRegionChunkStorageSave_fn Original_McRegionChunkStorageSave = nullptr;
    TileCloneTileId_fn Original_TileCloneTileId = nullptr;
    TileGetTextureFaceData_fn Original_StoneSlabGetTexture = nullptr;
    TileGetTextureFaceData_fn Original_WoodSlabGetTexture = nullptr;
    TileGetResource_fn Original_StoneSlabGetResource = nullptr;
    TileGetResource_fn Original_WoodSlabGetResource = nullptr;
    TileGetDescriptionId_fn Original_StoneSlabGetDescriptionId = nullptr;
    TileGetDescriptionId_fn Original_WoodSlabGetDescriptionId = nullptr;
    TileGetAuxName_fn Original_StoneSlabGetAuxName = nullptr;
    TileGetAuxName_fn Original_WoodSlabGetAuxName = nullptr;
    TileRegisterIcons_fn Original_StoneSlabRegisterIcons = nullptr;
    TileRegisterIcons_fn Original_WoodSlabRegisterIcons = nullptr;
    StoneSlabItemGetIcon_fn Original_StoneSlabItemGetIcon = nullptr;
    StoneSlabItemGetDescriptionId_fn Original_StoneSlabItemGetDescriptionId = nullptr;
    TileCloneTileId_fn Original_HalfSlabCloneTileId = nullptr;
    PlayerCanDestroy_fn Original_PlayerCanDestroy = nullptr;
    GameModeUseItem_fn     Original_ServerPlayerGameModeUseItem = nullptr;
    GameModeUseItem_fn     Original_MultiPlayerGameModeUseItem = nullptr;
    MinecraftSetLevel_fn   Original_MinecraftSetLevel = nullptr;
    TexturesBindTextureResource_fn Original_TexturesBindTextureResource = nullptr;
    TexturesLoadTextureByName_fn Original_TexturesLoadTextureByName = nullptr;
    TexturesLoadTextureByIndex_fn Original_TexturesLoadTextureByIndex = nullptr;
    TexturesReadImage_fn Original_TexturesReadImage = nullptr;
    StitchedTextureUV_fn   Original_StitchedGetU0 = nullptr;
    StitchedTextureUV_fn   Original_StitchedGetU1 = nullptr;
    StitchedTextureUV_fn   Original_StitchedGetV0 = nullptr;
    StitchedTextureUV_fn   Original_StitchedGetV1 = nullptr;
    BufferedImageCtorFile_fn Original_BufferedImageCtorFile = nullptr;
    BufferedImageCtorDLCPack_fn Original_BufferedImageCtorDLCPack = nullptr;
    TextureManagerCreateTexture_fn Original_TextureManagerCreateTexture = nullptr;
    TextureTransferFromImage_fn Original_TextureTransferFromImage = nullptr;
    TexturePackGetImageResource_fn Original_AbstractTexturePackGetImageResource = nullptr;
    TexturePackGetImageResource_fn Original_DLCTexturePackGetImageResource = nullptr;
    static int s_itemMineBlockHookCalls = 0;
    static void* s_currentLevel = nullptr;
    static thread_local void* s_activeUseLevel = nullptr;
    static LevelAddEntity_fn s_levelAddEntity = nullptr;
    static EntityIONewById_fn s_entityIoNewById = nullptr;
    static EntityMoveTo_fn s_entityMoveTo = nullptr;
    static EntitySetPos_fn s_entitySetPos = nullptr;
    static EntityGetLookAngle_fn s_entityGetLookAngle = nullptr;
    static LivingEntityGetViewVector_fn s_livingEntityGetViewVector = nullptr;
    static EntityLerpMotion_fn s_entityLerpMotion = nullptr;
    static InventoryRemoveResource_fn s_inventoryRemoveResource = nullptr;
    static void* s_inventoryVtable = nullptr;
    static ItemInstanceHurtAndBreak_fn s_itemInstanceHurtAndBreak = nullptr;
    static std::string s_modsPath;
    static std::unordered_map<std::string, std::string> s_modAssetRoots;
    static bool s_modAssetsIndexed = false;
    static std::mutex s_modAssetsMutex;
    // Verified from compiled Player::inventory accesses in this game build.
    static constexpr ptrdiff_t kPlayerInventoryOffset = 0x340;
    static constexpr ptrdiff_t kLevelIsClientSideOffset = 0x268;
    static constexpr ptrdiff_t kItemIdOffset = 0x20;
    static constexpr ptrdiff_t kTileIdOffset = 0x28;
    static constexpr ptrdiff_t kTileIconOffset = 0x78;
    static constexpr ptrdiff_t kEntityXOffset = 0x78;
    static constexpr ptrdiff_t kEntityYOffset = 0x80;
    static constexpr ptrdiff_t kEntityZOffset = 0x88;
    static constexpr ptrdiff_t kEntityRemovedOffset = 0xC7;
    static constexpr ptrdiff_t kFireballOwnerOffset = 0x1D0;
    static constexpr ptrdiff_t kFireballXPowerOffset = 0x1E8;
    static constexpr ptrdiff_t kFireballYPowerOffset = 0x1F0;
    static constexpr ptrdiff_t kFireballZPowerOffset = 0x1F8;
    static void* s_textureAtlasLocationBlocks = nullptr;
    static void* s_textureAtlasLocationItems = nullptr;
    static int s_textureAtlasIdBlocks = -1;
    static int s_textureAtlasIdItems = -1;
    static void* s_tileTilesArray = nullptr;
    static thread_local bool s_inLoadTextureByNameHook = false;
    static thread_local bool s_hasForcedBillboardRoute = false;
    static thread_local int s_forcedBillboardAtlas = -1;
    static thread_local int s_forcedBillboardPage = 0;
    static thread_local int s_activeStitchAtlasType = -1;
    static int s_animatedTextureGuardLogCount = 0;

    struct TextureNameArrayNative
    {
        int* data;
        unsigned int length;
    };

    struct ResourceLocationNative
    {
        TextureNameArrayNative textures;
        std::wstring path;
        bool preloaded;
    };

    static ResourceLocationNative s_pageResource;
    static bool s_pageResourceInit = false;
    static int s_pageRouteLogCount = 0;
    static int s_forcedTerrainRouteLogCount = 0;
    static uintptr_t s_gameModuleBase = 0;
    static uintptr_t s_gameModuleEnd = 0;
    static std::vector<void*> s_spawnedEntities;
    static int s_outOfWorldGuardLogCount = 0;
    static int s_pendingServerUseItemId = -1;
    static LevelGetTile_fn s_levelGetTile = nullptr;

    struct ManagedScheduledTick
    {
        void* levelPtr;
        int x;
        int y;
        int z;
        int blockId;
        int remainingTicks;
    };

    static std::vector<ManagedScheduledTick> s_managedScheduledTicks;
    static ULONGLONG s_pendingServerUseExpiryMs = 0;
    static TileGetTextureFaceData_fn s_tileGetTextureFaceData = nullptr;
    static bool s_preInitCalled = false;
    static bool s_initCalled = false;
    static bool s_postInitCalled = false;

    static void EnsurePageResourcesInitialized()
    {
        if (s_pageResourceInit)
            return;
        s_pageResource.textures = { nullptr, 0 };
        s_pageResource.path = L"";
        s_pageResource.preloaded = false;
        s_pageResourceInit = true;
    }

    static void EnsureGameModuleRange()
    {
        if (s_gameModuleBase != 0 && s_gameModuleEnd > s_gameModuleBase)
            return;

        HMODULE exe = GetModuleHandleW(nullptr);
        if (!exe)
            return;

        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(exe);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE)
            return;

        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(
            reinterpret_cast<unsigned char*>(exe) + dos->e_lfanew);
        if (!nt || nt->Signature != IMAGE_NT_SIGNATURE)
            return;

        s_gameModuleBase = reinterpret_cast<uintptr_t>(exe);
        s_gameModuleEnd = s_gameModuleBase + nt->OptionalHeader.SizeOfImage;
    }

    static bool IsCanonicalUserPtr(const void* ptr)
    {
        uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
        if (p < 0x10000ULL)
            return false;
        // User-mode canonical range on x64 Windows.
        if (p >= 0x0000800000000000ULL)
            return false;
        return true;
    }

    static bool IsGameCodePtr(const void* ptr)
    {
        if (!ptr)
            return false;
        EnsureGameModuleRange();
        if (s_gameModuleBase == 0 || s_gameModuleEnd <= s_gameModuleBase)
            return false;
        uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
        return p >= s_gameModuleBase && p < s_gameModuleEnd;
    }

    static bool IsReadableRange(const void* ptr, size_t bytes)
    {
        if (!ptr || bytes == 0 || !IsCanonicalUserPtr(ptr))
            return false;

        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) != sizeof(mbi))
            return false;
        if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS || (mbi.Protect & PAGE_GUARD))
            return false;

        const DWORD readMask = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                               PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        if ((mbi.Protect & readMask) == 0)
            return false;

        uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (p + bytes < p)
            return false;
        return (p + bytes) <= end;
    }

    static void* TryReadTileIcon(void* tilePtr)
    {
        if (!tilePtr)
            return nullptr;

        const void* iconSlot = static_cast<const char*>(tilePtr) + kTileIconOffset;
        if (!IsReadableRange(iconSlot, sizeof(void*)))
            return nullptr;

        void* iconPtr = *reinterpret_cast<void* const*>(iconSlot);
        if (!IsCanonicalUserPtr(iconPtr))
            return nullptr;

        return iconPtr;
    }

    static void PatchSingleSlabIcon(const CustomSlabRegistry::Definition& def, void*)
    {
        if (def.iconName.empty() || !s_tileTilesArray || !IsReadableRange(s_tileTilesArray, sizeof(void*)))
            return;

        void* modIcon = ModAtlas::LookupModIcon(def.iconName);
        if (!modIcon)
            return;

        const void* arrayPtr = *reinterpret_cast<const void* const*>(s_tileTilesArray);
        if (!arrayPtr || !IsReadableRange(arrayPtr, sizeof(void*) * 4096))
            return;

        auto* tiles = reinterpret_cast<void* const*>(const_cast<void*>(arrayPtr));
        const int ids[] = { def.halfBlockId, def.fullBlockId };
        for (int blockId : ids)
        {
            if (blockId < 0 || blockId >= 4096)
                continue;
            void* tilePtr = const_cast<void*>(tiles[blockId]);
            if (!tilePtr || !IsReadableRange(static_cast<const char*>(tilePtr) + kTileIconOffset, sizeof(void*)))
                continue;
            *reinterpret_cast<void**>(static_cast<char*>(tilePtr) + kTileIconOffset) = modIcon;
        }
    }

    static void PatchCustomSlabIcons()
    {
        CustomSlabRegistry::ForEachUnique(&PatchSingleSlabIcon, nullptr);
    }

    static std::wstring BuildVirtualAtlasPath(int atlasType, int page)
    {
        std::wstring base = L"/modloader/";
        if (atlasType == 0)
        {
            if (page <= 0) return base + L"terrain.png";
            return base + L"terrain_p" + std::to_wstring(page) + L".png";
        }

        if (page <= 0) return base + L"items.png";
        return base + L"items_p" + std::to_wstring(page) + L".png";
    }

    static std::wstring NormalizeLowerPath(const std::wstring& path)
    {
        std::wstring lower;
        lower.reserve(path.size());
        for (wchar_t ch : path)
        {
            wchar_t c = (ch == L'\\') ? L'/' : ch;
            lower.push_back((wchar_t)towlower(c));
        }
        return lower;
    }

    static bool EndsWithPath(const std::wstring& path, const wchar_t* suffix)
    {
        size_t pathLen = path.size();
        size_t suffLen = wcslen(suffix);
        if (suffLen > pathLen) return false;
        return path.compare(pathLen - suffLen, suffLen, suffix) == 0;
    }

    static bool TryParseMipmapLevel(const std::wstring& lowerPath, const wchar_t* stem, int& outLevel)
    {
        outLevel = 0;
        if (!stem) return false;
        std::wstring key = std::wstring(stem) + L"mipmaplevel";
        size_t pos = lowerPath.rfind(key);
        if (pos == std::wstring::npos)
            return false;

        size_t numStart = pos + key.size();
        size_t numEnd = lowerPath.find(L".png", numStart);
        if (numEnd == std::wstring::npos || numEnd <= numStart)
            return false;

        int value = 0;
        for (size_t i = numStart; i < numEnd; i++)
        {
            wchar_t ch = lowerPath[i];
            if (ch < L'0' || ch > L'9')
                return false;
            value = value * 10 + (ch - L'0');
        }

        if (value <= 1)
            return false;
        outLevel = value;
        return true;
    }

    static std::string ToLowerAscii(const std::string& value)
    {
        std::string out;
        out.reserve(value.size());
        for (char ch : value)
            out.push_back((char)tolower((unsigned char)ch));
        return out;
    }

    static std::string WStringToLowerAscii(const std::wstring& value)
    {
        std::string out;
        out.reserve(value.size());
        for (wchar_t ch : value)
        {
            if (ch > 0x7F)
                return std::string();
            out.push_back((char)tolower((unsigned char)ch));
        }
        return out;
    }

    static void BuildModAssetIndexLocked()
    {
        s_modAssetRoots.clear();
        s_modAssetsIndexed = true;
        if (s_modsPath.empty())
            return;

        WIN32_FIND_DATAA fd;
        std::string search = s_modsPath + "\\*";
        HANDLE h = FindFirstFileA(search.c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE)
            return;

        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (fd.cFileName[0] == '.') continue;

            std::string modFolder = fd.cFileName;
            std::string assetsPath = s_modsPath + "\\" + modFolder + "\\assets";
            DWORD attr = GetFileAttributesA(assetsPath.c_str());
            if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
                continue;

            WIN32_FIND_DATAA nsfd;
            std::string nsSearch = assetsPath + "\\*";
            HANDLE hNs = FindFirstFileA(nsSearch.c_str(), &nsfd);
            if (hNs == INVALID_HANDLE_VALUE)
                continue;
            do
            {
                if (!(nsfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                if (nsfd.cFileName[0] == '.') continue;

                std::string nsName = ToLowerAscii(nsfd.cFileName);
                if (nsName.empty())
                    continue;

                if (s_modAssetRoots.find(nsName) == s_modAssetRoots.end())
                {
                    s_modAssetRoots.emplace(nsName, assetsPath);
                }
                else
                {
                    LogUtil::Log("[WeaveLoader] ModAssets: duplicate namespace '%s' (folder=%s) ignored",
                                 nsName.c_str(), modFolder.c_str());
                }
            } while (FindNextFileA(hNs, &nsfd));
            FindClose(hNs);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    static void EnsureModAssetIndex()
    {
        if (s_modAssetsIndexed)
            return;
        std::lock_guard<std::mutex> guard(s_modAssetsMutex);
        if (!s_modAssetsIndexed)
            BuildModAssetIndexLocked();
    }

    static bool FileExistsW(const std::wstring& path)
    {
        DWORD attr = GetFileAttributesW(path.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
    }

    static bool TryResolveModAssetPath(const std::wstring& requestPath, std::wstring& outPath)
    {
        if (s_modsPath.empty())
            return false;

        std::wstring lower = NormalizeLowerPath(requestPath);
        if (lower.find(L"://") != std::wstring::npos)
            return false;

        const std::wstring kAssets = L"/assets/";
        size_t assetsPos = lower.find(kAssets);
        if (assetsPos == std::wstring::npos)
            return false;

        size_t nsStart = assetsPos + kAssets.size();
        if (nsStart >= lower.size())
            return false;
        size_t nsEnd = lower.find(L'/', nsStart);
        if (nsEnd == std::wstring::npos || nsEnd <= nsStart)
            return false;

        std::wstring ns = lower.substr(nsStart, nsEnd - nsStart);
        if (ns.empty())
            return false;

        size_t relStart = nsEnd + 1;
        if (relStart >= lower.size())
            return false;
        std::wstring rel = lower.substr(relStart);

        std::string nsKey = WStringToLowerAscii(ns);
        if (nsKey.empty())
            return false;

        EnsureModAssetIndex();
        auto it = s_modAssetRoots.find(nsKey);
        if (it == s_modAssetRoots.end())
            return false;

        std::wstring rootW(it->second.begin(), it->second.end());
        std::wstring relW = ns + L"/" + rel;
        for (wchar_t& ch : relW)
        {
            if (ch == L'/')
                ch = L'\\';
        }
        std::wstring fullPath = rootW + L"\\" + relW;
        if (!FileExistsW(fullPath))
            return false;

        outPath = fullPath;
        return true;
    }

    static int DetectAtlasTypeFromResource(void* resourcePtr)
    {
        if (!resourcePtr)
            return -1;

        if (resourcePtr == s_textureAtlasLocationBlocks) return 0;
        if (resourcePtr == s_textureAtlasLocationItems) return 1;

        const ResourceLocationNative* res = reinterpret_cast<const ResourceLocationNative*>(resourcePtr);
        if (res->textures.data && res->textures.length > 0)
        {
            const int texId = res->textures.data[0];
            if (texId == s_textureAtlasIdBlocks) return 0;
            if (texId == s_textureAtlasIdItems) return 1;
        }

        if (!res->path.empty())
        {
            std::wstring p = NormalizeLowerPath(res->path);
            if (p.find(L"terrain") != std::wstring::npos) return 0;
            if (p.find(L"items") != std::wstring::npos) return 1;
        }

        return -1;
    }

    static bool ParseVirtualAtlasRequest(const std::wstring& path, int& outAtlasType, int& outPage)
    {
        outAtlasType = -1;
        outPage = 0;

        std::wstring lower;
        lower.reserve(path.size());
        for (wchar_t ch : path)
        {
            wchar_t c = (ch == L'\\') ? L'/' : ch;
            lower.push_back((wchar_t)towlower(c));
        }

        size_t fileStart = std::wstring::npos;
        const std::wstring kPrefixA = L"/modloader/";
        const std::wstring kPrefixB = L"/mods/modloader/generated/";
        size_t prefixPosA = lower.find(kPrefixA);
        size_t prefixPosB = lower.find(kPrefixB);
        if (prefixPosA != std::wstring::npos)
            fileStart = prefixPosA + kPrefixA.size();
        else if (prefixPosB != std::wstring::npos)
            fileStart = prefixPosB + kPrefixB.size();
        else
            return false;

        if (fileStart >= lower.size())
            return false;
        std::wstring file = lower.substr(fileStart);

        auto parseStem = [&](const wchar_t* stem, int atlasType) -> bool
        {
            std::wstring stemStr(stem);
            if (file == stemStr + L".png")
            {
                outAtlasType = atlasType;
                outPage = 0;
                return true;
            }

            std::wstring prefix = stemStr + L"_p";
            if (file.rfind(prefix, 0) != 0)
                return false;
            size_t dot = file.find(L".png", prefix.size());
            if (dot == std::wstring::npos || dot <= prefix.size())
                return false;
            std::wstring num = file.substr(prefix.size(), dot - prefix.size());
            for (wchar_t c : num)
            {
                if (c < L'0' || c > L'9')
                    return false;
            }
            outAtlasType = atlasType;
            outPage = _wtoi(num.c_str());
            if (outPage < 0) outPage = 0;
            return true;
        };

        if (parseStem(L"terrain", 0)) return true;
        if (parseStem(L"items", 1)) return true;
        return false;
    }

    struct MineBlockNativeArgs
    {
        int itemId;
        int tileId;
        int x;
        int y;
        int z;
    };

    struct UseItemNativeArgs
    {
        int itemId;
        int isTestUseOnly;
        int isClientSide;
        void* itemInstancePtr;
        void* playerPtr;
        void* playerSharedPtr;
    };

    static bool IsFireballFamilyEntityId(int entityNumericId);
    static void* DecodeItemInstancePtrFromSharedArg(void* sharedArg);
    static void* DecodePlayerPtrFromSharedArg(void* sharedArg);

    static bool TryReadItemId(void* itemInstancePtr, int& outItemId)
    {
        if (!itemInstancePtr)
            return false;

        // ItemInstance inherits enable_shared_from_this, so id is not guaranteed at +0x10.
        // Probe known layouts observed across builds.
        static const int kCandidateOffsets[] = { 0x20, 0x18, 0x10, 0x28 };
        for (int off : kCandidateOffsets)
        {
            const char* idPtr = static_cast<char*>(itemInstancePtr) + off;
            if (!IsReadableRange(idPtr, sizeof(int)))
                continue;

            int id = *reinterpret_cast<const int*>(idPtr);
            if (id > 0 && id < 32000)
            {
                outItemId = id;
                return true;
            }
        }

        return false;
    }

    static int TryDispatchMineBlockFromItemInstancePtr(void* itemInstancePtr, int tile, int x, int y, int z, const char* sourceTag)
    {
        if (!itemInstancePtr)
            return 0;

        int itemId = 0;
        if (!TryReadItemId(itemInstancePtr, itemId))
            return 0;

        MineBlockNativeArgs args{ itemId, tile, x, y, z };
        int action = DotNetHost::CallItemMineBlock(&args, sizeof(args));
        return action;
    }

    static int TryDispatchUseItemFromSharedItemArg(void* itemInstanceSharedPtr, void* playerSharedPtr, bool bTestUseOnly, const char* sourceTag)
    {
        void* itemInstancePtr = DecodeItemInstancePtrFromSharedArg(itemInstanceSharedPtr);
        void* playerPtr = DecodePlayerPtrFromSharedArg(playerSharedPtr);
        if (!itemInstancePtr)
            return 0;

        int itemId = 0;
        if (!TryReadItemId(itemInstancePtr, itemId))
            return 0;

        int isClientSide = 0;
        if (s_activeUseLevel &&
            IsReadableRange(static_cast<char*>(s_activeUseLevel) + kLevelIsClientSideOffset, sizeof(bool)))
        {
            isClientSide =
                *reinterpret_cast<bool*>(static_cast<char*>(s_activeUseLevel) + kLevelIsClientSideOffset) ? 1 : 0;
        }

        UseItemNativeArgs args{ itemId, bTestUseOnly ? 1 : 0, isClientSide, itemInstancePtr, playerPtr, playerSharedPtr };
        return DotNetHost::CallItemUse(&args, sizeof(args));
    }

    void SetSummonSymbols(void* levelAddEntity,
                          void* entityIoNewById,
                          void* entityMoveTo,
                          void* entitySetPos)
    {
        s_levelAddEntity = reinterpret_cast<LevelAddEntity_fn>(levelAddEntity);
        s_entityIoNewById = reinterpret_cast<EntityIONewById_fn>(entityIoNewById);
        s_entityMoveTo = reinterpret_cast<EntityMoveTo_fn>(entityMoveTo);
        s_entitySetPos = reinterpret_cast<EntitySetPos_fn>(entitySetPos);
    }

    void SetUseActionSymbols(void* inventoryRemoveResource,
                             void* inventoryVtable,
                             void* itemInstanceHurtAndBreak,
                             void* containerBroadcastChanges,
                             void* entityGetLookAngle,
                             void* livingEntityGetViewVector,
                             void* entityLerpMotion,
                             void* entitySetPos)
    {
        s_inventoryRemoveResource = reinterpret_cast<InventoryRemoveResource_fn>(inventoryRemoveResource);
        s_inventoryVtable = inventoryVtable;
        s_itemInstanceHurtAndBreak = reinterpret_cast<ItemInstanceHurtAndBreak_fn>(itemInstanceHurtAndBreak);
        s_entityGetLookAngle = reinterpret_cast<EntityGetLookAngle_fn>(entityGetLookAngle);
        s_livingEntityGetViewVector = reinterpret_cast<LivingEntityGetViewVector_fn>(livingEntityGetViewVector);
        s_entityLerpMotion = reinterpret_cast<EntityLerpMotion_fn>(entityLerpMotion);
        s_entitySetPos = reinterpret_cast<EntitySetPos_fn>(entitySetPos);
    }

    void SetBlockHelperSymbols(void* tileGetTextureFaceData)
    {
        s_tileGetTextureFaceData = reinterpret_cast<TileGetTextureFaceData_fn>(tileGetTextureFaceData);
    }

    void SetManagedBlockDispatchSymbols(void* levelGetTile)
    {
        s_levelGetTile = reinterpret_cast<LevelGetTile_fn>(levelGetTile);
    }

    void EnqueueManagedBlockTick(void* levelPtr, int x, int y, int z, int blockId, int delay)
    {
        if (!levelPtr || blockId < 0 || !ManagedBlockRegistry::IsManaged(blockId))
            return;

        const int normalizedDelay = delay > 0 ? delay : 1;
        for (ManagedScheduledTick& tick : s_managedScheduledTicks)
        {
            if (tick.levelPtr == levelPtr &&
                tick.x == x &&
                tick.y == y &&
                tick.z == z &&
                tick.blockId == blockId)
            {
                if (normalizedDelay < tick.remainingTicks)
                    tick.remainingTicks = normalizedDelay;
                return;
            }
        }

        s_managedScheduledTicks.push_back({ levelPtr, x, y, z, blockId, normalizedDelay });
    }

    static bool IsInventoryObjectPtr(void* objectPtr)
    {
        if (!objectPtr || !s_inventoryVtable || !IsReadableRange(objectPtr, sizeof(void*)))
            return false;

        void* vt = *reinterpret_cast<void**>(objectPtr);
        if (!IsCanonicalUserPtr(vt))
            return false;
        return vt == s_inventoryVtable;
    }

    static void* FindInventoryPtrFromPlayer(void* playerPtr)
    {
        if (!playerPtr || !s_inventoryRemoveResource || !IsCanonicalUserPtr(playerPtr))
            return nullptr;

        const char* base = static_cast<const char*>(playerPtr);
        const void* slotPtr = base + kPlayerInventoryOffset;
        if (!IsReadableRange(slotPtr, sizeof(void*)))
            return nullptr;

        void* inventoryPtr = *reinterpret_cast<void* const*>(slotPtr);
        if (!IsInventoryObjectPtr(inventoryPtr))
            return nullptr;

        return inventoryPtr;
    }

    bool ConsumePlayerResource(void* playerPtr, int itemId, int count)
    {
        static int s_consumeFailLogCount = 0;
        if (!playerPtr || !s_inventoryRemoveResource || !s_inventoryVtable || itemId < 0 || count <= 0)
            return false;

        void* inventoryPtr = FindInventoryPtrFromPlayer(playerPtr);
        if (!inventoryPtr)
        {
            if (s_consumeFailLogCount < 20)
            {
                LogUtil::Log("[WeaveLoader] ConsumePlayerResource: no inventory ptr (player=%p item=%d count=%d)",
                             playerPtr, itemId, count);
                s_consumeFailLogCount++;
            }
            return false;
        }

        for (int i = 0; i < count; ++i)
        {
            __try
            {
                if (!s_inventoryRemoveResource(inventoryPtr, itemId))
                {
                    if (s_consumeFailLogCount < 20)
                    {
                        LogUtil::Log("[WeaveLoader] ConsumePlayerResource: removeResource false (inv=%p item=%d)",
                                     inventoryPtr, itemId);
                        s_consumeFailLogCount++;
                    }
                    return false;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                if (s_consumeFailLogCount < 20)
                {
                    LogUtil::Log("[WeaveLoader] ConsumePlayerResource: exception (inv=%p item=%d)", inventoryPtr, itemId);
                    s_consumeFailLogCount++;
                }
                return false;
            }
        }

        return true;
    }

    bool DamageItemInstance(void* itemInstancePtr, int amount, void* ownerSharedPtr)
    {
        if (!itemInstancePtr || !ownerSharedPtr || amount <= 0 || !s_itemInstanceHurtAndBreak)
            return false;

        __try
        {
            s_itemInstanceHurtAndBreak(itemInstancePtr, amount, ownerSharedPtr);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    static bool TryReadLookVector(void* playerPtr, double& dx, double& dy, double& dz)
    {
        dx = 0.0;
        dy = 0.0;
        dz = 0.0;
        if (!playerPtr)
            return false;

        void* vec = nullptr;
        if (s_livingEntityGetViewVector)
            vec = s_livingEntityGetViewVector(playerPtr, 1.0f);
        else if (s_entityGetLookAngle)
            vec = s_entityGetLookAngle(playerPtr);

        if (!IsReadableRange(vec, sizeof(double) * 3))
            return false;

        dx = *reinterpret_cast<double*>(static_cast<char*>(vec) + 0x00);
        dy = *reinterpret_cast<double*>(static_cast<char*>(vec) + 0x08);
        dz = *reinterpret_cast<double*>(static_cast<char*>(vec) + 0x10);
        return true;
    }

    static bool LooksLikeEntityPtr(void* candidate)
    {
        if (!candidate || !IsReadableRange(candidate, sizeof(void*)))
            return false;
        void* vt = *reinterpret_cast<void**>(candidate);
        if (!IsCanonicalUserPtr(vt))
            return false;
        if (!IsGameCodePtr(vt))
            return false;
        return true;
    }

    static bool TryReadPlayerPos(void* playerPtr, double& x, double& y, double& z)
    {
        x = y = z = 0.0;
        if (!playerPtr)
            return false;

        char* base = static_cast<char*>(playerPtr);
        if (!IsReadableRange(base + kEntityXOffset, sizeof(double)) ||
            !IsReadableRange(base + kEntityYOffset, sizeof(double)) ||
            !IsReadableRange(base + kEntityZOffset, sizeof(double)))
            return false;

        x = *reinterpret_cast<double*>(base + kEntityXOffset);
        y = *reinterpret_cast<double*>(base + kEntityYOffset);
        z = *reinterpret_cast<double*>(base + kEntityZOffset);
        return x > -32000000.0 && x < 32000000.0 &&
               y > -2048.0 && y < 4096.0 &&
               z > -32000000.0 && z < 32000000.0;
    }

    static bool IsEntityMarkedRemoved(void* entityPtr)
    {
        if (!entityPtr || !IsReadableRange(static_cast<char*>(entityPtr) + kEntityRemovedOffset, sizeof(bool)))
            return true;
        return *reinterpret_cast<bool*>(static_cast<char*>(entityPtr) + kEntityRemovedOffset);
    }

    static void MarkEntityRemoved(void* entityPtr)
    {
        if (!entityPtr || !IsReadableRange(static_cast<char*>(entityPtr) + kEntityRemovedOffset, sizeof(bool)))
            return;
        *reinterpret_cast<bool*>(static_cast<char*>(entityPtr) + kEntityRemovedOffset) = true;
    }

    static void TrackSpawnedEntity(void* entityPtr)
    {
        if (!entityPtr)
            return;
        s_spawnedEntities.push_back(entityPtr);
    }

    static void CullSpawnedEntitiesBelowWorld()
    {
        if (s_spawnedEntities.empty())
            return;

        size_t write = 0;
        for (size_t read = 0; read < s_spawnedEntities.size(); ++read)
        {
            void* entityPtr = s_spawnedEntities[read];
            if (!entityPtr || !LooksLikeEntityPtr(entityPtr))
                continue;

            if (IsEntityMarkedRemoved(entityPtr))
                continue;

            double x = 0.0, y = 0.0, z = 0.0;
            if (!TryReadPlayerPos(entityPtr, x, y, z))
                continue;

            if (y < -1.0)
            {
                MarkEntityRemoved(entityPtr);
                if (s_outOfWorldGuardLogCount < 20)
                {
                    LogUtil::Log("[WeaveLoader] OutOfWorldGuard: removed spawned entity=%p at y=%.3f", entityPtr, y);
                    s_outOfWorldGuardLogCount++;
                }
                continue;
            }

            s_spawnedEntities[write++] = entityPtr;
        }

        s_spawnedEntities.resize(write);
    }

    bool SummonEntityFromPlayerLook(void* playerPtr,
                                    void* playerSharedPtr,
                                    int entityNumericId,
                                    double speed,
                                    double spawnForward,
                                    double spawnUp)
    {
        static int s_summonFailLogCount = 0;
        void* levelPtr = s_activeUseLevel ? s_activeUseLevel : s_currentLevel;
        if (!levelPtr || !s_levelAddEntity || !playerPtr)
        {
            if (s_summonFailLogCount < 20)
            {
                LogUtil::Log("[WeaveLoader] SummonFromLook fail: preconditions level=%p add=%p player=%p shared=%p id=%d",
                             levelPtr, s_levelAddEntity, playerPtr, playerSharedPtr, entityNumericId);
                s_summonFailLogCount++;
            }
            return false;
        }

        double dx = 0.0, dy = 0.0, dz = 0.0;
        if (!TryReadLookVector(playerPtr, dx, dy, dz))
        {
            if (s_summonFailLogCount < 20)
            {
                LogUtil::Log("[WeaveLoader] SummonFromLook fail: look vector unavailable (player=%p)", playerPtr);
                s_summonFailLogCount++;
            }
            return false;
        }

        double px = 0.0, py = 0.0, pz = 0.0;
        if (!TryReadPlayerPos(playerPtr, px, py, pz))
        {
            if (s_summonFailLogCount < 20)
            {
                LogUtil::Log("[WeaveLoader] SummonFromLook fail: player position unavailable (player=%p)", playerPtr);
                s_summonFailLogCount++;
            }
            return false;
        }

        const double sx = px + (dx * spawnForward);
        const double sy = py + spawnUp + (dy * spawnForward);
        const double sz = pz + (dz * spawnForward);

        std::shared_ptr<void> entity;

        if (!s_entityIoNewById)
        {
            if (s_summonFailLogCount < 20)
            {
                LogUtil::Log("[WeaveLoader] SummonFromLook fail: newById unavailable (id=%d)", entityNumericId);
                s_summonFailLogCount++;
            }
            return false;
        }

        s_entityIoNewById(&entity, entityNumericId, levelPtr);
        if (!entity)
        {
            if (s_summonFailLogCount < 20)
            {
                LogUtil::Log("[WeaveLoader] SummonFromLook fail: EntityIO::newById returned null (id=%d)", entityNumericId);
                s_summonFailLogCount++;
            }
            return false;
        }

        if (s_entityMoveTo)
            s_entityMoveTo(entity.get(), sx, sy, sz, 0.0f, 0.0f);
        else if (s_entitySetPos)
            s_entitySetPos(entity.get(), sx, sy, sz);

        if (s_entityLerpMotion)
            s_entityLerpMotion(entity.get(), dx * speed, dy * speed, dz * speed);

        if (IsFireballFamilyEntityId(entityNumericId) &&
            IsReadableRange(entity.get(), kFireballZPowerOffset + sizeof(double)))
        {
            // Source + PDB: Fireball packets initialize xPower/yPower/zPower separately from xd/yd/zd.
            // EntityIO::newById gives us a game-owned shared_ptr<Entity>; patch the fireball fields in place.
            *reinterpret_cast<void**>(static_cast<char*>(entity.get()) + kFireballOwnerOffset) = nullptr;
            *reinterpret_cast<double*>(static_cast<char*>(entity.get()) + kFireballXPowerOffset) = dx * 0.10;
            *reinterpret_cast<double*>(static_cast<char*>(entity.get()) + kFireballYPowerOffset) = dy * 0.10;
            *reinterpret_cast<double*>(static_cast<char*>(entity.get()) + kFireballZPowerOffset) = dz * 0.10;
        }

        bool added = s_levelAddEntity(levelPtr, &entity);
        if (!added && s_summonFailLogCount < 20)
        {
            LogUtil::Log("[WeaveLoader] SummonFromLook fail: Level::addEntity returned false (id=%d)", entityNumericId);
            s_summonFailLogCount++;
        }
        if (added)
        {
            TrackSpawnedEntity(entity.get());
            DotNetHost::CallEntitySummoned(entityNumericId, static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(sz));
        }
        return added;
    }

    bool SummonEntityByNumericId(int entityNumericId, double x, double y, double z)
    {
        void* levelPtr = s_activeUseLevel ? s_activeUseLevel : s_currentLevel;
        if (!levelPtr || !s_levelAddEntity || !s_entityIoNewById)
            return false;

        std::shared_ptr<void> entity;
        s_entityIoNewById(&entity, entityNumericId, levelPtr);
        if (!entity)
            return false;

        if (s_entityMoveTo)
            s_entityMoveTo(entity.get(), x, y, z, 0.0f, 0.0f);

        bool added = s_levelAddEntity(levelPtr, &entity);
        if (added)
        {
            TrackSpawnedEntity(entity.get());
            DotNetHost::CallEntitySummoned(entityNumericId, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
        }
        return added;
    }

    void SetAtlasLocationPointers(void* blocksLocation, void* itemsLocation)
    {
        s_textureAtlasLocationBlocks = blocksLocation;
        s_textureAtlasLocationItems = itemsLocation;

        s_textureAtlasIdBlocks = -1;
        s_textureAtlasIdItems = -1;

        const ResourceLocationNative* blocks = reinterpret_cast<const ResourceLocationNative*>(blocksLocation);
        const ResourceLocationNative* items = reinterpret_cast<const ResourceLocationNative*>(itemsLocation);
        if (blocks && blocks->textures.data && blocks->textures.length > 0)
            s_textureAtlasIdBlocks = blocks->textures.data[0];
        if (items && items->textures.data && items->textures.length > 0)
            s_textureAtlasIdItems = items->textures.data[0];

        LogUtil::Log("[WeaveLoader] Atlas IDs: blocks=%d items=%d", s_textureAtlasIdBlocks, s_textureAtlasIdItems);
    }

    void SetTileTilesArray(void* tilesArray)
    {
        s_tileTilesArray = tilesArray;
    }

    static bool TryReadVec3(void* vecPtr, double& x, double& y, double& z)
    {
        if (!IsReadableRange(vecPtr, sizeof(double) * 3))
            return false;
        x = *reinterpret_cast<double*>(static_cast<char*>(vecPtr) + 0x00);
        y = *reinterpret_cast<double*>(static_cast<char*>(vecPtr) + 0x08);
        z = *reinterpret_cast<double*>(static_cast<char*>(vecPtr) + 0x10);
        return true;
    }

    static bool IsFireballFamilyEntityId(int entityNumericId)
    {
        return entityNumericId == 12
            || entityNumericId == 13
            || entityNumericId == 19
            || entityNumericId == 1000;
    }

    static void* DecodeItemInstancePtrFromSharedArg(void* sharedArg)
    {
        if (!sharedArg || !IsCanonicalUserPtr(sharedArg))
            return nullptr;

        // Candidate A: shared_ptr<ItemInstance> object where first field is raw ItemInstance*.
        __try
        {
            void* p = *reinterpret_cast<void**>(sharedArg);
            if (p && IsCanonicalUserPtr(p))
            {
                int id = 0;
                if (TryReadItemId(p, id)) return p;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        // Candidate B: argument itself is already ItemInstance*.
        __try
        {
            int id = 0;
            if (TryReadItemId(sharedArg, id)) return sharedArg;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        return nullptr;
    }

    static void* DecodePlayerPtrFromSharedArg(void* sharedArg)
    {
        static int s_decodePlayerLogCount = 0;
        if (!sharedArg || !IsCanonicalUserPtr(sharedArg))
            return nullptr;

        // shared_ptr<Player> is passed by reference; first field is raw Player*.
        __try
        {
            void* p = *reinterpret_cast<void**>(sharedArg);
            if (LooksLikeEntityPtr(p))
            {
                if (s_decodePlayerLogCount < 8)
                {
                    LogUtil::Log("[WeaveLoader] DecodePlayer: shared=%p -> player=%p", sharedArg, p);
                    s_decodePlayerLogCount++;
                }
                return p;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        if (s_decodePlayerLogCount < 8)
        {
            LogUtil::Log("[WeaveLoader] DecodePlayer: failed shared=%p", sharedArg);
            s_decodePlayerLogCount++;
        }

        return nullptr;
    }

    static int TryReadTileId(void* tilePtr)
    {
        if (!tilePtr || !IsReadableRange(static_cast<char*>(tilePtr) + kTileIdOffset, sizeof(int)))
            return -1;
        return *reinterpret_cast<int*>(static_cast<char*>(tilePtr) + kTileIdOffset);
    }

    static void DispatchManagedBlockUpdate(void* tilePtr, void* level, int x, int y, int z, int eventKind, int neighborBlockId)
    {
        const int blockId = TryReadTileId(tilePtr);
        if (!ManagedBlockRegistry::IsManaged(blockId))
            return;

        int isClientSide = 0;
        if (level && IsReadableRange(static_cast<char*>(level) + kLevelIsClientSideOffset, sizeof(bool)))
            isClientSide = *reinterpret_cast<bool*>(static_cast<char*>(level) + kLevelIsClientSideOffset) ? 1 : 0;

        struct BlockUpdateNativeArgs
        {
            int blockId;
            int isClientSide;
            void* levelPtr;
            int x;
            int y;
            int z;
        };
        struct BlockNeighborChangedNativeArgs
        {
            BlockUpdateNativeArgs block;
            int neighborBlockId;
        };

        if (eventKind == 0)
        {
            BlockUpdateNativeArgs args{ blockId, isClientSide, level, x, y, z };
            DotNetHost::CallBlockOnPlace(&args, sizeof(args));
        }
        else if (eventKind == 1)
        {
            BlockNeighborChangedNativeArgs args{};
            args.block = { blockId, isClientSide, level, x, y, z };
            args.neighborBlockId = neighborBlockId;
            DotNetHost::CallBlockNeighborChanged(&args, sizeof(args));
        }
        else if (eventKind == 2)
        {
            BlockUpdateNativeArgs args{ blockId, isClientSide, level, x, y, z };
            DotNetHost::CallBlockTick(&args, sizeof(args));
        }
    }

    static void DispatchManagedBlockById(int blockId, void* level, int x, int y, int z, int eventKind, int neighborBlockId)
    {
        if (!ManagedBlockRegistry::IsManaged(blockId))
            return;

        int isClientSide = 0;
        if (level && IsReadableRange(static_cast<char*>(level) + kLevelIsClientSideOffset, sizeof(bool)))
            isClientSide = *reinterpret_cast<bool*>(static_cast<char*>(level) + kLevelIsClientSideOffset) ? 1 : 0;

        struct BlockUpdateNativeArgs
        {
            int blockId;
            int isClientSide;
            void* levelPtr;
            int x;
            int y;
            int z;
        };
        struct BlockNeighborChangedNativeArgs
        {
            BlockUpdateNativeArgs block;
            int neighborBlockId;
        };

        if (eventKind == 0)
        {
            BlockUpdateNativeArgs args{ blockId, isClientSide, level, x, y, z };
            DotNetHost::CallBlockOnPlace(&args, sizeof(args));
        }
        else if (eventKind == 1)
        {
            BlockNeighborChangedNativeArgs args{};
            args.block = { blockId, isClientSide, level, x, y, z };
            args.neighborBlockId = neighborBlockId;
            DotNetHost::CallBlockNeighborChanged(&args, sizeof(args));
        }
        else if (eventKind == 2)
        {
            BlockUpdateNativeArgs args{ blockId, isClientSide, level, x, y, z };
            DotNetHost::CallBlockTick(&args, sizeof(args));
        }
    }

    void __fastcall Hooked_TileOnPlace(void* thisPtr, void* level, int x, int y, int z)
    {
        if (Original_TileOnPlace)
            Original_TileOnPlace(thisPtr, level, x, y, z);
        DispatchManagedBlockUpdate(thisPtr, level, x, y, z, 0, 0);
    }

    void __fastcall Hooked_TileNeighborChanged(void* thisPtr, void* level, int x, int y, int z, int type)
    {
        if (Original_TileNeighborChanged)
            Original_TileNeighborChanged(thisPtr, level, x, y, z, type);
        DispatchManagedBlockUpdate(thisPtr, level, x, y, z, 1, type);
    }

    void __fastcall Hooked_TileTick(void* thisPtr, void* level, int x, int y, int z, void* random)
    {
        if (Original_TileTick)
            Original_TileTick(thisPtr, level, x, y, z, random);
        DispatchManagedBlockUpdate(thisPtr, level, x, y, z, 2, 0);
    }

    bool __fastcall Hooked_LevelSetTileAndData(void* thisPtr, int x, int y, int z, int tile, int data, int updateFlags)
    {
        const bool result = Original_LevelSetTileAndData
            ? Original_LevelSetTileAndData(thisPtr, x, y, z, tile, data, updateFlags)
            : false;

        if (result && tile > 0)
            DispatchManagedBlockById(tile, thisPtr, x, y, z, 0, 0);

        return result;
    }

    bool __fastcall Hooked_LevelSetData(void* thisPtr, int x, int y, int z, int data, int updateFlags, bool forceUpdate)
    {
        const bool result = Original_LevelSetData
            ? Original_LevelSetData(thisPtr, x, y, z, data, updateFlags, forceUpdate)
            : false;

        return result;
    }

    void __fastcall Hooked_LevelUpdateNeighborsAt(void* thisPtr, int x, int y, int z, int type)
    {
        if (Original_LevelUpdateNeighborsAt)
            Original_LevelUpdateNeighborsAt(thisPtr, x, y, z, type);

        if (!s_levelGetTile)
            return;

        static const int kNeighborOffsets[6][3] = {
            {-1, 0, 0}, {1, 0, 0},
            {0, -1, 0}, {0, 1, 0},
            {0, 0, -1}, {0, 0, 1}
        };

        for (const auto& offset : kNeighborOffsets)
        {
            const int nx = x + offset[0];
            const int ny = y + offset[1];
            const int nz = z + offset[2];
            const int neighborBlockId = s_levelGetTile(thisPtr, nx, ny, nz);
            DispatchManagedBlockById(neighborBlockId, thisPtr, nx, ny, nz, 1, type);
        }
    }

    bool __fastcall Hooked_ServerLevelTickPendingTicks(void* thisPtr, bool force)
    {
        const bool originalResult = Original_ServerLevelTickPendingTicks
            ? Original_ServerLevelTickPendingTicks(thisPtr, force)
            : false;

        bool anyPending = false;
        for (auto it = s_managedScheduledTicks.begin(); it != s_managedScheduledTicks.end();)
        {
            if (it->levelPtr != thisPtr)
            {
                ++it;
                continue;
            }

            --it->remainingTicks;
            if (it->remainingTicks <= 0)
            {
                DispatchManagedBlockById(it->blockId, it->levelPtr, it->x, it->y, it->z, 2, 0);
                it = s_managedScheduledTicks.erase(it);
            }
            else
            {
                anyPending = true;
                ++it;
            }
        }

        return originalResult || anyPending;
    }

    void* __fastcall Hooked_McRegionChunkStorageLoad(void* thisPtr, void* level, int x, int z)
    {
        static int s_chunkLoadLogCount = 0;
        void* levelChunk = Original_McRegionChunkStorageLoad
            ? Original_McRegionChunkStorageLoad(thisPtr, level, x, z)
            : nullptr;
        if (levelChunk)
        {
            const int remapped = WorldIdRemap::RemapChunkBlockIds(thisPtr, levelChunk, x, z);
            if (s_chunkLoadLogCount < 64)
            {
                ++s_chunkLoadLogCount;
                LogUtil::Log("[WeaveLoader] WorldIdRemap chunk load: x=%d z=%d remapped=%d", x, z, remapped);
            }
        }
        return levelChunk;
    }

    void __fastcall Hooked_McRegionChunkStorageSave(void* thisPtr, void* level, void* levelChunk)
    {
        WorldIdRemap::SaveChunkBlockNamespaces(thisPtr, levelChunk);
        if (Original_McRegionChunkStorageSave)
            Original_McRegionChunkStorageSave(thisPtr, level, levelChunk);
    }

    int __fastcall Hooked_TileGetResource(void* thisPtr, int data, void* random, int playerBonusLevel)
    {
        const ManagedBlockRegistry::Definition* def = ManagedBlockRegistry::Find(TryReadTileId(thisPtr));
        if (def && def->dropBlockId >= 0)
            return def->dropBlockId;
        return Original_TileGetResource ? Original_TileGetResource(thisPtr, data, random, playerBonusLevel) : 0;
    }

    int __fastcall Hooked_TileCloneTileId(void* thisPtr, void* level, int x, int y, int z)
    {
        const ManagedBlockRegistry::Definition* def = ManagedBlockRegistry::Find(TryReadTileId(thisPtr));
        if (def && def->cloneBlockId >= 0)
            return def->cloneBlockId;
        return Original_TileCloneTileId ? Original_TileCloneTileId(thisPtr, level, x, y, z) : TryReadTileId(thisPtr);
    }

    void* __fastcall Hooked_StoneSlabGetTexture(void* thisPtr, int face, int data)
    {
        if (CustomSlabRegistry::Find(TryReadTileId(thisPtr)))
        {
            if (void* iconPtr = TryReadTileIcon(thisPtr))
                return iconPtr;
        }
        return Original_StoneSlabGetTexture ? Original_StoneSlabGetTexture(thisPtr, face, data) : nullptr;
    }

    void* __fastcall Hooked_WoodSlabGetTexture(void* thisPtr, int face, int data)
    {
        if (CustomSlabRegistry::Find(TryReadTileId(thisPtr)))
        {
            if (void* iconPtr = TryReadTileIcon(thisPtr))
                return iconPtr;
        }
        return Original_WoodSlabGetTexture ? Original_WoodSlabGetTexture(thisPtr, face, data) : nullptr;
    }

    int __fastcall Hooked_StoneSlabGetResource(void* thisPtr, int data, void* random, int playerBonusLevel)
    {
        const CustomSlabRegistry::Definition* def = CustomSlabRegistry::Find(TryReadTileId(thisPtr));
        if (def)
            return def->halfBlockId;
        return Original_StoneSlabGetResource ? Original_StoneSlabGetResource(thisPtr, data, random, playerBonusLevel) : 0;
    }

    int __fastcall Hooked_WoodSlabGetResource(void* thisPtr, int data, void* random, int playerBonusLevel)
    {
        const CustomSlabRegistry::Definition* def = CustomSlabRegistry::Find(TryReadTileId(thisPtr));
        if (def)
            return def->halfBlockId;
        return Original_WoodSlabGetResource ? Original_WoodSlabGetResource(thisPtr, data, random, playerBonusLevel) : 0;
    }

    unsigned int __fastcall Hooked_StoneSlabGetDescriptionId(void* thisPtr, int data)
    {
        const CustomSlabRegistry::Definition* def = CustomSlabRegistry::Find(TryReadTileId(thisPtr));
        if (def && def->descriptionId >= 0)
            return static_cast<unsigned int>(def->descriptionId);
        return Original_StoneSlabGetDescriptionId ? Original_StoneSlabGetDescriptionId(thisPtr, data) : 0;
    }

    unsigned int __fastcall Hooked_WoodSlabGetDescriptionId(void* thisPtr, int data)
    {
        const CustomSlabRegistry::Definition* def = CustomSlabRegistry::Find(TryReadTileId(thisPtr));
        if (def && def->descriptionId >= 0)
            return static_cast<unsigned int>(def->descriptionId);
        return Original_WoodSlabGetDescriptionId ? Original_WoodSlabGetDescriptionId(thisPtr, data) : 0;
    }

    int __fastcall Hooked_StoneSlabGetAuxName(void* thisPtr, int auxValue)
    {
        const CustomSlabRegistry::Definition* def = CustomSlabRegistry::Find(TryReadTileId(thisPtr));
        if (def && def->descriptionId >= 0)
            return def->descriptionId;
        return Original_StoneSlabGetAuxName ? Original_StoneSlabGetAuxName(thisPtr, auxValue) : 0;
    }

    int __fastcall Hooked_WoodSlabGetAuxName(void* thisPtr, int auxValue)
    {
        const CustomSlabRegistry::Definition* def = CustomSlabRegistry::Find(TryReadTileId(thisPtr));
        if (def && def->descriptionId >= 0)
            return def->descriptionId;
        return Original_WoodSlabGetAuxName ? Original_WoodSlabGetAuxName(thisPtr, auxValue) : 0;
    }

    void __fastcall Hooked_StoneSlabRegisterIcons(void* thisPtr, void* iconRegister)
    {
        if (Original_StoneSlabRegisterIcons)
            Original_StoneSlabRegisterIcons(thisPtr, iconRegister);
    }

    void __fastcall Hooked_WoodSlabRegisterIcons(void* thisPtr, void* iconRegister)
    {
        if (Original_WoodSlabRegisterIcons)
            Original_WoodSlabRegisterIcons(thisPtr, iconRegister);
    }

    void* __fastcall Hooked_StoneSlabItemGetIcon(void* thisPtr, int auxValue)
    {
        if (thisPtr && IsReadableRange(static_cast<const char*>(thisPtr) + kItemIdOffset, sizeof(int)))
        {
            const int itemId = *reinterpret_cast<const int*>(static_cast<const char*>(thisPtr) + kItemIdOffset);
            const CustomSlabRegistry::Definition* def = CustomSlabRegistry::Find(itemId);
            if (def && s_tileTilesArray && IsReadableRange(s_tileTilesArray, sizeof(void*)))
            {
                const void* arrayPtr = *reinterpret_cast<const void* const*>(s_tileTilesArray);
                if (arrayPtr && IsReadableRange(arrayPtr, sizeof(void*) * 4096))
                {
                    auto* tiles = reinterpret_cast<void* const*>(const_cast<void*>(arrayPtr));
                    if (def->halfBlockId >= 0 && def->halfBlockId < 4096)
                    {
                        void* halfTile = const_cast<void*>(tiles[def->halfBlockId]);
                        void* iconPtr = TryReadTileIcon(halfTile);
                        if (iconPtr)
                            return iconPtr;
                    }
                }
            }
        }

        return Original_StoneSlabItemGetIcon
            ? Original_StoneSlabItemGetIcon(thisPtr, auxValue)
            : nullptr;
    }

    unsigned int __fastcall Hooked_StoneSlabItemGetDescriptionId(void* thisPtr, void* itemInstanceSharedPtr)
    {
        if (thisPtr && IsReadableRange(static_cast<const char*>(thisPtr) + kItemIdOffset, sizeof(int)))
        {
            const int itemId = *reinterpret_cast<const int*>(static_cast<const char*>(thisPtr) + kItemIdOffset);
            const CustomSlabRegistry::Definition* def = CustomSlabRegistry::Find(itemId);
            if (def && def->descriptionId >= 0)
                return static_cast<unsigned int>(def->descriptionId);
        }

        return Original_StoneSlabItemGetDescriptionId
            ? Original_StoneSlabItemGetDescriptionId(thisPtr, itemInstanceSharedPtr)
            : 0;
    }

    int __fastcall Hooked_HalfSlabCloneTileId(void* thisPtr, void* level, int x, int y, int z)
    {
        const CustomSlabRegistry::Definition* def = CustomSlabRegistry::Find(TryReadTileId(thisPtr));
        if (def)
            return def->halfBlockId;
        return Original_HalfSlabCloneTileId ? Original_HalfSlabCloneTileId(thisPtr, level, x, y, z) : TryReadTileId(thisPtr);
    }

    void __fastcall Hooked_PreStitchedTextureMapStitch(void* thisPtr)
    {
        const int prevAtlasType = s_activeStitchAtlasType;
        int iconType = -1;
        if (thisPtr && IsReadableRange(static_cast<const char*>(thisPtr) + 8, sizeof(int)))
            iconType = *reinterpret_cast<const int*>(static_cast<const char*>(thisPtr) + 8);
        s_activeStitchAtlasType = iconType;
        if (Original_PreStitchedTextureMapStitch)
            Original_PreStitchedTextureMapStitch(thisPtr);
        s_activeStitchAtlasType = prevAtlasType;
    }

    void __fastcall Hooked_LoadUVs(void* thisPtr)
    {
        LogUtil::Log("[WeaveLoader] Hooked_LoadUVs: ENTER (textureMap=%p)", thisPtr);
        if (thisPtr && IsReadableRange(static_cast<const char*>(thisPtr) + 8, sizeof(int)))
            s_activeStitchAtlasType = *reinterpret_cast<const int*>(static_cast<const char*>(thisPtr) + 8);
        if (Original_LoadUVs)
            Original_LoadUVs(thisPtr);
        LogUtil::Log("[WeaveLoader] Hooked_LoadUVs: original returned, creating mod icons");
        ModAtlas::CreateModIcons(thisPtr);
        PatchCustomSlabIcons();
        LogUtil::Log("[WeaveLoader] Hooked_LoadUVs: DONE");
    }

    static float ApplyAtlasScale(void* iconPtr, float value, bool isU, bool isModIcon)
    {
        if (isModIcon)
            return value;

        int atlasType = -1;
        if (!ModAtlas::GetIconAtlasType(iconPtr, atlasType))
        {
            if (s_activeStitchAtlasType >= 0)
                atlasType = s_activeStitchAtlasType;
            else
                return value;
        }

        float uScale = 1.0f;
        float vScale = 1.0f;
        if (!ModAtlas::GetAtlasScale(atlasType, uScale, vScale))
            return value;

        const float scale = isU ? uScale : vScale;
        if (scale > 0.9995f && scale < 1.0005f)
            return value;
        return value * scale;
    }

    float __fastcall Hooked_StitchedGetU0(void* thisPtr, bool adjust)
    {
        int atlasType = -1;
        int page = 0;
        const bool isModIcon = ModAtlas::TryGetIconRoute(thisPtr, atlasType, page);
        if (isModIcon && atlasType == 0 && page > 0)
            ModAtlas::NotifyIconSampled(thisPtr);
        float value = Original_StitchedGetU0 ? Original_StitchedGetU0(thisPtr, adjust) : 0.0f;
        return ApplyAtlasScale(thisPtr, value, true, isModIcon);
    }

    float __fastcall Hooked_StitchedGetU1(void* thisPtr, bool adjust)
    {
        int atlasType = -1;
        int page = 0;
        const bool isModIcon = ModAtlas::TryGetIconRoute(thisPtr, atlasType, page);
        if (isModIcon && atlasType == 0 && page > 0)
            ModAtlas::NotifyIconSampled(thisPtr);
        float value = Original_StitchedGetU1 ? Original_StitchedGetU1(thisPtr, adjust) : 0.0f;
        return ApplyAtlasScale(thisPtr, value, true, isModIcon);
    }

    float __fastcall Hooked_StitchedGetV0(void* thisPtr, bool adjust)
    {
        int atlasType = -1;
        int page = 0;
        const bool isModIcon = ModAtlas::TryGetIconRoute(thisPtr, atlasType, page);
        if (isModIcon && atlasType == 0 && page > 0)
            ModAtlas::NotifyIconSampled(thisPtr);
        float value = Original_StitchedGetV0 ? Original_StitchedGetV0(thisPtr, adjust) : 0.0f;
        return ApplyAtlasScale(thisPtr, value, false, isModIcon);
    }

    float __fastcall Hooked_StitchedGetV1(void* thisPtr, bool adjust)
    {
        int atlasType = -1;
        int page = 0;
        const bool isModIcon = ModAtlas::TryGetIconRoute(thisPtr, atlasType, page);
        if (isModIcon && atlasType == 0 && page > 0)
            ModAtlas::NotifyIconSampled(thisPtr);
        float value = Original_StitchedGetV1 ? Original_StitchedGetV1(thisPtr, adjust) : 0.0f;
        return ApplyAtlasScale(thisPtr, value, false, isModIcon);
    }

    void __fastcall Hooked_TexturesBindTextureResource(void* thisPtr, void* resourcePtr)
    {
        if (!Original_TexturesBindTextureResource)
            return;

        const int boundAtlas = DetectAtlasTypeFromResource(resourcePtr);

        // Terrain/world rendering binds LOCATION_BLOCKS once for an entire pass.
        // Route terrain binds to merged page 1 so vanilla+modded block UVs work together.
        if (boundAtlas == 0)
        {
            std::string terrainPage1 = ModAtlas::GetMergedPagePath(0, 1);
            if (!terrainPage1.empty() && resourcePtr)
            {
                EnsurePageResourcesInitialized();
                const ResourceLocationNative* originalRes =
                    reinterpret_cast<const ResourceLocationNative*>(resourcePtr);
                s_pageResource.textures = originalRes->textures;
                s_pageResource.path = BuildVirtualAtlasPath(0, 1);
                s_pageResource.preloaded = false;
                if (s_forcedTerrainRouteLogCount < 20)
                {
                    LogUtil::Log("[WeaveLoader] AtlasRoute: forced terrain page=1 path=%ls",
                                 s_pageResource.path.c_str());
                    s_forcedTerrainRouteLogCount++;
                }
                ModAtlas::ClearPendingPage();
                Original_TexturesBindTextureResource(thisPtr, &s_pageResource);
                return;
            }
        }

        int pendingAtlas = -1;
        int pendingPage = 0;
        const bool hasPending = ModAtlas::PeekPendingPage(pendingAtlas, pendingPage);
        if (hasPending && pendingPage > 0 && resourcePtr)
        {
            const bool atlasMatches = (boundAtlas == pendingAtlas);

            if (atlasMatches)
            {
                EnsurePageResourcesInitialized();
                // Preserve texture-name metadata from the original atlas location.
                // Some codepaths consult this even when `preloaded` is false.
                const ResourceLocationNative* originalRes =
                    reinterpret_cast<const ResourceLocationNative*>(resourcePtr);
                s_pageResource.textures = originalRes->textures;
                s_pageResource.path = BuildVirtualAtlasPath(pendingAtlas, pendingPage);
                s_pageResource.preloaded = false;
                if (s_pageRouteLogCount < 40)
                {
                    LogUtil::Log("[WeaveLoader] AtlasRoute: atlas=%d page=%d path=%ls",
                                 pendingAtlas, pendingPage, s_pageResource.path.c_str());
                }
                s_pageRouteLogCount++;
                ModAtlas::ClearPendingPage();
                Original_TexturesBindTextureResource(thisPtr, &s_pageResource);
                return;
            }
        }

        Original_TexturesBindTextureResource(thisPtr, resourcePtr);
    }

    int __fastcall Hooked_TexturesLoadTextureByName(void* thisPtr, int texId, const std::wstring& resourceName)
    {
        if (!Original_TexturesLoadTextureByName)
            return 0;
        if (s_inLoadTextureByNameHook)
            return Original_TexturesLoadTextureByName(thisPtr, texId, resourceName);

        int atlasType = -1;
        int page = 0;
        if (ParseVirtualAtlasRequest(resourceName, atlasType, page) && page > 0)
        {
            const int remappedTexId =
                (atlasType == 0) ? s_textureAtlasIdBlocks :
                (atlasType == 1) ? s_textureAtlasIdItems : -1;
            if (remappedTexId >= 0)
            {
                s_inLoadTextureByNameHook = true;
                int routed = Original_TexturesLoadTextureByName(thisPtr, remappedTexId, resourceName);
                s_inLoadTextureByNameHook = false;
                return routed;
            }
        }

        return Original_TexturesLoadTextureByName(thisPtr, texId, resourceName);
    }

    int __fastcall Hooked_TexturesLoadTextureByIndex(void* thisPtr, int idx)
    {
        if (!Original_TexturesLoadTextureByIndex)
            return 0;

        // Some world/render paths bind TN_TERRAIN directly via loadTexture(int).
        // Route those binds to terrain page 1 so modded placed blocks remain visible.
        if (idx == s_textureAtlasIdBlocks && Original_TexturesLoadTextureByName)
        {
            std::string terrainPage1 = ModAtlas::GetMergedPagePath(0, 1);
            if (!terrainPage1.empty())
            {
                const std::wstring virtualPath = BuildVirtualAtlasPath(0, 1);
                return Original_TexturesLoadTextureByName(thisPtr, idx, virtualPath);
            }
        }

        return Original_TexturesLoadTextureByIndex(thisPtr, idx);
    }

    static bool IsModLoaderGeneratedPath(const std::wstring& lower)
    {
        return (lower.find(L"/mods/modloader/generated/") != std::wstring::npos) ||
               (lower.find(L"/modloader/") != std::wstring::npos);
    }

    static std::wstring ToLowerSimple(const std::wstring& s)
    {
        std::wstring lower;
        lower.reserve(s.size());
        for (wchar_t ch : s)
            lower.push_back((wchar_t)towlower(ch));
        return lower;
    }

    static std::unordered_map<void*, std::wstring> s_textureNames;

    void* __fastcall Hooked_TexturesReadImage(void* thisPtr, int texId, const std::wstring& name)
    {
        if (!Original_TexturesReadImage)
            return nullptr;
        void* img = Original_TexturesReadImage(thisPtr, texId, name);
        if (!img)
            return img;

        std::wstring lower = NormalizeLowerPath(name);
        if (IsModLoaderGeneratedPath(lower))
            return img;

        if (EndsWithPath(lower, L"terrain.png"))
        {
            ModAtlas::OverrideAtlasFromBufferedImage(0, img);
        }
        else if (EndsWithPath(lower, L"items.png"))
        {
            ModAtlas::OverrideAtlasFromBufferedImage(1, img);
        }
        return img;
    }

    void __fastcall Hooked_BufferedImageCtorFile(void* thisPtr, const std::wstring& file, bool filenameHasExtension, bool bTitleUpdateTexture, const std::wstring& drive)
    {
        if (Original_BufferedImageCtorFile)
            Original_BufferedImageCtorFile(thisPtr, file, filenameHasExtension, bTitleUpdateTexture, drive);

        // Intentionally left empty: handled in Textures::readImage hook to avoid
        // constructor-level crashes during boot.
    }

    void __fastcall Hooked_BufferedImageCtorDLCPack(void* thisPtr, void* dlcPack, const std::wstring& file, bool filenameHasExtension)
    {
        if (Original_BufferedImageCtorDLCPack)
            Original_BufferedImageCtorDLCPack(thisPtr, dlcPack, file, filenameHasExtension);

        // Intentionally left empty: handled in Textures::readImage hook to avoid
        // constructor-level crashes during boot.
    }

    void* __fastcall Hooked_TextureManagerCreateTexture(void* thisPtr, const std::wstring& name, int mode, int width, int height, int wrap, int format, int minFilter, int magFilter, bool mipmap, void* image)
    {
        if (!Original_TextureManagerCreateTexture)
            return nullptr;

        void* tex = Original_TextureManagerCreateTexture(thisPtr, name, mode, width, height, wrap, format, minFilter, magFilter, mipmap, image);
        if (tex)
        {
            std::wstring lower = ToLowerSimple(name);
            if (lower == L"terrain" || lower == L"items")
                s_textureNames[tex] = lower;
        }
        return tex;
    }

    void __fastcall Hooked_TextureTransferFromImage(void* thisPtr, void* image)
    {
        if (!Original_TextureTransferFromImage)
            return;

        auto it = s_textureNames.find(thisPtr);
        if (it != s_textureNames.end() && image)
        {
            if (it->second == L"terrain")
                ModAtlas::OverrideAtlasFromBufferedImage(0, image);
            else if (it->second == L"items")
                ModAtlas::OverrideAtlasFromBufferedImage(1, image);
        }

        Original_TextureTransferFromImage(thisPtr, image);
    }

    static void TryOverrideAtlasFromPackImage(const std::wstring& file, void* image)
    {
        if (!image) return;
        std::wstring lower = NormalizeLowerPath(file);
        if (IsModLoaderGeneratedPath(lower))
            return;

        if (EndsWithPath(lower, L"terrain.png"))
        {
            ModAtlas::OverrideAtlasFromBufferedImage(0, image);
        }
        else if (EndsWithPath(lower, L"items.png"))
        {
            ModAtlas::OverrideAtlasFromBufferedImage(1, image);
        }
    }

    void* __fastcall Hooked_AbstractTexturePackGetImageResource(void* thisPtr, const std::wstring& file, bool filenameHasExtension, bool bTitleUpdateTexture, const std::wstring& drive)
    {
        if (!Original_AbstractTexturePackGetImageResource)
            return nullptr;
        void* img = Original_AbstractTexturePackGetImageResource(thisPtr, file, filenameHasExtension, bTitleUpdateTexture, drive);
        TryOverrideAtlasFromPackImage(file, img);
        return img;
    }

    void* __fastcall Hooked_DLCTexturePackGetImageResource(void* thisPtr, const std::wstring& file, bool filenameHasExtension, bool bTitleUpdateTexture, const std::wstring& drive)
    {
        if (!Original_DLCTexturePackGetImageResource)
            return nullptr;
        void* img = Original_DLCTexturePackGetImageResource(thisPtr, file, filenameHasExtension, bTitleUpdateTexture, drive);
        TryOverrideAtlasFromPackImage(file, img);
        return img;
    }

    static int s_registerIconCallCount = 0;

    void* __fastcall Hooked_RegisterIcon(void* thisPtr, const std::wstring& name)
    {
        s_registerIconCallCount++;
        int iconType = -1;
        if (thisPtr && IsReadableRange(static_cast<const char*>(thisPtr) + 8, sizeof(int)))
            iconType = *reinterpret_cast<const int*>(static_cast<const char*>(thisPtr) + 8);

        void* modIcon = ModAtlas::LookupModIcon(name);
        if (modIcon)
        {
            if (iconType >= 0)
                ModAtlas::NoteIconAtlasType(modIcon, iconType);
            LogUtil::Log("[WeaveLoader] registerIcon #%d: '%ls' -> MOD ICON %p",
                         s_registerIconCallCount, name.c_str(), modIcon);
            return modIcon;
        }
        void* result = Original_RegisterIcon ? Original_RegisterIcon(thisPtr, name) : nullptr;
        if (result && iconType >= 0)
            ModAtlas::NoteIconAtlasType(result, iconType);
        if (s_registerIconCallCount <= 30 || !result)
        {
            LogUtil::Log("[WeaveLoader] registerIcon #%d: '%ls' -> vanilla %p",
                         s_registerIconCallCount, name.c_str(), result);
        }
        return result;
    }

    void* __fastcall Hooked_ItemInstanceGetIcon(void* thisPtr)
    {
        if (!Original_ItemInstanceGetIcon)
            return nullptr;

        void* icon = Original_ItemInstanceGetIcon(thisPtr);
        if (icon)
            ModAtlas::NotifyIconSampled(icon);
        else
            ModAtlas::ClearPendingPage();
        return icon;
    }

    void __fastcall Hooked_EntityRendererBindTextureResource(void* thisPtr, void* resourcePtr)
    {
        if (!Original_EntityRendererBindTextureResource)
            return;

        const int boundAtlas = DetectAtlasTypeFromResource(resourcePtr);
        if (s_hasForcedBillboardRoute && resourcePtr && boundAtlas == s_forcedBillboardAtlas && s_forcedBillboardPage > 0)
        {
            EnsurePageResourcesInitialized();
            const ResourceLocationNative* originalRes =
                reinterpret_cast<const ResourceLocationNative*>(resourcePtr);
            s_pageResource.textures = originalRes->textures;
            s_pageResource.path = BuildVirtualAtlasPath(s_forcedBillboardAtlas, s_forcedBillboardPage);
            s_pageResource.preloaded = false;
            Original_EntityRendererBindTextureResource(thisPtr, &s_pageResource);
            return;
        }

        Original_EntityRendererBindTextureResource(thisPtr, resourcePtr);
    }

    void __fastcall Hooked_ItemRendererRenderItemBillboard(void* thisPtr, void* entitySharedPtr, void* iconPtr, int count, float a, float red, float green, float blue)
    {
        const bool hadForcedRoute = s_hasForcedBillboardRoute;
        const int prevAtlas = s_forcedBillboardAtlas;
        const int prevPage = s_forcedBillboardPage;

        int atlasType = -1;
        int page = 0;
        if (iconPtr && ModAtlas::TryGetIconRoute(iconPtr, atlasType, page) && page > 0)
        {
            s_hasForcedBillboardRoute = true;
            s_forcedBillboardAtlas = atlasType;
            s_forcedBillboardPage = page;
        }

        if (Original_ItemRendererRenderItemBillboard)
            Original_ItemRendererRenderItemBillboard(thisPtr, entitySharedPtr, iconPtr, count, a, red, green, blue);

        s_hasForcedBillboardRoute = hadForcedRoute;
        s_forcedBillboardAtlas = prevAtlas;
        s_forcedBillboardPage = prevPage;
    }

    static void LogAnimatedTextureGuard(const char* what, void* thisPtr)
    {
        if (s_animatedTextureGuardLogCount >= 12)
            return;
        LogUtil::Log("[WeaveLoader] AnimatedTextureGuard: %s fallback for %p", what, thisPtr);
        s_animatedTextureGuardLogCount++;
    }

    void __fastcall Hooked_CompassTextureCycleFrames(void* thisPtr)
    {
        if (!Original_CompassTextureCycleFrames)
            return;
        __try
        {
            Original_CompassTextureCycleFrames(thisPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LogAnimatedTextureGuard("CompassTexture::cycleFrames", thisPtr);
        }
    }

    void __fastcall Hooked_ClockTextureCycleFrames(void* thisPtr)
    {
        if (!Original_ClockTextureCycleFrames)
            return;
        __try
        {
            Original_ClockTextureCycleFrames(thisPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LogAnimatedTextureGuard("ClockTexture::cycleFrames", thisPtr);
        }
    }

    int __fastcall Hooked_CompassTextureGetSourceWidth(void* thisPtr)
    {
        if (!Original_CompassTextureGetSourceWidth)
            return 16;
        __try
        {
            return Original_CompassTextureGetSourceWidth(thisPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LogAnimatedTextureGuard("CompassTexture::getSourceWidth", thisPtr);
            return 16;
        }
    }

    int __fastcall Hooked_CompassTextureGetSourceHeight(void* thisPtr)
    {
        if (!Original_CompassTextureGetSourceHeight)
            return 16;
        __try
        {
            return Original_CompassTextureGetSourceHeight(thisPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LogAnimatedTextureGuard("CompassTexture::getSourceHeight", thisPtr);
            return 16;
        }
    }

    int __fastcall Hooked_ClockTextureGetSourceWidth(void* thisPtr)
    {
        if (!Original_ClockTextureGetSourceWidth)
            return 16;
        __try
        {
            return Original_ClockTextureGetSourceWidth(thisPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LogAnimatedTextureGuard("ClockTexture::getSourceWidth", thisPtr);
            return 16;
        }
    }

    int __fastcall Hooked_ClockTextureGetSourceHeight(void* thisPtr)
    {
        if (!Original_ClockTextureGetSourceHeight)
            return 16;
        __try
        {
            return Original_ClockTextureGetSourceHeight(thisPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LogAnimatedTextureGuard("ClockTexture::getSourceHeight", thisPtr);
            return 16;
        }
    }

    void __fastcall Hooked_ItemInstanceMineBlock(void* thisPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr)
    {
        s_itemMineBlockHookCalls++;
        int action = TryDispatchMineBlockFromItemInstancePtr(thisPtr, tile, x, y, z, "ItemInstance::mineBlock");
        if (action == 2)
        {
            // Managed item explicitly canceled vanilla mineBlock behavior.
            return;
        }

        if (Original_ItemInstanceMineBlock)
            Original_ItemInstanceMineBlock(thisPtr, level, tile, x, y, z, ownerSharedPtr);
    }

    void* __fastcall Hooked_ItemInstanceSave(void* thisPtr, void* compoundTagPtr)
    {
        // Namespace marker now lives on ItemInstance::tag, so it must be present
        // before vanilla serialization copies that nested tag into the output.
        WorldIdRemap::TagModdedItemInstance(thisPtr, compoundTagPtr);
        void* outTag = Original_ItemInstanceSave
            ? Original_ItemInstanceSave(thisPtr, compoundTagPtr)
            : compoundTagPtr;
        return outTag;
    }

    void __fastcall Hooked_ItemInstanceLoad(void* thisPtr, void* compoundTagPtr)
    {
        if (Original_ItemInstanceLoad)
            Original_ItemInstanceLoad(thisPtr, compoundTagPtr);
        WorldIdRemap::RemapItemInstanceFromTag(thisPtr, compoundTagPtr);
    }

    bool __fastcall Hooked_ItemMineBlock(void* thisPtr, void* itemInstanceSharedPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr)
    {
        s_itemMineBlockHookCalls++;

        void* itemInstancePtr = DecodeItemInstancePtrFromSharedArg(itemInstanceSharedPtr);
        int action = TryDispatchMineBlockFromItemInstancePtr(itemInstancePtr, tile, x, y, z, "Item::mineBlock");
        if (action == 2)
            return true;

        if (Original_ItemMineBlock)
            return Original_ItemMineBlock(thisPtr, itemInstanceSharedPtr, level, tile, x, y, z, ownerSharedPtr);
        return false;
    }

    bool __fastcall Hooked_DiggerItemMineBlock(void* thisPtr, void* itemInstanceSharedPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr)
    {
        s_itemMineBlockHookCalls++;

        void* itemInstancePtr = DecodeItemInstancePtrFromSharedArg(itemInstanceSharedPtr);
        int action = TryDispatchMineBlockFromItemInstancePtr(itemInstancePtr, tile, x, y, z, "DiggerItem::mineBlock");
        if (action == 2)
            return true;

        if (Original_DiggerItemMineBlock)
            return Original_DiggerItemMineBlock(thisPtr, itemInstanceSharedPtr, level, tile, x, y, z, ownerSharedPtr);
        return false;
    }

    static bool TryReadItemIdFromPickaxe(void* pickaxeItemPtr, int& outItemId)
    {
        if (!pickaxeItemPtr || !IsReadableRange(pickaxeItemPtr, kItemIdOffset + sizeof(int)))
            return false;
        int itemId = *reinterpret_cast<const int*>(static_cast<const char*>(pickaxeItemPtr) + kItemIdOffset);
        if (itemId >= 0 && itemId < 32000)
        {
            outItemId = itemId;
            return true;
        }
        return false;
    }

    static constexpr int TILE_NUM_COUNT = 4096;

    static bool TryReadTileId(void* tilePtr, int& outTileId)
    {
        if (!tilePtr)
            return false;
        if (IsReadableRange(tilePtr, kTileIdOffset + sizeof(int)))
        {
            int id = *reinterpret_cast<const int*>(static_cast<const char*>(tilePtr) + kTileIdOffset);
            if (id >= 0 && id < TILE_NUM_COUNT)
            {
                outTileId = id;
                return true;
            }
        }
        // Fallback: resolve via Tile::tiles (Tile** - pointer to array). Must dereference once.
        if (s_tileTilesArray && IsReadableRange(s_tileTilesArray, sizeof(void*)))
        {
            const void* arrayPtr = *reinterpret_cast<const void* const*>(s_tileTilesArray);
            if (arrayPtr && IsReadableRange(arrayPtr, TILE_NUM_COUNT * sizeof(void*)))
            {
                const void* const* tiles = reinterpret_cast<const void* const*>(arrayPtr);
                for (int i = 0; i < TILE_NUM_COUNT; i++)
                {
                    if (tiles[i] == tilePtr)
                    {
                        outTileId = i;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    static int GetToolHarvestLevel(void* diggerItemPtr, int itemId)
    {
        const CustomToolMaterialRegistry::Definition* def = CustomToolMaterialRegistry::Find(itemId);
        if (def)
            return def->harvestLevel;
        if (!diggerItemPtr || !IsReadableRange(diggerItemPtr, 0xA8 + sizeof(void*)))
            return -1;
        const void* tierPtr = *reinterpret_cast<const void* const*>(static_cast<const char*>(diggerItemPtr) + 0xA8);
        if (!tierPtr || !IsReadableRange(tierPtr, sizeof(int)))
            return -1;
        return *reinterpret_cast<const int*>(tierPtr);
    }

    static float GetToolDestroySpeed(void* diggerItemPtr, int itemId)
    {
        const CustomToolMaterialRegistry::Definition* def = CustomToolMaterialRegistry::Find(itemId);
        if (def)
            return def->destroySpeed;
        if (!diggerItemPtr || !IsReadableRange(static_cast<const char*>(diggerItemPtr) + 0xA0, sizeof(float)))
            return 1.0f;
        return *reinterpret_cast<const float*>(static_cast<const char*>(diggerItemPtr) + 0xA0);
    }

    float __fastcall Hooked_PickaxeItemGetDestroySpeed(void* thisPtr, void* itemInstanceSharedPtr, void* tilePtr)
    {
        void* itemInstancePtr = DecodeItemInstancePtrFromSharedArg(itemInstanceSharedPtr);
        int itemId = 0;
        if (!TryReadItemId(itemInstancePtr, itemId))
        {
            if (Original_PickaxeItemGetDestroySpeed)
                return Original_PickaxeItemGetDestroySpeed(thisPtr, itemInstanceSharedPtr, tilePtr);
            return 1.0f;
        }

        int tileId = 0;
        if (tilePtr && TryReadTileId(tilePtr, tileId))
        {
            const CustomBlockRegistry::Definition* blockDef = CustomBlockRegistry::Find(tileId);
            if (blockDef && blockDef->requiredTool == CustomBlockRegistry::ToolType::Pickaxe)
            {
                int harvestLevel = GetToolHarvestLevel(thisPtr, itemId);
                if (harvestLevel >= 0 &&
                    (blockDef->requiredHarvestLevel < 0 || harvestLevel >= blockDef->requiredHarvestLevel))
                {
                    return GetToolDestroySpeed(thisPtr, itemId);
                }
                // Block requires pickaxe but harvest level insufficient - return slow speed
                return 1.0f;
            }
        }

        if (Original_PickaxeItemGetDestroySpeed)
            return Original_PickaxeItemGetDestroySpeed(thisPtr, itemInstanceSharedPtr, tilePtr);
        return 1.0f;
    }

    bool __fastcall Hooked_PickaxeItemCanDestroySpecial(void* thisPtr, void* tilePtr)
    {
        int itemId = 0;
        if (!TryReadItemIdFromPickaxe(thisPtr, itemId))
        {
            if (Original_PickaxeItemCanDestroySpecial)
                return Original_PickaxeItemCanDestroySpecial(thisPtr, tilePtr);
            return false;
        }

        int tileId = 0;
        if (tilePtr && TryReadTileId(tilePtr, tileId))
        {
            const CustomBlockRegistry::Definition* blockDef = CustomBlockRegistry::Find(tileId);
            if (blockDef && blockDef->requiredTool == CustomBlockRegistry::ToolType::Pickaxe)
            {
                int harvestLevel = GetToolHarvestLevel(thisPtr, itemId);
                if (harvestLevel >= 0 &&
                    (blockDef->requiredHarvestLevel < 0 || harvestLevel >= blockDef->requiredHarvestLevel))
                {
                    return true;
                }
                return false;
            }
        }

        if (Original_PickaxeItemCanDestroySpecial)
            return Original_PickaxeItemCanDestroySpecial(thisPtr, tilePtr);
        return false;
    }

    float __fastcall Hooked_ShovelItemGetDestroySpeed(void* thisPtr, void* itemInstanceSharedPtr, void* tilePtr)
    {
        void* itemInstancePtr = DecodeItemInstancePtrFromSharedArg(itemInstanceSharedPtr);
        int itemId = 0;
        if (!TryReadItemId(itemInstancePtr, itemId))
        {
            if (Original_ShovelItemGetDestroySpeed)
                return Original_ShovelItemGetDestroySpeed(thisPtr, itemInstanceSharedPtr, tilePtr);
            return 1.0f;
        }

        int tileId = 0;
        if (tilePtr && TryReadTileId(tilePtr, tileId))
        {
            const CustomBlockRegistry::Definition* blockDef = CustomBlockRegistry::Find(tileId);
            if (blockDef && blockDef->requiredTool == CustomBlockRegistry::ToolType::Shovel)
            {
                int harvestLevel = GetToolHarvestLevel(thisPtr, itemId);
                if (harvestLevel >= 0 &&
                    (blockDef->requiredHarvestLevel < 0 || harvestLevel >= blockDef->requiredHarvestLevel))
                {
                    return GetToolDestroySpeed(thisPtr, itemId);
                }
                return 1.0f;
            }
        }

        if (Original_ShovelItemGetDestroySpeed)
            return Original_ShovelItemGetDestroySpeed(thisPtr, itemInstanceSharedPtr, tilePtr);
        return 1.0f;
    }

    bool __fastcall Hooked_ShovelItemCanDestroySpecial(void* thisPtr, void* tilePtr)
    {
        int itemId = 0;
        if (!TryReadItemIdFromPickaxe(thisPtr, itemId))
        {
            if (Original_ShovelItemCanDestroySpecial)
                return Original_ShovelItemCanDestroySpecial(thisPtr, tilePtr);
            return false;
        }

        int tileId = 0;
        if (tilePtr && TryReadTileId(tilePtr, tileId))
        {
            const CustomBlockRegistry::Definition* blockDef = CustomBlockRegistry::Find(tileId);
            if (blockDef && blockDef->requiredTool == CustomBlockRegistry::ToolType::Shovel)
            {
                int harvestLevel = GetToolHarvestLevel(thisPtr, itemId);
                if (harvestLevel >= 0 &&
                    (blockDef->requiredHarvestLevel < 0 || harvestLevel >= blockDef->requiredHarvestLevel))
                {
                    return true;
                }
                return false;
            }
        }

        if (Original_ShovelItemCanDestroySpecial)
            return Original_ShovelItemCanDestroySpecial(thisPtr, tilePtr);
        return false;
    }

    // Inventory layout: items.data at +0x8, selected at +0x28. shared_ptr is 16 bytes.
    static void* GetSelectedItemInstanceFromPlayer(void* playerPtr)
    {
        void* inv = FindInventoryPtrFromPlayer(playerPtr);
        if (!inv || !IsReadableRange(inv, 0x30))
            return nullptr;
        void* itemsData = *reinterpret_cast<void* const*>(static_cast<const char*>(inv) + 0x8);
        int selected = *reinterpret_cast<const int*>(static_cast<const char*>(inv) + 0x28);
        if (!itemsData || selected < 0 || selected >= 36)
            return nullptr;
        // shared_ptr<ItemInstance> at itemsData[selected]; raw ptr is first 8 bytes
        const char* slotPtr = static_cast<const char*>(itemsData) + selected * 16;
        if (!IsReadableRange(slotPtr, 8))
            return nullptr;
        return *reinterpret_cast<void* const*>(slotPtr);
    }

    bool __fastcall Hooked_PlayerCanDestroy(void* thisPtr, void* tilePtr)
    {
        // For pickaxe harvest rules, Inventory::canDestroy -> ItemInstance::canDestroySpecial
        // already gives the correct source behavior:
        // proper tool/tier => normal speed + drops
        // insufficient tool/tier => slow break + no drops
        if (Original_PlayerCanDestroy)
            return Original_PlayerCanDestroy(thisPtr, tilePtr);
        return false;
    }

    bool __fastcall Hooked_ServerPlayerGameModeUseItem(void* thisPtr, void* playerSharedPtr, void* level, void* itemInstanceSharedPtr, bool bTestUseOnly)
    {
        void* previousLevel = s_activeUseLevel;
        s_activeUseLevel = level;
        static int s_serverUseLogCount = 0;
        void* itemInstancePtr = DecodeItemInstancePtrFromSharedArg(itemInstanceSharedPtr);
        int itemId = -1;
        TryReadItemId(itemInstancePtr, itemId);
        bool effectiveTestUseOnly = bTestUseOnly;
        const ULONGLONG nowMs = GetTickCount64();
        if (effectiveTestUseOnly &&
            itemId >= 0 &&
            s_pendingServerUseItemId == itemId &&
            nowMs <= s_pendingServerUseExpiryMs)
        {
            effectiveTestUseOnly = false;
            s_pendingServerUseItemId = -1;
            s_pendingServerUseExpiryMs = 0;
        }
        if (s_serverUseLogCount < 40)
        {
            LogUtil::Log("[WeaveLoader] UseHook ServerPlayerGameMode::useItem test=%d effective=%d item=%d itemPtr=%p playerShared=%p level=%p",
                         bTestUseOnly ? 1 : 0, effectiveTestUseOnly ? 1 : 0, itemId, itemInstancePtr, playerSharedPtr, level);
            s_serverUseLogCount++;
        }
        int action = TryDispatchUseItemFromSharedItemArg(itemInstanceSharedPtr, playerSharedPtr, effectiveTestUseOnly, "ServerPlayerGameMode::useItem");
        s_activeUseLevel = previousLevel;
        if (action == 2)
            return true;

        if (Original_ServerPlayerGameModeUseItem)
            return Original_ServerPlayerGameModeUseItem(thisPtr, playerSharedPtr, level, itemInstanceSharedPtr, bTestUseOnly);
        return false;
    }

    bool __fastcall Hooked_MultiPlayerGameModeUseItem(void* thisPtr, void* playerSharedPtr, void* level, void* itemInstanceSharedPtr, bool bTestUseOnly)
    {
        static int s_multiUseLogCount = 0;
        void* itemInstancePtr = DecodeItemInstancePtrFromSharedArg(itemInstanceSharedPtr);
        int itemId = -1;
        TryReadItemId(itemInstancePtr, itemId);
        if (!bTestUseOnly && itemId >= 0)
        {
            s_pendingServerUseItemId = itemId;
            s_pendingServerUseExpiryMs = GetTickCount64() + 1000;
        }
        if (s_multiUseLogCount < 40)
        {
            LogUtil::Log("[WeaveLoader] UseHook MultiPlayerGameMode::useItem test=%d item=%d itemPtr=%p playerShared=%p level=%p",
                         bTestUseOnly ? 1 : 0, itemId, itemInstancePtr, playerSharedPtr, level);
            s_multiUseLogCount++;
        }
        void* previousLevel = s_activeUseLevel;
        s_activeUseLevel = level;
        int action = TryDispatchUseItemFromSharedItemArg(itemInstanceSharedPtr, playerSharedPtr, bTestUseOnly, "MultiPlayerGameMode::useItem");
        s_activeUseLevel = previousLevel;
        if (action == 2)
            return true;
        if (Original_MultiPlayerGameModeUseItem)
            return Original_MultiPlayerGameModeUseItem(thisPtr, playerSharedPtr, level, itemInstanceSharedPtr, bTestUseOnly);
        return false;
    }

    void __fastcall Hooked_MinecraftSetLevel(void* thisPtr, void* level, int message, void* forceInsertPlayerSharedPtr, bool doForceStatsSave, bool bPrimaryPlayerSignedOut)
    {
        if (Original_MinecraftSetLevel)
        {
            Original_MinecraftSetLevel(thisPtr, level, message, forceInsertPlayerSharedPtr, doForceStatsSave, bPrimaryPlayerSignedOut);
        }

        s_currentLevel = level;
    }

    void* Hooked_GetResourceAsStream(const void* fileName)
    {
        const std::wstring* path = static_cast<const std::wstring*>(fileName);
        if (!Original_GetResourceAsStream || !path)
            return Original_GetResourceAsStream ? Original_GetResourceAsStream(fileName) : nullptr;

        int atlasType = -1;
        int page = 0;
        if (ParseVirtualAtlasRequest(*path, atlasType, page))
        {
            std::string atlasPath = ModAtlas::GetVirtualPagePath(atlasType, page);
            if (atlasPath.empty())
                atlasPath = ModAtlas::GetMergedPagePath(atlasType, page);
            if (!atlasPath.empty())
            {
                std::wstring ourPath(atlasPath.begin(), atlasPath.end());
                return Original_GetResourceAsStream(&ourPath);
            }
        }

        std::wstring lower = NormalizeLowerPath(*path);
        const bool isGenerated =
            (lower.find(L"/modloader/") != std::wstring::npos) ||
            (lower.find(L"/mods/modloader/generated/") != std::wstring::npos);

        if (!isGenerated)
        {
            int mipLevel = 0;
            if (TryParseMipmapLevel(lower, L"terrain", mipLevel))
            {
                std::string mipPath = ModAtlas::GetMergedMipmapPath(0, mipLevel);
                if (!mipPath.empty())
                {
                    std::wstring ourPath(mipPath.begin(), mipPath.end());
                    return Original_GetResourceAsStream(&ourPath);
                }
            }
            if (TryParseMipmapLevel(lower, L"items", mipLevel))
            {
                std::string mipPath = ModAtlas::GetMergedMipmapPath(1, mipLevel);
                if (!mipPath.empty())
                {
                    std::wstring ourPath(mipPath.begin(), mipPath.end());
                    return Original_GetResourceAsStream(&ourPath);
                }
            }

            if (EndsWithPath(lower, L"terrain.png"))
            {
                ModAtlas::SetOverrideAtlasPath(0, std::string(path->begin(), path->end()));
                ModAtlas::EnsureAtlasesBuilt();
                std::string terrainPath = ModAtlas::GetMergedTerrainPath();
                if (!terrainPath.empty())
                {
                    std::wstring ourPath(terrainPath.begin(), terrainPath.end());
                    LogUtil::Log("[WeaveLoader] getResourceAsStream: redirecting terrain.png to merged atlas");
                    return Original_GetResourceAsStream(&ourPath);
                }
            }

            if (EndsWithPath(lower, L"items.png"))
            {
                ModAtlas::SetOverrideAtlasPath(1, std::string(path->begin(), path->end()));
                ModAtlas::EnsureAtlasesBuilt();
                std::string itemsPath = ModAtlas::GetMergedItemsPath();
                if (!itemsPath.empty())
                {
                    std::wstring ourPath(itemsPath.begin(), itemsPath.end());
                    LogUtil::Log("[WeaveLoader] getResourceAsStream: redirecting items.png to merged atlas");
                    return Original_GetResourceAsStream(&ourPath);
                }
            }
        }

        std::wstring modAssetPath;
        if (TryResolveModAssetPath(*path, modAssetPath))
        {
            LogUtil::Log("[WeaveLoader] getResourceAsStream: redirecting %ls -> %ls",
                         path->c_str(), modAssetPath.c_str());
            return Original_GetResourceAsStream(&modAssetPath);
        }

        return Original_GetResourceAsStream(fileName);
    }

    static bool s_loggedGetString = false;
    const wchar_t* Hooked_GetString(int id)
    {
        if (ModStrings::IsModId(id))
        {
            const wchar_t* modStr = ModStrings::Get(id);
            LogUtil::Log("[WeaveLoader] GetString(id=%d) -> mod '%ls'", id,
                         (modStr && modStr[0]) ? modStr : L"<null/empty>");
            if (modStr && modStr[0])
                return modStr;
            return L"[Mod]";
        }
        if (!s_loggedGetString && id > 0)
        {
            s_loggedGetString = true;
            const wchar_t* r = Original_GetString ? Original_GetString(id) : L"";
            LogUtil::Log("[WeaveLoader] GetString(id=%d) -> vanilla '%ls' (first call sample)", id, r ? r : L"<null>");
            return r;
        }
        return Original_GetString ? Original_GetString(id) : L"";
    }


    void Hooked_RunStaticCtors()
    {
        LogUtil::Log("[WeaveLoader] Hook: RunStaticCtors -- calling PreInit");
        DotNetHost::CallPreInit();
        s_preInitCalled = true;

        Original_RunStaticCtors();

        WorldIdRemap::EnsureMissingPlaceholders();

        LogUtil::Log("[WeaveLoader] Hook: RunStaticCtors complete -- calling Init");
        DotNetHost::CallInit();
        s_initCalled = true;

        // Inject mod strings directly into the game's StringTable vector.
        // This is necessary because the compiler inlines GetString at call
        // sites like Item::getHoverName, bypassing our GetString hook.
        ModStrings::InjectAllIntoGameTable();
    }

    void __fastcall Hooked_MinecraftTick(void* thisPtr, bool bFirst, bool bUpdateTextures)
    {
        CullSpawnedEntitiesBelowWorld();
        Original_MinecraftTick(thisPtr, bFirst, bUpdateTextures);
        CullSpawnedEntitiesBelowWorld();

        if (bFirst)
        {
            DotNetHost::CallTick();
        }
    }

    void __fastcall Hooked_MinecraftInit(void* thisPtr)
    {
        char baseDir[MAX_PATH] = { 0 };
        GetModuleFileNameA(nullptr, baseDir, MAX_PATH);
        std::string base(baseDir);
        size_t pos = base.find_last_of("\\/");
        if (pos != std::string::npos) base.resize(pos + 1);
        std::string gameResPath = base + "Common\\res\\TitleUpdate\\res";
        std::string virtualAtlasDir = gameResPath + "\\modloader";
        ModAtlas::SetVirtualAtlasDirectory(virtualAtlasDir);

        HMODULE hMod = nullptr;
        std::string modsPath;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&Hooked_MinecraftInit, &hMod) && hMod)
        {
            char dllPath[MAX_PATH] = { 0 };
            if (GetModuleFileNameA(hMod, dllPath, MAX_PATH))
            {
                std::string dllDir(dllPath);
                size_t dllPos = dllDir.find_last_of("\\/");
                if (dllPos != std::string::npos)
                {
                    dllDir.resize(dllPos + 1);
                    modsPath = dllDir + "mods";
                    goto atlas_done;
                }
            }
        }
        modsPath = base + "mods";
    atlas_done:
        s_modsPath = modsPath;
        {
            std::lock_guard<std::mutex> guard(s_modAssetsMutex);
            s_modAssetsIndexed = false;
            s_modAssetRoots.clear();
        }
        NativeExports::SetModsPath(modsPath);
        ModAtlas::SetBasePaths(modsPath, gameResPath);
        ModAtlas::EnsureAtlasesBuilt();

        // Redirect terrain.png/items.png file opens to our merged atlases
        // so the game loads mod textures without modifying vanilla files.
        ModAtlas::InstallCreateFileHook(gameResPath);

        Original_MinecraftInit(thisPtr);

        // Textures are loaded into GPU memory now; remove the redirect.
        ModAtlas::RemoveCreateFileHook();

        // After init, vanilla icons have their source-image pointer (field_0x48)
        // fully populated. Copy it to our mod icons so getSourceHeight() works.
        ModAtlas::FixupModIcons();

        if (!s_preInitCalled)
        {
            LogUtil::Log("[WeaveLoader] Hook: Minecraft::init -- late PreInit fallback");
            DotNetHost::CallPreInit();
            s_preInitCalled = true;
        }
        if (!s_initCalled)
        {
            LogUtil::Log("[WeaveLoader] Hook: Minecraft::init -- late Init fallback");
            DotNetHost::CallInit();
            s_initCalled = true;
            ModStrings::InjectAllIntoGameTable();
        }

        if (!s_postInitCalled)
        {
            LogUtil::Log("[WeaveLoader] Hook: Minecraft::init complete -- calling PostInit");
            DotNetHost::CallPostInit();
            s_postInitCalled = true;
        }
    }

    void __fastcall Hooked_ExitGame(void* thisPtr)
    {
        LogUtil::Log("[WeaveLoader] Hook: ExitGame -- calling Shutdown");
        DotNetHost::CallShutdown();

        Original_ExitGame(thisPtr);
    }

    void Hooked_CreativeStaticCtor()
    {
        LogUtil::Log("[WeaveLoader] Hook: CreativeStaticCtor -- building vanilla creative lists");
        Original_CreativeStaticCtor();

        // Inject AFTER vanilla lists so modded entries are appended to the end
        // of each creative category.
        LogUtil::Log("[WeaveLoader] Hook: CreativeStaticCtor -- injecting modded items last");
        CreativeInventory::InjectItems();

        // Recalculate TabSpec page counts after appending mod items.
        LogUtil::Log("[WeaveLoader] Hook: CreativeStaticCtor -- updating tab page counts");
        CreativeInventory::UpdateTabPageCounts();
    }

    void __fastcall Hooked_MainMenuCustomDraw(void* thisPtr, void* region)
    {
        MainMenuOverlay::NotifyOnMainMenu();
        Original_MainMenuCustomDraw(thisPtr, region);
    }

    void __fastcall Hooked_Present(void* thisPtr)
    {
        MainMenuOverlay::RenderBranding();
        Original_Present(thisPtr);
    }

    void WINAPI Hooked_OutputDebugStringA(const char* lpOutputString)
    {
        if (lpOutputString && lpOutputString[0] != '\0')
        {
            // Strip trailing newlines/carriage returns for clean log output
            size_t len = strlen(lpOutputString);
            while (len > 0 && (lpOutputString[len - 1] == '\n' || lpOutputString[len - 1] == '\r'))
                len--;

            if (len > 0)
                LogUtil::LogGameOutput(lpOutputString, len);
        }

        Original_OutputDebugStringA(lpOutputString);
    }
}
