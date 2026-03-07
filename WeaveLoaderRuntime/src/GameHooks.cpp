#include "GameHooks.h"
#include "DotNetHost.h"
#include "CreativeInventory.h"
#include "MainMenuOverlay.h"
#include "ModStrings.h"
#include "ModAtlas.h"
#include "LogUtil.h"
#include <Windows.h>
#include <string>
#include <cstdio>
#include <cstring>
#include <cwctype>

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
    RegisterIcon_fn        Original_RegisterIcon = nullptr;
    ItemInstanceGetIcon_fn Original_ItemInstanceGetIcon = nullptr;
    ItemInstanceMineBlock_fn Original_ItemInstanceMineBlock = nullptr;
    ItemMineBlock_fn       Original_ItemMineBlock = nullptr;
    ItemMineBlock_fn       Original_DiggerItemMineBlock = nullptr;
    TexturesBindTextureResource_fn Original_TexturesBindTextureResource = nullptr;
    TexturesLoadTextureByName_fn Original_TexturesLoadTextureByName = nullptr;
    TexturesLoadTextureByIndex_fn Original_TexturesLoadTextureByIndex = nullptr;
    StitchedTextureUV_fn   Original_StitchedGetU0 = nullptr;
    StitchedTextureUV_fn   Original_StitchedGetU1 = nullptr;
    StitchedTextureUV_fn   Original_StitchedGetV0 = nullptr;
    StitchedTextureUV_fn   Original_StitchedGetV1 = nullptr;
    static int s_itemMineBlockHookCalls = 0;
    static void* s_textureAtlasLocationBlocks = nullptr;
    static void* s_textureAtlasLocationItems = nullptr;
    static int s_textureAtlasIdBlocks = -1;
    static int s_textureAtlasIdItems = -1;
    static thread_local bool s_inLoadTextureByNameHook = false;

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

    static void EnsurePageResourcesInitialized()
    {
        if (s_pageResourceInit)
            return;
        s_pageResource.textures = { nullptr, 0 };
        s_pageResource.path = L"";
        s_pageResource.preloaded = false;
        s_pageResourceInit = true;
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

    static bool TryReadItemId(void* itemInstancePtr, int& outItemId)
    {
        if (!itemInstancePtr)
            return false;

        // ItemInstance inherits enable_shared_from_this, so id is not guaranteed at +0x10.
        // Probe known layouts observed across builds.
        static const int kCandidateOffsets[] = { 0x20, 0x18, 0x10, 0x28 };
        for (int off : kCandidateOffsets)
        {
            __try
            {
                int id = *reinterpret_cast<int*>(static_cast<char*>(itemInstancePtr) + off);
                if (id > 0 && id < 32000)
                {
                    outItemId = id;
                    return true;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
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

    static void* DecodeItemInstancePtrFromSharedArg(void* sharedArg)
    {
        if (!sharedArg)
            return nullptr;

        // Candidate A: shared_ptr<ItemInstance> object where first field is raw ItemInstance*.
        __try
        {
            void* p = *reinterpret_cast<void**>(sharedArg);
            if (p)
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

    void __fastcall Hooked_LoadUVs(void* thisPtr)
    {
        LogUtil::Log("[WeaveLoader] Hooked_LoadUVs: ENTER (textureMap=%p)", thisPtr);
        if (Original_LoadUVs)
            Original_LoadUVs(thisPtr);
        LogUtil::Log("[WeaveLoader] Hooked_LoadUVs: original returned, creating mod icons");
        ModAtlas::CreateModIcons(thisPtr);
        LogUtil::Log("[WeaveLoader] Hooked_LoadUVs: DONE");
    }

    float __fastcall Hooked_StitchedGetU0(void* thisPtr, bool adjust)
    {
        int atlasType = -1;
        int page = 0;
        if (ModAtlas::TryGetIconRoute(thisPtr, atlasType, page) && atlasType == 0)
            ModAtlas::NotifyIconSampled(thisPtr);
        return Original_StitchedGetU0 ? Original_StitchedGetU0(thisPtr, adjust) : 0.0f;
    }

    float __fastcall Hooked_StitchedGetU1(void* thisPtr, bool adjust)
    {
        int atlasType = -1;
        int page = 0;
        if (ModAtlas::TryGetIconRoute(thisPtr, atlasType, page) && atlasType == 0)
            ModAtlas::NotifyIconSampled(thisPtr);
        return Original_StitchedGetU1 ? Original_StitchedGetU1(thisPtr, adjust) : 0.0f;
    }

    float __fastcall Hooked_StitchedGetV0(void* thisPtr, bool adjust)
    {
        int atlasType = -1;
        int page = 0;
        if (ModAtlas::TryGetIconRoute(thisPtr, atlasType, page) && atlasType == 0)
            ModAtlas::NotifyIconSampled(thisPtr);
        return Original_StitchedGetV0 ? Original_StitchedGetV0(thisPtr, adjust) : 0.0f;
    }

    float __fastcall Hooked_StitchedGetV1(void* thisPtr, bool adjust)
    {
        int atlasType = -1;
        int page = 0;
        if (ModAtlas::TryGetIconRoute(thisPtr, atlasType, page) && atlasType == 0)
            ModAtlas::NotifyIconSampled(thisPtr);
        return Original_StitchedGetV1 ? Original_StitchedGetV1(thisPtr, adjust) : 0.0f;
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

    static int s_registerIconCallCount = 0;

    void* __fastcall Hooked_RegisterIcon(void* thisPtr, const std::wstring& name)
    {
        s_registerIconCallCount++;
        void* modIcon = ModAtlas::LookupModIcon(name);
        if (modIcon)
        {
            LogUtil::Log("[WeaveLoader] registerIcon #%d: '%ls' -> MOD ICON %p",
                         s_registerIconCallCount, name.c_str(), modIcon);
            return modIcon;
        }
        void* result = Original_RegisterIcon ? Original_RegisterIcon(thisPtr, name) : nullptr;
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

    void* Hooked_GetResourceAsStream(const void* fileName)
    {
        const std::wstring* path = static_cast<const std::wstring*>(fileName);
        if (ModAtlas::HasModTextures() && Original_GetResourceAsStream && path)
        {
            std::string terrainPath = ModAtlas::GetMergedTerrainPath();
            std::string itemsPath = ModAtlas::GetMergedItemsPath();
            if (!terrainPath.empty() && path->find(L"terrain.png") != std::wstring::npos)
            {
                std::wstring ourPath(terrainPath.begin(), terrainPath.end());
                LogUtil::Log("[WeaveLoader] getResourceAsStream: redirecting terrain.png to merged atlas");
                return Original_GetResourceAsStream(&ourPath);
            }
            if (!itemsPath.empty() && path->find(L"items.png") != std::wstring::npos)
            {
                std::wstring ourPath(itemsPath.begin(), itemsPath.end());
                LogUtil::Log("[WeaveLoader] getResourceAsStream: redirecting items.png to merged atlas");
                return Original_GetResourceAsStream(&ourPath);
            }
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
        }
        return Original_GetResourceAsStream ? Original_GetResourceAsStream(fileName) : nullptr;
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

        Original_RunStaticCtors();

        LogUtil::Log("[WeaveLoader] Hook: RunStaticCtors complete -- calling Init");
        DotNetHost::CallInit();

        // Inject mod strings directly into the game's StringTable vector.
        // This is necessary because the compiler inlines GetString at call
        // sites like Item::getHoverName, bypassing our GetString hook.
        ModStrings::InjectAllIntoGameTable();
    }

    void __fastcall Hooked_MinecraftTick(void* thisPtr, bool bFirst, bool bUpdateTextures)
    {
        Original_MinecraftTick(thisPtr, bFirst, bUpdateTextures);

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
                    std::string modsPath = dllDir + "mods";
                    ModAtlas::BuildAtlases(modsPath, gameResPath);
                    goto atlas_done;
                }
            }
        }
        ModAtlas::BuildAtlases(base + "mods", gameResPath);
    atlas_done:

        // Redirect terrain.png/items.png file opens to our merged atlases
        // so the game loads mod textures without modifying vanilla files.
        ModAtlas::InstallCreateFileHook(gameResPath);

        Original_MinecraftInit(thisPtr);

        // Textures are loaded into GPU memory now; remove the redirect.
        ModAtlas::RemoveCreateFileHook();

        // After init, vanilla icons have their source-image pointer (field_0x48)
        // fully populated. Copy it to our mod icons so getSourceHeight() works.
        ModAtlas::FixupModIcons();

        LogUtil::Log("[WeaveLoader] Hook: Minecraft::init complete -- calling PostInit");
        DotNetHost::CallPostInit();
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
