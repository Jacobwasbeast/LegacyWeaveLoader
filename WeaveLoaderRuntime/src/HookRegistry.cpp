#include "HookRegistry.h"
#include "SymbolRegistry.h"
#include "LogUtil.h"
#include <Windows.h>
#include <MinHook.h>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <memory>
#include <cstring>

namespace
{
    struct HookEntry
    {
        std::string fullName;
        void* target = nullptr;
        void* original = nullptr;
        std::string signatureKey;
        NativeType retType = NativeType_I32;
        std::vector<NativeType> argTypes;
        int argCount = 0;
        bool hasThis = true;
        std::vector<void*> pre;
        std::vector<void*> post;
        void* thunk = nullptr;
    };

    std::unordered_map<std::string, std::unique_ptr<HookEntry>> g_hooks;
    std::mutex g_mutex;
    HookEntry* g_activeEntry = nullptr;

    bool ParseSignature(const std::string& key, NativeType& ret, std::vector<NativeType>& args)
    {
        if (key.empty())
            return false;
        args.clear();
        auto mapToken = [](char c, NativeType& out) -> bool
        {
            switch (c)
            {
            case 'i': out = NativeType_I32; return true;
            case 'l': out = NativeType_I64; return true;
            case 'p': out = NativeType_Ptr; return true;
            case 'b': out = NativeType_Bool; return true;
            case 'v': out = NativeType_I32; return true;
            default: return false;
            }
        };

        NativeType retType;
        if (!mapToken(key[0], retType))
            return false;
        ret = retType;

        size_t pos = 1;
        while (pos < key.size())
        {
            if (key[pos] != '_')
                break;
            ++pos;
            if (pos >= key.size())
                break;
            NativeType argType;
            if (!mapToken(key[pos], argType))
                return false;
            args.push_back(argType);
            ++pos;
        }
        return true;
    }

    uint64_t CallOriginal(HookEntry* entry, void* thisPtr, const uint64_t* a)
    {
        if (!entry || !entry->original)
            return 0;
        if (entry->hasThis)
        {
            switch (entry->argCount)
            {
            case 0: return reinterpret_cast<uint64_t(__fastcall *)(void*)>(entry->original)(thisPtr);
            case 1: return reinterpret_cast<uint64_t(__fastcall *)(void*, uint64_t)>(entry->original)(thisPtr, a[0]);
            case 2: return reinterpret_cast<uint64_t(__fastcall *)(void*, uint64_t, uint64_t)>(entry->original)(thisPtr, a[0], a[1]);
            case 3: return reinterpret_cast<uint64_t(__fastcall *)(void*, uint64_t, uint64_t, uint64_t)>(entry->original)(thisPtr, a[0], a[1], a[2]);
            case 4: return reinterpret_cast<uint64_t(__fastcall *)(void*, uint64_t, uint64_t, uint64_t, uint64_t)>(entry->original)(thisPtr, a[0], a[1], a[2], a[3]);
            case 5: return reinterpret_cast<uint64_t(__fastcall *)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(entry->original)(thisPtr, a[0], a[1], a[2], a[3], a[4]);
            default: return reinterpret_cast<uint64_t(__fastcall *)(void*, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(entry->original)(thisPtr, a[0], a[1], a[2], a[3], a[4], a[5]);
            }
        }
        else
        {
            switch (entry->argCount)
            {
            case 0: return reinterpret_cast<uint64_t(__fastcall *)()>(entry->original)();
            case 1: return reinterpret_cast<uint64_t(__fastcall *)(uint64_t)>(entry->original)(a[0]);
            case 2: return reinterpret_cast<uint64_t(__fastcall *)(uint64_t, uint64_t)>(entry->original)(a[0], a[1]);
            case 3: return reinterpret_cast<uint64_t(__fastcall *)(uint64_t, uint64_t, uint64_t)>(entry->original)(a[0], a[1], a[2]);
            case 4: return reinterpret_cast<uint64_t(__fastcall *)(uint64_t, uint64_t, uint64_t, uint64_t)>(entry->original)(a[0], a[1], a[2], a[3]);
            case 5: return reinterpret_cast<uint64_t(__fastcall *)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(entry->original)(a[0], a[1], a[2], a[3], a[4]);
            default: return reinterpret_cast<uint64_t(__fastcall *)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)>(entry->original)(a[0], a[1], a[2], a[3], a[4], a[5]);
            }
        }
    }

