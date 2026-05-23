from __future__ import annotations

import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_doctor import build_parser, build_steps, format_command, quick_steps  # noqa: E402


class RecoilDoctorTests(unittest.TestCase):
    def test_quick_steps_include_core_process_checks(self) -> None:
        labels = [step.label for step in quick_steps()]

        self.assertIn("VC manifest source policy", labels)
        self.assertIn("compiler/linker provenance", labels)
        self.assertIn("workspace hygiene", labels)
        self.assertIn("raw image address guard", labels)
        self.assertIn("raw assembly guard", labels)
        self.assertIn("reinterpret_cast guard", labels)
        self.assertIn("reference executable manifest", labels)
        self.assertIn("source file map freshness", labels)
        self.assertIn("functional manifest load", labels)
        self.assertIn("VC manifest load", labels)
        self.assertIn("Python tool tests", labels)
        self.assertNotIn("Binary Ninja bridge", labels)

    def test_binja_step_is_opt_in(self) -> None:
        args = build_parser().parse_args(["--binja"])

        steps = build_steps(args)
        commands = [step.command for step in steps if step.label == "Binary Ninja bridge"]

        self.assertEqual(1, len(commands))
        self.assertTrue(str(commands[0][1]).endswith("recoil_binja_preflight.py"))
        self.assertIn("--strict", commands[0])

    def test_active_steps_default_to_compile_only_verification(self) -> None:
        args = build_parser().parse_args(["--active", "0x415220"])

        steps = build_steps(args)
        verify_commands = [step.command for step in steps if step.label.startswith("active VC compile")]

        self.assertEqual(1, len(verify_commands))
        self.assertIn("--skip-bn-compare", verify_commands[0])

    def test_active_steps_can_enable_bn_compare(self) -> None:
        args = build_parser().parse_args(["--active", "0x415220", "--bn-compare"])

        steps = build_steps(args)
        verify_commands = [step.command for step in steps if step.label.startswith("active VC byte verify")]

        self.assertEqual(1, len(verify_commands))
        self.assertNotIn("--skip-bn-compare", verify_commands[0])

    def test_active_provider_steps_skip_vc_compile(self) -> None:
        args = build_parser().parse_args(["--active", "0x4c5ba0"])

        steps = build_steps(args)
        labels = [step.label for step in steps]

        self.assertIn("active status 0x4c5ba0", labels)
        self.assertFalse(any(label.startswith("active VC") for label in labels))

    def test_format_command_quotes_paths_with_spaces(self) -> None:
        formatted = format_command(("python", "D:/Recoil Project/tool.py", "--flag"))

        self.assertEqual('python "D:/Recoil Project/tool.py" --flag', formatted)


if __name__ == "__main__":
    unittest.main()
