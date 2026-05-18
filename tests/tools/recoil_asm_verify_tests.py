import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_asm_verify import (  # noqa: E402
    classify_instruction_differences,
    normalize_instruction_line,
    parse_assembly,
)


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

    def test_real_opcode_difference_is_mismatch(self):
        bn = parse_assembly("004b353e  83 e0 01         and     eax, 0x1", source="bn")
        cod = parse_assembly("  0002e\t83 c8 01\t or\t eax, 1", source="cod")

        report, mismatches = classify_instruction_differences(bn, cod)
        self.assertEqual(1, mismatches)
        self.assertIn("[mismatch]", "\n".join(report))


if __name__ == "__main__":
    unittest.main()
