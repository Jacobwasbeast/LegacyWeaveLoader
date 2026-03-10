#pragma once
#include <Windows.h>
#include "Symbols/SymbolGroups.h"

class SymbolResolver
{
public:
    bool Initialize();
    bool ResolveGameFunctions();
    void Cleanup();

    void* Resolve(const char* decoratedName);
    void* ResolveExact(const char* exactName);
    bool IsStub(void* ptr) const;

    CoreSymbols Core;
    UiSymbols Ui;
    ResourceSymbols Resource;
    TextureSymbols Texture;
    ItemSymbols Item;
    TileSymbols Tile;
    LevelSymbols Level;
    EntitySymbols Entity;
    InventorySymbols Inventory;
    NbtSymbols Nbt;

private:
    uintptr_t m_moduleBase = 0;
    bool m_initialized = false;
};
