from __future__ import annotations

from collections import Counter
from contextlib import redirect_stderr, redirect_stdout
import io
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.no_source_goto import (  # noqa: E402
    DEFAULT_BASELINE,
    GotoGuardError,
    load_baseline,
    main,
    scan_source_root,
)


EXPECTED_COUNTS = {
    "src/GameZRecoil/zVideo/zvid_main.c": 3,
    "src/GameZRecoil/zVideo/zvid_dd.c": 10,
    "src/GameZRecoil/zInput/zin_joystick.cpp": 43,
    "src/GameZRecoil/zSound/zsnd_error.cpp": 2,
    "src/GameZRecoil/zClass/Animate.c": 2,
    "src/GameZRecoil/zClass/Seq.c": 11,
    "src/GameZRecoil/zClass/cls_zbd.c": 4,
    "src/GameZRecoil/zEffect/zeff_anim_run.c": 4,
    "src/Battlesport/hud.cpp": 5,
    "src/Battlesport/WOL.cpp": 3,
    "src/GameZRecoil/zGeometry/zgeo_convexify.cpp": 1,
    "src/GameZRecoil/zGeometry/zgeo_weiler.cpp": 1,
    "src/GameZRecoil/zModel/gmod_matl.c": 7,
    "src/GameZRecoil/zReader/zreader.cpp": 2,
}


class RecoilNoSourceGotoTests(unittest.TestCase):
    def make_root(self) -> tuple[tempfile.TemporaryDirectory[str], Path]:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name) / "src"
        root.mkdir()
        return temporary, root

    def run_main(self, *arguments: str) -> tuple[int, str, str]:
        stdout = io.StringIO()
        stderr = io.StringIO()
        with redirect_stdout(stdout), redirect_stderr(stderr):
            result = main(list(arguments))
        return result, stdout.getvalue(), stderr.getvalue()

    def copy_production_inventory(self) -> tuple[Path, Path]:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        config = Path(temporary.name) / "config"
        config.mkdir()
        index = config / DEFAULT_BASELINE.name
        shutil.copy2(DEFAULT_BASELINE, index)
        shards = config / "no_source_goto_baseline"
        shutil.copytree(DEFAULT_BASELINE.parent / "no_source_goto_baseline", shards)
        return index, shards

    def test_frozen_repo_inventory_is_exact_reviewed_schema_v2(self) -> None:
        source_root, inventory = load_baseline(DEFAULT_BASELINE)
        current = scan_source_root(REPO_ROOT / "src", source_root=source_root)

        self.assertEqual(98, len(inventory))
        self.assertTrue(all(row.state == "retired" for row in inventory))
        self.assertEqual(tuple(range(1, 99)), tuple(row.row_id for row in inventory))
        self.assertEqual((), current)
        self.assertEqual(EXPECTED_COUNTS, dict(Counter(row.path for row in inventory)))
        index = json.loads(DEFAULT_BASELINE.read_text(encoding="utf-8"))
        self.assertEqual(2, index["schema_version"])
        self.assertEqual(14, len(index["shards"]))

    def test_index_schema_and_reviewed_total_drift_fail_closed(self) -> None:
        index, _ = self.copy_production_inventory()
        original = json.loads(index.read_text(encoding="utf-8"))
        for field, value in (("schema_version", 1), ("reviewed_total", 97)):
            with self.subTest(field=field):
                changed = dict(original)
                changed[field] = value
                index.write_text(json.dumps(changed), encoding="utf-8")
                with self.assertRaisesRegex(GotoGuardError, "exact schema version 2"):
                    load_baseline(index)

    def test_shard_rows_are_structural_and_retired_only(self) -> None:
        index, shards = self.copy_production_inventory()
        index_document = json.loads(index.read_text(encoding="utf-8"))
        shard_path = shards / Path(index_document["shards"][0]["config"]).name
        original = json.loads(shard_path.read_text(encoding="utf-8"))

        changed = json.loads(json.dumps(original))
        changed["entries"][0]["state"] = "active"
        shard_path.write_text(json.dumps(changed), encoding="utf-8")
        with self.assertRaisesRegex(GotoGuardError, "retired rows only"):
            load_baseline(index)

        changed = json.loads(json.dumps(original))
        changed["entries"][0]["row_id"] += 1
        shard_path.write_text(json.dumps(changed), encoding="utf-8")
        with self.assertRaisesRegex(GotoGuardError, "row order/path is not exact"):
            load_baseline(index)

        changed = json.loads(json.dumps(original))
        changed["entries"][0]["unexpected"] = True
        shard_path.write_text(json.dumps(changed), encoding="utf-8")
        with self.assertRaisesRegex(GotoGuardError, "row is malformed"):
            load_baseline(index)

    def test_loading_inventory_does_not_change_index_or_shard_bytes(self) -> None:
        index, shards = self.copy_production_inventory()
        before = {
            path.relative_to(index.parent).as_posix(): path.read_bytes()
            for path in (index, *sorted(shards.glob("*.json")))
        }
        load_baseline(index)
        after = {
            path.relative_to(index.parent).as_posix(): path.read_bytes()
            for path in (index, *sorted(shards.glob("*.json")))
        }
        self.assertEqual(before, after)

    def test_any_source_occurrence_is_rejected(self) -> None:
        _, root = self.make_root()
        (root / "legacy.cpp").write_text(
            "void f() { goto cleanup; cleanup: return; }\n", encoding="utf-8"
        )
        returncode, stdout, _ = self.run_main(
            "--root", str(root), "--baseline", str(DEFAULT_BASELINE), "--json"
        )
        payload = json.loads(stdout)
        self.assertEqual(1, returncode)
        self.assertEqual("strict-zero", payload["mode"])
        self.assertEqual(1, payload["remaining_debt"])
        self.assertEqual(98, payload["removed_debt"])
        self.assertEqual("cleanup", payload["violations"][0]["target"])

    def test_zero_source_occurrences_pass(self) -> None:
        _, root = self.make_root()
        (root / "clean.cpp").write_text("void f() { return; }\n", encoding="utf-8")
        returncode, stdout, stderr = self.run_main(
            "--root", str(root), "--baseline", str(DEFAULT_BASELINE), "--summary"
        )
        self.assertEqual(0, returncode, stderr)
        self.assertIn("mode=strict-zero", stdout)
        self.assertIn("remaining_debt=0", stdout)
        self.assertIn("removed_debt=98", stdout)

    def test_comments_strings_characters_and_inline_assembly_are_ignored(self) -> None:
        _, root = self.make_root()
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
        rows = scan_source_root(root, source_root="src")
        self.assertEqual((), rows)

    def test_registered_recoil_route_runs_the_frozen_inventory(self) -> None:
        result = subprocess.run(
            [
                sys.executable,
                str(REPO_ROOT / "tools" / "recoil.py"),
                "guard",
                "source-goto",
                "--root",
                "src",
                "--json",
                "--strict-zero",
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )
        self.assertEqual(0, result.returncode, result.stderr)
        payload = json.loads(result.stdout)
        self.assertEqual("pass", payload["status"])
        self.assertEqual("strict-zero", payload["mode"])
        self.assertEqual(2, payload["inventory_schema_version"])
        self.assertEqual(98, payload["reviewed_baseline"])
        self.assertEqual(0, payload["remaining_debt"])
        self.assertEqual(98, payload["removed_debt"])
        self.assertEqual({}, payload["file_counts"])


if __name__ == "__main__":
    unittest.main()
