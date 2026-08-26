from __future__ import annotations

from pathlib import Path
import sys
import unittest
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands import state_performance_audit as audit  # noqa: E402


class StatePerformanceAuditTests(unittest.TestCase):
    def test_percentile_uses_nearest_rank(self) -> None:
        self.assertEqual(5.0, audit.percentile([5, 1, 3, 2, 4], 0.95))

    def test_check_requires_successful_processes_and_strict_ceiling(self) -> None:
        passing = audit.Check("read", 0.25, (0.1,) * 5, (0,) * 5, ("",) * 5)
        at_limit = audit.Check("read", 0.25, (0.25,) * 5, (0,) * 5, ("",) * 5)
        failed = audit.Check("read", 0.25, (0.1,) * 5, (0, 0, 1, 0, 0), ("",) * 5)
        self.assertTrue(passing.passed)
        self.assertFalse(at_limit.passed)
        self.assertFalse(failed.passed)

    def test_main_rejects_fewer_than_five_fresh_process_samples(self) -> None:
        self.assertEqual(2, audit.main(["--samples", "4"]))

    def test_named_check_preserves_durations_and_limit(self) -> None:
        measured = audit.Check("", 0.0, (0.01,) * 5, (0,) * 5, ("",) * 5)
        with mock.patch.object(audit, "_measure", return_value=measured):
            result = audit._named_check("revision", 0.25, ["python"], samples=5)
        self.assertEqual("revision", result.name)
        self.assertEqual(0.25, result.limit_seconds)
        self.assertEqual((0.01,) * 5, result.durations)


if __name__ == "__main__":
    unittest.main()
