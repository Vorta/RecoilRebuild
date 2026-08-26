import json
import sys
import tempfile
import unittest
from copy import deepcopy
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.data_extent_progress import (  # noqa: E402
    DataExtentProgressError,
    mutate_data_extent,
    normalize_register_payload,
    plan_data_extent_registration,
)
from _recoil.commands.data_artifact_progress import (  # noqa: E402
    DataArtifactProgressError,
    mutate_data_artifact,
    normalize_register_payload as normalize_artifact_payload,
    plan_data_artifact_registration,
)
from _recoil.commands.data_artifact_evidence_repair import (  # noqa: E402
    DataArtifactEvidenceRepairError,
    mutate_data_artifact_evidence_repair,
    normalize_repair_payload,
    plan_data_artifact_evidence_repair,
)
from _recoil.commands.data_logical_alias_progress import (  # noqa: E402
    DataLogicalAliasProgressError,
    mutate_logical_data_alias_batch,
    normalize_register_payload as normalize_alias_payload,
    plan_logical_data_alias_batch,
)


FIRST_EVIDENCE = "recoil:evidence:r7:000001"
SECOND_EVIDENCE = "recoil:evidence:r7:000002"
ARTIFACT_ID = "recoil:data:0x4e1348"


def tracker(revision=7):
    return {
        "schema_version": 5,
        "revision": revision,
        "id_sequences": {},
        "migration": {},
        "binaries": {},
        "physical_blocks": {},
        "semantic_spans": {},
        "symbols": {
            ARTIFACT_ID: {
                "address": "0x4e1348",
                "binary": "recoil",
                "kind": "data",
                "extent_state": "unknown",
                "output_section_id": "recoil:section:.data",
                "evidence_ids": [FIRST_EVIDENCE],
                "source_traceability": {
                    "state": "resolved",
                    "source_edges": [
                        {
                            "relation": "defines",
                            "anchor_id": "recoil:anchor:sample-data",
                            "emission_context": {
                                "translation_unit": "src/sample.cpp"
                            },
                            "evidence_ids": [],
                        }
                    ],
                    "reason_code": None,
                },
                "logical_aliases": {
                    "recoil:logical-data:0x4e1348:sample": {
                        "kind": "data",
                    }
                },
            }
        },
        "output_sections": {
            "recoil:section:.text": {
                "binary": "recoil",
                "reference": {
                    "image_address": "0x401000",
                    "virtual_size": 0xCB9E8 - 0x1000,
                },
            },
            "recoil:section:.data": {
                "binary": "recoil",
                "reference": {
                    "image_address": "0x4e1000",
                    "virtual_size": 0x1000,
                },
            }
        },
        "storage_contributions": {},
        "owners": {},
        "verification_targets": {
            "recoil:vc5-target:sample-order": {
                "kind": "vc5",
            }
        },
        "work_items": {},
        "blockers": {},
        "evidence": {
            FIRST_EVIDENCE: {
                "freshness": "historical",
                "validation_mode": "imported",
            },
            SECOND_EVIDENCE: {
                "freshness": "historical",
                "validation_mode": "reviewed",
            },
        },
        "tombstones": {},
    }


def payload():
    return {
        "operation": "register-existing-data-extent",
        "parent_reviewed": True,
        "artifact_id": ARTIFACT_ID,
        "expected_current": {
            "extent_state": "unknown",
            "address": "0x4e1348",
        },
        "replacement": {
            "extent_state": "known",
            "size": 0x3E,
            "end_exclusive": "0x4e1386",
            "evidence_ids": [FIRST_EVIDENCE, SECOND_EVIDENCE],
        },
    }


def artifact_payload():
    return {
        "operation": "register-exact-data-artifact",
        "parent_reviewed": True,
        "artifact_id": "recoil:data:0x4e1320",
        "artifact": {
            "address": "0x4e1320",
            "binary": "recoil",
            "navigation_name": "SampleGeneratedJumpTable",
            "disposition": "authored",
            "source_traceability_state": "unresolved",
            "source_traceability_reason_code": "pending-reviewed-source-edge",
            "output_section_id": "recoil:section:.data",
            "size": 0x20,
            "end_exclusive": "0x4e1340",
            "evidence_ids": [FIRST_EVIDENCE, SECOND_EVIDENCE],
            "verification_target_ids": [
                "recoil:vc5-target:sample-order"
            ],
        },
    }


