from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
from typing import Any

from recoil_tooling import REPO_ROOT


DEFAULT_PROFILES = REPO_ROOT / "tools" / "compiler_linker_profiles.json"


@dataclass(frozen=True)
class VerificationProfile:
    name: str
    description: str
    compiler_env: str
    compiler_version_prefix: str
    compiler_flags: tuple[str, ...]

    @property
    def key(self) -> tuple[str, tuple[str, ...]]:
        return (self.compiler_env, self.compiler_flags)


def load_json_object(path: Path) -> dict[str, Any]:
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


def load_profiles(path: Path = DEFAULT_PROFILES) -> tuple[dict[str, Any], list[VerificationProfile]]:
    data = load_json_object(path)
    final_build = data.get("final_build")
    if not isinstance(final_build, dict):
        raise ValueError(f"{path}: expected object field 'final_build'")

    raw_profiles = data.get("verification_profiles")
    if not isinstance(raw_profiles, list):
        raise ValueError(f"{path}: expected list field 'verification_profiles'")

    profiles: list[VerificationProfile] = []
    seen_names: set[str] = set()
    seen_keys: set[tuple[str, tuple[str, ...]]] = set()
    for index, item in enumerate(raw_profiles):
        if not isinstance(item, dict):
            raise ValueError(f"{path}: verification_profiles[{index}] is not an object")
        profile = VerificationProfile(
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


def profiles_by_name(path: Path = DEFAULT_PROFILES) -> dict[str, VerificationProfile]:
    _final_build, profiles = load_profiles(path)
    return {profile.name: profile for profile in profiles}

