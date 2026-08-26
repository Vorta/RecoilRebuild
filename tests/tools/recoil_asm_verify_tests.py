import sys
import struct
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.asm_verify import (  # noqa: E402
    CoffObject,
    bytes_from_hexdump,
    classify_instruction_differences,
    compare_bn_data_to_obj,
    compare_masked_byte_sequences,
    format_mask_lines,
    format_byte_triage_lines,
    mismatch_clusters,
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


def build_coff_data_object(raw_data: bytes, relocations: list[tuple[int, int, int]], symbols: list[bytes]) -> bytes:
    header_size = 20
    section_header_size = 40
    raw_offset = header_size + section_header_size
    relocation_offset = raw_offset + len(raw_data)
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
        coff_short_name(".rdata")
        + struct.pack("<IIIIIIHHI", 0, 0, len(raw_data), raw_offset, relocation_offset, 0, len(relocations), 0, 0x40)
    )
    relocation_bytes = b"".join(struct.pack("<IIH", offset, symbol_index, relocation_type) for offset, symbol_index, relocation_type in relocations)
    return header + section + raw_data + relocation_bytes + b"".join(symbols) + string_table


def build_coff_object_with_bss_before_text(raw_code: bytes, bss_size: int, symbols: list[bytes]) -> bytes:
    header_size = 20
    section_header_size = 40
    text_raw_offset = header_size + section_header_size * 2
    symbol_table_offset = text_raw_offset + len(raw_code)
    string_table = struct.pack("<I", 4)
    header = struct.pack(
        "<HHIIIHH",
        0x14C,
        2,
        0,
        symbol_table_offset,
        len(symbols),
        0,
        0,
    )
    bss_section = (
        coff_short_name(".bss")
        + struct.pack("<IIIIIIHHI", 0, 0, bss_size, 0, 0, 0, 0, 0, 0x80)
    )
    text_section = (
        coff_short_name(".text")
        + struct.pack("<IIIIIIHHI", 0, 0, len(raw_code), text_raw_offset, 0, 0, 0, 0, 0x20)
    )
    return header + bss_section + text_section + raw_code + b"".join(symbols) + string_table


def build_coff_object_with_truncated_text(raw_size: int, raw_offset: int) -> bytes:
    header_size = 20
    section_header_size = 40
    symbol_table_offset = header_size + section_header_size
    string_table = struct.pack("<I", 4)
    header = struct.pack(
        "<HHIIIHH",
        0x14C,
        1,
        0,
        symbol_table_offset,
        0,
        0,
        0,
    )
    section = (
        coff_short_name(".text")
        + struct.pack("<IIIIIIHHI", 0, 0, raw_size, raw_offset, 0, 0, 0, 0, 0x20)
    )
    return header + section + string_table


