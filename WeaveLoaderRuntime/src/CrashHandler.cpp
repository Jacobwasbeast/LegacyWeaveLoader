#include "CrashHandler.h"
#include "LogUtil.h"
#include "PdbParser.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <eh.h>
#include <csignal>
#include <exception>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdlib>

static HMODULE s_runtimeModule = nullptr;
static uintptr_t s_gameBase = 0;
static volatile LONG s_handling = 0;

#ifndef STATUS_STACK_BUFFER_OVERRUN
#define STATUS_STACK_BUFFER_OVERRUN ((DWORD)0xC0000409)
#endif

#ifndef STATUS_INVALID_CRUNTIME_PARAMETER
#define STATUS_INVALID_CRUNTIME_PARAMETER ((DWORD)0xC0000417)
#endif

#ifndef STATUS_FAIL_FAST_EXCEPTION
#define STATUS_FAIL_FAST_EXCEPTION ((DWORD)0xC0000602)
#endif

static const char* ExceptionCodeToString(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_GUARD_PAGE:               return "EXCEPTION_GUARD_PAGE";
    case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
    case STATUS_STACK_BUFFER_OVERRUN:        return "STATUS_STACK_BUFFER_OVERRUN";
    case STATUS_INVALID_CRUNTIME_PARAMETER:  return "STATUS_INVALID_CRUNTIME_PARAMETER";
    case STATUS_FAIL_FAST_EXCEPTION:         return "STATUS_FAIL_FAST_EXCEPTION";
    default:                                 return "UNKNOWN_EXCEPTION";
    }
}

static bool IsFatalException(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
    case EXCEPTION_GUARD_PAGE:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
    case EXCEPTION_INVALID_DISPOSITION:
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_STACK_OVERFLOW:
    case STATUS_STACK_BUFFER_OVERRUN:
    case STATUS_INVALID_CRUNTIME_PARAMETER:
    case STATUS_FAIL_FAST_EXCEPTION:
        return true;
    default:
        return false;
    }
}

static bool ShouldWriteVectoredReport(EXCEPTION_RECORD* er)
{
    if (!er)
        return false;

    switch (er->ExceptionCode)
    {
    case STATUS_STACK_BUFFER_OVERRUN:
    case STATUS_INVALID_CRUNTIME_PARAMETER:
    case STATUS_FAIL_FAST_EXCEPTION:
    case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        return true;
    default:
        return false;
    }
}

static void GetModuleForAddr(DWORD64 addr, char* nameBuf, size_t nameBufSize, DWORD64* outBase)
{
    *outBase = 0;
    nameBuf[0] = '\0';

    HMODULE hMod = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(addr), &hMod))
    {
        GetModuleFileNameA(hMod, nameBuf, (DWORD)nameBufSize);
        *outBase = reinterpret_cast<DWORD64>(hMod);
    }
}

static void WalkStack(CONTEXT* ctx)
{
    LogUtil::LogCrash("Stack trace (from exception context):");

    CONTEXT localCtx = *ctx;
    int frame = 0;
    const int maxFrames = 64;

    while (frame < maxFrames)
    {
        DWORD64 rip = localCtx.Rip;
        if (rip == 0) break;

        char modPath[MAX_PATH] = {0};
        DWORD64 modBase = 0;
        GetModuleForAddr(rip, modPath, sizeof(modPath), &modBase);

        const char* modName = "???";
        if (modPath[0])
        {
            char* slash = strrchr(modPath, '\\');
            modName = slash ? slash + 1 : modPath;
        }

        char symName[512] = {0};
        uint32_t symOff = 0;
        if (s_gameBase != 0 && modBase == static_cast<DWORD64>(s_gameBase))
        {
            uint32_t rva = static_cast<uint32_t>(rip - modBase);
            if (PdbParser::FindNameByRVA(rva, symName, sizeof(symName), &symOff))
            {
                LogUtil::LogCrash("  [%2d] 0x%016llX  %s!%s+0x%X",
                                  frame, rip, modName, symName, symOff);
            }
            else
            {
                LogUtil::LogCrash("  [%2d] 0x%016llX  %s+0x%llX",
                                  frame, rip, modName, rip - modBase);
            }
        }
        else
        {
            LogUtil::LogCrash("  [%2d] 0x%016llX  %s+0x%llX",
                              frame, rip, modName, rip - modBase);
        }

        frame++;

        DWORD64 imageBase = 0;
        PRUNTIME_FUNCTION pFunc = RtlLookupFunctionEntry(rip, &imageBase, nullptr);
        if (!pFunc)
            break;

        void* handlerData = nullptr;
        DWORD64 establisherFrame = 0;
        KNONVOLATILE_CONTEXT_POINTERS nvCtx = {};
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, rip, pFunc,
                         &localCtx, &handlerData, &establisherFrame, &nvCtx);

        if (localCtx.Rip == 0) break;
    }
}

