from __future__ import annotations

import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.lib.owner_entries import OwnerEntryIndex  # noqa: E402
from _recoil.lib.source_owners import (  # noqa: E402
    OWNER_LEDGER_REPLACE_RETRY_DELAYS,
    SourceOwnerDocument,
    derive_owner_reimplementation_tier,
    owner_anchor_addresses,
    owner_data_address_records,
    owner_dependency_ids,
    owner_member_addresses,
    owner_relationships,
    primary_owners_for_entry,
)
from tests.tools.owner_fixture import ledger_payload, owner_record, write_ledger  # noqa: E402


READY_GATES = {
    "boundary": "accepted",
    "source": "accepted",
    "data": "accepted",
    "functional": "accepted",
    "linkage": "accepted",
    "byte": "blocked",
}


def winerror_5() -> PermissionError:
    exc = PermissionError(5, "Access is denied")
    exc.winerror = 5
    return exc


class SourceOwnerDocumentSaveTests(unittest.TestCase):
    def test_load_requires_schema_v4(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "SOURCE_OWNERS.json"
            path.write_text('{"schema_version": 3, "owners": []}', encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "schema_version 4 is required"):
                SourceOwnerDocument.load(path)

    def test_unified_progress_schema_one_two_and_three_project_identically_read_only(self) -> None:
        payload = {
            "schema_version": 1,
            "symbols": {
                "recoil:function:0x401000": {
                    "binary": "recoil", "kind": "function", "address": "0x401000"
                }
            },
            "evidence": {},
            "owners": {
                "recoil:owner:test.owner": {
                    "binary": "recoil", "kind": "standalone", "name": "Test",
                    "relationships": [
                        {"kind": "primary-function", "symbol_id": "recoil:function:0x401000", "address": "0x401000"}
                    ],
                    "gates": {
                        "boundary": "pending", "source": "pending", "data": "pending",
                        "functional": "pending", "owner_linkage": "pending", "byte": "pending",
                    },
                    "reimplementation": {
                        "entries": {"recoil:function:0x401000": {"kind": "function", "tier": "X", "evidence_ids": []}}
                    },
                }
            },
        }
        projections = []
        with tempfile.TemporaryDirectory() as temp:
            for version in (1, 2, 3):
                path = Path(temp) / f"progress-v{version}.json"
                payload["schema_version"] = version
                path.write_text(json.dumps(payload), encoding="utf-8")
                document = SourceOwnerDocument.load(path)
                projections.append(document.payload)
                with self.assertRaisesRegex(ValueError, "avoid overwriting"):
                    document.save()
        self.assertEqual(projections[0], projections[1])
        self.assertEqual(projections[1], projections[2])

    def test_save_rejects_stale_loaded_document(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "SOURCE_OWNERS.json"
            write_ledger(path, owner_record("test.owner", functions=["0x401000"]))
            first = SourceOwnerDocument.load(path)
            second = SourceOwnerDocument.load(path)
            first.link_address("test.owner", "0x401010")
            self.assertTrue(first.save())
            second.link_address("test.owner", "0x401020")
            with self.assertRaisesRegex(ValueError, "changed on disk after load"):
                second.save()
            self.assertEqual(
                ["0x401000", "0x401010"],
                json.loads(path.read_text(encoding="utf-8"))["owners"][0]["member_addresses"],
            )

    def test_save_rejects_existing_lock_file(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "SOURCE_OWNERS.json"
            write_ledger(path, owner_record("test.owner", functions=["0x401000"]))
            path.with_name(f"{path.name}.lock").write_text("pid=test\n", encoding="ascii")
            doc = SourceOwnerDocument.load(path)
            doc.link_address("test.owner", "0x401010")
            with self.assertRaisesRegex(ValueError, "owner ledger lock exists"):
                doc.save()

    def test_unlink_prunes_only_orphan_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "SOURCE_OWNERS.json"
            write_ledger(
                path,
                owner_record(
                    "test.owner",
                    anchors=["0x401000"],
                    functions=["0x401000", "0x401010"],
                    address_metadata={
                        "0x401000": {"name": "Anchor"},
                        "0x401010": {"name": "Removed"},
                    },
                ),
            )
            doc = SourceOwnerDocument.load(path)
            doc.unlink_address("test.owner", "0x401010")
            doc.save()
            metadata = json.loads(path.read_text(encoding="utf-8"))["owners"][0]["address_metadata"]
            self.assertEqual({"0x401000": {"name": "Anchor"}}, metadata)

    def test_save_retries_transient_windows_replace_failure(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "SOURCE_OWNERS.json"
            write_ledger(path, owner_record("test.owner", functions=["0x401000"]))
            real_replace = os.replace
            attempts = 0

            def flaky_replace(src: Path, dst: Path) -> None:
                nonlocal attempts
                attempts += 1
                if attempts <= 2:
                    raise winerror_5()
                real_replace(src, dst)

            doc = SourceOwnerDocument.load(path)
            doc.link_address("test.owner", "0x401010")
            with mock.patch("_recoil.lib.source_owners.os.replace", side_effect=flaky_replace):
                with mock.patch("_recoil.lib.source_owners.time.sleep") as sleep:
                    self.assertTrue(doc.save())
            self.assertEqual(3, attempts)
            self.assertEqual(2, sleep.call_count)

    def test_save_reports_persistent_windows_replace_failure_and_cleans_temp(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "SOURCE_OWNERS.json"
            write_ledger(path, owner_record("test.owner", functions=["0x401000"]))
            doc = SourceOwnerDocument.load(path)
            doc.link_address("test.owner", "0x401010")
            with mock.patch("_recoil.lib.source_owners.os.replace", side_effect=lambda *_: (_ for _ in ()).throw(winerror_5())):
                with mock.patch("_recoil.lib.source_owners.time.sleep") as sleep:
                    with self.assertRaisesRegex(PermissionError, "failed to atomically replace owner ledger"):
                        doc.save()
            self.assertEqual(len(OWNER_LEDGER_REPLACE_RETRY_DELAYS), sleep.call_count)
            self.assertEqual([], list(path.parent.glob(".SOURCE_OWNERS.json.*.tmp")))


class SourceOwnerRelationshipTests(unittest.TestCase):
    def test_schema_v4_relationships_and_mirrors_are_consistent(self) -> None:
        dependency = owner_record("test.dependency", functions=["0x402000"])
        primary = owner_record(
            "test.owner",
            anchors=["0x401000"],
            functions=["0x401010"],
            data=[("0x4f0000", "g_Test")],
            dependencies=[("test.dependency", "data-dependency")],
        )
        doc = SourceOwnerDocument(Path("unused"), ledger_payload(primary, dependency))
        owner = doc.owner("test.owner")
        self.assertEqual([], doc.validate())
        self.assertEqual({"0x401000"}, owner_anchor_addresses(owner))
        self.assertEqual({"0x401010"}, owner_member_addresses(owner))
        self.assertEqual([("0x4f0000", "g_Test")], [(row.address, row.name) for row in owner_data_address_records(owner)])
        self.assertEqual(["test.dependency"], owner_dependency_ids(owner))
        self.assertEqual(
            ["anchor-address", "primary-function", "primary-data", "depends-on-owner"],
            [row.kind for row in owner_relationships(owner)],
        )

    def test_validation_reports_mirror_drift_and_missing_dependency(self) -> None:
        owner = owner_record("test.owner", functions=["0x401000"])
        owner["member_addresses"] = ["0x401010"]
        owner["relationships"].append(
            {"kind": "depends-on-owner", "target_owner_id": "missing.owner", "reason": "manual"}
        )
        findings = SourceOwnerDocument(Path("unused"), ledger_payload(owner)).validate()
        self.assertTrue(any("primary-function relationships do not match member_addresses" in row for row in findings))
        self.assertTrue(any("dependency owner not found" in row for row in findings))

    def test_primary_owner_lookup_uses_typed_relationship(self) -> None:
        owner = owner_record("test.owner", functions=["0x401000"])
        doc = SourceOwnerDocument(Path("unused"), ledger_payload(owner))
        entry = OwnerEntryIndex.from_source_owners(doc).entries["0x401000"]
        self.assertEqual(["test.owner"], [row.id for row in primary_owners_for_entry(doc, entry)])

    def test_link_and_unlink_update_relationships_mirrors_and_entry_tiers(self) -> None:
        doc = SourceOwnerDocument(Path("unused"), ledger_payload(owner_record("test.owner")))
        doc.link_address("test.owner", "0x401000")
        doc.link_data("test.owner", "0x4f0000", "g_Test")
        owner = doc.owner("test.owner")
        self.assertEqual({"0x401000"}, owner_member_addresses(owner))
        self.assertEqual({"0x4f0000"}, {row.address for row in owner_data_address_records(owner)})
        self.assertEqual("X", owner.entry_reimplementation_tier("0x401000"))
        self.assertEqual("X", owner.entry_reimplementation_tier("0x4f0000"))
        doc.unlink_address("test.owner", "0x401000")
        doc.unlink_data("test.owner", "0x4f0000")
        self.assertEqual([], doc.validate())

    def test_cross_owner_primary_duplicates_are_rejected_by_mutation(self) -> None:
        doc = SourceOwnerDocument(
            Path("unused"),
            ledger_payload(
                owner_record("test.first", functions=["0x401000"]),
                owner_record("test.second"),
            ),
        )
        with self.assertRaisesRegex(ValueError, "already owned by test.first"):
            doc.link_address("test.second", "0x401000")
        with self.assertRaisesRegex(ValueError, "already primary function"):
            doc.link_data("test.second", "0x401000", "g_Conflict")


class SourceOwnerEntryTierTests(unittest.TestCase):
    def mixed_owner(self) -> dict[str, object]:
        return owner_record(
            "test.owner",
            anchors=["0x401000"],
            functions=["0x401000", "0x401010"],
            data=[("0x4f0000", "g_Test")],
            tiers={"0x401000": "S", "0x401010": "C", "0x4f0000": "B"},
            gates=READY_GATES,
        )

    def test_exact_primary_coverage_and_mixed_floor(self) -> None:
        doc = SourceOwnerDocument(Path("unused"), ledger_payload(self.mixed_owner()))
        self.assertEqual([], doc.validate())
        self.assertEqual("C", derive_owner_reimplementation_tier(doc.owner("test.owner")))
        self.assertEqual("S", doc.owner("test.owner").entry_reimplementation_tier("0x401000"))

    def test_all_s_is_capped_at_a_until_byte_gate_is_accepted(self) -> None:
        owner = self.mixed_owner()
        for record in owner["reimplementation"]["entries"].values():
            record["tier"] = "S"
        doc = SourceOwnerDocument(Path("unused"), ledger_payload(owner))
        self.assertEqual("A", doc.owner("test.owner").reimplementation_tier)
        owner["gates"]["byte"] = "accepted"
        self.assertEqual("S", SourceOwnerDocument(Path("unused"), ledger_payload(owner)).owner("test.owner").reimplementation_tier)

    def test_validation_reports_missing_extra_wrong_kind_and_missing_evidence(self) -> None:
        owner = self.mixed_owner()
        entries = owner["reimplementation"]["entries"]
        entries.pop("0x401010")
        entries["0x499999"] = {"kind": "data", "tier": "C", "evidence": "extra"}
        entries["0x401000"]["kind"] = "data"
        entries["0x4f0000"]["evidence"] = ""
        findings = SourceOwnerDocument(Path("unused"), ledger_payload(owner)).validate()
        self.assertTrue(any("missing primary entry tier" in row for row in findings))
        self.assertTrue(any("non-primary entry tier" in row for row in findings))
        self.assertTrue(any("does not match primary relationship" in row for row in findings))
        self.assertTrue(any("evidence must be non-empty" in row for row in findings))

    def test_unlink_refuses_accepted_tier_but_allows_new_x_entry(self) -> None:
        doc = SourceOwnerDocument(Path("unused"), ledger_payload(self.mixed_owner()))
        doc.link_address("test.owner", "0x401020")
        self.assertEqual("X", doc.owner("test.owner").entry_reimplementation_tier("0x401020"))
        doc.unlink_address("test.owner", "0x401020")
        with self.assertRaisesRegex(ValueError, "refuses to unlink accepted tier"):
            doc.unlink_address("test.owner", "0x401000")

    def test_tier_s_requires_b_baseline_and_records_reviewed_live_evidence(self) -> None:
        owner = owner_record(
            "test.owner",
            functions=["0x401000"],
            tiers={"0x401000": "C"},
            gates=READY_GATES,
        )
        doc = SourceOwnerDocument(Path("unused"), ledger_payload(owner))
        with self.assertRaisesRegex(ValueError, "tier-B/A source evidence baseline"):
            doc.set_entry_reimplementation("test.owner", "0x401000", "S", "live exact")
        doc.set_entry_reimplementation("test.owner", "0x401000", "B", "accepted source")
        old_tier, new_tier = doc.set_entry_reimplementation(
            "test.owner", "0x401000", "S", "parent reviewed current live exact comparison"
        )
        self.assertEqual(("B", "A"), (old_tier, new_tier))
        record = doc.owner("test.owner").entry_reimplementation("0x401000")
        self.assertEqual({"kind", "tier", "evidence"}, set(record))
        self.assertIn("current live", record["evidence"])


class OwnerEntryIndexTests(unittest.TestCase):
    def test_binary_filter_function_data_provider_and_anchor_exclusion(self) -> None:
        owners = [
            owner_record(
                "recoil.owner",
                anchors=["0x400000"],
                functions=["0x401020"],
                data=[("0x4f0000", "g_Value")],
                tiers={"0x401020": "C", "0x4f0000": "B"},
                gates=READY_GATES,
            ),
            owner_record(
                "messages.owner",
                binary="messages",
                functions=["0x10001010"],
                tiers={"0x10001010": "C"},
                gates=READY_GATES,
            ),
            owner_record(
                "provider.owner",
                kind="provider-boundary",
                functions=["0x402000"],
                name="ProviderFunction",
            ),
        ]
        doc = SourceOwnerDocument(Path("unused"), ledger_payload(*owners))
        recoil = OwnerEntryIndex.from_source_owners(doc, binary="recoil")
        messages = OwnerEntryIndex.from_source_owners(doc, binary="messages")
        self.assertEqual(["0x401020", "0x4f0000", "0x402000"], recoil.order)
        self.assertNotIn("0x400000", recoil.entries)
        self.assertEqual("data", recoil.entries["0x4f0000"].entry_kind)
        self.assertTrue(recoil.entries["0x402000"].is_provider_boundary)
        self.assertEqual(["0x10001010"], messages.order)

    def test_order_is_owner_order_then_numeric_address(self) -> None:
        doc = SourceOwnerDocument(
            Path("unused"),
            ledger_payload(
                owner_record("z.owner", functions=["0x401020", "0x401000"], data=[("0x401010", "g")]),
                owner_record("a.owner", functions=["0x402000"]),
            ),
        )
        self.assertEqual(
            ["0x401000", "0x401010", "0x401020", "0x402000"],
            OwnerEntryIndex.from_source_owners(doc).order,
        )

    def test_duplicate_primary_address_is_rejected(self) -> None:
        doc = SourceOwnerDocument(
            Path("unused"),
            ledger_payload(
                owner_record("first.owner", functions=["0x401000"]),
                owner_record("second.owner", functions=["0x401000"]),
            ),
        )
        with self.assertRaisesRegex(ValueError, "duplicate primary address"):
            OwnerEntryIndex.from_source_owners(doc)

    def test_metadata_falls_back_to_owner_and_relationship_fields(self) -> None:
        owner = owner_record(
            "test.owner",
            functions=["0x401000"],
            data=[("0x4f0000", "g_Test")],
            name="Test Owner",
            source_paths=["src/Test.cpp"],
            address_metadata={"0x401000": {"name": "SpecificName", "target": "specific_target"}},
        )
        index = OwnerEntryIndex.from_source_owners(SourceOwnerDocument(Path("unused"), ledger_payload(owner)))
        self.assertEqual("SpecificName", index.entries["0x401000"].reimplemented_name)
        self.assertEqual("specific_target", index.entries["0x401000"].functional_target)
        self.assertEqual("g_Test", index.entries["0x4f0000"].reimplemented_name)
        self.assertEqual("src/Test.cpp", index.entries["0x4f0000"].reimplemented_file)

    def test_missing_entry_tier_validates_and_projects_as_x(self) -> None:
        owner = owner_record("test.owner", functions=["0x401000"])
        owner["reimplementation"]["entries"] = {}
        doc = SourceOwnerDocument(Path("unused"), ledger_payload(owner))
        self.assertTrue(any("missing primary entry tier" in row for row in doc.validate()))
        self.assertEqual("X", OwnerEntryIndex.from_source_owners(doc).entries["0x401000"].reimplementation_tier)


if __name__ == "__main__":
    unittest.main()
