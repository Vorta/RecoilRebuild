"""Serial recover candidate lifecycle proof phase."""
from __future__ import annotations

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import comparison as _cc_comparison
from _recoil.call_contract import contributions as _cc_contributions
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import providers as _cc_providers
from _recoil.call_contract import receiver_equivalence as _cc_receiver_equivalence
from _recoil.call_contract import recoil_application as _cc_recoil_application
from _recoil.call_contract import recoil_audio as _cc_recoil_audio
from _recoil.call_contract import recoil_callbacks as _cc_recoil_callbacks
from _recoil.call_contract import recoil_hud_widgets as _cc_recoil_hud_widgets
from _recoil.call_contract import recoil_network as _cc_recoil_network
from _recoil.call_contract import recoil_ui as _cc_recoil_ui
from _recoil.call_contract import work as _cc_work


def recover_candidate_lifecycle(work: _cc_work.CallerWork) -> None:
    from _recoil.commands.startup_contract import image_bytes
    from _recoil.lib.path_contract import (
        MATRIX_PATH_BODIES,
        MATRIX_PRIMITIVES,
        image_matrix_effects,
        local_object_effects,
        local_primitive_effects,
        object_matrix_effects,
        path_depths,
    )
    from _recoil.lib.tooling import REPO_ROOT

    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.zwep_damage_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    zvid_dd_candidate_vptr_bridges = (
        _cc_receiver_equivalence._zvid_dd_candidate_com_vptr_storage_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, zvid_dd_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    zinterp_logf_candidate_ctx_callback_bridges = (
        _cc_recoil_callbacks._zinterp_logf_candidate_ctx_callback_bridge(
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            existing_bridges=work.candidate_layout_loop_vptr_bridges,
        )
    )
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, zinterp_logf_candidate_ctx_callback_bridges, family="session.candidate_layout_loop_vptr_bridges")
    zinterp_report_errorf_candidate_ctx_callback_bridges = (
        _cc_recoil_callbacks._zinterp_report_errorf_candidate_ctx_callback_bridge(
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            existing_bridges=work.candidate_layout_loop_vptr_bridges,
        )
    )
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, zinterp_report_errorf_candidate_ctx_callback_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.main_menu_blur_candidate_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.set_value_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.clear_display_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.update_weapon_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.rebuild_weapon_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.apply_text_line_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.enable_stacks_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.disable_stacks_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.apply_text_label_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.layout_set_active_objective_loop_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.layout_set_active_census_loop_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.layout_set_active_message_loop_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.track_counter_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.track_counter_switch_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.track_marker_loop_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    disable_track_marker_loop_vptr_bridges = (
        _cc_recoil_hud_widgets._hud_ui_mgr_disable_track_marker_loop_vptr_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    disable_slot_base_loop_vptr_bridges = (
        _cc_recoil_hud_widgets._hud_ui_mgr_disable_slot_base_loop_vptr_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(disable_track_marker_loop_vptr_bridges, disable_slot_base_loop_vptr_bridges, family="session.disable_track_marker_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, disable_track_marker_loop_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    _cc_proofs.merge_into(work.candidate_layout_loop_vptr_bridges, work.ensure_font_loop_candidate_vptr_bridges, family="session.candidate_layout_loop_vptr_bridges")
    work.candidate_local_callable_bridges = (
        _cc_callable_identity._arbitrate_candidate_local_reviewed_bridges(
            work.candidate_local_callable_bridges,
            reviewed_compiler_provider_bridges=(
                work.compiler_generated_bridges
            ),
            exact_destructor_bridges=(
                work.compiler_destructor_bridges
            ),
            exact_constructor_bridges=(
                work.exact_constructor_bridges
            ),
            exact_zwep_provider_bridges=(
                work.zwep_ciasin_provider_bridges
            ),
        )
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.candidate_local_callable_bridges, family="session.compiler_generated_bridges")
    candidate_local_control_flow_indices = (
        work.candidate_assembly.local_control_flow_indices
    )
    if work.zsnd_static_coordinator_bridges:
        if (
            work.zsnd_static_coordinator_bridges
            != {
                _cc_catalog._ZSND_STATIC_VECTOR_CTOR_COFF:
                _cc_catalog._ZSND_STATIC_VECTOR_CTOR_IDENTITY,
                _cc_catalog._ZSND_STATIC_ATEXIT_COFF:
                work.indexes.by_address.get("0x4a0830"),
            }
            or candidate_local_control_flow_indices
            or work.candidate_assembly.local_control_flow_targets
        ):
            raise ValueError(
                "zSnd static coordinator rejects incomplete exact "
                "E3/E5 bridge or local-tail classification authority"
            )
    work.trace_caller(
        "candidate-extraction-start",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
    )
    candidate_invocation_call_sites: list[str] = []
    work.candidate = _cc_extraction.extract_invocation_contract(
        work.candidate_assembly.instructions,
        source="cod",
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.candidate_exact_iat_indexes,
        bridge_names=work.exact_bridge_names,
        compiler_generated_bridges=work.compiler_generated_bridges,
        candidate_storage_bridges=work.candidate_storage_bridges,
        reviewed_register_storage_bridges=(
            work.candidate_register_storage_bridges
        ),
        reviewed_register_call_storage_bridges=(
            work.zinput_runtime_register_call_bridges
        ),
        reviewed_iat_register_definition_offsets=(
            work.zsnd_free_iat_definition_offsets
            | work.zeffect_fwrite_iat_definition_offsets
            | frozenset(work.candidate_exact_iat_load_proofs)
        ),
        candidate_exact_iat_register_load_proofs=(
            work.candidate_exact_iat_load_proofs
        ),
        reviewed_static_storage_reference_bridges=(
            work.static_storage_reference_bridges
        ),
        reviewed_absolute_storage_load_bridges=(
            work.absolute_storage_load_bridges
        ),
        reviewed_member_vptr_storage_bridges=(
            work.member_vptr_storage_bridges
        ),
        reviewed_call_result_bridges=work.candidate_call_result_bridges,
        reviewed_vptr_storage_bridges=(
            work.candidate_vptr_storage_bridges
        ),
        reviewed_loop_vptr_storage_bridges=(
            work.candidate_layout_loop_vptr_bridges
        ),
        reviewed_exact_indirect_storage_bridges=(
            work.ensure_remaining_indirect_bridges
        ),
        reviewed_inbound_entry_register_target_bridges=(
            work.inbound_entry_target_bridges
        ),
        reviewed_inbound_entry_register_roots=(
            work.inbound_entry_register_roots
        ),
        local_control_flow_indices=(
            candidate_local_control_flow_indices
        ),
        local_control_flow_targets=(
            work.candidate_assembly.local_control_flow_targets
        ),
        reviewed_retail_call_sites_by_ordinal=(
            tuple(work.retail_invocation_call_sites)
        ),
        invocation_call_sites_out=candidate_invocation_call_sites,
        candidate_caller_definition=(
            work.candidate_assembly.caller_definition
        ),
        candidate_classification_only_local_control_flow_targets=(
            work.candidate_assembly.classification_only_local_control_flow_targets
        ),
    )
    work.candidate_physical_contributions = _cc_contributions.physical_contributions(
        work.candidate_assembly.instructions, candidate_invocation_call_sites, side="candidate",
        caller_start=work.address, caller_end_exclusive=work.end_exclusive, candidate=work.candidate_assembly)
    work.candidate_physical_contributions, work.candidate_funclet_contributions = _cc_contributions.partition_native_funclets(
        work.candidate_physical_contributions, work.candidate_funclet_partition, caller_start=work.address)
    if work.candidate_funclet_contributions:
        work.trace_caller("native-funclet-invocation-partition", symbol_id=work.symbol_id,
            caller_index=work.caller_index, address=work.address,
            primary_count=len(work.candidate_physical_contributions),
            separate_body_contributions=[{"owner": row.owner, "location": row.contribution.location,
                "form": row.contribution.form, "dispatch": row.contribution.dispatch}
                for row in work.candidate_funclet_contributions])
    work.trace_caller(
        "candidate-extraction-complete",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
        candidate_invocation_count=len(work.candidate),
        candidate_invocation_call_site_count=len(
            candidate_invocation_call_sites
        ),
    )
    work.candidate, cleanup_receipts = (
        _cc_callable_identity._candidate_only_direct_callee_cleanup_contract(
            work.candidate, work.candidate_assembly
        )
    )
    if cleanup_receipts:
        work.candidate_cleanup_receipts_by_symbol[work.symbol_id] = (
            cleanup_receipts
        )
    work.candidate, zui_helper_receipt = (
        _cc_recoil_ui._zui_check_toggle_inline_helper_occurrence_projection(
            work.candidate,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
        )
    )
    if zui_helper_receipt is not None:
        if work.symbol_id in work.candidate_expansion_receipts_by_symbol:
            raise ValueError(
                "zUI caller emitted overlapping finite helper-graph "
                "expansions"
            )
        work.candidate_expansion_receipts_by_symbol[work.symbol_id] = (
            zui_helper_receipt
        )
    work.candidate, zui_vector_receipt = (
        _cc_recoil_ui._zui_candidate_local_vector_occurrence_projection(
            work.expected,
            work.candidate,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    if zui_vector_receipt is not None:
        if work.symbol_id in work.candidate_expansion_receipts_by_symbol:
            raise ValueError(
                "zUI caller emitted overlapping finite helper-graph "
                "expansions"
            )
        work.candidate_expansion_receipts_by_symbol[work.symbol_id] = (
            zui_vector_receipt
        )
    work.candidate = _cc_recoil_application._appframe_run_primary_contract_projection(
        work.expected,
        work.candidate,
        work.candidate_assembly,
        candidate_invocation_call_sites,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
    )
    if work.zinput_candidate_switch_indices:
        extracted_invocation_indices = frozenset(
            _cc_callable_identity._candidate_static_invocation_indices(
                work.candidate_assembly,
                caller_start=work.address,
                caller_end_exclusive=work.end_exclusive,
            )
        )
        if (
            work.zinput_candidate_switch_indices
            & extracted_invocation_indices
            or not work.zinput_candidate_switch_indices.issubset(
                work.candidate_assembly.local_control_flow_indices
            )
            or any(
                index
                not in work.candidate_assembly.local_control_flow_targets
                for index in work.zinput_candidate_switch_indices
            )
        ):
            raise ValueError(
                "zInput keyboard reviewed switch package was unused "
                "by final candidate extraction"
            )
    work.candidate = _cc_recoil_application._recoilapp_terminal_frame_candidate_cleanup_projection(
        work.candidate,
        work.candidate_assembly,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
    )
    work.candidate = _cc_recoil_audio._zsnd_play_directsound_candidate_projection(
        work.expected,
        work.candidate,
        work.candidate_assembly,
        work.retail_instructions,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    work.candidate = _cc_recoil_audio._zsnd_apply_mute_candidate_projection(
        work.expected,
        work.candidate,
        work.candidate_assembly,
        work.retail_instructions,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
    )
    work.candidate = _cc_recoil_audio._zsnd_static_e3_inline_candidate_projection(
        work.expected,
        work.candidate,
        work.candidate_assembly,
        work.retail_instructions,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge=work.bridge,
    )
    work.candidate = _cc_recoil_audio._zsnd_static_vector_dtor_candidate_projection(
        work.expected,
        work.candidate,
        work.candidate_assembly,
        work.retail_instructions,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge=work.bridge,
    )
    work.candidate = _cc_providers.candidate_provider_comdat_contract(work.candidate, work.candidate_assembly,
        document=work.document, indexes=work.indexes, bridge=work.bridge)
    work.candidate = _cc_recoil_ui._zui_exact_stack_receiver_storage_projection(
        work.expected,
        work.candidate,
        work.candidate_assembly,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
    )
    work.candidate = _cc_recoil_ui._zui_r4905_exact_receiver_rendering_projection(
        work.expected,
        work.candidate,
        work.candidate_assembly,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    work.candidate = _cc_recoil_ui._zui_check_toggle_receiver_storage_projection(
        work.expected,
        work.candidate,
        work.candidate_assembly,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
    )
    work.candidate = (
        _cc_recoil_network._mission_current_artifact_invocation_population_contract(
            work.expected,
            work.candidate,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
        )
    )
    work.expected, work.candidate = _cc_comparison._constructor_dispatch_contract_facts(
        work.expected, work.candidate, caller_start=work.address,
        candidate=work.candidate_assembly, bridge=work.bridge, indexes=work.indexes,
    )
    if work.address in ("0x474c20", "0x453880", "0x476cf0", "0x477b30"):
        from _recoil.commands.startup_contract import image_bytes
        from _recoil.lib.path_contract import (
            MATRIX_PATH_BODIES,
            MATRIX_PRIMITIVES,
            image_matrix_effects,
            local_object_effects,
            local_primitive_effects,
            object_matrix_effects,
            path_depths,
        )
        start_address = int(work.address, 16)
        _, _, _, reviewed = MATRIX_PATH_BODIES[start_address]
        retail_rows = [(bytes.fromhex(" ".join(row.bytes)), row.raw_text.split()[0])
                       for row in work.retail_instructions]
        work.retail_body = b"".join(row[0] for row in retail_rows)
        if work.retail_body != image_bytes((REPO_ROOT / "support/Recoil.exe").read_bytes(), start_address, len(work.retail_body)):
            raise ValueError("matrix path BN listing disagrees with immutable retail")
        effects = (image_matrix_effects(retail_rows, start_address, dict(reviewed.values()))
                   if reviewed is not None else local_primitive_effects(
                       retail_rows, start_address, dict(MATRIX_PRIMITIVES.values())))
        required = path_depths(retail_rows, work.retail_body, effects)
        work.definition = work.candidate_assembly.caller_definition
        if work.definition is None:
            raise ValueError("matrix path lacks fresh object definition")
        rows = [(bytes.fromhex(" ".join(row.bytes)), row.raw_text.split()[0])
                for row in work.candidate_assembly.instructions]
        effects = (object_matrix_effects(work.definition, reviewed) if reviewed is not None
                   else local_object_effects(rows, work.definition))
        observed = path_depths(rows, work.definition.data, effects)
        if observed != required:
            raise ValueError("matrix resource-depth paths differ from retail")
