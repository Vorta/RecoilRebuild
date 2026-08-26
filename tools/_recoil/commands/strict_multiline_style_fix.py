#!/usr/bin/env python3
"""Expand multi-argument C/C++ callable syntax for Recoil source style."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import pathlib
import re
import sys


SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".inl"}
SKIP_CALLEES = {
    "if",
    "for",
    "while",
    "switch",
    "catch",
    "sizeof",
    "offsetof",
    "RECOIL_STATIC_ASSERT",
}
SKIP_PREFIXES = ("#define", "#if", "#elif", "#include", "#pragma")


def leading_spaces(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def split_top_level_args(text: str) -> list[str] | None:
    args: list[str] = []
    start = 0
    paren = 0
    bracket = 0
    brace = 0
    angle = 0
    i = 0
    in_string: str | None = None
    in_block_comment = False
    while i < len(text):
        ch = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ""
        if in_block_comment:
            if ch == "*" and nxt == "/":
                in_block_comment = False
                i += 2
                continue
            i += 1
            continue
        if in_string:
            if ch == "\\":
                i += 2
                continue
            if ch == in_string:
                in_string = None
            i += 1
            continue
        if ch == "/" and nxt == "*":
            in_block_comment = True
            i += 2
            continue
        if ch == "/" and nxt == "/":
            break
        if ch in ('"', "'"):
            in_string = ch
            i += 1
            continue
        if ch == "(":
            paren += 1
        elif ch == ")":
            if paren > 0:
                paren -= 1
        elif ch == "[":
            bracket += 1
        elif ch == "]":
            if bracket > 0:
                bracket -= 1
        elif ch == "{":
            brace += 1
        elif ch == "}":
            if brace > 0:
                brace -= 1
        elif ch == "<":
            if i > 0 and (text[i - 1].isalnum() or text[i - 1] in "_>:"):
                angle += 1
        elif ch == ">":
            if angle > 0:
                angle -= 1
        elif ch == "," and paren == 0 and bracket == 0 and brace == 0 and angle == 0:
            arg = text[start:i].strip()
            if not arg:
                return None
            args.append(arg)
            start = i + 1
        i += 1
    if not args:
        return None
    arg = text[start:].strip()
    if not arg:
        return None
    args.append(arg)
    return args


def find_matching_close(line: str, open_index: int) -> int | None:
    depth = 0
    in_string: str | None = None
    i = open_index
    while i < len(line):
        ch = line[i]
        if in_string:
            if ch == "\\":
                i += 2
                continue
            if ch == in_string:
                in_string = None
            i += 1
            continue
        if ch in ('"', "'"):
            in_string = ch
            i += 1
            continue
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return None


def is_inside_string_or_comment(line: str, position: int) -> bool:
    in_string: str | None = None
    in_block_comment = False
    i = 0
    while i < position:
        ch = line[i]
        nxt = line[i + 1] if i + 1 < len(line) else ""
        if in_block_comment:
            if ch == "*" and nxt == "/":
                in_block_comment = False
                i += 2
                continue
            i += 1
            continue
        if in_string:
            if ch == "\\":
                i += 2
                continue
            if ch == in_string:
                in_string = None
            i += 1
            continue
        if ch == "/" and nxt == "*":
            in_block_comment = True
            i += 2
            continue
        if ch == "/" and nxt == "/":
            return True
        if ch in ('"', "'"):
            in_string = ch
        i += 1
    return in_string is not None or in_block_comment


def callee_name(prefix: str) -> str:
    match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*$", prefix)
    return match.group(1) if match else ""


def is_candidate(prefix: str, args: str, suffix: str) -> bool:
    stripped_prefix = prefix.strip()
    if not stripped_prefix:
        return False
    for skip_prefix in SKIP_PREFIXES:
        if stripped_prefix.startswith(skip_prefix):
            return False
    name = callee_name(stripped_prefix)
    if name in SKIP_CALLEES:
        return False
    if stripped_prefix.endswith(("return", "new", "delete", "case")):
        return False
    if not suffix.strip().startswith(
        (
            ";",
            "{",
            "}",
            ")",
            "]",
            ",",
            ".",
            "->",
            "const",
            "override",
            "=",
            "!",
            "<",
            ">",
            "&",
            "|",
            "?",
            ":",
            "*",
            "/",
            "+",
            "-",
        )
    ):
        return False
    return split_top_level_args(args) is not None


def rewrite_single_line(line: str) -> list[str] | None:
    if "//" in line or "/*" in line:
        return None
    base_indent = leading_spaces(line)
    open_index = line.find("(")
    while open_index >= 0:
        if is_inside_string_or_comment(line, open_index):
            open_index = line.find("(", open_index + 1)
            continue
        close_index = find_matching_close(line, open_index)
        if close_index is None:
            return None
        prefix = line[:open_index]
        args_text = line[open_index + 1 : close_index]
        suffix = line[close_index + 1 :]
        args = split_top_level_args(args_text)
        if args and is_candidate(prefix, args_text, suffix):
            indent = " " * base_indent
            arg_indent = indent + "    "
            return [prefix.rstrip() + "("] + [arg_indent + arg + "," for arg in args[:-1]] + [
                arg_indent + args[-1],
                indent + ")" + suffix,
            ]
        open_index = line.find("(", open_index + 1)
    return None


def rewrite_file(path: pathlib.Path) -> bool:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    out: list[str] = []
    changed = False
    formatter_off = False
    for line in lines:
        if "clang-format off" in line:
            formatter_off = True
        if formatter_off:
            out.append(line)
            if "clang-format on" in line:
                formatter_off = False
            continue
        rewritten = rewrite_single_line(line)
        if rewritten is None:
            out.append(line)
        else:
            out.extend(rewritten)
            changed = True
    if changed:
        path.write_text("\n".join(out) + "\n", encoding="utf-8", newline="\n")
    return changed


def main(argv: list[str]) -> int:
    root = pathlib.Path(argv[0] if argv else "src")
    changed = 0
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES:
            if rewrite_file(path):
                changed += 1
    print(f"strict multiline post-pass changed {changed} files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
