set(RECOIL_DXSDK_AUG2007_ROOT
    "${PROJECT_SOURCE_DIR}/support/sdk/DirectX_Aug2007"
    CACHE PATH "Vendored DirectX SDK August 2007 subset used for legacy DirectPlay/DirectDraw builds."
)

set(RECOIL_DXSDK_AUG2007_INCLUDE_DIR "${RECOIL_DXSDK_AUG2007_ROOT}/Include")
set(RECOIL_DXSDK_AUG2007_LIB_X86_DIR "${RECOIL_DXSDK_AUG2007_ROOT}/Lib/x86")

if(NOT EXISTS "${RECOIL_DXSDK_AUG2007_INCLUDE_DIR}/dplay.h")
    message(FATAL_ERROR "Missing vendored DirectPlay headers at ${RECOIL_DXSDK_AUG2007_INCLUDE_DIR}")
endif()

if(NOT EXISTS "${RECOIL_DXSDK_AUG2007_LIB_X86_DIR}/dplayx.lib")
    message(FATAL_ERROR "Missing vendored DirectPlay x86 import libraries at ${RECOIL_DXSDK_AUG2007_LIB_X86_DIR}")
endif()

add_library(recoil_directx_legacy INTERFACE)

target_include_directories(recoil_directx_legacy INTERFACE
    "${RECOIL_DXSDK_AUG2007_INCLUDE_DIR}"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_link_directories(recoil_directx_legacy INTERFACE
        "${RECOIL_DXSDK_AUG2007_LIB_X86_DIR}"
    )
else()
    message(WARNING "Legacy Recoil DirectX import libraries are vendored for x86 only. Use an x86 MSVC environment for binary-safe builds.")
endif()
