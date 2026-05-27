from __future__ import annotations

from pathlib import Path
import contextlib
from io import StringIO
import sys
import tempfile
import textwrap
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_handoff import print_address_report  # noqa: E402
from recoil_plan import PlanDocument  # noqa: E402


PLAN_TEXT = textwrap.dedent(
    """\
    # Test Plan

    ## M01 Test

    - 0x401000:
      - [✅] Reconstructed (Name: Sample)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: Sample File: src/Sample.cpp)
      - [❌] Binary-safe
    """
)


class RecoilHandoffTests(unittest.TestCase):
    def test_print_address_report_handles_missing_coverage(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "vc"
            functional_dir = root / "functional"
            manifest_dir.mkdir()
            functional_dir.mkdir()
            args = type(
                "Args",
                (),
                {
                    "groups": str(root / "missing_groups.md"),
                    "vc_manifest_dir": str(manifest_dir),
                    "functional_manifest_dir": str(functional_dir),
                    "include_artifacts": False,
                },
            )()
            doc = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines())
            output = StringIO()

            with contextlib.redirect_stdout(output):
                print_address_report("0x401000", args, doc)

        self.assertIn("0x401000", output.getvalue())
        self.assertIn("- vc: none", output.getvalue())
        self.assertIn("- functional: none", output.getvalue())


if __name__ == "__main__":
    unittest.main()
