# Contributing to LegacyForge

Thank you for your interest in contributing to LegacyForge!

## Project Structure

- **LegacyForge.Launcher/** -- C# console app that launches the game and injects the runtime DLL
- **LegacyForgeRuntime/** -- C++ DLL injected into the game process (hooks, .NET hosting, native exports)
- **LegacyForge.Core/** -- C# assembly loaded inside the game process (mod discovery, lifecycle management)
- **LegacyForge.API/** -- C# class library that mod authors reference (IMod, Registry, Events)
- **ExampleMod/** -- Sample mod demonstrating the API

## Building

### Prerequisites

- Visual Studio 2022+ with C++ Desktop Development and .NET 8 workloads
- CMake 3.24+
- .NET 8.0 SDK

### C++ Runtime

```bash
cd LegacyForgeRuntime
cmake -B build -A x64
cmake --build build --config Release
```

### C# Projects

```bash
dotnet build LegacyForge.sln -c Release
```

## Guidelines

- Keep game source modifications at zero -- all integration is via injection and hooking
- Test hooks against the latest game build with PDB symbols
- Use namespaced string IDs (`"modid:name"`) for all registered content
- Catch exceptions from mod code to prevent crashes
- Document public API methods with XML doc comments

## Pull Requests

- Fork the repository and create a feature branch
- Include a clear description of what your PR does
- Make sure the C# projects build without warnings
- Test DLL injection on a real game build if possible