static void WriteCrashReport(const char* origin, EXCEPTION_RECORD* er, CONTEXT* ctx)
{
    if (!er || !ctx)
        return;

    if (InterlockedCompareExchange(&s_handling, 1, 0) != 0)
        return;

    DWORD code = er->ExceptionCode;
    DWORD64 faultAddr = er->ExceptionAddress
        ? reinterpret_cast<DWORD64>(er->ExceptionAddress)
        : ctx->Rip;

    SYSTEMTIME st;
    GetLocalTime(&st);
    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    LogUtil::LogCrash("========================================");
    LogUtil::LogCrash("  WeaveLoader Crash Report");
    LogUtil::LogCrash("  %s", timeBuf);
    LogUtil::LogCrash("========================================");
    LogUtil::LogCrash("");
    LogUtil::LogCrash("Origin:    %s", origin ? origin : "<unknown>");
    LogUtil::LogCrash("Exception: %s (0x%08X)", ExceptionCodeToString(code), code);
    LogUtil::LogCrash("Address:   0x%016llX", faultAddr);
    LogUtil::LogCrash("Thread:    %lu", GetCurrentThreadId());

    if (code == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2)
    {
        const char* op = er->ExceptionInformation[0] == 0 ? "read"
                       : er->ExceptionInformation[0] == 1 ? "write"
                       : "execute";
        LogUtil::LogCrash("Fault:     %s of address 0x%016llX", op, er->ExceptionInformation[1]);
    }

    {
        char modPath[MAX_PATH] = {0};
        DWORD64 modBase = 0;
        GetModuleForAddr(faultAddr, modPath, sizeof(modPath), &modBase);
        if (modPath[0])
            LogUtil::LogCrash("Module:    %s (base: 0x%016llX, offset: +0x%llX)",
                              modPath, modBase, faultAddr - modBase);

        char symName[512] = {0};
        uint32_t symOff = 0;
        if (s_gameBase != 0 && modBase == static_cast<DWORD64>(s_gameBase))
        {
            uint32_t rva = static_cast<uint32_t>(faultAddr - modBase);
            if (PdbParser::FindNameByRVA(rva, symName, sizeof(symName), &symOff))
                LogUtil::LogCrash("Symbol:    %s+0x%X", symName, symOff);
        }
    }

    LogUtil::LogCrash("");
    LogUtil::LogCrash("Registers:");
    LogUtil::LogCrash("  RAX = 0x%016llX  RBX = 0x%016llX", ctx->Rax, ctx->Rbx);
    LogUtil::LogCrash("  RCX = 0x%016llX  RDX = 0x%016llX", ctx->Rcx, ctx->Rdx);
    LogUtil::LogCrash("  RSI = 0x%016llX  RDI = 0x%016llX", ctx->Rsi, ctx->Rdi);
    LogUtil::LogCrash("  RSP = 0x%016llX  RBP = 0x%016llX", ctx->Rsp, ctx->Rbp);
    LogUtil::LogCrash("  R8  = 0x%016llX  R9  = 0x%016llX", ctx->R8, ctx->R9);
    LogUtil::LogCrash("  R10 = 0x%016llX  R11 = 0x%016llX", ctx->R10, ctx->R11);
    LogUtil::LogCrash("  R12 = 0x%016llX  R13 = 0x%016llX", ctx->R12, ctx->R13);
    LogUtil::LogCrash("  R14 = 0x%016llX  R15 = 0x%016llX", ctx->R14, ctx->R15);
    LogUtil::LogCrash("  RIP = 0x%016llX  EFLAGS = 0x%08X", ctx->Rip, ctx->EFlags);

    LogUtil::LogCrash("");
    WalkStack(ctx);

    LogUtil::LogCrash("");
    LogUtil::LogCrash("Loaded modules:");
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 me;
        me.dwSize = sizeof(me);
        if (Module32First(hSnap, &me))
        {
            do {
                LogUtil::LogCrash("  0x%016llX - 0x%016llX  %s",
                                  reinterpret_cast<DWORD64>(me.modBaseAddr),
                                  reinterpret_cast<DWORD64>(me.modBaseAddr) + me.modBaseSize,
                                  me.szModule);
            } while (Module32Next(hSnap, &me));
        }
        CloseHandle(hSnap);
    }

    LogUtil::LogCrash("");
    LogUtil::LogCrash("========================================");
    LogUtil::LogCrash("  End of crash report");
    LogUtil::LogCrash("========================================");
    LogUtil::LogCrash("");

    LogUtil::Log("[WeaveLoader] CRASH DETECTED - see logs/crash.log for details");

    InterlockedExchange(&s_handling, 0);
}

