"""Per-caller working state for the serial proof phases.

This mutable construction record is not an acceptance fact. Producers publish
immutable evidence records; comparison freezes their claims as obligations and
results. Every caller receives a new record, with explicit named fields.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, Callable, Mapping, Sequence

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateCallerDefinition,
        CandidateExactIatRegisterLoadProof,
        IdentityIndexes,
        ProviderNamedImportThunk,
        RetailCallerScopedProofPackage,
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedCallResultBridge,
        ReviewedExactIndirectStorageBridge,
        ReviewedInboundEntryRegisterTargetBridge,
        ReviewedLoopVptrStorageBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedRegisterCallStorageBridge,
        ReviewedRetailProvenanceAdapters,
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
    )
    from _recoil.commands.asm_verify import Instruction
    from _recoil.lib.binja import BinaryNinjaBridge
    from _recoil.lib.progress import ProgressDocument


@dataclass(slots=True, kw_only=True)
class CallerWork:
    acquire_candidate_callee_definitions: Callable[..., Mapping[str, Any]] = None
    _provider_named_import_thunks: tuple[ProviderNamedImportThunk, ...] = None
    absolute_storage_load_bridges: dict[str, ReviewedAbsoluteStorageLoadBridge] = None
    address: str = None
    apply_text_label_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    apply_text_line_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    assembly_started: float = None
    authored_decorated_abi_retail_cache: Any = None
    bridge: BinaryNinjaBridge = None
    bridge_data_rows: Any = None
    caller_identity: str = None
    caller_index: int = None
    candidate: list[dict[str, Any]] = None
    candidate_assembly: CandidateAssembly = None
    candidate_call_result_bridges: dict[str, ReviewedCallResultBridge] = None
    candidate_cleanup_receipts_by_symbol: Any = None
    candidate_exact_iat_indexes: IdentityIndexes = None
    candidate_exact_iat_load_proofs: dict[str, CandidateExactIatRegisterLoadProof] = None
    candidate_expansion_receipts_by_symbol: Any = None
    candidate_layout_loop_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    candidate_physical_contributions: Any = None
    candidate_funclet_partition: Any = None
    candidate_funclet_contributions: Any = None
    candidate_register_storage_bridges: Any = None
    candidate_storage_bridges: dict[str, str] = None
    candidate_vptr_storage_bridges: dict[str, ReviewedVptrStorageBridge] = None
    chkstk_compiler_helper_bridges: dict[str, str] = None
    ciasin_provider_bridges: dict[str, str] = None
    clear_display_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    clear_display_retail_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    cls_util_authored_identity_bridges: dict[str, str] = None
    comparison_started: float = None
    compiler_destructor_bridges: dict[str, str] = None
    compiler_generated_bridges: dict[str, str] = None
    disable_current_layout_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    disable_current_layout_static_bridges: dict[str, ReviewedStaticStorageReferenceBridge] = None
    disable_set_enabled_absolute_bridges: dict[str, ReviewedAbsoluteStorageLoadBridge] = None
    disable_set_enabled_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    disable_stacks_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    disable_visibility_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    disable_visibility_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    document: ProgressDocument = None
    eh_array_destructor_bridges: dict[str, str] = None
    enable_current_layout_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    enable_current_layout_static_bridges: dict[str, ReviewedStaticStorageReferenceBridge] = None
    enable_hud_absolute_bridges: dict[str, ReviewedAbsoluteStorageLoadBridge] = None
    enable_hud_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    enable_stacks_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    end_exclusive: str = None
    ensure_font_loop_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    ensure_objective_label_settextfmt_candidate_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    ensure_remaining_indirect_bridges: dict[str, ReviewedExactIndirectStorageBridge] = None
    ensure_sensor_absolute_load_bridges: Any = None
    exact_bridge_names: Mapping[str, Sequence[Any]] = None
    exact_constructor_bridges: Any = None
    expected: list[dict[str, Any]] = None
    expected_direct_identity_bridges: dict[str, str] = None
    first_delete_absolute_bridges: dict[str, ReviewedAbsoluteStorageLoadBridge] = None
    first_delete_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    ftol_provider_bridges: dict[str, str] = None
    ftol_provider_identity: Any = None
    gettickcount_candidate_direct_bridges: dict[str, str] = None
    gettickcount_candidate_retail_iat_equivalences: dict[str, str] = None
    gettickcount_named_import_thunk: Any = None
    gettickcount_retail_e8_call_population: Any = None
    hud_ui_mgr_zrd_payload_bridges: dict[str, str] = None
    inbound_entry_register_roots: frozenset[str] = None
    inbound_entry_target_bridges: dict[int, ReviewedInboundEntryRegisterTargetBridge] = None
    indexes: IdentityIndexes = None
    layout_set_active_census_loop_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    layout_set_active_message_loop_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    layout_set_active_objective_loop_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    layout_set_active_timer_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    main_menu_blur_candidate_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    member_vptr_storage_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    named_thunk_candidate_direct_bridges: dict[str, str] = None
    named_thunk_candidate_direct_equivalences: dict[str, str] = None
    named_thunk_candidate_iat_equivalences: dict[str, str] = None
    objective_begin_desc_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    objective_begin_sensor_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    objective_begin_summary_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    objective_counter_delete_absolute_bridges: dict[str, ReviewedAbsoluteStorageLoadBridge] = None
    objective_counter_delete_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    objective_show_bar_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    objective_show_desc_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    objective_show_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    objective_show_sensor_overlay_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    objective_show_widget_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    objective_start_hide_topology_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    objective_start_hide_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    objective_start_hide_widget_center_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    objective_start_hide_widget_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    objective_update_meter_x_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    objective_update_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    objective_visibility_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    objective_visibility_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    player_compiler_provider_bridges: dict[str, str] = None
    provider_ordinal_import_thunks: Any = None
    r4564_callback_exact_indirect_bridges: Any = None
    r4564_provider_iat_equivalences: Any = None
    rebuild_weapon_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    refresh_counter_panel_member_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    registered_regex_bridges: dict[str, str] = None
    remaining_delete_absolute_bridges: dict[str, ReviewedAbsoluteStorageLoadBridge] = None
    remaining_delete_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    retail_call_result_bridges: dict[str, ReviewedCallResultBridge] = None
    retail_comparison_end_exclusive: str = None
    retail_extraction_bridge_names: dict[str, list[Any]] = None
    retail_import_targets: Sequence[Any] = None
    retail_instructions: list[Instruction] = None
    retail_invocation_call_sites: list[str] = None
    retail_layout_loop_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    retail_physical_contributions: Any = None
    retail_proof_package: RetailCallerScopedProofPackage = None
    retail_provenance_adapter_sites_by_symbol: Any = None
    retail_register_storage_bridges: Any = None
    retail_switch_targets: Any = None
    reviewed_retail_adapters: ReviewedRetailProvenanceAdapters = None
    set_aux_overlay_visible_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    set_float_timer_visible_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    set_value_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    shield_candidate_static_storage_bridges: dict[str, ReviewedStaticStorageReferenceBridge] = None
    shield_meter_candidate_static_storage_bridges: dict[str, ReviewedStaticStorageReferenceBridge] = None
    static_storage_reference_bridges: dict[str, ReviewedStaticStorageReferenceBridge] = None
    stats_list_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    stats_list_static_bridges: dict[str, ReviewedStaticStorageReferenceBridge] = None
    switch_active_dialog_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    symbol_id: str = None
    timer_panel_delete_absolute_bridges: dict[str, ReviewedAbsoluteStorageLoadBridge] = None
    timer_panel_delete_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    timings_ms: dict[str, float] = None
    trace_caller: Callable[..., None] = None
    track_counter_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    track_counter_switch_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    track_marker_exit_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    track_marker_loop_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    trigger_current_layout_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    update_frame_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    update_weapon_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    viewport_layout_vptr_bridges: dict[str, ReviewedVptrStorageBridge] = None
    wol_category_a_direct_bridges: dict[str, str] = None
    wol_cedit_provider_bridges: dict[str, str] = None
    zeffect_fwrite_iat_definition_offsets: frozenset[str] = None
    zinput_candidate_switch_indices: frozenset[int] = None
    zinput_joystick_member_vptr_bridges: dict[str, ReviewedMemberVptrStorageBridge] = None
    zinput_runtime_register_call_bridges: dict[str, ReviewedRegisterCallStorageBridge] = None
    zsnd_free_iat_definition_offsets: frozenset[str] = None
    zsnd_static_coordinator_bridges: dict[str, str] = None
    zwep_ciacos_provider_iat_equivalences: Any = None
    zwep_ciasin_provider_bridges: dict[str, str] = None
    zwep_damage_candidate_vptr_bridges: dict[str, ReviewedLoopVptrStorageBridge] = None
    additions: Any = None
    candidate_local_callable_bridges: dict[str, str] = None
    definition: CandidateCallerDefinition | None = None
    label: str = None
    retail_body: bytes = None
