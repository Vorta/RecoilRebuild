from __future__ import annotations

import contextlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands import final_data_diff  # noqa: E402
from tests.tools.owner_fixture import owner_record, write_ledger  # noqa: E402


def minimal_pe(section_name: str, *, rva: int, virtual_size: int, raw_size: int, raw_pointer: int = 0x200) -> bytes:
    data = bytearray(raw_pointer + raw_size)
    data[:2] = b"MZ"
    pe_offset = 0x80
    struct.pack_into("<I", data, 0x3C, pe_offset)
    data[pe_offset : pe_offset + 4] = b"PE\0\0"
    coff_offset = pe_offset + 4
    optional_header_size = 0xE0
    struct.pack_into("<HHIIIHH", data, coff_offset, 0x14C, 1, 0, 0, 0, optional_header_size, 0x010F)
    optional_offset = coff_offset + 20
    struct.pack_into("<H", data, optional_offset, 0x10B)
    struct.pack_into("<I", data, optional_offset + 16, 0x1000)
    struct.pack_into("<I", data, optional_offset + 28, 0x400000)
    struct.pack_into("<II", data, optional_offset + 32, 0x1000, 0x200)
    struct.pack_into("<I", data, optional_offset + 56, rva + ((virtual_size + 0xFFF) & ~0xFFF))
    struct.pack_into("<H", data, optional_offset + 68, 2)
    struct.pack_into("<I", data, optional_offset + 92, 16)
    section_offset = optional_offset + optional_header_size
    name = section_name.encode("ascii")[:8].ljust(8, b"\0")
    data[section_offset : section_offset + 8] = name
    struct.pack_into(
        "<IIIIIIHHI",
        data,
        section_offset + 8,
        virtual_size,
        rva,
        raw_size,
        raw_pointer,
        0,
        0,
        0,
        0,
        0xC0000040,
    )
    return bytes(data)


