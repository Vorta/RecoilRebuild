from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
from typing import Any, Iterable, Mapping

from _recoil.lib.tooling import REPO_ROOT, configure_stdio


RETIRED_PUBLIC_REFERENCES = (
    "progress work claim-current",
    "progress work leases",
    "progress handoff",
    "workspace worktree",
    "issue work reserve",
    "advance-live-byte --lane",
    "current-metadata refresh",
    "audit current-metadata",
    "audit workflow-contracts",
    "progress owner set-address-meta",
)
HISTORICAL_PROCESS_RECORDS = {
    "docs/reconstruction/final_executable_repro_history.md": (
        "docs/reconstruction/final_executable_repro.md"
    ),
}
HISTORICAL_BANNER = "> **Historical process record — not current operating guidance.**"
HARD_OPERATIONAL_PATTERNS = {
    "source-worker-role": re.compile(r"\bsource[- ]workers?\b", re.IGNORECASE),
    "fact-mapper-role": re.compile(r"\bfact[- ]mappers?\b", re.IGNORECASE),
    "verifier-agent-role": re.compile(r"\bverifier agents?\b", re.IGNORECASE),
    "orchestrator-role": re.compile(r"\borchestrat(?:or|ing agent)\b", re.IGNORECASE),
    "parent-only-action": re.compile(r"\bparent[- ](?:only|brokered)\b", re.IGNORECASE),
    "worker-handoff": re.compile(r"\bworker handoff\b", re.IGNORECASE),
    "work-packet": re.compile(r"\bwork packets?\b", re.IGNORECASE),
    "packet-branch": re.compile(r"\bpacket branch\b", re.IGNORECASE),
    "linked-worktree": re.compile(r"\blinked[- ]worktree\b", re.IGNORECASE),
    "agent-compile-host": re.compile(r"\bagent compile host\b", re.IGNORECASE),
    "source-owner-handoff": re.compile(r"\bsource[- ]owner handoff\b", re.IGNORECASE),
    "functional-lane": re.compile(r"\bfunctional[- ]lane\b", re.IGNORECASE),
    "binary-lane": re.compile(r"\bbinary[- ]lane\b", re.IGNORECASE),
    "parallel-workers": re.compile(r"\bnon-overlapping workers\b", re.IGNORECASE),
    "independent-lanes": re.compile(r"\bindependently monotonic\b", re.IGNORECASE),
    "stage-bypass": re.compile(
        r"\bwithout waiting for authored bytes\b",
        re.IGNORECASE,
    ),
    "ignored-target-state": re.compile(
        r"\blocal ignored verification state\b",
        re.IGNORECASE,
    ),
}
NON_NEGATABLE_CATEGORIES = {
    "functional-lane",
    "binary-lane",
    "agent-compile-host",
    "source-owner-handoff",
    "independent-lanes",
    "stage-bypass",
    "ignored-target-state",
}
NEGATION_RE = re.compile(
    r"\b(?:no|never|without|retired|removed|does not|do not|must not|"
    r"there (?:is|are) no|has no|have no)\b",
    re.IGNORECASE,
)
STALE_ROLE_RE = re.compile(r"\b(?:worker|mapper|orchestrator)\b", re.IGNORECASE)
STALE_ROLE_ACTION_RE = re.compile(
    r"\b(?:assign|delegate|dispatch|route|review|validate|accept|scrutinize|"
    r"reconcile|hand(?:ed)? off|return|claim|reserve|lease|schedule)\w*\b",
    re.IGNORECASE,
)
PARENT_PROCESS_RE = re.compile(
    r"\bparent\b.{0,100}\b(?:accept\w*|review\w*|scrutin\w*|reconcil\w*|"
    r"broker\w*|routing constraint)\b",
    re.IGNORECASE,
)
AGENT_DELEGATION_RE = re.compile(
    r"\b(?:assign|delegate|dispatch|route|hand(?:ed)? off|claim|reserve|lease|schedule)\w*\b",
    re.IGNORECASE,
)
WORKER_THREAD_RE = re.compile(r"\bworker threads?\b", re.IGNORECASE)
PACKET_LANE_PROCESS_RE = re.compile(
    r"\b(?:work|scheduler|scheduling|compatible|parallel)\s+(?:packet|lane)s?\b|"
    r"\b(?:packet|lane)\s+(?:allocator|claim|assignment|reservation|scheduler)\b",
    re.IGNORECASE,
)
WORK_ITEM_RE = re.compile(r"\bwork[- ]items?\b", re.IGNORECASE)
WORK_ITEM_REJECTION_PATTERNS = (
    re.compile(
        r"\bthere (?:is|are) no "
        r"(?:(?:[a-z0-9_./-]+(?:\s+[a-z0-9_./-]+){0,3}),\s*)*"
        r"(?P<work_item>work[- ]items?)\b"
        r"(?:\s+(?:collection|state|workflow))?",
        re.IGNORECASE,
    ),
    re.compile(
        r"\b(?:progress schema \d+|tracker|workspace|database|system) "
        r"(?:has|have) no (?P<work_item>work[- ]items?)\b"
        r"(?:\s+(?:collection|state|workflow))?",
        re.IGNORECASE,
    ),
    re.compile(
        r"(?P<work_item>\bwork[- ]items?\b)"
        r"(?:\s+(?:collection|state|workflow))?\s+"
        r"(?:does|do) not exist\b",
        re.IGNORECASE,
    ),
    re.compile(
        r"(?P<work_item>\bwork[- ]items?\b)"
        r"(?:\s+(?:collection|state|workflow))?\s+"
        r"(?:has|have) been removed\b",
        re.IGNORECASE,
    ),
    re.compile(
        r"\b(?:do not|never)\s+"
        r"(?:create|restore|introduce|maintain|use)"
        r"(?:\s+or\s+(?:create|restore|introduce|maintain|use))*\s+"
        r"(?:(?:a|the)\s+)?(?P<work_item>work[- ]items?)\b"
        r"(?:\s+(?:collection|state|workflow))?",
        re.IGNORECASE,
    ),
)


