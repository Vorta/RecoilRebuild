from __future__ import annotations

import argparse
from collections import deque
from dataclasses import dataclass, field
from pathlib import Path
import re
import sys

from recoil_binja import BinaryNinjaBridge, BridgeError, Symbol
from recoil_plan import PlanEntry, load_plan, normalize_address


CALL_RE = re.compile(r"\bcall\s+([^;]+)")
MLIL_CALL_RE = re.compile(r"\b(0x[0-9A-Fa-f]+)\(")
ADDR_RE = re.compile(r"^(0x[0-9A-Fa-f]+)$")
INDIRECT_CALL_PREFIXES = ("byte ", "word ", "dword ", "qword ", "[")


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

    def status_summary(self) -> str:
        if self.plan is None:
            return "not in plan"
        return (
            f"recon={self.plan.reconstructed_status} "
            f"deps={self.plan.source_dependencies_status} "
            f"impl={self.plan.reimplemented_status} "
            f"verify={self.plan.binary_verified_status}"
        )

    def blocks_source(self) -> bool:
        if self.plan is None:
            return self.kind != "import"
        return not self.plan.is_source_dependencies_satisfied or not self.plan.is_reimplemented

    def source_block_reason(self) -> str:
        if self.plan is None:
            return "not in plan" if self.kind != "import" else ""
        if not self.plan.is_source_dependencies_satisfied:
            return f"deps={self.plan.source_dependencies_status}"
        if not self.plan.is_reimplemented:
            return f"impl={self.plan.reimplemented_status}"
        return ""

    def blocks_binary_verification(self) -> bool:
        if self.plan is None:
            return self.kind != "import"
        return not self.plan.is_binary_verified


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
    return text.startswith(INDIRECT_CALL_PREFIXES)


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
        kind=(symbol.kind if symbol else "function"),
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
        node = load_node(current, bridge, plan, symbols_by_address, symbols_by_name)
        nodes[current] = node
        if level >= depth:
            continue
        for callee in node.callees:
            if callee not in nodes:
                queue.append((callee, level + 1))
    return nodes


def recommendation(root: str, nodes: dict[str, Node]) -> str:
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
        if node and node.blocks_binary_verification():
            return f"{node.address} {node.name} (source done; binary verification blocks caller)"
    if root_node.blocks_binary_verification():
        return f"{root_node.address} {root_node.name} (source done; verify this anchor)"
    return "No blocking dependency visible at requested depth."


def print_report(root: str, nodes: dict[str, Node]) -> None:
    root_node = nodes[root]
    print("# Recoil Dependency Frontier")
    print()
    print(f"Anchor: `{root_node.address}` `{root_node.name}`")
    if root_node.plan:
        print(f"Milestone: {root_node.plan.milestone}")
        print(f"Anchor status: {root_node.status_summary()}")
    print()
    print("## Direct Callees")
    if not root_node.callees:
        print("- none visible in parsed assembly/MLIL")
    for callee in root_node.callees:
        node = nodes.get(callee)
        if node:
            provider = f" provider={node.plan.provider}" if node.plan and node.plan.provider else ""
            print(f"- `{node.address}` `{node.name}` {node.status_summary()}{provider}")
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
    print("## Blocking Dependencies")
    blockers = [node for node in nodes.values() if node.address != root and node.blocks_source()]
    if not blockers:
        print("- no source blockers visible at requested depth")
    for node in blockers:
        print(
            f"- `{node.address}` `{node.name}` {node.status_summary()} "
            f"reason={node.source_block_reason()}"
        )
    print()
    print("## Recommended Next")
    print(f"- {recommendation(root, nodes)}")
    print()
    print(
        "Note: this report uses direct calls parsed from BN assembly/MLIL. Inspect vtable "
        "calls, indirect calls, globals, and shared types in BN before editing source."
    )


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
        print_report(root, nodes)
    except (BridgeError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
