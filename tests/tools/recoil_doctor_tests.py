from __future__ import annotations

import sys
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands.doctor import _steps  # noqa: E402


class DoctorTests(unittest.TestCase):
    def test_infrastructure_steps_are_serial_core_checks(self) -> None:
        steps = _steps(infrastructure_only=True)
        labels = [label for label, _command in steps]
        self.assertEqual(len(labels), len(set(labels)))
        self.assertIn("serial pipeline contract", labels)
        self.assertIn("serial pipeline reachability", labels)
        self.assertIn("live validation surface", labels)

    def test_full_steps_add_workspace_and_manifest_checks(self) -> None:
        steps = _steps(infrastructure_only=False)
        labels = [label for label, _command in steps]
        self.assertEqual("workspace hygiene", labels[-2])
        self.assertEqual("VC5 manifest source policy", labels[-1])


if __name__ == "__main__":
    unittest.main()
