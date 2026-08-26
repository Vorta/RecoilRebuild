from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
import re
import sys
from pathlib import Path

from _recoil.lib.tooling import REPO_ROOT, strip_comments_and_strings


DEFAULT_SOURCE = REPO_ROOT / "src" / "GameZRecoil" / "zInterp" / "zinterp_parse.cpp"
DEFAULT_HLIL = REPO_ROOT / "export" / "text.hlil.txt"
DEFAULT_DATA = REPO_ROOT / "export" / "data.linear.txt"

DISPATCH_START = "zInterp_Context::DispatchCoreCommand"
HLIL_START = "0x4c20a0 int32_t __thiscall zInterp_Context::DispatchCoreCommand"
HLIL_END = "0x4c5480"
DATA_START = 0x4E4A30
DATA_END = 0x4E5948
ORIGINAL_ONLY_TOKENS = {"Facade", "SEQ", "linear", "exponential"}


def _slice_between(text: str, start_marker: str, end_marker: str | None) -> str:
    start = text.index(start_marker)
    if end_marker is None:
        return text[start:]
    end = text.index(end_marker, start)
    return text[start:end]


def _source_function_body(text: str, name: str) -> str:
    """Return one lexical function definition without comment-marker sentinels."""

    stripped = strip_comments_and_strings(text)
    matches = list(re.finditer(rf"\b{re.escape(name)}\s*\(", stripped))
    bodies: list[tuple[int, int]] = []
    for match in matches:
        close = stripped.find(")", match.end())
        if close < 0:
            continue
        open_brace = stripped.find("{", close)
        semicolon = stripped.find(";", close)
        if open_brace < 0 or (semicolon >= 0 and semicolon < open_brace):
            continue
        depth = 0
        for index in range(open_brace, len(stripped)):
            if stripped[index] == "{":
                depth += 1
            elif stripped[index] == "}":
                depth -= 1
                if depth == 0:
                    bodies.append((match.start(), index + 1))
                    break
    if len(bodies) != 1:
        raise ValueError(f"{name} must resolve to exactly one lexical function body; found {len(bodies)}")
    start, end = bodies[0]
    return text[start:end]


def parse_source_commands(source_text: str) -> tuple[list[str], set[str], set[str], set[str]]:
    dispatch = _source_function_body(source_text, DISPATCH_START)
    ordered = [
        match.group(2)
        for match in re.finditer(
            r"Command(IsExact|Is|HasPrefix)\(this,\s*\"([^\"]+)\"", dispatch
        )
    ]
    prefix = {
        match.group(2)
        for match in re.finditer(r"Command(Is|HasPrefix)\(this,\s*\"([^\"]+)\"", dispatch)
    }
    exact = {
        match.group(1)
        for match in re.finditer(r"CommandIsExact\(this,\s*\"([^\"]+)\"", dispatch)
    }
    return ordered, set(ordered), prefix, exact


def parse_original_data_commands(data_text: str) -> set[str]:
    result: set[str] = set()
    for line in data_text.splitlines():
        match = re.match(r"0x([0-9a-fA-F]+).*?= \"(.*?)\", 0", line)
        if match is None:
            continue
        address = int(match.group(1), 16)
        if not (DATA_START <= address <= DATA_END):
            continue
        literal = match.group(2)
        if re.match(r"^[A-Za-z#][A-Za-z0-9#]+$", literal):
            result.add(literal)
    return result


