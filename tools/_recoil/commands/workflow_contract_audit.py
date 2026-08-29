from __future__ import annotations

import argparse
import ast
import contextlib
from dataclasses import dataclass
import inspect
import io
import json
import os
from pathlib import Path
import re
import shlex
import textwrap
from types import ModuleType
from typing import Any, Iterable, Mapping

from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


@dataclass(frozen=True)
class CommandContract:
    module: str
    prepend: tuple[str, ...]
    mutates: bool


REQUIRED_COMMANDS: dict[tuple[str, ...], CommandContract] = {
    ("audit", "workflow-contracts"): CommandContract(
        "workflow_contract_audit", (), False
    ),
    ("audit", "source-fragments"): CommandContract(
        "source_fragments", ("--audit",), False
    ),
    ("guard", "source-fragments"): CommandContract(
        "source_fragments", (), False
    ),
    ("docs", "readme-progress"): CommandContract("readme_progress", (), True),
    ("progress", "work", "claim-current"): CommandContract(
        "progress_cli", ("work", "claim-current"), True
    ),
    ("progress", "work", "create-explicit"): CommandContract(
        "progress_cli", ("work", "create-explicit"), True
    ),
    ("progress", "work", "return"): CommandContract(
        "progress_cli", ("work", "return"), True
    ),
    ("progress", "work", "return-binja"): CommandContract(
        "progress_cli", ("work", "return-binja"), True
    ),
    ("progress", "work", "recover-expired"): CommandContract(
        "progress_cli", ("work", "recover-expired"), True
    ),
    ("progress", "work", "recover-allocation"): CommandContract(
        "progress_cli", ("work", "recover-allocation"), True
    ),
    ("progress", "handoff"): CommandContract("progress_cli", ("handoff",), False),
    ("progress", "current-metadata", "refresh"): CommandContract(
        "current_metadata_mutation", ("refresh",), True
    ),
    ("progress", "relocation-exception", "set"): CommandContract(
        "relocation_expectation_mutation", ("set",), True
    ),
    ("progress", "relocation-target", "bind"): CommandContract(
        "relocation_target_mutation", ("bind",), True
    ),
    ("verify", "vc5-order"): CommandContract("vc5_verify", ("--order-only",), False),
    ("verify", "authored-object-byte"): CommandContract(
        "live_byte_verify", ("object",), False
    ),
    ("verify", "authored-byte"): CommandContract(
        "live_byte_verify", ("authored",), False
    ),
    ("verify", "linked-byte"): CommandContract(
        "live_byte_verify", ("linked",), False
    ),
    ("verify", "call-contract"): CommandContract(
        "call_contract_verify", (), False
    ),
    ("progress", "advance-live-order"): CommandContract(
        "progress_cli", ("advance-live-order",), True
    ),
    ("progress", "advance-live-byte"): CommandContract(
        "progress_cli", ("advance-live-byte",), True
    ),
    ("progress", "advance-live-call-contract"): CommandContract(
        "progress_cli", ("advance-live-call-contract",), True
    ),
    ("progress", "call-contract", "initialize"): CommandContract(
        "progress_cli", ("call-contract", "initialize"), True
    ),
    ("progress", "call-contract", "prepare-live-convergence"): CommandContract(
        "progress_cli", ("call-contract", "prepare-live-convergence"), True
    ),
    ("progress", "call-contract", "prepare-repair-continuation"): CommandContract(
        "progress_cli", ("call-contract", "prepare-repair-continuation"), True
    ),
    ("workspace", "worktree", "status"): CommandContract(
        "worktree_control", ("status",), False
    ),
    ("workspace", "worktree", "create"): CommandContract(
        "worktree_control", ("create",), True
    ),
    ("workspace", "worktree", "validate"): CommandContract(
        "worktree_control", ("validate",), False
    ),
    ("workspace", "worktree", "integrate"): CommandContract(
        "worktree_control", ("integrate",), True
    ),
    ("workspace", "worktree", "retire"): CommandContract(
        "worktree_control", ("retire",), True
    ),
    ("workspace", "worktree", "hygiene"): CommandContract(
        "worktree_control", ("hygiene",), False
    ),
}


EXPECTED_CALL_CONTRACT_ACCEPTANCE_POLICY = {
    "contract_version": 1,
    "acceptance_authority": "fresh-parent-direct-retail",
    "identity_role": "explicit-invalidation-and-generations",
    "slice_role": "cursor-window",
    "worker_acceptance_allowed": False,
    "partial_body_acceptance": True,
    "phase_closeout_required": True,
    "phase_closeout_no_reuse": True,
    "phase_closeout_global_clean": True,
    "lease_stales_semantic_evidence": False,
    "obligation_conflict_authority": "resource-claims",
    "obligation_packet_types": {
        "source": "call-contract-source-obligation-v1",
        "profile": "call-contract-profile-obligation-v1",
        "verifier": "call-contract-verifier-obligation-v1",
        "linker": "call-contract-linker-obligation-v1",
        "retail": "call-contract-retail-fact-read-v2",
    },
}

def _default_specs() -> Iterable[Any]:
    # tools/recoil.py is the public vocabulary owner. Importing it does not dispatch a
    # command, and keeps this audit independent of copied command-name prose.
    import recoil

    return recoil.COMMAND_SPECS


