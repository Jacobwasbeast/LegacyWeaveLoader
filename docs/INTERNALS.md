# Internals

This file covers how Weave Loader works under the hood.

## Symbol resolution

The runtime reads the game's PDB file and resolves function addresses by mangled C++ names. Resolved RVAs are added to the module base to build callable function pointers.

We use [raw_pdb](https://github.com/MolecularMatters/raw_pdb) to parse PDBs without the DIA SDK.

## Texture atlas merging

1. Mod textures are found under `mods/*/assets/<namespace>/textures/`.
2. The vanilla `terrain.png` and `items.png` are loaded.
3. Empty cells are detected and mod textures are placed into them.
4. The merged atlas is written to `mods/ModLoader/generated/`.
5. File open hooks redirect the game to the merged atlases during init.

## Model asset overlay

When the game requests `assets/<namespace>/models/...`, WeaveLoader redirects to the mod's matching file under `mods/*/assets/<namespace>/models/`.

## String table injection

The runtime locates the game's string table and appends mod strings directly. This avoids relying on inlined string lookup calls.

## Crash handler

A vectored exception handler writes crash reports with register dumps, stack traces, and loaded modules. It also skips `__debugbreak()` asserts so the game can continue running.

## ID ranges

| Type | Numeric Range | Notes |
|------|--------------|-------|
| Blocks (Tiles) | 174 -- 255 | 82 slots; maps to TileItem in `Item::items[]` |
| Items | 3000 -- 31999 | Constructor param = `numericId - 256` |
| Entities | 1000 -- 9999 | Reserved for mod entities |
