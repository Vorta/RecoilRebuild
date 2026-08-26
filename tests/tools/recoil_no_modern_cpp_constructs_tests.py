from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "no_modern_cpp_constructs.py"


class RecoilNoModernCppConstructsTests(unittest.TestCase):
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

    def test_clean_vc5_style_source_passes(self) -> None:
        _, root = self.make_temp()
        (root / "clean.cpp").write_text(
            "void f(void *p) { int *x = (int *)p; RECOIL_STATIC_ASSERT(sizeof(int) == 4); }\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 0, result.stdout)

    def test_comments_and_strings_are_ignored(self) -> None:
        _, root = self.make_temp()
        (root / "ignored.cpp").write_text(
            'const char *s = "static_cast<int *>(p) auto nullptr";\n'
            "// static_cast<int *>(p)\n"
            "/* auto value = nullptr; */\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 0, result.stdout)

    def test_named_casts_fail(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "void f(void *p) { int *x = static_cast<int *>(p); }\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("named C++ cast", result.stdout)
        self.assertIn("bad.cpp:1", result.stdout)

    def test_post_vc5_constructs_fail(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "#include <filesystem>\n"
            "void f() { auto x = []() { return nullptr; }; }\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("modern standard header", result.stdout)
        self.assertIn("auto", result.stdout)
        self.assertIn("lambda", result.stdout)
        self.assertIn("nullptr", result.stdout)

    def test_reconstruction_callconv_wrappers_fail(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "RECOIL_NOINLINE void RECOIL_FASTCALL f();\n"
            "void RECOIL_THISCALL C::g();\n"
            "void __thiscall h();\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("reconstruction noinline marker", result.stdout)
        self.assertIn("reconstruction call-convention wrapper", result.stdout)
        self.assertIn("explicit thiscall", result.stdout)

    def test_reconstruction_inline_marker_fails(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "RECOIL_FORCE" "INLINE int Helper() { return 1; }\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("reconstruction inline marker", result.stdout)

    def test_plain_inline_spelling_passes(self) -> None:
        _, root = self.make_temp()
        (root / "clean.cpp").write_text(
            "static inline int Helper() { return 1; }\n"
            "inline int OtherHelper() { return Helper(); }\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 0, result.stdout)

    def test_summary_reports_current_and_top_files(self) -> None:
        _, root = self.make_temp()
        directory = root / "GameZRecoil" / "zHud"
        directory.mkdir(parents=True)
        source = directory / "hud.cpp"
        source.write_text("void f() { auto x = 1; }\n", encoding="utf-8")

        result = self.run_guard(root, "--summary", "--top", "1")

        self.assertEqual(result.returncode, 1)
        self.assertIn("modern C++ production-source construct summary:", result.stdout)
        self.assertIn("current occurrences: 1", result.stdout)
        self.assertIn("hud.cpp", result.stdout)


if __name__ == "__main__":
    unittest.main()
