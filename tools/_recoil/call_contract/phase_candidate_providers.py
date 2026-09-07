"""Serial recover candidate providers proof phase."""
from __future__ import annotations

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import candidate_imports as _cc_candidate_imports
from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import providers as _cc_providers
from _recoil.call_contract import recoil_application as _cc_recoil_application
from _recoil.call_contract import recoil_audio as _cc_recoil_audio
from _recoil.call_contract import recoil_hud_construction as _cc_recoil_hud_construction
from _recoil.call_contract import recoil_hud_lifetimes as _cc_recoil_hud_lifetimes
from _recoil.call_contract import recoil_hud_objectives as _cc_recoil_hud_objectives
from _recoil.call_contract import recoil_hud_panels as _cc_recoil_hud_panels
from _recoil.call_contract import recoil_hud_stats as _cc_recoil_hud_stats
from _recoil.call_contract import recoil_hud_visibility as _cc_recoil_hud_visibility
from _recoil.call_contract import recoil_hud_widgets as _cc_recoil_hud_widgets
from _recoil.call_contract import recoil_lifecycle as _cc_recoil_lifecycle
from _recoil.call_contract import recoil_player as _cc_recoil_player
from _recoil.call_contract import retail_imports as _cc_retail_imports
from _recoil.call_contract import work as _cc_work


