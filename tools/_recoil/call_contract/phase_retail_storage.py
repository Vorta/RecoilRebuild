"""Serial recover retail storage proof phase."""
from __future__ import annotations

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import dispatch as _cc_dispatch
from _recoil.call_contract import iat as _cc_iat
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import lifecycle as _cc_lifecycle
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import recoil_hud_construction as _cc_recoil_hud_construction
from _recoil.call_contract import recoil_hud_layout as _cc_recoil_hud_layout
from _recoil.call_contract import recoil_hud_messages as _cc_recoil_hud_messages
from _recoil.call_contract import recoil_hud_sensor as _cc_recoil_hud_sensor
from _recoil.call_contract import recoil_hud_shield as _cc_recoil_hud_shield
from _recoil.call_contract import recoil_hud_timers_fonts as _cc_recoil_hud_timers_fonts
from _recoil.call_contract import recoil_hud_visibility as _cc_recoil_hud_visibility
from _recoil.call_contract import recoil_hud_widgets as _cc_recoil_hud_widgets
from _recoil.call_contract import recoil_network as _cc_recoil_network
from _recoil.call_contract import reporting as _cc_reporting
from _recoil.call_contract import retail as _cc_retail
from _recoil.call_contract import retail_imports as _cc_retail_imports
from _recoil.call_contract import work as _cc_work