static void LogCapturedCrash(const char* origin, DWORD code)
{
    EXCEPTION_RECORD er{};
    er.ExceptionCode = code;
    er.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

    CONTEXT ctx{};
    RtlCaptureContext(&ctx);
    er.ExceptionAddress = reinterpret_cast<void*>(ctx.Rip);

    WriteCrashReport(origin, &er, &ctx);
}

static void __cdecl PurecallHandler()
{
    LogCapturedCrash("_purecall", STATUS_INVALID_CRUNTIME_PARAMETER);
    TerminateProcess(GetCurrentProcess(), STATUS_INVALID_CRUNTIME_PARAMETER);
}

static void __cdecl TerminateHandler()
{
    LogCapturedCrash("std::terminate", STATUS_FAIL_FAST_EXCEPTION);
    TerminateProcess(GetCurrentProcess(), STATUS_FAIL_FAST_EXCEPTION);
}

static void __cdecl SignalHandler(int sig)
{
    DWORD code = STATUS_FAIL_FAST_EXCEPTION;
    switch (sig)
    {
    case SIGABRT: code = STATUS_STACK_BUFFER_OVERRUN; break;
    case SIGSEGV: code = EXCEPTION_ACCESS_VIOLATION; break;
    case SIGILL:  code = EXCEPTION_ILLEGAL_INSTRUCTION; break;
    case SIGFPE:  code = EXCEPTION_FLT_INVALID_OPERATION; break;
    default:      break;
    }

    LogCapturedCrash("signal", code);
    TerminateProcess(GetCurrentProcess(), code);
}

static void InvalidParameterHandler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
    LogCapturedCrash("_invalid_parameter", STATUS_INVALID_CRUNTIME_PARAMETER);
    TerminateProcess(GetCurrentProcess(), STATUS_INVALID_CRUNTIME_PARAMETER);
}

static LONG WINAPI TopLevelHandler(EXCEPTION_POINTERS* ep)
{
    if (!ep || !ep->ExceptionRecord || !ep->ContextRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    if (!IsFatalException(ep->ExceptionRecord->ExceptionCode))
        return EXCEPTION_CONTINUE_SEARCH;

    WriteCrashReport("UnhandledExceptionFilter", ep->ExceptionRecord, ep->ContextRecord);
    return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI VectoredHandler(EXCEPTION_POINTERS* ep)
{
    if (!ep || !ep->ExceptionRecord || !ep->ContextRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    DWORD code = ep->ExceptionRecord->ExceptionCode;

    // The game's debug build uses __debugbreak() (int 3) as assertions throughout
    // the texture system and other subsystems. Without a debugger the default
    // handler terminates the process. We skip past the 1-byte int 3 instruction
    // so the game's fallback code (e.g. missing-texture) can run normally.
    if (code == EXCEPTION_BREAKPOINT && s_gameBase != 0)
    {
        DWORD64 rip = ep->ContextRecord->Rip;
        HMODULE hMod = nullptr;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(rip), &hMod) &&
            reinterpret_cast<uintptr_t>(hMod) == s_gameBase)
        {
            ep->ContextRecord->Rip += 1;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    if (!IsFatalException(code))
        return EXCEPTION_CONTINUE_SEARCH;

    // TODO: Remove this suppression once the animated texture pack fault is fixed
    // at the source. For release builds, keep vectored reporting only for
    // fail-fast/noncontinuable cases so handled first-chance faults do not spam
    // logs and stall the process. In debug builds, log all fatal exceptions to
    // make crash investigation easier.
#ifdef _DEBUG
    WriteCrashReport("VectoredExceptionHandler", ep->ExceptionRecord, ep->ContextRecord);
#else
    if (ShouldWriteVectoredReport(ep->ExceptionRecord))
        WriteCrashReport("VectoredExceptionHandler", ep->ExceptionRecord, ep->ContextRecord);
#endif

    return EXCEPTION_CONTINUE_SEARCH;
}

namespace CrashHandler
{

void Install(HMODULE runtimeModule)
{
    s_runtimeModule = runtimeModule;
    AddVectoredExceptionHandler(1, VectoredHandler);
    SetUnhandledExceptionFilter(TopLevelHandler);
    _set_invalid_parameter_handler(InvalidParameterHandler);
    _set_purecall_handler(PurecallHandler);
    std::set_terminate(TerminateHandler);
    std::signal(SIGABRT, SignalHandler);
    std::signal(SIGSEGV, SignalHandler);
    std::signal(SIGILL, SignalHandler);
    std::signal(SIGFPE, SignalHandler);
}

void SetGameBase(uintptr_t base)
{
    s_gameBase = base;
    LogUtil::Log("[WeaveLoader] Crash handler: game base set to 0x%016llX", (DWORD64)base);
}

} // namespace CrashHandler
