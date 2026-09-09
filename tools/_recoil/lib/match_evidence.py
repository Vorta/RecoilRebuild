"""Match evidence and source mirrors, independent of serial-stage acceptance."""
from __future__ import annotations

from functools import lru_cache
import json
from pathlib import Path
import re
from typing import Any, Mapping

from _recoil.lib.function_match import MATCH_LEVELS, MATCH_VERSION
from _recoil.lib.tooling import REPO_ROOT, DEFAULT_VC5_ROOT

_TAG_LINE = re.compile(r"^[ \t]*\*[ \t]*@recoil-match[^\r\n]*(?:\r?\n|$)", re.MULTILINE)
_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^">]+)[">]', re.MULTILINE)


def strip_match_annotations(text: str) -> str:
    # Restore a blank comment line rather than deleting it: __LINE__ is code.
    return re.sub(r"(?m)^([ \t]*\*)(?:[ \t]+@recoil-match[^\r\n]*?)([ \t]*\*/)?(?=\r?$)",
                  lambda m: m[1] + "/" if m[2] else m[1], text)


def _text(path: Path) -> str:
    data = path.read_bytes()
    try:
        return data.decode("utf-8-sig")
    except UnicodeDecodeError:
        return data.decode("cp1252")


def source_context(source: str, config: Any) -> dict[str, Any]:
    """Capture review context directly; contents are not acceptance tokens.

    Includes are conservatively traversed across preprocessor branches. Match
    annotation lines alone are excluded so synchronizing a mirror is inert.
    """
    roots = [Path(x) for x in config.include_dirs]
    roots += [Path(DEFAULT_VC5_ROOT) / "VC" / "INCLUDE"]
    pending = [(REPO_ROOT / source).resolve()]
    files: dict[str, str] = {}
    while pending:
        path = pending.pop()
        try:
            key = path.relative_to(REPO_ROOT).as_posix()
        except ValueError:
            key = path.as_posix()
        if key in files:
            continue
        contents = _text(path)
        from _recoil.lib.source_traceability import _scan_comments
        normalized = strip_match_annotations(contents)
        for comment in reversed(_scan_comments(normalized)):
            normalized = normalized[:comment.start] + "".join(c for c in comment.text if c in "\r\n") + normalized[comment.end:]
        files[key] = normalized.replace("\r\n", "\n")
        for include in _INCLUDE.findall(contents):
            target = next((p.resolve() for root in [path.parent, *roots]
                           if (p := root / include).is_file()), None)
            if target is not None:
                pending.append(target)
    return {"version": MATCH_VERSION, "source": source, "files": dict(sorted(files.items())),
            "compile_flags": list(config.compile_flags), "compile_profile": config.compile_profile,
            "source_compile_profiles": dict(config.source_compile_profiles),
            "include_dirs": [str(p) for p in config.include_dirs],
            "link_flags": list(config.link_flags), "link_profile": config.link_profile,
            "library_profile": config.library_profile}


def dependency_states(paths: list[str]) -> list[dict[str, Any]]:
    result = []
    for name in sorted(set(paths)):
        path = REPO_ROOT / name
        try:
            stat = path.stat()
            result.append({"path": name, "size": stat.st_size, "mtime_ns": stat.st_mtime_ns})
        except OSError:
            result.append({"path": name, "missing": True})
    return result


def review_current(review: Any, context: Mapping[str, Any], differences: list[dict[str, Any]]) -> bool:
    return (isinstance(review, Mapping) and review.get("version") == MATCH_VERSION
            and review.get("decision") == "compiler-register-allocation-only"
            and review.get("no_remaining_credible_source_options") is True
            and bool(review.get("evidence_id"))
            and review.get("context") == context and review.get("differences") == differences)


def current_match_level(symbol: Mapping[str, Any], *, check_files: bool = True) -> str | None:
    state = symbol.get("function_match")
    if not isinstance(state, Mapping) or state.get("version") != MATCH_VERSION:
        return None
    if (state.get("level") not in MATCH_LEVELS or state.get("freshness") != "current"
            or state.get("validation_mode") != "live" or not state.get("evidence_ids")):
        return None
    dependencies = state.get("dependencies")
    if check_files and (not isinstance(dependencies, list) or not dependencies
                        or dependency_states([p["path"] for p in dependencies]) != dependencies):
        return None
    if state["level"] == "instruction":
        review = symbol.get("instruction_match_review")
        if (not isinstance(review, Mapping) or review.get("version") != MATCH_VERSION
                or state.get("review_evidence_id") != review.get("evidence_id")
                or review.get("decision") != "compiler-register-allocation-only"
                or review.get("no_remaining_credible_source_options") is not True):
            return None
    return str(state["level"])


