# Building

## Prerequisites

- Visual Studio 2022+ with C++ Desktop Development and .NET 8 workloads
- CMake 3.24+
- .NET 8 SDK

## Build steps

Build everything from the repo root:

```bash
dotnet build WeaveLoader.sln -c Debug
```

Outputs go to `build/`.

## Notes

- The launcher builds the native runtime automatically.
- If you only need the runtime DLL, you can build it directly:

```bash
cd WeaveLoaderRuntime
cmake -B build -A x64
cmake --build build --config Release
```
