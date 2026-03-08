#include "HookManager.h"
#include "GameHooks.h"
#include "ModAtlas.h"
#include "ModStrings.h"
#include "SymbolResolver.h"
#include "CreativeInventory.h"
#include "MainMenuOverlay.h"
#include "GameObjectFactory.h"
#include "FurnaceRecipeRegistry.h"
#include "LogUtil.h"
#include <MinHook.h>

bool HookManager::Install(const SymbolResolver& symbols)
{
    if (MH_Initialize() != MH_OK)
    {
        LogUtil::Log("[WeaveLoader] MH_Initialize failed");
        return false;
    }

    if (symbols.pRunStaticCtors)
    {
        if (MH_CreateHook(symbols.pRunStaticCtors,
                          reinterpret_cast<void*>(&GameHooks::Hooked_RunStaticCtors),
                          reinterpret_cast<void**>(&GameHooks::Original_RunStaticCtors)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Failed to hook RunStaticCtors");
            return false;
        }
        LogUtil::Log("[WeaveLoader] Hooked RunStaticCtors");
    }

    if (symbols.pMinecraftTick)
    {
        if (MH_CreateHook(symbols.pMinecraftTick,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftTick),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftTick)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Failed to hook Minecraft::tick");
            return false;
        }
        LogUtil::Log("[WeaveLoader] Hooked Minecraft::tick");
    }

    if (symbols.pMinecraftInit)
    {
        if (MH_CreateHook(symbols.pMinecraftInit,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftInit),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftInit)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Failed to hook Minecraft::init");
            return false;
        }
        LogUtil::Log("[WeaveLoader] Hooked Minecraft::init");
    }

    if (symbols.pMinecraftSetLevel)
    {
        if (MH_CreateHook(symbols.pMinecraftSetLevel,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftSetLevel),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftSetLevel)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Minecraft::setLevel");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Minecraft::setLevel (active level tracking)");
        }
    }

    if (symbols.pItemInstanceMineBlock)
    {
        if (MH_CreateHook(symbols.pItemInstanceMineBlock,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceMineBlock),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceMineBlock)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::mineBlock");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::mineBlock (managed item callbacks)");
        }
    }

    if (symbols.pItemInstanceGetIcon)
    {
        if (MH_CreateHook(symbols.pItemInstanceGetIcon,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemInstanceGetIcon),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemInstanceGetIcon)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ItemInstance::getIcon");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ItemInstance::getIcon (atlas page routing)");
        }
    }

    if (symbols.pItemMineBlock)
    {
        if (MH_CreateHook(symbols.pItemMineBlock,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ItemMineBlock),
                          reinterpret_cast<void**>(&GameHooks::Original_ItemMineBlock)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Item::mineBlock");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Item::mineBlock (managed item callbacks)");
        }
    }

    if (symbols.pDiggerItemMineBlock)
    {
        if (MH_CreateHook(symbols.pDiggerItemMineBlock,
                          reinterpret_cast<void*>(&GameHooks::Hooked_DiggerItemMineBlock),
                          reinterpret_cast<void**>(&GameHooks::Original_DiggerItemMineBlock)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook DiggerItem::mineBlock");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked DiggerItem::mineBlock (managed item callbacks)");
        }
    }

    if (symbols.pServerPlayerGameModeUseItem)
    {
        if (MH_CreateHook(symbols.pServerPlayerGameModeUseItem,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ServerPlayerGameModeUseItem),
                          reinterpret_cast<void**>(&GameHooks::Original_ServerPlayerGameModeUseItem)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ServerPlayerGameMode::useItem");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ServerPlayerGameMode::useItem (managed item callbacks)");
        }
    }

    if (symbols.pMultiPlayerGameModeUseItem)
    {
        if (MH_CreateHook(symbols.pMultiPlayerGameModeUseItem,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MultiPlayerGameModeUseItem),
                          reinterpret_cast<void**>(&GameHooks::Original_MultiPlayerGameModeUseItem)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook MultiPlayerGameMode::useItem");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked MultiPlayerGameMode::useItem (managed item callbacks)");
        }
    }

    GameHooks::SetAtlasLocationPointers(symbols.pTextureAtlasLocationBlocks, symbols.pTextureAtlasLocationItems);

    if (symbols.pTexturesBindTextureResource)
    {
        if (MH_CreateHook(symbols.pTexturesBindTextureResource,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TexturesBindTextureResource),
                          reinterpret_cast<void**>(&GameHooks::Original_TexturesBindTextureResource)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Textures::bindTexture(ResourceLocation)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Textures::bindTexture(ResourceLocation) (atlas page routing)");
        }
    }

    if (symbols.pTexturesLoadTextureByName)
    {
        if (MH_CreateHook(symbols.pTexturesLoadTextureByName,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TexturesLoadTextureByName),
                          reinterpret_cast<void**>(&GameHooks::Original_TexturesLoadTextureByName)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Textures::loadTexture(TEXTURE_NAME,wstring)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Textures::loadTexture(TEXTURE_NAME,wstring) (virtual atlas remap)");
        }
    }

    if (symbols.pTexturesLoadTextureByIndex)
    {
        if (MH_CreateHook(symbols.pTexturesLoadTextureByIndex,
                          reinterpret_cast<void*>(&GameHooks::Hooked_TexturesLoadTextureByIndex),
                          reinterpret_cast<void**>(&GameHooks::Original_TexturesLoadTextureByIndex)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook Textures::loadTexture(int)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked Textures::loadTexture(int) (terrain atlas page routing)");
        }
    }

    if (symbols.pStitchedGetU0)
    {
        if (MH_CreateHook(symbols.pStitchedGetU0,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetU0),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetU0)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getU0");
        }
    }
    if (symbols.pStitchedGetU1)
    {
        if (MH_CreateHook(symbols.pStitchedGetU1,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetU1),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetU1)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getU1");
        }
    }
    if (symbols.pStitchedGetV0)
    {
        if (MH_CreateHook(symbols.pStitchedGetV0,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetV0),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetV0)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getV0");
        }
    }
    if (symbols.pStitchedGetV1)
    {
        if (MH_CreateHook(symbols.pStitchedGetV1,
                          reinterpret_cast<void*>(&GameHooks::Hooked_StitchedGetV1),
                          reinterpret_cast<void**>(&GameHooks::Original_StitchedGetV1)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook StitchedTexture::getV1");
        }
    }

    if (symbols.pExitGame)
    {
        if (MH_CreateHook(symbols.pExitGame,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ExitGame),
                          reinterpret_cast<void**>(&GameHooks::Original_ExitGame)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook ExitGame (shutdown hook unavailable)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked ExitGame");
        }
    }

    GameObjectFactory::ResolveSymbols(const_cast<SymbolResolver&>(symbols));
    FurnaceRecipeRegistry::ResolveSymbols(const_cast<SymbolResolver&>(symbols));
    GameHooks::SetSummonSymbols(
        symbols.pLevelAddEntity,
        symbols.pEntityIONewById,
        symbols.pEntityMoveTo,
        symbols.pEntitySetPos);
    GameHooks::SetUseActionSymbols(
        symbols.pInventoryRemoveResource,
        symbols.pInventoryVtable,
        symbols.pItemInstanceHurtAndBreak,
        symbols.pAbstractContainerMenuBroadcastChanges,
        symbols.pEntityGetLookAngle,
        symbols.pLivingEntityGetViewVector,
        symbols.pEntityLerpMotion,
        symbols.pEntitySetPos);

    if (symbols.pLoadUVs && symbols.pSimpleIconCtor && symbols.pOperatorNew)
    {
        ModAtlas::SetInjectSymbols(symbols.pSimpleIconCtor, symbols.pOperatorNew);
        if (MH_CreateHook(symbols.pLoadUVs,
                          reinterpret_cast<void*>(&GameHooks::Hooked_LoadUVs),
                          reinterpret_cast<void**>(&GameHooks::Original_LoadUVs)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook PreStitchedTextureMap::loadUVs (mod textures may not appear)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked PreStitchedTextureMap::loadUVs (mod texture injection)");
        }

        if (symbols.pRegisterIcon)
        {
            if (MH_CreateHook(symbols.pRegisterIcon,
                              reinterpret_cast<void*>(&GameHooks::Hooked_RegisterIcon),
                              reinterpret_cast<void**>(&GameHooks::Original_RegisterIcon)) != MH_OK)
            {
                LogUtil::Log("[WeaveLoader] Warning: Failed to hook PreStitchedTextureMap::registerIcon");
            }
            else
            {
                LogUtil::Log("[WeaveLoader] Hooked PreStitchedTextureMap::registerIcon (mod icon lookup)");
                // Pass the trampoline to ModAtlas so it can look up vanilla icons
                // for copying internal state (field_0x48 source image pointer).
                ModAtlas::SetRegisterIconFn(GameHooks::Original_RegisterIcon);
            }
        }
    }
    else if (symbols.pLoadUVs)
    {
        LogUtil::Log("[WeaveLoader] Mod texture injection unavailable: SimpleIcon/operator new not resolved");
    }

    if (symbols.pCreativeStaticCtor)
    {
        CreativeInventory::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

        if (MH_CreateHook(symbols.pCreativeStaticCtor,
                          reinterpret_cast<void*>(&GameHooks::Hooked_CreativeStaticCtor),
                          reinterpret_cast<void**>(&GameHooks::Original_CreativeStaticCtor)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook CreativeStaticCtor");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked CreativeStaticCtor");
        }
    }

    if (symbols.pMainMenuCustomDraw)
    {
        if (MH_CreateHook(symbols.pMainMenuCustomDraw,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MainMenuCustomDraw),
                          reinterpret_cast<void**>(&GameHooks::Original_MainMenuCustomDraw)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook MainMenuCustomDraw");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked MainMenuCustomDraw");
        }
    }

    if (symbols.pPresent)
    {
        MainMenuOverlay::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

        if (MH_CreateHook(symbols.pPresent,
                          reinterpret_cast<void*>(&GameHooks::Hooked_Present),
                          reinterpret_cast<void**>(&GameHooks::Original_Present)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook C4JRender::Present");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked C4JRender::Present");
        }
    }

    if (symbols.pGetString)
    {
        // Read GetString prologue bytes BEFORE MinHook overwrites them.
        ModStrings::CaptureStringTableRef(symbols.pGetString);

        if (MH_CreateHook(symbols.pGetString,
                          reinterpret_cast<void*>(&GameHooks::Hooked_GetString),
                          reinterpret_cast<void**>(&GameHooks::Original_GetString)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook CMinecraftApp::GetString (mod names unavailable)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked CMinecraftApp::GetString (mod localization)");
        }
    }


    if (symbols.pGetResourceAsStream)
    {
        if (MH_CreateHook(symbols.pGetResourceAsStream,
                          reinterpret_cast<void*>(&GameHooks::Hooked_GetResourceAsStream),
                          reinterpret_cast<void**>(&GameHooks::Original_GetResourceAsStream)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] Warning: Failed to hook InputStream::getResourceAsStream (mod atlas unavailable)");
        }
        else
        {
            LogUtil::Log("[WeaveLoader] Hooked InputStream::getResourceAsStream (mod textures)");
        }
    }

    {
        void* pOutputDbgStr = reinterpret_cast<void*>(
            GetProcAddress(GetModuleHandleA("kernel32.dll"), "OutputDebugStringA"));
        if (pOutputDbgStr)
        {
            if (MH_CreateHook(pOutputDbgStr,
                              reinterpret_cast<void*>(&GameHooks::Hooked_OutputDebugStringA),
                              reinterpret_cast<void**>(&GameHooks::Original_OutputDebugStringA)) != MH_OK)
            {
                LogUtil::Log("[WeaveLoader] Warning: Failed to hook OutputDebugStringA");
            }
            else
            {
                LogUtil::Log("[WeaveLoader] Hooked OutputDebugStringA (game log capture)");
            }
        }
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        LogUtil::Log("[WeaveLoader] MH_EnableHook(MH_ALL_HOOKS) failed");
        return false;
    }

    LogUtil::Log("[WeaveLoader] All hooks installed and enabled");
    return true;
}

void HookManager::Cleanup()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
