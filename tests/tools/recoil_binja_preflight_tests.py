from __future__ import annotations

import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_binja_preflight import validate_binaries, validate_probe, validate_status  # noqa: E402


EXPECTED_FILE = "D:/Recoil Project/Decomp/Recoil.bndb"


class RecoilBinjaPreflightTests(unittest.TestCase):
    def test_validate_status_accepts_expected_loaded_database(self) -> None:
        status = {
            "loaded": True,
            "filename": EXPECTED_FILE,
            "platform": "windows-x86",
            "arch": "x86",
            "open_binaries": 1,
        }

        errors = validate_status(
            status,
            expected_file=EXPECTED_FILE,
            expected_platform="windows-x86",
            expected_arch="x86",
        )

        self.assertEqual([], errors)

    def test_validate_status_reports_wrong_database_shape(self) -> None:
        status = {
            "loaded": False,
            "filename": "D:/Other/Recoil.bndb",
            "platform": "windows",
            "arch": "x86_64",
            "open_binaries": 0,
        }

        errors = validate_status(
            status,
            expected_file=EXPECTED_FILE,
            expected_platform="windows-x86",
            expected_arch="x86",
        )

        self.assertGreaterEqual(len(errors), 5)
        self.assertTrue(any("no loaded" in error for error in errors))
        self.assertTrue(any("expected" in error for error in errors))

    def test_validate_binaries_accepts_active_expected_database(self) -> None:
        binaries = {
            "binaries": [
                {"filename": EXPECTED_FILE, "active": True},
            ]
        }

        self.assertEqual([], validate_binaries(binaries, expected_file=EXPECTED_FILE))

    def test_validate_binaries_rejects_inactive_expected_database(self) -> None:
        binaries = {
            "binaries": [
                {"filename": EXPECTED_FILE, "active": False},
            ]
        }

        errors = validate_binaries(binaries, expected_file=EXPECTED_FILE)

        self.assertEqual(1, len(errors))
        self.assertIn("not the active", errors[0])

    def test_validate_probe_accepts_expected_function(self) -> None:
        function_info = {
            "function": {
                "address": "0x401000",
                "name": "entry",
            }
        }

        self.assertEqual([], validate_probe(function_info, probe_address="0x401000"))

    def test_validate_probe_rejects_missing_function(self) -> None:
        errors = validate_probe({"function": None}, probe_address="0x401000")

        self.assertEqual(1, len(errors))
        self.assertIn("did not return", errors[0])

    def test_validate_probe_rejects_wrong_address(self) -> None:
        function_info = {
            "function": {
                "address": "0x402000",
                "name": "other",
            }
        }

        errors = validate_probe(function_info, probe_address="0x401000")

        self.assertEqual(1, len(errors))
        self.assertIn("expected 0x401000", errors[0])


if __name__ == "__main__":
    unittest.main()
