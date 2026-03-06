using System.Runtime.InteropServices;

namespace LegacyForge.API;

/// <summary>
/// Internal P/Invoke bindings to LegacyForgeRuntime.dll native exports.
/// Mod authors should use the Registry and Logger classes instead of calling these directly.
/// </summary>
internal static class NativeInterop
{
    private const string RuntimeDll = "LegacyForgeRuntime";

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_block(
        string namespacedId,
        int materialId,
        float hardness,
        float resistance,
        int soundType,
        string iconName,
        float lightEmission,
        int lightBlock);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_item(
        string namespacedId,
        int maxStackSize,
        int maxDamage);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_register_entity(
        string namespacedId,
        float width,
        float height,
        int trackingRange);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_add_shaped_recipe(
        string resultId,
        int resultCount,
        string pattern,
        string ingredientIds);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_add_furnace_recipe(
        string inputId,
        string outputId,
        float xp);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_log(string message, int level);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_get_block_id(string namespacedId);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_get_item_id(string namespacedId);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int native_get_entity_id(string namespacedId);

    [DllImport(RuntimeDll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern void native_subscribe_event(string eventName, IntPtr managedFnPtr);
}
