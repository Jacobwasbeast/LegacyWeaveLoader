#pragma once
#include <cstdint>

/// Function pointer typedefs matching the game's function signatures.
/// On x64 MSVC, member functions use __fastcall-like convention (this in rcx).
typedef void (*RunStaticCtors_fn)();
typedef void (__fastcall *MinecraftTick_fn)(void* thisPtr, bool bFirst, bool bUpdateTextures);
typedef void (__fastcall *MinecraftInit_fn)(void* thisPtr);
typedef void (__fastcall *MinecraftDestroy_fn)(void* thisPtr);

namespace GameHooks
{
    extern RunStaticCtors_fn Original_RunStaticCtors;
    extern MinecraftTick_fn Original_MinecraftTick;
    extern MinecraftInit_fn Original_MinecraftInit;
    extern MinecraftDestroy_fn Original_MinecraftDestroy;

    void Hooked_RunStaticCtors();
    void __fastcall Hooked_MinecraftTick(void* thisPtr, bool bFirst, bool bUpdateTextures);
    void __fastcall Hooked_MinecraftInit(void* thisPtr);
    void __fastcall Hooked_MinecraftDestroy(void* thisPtr);
}
