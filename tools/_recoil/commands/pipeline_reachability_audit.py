"""Audit executable task routes and operations needed to finish Recoil."""
from __future__ import annotations

import argparse
import ast
from contextlib import redirect_stderr
from dataclasses import asdict
import importlib
import io
import inspect
import json
from pathlib import Path
import shlex
from typing import Any, Mapping, Sequence

from _recoil.lib.pipeline_obligations import completion_obligations
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, PIPELINE_PHASES, ProgressDocument
from _recoil.lib.tooling import configure_stdio

STAGE_ROUTES = {
    "authored-function-order": {"progress advance-live-order"},
    "authored-call-contract": {"progress advance-live-call-contract", "progress call-contract close-live"},
    "authored-byte-match": {"progress advance-live-authored-byte"},
    "full-function-order": {"progress advance-live-order"},
    "linked-byte-match": {"progress advance-live-linked-byte"},
    "final-validation": {"verify final-image"},
}


def _parse_route(item: Any, rest: list[str]) -> argparse.Namespace:
    backend = importlib.import_module(item.module_name)
    if not callable(getattr(backend, "main", None)):
        raise ValueError(f"{item.module_name} has no callable main")
    factory = getattr(backend, "build_parser", None) or getattr(backend, "_parser", None)
    if not callable(factory):
        raise ValueError(f"{item.module_name} exposes no inspectable parser")
    diagnostics = io.StringIO()
    with redirect_stderr(diagnostics):
        try:
            return factory().parse_args([*item.prepend_args, *rest])
        except SystemExit as exc:
            raise ValueError(f"backend parser rejected route: {diagnostics.getvalue().strip()}") from exc


def _probe_arguments(item: Any) -> list[str]:
    """Nonexecuted arguments exercise the real parser and required guards."""
    if item.name == "verify final-image":
        return ["--json"]
    if item.name == "progress provider-target register":
        return ["--address", "0x401000", "--payload-json", "{}", "--expected-revision", "0", "--dry-run", "--json"]
    result = ["--apply", "--json"]
    if item.build_root_contract == "fresh-direct-root":
        result += ["--build-root", "build/live-validation/reachability-probe-never-executed"]
    if item.build_root_contract != "fresh-replay-sibling":
        for domain in item.required_revision_domains:
            flag = {"global": "--expected-revision", "transaction": "--expected-revision",
                    "semantic": "--expected-semantic-revision",
                    "evidence_generation": "--expected-evidence-generation-revision"}[domain]
            result += [flag, "0"]
    if item.name == "progress advance-live-order":
        result += ["--target", "recoil:verification-target:probe"]
    if item.name == "progress advance-live-call-contract":
        result += ["--slice", "recoil:call-contract-slice:0x401000-0x401010"]
    if item.name == "progress storage accept-live":
        result += ["--storage", "recoil:storage:va:0x500000", "--dimension", "extent"]
    if item.name in {"progress owner accept-live", "progress owner promote-live"}:
        result += ["--owner", "recoil:owner:probe", "--payload-file", "build/diagnostics/reachability-never-read.json"]
        result += ["--gate", "boundary"] if item.name.endswith("accept-live") else ["--tier", "C"]
    return result


# These are executable backend functions, not additional command aliases. The
# retained proof-kernel tests exercise their writes; this audit also checks that
# the public handler still calls the evidence, freshness and commit boundaries.
HANDLER_CONTRACTS = {
    "progress storage accept-live": ("accept_storage", {"_absolute_fresh_build_root", "storage_scope", "record_storage", "add_live_evidence"}),
    "progress owner accept-live": ("accept_owner", {"_absolute_fresh_build_root", "validate_review", "verify_owner", "record_owner", "add_live_evidence"}),
    "progress owner promote-live": ("accept_owner", {"_absolute_fresh_build_root", "validate_review", "verify_owner", "record_owner", "add_live_evidence"}),
    "progress advance-live-order": ("advance_live_order", {"_absolute_fresh_build_root", "_validate_order_result", "accept_live_order_block"}),
    "progress advance-live-authored-byte": ("advance_live_byte", {"_absolute_fresh_build_root", "_validate_byte_result", "accept_live_byte_groups"}),
    "progress advance-live-linked-byte": ("advance_live_byte", {"_absolute_fresh_build_root", "_validate_byte_result", "accept_live_byte_groups"}),
    "progress advance-live-call-contract": ("advance_live_call_contract", {"_absolute_fresh_build_root", "_precheck_call_contract_revisions", "_validate_call_contract_result", "_commit_validated_call_contract_slice"}),
    "progress call-contract replay-live": ("replay_live_call_contract", {"live_call_contract_result", "_require_call_contract_proof_inputs_unchanged", "_commit_validated_call_contract_slice"}),
    "progress call-contract close-live": ("close_live_call_contract", {"_absolute_fresh_build_root", "_run_parallel_call_contract_closeout_scan", "_run_call_contract_linkability_gate", "_require_call_contract_closeout_vector_unchanged", "_call_contract_scoped_patch_commit"}),
    "verify final-image": ("run", {"_load_coverage_from_open_retail", "_fresh_candidate", "compare_images"}),
    "progress provider-target register": ("register_provider_target", {"retail_import_target", "ProgressStore"}),
}


