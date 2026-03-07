# Weave Loader

An SKSE-style mod loader for Minecraft Legacy Edition. Weave Loader injects into the game process at runtime, hooks key engine functions, and hosts the .NET runtime to load C# mods. **Zero game source modifications required.**

## How It Works

1. **Weave Loader.exe** launches the game in a suspended state and injects `WeaveLoaderRuntime.dll`
2. The runtime DLL uses PDB debug symbols to locate game functions (`MinecraftWorld_RunStaticCtors`, `Minecraft::tick`, etc.)
3. MinHook detours those functions to insert mod lifecycle callbacks
4. The .NET CoreCLR runtime is hosted inside the game process via hostfxr
5. `WeaveLoader.Core` discovers and loads C# mod assemblies from the `mods/` folder
6. Mods use the `WeaveLoader.API` to register blocks, items, entities, and subscribe to game events using Fabric-style namespaced string IDs

## Project Structure

```
ModLoader/
├── WeaveLoader.Launcher/       C# launcher (the exe users run)
├── WeaveLoaderRuntime/          C++ DLL (injected into game process)
├── WeaveLoader.Core/            C# mod management (loaded inside game)
├── WeaveLoader.API/             C# mod API (what mod authors reference)
└── ExampleMod/                  Sample mod for reference
```

## Building

### Prerequisites

- Visual Studio 2022 or later (with C++ and .NET workloads)
- .NET 8.0 SDK or later
- CMake 3.24 or later
- The game must be compiled with PDB generation

### Build Steps

**C++ Runtime DLL:**

```bash
cd WeaveLoaderRuntime
cmake -B build -A x64
cmake --build build --config Release
```

**C# Projects:**

```bash
dotnet build Weave Loader.sln
```

## Usage

1. Build Weave Loader (see above)
2. Copy the output files to a folder:
   - `Weave Loader.exe`
   - `WeaveLoaderRuntime.dll`
   - `WeaveLoader.Core.dll`
   - `WeaveLoader.API.dll`
3. Create a `mods/` folder and drop mod DLLs in it
4. Run `Weave Loader.exe` -- it will ask for the game exe path on first launch
5. The game starts with mods loaded

## Writing a Mod

Create a new .NET 8 class library and reference `WeaveLoader.API`:

```csharp
using WeaveLoader.API;

[Mod("mymod", Name = "My Mod", Version = "1.0.0", Author = "You")]
public class MyMod : IMod
{
    public void OnInitialize()
    {
        var myBlock = Registry.Block.Register("mymod:cool_block",
            new BlockProperties()
                .Material(MaterialType.Stone)
                .Hardness(2.0f)
                .Resistance(10f));

        Logger.Info("My Mod loaded!");
    }
}
```

Build it, copy the DLL to `mods/`, and launch via Weave Loader.

## License

[MIT](LICENSE)
