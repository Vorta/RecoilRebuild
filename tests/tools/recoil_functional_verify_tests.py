from __future__ import annotations

import contextlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "functional_verify.py"

from _recoil.commands.functional_verify import (  # noqa: E402
    CANONICAL_NATIVE_PRESET,
    DEFAULT_SMOKE_CPP,
    canonical_build_commands,
    find_target,
    load_manifest,
    load_manifests,
    resolve_executable,
    run_target,
)
from _recoil.commands.functional_verify import main as functional_main  # noqa: E402
from _recoil.lib.verification_targets import functional_target_registration  # noqa: E402


MANIFEST_TEXT = """\
{
  "name": "sample_functional",
  "description": "Sample functional target.",
  "address": "0x401000",
  "source_from": "src/sample.cpp",
  "smoke_tests": [
    "sample_smoke"
  ],
  "vc5_attempt": "python tools/recoil.py verify vc5 0x401000",
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
        self.assertEqual(("0x401000",), targets[0].covered_addresses)
        self.assertEqual("recoil", targets[0].target_binary)

    def test_manifest_infers_messages_target_binary_from_source_and_address(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            data = json.loads(MANIFEST_TEXT)
            data["address"] = "0x10001010"
            data["source_from"] = "src/Messages/messages.c"
            path.write_text(json.dumps(data), encoding="utf-8")

            target = load_manifest(path)

        self.assertEqual("messages", target.target_binary)

    def test_manifest_rejects_cross_binary_covered_addresses(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            data = json.loads(MANIFEST_TEXT)
            data["covered_addresses"] = ["0x10001010"]
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "cross-binary addresses"):
                load_manifest(path)

    def test_manifest_rejects_explicit_binary_conflicting_with_address(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            data = json.loads(MANIFEST_TEXT)
            data["target_binary"] = "messages"
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "conflicts with inferred binary"):
                load_manifest(path)

    def test_messages_manifest_registration_uses_messages_stable_id(self) -> None:
        path = REPO_ROOT / "tools" / "functional_verify_targets" / "zloc_get_id_messages_lookup.json"

        target_id, record = functional_target_registration(path)

        self.assertEqual("messages:functional-target:zloc_get_id_messages_lookup", target_id)
        self.assertEqual("messages", record["binary"])
        self.assertEqual("messages", record["registration"]["binary"])
        self.assertEqual(["0x10001010"], record["registered_addresses"])

    def test_messages_functional_dry_run_is_direct_and_creates_no_validation_artifact(self) -> None:
        path = REPO_ROOT / "tools" / "functional_verify_targets" / "zloc_get_id_messages_lookup.json"
        target = load_manifest(path)
        with tempfile.TemporaryDirectory() as tmp:
            with contextlib.redirect_stdout(io.StringIO()):
                result = run_target(
                    target,
                    executable=Path(tmp) / "missing-smoke.exe",
                    smoke_cpp=DEFAULT_SMOKE_CPP,
                    dry_run=True,
                )

        self.assertEqual(0, result)
        self.assertEqual("messages", target.target_binary)

    def test_load_and_find_manifest_by_covered_address(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            (manifest_dir / "sample_functional.json").write_text(
                MANIFEST_TEXT.replace(
                    '"address": "0x401000",',
                    '"address": "0x401000",\n  "covered_addresses": [\n    "0x401020"\n  ],',
                ),
                encoding="utf-8",
            )

            targets = load_manifests(manifest_dir)

        self.assertEqual(("0x401000", "0x401020"), targets[0].covered_addresses)
        self.assertEqual("sample_functional", find_target(targets, "0x401020").name)

    def test_cli_dry_run_prints_smoke_command_and_owner_follow_up(self) -> None:
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
        self.assertIn("Diagnostic-only executable override", result.stdout)
        self.assertIn("dry_run: no canonical build or functional smoke executed", result.stdout)
        self.assertNotIn("parent_follow_up: review unified progress owner gates", result.stdout)
        self.assertNotIn("set 0x401000 functional", result.stdout)

    def test_cli_dry_run_uses_owner_follow_up_for_covered_address(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            (manifest_dir / "sample_functional.json").write_text(
                MANIFEST_TEXT.replace(
                    '"address": "0x401000",',
                    '"address": "0x401000",\n  "covered_addresses": [\n    "0x401020"\n  ],',
                ),
                encoding="utf-8",
            )
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "0x401020",
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
        self.assertIn("Covered addresses: 0x401000, 0x401020", result.stdout)
        self.assertIn("Diagnostic-only executable override", result.stdout)
        self.assertIn("dry_run: no canonical build or functional smoke executed", result.stdout)
        self.assertNotIn("parent_follow_up: review unified progress owner gates", result.stdout)
        self.assertNotIn("set 0x401020 functional", result.stdout)

    def test_cli_batch_dry_run_prints_per_target_summary(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            (manifest_dir / "sample_functional.json").write_text(MANIFEST_TEXT, encoding="utf-8")
            (manifest_dir / "second_functional.json").write_text(
                MANIFEST_TEXT.replace("sample_functional", "second_functional")
                .replace("0x401000", "0x401020")
                .replace("sample_smoke", "second_smoke"),
                encoding="utf-8",
            )
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "batch",
                    "--dry-run",
                    "--manifest-dir",
                    str(manifest_dir),
                    "0x401000",
                    "second_functional",
                ],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Functional batch summary:", result.stdout)
        self.assertIn("[DRY-RUN] 0x401000 -> sample_functional", result.stdout)
        self.assertIn("[DRY-RUN] second_functional -> second_functional", result.stdout)
        self.assertIn("sample_smoke", result.stdout)
        self.assertIn("second_smoke", result.stdout)

    def test_cli_targets_json_runs_batch_mode(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            (manifest_dir / "sample_functional.json").write_text(MANIFEST_TEXT, encoding="utf-8")
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--dry-run",
                    "--manifest-dir",
                    str(manifest_dir),
                    "--targets-json",
                    '["0x401000"]',
                ],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Functional batch summary:", result.stdout)
        self.assertIn("[DRY-RUN] 0x401000 -> sample_functional", result.stdout)

    def test_batch_returns_nonzero_when_any_target_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            (manifest_dir / "sample_functional.json").write_text(MANIFEST_TEXT, encoding="utf-8")
            executable = Path(tmp) / "recoil_native_smoke.exe"
            executable.write_text("", encoding="utf-8")
            stdout = io.StringIO()
            stderr = io.StringIO()
            failed = mock.Mock(returncode=3, stdout="smoke failed\n")
            with mock.patch("_recoil.commands.functional_verify.subprocess.run", return_value=failed):
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = functional_main(
                        [
                            "batch",
                            "0x401000",
                            "--manifest-dir",
                            str(manifest_dir),
                            "--executable",
                            str(executable),
                        ]
                    )

        self.assertEqual(rc, 1)
        self.assertEqual("", stderr.getvalue())
        self.assertIn("[FAIL] sample_smoke: exit 3", stdout.getvalue())
        self.assertIn("[FAIL] 0x401000 -> sample_functional", stdout.getvalue())

    def test_existing_executable_success_is_explicitly_diagnostic_non_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            (manifest_dir / "sample_functional.json").write_text(MANIFEST_TEXT, encoding="utf-8")
            executable = manifest_dir / "existing_native_smoke.exe"
            executable.write_text("existing", encoding="utf-8")
            stdout = io.StringIO()
            with mock.patch(
                "_recoil.commands.functional_verify.subprocess.run",
                return_value=mock.Mock(returncode=0, stdout=""),
            ) as run:
                with contextlib.redirect_stdout(stdout):
                    result = functional_main(
                        [
                            "sample_functional",
                            "--manifest-dir",
                            str(manifest_dir),
                            "--executable",
                            str(executable),
                        ]
                    )

        self.assertEqual(0, result)
        run.assert_called_once()
        output = stdout.getvalue()
        self.assertIn("Diagnostic-only executable override", output)
        self.assertIn("Diagnostic functional run passed", output)
        self.assertIn("diagnostic_only:", output)
        self.assertNotIn("Functional verification passed.", output)
        self.assertNotIn("parent_follow_up:", output)

    def test_manifest_requires_vc5_attempt_command_prefix(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            path.write_text(
                MANIFEST_TEXT.replace(
                    "python tools/recoil.py verify vc5 0x401000",
                    "python other_tool.py 0x401000",
                ),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "vc5_attempt"):
                load_manifest(path)

    def test_manifest_accepts_legacy_vc5_attempt_command_prefix(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            path.write_text(
                MANIFEST_TEXT.replace(
                    "python tools/recoil.py verify vc5 0x401000",
                    "python tools/recoil.py verify vc5 0x401000",
                ),
                encoding="utf-8",
            )

            target = load_manifest(path)

        self.assertTrue(target.vc5_attempt.startswith("python tools/recoil.py verify vc5"))

    def test_manifest_name_must_match_filename_target_id(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            path.write_text(
                MANIFEST_TEXT.replace("sample_functional", "Sample::Functional"),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "filename stem"):
                load_manifest(path)

    def test_manifest_target_id_must_be_lowercase_snake_case(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "bad-target.json"
            path.write_text(
                MANIFEST_TEXT.replace("sample_functional", "bad-target"),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "lowercase snake_case"):
                load_manifest(path)

    def test_manifest_requires_tier_s_state(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            path.write_text(MANIFEST_TEXT.replace('"known_limits": [\n    "byte comparison still fails in this test fixture"\n  ]', '"known_limits": []'), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "known_limits or tier_s_evidence"):
                load_manifest(path)

    def test_manifest_allows_tier_s_evidence_without_known_limits(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            path.write_text(
                MANIFEST_TEXT.replace(
                    '"known_limits": [\n    "byte comparison still fails in this test fixture"\n  ]',
                    '"known_limits": [],\n  "tier_s_evidence": [\n    "COFF byte comparison passed"\n  ]',
                ),
                encoding="utf-8",
            )

            target = load_manifest(path)

        self.assertEqual((), target.known_limits)
        self.assertEqual(("COFF byte comparison passed",), target.tier_s_evidence)

    def test_manifest_rejects_retired_tier_s_evidence_key(self) -> None:
        retired_key = "binary_" "safe_evidence"
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample_functional.json"
            path.write_text(
                MANIFEST_TEXT.replace(
                    '"known_limits": [\n    "byte comparison still fails in this test fixture"\n  ]',
                    f'"known_limits": [],\n  "{retired_key}": [\n    "COFF byte comparison passed"\n  ]',
                ),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "retired tier-S evidence key"):
                load_manifest(path)

    def test_normal_resolution_builds_only_the_canonical_current_source_x86_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            canonical_smoke = root / "build" / "ninja-x86-debug" / "tests" / "native" / "recoil_native_smoke.exe"
            stale_smoke = root / "build" / "vs-x86" / "tests" / "native" / "Debug" / "recoil_native_smoke.exe"
            stale_smoke.parent.mkdir(parents=True)
            stale_smoke.write_text("stale", encoding="utf-8")

            call_count = 0

            def complete_build(*args, **kwargs):
                nonlocal call_count
                call_count += 1
                if call_count == 2:
                    canonical_smoke.parent.mkdir(parents=True)
                    canonical_smoke.write_text("fresh", encoding="utf-8")
                return mock.Mock(returncode=0, stdout="")

            stdout = io.StringIO()
            with mock.patch(
                "_recoil.commands.functional_verify.CANONICAL_NATIVE_SMOKE_EXE",
                canonical_smoke,
            ), mock.patch(
                "_recoil.commands.functional_verify.subprocess.run",
                side_effect=complete_build,
            ) as run:
                with contextlib.redirect_stdout(stdout):
                    executable = resolve_executable(None, dry_run=False)

        self.assertEqual(canonical_smoke, executable)
        self.assertNotEqual(stale_smoke, executable)
        self.assertEqual(2, run.call_count)
        self.assertEqual(
            [command for _, command in canonical_build_commands()],
            [call.args[0] for call in run.call_args_list],
        )
        self.assertIn("[PASS] Canonical current-source x86 recoil_native_smoke build", stdout.getvalue())

    def test_normal_resolution_rejects_successful_build_without_canonical_output(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            missing_smoke = Path(tmp) / "build" / "ninja-x86-debug" / "tests" / "native" / "recoil_native_smoke.exe"
            with mock.patch(
                "_recoil.commands.functional_verify.CANONICAL_NATIVE_SMOKE_EXE",
                missing_smoke,
            ), mock.patch(
                "_recoil.commands.functional_verify.subprocess.run",
                side_effect=(
                    mock.Mock(returncode=0, stdout=""),
                    mock.Mock(returncode=0, stdout=""),
                ),
            ):
                with contextlib.redirect_stdout(io.StringIO()):
                    with self.assertRaisesRegex(RuntimeError, "did not produce"):
                        resolve_executable(None, dry_run=False)

    def test_canonical_build_commands_use_owned_x86_entrypoints_and_exact_target(self) -> None:
        commands = canonical_build_commands()
        configure = commands[0][1]
        target_build = commands[1][1]

        self.assertEqual("configure", commands[0][0])
        self.assertEqual("powershell", configure[0])
        self.assertIn("-File", configure)
        self.assertEqual("-ConfigureOnly", configure[-1])
        self.assertEqual("target build", commands[1][0])
        self.assertEqual(sys.executable, target_build[0])
        self.assertEqual(["build", "msvc-x86", "--"], target_build[2:5])
        self.assertEqual(
            [
                "cmake",
                "--build",
                "--preset",
                CANONICAL_NATIVE_PRESET,
                "--target",
                "recoil_native_smoke",
            ],
            target_build[5:],
        )

    def test_normal_run_never_starts_smoke_after_either_build_step_fails(self) -> None:
        for failing_step in (0, 1):
            with self.subTest(failing_step=failing_step), tempfile.TemporaryDirectory() as tmp:
                manifest_dir = Path(tmp)
                (manifest_dir / "sample_functional.json").write_text(MANIFEST_TEXT, encoding="utf-8")
                calls: list[list[str]] = []

                def fail_selected_step(command, **kwargs):
                    calls.append(command)
                    return mock.Mock(
                        returncode=7 if len(calls) - 1 == failing_step else 0,
                        stdout="selected build failure\n" if len(calls) - 1 == failing_step else "",
                    )

                with mock.patch(
                    "_recoil.commands.functional_verify.subprocess.run",
                    side_effect=fail_selected_step,
                ):
                    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                        result = functional_main(
                            [
                                "sample_functional",
                                "--manifest-dir",
                                str(manifest_dir),
                            ]
                        )

                self.assertEqual(1, result)
                self.assertEqual(failing_step + 1, len(calls))
                self.assertTrue(
                    all("run_native_smokes.py" not in " ".join(command) for command in calls)
                )

    def test_normal_batch_builds_canonical_target_once_before_all_smokes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            (manifest_dir / "sample_functional.json").write_text(MANIFEST_TEXT, encoding="utf-8")
            (manifest_dir / "second_functional.json").write_text(
                MANIFEST_TEXT.replace("sample_functional", "second_functional")
                .replace("0x401000", "0x401020")
                .replace("sample_smoke", "second_smoke"),
                encoding="utf-8",
            )
            canonical_smoke = root / "build" / "ninja-x86-debug" / "tests" / "native" / "recoil_native_smoke.exe"
            calls: list[list[str]] = []

            def build_then_smoke(command, **kwargs):
                calls.append(command)
                if len(calls) == 2:
                    canonical_smoke.parent.mkdir(parents=True)
                    canonical_smoke.write_text("fresh", encoding="utf-8")
                return mock.Mock(returncode=0, stdout="")

            with mock.patch(
                "_recoil.commands.functional_verify.CANONICAL_NATIVE_SMOKE_EXE",
                canonical_smoke,
            ), mock.patch(
                "_recoil.commands.functional_verify.subprocess.run",
                side_effect=build_then_smoke,
            ):
                with contextlib.redirect_stdout(io.StringIO()):
                    result = functional_main(
                        [
                            "batch",
                            "sample_functional",
                            "second_functional",
                            "--manifest-dir",
                            str(manifest_dir),
                        ]
                    )

        self.assertEqual(0, result)
        self.assertEqual(4, len(calls))
        self.assertIn("-ConfigureOnly", calls[0])
        self.assertEqual(["build", "msvc-x86", "--"], calls[1][2:5])
        self.assertIn("--target", calls[1])
        self.assertIn("recoil_native_smoke", calls[1])
        self.assertNotIn("run_native_smokes.py", " ".join(calls[0]))
        self.assertIn("run_native_smokes.py", " ".join(calls[2]))
        self.assertIn("run_native_smokes.py", " ".join(calls[3]))


if __name__ == "__main__":
    unittest.main()
