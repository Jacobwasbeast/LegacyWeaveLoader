#pragma once

#include "NativeExports.h"
#include <string>
#include <vector>

namespace HookRegistry
{
    enum HookAt
    {
        HookAt_Head = 0,
        HookAt_Tail = 1
    };

    struct HookContext
    {
        void* thisPtr = nullptr;
        NativeArg* args = nullptr;
        int argCount = 0;
        NativeRet* ret = nullptr;
        int cancel = 0;
    };

    using ManagedHookFn = void (__cdecl *)(HookContext* ctx);

    bool AddHook(const char* fullName, int at, void* managedCallback, int require);
    bool RemoveHook(const char* fullName, void* managedCallback);
}
