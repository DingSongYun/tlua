cmake_minimum_required(VERSION 3.15)
project(TypingLua C)

# ================================================================
# Build configuration: Debug vs Release
# ================================================================
#
# Debug:   Full symbols (PDB), no optimization, asserts enabled
# Release: Full optimization, no debug info
#
# Usage:
#   cmake -B build -DCMAKE_BUILD_TYPE=Debug
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Debug
#   cmake --build build --config Release
# ================================================================

# MSVC-specific: Always generate PDB for Debug builds
if(MSVC)
    # Debug: full debug info + no optimization
    set(CMAKE_C_FLAGS_DEBUG "/Zi /Od /DDEBUG /D_DEBUG /RTC1 /MDd")
    # Release: full optimization, no debug overhead
    set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG /MD")
    # RelWithDebInfo: optimized but with PDB (useful for profiling)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG /MD")

    # Ensure linker generates PDB files
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG:FULL /INCREMENTAL")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG:FULL /INCREMENTAL:NO /OPT:REF /OPT:ICF")
endif()

# Default build type if not specified (only for single-config generators)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug/Release/RelWithDebInfo)" FORCE)
    message(STATUS "Build type not specified, defaulting to: Debug")
endif()

# ================================================================
# Lua 5.4 core library sources (exclude lua.c and luac.c)
# ================================================================
set(LUA_LIB_SOURCES
    lua/lapi.c
    lua/lauxlib.c
    lua/lbaselib.c
    lua/lcode.c
    lua/lcorolib.c
    lua/lctype.c
    lua/ldblib.c
    lua/ldebug.c
    lua/ldo.c
    lua/ldump.c
    lua/lfunc.c
    lua/lgc.c
    lua/linit.c
    lua/liolib.c
    lua/llex.c
    lua/lmathlib.c
    lua/lmem.c
    lua/loadlib.c
    lua/lobject.c
    lua/lopcodes.c
    lua/loslib.c
    lua/lparser.c
    lua/lstate.c
    lua/lstring.c
    lua/lstrlib.c
    lua/ltable.c
    lua/ltablib.c
    lua/ltm.c
    lua/lundump.c
    lua/lutf8lib.c
    lua/lvm.c
    lua/lzio.c
)

# ================================================================
# Lua library: Dynamic (DLL) + Static builds
# ================================================================

# --- Dynamic library (lua54.dll) ---
# Required for lua-debug (actboy168) hook injection to work.
# The debugger needs to find Lua API symbols in a loaded DLL.
add_library(lua54_shared SHARED ${LUA_LIB_SOURCES})
target_include_directories(lua54_shared PUBLIC lua/)
# LUA_BUILD_AS_DLL: tells Lua headers to use __declspec(dllexport)
target_compile_definitions(lua54_shared PRIVATE LUA_BUILD_AS_DLL)
set_target_properties(lua54_shared PROPERTIES
    OUTPUT_NAME "lua54"
    PREFIX ""
)

# --- Static library (for tools that don't need debugging) ---
add_library(lua54_static STATIC ${LUA_LIB_SOURCES})
target_include_directories(lua54_static PUBLIC lua/)

# ================================================================
# Executables
# ================================================================

# lua.exe - Standard Lua interpreter (dynamic linked, debuggable)
add_executable(lua lua/lua.c)
target_link_libraries(lua PRIVATE lua54_shared)
target_compile_definitions(lua PRIVATE LUA_BUILD_AS_DLL)

# luac.exe - Standard Lua compiler (static, doesn't need debugging)
add_executable(luac lua/luac.c)
target_link_libraries(luac PRIVATE lua54_static)

# tluac.exe - .tlua transpiler (Type Erasure, no Lua VM needed)
set(TLUA_SOURCES
    tlua/tlua_main.c
    tlua/tlua_lexer.c
    tlua/tlua_parser.c
)
add_executable(tluac ${TLUA_SOURCES})
target_include_directories(tluac PRIVATE tlua/)

# tlua.exe - .tlua direct runner (transpile + execute, like ts-node)
# Uses dynamic Lua DLL so lua-debug can inject its debugger hooks.
set(TLUA_RUN_SOURCES
    tlua/tlua_run.c
    tlua/tlua_lexer.c
    tlua/tlua_parser.c
)
add_executable(tlua ${TLUA_RUN_SOURCES})
target_link_libraries(tlua PRIVATE lua54_shared)
target_compile_definitions(tlua PRIVATE LUA_BUILD_AS_DLL)
target_include_directories(tlua PRIVATE tlua/ lua/)

# ================================================================
# Output directory: keep all binaries together (exe + dll)
# ================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

# ================================================================
# Install (optional)
# ================================================================
install(TARGETS tlua tluac lua luac lua54_shared RUNTIME DESTINATION bin)

# ================================================================
# Testing
# ================================================================
enable_testing()

# --- test_lexer: Lexer unit tests ---
add_executable(test_lexer
    tests/test_lexer.c
    tlua/tlua_lexer.c
)
target_include_directories(test_lexer PRIVATE tlua/ tests/)

# --- test_transpiler: Transpiler unit tests ---
add_executable(test_transpiler
    tests/test_transpiler.c
    tlua/tlua_lexer.c
    tlua/tlua_parser.c
)
target_include_directories(test_transpiler PRIVATE tlua/ tests/)

# --- test_e2e: End-to-end tests ---
add_executable(test_e2e
    tests/test_e2e.c
    tlua/tlua_lexer.c
    tlua/tlua_parser.c
)
target_include_directories(test_e2e PRIVATE tlua/ tests/)

# Pass executable paths and fixture directory to E2E test via compile definitions
target_compile_definitions(test_e2e PRIVATE
    TLUAC_EXE="$<TARGET_FILE:tluac>"
    LUA_EXE="$<TARGET_FILE:lua>"
    FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
)

# --- test_runtime: Runtime execution tests (runs tlua.exe on fixtures) ---
add_executable(test_runtime
    tests/test_runtime.c
)
target_include_directories(test_runtime PRIVATE tests/)

# Pass tlua.exe path and fixture directory via compile definitions
target_compile_definitions(test_runtime PRIVATE
    TLUA_EXE="$<TARGET_FILE:tlua>"
    FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
)

# test_runtime depends on tlua being built first
add_dependencies(test_runtime tlua)

# Register tests with CTest
add_test(NAME test_lexer COMMAND test_lexer)
add_test(NAME test_transpiler COMMAND test_transpiler)
add_test(NAME test_e2e COMMAND test_e2e)
add_test(NAME test_runtime COMMAND test_runtime)
