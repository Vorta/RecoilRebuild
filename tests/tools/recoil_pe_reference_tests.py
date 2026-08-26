from __future__ import annotations

import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
REFERENCE_EXE = REPO_ROOT / "support" / "Recoil.exe"
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.pe_reference import compare_dicts, parse_pe  # noqa: E402


class RecoilPeReferenceTests(unittest.TestCase):
    def test_support_recoil_exe_reference_facts(self) -> None:
        if not REFERENCE_EXE.exists():
            self.skipTest("support/Recoil.exe is a local-only reference input")

        info = parse_pe(REFERENCE_EXE)

        self.assertEqual(1254912, info.size)
        self.assertEqual("0x14c", info.machine)
        self.assertEqual("0x400000", info.image_base)
        self.assertEqual("0x4c6140", info.entry_point_va)
        self.assertEqual([".text", ".rdata", ".data", ".rsrc"], [section.name for section in info.sections])
        self.assertEqual("MFC42.DLL", info.imports[2].dll)
        self.assertEqual(193, info.imports[2].count)

    def test_compare_dicts_can_ignore_path(self) -> None:
        mismatches = compare_dicts(
            {"path": "support/Recoil.exe", "size": 1},
            {"path": "build/Recoil.exe", "size": 1},
            ignored_keys={"path"},
        )

        self.assertEqual([], mismatches)

    def test_candidate_comparison_can_treat_timestamp_as_diagnostic(self) -> None:
        mismatches = compare_dicts(
            {"path": "support/Recoil.exe", "timestamp": 1, "checksum": "0x0"},
            {"path": "build/Recoil.exe", "timestamp": 2, "checksum": "0x0"},
            ignored_keys={"path", "timestamp"},
        )

        self.assertEqual([], mismatches)


if __name__ == "__main__":
    unittest.main()
