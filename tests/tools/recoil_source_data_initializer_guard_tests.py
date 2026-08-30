from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "source_data_initializer_guard.py"
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.lib.progress import empty_progress_document  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402


def initializer(*, state: str = "✅") -> dict[str, object]:
    return {
        "path": "src/sample.cpp",
        "name": "g_Table",
        "data_reimplemented": state,
        "expected": "concrete-initializer-list",
        "evidence": "unit-test retail data evidence",
    }


class RecoilSourceDataInitializerGuardTests(unittest.TestCase):
    def run_guard(
        self,
        root: Path,
        manifest: Path | None = None,
        *,
        cwd: Path | None = None,
        use_path_alias: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        scan_arg = "--path" if use_path_alias else "--root"
        command = [sys.executable, str(SCRIPT), scan_arg, str(root)]
        if manifest is not None:
            command.extend(["--progress", str(manifest)])
        return subprocess.run(
            command,
            cwd=cwd or root,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    @staticmethod
    def write_manifest(root: Path, *, state: str = "✅") -> Path:
        path = root / "manifest.json"
        path.write_text(json.dumps({"globals": [initializer(state=state)]}), encoding="utf-8")
        return path

    @staticmethod
    def write_progress(root: Path) -> Path:
        agent = root / ".agent"
        agent.mkdir()
        record = initializer()
        payload = empty_progress_document()
        payload["symbols"]["recoil:data:0x401000"] = {
            "kind": "data",
            "binary": "recoil",
            "address": "0x401000",
            "legacy_initializer": record,
            "evidence_ids": ["recoil:evidence:r0:000001"],
        }
        payload["evidence"]["recoil:evidence:r0:000001"] = {
            "kind": "legacy-data-initializer",
            "provenance": {"record": record},
        }
        path = agent / "RECONSTRUCTION_PROGRESS.sqlite3"
        ProgressSQLiteStore.create_from_mapping(
            path,
            payload,
            cutover_pair_id="source-data-test",
        )
        return path

    @staticmethod
    def write_source(root: Path, source: str) -> Path:
        source_dir = root / "src"
        source_dir.mkdir(exist_ok=True)
        path = source_dir / "sample.cpp"
        path.write_text(source, encoding="utf-8")
        return path

    def test_standalone_manifest_rejects_zero_placeholder(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.write_source(root, "Record g_Table[3] = {0};\n")
            result = self.run_guard(root, self.write_manifest(root))
        self.assertNotEqual(0, result.returncode)
        self.assertIn("zero placeholder", result.stdout)

    def test_standalone_manifest_accepts_explicit_initializer(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.write_source(root, 'Record g_Table[1] = {{"first", 1}};\n')
            result = self.run_guard(root, self.write_manifest(root))
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)

    def test_missing_definition_and_failed_state_are_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.write_source(root, "Record *g_TablePtr = 0;\n")
            missing = self.run_guard(root, self.write_manifest(root))
            failed = self.run_guard(root, self.write_manifest(root, state="❌"))
        self.assertIn("definition was not found", missing.stdout)
        self.assertIn("Data reimplemented ❌", failed.stdout)

    def test_default_sqlite_progress_is_discovered_from_source_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = self.write_source(root, 'Record g_Table[1] = {{"first", 1}};\n')
            self.write_progress(root)
            result = self.run_guard(source.parent, cwd=root)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)

    def test_sqlite_projection_enforces_initializer_shape(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.write_source(root, "Record g_Table[1] = {0};\n")
            progress = self.write_progress(root)
            result = self.run_guard(root, progress)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("zero placeholder", result.stdout)

    def test_path_alias_accepts_a_single_source_file(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = self.write_source(root, 'Record g_Table[1] = {{"first", 1}};\n')
            progress = self.write_progress(root)
            result = self.run_guard(source, progress, cwd=root, use_path_alias=True)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)

    def test_json_cannot_impersonate_unified_progress(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            manifest = root / "retired-progress.json"
            manifest.write_text(
                json.dumps({"schema_version": 6, "globals": []}),
                encoding="utf-8",
            )
            result = self.run_guard(root, manifest)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("unified progress input must be SQLite", result.stderr)


if __name__ == "__main__":
    unittest.main()
