cmake_minimum_required(VERSION 3.24)
project(LegacyForgeRuntime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── MinHook (fetched from GitHub) ──────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageworthy/minhook.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(minhook)

# ── Locate .NET hosting headers and libs ───────────────────────────────
# Users can set DOTNET_ROOT or NETHOST_DIR; otherwise we try the default SDK path.
if(NOT DEFINED NETHOST_INCLUDE_DIR)
    # Try to find nethost from the .NET SDK installation
    file(GLOB NETHOST_CANDIDATES
        "$ENV{ProgramFiles}/dotnet/packs/Microsoft.NETCore.App.Host.win-x64/*/runtimes/win-x64/native"
        "$ENV{DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.win-x64/*/runtimes/win-x64/native"
    )
    list(SORT NETHOST_CANDIDATES ORDER DESCENDING)
    if(NETHOST_CANDIDATES)
        list(GET NETHOST_CANDIDATES 0 NETHOST_DIR)
    endif()
endif()

if(NETHOST_DIR)
    message(STATUS "Using nethost from: ${NETHOST_DIR}")
    set(NETHOST_INCLUDE_DIR "${NETHOST_DIR}")
    set(NETHOST_LIB "${NETHOST_DIR}/nethost.lib")
else()
    message(STATUS "nethost not found automatically. Set NETHOST_DIR to the directory containing nethost.h and nethost.lib")
    # Fall back to bundled headers if present
    set(NETHOST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set(NETHOST_LIB "")
endif()

# ── Runtime DLL target ─────────────────────────────────────────────────
add_library(LegacyForgeRuntime SHARED
    src/dllmain.cpp
    src/SymbolResolver.cpp
    src/HookManager.cpp
    src/GameHooks.cpp
    src/DotNetHost.cpp
    src/IdRegistry.cpp
    src/NativeExports.cpp
)

target_include_directories(LegacyForgeRuntime PRIVATE
    "${NETHOST_INCLUDE_DIR}"
)

target_link_libraries(LegacyForgeRuntime PRIVATE
    minhook
    dbghelp
)

if(EXISTS "${NETHOST_LIB}")
    target_link_libraries(LegacyForgeRuntime PRIVATE "${NETHOST_LIB}")
else()
    message(WARNING "nethost.lib not found. You may need to set NETHOST_DIR.")
endif()

target_compile_definitions(LegacyForgeRuntime PRIVATE
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(LegacyForgeRuntime PRIVATE /W3 /MP)
endif()

set_target_properties(LegacyForgeRuntime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
