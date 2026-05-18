import sys
import struct
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_asm_verify import (  # noqa: E402
    CoffObject,
    classify_instruction_differences,
    compare_masked_byte_sequences,
    normalize_instruction_line,
    parse_assembly,
)


def coff_short_name(name: str) -> bytes:
    encoded = name.encode("ascii")
    if len(encoded) > 8:
        raise ValueError(name)
    return encoded.ljust(8, b"\x00")


def coff_symbol(name: str, value: int, section: int, symbol_type: int = 0x20, storage_class: int = 2) -> bytes:
    return coff_short_name(name) + struct.pack("<IhHBB", value, section, symbol_type, storage_class, 0)


def build_coff_object(raw_code: bytes, relocations: list[tuple[int, int, int]], symbols: list[bytes]) -> bytes:
    header_size = 20
    section_header_size = 40
    raw_offset = header_size + section_header_size
    relocation_offset = raw_offset + len(raw_code)
    symbol_table_offset = relocation_offset + len(relocations) * 10
    string_table = struct.pack("<I", 4)
    header = struct.pack(
        "<HHIIIHH",
        0x14C,
        1,
        0,
        symbol_table_offset,
        len(symbols),
        0,
        0,
    )
    section = (
        coff_short_name(".text")
        + struct.pack("<IIIIIIHHI", 0, 0, len(raw_code), raw_offset, relocation_offset, 0, len(relocations), 0, 0x20)
    )
    relocation_bytes = b"".join(struct.pack("<IIH", offset, symbol_index, relocation_type) for offset, symbol_index, relocation_type in relocations)
    return header + section + raw_code + relocation_bytes + b"".join(symbols) + string_table


