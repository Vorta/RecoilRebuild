"""Serial recover candidate receivers proof phase."""
from __future__ import annotations

from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import receiver_equivalence as _cc_receiver_equivalence
from _recoil.call_contract import recoil_hud_layout as _cc_recoil_hud_layout
from _recoil.call_contract import recoil_hud_panels as _cc_recoil_hud_panels
from _recoil.call_contract import recoil_hud_reticle as _cc_recoil_hud_reticle
from _recoil.call_contract import recoil_hud_timers_fonts as _cc_recoil_hud_timers_fonts
from _recoil.call_contract import recoil_weapons as _cc_recoil_weapons
from _recoil.call_contract import retail as _cc_retail
from _recoil.call_contract import work as _cc_work


def recover_candidate_receivers(work: _cc_work.CallerWork) -> None:

    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.zinput_joystick_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    percent_font_member_vptr_bridges = (
        _cc_recoil_hud_timers_fonts._hud_ui_mgr_percent_font_candidate_vptr_storage_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.enable_current_layout_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.disable_current_layout_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.disable_visibility_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.objective_visibility_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.objective_show_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.objective_show_desc_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.objective_begin_summary_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.objective_begin_desc_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.update_frame_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, percent_font_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.stats_list_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.first_delete_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.timer_panel_delete_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.objective_counter_delete_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.refresh_counter_panel_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.layout_set_active_timer_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    _cc_proofs.merge_into(work.member_vptr_storage_bridges, work.remaining_delete_member_vptr_bridges, family="session.member_vptr_storage_bridges")
    work.candidate_vptr_storage_bridges = (
        _cc_recoil_hud_panels._hud_counter_text_panel_constructor_vptr_storage_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            bridge=work.bridge,
            compiler_generated_bridges=(
                work.compiler_generated_bridges
            ),
            candidate_storage_bridges=work.candidate_storage_bridges,
            reviewed_register_storage_bridges=(
                work.candidate_register_storage_bridges
            ),
            reviewed_call_result_bridges=(
                work.candidate_call_result_bridges
            ),
        )
    )
    timer_panel_float_vptr_bridges = (
        _cc_retail._hud_timer_panel_float_candidate_vptr_storage_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    reticle_widget_vptr_bridges = (
        _cc_recoil_hud_reticle._hud_ui_mgr_reticle_widget_candidate_vptr_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, timer_panel_float_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.ensure_remaining_indirect_bridges, reticle_widget_vptr_bridges, family="session.ensure_remaining_indirect_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.viewport_layout_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.trigger_current_layout_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.switch_active_dialog_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.set_float_timer_visible_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.set_aux_overlay_visible_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.enable_hud_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.disable_set_enabled_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.disable_visibility_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_visibility_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_update_meter_x_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_show_sensor_overlay_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_show_widget_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_show_bar_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_begin_sensor_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_start_hide_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_start_hide_widget_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_start_hide_widget_center_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_start_hide_topology_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.objective_update_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    _cc_proofs.merge_into(work.candidate_vptr_storage_bridges, work.ensure_objective_label_settextfmt_candidate_vptr_bridges, family="session.candidate_vptr_storage_bridges")
    work.candidate_layout_loop_vptr_bridges = (
        _cc_recoil_hud_layout._hud_ui_mgr_layout_array_loop_vptr_storage_bridges(
            work.candidate_assembly.instructions,
            source="cod",
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            candidate=work.candidate_assembly,
        )
    )
    exact_body_vptr_bridges = _cc_receiver_equivalence._exact_body_vptr_candidate_bridges(
        work.candidate_assembly,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        expected=work.expected,
        retail_call_sites=work.retail_invocation_call_sites,
    )
    for site, proof in exact_body_vptr_bridges.items():
        prior = work.candidate_layout_loop_vptr_bridges.get(site)
        if prior is not None and prior != proof:
            raise ValueError("exact relocated-body virtual-call proof conflicts with another lineage")
        work.candidate_layout_loop_vptr_bridges[site] = proof
    work.zwep_damage_candidate_vptr_bridges = (
        _cc_recoil_weapons._zwep_damage_handler_candidate_vptr_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
