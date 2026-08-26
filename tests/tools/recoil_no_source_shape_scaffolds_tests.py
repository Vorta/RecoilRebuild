from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "no_source_shape_scaffolds.py"


class RecoilNoSourceShapeScaffoldsTests(unittest.TestCase):
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

    def test_clean_recovered_owner_names_pass(self) -> None:
        _, root = self.make_temp()
        (root / "clean.cpp").write_text(
            "struct HudUiElement {\n"
            "    void Draw();\n"
            "};\n"
            "struct zFMV_Action {\n"
            "    virtual int Update(double timeSec);\n"
            "};\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 0, result.stdout)

    def test_virtual_dispatch_struct_fails(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "struct HudUiCommonDispatch {\n"
            "    virtual void Draw();\n"
            "};\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("virtual dispatch/source-shape scaffold", result.stdout)
        self.assertIn("bad.cpp:1", result.stdout)

    def test_virtual_named_source_view_fails(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "struct HudUiMessageBoxVirtual {\n"
            "    virtual void OnOk();\n"
            "};\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("virtual dispatch/source-shape scaffold", result.stdout)

    def test_vtable_factory_fails(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "HudTable MakeHudUiFTable(void) { return HudTable(); }\n"
            "HudTable MakeActionVtable(void) { return HudTable(); }\n"
            "HudTable MakeStateVtbl(void) { return HudTable(); }\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("vtable/ftable factory scaffold", result.stdout)

    def test_vtable_ftable_type_declarations_fail(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "struct HudUiPanel_FTable {\n"
            "    void (*draw)(void *self);\n"
            "};\n"
            "class RecoilStateVTable {\n"
            "    void (*enter)(void *self);\n"
            "};\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("authored vtable/ftable type scaffold", result.stdout)
        self.assertIn("HudUiPanel_FTable", result.stdout)
        self.assertIn("RecoilStateVTable", result.stdout)

    def test_vtable_ftable_globals_fail(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "struct HudUiPanel_FTable;\n"
            "extern const HudUiPanel_FTable g_HudUiPanel_FTable;\n"
            "static RecoilStateVtbl *g_RecoilStateVtbl;\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("authored vtable/ftable object/global scaffold", result.stdout)

    def test_vtable_ftable_typedefs_fail(self) -> None:
        _, root = self.make_temp()
        (root / "bad.h").write_text(
            "typedef void (*HudUiPanelFTable)(void *self);\n"
            "typedef struct RecoilStateVtbl RecoilStateVtbl;\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("authored vtable/ftable typedef scaffold", result.stdout)

    def test_raw_slot_arrays_fail(self) -> None:
        _, root = self.make_temp()
        (root / "bad.h").write_text(
            "struct RawDispatch {\n"
            "    void *slots[8];\n"
            "    void *primarySlots[2];\n"
            "};\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("raw slot table scaffold", result.stdout)

    def test_scaffold_comment_fails(self) -> None:
        _, root = self.make_temp()
        (root / "bad.h").write_text(
            "/* Temporary ABI scaffold for a virtual table packet. */\n"
            "struct Packet { unsigned int slots[4]; };\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("scaffold marker in production source", result.stdout)

    def test_comments_and_strings_are_ignored_for_code_patterns(self) -> None:
        _, root = self.make_temp()
        (root / "clean.cpp").write_text(
            'const char *name = "struct FakeDispatch { virtual void Draw(); };";\n'
            "// struct OtherDispatch { virtual void Draw(); };\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 0, result.stdout)

    def test_summary_reports_occurrences(self) -> None:
        _, root = self.make_temp()
        (root / "bad.cpp").write_text(
            "struct HudUiCommonDispatch { virtual void Draw(); };\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--summary", "--top", "1")

        self.assertEqual(result.returncode, 1)
        self.assertIn("source-shape scaffold production-source summary:", result.stdout)
        self.assertIn("current occurrences: 1", result.stdout)

    def test_file_root_is_scanned(self) -> None:
        _, root = self.make_temp()
        path = root / "bad.cpp"
        path.write_text(
            "struct HudUiPanel_FTable {\n"
            "    unsigned int slots[4];\n"
            "};\n",
            encoding="utf-8",
        )

        result = self.run_guard(path)

        self.assertEqual(result.returncode, 1)
        self.assertIn("authored vtable/ftable type scaffold", result.stdout)
        self.assertIn("raw slot table scaffold", result.stdout)

    def test_repeated_path_arguments_are_scanned(self) -> None:
        _, root = self.make_temp()
        clean = root / "clean.cpp"
        bad = root / "bad.cpp"
        clean.write_text("struct HudUiElement { void Draw(); };\n", encoding="utf-8")
        bad.write_text(
            "HudTable MakeHudUiFTable(void) { return HudTable(); }\n",
            encoding="utf-8",
        )

        result = self.run_guard(clean, "--path", str(bad))

        self.assertEqual(result.returncode, 1)
        self.assertIn("vtable/ftable factory scaffold", result.stdout)

    def test_legacy_paths_and_max_aliases_are_accepted(self) -> None:
        _, root = self.make_temp()
        clean = root / "clean.cpp"
        bad_a = root / "bad_a.cpp"
        bad_b = root / "bad_b.cpp"
        clean.write_text("struct HudUiElement { void Draw(); };\n", encoding="utf-8")
        bad_a.write_text(
            "HudTable MakeHudUiFTable(void) { return HudTable(); }\n",
            encoding="utf-8",
        )
        bad_b.write_text(
            "struct HudUiPanel_FTable { unsigned int slots[4]; };\n",
            encoding="utf-8",
        )

        result = self.run_guard(
            clean,
            "--paths",
            str(bad_a),
            str(bad_b),
            "--summary",
            "--max",
            "1",
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("source-shape scaffold production-source summary:", result.stdout)
        self.assertIn("top labels (limit 1)", result.stdout)
        self.assertIn("vtable/ftable factory scaffold", result.stdout)
        self.assertIn("authored vtable/ftable type scaffold", result.stdout)

    def test_repeated_root_arguments_are_scanned(self) -> None:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        base = Path(temp.name)
        clean_root = base / "clean"
        bad_root = base / "bad"
        clean_root.mkdir()
        bad_root.mkdir()
        (clean_root / "clean.cpp").write_text(
            "struct HudUiElement { void Draw(); };\n",
            encoding="utf-8",
        )
        (bad_root / "bad.cpp").write_text(
            "HudTable MakeHudUiFTable(void) { return HudTable(); }\n",
            encoding="utf-8",
        )

        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT),
                "--root",
                str(clean_root),
                "--root",
                str(bad_root),
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("vtable/ftable factory scaffold", result.stdout)


if __name__ == "__main__":
    unittest.main()
