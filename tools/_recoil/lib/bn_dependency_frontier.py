from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from collections import deque
from dataclasses import dataclass, field
import json
from pathlib import Path
import re
import sys

from _recoil.lib.binja import DEFAULT_BRIDGE_URL, BinaryNinjaBridge, BridgeError, Symbol
from _recoil.lib.owner_entries import (
    FIELD_LABELS,
    FUNCTIONAL_LANE,
    LANE_CHOICES,
    OwnerEntry,
    blocker_field,
    blocker_priority,
    load_owner_entries,
    normalize_address,
    normalize_lane,
    status_summary,
)
from _recoil.commands.functional_verify import (
    DEFAULT_MANIFEST_DIR as DEFAULT_FUNCTIONAL_MANIFEST_DIR,
    load_manifest as load_functional_manifest,
)
from _recoil.lib.reference_images import reference_image, reference_image_keys, resolve_owner_ledger_path
from _recoil.commands.vc5_verify import (
    DEFAULT_MANIFEST_DIR as DEFAULT_VC5_MANIFEST_DIR,
    load_manifest as load_vc5_manifest,
)


CALL_RE = re.compile(r"\bcall\s+([^;]+)")
MLIL_CALL_RE = re.compile(r"\b(0x[0-9A-Fa-f]+)\(")
ADDR_RE = re.compile(r"^(0x[0-9A-Fa-f]+)$")
IMMEDIATE_ADDR_RE = re.compile(r"\b0x[0-9A-Fa-f]{6,8}\b")
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
    "__alloca_probe",
)
SOURCE_BLOCKER_FIELDS = {"recon", "provider", "deps", "owner", "impl"}
VERIFICATION_BLOCKER_FIELDS = {"functional", "owner", "data", "verify"}


@dataclass
class Node:
    address: str
    name: str = ""
    kind: str = "function"
    entry: OwnerEntry | None = None
    forwarded_to: str = ""
    forwarded_entry: OwnerEntry | None = None
    callees: list[str] = field(default_factory=list)
    function_refs: list[str] = field(default_factory=list)
    unresolved_calls: list[str] = field(default_factory=list)
    indirect_calls: list[str] = field(default_factory=list)
    bridge_error: str = ""

    def status_summary(self, *, lane: str = FUNCTIONAL_LANE) -> str:
        if self.entry is None and self.forwarded_entry is not None:
            return f"forwarder to {self.forwarded_to} {status_summary(self.forwarded_entry)}"
        if self.entry is None:
            return "provided by compiler/runtime" if self.kind == "compiler" else "not tracked by an owner entry"
        return status_summary(self.entry)

    def blocks_source(self, *, lane: str = FUNCTIONAL_LANE) -> bool:
        if self.entry is None and self.forwarded_entry is not None:
            return _source_block_field(self.forwarded_entry, lane=lane) != ""
        if self.entry is None:
            return self.kind not in PROVIDER_KINDS
        return _source_block_field(self.entry, lane=lane) != ""

    def source_block_reason(self, *, lane: str = FUNCTIONAL_LANE) -> str:
        if self.entry is None and self.forwarded_entry is not None:
            field = _source_block_field(self.forwarded_entry, lane=lane)
            reason = _block_reason(self.forwarded_entry, field)
            return f"forwarder target {reason}" if reason else ""
        if self.entry is None:
            return "not tracked by an owner entry" if self.kind not in PROVIDER_KINDS else ""
        field = _source_block_field(self.entry, lane=lane)
        return _block_reason(self.entry, field)

    def blocks_verification(self, *, lane: str = FUNCTIONAL_LANE) -> bool:
        if self.entry is None and self.forwarded_entry is not None:
            return blocker_field(self.forwarded_entry, lane=lane) in VERIFICATION_BLOCKER_FIELDS
        if self.entry is None:
            return self.kind not in PROVIDER_KINDS
        if self.entry.is_provider_ready:
            return False
        return blocker_field(self.entry, lane=lane) in VERIFICATION_BLOCKER_FIELDS

    def verification_block_reason(self, *, lane: str = FUNCTIONAL_LANE) -> str:
        if self.entry is None and self.forwarded_entry is not None:
            field = blocker_field(self.forwarded_entry, lane=lane)
            if field not in VERIFICATION_BLOCKER_FIELDS:
                return ""
            return (
                "forwarder target "
                f"{FIELD_LABELS.get(field, field)}="
                f"{_status_value(self.forwarded_entry, field)}"
            )
        if self.entry is None:
            return "not tracked by an owner entry" if self.kind not in PROVIDER_KINDS else ""
        if self.entry.is_provider_ready:
            return ""
        field = blocker_field(self.entry, lane=lane)
        if field not in VERIFICATION_BLOCKER_FIELDS:
            return ""
        return f"{FIELD_LABELS.get(field, field)}={_status_value(self.entry, field)}"

    def blocks_binary_verification(self, *, lane: str = FUNCTIONAL_LANE) -> bool:
        return self.blocks_verification(lane=lane)


