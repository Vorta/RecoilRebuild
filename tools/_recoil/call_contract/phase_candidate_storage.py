"""Serial recover candidate storage proof phase."""
from __future__ import annotations

from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import recoil_hud_frame as _cc_recoil_hud_frame
from _recoil.call_contract import recoil_hud_layout as _cc_recoil_hud_layout
from _recoil.call_contract import recoil_hud_objectives as _cc_recoil_hud_objectives
from _recoil.call_contract import recoil_hud_sensor as _cc_recoil_hud_sensor
from _recoil.call_contract import recoil_hud_visibility as _cc_recoil_hud_visibility
from _recoil.call_contract import work as _cc_work


def recover_candidate_storage(work: _cc_work.CallerWork) -> None:

    (
        disable_visibility_static_bridges,
        disable_visibility_absolute_bridges,
        work.disable_visibility_vptr_bridges,
        work.disable_visibility_member_vptr_bridges,
    ) = _cc_recoil_hud_visibility._hud_ui_mgr_disable_visibility_cluster_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        objective_visibility_static_bridges,
        objective_visibility_absolute_bridges,
        work.objective_visibility_vptr_bridges,
        work.objective_visibility_member_vptr_bridges,
    ) = _cc_recoil_hud_visibility._hud_ui_mgr_objective_visibility_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        objective_update_meter_x_absolute_bridges,
        work.objective_update_meter_x_vptr_bridges,
    ) = (
        _cc_recoil_hud_objectives._hud_ui_mgr_objective_update_meter_x_candidate_vptr_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    (
        objective_show_absolute_bridges,
        work.objective_show_member_vptr_bridges,
    ) = _cc_recoil_hud_objectives._hud_ui_mgr_objective_show_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        objective_show_desc_absolute_bridges,
        work.objective_show_desc_member_vptr_bridges,
    ) = _cc_recoil_hud_objectives._hud_ui_mgr_objective_show_desc_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        objective_show_sensor_overlay_absolute_bridges,
        work.objective_show_sensor_overlay_vptr_bridges,
    ) = (
        _cc_recoil_hud_sensor._hud_ui_mgr_objective_show_sensor_overlay_candidate_vptr_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    (
        objective_show_widget_absolute_bridges,
        work.objective_show_widget_vptr_bridges,
    ) = _cc_recoil_hud_objectives._hud_ui_mgr_objective_show_widget_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        objective_show_bar_absolute_bridges,
        work.objective_show_bar_vptr_bridges,
    ) = _cc_recoil_hud_objectives._hud_ui_mgr_objective_show_bar_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        objective_begin_summary_absolute_bridges,
        work.objective_begin_summary_member_vptr_bridges,
    ) = _cc_recoil_hud_objectives._hud_ui_mgr_objective_begin_summary_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        objective_begin_desc_absolute_bridges,
        work.objective_begin_desc_member_vptr_bridges,
    ) = _cc_recoil_hud_objectives._hud_ui_mgr_objective_begin_desc_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        objective_begin_sensor_absolute_bridges,
        work.objective_begin_sensor_vptr_bridges,
    ) = _cc_recoil_hud_sensor._hud_ui_mgr_objective_begin_sensor_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        objective_start_hide_absolute_bridges,
        work.objective_start_hide_vptr_bridges,
    ) = _cc_recoil_hud_objectives._hud_ui_mgr_objective_start_hide_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        objective_start_hide_widget_absolute_bridges,
        work.objective_start_hide_widget_vptr_bridges,
    ) = (
        _cc_recoil_hud_objectives._hud_ui_mgr_objective_start_hide_widget_candidate_vptr_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    (
        objective_start_hide_widget_center_absolute_bridges,
        work.objective_start_hide_widget_center_vptr_bridges,
    ) = (
        _cc_recoil_hud_objectives._hud_ui_mgr_objective_start_hide_widget_center_candidate_vptr_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    (
        objective_start_hide_topology_absolute_bridges,
        work.objective_start_hide_topology_vptr_bridges,
    ) = (
        _cc_recoil_hud_objectives._hud_ui_mgr_objective_start_hide_topology_candidate_vptr_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    (
        objective_update_absolute_bridges,
        work.objective_update_vptr_bridges,
    ) = _cc_recoil_hud_objectives._hud_ui_mgr_objective_update_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        update_frame_static_bridges,
        update_frame_absolute_bridges,
        work.update_frame_member_vptr_bridges,
        update_frame_tail_indirect_bridges,
    ) = _cc_recoil_hud_frame._hud_ui_mgr_update_frame_tail_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_proofs.merge_into(work.ensure_remaining_indirect_bridges, update_frame_tail_indirect_bridges, family="session.ensure_remaining_indirect_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, work.enable_current_layout_static_bridges, family="session.static_storage_reference_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, work.disable_current_layout_static_bridges, family="session.static_storage_reference_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, disable_visibility_static_bridges, family="session.static_storage_reference_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, objective_visibility_static_bridges, family="session.static_storage_reference_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, update_frame_static_bridges, family="session.static_storage_reference_bridges")
    work.absolute_storage_load_bridges = (
        _cc_recoil_hud_layout._hud_layout_hw_absolute_storage_load_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, work.ensure_sensor_absolute_load_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, work.enable_hud_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, work.disable_set_enabled_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, disable_visibility_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_visibility_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_update_meter_x_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_show_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_show_desc_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_show_sensor_overlay_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_show_widget_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_show_bar_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_begin_summary_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_begin_desc_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_begin_sensor_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_start_hide_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_start_hide_widget_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_start_hide_widget_center_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_start_hide_topology_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, objective_update_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, update_frame_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, work.first_delete_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, work.timer_panel_delete_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, work.objective_counter_delete_absolute_bridges, family="session.absolute_storage_load_bridges")
    _cc_proofs.merge_into(work.absolute_storage_load_bridges, work.remaining_delete_absolute_bridges, family="session.absolute_storage_load_bridges")
    work.member_vptr_storage_bridges = (
        _cc_recoil_hud_layout._hud_layout_member_vptr_storage_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
