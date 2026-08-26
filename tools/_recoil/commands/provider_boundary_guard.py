#!/usr/bin/env python3
"""Fail on fake provider/framework reimplementations and shims in production source."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from collections import Counter
import re
import sys
import tempfile
from pathlib import Path


SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl"}

IMPORTED_FRAMEWORK_CLASSES = (
    "CWnd",
    "CDialog",
    "CMenu",
    "CWinApp",
    "CButton",
    "CEdit",
    "CListBox",
    "CComboBox",
    "CSpinButtonCtrl",
    "CCmdUI",
)

FAKE_MFC_NAMES_RE = re.compile(
    r"\b(?:MfcWndStorage|WestwoodOnlineMfcWndStorage|kMfcC[A-Za-z0-9_]*_Vtable)\b"
)

IMPORTED_FRAMEWORK_DECL_RE = re.compile(
    r"\b(?:class|struct)\s+("
    + "|".join(re.escape(name) for name in IMPORTED_FRAMEWORK_CLASSES)
    + r")\b"
)

DIRECTX_VTABLE_RE = re.compile(
    r"\b(?:class|struct)\s+"
    r"(?:DirectSound|DirectDraw|DirectInput)[A-Za-z0-9_]*VTable\b"
)

PROVIDER_VTABLE_DECL_RE = re.compile(
    r"\b(?:class|struct)\s+([A-Za-z0-9_]*Provider[A-Za-z0-9_]*VTable)\b"
)

PROVIDER_SHIM_COMMENT_RE = re.compile(
    r"\b(?:access|provider|imported|vendor|mfc|directx|aureal|westwood|cpu)\b"
    r".{0,120}\bshim\b|\bshim\b.{0,120}\b(?:provider|imported|vendor|mfc|"
    r"directx|aureal|westwood|cpu)\b",
    re.IGNORECASE,
)


def strip_comments_and_strings(text: str) -> str:
    result: list[str] = []
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if ch == "/" and nxt == "/":
            result.extend("  ")
            i += 2
            while i < n and text[i] not in "\r\n":
                result.append(" ")
                i += 1
            continue
        if ch == "/" and nxt == "*":
            result.extend("  ")
            i += 2
            while i < n:
                if text[i] == "*" and i + 1 < n and text[i + 1] == "/":
                    result.extend("  ")
                    i += 2
                    break
                result.append("\n" if text[i] == "\n" else " ")
                i += 1
            continue
        if ch in ("'", '"'):
            quote = ch
            result.append(" ")
            i += 1
            while i < n:
                if text[i] == "\\":
                    result.append(" ")
                    i += 1
                    if i < n:
                        result.append("\n" if text[i] == "\n" else " ")
                        i += 1
                    continue
                if text[i] == quote:
                    result.append(" ")
                    i += 1
                    break
                result.append("\n" if text[i] == "\n" else " ")
                i += 1
            continue
        result.append(ch)
        i += 1
    return "".join(result)


def iter_source_files(root: Path) -> list[Path]:
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    )


def line_for_offset(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def display_path(path: Path, repo_root: Path) -> str:
    try:
        return path.relative_to(repo_root).as_posix()
    except ValueError:
        return path.as_posix()


def find_violations(scan_root: Path, repo_root: Path) -> list[tuple[str, int, str, str]]:
    violations: list[tuple[str, int, str, str]] = []
    for path in iter_source_files(scan_root):
        text = path.read_text(encoding="utf-8", errors="ignore")
        stripped = strip_comments_and_strings(text)
        lines = text.splitlines()
        rel = display_path(path, repo_root)

        for match in PROVIDER_SHIM_COMMENT_RE.finditer(text):
            line_no = line_for_offset(text, match.start())
            violations.append(
                (
                    rel,
                    line_no,
                    "provider ABI/access shim",
                    lines[line_no - 1].strip(),
                )
            )

        for match in IMPORTED_FRAMEWORK_DECL_RE.finditer(stripped):
            line_no = line_for_offset(stripped, match.start())
            violations.append(
                (
                    rel,
                    line_no,
                    "local imported framework class declaration",
                    lines[line_no - 1].strip(),
                )
            )

        for match in FAKE_MFC_NAMES_RE.finditer(stripped):
            line_no = line_for_offset(stripped, match.start())
            violations.append(
                (
                    rel,
                    line_no,
                    "fake MFC storage or vtable marker",
                    lines[line_no - 1].strip(),
                )
            )

        for match in DIRECTX_VTABLE_RE.finditer(stripped):
            line_no = line_for_offset(stripped, match.start())
            violations.append(
                (
                    rel,
                    line_no,
                    "local DirectX provider vtable mirror",
                    lines[line_no - 1].strip(),
                )
            )

        for match in PROVIDER_VTABLE_DECL_RE.finditer(stripped):
            line_no = line_for_offset(stripped, match.start())
            violations.append(
                (
                    rel,
                    line_no,
                    "local provider vtable shim",
                    lines[line_no - 1].strip(),
                )
            )

    return violations


def print_summary(*, violations: list[tuple[str, int, str, str]], top: int) -> None:
    print("provider-boundary production-source summary:")
    print(f"- current violations: {len(violations)}")

    by_label: Counter[str] = Counter()
    by_file: Counter[str] = Counter()
    for rel, _line_no, label, _line in violations:
        by_label[label] += 1
        by_file[rel] += 1

    print(f"- top labels (limit {top}):")
    if by_label:
        for label, count in by_label.most_common(top):
            print(f"  {count:4}  {label}")
    else:
        print("     0  <none>")

    print(f"- top files (limit {top}):")
    if by_file:
        for rel, count in by_file.most_common(top):
            print(f"  {count:4}  {rel}")
    else:
        print("     0  <none>")


def run_self_tests() -> int:
    cases: list[tuple[str, str, bool]] = [
        ("fake MFC storage", "struct MfcWndStorage { void *vtable; };\n", True),
        (
            "fake MFC vtable",
            'const RecoilNamedVtable kMfcCEdit_Vtable = {"fake"};\n',
            True,
        ),
        (
            "fake DirectSound vtable",
            "struct DirectSoundBufferVTable { void *slots[4]; };\n",
            True,
        ),
        (
            "real MFC access shim",
            "// Access shim for imported MFC42 metadata; this does not reimplement behavior.\n"
            "class DialogAccessor : public CDialog { public: void CallBase(); };\n",
            True,
        ),
        (
            "documented provider shim",
            "// Imported Aureal A3D COM ABI shim. This does not reimplement Aureal behavior.\n"
            "struct zA3dProviderSourceVTable { void *QueryInterface; void *SetGain; };\n"
            "RECOIL_STATIC_ASSERT(offsetof(zA3dProviderSourceVTable, SetGain) == 4);\n",
            True,
        ),
        (
            "undocumented provider shim",
            "struct VendorProviderThingVTable { void *slot; };\n",
            True,
        ),
    ]

    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        src = root / "src"
        src.mkdir()
        for index, (_name, source, _should_fail) in enumerate(cases):
            (src / f"case_{index}.h").write_text(source, encoding="utf-8")

        all_violations = find_violations(src, root)

    failures: list[str] = []
    for index, (name, _source, should_fail) in enumerate(cases):
        rel = f"src/case_{index}.h"
        failed = any(violation[0] == rel for violation in all_violations)
        if failed != should_fail:
            failures.append(f"{name}: expected failed={should_fail}, got failed={failed}")

    if failures:
        print("provider-boundary guard self-test failures:")
        for failure in failures:
            print(f"- {failure}")
        return 1
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default="src", help="production source root to scan")
    parser.add_argument("--summary", action="store_true", help="print current provider-boundary guard usage")
    parser.add_argument("--top", type=int, default=10, help="number of labels/files to print with --summary")
    parser.add_argument("--self-test", action="store_true", help="run guard fixture tests")
    args = parser.parse_args(argv)

    if args.self_test:
        return run_self_tests()

    repo_root = Path.cwd().resolve()
    scan_root = (repo_root / args.root).resolve()
    violations = find_violations(scan_root, repo_root)
    if args.summary:
        print_summary(violations=violations, top=max(args.top, 0))

    if not violations:
        return 0

    if args.summary:
        print()
    print("Provider/framework behavior must not be reimplemented or shimmed in production source.")
    print("Use real SDK/MFC/import/provider types; otherwise leave provider layout as a blocker.")
    print()
    for rel, line_no, label, line in violations:
        print(f"{rel}:{line_no}: {label}: {line}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
