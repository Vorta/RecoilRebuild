from __future__ import annotations

import json
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands import vc5_abi_equivalence
from _recoil.commands.asm_verify import CoffObject
from _recoil.commands.vc5_abi_equivalence import (
    _paired_raw_definition_normalization,
    SourceSymbolPair,
    SymbolPair,
    ZeroArgAbiTarget,
    compare_objects,
    eligibility_gates,
    load_zeroarg_targets,
    manifest_symbol_normalization,
)


def code_object(
    callee: str,
    caller: str,
    *,
    callee_body: bytes = b"\xc3",
    caller_tail: bytes = b"\xc3",
) -> CoffObject:
    long_names = bytearray()

    def name(value: str) -> bytes:
        encoded = value.encode("ascii")
        if len(encoded) <= 8:
            return encoded.ljust(8, b"\0")
        offset = 4 + len(long_names)
        long_names.extend(encoded + b"\0")
        return struct.pack("<II", 0, offset)

    text = callee_body + b"\xe8\x00\x00\x00\x00" + caller_tail
    section_offset = 20 + 40
    relocation_offset = section_offset + len(text)
    symbol_offset = relocation_offset + 10
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, symbol_offset, 2, 0, 0)
    section = (
        name(".text")
        + struct.pack(
            "<IIIIIIHHI",
            0,
            0,
            len(text),
            section_offset,
            relocation_offset,
            0,
            1,
            0,
            0x60000020,
        )
    )
    relocation = struct.pack("<IIH", len(callee_body) + 1, 0, 0x14)
    callee_symbol = name(callee) + struct.pack("<IhHBB", 0, 1, 0x20, 2, 0)
    caller_symbol = name(caller) + struct.pack(
        "<IhHBB",
        len(callee_body),
        1,
        0x20,
        2,
        0,
    )
    return CoffObject.from_bytes(
        header
        + section
        + text
        + relocation
        + callee_symbol
        + caller_symbol
        + struct.pack("<I", 4 + len(long_names))
        + long_names
    )


def external_call_object(callee: str, caller: str) -> CoffObject:
    long_names = bytearray()

    def name(value: str) -> bytes:
        encoded = value.encode("ascii")
        if len(encoded) <= 8:
            return encoded.ljust(8, b"\0")
        offset = 4 + len(long_names)
        long_names.extend(encoded + b"\0")
        return struct.pack("<II", 0, offset)

    text = b"\xe8\x00\x00\x00\x00\xc3"
    section_offset = 20 + 40
    relocation_offset = section_offset + len(text)
    symbol_offset = relocation_offset + 10
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, symbol_offset, 2, 0, 0)
    section = (
        name(".text")
        + struct.pack(
            "<IIIIIIHHI",
            0,
            0,
            len(text),
            section_offset,
            relocation_offset,
            0,
            1,
            0,
            0x60000020,
        )
    )
    relocation = struct.pack("<IIH", 1, 0, 0x14)
    callee_symbol = name(callee) + struct.pack("<IhHBB", 0, 0, 0x20, 2, 0)
    caller_symbol = name(caller) + struct.pack("<IhHBB", 0, 1, 0x20, 2, 0)
    return CoffObject.from_bytes(
        header
        + section
        + text
        + relocation
        + callee_symbol
        + caller_symbol
        + struct.pack("<I", 4 + len(long_names))
        + long_names
    )


def definition_object(
    names: list[str],
    *,
    body_sizes: list[int] | None = None,
    relocation_offsets: list[int] | None = None,
    symbol_values: list[int] | None = None,
) -> CoffObject:
    long_names = bytearray()

    def name(value: str) -> bytes:
        encoded = value.encode("ascii")
        if len(encoded) <= 8:
            return encoded.ljust(8, b"\0")
        offset = 4 + len(long_names)
        long_names.extend(encoded + b"\0")
        return struct.pack("<II", 0, offset)

    sizes = body_sizes or [1] * len(names)
    if len(sizes) != len(names) or any(size <= 0 for size in sizes):
        raise ValueError("body_sizes must contain one positive size per definition")
    text = b"".join(
        b"\xc3" + (b"\x90" * (size - 1))
        for size in sizes
    )
    offsets = relocation_offsets or []
    if any(offset < 0 or offset + 4 > len(text) for offset in offsets):
        raise ValueError("relocation offset must name four bytes inside .text")
    relocations = b"".join(
        struct.pack("<IIH", offset, 0, 0x0006)
        for offset in offsets
    )
    section_offset = 20 + 40
    relocation_offset = section_offset + len(text) if relocations else 0
    symbol_offset = section_offset + len(text) + len(relocations)
    header = struct.pack(
        "<HHIIIHH",
        0x14C,
        1,
        0,
        symbol_offset,
        len(names),
        0,
        0,
    )
    section = (
        name(".text")
        + struct.pack(
            "<IIIIIIHHI",
            0,
            0,
            len(text),
            section_offset,
            relocation_offset,
            0,
            len(offsets),
            0,
            0x60000020,
        )
    )
    values: list[int] = []
    cursor = 0
    for size in sizes:
        values.append(cursor)
        cursor += size
    if symbol_values is not None:
        if len(symbol_values) != len(names):
            raise ValueError("symbol_values must contain one value per definition")
        values = symbol_values
    symbols = b"".join(
        name(symbol) + struct.pack("<IhHBB", value, 1, 0x20, 2, 0)
        for symbol, value in zip(names, values)
    )
    return CoffObject.from_bytes(
        header
        + section
        + text
        + relocations
        + symbols
        + struct.pack("<I", 4 + len(long_names))
        + long_names
    )


