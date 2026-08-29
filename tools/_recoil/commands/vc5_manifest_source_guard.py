#!/usr/bin/env python3
"""Guard VC verification manifests against production-source drift."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
import json
import os
from functools import lru_cache
from pathlib import Path
import sys

from _recoil.commands.vc5_verify import (
    DEFAULT_MANIFEST_DIR,
    VerifyTarget,
    generated_file_shadows_project,
    load_manifest,
    load_manifests,
    normalize_generated_path,
    print_source_emission_warnings,
    target_source_fragment_findings,
)
from _recoil.lib.repository_paths import (
    GitTrackedPathInventory,
    RepositoryPathError,
    load_git_tracked_path_inventory,
    resolve_tracked_repository_file,
    validate_repository_relative_path,
)
from _recoil.lib.tooling import REPO_ROOT
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressStore
from _recoil.lib.worktree_control import (
    CANONICAL_ROOT_ENV,
    WorktreeControlError,
    resolve_canonical_control_root,
)


DEFAULT_FINAL_BUILD_MANIFEST = REPO_ROOT / "tools" / "_recoil" / "config" / "vc5_final_build.json"
DEFAULT_PROGRESS = DEFAULT_PROGRESS_PATH
CANONICAL_MFC_INCLUDE = "d:/recoil project/compiler/vc5sp3/vc/mfc/include"
CANONICAL_MFC_LIB = "d:/recoil project/compiler/vc5sp3/vc/mfc/lib"
FORBIDDEN_ACTIVE_MFC_PATHS = {
    "support/sdk/mfc42/include",
    "support/sdk/mfc42/lib/x86",
    "d:/recoil project/visual c++ 5.0/devstudio/vc/mfc/include",
}
LINKED_ORDER_DIAGNOSTIC_OVERLAY_KEYS = frozenset(
    {
        "name",
        "description",
        "target_binary",
        "compiler_profile",
        "linked_order_base_target",
        "linked_order_diagnostic_mode",
    }
)


@lru_cache(maxsize=4)
def _guard_git_inventory(repository_root_text: str) -> GitTrackedPathInventory:
    """Load one exact tracked-path inventory for this guard invocation."""

    return load_git_tracked_path_inventory(Path(repository_root_text))


def repo_manifest_key(
    path: str | Path,
    *,
    repository_root: Path | None = None,
    inventory: GitTrackedPathInventory | None = None,
) -> str:
    """Return the exact Git-index identity for a guard manifest input.

    This retains the guard's historical string result while delegating logical
    path admission to the neutral repository-path authority.  Physical path
    spelling is used neither to normalize nor to accept the manifest.
    """

    root = (repository_root or REPO_ROOT).absolute()
    supplied = Path(path)
    if supplied.is_absolute():
        raise ValueError(
            f"guard manifest path must be a repository-relative Git path: {path}"
        )
    path_text = supplied.as_posix()
    try:
        validate_repository_relative_path(path_text, context="guard manifest path")
        tracked = inventory or _guard_git_inventory(str(root))
        return resolve_tracked_repository_file(
            path_text,
            repository_root=root,
            inventory=tracked,
            context="guard manifest path",
            allowed_suffixes={".json"},
        ).git_path
    except RepositoryPathError as exc:
        raise ValueError(str(exc)) from exc


def _repo_manifest_key_from_filesystem_path(
    path: Path,
    *,
    repository_root: Path | None = None,
    inventory: GitTrackedPathInventory | None = None,
) -> str:
    """Authenticate a trusted discovered file without projecting resolved case."""

    root = (repository_root or REPO_ROOT).absolute()
    supplied = Path(path)
    if supplied.is_absolute():
        try:
            relative = supplied.absolute().relative_to(root)
        except ValueError as exc:
            raise ValueError(
                f"guard manifest path must be inside the executing worktree: {path}"
            ) from exc
    else:
        relative = supplied
    return repo_manifest_key(
        relative,
        repository_root=root,
        inventory=inventory,
    )


def normalized_repo_path(path: str) -> str:
    return path.replace("\\", "/").lower()


def nested_strings(value: object):
    if isinstance(value, str):
        yield value
    elif isinstance(value, list):
        for item in value:
            yield from nested_strings(item)
    elif isinstance(value, dict):
        for item in value.values():
            yield from nested_strings(item)


def active_mfc_path_debt(manifest_paths: list[Path]) -> set[tuple[str, str]]:
    debt: set[tuple[str, str]] = set()
    for manifest_path in manifest_paths:
        data = json.loads(manifest_path.read_text(encoding="utf-8"))
        for value in nested_strings(data):
            normalized = normalized_repo_path(value).rstrip("/")
            if normalized in FORBIDDEN_ACTIVE_MFC_PATHS:
                debt.add((_repo_manifest_key_from_filesystem_path(manifest_path), value))
    return debt


def discover_named_manifest(
    name: str,
    *,
    manifest_path: Path,
) -> VerifyTarget:
    candidates: list[Path] = []
    for candidate in sorted(manifest_path.parent.glob("*.json")):
        try:
            candidate_data = json.loads(candidate.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise ValueError(f"{candidate}: cannot inspect linked-order base target: {exc}") from exc
        if isinstance(candidate_data, dict) and candidate_data.get("name") == name:
            candidates.append(candidate)
    if not candidates:
        raise ValueError(
            f"{manifest_path}: linked_order_base_target not found in "
            f"{manifest_path.parent}: {name}"
        )
    if len(candidates) != 1:
        raise ValueError(
            f"{manifest_path}: duplicate linked_order_base_target name {name!r}: "
            + ", ".join(str(item) for item in candidates)
        )
    return load_manifest(candidates[0])


def is_linked_only_manifest_data(
    data: object,
    *,
    manifest_path: Path | None = None,
    target: VerifyTarget | None = None,
    manifests_by_name: dict[str, VerifyTarget] | None = None,
) -> bool:
    if not isinstance(data, dict):
        return False
    linked_intervals = data.get("linked_function_intervals", [])
    direct_linked_only = (
        isinstance(linked_intervals, list)
        and bool(linked_intervals)
        and not any(
            data.get(key)
            for key in ("functions", "data_symbols", "translation_unit_function_order")
        )
    )
    if target is None:
        return direct_linked_only
    if manifest_path is None or manifests_by_name is None:
        raise ValueError("parsed manifest context is required for diagnostic overlays")
    if not target.linked_order_base_target:
        return direct_linked_only

    unknown = sorted(set(data) - LINKED_ORDER_DIAGNOSTIC_OVERLAY_KEYS)
    if unknown:
        raise ValueError(
            f"{manifest_path}: unsupported linked-order diagnostic overlay keys: "
            + ", ".join(unknown)
        )
    base = manifests_by_name.get(target.linked_order_base_target)
    if base is None:
        base = discover_named_manifest(
            target.linked_order_base_target,
            manifest_path=manifest_path,
        )
    if base.linked_order_base_target or base.linked_order_diagnostic_mode.kind:
        raise ValueError(
            f"{manifest_path}: linked-order diagnostic overlays cannot inherit "
            "from another diagnostic overlay"
        )
    if not base.linked_function_intervals:
        raise ValueError(
            f"{manifest_path}: linked_order_base_target {base.name} "
            "has no linked_function_intervals"
        )
    if target.target_binary != base.target_binary:
        raise ValueError(
            f"{manifest_path}: diagnostic target_binary {target.target_binary!r} "
            f"does not match base target {base.target_binary!r}"
        )
    if target.compiler_profile and target.compiler_profile != base.compiler_profile:
        raise ValueError(
            f"{manifest_path}: diagnostic compiler_profile {target.compiler_profile!r} "
            f"does not match base target {base.compiler_profile!r}"
        )
    return True


def manifest_policy_debt(
    manifest_path: Path,
    *,
    target: VerifyTarget,
    manifests_by_name: dict[str, VerifyTarget],
) -> tuple[set[str], set[tuple[str, str]]]:
    inline_manifests: set[str] = set()
    generated_project_files: set[tuple[str, str]] = set()
    with manifest_path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    manifest_key = _repo_manifest_key_from_filesystem_path(manifest_path)
    generated_files = data.get("generated_files", {})
    if isinstance(generated_files, dict):
        for generated_path in generated_files:
            normalized = normalize_generated_path(generated_path)
            if generated_file_shadows_project(normalized):
                generated_project_files.add((manifest_key, normalized))
    if is_linked_only_manifest_data(
        data,
        manifest_path=manifest_path,
        target=target,
        manifests_by_name=manifests_by_name,
    ):
        return inline_manifests, generated_project_files
    if not data.get("source_from"):
        inline_manifests.add(manifest_key)
    return inline_manifests, generated_project_files


def actual_policy_debt(
    manifest_paths: list[Path],
    loaded_manifests: list[VerifyTarget],
) -> tuple[set[str], set[tuple[str, str]]]:
    inline_manifests: set[str] = set()
    generated_project_files: set[tuple[str, str]] = set()
    manifests_by_path = {
        target.manifest_path.resolve(): target
        for target in loaded_manifests
    }
    manifests_by_name = {target.name: target for target in loaded_manifests}
    for manifest_path in manifest_paths:
        target = manifests_by_path.get(manifest_path.resolve())
        if target is None:
            raise ValueError(f"{manifest_path}: parsed VC verification target is unavailable")
        inline_debt, generated_debt = manifest_policy_debt(
            manifest_path,
            target=target,
            manifests_by_name=manifests_by_name,
        )
        inline_manifests.update(inline_debt)
        generated_project_files.update(generated_debt)
    return inline_manifests, generated_project_files


def progress_physical_block_source_paths(progress_path: Path) -> set[str]:
    data = ProgressStore(progress_path).load().data
    blocks = data.get("physical_blocks", {}) if isinstance(data, dict) else {}
    if not isinstance(blocks, dict):
        raise ValueError(f"{progress_path}: physical_blocks must be an object")
    paths: set[str] = set()
    for block in blocks.values():
        if (
            not isinstance(block, dict)
            or block.get("binary") != "recoil"
            or block.get("row_kind") == "semantic-block"
        ):
            continue
        for key in ("agent_source_path", "original_source_path", "provisional_original_path", "source_path"):
            value = block.get(key)
            if not isinstance(value, str):
                continue
            path = value.split(":", 1)[0].replace("\\", "/")
            if path.startswith("src/") and Path(path).suffix.lower() in {".c", ".cpp"}:
                paths.add(normalized_repo_path(path))
    return paths


def final_build_source_debt(final_build_manifest: Path, progress_path: Path) -> set[str]:
    if not final_build_manifest.is_file():
        raise ValueError(f"{final_build_manifest}: final-build manifest does not exist")

    with final_build_manifest.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"{final_build_manifest}: final-build manifest root must be an object")

    sources_raw = data.get("sources", [])
    if not isinstance(sources_raw, list) or any(not isinstance(item, str) for item in sources_raw):
        raise ValueError(f"{final_build_manifest}: sources must be a list of strings")
    sources = {normalized_repo_path(item) for item in sources_raw}

    exclusions_raw = data.get("physical_block_source_exclusions", [])
    if not isinstance(exclusions_raw, list):
        raise ValueError(f"{final_build_manifest}: physical_block_source_exclusions must be a list")
    exclusions: set[str] = set()
    for index, item in enumerate(exclusions_raw):
        if not isinstance(item, dict):
            raise ValueError(f"{final_build_manifest}: physical_block_source_exclusions[{index}] must be an object")
        path = item.get("path")
        reason = str(item.get("reason", "")).strip()
        if not isinstance(path, str) or not path:
            raise ValueError(f"{final_build_manifest}: physical_block_source_exclusions[{index}].path must be a string")
        if not reason:
            raise ValueError(f"{final_build_manifest}: physical_block_source_exclusions[{index}].reason is required")
        exclusions.add(normalized_repo_path(path))

    return progress_physical_block_source_paths(progress_path) - sources - exclusions


def routed_progress_authority(explicit_progress: str | Path | None) -> Path:
    """Route live SQLite state canonically while tracked inputs remain linked."""

    canonical_text = os.environ.get(CANONICAL_ROOT_ENV)
    if not canonical_text:
        return Path(explicit_progress or DEFAULT_PROGRESS)
    canonical = resolve_canonical_control_root(
        executing_worktree_root=REPO_ROOT,
        required_machine_local_paths=(
            ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
        ),
        explicit_root=Path(canonical_text),
    )
    expected = (
        canonical.canonical_control_root
        / ".agent"
        / "RECONSTRUCTION_PROGRESS.sqlite3"
    ).resolve(strict=True)
    if explicit_progress is not None:
        supplied = Path(explicit_progress).resolve(strict=True)
        if supplied != expected:
            raise WorktreeControlError(
                "VC manifest source policy progress input does not equal the "
                f"authenticated canonical authority: {supplied} != {expected}"
            )
    return expected


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Reject new VC manifest-local source bodies and project-header shadows."
    )
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--path", help="validate one VC manifest JSON path instead of the manifest directory")
    parser.add_argument("--final-build-manifest", default=str(DEFAULT_FINAL_BUILD_MANIFEST))
    parser.add_argument("--progress")
    parser.add_argument(
        "--skip-final-build-source-audit",
        action="store_true",
        help="skip unified physical-block source coverage check for the final Recoil.exe build manifest",
    )
    parser.add_argument(
        "--strict-source-emissions",
        action="store_true",
        help=(
            "require compiler-generated authored-order rows to carry valid source-anchored "
            "emission_anchor metadata; validate canonical or supplied legacy emission "
            "markers when present"
        ),
    )
    parser.add_argument(
        "--strict-source-traceability",
        action="store_true",
        help=(
            "require canonical direct/emitted source-trace relationships for resolved "
            "authored VC rows; repository-wide graph enforcement remains the migrated "
            "source-trace audit"
        ),
    )
    args = parser.parse_args(argv)

    canonical_routing = bool(os.environ.get(CANONICAL_ROOT_ENV))
    try:
        progress_path = routed_progress_authority(args.progress)
    except (OSError, WorktreeControlError) as exc:
        print(exc, file=sys.stderr)
        return 1
    if canonical_routing or args.progress is not None:
        from _recoil.commands.pipeline_reachability_audit import _bound_vc5_tracker

        with _bound_vc5_tracker(progress_path):
            return _run_guard(args, progress_path=progress_path)
    return _run_guard(args, progress_path=progress_path)


def _run_guard(args: argparse.Namespace, *, progress_path: Path) -> int:

    manifest_dir = Path(args.manifest_dir)
    manifest_path = Path(args.path) if args.path else None

    try:
        load_options = {
            "strict_source_emissions": bool(args.strict_source_emissions),
            "strict_source_traceability": bool(args.strict_source_traceability),
        }
        if manifest_path is not None:
            if not manifest_path.is_file():
                raise ValueError(f"{manifest_path}: manifest path does not exist or is not a file")
            loaded_manifests = [load_manifest(manifest_path, **load_options)]
            manifest_paths = [manifest_path]
        else:
            loaded_manifests = load_manifests(manifest_dir, **load_options)
            manifest_paths = sorted(manifest_dir.glob("*.json"))
    except ValueError as exc:
        prefix = (
            "strict-source-emission-debt: "
            if args.strict_source_emissions
            else (
                "strict-source-traceability-debt: "
                if args.strict_source_traceability
                else ""
            )
        )
        print(f"{prefix}{exc}", file=sys.stderr)
        return 1

    print_source_emission_warnings(loaded_manifests)
    source_emission_warnings = [
        (target, warning)
        for target in loaded_manifests
        for warning in target.source_emission_warnings
    ]
    source_fragment_findings = [
        (target, finding)
        for target in loaded_manifests
        for finding in target_source_fragment_findings(target)
    ]

    try:
        actual_inline, actual_generated = actual_policy_debt(
            manifest_paths,
            loaded_manifests,
        )
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(exc, file=sys.stderr)
        return 1
    provider_paths = list(manifest_paths)
    final_build_manifest = Path(args.final_build_manifest)
    if final_build_manifest.is_file():
        provider_paths.append(final_build_manifest)
    actual_mfc_path_debt = active_mfc_path_debt(provider_paths)
    missing_final_sources: set[str] = set()
    try:
        if manifest_path is None and not args.skip_final_build_source_audit:
            missing_final_sources = final_build_source_debt(
                Path(args.final_build_manifest),
                progress_path,
            )
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(exc, file=sys.stderr)
        return 1

    if (
        actual_inline
        or actual_generated
        or missing_final_sources
        or actual_mfc_path_debt
        or source_fragment_findings
    ):
        for item in sorted(actual_inline):
            print(f"inline-manifest-source: {item}", file=sys.stderr)
        for manifest, generated in sorted(actual_generated):
            print(f"generated-project-header-shadow: {manifest}: {generated}", file=sys.stderr)
        for item in sorted(missing_final_sources):
            print(f"final-build-source-missing: {item}", file=sys.stderr)
        for manifest, path in sorted(actual_mfc_path_debt):
            print(f"forbidden-active-mfc-path: {manifest}: {path}", file=sys.stderr)
        for target, finding in source_fragment_findings:
            location = finding.get("path") or (
                f"{finding.get('source')}:{finding.get('line')} -> {finding.get('target')}"
            )
            print(
                f"vc5-source-fragment-{finding['kind']}: "
                f"{_repo_manifest_key_from_filesystem_path(target.manifest_path)}: {location}",
                file=sys.stderr,
            )
        return 1

    print(
        f"VC manifest source policy OK: {len(actual_inline)} inline manifest(s), "
        f"{len(actual_generated)} generated project-header shadow(s), "
        f"{len(missing_final_sources)} missing final-build source(s), "
        f"{len(actual_mfc_path_debt)} forbidden active MFC path(s), "
        f"{len(source_fragment_findings)} source-fragment finding(s), "
        f"{len(source_emission_warnings)} source-emission warning(s)."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