def _status_attr(field: str) -> str:
    if field == "functional":
        return "functional_equivalent_status"
    if field == "verify":
        return "binary_verified_status"
    return ""


def _source_block_field(entry: OwnerEntry, *, lane: str = FUNCTIONAL_LANE) -> str:
    if is_provider_source_ready(entry):
        return ""
    field = blocker_field(entry, lane=lane)
    return field if field in SOURCE_BLOCKER_FIELDS else ""


def _status_value(entry: OwnerEntry, field: str) -> str:
    if field in {"functional", "verify"}:
        return entry.reimplementation_tier
    if field == "owner":
        return f"{entry.source_owner_status or 'pending'}/{entry.source_owner or 'pending'}"
    if field == "data":
        return f"{entry.reimplementation_tier}/{entry.data_status or 'pending'}"
    attr = _status_attr(field)
    return getattr(entry, attr) if attr else ""


def _block_reason(entry: OwnerEntry, field: str) -> str:
    if field == "recon":
        return f"recon={entry.reconstructed_status}"
    if field == "provider":
        return f"{FIELD_LABELS.get(field, field)}={entry.provider_boundary_status}"
    if field == "deps":
        return f"deps={entry.source_dependencies_status}"
    if field == "owner":
        return f"{FIELD_LABELS.get(field, field)}={_status_value(entry, field)}"
    if field == "impl":
        return f"impl={entry.reimplemented_status}"
    return ""


def _verification_block_label(node: Node, *, lane: str) -> str:
    entry = node.entry or node.forwarded_entry
    field = blocker_field(entry, lane=lane) if entry is not None else ""
    if lane == FUNCTIONAL_LANE:
        return "tier C blocks caller"
    if field == "owner":
        return "Source owner blocks caller"
    if field == "data":
        return "Data reimplemented blocks caller"
    return "tier S blocks caller"


def _anchor_verification_label(node: Node, *, lane: str) -> str:
    entry = node.entry or node.forwarded_entry
    field = blocker_field(entry, lane=lane) if entry is not None else ""
    if lane == FUNCTIONAL_LANE:
        return "raise this anchor to tier C"
    if field == "owner":
        return "audit and reimplement the source owner before data or tier S work"
    if field == "data":
        return "reimplement or rule out touched globals and raise this anchor to tier B"
    return "raise this anchor to tier S"


def _node_blocker_priority(node: Node, *, lane: str) -> int:
    entry = node.entry or node.forwarded_entry
    if entry is None:
        return 999999
    return blocker_priority(entry, lane=lane)


def node_to_dict(node: Node, *, lane: str = FUNCTIONAL_LANE) -> dict[str, object]:
    return {
        "address": node.address,
        "name": node.name,
        "kind": node.kind,
        "status": node.status_summary(lane=lane),
        "forwarded_to": node.forwarded_to,
        "blocks_source": node.blocks_source(lane=lane),
        "source_block_reason": node.source_block_reason(lane=lane),
        "blocks_verification": node.blocks_verification(lane=lane),
        "blocks_binary_verification": node.blocks_binary_verification(lane=lane),
        "verification_block_reason": node.verification_block_reason(lane=lane),
        "callees": node.callees,
        "function_refs": node.function_refs,
        "unresolved_calls": node.unresolved_calls,
        "indirect_calls": node.indirect_calls,
        "bridge_error": node.bridge_error,
    }


