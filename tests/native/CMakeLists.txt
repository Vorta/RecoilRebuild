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
    briefing_runtime_smokes.cpp
    zhud_composite_panel_smokes.cpp
    player_fx_pass3_smokes.cpp
    zvideo_fxpass3_ui_local_config_smokes.cpp
    player_alt_gun_runtime_smokes.cpp
    player_slip_dynamics_smokes.cpp
    player_tests.cpp
    player_damage_hit_smokes.cpp
    zhud_cmd_dialog_state_smokes.cpp
    zhud_net_game_setup_smokes.cpp
    zhud_options_panel_smokes.cpp
    zhud_widget_tests.cpp
    hud_tests.cpp
    hud_save_load_tests.cpp
    recoil_app_message_map.cpp
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
    zdeclient_tests.cpp
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

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(WESTWOOD_ONLINE_UPGRADE_CONFIG_SMOKE_SOURCE
    "${CMAKE_CURRENT_BINARY_DIR}/westwood_online_upgrade_config_smokes.cpp"
)
file(WRITE "${WESTWOOD_ONLINE_UPGRADE_CONFIG_SMOKE_SOURCE}" [=[
#define RECOIL_NATIVE_WESTWOOD_ONLINE_UPGRADE_CONFIG_SMOKES_ONLY
#include "]=] "${CMAKE_CURRENT_SOURCE_DIR}/westwood_online_upgrade_tests.cpp" [=["
]=])
target_sources(recoil_native_smoke PRIVATE "${WESTWOOD_ONLINE_UPGRADE_CONFIG_SMOKE_SOURCE}")

