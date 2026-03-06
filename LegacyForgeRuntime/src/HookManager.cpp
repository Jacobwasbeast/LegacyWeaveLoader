#include "HookManager.h"
#include "GameHooks.h"
#include "SymbolResolver.h"
#include <MinHook.h>
#include <cstdio>

bool HookManager::Install(const SymbolResolver& symbols)
{
    if (MH_Initialize() != MH_OK)
    {
        printf("[LegacyForge] MH_Initialize failed\n");
        return false;
    }

    // Hook MinecraftWorld_RunStaticCtors
    if (symbols.pRunStaticCtors)
    {
        if (MH_CreateHook(symbols.pRunStaticCtors,
                          reinterpret_cast<void*>(&GameHooks::Hooked_RunStaticCtors),
                          reinterpret_cast<void**>(&GameHooks::Original_RunStaticCtors)) != MH_OK)
        {
            printf("[LegacyForge] Failed to hook RunStaticCtors\n");
            return false;
        }
    }

    // Hook Minecraft::tick
    if (symbols.pMinecraftTick)
    {
        if (MH_CreateHook(symbols.pMinecraftTick,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftTick),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftTick)) != MH_OK)
        {
            printf("[LegacyForge] Failed to hook Minecraft::tick\n");
            return false;
        }
    }

    // Hook Minecraft::init
    if (symbols.pMinecraftInit)
    {
        if (MH_CreateHook(symbols.pMinecraftInit,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftInit),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftInit)) != MH_OK)
        {
            printf("[LegacyForge] Failed to hook Minecraft::init\n");
            return false;
        }
    }

    // Hook Minecraft::destroy
    if (symbols.pMinecraftDestroy)
    {
        if (MH_CreateHook(symbols.pMinecraftDestroy,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftDestroy),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftDestroy)) != MH_OK)
        {
            printf("[LegacyForge] Failed to hook Minecraft::destroy\n");
            return false;
        }
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        printf("[LegacyForge] MH_EnableHook(MH_ALL_HOOKS) failed\n");
        return false;
    }

    printf("[LegacyForge] All hooks installed and enabled\n");
    return true;
}

void HookManager::Cleanup()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
