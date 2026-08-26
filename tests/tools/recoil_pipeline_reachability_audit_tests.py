from __future__ import annotations

from pathlib import Path
import contextlib
import io
import json
import os
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))


def canonical_input_root() -> Path:
    return Path(os.environ.get("RECOIL_CANONICAL_ROOT", str(REPO_ROOT))).resolve()

from _recoil.commands import progress_cli, workspace_issues  # noqa: E402
from _recoil.commands.pipeline_reachability_audit import (  # noqa: E402
    REQUIRED_COMMANDS,
    _bound_call_contract_include_roots,
    _bound_vc5_tracker,
    _probe_byte_lanes,
    _probe_call_contract,
    _routed_call_contract_include_roots,
    audit_pipeline_reachability,
    build_parser,
)
from _recoil.lib.issue_sqlite import create_issue_database  # noqa: E402


def valid_specs() -> list[dict[str, object]]:
    return [
        {
            "path": path,
            "module": contract.module,
            "prepend_args": contract.prepend,
            "mutates": contract.mutates,
        }
        for path, contract in REQUIRED_COMMANDS.items()
    ]


def reports() -> dict[str, object]:
    return {
        "order_report": {
            "reachable": True,
            "status": "ready",
            "complete_contiguous_slices": True,
            "covered_block_ids": ["recoil:block:0x401060", "recoil:block:0x402f60"],
            "next_command": "python tools/recoil.py progress advance-live-order --target sample --apply",
            "launchable_with_command": True,
        },
        "byte_report": {
            "reachable": True,
            "status": "ready",
            "verify_commands": {
                "object": "authored-object-byte",
                "authored": "authored-byte",
                "linked": "linked-byte",
            },
        },
        "call_contract_readiness_report": {
            "report_version": 2,
            "kind": "call-contract-readiness-audit",
            "validation_mode": "live-tracker-exact-source-closure",
            "candidate_independent_slice_membership": True,
            "source_authority": "accepted-target-registration",
            "all_slices": True,
            "original_slice_count": 22,
            "selected_slice_count": 22,
            "ready_slice_count": 22,
            "blocked_slice_count": 0,
            "accepted_state_body_count": 22,
            "current_accepted_state_count": 0,
            "pending_accepted_state_count": 22,
            "phase_closeout_required": True,
            "passed": True,
            "producer_operational": True,
            "infrastructure_findings": [],
            "candidate_readiness": False,
            "typed_reconstruction_blockers": [
                {
                    "kind": "call-contract-bodies-not-current",
                    "message": "fixture bodies remain pending",
                    "slice_id": "recoil:call-contract-slice:fixture-1",
                }
            ],
            "stale_registration_blockers": [],
            "slices": [
                {
                    "slice_id": f"recoil:call-contract-slice:fixture-{ordinal}",
                    "ordinal": ordinal,
                    "status": "ready",
                    "body_count": 1,
                    "accepted_state_schema": "direct-body-acceptance-v1",
                    "current_accepted_state_count": 0,
                    "pending_accepted_state_count": 1,
                }
                for ordinal in range(1, 23)
            ],
            "blockers": [
                {
                    "kind": "call-contract-bodies-not-current",
                    "message": "fixture bodies remain pending",
                    "slice_id": "recoil:call-contract-slice:fixture-1",
                }
            ],
        },
        "call_contract_report": _probe_call_contract(progress_cli),
        "relocation_report": {
            "kind": "relocation-expectations-audit",
            "validation_mode": "live-retail-derived",
            "candidate_independent": True,
            "passed": False,
            "reports": [
                {
                    "kind": "retail-relocation-expectations",
                    "validation_mode": "live-retail-derived",
                    "candidate_independent": True,
                    "passed": False,
                    "status": "unresolved",
                    "expectations": [],
                    "unresolved": [{"kind": "missing-target-identity"}],
                }
            ],
            "reviewed_exception_route": {
                "reachable": True,
                "revision_guarded": True,
                "dry_run_available": True,
            },
        },
        "final_catalog_report": {
            "kind": "final-image-catalog-audit",
            "validation_mode": "live-retail-plus-accepted-tracker",
            "legacy_catalog_required": False,
            "passed": False,
            "coverage": {
                "kind": "live-final-image-coverage",
                "validation_mode": "live-retail-plus-accepted-tracker",
                "complete": False,
                "failures": ["section .text has unresolved typed annotations"],
            },
            "failures": ["section .text has unresolved typed annotations"],
        },
        "final_verify_report": {
            "reachable": True,
            "status": "blocked-before-build",
            "uses_live_coverage": True,
            "build_started": False,
            "blocker": "live typed final-image coverage is incomplete",
        },
    }