def artifact_payload_with_new_evidence():
    reviewed = artifact_payload()
    reviewed["artifact_id"] = "recoil:data:0x41b898"
    reviewed["artifact"].update(
        {
            "address": "0x41b898",
            "navigation_name": "HelpDocsErrorJumpTable",
            "output_section_id": "recoil:section:.text",
            "size": 20,
            "end_exclusive": "0x41b8ac",
            "evidence_ids": [],
        }
    )
    reviewed["new_evidence"] = {
        "kind": "reviewed-data-artifact-observation",
        "method": "immutable-retail-bn-plus-vc5sp3-listing",
        "summary": (
            "Retail BN and current VC5 listing prove exact authored data "
            "artifact 0x41b898 with five uint32 jump-table entries."
        ),
        "scope_ids": ["recoil:data:0x41b898"],
        "observation": {
            "artifact_id": "recoil:data:0x41b898",
            "address": "0x41b898",
            "size": 20,
            "end_exclusive": "0x41b8ac",
            "output_section_id": "recoil:section:.text",
        },
        "command": (
            "python tools/recoil.py audit bn-data-evidence 0x41b898 "
            "--size 20 --nearby 32 --constants dword --binary recoil --json"
        ),
        "target_id": "recoil:vc5-target:sample-order",
        "artifacts": [],
    }
    return reviewed


def provider_tracker():
    current = tracker()
    current["symbols"]["recoil:data:0x4e1340"] = {
        "address": "0x4e1340",
        "binary": "recoil",
        "kind": "data",
        "disposition": "provider",
        "extent_state": "known",
        "size": 1,
        "end_exclusive": "0x4e1341",
        "output_section_id": "recoil:section:.data",
        "evidence_ids": [FIRST_EVIDENCE],
        "source_traceability": {
            "state": "not-applicable",
            "source_edges": [],
            "reason_code": "compiler-literal-pooling",
        },
    }
    return current


def alias_payload():
    return {
        "operation": "register-logical-data-alias-batch",
        "parent_reviewed": True,
        "physical_artifact_id": "recoil:data:0x4e1340",
        "expected_physical": {
            "disposition": "provider",
            "address": "0x4e1340",
            "extent_state": "known",
            "size": 1,
            "end_exclusive": "0x4e1341",
            "source_traceability_state": "not-applicable",
            "source_traceability_reason_code": "compiler-literal-pooling",
        },
        "pooling": {
            "mode": "compiler-literal-pooling",
            "reason": (
                "Reviewed VC5 object symbols and retail references prove "
                "distinct authored logical literals pooled to provider storage."
            ),
            "evidence_ids": [FIRST_EVIDENCE, SECOND_EVIDENCE],
        },
        "aliases": [
            {
                "artifact_id": "recoil:logical-data:0x4e1340:first",
                "disposition": "authored",
                "navigation_name": "FirstLogicalLiteral",
                "object_symbol": "??_C@_00A@first",
                "evidence_ids": [FIRST_EVIDENCE],
                "source_traceability_reason_code": "pending-reviewed-source-edge",
            },
            {
                "artifact_id": "recoil:logical-data:0x4e1340:second",
                "disposition": "authored",
                "navigation_name": "SecondLogicalLiteral",
                "object_symbol": "??_C@_00B@second",
                "evidence_ids": [SECOND_EVIDENCE],
                "source_traceability_reason_code": "pending-reviewed-source-edge",
            },
        ],
    }


