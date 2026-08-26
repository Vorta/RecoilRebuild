from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "provider_boundary_guard.py"


class RecoilProviderBoundaryGuardTests(unittest.TestCase):
    def run_guard(self, root: Path, *extra: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                str(SCRIPT),
                "--root",
                str(root),
                *extra,
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def make_temp(self) -> tuple[tempfile.TemporaryDirectory[str], Path]:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name) / "src"
        root.mkdir()
        return temp, root

    def test_clean_source_summary_passes(self) -> None:
        _, root = self.make_temp()
        (root / "clean.cpp").write_text(
            "struct GameOwnedState { int value; };\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--summary")

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("provider-boundary production-source summary:", result.stdout)
        self.assertIn("current violations: 0", result.stdout)

    def test_summary_preserves_provider_guard_failure(self) -> None:
        _, root = self.make_temp()
        (root / "bad.h").write_text(
            "struct VendorProviderThingVTable { void *slot; };\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, "--summary", "--top", "1")

        self.assertEqual(result.returncode, 1)
        self.assertIn("provider-boundary production-source summary:", result.stdout)
        self.assertIn("current violations: 1", result.stdout)
        self.assertIn("local provider vtable shim", result.stdout)
        self.assertIn("bad.h:1", result.stdout)


if __name__ == "__main__":
    unittest.main()
