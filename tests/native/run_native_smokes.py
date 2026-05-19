from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


SMOKE_ENTRY_RE = re.compile(r'\{\s*"([^"]+_smoke)"\s*,')


def load_smoke_names(smoke_cpp: Path) -> list[str]:
    text = smoke_cpp.read_text(encoding="utf-8")
    names = SMOKE_ENTRY_RE.findall(text)
    if not names:
        raise ValueError(f"no smoke entries found in {smoke_cpp}")
    return names


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Run native smoke tests in isolated processes.")
    parser.add_argument("executable", type=Path)
    parser.add_argument("--smoke-cpp", type=Path, required=True)
    parser.add_argument("--only", help="Run one smoke by name.")
    args = parser.parse_args(argv)

    names = load_smoke_names(args.smoke_cpp)
    if args.only:
        names = [name for name in names if name == args.only]
        if not names:
            print(f"unknown smoke: {args.only}", file=sys.stderr)
            return 2

    failures: list[tuple[str, int]] = []
    for name in names:
        result = subprocess.run(
            [str(args.executable), name],
            cwd=args.executable.parent,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        if result.returncode != 0:
            print(f"[FAIL] {name}: exit {result.returncode}")
            if result.stdout:
                print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
            failures.append((name, result.returncode))

    if failures:
        print(f"{len(failures)} native smoke(s) failed out of {len(names)}.")
        return 1

    print(f"{len(names)} native smoke(s) passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