class DataExtentProgressTests(unittest.TestCase):
    def test_plan_registers_only_exact_existing_data_extent(self):
        current = tracker()
        before = deepcopy(current)

        plan = plan_data_extent_registration(
            current,
            payload(),
            expected_revision=7,
        )

        self.assertEqual(before, current)
        self.assertEqual(ARTIFACT_ID, plan.artifact_id)
        self.assertEqual(0x3E, plan.size)
        self.assertEqual("0x4e1386", plan.end_exclusive)
        proposed_row = plan.proposed["symbols"][ARTIFACT_ID]
        self.assertEqual("known", proposed_row["extent_state"])
        self.assertEqual(0x3E, proposed_row["size"])
        self.assertEqual("0x4e1386", proposed_row["end_exclusive"])
        self.assertEqual(
            before["symbols"][ARTIFACT_ID]["source_traceability"],
            proposed_row["source_traceability"],
        )
        self.assertEqual(set(before["symbols"]), set(plan.proposed["symbols"]))
        self.assertEqual(
            [FIRST_EVIDENCE, SECOND_EVIDENCE],
            proposed_row["evidence_ids"],
        )

    def test_dry_run_and_apply_use_revision_cas(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "progress.json"
            path.write_text(json.dumps(tracker()), encoding="utf-8")

            dry_run = mutate_data_extent(
                path,
                payload(),
                expected_revision=7,
                apply=False,
            )
            self.assertFalse(dry_run["applied"])
            self.assertFalse(dry_run["acceptance_changed"])
            self.assertFalse(dry_run["artifact_created"])
            self.assertFalse(dry_run["source_edges_changed"])
            self.assertEqual("unknown", json.loads(path.read_text())["symbols"][ARTIFACT_ID]["extent_state"])

            applied = mutate_data_extent(
                path,
                payload(),
                expected_revision=7,
                apply=True,
            )
            self.assertTrue(applied["applied"])
            saved = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(8, saved["revision"])
            self.assertEqual("known", saved["symbols"][ARTIFACT_ID]["extent_state"])

    def test_rejects_inconsistent_end_and_size(self):
        invalid = payload()
        invalid["replacement"]["end_exclusive"] = "0x4e1385"
        with self.assertRaisesRegex(
            DataExtentProgressError, "does not equal address \\+ size"
        ):
            plan_data_extent_registration(
                tracker(),
                invalid,
                expected_revision=7,
            )

    def test_rejects_unknown_or_duplicate_extent_evidence(self):
        invalid = payload()
        invalid["replacement"]["evidence_ids"] = [
            "recoil:evidence:r7:999999"
        ]
        with self.assertRaisesRegex(
            DataExtentProgressError, "unknown evidence ids"
        ):
            plan_data_extent_registration(
                tracker(),
                invalid,
                expected_revision=7,
            )

        invalid = payload()
        invalid["replacement"]["evidence_ids"] = [
            FIRST_EVIDENCE,
            FIRST_EVIDENCE,
        ]
        with self.assertRaisesRegex(
            DataExtentProgressError, "contains duplicates"
        ):
            normalize_register_payload(invalid)

    def test_rejects_extent_outside_exact_retail_output_section(self):
        current = tracker()
        current["output_sections"]["recoil:section:.data"]["reference"][
            "virtual_size"
        ] = 0x350
        with self.assertRaisesRegex(
            DataExtentProgressError, "outside retail output section"
        ):
            plan_data_extent_registration(
                current,
                payload(),
                expected_revision=7,
            )

    def test_rejects_stale_known_extent_or_present_extent_fields(self):
        for field, value in (
            ("extent_state", "known"),
            ("size", 0x3E),
            ("end_exclusive", "0x4e1386"),
        ):
            current = tracker()
            current["symbols"][ARTIFACT_ID][field] = value
            with self.assertRaisesRegex(
                DataExtentProgressError, "exact current extent is stale"
            ):
                plan_data_extent_registration(
                    current,
                    payload(),
                    expected_revision=7,
                )

    def test_rejects_logical_alias_and_non_data_artifact_ids(self):
        for artifact_id in (
            "recoil:logical-data:0x4e1348:sample",
            "recoil:function:0x401000",
        ):
            invalid = payload()
            invalid["artifact_id"] = artifact_id
            with self.assertRaisesRegex(
                DataExtentProgressError, "exact physical"
            ):
                normalize_register_payload(invalid)

    def test_payload_is_fail_closed_and_parent_reviewed(self):
        invalid = payload()
        invalid["parent_reviewed"] = False
        with self.assertRaisesRegex(
            DataExtentProgressError, "parent_reviewed=true"
        ):
            normalize_register_payload(invalid)

        invalid = payload()
        invalid["unexpected"] = True
        with self.assertRaisesRegex(
            DataExtentProgressError, "keys must be exactly"
        ):
            normalize_register_payload(invalid)


class DataArtifactProgressTests(unittest.TestCase):
    def test_plan_creates_only_pending_exact_data_catalog_row(self):
        current = tracker()
        before = deepcopy(current)

        plan = plan_data_artifact_registration(
            current,
            artifact_payload(),
            expected_revision=7,
        )

        self.assertEqual(before, current)
        self.assertEqual(
            set(before["symbols"]) | {"recoil:data:0x4e1320"},
            set(plan.proposed["symbols"]),
        )
        row = plan.proposed["symbols"]["recoil:data:0x4e1320"]
        self.assertEqual("known", row["extent_state"])
        self.assertEqual(0x20, row["size"])
        self.assertEqual("0x4e1340", row["end_exclusive"])
        self.assertEqual([], row["storage_contribution_ids"])
        self.assertEqual(
            {
                "state": "unresolved",
                "source_edges": [],
                "reason_code": "pending-reviewed-source-edge",
            },
            row["source_traceability"],
        )
        self.assertNotIn("owner_ids", row)
        self.assertTrue(
            all(
                state["result"] == "pending"
                for state in row["binary_state"].values()
            )
        )

    def test_dry_run_creates_no_tracker_state(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "progress.json"
            path.write_text(json.dumps(tracker()), encoding="utf-8")

            result = mutate_data_artifact(
                path,
                artifact_payload(),
                expected_revision=7,
                apply=False,
            )

            self.assertFalse(result["applied"])
            self.assertFalse(result["acceptance_changed"])
            self.assertEqual(0, result["source_edges_created"])
            self.assertEqual(0, result["owner_links_created"])
            self.assertEqual(0, result["storage_contributions_created"])
            self.assertNotIn(
                "recoil:data:0x4e1320",
                json.loads(path.read_text(encoding="utf-8"))["symbols"],
            )

    def test_rejects_existing_identity_or_address_collision(self):
        current = tracker()
        invalid = artifact_payload()
        invalid["artifact_id"] = ARTIFACT_ID
        invalid["artifact"]["address"] = "0x4e1348"
        invalid["artifact"]["end_exclusive"] = "0x4e1368"
        with self.assertRaisesRegex(
            DataArtifactProgressError, "already exists"
        ):
            plan_data_artifact_registration(
                current,
                invalid,
                expected_revision=7,
            )

        current["symbols"]["recoil:data:0x4e1320:other"] = {
            "address": "0x4e1320",
            "binary": "recoil",
            "kind": "data",
        }
        with self.assertRaisesRegex(
            DataArtifactProgressError, "already has physical tracker identities"
        ):
            plan_data_artifact_registration(
                current,
                artifact_payload(),
                expected_revision=7,
            )

    def test_rejects_unknown_evidence_target_and_section_escape(self):
        invalid = artifact_payload()
        invalid["artifact"]["evidence_ids"] = [
            "recoil:evidence:r7:999999"
        ]
        with self.assertRaisesRegex(
            DataArtifactProgressError, "unknown evidence ids"
        ):
            plan_data_artifact_registration(
                tracker(),
                invalid,
                expected_revision=7,
            )

        invalid = artifact_payload()
        invalid["artifact"]["verification_target_ids"] = [
            "recoil:vc5-target:missing"
        ]
        with self.assertRaisesRegex(
            DataArtifactProgressError, "unknown verification target ids"
        ):
            plan_data_artifact_registration(
                tracker(),
                invalid,
                expected_revision=7,
            )

        invalid = artifact_payload()
        invalid["artifact"]["address"] = "0x4e2000"
        invalid["artifact_id"] = "recoil:data:0x4e2000"
        invalid["artifact"]["end_exclusive"] = "0x4e2020"
        with self.assertRaisesRegex(
            DataArtifactProgressError, "outside retail output section"
        ):
            plan_data_artifact_registration(
                tracker(),
                invalid,
                expected_revision=7,
            )

    def test_artifact_payload_is_exact_parent_reviewed_and_physical(self):
        invalid = artifact_payload()
        invalid["parent_reviewed"] = False
        with self.assertRaisesRegex(
            DataArtifactProgressError, "parent_reviewed=true"
        ):
            normalize_artifact_payload(invalid)

        invalid = artifact_payload()
        invalid["artifact_id"] = "recoil:logical-data:0x4e1320:sample"
        with self.assertRaisesRegex(
            DataArtifactProgressError, "exact physical"
        ):
            normalize_artifact_payload(invalid)

    def test_provider_registration_is_not_applicable_and_has_no_source_edge(self):
        reviewed = artifact_payload()
        reviewed["artifact_id"] = "recoil:data:0x4e1340"
        reviewed["artifact"]["address"] = "0x4e1340"
        reviewed["artifact"]["end_exclusive"] = "0x4e1341"
        reviewed["artifact"]["size"] = 1
        reviewed["artifact"]["navigation_name"] = "CompilerPooledLiteral"
        reviewed["artifact"]["disposition"] = "provider"
        reviewed["artifact"]["source_traceability_state"] = "not-applicable"
        reviewed["artifact"][
            "source_traceability_reason_code"
        ] = "compiler-literal-pooling"
        reviewed["artifact"]["verification_target_ids"] = []

        plan = plan_data_artifact_registration(
            tracker(),
            reviewed,
            expected_revision=7,
        )

        row = plan.proposed["symbols"]["recoil:data:0x4e1340"]
        self.assertEqual("provider", row["disposition"])
        self.assertEqual(
            {
                "state": "not-applicable",
                "source_edges": [],
                "reason_code": "compiler-literal-pooling",
            },
            row["source_traceability"],
        )
        self.assertEqual([], row["verification_target_ids"])

    def test_artifact_registration_atomically_allocates_non_gating_evidence(self):
        current = tracker()
        before = deepcopy(current)

        plan = plan_data_artifact_registration(
            current,
            artifact_payload_with_new_evidence(),
            expected_revision=7,
        )

        self.assertEqual(current, before)
        self.assertEqual("recoil:evidence:r8:000001", plan.evidence_id)
        self.assertEqual(
            ["recoil:evidence:r8:000001"],
            plan.row["evidence_ids"],
        )
        evidence = plan.proposed["evidence"][plan.evidence_id]
        self.assertFalse(evidence["gating"])
        self.assertEqual("observed", evidence["disposition"])
        self.assertEqual("observed", evidence["result"])
        self.assertEqual(
            "historical-observation",
            evidence["validation_mode"],
        )
        self.assertEqual("historical", evidence["freshness"])
        self.assertEqual(
            "none", evidence["provenance"]["acceptance_effect"]
        )
        self.assertEqual(
            ["recoil:data:0x41b898"], evidence["scope_ids"]
        )
        self.assertNotIn(
            "recoil:data:0x41b898", current["symbols"]
        )

    def test_atomic_evidence_apply_references_allocated_id_from_artifact(self):
        current = tracker()
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "progress.json"
            path.write_text(json.dumps(current), encoding="utf-8")

            result = mutate_data_artifact(
                path,
                artifact_payload_with_new_evidence(),
                expected_revision=7,
                apply=True,
            )
            updated = json.loads(path.read_text(encoding="utf-8"))

        evidence_id = result["registered_evidence"]["id"]
        self.assertTrue(result["applied"])
        self.assertEqual(8, updated["revision"])
        self.assertIn(evidence_id, updated["evidence"])
        self.assertEqual(
            [evidence_id],
            updated["symbols"]["recoil:data:0x41b898"]["evidence_ids"],
        )
        self.assertFalse(result["acceptance_changed"])

    def test_atomic_evidence_rejects_existing_named_evidence(self):
        current = tracker()
        current["evidence"][FIRST_EVIDENCE]["scope_ids"] = [
            "recoil:data:0x41b898"
        ]
        with self.assertRaisesRegex(
            DataArtifactProgressError,
            "only when no existing evidence row names the artifact",
        ):
            plan_data_artifact_registration(
                current,
                artifact_payload_with_new_evidence(),
                expected_revision=7,
            )

    def test_atomic_evidence_rejects_scope_observation_and_command_drift(self):
        cases = []
        invalid = artifact_payload_with_new_evidence()
        invalid["new_evidence"]["scope_ids"] = ["recoil:data:0x41b8ac"]
        cases.append((invalid, "scope_ids"))
        invalid = artifact_payload_with_new_evidence()
        invalid["new_evidence"]["observation"]["size"] = 21
        cases.append((invalid, "exactly repeat"))
        invalid = artifact_payload_with_new_evidence()
        invalid["new_evidence"]["command"] = (
            "python tools/recoil.py audit bn-data-evidence 0x41b8ac "
            "--size 20 --binary recoil --json"
        )
        cases.append((invalid, "current bn-data-evidence"))
        for invalid, message in cases:
            with self.subTest(message=message):
                with self.assertRaisesRegex(
                    DataArtifactProgressError, message
                ):
                    normalize_artifact_payload(invalid)

    def test_atomic_evidence_checks_supplied_file_size(self):
        reviewed = artifact_payload_with_new_evidence()
        reviewed["new_evidence"]["artifacts"] = [
            {"path": "evidence/mission.cod", "size": 4}
        ]
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            artifact = root / "evidence" / "mission.cod"
            artifact.parent.mkdir()
            artifact.write_bytes(b"five!")
            with self.assertRaisesRegex(
                DataArtifactProgressError, "file size changed"
            ):
                plan_data_artifact_registration(
                    tracker(),
                    reviewed,
                    expected_revision=7,
                    repo_root=root,
                )

    def test_disposition_and_source_traceability_state_must_agree(self):
        invalid = artifact_payload()
        invalid["artifact"]["disposition"] = "provider"
        with self.assertRaisesRegex(
            DataArtifactProgressError,
            "requires source_traceability_state='not-applicable'",
        ):
            normalize_artifact_payload(invalid)


class DataArtifactEvidenceRepairTests(unittest.TestCase):
    @staticmethod
    def repair_payload():
        return {
            "operation": (
                "repair-reviewed-data-artifact-observation-schema"
            ),
            "parent_reviewed": True,
            "artifact_id": "recoil:data:0x41b898",
            "evidence_id": "recoil:evidence:r7:000003",
            "expected_invalid": {
                "freshness": "current-unhashed",
                "validation_mode": "reviewed-non-gating-observation",
            },
        }

    @staticmethod
    def repair_tracker():
        current = tracker()
        artifact_id = "recoil:data:0x41b898"
        evidence_id = "recoil:evidence:r7:000003"
        current["symbols"][artifact_id] = {
            "address": "0x41b898",
            "binary": "recoil",
            "kind": "data",
            "evidence_ids": [evidence_id],
        }
        current["evidence"][evidence_id] = {
            "kind": "reviewed-data-artifact-observation",
            "summary": "Reviewed exact data observation.",
            "scope_ids": [artifact_id],
            "result": "observed",
            "disposition": "observed",
            "freshness": "current-unhashed",
            "gating": False,
            "validation_mode": "reviewed-non-gating-observation",
            "artifacts": [],
            "provenance": {
                "acceptance_effect": "none",
            },
        }
        return current

    def test_plan_repairs_only_invalid_schema_pair(self):
        current = self.repair_tracker()
        before = deepcopy(current)

        plan = plan_data_artifact_evidence_repair(
            current,
            self.repair_payload(),
            expected_revision=7,
        )

        self.assertEqual(current, before)
        repaired = plan.proposed["evidence"][plan.evidence_id]
        self.assertEqual("historical", repaired["freshness"])
        self.assertEqual(
            "historical-observation", repaired["validation_mode"]
        )
        expected = deepcopy(before["evidence"][plan.evidence_id])
        expected["freshness"] = "historical"
        expected["validation_mode"] = "historical-observation"
        self.assertEqual(expected, repaired)

    def test_apply_is_revision_guarded_and_reports_no_acceptance(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "progress.json"
            path.write_text(
                json.dumps(self.repair_tracker()), encoding="utf-8"
            )

            result = mutate_data_artifact_evidence_repair(
                path,
                self.repair_payload(),
                expected_revision=7,
                apply=True,
            )
            updated = json.loads(path.read_text(encoding="utf-8"))

        self.assertTrue(result["applied"])
        self.assertFalse(result["acceptance_changed"])
        self.assertFalse(result["owner_gates_changed"])
        self.assertFalse(result["owner_tiers_changed"])
        self.assertEqual(8, updated["revision"])
        self.assertEqual(
            "historical",
            updated["evidence"]["recoil:evidence:r7:000003"]["freshness"],
        )

    def test_rejects_semantic_drift_and_nonexact_expected_pair(self):
        drifted = self.repair_tracker()
        drifted["evidence"]["recoil:evidence:r7:000003"]["gating"] = True
        with self.assertRaisesRegex(
            DataArtifactEvidenceRepairError, "semantic identity drifted"
        ):
            plan_data_artifact_evidence_repair(
                drifted,
                self.repair_payload(),
                expected_revision=7,
            )

        payload = self.repair_payload()
        payload["expected_invalid"]["freshness"] = "historical"
        with self.assertRaisesRegex(
            DataArtifactEvidenceRepairError, "known invalid"
        ):
            normalize_repair_payload(payload)

    def test_rejects_artifact_evidence_link_drift(self):
        current = self.repair_tracker()
        current["symbols"]["recoil:data:0x41b898"]["evidence_ids"] = []
        with self.assertRaisesRegex(
            DataArtifactEvidenceRepairError, "does not reference"
        ):
            plan_data_artifact_evidence_repair(
                current,
                self.repair_payload(),
                expected_revision=7,
            )


class DataLogicalAliasProgressTests(unittest.TestCase):
    def test_plan_creates_only_authored_logical_occurrences(self):
        current = provider_tracker()
        before = deepcopy(current)

        plan = plan_logical_data_alias_batch(
            current,
            alias_payload(),
            expected_revision=7,
        )

        self.assertEqual(before, current)
        self.assertEqual(set(before["symbols"]), set(plan.proposed["symbols"]))
        physical = plan.proposed["symbols"]["recoil:data:0x4e1340"]
        self.assertEqual(
            before["symbols"]["recoil:data:0x4e1340"]["source_traceability"],
            physical["source_traceability"],
        )
        self.assertEqual(
            {
                "recoil:logical-data:0x4e1340:first",
                "recoil:logical-data:0x4e1340:second",
            },
            set(physical["logical_aliases"]),
        )
        for alias in physical["logical_aliases"].values():
            self.assertEqual("authored", alias["disposition"])
            self.assertEqual("compiler-literal-pooling", alias["pooling"]["mode"])
            self.assertEqual([], alias["source_traceability"]["source_edges"])
            self.assertEqual(
                "unresolved", alias["source_traceability"]["state"]
            )
            self.assertTrue(alias["object_symbol"])

    def test_dry_run_creates_no_alias_state(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "progress.json"
            path.write_text(json.dumps(provider_tracker()), encoding="utf-8")

            result = mutate_logical_data_alias_batch(
                path,
                alias_payload(),
                expected_revision=7,
                apply=False,
            )

            self.assertFalse(result["applied"])
            self.assertFalse(result["acceptance_changed"])
            self.assertEqual(0, result["physical_artifacts_created"])
            self.assertEqual(0, result["physical_source_edges_created"])
            saved = json.loads(path.read_text(encoding="utf-8"))
            self.assertNotIn(
                "logical_aliases",
                saved["symbols"]["recoil:data:0x4e1340"],
            )

    def test_rejects_non_provider_or_physical_source_edge(self):
        current = provider_tracker()
        current["symbols"]["recoil:data:0x4e1340"]["disposition"] = "authored"
        with self.assertRaisesRegex(
            DataLogicalAliasProgressError, "exact reviewed state is stale"
        ):
            plan_logical_data_alias_batch(
                current,
                alias_payload(),
                expected_revision=7,
            )

        current = provider_tracker()
        current["symbols"]["recoil:data:0x4e1340"]["source_traceability"][
            "source_edges"
        ] = [{"relation": "defines"}]
        with self.assertRaisesRegex(
            DataLogicalAliasProgressError, "must not have production source edges"
        ):
            plan_logical_data_alias_batch(
                current,
                alias_payload(),
                expected_revision=7,
            )

    def test_rejects_duplicate_ids_and_existing_alias(self):
        invalid = alias_payload()
        invalid["aliases"][1]["artifact_id"] = invalid["aliases"][0]["artifact_id"]
        with self.assertRaisesRegex(
            DataLogicalAliasProgressError, "duplicates"
        ):
            normalize_alias_payload(invalid)

        current = provider_tracker()
        current["symbols"]["recoil:data:0x4e1340"]["logical_aliases"] = {
            "recoil:logical-data:0x4e1340:first": {
                "kind": "data",
            }
        }
        with self.assertRaisesRegex(
            DataLogicalAliasProgressError, "already exist"
        ):
            plan_logical_data_alias_batch(
                current,
                alias_payload(),
                expected_revision=7,
            )

    def test_three_logical_occurrences_may_share_one_pooled_object_symbol(self):
        reviewed = alias_payload()
        shared_symbol = "??_C@_00A@?$AA@"
        reviewed["aliases"][0]["object_symbol"] = shared_symbol
        reviewed["aliases"][1]["object_symbol"] = shared_symbol
        third = deepcopy(reviewed["aliases"][1])
        third["artifact_id"] = "recoil:logical-data:0x4e1340:third"
        third["navigation_name"] = "ThirdLogicalLiteral"
        third["object_symbol"] = shared_symbol
        reviewed["aliases"].append(third)

        plan = plan_logical_data_alias_batch(
            provider_tracker(),
            reviewed,
            expected_revision=7,
        )

        aliases = plan.proposed["symbols"]["recoil:data:0x4e1340"][
            "logical_aliases"
        ]
        self.assertEqual(3, len(aliases))
        self.assertEqual(
            {shared_symbol},
            {alias["object_symbol"] for alias in aliases.values()},
        )

    def test_address_equality_without_evidence_or_object_symbol_is_rejected(self):
        invalid = alias_payload()
        invalid["aliases"][0]["evidence_ids"] = []
        with self.assertRaisesRegex(
            DataLogicalAliasProgressError, "non-empty array"
        ):
            normalize_alias_payload(invalid)

        invalid = alias_payload()
        invalid["aliases"][0]["object_symbol"] = ""
        with self.assertRaisesRegex(
            DataLogicalAliasProgressError, "exact non-whitespace"
        ):
            normalize_alias_payload(invalid)

    def test_rejects_unknown_pooling_or_alias_evidence(self):
        for field in ("pooling", "alias"):
            invalid = alias_payload()
            if field == "pooling":
                invalid["pooling"]["evidence_ids"] = [
                    "recoil:evidence:r7:999999"
                ]
            else:
                invalid["aliases"][0]["evidence_ids"] = [
                    "recoil:evidence:r7:999999"
                ]
            with self.assertRaisesRegex(
                DataLogicalAliasProgressError, "unknown evidence ids"
            ):
                plan_logical_data_alias_batch(
                    provider_tracker(),
                    invalid,
                    expected_revision=7,
                )


if __name__ == "__main__":
    unittest.main()
