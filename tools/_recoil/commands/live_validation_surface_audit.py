from __future__ import annotations

import argparse
import ast
from dataclasses import asdict, dataclass
import json
from pathlib import Path
import re
import sys
from typing import Iterable

from _recoil.lib.call_contract_generations import (
    CALL_CONTRACT_VERIFIER_COMPONENT_PATHS,
    required_call_contract_verifier_component_findings,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


TEXT_SUFFIXES = {".py", ".json", ".toml", ".md", ".txt", ".cmake", ".ps1", ".yml", ".yaml"}
ROOTS = (
    REPO_ROOT / "tools",
    REPO_ROOT / ".codex",
    REPO_ROOT / ".claude",
    REPO_ROOT / "docs" / "reconstruction",
    REPO_ROOT / "cmake",
    REPO_ROOT / "AGENTS.md",
    REPO_ROOT / "CLAUDE.md",
    REPO_ROOT / "README.md",
)
STRUCTURED_SQLITE_TEST_PATHS = (
    REPO_ROOT / "tests" / "tools" / "recoil_explicit_maintenance_work_tests.py",
    REPO_ROOT / "tests" / "tools" / "recoil_progress_revision_domain_tests.py",
)
CALL_CONTRACT_AUTHORITY_PATH = (
    REPO_ROOT / "tools" / "_recoil" / "commands" / "call_contract_verify.py"
)
REPOSITORY_PATH_AUTHORITY = "tools/_recoil/lib/repository_paths.py"
MACHINE_LOCAL_AUTHORITY_DEFAULTS = (
    (
        "tools/_recoil/lib/progress.py",
        "DEFAULT_PROGRESS_PATH",
        ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
        "logical-execution-root",
    ),
    (
        "tools/_recoil/commands/progress_cli.py",
        "DEFAULT_PROGRESS",
        ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
        "routed-live-authority",
    ),
    (
        "tools/_recoil/commands/progress_cli.py",
        "DEFAULT_ISSUE_LEDGER",
        ".agent/WORKSPACE_ISSUES.sqlite3",
        "routed-live-authority",
    ),
    (
        "tools/_recoil/commands/workspace_issues.py",
        "default_ledger",
        ".agent/WORKSPACE_ISSUES.sqlite3",
        "routed-live-authority",
    ),
    (
        "tools/_recoil/commands/workspace_issues.py",
        "default_progress_path",
        ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
        "routed-live-authority",
    ),
    (
        "tools/_recoil/commands/worktree_control.py",
        "default_ledger",
        ".agent/WORKSPACE_ISSUES.sqlite3",
        "routed-live-authority",
    ),
    (
        "tools/_recoil/commands/worktree_control.py",
        "default_progress_path",
        ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
        "routed-live-authority",
    ),
)
REPOSITORY_LOGICAL_CONSUMERS = frozenset(
    {
        "tools/_recoil/commands/vc5_verify.py",
        "tools/_recoil/commands/call_contract_verify.py",
        "tools/_recoil/commands/call_contract_readiness_audit.py",
        "tools/_recoil/lib/progress.py",
        "tools/_recoil/commands/progress_cli.py",
    }
)
SHARED_REPOSITORY_PATH_DEFINITIONS = frozenset(
    {
        "GitTrackedPathInventory",
        "TrackedRepositoryPath",
        "HistoricalPathResolution",
        "load_git_tracked_path_inventory",
        "validate_repository_relative_path",
        "resolve_tracked_repository_file",
        "diagnose_historical_repository_path",
        "normalize_generated_repository_path",
        "TrackedRepositoryFile",
        "_git_path_command",
        "_validate_repository_path_lexical",
    }
)
# Each entry must be a reviewed physical-only projection (for example a build
# artifact or machine-local diagnostic), never a current tracked logical path.
# Keep this list function-scoped so a new projection in the same module cannot
# inherit an unrelated allowance.
REVIEWED_PHYSICAL_TO_LOGICAL_SITES: frozenset[tuple[str, str]] = frozenset(
    {
        (
            "tools/_recoil/commands/call_contract_verify.py",
            "file_dependency_states",
        ),
        (
            "tools/_recoil/commands/call_contract_verify.py",
            "_candidate_artifact_path",
        ),
        (
            "tools/_recoil/commands/call_contract_verify.py",
            "_compile_call_contract_definition_sources",
        ),
        (
            "tools/_recoil/commands/progress_cli.py",
            "_progress_command_path",
        ),
    }
)
SELF = Path(__file__).resolve()
# Active reconstruction tooling may not compute or require cryptographic
# content summaries.  Git's internal object identifiers remain opaque state
# and are outside this source-level mechanism audit.
CURRENTNESS_IDENTITY_PATHS = frozenset()
ACTIVE_MECHANISM_RE = re.compile(
    r"(?ix)"
    r"(?:hashlib|\bsha(?:1|224|256|384|512)\b|\bmd5\b|\bblake\w*\b|"
    r"hexdigest\s*\(|\bdigest\s*\(|\bmerkle\b|\bfingerprint\b|"
    r"content[_ -]hash|tree[_ -]hash|content-address(?:ed)?)"
)
RETIRED_ARCHITECTURE_RE = re.compile(
    r"(?ix)(?:"
    r"content[_-]root|transcript[_-]bytes[_-]hex|"
    r"required[_-](?:tu[_-]envelope|body[_-]leaf)[_-]keys|"
    r"tu[_-]envelopes|body[_-]outcomes|"
    r"call_contract_verification_result_policy|"
    r"scheduler_output_cache_(?:namespace|generation)|"
    r"(?:source_header|dependency)[_-]signatures|"
    r"(?:object|cod|preprocessed|population)[_-]bytes[_-]hex"
    r")"
)
DOC_ROUTE_RE = re.compile(
    r"(?ix)(?:--expected-sha256|--receipt-out|--order-receipt-dir|--reuse-compile|"
    r"progress\s+evidence\s+import|verify\s+final-determinism|final-clock-qualification|"
    r"audit\s+final-repro|whole-file\s+sha-?256)"
)
UNSAFE_CODE_RE = re.compile(
    r"(?ix)(?:"
    r"candidate_expected_truth[\"']?\s*[:=]\s*(?:true|1)\b|"
    r"worker_acceptance_allowed[\"']?\s*[:=]\s*(?:true|1)\b|"
    r"lease_stales_semantic_evidence[\"']?\s*[:=]\s*(?:true|1)\b|"
    r"phase_closeout_(?:required|no_reuse|global_clean)[\"']?\s*[:=]\s*(?:false|0)\b|"
    r"(?:accept|qualify|promote|advance|import)[_-](?:saved[_-])?"
    r"(?:candidate|receipt|snapshot|artifact|object[_-]hash)"
    r")"
)
UNSAFE_DOC_RE = re.compile(
    r"(?ix)(?:"
    r"worker\s+(?:receipt|artifact|object|snapshot).{0,80}\baccept|"
    r"(?:object|candidate|artifact)\s+hash.{0,80}(?:expected\s+truth|qualif)|"
    r"lease.{0,80}stale.{0,40}semantic\s+evidence|"
    r"(?:skip|remove|optional).{0,80}(?:global|full).{0,40}(?:closeout|convergence)"
    r")"
)
RELATIVE_LIVE_LEDGER_RE = re.compile(
    r"--(?:progress|ledger|issue-ledger)\s+\.agent[/\\]"
    r"(?:RECONSTRUCTION_PROGRESS|WORKSPACE_ISSUES)\.sqlite3\b",
    re.IGNORECASE,
)
STALE_LIVE_REVISION_RE = re.compile(
    r"(?:progress\s+work\s+claim-current|"
    r"progress\s+advance-live-call-contract)[^\r\n]*--expected-revision\b",
    re.IGNORECASE,
)
UNBOUND_CALL_CONTRACT_ACCEPTANCE_RE = re.compile(
    r"progress\s+advance-live-call-contract(?![^\r\n]*--packet-id)"
    r"[^\r\n]*--apply\b",
    re.IGNORECASE,
)


@dataclass(frozen=True)
class Finding:
    path: str
    line: int
    token: str
    text: str


def _relative_path(path: Path) -> str | None:
    try:
        return path.resolve().relative_to(REPO_ROOT.resolve()).as_posix()
    except ValueError:
        return None


def _node_text(node: ast.AST) -> str:
    values: list[str] = []
    for child in ast.walk(node):
        if isinstance(child, ast.Name):
            values.append(child.id)
        elif isinstance(child, ast.Attribute):
            values.append(child.attr)
        elif isinstance(child, ast.Constant) and isinstance(child.value, str):
            values.append(child.value)
    return " ".join(values).casefold()


def _assignment_names(node: ast.AST) -> tuple[str, ...]:
    if isinstance(node, ast.Name):
        return (node.id.casefold(),)
    if isinstance(node, ast.Attribute):
        return (node.attr.casefold(),)
    if isinstance(node, (ast.Tuple, ast.List)):
        return tuple(
            name
            for item in node.elts
            for name in _assignment_names(item)
        )
    return ()


def _contains_candidate_artifact_digest(value_text: str) -> bool:
    return bool(
        re.search(
            r"\b(?:candidate|object|artifact|receipt)[_-](?:sha(?:256)?|hash(?:es)?)\b",
            value_text,
        )
    )


def _unsafe_python_findings(path: Path, source: str) -> list[Finding]:
    """Find executable attempts to turn candidate artifacts into expected truth."""

    try:
        tree = ast.parse(source)
    except SyntaxError:
        # Syntax errors belong to the normal Python/unit-test checks.  Retain the
        # line scanner below rather than disguising one as a validation finding.
        return []
    lines = source.splitlines()
    findings: list[Finding] = []
    display = display_path(path, REPO_ROOT)
    for node in ast.walk(tree):
        if isinstance(node, ast.Dict):
            pairs = {
                key.value.casefold(): value
                for key, value in zip(node.keys, node.values)
                if isinstance(key, ast.Constant)
                and isinstance(key.value, str)
                and value is not None
            }
            forbidden_booleans = {
                "candidate_expected_truth": True,
                "worker_acceptance_allowed": True,
                "lease_stales_semantic_evidence": True,
                "phase_closeout_required": False,
                "phase_closeout_no_reuse": False,
                "phase_closeout_global_clean": False,
            }
            for key, forbidden in forbidden_booleans.items():
                value = pairs.get(key)
                if isinstance(value, ast.Constant) and value.value is forbidden:
                    line = getattr(value, "lineno", getattr(node, "lineno", 1))
                    findings.append(
                        Finding(
                            path=display,
                            line=line,
                            token=key,
                            text=lines[line - 1].strip()[:240] if lines else "",
                        )
                    )
            for key, value in pairs.items():
                if not (
                    key.startswith("expected_")
                    or key in {"expected_truth", "retail_truth"}
                ):
                    continue
                value_text = _node_text(value)
                if _contains_candidate_artifact_digest(value_text):
                    line = getattr(value, "lineno", getattr(node, "lineno", 1))
                    findings.append(
                        Finding(
                            path=display,
                            line=line,
                            token="candidate-artifact-expected-truth",
                            text=lines[line - 1].strip()[:240] if lines else "",
                        )
                    )
        elif isinstance(node, (ast.Assign, ast.AnnAssign)):
            targets = (
                tuple(node.targets)
                if isinstance(node, ast.Assign)
                else (node.target,)
            )
            names = tuple(
                name for target in targets for name in _assignment_names(target)
            )
            value = node.value
            if value is None or not any(
                name.startswith("expected_") or name == "retail_truth"
                for name in names
            ):
                continue
            value_text = _node_text(value)
            if _contains_candidate_artifact_digest(value_text):
                line = getattr(node, "lineno", 1)
                findings.append(
                    Finding(
                        path=display,
                        line=line,
                        token="candidate-artifact-expected-truth",
                        text=lines[line - 1].strip()[:240] if lines else "",
                    )
                )
    return findings


def _files(paths: Iterable[Path]) -> Iterable[Path]:
    for path in paths:
        if path.is_file():
            yield path
            continue
        if not path.is_dir():
            continue
        for candidate in path.rglob("*"):
            if not candidate.is_file() or candidate.suffix.casefold() not in TEXT_SUFFIXES:
                continue
            if any(part in {"__pycache__", ".git"} for part in candidate.parts):
                continue
            yield candidate


def audit_paths(paths: Iterable[Path]) -> list[Finding]:
    findings: list[Finding] = []
    for path in sorted(set(_files(paths)), key=lambda item: item.as_posix().casefold()):
        resolved = path.resolve()
        if resolved == SELF:
            continue
        try:
            source = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        lines = source.splitlines()
        relative = _relative_path(path)
        approved_currentness = relative in CURRENTNESS_IDENTITY_PATHS
        if path.suffix.casefold() == ".py":
            findings.extend(_unsafe_python_findings(path, source))
        for line_number, line in enumerate(lines, start=1):
            # Game/runtime algorithms named hash table or hash value are not
            # workspace validation mechanisms.
            if "gameplay_hash" in line.casefold() or "game_hash" in line.casefold():
                continue
            if path.suffix.casefold() == ".md":
                match = (
                    RELATIVE_LIVE_LEDGER_RE.search(line)
                    or STALE_LIVE_REVISION_RE.search(line)
                    or UNBOUND_CALL_CONTRACT_ACCEPTANCE_RE.search(line)
                    or DOC_ROUTE_RE.search(line)
                )
                policy_context = " ".join(lines[max(0, line_number - 2):line_number])
                if match is None and not re.search(
                    r"(?i)\b(?:never|cannot|must\s+not|does\s+not|do\s+not|no)\b",
                    policy_context,
                ):
                    match = UNSAFE_DOC_RE.search(line)
            else:
                match = UNSAFE_CODE_RE.search(line)
                if match is None and not approved_currentness:
                    match = ACTIVE_MECHANISM_RE.search(line)
                if match is None:
                    match = RETIRED_ARCHITECTURE_RE.search(line)
            if match is not None:
                findings.append(
                    Finding(
                        path=display_path(path, REPO_ROOT),
                        line=line_number,
                        token=match.group(0),
                        text=line.strip()[:240],
                    )
                )
    return findings


def _targeted_direct_evidence_findings() -> list[Finding]:
    """Guard exact authority/test surfaces that generic token scans omit."""

    findings: list[Finding] = []
    for path in STRUCTURED_SQLITE_TEST_PATHS:
        try:
            source = path.read_text(encoding="utf-8")
            tree = ast.parse(source)
        except (OSError, UnicodeDecodeError, SyntaxError):
            continue
        lines = source.splitlines()
        for node in ast.walk(tree):
            token = ""
            if (
                isinstance(node, ast.Call)
                and isinstance(node.func, ast.Attribute)
                and node.func.attr == "read_bytes"
                and not node.args
                and not node.keywords
            ):
                token = "whole-sqlite-read_bytes"
            elif isinstance(node, ast.Call):
                call_text = _node_text(node)
                database_context = any(
                    word in call_text
                    for word in ("progress", "issues", "database", "sqlite")
                )
                function_name = (
                    node.func.attr.casefold()
                    if isinstance(node.func, ast.Attribute)
                    else node.func.id.casefold()
                    if isinstance(node.func, ast.Name)
                    else ""
                )
                if database_context and function_name in {
                    "cmp", "copy", "copy2", "copyfile", "copyfileobj",
                }:
                    token = "whole-sqlite-file-equivalence"
                elif (
                    database_context
                    and function_name == "open"
                    and any(
                        isinstance(argument, ast.Constant)
                        and isinstance(argument.value, str)
                        and "b" in argument.value.casefold()
                        for argument in node.args[1:]
                    )
                ):
                    token = "whole-sqlite-binary-open"
            if token:
                line = getattr(node, "lineno", 1)
                findings.append(Finding(
                    path=display_path(path, REPO_ROOT),
                    line=line,
                    token=token,
                    text=lines[line - 1].strip()[:240],
                ))
    try:
        source = CALL_CONTRACT_AUTHORITY_PATH.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        source = ""
    for line_number, line in enumerate(source.splitlines(), start=1):
        if "vc5_verify_bn_cache" in line:
            findings.append(Finding(
                path=display_path(CALL_CONTRACT_AUTHORITY_PATH, REPO_ROOT),
                line=line_number,
                token="persisted-bn-cache-authority",
                text=line.strip()[:240],
            ))
    return findings


def _call_attribute(node: ast.AST, attribute: str) -> bool:
    return any(
        isinstance(child, ast.Call)
        and isinstance(child.func, ast.Attribute)
        and child.func.attr == attribute
        for child in ast.walk(node)
    )


def _imported_shared_repository_paths(tree: ast.AST) -> bool:
    for node in ast.walk(tree):
        if isinstance(node, ast.ImportFrom) and node.module == "_recoil.lib.repository_paths":
            return True
        if isinstance(node, ast.Import):
            if any(
                alias.name == "_recoil.lib.repository_paths"
                for alias in node.names
            ):
                return True
    return False


def _enclosing_scope_names(tree: ast.AST) -> dict[int, str]:
    scopes: dict[int, str] = {}

    class Visitor(ast.NodeVisitor):
        def __init__(self) -> None:
            self.stack: list[str] = []

        def _visit_scope(self, node: ast.AST, name: str) -> None:
            self.stack.append(name)
            scopes[id(node)] = ".".join(self.stack)
            self.generic_visit(node)
            self.stack.pop()

        def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
            self._visit_scope(node, node.name)

        def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:
            self._visit_scope(node, node.name)

        def visit_ClassDef(self, node: ast.ClassDef) -> None:
            self._visit_scope(node, node.name)

        def generic_visit(self, node: ast.AST) -> None:
            scopes.setdefault(id(node), ".".join(self.stack) or "<module>")
            super().generic_visit(node)

    Visitor().visit(tree)
    return scopes


def _registered_repository_path_authority_findings(
    *,
    repository_root: Path = REPO_ROOT,
    component_paths: Iterable[str] = CALL_CONTRACT_VERIFIER_COMPONENT_PATHS,
) -> list[Finding]:
    """Reject independent physical-to-logical case authority in verifier code.

    ``Path.resolve()`` remains valid for physical containment, machine-local
    authorities, and build artifacts.  This guard targets only independent
    shared-API definitions and the reviewed anti-pattern that projects a
    resolved filesystem spelling back into a public repository-relative
    identity.
    """

    findings: list[Finding] = []
    for relative in sorted(set(component_paths), key=str.casefold):
        path = repository_root / Path(*relative.split("/"))
        try:
            source = path.read_text(encoding="utf-8")
            tree = ast.parse(source)
        except (OSError, UnicodeDecodeError, SyntaxError):
            continue
        lines = source.splitlines()
        display = display_path(path, repository_root)
        scopes = _enclosing_scope_names(tree)

        if (
            relative in REPOSITORY_LOGICAL_CONSUMERS
            and not _imported_shared_repository_paths(tree)
        ):
            findings.append(
                Finding(
                    path=display,
                    line=1,
                    token="missing-shared-repository-path-authority",
                    text=(
                        "repository-logical consumer must import "
                        "_recoil.lib.repository_paths"
                    ),
                )
            )

        for node in ast.walk(tree):
            if (
                relative != REPOSITORY_PATH_AUTHORITY
                and isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef))
                and node.name in SHARED_REPOSITORY_PATH_DEFINITIONS
            ):
                line = getattr(node, "lineno", 1)
                findings.append(
                    Finding(
                        path=display,
                        line=line,
                        token="independent-repository-path-authority",
                        text=lines[line - 1].strip()[:240] if lines else "",
                    )
                )

        resolved_names_by_scope: dict[str, set[str]] = {}
        for node in ast.walk(tree):
            if not isinstance(node, (ast.Assign, ast.AnnAssign)):
                continue
            value = node.value
            if value is None or not _call_attribute(value, "resolve"):
                continue
            scope = scopes.get(id(node), "<module>")
            targets = node.targets if isinstance(node, ast.Assign) else (node.target,)
            for target in targets:
                if isinstance(target, ast.Name):
                    resolved_names_by_scope.setdefault(scope, set()).add(target.id)

        for node in ast.walk(tree):
            if not (
                isinstance(node, ast.Call)
                and isinstance(node.func, ast.Attribute)
                and node.func.attr in {"as_posix", "replace"}
                and _call_attribute(node.func.value, "relative_to")
            ):
                continue
            scope = scopes.get(id(node), "<module>")
            physical_projection = (
                _call_attribute(node.func.value, "resolve")
                or any(
                    isinstance(child, ast.Name)
                    and child.id in resolved_names_by_scope.get(scope, set())
                    for child in ast.walk(node.func.value)
                )
            )
            if not physical_projection:
                continue
            if (relative, scope) in REVIEWED_PHYSICAL_TO_LOGICAL_SITES:
                continue
            line = getattr(node, "lineno", 1)
            findings.append(
                Finding(
                    path=display,
                    line=line,
                    token="physical-spelling-projected-to-logical-path",
                    text=(
                        f"{scope}: {lines[line - 1].strip()}"[:240]
                        if lines
                        else scope
                    ),
                )
            )
    return findings


