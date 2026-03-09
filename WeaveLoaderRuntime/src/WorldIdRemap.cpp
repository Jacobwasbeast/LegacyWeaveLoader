#include "WorldIdRemap.h"

#include "GameObjectFactory.h"
#include "IdRegistry.h"
#include "LogUtil.h"
#include "ModStrings.h"
#include "PdbParser.h"

#include <Windows.h>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <new>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr wchar_t kNamespaceTagKey[] = L"weaveloader:id";
    constexpr wchar_t kNamespaceKindTagKey[] = L"weaveloader:kind";
    constexpr char kMissingBlockId[] = "weaveloader:missing_block";
    constexpr char kMissingItemId[] = "weaveloader:missing_item";
    constexpr int kMissingBlockVanillaFallbackId = 7; // bedrock
    constexpr int kMissingBlockDescriptionId = 900000;
    constexpr int kMissingItemDescriptionId = 900001;
    constexpr ptrdiff_t kItemIdOffset = 0x20;
    constexpr ptrdiff_t kItemTagOffset = 0x28;
    constexpr unsigned char kTagStringId = 8;
    constexpr unsigned char kTagCompoundId = 10;
    constexpr int kChunkWidth = 16;
    constexpr int kChunkMaxY = 256;

    using TagNewTag_fn = void* (__fastcall *)(unsigned char type, const std::wstring& name);
    using LevelChunkGetTile_fn = int (__fastcall *)(void* thisPtr, int x, int y, int z);
    using LevelChunkSetTile_fn = bool (__fastcall *)(void* thisPtr, int x, int y, int z, int tile);
    using LevelChunkGetPos_fn = void* (__fastcall *)(void* thisPtr);
    using LevelChunkGetHighestNonEmptyY_fn = int (__fastcall *)(void* thisPtr);
    using CompressedTileStorageSet_fn = void (__fastcall *)(void* thisPtr, int x, int y, int z, int val);

    struct CompoundTagLayout
    {
        void* vtable;
        std::wstring name;
        std::unordered_map<std::wstring, void*> tags;
    };

    struct StringTagLayout
    {
        void* vtable;
        std::wstring name;
        std::wstring data;
    };

    std::once_flag s_symbolInitOnce;
    TagNewTag_fn s_tagNewTag = nullptr;
    void* s_tileArray = nullptr;
    LevelChunkGetTile_fn s_levelChunkGetTile = nullptr;
    LevelChunkSetTile_fn s_levelChunkSetTile = nullptr;
    LevelChunkGetPos_fn s_levelChunkGetPos = nullptr;
    LevelChunkGetHighestNonEmptyY_fn s_levelChunkGetHighestNonEmptyY = nullptr;
    CompressedTileStorageSet_fn s_compressedTileStorageSet = nullptr;

    // LevelChunk layout: vtable(8) + byteArray(16) = 24, then lowerBlocks at 24, upperBlocks at 32
    static constexpr size_t kLevelChunkLowerBlocksOffset = 24;
    bool s_missingPlaceholdersReady = false;
    int s_chunkRemapLogCount = 0;
    int s_chunkSaveLogCount = 0;
    int s_chunkIoLogCount = 0;
    std::mutex s_chunkMetaMutex;

    struct ChunkMeta
    {
        void* storagePtr = nullptr;
        int x = 0;
        int z = 0;
    };
    std::unordered_map<void*, ChunkMeta> s_chunkMetaByPtr;
    std::unordered_map<void*, std::unordered_map<int, std::string>> s_loadedNamespaceByChunk;
    std::unordered_map<std::string, std::unordered_map<int, std::string>> s_chunkNamespaceCache;

    struct ConsoleSavePathLayout
    {
        std::wstring path;
        explicit ConsoleSavePathLayout(const std::wstring& p) : path(p) {}
    };

    struct McRegionChunkStorageLayout
    {
        void* vtable;
        std::wstring prefix;
        void* saveFile;
    };

    struct ChunkPosLayout
    {
        int x;
        int z;
    };

    struct RuntimeConsoleSaveFile
    {
        virtual ~RuntimeConsoleSaveFile() = default;
        virtual void* createFile(const ConsoleSavePathLayout& fileName) = 0;
        virtual void deleteFile(void* fileEntry) = 0;
        virtual void setFilePointer(void* fileEntry, long distance, long* distanceHigh, unsigned long moveMethod) = 0;
        virtual int writeFile(void* fileEntry, const void* buffer, unsigned long bytesToWrite, unsigned long* bytesWritten) = 0;
        virtual int zeroFile(void* fileEntry, unsigned long bytesToWrite, unsigned long* bytesWritten) = 0;
        virtual int readFile(void* fileEntry, void* buffer, unsigned long bytesToRead, unsigned long* bytesRead) = 0;
        virtual int closeHandle(void* fileEntry) = 0;
        virtual void finalizeWrite() = 0;
        virtual void tick() = 0;
    };

    struct RuntimeFileEntryLayout
    {
        wchar_t filename[64];
        unsigned int length;
        unsigned int startOffsetOrRegion;
        long long lastModifiedTime;
        unsigned int currentFilePointer;
    };
    static bool IsReadableRange(const void* ptr, size_t bytes);

    static bool IsValidChunkStorage(const void* storagePtr)
    {
        return storagePtr && IsReadableRange(storagePtr, sizeof(void*) + sizeof(std::wstring) + sizeof(void*));
    }

    static std::wstring MakeChunkNamespacePathCoordsOnly(int chunkX, int chunkZ)
    {
        std::wstringstream ss;
        ss << L"modloader\\blockns_v3\\c." << chunkX << L"." << chunkZ << L".txt";
        return ss.str();
    }

    static std::string MakeChunkCacheKey(const McRegionChunkStorageLayout* storage, int chunkX, int chunkZ)
    {
        const std::wstring path = MakeChunkNamespacePathCoordsOnly(chunkX, chunkZ);
        std::string key(path.begin(), path.end());
        return key;
    }

    static bool SaveReadAllText(void* saveFile, const std::wstring& path, std::string* outText)
    {
        if (!saveFile || !outText || !IsReadableRange(saveFile, sizeof(void*)))
            return false;
        auto* save = reinterpret_cast<RuntimeConsoleSaveFile*>(saveFile);

        ConsoleSavePathLayout savePath(path);
        void* fileEntry = save->createFile(savePath);
        if (!fileEntry)
            return false;
        save->setFilePointer(fileEntry, 0, nullptr, FILE_BEGIN);

        if (!IsReadableRange(fileEntry, sizeof(RuntimeFileEntryLayout)))
        {
            save->closeHandle(fileEntry);
            return false;
        }
        const auto* entry = reinterpret_cast<const RuntimeFileEntryLayout*>(fileEntry);
        const unsigned int fileSize = entry->length;
        // Hard cap for corrupted metadata; chunk namespace files should stay tiny.
        if (fileSize > (1024u * 1024u))
        {
            save->closeHandle(fileEntry);
            return false;
        }

        std::string text;
        if (fileSize > 0)
        {
            text.resize(fileSize);
            unsigned long bytesRead = 0;
            const int ok = save->readFile(fileEntry, text.data(), fileSize, &bytesRead);
            if (!ok)
            {
                save->closeHandle(fileEntry);
                return false;
            }
            text.resize(bytesRead);
        }

        save->closeHandle(fileEntry);
        *outText = std::move(text);
        return true;
    }

    static bool SaveWriteAllText(void* saveFile, const std::wstring& path, const std::string& text)
    {
        if (!saveFile || !IsReadableRange(saveFile, sizeof(void*)))
            return false;
        auto* save = reinterpret_cast<RuntimeConsoleSaveFile*>(saveFile);

        ConsoleSavePathLayout savePath(path);
        void* fileEntry = save->createFile(savePath);
        if (!fileEntry)
            return false;
        save->setFilePointer(fileEntry, 0, nullptr, FILE_BEGIN);

        unsigned long bytesWritten = 0;
        const int ok = save->writeFile(fileEntry, text.data(), static_cast<unsigned long>(text.size()), &bytesWritten);
        save->closeHandle(fileEntry);
        return ok && bytesWritten == text.size();
    }

    static bool WriteChunkNamespaceMap(
        void* saveFile,
        const std::wstring& path,
        const std::unordered_map<int, std::string>& map);

    static bool ReadChunkNamespaceMap(
        void* saveFile,
        const std::wstring& path,
        std::unordered_map<int, std::string>* outMap)
    {
        if (!outMap)
            return false;
        outMap->clear();

        std::string text;
        if (!SaveReadAllText(saveFile, path, &text))
            return false;

        std::istringstream input(text);
        std::string line;
        int expectedEntries = -1;
        int parsedEntries = 0;
        while (std::getline(input, line))
        {
            if (line.empty())
                continue;

            if (expectedEntries < 0 && line.rfind("#count ", 0) == 0)
            {
                std::istringstream hs(line.substr(7));
                int count = -1;
                if (hs >> count && count >= 0)
                    expectedEntries = count;
                continue;
            }

            std::istringstream ss(line);
            int blockIndex = -1;
            std::string namespacedId;
            if (!(ss >> blockIndex >> namespacedId))
                continue;
            if (blockIndex < 0 || namespacedId.empty())
                continue;
            (*outMap)[blockIndex] = namespacedId;
            ++parsedEntries;
            if (expectedEntries >= 0 && parsedEntries >= expectedEntries)
                break;
        }
        if (expectedEntries < 0)
        {
            // Require explicit count header for safety; legacy files can contain stale tail bytes.
            outMap->clear();
            return false;
        }
        return true;
    }

    static bool IsLikelyNamespacedId(const std::string& s)
    {
        if (s.size() < 3 || s.size() > 200)
            return false;
        if (s.find(':') == std::string::npos)
            return false;
        for (char c : s)
        {
            if (c <= 0x20 || c == '\x7f')
                return false;
        }
        return true;
    }

    static bool ReadChunkNamespaceMapWithFallback(
        const McRegionChunkStorageLayout* storage,
        int chunkX,
        int chunkZ,
        std::unordered_map<int, std::string>* outMap)
    {
        if (!storage || !storage->saveFile || !outMap)
            return false;
        outMap->clear();

        // Preferred stable path that does not depend on object layout/prefix decoding.
        const std::wstring v3Path = MakeChunkNamespacePathCoordsOnly(chunkX, chunkZ);
        if (ReadChunkNamespaceMap(storage->saveFile, v3Path, outMap) && !outMap->empty())
            return true;
        return false;
    }

    static bool WriteChunkNamespaceMap(
        void* saveFile,
        const std::wstring& path,
        const std::unordered_map<int, std::string>& map)
    {
        std::ostringstream out;
        out << "#count " << map.size() << '\n';
        for (const auto& entry : map)
            out << entry.first << ' ' << entry.second << '\n';
        return SaveWriteAllText(saveFile, path, out.str());
    }

    static int MakeBlockIndex(int x, int y, int z)
    {
        return ((y & 0xFF) << 8) | ((z & 0x0F) << 4) | (x & 0x0F);
    }

    static void DecodeBlockIndex(int index, int* x, int* y, int* z)
    {
        if (x) *x = index & 0x0F;
        if (z) *z = (index >> 4) & 0x0F;
        if (y) *y = (index >> 8) & 0xFF;
    }

    static bool TryGetChunkMeta(void* chunkPtr, ChunkMeta* outMeta)
    {
        if (!chunkPtr || !outMeta)
            return false;
        std::lock_guard<std::mutex> lock(s_chunkMetaMutex);
        const auto it = s_chunkMetaByPtr.find(chunkPtr);
        if (it == s_chunkMetaByPtr.end())
            return false;
        *outMeta = it->second;
        return true;
    }

    static void SetChunkMeta(void* chunkPtr, void* storagePtr, int chunkX, int chunkZ)
    {
        if (!chunkPtr || !storagePtr)
            return;
        std::lock_guard<std::mutex> lock(s_chunkMetaMutex);
        ChunkMeta& meta = s_chunkMetaByPtr[chunkPtr];
        meta.storagePtr = storagePtr;
        meta.x = chunkX;
        meta.z = chunkZ;
    }

    static void SetLoadedChunkNamespaces(void* chunkPtr, const std::unordered_map<int, std::string>& entries)
    {
        if (!chunkPtr)
            return;
        std::lock_guard<std::mutex> lock(s_chunkMetaMutex);
        s_loadedNamespaceByChunk[chunkPtr] = entries;
    }

    static bool TryGetLoadedChunkNamespaces(void* chunkPtr, std::unordered_map<int, std::string>* outEntries)
    {
        if (!chunkPtr || !outEntries)
            return false;
        std::lock_guard<std::mutex> lock(s_chunkMetaMutex);
        const auto it = s_loadedNamespaceByChunk.find(chunkPtr);
        if (it == s_loadedNamespaceByChunk.end())
            return false;
        *outEntries = it->second;
        return true;
    }

    static bool TryResolveChunkCoords(void* levelChunkPtr, int* outX, int* outZ)
    {
        if (!levelChunkPtr || !s_levelChunkGetPos)
            return false;
        void* posPtr = s_levelChunkGetPos(levelChunkPtr);
        if (!posPtr || !IsReadableRange(posPtr, sizeof(ChunkPosLayout)))
            return false;
        const auto* pos = reinterpret_cast<const ChunkPosLayout*>(posPtr);
        if (outX) *outX = pos->x;
        if (outZ) *outZ = pos->z;
        return true;
    }

    static bool IsValidRuntimeTileId(int tileId)
    {
        if (tileId == 0)
            return true;
        if (tileId < 0 || tileId > 255)
            return false;
        if (!s_tileArray || !IsReadableRange(s_tileArray, sizeof(void*)))
            return true; // can't validate, don't aggressively rewrite

        // Tile::tiles is a global Tile** pointer variable in this build.
        // Symbol points at the variable, so dereference once to get the actual table.
        auto** tiles = *reinterpret_cast<void***>(s_tileArray);
        if (!tiles || !IsReadableRange(tiles, sizeof(void*) * 256))
            return true; // can't validate safely

        return tiles[tileId] != nullptr;
    }

    static bool IsVanillaId(const std::string& namespacedId)
    {
        return namespacedId.rfind("minecraft:", 0) == 0;
    }

    static int ResolveSafeMissingBlockFallbackId()
    {
        int fallbackId = IdRegistry::Instance().GetMissingFallback(IdRegistry::Type::Block);
        if (IsValidRuntimeTileId(fallbackId))
            return fallbackId;
        const int missingBlockId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, kMissingBlockId);
        if (IsValidRuntimeTileId(missingBlockId))
            return missingBlockId;
        if (IsValidRuntimeTileId(kMissingBlockVanillaFallbackId))
            return kMissingBlockVanillaFallbackId;
        if (IsValidRuntimeTileId(1))
            return 1;
        return 0;
    }

    static bool IsTilePointerValid(int tileId)
    {
        if (tileId == 0)
            return true;
        if (tileId < 0 || tileId > 255)
            return false;
        if (!s_tileArray || !IsReadableRange(s_tileArray, sizeof(void*)))
            return true;
        auto** tiles = *reinterpret_cast<void***>(s_tileArray);
        if (!tiles || !IsReadableRange(tiles, sizeof(void*) * 256))
            return true;
        return tiles[tileId] != nullptr;
    }

    static bool SafeSetChunkTile(void* levelChunkPtr, int x, int y, int z, int tileId)
    {
        if (!s_levelChunkSetTile)
            return false;

        // If the current block has no valid Tile pointer, setTileAndData will crash when it
        // calls Tile::tiles[old]->onRemoving. Clear it to air first via direct block storage write.
        if (s_levelChunkGetTile && s_compressedTileStorageSet && levelChunkPtr && IsReadableRange(levelChunkPtr, kLevelChunkLowerBlocksOffset + 16))
        {
#if defined(_MSC_VER)
            __try
            {
                int current = s_levelChunkGetTile(levelChunkPtr, x, y, z);
                if (current != 0 && !IsTilePointerValid(current))
                {
                    void* blocksPtr = *reinterpret_cast<void**>(static_cast<char*>(levelChunkPtr) + (y >= 128 ? kLevelChunkLowerBlocksOffset + 8 : kLevelChunkLowerBlocksOffset));
                    if (blocksPtr && IsReadableRange(blocksPtr, sizeof(void*)))
                    {
                        s_compressedTileStorageSet(blocksPtr, x, y % 128, z, 0);
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                // Ignore - we'll try setTile anyway
            }
#endif
        }

#if defined(_MSC_VER)
        __try
        {
            s_levelChunkSetTile(levelChunkPtr, x, y, z, tileId);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
#else
        s_levelChunkSetTile(levelChunkPtr, x, y, z, tileId);
        return true;
#endif
    }

    static std::string ResolveNamespacedItemId(int numericId)
    {
        std::string namespacedId = IdRegistry::Instance().GetStringId(IdRegistry::Type::Item, numericId);
        if (!namespacedId.empty())
            return namespacedId;

        // Block-backed inventory entries are TileItems whose stored item ID matches the block ID.
        return IdRegistry::Instance().GetStringId(IdRegistry::Type::Block, numericId);
    }

    static int ResolveNumericItemId(const std::string& namespacedId, int oldNumericId, const std::wstring& kind = L"")
    {
        if (namespacedId.empty())
            return -1;

        const bool wasKnownAsBlock = !IdRegistry::Instance().GetStringId(IdRegistry::Type::Block, oldNumericId).empty();
        const bool wasKnownAsItem = !IdRegistry::Instance().GetStringId(IdRegistry::Type::Item, oldNumericId).empty();

        if (wasKnownAsBlock && !wasKnownAsItem)
        {
            int blockId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, namespacedId);
            if (blockId >= 0)
                return blockId;
        }

        int itemId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Item, namespacedId);
        if (itemId >= 0)
            return itemId;

        int blockId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, namespacedId);
        if (blockId >= 0)
            return blockId;

        if (wasKnownAsBlock || kind == L"block")
            return IdRegistry::Instance().GetMissingFallback(IdRegistry::Type::Block);

        return IdRegistry::Instance().GetMissingFallback(IdRegistry::Type::Item);
    }

    static bool IsReadableRange(const void* ptr, size_t bytes);

    static CompoundTagLayout* GetItemTag(void* itemInstancePtr)
    {
        if (!itemInstancePtr || !IsReadableRange(static_cast<const char*>(itemInstancePtr) + kItemTagOffset, sizeof(void*)))
            return nullptr;
        return *reinterpret_cast<CompoundTagLayout**>(static_cast<char*>(itemInstancePtr) + kItemTagOffset);
    }

    static CompoundTagLayout* EnsureItemTag(void* itemInstancePtr)
    {
        CompoundTagLayout* tag = GetItemTag(itemInstancePtr);
        if (tag || !s_tagNewTag)
            return tag;

        void* createdTag = s_tagNewTag(kTagCompoundId, L"");
        if (!createdTag)
            return nullptr;

        *reinterpret_cast<void**>(static_cast<char*>(itemInstancePtr) + kItemTagOffset) = createdTag;
        return reinterpret_cast<CompoundTagLayout*>(createdTag);
    }

    static bool TryGetCompoundString(CompoundTagLayout* compoundTag, const std::wstring& key, std::wstring* outValue)
    {
        if (!compoundTag || !outValue)
            return false;

        const auto it = compoundTag->tags.find(key);
        if (it == compoundTag->tags.end() || !it->second)
            return false;

        const auto* stringTag = reinterpret_cast<const StringTagLayout*>(it->second);
        *outValue = stringTag->data;
        return true;
    }

    static bool PutCompoundString(CompoundTagLayout* compoundTag, const std::wstring& key, const std::wstring& value)
    {
        if (!compoundTag)
            return false;

        const auto it = compoundTag->tags.find(key);
        if (it != compoundTag->tags.end() && it->second)
        {
            auto* stringTag = reinterpret_cast<StringTagLayout*>(it->second);
            stringTag->data = value;
            return true;
        }

        if (!s_tagNewTag)
            return false;

        void* createdTag = s_tagNewTag(kTagStringId, key);
        if (!createdTag)
            return false;

        auto* stringTag = reinterpret_cast<StringTagLayout*>(createdTag);
        stringTag->data = value;
        compoundTag->tags[key] = createdTag;
        return true;
    }

    static bool IsReadableRange(const void* ptr, size_t bytes)
    {
        if (!ptr || bytes == 0)
            return false;

        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) != sizeof(mbi))
            return false;
        if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS || (mbi.Protect & PAGE_GUARD))
            return false;

        const DWORD readMask = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                               PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        if ((mbi.Protect & readMask) == 0)
            return false;

        const uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        return p + bytes >= p && p + bytes <= end;
    }

    static void ResolveSymbolsOnce()
    {
        std::call_once(s_symbolInitOnce, []() {
            if (s_tagNewTag)
                return;

            const uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
            uint32_t newTagRvaByName = PdbParser::FindSymbolRVAByName("Tag::newTag");
            uint32_t newTagRvaByDecorated = PdbParser::FindSymbolRVA(
                "?newTag@Tag@@SAPEAV1@EAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z");
            uint32_t newTagRva = newTagRvaByName;
            if (newTagRva == 0)
                newTagRva = newTagRvaByDecorated;
            if (newTagRva)
                s_tagNewTag = reinterpret_cast<TagNewTag_fn>(base + newTagRva);
        });
    }
}

