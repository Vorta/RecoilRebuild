add_executable(recoil_native_smoke
    directx_headers.cpp
    briefing_tests.cpp
    mfc42_provider.cpp
    czrecoil_frame_tests.cpp
    pickup_tests.cpp
    gamenet_tests.cpp
    player_tests.cpp
    fmv_script_tests.cpp
    zgame_tests.cpp
    zhud_ui_tests.cpp
    zinput_tests.cpp
    zimage_tests.cpp
    zloc_tests.cpp
    znetwork_tests.cpp
    zreader_tests.cpp
    zmath_tests.cpp
    recoil_app_message_map.cpp
    recoil_state_main_menu_transition.cpp
    recoil_state_base.cpp
    zrndr_tests.cpp
    zturret_tests.cpp
    zweapon_tests.cpp
    zvideo_tests.cpp
    zsys_tests.cpp
    zsnd_cd_tests.cpp
    zeffect_tests.cpp
    zcom_tests.cpp
    zdeclient_tests.cpp
    time_tests.cpp
    smoke.cpp
)

target_link_libraries(recoil_native_smoke PRIVATE recoil_native)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(
    NAME recoil_native_smoke
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/run_native_smokes.py
        $<TARGET_FILE:recoil_native_smoke>
        --smoke-cpp ${CMAKE_CURRENT_SOURCE_DIR}/smoke.cpp
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_no_raw_image_addresses
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil_no_raw_image_addresses.py
        --root src
        --allowlist .agent/RAW_ADDRESS_ALLOWLIST.txt
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_no_raw_assembly
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil_no_raw_assembly.py
        --root src
        --allowlist .agent/RAW_ASSEMBLY_ALLOWLIST.txt
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_no_reinterpret_cast
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil_no_reinterpret_cast.py
        --root src
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_vc6_manifest_source_policy
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil_vc6_manifest_source_guard.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_source_file_map_current
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil_source_file_map.py
        --check docs/reconstruction/source_file_map.md
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_compiler_linker_provenance
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil_provenance_audit.py
        --strict
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_workspace_hygiene
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil_workspace_hygiene.py
        --strict
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_tools_unittest
    COMMAND ${Python3_EXECUTABLE}
        -m unittest discover
        -s tests/tools
        -p *_tests.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
