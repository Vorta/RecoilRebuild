#!/usr/bin/env python3
"""Audit compiler/linker provenance profiles used by VC verification tooling."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import sys
from typing import Any

from recoil_tooling import REPO_ROOT, configure_stdio, display_path


DEFAULT_PROFILES = REPO_ROOT / "tools" / "compiler_linker_profiles.json"
DEFAULT_FINAL_BUILD = REPO_ROOT / "tools" / "vc6_final_build.json"
DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "vc6_verify_targets"


@dataclass(frozen=True)
class Profile:
    name: str
    description: str
    compiler_env: str
    compiler_version_prefix: str
    compiler_flags: tuple[str, ...]

    @property
    def key(self) -> tuple[str, tuple[str, ...]]:
        return (self.compiler_env, self.compiler_flags)


def load_json(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"{path}: expected JSON object")
    return data


def require_string(data: dict[str, Any], key: str, *, path: Path) -> str:
    value = data.get(key)
    if not isinstance(value, str) or not value:
        raise ValueError(f"{path}: expected non-empty string field '{key}'")
    return value


def require_string_list(data: dict[str, Any], key: str, *, path: Path) -> tuple[str, ...]:
    value = data.get(key)
    if not isinstance(value, list) or not all(isinstance(item, str) and item for item in value):
        raise ValueError(f"{path}: expected non-empty string list field '{key}'")
    return tuple(value)


def load_profiles(path: Path) -> tuple[dict[str, Any], list[Profile]]:
    data = load_json(path)
    final_build = data.get("final_build")
    if not isinstance(final_build, dict):
        raise ValueError(f"{path}: expected object field 'final_build'")

    raw_profiles = data.get("verification_profiles")
    if not isinstance(raw_profiles, list):
        raise ValueError(f"{path}: expected list field 'verification_profiles'")

    profiles: list[Profile] = []
    seen_names: set[str] = set()
    seen_keys: set[tuple[str, tuple[str, ...]]] = set()
    for index, item in enumerate(raw_profiles):
        if not isinstance(item, dict):
            raise ValueError(f"{path}: verification_profiles[{index}] is not an object")
        profile = Profile(
            name=require_string(item, "name", path=path),
            description=require_string(item, "description", path=path),
            compiler_env=require_string(item, "compiler_env", path=path),
            compiler_version_prefix=require_string(item, "compiler_version_prefix", path=path),
            compiler_flags=require_string_list(item, "compiler_flags", path=path),
        )
        if profile.name in seen_names:
            raise ValueError(f"{path}: duplicate profile name {profile.name}")
        if profile.key in seen_keys:
            raise ValueError(f"{path}: duplicate profile tuple {profile.name}")
        seen_names.add(profile.name)
        seen_keys.add(profile.key)
        profiles.append(profile)

    return final_build, profiles


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


def manifest_profile_key(manifest: dict[str, Any]) -> tuple[str, tuple[str, ...]]:
    env = manifest.get("compiler_env") or "${RECOIL_VC6_ROOT}/vc6-env.cmd"
    if not isinstance(env, str):
        raise ValueError("expected compiler_env to be a string")
    flags = manifest.get("compiler_flags")
    if not isinstance(flags, list) or not all(isinstance(item, str) and item for item in flags):
        raise ValueError("expected compiler_flags to be a non-empty string list")
    return env, tuple(flags)


def audit_manifests(manifest_dir: Path, profiles: list[Profile]) -> tuple[list[str], dict[str, int]]:
    profile_by_key = {profile.key: profile for profile in profiles}
    counts = {profile.name: 0 for profile in profiles}
    mismatches: list[str] = []

    for manifest_path in sorted(manifest_dir.glob("*.json")):
        try:
            manifest = load_json(manifest_path)
            key = manifest_profile_key(manifest)
        except ValueError as exc:
            mismatches.append(f"{display_path(manifest_path)}: {exc}")
            continue
        profile = profile_by_key.get(key)
        if profile is None:
            env, flags = key
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
