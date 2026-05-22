from __future__ import annotations

import argparse
from collections import deque
from dataclasses import dataclass, field
import json
from pathlib import Path
import re
import sys

from recoil_binja import BinaryNinjaBridge, BridgeError, Symbol
from recoil_plan import (
    FIELD_LABELS,
    FUNCTIONAL_LANE,
    LANE_CHOICES,
    PlanEntry,
    blocker_field,
    load_plan,
    normalize_address,
    normalize_lane,
    status_summary,
)
from recoil_vc6_verify import DEFAULT_MANIFEST_DIR, load_manifests


CALL_RE = re.compile(r"\bcall\s+([^;]+)")
MLIL_CALL_RE = re.compile(r"\b(0x[0-9A-Fa-f]+)\(")
ADDR_RE = re.compile(r"^(0x[0-9A-Fa-f]+)$")
INDIRECT_CALL_PREFIXES = ("byte ", "word ", "dword ", "qword ", "[")
INDIRECT_CALL_REGISTERS = {
    "eax",
    "ebx",
    "ecx",
    "edx",
    "esi",
    "edi",
    "esp",
    "ebp",
}
ORIGINAL_IMAGE_START = 0x00400000
ORIGINAL_IMAGE_END = 0x004FFFFF
PROVIDER_KINDS = {"import", "external", "compiler"}
COMPILER_GENERATED_SYMBOL_PREFIXES = (
    "MSVC_",
)


@dataclass
class Node:
    address: str
    name: str = ""
    kind: str = "function"
    plan: PlanEntry | None = None
    callees: list[str] = field(default_factory=list)
    unresolved_calls: list[str] = field(default_factory=list)
    indirect_calls: list[str] = field(default_factory=list)
    bridge_error: str = ""

    def status_summary(self, *, lane: str = FUNCTIONAL_LANE) -> str:
        if self.plan is None:
            return "provided by compiler/runtime" if self.kind == "compiler" else "not in plan"
        return status_summary(self.plan)

    def blocks_source(self) -> bool:
        if self.plan is None:
            return self.kind not in PROVIDER_KINDS
        return not self.plan.is_source_dependencies_satisfied or not self.plan.is_reimplemented

    def source_block_reason(self) -> str:
        if self.plan is None:
            return "not in plan" if self.kind not in PROVIDER_KINDS else ""
        if not self.plan.is_source_dependencies_satisfied:
            return f"deps={self.plan.source_dependencies_status}"
        if not self.plan.is_reimplemented:
            return f"impl={self.plan.reimplemented_status}"
        return ""

    def blocks_verification(self, *, lane: str = FUNCTIONAL_LANE) -> bool:
        if self.plan is None:
            return self.kind not in PROVIDER_KINDS
        return blocker_field(self.plan, lane=lane) in {"functional", "verify"}

    def verification_block_reason(self, *, lane: str = FUNCTIONAL_LANE) -> str:
        if self.plan is None:
            return "not in plan" if self.kind not in PROVIDER_KINDS else ""
        field = blocker_field(self.plan, lane=lane)
        if field not in {"functional", "verify"}:
            return ""
        return f"{FIELD_LABELS.get(field, field)}={getattr(self.plan, _status_attr(field))}"

    def blocks_binary_verification(self, *, lane: str = FUNCTIONAL_LANE) -> bool:
        return self.blocks_verification(lane=lane)


def _status_attr(field: str) -> str:
    if field == "functional":
        return "functional_equivalent_status"
    if field == "verify":
        return "binary_verified_status"
    return ""


def node_to_dict(node: Node, *, lane: str = FUNCTIONAL_LANE) -> dict[str, object]:
    return {
        "address": node.address,
        "name": node.name,
        "kind": node.kind,
        "status": node.status_summary(lane=lane),
        "blocks_source": node.blocks_source(),
        "source_block_reason": node.source_block_reason(),
        "blocks_verification": node.blocks_verification(lane=lane),
        "blocks_binary_verification": node.blocks_binary_verification(lane=lane),
        "verification_block_reason": node.verification_block_reason(lane=lane),
        "callees": node.callees,
        "unresolved_calls": node.unresolved_calls,
        "indirect_calls": node.indirect_calls,
        "bridge_error": node.bridge_error,
    }


