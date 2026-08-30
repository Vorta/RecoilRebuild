from __future__ import annotations

from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands import workspace_issues  # noqa: E402
from _recoil.lib.issue_sqlite import create_issue_database, export_issue_document  # noqa: E402
from _recoil.lib.live_progress import ISSUE_LEDGER_VERSION  # noqa: E402


class WorkspaceIssuesTests(unittest.TestCase):
    def test_report_and_resolve_without_work_allocation(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "issues.sqlite3"
            create_issue_database(
                path,
                {
                    "version": ISSUE_LEDGER_VERSION,
                    "revision": 0,
                    "id_sequences": {},
                    "issues": [],
                },
                cutover_pair_id="serial-test",
            )
            output = io.StringIO()
            with redirect_stdout(output):
                code = workspace_issues.main(
                    [
                        "report",
                        "--ledger",
                        str(path),
                        "--kind",
                        "tool-error",
                        "--severity",
                        "low",
                        "--summary",
                        "fixture",
                        "--area",
                        "tests",
                        "--impact",
                        "none",
                        "--next-action",
                        "resolve",
                        "--actual",
                        "fixture",
                        "--expected-revision",
                        "0",
                        "--apply",
                    ]
                )
            self.assertEqual(0, code)
            created = json.loads(output.getvalue())
            issue_id = created["issue"]["id"]
            document = export_issue_document(path)
            self.assertEqual(1, document["revision"])
            self.assertNotIn("work_packets", document)
            self.assertNotIn("reservations", document)

            with redirect_stdout(io.StringIO()):
                code = workspace_issues.main(
                    [
                        "resolve",
                        issue_id,
                        "--ledger",
                        str(path),
                        "--resolution",
                        "done",
                        "--expected-revision",
                        "1",
                        "--apply",
                    ]
                )
            self.assertEqual(0, code)
            self.assertEqual("resolved", export_issue_document(path)["issues"][0]["status"])


if __name__ == "__main__":
    unittest.main()