def _argument_value(node: ast.expr, arguments: argparse.Namespace) -> Any:
    """Read only the small argument predicates used by these CLI dispatchers."""
    if isinstance(node, ast.Constant):
        return node.value
    if isinstance(node, ast.Attribute) and isinstance(node.value, ast.Name) and node.value.id == "args":
        return getattr(arguments, node.attr)
    if isinstance(node, (ast.Set, ast.Tuple, ast.List)):
        return tuple(_argument_value(item, arguments) for item in node.elts)
    if isinstance(node, ast.BoolOp):
        if isinstance(node.op, ast.And):
            return all(_argument_value(item, arguments) for item in node.values)
        return any(_argument_value(item, arguments) for item in node.values)
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.Not):
        return not _argument_value(node.operand, arguments)
    if isinstance(node, ast.Compare) and len(node.ops) == 1:
        left, right = _argument_value(node.left, arguments), _argument_value(node.comparators[0], arguments)
        operator = node.ops[0]
        if isinstance(operator, ast.Eq): return left == right
        if isinstance(operator, ast.NotEq): return left != right
        if isinstance(operator, ast.In): return left in right
        if isinstance(operator, ast.NotIn): return left not in right
    raise ValueError(f"uninspected CLI dispatch predicate: {ast.unparse(node)}")


def _selected_dispatch_calls(statements: Sequence[ast.stmt], arguments: argparse.Namespace):
    """Inspect a selected main-path without executing any backend operation."""
    for statement in statements:
        if isinstance(statement, ast.If):
            selected = statement.body if _argument_value(statement.test, arguments) else statement.orelse
            yield from _selected_dispatch_calls(selected, arguments)
        elif isinstance(statement, (ast.Try, ast.With)):
            yield from _selected_dispatch_calls(statement.body, arguments)
        elif isinstance(statement, (ast.FunctionDef, ast.ClassDef)):
            continue
        else:
            for node in ast.walk(statement):
                if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
                    yield node.func.id
            if isinstance(statement, (ast.Return, ast.Raise)):
                # Propagate termination out of nested dispatch blocks.
                yield None
                return


def _dispatch_findings(backend: Any, item: Any, handler_name: str,
                       arguments: argparse.Namespace) -> list[str]:
    via = ({"progress call-contract replay-live": ("replay_live",),
            "progress provider-target register": ("run",)}.get(item.name, ()))
    chain = ("main", *via, handler_name)
    for caller_name, callee_name in zip(chain, chain[1:]):
        caller = getattr(backend, caller_name, None)
        callee = getattr(backend, callee_name, None)
        if not callable(caller) or not callable(callee) or caller.__globals__.get(callee_name) is not callee:
            return [f"CLI dispatch lacks its exact callable edge {caller_name} -> {callee_name}"]
        calls = _selected_dispatch_calls(ast.parse(inspect.getsource(caller)).body[0].body, arguments)
        for called in calls:
            if called == callee_name:
                break
            if called is None:
                return [f"selected CLI path returns before {caller_name} -> {callee_name}"]
        else:
            return [f"selected CLI path cannot reach {caller_name} -> {callee_name}"]
    return []


