from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_tooling import (  # noqa: E402
    display_path,
    iter_source_files,
    quote_cmd_arg,
    strip_comments_and_strings,
    write_text_if_changed,
)


class RecoilToolingTests(unittest.TestCase):
    def test_strip_comments_and_strings_preserves_code_positions(self) -> None:
        text = 'int x = 1; // 0x401000\nconst char *s = "reinterpret_cast<int *>(p)";\n'

        stripped = strip_comments_and_strings(text)

        self.assertIn("int x = 1;", stripped)
        self.assertNotIn("0x401000", stripped)
        self.assertNotIn("reinterpret_cast", stripped)
        self.assertEqual(text.count("\n"), stripped.count("\n"))

    def test_iter_source_files_uses_case_insensitive_suffixes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "a.CPP").write_text("", encoding="utf-8")
            (root / "b.txt").write_text("", encoding="utf-8")

            files = iter_source_files(root)

        self.assertEqual(["a.CPP"], [path.name for path in files])

    def test_display_path_falls_back_to_scan_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            scan_root = Path(tmp) / "src"
            path = scan_root / "sample.cpp"
            scan_root.mkdir()
            path.write_text("", encoding="utf-8")

            shown = display_path(path, REPO_ROOT, fallback_root=scan_root)

        self.assertEqual("sample.cpp", shown)

    def test_quote_cmd_arg_rejects_embedded_quotes(self) -> None:
        with self.assertRaises(ValueError):
            quote_cmd_arg('bad"arg')

    def test_write_text_if_changed_reports_noop(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "out.txt"
            self.assertTrue(write_text_if_changed(path, "sample\n"))
            self.assertFalse(write_text_if_changed(path, "sample\n"))


if __name__ == "__main__":
    unittest.main()
