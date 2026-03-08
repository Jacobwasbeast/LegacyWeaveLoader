#include "DotNetHost.h"
#include "LogUtil.h"

#include <windows.h>
#include <nethost.h>
#include <hostfxr.h>
#include <coreclr_delegates.h>

#include <cstring>
#include <string>

static hostfxr_initialize_for_runtime_config_fn init_fptr = nullptr;
static hostfxr_get_runtime_delegate_fn get_delegate_fptr = nullptr;
static hostfxr_close_fn close_fptr = nullptr;

typedef int (CORECLR_DELEGATE_CALLTYPE *managed_entry_fn)(void* args, int sizeBytes);

static managed_entry_fn fn_Initialize = nullptr;
static managed_entry_fn fn_DiscoverMods = nullptr;
static managed_entry_fn fn_PreInit = nullptr;
static managed_entry_fn fn_Init = nullptr;
static managed_entry_fn fn_PostInit = nullptr;
static managed_entry_fn fn_Tick = nullptr;
static managed_entry_fn fn_Shutdown = nullptr;
static managed_entry_fn fn_ItemMineBlock = nullptr;
static managed_entry_fn fn_ItemUse = nullptr;
static managed_entry_fn fn_BlockOnPlace = nullptr;
static managed_entry_fn fn_BlockNeighborChanged = nullptr;
static managed_entry_fn fn_BlockTick = nullptr;
static managed_entry_fn fn_EntitySummoned = nullptr;

static bool LoadHostfxr()
{
    char_t buffer[MAX_PATH];
    size_t buffer_size = sizeof(buffer) / sizeof(char_t);

    int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
    if (rc != 0)
    {
        LogUtil::Log("[WeaveLoader] get_hostfxr_path failed: 0x%x", rc);
        return false;
    }

    HMODULE lib = LoadLibraryW(buffer);
    if (!lib)
    {
        LogUtil::Log("[WeaveLoader] Failed to load hostfxr");
        return false;
    }

    init_fptr = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
        GetProcAddress(lib, "hostfxr_initialize_for_runtime_config"));
    get_delegate_fptr = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
        GetProcAddress(lib, "hostfxr_get_runtime_delegate"));
    close_fptr = reinterpret_cast<hostfxr_close_fn>(
        GetProcAddress(lib, "hostfxr_close"));

    return init_fptr && get_delegate_fptr && close_fptr;
}

static load_assembly_and_get_function_pointer_fn GetDotNetLoadAssembly(const wchar_t* configPath)
{
    hostfxr_handle cxt = nullptr;
    int rc = init_fptr(configPath, nullptr, &cxt);
    if (rc != 0 || cxt == nullptr)
    {
        char pathA[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, configPath, -1, pathA, MAX_PATH, nullptr, nullptr);
        LogUtil::Log("[WeaveLoader] hostfxr_initialize failed: 0x%x (config: %s)", rc, pathA);
        if (cxt) close_fptr(cxt);
        return nullptr;
    }

    void* load_fn = nullptr;
    rc = get_delegate_fptr(cxt, hdt_load_assembly_and_get_function_pointer, &load_fn);
    if (rc != 0 || load_fn == nullptr)
    {
        LogUtil::Log("[WeaveLoader] hostfxr_get_runtime_delegate failed: 0x%x", rc);
    }

    close_fptr(cxt);
    return reinterpret_cast<load_assembly_and_get_function_pointer_fn>(load_fn);
}

static bool ResolveManagedMethod(
    load_assembly_and_get_function_pointer_fn load_fn,
    const wchar_t* assemblyPath,
    const wchar_t* methodName,
    managed_entry_fn* outFn)
{
    int rc = load_fn(
        assemblyPath,
        L"WeaveLoader.Core.WeaveLoaderCore, WeaveLoader.Core",
        methodName,
        nullptr,
        nullptr,
        reinterpret_cast<void**>(outFn));

    if (rc != 0)
    {
        char nameA[64];
        WideCharToMultiByte(CP_UTF8, 0, methodName, -1, nameA, sizeof(nameA), nullptr, nullptr);
        LogUtil::Log("[WeaveLoader] load_assembly_and_get_function_pointer('%s') failed: 0x%x", nameA, rc);
    }
    return rc == 0 && *outFn != nullptr;
}