def registry_view(specs: Iterable[Any] | None = None) -> dict[tuple[str, ...], CommandContract]:
    result: dict[tuple[str, ...], CommandContract] = {}
    for item in _default_specs() if specs is None else specs:
        if isinstance(item, Mapping):
            raw_path = item.get("path", ())
            module = item.get("module", "")
            prepend = item.get("prepend_args", item.get("prepend", ()))
            mutates = item.get("mutates", False)
        else:
            raw_path = getattr(item, "path", ())
            module = getattr(item, "module", "")
            prepend = getattr(item, "prepend_args", ())
            mutates = getattr(item, "mutates", False)
        if isinstance(raw_path, str):
            path = tuple(raw_path.split())
        else:
            path = tuple(str(part) for part in raw_path)
        result[path] = CommandContract(
            module=str(module),
            prepend=tuple(str(part) for part in prepend),
            mutates=bool(mutates),
        )
    return result


def _finding(check: str, message: str) -> dict[str, str]:
    return {"check": check, "message": message}


def audit_generated_call_contract_commands(
    document: Any,
    payload: Mapping[str, Any],
) -> list[dict[str, str]]:
    """Validate actual scheduler-rendered applying call-contract commands."""

    from _recoil.lib.progress import (
        authenticate_explicit_output_root,
        normalize_resource_claims,
        work_resource_claims,
    )

    commands: list[str] = []

    def visit(value: Any) -> None:
        if isinstance(value, Mapping):
            for item in value.values():
                visit(item)
        elif isinstance(value, list):
            for item in value:
                visit(item)
        elif isinstance(value, str) and "--apply" in value and (
            "progress advance-live-call-contract" in value
            or "progress call-contract prepare-live-convergence" in value
        ):
            commands.append(value)

    visit(payload)
    failures: list[dict[str, str]] = []
    containment = payload.get("call_contract_containment")
    if isinstance(containment, Mapping) and containment.get("launchable") is False and commands:
        failures.append(
            _finding(
                "generated-call-contract-command",
                "contained call-contract projection exposes an applying command",
            )
        )
    if isinstance(containment, Mapping) and containment.get("launchable") is True:
        projected = containment.get("next_command")
        if not isinstance(projected, str) or not projected.strip():
            failures.append(
                _finding(
                    "generated-call-contract-command",
                    "launchable call-contract projection lacks a nonempty command",
                )
            )
        else:
            try:
                tokens = shlex.split(projected, posix=False)
            except ValueError:
                tokens = []
            normalized_tokens = [token.strip('"') for token in tokens]
            if (
                len(normalized_tokens) < 6
                or normalized_tokens[0].casefold() not in {"python", "python.exe"}
                or normalized_tokens[1:3] != ["-B", "tools/recoil.py"]
                or normalized_tokens[3] != "progress"
                or "--apply" not in normalized_tokens
            ):
                failures.append(
                    _finding(
                        "generated-call-contract-command",
                        "launchable call-contract projection command is not a parseable applying public route",
                    )
                )
    for command in commands:
        packet_match = re.search(r"(?:^|\s)--packet-id\s+(\S+)", command)
        semantic = re.findall(r"--expected-semantic-revision\s+\d+", command)
        evidence = re.findall(
            r"--expected-evidence-generation-revision\s+\d+", command
        )
        if packet_match is None:
            failures.append(
                _finding(
                    "generated-call-contract-command",
                    "generated applying call-contract command lacks --packet-id",
                )
            )
            continue
        if len(semantic) != 1 or len(evidence) != 1 or "--expected-revision" in command:
            failures.append(
                _finding(
                    "generated-call-contract-command",
                    "generated applying call-contract command lacks exact semantic/evidence guards",
                )
            )
        packet_id = packet_match.group(1)
        work = document.collection("work_items").get(packet_id)
        reservation = work.get("reservation") if isinstance(work, Mapping) else None
        if (
            not isinstance(work, Mapping)
            or work.get("state") != "active"
            or not isinstance(reservation, Mapping)
            or reservation.get("state") != "active"
        ):
            failures.append(
                _finding(
                    "generated-call-contract-command",
                    f"generated applying command names non-active packet {packet_id!r}",
                )
            )
            continue
        claims, complete, _source = work_resource_claims(work)
        reservation_claims = reservation.get("resource_claims")
        try:
            normalized_reservation = normalize_resource_claims(
                row for row in reservation_claims if isinstance(row, Mapping)
            ) if isinstance(reservation_claims, list) else []
        except Exception:
            normalized_reservation = []
        required_kinds = {
            ("binary-ninja-db", "Recoil.bndb", "read"),
            ("reference", "support/Recoil.exe", "read"),
        }
        actual = {
            (str(row["kind"]), str(row["id"]), str(row["access"]))
            for row in claims
        }
        if (
            not complete
            or normalized_reservation != claims
            or not required_kinds.issubset(actual)
            or not any(row["kind"] == "output-root" and row["access"] == "write" for row in claims)
            or not any(row["kind"] == "tracker" for row in claims)
        ):
            failures.append(
                _finding(
                    "generated-call-contract-command",
                    f"generated applying command packet {packet_id!r} lacks its required resource vector",
                )
            )
            continue
        try:
            authenticate_explicit_output_root(
                work,
                progress_path=getattr(document, "path", None)
                or REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3",
            )
        except Exception as exc:
            failures.append(
                _finding(
                    "generated-call-contract-command",
                    f"generated applying command packet {packet_id!r} has an unauthenticated output root: {exc}",
                )
            )
    return failures


def _audit_registry(view: Mapping[tuple[str, ...], CommandContract]) -> list[dict[str, str]]:
    failures: list[dict[str, str]] = []
    for path, expected in REQUIRED_COMMANDS.items():
        actual = view.get(path)
        label = " ".join(path)
        if actual is None:
            failures.append(_finding("public-registry", f"missing public command: {label}"))
            continue
        if actual != expected:
            failures.append(
                _finding(
                    "public-registry",
                    f"{label} contract is {actual!r}, expected {expected!r}",
                )
            )
    return failures


