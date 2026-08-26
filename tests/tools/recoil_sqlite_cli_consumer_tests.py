from __future__ import annotations

import contextlib
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

import recoil  # noqa: E402
from _recoil.lib.progress import empty_progress_document  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402
from _recoil.lib.source_owners import SourceOwnerDocument  # noqa: E402
from _recoil.lib.source_traceability import load_artifact_rows  # noqa: E402
from _recoil.commands.vc5_manifest_source_guard import (  # noqa: E402
    progress_physical_block_source_paths,
)
from _recoil.pipeline_context import load_pipeline_context  # noqa: E402


class SQLiteCliConsumerTests(unittest.TestCase):
    def test_new_commands_are_registered(self) -> None:
        self.assertEqual(
            "ledger_sqlite_migration",
            recoil.COMMANDS[("maintenance", "migrate-ledgers-sqlite")].module,
        )
        self.assertEqual(
            "state_performance_audit",
            recoil.COMMANDS[("audit", "state-performance")].module,
        )

    def test_progress_signature_reads_semantic_revision(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.sqlite3"
            document = empty_progress_document()
            document["revision"] = 17
            ProgressSQLiteStore.create_from_mapping(
                path, document, cutover_pair_id="test-pair"
            )
            self.assertEqual(17, recoil.progress_file_signature(path))

    def test_launcher_rejects_explicit_json_ledger_before_dispatch(self) -> None:
        stderr = io.StringIO()
        with (
            contextlib.redirect_stderr(stderr),
            mock.patch.object(recoil.subprocess, "run") as run,
        ):
            result = recoil.main(
                ["progress", "next", "--progress", "build/legacy.json", "--json"]
            )
        self.assertEqual(2, result)
        self.assertIn("no longer accepts a JSON ledger", stderr.getvalue())
        run.assert_not_called()

    def test_pipeline_context_materializes_sqlite_through_store(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                path, empty_progress_document(), cutover_pair_id="test-pair"
            )
            context = load_pipeline_context(path)
        self.assertEqual(0, context.tracker_revision)
        self.assertEqual(path.as_posix(), context.tracker_path)

    def test_projection_consumers_read_sqlite_through_progress_store(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.sqlite3"
            document = empty_progress_document()
            document["symbols"]["recoil:function:0x401000"] = {
                "kind": "function",
                "pipeline_class": "authored",
                "output_section_id": "recoil:section:.text",
            }
            document["physical_blocks"]["recoil:block:test"] = {
                "binary": "recoil",
                "row_kind": "physical-block",
                "agent_source_path": "src/Test.cpp",
            }
            ProgressSQLiteStore.create_from_mapping(
                path, document, cutover_pair_id="test-pair"
            )
            owners = SourceOwnerDocument.load(path)
            artifacts = load_artifact_rows(path)
            paths = progress_physical_block_source_paths(path)
        self.assertEqual([], owners.owners)
        self.assertEqual(
            "recoil:function:0x401000",
            artifacts.resolve("recoil:function:0x401000").physical_id,
        )
        self.assertEqual({"src/test.cpp"}, paths)


if __name__ == "__main__":
    unittest.main()
