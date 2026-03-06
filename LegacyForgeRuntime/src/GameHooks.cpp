#include "GameHooks.h"
#include "DotNetHost.h"
#include <cstdio>

namespace GameHooks
{
    RunStaticCtors_fn Original_RunStaticCtors = nullptr;
    MinecraftTick_fn Original_MinecraftTick = nullptr;
    MinecraftInit_fn Original_MinecraftInit = nullptr;
    MinecraftDestroy_fn Original_MinecraftDestroy = nullptr;

    void Hooked_RunStaticCtors()
    {
        printf("[LegacyForge] RunStaticCtors hook fired -- calling PreInit\n");
        DotNetHost::CallPreInit();

        Original_RunStaticCtors();

        printf("[LegacyForge] RunStaticCtors complete -- calling Init\n");
        DotNetHost::CallInit();
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
        Original_MinecraftInit(thisPtr);

        printf("[LegacyForge] Minecraft::init complete -- calling PostInit\n");
        DotNetHost::CallPostInit();
    }

    void __fastcall Hooked_MinecraftDestroy(void* thisPtr)
    {
        printf("[LegacyForge] Minecraft::destroy -- calling Shutdown\n");
        DotNetHost::CallShutdown();

        Original_MinecraftDestroy(thisPtr);
    }
}
