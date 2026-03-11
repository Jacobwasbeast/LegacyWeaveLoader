#include <Windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "LogUtil.h"
#include "CrashHandler.h"
#include "PdbParser.h"
#include "SymbolResolver.h"
#include "SymbolRegistry.h"
#include "HookManager.h"
#include "DotNetHost.h"
#include "MainMenuOverlay.h"

static HMODULE g_hModule = nullptr;

static std::string GetDllDirectory(HMODULE hModule)
{
    char path[MAX_PATH] = {0};
    GetModuleFileNameA(hModule, path, MAX_PATH);
    std::string s(path);
    size_t pos = s.find_last_of("\\/");
    if (pos != std::string::npos)
        return s.substr(0, pos + 1);
    return ".\\";
}

static bool ReadFileToString(const std::string& path, std::string& out)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in.is_open())
        return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

static bool ComputeSha256File(const char* path, std::string& outHex)
{
    if (!path || !*path)
        return false;
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    DWORD hashObjectSize = 0;
    DWORD hashSize = 0;
    DWORD cbData = 0;
    std::vector<unsigned char> hashObject;
    std::vector<unsigned char> hashBytes;

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
    {
        CloseHandle(hFile);
        return false;
    }

    if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&hashObjectSize),
                          sizeof(DWORD), &cbData, 0) != 0 ||
        BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashSize),
                          sizeof(DWORD), &cbData, 0) != 0)
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        CloseHandle(hFile);
        return false;
    }

    hashObject.resize(hashObjectSize);
    hashBytes.resize(hashSize);
    if (BCryptCreateHash(hAlg, &hHash, hashObject.data(), hashObjectSize, nullptr, 0, 0) != 0)
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        CloseHandle(hFile);
        return false;
    }

    unsigned char buffer[1 << 16];
    DWORD bytesRead = 0;
    while (ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
    {
        if (BCryptHashData(hHash, buffer, bytesRead, 0) != 0)
        {
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            CloseHandle(hFile);
            return false;
        }
    }

    if (BCryptFinishHash(hHash, hashBytes.data(), hashSize, 0) != 0)
    {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        CloseHandle(hFile);
        return false;
    }

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    CloseHandle(hFile);

    std::ostringstream oss;
    oss.setf(std::ios::hex, std::ios::basefield);
    oss.fill('0');
    for (unsigned char b : hashBytes)
        oss << std::setw(2) << static_cast<int>(b);
    outHex = oss.str();
    return true;
}

static bool ExtractGameExeSha256(const std::string& json, std::string& outSha)
{
    const std::string gameExeKey = "\"gameExe\"";
    const std::string shaKey = "\"sha256\"";
    size_t gamePos = json.find(gameExeKey);
    if (gamePos == std::string::npos)
        return false;
    size_t shaPos = json.find(shaKey, gamePos);
    if (shaPos == std::string::npos)
        return false;
    size_t colon = json.find(':', shaPos);
    if (colon == std::string::npos)
        return false;
    size_t quote1 = json.find('"', colon + 1);
    if (quote1 == std::string::npos)
        return false;
    size_t quote2 = json.find('"', quote1 + 1);
    if (quote2 == std::string::npos || quote2 <= quote1 + 1)
        return false;
    outSha = json.substr(quote1 + 1, quote2 - quote1 - 1);
    return !outSha.empty();
}

static std::string ToLowerAscii(std::string value)
{
    for (char& c : value)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return value;
}

static void RemoveStaleMetadata(const std::string& baseDir)
{
    const std::string metaDir = baseDir + "metadata\\";
    const char* files[] = {
        "mapping.json",
        "offsets.json",
        "metadata.json"
    };
    for (const char* name : files)
    {
        std::string path = metaDir + name;
        if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            if (DeleteFileA(path.c_str()) == 0)
                LogUtil::Log("[WeaveLoader] Warning: failed to delete stale %s", path.c_str());
            else
                LogUtil::Log("[WeaveLoader] Deleted stale %s", path.c_str());
        }
    }

    const char* rootFiles[] = {
        "mapping.json",
        "offsets.json"
    };
    for (const char* name : rootFiles)
    {
        std::string path = baseDir + name;
        if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            if (DeleteFileA(path.c_str()) == 0)
                LogUtil::Log("[WeaveLoader] Warning: failed to delete stale %s", path.c_str());
            else
                LogUtil::Log("[WeaveLoader] Deleted stale %s", path.c_str());
        }
    }
}