namespace WorldIdRemap
{
    static thread_local int s_remapStage = 0;

    void SetTagNewTagSymbol(void* fnPtr)
    {
        s_tagNewTag = reinterpret_cast<TagNewTag_fn>(fnPtr);
    }

    void SetTileArraySymbol(void* tileArrayPtr)
    {
        s_tileArray = tileArrayPtr;
    }

    void SetLevelChunkTileSymbols(void* getTileFn, void* setTileFn, void* getPosFn, void* getHighestNonEmptyYFn)
    {
        s_levelChunkGetTile = reinterpret_cast<LevelChunkGetTile_fn>(getTileFn);
        s_levelChunkSetTile = reinterpret_cast<LevelChunkSetTile_fn>(setTileFn);
        s_levelChunkGetPos = reinterpret_cast<LevelChunkGetPos_fn>(getPosFn);
        s_levelChunkGetHighestNonEmptyY = reinterpret_cast<LevelChunkGetHighestNonEmptyY_fn>(getHighestNonEmptyYFn);
    }

    void SetCompressedTileStorageSetSymbol(void* setFn)
    {
        s_compressedTileStorageSet = reinterpret_cast<CompressedTileStorageSet_fn>(setFn);
    }

    void EnsureMissingPlaceholders()
    {
        if (s_missingPlaceholdersReady)
            return;

        const int missingBlockId = IdRegistry::Instance().Register(IdRegistry::Type::Block, kMissingBlockId);
        const int missingItemId = IdRegistry::Instance().Register(IdRegistry::Type::Item, kMissingItemId);

        if (missingBlockId < 0)
            LogUtil::Log("[WeaveLoader] Failed to register missing block placeholder (no free block IDs)");
        if (missingItemId < 0)
            LogUtil::Log("[WeaveLoader] Failed to register missing item placeholder (no free item IDs)");

        if (missingBlockId >= 0)
        {
            ModStrings::Register(kMissingBlockDescriptionId, L"Missing Block");
            GameObjectFactory::CreateTile(
                missingBlockId,
                1,
                1.0f,
                1.0f,
                1,
                L"bedrock",
                0.0f,
                15,
                kMissingBlockDescriptionId);
            IdRegistry::Instance().SetMissingFallback(IdRegistry::Type::Block, missingBlockId);
        }

        if (missingItemId >= 0)
        {
            ModStrings::Register(kMissingItemDescriptionId, L"Missing Item");
            GameObjectFactory::CreateItem(
                missingItemId,
                64,
                0,
                L"apple",
                kMissingItemDescriptionId);
            IdRegistry::Instance().SetMissingFallback(IdRegistry::Type::Item, missingItemId);
        }

        s_missingPlaceholdersReady = true;
        LogUtil::Log("[WeaveLoader] Missing content placeholders ready: block=%d item=%d",
                     IdRegistry::Instance().GetMissingFallback(IdRegistry::Type::Block),
                     IdRegistry::Instance().GetMissingFallback(IdRegistry::Type::Item));
    }

