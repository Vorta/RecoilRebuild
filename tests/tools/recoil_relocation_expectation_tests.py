from __future__ import annotations

from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.asm_verify import (  # noqa: E402
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
)
from _recoil.commands.live_byte_verify import (  # noqa: E402
    TargetBinding,
    _bindings,
    run as run_live_byte,
)
from _recoil.commands.relocation_expectations import (  # noqa: E402
    RelocationExpectationError,
    _pe_bytes,
    _provider_null_relationship_state,
    bind_reviewed_exception_context,
    decode_x86_operand_sites,
    derive_relocation_expectations,
    normalize_relocation_target_binding,
    normalize_reviewed_exception,
)
from _recoil.lib.pe import parse_pe_headers  # noqa: E402


REFERENCE: Path


def canonical_retail_reference() -> Path:
    return REPO_ROOT / "support" / "Recoil.exe"


class FakeDocument:
    def __init__(
        self,
        symbols: dict[str, object],
        *,
        revision: int = 42,
        evidence: dict[str, object] | None = None,
    ) -> None:
        self.symbols = symbols
        self.revision = revision
        self.evidence = evidence or {}

    def collection(self, name: str) -> dict[str, object]:
        if name == "symbols":
            return self.symbols
        if name == "evidence":
            return self.evidence
        return {}

    def pipeline(self, binary: str) -> dict[str, object]:
        return {"authored_order_prefix_end": "0x401030"}


class CollectionDocument:
    def __init__(self, collections: dict[str, dict[str, object]]) -> None:
        self.collections = collections

    def collection(self, name: str) -> dict[str, object]:
        return self.collections.get(name, {})

def row(address: str, end_exclusive: str, symbol_id: str) -> dict[str, object]:
    value: dict[str, object] = {
        "binary": "recoil",
        "kind": "function",
        "address": address,
        "end_exclusive": end_exclusive,
        "pipeline_class": "authored-lifecycle",
        "authored_order_role": "authored-lifecycle-body",
        "symbol_id": symbol_id,
        "scope_ids": [symbol_id],
    }
    value["physical_rows"] = [value]
    return value


def target_binding(symbol: str) -> TargetBinding:
    return TargetBinding(
        target=SimpleNamespace(name="unit", source_from="unit.cpp"),
        function=SimpleNamespace(symbol=symbol, logical_identity_key=""),
        source_from="unit.cpp",
    )


def retail_body(start: int, end_exclusive: int) -> bytes:
    image = REFERENCE.read_bytes()
    headers = parse_pe_headers(image, source=str(REFERENCE))
    return _pe_bytes(image, headers, start, end_exclusive - start)


def governed_target_binding(
    *,
    target_kind: str,
    relationship: object,
    owner_kind: str = "class",
) -> dict[str, object]:
    return {
        "reviewed": True,
        "object_symbol": "?Target@@YAXXZ",
        "reason": "unit-test governed target relationship",
        "evidence_ids": ["unit:evidence:target-owner"],
        "binding_context": {
            "source_binding": {
                "symbol_id": "recoil:function:0x401000",
                "address": "0x401000",
                "end_exclusive": "0x401010",
                "object_symbol": "?Source@@YAXXZ",
                "physical_pipeline_class": "authored",
                "object_pipeline_class": "authored",
                "registration_ids": ["recoil:vc5-target:source"],
                "evidence_ids": [],
            },
            "relocation": {
                "offset": 1,
                "type": IMAGE_REL_I386_REL32,
                "type_name": "REL32",
                "retail_target": 0x402000,
                "instruction_offset": 0,
                "opcode": "e8",
            },
            "target": {
                "symbol_id": "recoil:function:0x402000",
                "address": "0x402000",
                "end_exclusive": "0x402010",
                "kind": target_kind,
                "navigation_name": "Target",
                "object_symbol": "?Target@@YAXXZ",
                "output_section_id": "recoil:section:.text",
                "pipeline_class": "authored",
                "ownership_state": "primary-owned",
            },
            "owner": {
                "owner_id": "recoil:owner:unit.target",
                "kind": owner_kind,
                "provider_state": "pending",
                "lifecycle_state": "pending",
                "binding_evidence_ids": ["unit:evidence:target-owner"],
            },
            "relationship": relationship,
            "creation_mode": "existing-symbol",
        },
    }


class RelocationExpectationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        global REFERENCE
        REFERENCE = canonical_retail_reference()

    def test_provider_null_relationship_accepts_exact_callable_and_iat_data_pair(self) -> None:
        function_id = "recoil:function:0x4cc5d8"
        data_id = "recoil:data:0x4cc5d8"
        document = CollectionDocument(
            {
                "symbols": {
                    function_id: {
                        "binary": "recoil",
                        "kind": "provider-function",
                        "address": "0x4cc5d8",
                        "end_exclusive": "0x4cc5d9",
                    },
                    data_id: {
                        "binary": "recoil",
                        "kind": "data",
                        "address": "0x4cc5d8",
                        "end_exclusive": "0x4cc5dc",
                    },
                }
            }
        )
        owner = {
            "kind": "provider-boundary",
            "provider_state": "accepted",
            "relationships": [
                {
                    "kind": "primary-function",
                    "address": "0x4cc5d8",
                    "symbol_id": function_id,
                },
                {
                    "kind": "primary-data",
                    "address": "0x4cc5d8",
                    "symbol_id": data_id,
                    "name": "MSVCRT.dll!rand IAT",
                },
            ],
        }
        target = {
            "kind": "provider-function",
            "address": "0x4cc5d8",
            "symbol_id": function_id,
        }

        applicable, current, invalid = _provider_null_relationship_state(
            document=document,
            owner=owner,
            target=target,
        )

        self.assertTrue(applicable)
        self.assertEqual([], invalid)
        self.assertEqual(
            ["primary-function", "primary-data"],
            [item["kind"] for item in current],
        )

    def test_provider_null_relationship_pair_rejects_missing_malformed_and_duplicate_rows(self) -> None:
        function_id = "recoil:function:0x4cc5d8"
        data_id = "recoil:data:0x4cc5d8"
        document = CollectionDocument(
            {
                "symbols": {
                    function_id: {
                        "binary": "recoil",
                        "kind": "provider-function",
                        "address": "0x4cc5d8",
                        "end_exclusive": "0x4cc5d9",
                    },
                    data_id: {
                        "binary": "recoil",
                        "kind": "data",
                        "address": "0x4cc5d8",
                        "end_exclusive": "0x4cc5dc",
                    },
                }
            }
        )
        function = {
            "kind": "primary-function",
            "address": "0x4cc5d8",
            "symbol_id": function_id,
        }
        data = {
            "kind": "primary-data",
            "address": "0x4cc5d8",
            "symbol_id": data_id,
            "name": "MSVCRT.dll!rand IAT",
        }
        target = {
            "kind": "provider-function",
            "address": "0x4cc5d8",
            "symbol_id": function_id,
        }
        cases = {
            "missing-function": [data],
            "missing-data": [function],
            "wrong-function-address": [{**function, "address": "0x4cc5d4"}, data],
            "wrong-function-symbol": [{**function, "symbol_id": "recoil:function:wrong"}, data],
            "wrong-function-kind": [{**function, "kind": "primary-data", "name": "wrong"}, data],
            "candidate-derived": [{**function, "candidate_symbol": "forbidden"}, data],
            "unsupported-field": [{**function, "unexpected": True}, data],
            "duplicate-function": [function, dict(function), data],
            "wrong-data-address": [function, {**data, "address": "0x4cc5dc"}],
            "wrong-data-symbol": [function, {**data, "symbol_id": "recoil:data:wrong"}],
            "duplicate-data": [function, data, dict(data)],
        }
        for name, relationships in cases.items():
            with self.subTest(name=name):
                applicable, _current, invalid = _provider_null_relationship_state(
                    document=document,
                    owner={
                        "kind": "provider-boundary",
                        "provider_state": "accepted",
                        "relationships": relationships,
                    },
                    target=target,
                )
                self.assertTrue(applicable)
                self.assertTrue(invalid)

        missing_data_document = CollectionDocument(
            {
                "symbols": {
                    function_id: {
                        "binary": "recoil",
                        "kind": "provider-function",
                        "address": "0x4cc5d8",
                        "end_exclusive": "0x4cc5d9",
                    }
                }
            }
        )
        applicable, _current, invalid = _provider_null_relationship_state(
            document=missing_data_document,
            owner={
                "kind": "provider-boundary",
                "provider_state": "accepted",
                "relationships": [function],
            },
            target=target,
        )
        self.assertTrue(applicable)
        self.assertTrue(invalid)

    def test_provider_null_relationship_accepts_exact_non_iat_primary_function(self) -> None:
        function_id = "recoil:function:0x4c5b64"
        document = CollectionDocument(
            {
                "symbols": {
                    function_id: {
                        "binary": "recoil",
                        "kind": "provider-function",
                        "address": "0x4c5b64",
                        "end_exclusive": "0x4c5b65",
                    }
                }
            }
        )
        applicable, current, invalid = _provider_null_relationship_state(
            document=document,
            owner={
                "kind": "provider-boundary",
                "provider_state": "accepted",
                "relationships": [
                    {
                        "kind": "primary-function",
                        "address": "0x4c5b64",
                        "symbol_id": function_id,
                    },
                    {
                        "kind": "primary-function",
                        "address": "0x4c6000",
                        "symbol_id": "recoil:function:0x4c6000",
                    },
                ],
            },
            target={
                "kind": "function",
                "address": "0x4c5b64",
                "symbol_id": function_id,
            },
        )
        self.assertTrue(applicable)
        self.assertEqual(
            [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": function_id,
                }
            ],
            current,
        )
        self.assertEqual([], invalid)

    def test_provider_null_non_iat_relationship_rejects_noncurrent_facts(self) -> None:
        function_id = "recoil:function:0x4c6000"
        document = CollectionDocument(
            {
                "symbols": {
                    function_id: {
                        "binary": "recoil",
                        "kind": "function",
                        "address": "0x4c6000",
                        "end_exclusive": "0x4c606f",
                    }
                }
            }
        )
        exact = {
            "kind": "primary-function",
            "address": "0x4c6000",
            "symbol_id": function_id,
        }
        target = {
            "kind": "function",
            "address": "0x4c6000",
            "symbol_id": function_id,
        }
        cases = {
            "missing": {
                "kind": "provider-boundary",
                "provider_state": "accepted",
                "relationships": [],
            },
            "malformed-shape": {
                "kind": "provider-boundary",
                "provider_state": "accepted",
                "relationships": "not-a-list",
            },
            "wrong-owner-state": {
                "kind": "provider-boundary",
                "provider_state": "pending",
                "relationships": [exact],
            },
            "wrong-address": {
                "kind": "provider-boundary",
                "provider_state": "accepted",
                "relationships": [{**exact, "address": "0x4c6001"}],
            },
            "wrong-symbol": {
                "kind": "provider-boundary",
                "provider_state": "accepted",
                "relationships": [
                    {**exact, "symbol_id": "recoil:function:0x4c606f"}
                ],
            },
            "wrong-kind": {
                "kind": "provider-boundary",
                "provider_state": "accepted",
                "relationships": [{**exact, "kind": "dependency"}],
            },
            "candidate-derived": {
                "kind": "provider-boundary",
                "provider_state": "accepted",
                "relationships": [{**exact, "candidate_symbol": "forbidden"}],
            },
            "duplicate": {
                "kind": "provider-boundary",
                "provider_state": "accepted",
                "relationships": [exact, dict(exact)],
            },
        }
        for name, owner in cases.items():
            with self.subTest(name=name):
                applicable, _current, invalid = _provider_null_relationship_state(
                    document=document,
                    owner=owner,
                    target=target,
                )
                self.assertTrue(applicable)
                self.assertTrue(invalid)

        applicable, current, invalid = _provider_null_relationship_state(
            document=document,
            owner={
                "kind": "class",
                "provider_state": "accepted",
                "relationships": [exact],
            },
            target=target,
        )
        self.assertFalse(applicable)
        self.assertEqual([], current)
        self.assertEqual([], invalid)

    def test_governed_target_relationship_matches_target_row_kind(self) -> None:
        function_relationship = {
            "kind": "primary-function",
            "address": "0x402000",
            "symbol_id": "recoil:function:0x402000",
        }
        normalized_function = normalize_relocation_target_binding(
            governed_target_binding(
                target_kind="function",
                relationship=function_relationship,
            )
        )
        self.assertEqual(
            function_relationship,
            normalized_function["binding_context"]["relationship"],
        )

        data_relationship = {
            "kind": "primary-data",
            "address": "0x402000",
            "symbol_id": "recoil:function:0x402000",
            "name": "g_UnitTarget",
        }
        normalized_data = normalize_relocation_target_binding(
            governed_target_binding(
                target_kind="data",
                relationship=data_relationship,
            )
        )
        self.assertEqual(
            data_relationship,
            normalized_data["binding_context"]["relationship"],
        )

        normalized_provider = normalize_relocation_target_binding(
            governed_target_binding(
                target_kind="provider-function",
                relationship=None,
                owner_kind="provider-boundary",
            )
        )
        self.assertIsNone(normalized_provider["binding_context"]["relationship"])

    def test_governed_target_relationship_fails_closed_for_mismatched_state(self) -> None:
        base = {
            "kind": "primary-function",
            "address": "0x402000",
            "symbol_id": "recoil:function:0x402000",
        }
        cases = {
            "wrong-kind": {**base, "kind": "primary-data", "name": "wrong"},
            "unsupported-kind": {**base, "kind": "secondary-function"},
            "wrong-symbol": {**base, "symbol_id": "recoil:function:0x402010"},
            "wrong-address": {**base, "address": "0x402010"},
            "function-name": {**base, "name": "not-part-of-function-schema"},
            "candidate-derived": {**base, "candidate_symbol": "?Candidate@@YAXXZ"},
        }
        for name, relationship in cases.items():
            with self.subTest(name=name), self.assertRaises(RelocationExpectationError):
                normalize_relocation_target_binding(
                    governed_target_binding(
                        target_kind="function",
                        relationship=relationship,
                    )
                )

        with self.assertRaisesRegex(RelocationExpectationError, "provider.*must be null"):
            normalize_relocation_target_binding(
                governed_target_binding(
                    target_kind="provider-function",
                    relationship=base,
                    owner_kind="provider-boundary",
                )
            )
        with self.assertRaisesRegex(RelocationExpectationError, "non-provider.*must be present"):
            normalize_relocation_target_binding(
                governed_target_binding(target_kind="function", relationship=None)
            )

    def derive_synthetic_rel32(
        self,
        body: bytes,
        *,
        object_symbol: str = "?Recursive@@YAXXZ",
    ) -> dict[str, object]:
        source_address = 0x401000
        source_id = "recoil:function:0x401000"
        current = row(hex(source_address), hex(source_address + len(body)), source_id)
        headers = SimpleNamespace(image_base=0x400000, size_of_image=0x100000)
        with (
            patch(
                "_recoil.commands.relocation_expectations.parse_pe_headers",
                return_value=headers,
            ),
            patch(
                "_recoil.commands.relocation_expectations._pe_bytes",
                return_value=body,
            ),
        ):
            return derive_relocation_expectations(
                document=FakeDocument({source_id: current}),
                row=current,
                object_symbol=object_symbol,
                bindings={},
                reference=REFERENCE,
            )

    def test_recursive_call_to_current_entry_uses_registered_symbol_with_zero_addend(
        self,
    ) -> None:
        report = self.derive_synthetic_rel32(bytes.fromhex("e8 fb ff ff ff c3"))

        self.assertTrue(report["passed"], report)
        self.assertFalse(report["explicit_empty"])
        self.assertEqual([], report["unresolved"])
        self.assertEqual(1, len(report["expectations"]))
        self.assertEqual(
            {
                "object_symbol": "?Recursive@@YAXXZ",
                "offset": 1,
                "type": IMAGE_REL_I386_REL32,
                "type_name": "REL32",
                "target_symbol": "?Recursive@@YAXXZ",
                "target_symbol_id": "recoil:function:0x401000",
                "coff_addend": 0,
                "resolved_target_addend": 0,
                "retail_target": 0x401000,
                "derivation": "current-registered-object-symbol",
                "instruction_offset": 0,
                "opcode": "e8",
            },
            report["expectations"][0],
        )

    def test_internal_non_call_branch_remains_relocation_free(self) -> None:
        report = self.derive_synthetic_rel32(bytes.fromhex("e9 00 00 00 00 c3"))

        self.assertTrue(report["passed"], report)
        self.assertTrue(report["explicit_empty"])
        self.assertEqual([], report["expectations"])
        self.assertEqual([], report["unresolved"])

    def test_call_to_current_function_interior_fails_closed(self) -> None:
        report = self.derive_synthetic_rel32(bytes.fromhex("e8 00 00 00 00 c3"))

        self.assertFalse(report["passed"])
        self.assertFalse(report["explicit_empty"])
        self.assertEqual([], report["expectations"])
        self.assertEqual(1, len(report["unresolved"]))
        unresolved = report["unresolved"][0]
        self.assertEqual("unsupported-internal-call-target", unresolved["kind"])
        self.assertEqual(1, unresolved["offset"])
        self.assertEqual("0x401005", unresolved["retail_target"])

    def test_registered_vc5_data_symbol_becomes_a_typed_target_identity(self) -> None:
        target_id = "recoil:vc5-target:data"
        data_id = "recoil:data:0x4cc738"
        data_symbol = SimpleNamespace(
            address="0x4cc738", symbol="??_7CAboutDlg@@6B@"
        )
        target = SimpleNamespace(
            name="data",
            manifest_path=REPO_ROOT / "tools" / "vc5_verify_targets" / "data.json",
            source_from="about.cpp",
            functions=(),
            data_symbols=(data_symbol,),
            translation_unit_function_order=(),
            linked_function_intervals=(),
        )
        document = CollectionDocument(
            {
                "symbols": {
                    data_id: {
                        "binary": "recoil",
                        "kind": "data",
                        "address": "0x4cc738",
                        "verification_target_ids": [target_id],
                    }
                },
                "verification_targets": {
                    target_id: {
                        "kind": "vc5",
                        "name": "data",
                        "registration": {"name": "data"},
                    }
                },
            }
        )
        with patch("_recoil.commands.live_byte_verify.load_manifests", return_value=[target]):
            bindings = _bindings(document, Path("tools/vc5_verify_targets"))
        self.assertEqual("??_7CAboutDlg@@6B@", bindings[data_id][0].function.symbol)

    def test_decoder_finds_rel32_and_dir32_without_candidate_input(self) -> None:
        data = bytes.fromhex(
            "8b 44 24 04 56 50 8b f1 6a 67 e8 55 4b 0c 00 "
            "c7 06 38 c7 4c 00 8b c6 5e c2 04 00 90 90 90 90 90"
        )
        sites, unresolved = decode_x86_operand_sites(data)
        self.assertEqual((), unresolved)
        self.assertEqual(
            [(11, IMAGE_REL_I386_REL32), (17, IMAGE_REL_I386_DIR32)],
            [(site.offset, site.relocation_type) for site in sites],
        )

    def test_derives_trailing_inline_switch_table_and_normal_instruction_relocations(self) -> None:
        source_address = 0x401060
        source_end = 0x401180
        source_id = "recoil:function:0x401060"
        source_symbol = "?SwitchSource@@YAXXZ"
        current = row(hex(source_address), hex(source_end), source_id)
        symbols: dict[str, object] = {source_id: current}
        bindings: dict[str, list[TargetBinding]] = {}
        for target_address in (
            0x401180,
            0x401710,
            0x401B20,
            0x401F60,
            0x402090,
            0x402170,
            0x402B70,
        ):
            target_id = f"recoil:function:0x{target_address:x}"
            symbols[target_id] = {
                "binary": "recoil",
                "kind": "function",
                "address": hex(target_address),
                "end_exclusive": hex(target_address + 1),
            }
            bindings[target_id] = [target_binding(f"?Target_{target_address:x}@@YAXXZ")]
        symbols["recoil:data:0x4f3a88"] = {
            "binary": "recoil",
            "kind": "data",
            "address": "0x4f3a88",
            "end_exclusive": "0x4f3a8c",
            "relocation_target_binding": {
                "reviewed": True,
                "object_symbol": "?SwitchGlobal@@3PAUAIStruct@@A",
                "reason": "unit-test exact global target",
                "evidence_ids": ["unit:evidence:switch-global"],
            },
        }

        report = derive_relocation_expectations(
            document=FakeDocument(symbols),
            row=current,
            object_symbol=source_symbol,
            bindings=bindings,
            reference=REFERENCE,
        )

        self.assertTrue(report["passed"], report)
        table_entries = [
            item for item in report["expectations"] if item["opcode"] == "switch-table"
        ]
        self.assertEqual(
            [0x108, 0x10C, 0x110, 0x114, 0x118, 0x11C],
            [item["offset"] for item in table_entries],
        )
        self.assertEqual(
            [0x4010A5, 0x4010F4, 0x4010FE, 0x40112A, 0x40113B, 0x4010AE],
            [item["retail_target"] for item in table_entries],
        )
        self.assertEqual(
            [0x45, 0x94, 0x9E, 0xCA, 0xDB, 0x4E],
            [item["coff_addend"] for item in table_entries],
        )
        self.assertTrue(
            all(item["target_symbol"] == source_symbol for item in table_entries)
        )
        instruction_relocations = [
            item for item in report["expectations"] if item["opcode"] != "switch-table"
        ]
        self.assertEqual(
            [
                0x1,
                0x41,
                0x48,
                0x6E,
                0x8D,
                0x97,
                0xA1,
                0xA8,
                0xC3,
                0xCD,
                0xD4,
                0xDE,
                0xE5,
                0x100,
            ],
            [item["offset"] for item in instruction_relocations],
        )

    def test_inline_switch_table_bound_mismatch_is_unresolved(self) -> None:
        body = bytearray(retail_body(0x401060, 0x401180))
        body[0x34] = 4
        sites, unresolved = decode_x86_operand_sites(
            bytes(body),
            function_address=0x401060,
        )
        self.assertFalse(any(site.kind == "switch-table-entry" for site in sites))
        self.assertEqual("ambiguous-inline-switch-table", unresolved[0]["kind"])
        self.assertEqual(0x41, unresolved[0]["offset"])

    def test_inline_switch_table_accepts_exact_unsigned_above_default_target(self) -> None:
        body = retail_body(0x401710, 0x401964)
        self.assertEqual(bytes.fromhex("aa 18 40 00"), body[0x248:0x24C])

        sites, unresolved = decode_x86_operand_sites(
            body,
            function_address=0x401710,
        )

        self.assertEqual((), unresolved)
        self.assertEqual(
            [0x238, 0x23C, 0x240, 0x244, 0x248, 0x24C, 0x250],
            [site.offset for site in sites if site.kind == "switch-table-entry"],
        )

    def test_inline_switch_table_rejects_entry_beyond_unsigned_above_default_target(self) -> None:
        body = bytearray(retail_body(0x401710, 0x401964))
        body[0x248] = 0xAB

        sites, unresolved = decode_x86_operand_sites(
            bytes(body),
            function_address=0x401710,
        )

        self.assertFalse(any(site.kind == "switch-table-entry" for site in sites))
        self.assertEqual("ambiguous-inline-switch-table", unresolved[0]["kind"])
        self.assertEqual(0x131, unresolved[0]["offset"])

    def test_inline_switch_table_rejects_malformed_lea_padding(self) -> None:
        body = bytearray(retail_body(0x401710, 0x401964))
        body[0x235:0x238] = bytes.fromhex("8d c9 90")

        sites, unresolved = decode_x86_operand_sites(
            bytes(body),
            function_address=0x401710,
        )

        self.assertFalse(any(site.kind == "switch-table-entry" for site in sites))
        self.assertEqual("ambiguous-inline-switch-table-boundary", unresolved[0]["kind"])
        self.assertEqual(0x238, unresolved[0]["offset"])

    def test_inline_switch_table_rejects_nonzero_lea_padding(self) -> None:
        body = bytearray(retail_body(0x401710, 0x401964))
        body[0x235:0x238] = bytes.fromhex("8d 49 01")

        sites, unresolved = decode_x86_operand_sites(
            bytes(body),
            function_address=0x401710,
        )

        self.assertFalse(any(site.kind == "switch-table-entry" for site in sites))
        self.assertEqual("ambiguous-inline-switch-table-boundary", unresolved[0]["kind"])
        self.assertEqual(0x238, unresolved[0]["offset"])

    def test_inline_switch_table_rejects_different_register_lea_padding(self) -> None:
        body = bytearray(retail_body(0x401710, 0x401964))
        body[0x235:0x238] = bytes.fromhex("8d 41 00")

        sites, unresolved = decode_x86_operand_sites(
            bytes(body),
            function_address=0x401710,
        )

        self.assertFalse(any(site.kind == "switch-table-entry" for site in sites))
        self.assertEqual("ambiguous-inline-switch-table-boundary", unresolved[0]["kind"])
        self.assertEqual(0x238, unresolved[0]["offset"])

    def test_inline_switch_table_code_data_seam_must_be_ret_nop_padding(self) -> None:
        body = bytearray(retail_body(0x401060, 0x401180))
        body[0x107] = 0xCC
        sites, unresolved = decode_x86_operand_sites(
            bytes(body),
            function_address=0x401060,
        )
        self.assertFalse(any(site.kind == "switch-table-entry" for site in sites))
        self.assertEqual("ambiguous-inline-switch-table-boundary", unresolved[0]["kind"])
        self.assertEqual(0x108, unresolved[0]["offset"])

    def test_derives_rel32_raw_coff_addends_and_exact_target_symbols_from_retail(self) -> None:
        for callee_address, expected_addend in (("0x4c5b64", 0), ("0x4c5b60", 4)):
            with self.subTest(callee_address=callee_address):
                current_id = "recoil:function:0x401000"
                current = row("0x401000", "0x401020", current_id)
                callee_id = f"recoil:function:{callee_address}"
                data_id = "recoil:data:0x4cc738"
                document = FakeDocument(
                    {
                        current_id: current,
                        callee_id: {
                            "binary": "recoil",
                            "kind": "function",
                            "address": callee_address,
                            "end_exclusive": "0x4c5b6a",
                        },
                        data_id: {
                            "binary": "recoil",
                            "kind": "data",
                            "address": "0x4cc738",
                            "end_exclusive": "0x4cc73c",
                            "relocation_target_binding": {
                                "reviewed": True,
                                "object_symbol": "??_7CAboutDlg@@6B@",
                                "reason": "unit-test exact typed data symbol",
                                "evidence_ids": ["unit:evidence:data-target"],
                            },
                        },
                    },
                    evidence={
                        "unit:evidence:callee": {},
                        "unit:evidence:vtable": {},
                        "unit:evidence:data": {},
                    },
                )
                bindings = {
                    callee_id: [target_binding("??0CDialog@@QAE@IPAVCWnd@@@Z")]
                }
                report = derive_relocation_expectations(
                    document=document,
                    row=current,
                    object_symbol="??0CAboutDlg@@QAE@I@Z",
                    bindings=bindings,
                    reference=REFERENCE,
                )
                self.assertTrue(report["passed"], report)
                self.assertTrue(report["candidate_independent"])
                self.assertFalse(report["explicit_empty"])
                self.assertEqual(2, len(report["expectations"]))
                rel32, dir32 = report["expectations"]
                self.assertEqual(11, rel32["offset"])
                self.assertEqual(IMAGE_REL_I386_REL32, rel32["type"])
                self.assertEqual(
                    "??0CDialog@@QAE@IPAVCWnd@@@Z", rel32["target_symbol"]
                )
                self.assertEqual(expected_addend, rel32["coff_addend"])
                self.assertEqual(expected_addend, rel32["resolved_target_addend"])
                self.assertEqual(0x4C5B64, rel32["retail_target"])
                self.assertEqual(17, dir32["offset"])
                self.assertEqual(IMAGE_REL_I386_DIR32, dir32["type"])
                self.assertEqual("??_7CAboutDlg@@6B@", dir32["target_symbol"])
                self.assertEqual(0, dir32["coff_addend"])
                self.assertEqual(0x4CC738, dir32["retail_target"])

    def test_relocation_free_function_yields_an_explicit_empty_set(self) -> None:
        symbol_id = "recoil:function:0x401020"
        current = row("0x401020", "0x401030", symbol_id)
        report = derive_relocation_expectations(
            document=FakeDocument({symbol_id: current}),
            row=current,
            object_symbol="?DoDataExchange@CAboutDlg@@MAEXPAVCDataExchange@@@Z",
            bindings={},
            reference=REFERENCE,
        )
        self.assertTrue(report["passed"], report)
        self.assertTrue(report["explicit_empty"])
        self.assertEqual([], report["expectations"])
        self.assertEqual([], report["unresolved"])

    def test_missing_typed_targets_are_actionable_ambiguities_not_guesses(self) -> None:
        symbol_id = "recoil:function:0x401000"
        current = row("0x401000", "0x401020", symbol_id)
        report = derive_relocation_expectations(
            document=FakeDocument({symbol_id: current}),
            row=current,
            object_symbol="??0CAboutDlg@@QAE@I@Z",
            bindings={},
            reference=REFERENCE,
        )
        self.assertFalse(report["passed"])
        self.assertEqual([], report["expectations"])
        self.assertEqual(
            [(11, "missing-target-identity"), (17, "missing-target-identity")],
            [(item["offset"], item["kind"]) for item in report["unresolved"]],
        )
        self.assertEqual("0x4c5b64", report["unresolved"][0]["retail_target"])
        self.assertEqual("0x4cc738", report["unresolved"][1]["retail_target"])

    def test_reviewed_exception_can_resolve_a_genuinely_missing_target_fact(self) -> None:
        symbol_id = "recoil:function:0x401000"
        current = row("0x401000", "0x401020", symbol_id)
        callee_id = "recoil:function:0x4c5b64"
        data_id = "recoil:data:0x4cc738"
        document = FakeDocument(
            {
                symbol_id: current,
                callee_id: {
                    "binary": "recoil",
                    "kind": "function",
                    "address": "0x4c5b64",
                    "end_exclusive": "0x4c5b6a",
                    "pipeline_class": "non-authored",
                },
                data_id: {
                    "binary": "recoil",
                    "kind": "data",
                    "address": "0x4cc738",
                    "end_exclusive": "0x4cc73c",
                    "pipeline_class": "authored",
                    "relocation_target_binding": {
                        "reviewed": True,
                        "object_symbol": "??_7CAboutDlg@@6B@",
                        "reason": "unit-test exact data target",
                        "evidence_ids": ["unit:evidence:data"],
                    },
                },
            },
            evidence={
                "unit:evidence:callee": {},
                "unit:evidence:vtable": {},
                "unit:evidence:data": {},
            },
        )
        bindings = {
            symbol_id: [target_binding("??0CAboutDlg@@QAE@I@Z")],
            callee_id: [target_binding("??0CDialog@@QAE@IPAVCWnd@@@Z")],
        }
        payloads = [
            {
                "reviewed": True,
                "object_symbol": "??0CAboutDlg@@QAE@I@Z",
                "offset": 11,
                "type": IMAGE_REL_I386_REL32,
                "target_symbol": "??0CDialog@@QAE@IPAVCWnd@@@Z",
                "target_symbol_id": callee_id,
                "coff_addend": 0,
                "resolved_target_addend": 0,
                "retail_target": "0x4c5b64",
                "reason": "unit-test reviewed provider target",
                "evidence_ids": ["unit:evidence:callee"],
            },
            {
                "reviewed": True,
                "object_symbol": "??0CAboutDlg@@QAE@I@Z",
                "offset": 17,
                "type": IMAGE_REL_I386_DIR32,
                "target_symbol": "??_7CAboutDlg@@6B@",
                "target_symbol_id": data_id,
                "coff_addend": 0,
                "resolved_target_addend": 0,
                "retail_target": "0x4cc738",
                "reason": "unit-test reviewed vtable target",
                "evidence_ids": ["unit:evidence:vtable"],
            },
        ]
        current["relocation_expectation_exceptions"] = [
            bind_reviewed_exception_context(
                payload,
                document=document,
                bindings=bindings,
                source_symbol_id=symbol_id,
            )
            for payload in payloads
        ]
        report = derive_relocation_expectations(
            document=document,
            row=current,
            object_symbol="??0CAboutDlg@@QAE@I@Z",
            bindings=bindings,
            reference=REFERENCE,
        )
        self.assertTrue(report["passed"], report)
        self.assertEqual(2, report["reviewed_exception_count"])
        self.assertTrue(all(item["derivation"] == "reviewed-exception" for item in report["expectations"]))

    def test_normalized_exception_requires_reason_and_supporting_evidence(self) -> None:
        incomplete = {
            "reviewed": True,
            "object_symbol": "?Source@@YAXXZ",
            "offset": 1,
            "type": IMAGE_REL_I386_REL32,
            "target_symbol": "?Target@@YAXXZ",
            "target_symbol_id": "recoil:function:0x402000",
            "coff_addend": 0xFFFFFFFC,
            "resolved_target_addend": -4,
            "retail_target": "0x402000",
        }
        with self.assertRaisesRegex(RelocationExpectationError, "reason"):
            normalize_reviewed_exception(incomplete)
        incomplete["reason"] = "reviewed unit target"
        with self.assertRaisesRegex(RelocationExpectationError, "evidence_ids"):
            normalize_reviewed_exception(incomplete)
        incomplete["evidence_ids"] = ["unit:evidence:1"]
        normalized = normalize_reviewed_exception(incomplete)
        self.assertEqual(0xFFFFFFFC, normalized["coff_addend"])
        self.assertEqual(-4, normalized["resolved_target_addend"])

    def test_authored_lane_blocks_before_expensive_build_when_expectations_are_unresolved(self) -> None:
        symbol_id = "recoil:function:0x401000"
        current = row("0x401000", "0x401020", symbol_id)
        document = FakeDocument({symbol_id: current})
        binding = target_binding("??0CAboutDlg@@QAE@I@Z")
        args = SimpleNamespace(
            mode="authored",
            at="0x401000",
            progress=Path("progress.json"),
            build_root=Path("build/live-validation/authored/unit"),
            manifest_dir=Path("tools/vc5_verify_targets"),
            final_config=Path("tools/_recoil/config/vc5_final_build.json"),
            reference=REFERENCE,
        )
        unresolved = {
            "passed": False,
            "status": "unresolved",
            "object_symbol": binding.function.symbol,
            "expectations": [],
            "unresolved": [{"kind": "missing-target-identity", "offset": 11}],
        }
        with (
            patch(
                "_recoil.commands.live_byte_verify.ProgressDocument.load",
                return_value=document,
            ),
            patch(
                "_recoil.commands.live_byte_verify._bindings",
                return_value={symbol_id: [binding]},
            ),
            patch(
                "_recoil.commands.live_byte_verify.derive_relocation_expectations",
                return_value=unresolved,
            ),
            patch("_recoil.commands.live_byte_verify._run_fresh_build") as build,
        ):
            report = run_live_byte(args)
        build.assert_not_called()
        self.assertFalse(report["passed"])
        self.assertFalse(report["build_performed"])
        self.assertIsNone(report["build_root"])
        self.assertEqual("relocation-expectations", report["first_divergence"]["stage"])


if __name__ == "__main__":
    unittest.main()
