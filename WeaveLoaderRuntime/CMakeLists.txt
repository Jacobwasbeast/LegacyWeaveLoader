cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0091 NEW)
project(WeaveLoaderRuntime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use static release CRT (/MT) for all configs to:
# 1. Avoid requiring MSVCP140.dll / VCRUNTIME140.dll / ucrtbase.dll at runtime
# 2. Match libnethost.lib which is also built with /MT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

# ── MinHook (fetched from GitHub) ──────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        v1.3.4
)
FetchContent_MakeAvailable(minhook)

# Ensure MinHook also uses static release CRT
set_target_properties(minhook PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")

# ── stb_image (PNG load/save for mod atlas) ───────────────────────────
set(STB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb")
if(NOT EXISTS "${STB_DIR}/stb_image.h")
    message(WARNING "stb_image.h not found. Run: curl -sL https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -o third_party/stb/stb_image.h")
endif()
if(NOT EXISTS "${STB_DIR}/stb_image_write.h")
    message(WARNING "stb_image_write.h not found. Run: curl -sL https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h -o third_party/stb/stb_image_write.h")
endif()

# ── raw_pdb (fallback PDB parser for Wine/Proton) ────────────────────
set(RAWPDB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raw_pdb
    GIT_REPOSITORY https://github.com/MolecularMatters/raw_pdb.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(raw_pdb)
set_target_properties(raw_pdb PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")

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
    # Prefer static lib (libnethost.lib) to avoid requiring nethost.dll at runtime
    if(EXISTS "${NETHOST_DIR}/libnethost.lib")
        set(NETHOST_LIB "${NETHOST_DIR}/libnethost.lib")
        set(NETHOST_STATIC TRUE)
        message(STATUS "Using static nethost (libnethost.lib)")
    elseif(EXISTS "${NETHOST_DIR}/nethost.lib")
        set(NETHOST_LIB "${NETHOST_DIR}/nethost.lib")
        set(NETHOST_STATIC FALSE)
        message(STATUS "Using dynamic nethost (nethost.lib) -- nethost.dll must be deployed alongside")
    endif()
else()
    message(STATUS "nethost not found automatically. Set NETHOST_DIR to the directory containing nethost.h and libnethost.lib")
    set(NETHOST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set(NETHOST_LIB "")
    set(NETHOST_STATIC FALSE)
endif()

# ── Runtime DLL target ─────────────────────────────────────────────────
add_library(WeaveLoaderRuntime SHARED
    src/dllmain.cpp
    src/LogUtil.cpp
    src/CrashHandler.cpp
    src/PdbParser.cpp
    src/SymbolResolver.cpp
    src/HookManager.cpp
    src/GameHooks.cpp
    src/DotNetHost.cpp
    src/IdRegistry.cpp
    src/NativeExports.cpp
    src/CreativeInventory.cpp
    src/MainMenuOverlay.cpp
    src/GameObjectFactory.cpp
    src/ModStrings.cpp
    src/ModAtlas.cpp
)

target_include_directories(WeaveLoaderRuntime PRIVATE
    "${NETHOST_INCLUDE_DIR}"
    "${STB_DIR}"
)

target_link_libraries(WeaveLoaderRuntime PRIVATE
    minhook
    raw_pdb
)

if(EXISTS "${NETHOST_LIB}")
    target_link_libraries(WeaveLoaderRuntime PRIVATE "${NETHOST_LIB}")
    if(NETHOST_STATIC)
        target_compile_definitions(WeaveLoaderRuntime PRIVATE NETHOST_USE_AS_STATIC)
    endif()
else()
    message(WARNING "nethost lib not found. You may need to set NETHOST_DIR.")
endif()

target_compile_definitions(WeaveLoaderRuntime PRIVATE
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(WeaveLoaderRuntime PRIVATE /W3 /MP)
endif()

set_target_properties(WeaveLoaderRuntime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../build"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../build"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../build"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../build"
)
