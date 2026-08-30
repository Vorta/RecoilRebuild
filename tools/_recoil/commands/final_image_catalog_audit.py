from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any, Mapping

from _recoil.commands.live_final_verify import (
    DEFAULT_REFERENCE,
    DEFAULT_TRACKER,
    _validate_catalog,
    semantic_projection,
)
from _recoil.commands.final_image_coverage import (
    coverage_summary,
    derive_final_image_coverage,
)
from _recoil.lib.progress import SCHEMA_VERSION, ProgressError, ProgressStore
from _recoil.lib.tooling import configure_stdio, display_path


CATALOG_PATH = "binaries.recoil.final_image_catalog"


def _load_progress(path: Path) -> dict[str, Any]:
    try:
        return ProgressStore(path).load().data
    except (OSError, UnicodeError, ProgressError) as exc:
        raise ValueError(
            f"cannot read progress tracker {display_path(path)}: {exc}"
        ) from exc


def audit_catalog(*, tracker: Path, reference: Path) -> dict[str, Any]:
    document = _load_progress(tracker)
    failures: list[str] = []
    if document.get("schema_version") != SCHEMA_VERSION:
        failures.append(
            f"progress tracker must be schema v{SCHEMA_VERSION} before live coverage derivation"
        )
    binaries = document.get("binaries")
    recoil = binaries.get("recoil") if isinstance(binaries, Mapping) else None
    legacy_catalog = recoil.get("final_image_catalog") if isinstance(recoil, Mapping) else None
    coverage: dict[str, Any] | None = None
    if not reference.is_file():
        failures.append(
            "retail reference is missing; live validation deliberately does not fabricate retail PE "
            f"facts: {display_path(reference)}"
        )
    elif document.get("schema_version") == SCHEMA_VERSION:
        reference_data = reference.read_bytes()
        coverage = derive_final_image_coverage(
            reference_data,
            document,
            source=display_path(reference),
        )
        failures.extend(str(item) for item in coverage["failures"])
    legacy_diagnostics: list[str] = []
    if isinstance(legacy_catalog, Mapping) and reference.is_file():
        projection = semantic_projection(reference.read_bytes(), source=str(reference))
        legacy_diagnostics.extend(
            f"legacy catalog diagnostic: {failure}"
            for failure in _validate_catalog(legacy_catalog, projection, document)
        )
    state = recoil.get("final_image_catalog_state") if isinstance(recoil, Mapping) else None
    return {
        "kind": "final-image-catalog-audit",
        "validation_mode": "live-retail-plus-accepted-tracker",
        "passed": not failures,
        "tracker": display_path(tracker),
        "tracker_schema": document.get("schema_version"),
        "tracker_revision": document.get("revision"),
        "catalog_path": CATALOG_PATH,
        "catalog_state": state,
        "catalog_state_diagnostic_only": True,
        "legacy_catalog_present": isinstance(legacy_catalog, Mapping),
        "legacy_catalog_required": False,
        "legacy_catalog_diagnostics": legacy_diagnostics,
        "coverage": coverage,
        "coverage_summary": coverage_summary(coverage) if coverage is not None else None,
        "failure_count": len(failures),
        "failures": failures,
        "next_action": (
            "resolve the first reported typed range or tracker acceptance gap, then rerun "
            "python tools/recoil.py audit final-image-catalog --json; no stored candidate or "
            "pre-populated catalog is accepted"
            if failures
            else "python tools/recoil.py verify final-image --json"
        ),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Derive and audit complete typed final-image coverage live from retail plus accepted "
            "tracker facts, without building or reading a candidate."
        )
    )
    parser.add_argument("--tracker", type=Path, default=DEFAULT_TRACKER)
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        result = audit_catalog(tracker=args.tracker, reference=args.reference)
    except ValueError as exc:
        print(f"final-image catalog audit error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        status = "PASS" if result["passed"] else "BLOCKED"
        print(f"Live final-image coverage: {status}")
        summary = result.get("coverage_summary")
        if isinstance(summary, Mapping):
            print(
                "- retail topology: "
                f"file={'complete' if summary['file_backed_topology_complete'] else 'blocked'}, "
                f"rva={'complete' if summary['rva_topology_complete'] else 'blocked'}"
            )
        for failure in result["failures"]:
            print(f"- {failure}")
        print(f"Next: {result['next_action']}")
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
