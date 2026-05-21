#!/usr/bin/env python3
"""Guard VC verification manifests against production-source drift."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

from recoil_vc6_verify import (
    DEFAULT_MANIFEST_DIR,
    DEFAULT_SOURCE_POLICY_BASELINE,
    generated_file_shadows_project,
    load_manifests,
    normalize_generated_path,
    read_source_policy_baseline,
    repo_manifest_key,
)


def actual_policy_debt(manifest_dir: Path) -> tuple[set[str], set[tuple[str, str]]]:
    inline_manifests: set[str] = set()
    generated_project_files: set[tuple[str, str]] = set()
    for manifest_path in sorted(manifest_dir.glob("*.json")):
        with manifest_path.open("r", encoding="utf-8") as handle:
            data = json.load(handle)
        manifest_key = repo_manifest_key(manifest_path)
        if not data.get("source_from"):
            inline_manifests.add(manifest_key)
        generated_files = data.get("generated_files", {})
        if not isinstance(generated_files, dict):
            continue
        for generated_path in generated_files:
            normalized = normalize_generated_path(generated_path)
            if generated_file_shadows_project(normalized):
                generated_project_files.add((manifest_key, normalized))
    return inline_manifests, generated_project_files


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Reject new VC manifest-local source bodies and project-header shadows."
    )
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--baseline", default=str(DEFAULT_SOURCE_POLICY_BASELINE))
    args = parser.parse_args(argv)

    manifest_dir = Path(args.manifest_dir)
    baseline_path = Path(args.baseline)
    baseline = read_source_policy_baseline(baseline_path)

    try:
        load_manifests(manifest_dir, source_policy_baseline=baseline)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1

    actual_inline, actual_generated = actual_policy_debt(manifest_dir)
    stale_inline = sorted(baseline.inline_manifests - actual_inline)
    stale_generated = sorted(baseline.generated_project_files - actual_generated)
    if stale_inline or stale_generated:
        for manifest in stale_inline:
            print(f"stale baseline entry: inline {manifest}", file=sys.stderr)
        for manifest, generated_path in stale_generated:
            print(f"stale baseline entry: generated {manifest} {generated_path}", file=sys.stderr)
        return 1

    print(
        f"VC manifest source policy OK: {len(actual_inline)} legacy inline manifest(s), "
        f"{len(actual_generated)} generated project-header shadow(s) baselined."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
