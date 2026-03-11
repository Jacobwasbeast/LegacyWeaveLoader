# Modding Guide

## Create a mod

Create a .NET 8 class library:

```bash
dotnet new classlib -n MyMod --framework net8.0
```

Reference the API:

```xml
<ItemGroup>
  <ProjectReference Include="..\\WeaveLoader.API\\WeaveLoader.API.csproj" />
</ItemGroup>
```

Send the output to the mods folder:

```xml
<PropertyGroup>
  <OutputPath>..\\build\\mods\\$(AssemblyName)</OutputPath>
  <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
</PropertyGroup>
```

## Minimal mod

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
        var oreBlock = Registry.Block.Register("mymod:example_ore",
            new BlockProperties()
                .Material(MaterialType.Stone)
                .Hardness(3.0f)
                .Resistance(15.0f)
                .Sound(SoundType.Stone)
                .Icon("mymod:block/example_ore")
                .InCreativeTab(CreativeTab.BuildingBlocks)
                .Name(Text.Translatable("block.mymod.example_ore")));

        var gem = Registry.Item.Register("mymod:example_gem",
            new ItemProperties()
                .MaxStackSize(64)
                .Icon("mymod:item/example_gem")
                .InCreativeTab(CreativeTab.Materials)
                .Name(Text.Translatable("item.mymod.example_gem")));

        Registry.Recipe.AddFurnace("mymod:example_ore", "mymod:example_gem", 1.0f);

        GameEvents.OnBlockBreak += (_, args) =>
        {
            if (args.BlockId == oreBlock.NumericId)
                Logger.Info("Player broke example ore!");
        };
    }
}
```

## Assets

Textures go under `assets/<namespace>/textures/`:

```
MyMod/
├── assets/
│   └── mymod/
│       └── textures/
│           ├── block/
│           │   └── example_ore.png
│           └── item/
│               └── example_gem.png
├── MyMod.cs
└── MyMod.csproj
```

Models go under `assets/<namespace>/models/`:

```
MyMod/
├── assets/
│   └── mymod/
│       └── models/
│           ├── block/
│           │   └── example_ore.json
│           ├── item/
│           │   └── example_gem.json
│           └── entity/
│               └── example_entity.json
└── ...
```

## Build and run

```bash
dotnet build MyMod.csproj
```

Run `build/WeaveLoader.exe` to launch with your mod.

## Mod lifecycle

| Phase | Method | When | Use for |
|-------|--------|------|---------|
| PreInit | `OnPreInit()` | Before vanilla static constructors | Early configuration |
| Init | `OnInitialize()` | After vanilla registries are set up | Registering blocks, items, recipes, events |
| PostInit | `OnPostInitialize()` | After `Minecraft::init` completes | Cross-mod interactions, late setup |
| Tick | `OnTick()` | Every game tick | Per-frame logic |
| Shutdown | `OnShutdown()` | When the game exits | Cleanup, saving state |

Each phase is wrapped in try/catch so one mod failure does not crash others.

## Localization

Use `Text.Translatable("item.mymod.example_gem")` for localized names or
`Text.Literal("Example Gem")` for fixed text.

Language files live at `assets/<namespace>/lang/<locale>.lang` using `key=value` lines.
By default, WeaveLoader uses the game's selected language.