def typed_blocked_order_report() -> dict[str, object]:
    address = "0x416790"
    identity = "recoil:function:0x416790"
    return {
        "reachable": True,
        "status": "blocked",
        "phase": "authored-function-order",
        "cursor_block_id": "recoil:block:0x415ab0",
        "reason_code": "order-target-role-gate-blocked",
        "reason": (
            "target 'recoil:vc5-target:map_text_block_order_current_shape' cannot cover a "
            f"whole block while order row {address} ({identity}; "
            "HudSensorTracker::MapShutdownAndResetThunk) is unresolved"
        ),
        "blocker": {
            "kind": "order-target-role-gate",
            "target_id": "recoil:vc5-target:map_text_block_order_current_shape",
            "phase": "authored-function-order",
            "address": address,
            "identity": identity,
            "label": "HudSensorTracker::MapShutdownAndResetThunk",
            "problems": [
                "registration pipeline_class='unresolved'",
                "registration authored_order_role='unresolved'",
            ],
        },
        "next_command": "",
        "launchable_with_command": False,
    }


def typed_blocked_readiness_report() -> dict[str, object]:
    ready_body_counts = [160] * 19 + [138]
    ready_slices = [
        {
            "slice_id": f"recoil:call-contract-slice:fixture-{ordinal}",
            "ordinal": ordinal,
            "status": "ready",
            "candidate_status": "not-ready",
            "body_count": body_count,
            "accepted_state_schema": "direct-body-acceptance-v1",
            "current_accepted_state_count": 0,
            "pending_accepted_state_count": body_count,
        }
        for ordinal, body_count in enumerate(ready_body_counts, start=1)
    ]
    blocked_slice_id = "recoil:call-contract-slice:0x4317b0-0x438920"
    stale_blocker = {
        "kind": "manifest-registration-drift",
        "slice_id": blocked_slice_id,
        "ordinal": 21,
        "target": "recoil:vc5-target:zui_438920_438980_authored_order",
        "removed": ["src/GameZRecoil/zUI/zui.cpp"],
        "added": ["src/GameZRecoil/zUI/zui_widgets.cpp"],
    }
    pending_blockers = [
        {
            "kind": "call-contract-bodies-not-current",
            "message": f"{body_count} selected call-contract bodies are not current",
            "slice_id": row["slice_id"],
            "ordinal": row["ordinal"],
        }
        for row, body_count in zip(ready_slices, ready_body_counts)
    ]
    return {
        "report_version": 2,
        "kind": "call-contract-readiness-audit",
        "validation_mode": "live-tracker-exact-source-closure",
        "candidate_independent_slice_membership": True,
        "source_authority": "accepted-target-registration",
        "all_slices": True,
        "original_slice_count": 21,
        "selected_slice_count": 21,
        "ready_slice_count": 20,
        "blocked_slice_count": 1,
        "accepted_state_body_count": 3178,
        "current_accepted_state_count": 0,
        "pending_accepted_state_count": 3178,
        "phase_closeout_required": True,
        "passed": True,
        "producer_operational": True,
        "infrastructure_findings": [],
        "candidate_readiness": False,
        "typed_reconstruction_blockers": pending_blockers,
        "stale_registration_blockers": [stale_blocker],
        "slices": [
            *ready_slices,
            {
                "slice_id": blocked_slice_id,
                "ordinal": 21,
                "status": "blocked",
                "candidate_status": "not-ready",
                "producer_operational": True,
                "body_count": 160,
                "stale_registration_blockers": [stale_blocker],
            },
        ],
        "blockers": [*pending_blockers, stale_blocker],
    }


class PipelineReachabilityAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        super().setUpClass()
        cls.issue_temporary = tempfile.TemporaryDirectory()
        cls.issue_ledger = Path(cls.issue_temporary.name) / "issues.sqlite3"
        create_issue_database(
            cls.issue_ledger,
            workspace_issues.empty_ledger(),
            cutover_pair_id="pair:test:pipeline-reachability",
        )

    @classmethod
    def tearDownClass(cls) -> None:
        cls.issue_temporary.cleanup()
        super().tearDownClass()

    def run_fixture(self, payload: dict[str, object] | None = None):
        selected = reports() if payload is None else payload
        with tempfile.TemporaryDirectory() as temporary:
            # The injected producer reports make this a non-mutating fixture; the path
            # proves the audit does not require or rewrite a real tracker.
            tracker = Path(temporary) / "progress.json"
            return audit_pipeline_reachability(
                specs=valid_specs(),
                tracker=tracker,
                progress_module=progress_cli,
                **selected,
            )

    def test_deep_vc5_probes_use_and_restore_authenticated_tracker(self) -> None:
        from _recoil.commands import vc5_verify

        payload = reports()
        expected_readiness = payload.pop("call_contract_readiness_report")
        expected_relocations = payload.pop("relocation_report")
        canonical = canonical_input_root()
        tracker = canonical / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
        reference = canonical / "support" / "Recoil.exe"
        original_tracker = vc5_verify.DEFAULT_PROGRESS_PATH
        observed: list[tuple[str, Path]] = []

        def readiness_probe(
            *, tracker: Path, execution_root: Path, canonical_root: Path | None
        ):
            observed.append(("readiness", vc5_verify.DEFAULT_PROGRESS_PATH))
            self.assertEqual(tracker.resolve(strict=True), vc5_verify.DEFAULT_PROGRESS_PATH)
            self.assertEqual(REPO_ROOT.resolve(), execution_root)
            self.assertEqual(canonical, canonical_root)
            return expected_readiness

        def relocation_probe(*_args, **_kwargs):
            observed.append(("relocations", vc5_verify.DEFAULT_PROGRESS_PATH))
            self.assertEqual(tracker.resolve(strict=True), vc5_verify.DEFAULT_PROGRESS_PATH)
            return expected_relocations

        with (
            patch(
                "_recoil.commands.pipeline_reachability_audit._probe_call_contract_readiness",
                side_effect=readiness_probe,
            ),
            patch(
                "_recoil.commands.pipeline_reachability_audit._probe_relocations",
                side_effect=relocation_probe,
            ),
        ):
            report = audit_pipeline_reachability(
                specs=valid_specs(),
                tracker=tracker,
                reference=reference,
                canonical_root=canonical,
                progress_module=progress_cli,
                **payload,
            )

        self.assertTrue(report["passed"], report["failures"])
        self.assertEqual(
            [("readiness", tracker.resolve()), ("relocations", tracker.resolve())],
            observed,
        )
        self.assertEqual(original_tracker, vc5_verify.DEFAULT_PROGRESS_PATH)

    def test_linked_call_contract_include_roots_route_only_machine_local_sdks(self) -> None:
        from _recoil.commands import call_contract_verify

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            execution = root / "linked"
            canonical = root / "canonical"
            config = execution / "tools" / "_recoil" / "config" / "vc5_final_build.json"
            config.parent.mkdir(parents=True)
            (execution / "tools" / "_recoil" / "compat" / "include").mkdir(parents=True)
            (execution / "src").mkdir()
            aureal = canonical / "support" / "sdk" / "Aureal" / "A3D20" / "inc"
            directx = canonical / "support" / "sdk" / "DirectX6" / "include"
            aureal.mkdir(parents=True)
            directx.mkdir(parents=True)
            config.write_text(
                json.dumps({
                    "include_dirs": [
                        "tools/_recoil/compat/include",
                        "src",
                        "support/sdk/Aureal/A3D20/inc",
                        "support/sdk/DirectX6/include",
                        "D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE",
                    ]
                }),
                encoding="utf-8",
            )
            original = call_contract_verify._call_contract_repository_include_roots
            with patch.object(
                call_contract_verify,
                "DEFAULT_FINAL_BUILD_MANIFEST",
                config,
            ):
                roots, identities = _routed_call_contract_include_roots(
                    execution_root=execution,
                    canonical_root=canonical,
                )
                with _bound_call_contract_include_roots(
                    execution_root=execution,
                    canonical_root=canonical,
                ):
                    self.assertEqual(
                        roots,
                        call_contract_verify._call_contract_repository_include_roots(),
                    )
            self.assertIs(
                original,
                call_contract_verify._call_contract_repository_include_roots,
            )
            projection = dict(roots)
            self.assertEqual(
                (execution / "tools" / "_recoil" / "compat" / "include").resolve(),
                projection["tools/_recoil/compat/include"],
            )
            self.assertEqual((execution / "src").resolve(), projection["src"])
            self.assertEqual(aureal.resolve(), projection["support/sdk/Aureal/A3D20/inc"])
            self.assertEqual(directx.resolve(), projection["support/sdk/DirectX6/include"])
            self.assertEqual(2, len(identities))
            self.assertFalse((execution / "support").exists())

    def test_linked_call_contract_include_roots_fail_closed(self) -> None:
        from _recoil.commands import call_contract_verify

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            execution = root / "linked"
            canonical = root / "canonical"
            config = execution / "tools" / "_recoil" / "config" / "vc5_final_build.json"
            config.parent.mkdir(parents=True)
            canonical.mkdir()
            config.write_text(
                json.dumps({"include_dirs": ["support/sdk/Aureal/A3D20/inc"]}),
                encoding="utf-8",
            )
            with patch.object(
                call_contract_verify,
                "DEFAULT_FINAL_BUILD_MANIFEST",
                config,
            ):
                with self.assertRaisesRegex(Exception, "does not exist"):
                    _routed_call_contract_include_roots(
                        execution_root=execution,
                        canonical_root=canonical,
                    )
                copied = execution / "support" / "sdk" / "Aureal" / "A3D20" / "inc"
                copied.mkdir(parents=True)
                with self.assertRaisesRegex(Exception, "unexpectedly contains"):
                    _routed_call_contract_include_roots(
                        execution_root=execution,
                        canonical_root=canonical,
                    )

    def test_vc5_tracker_binding_restores_global_and_caches_after_failure(self) -> None:
        from _recoil.commands import vc5_verify

        tracker = (
            canonical_input_root()
            / ".agent"
            / "RECONSTRUCTION_PROGRESS.sqlite3"
        )
        original_tracker = vc5_verify.DEFAULT_PROGRESS_PATH
        caches = (
            vc5_verify.canonical_tracker_artifact_index,
            vc5_verify.canonical_tracker_data,
            vc5_verify.registered_vc5_manifest_paths,
            vc5_verify.canonical_tracker_function_metadata,
        )
        with self.assertRaisesRegex(RuntimeError, "injected deep-probe failure"):
            with _bound_vc5_tracker(tracker):
                self.assertEqual(
                    tracker.resolve(strict=True), vc5_verify.DEFAULT_PROGRESS_PATH
                )
                raise RuntimeError("injected deep-probe failure")
        self.assertEqual(original_tracker, vc5_verify.DEFAULT_PROGRESS_PATH)
        self.assertTrue(all(function.cache_info().currsize == 0 for function in caches))

    def test_blocked_current_facts_still_pass_structural_reachability(self) -> None:
        report = self.run_fixture()
        self.assertTrue(report["passed"], report["failures"])
        self.assertFalse(report["reconstruction_complete"])
        self.assertEqual("blocked", report["producer_states"]["relocations"])
        self.assertEqual("blocked", report["producer_states"]["final_coverage"])

    def test_typed_blocked_order_lane_is_healthy_but_reconstruction_is_incomplete(self) -> None:
        payload = reports()
        payload["order_report"] = typed_blocked_order_report()
        report = self.run_fixture(payload)

        self.assertTrue(report["passed"], report["failures"])
        self.assertTrue(report["structural_reachability"])
        self.assertFalse(report["reconstruction_complete"])
        self.assertEqual("blocked", report["producer_states"]["order"])
        self.assertEqual(
            typed_blocked_order_report()["blocker"],
            report["producers"]["current_order"]["blocker"],
        )

    def test_unknown_or_malformed_order_blocker_is_not_healthy(self) -> None:
        for mutation in (
            "unknown-code",
            "unknown-status",
            "missing-identity",
            "unexpected-command",
        ):
            with self.subTest(mutation=mutation):
                payload = reports()
                blocked = typed_blocked_order_report()
                if mutation == "unknown-code":
                    blocked["reason_code"] = "order-target-unresolved"
                elif mutation == "unknown-status":
                    blocked["status"] = "stalled"
                elif mutation == "missing-identity":
                    blocked["blocker"].pop("identity")
                else:
                    blocked["next_command"] = "python tools/recoil.py progress advance-live-order"
                    blocked["launchable_with_command"] = True
                payload["order_report"] = blocked
                report = self.run_fixture(payload)
                self.assertFalse(report["passed"])
                self.assertTrue(
                    any(item["check"] == "order-producer" for item in report["failures"])
                )

    def test_blocked_order_prevents_complete_claim_even_if_later_reports_pass(self) -> None:
        payload = reports()
        payload["order_report"] = typed_blocked_order_report()
        payload["relocation_report"]["passed"] = True
        payload["final_catalog_report"]["passed"] = True
        payload["final_catalog_report"]["coverage"]["complete"] = True
        payload["final_catalog_report"]["coverage"]["failures"] = []
        report = self.run_fixture(payload)

        self.assertTrue(report["passed"], report["failures"])
        self.assertFalse(report["reconstruction_complete"])

    def test_candidate_derived_relocation_truth_is_rejected(self) -> None:
        payload = reports()
        payload["relocation_report"]["candidate_independent"] = False
        report = self.run_fixture(payload)
        self.assertFalse(report["passed"])
        self.assertTrue(
            any("candidate-derived" in item["message"] for item in report["failures"])
        )

    def test_nonoperational_call_contract_readiness_is_a_pipeline_failure(self) -> None:
        payload = reports()
        readiness = payload["call_contract_readiness_report"]
        readiness["passed"] = False
        readiness["producer_operational"] = False
        readiness["infrastructure_findings"] = [
            {"kind": "producer-invocation", "message": "fixture failure"}
        ]
        readiness["ready_slice_count"] = 21
        readiness["blocked_slice_count"] = 1
        readiness["blockers"] = [
            {
                "kind": "source-dependency-closure",
                "slice_id": "recoil:call-contract-slice:0x470b10-0x476070",
                "ordinal": 14,
                "message": "noncanonical source path",
            }
        ]
        report = self.run_fixture(payload)
        self.assertFalse(report["passed"])
        self.assertEqual(
            "blocked", report["producer_states"]["call_contract_readiness"]
        )
        self.assertTrue(
            any(
                item["check"] == "call-contract-readiness"
                for item in report["failures"]
            )
        )

    def test_typed_readiness_blocker_is_operational_infrastructure(self) -> None:
        payload = reports()
        payload["call_contract_readiness_report"] = typed_blocked_readiness_report()
        report = self.run_fixture(payload)
        self.assertTrue(report["passed"], report["failures"])
        self.assertFalse(report["reconstruction_complete"])
        self.assertEqual(
            "operational-not-ready",
            report["producer_states"]["call_contract_readiness"],
        )
        readiness = report["producers"]["call_contract_readiness"]
        self.assertEqual(3178, readiness["accepted_state_body_count"])
        self.assertEqual(160, readiness["slices"][-1]["body_count"])
        self.assertEqual(
            "manifest-registration-drift",
            readiness["slices"][-1]["stale_registration_blockers"][0]["kind"],
        )

    def test_typed_blocked_readiness_coverage_fails_closed(self) -> None:
        mutations = (
            "ready-missing-schema",
            "ready-missing-counts",
            "blocked-missing-body-count",
            "blocked-noninteger-body-count",
            "blocked-zero-body-count",
            "blocked-without-typed-blocker",
            "blocked-with-unknown-blocker",
            "blocked-overlaps-accepted-state",
            "aggregate-below-census",
            "aggregate-above-census",
            "blocked-claims-readiness",
            "blocked-claims-acceptance",
            "duplicate-slice",
        )
        for mutation in mutations:
            with self.subTest(mutation=mutation):
                payload = reports()
                readiness = typed_blocked_readiness_report()
                ready_row = readiness["slices"][0]
                blocked_row = readiness["slices"][-1]
                if mutation == "ready-missing-schema":
                    ready_row.pop("accepted_state_schema")
                elif mutation == "ready-missing-counts":
                    ready_row.pop("pending_accepted_state_count")
                elif mutation == "blocked-missing-body-count":
                    blocked_row.pop("body_count")
                elif mutation == "blocked-noninteger-body-count":
                    blocked_row["body_count"] = 160.0
                elif mutation == "blocked-zero-body-count":
                    blocked_row["body_count"] = 0
                elif mutation == "blocked-without-typed-blocker":
                    readiness["stale_registration_blockers"] = []
                    readiness["blockers"] = list(
                        readiness["typed_reconstruction_blockers"]
                    )
                    blocked_row["stale_registration_blockers"] = []
                elif mutation == "blocked-with-unknown-blocker":
                    unknown = {
                        "kind": "unexpected-blocker",
                        "slice_id": blocked_row["slice_id"],
                    }
                    readiness["stale_registration_blockers"] = [unknown]
                    readiness["blockers"][-1] = unknown
                    blocked_row["stale_registration_blockers"] = [unknown]
                elif mutation == "blocked-overlaps-accepted-state":
                    blocked_row["accepted_state_schema"] = "direct-body-acceptance-v1"
                    blocked_row["current_accepted_state_count"] = 0
                    blocked_row["pending_accepted_state_count"] = 160
                elif mutation == "aggregate-below-census":
                    readiness["accepted_state_body_count"] -= 1
                    readiness["pending_accepted_state_count"] -= 1
                elif mutation == "aggregate-above-census":
                    readiness["accepted_state_body_count"] += 1
                    readiness["pending_accepted_state_count"] += 1
                elif mutation == "blocked-claims-readiness":
                    blocked_row["candidate_status"] = "ready"
                elif mutation == "blocked-claims-acceptance":
                    blocked_row["candidate_status"] = "accepted"
                else:
                    readiness["slices"][-1]["slice_id"] = readiness["slices"][0][
                        "slice_id"
                    ]
                payload["call_contract_readiness_report"] = readiness
                report = self.run_fixture(payload)
                self.assertFalse(report["passed"])
                self.assertTrue(
                    any(
                        item["check"] == "call-contract-readiness"
                        for item in report["failures"]
                    ),
                    report["failures"],
                )

    def test_unreachable_relocation_ambiguity_route_is_rejected(self) -> None:
        payload = reports()
        payload["relocation_report"]["reviewed_exception_route"]["reachable"] = False
        report = self.run_fixture(payload)
        self.assertFalse(report["passed"])
        self.assertTrue(
            any("ambiguity route" in item["message"] for item in report["failures"])
        )

    def test_legacy_blob_only_final_path_is_rejected(self) -> None:
        payload = reports()
        payload["final_catalog_report"]["legacy_catalog_required"] = True
        payload["final_catalog_report"]["coverage"] = None
        report = self.run_fixture(payload)
        self.assertFalse(report["passed"])
        messages = [item["message"] for item in report["failures"]]
        self.assertTrue(any("legacy catalog blob" in item for item in messages))
        self.assertTrue(any("lacks live coverage" in item for item in messages))

    def test_incomplete_coverage_requires_an_explicit_typed_blocker(self) -> None:
        payload = reports()
        payload["final_catalog_report"]["coverage"]["failures"] = []
        report = self.run_fixture(payload)
        self.assertFalse(report["passed"])
        self.assertTrue(
            any("no explicit blocker" in item["message"] for item in report["failures"])
        )

    def test_partial_order_slice_is_rejected(self) -> None:
        payload = reports()
        payload["order_report"]["complete_contiguous_slices"] = False
        report = self.run_fixture(payload)
        self.assertFalse(report["passed"])
        self.assertTrue(
            any(item["check"] == "order-producer" for item in report["failures"])
        )

    def test_launchable_order_cursor_without_command_is_rejected(self) -> None:
        payload = reports()
        payload["order_report"]["next_command"] = ""
        payload["order_report"]["launchable_with_command"] = False
        report = self.run_fixture(payload)
        self.assertFalse(report["passed"])
        self.assertTrue(
            any(
                item["check"] == "order-producer" and "executable" in item["message"]
                for item in report["failures"]
            )
        )

    def test_byte_parser_exposes_all_three_advance_lanes(self) -> None:
        report = _probe_byte_lanes(progress_cli)
        self.assertEqual(
            {
                "object": "authored-object-byte",
                "authored": "authored-byte",
                "linked": "linked-byte",
            },
            report["verify_commands"],
        )

    def test_direct_call_contract_route_and_contained_repair_are_reachable(self) -> None:
        report = _probe_call_contract(progress_cli)
        self.assertEqual("ready", report["status"])
        self.assertTrue(report["acceptance_enabled"])
        self.assertEqual("verify call-contract", report["verify_command"])
        self.assertEqual(
            "progress advance-live-call-contract", report["advance_command"]
        )
        self.assertTrue(report["direct_route"]["packet_reservation_guard"])
        self.assertTrue(report["direct_route"]["parent_fresh_direct_retail"])
        self.assertFalse(report["direct_route"]["worker_results_accepting"])
        continuation = report["repair_continuation"]
        self.assertTrue(continuation["reachable"])
        self.assertTrue(continuation["parent_only"])
        self.assertTrue(continuation["contained_disabled_before_work"])
        self.assertTrue(continuation["nonaccepting"])
        self.assertFalse(continuation["acceptance_eligible"])

    def test_call_contract_parent_routes_reject_missing_packet_identity(self) -> None:
        parser = progress_cli._parser()
        commands = (
            [
                "advance-live-call-contract",
                "--slice",
                "recoil:call-contract-slice:test",
                "--build-root",
                "build/test",
                "--expected-semantic-revision",
                "1",
                "--expected-evidence-generation-revision",
                "1",
                "--dry-run",
            ],
            [
                "call-contract",
                "prepare-live-convergence",
                "--closeout",
                "--build-root",
                "build/test",
                "--jobs",
                "1",
                "--expected-semantic-revision",
                "1",
                "--expected-evidence-generation-revision",
                "1",
                "--dry-run",
            ],
        )
        for command in commands:
            with self.subTest(command=command), contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit):
                    parser.parse_args(command)

    def test_accepting_repair_continuation_is_a_pipeline_failure(self) -> None:
        payload = reports()
        payload["call_contract_report"]["repair_continuation"][
            "acceptance_eligible"
        ] = True
        report = self.run_fixture(payload)
        self.assertFalse(report["passed"])
        self.assertTrue(
            any(
                item["check"] == "call-contract-repair-continuation"
                for item in report["failures"]
            )
        )

    def test_missing_producer_command_is_a_structural_failure(self) -> None:
        specs = [
            item
            for item in valid_specs()
            if tuple(item["path"]) != ("audit", "relocation-expectations")
        ]
        with tempfile.TemporaryDirectory() as temporary:
            report = audit_pipeline_reachability(
                specs=specs,
                tracker=Path(temporary) / "unused.json",
                progress_module=progress_cli,
                **reports(),
            )
        self.assertFalse(report["passed"])
        self.assertTrue(
            any("relocation-expectations" in item["message"] for item in report["failures"])
        )

    def test_explicit_canonical_root_authenticates_explicit_retail_path(self) -> None:
        payload = reports()
        payload.pop("final_verify_report")
        with tempfile.TemporaryDirectory() as temporary:
            canonical_root = Path(temporary)
            retail = canonical_root / "support" / "Recoil.exe"
            retail.parent.mkdir()
            retail.write_bytes(b"retail fixture")
            resolution = SimpleNamespace(
                canonical_control_root=canonical_root,
                resolution_source="explicit",
            )
            with (
                patch(
                    "_recoil.commands.pipeline_reachability_audit.resolve_canonical_control_root",
                    return_value=resolution,
                ),
                patch(
                    "_recoil.commands.pipeline_reachability_audit.reauthenticate_canonical_control_root"
                ) as reauthenticate,
                patch(
                    "_recoil.commands.pipeline_reachability_audit._probe_final_verify",
                    return_value=reports()["final_verify_report"],
                ),
            ):
                report = audit_pipeline_reachability(
                    specs=valid_specs(),
                    reference=retail,
                    canonical_root=canonical_root,
                    progress_module=progress_cli,
                    **payload,
                )

            self.assertTrue(report["passed"], report["failures"])
            self.assertEqual(str(canonical_root), report["canonical_control_root"])
            self.assertEqual(str(retail.resolve()), report["retail_reference_path"])
            self.assertTrue(report["retail_reference_from_canonical_control_root"])
            reauthenticate.assert_called_once_with(resolution)

    def test_explicit_canonical_root_rejects_different_explicit_retail(self) -> None:
        payload = reports()
        payload.pop("final_verify_report")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            canonical = root / "canonical"
            retail = canonical / "support" / "Recoil.exe"
            retail.parent.mkdir(parents=True)
            retail.write_bytes(b"canonical retail")
            other = root / "other.exe"
            other.write_bytes(b"different retail")
            resolution = SimpleNamespace(
                canonical_control_root=canonical,
                resolution_source="explicit",
            )
            with patch(
                "_recoil.commands.pipeline_reachability_audit.resolve_canonical_control_root",
                return_value=resolution,
            ):
                with self.assertRaisesRegex(Exception, "does not equal"):
                    audit_pipeline_reachability(
                        specs=valid_specs(),
                        reference=other,
                        canonical_root=canonical,
                        progress_module=progress_cli,
                        **payload,
                    )

    def test_explicit_canonical_root_rejects_different_explicit_tracker(self) -> None:
        payload = reports()
        payload.pop("call_contract_readiness_report")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            canonical = root / "canonical"
            tracker = canonical / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
            tracker.parent.mkdir(parents=True)
            tracker.write_bytes(b"canonical tracker")
            other = root / "other.sqlite3"
            other.write_bytes(b"fixture tracker")
            resolution = SimpleNamespace(
                canonical_control_root=canonical,
                resolution_source="explicit",
            )
            with patch(
                "_recoil.commands.pipeline_reachability_audit.resolve_canonical_control_root",
                return_value=resolution,
            ):
                with self.assertRaisesRegex(Exception, "does not equal"):
                    audit_pipeline_reachability(
                        specs=valid_specs(),
                        tracker=other,
                        canonical_root=canonical,
                        progress_module=progress_cli,
                        **payload,
                    )

    def test_live_canonical_routing_rejects_nonexecuting_manifest_root(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            other_manifests = Path(temporary) / "tools" / "vc5_verify_targets"
            other_manifests.mkdir(parents=True)
            with self.assertRaisesRegex(Exception, "executing worktree"):
                audit_pipeline_reachability(
                    specs=valid_specs(),
                    manifest_dir=other_manifests,
                    canonical_root=REPO_ROOT,
                    progress_module=progress_cli,
                    **reports(),
                )

    def test_retail_handle_and_canonical_root_reauthenticate_on_abort(self) -> None:
        payload = reports()
        payload.pop("final_verify_report")
        identity = Mock()
        identity.to_dict.return_value = {"fixture": "retail"}
        handle = Mock(identity=identity)
        canonical = canonical_input_root()
        resolution = SimpleNamespace(
            canonical_control_root=canonical,
            resolution_source="explicit",
        )
        with (
            patch(
                "_recoil.commands.pipeline_reachability_audit.resolve_canonical_control_root",
                return_value=resolution,
            ),
            patch(
                "_recoil.commands.pipeline_reachability_audit.StableReadHandle",
                return_value=handle,
            ),
            patch(
                "_recoil.commands.pipeline_reachability_audit._probe_final_verify",
                side_effect=KeyboardInterrupt,
            ),
            patch(
                "_recoil.commands.pipeline_reachability_audit.physical_identity",
                return_value=identity,
            ),
            patch(
                "_recoil.commands.pipeline_reachability_audit.require_same_physical_object"
            ) as require_same,
            patch(
                "_recoil.commands.pipeline_reachability_audit.reauthenticate_canonical_control_root"
            ) as reauthenticate,
        ):
            with self.assertRaises(KeyboardInterrupt):
                audit_pipeline_reachability(
                    specs=valid_specs(),
                    reference=canonical / "support" / "Recoil.exe",
                    canonical_root=canonical,
                    progress_module=progress_cli,
                    **payload,
                )
        reauthenticate.assert_called_once_with(resolution)
        require_same.assert_called_once()
        handle.close.assert_called_once_with()

    def test_cli_accepts_strict_and_json(self) -> None:
        args = build_parser().parse_args(["--strict", "--json"])
        self.assertTrue(args.strict)
        self.assertTrue(args.json)


if __name__ == "__main__":
    unittest.main()
