"""Serial recover candidate iat and abi proof phase."""
from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import abi as _cc_abi
from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import callbacks as _cc_callbacks
from _recoil.call_contract import candidate_imports as _cc_candidate_imports
from _recoil.call_contract import dispatch as _cc_dispatch
from _recoil.call_contract import iat as _cc_iat
from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import recoil_hud_fonts as _cc_recoil_hud_fonts
from _recoil.call_contract import recoil_hud_objectives as _cc_recoil_hud_objectives
from _recoil.call_contract import recoil_hud_sensor as _cc_recoil_hud_sensor
from _recoil.call_contract import recoil_hud_sensor_track as _cc_recoil_hud_sensor_track
from _recoil.call_contract import recoil_hud_widgets as _cc_recoil_hud_widgets
from _recoil.call_contract import recoil_input as _cc_recoil_input
from _recoil.call_contract import recoil_lifecycle as _cc_recoil_lifecycle
from _recoil.call_contract import recoil_math as _cc_recoil_math
from _recoil.call_contract import recoil_mfc as _cc_recoil_mfc
from _recoil.call_contract import recoil_player as _cc_recoil_player
from _recoil.call_contract import recoil_weapons as _cc_recoil_weapons
from _recoil.call_contract import recoil_world as _cc_recoil_world
from _recoil.call_contract import retail as _cc_retail
from _recoil.call_contract import work as _cc_work

if TYPE_CHECKING:
    from _recoil.call_contract.records import ReviewedExactIndirectStorageBridge