    uint64_t InvokeHooks(HookEntry* entry, void* thisPtr, uint64_t* a)
    {
        if (!entry)
            return 0;

        NativeArg args[6] = {};
        for (int i = 0; i < entry->argCount && i < 6; ++i)
        {
            args[i].type = entry->argTypes[i];
            args[i].value = a[i];
        }

        NativeRet ret{};
        ret.type = entry->retType;
        ret.value = 0;

        HookRegistry::HookContext ctx;
        ctx.thisPtr = thisPtr;
        ctx.args = args;
        ctx.argCount = entry->argCount;
        ctx.ret = &ret;
        ctx.cancel = 0;

        for (void* cb : entry->pre)
            reinterpret_cast<HookRegistry::ManagedHookFn>(cb)(&ctx);

        for (int i = 0; i < entry->argCount && i < 6; ++i)
            a[i] = args[i].value;

        if (ctx.cancel == 0)
            ret.value = CallOriginal(entry, thisPtr, a);

        for (void* cb : entry->post)
            reinterpret_cast<HookRegistry::ManagedHookFn>(cb)(&ctx);

        return ret.value;
    }

    uint64_t __fastcall HandleThis0(void* thisPtr)
    {
        uint64_t args[1] = {};
        return InvokeHooks(g_activeEntry, thisPtr, args);
    }
    uint64_t __fastcall HandleThis1(void* thisPtr, uint64_t a0)
    {
        uint64_t args[1] = { a0 };
        return InvokeHooks(g_activeEntry, thisPtr, args);
    }
    uint64_t __fastcall HandleThis2(void* thisPtr, uint64_t a0, uint64_t a1)
    {
        uint64_t args[2] = { a0, a1 };
        return InvokeHooks(g_activeEntry, thisPtr, args);
    }
    uint64_t __fastcall HandleThis3(void* thisPtr, uint64_t a0, uint64_t a1, uint64_t a2)
    {
        uint64_t args[3] = { a0, a1, a2 };
        return InvokeHooks(g_activeEntry, thisPtr, args);
    }
    uint64_t __fastcall HandleThis4(void* thisPtr, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)
    {
        uint64_t args[4] = { a0, a1, a2, a3 };
        return InvokeHooks(g_activeEntry, thisPtr, args);
    }
    uint64_t __fastcall HandleThis5(void* thisPtr, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4)
    {
        uint64_t args[5] = { a0, a1, a2, a3, a4 };
        return InvokeHooks(g_activeEntry, thisPtr, args);
    }
    uint64_t __fastcall HandleThis6(void* thisPtr, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
    {
        uint64_t args[6] = { a0, a1, a2, a3, a4, a5 };
        return InvokeHooks(g_activeEntry, thisPtr, args);
    }

    uint64_t __fastcall HandleStatic0()
    {
        uint64_t args[1] = {};
        return InvokeHooks(g_activeEntry, nullptr, args);
    }
    uint64_t __fastcall HandleStatic1(uint64_t a0)
    {
        uint64_t args[1] = { a0 };
        return InvokeHooks(g_activeEntry, nullptr, args);
    }
    uint64_t __fastcall HandleStatic2(uint64_t a0, uint64_t a1)
    {
        uint64_t args[2] = { a0, a1 };
        return InvokeHooks(g_activeEntry, nullptr, args);
    }
    uint64_t __fastcall HandleStatic3(uint64_t a0, uint64_t a1, uint64_t a2)
    {
        uint64_t args[3] = { a0, a1, a2 };
        return InvokeHooks(g_activeEntry, nullptr, args);
    }
    uint64_t __fastcall HandleStatic4(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)
    {
        uint64_t args[4] = { a0, a1, a2, a3 };
        return InvokeHooks(g_activeEntry, nullptr, args);
    }
    uint64_t __fastcall HandleStatic5(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4)
    {
        uint64_t args[5] = { a0, a1, a2, a3, a4 };
        return InvokeHooks(g_activeEntry, nullptr, args);
    }
    uint64_t __fastcall HandleStatic6(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
    {
        uint64_t args[6] = { a0, a1, a2, a3, a4, a5 };
        return InvokeHooks(g_activeEntry, nullptr, args);
    }

    void* SelectHandler(bool hasThis, int argCount)
    {
        if (hasThis)
        {
            switch (argCount)
            {
            case 0: return reinterpret_cast<void*>(&HandleThis0);
            case 1: return reinterpret_cast<void*>(&HandleThis1);
            case 2: return reinterpret_cast<void*>(&HandleThis2);
            case 3: return reinterpret_cast<void*>(&HandleThis3);
            case 4: return reinterpret_cast<void*>(&HandleThis4);
            case 5: return reinterpret_cast<void*>(&HandleThis5);
            default: return reinterpret_cast<void*>(&HandleThis6);
            }
        }
        else
        {
            switch (argCount)
            {
            case 0: return reinterpret_cast<void*>(&HandleStatic0);
            case 1: return reinterpret_cast<void*>(&HandleStatic1);
            case 2: return reinterpret_cast<void*>(&HandleStatic2);
            case 3: return reinterpret_cast<void*>(&HandleStatic3);
            case 4: return reinterpret_cast<void*>(&HandleStatic4);
            case 5: return reinterpret_cast<void*>(&HandleStatic5);
            default: return reinterpret_cast<void*>(&HandleStatic6);
            }
        }
    }

    void* CreateThunk(void* handler, HookEntry* entry)
    {
        unsigned char* code = reinterpret_cast<unsigned char*>(
            VirtualAlloc(nullptr, 128, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!code)
            return nullptr;

        unsigned char* p = code;
        auto write64 = [&](uint64_t value)
        {
            memcpy(p, &value, sizeof(value));
            p += sizeof(value);
        };

        // mov rax, &g_activeEntry
        *p++ = 0x48; *p++ = 0xB8; write64(reinterpret_cast<uint64_t>(&g_activeEntry));
        // mov r11, [rax]
        *p++ = 0x4C; *p++ = 0x8B; *p++ = 0x18;
        // push r11
        *p++ = 0x41; *p++ = 0x53;
        // mov r11, entry
        *p++ = 0x49; *p++ = 0xBB; write64(reinterpret_cast<uint64_t>(entry));
        // mov [rax], r11
        *p++ = 0x4C; *p++ = 0x89; *p++ = 0x18;
        // sub rsp, 32
        *p++ = 0x48; *p++ = 0x83; *p++ = 0xEC; *p++ = 0x20;
        // mov r11, handler
        *p++ = 0x49; *p++ = 0xBB; write64(reinterpret_cast<uint64_t>(handler));
        // call r11
        *p++ = 0x41; *p++ = 0xFF; *p++ = 0xD3;
        // add rsp, 32
        *p++ = 0x48; *p++ = 0x83; *p++ = 0xC4; *p++ = 0x20;
        // mov rax, &g_activeEntry
        *p++ = 0x48; *p++ = 0xB8; write64(reinterpret_cast<uint64_t>(&g_activeEntry));
        // pop r10
        *p++ = 0x41; *p++ = 0x5A;
        // mov [rax], r10
        *p++ = 0x4C; *p++ = 0x89; *p++ = 0x10;
        // ret
        *p++ = 0xC3;

        return code;
    }
}

namespace HookRegistry
{
    bool AddHook(const char* fullName, int at, void* managedCallback, int require)
    {
        (void)require;
        if (!fullName || !managedCallback)
            return false;

        std::lock_guard<std::mutex> lock(g_mutex);
        HookEntry* entry = nullptr;
        auto it = g_hooks.find(fullName);
        if (it == g_hooks.end())
        {
            const SymbolRegistry::Entry* sym = SymbolRegistry::Instance().FindEntry(fullName);
            if (!sym || !sym->address)
                return false;

            auto created = std::make_unique<HookEntry>();
            created->fullName = fullName;
            created->target = sym->address;
            created->signatureKey = sym->signatureKey;
            created->hasThis = !sym->isStatic;

            if (!ParseSignature(created->signatureKey, created->retType, created->argTypes))
                return false;
            created->argCount = static_cast<int>(created->argTypes.size());
            if (created->argCount > 6)
                return false;

            void* handler = SelectHandler(created->hasThis, created->argCount);
            created->thunk = CreateThunk(handler, created.get());
            if (!created->thunk)
                return false;

            if (MH_Initialize() != MH_OK && MH_Initialize() != MH_ERROR_ALREADY_INITIALIZED)
                return false;

            if (MH_CreateHook(created->target, created->thunk, &created->original) != MH_OK)
                return false;
            if (MH_EnableHook(created->target) != MH_OK)
                return false;

            auto inserted = g_hooks.emplace(fullName, std::move(created));
            entry = inserted.first->second.get();
        }
        else
        {
            entry = it->second.get();
        }

        if (!entry)
            return false;

        if (at == HookAt_Tail)
            entry->post.push_back(managedCallback);
        else
            entry->pre.push_back(managedCallback);

        LogUtil::Log("[WeaveLoader] HookRegistry: hooked %s", fullName);
        return true;
    }

    bool RemoveHook(const char* fullName, void* managedCallback)
    {
        if (!fullName || !managedCallback)
            return false;
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_hooks.find(fullName);
        if (it == g_hooks.end())
            return false;
        HookEntry& entry = *it->second;
        auto removeFn = [&](std::vector<void*>& list)
        {
            list.erase(std::remove(list.begin(), list.end(), managedCallback), list.end());
        };
        removeFn(entry.pre);
        removeFn(entry.post);
        return true;
    }
}
