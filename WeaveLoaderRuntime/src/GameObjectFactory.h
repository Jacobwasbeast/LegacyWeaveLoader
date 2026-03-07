#pragma once
#include <cstdint>

class SymbolResolver;

namespace GameObjectFactory
{
    bool ResolveSymbols(SymbolResolver& resolver);

    // Create a Tile game object at the given tileId, register it in Tile::tiles[],
    // and create a matching TileItem in Item::items[].
    // materialType/soundType map to the C# API enums (see BlockProperties.cs).
    // iconName is the texture atlas key (e.g. "ruby_ore").
    // descriptionId: if >= 0, call setDescriptionId on the Tile for localization.
    bool CreateTile(int tileId, int materialType, float hardness, float resistance,
                    int soundType, const wchar_t* iconName, int descriptionId = -1);

    // Create an Item game object. itemId is the FINAL id (256 + constructor param).
    // The Item is registered in Item::items[itemId].
    // descriptionId: if >= 0, call setDescriptionId on the Item for localization.
    bool CreateItem(int itemId, int maxStackSize, const wchar_t* iconName, int descriptionId = -1);
}