class RecoilAsmVerifyTests(unittest.TestCase):
    def test_coff_symbol_preserves_section_and_weak_auxiliary_metadata(self):
        section_symbol = coff_short_name(".text") + struct.pack("<IhHBB", 0, 1, 0, 3, 1)
        section_aux = struct.pack("<IHHIhBBH", 4, 0, 0, 0, 1, 5, 0, 0)
        function_symbol = coff_symbol("_sample", 0, 1)
        coff = CoffObject.from_bytes(
            build_coff_object(b"\xc3\x00\x00\x00", [], [section_symbol, section_aux, function_symbol])
        )

        section = coff.symbols[0]
        self.assertEqual(5, section.section_definition_selection)
        self.assertEqual(1, section.section_definition_association)
        self.assertEqual(1, len(section.aux_records))

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

    def test_bytes_from_hexdump_ignores_labels_and_ascii(self):
        hexdump = "\n".join(
            [
                "4a3ef0  zSnd::ReportA3DError:",
                "4a3ef0  81 ec 00 01 00 00 85 c9 56 8b f2 0f 8f 05 03 00  ........V.......",
                "4a3f00  00 0f 84 f0                                      ....",
            ]
        )

        data = bytes_from_hexdump(hexdump, expected_length=20)

        self.assertEqual(
            bytes.fromhex("81 ec 00 01 00 00 85 c9 56 8b f2 0f 8f 05 03 00 00 0f 84 f0"),
            data,
        )

    def test_bytes_from_hexdump_stops_before_hex_looking_ascii_on_short_line(self):
        hexdump = "\n".join(
            [
                "4dadf4  52 65 61 64 46 69 6c 65 20 66 61 69 6c 65 64 20  ReadFile failed ",
                "4dae04  25 73                                            ad %s.",
            ]
        )

        data = bytes_from_hexdump(hexdump, expected_length=18)

        self.assertEqual(bytes.fromhex("52 65 61 64 46 69 6c 65 20 66 61 69 6c 65 64 20 25 73"), data)

    def test_bytes_from_hexdump_rejects_wrong_length(self):
        with self.assertRaisesRegex(ValueError, "expected 2"):
            bytes_from_hexdump("401000  c3  .", expected_length=2)

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
        self.assertEqual(len(raw_code), function.natural_end)
        self.assertEqual(0, function.excluded_tail_size)
        self.assertEqual(0, function.excluded_tail_relocation_count)
        self.assertEqual("_Target", function.relocations[0].symbol_name)
        self.assertEqual([1, 2, 3, 4], [index for index, masked in enumerate(function.relocation_mask) if masked])

    def test_coff_function_bytes_explicit_length_crops_and_reports_natural_tail(self):
        raw_code = bytes.fromhex("55 8b ec 33 c0 5d c3 90")
        symbols = [coff_symbol("_Sample", 0, 1)]
        obj = CoffObject.from_bytes(build_coff_object(raw_code, [], symbols))

        function = obj.function_bytes("_Sample", byte_length=7)

        self.assertEqual(raw_code[:7], function.data)
        self.assertEqual(7, function.end)
        self.assertEqual(8, function.natural_end)
        self.assertEqual(1, function.excluded_tail_size)
        self.assertEqual(0, function.excluded_tail_relocation_count)
        report = "\n".join(format_mask_lines(function))
        self.assertIn("compared_range: 0x0..0x7", report)
        self.assertIn("natural_coff_range: 0x0..0x8", report)
        self.assertIn("excluded_natural_tail_bytes: 1", report)
        self.assertIn("excluded_tail_disposition: inventory-only; not compared or accepted", report)

    def test_coff_function_bytes_rejects_invalid_explicit_lengths(self):
        raw_code = bytes.fromhex("55 8b ec 5d c3")
        symbols = [coff_symbol("_Sample", 0, 1)]
        obj = CoffObject.from_bytes(build_coff_object(raw_code, [], symbols))

        with self.assertRaisesRegex(ValueError, "must be positive"):
            obj.function_bytes("_Sample", byte_length=0)
        with self.assertRaisesRegex(ValueError, "past natural COFF extent"):
            obj.function_bytes("_Sample", byte_length=6)

    def test_coff_function_bytes_rejects_relocation_crossing_explicit_extent(self):
        raw_code = bytes.fromhex("90 90 00 00 00 00 c3 90")
        symbols = [
            coff_symbol("_Sample", 0, 1),
            coff_symbol("_Target", 0, 0, 0),
        ]
        obj = CoffObject.from_bytes(build_coff_object(raw_code, [(2, 1, 0x0006)], symbols))

        with self.assertRaisesRegex(ValueError, "extends past _Sample byte range"):
            obj.function_bytes("_Sample", byte_length=4)

    def test_coff_function_bytes_inventories_relocation_wholly_in_excluded_tail(self):
        raw_code = bytes.fromhex("55 8b ec c3 00 00 00 00")
        symbols = [
            coff_symbol("_Sample", 0, 1),
            coff_symbol("_Target", 0, 0, 0),
        ]
        obj = CoffObject.from_bytes(build_coff_object(raw_code, [(4, 1, 0x0006)], symbols))

        function = obj.function_bytes("_Sample", byte_length=4)

        self.assertEqual(raw_code[:4], function.data)
        self.assertEqual(1, function.excluded_tail_relocation_count)
        self.assertEqual((), function.relocations)

    def test_coff_data_symbol_bytes_include_relocation_mask(self):
        raw_data = bytes.fromhex("00 00 00 00 11 22 33 44")
        symbols = [
            coff_symbol("_Table", 0, 1, 0, 2),
            coff_symbol("_Target", 0, 0, 0),
        ]
        obj = CoffObject.from_bytes(build_coff_data_object(raw_data, [(0, 1, 0x0006)], symbols))

        data_symbol = obj.data_symbol_bytes("_Table", byte_length=8)

        self.assertEqual(raw_data, data_symbol.data)
        self.assertEqual("_Target", data_symbol.relocations[0].symbol_name)
        self.assertEqual([0, 1, 2, 3], [index for index, masked in enumerate(data_symbol.relocation_mask) if masked])

    def test_coff_data_symbol_bytes_select_member_slice_and_rebase_relocation_mask(self):
        raw_data = bytes.fromhex("aa bb cc dd 00 00 00 00 11 22 33 44")
        symbols = [
            coff_symbol("_Table", 0, 1, 0, 2),
            coff_symbol("_Target", 0, 0, 0),
        ]
        obj = CoffObject.from_bytes(build_coff_data_object(raw_data, [(4, 1, 0x0006)], symbols))

        data_symbol = obj.data_symbol_bytes("_Table", object_offset=4, byte_length=8)

        self.assertEqual(raw_data[4:12], data_symbol.data)
        self.assertEqual(0, data_symbol.symbol_start)
        self.assertEqual(4, data_symbol.object_offset)
        self.assertEqual(4, data_symbol.start)
        self.assertEqual(12, data_symbol.end)
        self.assertEqual("_Target", data_symbol.relocations[0].symbol_name)
        self.assertEqual(
            [0, 1, 2, 3],
            [index for index, masked in enumerate(data_symbol.relocation_mask) if masked],
        )

    def test_compare_bn_data_to_obj_writes_data_specific_relocation_mask(self):
        raw_data = bytes.fromhex("00 00 00 00 11 22 33 44")
        symbols = [
            coff_symbol("_Table", 0, 1, 0, 2),
            coff_symbol("_Target", 0, 0, 0),
        ]
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            obj_path = root / "sample.obj"
            obj_path.write_bytes(build_coff_data_object(raw_data, [(0, 1, 0x0006)], symbols))
            bn_path = root / "bn.txt"
            bn_path.write_text("00401000: 78 56 34 12 11 22 33 44\n", encoding="utf-8")

            comparison = compare_bn_data_to_obj(
                address="0x401000",
                obj_path=obj_path,
                symbol="_Table",
                byte_length=8,
                out_dir=root / "out",
                bridge_url="",
                bn_hexdump_path=bn_path,
            )

            report = comparison.mask_path.read_text(encoding="utf-8")
            self.assertEqual(0, comparison.mismatch_count)
            self.assertIn("symbol: _Table", report)
            self.assertIn("section: .rdata#1", report)
            self.assertIn("data_range: 0x0..0x8", report)
            self.assertIn("relocations: 1", report)
            self.assertIn("0x00000000 size=4 type=DIR32 symbol=_Target", report)
            self.assertNotIn("natural_coff_range", report)
            self.assertNotIn("excluded_natural_tail", report)

    def test_compare_bn_data_to_obj_reports_selected_object_slice(self):
        raw_data = bytes.fromhex("aa bb cc dd 00 00 00 00 11 22 33 44")
        symbols = [
            coff_symbol("_Table", 0, 1, 0, 2),
            coff_symbol("_Target", 0, 0, 0),
        ]
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            obj_path = root / "sample.obj"
            obj_path.write_bytes(build_coff_data_object(raw_data, [(4, 1, 0x0006)], symbols))
            bn_path = root / "bn.txt"
            bn_path.write_text("00401000: 78 56 34 12 11 22 33 44\n", encoding="utf-8")

            comparison = compare_bn_data_to_obj(
                address="0x401000",
                obj_path=obj_path,
                symbol="_Table",
                object_offset=4,
                byte_length=8,
                out_dir=root / "out",
                bridge_url="",
                bn_hexdump_path=bn_path,
            )

            mask_report = comparison.mask_path.read_text(encoding="utf-8")
            identity_report = comparison.relocation_identity_path.read_text(encoding="utf-8")
            vc5_dump = comparison.vc5_path.read_text(encoding="utf-8")
            self.assertEqual(0, comparison.mismatch_count)
            self.assertIn("symbol_start: 0x0", mask_report)
            self.assertIn("object_offset: 4 (0x4)", mask_report)
            self.assertIn("data_range: 0x4..0xc", mask_report)
            self.assertIn("0x00000000 size=4 type=DIR32 symbol=_Target", mask_report)
            self.assertIn("object_offset: 4 (0x4)", identity_report)
            self.assertIn("range: 0x4..0xc", identity_report)
            self.assertTrue(vc5_dump.startswith("00000004:"))

    def test_coff_data_symbol_rejects_relocation_past_explicit_length(self):
        raw_data = bytes.fromhex("00 00 00 00 11 22 33 44")
        symbols = [
            coff_symbol("_Table", 0, 1, 0, 2),
            coff_symbol("_Target", 0, 0, 0),
        ]
        obj = CoffObject.from_bytes(build_coff_data_object(raw_data, [(2, 1, 0x0006)], symbols))

        with self.assertRaisesRegex(ValueError, "crosses _Table data slice boundary"):
            obj.data_symbol_bytes("_Table", byte_length=4)

    def test_coff_data_symbol_rejects_relocation_crossing_slice_start(self):
        raw_data = bytes.fromhex("00 00 00 00 11 22 33 44")
        symbols = [
            coff_symbol("_Table", 0, 1, 0, 2),
            coff_symbol("_Target", 0, 0, 0),
        ]
        obj = CoffObject.from_bytes(build_coff_data_object(raw_data, [(2, 1, 0x0006)], symbols))

        with self.assertRaisesRegex(ValueError, "crosses _Table data slice boundary"):
            obj.data_symbol_bytes("_Table", object_offset=4, byte_length=4)

    def test_coff_data_symbol_rejects_relocation_crossing_slice_end(self):
        raw_data = bytes.fromhex("00 01 02 03 04 05 06 07 08 09 0a 0b")
        symbols = [
            coff_symbol("_Table", 0, 1, 0, 2),
            coff_symbol("_Target", 0, 0, 0),
        ]
        obj = CoffObject.from_bytes(build_coff_data_object(raw_data, [(6, 1, 0x0006)], symbols))

        with self.assertRaisesRegex(ValueError, "crosses _Table data slice boundary"):
            obj.data_symbol_bytes("_Table", object_offset=4, byte_length=4)

    def test_coff_data_symbol_rejects_nonzero_slice_past_natural_symbol_extent(self):
        raw_data = bytes.fromhex("00 01 02 03 04 05 06 07 08 09 0a 0b")
        symbols = [
            coff_symbol("_Table", 0, 1),
            coff_symbol("_Next", 8, 1),
        ]
        obj = CoffObject.from_bytes(build_coff_data_object(raw_data, [], symbols))

        with self.assertRaisesRegex(ValueError, "slice extends past its natural COFF extent"):
            obj.data_symbol_bytes("_Table", object_offset=4, byte_length=8)
        with self.assertRaisesRegex(ValueError, "object offset is outside its natural COFF extent"):
            obj.data_symbol_bytes("_Table", object_offset=8, byte_length=1)

    def test_coff_data_symbol_rejects_invalid_object_offset(self):
        raw_data = bytes.fromhex("00 01 02 03")
        symbols = [coff_symbol("_Table", 0, 1)]
        obj = CoffObject.from_bytes(build_coff_data_object(raw_data, [], symbols))

        for value in (-1, True, 1.5, "1"):
            with self.subTest(value=value):
                with self.assertRaisesRegex(ValueError, "object offset must be a non-negative integer"):
                    obj.data_symbol_bytes("_Table", object_offset=value, byte_length=1)

    def test_coff_data_symbol_rejects_code_section_symbol(self):
        raw_code = bytes.fromhex("90 c3")
        symbols = [coff_symbol("_Sample", 0, 1)]
        obj = CoffObject.from_bytes(build_coff_object(raw_code, [], symbols))

        with self.assertRaisesRegex(ValueError, "not a data section"):
            obj.data_symbol_bytes("_Sample")

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

    def test_coff_bss_section_without_raw_bytes_is_zero_filled(self):
        raw_code = bytes.fromhex("90 c3")
        symbols = [coff_symbol("_Sample", 0, 2)]
        obj = CoffObject.from_bytes(build_coff_object_with_bss_before_text(raw_code, 16, symbols))

        self.assertEqual(b"\x00" * 16, obj.section(1).raw_data)
        self.assertEqual(raw_code, obj.function_bytes("_Sample").data)

    def test_coff_truncated_text_section_is_rejected(self):
        obj_bytes = build_coff_object_with_truncated_text(raw_size=4, raw_offset=999)

        with self.assertRaisesRegex(ValueError, r"COFF section \.text raw data is truncated"):
            CoffObject.from_bytes(obj_bytes)

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
        self.assertEqual(2, result.trailing_vc5_nops_trimmed)

    def test_mismatch_clusters_group_nearby_offsets(self):
        result = compare_masked_byte_sequences(
            bytes.fromhex("01 02 03 04 05 06"),
            bytes.fromhex("01 ff ee 04 dd cc"),
            (False, False, False, False, False, False),
            trim_padding_nops=False,
        )

        self.assertEqual([(1, 2, 2), (4, 5, 2)], mismatch_clusters(result.mismatches, max_gap=1))

    def test_byte_triage_reports_first_cluster_and_next_checks(self):
        result = compare_masked_byte_sequences(
            bytes.fromhex("55 8b ec c3"),
            bytes.fromhex("55 8b 00 c3 90"),
            (False, False, False, False, False),
            trim_padding_nops=False,
        )

        report = "\n".join(
            format_byte_triage_lines(address="0x401000", symbol="_Sample", comparison=result)
        )

        self.assertIn("status: FAIL", report)
        self.assertIn("size_delta_vc5_minus_bn: 1", report)
        self.assertIn("first_mismatch_original: 0x401002", report)
        self.assertIn("Mismatch clusters:", report)
        self.assertIn("Function size differs", report)


if __name__ == "__main__":
    unittest.main()
