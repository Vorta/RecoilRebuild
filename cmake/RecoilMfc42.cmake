set(RECOIL_MFC42_ROOT
    "${PROJECT_SOURCE_DIR}/support/sdk/MFC42"
    CACHE PATH "Vendored Visual C++ 6.0 MFC42 SDK subset used for legacy MFC ABI work."
)

set(RECOIL_MFC42_INCLUDE_DIR "${RECOIL_MFC42_ROOT}/Include")
set(RECOIL_MFC42_LIB_X86_DIR "${RECOIL_MFC42_ROOT}/Lib/x86")
set(RECOIL_MFC42_SRC_DIR "${RECOIL_MFC42_ROOT}/Src")
set(RECOIL_MFC42_RUNTIME_DLL "${PROJECT_SOURCE_DIR}/support/mfc42.dll")

if(NOT EXISTS "${RECOIL_MFC42_INCLUDE_DIR}/AFXWIN.H")
    message(FATAL_ERROR "Missing vendored MFC42 headers at ${RECOIL_MFC42_INCLUDE_DIR}")
endif()

if(NOT EXISTS "${RECOIL_MFC42_LIB_X86_DIR}/MFC42.LIB")
    message(FATAL_ERROR "Missing vendored MFC42 x86 import library at ${RECOIL_MFC42_LIB_X86_DIR}")
endif()

if(NOT EXISTS "${RECOIL_MFC42_SRC_DIR}/APPCORE.CPP")
    message(FATAL_ERROR "Missing vendored MFC42 source evidence at ${RECOIL_MFC42_SRC_DIR}")
endif()

if(NOT EXISTS "${RECOIL_MFC42_RUNTIME_DLL}")
    message(FATAL_ERROR "Missing MFC42 runtime DLL at ${RECOIL_MFC42_RUNTIME_DLL}")
endif()

add_library(recoil_mfc42_legacy INTERFACE)

target_include_directories(recoil_mfc42_legacy INTERFACE
    "${RECOIL_MFC42_INCLUDE_DIR}"
)

target_compile_definitions(recoil_mfc42_legacy INTERFACE
    _AFXDLL
    _MBCS
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_link_libraries(recoil_mfc42_legacy INTERFACE
        "${RECOIL_MFC42_LIB_X86_DIR}/MFC42.LIB"
    )
else()
    message(WARNING "Legacy Recoil MFC42 import libraries are vendored for x86 only. Use an x86 MSVC environment for binary-safe builds.")
endif()
