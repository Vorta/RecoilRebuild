from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "recoil_status.py"


def write_plan(path: Path) -> None:
    path.write_text(
        "# Test Plan\n"
        "\n"
        "## M01. Sample\n"
        "\n"
        "- 0x401000:\n"
        "  - [✅] Reconstructed (Name: Sample)\n"
        "  - [✅] Source dependencies satisfied\n"
        "  - [✅] Reimplemented (Name: Sample File: src/sample.cpp)\n"
        "  - [❌] Binary-safe verified\n",
        encoding="utf-8",
    )


class RecoilStatusTests(unittest.TestCase):
    def test_cli_reports_plan_state_and_missing_vc6_coverage_without_frontier(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            plan = root / "RECOIL_PLAN.md"
            manifest_dir = root / "vc6"
            manifest_dir.mkdir()
            write_plan(plan)

            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--plan",
                    str(plan),
                    "--groups",
                    str(root / "missing_groups.md"),
                    "--vc6-manifest-dir",
                    str(manifest_dir),
                    "--no-frontier",
                ],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Address: 0x401000", result.stdout)
        self.assertIn("Next blocker: verify", result.stdout)
        self.assertIn("--explain-missing 0x401000", result.stdout)


if __name__ == "__main__":
    unittest.main()
