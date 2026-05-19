from __future__ import annotations

import os
import sys
import unittest
from pathlib import Path
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_msvc_x86_run import build_script, candidate_vcvarsall_paths, normalize_command  # noqa: E402


class RecoilMsvcX86RunTests(unittest.TestCase):
    def test_normalize_command_accepts_separator(self) -> None:
        self.assertEqual(["cmake", "--preset", "ninja-x86-debug"], normalize_command(["--", "cmake", "--preset", "ninja-x86-debug"]))

    def test_normalize_command_requires_command(self) -> None:
        with self.assertRaises(ValueError):
            normalize_command(["--"])

    def test_candidate_paths_honor_explicit_environment(self) -> None:
        with mock.patch.dict(os.environ, {"RECOIL_VCVARSALL": r"C:\VS\vcvarsall.bat"}, clear=True):
            candidates = candidate_vcvarsall_paths()

        self.assertEqual(Path(r"C:\VS\vcvarsall.bat"), candidates[0])

    def test_build_script_calls_vcvarsall_x86_then_command(self) -> None:
        script = build_script(Path(r"C:\VS Tools\vcvarsall.bat"), ["cmake", "--preset", "ninja-x86-debug"])

        self.assertIn(r'call "C:\VS Tools\vcvarsall.bat" x86 >nul', script)
        self.assertIn("cmake --preset ninja-x86-debug", script)
        self.assertTrue(script.endswith("exit /b %errorlevel%\r\n"))


if __name__ == "__main__":
    unittest.main()
