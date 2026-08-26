#!/usr/bin/env python3
"""Detect generated compiler/IDE artifacts outside approved local output roots."""

from __future__ import annotations

import json
import os
import re
import sys
from pathlib import Path, PurePosixPath

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse

from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


DEFAULT_ALLOWED_ROOTS = {
    ".git",
    ".vs",
    "_scratch",
    "artifacts",
    "build",
    "Debug",
    "DebugPublic",
    "Release",
    "Releases",
    "support",
    "temp",
    "x64",
    "x86",
}

ARTIFACT_SUFFIXES = {
    ".aps",
    ".binlog",
    ".cache",
    ".cod",
    ".coverage",
    ".coveragexml",
    ".exp",
    ".iobj",
    ".ilk",
    ".ipdb",
    ".lib",
    ".map",
    ".meta",
    ".ncb",
    ".obj",
    ".opendb",
    ".opensdf",
    ".pch",
    ".pdb",
    ".pgc",
    ".pgd",
    ".plg",
    ".res",
    ".rsp",
    ".sbr",
    ".sdf",
    ".tlb",
    ".tli",
    ".tlh",
    ".tmp_proj",
}

ARTIFACT_NAMES = {
    "NUL",
    "NUL.obj",
    "NUL.pdb",
}

DEVSPACE_MANIFEST_NAME = ".recoil-workspace-hygiene.json"
DEVSPACE_MANIFEST_SCHEMA = "recoil-governed-session-scratch-v1"


def is_upgrade_log(path: Path) -> bool:
    name = path.name.lower()
    return name.startswith("upgradelog") and name.endswith((".htm", ".html", ".xml"))


def is_generated_artifact(path: Path) -> bool:
    if path.name in ARTIFACT_NAMES:
        return True
    if path.suffix.lower() in ARTIFACT_SUFFIXES:
        return True
    return is_upgrade_log(path)


def is_under_allowed_root(path: Path, root: Path, allowed_roots: set[str]) -> bool:
    try:
        relative = path.relative_to(root)
    except ValueError:
        return False
    if not relative.parts:
        return False
    return relative.parts[0] in allowed_roots


def _resolved_path_key(path: Path) -> str:
    """Return a normalized key suitable for exact resolved-path comparisons."""

    return os.path.normcase(str(path.resolve()))


def _is_reparse_point(path: Path) -> bool:
    """Return true for symlinks and Windows junction/other reparse entries."""

    if path.is_symlink():
        return True
    try:
        attributes = path.stat(follow_symlinks=False).st_file_attributes
    except (AttributeError, OSError):
        return False
    return bool(attributes & 0x400)


def _normalized_session_relative_path(value: object) -> str | None:
    if not isinstance(value, str) or not value:
        return None
    raw = value.replace("\\", "/")
    path = Path(raw)
    if path.is_absolute() or re.match(r"^[A-Za-z]:/", raw):
        return None
    parts = PurePosixPath(raw).parts
    if not parts or any(part in {"", ".", ".."} for part in parts):
        return None
    normalized = PurePosixPath(*parts).as_posix()
    return normalized if normalized == raw else None


def governed_devspace_files(root: Path) -> tuple[set[str], list[Path]]:
    """Validate exact per-session scratch manifests and return allowed file keys.

    ``.devspace`` itself is never an allowed output root.  Only generated files
    named by a valid manifest immediately below one direct session child are
    exempted, and the manifest is rejected as an offender when any part of the
    declaration is malformed or no longer matches the filesystem.
    """

    devspace = root / ".devspace"
    if not devspace.is_dir() or _is_reparse_point(devspace):
        return set(), ([devspace] if devspace.exists() and _is_reparse_point(devspace) else [])

    allowed: set[str] = set()
    invalid: list[Path] = []
    try:
        children = sorted(devspace.iterdir(), key=lambda item: item.name.casefold())
    except OSError:
        return set(), [devspace]
    for session_root in children:
        if not session_root.is_dir() or _is_reparse_point(session_root):
            if session_root.exists() and _is_reparse_point(session_root):
                invalid.append(session_root)
            continue
        manifest_path = session_root / DEVSPACE_MANIFEST_NAME
        if not manifest_path.is_file() or _is_reparse_point(manifest_path):
            continue
        try:
            payload = json.loads(manifest_path.read_text(encoding="utf-8"))
        except (OSError, UnicodeError, json.JSONDecodeError):
            invalid.append(manifest_path)
            continue
        if (
            not isinstance(payload, dict)
            or set(payload)
            != {"schema", "status", "repo_root", "purpose", "artifacts"}
            or payload.get("schema") != DEVSPACE_MANIFEST_SCHEMA
            or payload.get("status") != "complete"
            or payload.get("purpose") != "compiler-probe"
            or not isinstance(payload.get("repo_root"), str)
            or not Path(payload["repo_root"]).is_absolute()
            or _resolved_path_key(Path(payload["repo_root"])) != _resolved_path_key(root)
            or not isinstance(payload.get("artifacts"), list)
        ):
            invalid.append(manifest_path)
            continue

        session_allowed: set[str] = set()
        valid = True
        for entry in payload["artifacts"]:
            if (
                not isinstance(entry, dict)
                or set(entry) != {"path", "bytes"}
                or isinstance(entry.get("bytes"), bool)
                or not isinstance(entry.get("bytes"), int)
                or entry["bytes"] < 0
            ):
                valid = False
                break
            relative = _normalized_session_relative_path(entry.get("path"))
            if relative is None or relative in session_allowed:
                valid = False
                break
            candidate = session_root / Path(*PurePosixPath(relative).parts)
            try:
                candidate.resolve().relative_to(session_root.resolve())
            except (OSError, ValueError):
                valid = False
                break
            if (
                not candidate.is_file()
                or _is_reparse_point(candidate)
                or candidate.stat().st_size != entry["bytes"]
                or not is_generated_artifact(candidate)
            ):
                valid = False
                break
            session_allowed.add(relative)
        if not valid:
            invalid.append(manifest_path)
            continue

        actual_generated: set[str] = set()
        try:
            for candidate in session_root.rglob("*"):
                if not candidate.is_file() or candidate == manifest_path:
                    continue
                if _is_reparse_point(candidate):
                    valid = False
                    break
                if is_generated_artifact(candidate):
                    actual_generated.add(candidate.relative_to(session_root).as_posix())
        except OSError:
            valid = False
        if not valid or actual_generated != session_allowed:
            invalid.append(manifest_path)
            continue
        allowed.update(
            _resolved_path_key(session_root / Path(*PurePosixPath(relative).parts))
            for relative in session_allowed
        )
    return allowed, invalid