def functional_coverage_by_address(manifest_dir: Path) -> dict[str, list[str]]:
    coverage: dict[str, list[str]] = {}
    for manifest_path in sorted(manifest_dir.glob("*.json")):
        try:
            manifest = load_functional_manifest(manifest_path)
        except FileNotFoundError:
            continue
        for address in manifest.covered_addresses:
            coverage.setdefault(address, []).append(manifest.name)
    return coverage


def vc5_coverage_by_address(manifest_dir: Path) -> dict[str, list[str]]:
    coverage: dict[str, list[str]] = {}
    for manifest_path in sorted(manifest_dir.glob("*.json")):
        try:
            manifest = load_vc5_manifest(manifest_path, enforce_source_policy=False)
        except FileNotFoundError:
            continue
        for function in manifest.functions:
            coverage.setdefault(function.address, []).append(manifest.name)
    return coverage


def verification_coverage_by_address(
    functional_manifest_dir: Path,
    vc5_manifest_dir: Path | None = None,
) -> dict[str, list[str]]:
    coverage = functional_coverage_by_address(functional_manifest_dir)
    if vc5_manifest_dir is None:
        return coverage
    for address, target_names in vc5_coverage_by_address(vc5_manifest_dir).items():
        merged = coverage.setdefault(address, [])
        for target_name in target_names:
            if target_name not in merged:
                merged.append(target_name)
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


def extract_function_reference_tokens(assembly: str) -> list[str]:
    tokens: list[str] = []
    for line in assembly.splitlines():
        code = line.split(";", 1)[0]
        if "\tcall" in code or re.search(r"\bcall\s", code):
            continue
        for match in IMMEDIATE_ADDR_RE.finditer(code):
            tokens.append(match.group(0))
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


def is_provider_source_ready(entry: OwnerEntry) -> bool:
    return entry.is_provider_ready


def owner_entries_by_name(entries: dict[str, OwnerEntry]) -> dict[str, OwnerEntry]:
    entries_by_name: dict[str, OwnerEntry] = {}
    for entry in entries.values():
        for name in (entry.reconstructed_name, entry.reimplemented_name, entry.provider_name):
            if name and name != "pending":
                entries_by_name.setdefault(name, entry)
    return entries_by_name


def forwarded_entry_for_symbol(symbol: Symbol | None, entries_by_name: dict[str, OwnerEntry]) -> OwnerEntry | None:
    if symbol is None or not symbol.name.endswith("_Forwarder"):
        return None
    base_name = symbol.name[: -len("_Forwarder")]
    return entries_by_name.get(base_name)


def load_node(
    address: str,
    bridge: BinaryNinjaBridge,
    entries: dict[str, OwnerEntry],
    symbols_by_address: dict[str, Symbol],
    symbols_by_name: dict[str, Symbol],
    entries_by_name: dict[str, OwnerEntry],
) -> Node:
    address = normalize_address(address)
    symbol = symbols_by_address.get(address)
    entry = entries.get(address)
    node = Node(
        address=address,
        name=(symbol.name if symbol else (entry.reconstructed_name if entry else "")),
        kind=inferred_symbol_kind(address, symbol),
        entry=entry,
    )
    if entry is None:
        forwarded = forwarded_entry_for_symbol(symbol, entries_by_name)
        if forwarded is not None:
            node.kind = "forwarder"
            node.forwarded_to = forwarded.address
            node.forwarded_entry = forwarded
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

    seen_refs: set[str] = set()
    for token in extract_function_reference_tokens(assembly):
        resolved = resolve_call(token, symbols_by_name)
        symbol = symbols_by_address.get(resolved) if resolved else None
        if (
            resolved
            and symbol is not None
            and resolved != address
            and resolved not in seen
            and resolved not in seen_refs
            and inferred_symbol_kind(resolved, symbol) == "function"
        ):
            seen_refs.add(resolved)
            node.function_refs.append(resolved)
    return node