def parse_original_match_modes(hlil_text: str) -> tuple[list[str], set[str], set[str]]:
    dispatch = _slice_between(hlil_text, HLIL_START, HLIL_END)
    ordered: list[str] = []
    for match in re.finditer(
        r"(?:CommandEquals(?:Prefix)?\(self,\s*|strncmp\([^\n]*?,\s*)\"([^\"]+)\"",
        dispatch,
    ):
        ordered.append(match.group(1))
    for match in re.finditer(r"char\* [A-Za-z0-9_]+ = \"([^\"]+)\"", dispatch):
        if match.group(1) not in ordered:
            ordered.append(match.group(1))

    prefix = set(re.findall(r"CommandEqualsPrefix\(self,\s*\"([^\"]+)\"", dispatch))
    prefix.update(re.findall(r"strncmp\([^\n]*?,\s*\"([^\"]+)\",\s*(?:0x[0-9a-fA-F]+|[0-9]+)\)", dispatch))

    exact = set(re.findall(r"CommandEquals\(self,\s*\"([^\"]+)\"", dispatch))
    for match in re.finditer(r"char\* [A-Za-z0-9_]+ = \"([^\"]+)\"", dispatch):
        nearby = dispatch[max(0, match.start() - 200) : match.start() + 200]
        if "strncmp" not in nearby:
            exact.add(match.group(1))
    return ordered, prefix, exact


def find_prefix_order_mismatches(
    source_order: list[str], original_order: list[str], source_prefix: set[str]
) -> set[str]:
    source_index = {value: index for index, value in enumerate(source_order)}
    original_index = {value: index for index, value in enumerate(original_order)}
    mismatches: set[str] = set()
    commands = sorted(set(source_order) & set(original_order) & source_prefix)
    for short in commands:
        for long in commands:
            if short == long or not long.startswith(short):
                continue
            if source_index[short] < source_index[long] and original_index[long] < original_index[short]:
                mismatches.add(f"{short} before {long}")
    return mismatches


def render_set(title: str, values: set[str]) -> list[str]:
    if not values:
        return [f"{title}: none"]
    lines = [f"{title}: {len(values)}"]
    lines.extend(f"  - {value}" for value in sorted(values))
    return lines


def require_export_input(path: Path, label: str) -> str:
    if path.exists():
        return path.read_text(encoding="utf-8", errors="replace")
    print(
        f"{label} export not found: {path}. "
        "This watcher-facing audit needs local Binary Ninja export dumps; "
        "normal reconstruction checks do not require them.",
        file=sys.stderr,
    )
    raise FileNotFoundError(path)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Audit zInterp DispatchCoreCommand command coverage and match modes "
            "against optional watcher-facing Binary Ninja export dumps."
        )
    )
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--hlil", type=Path, default=DEFAULT_HLIL)
    parser.add_argument("--data", type=Path, default=DEFAULT_DATA)
    args = parser.parse_args(argv)

    source_order, source_all, source_prefix, source_exact = parse_source_commands(
        args.source.read_text(encoding="utf-8")
    )
    try:
        data_text = require_export_input(args.data, "data")
        hlil_text = require_export_input(args.hlil, "HLIL")
    except FileNotFoundError:
        return 2

    original_data = parse_original_data_commands(data_text)
    original_order, original_prefix, original_exact = parse_original_match_modes(hlil_text)

    original_commands = original_data - ORIGINAL_ONLY_TOKENS
    missing_in_source = original_commands - source_all
    source_not_in_original = source_all - original_data
    exact_but_original_prefix = source_exact & original_prefix
    prefix_but_original_exact = source_prefix & original_exact
    prefix_order_mismatches = find_prefix_order_mismatches(
        source_order, original_order, source_prefix
    )

    print("# zInterp DispatchCoreCommand export audit")
    print(f"source commands: {len(source_all)}")
    print(f"source prefix commands: {len(source_prefix)}")
    print(f"source exact commands: {len(source_exact)}")
    print(f"original data command candidates: {len(original_commands)}")
    print()
    for line in render_set("missing in source", missing_in_source):
        print(line)
    for line in render_set("source not in original data", source_not_in_original):
        print(line)
    for line in render_set("source exact but original prefix", exact_but_original_prefix):
        print(line)
    for line in render_set("source prefix but original exact", prefix_but_original_exact):
        print(line)
    for line in render_set("prefix order mismatches", prefix_order_mismatches):
        print(line)

    failed = (
        missing_in_source
        or source_not_in_original
        or exact_but_original_prefix
        or prefix_but_original_exact
        or prefix_order_mismatches
    )
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