static void ValidateMetadataForExecutable(const std::string& baseDir, const char* exePath)
{
    const std::string metadataPath = baseDir + "metadata\\metadata.json";
    if (GetFileAttributesA(metadataPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        return;

    std::string json;
    if (!ReadFileToString(metadataPath, json))
        return;

    std::string expectedSha;
    if (!ExtractGameExeSha256(json, expectedSha))
        return;

    std::string actualSha;
    if (!ComputeSha256File(exePath, actualSha))
        return;

    if (ToLowerAscii(expectedSha) != ToLowerAscii(actualSha))
    {
        LogUtil::Log("[WeaveLoader] Metadata mismatch: game executable hash does not match metadata.json. Removing stale mappings/offsets.");
        RemoveStaleMetadata(baseDir);
    }
}

DWORD WINAPI InitThread(LPVOID lpParam)
{
    LogUtil::Log("[WeaveLoader] InitThread started (module=%p)", g_hModule);
#ifdef WEAVELOADER_DEBUG_BUILD
    LogUtil::Log("[WeaveLoader] Loader is running in debug mode");
#endif

    std::string baseDir = GetDllDirectory(g_hModule);
    LogUtil::Log("[WeaveLoader] Runtime DLL directory: %s", baseDir.c_str());

    char cwd[MAX_PATH] = {0};
    GetCurrentDirectoryA(MAX_PATH, cwd);
    LogUtil::Log("[WeaveLoader] Game working directory: %s", cwd);

    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    LogUtil::Log("[WeaveLoader] Host executable: %s", exePath);

    ValidateMetadataForExecutable(baseDir, exePath);

    std::string mappingPath = baseDir + "metadata\\mapping.json";
    if (!SymbolRegistry::Instance().LoadFromFile(mappingPath.c_str()))
    {
        std::string fallbackPath = baseDir + "mapping.json";
        SymbolRegistry::Instance().LoadFromFile(fallbackPath.c_str());
    }

    SymbolResolver symbols;
    if (!symbols.Initialize())
    {
        LogUtil::Log("[WeaveLoader] ERROR: Failed to initialize symbol resolver. Is the PDB present?");
        return 1;
    }
    LogUtil::Log("[WeaveLoader] Symbol resolver initialized");

    if (!symbols.ResolveGameFunctions())
    {
        LogUtil::Log("[WeaveLoader] ERROR: Failed to resolve critical game functions.");
        return 1;
    }
    LogUtil::Log("[WeaveLoader] Game functions resolved from PDB");

    if (!HookManager::Install(symbols))
    {
        LogUtil::Log("[WeaveLoader] ERROR: Failed to install hooks");
        symbols.Cleanup();
        return 1;
    }
    LogUtil::Log("[WeaveLoader] Hooks installed");

    // Build the RVA->name index before releasing the PDB.
    // This index survives PdbParser::Close() and is used by the crash handler.
    PdbParser::BuildAddressIndex();
    CrashHandler::SetGameBase(reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)));

    symbols.Cleanup();

    if (!DotNetHost::Initialize())
    {
        LogUtil::Log("[WeaveLoader] ERROR: Failed to initialize .NET host");
        return 1;
    }
    LogUtil::Log("[WeaveLoader] .NET runtime initialized");

    std::string modsPath = baseDir + "mods";
    LogUtil::Log("[WeaveLoader] Mods directory: %s", modsPath.c_str());

    DotNetHost::CallManagedInit();
    int modCount = DotNetHost::CallDiscoverMods(modsPath.c_str());
    MainMenuOverlay::SetModCount(modCount);
    LogUtil::Log("[WeaveLoader] Mod discovery complete (%d mods). Ready.", modCount);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        // Set up logging and crash handler BEFORE anything else.
        // These must work immediately so we catch crashes during init.
        {
            std::string baseDir = GetDllDirectory(hModule);
            LogUtil::SetBaseDir(baseDir.c_str());
            CrashHandler::Install(hModule);
        }

        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        HookManager::Cleanup();
        break;
    }
    return TRUE;
}