class _HandoffDocument:
    def __init__(self, work_items: Mapping[str, Mapping[str, Any]]) -> None:
        self._work_items = dict(work_items)
        self.path = None

    def next_work(self, binary: str) -> dict[str, Any]:
        if binary != "recoil":
            raise AssertionError(binary)
        return {
            "phase": "authored-function-order",
            "primary_lane": "order",
            "cursor": "0x401060",
            "physical_block_id": "recoil:block:0x401060",
        }

    def pipeline(self, binary: str, *, resolve_order_target: bool = True) -> dict[str, Any]:
        del resolve_order_target
        return self.next_work(binary)

    def collection(self, name: str) -> Mapping[str, Mapping[str, Any]]:
        if name != "work_items":
            raise AssertionError(name)
        return self._work_items

    def show(self, selector: str) -> dict[str, str]:
        return {"selector": selector}

    def scheduler_output(self, value: Mapping[str, Any]) -> dict[str, Any]:
        return dict(value)


def _valid_work() -> dict[str, Any]:
    return {
        "state": "active",
        "binary": "recoil",
        "packet_type": "order-edit-v1",
        "phase": "authored-function-order",
        "lane": "primary",
        "cursor": "0x401060",
        "block_id": "recoil:block:0x401060",
        "covered_block_ids": ["recoil:block:0x401060"],
        "target_id": "recoil:vc5-target:sample",
        "validation_commands": [
            "python tools/recoil.py verify vc5-order sample --build-root build/worker/sample"
        ],
        "resource_claims": [
            {"kind": "path", "id": "src/GameZRecoil/sample.cpp", "access": "write"},
            {
                "kind": "verification-target",
                "id": "recoil:vc5-target:sample",
                "access": "read",
            },
        ],
        "reservation": {
            "id": "recoil:work:sample:attempt:1",
            "state": "active",
        },
        "objective": "Make the order target pass.",
        "stop_condition": "PASS or a concrete scope contradiction.",
        "required_return_fields": ["packet_id", "outcome"],
    }


def _handoff_args() -> argparse.Namespace:
    return argparse.Namespace(
        packet_id=None,
        authored_object_byte=False,
        authored_byte=False,
        fallback_authored_byte=False,
    )


def _expect_handoff_rejection(
    progress_module: ModuleType,
    work: Mapping[str, Any] | None,
    *,
    label: str,
) -> dict[str, str] | None:
    rows = {} if work is None else {"recoil:work:sample": work}
    try:
        progress_module._handoff(_HandoffDocument(rows), _handoff_args())
    except Exception:
        return None
    return _finding("compact-reserved-handoff", f"handoff accepted {label}")


def _audit_handoff(progress_module: ModuleType) -> list[dict[str, str]]:
    failures: list[dict[str, str]] = []
    work = _valid_work()
    try:
        result = progress_module._handoff(
            _HandoffDocument({"recoil:work:sample": work}), _handoff_args()
        )
        packet = result.get("work_item")
        if not isinstance(packet, Mapping):
            failures.append(_finding("compact-reserved-handoff", "handoff lacks compact packet"))
        else:
            if packet.get("reservation_id") != "recoil:work:sample:attempt:1":
                failures.append(
                    _finding("compact-reserved-handoff", "handoff lost the active reservation id")
                )
            if packet.get("write_paths") != ["src/GameZRecoil/sample.cpp"]:
                failures.append(
                    _finding("compact-reserved-handoff", "handoff lacks its exact writable closure")
                )
            if "resource_claims" in packet or "validation_commands" in packet:
                failures.append(
                    _finding("compact-reserved-handoff", "handoff is not compact")
                )
            if (
                any(
                    forbidden in packet
                    for forbidden in (
                        "apply",
                        "acceptance_eligible",
                        "stored_acceptance_inputs",
                        "evidence_id",
                    )
                )
                or "body_results" in packet
            ):
                failures.append(
                    _finding(
                        "worker-receipt-nonaccepting",
                        "worker handoff exposes persisted acceptance result state",
                    )
                )
    except Exception as exc:
        failures.append(_finding("compact-reserved-handoff", f"valid handoff failed: {exc}"))

    try:
        packet_result = progress_module._handoff(
            _HandoffDocument({"recoil:work:sample": work}),
            argparse.Namespace(
                packet_id="recoil:work:sample",
                authored_object_byte=False,
                authored_byte=False,
                fallback_authored_byte=False,
            ),
        )
        if packet_result.get("work_item_id") != "recoil:work:sample":
            failures.append(
                _finding("compact-reserved-handoff", "packet-id handoff selected the wrong lease")
            )
    except Exception as exc:
        failures.append(
            _finding("compact-reserved-handoff", f"packet-id handoff failed: {exc}")
        )

    bad_cases: list[tuple[str, Mapping[str, Any] | None]] = [("no reservation", None)]
    absent_lease = _valid_work()
    absent_lease["reservation"] = None
    bad_cases.append(("absent active lease", absent_lease))
    empty_writes = _valid_work()
    empty_writes["resource_claims"] = [
        {"kind": "verification-target", "id": "recoil:vc5-target:sample", "access": "read"}
    ]
    bad_cases.append(("empty writable closure", empty_writes))
    parent_command = _valid_work()
    parent_command["validation_commands"] = [
        "python tools/recoil.py progress advance-live-order --target sample --apply"
    ]
    bad_cases.append(("parent advance-live command", parent_command))
    multiple_commands = _valid_work()
    multiple_commands["validation_commands"] = [
        "python tools/recoil.py verify vc5-order sample",
        "python tools/recoil.py verify authored-byte --at 0x401060",
    ]
    bad_cases.append(("multiple worker commands", multiple_commands))
    for label, bad_work in bad_cases:
        failure = _expect_handoff_rejection(progress_module, bad_work, label=label)
        if failure is not None:
            failures.append(failure)
    return failures