def stage_match_current(symbol: Mapping[str, Any], mode: str, accepted) -> bool:
    """Alternative stage predicate; exact byte states keep their old meaning."""
    from _recoil.lib.progress import AUTHORED_BYTE_DIMENSIONS, EXACT_LINK_DIMENSIONS
    state = symbol.get("binary_state", {})
    exact = AUTHORED_BYTE_DIMENSIONS if mode == "authored" else EXACT_LINK_DIMENSIONS
    if all(accepted(state.get(dimension)) for dimension in exact):
        return True
    if current_match_level(symbol) != "instruction":
        return False
    required = (("relocation_identity", "linked_presence", "linked_target_identity",
                 "object_instruction", "linked_body_instruction") if mode == "authored"
                else ("linked_address", "linked_targets", "linked_instruction"))
    return all(accepted(state.get(dimension)) for dimension in required)


def annotation_edits(documents, symbols: Mapping[str, Any]) -> tuple[list[dict[str, Any]], list[dict[str, str]]]:
    """Prepare guarded comment-only edits; never create source bodies/owners."""
    edits, excluded = [], []
    for document in documents:
        path = REPO_ROOT / document.path
        original = path.read_bytes()
        encoding = "utf-8-sig" if original.startswith(b"\xef\xbb\xbf") else "utf-8"
        try:
            text = original.decode(encoding)
        except UnicodeDecodeError:
            encoding = "cp1252"; text = original.decode(encoding)
        changes = []
        for anchor in document.anchors:
            if anchor.construct is None or anchor.construct.kind != "function":
                continue
            identities = [item.artifact_id for item in document.artifacts
                          if item.anchor_id == anchor.anchor_id and item.relation == "defines"
                          and item.section == ".text" and item.entity_kind == "function"]
            if not identities:
                continue
            levels = [current_match_level(symbols.get(identity, {})) for identity in identities]
            level = None if any(x is None for x in levels) else "instruction" if "instruction" in levels else "byte"
            comment = text[anchor.comment_start:anchor.comment_end]
            replacement = strip_match_annotations(comment)
            if level:
                # Reserve a separate blank row after the annotation group.
                # Never consume the prose separator or share the closing */.
                # New annotations without room are excluded rather than moving
                # source lines (which could change an expanded __LINE__).
                prefix_end = re.search(r"(?m)^[ \t]*\*[ \t]+(?!@|[ \t]*$)\S", replacement)
                prefix = replacement[:prefix_end.start()] if prefix_end else ""
                annotations = list(re.finditer(r"(?m)^[ \t]*\*[ \t]+@[^\r\n]*", prefix))
                blanks = list(re.finditer(r"(?m)^[ \t]*\*[ \t]*(?=\r?$)", prefix))
                if annotations and len(blanks) >= 2 and blanks[-1].start() > annotations[-1].end():
                    start, end = blanks[0].span()
                    indentation = re.match(r"[ \t]*", replacement[start:end]).group()
                    replacement = replacement[:start] + f"{indentation}* @recoil-match {level}" + replacement[end:]
                else:
                    excluded.extend({"symbol_id": identity, "reason": "no line-preserving annotation position with prose separator"} for identity in identities)
            if replacement != comment:
                changes.append((anchor.comment_start, anchor.comment_end, replacement))
            if not level:
                excluded.extend({"symbol_id": identity, "reason": "no complete current function-match proof"} for identity in identities)
        for start, end, replacement in sorted(changes, reverse=True):
            text = text[:start] + replacement + text[end:]
        if changes:
            edits.append({"path": document.path, "before": original, "after": text.encode(encoding), "annotations_changed": len(changes)})
    return edits, excluded


def annotation_findings(document, symbols):
    from _recoil.lib.source_traceability import SourceTraceFinding
    for match in document.matches:
        levels = [current_match_level(symbols.get(identity, {})) for identity in match.artifact_ids]
        expected = None if any(x is None for x in levels) else "instruction" if "instruction" in levels else "byte"
        if expected != match.level:
            yield SourceTraceFinding("match-evidence-disagreement", document.path, match.line,
                f"@recoil-match {match.level} disagrees with current complete proof {expected!r}", match.anchor_id)