def vc6_coverage_by_address(manifest_dir: Path) -> dict[str, list[str]]:
    coverage: dict[str, list[str]] = {}
    for manifest in load_manifests(manifest_dir):
        for function in manifest.functions:
            coverage.setdefault(function.address, []).append(manifest.name)
    return coverage


def extract_call_tokens(assembly: str, mlil: str) -> list[str]:
    tokens: list[str] = []
    for line in assembly.splitlines():
        match = CALL_RE.search(line)
        if match:
            tokens.append(match.group(1).strip())
    for line in mlil.splitlines():
        for match in MLIL_CALL_RE.finditer(line):
            tokens.append(match.group(1).strip())
    return tokens


def is_indirect_call_token(token: str) -> bool:
    text = " ".join(token.strip().lower().split())
    return text in INDIRECT_CALL_REGISTERS or text.startswith(INDIRECT_CALL_PREFIXES)


def resolve_call(token: str, symbols_by_name: dict[str, Symbol]) -> str | None:
    token = token.strip()
    if ADDR_RE.match(token):
        return normalize_address(token)
    if token.lower().startswith("sub_"):
        return normalize_address(token)
    symbol = symbols_by_name.get(token)
    if symbol:
        return symbol.address
    return None


def inferred_symbol_kind(address: str, symbol: Symbol | None) -> str:
    if symbol is not None:
        if is_compiler_generated_symbol(symbol.name):
            return "compiler"
        return symbol.kind
    value = int(normalize_address(address), 16)
    if value < ORIGINAL_IMAGE_START or value > ORIGINAL_IMAGE_END:
        return "external"
    return "function"


def is_compiler_generated_symbol(name: str) -> bool:
    return name.startswith(COMPILER_GENERATED_SYMBOL_PREFIXES)


def load_node(
    address: str,
    bridge: BinaryNinjaBridge,
    plan: dict[str, PlanEntry],
    symbols_by_address: dict[str, Symbol],
    symbols_by_name: dict[str, Symbol],
) -> Node:
    address = normalize_address(address)
    symbol = symbols_by_address.get(address)
    entry = plan.get(address)
    node = Node(
        address=address,
        name=(symbol.name if symbol else (entry.reconstructed_name if entry else "")),
        kind=inferred_symbol_kind(address, symbol),
        plan=entry,
    )
    try:
        assembly = bridge.assembly(address)
        mlil = bridge.il(address, view="mlil")
    except BridgeError as exc:
        node.bridge_error = str(exc)
        return node

    seen: set[str] = set()
    for token in extract_call_tokens(assembly, mlil):
        if is_indirect_call_token(token):
            if token not in node.indirect_calls:
                node.indirect_calls.append(token)
            continue
        resolved = resolve_call(token, symbols_by_name)
        if resolved and resolved != address and resolved not in seen:
            seen.add(resolved)
            node.callees.append(resolved)
        elif not resolved and token not in node.unresolved_calls:
            node.unresolved_calls.append(token)
    return node


def load_metadata_node(
    address: str,
    plan: dict[str, PlanEntry],
    symbols_by_address: dict[str, Symbol],
) -> Node:
    address = normalize_address(address)
    symbol = symbols_by_address.get(address)
    entry = plan.get(address)
    return Node(
        address=address,
        name=(symbol.name if symbol else (entry.reconstructed_name if entry else "")),
        kind=inferred_symbol_kind(address, symbol),
        plan=entry,
    )


def build_frontier(
    address: str,
    depth: int,
    bridge: BinaryNinjaBridge,
    plan: dict[str, PlanEntry],
) -> dict[str, Node]:
    symbols_by_address, symbols_by_name = bridge.symbols()
    root = normalize_address(address)
    nodes: dict[str, Node] = {}
    queue: deque[tuple[str, int]] = deque([(root, 0)])
    while queue:
        current, level = queue.popleft()
        if current in nodes:
            continue
        if level >= depth:
            node = load_metadata_node(current, plan, symbols_by_address)
        else:
            node = load_node(current, bridge, plan, symbols_by_address, symbols_by_name)
        nodes[current] = node
        if level >= depth:
            continue
        for callee in node.callees:
            if callee not in nodes:
                queue.append((callee, level + 1))
    return nodes