def _handler_findings(item: Any, arguments: argparse.Namespace) -> list[str]:
    contract = HANDLER_CONTRACTS.get(item.name)
    if contract is None:
        return ["acceptance effects have no inspected backend handler contract"]
    backend = importlib.import_module(item.module_name)
    handler_name, required = contract
    handler = getattr(backend, handler_name, None)
    if not callable(handler):
        return [f"backend handler {handler_name} is unavailable"]
    tree = ast.parse(inspect.getsource(handler))
    called = {node.func.id for node in ast.walk(tree)
              if isinstance(node, ast.Call) and isinstance(node.func, ast.Name)}
    findings = _dispatch_findings(backend, item, handler_name, arguments)
    findings.extend(f"backend handler {handler_name} no longer calls {name}" for name in sorted(required - called))
    for name in sorted(required & called):
        if not callable(handler.__globals__.get(name)):
            findings.append(f"backend handler dependency {name} is unavailable")
    if item.module == "scoped_acceptance":
        attributes = {ast.unparse(node.func) for node in ast.walk(tree)
                      if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)}
        for edge in ("live.build", "live.unchanged", "store.mutate"):
            if edge not in attributes:
                findings.append(f"scoped handler lacks live proof/commit edge {edge}")
        for method, dependency in (("build", "_run_fresh_build"), ("unchanged", "input_inventory"), ("input_inventory", "dependency_states")):
            implementation = getattr(backend.LiveInputs, method, None)
            if not callable(implementation):
                findings.append(f"scoped live input method {method} is unavailable")
                continue
            import textwrap
            code = ast.parse(textwrap.dedent(inspect.getsource(implementation)))
            dependencies = {node.func.id if isinstance(node.func, ast.Name) else node.func.attr
                            for node in ast.walk(code) if isinstance(node, ast.Call)
                            and isinstance(node.func, (ast.Name, ast.Attribute))}
            if dependency not in dependencies:
                findings.append(f"scoped {method} lacks required dependency {dependency}")
    return findings


def audit_completion_routes(registry: Sequence[Any] | None = None) -> dict[str, Any]:
    import recoil
    items = tuple(recoil.COMMAND_SPECS if registry is None else registry)
    findings: list[str] = []
    producers: dict[str, list[str]] = {}
    routes: list[dict[str, Any]] = []
    for item in items:
        if not item.acceptance_effects:
            continue
        problems: list[str] = []
        resolved, remaining = recoil.resolve_command(list(item.path))
        if resolved != item or remaining:
            problems.append("facade does not resolve the declared command exactly")
        try:
            arguments = _probe_arguments(item)
            parsed = _parse_route(item, arguments)
            problems.extend(_handler_findings(item, parsed))
            provider_import = item.name == "progress provider-target register"
            if not item.mutates or (not provider_import and not item.build_root_contract.startswith("fresh-")):
                problems.append("live producer lacks its mutation/fresh-build contract")
            if item.name != "verify final-image":
                if not item.required_revision_domains or item.mutation_scope == "none":
                    problems.append("acceptance producer lacks revision or mutation scope")
                if not provider_import and not recoil.requires_source_policy(item, arguments):
                    problems.append("acceptance producer bypasses the source-policy guard")
                # Replay obtains both CAS coordinates atomically from the task.
                if item.build_root_contract != "fresh-replay-sibling":
                    for flag in ("--expected-revision", "--expected-semantic-revision",
                                 "--expected-evidence-generation-revision", "--build-root"):
                        if flag in arguments:
                            index = arguments.index(flag)
                            try:
                                _parse_route(item, arguments[:index] + arguments[index + 2:])
                            except ValueError:
                                pass
                            else:
                                problems.append(f"backend does not require {flag}")
        except (ImportError, AttributeError, KeyError, OSError, ValueError) as exc:
            problems.append(str(exc))
        routes.append({"command": item.name, "operational": not problems,
                       "acceptance_effects": [asdict(effect) for effect in item.acceptance_effects],
                       "findings": problems})
        findings.extend(f"{item.name}: {problem}" for problem in problems)
        if not problems:
            for effect in item.acceptance_effects:
                producers.setdefault(effect.key, []).append(item.name)
    obligations = []
    for stage, effect, consumer in completion_obligations():
        commands = sorted(producers.get(effect.key, []))
        obligations.append({"id": effect.key, "stage": stage, **asdict(effect),
                            "consumer": consumer, "producers": commands,
                            "status": "supported" if commands else "missing-acceptance-operation"})
        if not commands:
            findings.append(f"missing acceptance operation: {effect.key} (required by {consumer})")
    return {"completion_routes_complete": not findings, "obligations": obligations,
            "routes": routes, "findings": findings}


