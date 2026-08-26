import json
import io
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from copy import deepcopy
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.source_trace_progress import (  # noqa: E402
    SourceTraceProgressError,
    iter_tracker_artifacts,
    load_replace_batch_payload,
    main,
    mutate_source_traceability_batch,
    normalize_legacy_claim_resolutions,
    normalize_source_traceability,
    plan_source_traceability_batch,
    resolve_tracker_artifact,
    show_source_traceability,
)
from _recoil.lib.live_progress import ConcurrentRevisionUpdate  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402


EVIDENCE_ID = "recoil:evidence:r7:000001"


def tracker(revision=7):
    return {
        "schema_version": 5,
        "revision": revision,
        "id_sequences": {},
        "migration": {},
        "binaries": {},
        "physical_blocks": {
            "recoil:block:0x401000": {"source_traceability": "must-stay-distinct"}
        },
        "semantic_spans": {},
        "symbols": {
            "recoil:function:0x401000": {
                "kind": "function",
                "output_section_id": "recoil:section:.text",
                "logical_aliases": {
                    "recoil:logical-function:0x401000:sample-run": {
                        "kind": "function",
                    }
                },
            },
            "recoil:data:0x4e0000": {
                "kind": "data",
                "output_section_id": "recoil:section:.data",
            },
            "messages:function:0x10001000": {
                "kind": "function",
                "output_section_id": "messages:section:.text",
            },
        },
        "output_sections": {},
        "storage_contributions": {},
        "owners": {
            "recoil:owner:sample": {
                "gates": {"source": "accepted"},
                "reimplementation": {"entries": {}},
            }
        },
        "verification_targets": {},
        "work_items": {},
        "blockers": {},
        "evidence": {
            EVIDENCE_ID: {
                "freshness": "historical",
                "validation_mode": "imported",
            },
        },
        "tombstones": {},
    }


def resolved_state(anchor="recoil:anchor:sample-run"):
    return {
        "state": "resolved",
        "source_edges": [
            {
                "relation": "defines",
                "anchor_id": anchor,
                "emission_context": {"translation_unit": "src/sample.cpp"},
                "evidence_ids": [EVIDENCE_ID],
            }
        ],
        "reason_code": None,
    }


def payload(artifact_id="recoil:function:0x401000", expected_current=None):
    return {
        "operation": "replace-batch",
        "parent_reviewed": True,
        "updates": [
            {
                "artifact_id": artifact_id,
                "expected_current": expected_current,
                "source_traceability": resolved_state(),
            }
        ],
    }


def legacy_claim(address="0x401000", kind_hint="function"):
    return {
        "binary": "recoil",
        "kind_hint": kind_hint,
        "address": address,
        "reason_code": "missing-artifact-identity",
        "source_path": "src/sample.cpp",
    }


def initialized_tracker(*claims):
    value = tracker()
    value["migration"]["source_traceability_v1"] = {
        "version": 1,
        "policy": "topology-only-no-acceptance",
        "initialized_from_revision": 6,
        "state": "initialized",
        "unresolved_legacy_claims": list(claims),
    }
    return value


def resolution_only_payload(*records):
    return {
        "operation": "replace-batch",
        "parent_reviewed": True,
        "updates": [],
        "legacy_claim_resolutions": list(records),
    }


def exact_resolution(claim, artifact_id="recoil:function:0x401000"):
    return {
        "expected_claim": claim,
        "replacement_resolution": {
            "kind": "exact-existing-artifact",
            "artifact_id": artifact_id,
        },
    }


def interior_resolution(
    claim,
    artifact_id="recoil:function:0x401000",
    *,
    reason="Retail address is a compiler label inside the exact function extent.",
    evidence_ids=None,
):
    return {
        "expected_claim": claim,
        "replacement_resolution": {
            "kind": "interior-of-existing-artifact",
            "artifact_id": artifact_id,
            "reason": reason,
            "evidence_ids": [EVIDENCE_ID] if evidence_ids is None else evidence_ids,
        },
    }


