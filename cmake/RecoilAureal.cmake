set(RECOIL_AUREAL_A3D20_ROOT
    "${PROJECT_SOURCE_DIR}/support/sdk/Aureal/A3D20"
    CACHE PATH "Vendored Aureal A3D 2.0 SDK headers used for source-faithful A3D provider builds."
)

set(RECOIL_AUREAL_A3D20_INCLUDE_DIR "${RECOIL_AUREAL_A3D20_ROOT}/inc")

if(NOT EXISTS "${RECOIL_AUREAL_A3D20_INCLUDE_DIR}/ia3dapi.h")
    message(FATAL_ERROR
        "Missing Aureal A3D 2.0 header ia3dapi.h at ${RECOIL_AUREAL_A3D20_INCLUDE_DIR}"
    )
endif()

add_library(recoil_aureal_a3d20_legacy INTERFACE)

target_include_directories(recoil_aureal_a3d20_legacy INTERFACE
    "${RECOIL_AUREAL_A3D20_INCLUDE_DIR}"
)
