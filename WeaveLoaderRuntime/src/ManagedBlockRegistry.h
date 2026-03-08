#pragma once

namespace ManagedBlockRegistry
{
    struct Definition
    {
        int dropBlockId = -1;
        int cloneBlockId = -1;
    };

    void Register(int blockId);
    void Configure(int blockId, int dropBlockId, int cloneBlockId);
    const Definition* Find(int blockId);
    bool IsManaged(int blockId);
}
