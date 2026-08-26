from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "no_raw_image_addresses.py"


class RecoilNoRawImageAddressesTests(unittest.TestCase):
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
        (root / "clean.cpp").write_text("int value = 0x00500000;\n", encoding="utf-8")

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_comments_and_strings_are_ignored(self) -> None:
        _, root = self.make_temp()
        (root / "ignored.cpp").write_text(
            'const char *s = "0x00401000";\n'
            "// 0x00401000\n"
            "/* 0x00401000 */\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_original_image_literal_fails(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text("int value = 0x00401000;\n", encoding="utf-8")

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("bad.cpp:1", result.stdout)
        self.assertIn("0x00401000", result.stdout)

    def test_allowlist_allows_literal(self) -> None:
        temp, root = self.make_temp()
        (root / "bad.cpp").write_text("int value = 0x00401000;\n", encoding="utf-8")
        allowlist = Path(temp.name) / "allowlist.txt"
        allowlist.write_text("bad.cpp 0x00401000\n", encoding="utf-8")

        result = self.run_guard(root, "--allowlist", str(allowlist))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
