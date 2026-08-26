from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "no_raw_assembly.py"


class RecoilNoRawAssemblyTests(unittest.TestCase):
    def run_guard(self, root: Path, *extra: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                str(SCRIPT),
                "--root",
                str(root),
                *extra,
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def make_temp(self) -> tuple[tempfile.TemporaryDirectory[str], Path]:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name) / "src"
        root.mkdir()
        return temp, root

    def run_cpu_probe_source(
        self,
        source_text: str,
        *,
        address: str = "0x401000",
    ) -> subprocess.CompletedProcess[str]:
        temp, root = self.make_temp()
        (root / "probe.cpp").write_text(source_text, encoding="utf-8")
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            f"probe.cpp {address} __asm cpu-probe documented CPU probe exception\n",
            encoding="utf-8",
        )
        return self.run_guard(root, "--allowlist", str(allowlist))

    def canonical_function_docblock(
        self,
        address: str,
        *,
        anchor: str = "test-function",
        relation: str = "defines",
        section: str = ".text",
        artifact_kind: str = "function",
        opening: str = "/**",
    ) -> str:
        return (
            f"{opening}\n"
            f" * @recoil-anchor recoil:anchor:{anchor}\n"
            f" * @recoil-artifact {relation} {section} "
            f"recoil:{artifact_kind}:{address}: Test.\n"
            " * Raw assembly: keeps the VC5 retail byte shape after C++ variants failed.\n"
            " * Purpose: Exercises the documented minimal raw assembly exception.\n"
            " */\n"
        )

    def compiler_provider_docblock(self, address: str = "0x40a170") -> str:
        return (
            "/**\n"
            " * Purpose: provide the exact compiler-provider body for retail "
            f"{address} after\n"
            " * credible VC5SP3 C++ variants failed to reproduce its exact output.\n"
            " */\n"
        )

    def test_clean_source_passes(self) -> None:
        _, root = self.make_temp()
        (root / "clean.cpp").write_text("void f() { int value = 1; }\n", encoding="utf-8")

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_comments_and_strings_are_ignored(self) -> None:
        _, root = self.make_temp()
        (root / "ignored.cpp").write_text(
            'const char *s = "__asm";\n'
            "// __asm { nop }\n"
            "/* __declspec(naked) */\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_raw_asm_fails(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text("void f() { __asm { nop } }\n", encoding="utf-8")

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("bad.cpp:1", result.stdout)
        self.assertIn("__asm", result.stdout)

    def test_stable_region_uses_exact_consumer_identity_after_path_move(self) -> None:
        temp, root = self.make_temp()
        region_id = "recoil:raw-asm:test.moved-region"
        artifact_id = "recoil:function:0x401000"
        (root / "moved.h").write_text(
            "/**\n"
            f" * @recoil-raw-asm {region_id}\n"
            f" * @recoil-raw-consumer {region_id} {artifact_id}\n"
            " * Raw assembly: VC5 retail byte shape after C++ variants failed.\n"
            " * Purpose: Exercises a stable moved raw region.\n"
            " */\n"
            "void MovedRegion() { __asm { nop } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            f"{region_id} {artifact_id} __asm source-faithful-inline-asm "
            "documented moved region\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_stable_region_rejects_nonexact_allowlist_consumer_set(self) -> None:
        temp, root = self.make_temp()
        region_id = "recoil:raw-asm:test.consumer-set"
        artifact_id = "recoil:function:0x401000"
        (root / "region.cpp").write_text(
            "/**\n"
            f" * @recoil-raw-asm {region_id}\n"
            f" * @recoil-raw-consumer {region_id} {artifact_id}\n"
            " * Raw assembly: VC5 retail byte shape after C++ variants failed.\n"
            " * Purpose: Exercises exact consumer-set rejection.\n"
            " */\n"
            "void Region() { __asm { nop } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            f"{region_id} {artifact_id} __asm source-faithful-inline-asm first\n"
            f"{region_id} recoil:function:0x401020 __asm "
            "source-faithful-inline-asm unlisted-consumer\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("consumer set is not an exact match", result.stdout)

    def test_assembly_file_fails(self) -> None:
        _, root = self.make_temp()
        (root / "bad.asm").write_text("nop\n", encoding="utf-8")

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("bad.asm:1", result.stdout)

    def test_allowlist_allows_cpu_probe_with_direct_canonical_function_artifact(self) -> None:
        temp, root = self.make_temp()
        (root / "bad.cpp").write_text(
            self.canonical_function_docblock("0x401000") + "void f() { __asm { nop } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text("bad.cpp 0x401000 __asm cpu-probe test CPU probe\n", encoding="utf-8")

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_allowlist_allows_render_mmx_with_direct_canonical_function_artifact(self) -> None:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        source = root / "src" / "GameZRecoil" / "zRender" / "zrndr_draw.c"
        source.parent.mkdir(parents=True)
        source.write_text(
            self.canonical_function_docblock("0x48d510") + "void f() { __asm { emms } }\n",
            encoding="utf-8",
        )
        allowlist = root / "allowlist.txt"
        allowlist.write_text(
            "src/GameZRecoil/zRender/zrndr_draw.c 0x48d510 __asm "
            "render-mmx user-approved zRndr MMX row exception\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_allowlist_rejects_legacy_reimplements_marker(self) -> None:
        result = self.run_cpu_probe_source(
            "/**\n"
            " * Reimplements 0x401000: Test.\n"
            " * Raw assembly: keeps the VC5 retail byte shape after C++ variants failed.\n"
            " * Purpose: Exercises the documented minimal raw assembly exception.\n"
            " */\n"
            "void f() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:6", result.stdout)

    def test_allowlist_rejects_legacy_emits_marker(self) -> None:
        result = self.run_cpu_probe_source(
            "/**\n"
            " * Emits 0x401000: Test.\n"
            " * Raw assembly: keeps the VC5 retail byte shape after C++ variants failed.\n"
            " * Purpose: Exercises the documented minimal raw assembly exception.\n"
            " */\n"
            "void f() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:6", result.stdout)

    def test_allowlist_rejects_canonical_emits_function_artifact(self) -> None:
        result = self.run_cpu_probe_source(
            self.canonical_function_docblock("0x401000", relation="emits")
            + "void f() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:7", result.stdout)

    def test_allowlist_rejects_canonical_data_artifact(self) -> None:
        result = self.run_cpu_probe_source(
            self.canonical_function_docblock("0x401000", artifact_kind="data")
            + "void f() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:7", result.stdout)

    def test_allowlist_rejects_detached_canonical_directive(self) -> None:
        result = self.run_cpu_probe_source(
            self.canonical_function_docblock("0x401000")
            + "/* detached from the function by a stacked comment */\n"
            + "void f() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:8", result.stdout)

    def test_allowlist_rejects_ordinary_block_comment_directive(self) -> None:
        result = self.run_cpu_probe_source(
            self.canonical_function_docblock("0x401000", opening="/*")
            + "void f() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:7", result.stdout)

    def test_allowlist_rejects_ordinary_line_comment_directives(self) -> None:
        result = self.run_cpu_probe_source(
            "// @recoil-anchor recoil:anchor:test-function\n"
            "// @recoil-artifact defines .text recoil:function:0x401000: Test.\n"
            "// Raw assembly: keeps the VC5 retail byte shape after C++ variants failed.\n"
            "// Purpose: Exercises the documented minimal raw assembly exception.\n"
            "void f() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:5", result.stdout)

    def test_allowlist_rejects_ambiguous_defining_function_artifacts(self) -> None:
        result = self.run_cpu_probe_source(
            "/**\n"
            " * @recoil-anchor recoil:anchor:test-function\n"
            " * @recoil-artifact defines .text recoil:function:0x401000: Test.\n"
            " * @recoil-artifact defines .text recoil:function:0x401020: Ambiguous alias.\n"
            " * Raw assembly: keeps the VC5 retail byte shape after C++ variants failed.\n"
            " * Purpose: Exercises the documented minimal raw assembly exception.\n"
            " */\n"
            "void f() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:8", result.stdout)

    def test_allowlist_rejects_wrong_section_function_artifact(self) -> None:
        result = self.run_cpu_probe_source(
            self.canonical_function_docblock("0x401000", section=".rdata")
            + "void f() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:7", result.stdout)

    def test_allowlist_rejects_malformed_canonical_directive(self) -> None:
        result = self.run_cpu_probe_source(
            "/**\n"
            " * @recoil-anchor recoil:anchor:test-function\n"
            " * @recoil-artifact defines .text recoil:function:0x401000\n"
            " * Raw assembly: keeps the VC5 retail byte shape after C++ variants failed.\n"
            " * Purpose: Exercises the documented minimal raw assembly exception.\n"
            " */\n"
            "void f() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:7", result.stdout)

    def test_allowlist_rejects_assembly_outside_direct_function_construct(self) -> None:
        result = self.run_cpu_probe_source(
            self.canonical_function_docblock("0x401000")
            + "void traced() { int value = 1; }\n"
            + "void untraced() { __asm { nop } }\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("probe.cpp:8", result.stdout)

    def test_allowlist_rejects_ainet_vector_macro_without_enclosing_function_artifact(self) -> None:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        source = root / "src" / "Battlesport" / "ai_net.h"
        source.parent.mkdir(parents=True)
        source.write_text(
            "/**\n"
            " * Reimplements 0x401180: AINet::TickAiMode2PathFollow.\n"
            " * Raw assembly: keeps the VC5 x87 helper barrier after C++ variants failed to byte-match.\n"
            " * Purpose: Computes the AINet path XZ dot product helper.\n"
            " */\n"
            "#define AINET_PATH_DOT_XZ(out, steer, delta) \\\n"
            "    do {                                    \\\n"
            "        __asm mov ecx, v0                   \\\n"
            "    } while (0)\n"
            "void f();\n",
            encoding="utf-8",
        )
        allowlist = root / "allowlist.txt"
        allowlist.write_text(
            "src/Battlesport/ai_net.h 0x401180 __asm "
            "ainet-vector user-approved AINet path-vector helper exception\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("src/Battlesport/ai_net.h:8", result.stdout)

    def test_allowlist_rejects_ainet_vector_outside_helper_macro(self) -> None:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        source = root / "src" / "Battlesport" / "ai_net.h"
        source.parent.mkdir(parents=True)
        source.write_text(
            "void f() { __asm { nop } }\n"
            "/** Reimplements 0x401180: AINet::TickAiMode2PathFollow. */\n",
            encoding="utf-8",
        )
        allowlist = root / "allowlist.txt"
        allowlist.write_text(
            "src/Battlesport/ai_net.h 0x401180 __asm "
            "ainet-vector user-approved AINet path-vector helper exception\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("src/Battlesport/ai_net.h:1", result.stdout)

    def test_allowlist_rejects_ainet_vector_naked_rows(self) -> None:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        source = root / "src" / "Battlesport" / "ai_net.h"
        source.parent.mkdir(parents=True)
        source.write_text("void f();\n", encoding="utf-8")
        allowlist = root / "allowlist.txt"
        allowlist.write_text(
            "src/Battlesport/ai_net.h 0x401180 __declspec(naked) "
            "ainet-vector user-approved AINet path-vector helper exception\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("ainet-vector allowlist rows permit only __asm", result.stderr)
        self.assertIn("__declspec(naked)", result.stderr)

    def test_allowlist_rejects_ainet_vector_outside_scoped_address(self) -> None:
        temp, root = self.make_temp()
        (root / "bad.h").write_text(
            "#define AINET_PATH_DOT_XZ(out, steer, delta) \\\n"
            "    do { __asm { nop } } while (0)\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "bad.h 0x401180 __asm ainet-vector wrong AINet helper exception\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("ainet-vector is limited", result.stderr)

    def test_allowlist_allows_render_mmx_span_callbacks_for_scoped_zrndr_addresses(self) -> None:
        span_addresses = [
            "0x49ea80",
            "0x49ec20",
            "0x49e400",
            "0x49e560",
            "0x49cbb0",
            "0x49cea0",
            "0x49da80",
            "0x49ddb0",
        ]
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        source = root / "src" / "GameZRecoil" / "zRender" / "zrndr_draw.c"
        source.parent.mkdir(parents=True)
        source.write_text(
            "".join(
                self.canonical_function_docblock(
                    address,
                    anchor=f"render-span-{address[2:]}",
                )
                + f"void f_{address[2:]}() {{ __asm {{ emms }} }}\n"
                for address in span_addresses
            ),
            encoding="utf-8",
        )
        allowlist = root / "allowlist.txt"
        allowlist.write_text(
            "".join(
                "src/GameZRecoil/zRender/zrndr_draw.c "
                f"{address} __asm render-mmx user-approved zRndr span-MMX exception\n"
                for address in span_addresses
            ),
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_allowlist_rejects_render_mmx_naked_rows(self) -> None:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        source = root / "src" / "GameZRecoil" / "zRender" / "zrndr_draw.c"
        source.parent.mkdir(parents=True)
        source.write_text("void f() { int value = 1; }\n", encoding="utf-8")
        allowlist = root / "allowlist.txt"
        allowlist.write_text(
            "src/GameZRecoil/zRender/zrndr_draw.c 0x48d510 __declspec(naked) "
            "render-mmx user-approved zRndr MMX row exception\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("render-mmx allowlist rows permit only __asm", result.stderr)
        self.assertIn("__declspec(naked)", result.stderr)

    def test_allowlist_rejects_render_mmx_outside_scoped_addresses(self) -> None:
        temp, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "// Reimplements 0x48d4b0: Test\n"
            "void f() { __asm { emms } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "bad.cpp 0x48d4b0 __asm render-mmx wrong zRndr MMX row exception\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("render-mmx is limited", result.stderr)
        self.assertIn("src/GameZRecoil/zRender/zrndr_draw.c", result.stderr)

    def test_allowlist_rejects_retired_zrndr_source_path(self) -> None:
        temp, root = self.make_temp()
        (root / "clean.cpp").write_text("void f() {}\n", encoding="utf-8")
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "src/GameZRecoil/zRndr/zRndr.cpp 0x48d510 __asm "
            "render-mmx retired renderer path\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("render-mmx is limited", result.stderr)
        self.assertIn("src/GameZRecoil/zRender/zrndr_draw.c", result.stderr)

    def test_allowlist_allows_render_esp_pivot_inline_asm_for_scoped_zrndr_addresses(self) -> None:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        source = root / "src" / "GameZRecoil" / "zRender" / "zrndr_draw.c"
        source.parent.mkdir(parents=True)
        source.write_text(
            self.canonical_function_docblock("0x49b7e0")
            + "void f() { __asm { push ax } }\n",
            encoding="utf-8",
        )
        allowlist = root / "allowlist.txt"
        allowlist.write_text(
            "src/GameZRecoil/zRender/zrndr_draw.c 0x49b7e0 __asm "
            "render-esp-pivot user-approved zRndr ESP-pivot span exception\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_allowlist_rejects_render_esp_pivot_naked_rows(self) -> None:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        source = root / "src" / "GameZRecoil" / "zRender" / "zrndr_draw.c"
        source.parent.mkdir(parents=True)
        source.write_text("void f() { int value = 1; }\n", encoding="utf-8")
        allowlist = root / "allowlist.txt"
        allowlist.write_text(
            "src/GameZRecoil/zRender/zrndr_draw.c 0x49b7e0 __declspec(naked) "
            "render-esp-pivot user-approved zRndr ESP-pivot span exception\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("render-esp-pivot allowlist rows permit only __asm", result.stderr)
        self.assertIn("__declspec(naked)", result.stderr)

    def test_allowlist_rejects_render_esp_pivot_outside_scoped_addresses(self) -> None:
        temp, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "// Reimplements 0x49ea80: Test\n"
            "void f() { __asm { push ax } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "bad.cpp 0x49ea80 __asm render-esp-pivot wrong zRndr span exception\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("render-esp-pivot is limited", result.stderr)
        self.assertIn("src/GameZRecoil/zRender/zrndr_draw.c", result.stderr)

    def test_allowlist_rejects_untagged_raw_assembly_exception(self) -> None:
        temp, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "// Reimplements 0x401000: Test\n"
            "void f() { __asm { nop } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text("bad.cpp 0x401000 __asm user-approved exception\n", encoding="utf-8")

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("ainet-vector", result.stderr)
        self.assertIn("cpu-probe", result.stderr)
        self.assertIn("render-esp-pivot", result.stderr)
        self.assertIn("render-mmx", result.stderr)
        self.assertIn("source-faithful-inline-asm", result.stderr)

    def test_allowlist_allows_source_faithful_inline_asm_with_docblock(self) -> None:
        temp, root = self.make_temp()
        (root / "good.cpp").write_text(
            self.canonical_function_docblock("0x401000") + "void f() { __asm { nop } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "good.cpp 0x401000 __asm source-faithful-inline-asm "
            "VC5 C++ variants failed and byte evidence requires the minimal asm snippet\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_allowlist_accepts_source_faithful_inline_asm_with_chatgpt_pro_receipt(self) -> None:
        temp, root = self.make_temp()
        (root / "good.cpp").write_text(
            "/**\n"
            " * @recoil-anchor recoil:anchor:test-function\n"
            " * @recoil-artifact defines .text recoil:function:0x401020: Test.\n"
            " * Raw assembly: chatgpt-pro-line transcript confirmed VC5 C++ alternatives failed.\n"
            " * Evidence: ChatGPT Pro receipt/transcript evidence recoil-inline-asm-2026-07-06.\n"
            " * Purpose: Keeps the byte-sensitive retail flag/register handoff in one minimal snippet.\n"
            " */\n"
            "void f() { __asm { nop } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "good.cpp 0x401020 __asm source-faithful-inline-asm "
            "chatgpt-pro-line ChatGPT Pro receipt/transcript evidence confirms minimal inline asm\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_source_faithful_inline_asm_requires_docblock(self) -> None:
        temp, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "// Reimplements 0x401000: Test\n"
            "void f() { __asm { nop } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "bad.cpp 0x401000 __asm source-faithful-inline-asm "
            "VC5 C++ variants failed and byte evidence requires the minimal asm snippet\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("bad.cpp:2", result.stdout)

    def test_source_faithful_inline_asm_rejects_emit_rows(self) -> None:
        temp, root = self.make_temp()
        (root / "bad.cpp").write_text("void f() { _emit 0x90 }\n", encoding="utf-8")
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "bad.cpp 0x401000 _emit source-faithful-inline-asm wrong byte emission\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("source-faithful-inline-asm rows permit only __asm", result.stderr)

    def test_allowlist_paths_match_case_insensitively(self) -> None:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        source = root / "src" / "GameZRecoil" / "zMath" / "zmth.h"
        source.parent.mkdir(parents=True)
        source.write_text(
            self.canonical_function_docblock("0x402f60")
            + "inline void f() { __asm { nop } }\n",
            encoding="utf-8",
        )
        allowlist = root / "allowlist.txt"
        allowlist.write_text(
            "src/GameZRecoil/zmath/ZMTH.h 0x402f60 __asm "
            "source-faithful-inline-asm VC5 C++ variants failed\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_provider_allowlist_is_rejected(self) -> None:
        temp, root = self.make_temp()
        (root / "provider.cpp").write_text("void f() { __asm { nop } }\n", encoding="utf-8")
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text("provider.cpp provider __asm cpu-probe wrong tag\n", encoding="utf-8")

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("provider raw assembly shims are not allowed", result.stderr)

    def test_compiler_provider_exact_requires_and_allows_paired_naked_and_asm_tokens(self) -> None:
        temp, root = self.make_temp()
        (root / "provider.cpp").write_text(
            self.compiler_provider_docblock()
            + "__declspec(naked) void provider() { __asm { ret } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "provider.cpp 0x40a170 __declspec(naked) compiler-provider-exact "
            "VC5SP3 exact compiler-provider body requires no compiler prologue\n"
            "provider.cpp 0x40a170 __asm compiler-provider-exact "
            "VC5SP3 exact compiler-provider body requires minimal inline assembly\n",
            encoding="utf-8",
        )
        result = self.run_guard(root, "--allowlist", str(allowlist))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

        allowlist.write_text(
            "provider.cpp 0x40a170 __asm compiler-provider-exact "
            "VC5SP3 exact compiler-provider body requires minimal inline assembly\n",
            encoding="utf-8",
        )
        result = self.run_guard(root, "--allowlist", str(allowlist))
        self.assertEqual(result.returncode, 1)
        self.assertIn("paired __declspec(naked) and __asm", result.stderr)

    def test_compiler_provider_exact_rejects_detached_docblock(self) -> None:
        temp, root = self.make_temp()
        (root / "provider.cpp").write_text(
            self.compiler_provider_docblock()
            + "extern void intervening_declaration();\n"
            + "__declspec(naked) void provider() { __asm { ret } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "provider.cpp 0x40a170 __declspec(naked) compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n"
            "provider.cpp 0x40a170 __asm compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("__declspec(naked)", result.stdout)
        self.assertIn("__asm", result.stdout)

    def test_compiler_provider_exact_rejects_wrong_address_docblock(self) -> None:
        temp, root = self.make_temp()
        (root / "provider.cpp").write_text(
            self.compiler_provider_docblock("0x40a171")
            + "__declspec(naked) void provider() { __asm { ret } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "provider.cpp 0x40a170 __declspec(naked) compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n"
            "provider.cpp 0x40a170 __asm compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("__declspec(naked)", result.stdout)
        self.assertIn("__asm", result.stdout)

    def test_compiler_provider_exact_rejects_incomplete_docblock_evidence(self) -> None:
        temp, root = self.make_temp()
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "provider.cpp 0x40a170 __declspec(naked) compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n"
            "provider.cpp 0x40a170 __asm compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n",
            encoding="utf-8",
        )
        docblocks = (
            (
                "/**\n"
                " * Exact compiler provider for retail 0x40a170 after credible VC5SP3\n"
                " * C++ variants failed.\n"
                " */\n"
            ),
            (
                "/**\n"
                " * Purpose: exact compiler provider for retail 0x40a170.\n"
                " */\n"
            ),
        )
        for docblock in docblocks:
            with self.subTest(docblock=docblock):
                (root / "provider.cpp").write_text(
                    docblock
                    + "__declspec(naked) void provider() { __asm { ret } }\n",
                    encoding="utf-8",
                )

                result = self.run_guard(root, "--allowlist", str(allowlist))

                self.assertEqual(result.returncode, 1)

    def test_compiler_provider_exact_requires_both_tokens_in_one_function(self) -> None:
        temp, root = self.make_temp()
        (root / "provider.cpp").write_text(
            self.compiler_provider_docblock()
            + "__declspec(naked) void provider_shell() { return; }\n"
            + self.compiler_provider_docblock()
            + "void provider_body() { __asm { ret } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "provider.cpp 0x40a170 __declspec(naked) compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n"
            "provider.cpp 0x40a170 __asm compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("__declspec(naked)", result.stdout)
        self.assertIn("__asm", result.stdout)

    def test_compiler_provider_exact_never_allows_emit(self) -> None:
        temp, root = self.make_temp()
        (root / "provider.cpp").write_text(
            self.compiler_provider_docblock()
            + "__declspec(naked) void provider() { __asm { ret } _emit 0x90 }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "provider.cpp 0x40a170 __declspec(naked) compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n"
            "provider.cpp 0x40a170 __asm compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("_emit", result.stdout)

    def test_compiler_provider_exact_rejects_other_addresses_in_allowlist(self) -> None:
        temp, root = self.make_temp()
        (root / "provider.cpp").write_text(
            self.compiler_provider_docblock("0x40a171")
            + "__declspec(naked) void provider() { __asm { ret } }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "provider.cpp 0x40a171 __declspec(naked) compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n"
            "provider.cpp 0x40a171 __asm compiler-provider-exact "
            "VC5SP3 exact compiler-provider body\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 1)
        self.assertIn("limited to the exact provider address 0x40a170", result.stderr)

    def test_mechanical_inline_residual_rejects_naked_and_emit(self) -> None:
        temp, root = self.make_temp()
        (root / "bad.cpp").write_text(
            self.canonical_function_docblock("0x409570")
            + "__declspec(naked) void residual() { _emit 0x90 }\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "bad.cpp 0x409570 __declspec(naked) mechanical-inline-residual "
            "VC5SP3 exact mechanical residual\n",
            encoding="utf-8",
        )
        result = self.run_guard(root, "--allowlist", str(allowlist))
        self.assertEqual(result.returncode, 1)
        self.assertIn("mechanical-inline-residual rows permit only __asm", result.stderr)

    def test_coff_weak_alias_only_rejects_naked_allowlist_token(self) -> None:
        temp, root = self.make_temp()
        (root / "alias.asm").write_text(
            ".386\n.model flat\nEXTERN _target:PROC\n"
            "ALIAS <_alias> = <_target>\nEND\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "alias.asm coff-alias __declspec(naked) coff-weak-alias-only "
            "zero-section alias-only COFF input\n",
            encoding="utf-8",
        )
        result = self.run_guard(root, "--allowlist", str(allowlist))
        self.assertEqual(result.returncode, 1)
        self.assertIn("coff-weak-alias-only rows require", result.stderr)

    def test_coff_weak_alias_only_allows_only_structural_alias_source(self) -> None:
        temp, root = self.make_temp()
        (root / "alias.asm").write_text(
            ".386\n.model flat\nEXTERN _target:PROC\n"
            "ALIAS <_alias> = <_target>\nEND\n",
            encoding="utf-8",
        )
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text(
            "alias.asm coff-alias .asm coff-weak-alias-only "
            "zero-section alias-only COFF input\n",
            encoding="utf-8",
        )
        result = self.run_guard(root, "--allowlist", str(allowlist))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

        (root / "alias.asm").write_text(
            ".386\n.model flat\n.code\n_alias PROC\nret\n_alias ENDP\nEND\n",
            encoding="utf-8",
        )
        result = self.run_guard(root, "--allowlist", str(allowlist))
        self.assertEqual(result.returncode, 1)
        self.assertIn("coff-weak-alias-only", result.stdout)


if __name__ == "__main__":
    unittest.main()