def load_metadata_node(
    address: str,
    entries: dict[str, OwnerEntry],
    symbols_by_address: dict[str, Symbol],
    entries_by_name: dict[str, OwnerEntry],
) -> Node:
    address = normalize_address(address)
    symbol = symbols_by_address.get(address)
    entry = entries.get(address)
    node = Node(
        address=address,
        name=(symbol.name if symbol else (entry.reconstructed_name if entry else "")),
        kind=inferred_symbol_kind(address, symbol),
        entry=entry,
    )
    if entry is None:
        forwarded = forwarded_entry_for_symbol(symbol, entries_by_name)
        if forwarded is not None:
            node.kind = "forwarder"
            node.forwarded_to = forwarded.address
            node.forwarded_entry = forwarded
    return node


def build_frontier(
    address: str,
    depth: int,
    bridge: BinaryNinjaBridge,
    entries: dict[str, OwnerEntry],
) -> dict[str, Node]:
    symbols_by_address, symbols_by_name = bridge.symbols()
    entries_by_name = owner_entries_by_name(entries)
    root = normalize_address(address)
    nodes: dict[str, Node] = {}
    queue: deque[tuple[str, int]] = deque([(root, 0)])
    while queue:
        current, level = queue.popleft()
        if current in nodes:
            continue
        if level >= depth:
            node = load_metadata_node(current, entries, symbols_by_address, entries_by_name)
        else:
            node = load_node(
                current,
                bridge,
                entries,
                symbols_by_address,
                symbols_by_name,
                entries_by_name,
            )
        nodes[current] = node
        if level >= depth:
            continue
        for dependency in [*node.callees, *node.function_refs]:
            if dependency not in nodes:
                queue.append((dependency, level + 1))
    return nodes


def frontier_truncated(nodes: dict[str, Node]) -> bool:
    return any(is_budget_error_text(node.bridge_error) for node in nodes.values())


def is_budget_error_text(text: str) -> bool:
    return "bridge call budget exhausted" in text.lower()


def recommendation(root: str, nodes: dict[str, Node], *, lane: str = FUNCTIONAL_LANE) -> str:
    lane = normalize_lane(lane)
    root_node = nodes[root]
    dependencies = [*root_node.callees, *root_node.function_refs]
    source_blocking_dependencies = [
        node
        for dependency in dependencies
        if (node := nodes.get(dependency)) is not None
        and node.blocks_source(lane=lane)
    ]
    for node in sorted(
        source_blocking_dependencies,
        key=lambda item: (_node_blocker_priority(item, lane=lane), dependencies.index(item.address)),
    ):
        return (
            f"{node.address} {node.name} "
            f"(lowest visible blocking callee: {node.source_block_reason(lane=lane)})"
        )
    if root_node.blocks_source(lane=lane):
        if root_node.entry and not root_node.entry.is_source_dependencies_satisfied:
            return (
                f"{root_node.address} {root_node.name} "
                "(audit/update source dependencies before implementation)"
            )
        reason = root_node.source_block_reason(lane=lane)
        if reason:
            return (
                f"{root_node.address} {root_node.name} "
                f"({reason}; no lower blocking callee visible at this depth)"
            )
        return (
            f"{root_node.address} {root_node.name} "
            "(no lower blocking callee visible at this depth)"
        )
    lane_blocking_dependencies = [
        node
        for dependency in dependencies
        if (node := nodes.get(dependency)) is not None
        and node.blocks_verification(lane=lane)
    ]
    for node in sorted(
        lane_blocking_dependencies,
        key=lambda item: (_node_blocker_priority(item, lane=lane), dependencies.index(item.address)),
    ):
        return f"{node.address} {node.name} (source done; {_verification_block_label(node, lane=lane)})"
    if root_node.blocks_verification(lane=lane):
        return f"{root_node.address} {root_node.name} (source done; {_anchor_verification_label(root_node, lane=lane)})"
    if frontier_truncated(nodes):
        return "No blocking dependency visible before Binary Ninja call budget was exhausted."
    return "No blocking dependency visible at requested depth."


