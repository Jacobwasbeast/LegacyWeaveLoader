#include "ModelRegistry.h"
#include "LogUtil.h"
#include <unordered_map>
#include <mutex>
#include <string>

namespace
{
    struct BlockModelEntry
    {
        std::vector<ModelBox> base;
        std::unordered_map<std::string, std::vector<ModelBox>> variants;
        int rotationProfile = 0;
    };

    std::unordered_map<int, BlockModelEntry> g_models;
    std::mutex g_mutex;
}

void ModelRegistry::RegisterBlockModel(int blockId, const ModelBox* boxes, int count)
{
    if (blockId < 0 || !boxes || count <= 0)
        return;

    std::vector<ModelBox> data;
    data.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; i++)
        data.push_back(boxes[i]);

    {
        std::lock_guard<std::mutex> guard(g_mutex);
        g_models[blockId].base = std::move(data);
    }

    LogUtil::Log("[WeaveLoader] ModelRegistry: registered %d box(es) for block %d", count, blockId);
}

void ModelRegistry::RegisterBlockModelVariant(int blockId, const char* key, const ModelBox* boxes, int count)
{
    if (blockId < 0 || !key || !boxes || count <= 0)
        return;

    std::vector<ModelBox> data;
    data.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; i++)
        data.push_back(boxes[i]);

    {
        std::lock_guard<std::mutex> guard(g_mutex);
        g_models[blockId].variants[std::string(key)] = std::move(data);
    }

    LogUtil::Log("[WeaveLoader] ModelRegistry: registered variant '%s' (%d box(es)) for block %d", key, count, blockId);
}

void ModelRegistry::SetRotationProfile(int blockId, int profile)
{
    if (blockId < 0)
        return;
    std::lock_guard<std::mutex> guard(g_mutex);
    g_models[blockId].rotationProfile = profile;
}

int ModelRegistry::GetRotationProfile(int blockId)
{
    std::lock_guard<std::mutex> guard(g_mutex);
    auto it = g_models.find(blockId);
    if (it == g_models.end())
        return 0;
    return it->second.rotationProfile;
}

bool ModelRegistry::TryGetModel(int blockId, const std::vector<ModelBox>*& outBoxes)
{
    std::lock_guard<std::mutex> guard(g_mutex);
    auto it = g_models.find(blockId);
    if (it == g_models.end())
        return false;
    if (it->second.base.empty())
        return false;
    outBoxes = &it->second.base;
    return true;
}

bool ModelRegistry::TryGetModelVariant(int blockId, const char* key, const std::vector<ModelBox>*& outBoxes)
{
    if (!key)
        return false;
    std::lock_guard<std::mutex> guard(g_mutex);
    auto it = g_models.find(blockId);
    if (it == g_models.end())
        return false;
    auto vit = it->second.variants.find(std::string(key));
    if (vit == it->second.variants.end())
        return false;
    outBoxes = &vit->second;
    return true;
}
