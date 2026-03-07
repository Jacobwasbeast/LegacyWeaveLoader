using System.Runtime.InteropServices;

namespace WeaveLoader.Launcher;

class Program
{
    private const string RuntimeDllName = "WeaveLoaderRuntime.dll";

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

        try
        {
            var config = Config.Load(configFile);

            if (args.Length > 0 && File.Exists(args[0]))
            {
                config.GameExePath = args[0];
                config.Save(configFile);
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

            var process = Injector.LaunchSuspended(config.GameExePath);
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
}
