set(RECOIL_VC5SP3_ROOT
    "${PROJECT_SOURCE_DIR}/../Compiler/VC5SP3"
    CACHE PATH "Portable Visual C++ 5.0 SP3 toolchain root."
)

set(RECOIL_MFC42_ROOT "${RECOIL_VC5SP3_ROOT}/VC/MFC")

set(RECOIL_MFC42_INCLUDE_DIR "${RECOIL_MFC42_ROOT}/INCLUDE")
set(RECOIL_MFC42_LIB_X86_DIR "${RECOIL_MFC42_ROOT}/LIB")
set(RECOIL_MFC42_SRC_DIR "${RECOIL_MFC42_ROOT}/SRC")
set(RECOIL_MFC42_RUNTIME_DLL "${PROJECT_SOURCE_DIR}/support/mfc42.dll")

if(NOT EXISTS "${RECOIL_MFC42_INCLUDE_DIR}/AFXWIN.H")
    message(FATAL_ERROR "Missing VC5SP3 MFC42 headers at ${RECOIL_MFC42_INCLUDE_DIR}")
endif()

if(NOT EXISTS "${RECOIL_MFC42_LIB_X86_DIR}/MFC42.LIB")
    message(FATAL_ERROR "Missing VC5SP3 MFC42 x86 import library at ${RECOIL_MFC42_LIB_X86_DIR}")
endif()

if(NOT EXISTS "${RECOIL_MFC42_SRC_DIR}/APPCORE.CPP")
    message(FATAL_ERROR "Missing VC5 MFC42 source evidence at ${RECOIL_MFC42_SRC_DIR}")
endif()

file(STRINGS "${RECOIL_MFC42_INCLUDE_DIR}/AFXWIN.H" RECOIL_MFC42_AFXWIN_COPYRIGHT
    LIMIT_COUNT 1
    REGEX "Copyright \\(C\\) 1992-1997 Microsoft Corporation"
)
file(STRINGS "${RECOIL_MFC42_INCLUDE_DIR}/AFXWIN.H" RECOIL_MFC42_AFXWIN_GUARD
    LIMIT_COUNT 1
    REGEX "^#define __AFXWIN_H__$"
)
if(NOT RECOIL_MFC42_AFXWIN_COPYRIGHT OR NOT RECOIL_MFC42_AFXWIN_GUARD)
    message(FATAL_ERROR
        "Unexpected AFXWIN.H provider at ${RECOIL_MFC42_INCLUDE_DIR}; "
        "expected the VC5-era Microsoft 1992-1997 marker and __AFXWIN_H__ guard."
    )
endif()

if(NOT EXISTS "${RECOIL_VC5SP3_ROOT}/VC/BIN/CL.EXE")
    message(FATAL_ERROR "Missing VC5SP3 compiler probe executable at ${RECOIL_VC5SP3_ROOT}/VC/BIN/CL.EXE")
endif()

if(NOT EXISTS "${RECOIL_MFC42_RUNTIME_DLL}")
    message(FATAL_ERROR "Missing MFC42 runtime DLL at ${RECOIL_MFC42_RUNTIME_DLL}")
endif()

add_library(recoil_mfc42_legacy INTERFACE)

target_include_directories(recoil_mfc42_legacy INTERFACE
    "${RECOIL_MFC42_INCLUDE_DIR}"
)

target_compile_definitions(recoil_mfc42_legacy INTERFACE
    _AFX_NOFORCE_LIBS
    _AFXDLL
    _MBCS
)

if(MSVC)
    target_compile_options(recoil_mfc42_legacy INTERFACE
        "/FI${PROJECT_SOURCE_DIR}/tools/_recoil/compat/include/recoil/Mfc42Abi.h"
    )
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_link_libraries(recoil_mfc42_legacy INTERFACE
        "${RECOIL_MFC42_LIB_X86_DIR}/MFC42.LIB"
    )
else()
    message(WARNING "Legacy Recoil MFC42 import libraries are vendored for x86 only. Use an x86 MSVC environment for native verification builds.")
endif()