def recover_candidate_providers(work: _cc_work.CallerWork) -> None:

    ensure_objective_label_setpos_absolute_load_bridges = (
        _cc_recoil_hud_objectives._hud_ui_mgr_ensure_objective_label_setpos_candidate_absolute_load_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.ensure_sensor_absolute_load_bridges, ensure_objective_label_setpos_absolute_load_bridges, family="session.ensure_sensor_absolute_load_bridges")
    (
        ensure_objective_label_settextfmt_absolute_load_bridges,
        work.ensure_objective_label_settextfmt_candidate_vptr_bridges,
    ) = (
        _cc_recoil_hud_objectives._hud_ui_mgr_ensure_objective_label_settextfmt_candidate_vptr_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.ensure_sensor_absolute_load_bridges, ensure_objective_label_settextfmt_absolute_load_bridges, family="session.ensure_sensor_absolute_load_bridges")
    work.ensure_remaining_indirect_bridges = (
        _cc_recoil_hud_construction._hud_ui_mgr_ensure_remaining_indirect_candidate_bridges(
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
    _cc_proofs.merge_into(work.ensure_remaining_indirect_bridges, work.r4564_callback_exact_indirect_bridges, family="session.ensure_remaining_indirect_bridges")
    mp_exit_deactivate_indirect_bridges = (
        _cc_recoil_application._recoil_app_mp_exit_deactivate_candidate_indirect_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.ensure_remaining_indirect_bridges, mp_exit_deactivate_indirect_bridges, family="session.ensure_remaining_indirect_bridges")
    mp_exit_update_should_quit_indirect_bridges = (
        _cc_recoil_application._recoil_app_mp_exit_update_should_quit_candidate_indirect_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.ensure_remaining_indirect_bridges, mp_exit_update_should_quit_indirect_bridges, family="session.ensure_remaining_indirect_bridges")
    mp_exit_update_indirect_bridges = (
        _cc_recoil_hud_widgets._hud_ui_mp_exit_update_candidate_indirect_bridge(
            work.expected,
            work.candidate_assembly,
            document=work.document,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    _cc_proofs.merge_into(work.ensure_remaining_indirect_bridges, mp_exit_update_indirect_bridges, family="session.ensure_remaining_indirect_bridges")
    _cc_proofs.merge_into(work.ensure_remaining_indirect_bridges, work.track_marker_exit_candidate_vptr_bridges, family="session.ensure_remaining_indirect_bridges")
    pre_ftol_direct_bridges = dict(work.registered_regex_bridges)
    for work.additions, work.label in (
        (
            work.named_thunk_candidate_direct_bridges,
            "named import direct",
        ),
        (
            work.expected_direct_identity_bridges,
            "expected authored direct",
        ),
    ):
        _cc_proofs.merge_into(pre_ftol_direct_bridges, work.additions, family="session.pre_ftol_direct_bridges")
    bridge_conflicts = (
        pre_ftol_direct_bridges.keys()
        & work.hud_ui_mgr_zrd_payload_bridges.keys()
    )
    _cc_proofs.merge_into(pre_ftol_direct_bridges, work.hud_ui_mgr_zrd_payload_bridges, family="session.pre_ftol_direct_bridges")
    ensure_unsupported_helper_bridges = (
        _cc_recoil_hud_stats._hud_ui_mgr_ensure_unsupported_helpers_candidate_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            compiler_generated_bridges=pre_ftol_direct_bridges,
        )
    )
    bridge_conflicts = (
        pre_ftol_direct_bridges.keys()
        & ensure_unsupported_helper_bridges.keys()
    )
    if bridge_conflicts:
        raise ValueError(
            "HUD EnsureHudLoaded unsupported-helper candidate bridge "
            "conflicts with another exact candidate identity: "
            + ", ".join(
                repr(name) for name in sorted(bridge_conflicts)
            )
        )
    _cc_proofs.merge_into(pre_ftol_direct_bridges, ensure_unsupported_helper_bridges, family="session.pre_ftol_direct_bridges")
    work.zsnd_static_coordinator_bridges = (
        _cc_recoil_audio._zsnd_static_coordinator_candidate_bridges(
            work.expected,
            work.candidate_assembly,
            work.retail_instructions,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
    directsoundcreate_ordinal1_bridges = (
        _cc_recoil_audio._directsoundcreate_ordinal1_candidate_bridge(
            work.expected,
            work.candidate_assembly,
            work.retail_instructions,
            document=work.document,
            thunks=work.provider_ordinal_import_thunks,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            bridge=work.bridge,
            retail_import_targets=work.retail_import_targets,
        )
    )
    bridge_conflicts = (
        pre_ftol_direct_bridges.keys()
        & directsoundcreate_ordinal1_bridges.keys()
    )
    _cc_proofs.merge_into(pre_ftol_direct_bridges, directsoundcreate_ordinal1_bridges, family="session.pre_ftol_direct_bridges")
    bridge_conflicts = (
        pre_ftol_direct_bridges.keys()
        & work.zsnd_static_coordinator_bridges.keys()
    )
    _cc_proofs.merge_into(pre_ftol_direct_bridges, work.zsnd_static_coordinator_bridges, family="session.pre_ftol_direct_bridges")
    work.candidate_local_callable_bridges = (
        _cc_providers._candidate_local_coff_callable_bridges(
            work.candidate_assembly,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            compiler_generated_bridges=pre_ftol_direct_bridges,
        )
    )
    _cc_proofs.merge_into(work.candidate_local_callable_bridges, work.zsnd_static_coordinator_bridges, family="session.candidate_local_callable_bridges")
    candidate_local_preflight_conflicts = (
        pre_ftol_direct_bridges.keys()
        & work.candidate_local_callable_bridges.keys()
    )
    candidate_local_identity_conflicts = {
        name
        for name in candidate_local_preflight_conflicts
        if pre_ftol_direct_bridges[name]
        != work.candidate_local_callable_bridges[name]
    }
    if candidate_local_identity_conflicts:
        raise ValueError(
            "candidate-local COFF callable preflight conflicts with "
            "a reviewed direct bridge: "
            + ", ".join(
                repr(name)
                for name in sorted(
                    candidate_local_identity_conflicts
                )
            )
        )
    _cc_proofs.merge_into(pre_ftol_direct_bridges, work.candidate_local_callable_bridges, family="session.pre_ftol_direct_bridges")
    work.ftol_provider_bridges = (
        _cc_retail_imports._ftol_provider_bridges(
            work.expected,
            work.candidate_assembly,
            provider_identity=work.ftol_provider_identity,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            reviewed_candidate_direct_bridges=(
                pre_ftol_direct_bridges
            ),
            reviewed_loop_vptr_storage_bridges=(
                work.ensure_font_loop_candidate_vptr_bridges
            ),
            reviewed_absolute_storage_load_bridges=(
                work.ensure_sensor_absolute_load_bridges
            ),
        )
    )
    (
        work.stats_list_static_bridges,
        work.stats_list_member_vptr_bridges,
    ) = _cc_recoil_hud_visibility._hud_ui_mgr_stats_list_set_visible_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    ordinal_supplemental_direct_bridges = (
        _cc_callable_identity._compose_supplemental_candidate_direct_bridges(
            work.wol_cedit_provider_bridges,
            work.wol_category_a_direct_bridges,
            work.chkstk_compiler_helper_bridges,
        )
    )
    ordinal_import_thunk_bridges = (
        _cc_candidate_imports._provider_ordinal_import_thunk_candidate_bridges_from_reviewed_directs(
            work.expected,
            work.candidate_assembly,
            thunks=work.provider_ordinal_import_thunks,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.candidate_exact_iat_indexes,
            bridge_names=work.exact_bridge_names,
            reviewed_candidate_direct_bridges=(
                pre_ftol_direct_bridges
            ),
            supplemental_candidate_direct_bridges=(
                ordinal_supplemental_direct_bridges
            ),
            reviewed_register_storage_bridges=(
                work.candidate_register_storage_bridges
            ),
            reviewed_static_storage_reference_bridges=(
                work.stats_list_static_bridges
            ),
            reviewed_member_vptr_storage_bridges=(
                work.stats_list_member_vptr_bridges
            ),
            candidate_exact_iat_register_load_proofs=(
                work.candidate_exact_iat_load_proofs
            ),
        )
    )
    work.compiler_generated_bridges = dict(
        work.eh_array_destructor_bridges
    )
    _cc_recoil_lifecycle._assert_eh_array_destructor_candidate_identity_stage(
        work.eh_array_destructor_bridges,
        work.candidate_exact_iat_indexes,
        stage="compiler-generated-composition",
        final_bridges=work.compiler_generated_bridges,
    )
    work.compiler_generated_bridges = (
        _cc_recoil_audio._merge_directsoundcreate_ordinal1_final_bridges(
            work.compiler_generated_bridges,
            directsoundcreate_ordinal1_bridges,
        )
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.ftol_provider_bridges, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & work.gettickcount_candidate_direct_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.gettickcount_candidate_direct_bridges, family="session.compiler_generated_bridges")
    for work.additions, work.label in (
        (
            work.named_thunk_candidate_direct_bridges,
            "named import direct",
        ),
        (
            work.expected_direct_identity_bridges,
            "expected authored direct",
        ),
    ):
        if work.label == "expected authored direct":
            work.compiler_generated_bridges = (
                _cc_recoil_player._merge_player_ftol_expected_direct_bridges(
                    work.compiler_generated_bridges,
                    work.additions,
                    caller_start=work.address,
                    player_compiler_provider_bridges=(
                        work.player_compiler_provider_bridges
                    ),
                    ftol_provider_bridges=work.ftol_provider_bridges,
                )
            )
            continue
        _cc_proofs.merge_into(work.compiler_generated_bridges, work.additions, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & work.chkstk_compiler_helper_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.chkstk_compiler_helper_bridges, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & work.ciasin_provider_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.ciasin_provider_bridges, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & work.zwep_ciasin_provider_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.zwep_ciasin_provider_bridges, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & work.wol_cedit_provider_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.wol_cedit_provider_bridges, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & work.wol_category_a_direct_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.wol_category_a_direct_bridges, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & work.cls_util_authored_identity_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.cls_util_authored_identity_bridges, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & ordinal_import_thunk_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, ordinal_import_thunk_bridges, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & work.registered_regex_bridges.keys()
    )
    if bridge_conflicts:
        raise ValueError(
            "registered target symbol-regex candidate bridge conflicts "
            "with another exact candidate identity: "
            + ", ".join(
                repr(name) for name in sorted(bridge_conflicts)
            )
        )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.registered_regex_bridges, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & work.hud_ui_mgr_zrd_payload_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, work.hud_ui_mgr_zrd_payload_bridges, family="session.compiler_generated_bridges")
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & ensure_unsupported_helper_bridges.keys()
    )
    if bridge_conflicts:
        raise ValueError(
            "HUD EnsureHudLoaded unsupported-helper candidate bridge "
            "conflicts with another reviewed compiler/provider bridge: "
            + ", ".join(
                repr(name) for name in sorted(bridge_conflicts)
            )
        )
    _cc_proofs.merge_into(work.compiler_generated_bridges, ensure_unsupported_helper_bridges, family="session.compiler_generated_bridges")
    hud_triplet_sort_key_bridges = (
        _cc_recoil_hud_panels._hud_triplet_sort_key_tu_local_candidate_bridge(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
        )
    )
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & hud_triplet_sort_key_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, hud_triplet_sort_key_bridges, family="session.compiler_generated_bridges")
    hud_triplet_ensure_capacity_bridges = (
        _cc_recoil_hud_construction._hud_triplet_ensure_capacity_tu_local_candidate_bridge(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            compiler_generated_bridges=(
                work.compiler_generated_bridges
            ),
        )
    )
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & hud_triplet_ensure_capacity_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, hud_triplet_ensure_capacity_bridges, family="session.compiler_generated_bridges")
    hud_triplet_panel_constructor_bridges = (
        _cc_recoil_hud_panels._hud_triplet_panel_constructor_candidate_bridge(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            compiler_generated_bridges=work.compiler_generated_bridges,
        )
    )
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & hud_triplet_panel_constructor_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, hud_triplet_panel_constructor_bridges, family="session.compiler_generated_bridges")
    work.exact_constructor_bridges: dict[str, str] = {}
    hud_text_input_constructor_bridges = (
        _cc_recoil_hud_widgets._hud_text_input_constructor_alias_candidate_bridge(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
            bridge_names=work.exact_bridge_names,
            compiler_generated_bridges=work.compiler_generated_bridges,
        )
    )
    bridge_conflicts = (
        work.compiler_generated_bridges.keys()
        & hud_text_input_constructor_bridges.keys()
    )
    _cc_proofs.merge_into(work.compiler_generated_bridges, hud_text_input_constructor_bridges, family="session.compiler_generated_bridges")
    (
        work.first_delete_absolute_bridges,
        work.first_delete_member_vptr_bridges,
    ) = _cc_recoil_hud_lifetimes._hud_ui_mgr_first_deleting_destructor_candidate_bridges(
        work.expected,
        work.candidate_assembly,
        caller_identity=work.caller_identity,
        caller_start=work.address,
        caller_end_exclusive=work.end_exclusive,
        indexes=work.indexes,
    )
    (
        work.timer_panel_delete_absolute_bridges,
        work.timer_panel_delete_member_vptr_bridges,
    ) = (
        _cc_recoil_hud_lifetimes._hud_ui_mgr_timer_panel_deleting_destructor_candidate_bridges(
            work.expected,
            work.candidate_assembly,
            caller_identity=work.caller_identity,
            caller_start=work.address,
            caller_end_exclusive=work.end_exclusive,
            indexes=work.indexes,
        )
    )
