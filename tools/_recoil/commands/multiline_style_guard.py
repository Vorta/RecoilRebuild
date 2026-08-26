#!/usr/bin/env python3
"""Focused guard for multiline C/C++ wrapping regressions."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
import pathlib
import sys
import tempfile
import textwrap


SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".inl"}

FOCUSED_SYMBOLS = {
    "OnApiStatus",
    "OnDownloadReadyResult",
    "OnPendingSessionRequestRemoved",
    "OnServerError",
    "OnStatusTextReceived",
}


def leading_spaces(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def source_files(root: pathlib.Path):
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES:
            yield path


def check_file(path: pathlib.Path) -> list[str]:
    issues: list[str] = []
    lines = path.read_text(encoding="utf-8").splitlines()

    for index, line in enumerate(lines):
        stripped = line.lstrip()
        if stripped.startswith("//"):
            continue

        for symbol in FOCUSED_SYMBOLS:
            marker = symbol + "("
            symbol_index = line.find(marker)
            if symbol_index < 0:
                continue

            after_open = line[symbol_index + len(marker) :].strip()
            if after_open:
                issues.append(
                    f"{path}:{index + 1}: put the first parameter/argument after "
                    f"{symbol}( on the next line"
                )
                continue

            base_indent = leading_spaces(line)
            expected_indent = base_indent + 4
            found_close = False

            for next_index in range(index + 1, min(index + 40, len(lines))):
                next_line = lines[next_index]
                next_stripped = next_line.strip()
                if not next_stripped:
                    continue

                if next_stripped.startswith(")"):
                    found_close = True
                    if leading_spaces(next_line) != base_indent:
                        issues.append(
                            f"{path}:{next_index + 1}: put the closing parenthesis "
                            f"for {symbol}( at the same indentation as the opening line"
                        )
                    break

                if leading_spaces(next_line) != expected_indent:
                    issues.append(
                        f"{path}:{next_index + 1}: indent wrapped parameters/arguments "
                        f"for {symbol}( by exactly 4 spaces"
                    )

            if not found_close:
                issues.append(f"{path}:{index + 1}: could not find closing line for {symbol}(")

    return issues


def check_root(root: pathlib.Path) -> list[str]:
    issues: list[str] = []
    for path in source_files(root):
        issues.extend(check_file(path))
    return issues


def write(path: pathlib.Path, content: str) -> None:
    path.write_text(textwrap.dedent(content).lstrip(), encoding="utf-8")


def self_test() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = pathlib.Path(tmp)
        bad = root / "bad.h"
        good = root / "good.h"

        write(
            bad,
            """
            struct Sink
            {
                static int OnPendingSessionRequestRemoved(void *callbackContext,
                                                         int status,
                                                         WestwoodOnlineUpgradeSessionRequest *sessionRequest);
            };
            """,
        )
        write(
            good,
            """
            struct Sink
            {
                static int OnPendingSessionRequestRemoved(
                    void *callbackContext,
                    int status,
                    WestwoodOnlineUpgradeSessionRequest *sessionRequest
                );
            };
            """,
        )

        bad_issues = check_file(bad)
        good_issues = check_file(good)

        if not bad_issues:
            raise AssertionError("bad multiline style was not detected")
        if good_issues:
            raise AssertionError(f"good multiline style was rejected: {good_issues}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default="src", help="source root to scan")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)

    if args.self_test:
        self_test()
        print("recoil_multiline_style_guard self-test passed")
        return 0

    root = pathlib.Path(args.root)
    issues = check_root(root)
    if issues:
        print("recoil_multiline_style_guard failed:")
        for issue in issues:
            print(f"  {issue}")
        return 1

    print("recoil_multiline_style_guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
