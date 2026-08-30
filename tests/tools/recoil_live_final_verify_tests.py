from __future__ import annotations

from copy import deepcopy
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.live_final_verify import (  # noqa: E402
    _validate_text_population,
    compare_images,
    run,
    semantic_projection,
)
from _recoil.lib.pe import data_directory, parse_pe_headers, rva_to_offset  # noqa: E402


def canonical_retail_reference() -> Path:
    return REPO_ROOT / "support" / "Recoil.exe"

class LiveFinalVerifyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.reference = canonical_retail_reference()
        self.reference_data = self.reference.read_bytes()
        self.headers = parse_pe_headers(self.reference_data, source=str(self.reference))
        self.catalog, self.map_rows, self.tracker = self.complete_catalog()

    def complete_catalog(
        self,
    ) -> tuple[dict[str, object], list[dict[str, object]], dict[str, object]]:
        sections: dict[str, object] = {}
        map_rows: list[dict[str, object]] = []
        symbols: dict[str, object] = {}
        blocks: dict[str, object] = {}
        storage: dict[str, object] = {}
        output_sections: dict[str, object] = {}
        for section in self.headers.sections:
            section_id = f"recoil:section:{section.name}"
            output_sections[section_id] = {
                "binary": "recoil",
                "name": section.name,
                "reference": {
                    "image_address": f"0x{self.headers.image_base + section.virtual_address:x}",
                    "virtual_size": section.virtual_size,
                    "raw_size": section.raw_size,
                },
            }
            entity: dict[str, object] = {
                "id": f"unit:{section.name}:complete",
                "file_start": 0,
                "file_end": section.raw_size,
                "virtual_start": 0,
                "virtual_end": section.virtual_size,
            }
            if section.name == ".text":
                symbol_id = "recoil:function:unit-complete-text"
                block_id = "recoil:block:unit-complete-text"
                entity.update(
                    {
                        "kind": "address-group",
                        "identities": [
                            {
                                "symbol_id": symbol_id,
                                "map_symbol": "unit_complete_text",
                                "object": "unit.obj",
                                "source_block_id": block_id,
                                "contribution_class": "authored",
                                "comdat": False,
                            }
                        ],
                        "relocations": [],
                    }
                )
                start = self.headers.image_base + section.virtual_address
                end = start + section.virtual_size
                symbols[symbol_id] = {
                    "binary": "recoil",
                    "kind": "function",
                    "pipeline_class": "authored",
                    "physical_block_id": block_id,
                    "output_section_id": section_id,
                    "extent_state": "known",
                    "address": f"0x{start:x}",
                    "end_exclusive": f"0x{end:x}",
                    "size": section.virtual_size,
                    "comdat": False,
                }
                blocks[block_id] = {
                    "binary": "recoil",
                    "start": f"0x{start:x}",
                    "end_exclusive": f"0x{end:x}",
                    "contribution_ids": [symbol_id],
                }
                map_rows.append(
                    {
                        "address": self.headers.image_base + section.virtual_address,
                        "symbol": "unit_complete_text",
                        "object": "unit.obj",
                        "is_function": True,
                    }
                )
            elif section.name in {".rdata", ".data"}:
                source_id = f"recoil:storage:unit:{section.name}"
                symbol_id = f"recoil:data:unit:{section.name}"
                entity.update(
                    {
                        "kind": "initialized-data",
                        "source_id": source_id,
                    }
                )
                start = self.headers.image_base + section.virtual_address
                end = start + section.virtual_size
                symbols[symbol_id] = {
                    "binary": "recoil",
                    "kind": "data",
                    "output_section_id": section_id,
                    "extent_state": "known",
                    "address": f"0x{start:x}",
                    "end_exclusive": f"0x{end:x}",
                    "size": section.virtual_size,
                    "storage_contribution_ids": [source_id],
                }
                storage[source_id] = {
                    "binary": "recoil",
                    "kind": "data-symbol",
                    "output_section_id": section_id,
                    "symbol_ids": [symbol_id],
                    "reference": {
                        "extent_state": "known",
                        "address": f"0x{start:x}",
                        "end_exclusive": f"0x{end:x}",
                    },
                }
            elif section.name == ".rsrc":
                entity["kind"] = "resource"
            elif section.name == ".reloc":
                entity["kind"] = "relocations"
            else:
                entity["kind"] = "section-payload"
            # Zero-length virtual/raw dimensions do not need a synthetic interval.
            if section.raw_size == 0:
                entity.pop("file_start")
                entity.pop("file_end")
            if section.virtual_size == 0:
                entity.pop("virtual_start")
                entity.pop("virtual_end")
            sections[section.name] = {"entities": [entity]}
        catalog = {
                "version": 1,
                "binary": "recoil",
                "directories": "exact-including-absence",
                "overlay": {"mode": "exact"},
                "sections": sections,
            }
        tracker = {
            "schema_version": 6,
            "revision": 1,
            "symbols": symbols,
            "physical_blocks": blocks,
            "storage_contributions": storage,
            "output_sections": output_sections,
        }
        return catalog, map_rows, tracker

    def compare_mutation(
        self,
        data: bytes,
        *,
        catalog: dict[str, object] | None = None,
        map_rows: list[dict[str, object]] | None = None,
        tracker: dict[str, object] | None = None,
    ) -> dict[str, object]:
        with tempfile.TemporaryDirectory() as temporary:
            candidate = Path(temporary) / "candidate.exe"
            candidate.write_bytes(data)
            return compare_images(
                candidate,
                self.reference,
                catalog=catalog or self.catalog,
                candidate_map_rows=map_rows or self.map_rows,
                tracker=tracker or self.tracker,
            )

    def test_timestamp_only_difference_passes_semantic_gate(self) -> None:
        data = bytearray(self.reference_data)
        timestamp_offset = self.headers.pe_offset + 8
        original = struct.unpack_from("<I", data, timestamp_offset)[0]
        struct.pack_into("<I", data, timestamp_offset, (original + 120) & 0xFFFFFFFF)
        report = self.compare_mutation(bytes(data))
        self.assertTrue(report["passed"], report["semantic_failures"])
        self.assertTrue(report["timestamp_is_diagnostic_only"])
        self.assertTrue(report["timestamp_only_raw_difference"])
        self.assertFalse(report["raw_file_equal_diagnostic"])
        self.assertNotEqual(report["candidate_timestamp"], report["retail_timestamp"])

    def test_no_historical_timestamp_is_forced(self) -> None:
        data = bytearray(self.reference_data)
        timestamp_offset = self.headers.pe_offset + 8
        struct.pack_into("<I", data, timestamp_offset, 0)
        report = self.compare_mutation(bytes(data))
        self.assertTrue(report["passed"], report["semantic_failures"])
        self.assertEqual(0, report["candidate_timestamp"])
        self.assertTrue(report["timestamp_is_diagnostic_only"])

    def test_text_and_data_mutations_fail_typed_section_semantics(self) -> None:
        for section_name in (".text", ".data"):
            with self.subTest(section=section_name):
                section = next(row for row in self.headers.sections if row.name == section_name)
                data = bytearray(self.reference_data)
                data[section.raw_pointer] ^= 1
                report = self.compare_mutation(bytes(data))
                self.assertFalse(report["passed"])
                self.assertTrue(
                    any(f"section {section_name} payload differs" in row for row in report["semantic_failures"])
                )

    def test_resource_payload_mutation_fails_resource_semantics(self) -> None:
        resource = data_directory(self.headers, 2)
        self.assertIsNotNone(resource.file_offset)
        data = bytearray(self.reference_data)
        data[int(resource.file_offset)] ^= 1
        report = self.compare_mutation(bytes(data))
        self.assertFalse(report["passed"])
        self.assertFalse(report["resource_payload_equal"])
        self.assertTrue(any("data directory resource" in row for row in report["semantic_failures"]))

    def test_import_identity_mutation_fails_import_semantics(self) -> None:
        imports = data_directory(self.headers, 1)
        self.assertIsNotNone(imports.file_offset)
        descriptor_offset = int(imports.file_offset)
        name_rva = struct.unpack_from("<I", self.reference_data, descriptor_offset + 12)[0]
        name_offset = rva_to_offset(name_rva, self.headers.sections)
        self.assertIsNotNone(name_offset)
        data = bytearray(self.reference_data)
        data[int(name_offset)] = ord("X") if data[int(name_offset)] != ord("X") else ord("Y")
        report = self.compare_mutation(bytes(data))
        self.assertFalse(report["passed"])
        self.assertFalse(report["imports_equal"])
        self.assertTrue(any(row["path"].startswith("imports") for row in report["semantic_differences"]))

    def test_relocation_target_mutation_fails(self) -> None:
        text = next(row for row in self.headers.sections if row.name == ".text")
        first_entry = self.reference_data.find(b"\xe8", text.raw_pointer, text.raw_pointer + text.raw_size - 4)
        self.assertGreaterEqual(first_entry, text.raw_pointer)
        operand_offset = first_entry + 1
        operand_rva = text.virtual_address + (operand_offset - text.raw_pointer)
        displacement = struct.unpack_from("<i", self.reference_data, operand_offset)[0]
        resolved_rva = operand_rva + 4 + displacement
        catalog = deepcopy(self.catalog)
        catalog["sections"][".text"]["entities"][0]["relocations"] = [
            {
                "offset": operand_offset - text.raw_pointer,
                "width": 4,
                "type": "rel32",
                "target_rva": resolved_rva,
                "addend": 0,
            }
        ]
        data = bytearray(self.reference_data)
        struct.pack_into("<i", data, operand_offset, displacement + 1)
        report = self.compare_mutation(bytes(data), catalog=catalog)
        self.assertFalse(report["passed"])
        self.assertFalse(report["resolved_operands_equal"])
        self.assertTrue(any("operand" in row for row in report["semantic_failures"]))

    def test_section_rva_and_raw_pointer_mutations_fail(self) -> None:
        optional_header_size = struct.unpack_from("<H", self.reference_data, self.headers.pe_offset + 20)[0]
        section_table = self.headers.pe_offset + 24 + optional_header_size
        for label, field_offset in (("virtual_address", 12), ("raw_pointer", 20)):
            with self.subTest(field=label):
                data = bytearray(self.reference_data)
                original = struct.unpack_from("<I", data, section_table + field_offset)[0]
                struct.pack_into("<I", data, section_table + field_offset, original + 0x200)
                report = self.compare_mutation(bytes(data))
                self.assertFalse(report["passed"])
                self.assertTrue(
                    any(row["path"] == f"sections[0].{label}" for row in report["semantic_differences"]),
                    report["semantic_differences"],
                )

    def test_governed_section_padding_byte_mutation_fails(self) -> None:
        section = next(
            row for row in self.headers.sections if row.raw_size > row.virtual_size
        )
        padding_offset = section.raw_pointer + section.virtual_size
        self.assertLess(padding_offset, section.raw_pointer + section.raw_size)
        data = bytearray(self.reference_data)
        data[padding_offset] ^= 1
        report = self.compare_mutation(bytes(data))
        self.assertFalse(report["passed"])
        self.assertTrue(
            any(f"section {section.name} payload differs" in row for row in report["semantic_failures"])
        )

    def test_raw_identity_is_diagnostic_and_report_has_no_content_summary(self) -> None:
        data = bytearray(self.reference_data)
        data[self.headers.pe_offset + 8] ^= 1
        report = self.compare_mutation(bytes(data))
        self.assertTrue(report["passed"], report["semantic_failures"])
        self.assertFalse(report["raw_file_equal_diagnostic"])

        def keys(value: object) -> list[str]:
            if isinstance(value, dict):
                return [str(key) for key in value] + [item for child in value.values() for item in keys(child)]
            if isinstance(value, list):
                return [item for child in value for item in keys(child)]
            return []

        forbidden = ("sha" + "256", "hex" + "di" + "gest", "mer" + "kle")
        self.assertFalse(any(any(token in key.casefold() for token in forbidden) for key in keys(report)))

    def test_unmodeled_range_fails_closed_even_when_candidate_is_retail(self) -> None:
        catalog = deepcopy(self.catalog)
        text = next(row for row in self.headers.sections if row.name == ".text")
        entity = catalog["sections"][".text"]["entities"][0]
        entity["file_end"] = text.raw_size - 1
        report = self.compare_mutation(self.reference_data, catalog=catalog)
        self.assertFalse(report["passed"])
        self.assertFalse(report["catalog_complete"])
        self.assertTrue(any("unmodeled range" in row for row in report["semantic_failures"]))

    def test_well_shaped_catalog_with_fake_tracker_identity_fails_closed(self) -> None:
        catalog = deepcopy(self.catalog)
        identity = catalog["sections"][".text"]["entities"][0]["identities"][0]
        identity["symbol_id"] = "recoil:function:well-shaped-but-fake"
        report = self.compare_mutation(self.reference_data, catalog=catalog)
        self.assertFalse(report["passed"])
        self.assertTrue(
            any("does not resolve to a recoil tracker symbol" in row for row in report["semantic_failures"])
        )

    def test_catalog_claims_must_match_tracker_block_comdat_and_data_extent(self) -> None:
        text_identity = self.catalog["sections"][".text"]["entities"][0]["identities"][0]
        text_symbol_id = text_identity["symbol_id"]
        block_id = text_identity["source_block_id"]
        data_source_id = self.catalog["sections"][".data"]["entities"][0]["source_id"]
        mutations = [
            (
                "COMDAT classification differs",
                lambda tracker: tracker["symbols"][text_symbol_id].update(comdat=True),
            ),
            (
                "contribution_class",
                lambda tracker: tracker["symbols"][text_symbol_id].update(
                    pipeline_class="non-authored"
                ),
            ),
            (
                "provider classification differs",
                lambda tracker: tracker["symbols"][text_symbol_id].update(provider="mfc42"),
            ),
            (
                "is not a contribution",
                lambda tracker: tracker["physical_blocks"][block_id].update(contribution_ids=[]),
            ),
            (
                "extent does not exactly match",
                lambda tracker: tracker["storage_contributions"][data_source_id]["reference"].update(
                    end_exclusive="0x1"
                ),
            ),
        ]
        for expected_failure, mutate in mutations:
            with self.subTest(expected_failure=expected_failure):
                tracker = deepcopy(self.tracker)
                mutate(tracker)
                report = self.compare_mutation(self.reference_data, tracker=tracker)
                self.assertFalse(report["passed"])
                self.assertTrue(
                    any(expected_failure in row for row in report["semantic_failures"]),
                    report["semantic_failures"],
                )

    def test_icf_catalog_winner_cross_resolves_to_every_tracker_alias(self) -> None:
        catalog = deepcopy(self.catalog)
        tracker = deepcopy(self.tracker)
        map_rows = deepcopy(self.map_rows)
        entity = catalog["sections"][".text"]["entities"][0]
        winner = entity["identities"][0]
        winner_id = winner["symbol_id"]
        alias_id = "recoil:function:unit-complete-text-alias"
        alias = deepcopy(winner)
        alias.update(symbol_id=alias_id, map_symbol="unit_complete_text_alias")
        entity["identities"].append(alias)
        entity["icf"] = {
            "winner_symbol_id": winner_id,
            "winner_symbol": winner["map_symbol"],
            "winner_status": "selected-winner",
        }
        winner_tracker = tracker["symbols"][winner_id]
        winner_tracker["icf_address_group"] = {
            "winner_identity_key": winner_id,
            "winner_status": "selected-winner",
        }
        tracker["symbols"][alias_id] = deepcopy(winner_tracker)
        block_id = winner["source_block_id"]
        tracker["physical_blocks"][block_id]["contribution_ids"].append(alias_id)
        map_rows.append(
            {
                "address": map_rows[0]["address"],
                "symbol": alias["map_symbol"],
                "object": alias["object"],
                "is_function": True,
            }
        )
        report = self.compare_mutation(
            self.reference_data,
            catalog=catalog,
            map_rows=map_rows,
            tracker=tracker,
        )
        self.assertTrue(report["passed"], report["semantic_failures"])

    def test_map_population_preserves_order_and_multiplicity(self) -> None:
        projection = semantic_projection(self.reference_data, source=str(self.reference))
        text_section = next(row for row in projection["sections"] if row["name"] == ".text")
        address = projection["image_base"] + text_section["virtual_address"]
        catalog = {
            "sections": {
                ".text": {
                    "entities": [
                        {
                            "kind": "address-group",
                            "virtual_start": 0,
                            "identities": [
                                {"map_symbol": "first", "object": "a.obj"},
                                {"map_symbol": "second", "object": "b.obj"},
                            ],
                        }
                    ]
                }
            }
        }
        reversed_rows = [
            {"address": address, "symbol": "second", "object": "b.obj"},
            {"address": address, "symbol": "first", "object": "a.obj"},
        ]
        failures = _validate_text_population(catalog, projection, reversed_rows)
        self.assertTrue(any("MAP order differs" in row for row in failures))
        duplicate_rows = [
            {"address": address, "symbol": "first", "object": "a.obj"},
            {"address": address, "symbol": "first", "object": "a.obj"},
            {"address": address, "symbol": "second", "object": "b.obj"},
        ]
        failures = _validate_text_population(catalog, projection, duplicate_rows)
        self.assertTrue(any("unexpected row" in row for row in failures))

    def test_existing_candidate_mode_is_diagnostic_and_not_acceptance_eligible(self) -> None:
        args = SimpleNamespace(
            progress=Path("progress.json"),
            candidate=Path("candidate.exe"),
            map=Path("candidate.map"),
            reference=self.reference,
            build_root=None,
            final_config=Path("final.json"),
        )
        with (
            patch(
                "_recoil.commands.live_final_verify._load_catalog_from_open_retail",
                return_value=(self.catalog, self.tracker),
            ),
            patch("_recoil.commands.live_final_verify.parse_link_map", return_value=SimpleNamespace(symbols=[])),
            patch("_recoil.commands.live_final_verify.compare_images", return_value={"passed": True}),
        ):
            report = run(args)
        self.assertFalse(report["passed"])
        self.assertTrue(report["semantic_comparison_passed"])
        self.assertFalse(report["acceptance_eligible"])
        self.assertEqual("existing-diagnostic", report["candidate_mode"])


if __name__ == "__main__":
    unittest.main()