def _named_call_count(function: Any, name: str) -> int:
    tree = ast.parse(textwrap.dedent(inspect.getsource(function)))
    return sum(
        1
        for node in ast.walk(tree)
        if isinstance(node, ast.Call)
        and (
            (isinstance(node.func, ast.Name) and node.func.id == name)
            or (isinstance(node.func, ast.Attribute) and node.func.attr == name)
        )
    )


def _literal_revision_domain_shape(node: ast.AST) -> Any:
    if isinstance(node, ast.Set):
        values = []
        for item in node.elts:
            if not isinstance(item, ast.Constant) or not isinstance(item.value, str):
                return None
            values.append(item.value)
        return frozenset(values)
    if isinstance(node, ast.IfExp):
        return (
            "if",
            _literal_revision_domain_shape(node.body),
            _literal_revision_domain_shape(node.orelse),
        )
    return None


def _named_call_keyword_shapes(function: Any, name: str, keyword: str) -> list[Any]:
    tree = ast.parse(textwrap.dedent(inspect.getsource(function)))
    shapes: list[Any] = []
    for node in ast.walk(tree):
        if not isinstance(node, ast.Call):
            continue
        called = (
            node.func.id
            if isinstance(node.func, ast.Name)
            else node.func.attr
            if isinstance(node.func, ast.Attribute)
            else ""
        )
        if called != name:
            continue
        matching = [item for item in node.keywords if item.arg == keyword]
        shapes.extend(_literal_revision_domain_shape(item.value) for item in matching)
    return shapes


def _audit_call_contract_direct_contract(
    progress_module: ModuleType,
    *,
    repository_root: Path = REPO_ROOT,
) -> list[dict[str, str]]:
    """Exercise fresh direct-comparison and explicit-invalidation boundaries."""

    from _recoil.commands import call_contract_verify
    from _recoil.lib import progress_sqlite
    from _recoil.lib.call_contract_generations import (
        current_generations,
        required_call_contract_verifier_component_findings,
    )

    failures: list[dict[str, str]] = []
    acceptance_policy = getattr(
        progress_module, "CALL_CONTRACT_VERIFICATION_ACCEPTANCE_POLICY", None
    )
    if not isinstance(acceptance_policy, Mapping) or any(
        acceptance_policy.get(key) != value
        for key, value in EXPECTED_CALL_CONTRACT_ACCEPTANCE_POLICY.items()
    ):
        failures.append(_finding(
            "call-contract-direct-policy",
            "call-contract acceptance must remain a fresh parent direct-retail operation "
            "with nonaccepting worker results and per-body acceptance",
        ))

    body_result_source = inspect.getsource(
        call_contract_verify._call_contract_body_results
    )
    required_direct_fields = {
        '"expected_fact_row"', '"expected_contract"', '"candidate_contract"',
        '"comparison_passed"', "current_generations()",
    }
    if not required_direct_fields.issubset(set(
        token for token in required_direct_fields if token in body_result_source
    )):
        failures.append(_finding(
            "call-contract-candidate-truth",
            "call-contract verifier lacks bounded exact expected/candidate body results",
        ))
    if "bytes_hex" in body_result_source or "candidate_expected_truth" in body_result_source:
        failures.append(_finding(
            "call-contract-direct-inputs",
            "call-contract body results retain serialized byte identities or candidate truth",
        ))

    generations = current_generations()
    if set(generations) != {
        "call_contract_verifier_generation",
        "normalizer_registry_generation",
        "expected_fact_schema_version",
    } or any(not isinstance(value, int) or value < 1 for value in generations.values()):
        failures.append(_finding(
            "call-contract-generations",
            "verifier, normalizer, and expected-fact generations must be positive integers",
        ))
    for row in required_call_contract_verifier_component_findings(repository_root):
        failures.append(
            _finding(
                "call-contract-required-component",
                f"{row['kind']}: {row['path']}: {row['detail']}",
            )
        )

    expected_domains = {
        "semantic": "semantic_revision",
        "evidence_generation": "evidence_generation_revision",
        "scheduler": "scheduler_revision",
    }
    vector = getattr(progress_sqlite, "SQLiteRevisionVector", None)
    store_type = getattr(progress_sqlite, "ProgressSQLiteStore", None)
    if (
        getattr(progress_sqlite, "REVISION_DOMAIN_COLUMNS", None) != expected_domains
        or tuple(getattr(vector, "__dataclass_fields__", {})) != (
            "transaction_revision",
            "semantic_revision",
            "evidence_generation_revision",
            "scheduler_revision",
        )
        or store_type is None
        or not callable(getattr(store_type, "read_revision_vector", None))
        or not callable(getattr(store_type, "persist_scoped_changes", None))
    ):
        failures.append(_finding(
            "progress-revision-domains",
            "progress SQLite lacks transaction and domain revision/CAS surfaces",
        ))

    parser = progress_module._parser()
    with contextlib.redirect_stderr(io.StringIO()):
        try:
            advance = parser.parse_args([
                "advance-live-call-contract", "--slice",
                "recoil:call-contract-slice:audit", "--packet-id",
                "recoil:explicit-work:audit-call-contract", "--build-root",
                "build/audit-call-contract-direct", "--expected-semantic-revision",
                "11", "--expected-evidence-generation-revision", "13", "--apply",
            ])
        except SystemExit as exc:
            advance = None
            failures.append(_finding(
                "call-contract-domain-guards",
                f"direct acceptance parser is unreachable: {exc}",
            ))
    if advance is not None and (
        advance.expected_semantic_revision != 11
        or advance.expected_evidence_generation_revision != 13
        or getattr(advance, "expected_revision", None) is not None
        or advance.packet_id != "recoil:explicit-work:audit-call-contract"
    ):
        failures.append(_finding(
            "call-contract-domain-guards",
            "direct acceptance parsed to the wrong revision domains",
        ))

    acceptance_source = inspect.getsource(progress_module.advance_live_call_contract)
    main_source = inspect.getsource(progress_module.main)
    if (
        "accept_live_call_contract_symbols" not in acceptance_source
        or "current_generations" not in acceptance_source
        or "--all-caller-divergences" not in acceptance_source
    ):
        failures.append(_finding(
            "call-contract-parent-live-authority",
            "parent acceptance must run the fresh verifier and accept direct body results",
        ))
    if "prepare-repair-continuation is contained-disabled" not in main_source:
        failures.append(_finding(
            "repair-continuation-route",
            "packetless repair continuation is not immediately contained-disabled",
        ))

    repo_tools = REPO_ROOT / "tools" / "_recoil" / "lib"
    present = [
        display_path(path)
        for path in (
            repo_tools / "call_contract_certificates.py",
            repo_tools / "scheduler_cache.py",
        )
        if path.exists()
    ]
    if present:
        failures.append(_finding(
            "removed-persistence-modules",
            f"removed persistence modules still exist: {present}",
        ))
    return failures


