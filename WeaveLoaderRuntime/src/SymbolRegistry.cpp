#include "SymbolRegistry.h"
#include "LogUtil.h"
#include <Windows.h>
#include <fstream>

namespace
{
    bool ReadFile(const char* path, std::string& out)
    {
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in.is_open())
            return false;
        in.seekg(0, std::ios::end);
        const std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);
        if (size <= 0)
            return false;
        out.resize(static_cast<size_t>(size));
        in.read(&out[0], size);
        return true;
    }

    size_t SkipWhitespace(const std::string& s, size_t pos)
    {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\n' || s[pos] == '\r' || s[pos] == '\t'))
            ++pos;
        return pos;
    }

    bool ExtractStringField(const std::string& s, size_t start, size_t end, const char* key, std::string& out)
    {
        const std::string needle = std::string("\"") + key + "\"";
        size_t pos = s.find(needle, start);
        if (pos == std::string::npos || pos >= end)
            return false;
        pos = s.find(':', pos);
        if (pos == std::string::npos || pos >= end)
            return false;
        pos = SkipWhitespace(s, pos + 1);
        if (pos >= end || s[pos] != '"')
            return false;
        ++pos;
        std::string value;
        while (pos < end)
        {
            char c = s[pos++];
            if (c == '\\' && pos < end)
            {
                char esc = s[pos++];
                switch (esc)
                {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                default: value.push_back(esc); break;
                }
                continue;
            }
            if (c == '"')
                break;
            value.push_back(c);
        }
        out = std::move(value);
        return true;
    }

    bool ExtractNumberField(const std::string& s, size_t start, size_t end, const char* key, uint32_t& out)
    {
        const std::string needle = std::string("\"") + key + "\"";
        size_t pos = s.find(needle, start);
        if (pos == std::string::npos || pos >= end)
            return false;
        pos = s.find(':', pos);
        if (pos == std::string::npos || pos >= end)
            return false;
        pos = SkipWhitespace(s, pos + 1);
        if (pos >= end)
            return false;
        uint32_t value = 0;
        while (pos < end && s[pos] >= '0' && s[pos] <= '9')
        {
            value = value * 10 + static_cast<uint32_t>(s[pos] - '0');
            ++pos;
        }
        out = value;
        return true;
    }

    bool ExtractBoolField(const std::string& s, size_t start, size_t end, const char* key, bool& out)
    {
        const std::string needle = std::string("\"") + key + "\"";
        size_t pos = s.find(needle, start);
        if (pos == std::string::npos || pos >= end)
            return false;
        pos = s.find(':', pos);
        if (pos == std::string::npos || pos >= end)
            return false;
        pos = SkipWhitespace(s, pos + 1);
        if (pos + 4 <= end && s.compare(pos, 4, "true") == 0)
        {
            out = true;
            return true;
        }
        if (pos + 5 <= end && s.compare(pos, 5, "false") == 0)
        {
            out = false;
            return true;
        }
        return false;
    }
}

SymbolRegistry& SymbolRegistry::Instance()
{
    static SymbolRegistry instance;
    return instance;
}

bool SymbolRegistry::LoadFromFile(const char* path)
{
    if (!path || !path[0])
        return false;

    std::string data;
    if (!ReadFile(path, data))
    {
        LogUtil::Log("[WeaveLoader] SymbolRegistry: failed to read mapping '%s'", path);
        return false;
    }

    m_entries.clear();
    m_moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    if (!m_moduleBase)
    {
        LogUtil::Log("[WeaveLoader] SymbolRegistry: failed to get module base");
        return false;
    }

    size_t pos = 0;
    size_t count = 0;
    while (true)
    {
        const size_t fullPos = data.find("\"fullName\"", pos);
        if (fullPos == std::string::npos)
            break;

        const size_t objEnd = data.find('}', fullPos);
        if (objEnd == std::string::npos)
            break;

        std::string fullName;
        uint32_t rva = 0;
        std::string sig;

        if (ExtractStringField(data, fullPos, objEnd, "fullName", fullName) &&
            ExtractNumberField(data, fullPos, objEnd, "rva", rva))
        {
            ExtractStringField(data, fullPos, objEnd, "signatureKey", sig);
            bool isStatic = false;
            ExtractBoolField(data, fullPos, objEnd, "isStatic", isStatic);
            Entry entry;
            entry.rva = rva;
            entry.address = reinterpret_cast<void*>(m_moduleBase + rva);
            entry.signatureKey = sig;
            entry.isStatic = isStatic;
            m_entries[fullName] = std::move(entry);
            ++count;
        }

        pos = objEnd + 1;
    }

    LogUtil::Log("[WeaveLoader] SymbolRegistry: loaded %zu symbol(s) from %s", count, path);
    return count > 0;
}

bool SymbolRegistry::Has(const char* fullName) const
{
    if (!fullName)
        return false;
    return m_entries.find(fullName) != m_entries.end();
}

void* SymbolRegistry::FindAddress(const char* fullName) const
{
    if (!fullName)
        return nullptr;
    auto it = m_entries.find(fullName);
    if (it == m_entries.end())
        return nullptr;
    return it->second.address;
}

const char* SymbolRegistry::FindSignatureKey(const char* fullName) const
{
    if (!fullName)
        return nullptr;
    auto it = m_entries.find(fullName);
    if (it == m_entries.end())
        return nullptr;
    return it->second.signatureKey.c_str();
}

const SymbolRegistry::Entry* SymbolRegistry::FindEntry(const char* fullName) const
{
    if (!fullName)
        return nullptr;
    auto it = m_entries.find(fullName);
    if (it == m_entries.end())
        return nullptr;
    return &it->second;
}