def print_report(
    root: str,
    nodes: dict[str, Node],
    *,
    verification_coverage: dict[str, list[str]] | None = None,
    lane: str = FUNCTIONAL_LANE,
    include_title: bool = True,
) -> None:
    lane = normalize_lane(lane)
    verification_coverage = verification_coverage or {}
    root_node = nodes[root]
    if include_title:
        print("# Recoil Dependency Frontier")
        print()
    print(f"Anchor: `{root_node.address}` `{root_node.name}`")
    if root_node.entry:
        print(f"Group: {root_node.entry.group_title}")
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
            provider_kind = (
                node.entry.provider_kind or node.entry.provider
                if node.entry
                else ""
            )
            provider = f" provider={provider_kind}" if provider_kind else ""
            error = f" bridge_error={node.bridge_error!r}" if node.bridge_error else ""
            print(f"- `{node.address}` `{node.name}` {node.status_summary(lane=lane)}{provider}{error}")
        else:
            print(f"- `{callee}` not expanded")
    if root_node.function_refs:
        print()
        print("## Function References")
        for function_ref in root_node.function_refs:
            node = nodes.get(function_ref)
            if node:
                provider_kind = (
                    node.entry.provider_kind or node.entry.provider
                    if node.entry
                    else ""
                )
                provider = f" provider={provider_kind}" if provider_kind else ""
                error = f" bridge_error={node.bridge_error!r}" if node.bridge_error else ""
                print(
                    f"- `{node.address}` `{node.name}` {node.status_summary(lane=lane)}"
                    f"{provider}{error}"
                )
            else:
                print(f"- `{function_ref}` not expanded")
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
    reported = [root_node.address, *root_node.callees, *root_node.function_refs]
    for address in reported:
        node = nodes.get(address)
        name = node.name if node else ""
        targets = verification_coverage.get(address, [])
        target_text = ", ".join(targets) if targets else "none"
        print(f"- `{address}` `{name}` targets={target_text}")
    print()
    print("## Blocking Dependencies")
    source_blockers = [
        node for node in nodes.values() if node.address != root and node.blocks_source(lane=lane)
    ]
    lane_blockers = [
        node
        for node in nodes.values()
        if node.address != root
        and not node.blocks_source(lane=lane)
        and node.blocks_verification(lane=lane)
    ]
    lane_blockers = sorted(
        lane_blockers,
        key=lambda node: (_node_blocker_priority(node, lane=lane), node.address),
    )
    if not source_blockers and not lane_blockers:
        print("- no source blockers visible at requested depth")
    for node in source_blockers:
        print(
            f"- `{node.address}` `{node.name}` {node.status_summary(lane=lane)} "
            f"reason={node.source_block_reason(lane=lane)}"
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
        "200-call Binary Ninja budget. Inspect vtable calls, indirect calls, globals, "
        "and shared types in BN before editing source."
    )


def report_to_dict(
    root: str,
    nodes: dict[str, Node],
    *,
    verification_coverage: dict[str, list[str]] | None = None,
    lane: str = FUNCTIONAL_LANE,
) -> dict[str, object]:
    lane = normalize_lane(lane)
    verification_coverage = verification_coverage or {}
    root_node = nodes[root]
    coverage = {
        address: verification_coverage.get(address, [])
        for address in [root, *root_node.callees, *root_node.function_refs]
    }
    return {
        "lane": lane,
        "truncated_due_to_bn_call_budget": frontier_truncated(nodes),
        "anchor": node_to_dict(root_node, lane=lane),
        "direct_callees": [
            node_to_dict(nodes[callee], lane=lane) for callee in root_node.callees if callee in nodes
        ],
        "function_refs": [
            node_to_dict(nodes[function_ref], lane=lane)
            for function_ref in root_node.function_refs
            if function_ref in nodes
        ],
        "verification_coverage": coverage,
        "vc5_coverage": coverage,
        "blocking_dependencies": [
                node_to_dict(node, lane=lane)
            for node in nodes.values()
            if node.address != root and node.blocks_source(lane=lane)
        ],
        "lane_blocking_dependencies": [
            node_to_dict(node, lane=lane)
            for node in nodes.values()
            if node.address != root
            and not node.blocks_source(lane=lane)
            and node.blocks_verification(lane=lane)
        ],
        "recommended_next": recommendation(root, nodes, lane=lane),
    }