def _audit_parser_and_validator_calls(
    progress_module: ModuleType,
    invocation_counts: Mapping[str, int] | None,
) -> list[dict[str, str]]:
    failures: list[dict[str, str]] = []
    try:
        parsed = progress_module._parser().parse_args(
            [
                "work",
                "claim-current",
                "--expected-revision",
                "7",
                "--apply",
            ]
        )
        if (
            parsed.command != "work"
            or parsed.work_command != "claim-current"
            or parsed.lane != "primary"
            or parsed.expected_revision != 7
            or parsed.apply is not True
        ):
            failures.append(
                _finding("progress-parser", "claim-current parsed to the wrong dispatch contract")
            )
    except (AttributeError, SystemExit) as exc:
        failures.append(_finding("progress-parser", f"claim-current is unreachable: {exc}"))

    try:
        parsed_all = progress_module._parser().parse_args(
            [
                "work",
                "claim-current",
                "--lane",
                "all",
                "--max-packets",
                "2",
                "--expected-revision",
                "7",
                "--apply",
            ]
        )
        if parsed_all.lane != "all" or parsed_all.max_packets != 2:
            failures.append(
                _finding("progress-parser", "multi-lane claim parsed to the wrong contract")
            )
        parsed_handoff = progress_module._parser().parse_args(
            ["handoff", "--packet-id", "recoil:work:sample", "--json"]
        )
        if parsed_handoff.packet_id != "recoil:work:sample":
            failures.append(
                _finding("progress-parser", "packet-id handoff parsed to the wrong contract")
            )
    except (AttributeError, SystemExit) as exc:
        failures.append(
            _finding("progress-parser", f"multi-lane claim or packet-id handoff is unreachable: {exc}")
        )

    try:
        full_convergence = progress_module._parser().parse_args(
            [
                "call-contract",
                "prepare-live-convergence",
                "--closeout",
                "--packet-id",
                "recoil:explicit-work:audit-full-convergence",
                "--build-root",
                "build/audit-full-convergence",
                "--jobs",
                "2",
                "--expected-semantic-revision",
                "11",
                "--expected-evidence-generation-revision",
                "13",
                "--dry-run",
            ]
        )
        if (
            full_convergence.command != "call-contract"
            or full_convergence.call_contract_command
            != "prepare-live-convergence"
            or full_convergence.packet_id
            != "recoil:explicit-work:audit-full-convergence"
            or Path(full_convergence.issue_ledger).resolve()
            != Path(progress_module.DEFAULT_ISSUE_LEDGER).resolve()
            or hasattr(full_convergence, "target")
            or full_convergence.closeout is not True
            or full_convergence.expected_semantic_revision != 11
            or full_convergence.expected_evidence_generation_revision != 13
            or getattr(full_convergence, "expected_revision", None) is not None
        ):
            failures.append(
                _finding(
                    "full-convergence-closeout-route",
                    "full convergence lacks its canonical issue-ledger lease boundary or exposes a target override",
                )
            )
    except (AttributeError, SystemExit, TypeError) as exc:
        failures.append(
            _finding(
                "full-convergence-closeout-route",
                f"full convergence closeout route is unreachable: {exc}",
            )
        )

    try:
        with contextlib.redirect_stderr(io.StringIO()):
            continuation = progress_module._parser().parse_args(
                [
                    "call-contract",
                    "prepare-repair-continuation",
                    "--returned-work-item",
                    "recoil:work:call-contract:audit-fixture",
                    "--linked-tool-issue",
                    "WSI-20260816-002",
                    "--build-root",
                    "build/audit-repair-continuation",
                    "--expected-revision",
                    "7",
                    "--dry-run",
                    "--json",
                ]
            )
        if (
            continuation.command != "call-contract"
            or continuation.call_contract_command
            != "prepare-repair-continuation"
            or continuation.returned_work_item
            != "recoil:work:call-contract:audit-fixture"
            or continuation.linked_tool_issue != "WSI-20260816-002"
            or continuation.expected_revision != 7
            or continuation.dry_run is not True
            or continuation.apply is not False
            or any(
                hasattr(continuation, forbidden)
                for forbidden in ("target", "jobs", "slice", "scope")
            )
            or Path(continuation.progress).resolve()
            != Path(progress_module.DEFAULT_PROGRESS).resolve()
            or Path(continuation.issue_ledger).resolve()
            != Path(progress_module.DEFAULT_ISSUE_LEDGER).resolve()
        ):
            failures.append(
                _finding(
                    "repair-continuation-route",
                    "repair continuation parsed to a broad or noncanonical parent contract",
                )
            )
    except (AttributeError, SystemExit, TypeError) as exc:
        failures.append(
            _finding(
                "repair-continuation-route",
                f"repair continuation parent route is unreachable: {exc}",
            )
        )

    try:
        with contextlib.redirect_stderr(io.StringIO()):
            progress_module._parser().parse_args(
                [
                    "call-contract",
                    "prepare-repair-continuation",
                    "--returned-work-item",
                    "recoil:work:call-contract:audit-fixture",
                    "--linked-tool-issue",
                    "WSI-20260816-002",
                    "--build-root",
                    "build/audit-repair-continuation",
                    "--expected-revision",
                    "7",
                    "--dry-run",
                    "--target",
                    "recoil:vc5-target:forbidden",
                ]
            )
    except SystemExit:
        pass
    else:
        failures.append(
            _finding(
                "repair-continuation-route",
                "repair continuation accepts a forbidden target override",
            )
        )

    counts = dict(invocation_counts or {})
    if invocation_counts is None:
        try:
            counts = {
                "advance-live-order": _named_call_count(
                    progress_module.advance_live_order, "_run_json_process"
                ),
                "advance-live-byte": _named_call_count(
                    progress_module.advance_live_byte, "_run_json_process"
                ),
                "advance-live-call-contract": _named_call_count(
                    progress_module.advance_live_call_contract, "_run_json_process"
                ),
            }
            if getattr(
                progress_module,
                "CALL_CONTRACT_VERIFICATION_RECORD_ACCEPTANCE_ENABLED",
                None,
            ) is False:
                # The direct-verification audit above separately proves the disabled gate
                # precedes the retained implementation.  Unreachable legacy code is
                # not a runnable validator invocation.
                counts["advance-live-call-contract"] = 0
        except (AttributeError, OSError, TypeError, SyntaxError) as exc:
            failures.append(
                _finding("single-validator-invocation", f"cannot inspect live acceptance: {exc}")
            )
            return failures
        try:
            continuation_handler = (
                progress_module.prepare_call_contract_repair_continuation
            )
            evaluator_count = _named_call_count(
                continuation_handler, "prepare_repair_continuation"
            )
            acceptance_count = _named_call_count(
                continuation_handler, "advance_live_call_contract"
            )
            if evaluator_count != 1 or acceptance_count != 0:
                failures.append(
                    _finding(
                        "repair-continuation-nonaccepting",
                        "repair continuation must invoke exactly one continuation evaluator and no live acceptance path",
                    )
                )
        except (AttributeError, OSError, TypeError, SyntaxError) as exc:
            failures.append(
                _finding(
                    "repair-continuation-nonaccepting",
                    f"cannot inspect repair continuation parent boundary: {exc}",
                )
            )
    expected_invocations = {
        "advance-live-order": 1,
        "advance-live-byte": 1,
        "advance-live-call-contract": (
            0
            if getattr(
                progress_module,
                "CALL_CONTRACT_VERIFICATION_RECORD_ACCEPTANCE_ENABLED",
                None,
            )
            is False
            else 1
        ),
    }
    for command, expected in expected_invocations.items():
        count = counts.get(command)
        if count != expected:
            failures.append(
                _finding(
                    "single-validator-invocation",
                    f"{command} launches {count!r} validators; exactly {expected} is required "
                    "by the current contained command contract",
                )
            )
    return failures


