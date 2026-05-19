from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "recoil_no_reinterpret_cast.py"


class RecoilNoReinterpretCastTests(unittest.TestCase):
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

    def test_clean_source_passes(self) -> None:
        _, root = self.make_temp()
        (root / "clean.cpp").write_text("void f() { int *p = (int *)0; }\n", encoding="utf-8")
        result = self.run_guard(root)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_comments_and_strings_are_ignored(self) -> None:
        _, root = self.make_temp()
        (root / "ignored.cpp").write_text(
            'const char *s = "reinterpret_cast<int *>(p)";\n'
            "// reinterpret_cast<int *>(p)\n"
            "/* reinterpret_cast<int *>(p) */\n",
            encoding="utf-8",
        )
        result = self.run_guard(root)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_cast_fails(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "void f(void *p) { int *x = reinterpret_cast<int *>(p); }\n",
            encoding="utf-8",
        )
        result = self.run_guard(root)
        self.assertEqual(result.returncode, 1)
        self.assertIn("bad.cpp:1", result.stdout)

    def test_summary_reports_current_and_top_files(self) -> None:
        _, root = self.make_temp()
        directory = root / "GameZRecoil" / "zHud"
        directory.mkdir(parents=True)
        source = directory / "hud.cpp"
        source.write_text("void f(void *p) { int *x = reinterpret_cast<int *>(p); }\n", encoding="utf-8")

        result = self.run_guard(root, "--summary", "--top", "1")

        self.assertEqual(result.returncode, 1)
        self.assertIn("reinterpret_cast summary:", result.stdout)
        self.assertIn("current occurrences: 1", result.stdout)
        self.assertIn("hud.cpp", result.stdout)


if __name__ == "__main__":
    unittest.main()
