#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

/// Function pointer typedefs matching the game's actual function signatures.
/// x64 MSVC uses __fastcall-like convention (this in rcx, args in rdx/r8/r9).

typedef void (*RunStaticCtors_fn)();
typedef void (__fastcall *MinecraftTick_fn)(void* thisPtr, bool bFirst, bool bUpdateTextures);
typedef void (__fastcall *MinecraftInit_fn)(void* thisPtr);
typedef void (__fastcall *ExitGame_fn)(void* thisPtr);
typedef void (*CreativeStaticCtor_fn)();
typedef void (__fastcall *MainMenuCustomDraw_fn)(void* thisPtr, void* region);
typedef void (__fastcall *Present_fn)(void* thisPtr);
typedef void (WINAPI *OutputDebugStringA_fn)(const char* lpOutputString);
typedef const wchar_t* (*GetString_fn)(int);
typedef void* (*GetResourceAsStream_fn)(const void* fileName);
typedef void (__fastcall *LoadUVs_fn)(void* thisPtr);
typedef void* (__fastcall *RegisterIcon_fn)(void* thisPtr, const std::wstring& name);
typedef void (__fastcall *ItemInstanceMineBlock_fn)(void* thisPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr);
typedef bool (__fastcall *ItemMineBlock_fn)(void* thisPtr, void* itemInstanceSharedPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr);

namespace GameHooks
{
    extern RunStaticCtors_fn      Original_RunStaticCtors;
    extern MinecraftTick_fn       Original_MinecraftTick;
    extern MinecraftInit_fn       Original_MinecraftInit;
    extern ExitGame_fn            Original_ExitGame;
    extern CreativeStaticCtor_fn  Original_CreativeStaticCtor;
    extern MainMenuCustomDraw_fn  Original_MainMenuCustomDraw;
    extern Present_fn             Original_Present;
    extern OutputDebugStringA_fn  Original_OutputDebugStringA;
    extern GetString_fn           Original_GetString;
    extern GetResourceAsStream_fn Original_GetResourceAsStream;
    extern LoadUVs_fn             Original_LoadUVs;
    extern RegisterIcon_fn        Original_RegisterIcon;
    extern ItemInstanceMineBlock_fn Original_ItemInstanceMineBlock;
    extern ItemMineBlock_fn       Original_ItemMineBlock;
    extern ItemMineBlock_fn       Original_DiggerItemMineBlock;

    void Hooked_RunStaticCtors();
    void __fastcall Hooked_MinecraftTick(void* thisPtr, bool bFirst, bool bUpdateTextures);
    void __fastcall Hooked_MinecraftInit(void* thisPtr);
    void __fastcall Hooked_ExitGame(void* thisPtr);
    void Hooked_CreativeStaticCtor();
    void __fastcall Hooked_MainMenuCustomDraw(void* thisPtr, void* region);
    void __fastcall Hooked_Present(void* thisPtr);
    void WINAPI Hooked_OutputDebugStringA(const char* lpOutputString);
    const wchar_t* Hooked_GetString(int id);
    void* Hooked_GetResourceAsStream(const void* fileName);
    void __fastcall Hooked_LoadUVs(void* thisPtr);
    void* __fastcall Hooked_RegisterIcon(void* thisPtr, const std::wstring& name);
    void __fastcall Hooked_ItemInstanceMineBlock(void* thisPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr);
    bool __fastcall Hooked_ItemMineBlock(void* thisPtr, void* itemInstanceSharedPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr);
    bool __fastcall Hooked_DiggerItemMineBlock(void* thisPtr, void* itemInstanceSharedPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr);
}
