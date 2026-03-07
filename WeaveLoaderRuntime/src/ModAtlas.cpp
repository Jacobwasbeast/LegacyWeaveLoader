#include "ModAtlas.h"
#include "LogUtil.h"
#include <Windows.h>
#include <MinHook.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

namespace ModAtlas
{
    static std::string s_mergedDir;
    static std::string s_virtualAtlasDir;
    static std::string s_mergedTerrainPage1Path;
    static std::string s_mergedItemsPage1Path;
    static std::vector<ModTextureEntry> s_blockEntries;
    static std::vector<ModTextureEntry> s_itemEntries;
    static bool s_hasModTextures = false;
    static bool s_hasTerrainAtlas = false;
    static bool s_hasItemsAtlas = false;
    static bool s_hasTerrainPage0Mods = false;
    static bool s_hasItemsPage0Mods = false;
    static int s_terrainPages = 0;
    static int s_itemPages = 0;
    static void* s_simpleIconCtor = nullptr;
    static void* (*s_operatorNew)(size_t) = nullptr;
    static std::unordered_map<std::wstring, void*> s_modIcons;
    struct IconRouteInfo { int atlasType; int page; };
    static std::unordered_map<void*, IconRouteInfo> s_iconRoutes;
    static RegisterIcon_fn s_originalRegisterIcon = nullptr;
    static thread_local bool s_hasPendingPage = false;
    static thread_local int s_pendingAtlasType = -1;
    static thread_local int s_pendingPage = 0;

    // iconType is at offset 8 in PreStitchedTextureMap (verified via getIconType disassembly)

    static std::string ToLower(const std::string& s)
    {
        std::string r = s;
        for (char& c : r)
            c = (char)tolower((unsigned char)c);
        return r;
    }

