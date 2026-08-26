#!/usr/bin/env python3
"""Run the common reconstruction process checks from one entry point."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import contextlib
from dataclasses import dataclass
import os
from pathlib import Path
import subprocess
import sys
import threading
import time
from typing import Literal, TextIO
import unittest

from _recoil.lib.binja import BN_CALL_BUDGET_FILE_ENV, create_shared_budget_file, env_with_shared_budget
from _recoil.lib.owner_entries import OwnerEntryIndex, normalize_address
from _recoil.lib.reference_images import (
    reference_image,
    reference_image_keys,
)
from _recoil.lib.tooling import REPO_ROOT, hidden_creation_flags
from _recoil.lib.worktree_control import (
    CanonicalControlRoot,
    canonical_validation_environment,
    resolve_topology,
    reauthenticate_canonical_control_root,
    resolve_canonical_control_root,
)


DoctorCategory = Literal["infrastructure", "reconstruction"]
DOCTOR_CATEGORIES = frozenset({"infrastructure", "reconstruction"})
INFRASTRUCTURE_ONLY_NOTICE = (
    "Infrastructure-only mode: reconstruction checks were skipped; "
    "run `python tools/recoil.py doctor --quick` for full health."
)
_TOOL_TEST_CHILD_FLAG = "--_run-tool-test-file"
_VC_MANIFEST_CHILD_FLAG = "--_validate-vc-manifests"


@dataclass(frozen=True)
class DoctorStep:
    label: str
    category: DoctorCategory
    command: tuple[str, ...] = ()
    kind: str = "command"
    required: bool = True
    working_directory: Path | None = None
    python_path_root: Path | None = None

    def __post_init__(self) -> None:
        if self.category not in DOCTOR_CATEGORIES:
            raise ValueError(f"invalid doctor category: {self.category}")


@dataclass(frozen=True)
class DoctorResult:
    label: str
    returncode: int
    stdout: str
    duration: float


def py(*args: str | Path) -> tuple[str, ...]:
    return (sys.executable, *(str(arg) for arg in args))


def gate(*args: str, execution_root: Path = REPO_ROOT) -> tuple[str, ...]:
    return py(execution_root / "tools" / "recoil.py", *args)


def quick_steps(
    binary: str,
    *,
    execution_root: Path = REPO_ROOT,
    canonical_root: Path = REPO_ROOT,
) -> list[DoctorStep]:
    image = reference_image(binary)
    def execution_gate(*args: str) -> tuple[str, ...]:
        return gate(*args, execution_root=execution_root)

    progress_path = canonical_root / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
    issue_path = canonical_root / ".agent" / "WORKSPACE_ISSUES.sqlite3"
    owners_path = progress_path
    reference_path = canonical_root / image.reference_path
    manifest_path = execution_root / image.manifest_path
    source_root = image.source_root
    reference_manifest_command = py(
        "-m",
        "_recoil.commands.pe_reference",
        "--reference",
        image.reference_path,
        "--manifest",
        manifest_path,
        "--verify",
    )
    source_goto_args = ["guard", "source-goto", "--root", source_root, "--summary"]
    if binary == "messages":
        source_goto_args.append("--strict-zero")
    return [
        DoctorStep("VC manifest source policy", "reconstruction", execution_gate("guard", "vc5-manifest")),
        DoctorStep(
            "production source fragments",
            "reconstruction",
            execution_gate("guard", "source-fragments", "--root", source_root),
        ),
        DoctorStep("compiler/linker provenance", "infrastructure", execution_gate("audit", "provenance", "--strict")),
        DoctorStep("workspace hygiene", "infrastructure", execution_gate("audit", "workspace", "--strict")),
        DoctorStep(
            "raw image address guard",
            "reconstruction",
            execution_gate(
                "guard",
                "raw-image",
                "--root",
                source_root,
                "--allowlist",
                ".agent/RAW_ADDRESS_ALLOWLIST.txt",
            ),
        ),
        DoctorStep(
            "raw assembly guard",
            "reconstruction",
            execution_gate(
                "guard",
                "raw-assembly",
                "--root",
                source_root,
                "--allowlist",
                ".agent/RAW_ASSEMBLY_ALLOWLIST.txt",
            ),
        ),
        DoctorStep(
            "source-level goto guard",
            "reconstruction",
            execution_gate(*source_goto_args),
        ),
        DoctorStep(
            "modern C++ construct guard",
            "reconstruction",
            execution_gate(
                "guard",
                "modern-cpp",
                "--root",
                source_root,
            ),
        ),
        DoctorStep(
            "source-shape scaffold guard",
            "reconstruction",
            execution_gate(
                "guard",
                "source-shape",
                "--root",
                source_root,
            ),
        ),
        DoctorStep(
            "raw offset tier guard",
            "reconstruction",
            execution_gate(
                "guard",
                "raw-offset",
                "--root",
                source_root,
                "--progress",
                progress_path,
                "--binary",
                binary,
                "--allowlist",
                ".agent/RAW_OFFSET_ALLOWLIST.txt",
            ),
        ),
        DoctorStep(
            "original source symbol guard",
            "reconstruction",
            execution_gate(
                "guard",
                "original-symbol",
                "--root",
                source_root,
                "--progress",
                owners_path,
            ),
        ),
        DoctorStep(
            "unified reconstruction progress",
            "infrastructure",
            execution_gate("progress", "audit", "--strict", "--progress", str(progress_path)),
        ),
        DoctorStep(
            "current metadata and README freshness",
            "infrastructure",
            execution_gate(
                "audit", "current-metadata", "--strict",
                "--progress", str(progress_path),
                "--readme", str(execution_root / "README.md"),
            ),
        ),
        DoctorStep("agent surface alignment", "infrastructure", execution_gate("audit", "agent-surface", "--strict")),
        DoctorStep(
            "live workflow contracts",
            "infrastructure",
            execution_gate(
                "audit", "workflow-contracts", "--strict",
                "--root", str(execution_root),
                "--canonical-root", str(canonical_root),
                "--progress", str(progress_path),
            ),
        ),
        DoctorStep(
            "pipeline producer reachability",
            "infrastructure",
            execution_gate(
                "audit", "pipeline-reachability", "--strict",
                "--root", str(execution_root),
                "--canonical-root", str(canonical_root),
                "--tracker", str(progress_path),
                "--reference", str(reference_path),
                "--manifest-dir", str(execution_root / "tools" / "vc5_verify_targets"),
            ),
        ),
        DoctorStep(
            "agent tooling/process issue ledger",
            "infrastructure",
            execution_gate("issue", "audit", "--strict", "--ledger", str(issue_path)),
        ),
        DoctorStep(
            f"reference {image.display_name} manifest",
            "infrastructure",
            reference_manifest_command,
            working_directory=canonical_root,
            python_path_root=execution_root / "tools",
        ),
        DoctorStep("functional manifest load", "infrastructure", execution_gate("verify", "functional", "--list")),
        DoctorStep(
            "VC manifest load",
            "infrastructure",
            py(
                execution_root / "tools" / "_recoil" / "commands" / "doctor.py",
                _VC_MANIFEST_CHILD_FLAG,
            ),
            working_directory=canonical_root,
        ),
        DoctorStep(
            "workspace worktree status",
            "infrastructure",
            execution_gate("workspace", "worktree", "status", "--ledger", str(issue_path), "--json"),
        ),
        DoctorStep(
            "workspace worktree hygiene",
            "infrastructure",
            execution_gate("workspace", "worktree", "hygiene", "--ledger", str(issue_path), "--json"),
        ),
        DoctorStep("Python tool tests", "infrastructure", kind="python_tool_tests"),
    ]


def native_x86_steps(*, execution_root: Path = REPO_ROOT) -> list[DoctorStep]:
    return [DoctorStep(
        "native x86 environment",
        "infrastructure",
        gate("env", "--native-x86", execution_root=execution_root),
    )]


def binja_steps(binary: str, *, execution_root: Path = REPO_ROOT) -> list[DoctorStep]:
    return [
        DoctorStep(
            "Binary Ninja bridge",
            "infrastructure",
            gate(
                "binja", "preflight", "--binary", binary, "--strict",
                execution_root=execution_root,
            ),
        )
    ]


def active_steps(
    address: str,
    *,
    binary: str,
    bn_compare: bool,
    execution_root: Path = REPO_ROOT,
    canonical_root: Path = REPO_ROOT,
) -> list[DoctorStep]:
    normalized = normalize_address(address)
    progress_path = canonical_root / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
    steps = [
        DoctorStep(
            f"active status {normalized}",
            "reconstruction",
            gate(
                "progress", "show", normalized,
                "--progress", str(progress_path),
                execution_root=execution_root,
            ),
        )
    ]

    plan = OwnerEntryIndex.load(
        progress_path,
        binary=binary,
    )
    entry = plan.entries.get(normalized)
    if entry is not None and entry.is_provider:
        return steps

    verify_args: list[str] = [
        "verify", "vc5", address, "--all-covering",
        "--progress", str(progress_path),
    ]
    if not bn_compare:
        verify_args.append("--skip-bn-compare")
    steps.append(
        DoctorStep(
            f"active VC {'byte verify' if bn_compare else 'compile'} {normalized}",
            "reconstruction",
            gate(*verify_args, execution_root=execution_root),
        )
    )
    return steps


def build_steps(
    args: argparse.Namespace,
    *,
    execution_root: Path = REPO_ROOT,
    canonical_root: Path = REPO_ROOT,
) -> list[DoctorStep]:
    steps = quick_steps(
        args.binary,
        execution_root=execution_root,
        canonical_root=canonical_root,
    )
    if args.binja:
        steps.extend(binja_steps(args.binary, execution_root=execution_root))
    if args.native_x86:
        steps.extend(native_x86_steps(execution_root=execution_root))
    if args.active:
        steps.extend(active_steps(
            args.active,
            binary=args.binary,
            bn_compare=args.bn_compare,
            execution_root=execution_root,
            canonical_root=canonical_root,
        ))
    if args.infrastructure_only:
        steps = [step for step in steps if step.category == "infrastructure"]
    return steps


def format_command(command: tuple[str, ...]) -> str:
    return " ".join(f'"{part}"' if any(char.isspace() for char in part) else part for part in command)


def format_step_command(step: DoctorStep) -> str:
    if step.kind == "python_tool_tests":
        return "parallel per-file unittest discovery for tests/tools/*_tests.py"
    return format_command(step.command)


def default_jobs() -> int:
    return min(6, os.cpu_count() or 2)


def default_test_jobs() -> int:
    return min(8, os.cpu_count() or 2)


def positive_int(raw: str) -> int:
    value = int(raw)
    if value < 1:
        raise argparse.ArgumentTypeError("must be at least 1")
    return value


def tool_test_files(*, execution_root: Path = REPO_ROOT) -> list[Path]:
    retired_tracker_tests = {"recoil_source_file_map_tests.py"}
    return sorted(
        (
            path for path in (execution_root / "tests" / "tools").glob("*_tests.py")
            if path.name not in retired_tracker_tests
        ),
        key=lambda path: path.name,
    )


def run_subprocess(
    command: tuple[str, ...],
    *,
    env: dict[str, str] | None = None,
    execution_root: Path = REPO_ROOT,
    working_directory: Path | None = None,
) -> tuple[int, str]:
    completed = subprocess.run(
        command,
        cwd=working_directory or execution_root,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        errors="replace",
        creationflags=hidden_creation_flags(),
    )
    return completed.returncode, completed.stdout


def run_tool_test_file(
    path: Path,
    *,
    env: dict[str, str] | None = None,
    execution_root: Path = REPO_ROOT,
) -> tuple[str, int, str, float]:
    start = time.perf_counter()
    child_env = env
    working_directory: Path | None = None
    if env is not None and env.get("RECOIL_CANONICAL_ROOT"):
        child_env = dict(env)
        working_directory = execution_root
        command = py(
            execution_root / "tools" / "_recoil" / "commands" / "doctor.py",
            _TOOL_TEST_CHILD_FLAG,
            path.resolve(strict=True),
        )
    else:
        command = py(
            "-m", "unittest", "discover", "-s", "tests/tools", "-p", path.name
        )
    returncode, stdout = run_subprocess(
        command,
        env=child_env,
        execution_root=execution_root,
        working_directory=working_directory,
    )
    return path.name, returncode, stdout, time.perf_counter() - start


def _validated_child_roots() -> tuple[Path, CanonicalControlRoot]:
    """Reauthenticate bounded roots inherited from the parent doctor."""

    execution_text = os.environ.get("RECOIL_EXECUTION_WORKTREE_ROOT")
    canonical_text = os.environ.get("RECOIL_CANONICAL_ROOT")
    if not execution_text or not canonical_text:
        raise RuntimeError("doctor child is missing authenticated root provenance")
    execution_root = Path(execution_text).resolve(strict=True)
    if execution_root != REPO_ROOT.resolve(strict=True):
        raise RuntimeError(
            "doctor child execution root does not match its tracked implementation"
        )
    canonical = resolve_canonical_control_root(
        executing_worktree_root=execution_root,
        required_machine_local_paths=(
            ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
            ".agent/WORKSPACE_ISSUES.sqlite3",
        ),
        explicit_root=Path(canonical_text),
    )
    return execution_root, canonical


def _run_vc_manifest_child() -> int:
    """Load linked tracked VC manifests against the canonical live tracker."""

    from _recoil.commands import vc5_verify
    from _recoil.commands.pipeline_reachability_audit import _bound_vc5_tracker

    execution_root, canonical = _validated_child_roots()
    tracker = (
        canonical.canonical_control_root
        / ".agent"
        / "RECONSTRUCTION_PROGRESS.sqlite3"
    )
    try:
        with _bound_vc5_tracker(tracker):
            return vc5_verify.main(
                [
                    "--list",
                    "--manifest-dir",
                    str(execution_root / "tools" / "vc5_verify_targets"),
                ]
            )
    finally:
        reauthenticate_canonical_control_root(canonical)


def _same_doctor_child_physical_name(left: object, right: Path) -> bool:
    try:
        left_text = os.path.abspath(os.fspath(left))
    except TypeError:
        return False
    return os.path.normcase(left_text) == os.path.normcase(str(right))


@contextlib.contextmanager
def _doctor_test_machine_local_bindings(
    *,
    execution_root: Path,
    canonical_root: Path,
) -> object:
    """Route only exact linked machine-local defaults for a test child."""

    from unittest import mock

    from _recoil.commands import (
        call_contract_verify,
        progress_cli,
        provider_target_mutation,
    )
    from _recoil.lib import progress_sqlite
    from _recoil.lib.windows_identity import StableReadHandle

    linked_retail = execution_root / "support" / "Recoil.exe"
    canonical_retail = canonical_root / "support" / "Recoil.exe"
    linked_progress = (
        execution_root / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
    )
    canonical_progress = (
        canonical_root / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
    )
    canonical_issue_ledger = (
        canonical_root / ".agent" / "WORKSPACE_ISSUES.sqlite3"
    )
    linked_vc5_environment = (
        execution_root.parent / "Compiler" / "VC5SP3" / "vc5sp3-env.cmd"
    )
    canonical_vc5_environment = (
        canonical_root.parent / "Compiler" / "VC5SP3" / "vc5sp3-env.cmd"
    )
    canonical_zeffect_backends: list[dict[str, object]] = []
    for backend in call_contract_verify.ZEFFECT_PROFILE_MATRIX_BACKENDS:
        routed_backend = dict(backend)
        if backend.get("backend_id") == "vc5sp3-c2-7303":
            routed_backend["c2_path"] = (
                canonical_vc5_environment.parent / "VC/BIN/C2.EXE"
            )
        elif backend.get("backend_id") == "vc5-rtm-c2-1100-diagnostic":
            routed_backend["c2_path"] = (
                canonical_root.parent
                / "Visual C++ 5.0/DEVSTUDIO/VC/BIN/C2.EXE"
            )
        canonical_zeffect_backends.append(routed_backend)
    original_path_stat = Path.stat
    original_path_is_file = Path.is_file
    original_path_open = Path.open
    original_progress_store_init = progress_sqlite.ProgressSQLiteStore.__init__

    def routed_path_stat(
        selected_path: Path, *args: object, **kwargs: object
    ) -> os.stat_result:
        # Preserve the linked logical spelling while routing only the exact
        # machine-local retail stat to the authenticated canonical file.
        if _same_doctor_child_physical_name(selected_path, linked_retail):
            return original_path_stat(canonical_retail, *args, **kwargs)
        return original_path_stat(selected_path, *args, **kwargs)

    def routed_path_is_file(
        selected_path: Path, *args: object, **kwargs: object
    ) -> bool:
        if _same_doctor_child_physical_name(selected_path, linked_retail):
            return original_path_is_file(canonical_retail, *args, **kwargs)
        return original_path_is_file(selected_path, *args, **kwargs)

    def routed_path_open(
        selected_path: Path, *args: object, **kwargs: object
    ) -> object:
        physical_path = (
            canonical_retail
            if _same_doctor_child_physical_name(selected_path, linked_retail)
            else selected_path
        )
        return original_path_open(physical_path, *args, **kwargs)

    def routed_stable_read_handle(
        selected_path: str | Path, *args: object, **kwargs: object
    ) -> StableReadHandle:
        physical_path = (
            canonical_retail
            if _same_doctor_child_physical_name(selected_path, linked_retail)
            else selected_path
        )
        return StableReadHandle(physical_path, *args, **kwargs)

    def routed_progress_store_init(
        store: object,
        selected_path: str | Path,
        *args: object,
        **kwargs: object,
    ) -> None:
        physical_path = (
            canonical_progress
            if _same_doctor_child_physical_name(selected_path, linked_progress)
            else selected_path
        )
        original_progress_store_init(store, physical_path, *args, **kwargs)

    restored_function_defaults: list[
        tuple[object, tuple[object, ...] | None, dict[str, object] | None]
    ] = []
    for candidate in vars(call_contract_verify).values():
        if not callable(candidate) or getattr(candidate, "__module__", None) != (
            call_contract_verify.__name__
        ):
            continue
        defaults = getattr(candidate, "__defaults__", None)
        keyword_defaults = getattr(candidate, "__kwdefaults__", None)
        changed_defaults = defaults
        changed_keyword_defaults = keyword_defaults
        if defaults:
            changed_defaults = tuple(
                canonical_vc5_environment
                if _same_doctor_child_physical_name(
                    value, linked_vc5_environment
                )
                else value
                for value in defaults
            )
        if keyword_defaults:
            changed_keyword_defaults = {
                key: (
                    canonical_vc5_environment
                    if _same_doctor_child_physical_name(
                        value, linked_vc5_environment
                    )
                    else value
                )
                for key, value in keyword_defaults.items()
            }
        if (
            changed_defaults != defaults
            or changed_keyword_defaults != keyword_defaults
        ):
            restored_function_defaults.append(
                (candidate, defaults, keyword_defaults)
            )
            candidate.__defaults__ = changed_defaults
            candidate.__kwdefaults__ = changed_keyword_defaults

    with contextlib.ExitStack() as bindings:
        for candidate, defaults, keyword_defaults in restored_function_defaults:
            bindings.callback(
                setattr, candidate, "__kwdefaults__", keyword_defaults
            )
            bindings.callback(setattr, candidate, "__defaults__", defaults)
        bindings.enter_context(mock.patch.object(Path, "stat", routed_path_stat))
        bindings.enter_context(mock.patch.object(
            Path,
            "is_file",
            routed_path_is_file,
        ))
        bindings.enter_context(mock.patch.object(
            Path,
            "open",
            routed_path_open,
        ))
        bindings.enter_context(mock.patch.object(
            progress_sqlite.ProgressSQLiteStore,
            "__init__",
            routed_progress_store_init,
        ))
        bindings.enter_context(mock.patch.object(
            call_contract_verify,
            "StableReadHandle",
            routed_stable_read_handle,
        ))
        bindings.enter_context(mock.patch.object(
            provider_target_mutation,
            "StableReadHandle",
            routed_stable_read_handle,
        ))
        bindings.enter_context(mock.patch.object(
            call_contract_verify,
            "DEFAULT_REFERENCE",
            canonical_retail,
        ))
        bindings.enter_context(mock.patch.object(
            call_contract_verify,
            "DEFAULT_VC5_ENV",
            canonical_vc5_environment,
        ))
        bindings.enter_context(mock.patch.object(
            call_contract_verify,
            "ZEFFECT_PROFILE_MATRIX_BACKENDS",
            tuple(canonical_zeffect_backends),
        ))
        bindings.enter_context(mock.patch.object(
            provider_target_mutation,
            "DEFAULT_REFERENCE",
            canonical_retail,
        ))
        bindings.enter_context(mock.patch.object(
            progress_cli,
            "DEFAULT_ISSUE_LEDGER",
            canonical_issue_ledger,
        ))
        yield


def _run_tool_test_child(path_text: str) -> int:
    """Discover one linked test file with canonical machine-local authorities."""

    from _recoil.commands.pipeline_reachability_audit import (
        _bound_call_contract_include_roots,
        _bound_vc5_tracker,
    )

    execution_root, canonical = _validated_child_roots()
    test_root = (execution_root / "tests" / "tools").resolve(strict=True)
    test_path = Path(path_text).resolve(strict=True)
    if test_path.parent != test_root or not test_path.name.endswith("_tests.py"):
        raise RuntimeError(
            f"doctor child test path is outside linked tests/tools: {test_path}"
        )
    tracker = (
        canonical.canonical_control_root
        / ".agent"
        / "RECONSTRUCTION_PROGRESS.sqlite3"
    )
    (execution_root / "build").mkdir(exist_ok=True)
    try:
        if str(execution_root) not in sys.path:
            sys.path.insert(0, str(execution_root))
        with contextlib.ExitStack() as bindings:
            bindings.enter_context(_bound_vc5_tracker(tracker))
            bindings.enter_context(_bound_call_contract_include_roots(
                execution_root=execution_root,
                canonical_root=canonical.canonical_control_root,
            ))
            bindings.enter_context(_doctor_test_machine_local_bindings(
                execution_root=execution_root,
                canonical_root=canonical.canonical_control_root,
            ))
            suite = unittest.defaultTestLoader.discover(
                start_dir=str(test_root),
                pattern=test_path.name,
            )
            if test_path.name == "recoil_provider_function_mutation_tests.py":
                module = sys.modules.get(test_path.stem)
                if module is None:
                    raise RuntimeError(
                        "doctor child did not load the linked provider mutation test"
                    )
                module.VC5_ROOT = (
                    canonical.canonical_control_root.parent
                    / "Compiler"
                    / "VC5SP3"
                )
            result = unittest.TextTestRunner().run(suite)
        return 0 if result.wasSuccessful() else 1
    finally:
        reauthenticate_canonical_control_root(canonical)


def format_tool_test_output(results: list[tuple[str, int, str, float]], *, verbose: bool, timings: bool) -> str:
    lines: list[str] = []
    for name, returncode, stdout, duration in results:
        status = "OK" if returncode == 0 else f"FAIL exit {returncode}"
        timing = f" ({duration:.2f}s)" if timings else ""
        lines.append(f"{name}: {status}{timing}")
        if stdout and (verbose or returncode != 0):
            lines.append(stdout.rstrip())
    return "\n".join(line for line in lines if line)


def run_python_tool_tests(
    *,
    test_jobs: int,
    verbose: bool,
    timings: bool,
    env: dict[str, str] | None = None,
    execution_root: Path = REPO_ROOT,
) -> tuple[int, str]:
    files = tool_test_files(execution_root=execution_root)
    if not files:
        return 1, "No tests/tools/*_tests.py files found."

    results: list[tuple[str, int, str, float] | None] = [None] * len(files)
    if test_jobs == 1 or len(files) == 1:
        for index, path in enumerate(files):
            results[index] = run_tool_test_file(
                path, env=env, execution_root=execution_root
            )
    else:
        with ThreadPoolExecutor(max_workers=min(test_jobs, len(files))) as executor:
            future_indexes = {
                executor.submit(
                    run_tool_test_file,
                    path,
                    env=env,
                    execution_root=execution_root,
                ): index
                for index, path in enumerate(files)
            }
            for future in as_completed(future_indexes):
                results[future_indexes[future]] = future.result()

    completed_results = [result for result in results if result is not None]
    failed = [returncode for _, returncode, _, _ in completed_results if returncode != 0]
    returncode = failed[0] if failed else 0
    return returncode, format_tool_test_output(completed_results, verbose=verbose, timings=timings)


def run_step_result(
    step: DoctorStep,
    *,
    verbose: bool,
    timings: bool,
    test_jobs: int,
    env: dict[str, str] | None = None,
    binja_lock: threading.Lock | None = None,
    execution_root: Path = REPO_ROOT,
) -> DoctorResult:
    start = time.perf_counter()
    step_env = env
    if step.python_path_root is not None:
        step_env = dict(os.environ if env is None else env)
        existing_python_path = step_env.get("PYTHONPATH", "")
        python_path_parts = [str(step.python_path_root)]
        if existing_python_path:
            python_path_parts.append(existing_python_path)
        step_env["PYTHONPATH"] = os.pathsep.join(python_path_parts)
    if step.kind == "python_tool_tests":
        returncode, stdout = run_python_tool_tests(
            test_jobs=test_jobs,
            verbose=verbose,
            timings=timings,
            env=step_env,
            execution_root=execution_root,
        )
    else:
        if binja_lock is not None and step_uses_binja(step):
            with binja_lock:
                returncode, stdout = run_subprocess(
                    step.command,
                    env=step_env,
                    execution_root=execution_root,
                    working_directory=step.working_directory,
                )
        else:
            returncode, stdout = run_subprocess(
                step.command,
                env=step_env,
                execution_root=execution_root,
                working_directory=step.working_directory,
            )
    return DoctorResult(step.label, returncode, stdout, time.perf_counter() - start)


def safe_print(text: str = "", *, file: TextIO | None = None) -> None:
    """Print while replacing characters unsupported by the current console."""
    stream = file if file is not None else sys.stdout
    try:
        print(text, file=stream)
        return
    except UnicodeEncodeError:
        encoding = getattr(stream, "encoding", None) or "utf-8"
        safe_text = text.encode(encoding, errors="replace").decode(encoding, errors="replace")
        print(safe_text, file=stream)


def print_result(result: DoctorResult, *, verbose: bool, timings: bool) -> None:
    timing = f" ({result.duration:.2f}s)" if timings else ""
    if result.returncode == 0:
        safe_print(f"OK: {result.label}{timing}")
        if result.stdout and (verbose or (timings and result.label == "Python tool tests")):
            safe_print(result.stdout.rstrip())
        return

    safe_print(f"FAIL: {result.label} (exit {result.returncode}){timing}")
    if result.stdout:
        safe_print(result.stdout.rstrip())


def print_advisory_result(result: DoctorResult, *, verbose: bool, timings: bool) -> None:
    timing = f" ({result.duration:.2f}s)" if timings else ""
    safe_print(f"WARN: {result.label} (exit {result.returncode}){timing}")
    if result.stdout and verbose:
        safe_print(result.stdout.rstrip())


def step_uses_binja(step: DoctorStep) -> bool:
    return step.label == "Binary Ninja bridge" or step.label.startswith("active ")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Run common Recoil reconstruction process checks.")
    parser.add_argument("--binary", choices=reference_image_keys(), default="recoil")
    parser.add_argument("--quick", action="store_true", help="Run the standard quick checks (default).")
    parser.add_argument(
        "--infrastructure-only",
        action="store_true",
        help="Run only tooling, policy, ledger, manifest, and environment infrastructure checks.",
    )
    parser.add_argument("--binja", action="store_true", help="Also check the expected Binary Ninja bridge/database state.")
    parser.add_argument("--native-x86", action="store_true", help="Also check the current native x86 MSVC environment.")
    parser.add_argument("--active", metavar="ADDRESS", help="Also run status and VC verification for an active address.")
    parser.add_argument(
        "--bn-compare",
        action="store_true",
        help="Use full Binary Ninja byte comparison for --active instead of compile-only VC verification.",
    )
    parser.add_argument("--dry-run", action="store_true", help="Print the planned checks without running them.")
    parser.add_argument("--verbose", action="store_true", help="Print successful command output.")
    parser.add_argument(
        "--jobs",
        type=positive_int,
        default=default_jobs(),
        help=f"Maximum concurrent doctor checks (default: {default_jobs()}). Use 1 for serial execution.",
    )
    parser.add_argument(
        "--test-jobs",
        type=positive_int,
        default=default_test_jobs(),
        help=f"Maximum concurrent tests/tools per-file subprocesses (default: {default_test_jobs()}). Use 1 for serial execution.",
    )
    parser.add_argument("--timings", action="store_true", help="Print elapsed time for each check.")
    parser.add_argument(
        "--canonical-root",
        type=Path,
        help="Explicit canonical control root for machine-local retail and live SQLite inputs.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    selected_argv = list(sys.argv[1:] if argv is None else argv)
    if selected_argv == [_VC_MANIFEST_CHILD_FLAG]:
        return _run_vc_manifest_child()
    if len(selected_argv) == 2 and selected_argv[0] == _TOOL_TEST_CHILD_FLAG:
        return _run_tool_test_child(selected_argv[1])
    parser = build_parser()
    args = parser.parse_args(selected_argv)
    if args.infrastructure_only and args.active:
        parser.error("--infrastructure-only cannot be used with --active")
    if args.infrastructure_only and args.bn_compare:
        parser.error("--infrastructure-only cannot be used with --bn-compare")
    execution_root = REPO_ROOT.resolve(strict=True)
    image = reference_image(args.binary)
    canonical = resolve_canonical_control_root(
        executing_worktree_root=execution_root,
        required_machine_local_paths=(
            image.reference_path,
            ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
            ".agent/WORKSPACE_ISSUES.sqlite3",
        ),
        explicit_root=args.canonical_root,
    )
    steps = build_steps(
        args,
        execution_root=execution_root,
        canonical_root=canonical.canonical_control_root,
    )

    if args.infrastructure_only:
        safe_print(INFRASTRUCTURE_ONLY_NOTICE)

    safe_print(f"execution_worktree_root: {execution_root}")
    safe_print(f"canonical_control_root: {canonical.canonical_control_root}")
    topology = resolve_topology(canonical.canonical_control_root)
    external_build_root = Path(os.environ.get(
        "RECOIL_EXTERNAL_BUILD_ROOT",
        str(topology.build_parent / "doctor-validation"),
    ))
    safe_print(f"external_build_root: {external_build_root.resolve(strict=False)}")

    if args.dry_run:
        try:
            for step in steps:
                safe_print(f"{step.label}: {format_step_command(step)}")
        finally:
            reauthenticate_canonical_control_root(canonical)
        return 0

    safe_print(f"Running {len(steps)} process check(s).")
    inherited_budget_file = os.environ.get(BN_CALL_BUDGET_FILE_ENV)
    budget_file = None
    owns_budget_file = False
    if args.binja or args.active:
        if inherited_budget_file:
            budget_file = Path(inherited_budget_file)
        else:
            budget_file = create_shared_budget_file()
            owns_budget_file = True
    child_env = canonical_validation_environment(
        canonical,
        external_build_root=external_build_root,
    )
    if budget_file is not None:
        child_env = env_with_shared_budget(budget_file, child_env)
    results: list[DoctorResult | None] = [None] * len(steps)
    binja_lock = threading.Lock()
    start = time.perf_counter()
    try:
        if args.jobs == 1 or len(steps) == 1:
            for index, step in enumerate(steps):
                results[index] = run_step_result(
                    step,
                    verbose=args.verbose,
                    timings=args.timings,
                    test_jobs=args.test_jobs,
                    env=child_env,
                    binja_lock=binja_lock,
                    execution_root=execution_root,
                )
        else:
            with ThreadPoolExecutor(max_workers=min(args.jobs, len(steps))) as executor:
                future_indexes = {}
                for index, step in enumerate(steps):
                    future = executor.submit(
                        run_step_result,
                        step,
                        verbose=args.verbose,
                        timings=args.timings,
                        test_jobs=args.test_jobs,
                        env=child_env,
                        binja_lock=binja_lock,
                        execution_root=execution_root,
                    )
                    future_indexes[future] = index
                for future in as_completed(future_indexes):
                    results[future_indexes[future]] = future.result()
    finally:
        if budget_file is not None and owns_budget_file:
            try:
                budget_file.unlink()
            except FileNotFoundError:
                pass
        reauthenticate_canonical_control_root(canonical)

    infrastructure_failures = 0
    reconstruction_failures = 0
    advisory_failures = 0
    completed_results = [result for result in results if result is not None]
    for index, result in enumerate(completed_results, start=1):
        safe_print(f"[{index}/{len(steps)}] {result.label}")
        step = steps[index - 1]
        if result.returncode != 0 and not step.required:
            print_advisory_result(result, verbose=args.verbose, timings=args.timings)
            advisory_failures += 1
            continue
        print_result(result, verbose=args.verbose, timings=args.timings)
        if result.returncode != 0:
            if step.category == "infrastructure":
                infrastructure_failures += 1
            else:
                reconstruction_failures += 1

    failures = infrastructure_failures + reconstruction_failures
    if failures:
        safe_print(f"\nDoctor failed: {failures} check(s) failed.")
        if args.timings:
            safe_print(f"Elapsed: {time.perf_counter() - start:.2f}s")
        safe_print(
            "doctor_summary: "
            f"infrastructure_failures={infrastructure_failures} "
            f"reconstruction_failures={reconstruction_failures} "
            f"advisory_failures={advisory_failures}"
        )
        return 1
    safe_print("\nDoctor passed.")
    if args.timings:
        safe_print(f"Elapsed: {time.perf_counter() - start:.2f}s")
    safe_print(
        "doctor_summary: "
        f"infrastructure_failures={infrastructure_failures} "
        f"reconstruction_failures={reconstruction_failures} "
        f"advisory_failures={advisory_failures}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
