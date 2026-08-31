from __future__ import annotations

from copy import deepcopy
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from contextlib import redirect_stdout


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.source_trace_progress import (  # noqa: E402
    SourceTraceProgressError,
    iter_tracker_artifacts,
    load_replace_batch_payload,
    main,
    mutate_source_traceability_batch,
    normalize_replace_batch_payload,
    normalize_source_traceability,
    plan_source_traceability_batch,
    resolve_tracker_artifact,
    show_source_traceability,
)
from _recoil.lib.live_progress import ConcurrentRevisionUpdate  # noqa: E402
from _recoil.lib.progress import empty_progress_document  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402


EVIDENCE_ID = "recoil:evidence:r7:000001"


def tracker(revision: int = 7) -> dict[str, object]:
    value = empty_progress_document()
    value["revision"] = revision
    value["physical_blocks"] = {
        "recoil:block:0x401000": {"source_traceability": "must-stay-distinct"}
    }
    value["symbols"] = {
        "recoil:function:0x401000": {
            "kind": "function",
            "output_section_id": "recoil:section:.text",
            "logical_aliases": {
                "recoil:logical-function:0x401000:sample-run": {
                    "kind": "function"
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
    }
    value["evidence"] = {
        EVIDENCE_ID: {"freshness": "historical", "validation_mode": "imported"}
    }
    return value


def resolved_state(anchor: str = "recoil:anchor:sample-run") -> dict[str, object]:
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


def payload(
    artifact_id: str = "recoil:function:0x401000",
    expected_current: object = None,
) -> dict[str, object]:
    return {
        "operation": "replace-batch",
        "reviewed": True,
        "updates": [
            {
                "artifact_id": artifact_id,
                "expected_current": expected_current,
                "source_traceability": resolved_state(),
            }
        ],
    }


class RecoilSourceTraceProgressTests(unittest.TestCase):
    def test_resolver_indexes_only_symbols_and_nested_aliases(self) -> None:
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
        self.assertEqual(".text", alias.output_section)

    def test_resolver_fails_closed_on_missing_and_ambiguous_aliases(self) -> None:
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

    def test_state_and_replace_payload_schemas_are_exact(self) -> None:
        self.assertEqual(resolved_state(), normalize_source_traceability(resolved_state()))
        unresolved = {
            "state": "unresolved",
            "source_edges": [],
            "reason_code": "pending-reviewed-source-edge",
        }
        self.assertEqual(unresolved, normalize_source_traceability(unresolved))
        extra = payload()
        extra["legacy_claim_resolutions"] = []
        with self.assertRaisesRegex(SourceTraceProgressError, "keys must be exactly"):
            normalize_replace_batch_payload(extra)
        empty = payload()
        empty["updates"] = []
        with self.assertRaisesRegex(SourceTraceProgressError, "at least one"):
            normalize_replace_batch_payload(empty)

    def test_plan_is_topology_only_and_preserves_completed_migration_metadata(self) -> None:
        value = tracker()
        value["migration"]["source_traceability_v1"] = {
            "state": "completed",
            "historical": True,
        }
        original = deepcopy(value)
        plan = plan_source_traceability_batch(value, payload(), expected_revision=7)
        self.assertEqual(original, value)
        self.assertEqual(("recoil:function:0x401000",), plan.artifact_ids)
        self.assertEqual(original["migration"], plan.proposed["migration"])
        self.assertEqual(
            "must-stay-distinct",
            plan.proposed["physical_blocks"]["recoil:block:0x401000"][
                "source_traceability"
            ],
        )

    def test_plan_requires_exact_current_and_known_evidence(self) -> None:
        stale = payload(
            expected_current={
                "state": "unresolved",
                "source_edges": [],
                "reason_code": "missing-anchor",
            }
        )
        with self.assertRaisesRegex(SourceTraceProgressError, "stale"):
            plan_source_traceability_batch(tracker(), stale, expected_revision=7)
        unknown = payload()
        unknown["updates"][0]["source_traceability"]["source_edges"][0][
            "evidence_ids"
        ] = ["recoil:evidence:r7:999999"]
        with self.assertRaisesRegex(SourceTraceProgressError, "unknown evidence"):
            plan_source_traceability_batch(tracker(), unknown, expected_revision=7)

    def test_dry_run_apply_and_revision_cas(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                path,
                tracker(),
                cutover_pair_id="source-trace-progress-test",
            )
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
                    path, payload(), expected_revision=7, apply=False
                )

    def test_payload_file_and_json_are_mutually_exclusive(self) -> None:
        body = json.dumps(payload())
        self.assertEqual(payload(), load_replace_batch_payload(payload_json=body))
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "payload.json"
            path.write_text(body, encoding="utf-8")
            self.assertEqual(payload(), load_replace_batch_payload(payload_file=path))
            with self.assertRaisesRegex(SourceTraceProgressError, "exactly one"):
                load_replace_batch_payload(payload_json=body, payload_file=path)

    def test_show_and_cli_are_read_only(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                path,
                tracker(),
                cutover_pair_id="source-trace-progress-test",
            )
            before = path.read_bytes()
            shown = show_source_traceability(
                path, artifact_ids=["messages:function:0x10001000"]
            )
            self.assertTrue(shown["read_only"])
            self.assertEqual(1, shown["artifact_count"])

            stdout = io.StringIO()
            with redirect_stdout(stdout):
                result = main(
                    [
                        "show",
                        "--artifact-id",
                        "recoil:function:0x401000",
                        "--progress",
                        str(path),
                        "--json",
                    ]
                )
            self.assertEqual(0, result)
            self.assertTrue(json.loads(stdout.getvalue())["read_only"])
            self.assertEqual(before, path.read_bytes())


if __name__ == "__main__":
    unittest.main()
