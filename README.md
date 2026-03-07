# Weave Loader

A runtime mod loader for Minecraft Legacy Edition (Xbox 360 / PS3 / Windows 64-bit port). Weave Loader injects into the game process, hooks engine functions via PDB symbol resolution, and hosts the .NET runtime so mods can be written in C#. **Zero game source modifications required.**

## Features

- **DLL injection** -- Launcher starts the game suspended, injects the runtime DLL, then resumes
- **PDB symbol resolution** -- Uses raw PDB parsing (no DIA dependency) to locate game functions by their mangled names at runtime
- **Function hooking** -- MinHook detours on game lifecycle functions (init, tick, static constructors, rendering)
- **Full .NET hosting** -- .NET 8 CoreCLR is loaded inside the game process via hostfxr; mods are standard C# class libraries
- **Block and item registration** -- Create real game objects (Tile, TileItem, Item) by calling the game's own constructors through resolved PDB symbols
- **Dynamic texture atlas merging** -- Mod textures are stitched into the game's `terrain.png` and `items.png` atlases at runtime using empty cells
- **Creative inventory injection** -- Mod items appear in the correct creative tabs with proper pagination
- **Localized display names** -- Mod strings are injected directly into the game's `StringTable` vector, bypassing inlined `GetString` calls
- **Crash reporting** -- Vectored exception handler produces detailed crash logs with register dumps, symbolicated stack traces, and loaded module lists
- **Main menu branding** -- Renders loader version and mod count on the main menu via the game's own font renderer
- **Furnace recipes** -- Register smelting recipes with input, output, and XP values
- **Event system** -- Subscribe to block break, block place, chat, entity spawn, and player join events

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                     WeaveLoader.Launcher                     │
│              Starts game, injects runtime DLL                │
└────────────────────────────┬─────────────────────────────────┘
                             │ CreateRemoteThread
┌────────────────────────────▼─────────────────────────────────┐
│                    WeaveLoaderRuntime.dll                     │
│  C++ runtime injected into game process                      │
│                                                              │
│  ┌─────────────┐ ┌──────────────┐ ┌───────────────────────┐ │
│  │ PDB Parser  │ │  Hook Mgr    │ │  .NET Host (hostfxr)  │ │
│  │ (raw_pdb)   │ │  (MinHook)   │ │                       │ │
│  └──────┬──────┘ └──────┬───────┘ └───────────┬───────────┘ │
│         │               │                     │             │
│  ┌──────▼──────┐ ┌──────▼───────┐ ┌───────────▼───────────┐ │
│  │ Symbol      │ │ Game Hooks   │ │ WeaveLoader.Core.dll   │ │
│  │ Resolver    │ │ (lifecycle,  │ │ (mod discovery,        │ │
│  │             │ │  textures,   │ │  lifecycle mgmt)       │ │
│  │             │ │  UI, strings)│ │                        │ │
│  └─────────────┘ └─────────────┘ └───────────┬───────────┘ │
│                                               │             │
│  ┌────────────────────────────────────────────▼───────────┐ │
│  │ Native Exports (C ABI)                                 │ │
│  │ register_block, register_item, add_furnace_recipe, ... │ │
│  └────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
                             │ P/Invoke
┌────────────────────────────▼─────────────────────────────────┐
│                      WeaveLoader.API                         │
│        Public C# API that mod authors reference              │
│                                                              │
│  Registry.Block  ·  Registry.Item  ·  Registry.Recipe        │
│  Registry.Entity ·  Registry.Assets · GameEvents             │
│  Logger · CreativeTab · Identifier · [Mod] attribute         │
└──────────────────────────────────────────────────────────────┘
                             │ implements IMod