    static void FindModTextures(const std::string& modsPath,
                                std::vector<std::pair<std::string, std::string>>& blocks,
                                std::vector<std::pair<std::string, std::string>>& items)
    {
        blocks.clear();
        items.clear();

        WIN32_FIND_DATAA fd;
        std::string search = modsPath + "\\*";
        HANDLE h = FindFirstFileA(search.c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) return;

        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (fd.cFileName[0] == '.') continue;

            std::string modFolder = modsPath + "\\" + fd.cFileName;
            std::string assetsPath = modFolder + "\\assets";
            if (GetFileAttributesA(assetsPath.c_str()) != INVALID_FILE_ATTRIBUTES)
            {
                std::string modId = ToLower(fd.cFileName);
                size_t pos = modId.find('-');
                while (pos != std::string::npos) { modId.erase(pos, 1); pos = modId.find('-'); }

                std::string blocksPath = assetsPath + "\\blocks";
                std::string itemsPath = assetsPath + "\\items";

                auto scanDir = [&](const std::string& dir, std::vector<std::pair<std::string, std::string>>& out, const std::string& prefix)
                {
                    std::string search2 = dir + "\\*.png";
                    HANDLE h2 = FindFirstFileA(search2.c_str(), &fd);
                    if (h2 == INVALID_HANDLE_VALUE) return;
                    do
                    {
                        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                        std::string name = fd.cFileName;
                        name.resize(name.size() - 4);
                        std::string iconName = modId + ":" + name;
                        std::string fullPath = dir + "\\" + fd.cFileName;
                        out.push_back({ iconName, fullPath });
                    } while (FindNextFileA(h2, &fd));
                    FindClose(h2);
                };

                if (GetFileAttributesA(blocksPath.c_str()) == FILE_ATTRIBUTE_DIRECTORY)
                    scanDir(blocksPath, blocks, "blocks");
                if (GetFileAttributesA(itemsPath.c_str()) == FILE_ATTRIBUTE_DIRECTORY)
                    scanDir(itemsPath, items, "items");
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    static bool LoadPng(const std::string& path, int* w, int* h, int* comp, unsigned char** data)
    {
        *data = stbi_load(path.c_str(), w, h, comp, 4);
        return *data != nullptr;
    }

    static bool FileExists(const std::string& path)
    {
        DWORD attr = GetFileAttributesA(path.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
    }

    static std::string BuildPageOutputPath(const std::string& baseDir, const char* stem, int page)
    {
        if (page <= 0) return baseDir + "\\" + stem + ".png";
        return baseDir + "\\" + stem + "_p" + std::to_string(page) + ".png";
    }

    static std::string BuildVirtualPageOutputPath(const std::string& baseDir, const char* stem, int page)
    {
        if (baseDir.empty()) return "";
        if (page <= 0) return baseDir + "\\" + stem + ".png";
        return baseDir + "\\" + stem + "_p" + std::to_string(page) + ".png";
    }

    static void Blit16x16(unsigned char* dst, int dstW, int dstH, int dstX, int dstY,
                          const unsigned char* src, int srcW, int srcH)
    {
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                int sx = (srcW > 0) ? (x * srcW / 16) : 0;
                int sy = (srcH > 0) ? (y * srcH / 16) : 0;
                int di = ((dstY + y) * dstW + (dstX + x)) * 4;
                int si = (sy * srcW + sx) * 4;
                if (si < srcW * srcH * 4 && di < dstW * dstH * 4)
                {
                    dst[di] = src[si];
                    dst[di + 1] = src[si + 1];
                    dst[di + 2] = src[si + 2];
                    dst[di + 3] = src[si + 3];
                }
            }
        }
    }

    static bool IsCellEmpty(const unsigned char* img, int imgW, int imgH,
                            int cellX, int cellY, int cellSize)
    {
        for (int y = 0; y < cellSize; y++)
        {
            for (int x = 0; x < cellSize; x++)
            {
                int px = cellX + x;
                int py = cellY + y;
                if (px >= imgW || py >= imgH) continue;
                int idx = (py * imgW + px) * 4;
                if (img[idx + 3] > 0)
                    return false;
            }
        }
        return true;
    }

    static size_t BuildAtlasPage(const std::string& vanillaPath, const std::string& outPath,
                           const std::vector<std::pair<std::string, std::string>>& modTextures, size_t startIndex,
                           int atlasType, int page,
                           int gridCols, int gridRows, int iconSize,
                           std::vector<ModTextureEntry>& entries)
    {
        int imgW = gridCols * iconSize;
        int imgH = gridRows * iconSize;

        unsigned char* img = (unsigned char*)calloc(imgW * imgH, 4);
        if (!img) return false;

        int vw = 0, vh = 0, vc = 0;
        bool loadedVanilla = false;
        if (!vanillaPath.empty())
        {
            unsigned char* vanilla = stbi_load(vanillaPath.c_str(), &vw, &vh, &vc, 4);
            if (vanilla)
            {
                int copyW = (vw < imgW) ? vw : imgW;
                int copyH = (vh < imgH) ? vh : imgH;
                for (int y = 0; y < copyH; y++)
                    memcpy(img + y * imgW * 4, vanilla + y * vw * 4, copyW * 4);
                stbi_image_free(vanilla);
                loadedVanilla = true;
            }
        }

        if (!loadedVanilla)
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: ERROR could not load vanilla atlas base '%s' (atlasType=%d page=%d)",
                         vanillaPath.c_str(), atlasType, page);
            free(img);
            return 0;
        }

        // Find all empty (fully transparent) cells in the atlas
        std::vector<std::pair<int, int>> emptyCells;
        for (int row = 0; row < gridRows; row++)
        {
            for (int col = 0; col < gridCols; col++)
            {
                if (IsCellEmpty(img, imgW, imgH, col * iconSize, row * iconSize, iconSize))
                    emptyCells.push_back({ row, col });
            }
        }

        LogUtil::Log("[WeaveLoader] ModAtlas: found %zu empty cells in %dx%d atlas",
                     emptyCells.size(), gridCols, gridRows);

