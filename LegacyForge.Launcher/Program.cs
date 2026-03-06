namespace LegacyForge.Launcher;

class Program
{
    private const string ConfigFile = "legacyforge.json";
    private const string RuntimeDll = "LegacyForgeRuntime.dll";

    static int Main(string[] args)
    {
        Console.WriteLine("╔══════════════════════════════════╗");
        Console.WriteLine("║         LegacyForge v1.0        ║");
        Console.WriteLine("║  Mod Loader for MC Legacy Edition║");
        Console.WriteLine("╚══════════════════════════════════╝");
        Console.WriteLine();

        try
        {
            var config = Config.Load(ConfigFile);

            if (args.Length > 0 && File.Exists(args[0]))
            {
                config.GameExePath = args[0];
                config.Save(ConfigFile);
            }

            if (string.IsNullOrEmpty(config.GameExePath) || !File.Exists(config.GameExePath))
            {
                Console.Write("Enter path to MinecraftLegacy.exe: ");
                string? input = Console.ReadLine()?.Trim().Trim('"');

                if (string.IsNullOrEmpty(input) || !File.Exists(input))
                {
                    Console.Error.WriteLine("Error: Invalid path or file not found.");
                    return 1;
                }

                config.GameExePath = Path.GetFullPath(input);
                config.Save(ConfigFile);
                Console.WriteLine($"Saved game path to {ConfigFile}");
            }

            if (!File.Exists(RuntimeDll))
            {
                Console.Error.WriteLine($"Error: {RuntimeDll} not found in current directory.");
                Console.Error.WriteLine("Make sure the runtime DLL is next to LegacyForge.exe.");
                return 1;
            }

            string modsDir = Path.Combine(AppContext.BaseDirectory, "mods");
            if (!Directory.Exists(modsDir))
            {
                Directory.CreateDirectory(modsDir);
                Console.WriteLine($"Created mods/ directory at {modsDir}");
            }

            int modCount = Directory.GetFiles(modsDir, "*.dll").Length;
            Console.WriteLine($"Found {modCount} mod(s) in mods/");
            Console.WriteLine($"Launching {Path.GetFileName(config.GameExePath)}...");

            var process = Injector.LaunchSuspended(config.GameExePath);
            Console.WriteLine($"Game process created (PID: {process.ProcessId}), injecting runtime...");

            Injector.InjectDll(process, RuntimeDll);
            Console.WriteLine("LegacyForgeRuntime.dll injected successfully.");

            Injector.ResumeProcess(process);
            Console.WriteLine("Game resumed. LegacyForge is active.");
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
