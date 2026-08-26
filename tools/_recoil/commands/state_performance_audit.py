from __future__ import annotations

import argparse
from contextlib import closing
from dataclasses import dataclass
import json
import math
import os
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile
import time
from typing import Sequence

from _recoil.commands.workspace_issues import DEFAULT_LEDGER
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.progress_sqlite import read_progress_metadata
from _recoil.lib.issue_sqlite import read_issue_metadata
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


READ_LIMIT_SECONDS = 0.250
SCHEDULER_LIMIT_SECONDS = 2.0
MUTATION_LIMIT_SECONDS = 2.0
DEFAULT_SAMPLES = 5


@dataclass(frozen=True)
class Check:
    name: str
    limit_seconds: float
    durations: tuple[float, ...]
    returncodes: tuple[int, ...]
    stderr: tuple[str, ...]

    @property
    def p95_seconds(self) -> float:
        return percentile(self.durations, 0.95)

    @property
    def passed(self) -> bool:
        return bool(self.durations) and all(code == 0 for code in self.returncodes) and self.p95_seconds < self.limit_seconds

    def to_dict(self) -> dict[str, object]:
        return {
            "name": self.name,
            "samples": len(self.durations),
            "durations_seconds": [round(value, 6) for value in self.durations],
            "p95_seconds": round(self.p95_seconds, 6),
            "limit_seconds": self.limit_seconds,
            "passed": self.passed,
            "returncodes": list(self.returncodes),
            "stderr": [value for value in self.stderr if value],
        }


def percentile(values: Sequence[float], fraction: float) -> float:
    if not values:
        return math.inf
    ordered = sorted(float(value) for value in values)
    rank = max(1, math.ceil(fraction * len(ordered)))
    return ordered[rank - 1]


def _environment() -> dict[str, str]:
    environment = os.environ.copy()
    existing = environment.get("PYTHONPATH", "")
    environment["PYTHONPATH"] = os.pathsep.join(
        item for item in (str(REPO_ROOT / "tools"), existing) if item
    )
    return environment


def _measure(command: Sequence[str], *, samples: int, warm_up: bool = False) -> Check:
    if warm_up:
        try:
            subprocess.run(
                command,
                cwd=REPO_ROOT,
                env=_environment(),
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=30.0,
                check=False,
            )
        except subprocess.TimeoutExpired:
            pass
    durations: list[float] = []
    returncodes: list[int] = []
    errors: list[str] = []
    for _ in range(samples):
        started = time.perf_counter()
        try:
            completed = subprocess.run(
                command,
                cwd=REPO_ROOT,
                env=_environment(),
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
                timeout=30.0,
                check=False,
            )
            returncodes.append(completed.returncode)
            errors.append(completed.stderr.strip())
        except subprocess.TimeoutExpired:
            returncodes.append(124)
            errors.append("fresh-process sample exceeded 30 seconds")
        durations.append(time.perf_counter() - started)
    return Check("", 0.0, tuple(durations), tuple(returncodes), tuple(errors))


def _named_check(
    name: str,
    limit: float,
    command: Sequence[str],
    *,
    samples: int,
    warm_up: bool = False,
) -> Check:
    result = _measure(command, samples=samples, warm_up=warm_up)
    return Check(name, limit, result.durations, result.returncodes, result.stderr)


def _python(code: str, *arguments: Path) -> list[str]:
    return [sys.executable, "-c", code, *(str(argument) for argument in arguments)]


def _backup(source: Path, target: Path) -> None:
    source_uri = source.resolve().as_uri() + "?mode=ro"
    with closing(sqlite3.connect(source_uri, uri=True)) as source_connection:
        with closing(sqlite3.connect(target)) as target_connection:
            source_connection.backup(target_connection)


