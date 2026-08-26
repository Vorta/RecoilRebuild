#!/usr/bin/env python3
"""Audit provider/compiler-generated dependency closure."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import sys
from typing import Any

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from _recoil.lib.bn_dependency_frontier import Node, build_frontier
from _recoil.lib.binja import DEFAULT_BRIDGE_URL, BinaryNinjaBridge, BridgeError
from _recoil.lib.owner_entries import OwnerEntryIndex, OwnerEntry, normalize_address
from _recoil.lib.provider_closure import (
    ProviderClosureFinding,
    audit_provider_owner_entry,
    classify_untracked_symbol,
)
from _recoil.lib.reference_images import reference_image, reference_image_keys, resolve_owner_ledger_path
from _recoil.lib.tooling import configure_stdio


@dataclass(frozen=True)
class ProviderClosureAuditResult:
    binary: str
    mode: str
    checked_addresses: int
    checked_provider_rows: int
    checked_dependencies: int
    findings: tuple[ProviderClosureFinding, ...]

    @property
    def error_count(self) -> int:
        return sum(1 for finding in self.findings if finding.severity == "error")

    @property
    def warning_count(self) -> int:
        return sum(1 for finding in self.findings if finding.severity == "warning")

    @property
    def incomplete_count(self) -> int:
        return sum(1 for finding in self.findings if finding.severity == "incomplete")

    @property
    def strict_failure(self) -> bool:
        return bool(self.error_count or self.incomplete_count)

    def as_dict(self) -> dict[str, Any]:
        return {
            "binary": self.binary,
            "mode": self.mode,
            "checked_addresses": self.checked_addresses,
            "checked_provider_rows": self.checked_provider_rows,
            "checked_dependencies": self.checked_dependencies,
            "error_count": self.error_count,
            "warning_count": self.warning_count,
            "incomplete_count": self.incomplete_count,
            "findings": [finding.as_dict() for finding in self.findings],
        }


def audit_provider_owner_entries(*, binary: str, entries: dict[str, OwnerEntry]) -> ProviderClosureAuditResult:
    provider_rows = [entry for entry in entries.values() if entry.is_provider_boundary]
    findings: list[ProviderClosureFinding] = []
    for entry in provider_rows:
        findings.extend(audit_provider_owner_entry(entry))
    return ProviderClosureAuditResult(
        binary=binary,
        mode="owners-only",
        checked_addresses=0,
        checked_provider_rows=len(provider_rows),
        checked_dependencies=0,
        findings=tuple(findings),
    )


def incomplete_finding(address: str, message: str) -> ProviderClosureFinding:
    return ProviderClosureFinding(
        severity="incomplete",
        address=normalize_address(address),
        closure_kind="bn-frontier",
        message=message,
        required_action="Rerun with Binary Ninja available and within the bridge call budget.",
    )


def audit_node(node: Node) -> list[ProviderClosureFinding]:
    findings: list[ProviderClosureFinding] = []
    if node.bridge_error:
        return [incomplete_finding(node.address, f"{node.address}: BN evidence unavailable: {node.bridge_error}")]
    if node.entry is not None:
        return audit_provider_owner_entry(node.entry)
    if node.forwarded_entry is not None:
        if node.forwarded_entry.is_provider_boundary:
            findings.append(
                ProviderClosureFinding(
                    severity="info",
                    address=node.address,
                    symbol_name=node.name,
                    closure_kind="forwarder-target",
                    message=f"{node.address}: forwarder closure is delegated to provider target {node.forwarded_to}",
                    bn_evidence={
                        "name": node.name,
                        "kind": node.kind,
                        "forwarded_to": node.forwarded_to,
                    },
                )
            )
            findings.extend(audit_provider_owner_entry(node.forwarded_entry))
        else:
            findings.append(
                ProviderClosureFinding(
                    severity="info",
                    address=node.address,
                    symbol_name=node.name,
                    closure_kind="forwarder-target",
                    message=f"{node.address}: forwarder target is authored, not provider closure",
                    bn_evidence={
                        "name": node.name,
                        "kind": node.kind,
                        "forwarded_to": node.forwarded_to,
                    },
                )
            )
        return findings
    if node.name.endswith("_Forwarder"):
        findings.append(
            ProviderClosureFinding(
                severity="error",
                address=node.address,
                symbol_name=node.name,
                closure_kind="forwarder-unclassified",
                message=f"{node.address}: untracked forwarder needs explicit target classification",
                bn_evidence={"name": node.name, "kind": node.kind},
                required_action="Add an explicit provider-boundary row, link it to an authored target, or reclassify it.",
            )
        )
        return findings
    finding = classify_untracked_symbol(address=node.address, name=node.name, kind=node.kind)
    return [finding] if finding is not None else []


def audit_frontier_addresses(
    *,
    binary: str,
    entries: dict[str, OwnerEntry],
    bridge: BinaryNinjaBridge,
    addresses: list[str],
    depth: int,
) -> ProviderClosureAuditResult:
    findings: list[ProviderClosureFinding] = []
    seen_nodes: set[str] = set()
    roots = [normalize_address(address) for address in addresses]
    provider_rows_seen: set[str] = set()
    for root in roots:
        try:
            nodes = build_frontier(root, depth, bridge, entries)
        except (BridgeError, ValueError) as exc:
            findings.append(incomplete_finding(root, f"{root}: provider-closure frontier unavailable: {exc}"))
            continue
        for address, node in nodes.items():
            if address in seen_nodes:
                continue
            seen_nodes.add(address)
            node_findings = audit_node(node)
            findings.extend(node_findings)
            if node.entry is not None and node.entry.is_provider_boundary:
                provider_rows_seen.add(node.entry.address)
            if node.forwarded_entry is not None and node.forwarded_entry.is_provider_boundary:
                provider_rows_seen.add(node.forwarded_entry.address)
    return ProviderClosureAuditResult(
        binary=binary,
        mode="frontier",
        checked_addresses=len(roots),
        checked_provider_rows=len(provider_rows_seen),
        checked_dependencies=max(0, len(seen_nodes) - len(set(roots))),
        findings=tuple(findings),
    )


def print_human(result: ProviderClosureAuditResult, *, limit: int) -> None:
    print(
        "provider_closure_audit "
        f"binary={result.binary} mode={result.mode} "
        f"addresses={result.checked_addresses} "
        f"provider_rows={result.checked_provider_rows} "
        f"dependencies={result.checked_dependencies} "
        f"errors={result.error_count} warnings={result.warning_count} "
        f"incomplete={result.incomplete_count}"
    )
    severity_order = {"error": 0, "incomplete": 1, "warning": 2, "info": 3}
    reportable = sorted(
        [finding for finding in result.findings if finding.severity != "info"],
        key=lambda finding: (severity_order.get(finding.severity, 99), finding.address, finding.message),
    )
    if not reportable:
        print("- no provider-closure errors, warnings, or incomplete checks")
        return
    selected = reportable[:limit] if limit >= 0 else reportable
    for finding in selected:
        symbol = f" symbol={finding.symbol_name}" if finding.symbol_name else ""
        action = f" action={finding.required_action}" if finding.required_action else ""
        print(
            f"- {finding.severity}: {finding.address} "
            f"closure={finding.closure_kind}{symbol}: {finding.message}{action}"
        )
    if limit >= 0 and len(reportable) > limit:
        print(f"- ... {len(reportable) - limit} more")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Audit provider/compiler-generated dependency closure. The command is read-only: "
            "it reports explicit provider-boundary metadata issues, narrow implicit compiler-helper "
            "closure, and unclassified generated-looking BN symbols without changing owners, BN, "
            "source, or ledgers."
        )
    )
    parser.add_argument("addresses", nargs="*", metavar="ADDRESS", help="Address roots for BN frontier mode.")
    parser.add_argument("--binary", choices=reference_image_keys(), default="recoil")
    parser.add_argument("--progress", help="unified reconstruction progress path")
    parser.add_argument(
        "--owners-only",
        action="store_true",
        help="Scan provider entries from unified progress without Binary Ninja.",
    )
    parser.add_argument("--bridge-url", default=DEFAULT_BRIDGE_URL)
    parser.add_argument("--depth", type=int, default=1, help="BN frontier depth for address mode.")
    parser.add_argument("--json", action="store_true", help="Emit structured findings.")
    parser.add_argument("--strict", action="store_true", help="Return nonzero for errors or incomplete checks.")
    parser.add_argument("--limit", type=int, default=40, help="Maximum non-info findings to print in text mode.")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        if args.owners_only and args.addresses:
            raise ValueError("audit provider-closure accepts either --owners-only or addresses, not both")
        if not args.owners_only and not args.addresses:
            raise ValueError("audit provider-closure requires --owners-only or at least one address")
        if args.depth < 0:
            raise ValueError("--depth must be non-negative")
        image = reference_image(args.binary)
        owners_path = Path(args.progress or resolve_owner_ledger_path(args.binary))
        entries = OwnerEntryIndex.load(owners_path, binary=args.binary).entries
        if args.owners_only:
            result = audit_provider_owner_entries(binary=args.binary, entries=entries)
        else:
            bridge = BinaryNinjaBridge(args.bridge_url, binary=Path(image.bndb_path).name)
            result = audit_frontier_addresses(
                binary=args.binary,
                entries=entries,
                bridge=bridge,
                addresses=args.addresses,
                depth=args.depth,
            )
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    if args.json:
        print(json.dumps(result.as_dict(), indent=2, sort_keys=True))
    else:
        print_human(result, limit=args.limit)
    return 2 if args.strict and result.strict_failure else 0


if __name__ == "__main__":
    raise SystemExit(main())
