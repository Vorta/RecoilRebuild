from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "recoil_no_reinterpret_cast.py"


class RecoilNoReinterpretCastTests(unittest.TestCase):
    def run_guard(self, root: Path, baseline: Path, *extra: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                str(SCRIPT),
                "--root",
                str(root),
                "--baseline",
                str(baseline),
                *extra,
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def make_temp(self) -> tuple[tempfile.TemporaryDirectory[str], Path, Path]:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name) / "src"
        root.mkdir()
        baseline = Path(temp.name) / "baseline.txt"
        return temp, root, baseline

    def test_clean_source_passes(self) -> None:
        _, root, baseline = self.make_temp()
        (root / "clean.cpp").write_text("void f() { int *p = (int *)0; }\n", encoding="utf-8")
        result = self.run_guard(root, baseline)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_comments_and_strings_are_ignored(self) -> None:
        _, root, baseline = self.make_temp()
        (root / "ignored.cpp").write_text(
            'const char *s = "reinterpret_cast<int *>(p)";\n'
            "// reinterpret_cast<int *>(p)\n"
            "/* reinterpret_cast<int *>(p) */\n",
            encoding="utf-8",
        )
        result = self.run_guard(root, baseline)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_unbaselined_cast_fails(self) -> None:
        _, root, baseline = self.make_temp()
        (root / "bad.cpp").write_text(
            "void f(void *p) { int *x = reinterpret_cast<int *>(p); }\n",
            encoding="utf-8",
        )
        result = self.run_guard(root, baseline)
        self.assertEqual(result.returncode, 1)
        self.assertIn("bad.cpp:1", result.stdout)

    def test_baselined_cast_passes(self) -> None:
        _, root, baseline = self.make_temp()
        (root / "old.cpp").write_text(
            "void f(void *p) { int *x = reinterpret_cast<int *>(p); }\n",
            encoding="utf-8",
        )
        write_result = self.run_guard(root, baseline, "--write-baseline")
        self.assertEqual(write_result.returncode, 0, write_result.stderr)

        check_result = self.run_guard(root, baseline)
        self.assertEqual(check_result.returncode, 0, check_result.stderr)

    def test_baseline_count_rejects_new_duplicate(self) -> None:
        _, root, baseline = self.make_temp()
        source = root / "old.cpp"
        source.write_text("void f(void *p) { int *x = reinterpret_cast<int *>(p); }\n", encoding="utf-8")
        write_result = self.run_guard(root, baseline, "--write-baseline")
        self.assertEqual(write_result.returncode, 0, write_result.stderr)

        source.write_text(
            "void f(void *p) { int *x = reinterpret_cast<int *>(p); }\n"
            "void f(void *p) { int *x = reinterpret_cast<int *>(p); }\n",
            encoding="utf-8",
        )
        check_result = self.run_guard(root, baseline)
        self.assertEqual(check_result.returncode, 1)
        self.assertIn("old.cpp:2", check_result.stdout)

    def test_removed_baseline_occurrence_does_not_fail(self) -> None:
        _, root, baseline = self.make_temp()
        source = root / "old.cpp"
        source.write_text("void f(void *p) { int *x = reinterpret_cast<int *>(p); }\n", encoding="utf-8")
        write_result = self.run_guard(root, baseline, "--write-baseline")
        self.assertEqual(write_result.returncode, 0, write_result.stderr)

        source.write_text("void f(void *p) { int *x = (int *)p; }\n", encoding="utf-8")
        check_result = self.run_guard(root, baseline)
        self.assertEqual(check_result.returncode, 0, check_result.stderr)


if __name__ == "__main__":
    unittest.main()