class RecoilAsmVerifyTests(unittest.TestCase):
    def test_ret_zero_normalizes_to_retn(self):
        bn = parse_assembly("004b3544  c3               retn", source="bn")
        cod = parse_assembly("  00034\tc3\t\t ret\t 0", source="cod")

        self.assertEqual(["retn"], [instruction.text for instruction in bn])
        self.assertEqual(["retn"], [instruction.text for instruction in cod])
        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("normalized_spelling_matches: 1", "\n".join(report))

    def test_signed_decimal_word_immediate_normalizes_to_hex(self):
        self.assertEqual(
            "and ax, 0xf000",
            normalize_instruction_line("  0001e\t66 25 00 f0\t and\t ax, -4096", source="cod"),
        )

    def test_signed_decimal_byte_immediate_normalizes_to_hex(self):
        self.assertEqual(
            "or ah, 0x80",
            normalize_instruction_line("  0004f\t80 cc 80\t or\t ah, -128", source="cod"),
        )

    def test_decimal_dword_immediate_normalizes_to_hex(self):
        self.assertEqual(
            "and eax, 0xffff",
            normalize_instruction_line("  00039\t25 ff ff 00 00\t and\t eax, 65535", source="cod"),
        )

    def test_multiline_cod_bytes_are_combined(self):
        instructions = parse_assembly(
            "  00045\t8b 0d 00 00 00\n"
            "\t00\t\t mov\t ecx, DWORD PTR _g_zSys_CpuVendorNonIntelMarker",
            source="cod",
        )

        self.assertEqual(1, len(instructions))
        self.assertEqual(("8b", "0d", "00", "00", "00", "00"), instructions[0].bytes)
        self.assertEqual("mov ecx, dword _g_zSys_CpuVendorNonIntelMarker", instructions[0].text)

    def test_byte_identical_branch_label_difference_is_classified(self):
        bn = parse_assembly("004b3428  74 07            je      0x4b3431", source="bn")
        cod = parse_assembly("  00008\t74 07\t\t je\t SHORT $L242", source="cod")

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[byte-identical spelling]", "\n".join(report))

    def test_call_relocation_placeholder_is_classified(self):
        bn = parse_assembly("004b3420  e8 cb ff ff ff   call    zSys::HasCpuidSupport", source="bn")
        cod = parse_assembly(
            "  00000\te8 00 00 00 00\t call\t ?HasCpuidSupport@zSys@@YAHXZ",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_mov_immediate_relocation_placeholder_is_classified(self):
        bn = parse_assembly("00407179  c7 06 50 cd 4c 00 mov     dword [esi], 0x4ccd50", source="bn")
        cod = parse_assembly(
            "  00009\tc7 06 00 00 00 00\t mov\t DWORD PTR [esi], OFFSET ?g_RecoilStateBase_Vtbl@@3URecoilApp_IState_Vtbl@@A",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_mov_global_load_symbol_relocation_is_classified(self):
        bn = parse_assembly("00415630  8b 0d 6c dc 4e 00 mov     ecx, dword [0x4edc6c]", source="bn")
        cod = parse_assembly(
            "  00000\t8b 0d 14 00 00 00\t mov\t ecx, DWORD PTR ?g_RecoilState_MainMenuTransition@@3URecoilStateMainMenuTransition@@A+20",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_mov_global_store_symbol_relocation_is_classified(self):
        bn = parse_assembly("00415650  89 0d 64 dc 4e 00 mov     dword [0x4edc64], ecx", source="bn")
        cod = parse_assembly(
            "  00000\t89 0d 0c 00 00 00\t mov\t DWORD PTR ?g_RecoilState_MainMenuTransition@@3URecoilStateMainMenuTransition@@A+12, ecx",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_mov_eax_to_absolute_symbol_plus_offset_relocation_is_classified(self):
        bn = parse_assembly("00490375  a3 74 20 63 00   mov     dword [0x632074], eax", source="bn")
        cod = parse_assembly(
            "  00035\ta3 08 00 00 00\t mov\t DWORD PTR ?g_activeRegionRect@zRndr@@3UActiveRegionRectPartial@1@A+8, eax",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_imul_eax_absolute_symbol_relocation_is_classified(self):
        bn = parse_assembly("0049039c  0f af 05 60 20 63 00 imul    eax, dword [0x632060]", source="bn")
        cod = parse_assembly(
            "  0005c\t0f af 05 00 00\n"
            "\t00 00\t\t imul\t eax, DWORD PTR ?g_bytesPerPixel@zRndr@@3HA",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_push_symbol_relocation_is_classified(self):
        bn = parse_assembly("00415658  68 58 dc 4e 00   push    0x4edc58", source="bn")
        cod = parse_assembly(
            "  00008\t68 00 00 00 00\t push\t OFFSET ?g_RecoilState_MainMenuTransition@@3URecoilStateMainMenuTransition@@A",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_mov_register_immediate_symbol_relocation_is_classified(self):
        bn = parse_assembly("0041565d  b9 a8 3c 4f 00   mov     ecx, 0x4f3ca8", source="bn")
        cod = parse_assembly(
            "  0000d\tb9 00 00 00 00\t mov\t ecx, OFFSET ?g_RecoilApp@@3URecoilApp@@A",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_mov_eax_absolute_symbol_relocation_is_classified(self):
        bn = parse_assembly("004306f1  a1 e8 bb 56 00   mov     eax, dword [0x56bbe8]", source="bn")
        cod = parse_assembly(
            "  00001\ta1 00 00 00 00\t mov\t eax, DWORD PTR ?g_zVideo_ActiveRendererPath@@3HA",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_import_indirect_call_relocation_is_classified(self):
        bn = parse_assembly("004a5ad4  ff 15 b8 c0 4c 00 call    dword [LoadLibraryA]", source="bn")
        cod = parse_assembly(
            "  00004\tff 15 00 00 00 00\t call\t DWORD PTR __imp__LoadLibraryA@4",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_import_iat_cell_symbol_relocation_is_classified(self):
        bn = parse_assembly("0040c466  8b 3d b8 c0 4c 00 mov     edi, dword [0x4cc0b8]", source="bn")
        cod = parse_assembly(
            "  000f6\t8b 3d 00 00 00 00\t mov\t edi, DWORD PTR _LoadLibraryA",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_virtual_call_argument_push_vtable_load_schedule_is_classified(self):
        bn = parse_assembly(
            "00408f62  6a 00            push    0x0\n"
            "00408f64  8b 01            mov     eax, dword [ecx]",
            source="bn",
        )
        cod = parse_assembly(
            "  00012\t8b 01\t\t mov\t eax, DWORD PTR [ecx]\n"
            "  00014\t6a 00\t\t push\t 0",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        joined = "\n".join(report)
        self.assertIn("[schedule-equivalent]", joined)
        self.assertIn("schedule_equivalent_differences: 2", joined)

    def test_indirect_jump_symbol_relocation_is_classified(self):
        bn = parse_assembly("004a68d5  ff 25 c0 33 63 00 jmp     dword [0x6333c0]", source="bn")
        cod = parse_assembly(
            "  00005\tff 25 00 00 00\n"
            "\t00\t\t jmp\t DWORD PTR ?g_zVideo_pfnUnlockSurfaceState@@3P6IHPAUzVideo_SurfaceStatePartial@@@ZA",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_cmp_absolute_symbol_relocation_is_classified(self):
        bn = parse_assembly("0048f519  39 05 50 20 63 00 cmp     dword [0x632050], eax", source="bn")
        cod = parse_assembly(
            "  0000e\t39 05 00 00 00\n"
            "\t00\t\t cmp\t DWORD PTR ?g_frameBuffer@zRndr@@3PAXA, eax",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_cmp_register_offset_symbol_relocation_is_classified(self):
        bn = parse_assembly("004a7fcf  81 fe 40 22 63 00 cmp     esi, 0x632240", source="bn")
        cod = parse_assembly(
            "  0000f\t81 fe 00 00 00\n"
            "\t00\t\t cmp\t esi, OFFSET FLAT:?g_zVideo_DisplayModeSurfaceState@@3UzVideo_SurfaceStatePartial@@A",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_inc_absolute_symbol_relocation_is_classified(self):
        bn = parse_assembly("004a6919  ff 05 d8 bb 56 00 inc     dword [0x56bbd8]", source="bn")
        cod = parse_assembly(
            "  00019\tff 05 00 00 00\n"
            "\t00\t\t inc\t DWORD PTR ?g_zVideo_FrameTick@@3HA",
            source="cod",
        )

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(0, mismatches)
        self.assertIn("[relocation-sensitive]", "\n".join(report))

    def test_real_opcode_difference_is_mismatch(self):
        bn = parse_assembly("004b353e  83 e0 01         and     eax, 0x1", source="bn")
        cod = parse_assembly("  0002e\t83 c8 01\t or\t eax, 1", source="cod")

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(1, mismatches)
        self.assertIn("[mismatch]", "\n".join(report))

    def test_coff_function_bytes_include_relocation_mask(self):
        raw_code = bytes.fromhex("b8 00 00 00 00 c3 90")
        symbols = [
            coff_symbol("_Sample", 0, 1),
            coff_symbol("_Target", 0, 0, 0),
        ]
        obj = CoffObject.from_bytes(build_coff_object(raw_code, [(1, 1, 0x0006)], symbols))

        function = obj.function_bytes("_Sample")

        self.assertEqual(raw_code, function.data)
        self.assertEqual("_Target", function.relocations[0].symbol_name)
        self.assertEqual([1, 2, 3, 4], [index for index, masked in enumerate(function.relocation_mask) if masked])

    def test_coff_function_end_ignores_internal_label_symbols(self):
        raw_code = bytes.fromhex("90 c3 90 c3")
        symbols = [
            coff_symbol("_First", 0, 1),
            coff_symbol("$L1", 1, 1, 0, 6),
            coff_symbol("_Second", 2, 1),
        ]
        obj = CoffObject.from_bytes(build_coff_object(raw_code, [], symbols))

        function = obj.function_bytes("_First")

        self.assertEqual(bytes.fromhex("90 c3"), function.data)

    def test_masked_byte_compare_accepts_relocated_operand_bytes(self):
        result = compare_masked_byte_sequences(
            bytes.fromhex("b8 78 56 34 12 c3"),
            bytes.fromhex("b8 00 00 00 00 c3"),
            (False, True, True, True, True, False),
        )

        self.assertEqual(0, result.mismatch_count)
        self.assertEqual(4, result.relocation_masked_bytes)

    def test_masked_byte_compare_rejects_unmasked_opcode_difference(self):
        result = compare_masked_byte_sequences(
            bytes.fromhex("b8 78 56 34 12 c3"),
            bytes.fromhex("b9 00 00 00 00 c3"),
            (False, True, True, True, True, False),
        )

        self.assertEqual(1, result.mismatch_count)
        self.assertEqual(0, result.mismatches[0].offset)

    def test_masked_byte_compare_trims_trailing_padding_nops(self):
        result = compare_masked_byte_sequences(
            bytes.fromhex("c3"),
            bytes.fromhex("c3 90 90"),
            (False, False, False),
        )

        self.assertEqual(0, result.mismatch_count)
        self.assertEqual(2, result.trailing_vc6_nops_trimmed)


if __name__ == "__main__":
    unittest.main()
