add_executable(recoil_native_smoke
    directx_headers.cpp
    ai_property_dialog_tests.cpp
    mfc42_provider.cpp
    mfc_three_float_dialog_tests.cpp
    zstr_tests.cpp
    zinput_tests.cpp
    zimage_tests.cpp
    zinterp_tests.cpp
    zmodel_damage_mask_smokes.cpp
    zvideo_primary_blit_smokes.cpp
    zvideo_clear_dispatch_smokes.cpp
    zvideo_dd_enum_smokes.cpp
    zvideo_sw_render_frame_smokes.cpp
    zvideo_fx_surface_smokes.cpp
    zvideo_noise_tests.cpp
    zvideo_blur_region_smokes.cpp
    zloc_tests.cpp
    messages_lookup_tests.cpp
    zhud_triplet_smokes.cpp
    zhud_timer_panel_smokes.cpp
    zhud_ui_tests.cpp
    zsnd_cd_tests.cpp
    zsnd_stream_tests.cpp
    zreader_tests.cpp
    znetwork_identity_smokes.cpp
    gamenet_launch_smokes.cpp
    gamenet_net_smokes.cpp
    net_session_browser_dialog_smokes.cpp
    net_session_config_dialog_smokes.cpp
    westwood_online_upgrade_event_sink_smokes.cpp
    westwood_online_upgrade_smokes.cpp
    westwood_online_upgrade_tests.cpp
    westwood_online_upgrade_progress_dialog_smokes.cpp
    player_bootstrap_smokes.cpp
    player_zar_smokes.cpp
    zutil_save_game_smokes.cpp
    fmv_script_core_smokes.cpp
    fmv_script_tests.cpp
    fmv_action_manifest_smokes.cpp
    fmv_action_functional_smokes.cpp
    briefing_runtime_smokes.cpp
    zhud_composite_panel_smokes.cpp
    player_fx_pass3_smokes.cpp
    zvideo_fxpass3_ui_local_config_smokes.cpp
    player_alt_gun_runtime_smokes.cpp
    player_slip_dynamics_smokes.cpp
    player_tests.cpp
    player_master_mode_functional_smokes.cpp
    player_damage_hit_smokes.cpp
    zhud_cmd_dialog_state_smokes.cpp
    zhud_net_game_setup_smokes.cpp
    zhud_fill_bitmap_regression_smoke.cpp
    zhud_options_panel_smokes.cpp
    zhud_widget_tests.cpp
    hud_tests.cpp
    hud_save_load_tests.cpp
    recoil_app_message_map.cpp
    recoil_app_functional_smokes.cpp
    hud_ui_net_exit_smokes.cpp
    hud_weather_fx_smokes.cpp
    czframe_constructor_smokes.cpp
    czrecoil_frame_tests.cpp
    recoil_state_main_menu_transition_constructor_tests.cpp
    zmath_tests.cpp
    recoil_version_tests.cpp
    zrndr_tests.cpp
    zturret_tests.cpp
    zmodel_di_smokes.cpp
    zclass_tests.cpp
    zclass_cls_di_smokes.cpp
    zstub_vtable_stubs_smokes.cpp
    zdeclient_tests.cpp
    zeffect_tests.cpp
    zweapon_tests.cpp
    zweapon_timed_hit_smokes.cpp
    zsys_tests.cpp
    zcom_tests.cpp
    time_tests.cpp
    briefing_set_progress_smoke.cpp
    hud_sensor_tracker_mission_smokes.cpp
    hud_sensor_tracker_objective_smokes.cpp
    player_ai_mode2_smokes.cpp
    recoil_app_fmv_state_smokes.cpp
    recoil_app_init_std_log_files_smoke.cpp
    recoil_app_save_load_transition_smokes.cpp
    recoil_state_main_menu_transition_on_try_smoke.cpp
    westwood_online_upgrade_config_smokes.cpp
    westwood_online_upgrade_dialog_functional_smokes.cpp
    zclass_camera_convex_hull_xz_smokes.cpp
    zclass_camera_render_pass_smokes.cpp
    zclass_world_settings_smokes.cpp
    zdi_entry_material_helper_smoke.cpp
    zfmv_play_mci_smokes.cpp
    zhud_background_video_widget_draw_smokes.cpp
    zhud_background_video_widget_update_smoke.cpp
    zhud_message_box_smokes.cpp
    zhud_sensor_objective_smokes.cpp
    zhud_ui_net_game_setup_deactivate_smoke.cpp
    zhud_functional_smokes.cpp
    zmodel_dipool_stream_smokes.cpp
    zmodel_shutdown_smokes.cpp
    briefing_functional_smokes.cpp
    main_menu_functional_smokes.cpp
    pickup_functional_smokes.cpp
    zgame_functional_smokes.cpp
    znetwork_functional_smokes.cpp
    zvideo_functional_smokes.cpp
    smoke.cpp
    recoil_test_resources.rc
)