    static int RemapChunkBlockIdsImpl(void* chunkStoragePtr, void* levelChunkPtr, int chunkX, int chunkZ)
    {
        s_remapStage = 1;
        if (!chunkStoragePtr || !levelChunkPtr || !s_levelChunkGetTile || !s_levelChunkSetTile)
            return 0;
        if (!IsValidChunkStorage(chunkStoragePtr))
            return 0;

        auto* storage = reinterpret_cast<McRegionChunkStorageLayout*>(chunkStoragePtr);
        if (!storage->saveFile)
            return 0;

        SetChunkMeta(levelChunkPtr, chunkStoragePtr, chunkX, chunkZ);

        std::unordered_map<int, std::string> entries;
        const std::string cacheKey = MakeChunkCacheKey(storage, chunkX, chunkZ);
        {
            std::lock_guard<std::mutex> lock(s_chunkMetaMutex);
            const auto cacheIt = s_chunkNamespaceCache.find(cacheKey);
            if (cacheIt != s_chunkNamespaceCache.end())
                entries = cacheIt->second;
        }
        if (s_chunkIoLogCount < 32)
        {
            ++s_chunkIoLogCount;
            LogUtil::Log("[WeaveLoader] WorldIdRemap blocks: reading chunk namespace map x=%d z=%d", chunkX, chunkZ);
        }
        if (entries.empty())
        {
            s_remapStage = 2;
            if (ReadChunkNamespaceMapWithFallback(storage, chunkX, chunkZ, &entries) && !entries.empty())
            {
                std::lock_guard<std::mutex> lock(s_chunkMetaMutex);
                s_chunkNamespaceCache[cacheKey] = entries;
            }
        }
        if (!entries.empty())
            SetLoadedChunkNamespaces(levelChunkPtr, entries);

        const int sanitizeFallbackId = ResolveSafeMissingBlockFallbackId();
        int applyMaxY = 127;
        if (s_levelChunkGetHighestNonEmptyY)
        {
            const int highestY = s_levelChunkGetHighestNonEmptyY(levelChunkPtr);
            if (highestY >= 0 && highestY < kChunkMaxY)
            {
                int bound = highestY + 16;
                if (bound >= kChunkMaxY)
                    bound = kChunkMaxY - 1;
                applyMaxY = bound;
            }
            else if (highestY >= 0)
            {
                applyMaxY = kChunkMaxY - 1;
            }
        }

        if (applyMaxY > 127)
            applyMaxY = 127;

        int changed = 0;
        int skippedInvalid = 0;
        int writeExceptions = 0;
        s_remapStage = 3;
        for (const auto& entry : entries)
        {
            int x = 0;
            int y = 0;
            int z = 0;
            DecodeBlockIndex(entry.first, &x, &y, &z);
            if (y < 0 || y > applyMaxY)
            {
                ++skippedInvalid;
                continue;
            }
            if (!IsLikelyNamespacedId(entry.second))
            {
                ++skippedInvalid;
                continue;
            }
            int newId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, entry.second);
            if (newId < 0)
                newId = sanitizeFallbackId;
            if (!IsValidRuntimeTileId(newId))
                newId = sanitizeFallbackId;
            if (newId < 0 || newId > 255)
            {
                ++skippedInvalid;
                continue;
            }
            if (!SafeSetChunkTile(levelChunkPtr, x, y, z, newId))
            {
                ++writeExceptions;
                continue;
            }
            ++changed;
        }

