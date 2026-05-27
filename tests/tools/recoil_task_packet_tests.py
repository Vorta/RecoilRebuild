from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace
import sys
import textwrap
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_plan import PlanDocument  # noqa: E402
from recoil_task_packet import select_entry  # noqa: E402


PLAN_TEXT = textwrap.dedent(
    """\
    # Test Plan

    ## M01 Test

    - 0x401000:
      - [✅] Reconstructed (Name: Done)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: Done File: src/Done.cpp)
      - [✅] Functional-equivalent (Target: done)
      - [✅] Binary-safe

    - 0x401020:
      - [✅] Reconstructed (Name: Pending)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: Pending File: src/Pending.cpp)
      - [❌] Functional-equivalent (Target: pending)
      - [❌] Binary-safe
    """
)


class RecoilTaskPacketTests(unittest.TestCase):
    def test_select_entry_uses_specific_address(self) -> None:
        doc = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())
        args = SimpleNamespace(address="0x401000", lane="functional")

        entry = select_entry(args, doc)

        self.assertEqual("0x401000", entry.address)

    def test_select_entry_defaults_to_first_unfinished(self) -> None:
        doc = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())
        args = SimpleNamespace(address="", lane="functional")

        entry = select_entry(args, doc)

        self.assertEqual("0x401020", entry.address)


if __name__ == "__main__":
    unittest.main()