def _audit_relocation_target_mutation() -> list[dict[str, str]]:
    from _recoil.commands import relocation_target_mutation

    failures: list[dict[str, str]] = []
    try:
        parsed = relocation_target_mutation.build_parser().parse_args(
            [
                "bind",
                "--source-symbol-id",
                "recoil:function:0x401000",
                "--source-address",
                "0x401000",
                "--payload-json",
                "{}",
                "--expected-revision",
                "7",
                "--apply",
                "--json",
            ]
        )
        if (
            parsed.command != "bind"
            or parsed.expected_revision != 7
            or parsed.apply is not True
            or parsed.dry_run is not False
        ):
            failures.append(
                _finding(
                    "relocation-target-mutation",
                    "relocation-target bind parsed to the wrong CAS mutation contract",
                )
            )
    except (AttributeError, SystemExit) as exc:
        failures.append(
            _finding(
                "relocation-target-mutation",
                f"relocation-target bind parser is unreachable: {exc}",
            )
        )
    try:
        commit_count = _named_call_count(
            relocation_target_mutation.bind_relocation_target, "commit"
        )
        if commit_count != 1:
            failures.append(
                _finding(
                    "relocation-target-mutation",
                    f"relocation-target bind contains {commit_count} commit calls; exactly one is required",
                )
            )
    except (OSError, TypeError, SyntaxError) as exc:
        failures.append(
            _finding(
                "relocation-target-mutation",
                f"cannot inspect relocation-target CAS contract: {exc}",
            )
        )
    return failures


