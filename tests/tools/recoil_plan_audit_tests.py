from __future__ import annotations

import sys
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_plan import PlanDocument  # noqa: E402
from recoil_plan_audit import (  # noqa: E402
    find_impl_green_deps_not_green,
    find_verify_done_deps_not_green,
    find_verify_done_impl_not_green,
)


PLAN_TEXT = textwrap.dedent(
    """\
    # Test Plan

    ## M01 Test

    - 0x401000:
      - [✅] Reconstructed (Name: Good)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: Good File: src/Good.cpp)
      - [✅] Binary-safe

    - 0x401020:
      - [✅] Reconstructed (Name: ImplBeforeDeps)
      - [❓] Source dependencies satisfied
      - [✅] Reimplemented (Name: ImplBeforeDeps File: src/ImplBeforeDeps.cpp)
      - [❓] Binary-safe

    - 0x401040:
      - [✅] Reconstructed (Name: VerifyBeforeImpl)
      - [✅] Source dependencies satisfied
      - [❌] Reimplemented (Name: pending File: pending)
      - [✅] Binary-safe
    """
)


class RecoilPlanAuditTests(unittest.TestCase):
    def test_marker_consistency_finders(self) -> None:
        document = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())
        entries = list(document.entries.values())

        self.assertEqual(["0x401020"], [entry.address for entry in find_impl_green_deps_not_green(entries)])
        self.assertEqual(["0x401040"], [entry.address for entry in find_verify_done_impl_not_green(entries)])
        self.assertEqual([], find_verify_done_deps_not_green(entries))


if __name__ == "__main__":
    unittest.main()
