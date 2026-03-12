#pragma once

#include <vector>

struct ModelBox
{
    float x0;
    float y0;
    float z0;
    float x1;
    float y1;
    float z1;
};

namespace ModelRegistry
{
    void RegisterBlockModel(int blockId, const ModelBox* boxes, int count);
    void RegisterBlockModelVariant(int blockId, const char* key, const ModelBox* boxes, int count);
    void SetRotationProfile(int blockId, int profile);
    int GetRotationProfile(int blockId);
    bool TryGetModel(int blockId, const std::vector<ModelBox>*& outBoxes);
    bool TryGetModelVariant(int blockId, const char* key, const std::vector<ModelBox>*& outBoxes);
}
