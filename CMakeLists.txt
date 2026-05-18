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

add_subdirectory(src/native)

if(RECOIL_BUILD_NATIVE_TESTS)
    enable_testing()
    add_subdirectory(tests/native)
endif()
