from __future__ import annotations

import argparse
from copy import deepcopy
from contextlib import redirect_stderr, redirect_stdout
import io
import json
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands import (  # noqa: E402
    call_contract_convergence as convergence,
    call_contract_verify as call_contract_verify_command,
    linked_order,
    progress_cli,
    vc5_build,
    workspace_issues,
)
from _recoil.commands.progress_cli import (  # noqa: E402
    _accept_authored_non_gating_blocks,
    _call_contract_claim_candidate,
    _call_contract_slice_write_paths,
    _order_claim_candidate,
    _parse_authored_non_gating_block_accept_payload,
    _parse_provider_block_reclassify_payload,
    _parse_owner_downgrade_payload,
    _parse_owner_primary_data_tier_x_repair_payload,
    _parse_owner_replace_batch_payload,
    _parse_logical_alias_group_payload,
    _parse_function_padding_correction_payload,
    _parse_physical_block_replace_payload,
    _mutate_authored_storage_no_new_debt,
    _registration_paths,
    _replace_function_with_padding,
    _replace_physical_block,
    _reclassify_provider_block,
    _retire_verification_target,
    _downgrade_owner,
    _repair_owner_primary_data_tier_x,
    _replace_owner_batch,
    _set_symbol_logical_alias_group,
    _sync_verification_targets,
)
from _recoil.commands.call_contract_verify import (  # noqa: E402
    call_contract_source_closure,
)
from _recoil.lib.live_progress import (  # noqa: E402
    ConcurrentRevisionUpdate,
    RevisionStore,
    allocate_revision_entity_id,
    revision_entity_id,
)
from _recoil.lib.issue_sqlite import create_issue_database  # noqa: E402
from _recoil.lib.progress import (  # noqa: E402
    AUTHORED_ORDER_DIMENSIONS,
    FULL_ORDER_DIMENSIONS,
    ProgressDocument,
    ProgressError,
    create_and_reserve_claim_current_work_item,
    create_and_reserve_repair_continuation_work_item,
    empty_progress_document,
    state_record,
)
from _recoil.lib.worktree_control import resolve_canonical_control_root  # noqa: E402


def canonical_retail_reference() -> Path:
    resolution = resolve_canonical_control_root(
        executing_worktree_root=REPO_ROOT,
        required_machine_local_paths=("support/Recoil.exe",),
    )
    return resolution.canonical_control_root / "support" / "Recoil.exe"


from _recoil.lib.verification_targets import (  # noqa: E402
    load_target_registrations,
)


class RevisionIdentityTests(unittest.TestCase):
    def test_progress_cli_registered_manifest_uses_exact_git_spelling(self) -> None:
        exact = {
            "registration": {
                "manifest_path": "tools/vc5_verify_targets/ainet_text_block_order.json"
            }
        }
        self.assertEqual(
            ("0x401060", "0x4038a0"),
            progress_cli._registered_order_interval(exact),
        )

        for supplied, message in (
            (
                "tools/VC5_VERIFY_TARGETS/ainet_text_block_order.json",
                "expected 'tools/vc5_verify_targets/ainet_text_block_order.json'",
            ),
            (
                str(
                    (
                        REPO_ROOT
                        / "tools/vc5_verify_targets/ainet_text_block_order.json"
                    ).resolve()
                ),
                "normalized repo-local path",
            ),
            (
                "tools/vc5_verify_targets/not_registered_here.json",
                "Git-tracked path",
            ),
        ):
            target = {"registration": {"manifest_path": supplied}}
            with self.subTest(supplied=supplied), self.assertRaisesRegex(
                ProgressError, message
            ):
                progress_cli._registered_order_interval(target)

    def test_revision_entity_ids_are_typed_and_zero_padded(self) -> None:
        self.assertEqual(revision_entity_id("evidence", 725, 1), "recoil:evidence:r725:000001")
        data = {
            "evidence": {
                "recoil:evidence:r725:000001": {},
                "recoil:evidence:r725:000003": {},
            }
        }
        self.assertEqual(
            allocate_revision_entity_id(data, kind="evidence", revision=725, collection="evidence"),
            "recoil:evidence:r725:000002",
        )

    def test_revision_store_uses_revision_only_concurrency(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "store.json"
            path.write_text(
                json.dumps({"version": 2, "revision": 3, "items": []}) + "\n",
                encoding="utf-8",
            )
            store = RevisionStore(path, schema_field="version", schema_version=2)
            proposed = store.load()
            proposed["items"].append("direct-comparison")
            dry = store.commit(proposed, expected_revision=3, apply=False)
            applied = store.commit(proposed, expected_revision=3, apply=True)
            self.assertFalse(dry.applied)
            self.assertTrue(applied.applied)
            self.assertEqual(dry.revision, applied.revision)
            self.assertEqual(json.loads(path.read_text(encoding="utf-8"))["revision"], 4)
            with self.assertRaises(ConcurrentRevisionUpdate):
                store.commit(proposed, expected_revision=3, apply=True)


class LogicalAliasGroupInvalidationTests(unittest.TestCase):
    @staticmethod
    def _classification_payload(*, lifecycle: bool, winner_unknown: bool) -> dict:
        from tests.tools.recoil_cli_tests import RecoilCliTests

        payload = deepcopy(RecoilCliTests.logical_alias_group_payload())
        if lifecycle:
            for alias in payload["logical_aliases"].values():
                alias["pipeline_class"] = "authored-lifecycle"
                alias["authored_order_role"] = "authored-lifecycle-body"
        if winner_unknown:
            payload["icf_address_group"]["winner_status"] = "winner-unknown"
            payload["icf_address_group"]["winner_identity_key"] = None
            for alias in payload["logical_aliases"].values():
                alias["fold_status"] = "proven-fold-alias"
        return payload

    def test_logical_alias_group_accepts_exact_ordinary_and_lifecycle_pairs(self) -> None:
        for lifecycle in (False, True):
            with self.subTest(lifecycle=lifecycle):
                payload = self._classification_payload(
                    lifecycle=lifecycle,
                    winner_unknown=True,
                )
                parsed = _parse_logical_alias_group_payload(json.dumps(payload))
                self.assertEqual(
                    "winner-unknown",
                    parsed["icf_address_group"]["winner_status"],
                )
                self.assertIsNone(
                    parsed["icf_address_group"]["winner_identity_key"]
                )

    def test_logical_alias_group_rejects_invalid_winner_status_key_and_fold_pairs(self) -> None:
        cases = {}
        payload = self._classification_payload(lifecycle=True, winner_unknown=True)
        payload["icf_address_group"]["winner_status"] = "unreviewed"
        cases["status"] = (payload, "winner_status")
        payload = self._classification_payload(lifecycle=True, winner_unknown=True)
        payload["icf_address_group"]["winner_identity_key"] = next(
            iter(payload["logical_aliases"])
        )
        cases["unknown-with-key"] = (payload, "must be null")
        payload = self._classification_payload(lifecycle=True, winner_unknown=True)
        next(iter(payload["logical_aliases"].values()))["fold_status"] = (
            "selected-winner"
        )
        cases["unknown-with-selected"] = (payload, "every logical alias")
        payload = self._classification_payload(lifecycle=True, winner_unknown=False)
        payload["icf_address_group"]["winner_identity_key"] = None
        cases["selected-without-key"] = (payload, "non-empty string")
        payload = self._classification_payload(lifecycle=True, winner_unknown=True)
        next(iter(payload["logical_aliases"].values()))[
            "authored_order_role"
        ] = "authored-body"
        cases["mixed-classification"] = (payload, "exact authored classification pair")

        for name, (case, message) in cases.items():
            with self.subTest(name=name), self.assertRaisesRegex(
                ProgressError, message
            ):
                _parse_logical_alias_group_payload(json.dumps(case))

    def test_logical_alias_group_v2_rejects_caller_ids_scopes_and_candidate_inputs(
        self,
    ) -> None:
        from tests.tools.recoil_cli_tests import RecoilCliTests

        cases = {}
        payload = RecoilCliTests.logical_alias_group_v2_payload()
        payload["icf_address_group"]["evidence_ids"] = ["caller-selected"]
        cases["group-id"] = (payload, "evidence_ids are generated")
        payload = RecoilCliTests.logical_alias_group_v2_payload()
        next(iter(payload["logical_aliases"].values()))["evidence_ids"] = [
            "caller-selected"
        ]
        cases["alias-id"] = (payload, "evidence_ids are generated")
        payload = RecoilCliTests.logical_alias_group_v2_payload()
        payload["new_evidence"]["provenance"]["scope_ids"] = ["caller-scope"]
        cases["scope"] = (payload, "must not supply scope_ids")
        payload = RecoilCliTests.logical_alias_group_v2_payload()
        payload["new_evidence"]["provenance"]["candidate_independent"] = False
        cases["candidate-dependent"] = (payload, "candidate_independent=true")
        payload = RecoilCliTests.logical_alias_group_v2_payload()
        payload["new_evidence"]["validation_context"]["candidate_output_used"] = True
        cases["candidate-output"] = (payload, "candidate_output_used=false")
        payload = RecoilCliTests.logical_alias_group_v2_payload()
        payload["new_evidence"]["artifacts"] = [
            {
                "path": "build/reconstruction-evidence/runs/candidate.obj",
                "size": 1,
            }
        ]
        cases["candidate-artifact"] = (payload, "candidate output/artifact")

        for name, (case, message) in cases.items():
            with self.subTest(name=name), self.assertRaisesRegex(
                ProgressError, message
            ):
                _parse_logical_alias_group_payload(json.dumps(case))

    def test_logical_alias_group_v3_is_existing_group_only_and_preserves_status(
        self,
    ) -> None:
        from tests.tools.recoil_cli_tests import RecoilCliTests

        data = RecoilCliTests.logical_alias_group_v3_document()
        payload = RecoilCliTests.logical_alias_group_v3_payload(data)
        parsed = _parse_logical_alias_group_payload(json.dumps(payload))
        provisional_id = "recoil:logical-function:0x401000:api-add-ref"
        self.assertEqual(
            "provisional",
            parsed["logical_aliases"][provisional_id]["original_name_status"],
        )

        cases = {}
        changed = deepcopy(payload)
        changed["icf_address_group"] = deepcopy(
            changed["current"]["icf_address_group"]
        )
        cases["replacement-field"] = (changed, "unsupported icf_address_group")
        changed = deepcopy(payload)
        changed["current"]["icf_address_group"] = None
        cases["missing-existing-group"] = (changed, "existing-group-only")
        changed = deepcopy(payload)
        changed["current"]["logical_aliases"][provisional_id][
            "original_name_status"
        ] = "invented"
        cases["status-upgrade"] = (changed, "preserved existing value")
        changed = deepcopy(payload)
        changed["current"]["icf_address_group"]["winner_status"] = (
            "selected-winner"
        )
        changed["current"]["icf_address_group"]["winner_identity_key"] = (
            provisional_id
        )
        changed["current"]["logical_aliases"][provisional_id]["fold_status"] = (
            "selected-winner"
        )
        cases["selected-winner"] = (changed, "existing winner-unknown")

        for label, (case, message) in cases.items():
            with self.subTest(label=label), self.assertRaisesRegex(
                ProgressError, message
            ):
                _parse_logical_alias_group_payload(json.dumps(case))

    def test_logical_alias_group_v3_fails_closed_on_cas_owner_and_target_drift(
        self,
    ) -> None:
        from tests.tools.recoil_cli_tests import RecoilCliTests

        def fixture() -> tuple[dict, dict, str]:
            data = RecoilCliTests.logical_alias_group_v3_document()
            payload = _parse_logical_alias_group_payload(
                json.dumps(RecoilCliTests.logical_alias_group_v3_payload(data))
            )
            return data, payload, "recoil:vc5-target:fixture-logical-alias-v3"

        data, payload, _target_id = fixture()
        payload["current"]["logical_aliases"][
            "recoil:logical-function:0x401000:api-add-ref"
        ]["original_name"] = "Renamed::Alias"
        with self.assertRaisesRegex(ProgressError, "current state is stale"):
            _set_symbol_logical_alias_group(data, payload)

        data, payload, _target_id = fixture()
        data["owners"]["recoil:owner:fixture.api"]["provider_state"] = (
            "provider-owned"
        )
        with self.assertRaisesRegex(ProgressError, "non-provider Recoil owner"):
            _set_symbol_logical_alias_group(data, payload)

        data, payload, _target_id = fixture()
        data["verification_targets"].clear()
        with self.assertRaisesRegex(ProgressError, "exactly one governed VC5 target"):
            _set_symbol_logical_alias_group(data, payload)

        data, payload, target_id = fixture()
        data["verification_targets"][target_id + "-duplicate"] = deepcopy(
            data["verification_targets"][target_id]
        )
        with self.assertRaisesRegex(ProgressError, "found 2"):
            _set_symbol_logical_alias_group(data, payload)

        data, payload, target_id = fixture()
        data["verification_targets"][target_id]["registration"][
            "translation_unit_function_order"
        ][0]["functions"].pop()
        synchronized = deepcopy(data["verification_targets"][target_id])
        with patch(
            "_recoil.lib.verification_targets.vc5_target_registration",
            return_value=(target_id, synchronized),
        ), self.assertRaisesRegex(ProgressError, "complete exact governed alias"):
            _set_symbol_logical_alias_group(data, payload)

    def test_logical_alias_group_v3_never_uses_target_display_name_as_authority(
        self,
    ) -> None:
        from tests.tools.recoil_cli_tests import RecoilCliTests

        data = RecoilCliTests.logical_alias_group_v3_document()
        payload = _parse_logical_alias_group_payload(
            json.dumps(RecoilCliTests.logical_alias_group_v3_payload(data))
        )
        target_id = "recoil:vc5-target:fixture-logical-alias-v3"
        target = data["verification_targets"][target_id]
        target["registration"]["translation_unit_function_order"][0][
            "functions"
        ][1]["name"] = "Untrusted::ProvisionalDisplayName"
        synchronized = deepcopy(target)
        with patch(
            "_recoil.lib.verification_targets.vc5_target_registration",
            return_value=(target_id, synchronized),
        ):
            details = _set_symbol_logical_alias_group(data, payload)
        self.assertEqual(target_id, details["governed_target_id"])
        self.assertEqual("physical-icf-group-only", details["authority_scope"])
        self.assertEqual(
            "provisional",
            data["symbols"]["recoil:function:0x401000"]["logical_aliases"][
                "recoil:logical-function:0x401000:api-add-ref"
            ]["original_name_status"],
        )

    def test_logical_alias_group_invalidates_dependent_order_and_byte_facts(self) -> None:
        from tests.tools.recoil_cli_tests import RecoilCliTests

        data = RecoilCliTests.logical_alias_group_document()
        block_id = "recoil:block:0x401000"
        symbol_id = "recoil:function:0x401000"
        block = data["physical_blocks"][block_id]
        block["accepted_order_facts"] = {"stale": True}
        block["order"] = {
            "authored": {
                name: state_record(result="accepted", evidence_ids=["stale-order"])
                for name in AUTHORED_ORDER_DIMENSIONS
            },
            "full": {
                name: state_record(result="accepted", evidence_ids=["stale-order"])
                for name in FULL_ORDER_DIMENSIONS
            },
        }
        row = data["symbols"][symbol_id]
        row["accepted_order_facts"] = {"stale": True}
        row["accepted_byte_facts"] = {"stale": True}
        row["binary_state"] = {
            name: state_record(result="accepted", evidence_ids=["stale-byte"])
            for name in (
                "object_byte",
                "relocation_identity",
                "linked_presence",
                "linked_body_byte",
                "linked_address",
                "linked_targets",
                "linked_target_identity",
                "linked_byte",
            )
        }
        payload = _parse_logical_alias_group_payload(
            json.dumps(RecoilCliTests.logical_alias_group_payload())
        )

        details = _set_symbol_logical_alias_group(data, payload)

        self.assertEqual([block_id], details["invalidated"]["block_ids"])
        self.assertEqual([symbol_id], details["invalidated"]["symbol_ids"])
        self.assertIsNone(block.get("accepted_order_facts"))
        self.assertIsNone(row["accepted_order_facts"])
        self.assertIsNone(row["accepted_byte_facts"])
        for family in block["order"].values():
            for state in family.values():
                self.assertEqual("pending", state["result"])
                self.assertEqual("changed", state["freshness"])
                self.assertEqual([], state["evidence_ids"])
        for state in row["binary_state"].values():
            self.assertEqual("pending", state["result"])
            self.assertEqual("changed", state["freshness"])
            self.assertEqual([], state["evidence_ids"])


class OrderEditPathContractTests(unittest.TestCase):
    _fixture_manifest_paths = frozenset(
        {
            "tools/vc5_verify_targets/briefing.json",
            "tools/vc5_verify_targets/camera-order.json",
            "tools/vc5_verify_targets/unit-order.json",
            "tools/vc5_verify_targets/unit.json",
            "tools/vc5_verify_targets/zinput-keyboard-order.json",
            "tools/vc5_verify_targets/znetwork-order.json",
        }
    )

    def setUp(self) -> None:
        """Keep copied/in-memory manifest fixtures explicit and fixture-local."""

        original_progress_resolver = progress_cli._resolve_tracked_progress_file
        original_contract_resolver = (
            call_contract_verify_command.resolve_tracked_repository_file
        )

        def progress_fixture_resolver(
            path_text: str,
            *,
            context: str,
            inventory=None,
        ):
            if path_text in self._fixture_manifest_paths:
                return SimpleNamespace(
                    git_path=path_text,
                    physical_path=REPO_ROOT / path_text,
                    repository_root=REPO_ROOT,
                )
            return original_progress_resolver(
                path_text,
                context=context,
                inventory=inventory,
            )

        def contract_fixture_resolver(path_text: str, **kwargs):
            if path_text in self._fixture_manifest_paths:
                return SimpleNamespace(
                    git_path=path_text,
                    physical_path=REPO_ROOT / path_text,
                    repository_root=REPO_ROOT,
                )
            return original_contract_resolver(path_text, **kwargs)

        progress_patch = patch.object(
            progress_cli,
            "_resolve_tracked_progress_file",
            side_effect=progress_fixture_resolver,
        )
        contract_patch = patch.object(
            call_contract_verify_command,
            "resolve_tracked_repository_file",
            side_effect=contract_fixture_resolver,
        )
        progress_patch.start()
        contract_patch.start()
        self.addCleanup(progress_patch.stop)
        self.addCleanup(contract_patch.stop)
    @staticmethod
    def _same_host_bootstrap_fixture(
        *,
        provider_pipeline_class: str = "non-authored",
        provider_role: str = "non-authored",
    ) -> tuple[str, str, dict[str, object], dict[str, object], SimpleNamespace]:
        target_id = "recoil:vc5-target:znetwork-order"
        source_from = "src/GameZRecoil/zNetwork/znet_dplay.cpp"
        provider = SimpleNamespace(
            address="0x48bf10",
            authored_order_role=provider_role,
            pipeline_class=provider_pipeline_class,
            provenance="provider-boundary",
            logical_identity_key="",
            icf_fold_status="",
        )
        authored = SimpleNamespace(
            address="0x489d00",
            authored_order_role="authored-body",
            pipeline_class="authored",
            provenance="",
            logical_identity_key="",
            icf_fold_status="",
        )
        registration = {
            "binary": "recoil",
            "check_translation_unit_function_order": True,
            "function_order_scope": "authored",
            "manifest_path": "tools/vc5_verify_targets/znetwork-order.json",
            "name": "znetwork-order",
            "source_from": source_from,
            "translation_unit_function_order": [
                {
                    "source_from": source_from,
                    "order_scope": "authored",
                    "inventory_only": False,
                    "functions": [
                        {
                            "address": provider.address,
                            "pipeline_class": provider.pipeline_class,
                            "authored_order_role": provider.authored_order_role,
                        },
                        {
                            "address": authored.address,
                            "pipeline_class": authored.pipeline_class,
                            "authored_order_role": authored.authored_order_role,
                        },
                    ],
                }
            ],
            "linked_function_intervals": [],
            "order_edit_paths": [source_from],
        }
        loaded = {
            target_id: {
                "binary": "recoil",
                "kind": "vc5",
                "name": "znetwork-order",
                "registration": registration,
                "registered_addresses": [provider.address, authored.address],
            }
        }
        manifest_data = {
            "retail_start": "0x489d00",
            "retail_end_exclusive": "0x48c7d0",
            "order_edit_paths": [source_from],
        }
        target = SimpleNamespace(
            source_from=source_from,
            functions=(provider, authored),
            translation_unit_function_order=(
                SimpleNamespace(source_from=source_from, functions=(provider, authored)),
            ),
            generated_files=(),
            source_emission_policy_strict=False,
        )
        return target_id, source_from, loaded, manifest_data, target

    def test_registration_sync_preserves_canonical_order_edit_metadata(self) -> None:
        order_edit_paths = [
            "src/Battlesport/ai_net.cpp",
            "src/Battlesport/ai_net.h",
        ]
        registration = {
            "binary": "recoil",
            "check_function_order": True,
            "function_order_scope": "authored",
            "function_addresses": ["0x401000"],
            "functions": [{"address": "0x401000"}],
            "manifest_path": "tools/vc5_verify_targets/unit.json",
            "name": "unit",
            "order_edit_paths": order_edit_paths,
            "source_from": "src/Battlesport/ai_net.cpp",
            "translation_unit_function_order": [],
        }
        loaded = {
            "recoil:vc5-target:unit": {
                "binary": "recoil",
                "kind": "vc5",
                "name": "unit",
                "registration": registration,
                "registered_addresses": ["0x401000"],
            }
        }
        before = deepcopy(loaded["recoil:vc5-target:unit"])
        tracker = {
            "verification_targets": {},
            "symbols": {},
            "physical_blocks": {},
            "work_items": {
                "unit:primary": {
                    "lane": "primary",
                    "phase": "authored-function-order",
                    "state": "ready",
                }
            },
        }
        work_items_before = deepcopy(tracker["work_items"])
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch(
                "pathlib.Path.read_text",
                side_effect=AssertionError(
                    "ordinary sync must use canonical registration metadata"
                ),
            ),
            patch("_recoil.commands.progress_cli.load_vc5_manifest"),
        ):
            details = _sync_verification_targets(tracker, binary="recoil", selectors=["unit"])

        stored = tracker["verification_targets"]["recoil:vc5-target:unit"]
        self.assertEqual(before, stored)
        self.assertEqual(order_edit_paths, stored["registration"]["order_edit_paths"])
        self.assertEqual(["recoil:vc5-target:unit"], details["added"])
        self.assertEqual([], details["invalidated"]["block_ids"])
        self.assertEqual(["recoil:vc5-target:unit"], details["source_policy_enforced"])
        self.assertEqual([], details["source_policy_bootstrapped"])
        self.assertEqual(work_items_before, tracker["work_items"])

    def test_current_player_registration_includes_zinput_initializer_source(self) -> None:
        target_id = "recoil:vc5-target:player_41ea90_42de10_authored_order"
        initializer_source = "src/GameZRecoil/zInput/zin_init.cpp"
        registrations = load_target_registrations(
            functional_manifest_dir=TOOLS_ROOT / "functional_verify_targets",
            vc5_manifest_dir=TOOLS_ROOT / "vc5_verify_targets",
        )

        registration = registrations[target_id]["registration"]
        self.assertEqual(
            1,
            registration["order_edit_paths"].count(initializer_source),
        )

    def test_current_player_registration_retires_handwritten_zinput_lifecycles(
        self,
    ) -> None:
        relative_manifest = (
            "tools/vc5_verify_targets/"
            "player_41ea90_42de10_authored_order.json"
        )
        manifest = json.loads((REPO_ROOT / relative_manifest).read_text(encoding="utf-8"))
        retired_addresses = {
            "0x429f10",
            "0x429f20",
            "0x429f40",
            "0x429f50",
        }
        expected_neighbor_seam = [
            "0x429ed0",
            "0x429ef0",
            "0x429f80",
            "0x42a000",
            "0x42a070",
        ]

        for array_name in (
            "linked_function_intervals",
            "translation_unit_function_order",
        ):
            addresses = [
                row["address"] for row in manifest[array_name][0]["functions"]
            ]
            self.assertTrue(retired_addresses.isdisjoint(addresses))
            seam_start = addresses.index(expected_neighbor_seam[0])
            self.assertEqual(
                expected_neighbor_seam,
                addresses[seam_start : seam_start + len(expected_neighbor_seam)],
            )

        registrations = load_target_registrations(
            functional_manifest_dir=TOOLS_ROOT / "functional_verify_targets",
            vc5_manifest_dir=TOOLS_ROOT / "vc5_verify_targets",
        )
        self.assertNotIn(
            "recoil:vc5-target:zinput_bind_group_static_lifetime",
            registrations,
        )
        self.assertNotIn(
            "recoil:vc5-target:"
            "zinput_bindgroup_list_static_init_and_register_at_exit",
            registrations,
        )

    @staticmethod
    def _accepted_target_sync_invalidation_fixture(
        *, registration_changed: bool,
    ) -> tuple[str, str, dict[str, object], dict[str, object]]:
        target_id = "recoil:vc5-target:unit-order"
        target_name = "unit-order"
        registration = {
            "binary": "recoil",
            "check_function_order": True,
            "function_order_scope": "authored",
            "function_addresses": ["0x401000"],
            "functions": [{"address": "0x401000"}],
            "manifest_path": "tools/vc5_verify_targets/unit-order.json",
            "name": target_name,
            "order_edit_paths": ["src/Battlesport/unit.cpp"],
            "source_from": "src/Battlesport/unit.cpp",
            "translation_unit_function_order": [],
        }
        current_record = {
            "binary": "recoil",
            "kind": "vc5",
            "name": target_name,
            "registration": deepcopy(registration),
            "registered_addresses": ["0x401000"],
        }
        loaded_record = deepcopy(current_record)
        if registration_changed:
            loaded_record["registration"]["order_edit_paths"] = [
                "src/Battlesport/unit.cpp",
                "src/Battlesport/unit.h",
            ]

        accepted = {
            "authored": {
                name: state_record(
                    result="passed",
                    disposition="accepted",
                    freshness="current",
                    evidence_ids=["recoil:evidence:order"],
                )
                for name in AUTHORED_ORDER_DIMENSIONS
            },
            "full": {
                name: state_record(
                    result="passed",
                    disposition="accepted",
                    freshness="current",
                    evidence_ids=["recoil:evidence:order"],
                )
                for name in FULL_ORDER_DIMENSIONS
            },
        }

        def block(accepted_facts: dict[str, str]) -> dict[str, object]:
            return {
                "order_targets": {},
                "order": deepcopy(accepted),
                "accepted_order_facts": accepted_facts,
                "unrelated_block_metadata": "preserve",
            }

        symbol_state = {
            "object_byte": state_record(
                result="passed",
                disposition="accepted",
                freshness="current",
                evidence_ids=["recoil:evidence:byte"],
            )
        }
        tracker = {
            "verification_targets": {target_id: current_record},
            "symbols": {
                "recoil:function:0x401000": {
                    "address": "0x401000",
                    "verification_target_ids": [target_id],
                    "binary_state": symbol_state,
                    "accepted_byte_facts": {"target_id": target_id},
                }
            },
            "physical_blocks": {
                "recoil:block:exact-id": block({"target_id": target_id}),
                "recoil:block:name-in-target-id": block(
                    {"target_id": target_name}
                ),
                "recoil:block:target-name": block(
                    {"target_name": target_name}
                ),
                "recoil:block:unrelated": block(
                    {"target_id": "recoil:vc5-target:unrelated"}
                ),
            },
            "owners": {"recoil:owner:unit": {"tier": "A"}},
            "work_items": {"unit:work": {"state": "ready"}},
        }
        return target_id, target_name, {target_id: loaded_record}, tracker

    def test_changed_registration_invalidates_accepted_target_facts_without_order_targets(
        self,
    ) -> None:
        target_id, target_name, loaded, tracker = (
            self._accepted_target_sync_invalidation_fixture(
                registration_changed=False
            )
        )
        loaded[target_id]["registration"]["function_addresses"] = [
            "0x401000",
            "0x401010",
        ]
        symbols_before = deepcopy(tracker["symbols"])
        owners_before = deepcopy(tracker["owners"])
        work_items_before = deepcopy(tracker["work_items"])
        unrelated_before = deepcopy(
            tracker["physical_blocks"]["recoil:block:unrelated"]
        )
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("_recoil.commands.progress_cli.load_vc5_manifest"),
        ):
            details = _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=[target_id],
            )

        affected = [
            "recoil:block:exact-id",
            "recoil:block:name-in-target-id",
            "recoil:block:target-name",
        ]
        self.assertEqual([target_id], details["updated"])
        self.assertEqual(affected, details["invalidated"]["block_ids"])
        self.assertEqual([], details["invalidated"]["symbol_ids"])
        self.assertEqual([], details["revalidated_accepted_order_target_ids"])
        self.assertEqual([], details["revalidated_accepted_order_block_ids"])
        for block_id in affected:
            block = tracker["physical_blocks"][block_id]
            self.assertEqual({}, block["order_targets"])
            self.assertNotIn("accepted_order_facts", block)
            self.assertEqual("preserve", block["unrelated_block_metadata"])
            for family in block["order"].values():
                for state in family.values():
                    self.assertEqual("pending", state["result"])
                    self.assertEqual("changed", state["freshness"])
                    self.assertEqual([], state["evidence_ids"])
        self.assertEqual(
            loaded[target_id], tracker["verification_targets"][target_id]
        )
        self.assertEqual(unrelated_before, tracker["physical_blocks"]["recoil:block:unrelated"])
        self.assertEqual(symbols_before, tracker["symbols"])
        self.assertEqual(owners_before, tracker["owners"])
        self.assertEqual(work_items_before, tracker["work_items"])
        self.assertEqual(target_name, tracker["verification_targets"][target_id]["name"])

    def test_changed_registered_addresses_invalidates_accepted_target_facts(
        self,
    ) -> None:
        target_id, _target_name, loaded, tracker = (
            self._accepted_target_sync_invalidation_fixture(
                registration_changed=False
            )
        )
        loaded[target_id]["registered_addresses"] = ["0x401000", "0x401010"]

        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("_recoil.commands.progress_cli.load_vc5_manifest"),
        ):
            details = _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=[target_id],
            )

        self.assertEqual([target_id], details["updated"])
        self.assertEqual(
            [
                "recoil:block:exact-id",
                "recoil:block:name-in-target-id",
                "recoil:block:target-name",
            ],
            details["invalidated"]["block_ids"],
        )

    def test_order_edit_path_only_sync_preserves_accepted_order_and_call_contract(
        self,
    ) -> None:
        target_id, _target_name, loaded, tracker = (
            self._accepted_target_sync_invalidation_fixture(
                registration_changed=True
            )
        )
        symbol = tracker["symbols"]["recoil:function:0x401000"]
        symbol.update(
            {
                "binary": "recoil",
                "kind": "function",
                "pipeline_class": "authored",
                "authored_order_role": "authored-body",
                "physical_block_id": "recoil:block:exact-id",
                "address": "0x401000",
                "end_exclusive": "0x401010",
            }
        )
        exact_block = tracker["physical_blocks"]["recoil:block:exact-id"]
        for state in exact_block["order"]["authored"].values():
            state["validation_mode"] = "live"
        exact_block["accepted_order_facts"].update(
            {
                "phase": "authored-function-order",
                "matched_identities": ["recoil:function:0x401000"],
            }
        )
        exact_block["contribution_ids"] = ["recoil:function:0x401000"]
        accepted_order_before = deepcopy(tracker["physical_blocks"])
        symbols_before = deepcopy(tracker["symbols"])

        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("_recoil.commands.progress_cli.load_vc5_manifest"),
        ):
            details = _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=[target_id],
            )

        self.assertEqual([target_id], details["updated"])
        self.assertEqual([], details["invalidated"]["block_ids"])
        self.assertEqual(accepted_order_before, tracker["physical_blocks"])
        self.assertEqual(symbols_before, tracker["symbols"])
        self.assertEqual(
            ["src/Battlesport/unit.cpp", "src/Battlesport/unit.h"],
            tracker["verification_targets"][target_id]["registration"][
                "order_edit_paths"
            ],
        )
        slices = ProgressDocument(tracker).authored_call_contract_slices()
        self.assertEqual(
            ["recoil:function:0x401000"],
            slices[0]["symbol_ids"],
        )

    def test_changed_registration_rejects_malformed_dependency_before_mutation(
        self,
    ) -> None:
        target_id, _target_name, loaded, tracker = (
            self._accepted_target_sync_invalidation_fixture(
                registration_changed=True
            )
        )
        tracker["physical_blocks"]["recoil:block:unrelated"][
            "accepted_order_facts"
        ] = {"target_id": [target_id]}
        before = deepcopy(tracker)
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("_recoil.commands.progress_cli.load_vc5_manifest"),
            self.assertRaisesRegex(
                ProgressError,
                "invalid accepted_order_facts.target_id",
            ),
        ):
            _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=[target_id],
            )
        self.assertEqual(before, tracker)

    def test_explicit_revalidation_recovers_already_current_accepted_target(
        self,
    ) -> None:
        target_id, _target_name, loaded, tracker = (
            self._accepted_target_sync_invalidation_fixture(
                registration_changed=False
            )
        )
        target_before = deepcopy(tracker["verification_targets"][target_id])
        symbols_before = deepcopy(tracker["symbols"])
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("_recoil.commands.progress_cli.load_vc5_manifest"),
        ):
            details = _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=[target_id],
                revalidate_accepted_order=True,
            )

        affected = [
            "recoil:block:exact-id",
            "recoil:block:name-in-target-id",
            "recoil:block:target-name",
        ]
        self.assertEqual([target_id], details["unchanged"])
        self.assertEqual([], details["updated"])
        self.assertEqual(affected, details["invalidated"]["block_ids"])
        self.assertEqual(
            [target_id], details["revalidated_accepted_order_target_ids"]
        )
        self.assertEqual(
            affected, details["revalidated_accepted_order_block_ids"]
        )
        self.assertEqual(target_before, tracker["verification_targets"][target_id])
        self.assertEqual(symbols_before, tracker["symbols"])

    def test_explicit_revalidation_fails_closed_without_exact_accepted_facts(
        self,
    ) -> None:
        target_id, _target_name, loaded, tracker = (
            self._accepted_target_sync_invalidation_fixture(
                registration_changed=False
            )
        )
        for block in tracker["physical_blocks"].values():
            block["accepted_order_facts"] = {
                "target_id": "recoil:vc5-target:unrelated"
            }
        before = deepcopy(tracker)
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("_recoil.commands.progress_cli.load_vc5_manifest"),
            self.assertRaisesRegex(
                ProgressError,
                "found no exact accepted order facts",
            ),
        ):
            _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=[target_id],
                revalidate_accepted_order=True,
            )
        self.assertEqual(before, tracker)

    def test_explicit_revalidation_cli_requires_the_opt_in_flag(self) -> None:
        args = progress_cli._parser().parse_args(
            [
                "verification-target",
                "sync",
                "--target",
                "recoil:vc5-target:unit-order",
                "--revalidate-accepted-order",
                "--expected-revision",
                "12",
                "--dry-run",
            ]
        )
        self.assertTrue(args.revalidate_accepted_order)

    def test_changed_registration_regresses_pipeline_and_routes_fresh_order_claim(
        self,
    ) -> None:
        game_blocks = (
            "recoil:block:0x4b2960",
            "recoil:block:0x4b31f0",
        )
        all_blocks = (
            "recoil:block:0x4b2960",
            "recoil:block:0x4b31c0",
            "recoil:block:0x4b31f0",
            "recoil:block:0x4b33f0",
        )
        tracker = zgame_zsys_multi_slice_fixture(
            accepted_authored_blocks=all_blocks
        )
        for block_id in game_blocks:
            tracker["physical_blocks"][block_id]["order_targets"] = {}
        symbol_id = tracker["physical_blocks"][game_blocks[0]][
            "contribution_ids"
        ][0]
        tracker["symbols"][symbol_id]["binary_state"] = {
            "object_byte": state_record(
                result="passed",
                disposition="accepted",
                freshness="current",
                evidence_ids=["recoil:evidence:byte"],
            )
        }
        tracker["symbols"][symbol_id]["accepted_byte_facts"] = {
            "sentinel": "preserve-independent-byte-state"
        }
        byte_state_before = deepcopy(
            tracker["symbols"][symbol_id]["binary_state"]
        )
        byte_facts_before = deepcopy(
            tracker["symbols"][symbol_id]["accepted_byte_facts"]
        )
        pipeline_before = ProgressDocument(tracker).pipeline(
            "recoil", resolve_order_target=False
        )
        self.assertEqual("full-function-order", pipeline_before["phase"])

        loaded = {
            ZGAME_MULTI_SLICE_ID: deepcopy(
                tracker["verification_targets"][ZGAME_MULTI_SLICE_ID]
            )
        }
        loaded[ZGAME_MULTI_SLICE_ID]["sync_contract_revision"] = 2
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("_recoil.commands.progress_cli.load_vc5_manifest"),
        ):
            details = _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=[ZGAME_MULTI_SLICE_ID],
            )

        self.assertEqual(list(game_blocks), details["invalidated"]["block_ids"])
        self.assertEqual([], details["invalidated"]["symbol_ids"])
        document = ProgressDocument(tracker)
        pipeline_after = document.pipeline("recoil")
        self.assertEqual("authored-function-order", pipeline_after["phase"])
        self.assertEqual(game_blocks[0], pipeline_after["physical_block_id"])
        self.assertEqual(
            {
                "status": "ready",
                "target_id": ZGAME_MULTI_SLICE_ID,
                "covered_block_ids": [game_blocks[0]],
                "override_option": "--object-target",
                "override_selector": ZGAME_MULTI_SLICE_ID,
            },
            {
                key: pipeline_after["order_target_resolution"][key]
                for key in (
                    "status",
                    "target_id",
                    "covered_block_ids",
                    "override_option",
                    "override_selector",
                )
            },
        )
        _work_id, packet = _order_claim_candidate(document, pipeline_after)
        self.assertEqual("order-edit-v1", packet["packet_type"])
        self.assertEqual(ZGAME_MULTI_SLICE_ID, packet["target_id"])
        self.assertEqual([game_blocks[0]], packet["covered_block_ids"])
        self.assertEqual(byte_state_before, tracker["symbols"][symbol_id]["binary_state"])
        self.assertEqual(
            byte_facts_before,
            tracker["symbols"][symbol_id]["accepted_byte_facts"],
        )

        with (
            patch.object(
                progress_cli,
                "_call_contract_convergence_repair_candidates",
                side_effect=AssertionError(
                    "authored-order scheduling consulted the invalid call-contract census"
                ),
            ),
            patch.object(
                progress_cli,
                "_call_contract_retail_fact_candidates",
                side_effect=AssertionError(
                    "authored-order scheduling consulted call-contract retail packets"
                ),
            ),
        ):
            next_work = document.next_work("recoil")
        self.assertEqual("authored-function-order", next_work["phase"])
        self.assertEqual(game_blocks[0], next_work["physical_block_id"])
        self.assertEqual("0x4b2960", next_work["cursor"])
        self.assertEqual(
            "order",
            next_work["primary_lane"],
        )

        tracker["binaries"]["recoil"]["source_layout_context"] = {
            "provenance_status_summary": {
                "remaining_blocker": (
                    "Dynamic scheduler state: consult generated_current."
                )
            }
        }
        from _recoil.commands.current_metadata_audit import (
            refresh_remaining_blocker_metadata,
        )

        with patch.object(
            progress_cli,
            "_call_contract_convergence_repair_candidates",
            side_effect=AssertionError(
                "metadata refresh consulted the invalid call-contract census"
            ),
        ):
            refreshed = refresh_remaining_blocker_metadata(tracker)
        self.assertEqual(
            "authored-function-order",
            refreshed["generated_current"]["phase"],
        )
        self.assertEqual("0x4b2960", refreshed["generated_current"]["cursor"])

    def test_source_policy_bootstrap_registers_one_camera_style_cross_file_closure(self) -> None:
        target_id = "recoil:vc5-target:camera-order"
        source_from = "src/GameZRecoil/zClass/Camera.c"
        donor_source = "src/GameZRecoil/zClass/Sound.c"
        registration = {
            "binary": "recoil",
            "check_translation_unit_function_order": True,
            "function_order_scope": "authored",
            "manifest_path": "tools/vc5_verify_targets/camera-order.json",
            "name": "camera-order",
            "source_from": source_from,
            "translation_unit_function_order": [
                {
                    "source_from": source_from,
                    "order_scope": "authored",
                    "inventory_only": False,
                    "functions": [
                        {
                            "address": "0x449d10",
                            "pipeline_class": "authored",
                            "authored_order_role": "authored-body",
                        }
                    ],
                }
            ],
            "linked_function_intervals": [],
            "order_edit_paths": [source_from, donor_source],
        }
        loaded = {
            target_id: {
                "binary": "recoil",
                "kind": "vc5",
                "name": "camera-order",
                "registration": registration,
                "registered_addresses": ["0x449d10"],
            }
        }
        manifest_data = {
            "retail_start": "0x449ba0",
            "retail_end_exclusive": "0x44d990",
            "order_edit_paths": [source_from, donor_source],
        }
        tracker = {"verification_targets": {}, "symbols": {}, "physical_blocks": {}}
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("pathlib.Path.read_text", return_value=json.dumps(manifest_data)),
            patch(
                "_recoil.commands.progress_cli.load_vc5_manifest",
                side_effect=ValueError(
                    "Camera.c does not contain provenance docblock/comment "
                    "'Reimplements 0x449d10:'"
                ),
            ),
        ):
            details = _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=["camera-order"],
                source_policy_bootstrap=True,
            )

        stored = tracker["verification_targets"][target_id]["registration"]
        self.assertEqual(
            {
                "state": "pending-source-placement",
                "registration_only": True,
                "order_scopes": ["authored"],
                "retail_start": "0x449ba0",
                "retail_end_exclusive": "0x44d990",
                "writable_closure": [source_from, donor_source],
            },
            stored["source_policy_bootstrap"],
        )
        self.assertEqual([target_id], details["source_policy_bootstrapped"])
        self.assertEqual([], details["source_policy_enforced"])

    def test_source_policy_bootstrap_registers_same_host_provider_anchor_only(self) -> None:
        target_id, source_from, loaded, manifest_data, target = (
            self._same_host_bootstrap_fixture()
        )
        tracker = {"verification_targets": {}, "symbols": {}, "physical_blocks": {}}
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("pathlib.Path.read_text", return_value=json.dumps(manifest_data)),
            patch(
                "_recoil.commands.progress_cli.load_vc5_manifest",
                side_effect=[
                    ValueError(
                        f"{source_from} does not contain provider-boundary provenance "
                        "docblock/comment for 0x48bf10"
                    ),
                    target,
                ],
            ),
            patch(
                "_recoil.commands.progress_cli.source_from_policy_text",
                return_value="/* Reimplements 0x489d00: authored body. */",
            ),
            patch(
                "_recoil.commands.progress_cli._direct_compile_host_policy_text",
                return_value=(
                    "/**\n"
                    " * @recoil-anchor recoil:anchor:znetwork-authored-body\n"
                    " * @recoil-artifact defines .text recoil:function:0x489d00: authored body.\n"
                    " */\n"
                    "int AuthoredBody() { return 0; }\n"
                ),
            ),
        ):
            details = _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=["znetwork-order"],
                source_policy_bootstrap=True,
            )

        bootstrap = tracker["verification_targets"][target_id]["registration"][
            "source_policy_bootstrap"
        ]
        self.assertEqual([source_from], bootstrap["writable_closure"])
        self.assertEqual(
            "same-host-nondefining-provenance-anchor",
            bootstrap["bootstrap_reason"],
        )
        self.assertEqual(
            [{"address": "0x48bf10", "provenance": "provider-boundary"}],
            bootstrap["pending_nondefining_provenance_anchors"],
        )
        self.assertEqual([target_id], details["source_policy_bootstrapped"])

    def test_same_host_provider_bootstrap_rejects_missing_authored_definition(self) -> None:
        _target_id, source_from, loaded, manifest_data, target = (
            self._same_host_bootstrap_fixture()
        )
        tracker = {"verification_targets": {}, "symbols": {}, "physical_blocks": {}}
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("pathlib.Path.read_text", return_value=json.dumps(manifest_data)),
            patch(
                "_recoil.commands.progress_cli.load_vc5_manifest",
                side_effect=[
                    ValueError(
                        f"{source_from} does not contain provider-boundary provenance "
                        "docblock/comment for 0x48bf10"
                    ),
                    target,
                ],
            ),
            patch(
                "_recoil.commands.progress_cli.source_from_policy_text",
                return_value="",
            ),
            patch(
                "_recoil.commands.progress_cli._direct_compile_host_policy_text",
                return_value="/* Reimplements 0x489d00: legacy inventory only. */",
            ),
            self.assertRaisesRegex(
                ProgressError,
                "cannot relocate or supply missing authored definition 0x489d00",
            ),
        ):
            _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=["znetwork-order"],
                source_policy_bootstrap=True,
            )

    def test_same_host_provider_bootstrap_rejects_authored_stand_in(self) -> None:
        _target_id, source_from, loaded, manifest_data, target = (
            self._same_host_bootstrap_fixture(
                provider_pipeline_class="authored",
                provider_role="authored-body",
            )
        )
        tracker = {"verification_targets": {}, "symbols": {}, "physical_blocks": {}}
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("pathlib.Path.read_text", return_value=json.dumps(manifest_data)),
            patch(
                "_recoil.commands.progress_cli.load_vc5_manifest",
                side_effect=[
                    ValueError(
                        f"{source_from} does not contain provider-boundary provenance "
                        "docblock/comment for 0x48bf10"
                    ),
                    target,
                ],
            ),
            patch(
                "_recoil.commands.progress_cli.source_from_policy_text",
                return_value="/* Reimplements 0x489d00: authored body. */",
            ),
            patch(
                "_recoil.commands.progress_cli._direct_compile_host_policy_text",
                return_value=(
                    "/**\n"
                    " * @recoil-anchor recoil:anchor:znetwork-authored-body\n"
                    " * @recoil-artifact defines .text recoil:function:0x489d00: authored body.\n"
                    " */\n"
                    "int AuthoredBody() { return 0; }\n"
                ),
            ),
            self.assertRaisesRegex(ProgressError, "refuses authored stand-in provenance"),
        ):
            _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=["znetwork-order"],
                source_policy_bootstrap=True,
            )

    def test_source_policy_bootstrap_rejects_missing_exact_writable_closure(self) -> None:
        target_id = "recoil:vc5-target:camera-order"
        source_from = "src/GameZRecoil/zClass/Camera.c"
        loaded = {
            target_id: {
                "binary": "recoil",
                "kind": "vc5",
                "name": "camera-order",
                "registration": {
                    "binary": "recoil",
                    "check_translation_unit_function_order": True,
                    "manifest_path": "tools/vc5_verify_targets/camera-order.json",
                    "name": "camera-order",
                    "source_from": source_from,
                    "translation_unit_function_order": [
                        {
                            "source_from": source_from,
                            "order_scope": "authored",
                            "functions": [
                                {
                                    "address": "0x449d10",
                                    "pipeline_class": "authored",
                                    "authored_order_role": "authored-body",
                                }
                            ],
                        }
                    ],
                    "linked_function_intervals": [],
                },
                "registered_addresses": ["0x449d10"],
            }
        }
        manifest_data = {
            "retail_start": "0x449ba0",
            "retail_end_exclusive": "0x44d990",
        }
        tracker = {"verification_targets": {}, "symbols": {}, "physical_blocks": {}}
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("pathlib.Path.read_text", return_value=json.dumps(manifest_data)),
            self.assertRaisesRegex(ProgressError, "requires exact order_edit_paths"),
        ):
            _sync_verification_targets(
                tracker,
                binary="recoil",
                selectors=["camera-order"],
                source_policy_bootstrap=True,
            )

    def test_ordinary_sync_keeps_final_source_policy_mandatory(self) -> None:
        target_id = "recoil:vc5-target:unit"
        loaded = {
            target_id: {
                "binary": "recoil",
                "kind": "vc5",
                "name": "unit",
                "registration": {
                    "binary": "recoil",
                    "manifest_path": "tools/vc5_verify_targets/unit.json",
                    "name": "unit",
                },
                "registered_addresses": ["0x401000"],
            }
        }
        tracker = {"verification_targets": {}, "symbols": {}, "physical_blocks": {}}
        with (
            patch(
                "_recoil.lib.verification_targets.load_target_registrations",
                return_value=loaded,
            ),
            patch("pathlib.Path.read_text", return_value="{}"),
            patch(
                "_recoil.commands.progress_cli.load_vc5_manifest",
                side_effect=ValueError("missing final source provenance"),
            ),
            self.assertRaisesRegex(ValueError, "missing final source provenance"),
        ):
            _sync_verification_targets(tracker, binary="recoil", selectors=["unit"])


    def test_primary_order_packet_merges_order_edit_paths_into_writable_claims_only(self) -> None:
        registration = {
            "name": "unit-order",
            "manifest_path": "tools/vc5_verify_targets/unit-order.json",
            "source_from": "src/Battlesport/ai_net.cpp",
            "order_edit_paths": [
                "src/Battlesport/ai_net.cpp",
                "src/Battlesport/ai_net.h",
                "src/Battlesport/hud.h",
            ],
            "source_policy_bootstrap": {
                "state": "pending-source-placement",
                "registration_only": True,
                "writable_closure": [
                    "src/Battlesport/ai_net.cpp",
                    "src/Battlesport/ai_net.h",
                    "src/Battlesport/hud.h",
                ],
            },
            "translation_unit_function_order": [],
        }
        target_id = "recoil:vc5-target:unit-order"
        target = {"name": "unit-order", "registration": registration}
        block_id = "recoil:block:0x401000"
        collections = {
            "work_items": {},
            "physical_blocks": {
                block_id: {
                    "agent_source_path": "src/Battlesport/ai_net.cpp",
                    "source_shape_inputs": [{"path": "src/Battlesport/hud.cpp"}],
                }
            },
        }
        document = SimpleNamespace(
            revision=12,
            collection=lambda name: collections[name],
            _fresh_root=lambda *args: "build/live-validation/worker-order/unit-r13",
        )
        contract = {
            "target_id": target_id,
            "target": target,
            "phase": "authored-function-order",
            "cursor_block_id": block_id,
            "covered_block_ids": [block_id],
        }
        pipeline = {
            "cursor": "0x401000",
            "order_target_resolution": {"status": "ready", "target_id": target_id},
        }
        with patch(
            "_recoil.commands.progress_cli._current_order_contract",
            return_value=contract,
        ):
            _work_id, work = _order_claim_candidate(document, pipeline)

        expected_paths = [
            "src/Battlesport/ai_net.cpp",
            "src/Battlesport/ai_net.h",
            "src/Battlesport/hud.cpp",
            "src/Battlesport/hud.h",
        ]
        self.assertEqual(expected_paths, work["allowed_paths"])
        self.assertEqual(
            expected_paths,
            sorted(
                claim["id"]
                for claim in work["resource_claims"]
                if claim["kind"] == "path" and claim["access"] == "write"
            ),
        )
        self.assertEqual(
            "pending-source-placement",
            work["source_policy_bootstrap"]["state"],
        )
        self.assertEqual(
            [
                "python tools/recoil.py verify vc5-order unit-order "
                "--build-root build/live-validation/worker-order/unit-r13"
            ],
            work["validation_commands"],
        )
        byte_sources, _read_paths = _registration_paths(registration)
        self.assertEqual({"src/Battlesport/ai_net.cpp"}, byte_sources)

    def test_primary_order_packet_casefolds_tracker_alias_to_registered_path(self) -> None:
        canonical_source = "src/GameZRecoil/zInput/zInput.cpp"
        registration = {
            "name": "zinput-keyboard-order",
            "manifest_path": "tools/vc5_verify_targets/zinput-keyboard-order.json",
            "source_from": canonical_source,
            "order_edit_paths": [canonical_source],
            "translation_unit_function_order": [],
        }
        target_id = "recoil:vc5-target:zinput-keyboard-order"
        block_id = "recoil:block:0x46f300"
        collections = {
            "work_items": {},
            "physical_blocks": {
                block_id: {
                    "agent_source_path": "src/GameZRecoil/zInput/zinput.cpp",
                    "source_shape_inputs": [],
                }
            },
        }
        document = SimpleNamespace(
            revision=1068,
            collection=lambda name: collections[name],
            _fresh_root=lambda *args: (
                "build/live-validation/worker-order/zinput-keyboard-r1069"
            ),
        )
        contract = {
            "target_id": target_id,
            "target": {"name": "zinput-keyboard-order", "registration": registration},
            "phase": "authored-function-order",
            "cursor_block_id": block_id,
            "covered_block_ids": [block_id],
        }
        pipeline = {
            "cursor": "0x46f300",
            "order_target_resolution": {"status": "ready", "target_id": target_id},
        }

        with patch(
            "_recoil.commands.progress_cli._current_order_contract",
            return_value=contract,
        ):
            _work_id, work = _order_claim_candidate(document, pipeline)

        self.assertEqual([canonical_source], work["allowed_paths"])
        self.assertEqual(
            [canonical_source],
            [
                claim["id"]
                for claim in work["resource_claims"]
                if claim["kind"] == "path" and claim["access"] == "write"
            ],
        )

    def test_primary_order_packet_revalidates_stale_invalid_registered_paths(self) -> None:
        registration = {
            "name": "unit-order",
            "manifest_path": "tools/vc5_verify_targets/unit-order.json",
            "source_from": "src/Battlesport/ai_net.cpp",
            "order_edit_paths": ["../outside.h"],
            "translation_unit_function_order": [],
        }
        target_id = "recoil:vc5-target:unit-order"
        block_id = "recoil:block:0x401000"
        document = SimpleNamespace(
            revision=12,
            collection=lambda name: (
                {} if name == "work_items" else {block_id: {"agent_source_path": "sample.cpp"}}
            ),
            _fresh_root=lambda *args: "build/live-validation/worker-order/unit-r13",
        )
        contract = {
            "target_id": target_id,
            "target": {"name": "unit-order", "registration": registration},
            "phase": "authored-function-order",
            "cursor_block_id": block_id,
            "covered_block_ids": [block_id],
        }
        pipeline = {
            "cursor": "0x401000",
            "order_target_resolution": {"status": "ready", "target_id": target_id},
        }
        with (
            patch(
                "_recoil.commands.progress_cli._current_order_contract",
                return_value=contract,
            ),
            self.assertRaisesRegex(ValueError, "not normalized"),
        ):
            _order_claim_candidate(document, pipeline)

    @staticmethod
    def _call_contract_claim_fixture(
        registrations: dict[str, dict[str, object]],
    ) -> tuple[SimpleNamespace, dict[str, object], dict[str, object]]:
        target_ids = list(registrations)
        slice_row: dict[str, object] = {
            "id": "recoil:call-contract-slice:0x403930-0x403b20",
            "start": "0x403930",
            "body_count": 2,
            "physical_block_ids": ["recoil:block:0x403930"],
            "symbol_ids": [
                "recoil:function:0x403930",
                "recoil:function:0x403b00",
            ],
            "target_ids": target_ids,
            "source_paths": [
                "src/Battlesport/Briefing.cpp",
                "src/Battlesport/about.cpp",
            ],
        }
        collections: dict[str, object] = {
            "work_items": {},
            "verification_targets": {
                target_id: {
                    "kind": "vc5",
                    "registration": registration,
                }
                for target_id, registration in registrations.items()
            },
        }
        document = SimpleNamespace(
            revision=1302,
            authored_call_contract_slices=lambda: [slice_row],
            collection=lambda name: collections[name],
            _fresh_root=lambda *args: (
                "build/live-validation/worker-call-contract/403930-r1302"
            ),
        )
        pipeline: dict[str, object] = {
            "authored_call_contract_slice_id": slice_row["id"],
        }
        return document, pipeline, slice_row

    def test_call_contract_packet_uses_shared_include_closure_as_writes(self) -> None:
        registrations = {
            "recoil:vc5-target:briefing": {
                "manifest_path": "tools/vc5_verify_targets/briefing.json",
                "order_edit_paths": [
                    "src/Battlesport/Briefing.cpp",
                    "src/Battlesport/Briefing.h",
                ],
            },
            "recoil:vc5-target:briefing-neighbor": {
                "manifest_path": "tools/vc5_verify_targets/briefing-neighbor.json",
                "order_edit_paths": [
                    "src/Battlesport/Briefing.h",
                    "src/Battlesport/ai_net.h",
                    "tools/_recoil/probes/cabout_pch/stdafx.h",
                ],
            },
        }
        document, pipeline, slice_row = self._call_contract_claim_fixture(
            registrations
        )

        expected = [
            "src/Battlesport/about.cpp",
            "src/Battlesport/Briefing.cpp",
            "src/Battlesport/Briefing.h",
            "src/Battlesport/about.h",
        ]
        dependency_paths = [
            *expected,
            "src/Battlesport/transitive_only.h",
            "src/Battlesport/definition_only.cpp",
            "tools/_recoil/config/vc5_final_build.json",
            "tools/vc5_verify_targets/briefing-neighbor.json",
            "tools/vc5_verify_targets/briefing.json",
        ]
        source_closure = SimpleNamespace(
            source_edit_paths=tuple(expected),
            definition_source_paths=(
                "src/Battlesport/definition_only.cpp",
            ),
            dependency_paths=tuple(dependency_paths),
            definition_resolution={
                "kind": "call-contract-definition-source-resolution",
                "contract_version": 2,
                "mode": "conservative-full-closure",
            },
        )
        with (
            patch(
                "_recoil.commands.progress_cli.call_contract_source_write_paths",
                return_value=expected,
            ) as closure,
            patch(
                "_recoil.commands.progress_cli.call_contract_source_closure",
                return_value=source_closure,
            ) as source_closure_call,
        ):
            self.assertEqual(
                expected,
                _call_contract_slice_write_paths(document, slice_row),
            )
            _work_id, work = _call_contract_claim_candidate(
                document,
                pipeline,
                progress_path=Path(".agent/RECONSTRUCTION_PROGRESS.json"),
            )

        self.assertEqual(1, closure.call_count)
        source_closure_call.assert_called_once_with(document, slice_row)

        self.assertEqual(expected, work["allowed_paths"])
        self.assertIn("--progress {progress_path}", work["validation_commands"][0])
        self.assertNotIn(
            "--progress .agent/", work["validation_commands"][0].replace("\\", "/")
        )
        write_paths = sorted(
            row["id"]
            for row in work["resource_claims"]
            if row["kind"] == "path" and row["access"] == "write"
        )
        self.assertEqual(sorted(expected), write_paths)
        self.assertEqual(1, write_paths.count("src/Battlesport/Briefing.h"))
        read_paths = {
            row["id"]
            for row in work["resource_claims"]
            if row["kind"] == "path" and row["access"] == "read"
        }
        self.assertIn("src/Battlesport/transitive_only.h", read_paths)
        self.assertIn("src/Battlesport/definition_only.cpp", read_paths)
        self.assertNotIn("src/Battlesport/transitive_only.h", write_paths)
        self.assertNotIn("src/Battlesport/definition_only.cpp", write_paths)
        manifest_paths = {
            "tools/vc5_verify_targets/briefing.json",
            "tools/vc5_verify_targets/briefing-neighbor.json",
        }
        self.assertTrue(
            manifest_paths.issubset(
                {
                    row["id"]
                    for row in work["resource_claims"]
                    if row["kind"] == "path" and row["access"] == "read"
                }
            )
        )
        self.assertTrue(manifest_paths.isdisjoint(write_paths))

    def test_current_player_call_contract_packet_writes_zinput_initializer_source(
        self,
    ) -> None:
        target_id = "recoil:vc5-target:player_41ea90_42de10_authored_order"
        relative_manifest = (
            "tools/vc5_verify_targets/"
            "player_41ea90_42de10_authored_order.json"
        )
        initializer_source = "src/GameZRecoil/zInput/zin_init.cpp"
        manifest = json.loads((REPO_ROOT / relative_manifest).read_text(encoding="utf-8"))
        registrations = {
            target_id: {
                "manifest_path": relative_manifest,
                "order_edit_paths": manifest["order_edit_paths"],
            }
        }
        document, pipeline, slice_row = self._call_contract_claim_fixture(
            registrations
        )
        slice_row["source_paths"] = ["src/Battlesport/player.cpp"]

        work_id, work = _call_contract_claim_candidate(
            document,
            pipeline,
            progress_path=Path(".agent/RECONSTRUCTION_PROGRESS.json"),
        )
        work["state"] = "active"
        work["reservation"] = {
            "id": "recoil:reservation:player-call-contract",
            "state": "active",
        }
        work["packet_contract_version"] = 4
        work["progress_packet_adapter"] = "native-git-v1-planned"

        self.assertEqual(1, work["allowed_paths"].count(initializer_source))
        self.assertEqual([target_id], work["target_ids"])
        with self.assertRaisesRegex(
            ProgressError, "planned native-git-v1 allocation.*not handoff-visible"
        ):
            progress_cli._compact_reserved_packet(work_id, work)

    def test_current_mission_map_closure_keeps_definition_sources_read_only(
        self,
    ) -> None:
        target_specs = (
            (
                "recoil:vc5-target:map_text_block_order_current_shape",
                "tools/vc5_verify_targets/"
                "map_text_block_order_current_shape.json",
            ),
            (
                "recoil:vc5-target:mission_417350_41cc10_authored_order",
                "tools/vc5_verify_targets/"
                "mission_417350_41cc10_authored_order.json",
            ),
        )
        registrations: dict[str, dict[str, object]] = {}
        for target_id, relative_manifest in target_specs:
            manifest = json.loads(
                (REPO_ROOT / relative_manifest).read_text(encoding="utf-8")
            )
            registrations[target_id] = {
                "manifest_path": relative_manifest,
            }
            if manifest.get("order_edit_paths"):
                registrations[target_id]["order_edit_paths"] = manifest[
                    "order_edit_paths"
                ]
            else:
                registrations[target_id]["source_from"] = manifest["source_from"]
        document, _pipeline, slice_row = self._call_contract_claim_fixture(
            registrations
        )
        slice_row["source_paths"] = [
            "src/Battlesport/map.cpp",
            "src/Battlesport/mission.cpp",
        ]

        closure = call_contract_source_closure(document, slice_row)

        self.assertIn("src/GameZRecoil/zEffect/zeff.h", closure.header_paths)
        self.assertIn(
            "src/GameZRecoil/zEffect/zeff_anim_init.c",
            closure.definition_source_paths,
        )
        self.assertIn(
            "src/Battlesport/pickup.cpp", closure.definition_source_paths
        )
        self.assertIn(
            "src/GameZRecoil/zClass/cls_util.c",
            closure.definition_source_paths,
        )
        self.assertNotIn(
            "src/GameZRecoil/zEffect/zeff_anim_init.c",
            closure.source_edit_paths,
        )
        self.assertIn("src/Battlesport/map.cpp", closure.source_edit_paths)
        self.assertIn("src/Battlesport/mission.cpp", closure.source_edit_paths)

    def test_current_hud_target_handoffs_primary_headers_without_owner_broadening(
        self,
    ) -> None:
        manifest_path = (
            REPO_ROOT
            / "tools/vc5_verify_targets/hud_404ca0_415ab0_authored_order.json"
        )
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        hud_target_id = (
            "recoil:vc5-target:hud_404ca0_415ab0_authored_order"
        )
        unrelated_target_id = "recoil:vc5-target:unrelated-order"
        primary_headers = {
            "src/Battlesport/recoil_app.h",
            "src/GameZRecoil/zFMV/fmv.h",
        }
        unrelated_header = "src/Battlesport/ai_property_dlg.h"
        registrations = {
            hud_target_id: {
                "manifest_path": manifest_path.relative_to(REPO_ROOT).as_posix(),
                "order_edit_paths": manifest["order_edit_paths"],
            }
        }
        document, pipeline, slice_row = self._call_contract_claim_fixture(
            registrations
        )
        slice_row["source_paths"] = ["src/Battlesport/hud.cpp"]
        document.collection("verification_targets")[unrelated_target_id] = {
            "kind": "vc5",
            "registration": {
                "manifest_path": "tools/vc5_verify_targets/unrelated-order.json",
                "order_edit_paths": [unrelated_header],
            },
        }

        source_closure = call_contract_source_closure(document, slice_row)
        atomic_resolution = source_closure.definition_resolution[
            "source_atomic_header_definition_resolution"
        ]
        routes_by_header = {
            row["header_path"]: set(row["definition_source_paths"])
            for row in atomic_resolution["header_routes"]
        }
        self.assertIn(
            "src/Battlesport/RecoilApp.cpp",
            routes_by_header["src/Battlesport/recoil_app.h"],
        )
        self.assertIn(
            "src/GameZRecoil/zInput/zInput.cpp",
            routes_by_header["src/GameZRecoil/zInput/zInput.h"],
        )
        self.assertNotIn(
            "src/Battlesport/ai_net.cpp",
            atomic_resolution["definition_source_paths"],
        )

        work_id, work = _call_contract_claim_candidate(
            document,
            pipeline,
            progress_path=Path(".agent/RECONSTRUCTION_PROGRESS.json"),
        )
        work["state"] = "active"
        work["reservation"] = {
            "id": "recoil:reservation:hud-call-contract",
            "state": "active",
        }
        work["packet_contract_version"] = 4
        work["progress_packet_adapter"] = "native-git-v1-planned"

        for header_path in primary_headers:
            with self.subTest(header_path=header_path):
                self.assertEqual(1, work["allowed_paths"].count(header_path))
        self.assertEqual([hud_target_id], work["target_ids"])
        self.assertNotIn(unrelated_header, work["allowed_paths"])
        translation_unit_sources = {
            row["source_from"]
            for row in manifest["translation_unit_function_order"]
        }
        self.assertTrue(primary_headers.isdisjoint(manifest["source_files"]))
        self.assertTrue(primary_headers.isdisjoint(translation_unit_sources))

    def test_call_contract_packet_uses_current_manifest_when_historical_field_absent(
        self,
    ) -> None:
        registrations = {
            "recoil:vc5-target:briefing": {
                "manifest_path": (
                    "tools/vc5_verify_targets/"
                    "briefing_text_block_order_current_shape.json"
                ),
            }
        }
        document, _pipeline, slice_row = self._call_contract_claim_fixture(
            registrations
        )
        with patch(
            "_recoil.commands.call_contract_verify.load_manifest",
            return_value=SimpleNamespace(
                order_edit_paths=(
                    "src/Battlesport/Briefing.cpp",
                    "src/Battlesport/Briefing.h",
                )
            ),
        ) as loader:
            paths = _call_contract_slice_write_paths(document, slice_row)

        self.assertIn("src/Battlesport/Briefing.h", paths)
        loader.assert_called_once()
        self.assertEqual(
            REPO_ROOT
            / "tools/vc5_verify_targets/"
            "briefing_text_block_order_current_shape.json",
            loader.call_args.args[0],
        )
        self.assertIn("tracked_path_inventory", loader.call_args.kwargs)

    def test_call_contract_order_edit_path_state_fails_closed(self) -> None:
        cases = (
            (
                "wrong-type",
                {
                    "manifest_path": (
                        "tools/vc5_verify_targets/"
                        "briefing_text_block_order_current_shape.json"
                    ),
                    "order_edit_paths": "src/Battlesport/briefing.h",
                },
                "registration has malformed semantic paths",
            ),
            (
                "path-escape",
                {
                    "manifest_path": (
                        "tools/vc5_verify_targets/"
                        "briefing_text_block_order_current_shape.json"
                    ),
                    "order_edit_paths": ["../outside.h"],
                },
                "not normalized",
            ),
            (
                "manifest-is-not-edit-path",
                {
                    "manifest_path": (
                        "tools/vc5_verify_targets/"
                        "briefing_text_block_order_current_shape.json"
                    ),
                    "order_edit_paths": [
                        "tools/vc5_verify_targets/"
                        "briefing_text_block_order_current_shape.json"
                    ],
                },
                "manifest-registration-drift",
            ),
        )
        for label, registration, message in cases:
            with self.subTest(label=label):
                document, _pipeline, slice_row = (
                    self._call_contract_claim_fixture(
                        {"recoil:vc5-target:briefing": registration}
                    )
                )
                with self.assertRaisesRegex((ProgressError, ValueError), message):
                    _call_contract_slice_write_paths(document, slice_row)


def cabout_full_order_dual_target_fixture() -> dict[str, object]:
    from _recoil.lib.verification_targets import vc5_target_registration

    data = empty_progress_document()
    data["revision"] = 1380
    data["binaries"]["recoil"] = {
        "binary": "recoil",
        "primary_scheduler": True,
        "text": {"start": "0x401000", "end_exclusive": "0x401060"},
    }
    block_id = "recoil:block:0x401000"
    roles = {
        "0x401000": ("authored-lifecycle", "authored-lifecycle-body"),
        "0x401020": ("non-authored", "non-authored"),
        "0x401030": ("authored", "authored-body"),
        "0x401040": ("non-authored", "non-authored"),
        "0x401050": ("non-authored", "non-authored"),
    }
    contribution_ids = []
    ordered_addresses = list(roles)
    for index, address in enumerate(ordered_addresses):
        symbol_id = f"recoil:function:{address}"
        contribution_ids.append(symbol_id)
        pipeline_class, authored_role = roles[address]
        next_address = ordered_addresses[index + 1] if index + 1 < len(ordered_addresses) else "0x401060"
        data["symbols"][symbol_id] = {
            "address": address,
            "authored_order_role": authored_role,
            "binary": "recoil",
            "binary_state": {},
            "end_exclusive": next_address,
            "kind": "function",
            "physical_block_id": block_id,
            "pipeline_class": pipeline_class,
            "semantic_span_ids": [],
        }
    accepted_authored = {
        name: state_record(
            result="passed",
            disposition="accepted",
            freshness="current",
            evidence_ids=["recoil:evidence:fixture"],
            gating=True,
            validation_mode="live",
        )
        for name in AUTHORED_ORDER_DIMENSIONS
    }
    data["physical_blocks"][block_id] = {
        "agent_source_path": "src/Battlesport/about.cpp",
        "binary": "recoil",
        "contribution_ids": contribution_ids,
        "contribution_kind": "authored",
        "end_exclusive": "0x401060",
        "mapping": {"state": "mapped-no-literal-provisional", "status": "fixture"},
        "order": {
            "authored": accepted_authored,
            "full": {name: state_record() for name in FULL_ORDER_DIMENSIONS},
        },
        "order_targets": {
            "linked": "cabout_retail_interval_linked_order",
            "object": "cabout_prelude_provider_order_current_shape",
        },
        "source_path": "src/Battlesport/about.cpp",
        "source_shape_inputs": [{"path": "src/Battlesport/about.h", "role": "own-header"}],
        "start": "0x401000",
    }
    for name in (
        "cabout_retail_interval_linked_order",
        "cabout_prelude_provider_order_current_shape",
    ):
        target_id, target = vc5_target_registration(
            REPO_ROOT / "tools" / "vc5_verify_targets" / f"{name}.json"
        )
        data["verification_targets"][target_id] = target
    return data


class VerificationTargetRetirementTests(unittest.TestCase):
    VC5_ID = "recoil:vc5-target:zinput_bind_group_info_vec_count"
    FUNCTIONAL_ID = "recoil:functional-target:zinput_bind_group_info_vec_count"
    SHARED_NAME = "zinput_bind_group_info_vec_count"

    @classmethod
    def fixture(cls) -> dict[str, object]:
        accepted = {
            name: state_record(
                result="passed",
                disposition="accepted",
                freshness="current",
                evidence_ids=["recoil:evidence:r1:000001"],
            )
            for name in (
                "object_byte",
                "relocation_identity",
                "linked_presence",
                "linked_target_identity",
                "linked_body_byte",
                "call_contract",
                "linked_address",
                "linked_targets",
                "linked_byte",
            )
        }
        accepted_order = {
            "authored": {
                name: state_record(
                    result="passed",
                    disposition="accepted",
                    freshness="current",
                    evidence_ids=["recoil:evidence:r1:000001"],
                )
                for name in AUTHORED_ORDER_DIMENSIONS
            },
            "full": {
                name: state_record(
                    result="passed",
                    disposition="accepted",
                    freshness="current",
                    evidence_ids=["recoil:evidence:r1:000001"],
                )
                for name in FULL_ORDER_DIMENSIONS
            },
        }
        return {
            "verification_targets": {
                cls.VC5_ID: {
                    "binary": "recoil",
                    "kind": "vc5",
                    "name": cls.SHARED_NAME,
                    "registration": {"name": cls.SHARED_NAME},
                },
                cls.FUNCTIONAL_ID: {
                    "binary": "recoil",
                    "kind": "functional",
                    "name": cls.SHARED_NAME,
                    "registration": {"name": cls.SHARED_NAME},
                },
            },
            "symbols": {
                "recoil:function:0x401000": {
                    "verification_target_ids": [cls.VC5_ID, cls.FUNCTIONAL_ID],
                    "binary_state": deepcopy(accepted),
                    "accepted_byte_facts": {"target_id": cls.VC5_ID},
                },
                "recoil:function:0x401010": {
                    "verification_target_ids": [cls.FUNCTIONAL_ID],
                    "binary_state": deepcopy(accepted),
                    "accepted_byte_facts": {"target_id": cls.FUNCTIONAL_ID},
                },
            },
            "physical_blocks": {
                "recoil:block:0x401000": {
                    "order_targets": {"object": cls.VC5_ID, "linked": ""},
                    "order": deepcopy(accepted_order),
                    "accepted_order_facts": {"target_id": cls.VC5_ID},
                },
                "recoil:block:0x401010": {
                    "order_targets": {"object": cls.SHARED_NAME, "linked": ""},
                    "order": deepcopy(accepted_order),
                    "accepted_order_facts": {"target_id": cls.VC5_ID},
                },
                "recoil:block:0x401020": {
                    "order_targets": {"object": cls.FUNCTIONAL_ID, "linked": ""},
                    "order": deepcopy(accepted_order),
                    "accepted_order_facts": {"target_id": cls.FUNCTIONAL_ID},
                },
            },
        }

    def test_exact_id_retires_only_one_colliding_name_registration(self) -> None:
        data = self.fixture()
        unrelated_symbol_before = deepcopy(data["symbols"]["recoil:function:0x401010"])
        unrelated_block_before = deepcopy(data["physical_blocks"]["recoil:block:0x401020"])

        details = _retire_verification_target(data, selector=self.VC5_ID)

        self.assertEqual(self.VC5_ID, details["retired_target_id"])
        self.assertEqual(self.SHARED_NAME, details["retired_target_name"])
        self.assertNotIn(self.VC5_ID, data["verification_targets"])
        self.assertIn(self.FUNCTIONAL_ID, data["verification_targets"])
        self.assertEqual(
            [self.FUNCTIONAL_ID],
            data["symbols"]["recoil:function:0x401000"]["verification_target_ids"],
        )
        self.assertEqual(
            ["recoil:function:0x401000"],
            details["detached_symbol_ids"],
        )
        self.assertEqual(
            ["recoil:block:0x401000", "recoil:block:0x401010"],
            details["invalidated"]["block_ids"],
        )
        self.assertEqual(
            ["recoil:block:0x401000", "recoil:block:0x401010"],
            details["preserved_block_order_target_block_ids"],
        )
        self.assertEqual(
            ["recoil:function:0x401000"],
            details["invalidated"]["symbol_ids"],
        )
        self.assertEqual(
            {"object": self.VC5_ID, "linked": ""},
            data["physical_blocks"]["recoil:block:0x401000"]["order_targets"],
        )
        self.assertNotIn(
            "accepted_order_facts",
            data["physical_blocks"]["recoil:block:0x401000"],
        )
        self.assertNotIn(
            "accepted_byte_facts",
            data["symbols"]["recoil:function:0x401000"],
        )
        for state in data["symbols"]["recoil:function:0x401000"]["binary_state"].values():
            self.assertEqual("pending", state["result"])
            self.assertEqual("changed", state["freshness"])
        self.assertEqual(
            unrelated_symbol_before,
            data["symbols"]["recoil:function:0x401010"],
        )
        self.assertEqual(
            unrelated_block_before,
            data["physical_blocks"]["recoil:block:0x401020"],
        )

    def test_shared_name_fails_closed_without_mutation(self) -> None:
        data = self.fixture()
        before = deepcopy(data)

        with self.assertRaisesRegex(
            ProgressError,
            "resolves to 2 registrations; use the exact tracker target id",
        ):
            _retire_verification_target(data, selector=self.SHARED_NAME)

        self.assertEqual(before, data)

    def test_unique_name_is_supported_and_malformed_relationship_fails_before_mutation(self) -> None:
        data = self.fixture()
        del data["verification_targets"][self.FUNCTIONAL_ID]
        details = _retire_verification_target(data, selector=self.SHARED_NAME)
        self.assertEqual(self.VC5_ID, details["retired_target_id"])

        malformed = self.fixture()
        malformed["symbols"]["recoil:function:0x401010"]["verification_target_ids"] = "bad"
        before = deepcopy(malformed)
        with self.assertRaisesRegex(
            ProgressError,
            "invalid verification_target_ids",
        ):
            _retire_verification_target(malformed, selector=self.VC5_ID)
        self.assertEqual(before, malformed)


class FullOrderDualTargetRoutingTests(unittest.TestCase):
    LINKED_ID = "recoil:vc5-target:cabout_retail_interval_linked_order"
    OBJECT_ID = "recoil:vc5-target:cabout_prelude_provider_order_current_shape"

    def _full_order_contract(self) -> dict[str, object]:
        return progress_cli._current_order_contract(
            ProgressDocument(cabout_full_order_dual_target_fixture()),
            self.LINKED_ID,
            object_selector=self.OBJECT_ID,
        )

    def _run_full_order_summary(
        self,
        *,
        summary: dict[str, object],
        child_returncode: int,
        report: dict[str, object] | None = None,
    ) -> tuple[int, dict[str, object], str]:
        contract = self._full_order_contract()
        with tempfile.TemporaryDirectory() as temporary:
            fixture_root = Path(temporary)
            fixture_build_root = fixture_root / "build"
            fixture_build_root.mkdir()
            build_root = fixture_build_root / "linked"

            def run_linked(_command: list[str], **_kwargs: object) -> SimpleNamespace:
                build_root.mkdir(parents=True)
                emitted_summary = deepcopy(summary)
                if report is not None:
                    report_path = build_root / "linked_order_cabout.json"
                    report_path.write_text(json.dumps(report), encoding="utf-8")
                    emitted_summary["order_reports"] = [
                        {"path": str(report_path.resolve())}
                    ]
                (build_root / "summary.json").write_text(
                    json.dumps(emitted_summary),
                    encoding="utf-8",
                )
                return SimpleNamespace(
                    returncode=child_returncode,
                    stdout="",
                    stderr="delegated child stderr",
                )

            with (
                patch.object(progress_cli, "REPO_ROOT", fixture_root),
                patch.object(progress_cli.subprocess, "run", side_effect=run_linked),
            ):
                return progress_cli._run_full_linked_order_validation(
                    contract=contract,
                    build_root=build_root,
                    progress_path=Path("fixture-progress.json"),
                )

    def test_sync_keeps_cabout_multi_source_targets_current_and_launchable(self) -> None:
        data = cabout_full_order_dual_target_fixture()

        details = _sync_verification_targets(
            data,
            binary="recoil",
            selectors=[self.LINKED_ID, self.OBJECT_ID],
        )

        self.assertEqual([], details["added"])
        self.assertEqual([], details["updated"])
        self.assertEqual(sorted([self.LINKED_ID, self.OBJECT_ID]), details["unchanged"])
        self.assertEqual(
            [
                "src/Battlesport/RecoilApp.cpp",
                "src/Battlesport/about.cpp",
                "src/Battlesport/about.h",
                "src/Battlesport/hud.cpp",
                "tools/_recoil/compat/include/recoil/Mfc42Abi.h",
                "src/GameZRecoil/zUI/zui_widgets.cpp",
            ],
            data["verification_targets"][self.OBJECT_ID]["registration"][
                "order_edit_paths"
            ],
        )
        self.assertNotIn(
            "order_edit_paths",
            data["verification_targets"][self.LINKED_ID]["registration"],
        )

        document = ProgressDocument(data)
        pipeline = document.pipeline("recoil")
        self.assertEqual("full-function-order", pipeline["phase"])
        self.assertEqual("ready", pipeline["order_target_resolution"]["status"])
        contract = progress_cli._current_order_contract(
            document,
            self.LINKED_ID,
            object_selector=self.OBJECT_ID,
        )
        self.assertEqual(self.LINKED_ID, contract["target_id"])
        self.assertEqual(self.OBJECT_ID, contract["object_contract"]["target_id"])
        _work_id, packet = _order_claim_candidate(document, pipeline)
        self.assertEqual(self.OBJECT_ID, packet["worker_target_id"])

    def test_exact_cabout_401000_routes_object_worker_and_linked_parent(self) -> None:
        document = ProgressDocument(cabout_full_order_dual_target_fixture())
        pipeline = document.pipeline("recoil")

        self.assertEqual("full-function-order", pipeline["phase"])
        resolution = pipeline["order_target_resolution"]
        self.assertEqual("ready", resolution["status"])
        self.assertEqual(self.LINKED_ID, resolution["target_id"])
        self.assertEqual(self.LINKED_ID, resolution["linked_target_id"])
        self.assertEqual(self.OBJECT_ID, resolution["object_target_id"])
        self.assertIn(f"--target {self.LINKED_ID}", pipeline["next_command"])
        self.assertIn(f"--object-target {self.OBJECT_ID}", pipeline["next_command"])

        contract = progress_cli._current_order_contract(
            document,
            self.LINKED_ID,
            object_selector=self.OBJECT_ID,
        )
        self.assertEqual(
            [f"recoil:function:{address}" for address in ("0x401000", "0x401020", "0x401030", "0x401040", "0x401050")],
            contract["identities"],
        )
        self.assertEqual(
            ["recoil:function:0x401000", "recoil:function:0x401030"],
            contract["object_contract"]["identities"],
        )

        _work_id, packet = _order_claim_candidate(document, pipeline)
        self.assertEqual(self.LINKED_ID, packet["target_id"])
        self.assertEqual(self.LINKED_ID, packet["linked_target_id"])
        self.assertEqual(self.OBJECT_ID, packet["object_target_id"])
        self.assertEqual(self.OBJECT_ID, packet["worker_target_id"])
        self.assertEqual(1, len(packet["validation_commands"]))
        self.assertIn(
            "verify vc5-order cabout_prelude_provider_order_current_shape",
            packet["validation_commands"][0],
        )
        self.assertIn(
            "--linked-target cabout_retail_interval_linked_order",
            packet["validation_commands"][0],
        )
        self.assertIn(
            "--build-root build/live-validation/worker-order/"
            "cabout_retail_interval_linked_order-r1381",
            packet["validation_commands"][0],
        )
        self.assertIn("exact full linked selected population/seams", packet["objective"])
        self.assertNotEqual(
            "cabout_retail_interval_linked_order",
            packet["validation_commands"][0].split("verify vc5-order ", 1)[1].split()[0],
        )

    def test_full_order_dual_target_router_fails_closed(self) -> None:
        cases = []

        data = cabout_full_order_dual_target_fixture()
        data["physical_blocks"]["recoil:block:0x401000"]["order_targets"]["object"] = ""
        cases.append((data, "full-order-object-target-missing"))

        data = cabout_full_order_dual_target_fixture()
        data["physical_blocks"]["recoil:block:0x401000"]["order_targets"]["object"] = (
            "cabout_retail_interval_linked_order"
        )
        cases.append((data, "linked-only"))

        data = cabout_full_order_dual_target_fixture()
        data["verification_targets"][self.OBJECT_ID]["binary"] = "messages"
        cases.append((data, "must be one recoil VC5"))

        data = cabout_full_order_dual_target_fixture()
        data["verification_targets"][self.OBJECT_ID]["registration"]["name"] = "stale"
        cases.append((data, "stale or conflicting"))

        data = cabout_full_order_dual_target_fixture()
        data["verification_targets"][self.OBJECT_ID]["registration"][
            "order_edit_paths"
        ].pop()
        cases.append((data, "stale or conflicting"))

        data = cabout_full_order_dual_target_fixture()
        duplicate = deepcopy(data["verification_targets"][self.OBJECT_ID])
        data["verification_targets"]["recoil:vc5-target:cabout-duplicate"] = duplicate
        cases.append((data, "resolved to 2 registered targets"))

        for data, message in cases:
            with self.subTest(message=message):
                resolution = ProgressDocument(data).pipeline("recoil")["order_target_resolution"]
                self.assertEqual("blocked", resolution["status"])
                self.assertIn(
                    message,
                    f"{resolution.get('reason_code', '')} {resolution['reason']}",
                )

    def test_full_parent_validation_consumes_exact_linked_report_not_object_sequence(self) -> None:
        document = ProgressDocument(cabout_full_order_dual_target_fixture())
        contract = progress_cli._current_order_contract(
            document,
            self.LINKED_ID,
            object_selector=self.OBJECT_ID,
        )
        with tempfile.TemporaryDirectory() as temporary:
            fixture_root = Path(temporary)
            fixture_build_root = fixture_root / "build"
            fixture_build_root.mkdir()
            build_root = fixture_build_root / "linked"

            def run_linked(command: list[str], **_kwargs: object) -> SimpleNamespace:
                child_argv = command[command.index("linked-order") + 1 :]
                child_args = linked_order.build_parser().parse_args(child_argv)
                self.assertEqual("cabout_retail_interval_linked_order", child_args.target)
                self.assertEqual("full", child_args.scope)
                self.assertFalse(child_args.build_root.is_absolute())
                self.assertEqual(
                    build_root.resolve(),
                    (fixture_root / child_args.build_root).resolve(),
                )
                def resolve_explicit_build_root(
                    directory: Path,
                    *,
                    canonical_build_dir: Path,
                ) -> Path:
                    self.assertFalse(directory.is_absolute())
                    self.assertNotEqual(
                        build_root.resolve(),
                        canonical_build_dir.resolve(),
                    )
                    return (fixture_root / directory).resolve()

                with (
                    patch.object(
                        vc5_build,
                        "validate_explicit_build_dir",
                        side_effect=resolve_explicit_build_root,
                    ),
                    patch.object(vc5_build, "run_build", return_value=0) as run_build,
                ):
                    self.assertEqual(0, linked_order.main(child_argv))
                configured = run_build.call_args.args[0]
                self.assertEqual(build_root.resolve(), configured.build_dir.resolve())
                self.assertTrue(configured.build_dir_explicit)

                build_root.mkdir(parents=True)
                report_path = build_root / "linked_order_cabout.json"
                report = {
                    "kind": "linked-function-order-report",
                    "target": "cabout_retail_interval_linked_order",
                    "interval": "cabout_retail_interval",
                    "order_scope": "full",
                    "binary": "recoil",
                    "retail_start": "0x401000",
                    "retail_end_exclusive": "0x401060",
                    "passed": True,
                    "linked_order_evaluated": True,
                    "linked_order_passed": True,
                    "required_presence_passed": True,
                    "block_precedence_passed": True,
                    "exact_selected_sequence_matches_manifest": True,
                    "exact_sequence_address_seam_claimed": True,
                    "boundary_sentinels_passed": True,
                    "linked_exact_selected_population_evaluated": True,
                    "linked_exact_selected_population_passed": True,
                    "linked_seams_and_rvas_evaluated": True,
                    "linked_seams_and_rvas_passed": True,
                    "raw_definition_inventory_complete": True,
                    "first_divergence": None,
                }
                report_path.write_text(json.dumps(report), encoding="utf-8")
                (build_root / "summary.json").write_text(
                    json.dumps(
                        {
                            "kind": "linked-function-order-run",
                            "success": True,
                            "binary": "recoil",
                            "order_scope": "full",
                            "order_reports": [{"path": str(report_path.resolve())}],
                        }
                    ),
                    encoding="utf-8",
                )
                self.assertIn("linked-order", command)
                self.assertIn("cabout_retail_interval_linked_order", command)
                self.assertNotIn("cabout_prelude_provider_order_current_shape", command)
                return SimpleNamespace(returncode=0, stdout="", stderr="")

            with (
                patch.object(progress_cli, "REPO_ROOT", fixture_root),
                patch.object(progress_cli.subprocess, "run", side_effect=run_linked),
            ):
                code, result, _stderr = progress_cli._run_full_linked_order_validation(
                    contract=contract,
                    build_root=build_root,
                    progress_path=Path("fixture-progress.json"),
                )
        self.assertEqual(0, code)
        self.assertTrue(result["passed"])
        self.assertEqual(5, len(result["expected_sequence"]))
        self.assertEqual(contract["identities"], result["candidate_sequence"])

    def test_full_parent_validation_reports_first_delegated_vc5_execution_failure(self) -> None:
        cases = (
            (
                "compile:zGame.cpp",
                2,
                {
                    "stdout_log": "build/live-validation/logs/compile.out.log",
                    "stderr_log": "build/live-validation/logs/compile.err.log",
                },
            ),
            ("resource", 3, {}),
            (
                "link",
                1104,
                {"stderr_log": "build/live-validation/logs/link.err.log"},
            ),
        )
        for name, exit_code, logs in cases:
            with self.subTest(name=name):
                summary = {
                    "dry_run": False,
                    "results": [
                        {"name": "compile:earlier.cpp", "returncode": 0},
                        {"name": name, "returncode": exit_code, **logs},
                    ],
                }
                with self.assertRaisesRegex(
                    ProgressError,
                    rf"delegated VC5 build failed: "
                    rf"first_nonzero_result='{name}', exit_code={exit_code}",
                ) as raised:
                    self._run_full_order_summary(
                        summary=summary,
                        child_returncode=exit_code,
                    )
                diagnostic = str(raised.exception)
                for key, value in logs.items():
                    self.assertIn(f"{key}='{value}'", diagnostic)
                for key in {"stdout_log", "stderr_log"} - logs.keys():
                    self.assertNotIn(f"{key}=", diagnostic)
                self.assertNotIn("wrong kind, binary, or scope", diagnostic)

    def test_full_parent_validation_keeps_divergence_distinct_from_malformed_success(self) -> None:
        report = {
            "kind": "linked-function-order-report",
            "target": "cabout_retail_interval_linked_order",
            "interval": "cabout_retail_interval",
            "order_scope": "full",
            "binary": "recoil",
            "retail_start": "0x401000",
            "retail_end_exclusive": "0x401060",
            "passed": False,
            "first_divergence": {
                "kind": "candidate-sequence-mismatch",
                "index": 2,
            },
        }
        code, result, _stderr = self._run_full_order_summary(
            summary={
                "kind": "linked-function-order-run",
                "success": False,
                "binary": "recoil",
                "order_scope": "full",
            },
            child_returncode=1,
            report=report,
        )
        self.assertEqual(1, code)
        self.assertFalse(result["passed"])
        self.assertEqual(report["first_divergence"], result["first_divergence"])

        with self.assertRaisesRegex(
            ProgressError,
            "full linked-order summary has the wrong kind, binary, or scope",
        ):
            self._run_full_order_summary(
                summary={
                    "dry_run": False,
                    "results": [{"name": "compile:zGame.cpp", "returncode": 0}],
                },
                child_returncode=0,
            )
        with self.assertRaisesRegex(
            ProgressError,
            "full linked-order summary must expose one exact report",
        ):
            self._run_full_order_summary(
                summary={
                    "kind": "linked-function-order-run",
                    "success": True,
                    "binary": "recoil",
                    "order_scope": "full",
                    "order_reports": [],
                },
                child_returncode=0,
            )

    def test_delegated_vc5_build_failure_cannot_mutate_progress(self) -> None:
        data = cabout_full_order_dual_target_fixture()
        contract = progress_cli._current_order_contract(
            ProgressDocument(data),
            self.LINKED_ID,
            object_selector=self.OBJECT_ID,
        )
        with tempfile.TemporaryDirectory() as temporary:
            fixture_root = Path(temporary)
            temporary_path = fixture_root / "build"
            temporary_path.mkdir()
            progress_path = temporary_path / "progress.json"
            original = json.dumps(data, indent=2) + "\n"
            progress_path.write_text(original, encoding="utf-8")
            build_root = temporary_path / "linked"
            real_subprocess_run = progress_cli.subprocess.run

            def fail_compile(command: list[str], **kwargs: object) -> SimpleNamespace:
                if command and command[0] == "git":
                    return real_subprocess_run(command, **kwargs)
                build_root.mkdir(parents=True)
                (build_root / "summary.json").write_text(
                    json.dumps(
                        {
                            "dry_run": False,
                            "results": [
                                {
                                    "name": "compile:zGame.cpp",
                                    "returncode": 2,
                                    "stderr_log": str(
                                        build_root / "logs" / "compile.err.log"
                                    ),
                                }
                            ],
                        }
                    ),
                    encoding="utf-8",
                )
                return SimpleNamespace(returncode=2, stdout="", stderr="")

            args = argparse.Namespace(
                progress=progress_path,
                expected_revision=1380,
                target=self.LINKED_ID,
                object_target=self.OBJECT_ID,
                linked_target=None,
                build_root=build_root,
                apply=True,
            )
            with (
                patch.object(progress_cli, "REPO_ROOT", fixture_root),
                patch.object(
                    progress_cli,
                    "_current_order_contract",
                    return_value=contract,
                ),
                patch.object(
                    progress_cli,
                    "_require_order_contract_source_fragments_clean",
                ),
                patch.object(progress_cli.subprocess, "run", side_effect=fail_compile),
                self.assertRaisesRegex(
                    ProgressError,
                    "first_nonzero_result='compile:zGame.cpp', exit_code=2",
                ),
            ):
                progress_cli.advance_live_order(args)
            after = progress_path.read_text(encoding="utf-8")

        self.assertEqual(original, after)


ZGAME_MULTI_SLICE_ID = "recoil:vc5-target:zgame_opt_4b2960_4b33f0_authored_order"
ZSYS_MULTI_SLICE_ID = "recoil:vc5-target:zsys_cpu_4b33f0_4b3ce0_authored_order"


def zgame_zsys_multi_slice_fixture(
    *, accepted_authored_blocks: tuple[str, ...] = ()
) -> dict[str, object]:
    from _recoil.lib.verification_targets import vc5_target_registration

    data = empty_progress_document()
    data["revision"] = 1368
    data["binaries"]["recoil"] = {
        "binary": "recoil",
        "primary_scheduler": True,
        "text": {"start": "0x4b2960", "end_exclusive": "0x4b3ce0"},
    }
    target_specs = (
        (
            ZGAME_MULTI_SLICE_ID,
            "zgame_opt_4b2960_4b33f0_authored_order",
            "zgame_opt.c",
        ),
        (
            ZSYS_MULTI_SLICE_ID,
            "zsys_cpu_4b33f0_4b3ce0_authored_order",
            "zsys_cpu.cpp",
        ),
    )
    target_rows: dict[str, list[dict[str, object]]] = {}
    for expected_id, name, _source_name in target_specs:
        target_id, target = vc5_target_registration(
            REPO_ROOT / "tools" / "vc5_verify_targets" / f"{name}.json"
        )
        if target_id != expected_id:
            raise AssertionError((target_id, expected_id))
        data["verification_targets"][target_id] = target
        target_rows[target_id] = [
            dict(row)
            for group in target["registration"]["translation_unit_function_order"]
            for row in group["functions"]
        ]

    block_specs = (
        (
            "recoil:block:0x4b2960",
            "0x4b2960",
            "0x4b31c0",
            ZGAME_MULTI_SLICE_ID,
            "zgame_opt_4b2960_4b33f0_authored_order",
            "src/GameZRecoil/zGame/zgame_opt.c",
        ),
        (
            "recoil:block:0x4b31c0",
            "0x4b31c0",
            "0x4b31f0",
            ZSYS_MULTI_SLICE_ID,
            "zsys_cpu_4b33f0_4b3ce0_authored_order",
            "src/GameZRecoil/zSys/zsys_cpu.cpp",
        ),
        (
            "recoil:block:0x4b31f0",
            "0x4b31f0",
            "0x4b33f0",
            ZGAME_MULTI_SLICE_ID,
            "zgame_opt_4b2960_4b33f0_authored_order",
            "src/GameZRecoil/zGame/zgame_opt.c",
        ),
        (
            "recoil:block:0x4b33f0",
            "0x4b33f0",
            "0x4b3ce0",
            ZSYS_MULTI_SLICE_ID,
            "zsys_cpu_4b33f0_4b3ce0_authored_order",
            "src/GameZRecoil/zSys/zsys_cpu.cpp",
        ),
    )
    accepted_state = {
        name: state_record(
            result="passed",
            disposition="accepted",
            freshness="current",
            evidence_ids=["recoil:evidence:fixture"],
            gating=True,
            validation_mode="live",
        )
        for name in AUTHORED_ORDER_DIMENSIONS
    }
    for block_id, start, end, target_id, target_name, source_path in block_specs:
        rows = [
            row
            for row in target_rows[target_id]
            if int(start, 16) <= int(str(row["address"]), 16) < int(end, 16)
        ]
        contribution_ids = []
        for index, row in enumerate(rows):
            address = str(row["address"])
            symbol_id = f"recoil:function:{address}"
            contribution_ids.append(symbol_id)
            next_address = str(rows[index + 1]["address"]) if index + 1 < len(rows) else end
            data["symbols"][symbol_id] = {
                "address": address,
                "authored_order_role": "authored-body",
                "binary": "recoil",
                "binary_state": {},
                "end_exclusive": next_address,
                "kind": "function",
                "physical_block_id": block_id,
                "pipeline_class": "authored",
                "semantic_span_ids": [],
            }
        block = {
            "agent_source_path": source_path,
            "binary": "recoil",
            "contribution_ids": contribution_ids,
            "contribution_kind": "authored",
            "end_exclusive": end,
            "mapping": {"state": "mapped-no-literal-provisional", "status": "fixture"},
            "order": {
                "authored": (
                    deepcopy(accepted_state)
                    if block_id in accepted_authored_blocks
                    else {name: state_record() for name in AUTHORED_ORDER_DIMENSIONS}
                ),
                "full": {name: state_record() for name in FULL_ORDER_DIMENSIONS},
            },
            "order_targets": {"linked": "", "object": target_name},
            "source_path": source_path,
            "source_shape_inputs": [],
            "start": start,
        }
        if block_id in accepted_authored_blocks:
            block["accepted_order_facts"] = {
                "validation_mode": "live",
                "target_id": target_id,
                "phase": "authored-function-order",
                "covered_block_ids": [block_id],
                "matched_identities": contribution_ids,
            }
        data["physical_blocks"][block_id] = block
    return data


class AuthoredOrderMultiSliceRoutingTests(unittest.TestCase):
    GAME_FIRST = "recoil:block:0x4b2960"
    SYS_FIRST = "recoil:block:0x4b31c0"
    GAME_LATER = "recoil:block:0x4b31f0"
    SYS_LATER = "recoil:block:0x4b33f0"

    def test_interleaved_targets_route_only_the_current_exact_slice(self) -> None:
        stages = (
            ((), ZGAME_MULTI_SLICE_ID, self.GAME_FIRST, 0),
            ((self.GAME_FIRST,), ZSYS_MULTI_SLICE_ID, self.SYS_FIRST, 0),
            (
                (self.GAME_FIRST, self.SYS_FIRST),
                ZGAME_MULTI_SLICE_ID,
                self.GAME_LATER,
                1,
            ),
            (
                (self.GAME_FIRST, self.SYS_FIRST, self.GAME_LATER),
                ZSYS_MULTI_SLICE_ID,
                self.SYS_LATER,
                1,
            ),
        )
        expected_owned = {
            ZGAME_MULTI_SLICE_ID: [self.GAME_FIRST, self.GAME_LATER],
            ZSYS_MULTI_SLICE_ID: [self.SYS_FIRST, self.SYS_LATER],
        }
        for accepted, expected_target, current_block, slice_index in stages:
            with self.subTest(current_block=current_block):
                document = ProgressDocument(
                    zgame_zsys_multi_slice_fixture(accepted_authored_blocks=accepted)
                )
                pipeline = document.pipeline("recoil")
                resolution = pipeline["order_target_resolution"]
                self.assertEqual("ready", resolution["status"])
                self.assertEqual(expected_target, resolution["target_id"])
                self.assertEqual([current_block], resolution["covered_block_ids"])
                contract = progress_cli._current_order_contract(document, expected_target)
                self.assertEqual([current_block], contract["covered_block_ids"])
                self.assertEqual(expected_owned[expected_target], contract["target_owned_block_ids"])
                self.assertEqual(slice_index, contract["current_slice_index"])
                self.assertEqual(
                    [[block_id] for block_id in expected_owned[expected_target]],
                    contract["target_slices"],
                )
                _work_id, packet = _order_claim_candidate(document, pipeline)
                self.assertEqual([current_block], packet["covered_block_ids"])
                self.assertEqual(
                    [current_block],
                    [
                        claim["id"]
                        for claim in packet["resource_claims"]
                        if claim["kind"] == "block" and claim["access"] == "write"
                    ],
                )

    def test_interleaving_fails_closed_for_partial_later_acceptance_or_incomplete_block(self) -> None:
        data = zgame_zsys_multi_slice_fixture(
            accepted_authored_blocks=(self.GAME_LATER,)
        )
        data["physical_blocks"][self.GAME_LATER]["order"]["authored"][
            AUTHORED_ORDER_DIMENSIONS[0]
        ] = state_record("pending", "observed", "changed", [])
        resolution = ProgressDocument(data).pipeline("recoil")["order_target_resolution"]
        self.assertEqual("blocked", resolution["status"])
        detail = " ".join(
            [
                str(resolution.get("reason", "")),
                str(resolution.get("configured_error", "")),
                *[str(item) for item in resolution.get("rejected_examples", [])],
            ]
        )
        self.assertIn("later physical slice", detail)
        self.assertIn("partial, stale, mixed, or ambiguous order state", detail)

        data = zgame_zsys_multi_slice_fixture()
        extra_id = "recoil:function:0x4b2f80"
        data["symbols"][extra_id] = {
            "address": "0x4b2f80",
            "authored_order_role": "authored-body",
            "binary": "recoil",
            "binary_state": {},
            "end_exclusive": "0x4b2fa0",
            "kind": "function",
            "physical_block_id": self.GAME_FIRST,
            "pipeline_class": "authored",
            "semantic_span_ids": [],
        }
        data["physical_blocks"][self.GAME_FIRST]["contribution_ids"].append(extra_id)
        resolution = ProgressDocument(data).pipeline("recoil")["order_target_resolution"]
        self.assertEqual("blocked", resolution["status"])
        detail = " ".join(
            [
                str(resolution.get("reason", "")),
                str(resolution.get("configured_error", "")),
                *[str(item) for item in resolution.get("rejected_examples", [])],
            ]
        )
        self.assertIn("omits a covered-block authored gating identity", detail)

    def test_r1371_routes_zsys_and_keeps_unrelated_ainet_role_gate_diagnostic(self) -> None:
        from _recoil.lib.verification_targets import vc5_target_registration

        data = zgame_zsys_multi_slice_fixture(
            accepted_authored_blocks=(self.GAME_FIRST, self.SYS_LATER)
        )
        data["revision"] = 1371
        data["physical_blocks"][self.SYS_FIRST]["order_targets"]["object"] = ""
        ainet_id, ainet = vc5_target_registration(
            REPO_ROOT / "tools" / "vc5_verify_targets" / "ainet_text_block_order.json"
        )
        for group in ainet["registration"]["translation_unit_function_order"]:
            for row in group["functions"]:
                if row["address"] == "0x401060":
                    row.pop("authored_order_role", None)
        data["verification_targets"][ainet_id] = ainet

        document = ProgressDocument(data)
        pipeline = document.pipeline("recoil")
        resolution = pipeline["order_target_resolution"]

        self.assertEqual(1371, document.revision)
        self.assertEqual("0x4b31c0", pipeline["cursor"])
        self.assertEqual("ready", resolution["status"])
        self.assertEqual(ZSYS_MULTI_SLICE_ID, resolution["target_id"])
        self.assertEqual([self.SYS_FIRST], resolution["covered_block_ids"])
        self.assertTrue(
            any(
                ainet_id in item and "role-gate diagnostic" in item
                for item in resolution["rejected_examples"]
            ),
            resolution["rejected_examples"],
        )

        contract = progress_cli._current_order_contract(
            document,
            ZSYS_MULTI_SLICE_ID,
            override_selector=ZSYS_MULTI_SLICE_ID,
        )
        self.assertEqual([self.SYS_FIRST], contract["covered_block_ids"])
        self.assertEqual(
            [self.SYS_FIRST, self.SYS_LATER], contract["target_owned_block_ids"]
        )

    def test_owning_target_unresolved_role_remains_a_typed_blocker(self) -> None:
        data = zgame_zsys_multi_slice_fixture(
            accepted_authored_blocks=(self.GAME_FIRST, self.SYS_LATER)
        )
        data["physical_blocks"][self.SYS_FIRST]["order_targets"]["object"] = ""
        data["symbols"]["recoil:function:0x4b31c0"]["authored_order_role"] = (
            "unresolved"
        )

        resolution = ProgressDocument(data).pipeline("recoil")["order_target_resolution"]

        self.assertEqual("blocked", resolution["status"])
        self.assertEqual("order-target-role-gate-blocked", resolution["reason_code"])
        self.assertEqual(ZSYS_MULTI_SLICE_ID, resolution["blocker"]["target_id"])
        self.assertEqual("0x4b31c0", resolution["blocker"]["address"])

    def test_whole_target_pass_accepts_only_the_later_current_slice(self) -> None:
        data = zgame_zsys_multi_slice_fixture(
            accepted_authored_blocks=(self.GAME_FIRST, self.SYS_FIRST)
        )
        with tempfile.TemporaryDirectory(dir=REPO_ROOT) as temporary:
            temporary_path = Path(temporary)
            progress_path = temporary_path / "progress.json"
            progress_path.write_text(json.dumps(data), encoding="utf-8")
            document = ProgressDocument(data)
            contract = progress_cli._current_order_contract(document, ZGAME_MULTI_SLICE_ID)
            expected = list(contract["identities"])
            raw = {
                "kind": "vc5-order-live-result",
                "target_id": "zgame_opt_4b2960_4b33f0_authored_order",
                "phase": "authored-function-order",
                "physical_block_id": self.GAME_FIRST,
                "passed": True,
                "expected_sequence": expected,
                "candidate_sequence": expected,
                "matched_prefix_count": len(expected),
                "first_divergence": None,
            }
            build_root = Path("build") / temporary_path.name / "later-zgame-slice"
            args = argparse.Namespace(
                progress=progress_path,
                expected_revision=1368,
                target=ZGAME_MULTI_SLICE_ID,
                object_target=None,
                linked_target=None,
                build_root=build_root,
                apply=True,
            )
            with (
                patch.object(
                    progress_cli,
                    "_require_order_contract_source_fragments_clean",
                ),
                patch.object(progress_cli, "_run_json_process", return_value=(0, raw, "")),
            ):
                returncode, result = progress_cli.advance_live_order(args)
            applied = json.loads(progress_path.read_text(encoding="utf-8"))

        self.assertEqual(0, returncode)
        self.assertEqual([self.GAME_LATER], result["accepted_block_ids"])
        self.assertEqual(20, result["validated_identity_count"])
        self.assertEqual(7, result["committed_identity_count"])
        self.assertTrue(
            ProgressDocument(applied)._order_group_current(
                applied["physical_blocks"][self.GAME_LATER],
                "authored",
                AUTHORED_ORDER_DIMENSIONS,
            )
        )
        self.assertFalse(
            ProgressDocument(applied)._order_group_current(
                applied["physical_blocks"][self.SYS_LATER],
                "authored",
                AUTHORED_ORDER_DIMENSIONS,
            )
        )


class ClaimCurrentIsolationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        super().setUpClass()
        cls.issue_temporary = tempfile.TemporaryDirectory()
        cls.issue_ledger = Path(cls.issue_temporary.name) / "issues.sqlite3"
        create_issue_database(
            cls.issue_ledger,
            workspace_issues.empty_ledger(),
            cutover_pair_id="pair:test:claim-current-isolation",
        )

    @classmethod
    def tearDownClass(cls) -> None:
        cls.issue_temporary.cleanup()
        super().tearDownClass()

    @staticmethod
    def _packet(*, lane: str, path: str, cursor: str) -> dict[str, object]:
        return {
            "state": "ready",
            "packet_type": "order-edit-v1" if lane == "primary" else "byte-edit-v1",
            "phase": "authored-function-order" if lane == "primary" else "authored-byte-match",
            "lane": lane,
            "cursor": cursor,
            "validation_commands": [
                "python tools/recoil.py verify vc5-order unit --build-root build/unit"
            ],
            "resource_claims": [
                {"kind": "path", "id": path, "access": "write"},
                {"kind": "lane", "id": f"unit-{lane}", "access": "write"},
            ],
        }

    @staticmethod
    def _pipeline() -> dict[str, object]:
        return {
            "complete": False,
            "primary_lane": "order",
            "phase": "authored-function-order",
            "cursor": "0x480000",
            "parallel_authored_byte_cursor": "0x401000",
            "parallel_authored_object_byte_cursor": "0x402000",
            "authored_byte_lane": {"state": "ready"},
            "authored_object_byte_lane": {"state": "ready"},
        }

    def test_read_only_claim_description_does_not_create_work_items(self) -> None:
        work_items: dict[str, object] = {}
        document = SimpleNamespace(
            path=Path("unit-progress.json"),
            revision=12,
            collection=lambda name: work_items if name == "work_items" else {},
        )

        def byte_candidate(*_args: object, **kwargs: object) -> tuple[str, dict[str, object]]:
            lane = str(kwargs["packet_lane"])
            cursor = str(kwargs["cursor"])
            return (
                f"unit:{lane}",
                self._packet(lane=lane, path=f"src/{lane}.cpp", cursor=cursor),
            )

        with (
            patch(
                "_recoil.commands.progress_cli._order_claim_candidate",
                return_value=(
                    "unit:primary",
                    self._packet(lane="primary", path="src/order.cpp", cursor="0x480000"),
                ),
            ),
            patch(
                "_recoil.commands.progress_cli._byte_lane_preflight",
                return_value={
                    "passed": True,
                    "reason_code": "live-byte-preflight-ready",
                    "reason": "ready",
                },
            ),
            patch(
                "_recoil.commands.progress_cli._byte_claim_candidate",
                side_effect=byte_candidate,
            ),
            patch(
                "_recoil.commands.progress_cli._candidate_active_conflicts",
                return_value=([], []),
            ),
        ):
            described = progress_cli.describe_current_claim_opportunities(
                document, self._pipeline(), issue_ledger=self.issue_ledger
            )

        self.assertEqual({}, work_items)
        self.assertEqual(
            ["primary", "authored", "object"],
            [row["lane"] for row in described["launch_plan"]],
        )

    def test_next_command_uses_its_routed_issue_ledger_argument(self) -> None:
        progress_path = Path("C:/canonical/.agent/RECONSTRUCTION_PROGRESS.sqlite3")
        issue_path = Path("C:/canonical/.agent/WORKSPACE_ISSUES.sqlite3")
        document = SimpleNamespace()
        captured: dict[str, Path] = {}

        def next_work(
            observed_document: object,
            _binary: str,
            *,
            issue_ledger: str | Path,
        ) -> dict[str, object]:
            self.assertIs(document, observed_document)
            captured["issue_ledger"] = Path(issue_ledger)
            return {"phase": "unit", "cursor": ""}

        with (
            patch.object(progress_cli, "_load", return_value=document) as load,
            patch.object(
                progress_cli,
                "_next_work_with_issue_ledger",
                side_effect=next_work,
            ),
            patch.object(
                progress_cli,
                "_scheduler_domain_guarded_call_contract_commands",
                side_effect=lambda _document, payload: payload,
            ),
            patch.object(progress_cli, "_print_json"),
        ):
            result = progress_cli.main(
                [
                    "next",
                    "--progress",
                    str(progress_path),
                    "--issue-ledger",
                    str(issue_path),
                    "--json",
                ]
            )

        self.assertEqual(0, result)
        load.assert_called_once_with(progress_path)
        self.assertEqual(issue_path, captured["issue_ledger"])

    def test_planned_active_tracked_write_packet_is_not_handoff_visible(self) -> None:
        work_id = "unit:planned-active"
        work = self._packet(
            lane="primary", path="src/order.cpp", cursor="0x480000"
        )
        work["reservation"] = {
            "id": "recoil:reservation:planned-active",
            "state": "active",
        }
        work["packet_contract_version"] = 4
        work["progress_packet_adapter"] = "native-git-v1-planned"

        with self.assertRaisesRegex(
            ProgressError, "planned native-git-v1 allocation.*not handoff-visible"
        ):
            progress_cli._compact_reserved_packet(work_id, work)

    def test_byte_packet_defers_progress_binding_until_authenticated_allocation(
        self,
    ) -> None:
        document = SimpleNamespace(
            revision=12,
            _fresh_root=lambda *_args: "build/live-validation/worker-authored-byte/unit",
            collection=lambda _name: {},
        )
        scope = {
            "source_paths": ["src/unit.cpp"],
            "read_paths": ["src/unit.h"],
            "owner_ids": ["recoil:owner:unit"],
            "block_id": "recoil:block:0x401000",
            "scope_ids": ["recoil:function:0x401000"],
            "target_ids": ["recoil:vc5-target:unit"],
        }
        with (
            patch.object(progress_cli, "_byte_scope", return_value=scope),
            patch.object(
                progress_cli,
                "bind_work_packet_contract",
                side_effect=lambda _document, packet: packet,
            ),
        ):
            _work_id, work = progress_cli._byte_claim_candidate(
                document,
                packet_lane="authored",
                verifier_lane="authored",
                cursor="0x401000",
                phase="authored-call-contract",
                progress_path=Path(".agent/RECONSTRUCTION_PROGRESS.sqlite3"),
                preflight={"passed": True},
            )

        command = work["validation_commands"][0]
        self.assertIn("--progress {progress_path}", command)
        self.assertNotIn("--progress .agent/", command.replace("\\", "/"))

    def test_native_git_progress_binding_rejects_relative_and_authenticates_absolute(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            canonical = Path(temporary) / "canonical authority.sqlite3"
            canonical.write_text("fixture", encoding="utf-8")
            template = (
                "python tools/recoil.py verify authored-byte --at 0x401000 "
                "--build-root build/unit --progress {progress_path} --json"
            )
            bound = progress_cli._bind_native_git_progress_authority(
                template,
                canonical,
            )
            self.assertNotIn("{progress_path}", bound)
            self.assertIn(canonical.resolve().as_posix(), bound)
            self.assertNotIn("--progress .agent/", bound.replace("\\", "/"))

            with self.assertRaisesRegex(
                ProgressError,
                "binds a live progress authority before authenticated native-Git allocation",
            ):
                progress_cli._bind_native_git_progress_authority(
                    "python tools/recoil.py verify authored-byte --at 0x401000 "
                    "--build-root build/unit --progress "
                    ".agent/RECONSTRUCTION_PROGRESS.sqlite3 --json",
                    canonical,
                )
            with self.assertRaisesRegex(
                ProgressError,
                "linked-worktree-relative live progress authority",
            ):
                progress_cli._validate_native_git_progress_authority(
                    [
                        "python",
                        "tools/recoil.py",
                        "verify",
                        "authored-byte",
                        "--progress",
                        ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
                    ],
                    progress_path=canonical,
                )

    def test_dry_run_claim_routing_does_no_verification_before_reservation(self) -> None:
        original = {
            "revision": 12,
            "migration": {},
            "work_items": {},
        }
        packet = self._packet(
            lane="primary",
            path="src/order.cpp",
            cursor="0x480000",
        )

        class RoutingDocument:
            def __init__(self, data, path=None):
                self.data = data
                self.path = path
                self.revision = int(data["revision"])

            def pipeline(self, _binary):
                return ClaimCurrentIsolationTests._pipeline()

            def collection(self, name):
                return self.data.get(name, {})

        class DryRunCommit:
            def to_dict(self):
                return {"applied": False, "previous_revision": 12, "revision": 13}

        class DryRunStore:
            def __init__(self):
                self.proposed = None

            def mutate(self, transform, **_kwargs):
                self.proposed = deepcopy(original)
                transform(self.proposed)
                return DryRunCommit()

        dry_store = DryRunStore()
        args = argparse.Namespace(
            max_packets=1,
            issue_ledger=self.issue_ledger,
            lane="primary",
            progress=Path("unit-progress.sqlite3"),
            expected_revision=12,
            expected_scheduler_revision=None,
            apply=False,
            dry_run=True,
        )
        opportunity = {
            "primary": {
                "lane": "primary",
                "cursor": "0x480000",
                "launchability": "launchable",
                "reason_code": "live-order-ready",
                "reason": "ready",
                "packet": packet,
                "work_item_id": "unit:primary",
                "conflicts": [],
            }
        }
        with (
            patch.object(
                progress_cli,
                "_precheck_scheduler_revision",
                return_value=(RoutingDocument(original), None),
            ),
            patch.object(progress_cli, "ProgressDocument", RoutingDocument),
            patch.object(progress_cli, "ProgressStore", return_value=dry_store),
            patch.object(
                progress_cli,
                "_call_contract_mixed_obligation_candidates",
                return_value=([], []),
            ),
            patch.object(
                progress_cli,
                "_call_contract_convergence_repair_candidates",
                return_value=([], []),
            ),
            patch.object(
                progress_cli,
                "_call_contract_retail_fact_candidates",
                return_value=([], []),
            ),
            patch.object(
                progress_cli,
                "_current_claim_opportunities",
                return_value=opportunity,
            ),
            patch.object(
                progress_cli,
                "_run_json_process",
                side_effect=AssertionError("compiler/LINK subprocess invoked"),
            ),
            patch.object(
                progress_cli,
                "BinaryNinjaBridge",
                side_effect=AssertionError("Binary Ninja invoked"),
            ),
            patch(
                "_recoil.commands.call_contract_verify.BinaryNinjaBridge",
                side_effect=AssertionError("call-contract Binary Ninja invoked"),
            ),
            patch(
                "subprocess.run",
                side_effect=AssertionError("compiler/preprocessor/LINK invoked"),
            ),
            patch(
                "tempfile.TemporaryDirectory",
                side_effect=AssertionError("temporary build root created"),
            ),
        ):
            payload = progress_cli.claim_current_work(args)

        self.assertFalse(payload["commit"]["applied"])
        self.assertEqual({}, original["work_items"])
        self.assertIsNotNone(dry_store.proposed)
        self.assertEqual("active", dry_store.proposed["work_items"]["unit:primary"]["state"])

    def test_arbitrary_existing_sqlite_cannot_allocate_native_git_topology(self) -> None:
        original = {
            "revision": 12,
            "migration": {},
            "work_items": {},
        }
        packet = self._packet(
            lane="primary", path="src/order.cpp", cursor="0x480000"
        )

        class RoutingDocument:
            def __init__(self, data, path=None):
                self.data = data
                self.path = path
                self.revision = int(data["revision"])

            def pipeline(self, _binary):
                return ClaimCurrentIsolationTests._pipeline()

            def collection(self, name):
                return self.data.get(name, {})

        class AppliedCommit:
            def to_dict(self):
                return {"applied": True, "previous_revision": 12, "revision": 13}

        class FixtureStore:
            def __init__(self):
                self.proposed = None

            def mutate(self, transform, **_kwargs):
                self.proposed = deepcopy(original)
                transform(self.proposed)
                return AppliedCommit()

        opportunity = {
            "primary": {
                "lane": "primary",
                "cursor": "0x480000",
                "launchability": "launchable",
                "reason_code": "live-order-ready",
                "reason": "ready",
                "packet": packet,
                "work_item_id": "unit:primary",
                "conflicts": [],
            }
        }
        topology_before = progress_cli.resolve_topology(REPO_ROOT)
        before = [
            (str(row.root), row.branch, row.head, row.prunable)
            for row in topology_before.worktrees
        ]
        with tempfile.TemporaryDirectory() as temporary:
            arbitrary_progress = Path(temporary) / "arbitrary.sqlite3"
            connection = sqlite3.connect(arbitrary_progress)
            try:
                connection.execute("CREATE TABLE fixture (id INTEGER PRIMARY KEY)")
                connection.commit()
            finally:
                connection.close()
            args = argparse.Namespace(
                max_packets=1,
                issue_ledger=self.issue_ledger,
                lane="primary",
                progress=arbitrary_progress,
                expected_revision=12,
                expected_scheduler_revision=None,
                apply=True,
                dry_run=False,
            )
            fixture_store = FixtureStore()
            with (
                patch.object(
                    progress_cli,
                    "_precheck_scheduler_revision",
                    return_value=(RoutingDocument(original), None),
                ),
                patch.object(progress_cli, "ProgressDocument", RoutingDocument),
                patch.object(progress_cli, "ProgressStore", return_value=fixture_store),
                patch.object(
                    progress_cli,
                    "_call_contract_mixed_obligation_candidates",
                    return_value=([], []),
                ),
                patch.object(
                    progress_cli,
                    "_call_contract_convergence_repair_candidates",
                    return_value=([], []),
                ),
                patch.object(
                    progress_cli,
                    "_call_contract_retail_fact_candidates",
                    return_value=([], []),
                ),
                patch.object(
                    progress_cli,
                    "_current_claim_opportunities",
                    return_value=opportunity,
                ),
                patch.object(
                    progress_cli,
                    "resolve_topology",
                    side_effect=AssertionError("arbitrary SQLite reached topology allocation"),
                ),
            ):
                payload = progress_cli.claim_current_work(args)

        topology_after = progress_cli.resolve_topology(REPO_ROOT)
        after = [
            (str(row.root), row.branch, row.head, row.prunable)
            for row in topology_after.worktrees
        ]
        self.assertEqual(before, after)
        self.assertTrue(payload["packet"]["planned"])
        self.assertFalse(payload["packet"]["handoff_visible"])
        self.assertFalse(
            payload["packet"]["progress_packet_adapter"]["runtime_authority"]
        )

    def test_retail_blocked_call_contract_lane_has_non_source_parent_action(
        self,
    ) -> None:
        pipeline = {
            "complete": False,
            "primary_lane": "call-contract",
            "phase": "authored-call-contract",
            "cursor": "0x401000",
            "authored_call_contract_convergence": {
                "mode": "retail-blocked",
                "retail_blocker_target_count": 1,
                "retail_blocker_caller_count": 1,
            },
        }
        opportunities = progress_cli._current_claim_opportunities(
            SimpleNamespace(),
            pipeline,
            progress_path=Path("unit-progress.json"),
            issue_ledger=Path("unit-issues.json"),
            lanes=("primary",),
        )
        primary = opportunities["primary"]
        self.assertEqual("blocked", primary["state"])
        self.assertEqual("retail-blocked", primary["reason_code"])
        self.assertEqual(
            "claim-retail-fact-packets",
            primary["required_parent_action"],
        )
        self.assertNotEqual("claim-convergence-repairs", primary["required_parent_action"])

    def test_removed_convergence_packet_factories_stay_absent(self) -> None:
        self.assertFalse(hasattr(convergence, "CallContractSourceClosure"))
        self.assertFalse(hasattr(convergence, "_retail_blocker_descriptor"))

    def test_packetless_repair_state_is_nonactive_and_nonblocking(self) -> None:
        from _recoil.commands.call_contract_continuation import continuation_state

        state = continuation_state()
        self.assertEqual("none", state["state"])
        self.assertFalse(state["active"])
        self.assertIsNone(state["checkpoint"])

    def test_dependent_owner_blocked_lane_never_requeues_caller_source(self) -> None:
        pipeline = {
            "complete": False,
            "primary_lane": "call-contract",
            "phase": "authored-call-contract",
            "cursor": "0x401000",
            "authored_call_contract_convergence": {
                "mode": "dependent-owner-blocked",
                "dependent_owner_blocker_target_count": 1,
                "required_parent_action": "resolve-dependent-owner-ambiguity",
            },
        }

        opportunities = progress_cli._current_claim_opportunities(
            SimpleNamespace(),
            pipeline,
            progress_path=Path("unit-progress.json"),
            issue_ledger=Path("unit-issues.json"),
            lanes=("primary",),
        )

        primary = opportunities["primary"]
        self.assertEqual("blocked", primary["state"])
        self.assertEqual("dependent-owner-blocked", primary["reason_code"])
        self.assertEqual(
            "resolve-dependent-owner-ambiguity",
            primary["required_parent_action"],
        )
        self.assertNotEqual("claim-convergence-repairs", primary["required_parent_action"])

    def test_wol_prospective_profile_replaces_broad_convergence_packet(
        self,
    ) -> None:
        target_id = progress_cli.WOL_PROFILE_MATRIX_TARGET_ID
        write_paths = list(progress_cli.WOL_PROFILE_SOURCE_HANDOFF_WRITE_PATHS)
        registered_paths = [
            "src/Battlesport/WOL.cpp",
            "src/GameZRecoil/westwoodonline/WolapiProgressDialog.cpp",
            "src/Battlesport/wol_api.h",
            "src/Battlesport/wol_api_event_sink.h",
            "src/Battlesport/wol_ref_count_and_lock.h",
            "src/Battlesport/wol_config_dialog.h",
            "src/Battlesport/wol_dialog.h",
            "src/Battlesport/wol_download.h",
        ]
        manifest = (
            "tools/vc5_verify_targets/"
            "wol_43cf90_442890_authored_order.json"
        )
        dependencies = [*registered_paths, manifest]
        scope_ids = [
            f"recoil:function:0x{0x43CF90 + index * 0x10:x}"
            for index in range(109)
        ]
        blocks = ["recoil:block:0x43cf90", "recoil:block:0x442220"]
        divergence = {
            "kind": "mismatch",
            "symbol_id": "recoil:function:0x43e1c0",
            "address": "0x43e1c0",
            "ordinal": 4,
        }
        derived_descriptor = {
            "kind": (
                "call-contract-convergence-derived-prospective-profile-descriptor"
            ),
            "contract_version": 1,
            "generation_id": "wol-profile-generation",
            "target_id": target_id,
            "packet_type": "call-contract-edit-v1",
            "routing_kind": "wol-prospective-profile-source-v1",
            "cursor": "0x43cf90",
            "body_count": 109,
            "symbol_ids": scope_ids,
            "physical_block_ids": blocks,
            "original_slice_ids": ["slice-a", "slice-b"],
            "source_edit_paths": write_paths,
            "target_source_edit_paths": write_paths,
            "registered_source_paths": registered_paths,
            "header_paths": registered_paths[2:],
            "definition_source_paths": registered_paths[:2],
            "dependency_paths": [*dependencies, "src/Battlesport/hud.cpp"],
            "first_divergence": divergence,
            "caller_divergences": [divergence],
        }
        generation = {
            "generation_id": "wol-profile-generation",
            "status": "failed-targets",
            "repair_descriptors": [],
            "dependent_owner_repair_descriptors": [],
            "dependent_owner_blocker_descriptors": [
                {
                    "target_id": target_id,
                    "generation_id": "wol-profile-generation",
                }
            ],
            "dependency_paths": dependencies,
            "dependency_signatures": [],
        }
        route = {
            "source_edit_paths": write_paths,
            "registered_source_paths": registered_paths,
            "registered_read_paths": [
                path for path in registered_paths if path not in write_paths
            ],
            "definition_source_paths": registered_paths[:2],
            "dependency_paths": dependencies,
            "scope_ids": scope_ids,
            "physical_block_ids": blocks,
            "expected_truth": (
                progress_cli.WOL_PROFILE_SOURCE_HANDOFF_EXPECTED_TRUTH
            ),
        }
        document = SimpleNamespace(
            data={},
            revision=3571,
            collection=lambda _name: {},
            _fresh_root=lambda *_args: "build/wol-profile-source-worker",
        )
        with (
            patch.object(
                progress_cli,
                "convergence_generation_state",
                return_value={"current": True, "generation": generation},
            ),
            patch.object(
                progress_cli,
                "prospective_wol_profile_convergence_route",
                return_value={
                    "state": "launchable",
                    "target_id": target_id,
                    "route": route,
                    "descriptor": derived_descriptor,
                },
            ),
            patch.object(
                progress_cli,
                "_valid_wol_profile_source_handoff_route",
                return_value=True,
            ),
            patch.object(
                progress_cli,
                "bind_work_packet_contract",
                side_effect=lambda _document, row: row,
            ),
            patch.object(
                progress_cli,
                "_candidate_active_conflicts",
                return_value=([], []),
            ),
        ):
            candidates, blockers = (
                progress_cli._call_contract_convergence_repair_candidates(
                    document,
                    progress_path=Path("unit-progress.json"),
                    issue_ledger=Path("unit-issues.json"),
                )
            )

        self.assertEqual([], blockers)
        self.assertEqual(1, len(candidates))
        work_id, work = candidates[0]
        self.assertEqual("call-contract-edit-v1", work["packet_type"])
        self.assertEqual(write_paths, work["allowed_paths"])
        self.assertEqual(route, work["prospective_profile_handoff"])
        self.assertIn("--profile-matrix", work["validation_commands"][0])
        self.assertNotIn("--all-authored-bodies", work["validation_commands"][0])
        writes = {
            row["id"]
            for row in work["resource_claims"]
            if row["kind"] == "path" and row["access"] == "write"
        }
        reads = {
            row["id"]
            for row in work["resource_claims"]
            if row["kind"] == "path" and row["access"] == "read"
        }
        self.assertEqual(set(write_paths), writes)
        self.assertTrue(set(route["registered_read_paths"]).issubset(reads))
        self.assertNotIn("src/Battlesport/hud.cpp", writes)

        work["reservation"] = {
            "id": "reservation:wol-profile",
            "state": "active",
        }
        work["packet_contract_version"] = 4
        work["progress_packet_adapter"] = "native-git-v1-planned"
        self.assertEqual(write_paths, work["allowed_paths"])
        self.assertEqual(route, work["prospective_profile_handoff"])
        self.assertTrue(work["nonaccepting"])
        self.assertFalse(work["acceptance_eligible"])
        self.assertFalse(work["manifest_mutation_allowed"])

    def test_malformed_wol_prospective_profile_suppresses_broad_fallback(
        self,
    ) -> None:
        target_id = progress_cli.WOL_PROFILE_MATRIX_TARGET_ID
        generation = {
            "generation_id": "wol-profile-generation",
            "status": "failed-targets",
            "repair_descriptors": [],
            "dependent_owner_repair_descriptors": [],
            "dependent_owner_blocker_descriptors": [
                {
                    "target_id": target_id,
                    "generation_id": "wol-profile-generation",
                    "blocker_count": 0,
                }
            ],
        }
        document = SimpleNamespace(data={}, collection=lambda _name: {})
        with (
            patch.object(
                progress_cli,
                "convergence_generation_state",
                return_value={"current": True, "generation": generation},
            ),
            patch.object(
                progress_cli,
                "prospective_wol_profile_convergence_route",
                return_value={
                    "state": "blocked",
                    "target_id": target_id,
                    "reason_code": "incomplete-prospective-profile-proof",
                },
            ),
        ):
            candidates, blockers = (
                progress_cli._call_contract_convergence_repair_candidates(
                    document,
                    progress_path=Path("unit-progress.json"),
                    issue_ledger=Path("unit-issues.json"),
                )
            )
        self.assertEqual([], candidates)
        self.assertEqual(
            "incomplete-prospective-profile-proof",
            blockers[0]["reason_code"],
        )




    def test_dependent_header_blocked_lane_retains_truthful_parent_action(
        self,
    ) -> None:
        pipeline = {
            "complete": False,
            "primary_lane": "call-contract",
            "phase": "authored-call-contract",
            "cursor": "0x401000",
            "authored_call_contract_convergence": {
                "mode": "dependent-header-blocked",
                "dependent_header_blocker_target_count": 1,
            },
        }
        opportunities = progress_cli._current_claim_opportunities(
            SimpleNamespace(),
            pipeline,
            progress_path=Path("unit-progress.json"),
            issue_ledger=Path("unit-issues.json"),
            lanes=("primary",),
        )
        self.assertEqual(
            "resolve-dependent-header-routing",
            opportunities["primary"]["required_parent_action"],
        )

    def test_zero_launchable_repairs_report_exact_blocker_and_keep_byte_lane(
        self,
    ) -> None:
        target_id = (
            "recoil:vc5-target:recoil_app_42de10_436630_authored_order"
        )
        pipeline = {
            "complete": False,
            "primary_lane": "call-contract",
            "phase": "authored-call-contract",
            "cursor": "0x42eed0",
            "parallel_authored_byte_cursor": "0x401000",
            "parallel_authored_object_byte_cursor": "",
            "authored_byte_lane": {"state": "ready"},
            "authored_object_byte_lane": {"state": "caught-up"},
            "authored_call_contract_convergence": {
                "mode": "repairing/failed-targets",
                "dependent_owner_repair_target_count": 1,
            },
        }
        document = SimpleNamespace(
            path=Path("unit-progress.json"),
            revision=4355,
            collection=lambda _name: {},
        )
        blocker = {
            "target_id": target_id,
            "reason_code": "target-wide-verifier-blocked-before-repair",
            "launchability": {
                "state": "blocked",
                "reason_code": "earlier-target-wide-verifier-blocker",
            },
        }
        authored_packet = self._packet(
            lane="authored",
            path="src/independent-byte.cpp",
            cursor="0x401000",
        )
        with (
            patch.object(
                progress_cli,
                "_call_contract_convergence_repair_candidates",
                return_value=([], [blocker]),
            ),
            patch.object(
                progress_cli,
                "_call_contract_retail_fact_candidates",
                return_value=([], []),
            ),
            patch.object(
                progress_cli,
                "_byte_lane_preflight",
                return_value={
                    "passed": True,
                    "reason_code": "live-byte-preflight-ready",
                    "reason": "ready",
                },
            ),
            patch.object(
                progress_cli,
                "_byte_claim_candidate",
                return_value=("unit:authored", authored_packet),
            ),
            patch.object(
                progress_cli,
                "_candidate_active_conflicts",
                return_value=([], []),
            ),
        ):
            described = progress_cli.describe_current_claim_opportunities(
                document,
                pipeline,
                issue_ledger=self.issue_ledger,
            )

        primary = described["cursor_launchability"]["primary"]
        self.assertEqual("blocked", primary["launchability"])
        self.assertEqual(
            "target-wide-verifier-blocked-before-repair",
            primary["reason_code"],
        )
        self.assertEqual(
            "resolve-target-wide-verifier-blocker",
            primary["required_parent_action"],
        )
        self.assertEqual([blocker], primary["repair_blockers"])
        self.assertNotEqual(
            "claim-convergence-repairs",
            primary["required_parent_action"],
        )
        selected_lanes = [
            row["lane"]
            for row in described["launch_plan"]
            if row["selected_opportunity"]
        ]
        self.assertEqual(["authored"], selected_lanes)

    def test_definition_owner_repair_remains_primary_reachable_with_retail_debt(
        self,
    ) -> None:
        pipeline = {
            "complete": False,
            "primary_lane": "call-contract",
            "phase": "authored-call-contract",
            "cursor": "0x44e630",
            "parallel_authored_byte_cursor": "",
            "parallel_authored_object_byte_cursor": "",
            "authored_byte_lane": {"state": "caught-up"},
            "authored_object_byte_lane": {"state": "caught-up"},
            "authored_call_contract_convergence": {
                "mode": "repairing/failed-targets",
                "repair_descriptor_count": 1,
                "retail_blocker_target_count": 1,
            },
        }
        packet = {
            "state": "ready",
            "packet_type": "call-contract-converge-edit-v1",
            "phase": "authored-call-contract",
            "lane": "call-contract",
            "cursor": "0x44e630",
            "target_id": (
                "recoil:vc5-target:list_44e630_44f7a0_authored_order"
            ),
            "allowed_paths": ["src/GameZRecoil/zClass/List.c"],
            "validation_commands": [
                "python tools/recoil.py verify call-contract "
                "--target recoil:vc5-target:list_44e630_44f7a0_authored_order "
                "--all-authored-bodies --progress unit-progress.json "
                "--build-root build/list-repair --json"
            ],
            "resource_claims": [
                {
                    "kind": "path",
                    "id": "src/GameZRecoil/zClass/List.c",
                    "access": "write",
                },
                {
                    "kind": "path",
                    "id": "src/GameZRecoil/include/zClass.h",
                    "access": "read",
                },
            ],
        }
        document = SimpleNamespace(
            path=Path("unit-progress.json"),
            revision=20,
            collection=lambda _name: {},
        )
        with patch(
            "_recoil.commands.progress_cli._call_contract_convergence_repair_candidates",
            return_value=([("unit:list-repair", packet)], []),
        ):
            described = progress_cli.describe_current_claim_opportunities(
                document, pipeline, issue_ledger=self.issue_ledger
            )

        primary = described["cursor_launchability"]["primary"]
        self.assertEqual("launchable", primary["launchability"])
        self.assertEqual(
            "call-contract-convergence-repairs-ready",
            primary["reason_code"],
        )
        self.assertEqual("claim-and-launch", primary["required_parent_action"])
        self.assertEqual(
            ["primary"],
            [
                row["lane"]
                for row in described["launch_plan"]
                if row["selected_opportunity"]
            ],
        )

    def test_primary_only_claim_never_constructs_or_reserves_byte_packet(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            progress_path = root / "progress.json"
            progress_path.write_text(json.dumps(empty_progress_document()) + "\n", encoding="utf-8")
            args = argparse.Namespace(
                progress=progress_path,
                issue_ledger=root / "issues.json",
                lane="primary",
                max_packets=3,
                expected_revision=0,
                dry_run=False,
                apply=True,
            )
            order_id = "unit:primary"
            order_packet = self._packet(
                lane="primary", path="src/order.cpp", cursor="0x480000"
            )
            with (
                patch.object(
                    ProgressDocument,
                    "pipeline",
                    return_value=self._pipeline(),
                ),
                patch(
                    "_recoil.commands.progress_cli._order_claim_candidate",
                    return_value=(order_id, order_packet),
                ),
                patch(
                    "_recoil.commands.progress_cli._byte_lane_preflight",
                    side_effect=AssertionError("primary claim reached byte preflight"),
                ) as byte_preflight,
                patch(
                    "_recoil.commands.progress_cli._candidate_active_conflicts",
                    return_value=([], []),
                ),
            ):
                result = progress_cli.claim_current_work(args)

            stored = json.loads(progress_path.read_text(encoding="utf-8"))
            self.assertEqual([order_id], list(stored["work_items"]))
            self.assertEqual("active", stored["work_items"][order_id]["reservation"]["state"])
            self.assertEqual(["primary"], [packet["lane"] for packet in result["packets"]])
            expected_provenance = {
                "schema_version": 1,
                "command": "progress work claim-current",
                "requested_lane": "primary",
                "selected_lane": "primary",
                "max_packets": 3,
            }
            self.assertEqual(
                expected_provenance,
                stored["work_items"][order_id]["claim_provenance"],
            )
            self.assertEqual(expected_provenance, result["packets"][0]["claim_provenance"])
            byte_preflight.assert_not_called()

    def test_scheduler_domain_claim_does_not_rewrite_convergence_migration(self) -> None:
        data = empty_progress_document()
        data["migration"][convergence.CONVERGENCE_MIGRATION_KEY] = {
            "contract_version": 1,
            "generation_id": "historical-byte-exact-fixture",
            "opaque": {"preserve": [1, 2, 3]},
        }
        before_migration = deepcopy(data["migration"])
        scheduler_document = ProgressDocument(
            deepcopy(data),
            path=Path("unit-progress.sqlite3"),
        )
        args = argparse.Namespace(
            progress=Path("unit-progress.sqlite3"),
            issue_ledger=self.issue_ledger,
            lane="primary",
            max_packets=1,
            expected_revision=None,
            expected_scheduler_revision=17,
            dry_run=False,
            apply=True,
        )
        order_id = "unit:scheduler-domain-primary"
        order_packet = self._packet(
            lane="primary", path="src/order.cpp", cursor="0x480000"
        )
        observed: dict[str, object] = {}

        class _Commit:
            @staticmethod
            def to_dict() -> dict[str, object]:
                return {"applied": True, "revision": 1}

        def scoped_commit(**kwargs: object):
            proposed = deepcopy(scheduler_document.data)
            kwargs["transform"](proposed)
            observed["migration"] = deepcopy(proposed["migration"])
            observed["work_items"] = deepcopy(proposed["work_items"])
            self.assertEqual({"scheduler": 17}, kwargs["expected_domains"])
            self.assertEqual({"scheduler"}, set(kwargs["increment_domains"] or []))
            return _Commit()

        with (
            patch.object(
                progress_cli,
                "_precheck_scheduler_revision",
                return_value=(scheduler_document, {"scheduler": 17}),
            ),
            patch.object(
                ProgressDocument,
                "pipeline",
                return_value=self._pipeline(),
            ),
            patch.object(
                progress_cli,
                "_call_contract_mixed_obligation_candidates",
                return_value=([], []),
            ),
            patch.object(
                progress_cli,
                "_call_contract_convergence_repair_candidates",
                return_value=([], []),
            ),
            patch.object(
                progress_cli,
                "_call_contract_retail_fact_candidates",
                return_value=([], []),
            ),
            patch.object(
                progress_cli,
                "_order_claim_candidate",
                return_value=(order_id, order_packet),
            ),
            patch.object(
                progress_cli,
                "_candidate_active_conflicts",
                return_value=([], []),
            ),
            patch.object(
                progress_cli,
                "_call_contract_scoped_patch_commit",
                side_effect=scoped_commit,
            ),
        ):
            result = progress_cli.claim_current_work(args)

        self.assertEqual(before_migration, observed["migration"])
        self.assertEqual([order_id], list(observed["work_items"]))
        self.assertFalse(result["convergence_generation_carried"])

    def test_explicit_all_claim_keeps_priority_and_resource_conflict_checks(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            progress_path = root / "progress.json"
            progress_path.write_text(json.dumps(empty_progress_document()) + "\n", encoding="utf-8")
            args = argparse.Namespace(
                progress=progress_path,
                issue_ledger=root / "issues.json",
                lane="all",
                max_packets=3,
                expected_revision=0,
                dry_run=False,
                apply=True,
            )
            packets = {
                "primary": self._packet(
                    lane="primary", path="src/order.cpp", cursor="0x480000"
                ),
                "authored": self._packet(
                    lane="authored", path="src/shared-byte.cpp", cursor="0x401000"
                ),
                "object": self._packet(
                    lane="object", path="src/shared-byte.cpp", cursor="0x402000"
                ),
            }
            observed_lanes: list[tuple[str, ...]] = []

            def opportunities(
                _document: object,
                _pipeline: object,
                **kwargs: object,
            ) -> dict[str, dict[str, object]]:
                observed_lanes.append(tuple(str(lane) for lane in kwargs["lanes"]))
                return {
                    lane: {
                        "lane": lane,
                        "cursor": packet["cursor"],
                        "launchability": "launchable",
                        "reason_code": "ready",
                        "reason": "ready",
                        "conflicts": [],
                        "work_item_id": f"unit:{lane}",
                        "packet": packet,
                    }
                    for lane, packet in packets.items()
                }

            with (
                patch.object(
                    ProgressDocument,
                    "pipeline",
                    return_value=self._pipeline(),
                ),
                patch(
                    "_recoil.commands.progress_cli._current_claim_opportunities",
                    side_effect=opportunities,
                ),
            ):
                result = progress_cli.claim_current_work(args)

            stored = json.loads(progress_path.read_text(encoding="utf-8"))
            self.assertEqual([("primary", "authored", "object")], observed_lanes)
            self.assertEqual(
                {"unit:primary", "unit:authored"},
                set(stored["work_items"]),
            )
            self.assertEqual(
                ["primary", "authored"],
                [packet["lane"] for packet in result["packets"]],
            )
            self.assertEqual(
                [
                    {
                        "schema_version": 1,
                        "command": "progress work claim-current",
                        "requested_lane": "all",
                        "selected_lane": lane,
                        "max_packets": 3,
                    }
                    for lane in ("primary", "authored")
                ],
                [packet["claim_provenance"] for packet in result["packets"]],
            )
            self.assertEqual("object", result["skipped"][0]["lane"])
            self.assertEqual(
                "higher-priority-resource-conflict",
                result["skipped"][0]["reason_code"],
            )

    def test_explicit_single_byte_lane_claims_select_only_the_requested_lane(self) -> None:
        for lane, cursor in (("authored", "0x401000"), ("object", "0x402000")):
            with self.subTest(lane=lane), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                progress_path = root / "progress.json"
                progress_path.write_text(
                    json.dumps(empty_progress_document()) + "\n",
                    encoding="utf-8",
                )
                args = argparse.Namespace(
                    progress=progress_path,
                    issue_ledger=root / "issues.json",
                    lane=lane,
                    max_packets=2,
                    expected_revision=0,
                    dry_run=False,
                    apply=True,
                )
                work_id = f"unit:{lane}"
                packet = self._packet(
                    lane=lane,
                    path=f"src/{lane}.cpp",
                    cursor=cursor,
                )
                observed_lanes: list[tuple[str, ...]] = []

                def opportunities(
                    _document: object,
                    _pipeline: object,
                    **kwargs: object,
                ) -> dict[str, dict[str, object]]:
                    observed_lanes.append(
                        tuple(str(value) for value in kwargs["lanes"])
                    )
                    return {
                        lane: {
                            "lane": lane,
                            "cursor": cursor,
                            "launchability": "launchable",
                            "reason_code": "ready",
                            "reason": "ready",
                            "conflicts": [],
                            "work_item_id": work_id,
                            "packet": packet,
                        }
                    }

                with (
                    patch.object(
                        ProgressDocument,
                        "pipeline",
                        return_value=self._pipeline(),
                    ),
                    patch(
                        "_recoil.commands.progress_cli._current_claim_opportunities",
                        side_effect=opportunities,
                    ),
                ):
                    result = progress_cli.claim_current_work(args)

                stored = json.loads(progress_path.read_text(encoding="utf-8"))
                self.assertEqual([(lane,)], observed_lanes)
                self.assertEqual([work_id], list(stored["work_items"]))
                self.assertEqual([lane], [item["lane"] for item in result["packets"]])
                self.assertEqual(
                    {
                        "schema_version": 1,
                        "command": "progress work claim-current",
                        "requested_lane": lane,
                        "selected_lane": lane,
                        "max_packets": 2,
                    },
                    result["packets"][0]["claim_provenance"],
                )

    def test_central_creation_path_rejects_non_claim_authority_and_spoofed_provenance(
        self,
    ) -> None:
        packet = self._packet(
            lane="primary",
            path="src/order.cpp",
            cursor="0x480000",
        )
        cases = (
            (
                {**packet},
                {
                    "command": "progress next",
                    "requested_lane": "primary",
                    "selected_lane": "primary",
                },
                "authorized only",
            ),
            (
                {**packet, "claim_provenance": {"schema_version": 1}},
                {
                    "command": "progress work claim-current",
                    "requested_lane": "primary",
                    "selected_lane": "primary",
                },
                "must not supply",
            ),
            (
                {**packet},
                {
                    "command": "progress work claim-current",
                    "requested_lane": "primary",
                    "selected_lane": "authored",
                },
                "selected lane must equal",
            ),
        )
        for work, authority, message in cases:
            with self.subTest(message=message):
                data = empty_progress_document()
                with self.assertRaisesRegex(ProgressError, message):
                    create_and_reserve_claim_current_work_item(
                        data,
                        work_id="unit:primary",
                        work=work,
                        max_packets=3,
                        **authority,
                    )
                self.assertEqual({}, data["work_items"])

    def test_repair_continuation_creation_preserves_terminal_scope_and_is_one_hop(
        self,
    ) -> None:
        data = empty_progress_document()
        predecessor_id = "unit:returned-tool-blocked"
        inherited = {
            "target_id": "unit:target",
            "cursor": "0x480000",
            "block_id": "unit:block",
            "covered_block_ids": ["unit:block"],
            "scope_ids": ["unit:symbol"],
            "target_ids": ["unit:target"],
            "original_slice_ids": ["unit:slice"],
            "allowed_paths": ["src/unit.cpp"],
            "source_edit_paths": ["src/unit.cpp"],
            "definition_source_paths": ["src/unit.cpp"],
            "dependency_paths": ["src/unit.cpp", "src/unit.h"],
        }
        data["work_items"][predecessor_id] = {
            **deepcopy(inherited),
            "state": "returned-tool-blocked",
        }
        checkpoint_id = "recoil:call-contract-repair-continuation:r1:0001"
        child_id = "recoil:work:call-contract-repair-continuation-0001-r1"
        child = {
            **deepcopy(inherited),
            "binary": "recoil",
            "handoff_role": "recoil_source_worker",
            "packet_type": "call-contract-repair-continuation-edit-v2",
            "state": "ready",
            "phase": "authored-call-contract",
            "lane": "primary",
            "nonaccepting": True,
            "acceptance_eligible": False,
            "candidate_expected_truth": False,
            "full_convergence_required": True,
            "continuation_provenance": {
                "schema_version": 2,
                "command": "progress work claim-current",
                "checkpoint_id": checkpoint_id,
                "descriptor_id": "unit:descriptor",
                "predecessor_work_item_id": predecessor_id,
                "producer_work_item_id": "unit:producer",
                "hop": 1,
                "max_hops": 1,
            },
            "route_descriptor": {
                "schema": "call-contract-repair-route-descriptor-v1",
                "descriptor_id": "unit:descriptor",
                "predecessor_work_item_id": predecessor_id,
                "producer_work_item_id": "unit:producer",
                "hop": 1,
                "max_hops": 1,
                "fresh_parent_acceptance_required": True,
                "candidate_expected_truth": False,
                "controlling_declaration_path": "src/unit.h",
                "controlling_definition_path": "src/definition.cpp",
                "write_paths": ["src/unit.cpp", "src/unit.h", "src/definition.cpp"],
            },
            "allowed_paths": ["src/unit.cpp", "src/unit.h", "src/definition.cpp"],
            "source_edit_paths": ["src/unit.cpp", "src/unit.h", "src/definition.cpp"],
            "validation_commands": ["python tools/recoil.py verify call-contract --target unit:target"],
            "resource_claims": [
                {"kind": "path", "id": "src/unit.cpp", "access": "write"},
                {"kind": "path", "id": "src/unit.h", "access": "write"},
                {"kind": "path", "id": "src/definition.cpp", "access": "write"},
            ],
        }

        result = create_and_reserve_repair_continuation_work_item(
            data,
            work_id=child_id,
            work=child,
            predecessor_work_item_id=predecessor_id,
            checkpoint_id=checkpoint_id,
        )

        self.assertEqual("active", result["reservation"]["state"])
        self.assertEqual("returned-tool-blocked", data["work_items"][predecessor_id]["state"])
        self.assertNotIn("claim_provenance", data["work_items"][child_id])
        self.assertEqual(1, data["work_items"][child_id]["continuation_provenance"]["hop"])

        broadened = deepcopy(child)
        broadened["allowed_paths"] = ["src/unit.cpp", "src/other.cpp"]
        with self.assertRaisesRegex(ProgressError, "write closure differs"):
            create_and_reserve_repair_continuation_work_item(
                data,
                work_id="unit:broadened",
                work=broadened,
                predecessor_work_item_id=predecessor_id,
                checkpoint_id=checkpoint_id,
            )

        with self.assertRaisesRegex(ProgressError, "only one repair continuation"):
            create_and_reserve_repair_continuation_work_item(
                data,
                work_id="unit:duplicate",
                work=child,
                predecessor_work_item_id=predecessor_id,
                checkpoint_id=checkpoint_id,
            )

    def test_return_and_close_remove_the_selected_terminal_work_item(self) -> None:
        for outcome in ("returned", "closed"):
            with self.subTest(outcome=outcome), tempfile.TemporaryDirectory() as temporary:
                progress_path = Path(temporary) / "progress.json"
                data = empty_progress_document()
                data["work_items"]["unit:primary"] = {
                    "state": "active",
                    "phase": "authored-function-order",
                    "lane": "primary",
                    "reservation": {
                        "id": "unit:primary:attempt:1",
                        "state": "active",
                    },
                }
                progress_path.write_text(json.dumps(data) + "\n", encoding="utf-8")

                stdout, stderr = io.StringIO(), io.StringIO()
                with redirect_stdout(stdout), redirect_stderr(stderr):
                    rc = progress_cli.main(
                        [
                            "work",
                            "close",
                            "unit:primary",
                            "--outcome",
                            outcome,
                            "--progress",
                            str(progress_path),
                            "--expected-revision",
                            "0",
                            "--apply",
                            "--json",
                        ]
                    )

                self.assertEqual(0, rc, stderr.getvalue())
                stored = json.loads(progress_path.read_text(encoding="utf-8"))
                self.assertEqual({}, stored["work_items"])
                self.assertFalse(
                    any(
                        work.get("lane") in {"authored", "object"}
                        for work in stored["work_items"].values()
                    )
                )

    def test_non_call_contract_tool_blocked_return_accepts_any_real_open_issue(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            progress_path = Path(temporary) / "progress.json"
            data = empty_progress_document()
            data["work_items"]["unit:byte"] = {
                "state": "active",
                "packet_type": "byte-edit-v1",
                "phase": "authored-call-contract",
                "lane": "authored",
                "byte_lane": "authored",
                "cursor": "0x401000",
                "block_id": "recoil:block:0x401000",
                "covered_block_ids": ["recoil:block:0x401000"],
                "scope_ids": ["recoil:function:0x401000"],
                "target_ids": ["recoil:vc5-target:unit"],
                "reservation": {
                    "id": "unit:byte:attempt:1",
                    "state": "active",
                },
            }
            progress_path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            issue_ledger = {
                "revision": 31,
                "issues": [
                    {
                        "id": "WSI-20260830-003",
                        "status": "open",
                    }
                ],
            }
            fake_store = SimpleNamespace(load=lambda: issue_ledger)
            stdout, stderr = io.StringIO(), io.StringIO()
            with (
                patch.object(workspace_issues, "issue_store", return_value=fake_store),
                redirect_stdout(stdout),
                redirect_stderr(stderr),
            ):
                rc = progress_cli.main(
                    [
                        "work",
                        "close",
                        "unit:byte",
                        "--outcome",
                        "returned-tool-blocked",
                        "--linked-tool-issue",
                        "WSI-20260830-003",
                        "--progress",
                        str(progress_path),
                        "--issue-ledger",
                        str(Path(temporary) / "issues.sqlite3"),
                        "--expected-revision",
                        "0",
                        "--apply",
                        "--json",
                    ]
                )

            self.assertEqual(0, rc, stderr.getvalue())
            stored = json.loads(progress_path.read_text(encoding="utf-8"))
            retained = stored["work_items"]["unit:byte"]
            self.assertEqual("returned-tool-blocked", retained["state"])
            provenance = retained[progress_cli.TOOL_BLOCKED_PROVENANCE_FIELD]
            self.assertEqual("WSI-20260830-003", provenance["linked_issue_id"])
            self.assertEqual("byte-edit-v1", provenance["packet_type"])
            self.assertNotIn(progress_cli.RETURN_PROVENANCE_FIELD, retained)
            result = json.loads(stdout.getvalue())
            self.assertFalse(result["call_contract_continuation"])

    def test_call_contract_continuation_rejects_arbitrary_linked_issue(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            progress_path = Path(temporary) / "progress.json"
            data = empty_progress_document()
            data["work_items"]["unit:call-contract"] = {
                "state": "active",
                "packet_type": "call-contract-edit-v1",
                "phase": "authored-call-contract",
                "lane": "primary",
                "target_id": "recoil:vc5-target:unit",
                "reservation": {
                    "id": "unit:call-contract:attempt:1",
                    "state": "active",
                },
            }
            progress_path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            stderr = io.StringIO()
            with redirect_stdout(io.StringIO()), redirect_stderr(stderr):
                rc = progress_cli.main(
                    [
                        "work",
                        "close",
                        "unit:call-contract",
                        "--outcome",
                        "returned-tool-blocked",
                        "--linked-tool-issue",
                        "WSI-20260830-003",
                        "--progress",
                        str(progress_path),
                        "--expected-revision",
                        "0",
                        "--apply",
                        "--json",
                    ]
                )

            self.assertEqual(2, rc)
            self.assertIn("governed only by WSI-20260809-007", stderr.getvalue())
            stored = json.loads(progress_path.read_text(encoding="utf-8"))
            self.assertEqual("active", stored["work_items"]["unit:call-contract"]["state"])


UTIL_SYMBOL_INTERVALS = (
    ("0x437e60", "0x437ea0"),
    ("0x437ea0", "0x437ef0"),
    ("0x437ef0", "0x437fe4"),
    ("0x437fe4", "0x437ff0"),
    ("0x437ff0", "0x438000"),
    ("0x438000", "0x438020"),
    ("0x438020", "0x438180"),
    ("0x438180", "0x4381d0"),
    ("0x4381d0", "0x438350"),
    ("0x438350", "0x4383e0"),
    ("0x4383e0", "0x438430"),
    ("0x438430", "0x4384e0"),
    ("0x4384e0", "0x438540"),
    ("0x438540", "0x4385a0"),
    ("0x4385a0", "0x4385f0"),
    ("0x4385f0", "0x438630"),
    ("0x438630", "0x438660"),
    ("0x438660", "0x438690"),
    ("0x438690", "0x4386c0"),
    ("0x4386c0", "0x438920"),
    ("0x438920", "0x438980"),
)
UTIL_OLD_SEMANTIC_INTERVALS = (
    ("0x437e60", "0x437ef0", "semantic:zclass-node-recursive-helpers"),
    ("0x437ef0", "0x437fe4", "semantic:zvideo-software-mode-hotkey-helper"),
    ("0x437fe4", "0x438350", "semantic:zclass-object3d-model-ref-lerp-queue"),
    ("0x438350", "0x4383e0", "semantic:hud-ui-message-box-helper"),
    ("0x4383e0", "0x438540", "semantic:zsavegame-state-list-allocation-cleanup"),
    ("0x438540", "0x438920", "semantic:zsavegame-modal-sfx-helpers"),
    ("0x438920", "0x438980", "semantic:hud-sensor-track-list-add"),
)
UTIL_REPLACEMENT_BLOCKS = (
    ("0x437e60", "0x437ef0", "src/GameZRecoil/zClass/Class.c"),
    ("0x437ef0", "0x437ff0", "src/GameZRecoil/zVideo/zvid_main.c"),
    ("0x437ff0", "0x438350", "src/GameZRecoil/zClass/Object3d.c"),
    ("0x438350", "0x438540", "src/Battlesport/util.cpp"),
    ("0x438540", "0x438920", "src/Battlesport/player.cpp"),
    ("0x438920", "0x438980", "src/GameZRecoil/zUI/zui.cpp"),
)
UTIL_REPLACEMENT_SEMANTIC_INTERVALS = (
    ("0x437e60", "0x437ef0", "semantic:zclass-node-recursive-helpers"),
    ("0x437ef0", "0x437fe4", "semantic:zvideo-software-mode-hotkey-helper"),
    ("0x437fe4", "0x437ff0", "semantic:zclass-object3d-model-ref-lerp-queue"),
    ("0x437ff0", "0x438350", "semantic:zclass-object3d-model-ref-lerp-queue"),
    ("0x438350", "0x4383e0", "semantic:hud-ui-message-box-helper"),
    ("0x4383e0", "0x438540", "semantic:zsavegame-state-list-allocation-cleanup"),
    ("0x438540", "0x438920", "semantic:zsavegame-modal-sfx-helpers"),
    ("0x438920", "0x438980", "semantic:hud-sensor-track-list-add"),
)


def _pending_order() -> dict[str, object]:
    return {
        "authored": {name: state_record() for name in AUTHORED_ORDER_DIMENSIONS},
        "full": {name: state_record() for name in FULL_ORDER_DIMENSIONS},
    }


def _accepted_order_group(dimensions: tuple[str, ...]) -> dict[str, object]:
    return {
        name: state_record(
            result="passed",
            disposition="accepted",
            freshness="current",
            evidence_ids=["recoil:evidence:fixture"],
            gating=True,
            validation_mode="live",
        )
        for name in dimensions
    }


def _interval_contains(start: str, end: str, item_start: str, item_end: str) -> bool:
    return int(start, 16) <= int(item_start, 16) and int(item_end, 16) <= int(end, 16)


def _semantic_id(start: str, end: str) -> str:
    return f"recoil:semantic:{start}-{end}"


def util_physical_block_replace_fixture() -> tuple[dict[str, object], dict[str, object]]:
    data = empty_progress_document()
    data["revision"] = 845
    data["binaries"]["recoil"] = {
        "binary": "recoil",
        "primary_scheduler": True,
        "text": {"start": "0x437e60", "end_exclusive": "0x438990"},
    }
    old_block_id = "recoil:block:0x437e60"
    old_span_ids = [_semantic_id(start, end) for start, end, _source in UTIL_OLD_SEMANTIC_INTERVALS]
    symbol_ids = [f"recoil:function:{start}" for start, _end in UTIL_SYMBOL_INTERVALS]
    data["physical_blocks"][old_block_id] = {
        "accepted_order_facts": None,
        "agent_source_path": "src/Battlesport/util.cpp",
        "binary": "recoil",
        "contribution_ids": symbol_ids,
        "contribution_kind": "authored",
        "end_exclusive": "0x438980",
        "first_external_callers": [],
        "mapping": {
            "confidence": "provisional mixed no-literal block",
            "evidence_ids": [],
            "file_literal": None,
            "literal_xrefs": [],
            "state": "unresolved",
            "status": "mapped-no-literal-mixed",
        },
        "order": _pending_order(),
        "order_diagnostic": {},
        "order_diagnostics": {},
        "order_targets": {"linked": "", "object": ""},
        "original_source_path": None,
        "provisional_original_path": "src/Battlesport/util.cpp",
        "row_kind": "physical-source-block",
        "semantic_span_ids": old_span_ids,
        "source_path": "src/Battlesport/util.cpp",
        "source_shape_inputs": [],
        "start": "0x437e60",
    }
    for start, end, source_path in UTIL_OLD_SEMANTIC_INTERVALS:
        span_id = _semantic_id(start, end)
        span_symbol_ids = [
            f"recoil:function:{symbol_start}"
            for symbol_start, symbol_end in UTIL_SYMBOL_INTERVALS
            if _interval_contains(start, end, symbol_start, symbol_end)
        ]
        data["semantic_spans"][span_id] = {
            "binary": "recoil",
            "confidence": "current Util semantic fixture",
            "end_exclusive": end,
            "evidence_ids": [],
            "navigation_aliases": [],
            "physical_block_id": old_block_id,
            "source_path": source_path,
            "start": start,
            "status": "semantic-source-unresolved",
            "symbol_ids": span_symbol_ids,
        }
    for start, end in UTIL_SYMBOL_INTERVALS:
        symbol_id = f"recoil:function:{start}"
        old_span_id = next(
            _semantic_id(span_start, span_end)
            for span_start, span_end, _source in UTIL_OLD_SEMANTIC_INTERVALS
            if _interval_contains(span_start, span_end, start, end)
        )
        row: dict[str, object] = {
            "accepted_byte_facts": None,
            "accepted_order_facts": None,
            "address": start,
            "binary": "recoil",
            "binary_state": {},
            "end_exclusive": end,
            "kind": "function",
            "navigation_name": f"UtilFixture_{start}",
            "physical_block_id": old_block_id,
            "pipeline_class": "authored",
            "semantic_span_ids": [old_span_id],
        }
        if start == "0x437fe4":
            row["pipeline_class"] = "non-authored"
            row["authored_order_role"] = "non-authored"
        elif start == "0x437ff0":
            row["pipeline_class"] = "non-authored"
            row["authored_order_role"] = "compiler-generated-thunk"
        data["symbols"][symbol_id] = row

    successor_id = "recoil:function:0x438980"
    data["physical_blocks"]["recoil:block:0x438980"] = {
        "agent_source_path": "src/Battlesport/version.cpp",
        "binary": "recoil",
        "contribution_ids": [successor_id],
        "end_exclusive": "0x438990",
        "mapping": {"state": "unresolved", "status": "fixture"},
        "order": _pending_order(),
        "order_targets": {"linked": "", "object": ""},
        "original_source_path": None,
        "provisional_original_path": None,
        "semantic_span_ids": [],
        "source_path": "src/Battlesport/version.cpp",
        "start": "0x438980",
    }
    data["symbols"][successor_id] = {
        "address": "0x438980",
        "binary": "recoil",
        "binary_state": {},
        "end_exclusive": "0x438990",
        "kind": "function",
        "physical_block_id": "recoil:block:0x438980",
        "pipeline_class": "authored",
        "semantic_span_ids": [],
    }

    replacement_spans: list[dict[str, object]] = []
    for start, end, source_path in UTIL_REPLACEMENT_SEMANTIC_INTERVALS:
        block_id = next(
            f"recoil:block:{block_start}"
            for block_start, block_end, _path in UTIL_REPLACEMENT_BLOCKS
            if _interval_contains(block_start, block_end, start, end)
        )
        replacement_spans.append(
            {
                "id": _semantic_id(start, end),
                "start": start,
                "end_exclusive": end,
                "physical_block_id": block_id,
                "source_path": source_path,
                "status": "semantic-source-unresolved",
                "confidence": "current Util semantic fixture",
                "symbol_ids": [
                    f"recoil:function:{symbol_start}"
                    for symbol_start, symbol_end in UTIL_SYMBOL_INTERVALS
                    if _interval_contains(start, end, symbol_start, symbol_end)
                ],
            }
        )
    replacement_blocks: list[dict[str, object]] = []
    for start, end, source_path in UTIL_REPLACEMENT_BLOCKS:
        replacement_blocks.append(
            {
                "id": f"recoil:block:{start}",
                "start": start,
                "end_exclusive": end,
                "source_path": source_path,
                "agent_source_path": source_path,
                "original_source_path": None,
                "provisional_original_path": None,
                "mapping_state": "unresolved",
                "mapping_status": "reviewed-current-compile-provider-original-unresolved",
                "mapping_confidence": "current compile provider only; no original source-path acceptance",
                "contribution_ids": [
                    f"recoil:function:{symbol_start}"
                    for symbol_start, symbol_end in UTIL_SYMBOL_INTERVALS
                    if _interval_contains(start, end, symbol_start, symbol_end)
                ],
                "semantic_span_ids": [
                    str(span["id"])
                    for span in replacement_spans
                    if span["physical_block_id"] == f"recoil:block:{start}"
                ],
            }
        )
    payload: dict[str, object] = {
        "schema": "recoil-physical-block-replace-v1",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": "Current Util fixture requires the reviewed six-provider partition and queue seam split.",
        "binary": "recoil",
        "current_block": {
            "id": old_block_id,
            "start": "0x437e60",
            "end_exclusive": "0x438980",
            "source_path": "src/Battlesport/util.cpp",
            "agent_source_path": "src/Battlesport/util.cpp",
            "original_source_path": None,
            "provisional_original_path": "src/Battlesport/util.cpp",
            "mapping_state": "unresolved",
            "mapping_status": "mapped-no-literal-mixed",
            "contribution_ids": symbol_ids,
            "semantic_span_ids": old_span_ids,
        },
        "replacement_blocks": replacement_blocks,
        "replacement_semantic_spans": replacement_spans,
    }
    return data, payload


def util_physical_block_replace_relationship_fixture(
) -> tuple[dict[str, object], dict[str, object]]:
    data, payload = util_physical_block_replace_fixture()
    source_shape_input = {
        "path": "src/Battlesport/util.h",
        "role": "own-header",
        "evidence": "Reviewed source-shape input fixture.",
    }
    header_contributor = {
        "candidate_header_paths": ["src/GameZRecoil/zVideo/zvid.h"],
        "evidence": "Reviewed candidate header contributor fixture.",
        "included_in_candidate": "src/GameZRecoil/zVideo/zvid_main.c",
        "range": "[0x437ef0,0x437ff0)",
        "status": "candidate-unproven-header-like-fixture",
    }
    old_block = data["physical_blocks"]["recoil:block:0x437e60"]
    old_block["source_shape_inputs"] = [deepcopy(source_shape_input)]
    old_block["candidate_header_contributors"] = [deepcopy(header_contributor)]
    payload["current_block"]["source_shape_inputs"] = [deepcopy(source_shape_input)]
    payload["current_block"]["candidate_header_contributors"] = [
        deepcopy(header_contributor)
    ]
    payload["replacement_blocks"][0]["source_shape_inputs"] = [
        deepcopy(source_shape_input)
    ]
    payload["replacement_blocks"][1]["candidate_header_contributors"] = [
        deepcopy(header_contributor)
    ]
    return data, payload


def util_physical_block_replace_padding_fixture(
) -> tuple[dict[str, object], dict[str, object]]:
    data, payload = util_physical_block_replace_fixture()
    old_span_id = "recoil:semantic:0x438920-0x438980"
    authored_span_id = "recoil:semantic:0x438920-0x438970"
    padding_span_id = "recoil:semantic:0x438970-0x438980"
    block_id = "recoil:block:0x438920"
    symbol_id = "recoil:function:0x438920"

    data["symbols"][symbol_id]["end_exclusive"] = "0x438970"
    data["symbols"][symbol_id]["semantic_span_ids"] = [authored_span_id]
    old_span = data["semantic_spans"].pop(old_span_id)
    authored_span = deepcopy(old_span)
    authored_span["end_exclusive"] = "0x438970"
    authored_span["symbol_ids"] = [symbol_id]
    padding_span = {
        "binary": "recoil",
        "confidence": "high reviewed fixture padding",
        "end_exclusive": "0x438980",
        "evidence_ids": ["recoil:evidence:padding-fixture"],
        "navigation_aliases": ["padding-fixture"],
        "physical_block_id": "recoil:block:0x437e60",
        "source_path": "padding:util-fixture-tail",
        "start": "0x438970",
        "status": "padding",
        "symbol_ids": [],
    }
    data["semantic_spans"][authored_span_id] = authored_span
    data["semantic_spans"][padding_span_id] = padding_span
    data["physical_blocks"]["recoil:block:0x437e60"]["semantic_span_ids"][-1:] = [
        authored_span_id,
        padding_span_id,
    ]
    payload["current_block"]["semantic_span_ids"] = list(
        data["physical_blocks"]["recoil:block:0x437e60"]["semantic_span_ids"]
    )

    authored_payload = {
        "id": authored_span_id,
        "start": "0x438920",
        "end_exclusive": "0x438970",
        "physical_block_id": block_id,
        "source_path": authored_span["source_path"],
        "status": authored_span["status"],
        "confidence": authored_span["confidence"],
        "symbol_ids": [symbol_id],
    }
    padding_payload = {
        "id": padding_span_id,
        "start": "0x438970",
        "end_exclusive": "0x438980",
        "physical_block_id": block_id,
        "source_path": padding_span["source_path"],
        "status": padding_span["status"],
        "confidence": padding_span["confidence"],
        "symbol_ids": [],
    }
    payload["replacement_semantic_spans"][-1:] = [
        authored_payload,
        padding_payload,
    ]
    payload["replacement_blocks"][-1]["semantic_span_ids"][-1:] = [
        authored_span_id,
        padding_span_id,
    ]
    return data, payload


def util_physical_block_replace_full_order_fixture(
) -> tuple[dict[str, object], dict[str, object]]:
    data, payload = util_physical_block_replace_fixture()
    old_block = data["physical_blocks"]["recoil:block:0x437e60"]
    successor = data["physical_blocks"]["recoil:block:0x438980"]
    old_block["order"]["authored"] = _accepted_order_group(AUTHORED_ORDER_DIMENSIONS)
    old_block["order"]["full"] = _accepted_order_group(FULL_ORDER_DIMENSIONS)
    successor["order"]["authored"] = _accepted_order_group(AUTHORED_ORDER_DIMENSIONS)
    for symbol_id in payload["current_block"]["contribution_ids"]:
        data["symbols"][symbol_id]["accepted_byte_facts"] = {
            "sentinel": f"preserve:{symbol_id}"
        }
    return data, payload


def util_physical_block_replace_call_contract_fixture(
) -> tuple[dict[str, object], dict[str, object]]:
    from _recoil.lib.verification_targets import vc5_target_registration

    data, payload = util_physical_block_replace_full_order_fixture()
    target_id, target = vc5_target_registration(
        REPO_ROOT
        / "tools"
        / "vc5_verify_targets"
        / "battlesport_transition_order_current_shape.json"
    )
    data["verification_targets"][target_id] = target

    affected_symbol_id = "recoil:function:0x437e60"
    successor_symbol_id = "recoil:function:0x438980"
    gating_symbol_ids = {affected_symbol_id, successor_symbol_id}
    for symbol_id, symbol in data["symbols"].items():
        if symbol_id in gating_symbol_ids:
            symbol["pipeline_class"] = "authored"
            symbol["authored_order_role"] = "authored-body"
            symbol["binary_state"]["call_contract"] = state_record()
        else:
            symbol["pipeline_class"] = "non-authored"
            symbol["authored_order_role"] = "non-authored"

    data["symbols"][affected_symbol_id]["binary_state"]["call_contract"] = state_record(
        result="passed",
        disposition="accepted",
        freshness="current",
        evidence_ids=["recoil:evidence:stale-call-contract-fixture"],
        gating=True,
        validation_mode="live",
    )
    data["symbols"][affected_symbol_id]["accepted_call_contract_facts"] = {
        "stale": "old physical-block slice closure"
    }
    data["symbols"][successor_symbol_id]["accepted_call_contract_facts"] = {
        "stale": "same old physical-block slice closure"
    }

    old_block = data["physical_blocks"]["recoil:block:0x437e60"]
    successor = data["physical_blocks"]["recoil:block:0x438980"]
    old_block["accepted_order_facts"] = {
        "phase": "authored-function-order",
        "target_id": target_id,
        "matched_identities": [affected_symbol_id],
    }
    successor["accepted_order_facts"] = {
        "phase": "authored-function-order",
        "target_id": target_id,
        "matched_identities": [successor_symbol_id],
    }
    return data, payload


class PhysicalBlockReplaceTests(unittest.TestCase):
    def test_preserves_existing_semantic_observations_across_a_span_split(self) -> None:
        data, raw_payload = util_physical_block_replace_fixture()
        current_span_id = "recoil:semantic:0x437fe4-0x438350"
        current_span = data["semantic_spans"][current_span_id]
        current_span.update(
            {
                "confidence": "high reviewed provider category fixture",
                "evidence_ids": ["recoil:evidence:semantic-preservation"],
                "navigation_aliases": ["provider-fixture"],
                "observation": {"kind": "compiler-generated-fixture"},
                "source_path": "provider:msvc5-fixture",
                "status": "compiler-generated-fixture",
            }
        )
        split_ids = {
            "recoil:semantic:0x437fe4-0x437ff0",
            "recoil:semantic:0x437ff0-0x438350",
        }
        for span in raw_payload["replacement_semantic_spans"]:
            if span["id"] in split_ids:
                span["confidence"] = current_span["confidence"]
                span["source_path"] = current_span["source_path"]
                span["status"] = current_span["status"]

        payload = _parse_physical_block_replace_payload(json.dumps(raw_payload))
        details = _replace_physical_block(data, payload)

        self.assertTrue(
            details["relationship_checks"]["semantic_observations_preserved"]
        )
        for split_id in split_ids:
            stored = data["semantic_spans"][split_id]
            self.assertEqual(current_span["confidence"], stored["confidence"])
            self.assertEqual(current_span["source_path"], stored["source_path"])
            self.assertEqual(current_span["status"], stored["status"])
            self.assertEqual(current_span["evidence_ids"], stored["evidence_ids"])
            self.assertEqual(
                current_span["navigation_aliases"],
                stored["navigation_aliases"],
            )
            self.assertEqual(current_span["observation"], stored["observation"])

    def test_preserves_exact_zero_symbol_padding_span(self) -> None:
        data, raw_payload = util_physical_block_replace_padding_fixture()
        padding_id = "recoil:semantic:0x438970-0x438980"
        expected_padding = deepcopy(data["semantic_spans"][padding_id])

        payload = _parse_physical_block_replace_payload(json.dumps(raw_payload))
        details = _replace_physical_block(data, payload)

        stored = data["semantic_spans"][padding_id]
        self.assertEqual([], stored["symbol_ids"])
        self.assertEqual("padding", stored["status"])
        self.assertEqual("padding:util-fixture-tail", stored["source_path"])
        self.assertEqual(expected_padding["evidence_ids"], stored["evidence_ids"])
        self.assertEqual(
            expected_padding["navigation_aliases"],
            stored["navigation_aliases"],
        )
        self.assertTrue(
            details["relationship_checks"][
                "zero_symbol_spans_limited_to_exact_padding"
            ]
        )

    def test_rejects_semantic_status_path_or_symbol_population_drift(self) -> None:
        cases = []
        data, payload = util_physical_block_replace_fixture()
        payload["replacement_semantic_spans"][0]["status"] = "invented-status"
        cases.append((data, payload, "preserve current status"))

        data, payload = util_physical_block_replace_fixture()
        payload["replacement_semantic_spans"][0]["source_path"] = "provider:invented"
        cases.append((data, payload, "preserve current source_path"))

        data, payload = util_physical_block_replace_fixture()
        payload["replacement_semantic_spans"][3]["symbol_ids"].pop()
        cases.append((data, payload, "complete exact symbol assignment"))

        for data, raw_payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(
                ProgressError,
                message,
            ):
                parsed = _parse_physical_block_replace_payload(
                    json.dumps(raw_payload)
                )
                _replace_physical_block(data, parsed)

    def test_rejects_invented_empty_non_padding_semantic_span(self) -> None:
        data, raw_payload = util_physical_block_replace_fixture()
        data["symbols"]["recoil:function:0x438920"]["end_exclusive"] = "0x438970"
        old_replacement = raw_payload["replacement_semantic_spans"].pop()
        first = deepcopy(old_replacement)
        first.update(
            {
                "id": "recoil:semantic:0x438920-0x438970",
                "end_exclusive": "0x438970",
            }
        )
        empty = deepcopy(old_replacement)
        empty.update(
            {
                "id": "recoil:semantic:0x438970-0x438980",
                "start": "0x438970",
                "symbol_ids": [],
            }
        )
        raw_payload["replacement_semantic_spans"].extend([first, empty])
        raw_payload["replacement_blocks"][-1]["semantic_span_ids"][-1:] = [
            first["id"],
            empty["id"],
        ]

        parsed = _parse_physical_block_replace_payload(json.dumps(raw_payload))
        with self.assertRaisesRegex(
            ProgressError,
            "only when it exactly preserves a current reviewed zero-symbol padding span",
        ):
            _replace_physical_block(data, parsed)

    def test_rejects_padding_merge_and_drop(self) -> None:
        data, payload = util_physical_block_replace_padding_fixture()
        authored = payload["replacement_semantic_spans"][-2]
        padding = payload["replacement_semantic_spans"][-1]
        merged = deepcopy(authored)
        merged.update(
            {
                "id": "recoil:semantic:0x438920-0x438980",
                "end_exclusive": "0x438980",
            }
        )
        payload["replacement_semantic_spans"][-2:] = [merged]
        payload["replacement_blocks"][-1]["semantic_span_ids"][-2:] = [merged["id"]]
        parsed = _parse_physical_block_replace_payload(json.dumps(payload))
        with self.assertRaisesRegex(ProgressError, "merges or crosses"):
            _replace_physical_block(data, parsed)

        data, payload = util_physical_block_replace_padding_fixture()
        dropped_id = payload["replacement_semantic_spans"].pop()["id"]
        payload["replacement_blocks"][-1]["semantic_span_ids"].remove(dropped_id)
        parsed = _parse_physical_block_replace_payload(json.dumps(payload))
        with self.assertRaisesRegex(ProgressError, "not old block end"):
            _replace_physical_block(data, parsed)

    def test_completed_authored_order_regresses_exactly_and_preserves_byte_state(self) -> None:
        data, raw_payload = util_physical_block_replace_full_order_fixture()
        symbol_ids = list(raw_payload["current_block"]["contribution_ids"])
        byte_facts_before = {
            symbol_id: {
                "binary_state": deepcopy(data["symbols"][symbol_id]["binary_state"]),
                "accepted_byte_facts": deepcopy(
                    data["symbols"][symbol_id]["accepted_byte_facts"]
                ),
            }
            for symbol_id in symbol_ids
        }
        scheduler_before = ProgressDocument(data).pipeline(
            "recoil", resolve_order_target=False
        )
        self.assertEqual("full-function-order", scheduler_before["phase"])
        self.assertEqual("0x438980", scheduler_before["cursor"])

        payload = _parse_physical_block_replace_payload(json.dumps(raw_payload))
        details = _replace_physical_block(data, payload)

        scheduler_after = ProgressDocument(data).pipeline(
            "recoil", resolve_order_target=False
        )
        self.assertEqual("authored-function-order", scheduler_after["phase"])
        self.assertEqual("0x437e60", scheduler_after["cursor"])
        self.assertEqual(
            "replacement-authored-order-regression",
            details["phase_transition_contract"]["transition"],
        )
        self.assertEqual([], details["invalidated"]["symbol_ids"])
        self.assertTrue(all(details["independent_byte_state"].values()))
        self.assertEqual(
            scheduler_before["authored_byte_cursor"],
            scheduler_after["authored_byte_cursor"],
        )
        self.assertEqual(
            scheduler_before["linked_byte_match_prefix_end"],
            scheduler_after["linked_byte_match_prefix_end"],
        )
        self.assertEqual(
            byte_facts_before,
            {
                symbol_id: {
                    "binary_state": data["symbols"][symbol_id]["binary_state"],
                    "accepted_byte_facts": data["symbols"][symbol_id][
                        "accepted_byte_facts"
                    ],
                }
                for symbol_id in symbol_ids
            },
        )

    def test_started_call_contract_regresses_and_invalidates_the_affected_slice(self) -> None:
        data, raw_payload = util_physical_block_replace_call_contract_fixture()
        affected_symbol_id = "recoil:function:0x437e60"
        successor_symbol_id = "recoil:function:0x438980"
        dependency_symbol_ids = [affected_symbol_id, successor_symbol_id]
        independent_state_before = {
            symbol_id: {
                dimension: deepcopy(state)
                for dimension, state in data["symbols"][symbol_id][
                    "binary_state"
                ].items()
                if dimension != "call_contract"
            }
            for symbol_id in dependency_symbol_ids
        }
        accepted_byte_facts_before = {
            symbol_id: deepcopy(
                data["symbols"][symbol_id].get("accepted_byte_facts")
            )
            for symbol_id in dependency_symbol_ids
        }
        scheduler_before = ProgressDocument(data).pipeline(
            "recoil", resolve_order_target=False
        )
        self.assertEqual("authored-call-contract", scheduler_before["phase"])
        self.assertEqual(0, scheduler_before["authored_function_order_counts"]["remaining"])

        payload = _parse_physical_block_replace_payload(json.dumps(raw_payload))
        details = _replace_physical_block(data, payload)

        scheduler_after = ProgressDocument(data).pipeline(
            "recoil", resolve_order_target=False
        )
        self.assertEqual("authored-function-order", scheduler_after["phase"])
        self.assertEqual("0x437e60", scheduler_after["cursor"])
        self.assertEqual(
            "replacement-authored-order-regression",
            details["phase_transition_contract"]["transition"],
        )
        self.assertEqual(
            dependency_symbol_ids,
            details["invalidated"]["call_contract_symbol_ids"],
        )
        for symbol_id in dependency_symbol_ids:
            self.assertEqual(
                "pending",
                data["symbols"][symbol_id]["binary_state"]["call_contract"]["result"],
            )
            self.assertNotIn(
                "accepted_call_contract_facts", data["symbols"][symbol_id]
            )
            self.assertEqual(
                independent_state_before[symbol_id],
                {
                    dimension: state
                    for dimension, state in data["symbols"][symbol_id][
                        "binary_state"
                    ].items()
                    if dimension != "call_contract"
                },
            )
            self.assertEqual(
                accepted_byte_facts_before[symbol_id],
                data["symbols"][symbol_id].get("accepted_byte_facts"),
            )
        self.assertEqual(
            scheduler_before["authored_byte_cursor"],
            scheduler_after["authored_byte_cursor"],
        )
        self.assertEqual(
            scheduler_before["linked_byte_match_prefix_end"],
            scheduler_after["linked_byte_match_prefix_end"],
        )

    def test_physical_block_replace_rejects_every_other_phase_change(self) -> None:
        replacement_ids = ["recoil:block:0x401000"]
        invalidated = {
            "block_ids": replacement_ids,
            "symbol_ids": [],
            "call_contract_symbol_ids": [],
        }
        common_after = {
            "cursor": "0x401000",
            "physical_block_id": "recoil:block:0x401000",
            "authored_order_prefix_end": "0x401000",
            "authored_function_order_counts": {
                "accepted": 0,
                "remaining": 1,
                "total": 1,
            },
        }
        cases = (
            (
                {"phase": "authored-function-order", "cursor": "0x401000"},
                {**common_after, "phase": "full-function-order"},
                "only authored-call-contract or full-function-order",
            ),
            (
                {"phase": "linked-byte-match", "cursor": "0x401000"},
                {**common_after, "phase": "authored-function-order"},
                "only authored-call-contract or full-function-order",
            ),
            (
                {
                    "phase": "full-function-order",
                    "cursor": "0x402000",
                    "authored_function_order_counts": {
                        "accepted": 1,
                        "remaining": 0,
                        "total": 1,
                    },
                },
                {
                    **common_after,
                    "phase": "authored-function-order",
                    "cursor": "0x401010",
                },
                "not the exact replacement invalidation transition",
            ),
        )
        for before, after, message in cases:
            with self.subTest(before=before["phase"], after=after["phase"]):
                with self.assertRaisesRegex(ProgressError, message):
                    progress_cli._validate_physical_block_replace_scheduler_transition(
                        before,
                        after,
                        old_block_id="recoil:block:0x401000",
                        old_start="0x401000",
                        replacement_block_ids=replacement_ids,
                        invalidated=invalidated,
                        call_contract_dependency_symbol_ids=[],
                    )

    def test_physical_block_replace_rejects_regression_from_byte_phase(self) -> None:
        data, raw_payload = util_physical_block_replace_full_order_fixture()
        successor = data["physical_blocks"]["recoil:block:0x438980"]
        successor["order"]["full"] = _accepted_order_group(FULL_ORDER_DIMENSIONS)
        self.assertEqual(
            "authored-byte-match",
            ProgressDocument(data).pipeline("recoil", resolve_order_target=False)["phase"],
        )
        payload = _parse_physical_block_replace_payload(json.dumps(raw_payload))
        with self.assertRaisesRegex(
            ProgressError,
            "only authored-call-contract or full-function-order",
        ):
            _replace_physical_block(data, payload)

    def test_current_util_partition_replaces_all_relationships_atomically(self) -> None:
        data, raw_payload = util_physical_block_replace_fixture()
        payload = _parse_physical_block_replace_payload(json.dumps(raw_payload))
        details = _replace_physical_block(data, payload)

        self.assertEqual(
            [f"recoil:block:{start}" for start, _end, _path in UTIL_REPLACEMENT_BLOCKS],
            details["replacement_block_ids"],
        )
        self.assertEqual(21, len(details["reassigned_symbol_ids"]))
        self.assertTrue(details["relationship_checks"]["complete_block_coverage"])
        self.assertTrue(details["relationship_checks"]["complete_semantic_span_coverage"])
        self.assertIsNone(data["physical_blocks"]["recoil:block:0x437ff0"]["original_source_path"])
        self.assertIsNone(
            data["physical_blocks"]["recoil:block:0x437ff0"]["provisional_original_path"]
        )
        self.assertEqual(
            "unresolved",
            data["physical_blocks"]["recoil:block:0x437ff0"]["mapping"]["state"],
        )
        self.assertNotIn("recoil:semantic:0x437fe4-0x438350", data["semantic_spans"])
        self.assertEqual(
            ["recoil:semantic:0x437fe4-0x437ff0"],
            data["symbols"]["recoil:function:0x437fe4"]["semantic_span_ids"],
        )
        self.assertEqual(
            "recoil:block:0x437ff0",
            data["symbols"]["recoil:function:0x437ff0"]["physical_block_id"],
        )
        scheduler = ProgressDocument(data).pipeline("recoil", resolve_order_target=False)
        self.assertEqual("0x437e60", scheduler["cursor"])
        self.assertEqual("recoil:block:0x437e60", scheduler["physical_block_id"])
        self.assertEqual(7, scheduler["authored_function_order_counts"]["total"])

    def test_physical_block_replace_reassigns_source_shape_relationships_atomically(self) -> None:
        data, raw_payload = util_physical_block_replace_relationship_fixture()
        payload = _parse_physical_block_replace_payload(json.dumps(raw_payload))
        details = _replace_physical_block(data, payload)

        self.assertEqual(1, details["reassigned_source_shape_input_count"])
        self.assertEqual(1, details["reassigned_candidate_header_contributor_count"])
        self.assertTrue(
            details["relationship_checks"]["complete_source_shape_input_reassignment"]
        )
        self.assertTrue(
            details["relationship_checks"][
                "complete_candidate_header_contributor_reassignment"
            ]
        )
        self.assertEqual(
            raw_payload["current_block"]["source_shape_inputs"],
            data["physical_blocks"]["recoil:block:0x437e60"]["source_shape_inputs"],
        )
        self.assertEqual(
            raw_payload["current_block"]["candidate_header_contributors"],
            data["physical_blocks"]["recoil:block:0x437ef0"][
                "candidate_header_contributors"
            ],
        )
        self.assertEqual(
            [],
            data["physical_blocks"]["recoil:block:0x437ef0"]["source_shape_inputs"],
        )

    def test_physical_block_replace_rejects_unsafe_source_shape_reassignments(self) -> None:
        cases = []

        data, payload = util_physical_block_replace_relationship_fixture()
        del payload["current_block"]["source_shape_inputs"]
        del payload["current_block"]["candidate_header_contributors"]
        cases.append((data, payload, "stale in fields"))

        data, payload = util_physical_block_replace_relationship_fixture()
        payload["replacement_blocks"][0]["source_shape_inputs"] = []
        cases.append((data, payload, "complete exact source_shape_inputs reassignment"))

        data, payload = util_physical_block_replace_relationship_fixture()
        duplicate = deepcopy(payload["replacement_blocks"][0]["source_shape_inputs"][0])
        payload["replacement_blocks"][1]["source_shape_inputs"] = [duplicate]
        cases.append((data, payload, "duplicate a source_shape_inputs relationship"))

        data, payload = util_physical_block_replace_relationship_fixture()
        payload["replacement_blocks"][0]["source_shape_inputs"][0]["role"] = "altered"
        cases.append((data, payload, "complete exact source_shape_inputs reassignment"))

        data, payload = util_physical_block_replace_relationship_fixture()
        header = payload["replacement_blocks"][1].pop("candidate_header_contributors")
        payload["replacement_blocks"][0]["candidate_header_contributors"] = header
        cases.append((data, payload, "invalid replacement block target"))

        data, payload = util_physical_block_replace_relationship_fixture()
        payload["replacement_blocks"][1]["candidate_header_contributors"] = []
        cases.append(
            (data, payload, "complete exact candidate_header_contributors reassignment")
        )

        for data, raw_payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(ProgressError, message):
                parsed = _parse_physical_block_replace_payload(json.dumps(raw_payload))
                _replace_physical_block(data, parsed)

    def test_physical_block_replace_rejects_incomplete_or_unsafe_packages(self) -> None:
        cases = []

        data, payload = util_physical_block_replace_fixture()
        payload["replacement_blocks"][1]["start"] = "0x437f00"
        cases.append((data, payload, "gap"))

        data, payload = util_physical_block_replace_fixture()
        payload["replacement_blocks"][2]["contribution_ids"].pop()
        cases.append((data, payload, "complete exact"))

        data, payload = util_physical_block_replace_fixture()
        payload["replacement_semantic_spans"][3]["symbol_ids"].pop()
        cases.append((data, payload, "complete exact symbol assignment"))

        data, payload = util_physical_block_replace_fixture()
        payload["current_block"]["mapping_status"] = "stale-status"
        cases.append((data, payload, "stale in fields"))

        data, payload = util_physical_block_replace_fixture()
        data["work_items"]["recoil:work:util"] = {
            "state": "active",
            "block_id": "recoil:block:0x437e60",
        }
        cases.append((data, payload, "schedulable work item"))

        for data, raw_payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(ProgressError, message):
                parsed = _parse_physical_block_replace_payload(json.dumps(raw_payload))
                _replace_physical_block(data, parsed)

        _data, payload = util_physical_block_replace_fixture()
        payload["replacement_blocks"][0]["original_source_path"] = "util.cpp"
        with self.assertRaisesRegex(ProgressError, "preserve unresolved original provenance"):
            _parse_physical_block_replace_payload(json.dumps(payload))

    def test_physical_block_replace_dry_run_apply_and_revision_cas(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            data, raw_payload = util_physical_block_replace_fixture()
            path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            payload_json = json.dumps(raw_payload)

            stdout = io.StringIO()
            stderr = io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                dry_rc = progress_cli.main(
                    [
                        "block",
                        "replace",
                        "--progress",
                        str(path),
                        "--payload-json",
                        payload_json,
                        "--expected-revision",
                        "845",
                        "--dry-run",
                        "--json",
                    ]
                )
            self.assertEqual(0, dry_rc, stderr.getvalue())
            dry = json.loads(stdout.getvalue())
            self.assertFalse(dry["commit"]["applied"])
            self.assertEqual(846, dry["commit"]["revision"])
            self.assertEqual(845, json.loads(path.read_text(encoding="utf-8"))["revision"])

            stdout = io.StringIO()
            stderr = io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                apply_rc = progress_cli.main(
                    [
                        "block",
                        "replace",
                        "--progress",
                        str(path),
                        "--payload-json",
                        payload_json,
                        "--expected-revision",
                        "845",
                        "--apply",
                        "--json",
                    ]
                )
            self.assertEqual(0, apply_rc, stderr.getvalue())
            applied = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(846, applied["revision"])
            self.assertEqual("0x437ef0", applied["physical_blocks"]["recoil:block:0x437e60"]["end_exclusive"])

            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                stale_rc = progress_cli.main(
                    [
                        "block",
                        "replace",
                        "--progress",
                        str(path),
                        "--payload-json",
                        payload_json,
                        "--expected-revision",
                        "845",
                        "--apply",
                        "--json",
                    ]
                )
            self.assertEqual(2, stale_rc)

    def test_physical_block_replace_payload_file_dry_run_uses_same_parser(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture_root = Path(temporary)
            temporary_path = fixture_root / "build"
            temporary_path.mkdir()
            progress_path = temporary_path / "progress.json"
            payload_path = temporary_path / "replace.json"
            data, raw_payload = util_physical_block_replace_fixture()
            progress_path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            payload_path.write_text(
                json.dumps(raw_payload) + "\n",
                encoding="utf-8",
            )

            stdout = io.StringIO()
            stderr = io.StringIO()
            with (
                patch.object(progress_cli, "REPO_ROOT", fixture_root),
                redirect_stdout(stdout),
                redirect_stderr(stderr),
            ):
                rc = progress_cli.main(
                    [
                        "block",
                        "replace",
                        "--progress",
                        str(progress_path),
                        "--payload-file",
                        str(payload_path),
                        "--expected-revision",
                        "845",
                        "--dry-run",
                        "--json",
                    ]
                )

            self.assertEqual(0, rc, stderr.getvalue())
            result = json.loads(stdout.getvalue())
            self.assertFalse(result["commit"]["applied"])
            self.assertEqual("physical-block-replace", result["kind"])
            self.assertEqual(
                845,
                json.loads(progress_path.read_text(encoding="utf-8"))["revision"],
            )

    def test_physical_block_replace_payload_sources_are_mutually_exclusive(self) -> None:
        stderr = io.StringIO()
        with redirect_stderr(stderr), self.assertRaises(SystemExit) as raised:
            progress_cli._parser().parse_args(
                [
                    "block",
                    "replace",
                    "--payload-json",
                    "{}",
                    "--payload-file",
                    "build/replace.json",
                    "--expected-revision",
                    "845",
                    "--dry-run",
                    "--json",
                ]
            )
        self.assertEqual(2, raised.exception.code)
        self.assertIn("not allowed with argument --payload-json", stderr.getvalue())

    def test_physical_block_replace_payload_file_reports_input_failures(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture_root = Path(temporary)
            temporary_path = fixture_root / "build"
            temporary_path.mkdir()
            progress_path = temporary_path / "progress.json"
            data, _raw_payload = util_physical_block_replace_fixture()
            progress_path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            cases = (
                (
                    temporary_path / "missing.json",
                    None,
                    "missing or unreadable",
                ),
                (
                    temporary_path / "invalid-utf8.json",
                    b"\xff\xfe\xfa",
                    "not valid UTF-8",
                ),
                (
                    temporary_path / "invalid-json.json",
                    b"{",
                    "--payload-file is not valid JSON",
                ),
            )
            for payload_path, content, message in cases:
                if content is not None:
                    payload_path.write_bytes(content)
                stdout = io.StringIO()
                stderr = io.StringIO()
                with (
                    self.subTest(message=message),
                    patch.object(progress_cli, "REPO_ROOT", fixture_root),
                    redirect_stdout(stdout),
                    redirect_stderr(stderr),
                ):
                    rc = progress_cli.main(
                        [
                            "block",
                            "replace",
                            "--progress",
                            str(progress_path),
                            "--payload-file",
                            str(payload_path),
                            "--expected-revision",
                            "845",
                            "--dry-run",
                            "--json",
                        ]
                    )
                self.assertEqual(2, rc)
                self.assertEqual("", stdout.getvalue())
                self.assertIn(message, stderr.getvalue())


AUTHORED_NON_GATING_TAIL_BLOCKS = (
    ("0x4c5a50", "0x4c5eb8", "provider-boundary", "provider", 188),
    ("0x4c5eb8", "0x4c5ec0", "padding", "padding", 0),
    ("0x4c5ec0", "0x4c60a0", "provider-boundary", "provider", 10),
    ("0x4c60a0", "0x4c60b0", "provider-boundary", "provider", 2),
    ("0x4c60b0", "0x4c637c", "provider-boundary", "provider", 16),
    ("0x4c637c", "0x4c63f0", "provider-boundary", "provider", 17),
    ("0x4c63f0", "0x4c7408", "provider-data", "provider", 0),
    ("0x4c7408", "0x4c7410", "padding", "padding", 0),
    ("0x4c7410", "0x4c7498", "provider-data", "provider", 0),
    ("0x4c7498", "0x4c74a0", "provider-boundary", "provider", 1),
    ("0x4c74a0", "0x4c7ef8", "provider-data", "provider", 0),
    ("0x4c7ef8", "0x4c7f00", "padding", "padding", 0),
)


def authored_non_gating_tail_fixture() -> tuple[dict[str, object], dict[str, object]]:
    data = empty_progress_document()
    data["revision"] = 1344
    data["binaries"]["recoil"] = {
        "binary": "recoil",
        "primary_scheduler": True,
        "text": {"start": "0x4c5a50", "end_exclusive": "0x4c7f10"},
    }
    for start, end, status, contribution_kind, count in AUTHORED_NON_GATING_TAIL_BLOCKS:
        block_id = f"recoil:block:{start}"
        contribution_ids: list[str] = []
        start_value = int(start, 16)
        for index in range(count):
            address_value = start_value + (index * 6 if start == "0x4c5a50" else index)
            address = f"0x{address_value:x}"
            symbol_id = f"recoil:function:{address}"
            contribution_ids.append(symbol_id)
            data["symbols"][symbol_id] = {
                "address": address,
                "authored_order_role": "non-authored",
                "binary": "recoil",
                "binary_state": {},
                "end_exclusive": f"0x{address_value + 1:x}",
                "kind": "function",
                "physical_block_id": block_id,
                "pipeline_class": "non-authored",
                "semantic_span_ids": [],
            }
        confidence = {
            "provider-boundary": "high provider",
            "provider-data": "high provider-data",
            "padding": "high padding",
        }[status]
        data["physical_blocks"][block_id] = {
            "accepted_order_facts": None,
            "agent_source_path": f"{contribution_kind}:fixture-{start}",
            "binary": "recoil",
            "contribution_ids": contribution_ids,
            "contribution_kind": contribution_kind,
            "end_exclusive": end,
            "first_external_callers": [],
            "mapping": {
                "confidence": confidence,
                "evidence_ids": [],
                "file_literal": None,
                "literal_xrefs": [],
                "state": "unresolved",
                "status": status,
            },
            "order": _pending_order(),
            "order_diagnostic": {},
            "order_diagnostics": {},
            "order_targets": {"linked": "", "object": ""},
            "original_source_path": None,
            "provisional_original_path": None,
            "row_kind": "physical-source-block",
            "semantic_span_ids": [],
            "source_path": f"{contribution_kind}:fixture-{start}",
            "source_shape_inputs": [],
            "start": start,
        }
    successor_block_id = "recoil:block:0x4c7f00"
    successor_symbol_id = "recoil:function:0x4c7f00"
    data["physical_blocks"][successor_block_id] = {
        "accepted_order_facts": None,
        "agent_source_path": "src/tail_authored.cpp",
        "binary": "recoil",
        "contribution_ids": [successor_symbol_id],
        "contribution_kind": "authored",
        "end_exclusive": "0x4c7f10",
        "mapping": {"confidence": "high authored", "state": "accepted", "status": "mapped"},
        "order": _pending_order(),
        "order_targets": {"linked": "", "object": ""},
        "semantic_span_ids": [],
        "source_path": "src/tail_authored.cpp",
        "start": "0x4c7f00",
    }
    data["symbols"][successor_symbol_id] = {
        "address": "0x4c7f00",
        "authored_order_role": "authored-body",
        "binary": "recoil",
        "binary_state": {},
        "end_exclusive": "0x4c7f10",
        "kind": "function",
        "physical_block_id": successor_block_id,
        "pipeline_class": "authored",
        "semantic_span_ids": [],
    }
    payload: dict[str, object] = {
        "schema": "recoil-authored-non-gating-block-accept-v1",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": "Exact retail tail rows are classified provider-only or non-code.",
        "binary": "recoil",
        "current_cursor": "0x4c5a50",
        "expected_cursor_after": "0x4c7f00",
        "blocks": [
            {
                "id": f"recoil:block:{start}",
                "current": deepcopy(data["physical_blocks"][f"recoil:block:{start}"]),
            }
            for start, _end, _status, _kind, _count in AUTHORED_NON_GATING_TAIL_BLOCKS
        ],
    }
    return data, payload


def authored_lifecycle_helper_tail_fixture() -> tuple[dict[str, object], dict[str, object]]:
    data = empty_progress_document()
    data["revision"] = 1344
    data["binaries"]["recoil"] = {
        "binary": "recoil",
        "primary_scheduler": True,
        "text": {"start": "0x4c8230", "end_exclusive": "0x4cb9e8"},
    }
    block_id = "recoil:block:0x4c8230"
    contribution_ids: list[str] = []
    for index in range(1060):
        address = f"0x{0x4C8230 + index:x}"
        symbol_id = f"recoil:function:{address}"
        contribution_ids.append(symbol_id)
        data["symbols"][symbol_id] = {
            "address": address,
            "authored_order_role": "compiler-generated-eh-helper",
            "binary": "recoil",
            "binary_state": {},
            "end_exclusive": f"0x{0x4C8231 + index:x}",
            "kind": "function",
            "physical_block_id": block_id,
            "pipeline_class": "authored-lifecycle",
            "semantic_span_ids": [],
        }
    data["physical_blocks"][block_id] = {
        "accepted_order_facts": None,
        "agent_source_path": "provider:msvc-cxx-eh-funclet-tail",
        "binary": "recoil",
        "contribution_ids": contribution_ids,
        "contribution_kind": "provider",
        "end_exclusive": "0x4cb9e8",
        "first_external_callers": [],
        "mapping": {
            "confidence": "high compiler-provider",
            "evidence_ids": [],
            "file_literal": None,
            "literal_xrefs": [],
            "state": "unresolved",
            "status": "provider-boundary",
        },
        "order": _pending_order(),
        "order_diagnostic": {},
        "order_diagnostics": {},
        "order_targets": {"linked": "", "object": ""},
        "original_source_path": None,
        "provisional_original_path": None,
        "row_kind": "physical-source-block",
        "semantic_span_ids": [],
        "source_path": "provider:msvc-cxx-eh-funclet-tail",
        "source_shape_inputs": [],
        "start": "0x4c8230",
    }
    payload: dict[str, object] = {
        "schema": "recoil-authored-non-gating-block-accept-v1",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": "All retail tail rows are resolved compiler-generated EH helpers.",
        "binary": "recoil",
        "current_cursor": "0x4c8230",
        "expected_cursor_after": "0x4cb9e8",
        "blocks": [{"id": block_id, "current": deepcopy(data["physical_blocks"][block_id])}],
    }
    return data, payload


class AuthoredNonGatingBlockAcceptanceTests(unittest.TestCase):
    def test_exact_twelve_block_tail_accepts_only_authored_not_applicable_dimensions(
        self,
    ) -> None:
        data, raw_payload = authored_non_gating_tail_fixture()
        before = deepcopy(data)
        payload = _parse_authored_non_gating_block_accept_payload(json.dumps(raw_payload))

        details = _accept_authored_non_gating_blocks(data, payload)

        expected_ids = [
            f"recoil:block:{start}"
            for start, _end, _status, _kind, _count in AUTHORED_NON_GATING_TAIL_BLOCKS
        ]
        self.assertEqual(expected_ids, details["accepted_block_ids"])
        self.assertEqual(12, details["accepted_block_count"])
        self.assertEqual("0x4c5a50", details["scheduler_before"]["cursor"])
        self.assertEqual("0x4c7f00", details["scheduler_after"]["cursor"])
        self.assertEqual(
            details["scheduler_before"]["full_order_prefix_end"],
            details["scheduler_after"]["full_order_prefix_end"],
        )
        self.assertEqual(1, len(set(data["evidence"]) - set(before["evidence"])))
        evidence_id = details["evidence_id"]
        for block_id in expected_ids:
            authored = data["physical_blocks"][block_id]["order"]["authored"]
            for dimension in AUTHORED_ORDER_DIMENSIONS:
                self.assertEqual(
                    {
                        "result": "not-applicable",
                        "disposition": "accepted",
                        "freshness": "current",
                        "evidence_ids": [evidence_id],
                        "gating": True,
                        "validation_mode": "live",
                    },
                    authored[dimension],
                )
            self.assertEqual(
                before["physical_blocks"][block_id]["order"]["full"],
                data["physical_blocks"][block_id]["order"]["full"],
            )
            self.assertIsNone(data["physical_blocks"][block_id]["accepted_order_facts"])
        self.assertEqual(before["symbols"], data["symbols"])
        self.assertEqual(before["owners"], data["owners"])
        self.assertEqual(before["storage_contributions"], data["storage_contributions"])
        self.assertTrue(all(details["invariants"].values()))

    def test_lifecycle_only_eh_helper_tail_completes_authored_order_without_full_order(
        self,
    ) -> None:
        data, raw_payload = authored_lifecycle_helper_tail_fixture()
        before = deepcopy(data)
        payload = _parse_authored_non_gating_block_accept_payload(json.dumps(raw_payload))

        details = _accept_authored_non_gating_blocks(data, payload)

        self.assertEqual(
            {"authored-lifecycle|compiler-generated-eh-helper": 1060},
            details["classification_counts"],
        )
        self.assertEqual("full-function-order", details["scheduler_after"]["phase"])
        self.assertEqual("0x4c8230", details["scheduler_after"]["cursor"])
        self.assertEqual("0x4cb9e8", details["scheduler_after"]["authored_order_prefix_end"])
        self.assertEqual(
            details["scheduler_before"]["full_order_prefix_end"],
            details["scheduler_after"]["full_order_prefix_end"],
        )
        self.assertEqual(before["symbols"], data["symbols"])
        self.assertEqual(
            before["physical_blocks"]["recoil:block:0x4c8230"]["order"]["full"],
            data["physical_blocks"]["recoil:block:0x4c8230"]["order"]["full"],
        )

    def test_large_lifecycle_snapshot_supports_payload_file(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            progress_path = root / "progress.json"
            payload_path = root / "payload.json"
            data, payload = authored_lifecycle_helper_tail_fixture()
            progress_path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            payload_path.write_text(json.dumps(payload) + "\n", encoding="utf-8")
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                rc = progress_cli.main(
                    [
                        "block",
                        "accept-authored-non-gating",
                        "--progress",
                        str(progress_path),
                        "--payload-file",
                        str(payload_path),
                        "--expected-revision",
                        "1344",
                        "--dry-run",
                        "--json",
                    ]
                )
            self.assertEqual(0, rc, stderr.getvalue())
            result = json.loads(stdout.getvalue())
            self.assertFalse(result["commit"]["applied"])
            self.assertEqual(
                {"authored-lifecycle|compiler-generated-eh-helper": 1060},
                result["classification_counts"],
            )
            self.assertEqual("full-function-order", result["scheduler_after"]["phase"])

    def test_rejects_stale_unsafe_or_conflicting_zero_gate_batches(self) -> None:
        cases: list[tuple[dict[str, object], dict[str, object], str]] = []

        data, payload = authored_non_gating_tail_fixture()
        payload["blocks"][0]["current"]["mapping"]["confidence"] = "high stale"
        cases.append((data, payload, "snapshot is stale"))

        data, payload = authored_non_gating_tail_fixture()
        payload["blocks"].pop(1)
        cases.append((data, payload, "exact contiguous physical-block prefix"))

        data, payload = authored_non_gating_tail_fixture()
        first_symbol = data["physical_blocks"]["recoil:block:0x4c5a50"]["contribution_ids"][0]
        data["symbols"][first_symbol]["pipeline_class"] = "unresolved"
        data["symbols"][first_symbol]["authored_order_role"] = "unresolved"
        cases.append((data, payload, "fully resolved with a non-gating"))

        data, payload = authored_non_gating_tail_fixture()
        first_symbol = data["physical_blocks"]["recoil:block:0x4c5a50"]["contribution_ids"][0]
        data["symbols"][first_symbol]["pipeline_class"] = "authored"
        data["symbols"][first_symbol]["authored_order_role"] = "authored-body"
        cases.append((data, payload, "fully resolved with a non-gating"))

        data, payload = authored_non_gating_tail_fixture()
        data["physical_blocks"]["recoil:block:0x4c5ec0"]["contribution_ids"].pop()
        payload["blocks"][2]["current"] = deepcopy(
            data["physical_blocks"]["recoil:block:0x4c5ec0"]
        )
        cases.append((data, payload, "contribution membership is incomplete"))

        data, payload = authored_non_gating_tail_fixture()
        data["physical_blocks"]["recoil:block:0x4c5ec0"]["mapping"]["confidence"] = "medium provider"
        payload["blocks"][2]["current"] = deepcopy(
            data["physical_blocks"]["recoil:block:0x4c5ec0"]
        )
        cases.append((data, payload, "explicit high-confidence mapping"))

        data, payload = authored_non_gating_tail_fixture()
        data["physical_blocks"]["recoil:block:0x4c63f0"]["mapping"]["status"] = "provider-boundary"
        payload["blocks"][6]["current"] = deepcopy(
            data["physical_blocks"]["recoil:block:0x4c63f0"]
        )
        cases.append((data, payload, "typed provider-data or padding"))

        data, payload = authored_non_gating_tail_fixture()
        data["physical_blocks"]["recoil:block:0x4c5a50"]["order_targets"]["object"] = "tail"
        payload["blocks"][0]["current"] = deepcopy(
            data["physical_blocks"]["recoil:block:0x4c5a50"]
        )
        cases.append((data, payload, "active configured order target"))

        data, payload = authored_non_gating_tail_fixture()
        data["work_items"]["recoil:work:active-tail"] = {
            "state": "active",
            "reservation": {"state": "active"},
            "resource_claims": [
                {"kind": "tracker", "id": "recoil", "access": "read"}
            ],
        }
        cases.append((data, payload, "conflicts with active work"))

        data, payload = authored_non_gating_tail_fixture()
        payload["expected_cursor_after"] = "0x4c7ef8"
        cases.append((data, payload, "exact batch end"))

        for data, raw_payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(ProgressError, message):
                payload = _parse_authored_non_gating_block_accept_payload(
                    json.dumps(raw_payload)
                )
                _accept_authored_non_gating_blocks(data, payload)

    def test_parent_route_is_dry_run_apply_atomic_and_revision_guarded(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            data, payload = authored_non_gating_tail_fixture()
            path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            arguments = [
                "block",
                "accept-authored-non-gating",
                "--progress",
                str(path),
                "--payload-json",
                json.dumps(payload),
                "--expected-revision",
                "1344",
                "--dry-run",
                "--json",
            ]
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                dry_rc = progress_cli.main(arguments)
            self.assertEqual(0, dry_rc, stderr.getvalue())
            self.assertFalse(json.loads(stdout.getvalue())["commit"]["applied"])
            self.assertEqual(1344, json.loads(path.read_text(encoding="utf-8"))["revision"])

            arguments[arguments.index("--dry-run")] = "--apply"
            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                apply_rc = progress_cli.main(arguments)
            self.assertEqual(0, apply_rc)
            applied = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(1345, applied["revision"])
            self.assertEqual(
                "not-applicable",
                applied["physical_blocks"]["recoil:block:0x4c5a50"]["order"]["authored"]
                ["object_identity_presence"]["result"],
            )

            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                stale_rc = progress_cli.main(arguments)
            self.assertEqual(2, stale_rc)


ZBD_PROVIDER_FUNCTIONS = (
    ("0x4c07d0", "zZbdManager::SortSectionHandlers"),
    ("0x4c0b60", "zZbdSectionHandlerList::Front"),
    ("0x4c0b70", "zZbdSectionHandlerList::Constructor"),
    ("0x4c0ba0", "zZbdSectionHandlerList::Swap"),
    ("0x4c0bd0", "zZbdSectionHandlerList::Merge"),
    ("0x4c0ce0", "zZbdSectionHandlerList::SpliceThreeNodes"),
)


def _owner_fixture_row(
    *, owner_id: str, kind: str, addresses: list[str], dependency: str = ""
) -> dict[str, object]:
    relationships: list[dict[str, str]] = []
    if addresses:
        relationships.append({"kind": "anchor-address", "address": addresses[0]})
    relationships.extend(
        {
            "kind": "primary-function",
            "address": address,
            "symbol_id": f"recoil:function:{address}",
        }
        for address in addresses
    )
    if dependency:
        relationships.append(
            {
                "kind": "depends-on-owner",
                "target_owner_id": dependency,
                "reason": "manual",
            }
        )
    row: dict[str, object] = {
        "address_metadata": {
            address: {
                "name": dict(ZBD_PROVIDER_FUNCTIONS).get(address, f"Function {address}"),
                "group": "engine.zutil",
            }
            for address in addresses
        },
        "binary": "recoil",
        "blocker": "none",
        "evidence_ids": [],
        "gates": {
            "boundary": "accepted",
            "byte": "deferred",
            "data": "none",
            "functional": "accepted" if kind != "provider-boundary" else "none",
            "owner_linkage": "accepted" if kind != "provider-boundary" else "none",
            "source": "accepted",
        },
        "kind": kind,
        "legacy_id": owner_id.split("recoil:owner:", 1)[-1],
        "lifecycle_state": "accepted",
        "name": owner_id,
        "provider_state": "accepted" if kind == "provider-boundary" else "pending",
        "relationships": relationships,
        "section": "provider.compiler" if kind == "provider-boundary" else "core_util_archive",
        "source_paths": ["provider:msvc5-list"] if kind == "provider-boundary" else ["zZbd.cpp"],
    }
    if kind != "provider-boundary":
        row["reimplementation"] = {
            "entries": {
                f"recoil:function:{address}": {
                    "kind": "function",
                    "tier": "B",
                    "evidence_ids": [],
                }
                for address in addresses
            }
        }
    return row


def owner_downgrade_fixture() -> tuple[dict[str, object], dict[str, object]]:
    data = empty_progress_document()
    data["revision"] = 1330
    symbol_id = "recoil:function:0x407700"
    owner_id = "recoil:owner:engine.zgame.options_load_game_options"
    data["symbols"][symbol_id] = {
        "address": "0x407700",
        "binary": "recoil",
        "kind": "function",
        "navigation_name": "zGame::Options_LoadGameOptions",
        "ownership_state": "primary-owned",
        "pipeline_class": "authored",
        "authored_order_role": "authored-body",
    }
    owner = _owner_fixture_row(
        owner_id=owner_id,
        kind="subsystem",
        addresses=["0x407700"],
    )
    owner["gates"]["source"] = "accepted"
    owner["gates"]["data"] = "accepted"
    owner["reimplementation"]["entries"][symbol_id]["tier"] = "B"
    data["owners"][owner_id] = owner
    unrelated_owner_id = "recoil:owner:unrelated"
    unrelated_owner = _owner_fixture_row(
        owner_id=unrelated_owner_id,
        kind="subsystem",
        addresses=[],
    )
    unrelated_owner["address_metadata"] = {}
    unrelated_owner["reimplementation"] = {"entries": {}}
    data["owners"][unrelated_owner_id] = unrelated_owner
    payload: dict[str, object] = {
        "schema": "recoil-owner-downgrade-v1",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": (
            "Live call-contract source contradiction invalidated the accepted "
            "source/data gates and tier-B entry."
        ),
        "binary": "recoil",
        "owner_id": owner_id,
        "current_gates": {
            "source": "accepted",
            "data": "accepted",
        },
        "new_gates": {
            "source": "blocked",
            "data": "blocked",
        },
        "current_entry_tiers": {
            symbol_id: "B",
        },
        "new_entry_tiers": {
            symbol_id: "X",
        },
    }
    return data, payload


def zbd_owner_replace_fixture() -> tuple[dict[str, object], dict[str, object]]:
    data = empty_progress_document()
    data["revision"] = 1307
    for address, name in ZBD_PROVIDER_FUNCTIONS:
        data["symbols"][f"recoil:function:{address}"] = {
            "address": address,
            "binary": "recoil",
            "kind": "function",
            "navigation_name": name,
            "pipeline_class": "non-authored",
            "authored_order_role": "non-authored",
        }
    manager_id = "recoil:owner:core_util_archive.zzbd_manager_core_class"
    list_id = "recoil:owner:core_util_archive.zzbd_section_handler_list_class"
    provider_id = "recoil:owner:provider.msvc5_std_list_zzbd_section_handler_instantiation"
    manager = _owner_fixture_row(
        owner_id=manager_id,
        kind="class",
        addresses=["0x4c07d0"],
    )
    list_owner = _owner_fixture_row(
        owner_id=list_id,
        kind="class",
        addresses=[address for address, _name in ZBD_PROVIDER_FUNCTIONS[1:]],
        dependency=manager_id,
    )
    data["owners"][manager_id] = deepcopy(manager)
    data["owners"][list_id] = deepcopy(list_owner)

    manager_replacement = deepcopy(manager)
    manager_replacement["relationships"] = [
        row
        for row in manager_replacement["relationships"]
        if row.get("kind") != "primary-function"
    ]
    manager_replacement["address_metadata"] = {}
    manager_replacement["reimplementation"] = {"entries": {}}
    provider = _owner_fixture_row(
        owner_id=provider_id,
        kind="provider-boundary",
        addresses=[address for address, _name in ZBD_PROVIDER_FUNCTIONS],
    )
    payload: dict[str, object] = {
        "schema": "recoil-owner-replace-batch-v1",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": "Reviewed VC5 std::list provider-instantiation replacement fixture.",
        "binary": "recoil",
        "current_owners": [
            {"id": manager_id, "record": deepcopy(manager)},
            {"id": list_id, "record": deepcopy(list_owner)},
        ],
        "replacement_owners": [
            {"id": manager_id, "record": manager_replacement},
            {"id": provider_id, "record": provider},
        ],
    }
    return data, payload


def ui_vector_owner_replace_fixture() -> tuple[dict[str, object], dict[str, object]]:
    data = empty_progress_document()
    data["revision"] = 1309
    function_rows = (
        ("0x4bbfa0", "std::vector<HudUiCompositePanelEntry>::~vector"),
        ("0x4bbff0", "std::vector<HudUiCompositePanelEntry>::insert"),
    )
    for address, name in function_rows:
        data["symbols"][f"recoil:function:{address}"] = {
            "address": address,
            "binary": "recoil",
            "kind": "function",
            "navigation_name": name,
            "pipeline_class": "non-authored",
            "authored_order_role": "non-authored",
        }
    vector_id = "recoil:owner:legacy.hud_ui.struct_huduicompositepanelvector"
    panel_id = "recoil:owner:legacy.hud_ui.class_huduicompositepanel"
    entry_id = "recoil:owner:legacy.hud_ui.struct_huduicompositepanelentry"
    provider_id = "recoil:owner:provider.compiler.vc5_huduicompositepanelentry_vector_comdats"
    entry_owner = _owner_fixture_row(owner_id=entry_id, kind="record", addresses=[])
    vector = _owner_fixture_row(
        owner_id=vector_id,
        kind="record",
        addresses=[address for address, _name in function_rows],
        dependency=entry_id,
    )
    panel = _owner_fixture_row(
        owner_id=panel_id,
        kind="class",
        addresses=[],
        dependency=vector_id,
    )
    data["owners"][entry_id] = entry_owner
    data["owners"][vector_id] = deepcopy(vector)
    data["owners"][panel_id] = deepcopy(panel)

    panel_replacement = deepcopy(panel)
    panel_replacement["relationships"][0]["target_owner_id"] = provider_id
    panel_replacement["relationships"][0]["reason"] = "provider-boundary"
    provider = _owner_fixture_row(
        owner_id=provider_id,
        kind="provider-boundary",
        addresses=[address for address, _name in function_rows],
    )
    provider["name"] = "VC5 STL std::vector<HudUiCompositePanelEntry> destructor/insert COMDATs"
    provider["source_paths"] = []
    payload: dict[str, object] = {
        "schema": "recoil-owner-replace-batch-v1",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": "Reviewed VC5 std::vector provider replacement with incoming panel dependency retarget.",
        "binary": "recoil",
        "current_owners": [
            {"id": vector_id, "record": deepcopy(vector)},
            {"id": panel_id, "record": deepcopy(panel)},
        ],
        "replacement_owners": [
            {"id": panel_id, "record": panel_replacement},
            {"id": provider_id, "record": provider},
        ],
    }
    return data, payload


def appmodul_owner_replace_fixture() -> tuple[dict[str, object], dict[str, object]]:
    data = empty_progress_document()
    data["revision"] = 1317
    data["output_sections"] = {
        "recoil:section:.data": {"binary": "recoil", "name": ".data"}
    }
    function_addresses = (
        "0x4c81c0", "0x4c81d8", "0x4c8201", "0x4c8214", "0x4c8224"
    )
    for address in function_addresses:
        data["symbols"][f"recoil:function:{address}"] = {
            "address": address,
            "binary": "recoil",
            "kind": "function",
            "navigation_name": f"APPMODUL fixture {address}",
            "ownership_state": (
                "primary-owned" if address in {"0x4c81c0", "0x4c8214"} else "unresolved"
            ),
        }
    existing_data_id = "recoil:data:0x4da004"
    data["symbols"][existing_data_id] = {
        "address": "0x4da004",
        "binary": "recoil",
        "kind": "data",
        "extent_state": "unknown",
        "storage_contribution_ids": ["recoil:storage:va:0x4da004"],
    }
    data["storage_contributions"]["recoil:storage:va:0x4da004"] = {
        "binary": "recoil",
        "owner_ids": ["recoil:owner:provider.mfc.crt_static_mbcp_initializer"],
        "reference": {"address": "0x4da004", "extent_state": "unknown"},
        "symbol_ids": [existing_data_id],
    }
    win_owner_id = "recoil:owner:misc.authored_stubs_winmain"
    mbcp_owner_id = "recoil:owner:provider.mfc.crt_static_mbcp_initializer"
    ordinal_owner_id = "recoil:owner:provider.mfc42.afxwinmain_ordinal_1576_import"
    app_owner_id = "recoil:owner:provider.mfc.appmodul_cpp_application_startup"
    win_owner = _owner_fixture_row(
        owner_id=win_owner_id, kind="source-file", addresses=["0x4c81c0"]
    )
    mbcp_owner = _owner_fixture_row(
        owner_id=mbcp_owner_id, kind="provider-boundary", addresses=["0x4c8214"]
    )
    mbcp_owner["relationships"].append(
        {
            "kind": "primary-data",
            "address": "0x4da004",
            "symbol_id": existing_data_id,
            "name": "g_CrtInitFn_AfxInitialize",
        }
    )
    ordinal_owner = _owner_fixture_row(
        owner_id=ordinal_owner_id, kind="provider-boundary", addresses=[]
    )
    data["owners"][win_owner_id] = deepcopy(win_owner)
    data["owners"][mbcp_owner_id] = deepcopy(mbcp_owner)
    data["owners"][ordinal_owner_id] = ordinal_owner

    app_owner = _owner_fixture_row(
        owner_id=app_owner_id,
        kind="provider-boundary",
        addresses=list(function_addresses),
        dependency=ordinal_owner_id,
    )
    app_owner["legacy_id"] = "provider.mfc.appmodul_cpp_application_startup"
    app_owner["name"] = "MFC APPMODUL.CPP application startup"
    app_owner["section"] = "provider.mfc"
    app_owner["source_paths"] = []
    app_owner["relationships"][-1]["reason"] = "provider-boundary"
    app_owner["relationships"].extend(
        [
            {
                "kind": "primary-data",
                "address": "0x4da004",
                "symbol_id": existing_data_id,
                "name": "g_CrtInitFn_AfxInitialize",
            },
            {
                "kind": "primary-data",
                "address": "0x56cc28",
                "symbol_id": "recoil:data:0x56cc28",
                "name": "_afxTermAppState",
            },
        ]
    )
    payload: dict[str, object] = {
        "schema": "recoil-owner-replace-batch-v2",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": "Reviewed APPMODUL.CPP provider graph fixture.",
        "binary": "recoil",
        "current_owners": [
            {"id": win_owner_id, "record": deepcopy(win_owner)},
            {"id": mbcp_owner_id, "record": deepcopy(mbcp_owner)},
        ],
        "replacement_owners": [{"id": app_owner_id, "record": app_owner}],
        "primary_function_bootstraps": [
            {
                "reviewed": True,
                "symbol_id": f"recoil:function:{address}",
                "address": address,
                "current_ownership_state": "unresolved",
                "new_owner_id": app_owner_id,
            }
            for address in ("0x4c81d8", "0x4c8201", "0x4c8224")
        ],
        "primary_data_reassignments": [
            {
                "reviewed": True,
                "symbol_id": existing_data_id,
                "address": "0x4da004",
                "current_owner_id": mbcp_owner_id,
                "current_ownership_state": None,
                "new_owner_id": app_owner_id,
            },
            {
                "reviewed": True,
                "symbol_id": "recoil:data:0x56cc28",
                "address": "0x56cc28",
                "current_owner_id": None,
                "current_ownership_state": None,
                "new_owner_id": app_owner_id,
            },
        ],
        "unknown_data_symbol_bootstraps": [
            {
                "reviewed": True,
                "symbol_id": "recoil:data:0x56cc28",
                "address": "0x56cc28",
                "navigation_name": "_afxTermAppState",
                "disposition": "provider",
                "output_section_id": "recoil:section:.data",
            }
        ],
    }
    return data, payload


def data_only_owner_replace_fixture() -> tuple[dict[str, object], dict[str, object]]:
    data = empty_progress_document()
    data["revision"] = 1340
    data["output_sections"] = {
        "recoil:section:.data": {"binary": "recoil", "name": ".data"}
    }
    old_owner_id = "recoil:owner:provider.runtime.legacy_data_owner"
    new_owner_id = "recoil:owner:provider.runtime.reviewed_data_owner"
    existing_data_id = "recoil:data:0x510000"
    unknown_data_id = "recoil:data:0x510004"
    storage_id = "recoil:storage:va:0x510000"
    data["symbols"][existing_data_id] = {
        "address": "0x510000",
        "binary": "recoil",
        "kind": "data",
        "extent_state": "unknown",
        "ownership_state": "primary-owned",
        "storage_contribution_ids": [storage_id],
    }
    data["storage_contributions"][storage_id] = {
        "binary": "recoil",
        "owner_ids": [old_owner_id],
        "reference": {"address": "0x510000", "extent_state": "unknown"},
        "symbol_ids": [existing_data_id],
    }
    old_owner = _owner_fixture_row(
        owner_id=old_owner_id,
        kind="provider-boundary",
        addresses=[],
    )
    old_owner["relationships"].append(
        {
            "kind": "primary-data",
            "address": "0x510000",
            "symbol_id": existing_data_id,
            "name": "g_ExistingRuntimeData",
        }
    )
    new_owner = _owner_fixture_row(
        owner_id=new_owner_id,
        kind="provider-boundary",
        addresses=[],
    )
    new_owner["relationships"].extend(
        [
            {
                "kind": "primary-data",
                "address": "0x510000",
                "symbol_id": existing_data_id,
                "name": "g_ExistingRuntimeData",
            },
            {
                "kind": "primary-data",
                "address": "0x510004",
                "symbol_id": unknown_data_id,
                "name": "g_UnknownRuntimeData",
            },
        ]
    )
    data["owners"][old_owner_id] = deepcopy(old_owner)
    payload: dict[str, object] = {
        "schema": "recoil-owner-replace-batch-v2",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": "Reviewed data-only owner replacement fixture.",
        "binary": "recoil",
        "current_owners": [{"id": old_owner_id, "record": deepcopy(old_owner)}],
        "replacement_owners": [{"id": new_owner_id, "record": new_owner}],
        "primary_function_bootstraps": [],
        "primary_data_reassignments": [
            {
                "reviewed": True,
                "symbol_id": existing_data_id,
                "address": "0x510000",
                "current_owner_id": old_owner_id,
                "current_ownership_state": "primary-owned",
                "new_owner_id": new_owner_id,
            },
            {
                "reviewed": True,
                "symbol_id": unknown_data_id,
                "address": "0x510004",
                "current_owner_id": None,
                "current_ownership_state": None,
                "new_owner_id": new_owner_id,
            },
        ],
        "unknown_data_symbol_bootstraps": [
            {
                "reviewed": True,
                "symbol_id": unknown_data_id,
                "address": "0x510004",
                "navigation_name": "g_UnknownRuntimeData",
                "disposition": "provider",
                "output_section_id": "recoil:section:.data",
            }
        ],
    }
    return data, payload


def retained_owner_data_tier_move_fixture() -> tuple[
    dict[str, object], dict[str, object]
]:
    data = empty_progress_document()
    data["revision"] = 1334
    source_owner_id = (
        "recoil:owner:network_online.gamenet_refresh_player_list_menu"
    )
    destination_owner_id = "recoil:owner:hud_ui.hud_ui_mgr_data"
    moved_symbol_id = "recoil:data:0x4ed4e0"
    retained_symbol_id = "recoil:function:0x401000"
    storage_id = "recoil:storage:va:0x4ed4e0"
    data["symbols"][moved_symbol_id] = {
        "address": "0x4ed4e0",
        "binary": "recoil",
        "kind": "data",
        "extent_state": "unknown",
        "ownership_state": "primary-owned",
        "storage_contribution_ids": [storage_id],
    }
    data["symbols"][retained_symbol_id] = {
        "address": "0x401000",
        "binary": "recoil",
        "kind": "function",
        "ownership_state": "primary-owned",
    }
    data["storage_contributions"][storage_id] = {
        "binary": "recoil",
        "owner_ids": [source_owner_id],
        "reference": {"address": "0x4ed4e0", "extent_state": "unknown"},
        "symbol_ids": [moved_symbol_id],
    }

    source_owner = _owner_fixture_row(
        owner_id=source_owner_id,
        kind="subsystem",
        addresses=[],
    )
    moved_relationship = {
        "kind": "primary-data",
        "address": "0x4ed4e0",
        "symbol_id": moved_symbol_id,
        "name": "g_GameNetRefreshPlayerListMenu",
    }
    moved_tier = {
        "kind": "data",
        "tier": "X",
        "evidence_ids": ["recoil:evidence:retained-data-tier-x"],
    }
    source_owner["relationships"].append(deepcopy(moved_relationship))
    source_owner["reimplementation"]["entries"][moved_symbol_id] = deepcopy(
        moved_tier
    )

    destination_owner = _owner_fixture_row(
        owner_id=destination_owner_id,
        kind="subsystem",
        addresses=["0x401000"],
    )
    data["owners"][source_owner_id] = deepcopy(source_owner)
    data["owners"][destination_owner_id] = deepcopy(destination_owner)

    source_replacement = deepcopy(source_owner)
    source_replacement["relationships"] = []
    source_replacement["reimplementation"]["entries"] = {}
    destination_replacement = deepcopy(destination_owner)
    destination_replacement["relationships"].append(
        deepcopy(moved_relationship)
    )
    destination_replacement["reimplementation"]["entries"][
        moved_symbol_id
    ] = deepcopy(moved_tier)
    payload: dict[str, object] = {
        "schema": "recoil-owner-replace-batch-v2",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": (
            "Reviewed retained-owner primary-data reassignment with its exact "
            "unchanged tier-X record."
        ),
        "binary": "recoil",
        "current_owners": [
            {"id": source_owner_id, "record": deepcopy(source_owner)},
            {
                "id": destination_owner_id,
                "record": deepcopy(destination_owner),
            },
        ],
        "replacement_owners": [
            {"id": source_owner_id, "record": source_replacement},
            {
                "id": destination_owner_id,
                "record": destination_replacement,
            },
        ],
        "primary_function_bootstraps": [],
        "primary_data_reassignments": [
            {
                "reviewed": True,
                "symbol_id": moved_symbol_id,
                "address": "0x4ed4e0",
                "current_owner_id": source_owner_id,
                "current_ownership_state": "primary-owned",
                "new_owner_id": destination_owner_id,
            }
        ],
        "unknown_data_symbol_bootstraps": [],
    }
    return data, payload


def retained_owner_unowned_authored_data_tier_x_fixture() -> tuple[
    dict[str, object], dict[str, object]
]:
    data = empty_progress_document()
    data["revision"] = 1360
    owner_id = "recoil:owner:hud_ui.hud_ui_timer_panel_float_class"
    data_symbol_id = "recoil:data:0x4ce7d8"
    function_symbol_id = "recoil:function:0x40ef60"
    data["symbols"][data_symbol_id] = {
        "address": "0x4ce7d8",
        "binary": "recoil",
        "disposition": "authored",
        "extent_state": "known",
        "kind": "data",
        "storage_contribution_ids": [],
    }
    data["symbols"][function_symbol_id] = {
        "address": "0x40ef60",
        "binary": "recoil",
        "kind": "function",
        "ownership_state": "primary-owned",
    }
    owner = _owner_fixture_row(
        owner_id=owner_id,
        kind="class",
        addresses=["0x40ef60"],
    )
    owner["reimplementation"]["entries"][function_symbol_id] = {
        "kind": "function",
        "tier": "B",
        "evidence_ids": ["recoil:evidence:retained-function-tier-b"],
    }
    data["owners"][owner_id] = deepcopy(owner)

    replacement = deepcopy(owner)
    replacement["relationships"].append(
        {
            "address": "0x4ce7d8",
            "kind": "primary-data",
            "name": "g_HudUiTimerPanelFloat_FTable",
            "symbol_id": data_symbol_id,
        }
    )
    replacement["reimplementation"]["entries"][data_symbol_id] = {
        "kind": "data",
        "tier": "X",
        "evidence_ids": [],
    }
    payload: dict[str, object] = {
        "schema": "recoil-owner-replace-batch-v2",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": (
            "Reviewed exact formerly-unowned authored primary-data assignment "
            "with non-promoting tier-X bookkeeping."
        ),
        "binary": "recoil",
        "current_owners": [{"id": owner_id, "record": deepcopy(owner)}],
        "replacement_owners": [{"id": owner_id, "record": replacement}],
        "primary_function_bootstraps": [],
        "primary_data_reassignments": [
            {
                "reviewed": True,
                "symbol_id": data_symbol_id,
                "address": "0x4ce7d8",
                "current_owner_id": None,
                "current_ownership_state": None,
                "new_owner_id": owner_id,
            }
        ],
        "unknown_data_symbol_bootstraps": [],
    }
    return data, payload


def provider_block_reclassify_fixture() -> tuple[dict[str, object], dict[str, object]]:
    data = empty_progress_document()
    data["revision"] = 1351
    data["binaries"]["recoil"] = {
        "binary": "recoil",
        "primary_scheduler": True,
        "text": {"start": "0x4c81c0", "end_exclusive": "0x4c81e8"},
    }
    block_id = "recoil:block:0x4c81c0"
    symbol_id = "recoil:function:0x4c81c0"
    span_id = "recoil:semantic:0x4c81c0-0x4c81d8"
    owner_id = "recoil:owner:provider.mfc.appmodul_cpp_application_startup"
    data["physical_blocks"][block_id] = {
        "accepted_order_facts": None,
        "agent_source_path": "src/WinMain.cpp",
        "binary": "recoil",
        "contribution_ids": [symbol_id],
        "contribution_kind": "authored",
        "end_exclusive": "0x4c81d8",
        "first_external_callers": [],
        "mapping": {
            "confidence": (
                "medium entry-thunk semantics; low exact source filename confidence. "
                "Retail WinMain wrapper has no WinMain.cpp source-path literal."
            ),
            "evidence_ids": ["recoil:evidence:r725:019021"],
            "file_literal": None,
            "literal_xrefs": [],
            "state": "unresolved",
            "status": "mapped-no-literal-entry-thunk-candidate",
        },
        "order": _pending_order(),
        "order_diagnostic": {"navigation-only": "preserve"},
        "order_diagnostics": {"sentinel": {"result": "pending"}},
        "order_targets": {"linked": "", "object": ""},
        "original_source_path": None,
        "provisional_original_path": "src/WinMain.cpp",
        "row_kind": "physical-source-block",
        "semantic_span_ids": [span_id],
        "source_path": "src/WinMain.cpp",
        "source_shape_inputs": [],
        "candidate_header_contributors": [],
        "start": "0x4c81c0",
    }
    data["semantic_spans"][span_id] = {
        "binary": "recoil",
        "confidence": "medium entry-thunk semantics; source filename unresolved",
        "end_exclusive": "0x4c81d8",
        "evidence_ids": ["recoil:evidence:r725:004783"],
        "navigation_aliases": [],
        "physical_block_id": block_id,
        "source_path": "tail-authored-island:WinMain",
        "start": "0x4c81c0",
        "status": "mapped-no-literal-audited-single",
        "symbol_ids": [symbol_id],
    }
    data["symbols"][symbol_id] = {
        "accepted_byte_facts": {"sentinel": "preserve"},
        "accepted_order_facts": None,
        "address": "0x4c81c0",
        "authored_order_role": "non-authored",
        "binary": "recoil",
        "binary_state": {"sentinel": {"result": "pending"}},
        "disposition": "unresolved",
        "end_exclusive": "0x4c81d8",
        "extent_state": "known",
        "kind": "function",
        "navigation_name": "WinMain",
        "output_section_id": "recoil:section:.text",
        "ownership_state": "primary-owned",
        "physical_block_id": block_id,
        "pipeline_class": "non-authored",
        "semantic_span_ids": [span_id],
        "size": 24,
        "storage_contribution_ids": [],
        "verification_target_ids": ["recoil:vc5-target:winmain"],
    }
    provider = _owner_fixture_row(
        owner_id=owner_id,
        kind="provider-boundary",
        addresses=["0x4c81c0"],
    )
    provider["name"] = "MFC APPMODUL.CPP application startup"
    provider["section"] = "provider.mfc"
    provider["source_paths"] = []
    data["owners"][owner_id] = provider
    data["output_sections"]["recoil:section:.text"] = {
        "binary": "recoil",
        "name": ".text",
        "sentinel": "preserve",
    }
    data["storage_contributions"]["recoil:storage:sentinel"] = {
        "binary": "recoil",
        "owner_ids": [owner_id],
        "symbol_ids": [],
    }
    successor_block_id = "recoil:block:0x4c81d8"
    successor_symbol_id = "recoil:function:0x4c81d8"
    data["physical_blocks"][successor_block_id] = {
        "accepted_order_facts": None,
        "agent_source_path": "src/tail.cpp",
        "binary": "recoil",
        "contribution_ids": [successor_symbol_id],
        "contribution_kind": "authored",
        "end_exclusive": "0x4c81e8",
        "mapping": {"confidence": "high authored", "state": "accepted", "status": "mapped"},
        "order": _pending_order(),
        "order_targets": {"linked": "", "object": ""},
        "semantic_span_ids": [],
        "source_path": "src/tail.cpp",
        "start": "0x4c81d8",
    }
    data["symbols"][successor_symbol_id] = {
        "address": "0x4c81d8",
        "authored_order_role": "authored-body",
        "binary": "recoil",
        "binary_state": {},
        "end_exclusive": "0x4c81e8",
        "kind": "function",
        "physical_block_id": successor_block_id,
        "pipeline_class": "authored",
        "semantic_span_ids": [],
    }
    payload: dict[str, object] = {
        "schema": "recoil-provider-block-reclassify-v1",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": (
            "Reviewed APPMODUL.CPP provider ownership supersedes the stale provisional "
            "WinMain.cpp physical placement."
        ),
        "binary": "recoil",
        "block_id": block_id,
        "current_block": deepcopy(data["physical_blocks"][block_id]),
        "expected_provider_owner_ids": [owner_id],
        "clear_provisional_compile_source_placement": True,
        "replacement": {
            "contribution_kind": "provider",
            "source_path": "provider:mfc42-appmodul-cpp-application-startup",
            "agent_source_path": "provider:mfc42-appmodul-cpp-application-startup",
            "provisional_original_path": None,
            "mapping_status": "provider-boundary",
            "mapping_confidence": (
                "high provider MFC APPMODUL.CPP WinMain/AfxWinMain forwarding wrapper; "
                "no authored Recoil source placement"
            ),
        },
    }
    return data, payload


class ProviderBlockReclassificationTests(unittest.TestCase):
    def test_exact_4c81c0_reclassifies_only_reviewed_mapping_fields(self) -> None:
        data, raw_payload = provider_block_reclassify_fixture()
        before = deepcopy(data)
        payload = _parse_provider_block_reclassify_payload(json.dumps(raw_payload))

        details = _reclassify_provider_block(data, payload)

        block_id = "recoil:block:0x4c81c0"
        block = data["physical_blocks"][block_id]
        self.assertEqual("provider", block["contribution_kind"])
        self.assertEqual(
            "provider:mfc42-appmodul-cpp-application-startup", block["source_path"]
        )
        self.assertEqual(block["source_path"], block["agent_source_path"])
        self.assertIsNone(block["provisional_original_path"])
        self.assertEqual("provider-boundary", block["mapping"]["status"])
        self.assertTrue(block["mapping"]["confidence"].startswith("high "))
        self.assertIsNone(block["original_source_path"])
        self.assertEqual("unresolved", block["mapping"]["state"])
        self.assertEqual(
            ["recoil:evidence:r725:019021"], block["mapping"]["evidence_ids"]
        )
        self.assertEqual(before["semantic_spans"], data["semantic_spans"])
        self.assertEqual(before["symbols"], data["symbols"])
        self.assertEqual(before["owners"], data["owners"])
        self.assertEqual(before["storage_contributions"], data["storage_contributions"])
        self.assertEqual(before["output_sections"], data["output_sections"])
        self.assertEqual(before["evidence"], data["evidence"])
        self.assertEqual(before["id_sequences"], data["id_sequences"])
        self.assertEqual("0x4c81c0", details["scheduler_before"]["cursor"])
        self.assertEqual(details["scheduler_before"], details["scheduler_after"])
        self.assertTrue(all(details["invariants"].values()))

    def test_rejects_stale_classification_owner_provenance_target_and_work(self) -> None:
        cases: list[tuple[dict[str, object], dict[str, object], str]] = []

        data, payload = provider_block_reclassify_fixture()
        payload["current_block"]["mapping"]["confidence"] = "stale"
        cases.append((data, payload, "snapshot is stale"))

        data, payload = provider_block_reclassify_fixture()
        data["symbols"]["recoil:function:0x4c81c0"]["pipeline_class"] = "unresolved"
        cases.append((data, payload, "resolved non-authored/non-authored"))

        data, payload = provider_block_reclassify_fixture()
        data["symbols"]["recoil:function:0x4c81c0"]["ownership_state"] = "unresolved"
        cases.append((data, payload, "ownership_state='primary-owned'"))

        data, payload = provider_block_reclassify_fixture()
        owner = data["owners"][
            "recoil:owner:provider.mfc.appmodul_cpp_application_startup"
        ]
        owner["provider_state"] = "pending"
        cases.append((data, payload, "not an accepted provider-boundary owner"))

        data, payload = provider_block_reclassify_fixture()
        payload["expected_provider_owner_ids"] = ["recoil:owner:provider.mfc.other"]
        cases.append((data, payload, "expected_provider_owner_ids is stale"))

        data, payload = provider_block_reclassify_fixture()
        data["physical_blocks"]["recoil:block:0x4c81c0"]["original_source_path"] = (
            "src/Accepted.cpp"
        )
        payload["current_block"] = deepcopy(
            data["physical_blocks"]["recoil:block:0x4c81c0"]
        )
        cases.append((data, payload, "accepted original-source provenance"))

        data, payload = provider_block_reclassify_fixture()
        data["physical_blocks"]["recoil:block:0x4c81c0"]["mapping"]["state"] = "accepted"
        payload["current_block"] = deepcopy(
            data["physical_blocks"]["recoil:block:0x4c81c0"]
        )
        cases.append((data, payload, "must remain unaccepted/unresolved"))

        data, payload = provider_block_reclassify_fixture()
        data["physical_blocks"]["recoil:block:0x4c81c0"]["order_targets"]["object"] = (
            "winmain"
        )
        payload["current_block"] = deepcopy(
            data["physical_blocks"]["recoil:block:0x4c81c0"]
        )
        cases.append((data, payload, "active configured order target"))

        data, payload = provider_block_reclassify_fixture()
        data["work_items"]["recoil:work:conflict"] = {
            "state": "active",
            "reservation": {"state": "active"},
            "resource_claims": [{"kind": "tracker", "id": "recoil", "access": "read"}],
        }
        cases.append((data, payload, "conflicts with active work"))

        for data, raw_payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(ProgressError, message):
                payload = _parse_provider_block_reclassify_payload(json.dumps(raw_payload))
                _reclassify_provider_block(data, payload)

    def test_parent_route_is_dry_run_apply_atomic_and_revision_guarded(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            data, payload = provider_block_reclassify_fixture()
            path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            arguments = [
                "block",
                "reclassify-provider",
                "--progress",
                str(path),
                "--payload-json",
                json.dumps(payload),
                "--expected-revision",
                "1351",
                "--dry-run",
                "--json",
            ]
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                dry_rc = progress_cli.main(arguments)
            self.assertEqual(0, dry_rc, stderr.getvalue())
            dry_result = json.loads(stdout.getvalue())
            self.assertFalse(dry_result["commit"]["applied"])
            self.assertEqual(1351, json.loads(path.read_text(encoding="utf-8"))["revision"])

            arguments[arguments.index("--dry-run")] = "--apply"
            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                apply_rc = progress_cli.main(arguments)
            self.assertEqual(0, apply_rc)
            applied = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(1352, applied["revision"])
            self.assertEqual(
                "provider",
                applied["physical_blocks"]["recoil:block:0x4c81c0"]["contribution_kind"],
            )
            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                stale_rc = progress_cli.main(arguments)
            self.assertEqual(2, stale_rc)


class OwnerDowngradeTests(unittest.TestCase):
    def test_owner_downgrade_is_atomic_and_preserves_unrelated_state(self) -> None:
        data, raw_payload = owner_downgrade_fixture()
        before = deepcopy(data)
        payload = _parse_owner_downgrade_payload(json.dumps(raw_payload))
        details = _downgrade_owner(data, payload)

        owner_id = str(raw_payload["owner_id"])
        symbol_id = "recoil:function:0x407700"
        owner = data["owners"][owner_id]
        evidence_id = details["evidence_id"]
        self.assertEqual("blocked", owner["gates"]["source"])
        self.assertEqual("blocked", owner["gates"]["data"])
        self.assertEqual(
            "X",
            owner["reimplementation"]["entries"][symbol_id]["tier"],
        )
        self.assertIn(evidence_id, owner["evidence_ids"])
        self.assertIn(
            evidence_id,
            owner["reimplementation"]["entries"][symbol_id]["evidence_ids"],
        )
        self.assertEqual("current", data["evidence"][evidence_id]["freshness"])
        self.assertEqual("live", data["evidence"][evidence_id]["validation_mode"])
        self.assertEqual(
            before["owners"]["recoil:owner:unrelated"],
            data["owners"]["recoil:owner:unrelated"],
        )
        for collection in (
            "binaries",
            "physical_blocks",
            "semantic_spans",
            "symbols",
            "storage_contributions",
            "output_sections",
            "verification_targets",
            "work_items",
            "blockers",
        ):
            self.assertEqual(before[collection], data[collection], collection)
        self.assertTrue(details["unrelated_state_preserved"])

    def test_owner_downgrade_rejects_stale_gate_or_entry_tier(self) -> None:
        for label, mutate, message in (
            (
                "gate",
                lambda data: data["owners"][
                    "recoil:owner:engine.zgame.options_load_game_options"
                ]["gates"].__setitem__("source", "pending"),
                "current state is stale",
            ),
            (
                "tier",
                lambda data: data["owners"][
                    "recoil:owner:engine.zgame.options_load_game_options"
                ]["reimplementation"]["entries"][
                    "recoil:function:0x407700"
                ].__setitem__("tier", "C"),
                "current tier is stale",
            ),
        ):
            data, raw_payload = owner_downgrade_fixture()
            mutate(data)
            payload = _parse_owner_downgrade_payload(json.dumps(raw_payload))
            with self.subTest(label=label), self.assertRaisesRegex(
                ProgressError, message
            ):
                _downgrade_owner(data, payload)

    def test_owner_downgrade_rejects_empty_or_noop_payload(self) -> None:
        _data, raw_payload = owner_downgrade_fixture()
        raw_payload["current_gates"] = {}
        raw_payload["new_gates"] = {}
        raw_payload["current_entry_tiers"] = {}
        raw_payload["new_entry_tiers"] = {}
        with self.assertRaisesRegex(ProgressError, "at least one"):
            _parse_owner_downgrade_payload(json.dumps(raw_payload))

        _data, raw_payload = owner_downgrade_fixture()
        raw_payload["new_gates"]["source"] = "accepted"
        with self.assertRaisesRegex(ProgressError, "not a conservative downgrade"):
            _parse_owner_downgrade_payload(json.dumps(raw_payload))

    def test_owner_downgrade_rejects_gate_or_tier_promotion(self) -> None:
        _data, raw_payload = owner_downgrade_fixture()
        raw_payload["current_gates"] = {"source": "blocked"}
        raw_payload["new_gates"] = {"source": "accepted"}
        raw_payload["current_entry_tiers"] = {}
        raw_payload["new_entry_tiers"] = {}
        with self.assertRaisesRegex(ProgressError, "not a conservative downgrade"):
            _parse_owner_downgrade_payload(json.dumps(raw_payload))

        _data, raw_payload = owner_downgrade_fixture()
        raw_payload["current_gates"] = {}
        raw_payload["new_gates"] = {}
        raw_payload["new_entry_tiers"]["recoil:function:0x407700"] = "A"
        with self.assertRaisesRegex(ProgressError, "not a strict tier downgrade"):
            _parse_owner_downgrade_payload(json.dumps(raw_payload))

    def test_owner_downgrade_rejects_provider_or_non_primary_entry(self) -> None:
        data, raw_payload = owner_downgrade_fixture()
        owner_id = str(raw_payload["owner_id"])
        data["owners"][owner_id]["kind"] = "provider-boundary"
        payload = _parse_owner_downgrade_payload(json.dumps(raw_payload))
        with self.assertRaisesRegex(ProgressError, "provider-boundary"):
            _downgrade_owner(data, payload)

        data, raw_payload = owner_downgrade_fixture()
        owner = data["owners"][str(raw_payload["owner_id"])]
        non_primary = "recoil:function:0x407710"
        owner["reimplementation"]["entries"][non_primary] = {
            "kind": "function",
            "tier": "B",
            "evidence_ids": [],
        }
        raw_payload["current_gates"] = {}
        raw_payload["new_gates"] = {}
        raw_payload["current_entry_tiers"] = {non_primary: "B"}
        raw_payload["new_entry_tiers"] = {non_primary: "X"}
        payload = _parse_owner_downgrade_payload(json.dumps(raw_payload))
        with self.assertRaisesRegex(ProgressError, "is not a primary entry"):
            _downgrade_owner(data, payload)

    def test_owner_downgrade_dry_run_apply_and_revision_cas(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            data, payload = owner_downgrade_fixture()
            path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            arguments = [
                "owner",
                "downgrade",
                "--progress",
                str(path),
                "--payload-json",
                json.dumps(payload),
                "--expected-revision",
                "1330",
                "--dry-run",
                "--json",
            ]
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                dry_rc = progress_cli.main(arguments)
            self.assertEqual(0, dry_rc, stderr.getvalue())
            dry_result = json.loads(stdout.getvalue())
            self.assertFalse(dry_result["commit"]["applied"])
            self.assertEqual(
                1330,
                json.loads(path.read_text(encoding="utf-8"))["revision"],
            )

            arguments[arguments.index("--dry-run")] = "--apply"
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                apply_rc = progress_cli.main(arguments)
            self.assertEqual(0, apply_rc, stderr.getvalue())
            applied = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(1331, applied["revision"])
            self.assertEqual(
                "blocked",
                applied["owners"][str(payload["owner_id"])]["gates"]["source"],
            )

            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                stale_rc = progress_cli.main(arguments)
            self.assertEqual(2, stale_rc)


class OwnerReplaceBatchTests(unittest.TestCase):
    @staticmethod
    def _physical_detachment_fixture(
        *,
        symbol_id: str = "recoil:function:0x401000",
        address: str = "0x401000",
        owner_id: str = "recoil:owner:fixture.download",
        authored_order_role: str = "compiler-generated-icf-representative",
    ) -> tuple[dict, dict]:
        from tests.tools.recoil_cli_tests import RecoilCliTests

        data = RecoilCliTests.logical_alias_group_document()
        for owner_id_value, owner in data["owners"].items():
            owner.setdefault("name", owner_id_value)
            owner.setdefault("section", "fixture")
            owner.setdefault("lifecycle_state", "accepted")
            owner["gates"] = {
                "boundary": "accepted",
                "source": "accepted",
                "data": "none",
                "functional": "none",
                "byte": "deferred",
                "owner_linkage": "accepted",
            }
            owner.setdefault("relationships", [])
            owner.setdefault("address_metadata", {})
            owner.setdefault("reimplementation", {"entries": {}})
        if symbol_id not in data["symbols"]:
            data["symbols"][symbol_id] = deepcopy(
                data["symbols"]["recoil:function:0x401000"]
            )
        data["symbols"][symbol_id]["address"] = address
        data["symbols"][symbol_id]["authored_order_role"] = authored_order_role
        data["symbols"][symbol_id]["ownership_state"] = "primary-owned"
        if owner_id not in data["owners"]:
            owner = deepcopy(data["owners"]["recoil:owner:fixture.download"])
            owner["name"] = "zInput bind group info records"
            owner["legacy_id"] = "engine.zinput.bindgroup_info_records"
            data["owners"][owner_id] = owner
        owner = data["owners"][owner_id]
        owner["address_metadata"] = {
            address: {
                "name": (
                    "zInput_BindGroupInfoVec::Count"
                    if address == "0x42a9d0"
                    else "FoldedBody"
                ),
                "target": (
                    "zinput_bind_group_info_vec_count"
                    if address == "0x42a9d0"
                    else "folded_body"
                ),
            }
        }
        owner["relationships"] = [
            {
                "address": address,
                "kind": "primary-function",
                "symbol_id": symbol_id,
            }
        ]
        owner["reimplementation"] = {
            "entries": {
                symbol_id: {
                    "kind": "function",
                    "tier": "B",
                    "evidence_ids": ["recoil:evidence:r7:000001"],
                }
            }
        }
        replacement = deepcopy(owner)
        replacement["address_metadata"] = {}
        replacement["relationships"] = []
        replacement["reimplementation"] = {"entries": {}}
        payload = {
            "schema": "recoil-owner-replace-batch-v2",
            "reviewed": True,
            "parent_reviewed": True,
            "reason": "Detach one reviewed physical ICF representative from authored ownership.",
            "binary": "recoil",
            "current_owners": [{"id": owner_id, "record": deepcopy(owner)}],
            "replacement_owners": [{"id": owner_id, "record": replacement}],
            "primary_function_bootstraps": [],
            "primary_function_detachments": [
                {
                    "reviewed": True,
                    "symbol_id": symbol_id,
                    "address": address,
                    "current_owner_id": owner_id,
                    "current_ownership_state": "primary-owned",
                    "current_pipeline_class": "non-authored",
                    "current_authored_order_role": authored_order_role,
                }
            ],
            "primary_data_reassignments": [],
            "unknown_data_symbol_bootstraps": [],
        }
        return data, payload

    @staticmethod
    def _physical_icf_detachment_fixture() -> tuple[dict, dict]:
        return OwnerReplaceBatchTests._physical_detachment_fixture()

    @staticmethod
    def _zinput_count_detachment_fixture() -> tuple[dict, dict]:
        return OwnerReplaceBatchTests._physical_detachment_fixture(
            symbol_id="recoil:function:0x42a9d0",
            address="0x42a9d0",
            owner_id="recoil:owner:engine.zinput.bindgroup_info_records",
            authored_order_role="non-authored",
        )

    @staticmethod
    def _add_unrelated_missing_tier_debt(data: dict) -> str:
        owner_id = "recoil:owner:unrelated.preexisting-tier-debt"
        addresses = ["0x4cc838", "0x4cc83c"]
        owner = _owner_fixture_row(
            owner_id=owner_id,
            kind="subsystem",
            addresses=addresses,
        )
        owner["reimplementation"] = {"entries": {}}
        data["owners"][owner_id] = owner
        for address in addresses:
            symbol_id = f"recoil:function:{address}"
            data["symbols"][symbol_id] = {
                "address": address,
                "binary": "recoil",
                "kind": "function",
                "navigation_name": f"Unrelated debt {address}",
                "ownership_state": "primary-owned",
                "pipeline_class": "authored",
                "authored_order_role": "authored-body",
            }
        return owner_id

    def test_v2_detaches_exact_physical_icf_representative_and_tier(self) -> None:
        data, raw_payload = self._physical_icf_detachment_fixture()
        details = _replace_owner_batch(
            data,
            _parse_owner_replace_batch_payload(json.dumps(raw_payload)),
        )
        symbol_id = "recoil:function:0x401000"
        owner = data["owners"]["recoil:owner:fixture.download"]
        self.assertEqual([symbol_id], details["detached_primary_function_ids"])
        self.assertTrue(details["primary_detachments_reviewed"])
        self.assertFalse(details["primary_membership_preserved"])
        self.assertEqual("primary-owned", data["symbols"][symbol_id]["ownership_state"])
        self.assertEqual([], owner["relationships"])
        self.assertEqual({}, owner["reimplementation"]["entries"])
        self.assertEqual({}, owner["address_metadata"])

    def test_v2_detaches_exact_non_authored_zinput_count_and_preserves_symbol(self) -> None:
        data, raw_payload = self._zinput_count_detachment_fixture()
        symbol_id = "recoil:function:0x42a9d0"
        symbol_before = deepcopy(data["symbols"][symbol_id])

        details = _replace_owner_batch(
            data,
            _parse_owner_replace_batch_payload(json.dumps(raw_payload)),
        )

        owner = data["owners"][
            "recoil:owner:engine.zinput.bindgroup_info_records"
        ]
        self.assertEqual([symbol_id], details["detached_primary_function_ids"])
        self.assertTrue(details["primary_detachments_reviewed"])
        self.assertFalse(details["primary_membership_preserved"])
        self.assertEqual(symbol_before, data["symbols"][symbol_id])
        self.assertEqual([], owner["relationships"])
        self.assertEqual({}, owner["reimplementation"]["entries"])
        self.assertEqual({}, owner["address_metadata"])

    def test_v2_detachment_preserves_identical_unrelated_real_shape_debt(self) -> None:
        data, raw_payload = self._physical_icf_detachment_fixture()
        unrelated_owner_id = self._add_unrelated_missing_tier_debt(data)

        details = _replace_owner_batch(
            data,
            _parse_owner_replace_batch_payload(json.dumps(raw_payload)),
        )

        self.assertEqual("no-introduced-debt", details["owner_invariant_mode"])
        self.assertEqual(1, details["preserved_unrelated_finding_count"])
        self.assertEqual(0, details["touched_finding_count"])
        self.assertEqual(
            {"entries": {}},
            data["owners"][unrelated_owner_id]["reimplementation"],
        )

    def test_v2_detachment_rejects_canonical_debt_in_touched_scope(self) -> None:
        data, raw_payload = self._physical_icf_detachment_fixture()
        owner_id = "recoil:owner:fixture.download"
        data["owners"][owner_id]["reimplementation"] = {"entries": {}}
        raw_payload["current_owners"][0]["record"]["reimplementation"] = {
            "entries": {}
        }

        with self.assertRaisesRegex(
            ProgressError,
            "canonical findings intersect touched owner/symbol/address scope",
        ):
            _replace_owner_batch(
                data,
                _parse_owner_replace_batch_payload(json.dumps(raw_payload)),
            )

    def test_v2_detachment_rejects_introduced_removed_or_changed_unrelated_debt(
        self,
    ) -> None:
        def finding(text: str) -> dict[str, object]:
            return {
                "text": text,
                "owner_id": "recoil:owner:unrelated",
                "addresses": frozenset({"0x4cc838"}),
                "global": False,
            }

        first = finding(
            "owners[recoil:owner:unrelated]: missing primary entry tier records: 0x4cc838"
        )
        second = finding(
            "owners[recoil:owner:unrelated]: missing primary entry tier records: 0x4cc83c"
        )
        changed = finding(
            "owners[recoil:owner:unrelated]: entry 0x4cc838 tier must be one of X, C, B, A, S"
        )
        cases = {
            "introduced": ([first], [first, second]),
            "removed": ([first, second], [first]),
            "removed-and-replaced": ([first], [changed]),
        }
        for label, (before, after) in cases.items():
            data, raw_payload = self._physical_icf_detachment_fixture()
            with (
                self.subTest(label=label),
                patch.object(
                    progress_cli,
                    "_normalized_canonical_owner_findings",
                    side_effect=[before, after],
                ),
                self.assertRaisesRegex(
                    ProgressError,
                    "changed unrelated pre-existing canonical findings",
                ),
            ):
                _replace_owner_batch(
                    data,
                    _parse_owner_replace_batch_payload(json.dumps(raw_payload)),
                )

    def test_v2_physical_icf_detachment_rejects_stale_or_partial_loss(self) -> None:
        cases = []
        data, payload = self._physical_icf_detachment_fixture()
        payload["primary_function_detachments"][0]["current_owner_id"] = (
            "recoil:owner:fixture.api"
        )
        cases.append((data, payload, "current owner is stale"))
        data, payload = self._physical_icf_detachment_fixture()
        data["symbols"]["recoil:function:0x401000"]["pipeline_class"] = "authored"
        cases.append((data, payload, "pipeline_class is stale"))
        data, payload = self._physical_icf_detachment_fixture()
        payload["replacement_owners"][0]["record"]["relationships"] = deepcopy(
            payload["current_owners"][0]["record"]["relationships"]
        )
        cases.append((data, payload, "partial primary-function membership"))
        data, payload = self._physical_icf_detachment_fixture()
        payload["replacement_owners"][0]["record"]["reimplementation"] = deepcopy(
            payload["current_owners"][0]["record"]["reimplementation"]
        )
        cases.append((data, payload, "remove exactly the stale tiers"))

        for data, raw_payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(
                ProgressError, message
            ):
                _replace_owner_batch(
                    data,
                    _parse_owner_replace_batch_payload(json.dumps(raw_payload)),
                )

    def test_v2_non_authored_detachment_rejects_class_role_and_provider_drift(
        self,
    ) -> None:
        cases = []
        data, payload = self._zinput_count_detachment_fixture()
        data["symbols"]["recoil:function:0x42a9d0"]["pipeline_class"] = "authored"
        cases.append((data, payload, "pipeline_class is stale"))

        data, payload = self._zinput_count_detachment_fixture()
        data["symbols"]["recoil:function:0x42a9d0"]["authored_order_role"] = (
            "compiler-generated-icf-representative"
        )
        cases.append((data, payload, "authored_order_role is stale"))

        data, payload = self._zinput_count_detachment_fixture()
        payload["replacement_owners"][0]["record"]["kind"] = "provider-boundary"
        cases.append((data, payload, "unrelated field 'kind'"))

        data, payload = self._zinput_count_detachment_fixture()
        payload["primary_function_detachments"] = []
        cases.append((data, payload, "partial primary-function membership"))

        for data, raw_payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(
                ProgressError, message
            ):
                _replace_owner_batch(
                    data,
                    _parse_owner_replace_batch_payload(json.dumps(raw_payload)),
                )

    def test_retained_owner_bootstraps_exact_unowned_authored_data_tier_x(self) -> None:
        data, raw_payload = retained_owner_unowned_authored_data_tier_x_fixture()
        payload = _parse_owner_replace_batch_payload(json.dumps(raw_payload))

        details = _replace_owner_batch(data, payload)

        symbol_id = "recoil:data:0x4ce7d8"
        owner = data["owners"][
            "recoil:owner:hud_ui.hud_ui_timer_panel_float_class"
        ]
        self.assertEqual(
            [symbol_id],
            details["bootstrapped_unowned_primary_data_tier_x_ids"],
        )
        self.assertEqual(
            {
                "kind": "data",
                "tier": "X",
                "evidence_ids": [],
            },
            owner["reimplementation"]["entries"][symbol_id],
        )
        self.assertIn(
            {
                "address": "0x4ce7d8",
                "kind": "primary-data",
                "name": "g_HudUiTimerPanelFloat_FTable",
                "symbol_id": symbol_id,
            },
            owner["relationships"],
        )
        self.assertEqual("primary-owned", data["symbols"][symbol_id]["ownership_state"])
        self.assertEqual([], details["storage_owner_reassignments"])
        self.assertEqual({}, data["storage_contributions"])

    def test_unowned_authored_data_tier_x_bootstrap_rejects_other_shapes(self) -> None:
        cases: list[tuple[str, dict[str, object], dict[str, object], str]] = []
        owner_id = "recoil:owner:hud_ui.hud_ui_timer_panel_float_class"
        symbol_id = "recoil:data:0x4ce7d8"
        function_id = "recoil:function:0x40ef60"

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        del payload["replacement_owners"][0]["record"]["reimplementation"][
            "entries"
        ][symbol_id]
        cases.append(("omitted entry", data, payload, "remove exactly the stale tiers"))

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        payload["replacement_owners"][0]["record"]["reimplementation"]["entries"][
            symbol_id
        ]["tier"] = "C"
        cases.append(("positive tier", data, payload, "remove exactly the stale tiers"))

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        payload["replacement_owners"][0]["record"]["reimplementation"]["entries"][
            symbol_id
        ]["evidence_ids"] = ["recoil:evidence:invented"]
        cases.append(("nonempty evidence", data, payload, "remove exactly the stale tiers"))

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        payload["replacement_owners"][0]["record"]["reimplementation"]["entries"][
            symbol_id
        ]["kind"] = "function"
        cases.append(("wrong kind", data, payload, "remove exactly the stale tiers"))

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        payload["replacement_owners"][0]["record"]["reimplementation"]["entries"][
            symbol_id
        ]["unexpected"] = True
        cases.append(("wrong shape", data, payload, "remove exactly the stale tiers"))

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        payload["replacement_owners"][0]["record"]["reimplementation"]["entries"][
            function_id
        ]["tier"] = "A"
        cases.append(
            ("unrelated function tier", data, payload, "remove exactly the stale tiers")
        )

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        data["owners"][owner_id]["kind"] = "provider-boundary"
        payload["current_owners"][0]["record"]["kind"] = "provider-boundary"
        payload["replacement_owners"][0]["record"]["kind"] = "provider-boundary"
        cases.append(("provider target", data, payload, "rejects a provider destination"))

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        data["symbols"][symbol_id]["ownership_state"] = "unresolved"
        cases.append(("stale ownership state", data, payload, "ownership_state is stale"))

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        data["symbols"][symbol_id]["ownership_state"] = "primary-owned"
        payload["primary_data_reassignments"][0][
            "current_ownership_state"
        ] = "primary-owned"
        cases.append(
            (
                "non-unowned ownership state",
                data,
                payload,
                "not exact currently-unowned state",
            )
        )

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        payload["primary_data_reassignments"] = []
        cases.append(("missing reviewed row", data, payload, "exact reviewed rows"))

        data, payload = retained_owner_unowned_authored_data_tier_x_fixture()
        current_relation = deepcopy(
            payload["replacement_owners"][0]["record"]["relationships"][-1]
        )
        data["owners"][owner_id]["relationships"].append(
            deepcopy(current_relation)
        )
        payload["current_owners"][0]["record"]["relationships"].append(
            current_relation
        )
        cases.append(("stale primary membership", data, payload, "exact reviewed rows"))

        for label, data, raw_payload, message in cases:
            with (
                self.subTest(label=label),
                self.assertRaisesRegex(ProgressError, message),
            ):
                parsed = _parse_owner_replace_batch_payload(
                    json.dumps(raw_payload)
                )
                _replace_owner_batch(data, parsed)

    def test_retained_owner_move_preserves_exact_moved_in_tier(self) -> None:
        data, raw_payload = retained_owner_data_tier_move_fixture()
        payload = _parse_owner_replace_batch_payload(json.dumps(raw_payload))

        details = _replace_owner_batch(data, payload)

        symbol_id = "recoil:data:0x4ed4e0"
        source = data["owners"][
            "recoil:owner:network_online.gamenet_refresh_player_list_menu"
        ]
        destination = data["owners"]["recoil:owner:hud_ui.hud_ui_mgr_data"]
        self.assertEqual([symbol_id], details["reassigned_primary_data_ids"])
        self.assertNotIn(symbol_id, source["reimplementation"]["entries"])
        self.assertEqual(
            {
                "kind": "data",
                "tier": "X",
                "evidence_ids": ["recoil:evidence:retained-data-tier-x"],
            },
            destination["reimplementation"]["entries"][symbol_id],
        )
        self.assertEqual(
            ["recoil:owner:hud_ui.hud_ui_mgr_data"],
            data["storage_contributions"][
                "recoil:storage:va:0x4ed4e0"
            ]["owner_ids"],
        )

    def test_retained_owner_move_rejects_changed_or_unrelated_tiers(self) -> None:
        cases: list[tuple[str, dict[str, object], dict[str, object], str]] = []
        destination_id = "recoil:owner:hud_ui.hud_ui_mgr_data"
        source_id = (
            "recoil:owner:network_online.gamenet_refresh_player_list_menu"
        )
        moved_symbol_id = "recoil:data:0x4ed4e0"

        data, payload = retained_owner_data_tier_move_fixture()
        moved_entry = payload["replacement_owners"][1]["record"][
            "reimplementation"
        ]["entries"][moved_symbol_id]
        moved_entry["tier"] = "C"
        cases.append(("changed tier", data, payload, "remove exactly the stale tiers"))

        data, payload = retained_owner_data_tier_move_fixture()
        moved_entry = payload["replacement_owners"][1]["record"][
            "reimplementation"
        ]["entries"][moved_symbol_id]
        moved_entry["evidence_ids"] = ["recoil:evidence:changed"]
        cases.append(
            ("changed evidence", data, payload, "remove exactly the stale tiers")
        )

        data, payload = retained_owner_data_tier_move_fixture()
        moved_entry = payload["replacement_owners"][1]["record"][
            "reimplementation"
        ]["entries"][moved_symbol_id]
        moved_entry["kind"] = "function"
        cases.append(("changed kind", data, payload, "remove exactly the stale tiers"))

        data, payload = retained_owner_data_tier_move_fixture()
        moved_entry = payload["replacement_owners"][1]["record"][
            "reimplementation"
        ]["entries"][moved_symbol_id]
        moved_entry["unexpected"] = True
        cases.append(
            ("changed record shape", data, payload, "remove exactly the stale tiers")
        )

        data, payload = retained_owner_data_tier_move_fixture()
        payload["replacement_owners"][1]["record"]["reimplementation"][
            "entries"
        ]["recoil:data:0x4ed4e4"] = {
            "kind": "data",
            "tier": "X",
            "evidence_ids": [],
        }
        cases.append(
            ("invented moved-in row", data, payload, "remove exactly the stale tiers")
        )

        data, payload = retained_owner_data_tier_move_fixture()
        del data["owners"][source_id]["reimplementation"]["entries"][
            moved_symbol_id
        ]
        del payload["current_owners"][0]["record"]["reimplementation"][
            "entries"
        ][moved_symbol_id]
        cases.append(
            (
                "missing source entry",
                data,
                payload,
                "has no exact reimplementation record",
            )
        )

        data, payload = retained_owner_data_tier_move_fixture()
        payload["replacement_owners"][1]["record"]["reimplementation"][
            "entries"
        ]["recoil:function:0x401000"]["tier"] = "A"
        cases.append(
            (
                "unrelated tier change",
                data,
                payload,
                "remove exactly the stale tiers",
            )
        )

        for label, data, raw_payload, message in cases:
            with (
                self.subTest(label=label),
                self.assertRaisesRegex(ProgressError, message),
            ):
                parsed = _parse_owner_replace_batch_payload(
                    json.dumps(raw_payload)
                )
                _replace_owner_batch(data, parsed)

    def test_zbd_and_five_list_members_migrate_atomically(self) -> None:
        data, raw_payload = zbd_owner_replace_fixture()
        payload = _parse_owner_replace_batch_payload(json.dumps(raw_payload))
        details = _replace_owner_batch(data, payload)

        self.assertEqual(
            ["recoil:owner:provider.msvc5_std_list_zzbd_section_handler_instantiation"],
            details["created_owner_ids"],
        )
        self.assertEqual(
            ["recoil:owner:core_util_archive.zzbd_section_handler_list_class"],
            details["retired_owner_ids"],
        )
        self.assertEqual(6, len(details["reassigned_primary_function_ids"]))
        manager = data["owners"]["recoil:owner:core_util_archive.zzbd_manager_core_class"]
        self.assertEqual({}, manager["reimplementation"]["entries"])
        self.assertEqual("accepted", manager["gates"]["source"])
        provider = data["owners"][
            "recoil:owner:provider.msvc5_std_list_zzbd_section_handler_instantiation"
        ]
        self.assertNotIn("reimplementation", provider)

    def test_ui_vector_members_migrate_with_exact_incoming_dependency_retarget(self) -> None:
        data, raw_payload = ui_vector_owner_replace_fixture()
        payload = _parse_owner_replace_batch_payload(json.dumps(raw_payload))
        details = _replace_owner_batch(data, payload)

        self.assertEqual(2, len(details["reassigned_primary_function_ids"]))
        self.assertEqual(
            ["recoil:owner:legacy.hud_ui.struct_huduicompositepanelvector"],
            details["retired_owner_ids"],
        )
        self.assertEqual(
            [
                {
                    "owner_id": "recoil:owner:legacy.hud_ui.class_huduicompositepanel",
                    "old_target_owner_id": "recoil:owner:legacy.hud_ui.struct_huduicompositepanelvector",
                    "new_target_owner_id": "recoil:owner:provider.compiler.vc5_huduicompositepanelentry_vector_comdats",
                    "old_reason": "manual",
                    "new_reason": "provider-boundary",
                }
            ],
            details["dependency_retargets"],
        )
        panel = data["owners"]["recoil:owner:legacy.hud_ui.class_huduicompositepanel"]
        self.assertEqual(
            "recoil:owner:provider.compiler.vc5_huduicompositepanelentry_vector_comdats",
            panel["relationships"][0]["target_owner_id"],
        )
        self.assertEqual("provider-boundary", panel["relationships"][0]["reason"])

    def test_appmodul_v2_bootstraps_unowned_functions_and_rehomes_data(self) -> None:
        data, raw_payload = appmodul_owner_replace_fixture()
        payload = _parse_owner_replace_batch_payload(json.dumps(raw_payload))
        details = _replace_owner_batch(data, payload)

        self.assertEqual(
            [
                "recoil:function:0x4c81d8",
                "recoil:function:0x4c8201",
                "recoil:function:0x4c8224",
            ],
            details["bootstrapped_primary_function_ids"],
        )
        self.assertEqual(
            ["recoil:data:0x4da004", "recoil:data:0x56cc28"],
            details["reassigned_primary_data_ids"],
        )
        for symbol_id in details["bootstrapped_primary_function_ids"]:
            self.assertEqual("primary-owned", data["symbols"][symbol_id]["ownership_state"])
        unknown = data["symbols"]["recoil:data:0x56cc28"]
        self.assertEqual("unknown", unknown["extent_state"])
        self.assertNotIn("end_exclusive", unknown)
        self.assertNotIn("size", unknown)
        self.assertEqual([], unknown["storage_contribution_ids"])
        self.assertEqual(
            ["recoil:owner:provider.mfc.appmodul_cpp_application_startup"],
            data["storage_contributions"]["recoil:storage:va:0x4da004"]["owner_ids"],
        )

    def test_v2_data_only_batch_rehomes_exact_data_without_function_changes(self) -> None:
        data, raw_payload = data_only_owner_replace_fixture()
        payload = _parse_owner_replace_batch_payload(json.dumps(raw_payload))

        details = _replace_owner_batch(data, payload)

        self.assertEqual([], details["reassigned_primary_function_ids"])
        self.assertEqual([], details["bootstrapped_primary_function_ids"])
        self.assertEqual(
            ["recoil:data:0x510000", "recoil:data:0x510004"],
            details["reassigned_primary_data_ids"],
        )
        self.assertEqual(
            ["recoil:data:0x510004"],
            details["bootstrapped_unknown_data_symbol_ids"],
        )
        self.assertEqual(
            ["recoil:owner:provider.runtime.reviewed_data_owner"],
            data["storage_contributions"]["recoil:storage:va:0x510000"]["owner_ids"],
        )
        self.assertEqual(
            "primary-owned",
            data["symbols"]["recoil:data:0x510004"]["ownership_state"],
        )

    def test_v1_and_empty_or_noop_v2_batches_remain_fail_closed(self) -> None:
        data, raw_payload = data_only_owner_replace_fixture()
        v1_payload = deepcopy(raw_payload)
        v1_payload["schema"] = "recoil-owner-replace-batch-v1"
        v1_payload["replacement_owners"][0]["record"]["relationships"] = [
            row
            for row in v1_payload["replacement_owners"][0]["record"]["relationships"]
            if row.get("symbol_id") != "recoil:data:0x510004"
        ]
        v1_payload.pop("primary_function_bootstraps")
        v1_payload.pop("primary_data_reassignments")
        v1_payload.pop("unknown_data_symbol_bootstraps")
        with self.assertRaisesRegex(
            ProgressError,
            "does not reassign any primary functions",
        ):
            _replace_owner_batch(
                data,
                _parse_owner_replace_batch_payload(json.dumps(v1_payload)),
            )

        data, graph_only_payload = data_only_owner_replace_fixture()
        for owner in data["owners"].values():
            owner["relationships"] = [
                row
                for row in owner["relationships"]
                if row.get("kind") != "primary-data"
            ]
        graph_only_payload["current_owners"][0]["record"]["relationships"] = []
        graph_only_payload["replacement_owners"][0]["record"]["relationships"] = []
        graph_only_payload["primary_data_reassignments"] = []
        graph_only_payload["unknown_data_symbol_bootstraps"] = []
        with self.assertRaisesRegex(
            ProgressError,
            "does not reassign any primary functions or primary data",
        ):
            _replace_owner_batch(
                data,
                _parse_owner_replace_batch_payload(json.dumps(graph_only_payload)),
            )

        data, noop_payload = data_only_owner_replace_fixture()
        noop_payload["replacement_owners"] = deepcopy(noop_payload["current_owners"])
        noop_payload["primary_data_reassignments"] = []
        noop_payload["unknown_data_symbol_bootstraps"] = []
        with self.assertRaisesRegex(ProgressError, "batch is a no-op"):
            _replace_owner_batch(
                data,
                _parse_owner_replace_batch_payload(json.dumps(noop_payload)),
            )

        _data, empty_payload = data_only_owner_replace_fixture()
        empty_payload["current_owners"] = []
        with self.assertRaisesRegex(ProgressError, "current_owners must be a non-empty array"):
            _parse_owner_replace_batch_payload(json.dumps(empty_payload))

    def test_appmodul_v2_rejects_stale_duplicate_unknown_data_loss_and_arbitrary_extra(self) -> None:
        cases: list[tuple[dict[str, object], dict[str, object], str]] = []

        data, payload = appmodul_owner_replace_fixture()
        payload["primary_function_bootstraps"][0]["current_ownership_state"] = "primary-owned"
        cases.append((data, payload, "ownership_state is stale"))

        data, payload = appmodul_owner_replace_fixture()
        payload["primary_function_bootstraps"].append(
            deepcopy(payload["primary_function_bootstraps"][0])
        )
        cases.append((data, payload, "duplicate symbol or address"))

        data, payload = appmodul_owner_replace_fixture()
        payload["primary_function_bootstraps"][0]["symbol_id"] = "recoil:function:0xdead00"
        payload["primary_function_bootstraps"][0]["address"] = "0xdead00"
        cases.append((data, payload, "unknown symbol"))

        data, payload = appmodul_owner_replace_fixture()
        payload["primary_data_reassignments"].pop(0)
        cases.append((data, payload, "exact reviewed rows"))

        data, payload = appmodul_owner_replace_fixture()
        extra_id = "recoil:function:0x4c8230"
        data["symbols"][extra_id] = {
            "address": "0x4c8230", "binary": "recoil", "kind": "function",
            "ownership_state": "unresolved",
        }
        payload["replacement_owners"][0]["record"]["relationships"].append(
            {
                "kind": "primary-function", "address": "0x4c8230", "symbol_id": extra_id,
            }
        )
        cases.append((data, payload, "partial primary-function membership"))

        data, payload = appmodul_owner_replace_fixture()
        data["storage_contributions"]["recoil:storage:va:0x4da004"]["owner_ids"] = []
        cases.append((data, payload, "does not exactly reference current owner"))

        for data, raw_payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(ProgressError, message):
                parsed = _parse_owner_replace_batch_payload(json.dumps(raw_payload))
                _replace_owner_batch(data, parsed)

    def test_dependency_retarget_rejects_add_remove_unretired_and_undeclared_changes(self) -> None:
        cases: list[tuple[dict[str, object], dict[str, object], str]] = []

        data, payload = ui_vector_owner_replace_fixture()
        payload["replacement_owners"][0]["record"]["relationships"].append(
            {
                "kind": "depends-on-owner",
                "target_owner_id": "recoil:owner:legacy.hud_ui.struct_huduicompositepanelentry",
                "reason": "manual",
            }
        )
        cases.append((data, payload, "may not add or remove non-primary relationships"))

        data, payload = ui_vector_owner_replace_fixture()
        payload["replacement_owners"][0]["record"]["relationships"][0]["extra"] = "changed"
        cases.append((data, payload, "fields other than target_owner_id and reason"))

        data, payload = ui_vector_owner_replace_fixture()
        payload["replacement_owners"][0]["record"]["relationships"][0][
            "target_owner_id"
        ] = "recoil:owner:legacy.hud_ui.struct_huduicompositepanelentry"
        cases.append((data, payload, "not an explicit replacement owner"))

        data, payload = ui_vector_owner_replace_fixture()
        payload["current_owners"][1]["record"]["relationships"][0][
            "target_owner_id"
        ] = "recoil:owner:legacy.hud_ui.struct_huduicompositepanelentry"
        data["owners"]["recoil:owner:legacy.hud_ui.class_huduicompositepanel"][
            "relationships"
        ][0]["target_owner_id"] = "recoil:owner:legacy.hud_ui.struct_huduicompositepanelentry"
        cases.append((data, payload, "old target .* is not retired by this batch"))

        for data, raw_payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(ProgressError, message):
                parsed = _parse_owner_replace_batch_payload(json.dumps(raw_payload))
                _replace_owner_batch(data, parsed)

    def test_rejects_stale_partial_duplicate_dangling_and_unrelated_changes(self) -> None:
        cases: list[tuple[dict[str, object], dict[str, object], str]] = []
        data, payload = zbd_owner_replace_fixture()
        payload["current_owners"][0]["record"]["name"] = "stale"
        cases.append((data, payload, "snapshot is stale"))

        data, payload = zbd_owner_replace_fixture()
        payload["replacement_owners"][1]["record"]["relationships"].pop()
        cases.append((data, payload, "partial primary-function membership"))

        data, payload = zbd_owner_replace_fixture()
        duplicate = deepcopy(payload["replacement_owners"][1]["record"]["relationships"][1])
        payload["replacement_owners"][1]["record"]["relationships"].append(duplicate)
        cases.append((data, payload, "duplicate primary-function ownership"))

        data, payload = zbd_owner_replace_fixture()
        data["symbols"]["recoil:function:provider-alias"] = {
            "address": "0x4c07d0",
            "binary": "recoil",
            "kind": "provider-function",
        }
        payload["replacement_owners"][1]["record"]["relationships"].append(
            {
                "kind": "primary-function",
                "address": "0x4c07d0",
                "symbol_id": "recoil:function:provider-alias",
            }
        )
        cases.append((data, payload, "duplicate primary-function address ownership"))

        data, payload = zbd_owner_replace_fixture()
        payload["replacement_owners"][0]["record"]["gates"]["source"] = "pending"
        cases.append((data, payload, "unrelated field 'gates'"))

        data, payload = zbd_owner_replace_fixture()
        payload["replacement_owners"][0]["record"]["reimplementation"]["tier"] = "S"
        cases.append((data, payload, "remove exactly the stale tiers"))

        data, payload = zbd_owner_replace_fixture()
        data["owners"]["recoil:owner:dependent"] = _owner_fixture_row(
            owner_id="recoil:owner:dependent", kind="class", addresses=[],
        )
        data["owners"]["recoil:owner:dependent"]["relationships"] = [
            {
                "kind": "depends-on-owner",
                "target_owner_id": "recoil:owner:core_util_archive.zzbd_section_handler_list_class",
                "reason": "manual",
            }
        ]
        data["owners"]["recoil:owner:dependent"]["address_metadata"] = {}
        data["owners"]["recoil:owner:dependent"]["reimplementation"] = {"entries": {}}
        cases.append((data, payload, "dependency owner not found"))

        for data, raw_payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(ProgressError, message):
                parsed = _parse_owner_replace_batch_payload(json.dumps(raw_payload))
                _replace_owner_batch(data, parsed)

    def test_owner_replace_batch_dry_run_apply_and_revision_cas(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            data, payload = zbd_owner_replace_fixture()
            path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            arguments = [
                "owner", "replace-batch", "--progress", str(path),
                "--payload-json", json.dumps(payload), "--expected-revision", "1307",
                "--dry-run", "--json",
            ]
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                dry_rc = progress_cli.main(arguments)
            self.assertEqual(0, dry_rc, stderr.getvalue())
            self.assertFalse(json.loads(stdout.getvalue())["commit"]["applied"])
            self.assertEqual(1307, json.loads(path.read_text(encoding="utf-8"))["revision"])

            arguments[arguments.index("--dry-run")] = "--apply"
            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                apply_rc = progress_cli.main(arguments)
            self.assertEqual(0, apply_rc)
            self.assertEqual(1308, json.loads(path.read_text(encoding="utf-8"))["revision"])

            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                stale_rc = progress_cli.main(arguments)
            self.assertEqual(2, stale_rc)

    def test_owner_replace_payload_file_preserves_dry_run_apply_and_revision_cas(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture_root = Path(temporary)
            root = fixture_root / "build"
            root.mkdir()
            progress_path = root / "progress.json"
            payload_path = root / "owner-replace.json"
            data, payload = data_only_owner_replace_fixture()
            progress_path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            payload_path.write_text(
                json.dumps(payload) + (" " * 100_000) + "\n",
                encoding="utf-8",
            )
            self.assertGreater(payload_path.stat().st_size, 32 * 1024)
            arguments = [
                "owner",
                "replace-batch",
                "--progress",
                str(progress_path),
                "--payload-file",
                str(payload_path),
                "--expected-revision",
                "1340",
                "--dry-run",
                "--json",
            ]

            stdout, stderr = io.StringIO(), io.StringIO()
            with (
                patch.object(progress_cli, "REPO_ROOT", fixture_root),
                redirect_stdout(stdout),
                redirect_stderr(stderr),
            ):
                dry_rc = progress_cli.main(arguments)
            self.assertEqual(0, dry_rc, stderr.getvalue())
            dry_result = json.loads(stdout.getvalue())
            self.assertFalse(dry_result["commit"]["applied"])
            self.assertEqual([], dry_result["reassigned_primary_function_ids"])
            self.assertEqual(
                ["recoil:data:0x510000", "recoil:data:0x510004"],
                dry_result["reassigned_primary_data_ids"],
            )
            self.assertEqual(
                1340,
                json.loads(progress_path.read_text(encoding="utf-8"))["revision"],
            )

            arguments[arguments.index("--dry-run")] = "--apply"
            with (
                patch.object(progress_cli, "REPO_ROOT", fixture_root),
                redirect_stdout(io.StringIO()),
                redirect_stderr(io.StringIO()),
            ):
                apply_rc = progress_cli.main(arguments)
            self.assertEqual(0, apply_rc)
            self.assertEqual(
                1341,
                json.loads(progress_path.read_text(encoding="utf-8"))["revision"],
            )

            with (
                patch.object(progress_cli, "REPO_ROOT", fixture_root),
                redirect_stdout(io.StringIO()),
                redirect_stderr(io.StringIO()),
            ):
                stale_rc = progress_cli.main(arguments)
            self.assertEqual(2, stale_rc)

    def test_owner_replace_payload_sources_are_mutually_exclusive(self) -> None:
        stderr = io.StringIO()
        with redirect_stderr(stderr), self.assertRaises(SystemExit) as raised:
            progress_cli._parser().parse_args(
                [
                    "owner",
                    "replace-batch",
                    "--payload-json",
                    "{}",
                    "--payload-file",
                    "build/owner-replace.json",
                    "--expected-revision",
                    "1340",
                    "--dry-run",
                    "--json",
                ]
            )
        self.assertEqual(2, raised.exception.code)
        self.assertIn("not allowed with argument --payload-json", stderr.getvalue())

    def test_owner_replace_payload_file_reports_input_failures(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture_root = Path(temporary)
            root = fixture_root / "build"
            root.mkdir()
            progress_path = root / "progress.json"
            data, _payload = data_only_owner_replace_fixture()
            progress_path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            cases = (
                (root / "missing.json", None, "missing or unreadable"),
                (root / "invalid-utf8.json", b"\xff\xfe\xfa", "not valid UTF-8"),
                (root / "invalid-json.json", b"{", "--payload-file is not valid JSON"),
            )
            for payload_path, content, message in cases:
                if content is not None:
                    payload_path.write_bytes(content)
                stdout, stderr = io.StringIO(), io.StringIO()
                with (
                    self.subTest(message=message),
                    patch.object(progress_cli, "REPO_ROOT", fixture_root),
                    redirect_stdout(stdout),
                    redirect_stderr(stderr),
                ):
                    rc = progress_cli.main(
                        [
                            "owner",
                            "replace-batch",
                            "--progress",
                            str(progress_path),
                            "--payload-file",
                            str(payload_path),
                            "--expected-revision",
                            "1340",
                            "--dry-run",
                            "--json",
                        ]
                    )
                self.assertEqual(2, rc)
                self.assertEqual("", stdout.getvalue())
                self.assertIn(message, stderr.getvalue())

    def test_owner_audit_runs_canonical_owner_invariants(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            data, _payload = zbd_owner_replace_fixture()
            path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                passed_rc = progress_cli.main(
                    ["owner", "audit", "--progress", str(path), "--strict", "--json"]
                )
            self.assertEqual(0, passed_rc, stderr.getvalue())
            self.assertTrue(json.loads(stdout.getvalue())["passed"])

            duplicate = deepcopy(
                data["owners"]["recoil:owner:core_util_archive.zzbd_manager_core_class"]
                ["relationships"][1]
            )
            data["owners"]["recoil:owner:core_util_archive.zzbd_manager_core_class"][
                "relationships"
            ].append(duplicate)
            path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                failed_rc = progress_cli.main(
                    ["owner", "audit", "--progress", str(path), "--strict", "--json"]
                )
            self.assertEqual(1, failed_rc, stderr.getvalue())
            failed = json.loads(stdout.getvalue())
            self.assertFalse(failed["passed"])
            self.assertIn("duplicate relationship", failed["findings"][0]["message"])


class OwnerPrimaryDataTierXRepairTests(unittest.TestCase):
    @staticmethod
    def _fixture() -> tuple[dict[str, object], dict[str, object]]:
        data = empty_progress_document()
        data["revision"] = 1400
        owner_id = (
            "recoil:owner:battlesport_gameplay."
            "player_ai_mode2_top_level_steering"
        )
        symbol_rows = (
            (
                "recoil:data:0x4cc838",
                "0x4cc838",
                "g_AINetSolveAltGunLeadTargetPoint_FloatLiteral_4CC838",
            ),
            (
                "recoil:data:0x4cc83c",
                "0x4cc83c",
                "g_AINetSolveAltGunLeadTargetPoint_FloatLiteral_4CC83C",
            ),
        )
        owner = _owner_fixture_row(
            owner_id=owner_id,
            kind="subsystem",
            addresses=[],
        )
        relationships: list[dict[str, str]] = []
        payload_rows: list[dict[str, object]] = []
        for symbol_id, address, name in symbol_rows:
            relationship = {
                "address": address,
                "kind": "primary-data",
                "name": name,
                "symbol_id": symbol_id,
            }
            relationships.append(relationship)
            payload_rows.append(
                {
                    "symbol_id": symbol_id,
                    "address": address,
                    "current_ownership_state": "primary-owned",
                    "current_relationship": deepcopy(relationship),
                }
            )
            data["symbols"][symbol_id] = {
                "address": address,
                "binary": "recoil",
                "disposition": "authored",
                "extent_state": "known",
                "kind": "data",
                "navigation_name": name,
                "output_section_id": "recoil:section:.rdata",
                "ownership_state": "primary-owned",
                "size": 4,
                "storage_contribution_ids": [],
            }
        owner["relationships"] = relationships
        owner["reimplementation"] = {"entries": {}}
        data["owners"][owner_id] = owner
        payload: dict[str, object] = {
            "schema": "recoil-owner-primary-data-tier-x-repair-v1",
            "reviewed": True,
            "parent_reviewed": True,
            "reason": (
                "Initialize absent bookkeeping tiers for exact existing authored "
                "primary-data relationships."
            ),
            "binary": "recoil",
            "owner_id": owner_id,
            "primary_data": payload_rows,
        }
        return data, payload

    def test_repairs_only_absent_exact_same_owner_authored_data_entries(self) -> None:
        data, raw_payload = self._fixture()
        payload = _parse_owner_primary_data_tier_x_repair_payload(
            json.dumps(raw_payload)
        )

        details = _repair_owner_primary_data_tier_x(data, payload)

        expected_ids = [row["symbol_id"] for row in raw_payload["primary_data"]]
        self.assertEqual(expected_ids, details["initialized_primary_data_tier_x_ids"])
        owner = data["owners"][raw_payload["owner_id"]]
        for symbol_id in expected_ids:
            self.assertEqual(
                {"kind": "data", "tier": "X", "evidence_ids": []},
                owner["reimplementation"]["entries"][symbol_id],
            )
        self.assertEqual(
            raw_payload["primary_data"][0]["current_relationship"],
            owner["relationships"][0],
        )
        self.assertTrue(details["membership_unchanged"])
        self.assertTrue(details["all_other_state_unchanged"])

    def test_rejects_existing_entry_function_provider_ambiguity_and_stale_state(
        self,
    ) -> None:
        cases: list[tuple[str, dict[str, object], dict[str, object], str]] = []
        data, payload = self._fixture()
        symbol_id = payload["primary_data"][0]["symbol_id"]
        data["owners"][payload["owner_id"]]["reimplementation"]["entries"][
            symbol_id
        ] = {"kind": "data", "tier": "X", "evidence_ids": []}
        cases.append(("existing entry", data, payload, "already has"))

        data, payload = self._fixture()
        symbol_id = payload["primary_data"][0]["symbol_id"]
        data["symbols"][symbol_id]["kind"] = "function"
        cases.append(("function", data, payload, "not authored data"))

        data, payload = self._fixture()
        data["owners"][payload["owner_id"]]["kind"] = "provider-boundary"
        cases.append(("provider", data, payload, "provider-boundary"))

        data, payload = self._fixture()
        symbol_id = payload["primary_data"][0]["symbol_id"]
        other = _owner_fixture_row(
            owner_id="recoil:owner:duplicate",
            kind="subsystem",
            addresses=[],
        )
        other["relationships"] = [
            deepcopy(payload["primary_data"][0]["current_relationship"])
        ]
        other["reimplementation"] = {"entries": {}}
        data["owners"]["recoil:owner:duplicate"] = other
        cases.append(("ambiguity", data, payload, "ambiguous or changed"))

        data, payload = self._fixture()
        payload["primary_data"][0]["current_relationship"]["name"] = "stale"
        cases.append(("stale relationship", data, payload, "snapshot is stale"))

        data, payload = self._fixture()
        symbol_id = payload["primary_data"][0]["symbol_id"]
        data["symbols"][symbol_id]["ownership_state"] = "unresolved"
        cases.append(("stale ownership", data, payload, "ownership_state is stale"))

        for label, data, raw_payload, message in cases:
            with self.subTest(label=label), self.assertRaisesRegex(
                ProgressError, message
            ):
                _repair_owner_primary_data_tier_x(
                    data,
                    _parse_owner_primary_data_tier_x_repair_payload(
                        json.dumps(raw_payload)
                    ),
                )

    def test_cli_audit_dry_run_apply_and_revision_cas(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            data, payload = self._fixture()
            path.write_text(json.dumps(data) + "\n", encoding="utf-8")

            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                audit_rc = progress_cli.main(
                    ["owner", "audit", "--progress", str(path), "--strict", "--json"]
                )
            self.assertEqual(1, audit_rc, stderr.getvalue())
            self.assertIn(
                "missing primary entry tier records: 0x4cc838, 0x4cc83c",
                json.loads(stdout.getvalue())["findings"][0]["message"],
            )

            arguments = [
                "owner",
                "repair-primary-data-tier-x",
                "--progress",
                str(path),
                "--payload-json",
                json.dumps(payload),
                "--expected-revision",
                "1400",
                "--dry-run",
                "--json",
            ]
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                dry_rc = progress_cli.main(arguments)
            self.assertEqual(0, dry_rc, stderr.getvalue())
            dry_result = json.loads(stdout.getvalue())
            self.assertFalse(dry_result["commit"]["applied"])
            self.assertEqual(1400, json.loads(path.read_text(encoding="utf-8"))["revision"])

            arguments[arguments.index("--dry-run")] = "--apply"
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                apply_rc = progress_cli.main(arguments)
            self.assertEqual(0, apply_rc, stderr.getvalue())
            applied = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(1401, applied["revision"])

            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                passed_audit_rc = progress_cli.main(
                    ["owner", "audit", "--progress", str(path), "--strict", "--json"]
                )
            self.assertEqual(0, passed_audit_rc, stderr.getvalue())
            self.assertTrue(json.loads(stdout.getvalue())["passed"])

            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                stale_rc = progress_cli.main(arguments)
            self.assertEqual(2, stale_rc)


class AuthoredStorageNoIntroducedDebtTests(unittest.TestCase):
    @staticmethod
    def _fixture_with_unrelated_debt() -> tuple[dict, dict, str]:
        from tests.tools.recoil_storage_contribution_progress_tests import (
            payload as storage_payload,
            tracker as storage_tracker,
        )

        data = storage_tracker()
        owner_id = "recoil:owner:unrelated.preexisting-tier-debt"
        addresses = ["0x4cc838", "0x4cc83c"]
        owner = _owner_fixture_row(
            owner_id=owner_id,
            kind="subsystem",
            addresses=addresses,
        )
        owner["reimplementation"] = {"entries": {}}
        data["owners"][owner_id] = owner
        for address in addresses:
            data["symbols"][f"recoil:function:{address}"] = {
                "address": address,
                "binary": "recoil",
                "kind": "function",
                "navigation_name": f"Unrelated debt {address}",
                "ownership_state": "primary-owned",
                "pipeline_class": "authored",
                "authored_order_role": "authored-body",
            }
        return data, storage_payload(), owner_id

    @staticmethod
    def _write_tracker(data: dict, temporary: str) -> Path:
        path = Path(temporary) / "progress.json"
        path.write_text(json.dumps(data) + "\n", encoding="utf-8")
        return path

    def test_cli_dry_run_preserves_exact_unrelated_owner_debt(self) -> None:
        data, payload, unrelated_owner_id = self._fixture_with_unrelated_debt()
        with tempfile.TemporaryDirectory() as temporary:
            path = self._write_tracker(data, temporary)
            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                rc = progress_cli.main(
                    [
                        "storage",
                        "register-authored-data",
                        "--progress",
                        str(path),
                        "--payload-json",
                        json.dumps(payload),
                        "--expected-revision",
                        "7",
                        "--dry-run",
                        "--json",
                    ]
                )
            self.assertEqual(0, rc, stderr.getvalue())
            result = json.loads(stdout.getvalue())
            self.assertEqual("no-introduced-debt", result["owner_invariant_mode"])
            self.assertEqual(1, result["preserved_unrelated_finding_count"])
            self.assertEqual(0, result["touched_finding_count"])
            saved = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(7, saved["revision"])
            self.assertEqual(
                {"entries": {}},
                saved["owners"][unrelated_owner_id]["reimplementation"],
            )

            applied = _mutate_authored_storage_no_new_debt(
                path,
                payload,
                expected_revision=7,
                apply=True,
            )
            self.assertTrue(applied["applied"])
            saved = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(8, saved["revision"])
            self.assertIn(
                payload["storage_contribution_id"],
                saved["storage_contributions"],
            )
            self.assertEqual(
                {"entries": {}},
                saved["owners"][unrelated_owner_id]["reimplementation"],
            )
            with self.assertRaises(ConcurrentRevisionUpdate):
                _mutate_authored_storage_no_new_debt(
                    path,
                    payload,
                    expected_revision=7,
                    apply=False,
                )

    def test_storage_rejects_preexisting_or_new_touched_owner_debt(self) -> None:
        from tests.tools.recoil_storage_contribution_progress_tests import OWNER_ID

        data, payload, _unrelated_owner_id = self._fixture_with_unrelated_debt()
        data["owners"][OWNER_ID]["reimplementation"] = {"entries": {}}
        with tempfile.TemporaryDirectory() as temporary:
            path = self._write_tracker(data, temporary)
            with self.assertRaisesRegex(
                ValueError,
                "authored storage registration canonical findings intersect "
                "touched owner/symbol/address scope",
            ):
                _mutate_authored_storage_no_new_debt(
                    path,
                    payload,
                    expected_revision=7,
                    apply=False,
                )

    def test_storage_rejects_changed_unrelated_debt_and_preserves_cas(self) -> None:
        data, payload, _unrelated_owner_id = self._fixture_with_unrelated_debt()

        def finding(text: str) -> dict[str, object]:
            return {
                "text": text,
                "owner_id": "recoil:owner:unrelated.preexisting-tier-debt",
                "addresses": frozenset({"0x4cc838"}),
                "global": False,
            }

        first = finding(
            "owners[recoil:owner:unrelated.preexisting-tier-debt]: "
            "missing primary entry tier records: 0x4cc838"
        )
        second = finding(
            "owners[recoil:owner:unrelated.preexisting-tier-debt]: "
            "missing primary entry tier records: 0x4cc83c"
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = self._write_tracker(data, temporary)
            with (
                patch.object(
                    progress_cli,
                    "_normalized_canonical_owner_findings",
                    side_effect=[[first], [first, second]],
                ),
                self.assertRaisesRegex(
                    ValueError,
                    "authored storage registration changed unrelated "
                    "pre-existing canonical findings",
                ),
            ):
                _mutate_authored_storage_no_new_debt(
                    path,
                    payload,
                    expected_revision=7,
                    apply=False,
                )

            with self.assertRaises(ConcurrentRevisionUpdate):
                _mutate_authored_storage_no_new_debt(
                    path,
                    payload,
                    expected_revision=6,
                    apply=False,
                )


def active_issue_ledger(*, version: int) -> dict[str, object]:
    packet_id = "issue:work:wsi-20260719-999:native-git-fixture"
    reservation_id = packet_id + ":attempt:1"
    claims = [
        {"kind": "issue", "id": "WSI-20260719-999", "access": "read"},
        {"kind": "lane", "id": "workspace-issue/WSI-20260719-999", "access": "write"},
    ]
    ledger: dict[str, object] = {
        "version": version,
        "issues": [
            {
                "id": "WSI-20260719-999",
                "status": "in-progress",
                "kind": "improvement",
                "severity": "high",
                "created": "2026-07-19T00:00:00Z",
                "updated": "2026-07-19T00:00:00Z",
                "summary": "Use live validation",
                "area": "tools/recoil.py",
                "impact": "Content-bound gates obstruct source iteration.",
                "next_action": "Implement the live comparison path.",
                "requested_change": "Replace content-bound validation.",
                "benefit": "Docblock-only edits no longer invalidate semantic evidence.",
                "commands": [],
                "files": [],
                "tags": [],
                "history": [],
            }
        ],
        "work_packets": [
            {
                "id": packet_id,
                "issue_id": "WSI-20260719-999",
                "handoff_role": "recoil_tool_maintainer",
                "state": "active",
                "scope": "Implement live validation.",
                "next_command": "python tools/recoil.py audit live-validation-surface",
                "allowed_paths": ["tools"],
                "forbidden_paths": ["src"],
                "validation_commands": ["python -m unittest tests.tools.recoil_live_progress_tests"],
                "required_return_fields": ["exact_test_results"],
                "resource_claims": claims,
                "reservation_id": reservation_id,
                "outcome": None,
                "created": "2026-07-19T00:00:00Z",
                "updated": "2026-07-19T00:00:00Z",
            }
        ],
        "reservations": [
            {
                "id": reservation_id,
                "packet_id": packet_id,
                "state": "active",
                "created": "2026-07-19T00:00:00Z",
                "released": None,
                "outcome": None,
                "evidence_ids": [],
                "resource_claims": claims,
                "expires": None,
                "git_workspace_baseline": {
                    "schema": "recoil-git-workspace-baseline-v2",
                    "packet_id": packet_id,
                    "baseline_commit": "opaque-test-commit",
                    "branch": "test-packet",
                    "writable_paths": ["tools"],
                    "status_porcelain_v2": [],
                    "ignored_paths": [],
                    "git_object_ids_are_opaque": True,
                },
                "protected_progress_database_baseline": {"fixture": "unchanged"},
            }
        ],
    }
    if version == 2:
        ledger["revision"] = 12
        ledger["id_sequences"] = {}
        ledger["work_packets"][0]["semantic_contract_version"] = 1
        ledger["reservations"][0]["semantic_contract_version"] = 1
    return ledger


class WorkspaceIssueRevisionTests(unittest.TestCase):
    def _close_args(self, path: Path, *, revision: int, apply: bool) -> argparse.Namespace:
        return argparse.Namespace(
            ledger=path,
            progress=path.parent / "protected-progress.sqlite3",
            id="issue:work:wsi-20260719-999:native-git-fixture",
            outcome="returned",
            evidence_id=[],
            expected_revision=revision,
            dry_run=not apply,
            apply=apply,
        )

    def test_historical_version_one_git_baseline_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "issues.json"
            original = active_issue_ledger(version=1)
            historical = original["reservations"][0]["git_workspace_baseline"]
            historical["schema"] = "recoil-git-workspace-baseline-v1"
            historical.pop("status_porcelain_v2")
            historical.pop("ignored_paths")
            for apply in (False, True):
                with self.subTest(apply=apply):
                    path.write_text(json.dumps(original) + "\n", encoding="utf-8")
                    stderr = io.StringIO()
                    with redirect_stdout(io.StringIO()), redirect_stderr(stderr):
                        self.assertEqual(
                            workspace_issues.command_work_close(
                                self._close_args(path, revision=0, apply=apply)
                            ),
                            2,
                        )
                    self.assertIn("unsupported Git workspace baseline", stderr.getvalue())
                    self.assertEqual(
                        json.loads(path.read_text(encoding="utf-8")), original
                    )

    def test_version_two_close_uses_revision_only_concurrency(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "issues.json"
            path.write_text(
                json.dumps(active_issue_ledger(version=2)) + "\n",
                encoding="utf-8",
            )
            with patch.object(
                workspace_issues, "capture_git_closeout",
                return_value={"passed": True, "changed_paths": [], "unexpected_paths": []},
            ), patch.object(
                workspace_issues, "_capture_protected_progress_database",
                return_value={"fixture": "unchanged"},
            ), redirect_stdout(io.StringIO()):
                self.assertEqual(
                    workspace_issues.command_work_close(
                        self._close_args(path, revision=12, apply=False)
                    ),
                    0,
                )
            self.assertEqual(json.loads(path.read_text(encoding="utf-8"))["revision"], 12)
            with patch.object(
                workspace_issues, "capture_git_closeout",
                return_value={"passed": True, "changed_paths": [], "unexpected_paths": []},
            ), patch.object(
                workspace_issues, "_capture_protected_progress_database",
                return_value={"fixture": "unchanged"},
            ), redirect_stdout(io.StringIO()):
                self.assertEqual(
                    workspace_issues.command_work_close(
                        self._close_args(path, revision=12, apply=True)
                    ),
                    0,
                )
            closed = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(closed["revision"], 13)
            self.assertEqual(closed["work_packets"], [])
            self.assertEqual(closed["reservations"], [])
            with patch.object(
                workspace_issues, "capture_git_closeout",
                return_value={"passed": True, "changed_paths": [], "unexpected_paths": []},
            ), patch.object(
                workspace_issues, "_capture_protected_progress_database",
                return_value={"fixture": "unchanged"},
            ), redirect_stdout(io.StringIO()):
                self.assertEqual(
                    workspace_issues.command_work_close(
                        self._close_args(path, revision=12, apply=True)
                    ),
                    2,
                )


PADDING_FUNCTION_ID = "recoil:function:0x4bcb48"
PADDING_BLOCK_ID = "recoil:block:0x4bcb48"
PADDING_SPAN_ID = "recoil:semantic:0x4bcb48-0x4bcb50"


def function_padding_correction_fixture() -> tuple[dict[str, object], dict[str, object]]:
    data = empty_progress_document()
    data["revision"] = 71
    data["binaries"]["recoil"] = {
        "binary": "recoil",
        "primary_scheduler": True,
        "reference": {
            "image_base": "0x400000",
            "manifest_path": ".agent/REFERENCE_EXECUTABLE.json",
            "path": "support/Recoil.exe",
        },
        "inventory_snapshot": {
            "function_count": 1,
            "names_are_provisional": True,
            "source": "unit fixture",
        },
        "text": {"start": "0x401000", "end_exclusive": "0x4bcb50"},
    }
    data["physical_blocks"]["recoil:block:0x401000"] = {
        "accepted_order_facts": None,
        "agent_source_path": "src/fixture.cpp",
        "binary": "recoil",
        "contribution_ids": ["recoil:function:0x401000"],
        "contribution_kind": "authored",
        "end_exclusive": "0x401010",
        "first_external_callers": [],
        "mapping": {"state": "unresolved", "status": "fixture"},
        "order": _pending_order(),
        "order_diagnostic": {},
        "order_diagnostics": {},
        "order_targets": {"linked": "", "object": ""},
        "original_source_path": None,
        "provisional_original_path": "src/fixture.cpp",
        "row_kind": "physical-source-block",
        "semantic_span_ids": [],
        "source_path": "src/fixture.cpp",
        "source_shape_inputs": [],
        "candidate_header_contributors": [],
        "start": "0x401000",
    }
    data["symbols"]["recoil:function:0x401000"] = {
        "accepted_byte_facts": None,
        "accepted_order_facts": None,
        "address": "0x401000",
        "binary": "recoil",
        "binary_state": {},
        "end_exclusive": "0x401010",
        "kind": "function",
        "navigation_name": "FixtureFirstFunction",
        "physical_block_id": "recoil:block:0x401000",
        "pipeline_class": "authored",
        "semantic_span_ids": [],
    }
    data["physical_blocks"][PADDING_BLOCK_ID] = {
        "accepted_order_facts": None,
        "agent_source_path": "src/padding-fixture.cpp",
        "binary": "recoil",
        "contribution_ids": [PADDING_FUNCTION_ID],
        "contribution_kind": "authored",
        "end_exclusive": "0x4bcb50",
        "first_external_callers": [],
        "mapping": {
            "confidence": "unit fixture",
            "evidence_ids": [],
            "file_literal": None,
            "literal_xrefs": [],
            "state": "unresolved",
            "status": "fixture-padding",
        },
        "order": _pending_order(),
        "order_diagnostic": {},
        "order_diagnostics": {},
        "order_targets": {"linked": "", "object": ""},
        "original_source_path": None,
        "provisional_original_path": "src/padding-fixture.cpp",
        "row_kind": "physical-source-block",
        "semantic_span_ids": [PADDING_SPAN_ID],
        "source_path": "src/padding-fixture.cpp",
        "source_shape_inputs": [],
        "candidate_header_contributors": [],
        "start": "0x4bcb48",
    }
    data["semantic_spans"][PADDING_SPAN_ID] = {
        "binary": "recoil",
        "confidence": "unit fixture exact padding",
        "end_exclusive": "0x4bcb50",
        "evidence_ids": [],
        "navigation_aliases": [],
        "physical_block_id": PADDING_BLOCK_ID,
        "source_path": "padding:unit-fixture",
        "start": "0x4bcb48",
        "status": "padding",
        "symbol_ids": [PADDING_FUNCTION_ID],
    }
    data["symbols"][PADDING_FUNCTION_ID] = {
        "accepted_byte_facts": None,
        "accepted_order_facts": None,
        "address": "0x4bcb48",
        "binary": "recoil",
        "binary_state": {
            "linked_presence": {
                "disposition": "claim",
                "freshness": "current-unhashed",
                "result": "pending",
                "evidence_ids": [],
            }
        },
        "binary_state_diagnostics": {},
        "disposition": "unresolved",
        "end_exclusive": "0x4bcb50",
        "evidence_ids": [],
        "extent_state": "known",
        "kind": "function",
        "navigation_name": "FixturePaddingFalseFunction",
        "output_section_id": "recoil:section:.text",
        "ownership_state": "unresolved",
        "physical_block_id": PADDING_BLOCK_ID,
        "pipeline_class": "unresolved",
        "placement_exceptions": [
            {
                "address": "0x4bcb48",
                "classification": "padding/fallthrough",
                "name": "FixturePaddingFalseFunction",
                "physical_block": "unit-fixture",
                "summary": "Eight retail NOP bytes, not a function.",
            }
        ],
        "semantic_span_ids": [PADDING_SPAN_ID],
        "size": 8,
        "storage_contribution_ids": [],
        "verification_target_ids": [],
    }
    classification = {
        "authored_body_seeded": False,
        "authored_presence_seeded": False,
        "authored_target_seeded": False,
        "classification_reason": "no unique non-provider primary owner",
        "pipeline_class": "unresolved",
        "symbol_id": PADDING_FUNCTION_ID,
    }
    data["migration"] = {
        "schema_v4": {
            "symbol_classifications": [classification],
            "unclassified_symbols": [PADDING_FUNCTION_ID],
        }
    }
    block = data["physical_blocks"][PADDING_BLOCK_ID]
    payload = {
        "schema": "recoil-function-padding-correction-v1",
        "reviewed": True,
        "parent_reviewed": True,
        "reason": "Unit fixture proves exact immutable-retail NOP padding correction.",
        "binary": "recoil",
        "current_function": {
            "id": PADDING_FUNCTION_ID,
            "record": deepcopy(data["symbols"][PADDING_FUNCTION_ID]),
        },
        "current_block": {
            "id": PADDING_BLOCK_ID,
            "binary": "recoil",
            "start": block["start"],
            "end_exclusive": block["end_exclusive"],
            "row_kind": block["row_kind"],
            "contribution_kind": block["contribution_kind"],
            "source_path": block["source_path"],
            "agent_source_path": block["agent_source_path"],
            "original_source_path": block["original_source_path"],
            "provisional_original_path": block["provisional_original_path"],
            "mapping_state": block["mapping"]["state"],
            "mapping_status": block["mapping"]["status"],
            "accepted_order_facts": None,
            "expected_contribution_count": 1,
            "expected_contains_function_id": PADDING_FUNCTION_ID,
            "expected_function_membership_count": 1,
            "expected_semantic_span_count": 1,
            "expected_contains_semantic_span_id": PADDING_SPAN_ID,
            "expected_semantic_span_membership_count": 1,
            "source_shape_input_count": 0,
            "candidate_header_contributor_count": 0,
        },
        "current_semantic_span": {
            "id": PADDING_SPAN_ID,
            "record": deepcopy(data["semantic_spans"][PADDING_SPAN_ID]),
        },
        "replacement_padding": {
            "start": "0x4bcb48",
            "end_exclusive": "0x4bcb50",
            "retail_bytes_hex": "9090909090909090",
            "keep_physical_block_id": PADDING_BLOCK_ID,
            "keep_semantic_span_id": PADDING_SPAN_ID,
            "replacement_symbol_ids": [],
            "remove_function_id": PADDING_FUNCTION_ID,
        },
    }
    return data, payload


class FunctionPaddingCorrectionTests(unittest.TestCase):
    def _parsed(
        self,
        payload: dict[str, object],
    ) -> dict[str, object]:
        return _parse_function_padding_correction_payload(json.dumps(payload))

    def test_exact_nops_apply_only_the_scoped_diff_and_preserve_frontiers(self) -> None:
        data, raw_payload = function_padding_correction_fixture()
        before = deepcopy(data)
        details = _replace_function_with_padding(data, self._parsed(raw_payload))

        expected = deepcopy(before)
        del expected["symbols"][PADDING_FUNCTION_ID]
        expected["physical_blocks"][PADDING_BLOCK_ID]["contribution_ids"] = []
        expected["semantic_spans"][PADDING_SPAN_ID]["symbol_ids"] = []
        expected["migration"]["schema_v4"]["symbol_classifications"] = []
        expected["migration"]["schema_v4"]["unclassified_symbols"] = []
        self.assertEqual(expected, data)
        self.assertEqual("9090909090909090", details["padding"]["retail_bytes_hex"])
        self.assertTrue(details["padding"]["verified_all_nop_0x90"])
        self.assertEqual(details["frontiers_before"], details["frontiers_after"])
        self.assertEqual(1, details["tracker_function_count_after"])
        self.assertEqual(
            1,
            data["binaries"]["recoil"]["inventory_snapshot"]["function_count"],
        )
        self.assertIn(PADDING_BLOCK_ID, data["physical_blocks"])
        self.assertIn(PADDING_SPAN_ID, data["semantic_spans"])
        self.assertEqual([], data["semantic_spans"][PADDING_SPAN_ID]["symbol_ids"])

    def test_exact_nops_do_not_require_obsolete_schema_v4_metadata(self) -> None:
        data, raw_payload = function_padding_correction_fixture()
        data["migration"].pop("schema_v4")
        details = _replace_function_with_padding(data, self._parsed(raw_payload))

        self.assertNotIn(PADDING_FUNCTION_ID, data["symbols"])
        self.assertNotIn("schema_v4", data["migration"])
        self.assertEqual(
            0,
            details["removed_relationships"]["schema_v4_symbol_classifications"],
        )
        self.assertEqual(
            0,
            details["removed_relationships"][
                "schema_v4_unclassified_symbol_memberships"
            ],
        )

    def test_rejects_payload_mismatch_and_nonpadding_retail_bytes(self) -> None:
        data, raw_payload = function_padding_correction_fixture()
        raw_payload["replacement_padding"]["retail_bytes_hex"] = "cccccccccccccccc"
        with self.assertRaisesRegex(ProgressError, "immutable retail bytes do not exactly match"):
            _replace_function_with_padding(data, self._parsed(raw_payload))

        data, raw_payload = function_padding_correction_fixture()
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            reference = root / "support" / "Recoil.exe"
            reference.parent.mkdir(parents=True)
            image = bytearray(canonical_retail_reference().read_bytes())
            image[0xBBF48] = 0xCC
            reference.write_bytes(image)
            with (
                patch.object(progress_cli, "REPO_ROOT", root),
                patch.object(progress_cli, "MACHINE_RETAIL_REFERENCE", reference),
                self.assertRaisesRegex(
                    ProgressError,
                    "not exact 0x90 NOP padding",
                ),
            ):
                raw_payload["replacement_padding"]["retail_bytes_hex"] = (
                    "cc90909090909090"
                )
                _replace_function_with_padding(data, self._parsed(raw_payload))

    def test_rejects_unsafe_relationships_and_accepted_facts(self) -> None:
        cases = []
        for collection, label in (
            ("owners", "owner"),
            ("verification_targets", "verification-target"),
            ("storage_contributions", "storage"),
            ("work_items", "work-item"),
        ):
            data, payload = function_padding_correction_fixture()
            data[collection][f"recoil:fixture:{collection}"] = {
                "scope_ids": [PADDING_FUNCTION_ID]
            }
            cases.append((data, payload, label))

        data, payload = function_padding_correction_fixture()
        data["symbols"][PADDING_FUNCTION_ID]["accepted_order_facts"] = {"accepted": True}
        payload["current_function"]["record"] = deepcopy(
            data["symbols"][PADDING_FUNCTION_ID]
        )
        cases.append((data, payload, "accepted-order"))

        data, payload = function_padding_correction_fixture()
        data["symbols"][PADDING_FUNCTION_ID]["accepted_byte_facts"] = {"accepted": True}
        payload["current_function"]["record"] = deepcopy(
            data["symbols"][PADDING_FUNCTION_ID]
        )
        cases.append((data, payload, "accepted-byte"))

        data, payload = function_padding_correction_fixture()
        data["symbols"][PADDING_FUNCTION_ID]["binary_state"]["object_byte"] = {
            "result": "passed",
            "disposition": "accepted",
            "freshness": "current",
            "gating": True,
            "validation_mode": "live",
            "evidence_ids": ["recoil:evidence:r1:000001"],
        }
        payload["current_function"]["record"] = deepcopy(
            data["symbols"][PADDING_FUNCTION_ID]
        )
        cases.append((data, payload, "accepted binary-state"))

        data, payload = function_padding_correction_fixture()
        data["physical_blocks"][PADDING_BLOCK_ID]["order"]["authored"][
            "object_identity_presence"
        ] = {
            "result": "passed",
            "disposition": "accepted",
            "freshness": "current",
            "gating": True,
            "validation_mode": "live",
            "evidence_ids": ["recoil:evidence:r1:000001"],
        }
        cases.append((data, payload, "physical block with accepted order"))

        for data, payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(
                ProgressError,
                message,
            ):
                _replace_function_with_padding(data, self._parsed(payload))

    def test_rejects_stale_function_block_span_and_migration_guards(self) -> None:
        cases = []
        data, payload = function_padding_correction_fixture()
        payload["current_function"]["record"]["navigation_name"] = "stale"
        cases.append((data, payload, "function.*stale"))

        data, payload = function_padding_correction_fixture()
        payload["current_block"]["expected_contribution_count"] = 2
        cases.append((data, payload, "block.*stale"))

        data, payload = function_padding_correction_fixture()
        payload["current_semantic_span"]["record"]["confidence"] = "stale"
        cases.append((data, payload, "span.*stale"))

        data, payload = function_padding_correction_fixture()
        data["migration"]["schema_v4"]["unclassified_symbols"].append(
            PADDING_FUNCTION_ID
        )
        cases.append((data, payload, "ambiguous schema-v4 unclassified"))

        for data, payload, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(
                ProgressError,
                message,
            ):
                _replace_function_with_padding(data, self._parsed(payload))

    def test_cli_dry_run_apply_and_revision_cas(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            data, raw_payload = function_padding_correction_fixture()
            path.write_text(json.dumps(data) + "\n", encoding="utf-8")
            payload_json = json.dumps(raw_payload)

            stdout = io.StringIO()
            stderr = io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                dry_rc = progress_cli.main(
                    [
                        "symbol",
                        "replace-padding",
                        "--progress",
                        str(path),
                        "--payload-json",
                        payload_json,
                        "--expected-revision",
                        "71",
                        "--dry-run",
                        "--json",
                    ]
                )
            self.assertEqual(0, dry_rc, stderr.getvalue())
            dry = json.loads(stdout.getvalue())
            self.assertFalse(dry["commit"]["applied"])
            self.assertEqual(72, dry["commit"]["revision"])
            self.assertEqual(data, json.loads(path.read_text(encoding="utf-8")))

            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                stale_rc = progress_cli.main(
                    [
                        "symbol",
                        "replace-padding",
                        "--progress",
                        str(path),
                        "--payload-json",
                        payload_json,
                        "--expected-revision",
                        "70",
                        "--apply",
                        "--json",
                    ]
                )
            self.assertEqual(2, stale_rc)
            self.assertEqual(data, json.loads(path.read_text(encoding="utf-8")))

            stdout = io.StringIO()
            stderr = io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                apply_rc = progress_cli.main(
                    [
                        "symbol",
                        "replace-padding",
                        "--progress",
                        str(path),
                        "--payload-json",
                        payload_json,
                        "--expected-revision",
                        "71",
                        "--apply",
                        "--json",
                    ]
                )
            self.assertEqual(0, apply_rc, stderr.getvalue())
            applied = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(72, applied["revision"])
            self.assertNotIn(PADDING_FUNCTION_ID, applied["symbols"])
            self.assertEqual(
                [],
                applied["semantic_spans"][PADDING_SPAN_ID]["symbol_ids"],
            )


if __name__ == "__main__":
    unittest.main()
