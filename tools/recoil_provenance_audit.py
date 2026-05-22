#!/usr/bin/env python3
"""Audit compiler/linker provenance profiles used by VC verification tooling."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

from recoil_profiles import (
    VerificationProfile as Profile,
    load_json_object as load_json,
    load_profiles,
    require_string,
    require_string_list,
)
from recoil_tooling import REPO_ROOT, configure_stdio, display_path


DEFAULT_PROFILES = REPO_ROOT / "tools" / "compiler_linker_profiles.json"
DEFAULT_FINAL_BUILD = REPO_ROOT / "tools" / "vc6_final_build.json"
DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "vc6_verify_targets"


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
    actual_env = require_string(final_build, "vc6_env", path=final_build_path)
    if actual_env != expected_env:
        mismatches.append(f"final vc6_env: expected {expected_env}; got {actual_env}")

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


def audit_manifests(manifest_dir: Path, profiles: list[Profile]) -> tuple[list[str], dict[str, int]]:
    profile_by_key = {profile.key: profile for profile in profiles}
    profile_by_name = {profile.name: profile for profile in profiles}
    counts = {profile.name: 0 for profile in profiles}
    default_compiler_env = profiles[0].compiler_env if profiles else r"D:\Recoil Project\Compiler\VC6/vc6-env.cmd"
    mismatches: list[str] = []

    for manifest_path in sorted(manifest_dir.glob("*.json")):
        try:
            manifest = load_json(manifest_path)
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
        description="Audit documented compiler/linker profiles against VC6 build manifests."
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

    final_profile, profiles = load_profiles(profiles_path)
    final_mismatches = audit_final_build(final_profile, final_build_path)
    manifest_mismatches, counts = audit_manifests(manifest_dir, profiles)
    mismatches = final_mismatches + manifest_mismatches

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
