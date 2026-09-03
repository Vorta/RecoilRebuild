#!/usr/bin/env python3
"""Run the complete production-source policy once, sequentially and fail closed."""

from __future__ import annotations

import argparse
import subprocess
import sys

from _recoil.lib.tooling import REPO_ROOT, configure_stdio


POLICY_COMMANDS: tuple[tuple[str, ...], ...] = (
    ("guard", "raw-image", "--root", "src", "--allowlist", ".agent/RAW_ADDRESS_ALLOWLIST.txt"),
    ("guard", "raw-assembly", "--root", "src", "--allowlist", ".agent/RAW_ASSEMBLY_ALLOWLIST.txt"),
    ("guard", "modern-cpp", "--root", "src", "--summary"),
    ("guard", "source-goto", "--root", "src", "--summary"),
    ("guard", "source-shape", "--root", "src", "--summary"),
    ("guard", "raw-offset", "--root", "src", "--summary"),
    ("guard", "provider", "--root", "src", "--summary"),
    ("guard", "original-symbol", "--root", "src", "--max", "50"),
    ("guard", "source-fragments", "--root", "src"),
    ("guard", "source-placement", "--root", "src"),
    ("guard", "source-data"),
    ("guard", "vc5-manifest"),
    ("audit", "provenance", "--strict"),
)


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(
        description="Run every mandatory production-source policy exactly once."
    )
    parser.parse_args(argv)
    for arguments in POLICY_COMMANDS:
        command = [sys.executable, str(REPO_ROOT / "tools/recoil.py"), *arguments]
        completed = subprocess.run(command, cwd=REPO_ROOT, check=False)
        if completed.returncode:
            print(
                "source-policy failed: " + " ".join(arguments),
                file=sys.stderr,
            )
            return completed.returncode
    print(f"[PASS] source-policy ({len(POLICY_COMMANDS)} checks)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