def frontier_truncated(nodes: dict[str, Node]) -> bool:
    return any(is_budget_error_text(node.bridge_error) for node in nodes.values())


def is_budget_error_text(text: str) -> bool:
    return "bridge call budget exhausted" in text.lower()


def recommendation(root: str, nodes: dict[str, Node], *, lane: str = FUNCTIONAL_LANE) -> str:
    lane = normalize_lane(lane)
    root_node = nodes[root]
    for callee in root_node.callees:
        node = nodes.get(callee)
        if node and node.blocks_source():
            return (
                f"{node.address} {node.name} "
                f"(lowest visible blocking callee: {node.source_block_reason()})"
            )
    if root_node.blocks_source():
        if root_node.plan and not root_node.plan.is_source_dependencies_satisfied:
            return (
                f"{root_node.address} {root_node.name} "
                "(audit/update source dependencies before implementation)"
            )
        return (
            f"{root_node.address} {root_node.name} "
            "(no lower blocking callee visible at this depth)"
        )
    for callee in root_node.callees:
        node = nodes.get(callee)
        if node and node.blocks_verification(lane=lane):
            if lane == FUNCTIONAL_LANE:
                return f"{node.address} {node.name} (source done; functional equivalence blocks caller)"
            return f"{node.address} {node.name} (source done; binary verification blocks caller)"
    if root_node.blocks_verification(lane=lane):
        if lane == FUNCTIONAL_LANE:
            return f"{root_node.address} {root_node.name} (source done; functionally accept this anchor)"
        return f"{root_node.address} {root_node.name} (source done; verify this anchor)"
    if frontier_truncated(nodes):
        return "No blocking dependency visible before Binary Ninja call budget was exhausted."
    return "No blocking dependency visible at requested depth."


def print_report(
    root: str,
    nodes: dict[str, Node],
    *,
    vc6_coverage: dict[str, list[str]] | None = None,
    lane: str = FUNCTIONAL_LANE,
) -> None:
    lane = normalize_lane(lane)
    vc6_coverage = vc6_coverage or {}
    root_node = nodes[root]
    print("# Recoil Dependency Frontier")
    print()
    print(f"Anchor: `{root_node.address}` `{root_node.name}`")
    if root_node.plan:
        print(f"Milestone: {root_node.plan.milestone}")
        print(f"Anchor status: {root_node.status_summary(lane=lane)}")
        print(f"Lane: {lane}")
    if frontier_truncated(nodes):
        print("Truncated: yes (Binary Ninja call budget exhausted)")
    print()
    print("## Direct Callees")
    if not root_node.callees:
        print("- none visible in parsed assembly/MLIL")
    for callee in root_node.callees:
        node = nodes.get(callee)
        if node:
            provider = f" provider={node.plan.provider}" if node.plan and node.plan.provider else ""
            error = f" bridge_error={node.bridge_error!r}" if node.bridge_error else ""
            print(f"- `{node.address}` `{node.name}` {node.status_summary(lane=lane)}{provider}{error}")
        else:
            print(f"- `{callee}` not expanded")
    if root_node.unresolved_calls:
        print()
        print("## Unresolved Call Tokens")
        for token in root_node.unresolved_calls:
            print(f"- `{token}`")
    if root_node.indirect_calls:
        print()
        print("## Indirect Calls")
        for token in root_node.indirect_calls:
            print(f"- `{token}`")
    print()
    print("## Verification Coverage")
    reported = [root_node.address, *root_node.callees]
    for address in reported:
        node = nodes.get(address)
        name = node.name if node else ""
        targets = vc6_coverage.get(address, [])
        target_text = ", ".join(targets) if targets else "none"
        print(f"- `{address}` `{name}` targets={target_text}")
    print()
    print("## Blocking Dependencies")
    source_blockers = [node for node in nodes.values() if node.address != root and node.blocks_source()]
    lane_blockers = [
        node
        for node in nodes.values()
        if node.address != root and not node.blocks_source() and node.blocks_verification(lane=lane)
    ]
    if not source_blockers and not lane_blockers:
        print("- no source blockers visible at requested depth")
    for node in source_blockers:
        print(
            f"- `{node.address}` `{node.name}` {node.status_summary(lane=lane)} "
            f"reason={node.source_block_reason()}"
        )
    for node in lane_blockers:
        print(
            f"- `{node.address}` `{node.name}` {node.status_summary(lane=lane)} "
            f"reason={node.verification_block_reason(lane=lane)}"
        )
    print()
    print("## Recommended Next")
    print(f"- {recommendation(root, nodes, lane=lane)}")
    print()
    print(
        "Note: this report uses direct calls parsed from BN assembly/MLIL within the "
        "10-call Binary Ninja budget. Inspect vtable calls, indirect calls, globals, "
        "and shared types in BN before editing source."
    )