def audit_task_routes(task: Mapping[str, Any]) -> list[str]:
    import recoil
    findings = _task_contract_findings(task)
    stage = str(task.get("stage", ""))
    if stage not in PIPELINE_PHASES or set(STAGE_ROUTES) != set(PIPELINE_PHASES):
        findings.append(f"unknown or unaccounted serial stage {stage!r}")
    for field in ("acceptance_command", "check_command", "stage_runner_command"):
        command = task.get(field)
        if command is None:
            continue
        try:
            if not isinstance(command, str):
                raise ValueError("command must be text")
            tokens = shlex.split(command)
            if tokens[:2] != ["python", "tools/recoil.py"]:
                raise ValueError("command does not use the canonical facade")
            item, rest = recoil.resolve_command(tokens[2:])
            if item is None:
                raise ValueError("command has no public route")
            parsed = _parse_route(item, rest)
            if field in {"acceptance_command", "stage_runner_command"}:
                findings.extend(f"{field}: {problem}" for problem in _handler_findings(item, parsed))
            if field == "acceptance_command":
                if item.name not in STAGE_ROUTES.get(stage, set()):
                    raise ValueError(f"{item.name} cannot accept stage {stage}")
                if not item.acceptance_effects:
                    raise ValueError("route declares no acceptance effect")
                if item.name != "verify final-image" and not getattr(parsed, "apply", False):
                    raise ValueError("acceptance route does not apply")
                vector = task.get("revision_vector", {})
                for attribute, coordinate in (("expected_revision", "transaction_revision"),
                        ("expected_semantic_revision", "semantic_revision"),
                        ("expected_evidence_generation_revision", "evidence_generation_revision")):
                    value = getattr(parsed, attribute, None)
                    if value is not None and value != vector.get(coordinate):
                        raise ValueError(f"{attribute} does not match the task revision")
                if getattr(parsed, "candidate", None) is not None:
                    raise ValueError("acceptance must rebuild current source")
        except (ImportError, AttributeError, OSError, TypeError, ValueError) as exc:
            findings.append(f"{field}: {exc}")
    return findings


def audit_reachability(progress: Path) -> dict[str, Any]:
    task = ProgressDocument.load(progress).current_task("recoil")
    task_findings = audit_task_routes(task)
    current_task_reachable = not task_findings and task.get("state") in {"ready", "complete"}
    if not task_findings and task.get("state") == "blocked":
        task_findings.append(f"current task is blocked: {task.get('blocker')}")
    completion = audit_completion_routes()
    findings = [*task_findings, *completion["findings"]]
    return {**completion, "passed": not findings, "findings": findings, "task": task,
            "current_task_reachable": current_task_reachable,
            "current_task_blocker": task.get("blocker"),
            "scope": "command-capability; current source and evidence still require live verification"}


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(description=__doc__)
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
    "stage_runner_command",
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


def _task_contract_findings(task: Mapping[str, Any]) -> list[str]:
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
    stage_runner = task.get("stage_runner_command")
    expected_stage_runner = (
        "python tools/recoil.py progress call-contract replay-live "
        "--apply --json"
        if task.get("stage") == "authored-call-contract"
        else None
    )
    if stage_runner != expected_stage_runner:
        findings.append(
            "stage_runner_command differs: "
            f"expected {expected_stage_runner!r}, found {stage_runner!r}"
        )
    for field in (
        "check_command",
        "stage_runner_command",
        "acceptance_command",
    ):
        value = task.get(field)
        if not isinstance(value, str):
            continue
        padded = f" {value.casefold()} "
        for token in RETIRED_COMMAND_TOKENS:
            if token in padded:
                findings.append(f"{field} exposes retired orchestration token {token.strip()!r}")
    if not isinstance(task.get("scope"), dict):
        findings.append("scope is not an advisory object")
    return findings



if __name__ == "__main__":
    raise SystemExit(main())
