from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.storage_contribution_progress import (  # noqa: E402
    APPLICABILITY,
    PAYLOAD_SCHEMA,
    REGISTER_OPERATION,
    StorageContributionProgressError,
    mutate_authored_storage,
    normalize_register_payload,
    plan_authored_storage_registration,
)
from _recoil.lib.live_progress import ConcurrentRevisionUpdate  # noqa: E402
from _recoil.lib.progress import empty_progress_document  # noqa: E402


SYMBOL_ID = "recoil:data:0x4e1320"
STORAGE_ID = "recoil:storage:va:0x4e1320"
OWNER_ID = "recoil:owner:sample.authored_data"
SECTION_ID = "recoil:section:.data"
ADDRESS = "0x4e1320"
SIZE = 0x20
END_EXCLUSIVE = "0x4e1340"
NAME = "SampleAuthoredTable"


def tracker(*, revision: int = 7) -> dict:
    data = empty_progress_document()
    data["revision"] = revision
    data["output_sections"][SECTION_ID] = {
        "binary": "recoil",
        "reference": {
            "image_address": "0x4e1000",
            "virtual_size": 0x1000,
        },
    }
    data["symbols"][SYMBOL_ID] = {
        "address": ADDRESS,
        "binary": "recoil",
        "binary_state": {
            "linked_address": {
                "disposition": "claim",
                "evidence_ids": [],
                "freshness": "current-unhashed",
                "result": "pending",
            }
        },
        "disposition": "authored",
        "end_exclusive": END_EXCLUSIVE,
        "evidence_ids": [],
        "extent_state": "known",
        "kind": "data",
        "navigation_name": NAME,
        "output_section_id": SECTION_ID,
        "physical_block_id": None,
        "semantic_span_ids": [],
        "size": SIZE,
        "source_traceability": {
            "state": "unresolved",
            "source_edges": [],
            "reason_code": "compiler-emitted-data-source-edge-pending",
        },
        "storage_contribution_ids": [],
        "verification_target_ids": [],
    }
    data["owners"][OWNER_ID] = {
        "address_metadata": {},
        "binary": "recoil",
        "blocker": "none",
        "evidence_ids": [],
        "gates": {
            "boundary": "accepted",
            "byte": "deferred",
            "data": "accepted",
            "functional": "accepted",
            "owner_linkage": "accepted",
            "source": "accepted",
        },
        "kind": "data-owner",
        "lifecycle_state": "accepted",
        "name": "SampleAuthoredData",
        "provider_state": "pending",
        "reimplementation": {
            "entries": {
                SYMBOL_ID: {
                    "evidence_ids": [],
                    "kind": "data",
                    "tier": "B",
                }
            }
        },
        "relationships": [
            {
                "address": ADDRESS,
                "kind": "primary-data",
                "name": NAME,
                "symbol_id": SYMBOL_ID,
            }
        ],
        "section": "sample",
        "source_paths": ["src/sample.cpp"],
    }
    return data


def payload() -> dict:
    return {
        "schema": PAYLOAD_SCHEMA,
        "operation": REGISTER_OPERATION,
        "reviewed": True,
        "parent_reviewed": True,
        "symbol_id": SYMBOL_ID,
        "storage_contribution_id": STORAGE_ID,
        "owner_id": OWNER_ID,
        "expected_symbol": {
            "binary": "recoil",
            "kind": "data",
            "disposition": "authored",
            "address": ADDRESS,
            "extent_state": "known",
            "size": SIZE,
            "end_exclusive": END_EXCLUSIVE,
            "output_section_id": SECTION_ID,
            "storage_contribution_ids": [],
        },
        "expected_owner_relationship": {
            "kind": "primary-data",
            "symbol_id": SYMBOL_ID,
            "address": ADDRESS,
            "name": NAME,
        },
    }