class RecoilSourceTraceProgressTests(unittest.TestCase):
    def test_sqlite_cli_show_and_revision_guarded_replace(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                path,
                tracker(),
                cutover_pair_id="source-trace-progress-test",
            )
            before = ProgressSQLiteStore(path, read_only=True).materialize()

            stdout = io.StringIO()
            with redirect_stdout(stdout):
                return_code = main(
                    [
                        "show",
                        "--artifact-id",
                        "recoil:function:0x401000",
                        "--progress",
                        str(path),
                        "--json",
                    ]
                )
            self.assertEqual(0, return_code)
            shown = json.loads(stdout.getvalue())
            self.assertTrue(shown["read_only"])
            self.assertEqual(7, shown["revision"])
            self.assertEqual(
                ["recoil:function:0x401000"],
                [row["artifact_id"] for row in shown["artifacts"]],
            )
            self.assertEqual(
                before,
                ProgressSQLiteStore(path, read_only=True).materialize(),
            )

            dry = mutate_source_traceability_batch(
                path,
                payload(),
                expected_revision=7,
                apply=False,
            )
            self.assertFalse(dry["applied"])
            self.assertEqual(
                before,
                ProgressSQLiteStore(path, read_only=True).materialize(),
            )

            applied = mutate_source_traceability_batch(
                path,
                payload(),
                expected_revision=7,
                apply=True,
            )
            self.assertTrue(applied["applied"])
            current = ProgressSQLiteStore(path, read_only=True).materialize()
            self.assertEqual(8, current["revision"])
            self.assertEqual(
                resolved_state(),
                current["symbols"]["recoil:function:0x401000"][
                    "source_traceability"
                ],
            )
            with self.assertRaises(ConcurrentRevisionUpdate):
                mutate_source_traceability_batch(
                    path,
                    payload(),
                    expected_revision=7,
                    apply=False,
                )

    def test_resolver_indexes_only_symbols_and_nested_aliases(self):
        rows = iter_tracker_artifacts(tracker())
        self.assertEqual(
            [
                "messages:function:0x10001000",
                "recoil:data:0x4e0000",
                "recoil:function:0x401000",
                "recoil:logical-function:0x401000:sample-run",
            ],
            [row.artifact_id for row in rows],
        )
        alias = resolve_tracker_artifact(
            tracker(), "recoil:logical-function:0x401000:sample-run"
        )
        self.assertEqual("recoil:function:0x401000", alias.parent_artifact_id)
        self.assertEqual("recoil:section:.text", alias.output_section_id)
        self.assertEqual(".text", alias.output_section)
        messages = resolve_tracker_artifact(
            tracker(), "messages:function:0x10001000"
        )
        self.assertEqual("messages:section:.text", messages.output_section_id)
        self.assertEqual(".text", messages.output_section)
        messages_payload = payload("messages:function:0x10001000")
        messages_payload["updates"][0]["source_traceability"]["source_edges"][0][
            "evidence_ids"
        ] = []
        plan = plan_source_traceability_batch(
            tracker(), messages_payload, expected_revision=7
        )
        self.assertEqual(
            ("messages:function:0x10001000",),
            plan.artifact_ids,
        )
        with self.assertRaisesRegex(SourceTraceProgressError, "unsupported"):
            resolve_tracker_artifact(tracker(), "recoil:block:0x401000")

    def test_resolver_fails_closed_on_missing_and_ambiguous_aliases(self):
        with self.assertRaisesRegex(SourceTraceProgressError, "found 0"):
            resolve_tracker_artifact(tracker(), "recoil:function:0x430230")
        value = tracker()
        value["symbols"]["recoil:function:0x401010"] = {
            "output_section_id": "recoil:section:.text",
            "logical_aliases": {
                "recoil:logical-function:0x401000:sample-run": {}
            },
        }
        with self.assertRaisesRegex(SourceTraceProgressError, "ambiguous"):
            iter_tracker_artifacts(value)

    def test_replace_batch_resolves_nested_logical_data_alias_by_exact_id(self):
        value = tracker()
        logical_id = "recoil:logical-data:0x4e0000:pooled-occurrence"
        value["symbols"]["recoil:data:0x4e0000"]["logical_aliases"] = {
            logical_id: {
                "kind": "data",
                "disposition": "authored",
                "object_symbol": "??_C@_00A@?$AA@",
                "source_traceability": {
                    "state": "unresolved",
                    "source_edges": [],
                    "reason_code": "pending-reviewed-source-edge",
                },
            }
        }
        update = payload(
            logical_id,
            expected_current={
                "state": "unresolved",
                "source_edges": [],
                "reason_code": "pending-reviewed-source-edge",
            },
        )
        update["updates"][0]["source_traceability"] = resolved_state(
            "recoil:anchor:pooled-occurrence"
        )

        plan = plan_source_traceability_batch(
            value,
            update,
            expected_revision=7,
        )

        self.assertEqual((logical_id,), plan.artifact_ids)
        self.assertEqual(
            "resolved",
            plan.proposed["symbols"]["recoil:data:0x4e0000"][
                "logical_aliases"
            ][logical_id]["source_traceability"]["state"],
        )

    def test_exact_state_schema_and_state_edge_rules(self):
        self.assertEqual(resolved_state(), normalize_source_traceability(resolved_state()))
        for state in ("unresolved", "not-applicable"):
            row = {
                "state": state,
                "source_edges": [],
                "reason_code": "reviewed-no-source-edge",
            }
            self.assertEqual(row, normalize_source_traceability(row))
        bad = resolved_state()
        bad["defines"] = []
        with self.assertRaisesRegex(SourceTraceProgressError, "keys must be exactly"):
            normalize_source_traceability(bad)
        bad = resolved_state()
        bad["source_edges"][0]["relation"] = "Defines"
        with self.assertRaisesRegex(SourceTraceProgressError, "lowercase"):
            normalize_source_traceability(bad)
        bad = resolved_state()
        bad["source_edges"] = []
        with self.assertRaisesRegex(SourceTraceProgressError, "at least one"):
            normalize_source_traceability(bad)
        bad = resolved_state()
        bad["reason_code"] = "should-be-null"
        with self.assertRaisesRegex(SourceTraceProgressError, "must be null"):
            normalize_source_traceability(bad)

    def test_plan_checks_exact_current_and_initializes_metadata_once(self):
        value = tracker()
        original = deepcopy(value)
        plan = plan_source_traceability_batch(value, payload(), expected_revision=7)
        self.assertEqual(original, value)
        self.assertEqual(("recoil:function:0x401000",), plan.artifact_ids)
        self.assertTrue(plan.initializes_migration)
        self.assertEqual(
            {
                "version": 1,
                "policy": "topology-only-no-acceptance",
                "initialized_from_revision": 7,
                "state": "initialized",
            },
            plan.migration_metadata,
        )
        self.assertEqual(5, plan.proposed["schema_version"])
        self.assertEqual(
            "must-stay-distinct",
            plan.proposed["physical_blocks"]["recoil:block:0x401000"][
                "source_traceability"
            ],
        )
        with self.assertRaisesRegex(SourceTraceProgressError, "stale"):
            plan_source_traceability_batch(
                value,
                payload(expected_current={
                    "state": "unresolved",
                    "source_edges": [],
                    "reason_code": "missing-anchor",
                }),
                expected_revision=7,
            )

    def test_dry_run_apply_and_revision_cas(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "progress.json"
            path.write_text(json.dumps(tracker()), encoding="utf-8")
            before = path.read_bytes()
            dry = mutate_source_traceability_batch(
                path, payload(), expected_revision=7, apply=False
            )
            self.assertFalse(dry["applied"])
            self.assertFalse(dry["acceptance_changed"])
            self.assertEqual(before, path.read_bytes())

            applied = mutate_source_traceability_batch(
                path, payload(), expected_revision=7, apply=True
            )
            self.assertTrue(applied["applied"])
            current = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(8, current["revision"])
            self.assertEqual(
                resolved_state(),
                current["symbols"]["recoil:function:0x401000"][
                    "source_traceability"
                ],
            )
            self.assertEqual(
                7,
                current["migration"]["source_traceability_v1"][
                    "initialized_from_revision"
                ],
            )
            with self.assertRaises(ConcurrentRevisionUpdate):
                mutate_source_traceability_batch(
                    path, payload(), expected_revision=7, apply=False
                )

            second_payload = payload(
                "recoil:logical-function:0x401000:sample-run"
            )
            second_payload["updates"][0]["source_traceability"] = resolved_state(
                "recoil:anchor:sample-alias"
            )
            mutate_source_traceability_batch(
                path, second_payload, expected_revision=8, apply=True
            )
            final = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(
                7,
                final["migration"]["source_traceability_v1"][
                    "initialized_from_revision"
                ],
            )

    def test_parent_review_evidence_and_missing_artifact_fail_closed(self):
        unreviewed = payload()
        unreviewed["parent_reviewed"] = False
        with self.assertRaisesRegex(SourceTraceProgressError, "parent_reviewed"):
            plan_source_traceability_batch(
                tracker(), unreviewed, expected_revision=7
            )
        missing = payload("recoil:function:0x430230")
        with self.assertRaisesRegex(SourceTraceProgressError, "does not exist"):
            plan_source_traceability_batch(
                tracker(), missing, expected_revision=7
            )
        unknown_evidence = payload()
        unknown_evidence["updates"][0]["source_traceability"]["source_edges"][0][
            "evidence_ids"
        ] = ["recoil:evidence:r7:999999"]
        with self.assertRaisesRegex(SourceTraceProgressError, "unknown evidence"):
            plan_source_traceability_batch(
                tracker(), unknown_evidence, expected_revision=7
            )

    def test_payload_json_file_and_mutual_exclusion(self):
        body = json.dumps(payload())
        self.assertEqual(
            payload(), load_replace_batch_payload(payload_json=body)
        )
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "payload.json"
            path.write_text(body, encoding="utf-8")
            self.assertEqual(
                payload(), load_replace_batch_payload(payload_file=path)
            )
            with self.assertRaisesRegex(SourceTraceProgressError, "exactly one"):
                load_replace_batch_payload(
                    payload_json=body, payload_file=path
                )
        with self.assertRaisesRegex(SourceTraceProgressError, "exactly one"):
            load_replace_batch_payload()

    def test_five_missing_identity_claims_initialize_and_then_compare_exactly(self):
        debts = [
            {
                "binary": "recoil",
                "kind_hint": kind,
                "address": address,
                "reason_code": "missing-artifact-identity",
                "source_path": source_path,
            }
            for kind, address, source_path in (
                ("function", "0x430230", "src/Battlesport/hud.cpp"),
                ("function", "0x4306d0", "src/Battlesport/hud.cpp"),
                ("data", "0x4e5ce0", "src/Battlesport/hud.cpp"),
                ("data", "0x4e1378", "src/GameZRecoil/RecoilApp.cpp"),
                ("data", "0x4e1380", "src/GameZRecoil/RecoilApp.cpp"),
            )
        ]
        first_payload = payload()
        first_payload["unresolved_legacy_claims"] = list(reversed(debts))
        plan = plan_source_traceability_batch(
            tracker(), first_payload, expected_revision=7
        )
        metadata = plan.proposed["migration"]["source_traceability_v1"]
        self.assertEqual(5, len(metadata["unresolved_legacy_claims"]))
        self.assertEqual(
            ["0x430230", "0x4306d0", "0x4e1378", "0x4e1380", "0x4e5ce0"],
            [row["address"] for row in metadata["unresolved_legacy_claims"]],
        )
        self.assertTrue(
            all(
                set(row)
                == {
                    "binary",
                    "kind_hint",
                    "address",
                    "reason_code",
                    "source_path",
                }
                for row in metadata["unresolved_legacy_claims"]
            )
        )

        established = deepcopy(plan.proposed)
        established["revision"] = 8
        second = payload("recoil:data:0x4e0000")
        second["unresolved_legacy_claims"] = deepcopy(
            metadata["unresolved_legacy_claims"]
        )
        next_plan = plan_source_traceability_batch(
            established, second, expected_revision=8
        )
        self.assertFalse(next_plan.initializes_migration)
        self.assertEqual(
            metadata,
            next_plan.proposed["migration"]["source_traceability_v1"],
        )
        second["unresolved_legacy_claims"][0]["source_path"] = "src/wrong.cpp"
        with self.assertRaisesRegex(
            SourceTraceProgressError, "differ from the initialized"
        ):
            plan_source_traceability_batch(
                established, second, expected_revision=8
            )

    def test_exact_legacy_claim_resolution_preserves_immutable_inventory(self):
        claim = legacy_claim()
        value = initialized_tracker(claim)
        original = deepcopy(value)
        request = resolution_only_payload(exact_resolution(claim))
        plan = plan_source_traceability_batch(
            value, request, expected_revision=7
        )

        self.assertEqual(original, value)
        self.assertEqual((), plan.artifact_ids)
        self.assertEqual(1, plan.legacy_claim_resolution_count)
        metadata = plan.proposed["migration"]["source_traceability_v1"]
        self.assertEqual([claim], metadata["unresolved_legacy_claims"])
        self.assertEqual(
            request["legacy_claim_resolutions"],
            metadata["legacy_claim_resolutions"],
        )
        self.assertNotIn(
            "source_traceability",
            plan.proposed["symbols"]["recoil:function:0x401000"],
        )
        self.assertEqual(
            set(value["symbols"]), set(plan.proposed["symbols"])
        )

    def test_resolution_only_dry_run_apply_cas_and_append_only_history(self):
        first_claim = legacy_claim()
        second_claim = legacy_claim("0x4e0000", "data")
        value = initialized_tracker(first_claim, second_claim)
        first_record = exact_resolution(first_claim)
        second_record = exact_resolution(
            second_claim, "recoil:data:0x4e0000"
        )
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "progress.json"
            path.write_text(json.dumps(value), encoding="utf-8")
            before = path.read_bytes()
            dry = mutate_source_traceability_batch(
                path,
                resolution_only_payload(first_record),
                expected_revision=7,
                apply=False,
            )
            self.assertFalse(dry["applied"])
            self.assertFalse(dry["acceptance_changed"])
            self.assertTrue(dry["topology_only"])
            self.assertEqual(1, dry["legacy_claim_resolution_count"])
            self.assertEqual(before, path.read_bytes())

            mutate_source_traceability_batch(
                path,
                resolution_only_payload(first_record),
                expected_revision=7,
                apply=True,
            )
            after_first = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(
                [first_record],
                after_first["migration"]["source_traceability_v1"][
                    "legacy_claim_resolutions"
                ],
            )
            self.assertEqual(
                [first_claim, second_claim],
                after_first["migration"]["source_traceability_v1"][
                    "unresolved_legacy_claims"
                ],
            )
            mutate_source_traceability_batch(
                path,
                resolution_only_payload(second_record),
                expected_revision=8,
                apply=True,
            )
            after_second = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(
                [first_record, second_record],
                after_second["migration"]["source_traceability_v1"][
                    "legacy_claim_resolutions"
                ],
            )
            with self.assertRaises(ConcurrentRevisionUpdate):
                mutate_source_traceability_batch(
                    path,
                    resolution_only_payload(second_record),
                    expected_revision=8,
                    apply=False,
                )

    def test_interior_resolution_requires_proven_exact_extent_and_evidence(self):
        claim = legacy_claim("0x401008")
        value = initialized_tracker(claim)
        function = value["symbols"]["recoil:function:0x401000"]
        function.update(
            {
                "address": "0x401000",
                "extent_state": "known",
                "end_exclusive": "0x401010",
                "size": 16,
            }
        )
        record = interior_resolution(claim)
        plan = plan_source_traceability_batch(
            value,
            resolution_only_payload(record),
            expected_revision=7,
        )
        self.assertEqual(
            [record],
            plan.proposed["migration"]["source_traceability_v1"][
                "legacy_claim_resolutions"
            ],
        )

        for field, replacement, message in (
            ("reason", "", "non-empty trimmed"),
            ("evidence_ids", [], "non-empty array"),
            (
                "evidence_ids",
                ["recoil:evidence:r7:999999"],
                "unknown evidence",
            ),
        ):
            invalid = interior_resolution(claim)
            invalid["replacement_resolution"][field] = replacement
            with self.assertRaisesRegex(SourceTraceProgressError, message):
                plan_source_traceability_batch(
                    value,
                    resolution_only_payload(invalid),
                    expected_revision=7,
                )

        unknown_extent = deepcopy(value)
        unknown_extent["symbols"]["recoil:function:0x401000"][
            "extent_state"
        ] = "unknown"
        with self.assertRaisesRegex(SourceTraceProgressError, "known exact extent"):
            plan_source_traceability_batch(
                unknown_extent,
                resolution_only_payload(record),
                expected_revision=7,
            )

        inconsistent = deepcopy(value)
        inconsistent["symbols"]["recoil:function:0x401000"]["size"] = 15
        with self.assertRaisesRegex(SourceTraceProgressError, "inconsistent"):
            plan_source_traceability_batch(
                inconsistent,
                resolution_only_payload(record),
                expected_revision=7,
            )

    def test_legacy_claim_resolution_rejects_stale_duplicate_or_wrong_targets(self):
        claim = legacy_claim()
        value = initialized_tracker(claim)
        exact = exact_resolution(claim)

        stale = exact_resolution(deepcopy(claim))
        stale["expected_claim"]["source_path"] = "src/stale.cpp"
        with self.assertRaisesRegex(SourceTraceProgressError, "stale or absent"):
            plan_source_traceability_batch(
                value,
                resolution_only_payload(stale),
                expected_revision=7,
            )

        missing = exact_resolution(
            claim, "recoil:function:0x430230"
        )
        with self.assertRaisesRegex(SourceTraceProgressError, "found 0"):
            plan_source_traceability_batch(
                value,
                resolution_only_payload(missing),
                expected_revision=7,
            )
        wrong_kind = exact_resolution(
            claim, "recoil:data:0x4e0000"
        )
        with self.assertRaisesRegex(SourceTraceProgressError, "binary/kind"):
            plan_source_traceability_batch(
                value,
                resolution_only_payload(wrong_kind),
                expected_revision=7,
            )
        wrong_address = exact_resolution(
            claim, "messages:function:0x10001000"
        )
        with self.assertRaisesRegex(SourceTraceProgressError, "binary/kind"):
            plan_source_traceability_batch(
                value,
                resolution_only_payload(wrong_address),
                expected_revision=7,
            )

        established = deepcopy(value)
        established["migration"]["source_traceability_v1"][
            "legacy_claim_resolutions"
        ] = [exact]
        with self.assertRaisesRegex(SourceTraceProgressError, "already has"):
            plan_source_traceability_batch(
                established,
                resolution_only_payload(exact),
                expected_revision=7,
            )

        with self.assertRaisesRegex(SourceTraceProgressError, "duplicates"):
            normalize_legacy_claim_resolutions([exact, exact])

    def test_interior_resolution_rejects_boundaries_and_logical_aliases(self):
        claim = legacy_claim("0x401008")
        value = initialized_tracker(claim)
        value["symbols"]["recoil:function:0x401000"].update(
            {
                "address": "0x401000",
                "extent_state": "known",
                "end_exclusive": "0x401010",
                "size": 16,
            }
        )
        alias_record = interior_resolution(
            claim, "recoil:logical-function:0x401000:sample-run"
        )
        with self.assertRaisesRegex(SourceTraceProgressError, "logical alias"):
            plan_source_traceability_batch(
                value,
                resolution_only_payload(alias_record),
                expected_revision=7,
            )

        for boundary in ("0x401000", "0x401010"):
            boundary_claim = legacy_claim(boundary)
            boundary_value = initialized_tracker(boundary_claim)
            boundary_value["symbols"]["recoil:function:0x401000"].update(
                {
                    "address": "0x401000",
                    "extent_state": "known",
                    "end_exclusive": "0x401010",
                    "size": 16,
                }
            )
            with self.assertRaisesRegex(SourceTraceProgressError, "not strictly interior"):
                plan_source_traceability_batch(
                    boundary_value,
                    resolution_only_payload(
                        interior_resolution(boundary_claim)
                    ),
                    expected_revision=7,
                )

    def test_legacy_claim_resolutions_require_initialized_inventory(self):
        claim = legacy_claim()
        with self.assertRaisesRegex(
            SourceTraceProgressError, "already initialized"
        ):
            plan_source_traceability_batch(
                tracker(),
                resolution_only_payload(exact_resolution(claim)),
                expected_revision=7,
            )
        value = tracker()
        value["migration"]["source_traceability_v1"] = {
            "version": 1,
            "policy": "topology-only-no-acceptance",
            "initialized_from_revision": 6,
            "state": "initialized",
        }
        with self.assertRaisesRegex(
            SourceTraceProgressError, "immutable unresolved"
        ):
            plan_source_traceability_batch(
                value,
                resolution_only_payload(exact_resolution(claim)),
                expected_revision=7,
            )

    def test_show_is_read_only_and_supports_messages(self):
        value = tracker()
        original = deepcopy(value)
        result = show_source_traceability(
            value,
            artifact_ids=[
                "messages:function:0x10001000",
                "recoil:logical-function:0x401000:sample-run",
            ],
        )
        self.assertEqual(original, value)
        self.assertTrue(result["read_only"])
        self.assertEqual(2, result["artifact_count"])
        self.assertEqual(
            [
                "messages:function:0x10001000",
                "recoil:logical-function:0x401000:sample-run",
            ],
            [row["artifact_id"] for row in result["artifacts"]],
        )
        self.assertTrue(
            all(row["source_traceability"] is None for row in result["artifacts"])
        )
        by_address = show_source_traceability(value, addresses=["0x00401000"])
        self.assertEqual(
            [
                "recoil:function:0x401000",
                "recoil:logical-function:0x401000:sample-run",
            ],
            [row["artifact_id"] for row in by_address["artifacts"]],
        )
        self.assertEqual(
            "recoil:function:0x401000",
            by_address["artifacts"][1]["parent_artifact_id"],
        )
        with self.assertRaisesRegex(SourceTraceProgressError, "not both"):
            show_source_traceability(
                value,
                artifact_ids=["recoil:function:0x401000"],
                addresses=["0x401000"],
            )


if __name__ == "__main__":
    unittest.main()
