from __future__ import annotations

import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.final_image_catalog_audit import audit_catalog  # noqa: E402
from _recoil.commands.final_image_coverage import derive_final_image_coverage  # noqa: E402
from _recoil.commands.live_final_verify import compare_images  # noqa: E402
from _recoil.lib.pe import parse_pe_headers  # noqa: E402
from _recoil.lib.progress import EXACT_LINK_DIMENSIONS, FULL_ORDER_DIMENSIONS  # noqa: E402
from _recoil.lib.worktree_control import resolve_canonical_control_root  # noqa: E402


def canonical_retail_reference() -> Path:
    resolution = resolve_canonical_control_root(
        executing_worktree_root=REPO_ROOT,
        required_machine_local_paths=("support/Recoil.exe",),
    )
    return resolution.canonical_control_root / "support" / "Recoil.exe"


def accepted_state() -> dict[str, object]:
    return {
        "result": "passed",
        "disposition": "accepted",
        "freshness": "current",
        "gating": True,
        "validation_mode": "live",
        "evidence_ids": ["unit:evidence"],
    }


class FinalImageCoverageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.reference = canonical_retail_reference()
        cls.reference_data = cls.reference.read_bytes()
        cls.headers = parse_pe_headers(cls.reference_data, source=str(cls.reference))

    def output_sections(self) -> dict[str, object]:
        result: dict[str, object] = {}
        for ordinal, section in enumerate(self.headers.sections):
            result[f"recoil:section:{section.name}"] = {
                "binary": "recoil",
                "name": section.name,
                "ordinal": ordinal,
                "reference": {
                    "image_address": f"0x{self.headers.image_base + section.virtual_address:x}",
                    "rva": f"0x{section.virtual_address:x}",
                    "virtual_size": section.virtual_size,
                    "raw_pointer": section.raw_pointer,
                    "raw_size": section.raw_size,
                    "characteristics": f"0x{section.characteristics:x}",
                },
            }
        return result

    def tracker(self, *, complete: bool) -> dict[str, object]:
        tracker: dict[str, object] = {
            "schema_version": 5,
            "revision": 12,
            "binaries": {"recoil": {"binary": "recoil"}},
            "output_sections": self.output_sections(),
            "symbols": {},
            "physical_blocks": {},
            "storage_contributions": {},
            "verification_targets": {},
        }
        if not complete:
            return tracker
        symbols = tracker["symbols"]
        blocks = tracker["physical_blocks"]
        storage_rows = tracker["storage_contributions"]
        text = next(section for section in self.headers.sections if section.name == ".text")
        text_start = self.headers.image_base + text.virtual_address
        text_end = text_start + text.virtual_size
        symbol_id = "recoil:function:unit-full-text"
        block_id = "recoil:block:unit-full-text"
        symbols[symbol_id] = {
            "binary": "recoil",
            "kind": "function",
            "pipeline_class": "authored",
            "physical_block_id": block_id,
            "output_section_id": "recoil:section:.text",
            "extent_state": "known",
            "address": f"0x{text_start:x}",
            "end_exclusive": f"0x{text_end:x}",
            "map_symbol": "unit_full_text",
            "object": "unit.obj",
            "accepted_byte_facts": {"validation_mode": "live", "lane": "linked"},
            "binary_state": {
                dimension: accepted_state() for dimension in EXACT_LINK_DIMENSIONS
            },
            "verification_target_ids": [],
        }
        blocks[block_id] = {
            "binary": "recoil",
            "contribution_ids": [symbol_id],
            "accepted_order_facts": {
                "phase": "full-function-order",
                "validation_mode": "live",
                "covered_block_ids": [block_id],
                "matched_identities": [symbol_id],
            },
            "order": {
                "full": {dimension: accepted_state() for dimension in FULL_ORDER_DIMENSIONS}
            },
        }
        for section_name in (".rdata", ".data"):
            section = next(row for row in self.headers.sections if row.name == section_name)
            start = self.headers.image_base + section.virtual_address
            end = start + section.virtual_size
            storage_id = f"recoil:storage:unit:{section_name}"
            data_symbol_id = f"recoil:data:unit:{section_name}"
            symbols[data_symbol_id] = {
                "binary": "recoil",
                "kind": "data",
                "output_section_id": f"recoil:section:{section_name}",
                "extent_state": "known",
                "address": f"0x{start:x}",
                "end_exclusive": f"0x{end:x}",
                "storage_contribution_ids": [storage_id],
            }
            storage_rows[storage_id] = {
                "binary": "recoil",
                "kind": "data-symbol",
                "output_section_id": f"recoil:section:{section_name}",
                "symbol_ids": [data_symbol_id],
                "reference": {
                    "extent_state": "known",
                    "address": f"0x{start:x}",
                    "end_exclusive": f"0x{end:x}",
                },
                "applicability": {"extent": True, "raw": True, "link": True},
                "verification": {
                    name: accepted_state() for name in ("extent", "raw", "link")
                },
            }
        return tracker

    def test_missing_blob_yields_live_typed_gaps(self) -> None:
        tracker = self.tracker(complete=False)
        with tempfile.TemporaryDirectory() as temporary:
            progress = Path(temporary) / "progress.json"
            progress.write_text(json.dumps(tracker), encoding="utf-8")
            result = audit_catalog(tracker=progress, reference=self.reference)
        self.assertFalse(result["passed"])
        self.assertFalse(result["legacy_catalog_present"])
        self.assertFalse(result["legacy_catalog_required"])
        self.assertIsNotNone(result["coverage"])
        self.assertTrue(result["coverage"]["file_backed_topology"]["complete"])
        self.assertTrue(result["coverage"]["rva_topology"]["complete"])
        self.assertTrue(any("section .text" in failure for failure in result["failures"]))
        self.assertFalse(any("missing binaries.recoil.final_image_catalog" in failure for failure in result["failures"]))

    def test_complete_accepted_tracker_generates_complete_coverage(self) -> None:
        coverage = derive_final_image_coverage(
            self.reference_data,
            self.tracker(complete=True),
            source="support/Recoil.exe",
        )
        self.assertTrue(coverage["complete"], coverage["failures"])
        self.assertEqual(1, len(coverage["selected_text_identities"]))
        for section in coverage["sections"]:
            self.assertTrue(section["file_semantic_coverage"]["complete"], section)
            self.assertTrue(section["virtual_semantic_coverage"]["complete"], section)

    def test_overlapping_accepted_text_annotations_fail_closed(self) -> None:
        tracker = self.tracker(complete=True)
        text = next(section for section in self.headers.sections if section.name == ".text")
        start = self.headers.image_base + text.virtual_address
        original_id = "recoil:function:unit-full-text"
        tracker["symbols"][original_id]["end_exclusive"] = f"0x{start + 0x100:x}"
        overlap_id = "recoil:function:unit-overlap-text"
        overlap_block = "recoil:block:unit-overlap-text"
        tracker["symbols"][overlap_id] = {
            **tracker["symbols"][original_id],
            "address": f"0x{start + 0x80:x}",
            "end_exclusive": f"0x{start + text.virtual_size:x}",
            "physical_block_id": overlap_block,
            "map_symbol": "unit_overlap_text",
        }
        tracker["physical_blocks"][overlap_block] = {
            **tracker["physical_blocks"]["recoil:block:unit-full-text"],
            "contribution_ids": [overlap_id],
            "accepted_order_facts": {
                "phase": "full-function-order",
                "validation_mode": "live",
                "covered_block_ids": [overlap_block],
                "matched_identities": [overlap_id],
            },
        }
        coverage = derive_final_image_coverage(
            self.reference_data,
            tracker,
            source="support/Recoil.exe",
        )
        text_row = next(row for row in coverage["sections"] if row["name"] == ".text")
        self.assertFalse(coverage["complete"])
        self.assertTrue(text_row["virtual_semantic_coverage"]["overlaps"])
        self.assertTrue(any("overlapping typed entities" in failure for failure in coverage["failures"]))

    def test_generated_coverage_keeps_timestamp_and_whole_file_diagnostic(self) -> None:
        coverage = derive_final_image_coverage(
            self.reference_data,
            self.tracker(complete=True),
            source="support/Recoil.exe",
        )
        data = bytearray(self.reference_data)
        timestamp_offset = self.headers.pe_offset + 8
        original = struct.unpack_from("<I", data, timestamp_offset)[0]
        struct.pack_into("<I", data, timestamp_offset, (original + 1) & 0xFFFFFFFF)
        with tempfile.TemporaryDirectory() as temporary:
            candidate = Path(temporary) / "candidate.exe"
            candidate.write_bytes(data)
            report = compare_images(
                candidate,
                self.reference,
                coverage=coverage,
                candidate_map_rows=[
                    {
                        "address": coverage["selected_text_identities"][0]["address"],
                        "symbol": "unit_full_text",
                        "object": "unit.obj",
                    }
                ],
                tracker=self.tracker(complete=True),
            )
        self.assertTrue(report["passed"], report["semantic_failures"])
        self.assertEqual("live-generated", report["coverage_mode"])
        self.assertTrue(report["timestamp_is_diagnostic_only"])
        self.assertFalse(report["raw_file_equal_diagnostic"])
        self.assertTrue(report["timestamp_only_raw_difference"])


if __name__ == "__main__":
    unittest.main()
