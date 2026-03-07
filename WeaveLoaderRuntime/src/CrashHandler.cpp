#include "CrashHandler.h"
#include "LogUtil.h"
#include "PdbParser.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <cstring>
#include <ctime>

static HMODULE s_runtimeModule = nullptr;
static uintptr_t s_gameBase = 0;
static volatile LONG s_handling = 0;

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
    case EXCEPTION_SINGLE_STEP:             return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
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

    // Prevent re-entrancy if the crash handler itself crashes
    if (InterlockedCompareExchange(&s_handling, 1, 0) != 0)
        return EXCEPTION_CONTINUE_SEARCH;

    EXCEPTION_RECORD* er = ep->ExceptionRecord;
    CONTEXT* ctx = ep->ContextRecord;

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
    LogUtil::LogCrash("Exception: %s (0x%08X)", ExceptionCodeToString(code), code);
    LogUtil::LogCrash("Address:   0x%016llX", reinterpret_cast<DWORD64>(er->ExceptionAddress));
    LogUtil::LogCrash("Thread:    %lu", GetCurrentThreadId());

    if (code == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2)
    {
        const char* op = er->ExceptionInformation[0] == 0 ? "read"
                       : er->ExceptionInformation[0] == 1 ? "write"
                       : "execute";
        LogUtil::LogCrash("Fault:     %s of address 0x%016llX", op, er->ExceptionInformation[1]);
    }

    // Module containing the faulting address + PDB symbol resolution
    {
        DWORD64 faultAddr = reinterpret_cast<DWORD64>(er->ExceptionAddress);
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

    // Loaded modules
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
    return EXCEPTION_CONTINUE_SEARCH;
}

namespace CrashHandler
{

void Install(HMODULE runtimeModule)
{
    s_runtimeModule = runtimeModule;
    AddVectoredExceptionHandler(1, VectoredHandler);
}

void SetGameBase(uintptr_t base)
{
    s_gameBase = base;
    LogUtil::Log("[WeaveLoader] Crash handler: game base set to 0x%016llX", (DWORD64)base);
}

} // namespace CrashHandler
