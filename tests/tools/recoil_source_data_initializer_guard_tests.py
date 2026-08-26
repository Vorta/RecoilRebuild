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


class RecoilSourceDataInitializerGuardTests(unittest.TestCase):
    def run_guard(
        self,
        root: Path,
        manifest: Path | None = None,
        cwd: Path | None = None,
        extra_args: tuple[str, ...] = (),
        use_path_alias: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        scan_arg = "--path" if use_path_alias else "--root"
        command = [sys.executable, str(SCRIPT), scan_arg, str(root)]
        if manifest is not None:
            command.extend(["--progress", str(manifest)])
        command.extend(extra_args)
        return subprocess.run(
            command,
            cwd=cwd or root,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def write_manifest(self, root: Path) -> Path:
        manifest = root / "manifest.json"
        manifest.write_text(
            json.dumps(
                {
                    "globals": [
                        {
                            "path": "src/sample.cpp",
                            "name": "g_Table",
                            "data_reimplemented": "✅",
                            "group": "test.group",
                            "original_address": "0x401000",
                            "original_section": ".rdata",
                            "original_type": "Record[3]",
                            "expected": "concrete-initializer-list",
                            "source_shape": "typed initializer list",
                            "touched_by": ["0x401000"],
                            "evidence": "test evidence",
                        }
                    ]
                }
            ),
            encoding="utf-8",
        )
        return manifest

    def test_rejects_zero_placeholder_concrete_global(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "sample.cpp").write_text("Record g_Table[3] = {0};\n", encoding="utf-8")
            result = self.run_guard(root, self.write_manifest(root))

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("g_Table", result.stdout)
        self.assertIn("zero placeholder", result.stdout)

    def test_accepts_explicit_initializer_list(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "sample.cpp").write_text(
                "Record g_Table[3] = {\n"
                "    {\"first\", 1},\n"
                "    {\"second\", 2},\n"
                "    {\"third\", 3},\n"
                "};\n",
                encoding="utf-8",
            )
            result = self.run_guard(root, self.write_manifest(root))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_rejects_missing_definition(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "sample.cpp").write_text("Record *g_TablePtr = 0;\n", encoding="utf-8")
            result = self.run_guard(root, self.write_manifest(root))

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("definition was not found", result.stdout)

    def test_rejects_data_reimplemented_failed_ledger_entry(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "sample.cpp").write_text("Record g_Table[3] = {0};\n", encoding="utf-8")
            manifest = root / "manifest.json"
            manifest.write_text(
                json.dumps(
                    {
                        "globals": [
                            {
                                "path": "src/sample.cpp",
                                "name": "g_Table",
                                "data_reimplemented": "❌",
                                "expected": "concrete-initializer-list",
                                "evidence": "known original data exists",
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, manifest)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Data reimplemented ❌", result.stdout)

    def test_explicit_legacy_manifest_uses_workspace_root_when_scan_root_is_src(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            agent_dir = root / ".agent"
            source_dir.mkdir()
            agent_dir.mkdir()
            (source_dir / "sample.cpp").write_text(
                "Record g_Table[3] = {\n"
                "    {\"first\", 1},\n"
                "    {\"second\", 2},\n"
                "    {\"third\", 3},\n"
                "};\n",
                encoding="utf-8",
            )
            progress = agent_dir / "RECONSTRUCTION_PROGRESS.json"
            progress.write_text(
                json.dumps(
                    {
                        "globals": [
                            {
                                "path": "src/sample.cpp",
                                "name": "g_Table",
                                "data_reimplemented": "✅",
                                "expected": "concrete-initializer-list",
                                "evidence": "test evidence",
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(source_dir, progress, cwd=root)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_summary_flag_is_accepted_for_handoff_compatibility(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "sample.cpp").write_text(
                "Record g_Table[3] = {\n"
                "    {\"first\", 1},\n"
                "    {\"second\", 2},\n"
                "    {\"third\", 3},\n"
                "};\n",
                encoding="utf-8",
            )
            result = self.run_guard(root, self.write_manifest(root), extra_args=("--summary",))

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_unified_progress_schema_two_legacy_initializer_projection_is_supported(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "sample.cpp").write_text(
                'Record g_Table[1] = {{"first", 1}};\n', encoding="utf-8"
            )
            progress = root / "progress.json"
            progress.write_text(
                json.dumps(
                    {
                        "schema_version": 2,
                        "evidence": {
                            "legacy:evidence:000001": {
                                "kind": "legacy-data-initializer",
                                "provenance": {
                                    "record": {
                                        "path": "src/sample.cpp",
                                        "name": "g_Table",
                                        "data_reimplemented": "✅",
                                        "expected": "concrete-initializer-list",
                                        "evidence": "test evidence",
                                    }
                                },
                            }
                        },
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, progress)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_unified_progress_schema_five_initializer_projection_is_enforced(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "sample.cpp").write_text(
                "Record g_Table[1] = {0};\n",
                encoding="utf-8",
            )
            initializer = {
                "path": "src/sample.cpp",
                "name": "g_Table",
                "data_reimplemented": "✅",
                "expected": "concrete-initializer-list",
                "evidence": "schema-5 test evidence",
            }
            progress = root / "progress.json"
            progress.write_text(
                json.dumps(
                    {
                        "schema_version": 5,
                        "symbols": {
                            "recoil:data:0x401000": {
                                "kind": "data",
                                "binary": "recoil",
                                "address": "0x401000",
                                "legacy_initializer": initializer,
                                "evidence_ids": ["recoil:evidence:r1:000001"],
                            }
                        },
                        "evidence": {
                            "recoil:evidence:r1:000001": {
                                "kind": "legacy-data-initializer",
                                "provenance": {
                                    "record": initializer,
                                },
                            }
                        },
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, progress)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("g_Table", result.stdout)
        self.assertIn("zero placeholder", result.stdout)

    def test_unified_progress_schema_five_requires_symbols_collection(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            progress = root / "progress.json"
            progress.write_text(
                json.dumps(
                    {
                        "schema_version": 5,
                        "evidence": {},
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, progress)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("progress collection 'symbols' must be an object", result.stderr)

    def test_unified_progress_schema_five_requires_evidence_collection(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            progress = root / "progress.json"
            progress.write_text(
                json.dumps(
                    {
                        "schema_version": 5,
                        "symbols": {},
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, progress)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("progress collection 'evidence' must be an object", result.stderr)

    def test_unified_progress_schema_five_rejects_malformed_evidence_record(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            progress = root / "progress.json"
            progress.write_text(
                json.dumps(
                    {
                        "schema_version": 5,
                        "symbols": {},
                        "evidence": {
                            "recoil:evidence:r1:000001": "not an object",
                        },
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, progress)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("evidence['recoil:evidence:r1:000001'] must be an object", result.stderr)

    def test_unified_progress_schema_five_rejects_malformed_symbol_record(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            progress = root / "progress.json"
            progress.write_text(
                json.dumps(
                    {
                        "schema_version": 5,
                        "symbols": {
                            "recoil:data:0x401000": "not an object",
                        },
                        "evidence": {},
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, progress)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("symbols['recoil:data:0x401000'] must be an object", result.stderr)

    def test_unified_progress_schema_five_rejects_malformed_typed_initializer(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            progress = root / "progress.json"
            progress.write_text(
                json.dumps(
                    {
                        "schema_version": 5,
                        "symbols": {
                            "recoil:data:0x401000": {
                                "kind": "data",
                                "binary": "recoil",
                                "address": "0x401000",
                                "legacy_initializer": "not an object",
                                "evidence_ids": ["recoil:evidence:r1:000001"],
                            }
                        },
                        "evidence": {},
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, progress)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("expected object field 'legacy_initializer'", result.stderr)

    def test_unified_progress_schema_five_rejects_unlinked_initializer_record(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            initializer = {
                "path": "src/sample.cpp",
                "name": "g_Table",
                "data_reimplemented": "✅",
                "expected": "concrete-initializer-list",
                "evidence": "schema-5 test evidence",
            }
            progress = root / "progress.json"
            progress.write_text(
                json.dumps(
                    {
                        "schema_version": 5,
                        "symbols": {
                            "recoil:data:0x401000": {
                                "kind": "data",
                                "binary": "recoil",
                                "address": "0x401000",
                                "legacy_initializer": initializer,
                                "evidence_ids": ["recoil:evidence:r1:000001"],
                            }
                        },
                        "evidence": {
                            "recoil:evidence:r1:000001": {
                                "kind": "legacy-data-initializer",
                                "provenance": {},
                            },
                        },
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, progress)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("provenance expected object field 'record'", result.stderr)

    def test_unified_progress_schema_five_orphan_legacy_evidence_is_non_gating(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "sample.cpp").write_text(
                "Record g_Table[1] = {0};\n",
                encoding="utf-8",
            )
            progress = root / "progress.json"
            progress.write_text(
                json.dumps(
                    {
                        "schema_version": 5,
                        "symbols": {},
                        "evidence": {
                            "recoil:evidence:r1:000001": {
                                "kind": "legacy-data-initializer",
                                "provenance": {
                                    "record": {
                                        "path": "src/sample.cpp",
                                        "name": "g_Table",
                                        "data_reimplemented": "✅",
                                        "expected": "concrete-initializer-list",
                                        "evidence": "orphan historical observation",
                                    }
                                },
                            }
                        },
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, progress)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_unsupported_unified_progress_schema_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            progress = root / "progress.json"
            progress.write_text(
                json.dumps(
                    {
                        "schema_version": 4,
                        "globals": [],
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(root, progress)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unsupported unified progress schema_version 4", result.stderr)

    def test_path_alias_accepts_single_file_scan_for_handoff_compatibility(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            agent_dir = root / ".agent"
            source_dir.mkdir()
            agent_dir.mkdir()
            source = source_dir / "sample.cpp"
            source.write_text(
                "Record g_Table[3] = {\n"
                "    {\"first\", 1},\n"
                "    {\"second\", 2},\n"
                "    {\"third\", 3},\n"
                "};\n",
                encoding="utf-8",
            )
            progress = agent_dir / "RECONSTRUCTION_PROGRESS.json"
            progress.write_text(
                json.dumps(
                    {
                        "globals": [
                            {
                                "path": "src/sample.cpp",
                                "name": "g_Table",
                                "data_reimplemented": "✅",
                                "expected": "concrete-initializer-list",
                                "evidence": "test evidence",
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )
            result = self.run_guard(
                source,
                progress,
                cwd=root,
                extra_args=("--summary",),
                use_path_alias=True,
            )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_default_manifest_loads_sqlite_semantics(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_dir = root / "src"
            agent_dir = root / ".agent"
            source_dir.mkdir()
            agent_dir.mkdir()
            (source_dir / "sample.cpp").write_text(
                'Record g_Table[1] = {{"first", 1}};\n', encoding="utf-8"
            )
            initializer = {
                "path": "src/sample.cpp",
                "name": "g_Table",
                "data_reimplemented": "✅",
                "expected": "concrete-initializer-list",
                "evidence": "test evidence",
            }
            payload = empty_progress_document()
            payload["symbols"]["recoil:data:0x401000"] = {
                "kind": "data",
                "binary": "recoil",
                "address": "0x401000",
                "legacy_initializer": initializer,
                "evidence_ids": ["recoil:evidence:r0:000001"],
            }
            payload["evidence"]["recoil:evidence:r0:000001"] = {
                "kind": "legacy-data-initializer",
                "provenance": {"record": initializer},
            }
            ProgressSQLiteStore.create_from_mapping(
                agent_dir / "RECONSTRUCTION_PROGRESS.sqlite3",
                payload,
                cutover_pair_id="source-data-test",
            )
            result = self.run_guard(source_dir, cwd=root)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
