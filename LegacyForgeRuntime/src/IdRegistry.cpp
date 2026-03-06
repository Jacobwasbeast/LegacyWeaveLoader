#include "IdRegistry.h"
#include <cstdio>

IdRegistry& IdRegistry::Instance()
{
    static IdRegistry instance;
    return instance;
}

IdRegistry::IdRegistry()
{
    m_registries[static_cast<int>(Type::Block)].nextFreeId = BLOCK_MOD_START;
    m_registries[static_cast<int>(Type::Item)].nextFreeId = ITEM_MOD_START;
    m_registries[static_cast<int>(Type::Entity)].nextFreeId = ENTITY_MOD_START;
}

int IdRegistry::Register(Type type, const std::string& namespacedId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& reg = m_registries[static_cast<int>(type)];

    auto it = reg.stringToNum.find(namespacedId);
    if (it != reg.stringToNum.end())
        return it->second;

    int maxId;
    switch (type)
    {
    case Type::Block:  maxId = BLOCK_MAX; break;
    case Type::Item:   maxId = ITEM_MAX; break;
    case Type::Entity: maxId = ENTITY_MAX; break;
    default: return -1;
    }

    if (reg.nextFreeId > maxId)
    {
        printf("[LegacyForge] IdRegistry: No free IDs for type %d (max %d)\n",
               static_cast<int>(type), maxId);
        return -1;
    }

    int id = reg.nextFreeId++;

    // Skip IDs that are already taken by vanilla entries
    while (reg.numToString.count(id) && id <= maxId)
        id = reg.nextFreeId++;

    if (id > maxId) return -1;

    reg.stringToNum[namespacedId] = id;
    reg.numToString[id] = namespacedId;

    return id;
}

int IdRegistry::GetNumericId(Type type, const std::string& namespacedId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto& reg = m_registries[static_cast<int>(type)];
    auto it = reg.stringToNum.find(namespacedId);
    return (it != reg.stringToNum.end()) ? it->second : -1;
}

std::string IdRegistry::GetStringId(Type type, int numericId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto& reg = m_registries[static_cast<int>(type)];
    auto it = reg.numToString.find(numericId);
    return (it != reg.numToString.end()) ? it->second : "";
}

void IdRegistry::RegisterVanilla(Type type, int numericId, const std::string& namespacedId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& reg = m_registries[static_cast<int>(type)];
    reg.stringToNum[namespacedId] = numericId;
    reg.numToString[numericId] = namespacedId;
}
