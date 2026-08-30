from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]


class LiveProgressTests(unittest.TestCase):
    def invoke(self, command: str) -> dict[str, object]:
        completed = subprocess.run(
            [sys.executable, "tools/recoil.py", "progress", command, "--json"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )
        return json.loads(completed.stdout)

    def test_next_status_and_report_share_one_current_task_contract(self) -> None:
        results = [self.invoke(command) for command in ("next", "status", "report")]
        self.assertEqual(results[0], results[1])
        self.assertEqual(results[0], results[2])
        task = results[0]
        self.assertEqual("recoil-current-task-v2", task["schema"])
        self.assertIn(task["stage"], {
            "authored-function-order",
            "authored-call-contract",
            "authored-byte-match",
            "full-function-order",
            "linked-byte-match",
            "final-validation",
        })
        serialized = json.dumps(task).lower()
        for retired in ("packet", "worker", "lease", "claim", "scheduler_revision"):
            self.assertNotIn(retired, serialized)


if __name__ == "__main__":
    unittest.main()
