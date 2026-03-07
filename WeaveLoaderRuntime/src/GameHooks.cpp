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
    ItemInstanceMineBlock_fn Original_ItemInstanceMineBlock = nullptr;
    ItemMineBlock_fn       Original_ItemMineBlock = nullptr;
    ItemMineBlock_fn       Original_DiggerItemMineBlock = nullptr;
    static int s_itemMineBlockHookCalls = 0;

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
        // Inject mod items BEFORE vanilla staticCtor so they are included in the
        // TabSpec page-count calculation that happens at the end of staticCtor.
        LogUtil::Log("[WeaveLoader] Hook: CreativeStaticCtor -- injecting modded items first");
        CreativeInventory::InjectItems();

        LogUtil::Log("[WeaveLoader] Hook: CreativeStaticCtor -- building vanilla creative lists");
        Original_CreativeStaticCtor();

        // Safety: recalculate TabSpec page counts in case the injection-before
        // approach didn't fully account for all items (e.g. different binary).
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
