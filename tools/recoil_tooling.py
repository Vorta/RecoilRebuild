from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass
import os
from pathlib import Path
import subprocess
import sys
from typing import Any, TextIO


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_VC6_ROOT = REPO_ROOT.parent / "Compiler" / "VC6"

SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".inl"})
ASM_SUFFIXES = frozenset({".asm", ".s", ".S"})


@dataclass(frozen=True)
class CommandScriptResult:
    returncode: int
    script_path: Path
    stdout: str = ""
    stderr: str = ""


def configure_stdio() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8")


def require_string(data: dict[str, Any], key: str, *, manifest_path: Path) -> str:
    value = data.get(key)
    if not isinstance(value, str) or not value:
        raise ValueError(f"{manifest_path}: expected non-empty string field '{key}'")
    return value


def require_string_list(
    data: dict[str, Any],
    key: str,
    *,
    manifest_path: Path,
    allow_empty_items: bool = False,
) -> tuple[str, ...]:
    value = data.get(key, [])
    if not isinstance(value, list):
        raise ValueError(f"{manifest_path}: expected string list field '{key}'")
    result: list[str] = []
    for item in value:
        if not isinstance(item, str) or (not allow_empty_items and not item):
            raise ValueError(f"{manifest_path}: expected string list field '{key}'")
        result.append(item)
    return tuple(result)


def optional_bool(data: dict[str, Any], key: str, default: bool, *, manifest_path: Path) -> bool:
    value = data.get(key, default)
    if not isinstance(value, bool):
        raise ValueError(f"{manifest_path}: expected boolean field '{key}'")
    return value


def expand_config_path(path_text: str | Path) -> str:
    text = str(path_text)
    vc6_root = os.environ.get("RECOIL_VC6_ROOT", str(DEFAULT_VC6_ROOT))
    text = text.replace("${RECOIL_VC6_ROOT}", vc6_root)
    text = text.replace("%RECOIL_VC6_ROOT%", vc6_root)
    return os.path.expandvars(text)


def repo_path(path_text: str | Path) -> Path:
    path = Path(expand_config_path(path_text))
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def display_path(path: Path, root: Path = REPO_ROOT, *, fallback_root: Path | None = None) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        if fallback_root is not None:
            try:
                return path.relative_to(fallback_root).as_posix()
            except ValueError:
                pass
        return str(path).replace("\\", "/")


def quote_cmd_arg(value: str | Path) -> str:
    text = str(value)
    if '"' in text:
        raise ValueError(f"Command argument contains an unsupported quote: {text}")
    return f'"{text}"'


def response_line(value: str | Path) -> str:
    return quote_cmd_arg(value) if any(char.isspace() for char in str(value)) else str(value)


def hidden_creation_flags() -> int:
    if sys.platform == "win32":
        return subprocess.CREATE_NO_WINDOW  # type: ignore[attr-defined]
    return 0


def write_cmd_script(path: Path, command: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("@echo off\r\n" + command + "\r\n", encoding="ascii")


def run_cmd_script(
    command: str,
    *,
    cwd: Path,
    script_name: str = "_run_cmd.cmd",
    stdout: TextIO | None = None,
    stderr: TextIO | None = None,
    capture_output: bool = False,
) -> CommandScriptResult:
    script_path = cwd / script_name
    write_cmd_script(script_path, command)
    comspec = os.environ.get("ComSpec", r"C:\Windows\System32\cmd.exe")
    completed = subprocess.run(
        [comspec, "/d", "/c", str(script_path)],
        cwd=str(cwd),
        text=True,
        stdout=subprocess.PIPE if capture_output else stdout,
        stderr=subprocess.PIPE if capture_output else stderr,
        creationflags=hidden_creation_flags(),
    )
    return CommandScriptResult(
        returncode=completed.returncode,
        script_path=script_path,
        stdout=completed.stdout or "",
        stderr=completed.stderr or "",
    )


def write_if_changed(path: Path, content: bytes) -> bool:
    if path.exists() and path.read_bytes() == content:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(content)
    return True


def write_text_if_changed(path: Path, content: str, *, encoding: str = "utf-8") -> bool:
    return write_if_changed(path, content.encode(encoding))


def iter_source_files(root: Path, *, suffixes: Iterable[str] = SOURCE_SUFFIXES) -> list[Path]:
    normalized_suffixes = {suffix.lower() for suffix in suffixes}
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.suffix.lower() in normalized_suffixes
    )


def strip_comments_and_strings(text: str) -> str:
    result: list[str] = []
    i = 0
    state = "code"
    while i < len(text):
        ch = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ""

        if state == "code":
            if ch == "/" and nxt == "/":
                result.extend("  ")
                i += 2
                state = "line_comment"
            elif ch == "/" and nxt == "*":
                result.extend("  ")
                i += 2
                state = "block_comment"
            elif ch == '"':
                result.append(" ")
                i += 1
                state = "string"
            elif ch == "'":
                result.append(" ")
                i += 1
                state = "char"
            else:
                result.append(ch)
                i += 1
        elif state == "line_comment":
            if ch == "\n":
                result.append(ch)
                state = "code"
            else:
                result.append(" ")
            i += 1
        elif state == "block_comment":
            if ch == "*" and nxt == "/":
                result.extend("  ")
                i += 2
                state = "code"
            else:
                result.append("\n" if ch == "\n" else " ")
                i += 1
        elif state == "string":
            if ch == "\\":
                result.extend("  " if nxt else " ")
                i += 2 if nxt else 1
            elif ch == '"':
                result.append(" ")
                i += 1
                state = "code"
            else:
                result.append("\n" if ch == "\n" else " ")
                i += 1
        elif state == "char":
            if ch == "\\":
                result.extend("  " if nxt else " ")
                i += 2 if nxt else 1
            elif ch == "'":
                result.append(" ")
                i += 1
                state = "code"
            else:
                result.append("\n" if ch == "\n" else " ")
                i += 1

    return "".join(result)
