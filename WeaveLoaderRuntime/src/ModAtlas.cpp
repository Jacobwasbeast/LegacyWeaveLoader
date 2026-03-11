#include "ModAtlas.h"
#include "LogUtil.h"
#include <Windows.h>
#include <MinHook.h>
#include <algorithm>
#include <atomic>
#include <cwctype>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <thread>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

namespace ModAtlas
{
    static std::string s_mergedDir;
    static std::string s_virtualAtlasDir;
    static std::string s_modsPath;
    static std::string s_gameResPath;
    static std::string s_overrideTerrainPath;
    static std::string s_overrideItemsPath;
    static std::string s_lastTerrainBasePath;
    static std::string s_lastItemsBasePath;
    static int s_lastTerrainBaseWidth = 0;
    static int s_lastTerrainBaseHeight = 0;
    static int s_lastItemsBaseWidth = 0;
    static int s_lastItemsBaseHeight = 0;
    static void* s_lastTerrainImagePtr = nullptr;
    static void* s_lastItemsImagePtr = nullptr;
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
    static int s_terrainBaseRows = 32;
    static int s_terrainRows = 32;
    static float s_terrainVScale = 1.0f;
    static int s_itemRows = 16;
    static float s_itemVScale = 1.0f;
    static void* s_simpleIconCtor = nullptr;
    static void* (*s_operatorNew)(size_t) = nullptr;
    static std::unordered_map<std::wstring, void*> s_modIcons;
    static std::unordered_map<void*, int> s_iconAtlasType;
    struct IconRouteInfo { int atlasType; int page; };
    static std::unordered_map<void*, IconRouteInfo> s_iconRoutes;
    static RegisterIcon_fn s_originalRegisterIcon = nullptr;
    static thread_local bool s_hasPendingPage = false;
    static thread_local int s_pendingAtlasType = -1;
    static thread_local int s_pendingPage = 0;
    static std::atomic<bool> s_buildInProgress{false};
    static std::atomic<bool> s_asyncBuildQueued{false};
    static std::atomic<bool> s_asyncBuildComplete{false};
    static bool s_createFileHookInstalled = false;
    static bool s_applyMergedToBufferedImage = false;

    static constexpr int kTerrainGridCols = 16;
    // Rows used by vanilla UVs in PreStitchedTextureMap (rows 0-19 inclusive).
    // Keeping these reserved prevents transparent-but-used tiles (e.g., redstone)
    // from being treated as empty during mod atlas packing.
    static constexpr int kReservedTerrainRows = 20;

    static int GetMaxTerrainAtlasPixels()
    {
        static int s_maxPixels = -1;
        if (s_maxPixels >= 0)
            return s_maxPixels;

        s_maxPixels = 4096;
        const char* env = std::getenv("WEAVELOADER_TERRAIN_MAX_PX");
        if (env && *env)
        {
            int value = std::atoi(env);
            if (value >= 512)
                s_maxPixels = value;
        }
        return s_maxPixels;
    }

    static std::string BuildAtlasesInternal(const std::string& modsPath,
                                            const std::string& terrainBasePath,
                                            const std::string& itemsBasePath);
    static void RefreshMergedPaths();

    static bool IsReservedTerrainCell(int row, int col)
    {
        if (row < 0 || col < 0 || col >= kTerrainGridCols)
            return false;
        const int reserveRows = (s_terrainBaseRows < kReservedTerrainRows)
            ? s_terrainBaseRows
            : kReservedTerrainRows;
        return row < reserveRows;
    }

    static bool IsAsyncAtlasEnabled()
    {
        return true;
    }

    static void StartAsyncBuild(const std::string& terrainPath, const std::string& itemsPath)
    {
        if (s_asyncBuildQueued.load())
            return;
        s_asyncBuildQueued = true;
        s_buildInProgress = true;
        LogUtil::Log("[WeaveLoader] ModAtlas: starting async build");

        const std::string modsPath = s_modsPath;
        std::thread([modsPath, terrainPath, itemsPath]()
        {
            BuildAtlasesInternal(modsPath, terrainPath, itemsPath);
            s_lastTerrainBasePath = terrainPath;
            s_lastItemsBasePath = itemsPath;
            s_buildInProgress = false;
            s_asyncBuildComplete = true;
            LogUtil::Log("[WeaveLoader] ModAtlas: async build complete");
        }).detach();
    }

    // CreateFileW hook: redirect game file opens to merged atlases
    static std::wstring s_mergedTerrainW;
    static std::wstring s_mergedItemsW;
    static std::wstring s_vanillaTerrainW;
    static std::wstring s_vanillaItemsW;

    typedef HANDLE (WINAPI *CreateFileW_fn)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    static CreateFileW_fn s_originalCreateFileW = nullptr;

    // iconType is at offset 8 in PreStitchedTextureMap (verified via getIconType disassembly)

    static std::string ToLower(const std::string& s)
    {
        std::string r = s;
        for (char& c : r)
            c = (char)tolower((unsigned char)c);
        return r;
    }

    static bool HasPngExtension(const std::string& name)
    {
        if (name.size() < 4) return false;
        char a = (char)tolower((unsigned char)name[name.size() - 4]);
        char b = (char)tolower((unsigned char)name[name.size() - 3]);
        char c = (char)tolower((unsigned char)name[name.size() - 2]);
        char d = (char)tolower((unsigned char)name[name.size() - 1]);
        return a == '.' && b == 'p' && c == 'n' && d == 'g';
    }

