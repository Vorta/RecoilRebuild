from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import sys
from typing import Any, Iterable, Mapping

from _recoil.lib.live_progress import atomic_replace
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressDocument, ProgressStore
from _recoil.lib.source_owners import (
    OWNER_GATES,
    OWNER_REIMPLEMENTATION_TIERS,
    SourceOwner,
    SourceOwnerDocument,
    owner_data_addresses,
    owner_member_addresses,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


DEFAULT_README_PATH = REPO_ROOT / "README.md"
START_MARKER = "<!-- RECOIL_PROGRESS:START -->"
END_MARKER = "<!-- RECOIL_PROGRESS:END -->"
GATE_STATE_COLUMNS = ("accepted", "blocked", "deferred", "none", "pending")
PREFERRED_BINARY_ROWS = ("recoil", "messages")


class ReadmeProgressError(RuntimeError):
    pass


def percentage(count: int, total: int) -> str:
    return f"{(count * 100.0 / total) if total else 0.0:.1f}%"


def authored_owners(owners: Iterable[SourceOwner]) -> list[SourceOwner]:
    return [owner for owner in owners if owner.kind != "provider-boundary"]


def ordered_binary_labels(owners: Iterable[SourceOwner]) -> list[str]:
    labels = {owner.binary or "unknown" for owner in owners}
    ordered = [label for label in PREFERRED_BINARY_ROWS if label in labels]
    ordered.extend(sorted(label for label in labels if label not in PREFERRED_BINARY_ROWS))
    return ordered


def _overview_row(label: str, owners: list[SourceOwner]) -> str:
    authored_count = sum(owner.kind != "provider-boundary" for owner in owners)
    provider_count = sum(owner.kind == "provider-boundary" for owner in owners)
    return f"| {label} | {len(owners)} | {authored_count} | {provider_count} |"


def build_overview_section(owners: list[SourceOwner]) -> str:
    rows = [
        _overview_row(
            label,
            [owner for owner in owners if (owner.binary or "unknown") == label],
        )
        for label in ordered_binary_labels(owners)
    ]
    rows.append(_overview_row("Total", owners))
    return "\n".join(
        [
            "### Source-Owner Overview",
            "",
            "| Binary | Owners | Authored owners | Provider boundaries |",
            "| --- | ---: | ---: | ---: |",
            *rows,
        ]
    )


def build_gate_section(owners: list[SourceOwner]) -> str:
    rows: list[str] = []
    for gate in OWNER_GATES:
        counts = Counter(owner.gate(gate) for owner in owners)
        cells = [str(counts.get(state, 0)) for state in GATE_STATE_COLUMNS]
        rows.append(f"| {gate} | {' | '.join(cells)} | {len(owners)} |")
    return "\n".join(
        [
            "### Source-Owner Gates",
            "",
            "| Gate | accepted | blocked | deferred | none | pending | Total |",
            "| --- | ---: | ---: | ---: | ---: | ---: | ---: |",
            *rows,
        ]
    )


def build_owner_tier_section(owners: list[SourceOwner]) -> str:
    authored = authored_owners(owners)
    counts = Counter(owner.reimplementation_tier for owner in authored)
    rows = [
        f"| {tier} | {counts.get(tier, 0)} | "
        f"{percentage(counts.get(tier, 0), len(authored))} |"
        for tier in OWNER_REIMPLEMENTATION_TIERS
    ]
    return "\n".join(
        [
            "### Owner Reimplementation Tiers",
            "",
            "| Tier | Count | Percent of authored owners |",
            "| --- | ---: | ---: |",
            *rows,
        ]
    )


def _entry_tier_counts(
    owners: Iterable[SourceOwner],
    *,
    kind: str,
) -> tuple[Counter[str], int]:
    counts: Counter[str] = Counter()
    total = 0
    for owner in authored_owners(owners):
        addresses = (
            owner_member_addresses(owner)
            if kind == "function"
            else owner_data_addresses(owner)
        )
        for address in addresses:
            counts[owner.entry_reimplementation_tier(address)] += 1
            total += 1
    return counts, total


def build_entry_tier_section(
    owners: list[SourceOwner],
    *,
    title: str,
    noun: str,
    kind: str,
) -> str:
    labels = ordered_binary_labels(owners)
    counts_by_binary = {
        label: _entry_tier_counts(
            [owner for owner in owners if (owner.binary or "unknown") == label],
            kind=kind,
        )
        for label in labels
    }
    total = sum(entry_total for _counts, entry_total in counts_by_binary.values())
    rows: list[str] = []
    for tier in OWNER_REIMPLEMENTATION_TIERS:
        cells = [counts_by_binary[label][0].get(tier, 0) for label in labels]
        count = sum(cells)
        rows.append(
            f"| {tier} | {' | '.join(str(cell) for cell in cells)} | {count} | "
            f"{percentage(count, total)} |"
        )
    return "\n".join(
        [
            f"### {title}",
            "",
            f"Counts durable per-primary-entry tiers for {noun}. Owner tiers are derived "
            "separately and may be lower because of sibling entries or owner gates.",
            "",
            f"| Tier | {' | '.join(labels)} | Total | Percent of authored entries |",
            f"| --- | {' | '.join('---:' for _ in labels)} | ---: | ---: |",
            *rows,
        ]
    )


def build_kind_section(owners: list[SourceOwner]) -> str:
    counts = Counter(owner.kind for owner in owners)
    rows = [
        f"| {kind} | {count} | {percentage(count, len(owners))} |"
        for kind, count in sorted(counts.items())
    ]
    return "\n".join(
        [
            "### Owner Kinds",
            "",
            "| Kind | Count | Percent of owners |",
            "| --- | ---: | ---: |",
            *rows,
        ]
    )


def _count_text(counts: Mapping[str, Any], *, total_key: str = "total") -> str:
    accepted = int(counts.get("accepted", 0))
    total = int(counts.get(total_key, 0))
    return f"{accepted} / {total}"


def _stage_state(
    phase: str,
    stage: str,
    counts: Mapping[str, Any] | None = None,
) -> str:
    if counts is not None and int(counts.get("remaining", 0)) == 0:
        return "complete"
    if phase == stage:
        return "current"
    order = {
        "authored-function-order": 0,
        "authored-call-contract": 1,
        "authored-byte-match": 2,
        "full-function-order": 3,
        "linked-byte-match": 4,
        "final-validation": 5,
    }
    return "waiting" if order.get(phase, 99) < order.get(stage, 99) else "ready"


def _primary_blocker(pipeline: Mapping[str, Any]) -> str:
    resolution = pipeline.get("order_target_resolution", {})
    if not isinstance(resolution, Mapping) or resolution.get("status") != "blocked":
        return "—"
    code = str(resolution.get("reason_code") or "blocked")
    blocker = resolution.get("blocker", {})
    if not isinstance(blocker, Mapping):
        return code
    address = str(blocker.get("address") or "")
    label = str(blocker.get("label") or "")
    detail = " ".join(part for part in (address, f"({label})" if label else "") if part)
    return f"{code}: {detail}" if detail else code


def build_pipeline_section(document: ProgressDocument) -> str:
    pipeline = document.pipeline("recoil")
    authored_order = pipeline["authored_function_order_counts"]
    authored_byte = pipeline["authored_byte_counts"]
    call_contract = pipeline["authored_call_contract_counts"]
    full_order = pipeline["full_function_order_counts"]
    linked_byte = pipeline["linked_byte_counts"]
    phase = str(pipeline.get("phase") or "")
    primary_blocker = _primary_blocker(pipeline)
    current_blocker = str(pipeline.get("blocker") or "—")
    rows = [
        (
            "authored-function-order",
            _stage_state(phase, "authored-function-order", authored_order),
            _count_text(authored_order),
            str(pipeline.get("authored_function_order_prefix_end") or "—"),
            primary_blocker if phase == "authored-function-order" else "—",
        ),
        (
            "authored-call-contract",
            _stage_state(phase, "authored-call-contract", call_contract),
            _count_text(call_contract),
            str(pipeline.get("authored_call_contract_cursor") or "—"),
            current_blocker if phase == "authored-call-contract" else "—",
        ),
        (
            "authored-byte-match",
            _stage_state(phase, "authored-byte-match", authored_byte),
            _count_text(authored_byte),
            str(pipeline.get("authored_byte_match_frontier") or "—"),
            current_blocker if phase == "authored-byte-match" else "—",
        ),
        (
            "full-function-order",
            _stage_state(phase, "full-function-order", full_order),
            _count_text(full_order),
            str(pipeline.get("full_function_order_prefix_end") or "—"),
            primary_blocker if phase == "full-function-order" else "—",
        ),
        (
            "linked-byte-match",
            _stage_state(phase, "linked-byte-match", linked_byte),
            _count_text(linked_byte),
            str(pipeline.get("linked_byte_match_prefix_end") or "—"),
            "—",
        ),
        (
            "final-validation",
            "current" if phase == "final-validation" else "waiting",
            "typed whole image",
            "—",
            "—",
        ),
    ]
    rendered = [
        "### Live Reconstruction Pipeline",
        "",
        "| Stage | State | Accepted / total | Frontier | Typed blocker |",
        "| --- | --- | ---: | --- | --- |",
    ]
    rendered.extend(
        f"| {stage} | {state} | {counts} | {frontier} | {blocker} |"
        for stage, state, counts, frontier, blocker in rows
    )
    return "\n".join(rendered)


def build_progress_block(document: ProgressDocument, owners: list[SourceOwner]) -> str:
    return "\n".join(
        [
            START_MARKER,
            "Generated from the unified reconstruction tracker. The tracker remains the sole progress authority.",
            "",
            build_pipeline_section(document),
            "",
            build_overview_section(owners),
            "",
            build_gate_section(owners),
            "",
            build_owner_tier_section(owners),
            "",
            build_entry_tier_section(
                owners,
                title="Function Reimplementation Tiers",
                noun="authored primary functions",
                kind="function",
            ),
            "",
            build_entry_tier_section(
                owners,
                title="Data Reimplementation Tiers",
                noun="authored primary data entries",
                kind="data",
            ),
            "",
            build_kind_section(owners),
            END_MARKER,
        ]
    )


def render_progress_block(progress_path: Path) -> str:
    document = ProgressStore(progress_path).load()
    owners = SourceOwnerDocument.from_progress_document(
        document, path=progress_path
    ).owners
    return build_progress_block(document, owners)


def planned_readme_text(readme_path: Path, block: str) -> tuple[str, str]:
    try:
        before = readme_path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        raise ReadmeProgressError(f"{readme_path}: unreadable README: {exc}") from exc
    starts = before.count(START_MARKER)
    ends = before.count(END_MARKER)
    if starts != ends or starts not in {0, 1}:
        raise ReadmeProgressError(
            "README progress markers must be one balanced, non-duplicated pair"
        )
    if starts == 1:
        start = before.index(START_MARKER)
        end = before.index(END_MARKER, start) + len(END_MARKER)
        return before, before[:start] + block + before[end:]

    heading = "## Status"
    heading_start = before.find(heading)
    if heading_start < 0 or (heading_start > 0 and before[heading_start - 1] != "\n"):
        raise ReadmeProgressError("README is missing the '## Status' section")
    heading_end = before.find("\n", heading_start)
    if heading_end < 0:
        heading_end = len(before)
    else:
        heading_end += 1
    insertion = heading_end
    while insertion < len(before) and before[insertion] == "\n":
        insertion += 1
    after = before[:heading_end] + "\n" + block + "\n\n" + before[insertion:]
    return before, after


def synchronize_readme(
    *,
    progress_path: Path = DEFAULT_PROGRESS_PATH,
    readme_path: Path = DEFAULT_README_PATH,
    check: bool = False,
) -> dict[str, Any]:
    block = render_progress_block(progress_path)
    before, after = planned_readme_text(readme_path, block)
    stale = before != after
    changed = stale and not check
    if changed:
        atomic_replace(readme_path, after.encode("utf-8"))
    return {
        "report_version": 1,
        "kind": "readme-progress",
        "mode": "check" if check else "update",
        "progress": display_path(progress_path),
        "readme": display_path(readme_path),
        "current": not stale,
        "changed": changed,
    }


def readme_freshness_findings(
    *,
    progress_path: Path,
    readme_path: Path,
) -> list[dict[str, str]]:
    try:
        report = synchronize_readme(
            progress_path=progress_path,
            readme_path=readme_path,
            check=True,
        )
    except (OSError, ValueError, ReadmeProgressError) as exc:
        return [
            {
                "severity": "error",
                "code": "readme-progress-invalid",
                "path": display_path(readme_path),
                "message": str(exc),
                "remediation": "python tools/recoil.py docs readme-progress",
            }
        ]
    if report["current"]:
        return []
    return [
        {
            "severity": "error",
            "code": "readme-progress-stale",
            "path": display_path(readme_path),
            "message": "generated README progress does not match the unified tracker",
            "remediation": "python tools/recoil.py docs readme-progress",
        }
    ]


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Update or check the deterministic public README progress block."
    )
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    parser.add_argument("--readme", type=Path)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    progress = args.progress
    readme = args.readme or DEFAULT_README_PATH
    if (
        progress.resolve() != DEFAULT_PROGRESS_PATH.resolve()
        and args.readme is None
        and readme.resolve() == DEFAULT_README_PATH.resolve()
    ):
        print(
            "README progress error: a non-default tracker requires an explicit --readme target",
            file=sys.stderr,
        )
        return 2
    try:
        report = synchronize_readme(
            progress_path=progress,
            readme_path=readme,
            check=bool(args.check),
        )
    except (OSError, ValueError, ReadmeProgressError) as exc:
        print(f"README progress error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    elif args.check:
        print(f"README progress: {'CURRENT' if report['current'] else 'STALE'}")
    else:
        print(f"README progress: {'UPDATED' if report['changed'] else 'CURRENT'}")
    return 1 if args.check and not report["current"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