class FinalDataDiffTests(unittest.TestCase):
    def test_section_payload_comparison_reports_first_live_byte_difference(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            reference = root / "reference.exe"
            candidate = root / "candidate.exe"
            reference_data = bytearray(
                minimal_pe(".data", rva=0x2000, virtual_size=0x20, raw_size=0x20)
            )
            candidate_data = bytearray(reference_data)
            reference_data[0x205] = 0x11
            candidate_data[0x205] = 0x22
            reference.write_bytes(reference_data)
            candidate.write_bytes(candidate_data)
            reference_facts = final_data_diff.pe_section_facts(reference, ".data")
            candidate_facts = final_data_diff.pe_section_facts(candidate, ".data")

            comparison = final_data_diff.compare_section_byte_slices(
                reference, candidate, reference_facts, candidate_facts
            )

            self.assertFalse(comparison.equal)
            self.assertEqual(1, comparison.mismatch_count)
            self.assertEqual(5, comparison.first_mismatch_offset)
            self.assertEqual(0x11, comparison.first_reference_byte)
            self.assertEqual(0x22, comparison.first_candidate_byte)

    def _report_with_direct_manifest_issues(self, count: int) -> final_data_diff.FinalDataReport:
        issues = tuple(
            final_data_diff.ManifestIssue(
                manifest=f"tools/vc5_verify_targets/sample_{index}.json",
                target=f"sample_data_{index}",
                name=f"g_Sample_{index}",
                symbol=f"_g_Sample_{index}",
                address=f"0x{0x4DA000 + index * 4:x}",
                byte_length=4,
                kind="candidate-address-drift",
                detail="candidate map places symbol elsewhere",
            )
            for index in range(count)
        )
        return final_data_diff.FinalDataReport(
            reference="support/Recoil.exe",
            candidate="build/vc5-final/Recoil.exe",
            map="build/vc5-final/Recoil.map",
            link_rsp="build/vc5-final/rsp/link.rsp",
            section=".data",
            reference_section=final_data_diff.SectionFacts(".data", 0xDA000, 0x100, 0x80, 0x200, 0x80),
            candidate_section=final_data_diff.SectionFacts(".data", 0xBE000, 0xE0, 0x90, 0x200, 0x50),
            deltas=(
                final_data_diff.SectionDelta("rva", 0xDA000, 0xBE000, -0x1C000),
            ),
            rankings={},
            map_sections=(),
            manifest_coverage=final_data_diff.ManifestCoverage(
                manifest_count=count,
                data_symbol_count=count,
                in_reference_section=count,
                symbol_name_matches=count,
                exact_address_matches=0,
                issues=issues,
            ),
            bn_coverage=final_data_diff.BnCoverage(available=False, error="mocked"),
            classifications=(),
        )

    def _write_empty_owners(self, root: Path) -> Path:
        owners = root / "SOURCE_OWNERS.json"
        write_ledger(owners)
        return owners

    def test_section_delta_model_reports_raw_virtual_and_zero_fill_drift(self) -> None:
        reference = final_data_diff.SectionFacts(
            name=".data",
            rva=0xDA000,
            virtual_size=0x29FAC0,
            raw_size=0xBC00,
            raw_pointer=0xD0000,
            zero_fill_tail=0x293EC0,
        )
        candidate = final_data_diff.SectionFacts(
            name=".data",
            rva=0xBE000,
            virtual_size=0x29DE18,
            raw_size=0xD600,
            raw_pointer=0xB4000,
            zero_fill_tail=0x290818,
        )

        deltas = {item.field: item.delta for item in final_data_diff.section_deltas(reference, candidate)}

        self.assertEqual(-0x1C000, deltas["rva"])
        self.assertEqual(-0x1CA8, deltas["virtual_size"])
        self.assertEqual(0x1A00, deltas["raw_size"])
        self.assertEqual(-0x36A8, deltas["zero_fill_tail"])

    def test_parse_map_extracts_sections_symbols_and_boundary_rankings(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample.map"
            path.write_text(
                "\n".join(
                    [
                        " Start         Length     Name                   Class",
                        " 0003:00000070 00000030H .data                   DATA",
                        " 0003:000000a0 00000100H .bss                    DATA",
                        "",
                        "  Address         Publics by Value              Rva+Base     Lib:Object",
                        " 0003:00000080       _g_before                  0040a080     alpha.obj",
                        " 0003:000000bc       _g_at_raw_boundary         0040a0bc     beta.obj",
                        " 0003:000000c0       ?g_path@?%D:\\Recoil Project\\src.cpp123@@3HA 0040a0c0     gamma.obj",
                    ]
                ),
                encoding="utf-8",
            )

            sections, symbols = final_data_diff.parse_map(path)
            data_section = final_data_diff.data_map_section(sections, ".data")
            segment_symbols = final_data_diff.symbols_in_segment(symbols, 3)
            beyond = final_data_diff.symbols_at_or_beyond(segment_symbols, 0xBC, 10)

        self.assertIsNotNone(data_section)
        self.assertEqual(0x70, data_section.offset)
        self.assertEqual("_g_at_raw_boundary", beyond[0]["symbol"])
        self.assertEqual("beta.obj", beyond[0]["object"])
        self.assertEqual("gamma.obj", beyond[1]["object"])

    def test_map_section_transitions_report_data_to_bss_objects_and_provider_origin(self) -> None:
        sections = (
            final_data_diff.MapSection(3, 0x70, 0x2D4, ".data", "DATA"),
            final_data_diff.MapSection(3, 0x348, 0x100, ".bss", "DATA"),
        )
        symbols = (
            final_data_diff.MapSymbol(3, 0x300, "_g_data", 0x4C6300, "zVideo.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0x340, "___defaultmatherr", 0x4C6340, "MSVCRT:merr.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0x348, "_g_bss", 0x4C6348, "ainet.obj", "Publics by Value"),
        )
        object_rows = (
            final_data_diff.ObjectContribution("zVideo.obj", data_size=0x540, bss_size=0x1000, sections=({"name": ".data", "size": 0x540},)),
            final_data_diff.ObjectContribution("ainet.obj", data_size=0x10, bss_size=0x20, sections=({"name": ".bss", "size": 0x20},)),
        )

        transitions = final_data_diff.map_section_transitions(
            map_sections=sections,
            segment_symbols=symbols,
            candidate_section=final_data_diff.SectionFacts(".data", 0xBB000, 0x1000, 0x800, 0x200, 0x800),
            data_section=sections[0],
            bss_boundary=0x348,
            object_rows=object_rows,
            limit=4,
        )

        self.assertEqual(2, len(transitions))
        boundary = transitions[1]
        self.assertEqual(".data_to_bss", boundary["name"])
        self.assertEqual("0x4bb348", boundary["boundary_address"])
        self.assertEqual(".data", boundary["preceding_section"]["name"])
        self.assertEqual(".bss", boundary["following_section"]["name"])
        self.assertEqual(4, boundary["gap_from_preceding_end"])
        provider_symbol = boundary["symbols_before"][1]
        self.assertEqual("___defaultmatherr", provider_symbol["symbol"])
        self.assertEqual("library-member", provider_symbol["object_origin"]["kind"])
        self.assertEqual("MSVCRT", provider_symbol["object_origin"]["library"])
        self.assertTrue(provider_symbol["object_origin"]["provider_candidate"])
        self.assertEqual("ainet.obj", boundary["objects_after"][0]["object"])
        self.assertEqual(0x20, boundary["objects_after"][0]["bss_size"])

    def test_candidate_boundary_packing_reports_boundary_gaps_and_ordered_object_sections(self) -> None:
        sections = (
            final_data_diff.MapSection(3, 0x70, 0x100, ".data", "DATA"),
            final_data_diff.MapSection(3, 0x180, 0x280, ".bss", "DATA"),
        )
        symbols = (
            final_data_diff.MapSymbol(3, 0x150, "_g_initialized_end", 0x4C6150, "zVideo.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0x180, "_g_bss_start", 0x4C6180, "ainet.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0x3F0, "_g_virtual_tail", 0x4C63F0, "ainet.obj", "Publics by Value"),
        )
        object_rows = (
            final_data_diff.ObjectContribution(
                "zVideo.obj",
                data_size=0x540,
                bss_size=0,
                sections=(
                    {"name": ".data", "size": 0x500, "order": 1},
                    {"name": ".data$z", "size": 0x40, "order": 4},
                ),
            ),
            final_data_diff.ObjectContribution(
                "ainet.obj",
                data_size=0x10,
                bss_size=0x280,
                sections=(
                    {"name": ".data", "size": 0x10, "order": 2},
                    {"name": ".bss", "size": 0x280, "order": 5},
                ),
            ),
        )

        packing = final_data_diff.candidate_boundary_packing(
            map_sections=sections,
            segment_symbols=symbols,
            candidate_section=final_data_diff.SectionFacts(".data", 0xC600, 0x400, 0x200, 0x200, 0x200),
            data_section=sections[0],
            bss_boundary=0x180,
            object_rows=object_rows,
            limit=4,
        )

        self.assertTrue(packing["available"])
        summary = packing["summary"]
        self.assertEqual(0x170, summary["data_section_end_offset"])
        self.assertEqual(0x200, summary["candidate_raw_end_offset"])
        self.assertEqual(0x180, summary["bss_start_offset"])
        self.assertEqual(0x10, summary["data_end_to_bss_start_slack"])
        self.assertEqual(-0x80, summary["raw_end_to_bss_start_slack"])
        bss_boundary = next(item for item in packing["boundaries"] if item["name"] == "bss_start")
        self.assertEqual(".data", bss_boundary["preceding_section"]["name"])
        self.assertEqual(".bss", bss_boundary["following_section"]["name"])
        self.assertEqual(0x10, bss_boundary["gap_from_preceding_end"])
        after_object = bss_boundary["nearby_objects_after"][0]
        self.assertEqual("ainet.obj", after_object["object"])
        self.assertEqual(0x280, after_object["bss_size"])
        self.assertEqual(".data", after_object["sections"][0]["name"])
        self.assertEqual(2, after_object["sections"][0]["order"])
        raw_boundary = next(item for item in packing["boundaries"] if item["name"] == "candidate_raw_end")
        self.assertEqual(".bss", raw_boundary["containing_section"]["name"])
        self.assertEqual(0x80, raw_boundary["slack_before_within_containing"])
        self.assertEqual(0x200, raw_boundary["slack_after_within_containing"])

    def test_candidate_boundary_contribution_summary_groups_nearby_symbols_by_object(self) -> None:
        sections = (
            final_data_diff.MapSection(3, 0x70, 0x100, ".data", "DATA"),
            final_data_diff.MapSection(3, 0x180, 0x280, ".bss", "DATA"),
        )
        symbols = (
            final_data_diff.MapSymbol(3, 0x150, "_g_initialized_end", 0x4C6150, "zVideo.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0x170, "___defaultmatherr", 0x4C6170, "MSVCRT:merr.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0x180, "_g_bss_start", 0x4C6180, "ainet.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0x190, "_g_after_bss_start", 0x4C6190, "ainet.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0x3F0, "_g_virtual_tail", 0x4C63F0, "tail.obj", "Publics by Value"),
        )
        object_rows = (
            final_data_diff.ObjectContribution(
                "ainet.obj",
                data_size=0x10,
                bss_size=0x280,
                sections=(
                    {"name": ".data", "size": 0x10, "order": 2},
                    {"name": ".bss", "size": 0x280, "order": 5},
                ),
            ),
            final_data_diff.ObjectContribution(
                "tail.obj",
                data_size=0,
                bss_size=0x40,
                sections=({"name": ".bss", "size": 0x40, "order": 6},),
            ),
        )

        summary = final_data_diff.candidate_boundary_contribution_summary(
            map_sections=sections,
            segment_symbols=symbols,
            candidate_section=final_data_diff.SectionFacts(".data", 0xC600, 0x400, 0x200, 0x200, 0x200),
            data_section=sections[0],
            bss_boundary=0x180,
            object_rows=object_rows,
            limit=4,
        )

        self.assertTrue(summary["available"])
        self.assertEqual(0x170, summary["summary"]["data_end_offset"])
        self.assertEqual(0x180, summary["summary"]["bss_start_offset"])
        self.assertEqual(0x200, summary["summary"]["candidate_raw_end_offset"])
        self.assertEqual(0x400, summary["summary"]["candidate_virtual_end_offset"])
        bss_boundary = next(item for item in summary["boundaries"] if item["name"] == "bss_start")
        self.assertEqual(".data", bss_boundary["preceding_section"]["name"])
        self.assertEqual(".bss", bss_boundary["following_section"]["name"])
        provider_object = bss_boundary["objects_before"][1]
        self.assertEqual("MSVCRT:merr.obj", provider_object["object"])
        self.assertEqual("library-member", provider_object["object_origin"]["kind"])
        self.assertTrue(provider_object["object_origin"]["provider_candidate"])
        after_object = bss_boundary["objects_after"][0]
        self.assertEqual("ainet.obj", after_object["object"])
        self.assertEqual(0x280, after_object["bss_size"])
        self.assertEqual(2, len(after_object["symbols"]))
        self.assertEqual("_g_bss_start", after_object["symbols"][0]["symbol"])

    def test_candidate_initialized_data_thresholds_report_raw_aligned_crossings(self) -> None:
        sections = (
            final_data_diff.MapSection(3, 0x70, 0x100, ".data", "DATA"),
            final_data_diff.MapSection(3, 0x180, 0x300, ".bss", "DATA"),
        )
        symbols = (
            final_data_diff.MapSymbol(3, 0x150, "_g_initialized_end", 0x4C6150, "zVideo.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0x180, "_g_bss_start", 0x4C6180, "ainet.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0x410, "_g_bss_threshold_tail", 0x4C6410, "ainet.obj", "Publics by Value"),
        )
        object_rows = (
            final_data_diff.ObjectContribution(
                "zVideo.obj",
                data_size=0x540,
                bss_size=0,
                sections=({"name": ".data", "size": 0x540, "order": 1},),
            ),
            final_data_diff.ObjectContribution(
                "ainet.obj",
                data_size=0x10,
                bss_size=0x300,
                sections=(
                    {"name": ".data", "size": 0x10, "order": 2},
                    {"name": ".bss", "size": 0x300, "order": 5},
                ),
            ),
        )

        thresholds = final_data_diff.candidate_initialized_data_thresholds(
            map_sections=sections,
            segment_symbols=symbols,
            reference_section=final_data_diff.SectionFacts(".data", 0xC600, 0x800, 0x600, 0x200, 0x200),
            candidate_section=final_data_diff.SectionFacts(".data", 0xC600, 0x480, 0x200, 0x200, 0x280),
            data_section=sections[0],
            bss_boundary=0x180,
            object_rows=object_rows,
            candidate_file_alignment=0x200,
            limit=4,
        )

        self.assertTrue(thresholds["available"])
        summary = thresholds["summary"]
        self.assertEqual(0x170, summary["candidate_initialized_data_end_offset"])
        self.assertEqual(0x180, summary["bss_start_offset"])
        self.assertEqual(0x200, summary["candidate_raw_end_offset"])
        self.assertEqual(0x600, summary["reference_raw_end_offset"])
        self.assertEqual(0x200, summary["candidate_file_alignment"])
        rows = thresholds["thresholds"]
        self.assertEqual((0x400, 0x600), tuple(row["target_raw_end_offset"] for row in rows))
        self.assertEqual(0x200, rows[0]["previous_raw_end_offset"])
        self.assertEqual(0x91, rows[0]["bytes_needed_from_data_end"])
        self.assertEqual(0x290, rows[0]["bytes_to_fill_target_raw_end"])
        self.assertEqual(0x200, rows[0]["bytes_needed_from_current_raw_end"])
        self.assertFalse(rows[0]["reaches_reference_raw_end"])
        self.assertEqual(".bss", rows[0]["containing_section"]["name"])
        self.assertEqual("ainet.obj", rows[0]["nearby_objects_after"][0]["object"])
        self.assertEqual(0x300, rows[0]["nearby_objects_after"][0]["bss_size"])
        self.assertEqual(0x291, rows[1]["bytes_needed_from_data_end"])
        self.assertEqual(0x400, rows[1]["bytes_needed_from_current_raw_end"])
        self.assertTrue(rows[1]["matches_reference_raw_end"])
        self.assertTrue(rows[1]["reaches_reference_raw_end"])

    def test_candidate_initialized_data_thresholds_unavailable_without_map_data(self) -> None:
        thresholds = final_data_diff.candidate_initialized_data_thresholds(
            map_sections=(),
            segment_symbols=(),
            reference_section=final_data_diff.SectionFacts(".data", 0xC600, 0x800, 0x600, 0x200, 0x200),
            candidate_section=final_data_diff.SectionFacts(".data", 0xC600, 0x480, 0x200, 0x200, 0x280),
            data_section=None,
            bss_boundary=None,
            object_rows=(),
            candidate_file_alignment=0x200,
            limit=4,
        )

        self.assertFalse(thresholds["available"])
        self.assertIn("candidate map .data section not found", thresholds["reason"])

    def test_cli_writes_json_and_returns_strict_drift(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            reference = root / "reference.exe"
            candidate = root / "candidate.exe"
            map_path = root / "candidate.map"
            rsp_path = root / "link.rsp"
            manifest_dir = root / "manifests"
            json_out = root / "report.json"
            progress = root / "progress.json"
            manifest_dir.mkdir()
            progress.write_text(json.dumps({"schema_version": 1, "revision": 9}), encoding="utf-8")
            reference.write_bytes(minimal_pe(".data", rva=0x2000, virtual_size=0x500, raw_size=0x200))
            candidate.write_bytes(minimal_pe(".data", rva=0x3000, virtual_size=0x480, raw_size=0x240))
            map_path.write_text(
                "\n".join(
                    [
                        " Start         Length     Name                   Class",
                        " 0003:00000000 00000240H .data                   DATA",
                        " 0003:00000240 00000240H .bss                    DATA",
                        "",
                        "  Address         Publics by Value              Rva+Base     Lib:Object",
                        " 0003:00000200       _g_boundary                00403200     sample.obj",
                    ]
                ),
                encoding="utf-8",
            )
            rsp_path.write_text('"missing.obj"\n', encoding="utf-8")
            (manifest_dir / "sample.json").write_text(
                json.dumps(
                    {
                        "name": "sample_data",
                        "description": "synthetic data manifest",
                        "source_filename": "sample.cpp",
                        "source_from": "src/sample.cpp",
                        "data_symbols": [
                            {
                                "address": "0x402000",
                                "symbol": "_g_missing",
                                "name": "g_missing",
                                "byte_length": 4,
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )
            stdout = io.StringIO()
            stderr = io.StringIO()
            with mock.patch.object(final_data_diff, "bn_coverage", return_value=final_data_diff.BnCoverage(available=False, error="mocked")):
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = final_data_diff.main(
                        [
                            "--reference",
                            str(reference),
                            "--candidate",
                            str(candidate),
                            "--map",
                            str(map_path),
                            "--link-rsp",
                            str(rsp_path),
                            "--manifest-dir",
                            str(manifest_dir),
                            "--progress",
                            str(progress),
                            "--json-out",
                            str(json_out),
                            "--limit",
                            "5",
                            "--strict",
                        ]
                    )

            self.assertEqual(1, rc)
            self.assertEqual("", stderr.getvalue())
            self.assertIn("section_deltas:", stdout.getvalue())
            self.assertIn("candidate_boundary_packing:", stdout.getvalue())
            self.assertIn("candidate_boundary_contribution_summary:", stdout.getvalue())
            self.assertIn("candidate_initialized_data_thresholds:", stdout.getvalue())
            self.assertIn("candidate_object_subsection_attribution:", stdout.getvalue())
            payload = json.loads(json_out.read_text(encoding="utf-8"))
            self.assertEqual(3, payload["report_version"])
            self.assertEqual("final-data-report", payload["kind"])
            self.assertEqual("failed", payload["result"])
            self.assertEqual("recoil", payload["binary"])
            self.assertEqual("recoil:section:.data", payload["output_section_id"])
            self.assertEqual(".data", payload["section"])
            self.assertEqual(0x40, next(item["delta"] for item in payload["deltas"] if item["field"] == "raw_size"))
            self.assertIn("candidate_initialized_data_thresholds", payload)
            thresholds = payload["candidate_initialized_data_thresholds"]
            self.assertEqual(0x240, thresholds["summary"]["candidate_raw_end_offset"])
            self.assertEqual(0x200, thresholds["summary"]["candidate_file_alignment"])
            self.assertEqual(0x400, thresholds["thresholds"][0]["target_raw_end_offset"])
            self.assertIn("candidate_boundary_contribution_summary", payload)
            self.assertEqual(0x240, payload["candidate_boundary_contribution_summary"]["summary"]["candidate_raw_end_offset"])
            self.assertIn("candidate_boundary_packing", payload)
            self.assertEqual(0x240, payload["candidate_boundary_packing"]["summary"]["candidate_raw_end_offset"])
            self.assertIn("candidate_object_subsection_attribution", payload)
            self.assertEqual(".data", payload["output_section"]["name"])
            self.assertFalse(payload["raw_byte_comparison"]["equal"])
            self.assertGreater(payload["raw_byte_comparison"]["mismatch_count"], 0)
            self.assertEqual(
                {"path", "present", "size"}, set(payload["artifacts"]["candidate"])
            )
            self.assertEqual({"path", "present", "size"}, set(payload["artifacts"]["map"]))
            self.assertEqual(1, len(payload["artifacts"]["manifests"]))
            self.assertTrue(payload["storage_contributions"])
            self.assertTrue(all(item["output_section_id"] == "recoil:section:.data" for item in payload["storage_contributions"]))
            self.assertIsNone(payload["owner_correlations"])
            subsection = payload["candidate_object_subsection_attribution"]
            self.assertTrue(subsection["available"])
            self.assertEqual([0x121, 0x321, 0x521], subsection["summary"]["threshold_bytes_past_candidate_initialized_data_end"])
            self.assertEqual("sample.obj", subsection["objects"][0]["object"])
            self.assertEqual("authored-object", subsection["objects"][0]["provenance_kind"])
            self.assertIn("does not prove source ownership", subsection["limitations"][0])

    def test_raw_shrink_report_attributes_reference_tail_and_candidate_boundary(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            reference = root / "reference.exe"
            candidate = root / "candidate.exe"
            map_path = root / "candidate.map"
            rsp_path = root / "link.rsp"
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            reference_image = bytearray(minimal_pe(".data", rva=0x2000, virtual_size=0x400, raw_size=0x200))
            reference_tail = bytearray(0x80)
            reference_tail[0x05:0x10] = b"HELLO-TAIL!"
            reference_tail[0x7F] = 0x41
            reference_image[0x200 + 0x180 : 0x200 + 0x200] = reference_tail
            reference.write_bytes(bytes(reference_image))
            candidate.write_bytes(minimal_pe(".data", rva=0x3000, virtual_size=0x400, raw_size=0x180))
            map_path.write_text(
                "\n".join(
                    [
                        " Start         Length     Name                   Class",
                        " 0003:00000000 00000170H .data                   DATA",
                        " 0003:00000180 00000280H .bss                    DATA",
                        "",
                        "  Address         Publics by Value              Rva+Base     Lib:Object",
                        " 0003:00000160       _g_initialized_end         00403160     zVideo.obj",
                        " 0003:00000180       _g_bss_start               00403180     ainet.obj",
                        " 0003:00000190       _g_after_bss_start         00403190     ainet.obj",
                    ]
                ),
                encoding="utf-8",
            )
            rsp_path.write_text('"missing.obj"\n', encoding="utf-8")
            (manifest_dir / "sample_tail_data.json").write_text(
                json.dumps(
                    {
                        "name": "sample_tail_data",
                        "description": "synthetic tail data manifest",
                        "source_filename": "sample_tail.cpp",
                        "source_from": "src/sample_tail.cpp",
                        "data_symbols": [
                            {
                                "address": "0x402188",
                                "symbol": "_g_tail_manifest",
                                "name": "g_tail_manifest",
                                "byte_length": 0x20,
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )
            ref_section = final_data_diff.SectionFacts(".data", 0x2000, 0x400, 0x200, 0x200, 0x200)
            tail_item = final_data_diff.data_item_tail_record(
                final_data_diff.DataItem(
                    address="0x402190",
                    start=0x402190,
                    size=0x20,
                    name="??_R0?AVtype_info@@@8",
                    data_type="type_info",
                    section=".data",
                ),
                ref_section,
            )
            bn = final_data_diff.BnCoverage(
                available=True,
                item_count=1,
                section_item_count=1,
                manifest_covered_count=0,
                raw_tail_items=(tail_item,),
            )
            args = final_data_diff.build_parser().parse_args(
                [
                    "--reference",
                    str(reference),
                    "--candidate",
                    str(candidate),
                    "--map",
                    str(map_path),
                    "--link-rsp",
                    str(rsp_path),
                    "--manifest-dir",
                    str(manifest_dir),
                    "--limit",
                    "6",
                ]
            )

            with mock.patch.object(final_data_diff, "bn_coverage", return_value=bn) as mocked_bn:
                report = final_data_diff.build_report(args)

            attribution = report.raw_tail_attribution
            self.assertTrue(attribution["available"])
            self.assertEqual("reference-raw-tail-missing-from-candidate", attribution["mode"])
            self.assertEqual(-0x80, attribution["raw_size_delta"])
            self.assertEqual("0x402180", attribution["reference_tail_window"]["start"])
            self.assertEqual("0x402200", attribution["reference_tail_window"]["end"])
            self.assertEqual("raw-backed", attribution["reference_tail_window"]["backing"])
            self.assertEqual("zero-fill-bss", attribution["candidate_corresponding_window"]["backing"])
            self.assertEqual(0x170, attribution["candidate_map_boundaries"]["data_section_end_offset"])
            self.assertEqual(0x180, attribution["candidate_map_boundaries"]["bss_start_offset"])
            byte_summary = attribution["reference_tail_byte_summary"]
            self.assertTrue(byte_summary["available"])
            self.assertEqual(0x80, byte_summary["file_backed_byte_count"])
            self.assertEqual(12, byte_summary["nonzero_byte_count"])
            self.assertEqual(0x74, byte_summary["zero_byte_count"])
            self.assertEqual(0x185, byte_summary["first_nonzero_offset"])
            self.assertEqual(0x1FF, byte_summary["last_nonzero_offset"])
            self.assertEqual("0x402185", byte_summary["first_nonzero_address"])
            self.assertEqual("0x4021ff", byte_summary["last_nonzero_address"])
            self.assertEqual("HELLO-TAIL!", byte_summary["printable_runs"][0]["text"])
            self.assertIn("does not prove source ownership", byte_summary["limitations"][0])
            candidate_byte_summary = attribution["candidate_corresponding_window_byte_summary"]
            self.assertFalse(candidate_byte_summary["available"])
            self.assertEqual("zero-fill-bss", candidate_byte_summary["backing"])
            self.assertEqual(0, candidate_byte_summary["file_backed_byte_count"])
            self.assertIn("no file-backed bytes were read", candidate_byte_summary["limitations"][-1])
            self.assertEqual("provider-candidate:rtti/type_info", attribution["reference_tail_bn_items"][0]["classification"])
            self.assertFalse(attribution["reference_tail_bn_items"][0]["manifest_unmatched"])
            self.assertEqual(1, attribution["reference_tail_bn_items"][0]["manifest_match_count"])
            manifest_match = attribution["reference_tail_bn_items"][0]["manifest_matches"][0]
            self.assertEqual("sample_tail_data", manifest_match["target"])
            self.assertEqual("g_tail_manifest", manifest_match["name"])
            self.assertEqual("0x402190..0x4021a8", manifest_match["overlap_range"])
            self.assertEqual(0x18, manifest_match["overlap_size"])
            self.assertEqual("src/sample_tail.cpp", manifest_match["source_from"])
            source_summary = attribution["reference_tail_source_summary"]
            self.assertEqual(1, source_summary["source_group_count"])
            self.assertEqual(1, source_summary["manifest_backed_item_count"])
            self.assertEqual(0x20, source_summary["manifest_backed_item_bytes"])
            self.assertEqual(0x18, source_summary["matched_overlap_bytes"])
            self.assertEqual(0, source_summary["unmatched_item_count"])
            source_group = source_summary["source_groups"][0]
            self.assertEqual("src/sample_tail.cpp", source_group["source_from"])
            self.assertEqual("sample_tail.cpp", source_group["source_filename"])
            self.assertEqual("sample_tail_data", source_group["target"])
            self.assertEqual(1, source_group["item_count"])
            self.assertEqual(0x20, source_group["item_bytes"])
            self.assertEqual(0x18, source_group["matched_overlap_bytes"])
            self.assertEqual(("0x402190..0x4021b0",), source_group["ranges"])
            self.assertEqual(("g_tail_manifest",), source_group["names"])
            self.assertEqual("after-bss-start", attribution["candidate_boundary_map_symbols"][-1]["relation_to_bss_start"])
            mocked_bn.assert_called_once()
            self.assertEqual(0x402180, mocked_bn.call_args.kwargs["raw_tail_start"])
            self.assertEqual(0x402200, mocked_bn.call_args.kwargs["raw_tail_end"])

            payload = json.loads(json.dumps(final_data_diff.asdict(report)))
            self.assertIn("raw_tail_attribution", payload)
            self.assertEqual(
                "HELLO-TAIL!",
                payload["raw_tail_attribution"]["reference_tail_byte_summary"]["printable_runs"][0]["text"],
            )
            self.assertEqual(
                "sample_tail_data",
                payload["raw_tail_attribution"]["reference_tail_bn_items"][0]["manifest_matches"][0]["target"],
            )
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                final_data_diff.print_report(report, limit=6)
            self.assertIn("raw_tail_attribution:", stdout.getvalue())
            self.assertIn("reference_tail_source_summary:", stdout.getvalue())
            self.assertIn("source=src/sample_tail.cpp target=sample_tail_data", stdout.getvalue())
            self.assertIn("reference_tail_byte_summary:", stdout.getvalue())
            self.assertIn("nonzero=12", stdout.getvalue())
            self.assertIn("candidate_corresponding_window_byte_summary:", stdout.getvalue())
            self.assertIn("reference_tail_bn_items:", stdout.getvalue())
            self.assertIn("manifest_matches=sample_tail_data:g_tail_manifest", stdout.getvalue())

    def test_bn_coverage_preserves_tail_items_beyond_display_limit(self) -> None:
        ref_section = final_data_diff.SectionFacts(".data", 0x2000, 0x100, 0x80, 0x200, 0x80)
        items = [
            final_data_diff.DataItem(
                address=f"0x{0x402000 + index * 0x10:x}",
                start=0x402000 + index * 0x10,
                size=0x10,
                name=f"g_tail_{index}",
                data_type="char[16]",
                section=".data",
            )
            for index in range(4)
        ]

        with mock.patch.object(final_data_diff, "BinaryNinjaBridge", return_value=object()):
            with mock.patch.object(final_data_diff, "fetch_data_items", return_value=items):
                coverage = final_data_diff.bn_coverage(
                    bridge_url="http://127.0.0.1:0",
                    section_name=".data",
                    reference_section=ref_section,
                    manifest_covered_addresses=set(),
                    raw_tail_start=0x402000,
                    raw_tail_end=0x402080,
                    limit=2,
                )

        self.assertTrue(coverage.available)
        self.assertEqual(4, len(coverage.raw_tail_items))
        self.assertEqual("g_tail_3", coverage.raw_tail_items[-1]["name"])
        self.assertEqual("0x402030..0x402040", coverage.raw_tail_items[-1]["tail_overlap_range"])

    def test_reference_tail_summary_reports_manifest_uncovered_spans(self) -> None:
        item = {
            "address": "0x402000",
            "end_address": "0x402040",
            "size": 0x40,
            "range": "0x402000..0x402040",
            "name": "g_partial_tail",
            "type": "char[64]",
            "section": ".data",
            "backing": "raw-backed",
            "classification": "",
            "tail_overlap_start": "0x402000",
            "tail_overlap_end": "0x402040",
            "tail_overlap_range": "0x402000..0x402040",
            "tail_overlap_size": 0x40,
        }
        manifest_range = final_data_diff.ManifestDataSymbolRange(
            manifest="tools/vc5_verify_targets/sample.json",
            target="sample_tail_data",
            name="g_manifest_slice",
            symbol="_g_manifest_slice",
            address="0x402010",
            start=0x402010,
            end=0x402020,
            byte_length=0x10,
            source_from="src/sample_tail.cpp",
            source_filename="sample_tail.cpp",
        )

        annotated = final_data_diff.annotate_tail_manifest_matches(
            (item,),
            (manifest_range,),
            limit=4,
        )
        summary = final_data_diff.reference_tail_source_summary(annotated, limit=4)

        self.assertFalse(annotated[0]["manifest_unmatched"])
        self.assertFalse(annotated[0]["manifest_fully_covered"])
        self.assertEqual(0x30, annotated[0]["manifest_uncovered_bytes"])
        self.assertEqual(2, annotated[0]["manifest_uncovered_range_count"])
        self.assertEqual(0, summary["unmatched_item_count"])
        self.assertEqual(2, summary["manifest_uncovered_span_count"])
        self.assertEqual(0x30, summary["manifest_uncovered_bytes"])
        self.assertEqual("0x402000..0x402010", summary["manifest_uncovered_spans"][0]["range"])
        self.assertEqual("0x402020..0x402040", summary["manifest_uncovered_spans"][1]["range"])

        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            final_data_diff.print_reference_tail_source_summary(summary, limit=4)
        self.assertIn("manifest-uncovered spans=2 bytes=48", stdout.getvalue())

    def test_virtual_shrink_report_attributes_reference_tail_and_candidate_virtual_boundary(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            reference = root / "reference.exe"
            candidate = root / "candidate.exe"
            map_path = root / "candidate.map"
            rsp_path = root / "link.rsp"
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            reference.write_bytes(minimal_pe(".data", rva=0x2000, virtual_size=0x500, raw_size=0x200))
            candidate.write_bytes(minimal_pe(".data", rva=0x3000, virtual_size=0x480, raw_size=0x200))
            map_path.write_text(
                "\n".join(
                    [
                        " Start         Length     Name                   Class",
                        " 0003:00000000 00000200H .data                   DATA",
                        " 0003:00000200 00000280H .bss                    DATA",
                        "",
                        "  Address         Publics by Value              Rva+Base     Lib:Object",
                        " 0003:00000460       _g_OptCatalogRuntimeWorld  00403460     OptCatalog.obj",
                        " 0003:00000470       ___onexitbegin             00403470     LIBC:crt0dat.obj",
                        " 0003:00000480       ___onexitend               00403480     LIBC:crt0dat.obj",
                    ]
                ),
                encoding="utf-8",
            )
            rsp_path.write_text('"missing.obj"\n', encoding="utf-8")
            ref_section = final_data_diff.SectionFacts(".data", 0x2000, 0x500, 0x200, 0x200, 0x300)
            tail_item = final_data_diff.data_item_tail_record(
                final_data_diff.DataItem(
                    address="0x402460",
                    start=0x402460,
                    size=0xA0,
                    name="g_zVideo_OverwriteQueueBase",
                    data_type="uint8_t[160]",
                    section=".data",
                ),
                ref_section,
            )
            bn = final_data_diff.BnCoverage(
                available=True,
                item_count=1,
                section_item_count=1,
                manifest_covered_count=0,
                virtual_tail_items=(tail_item,),
            )
            args = final_data_diff.build_parser().parse_args(
                [
                    "--reference",
                    str(reference),
                    "--candidate",
                    str(candidate),
                    "--map",
                    str(map_path),
                    "--link-rsp",
                    str(rsp_path),
                    "--manifest-dir",
                    str(manifest_dir),
                    "--limit",
                    "6",
                ]
            )

            with mock.patch.object(final_data_diff, "bn_coverage", return_value=bn) as mocked_bn:
                report = final_data_diff.build_report(args)

            attribution = report.virtual_tail_attribution
            self.assertTrue(attribution["available"])
            self.assertEqual("reference-virtual-tail-missing-from-candidate", attribution["mode"])
            self.assertEqual(-0x80, attribution["virtual_size_delta"])
            self.assertEqual("0x402480", attribution["reference_tail_window"]["start"])
            self.assertEqual("0x402500", attribution["reference_tail_window"]["end"])
            self.assertEqual("zero-fill-tail", attribution["reference_tail_window"]["backing"])
            self.assertEqual("0x403480", attribution["candidate_corresponding_window"]["start"])
            self.assertEqual("outside-candidate-section", attribution["candidate_corresponding_window"]["backing"])
            self.assertFalse(attribution["reference_tail_byte_summary"]["available"])
            self.assertEqual("zero-fill-tail", attribution["reference_tail_byte_summary"]["backing"])
            self.assertFalse(attribution["candidate_corresponding_window_byte_summary"]["available"])
            self.assertEqual(
                "outside-candidate-section",
                attribution["candidate_corresponding_window_byte_summary"]["backing"],
            )
            self.assertEqual(0x480, attribution["candidate_map_boundaries"]["candidate_virtual_end_offset"])
            self.assertEqual("g_zVideo_OverwriteQueueBase", attribution["reference_tail_bn_items"][0]["name"])
            self.assertTrue(attribution["reference_tail_bn_items"][0]["manifest_unmatched"])
            self.assertEqual(0, attribution["reference_tail_bn_items"][0]["manifest_match_count"])
            source_summary = attribution["reference_tail_source_summary"]
            self.assertEqual(0, source_summary["source_group_count"])
            self.assertEqual(1, source_summary["unmatched_item_count"])
            self.assertEqual(0xA0, source_summary["unmatched_bytes"])
            self.assertEqual("g_zVideo_OverwriteQueueBase", source_summary["unmatched_items"][0]["name"])
            self.assertEqual("after-bss-start", attribution["candidate_boundary_map_symbols"][-1]["relation_to_bss_start"])
            mocked_bn.assert_called_once()
            self.assertEqual(0x402480, mocked_bn.call_args.kwargs["virtual_tail_start"])
            self.assertEqual(0x402500, mocked_bn.call_args.kwargs["virtual_tail_end"])

            payload = json.loads(json.dumps(final_data_diff.asdict(report)))
            self.assertIn("virtual_tail_attribution", payload)
            self.assertTrue(payload["virtual_tail_attribution"]["reference_tail_bn_items"][0]["manifest_unmatched"])
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                final_data_diff.print_report(report, limit=6)
            self.assertIn("virtual_tail_attribution:", stdout.getvalue())
            self.assertIn("reference_tail_source_summary:", stdout.getvalue())
            self.assertIn("unmatched items=1 bytes=160", stdout.getvalue())
            self.assertIn("reference_virtual_tail_bn_items:", stdout.getvalue())
            self.assertIn("manifest_matches=unmatched", stdout.getvalue())

    def test_manifest_coverage_uses_data_symbol_regex_for_candidate_map_symbols(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            map_path = root / "candidate.map"
            map_path.write_text(
                "\n".join(
                    [
                        " Start         Length     Name                   Class",
                        " 0003:00000000 00000240H .data                   DATA",
                        "",
                        "  Address         Publics by Value              Rva+Base     Lib:Object",
                        " 0003:00009ca4       ??_C@_08DLCI@MSL_LOCK?$AA@ 004c4ca4     zTurret.obj",
                    ]
                ),
                encoding="utf-8",
            )
            (manifest_dir / "zturret_config_literals_data.json").write_text(
                json.dumps(
                    {
                        "name": "zturret_config_literals_data",
                        "description": "synthetic zTurret data manifest",
                        "source_filename": "zTurret.cpp",
                        "source_from": "src/GameZRecoil/zTurret/zTurret.cpp",
                        "data_symbols": [
                            {
                                "address": "0x4dd09c",
                                "symbol_regex": r"\?\?_C@_08[A-Z0-9]+@MSL_LOCK\?\$AA@",
                                "name": "g_zTurret_ConfigKey_MslLock",
                                "byte_length": 9,
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )

            _sections, symbols = final_data_diff.parse_map(map_path)
            coverage = final_data_diff.manifest_coverage(
                manifest_dir,
                symbols,
                final_data_diff.SectionFacts(".data", 0xDD000, 0x1000, 0x800, 0x200, 0x800),
            )

        self.assertEqual(1, coverage.symbol_name_matches)
        self.assertEqual(0, coverage.exact_address_matches)
        self.assertEqual(1, len(coverage.issues))
        self.assertEqual("candidate-address-drift", coverage.issues[0].kind)
        self.assertEqual("??_C@_08DLCI@MSL_LOCK?$AA@", coverage.issues[0].symbol)
        self.assertEqual("0x4c4ca4", coverage.issues[0].candidate_address)
        self.assertNotEqual("missing-candidate-map-symbol", coverage.issues[0].kind)

    def test_owner_correlation_expands_s_tier_rows_through_affected_owner(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            owners = root / "SOURCE_OWNERS.json"
            write_ledger(
                owners,
                owner_record(
                    "sample.owner",
                    kind="data-owner",
                    anchors=("0x4da0ec",),
                    data=(("0x4da0ec", "g_A"), ("0x4da0f0", "g_B"), ("0x4da100", "g_C")),
                    tiers={"0x4da0ec": "S", "0x4da0f0": "S", "0x4da100": "B"},
                    gates={
                        "boundary": "accepted",
                        "source": "accepted",
                        "data": "accepted",
                        "functional": "none",
                        "linkage": "accepted",
                        "byte": "accepted",
                    },
                    name="Sample data",
                    section="data.sample",
                    source_paths=("src/sample.cpp",),
                    address_metadata={
                        address: {
                            "name": name,
                            "source_path": "src/sample.cpp",
                            "target": "sample_data",
                            "section": ".data",
                            "size": "4",
                            "type": "char[4]",
                        }
                        for address, name in (
                            ("0x4da0ec", "g_A"),
                            ("0x4da0f0", "g_B"),
                            ("0x4da100", "g_C"),
                        )
                    },
                ),
            )
            report = final_data_diff.FinalDataReport(
                reference="support/Recoil.exe",
                candidate="build/vc5-final/Recoil.exe",
                map="build/vc5-final/Recoil.map",
                link_rsp="build/vc5-final/rsp/link.rsp",
                section=".data",
                reference_section=final_data_diff.SectionFacts(".data", 0xDA000, 0x100, 0x80, 0x200, 0x80),
                candidate_section=final_data_diff.SectionFacts(".data", 0xBE000, 0xE0, 0x90, 0x200, 0x50),
                deltas=(
                    final_data_diff.SectionDelta("rva", 0xDA000, 0xBE000, -0x1C000),
                    final_data_diff.SectionDelta("virtual_size", 0x100, 0xE0, -0x20),
                    final_data_diff.SectionDelta("raw_size", 0x80, 0x90, 0x10),
                    final_data_diff.SectionDelta("zero_fill_tail", 0x80, 0x50, -0x30),
                ),
                rankings={},
                map_sections=(),
                manifest_coverage=final_data_diff.ManifestCoverage(
                    manifest_count=1,
                    data_symbol_count=1,
                    in_reference_section=1,
                    symbol_name_matches=1,
                    exact_address_matches=0,
                    issues=(
                        final_data_diff.ManifestIssue(
                            manifest="tools/vc5_verify_targets/sample.json",
                            target="sample_data",
                            name="g_A",
                            symbol="_g_A",
                            address="0x4da0ec",
                            byte_length=4,
                            kind="candidate-address-drift",
                            detail="candidate map places symbol elsewhere",
                        ),
                    ),
                ),
                bn_coverage=final_data_diff.BnCoverage(available=False, error="mocked"),
                classifications=(),
            )

            actions = final_data_diff.correlate_owners(
                report,
                progress_path=owners,
            )

        self.assertEqual(("sample.owner",), actions.affected_owner_ids)
        self.assertEqual(("0x4da0ec", "0x4da0f0"), actions.affected_owner_addresses)
        self.assertEqual(("sample.owner",), actions.diagnostic_owner_ids)
        self.assertEqual(("0x4da0ec",), actions.diagnostic_owner_addresses)
        self.assertEqual(1, actions.diagnostic_owner_issue_count)
        self.assertEqual(1, actions.direct_s_tier_issue_count)
        self.assertFalse(hasattr(actions, "commands"))
        self.assertFalse(hasattr(actions, "evidence"))
        self.assertIn("non-authoritative", actions.limitations[0])
        self.assertIn("never mutates", actions.limitations[1])

    def test_owner_correlation_uses_primary_data_owner_not_anchor_only_owner(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            owners = root / "SOURCE_OWNERS.json"
            accepted_gates = {
                "boundary": "accepted",
                "source": "accepted",
                "data": "accepted",
                "functional": "none",
                "linkage": "accepted",
                "byte": "accepted",
            }
            write_ledger(
                owners,
                owner_record(
                    "anchor.only",
                    kind="data-owner",
                    anchors=("0x4da000",),
                    gates=accepted_gates,
                    name="Anchor only",
                    section="data.sample",
                ),
                owner_record(
                    "primary.owner",
                    kind="data-owner",
                    data=(("0x4da000", "g_A"),),
                    tiers={"0x4da000": "S"},
                    gates=accepted_gates,
                    name="Primary owner",
                    section="data.sample",
                    source_paths=("src/sample.cpp",),
                    address_metadata={
                        "0x4da000": {
                            "name": "g_A",
                            "source_path": "src/sample.cpp",
                            "target": "sample_data",
                            "section": ".data",
                            "size": "4",
                            "type": "int",
                        }
                    },
                ),
            )
            report = self._report_with_direct_manifest_issues(1)

            actions = final_data_diff.correlate_owners(
                report,
                progress_path=owners,
            )

        self.assertEqual(("primary.owner",), actions.affected_owner_ids)
        self.assertEqual(("primary.owner",), actions.diagnostic_owner_ids)
        self.assertEqual(("0x4da000",), actions.affected_owner_addresses)
        self.assertNotIn("anchor.only", actions.diagnostic_owner_ids)

    def test_owner_correlation_reports_non_s_owner_entry_diagnostics_without_actions(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            owners = root / "SOURCE_OWNERS.json"
            write_ledger(
                owners,
                owner_record(
                    "sample.owner",
                    kind="data-owner",
                    anchors=("0x4da100",),
                    data=(("0x4da100", "g_C"),),
                    tiers={"0x4da100": "B"},
                    gates={
                        "boundary": "accepted",
                        "source": "accepted",
                        "data": "accepted",
                        "functional": "none",
                        "linkage": "accepted",
                        "byte": "pending",
                    },
                    name="Sample data",
                    section="data.sample",
                    source_paths=("src/sample.cpp",),
                    address_metadata={
                        "0x4da100": {
                            "name": "g_C",
                            "source_path": "src/sample.cpp",
                            "target": "sample_data",
                            "section": ".data",
                            "size": "4",
                            "type": "char[4]",
                        }
                    },
                ),
            )
            report = final_data_diff.FinalDataReport(
                reference="support/Recoil.exe",
                candidate="build/vc5-final/Recoil.exe",
                map="build/vc5-final/Recoil.map",
                link_rsp="build/vc5-final/rsp/link.rsp",
                section=".data",
                reference_section=final_data_diff.SectionFacts(".data", 0xDA000, 0x100, 0x80, 0x200, 0x80),
                candidate_section=final_data_diff.SectionFacts(".data", 0xBE000, 0xE0, 0x90, 0x200, 0x50),
                deltas=(
                    final_data_diff.SectionDelta("rva", 0xDA000, 0xBE000, -0x1C000),
                ),
                rankings={},
                map_sections=(),
                manifest_coverage=final_data_diff.ManifestCoverage(
                    manifest_count=1,
                    data_symbol_count=1,
                    in_reference_section=1,
                    symbol_name_matches=1,
                    exact_address_matches=0,
                    issues=(
                        final_data_diff.ManifestIssue(
                            manifest="tools/vc5_verify_targets/sample.json",
                            target="sample_data",
                            name="g_C",
                            symbol="_g_C",
                            address="0x4da100",
                            byte_length=4,
                            kind="candidate-address-drift",
                            detail="candidate map places symbol elsewhere",
                        ),
                    ),
                ),
                bn_coverage=final_data_diff.BnCoverage(available=False, error="mocked"),
                classifications=(),
            )

            actions = final_data_diff.correlate_owners(
                report,
                progress_path=owners,
            )

        self.assertEqual(1, actions.direct_issue_count)
        self.assertEqual(1, actions.direct_issue_detail_count)
        self.assertEqual(0, actions.direct_issue_truncated_count)
        self.assertFalse(actions.direct_issues_truncated)
        self.assertEqual(1, len(actions.direct_issues))
        self.assertEqual("0x4da100", actions.direct_issues[0].address)
        self.assertEqual("0x4da100..0x4da104", actions.direct_issues[0].range)
        self.assertEqual("candidate-address-drift", actions.direct_issues[0].issue_kind)
        self.assertEqual("candidate map places symbol elsewhere", actions.direct_issues[0].reason)
        self.assertEqual("diagnostic-data-owner-entry-entry", actions.direct_issues[0].status)
        self.assertEqual("0x4da100", actions.direct_issues[0].owner_address)
        self.assertEqual("g_C", actions.direct_issues[0].owner_name)
        self.assertEqual("sample_data", actions.direct_issues[0].owner_target)
        self.assertEqual("data.sample", actions.direct_issues[0].owner_group)
        self.assertEqual("B", actions.direct_issues[0].owner_tier)
        self.assertEqual("0x4da100", actions.direct_issues[0].reference_address)
        self.assertEqual("0x4da100..0x4da104", actions.direct_issues[0].reference_range)
        self.assertEqual("", actions.direct_issues[0].candidate_address)
        self.assertEqual("", actions.direct_issues[0].candidate_range)
        self.assertEqual(("sample.owner",), actions.direct_issues[0].owner_ids)
        self.assertIn("bounded sample", actions.direct_issues_scope)
        self.assertEqual(1, actions.diagnostic_owner_issue_count)
        self.assertEqual(("sample.owner",), actions.diagnostic_owner_ids)
        self.assertEqual(("0x4da100",), actions.diagnostic_owner_addresses)
        self.assertEqual(0, actions.direct_s_tier_issue_count)
        self.assertEqual((), actions.affected_owner_ids)
        self.assertEqual((), actions.affected_owner_addresses)
        self.assertFalse(hasattr(actions, "commands"))

    def test_owner_correlation_uses_default_direct_issue_detail_limit(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            owners = self._write_empty_owners(Path(tmp))

            actions = final_data_diff.correlate_owners(
                self._report_with_direct_manifest_issues(252),
                progress_path=owners,
            )

        self.assertEqual(252, actions.direct_issue_count)
        self.assertEqual(250, actions.direct_issue_detail_limit)
        self.assertEqual(250, actions.direct_issue_detail_count)
        self.assertEqual(2, actions.direct_issue_truncated_count)
        self.assertTrue(actions.direct_issues_truncated)
        self.assertEqual(250, len(actions.direct_issues))
        self.assertIn("bounded sample", actions.direct_issues_scope)
        self.assertEqual("0x4da000", actions.direct_issues[0].address)
        self.assertEqual("0x4da3e4", actions.direct_issues[-1].address)

    def test_owner_correlation_honors_custom_direct_issue_detail_limit(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            owners = self._write_empty_owners(Path(tmp))

            actions = final_data_diff.correlate_owners(
                self._report_with_direct_manifest_issues(5),
                progress_path=owners,
                direct_issue_detail_limit=2,
            )

        self.assertEqual(5, actions.direct_issue_count)
        self.assertEqual(2, actions.direct_issue_detail_limit)
        self.assertEqual(2, actions.direct_issue_detail_count)
        self.assertEqual(3, actions.direct_issue_truncated_count)
        self.assertTrue(actions.direct_issues_truncated)
        self.assertEqual(("0x4da000", "0x4da004"), tuple(issue.address for issue in actions.direct_issues))
        self.assertIn("bounded sample", actions.direct_issues_scope)

    def test_owner_correlation_allows_unlimited_direct_issue_details(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            owners = self._write_empty_owners(Path(tmp))

            actions = final_data_diff.correlate_owners(
                self._report_with_direct_manifest_issues(5),
                progress_path=owners,
                direct_issue_detail_limit=0,
            )

        self.assertEqual(5, actions.direct_issue_count)
        self.assertEqual(0, actions.direct_issue_detail_limit)
        self.assertEqual(5, actions.direct_issue_detail_count)
        self.assertEqual(0, actions.direct_issue_truncated_count)
        self.assertFalse(actions.direct_issues_truncated)
        self.assertEqual(
            ("0x4da000", "0x4da004", "0x4da008", "0x4da00c", "0x4da010"),
            tuple(issue.address for issue in actions.direct_issues),
        )
        self.assertIn("all manifest direct issues", actions.direct_issues_scope)
        self.assertNotIn("bounded sample", actions.direct_issues_scope)

    def test_final_data_peer_work_unit_api_is_removed(self) -> None:
        self.assertFalse(hasattr(final_data_diff, "FinalDataWorkUnit"))
        self.assertFalse(hasattr(final_data_diff, "final_data_work_unit"))
        self.assertFalse(hasattr(final_data_diff, "final_data_work_unit_from_report"))

    def test_retired_owner_action_output_returns_controlled_migration_error(self) -> None:
        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr):
            rc = final_data_diff.main(["--owner-actions-json", "legacy.json"])
        self.assertEqual(2, rc)
        self.assertIn("is retired", stderr.getvalue())
        self.assertIn("read-only live", stderr.getvalue())

    def test_public_help_omits_retired_action_batch_options(self) -> None:
        help_text = final_data_diff.build_parser().format_help()
        self.assertIn("--include-owners", help_text)
        self.assertIn("non-authoritative correlations", help_text)
        self.assertNotIn("--owner-actions-json", help_text)
        self.assertNotIn("--action-chunk-size", help_text)

    def test_malformed_progress_returns_controlled_two(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            progress = Path(tmp) / "progress.json"
            progress.write_text("[]", encoding="utf-8")
            stderr = io.StringIO()
            with mock.patch.object(final_data_diff, "build_report", return_value=self._report_with_direct_manifest_issues(1)):
                with contextlib.redirect_stderr(stderr):
                    rc = final_data_diff.main(["--progress", str(progress), "--include-owners"])
        self.assertEqual(2, rc)
        self.assertIn("expected JSON object", stderr.getvalue())


if __name__ == "__main__":
    unittest.main()
