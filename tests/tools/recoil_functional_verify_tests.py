from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))
SCRIPT = REPO_ROOT / "tools" / "recoil_functional_verify.py"

from recoil_functional_verify import find_target, load_manifest, load_manifests  # noqa: E402


MANIFEST_TEXT = """\
{
  "name": "sample_functional",
  "description": "Sample functional target.",
  "address": "0x401000",
  "source_from": "src/sample.cpp",
  "smoke_tests": [
    "sample_smoke"
  ],
  "vc6_attempt": "python tools/recoil_vc6_verify.py 0x401000",
  "known_limits": [
    "byte comparison still fails in this test fixture"
  ]
}
"""


class RecoilFunctionalVerifyTests(unittest.TestCase):
    def test_load_and_find_manifest_by_address_or_name(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            (manifest_dir / "sample_functional.json").write_text(MANIFEST_TEXT, encoding="utf-8")

            targets = load_manifests(manifest_dir)

        self.assertEqual(1, len(targets))
        self.assertEqual("sample_functional", find_target(targets, "sample_functional").name)
        self.assertEqual("sample_functional", find_target(targets, "0x401000").name)
        self.assertEqual(("sample_smoke",), targets[0].smoke_tests)

    def test_cli_dry_run_prints_smoke_command_and_marker_hint(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            (manifest_dir / "sample_functional.json").write_text(MANIFEST_TEXT, encoding="utf-8")
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "0x401000",
                    "--manifest-dir",
                    str(manifest_dir),
                    "--executable",
                    str(Path(tmp) / "missing_native_smoke.exe"),
                    "--dry-run",
                ],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("sample_smoke", result.stdout)
        self.assertIn("run_native_smokes.py", result.stdout)

    def test_manifest_requires_vc6_attempt_command_prefix(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            path.write_text(
                MANIFEST_TEXT.replace(
                    "python tools/recoil_vc6_verify.py 0x401000",
                    "python other_tool.py 0x401000",
                ),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "vc6_attempt"):
                load_manifest(path)

    def test_manifest_requires_binary_safety_state(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            path.write_text(MANIFEST_TEXT.replace('"known_limits": [\n    "byte comparison still fails in this test fixture"\n  ]', '"known_limits": []'), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "known_limits or binary_safe_evidence"):
                load_manifest(path)

    def test_manifest_allows_binary_safe_evidence_without_known_limits(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            path.write_text(
                MANIFEST_TEXT.replace(
                    '"known_limits": [\n    "byte comparison still fails in this test fixture"\n  ]',
                    '"known_limits": [],\n  "binary_safe_evidence": [\n    "COFF byte comparison passed"\n  ]',
                ),
                encoding="utf-8",
            )

            target = load_manifest(path)

        self.assertEqual((), target.known_limits)
        self.assertEqual(("COFF byte comparison passed",), target.binary_safe_evidence)


if __name__ == "__main__":
    unittest.main()
