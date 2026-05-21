from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
CLI = REPO_ROOT / "tools" / "recoil_plan_cli.py"

PLAN_TEXT = textwrap.dedent(
    """\
    # Test Plan

    ## M01 Test Milestone

    - 0x401000:
      - [✅] Reconstructed (Name: TestLeaf)
      - [✅] Source dependencies satisfied
      - [❌] Reimplemented (Name: pending File: pending)
      - [❌] Binary-safe verified

    - 0x401020:
      - [❌] Reconstructed (Name: TestCaller)
      - [❌] Source dependencies satisfied
      - [❌] Reimplemented (Name: pending File: pending)
      - [❌] Binary-safe verified

    - 0x401040:
      - [✅] Reconstructed (Name: FunctionalOnly)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: FunctionalOnly File: src/FunctionalOnly.cpp)
      - [❌] Binary-safe verified
      - [✅] Functional-equivalent accepted (Target: functional_only)
    """
)


class RecoilPlanCliTests(unittest.TestCase):
    def run_cli(self, plan: Path, *args: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(CLI), "--plan", str(plan), *args],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def with_plan(self) -> tempfile.TemporaryDirectory[str]:
        temp = tempfile.TemporaryDirectory()
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        plan.write_text(PLAN_TEXT, encoding="utf-8")
        self.addCleanup(temp.cleanup)
        return temp

    def test_show_prints_one_entry(self) -> None:
        temp = self.with_plan()
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        result = self.run_cli(plan, "show", "0x401000")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("0x401000", result.stdout)
        self.assertIn("TestLeaf", result.stdout)
        self.assertNotIn("0x401020", result.stdout)

    def test_find_and_milestone_print_compact_entries(self) -> None:
        temp = self.with_plan()
        plan = Path(temp.name) / "RECOIL_PLAN.md"

        find_result = self.run_cli(plan, "find", "TestCaller")
        self.assertEqual(find_result.returncode, 0, find_result.stderr)
        self.assertIn("0x401020", find_result.stdout)
        self.assertIn("blocker=recon", find_result.stdout)
        self.assertIn("name=TestCaller file=pending", find_result.stdout)

        milestone_result = self.run_cli(plan, "milestone", "M01")
        self.assertEqual(milestone_result.returncode, 0, milestone_result.stderr)
        self.assertIn("0x401000", milestone_result.stdout)
        self.assertIn("0x401020", milestone_result.stdout)

    def test_dry_run_does_not_mutate_file(self) -> None:
        temp = self.with_plan()
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        before = plan.read_text(encoding="utf-8")
        result = self.run_cli(plan, "set", "0x401000", "deps", "❌", "--dry-run")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Before:", result.stdout)
        self.assertIn("After:", result.stdout)
        self.assertEqual(plan.read_text(encoding="utf-8"), before)

    def test_set_updates_one_marker(self) -> None:
        temp = self.with_plan()
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        result = self.run_cli(plan, "set", "0x401000", "deps", "❌")
        self.assertEqual(result.returncode, 0, result.stderr)
        updated = plan.read_text(encoding="utf-8")
        self.assertIn("  - [❌] Source dependencies satisfied", updated)
        self.assertIn("  - [✅] Reconstructed (Name: TestLeaf)", updated)

    def test_done_updates_require_evidence(self) -> None:
        temp = self.with_plan()
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        result = self.run_cli(plan, "set", "0x401020", "deps", "✅")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("requires --evidence", result.stderr)

    def test_set_reimplemented_requires_and_writes_name_file(self) -> None:
        temp = self.with_plan()
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        result = self.run_cli(
            plan,
            "set",
            "0x401000",
            "impl",
            "✅",
            "--name",
            "TestLeaf",
            "--file",
            "src/Test.cpp",
            "--evidence",
            "unit test evidence",
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        updated = plan.read_text(encoding="utf-8")
        self.assertIn("  - [✅] Reimplemented (Name: TestLeaf File: src/Test.cpp)", updated)

    def test_set_reimplemented_unknown_resets_to_pending(self) -> None:
        temp = self.with_plan()
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        result = self.run_cli(plan, "set", "0x401000", "impl", "❓")
        self.assertEqual(result.returncode, 0, result.stderr)
        updated = plan.read_text(encoding="utf-8")
        self.assertIn("  - [❓] Reimplemented (Name: pending File: pending)", updated)

    def test_set_functional_marker_inserts_optional_line(self) -> None:
        temp = self.with_plan()
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        result = self.run_cli(
            plan,
            "set",
            "0x401000",
            "functional",
            "✅",
            "--file",
            "test_leaf_functional",
            "--evidence",
            "functional smoke evidence",
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        updated = plan.read_text(encoding="utf-8")
        self.assertIn(
            "  - [✅] Functional-equivalent accepted (Target: test_leaf_functional)",
            updated,
        )

    def test_next_progress_lane_skips_functionally_accepted_byte_blocker(self) -> None:
        temp = self.with_plan()
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        result = self.run_cli(plan, "next", "--lane", "progress")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("0x401000", result.stdout)
        self.assertIn("next_field=impl", result.stdout)

    def test_strict_lane_keeps_functionally_accepted_byte_blocker(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            plan = Path(tmp) / "RECOIL_PLAN.md"
            plan.write_text(
                "# Test Plan\n"
                "\n"
                "## M01 Test\n"
                "\n"
                "- 0x401000:\n"
                "  - [✅] Reconstructed (Name: FunctionalOnly)\n"
                "  - [✅] Source dependencies satisfied\n"
                "  - [✅] Reimplemented (Name: FunctionalOnly File: src/FunctionalOnly.cpp)\n"
                "  - [❌] Binary-safe verified\n"
                "  - [✅] Functional-equivalent accepted (Target: functional_only)\n",
                encoding="utf-8",
            )
            strict_result = self.run_cli(plan, "next")
            progress_result = self.run_cli(plan, "next", "--lane", "progress")

        self.assertEqual(strict_result.returncode, 0, strict_result.stderr)
        self.assertIn("next_field=verify", strict_result.stdout)
        self.assertEqual(progress_result.returncode, 0, progress_result.stderr)
        self.assertIn("No unfinished plan entries found.", progress_result.stdout)

    def test_unknown_address_fails(self) -> None:
        temp = self.with_plan()
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        result = self.run_cli(plan, "show", "0x499999")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("plan entry not found", result.stderr)

    def test_save_preserves_crlf_line_endings(self) -> None:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        plan = Path(temp.name) / "RECOIL_PLAN.md"
        plan.write_bytes(PLAN_TEXT.replace("\n", "\r\n").encode("utf-8"))
        result = self.run_cli(plan, "set", "0x401000", "deps", "❌")
        self.assertEqual(result.returncode, 0, result.stderr)
        data = plan.read_bytes()
        self.assertIn(b"\r\n", data)
        self.assertNotIn(b"\r\r\n", data)


if __name__ == "__main__":
    unittest.main()
