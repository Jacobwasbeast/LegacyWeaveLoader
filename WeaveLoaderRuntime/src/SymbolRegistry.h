#pragma once

#include <string>
#include <unordered_map>

class SymbolRegistry
{
public:
    struct Entry
    {
        uint32_t rva = 0;
        void* address = nullptr;
        std::string signatureKey;
        bool isStatic = false;
    };

    static SymbolRegistry& Instance();

    bool LoadFromFile(const char* path);
    bool Has(const char* fullName) const;
    void* FindAddress(const char* fullName) const;
    const char* FindSignatureKey(const char* fullName) const;
    const Entry* FindEntry(const char* fullName) const;

private:
    std::unordered_map<std::string, Entry> m_entries;
    uintptr_t m_moduleBase = 0;
};
