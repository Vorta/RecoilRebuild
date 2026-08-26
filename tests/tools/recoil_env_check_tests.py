from __future__ import annotations

import contextlib
from io import StringIO
import os
from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.env_check import EnvReport, find_case_insensitive_file, split_path_env  # noqa: E402


class RecoilEnvCheckTests(unittest.TestCase):
    def test_split_path_env_omits_empty_entries(self) -> None:
        value = os.pathsep.join(["first", "", "second", ""])

        paths = split_path_env(value)

        self.assertEqual([path.name for path in paths], ["first", "second"])

    def test_find_case_insensitive_file_matches_existing_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            expected = directory / "Windows.H"
            expected.write_text("", encoding="utf-8")

            result = find_case_insensitive_file([directory], "windows.h")

        self.assertEqual(result, expected)

    def test_find_case_insensitive_file_returns_none_for_missing_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = find_case_insensitive_file([Path(tmp)], "kernel32.lib")

        self.assertIsNone(result)

    def test_env_report_counts_warnings_and_failures(self) -> None:
        report = EnvReport()

        with contextlib.redirect_stdout(StringIO()):
            report.ok("ready")
            report.warn("limited")
            report.fail("missing")

        self.assertEqual(report.warnings, 1)
        self.assertEqual(report.failures, 1)


if __name__ == "__main__":
    unittest.main()