bool DotNetHost::Initialize()
{
    if (!LoadHostfxr())
    {
        LogUtil::Log("[WeaveLoader] Failed to load hostfxr library");
        return false;
    }

    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

    std::wstring exeDir(modulePath);
    size_t lastSlash = exeDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
        exeDir = exeDir.substr(0, lastSlash + 1);
    else
        exeDir = L".\\";

    HMODULE hSelf = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&DotNetHost::Initialize), &hSelf);

    wchar_t selfPath[MAX_PATH];
    GetModuleFileNameW(hSelf, selfPath, MAX_PATH);
    std::wstring selfDir(selfPath);
    lastSlash = selfDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
        selfDir = selfDir.substr(0, lastSlash + 1);
    else
        selfDir = L".\\";

    std::wstring configPath = selfDir + L"WeaveLoader.Core.runtimeconfig.json";
    std::wstring assemblyPath = selfDir + L"WeaveLoader.Core.dll";

    if (GetFileAttributesW(assemblyPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        char pathA[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, assemblyPath.c_str(), -1, pathA, MAX_PATH, nullptr, nullptr);
        LogUtil::Log("[WeaveLoader] WeaveLoader.Core.dll not found at: %s", pathA);
        LogUtil::Log("[WeaveLoader] Copy WeaveLoader.Core.dll, WeaveLoader.API.dll, and "
                     "WeaveLoader.Core.runtimeconfig.json to the same folder as WeaveLoaderRuntime.dll");
        return false;
    }
    if (GetFileAttributesW(configPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        char pathA[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, configPath.c_str(), -1, pathA, MAX_PATH, nullptr, nullptr);
        LogUtil::Log("[WeaveLoader] WeaveLoader.Core.runtimeconfig.json not found at: %s", pathA);
        return false;
    }

    auto load_fn = GetDotNetLoadAssembly(configPath.c_str());
    if (!load_fn)
    {
        LogUtil::Log("[WeaveLoader] Failed to get load_assembly_and_get_function_pointer");
        return false;
    }

    auto resolve = [&](const wchar_t* name, managed_entry_fn* out) -> bool {
        bool r = ResolveManagedMethod(load_fn, assemblyPath.c_str(), name, out);
        if (!r)
        {
            char nameA[64];
            WideCharToMultiByte(CP_UTF8, 0, name, -1, nameA, sizeof(nameA), nullptr, nullptr);
            LogUtil::Log("[WeaveLoader] Failed to resolve: %s", nameA);
        }
        return r;
    };

    bool ok = true;
    ok &= resolve(L"Initialize", &fn_Initialize);
    ok &= resolve(L"DiscoverMods", &fn_DiscoverMods);
    ok &= resolve(L"PreInit", &fn_PreInit);
    ok &= resolve(L"Init", &fn_Init);
    ok &= resolve(L"PostInit", &fn_PostInit);
    ok &= resolve(L"Tick", &fn_Tick);
    ok &= resolve(L"Shutdown", &fn_Shutdown);
    ok &= resolve(L"OnItemMineBlock", &fn_ItemMineBlock);
    ok &= resolve(L"OnItemUse", &fn_ItemUse);
    ok &= resolve(L"OnBlockPlace", &fn_BlockOnPlace);
    ok &= resolve(L"OnBlockNeighborChanged", &fn_BlockNeighborChanged);
    ok &= resolve(L"OnBlockTick", &fn_BlockTick);
    ok &= resolve(L"OnEntitySummoned", &fn_EntitySummoned);

    if (!ok)
    {
        LogUtil::Log("[WeaveLoader] Failed to resolve one or more managed entry points. "
                     "Ensure .NET 8 runtime is installed and WeaveLoader.Core.dll is built for net8.0.");
        return false;
    }

    LogUtil::Log("[WeaveLoader] All managed entry points resolved");
    return true;
}

void DotNetHost::CallManagedInit()
{
    if (fn_Initialize)
        fn_Initialize(nullptr, 0);
}

int DotNetHost::CallDiscoverMods(const char* modsPath)
{
    if (fn_DiscoverMods)
        return fn_DiscoverMods(const_cast<char*>(modsPath), static_cast<int>(strlen(modsPath)));
    return 0;
}

void DotNetHost::CallPreInit()
{
    if (fn_PreInit) fn_PreInit(nullptr, 0);
}

void DotNetHost::CallInit()
{
    if (fn_Init) fn_Init(nullptr, 0);
}

void DotNetHost::CallPostInit()
{
    if (fn_PostInit) fn_PostInit(nullptr, 0);
}

void DotNetHost::CallTick()
{
    if (fn_Tick) fn_Tick(nullptr, 0);
}

void DotNetHost::CallShutdown()
{
    if (fn_Shutdown) fn_Shutdown(nullptr, 0);
}

int DotNetHost::CallItemMineBlock(const void* args, int sizeBytes)
{
    if (!fn_ItemMineBlock || !args || sizeBytes <= 0)
        return 0;
    return fn_ItemMineBlock(const_cast<void*>(args), sizeBytes);
}

int DotNetHost::CallItemUse(const void* args, int sizeBytes)
{
    if (!fn_ItemUse || !args || sizeBytes <= 0)
        return 0;
    return fn_ItemUse(const_cast<void*>(args), sizeBytes);
}

int DotNetHost::CallBlockOnPlace(const void* args, int sizeBytes)
{
    if (!fn_BlockOnPlace || !args || sizeBytes <= 0)
        return 0;
    return fn_BlockOnPlace(const_cast<void*>(args), sizeBytes);
}

int DotNetHost::CallBlockNeighborChanged(const void* args, int sizeBytes)
{
    if (!fn_BlockNeighborChanged || !args || sizeBytes <= 0)
        return 0;
    return fn_BlockNeighborChanged(const_cast<void*>(args), sizeBytes);
}

int DotNetHost::CallBlockTick(const void* args, int sizeBytes)
{
    if (!fn_BlockTick || !args || sizeBytes <= 0)
        return 0;
    return fn_BlockTick(const_cast<void*>(args), sizeBytes);
}

void DotNetHost::CallEntitySummoned(int entityNumericId, float x, float y, float z)
{
    if (!fn_EntitySummoned)
        return;

    struct EntitySummonedNativeArgs
    {
        int entityNumericId;
        float x;
        float y;
        float z;
    } native{ entityNumericId, x, y, z };

    fn_EntitySummoned(&native, sizeof(native));
}

void DotNetHost::Cleanup()
{
}