    static void ScanPngTree(const std::string& dir,
                            const std::string& baseDir,
                            const std::string& iconPrefix,
                            std::vector<std::pair<std::string, std::string>>& out,
                            std::unordered_set<std::string>& seen)
    {
        WIN32_FIND_DATAA fd;
        std::string search = dir + "\\*";
        HANDLE h = FindFirstFileA(search.c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) return;
        do
        {
            if (fd.cFileName[0] == '.') continue;
            std::string fullPath = dir + "\\" + fd.cFileName;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                ScanPngTree(fullPath, baseDir, iconPrefix, out, seen);
                continue;
            }
            if (!HasPngExtension(fd.cFileName)) continue;

            if (fullPath.size() <= baseDir.size()) continue;
            std::string rel = fullPath.substr(baseDir.size());
            if (!rel.empty() && (rel[0] == '\\' || rel[0] == '/'))
                rel.erase(0, 1);
            for (char& ch : rel)
            {
                if (ch == '\\') ch = '/';
            }
            if (rel.size() < 4) continue;
            rel.resize(rel.size() - 4);
            rel = ToLower(rel);

            std::string iconName = iconPrefix + rel;
            if (seen.insert(iconName).second)
                out.push_back({ iconName, fullPath });
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    static void FindModTextures(const std::string& modsPath,
                                std::vector<std::pair<std::string, std::string>>& blocks,
                                std::vector<std::pair<std::string, std::string>>& items)
    {
        blocks.clear();
        items.clear();
        std::unordered_set<std::string> seenBlocks;
        std::unordered_set<std::string> seenItems;

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
                // Java-style assets: assets/<namespace>/textures/block|item/*.png
                WIN32_FIND_DATAA nsfd;
                std::string nsSearch = assetsPath + "\\*";
                HANDLE hNs = FindFirstFileA(nsSearch.c_str(), &nsfd);
                if (hNs != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        if (!(nsfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                        if (nsfd.cFileName[0] == '.') continue;

                        std::string nsFolder = nsfd.cFileName;
                        std::string nsName = ToLower(nsFolder);
                        std::string nsPath = assetsPath + "\\" + nsFolder;
                        std::string texturesPath = nsPath + "\\textures";
                        std::string blocksPath = texturesPath + "\\block";
                        std::string itemsPath = texturesPath + "\\item";

                        if (GetFileAttributesA(blocksPath.c_str()) == FILE_ATTRIBUTE_DIRECTORY)
                            ScanPngTree(blocksPath, blocksPath, nsName + ":block/", blocks, seenBlocks);
                        if (GetFileAttributesA(itemsPath.c_str()) == FILE_ATTRIBUTE_DIRECTORY)
                            ScanPngTree(itemsPath, itemsPath, nsName + ":item/", items, seenItems);
                    } while (FindNextFileA(hNs, &nsfd));
                    FindClose(hNs);
                }
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    static bool LoadPng(const std::string& path, int* w, int* h, int* comp, unsigned char** data)
    {
        *data = stbi_load(path.c_str(), w, h, comp, 4);
        return *data != nullptr;
    }

    static bool SavePngToBytes(const unsigned char* img, int w, int h,
                               std::vector<unsigned char>& outBytes)
    {
        outBytes.clear();
        if (!img || w <= 0 || h <= 0) return false;

        auto writeFunc = [](void* context, void* data, int size)
        {
            auto* bytes = reinterpret_cast<std::vector<unsigned char>*>(context);
            const unsigned char* p = reinterpret_cast<const unsigned char*>(data);
            bytes->insert(bytes->end(), p, p + size);
        };

        int ok = stbi_write_png_to_func(writeFunc, &outBytes, w, h, 4, img, w * 4);
        return ok != 0 && !outBytes.empty();
    }

    struct MipLevel
    {
        int w = 0;
        int h = 0;
        std::vector<unsigned char> rgba;
    };

    static void Downsample2x(const unsigned char* src, int srcW, int srcH,
                             std::vector<unsigned char>& out, int& outW, int& outH)
    {
        outW = (srcW > 1) ? (srcW / 2) : 1;
        outH = (srcH > 1) ? (srcH / 2) : 1;
        out.assign(static_cast<size_t>(outW) * static_cast<size_t>(outH) * 4, 0);

        for (int y = 0; y < outH; y++)
        {
            int sy = y * 2;
            for (int x = 0; x < outW; x++)
            {
                int sx = x * 2;
                int sx1 = (sx + 1 < srcW) ? (sx + 1) : (srcW - 1);
                int sy1 = (sy + 1 < srcH) ? (sy + 1) : (srcH - 1);

                const unsigned char* p00 = src + (sy * srcW + sx) * 4;
                const unsigned char* p10 = src + (sy * srcW + sx1) * 4;
                const unsigned char* p01 = src + (sy1 * srcW + sx) * 4;
                const unsigned char* p11 = src + (sy1 * srcW + sx1) * 4;

                unsigned int r = p00[0] + p10[0] + p01[0] + p11[0];
                unsigned int g = p00[1] + p10[1] + p01[1] + p11[1];
                unsigned int b = p00[2] + p10[2] + p01[2] + p11[2];
                unsigned int a = p00[3] + p10[3] + p01[3] + p11[3];

                unsigned char* dst = out.data() + (y * outW + x) * 4;
                dst[0] = static_cast<unsigned char>(r / 4);
                dst[1] = static_cast<unsigned char>(g / 4);
                dst[2] = static_cast<unsigned char>(b / 4);
                dst[3] = static_cast<unsigned char>(a / 4);
            }
        }
    }

    static void BuildMipChain(const unsigned char* base, int baseW, int baseH,
                              std::vector<MipLevel>& outLevels)
    {
        outLevels.clear();
        if (!base || baseW <= 0 || baseH <= 0)
            return;

        std::vector<unsigned char> current(base, base + (static_cast<size_t>(baseW) * baseH * 4));
        int curW = baseW;
        int curH = baseH;

        for (int level = 1; level < 10; level++)
        {
            if (curW == 1 && curH == 1)
                break;

            MipLevel mip;
            Downsample2x(current.data(), curW, curH, mip.rgba, mip.w, mip.h);
            outLevels.push_back(mip);
            current = mip.rgba;
            curW = mip.w;
            curH = mip.h;
        }
    }

    static void ApplyMipChainToBufferedImage(int** data, const std::vector<MipLevel>& levels)
    {
        if (!data)
            return;

        size_t count = (levels.size() > 9) ? 9 : levels.size();
        for (size_t i = 0; i < count; i++)
        {
            const MipLevel& mip = levels[i];
            if (mip.w <= 0 || mip.h <= 0 || mip.rgba.empty())
                continue;

            int pixelCount = mip.w * mip.h;
            int* buf = new int[pixelCount];
            for (int p = 0; p < pixelCount; p++)
            {
                const unsigned char* px = &mip.rgba[p * 4];
                buf[p] = (static_cast<int>(px[3]) << 24) |
                         (static_cast<int>(px[0]) << 16) |
                         (static_cast<int>(px[1]) << 8) |
                         (static_cast<int>(px[2]) << 0);
            }

            if (data[i + 1])
                delete[] data[i + 1];
            data[i + 1] = buf;
        }

        for (size_t i = count + 1; i < 10; i++)
        {
            if (data[i])
            {
                delete[] data[i];
                data[i] = nullptr;
            }
        }
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

    static std::string BuildMipOutputPath(const std::string& baseDir, const char* stem, int level)
    {
        if (level <= 1) return baseDir + "\\" + stem + ".png";
        return baseDir + "\\" + stem + "MipMapLevel" + std::to_string(level) + ".png";
    }

    static std::string BuildVirtualPageOutputPath(const std::string& baseDir, const char* stem, int page)
    {
        if (baseDir.empty()) return "";
        if (page <= 0) return baseDir + "\\" + stem + ".png";
        return baseDir + "\\" + stem + "_p" + std::to_string(page) + ".png";
    }

    static void BlitIcon(unsigned char* dst, int dstW, int dstH, int dstX, int dstY, int iconSize,
                         const unsigned char* src, int srcW, int srcH)
    {
        if (iconSize <= 0) return;
        for (int y = 0; y < iconSize; y++)
        {
            for (int x = 0; x < iconSize; x++)
            {
                int sx = (srcW > 0) ? (x * srcW / iconSize) : 0;
                int sy = (srcH > 0) ? (y * srcH / iconSize) : 0;
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

    static int ComputeIconSize(int imgW, int imgH, int gridCols, int baseRows)
    {
        const int sizeW = (gridCols > 0) ? (imgW / gridCols) : 0;
        const int sizeH = (baseRows > 0) ? (imgH / baseRows) : 0;
        if (sizeW <= 0 && sizeH <= 0) return 0;
        if (sizeW <= 0) return sizeH;
        if (sizeH <= 0) return sizeW;
        return (sizeW < sizeH) ? sizeW : sizeH;
    }

    static size_t BuildAtlasPageExpanded(const std::string& vanillaPath, const std::string& outPath,
                           const std::vector<std::pair<std::string, std::string>>& modTextures, size_t startIndex,
                           int atlasType, int page,
                           int gridCols, int baseRows,
                           int& outIconSize, int& outTotalRows,
                           std::vector<ModTextureEntry>& entries)
    {
        outIconSize = 0;
        outTotalRows = baseRows;

        int vw = 0, vh = 0, vc = 0;
        unsigned char* vanilla = nullptr;
        if (!vanillaPath.empty())
            vanilla = stbi_load(vanillaPath.c_str(), &vw, &vh, &vc, 4);

        if (!vanilla)
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: ERROR could not load atlas base '%s' (atlasType=%d page=%d)",
                         vanillaPath.c_str(), atlasType, page);
            return 0;
        }

        int iconSize = ComputeIconSize(vw, vh, gridCols, baseRows);
        if (iconSize <= 0)
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: ERROR invalid atlas size %dx%d for baseRows=%d cols=%d",
                         vw, vh, baseRows, gridCols);
            stbi_image_free(vanilla);
            return 0;
        }

        const int baseW = gridCols * iconSize;
        const int baseH = baseRows * iconSize;
        if (vw != baseW || vh != baseH)
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: atlas base size %dx%d does not match expected %dx%d (icon=%d). Using cropped base.",
                         vw, vh, baseW, baseH, iconSize);
        }

        std::vector<unsigned char> baseImg((size_t)baseW * (size_t)baseH * 4, 0);
        const int copyW = (vw < baseW) ? vw : baseW;
        const int copyH = (vh < baseH) ? vh : baseH;
        for (int y = 0; y < copyH; y++)
        {
            memcpy(baseImg.data() + (size_t)y * baseW * 4,
                   vanilla + (size_t)y * vw * 4,
                   (size_t)copyW * 4);
        }
        stbi_image_free(vanilla);

        // Find all empty (fully transparent) cells in the base atlas
        std::vector<std::pair<int, int>> emptyCells;
        for (int row = 0; row < baseRows; row++)
        {
            for (int col = 0; col < gridCols; col++)
            {
                if (atlasType == 0 && IsReservedTerrainCell(row, col))
                    continue;
                if (IsCellEmpty(baseImg.data(), baseW, baseH, col * iconSize, row * iconSize, iconSize))
                    emptyCells.push_back({ row, col });
            }
        }

        const size_t totalMods = (startIndex < modTextures.size()) ? (modTextures.size() - startIndex) : 0;
        const size_t baseCapacity = emptyCells.size();
        const size_t extraNeeded = (totalMods > baseCapacity) ? (totalMods - baseCapacity) : 0;
        int extraRows = (extraNeeded > 0)
            ? (int)((extraNeeded + (size_t)gridCols - 1) / (size_t)gridCols)
            : 0;
        int totalRows = baseRows + extraRows;

        if (atlasType == 0)
        {
            const int maxPixels = GetMaxTerrainAtlasPixels();
            int maxRows = (iconSize > 0) ? (maxPixels / iconSize) : 0;
            if (maxRows < baseRows)
                maxRows = baseRows;
            if (totalRows > maxRows)
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: terrain rows clamped %d -> %d (maxPx=%d icon=%d)",
                             totalRows, maxRows, maxPixels, iconSize);
                totalRows = maxRows;
            }
        }

