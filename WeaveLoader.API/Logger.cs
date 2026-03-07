namespace WeaveLoader.API;

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
    private static Action<string, LogLevel>? LogHandler;

    /// <summary>
    /// Set the log handler that routes messages to the native runtime.
    /// Called by WeaveLoader.Core during initialization.
    /// </summary>
    public static void SetLogHandler(Action<string, LogLevel> handler) => LogHandler = handler;

    public static void Debug(string message) => Log(message, LogLevel.Debug);
    public static void Info(string message) => Log(message, LogLevel.Info);
    public static void Warning(string message) => Log(message, LogLevel.Warning);
    public static void Error(string message) => Log(message, LogLevel.Error);

    public static void Log(string message, LogLevel level = LogLevel.Info)
    {
        if (LogHandler != null)
            LogHandler(message, level);
        else
            Console.WriteLine($"[WeaveLoader/{level}] {message}");
    }
}
