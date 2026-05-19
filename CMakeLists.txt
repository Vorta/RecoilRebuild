cmake_minimum_required(VERSION 3.25)

project(RecoilRebuildNative LANGUAGES C CXX)

if(MSVC)
    set(CMAKE_CXX17_STANDARD_COMPILE_OPTION "/std:c++17")
    set(CMAKE_CXX17_EXTENSION_COMPILE_OPTION "/std:c++17")
endif()

option(RECOIL_BUILD_NATIVE_TESTS "Build native CTest smoke and behavior tests." ON)
option(RECOIL_REQUIRE_32BIT "Fail configure unless the selected native compiler targets 32-bit." OFF)
option(RECOIL_EMIT_ASM "Ask MSVC to emit assembly listings for binary-safety comparison." OFF)

include(cmake/RecoilCompiler.cmake)
include(cmake/RecoilDirectX.cmake)
include(cmake/RecoilMfc42.cmake)

if(RECOIL_REQUIRE_32BIT AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "RECOIL_REQUIRE_32BIT is ON, but the selected compiler targets ${CMAKE_SIZEOF_VOID_P}-byte pointers. Use an x86 MSVC developer environment.")
endif()

if(MSVC)
    include(CheckCSourceCompiles)

    if(RECOIL_REQUIRE_32BIT)
        set(_recoil_msvc_env_check_source [=[
#include <stddef.h>
#include <windows.h>
#if !defined(_M_IX86)
#error Expected an x86 MSVC compiler target.
#endif
int main(void) { return sizeof(void*) == 4 ? 0 : 1; }
]=])
    else()
        set(_recoil_msvc_env_check_source [=[
#include <stddef.h>
#include <windows.h>
int main(void) { return 0; }
]=])
    endif()

    unset(RECOIL_MSVC_ENV_HAS_REQUIRED_HEADERS CACHE)
    set(_recoil_cmake_required_quiet "${CMAKE_REQUIRED_QUIET}")
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_c_source_compiles(
        "${_recoil_msvc_env_check_source}"
        RECOIL_MSVC_ENV_HAS_REQUIRED_HEADERS
    )
    set(CMAKE_REQUIRED_QUIET "${_recoil_cmake_required_quiet}")

    if(NOT RECOIL_MSVC_ENV_HAS_REQUIRED_HEADERS)
        message(FATAL_ERROR
            "The selected MSVC environment cannot compile a minimal Recoil native "
            "x86 probe including <stddef.h> and <windows.h>. Run "
            "`python tools/recoil_env_check.py --native-x86` from the same shell, "
            "then configure from an x86 MSVC Developer PowerShell or after vcvarsall x86."
        )
    endif()
endif()

add_subdirectory(src/native)

if(RECOIL_BUILD_NATIVE_TESTS)
    enable_testing()
    add_subdirectory(tests/native)
endif()
