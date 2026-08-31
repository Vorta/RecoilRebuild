from __future__ import annotations

from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.final_image_coverage import COVERAGE_VERSION  # noqa: E402
from _recoil.commands.live_final_verify import (  # noqa: E402
    compare_images,
    run,
)
from _recoil.lib.pe import parse_pe_headers  # noqa: E402


class LiveFinalVerifyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.reference = REPO_ROOT / "support" / "Recoil.exe"
        self.reference_data = self.reference.read_bytes()
        self.headers = parse_pe_headers(self.reference_data, source=str(self.reference))

    def coverage(self, *, complete: bool = True) -> dict[str, object]:
        failures = [] if complete else ["fixture typed coverage gap"]
        return {
            "version": COVERAGE_VERSION,
            "kind": "live-final-image-coverage",
            "binary": "recoil",
            "complete": complete,
            "failures": failures,
            "reference_layout": {
                "image_base": self.headers.image_base,
                "size_of_image": self.headers.size_of_image,
                "file_size": len(self.reference_data),
                "section_count": self.headers.section_count,
            },
            "selected_text_identities": [],
            "timestamp_is_diagnostic_only": True,
            "raw_whole_file_equality_is_diagnostic_only": True,
        }

    def compare(self, data: bytes, *, coverage: dict[str, object] | None = None):
        with tempfile.TemporaryDirectory() as temporary:
            candidate = Path(temporary) / "candidate.exe"
            candidate.write_bytes(data)
            return compare_images(
                candidate,
                self.reference,
                coverage=coverage or self.coverage(),
                candidate_map_rows=[],
            )

    def test_timestamp_only_difference_passes_typed_semantic_gate(self) -> None:
        data = bytearray(self.reference_data)
        timestamp_offset = self.headers.pe_offset + 8
        original = struct.unpack_from("<I", data, timestamp_offset)[0]
        struct.pack_into("<I", data, timestamp_offset, (original + 1) & 0xFFFFFFFF)
        report = self.compare(bytes(data))
        self.assertTrue(report["passed"], report["semantic_failures"])
        self.assertTrue(report["timestamp_only_raw_difference"])
        self.assertFalse(report["raw_file_equal_diagnostic"])
        self.assertEqual("live-generated", report["coverage_mode"])
        self.assertNotIn("catalog_version", report)

    def test_section_payload_mutation_fails(self) -> None:
        text = next(section for section in self.headers.sections if section.name == ".text")
        data = bytearray(self.reference_data)
        data[text.raw_pointer] ^= 1
        report = self.compare(bytes(data))
        self.assertFalse(report["passed"])
        self.assertTrue(
            any("section .text payload differs" in row for row in report["semantic_failures"])
        )

    def test_incomplete_live_coverage_fails_before_acceptance(self) -> None:
        report = self.compare(self.reference_data, coverage=self.coverage(complete=False))
        self.assertFalse(report["passed"])
        self.assertFalse(report["coverage_complete"])
        self.assertIn("live final coverage is incomplete", report["semantic_failures"])
        self.assertIn("fixture typed coverage gap", report["semantic_failures"])

    def test_selected_text_population_is_live_coverage_driven(self) -> None:
        text = next(section for section in self.headers.sections if section.name == ".text")
        coverage = self.coverage()
        coverage["selected_text_identities"] = [
            {
                "address": self.headers.image_base + text.virtual_address,
                "symbol": "expected_symbol",
                "object": "expected.obj",
            }
        ]
        report = self.compare(self.reference_data, coverage=coverage)
        self.assertFalse(report["passed"])
        self.assertTrue(
            any("MAP population is missing" in row for row in report["semantic_failures"])
        )

    def test_report_contains_no_content_summary_or_legacy_catalog_fields(self) -> None:
        report = self.compare(self.reference_data)

        def keys(value: object) -> list[str]:
            if isinstance(value, dict):
                return [str(key) for key in value] + [
                    item for child in value.values() for item in keys(child)
                ]
            if isinstance(value, list):
                return [item for child in value for item in keys(child)]
            return []

        all_keys = [key.casefold() for key in keys(report)]
        for forbidden in ("sha256", "hexdigest", "merkle", "legacy_catalog", "catalog_version"):
            self.assertFalse(any(forbidden in key for key in all_keys))

    def test_existing_candidate_mode_remains_diagnostic_only(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            candidate = root / "candidate.exe"
            candidate.write_bytes(self.reference_data)
            map_path = root / "candidate.map"
            map_path.write_text("", encoding="utf-8")
            args = SimpleNamespace(
                progress=root / "progress.sqlite3",
                reference=self.reference,
                candidate=candidate,
                map=map_path,
                build_root=None,
            )
            with (
                patch(
                    "_recoil.commands.live_final_verify._load_coverage_from_open_retail",
                    return_value=(self.coverage(), {}),
                ),
                patch(
                    "_recoil.commands.live_final_verify.parse_link_map",
                    return_value=SimpleNamespace(symbols=[]),
                ),
            ):
                report = run(args)
        self.assertTrue(report["semantic_comparison_passed"])
        self.assertFalse(report["passed"])
        self.assertFalse(report["acceptance_eligible"])
        self.assertEqual("existing-diagnostic", report["candidate_mode"])


if __name__ == "__main__":
    unittest.main()
