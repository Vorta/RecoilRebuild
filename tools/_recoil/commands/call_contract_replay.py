"""CLI presentation for the governed full-census call-contract replay."""

from __future__ import annotations

import argparse
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import sys
from typing import Any

from _recoil.commands.progress_cli import replay_live_call_contract
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressError
from _recoil.lib.tooling import configure_stdio


def _stderr_status(message: str) -> None:
    print(message, file=sys.stderr, flush=True)


def replay_live(
    args: argparse.Namespace,
    *,
    status_sink: Any | None = None,
) -> tuple[int, dict[str, Any]]:
    """Retain the public entry point while delegating all proof decisions."""

    return replay_live_call_contract(
        args,
        status_sink=status_sink or _stderr_status,
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Prove the complete authored call-contract census once, then "
            "serially commit its original-slice projections."
        )
    )
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument(
        "--dry-run",
        action="store_true",
        help="Plan the full-census replay without building, reading BN, or mutating.",
    )
    mode.add_argument(
        "--apply",
        action="store_true",
        help="Run one fresh proof and serially apply governed slice receipts.",
    )
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    incidental_stdout = io.StringIO()
    try:
        with redirect_stdout(incidental_stdout):
            returncode, payload = replay_live(args)
        incidental = incidental_stdout.getvalue()
        if incidental:
            print(incidental, file=sys.stderr, end="")
        print(json.dumps(payload, indent=2, ensure_ascii=False))
        return returncode
    except KeyboardInterrupt:
        print("call-contract replay interrupted", file=sys.stderr)
        return 130
    except (OSError, ProgressError, ValueError) as exc:
        incidental = incidental_stdout.getvalue()
        if incidental:
            print(incidental, file=sys.stderr, end="")
        print(f"call-contract replay error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
