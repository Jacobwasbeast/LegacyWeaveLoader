using System.Text;

namespace WeaveLoader.API.Native;

internal static class NativeSymbol
{
    internal static nint Find(string fullName)
        => NativeInterop.native_find_symbol(fullName);

    internal static bool Has(string fullName)
        => NativeInterop.native_has_symbol(fullName) != 0;

    internal static string? GetSignatureKey(string fullName)
    {
        var sb = new StringBuilder(64);
        int len = NativeInterop.native_get_signature_key(fullName, sb, sb.Capacity);
        return len > 0 ? sb.ToString() : null;
    }
}