┌────────────────────────────▼─────────────────────────────────┐
│                        Mod DLLs                              │
│               ExampleMod, user mods, etc.                    │
└──────────────────────────────────────────────────────────────┘
```

## Project Structure

```
ModLoader/
├── WeaveLoader.Launcher/      # C# launcher executable
├── WeaveLoaderRuntime/         # C++ DLL injected into the game
│   └── src/
│       ├── dllmain.cpp         # Entry point, init thread
│       ├── PdbParser.cpp       # Raw PDB symbol parsing (no DIA)
│       ├── SymbolResolver.cpp  # Resolves game functions by mangled name
│       ├── HookManager.cpp     # MinHook-based function detouring
│       ├── GameHooks.cpp       # Hook implementations (lifecycle, UI)
│       ├── GameObjectFactory.cpp # Creates Tile/Item objects via resolved ctors
│       ├── CreativeInventory.cpp # Injects mod items into creative tabs
│       ├── ModAtlas.cpp        # Texture atlas merging (terrain.png, items.png)
│       ├── ModStrings.cpp      # String table injection for item names
│       ├── CrashHandler.cpp    # Vectored exception handler + crash logs
│       ├── MainMenuOverlay.cpp # Renders branding text on main menu
│       ├── DotNetHost.cpp      # Hosts .NET CoreCLR via hostfxr
│       ├── NativeExports.cpp   # C exports called by C# via P/Invoke
│       ├── IdRegistry.cpp      # Namespaced ID <-> numeric ID mapping
│       └── LogUtil.cpp         # Timestamped logging to files
├── WeaveLoader.Core/           # C# mod discovery and lifecycle
│   ├── ModDiscovery.cs         # Scans mods/ for IMod implementations
│   ├── ModManager.cs           # Calls lifecycle hooks with error isolation
│   └── WeaveLoaderCore.cs      # Entry points called from C++ runtime
├── WeaveLoader.API/            # C# public API for mod authors
│   ├── IMod.cs                 # Mod interface with lifecycle hooks
│   ├── ModAttribute.cs         # [Mod("id", Name, Version, Author)]
│   ├── Registry.cs             # Static facade for all registries
│   ├── Block/                  # BlockRegistry, BlockProperties, MaterialType
│   ├── Item/                   # ItemRegistry, ItemProperties
│   ├── Recipe/                 # RecipeRegistry (shaped, furnace)
│   ├── Entity/                 # EntityRegistry, EntityDefinition
│   ├── Assets/                 # AssetRegistry (string table access)
│   ├── Events/                 # GameEvents (block break/place, chat, etc.)
│   ├── Logger.cs               # Debug/Info/Warning/Error logging
│   ├── CreativeTab.cs          # Creative inventory tab enum
│   └── Identifier.cs           # Namespaced ID parsing ("namespace:path")
├── ExampleMod/                 # Sample mod demonstrating the API
│   ├── ExampleMod.cs
│   └── assets/
│       ├── blocks/ruby_ore.png
│       └── items/ruby.png
├── build/                      # Shared build output
│   ├── mods/                   # Mod DLLs and assets go here
│   └── logs/                   # weaveloader.log, game_debug.log, crash.log
├── WeaveLoader.sln
├── README.md
└── CONTRIBUTING.md
```

## Building

### Prerequisites

- Visual Studio 2022+ with **C++ Desktop Development** and **.NET 8** workloads
- CMake 3.24+
- .NET 8.0 SDK

### Build Steps

Build the C++ runtime DLL:

```bash
cd WeaveLoaderRuntime
cmake -B build -A x64
cmake --build build --config Release
```

Build all C# projects (launcher, core, API, example mod):

```bash
dotnet build WeaveLoader.sln -c Debug
```

All outputs land in `build/`.

## Usage

1. Build Weave Loader (see above)
2. Run `WeaveLoader.exe` -- it prompts for the game executable path on first launch (saved to `weaveloader.json`)
3. The launcher starts the game suspended, injects `WeaveLoaderRuntime.dll`, and resumes
4. Mods are loaded from `build/mods/` automatically

### Log Files

All logs are written to `build/logs/`:

| File | Contents |
|------|----------|
| `weaveloader.log` | Loader initialization, symbol resolution, hook installation, mod lifecycle |
| `game_debug.log` | Game's own `OutputDebugString` output (captured via hook) |
| `crash.log` | Detailed crash reports with symbolicated stack traces |

## Writing a Mod

### 1. Create a .NET 8 class library

```bash
dotnet new classlib -n MyMod --framework net8.0
```

### 2. Reference the API

Add a project reference to `WeaveLoader.API`:

```xml
<ItemGroup>
  <ProjectReference Include="..\WeaveLoader.API\WeaveLoader.API.csproj" />
</ItemGroup>
```

Set the output to the mods folder:

```xml
<PropertyGroup>
  <OutputPath>..\build\mods\$(AssemblyName)</OutputPath>
  <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
</PropertyGroup>
```

### 3. Implement IMod

```csharp
using WeaveLoader.API;
using WeaveLoader.API.Block;
using WeaveLoader.API.Item;
using WeaveLoader.API.Recipe;
using WeaveLoader.API.Events;

