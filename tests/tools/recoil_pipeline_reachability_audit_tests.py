from __future__ import annotations

import sys
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands.pipeline_reachability_audit import (  # noqa: E402
    STAGE_COMMANDS,
    audit_reachability,
)
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH  # noqa: E402


class PipelineReachabilityAuditTests(unittest.TestCase):
    def test_every_serial_stage_has_one_direct_route(self) -> None:
        self.assertEqual(
            {
                "authored-function-order",
                "authored-call-contract",
                "authored-byte-match",
                "full-function-order",
                "linked-byte-match",
                "final-validation",
            },
            set(STAGE_COMMANDS),
        )

    def test_live_current_task_is_reachable(self) -> None:
        result = audit_reachability(DEFAULT_PROGRESS_PATH)
        self.assertTrue(result["passed"], result["findings"])


if __name__ == "__main__":
    unittest.main()
