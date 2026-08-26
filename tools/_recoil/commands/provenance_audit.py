#!/usr/bin/env python3
"""Audit compiler/linker provenance profiles used by VC verification tooling."""

from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse

from _recoil.commands.vc5_manifest_source_guard import (
    CANONICAL_MFC_INCLUDE,
    CANONICAL_MFC_LIB,
    FORBIDDEN_ACTIVE_MFC_PATHS,
    is_linked_only_manifest_data,
    normalized_repo_path,
)
from _recoil.lib.profiles import (
    VerificationProfile as Profile,
    load_json_object as load_json,
    load_profiles,
    require_string,
    require_string_list,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


DEFAULT_PROFILES = REPO_ROOT / "tools" / "_recoil" / "config" / "compiler_linker_profiles.json"
DEFAULT_FINAL_BUILD = REPO_ROOT / "tools" / "_recoil" / "config" / "vc5_final_build.json"
DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "vc5_verify_targets"


def compare_tuple(
    mismatches: list[str],
    label: str,
    actual: tuple[str, ...],
    expected: tuple[str, ...],
) -> None:
    if actual != expected:
        mismatches.append(
            f"{label}: expected {' '.join(expected) or '-'}; got {' '.join(actual) or '-'}"
        )


def audit_final_build(final_profile: dict[str, Any], final_build_path: Path) -> list[str]:
    final_build = load_json(final_build_path)
    mismatches: list[str] = []
    expected_env = require_string(final_profile, "compiler_env", path=DEFAULT_PROFILES)
    actual_env = require_string(final_build, "vc5_env", path=final_build_path)
    if actual_env != expected_env:
        mismatches.append(f"final vc5_env: expected {expected_env}; got {actual_env}")

    compare_tuple(
        mismatches,
        "final compile_flags",
        require_string_list(final_build, "compile_flags", path=final_build_path),
        require_string_list(final_profile, "compile_flags", path=DEFAULT_PROFILES),
    )
    compare_tuple(
        mismatches,
        "final resource_flags",
        require_string_list(final_build, "resource_flags", path=final_build_path),
        require_string_list(final_profile, "resource_flags", path=DEFAULT_PROFILES),
    )
    compare_tuple(
        mismatches,
        "final link_flags",
        require_string_list(final_build, "link_flags", path=final_build_path),
        require_string_list(final_profile, "link_flags", path=DEFAULT_PROFILES),
    )
    include_dirs = tuple(normalized_repo_path(item).rstrip("/") for item in require_string_list(final_build, "include_dirs", path=final_build_path))
    lib_dirs = tuple(normalized_repo_path(item).rstrip("/") for item in require_string_list(final_build, "lib_dirs", path=final_build_path))
    if CANONICAL_MFC_INCLUDE not in include_dirs:
        mismatches.append(f"final include_dirs omit canonical VC5SP3 MFC root {CANONICAL_MFC_INCLUDE}")
    if CANONICAL_MFC_LIB not in lib_dirs:
        mismatches.append(f"final lib_dirs omit canonical VC5SP3 MFC root {CANONICAL_MFC_LIB}")
    forbidden = sorted((set(include_dirs) | set(lib_dirs)) & FORBIDDEN_ACTIVE_MFC_PATHS)
    if forbidden:
        mismatches.append("final build contains forbidden active MFC paths: " + ", ".join(forbidden))
    canonical = final_build.get("canonical_mfc")
    if not isinstance(canonical, dict):
        mismatches.append("final build omits canonical_mfc provider contract")
    else:
        actual_include = normalized_repo_path(str(canonical.get("include_root", ""))).rstrip("/")
        actual_lib = normalized_repo_path(str(canonical.get("lib_root", ""))).rstrip("/")
        if actual_include != CANONICAL_MFC_INCLUDE or actual_lib != CANONICAL_MFC_LIB:
            mismatches.append(
                "final canonical_mfc roots do not match the VC5SP3 provider: "
                f"include={actual_include or '-'} lib={actual_lib or '-'}"
            )
    return mismatches


def audit_diagnostic_profile_extensions(data: dict[str, Any]) -> list[str]:
    mismatches: list[str] = []
    alternate_include = "d:/recoil project/visual c++ 5.0/devstudio/vc/mfc/include"
    profiles = data.get("verification_profiles", [])
    if not isinstance(profiles, list):
        return ["verification_profiles must be a list"]
    for row in profiles:
        if not isinstance(row, dict):
            continue
        flags = row.get("final_build_compile_flags")
        if flags is None:
            continue
        name = str(row.get("name", "<unnamed>"))
        if not isinstance(flags, list) or not all(isinstance(item, str) and item for item in flags):
            mismatches.append(f"{name}: final_build_compile_flags must be a non-empty string list")
            continue
        upper = [item.upper() for item in flags]
        if any(item == "/FACS" or item == "/I" or item.startswith("/I.") for item in upper):
            mismatches.append(f"{name}: final_build_compile_flags contains verification-only listing/include flags")
        if any(alternate_include in normalized_repo_path(item) for item in flags):
            mismatches.append(f"{name}: final_build_compile_flags activates alternate Visual C++ MFC headers")
    libraries = data.get("diagnostic_library_profiles", [])
    if not isinstance(libraries, list):
        mismatches.append("diagnostic_library_profiles must be a list")
    else:
        for row in libraries:
            if not isinstance(row, dict):
                continue
            if row.get("acceptance_eligible") is not False:
                mismatches.append(f"{row.get('name', '<unnamed>')}: diagnostic library profile must be acceptance_eligible=false")
            if any("/include" in normalized_repo_path(value) for value in row.get("libraries", []) if isinstance(value, str)):
                mismatches.append(f"{row.get('name', '<unnamed>')}: diagnostic library profile must not supply headers")
    return mismatches


def manifest_profile_key(
    manifest: dict[str, object],
    profiles_by_name: dict[str, Profile],
    default_compiler_env: str,
) -> tuple[str, tuple[str, ...], str]:
    profile_name = manifest.get("compiler_profile")
    has_raw_env = "compiler_env" in manifest
    has_raw_flags = "compiler_flags" in manifest
    if profile_name is not None:
        if not isinstance(profile_name, str) or not profile_name:
            raise ValueError("expected compiler_profile to be a non-empty string")
        if has_raw_env or has_raw_flags:
            raise ValueError("compiler_profile is mutually exclusive with compiler_env/compiler_flags")
        profile = profiles_by_name.get(profile_name)
        if profile is None:
            raise ValueError(f"unknown compiler_profile {profile_name}")
        return profile.compiler_env, profile.compiler_flags, profile.name

    env = manifest.get("compiler_env") or default_compiler_env
    if not isinstance(env, str):
        raise ValueError("expected compiler_env to be a string")
    flags = manifest.get("compiler_flags")
    if not isinstance(flags, list) or not all(isinstance(item, str) and item for item in flags):
        raise ValueError("expected compiler_flags to be a non-empty string list")
    return env, tuple(flags), ""


def audit_manifests(
    manifest_dir: Path,
    profiles: list[Profile],
    *,
    final_profile: dict[str, Any] | None = None,
    compile_context_root: Path = REPO_ROOT,
) -> tuple[list[str], dict[str, int]]:
    profile_by_key = {profile.key: profile for profile in profiles}
    profile_by_name = {profile.name: profile for profile in profiles}
    counts = {profile.name: 0 for profile in profiles}
    default_compiler_env = profiles[0].compiler_env if profiles else r"D:\Recoil Project\Compiler\VC5SP3/vc5sp3-env.cmd"
    mismatches: list[str] = []

    for manifest_path in sorted(manifest_dir.glob("*.json")):
        try:
            manifest = load_json(manifest_path)
            include_dirs = manifest.get("include_dirs", [])
            if isinstance(include_dirs, list):
                forbidden = sorted(
                    normalized_repo_path(item).rstrip("/")
                    for item in include_dirs
                    if isinstance(item, str)
                    and normalized_repo_path(item).rstrip("/") in FORBIDDEN_ACTIVE_MFC_PATHS
                )
                if forbidden:
                    raise ValueError("forbidden active MFC include path(s): " + ", ".join(forbidden))
            if is_linked_only_manifest_data(manifest):
                continue
            compile_context_from = manifest.get("compile_context_from")
            if compile_context_from is not None:
                if not isinstance(compile_context_from, str) or not compile_context_from:
                    raise ValueError("expected compile_context_from to be a non-empty string")
                if any(key in manifest for key in ("compiler_profile", "compiler_env", "compiler_flags")):
                    raise ValueError(
                        "compile_context_from is mutually exclusive with compiler_profile/compiler_env/compiler_flags"
                    )
                context_path = Path(compile_context_from)
                if not context_path.is_absolute():
                    context_path = compile_context_root / context_path
                context = load_json(context_path)
                context_env = require_string(context, "vc5_env", path=context_path)
                context_flags = require_string_list(context, "compile_flags", path=context_path)
                require_string_list(context, "defines", path=context_path)
                require_string_list(context, "include_dirs", path=context_path)
                if final_profile is not None:
                    expected_env = require_string(final_profile, "compiler_env", path=DEFAULT_PROFILES)
                    expected_flags = require_string_list(final_profile, "compile_flags", path=DEFAULT_PROFILES)
                    if context_env != expected_env or context_flags != expected_flags:
                        raise ValueError(
                            "compile_context_from does not match the documented final-build compiler context"
                        )
                counts["final-build-context"] = counts.get("final-build-context", 0) + 1
                continue
            env, flags, explicit_profile_name = manifest_profile_key(
                manifest,
                profile_by_name,
                default_compiler_env,
            )
        except ValueError as exc:
            mismatches.append(f"{display_path(manifest_path)}: {exc}")
            continue
        if explicit_profile_name:
            counts[explicit_profile_name] += 1
            continue
        profile = profile_by_key.get((env, flags))
        if profile is None:
            mismatches.append(
                f"{display_path(manifest_path)}: undocumented compiler profile "
                f"env={env} flags={' '.join(flags)}"
            )
            continue
        counts[profile.name] += 1

    return mismatches, counts


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit documented compiler/linker profiles against VC5SP3 build manifests."
    )
    parser.add_argument("--profiles", default=str(DEFAULT_PROFILES))
    parser.add_argument("--final-build", default=str(DEFAULT_FINAL_BUILD))
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--summary", action="store_true", help="Print accepted profile usage counts.")
    parser.add_argument("--strict", action="store_true", help="Return nonzero on undocumented drift.")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)

    profiles_path = Path(args.profiles)
    final_build_path = Path(args.final_build)
    manifest_dir = Path(args.manifest_dir)

    profiles_data = load_json(profiles_path)
    final_profile, profiles = load_profiles(profiles_path)
    final_mismatches = audit_final_build(final_profile, final_build_path)
    manifest_mismatches, counts = audit_manifests(
        manifest_dir,
        profiles,
        final_profile=final_profile,
    )
    mismatches = final_mismatches + audit_diagnostic_profile_extensions(profiles_data) + manifest_mismatches

    if args.summary:
        print(f"verification profiles: {len(profiles)}")
        for profile in profiles:
            print(f"- {profile.name}: {counts[profile.name]}")
    if mismatches:
        print("Compiler/linker provenance drift:")
        for mismatch in mismatches:
            print(f"- {mismatch}")
    else:
        print(
            "Compiler/linker provenance OK: "
            f"{len(profiles)} verification profile(s), {sum(counts.values())} manifest(s)."
        )

    return 1 if args.strict and mismatches else 0


if __name__ == "__main__":
    raise SystemExit(main())
