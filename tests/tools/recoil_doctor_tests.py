from __future__ import annotations

import sys
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands.doctor import _steps  # noqa: E402


class DoctorTests(unittest.TestCase):
    def test_infrastructure_quick_steps_are_serial_core_checks(self) -> None:
        steps = _steps(
            infrastructure_only=True,
            quick=True,
            binja=False,
            binary="recoil",
        )
        labels = [label for label, _command in steps]
        self.assertEqual(len(labels), len(set(labels)))
        self.assertIn("serial pipeline contract", labels)
        self.assertIn("serial pipeline reachability", labels)
        self.assertNotIn("Binary Ninja preflight", labels)

    def test_binja_is_one_optional_terminal_step(self) -> None:
        steps = _steps(
            infrastructure_only=True,
            quick=True,
            binja=True,
            binary="recoil",
        )
        self.assertEqual("Binary Ninja preflight", steps[-1][0])
        self.assertIn("binja", steps[-1][1])


if __name__ == "__main__":
    unittest.main()