def load_frontier_addresses(args: argparse.Namespace) -> list[str]:
    addresses = [normalize_address(address) for address in args.addresses]
    seen: set[str] = set()
    for address in addresses:
        if address in seen:
            raise ValueError(f"frontier received duplicate address: {address}")
        seen.add(address)
    return addresses


def main(argv: list[str] | None = None) -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8")

    parser = argparse.ArgumentParser(
        description="Report the dependency frontier for a Recoil function anchor."
    )
    parser.add_argument("addresses", nargs="+", help="Anchor function address or addresses, e.g. 0x4301e0")
    parser.add_argument(
        "--depth",
        type=int,
        default=1,
        help="Direct-call traversal depth. Default: 1",
    )
    parser.add_argument("--binary", choices=reference_image_keys(), default="recoil")
    parser.add_argument("--progress", default=None, help="Unified reconstruction progress path.")
    parser.add_argument(
        "--bridge-url",
        default=DEFAULT_BRIDGE_URL,
        help="Binary Ninja bridge URL",
    )
    parser.add_argument("--functional-manifest-dir", default=str(DEFAULT_FUNCTIONAL_MANIFEST_DIR))
    parser.add_argument("--vc5-manifest-dir", default=str(DEFAULT_VC5_MANIFEST_DIR))
    parser.add_argument("--json", action="store_true", help="Emit a machine-readable frontier report.")
    parser.add_argument(
        "--lane",
        choices=LANE_CHOICES,
        default=FUNCTIONAL_LANE,
        help="functional requires tier C; binary prioritizes source-owner before implementation and data before tier S verification.",
    )
    args = parser.parse_args(argv)

    owners_path = Path(resolve_owner_ledger_path(args.binary, args.progress))
    if args.progress is not None and not owners_path.exists():
        print(f"Owners file not found: {owners_path}", file=sys.stderr)
        return 2
    try:
        entries = load_owner_entries(owners_path, binary=args.binary)
        roots = load_frontier_addresses(args)
        image = reference_image(args.binary)
        bridge = BinaryNinjaBridge(args.bridge_url, binary=Path(image.bndb_path).name)
        coverage = verification_coverage_by_address(
            Path(args.functional_manifest_dir),
            Path(args.vc5_manifest_dir),
        )
        lane = normalize_lane(args.lane)
        reports: list[dict[str, object]] = []
        multi_entry = len(roots) > 1
        if multi_entry and not args.json:
            print("# Recoil Dependency Frontier Batch")
            print()
            print(f"Entries: {len(roots)}")
        for index, root in enumerate(roots, start=1):
            nodes = build_frontier(root, args.depth, bridge, entries)
            if args.json:
                reports.append(report_to_dict(root, nodes, verification_coverage=coverage, lane=lane))
                continue
            if multi_entry:
                print()
                print(f"## Entry {index}/{len(roots)}: {root}")
                print()
            print_report(
                root,
                nodes,
                verification_coverage=coverage,
                lane=lane,
                include_title=not multi_entry,
            )
        if args.json:
            payload: dict[str, object] | list[dict[str, object]]
            payload = reports[0] if len(reports) == 1 else {"frontiers": reports}
            print(
                json.dumps(
                    payload,
                    indent=2,
                    ensure_ascii=False,
                )
            )
    except (BridgeError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
