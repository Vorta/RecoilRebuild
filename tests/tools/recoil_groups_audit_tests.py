from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))
CLI = REPO_ROOT / "tools" / "recoil_groups_audit.py"

from recoil_groups_audit import audit_groups, parse_groups  # noqa: E402
from recoil_plan import PlanDocument  # noqa: E402


PLAN_TEXT = textwrap.dedent(
    """\
    # Test Plan

    ## M01 Test

    - 0x401000:
      - [✅] Reconstructed (Name: VerifiedLeaf)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: VerifiedLeaf File: src/VerifiedLeaf.cpp)
      - [✅] Functional-equivalent (Target: verified_leaf)
      - [✅] Binary-safe

    - 0x401020:
      - [✅] Reconstructed (Name: SourceDone)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: SourceDone File: src/SourceDone.cpp)
      - [✅] Functional-equivalent (Target: source_done)
      - [❌] Binary-safe

    - 0x401040:
      - [✅] Reconstructed (Name: SourcePending)
      - [❓] Source dependencies satisfied
      - [❌] Reimplemented (Name: pending File: pending)
      - [❌] Functional-equivalent (Target: pending)
      - [❌] Binary-safe

    - 0x401060:
      - [✅] Reconstructed (Name: NeedsFunctional)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: NeedsFunctional File: src/NeedsFunctional.cpp)
      - [❌] Functional-equivalent (Target: pending)
      - [❌] Binary-safe
    """
)


GROUPS_TEXT = textwrap.dedent(
    """\
    # Implementation Groups

    ## Active Groups

    ### Group: source pending group

    - Plan entries:
      - 0x401040 SourcePending - source blocked

    ### Group: condense group

    - Plan entries:
      - 0x401020 SourceDone - Reimplemented; binary-safe pending
    - Notes:
      - Hex constants such as 0xf80000 are not plan addresses.

    ### Group: functional pending group

    - Plan entries:
      - 0x401060 NeedsFunctional - functional evidence pending

    ### Group: move group

    - Plan entries:
      - 0x401000 VerifiedLeaf - Binary-safe

    ## legacy active note

    - Current dependency frontier:
      - 0x499999 Missing - pending

    ## Completed Groups

    ### Group: completed group

    - Plan entries:
      - 0x401000 VerifiedLeaf - Binary-safe
    """
)


class RecoilGroupsAuditTests(unittest.TestCase):
    def test_parse_groups_skips_template_area_and_keeps_sections(self) -> None:
        groups = parse_groups(GROUPS_TEXT)

        self.assertEqual(
            [
                "source pending group",
                "condense group",
                "functional pending group",
                "move group",
                "legacy active note",
                "completed group",
            ],
            [group.title for group in groups],
        )
        self.assertEqual(
            ["active", "active", "active", "active", "active", "completed"],
            [group.section for group in groups],
        )
        self.assertFalse(groups[4].standard_heading)

    def test_audit_classifies_groups_against_plan_markers(self) -> None:
        plan = PlanDocument.load_from_lines(Path("test"), PLAN_TEXT.splitlines()).entries
        audits = audit_groups(parse_groups(GROUPS_TEXT), plan)
        recommendations = {audit.group.title: audit.recommendation for audit in audits}

        self.assertEqual("keep-active", recommendations["source pending group"])
        self.assertEqual("condense", recommendations["condense group"])
        self.assertEqual("keep-active", recommendations["functional pending group"])
        self.assertEqual("move-completed", recommendations["move group"])
        self.assertEqual("needs-review", recommendations["legacy active note"])
        self.assertEqual("completed", recommendations["completed group"])

        legacy = next(audit for audit in audits if audit.group.title == "legacy active note")
        self.assertEqual(("0x499999",), legacy.missing_addresses)
        self.assertIn("nonstandard heading", legacy.reasons)

        condense = next(audit for audit in audits if audit.group.title == "condense group")
        self.assertEqual((), condense.missing_addresses)

    def test_cli_smoke_on_real_files(self) -> None:
        result = subprocess.run(
            [
                sys.executable,
                str(CLI),
                "--summary",
                "--wip-limit",
                "1000",
                "--limit",
                "3",
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("groups:", result.stdout)
        self.assertIn("recommendations:", result.stdout)
        self.assertIn("active_wip_limit:", result.stdout)

    def test_cli_strict_wip_fails_when_limit_is_exceeded(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            groups_path = tmp_path / "IMPLEMENTATION_GROUPS.md"
            plan_path = tmp_path / "RECOIL_PLAN.md"
            groups_path.write_text(GROUPS_TEXT, encoding="utf-8")
            plan_path.write_text(PLAN_TEXT, encoding="utf-8")

            result = subprocess.run(
                [
                    sys.executable,
                    str(CLI),
                    "--groups",
                    str(groups_path),
                    "--plan",
                    str(plan_path),
                    "--section",
                    "active",
                    "--wip-limit",
                    "0",
                    "--strict-wip",
                    "--limit",
                    "0",
                ],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )

        self.assertEqual(result.returncode, 1)


if __name__ == "__main__":
    unittest.main()
