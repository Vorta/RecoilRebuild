add_executable(recoil_zbd_model_viewer
    zbd_model_viewer.cpp
)

target_link_libraries(recoil_zbd_model_viewer PRIVATE recoil_native Dbghelp)

add_custom_target(recoil_zbd_model_viewer_copy_to_playground
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:recoil_zbd_model_viewer>
        ${PROJECT_SOURCE_DIR}/playground/$<TARGET_FILE_NAME:recoil_zbd_model_viewer>
    DEPENDS recoil_zbd_model_viewer
    COMMENT "Copying recoil_zbd_model_viewer.exe to playground runtime folder"
)
