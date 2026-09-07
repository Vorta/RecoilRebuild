"""Serial recover retail receivers proof phase."""
from __future__ import annotations

from _recoil.call_contract import comparison as _cc_comparison
from _recoil.call_contract import contributions as _cc_contributions
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import recoil_application as _cc_recoil_application
from _recoil.call_contract import recoil_hud_fonts as _cc_recoil_hud_fonts
from _recoil.call_contract import recoil_hud_layout as _cc_recoil_hud_layout
from _recoil.call_contract import recoil_hud_messages as _cc_recoil_hud_messages
from _recoil.call_contract import recoil_hud_objectives as _cc_recoil_hud_objectives
from _recoil.call_contract import recoil_hud_sensor as _cc_recoil_hud_sensor
from _recoil.call_contract import recoil_hud_sensor_track as _cc_recoil_hud_sensor_track
from _recoil.call_contract import recoil_hud_visibility as _cc_recoil_hud_visibility
from _recoil.call_contract import recoil_hud_widgets as _cc_recoil_hud_widgets
from _recoil.call_contract import recoil_input as _cc_recoil_input
from _recoil.call_contract import work as _cc_work


def recover_retail_receivers(work: _cc_work.CallerWork) -> None:

    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, work.clear_display_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    (
        work.indexes,
        update_weapon_retail_vptr_bridges,
        work.update_weapon_candidate_vptr_bridges,
    ) = _cc_recoil_hud_messages._hud_ui_message_update_selected_weapon_vptr_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, update_weapon_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    (
        rebuild_weapon_retail_vptr_bridges,
        work.rebuild_weapon_candidate_vptr_bridges,
    ) = _cc_recoil_hud_layout._hud_ui_message_rebuild_weapon_layout_vptr_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, rebuild_weapon_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    (
        track_counter_retail_vptr_bridges,
        work.track_counter_candidate_vptr_bridges,
    ) = _cc_recoil_hud_sensor_track._hud_ui_mgr_sensor_track_counter_slot_vptr_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, track_counter_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    (
        track_counter_switch_retail_vptr_bridges,
        work.track_counter_switch_candidate_vptr_bridges,
    ) = _cc_recoil_hud_sensor_track._hud_ui_mgr_sensor_track_counter_switch_vptr_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, track_counter_switch_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    ensure_font_loop_vptr_bridges = (
        _cc_recoil_hud_fonts._hud_ui_mgr_ensure_font_loop_retail_vptr_bridges(
            work.retail_instructions,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_recoil_hud_sensor._hud_ui_mgr_ensure_sensor_center_retail_guard(
        work.retail_instructions,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_recoil_hud_sensor._hud_ui_mgr_ensure_sensor_meter_setclip_retail_guard(
        work.retail_instructions,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_recoil_hud_sensor._hud_ui_mgr_ensure_objective_sensor_center_retail_guard(
        work.retail_instructions,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_recoil_hud_objectives._hud_ui_mgr_ensure_objective_widget_center_retail_guard(
        work.retail_instructions,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_recoil_hud_objectives._hud_ui_mgr_ensure_objective_text_setpos_retail_guard(
        work.retail_instructions,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_recoil_hud_objectives._hud_ui_mgr_ensure_objective_label_setpos_retail_guard(
        work.retail_instructions,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    ensure_objective_label_settextfmt_retail_vptr_bridges = (
        _cc_recoil_hud_objectives._hud_ui_mgr_ensure_objective_label_settextfmt_retail_vptr_bridge(
            work.retail_instructions,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, ensure_font_loop_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, ensure_objective_label_settextfmt_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    _cc_recoil_hud_visibility._hud_ui_mgr_stats_list_set_visible_retail_guard(
        work.retail_instructions,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    apply_text_line_retail_vptr_bridges = (
        _cc_recoil_hud_widgets._hud_ui_aux_overlay_apply_text_line_retail_vptr_bridges(
            work.retail_instructions,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, apply_text_line_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    enable_stacks_retail_vptr_bridges = (
        _cc_recoil_hud_widgets._hud_ui_mgr_enable_stacks_retail_vptr_bridges(
            work.retail_instructions,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, enable_stacks_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    disable_stacks_retail_vptr_bridges = (
        _cc_recoil_hud_widgets._hud_ui_mgr_disable_stacks_retail_vptr_bridges(
            work.retail_instructions,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, disable_stacks_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    apply_text_label_retail_vptr_bridges = (
        _cc_recoil_hud_layout._hud_layout_apply_text_label_retail_vptr_bridges(
            work.retail_instructions,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, apply_text_label_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    (
        main_menu_blur_begin_eax_bridges,
        work.main_menu_blur_candidate_bridges,
    ) = (
        _cc_recoil_application._recoil_main_menu_transition_blur_begin_eax_bridge(
            work.retail_instructions,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge=work.bridge,
        )
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, main_menu_blur_begin_eax_bridges, family="session.retail_layout_loop_vptr_bridges")
    work.trace_caller(
        "expected-extraction-start",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
    )
    work.retail_invocation_call_sites: list[str] = []
    work.expected = _cc_extraction.extract_invocation_contract(
        work.retail_instructions,
        source="bn",
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.retail_comparison_end_exclusive,
        indexes=work.indexes,
        bridge_names=work.retail_extraction_bridge_names,
        reviewed_register_storage_bridges=(
            work.retail_register_storage_bridges
        ),
        reviewed_register_call_storage_bridges=(
            work.reviewed_retail_adapters.register_call_storage_bridges
        ),
        reviewed_call_result_bridges=work.retail_call_result_bridges,
        reviewed_loop_vptr_storage_bridges=(
            work.retail_layout_loop_vptr_bridges
        ),
        reviewed_exact_indirect_storage_bridges=(
            work.reviewed_retail_adapters
            .exact_indirect_storage_bridges
        ),
        reviewed_inbound_entry_register_target_bridges=(
            work.inbound_entry_target_bridges
        ),
        reviewed_inbound_entry_register_roots=(
            work.inbound_entry_register_roots
        ),
        local_control_flow_indices=frozenset(work.retail_switch_targets),
        local_control_flow_targets=work.retail_switch_targets,
        retail_proof_package=work.retail_proof_package,
        invocation_call_sites_out=work.retail_invocation_call_sites,
    )
    work.retail_physical_contributions = _cc_contributions.physical_contributions(work.retail_instructions, work.retail_invocation_call_sites, side="retail", caller_start=work.address)
    (
        work.expected,
        work.retail_invocation_call_sites,
    ) = _cc_recoil_input._zinput_runtime_dispatch_normalize_retail_contract(
        work.expected,
        work.retail_invocation_call_sites,
        caller_start=work.address,
        indexes=work.indexes,
    )
    work.expected = _cc_comparison._recoilapp_reviewed_retail_cleanup_contract(
        work.expected,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
    )
    if len(work.retail_invocation_call_sites) != len(work.expected):
        raise ValueError(
            "retail invocation call-site population does not match its "
            "complete extracted contract"
        )
    work.trace_caller(
        "expected-extraction-complete",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
        expected_invocation_count=len(work.expected),
        retail_invocation_call_site_count=len(
            work.retail_invocation_call_sites
        ),
    )