        const int imgW = baseW;
        const int imgH = totalRows * iconSize;
        unsigned char* img = (unsigned char*)calloc((size_t)imgW * (size_t)imgH, 4);
        if (!img)
            return 0;

        // Copy base atlas into the top portion of the expanded atlas
        for (int y = 0; y < baseH; y++)
            memcpy(img + (size_t)y * imgW * 4, baseImg.data() + (size_t)y * baseW * 4, (size_t)baseW * 4);

        LogUtil::Log("[WeaveLoader] ModAtlas: base %dx%d (rows=%d icon=%d) emptyCells=%zu extraRows=%d totalRows=%d",
                     baseW, baseH, baseRows, iconSize, emptyCells.size(), extraRows, totalRows);

        size_t consumed = 0;
        const int maxExtraCells = (totalRows > baseRows) ? ((totalRows - baseRows) * gridCols) : 0;
        size_t cellIdx = 0;
        size_t extraIdx = 0;
        for (size_t texIdx = startIndex; texIdx < modTextures.size(); ++texIdx)
        {
            const auto& tex = modTextures[texIdx];
            const std::string& iconName = tex.first;
            const std::string& path = tex.second;

            int row = 0;
            int col = 0;
            if (cellIdx < emptyCells.size())
            {
                row = emptyCells[cellIdx].first;
                col = emptyCells[cellIdx].second;
                cellIdx++;
            }
            else
            {
                if ((int)extraIdx >= maxExtraCells)
                {
                    LogUtil::Log("[WeaveLoader] ModAtlas: atlas capacity reached (page=%d, rows=%d); skipping remaining textures",
                                 page, totalRows);
                    break;
                }
                row = baseRows + (int)(extraIdx / (size_t)gridCols);
                col = (int)(extraIdx % (size_t)gridCols);
                extraIdx++;
            }

            int sw = 0, sh = 0, sc = 0;
            unsigned char* src = nullptr;
            if (!LoadPng(path, &sw, &sh, &sc, &src))
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: failed to load %s", path.c_str());
                continue;
            }

