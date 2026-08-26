from __future__ import annotations

from pathlib import Path
import struct
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands import final_data_diff  # noqa: E402


def _coff_name_bytes(name: str, string_offsets: dict[str, int], strings: bytearray) -> bytes:
    encoded = name.encode("utf-8")
    if len(encoded) <= 8:
        return encoded.ljust(8, b"\0")
    offset = string_offsets.get(name)
    if offset is None:
        offset = 4 + len(strings)
        string_offsets[name] = offset
        strings.extend(encoded + b"\0")
    return struct.pack("<II", 0, offset)


def write_test_coff(
    path: Path,
    *,
    sections: tuple[tuple[str, int, int], ...],
    symbols: tuple[tuple[str, int, int, int], ...],
) -> None:
    strings = bytearray()
    string_offsets: dict[str, int] = {}
    for name, _value, _section_number, _storage_class in symbols:
        _coff_name_bytes(name, string_offsets, strings)

    header_size = 20
    section_table_size = 40 * len(sections)
    raw_offset = header_size + section_table_size
    section_headers = bytearray()
    raw_data = bytearray()
    section_raw_offsets: list[int] = []
    for _name, size, characteristics in sections:
        if characteristics & final_data_diff.IMAGE_SCN_CNT_UNINITIALIZED_DATA:
            section_raw_offsets.append(0)
        else:
            section_raw_offsets.append(raw_offset + len(raw_data))
            raw_data.extend(b"\0" * size)

    symbol_table_offset = raw_offset + len(raw_data)
    string_table = struct.pack("<I", 4 + len(strings)) + bytes(strings)
    for index, (name, size, characteristics) in enumerate(sections):
        section_headers.extend(
            struct.pack(
                "<8sIIIIIIHHI",
                _coff_name_bytes(name, string_offsets, strings),
                0,
                0,
                size,
                section_raw_offsets[index],
                0,
                0,
                0,
                0,
                characteristics,
            )
        )

    symbol_rows = bytearray()
    for name, value, section_number, storage_class in symbols:
        symbol_rows.extend(
            _coff_name_bytes(name, string_offsets, strings)
            + struct.pack("<IhHBB", value, section_number, 0, storage_class, 0)
        )
    string_table = struct.pack("<I", 4 + len(strings)) + bytes(strings)
    header = struct.pack(
        "<HHIIIHH",
        0x14C,
        len(sections),
        0,
        symbol_table_offset,
        len(symbols),
        0,
        0,
    )
    path.write_bytes(bytes(header + section_headers + raw_data + symbol_rows + string_table))