def _audit_workspace_worktree_contract() -> list[dict[str, str]]:
    from _recoil.commands import worktree_control
    from _recoil.lib import worktree_control as worktree_lib

    failures: list[dict[str, str]] = []
    if (
        worktree_lib.PROGRESS_ADAPTER_STATE != "contained-disabled"
        or worktree_lib.PROGRESS_ADAPTER_REASON
        != "progress packets do not yet record a native-Git baseline"
    ):
        failures.append(_finding(
            "progress-worktree-adapter-containment",
            "progress worktree adapter is not exactly contained-disabled",
        ))
    parser = worktree_control.build_parser()
    parser_cases = {
        "status": ["status", "--json"],
        "create": ["create", "--authority", "issue", "--id", "packet", "--expected-revision", "7", "--apply"],
        "validate": ["validate", "--id", "packet", "--json"],
        "integrate": ["integrate", "--id", "packet", "--apply"],
        "retire": ["retire", "--id", "packet", "--apply"],
        "hygiene": ["hygiene", "--strict", "--json"],
    }
    for operation, arguments in parser_cases.items():
        try:
            parsed = parser.parse_args(arguments)
        except SystemExit as exc:
            failures.append(_finding(
                "workspace-worktree-lifecycle",
                f"workspace worktree {operation} parser is unreachable: {exc}",
            ))
            continue
        if parsed.command != operation:
            failures.append(_finding(
                "workspace-worktree-lifecycle",
                f"workspace worktree {operation} parsed to the wrong operation",
            ))
    try:
        progress = parser.parse_args([
            "create", "--authority", "progress", "--id", "packet",
            "--expected-revision", "7", "--apply",
        ])
        if progress.authority != "progress":
            raise ValueError("progress containment selector disappeared")
    except (SystemExit, ValueError) as exc:
        failures.append(_finding(
            "progress-worktree-adapter-containment",
            f"progress contained-disabled route cannot be represented fail-closed: {exc}",
        ))
    handoff_source = inspect.getsource(
        __import__(
            "_recoil.commands.workspace_packet_handoff",
            fromlist=["_compact_reserved_packet"],
        )._compact_reserved_packet
    )
    for required in (
        "resolve_exact_packet_worktree",
        "external_build_root",
        "worker_may_create_one_packet_commit",
        "worker_branch_worktree_integration_allowed",
    ):
        if required not in handoff_source:
            failures.append(_finding(
                "workspace-worktree-lifecycle",
                f"workspace handoff omits required worktree contract {required!r}",
            ))
    for required in (
        "authenticated_validation_command_tokens",
        "validation_command_contract_version",
        "resource_claims=claims",
        "len(validation_commands) != 1",
        "worker_command != commands[0]",
    ):
        if required not in handoff_source:
            failures.append(_finding(
                "workspace-handoff-validation-command",
                "workspace handoff omits the shared exact command contract "
                f"boundary {required!r}",
            ))
    for forbidden in (
        "lowered = command.casefold()",
        '"progress advance-live-" in lowered',
        '"issue work close" in lowered',
    ):
        if forbidden in handoff_source:
            failures.append(_finding(
                "workspace-handoff-validation-command",
                "workspace handoff retains a parallel substring command policy "
                f"{forbidden!r}",
            ))
    return failures


def _audit_integration_validation_guidance(
    execution_root: Path,
) -> list[dict[str, str]]:
    """Require the canonical runbook to keep fallible checks before master moves."""

    skill_path = execution_root / ".codex" / "skills" / "recoil-validation" / "SKILL.md"
    try:
        guidance = skill_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [
            _finding(
                "integration-validation-order",
                f"cannot read executing-worktree validation guidance {skill_path}: {exc}",
            )
        ]

    lowered = guidance.casefold()
    failures: list[dict[str, str]] = []
    prohibited = (
        "and again on canonical `master`",
        "rerun fallible semantic validation on canonical `master`",
    )
    if any(text in lowered for text in prohibited):
        failures.append(
            _finding(
                "integration-validation-order",
                "validation guidance still requires fallible semantic validation after master advances",
            )
        )
    required_fragments = (
        ("before `master` advances",),
        ("deterministic",),
        ("canonical control root", "canonical-control-root"),
        ("executing worktree",),
    )
    missing = [
        alternatives[0]
        for alternatives in required_fragments
        if not any(fragment in lowered for fragment in alternatives)
    ]
    if missing:
        failures.append(
            _finding(
                "integration-validation-order",
                "validation guidance omits the pre-fast-forward/root-routing contract: "
                + ", ".join(missing),
            )
        )
    return failures


