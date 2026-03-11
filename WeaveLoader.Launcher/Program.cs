using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.Json;

namespace WeaveLoader.Launcher;

class Program
{
    private const string RuntimeDllName = "WeaveLoaderRuntime.dll";
    private const string MetadataFileName = "metadata.json";

    [STAThread]
    static int Main(string[] args)
    {
        Console.WriteLine("╔══════════════════════════════════╗");
        Console.WriteLine("║         WeaveLoader v1.0        ║");
        Console.WriteLine("║  Mod Loader for MC Legacy Edition║");
        Console.WriteLine("╚══════════════════════════════════╝");
        Console.WriteLine();

        // All paths relative to where the exe lives, not the working directory
        string baseDir = AppContext.BaseDirectory;
        string configFile = Path.Combine(baseDir, "weaveloader.json");
        string runtimeDll = Path.Combine(baseDir, RuntimeDllName);
        string modsDir = Path.Combine(baseDir, "mods");
        string metadataDir = Path.Combine(baseDir, "metadata");
        string mappingPath = Path.Combine(metadataDir, "mapping.json");
        string pdbDumpExe = Path.Combine(baseDir, "pdbdump.exe");

        try
        {
            var config = Config.Load(configFile);
            bool extensiveSymbolScan = false;

            foreach (string arg in args)
            {
                if (arg == "--extensive-symbol-scan")
                {
                    extensiveSymbolScan = true;
                    continue;
                }

                if (File.Exists(arg))
                {
                    config.GameExePath = arg;
                    config.Save(configFile);
                }
            }

            if (string.IsNullOrEmpty(config.GameExePath) || !File.Exists(config.GameExePath))
            {
                Console.WriteLine("Please select Minecraft.Client.exe...");
                string? selected = FileDialog.OpenFileDialog(
                    "Select Minecraft.Client.exe",
                    "Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0");

                if (string.IsNullOrEmpty(selected) || !File.Exists(selected))
                {
                    Console.Error.WriteLine("Error: No file selected or file not found.");
                    return 1;
                }

                config.GameExePath = Path.GetFullPath(selected);
                config.Save(configFile);
                Console.WriteLine($"Saved game path to {configFile}");
            }

            string metadataPath = Path.Combine(metadataDir, MetadataFileName);
            string offsetsPath = Path.Combine(metadataDir, "offsets.json");

            if (File.Exists(metadataPath))
            {
                if (TryReadMetadataSha(metadataPath, out string expectedSha) &&
                    TryGetFileSha256(config.GameExePath, out string actualSha) &&
                    !string.Equals(expectedSha, actualSha, StringComparison.OrdinalIgnoreCase))
                {
                    Console.WriteLine("[WARN] metadata.json does not match game executable. Removing stale metadata.");
                    SafeDelete(metadataPath);
                    SafeDelete(mappingPath);
                    SafeDelete(offsetsPath);
                }
            }

            bool mappingMissing = !File.Exists(mappingPath);
            bool offsetsMissing = !File.Exists(offsetsPath);

            if (mappingMissing || offsetsMissing)
            {
                if (!Directory.Exists(metadataDir))
                    Directory.CreateDirectory(metadataDir);

                string pdbPath = Path.ChangeExtension(config.GameExePath, ".pdb") ?? "";
                if (!File.Exists(pdbDumpExe))
                {
                    Console.WriteLine($"[WARN] pdbdump.exe not found at {pdbDumpExe} (mapping.json will not be generated)");
                }
                else if (!File.Exists(pdbPath))
                {
                    Console.WriteLine($"[WARN] PDB not found at {pdbPath} (mapping.json will not be generated)");
                }
                else
                {
                    Console.WriteLine("[..] Generating metadata from PDB...");
                    var psi = new ProcessStartInfo
                    {
                        FileName = pdbDumpExe,
                        Arguments = $"\"{pdbPath}\" \"{mappingPath}\" --offsets \"{config.GameExePath}\" \"{offsetsPath}\" --all-types",
                        UseShellExecute = false,
                        CreateNoWindow = true
                    };
                    using var proc = Process.Start(psi);
                    proc?.WaitForExit();
                    if (proc == null || proc.ExitCode != 0 || !File.Exists(mappingPath))
                        Console.WriteLine("[WARN] mapping.json generation failed");
                    else
                        Console.WriteLine("[OK] mapping.json generated");
                }
            }

            if (!File.Exists(runtimeDll))
            {
                Console.Error.WriteLine($"Error: {RuntimeDllName} not found.");
                Console.Error.WriteLine($"Expected at: {runtimeDll}");
                Console.Error.WriteLine();
                Console.Error.WriteLine("The C++ runtime DLL must be built separately with CMake:");
                Console.Error.WriteLine("  cd WeaveLoaderRuntime");
                Console.Error.WriteLine("  cmake -B build -A x64");
                Console.Error.WriteLine("  cmake --build build --config Release");
                Console.Error.WriteLine();
                Console.Error.WriteLine("Then copy WeaveLoaderRuntime.dll to the same folder as WeaveLoader.exe.");
                return 1;
            }

            if (!Directory.Exists(modsDir))
            {
                Directory.CreateDirectory(modsDir);
                Console.WriteLine($"Created mods/ directory");
            }

            int modCount = Directory.GetFiles(modsDir, "*.dll").Length;
            Console.WriteLine($"Found {modCount} mod(s) in mods/");
            Console.WriteLine($"Launching {Path.GetFileName(config.GameExePath)}...");
            if (extensiveSymbolScan)
                Console.WriteLine("[..] Extensive symbol scan mode enabled");

            var process = Injector.LaunchSuspended(
                config.GameExePath,
                extraArgs: extensiveSymbolScan ? "--extensive-symbol-scan" : null);
            Console.WriteLine($"[OK] Game process created (PID: {process.ProcessId})");
            Console.WriteLine($"[..] Injecting {RuntimeDllName}...");

            Injector.InjectDll(process, runtimeDll);
            Console.WriteLine($"[OK] {RuntimeDllName} injected and loaded in target process.");

            Injector.ResumeProcess(process);
            Console.WriteLine("[OK] Game resumed. WeaveLoader is active.");
            Console.WriteLine();
            Console.WriteLine("Press any key to exit the launcher (game will keep running).");
            Console.ReadKey(true);

            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Fatal error: {ex.Message}");
            Console.Error.WriteLine(ex.StackTrace);
            Console.WriteLine("Press any key to exit.");
            Console.ReadKey(true);
            return 1;
        }
    }

    private static bool TryReadMetadataSha(string metadataPath, out string sha)
    {
        sha = "";
        try
        {
            using var stream = File.OpenRead(metadataPath);
            using var doc = JsonDocument.Parse(stream);
            if (!doc.RootElement.TryGetProperty("gameExe", out var gameExe))
                return false;
            if (!gameExe.TryGetProperty("sha256", out var shaProp))
                return false;
            sha = shaProp.GetString() ?? "";
            return !string.IsNullOrWhiteSpace(sha);
        }
        catch
        {
            return false;
        }
    }

    private static bool TryGetFileSha256(string path, out string sha)
    {
        sha = "";
        try
        {
            using var stream = File.OpenRead(path);
            using var sha256 = SHA256.Create();
            byte[] hash = sha256.ComputeHash(stream);
            sha = Convert.ToHexString(hash).ToLowerInvariant();
            return true;
        }
        catch
        {
            return false;
        }
    }

    private static void SafeDelete(string path)
    {
        try
        {
            if (File.Exists(path))
                File.Delete(path);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[WARN] Failed to delete {path}: {ex.Message}");
        }
    }
}