def _relative(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def _front_matter(text: str) -> tuple[dict[str, str], int]:
    lines = text.splitlines()
    if not lines or lines[0] != "---":
        return {}, 0
    metadata: dict[str, str] = {}
    for index, line in enumerate(lines[1:], start=1):
        if line == "---":
            return metadata, index + 1
        if not line or line[:1].isspace() or ":" not in line:
            continue
        key, value = line.split(":", 1)
        metadata[key.strip()] = value.strip().strip('"\'')
    return {}, 0


def _segments(text: str) -> Iterable[tuple[int, str]]:
    buffer: list[str] = []
    start_line = 1
    in_fence = False

    def flush() -> tuple[int, str] | None:
        nonlocal buffer
        if not buffer:
            return None
        result = (start_line, " ".join(item.strip() for item in buffer))
        buffer = []
        return result

    for line_number, line in enumerate(text.splitlines(), start=1):
        stripped = line.strip()
        if stripped.startswith("```"):
            pending = flush()
            if pending is not None:
                yield pending
            yield line_number, stripped
            in_fence = not in_fence
            start_line = line_number + 1
            continue
        if in_fence:
            pending = flush()
            if pending is not None:
                yield pending
            if stripped:
                yield line_number, stripped
            start_line = line_number + 1
            continue
        standalone = (
            not stripped
            or stripped.startswith("#")
            or stripped.startswith("|")
            or re.match(r"^(?:[-*+] |\d+[.)] )", stripped) is not None
        )
        if standalone:
            pending = flush()
            if pending is not None:
                yield pending
            if stripped:
                yield line_number, stripped
            start_line = line_number + 1
            continue
        if not buffer:
            start_line = line_number
        buffer.append(stripped)
    pending = flush()
    if pending is not None:
        yield pending


def _explicitly_negated(text: str, start: int) -> bool:
    prefix = text[max(0, start - 160):start]
    return NEGATION_RE.search(prefix) is not None


def _explicit_work_item_rejection_spans(segment: str) -> set[tuple[int, int]]:
    return {
        match.span("work_item")
        for pattern in WORK_ITEM_REJECTION_PATTERNS
        for match in pattern.finditer(segment)
    }


def _retired_language_findings(
    text: str,
    *,
    location: str,
    line_offset: int = 0,
) -> list[str]:
    findings: list[str] = []
    folded = text.casefold()
    for token in RETIRED_PUBLIC_REFERENCES:
        index = folded.find(token)
        if index >= 0 and not _explicitly_negated(text, index):
            findings.append(
                f"{location} [retired-command] references {token!r}; use the direct serial command surface"
            )
    for line_number, segment in _segments(text):
        is_heading = segment.startswith("#")
        rejected_work_item_spans = _explicit_work_item_rejection_spans(segment)
        for match in WORK_ITEM_RE.finditer(segment):
            if match.span() in rejected_work_item_spans:
                continue
            findings.append(
                f"{location}:{line_number + line_offset} [structured-work-item] "
                f"{match.group(0)!r}; use current task, selector, owner, symbol, "
                "target, fact, or evidence wording"
            )
        for category, pattern in HARD_OPERATIONAL_PATTERNS.items():
            for match in pattern.finditer(segment):
                if (
                    category not in NON_NEGATABLE_CATEGORIES
                    and _explicitly_negated(segment, match.start())
                ):
                    continue
                findings.append(
                    f"{location}:{line_number + line_offset} [{category}] {match.group(0)!r}; use direct single-agent wording"
                )
        if (
            STALE_ROLE_RE.search(segment)
            and STALE_ROLE_ACTION_RE.search(segment)
            and not NEGATION_RE.search(segment)
            and not is_heading
            and not WORKER_THREAD_RE.search(segment)
        ):
            findings.append(
                f"{location}:{line_number + line_offset} [role-action] {segment!r}; name the direct action or evidence instead of a retired role"
            )
        if PARENT_PROCESS_RE.search(segment) and not NEGATION_RE.search(segment):
            findings.append(
                f"{location}:{line_number + line_offset} [parent-process] {segment!r}; name the direct review or acceptance action"
            )
        if (
            re.search(r"\bagent\b", segment, re.IGNORECASE)
            and AGENT_DELEGATION_RE.search(segment)
            and not NEGATION_RE.search(segment)
            and not is_heading
        ):
            findings.append(
                f"{location}:{line_number + line_offset} [agent-delegation] {segment!r}; the current agent works directly"
            )
        if (
            PACKET_LANE_PROCESS_RE.search(segment)
            and not NEGATION_RE.search(segment)
        ):
            findings.append(
                f"{location}:{line_number + line_offset} [packet-lane-process] {segment!r}; use task, evidence, group, or comparison-mode wording"
            )
        if re.search(r"verify functional(?!-batch)[^`\n]*--target\b", segment, re.IGNORECASE):
            findings.append(
                f"{location}:{line_number + line_offset} [invalid-command] verify functional takes a positional target"
            )
        if re.search(r"verify functional(?!-batch)[^`\n]*--json\b", segment, re.IGNORECASE):
            findings.append(
                f"{location}:{line_number + line_offset} [invalid-command] verify functional has no --json option"
            )
    return findings


def _json_strings(value: Any, pointer: str = "") -> Iterable[tuple[str, str]]:
    if isinstance(value, str):
        yield pointer or "/", value
    elif isinstance(value, list):
        for index, item in enumerate(value):
            yield from _json_strings(item, f"{pointer}/{index}")
    elif isinstance(value, Mapping):
        for key, item in value.items():
            escaped = str(key).replace("~", "~0").replace("/", "~1")
            yield from _json_strings(item, f"{pointer}/{escaped}")


def _historical_record_findings(root: Path, path: Path, expected_superseder: str) -> list[str]:
    relative = _relative(path, root)
    findings: list[str] = []
    if not path.is_file():
        return [f"{relative} [historical-record] file is missing"]
    text = path.read_text(encoding="utf-8", errors="replace")
    metadata, _body_start = _front_matter(text)
    expected = {
        "document_status": "historical-process-record",
        "operational_guidance": "false",
        "superseded_by": expected_superseder,
    }
    for key, value in expected.items():
        if metadata.get(key) != value:
            findings.append(
                f"{relative} [historical-metadata] {key} must be {value!r}"
            )
    if HISTORICAL_BANNER not in text:
        findings.append(
            f"{relative} [historical-banner] visible non-operational banner is missing"
        )
    if not (root / expected_superseder).is_file():
        findings.append(
            f"{relative} [historical-superseder] {expected_superseder} does not exist"
        )
    return findings


def _active_surface_paths(root: Path, canonical: Mapping[str, Path]) -> list[Path]:
    paths = {
        root / "AGENTS.md",
        root / "README.md",
        root / "tools" / "README.md",
        root / "tools" / "functional_verify_targets" / "README.md",
        root / "tools" / "vc5_verify_targets" / "README.md",
        *canonical.values(),
    }
    docs = root / "docs" / "reconstruction"
    if docs.is_dir():
        paths.update(docs.rglob("*.md"))
    for skill in canonical.values():
        paths.add(skill.parent / "agents" / "openai.yaml")
    return sorted(paths)


def audit_agent_surface(root: Path = REPO_ROOT) -> dict[str, object]:
    findings: list[str] = []
    codex_skills = root / ".codex" / "skills"
    canonical = {
        path.parent.name: path
        for path in codex_skills.glob("recoil-*/SKILL.md")
        if path.parent.name != "recoil-address-handoff"
    }
    retired_surfaces = [
        path for path in (root / "CLAUDE.md", root / ".claude", root / ".agent" / "AGENTS.md")
        if path.exists()
    ]
    if retired_surfaces:
        findings.append(
            "retired duplicate instruction surfaces still exist: "
            + ", ".join(_relative(path, root) for path in retired_surfaces)
        )
    for name, path in canonical.items():
        relative = _relative(path, root)
        text = path.read_text(encoding="utf-8")
        metadata, _body_start = _front_matter(text)
        if metadata.get("name") != name or not metadata.get("description"):
            findings.append(f"{relative} has invalid front matter")
        agents_yaml = path.parent / "agents" / "openai.yaml"
        if not agents_yaml.is_file():
            findings.append(f"{_relative(agents_yaml, root)} is missing")
        else:
            yaml_text = agents_yaml.read_text(encoding="utf-8", errors="replace")
            default_prompt = re.search(
                r'^\s*default_prompt:\s*"([^"]*)"\s*$',
                yaml_text,
                re.MULTILINE,
            )
            if default_prompt is None or f"${name}" not in default_prompt.group(1):
                findings.append(
                    f"{_relative(agents_yaml, root)} default_prompt must reference ${name}"
                )
            short_description = re.search(
                r'^\s*short_description:\s*"([^"]*)"\s*$',
                yaml_text,
                re.MULTILINE,
            )
            if short_description and re.search(
                r"\b(?:delegate|dispatch|orchestrate)\b",
                short_description.group(1),
                re.IGNORECASE,
            ):
                findings.append(
                    f"{_relative(agents_yaml, root)} [role-metadata] short_description must describe direct work"
                )
    role_files: list[Path] = []
    for directory in (root / ".codex" / "agents",):
        if directory.is_dir():
            role_files.extend(sorted(path for path in directory.glob("*") if path.is_file()))
    if role_files:
        findings.append(
            "retired Recoil role registry still contains files: "
            + ", ".join(_relative(path, root) for path in role_files)
        )
    address_skill_paths = (
        root / ".codex" / "skills" / "recoil-address-handoff",
    )
    for path in address_skill_paths:
        if path.exists() and any(path.rglob("*")):
            findings.append(f"retired address-handoff skill still exists: {_relative(path, root)}")

    for relative, superseder in HISTORICAL_PROCESS_RECORDS.items():
        findings.extend(_historical_record_findings(root, root / relative, superseder))

    active_paths = _active_surface_paths(root, canonical)
    historical_paths = {root / relative for relative in HISTORICAL_PROCESS_RECORDS}
    for path in active_paths:
        relative = _relative(path, root)
        if not path.is_file():
            findings.append(f"active agent surface file is missing: {relative}")
            continue
        if path in historical_paths:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        metadata, _body_start = _front_matter(text)
        if metadata.get("document_status") == "historical-process-record":
            findings.append(
                f"{relative} [historical-registration] is not an approved historical process record"
            )
        findings.extend(_retired_language_findings(text, location=relative))

    for manifest_directory in (
        root / "tools" / "functional_verify_targets",
        root / "tools" / "vc5_verify_targets",
    ):
        if not manifest_directory.is_dir():
            findings.append(
                f"active agent surface directory is missing: {_relative(manifest_directory, root)}"
            )
            continue
        for path in sorted(manifest_directory.glob("*.json")):
            relative = _relative(path, root)
            try:
                manifest = json.loads(path.read_text(encoding="utf-8"))
            except (OSError, json.JSONDecodeError) as exc:
                findings.append(f"{relative} [json] cannot be read: {exc}")
                continue
            for pointer, value in _json_strings(manifest):
                findings.extend(
                    _retired_language_findings(
                        value,
                        location=f"{relative}#{pointer}",
                    )
                )

    readme = root / "README.md"
    if readme.is_file():
        readme_text = readme.read_text(encoding="utf-8", errors="replace")
        folded = " ".join(readme_text.casefold().split())
        stage_positions = [folded.find(stage) for stage in (
            "authored-function-order",
            "authored-call-contract",
            "authored-byte-match",
            "full-function-order",
            "linked-byte-match",
            "final-validation",
        )]
        if any(position < 0 for position in stage_positions) or stage_positions != sorted(stage_positions):
            findings.append(
                "README.md [serial-stage-order] must list the six canonical stages in order"
            )
        required_prerequisite = (
            "only after every authored call contract, its fresh closeout, and every authored byte group are current"
        )
        if required_prerequisite not in folded:
            findings.append(
                "README.md [full-order-prerequisite] must require call-contract closeout and all authored bytes"
            )

    functional_skill = canonical.get("recoil-functional-targets")
    if functional_skill is not None:
        normalized = " ".join(
            functional_skill.read_text(encoding="utf-8", errors="replace").split()
        )
        required_sync = (
            "python tools/recoil.py progress verification-target sync --target <target-id> "
            "--expected-revision <revision> --dry-run --json"
        )
        if required_sync not in normalized:
            findings.append(
                f"{_relative(functional_skill, root)} [sync-command] must show a targeted revision-guarded dry-run"
            )

    return {
        "passed": not findings,
        "findings": findings,
        "canonical_skill_count": len(canonical),
        "role_file_count": len(role_files),
    }


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(
        description="Audit the direct Recoil skill and instruction surface."
    )
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    result = audit_agent_surface()
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print("agent surface audit OK" if result["passed"] else "\n".join(result["findings"]))
    return 0 if result["passed"] or not args.strict else 1


if __name__ == "__main__":
    raise SystemExit(main())
