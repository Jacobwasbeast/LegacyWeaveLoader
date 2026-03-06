#include "SymbolResolver.h"
#include <cstdio>
#include <cstring>

#pragma comment(lib, "dbghelp.lib")

bool SymbolResolver::Initialize()
{
    m_process = GetCurrentProcess();

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_DEBUG);

    if (!SymInitialize(m_process, nullptr, TRUE))
    {
        return false;
    }

    m_initialized = true;
    return true;
}

void* SymbolResolver::Resolve(const char* functionName)
{
    if (!m_initialized) return nullptr;

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];
    memset(buffer, 0, sizeof(buffer));

    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    if (SymFromName(m_process, functionName, symbol))
    {
        return reinterpret_cast<void*>(symbol->Address);
    }

    return nullptr;
}

struct EnumContext
{
    const char* targetName;
    void* result;
};

static BOOL CALLBACK EnumSymbolsCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
    auto* ctx = static_cast<EnumContext*>(UserContext);
    if (strstr(pSymInfo->Name, ctx->targetName) != nullptr)
    {
        ctx->result = reinterpret_cast<void*>(pSymInfo->Address);
        return FALSE;
    }
    return TRUE;
}

bool SymbolResolver::ResolveGameFunctions()
{
    // MinecraftWorld_RunStaticCtors is a free function (not mangled in complex ways)
    pRunStaticCtors = Resolve("MinecraftWorld_RunStaticCtors");
    if (!pRunStaticCtors)
    {
        // Try with wildcard enumeration
        EnumContext ctx = { "MinecraftWorld_RunStaticCtors", nullptr };
        SymEnumSymbols(m_process, 0, "*MinecraftWorld_RunStaticCtors*", EnumSymbolsCallback, &ctx);
        pRunStaticCtors = ctx.result;
    }

    // Minecraft::tick -- MSVC mangles this. Try undecorated name first.
    pMinecraftTick = Resolve("Minecraft::tick");
    if (!pMinecraftTick)
    {
        EnumContext ctx = { "Minecraft::tick", nullptr };
        SymEnumSymbols(m_process, 0, "*Minecraft*tick*", EnumSymbolsCallback, &ctx);
        pMinecraftTick = ctx.result;
    }

    // Minecraft::init
    pMinecraftInit = Resolve("Minecraft::init");
    if (!pMinecraftInit)
    {
        EnumContext ctx = { "Minecraft::init", nullptr };
        SymEnumSymbols(m_process, 0, "*Minecraft*init*", EnumSymbolsCallback, &ctx);
        pMinecraftInit = ctx.result;
    }

    // Minecraft::destroy
    pMinecraftDestroy = Resolve("Minecraft::destroy");
    if (!pMinecraftDestroy)
    {
        EnumContext ctx = { "Minecraft::destroy", nullptr };
        SymEnumSymbols(m_process, 0, "*Minecraft*destroy*", EnumSymbolsCallback, &ctx);
        pMinecraftDestroy = ctx.result;
    }

    bool allResolved = pRunStaticCtors && pMinecraftTick && pMinecraftInit && pMinecraftDestroy;

    if (pRunStaticCtors) printf("[LegacyForge] Resolved RunStaticCtors @ %p\n", pRunStaticCtors);
    else printf("[LegacyForge] MISSING: MinecraftWorld_RunStaticCtors\n");

    if (pMinecraftTick) printf("[LegacyForge] Resolved Minecraft::tick @ %p\n", pMinecraftTick);
    else printf("[LegacyForge] MISSING: Minecraft::tick\n");

    if (pMinecraftInit) printf("[LegacyForge] Resolved Minecraft::init @ %p\n", pMinecraftInit);
    else printf("[LegacyForge] MISSING: Minecraft::init\n");

    if (pMinecraftDestroy) printf("[LegacyForge] Resolved Minecraft::destroy @ %p\n", pMinecraftDestroy);
    else printf("[LegacyForge] MISSING: Minecraft::destroy\n");

    return allResolved;
}

void SymbolResolver::Cleanup()
{
    if (m_initialized)
    {
        SymCleanup(m_process);
        m_initialized = false;
    }
}