def _machine_local_authority_routing_findings(
    *,
    repository_root: Path = REPO_ROOT,
    consumers: Iterable[tuple[str, str, str, str]] = MACHINE_LOCAL_AUTHORITY_DEFAULTS,
) -> list[Finding]:
    """Keep logical library paths local and route only live command defaults."""

    findings: list[Finding] = []
    for relative, assignment_name, authority_path, routing_mode in consumers:
        path = repository_root / Path(*relative.split("/"))
        try:
            source = path.read_text(encoding="utf-8")
            tree = ast.parse(source)
        except (OSError, UnicodeDecodeError, SyntaxError):
            continue
        lines = source.splitlines()
        string_constants: dict[str, str] = {}
        for node in ast.walk(tree):
            if not isinstance(node, (ast.Assign, ast.AnnAssign)):
                continue
            if not isinstance(node.value, ast.Constant) or not isinstance(
                node.value.value, str
            ):
                continue
            targets = node.targets if isinstance(node, ast.Assign) else (node.target,)
            for target in targets:
                for name in _assignment_names(target):
                    string_constants[name] = node.value.value
        assignments = [
            node
            for node in ast.walk(tree)
            if isinstance(node, (ast.Assign, ast.AnnAssign))
            and assignment_name.casefold()
            in {
                name
                for target in (
                    node.targets if isinstance(node, ast.Assign) else (node.target,)
                )
                for name in _assignment_names(target)
            }
        ]
        valid = False
        for node in assignments:
            text = _node_text(node)
            referenced_names = {
                child.id.casefold()
                for child in ast.walk(node)
                if isinstance(child, ast.Name)
            }
            authority_binding_present = (
                authority_path.casefold() in text
                or any(
                    name in referenced_names
                    and value.casefold() == authority_path.casefold()
                    for name, value in string_constants.items()
                )
            )
            path_tokens_present = all(
                token.casefold() in text for token in Path(authority_path).parts
            )
            if routing_mode == "routed-live-authority":
                valid = (
                    "routed_machine_local_path" in text
                    and authority_binding_present
                )
            elif routing_mode == "logical-execution-root":
                valid = (
                    "repo_root" in text
                    and "routed_machine_local_path" not in text
                    and path_tokens_present
                )
            else:
                raise ValueError(f"unknown authority routing mode {routing_mode!r}")
            if valid:
                break
        if valid:
            continue
        line = getattr(assignments[0], "lineno", 1) if assignments else 1
        findings.append(
            Finding(
                path=display_path(path, repository_root),
                line=line,
                token=(
                    "canonicalized-library-logical-default"
                    if routing_mode == "logical-execution-root"
                    else "direct-live-authority-default"
                ),
                text=(
                    lines[line - 1].strip()[:240]
                    if lines and line <= len(lines)
                    else f"missing routed assignment for {assignment_name}"
                ),
            )
        )
    return findings


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Reject operational cryptographic content summaries and candidate-derived expected truth."
        )
    )
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--path", action="append", type=Path, default=[])
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    findings = audit_paths(args.path or ROOTS)
    findings.extend(_targeted_direct_evidence_findings())
    findings.extend(_registered_repository_path_authority_findings())
    findings.extend(_machine_local_authority_routing_findings())
    findings.extend(
        Finding(
            path=row["path"],
            line=1,
            token=f"required-verifier-component-{row['kind']}",
            text=row["detail"],
        )
        for row in required_call_contract_verifier_component_findings(REPO_ROOT)
    )
    payload = {
        "kind": "live-validation-surface-audit",
        "passed": not findings,
        "finding_count": len(findings),
        "findings": [asdict(item) for item in findings],
        "allowances": [
            {
                "path": display_path(SELF, REPO_ROOT),
                "role": "the audit's own constructed forbidden-token expressions",
            }
        ],
    }
    if args.json:
        print(json.dumps(payload, indent=2))
    elif findings:
        print(f"Live-validation surface audit: {len(findings)} unsafe mechanism occurrence(s)")
        for finding in findings[:200]:
            print(f"- {finding.path}:{finding.line}: {finding.token}: {finding.text}")
    else:
        print("Live-validation surface audit OK")
    return 1 if args.strict and findings else 0


if __name__ == "__main__":
    raise SystemExit(main())
