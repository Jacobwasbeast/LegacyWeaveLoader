using System.Runtime.InteropServices;

namespace WeaveLoader.Launcher;

/// <summary>
/// Thin wrapper around the Win32 GetOpenFileName API.
/// No WinForms/WPF dependency required.
/// </summary>
internal static class FileDialog
{
    public static string? OpenFileDialog(string title, string filter)
    {
        var ofn = new OpenFileName();
        ofn.lStructSize = Marshal.SizeOf(ofn);
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = new string('\0', 260);
        ofn.nMaxFile = 260;
        ofn.lpstrTitle = title;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileName(ofn))
            return ofn.lpstrFile.TrimEnd('\0');

        return null;
    }

    private const int OFN_PATHMUSTEXIST = 0x00000800;
    private const int OFN_FILEMUSTEXIST = 0x00001000;
    private const int OFN_NOCHANGEDIR   = 0x00000008;

    [DllImport("comdlg32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern bool GetOpenFileName([In, Out] OpenFileName ofn);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private class OpenFileName
    {
        public int lStructSize;
        public IntPtr hwndOwner;
        public IntPtr hInstance;
        public string? lpstrFilter;
        public string? lpstrCustomFilter;
        public int nMaxCustFilter;
        public int nFilterIndex;
        public string? lpstrFile;
        public int nMaxFile;
        public string? lpstrFileTitle;
        public int nMaxFileTitle;
        public string? lpstrInitialDir;
        public string? lpstrTitle;
        public int Flags;
        public short nFileOffset;
        public short nFileExtension;
        public string? lpstrDefExt;
        public IntPtr lCustData;
        public IntPtr lpfnHook;
        public string? lpTemplateName;
        public IntPtr pvReserved;
        public int dwReserved;
        public int FlagsEx;
    }
}
