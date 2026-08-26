from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from _recoil.lib.binja import Symbol
from _recoil.lib.owner_entries import ACCEPTED_STATUSES, LIMITED_STATUS, OwnerEntry, normalize_address


ORIGINAL_IMAGE_START = 0x00400000
ORIGINAL_IMAGE_END = 0x004FFFFF
MESSAGES_IMAGE_START = 0x10000000
MESSAGES_IMAGE_END = 0x100FFFFF
PROVIDER_SYMBOL_KINDS = {"import", "external"}
IMPLICIT_COMPILER_HELPER_EXACT_NAMES = {"__alloca_probe"}
IMPLICIT_COMPILER_HELPER_PREFIXES = (
    "MSVC_EH_ArrayConstructor",
    "MSVC_EH_ArrayDestructor",
)
GENERIC_ORIGIN_TEXTS = {
    "compiler-generated",
    "compiler generated",
    "external",
    "pending",
}


@dataclass(frozen=True)
class ProviderClosureFinding:
    severity: str
    address: str
    message: str
    closure_kind: str
    symbol_name: str = ""
    owner_entry_evidence: dict[str, Any] | None = None
    bn_evidence: dict[str, Any] | None = None
    required_action: str = ""

    def as_dict(self) -> dict[str, Any]:
        payload: dict[str, Any] = {
            "severity": self.severity,
            "address": self.address,
            "closure_kind": self.closure_kind,
            "message": self.message,
        }
        if self.symbol_name:
            payload["symbol_name"] = self.symbol_name
        if self.owner_entry_evidence is not None:
            payload["owner_entry_evidence"] = self.owner_entry_evidence
        if self.bn_evidence is not None:
            payload["bn_evidence"] = self.bn_evidence
        if self.required_action:
            payload["required_action"] = self.required_action
        return payload


def symbol_name(symbol: Symbol | None, fallback: str = "") -> str:
    if symbol is None:
        return fallback
    for value in (symbol.name, symbol.raw_name, symbol.full_name, fallback):
        if value:
            return value
    return ""


def is_in_known_image(address: str) -> bool:
    value = int(normalize_address(address), 16)
    return ORIGINAL_IMAGE_START <= value <= ORIGINAL_IMAGE_END or MESSAGES_IMAGE_START <= value <= MESSAGES_IMAGE_END


def is_implicit_compiler_helper_name(name: str) -> bool:
    text = name.strip()
    if text in IMPLICIT_COMPILER_HELPER_EXACT_NAMES:
        return True
    return any(text.startswith(prefix) for prefix in IMPLICIT_COMPILER_HELPER_PREFIXES)


def provider_owner_entry_evidence(entry: OwnerEntry) -> dict[str, Any]:
    return {
        "present": True,
        "address": entry.address,
        "reconstructed_status": entry.reconstructed_status,
        "provider_boundary_status": entry.provider_boundary_status,
        "provider_kind": entry.provider_kind or entry.provider,
        "provider_name": entry.provider_name,
        "provider_origin": entry.provider_origin,
        "provider_file": entry.provider_file,
        "provider_target": entry.provider_target,
        "group": entry.entry_group or entry.group_id,
    }


def symbol_evidence(symbol: Symbol | None, *, name: str = "", kind: str = "") -> dict[str, Any]:
    if symbol is None:
        return {
            "present": bool(name or kind),
            "name": name,
            "kind": kind,
        }
    return {
        "present": True,
        "address": symbol.address,
        "name": symbol.name,
        "raw_name": symbol.raw_name,
        "full_name": symbol.full_name,
        "kind": symbol.kind,
    }


