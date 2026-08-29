from __future__ import annotations

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))


def canonical_test_root() -> Path:
    return Path(os.environ.get("RECOIL_CANONICAL_ROOT", str(REPO_ROOT)))

from _recoil.commands.doctor import (  # noqa: E402
    DOCTOR_CATEGORIES,
    INFRASTRUCTURE_ONLY_NOTICE,
    DoctorResult,
    DoctorStep,
    _doctor_test_machine_local_bindings,
    active_steps,
    build_parser,
    build_steps,
    default_jobs,
    default_test_jobs,
    format_command,
    format_step_command,
    main,
    print_advisory_result,
    print_result,
    quick_steps,
    run_python_tool_tests,
    run_step_result,
    run_tool_test_file,
    tool_test_files,
)


class Cp1252StrictStream:
    encoding = "cp1252"

    def __init__(self) -> None:
        self._chunks: list[str] = []

    def write(self, text: str) -> int:
        text.encode(self.encoding, errors="strict")
        self._chunks.append(text)
        return len(text)

    def flush(self) -> None:
        pass

    def getvalue(self) -> str:
        return "".join(self._chunks)


class RecoilDoctorTests(unittest.TestCase):
    def test_doctor_steps_use_only_the_two_declared_categories(self) -> None:
        quick = quick_steps("recoil")
        reconstruction_labels = {
            "VC manifest source policy",
            "production source fragments",
            "raw image address guard",
            "raw assembly guard",
            "source-level goto guard",
            "modern C++ construct guard",
            "source-shape scaffold guard",
            "raw offset tier guard",
            "original source symbol guard",
        }

        self.assertEqual({"infrastructure", "reconstruction"}, set(DOCTOR_CATEGORIES))
        self.assertEqual(
            reconstruction_labels,
            {step.label for step in quick if step.category == "reconstruction"},
        )
        self.assertTrue(all(step.category in DOCTOR_CATEGORIES for step in quick))

        expanded = build_steps(
            build_parser().parse_args(["--binja", "--native-x86", "--active", "0x415220"]),
            execution_root=REPO_ROOT,
            canonical_root=canonical_test_root(),
        )
        categories = {step.label: step.category for step in expanded}
        self.assertEqual("infrastructure", categories["Binary Ninja bridge"])
        self.assertEqual("infrastructure", categories["native x86 environment"])
        self.assertEqual("reconstruction", categories["active status 0x415220"])
        self.assertEqual("reconstruction", categories["active VC compile 0x415220"])

    def test_doctor_step_rejects_unknown_category(self) -> None:
        with self.assertRaises(ValueError):
            DoctorStep("invalid", "unknown")  # type: ignore[arg-type]

    def test_quick_steps_include_core_process_checks(self) -> None:
        labels = [step.label for step in quick_steps("recoil")]

        self.assertIn("VC manifest source policy", labels)
        self.assertIn("production source fragments", labels)
        self.assertIn("compiler/linker provenance", labels)
        self.assertIn("workspace hygiene", labels)
        self.assertIn("live workflow contracts", labels)
        self.assertIn("pipeline producer reachability", labels)
        self.assertIn("raw image address guard", labels)
        self.assertIn("raw assembly guard", labels)
        self.assertIn("source-level goto guard", labels)
        self.assertIn("modern C++ construct guard", labels)
        self.assertIn("source-shape scaffold guard", labels)
        self.assertIn("raw offset tier guard", labels)
        self.assertIn("original source symbol guard", labels)
        self.assertIn("agent surface alignment", labels)
        self.assertIn("agent tooling/process issue ledger", labels)
        self.assertIn("unified reconstruction progress", labels)
        self.assertIn("reference Recoil.exe manifest", labels)
        self.assertNotIn("source file map freshness", labels)
        self.assertIn("current metadata and README freshness", labels)
        self.assertIn("functional manifest load", labels)
        self.assertIn("VC manifest load", labels)
        self.assertIn("Python tool tests", labels)
        self.assertNotIn("Binary Ninja bridge", labels)

    def test_python_tool_tests_are_a_special_parallel_step(self) -> None:
        steps = [step for step in quick_steps("recoil") if step.label == "Python tool tests"]

        self.assertEqual(1, len(steps))
        self.assertEqual("python_tool_tests", steps[0].kind)
        self.assertEqual((), steps[0].command)
        self.assertEqual(
            "parallel per-file unittest discovery for tests/tools/*_tests.py",
            format_step_command(steps[0]),
        )

    def test_unified_progress_audit_is_required(self) -> None:
        steps = [step for step in quick_steps("recoil") if step.label == "unified reconstruction progress"]

        self.assertEqual(1, len(steps))
        self.assertTrue(steps[0].required)

    def test_messages_quick_steps_use_messages_source_root_and_reference(self) -> None:
        steps = quick_steps("messages")
        commands = [step.command for step in steps if step.command]
        rendered = "\n".join(format_command(command) for command in commands)
        progress = REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"

        self.assertIn("reference messages.dll manifest", [step.label for step in steps])
        self.assertIn("guard raw-image --root src/Messages", rendered)
        self.assertIn(
            "guard source-goto --root src/Messages --summary --strict-zero",
            rendered,
        )
        self.assertIn(
            f'guard raw-offset --root src/Messages --progress "{progress}" --binary messages',
            rendered,
        )
        self.assertIn(
            f'guard original-symbol --root src/Messages --progress "{progress}"',
            rendered,
        )
        self.assertIn(
            "-m _recoil.commands.pe_reference --reference support/messages.dll",
            rendered,
        )

    def test_quick_steps_keep_tracked_commands_in_execution_worktree(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            execution = root / "linked"
            canonical = root / "canonical"
            steps = quick_steps(
                "recoil",
                execution_root=execution,
                canonical_root=canonical,
            )
        commands = [step.command for step in steps if step.command]
        self.assertTrue(commands)
        ordinary_commands = [
            step.command for step in steps
            if step.command and step.label not in {
                "reference Recoil.exe manifest",
                "VC manifest load",
            }
        ]
        self.assertTrue(all(
            command[1] == str(execution / "tools" / "recoil.py")
            for command in ordinary_commands
        ))
        pipeline = next(step.command for step in steps if step.label == "pipeline producer reachability")
        self.assertIn(str(canonical / "support" / "Recoil.exe"), pipeline)
        self.assertIn(str(execution / "tools" / "vc5_verify_targets"), pipeline)
        reference = next(step.command for step in steps if step.label == "reference Recoil.exe manifest")
        reference_step = next(step for step in steps if step.label == "reference Recoil.exe manifest")
        self.assertIn(str(execution / ".agent" / "REFERENCE_EXECUTABLE.json"), reference)
        self.assertEqual("support/Recoil.exe", reference[reference.index("--reference") + 1])
        self.assertEqual(canonical, reference_step.working_directory)
        self.assertEqual(execution / "tools", reference_step.python_path_root)
        vc_manifest_step = next(
            step for step in steps if step.label == "VC manifest load"
        )
        self.assertEqual(
            str(execution / "tools" / "_recoil" / "commands" / "doctor.py"),
            vc_manifest_step.command[1],
        )
        self.assertEqual("--_validate-vc-manifests", vc_manifest_step.command[2])
        self.assertEqual(canonical, vc_manifest_step.working_directory)
        self.assertIsNone(vc_manifest_step.python_path_root)

    def test_reference_manifest_check_uses_logical_path_from_canonical_cwd(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            canonical = root / "canonical"
            linked = root / "linked"
            for execution in (canonical, linked):
                with self.subTest(execution=execution):
                    step = next(
                        step for step in quick_steps(
                            "recoil",
                            execution_root=execution,
                            canonical_root=canonical,
                        )
                        if step.label == "reference Recoil.exe manifest"
                    )
                    reference_index = step.command.index("--reference")
                    manifest_index = step.command.index("--manifest")
                    self.assertEqual(
                        ("-m", "_recoil.commands.pe_reference"),
                        step.command[1:3],
                    )
                    self.assertEqual("support/Recoil.exe", step.command[reference_index + 1])
                    self.assertEqual(
                        str(execution / ".agent" / "REFERENCE_EXECUTABLE.json"),
                        step.command[manifest_index + 1],
                    )
                    self.assertEqual(canonical, step.working_directory)
                    self.assertEqual(execution / "tools", step.python_path_root)
                    with mock.patch(
                        "_recoil.commands.doctor.run_subprocess",
                        return_value=(0, "manifest verified"),
                    ) as run:
                        result = run_step_result(
                            step,
                            verbose=False,
                            timings=False,
                            test_jobs=1,
                            execution_root=execution,
                        )
                    self.assertEqual(0, result.returncode)
                    run.assert_called_once()
                    call = run.call_args
                    self.assertEqual((step.command,), call.args)
                    self.assertEqual(execution, call.kwargs["execution_root"])
                    self.assertEqual(canonical, call.kwargs["working_directory"])
                    self.assertEqual(
                        str(execution / "tools"),
                        call.kwargs["env"]["PYTHONPATH"].split(os.pathsep)[0],
                    )

    def test_doctor_root_environment_does_not_contaminate_foreign_git_fixtures(self) -> None:
        identities = (
            "test_canonical_file_and_transient_build_replacement_are_rejected",
            "test_canonical_root_executing_root_and_validated_environment",
            "test_canonical_root_unique_discovery_routes_from_linked_execution",
            "test_live_sqlite_identity_observation_allows_concurrent_reader",
            "test_live_sqlite_observation_does_not_use_share_denying_file_handle",
            "test_missing_directory_ambiguous_and_identity_failures_are_closed",
        )
        selectors = [
            "tests.tools.recoil_worktree_control_tests.WorktreeControlTests." + identity
            for identity in identities
        ]
        with tempfile.TemporaryDirectory() as temporary:
            environment = dict(os.environ)
            environment.update({
                "RECOIL_CANONICAL_ROOT": str(REPO_ROOT),
                "RECOIL_EXECUTION_WORKTREE_ROOT": str(REPO_ROOT),
                "RECOIL_EXTERNAL_BUILD_ROOT": temporary,
            })
            completed = subprocess.run(
                [sys.executable, "-B", "-m", "unittest", *selectors],
                cwd=REPO_ROOT,
                env=environment,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                encoding="utf-8",
                errors="replace",
            )
        self.assertEqual(0, completed.returncode, completed.stdout)
        self.assertIn("Ran 6 tests", completed.stdout)

    def test_active_steps_use_linked_tools_and_canonical_progress(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            execution = root / "linked"
            canonical = root / "canonical"
            progress = canonical / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
            with mock.patch(
                "_recoil.commands.doctor.OwnerEntryIndex.load",
                return_value=SimpleNamespace(entries={}),
            ) as load:
                steps = active_steps(
                    "0x401000",
                    binary="recoil",
                    bn_compare=False,
                    execution_root=execution,
                    canonical_root=canonical,
                )
        load.assert_called_once_with(progress, binary="recoil")
        self.assertEqual(2, len(steps))
        self.assertTrue(all(
            step.command[1] == str(execution / "tools" / "recoil.py")
            for step in steps
        ))
        self.assertTrue(all(str(progress) in step.command for step in steps))

    def test_tool_test_discovery_uses_the_executing_worktree(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tests = root / "tests" / "tools"
            tests.mkdir(parents=True)
            sentinel = tests / "recoil_linked_only_tests.py"
            sentinel.write_text("# linked-only tracked sentinel\n", encoding="utf-8")
            names = [path.name for path in tool_test_files(execution_root=root)]
        self.assertEqual([sentinel.name], names)

    def test_linked_only_test_sentinel_is_executed_from_selected_root(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tests = root / "tests" / "tools"
            tests.mkdir(parents=True)
            (tests / "recoil_linked_only_tests.py").write_text(
                "import unittest\n"
                "class LinkedOnlySentinel(unittest.TestCase):\n"
                "    def test_linked_only_candidate(self):\n"
                "        print('LINKED_ONLY_TEST_SENTINEL')\n"
                "        self.assertTrue(True)\n",
                encoding="utf-8",
            )
            returncode, output = run_python_tool_tests(
                test_jobs=1,
                verbose=True,
                timings=False,
                execution_root=root,
            )
        self.assertEqual(0, returncode, output)
        self.assertIn("LINKED_ONLY_TEST_SENTINEL", output)

    def test_canonical_test_child_discovers_exact_linked_test_file(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tests = root / "tests" / "tools"
            tests.mkdir(parents=True)
            sentinel = tests / "recoil_linked_only_tests.py"
            sentinel.write_text("# linked-only tracked sentinel\n", encoding="utf-8")
            canonical = root / "canonical"
            environment = {
                "RECOIL_CANONICAL_ROOT": str(canonical),
                "RECOIL_EXECUTION_WORKTREE_ROOT": str(root),
                "RECOIL_EXTERNAL_BUILD_ROOT": str(root / "build"),
                "PYTHONPATH": "inherited-python-path",
            }
            with mock.patch(
                "_recoil.commands.doctor.run_subprocess",
                return_value=(0, "linked child passed"),
            ) as run:
                name, returncode, output, _duration = run_tool_test_file(
                    sentinel,
                    env=environment,
                    execution_root=root,
                )
        self.assertEqual(sentinel.name, name)
        self.assertEqual(0, returncode)
        self.assertEqual("linked child passed", output)
        command = run.call_args.args[0]
        self.assertEqual(
            str(root / "tools" / "_recoil" / "commands" / "doctor.py"),
            command[1],
        )
        self.assertEqual("--_run-tool-test-file", command[2])
        self.assertEqual(str(sentinel.resolve()), command[3])
        self.assertEqual(root, run.call_args.kwargs["working_directory"])
        child_env = run.call_args.kwargs["env"]
        self.assertEqual("inherited-python-path", child_env["PYTHONPATH"])

    def test_linked_test_child_routes_only_exact_machine_local_defaults(self) -> None:
        from _recoil.commands import call_contract_verify, progress_cli
        from _recoil.lib.progress_sqlite import ProgressSQLiteStore

        original_progress_root = progress_cli.REPO_ROOT
        canonical_root = Path(
            os.environ.get("RECOIL_CANONICAL_ROOT", str(REPO_ROOT))
        )
        with tempfile.TemporaryDirectory() as temporary:
            execution = Path(temporary) / "linked"
            linked_build = execution / "build" / "fixture.txt"
            linked_build.parent.mkdir(parents=True)
            linked_build.write_text("linked fixture\n", encoding="utf-8")
            linked_retail = execution / "support" / "Recoil.exe"
            linked_progress = (
                execution / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
            )
            canonical_retail = canonical_root / "support" / "Recoil.exe"
            canonical_progress = (
                canonical_root / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
            )
            canonical_issue = canonical_root / ".agent" / "WORKSPACE_ISSUES.sqlite3"

            self.assertFalse(linked_retail.exists())
            self.assertFalse(linked_progress.exists())
            with _doctor_test_machine_local_bindings(
                execution_root=execution,
                canonical_root=canonical_root,
            ):
                self.assertTrue(linked_retail.is_file())
                self.assertEqual(
                    canonical_retail.stat().st_size,
                    linked_retail.stat().st_size,
                )
                with call_contract_verify.StableReadHandle(linked_retail) as handle:
                    self.assertEqual(canonical_retail.resolve(), handle.path)
                self.assertEqual(
                    canonical_retail.read_bytes()[:2],
                    linked_retail.read_bytes()[:2],
                )
                store = ProgressSQLiteStore(linked_progress, read_only=True)
                self.assertEqual(canonical_progress, store.path)
                self.assertEqual(canonical_issue, progress_cli.DEFAULT_ISSUE_LEDGER)
                self.assertEqual(
                    canonical_root.parent
                    / "Compiler"
                    / "VC5SP3"
                    / "vc5sp3-env.cmd",
                    call_contract_verify.DEFAULT_VC5_ENV,
                )
                self.assertEqual(
                    canonical_root.parent
                    / "Compiler"
                    / "VC5SP3"
                    / "vc5sp3-env.cmd",
                    call_contract_verify.player_call_contract_profile_matrix
                    .__kwdefaults__["vc5_env"],
                )
                self.assertEqual(
                    canonical_root.parent
                    / "Compiler"
                    / "VC5SP3"
                    / "VC"
                    / "BIN"
                    / "C2.EXE",
                    call_contract_verify.ZEFFECT_PROFILE_MATRIX_BACKENDS[0][
                        "c2_path"
                    ],
                )
                self.assertEqual(
                    canonical_root.parent
                    / "Visual C++ 5.0"
                    / "DEVSTUDIO"
                    / "VC"
                    / "BIN"
                    / "C2.EXE",
                    call_contract_verify.ZEFFECT_PROFILE_MATRIX_BACKENDS[1][
                        "c2_path"
                    ],
                )
                self.assertEqual(original_progress_root, progress_cli.REPO_ROOT)
                self.assertEqual("linked fixture\n", linked_build.read_text(encoding="utf-8"))

            self.assertFalse(linked_retail.exists())
            self.assertFalse(linked_progress.exists())
            self.assertEqual(original_progress_root, progress_cli.REPO_ROOT)

    def test_main_propagates_bounded_roots_to_linked_children(self) -> None:
        canonical = SimpleNamespace(
            canonical_control_root=REPO_ROOT,
            executing_worktree_root=REPO_ROOT,
        )
        topology = SimpleNamespace(build_parent=REPO_ROOT.parent / "RecoilRebuild.builds")
        step = DoctorStep("fixture", "infrastructure", ("fixture",))
        observed: dict[str, object] = {}

        def capture_result(selected: DoctorStep, **kwargs: object) -> DoctorResult:
            observed.update(kwargs)
            return DoctorResult(selected.label, 0, "", 0.01)

        child_env = {
            "RECOIL_CANONICAL_ROOT": str(REPO_ROOT),
            "RECOIL_EXECUTION_WORKTREE_ROOT": str(REPO_ROOT),
            "RECOIL_EXTERNAL_BUILD_ROOT": str(topology.build_parent / "doctor-validation"),
        }
        with (
            mock.patch("_recoil.commands.doctor.resolve_canonical_control_root", return_value=canonical),
            mock.patch("_recoil.commands.doctor.resolve_topology", return_value=topology),
            mock.patch("_recoil.commands.doctor.canonical_validation_environment", return_value=child_env),
            mock.patch("_recoil.commands.doctor.reauthenticate_canonical_control_root") as reauthenticate,
            mock.patch("_recoil.commands.doctor.build_steps", return_value=[step]),
            mock.patch("_recoil.commands.doctor.run_step_result", side_effect=capture_result),
            mock.patch("_recoil.commands.doctor.safe_print"),
        ):
            returncode = main(["--jobs", "1"])

        self.assertEqual(0, returncode)
        self.assertEqual(child_env, observed["env"])
        self.assertEqual(REPO_ROOT.resolve(), observed["execution_root"])
        reauthenticate.assert_called_once_with(canonical)

    def test_quick_steps_do_not_route_legacy_groups_or_handoffs(self) -> None:
        for binary in ("recoil", "messages"):
            with self.subTest(binary=binary):
                steps = quick_steps(binary)
                labels = {step.label for step in steps}
                self.assertNotIn("active implementation groups", labels)
                self.assertNotIn("subagent handoff blocks", labels)

    def test_dry_run_prints_unified_progress_and_no_legacy_group_commands(self) -> None:
        for binary in ("recoil", "messages"):
            with self.subTest(binary=binary):
                output: list[str] = []
                with mock.patch("_recoil.commands.doctor.safe_print", side_effect=output.append):
                    returncode = main(["--binary", binary, "--quick", "--dry-run"])

                rendered = "\n".join(output)
                self.assertEqual(0, returncode)
                self.assertIn("progress audit --strict", rendered)
                self.assertNotIn("audit groups", rendered)
                self.assertNotIn("audit handoff", rendered)

    def test_infrastructure_only_filters_reconstruction_and_keeps_opt_in_infrastructure(self) -> None:
        args = build_parser().parse_args(
            ["--quick", "--infrastructure-only", "--binja", "--native-x86"]
        )

        steps = build_steps(
            args,
            execution_root=REPO_ROOT,
            canonical_root=canonical_test_root(),
        )
        labels = {step.label for step in steps}

        self.assertTrue(steps)
        self.assertTrue(all(step.category == "infrastructure" for step in steps))
        self.assertIn("Binary Ninja bridge", labels)
        self.assertIn("native x86 environment", labels)
        self.assertNotIn("VC manifest source policy", labels)
        self.assertNotIn("production source fragments", labels)
        self.assertNotIn("original source symbol guard", labels)

    def test_infrastructure_only_wins_when_quick_is_redundant(self) -> None:
        infrastructure_only = build_steps(build_parser().parse_args(["--infrastructure-only"]))
        redundant_quick = build_steps(
            build_parser().parse_args(["--quick", "--infrastructure-only"])
        )

        self.assertEqual(infrastructure_only, redundant_quick)

    def test_infrastructure_only_accepts_supported_execution_options(self) -> None:
        args = build_parser().parse_args(
            [
                "--infrastructure-only",
                "--binary",
                "messages",
                "--binja",
                "--native-x86",
                "--dry-run",
                "--verbose",
                "--timings",
                "--jobs",
                "2",
                "--test-jobs",
                "3",
            ]
        )

        self.assertTrue(args.infrastructure_only)
        self.assertEqual("messages", args.binary)
        self.assertTrue(args.binja)
        self.assertTrue(args.native_x86)
        self.assertTrue(args.dry_run)
        self.assertTrue(args.verbose)
        self.assertTrue(args.timings)
        self.assertEqual(2, args.jobs)
        self.assertEqual(3, args.test_jobs)

    def test_infrastructure_only_rejects_active_and_bn_compare(self) -> None:
        with self.assertRaises(SystemExit):
            main(["--infrastructure-only", "--active", "0x415220", "--dry-run"])
        with self.assertRaises(SystemExit):
            main(["--infrastructure-only", "--bn-compare", "--dry-run"])

    def test_infrastructure_only_dry_run_explains_skipped_reconstruction(self) -> None:
        output: list[str] = []
        with mock.patch("_recoil.commands.doctor.safe_print", side_effect=output.append):
            returncode = main(["--infrastructure-only", "--dry-run"])

        self.assertEqual(0, returncode)
        self.assertEqual(INFRASTRUCTURE_ONLY_NOTICE, output[0])
        self.assertIn("reconstruction checks were skipped", output[0])
        self.assertIn("doctor --quick", output[0])
        self.assertNotIn("original source symbol guard", "\n".join(output))

    def test_executed_run_reports_category_and_advisory_failure_totals(self) -> None:
        steps = [
            DoctorStep("infrastructure required", "infrastructure", ("ignored",)),
            DoctorStep("reconstruction required", "reconstruction", ("ignored",)),
            DoctorStep("infrastructure advisory", "infrastructure", ("ignored",), required=False),
        ]
        returncodes = {
            "infrastructure required": 2,
            "reconstruction required": 3,
            "infrastructure advisory": 4,
        }

        def result_for(step: DoctorStep, **_: object) -> DoctorResult:
            return DoctorResult(step.label, returncodes[step.label], "", 0.01)

        output: list[str] = []
        with (
            mock.patch("_recoil.commands.doctor.build_steps", return_value=steps),
            mock.patch("_recoil.commands.doctor.run_step_result", side_effect=result_for),
            mock.patch("_recoil.commands.doctor.safe_print", side_effect=output.append),
        ):
            returncode = main(["--jobs", "1"])

        self.assertEqual(1, returncode)
        self.assertEqual(
            "doctor_summary: infrastructure_failures=1 reconstruction_failures=1 advisory_failures=1",
            output[-1],
        )

    def test_advisory_failure_does_not_change_success_exit_semantics(self) -> None:
        steps = [
            DoctorStep("infrastructure advisory", "infrastructure", ("ignored",), required=False),
        ]

        output: list[str] = []
        with (
            mock.patch("_recoil.commands.doctor.build_steps", return_value=steps),
            mock.patch(
                "_recoil.commands.doctor.run_step_result",
                return_value=DoctorResult("infrastructure advisory", 5, "", 0.01),
            ),
            mock.patch("_recoil.commands.doctor.safe_print", side_effect=output.append),
        ):
            returncode = main(["--jobs", "1"])

        self.assertEqual(0, returncode)
        self.assertTrue(any(line.endswith("Doctor passed.") for line in output))
        self.assertEqual(
            "doctor_summary: infrastructure_failures=0 reconstruction_failures=0 advisory_failures=1",
            output[-1],
        )

    def test_tool_test_files_are_deterministic(self) -> None:
        names = [path.name for path in tool_test_files()]

        self.assertIn("recoil_doctor_tests.py", names)
        self.assertEqual(sorted(names), names)

    def test_binja_step_is_opt_in(self) -> None:
        args = build_parser().parse_args(["--binja"])

        steps = build_steps(
            args,
            execution_root=REPO_ROOT,
            canonical_root=canonical_test_root(),
        )
        commands = [step.command for step in steps if step.label == "Binary Ninja bridge"]

        self.assertEqual(1, len(commands))
        self.assertTrue(str(commands[0][1]).endswith("recoil.py"))
        self.assertIn("binja", commands[0])
        self.assertIn("preflight", commands[0])
        self.assertIn("--strict", commands[0])

    def test_active_steps_default_to_compile_only_verification(self) -> None:
        args = build_parser().parse_args(["--active", "0x415220"])

        steps = build_steps(
            args,
            execution_root=REPO_ROOT,
            canonical_root=canonical_test_root(),
        )
        verify_commands = [step.command for step in steps if step.label.startswith("active VC compile")]

        self.assertEqual(1, len(verify_commands))
        self.assertIn("--skip-bn-compare", verify_commands[0])

    def test_active_steps_can_enable_bn_compare(self) -> None:
        args = build_parser().parse_args(["--active", "0x415220", "--bn-compare"])

        steps = build_steps(
            args,
            execution_root=REPO_ROOT,
            canonical_root=canonical_test_root(),
        )
        verify_commands = [step.command for step in steps if step.label.startswith("active VC byte verify")]

        self.assertEqual(1, len(verify_commands))
        self.assertNotIn("--skip-bn-compare", verify_commands[0])

    def test_active_steps_include_vc_compile_for_owner_projection(self) -> None:
        args = build_parser().parse_args(["--active", "0x407010"])

        steps = build_steps(
            args,
            execution_root=REPO_ROOT,
            canonical_root=canonical_test_root(),
        )
        labels = [step.label for step in steps]

        self.assertIn("active status 0x407010", labels)
        self.assertTrue(any(label.startswith("active VC") for label in labels))

    def test_format_command_quotes_paths_with_spaces(self) -> None:
        formatted = format_command(("python", "D:/Recoil Project/tool.py", "--flag"))

        self.assertEqual('python "D:/Recoil Project/tool.py" --flag', formatted)

    def test_dirty_delta_flag_is_not_registered(self) -> None:
        with self.assertRaises(SystemExit):
            build_parser().parse_args(["--allow-dirty-delta"])

    def test_parallel_flags_are_registered(self) -> None:
        args = build_parser().parse_args(["--jobs", "3", "--test-jobs", "4", "--timings"])

        self.assertEqual(3, args.jobs)
        self.assertEqual(4, args.test_jobs)
        self.assertTrue(args.timings)

    def test_parallel_flag_defaults_are_bounded(self) -> None:
        args = build_parser().parse_args([])

        self.assertEqual(default_jobs(), args.jobs)
        self.assertEqual(default_test_jobs(), args.test_jobs)
        self.assertGreaterEqual(args.jobs, 1)
        self.assertLessEqual(args.jobs, 6)
        self.assertGreaterEqual(args.test_jobs, 1)
        self.assertLessEqual(args.test_jobs, 8)

    def test_parallel_flags_reject_zero(self) -> None:
        with self.assertRaises(SystemExit):
            build_parser().parse_args(["--jobs", "0"])
        with self.assertRaises(SystemExit):
            build_parser().parse_args(["--test-jobs", "0"])

    def test_print_result_replaces_unicode_output_for_cp1252_console(self) -> None:
        stream = Cp1252StrictStream()
        result = DoctorResult("Python tool tests", 1, "test output ✅", 0.01)

        with mock.patch("sys.stdout", stream):
            print_result(result, verbose=False, timings=False)

        output = stream.getvalue()
        self.assertIn("FAIL: Python tool tests (exit 1)", output)
        self.assertIn("test output ?", output)

    def test_print_advisory_result_replaces_unicode_output_for_cp1252_console(self) -> None:
        stream = Cp1252StrictStream()
        result = DoctorResult("README progress freshness", 1, "optional check ✅", 0.01)

        with mock.patch("sys.stdout", stream):
            print_advisory_result(result, verbose=True, timings=False)

        output = stream.getvalue()
        self.assertIn("WARN: README progress freshness (exit 1)", output)
        self.assertIn("optional check ?", output)


if __name__ == "__main__":
    unittest.main()
