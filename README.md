# Weave Loader

Weave Loader is a mod loader for Minecraft Legacy Edition (Xbox 360 / PS3 / Windows 64-bit). It launches the game and loads mods from a folder. It does not modify game files.

## What it does

- Launches the game and loads mods
- Lets mods add blocks, items, recipes, and events
- Merges mod textures and models at runtime
- Shows loader info in the main menu
- Writes logs and crash reports

## Quick start

1. Install Visual Studio 2022 (C++ + .NET 8), CMake, and the .NET 8 SDK
2. Build: `dotnet build WeaveLoader.sln -c Debug`
3. Run: `build/WeaveLoader.exe` and select `Minecraft.Client.exe`
4. Drop mod DLLs into `build/mods/`

## Writing a mod (short)

Create a .NET 8 class library, reference `WeaveLoader.API`, and implement `IMod`:

```csharp
using WeaveLoader.API;

[Mod("mymod", Name = "My Mod", Version = "1.0.0", Author = "You")]
public class MyMod : IMod
{
    public void OnInitialize()
    {
        Logger.Info("My Mod loaded!");
    }
}
```

## Logs

- `build/logs/weaveloader.log`
- `build/logs/game_debug.log`
- `build/logs/crash.log`

## Docs

- Building: `docs/BUILDING.md`
- Modding guide: `docs/MODDING.md`
- API reference: `docs/API_REFERENCE.md`
- Mixins: `docs/MIXINS.md`
- Project structure: `docs/PROJECT_STRUCTURE.md`
- Internals: `docs/INTERNALS.md`

## License

MIT
