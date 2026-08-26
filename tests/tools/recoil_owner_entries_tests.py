from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from _recoil.lib.owner_entries import OwnerEntryIndex  # noqa: E402
from _recoil.lib.source_owners import SourceOwnerDocument  # noqa: E402


def owner(
    owner_id: str,
    *,
    binary: str = "recoil",
    kind: str = "source-file",
    relationships: list[dict[str, str]] | None = None,
    entries: dict[str, dict[str, str]] | None = None,
    gates: dict[str, str] | None = None,
) -> dict[str, object]:
    relationships = relationships or []
    members = [item["address"] for item in relationships if item["kind"] == "primary-function"]
    data = [
        {"address": item["address"], "name": item.get("name", "pending")}
        for item in relationships
        if item["kind"] == "primary-data"
    ]
    anchors = [item["address"] for item in relationships if item["kind"] == "anchor-address"]
    dependencies = [item["target_owner_id"] for item in relationships if item["kind"] == "depends-on-owner"]
    payload: dict[str, object] = {
        "id": owner_id,
        "kind": kind,
        "name": owner_id,
        "section": "test_section",
        "state": "active",
        "anchors": anchors,
        "member_addresses": members,
        "data_addresses": data,
        "source_paths": ["src/test.cpp"] if kind != "provider-boundary" else [],
        "dependencies": dependencies,
        "relationships": relationships,
        "gates": gates or {
            "boundary": "accepted",
            "source": "accepted",
            "data": "none",
            "functional": "accepted",
            "linkage": "accepted",
            "byte": "blocked",
        },
        "blocker": "test blocker",
        "evidence": ["test evidence"],
        "binary": binary,
        "address_metadata": {},
    }
    if kind != "provider-boundary":
        payload["reimplementation"] = {"entries": entries or {}}
    return payload


class OwnerEntryIndexTests(unittest.TestCase):
    def write_ledger(self, root: Path, owners: list[dict[str, object]]) -> Path:
        path = root / "SOURCE_OWNERS.json"
        path.write_text(json.dumps({"schema_version": 4, "owners": owners}), encoding="utf-8")
        return path

    def test_builds_function_data_and_provider_entries_with_binary_filter(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = self.write_ledger(
                root,
                [
                    owner(
                        "test.authored",
                        relationships=[
                            {"kind": "anchor-address", "address": "0x401000"},
                            {"kind": "primary-data", "address": "0x401020", "name": "g_Test"},
                            {"kind": "primary-function", "address": "0x401010"},
                        ],
                        entries={
                            "0x401010": {"kind": "function", "tier": "C", "evidence": "function"},
                            "0x401020": {"kind": "data", "tier": "B", "evidence": "data"},
                        },
                    ),
                    owner(
                        "test.provider",
                        kind="provider-boundary",
                        relationships=[{"kind": "primary-function", "address": "0x402000"}],
                    ),
                    owner(
                        "messages.entry",
                        binary="messages",
                        relationships=[{"kind": "primary-function", "address": "0x10001010"}],
                        entries={"0x10001010": {"kind": "function", "tier": "S", "evidence": "messages"}},
                        gates={
                            "boundary": "accepted",
                            "source": "accepted",
                            "data": "none",
                            "functional": "accepted",
                            "linkage": "accepted",
                            "byte": "accepted",
                        },
                    ),
                ],
            )
            index = OwnerEntryIndex.load(path, binary="recoil")
            self.assertEqual(["0x401010", "0x401020", "0x402000"], index.order)
            self.assertNotIn("0x401000", index.entries)
            self.assertTrue(index.entries["0x401020"].is_data_entry)
            self.assertTrue(index.entries["0x402000"].is_provider_boundary)
            self.assertNotIn("0x10001010", index.entries)

    def test_load_projects_schema_v5_unified_progress_owner_mapping(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "RECONSTRUCTION_PROGRESS.json"
            payload = {
                "schema_version": 5,
                "revision": 1,
                "symbols": {
                    "recoil:function:0x401000": {
                        "binary": "recoil",
                        "kind": "function",
                        "address": "0x401000",
                        "navigation_name": "TestFunction",
                    }
                },
                "evidence": {},
                "owners": {
                    "recoil:owner:test.function": {
                        "legacy_id": "test.function",
                        "binary": "recoil",
                        "kind": "source-file",
                        "name": "Test Function",
                        "lifecycle_state": "active",
                        "section": "test",
                        "source_paths": ["src/test.cpp"],
                        "relationships": [
                            {
                                "kind": "primary-function",
                                "address": "0x401000",
                                "symbol_id": "recoil:function:0x401000",
                            }
                        ],
                        "gates": {},
                        "reimplementation": {
                            "entries": {
                                "recoil:function:0x401000": {
                                    "kind": "function",
                                    "tier": "C",
                                    "evidence_ids": [],
                                }
                            }
                        },
                    }
                },
            }
            path.write_text(
                json.dumps(payload),
                encoding="utf-8",
            )

            index = OwnerEntryIndex.load(path, binary="recoil")

            self.assertIn("0x401000", index.entries)
            entry = index.entries["0x401000"]
            self.assertEqual("test.function", entry.source_owner_parent)
            self.assertEqual("function", entry.entry_kind)
            self.assertEqual("C", entry.reimplementation_tier)

    def test_mixed_entry_tiers_remain_visible_while_owner_gate_caps_tier(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self.write_ledger(
                Path(tmp),
                [
                    owner(
                        "test.mixed",
                        relationships=[
                            {"kind": "primary-function", "address": "0x401000"},
                            {"kind": "primary-function", "address": "0x401010"},
                        ],
                        entries={
                            "0x401000": {"kind": "function", "tier": "B", "evidence": "b"},
                            "0x401010": {"kind": "function", "tier": "S", "evidence": "s"},
                        },
                        gates={
                            "boundary": "blocked",
                            "source": "blocked",
                            "data": "none",
                            "functional": "accepted",
                            "linkage": "blocked",
                            "byte": "blocked",
                        },
                    )
                ],
            )
            doc = SourceOwnerDocument.load(path)
            self.assertEqual("C", doc.owner("test.mixed").reimplementation_tier)
            index = OwnerEntryIndex.from_source_owners(doc)
            self.assertEqual("B", index.entries["0x401000"].reimplementation_tier)
            self.assertEqual("S", index.entries["0x401010"].reimplementation_tier)

    def test_duplicate_primary_address_across_owners_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self.write_ledger(
                Path(tmp),
                [
                    owner("test.first", relationships=[{"kind": "primary-function", "address": "0x401000"}], entries={"0x401000": {"kind": "function", "tier": "C", "evidence": "first"}}),
                    owner("test.second", relationships=[{"kind": "primary-function", "address": "0x401000"}], entries={"0x401000": {"kind": "function", "tier": "C", "evidence": "second"}}),
                ],
            )
            with self.assertRaisesRegex(ValueError, "duplicate primary address"):
                OwnerEntryIndex.load(path)

    def test_missing_entry_tier_is_reported_by_schema_v4_validation(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self.write_ledger(
                Path(tmp),
                [owner("test.missing", relationships=[{"kind": "primary-function", "address": "0x401000"}])],
            )
            findings = SourceOwnerDocument.load(path).validate()
            self.assertTrue(any("missing primary entry tier records: 0x401000" in item for item in findings))


if __name__ == "__main__":
    unittest.main()