def report_to_dict(
    root: str,
    nodes: dict[str, Node],
    *,
    vc6_coverage: dict[str, list[str]] | None = None,
    lane: str = FUNCTIONAL_LANE,
) -> dict[str, object]:
    lane = normalize_lane(lane)
    vc6_coverage = vc6_coverage or {}
    root_node = nodes[root]
    return {
        "lane": lane,
        "truncated_due_to_bn_call_budget": frontier_truncated(nodes),
        "anchor": node_to_dict(root_node, lane=lane),
        "direct_callees": [
            node_to_dict(nodes[callee], lane=lane) for callee in root_node.callees if callee in nodes
        ],
        "vc6_coverage": {address: vc6_coverage.get(address, []) for address in [root, *root_node.callees]},
        "blocking_dependencies": [
                node_to_dict(node, lane=lane)
            for node in nodes.values()
            if node.address != root and node.blocks_source()
        ],
        "lane_blocking_dependencies": [
            node_to_dict(node, lane=lane)
            for node in nodes.values()
            if node.address != root and not node.blocks_source() and node.blocks_verification(lane=lane)
        ],
        "recommended_next": recommendation(root, nodes, lane=lane),
    }


def main(argv: list[str] | None = None) -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8")

    parser = argparse.ArgumentParser(
        description="Report the dependency frontier for a Recoil function anchor."
    )
    parser.add_argument("address", help="Anchor function address, e.g. 0x4301e0")
    parser.add_argument(
        "--depth",
        type=int,
        default=1,
        help="Direct-call traversal depth. Default: 1",
    )
    parser.add_argument("--plan", default=".agent/RECOIL_PLAN.md", help="Path to RECOIL_PLAN.md")
    parser.add_argument(
        "--bridge-url",
        default="http://localhost:9009",
        help="Binary Ninja bridge URL",
    )
    parser.add_argument("--vc6-manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--json", action="store_true", help="Emit a machine-readable frontier report.")
    parser.add_argument(
        "--lane",
        choices=LANE_CHOICES,
        default=FUNCTIONAL_LANE,
        help="functional stops at Functional-equivalent; binary also requires Binary-safe.",
    )
    args = parser.parse_args(argv)

    plan_path = Path(args.plan)
    if not plan_path.exists():
        print(f"Plan file not found: {plan_path}", file=sys.stderr)
        return 2
    try:
        plan = load_plan(plan_path)
        bridge = BinaryNinjaBridge(args.bridge_url)
        root = normalize_address(args.address)
        nodes = build_frontier(root, args.depth, bridge, plan)
        coverage = vc6_coverage_by_address(Path(args.vc6_manifest_dir))
        lane = normalize_lane(args.lane)
        if args.json:
            print(
                json.dumps(
                    report_to_dict(root, nodes, vc6_coverage=coverage, lane=lane),
                    indent=2,
                    ensure_ascii=False,
                )
            )
        else:
            print_report(root, nodes, vc6_coverage=coverage, lane=lane)
    except (BridgeError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
