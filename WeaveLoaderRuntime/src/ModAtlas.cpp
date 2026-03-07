#include "ModAtlas.h"
#include "LogUtil.h"
#include <Windows.h>
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
    static std::vector<ModTextureEntry> s_blockEntries;
    static std::vector<ModTextureEntry> s_itemEntries;
    static bool s_hasModTextures = false;
    static void* s_simpleIconCtor = nullptr;
    static void* (*s_operatorNew)(size_t) = nullptr;

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

    static bool BuildAtlas(const std::string& vanillaPath, const std::string& outPath,
                           const std::vector<std::pair<std::string, std::string>>& modTextures,
                           int gridCols, int gridRows, int iconSize,
                           std::vector<ModTextureEntry>& entries)
    {
        int imgW = gridCols * iconSize;
        int imgH = gridRows * iconSize;

        unsigned char* img = (unsigned char*)calloc(imgW * imgH, 4);
        if (!img) return false;

        int vw = 0, vh = 0, vc = 0;
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
            }
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

        size_t cellIdx = 0;
        for (const auto& tex : modTextures)
        {
            const std::string& iconName = tex.first;
            const std::string& path = tex.second;

            if (cellIdx >= emptyCells.size())
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: no empty cells left for %s!", iconName.c_str());
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
            entries.push_back({ wname, 0, row, col });

            LogUtil::Log("[WeaveLoader] ModAtlas: placed '%s' at row=%d col=%d", iconName.c_str(), row, col);
        }

        std::string dir = outPath.substr(0, outPath.find_last_of("\\/"));
        CreateDirectoryA(dir.c_str(), nullptr);

        int ok = stbi_write_png(outPath.c_str(), imgW, imgH, 4, img, imgW * 4);
        free(img);
        return ok != 0;
    }

    std::string BuildAtlases(const std::string& modsPath, const std::string& gameResPath)
    {
        s_blockEntries.clear();
        s_itemEntries.clear();
        s_hasModTextures = false;

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

        std::string vanillaTerrain = gameResPath + "\\terrain.png";
        std::string vanillaItems = gameResPath + "\\items.png";
        std::string outTerrain = s_mergedDir + "\\terrain.png";
        std::string outItems = s_mergedDir + "\\items.png";

        if (!blockPaths.empty())
        {
            if (BuildAtlas(vanillaTerrain, outTerrain, blockPaths, 16, 32, 16, s_blockEntries))
            {
                s_hasModTextures = true;
                LogUtil::Log("[WeaveLoader] ModAtlas: built terrain.png with %zu mod textures", s_blockEntries.size());
            }
        }

        if (!itemPaths.empty())
        {
            if (BuildAtlas(vanillaItems, outItems, itemPaths, 16, 16, 16, s_itemEntries))
            {
                s_hasModTextures = true;
                for (auto& e : s_itemEntries) e.atlasType = 1;
                LogUtil::Log("[WeaveLoader] ModAtlas: built items.png with %zu mod textures", s_itemEntries.size());
            }
        }

        return s_hasModTextures ? s_mergedDir : "";
    }

    std::string GetMergedTerrainPath()
    {
        return s_mergedDir.empty() ? "" : s_mergedDir + "\\terrain.png";
    }

    std::string GetMergedItemsPath()
    {
        return s_mergedDir.empty() ? "" : s_mergedDir + "\\items.png";
    }

    const std::vector<ModTextureEntry>& GetBlockEntries() { return s_blockEntries; }
    const std::vector<ModTextureEntry>& GetItemEntries() { return s_itemEntries; }
    bool HasModTextures() { return s_hasModTextures; }

    static std::unordered_map<std::wstring, void*> s_modIcons;
    static RegisterIcon_fn s_originalRegisterIcon = nullptr;
    static std::string s_gameResPath;
    static std::string s_backupTerrainPath;
    static std::string s_backupItemsPath;

    // Per-atlas-type textureMap pointers, saved during CreateModIcons for FixupModIcons.
    static void* s_terrainTextureMap = nullptr;
    static void* s_itemsTextureMap = nullptr;

    void SetInjectSymbols(void* simpleIconCtor, void* operatorNew)
    {
        s_simpleIconCtor = simpleIconCtor;
        s_operatorNew = reinterpret_cast<void* (*)(size_t)>(operatorNew);
    }

    void SetRegisterIconFn(RegisterIcon_fn fn)
    {
        s_originalRegisterIcon = fn;
    }

    void InstallAtlasFiles(const std::string& gameResPath)
    {
        if (!s_hasModTextures) return;
        s_gameResPath = gameResPath;

        std::string vanillaTerrain = gameResPath + "\\terrain.png";
        std::string vanillaItems   = gameResPath + "\\items.png";
        std::string mergedTerrain  = GetMergedTerrainPath();
        std::string mergedItems    = GetMergedItemsPath();

        if (!mergedTerrain.empty() && !s_blockEntries.empty())
        {
            s_backupTerrainPath = vanillaTerrain + ".weaveloader_backup";
            CopyFileA(vanillaTerrain.c_str(), s_backupTerrainPath.c_str(), FALSE);
            if (CopyFileA(mergedTerrain.c_str(), vanillaTerrain.c_str(), FALSE))
                LogUtil::Log("[WeaveLoader] ModAtlas: installed merged terrain.png over game file");
            else
                LogUtil::Log("[WeaveLoader] ModAtlas: WARNING - failed to copy merged terrain.png (err=%lu)", GetLastError());
        }

        if (!mergedItems.empty() && !s_itemEntries.empty())
        {
            s_backupItemsPath = vanillaItems + ".weaveloader_backup";
            CopyFileA(vanillaItems.c_str(), s_backupItemsPath.c_str(), FALSE);
            if (CopyFileA(mergedItems.c_str(), vanillaItems.c_str(), FALSE))
                LogUtil::Log("[WeaveLoader] ModAtlas: installed merged items.png over game file");
            else
                LogUtil::Log("[WeaveLoader] ModAtlas: WARNING - failed to copy merged items.png (err=%lu)", GetLastError());
        }
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

                void* icon = s_operatorNew(128);
                if (icon)
                {
                    memset(icon, 0, 128);
                    ctor(icon, &e.iconName, &e.iconName, u0, v0, u1, v1);
                    s_modIcons[e.iconName] = icon;
                    LogUtil::Log("[WeaveLoader] ModAtlas: created icon '%ls' (atlas=%d, row=%d, col=%d)",
                                 e.iconName.c_str(), iconType, e.row, e.col);
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
            for (auto& kv : s_modIcons)
            {
                void* icon = kv.second;
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

        // Restore backed-up vanilla atlas files
        if (!s_backupTerrainPath.empty())
        {
            std::string vanillaTerrain = s_gameResPath + "\\terrain.png";
            if (MoveFileExA(s_backupTerrainPath.c_str(), vanillaTerrain.c_str(), MOVEFILE_REPLACE_EXISTING))
                LogUtil::Log("[WeaveLoader] ModAtlas: restored original terrain.png");
            s_backupTerrainPath.clear();
        }
        if (!s_backupItemsPath.empty())
        {
            std::string vanillaItems = s_gameResPath + "\\items.png";
            if (MoveFileExA(s_backupItemsPath.c_str(), vanillaItems.c_str(), MOVEFILE_REPLACE_EXISTING))
                LogUtil::Log("[WeaveLoader] ModAtlas: restored original items.png");
            s_backupItemsPath.clear();
        }
    }

    void* LookupModIcon(const std::wstring& name)
    {
        auto it = s_modIcons.find(name);
        if (it != s_modIcons.end())
            return it->second;
        return nullptr;
    }
}
