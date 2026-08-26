from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "multiline_style_guard.py"


class RecoilMultilineStyleGuardTests(unittest.TestCase):
    def run_guard(self, root: Path) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(SCRIPT), "--root", str(root)],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def make_temp_src(self) -> tuple[tempfile.TemporaryDirectory[str], Path]:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name) / "src"
        root.mkdir()
        return temp, root

    def test_rejects_first_parameter_on_opening_line(self) -> None:
        _, root = self.make_temp_src()
        (root / "bad.h").write_text(
            "struct Sink\n"
            "{\n"
            "    static int OnPendingSessionRequestRemoved(void *callbackContext,\n"
            "                                           int status,\n"
            "                                           WestwoodOnlineUpgradeSessionRequest *request);\n"
            "};\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 1)
        self.assertIn("first parameter/argument", result.stdout)

    def test_accepts_block_wrapped_parameters(self) -> None:
        _, root = self.make_temp_src()
        (root / "good.h").write_text(
            "struct Sink\n"
            "{\n"
            "    static int OnPendingSessionRequestRemoved(\n"
            "        void *callbackContext,\n"
            "        int status,\n"
            "        WestwoodOnlineUpgradeSessionRequest *request\n"
            "    );\n"
            "};\n",
            encoding="utf-8",
        )

        result = self.run_guard(root)

        self.assertEqual(result.returncode, 0, result.stdout)


if __name__ == "__main__":
    unittest.main()