def recover_retail_storage(work: _cc_work.CallerWork) -> None:
    import re
    import time

    from _recoil.lib.progress import normalize_address

    work.trace_caller(
        "retail-assembly-start",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
    )
    retail_assembly = work.bridge.assembly(work.address)
    _cc_reporting._add_elapsed_ms(
        work.timings_ms,
        "binary_ninja_assembly",
        work.assembly_started,
    )
    work.trace_caller(
        "retail-assembly-complete",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
        retail_assembly_character_count=len(retail_assembly),
    )
    work.comparison_started = time.perf_counter()
    work.retail_instructions = _cc_listing.parse_assembly(retail_assembly, source="bn")
    work.retail_comparison_end_exclusive = (
        _cc_identity._retail_call_contract_comparison_end_exclusive(
            work.retail_instructions,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
        )
    )
    work.indexes = _cc_retail_imports._retail_direct_import_thunk_indexes(
        work.retail_instructions,
        indexes=work.indexes,
        thunks=work.provider_ordinal_import_thunks,
    )
    work.trace_caller(
        "retail-adapters-start",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
        retail_instruction_count=len(work.retail_instructions),
    )
    work.reviewed_retail_adapters = (
        _cc_identity._compose_reviewed_retail_provenance_adapters(
            work.retail_instructions,
            document=work.document,
            caller_start=work.address,
            caller_end_exclusive=work.retail_comparison_end_exclusive,
            indexes=work.indexes,
            bridge=work.bridge,
            ordinal_provider_thunks=work.provider_ordinal_import_thunks,
            retail_import_targets=work.retail_import_targets,
        )
    )
    work.trace_caller(
        "retail-adapters-complete",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
        reviewed_retail_call_site_count=len(
            work.reviewed_retail_adapters.reviewed_call_sites
        ),
        reviewed_register_call_bridge_count=len(
            work.reviewed_retail_adapters.register_call_storage_bridges
        ),
    )
    work.indexes = work.reviewed_retail_adapters.indexes
    work.retail_extraction_bridge_names = (
        _cc_identity._reviewed_r3994_dplay_ordinal_retail_bridge_names(
            work.retail_instructions,
            caller_start=work.address,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
        )
    )
    work.trace_caller(
        "retail-proof-package-start",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
    )
    work.indexes, work.retail_proof_package = (
        _cc_identity._compose_caller_scoped_retail_proof_package(
            work.retail_instructions,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            caller_execution_end_exclusive=(
                work.retail_comparison_end_exclusive
            ),
            indexes=work.indexes,
            bridge=work.bridge,
            retail_import_targets=work.retail_import_targets,
            reviewed_provenance_adapters=(
                work.reviewed_retail_adapters
            ),
            _trace_stage=lambda stage, **extra: work.trace_caller(
                stage,
                caller_index=work.caller_index,
                symbol_id=work.symbol_id,
                address=work.address,
                **extra,
            ),
        )
    )
    work.trace_caller(
        "retail-proof-package-complete",
        caller_index=work.caller_index,
        symbol_id=work.symbol_id,
        address=work.address,
    )
    if normalize_address(work.address) != "0x4b9850" and any(
        move[0] in {"ebx", "esi", "edi", "ebp"}
        and move[1] == "ecx"
        for instruction in work.retail_instructions
        if (move := _cc_receiver_instructions._exact_register_move(instruction)) is not None
    ):
        work.inbound_entry_register_roots = (
            _cc_dispatch._retail_inbound_entry_register_lineage_roots(
                work.retail_instructions,
                caller_start=work.address,
                bridge=work.bridge,
            )
        )
    if normalize_address(work.address) != "0x4b9850" and any(
        _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and _cc_catalog.REGISTER_RE.fullmatch(
            re.sub(
                r"^(?:near|far)\s+(?:ptr\s+)?",
                "",
                _cc_cfg._instruction_operand(instruction).strip(),
                flags=re.IGNORECASE,
            )
        )
        is not None
        for instruction in work.retail_instructions
    ):
        work.inbound_entry_target_bridges = (
            _cc_dispatch._retail_inbound_entry_register_target_bridges(
                work.retail_instructions,
                caller_start=work.address,
                caller_end_exclusive=work.end_exclusive,
                indexes=work.indexes,
                bridge=work.bridge,
            )
        )
    work.indexes = _cc_retail_imports._retail_direct_import_thunk_indexes(
        work.retail_instructions,
        indexes=work.indexes,
        thunks=(
            *(
                (work.gettickcount_named_import_thunk,)
                if work.gettickcount_named_import_thunk is not None
                else ()
            ),
            *work._provider_named_import_thunks,
            *work.provider_ordinal_import_thunks,
        ),
        preserved_provider_identities=(
            frozenset(
                identity
                for identity in (
                    work.ftol_provider_identity,
                    _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY,
                )
                if identity in work.indexes.provider_ids
            )
        ),
    )
    work.gettickcount_retail_e8_call_population = (
        _cc_retail_imports._gettickcount_retail_e8_call_population(
            work.retail_instructions,
            thunk=work.gettickcount_named_import_thunk,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
        )
    )
    work.indexes = _cc_iat._cached_fread_retail_iat_indexes(
        work.retail_instructions,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    work.indexes = _cc_recoil_hud_timers_fonts._hud_timer_floor_retail_iat_indexes(
        work.retail_instructions,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_data_rows=work.bridge_data_rows,
        bridge=work.bridge,
    )
    work.indexes = _cc_recoil_hud_widgets._hud_loading_checkpoint_stdio_iat_indexes(
        work.retail_instructions,
        candidate=work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_data_rows=work.bridge_data_rows,
        bridge=work.bridge,
    )
    work.indexes = _cc_recoil_hud_construction._hud_ui_mgr_ensure_ceil_retail_iat_indexes(
        work.retail_instructions,
        candidate=work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_data_rows=work.bridge_data_rows,
        bridge=work.bridge,
    )
    (
        zinput_retail_register_storage_bridges,
        zinput_candidate_register_storage_bridges,
    ) = _cc_retail._zinput_wait_sleep_register_iat_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
        bridge_data_rows=work.bridge_data_rows,
    )
    (
        briefing_retail_register_storage_bridges,
        briefing_candidate_register_storage_bridges,
    ) = _cc_retail._briefing_start_sleep_register_iat_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
        bridge_data_rows=work.bridge_data_rows,
    )
    (
        work.retail_call_result_bridges,
        work.candidate_call_result_bridges,
    ) = _cc_retail._zsys_directdrawcreate_dynamic_export_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
        bridge_data_rows=work.bridge_data_rows,
        bridge=work.bridge,
    )
    if (
        set(zinput_retail_register_storage_bridges)
        & set(briefing_retail_register_storage_bridges)
        or set(zinput_candidate_register_storage_bridges)
        & set(briefing_candidate_register_storage_bridges)
    ):
        raise ValueError(
            "fixed register-IAT bridge specs overlap one caller"
        )
    work.retail_register_storage_bridges = {
        **zinput_retail_register_storage_bridges,
        **briefing_retail_register_storage_bridges,
    }
    work.candidate_register_storage_bridges = {
        **zinput_candidate_register_storage_bridges,
        **briefing_candidate_register_storage_bridges,
    }
    (
        shield_retail_register_storage_bridges,
        work.shield_candidate_static_storage_bridges,
    ) = _cc_recoil_hud_shield._hud_shield_layout_affine_storage_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
    )
    (
        shield_meter_retail_register_storage_bridges,
        work.shield_meter_candidate_static_storage_bridges,
    ) = _cc_recoil_hud_sensor._hud_ui_mgr_sensor_shield_meter_affine_storage_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        selected_progress_retail_register_storage_bridges,
        selected_progress_candidate_register_storage_bridges,
    ) = (
        _cc_recoil_hud_visibility._hud_ui_mgr_selected_progress_set_visible_register_storage_bridges(
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
        chat_compose_retail_register_storage_bridges,
        chat_compose_candidate_register_storage_bridges,
    ) = _cc_recoil_network._gamenet_chat_compose_candidate_register_storage_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        end_chat_retail_register_storage_bridges,
        end_chat_candidate_register_storage_bridges,
    ) = _cc_recoil_network._gamenet_end_chat_compose_strncat_register_storage_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
    )
    (
        hide_tracked_progress_retail_register_storage_bridges,
        hide_tracked_progress_candidate_register_storage_bridges,
    ) = (
        _cc_recoil_hud_visibility._hud_ui_mgr_hide_tracked_progress_set_visible_register_storage_bridges(
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
        objective_dirty_retail_register_storage_bridges,
        objective_dirty_candidate_register_storage_bridges,
    ) = (
        _cc_recoil_hud_layout._hud_layout_hw_update_objective_dirty_rect_register_storage_bridges(
            work.retail_instructions,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.retail_register_storage_bridges, shield_retail_register_storage_bridges, family="session.retail_register_storage_bridges")
    _cc_proofs.merge_into(work.retail_register_storage_bridges, shield_meter_retail_register_storage_bridges, family="session.retail_register_storage_bridges")
    _cc_proofs.merge_into(work.retail_register_storage_bridges, selected_progress_retail_register_storage_bridges, family="session.retail_register_storage_bridges")
    _cc_proofs.merge_into(work.candidate_register_storage_bridges, selected_progress_candidate_register_storage_bridges, family="session.candidate_register_storage_bridges")
    _cc_proofs.merge_into(work.retail_register_storage_bridges, chat_compose_retail_register_storage_bridges, family="session.retail_register_storage_bridges")
    _cc_proofs.merge_into(work.candidate_register_storage_bridges, chat_compose_candidate_register_storage_bridges, family="session.candidate_register_storage_bridges")
    _cc_proofs.merge_into(work.retail_register_storage_bridges, end_chat_retail_register_storage_bridges, family="session.retail_register_storage_bridges")
    _cc_proofs.merge_into(work.candidate_register_storage_bridges, end_chat_candidate_register_storage_bridges, family="session.candidate_register_storage_bridges")
    _cc_proofs.merge_into(work.retail_register_storage_bridges, hide_tracked_progress_retail_register_storage_bridges, family="session.retail_register_storage_bridges")
    _cc_proofs.merge_into(work.candidate_register_storage_bridges, hide_tracked_progress_candidate_register_storage_bridges, family="session.candidate_register_storage_bridges")
    _cc_proofs.merge_into(work.retail_register_storage_bridges, objective_dirty_retail_register_storage_bridges, family="session.retail_register_storage_bridges")
    _cc_proofs.merge_into(work.candidate_register_storage_bridges, objective_dirty_candidate_register_storage_bridges, family="session.candidate_register_storage_bridges")
    _cc_retail._briefing_beginthread_direct_iat_storage_bridge(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
        bridge_data_rows=work.bridge_data_rows,
    )
    _cc_retail._briefing_strerror_direct_iat_storage_bridge(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
        bridge_data_rows=work.bridge_data_rows,
    )
    work.indexes = _cc_retail._hud_timer_panel_float_retail_storage_indexes(
        work.retail_instructions,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge=work.bridge,
    )
    work.indexes = _cc_lifecycle._retail_constructor_absolute_table_storage_indexes(
        work.retail_instructions,
        indexes=work.indexes,
        bridge=work.bridge,
    )
    work.retail_provenance_adapter_sites_by_symbol[work.symbol_id] = list(
        work.reviewed_retail_adapters.reviewed_call_sites
    )
    work.retail_switch_targets = dict(
        work.retail_proof_package.local_control_flow_targets
    )
    work.retail_layout_loop_vptr_bridges = (
        _cc_recoil_hud_layout._hud_ui_mgr_layout_array_loop_vptr_storage_bridges(
            work.retail_instructions,
            source="bn",
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
        )
    )
    constructor_table_retail_vptr_bridges = (
        _cc_lifecycle._retail_constructor_table_dispatch_bridges(
            work.retail_instructions,
            indexes=work.indexes,
            bridge=work.bridge,
        )
    )
    _cc_proofs.merge_into(constructor_table_retail_vptr_bridges, work.reviewed_retail_adapters.vptr_storage_bridges, family="session.constructor_table_retail_vptr_bridges")
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, constructor_table_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    (
        set_value_retail_vptr_bridges,
        work.set_value_candidate_vptr_bridges,
    ) = _cc_recoil_hud_messages._hud_ui_message_set_value_vptr_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    _cc_proofs.merge_into(work.retail_layout_loop_vptr_bridges, set_value_retail_vptr_bridges, family="session.retail_layout_loop_vptr_bridges")
    (
        work.clear_display_retail_vptr_bridges,
        work.clear_display_candidate_vptr_bridges,
    ) = _cc_recoil_hud_messages._hud_ui_message_clear_display_vptr_bridges(
        work.retail_instructions,
        work.candidate_assembly,
        document=work.document,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
        bridge_names=work.exact_bridge_names,
    )
