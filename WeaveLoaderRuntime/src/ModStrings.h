#pragma once

#include <unordered_map>
#include <string>
#include <mutex>

namespace ModStrings
{
    constexpr int MOD_DESC_ID_BASE = 10000;

    void Register(int descriptionId, const wchar_t* value);
    const wchar_t* Get(int descriptionId);
    int AllocateId();
    bool IsModId(int id);

    // Parse CMinecraftApp::GetString machine code to locate the game's
    // string table pointer. Must be called BEFORE MinHook overwrites
    // the function prologue.
    void CaptureStringTableRef(void* pGetStringFunc);

    // After the string table is loaded (e.g. during PreInit), call this
    // to inject all previously registered mod strings into the game's
    // m_stringsVec so that inlined GetString calls find them.
    void InjectAllIntoGameTable();
}