set(PLAYER_AI_MODE2_SMOKE_SOURCE
    "${CMAKE_CURRENT_BINARY_DIR}/player_ai_mode2_smokes.cpp"
)
set(PLAYER_AI_MODE2_SMOKE_GENERATOR
    "${CMAKE_CURRENT_BINARY_DIR}/generate_player_ai_mode2_smokes.py"
)
file(WRITE "${PLAYER_AI_MODE2_SMOKE_GENERATOR}" [=[
from pathlib import Path
import sys

source_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
text = source_path.read_text()
lines = text.splitlines()

prefix_end = next(
    index for index, line in enumerate(lines)
    if line.startswith("#ifndef RECOIL_NATIVE_PLAYER_TESTS_GUN_DISPATCH_ONLY")
)

function_names = [
    "player_tick_ai_mode2_top_level_smoke",
    "player_build_ai_peer_rings_by_ai_net_id_smoke",
    "player_tick_ai_mode2_path_follow_smoke",
    "player_ai_mode2_forward_probe_requires_auto_turn_smoke",
    "player_ai_advance_path_cursor_and_compute_target_vec_smoke",
    "player_ai_choose_next_path_branch_index_smoke",
    "player_tick_ai_mode2_steering_substate_smoke",
    "player_update_ai_mode2_move_and_turn_toward_target_smoke",
    "player_tick_ai_mode2_offset_target_steering_smoke",
    "player_tick_ai_mode2_dynamic_offset_target_steering_smoke",
    "player_ai_try_enter_mode2_attack_pursuit_los_smoke",
    "player_ai_alert_attack_buddies_smoke",
    "player_ai_enter_mode2_steering_pursuit_smoke",
    "player_ai_rebuild_synthetic_path_to_node_if_far_smoke",
    "player_update_ai_mode2_turn_toward_player_no_throttle_smoke",
    "player_update_ai_mode2_turn_in_place_toward_player_smoke",
    "player_update_ai_mode2_move_and_turn_toward_offset_target_smoke",
    "player_update_ai_mode2_move_and_turn_toward_dynamic_offset_target_smoke",
    "player_tick_ai_mode2_timed_path_steering_smoke",
    "player_tick_remote_network_player_smoke",
]


def skip_literal(text, index):
    quote = text[index]
    index += 1
    while index < len(text):
        if text[index] == "\\":
            index += 2
            continue
        if text[index] == quote:
            return index + 1
        index += 1
    return index


def skip_line_comment(text, index):
    newline = text.find("\n", index + 2)
    return len(text) if newline == -1 else newline + 1


def skip_block_comment(text, index):
    end = text.find("*/", index + 2)
    return len(text) if end == -1 else end + 2


def extract_function(name):
    needle = f"{name}("
    name_pos = text.index(needle)
    start = text.rfind("\n", 0, name_pos) + 1
    index = text.index("{", start)
    depth = 0
    while index < len(text):
        if text[index] in ('"', "'"):
            index = skip_literal(text, index)
            continue
        if text.startswith("//", index):
            index = skip_line_comment(text, index)
            continue
        if text.startswith("/*", index):
            index = skip_block_comment(text, index)
            continue
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
        index += 1
    raise RuntimeError(f"could not find end of {name}")


output = []
output.append("// Generated by tests/native/CMakeLists.txt from player_tests.cpp.")
output.append("// Emits selected player smoke functions to avoid duplicate smoke definitions.")
output.append("")
output.extend(lines[:prefix_end])
output.append("")
output.append(r"""
namespace {
template <typename T> T *AllocZeroedMalloc() {
    void *const mem = std::calloc(1, sizeof(T));
    return static_cast<T *>(mem);
}

template <typename T> T *AllocZeroedNew() {
    T *const value = static_cast<T *>(::operator new(sizeof(T)));
    std::memset(value, 0, sizeof(T));
    return value;
}

bool Vec3Equals(const zVec3 &value, const zVec3 &expected) {
    return value.x == expected.x && value.y == expected.y && value.z == expected.z;
}

bool FloatNear(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

PlayerPendingContact *MakePendingContactChain(int count) {
    PlayerPendingContact *head = nullptr;
    PlayerPendingContact *tail = nullptr;
    for (int i = 0; i < count; ++i) {
        PlayerPendingContact *const contact = AllocZeroedNew<PlayerPendingContact>();
        if (head == nullptr) {
            head = contact;
        } else {
            tail->next = contact;
        }
        tail = contact;
    }
    return head;
}

void FillPendingContactQueue(PlayerPendingContactQueue *queue, int count) {
    queue->listAux = 0x12340000 + count;
    queue->head = MakePendingContactChain(count);
    queue->tail = queue->head;
    while (queue->tail != nullptr && queue->tail->next != nullptr) {
        queue->tail = queue->tail->next;
    }
    queue->count = count;
}

bool PendingContactQueueCleared(const PlayerPendingContactQueue &queue) {
    return queue.listAux == 0 && queue.head == nullptr && queue.tail == nullptr &&
           queue.count == 0;
}

bool PendingContactPayloadMatches(const PlayerPendingContact *contact,
                                  const zClassDiPickCandidateEntry &candidate,
                                  const zVec3 &segmentStart, const zVec3 &segmentEnd,
                                  int segmentTag) {
    return contact != nullptr && contact->hit.node == candidate.node &&
           contact->hit.scenePayload == candidate.scenePayload &&
           contact->hit.surfaceNormal.y == candidate.surfaceNormal.y &&
           Vec3Equals(contact->sweepStart, segmentStart) &&
           Vec3Equals(contact->sweepEnd, segmentEnd) && contact->segmentTag == segmentTag;
}

void InitObjectPositionNode(zClass_NodePartial *node, zClass_Object3DDataPartial *data,
                            float x, float y, float z) {
    std::memset(node, 0, sizeof(*node));
    std::memset(data, 0, sizeof(*data));
    node->classId = 5;
    node->classData = data;
    node->flags = 1;
    data->localMatrix[9] = x;
    data->localMatrix[10] = y;
    data->localMatrix[11] = z;
}
} // namespace
""".strip())
output.append("")

for function_name in function_names:
    output.append(extract_function(function_name).rstrip())
    output.append("")

output_path.write_text("\n".join(output))
]=])
execute_process(
    COMMAND ${Python3_EXECUTABLE}
        "${PLAYER_AI_MODE2_SMOKE_GENERATOR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/player_tests.cpp"
        "${PLAYER_AI_MODE2_SMOKE_SOURCE}"
    RESULT_VARIABLE PLAYER_AI_MODE2_SMOKE_GENERATOR_RESULT
)
if(NOT PLAYER_AI_MODE2_SMOKE_GENERATOR_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate player AI mode-2 smoke source")
endif()
target_sources(recoil_native_smoke PRIVATE "${PLAYER_AI_MODE2_SMOKE_SOURCE}")

set(HUD_SENSOR_TRACKER_MISSION_SMOKE_SOURCE
    "${CMAKE_CURRENT_BINARY_DIR}/hud_sensor_tracker_mission_smokes.cpp"
)
set(HUD_SENSOR_TRACKER_MISSION_SMOKE_GENERATOR
    "${CMAKE_CURRENT_BINARY_DIR}/generate_hud_sensor_tracker_mission_smokes.py"
)
file(WRITE "${HUD_SENSOR_TRACKER_MISSION_SMOKE_GENERATOR}" [=[
from pathlib import Path
import sys

source_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
text = source_path.read_text()
lines = text.splitlines()

prefix_end = next(
    index for index, line in enumerate(lines)
    if line.startswith("#ifndef RECOIL_NATIVE_PLAYER_TESTS_GUN_DISPATCH_ONLY")
)
helper_end = next(
    index for index in range(prefix_end + 1, len(lines))
    if lines[index] == "} // namespace"
)


def skip_literal(text, index):
    quote = text[index]
    index += 1
    while index < len(text):
        if text[index] == "\\":
            index += 2
            continue
        if text[index] == quote:
            return index + 1
        index += 1
    return index


def skip_line_comment(text, index):
    newline = text.find("\n", index + 2)
    return len(text) if newline == -1 else newline + 1


def skip_block_comment(text, index):
    end = text.find("*/", index + 2)
    return len(text) if end == -1 else end + 2


def extract_function(name):
    needle = f"{name}("
    name_pos = text.index(needle)
    start = text.rfind("\n", 0, name_pos) + 1
    index = text.index("{", start)
    depth = 0
    while index < len(text):
        if text[index] in ('"', "'"):
            index = skip_literal(text, index)
            continue
        if text.startswith("//", index):
            index = skip_line_comment(text, index)
            continue
        if text.startswith("/*", index):
            index = skip_block_comment(text, index)
            continue
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
        index += 1
    raise RuntimeError(f"could not find end of {name}")


output = []
output.append("// Generated by tests/native/CMakeLists.txt from player_tests.cpp.")
output.append("// Emits the HudSensorTracker mission-start smoke without broad player tests.")
output.append("")
output.extend(lines[:prefix_end])
output.append('#include "GameZRecoil/zHud/zhud_ui.h"')
output.append("")
output.append("#include <cstdint>")
output.append("#include <new>")
output.append("")
output.append('extern "C" int g_Player_MissionInitFirstRunFlag;')
output.append("")
for helper_name in [
    "MakeAinetReaderStringNode",
    "MakeAinetReaderIntNode",
    "MakeAinetReaderFloatNode",
    "MakeAinetReaderArrayNode",
    "WriteAinetZrdU32",
    "WriteAinetZrdNode",
]:
    output.append(extract_function(helper_name).rstrip())
    output.append("")
output.append("""struct AinetZrdArchiveEntry {
    const char *name;
    const zReader::Node *root;
};""")
output.append("")
output.append(extract_function("MountAinetZrdArchive").rstrip())
output.append("")
output.append(extract_function("FloatNear").rstrip())
output.append("")
output.append(extract_function("hud_sensor_tracker_init_mission_gameplay_systems_smoke").rstrip())
output.append("")
output.append(extract_function("hud_sensor_tracker_parse_checkpoint_number_from_node_smoke").rstrip())
output.append("")

output_path.write_text("\n".join(output))
]=])
execute_process(
    COMMAND ${Python3_EXECUTABLE}
        "${HUD_SENSOR_TRACKER_MISSION_SMOKE_GENERATOR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/player_tests.cpp"
        "${HUD_SENSOR_TRACKER_MISSION_SMOKE_SOURCE}"
    RESULT_VARIABLE HUD_SENSOR_TRACKER_MISSION_SMOKE_GENERATOR_RESULT
)
if(NOT HUD_SENSOR_TRACKER_MISSION_SMOKE_GENERATOR_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate HudSensorTracker mission smoke source")
endif()
target_sources(recoil_native_smoke PRIVATE "${HUD_SENSOR_TRACKER_MISSION_SMOKE_SOURCE}")

set(HUD_SENSOR_TRACKER_OBJECTIVE_SMOKE_SOURCE
    "${CMAKE_CURRENT_BINARY_DIR}/hud_sensor_tracker_objective_smokes.cpp"
)
set(HUD_SENSOR_TRACKER_OBJECTIVE_SMOKE_GENERATOR
    "${CMAKE_CURRENT_BINARY_DIR}/generate_hud_sensor_tracker_objective_smokes.py"
)
file(WRITE "${HUD_SENSOR_TRACKER_OBJECTIVE_SMOKE_GENERATOR}" [=[
from pathlib import Path
import sys

source_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
text = source_path.read_text()

include_end = text.index('\n#ifndef RECOIL_ZGAME_TESTS_ZMODEL_CONST_ONLY')
namespace_start = text.index('\nnamespace {', include_end) + 1
namespace_end = text.index('\nextern "C" int zgame_return_only_stub_smoke')
prefix = text[:include_end] + "\n\n" + text[namespace_start:namespace_end]


def skip_literal(text, index):
    quote = text[index]
    index += 1
    while index < len(text):
        if text[index] == "\\":
            index += 2
            continue
        if text[index] == quote:
            return index + 1
        index += 1
    return index


def skip_line_comment(text, index):
    newline = text.find("\n", index + 2)
    return len(text) if newline == -1 else newline + 1


def skip_block_comment(text, index):
    end = text.find("*/", index + 2)
    return len(text) if end == -1 else end + 2


def extract_function(name):
    needle = f"{name}("
    name_pos = text.index(needle)
    start = text.rfind("\n", 0, name_pos) + 1
    index = text.index("{", start)
    depth = 0
    while index < len(text):
        if text[index] in ('"', "'"):
            index = skip_literal(text, index)
            continue
        if text.startswith("//", index):
            index = skip_line_comment(text, index)
            continue
        if text.startswith("/*", index):
            index = skip_block_comment(text, index)
            continue
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
        index += 1
    raise RuntimeError(f"could not find end of {name}")


output = []
output.append("// Generated by tests/native/CMakeLists.txt from zgame_tests.cpp.")
output.append("// Emits only the HudSensorTracker objective smokes needed by functional manifests.")
output.append("")
output.append(prefix.rstrip())
output.append("")
for function_name in [
    "hud_sensor_objective_slot_reset_smoke",
    "hud_sensor_tracker_unload_objectives_smoke",
    "hud_sensor_tracker_load_objectives_from_path_smoke",
    "hud_sensor_tracker_load_objectives_from_zrd_smoke",
]:
    output.append(extract_function(function_name).rstrip())
    output.append("")

output_path.write_text("\n".join(output))
]=])
execute_process(
    COMMAND ${Python3_EXECUTABLE}
        "${HUD_SENSOR_TRACKER_OBJECTIVE_SMOKE_GENERATOR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/zgame_tests.cpp"
        "${HUD_SENSOR_TRACKER_OBJECTIVE_SMOKE_SOURCE}"
    RESULT_VARIABLE HUD_SENSOR_TRACKER_OBJECTIVE_SMOKE_GENERATOR_RESULT
)
if(NOT HUD_SENSOR_TRACKER_OBJECTIVE_SMOKE_GENERATOR_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate HudSensorTracker objective smoke source")
endif()
target_sources(recoil_native_smoke PRIVATE "${HUD_SENSOR_TRACKER_OBJECTIVE_SMOKE_SOURCE}")

set(ZMODEL_SHUTDOWN_SMOKE_SOURCE
    "${CMAKE_CURRENT_BINARY_DIR}/zmodel_shutdown_smokes.cpp"
)
set(ZMODEL_SHUTDOWN_SMOKE_GENERATOR
    "${CMAKE_CURRENT_BINARY_DIR}/generate_zmodel_shutdown_smokes.py"
)
file(WRITE "${ZMODEL_SHUTDOWN_SMOKE_GENERATOR}" [=[
from pathlib import Path
import sys

source_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
text = source_path.read_text()

include_end = text.index('\n#ifndef RECOIL_ZGAME_TESTS_ZMODEL_CONST_ONLY')
namespace_start = text.index('\nnamespace {', include_end) + 1
namespace_end = text.index('\nextern "C" int zgame_return_only_stub_smoke')
prefix = text[:include_end] + "\n\n" + text[namespace_start:namespace_end]


def skip_literal(text, index):
    quote = text[index]
    index += 1
    while index < len(text):
        if text[index] == "\\":
            index += 2
            continue
        if text[index] == quote:
            return index + 1
        index += 1
    return index


def skip_line_comment(text, index):
    newline = text.find("\n", index + 2)
    return len(text) if newline == -1 else newline + 1


def skip_block_comment(text, index):
    end = text.find("*/", index + 2)
    return len(text) if end == -1 else end + 2


def extract_function(name):
    needle = f"{name}("
    name_pos = text.index(needle)
    start = text.rfind("\n", 0, name_pos) + 1
    index = text.index("{", start)
    depth = 0
    while index < len(text):
        if text[index] in ('"', "'"):
            index = skip_literal(text, index)
            continue
        if text.startswith("//", index):
            index = skip_line_comment(text, index)
            continue
        if text.startswith("/*", index):
            index = skip_block_comment(text, index)
            continue
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
        index += 1
    raise RuntimeError(f"could not find end of {name}")


output = []
output.append("// Generated by tests/native/CMakeLists.txt from zgame_tests.cpp.")
output.append("// Emits only the zModel shutdown smokes needed by functional manifests.")
output.append("")
output.append(prefix.rstrip())
output.append("")
for function_name in [
    "zmodel_display_shutdown_smoke",
    "zmodel_matlslot_release_smoke",
    "zmodel_matlbuffer_release_all_active_smoke",
]:
    output.append(extract_function(function_name).rstrip())
    output.append("")

output_path.write_text("\n".join(output))
]=])
execute_process(
    COMMAND ${Python3_EXECUTABLE}
        "${ZMODEL_SHUTDOWN_SMOKE_GENERATOR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/zgame_tests.cpp"
        "${ZMODEL_SHUTDOWN_SMOKE_SOURCE}"
    RESULT_VARIABLE ZMODEL_SHUTDOWN_SMOKE_GENERATOR_RESULT
)
if(NOT ZMODEL_SHUTDOWN_SMOKE_GENERATOR_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate zModel shutdown smoke source")
endif()
target_sources(recoil_native_smoke PRIVATE "${ZMODEL_SHUTDOWN_SMOKE_SOURCE}")

set(ZHUD_SENSOR_OBJECTIVE_SMOKE_SOURCE
    "${CMAKE_CURRENT_BINARY_DIR}/zhud_sensor_objective_smokes.cpp"
)
set(ZHUD_SENSOR_OBJECTIVE_SMOKE_GENERATOR
    "${CMAKE_CURRENT_BINARY_DIR}/generate_zhud_sensor_objective_smokes.py"
)
file(WRITE "${ZHUD_SENSOR_OBJECTIVE_SMOKE_GENERATOR}" [=[
from pathlib import Path
import sys

source_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
text = source_path.read_text()

branch_start = text.index('\n#else\n\n#include "Battlesport/GameNet.h"') + len('\n#else\n\n')
prefix_end = text.index('\nnamespace {', branch_start)
prefix = text[branch_start:prefix_end]


def skip_literal(text, index):
    quote = text[index]
    index += 1
    while index < len(text):
        if text[index] == "\\":
            index += 2
            continue
        if text[index] == quote:
            return index + 1
        index += 1
    return index


def skip_line_comment(text, index):
    newline = text.find("\n", index + 2)
    return len(text) if newline == -1 else newline + 1


def skip_block_comment(text, index):
    end = text.find("*/", index + 2)
    return len(text) if end == -1 else end + 2


def extract_function(name):
    needle = f"{name}("
    name_pos = text.index(needle)
    start = text.rfind("\n", 0, name_pos) + 1
    index = text.index("{", start)
    depth = 0
    while index < len(text):
        if text[index] in ('"', "'"):
            index = skip_literal(text, index)
            continue
        if text.startswith("//", index):
            index = skip_line_comment(text, index)
            continue
        if text.startswith("/*", index):
            index = skip_block_comment(text, index)
            continue
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
        index += 1
    raise RuntimeError(f"could not find end of {name}")


def extract_span(start_needle, end_needle):
    start = text.index(start_needle)
    end = text.index(end_needle, start) + len(end_needle)
    return text[start:end]


def strip_legacy_ftable_assignments(function_text):
    output = []
    skipping = False
    for line in function_text.splitlines():
        if skipping:
            if ";" in line:
                skipping = False
            continue
        if ".ftable" in line and "=" in line:
            if ";" not in line:
                skipping = True
            continue
        output.append(line)
    return "\n".join(output)


output = []
output.append("// Generated by tests/native/CMakeLists.txt from zhud_ui_tests.cpp.")
output.append("// Emits only the zHud/HudSensorTracker objective smokes needed by functional manifests.")
output.append("")
output.append(prefix.rstrip())
output.append("")
output.append("namespace {")
output.append(extract_span(
    "template <typename T> T &TestFieldAt",
    "return delta > -0.0001f && delta < 0.0001f;\n}"
).rstrip())
output.append("")
output.append(extract_span(
    "struct TestReticleAttachState",
    "zClass_NodePartial *rootNode;\n};"
).rstrip())
output.append("} // namespace")
output.append("")
for function_name in [
    "zhud_sensor_viewport_rect_smoke",
    "hud_sensor_tracker_show_objective_pickup_info_smoke",
    "hud_sensor_tracker_objective_panel_visible_smoke",
    "hud_sensor_tracker_shutdown_smoke",
]:
    output.append(strip_legacy_ftable_assignments(extract_function(function_name)).rstrip())
    output.append("")

output_path.write_text("\n".join(output))
]=])
execute_process(
    COMMAND ${Python3_EXECUTABLE}
        "${ZHUD_SENSOR_OBJECTIVE_SMOKE_GENERATOR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/zhud_ui_tests.cpp"
        "${ZHUD_SENSOR_OBJECTIVE_SMOKE_SOURCE}"
    RESULT_VARIABLE ZHUD_SENSOR_OBJECTIVE_SMOKE_GENERATOR_RESULT
)
if(NOT ZHUD_SENSOR_OBJECTIVE_SMOKE_GENERATOR_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate zHud sensor objective smoke source")
endif()
target_sources(recoil_native_smoke PRIVATE "${ZHUD_SENSOR_OBJECTIVE_SMOKE_SOURCE}")

set(BRIEFING_SET_PROGRESS_SMOKE_SOURCE
    "${CMAKE_CURRENT_BINARY_DIR}/briefing_set_progress_smoke.cpp"
)
set(BRIEFING_SET_PROGRESS_SMOKE_GENERATOR
    "${CMAKE_CURRENT_BINARY_DIR}/generate_briefing_set_progress_smoke.py"
)
file(WRITE "${BRIEFING_SET_PROGRESS_SMOKE_GENERATOR}" [=[
from pathlib import Path
import sys

source_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
text = source_path.read_text()

prefix_marker = 'extern "C" int briefing_stop_and_shutdown_thread_smoke'
prefix_end = text.index(prefix_marker)


def skip_literal(text, index):
    quote = text[index]
    index += 1
    while index < len(text):
        if text[index] == "\\":
            index += 2
            continue
        if text[index] == quote:
            return index + 1
        index += 1
    return index


def skip_line_comment(text, index):
    newline = text.find("\n", index + 2)
    return len(text) if newline == -1 else newline + 1


def skip_block_comment(text, index):
    end = text.find("*/", index + 2)
    return len(text) if end == -1 else end + 2


def extract_function(name):
    needle = f"{name}("
    name_pos = text.index(needle)
    start = text.rfind("\n", 0, name_pos) + 1
    index = text.index("{", start)
    depth = 0
    while index < len(text):
        if text[index] in ('"', "'"):
            index = skip_literal(text, index)
            continue
        if text.startswith("//", index):
            index = skip_line_comment(text, index)
            continue
        if text.startswith("/*", index):
            index = skip_block_comment(text, index)
            continue
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
        index += 1
    raise RuntimeError(f"could not find end of {name}")


output = []
output.append("// Generated by tests/native/CMakeLists.txt from briefing_tests.cpp.")
output.append("// Emits the Briefing progress smoke without broad briefing test duplicates.")
output.append("")
output.append(text[:prefix_end].rstrip())
output.append("")
output.append(extract_function("briefing_set_progress_and_sleep_smoke").rstrip())
output.append("")

output_path.write_text("\n".join(output))
]=])
execute_process(
    COMMAND ${Python3_EXECUTABLE}
        "${BRIEFING_SET_PROGRESS_SMOKE_GENERATOR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/briefing_tests.cpp"
        "${BRIEFING_SET_PROGRESS_SMOKE_SOURCE}"
    RESULT_VARIABLE BRIEFING_SET_PROGRESS_SMOKE_GENERATOR_RESULT
)
if(NOT BRIEFING_SET_PROGRESS_SMOKE_GENERATOR_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate Briefing set-progress smoke source")
endif()
target_sources(recoil_native_smoke PRIVATE "${BRIEFING_SET_PROGRESS_SMOKE_SOURCE}")

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
#include "GameZRecoil/zNetwork/zNetwork.h"
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