        if (changed > 0 && s_chunkRemapLogCount < 64)
        {
            ++s_chunkRemapLogCount;
            LogUtil::Log("[WeaveLoader] WorldIdRemap blocks: remapped %d block ids in chunk", changed);
        }
        if (skippedInvalid > 0 && s_chunkRemapLogCount < 64)
        {
            ++s_chunkRemapLogCount;
            LogUtil::Log("[WeaveLoader] WorldIdRemap blocks: skipped %d invalid namespace entries in chunk", skippedInvalid);
        }
        if (writeExceptions > 0 && s_chunkRemapLogCount < 64)
        {
            ++s_chunkRemapLogCount;
            LogUtil::Log("[WeaveLoader] WorldIdRemap blocks: skipped %d writes due to setTile exceptions", writeExceptions);
        }
        s_remapStage = 0;
        return changed;
    }

    int RemapChunkBlockIds(void* chunkStoragePtr, void* levelChunkPtr, int chunkX, int chunkZ)
    {
#if defined(_MSC_VER)
        __try
        {
            return RemapChunkBlockIdsImpl(chunkStoragePtr, levelChunkPtr, chunkX, chunkZ);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LogUtil::Log("[WeaveLoader] WorldIdRemap blocks: exception while remapping chunk x=%d z=%d stage=%d (skipped)", chunkX, chunkZ, s_remapStage);
            return 0;
        }
#else
        return RemapChunkBlockIdsImpl(chunkStoragePtr, levelChunkPtr, chunkX, chunkZ);
#endif
    }

    void SaveChunkBlockNamespaces(void* chunkStoragePtr, void* levelChunkPtr)
    {
        if (!chunkStoragePtr || !levelChunkPtr || !s_levelChunkGetTile)
            return;
        if (!IsValidChunkStorage(chunkStoragePtr))
            return;

        auto* storage = reinterpret_cast<McRegionChunkStorageLayout*>(chunkStoragePtr);
        if (!storage->saveFile)
            return;

        ChunkMeta meta{};
        if (!TryGetChunkMeta(levelChunkPtr, &meta))
        {
            meta.storagePtr = chunkStoragePtr;
            if (!TryResolveChunkCoords(levelChunkPtr, &meta.x, &meta.z))
                return;
            SetChunkMeta(levelChunkPtr, chunkStoragePtr, meta.x, meta.z);
        }

        const std::wstring path = MakeChunkNamespacePathCoordsOnly(meta.x, meta.z);
        const std::string cacheKey = MakeChunkCacheKey(storage, meta.x, meta.z);
        std::unordered_map<int, std::string> loadedEntries;
        const bool hadLoadedMap = TryGetLoadedChunkNamespaces(levelChunkPtr, &loadedEntries) && !loadedEntries.empty();

        std::unordered_map<int, std::string> nextEntries;
        const int missingBlockFallback = IdRegistry::Instance().GetMissingFallback(IdRegistry::Type::Block);
        int maxY = kChunkMaxY - 1;
        if (s_levelChunkGetHighestNonEmptyY)
        {
            const int highestY = s_levelChunkGetHighestNonEmptyY(levelChunkPtr);
            if (highestY < 0 && !hadLoadedMap)
                return;
            if (highestY >= 0 && highestY < kChunkMaxY)
                maxY = highestY;
        }

        for (int y = 0; y <= maxY; ++y)
        {
            for (int z = 0; z < kChunkWidth; ++z)
            {
                for (int x = 0; x < kChunkWidth; ++x)
                {
                    const int blockId = s_levelChunkGetTile(levelChunkPtr, x, y, z);
                    const int blockIndex = MakeBlockIndex(x, y, z);

                    if (blockId == missingBlockFallback)
                    {
                        const auto loadedIt = loadedEntries.find(blockIndex);
                        if (loadedIt != loadedEntries.end() && !loadedIt->second.empty() && loadedIt->second != kMissingBlockId)
                        {
                            nextEntries[blockIndex] = loadedIt->second;
                            continue;
                        }
                        continue;
                    }

                    std::string namespacedId = IdRegistry::Instance().GetStringId(IdRegistry::Type::Block, blockId);
                    if (namespacedId.empty())
                        continue;
                    if (IsVanillaId(namespacedId))
                        continue;
                    if (namespacedId == kMissingBlockId)
                    {
                        const auto loadedIt = loadedEntries.find(blockIndex);
                        if (loadedIt != loadedEntries.end() && !loadedIt->second.empty() && loadedIt->second != kMissingBlockId)
                        {
                            nextEntries[blockIndex] = loadedIt->second;
                            continue;
                        }
                        continue;
                    }

                    nextEntries[blockIndex] = std::move(namespacedId);
                }
            }
        }

        if (nextEntries.empty() && !hadLoadedMap)
            return;
        if (!WriteChunkNamespaceMap(storage->saveFile, path, nextEntries))
            return;
        {
            std::lock_guard<std::mutex> lock(s_chunkMetaMutex);
            s_chunkNamespaceCache[cacheKey] = nextEntries;
        }

        if (s_chunkSaveLogCount < 64)
        {
            ++s_chunkSaveLogCount;
            LogUtil::Log("[WeaveLoader] WorldIdRemap blocks: saved %zu namespace entries for chunk x=%d z=%d",
                         nextEntries.size(), meta.x, meta.z);
        }
    }

    void TagModdedItemInstance(void* itemInstancePtr, void* compoundTagPtr)
    {
        ResolveSymbolsOnce();
        if (!itemInstancePtr)
            return;
        if (!IsReadableRange(static_cast<const char*>(itemInstancePtr) + kItemIdOffset, sizeof(int)))
            return;
        (void)compoundTagPtr;

        const int itemId = *reinterpret_cast<const int*>(static_cast<const char*>(itemInstancePtr) + kItemIdOffset);
        const std::string namespacedId = ResolveNamespacedItemId(itemId);
        if (namespacedId.empty() || IsVanillaId(namespacedId))
            return;
        const bool isBlockBacked = !IdRegistry::Instance().GetStringId(IdRegistry::Type::Block, itemId).empty() &&
                                   IdRegistry::Instance().GetStringId(IdRegistry::Type::Item, itemId).empty();

        CompoundTagLayout* itemTag = EnsureItemTag(itemInstancePtr);
        if (!itemTag)
            return;

        // Never let placeholder IDs overwrite a previously saved real namespace.
        if (namespacedId == kMissingItemId || namespacedId == kMissingBlockId)
        {
            std::wstring existingNamespace;
            if (TryGetCompoundString(itemTag, kNamespaceTagKey, &existingNamespace) && !existingNamespace.empty())
                return;
        }

        const std::wstring namespacedWide(namespacedId.begin(), namespacedId.end());
        if (!PutCompoundString(itemTag, kNamespaceTagKey, namespacedWide))
            return;
        (void)PutCompoundString(itemTag, kNamespaceKindTagKey, isBlockBacked ? L"block" : L"item");
    }

    void RemapItemInstanceFromTag(void* itemInstancePtr, void* compoundTagPtr)
    {
        ResolveSymbolsOnce();
        if (!itemInstancePtr)
            return;
        (void)compoundTagPtr;
        if (!IsReadableRange(static_cast<const char*>(itemInstancePtr) + kItemIdOffset, sizeof(int)))
            return;

        const int oldItemId = *reinterpret_cast<const int*>(static_cast<const char*>(itemInstancePtr) + kItemIdOffset);
        CompoundTagLayout* itemTag = GetItemTag(itemInstancePtr);
        if (!itemTag)
            return;

        std::wstring namespacedWide;
        if (!TryGetCompoundString(itemTag, kNamespaceTagKey, &namespacedWide) || namespacedWide.empty())
            return;
        std::wstring kindWide;
        (void)TryGetCompoundString(itemTag, kNamespaceKindTagKey, &kindWide);

        const std::string namespacedId(namespacedWide.begin(), namespacedWide.end());
        int numericId = ResolveNumericItemId(namespacedId, oldItemId, kindWide);
        if (numericId >= 0)
            *reinterpret_cast<int*>(static_cast<char*>(itemInstancePtr) + kItemIdOffset) = numericId;
    }
}
