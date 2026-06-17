add_executable(recoil_native_smoke
    directx_headers.cpp
    ai_property_dialog_tests.cpp
    mfc42_provider.cpp
    mfc_three_float_dialog_tests.cpp
    zstr_tests.cpp
    zinput_tests.cpp
    zimage_tests.cpp
    zinterp_tests.cpp
    zvideo_primary_blit_smokes.cpp
    zvideo_clear_dispatch_smokes.cpp
    zvideo_dd_enum_smokes.cpp
    zvideo_noise_tests.cpp
    zvideo_blur_region_smokes.cpp
    zloc_tests.cpp
    zhud_triplet_smokes.cpp
    zsnd_cd_tests.cpp
    zsnd_stream_tests.cpp
    zreader_tests.cpp
    znetwork_identity_smokes.cpp
    gamenet_launch_smokes.cpp
    player_bootstrap_smokes.cpp
    zutil_save_game_smokes.cpp
    fmv_script_core_smokes.cpp
    fmv_action_manifest_smokes.cpp
    briefing_runtime_smokes.cpp
    zhud_composite_panel_smokes.cpp
    player_fx_pass3_smokes.cpp
    zhud_cmd_dialog_state_smokes.cpp
    zhud_net_game_setup_smokes.cpp
    zhud_options_panel_smokes.cpp
    zhud_widget_tests.cpp
    hud_tests.cpp
    hud_save_load_tests.cpp
    czframe_constructor_smokes.cpp
    recoil_state_main_menu_transition_constructor_tests.cpp
    zmath_tests.cpp
    recoil_version_tests.cpp
    zrndr_tests.cpp
    zturret_tests.cpp
    zmodel_di_smokes.cpp
    zclass_tests.cpp
    zeffect_tests.cpp
    zweapon_tests.cpp
    zweapon_timed_hit_smokes.cpp
    zsys_tests.cpp
    zcom_tests.cpp
    time_tests.cpp
    smoke.cpp
    recoil_test_resources.rc
)

target_link_libraries(recoil_native_smoke PRIVATE recoil_native)

set_source_files_properties(
    zweapon_tests.cpp
    PROPERTIES COMPILE_DEFINITIONS RECOIL_NATIVE_ZWEAPON_SKIP_DUPLICATE_TIMED_HIT_SMOKE
)

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
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        raw-image
        --root src
        --allowlist .agent/RAW_ADDRESS_ALLOWLIST.txt
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_no_raw_assembly
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        raw-assembly
        --root src
        --allowlist .agent/RAW_ASSEMBLY_ALLOWLIST.txt
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_no_modern_cpp_constructs
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        modern-cpp
        --root src
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_raw_offset_guard
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        raw-offset
        --root src
        --plan .agent/RECOIL_PLAN.md
        --allowlist .agent/RAW_OFFSET_ALLOWLIST.txt
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_provider_boundary_guard
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        provider
        --root src
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_provider_boundary_guard_selftest
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        provider
        --self-test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_source_placement_guard
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        source-placement
        --root src
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_source_placement_guard_selftest
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        source-placement
        --self-test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_multiline_style_guard
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        multiline
        --root src
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_multiline_style_guard_selftest
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        multiline
        --self-test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_source_data_initializer_guard
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        source-data
        --manifest .agent/SOURCE_DATA_INITIALIZERS.json
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_source_data_initializer_guard_selftest
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        source-data
        --self-test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_vc5_manifest_source_policy
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        guard
        vc5-manifest
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_source_file_map_current
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        audit
        source-map
        --check docs/reconstruction/source_file_map.md
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_compiler_linker_provenance
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        audit
        provenance
        --strict
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME recoil_workspace_hygiene
    COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/recoil.py
        audit
        workspace
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
