from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
PROGRESS = REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"


class ProgressQueryPurityTests(unittest.TestCase):
    @staticmethod
    def digest() -> str:
        return hashlib.sha256(PROGRESS.read_bytes()).hexdigest()

    def test_all_current_task_views_are_read_only(self) -> None:
        before = self.digest()
        for command in ("next", "status"):
            completed = subprocess.run(
                [sys.executable, "tools/recoil.py", "progress", command, "--json"],
                cwd=REPO_ROOT,
                check=True,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )
            self.assertIn('"schema": "recoil-current-task-v2"', completed.stdout)
        self.assertEqual(before, self.digest())


if __name__ == "__main__":
    unittest.main()
