#pragma once
#include <Windows.h>
#include <cstdint>

namespace CrashHandler
{
    // Installs the vectored exception handler. Safe to call from DllMain.
    void Install(HMODULE runtimeModule);

    // Store the game exe's base address so we can compute RVAs for PDB lookups.
    void SetGameBase(uintptr_t base);
}
