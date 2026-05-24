from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_source_file_map import collect_entries, main, normalize_original_source, render_markdown  # noqa: E402


class RecoilSourceFileMapTests(unittest.TestCase):
    def test_normalize_original_source_strips_known_drive_prefix(self) -> None:
        self.assertEqual(
            "GameZRecoil/zVideo/zvid_init.c",
            normalize_original_source(r"D:\Proj\GameZRecoil\zVideo\zvid_init.c"),
        )

    def test_collect_entries_from_reimplements_comments(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "src" / "module"
            source.mkdir(parents=True)
            (source / "sample.cpp").write_text(
                "\n".join(
                    [
                        "// Reimplements 0x401000: Sample::Known (D:\\Proj\\Battlesport\\Sample.cpp)",
                        "int x;",
                        "// Reimplements 0x401010: Sample::Unknown",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            entries = collect_entries(root / "src", repo_root=root)

        self.assertEqual(2, len(entries))
        self.assertEqual("Battlesport/Sample.cpp", entries[0].original_source)
        self.assertEqual("0x401000", entries[0].address)
        self.assertEqual("Sample::Known", entries[0].symbol)
        self.assertEqual("src/module/sample.cpp", entries[0].repo_file)
        self.assertEqual("unknown original source", entries[1].original_source)

    def test_render_markdown_groups_by_original_source(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "src"
            source.mkdir()
            (source / "sample.cpp").write_text(
                "// Reimplements 0x401000: Sample::Known (D:\\Proj\\Battlesport\\Sample.cpp)\n",
                encoding="utf-8",
            )
            entries = collect_entries(source, repo_root=root)

        rendered = render_markdown(entries)

        self.assertIn("## Battlesport/Sample.cpp", rendered)
        self.assertIn("`0x401000` `Sample::Known` -> `src/sample.cpp:1`", rendered)
        self.assertIn("excludes helpers fully inlined by the retail compiler", rendered)

    def test_render_markdown_warns_for_case_insensitive_collisions(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "src"
            source.mkdir()
            (source / "sample.cpp").write_text(
                "\n".join(
                    [
                        "// Reimplements 0x401000: Sample::Upper (D:\\Proj\\Battlesport\\HudUi.cpp)",
                        "// Reimplements 0x401010: Sample::Lower (D:\\Proj\\Battlesport\\hudui.cpp)",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
            entries = collect_entries(source, repo_root=root)

        rendered = render_markdown(entries)

        self.assertIn("## Case-insensitive source path collisions", rendered)
        self.assertIn("`Battlesport/HudUi.cpp`, `Battlesport/hudui.cpp`", rendered)

    def test_check_mode_fails_when_output_is_stale(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "src"
            source.mkdir()
            (source / "sample.cpp").write_text(
                "// Reimplements 0x401000: Sample::Known (D:\\Proj\\Battlesport\\Sample.cpp)\n",
                encoding="utf-8",
            )
            stale = root / "source_file_map.md"
            stale.write_text("stale\n", encoding="utf-8")

            result = main(["--root", str(source), "--check", str(stale)])

        self.assertEqual(1, result)


if __name__ == "__main__":
    unittest.main()
