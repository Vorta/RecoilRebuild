"""Smoke tests for tools/recoil_priority.py."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def test_recoil_priority_summary_exits_zero() -> None:
    proc = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "recoil_priority.py"), "--summary"],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    assert proc.returncode == 0
    assert "P0" in proc.stdout


def test_recoil_priority_pending_impl_filter() -> None:
    proc = subprocess.run(
        [
            sys.executable,
            str(ROOT / "tools" / "recoil_priority.py"),
            "--bucket",
            "P0",
            "--pending-impl",
            "--limit",
            "3",
        ],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    assert proc.returncode == 0