def recover_candidate_iat_and_abi(work: _cc_work.CallerWork) -> None:

    (
        work.eh_array_destructor_bridges,
        work.indexes,
    ) = _cc_recoil_lifecycle._preflight_eh_array_destructor_candidate_identity(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
        bridge=work.bridge,
    )
    _cc_recoil_lifecycle._assert_eh_array_destructor_candidate_identity_stage(
        work.eh_array_destructor_bridges,
        work.indexes,
        stage="candidate-entry-preflight",
    )
    (
        work.candidate_assembly,
        work.zinput_candidate_switch_indices,
    ) = _cc_recoil_input._zinput_runtime_candidate_switch_extraction_package(
        work.candidate_assembly,
        caller_start=work.address,
    )
    (
        work.candidate_assembly,
        zinput_translate_switch_indices,
    ) = _cc_recoil_input._zinput_translate_candidate_switch_extraction_package(
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    if work.zinput_candidate_switch_indices & zinput_translate_switch_indices:
        raise ValueError("zInput reviewed switch packages collide")
    work.zinput_candidate_switch_indices = (
        work.zinput_candidate_switch_indices
        | zinput_translate_switch_indices
    )
    (
        work.gettickcount_candidate_direct_bridges,
        gettickcount_candidate_register_storage_bridges,
        work.gettickcount_candidate_retail_iat_equivalences,
    ) = _cc_candidate_imports._gettickcount_candidate_bridges_for_caller(
        work.gettickcount_retail_e8_call_population,
        work.expected,
        work.candidate_assembly,
        thunk=work.gettickcount_named_import_thunk,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
    )
    _cc_proofs.merge_into(work.candidate_register_storage_bridges, gettickcount_candidate_register_storage_bridges, family="session.candidate_register_storage_bridges")
    work.zsnd_free_iat_definition_offsets = (
        _cc_retail._zsnd_destroy_owned_data_free_candidate_iat_provenance(
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    (
        work.zeffect_fwrite_iat_definition_offsets,
        zeffect_fwrite_register_bridges,
    ) = (
        _cc_retail._zeffect_save_running_anim_record_fwrite_candidate_iat_provenance(
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.candidate_register_storage_bridges, zeffect_fwrite_register_bridges, family="session.candidate_register_storage_bridges")
    qsand_callback_register_bridges = (
        _cc_recoil_world._zdeclient_qsand_callback_candidate_register_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.candidate_register_storage_bridges, qsand_callback_register_bridges, family="session.candidate_register_storage_bridges")
    crater_callback_register_bridges = (
        _cc_recoil_world._zdeclient_crater_callback_candidate_register_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.candidate_register_storage_bridges, crater_callback_register_bridges, family="session.candidate_register_storage_bridges")
    (
        wol_category_a_atoi_bridges,
        wol_category_a_atoi_proofs,
    ) = (
        _cc_recoil_mfc._wol_category_a_atoi_register_storage_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.candidate_register_storage_bridges, wol_category_a_atoi_bridges, family="session.candidate_register_storage_bridges")
    (
        named_thunk_candidate_iat_bridges,
        work.named_thunk_candidate_iat_equivalences,
        named_thunk_candidate_canonical_import_names,
    ) = _cc_candidate_imports._provider_named_import_thunk_candidate_iat_storage_bridges(
        work.expected,
        work.candidate_assembly,
        thunks=work._provider_named_import_thunks,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
    )
    (
        work.named_thunk_candidate_direct_bridges,
        work.named_thunk_candidate_direct_equivalences,
    ) = _cc_candidate_imports._provider_named_import_thunk_candidate_direct_bridges(
        work.expected,
        work.candidate_assembly,
        thunks=work._provider_named_import_thunks,
        indexes=work.indexes,
        retail_call_sites=work.retail_invocation_call_sites,
        retail_import_targets=work.retail_import_targets,
    )
    # The ordinal-import bridge performs a provisional extraction of
    # the complete candidate caller.  Build the generic exact-IAT
    # register package before that first consumer, then reuse the
    # identical indexes/proofs for the final extraction below.  This
    # keeps independent definitions (including unrelated named IAT
    # reloads) available to the same fail-closed CFG traversal.
    work.candidate_exact_iat_indexes = (
        _cc_iat._candidate_exact_iat_storage_indexes(
            work.indexes,
            named_thunk_candidate_iat_bridges,
            work.candidate_register_storage_bridges,
            reviewed_canonical_import_names=(
                named_thunk_candidate_canonical_import_names
            ),
        )
    )
    _cc_recoil_lifecycle._assert_eh_array_destructor_candidate_identity_stage(
        work.eh_array_destructor_bridges,
        work.candidate_exact_iat_indexes,
        stage="exact-IAT-provisional-index",
    )
    work.candidate_exact_iat_load_proofs = (
        _cc_iat._candidate_exact_iat_register_load_proofs(
            work.candidate_assembly,
            indexes=work.candidate_exact_iat_indexes,
        )
    )
    _cc_proofs.merge_into(work.candidate_exact_iat_load_proofs, wol_category_a_atoi_proofs, family="session.candidate_exact_iat_load_proofs")
    work.ciasin_provider_bridges = _cc_recoil_weapons._ciasin_provider_candidate_bridge(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
    )
    work.zwep_ciacos_provider_iat_equivalences: dict[str, str] = {}
    work.zwep_ciasin_provider_bridges = (
        _cc_recoil_weapons._zwep_ciasin_provider_candidate_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            named_import_thunks=work._provider_named_import_thunks,
            reviewed_provider_iat_equivalences_out=(
                work.zwep_ciacos_provider_iat_equivalences
            ),
        )
    )
    work.wol_cedit_provider_bridges = (
        _cc_recoil_mfc._wol_cedit_provider_candidate_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            ordinal_import_thunks=work.provider_ordinal_import_thunks,
        )
    )
    work.wol_category_a_direct_bridges = (
        _cc_recoil_mfc._wol_category_a_direct_candidate_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
        )
    )
    work.chkstk_compiler_helper_bridges = (
        _cc_callable_identity._chkstk_compiler_helper_candidate_bridge(
            work.expected,
            work.retail_instructions,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            bridge=work.bridge,
        )
    )
    work.cls_util_authored_identity_bridges = (
        _cc_recoil_world._cls_util_candidate_direct_authored_identity_closure(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
        )
    )
    work.candidate_storage_bridges = (
        _cc_retail._briefing_objective_picture_vftable_storage_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            bridge=work.bridge,
        )
    )
    work.r4564_callback_exact_indirect_bridges: dict[
        str, ReviewedExactIndirectStorageBridge
    ] = {}
    r4564_callback_storage_bridges = (
        _cc_callbacks._r4564_candidate_callback_storage_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            indexes=work.indexes,
            reviewed_exact_indirect_storage_bridges_out=(
                work.r4564_callback_exact_indirect_bridges
            ),
        )
    )
    _cc_proofs.merge_into(work.candidate_storage_bridges, r4564_callback_storage_bridges, family="session.candidate_storage_bridges")
    locator_panel_bridges = (
        _cc_retail._briefing_locator_panel_vftable_storage_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            bridge=work.bridge,
        )
    )
    runtime_bridges = _cc_retail._briefing_runtime_vftable_storage_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
        bridge=work.bridge,
    )
    hud_element_constructor_table_bridges = (
        _cc_retail._hud_ui_element_constructor_absolute_table_candidate_storage_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    hud_container_constructor_table_bridges = (
        _cc_retail._hud_ui_container_constructor_absolute_table_candidate_storage_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    for reviewed_bridges in (
        locator_panel_bridges,
        runtime_bridges,
        hud_element_constructor_table_bridges,
        hud_container_constructor_table_bridges,
    ):
        _cc_proofs.merge_into(work.candidate_storage_bridges, reviewed_bridges, family="session.candidate_storage_bridges")
    zmath_camera_negate_bridges = (
        _cc_recoil_math._zmath_camera_negate_float_sign_bit_candidate_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            compiler_generated_bridges={},
        )
    )
    work.registered_regex_bridges = (
        _cc_dispatch._registered_target_symbol_regex_direct_candidate_bridges(
            work.candidate_assembly,
            document=work.document,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            compiler_generated_bridges={
                **zmath_camera_negate_bridges,
            },
        )
    )
    authored_decorated_abi_bridges = (
        _cc_abi._authored_decorated_abi_direct_candidate_bridges(
            work.candidate_assembly,
            document=work.document,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            compiler_generated_bridges={
                **zmath_camera_negate_bridges,
                **work.registered_regex_bridges,
            },
            bridge=work.bridge,
            retail_abi_cache=work.authored_decorated_abi_retail_cache,
        )
    )
    registered_decorated_target_bridges = (
        _cc_dispatch._registered_decorated_target_direct_candidate_bridges(
            work.candidate_assembly,
            document=work.document,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            compiler_generated_bridges={
                **zmath_camera_negate_bridges,
                **work.registered_regex_bridges,
                **authored_decorated_abi_bridges,
            },
        )
    )
    _cc_proofs.merge_into(work.registered_regex_bridges, authored_decorated_abi_bridges, family="session.registered_regex_bridges")
    _cc_proofs.merge_into(work.registered_regex_bridges, registered_decorated_target_bridges, family="session.registered_regex_bridges")
    _cc_proofs.merge_into(work.registered_regex_bridges, zmath_camera_negate_bridges, family="session.registered_regex_bridges")
    # The compiler-lifecycle identity was resolved and installed at
    # candidate-entry before any producer could preflight this caller.
    early_direct_bridges = dict(work.registered_regex_bridges)
    _cc_proofs.merge_into(early_direct_bridges, work.eh_array_destructor_bridges, family="session.early_direct_bridges")
    work.expected_direct_identity_bridges = (
        _cc_callable_identity._expected_direct_candidate_identity_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            existing_bridges=early_direct_bridges,
        )
    )
    _cc_recoil_lifecycle._assert_eh_array_destructor_candidate_identity_stage(
        work.eh_array_destructor_bridges,
        work.indexes,
        stage="expected-direct-producer",
        final_bridges=early_direct_bridges,
    )
    work.r4564_provider_iat_equivalences: dict[str, str] = {}
    r4564_non_source_direct_bridges = (
        _cc_callable_identity._r4564_non_source_candidate_direct_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            reviewed_provider_iat_equivalences_out=(
                work.r4564_provider_iat_equivalences
            ),
        )
    )
    work.player_compiler_provider_bridges = (
        _cc_recoil_player._r4572_player_compiler_provider_candidate_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.expected_direct_identity_bridges, r4564_non_source_direct_bridges, family="session.expected_direct_identity_bridges")
    _cc_proofs.merge_into(work.expected_direct_identity_bridges, work.player_compiler_provider_bridges, family="session.expected_direct_identity_bridges")
    work.ensure_font_loop_candidate_vptr_bridges = (
        _cc_recoil_hud_fonts._hud_ui_mgr_ensure_font_loop_candidate_vptr_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    work.track_marker_loop_candidate_vptr_bridges = (
        _cc_recoil_hud_sensor_track._hud_ui_mgr_sensor_track_marker_loop_candidate_vptr_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    work.track_marker_exit_candidate_vptr_bridges = (
        _cc_recoil_hud_sensor_track._hud_ui_mgr_sensor_track_marker_exit_candidate_vptr_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    ensure_sensor_center_absolute_load_bridges = (
        _cc_recoil_hud_sensor._hud_ui_mgr_ensure_sensor_center_candidate_absolute_load_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    ensure_sensor_meter_absolute_load_bridges = (
        _cc_recoil_hud_sensor._hud_ui_mgr_ensure_sensor_meter_setclip_candidate_absolute_load_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    work.ensure_sensor_absolute_load_bridges = dict(
        ensure_sensor_center_absolute_load_bridges
    )
    _cc_proofs.merge_into(work.ensure_sensor_absolute_load_bridges, ensure_sensor_meter_absolute_load_bridges, family="session.ensure_sensor_absolute_load_bridges")
    work.hud_ui_mgr_zrd_payload_bridges = (
        _cc_recoil_hud_widgets._hud_ui_mgr_zrd_payload_tu_local_candidate_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            compiler_generated_bridges=work.registered_regex_bridges,
        )
    )
    ensure_objective_sensor_absolute_load_bridges = (
        _cc_recoil_hud_sensor._hud_ui_mgr_ensure_objective_sensor_center_candidate_absolute_load_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            zrd_payload_bridges=work.hud_ui_mgr_zrd_payload_bridges,
        )
    )
    _cc_proofs.merge_into(work.ensure_sensor_absolute_load_bridges, ensure_objective_sensor_absolute_load_bridges, family="session.ensure_sensor_absolute_load_bridges")
    ensure_objective_widget_absolute_load_bridges = (
        _cc_recoil_hud_objectives._hud_ui_mgr_ensure_objective_widget_center_candidate_absolute_load_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.ensure_sensor_absolute_load_bridges, ensure_objective_widget_absolute_load_bridges, family="session.ensure_sensor_absolute_load_bridges")
    ensure_objective_text_setpos_absolute_load_bridges = (
        _cc_recoil_hud_objectives._hud_ui_mgr_ensure_objective_text_setpos_candidate_absolute_load_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.ensure_sensor_absolute_load_bridges, ensure_objective_text_setpos_absolute_load_bridges, family="session.ensure_sensor_absolute_load_bridges")
