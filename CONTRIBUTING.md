# Contributing to Weave Loader

Thanks for your interest in contributing. This guide covers how the project is organized, how to build it, and what conventions to follow.

## Project Overview

Weave Loader is split into four main components plus an example mod:

| Component | Language | Purpose |
|-----------|----------|---------|
| **WeaveLoader.Launcher** | C# | Console app that launches the game suspended and injects the runtime DLL |
| **WeaveLoaderRuntime** | C++ | DLL injected into the game process; handles PDB parsing, hooking, .NET hosting, and all native game interaction |
| **WeaveLoader.Core** | C# | Loaded inside the game process by the runtime; discovers mods, manages lifecycle |
| **WeaveLoader.API** | C# | Public API that mod authors reference; contains registries, events, and the `IMod` interface |
| **ExampleMod** | C# | Working sample mod that registers a block, item, recipe, and event handler |

### Communication Flow

```
Launcher  ──(inject)──>  Runtime (C++)  ──(hostfxr)──>  Core (C#)  ──(reflection)──>  Mods
                              ▲                                                          │
                              └──────────── P/Invoke (NativeExports) ─────────────────────┘
```

Mods call `Registry.Block.Register(...)` in C#, which calls `NativeInterop.native_register_block(...)` via P/Invoke, which arrives in `NativeExports.cpp` in the C++ runtime, which calls `GameObjectFactory::CreateTile(...)` to construct a real game `Tile` object through resolved PDB symbols.

## Building

### Prerequisites

- Visual Studio 2022+ with **C++ Desktop Development** and **.NET 8** workloads
- CMake 3.24+
- .NET 8.0 SDK
- The target game must have been compiled with PDB generation (`/Zi` or `/ZI`)

### C++ Runtime

```bash
cd WeaveLoaderRuntime
cmake -B build -A x64
cmake --build build --config Release
```

The output is `WeaveLoaderRuntime.dll` in `build/Release/`.

### C# Projects

```bash
dotnet build WeaveLoader.sln -c Debug
```

All outputs go to `build/`.

### Full Clean Build

```bash
dotnet clean WeaveLoader.sln
cd WeaveLoaderRuntime && cmake --build build --target clean && cd ..
dotnet build WeaveLoader.sln -c Debug
cd WeaveLoaderRuntime && cmake --build build --config Release
```

## Development Guidelines

### Core Principles

1. **No game source modifications.** All interaction with the game is through DLL injection, PDB symbol resolution, and MinHook detours. Never suggest changes to the game's own source code.

2. **Fix root causes, not symptoms.** If something crashes, understand why at the engine level. Don't add null checks around broken pointers -- resolve the actual construction or lifecycle issue.

3. **Use PDB symbols, not hardcoded offsets.** Game updates change binary layouts. All function pointers must be resolved from the PDB at runtime using their decorated (mangled) names.

### C++ Runtime (`WeaveLoaderRuntime/`)

- **Symbol resolution**: Add new game functions in `SymbolResolver.cpp`. Use `llvm-pdbutil dump --publics` or the built-in `PdbParser::DumpMatching()` to find the correct mangled name. MSVC mangles are sensitive to calling convention, const qualifiers, and parameter types -- double-check them.

- **Hooking**: New hooks go in `GameHooks.cpp` (implementation) and `HookManager.cpp` (installation). Always store the original function pointer so the hook can call through to the original.

- **Native exports**: Functions called by C# are declared in `NativeExports.cpp` with `extern "C" __declspec(dllexport)`. Keep signatures simple (primitive types, `const char*`). Add corresponding `[DllImport]` entries in `WeaveLoader.API/NativeInterop.cs`.

- **Logging**: Use `LogUtil::Log(message)` for normal output (goes to `weaveloader.log`). Never use `printf` or `std::cout`.

- **Memory safety**: The game's memory layout is discovered at runtime. Always validate pointers before dereferencing. Add diagnostic logging when traversing game structures.

### C# API (`WeaveLoader.API/`)

- **Backward compatibility**: The API is what mod authors compile against. Avoid breaking changes to public interfaces. New features should be additive.

- **Thread safety**: `OnTick()` runs on the game thread. Registration methods are called during init. The API's internal P/Invoke calls are not thread-safe by design.

- **Naming**: Use `PascalCase` for public members and `camelCase` for parameters. Registry methods follow the pattern `Registry.<Type>.Register(id, properties)`.

- **Identifiers**: All content IDs use the `"namespace:path"` format. The `Identifier` class handles parsing. Default namespace is `"minecraft"`.

### C# Core (`WeaveLoader.Core/`)

- **Mod isolation**: Each mod's lifecycle method is wrapped in try/catch in `ModManager.cs`. A failing mod should never crash the loader or other mods.

- **Discovery**: Mods are found by scanning `mods/*/` for assemblies containing types that implement `IMod` and have the `[Mod]` attribute.

### Testing

- Always test DLL injection on a real game build with PDB symbols available.
- Verify that new hooks don't break existing functionality (creative inventory, item names, textures).
- Check `logs/crash.log` after crashes -- the symbolicated stack trace usually points directly to the issue.
- Test with multiple mods loaded to ensure isolation works.

## Adding a New Registry Type

To add support for a new game object type (e.g., particles):

1. **PDB**: Find the constructor and relevant setters using `PdbParser::DumpMatching("ParticleType")` or `llvm-pdbutil`
2. **SymbolResolver**: Add the mangled symbol constants and resolution in `SymbolResolver.cpp`
3. **GameObjectFactory**: Add a `CreateParticle()` function that allocates memory and calls the resolved constructor
4. **NativeExports**: Add `native_register_particle()` as a C export
5. **NativeInterop.cs**: Add the `[DllImport]` declaration
6. **API**: Create `ParticleRegistry.cs`, `ParticleProperties.cs` in `WeaveLoader.API/Particle/`
7. **Registry.cs**: Add `public static ParticleRegistry Particle { get; }` to the facade
8. **ExampleMod**: Add a usage example

## Adding a New Game Hook

1. Find the target function's mangled name in the PDB
2. Add the symbol constant and resolution in `SymbolResolver.cpp`
3. Define the function pointer typedef and `Original_*` variable in `GameHooks.h`
4. Implement `Hooked_*` in `GameHooks.cpp`, calling `Original_*` at the appropriate point
5. Add `MH_CreateHook` / `MH_EnableHook` calls in `HookManager.cpp`

## Commit Messages

- Use present tense: "Add particle registry" not "Added particle registry"
- Be specific: "Fix creative tab pagination after mod item injection" not "Fix bug"
- Prefix with area when helpful: "runtime: resolve Tile::setLightLevel from PDB"

## Pull Requests

- Fork the repository and create a feature branch from `main`
- Include a clear description of what the PR does and why
- Ensure both C++ and C# projects build without errors
- Test DLL injection on a real game build if possible
- Keep PRs focused -- one feature or fix per PR
