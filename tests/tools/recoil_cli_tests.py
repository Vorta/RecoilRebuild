from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.lib.progress import empty_progress_document  # noqa: E402


def canonical_retail_reference() -> Path:
    return REPO_ROOT / "support" / "Recoil.exe"


class RecoilCliTests(unittest.TestCase):
    """Compact public-CLI contract plus shared logical-alias fixtures."""

    def run_cli(self, *args: str) -> tuple[int, str, str]:
        completed = subprocess.run(
            [sys.executable, "tools/recoil.py", *args],
            cwd=REPO_ROOT,
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )
        return completed.returncode, completed.stdout, completed.stderr

    @staticmethod
    def logical_alias_group_document(*, revision: int = 7) -> dict[str, object]:
        data = empty_progress_document()
        data["revision"] = revision
        block_id = "recoil:block:0x401000"
        symbol_id = "recoil:function:0x401000"
        data["physical_blocks"][block_id] = {
            "binary": "recoil",
            "start": "0x401000",
            "end_exclusive": "0x401010",
            "source_path": "sample.cpp",
            "agent_source_path": "sample.cpp",
            "contribution_ids": [symbol_id],
            "semantic_span_ids": [],
            "order": {},
        }
        data["symbols"][symbol_id] = {
            "binary": "recoil",
            "kind": "function",
            "address": "0x401000",
            "end_exclusive": "0x401010",
            "navigation_name": "FoldedBody",
            "pipeline_class": "non-authored",
            "authored_order_role": "compiler-generated-icf-representative",
            "physical_block_id": block_id,
            "semantic_span_ids": [],
            "unrelated": {"preserved": True},
        }
        for evidence_id, summary in (
            ("recoil:evidence:r7:000001", "Retail fold group."),
            ("recoil:evidence:r7:000002", "Winner identity."),
            ("recoil:evidence:r7:000003", "Fold alias identity."),
        ):
            data["evidence"][evidence_id] = {
                "artifacts": [],
                "disposition": "observed",
                "freshness": "historical",
                "gating": False,
                "kind": "legacy-owner",
                "migrated_at_revision": 7,
                "provenance": {},
                "result": "pending",
                "scope_ids": [],
                "summary": summary,
                "validation_mode": "historical-observation",
            }
        for owner_id in ("recoil:owner:fixture.download", "recoil:owner:fixture.api"):
            data["owners"][owner_id] = {
                "binary": "recoil",
                "kind": "class",
                "provider_state": "pending",
                "gates": {"source": "accepted", "owner_linkage": "accepted"},
            }
        return data

    @staticmethod
    def logical_alias_group_payload(**overrides: object) -> dict[str, object]:
        winner_key = "recoil:logical-function:0x401000:download-add-ref"
        alias_key = "recoil:logical-function:0x401000:api-add-ref"
        payload: dict[str, object] = {
            "schema": "recoil-logical-alias-group-v1",
            "reviewed": True,
            "reason": "Retail vtables prove one folded AddRef address group.",
            "symbol_id": "recoil:function:0x401000",
            "address": "0x401000",
            "current": {
                "pipeline_class": "non-authored",
                "authored_order_role": "compiler-generated-icf-representative",
                "physical_block_id": "recoil:block:0x401000",
                "linked_address_group": None,
                "icf_address_group": None,
                "logical_aliases": None,
            },
            "icf_address_group": {
                "winner_status": "selected-winner",
                "winner_identity_key": winner_key,
                "evidence_ids": ["recoil:evidence:r7:000001"],
            },
            "logical_aliases": {
                winner_key: {
                    "object_symbol": "?AddRef@DownloadSink@@UAGKXZ",
                    "original_name": "DownloadSink::AddRef",
                    "original_name_status": "recovered",
                    "source_owner_status": "authored-owner",
                    "owner_id": "recoil:owner:fixture.download",
                    "pipeline_class": "authored",
                    "authored_order_role": "authored-body",
                    "fold_status": "selected-winner",
                    "evidence_ids": ["recoil:evidence:r7:000002"],
                },
                alias_key: {
                    "object_symbol": "?AddRef@ApiSink@@UAGKXZ",
                    "original_name": "ApiSink::AddRef",
                    "original_name_status": "recovered",
                    "source_owner_status": "authored-owner",
                    "owner_id": "recoil:owner:fixture.api",
                    "pipeline_class": "authored",
                    "authored_order_role": "authored-body",
                    "fold_status": "proven-fold-alias",
                    "evidence_ids": ["recoil:evidence:r7:000003"],
                },
            },
        }
        payload.update(overrides)
        return payload

    @staticmethod
    def logical_alias_group_v2_payload(**overrides: object) -> dict[str, object]:
        payload = deepcopy(RecoilCliTests.logical_alias_group_payload())
        payload["schema"] = "recoil-logical-alias-group-v2"
        payload["icf_address_group"].pop("evidence_ids")
        for alias in payload["logical_aliases"].values():
            alias.pop("evidence_ids")
        payload["new_evidence"] = {
            "summary": "Immutable retail facts prove the authored ICF aliases.",
            "provenance": {
                "candidate_independent": True,
                "reference": "support/Recoil.exe",
                "producer": "direct-retail-and-binary-ninja-review",
            },
            "artifacts": [
                {
                    "path": "support/Recoil.exe",
                    "size": canonical_retail_reference().stat().st_size,
                }
            ],
            "validation_context": {
                "candidate_output_used": False,
                "review_method": "immutable-retail-and-saved-analysis-review",
            },
        }
        payload.update(overrides)
        return payload

    @staticmethod
    def logical_alias_group_v3_document(*, revision: int = 7) -> dict[str, object]:
        data = RecoilCliTests.logical_alias_group_document(revision=revision)
        symbol_id = "recoil:function:0x401000"
        base = RecoilCliTests.logical_alias_group_payload()
        group = deepcopy(base["icf_address_group"])
        group["winner_status"] = "winner-unknown"
        group["winner_identity_key"] = None
        aliases = deepcopy(base["logical_aliases"])
        for alias in aliases.values():
            alias["fold_status"] = "proven-fold-alias"
        aliases["recoil:logical-function:0x401000:api-add-ref"][
            "original_name_status"
        ] = "provisional"
        row = data["symbols"][symbol_id]
        row["icf_address_group"] = group
        row["logical_aliases"] = aliases
        row["accepted_order_facts"] = {"stale": True}
        rows = [
            {
                "address": "0x401000",
                "authored_order_gate": True,
                "authored_order_role": alias["authored_order_role"],
                "authored_relative_order_gate": False,
                "full_order_gate": False,
                "icf_fold_status": "proven-fold-alias",
                "logical_identity_key": alias_id,
                "name": alias["original_name"],
                "pipeline_class": alias["pipeline_class"],
                "required_presence": True,
                "symbol": alias["object_symbol"],
                "symbol_regex": None,
            }
            for alias_id, alias in aliases.items()
        ]
        data["verification_targets"]["recoil:vc5-target:fixture-logical-alias-v3"] = {
            "binary": "recoil",
            "kind": "vc5",
            "name": "fixture_logical_alias_v3",
            "registered_addresses": ["0x401000"],
            "registration": {
                "binary": "recoil",
                "manifest_path": "tools/vc5_verify_targets/cabout_prelude_provider_order_current_shape.json",
                "name": "fixture_logical_alias_v3",
                "translation_unit_function_order": [{"functions": rows}],
            },
        }
        return data

    @staticmethod
    def logical_alias_group_v3_payload(
        document: dict[str, object] | None = None,
        **overrides: object,
    ) -> dict[str, object]:
        data = document or RecoilCliTests.logical_alias_group_v3_document()
        row = data["symbols"]["recoil:function:0x401000"]
        payload: dict[str, object] = {
            "schema": "recoil-logical-alias-group-v3",
            "reviewed": True,
            "reason": "Refresh candidate-independent physical-group evidence.",
            "symbol_id": "recoil:function:0x401000",
            "address": "0x401000",
            "current": {
                "pipeline_class": row["pipeline_class"],
                "authored_order_role": row["authored_order_role"],
                "physical_block_id": row["physical_block_id"],
                "linked_address_group": deepcopy(row.get("linked_address_group")),
                "icf_address_group": deepcopy(row["icf_address_group"]),
                "logical_aliases": deepcopy(row["logical_aliases"]),
            },
            "new_evidence": deepcopy(
                RecoilCliTests.logical_alias_group_v2_payload()["new_evidence"]
            ),
        }
        payload.update(overrides)
        return payload

    def test_progress_surface_is_single_task_and_direct_acceptance(self) -> None:
        rc, stdout, stderr = self.run_cli("help", "progress")
        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("next", stdout)
        self.assertIn("advance-live-order", stdout)
        self.assertIn("advance-live-call-contract", stdout)
        self.assertIn("advance-live-authored-byte", stdout)
        self.assertIn("advance-live-linked-byte", stdout)
        for retired in ("claim-current", "handoff", " work ", "lease"):
            self.assertNotIn(retired, stdout)

    def test_live_next_exposes_only_the_serial_contract(self) -> None:
        rc, stdout, stderr = self.run_cli("progress", "next", "--json")
        self.assertEqual(0, rc, stderr)
        payload = json.loads(stdout)
        self.assertEqual("recoil-current-task-v2", payload["schema"])
        self.assertEqual(
            {
                "schema",
                "binary",
                "stage",
                "task_id",
                "state",
                "cursor",
                "scope",
                "objective",
                "check_command",
                "acceptance_command",
                "blocker",
                "revision_vector",
            },
            set(payload),
        )
        serialized = json.dumps(payload).lower()
        self.assertNotIn("packet", serialized)
        self.assertNotIn("worker", serialized)
        self.assertNotIn("lane", serialized)


if __name__ == "__main__":
    unittest.main()
