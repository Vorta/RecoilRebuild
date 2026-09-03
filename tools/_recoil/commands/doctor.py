#!/usr/bin/env python3
"""Run Recoil workspace health checks sequentially."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys
import time

from _recoil.lib.tooling import REPO_ROOT, configure_stdio


def _command(*args: str) -> list[str]:
    return [sys.executable, str(REPO_ROOT / "tools/recoil.py"), *args]


def _steps() -> list[tuple[str, list[str]]]:
    steps: list[tuple[str, list[str]]] = [
        ("agent surface", _command("audit", "agent-surface", "--strict")),
        ("serial pipeline contract", _command("audit", "pipeline-contracts", "--strict")),
        ("serial pipeline reachability", _command("audit", "pipeline-reachability", "--strict")),
        ("issue ledger", _command("issue", "audit", "--strict", "--json")),
        ("progress tracker", _command("progress", "audit", "--scope", "pipeline", "--strict", "--json")),
        ("live validation surface", _command("audit", "live-validation-surface", "--strict")),
        ("source policy", _command("audit", "source-policy")),
    ]
    return steps


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(description="Run Recoil health checks one at a time.")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)

    results: list[dict[str, object]] = []
    for label, command in _steps():
        started = time.monotonic()
        completed = subprocess.run(
            command,
            cwd=REPO_ROOT,
            text=True,
            encoding="utf-8",
            errors="replace",
            capture_output=True,
            check=False,
        )
        row = {
            "label": label,
            "passed": completed.returncode == 0,
            "returncode": completed.returncode,
            "duration_seconds": round(time.monotonic() - started, 3),
            "command": command,
            "stdout": completed.stdout.strip(),
            "stderr": completed.stderr.strip(),
        }
        results.append(row)
        if not args.json:
            print(f"{'PASS' if row['passed'] else 'FAIL'} {label}")
            if not row["passed"]:
                if row["stdout"]:
                    print(row["stdout"])
                if row["stderr"]:
                    print(row["stderr"], file=sys.stderr)
        if not row["passed"]:
            break

    payload = {
        "passed": all(bool(row["passed"]) for row in results),
        "mode": "infrastructure",
        "execution": "sequential-fail-fast",
        "results": results,
    }
    if args.json:
        print(json.dumps(payload, indent=2))
    return 0 if payload["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
