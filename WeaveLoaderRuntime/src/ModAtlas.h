#pragma once

#include <string>
#include <vector>
#include <unordered_map>

/// <summary>
/// Builds merged terrain.png and items.png atlases from mod assets.
/// Scans mods/*/assets/blocks/*.png and items/*.png, stitches into vanilla atlases.
/// </summary>
namespace ModAtlas
{
    struct ModTextureEntry
    {
        std::wstring iconName;  // e.g. "examplemod:ruby_ore"
        int atlasType;          // 0=blocks/terrain, 1=items
        int row, col;           // Grid position in atlas
    };

    /// Call before textures->stitch(). Returns path to generated dir, or empty if none.
    std::string BuildAtlases(const std::string& modsPath, const std::string& gameResPath);

    /// Get path to merged terrain.png (if built)
    std::string GetMergedTerrainPath();

    /// Get path to merged items.png (if built)
    std::string GetMergedItemsPath();

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

    /// Copy merged atlas PNGs over the game's vanilla atlas files.
    /// Call before Minecraft::init so the game loads our textures.
    void InstallAtlasFiles(const std::string& gameResPath);

    /// Create SimpleIcon objects for our mod textures after loadUVs runs.
    /// Call from loadUVs hook after original returns. atlasType is read from textureMap.
    void CreateModIcons(void* textureMap);

    /// Fix mod icons' internal source-image pointer (field_0x48) by copying
    /// from a fully-initialized vanilla icon. Call AFTER Minecraft::init completes.
    void FixupModIcons();

    /// Look up a mod icon by name. Returns the SimpleIcon* or nullptr if not a mod icon.
    void* LookupModIcon(const std::wstring& name);
}
