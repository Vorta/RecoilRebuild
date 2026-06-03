set(RECOIL_DXSDK6_ROOT
    "${PROJECT_SOURCE_DIR}/support/sdk/DirectX6"
    CACHE PATH "Vendored DirectX 6 SDK used for source-faithful legacy DirectX builds."
)

set(RECOIL_DXSDK6_INCLUDE_DIR "${RECOIL_DXSDK6_ROOT}/include")
set(RECOIL_DXSDK6_LIB_X86_DIR "${RECOIL_DXSDK6_ROOT}/lib")

foreach(_recoil_dx6_header IN ITEMS ddraw.h d3d.h dinput.h dplay.h dplobby.h dsound.h)
    if(NOT EXISTS "${RECOIL_DXSDK6_INCLUDE_DIR}/${_recoil_dx6_header}")
        message(FATAL_ERROR
            "Missing DirectX 6 header ${_recoil_dx6_header} at ${RECOIL_DXSDK6_INCLUDE_DIR}"
        )
    endif()
endforeach()

foreach(_recoil_dx6_lib IN ITEMS ddraw.lib d3dim.lib dinput.lib dplayx.lib dsound.lib dxguid.lib)
    if(NOT EXISTS "${RECOIL_DXSDK6_LIB_X86_DIR}/${_recoil_dx6_lib}")
        message(FATAL_ERROR
            "Missing DirectX 6 x86 import library ${_recoil_dx6_lib} at ${RECOIL_DXSDK6_LIB_X86_DIR}"
        )
    endif()
endforeach()

add_library(recoil_directx_legacy INTERFACE)

target_include_directories(recoil_directx_legacy INTERFACE
    "${RECOIL_DXSDK6_INCLUDE_DIR}"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_link_directories(recoil_directx_legacy INTERFACE
        "${RECOIL_DXSDK6_LIB_X86_DIR}"
    )
else()
    message(WARNING "DirectX 6 import libraries are vendored for x86 only. Use an x86 MSVC environment for binary-safe builds.")
endif()