def eligible_target(**overrides: object) -> ZeroArgAbiTarget:
    evidence = {
        "free_or_static": True,
        "explicit_argument_count": 0,
        "hidden_this": False,
        "hidden_sret": False,
        "lifecycle": False,
        "variadic": False,
        "address_taken": False,
        "callback": False,
        "vtable": False,
        "export": False,
        "import": False,
        "function_pointer": False,
        "direct_calls": [
            {
                "address": "0x401000",
                "dispatch": "direct",
                "explicit_argument_count": 0,
                "callee_return": "plain-ret",
            }
        ],
    }
    values: dict[str, object] = {
        "target_id": "sample",
        "identity": "recoil:function:sample",
        "callee_source": ROOT / "src" / "Battlesport" / "hud.cpp",
        "callee": SymbolPair("_calld", "@callr"),
        "callers": (
            SourceSymbolPair(
                ROOT / "src" / "Battlesport" / "hud.cpp",
                "_used",
                "@user",
            ),
        ),
        "return_category": "void",
        "retail_evidence": evidence,
        "eh_policy": {"kind": "none", "retail_proven": True},
        "st0_policy": None,
    }
    values.update(overrides)
    return ZeroArgAbiTarget(**values)  # type: ignore[arg-type]


class Vc5AbiEquivalenceTests(unittest.TestCase):
    def _compare(
        self,
        target: ZeroArgAbiTarget,
        cdecl: CoffObject,
        fastcall: CoffObject,
        *,
        all_targets: tuple[ZeroArgAbiTarget, ...] | None = None,
    ) -> tuple[list[dict[str, object]], dict[str, object]]:
        source = target.callee_source
        return compare_objects(
            target,
            {source: cdecl},
            {source: fastcall},
            normalization=manifest_symbol_normalization(
                all_targets or (target,)
            ),
        )

    def test_paired_compile_profiles_define_abi_equivalence_probe(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as temp:
            build_root = Path(temp)
            source = build_root / "probe.cpp"
            source.write_text("void probe() {}\n", encoding="utf-8")
            config = vc5_abi_equivalence.load_config(
                vc5_abi_equivalence.DEFAULT_MANIFEST
            )
            commands: list[tuple[str, dict[str, object]]] = []

            def fake_run_cmd_script(
                command: str,
                **kwargs: object,
            ) -> SimpleNamespace:
                commands.append((command, kwargs))
                return SimpleNamespace(returncode=0)

            with patch.object(
                vc5_abi_equivalence,
                "run_cmd_script",
                side_effect=fake_run_cmd_script,
            ):
                for convention in ("Gd", "Gr"):
                    with self.subTest(convention=convention):
                        returncode, _obj, _stdout_log, _stderr_log = (
                            vc5_abi_equivalence._compile_one(
                                config,
                                source,
                                convention=convention,
                                build_root=build_root,
                            )
                        )
                        self.assertEqual(0, returncode)
                        response_files = list(
                            (build_root / convention.lower() / "rsp").rglob("*.rsp")
                        )
                        self.assertEqual(1, len(response_files))
                        response = response_files[0]
                        response_lines = response.read_text(
                            encoding="ascii"
                        ).splitlines()
                        self.assertEqual(
                            1,
                            response_lines.count(
                                "/DRECOIL_VC5_ABI_EQUIVALENCE_PROBE"
                            ),
                        )
                        self.assertIn(f"/{convention}", response_lines)
                        opposite = "Gr" if convention == "Gd" else "Gd"
                        self.assertNotIn(f"/{opposite}", response_lines)
                        command, kwargs = commands[-1]
                        self.assertIn(str(response), command)
                        self.assertEqual(
                            str(
                                build_root
                                / convention.lower()
                                / "_compile.cmd"
                            ),
                            kwargs["script_name"],
                        )

    def test_complete_eligibility_and_paired_candidate_gates_pass(self) -> None:
        target = eligible_target()
        self.assertTrue(all(row["passed"] for row in eligibility_gates(target)))
        comparison, _details = self._compare(
            target,
            code_object("_calld", "_used"),
            code_object("@callr", "@user"),
        )
        self.assertTrue(all(row["passed"] for row in comparison), comparison)

    def test_plain_ret_accepts_terminal_c3_with_coff_nop_padding_untrimmed(
        self,
    ) -> None:
        callee_body = bytes.fromhex(
            "8b0d0400000085c9750333c0c3"
            "a1080000002bc1c1f802c3"
            "9090909090909090"
        )
        gates, details = self._compare(
            eligible_target(),
            code_object("_calld", "_used", callee_body=callee_body),
            code_object("@callr", "@user", callee_body=callee_body),
        )
        plain_ret = next(
            row for row in gates if row["gate"] == "candidate-plain-ret"
        )
        self.assertTrue(plain_ret["passed"], plain_ret)
        self.assertEqual(
            callee_body.hex(),
            details["callee"]["cdecl"]["bytes_hex"],
        )
        self.assertEqual(
            callee_body.hex(),
            details["callee"]["fastcall"]["bytes_hex"],
        )

    def test_plain_ret_rejects_nonret_and_non_nop_suffixes(self) -> None:
        self.assertTrue(
            vc5_abi_equivalence._is_plain_ret_with_coff_nop_padding(b"\xc3")
        )
        for data in (
            b"",
            b"\x90",
            b"\x90\x90\x90",
            b"\xc2\x04\x00",
            b"\xc3\xcc",
            b"\xc3\x90\xcc",
        ):
            with self.subTest(data=data.hex()):
                self.assertFalse(
                    vc5_abi_equivalence._is_plain_ret_with_coff_nop_padding(data)
                )
        self.assertFalse(
            vc5_abi_equivalence._is_plain_ret_with_coff_nop_padding(
                b"\xc3\x90",
                relocation_mask=(False, True),
            )
        )
        self.assertFalse(
            vc5_abi_equivalence._is_plain_ret_with_coff_nop_padding(
                b"\xc3",
                relocation_mask=(True,),
            )
        )
        with self.assertRaisesRegex(ValueError, "mask length"):
            vc5_abi_equivalence._is_plain_ret_with_coff_nop_padding(
                b"\xc3\x90",
                relocation_mask=(False,),
            )

    def test_each_forbidden_abi_context_fails_closed(self) -> None:
        for field in (
            "hidden_this",
            "hidden_sret",
            "lifecycle",
            "variadic",
            "address_taken",
            "callback",
            "vtable",
            "export",
            "import",
            "function_pointer",
        ):
            with self.subTest(field=field):
                target = eligible_target()
                evidence = dict(target.retail_evidence)
                evidence[field] = True
                gates = eligibility_gates(
                    eligible_target(retail_evidence=evidence)
                )
                self.assertFalse(all(row["passed"] for row in gates))

    def test_missing_direct_retail_noarg_plain_ret_proof_fails(self) -> None:
        target = eligible_target()
        for direct_calls in (
            [],
            [{"address": "0x401000", "dispatch": "indirect", "explicit_argument_count": 0, "callee_return": "plain-ret"}],
            [{"address": "0x401000", "dispatch": "direct", "explicit_argument_count": 1, "callee_return": "plain-ret"}],
            [{"address": "0x401000", "dispatch": "direct", "explicit_argument_count": 0, "callee_return": "ret-4"}],
        ):
            evidence = dict(target.retail_evidence)
            evidence["direct_calls"] = direct_calls
            self.assertFalse(
                all(
                    row["passed"]
                    for row in eligibility_gates(
                        eligible_target(retail_evidence=evidence)
                    )
                )
            )

    def test_return_category_is_bounded_and_x87_requires_separate_st0_proof(self) -> None:
        self.assertFalse(
            all(
                row["passed"]
                for row in eligibility_gates(
                    eligible_target(return_category="integral64")
                )
            )
        )
        self.assertFalse(
            all(
                row["passed"]
                for row in eligibility_gates(
                    eligible_target(return_category="x87-float")
                )
            )
        )
        self.assertTrue(
            all(
                row["passed"]
                for row in eligibility_gates(
                    eligible_target(
                        return_category="x87-float",
                        st0_policy={"proven": True, "evidence": "separate retail ST0 comparison"},
                    )
                )
            )
        )

    def test_candidate_byte_or_callsite_divergence_fails(self) -> None:
        gates, _details = self._compare(
            eligible_target(),
            code_object("_calld", "_used"),
            code_object("@callr", "@user", caller_tail=b"\x90\xc3"),
        )
        self.assertFalse(all(row["passed"] for row in gates))
        failed_names = {row["gate"] for row in gates if not row["passed"]}
        self.assertIn("candidate-direct-callsite-bytes-and-relocations", failed_names)

    def test_cross_source_callers_and_all_manifest_pairs_normalize_raw_order(
        self,
    ) -> None:
        callee_source = ROOT / "src" / "Battlesport" / "hud.cpp"
        caller_source = ROOT / "src" / "GameZRecoil" / "zui.cpp"
        target = eligible_target(
            callee_source=callee_source,
            callers=(
                SourceSymbolPair(caller_source, "_used", "@user"),
            ),
        )
        unrelated = eligible_target(
            target_id="unrelated",
            identity="recoil:function:unrelated",
            callee_source=callee_source,
            callee=SymbolPair("_otherd", "@otherr"),
            callers=(
                SourceSymbolPair(caller_source, "_elsed", "@elser"),
            ),
        )
        normalization = manifest_symbol_normalization((target, unrelated))
        gates, details = compare_objects(
            target,
            {
                callee_source: code_object("_calld", "_otherd"),
                caller_source: external_call_object("_calld", "_used"),
            },
            {
                callee_source: code_object("@callr", "@otherr"),
                caller_source: external_call_object("@callr", "@user"),
            },
            normalization=normalization,
        )
        self.assertTrue(all(row["passed"] for row in gates), gates)
        self.assertEqual(
            str(caller_source.resolve()),
            details["callers"][0]["source"],
        )
        self.assertEqual(2, len(details["raw_definition_order"]))

    def test_stable_member_caller_symbol_is_compared_without_pair_normalization(
        self,
    ) -> None:
        source = ROOT / "src" / "Battlesport" / "hud.cpp"
        member = "?OnActivate@HudButton@@UAEXXZ"
        target = eligible_target(
            callers=(SourceSymbolPair(source, member, member),),
        )
        normalization = manifest_symbol_normalization((target,))
        self.assertNotIn(member, normalization)
        gates, _details = compare_objects(
            target,
            {source: code_object("_calld", member)},
            {source: code_object("@callr", member)},
            normalization=normalization,
        )
        self.assertTrue(all(row["passed"] for row in gates), gates)

    def test_raw_order_mechanically_normalizes_unlisted_zeroarg_definitions(
        self,
    ) -> None:
        callee_source = ROOT / "src" / "Battlesport" / "hud.cpp"
        caller_source = ROOT / "src" / "GameZRecoil" / "zui.cpp"
        target = eligible_target(
            callee_source=callee_source,
            callers=(
                SourceSymbolPair(caller_source, "_used", "@user"),
            ),
        )
        cases = (
            ("?Helper@@YAHXZ", "?Helper@@YIHXZ"),
            ("?Helper@Owner@@SAHXZ", "?Helper@Owner@@SIHXZ"),
            ("_helper", "@helper@0"),
        )
        for cdecl_helper, fastcall_helper in cases:
            with self.subTest(
                cdecl=cdecl_helper,
                fastcall=fastcall_helper,
            ):
                gates, _details = compare_objects(
                    target,
                    {
                        callee_source: code_object("_calld", cdecl_helper),
                        caller_source: external_call_object("_calld", "_used"),
                    },
                    {
                        callee_source: code_object("@callr", fastcall_helper),
                        caller_source: external_call_object("@callr", "@user"),
                    },
                    normalization=manifest_symbol_normalization((target,)),
                )
                self.assertTrue(all(row["passed"] for row in gates), gates)

    def test_raw_order_zeroarg_pairing_fails_on_unpaired_or_colliding_rows(
        self,
    ) -> None:
        with self.assertRaisesRegex(ValueError, "unpaired /Gd"):
            _paired_raw_definition_normalization(
                definition_object(["?Helper@@YAHXZ"]),
                definition_object(["unchanged"]),
                {},
            )
        with self.assertRaisesRegex(ValueError, "both transformed and stable"):
            _paired_raw_definition_normalization(
                definition_object(["?Helper@@YAHXZ"]),
                definition_object(
                    ["?Helper@@YAHXZ", "?Helper@@YIHXZ"]
                ),
                {},
            )

    def test_raw_order_pairs_unique_extern_c_stack_byte_suffix(self) -> None:
        normalization = _paired_raw_definition_normalization(
            definition_object(["_fabsf"]),
            definition_object(["@fabsf@4"]),
            {},
        )
        self.assertEqual(
            normalization["_fabsf"],
            normalization["@fabsf@4"],
        )
        self.assertEqual(
            "mechanical-extern-c-definition:_fabsf:"
            "fastcall-stack-bytes:4",
            normalization["_fabsf"],
        )

    def test_raw_order_pairs_exact_nonzeroarg_global_default_conventions(
        self,
    ) -> None:
        cdecl_names = [
            "?TransformPointByMatrix@Player@@"
            "YA?AUzVec3@@ABU2@ABUzMat4x3@@@Z",
            "?HudUiMgrObjective_SetSlidePosition@HudUiMgrObjective@@YAXM@Z",
            "?SetStaticSlide@HudUiMgrObjective@@SAXM@Z",
        ]
        fastcall_names = [
            "?TransformPointByMatrix@Player@@"
            "YI?AUzVec3@@ABU2@ABUzMat4x3@@@Z",
            "?HudUiMgrObjective_SetSlidePosition@HudUiMgrObjective@@YIXM@Z",
            "?SetStaticSlide@HudUiMgrObjective@@SIXM@Z",
        ]
        normalization = _paired_raw_definition_normalization(
            definition_object(cdecl_names),
            definition_object(fastcall_names),
            {},
        )
        for cdecl_name, fastcall_name in zip(cdecl_names, fastcall_names):
            with self.subTest(cdecl=cdecl_name):
                self.assertEqual(
                    normalization[cdecl_name],
                    normalization[fastcall_name],
                )
                self.assertTrue(
                    normalization[cdecl_name].startswith(
                        "mechanical-abi-definition:"
                    )
                )

    def test_raw_order_global_nonzeroarg_pairing_fails_closed(self) -> None:
        cdecl_name = (
            "?HudUiMgrObjective_SetSlidePosition@HudUiMgrObjective@@YAXM@Z"
        )
        fastcall_name = (
            "?HudUiMgrObjective_SetSlidePosition@HudUiMgrObjective@@YIXM@Z"
        )
        fastcall_tail_drift = (
            "?HudUiMgrObjective_SetSlidePosition@HudUiMgrObjective@@YIXH@Z"
        )
        with self.assertRaisesRegex(ValueError, "unpaired /Gd raw definition"):
            _paired_raw_definition_normalization(
                definition_object([cdecl_name]),
                definition_object([fastcall_tail_drift]),
                {},
            )
        with self.assertRaisesRegex(ValueError, "both transformed and stable"):
            _paired_raw_definition_normalization(
                definition_object([cdecl_name]),
                definition_object([cdecl_name, fastcall_name]),
                {},
            )
        with self.assertRaisesRegex(ValueError, "ambiguous paired raw definition"):
            _paired_raw_definition_normalization(
                definition_object([cdecl_name]),
                definition_object([fastcall_name, fastcall_name]),
                {},
            )

    def test_raw_order_rejects_ambiguous_extern_c_stack_byte_suffixes(
        self,
    ) -> None:
        with self.assertRaisesRegex(
            ValueError,
            "multiple /Gr stack-byte candidates",
        ):
            _paired_raw_definition_normalization(
                definition_object(["_fabsf"]),
                definition_object(["@fabsf@0", "@fabsf@4"]),
                {},
            )

    def test_raw_order_normalizes_only_exact_local_source_discriminator_pair(
        self,
    ) -> None:
        source = (ROOT / "src" / "Battlesport" / "player.cpp").resolve()

        def local_symbol(
            discriminator: str,
            convention: str,
            *,
            source_path: Path = source,
        ) -> str:
            return (
                "?PlayerAllocMasterCommonData@?%"
                f"{source_path}{discriminator}@@{convention}"
                "PAUPlayerMasterCommonData@@XZ"
            )

        cdecl_name = local_symbol("95422675", "YA")
        fastcall_name = local_symbol("202902678", "YI")
        normalization = _paired_raw_definition_normalization(
            definition_object([cdecl_name]),
            definition_object([fastcall_name]),
            {},
        )
        self.assertEqual(
            normalization[cdecl_name],
            normalization[fastcall_name],
        )

    def test_raw_order_normalizes_nonzeroarg_local_discriminator_pair(
        self,
    ) -> None:
        source = (ROOT / "src" / "Battlesport" / "player.cpp").resolve()

        def local_symbol(discriminator: str, convention: str, tail: str) -> str:
            return (
                "?PlayerAllocMasterCommonData@?%"
                f"{source}{discriminator}@@{convention}{tail}"
            )

        type_and_argument_tail = "PAUPlayerMasterCommonData@@H@Z"
        cdecl_name = local_symbol("95422675", "YA", type_and_argument_tail)
        fastcall_name = local_symbol("202902678", "YI", type_and_argument_tail)
        normalization = _paired_raw_definition_normalization(
            definition_object([cdecl_name]),
            definition_object([fastcall_name]),
            {},
        )
        self.assertEqual(
            normalization[cdecl_name],
            normalization[fastcall_name],
        )

    def test_raw_order_local_discriminator_rejects_type_tail_drift(self) -> None:
        source = (ROOT / "src" / "Battlesport" / "player.cpp").resolve()

        def local_symbol(discriminator: str, convention: str, tail: str) -> str:
            return (
                "?PlayerAllocMasterCommonData@?%"
                f"{source}{discriminator}@@{convention}{tail}"
            )

        with self.assertRaisesRegex(ValueError, "unpaired /Gd"):
            _paired_raw_definition_normalization(
                definition_object(
                    [
                        local_symbol(
                            "95422675",
                            "YA",
                            "PAUPlayerMasterCommonData@@H@Z",
                        )
                    ]
                ),
                definition_object(
                    [
                        local_symbol(
                            "202902678",
                            "YI",
                            "PAUPlayerMasterCommonData@@I@Z",
                        )
                    ]
                ),
                {},
            )

    def test_raw_order_local_discriminator_rejects_path_drift_and_collision(
        self,
    ) -> None:
        player = (ROOT / "src" / "Battlesport" / "player.cpp").resolve()
        hud = (ROOT / "src" / "Battlesport" / "hud.cpp").resolve()

        def local_symbol(
            source: Path,
            discriminator: str,
            convention: str,
        ) -> str:
            return (
                "?PlayerAllocMasterCommonData@?%"
                f"{source}{discriminator}@@{convention}"
                "PAUPlayerMasterCommonData@@XZ"
            )

        cdecl_name = local_symbol(player, "95422675", "YA")
        with self.assertRaisesRegex(ValueError, "unpaired /Gd"):
            _paired_raw_definition_normalization(
                definition_object([cdecl_name]),
                definition_object(
                    [local_symbol(hud, "202902678", "YI")]
                ),
                {},
            )
        with self.assertRaisesRegex(
            ValueError,
            "ambiguous Gr local definition",
        ):
            _paired_raw_definition_normalization(
                definition_object([cdecl_name]),
                definition_object(
                    [
                        local_symbol(player, "202902678", "YI"),
                        local_symbol(player, "314159265", "YI"),
                    ]
                ),
                {},
            )

    def test_raw_order_pairs_exact_local_stable_suffix_deleting_dtor(
        self,
    ) -> None:
        source = (ROOT / "src" / "Battlesport" / "hud.cpp").resolve()

        def deleting_dtor(discriminator: str, suffix: str = "UAEPAXI@Z") -> str:
            return (
                "??_GzFMV_ActionBlurStack@?%"
                f"{source}{discriminator}@@{suffix}"
            )

        cdecl_name = deleting_dtor("291167220")
        fastcall_name = deleting_dtor("70967224")
        normalization = _paired_raw_definition_normalization(
            definition_object([cdecl_name]),
            definition_object([fastcall_name]),
            {},
        )
        self.assertEqual(
            normalization[cdecl_name],
            normalization[fastcall_name],
        )

    def test_raw_order_local_stable_suffix_fails_closed(self) -> None:
        source = (ROOT / "src" / "Battlesport" / "hud.cpp").resolve()

        def local_symbol(
            discriminator: str,
            suffix: str,
        ) -> str:
            return (
                "??_GzFMV_ActionBlurStack@?%"
                f"{source}{discriminator}@@{suffix}"
            )

        cdecl_name = local_symbol("291167220", "UAEPAXI@Z")
        with self.assertRaisesRegex(ValueError, "unpaired /Gd raw definition"):
            _paired_raw_definition_normalization(
                definition_object([cdecl_name]),
                definition_object(
                    [local_symbol("70967224", "UAEPAXH@Z")]
                ),
                {},
            )

        with self.assertRaisesRegex(ValueError, "wrong /Gd side"):
            _paired_raw_definition_normalization(
                definition_object(
                    [
                        local_symbol(
                            "291167220",
                            "YIPAXI@Z",
                        )
                    ]
                ),
                definition_object(
                    [
                        local_symbol(
                            "70967224",
                            "YAPAXI@Z",
                        )
                    ]
                ),
                {},
            )

        for fastcall_names in (
            [
                local_symbol("70967224", "UAEPAXI@Z"),
                local_symbol("314159265", "UAEPAXI@Z"),
            ],
            [
                local_symbol("70967224", "UAEPAXI@Z"),
                local_symbol("70967224", "UAEPAXI@Z"),
            ],
        ):
            with self.subTest(fastcall_names=fastcall_names):
                with self.assertRaisesRegex(
                    ValueError,
                    "ambiguous Gr local definition",
                ):
                    _paired_raw_definition_normalization(
                        definition_object([cdecl_name]),
                        definition_object(fastcall_names),
                        {},
                    )

    def test_raw_order_ignores_only_definition_body_size(self) -> None:
        cdecl = definition_object(
            ["?Helper@@YAHXZ"],
            body_sizes=[1],
        )
        fastcall = definition_object(
            ["?Helper@@YIHXZ"],
            body_sizes=[9],
        )
        normalization = _paired_raw_definition_normalization(
            cdecl,
            fastcall,
            {},
        )
        cdecl_order = vc5_abi_equivalence._definition_order(
            cdecl,
            normalization,
        )
        fastcall_order = vc5_abi_equivalence._definition_order(
            fastcall,
            normalization,
        )
        self.assertEqual(cdecl_order, fastcall_order)
        self.assertNotIn("body_size", cdecl_order[0])

    def test_raw_order_ignores_only_definition_relocation_count(self) -> None:
        cdecl = definition_object(
            ["?Helper@@YAHXZ"],
            body_sizes=[8],
        )
        fastcall = definition_object(
            ["?Helper@@YIHXZ"],
            body_sizes=[8],
            relocation_offsets=[1],
        )
        normalization = _paired_raw_definition_normalization(
            cdecl,
            fastcall,
            {},
        )
        cdecl_order = vc5_abi_equivalence._definition_order(
            cdecl,
            normalization,
        )
        fastcall_order = vc5_abi_equivalence._definition_order(
            fastcall,
            normalization,
        )
        self.assertEqual(cdecl_order, fastcall_order)
        self.assertNotIn("relocation_count", cdecl_order[0])

    def test_raw_order_still_validates_function_extent(self) -> None:
        invalid = definition_object(
            ["?Helper@@YAHXZ"],
            symbol_values=[1],
        )
        with self.assertRaisesRegex(ValueError, "starts outside its section"):
            vc5_abi_equivalence._definition_order(invalid, {})

    def test_raw_order_still_rejects_structural_value_drift(self) -> None:
        cdecl = definition_object(
            ["?First@@YAHXZ", "?Second@@YAHXZ"],
            body_sizes=[1, 1],
        )
        fastcall = definition_object(
            ["?First@@YIHXZ", "?Second@@YIHXZ"],
            body_sizes=[2, 1],
        )
        normalization = _paired_raw_definition_normalization(
            cdecl,
            fastcall,
            {},
        )
        self.assertNotEqual(
            vc5_abi_equivalence._definition_order(cdecl, normalization),
            vc5_abi_equivalence._definition_order(fastcall, normalization),
        )

    def test_manifest_queue_is_exactly_targeted_and_paths_are_repository_relative(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as temp:
            root = Path(temp)
            source = root / "probe.cpp"
            source.write_text("void f() {}\n", encoding="utf-8")
            source_relative = source.relative_to(ROOT).as_posix()
            row = {
                "id": "sample",
                "identity": "recoil:function:sample",
                "callee_source": source_relative,
                "cdecl_symbol": "_calld",
                "fastcall_symbol": "@callr",
                "callers": [
                    {
                        "source": source_relative,
                        "cdecl_symbol": "_used",
                        "fastcall_symbol": "@user",
                    }
                ],
                "return_category": "void",
                "retail_evidence": eligible_target().retail_evidence,
                "eh_policy": {"kind": "none", "retail_proven": True},
            }
            manifest = root / "manifest.json"
            manifest.write_text(
                json.dumps(
                    {
                        "sources": [source_relative],
                        "zeroarg_abi_equivalence": [row],
                    }
                ),
                encoding="utf-8",
            )
            targets = load_zeroarg_targets(manifest)
            self.assertEqual([target.target_id for target in targets], ["sample"])
            row["callee_source"] = str(source.resolve())
            manifest.write_text(
                json.dumps(
                    {
                        "sources": [source_relative],
                        "zeroarg_abi_equivalence": [row],
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "repository-relative"):
                load_zeroarg_targets(manifest)

    def test_manifest_rejects_legacy_or_unlisted_source_rows(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as temp:
            root = Path(temp)
            source = root / "probe.cpp"
            other = root / "other.cpp"
            source.write_text("void f() {}\n", encoding="utf-8")
            other.write_text("void g() {}\n", encoding="utf-8")
            source_relative = source.relative_to(ROOT).as_posix()
            other_relative = other.relative_to(ROOT).as_posix()
            base = {
                "id": "sample",
                "identity": "recoil:function:sample",
                "callee_source": source_relative,
                "cdecl_symbol": "_calld",
                "fastcall_symbol": "@callr",
                "callers": [
                    {
                        "source": source_relative,
                        "cdecl_symbol": "_used",
                        "fastcall_symbol": "@user",
                    }
                ],
                "return_category": "void",
                "retail_evidence": eligible_target().retail_evidence,
                "eh_policy": {"kind": "none", "retail_proven": True},
            }
            manifest = root / "manifest.json"

            legacy = dict(base)
            legacy["source"] = legacy.pop("callee_source")
            manifest.write_text(
                json.dumps(
                    {
                        "sources": [source_relative],
                        "zeroarg_abi_equivalence": [legacy],
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "unsupported fields: source"):
                load_zeroarg_targets(manifest)

            unlisted = dict(base)
            unlisted["callers"] = [
                {
                    "source": other_relative,
                    "cdecl_symbol": "_used",
                    "fastcall_symbol": "@user",
                }
            ]
            manifest.write_text(
                json.dumps(
                    {
                        "sources": [source_relative],
                        "zeroarg_abi_equivalence": [unlisted],
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "exact configured source"):
                load_zeroarg_targets(manifest)

    def test_manifest_rejects_duplicate_paths_and_ambiguous_symbol_pairs(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as temp:
            root = Path(temp)
            source = root / "probe.cpp"
            source.write_text("void f() {}\n", encoding="utf-8")
            source_relative = source.relative_to(ROOT).as_posix()
            manifest = root / "manifest.json"
            manifest.write_text(
                json.dumps(
                    {
                        "sources": [source_relative, source_relative],
                        "zeroarg_abi_equivalence": [],
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "duplicate or ambiguous"):
                load_zeroarg_targets(manifest)

        target = eligible_target()
        ambiguous = eligible_target(
            target_id="other",
            identity="recoil:function:other",
            callee=SymbolPair("_calld", "@different"),
        )
        with self.assertRaisesRegex(ValueError, "ambiguous"):
            manifest_symbol_normalization((target, ambiguous))

    def test_manifest_rejects_duplicate_exact_caller_rows(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as temp:
            root = Path(temp)
            source = root / "probe.cpp"
            source.write_text("void f() {}\n", encoding="utf-8")
            relative = source.relative_to(ROOT).as_posix()
            caller = {
                "source": relative,
                "cdecl_symbol": "?Member@@QAEHXZ",
                "fastcall_symbol": "?Member@@QAEHXZ",
            }
            row = {
                "id": "sample",
                "identity": "recoil:function:sample",
                "callee_source": relative,
                "cdecl_symbol": "_calld",
                "fastcall_symbol": "@callr",
                "callers": [caller, dict(caller)],
                "return_category": "void",
                "retail_evidence": eligible_target().retail_evidence,
                "eh_policy": {"kind": "none", "retail_proven": True},
            }
            manifest = root / "manifest.json"
            manifest.write_text(
                json.dumps(
                    {
                        "sources": [relative],
                        "zeroarg_abi_equivalence": [row],
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "duplicate exact caller"):
                load_zeroarg_targets(manifest)

    def test_result_contract_grants_only_mechanical_cdecl_normalization(self) -> None:
        target = eligible_target()
        self.assertTrue(all(row["passed"] for row in eligibility_gates(target)))
        self.assertNotIn("original_abi", target.retail_evidence)


if __name__ == "__main__":
    unittest.main()