class StorageContributionProgressTests(unittest.TestCase):
    def test_plan_creates_only_pending_storage_and_symbol_link(self) -> None:
        current = tracker()
        before = deepcopy(current)

        plan = plan_authored_storage_registration(
            current,
            payload(),
            expected_revision=7,
        )

        self.assertEqual(before, current)
        self.assertEqual(
            set(before["storage_contributions"]) | {STORAGE_ID},
            set(plan.proposed["storage_contributions"]),
        )
        expected = deepcopy(before)
        expected["storage_contributions"][STORAGE_ID] = deepcopy(
            plan.storage_row
        )
        expected["symbols"][SYMBOL_ID]["storage_contribution_ids"] = [
            STORAGE_ID
        ]
        self.assertEqual(expected, plan.proposed)
        row = plan.storage_row
        self.assertEqual("data-symbol", row["kind"])
        self.assertEqual([OWNER_ID], row["owner_ids"])
        self.assertEqual([SYMBOL_ID], row["symbol_ids"])
        self.assertEqual(APPLICABILITY, row["applicability"])
        self.assertEqual(
            {
                "address": ADDRESS,
                "end_exclusive": END_EXCLUSIVE,
                "evidence_ids": [],
                "extent_state": "known",
                "size": SIZE,
            },
            row["reference"],
        )
        for state in row["verification"].values():
            self.assertEqual("claim", state["disposition"])
            self.assertEqual("pending", state["result"])
            self.assertNotIn("gating", state)
            self.assertNotIn("validation_mode", state)
        self.assertEqual(
            before["owners"],
            plan.proposed["owners"],
        )

    def test_dry_run_apply_and_revision_cas_are_exact(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "progress.json"
            path.write_text(json.dumps(tracker()), encoding="utf-8")

            dry_run = mutate_authored_storage(
                path,
                payload(),
                expected_revision=7,
                apply=False,
            )
            self.assertFalse(dry_run["applied"])
            self.assertFalse(dry_run["acceptance_changed"])
            self.assertEqual(
                [],
                json.loads(path.read_text(encoding="utf-8"))["symbols"][
                    SYMBOL_ID
                ]["storage_contribution_ids"],
            )
            for field in (
                "source_edges_created",
                "owner_records_changed",
                "owner_gates_changed",
                "owner_tiers_changed",
            ):
                self.assertEqual(0, dry_run[field])
            for field in (
                "order_state_changed",
                "byte_state_changed",
                "provider_state_changed",
                "link_state_changed",
                "final_image_state_changed",
            ):
                self.assertFalse(dry_run[field])

            applied = mutate_authored_storage(
                path,
                payload(),
                expected_revision=7,
                apply=True,
            )
            self.assertTrue(applied["applied"])
            saved = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(8, saved["revision"])
            self.assertEqual(
                [STORAGE_ID],
                saved["symbols"][SYMBOL_ID]["storage_contribution_ids"],
            )
            self.assertIn(STORAGE_ID, saved["storage_contributions"])
            self.assertEqual(tracker()["owners"], saved["owners"])

            with self.assertRaises(ConcurrentRevisionUpdate):
                plan_authored_storage_registration(
                    saved,
                    payload(),
                    expected_revision=7,
                )

    def test_payload_requires_exact_schema_review_and_keys(self) -> None:
        for field, value, message in (
            ("schema", "wrong", "payload.schema"),
            ("operation", "wrong", "payload.operation"),
            ("reviewed", False, "reviewed=true"),
            ("parent_reviewed", False, "reviewed=true"),
        ):
            invalid = payload()
            invalid[field] = value
            with self.assertRaisesRegex(
                StorageContributionProgressError,
                message,
            ):
                normalize_register_payload(invalid)

        invalid = payload()
        invalid["unexpected"] = True
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "keys must be exactly",
        ):
            normalize_register_payload(invalid)

    def test_rejects_noncanonical_or_mismatched_ids(self) -> None:
        for field, value, message in (
            ("symbol_id", "recoil:data:0X4E1320", "canonical physical"),
            (
                "storage_contribution_id",
                "recoil:storage:va:0x4e1324",
                "canonical",
            ),
            ("owner_id", "messages:owner:sample", "canonical owner"),
        ):
            invalid = payload()
            invalid[field] = value
            with self.assertRaisesRegex(
                StorageContributionProgressError,
                message,
            ):
                normalize_register_payload(invalid)

    def test_rejects_invalid_expected_symbol_shape_or_extent(self) -> None:
        for field, value, message in (
            ("kind", "function", "kind must be 'data'"),
            ("disposition", "provider", "existing authored"),
            ("extent_state", "unknown", "must be 'known'"),
            ("size", 0, "must be positive"),
            ("end_exclusive", "0x4e133f", "address \\+ size"),
            ("output_section_id", "messages:section:.data", "canonical"),
            (
                "storage_contribution_ids",
                ["recoil:storage:va:0x4e1320"],
                "exact empty",
            ),
        ):
            invalid = payload()
            invalid["expected_symbol"][field] = value
            with self.assertRaisesRegex(
                StorageContributionProgressError,
                message,
            ):
                normalize_register_payload(invalid)

    def test_rejects_stale_existing_symbol_snapshot(self) -> None:
        for field, value in (
            ("disposition", "provider"),
            ("extent_state", "unknown"),
            ("size", SIZE + 4),
            ("end_exclusive", "0x4e1344"),
            ("output_section_id", "recoil:section:.rdata"),
            ("storage_contribution_ids", [STORAGE_ID]),
        ):
            current = tracker()
            current["symbols"][SYMBOL_ID][field] = value
            with self.assertRaisesRegex(
                StorageContributionProgressError,
                "snapshot is stale",
            ):
                plan_authored_storage_registration(
                    current,
                    payload(),
                    expected_revision=7,
                )

    def test_rejects_missing_ambiguous_or_stale_primary_owner(self) -> None:
        current = tracker()
        current["owners"][OWNER_ID]["relationships"] = []
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "exactly one existing primary-data owner",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

        current = tracker()
        duplicate = deepcopy(current["owners"][OWNER_ID])
        current["owners"]["recoil:owner:sample.duplicate"] = duplicate
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "found 2",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

        current = tracker()
        current["owners"][OWNER_ID]["relationships"][0]["name"] = "Drifted"
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "relationship is stale",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

    def test_rejects_duplicate_primary_data_address_identity(self) -> None:
        current = tracker()
        duplicate = deepcopy(current["owners"][OWNER_ID])
        duplicate["id"] = "recoil:owner:sample.duplicate_address"
        duplicate["relationships"] = [
            {
                "kind": "primary-data",
                "symbol_id": "recoil:data:0x4e1320:logical-duplicate",
                "address": "0x4e1320",
                "name": "LogicalDuplicate",
            }
        ]
        current["owners"][duplicate["id"]] = duplicate
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "exactly one primary-data identity at address",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

    def test_rejects_provider_owner(self) -> None:
        current = tracker()
        current["owners"][OWNER_ID]["kind"] = "provider-boundary"
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "rejects provider-boundary",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

    def test_rejects_section_binary_mismatch_or_escape(self) -> None:
        current = tracker()
        current["output_sections"][SECTION_ID]["binary"] = "messages"
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "does not belong",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

        current = tracker()
        current["output_sections"][SECTION_ID]["reference"][
            "virtual_size"
        ] = 0x325
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "outside retail output section",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

    def test_rejects_existing_storage_identity_or_symbol_reference(self) -> None:
        current = tracker()
        current["storage_contributions"][STORAGE_ID] = {}
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "already exists",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

        current = tracker()
        current["storage_contributions"][
            "recoil:storage:va:0x4e1400"
        ] = {
            "binary": "recoil",
            "reference": {
                "address": "0x4e1400",
                "extent_state": "unknown",
            },
            "symbol_ids": [SYMBOL_ID],
        }
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "already referenced",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

    def test_rejects_known_and_unknown_storage_overlap(self) -> None:
        current = tracker()
        current["storage_contributions"][
            "recoil:storage:va:0x4e1310"
        ] = {
            "binary": "recoil",
            "reference": {
                "address": "0x4e1310",
                "extent_state": "known",
                "size": 0x20,
                "end_exclusive": "0x4e1330",
            },
            "symbol_ids": ["recoil:data:0x4e1310"],
        }
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "overlaps existing storage",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

        current = tracker()
        current["storage_contributions"][
            "recoil:storage:va:0x4e1330"
        ] = {
            "binary": "recoil",
            "reference": {
                "address": "0x4e1330",
                "extent_state": "unknown",
            },
            "symbol_ids": ["recoil:data:0x4e1330"],
        }
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "unknown-extent start",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

    def test_rejects_malformed_existing_known_storage_extent(self) -> None:
        current = tracker()
        current["storage_contributions"][
            "recoil:storage:va:0x4e1400"
        ] = {
            "binary": "recoil",
            "reference": {
                "address": "0x4e1400",
                "extent_state": "known",
                "size": 0x10,
                "end_exclusive": "0x4e1420",
            },
            "symbol_ids": ["recoil:data:0x4e1400"],
        }
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "end_exclusive must equal address",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

    def test_rejects_physical_data_symbol_overlap(self) -> None:
        current = tracker()
        current["symbols"]["recoil:data:0x4e1318"] = {
            "address": "0x4e1318",
            "binary": "recoil",
            "kind": "data",
            "extent_state": "known",
            "size": 0x10,
            "end_exclusive": "0x4e1328",
        }
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "overlaps existing physical data symbol",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

        current = tracker()
        current["symbols"]["recoil:data:0x4e1338"] = {
            "address": "0x4e1338",
            "binary": "recoil",
            "kind": "data",
            "extent_state": "unknown",
        }
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "unknown-extent start",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )

    def test_rejects_stale_relationship_snapshot(self) -> None:
        invalid = payload()
        invalid["expected_owner_relationship"]["name"] = "OtherName"
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "relationship is stale",
        ):
            plan_authored_storage_registration(
                tracker(),
                invalid,
                expected_revision=7,
            )

    def test_rejects_noncanonical_owner_id(self) -> None:
        invalid = payload()
        invalid["owner_id"] = "recoil:owner:bad id"
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "invalid owner id",
        ):
            plan_authored_storage_registration(
                tracker(),
                invalid,
                expected_revision=7,
            )

    def test_rejects_dangling_storage_reference_from_another_symbol(self) -> None:
        current = tracker()
        current["symbols"]["recoil:data:0x4e1400"] = {
            "address": "0x4e1400",
            "binary": "recoil",
            "disposition": "authored",
            "extent_state": "unknown",
            "kind": "data",
            "storage_contribution_ids": [STORAGE_ID],
        }
        with self.assertRaisesRegex(
            StorageContributionProgressError,
            "already referenced by symbols",
        ):
            plan_authored_storage_registration(
                current,
                payload(),
                expected_revision=7,
            )


if __name__ == "__main__":
    unittest.main()