            BlitIcon(img, imgW, imgH, col * iconSize, row * iconSize, iconSize, src, sw, sh);
            stbi_image_free(src);

            std::wstring wname(iconName.begin(), iconName.end());
            entries.push_back({ wname, atlasType, page, row, col });
            consumed++;

            if (consumed <= 20)
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: placed '%s' at page=%d row=%d col=%d",
                             iconName.c_str(), page, row, col);
            }
        }

        std::string dir = outPath.substr(0, outPath.find_last_of("\\/"));
        CreateDirectoryA(dir.c_str(), nullptr);

        int ok = stbi_write_png(outPath.c_str(), imgW, imgH, 4, img, imgW * 4);
        free(img);
        if (!ok) return 0;

        outIconSize = iconSize;
        outTotalRows = totalRows;
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

            BlitIcon(img, imgW, imgH, col * iconSize, row * iconSize, iconSize, src, sw, sh);
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

    static void WriteMipChainToDisk(const std::string& baseDir, const char* stem,
                                    const std::vector<MipLevel>& levels)
    {
        for (size_t i = 0; i < levels.size() && i < 9; i++)
        {
            const MipLevel& mip = levels[i];
            if (mip.w <= 0 || mip.h <= 0 || mip.rgba.empty())
                continue;

            std::vector<unsigned char> pngBytes;
            if (!SavePngToBytes(mip.rgba.data(), mip.w, mip.h, pngBytes))
                continue;

            int mipLevel = static_cast<int>(i) + 2;
            std::string outPath = BuildMipOutputPath(baseDir, stem, mipLevel);
            std::ofstream out(outPath, std::ios::binary);
            if (out.is_open())
                out.write(reinterpret_cast<const char*>(pngBytes.data()),
                          static_cast<std::streamsize>(pngBytes.size()));
        }
    }

    static void GenerateMipmapsForAtlas(const std::string& basePath,
                                        const std::string& outDir,
                                        const char* stem)
    {
        if (basePath.empty() || outDir.empty())
            return;

        int w = 0, h = 0, c = 0;
        unsigned char* base = stbi_load(basePath.c_str(), &w, &h, &c, 4);
        if (!base)
            return;

        std::vector<MipLevel> levels;
        BuildMipChain(base, w, h, levels);
        stbi_image_free(base);

        WriteMipChainToDisk(outDir, stem, levels);
    }

    static std::string BuildAtlasesInternal(const std::string& modsPath,
                                            const std::string& terrainBasePath,
                                            const std::string& itemsBasePath)
    {
        s_buildInProgress = true;
        s_blockEntries.clear();
        s_itemEntries.clear();
        s_modIcons.clear();
        s_iconRoutes.clear();
        s_iconAtlasType.clear();
        s_hasModTextures = false;
        s_hasTerrainAtlas = false;
        s_hasItemsAtlas = false;
        s_hasTerrainPage0Mods = false;
        s_hasItemsPage0Mods = false;
        s_terrainPages = 0;
        s_itemPages = 0;
        s_mergedTerrainPage1Path.clear();
        s_mergedItemsPage1Path.clear();
        s_terrainRows = s_terrainBaseRows;
        s_terrainVScale = 1.0f;
        s_itemRows = 16;
        s_itemVScale = 1.0f;

        std::vector<std::pair<std::string, std::string>> blockPaths, itemPaths;
        FindModTextures(modsPath, blockPaths, itemPaths);

        if (blockPaths.empty() && itemPaths.empty())
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: no mod textures found");
            s_buildInProgress = false;
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

        if (!blockPaths.empty())
        {
            const std::string outPath = BuildPageOutputPath(s_mergedDir, "terrain", 0);
            int iconSize = 0;
            int totalRows = s_terrainBaseRows;
            size_t placed = BuildAtlasPageExpanded(terrainBasePath, outPath, blockPaths, 0, 0, 0, 16, s_terrainBaseRows,
                                                   iconSize, totalRows, s_blockEntries);
            if (placed > 0)
            {
                s_hasModTextures = true;
                s_hasTerrainAtlas = true;
                s_hasTerrainPage0Mods = true;
                s_terrainPages = 1; // only page 0
                s_terrainRows = (totalRows > 0) ? totalRows : s_terrainBaseRows;
                if (s_terrainRows > 0)
                    s_terrainVScale = (float)s_terrainBaseRows / (float)s_terrainRows;
                LogUtil::Log("[WeaveLoader] ModAtlas: built terrain atlas rows=%d (scaleV=%.4f)",
                             s_terrainRows, s_terrainVScale);

                if (!s_virtualAtlasDir.empty())
                {
                    std::string vpath = BuildVirtualPageOutputPath(s_virtualAtlasDir, "terrain", 0);
                    if (!vpath.empty())
                        CopyFileA(outPath.c_str(), vpath.c_str(), FALSE);
                }

                GenerateMipmapsForAtlas(outPath, s_mergedDir, "terrain");
            }

            if (placed < blockPaths.size())
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: WARNING terrain overflow, dropped %zu textures",
                             blockPaths.size() - placed);
            }
        }

        if (!itemPaths.empty())
        {
            int itemIconSize = 16;
            if (!itemsBasePath.empty())
            {
                int iw = 0, ih = 0, ic = 0;
                unsigned char* baseItems = stbi_load(itemsBasePath.c_str(), &iw, &ih, &ic, 4);
                if (baseItems)
                {
                    int computed = ComputeIconSize(iw, ih, 16, 16);
                    if (computed > 0)
                        itemIconSize = computed;
                    stbi_image_free(baseItems);
                }
            }

            size_t cursor = 0;
            int page = 1; // page 0 remains vanilla; modded items use dedicated pages
            while (cursor < itemPaths.size())
            {
                std::string outPath = BuildPageOutputPath(s_mergedDir, "items", page);
                size_t placed = BuildModOnlyAtlasPage(outPath, itemPaths, cursor, 1, page, 16, 16, itemIconSize, s_itemEntries);
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

        s_buildInProgress = false;
        return s_hasModTextures ? s_mergedDir : "";
    }

    void SetBasePaths(const std::string& modsPath, const std::string& gameResPath)
    {
        s_modsPath = modsPath;
        s_gameResPath = gameResPath;
        s_asyncBuildQueued = false;
        s_asyncBuildComplete = false;
        s_buildInProgress = false;
    }

    void SetOverrideAtlasPath(int atlasType, const std::string& path)
    {
        if (atlasType == 0) s_overrideTerrainPath = path;
        else if (atlasType == 1) s_overrideItemsPath = path;
        s_asyncBuildQueued = false;
        s_asyncBuildComplete = false;
    }

    static void UpdateLastBaseDims(int atlasType, int w, int h)
    {
        if (atlasType == 0)
        {
            s_lastTerrainBaseWidth = w;
            s_lastTerrainBaseHeight = h;
        }
        else if (atlasType == 1)
        {
            s_lastItemsBaseWidth = w;
            s_lastItemsBaseHeight = h;
        }
    }

    static bool BaseDimsUnchanged(int atlasType, int w, int h)
    {
        if (atlasType == 0)
            return (w == s_lastTerrainBaseWidth) && (h == s_lastTerrainBaseHeight);
        if (atlasType == 1)
            return (w == s_lastItemsBaseWidth) && (h == s_lastItemsBaseHeight);
        return false;
    }

    static bool BuildTerrainFromBaseRgba(const unsigned char* baseRgba, int baseW, int baseH,
                                         std::vector<unsigned char>& outRgba,
                                         int& outW, int& outH)
    {
        outRgba.clear();
        outW = 0;
        outH = 0;
        if (!baseRgba || baseW <= 0 || baseH <= 0)
            return false;
        if (s_blockEntries.empty())
            return false;

        int iconSize = ComputeIconSize(baseW, baseH, 16, s_terrainBaseRows);
        if (iconSize <= 0)
            return false;

        outW = iconSize * 16;
        outH = iconSize * ((s_terrainRows > 0) ? s_terrainRows : s_terrainBaseRows);
        if (outW <= 0 || outH <= 0)
            return false;

        outRgba.assign(static_cast<size_t>(outW) * static_cast<size_t>(outH) * 4, 0);

        const int copyW = (baseW < outW) ? baseW : outW;
        const int copyH = (baseH < outH) ? baseH : outH;
        for (int y = 0; y < copyH; y++)
        {
            const int srcRow = y * baseW * 4;
            const int dstRow = y * outW * 4;
            memcpy(outRgba.data() + dstRow, baseRgba + srcRow, static_cast<size_t>(copyW) * 4);
        }

        std::vector<std::pair<std::string, std::string>> blockPaths, itemPaths;
        FindModTextures(s_modsPath, blockPaths, itemPaths);
        std::unordered_map<std::string, std::string> pathMap;
        pathMap.reserve(blockPaths.size());
        for (const auto& p : blockPaths)
            pathMap[p.first] = p.second;

        for (const auto& e : s_blockEntries)
        {
            auto it = pathMap.find(std::string(e.iconName.begin(), e.iconName.end()));
            if (it == pathMap.end())
                continue;
            int iw = 0, ih = 0, ic = 0;
            unsigned char* src = stbi_load(it->second.c_str(), &iw, &ih, &ic, 4);
            if (!src)
                continue;
            const int dstX = e.col * iconSize;
            const int dstY = e.row * iconSize;
            BlitIcon(outRgba.data(), outW, outH, dstX, dstY, iconSize, src, iw, ih);
            stbi_image_free(src);
        }

        return true;
    }

    bool OverrideAtlasFromBufferedImage(int atlasType, void* bufferedImage)
    {
        if (!bufferedImage)
            return false;
        if (s_modsPath.empty() || s_gameResPath.empty())
            return false;

        // BufferedImage layout: int* data[10] at offset 0, width at +0x50, height at +0x54.
        // We treat data[0] as ARGB 32-bit pixels (as used by Texture::transferFromImage).
        char* base = static_cast<char*>(bufferedImage);
        int** data = reinterpret_cast<int**>(base);
        int* pixels = data[0];
        int w = *reinterpret_cast<int*>(base + 0x50);
        int h = *reinterpret_cast<int*>(base + 0x54);
        if (!pixels || w <= 0 || h <= 0)
            return false;

        if (atlasType == 0)
        {
            if (bufferedImage == s_lastTerrainImagePtr)
                return false;
            s_lastTerrainImagePtr = bufferedImage;
        }
        else if (atlasType == 1)
        {
            if (bufferedImage == s_lastItemsImagePtr)
                return false;
            s_lastItemsImagePtr = bufferedImage;
        }

        std::vector<unsigned char> rgba(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
        for (int i = 0; i < w * h; i++)
        {
            unsigned int argb = static_cast<unsigned int>(pixels[i]);
            rgba[i * 4 + 0] = (argb >> 16) & 0xFF; // R
            rgba[i * 4 + 1] = (argb >> 8) & 0xFF;  // G
            rgba[i * 4 + 2] = (argb >> 0) & 0xFF;  // B
            rgba[i * 4 + 3] = (argb >> 24) & 0xFF; // A
        }

        std::vector<unsigned char> mergedRgba;
        int outW = w;
        int outH = h;
        bool ok = false;
        if (atlasType == 0 && s_hasModTextures && !s_blockEntries.empty())
        {
            ok = BuildTerrainFromBaseRgba(rgba.data(), w, h, mergedRgba, outW, outH);
        }
        else if (atlasType == 0)
        {
            std::vector<unsigned char> pngBytes;
            if (!SavePngToBytes(rgba.data(), w, h, pngBytes))
                return false;

            const std::string tempDir = s_gameResPath + "\\modloader\\temp";
            CreateDirectoryA(tempDir.c_str(), nullptr);
            const std::string tempPath = tempDir + "\\terrain.png";
            {
                std::ofstream out(tempPath, std::ios::binary);
                if (!out.is_open())
                    return false;
                out.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
            }

            s_applyMergedToBufferedImage = true;
            SetOverrideAtlasPath(0, tempPath);
            ok = EnsureAtlasesBuilt();
            s_applyMergedToBufferedImage = false;
            UpdateLastBaseDims(0, w, h);

            if (!ok)
                return false;

            const std::string mergedPath = GetMergedTerrainPath();
            if (mergedPath.empty())
                return false;

            int mw = 0, mh = 0, mc = 0;
            unsigned char* merged = stbi_load(mergedPath.c_str(), &mw, &mh, &mc, 4);
            if (!merged)
                return false;
            outW = (mw > 0) ? mw : w;
            outH = (mh > 0) ? mh : h;
            mergedRgba.assign(merged, merged + (mw * mh * 4));
            stbi_image_free(merged);
        }
        else
        {
            // Do not override item atlases here (keep mod item paging stable).
            return false;
        }

        if (!ok || mergedRgba.empty())
            return false;

        if (atlasType == 0 && !s_mergedDir.empty())
        {
            std::vector<unsigned char> pngBytes;
            if (SavePngToBytes(mergedRgba.data(), outW, outH, pngBytes))
            {
                const std::string outPath = BuildPageOutputPath(s_mergedDir, "terrain", 0);
                std::ofstream out(outPath, std::ios::binary);
                if (out.is_open())
                    out.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
            }
        }

        const bool needsResize = (outW != w) || (outH != h);

        int* target = pixels;
        if (needsResize)
        {
            target = new int[outW * outH];
            std::fill(target, target + (outW * outH), 0);
        }

        const int copyW = outW;
        const int copyH = outH;
        for (int y = 0; y < copyH; y++)
        {
            for (int x = 0; x < copyW; x++)
            {
                const int si = (y * outW + x) * 4;
                const int di = y * outW + x;
                unsigned int argb = (static_cast<unsigned int>(mergedRgba[si + 3]) << 24) |
                                    (static_cast<unsigned int>(mergedRgba[si + 0]) << 16) |
                                    (static_cast<unsigned int>(mergedRgba[si + 1]) << 8) |
                                    (static_cast<unsigned int>(mergedRgba[si + 2]) << 0);
                target[di] = static_cast<int>(argb);
            }
        }

        if (needsResize)
        {
            delete[] data[0];
            for (int i = 1; i < 10; i++)
            {
                if (data[i])
                {
                    delete[] data[i];
                    data[i] = nullptr;
                }
            }
            data[0] = target;
            *reinterpret_cast<int*>(base + 0x50) = outW;
            *reinterpret_cast<int*>(base + 0x54) = outH;
        }

        if (atlasType == 0)
        {
            std::vector<MipLevel> levels;
            BuildMipChain(mergedRgba.data(), outW, outH, levels);
            ApplyMipChainToBufferedImage(data, levels);
        }
        return true;
    }

    bool EnsureAtlasesBuilt()
    {
        if (s_buildInProgress.load())
            return s_hasModTextures;
        if (s_modsPath.empty() || s_gameResPath.empty())
            return false;

        if (IsAsyncAtlasEnabled() && !s_applyMergedToBufferedImage)
        {
            if (s_asyncBuildComplete.load())
                return s_hasModTextures;

            std::string terrainPath = !s_overrideTerrainPath.empty()
                ? s_overrideTerrainPath
                : (s_gameResPath + "\\terrain.png");
            std::string itemsPath = !s_overrideItemsPath.empty()
                ? s_overrideItemsPath
                : (s_gameResPath + "\\items.png");

            StartAsyncBuild(terrainPath, itemsPath);
            return s_hasModTextures;
        }

        s_buildInProgress = true;

        // Pre-scan for mods so we can avoid nuking a valid atlas when a pack
        // path can't be loaded (e.g., DLC packs not backed by filesystem files).
        std::vector<std::pair<std::string, std::string>> blockPaths, itemPaths;
        FindModTextures(s_modsPath, blockPaths, itemPaths);

        std::string terrainPath = !s_overrideTerrainPath.empty()
            ? s_overrideTerrainPath
            : (s_gameResPath + "\\terrain.png");
        std::string itemsPath = !s_overrideItemsPath.empty()
            ? s_overrideItemsPath
            : (s_gameResPath + "\\items.png");

        if (!s_applyMergedToBufferedImage &&
            terrainPath == s_lastTerrainBasePath &&
            itemsPath == s_lastItemsBasePath &&
            !s_mergedDir.empty())
        {
            s_buildInProgress = false;
            return s_hasModTextures;
        }

        // Fallback to default if override path doesn't exist
        if (!s_overrideTerrainPath.empty() && !FileExists(terrainPath))
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: override terrain path missing, falling back to default");
            terrainPath = s_gameResPath + "\\terrain.png";
        }
        if (!s_overrideItemsPath.empty() && !FileExists(itemsPath))
        {
            LogUtil::Log("[WeaveLoader] ModAtlas: override items path missing, falling back to default");
            itemsPath = s_gameResPath + "\\items.png";
        }

        if (!blockPaths.empty())
        {
            int tw = 0, th = 0, tc = 0;
            unsigned char* probe = stbi_load(terrainPath.c_str(), &tw, &th, &tc, 4);
            if (!probe)
            {
                LogUtil::Log("[WeaveLoader] ModAtlas: cannot load terrain base '%s' (keeping existing atlas)",
                             terrainPath.c_str());
                s_buildInProgress = false;
                return s_hasModTextures;
            }
            stbi_image_free(probe);
            UpdateLastBaseDims(0, tw, th);
        }
        if (!itemPaths.empty())
        {
            int iw = 0, ih = 0, ic = 0;
            unsigned char* probe = stbi_load(itemsPath.c_str(), &iw, &ih, &ic, 4);
            if (probe)
            {
                stbi_image_free(probe);
                UpdateLastBaseDims(1, iw, ih);
            }
        }

        BuildAtlasesInternal(s_modsPath, terrainPath, itemsPath);
        s_lastTerrainBasePath = terrainPath;
        s_lastItemsBasePath = itemsPath;

        // If the CreateFileW hook is installed, refresh redirect targets after rebuild.
        if (s_originalCreateFileW)
        {
            std::string mergedTerrain = GetMergedTerrainPath();
            std::string mergedItems = GetMergedItemsPath();
            if (!mergedTerrain.empty())
                s_mergedTerrainW = std::wstring(mergedTerrain.begin(), mergedTerrain.end());
            if (!mergedItems.empty())
                s_mergedItemsW = std::wstring(mergedItems.begin(), mergedItems.end());
        }
        return s_hasModTextures;
    }

    std::string BuildAtlases(const std::string& modsPath, const std::string& gameResPath)
    {
        SetBasePaths(modsPath, gameResPath);
        s_overrideTerrainPath.clear();
        s_overrideItemsPath.clear();
        s_lastTerrainBasePath.clear();
        s_lastItemsBasePath.clear();
        s_lastTerrainBaseWidth = 0;
        s_lastTerrainBaseHeight = 0;
        s_lastItemsBaseWidth = 0;
        s_lastItemsBaseHeight = 0;
        return EnsureAtlasesBuilt() ? s_mergedDir : "";
    }

    void SetVirtualAtlasDirectory(const std::string& dir)
    {
        s_virtualAtlasDir = dir;
        if (!s_virtualAtlasDir.empty())
            CreateDirectoryA(s_virtualAtlasDir.c_str(), nullptr);
    }

    void PollAsyncBuild()
    {
        if (!IsAsyncAtlasEnabled())
            return;
        if (!s_asyncBuildComplete.exchange(false))
            return;

        RefreshMergedPaths();
        InstallCreateFileHook(s_gameResPath);
        LogUtil::Log("[WeaveLoader] ModAtlas: async build applied");
    }

    std::string GetMergedTerrainPath()
    {
        if (s_buildInProgress.load())
            return "";
        return s_hasTerrainPage0Mods ? GetMergedPagePath(0, 0) : "";
    }

    std::string GetMergedItemsPath()
    {
        if (s_buildInProgress.load())
            return "";
        // Never replace vanilla items page 0 unless explicitly needed.
        return s_hasItemsPage0Mods ? GetMergedPagePath(1, 0) : "";
    }

    std::string GetMergedPagePath(int atlasType, int page)
    {
        if (s_buildInProgress.load())
            return "";
        if (s_mergedDir.empty() || page < 0) return "";
        if (atlasType == 0)
        {
            if (!s_hasTerrainAtlas || page >= s_terrainPages) return "";
            return BuildPageOutputPath(s_mergedDir, "terrain", page);
        }
        if (!s_hasItemsAtlas || page >= s_itemPages) return "";
        return BuildPageOutputPath(s_mergedDir, "items", page);
    }

    std::string GetMergedMipmapPath(int atlasType, int mipLevel)
    {
        if (s_buildInProgress.load())
            return "";
        if (s_mergedDir.empty() || mipLevel <= 1)
            return "";
        const char* stem = (atlasType == 0) ? "terrain" : "items";
        std::string path = BuildMipOutputPath(s_mergedDir, stem, mipLevel);
        if (!FileExists(path))
            return "";
        return path;
    }

    std::string GetVirtualPagePath(int atlasType, int page)
    {
        if (s_buildInProgress.load())
            return "";
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
    bool HasModTextures() { return s_buildInProgress.load() ? false : s_hasModTextures; }

    void NoteIconAtlasType(void* iconPtr, int atlasType)
    {
        if (!iconPtr) return;
        s_iconAtlasType[iconPtr] = atlasType;
    }

    bool GetIconAtlasType(void* iconPtr, int& outAtlasType)
    {
        auto it = s_iconAtlasType.find(iconPtr);
        if (it == s_iconAtlasType.end())
            return false;
        outAtlasType = it->second;
        return true;
    }

    bool GetAtlasScale(int atlasType, float& outUScale, float& outVScale)
    {
        if (atlasType == 0)
        {
            outUScale = 1.0f;
            outVScale = s_terrainVScale;
            return true;
        }
        if (atlasType == 1)
        {
            outUScale = 1.0f;
            outVScale = s_itemVScale;
            return true;
        }
        return false;
    }

    // Per-atlas-type textureMap pointers, saved during CreateModIcons for FixupModIcons.
    static void* s_terrainTextureMap = nullptr;
    static void* s_itemsTextureMap = nullptr;

    static bool EndsWith(const wchar_t* path, const wchar_t* suffix)
    {
        size_t pathLen = wcslen(path);
        size_t suffLen = wcslen(suffix);
        if (suffLen > pathLen) return false;
        return _wcsicmp(path + pathLen - suffLen, suffix) == 0;
    }

    static bool IsGeneratedAtlasPath(const wchar_t* path)
    {
        if (!path) return false;
        std::wstring lower;
        lower.reserve(wcslen(path));
        for (const wchar_t* p = path; *p; ++p)
        {
            wchar_t c = (*p == L'\\') ? L'/' : *p;
            lower.push_back((wchar_t)towlower(c));
        }
        return (lower.find(L"/mods/modloader/generated/") != std::wstring::npos) ||
               (lower.find(L"/modloader/") != std::wstring::npos);
    }

    static bool TryExtractMipmapLevel(const wchar_t* path, const wchar_t* stem, int& outLevel)
    {
        outLevel = 0;
        if (!path || !stem) return false;

        std::wstring lower;
        lower.reserve(wcslen(path));
        for (const wchar_t* p = path; *p; ++p)
        {
            wchar_t c = (*p == L'\\') ? L'/' : *p;
            lower.push_back((wchar_t)towlower(c));
        }

        std::wstring key = std::wstring(stem) + L"mipmaplevel";
        size_t pos = lower.rfind(key);
        if (pos == std::wstring::npos)
            return false;

        size_t numStart = pos + key.size();
        size_t numEnd = lower.find(L".png", numStart);
        if (numEnd == std::wstring::npos || numEnd <= numStart)
            return false;

        int value = 0;
        for (size_t i = numStart; i < numEnd; i++)
        {
            wchar_t ch = lower[i];
            if (ch < L'0' || ch > L'9')
                return false;
            value = value * 10 + (ch - L'0');
        }

        if (value <= 1)
            return false;
        outLevel = value;
        return true;
    }

    static void RefreshMergedPaths()
    {
        const std::string mergedTerrain = GetMergedTerrainPath();
        const std::string mergedItems = GetMergedItemsPath();
        if (!mergedTerrain.empty())
            s_mergedTerrainW = std::wstring(mergedTerrain.begin(), mergedTerrain.end());
        else
            s_mergedTerrainW.clear();
        if (!mergedItems.empty())
            s_mergedItemsW = std::wstring(mergedItems.begin(), mergedItems.end());
        else
            s_mergedItemsW.clear();
    }

    static HANDLE WINAPI Hooked_CreateFileW(
        LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
    {
        if (s_buildInProgress.load())
        {
            return s_originalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
                lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }
        if (lpFileName && s_hasModTextures && !IsGeneratedAtlasPath(lpFileName))
        {
            int mipLevel = 0;
            if (TryExtractMipmapLevel(lpFileName, L"terrain", mipLevel))
            {
                std::string mipPath = GetMergedMipmapPath(0, mipLevel);
                if (!mipPath.empty())
                {
                    std::wstring wmip(mipPath.begin(), mipPath.end());
                    return s_originalCreateFileW(wmip.c_str(), dwDesiredAccess, dwShareMode,
                        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
                }
            }
            if (TryExtractMipmapLevel(lpFileName, L"items", mipLevel))
            {
                std::string mipPath = GetMergedMipmapPath(1, mipLevel);
                if (!mipPath.empty())
                {
                    std::wstring wmip(mipPath.begin(), mipPath.end());
                    return s_originalCreateFileW(wmip.c_str(), dwDesiredAccess, dwShareMode,
                        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
                }
            }
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
        if (s_createFileHookInstalled)
            return true;
        if (!s_hasModTextures && !s_buildInProgress.load()) return false;

        std::string mergedTerrain = GetMergedTerrainPath();
        std::string mergedItems = GetMergedItemsPath();
        if (mergedTerrain.empty() && mergedItems.empty() && !s_buildInProgress.load())
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

        s_createFileHookInstalled = true;
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
        s_createFileHookInstalled = false;
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
        if (s_buildInProgress.load())
            return;
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
                    NoteIconAtlasType(icon, iconType);
                    LogUtil::Log("[WeaveLoader] ModAtlas: created icon '%ls' (atlas=%d, page=%d, row=%d, col=%d)",
                                 e.iconName.c_str(), iconType, e.page, e.row, e.col);
                }
            }
        };

        if (iconType == 0)
            create(s_blockEntries, 1.0f / (float)((s_terrainRows > 0) ? s_terrainRows : 32));
        else if (iconType == 1)
            create(s_itemEntries, 1.0f / (float)((s_itemRows > 0) ? s_itemRows : 16));

        LogUtil::Log("[WeaveLoader] ModAtlas: s_modIcons now has %zu entries total", s_modIcons.size());
    }

    void FixupModIcons()
    {
        if (s_buildInProgress.load())
            return;
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
        if (s_buildInProgress.load())
            return nullptr;
        auto it = s_modIcons.find(name);
        if (it != s_modIcons.end())
            return it->second;
        return nullptr;
    }

    bool TryGetIconRoute(void* iconPtr, int& outAtlasType, int& outPage)
    {
        if (s_buildInProgress.load())
            return false;
        auto it = s_iconRoutes.find(iconPtr);
        if (it == s_iconRoutes.end())
            return false;
        outAtlasType = it->second.atlasType;
        outPage = it->second.page;
        return true;
    }

    void NotifyIconSampled(void* iconPtr)
    {
        if (s_buildInProgress.load())
        {
            s_hasPendingPage = false;
            s_pendingAtlasType = -1;
            s_pendingPage = 0;
            return;
        }
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
