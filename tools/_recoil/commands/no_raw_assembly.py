#!/usr/bin/env python3
"""Fail on raw assembly and naked functions outside documented exceptions."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
import re
import sys
from collections.abc import Iterator
from dataclasses import dataclass
from pathlib import Path

from _recoil.lib.source_constructs import parse_source_constructs
from _recoil.lib.source_traceability import SourceTraceDocument, parse_source_trace_text
from _recoil.lib import source_traceability as source_traceability_lib
from _recoil.lib.coff_alias import parse_alias_source_text
from _recoil.lib.tooling import (
    ASM_SUFFIXES,
    REPO_ROOT,
    SOURCE_SUFFIXES,
    display_path,
    iter_source_files,
    strip_comments_and_strings,
)

FORBIDDEN_PATTERNS = [
    ("__asm", re.compile(r"\b__asm\b")),
    ("_asm", re.compile(r"\b_asm\b")),
    ("_emit", re.compile(r"\b_emit\b")),
    ("asm(...)", re.compile(r"\basm\s*(?:volatile\s*)?\(")),
    ("__declspec(naked)", re.compile(r"__declspec\s*\(\s*naked\s*\)")),
    ("__attribute__((naked))", re.compile(r"__attribute__\s*\(\s*\(\s*naked\s*\)\s*\)")),
    ("RECOIL_NAKED", re.compile(r"\bRECOIL_NAKED\b")),
]
ADDRESS_RE = re.compile(r"0x[0-9a-fA-F]+")
RAW_REGION_ID_RE = re.compile(r"recoil:raw-asm:[a-z0-9._-]+")
RAW_ASM_REGION_RE = re.compile(
    r"(?m)^[ \t]*(?:\*[ \t]*)?@recoil-raw-asm[ \t]+"
    r"(recoil:raw-asm:[a-z0-9._-]+)[ \t]*$"
)
RAW_CONSUMER_RE = re.compile(
    r"(?m)^[ \t]*(?:\*[ \t]*)?@recoil-raw-consumer[ \t]+"
    r"(recoil:raw-asm:[a-z0-9._-]+)"
    r"(?:[ \t]+(recoil:function:0x[0-9a-f]+))?[ \t]*$"
)
RAW_ASM_DOC_RE = re.compile(
    r"raw[- ]assembly|inline[- ]asm|__asm|_emit|naked|raw mmx|"
    r"mmx-selected|mmx[- ]block|esp-pivot",
    re.IGNORECASE,
)
RAW_ASM_REASON_RE = re.compile(
    r"vc5|vc5sp3|retail|byte|binary ninja|\bbn\b|c\+\+|compiler|"
    r"intrinsic|opcode|chatgpt-pro-line|failed|not sufficient",
    re.IGNORECASE,
)
EXCEPTION_TAGS = {
    "ainet-vector",
    "coff-weak-alias-only",
    "compiler-provider-exact",
    "cpu-probe",
    "hard-byte-naked",
    "mechanical-inline-residual",
    "render-mmx",
    "render-esp-pivot",
    "source-faithful-inline-asm",
}
COMPILER_PROVIDER_EXACT_ADDRESS = "0x40a170"
COMPILER_PROVIDER_EVIDENCE_PATTERNS = (
    re.compile(r"\bcredible\b", re.IGNORECASE),
    re.compile(r"\bvc5(?:sp3)?\b", re.IGNORECASE),
    re.compile(r"\bc\+\+", re.IGNORECASE),
    re.compile(r"\bvariants?\b", re.IGNORECASE),
    re.compile(r"\bfailed\b", re.IGNORECASE),
)
AINET_VECTOR_EXCEPTIONS = {
    ("src/Battlesport/ai_net.h", "0x401180"),
}
HARD_BYTE_NAKED_EXCEPTIONS = {
    ("src/Battlesport/ai_net.h", "0x401d50"),
}
HARD_BYTE_NAKED_LABELS = {
    "__asm",
    "__declspec(naked)",
}
RENDER_MMX_EXCEPTIONS = {
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x48d510"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x48d5f0"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49ea80"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49ec20"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49e400"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49e560"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49cbb0"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49cea0"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49da80"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49ddb0"),
}
RENDER_ESP_PIVOT_EXCEPTIONS = {
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49b7e0"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49bbf0"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49e6c0"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49edc0"),
    ("src/GameZRecoil/zRender/zrndr_draw.c", "0x49f180"),
}


def normalize_key_path(path: str) -> str:
    return path.replace("\\", "/").lower()


def normalize_exception_set(values: set[tuple[str, str]]) -> set[tuple[str, str]]:
    return {(normalize_key_path(path), address.lower()) for path, address in values}


AINET_VECTOR_EXCEPTIONS = normalize_exception_set(AINET_VECTOR_EXCEPTIONS)
HARD_BYTE_NAKED_EXCEPTIONS = normalize_exception_set(HARD_BYTE_NAKED_EXCEPTIONS)
RENDER_MMX_EXCEPTIONS = normalize_exception_set(RENDER_MMX_EXCEPTIONS)
RENDER_ESP_PIVOT_EXCEPTIONS = normalize_exception_set(RENDER_ESP_PIVOT_EXCEPTIONS)


@dataclass(frozen=True)
class RawRegion:
    region_id: str
    rel: str
    start: int
    end: int
    consumers: frozenset[str]
    valid: bool


def load_allowlist(path: Path) -> dict[tuple[str, str, str], set[str]]:
    allowed: dict[tuple[str, str, str], set[str]] = {}
    if not path.exists():
        return allowed

    for line_no, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.split("#", 1)[0].strip()
        if not line:
            continue

        fields = line.split()
        if len(fields) < 4:
            raise ValueError(
                f"{path}:{line_no}: expected '<path> <address> <token> <exception-tag> reason'"
            )

        rel, address, label = fields[:3]
        rel = normalize_key_path(rel)
        address = address.lower()
        region_mode = RAW_REGION_ID_RE.fullmatch(rel) is not None
        if region_mode:
            if not re.fullmatch(r"recoil:function:0x[0-9a-f]+", address):
                raise ValueError(
                    f"{path}:{line_no}: stable raw region rows require an exact "
                    "recoil:function:0x... consumer artifact id"
                )
            consumer_address = address.rsplit(":", 1)[-1]
        else:
            consumer_address = address
        reason = " ".join(fields[3:]).lower()
        tags = {tag for tag in EXCEPTION_TAGS if tag in reason}
        if not tags:
            raise ValueError(
                f"{path}:{line_no}: raw assembly exceptions must be tagged one of: "
                f"{', '.join(sorted(EXCEPTION_TAGS))}"
            )
        if address == "provider":
            raise ValueError(f"{path}:{line_no}: provider raw assembly shims are not allowed")
        exact_policy_tags = tags.intersection(
            {
                "coff-weak-alias-only",
                "compiler-provider-exact",
                "mechanical-inline-residual",
            }
        )
        if exact_policy_tags and len(tags) != 1:
            raise ValueError(
                f"{path}:{line_no}: {next(iter(exact_policy_tags))} must be the row's only exception tag"
            )
        if "coff-weak-alias-only" in tags:
            if address != "coff-alias" or label != ".asm":
                raise ValueError(
                    f"{path}:{line_no}: coff-weak-alias-only rows require "
                    "'<path> coff-alias .asm coff-weak-alias-only ...'"
                )
        if "compiler-provider-exact" in tags:
            if address != COMPILER_PROVIDER_EXACT_ADDRESS:
                raise ValueError(
                    f"{path}:{line_no}: compiler-provider-exact is limited to "
                    f"the exact provider address {COMPILER_PROVIDER_EXACT_ADDRESS}"
                )
            if label not in {"__asm", "__declspec(naked)"}:
                raise ValueError(
                    f"{path}:{line_no}: compiler-provider-exact rows permit only "
                    "__asm and __declspec(naked); _emit remains forbidden"
                )
        if "mechanical-inline-residual" in tags and label != "__asm":
            raise ValueError(
                    f"{path}:{line_no}: mechanical-inline-residual rows permit only "
                    "__asm; naked functions and _emit remain forbidden"
                )
        ainet_allowed = (
            True
            if region_mode
            else (rel, address) in AINET_VECTOR_EXCEPTIONS
        )
        if "ainet-vector" in tags and not ainet_allowed:
            allowed_addresses = ", ".join(address for _, address in sorted(AINET_VECTOR_EXCEPTIONS))
            raise ValueError(
                f"{path}:{line_no}: ainet-vector is limited to "
                f"src/Battlesport/ai_net.h at {allowed_addresses}"
            )
        if "ainet-vector" in tags and label != "__asm":
            raise ValueError(
                f"{path}:{line_no}: ainet-vector allowlist rows permit only __asm; "
                f"{label} is not allowed for the AINet path-vector helper"
            )
        if "source-faithful-inline-asm" in tags and label != "__asm":
            raise ValueError(
                f"{path}:{line_no}: source-faithful-inline-asm rows permit only __asm; "
                f"{label} requires a narrower documented exception"
            )
        hard_byte_allowed = (
            True
            if region_mode
            else (rel, address) in HARD_BYTE_NAKED_EXCEPTIONS
        )
        if "hard-byte-naked" in tags and not hard_byte_allowed:
            allowed_addresses = ", ".join(
                address for _, address in sorted(HARD_BYTE_NAKED_EXCEPTIONS)
            )
            raise ValueError(
                f"{path}:{line_no}: hard-byte-naked is limited to "
                f"src/Battlesport/ai_net.h at {allowed_addresses}"
            )
        if "hard-byte-naked" in tags and label not in HARD_BYTE_NAKED_LABELS:
            raise ValueError(
                f"{path}:{line_no}: hard-byte-naked allowlist rows permit only "
                f"{', '.join(sorted(HARD_BYTE_NAKED_LABELS))}; {label} is not allowed"
            )
        render_mmx_allowed = (
            True
            if region_mode
            else (rel, address) in RENDER_MMX_EXCEPTIONS
        )
        if "render-mmx" in tags and not render_mmx_allowed:
            allowed_addresses = ", ".join(address for _, address in sorted(RENDER_MMX_EXCEPTIONS))
            raise ValueError(
                f"{path}:{line_no}: render-mmx is limited to "
                f"src/GameZRecoil/zRender/zrndr_draw.c at {allowed_addresses}"
            )
        if "render-mmx" in tags and label != "__asm":
            raise ValueError(
                f"{path}:{line_no}: render-mmx allowlist rows permit only __asm; "
                f"{label} is not allowed for the zRndr render MMX functions"
            )
        render_pivot_allowed = (
            True
            if region_mode
            else (rel, address) in RENDER_ESP_PIVOT_EXCEPTIONS
        )
        if "render-esp-pivot" in tags and not render_pivot_allowed:
            allowed_addresses = ", ".join(address for _, address in sorted(RENDER_ESP_PIVOT_EXCEPTIONS))
            raise ValueError(
                f"{path}:{line_no}: render-esp-pivot is limited to "
                f"src/GameZRecoil/zRender/zrndr_draw.c at {allowed_addresses}"
            )
        if "render-esp-pivot" in tags and label != "__asm":
            raise ValueError(
                f"{path}:{line_no}: render-esp-pivot allowlist rows permit only __asm; "
                f"{label} is not allowed for the ESP-pivot span functions"
            )
        allowed.setdefault((rel, address, label), set()).update(tags)

    compiler_provider_groups: dict[tuple[str, str], set[str]] = {}
    for (rel, address, label), tags in allowed.items():
        if "compiler-provider-exact" in tags:
            compiler_provider_groups.setdefault((rel, address), set()).add(label)
    for (rel, address), labels in compiler_provider_groups.items():
        expected_labels = {"__asm", "__declspec(naked)"}
        if labels != expected_labels:
            raise ValueError(
                f"{path}: compiler-provider-exact {rel} {address} must have exactly "
                "paired __declspec(naked) and __asm allowlist rows"
            )

    return allowed


def _region_extent(text: str, comment_end: int) -> tuple[int, int] | None:
    cursor = comment_end
    while cursor < len(text) and text[cursor].isspace():
        cursor += 1
    if text.startswith("#", cursor):
        line_end = text.find("\n", cursor)
        line_end = len(text) if line_end < 0 else line_end + 1
        first_line = text[cursor:line_end]
        if re.match(r"#\s*define\s+[A-Za-z_]\w*", first_line) is None:
            return None
        end = line_end
        while text[cursor:end].rstrip("\r\n").endswith("\\") and end < len(text):
            cursor = end
            next_end = text.find("\n", cursor)
            end = len(text) if next_end < 0 else next_end + 1
        return comment_end, end
    for construct in parse_source_constructs(text):
        if construct.start <= cursor < construct.end and not text[construct.start:cursor].strip():
            return comment_end, construct.end
    return None


def raw_region_inventory(
    source_files: list[tuple[Path, str, str]],
    allowlist: dict[tuple[str, str, str], set[str]],
    *,
    require_all_allowlist_regions: bool = False,
) -> tuple[dict[str, tuple[RawRegion, ...]], tuple[str, ...]]:
    declared_consumers: dict[str, set[str]] = {}
    region_rows: list[tuple[str, str, int, int, bool]] = []
    errors: list[str] = []

    for _path, rel, text in source_files:
        try:
            document = parse_source_trace_text(text, path=rel)
        except ValueError as exc:
            errors.append(f"{rel}: source trace parse failed: {exc}")
            document = None
        comments = source_traceability_lib._scan_comments(text)
        for comment in comments:
            consumer_rows = RAW_CONSUMER_RE.findall(comment.text)
            if consumer_rows:
                if comment.style != "doxygen":
                    errors.append(f"{rel}:{comment.line}: raw consumer marker requires a Doxygen block")
                    continue
                implicit_artifact_id: str | None = None
                if any(not artifact_id for _region_id, artifact_id in consumer_rows):
                    anchors = [
                        anchor
                        for anchor in (document.anchors if document is not None else ())
                        if anchor.comment_start == comment.start
                        and anchor.comment_end == comment.end
                    ]
                    artifacts = [
                        artifact
                        for artifact in (document.artifacts if document is not None else ())
                        if anchors
                        and artifact.anchor_id == anchors[0].anchor_id
                        and artifact.direct
                        and artifact.relation == "defines"
                        and artifact.section == ".text"
                        and artifact.entity_kind == "function"
                    ]
                    if len(anchors) != 1 or len(artifacts) != 1:
                        errors.append(
                            f"{rel}:{comment.line}: raw consumer marker without an "
                            "explicit artifact id requires exactly one direct canonical "
                            "defines .text function artifact"
                        )
                        continue
                    implicit_artifact_id = artifacts[0].artifact_id
                explicit_rows = [
                    (region_id, artifact_id or implicit_artifact_id)
                    for region_id, artifact_id in consumer_rows
                ]
                if any(artifact_id is None for _region_id, artifact_id in explicit_rows):
                    continue
                if any(artifact_id for _region_id, artifact_id in consumer_rows):
                    extent = _region_extent(text, comment.end)
                    if extent is None or not any(
                        item.kind == "function"
                        and item.start < extent[1] <= item.end
                        for item in parse_source_constructs(text)
                    ):
                        errors.append(
                            f"{rel}:{comment.line}: explicit raw consumer identity must "
                            "be immediately attached to a function definition"
                        )
                        continue
                for region_id, artifact_id in explicit_rows:
                    assert artifact_id is not None
                    declared_consumers.setdefault(region_id, set()).add(artifact_id)
            for region_id in RAW_ASM_REGION_RE.findall(comment.text):
                extent = _region_extent(text, comment.end)
                valid = (
                    comment.style == "doxygen"
                    and extent is not None
                    and RAW_ASM_DOC_RE.search(comment.text) is not None
                    and RAW_ASM_REASON_RE.search(comment.text) is not None
                    and "Purpose:" in comment.text
                )
                if extent is None:
                    errors.append(
                        f"{rel}:{comment.line}: raw assembly region marker is not attached "
                        "to a function/data/MFC construct or #define region"
                    )
                    continue
                if not valid:
                    errors.append(
                        f"{rel}:{comment.line}: raw assembly region requires a Doxygen "
                        "Purpose and VC5/retail/failed-C++ evidence"
                    )
                region_rows.append((region_id, normalize_key_path(rel), extent[0], extent[1], valid))

    by_region_allowlist: dict[str, dict[str, set[str]]] = {}
    for (scope, consumer, label), _tags in allowlist.items():
        if RAW_REGION_ID_RE.fullmatch(scope) is None:
            continue
        by_region_allowlist.setdefault(scope, {}).setdefault(label, set()).add(consumer)

    result: dict[str, list[RawRegion]] = {}
    for region_id, rel, start, end, valid in region_rows:
        consumers = frozenset(declared_consumers.get(region_id, set()))
        label_sets = by_region_allowlist.get(region_id, {})
        exact = bool(consumers) and bool(label_sets) and all(
            allowed_consumers == set(consumers)
            for allowed_consumers in label_sets.values()
        )
        if not exact:
            errors.append(
                f"{rel}: raw region {region_id} consumer set is not an exact match "
                "for every allowlisted token"
            )
        result.setdefault(rel, []).append(
            RawRegion(region_id, rel, start, end, consumers, valid and exact)
        )
    for region_id in sorted(set(declared_consumers) - {row[0] for row in region_rows}):
        errors.append(f"raw consumer set names missing region {region_id}")
    if require_all_allowlist_regions:
        for region_id in sorted(set(by_region_allowlist) - {row[0] for row in region_rows}):
            errors.append(f"raw allowlist names missing region {region_id}")
    return {key: tuple(value) for key, value in result.items()}, tuple(errors)


def stable_region_allows(
    *,
    rel: str,
    label: str,
    offset: int,
    regions: dict[str, tuple[RawRegion, ...]],
    allowlist: dict[tuple[str, str, str], set[str]],
) -> bool:
    for region in regions.get(rel, ()):
        if not region.valid or not (region.start <= offset < region.end):
            continue
        if all(
            (region.region_id, consumer, label) in allowlist
            for consumer in region.consumers
        ):
            return True
    return False


def find_enclosing_macro(lines: list[str], line_no: int) -> str | None:
    start = max(0, line_no - 80)
    for index in range(line_no - 1, start - 1, -1):
        stripped = lines[index].strip()
        if not stripped.startswith("#define "):
            continue
        fields = stripped.split(None, 2)
        if len(fields) < 2:
            continue
        return fields[1].split("(", 1)[0]
    return None


def iter_docblocks_before(
    lines: list[str],
    line_no: int,
    *,
    max_scan_lines: int = 520,
) -> Iterator[str]:
    index = line_no - 2
    scan_stop = max(-1, line_no - max_scan_lines - 1)
    while index > scan_stop:
        if "*/" not in lines[index]:
            index -= 1
            continue

        end = index
        start = index
        while start > scan_stop and "/**" not in lines[start]:
            start -= 1
        if start <= scan_stop:
            break
        if "/**" in lines[start]:
            yield "\n".join(lines[start : end + 1])
            index = start - 1
        else:
            index -= 1


def has_raw_asm_docblock(lines: list[str], line_no: int, address: str) -> bool:
    text = "\n".join(iter_docblocks_before(lines, line_no))
    if not text:
        return False

    addresses = {value.lower() for value in ADDRESS_RE.findall(text)}
    if address.lower() not in addresses:
        return False

    return (
        RAW_ASM_DOC_RE.search(text) is not None
        and RAW_ASM_REASON_RE.search(text) is not None
        and "Purpose:" in text
    )


def has_attached_compiler_provider_docblock(
    text: str,
    source_offset: int,
) -> bool:
    """Validate the non-authored exact-provider exception at retail 0x40a170."""

    functions = [
        construct
        for construct in parse_source_constructs(text)
        if (
            construct.kind == "function"
            and construct.start <= source_offset < construct.end
        )
    ]
    if len(functions) != 1:
        return False
    function = functions[0]

    attached_comments = [
        comment
        for comment in source_traceability_lib._scan_comments(text)
        if (
            comment.style == "doxygen"
            and comment.end <= function.start
            and not text[comment.end : function.start].strip()
        )
    ]
    if len(attached_comments) != 1:
        return False
    docblock = attached_comments[0].text
    addresses = {value.lower() for value in ADDRESS_RE.findall(docblock)}
    if addresses != {COMPILER_PROVIDER_EXACT_ADDRESS}:
        return False
    if "Purpose:" not in docblock:
        return False
    if not all(pattern.search(docblock) for pattern in COMPILER_PROVIDER_EVIDENCE_PATTERNS):
        return False

    function_text = strip_comments_and_strings(text[function.start : function.end])
    return (
        re.search(r"__declspec\s*\(\s*naked\s*\)", function_text) is not None
        and re.search(r"\b__asm\b", function_text) is not None
        and re.search(r"\b_emit\b", function_text) is None
    )


def direct_defining_function_address(
    document: SourceTraceDocument,
    text: str,
    source_offset: int,
) -> str | None:
    """Return a valid canonical `.text` function address enclosing the token."""

    artifact = document.direct_defining_function_at(source_offset)
    if (
        artifact is None
        or artifact.section != ".text"
        or artifact.comment_style != "doxygen"
        or artifact.attachment_status != "attached"
        or artifact.anchor_id is None
        or artifact.address is None
    ):
        return None

    anchor = next(
        (item for item in document.anchors if item.anchor_id == artifact.anchor_id),
        None,
    )
    if anchor is None:
        return None
    comment_first_line = text.count("\n", 0, anchor.comment_start) + 1
    comment_last_line = text.count("\n", 0, anchor.comment_end) + 1
    for finding in document.findings:
        if (
            finding.anchor_id == artifact.anchor_id
            or finding.artifact_id == artifact.artifact_id
            or (
                finding.anchor_id is None
                and finding.artifact_id is None
                and comment_first_line <= finding.line <= comment_last_line
            )
        ):
            return None
    return artifact.address


def is_allowlisted_raw_assembly(
    rel: str,
    label: str,
    document: SourceTraceDocument | None,
    text: str,
    source_offset: int,
    lines: list[str],
    line_no: int,
    allowlist: dict[tuple[str, str, str], set[str]],
) -> bool:
    compiler_provider_tags = allowlist.get(
        (rel, COMPILER_PROVIDER_EXACT_ADDRESS, label)
    )
    if compiler_provider_tags == {"compiler-provider-exact"}:
        return has_attached_compiler_provider_docblock(text, source_offset)

    if document is None:
        return False
    address = direct_defining_function_address(document, text, source_offset)
    if address is None:
        return False

    tags = allowlist.get((rel, address, label))
    if tags is None or not has_raw_asm_docblock(lines, line_no, address):
        return False
    if "ainet-vector" in tags:
        macro = find_enclosing_macro(lines, line_no)
        if macro is None or not macro.startswith("AINET_PATH_"):
            return False
        if (rel, address) not in AINET_VECTOR_EXCEPTIONS:
            return False
    if "source-faithful-inline-asm" in tags and label != "__asm":
        return False
    if "hard-byte-naked" in tags:
        if (rel, address) not in HARD_BYTE_NAKED_EXCEPTIONS:
            return False
        if label not in HARD_BYTE_NAKED_LABELS:
            return False
    return bool(tags)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default="src", help="production source root to scan")
    parser.add_argument(
        "--allowlist",
        default=".agent/RAW_ASSEMBLY_ALLOWLIST.txt",
        help="documented raw-assembly exceptions",
    )
    args = parser.parse_args()

    repo_root = REPO_ROOT
    scan_root = (repo_root / args.root).resolve()
    allowlist = load_allowlist(repo_root / args.allowlist) if args.allowlist else {}
    violations: list[tuple[str, int, str, str]] = []

    paths = list(iter_source_files(scan_root, suffixes=ASM_SUFFIXES.union(SOURCE_SUFFIXES)))
    source_files: list[tuple[Path, str, str]] = []
    for path in paths:
        if path.suffix.lower() in {suffix.lower() for suffix in ASM_SUFFIXES}:
            continue
        rel = display_path(path, repo_root, fallback_root=scan_root)
        source_files.append(
            (path, rel, path.read_text(encoding="utf-8", errors="ignore"))
        )
    stable_regions, region_errors = raw_region_inventory(
        source_files,
        allowlist,
        require_all_allowlist_regions=(
            scan_root == (repo_root / "src").resolve()
        ),
    )
    for error in region_errors:
        rel, _, detail = error.partition(":")
        violations.append((rel, 1, "raw-region-contract", detail.strip() or error))

    source_text_by_path = {path: (rel, text) for path, rel, text in source_files}
    for path in paths:
        rel = display_path(path, repo_root, fallback_root=scan_root)
        if path.suffix.lower() in {suffix.lower() for suffix in ASM_SUFFIXES}:
            tags = allowlist.get(
                (normalize_key_path(rel), "coff-alias", ".asm")
            )
            if tags == {"coff-weak-alias-only"}:
                try:
                    _externs, aliases = parse_alias_source_text(
                        path.read_text(encoding="utf-8", errors="strict"),
                        path=path,
                    )
                    if not aliases:
                        raise ValueError("no ALIAS rows were declared")
                except (OSError, UnicodeError, ValueError) as exc:
                    violations.append(
                        (rel, 1, "coff-weak-alias-only", str(exc))
                    )
            else:
                violations.append((rel, 1, path.suffix, "checked-in assembly source file"))
            continue

        _stored_rel, text = source_text_by_path[path]
        stripped = strip_comments_and_strings(text)
        lines = text.splitlines()
        rel_key = normalize_key_path(rel)
        try:
            source_trace = parse_source_trace_text(text, path=rel)
        except ValueError:
            source_trace = None
        for label, pattern in FORBIDDEN_PATTERNS:
            for match in pattern.finditer(stripped):
                line_no = stripped.count("\n", 0, match.start()) + 1
                if stable_region_allows(
                    rel=rel_key,
                    label=label,
                    offset=match.start(),
                    regions=stable_regions,
                    allowlist=allowlist,
                ):
                    continue
                if is_allowlisted_raw_assembly(
                    rel_key,
                    label,
                    source_trace,
                    text,
                    match.start(),
                    lines,
                    line_no,
                    allowlist,
                ):
                    continue
                violations.append((rel, line_no, label, lines[line_no - 1].strip()))

    if violations:
        print("Raw assembly and naked functions are not allowed in production source.")
        print("Only documented, address-scoped, source-faithful raw assembly exceptions may be allowlisted.")
        print(
            "Prefer C/C++ source; allow minimal inline __asm only after byte evidence, "
            "failed C/C++ routes, or chatgpt-pro-line confirmation."
        )
        print()
        for rel, line_no, label, line in violations:
            print(f"{rel}:{line_no}: {label}: {line}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
