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
      - [✅] Binary-safe verified

    - 0x401020:
      - [✅] Reconstructed (Name: Pending)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: Pending File: src/Pending.cpp)
      - [❌] Binary-safe verified
    """
)


class RecoilTaskPacketTests(unittest.TestCase):
    def test_select_entry_uses_specific_address(self) -> None:
        doc = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())
        args = SimpleNamespace(address="0x401000", claim_next=False, lane="strict", claims_dir="")

        entry, should_claim = select_entry(args, doc)

        self.assertEqual("0x401000", entry.address)
        self.assertFalse(should_claim)

    def test_select_entry_defaults_to_first_unfinished(self) -> None:
        doc = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())
        args = SimpleNamespace(address="", claim_next=False, lane="strict", claims_dir="")

        entry, should_claim = select_entry(args, doc)

        self.assertEqual("0x401020", entry.address)
        self.assertFalse(should_claim)


if __name__ == "__main__":
    unittest.main()

