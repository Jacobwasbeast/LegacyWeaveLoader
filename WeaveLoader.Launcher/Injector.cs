using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace WeaveLoader.Launcher;

/// <summary>
/// Handles launching the game process in a suspended state and injecting
/// the WeaveLoaderRuntime DLL via CreateRemoteThread + LoadLibraryW.
/// </summary>
public static class Injector
{
    public record InjectedProcess(
        IntPtr ProcessHandle,
        IntPtr ThreadHandle,
        int ProcessId);

    public static InjectedProcess LaunchSuspended(string exePath, string? workingDir = null)
    {
        workingDir ??= Path.GetDirectoryName(exePath);

        var si = new STARTUPINFO { cb = Marshal.SizeOf<STARTUPINFO>() };

        bool success = CreateProcess(
            exePath,
            null,
            IntPtr.Zero,
            IntPtr.Zero,
            false,
            CREATE_SUSPENDED,
            IntPtr.Zero,
            workingDir,
            ref si,
            out PROCESS_INFORMATION pi);

        if (!success)
            throw new Win32Exception(Marshal.GetLastWin32Error(),
                $"Failed to launch process: {exePath}");

        return new InjectedProcess(pi.hProcess, pi.hThread, pi.dwProcessId);
    }

    public static void InjectDll(InjectedProcess process, string dllPath)
    {
        string fullDllPath = Path.GetFullPath(dllPath);
        if (!File.Exists(fullDllPath))
            throw new FileNotFoundException($"Runtime DLL not found: {fullDllPath}");

        byte[] dllPathBytes = Encoding.Unicode.GetBytes(fullDllPath + '\0');

        IntPtr remoteMem = VirtualAllocEx(
            process.ProcessHandle,
            IntPtr.Zero,
            (uint)dllPathBytes.Length,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE);

        if (remoteMem == IntPtr.Zero)
        {
            TerminateProcess(process.ProcessHandle, 1);
            throw new Win32Exception(Marshal.GetLastWin32Error(),
                "Failed to allocate memory in target process");
        }

        bool wrote = WriteProcessMemory(
            process.ProcessHandle,
            remoteMem,
            dllPathBytes,
            (uint)dllPathBytes.Length,
            out _);

        if (!wrote)
        {
            TerminateProcess(process.ProcessHandle, 1);
            throw new Win32Exception(Marshal.GetLastWin32Error(),
                "Failed to write DLL path to target process");
        }

        IntPtr kernel32 = GetModuleHandle("kernel32.dll");
        IntPtr loadLibraryW = GetProcAddress(kernel32, "LoadLibraryW");

        IntPtr remoteThread = CreateRemoteThread(
            process.ProcessHandle,
            IntPtr.Zero,
            0,
            loadLibraryW,
            remoteMem,
            0,
            out _);

        if (remoteThread == IntPtr.Zero)
        {
            TerminateProcess(process.ProcessHandle, 1);
            throw new Win32Exception(Marshal.GetLastWin32Error(),
                "Failed to create remote thread for DLL injection");
        }

        uint waitResult = WaitForSingleObject(remoteThread, 10000);
        if (waitResult != 0)
        {
            CloseHandle(remoteThread);
            VirtualFreeEx(process.ProcessHandle, remoteMem, 0, MEM_RELEASE);
            TerminateProcess(process.ProcessHandle, 1);
            throw new Exception(
                $"Timed out waiting for DLL injection (WaitForSingleObject returned {waitResult})");
        }

        GetExitCodeThread(remoteThread, out uint exitCode);
        CloseHandle(remoteThread);
        VirtualFreeEx(process.ProcessHandle, remoteMem, 0, MEM_RELEASE);

        if (exitCode == 0)
        {
            TerminateProcess(process.ProcessHandle, 1);
            throw new Exception(
                "DLL injection failed: LoadLibraryW returned NULL.\n" +
                "This usually means the DLL or one of its dependencies could not be found.\n" +
                "Make sure the MSVC redistributable is installed and the DLL was built in Release mode.");
        }

        Console.WriteLine($"  LoadLibraryW returned module handle 0x{exitCode:X} -- DLL loaded in target process.");
    }

    public static void ResumeProcess(InjectedProcess process)
    {
        ResumeThread(process.ThreadHandle);
        CloseHandle(process.ThreadHandle);
    }

    #region Win32 Interop

    private const uint CREATE_SUSPENDED = 0x00000004;
    private const uint MEM_COMMIT = 0x1000;
    private const uint MEM_RESERVE = 0x2000;
    private const uint MEM_RELEASE = 0x8000;
    private const uint PAGE_READWRITE = 0x04;

    [StructLayout(LayoutKind.Sequential)]
    private struct STARTUPINFO
    {
        public int cb;
        public string lpReserved, lpDesktop, lpTitle;
        public int dwX, dwY, dwXSize, dwYSize;
        public int dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags;
        public short wShowWindow, cbReserved2;
        public IntPtr lpReserved2, hStdInput, hStdOutput, hStdError;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct PROCESS_INFORMATION
    {
        public IntPtr hProcess, hThread;
        public int dwProcessId, dwThreadId;
    }

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern bool CreateProcess(
        string lpApplicationName, string? lpCommandLine,
        IntPtr lpProcessAttributes, IntPtr lpThreadAttributes,
        bool bInheritHandles, uint dwCreationFlags,
        IntPtr lpEnvironment, string? lpCurrentDirectory,
        ref STARTUPINFO lpStartupInfo, out PROCESS_INFORMATION lpProcessInformation);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr VirtualAllocEx(
        IntPtr hProcess, IntPtr lpAddress, uint dwSize,
        uint flAllocationType, uint flProtect);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool VirtualFreeEx(
        IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint dwFreeType);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool WriteProcessMemory(
        IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer,
        uint nSize, out int lpNumberOfBytesWritten);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr CreateRemoteThread(
        IntPtr hProcess, IntPtr lpThreadAttributes, uint dwStackSize,
        IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags,
        out uint lpThreadId);

    [DllImport("kernel32.dll", CharSet = CharSet.Ansi)]
    private static extern IntPtr GetModuleHandle(string lpModuleName);

    [DllImport("kernel32.dll", CharSet = CharSet.Ansi)]
    private static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

    [DllImport("kernel32.dll")]
    private static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

    [DllImport("kernel32.dll")]
    private static extern uint ResumeThread(IntPtr hThread);

    [DllImport("kernel32.dll")]
    private static extern bool CloseHandle(IntPtr hObject);

    [DllImport("kernel32.dll")]
    private static extern bool TerminateProcess(IntPtr hProcess, uint uExitCode);

    [DllImport("kernel32.dll")]
    private static extern bool GetExitCodeThread(IntPtr hThread, out uint lpExitCode);

    #endregion
}
