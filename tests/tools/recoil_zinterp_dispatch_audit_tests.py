from __future__ import annotations

import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOL = REPO_ROOT / "tools" / "_recoil" / "commands" / "zinterp_dispatch_audit.py"


class RecoilZinterpDispatchAuditTests(unittest.TestCase):
    def run_tool(
        self, source: Path, hlil: Path, data: Path
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                str(TOOL),
                "--source",
                str(source),
                "--hlil",
                str(hlil),
                "--data",
                str(data),
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def write_fixture(
        self, root: Path, source_text: str, hlil_text: str, data_text: str
    ) -> tuple[Path, Path, Path]:
        source = root / "zinterp_parse.cpp"
        hlil = root / "text.hlil.txt"
        data = root / "data.linear.txt"
        source.write_text(source_text, encoding="utf-8")
        hlil.write_text(hlil_text, encoding="utf-8")
        data.write_text(data_text, encoding="utf-8")
        return source, hlil, data

    def test_accepts_matching_coverage_and_modes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, hlil, data = self.write_fixture(
                root,
                textwrap.dedent(
                    """\
                    int zInterp_Context::DispatchCoreCommand(char *commandToken)
                    {
                        if (CommandIs(this, "Alpha") != 0) return 1;
                        if (CommandIsExact(this, "Beta") != 0) return 1;
                        return 1;
                    }
                    // Reimplements 0x4c2030
                    """
                ),
                textwrap.dedent(
                    """\
                    0x4c20a0 int32_t __thiscall zInterp_Context::DispatchCoreCommand
                        if (strncmp(_Str1, "Alpha", 5) == 0)
                        if (zInterp_Context::CommandEquals(self, "Beta") != 0)
                    0x4c5480
                    """
                ),
                textwrap.dedent(
                    """\
                    0x4e4a30 char data_4e4a30[0x6] = "Alpha", 0
                    0x4e4a38 char data_4e4a38[0x5] = "Beta", 0
                    """
                ),
            )

            result = self.run_tool(source, hlil, data)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("missing in source: none", result.stdout)

    def test_rejects_missing_command_and_mode_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, hlil, data = self.write_fixture(
                root,
                textwrap.dedent(
                    """\
                    int zInterp_Context::DispatchCoreCommand(char *commandToken)
                    {
                        if (CommandIsExact(this, "Alpha") != 0) return 1;
                        return 1;
                    }
                    // Reimplements 0x4c2030
                    """
                ),
                textwrap.dedent(
                    """\
                    0x4c20a0 int32_t __thiscall zInterp_Context::DispatchCoreCommand
                        if (strncmp(_Str1, "Alpha", 5) == 0)
                        if (strncmp(_Str1, "Missing", 7) == 0)
                    0x4c5480
                    """
                ),
                textwrap.dedent(
                    """\
                    0x4e4a30 char data_4e4a30[0x6] = "Alpha", 0
                    0x4e4a38 char data_4e4a38[0x8] = "Missing", 0
                    """
                ),
            )

            result = self.run_tool(source, hlil, data)

        self.assertEqual(result.returncode, 1)
        self.assertIn("missing in source: 1", result.stdout)
        self.assertIn("Missing", result.stdout)
        self.assertIn("source exact but original prefix: 1", result.stdout)
        self.assertIn("Alpha", result.stdout)

    def test_rejects_prefix_order_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, hlil, data = self.write_fixture(
                root,
                textwrap.dedent(
                    """\
                    int zInterp_Context::DispatchCoreCommand(char *commandToken)
                    {
                        if (CommandIs(this, "Short") != 0) return 1;
                        if (CommandIs(this, "ShortLong") != 0) return 1;
                        return 1;
                    }
                    // Reimplements 0x4c2030
                    """
                ),
                textwrap.dedent(
                    """\
                    0x4c20a0 int32_t __thiscall zInterp_Context::DispatchCoreCommand
                        if (strncmp(_Str1, "ShortLong", 9) == 0)
                        if (strncmp(_Str1, "Short", 5) == 0)
                    0x4c5480
                    """
                ),
                textwrap.dedent(
                    """\
                    0x4e4a30 char data_4e4a30[0x6] = "Short", 0
                    0x4e4a38 char data_4e4a38[0xa] = "ShortLong", 0
                    """
                ),
            )

            result = self.run_tool(source, hlil, data)

        self.assertEqual(result.returncode, 1)
        self.assertIn("prefix order mismatches: 1", result.stdout)
        self.assertIn("Short before ShortLong", result.stdout)


if __name__ == "__main__":
    unittest.main()
