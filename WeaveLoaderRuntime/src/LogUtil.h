#pragma once
#include <cstddef>

namespace LogUtil
{
    // Must be called once at startup with the runtime DLL's directory (with trailing backslash).
    // Creates a logs/ subdirectory and sets up all log file paths.
    void SetBaseDir(const char* baseDir);

    // Returns the logs directory path (with trailing backslash)
    const char* GetLogsDir();

    // Appends a timestamped line to weaveloader.log and prints to stdout
    void Log(const char* fmt, ...);

    // Writes game debug output to game_debug.log with a timestamp
    void LogGameOutput(const char* str, size_t len);

    // Writes crash information to crash.log (no timestamp prefix -- caller manages formatting)
    void LogCrash(const char* fmt, ...);
}