def is_staged_chatgpt_upload(path: Path, root: Path) -> bool:
    """Recognize an exact, successfully staged ChatGPT Pro upload artifact.

    This is deliberately fail-closed.  It does not allow ``.devspace`` as a
    general output root: repository identity, staged path, upload status, file
    size, and the configured original path/size must agree with run metadata.
    A later response failure is irrelevant once the upload itself succeeded.
    """

    try:
        relative = path.relative_to(root)
    except ValueError:
        return False
    if len(relative.parts) < 5 or relative.parts[:2] != (".devspace", "runs"):
        return False

    run_root = root / relative.parts[0] / relative.parts[1] / relative.parts[2]
    uploads_root = run_root / "uploads"
    try:
        path.resolve().relative_to(uploads_root.resolve())
    except ValueError:
        return False

    run_metadata_path = run_root / "receipt.json"  # external ChatGPT transport schema
    try:
        run_metadata = json.loads(run_metadata_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError):
        return False
    if not isinstance(run_metadata, dict):
        return False

    project = run_metadata.get("project")
    if not isinstance(project, dict):
        return False
    metadata_repo_root = project.get("repoRoot")
    if not isinstance(metadata_repo_root, str) or not Path(metadata_repo_root).is_absolute():
        return False
    if _resolved_path_key(Path(metadata_repo_root)) != _resolved_path_key(root):
        return False

    upload = run_metadata.get("upload")
    if not isinstance(upload, dict) or upload.get("ok") is not True:
        return False
    files = upload.get("files")
    if not isinstance(files, list):
        return False

    candidate_key = _resolved_path_key(path)
    try:
        candidate_size = path.stat().st_size
    except OSError:
        return False

    for entry in files:
        if not isinstance(entry, dict) or entry.get("staged") is not True:
            continue
        entry_path = entry.get("path")
        if not isinstance(entry_path, str) or not Path(entry_path).is_absolute():
            continue
        if _resolved_path_key(Path(entry_path)) != candidate_key:
            continue
        if entry.get("bytes") != candidate_size:
            continue
        original = entry.get("original")
        if not isinstance(original, dict):
            continue
        original_path = original.get("path")
        original_size = original.get("bytes")
        if not isinstance(original_path, str) or not Path(original_path).is_absolute():
            continue
        source = Path(original_path)
        try:
            source.resolve().relative_to(root.resolve())
            live_original_size = source.stat().st_size
        except (ValueError, OSError):
            continue
        if original_size != live_original_size:
            continue
        return True
    return False


def find_offenders(root: Path, allowed_roots: set[str]) -> list[Path]:
    governed_files, invalid_manifests = governed_devspace_files(root)
    offenders: list[Path] = list(invalid_manifests)
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if is_under_allowed_root(path, root, allowed_roots):
            continue
        if is_generated_artifact(path):
            if _resolved_path_key(path) in governed_files:
                continue
            if is_staged_chatgpt_upload(path, root):
                continue
            offenders.append(path)
    return sorted(set(offenders))


def parse_allowed_roots(values: list[str]) -> set[str]:
    result = set(DEFAULT_ALLOWED_ROOTS)
    result.update(value.strip("/\\") for value in values if value.strip("/\\"))
    return result


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Reject generated compiler/IDE artifacts outside approved output roots."
    )
    parser.add_argument("--root", default=str(REPO_ROOT), help="Repository root to scan.")
    parser.add_argument(
        "--allow-root",
        action="append",
        default=[],
        help="Additional top-level directory allowed to contain generated artifacts.",
    )
    parser.add_argument("--summary", action="store_true", help="Print a compact summary.")
    parser.add_argument("--strict", action="store_true", help="Return nonzero when artifacts are found.")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    allowed_roots = parse_allowed_roots(args.allow_root)
    offenders = find_offenders(root, allowed_roots)

    if args.summary:
        print(f"workspace hygiene offenders: {len(offenders)}")
    if offenders:
        print("Generated artifacts outside approved output roots:")
        for path in offenders:
            print(f"- {display_path(path, root)}")
    else:
        print("Workspace hygiene OK: no generated compiler/IDE artifacts outside approved output roots.")

    return 1 if args.strict and offenders else 0


if __name__ == "__main__":
    raise SystemExit(main())
