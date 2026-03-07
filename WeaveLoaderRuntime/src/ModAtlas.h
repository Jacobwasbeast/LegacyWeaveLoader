#pragma once

#include <string>
#include <vector>
#include <unordered_map>

/// <summary>
/// Builds merged terrain.png and items.png atlases from mod assets.
/// Scans mods/*/assets/blocks/*.png and items/*.png, stitches into copies of the
/// vanilla atlases stored in mods/ModLoader/generated/. A CreateFileW hook redirects
/// the game's file opens to the merged copies so vanilla files are never modified.
/// </summary>
namespace ModAtlas
{
    struct ModTextureEntry
    {
        std::wstring iconName;  // e.g. "examplemod:ruby_ore"
        int atlasType;          // 0=blocks/terrain, 1=items
        int page;               // atlas page index (0 or 1)
        int row, col;           // Grid position in atlas
    };

    /// Call before textures->stitch(). Returns path to generated dir, or empty if none.
    std::string BuildAtlases(const std::string& modsPath, const std::string& gameResPath);
    void SetVirtualAtlasDirectory(const std::string& dir);

    /// Get path to merged terrain.png (if built)
    std::string GetMergedTerrainPath();

    /// Get path to merged items.png (if built)
    std::string GetMergedItemsPath();
    std::string GetMergedPagePath(int atlasType, int page);
    std::string GetVirtualPagePath(int atlasType, int page);
    std::string GetMergedTerrainPage1Path();
    std::string GetMergedItemsPage1Path();

    /// Get mod texture entries for injection into texture map
    const std::vector<ModTextureEntry>& GetBlockEntries();
    const std::vector<ModTextureEntry>& GetItemEntries();

    /// Whether atlases were built (have mod textures)
    bool HasModTextures();

    typedef void* (__fastcall *RegisterIcon_fn)(void* textureMap, const std::wstring& name);

    /// Set resolved symbols for texture injection (called from HookManager)
    void SetInjectSymbols(void* simpleIconCtor, void* operatorNew);

    /// Set the original registerIcon function for vanilla icon lookups
    void SetRegisterIconFn(RegisterIcon_fn fn);

    /// Install a CreateFileW hook that redirects terrain.png/items.png reads to
    /// the merged atlases. Call after BuildAtlases, before Minecraft::init.
    bool InstallCreateFileHook(const std::string& gameResPath);

    /// Remove the CreateFileW hook after init has loaded textures into memory.
    void RemoveCreateFileHook();

    /// Create SimpleIcon objects for our mod textures after loadUVs runs.
    /// Call from loadUVs hook after original returns. atlasType is read from textureMap.
    void CreateModIcons(void* textureMap);

    /// Fix mod icons' internal source-image pointer (field_0x48) by copying
    /// from a fully-initialized vanilla icon. Call AFTER Minecraft::init completes.
    void FixupModIcons();

    /// Look up a mod icon by name. Returns the SimpleIcon* or nullptr if not a mod icon.
    void* LookupModIcon(const std::wstring& name);
    bool TryGetIconRoute(void* iconPtr, int& outAtlasType, int& outPage);

    /// Called by icon UV hooks so bind hooks can route to the right page.
    void NotifyIconSampled(void* iconPtr);

    /// Pop pending page decision from the last icon UV access.
    bool PopPendingPage(int& outAtlasType, int& outPage);
    bool PeekPendingPage(int& outAtlasType, int& outPage);
    void ClearPendingPage();
}