def audit_provider_owner_entry(entry: OwnerEntry) -> list[ProviderClosureFinding]:
    findings: list[ProviderClosureFinding] = []
    if not entry.is_provider_boundary:
        return findings

    evidence = provider_owner_entry_evidence(entry)
    if entry.provider_boundary_status not in ACCEPTED_STATUSES:
        findings.append(
            ProviderClosureFinding(
                severity="error",
                address=entry.address,
                closure_kind="provider-owner-entry",
                message=f"{entry.address}: provider-boundary row is not accepted",
                owner_entry_evidence=evidence,
                required_action="Accept the provider boundary with current evidence or reclassify the row.",
            )
        )
        return findings

    if entry.reconstructed_status not in ACCEPTED_STATUSES:
        findings.append(
            ProviderClosureFinding(
                severity="error",
                address=entry.address,
                closure_kind="provider-owner-entry",
                message=f"{entry.address}: provider-boundary row lacks accepted Reconstructed marker",
                owner_entry_evidence=evidence,
                required_action="Reconstruct the provider row before using it for dependency closure.",
            )
        )
    if (entry.provider_file or "").strip() != "external":
        findings.append(
            ProviderClosureFinding(
                severity="error",
                address=entry.address,
                closure_kind="provider-owner-entry",
                message=f"{entry.address}: accepted provider-boundary row must use File: external",
                owner_entry_evidence=evidence,
                required_action="Repair provider metadata or reclassify the row as authored.",
            )
        )
    missing = [
        label
        for label, value in (
            ("provider kind", entry.provider_kind or entry.provider),
            ("provider name", entry.provider_name),
            ("provider origin", entry.provider_origin),
        )
        if not value or value == "pending"
    ]
    if missing:
        findings.append(
            ProviderClosureFinding(
                severity="error",
                address=entry.address,
                closure_kind="provider-owner-entry",
                message=f"{entry.address}: accepted provider-boundary row is missing {', '.join(missing)}",
                owner_entry_evidence=evidence,
                required_action="Fill provider metadata from BN/provider evidence or reclassify the row.",
            )
        )
    if entry.provider_boundary_status == LIMITED_STATUS or entry.reconstructed_status == LIMITED_STATUS:
        findings.append(
            ProviderClosureFinding(
                severity="warning",
                address=entry.address,
                closure_kind="provider-owner-entry",
                message=f"{entry.address}: provider closure is limited-accepted",
                owner_entry_evidence=evidence,
                required_action="Preserve the limitation unless current evidence can remove it.",
            )
        )
    if not entry.provider_target or entry.provider_target == "pending":
        findings.append(
            ProviderClosureFinding(
                severity="warning",
                address=entry.address,
                closure_kind="provider-owner-entry",
                message=f"{entry.address}: provider target is pending",
                owner_entry_evidence=evidence,
                required_action="Record the owning provider target when current evidence identifies it.",
            )
        )
    if (entry.provider_origin or "").strip().lower() in GENERIC_ORIGIN_TEXTS:
        findings.append(
            ProviderClosureFinding(
                severity="warning",
                address=entry.address,
                closure_kind="provider-owner-entry",
                message=f"{entry.address}: provider origin is generic",
                owner_entry_evidence=evidence,
                required_action="Replace generic origin text with concrete BN/provider evidence when available.",
            )
        )
    if not findings:
        findings.append(
            ProviderClosureFinding(
                severity="info",
                address=entry.address,
                closure_kind="provider-owner-entry",
                message=f"{entry.address}: provider-boundary row is explicit and accepted",
                owner_entry_evidence=evidence,
            )
        )
    return findings


def classify_untracked_symbol(
    *,
    address: str,
    symbol: Symbol | None = None,
    name: str = "",
    kind: str = "",
) -> ProviderClosureFinding | None:
    address = normalize_address(address)
    resolved_name = symbol_name(symbol, name)
    resolved_kind = symbol.kind if symbol is not None else kind
    evidence = symbol_evidence(symbol, name=resolved_name, kind=resolved_kind)
    if resolved_kind in PROVIDER_SYMBOL_KINDS or not is_in_known_image(address):
        return ProviderClosureFinding(
            severity="info",
            address=address,
            symbol_name=resolved_name,
            closure_kind="bn-provider-symbol",
            message=f"{address}: untracked BN {resolved_kind or 'external'} symbol closes as provider/runtime",
            bn_evidence=evidence,
        )
    if is_implicit_compiler_helper_name(resolved_name):
        return ProviderClosureFinding(
            severity="info",
            address=address,
            symbol_name=resolved_name,
            closure_kind="implicit-compiler-helper",
            message=f"{address}: narrow implicit compiler helper closes without an owner entry",
            bn_evidence=evidence,
        )
    if resolved_kind == "compiler" or resolved_name.startswith("MSVC_") or resolved_name.startswith("__"):
        return ProviderClosureFinding(
            severity="error",
            address=address,
            symbol_name=resolved_name,
            closure_kind="implicit-compiler-helper",
            message=f"{address}: generated-looking in-image symbol is not in the implicit compiler-helper allowlist",
            bn_evidence=evidence,
            required_action="Add an explicit provider-boundary row with evidence or reclassify as authored.",
        )
    return None
