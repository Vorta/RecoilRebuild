from __future__ import annotations

from pathlib import Path
import sys
import textwrap
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_plan import PlanDocument  # noqa: E402
from recoil_verification_backlog import build_backlog  # noqa: E402


PLAN_TEXT = textwrap.dedent(
    """\
    # Test Plan

    ## M01 Test

    - 0x401000:
      - [✅] Reconstructed (Name: NeedsManifest)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: NeedsManifest File: src/NeedsManifest.cpp)
      - [❌] Binary-safe verified

    - 0x401020:
      - [✅] Reconstructed (Name: Covered)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: Covered File: src/Covered.cpp)
      - [❌] Binary-safe verified

    - 0x401040:
      - [✅] Reconstructed (Name: Provider)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Provided by compiler/runtime; Name: Provider File: external)
      - [❌] Binary-safe verified
    """
)


class RecoilVerificationBacklogTests(unittest.TestCase):
    def test_build_backlog_filters_covered_and_provider_entries(self) -> None:
        doc = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())

        items = build_backlog(doc, {"0x401020"})

        self.assertEqual(["0x401000"], [item.address for item in items])
        self.assertEqual("src/NeedsManifest.cpp", items[0].source_file)

    def test_build_backlog_can_filter_by_source_file(self) -> None:
        doc = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())

        items = build_backlog(doc, set(), source_filter="src/Covered.cpp")

        self.assertEqual(["0x401020"], [item.address for item in items])


if __name__ == "__main__":
    unittest.main()