def run_audit(progress: Path, issues: Path, *, samples: int) -> dict[str, object]:
    # Fail closed before spawning timing processes, and verify the canonical
    # pair relationship rather than timing unrelated databases.
    progress_metadata = read_progress_metadata(progress)
    issue_metadata = read_issue_metadata(issues)
    if progress_metadata.cutover_pair_id != issue_metadata.cutover_pair_id:
        raise ValueError("progress and issue SQLite cutover_pair_id values do not match")

    progress_code = (
        "from _recoil.lib.progress_sqlite import read_progress_metadata;"
        "import sys; print(read_progress_metadata(sys.argv[1]).revision)"
    )
    issue_code = (
        "from _recoil.lib.issue_sqlite import read_issue_metadata;"
        "import sys; print(read_issue_metadata(sys.argv[1]).revision)"
    )
    address_code = (
        "import json,pathlib,sqlite3,sys;"
        "c=sqlite3.connect(pathlib.Path(sys.argv[1]).resolve().as_uri()+'?mode=ro',uri=True);"
        "r=c.execute('SELECT e.payload FROM address_index a JOIN entities e ON "
        "e.collection=a.collection AND e.entity_id=a.entity_id ORDER BY a.address LIMIT 1').fetchone();"
        "json.loads(r[0]);c.close()"
    )
    summary_code = (
        "import pathlib,sqlite3,sys;"
        "c=sqlite3.connect(pathlib.Path(sys.argv[1]).resolve().as_uri()+'?mode=ro',uri=True);"
        "list(c.execute('SELECT collection,COUNT(*) FROM entities GROUP BY collection'));c.close()"
    )
    checks = [
        _named_check("progress-revision", READ_LIMIT_SECONDS, _python(progress_code, progress), samples=samples),
        _named_check("issue-revision", READ_LIMIT_SECONDS, _python(issue_code, issues), samples=samples),
        _named_check("indexed-address-read", READ_LIMIT_SECONDS, _python(address_code, progress), samples=samples),
        _named_check("indexed-summary-read", READ_LIMIT_SECONDS, _python(summary_code, progress), samples=samples),
        _named_check(
            "progress-next-cached",
            SCHEDULER_LIMIT_SECONDS,
            [sys.executable, "-m", "_recoil.commands.progress_cli", "next", "--progress", str(progress), "--json"],
            samples=samples,
            warm_up=True,
        ),
        _named_check(
            "progress-status-cached",
            SCHEDULER_LIMIT_SECONDS,
            [sys.executable, "-m", "_recoil.commands.progress_cli", "status", "--progress", str(progress), "--json"],
            samples=samples,
            warm_up=True,
        ),
    ]

    build_root = REPO_ROOT / "build"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="state-performance-", dir=build_root) as temporary:
        scratch_root = Path(temporary)
        progress_scratch = scratch_root / "progress.sqlite3"
        issues_scratch = scratch_root / "issues.sqlite3"
        _backup(progress, progress_scratch)
        _backup(issues, issues_scratch)
        progress_dry_code = (
            "from _recoil.lib.progress_sqlite import ProgressSQLiteStore;import json,sqlite3,sys;"
            "s=ProgressSQLiteStore(sys.argv[1]);r=s.read_revision();"
            "c=sqlite3.connect(sys.argv[1]);row=c.execute('SELECT collection,entity_id,payload FROM entities ORDER BY collection,entity_id LIMIT 1').fetchone();c.close();"
            "s.persist_changes(upserts={row[0]:{row[1]:json.loads(row[2])}},expected_revision=r,apply=False)"
        )
        progress_apply_code = (
            "from _recoil.lib.progress_sqlite import ProgressSQLiteStore;import json,sqlite3,sys;"
            "s=ProgressSQLiteStore(sys.argv[1]);r=s.read_revision();"
            "c=sqlite3.connect(sys.argv[1]);row=c.execute('SELECT collection,entity_id,payload FROM entities ORDER BY collection,entity_id LIMIT 1').fetchone();c.close();"
            "s.persist_changes(upserts={row[0]:{row[1]:json.loads(row[2])}},expected_revision=r,apply=True)"
        )
        issue_apply_code = (
            "from _recoil.lib.issue_sqlite import IssueSQLiteStore,read_issue_metadata;import sys;"
            "s=IssueSQLiteStore(sys.argv[1]);d=s.load();r=read_issue_metadata(sys.argv[1]).revision;"
            "s.commit(d,expected_revision=r,apply=True)"
        )
        checks.extend(
            [
                _named_check("progress-narrow-dry-run", MUTATION_LIMIT_SECONDS, _python(progress_dry_code, progress_scratch), samples=samples),
                _named_check("progress-narrow-apply", MUTATION_LIMIT_SECONDS, _python(progress_apply_code, progress_scratch), samples=samples),
                _named_check("issue-narrow-apply", MUTATION_LIMIT_SECONDS, _python(issue_apply_code, issues_scratch), samples=samples),
            ]
        )

    failures = [check.name for check in checks if not check.passed]
    return {
        "report_version": 1,
        "kind": "state-performance",
        "progress": display_path(progress),
        "issues": display_path(issues),
        "progress_revision": progress_metadata.revision,
        "issue_revision": issue_metadata.revision,
        "cutover_pair_id": progress_metadata.cutover_pair_id,
        "sample_count": samples,
        "checks": [check.to_dict() for check in checks],
        "failures": failures,
        "passed": not failures,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Audit SQLite state-store performance.")
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    parser.add_argument("--issues", type=Path, default=DEFAULT_LEDGER)
    parser.add_argument("--samples", type=int, default=DEFAULT_SAMPLES)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    if args.samples < DEFAULT_SAMPLES:
        print(f"state performance error: --samples must be at least {DEFAULT_SAMPLES}", file=sys.stderr)
        return 2
    try:
        report = run_audit(args.progress, args.issues, samples=args.samples)
    except (OSError, sqlite3.Error, ValueError, RuntimeError) as exc:
        print(f"state performance error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        for check in report["checks"]:
            state = "PASS" if check["passed"] else "FAIL"
            print(f"{state} {check['name']}: p95={check['p95_seconds']:.6f}s limit={check['limit_seconds']:.3f}s")
        print(f"passed={str(report['passed']).lower()}")
    return 1 if args.strict and not report["passed"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
