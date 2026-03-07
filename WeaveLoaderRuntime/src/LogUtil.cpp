#include "LogUtil.h"
#include <Windows.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <string>

static std::string s_logsDir;
static std::string s_logPath;
static std::string s_gameLogPath;
static std::string s_crashLogPath;

static void GetTimestamp(char* buf, size_t bufSize)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buf, bufSize, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

namespace LogUtil
{

void SetBaseDir(const char* baseDir)
{
    s_logsDir      = std::string(baseDir) + "logs\\";
    CreateDirectoryA(s_logsDir.c_str(), nullptr);
    s_logPath      = s_logsDir + "weaveloader.log";
    s_gameLogPath  = s_logsDir + "game_debug.log";
    s_crashLogPath = s_logsDir + "crash.log";
}

const char* GetLogsDir()
{
    return s_logsDir.c_str();
}

void Log(const char* fmt, ...)
{
    char ts[32];
    GetTimestamp(ts, sizeof(ts));

    printf("[%s] ", ts);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");

    if (s_logPath.empty()) return;

    FILE* f = nullptr;
    fopen_s(&f, s_logPath.c_str(), "a");
    if (f)
    {
        fprintf(f, "[%s] ", ts);
        va_list args2;
        va_start(args2, fmt);
        vfprintf(f, fmt, args2);
        va_end(args2);
        fprintf(f, "\n");
        fclose(f);
    }
}

void LogGameOutput(const char* str, size_t len)
{
    if (s_gameLogPath.empty() || !str || len == 0) return;

    char ts[32];
    GetTimestamp(ts, sizeof(ts));

    FILE* f = nullptr;
    fopen_s(&f, s_gameLogPath.c_str(), "a");
    if (f)
    {
        fprintf(f, "[%s] ", ts);
        fwrite(str, 1, len, f);
        fprintf(f, "\n");
        fclose(f);
    }
}

void LogCrash(const char* fmt, ...)
{
    if (s_crashLogPath.empty()) return;

    FILE* f = nullptr;
    fopen_s(&f, s_crashLogPath.c_str(), "a");
    if (f)
    {
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fprintf(f, "\n");
        fclose(f);
    }
}

} // namespace LogUtil
