"""Serial recover candidate dispatch proof phase."""
from __future__ import annotations

from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import recoil_callbacks as _cc_recoil_callbacks
from _recoil.call_contract import recoil_hud_layout as _cc_recoil_hud_layout
from _recoil.call_contract import (
    recoil_hud_layout_active as _cc_recoil_hud_layout_active,
)
from _recoil.call_contract import recoil_hud_lifetimes as _cc_recoil_hud_lifetimes
from _recoil.call_contract import recoil_hud_objectives as _cc_recoil_hud_objectives
from _recoil.call_contract import recoil_hud_visibility as _cc_recoil_hud_visibility
from _recoil.call_contract import recoil_hud_widgets as _cc_recoil_hud_widgets
from _recoil.call_contract import recoil_input as _cc_recoil_input
from _recoil.call_contract import recoil_lifecycle as _cc_recoil_lifecycle
from _recoil.call_contract import recoil_weapons as _cc_recoil_weapons
from _recoil.call_contract import work as _cc_work


def recover_candidate_dispatch(work: _cc_work.CallerWork) -> None:

    (
        work.objective_counter_delete_absolute_bridges,
        work.objective_counter_delete_member_vptr_bridges,
    ) = (
        _cc_recoil_hud_lifetimes._hud_ui_mgr_objective_counter_deleting_destructor_candidate_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    (
        work.remaining_delete_absolute_bridges,
        work.remaining_delete_member_vptr_bridges,
    ) = _cc_recoil_hud_lifetimes._hud_ui_mgr_remaining_deleting_destructor_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    if (
        work.first_delete_absolute_bridges.keys()
        & work.timer_panel_delete_absolute_bridges.keys()
        or work.first_delete_member_vptr_bridges.keys()
        & work.timer_panel_delete_member_vptr_bridges.keys()
        or work.first_delete_absolute_bridges.keys()
        & work.objective_counter_delete_absolute_bridges.keys()
        or work.first_delete_member_vptr_bridges.keys()
        & work.objective_counter_delete_member_vptr_bridges.keys()
        or work.timer_panel_delete_absolute_bridges.keys()
        & work.objective_counter_delete_absolute_bridges.keys()
        or work.timer_panel_delete_member_vptr_bridges.keys()
        & work.objective_counter_delete_member_vptr_bridges.keys()
        or work.first_delete_absolute_bridges.keys()
        & work.remaining_delete_absolute_bridges.keys()
        or work.first_delete_member_vptr_bridges.keys()
        & work.remaining_delete_member_vptr_bridges.keys()
        or work.timer_panel_delete_absolute_bridges.keys()
        & work.remaining_delete_absolute_bridges.keys()
        or work.timer_panel_delete_member_vptr_bridges.keys()
        & work.remaining_delete_member_vptr_bridges.keys()
        or work.objective_counter_delete_absolute_bridges.keys()
        & work.remaining_delete_absolute_bridges.keys()
        or work.objective_counter_delete_member_vptr_bridges.keys()
        & work.remaining_delete_member_vptr_bridges.keys()
    ):
        raise ValueError(
            "reviewed HUD deleting-destructor bridge callsites overlap"
        )
    work.compiler_destructor_bridges = (
        _cc_recoil_lifecycle._compiler_destructor_provider_bridges(
            work.expected,
            work.candidate_assembly,
            retail_instructions=work.retail_instructions,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            bridge=work.bridge,
            compiler_generated_bridges=work.compiler_generated_bridges,
            registered_regex_bridges=work.registered_regex_bridges,
            candidate_storage_bridges=work.candidate_storage_bridges,
        )
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.compiler_destructor_bridges, family="session.compiler_generated_bridges")
    work.static_storage_reference_bridges = (
        _cc_recoil_hud_widgets._hud_ui_mgr_stats_list_static_storage_reference_bridges(
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            candidate_storage_bridges=work.candidate_storage_bridges,
            compiler_generated_bridges=(
                work.compiler_generated_bridges
            ),
        )
    )
    (
        zinput_joystick_static_bridges,
        work.zinput_joystick_member_vptr_bridges,
    ) = _cc_recoil_input._zinput_joystick_aggregate_leaf_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        zinput_keyboard_static_bridges,
        zinput_keyboard_member_vptr_bridges,
    ) = _cc_recoil_input._zinput_keyboard_aggregate_leaf_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    work.zinput_runtime_register_call_bridges = (
        _cc_recoil_input._zinput_runtime_dispatch_candidate_register_bridges(
            work.candidate_assembly,
            caller_start=work.address,
            indexes=work.indexes,
        )
    )
    zinput_bindmap_register_call_bridges = (
        _cc_recoil_input._zinput_bindmap_runtime_dispatch_candidate_register_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    zcom_resolver_register_call_bridges = (
        _cc_recoil_callbacks._zcom_interface_map_resolver_candidate_register_bridge(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    zcom_addref_indirect_bridges = (
        _cc_recoil_callbacks._zcom_interface_map_addref_candidate_indirect_bridge(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.ensure_remaining_indirect_bridges, zcom_addref_indirect_bridges, family="session.ensure_remaining_indirect_bridges")
    zwep_runtime_callback_register_bridges = (
        _cc_recoil_weapons._zwep_runtime_callback_candidate_register_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    zwep_entry_callback_register_bridges = (
        _cc_recoil_weapons._zwep_entry_callback_candidate_register_bridge(
            work.expected,
            work.retail_instructions,
            work.retail_invocation_call_sites,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.zinput_runtime_register_call_bridges, zinput_bindmap_register_call_bridges, family="session.zinput_runtime_register_call_bridges")
    _cc_proofs.merge_into(work.zinput_runtime_register_call_bridges, zcom_resolver_register_call_bridges, family="session.zinput_runtime_register_call_bridges")
    _cc_proofs.merge_into(work.zinput_runtime_register_call_bridges, zwep_runtime_callback_register_bridges, family="session.zinput_runtime_register_call_bridges")
    _cc_proofs.merge_into(work.zinput_runtime_register_call_bridges, zwep_entry_callback_register_bridges, family="session.zinput_runtime_register_call_bridges")
    (
        zinput_mouse_static_bridges,
        zinput_mouse_member_vptr_bridges,
    ) = _cc_recoil_input._zinput_mouse_aggregate_leaf_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    for work.label, work.additions in (
        ("keyboard", zinput_keyboard_static_bridges),
        ("mouse", zinput_mouse_static_bridges),
    ):
        _cc_proofs.merge_into(zinput_joystick_static_bridges, work.additions, family="session.zinput_joystick_static_bridges")
    for work.label, work.additions in (
        ("keyboard", zinput_keyboard_member_vptr_bridges),
        ("mouse", zinput_mouse_member_vptr_bridges),
    ):
        _cc_proofs.merge_into(work.zinput_joystick_member_vptr_bridges, work.additions, family="session.zinput_joystick_member_vptr_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, zinput_joystick_static_bridges, family="session.static_storage_reference_bridges")
    (
        refresh_counter_panel_static_bridges,
        work.refresh_counter_panel_member_vptr_bridges,
    ) = (
        _cc_recoil_hud_objectives._hud_ui_mgr_objective_refresh_counter_panel_storage_bridges(
            work.expected,
            work.retail_instructions,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    (
        layout_set_active_objective_static_bridges,
        work.layout_set_active_objective_loop_vptr_bridges,
    ) = (
        _cc_recoil_hud_layout_active._hud_layout_hw_set_active_objective_counter_candidate_bridges(
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
        layout_set_active_timer_static_bridges,
        work.layout_set_active_timer_member_vptr_bridges,
    ) = _cc_recoil_hud_layout_active._hud_layout_hw_set_active_timer_panel_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        layout_set_active_census_static_bridges,
        work.layout_set_active_census_loop_vptr_bridges,
    ) = _cc_recoil_hud_layout_active._hud_layout_hw_set_active_static_panel_census_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    work.layout_set_active_message_loop_vptr_bridges = (
        _cc_recoil_hud_layout_active._hud_layout_hw_set_active_message_loop_candidate_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.static_storage_reference_bridges, work.shield_candidate_static_storage_bridges, family="session.static_storage_reference_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, work.shield_meter_candidate_static_storage_bridges, family="session.static_storage_reference_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, refresh_counter_panel_static_bridges, family="session.static_storage_reference_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, layout_set_active_objective_static_bridges, family="session.static_storage_reference_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, layout_set_active_timer_static_bridges, family="session.static_storage_reference_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, layout_set_active_census_static_bridges, family="session.static_storage_reference_bridges")
    _cc_proofs.merge_into(work.static_storage_reference_bridges, work.stats_list_static_bridges, family="session.static_storage_reference_bridges")
    (
        viewport_layout_static_bridges,
        work.viewport_layout_vptr_bridges,
    ) = _cc_recoil_hud_layout._hud_ui_mgr_viewport_layout_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_proofs.merge_into(work.static_storage_reference_bridges, viewport_layout_static_bridges, family="session.static_storage_reference_bridges")
    (
        trigger_current_layout_static_bridges,
        work.trigger_current_layout_vptr_bridges,
    ) = _cc_recoil_hud_layout._hud_ui_mgr_trigger_current_layout_on_activated_bridges(
        work.expected,
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_proofs.merge_into(work.static_storage_reference_bridges, trigger_current_layout_static_bridges, family="session.static_storage_reference_bridges")
    (
        switch_active_dialog_static_bridges,
        work.switch_active_dialog_vptr_bridges,
    ) = _cc_recoil_hud_layout._hud_ui_mgr_switch_active_dialog_current_layout_bridges(
        work.expected,
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_proofs.merge_into(work.static_storage_reference_bridges, switch_active_dialog_static_bridges, family="session.static_storage_reference_bridges")
    (
        set_float_timer_visible_static_bridges,
        work.set_float_timer_visible_vptr_bridges,
    ) = _cc_recoil_hud_visibility._hud_ui_mgr_set_float_timer_visible_bridges(
        work.expected,
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_proofs.merge_into(work.static_storage_reference_bridges, set_float_timer_visible_static_bridges, family="session.static_storage_reference_bridges")
    (
        set_aux_overlay_visible_static_bridges,
        work.set_aux_overlay_visible_vptr_bridges,
    ) = _cc_recoil_hud_visibility._hud_ui_mgr_set_aux_overlay_visible_bridges(
        work.expected,
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_proofs.merge_into(work.static_storage_reference_bridges, set_aux_overlay_visible_static_bridges, family="session.static_storage_reference_bridges")
    (
        apply_text_line_static_bridges,
        work.apply_text_line_candidate_vptr_bridges,
    ) = _cc_recoil_hud_widgets._hud_ui_aux_overlay_apply_text_line_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_proofs.merge_into(work.static_storage_reference_bridges, apply_text_line_static_bridges, family="session.static_storage_reference_bridges")
    work.enable_stacks_candidate_vptr_bridges = (
        _cc_recoil_hud_widgets._hud_ui_mgr_enable_stacks_candidate_vptr_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    work.disable_stacks_candidate_vptr_bridges = (
        _cc_recoil_hud_widgets._hud_ui_mgr_disable_stacks_candidate_vptr_bridges(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    work.apply_text_label_candidate_vptr_bridges = (
        _cc_recoil_hud_layout._hud_layout_apply_text_label_candidate_vptr_bridges(
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
        work.enable_hud_absolute_bridges,
        work.enable_hud_vptr_bridges,
    ) = _cc_recoil_hud_widgets._hud_ui_mgr_enable_hud_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        work.disable_set_enabled_absolute_bridges,
        work.disable_set_enabled_vptr_bridges,
    ) = _cc_recoil_hud_widgets._hud_ui_mgr_disable_set_enabled_candidate_vptr_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        work.enable_current_layout_static_bridges,
        work.enable_current_layout_member_vptr_bridges,
    ) = _cc_recoil_hud_layout._hud_ui_mgr_enable_current_layout_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        work.disable_current_layout_static_bridges,
        work.disable_current_layout_member_vptr_bridges,
    ) = _cc_recoil_hud_layout._hud_ui_mgr_disable_current_layout_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
