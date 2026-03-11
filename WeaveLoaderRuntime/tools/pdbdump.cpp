#include "PdbParser.h"
#include <Windows.h>
#include <bcrypt.h>
#include <DbgHelp.h>
#include <OleAuto.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
#ifndef SymTagData
    constexpr DWORD SymTagData = 7;
#endif
    struct FileFingerprint
    {
        std::string path;
        uint64_t size = 0;
        uint64_t mtime = 0;
        std::string sha256;
        bool ok = false;
    };

    uint64_t FileTimeToUint64(const FILETIME& ft)
    {
        return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    }
    std::string Trim(const std::string& s)
    {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
            ++start;
        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
            --end;
        return s.substr(start, end - start);
    }

    std::string ToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    void CleanName(std::string& s)
    {
        s.erase(std::remove_if(s.begin(), s.end(),
            [](unsigned char c)
            {
                return c == '\0' || c < 0x20;
            }), s.end());
        s = Trim(s);
    }

    bool Contains(const std::string& s, const char* needle)
    {
        return s.find(needle) != std::string::npos;
    }

    std::string Demangle(const std::string& decorated)
    {
        char buffer[2048] = {};
        if (UnDecorateSymbolName(decorated.c_str(), buffer, sizeof(buffer), UNDNAME_COMPLETE) == 0)
            return decorated;
        return std::string(buffer);
    }

    bool ComputeSha256(HANDLE file, std::string& outHex)
    {
        if (file == INVALID_HANDLE_VALUE)
            return false;
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        DWORD hashObjectSize = 0;
        DWORD hashSize = 0;
        DWORD cbData = 0;
        std::vector<unsigned char> hashObject;
        std::vector<unsigned char> hashBytes;

        if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
            return false;

        if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&hashObjectSize),
                              sizeof(DWORD), &cbData, 0) != 0)
        {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }
        if (BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashSize),
                              sizeof(DWORD), &cbData, 0) != 0)
        {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        hashObject.resize(hashObjectSize);
        hashBytes.resize(hashSize);

        if (BCryptCreateHash(hAlg, &hHash, hashObject.data(), hashObjectSize, nullptr, 0, 0) != 0)
        {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        if (SetFilePointer(file, 0, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        unsigned char buffer[1 << 16];
        DWORD bytesRead = 0;
        while (ReadFile(file, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
        {
            if (BCryptHashData(hHash, buffer, bytesRead, 0) != 0)
            {
                BCryptDestroyHash(hHash);
                BCryptCloseAlgorithmProvider(hAlg, 0);
                return false;
            }
        }

        if (BCryptFinishHash(hHash, hashBytes.data(), hashSize, 0) != 0)
        {
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);

        std::ostringstream oss;
        oss.setf(std::ios::hex, std::ios::basefield);
        oss.fill('0');
        for (unsigned char b : hashBytes)
            oss << std::setw(2) << static_cast<int>(b);
        outHex = oss.str();
        return true;
    }

    bool GetFileFingerprint(const char* path, FileFingerprint& out)
    {
        if (!path || !*path)
            return false;
        out.path = path;
        HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            return false;

        LARGE_INTEGER size{};
        FILETIME ft{};
        if (!GetFileSizeEx(hFile, &size) || !GetFileTime(hFile, nullptr, nullptr, &ft))
        {
            CloseHandle(hFile);
            return false;
        }

        out.size = static_cast<uint64_t>(size.QuadPart);
        out.mtime = FileTimeToUint64(ft);
        out.ok = ComputeSha256(hFile, out.sha256);
        CloseHandle(hFile);
        return out.ok;
    }

    std::string GetDirName(const std::string& path)
    {
        const size_t pos = path.find_last_of("\\/");
        if (pos == std::string::npos)
            return std::string();
        return path.substr(0, pos);
    }

    struct TypeInfo
    {
        std::string raw;
        std::string canonical;
        char token = 'u';
        bool callable = false;
        bool hasRef = false;
    };

    TypeInfo NormalizeType(const std::string& input)
    {
        TypeInfo info;
        info.raw = Trim(input);
        std::string t = info.raw;
        if (t.empty())
            return info;

        if (t.find('&') != std::string::npos)
            info.hasRef = true;

        bool isPtr = t.find('*') != std::string::npos;
        std::string lower = ToLower(t);

        auto stripToken = [&](const char* token)
        {
            size_t pos = lower.find(token);
            if (pos != std::string::npos)
            {
                t.erase(pos, strlen(token));
                lower.erase(pos, strlen(token));
            }
        };

        const char* qualifiers[] = {
            "const", "volatile", "class", "struct", "enum", "__ptr64", "__cdecl",
            "__thiscall", "__stdcall", "__vectorcall", "__fastcall"
        };
        for (const char* q : qualifiers)
            stripToken(q);

        t = Trim(t);
        lower = ToLower(t);

        if (isPtr || Contains(lower, "*"))
        {
            info.canonical = "ptr";
            info.token = 'p';
            info.callable = !info.hasRef;
            return info;
        }

        if (Contains(lower, "bool"))
        {
            info.canonical = "bool";
            info.token = 'b';
            info.callable = !info.hasRef;
            return info;
        }
        if (Contains(lower, "float"))
        {
            info.canonical = "float";
            info.token = 'f';
            info.callable = !info.hasRef;
            return info;
        }
        if (Contains(lower, "double"))
        {
            info.canonical = "double";
            info.token = 'd';
            info.callable = !info.hasRef;
            return info;
        }
        if (Contains(lower, "long long") || Contains(lower, "__int64") || Contains(lower, "int64"))
        {
            info.canonical = "int64";
            info.token = 'l';
            info.callable = !info.hasRef;
            return info;
        }
        if (Contains(lower, "int") || Contains(lower, "short") || Contains(lower, "char") || Contains(lower, "long"))
        {
            info.canonical = "int";
            info.token = 'i';
            info.callable = !info.hasRef;
            return info;
        }
        if (Contains(lower, "void"))
        {
            info.canonical = "void";
            info.token = 'v';
            info.callable = true;
            return info;
        }

        info.canonical = Trim(t);
        info.token = 'u';
        info.callable = false;
        return info;
    }

    std::vector<std::string> SplitParams(const std::string& params)
    {
        std::vector<std::string> out;
        std::string current;
        int depth = 0;
        for (size_t i = 0; i < params.size(); ++i)
        {
            char c = params[i];
            if (c == '<')
                ++depth;
            else if (c == '>' && depth > 0)
                --depth;

            if (c == ',' && depth == 0)
            {
                out.push_back(Trim(current));
                current.clear();
                continue;
            }
            current.push_back(c);
        }
        if (!current.empty())
            out.push_back(Trim(current));
        return out;
    }

    struct MethodInfo
    {
        std::string className;
        std::string methodName;
        std::string fullName;
        std::string decorated;
        std::string undecorated;
        uint32_t rva = 0;
        bool isStatic = false;
        bool isVirtual = false;
        TypeInfo returnType;
        std::vector<TypeInfo> params;
        std::string signatureKey;
        char tier = 'B';
    };

    std::string ExtractReturnType(const std::string& undecorated, size_t methodPos)
    {
        std::string prefix = Trim(undecorated.substr(0, methodPos));
        if (prefix.empty())
            return "void";

        std::vector<std::string> tokens;
        {
            std::istringstream iss(prefix);
            std::string tok;
            while (iss >> tok)
                tokens.push_back(tok);
        }

        auto isQualifier = [](const std::string& tok)
        {
            static const char* kQualifiers[] = {
                "public:", "private:", "protected:", "static", "virtual", "inline",
                "__cdecl", "__thiscall", "__stdcall", "__vectorcall", "__fastcall"
            };
            for (const char* q : kQualifiers)
                if (tok == q)
                    return true;
            return false;
        };

        std::string result;
        for (const std::string& tok : tokens)
        {
            if (isQualifier(tok))
                continue;
            if (!result.empty())
                result.push_back(' ');
            result += tok;
        }

        if (result.empty())
            return "void";
        return result;
    }

    MethodInfo BuildMethod(const PdbParser::SymbolInfo& sym)
    {
        MethodInfo info;
        info.decorated = sym.name;
        info.rva = sym.rva;
        info.undecorated = Demangle(info.decorated);

        const size_t parenPos = info.undecorated.find('(');
        const size_t closePos = info.undecorated.rfind(')');
        if (parenPos == std::string::npos || closePos == std::string::npos || closePos <= parenPos)
            return info;

        const std::string beforeParen = info.undecorated.substr(0, parenPos);
        const size_t classPos = beforeParen.rfind("::");
        if (classPos != std::string::npos)
        {
            size_t nameStart = classPos + 2;
            info.methodName = beforeParen.substr(nameStart);
            size_t classStart = beforeParen.rfind(' ', classPos);
            if (classStart == std::string::npos)
                classStart = 0;
            else
                classStart += 1;
            info.className = beforeParen.substr(classStart, classPos - classStart);
        }
        else
        {
            info.className = "Global";
            info.methodName = Trim(beforeParen);
        }

        info.fullName = info.className + "::" + info.methodName;
        info.isStatic = Contains(info.undecorated, " static ");
        info.isVirtual = Contains(info.undecorated, " virtual ");

        std::string returnStr = ExtractReturnType(info.undecorated, classPos == std::string::npos ? 0 : classPos);
        info.returnType = NormalizeType(returnStr);

        std::string paramsStr = info.undecorated.substr(parenPos + 1, closePos - parenPos - 1);
        paramsStr = Trim(paramsStr);
        if (!paramsStr.empty() && paramsStr != "void")
        {
            for (const std::string& p : SplitParams(paramsStr))
            {
                if (p.empty())
                    continue;
                info.params.push_back(NormalizeType(p));
            }
        }

        std::ostringstream sig;
        sig << info.returnType.token;
        for (const TypeInfo& p : info.params)
            sig << "_" << p.token;
        info.signatureKey = sig.str();

        bool ok = info.returnType.callable;
        for (const TypeInfo& p : info.params)
            ok = ok && p.callable;
        info.tier = ok ? 'A' : 'B';
        return info;
    }

    void WriteJsonString(std::ostream& out, const std::string& s)
    {
        out << '"';
        for (char c : s)
        {
            switch (c)
            {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
            }
        }
        out << '"';
    }

    void WriteFingerprint(std::ostream& out, const char* label, const char* path, const FileFingerprint& fp)
    {
        out << "  \"" << label << "\": {\n";
        out << "    \"path\": ";
        WriteJsonString(out, path ? path : "");
        out << ",\n";
        out << "    \"sha256\": ";
        WriteJsonString(out, fp.ok ? fp.sha256 : "");
        out << ",\n";
        out << "    \"size\": " << (fp.ok ? fp.size : 0) << ",\n";
        out << "    \"mtime\": " << (fp.ok ? fp.mtime : 0) << ",\n";
        out << "    \"ok\": " << (fp.ok ? "true" : "false") << "\n";
        out << "  }";
    }

    bool WriteMetadataJson(const std::string& outPath,
                           const char* pdbPath,
                           const char* exePath,
                           const char* mappingPath,
                           const char* offsetsPath)
    {
        std::ofstream out(outPath, std::ios::out | std::ios::trunc);
        if (!out.is_open())
            return false;

        FileFingerprint pdbFp{};
        FileFingerprint exeFp{};
        if (pdbPath)
            GetFileFingerprint(pdbPath, pdbFp);
        if (exePath)
            GetFileFingerprint(exePath, exeFp);

        out << "{\n";
        out << "  \"format\": 1,\n";
        out << "  \"mappingJson\": ";
        WriteJsonString(out, mappingPath ? mappingPath : "");
        out << ",\n";
        out << "  \"offsetsJson\": ";
        WriteJsonString(out, offsetsPath ? offsetsPath : "");
        out << ",\n";
        WriteFingerprint(out, "pdb", pdbPath, pdbFp);
        if (exePath && *exePath)
        {
            out << ",\n";
            WriteFingerprint(out, "gameExe", exePath, exeFp);
        }
        out << "\n}\n";
        return true;
    }
}

struct EnumTypesContext
{
    std::vector<std::string>* outTypes = nullptr;
};

static BOOL CALLBACK EnumTypesCallback(PSYMBOL_INFO symInfo, ULONG, void* userContext)
{
    if (!symInfo || !userContext)
        return TRUE;
    auto* ctx = reinterpret_cast<EnumTypesContext*>(userContext);
    if (!ctx->outTypes)
        return TRUE;
    if (symInfo->NameLen > 0)
    {
        std::string name(symInfo->Name, symInfo->NameLen);
        CleanName(name);
        if (!name.empty())
            ctx->outTypes->push_back(std::move(name));
    }
    return TRUE;
}

static bool WriteOffsetsJson(const char* exePath, const char* outPath, const std::vector<std::string>& types, bool allTypes)
{
    if (!exePath || !outPath)
        return false;

    HANDLE proc = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(proc, nullptr, FALSE))
        return false;

    DWORD64 base = SymLoadModuleEx(proc, nullptr, exePath, nullptr, 0, 0, nullptr, 0);
    if (base == 0)
    {
        SymCleanup(proc);
        return false;
    }

    auto getTypeFields = [&](const std::string& typeName, std::vector<std::pair<std::string, ULONG>>& outFields)
    {
        outFields.clear();
        SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(calloc(1, sizeof(SYMBOL_INFO) + MAX_SYM_NAME));
        if (!sym)
            return;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = MAX_SYM_NAME;
        if (!SymGetTypeFromName(proc, base, typeName.c_str(), sym))
        {
            free(sym);
            return;
        }

        DWORD typeId = sym->TypeIndex;
        ULONG childrenCount = 0;
        if (!SymGetTypeInfo(proc, base, typeId, TI_GET_CHILDRENCOUNT, &childrenCount) || childrenCount == 0)
        {
            free(sym);
            return;
        }

        const size_t paramsSize = sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * childrenCount;
        auto* params = reinterpret_cast<TI_FINDCHILDREN_PARAMS*>(calloc(1, paramsSize));
        if (!params)
        {
            free(sym);
            return;
        }

        params->Count = childrenCount;
        params->Start = 0;
        if (!SymGetTypeInfo(proc, base, typeId, TI_FINDCHILDREN, params))
        {
            free(params);
            free(sym);
            return;
        }

        for (ULONG i = 0; i < childrenCount; ++i)
        {
            DWORD childId = params->ChildId[i];
            DWORD tag = 0;
            if (!SymGetTypeInfo(proc, base, childId, TI_GET_SYMTAG, &tag))
                continue;
            if (tag != SymTagData)
                continue;

            BSTR bstrName = nullptr;
            if (!SymGetTypeInfo(proc, base, childId, TI_GET_SYMNAME, &bstrName) || !bstrName)
                continue;

            ULONG offset = 0;
            if (!SymGetTypeInfo(proc, base, childId, TI_GET_OFFSET, &offset))
            {
                SysFreeString(bstrName);
                continue;
            }

            int len = WideCharToMultiByte(CP_UTF8, 0, bstrName, -1, nullptr, 0, nullptr, nullptr);
            std::string name;
            if (len > 1)
            {
                name.resize(static_cast<size_t>(len - 1));
                WideCharToMultiByte(CP_UTF8, 0, bstrName, -1, name.data(), len, nullptr, nullptr);
            }
            SysFreeString(bstrName);

            CleanName(name);
            if (!name.empty())
                outFields.emplace_back(std::move(name), offset);
        }

        free(params);
        free(sym);
    };

    std::vector<std::string> resolvedTypes = types;
    if (allTypes)
    {
        resolvedTypes.clear();
        EnumTypesContext ctx{ &resolvedTypes };
        SymEnumTypes(proc, base, EnumTypesCallback, &ctx);
        std::sort(resolvedTypes.begin(), resolvedTypes.end());
        resolvedTypes.erase(std::unique(resolvedTypes.begin(), resolvedTypes.end()), resolvedTypes.end());
    }

    std::ofstream out(outPath, std::ios::out | std::ios::trunc);
    if (!out.is_open())
    {
        SymCleanup(proc);
        return false;
    }

    out << "{\n";
    bool firstType = true;
    for (const std::string& typeName : resolvedTypes)
    {
        std::vector<std::pair<std::string, ULONG>> fields;
        getTypeFields(typeName, fields);
        if (fields.empty())
            continue;

        if (!firstType) out << ",\n";
        firstType = false;
        out << "  ";
        WriteJsonString(out, typeName);
        out << ": {\n";
        bool firstField = true;
        for (const auto& [fieldName, offset] : fields)
        {
            if (!firstField) out << ",\n";
            firstField = false;
            out << "    ";
            WriteJsonString(out, fieldName);
            out << ": " << offset;
        }
        out << "\n  }";
    }
    out << "\n}\n";

    SymCleanup(proc);
    return true;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: pdbdump <pdb_path> <out_json>\n";
        return 1;
    }

    const char* pdbPath = argv[1];
    const char* outPath = argv[2];

    const char* offsetsExe = nullptr;
    const char* offsetsOut = nullptr;
    std::vector<std::string> offsetTypes;
    bool allTypes = false;
    for (int i = 3; i < argc; ++i)
    {
        if (strcmp(argv[i], "--offsets") == 0 && i + 2 < argc)
        {
            offsetsExe = argv[++i];
            offsetsOut = argv[++i];
        }
        else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc)
        {
            offsetTypes.emplace_back(argv[++i]);
        }
        else if (strcmp(argv[i], "--all-types") == 0)
        {
            allTypes = true;
        }
    }
    if (!allTypes && offsetTypes.empty())
        allTypes = true;

    if (!PdbParser::Open(pdbPath))
    {
        std::cerr << "Failed to open PDB: " << pdbPath << "\n";
        return 1;
    }

    std::vector<PdbParser::SymbolInfo> symbols;
    if (!PdbParser::EnumerateSymbols(symbols))
    {
        std::cerr << "Failed to enumerate symbols\n";
        return 1;
    }

    std::unordered_map<std::string, std::vector<MethodInfo>> byClass;
    std::unordered_map<std::string, uint32_t> seen;
    for (const auto& sym : symbols)
    {
        if (!sym.isProc)
            continue;

        MethodInfo mi = BuildMethod(sym);
        if (mi.methodName.empty())
            continue;

        const std::string key = mi.decorated + ":" + std::to_string(mi.rva);
        if (seen.emplace(key, mi.rva).second == false)
            continue;

        byClass[mi.className].push_back(std::move(mi));
    }

    std::ofstream out(outPath, std::ios::out | std::ios::trunc);
    if (!out.is_open())
    {
        std::cerr << "Failed to open output: " << outPath << "\n";
        return 1;
    }

    out << "{\n  \"types\": [\n";
    bool firstType = true;
    for (auto& [className, methods] : byClass)
    {
        if (!firstType) out << ",\n";
        firstType = false;
        out << "    {\"name\": ";
        WriteJsonString(out, className);
        out << ", \"methods\": [\n";

        bool firstMethod = true;
        for (const MethodInfo& mi : methods)
        {
            if (!firstMethod) out << ",\n";
            firstMethod = false;

            out << "      {\"name\": ";
            WriteJsonString(out, mi.methodName);
            out << ", \"fullName\": ";
            WriteJsonString(out, mi.fullName);
            out << ", \"decorated\": ";
            WriteJsonString(out, mi.decorated);
            out << ", \"undecorated\": ";
            WriteJsonString(out, mi.undecorated);
            out << ", \"rva\": " << mi.rva;
            out << ", \"isStatic\": " << (mi.isStatic ? "true" : "false");
            out << ", \"isVirtual\": " << (mi.isVirtual ? "true" : "false");
            out << ", \"returnType\": ";
            WriteJsonString(out, mi.returnType.canonical);
            out << ", \"paramTypes\": [";
            for (size_t i = 0; i < mi.params.size(); ++i)
            {
                if (i) out << ", ";
                WriteJsonString(out, mi.params[i].canonical);
            }
            out << "]";
            out << ", \"signatureKey\": ";
            WriteJsonString(out, mi.signatureKey);
            out << ", \"callableTier\": ";
            WriteJsonString(out, std::string(1, mi.tier));
            out << "}";
        }

        out << "\n    ]}";
    }
    out << "\n  ]\n}\n";

    std::cout << "Wrote " << outPath << "\n";

    if (offsetsExe && offsetsOut)
    {
        if (WriteOffsetsJson(offsetsExe, offsetsOut, offsetTypes, allTypes))
            std::cout << "Wrote " << offsetsOut << "\n";
        else
            std::cout << "Failed to write " << offsetsOut << "\n";
    }

    const std::string outDir = GetDirName(outPath);
    const std::string metadataPath = outDir.empty()
        ? std::string("metadata.json")
        : (outDir + "\\metadata.json");
    if (WriteMetadataJson(metadataPath, pdbPath, offsetsExe, outPath, offsetsOut))
        std::cout << "Wrote " << metadataPath << "\n";
    else
        std::cout << "Failed to write " << metadataPath << "\n";
    return 0;
}
