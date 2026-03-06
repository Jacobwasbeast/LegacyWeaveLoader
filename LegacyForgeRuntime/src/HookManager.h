#pragma once

class SymbolResolver;

namespace HookManager
{
    bool Install(const SymbolResolver& symbols);
    void Cleanup();
}