def audit_workflow_contracts(
    *,
    specs: Iterable[Any] | None = None,
    progress_module: ModuleType | None = None,
    invocation_counts: Mapping[str, int] | None = None,
    generated_command_payload: Mapping[str, Any] | None = None,
    generated_command_document: Any | None = None,
    progress_path: Path | None = None,
    executing_worktree_root: Path = REPO_ROOT,
    canonical_root: Path | None = None,
) -> dict[str, Any]:
    execution_root = executing_worktree_root.resolve(strict=True)
    canonical = None
    if progress_module is None:
        from _recoil.commands import progress_cli as progress_module

    failures = _audit_registry(registry_view(specs))
    failures.extend(
        _audit_call_contract_direct_contract(
            progress_module, repository_root=execution_root
        )
    )
    failures.extend(_audit_parser_and_validator_calls(progress_module, invocation_counts))
    failures.extend(_audit_handoff(progress_module))
    failures.extend(_audit_relocation_target_mutation())
    failures.extend(_audit_workspace_worktree_contract())
    failures.extend(_audit_integration_validation_guidance(execution_root))
    if generated_command_document is None:
        from _recoil.lib.progress import ProgressDocument
        canonical_requested = bool(
            canonical_root is not None or "RECOIL_CANONICAL_ROOT" in os.environ
        )
        if progress_path is None or canonical_requested:
            from _recoil.lib.worktree_control import (
                WorktreeControlError,
                resolve_canonical_control_root,
            )

            canonical = resolve_canonical_control_root(
                executing_worktree_root=execution_root,
                required_machine_local_paths=(
                    ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
                ),
                explicit_root=canonical_root,
            )
            canonical_progress_path = (
                canonical.canonical_control_root
                / ".agent"
                / "RECONSTRUCTION_PROGRESS.sqlite3"
            )
            if progress_path is None:
                progress_path = canonical_progress_path
            else:
                try:
                    supplied_progress = progress_path.resolve(strict=True)
                    expected_progress = canonical_progress_path.resolve(strict=True)
                except OSError as exc:
                    raise WorktreeControlError(
                        f"cannot authenticate explicit progress path: {exc}"
                    ) from exc
                if supplied_progress != expected_progress:
                    raise WorktreeControlError(
                        "explicit progress path does not equal the authenticated "
                        f"canonical authority: {supplied_progress} != {expected_progress}"
                    )
                progress_path = supplied_progress
        try:
            generated_command_document = ProgressDocument.load(progress_path)
        except BaseException:
            if canonical is not None:
                from _recoil.lib.worktree_control import reauthenticate_canonical_control_root

                reauthenticate_canonical_control_root(canonical)
            raise
    try:
        if generated_command_payload is None:
            generated_command_payload = generated_command_document.pipeline("recoil")
        failures.extend(
            audit_generated_call_contract_commands(
                generated_command_document, generated_command_payload
            )
        )
    finally:
        if canonical is not None:
            from _recoil.lib.worktree_control import reauthenticate_canonical_control_root

            reauthenticate_canonical_control_root(canonical)
    return {
        "report_version": 1,
        "kind": "workflow-contract-audit",
        "passed": not failures,
        "checks": {
            "public_registry": "passed" if not any(
                item["check"] == "public-registry" for item in failures
            ) else "failed",
            "claim_current_parser": "passed" if not any(
                item["check"] == "progress-parser" for item in failures
            ) else "failed",
            "repair_continuation_route": "passed" if not any(
                item["check"] == "repair-continuation-route"
                for item in failures
            ) else "failed",
            "repair_continuation_nonaccepting": "passed" if not any(
                item["check"] == "repair-continuation-nonaccepting"
                for item in failures
            ) else "failed",
            "full_convergence_closeout_route": "passed" if not any(
                item["check"] == "full-convergence-closeout-route"
                for item in failures
            ) else "failed",
            "call_contract_direct_policy": "passed" if not any(
                item["check"] in {
                    "call-contract-direct-policy",
                    "call-contract-candidate-truth",
                    "call-contract-direct-inputs",
                    "call-contract-generations",
                    "call-contract-required-component",
                    "call-contract-domain-guards",
                    "progress-revision-domains",
                    "call-contract-parent-live-authority",
                    "removed-persistence-modules",
                }
                for item in failures
            ) else "failed",
            "compact_reserved_handoff": "passed" if not any(
                item["check"] == "compact-reserved-handoff" for item in failures
            ) else "failed",
            "worker_receipt_nonaccepting": "passed" if not any(
                item["check"] == "worker-receipt-nonaccepting"
                for item in failures
            ) else "failed",
            "single_validator_invocation": "passed" if not any(
                item["check"] == "single-validator-invocation" for item in failures
            ) else "failed",
            "verify_vs_advance_mutation_boundary": "passed" if not any(
                item["check"] == "public-registry" and "contract" in item["message"]
                for item in failures
            ) else "failed",
            "relocation_target_mutation": "passed" if not any(
                item["check"] == "relocation-target-mutation" for item in failures
            ) else "failed",
            "generated_live_commands": "passed" if not any(
                item["check"] == "generated-call-contract-command"
                for item in failures
            ) else "failed",
            "workspace_worktree_lifecycle": "passed" if not any(
                item["check"] == "workspace-worktree-lifecycle"
                for item in failures
            ) else "failed",
            "workspace_handoff_validation_command": "passed" if not any(
                item["check"] == "workspace-handoff-validation-command"
                for item in failures
            ) else "failed",
            "integration_validation_order": "passed" if not any(
                item["check"] == "integration-validation-order"
                for item in failures
            ) else "failed",
            "progress_worktree_adapter_containment": "passed" if not any(
                item["check"] == "progress-worktree-adapter-containment"
                for item in failures
            ) else "failed",
        },
        "failure_count": len(failures),
        "failures": failures,
        "execution_worktree_root": str(execution_root),
        "canonical_control_root": (
            str(canonical.canonical_control_root) if canonical is not None else None
        ),
        "canonical_resolution_source": (
            canonical.resolution_source if canonical is not None else None
        ),
        "progress_path": str(progress_path) if progress_path is not None else None,
        "tracked_contract_inputs_from_execution_worktree": True,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Exercise live work-packet, handoff, parser, mutation-boundary, and "
            "single-validator acceptance contracts."
        )
    )
    parser.add_argument("--root", type=Path, default=REPO_ROOT)
    parser.add_argument("--canonical-root", type=Path)
    parser.add_argument("--progress", type=Path)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    if args.root.resolve() != REPO_ROOT.resolve():
        report = {
            "report_version": 1,
            "kind": "workflow-contract-audit",
            "passed": False,
            "failure_count": 1,
            "failures": [
                _finding(
                    "repository-root",
                    f"audit backend is bound to {display_path(REPO_ROOT)}, not {args.root}",
                )
            ],
        }
    else:
        report = audit_workflow_contracts(
            progress_path=args.progress,
            executing_worktree_root=args.root,
            canonical_root=args.canonical_root,
        )
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"Workflow contracts: {'PASS' if report['passed'] else 'FAIL'}")
        for failure in report["failures"]:
            print(f"- {failure['check']}: {failure['message']}")
    # Structural failures always fail; --strict is accepted for audit-suite symmetry.
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