class FinalDataThresholdAttributionTests(unittest.TestCase):
    def test_threshold_attribution_reports_boundary_offsets_and_bytes_needed(self) -> None:
        sections = (
            final_data_diff.MapSection(3, 0x70, 0xB470, ".data", "DATA"),
            final_data_diff.MapSection(3, 0xB4E0, 0x1200, ".bss", "DATA"),
        )
        symbols = (
            final_data_diff.MapSymbol(3, 0xB4C0, "??_R0?AVtype_info@@@8", 0x4C94C0, "MSVCRT:ti_inst.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0xB4E0, "_g_AiNetBssStart", 0x4C94E0, "ainet.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0xB7F0, "_g_BeforeB800", 0x4C97F0, "Briefing.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0xB800, "_g_AtB800", 0x4C9800, "HudUiMessageBoxDialog.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0xB9F0, "_g_BeforeBA00", 0x4C99F0, "GameNet.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0xBA00, "_g_AtBA00", 0x4C9A00, "zVideo.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0xBBF0, "_g_BeforeBC00", 0x4C9BF0, "tail.obj", "Publics by Value"),
        )
        object_rows = (
            final_data_diff.ObjectContribution(
                "MSVCRT:ti_inst.obj",
                data_size=0x20,
                bss_size=0,
                sections=({"name": ".data", "size": 0x20, "order": 1},),
            ),
            final_data_diff.ObjectContribution(
                "ainet.obj",
                data_size=0,
                bss_size=0x300,
                sections=({"name": ".bss", "size": 0x300, "order": 2},),
            ),
            final_data_diff.ObjectContribution(
                "HudUiMessageBoxDialog.obj",
                data_size=0,
                bss_size=0x80,
                sections=({"name": ".bss", "size": 0x80, "order": 5},),
            ),
            final_data_diff.ObjectContribution(
                "zVideo.obj",
                data_size=0,
                bss_size=0x200,
                sections=({"name": ".bss", "size": 0x200, "order": 8},),
            ),
        )
        reference_section = final_data_diff.SectionFacts(".data", 0xD1000, 0x2A0000, 0xBC00, 0x200, 0x294400)
        candidate_section = final_data_diff.SectionFacts(".data", 0xBE000, 0x29C248, 0xB600, 0x200, 0x290C48)
        thresholds = final_data_diff.candidate_initialized_data_thresholds(
            map_sections=sections,
            segment_symbols=symbols,
            reference_section=reference_section,
            candidate_section=candidate_section,
            data_section=sections[0],
            bss_boundary=0xB4E0,
            object_rows=object_rows,
            candidate_file_alignment=0x200,
            limit=6,
        )

        attribution = final_data_diff.candidate_threshold_attribution(
            thresholds=thresholds,
            map_sections=sections,
            segment_symbols=symbols,
            reference_section=reference_section,
            candidate_section=candidate_section,
            data_section=sections[0],
            bss_boundary=0xB4E0,
            object_rows=object_rows,
            candidate_file_alignment=0x200,
            limit=6,
        )

        self.assertTrue(attribution["available"])
        summary = attribution["summary"]
        self.assertEqual(0xB4E0, summary["candidate_initialized_data_end_offset"])
        self.assertEqual("0x4c94e0", summary["candidate_initialized_data_end_address"])
        self.assertEqual(0xB4E0, summary["bss_start_offset"])
        self.assertEqual("0x4c94e0", summary["bss_start_address"])
        self.assertEqual(0xB600, summary["candidate_raw_end_offset"])
        self.assertEqual(0xBC00, summary["reference_raw_end_offset"])
        rows = attribution["raw_alignment_thresholds"]
        self.assertEqual((0xB800, 0xBA00, 0xBC00), tuple(row["target_raw_end_offset"] for row in rows))
        self.assertEqual((0x121, 0x321, 0x521), tuple(row["bytes_needed_from_data_end"] for row in rows))
        self.assertTrue(rows[-1]["matches_reference_raw_end"])

        points = {point["name"]: point for point in attribution["attribution_points"]}
        self.assertEqual("bss", points["candidate_initialized_data_end"]["map_evidence_kind"])
        before_data_end = points["candidate_initialized_data_end"]["objects_before"][0]
        self.assertEqual("MSVCRT:ti_inst.obj", before_data_end["object"])
        self.assertEqual("provider/library", before_data_end["map_evidence_kind"])
        after_data_end = points["candidate_initialized_data_end"]["objects_after"][0]
        self.assertEqual("ainet.obj", after_data_end["object"])
        self.assertEqual("bss", after_data_end["map_evidence_kind"])
        threshold = points["threshold_0xb800"]
        self.assertEqual("HudUiMessageBoxDialog.obj", threshold["objects_after"][0]["object"])
        self.assertEqual("bss", threshold["objects_after"][0]["map_evidence_kind"])
        self.assertIn("does not prove source ownership", attribution["limitations"][0])

    def test_threshold_attribution_is_unavailable_without_candidate_map_data(self) -> None:
        attribution = final_data_diff.candidate_threshold_attribution(
            thresholds={"available": False, "thresholds": ()},
            map_sections=(),
            segment_symbols=(),
            reference_section=final_data_diff.SectionFacts(".data", 0xD1000, 0x1000, 0x800, 0x200, 0x800),
            candidate_section=final_data_diff.SectionFacts(".data", 0xBE000, 0x800, 0x600, 0x200, 0x200),
            data_section=None,
            bss_boundary=None,
            object_rows=(),
            candidate_file_alignment=0x200,
            limit=4,
        )

        self.assertFalse(attribution["available"])
        self.assertIn("candidate map .data section not found", attribution["reason"])
        self.assertIn("does not build, relink, generate probes", attribution["limitations"][1])

    def test_object_subsection_attribution_reports_focus_objects_and_threshold_sizes(self) -> None:
        sections = (
            final_data_diff.MapSection(3, 0x70, 0xB470, ".data", "DATA"),
            final_data_diff.MapSection(3, 0xB4E0, 0x1200, ".bss", "DATA"),
        )
        symbols = (
            final_data_diff.MapSymbol(3, 0xB4C0, "??_R0?AVtype_info@@@8", 0x4C94C0, "MSVCRT:ti_inst.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0xB4E0, "_g_AiNetBssStart", 0x4C94E0, "ainet.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0xB7F0, "_g_BeforeB800", 0x4C97F0, "Briefing.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0xB800, "_g_AtB800", 0x4C9800, "HudUiMessageBoxDialog.obj", "Publics by Value"),
            final_data_diff.MapSymbol(3, 0xBA00, "_g_AtBA00", 0x4C9A00, "zVideo.obj", "Publics by Value"),
        )
        object_rows = (
            final_data_diff.ObjectContribution(
                "MSVCRT:ti_inst.obj",
                data_size=0x20,
                bss_size=0,
                sections=({"name": ".data", "size": 0x20, "order": 1, "kind": "initialized-data"},),
                object_path="lib/MSVCRT/ti_inst.obj",
            ),
            final_data_diff.ObjectContribution(
                "ainet.obj",
                data_size=0,
                bss_size=0x300,
                sections=({"name": ".bss", "size": 0x300, "order": 2, "kind": "bss"},),
                object_path="build/obj/ainet.obj",
            ),
            final_data_diff.ObjectContribution(
                "HudUiMessageBoxDialog.obj",
                data_size=0,
                bss_size=0x80,
                sections=({"name": ".bss", "size": 0x80, "order": 5, "kind": "bss"},),
            ),
            final_data_diff.ObjectContribution(
                "zVideo.obj",
                data_size=0,
                bss_size=0x200,
                sections=({"name": ".bss", "size": 0x200, "order": 8, "kind": "bss"},),
            ),
        )
        reference_section = final_data_diff.SectionFacts(".data", 0xD1000, 0x2A0000, 0xBC00, 0x200, 0x294400)
        candidate_section = final_data_diff.SectionFacts(".data", 0xBE000, 0x29C248, 0xB600, 0x200, 0x290C48)
        thresholds = final_data_diff.candidate_initialized_data_thresholds(
            map_sections=sections,
            segment_symbols=symbols,
            reference_section=reference_section,
            candidate_section=candidate_section,
            data_section=sections[0],
            bss_boundary=0xB4E0,
            object_rows=object_rows,
            candidate_file_alignment=0x200,
            limit=6,
        )

        attribution = final_data_diff.candidate_object_subsection_attribution(
            thresholds=thresholds,
            map_sections=sections,
            segment_symbols=symbols,
            reference_section=reference_section,
            candidate_section=candidate_section,
            data_section=sections[0],
            bss_boundary=0xB4E0,
            object_rows=object_rows,
            candidate_file_alignment=0x200,
            limit=6,
        )

        self.assertTrue(attribution["available"])
        self.assertEqual((0x121, 0x321, 0x521), attribution["summary"]["threshold_bytes_past_candidate_initialized_data_end"])
        objects = {item["object"]: item for item in attribution["objects"]}
        self.assertIn("focus_object", objects["zVideo.obj"]["selection_reasons"])
        self.assertEqual("provider/library", objects["MSVCRT:ti_inst.obj"]["provenance_kind"])
        self.assertEqual("provider/library", objects["MSVCRT:ti_inst.obj"]["map_evidence_kind"])
        self.assertEqual("lib/MSVCRT/ti_inst.obj", objects["MSVCRT:ti_inst.obj"]["object_path"])
        self.assertEqual("bss", objects["ainet.obj"]["sections"][0]["kind"])
        self.assertTrue(objects["ainet.obj"]["can_satisfy_thresholds_by_size"][0]["bss_size_satisfies"])
        self.assertFalse(objects["ainet.obj"]["can_satisfy_thresholds_by_size"][1]["bss_size_satisfies"])
        self.assertFalse(objects["HudUiMessageBoxDialog.obj"]["can_satisfy_thresholds_by_size"][0]["initialized_or_bss_size_satisfies"])
        self.assertIn("Briefing.obj", objects)
        self.assertEqual(0, objects["Briefing.obj"]["initialized_data_contribution_size"])
        self.assertIn("does not prove source ownership", attribution["limitations"][0])

    def test_object_subsection_attribution_reports_selected_coff_symbol_sections(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data_characteristics = final_data_diff.IMAGE_SCN_CNT_INITIALIZED_DATA
            bss_characteristics = final_data_diff.IMAGE_SCN_CNT_UNINITIALIZED_DATA
            write_test_coff(
                root / "zinterp_parse.obj",
                sections=((".data", 0x140, data_characteristics),),
                symbols=(("_g_zInterp_UnresolvedFloatDefaults", 0x20, 1, 2),),
            )
            write_test_coff(
                root / "zVideo.obj",
                sections=((".data", 0x80, data_characteristics), (".bss", 0x200, bss_characteristics)),
                symbols=(
                    ("_g_zVideo_PaletteOpenFailedFormat", 0x10, 1, 2),
                    ("_g_zVideo_OverwriteQueueBase", 0x40, 2, 3),
                ),
            )
            write_test_coff(
                root / "player.obj",
                sections=((".bss", 0x300, bss_characteristics),),
                symbols=(
                    ("_g_Player_AivParentDir", 0x20, 1, 2),
                    ("_g_Player_LocalFxOffsetWorldPtr", 0x30, 1, 2),
                ),
            )
            sections = (
                final_data_diff.MapSection(3, 0x100, 0x180, ".data", "DATA"),
                final_data_diff.MapSection(3, 0x280, 0x500, ".bss", "DATA"),
            )
            symbols = (
                final_data_diff.MapSymbol(3, 0x120, "_g_zInterp_UnresolvedFloatDefaults", 0x4BE120, "zinterp_parse.obj", "Publics by Value"),
                final_data_diff.MapSymbol(3, 0x170, "_g_zVideo_PaletteOpenFailedFormat", 0x4BE170, "zVideo.obj", "Publics by Value"),
                final_data_diff.MapSymbol(3, 0x180, "_g_missingFromZVideoObj", 0x4BE180, "zVideo.obj", "Publics by Value"),
                final_data_diff.MapSymbol(3, 0x290, "_g_zVideo_OverwriteQueueBase", 0x4BE290, "zVideo.obj", "Publics by Value"),
                final_data_diff.MapSymbol(3, 0x300, "_g_Player_AivParentDir", 0x4BE300, "player.obj", "Publics by Value"),
                final_data_diff.MapSymbol(3, 0x310, "_g_Player_LocalFxOffsetWorldPtr", 0x4BE310, "player.obj", "Publics by Value"),
            )
            object_rows = (
                final_data_diff.ObjectContribution(
                    "zinterp_parse.obj",
                    data_size=0x140,
                    bss_size=0,
                    sections=({"name": ".data", "size": 0x140, "order": 0, "kind": "initialized-data"},),
                    object_path=str(root / "zinterp_parse.obj"),
                ),
                final_data_diff.ObjectContribution(
                    "zVideo.obj",
                    data_size=0x80,
                    bss_size=0x200,
                    sections=(
                        {"name": ".data", "size": 0x80, "order": 0, "kind": "initialized-data"},
                        {"name": ".bss", "size": 0x200, "order": 1, "kind": "bss"},
                    ),
                    object_path=str(root / "zVideo.obj"),
                ),
                final_data_diff.ObjectContribution(
                    "player.obj",
                    data_size=0,
                    bss_size=0x300,
                    sections=({"name": ".bss", "size": 0x300, "order": 0, "kind": "bss"},),
                    object_path=str(root / "player.obj"),
                ),
            )
            manifest = final_data_diff.ManifestCoverage(
                manifest_count=4,
                data_symbol_count=4,
                in_reference_section=4,
                symbol_name_matches=4,
                exact_address_matches=0,
                issues=(
                    final_data_diff.ManifestIssue(
                        manifest="tools/vc5_verify_targets/zinterp_parse.json",
                        target="zinterp_unresolved_float_defaults",
                        name="g_zInterp_UnresolvedFloatDefaults",
                        symbol="_g_zInterp_UnresolvedFloatDefaults",
                        address="0x4d5954",
                        byte_length=0xFC,
                        kind="candidate-address-drift",
                        detail="candidate map places symbol elsewhere",
                        candidate_address="0x4be120",
                    ),
                    final_data_diff.ManifestIssue(
                        manifest="tools/vc5_verify_targets/zvideo.json",
                        target="zvideo_palette_open_failed_format",
                        name="g_zVideo_PaletteOpenFailedFormat",
                        symbol="symbol_regex=_g_zVideo_PaletteOpenFailed.*",
                        address="0x4d6100",
                        byte_length=4,
                        kind="candidate-address-drift",
                        detail="candidate map places symbol elsewhere",
                        candidate_address="0x4be170",
                    ),
                    final_data_diff.ManifestIssue(
                        manifest="tools/vc5_verify_targets/zvideo.json",
                        target="zvideo_missing",
                        name="g_missingFromZVideoObj",
                        symbol="_g_missingFromZVideoObj",
                        address="0x4d6104",
                        byte_length=4,
                        kind="candidate-address-drift",
                        detail="candidate map places symbol elsewhere",
                        candidate_address="0x4be180",
                    ),
                    final_data_diff.ManifestIssue(
                        manifest="tools/vc5_verify_targets/zvideo.json",
                        target="zvideo_overwrite_queue",
                        name="g_zVideo_OverwriteQueueBase",
                        symbol="_g_zVideo_OverwriteQueueBase",
                        address="0x4be520",
                        byte_length=4,
                        kind="candidate-address-drift",
                        detail="candidate map places symbol elsewhere",
                        candidate_address="0x4be290",
                    ),
                ),
            )
            virtual_tail = {
                "available": True,
                "reference_tail_bn_items": (
                    {
                        "manifest_matches": (
                            {"symbol": "_g_Player_AivParentDir"},
                            {"symbol": "_g_Player_LocalFxOffsetWorldPtr"},
                        )
                    },
                ),
            }
            thresholds = {
                "summary": {"candidate_initialized_data_end_offset": 0x280},
                "thresholds": ({"target_raw_end_offset": 0x400},),
            }

            attribution = final_data_diff.candidate_object_subsection_attribution(
                thresholds=thresholds,
                map_sections=sections,
                segment_symbols=symbols,
                reference_section=final_data_diff.SectionFacts(".data", 0xBE000, 0x800, 0x600, 0x200, 0x200),
                candidate_section=final_data_diff.SectionFacts(".data", 0xBE000, 0x700, 0x500, 0x200, 0x200),
                data_section=sections[0],
                bss_boundary=0x280,
                object_rows=object_rows,
                candidate_file_alignment=0x200,
                limit=1,
                manifest_coverage=manifest,
                virtual_tail_attribution=virtual_tail,
            )

        objects = {item["object"]: item for item in attribution["objects"]}
        zinterp_symbols = {row["symbol"]: row for row in objects["zinterp_parse.obj"]["selected_coff_symbols"]}
        self.assertEqual("matched", zinterp_symbols["_g_zInterp_UnresolvedFloatDefaults"]["lookup_status"])
        self.assertEqual(".data", zinterp_symbols["_g_zInterp_UnresolvedFloatDefaults"]["coff_section_name"])
        self.assertEqual(0, zinterp_symbols["_g_zInterp_UnresolvedFloatDefaults"]["coff_section_order"])
        self.assertEqual("initialized-data", zinterp_symbols["_g_zInterp_UnresolvedFloatDefaults"]["coff_section_kind"])
        self.assertEqual(0x20, zinterp_symbols["_g_zInterp_UnresolvedFloatDefaults"]["coff_value"])

        zvideo_symbols = {row["symbol"]: row for row in objects["zVideo.obj"]["selected_coff_symbols"]}
        self.assertEqual(".data", zvideo_symbols["_g_zVideo_PaletteOpenFailedFormat"]["coff_section_name"])
        self.assertEqual("initialized-data", zvideo_symbols["_g_zVideo_PaletteOpenFailedFormat"]["coff_section_kind"])
        self.assertEqual(".bss", zvideo_symbols["_g_zVideo_OverwriteQueueBase"]["coff_section_name"])
        self.assertEqual("bss", zvideo_symbols["_g_zVideo_OverwriteQueueBase"]["coff_section_kind"])
        self.assertEqual(1, zvideo_symbols["_g_zVideo_OverwriteQueueBase"]["coff_section_order"])
        self.assertEqual(3, zvideo_symbols["_g_zVideo_OverwriteQueueBase"]["storage_class"])
        overwrite_correlations = zvideo_symbols["_g_zVideo_OverwriteQueueBase"]["manifest_issue_correlations"]
        self.assertEqual(1, len(overwrite_correlations))
        self.assertEqual("0x4be520", overwrite_correlations[0]["reference_address"])
        self.assertEqual(0x520, overwrite_correlations[0]["reference_offset"])
        self.assertEqual(0x290, overwrite_correlations[0]["candidate_map_offset"])
        self.assertEqual(-0x290, overwrite_correlations[0]["candidate_offset_delta_from_reference"])
        self.assertTrue(overwrite_correlations[0]["reference_in_raw_tail_gap"])
        self.assertEqual("after", overwrite_correlations[0]["reference_relation_to_candidate_raw_end"])
        self.assertEqual("after", overwrite_correlations[0]["candidate_relation_to_bss_start"])
        self.assertEqual("before", overwrite_correlations[0]["candidate_relation_to_raw_end"])
        self.assertEqual("coff-symbol-missing", zvideo_symbols["_g_missingFromZVideoObj"]["lookup_status"])

        player_symbols = {row["symbol"]: row for row in objects["player.obj"]["selected_coff_symbols"]}
        self.assertEqual("matched", player_symbols["_g_Player_AivParentDir"]["lookup_status"])
        self.assertEqual("bss", player_symbols["_g_Player_AivParentDir"]["coff_section_kind"])
        self.assertEqual("matched", player_symbols["_g_Player_LocalFxOffsetWorldPtr"]["lookup_status"])
        self.assertIn("virtual_tail_manifest_match", player_symbols["_g_Player_LocalFxOffsetWorldPtr"]["selection_reasons"])
        self.assertEqual(1, len(objects["player.obj"]["map_symbols"]))

    def test_object_trace_reports_named_object_link_coff_and_threshold_relationships(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data_characteristics = final_data_diff.IMAGE_SCN_CNT_INITIALIZED_DATA
            bss_characteristics = final_data_diff.IMAGE_SCN_CNT_UNINITIALIZED_DATA
            player_obj = root / "player.obj"
            ainet_obj = root / "ainet.obj"
            write_test_coff(
                player_obj,
                sections=((".data", 0x20, data_characteristics), (".bss", 0x300, bss_characteristics)),
                symbols=(("_g_Player_AivParentDir", 0x20, 2, 2),),
            )
            write_test_coff(
                ainet_obj,
                sections=((".bss", 0x321, bss_characteristics),),
                symbols=(("_g_AiNetBssStart", 0x40, 1, 3),),
            )
            sections = (
                final_data_diff.MapSection(3, 0x100, 0x180, ".data", "DATA"),
                final_data_diff.MapSection(3, 0x280, 0x500, ".bss", "DATA"),
            )
            symbols = (
                final_data_diff.MapSymbol(3, 0x290, "_g_AiNetBssStart", 0x4BE290, "ainet.obj", "Publics by Value"),
                final_data_diff.MapSymbol(3, 0x300, "_g_Player_AivParentDir", 0x4BE300, "player.obj", "Publics by Value"),
                final_data_diff.MapSymbol(3, 0x330, "??_R0?AVtype_info@@@8", 0x4BE330, "MSVCRT:ti_inst.obj", "Publics by Value"),
            )
            object_rows = (
                final_data_diff.ObjectContribution(
                    "player.obj",
                    data_size=0x20,
                    bss_size=0x300,
                    sections=(
                        {"name": ".data", "size": 0x20, "order": 0, "kind": "initialized-data", "characteristics": data_characteristics},
                        {"name": ".bss", "size": 0x300, "order": 1, "kind": "bss", "characteristics": bss_characteristics},
                    ),
                    object_path=str(player_obj),
                ),
                final_data_diff.ObjectContribution(
                    "ainet.obj",
                    data_size=0,
                    bss_size=0x321,
                    sections=({"name": ".bss", "size": 0x321, "order": 0, "kind": "bss", "characteristics": bss_characteristics},),
                    object_path=str(ainet_obj),
                ),
            )

            trace = final_data_diff.candidate_object_traces(
                requested_objects=final_data_diff.parse_trace_object_names(
                    ["player.obj,MSVCRT:ti_inst.obj"]
                ),
                map_sections=sections,
                segment_symbols=symbols,
                reference_section=final_data_diff.SectionFacts(".data", 0xD1000, 0xBC00, 0xBC00, 0x200, 0),
                candidate_section=final_data_diff.SectionFacts(".data", 0xBE000, 0x700, 0x500, 0x200, 0x200),
                data_section=sections[0],
                bss_boundary=0x280,
                object_rows=object_rows,
                object_paths=(player_obj, ainet_obj),
                limit=4,
            )

        self.assertTrue(trace["available"])
        objects = {item["object"]: item for item in trace["objects"]}
        player = objects["player.obj"]
        self.assertEqual(0, player["link_rsp_order"])
        self.assertEqual(str(player_obj), player["object_path"])
        self.assertEqual("read", player["coff_status"])
        self.assertEqual(2, len(player["coff_sections"]))
        self.assertEqual(2, player["coff_sections"][1]["index"])
        self.assertEqual("bss", player["coff_sections"][1]["kind"])
        self.assertEqual(0x300, player["map_contribution"]["bss_contribution_size"])
        player_symbol = player["selected_symbols"][0]
        self.assertEqual("_g_Player_AivParentDir", player_symbol["symbol"])
        self.assertEqual(0xBE300, player_symbol["map_rva"])
        self.assertEqual(".bss", player_symbol["coff_section_name"])
        self.assertEqual(0x20, player_symbol["coff_value"])
        self.assertEqual("after", player_symbol["relationships"]["candidate_data_end"]["relation"])
        self.assertEqual("before", player_symbol["relationships"]["candidate_raw_end"]["relation"])
        self.assertEqual("before", player_symbol["relationships"]["reference_raw_end"]["relation"])
        self.assertEqual(
            (0x121, 0x321, 0x521),
            player["threshold_relationships"]["threshold_bytes_past_candidate_data_end"],
        )

        provider = objects["MSVCRT:ti_inst.obj"]
        self.assertFalse(provider["found_in_link_rsp"])
        self.assertEqual("provider/library", provider["provenance_kind"])
        self.assertEqual("coff-object-path-unavailable", provider["coff_status"])
        self.assertEqual("provider/library", provider["map_contribution"]["map_evidence_kind"])
        self.assertEqual("coff-object-path-unavailable", provider["selected_symbols"][0]["lookup_status"])


if __name__ == "__main__":
    unittest.main()
