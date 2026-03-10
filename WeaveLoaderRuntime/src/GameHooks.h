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
typedef void (__fastcall *PreStitchedTextureMapStitch_fn)(void* thisPtr);
typedef void* (__fastcall *RegisterIcon_fn)(void* thisPtr, const std::wstring& name);
typedef void* (__fastcall *ItemInstanceGetIcon_fn)(void* thisPtr);
typedef void (__fastcall *EntityRendererBindTextureResource_fn)(void* thisPtr, void* resourcePtr);
typedef void (__fastcall *ItemRendererRenderItemBillboard_fn)(void* thisPtr, void* entitySharedPtr, void* iconPtr, int count, float a, float red, float green, float blue);
typedef void (__fastcall *AnimatedTextureCycleFrames_fn)(void* thisPtr);
typedef int (__fastcall *TextureGetSourceDim_fn)(void* thisPtr);
typedef void (__fastcall *ItemInstanceMineBlock_fn)(void* thisPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr);
typedef void* (__fastcall *ItemInstanceSave_fn)(void* thisPtr, void* compoundTagPtr);
typedef void (__fastcall *ItemInstanceLoad_fn)(void* thisPtr, void* compoundTagPtr);
typedef bool (__fastcall *ItemMineBlock_fn)(void* thisPtr, void* itemInstanceSharedPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr);
typedef float (__fastcall *PickaxeGetDestroySpeed_fn)(void* thisPtr, void* itemInstanceSharedPtr, void* tilePtr);
typedef bool (__fastcall *PickaxeCanDestroySpecial_fn)(void* thisPtr, void* tilePtr);
typedef void (__fastcall *TileOnPlace_fn)(void* thisPtr, void* level, int x, int y, int z);
typedef void (__fastcall *TileNeighborChanged_fn)(void* thisPtr, void* level, int x, int y, int z, int type);
typedef void (__fastcall *TileTick_fn)(void* thisPtr, void* level, int x, int y, int z, void* random);
typedef bool (__fastcall *LevelSetTileAndDataDispatch_fn)(void* thisPtr, int x, int y, int z, int tile, int data, int updateFlags);
typedef bool (__fastcall *LevelSetDataDispatch_fn)(void* thisPtr, int x, int y, int z, int data, int updateFlags, bool forceUpdate);
typedef void (__fastcall *LevelUpdateNeighborsAtDispatch_fn)(void* thisPtr, int x, int y, int z, int type);
typedef bool (__fastcall *ServerLevelTickPendingTicks_fn)(void* thisPtr, bool force);
typedef int (__fastcall *LevelGetTile_fn)(void* thisPtr, int x, int y, int z);
typedef void* (__fastcall *McRegionChunkStorageLoad_fn)(void* thisPtr, void* level, int x, int z);
typedef void (__fastcall *McRegionChunkStorageSave_fn)(void* thisPtr, void* level, void* levelChunk);
typedef int (__fastcall *TileGetResource_fn)(void* thisPtr, int data, void* random, int playerBonusLevel);
typedef int (__fastcall *TileCloneTileId_fn)(void* thisPtr, void* level, int x, int y, int z);
typedef void* (__fastcall *TileGetTextureFaceData_fn)(void* thisPtr, int face, int data);
typedef unsigned int (__fastcall *TileGetDescriptionId_fn)(void* thisPtr, int data);
typedef int (__fastcall *TileGetAuxName_fn)(void* thisPtr, int auxValue);
typedef void (__fastcall *TileRegisterIcons_fn)(void* thisPtr, void* iconRegister);
typedef void* (__fastcall *StoneSlabItemGetIcon_fn)(void* thisPtr, int auxValue);
typedef unsigned int (__fastcall *StoneSlabItemGetDescriptionId_fn)(void* thisPtr, void* itemInstanceSharedPtr);
typedef bool (__fastcall *PlayerCanDestroy_fn)(void* thisPtr, void* tilePtr);
typedef bool (__fastcall *GameModeUseItem_fn)(void* thisPtr, void* playerSharedPtr, void* level, void* itemInstanceSharedPtr, bool bTestUseOnly);
typedef void (__fastcall *MinecraftSetLevel_fn)(void* thisPtr, void* level, int message, void* forceInsertPlayerSharedPtr, bool doForceStatsSave, bool bPrimaryPlayerSignedOut);
typedef bool (__fastcall *LevelAddEntity_fn)(void* thisPtr, void* entitySharedPtr);
typedef void (__fastcall *EntityMoveTo_fn)(void* thisPtr, double x, double y, double z, float yRot, float xRot);
typedef void (__fastcall *EntityIONewById_fn)(void* outSharedPtr, int entityNumericId, void* level);
typedef void (__fastcall *EntitySetPos_fn)(void* thisPtr, double x, double y, double z);
typedef void* (__fastcall *EntityGetLookAngle_fn)(void* thisPtr);
typedef void* (__fastcall *LivingEntityGetViewVector_fn)(void* thisPtr, float partialTicks);
typedef void (__fastcall *EntityLerpMotion_fn)(void* thisPtr, double x, double y, double z);
typedef bool (__fastcall *InventoryRemoveResource_fn)(void* thisPtr, int itemId);
typedef void (__fastcall *ItemInstanceHurtAndBreak_fn)(void* thisPtr, int amount, void* ownerSharedPtr);
typedef void (__fastcall *AbstractContainerMenuBroadcastChanges_fn)(void* thisPtr);
typedef void (__fastcall *TexturesBindTextureResource_fn)(void* thisPtr, void* resourcePtr);
typedef int (__fastcall *TexturesLoadTextureByName_fn)(void* thisPtr, int texId, const std::wstring& resourceName);
typedef int (__fastcall *TexturesLoadTextureByIndex_fn)(void* thisPtr, int idx);
typedef void* (__fastcall *TexturesReadImage_fn)(void* thisPtr, int texId, const std::wstring& name);
typedef float (__fastcall *StitchedTextureUV_fn)(void* thisPtr, bool adjust);
typedef void (__fastcall *BufferedImageCtorFile_fn)(void* thisPtr, const std::wstring& file, bool filenameHasExtension, bool bTitleUpdateTexture, const std::wstring& drive);
typedef void (__fastcall *BufferedImageCtorDLCPack_fn)(void* thisPtr, void* dlcPack, const std::wstring& file, bool filenameHasExtension);
typedef void* (__fastcall *TextureManagerCreateTexture_fn)(void* thisPtr, const std::wstring& name, int mode, int width, int height, int wrap, int format, int minFilter, int magFilter, bool mipmap, void* image);
typedef void (__fastcall *TextureTransferFromImage_fn)(void* thisPtr, void* image);
typedef void* (__fastcall *TexturePackGetImageResource_fn)(void* thisPtr, const std::wstring& file, bool filenameHasExtension, bool bTitleUpdateTexture, const std::wstring& drive);

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
    extern PreStitchedTextureMapStitch_fn Original_PreStitchedTextureMapStitch;
    extern RegisterIcon_fn        Original_RegisterIcon;
    extern ItemInstanceGetIcon_fn Original_ItemInstanceGetIcon;
    extern EntityRendererBindTextureResource_fn Original_EntityRendererBindTextureResource;
    extern ItemRendererRenderItemBillboard_fn Original_ItemRendererRenderItemBillboard;
    extern AnimatedTextureCycleFrames_fn Original_CompassTextureCycleFrames;
    extern AnimatedTextureCycleFrames_fn Original_ClockTextureCycleFrames;
    extern TextureGetSourceDim_fn Original_CompassTextureGetSourceWidth;
    extern TextureGetSourceDim_fn Original_CompassTextureGetSourceHeight;
    extern TextureGetSourceDim_fn Original_ClockTextureGetSourceWidth;
    extern TextureGetSourceDim_fn Original_ClockTextureGetSourceHeight;
    extern ItemInstanceMineBlock_fn Original_ItemInstanceMineBlock;
    extern ItemInstanceSave_fn Original_ItemInstanceSave;
    extern ItemInstanceLoad_fn Original_ItemInstanceLoad;
    extern ItemMineBlock_fn       Original_ItemMineBlock;
    extern ItemMineBlock_fn       Original_DiggerItemMineBlock;
    extern PickaxeGetDestroySpeed_fn Original_PickaxeItemGetDestroySpeed;
    extern PickaxeCanDestroySpecial_fn Original_PickaxeItemCanDestroySpecial;
    extern PickaxeGetDestroySpeed_fn Original_ShovelItemGetDestroySpeed;
    extern PickaxeCanDestroySpecial_fn Original_ShovelItemCanDestroySpecial;
    extern TileOnPlace_fn Original_TileOnPlace;
    extern TileNeighborChanged_fn Original_TileNeighborChanged;
    extern TileTick_fn Original_TileTick;
    extern LevelSetTileAndDataDispatch_fn Original_LevelSetTileAndData;
    extern LevelSetDataDispatch_fn Original_LevelSetData;
    extern LevelUpdateNeighborsAtDispatch_fn Original_LevelUpdateNeighborsAt;
    extern ServerLevelTickPendingTicks_fn Original_ServerLevelTickPendingTicks;
    extern TileGetResource_fn Original_TileGetResource;
    extern McRegionChunkStorageLoad_fn Original_McRegionChunkStorageLoad;
    extern McRegionChunkStorageSave_fn Original_McRegionChunkStorageSave;
    extern TileCloneTileId_fn Original_TileCloneTileId;
    extern TileGetTextureFaceData_fn Original_StoneSlabGetTexture;
    extern TileGetTextureFaceData_fn Original_WoodSlabGetTexture;
    extern TileGetResource_fn Original_StoneSlabGetResource;
    extern TileGetResource_fn Original_WoodSlabGetResource;
    extern TileGetDescriptionId_fn Original_StoneSlabGetDescriptionId;
    extern TileGetDescriptionId_fn Original_WoodSlabGetDescriptionId;
    extern TileGetAuxName_fn Original_StoneSlabGetAuxName;
    extern TileGetAuxName_fn Original_WoodSlabGetAuxName;
    extern TileRegisterIcons_fn Original_StoneSlabRegisterIcons;
    extern TileRegisterIcons_fn Original_WoodSlabRegisterIcons;
    extern StoneSlabItemGetIcon_fn Original_StoneSlabItemGetIcon;
    extern StoneSlabItemGetDescriptionId_fn Original_StoneSlabItemGetDescriptionId;
    extern TileCloneTileId_fn Original_HalfSlabCloneTileId;
    extern PlayerCanDestroy_fn Original_PlayerCanDestroy;
    extern GameModeUseItem_fn     Original_ServerPlayerGameModeUseItem;
    extern GameModeUseItem_fn     Original_MultiPlayerGameModeUseItem;
    extern MinecraftSetLevel_fn   Original_MinecraftSetLevel;
    extern TexturesBindTextureResource_fn Original_TexturesBindTextureResource;
    extern TexturesLoadTextureByName_fn Original_TexturesLoadTextureByName;
    extern TexturesLoadTextureByIndex_fn Original_TexturesLoadTextureByIndex;
    extern TexturesReadImage_fn Original_TexturesReadImage;
    extern StitchedTextureUV_fn   Original_StitchedGetU0;
    extern StitchedTextureUV_fn   Original_StitchedGetU1;
    extern StitchedTextureUV_fn   Original_StitchedGetV0;
    extern StitchedTextureUV_fn   Original_StitchedGetV1;
    extern BufferedImageCtorFile_fn Original_BufferedImageCtorFile;
    extern BufferedImageCtorDLCPack_fn Original_BufferedImageCtorDLCPack;
    extern TextureManagerCreateTexture_fn Original_TextureManagerCreateTexture;
    extern TextureTransferFromImage_fn Original_TextureTransferFromImage;
    extern TexturePackGetImageResource_fn Original_AbstractTexturePackGetImageResource;
    extern TexturePackGetImageResource_fn Original_DLCTexturePackGetImageResource;

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
    void __fastcall Hooked_PreStitchedTextureMapStitch(void* thisPtr);
    void* __fastcall Hooked_RegisterIcon(void* thisPtr, const std::wstring& name);
    void* __fastcall Hooked_ItemInstanceGetIcon(void* thisPtr);
    void __fastcall Hooked_EntityRendererBindTextureResource(void* thisPtr, void* resourcePtr);
    void __fastcall Hooked_ItemRendererRenderItemBillboard(void* thisPtr, void* entitySharedPtr, void* iconPtr, int count, float a, float red, float green, float blue);
    void __fastcall Hooked_CompassTextureCycleFrames(void* thisPtr);
    void __fastcall Hooked_ClockTextureCycleFrames(void* thisPtr);
    int __fastcall Hooked_CompassTextureGetSourceWidth(void* thisPtr);
    int __fastcall Hooked_CompassTextureGetSourceHeight(void* thisPtr);
    int __fastcall Hooked_ClockTextureGetSourceWidth(void* thisPtr);
    int __fastcall Hooked_ClockTextureGetSourceHeight(void* thisPtr);
    void __fastcall Hooked_ItemInstanceMineBlock(void* thisPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr);
    void* __fastcall Hooked_ItemInstanceSave(void* thisPtr, void* compoundTagPtr);
    void __fastcall Hooked_ItemInstanceLoad(void* thisPtr, void* compoundTagPtr);
    bool __fastcall Hooked_ItemMineBlock(void* thisPtr, void* itemInstanceSharedPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr);
    bool __fastcall Hooked_DiggerItemMineBlock(void* thisPtr, void* itemInstanceSharedPtr, void* level, int tile, int x, int y, int z, void* ownerSharedPtr);
    float __fastcall Hooked_PickaxeItemGetDestroySpeed(void* thisPtr, void* itemInstanceSharedPtr, void* tilePtr);
    bool __fastcall Hooked_PickaxeItemCanDestroySpecial(void* thisPtr, void* tilePtr);
    float __fastcall Hooked_ShovelItemGetDestroySpeed(void* thisPtr, void* itemInstanceSharedPtr, void* tilePtr);
    bool __fastcall Hooked_ShovelItemCanDestroySpecial(void* thisPtr, void* tilePtr);
    void __fastcall Hooked_TileOnPlace(void* thisPtr, void* level, int x, int y, int z);
    void __fastcall Hooked_TileNeighborChanged(void* thisPtr, void* level, int x, int y, int z, int type);
    void __fastcall Hooked_TileTick(void* thisPtr, void* level, int x, int y, int z, void* random);
    bool __fastcall Hooked_LevelSetTileAndData(void* thisPtr, int x, int y, int z, int tile, int data, int updateFlags);
    bool __fastcall Hooked_LevelSetData(void* thisPtr, int x, int y, int z, int data, int updateFlags, bool forceUpdate);
    void __fastcall Hooked_LevelUpdateNeighborsAt(void* thisPtr, int x, int y, int z, int type);
    bool __fastcall Hooked_ServerLevelTickPendingTicks(void* thisPtr, bool force);
    void* __fastcall Hooked_McRegionChunkStorageLoad(void* thisPtr, void* level, int x, int z);
    void __fastcall Hooked_McRegionChunkStorageSave(void* thisPtr, void* level, void* levelChunk);
    int __fastcall Hooked_TileGetResource(void* thisPtr, int data, void* random, int playerBonusLevel);
    int __fastcall Hooked_TileCloneTileId(void* thisPtr, void* level, int x, int y, int z);
    void* __fastcall Hooked_StoneSlabGetTexture(void* thisPtr, int face, int data);
    void* __fastcall Hooked_WoodSlabGetTexture(void* thisPtr, int face, int data);
    int __fastcall Hooked_StoneSlabGetResource(void* thisPtr, int data, void* random, int playerBonusLevel);
    int __fastcall Hooked_WoodSlabGetResource(void* thisPtr, int data, void* random, int playerBonusLevel);
    unsigned int __fastcall Hooked_StoneSlabGetDescriptionId(void* thisPtr, int data);
    unsigned int __fastcall Hooked_WoodSlabGetDescriptionId(void* thisPtr, int data);
    int __fastcall Hooked_StoneSlabGetAuxName(void* thisPtr, int auxValue);
    int __fastcall Hooked_WoodSlabGetAuxName(void* thisPtr, int auxValue);
    void __fastcall Hooked_StoneSlabRegisterIcons(void* thisPtr, void* iconRegister);
    void __fastcall Hooked_WoodSlabRegisterIcons(void* thisPtr, void* iconRegister);
    void* __fastcall Hooked_StoneSlabItemGetIcon(void* thisPtr, int auxValue);
    unsigned int __fastcall Hooked_StoneSlabItemGetDescriptionId(void* thisPtr, void* itemInstanceSharedPtr);
    int __fastcall Hooked_HalfSlabCloneTileId(void* thisPtr, void* level, int x, int y, int z);
    bool __fastcall Hooked_PlayerCanDestroy(void* thisPtr, void* tilePtr);
    bool __fastcall Hooked_ServerPlayerGameModeUseItem(void* thisPtr, void* playerSharedPtr, void* level, void* itemInstanceSharedPtr, bool bTestUseOnly);
    bool __fastcall Hooked_MultiPlayerGameModeUseItem(void* thisPtr, void* playerSharedPtr, void* level, void* itemInstanceSharedPtr, bool bTestUseOnly);
    void __fastcall Hooked_MinecraftSetLevel(void* thisPtr, void* level, int message, void* forceInsertPlayerSharedPtr, bool doForceStatsSave, bool bPrimaryPlayerSignedOut);
    void __fastcall Hooked_TexturesBindTextureResource(void* thisPtr, void* resourcePtr);
    int __fastcall Hooked_TexturesLoadTextureByName(void* thisPtr, int texId, const std::wstring& resourceName);
    int __fastcall Hooked_TexturesLoadTextureByIndex(void* thisPtr, int idx);
    void* __fastcall Hooked_TexturesReadImage(void* thisPtr, int texId, const std::wstring& name);
    float __fastcall Hooked_StitchedGetU0(void* thisPtr, bool adjust);
    float __fastcall Hooked_StitchedGetU1(void* thisPtr, bool adjust);
    float __fastcall Hooked_StitchedGetV0(void* thisPtr, bool adjust);
    float __fastcall Hooked_StitchedGetV1(void* thisPtr, bool adjust);
    void __fastcall Hooked_BufferedImageCtorFile(void* thisPtr, const std::wstring& file, bool filenameHasExtension, bool bTitleUpdateTexture, const std::wstring& drive);
    void __fastcall Hooked_BufferedImageCtorDLCPack(void* thisPtr, void* dlcPack, const std::wstring& file, bool filenameHasExtension);
    void* __fastcall Hooked_TextureManagerCreateTexture(void* thisPtr, const std::wstring& name, int mode, int width, int height, int wrap, int format, int minFilter, int magFilter, bool mipmap, void* image);
    void __fastcall Hooked_TextureTransferFromImage(void* thisPtr, void* image);
    void* __fastcall Hooked_AbstractTexturePackGetImageResource(void* thisPtr, const std::wstring& file, bool filenameHasExtension, bool bTitleUpdateTexture, const std::wstring& drive);
    void* __fastcall Hooked_DLCTexturePackGetImageResource(void* thisPtr, const std::wstring& file, bool filenameHasExtension, bool bTitleUpdateTexture, const std::wstring& drive);
    void SetAtlasLocationPointers(void* blocksLocation, void* itemsLocation);
    void SetTileTilesArray(void* tilesArray);
    void SetSummonSymbols(void* levelAddEntity,
                          void* entityIoNewById,
                          void* entityMoveTo,
                          void* entitySetPos);
    void SetUseActionSymbols(void* inventoryRemoveResource,
                             void* inventoryVtable,
                             void* itemInstanceHurtAndBreak,
                             void* containerBroadcastChanges,
                             void* entityGetLookAngle,
                             void* livingEntityGetViewVector,
                             void* entityLerpMotion,
                             void* entitySetPos);
    void SetBlockHelperSymbols(void* tileGetTextureFaceData);
    void SetManagedBlockDispatchSymbols(void* levelGetTile);
    void EnqueueManagedBlockTick(void* levelPtr, int x, int y, int z, int blockId, int delay);
    bool SummonEntityByNumericId(int entityNumericId, double x, double y, double z);
    bool ConsumePlayerResource(void* playerPtr, int itemId, int count);
    bool DamageItemInstance(void* itemInstancePtr, int amount, void* ownerSharedPtr);
    bool SummonEntityFromPlayerLook(void* playerPtr,
                                    void* playerSharedPtr,
                                    int entityNumericId,
                                    double speed,
                                    double spawnForward,
                                    double spawnUp);
}
