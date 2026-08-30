from __future__ import annotations

import argparse
import json
from pathlib import Path

from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressDocument
from _recoil.lib.tooling import configure_stdio


EXPECTED_KEYS = {
    "schema",
    "binary",
    "stage",
    "task_id",
    "state",
    "cursor",
    "scope",
    "objective",
    "check_command",
    "acceptance_command",
    "blocker",
    "revision_vector",
}
REVISION_KEYS = {
    "transaction_revision",
    "semantic_revision",
    "evidence_generation_revision",
}
RETIRED_COMMAND_TOKENS = (
    " claim-current ",
    " progress handoff ",
    " progress work ",
    " --packet-id ",
    " --lane ",
    "workspace worktree",
)


def audit_pipeline_contract(progress: Path) -> dict[str, object]:
    task = ProgressDocument.load(progress).current_task("recoil")
    findings: list[str] = []
    if set(task) != EXPECTED_KEYS:
        findings.append(
            "current task keys differ: "
            f"expected {sorted(EXPECTED_KEYS)}, found {sorted(task)}"
        )
    if task.get("schema") != "recoil-current-task-v2":
        findings.append("current task schema is not recoil-current-task-v2")
    vector = task.get("revision_vector")
    if not isinstance(vector, dict) or set(vector) != REVISION_KEYS:
        findings.append("revision_vector does not contain exactly the retained domains")
    acceptance = task.get("acceptance_command")
    if task.get("state") == "ready" and not (
        isinstance(acceptance, str) and acceptance.strip()
    ):
        findings.append("ready task lacks its one direct acceptance command")
    for field in ("check_command", "acceptance_command"):
        value = task.get(field)
        if not isinstance(value, str):
            continue
        padded = f" {value.casefold()} "
        for token in RETIRED_COMMAND_TOKENS:
            if token in padded:
                findings.append(f"{field} exposes retired orchestration token {token.strip()!r}")
    if not isinstance(task.get("scope"), dict):
        findings.append("scope is not an advisory object")
    return {"passed": not findings, "findings": findings, "task": task}


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(
        description="Audit the one serial direct-work pipeline contract."
    )
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    result = audit_pipeline_contract(args.progress)
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print("pipeline contract audit OK" if result["passed"] else "\n".join(result["findings"]))
    return 0 if result["passed"] or not args.strict else 1


if __name__ == "__main__":
    raise SystemExit(main())
