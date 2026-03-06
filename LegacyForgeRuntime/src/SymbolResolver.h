#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>

class SymbolResolver
{
public:
    bool Initialize();
    bool ResolveGameFunctions();
    void Cleanup();

    void* Resolve(const char* functionName);

    void* pRunStaticCtors = nullptr;
    void* pMinecraftTick = nullptr;
    void* pMinecraftInit = nullptr;
    void* pMinecraftDestroy = nullptr;

private:
    HANDLE m_process = nullptr;
    bool m_initialized = false;
};
