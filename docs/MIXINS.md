# Mixins

This guide explains how to hook game functions using Weave Loader mixins.

## What is a mixin?

A mixin lets you attach your code to an existing game function. You can run code:

- At the start of a function (head)
- At the end of a function (tail)

## 1. Register a mixin class

Create a file named `weave.mixins.json` in your mod folder:

```json
{
  "package": "ExampleMod.Mixins",
  "mixins": [
    "CreeperExplosionMixin"
  ]
}
```

Fields (currently supported):

- `package`: C# namespace that contains your mixin classes
- `mixins`: Class names under that namespace
- `client`, `server`: Optional lists (loaded the same way as `mixins`)

Fields (parsed but not enforced yet):

- `required`
- `injectors.defaultRequire`

Place the file next to your mod DLL in `build/mods/<YourMod>/`.

## 2. Create a mixin class

### Target class

```csharp
using WeaveLoader.API.Mixins;

[Mixin("Creeper")]
public class CreeperExplosionMixin
{
}
```

The string in `[Mixin("...")]` is the class name used in `mapping.json`.

### Inject at the start (head)

```csharp
using WeaveLoader.API.Mixins;

[Mixin("Creeper")]
public class CreeperExplosionMixin
{
    [Inject("tick", At.Head)]
    private static void OnTick(MixinContext ctx)
    {
        // Code runs before Creeper::tick
    }
}
```

### Inject at the end (tail)

```csharp
using WeaveLoader.API.Mixins;

[Mixin("Creeper")]
public class CreeperExplosionMixin
{
    [Inject("tick", At.Tail)]
    private static void OnTick(MixinContext ctx)
    {
        // Code runs after Creeper::tick
    }
}
```

## 3. Using arguments and return values

Mixin methods must be `static` and return `void`. They can take either:

- No parameters
- One parameter: `MixinContext`

`MixinContext` lets you read or change arguments and return values.

```csharp
using WeaveLoader.API.Mixins;

[Mixin("Item")]
public class ItemMixin
{
    [Inject("use", At.Head)]
    private static void OnUse(MixinContext ctx)
    {
        var arg0 = ctx.GetArg(0);
        var arg1 = ctx.GetArg(1);
        // use ctx.SetArg(index, value) to modify
    }

    [Inject("use", At.Tail)]
    private static void AfterUse(MixinContext ctx)
    {
        var ret = ctx.GetReturn();
        // use ctx.SetReturn(value) to modify
    }
}
```

## 4. Accessing the instance (`this`)

For non-static methods, the instance pointer is always arg 0:

```csharp
var self = ctx.GetArg(0);
```

For static methods, `ctx.ThisPtr` will be `0`.

## 5. Canceling the original method

You can skip the original game method at head injection:

```csharp
using WeaveLoader.API.Mixins;
using WeaveLoader.API.Native;

[Mixin("SomeClass")]
public class ExampleCancel
{
    [Inject("doThing", At.Head)]
    private static void OnDoThing(MixinContext ctx)
    {
        ctx.SetReturn(new NativeRet { Type = NativeType.I32, Value = 0 });
        ctx.Cancel();
    }
}
```

Canceling is only effective at the head. Tail injections run after the original method.

## 6. Picking the right method name

`InjectAttribute` takes either:

- A method name (`"tick"`) which becomes `Class::tick`
- A full name (`"Creeper::tick"`) from `mapping.json`

If you have overloads, use the exact `fullName` from `mapping.json`.

## 7. Confirm it is hooked

When a mixin loads, you should see logs like:

```
Processing mixin: ExampleMod.Mixins.CreeperExplosionMixin -> Creeper
HookRegistry: hooked Creeper::tick
Mixin hook installed: Creeper::tick (Tail)
```

If you do not see these logs:

- Check `weave.mixins.json` is next to the mod DLL
- Check the namespace in `package`
- Check the class name and method name match the PDB mapping
- Check the mod loaded at all

## 8. Common issues

- **Wrong class or method name**: PDB names are case-sensitive.
- **Crashes when accessing args**: Make sure the method signature matches the game method exactly.
- **Mixins not running**: Verify you see the “hook installed” log.

## 9. Logging

Use `Logger.Debug` inside mixins while testing:

```csharp
Logger.Debug("Creeper mixin tick");
```

Remove or reduce logs for release builds to avoid overhead.

## 10. Limits and not supported yet

- Hooks support up to 6 arguments.
- Overwrite mixins are not supported yet.
- Invoker mixins are not supported yet.
