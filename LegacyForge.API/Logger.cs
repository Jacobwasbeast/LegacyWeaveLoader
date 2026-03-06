namespace LegacyForge.API;

public enum LogLevel
{
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
}

/// <summary>
/// Logging facade that routes messages through the native runtime to the game's debug output.
/// </summary>
public static class Logger
{
    internal static Action<string, LogLevel>? LogHandler;

    public static void Debug(string message) => Log(message, LogLevel.Debug);
    public static void Info(string message) => Log(message, LogLevel.Info);
    public static void Warning(string message) => Log(message, LogLevel.Warning);
    public static void Error(string message) => Log(message, LogLevel.Error);

    public static void Log(string message, LogLevel level = LogLevel.Info)
    {
        if (LogHandler != null)
            LogHandler(message, level);
        else
            Console.WriteLine($"[LegacyForge/{level}] {message}");
    }
}