[Mod("mymod", Name = "My Mod", Version = "1.0.0", Author = "You")]
public class MyMod : IMod
{
    public void OnInitialize()
    {
        // Register a block
        var oreBlock = Registry.Block.Register("mymod:example_ore",
            new BlockProperties()
                .Material(MaterialType.Stone)
                .Hardness(3.0f)
                .Resistance(15.0f)
                .Sound(SoundType.Stone)
                .Icon("mymod:example_ore")
                .InCreativeTab(CreativeTab.BuildingBlocks)
                .Name("Example Ore"));

        // Register an item
        var gem = Registry.Item.Register("mymod:example_gem",
            new ItemProperties()
                .MaxStackSize(64)
                .Icon("mymod:example_gem")
                .InCreativeTab(CreativeTab.Materials)
                .Name("Example Gem"));

        // Add a smelting recipe
        Registry.Recipe.AddFurnace("mymod:example_ore", "mymod:example_gem", 1.0f);

        // Subscribe to events
        GameEvents.OnBlockBreak += (sender, args) =>
        {
            if (args.BlockId == oreBlock.NumericId)
                Logger.Info("Player broke example ore!");
        };

        Logger.Info("My Mod loaded!");
    }
}
```

### 4. Add textures

Place 16x16 PNG textures in your mod's assets folder:

```
MyMod/
├── assets/
│   ├── blocks/
│   │   └── example_ore.png     # Block texture
│   └── items/
│       └── example_gem.png     # Item texture
├── MyMod.cs
└── MyMod.csproj
```

The icon name in `BlockProperties.Icon()` / `ItemProperties.Icon()` uses the format `"modid:texture_name"` where `texture_name` matches the PNG filename (without extension).

### 5. Build and run

```bash
dotnet build MyMod.csproj
# Output goes to build/mods/MyMod/
# Run WeaveLoader.exe to launch with mods
```

## Mod Lifecycle

Mods go through these phases in order:

| Phase | Method | When | Use for |
|-------|--------|------|---------|
| PreInit | `OnPreInit()` | Before vanilla static constructors | Early configuration |
| Init | `OnInitialize()` | After vanilla registries are set up | Registering blocks, items, recipes, events |
| PostInit | `OnPostInitialize()` | After `Minecraft::init` completes | Cross-mod interactions, late setup |
| Tick | `OnTick()` | Every game tick (~20 Hz) | Per-frame logic |
| Shutdown | `OnShutdown()` | When the game exits | Cleanup, saving state |

Each mod's lifecycle methods are wrapped in try/catch, so one mod crashing won't take down others.

## API Reference

### Registry.Block

```csharp
RegisteredBlock Register(string id, BlockProperties properties)
```

Creates a real `Tile` game object with the specified material, hardness, resistance, and sound type. Automatically creates a corresponding `TileItem` so the block appears in inventory.

### Registry.Item

```csharp
RegisteredItem Register(string id, ItemProperties properties)
```

Creates a real `Item` game object. The constructor parameter is derived from the numeric ID to match the game's internal convention (`Item::Item(numericId - 256)`).

### Registry.Recipe

```csharp
void AddFurnace(string inputId, string outputId, float xp)
```

Registers a furnace smelting recipe.

### GameEvents

```csharp
event EventHandler<BlockBreakEventArgs> OnBlockBreak;
event EventHandler<BlockPlaceEventArgs> OnBlockPlace;
event EventHandler<ChatEventArgs> OnChat;
event EventHandler<EntitySpawnEventArgs> OnEntitySpawn;
event EventHandler<PlayerJoinEventArgs> OnPlayerJoin;
```

### Logger

```csharp
Logger.Debug(string message)
Logger.Info(string message)
Logger.Warning(string message)
Logger.Error(string message)
```

All log output goes to `logs/weaveloader.log` with timestamps and log level prefixes.

### Identifier

Namespaced string IDs follow the `"namespace:path"` convention (e.g., `"mymod:ruby_ore"`). If no namespace is provided, `"minecraft"` is assumed.

## ID Ranges

| Type | Numeric Range | Notes |
|------|--------------|-------|
| Blocks (Tiles) | 174 -- 255 | 82 slots; maps to TileItem in `Item::items[]` |
| Items | 3000 -- 31999 | Constructor param = `numericId - 256` |
| Entities | 1000 -- 9999 | Reserved for mod entities |

## How It Works Internally

### Symbol Resolution

The runtime opens the game's PDB file and parses it using [raw_pdb](https://github.com/MolecularMatters/raw_pdb) (no dependency on Microsoft's DIA SDK). It searches public, global, and module symbol streams for decorated C++ names like `??0Tile@@IEAA@HPEAVMaterial@@_N@Z` (Tile's protected constructor). The resolved RVAs are added to the module base address to get callable function pointers.

### Texture Atlas Merging

1. Mod textures are discovered in `mods/*/assets/blocks/` and `mods/*/assets/items/`
2. The vanilla `terrain.png` (16x32 grid) and `items.png` (16x16 grid) are loaded via stb_image
3. Empty cells are identified by checking for fully transparent pixels
4. Mod textures are placed into empty cells
5. The merged atlas is written to a temp directory and swapped in before `Minecraft::init`
6. After the game loads textures, the original files are restored from backup
7. `SimpleIcon` objects are created for each mod texture with correct UV coordinates

### String Table Injection

The game's `CMinecraftApp::GetString(int)` is a thin wrapper around `StringTable::getString(int)`, which does a vector index lookup. Since MSVC's link-time optimization inlines `GetString` at call sites like `Item::getHoverName`, a MinHook detour alone isn't sufficient. The runtime parses the x64 machine code of `GetString` to locate the RIP-relative reference to `app.m_stringTable`, then directly resizes the string table's internal vector and writes mod strings at the allocated indices.

### Crash Handler

A vectored exception handler catches access violations and other fatal exceptions. It uses the PDB address index (built during initialization) to resolve crash addresses to symbol names, producing crash reports with full register dumps, symbolicated stack traces, and loaded module lists. The handler also intercepts `int 3` breakpoints in game code (used by `__debugbreak()` for assertions) and skips them so execution continues.

## License

[MIT](LICENSE)
