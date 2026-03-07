#include "ModStrings.h"
#include "LogUtil.h"
#include <vector>
#include <cstring>
#include <cstdint>

namespace ModStrings
{
    static std::mutex s_mutex;
    static std::unordered_map<int, std::wstring> s_strings;
    static int s_nextId = MOD_DESC_ID_BASE;

    // ---- Game string table injection ----
    // Points to the field inside 'app' that holds the StringTable* pointer.
    static void** s_pStringTableField = nullptr;
    // Offset of m_stringsVec inside the StringTable object (found by heuristic scan).
    static int s_vecOffset = -1;

    void Register(int descriptionId, const wchar_t* value)
    {
        if (!value) return;
        std::lock_guard<std::mutex> lock(s_mutex);
        s_strings[descriptionId] = value;
        LogUtil::Log("[WeaveLoader] ModStrings: registered id=%d -> %ls", descriptionId, value);
    }

    const wchar_t* Get(int descriptionId)
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_strings.find(descriptionId);
        if (it != s_strings.end())
            return it->second.c_str();
        return nullptr;
    }

    int AllocateId()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_nextId++;
    }

    bool IsModId(int id)
    {
        return id >= MOD_DESC_ID_BASE;
    }

    // ---- Machine code parsing to locate string table ----

    void CaptureStringTableRef(void* pGetStringFunc)
    {
        if (!pGetStringFunc) return;

        const uint8_t* code = static_cast<const uint8_t*>(pGetStringFunc);
        LogUtil::Log("[WeaveLoader] ModStrings: scanning GetString prologue at %p", pGetStringFunc);

        // Log first 32 bytes for diagnostics
        char hexBuf[200];
        for (int i = 0; i < 32 && i < 200/3; i++)
            sprintf(hexBuf + i * 3, "%02X ", code[i]);
        LogUtil::Log("[WeaveLoader] ModStrings: bytes: %s", hexBuf);

        // Search for RIP-relative memory accesses in first 80 bytes.
        // GetString is: return app.m_stringTable->getString(iID);
        // The compiler will load app.m_stringTable via a RIP-relative MOV.
        for (int i = 0; i < 74; i++)
        {
            // REX.W prefix (0x48 or 0x4C for r8-r15)
            if ((code[i] & 0xF8) != 0x48) continue;
            uint8_t rex = code[i];

            // MOV reg, [RIP + disp32]  =>  0x8B modrm
            // LEA reg, [RIP + disp32]  =>  0x8D modrm
            if (code[i + 1] != 0x8B && code[i + 1] != 0x8D) continue;

            uint8_t modrm = code[i + 2];
            // mod=00, rm=101 means [RIP + disp32]
            if ((modrm & 0xC7) != 0x05) continue;

            int32_t disp = *reinterpret_cast<const int32_t*>(code + i + 3);
            uintptr_t effectiveAddr = reinterpret_cast<uintptr_t>(code + i + 7) + disp;

            if (code[i + 1] == 0x8B)
            {
                // MOV reg, [RIP+disp] - directly loads a pointer value.
                // This is likely loading app.m_stringTable (or &app if it's a pointer global).
                // For a static member or global struct, the compiler often uses a direct
                // RIP-relative MOV to load the m_stringTable pointer field.
                s_pStringTableField = reinterpret_cast<void**>(effectiveAddr);
                LogUtil::Log("[WeaveLoader] ModStrings: MOV [RIP+disp] -> field at %p", s_pStringTableField);
                break;
            }
            else // LEA
            {
                // LEA reg, [RIP+disp] -> &app
                // Next instruction should load m_stringTable from app + offset
                uintptr_t appAddr = effectiveAddr;
                int j = i + 7;
                // Look for MOV reg, [reg + disp8/disp32]
                if (j + 3 < 80 && (code[j] & 0xF8) == 0x48 && code[j + 1] == 0x8B)
                {
                    uint8_t modrm2 = code[j + 2];
                    uint8_t mod2 = modrm2 >> 6;
                    if (mod2 == 1)
                    {
                        int8_t off = static_cast<int8_t>(code[j + 3]);
                        s_pStringTableField = reinterpret_cast<void**>(appAddr + off);
                        LogUtil::Log("[WeaveLoader] ModStrings: LEA+MOV [reg+%d] -> field at %p", (int)off, s_pStringTableField);
                        break;
                    }
                    else if (mod2 == 2)
                    {
                        int32_t off = *reinterpret_cast<const int32_t*>(code + j + 3);
                        s_pStringTableField = reinterpret_cast<void**>(appAddr + off);
                        LogUtil::Log("[WeaveLoader] ModStrings: LEA+MOV [reg+%d] -> field at %p", off, s_pStringTableField);
                        break;
                    }
                }
            }
        }

        if (!s_pStringTableField)
            LogUtil::Log("[WeaveLoader] ModStrings: WARNING - could not locate string table reference");
    }

    // Heuristic: find the vector<wstring> inside a StringTable object.
    static std::vector<std::wstring>* FindStringsVec(void* stringTable)
    {
        char* base = static_cast<char*>(stringTable);

        // StringTable layout (MSVC x64):
        //   +0x00: bool isStatic
        //   +0x08: unordered_map<wstring,wstring>  (size varies, typically 64 bytes)
        //   +0x??: vector<wstring> m_stringsVec
        // We scan pointer-aligned offsets looking for a valid vector triple.
        for (int off = 0x08; off < 0x120; off += 8)
        {
            uintptr_t* ptrs = reinterpret_cast<uintptr_t*>(base + off);
            uintptr_t begin_ = ptrs[0];
            uintptr_t end_   = ptrs[1];
            uintptr_t cap_   = ptrs[2];

            if (begin_ == 0 || end_ == 0 || cap_ == 0) continue;
            if (begin_ > end_ || end_ > cap_) continue;

            size_t sizeBytes = end_ - begin_;
            // sizeof(std::wstring) is 32 on MSVC x64 (SSO buffer + size + capacity)
            if (sizeBytes == 0 || sizeBytes % 32 != 0) continue;

            size_t count = sizeBytes / 32;
            if (count < 50 || count > 50000) continue;

            // Quick validation: first element should be a valid wstring
            const std::wstring* first = reinterpret_cast<const std::wstring*>(begin_);
            if (first->size() > 0 && first->size() < 10000)
            {
                s_vecOffset = off;
                LogUtil::Log("[WeaveLoader] ModStrings: found m_stringsVec at StringTable+0x%X (%zu entries)",
                             off, count);
                return reinterpret_cast<std::vector<std::wstring>*>(base + off);
            }
        }
        return nullptr;
    }

    void InjectAllIntoGameTable()
    {
        if (!s_pStringTableField)
        {
            LogUtil::Log("[WeaveLoader] ModStrings: no string table ref - cannot inject");
            return;
        }

        void* stringTable = *s_pStringTableField;
        if (!stringTable)
        {
            LogUtil::Log("[WeaveLoader] ModStrings: m_stringTable pointer is NULL");
            return;
        }

        LogUtil::Log("[WeaveLoader] ModStrings: StringTable object at %p", stringTable);

        std::vector<std::wstring>* vec = FindStringsVec(stringTable);
        if (!vec)
        {
            LogUtil::Log("[WeaveLoader] ModStrings: FAILED to locate m_stringsVec in StringTable");
            return;
        }

        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_strings.empty())
        {
            LogUtil::Log("[WeaveLoader] ModStrings: no mod strings to inject");
            return;
        }

        // Find the highest ID we need
        int maxId = 0;
        for (auto& kv : s_strings)
            if (kv.first > maxId) maxId = kv.first;

        size_t oldSize = vec->size();
        if (static_cast<size_t>(maxId) >= oldSize)
        {
            vec->resize(maxId + 1);
            LogUtil::Log("[WeaveLoader] ModStrings: resized m_stringsVec %zu -> %zu", oldSize, vec->size());
        }

        int count = 0;
        for (auto& kv : s_strings)
        {
            (*vec)[kv.first] = kv.second;
            count++;
        }

        LogUtil::Log("[WeaveLoader] ModStrings: injected %d mod strings into game string table", count);
    }
}
