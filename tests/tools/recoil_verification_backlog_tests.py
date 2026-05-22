from __future__ import annotations

from pathlib import Path
import sys
import textwrap
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_plan import PlanDocument  # noqa: E402
from recoil_verification_backlog import build_backlog, source_blocked_gap_count  # noqa: E402


PLAN_TEXT = textwrap.dedent(
    """\
    # Test Plan

    ## M01 Test

    - 0x401000:
      - [✅] Reconstructed (Name: NeedsFunctional)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: NeedsFunctional File: src/NeedsFunctional.cpp)
      - [❌] Functional-equivalent (Target: pending)
      - [❌] Binary-safe

    - 0x401020:
      - [✅] Reconstructed (Name: TestClass::NeedsBinary)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: TestClass::NeedsBinary File: src/NeedsBinary.cpp)
      - [✅] Functional-equivalent (Target: needs_binary)
      - [❌] Binary-safe

    - 0x401040:
      - [✅] Reconstructed (Name: Provider)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Provided by compiler/runtime; Name: Provider File: external)
      - [❌] Functional-equivalent (Target: pending)
      - [❌] Binary-safe

    - 0x401060:
      - [✅] Reconstructed (Name: SourceBlockedImpl)
      - [❌] Source dependencies satisfied
      - [✅] Reimplemented (Name: SourceBlockedImpl File: src/SourceBlockedImpl.cpp)
      - [❌] Functional-equivalent (Target: pending)
      - [❌] Binary-safe
    """
)


class RecoilVerificationBacklogTests(unittest.TestCase):
    def test_functional_lane_filters_provider_entries(self) -> None:
        doc = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())

        items = build_backlog(doc, set(), {"0x401000"})

        self.assertEqual(["0x401000"], [item.address for item in items])
        self.assertEqual("src/NeedsFunctional.cpp", items[0].source_file)
        self.assertTrue(items[0].has_functional_target)
        self.assertEqual(1, source_blocked_gap_count(doc, lane="functional"))

    def test_binary_lane_groups_functionally_equivalent_binary_debt(self) -> None:
        doc = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())

        items = build_backlog(doc, {"0x401020"}, lane="binary")

        self.assertEqual(["0x401020"], [item.address for item in items])
        self.assertEqual("TestClass", items[0].group)
        self.assertTrue(items[0].has_vc_target)

    def test_build_backlog_can_filter_by_source_file(self) -> None:
        doc = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())

        items = build_backlog(
            doc,
            set(),
            lane="binary",
            source_filter="src/NeedsBinary.cpp",
        )

        self.assertEqual(["0x401020"], [item.address for item in items])


if __name__ == "__main__":
    unittest.main()