target_link_libraries(recoil_native_smoke PRIVATE recoil_native)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_executable(recoil_volume_regression
    mfc42_provider.cpp
    zhud_fill_bitmap_regression_smoke.cpp
    zhud_options_panel_smokes.cpp
    volume_regression_main.cpp
)

target_link_libraries(recoil_volume_regression PRIVATE recoil_native)

add_test(
    NAME recoil_volume_regression
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/run_native_smokes.py
        $<TARGET_FILE:recoil_volume_regression>
        --smoke-cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/volume_regression_main.cpp
        --timeout-seconds
        10
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

set_source_files_properties(
    zweapon_tests.cpp
    PROPERTIES COMPILE_DEFINITIONS RECOIL_NATIVE_ZWEAPON_SKIP_DUPLICATE_TIMED_HIT_SMOKE
)

set_source_files_properties(
    zhud_ui_tests.cpp
    PROPERTIES COMPILE_DEFINITIONS RECOIL_NATIVE_ZHUD_UI_LIFECYCLE_ONLY
)

set_source_files_properties(
    czrecoil_frame_tests.cpp
    PROPERTIES COMPILE_DEFINITIONS
        "RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES;RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_MENU_SMOKES"
)

set_source_files_properties(
    player_tests.cpp
    PROPERTIES COMPILE_DEFINITIONS
        "RECOIL_NATIVE_PLAYER_TESTS_GUN_DISPATCH_ONLY;RECOIL_NATIVE_PLAYER_TESTS_CAMERA_SMOKES;RECOIL_NATIVE_PLAYER_TESTS_ENV_PROBE_SMOKES;RECOIL_NATIVE_PLAYER_TESTS_PENDING_CONTACT_SMOKES;RECOIL_NATIVE_PLAYER_TESTS_ASYNC_COMMAND_CALLBACK_SMOKE;RECOIL_NATIVE_PLAYER_TESTS_GAMEPLAY_CALLBACK_SMOKE"
)

set_source_files_properties(
    fmv_script_tests.cpp
    PROPERTIES COMPILE_DEFINITIONS RECOIL_FMV_SCRIPT_TESTS_WAIT_ONLY
)

set_source_files_properties(
    recoil_app_message_map.cpp
    PROPERTIES COMPILE_DEFINITIONS RECOIL_NATIVE_RECOIL_APP_MESSAGE_MAP_CALLBACK_SMOKES_ONLY
)

set(ZDECLIENT_NATIVE_SMOKE_PREFIX
    "${CMAKE_CURRENT_BINARY_DIR}/zdeclient_native_smoke_prefix.h"
)
file(WRITE "${ZDECLIENT_NATIVE_SMOKE_PREFIX}" [=[
#include "recoil/Mfc42Abi.h"
#include "GameZRecoil/zNetwork/znet.h"
#include <cstdint>

struct zNetwork_DPlay4SmokeVtable;
struct zNetwork_DPlay4Smoke {
    const zNetwork_DPlay4SmokeVtable *lpVtbl;
};

typedef std::int32_t(__stdcall *zNetwork_DPlay4SmokeSendFn)(
    zNetwork_DPlay4Smoke *,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    void *,
    std::uint32_t
);

struct zNetwork_DPlay4SmokeVtable {
    void *slots_00_64[26];
    zNetwork_DPlay4SmokeSendFn Send_68;
};

#define zNetwork_DPlay4 zNetwork_DPlay4Smoke
#define zNetwork_DPlay4Vtable zNetwork_DPlay4SmokeVtable
#define g_zNetwork_pDirectPlay4 (*((zNetwork_DPlay4Smoke **)&g_zNetwork_pDirectPlay4))
]=])

set_source_files_properties(
    westwood_online_upgrade_tests.cpp
    PROPERTIES COMPILE_DEFINITIONS RECOIL_NATIVE_WESTWOOD_ONLINE_UPGRADE_UNADVISE_SMOKE_ONLY
)

set_source_files_properties(
    zdeclient_tests.cpp
    PROPERTIES COMPILE_OPTIONS "/FI${ZDECLIENT_NATIVE_SMOKE_PREFIX}"
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
        --progress .agent/RECONSTRUCTION_PROGRESS.sqlite3
        --binary recoil
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
        --progress .agent/RECONSTRUCTION_PROGRESS.sqlite3
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