        size_t consumed = 0;
        size_t cellIdx = 0;
        for (size_t texIdx = startIndex; texIdx < modTextures.size(); ++texIdx)
        {
            const auto& tex = modTextures[texIdx];
            const std::string& iconName = tex.first;
            const std::string& path = tex.second;

            if (cellIdx >= emptyCells.size())
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: page %d full for atlasType=%d (stopped at %s)",
                             page, atlasType, iconName.c_str());
                break;
            }

            int sw = 0, sh = 0, sc = 0;
            unsigned char* src = nullptr;
            if (!LoadPng(path, &sw, &sh, &sc, &src))
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: failed to load %s", path.c_str());
                continue;
            }

            int row = emptyCells[cellIdx].first;
            int col = emptyCells[cellIdx].second;
            cellIdx++;

            Blit16x16(img, imgW, imgH, col * iconSize, row * iconSize, src, sw, sh);
            stbi_image_free(src);

            std::wstring wname(iconName.begin(), iconName.end());
            entries.push_back({ wname, atlasType, page, row, col });
            consumed++;

            LogUtil::Log("[WeaveLoader] ModAtlas: placed '%s' at page=%d row=%d col=%d",
                         iconName.c_str(), page, row, col);
        }

        std::string dir = outPath.substr(0, outPath.find_last_of("\\/"));
        CreateDirectoryA(dir.c_str(), nullptr);

        int ok = stbi_write_png(outPath.c_str(), imgW, imgH, 4, img, imgW * 4);
        free(img);
        if (!ok) return 0;
        return consumed;
    }

    static size_t BuildModOnlyAtlasPage(const std::string& outPath,
                                  const std::vector<std::pair<std::string, std::string>>& modTextures, size_t startIndex,
                                  int atlasType, int page,
                                  int gridCols, int gridRows, int iconSize,
                                  std::vector<ModTextureEntry>& entries)
    {
        const int imgW = gridCols * iconSize;
        const int imgH = gridRows * iconSize;
        unsigned char* img = (unsigned char*)calloc(imgW * imgH, 4); // transparent base
        if (!img) return 0;

        const size_t capacity = (size_t)gridCols * (size_t)gridRows;
        size_t consumed = 0;
        for (size_t texIdx = startIndex; texIdx < modTextures.size() && consumed < capacity; ++texIdx)
        {
            const auto& tex = modTextures[texIdx];
            const std::string& iconName = tex.first;
            const std::string& path = tex.second;

            int sw = 0, sh = 0, sc = 0;
            unsigned char* src = nullptr;
            if (!LoadPng(path, &sw, &sh, &sc, &src))
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: failed to load %s", path.c_str());
                continue;
            }

            int row = (int)(consumed / (size_t)gridCols);
            int col = (int)(consumed % (size_t)gridCols);
            consumed++;

            Blit16x16(img, imgW, imgH, col * iconSize, row * iconSize, src, sw, sh);
            stbi_image_free(src);

            std::wstring wname(iconName.begin(), iconName.end());
            entries.push_back({ wname, atlasType, page, row, col });
            LogUtil::Log("[WeaveLoader] ModAtlas: placed '%s' at page=%d row=%d col=%d (mod-only)",
                         iconName.c_str(), page, row, col);
        }

        std::string dir = outPath.substr(0, outPath.find_last_of("\\/"));
        CreateDirectoryA(dir.c_str(), nullptr);

        int ok = stbi_write_png(outPath.c_str(), imgW, imgH, 4, img, imgW * 4);
        free(img);
        if (!ok) return 0;
        return consumed;
    }

    std::string BuildAtlases(const std::string& modsPath, const std::string& gameResPath)
    {
        s_blockEntries.clear();
        s_itemEntries.clear();
        s_modIcons.clear();
        s_iconRoutes.clear();
        s_hasModTextures = false;
        s_hasTerrainAtlas = false;
        s_hasItemsAtlas = false;
        s_hasTerrainPage0Mods = false;
        s_hasItemsPage0Mods = false;
        s_terrainPages = 0;
        s_itemPages = 0;
        s_mergedTerrainPage1Path.clear();
        s_mergedItemsPage1Path.clear();

        std::vector<std::pair<std::string, std::string>> blockPaths, itemPaths;
        FindModTextures(modsPath, blockPaths, itemPaths);

        if (blockPaths.empty() && itemPaths.empty())
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: no mod textures found");
            return "";
        }

        char baseDir[MAX_PATH] = { 0 };
        GetModuleFileNameA(nullptr, baseDir, MAX_PATH);
        std::string base(baseDir);
        size_t pos = base.find_last_of("\\/");
        if (pos != std::string::npos) base.resize(pos + 1);

        s_mergedDir = base + "mods\\ModLoader\\generated";
        CreateDirectoryA((base + "mods").c_str(), nullptr);
        CreateDirectoryA((base + "mods\\ModLoader").c_str(), nullptr);
        CreateDirectoryA(s_mergedDir.c_str(), nullptr);
        if (!s_virtualAtlasDir.empty())
            CreateDirectoryA(s_virtualAtlasDir.c_str(), nullptr);

        s_mergedTerrainPage1Path = BuildPageOutputPath(s_mergedDir, "terrain", 1);
        s_mergedItemsPage1Path = BuildPageOutputPath(s_mergedDir, "items", 1);

        std::string vanillaTerrainPath = gameResPath + "\\terrain.png";

        if (!blockPaths.empty())
        {
            size_t cursor = 0;
            int page = 1; // page 0 remains vanilla; modded blocks use dedicated pages
            while (cursor < blockPaths.size())
            {
                std::string outPath = BuildPageOutputPath(s_mergedDir, "terrain", page);
                // World block rendering binds the terrain atlas once for a whole pass.
                // Keep terrain page 1 as a vanilla+mod merged atlas so both vanilla and
                // modded block icons resolve in the same draw pass.
                size_t placed = BuildAtlasPage(vanillaTerrainPath, outPath, blockPaths, cursor, 0, page, 16, 32, 16, s_blockEntries);
                if (placed == 0)
                    break;
                if (!s_virtualAtlasDir.empty() && page > 0)
                {
                    std::string vpath = BuildVirtualPageOutputPath(s_virtualAtlasDir, "terrain", page);
                    if (!vpath.empty())
                        CopyFileA(outPath.c_str(), vpath.c_str(), FALSE);
                }
                cursor += placed;
                page++;
            }

            if (cursor > 0)
            {
                s_hasModTextures = true;
                s_hasTerrainAtlas = true;
                s_terrainPages = page;
                LogUtil::Log("[WeaveLoader] ModAtlas: built terrain pages count=%d", s_terrainPages);
            }

            if (cursor < blockPaths.size())
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: WARNING terrain overflow, dropped %zu textures",
                             blockPaths.size() - cursor);
            }
        }

        if (!itemPaths.empty())
        {
            size_t cursor = 0;
            int page = 1; // page 0 remains vanilla; modded items use dedicated pages
            while (cursor < itemPaths.size())
            {
                std::string outPath = BuildPageOutputPath(s_mergedDir, "items", page);
                size_t placed = BuildModOnlyAtlasPage(outPath, itemPaths, cursor, 1, page, 16, 16, 16, s_itemEntries);
                if (placed == 0)
                    break;
                if (!s_virtualAtlasDir.empty() && page > 0)
                {
                    std::string vpath = BuildVirtualPageOutputPath(s_virtualAtlasDir, "items", page);
                    if (!vpath.empty())
                        CopyFileA(outPath.c_str(), vpath.c_str(), FALSE);
                }
                cursor += placed;
                page++;
            }

            if (cursor > 0)
            {
                s_hasModTextures = true;
                s_hasItemsAtlas = true;
                // page is one past the last generated page index.
                s_itemPages = page;
                LogUtil::Log("[WeaveLoader] ModAtlas: built item pages count=%d", s_itemPages);
            }

            if (cursor < itemPaths.size())
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: WARNING item overflow, dropped %zu textures",
                             itemPaths.size() - cursor);
            }
        }

        return s_hasModTextures ? s_mergedDir : "";
    }

    void SetVirtualAtlasDirectory(const std::string& dir)
    {
        s_virtualAtlasDir = dir;
        if (!s_virtualAtlasDir.empty())
            CreateDirectoryA(s_virtualAtlasDir.c_str(), nullptr);
    }

    std::string GetMergedTerrainPath()
    {
        return s_hasTerrainPage0Mods ? GetMergedPagePath(0, 0) : "";
    }

    std::string GetMergedItemsPath()
    {
        // Never replace vanilla items page 0 unless explicitly needed.
        return s_hasItemsPage0Mods ? GetMergedPagePath(1, 0) : "";
    }

    std::string GetMergedPagePath(int atlasType, int page)
    {
        if (s_mergedDir.empty() || page < 0) return "";
        if (atlasType == 0)
        {
            if (!s_hasTerrainAtlas || page >= s_terrainPages) return "";
            return BuildPageOutputPath(s_mergedDir, "terrain", page);
        }
        if (!s_hasItemsAtlas || page >= s_itemPages) return "";
        return BuildPageOutputPath(s_mergedDir, "items", page);
    }

    std::string GetVirtualPagePath(int atlasType, int page)
    {
        if (s_virtualAtlasDir.empty() || page < 0) return "";
        if (atlasType == 0)
            return BuildVirtualPageOutputPath(s_virtualAtlasDir, "terrain", page);
        return BuildVirtualPageOutputPath(s_virtualAtlasDir, "items", page);
    }

    std::string GetMergedTerrainPage1Path()
    {
        return GetMergedPagePath(0, 1);
    }

    std::string GetMergedItemsPage1Path()
    {
        return GetMergedPagePath(1, 1);
    }

    const std::vector<ModTextureEntry>& GetBlockEntries() { return s_blockEntries; }
    const std::vector<ModTextureEntry>& GetItemEntries() { return s_itemEntries; }
    bool HasModTextures() { return s_hasModTextures; }

    // Per-atlas-type textureMap pointers, saved during CreateModIcons for FixupModIcons.
    static void* s_terrainTextureMap = nullptr;
    static void* s_itemsTextureMap = nullptr;

    // CreateFileW hook: redirect game file opens to merged atlases
    static std::wstring s_mergedTerrainW;
    static std::wstring s_mergedItemsW;
    static std::wstring s_vanillaTerrainW;
    static std::wstring s_vanillaItemsW;

    typedef HANDLE (WINAPI *CreateFileW_fn)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    static CreateFileW_fn s_originalCreateFileW = nullptr;

    static bool EndsWith(const wchar_t* path, const wchar_t* suffix)
    {
        size_t pathLen = wcslen(path);
        size_t suffLen = wcslen(suffix);
        if (suffLen > pathLen) return false;
        return _wcsicmp(path + pathLen - suffLen, suffix) == 0;
    }

    static HANDLE WINAPI Hooked_CreateFileW(
        LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
    {
        if (lpFileName && s_hasModTextures)
        {
            if (!s_mergedTerrainW.empty() && EndsWith(lpFileName, L"\\terrain.png"))
            {
                LogUtil::Log("[WeaveLoader] CreateFileW: redirecting terrain.png to merged atlas");
                return s_originalCreateFileW(s_mergedTerrainW.c_str(), dwDesiredAccess, dwShareMode,
                    lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
            }
            if (!s_mergedItemsW.empty() && EndsWith(lpFileName, L"\\items.png"))
            {
                LogUtil::Log("[WeaveLoader] CreateFileW: redirecting items.png to merged atlas");
                return s_originalCreateFileW(s_mergedItemsW.c_str(), dwDesiredAccess, dwShareMode,
                    lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
            }
        }
        return s_originalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
            lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    bool InstallCreateFileHook(const std::string& gameResPath)
    {
        if (!s_hasModTextures) return false;

        std::string mergedTerrain = GetMergedTerrainPath();
        std::string mergedItems = GetMergedItemsPath();
        if (mergedTerrain.empty() && mergedItems.empty())
            return false;

        if (!mergedTerrain.empty())
            s_mergedTerrainW = std::wstring(mergedTerrain.begin(), mergedTerrain.end());
        if (!mergedItems.empty())
            s_mergedItemsW = std::wstring(mergedItems.begin(), mergedItems.end());

        void* pCreateFileW = reinterpret_cast<void*>(
            GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileW"));
        if (!pCreateFileW)
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: could not find CreateFileW");
            return false;
        }

        if (MH_CreateHook(pCreateFileW, reinterpret_cast<void*>(&Hooked_CreateFileW),
                          reinterpret_cast<void**>(&s_originalCreateFileW)) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: failed to hook CreateFileW");
            return false;
        }
        if (MH_EnableHook(pCreateFileW) != MH_OK)
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: failed to enable CreateFileW hook");
            return false;
        }

        LogUtil::Log("[WeaveLoader] ModAtlas: CreateFileW hook installed (terrain=%s, items=%s)",
                     mergedTerrain.c_str(), mergedItems.c_str());
        return true;
    }

    void RemoveCreateFileHook()
    {
        void* pCreateFileW = reinterpret_cast<void*>(
            GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileW"));
        if (pCreateFileW)
        {
            MH_DisableHook(pCreateFileW);
            MH_RemoveHook(pCreateFileW);
        }
        s_mergedTerrainW.clear();
        s_mergedItemsW.clear();
        LogUtil::Log("[WeaveLoader] ModAtlas: CreateFileW hook removed");
    }

    void SetInjectSymbols(void* simpleIconCtor, void* operatorNew)
    {
        s_simpleIconCtor = simpleIconCtor;
        s_operatorNew = reinterpret_cast<void* (*)(size_t)>(operatorNew);
    }

    void SetRegisterIconFn(RegisterIcon_fn fn)
    {
        s_originalRegisterIcon = fn;
    }

    void CreateModIcons(void* textureMap)
    {
        if (!s_hasModTextures || !s_simpleIconCtor || !textureMap) return;
        if (!s_operatorNew) { LogUtil::Log("[WeaveLoader] ModAtlas: operator new not resolved, skipping icon creation"); return; }

        int iconType = *reinterpret_cast<int*>(reinterpret_cast<char*>(textureMap) + 8);
        LogUtil::Log("[WeaveLoader] ModAtlas: CreateModIcons called for atlas type %d (textureMap=%p)", iconType, textureMap);

        if (iconType == 0) s_terrainTextureMap = textureMap;
        else if (iconType == 1) s_itemsTextureMap = textureMap;

        typedef void (__fastcall* SimpleIconCtor_fn)(void* thisPtr, const std::wstring* name,
            const std::wstring* filename, float u0, float v0, float u1, float v1);

        auto ctor = reinterpret_cast<SimpleIconCtor_fn>(s_simpleIconCtor);

        auto create = [&](const std::vector<ModTextureEntry>& entries, float vertRatio) {
            for (const auto& e : entries)
            {
                if (e.atlasType != iconType) continue;

                float u0 = static_cast<float>(e.col) / 16.0f;
                float v0 = static_cast<float>(e.row) * vertRatio;
                float u1 = static_cast<float>(e.col + 1) / 16.0f;
                float v1 = static_cast<float>(e.row + 1) * vertRatio;

                // SimpleIcon/StitchedTexture contains multiple std::wstring/vector fields.
                // 128 bytes is too small in this binary and causes adjacent-object corruption.
                constexpr size_t kSimpleIconAllocSize = 0x200;
                void* icon = s_operatorNew(kSimpleIconAllocSize);
                if (icon)
                {
                    memset(icon, 0, kSimpleIconAllocSize);
                    ctor(icon, &e.iconName, &e.iconName, u0, v0, u1, v1);
                    s_modIcons[e.iconName] = icon;
                    s_iconRoutes[icon] = { iconType, e.page };
                    LogUtil::Log("[WeaveLoader] ModAtlas: created icon '%ls' (atlas=%d, page=%d, row=%d, col=%d)",
                                 e.iconName.c_str(), iconType, e.page, e.row, e.col);
                }
            }
        };

        if (iconType == 0)
            create(s_blockEntries, 1.0f / 32.0f);
        else if (iconType == 1)
            create(s_itemEntries, 1.0f / 16.0f);

        LogUtil::Log("[WeaveLoader] ModAtlas: s_modIcons now has %zu entries total", s_modIcons.size());
    }

    void FixupModIcons()
    {
        if (s_modIcons.empty() || !s_originalRegisterIcon) return;

        // After Minecraft::init, vanilla icons have field_0x48 properly set.
        // Grab the source-image pointer from a vanilla icon for each atlas type
        // and copy it to our mod icons.
        auto fixForAtlas = [](void* textureMap, int atlasType, const wchar_t* probeName) {
            if (!textureMap) return;

            std::wstring probeStr(probeName);
            void* probeIcon = s_originalRegisterIcon(textureMap, probeStr);
            if (!probeIcon)
            {
                LogUtil::Log("[WeaveLoader] FixupModIcons: could not find vanilla icon '%ls'", probeName);
                return;
            }

            void* srcPtr = *reinterpret_cast<void**>(static_cast<char*>(probeIcon) + 0x48);
            LogUtil::Log("[WeaveLoader] FixupModIcons: vanilla '%ls' field_0x48 = %p", probeName, srcPtr);

            if (!srcPtr)
            {
                LogUtil::Log("[WeaveLoader] FixupModIcons: WARNING - vanilla source ptr still NULL for atlas %d", atlasType);
                return;
            }

            int fixed = 0;
            for (auto& routeKv : s_iconRoutes)
            {
                void* icon = routeKv.first;
                const IconRouteInfo& route = routeKv.second;
                if (route.atlasType != atlasType)
                    continue;

                void* existing = *reinterpret_cast<void**>(static_cast<char*>(icon) + 0x48);
                if (!existing)
                {
                    *reinterpret_cast<void**>(static_cast<char*>(icon) + 0x48) = srcPtr;
                    fixed++;
                }
            }
            LogUtil::Log("[WeaveLoader] FixupModIcons: patched field_0x48 on %d mod icons (atlas %d, srcPtr=%p)",
                         fixed, atlasType, srcPtr);
        };

        fixForAtlas(s_terrainTextureMap, 0, L"stone");
        fixForAtlas(s_itemsTextureMap,   1, L"diamond");
    }

    void* LookupModIcon(const std::wstring& name)
    {
        auto it = s_modIcons.find(name);
        if (it != s_modIcons.end())
            return it->second;
        return nullptr;
    }

    bool TryGetIconRoute(void* iconPtr, int& outAtlasType, int& outPage)
    {
        auto it = s_iconRoutes.find(iconPtr);
        if (it == s_iconRoutes.end())
            return false;
        outAtlasType = it->second.atlasType;
        outPage = it->second.page;
        return true;
    }

    void NotifyIconSampled(void* iconPtr)
    {
        auto it = s_iconRoutes.find(iconPtr);
        if (it != s_iconRoutes.end())
        {
            s_hasPendingPage = true;
            s_pendingAtlasType = it->second.atlasType;
            s_pendingPage = it->second.page;
            return;
        }

        s_hasPendingPage = false;
        s_pendingAtlasType = -1;
        s_pendingPage = 0;
    }

    bool PopPendingPage(int& outAtlasType, int& outPage)
    {
        if (!s_hasPendingPage)
            return false;

        outAtlasType = s_pendingAtlasType;
        outPage = s_pendingPage;
        s_hasPendingPage = false;
        s_pendingAtlasType = -1;
        s_pendingPage = 0;
        return true;
    }

    bool PeekPendingPage(int& outAtlasType, int& outPage)
    {
        if (!s_hasPendingPage)
            return false;
        outAtlasType = s_pendingAtlasType;
        outPage = s_pendingPage;
        return true;
    }

    void ClearPendingPage()
    {
        s_hasPendingPage = false;
        s_pendingAtlasType = -1;
        s_pendingPage = 0;
    }
}
