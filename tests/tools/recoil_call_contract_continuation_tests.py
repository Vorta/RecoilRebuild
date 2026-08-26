from __future__ import annotations

import io
from pathlib import Path
import sys
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from _recoil.commands import call_contract_continuation as continuation  # noqa: E402
from _recoil.commands import progress_cli  # noqa: E402
from _recoil.lib.progress import ProgressError  # noqa: E402


class CallContractContinuationTests(unittest.TestCase):
    def test_continuation_module_is_contained_disabled(self) -> None:
        state = continuation.continuation_state()
        self.assertEqual("contained-disabled", state["status"])
        self.assertFalse(state["active"])
        self.assertIn("active-packet producer", state["reason"])
        self.assertFalse(continuation.continuation_snapshots_equal({}, {}))
        self.assertFalse(continuation.validate_continuation_checkpoint({}))
        self.assertEqual(
            "contained-disabled",
            continuation.returned_tool_blocked_provenance()["status"],
        )

    def test_every_mutating_or_expensive_module_entry_point_fails_closed(self) -> None:
        for entry_point in (
            continuation.capture_continuation_input_snapshot,
            continuation.prepare_repair_continuation,
            continuation.activate_continuation_child,
            continuation.finalize_continuation_child,
            continuation.archive_continuation_checkpoint,
        ):
            with self.subTest(entry_point=entry_point.__name__):
                with self.assertRaisesRegex(ProgressError, "contained-disabled"):
                    entry_point()

    def test_cli_fails_before_ledger_root_evaluator_compiler_or_binja(self) -> None:
        argv = [
            "call-contract",
            "prepare-repair-continuation",
            "--returned-work-item",
            "recoil:work:returned",
            "--linked-tool-issue",
            "WSI-fixture",
            "--build-root",
            "build/forbidden-continuation",
            "--expected-revision",
            "17",
            "--apply",
            "--json",
        ]
        forbidden = AssertionError("packetless continuation crossed containment")
        with mock.patch.object(
            progress_cli, "_load", side_effect=forbidden
        ), mock.patch.object(
            progress_cli, "_absolute_fresh_build_root", side_effect=forbidden
        ), mock.patch.object(
            progress_cli, "prepare_repair_continuation", side_effect=forbidden
        ), mock.patch.object(
            progress_cli, "_run_json_process", side_effect=forbidden
        ), mock.patch.object(
            progress_cli, "BinaryNinjaBridge", side_effect=forbidden
        ), mock.patch(
            "_recoil.commands.workspace_issues.issue_store", side_effect=forbidden
        ), mock.patch(
            "sys.stderr", new=io.StringIO()
        ):
            self.assertEqual(2, progress_cli.main(argv))

    def test_cli_parser_requires_the_explicit_contained_route_inputs(self) -> None:
        parser = progress_cli._parser()
        base = ["call-contract", "prepare-repair-continuation"]
        with mock.patch("sys.stderr", new=io.StringIO()), self.assertRaises(SystemExit):
            parser.parse_args(base)
        parsed = parser.parse_args(
            [
                *base,
                "--returned-work-item",
                "recoil:work:returned",
                "--linked-tool-issue",
                "WSI-fixture",
                "--build-root",
                "build/forbidden-continuation",
                "--expected-revision",
                "17",
                "--apply",
            ]
        )
        self.assertEqual("prepare-repair-continuation", parsed.call_contract_command)


if __name__ == "__main__":
    unittest.main()
