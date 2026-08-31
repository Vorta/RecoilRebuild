from __future__ import annotations

from contextlib import redirect_stderr, redirect_stdout
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.no_source_goto import main, scan_source_root  # noqa: E402


class RecoilNoSourceGotoTests(unittest.TestCase):
    def make_root(self) -> Path:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name) / "src"
        root.mkdir()
        return root

    def run_main(self, *arguments: str) -> tuple[int, str, str]:
        stdout = io.StringIO()
        stderr = io.StringIO()
        with redirect_stdout(stdout), redirect_stderr(stderr):
            result = main(list(arguments))
        return result, stdout.getvalue(), stderr.getvalue()

    def test_current_production_source_has_no_goto(self) -> None:
        self.assertEqual((), scan_source_root(REPO_ROOT / "src"))

    def test_any_source_occurrence_is_rejected(self) -> None:
        root = self.make_root()
        (root / "legacy.cpp").write_text(
            "void f() { goto cleanup; cleanup: return; }\n", encoding="utf-8"
        )
        returncode, stdout, _ = self.run_main("--root", str(root), "--json")
        payload = json.loads(stdout)
        self.assertEqual(1, returncode)
        self.assertEqual("zero-tolerance", payload["mode"])
        self.assertEqual(1, payload["violation_count"])
        self.assertEqual("cleanup", payload["violations"][0]["target"])

    def test_zero_source_occurrences_pass(self) -> None:
        root = self.make_root()
        (root / "clean.cpp").write_text("void f() { return; }\n", encoding="utf-8")
        returncode, stdout, stderr = self.run_main("--root", str(root), "--summary")
        self.assertEqual(0, returncode, stderr)
        self.assertIn("violations=0", stdout)

    def test_comments_strings_characters_and_inline_assembly_are_ignored(self) -> None:
        root = self.make_root()
        (root / "ignored.cpp").write_text(
            "const char *s = \"goto stringTarget;\";\n"
            "char c = 'g'; // goto lineComment;\n"
            "/* goto blockComment; */\n"
            "void f() {\n"
            "    __asm { goto asmText }\n"
            "    _asm goto oneLineAsmText\n"
            "}\n",
            encoding="utf-8",
        )
        self.assertEqual((), scan_source_root(root))

    def test_registered_route_is_direct_zero_tolerance(self) -> None:
        result = subprocess.run(
            [
                sys.executable,
                str(REPO_ROOT / "tools" / "recoil.py"),
                "guard",
                "source-goto",
                "--root",
                "src",
                "--json",
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )
        self.assertEqual(0, result.returncode, result.stderr)
        payload = json.loads(result.stdout)
        self.assertEqual("pass", payload["status"])
        self.assertEqual("zero-tolerance", payload["mode"])
        self.assertEqual(0, payload["violation_count"])


if __name__ == "__main__":
    unittest.main()
