"""Audit the direct reachability of the one serial reconstruction task."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from _recoil.commands.pipeline_contract_audit import audit_pipeline_contract
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.tooling import configure_stdio


STAGE_COMMANDS = {
    "authored-function-order": " progress advance-live-order ",
    "authored-call-contract": " progress advance-live-call-contract ",
    "authored-byte-match": " progress advance-live-authored-byte ",
    "full-function-order": " progress advance-live-order ",
    "linked-byte-match": " progress advance-live-linked-byte ",
    "final-validation": " verify final-image ",
}


def audit_reachability(progress: Path) -> dict[str, object]:
    contract = audit_pipeline_contract(progress)
    findings = list(contract["findings"])
    task = contract["task"]
    stage = str(task.get("stage", ""))
    if stage not in STAGE_COMMANDS:
        findings.append(f"unknown serial stage {stage!r}")
    acceptance = task.get("acceptance_command")
    if task.get("state") == "ready" and isinstance(acceptance, str):
        padded = f" {acceptance.casefold()} "
        expected = STAGE_COMMANDS.get(stage)
        if expected and expected not in padded:
            findings.append(
                f"{stage} acceptance does not route to {expected.strip()!r}"
            )
        if not padded.startswith(" python tools/recoil.py "):
            findings.append("acceptance command does not route through tools/recoil.py")
    check = task.get("check_command")
    if isinstance(check, str) and not check.startswith("python tools/recoil.py "):
        findings.append("check command does not route through tools/recoil.py")
    return {"passed": not findings, "findings": findings, "task": task}


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(description="Audit the one direct serial transition.")
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    result = audit_reachability(args.progress)
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print("pipeline reachability audit OK" if result["passed"] else "\n".join(result["findings"]))
    return 0 if result["passed"] or not args.strict else 1


if __name__ == "__main__":
    raise SystemExit(main())
