#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

/// Maps namespaced string IDs ("namespace:path") to auto-allocated numeric IDs.
/// Separate pools for blocks, items, and entities.
/// Thread-safe for registration calls from the .NET runtime.
class IdRegistry
{
public:
    enum class Type { Block, Item, Entity };

    static IdRegistry& Instance();

    /// Register a new entry and auto-allocate a numeric ID.
    /// Returns the allocated numeric ID, or -1 on failure.
    int Register(Type type, const std::string& namespacedId);

    /// Look up the numeric ID for a string ID. Returns -1 if not found.
    int GetNumericId(Type type, const std::string& namespacedId) const;

    /// Look up the string ID for a numeric ID. Returns empty string if not found.
    std::string GetStringId(Type type, int numericId) const;

    /// Pre-register a vanilla entry with a known numeric ID.
    void RegisterVanilla(Type type, int numericId, const std::string& namespacedId);

private:
    IdRegistry();

    struct RegistryData
    {
        std::unordered_map<std::string, int> stringToNum;
        std::unordered_map<int, std::string> numToString;
        int nextFreeId;
    };

    // Tile IDs 174-255 are unused by vanilla (161-169 also free but small).
    // Must stay <= 255 so TileItem can map tile ID to Item::items[tileId].
    static constexpr int BLOCK_MOD_START = 174;
    static constexpr int BLOCK_MAX = 255;

    // Item IDs here are the FINAL id stored in Item::id (= 256 + constructor param).
    // Vanilla records go up to stored ID 2267 (constructor param 2011).
    // We start above that to avoid conflicts.
    static constexpr int ITEM_MOD_START = 3000;
    static constexpr int ITEM_MAX = 31999;
    static constexpr int ENTITY_MOD_START = 1000;
    static constexpr int ENTITY_MAX = 9999;

    RegistryData m_registries[3];
    mutable std::mutex m_mutex;
};
