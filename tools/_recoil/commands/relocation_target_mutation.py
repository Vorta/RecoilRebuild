from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path, PurePosixPath
import sys
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    CoffObject,
    IMAGE_SCN_CNT_CODE,
    IMAGE_SYM_CLASS_EXTERNAL,
    relocation_size,
)
from _recoil.commands.relocation_expectations import (
    DEFAULT_REFERENCE,
    RelocationExpectationError,
    build_object_binding_snapshot,
    decode_retail_relocation_at_offset,
    normalize_relocation_target_binding,
    relocation_target_binding_staleness,
    relocation_target_owner_context,
    relocation_target_row_context,
)
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.progress import (
    ConcurrentProgressUpdate,
    DEFAULT_PROGRESS_PATH,
    ProgressDocument,
    ProgressError,
    ProgressStore,
    address_value,
    normalize_address,
    state_record,
)
from _recoil.lib.tooling import (
    DEFAULT_VC5_ROOT,
    REPO_ROOT,
    configure_stdio,
    display_path,
)
from _recoil.lib.verification_targets import vc5_target_registration
from _recoil.commands.vc5_verify import load_manifest as load_vc5_manifest


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "vc5_verify_targets"


class RelocationTargetMutationError(RuntimeError):
    pass


def _payload(value: str) -> dict[str, Any]:
    try:
        parsed = json.loads(value)
    except json.JSONDecodeError as exc:
        raise RelocationTargetMutationError(f"invalid --payload-json: {exc}") from exc
    if not isinstance(parsed, dict):
        raise RelocationTargetMutationError("--payload-json must decode to an object")
    return parsed


def _normalize_correction_selector(value: Any) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        raise RelocationTargetMutationError("correction must be an object")
    allowed = {
        "prior_target_symbol_id",
        "prior_target_object_symbol",
        "source_symbol_id",
        "source_address",
        "source_object_symbol",
        "offset",
        "refresh_only",
    }
    keys = {str(key) for key in value}
    unknown = sorted(keys - allowed)
    if unknown:
        raise RelocationTargetMutationError(
            f"correction selector has unsupported fields: {unknown}"
        )
    required = allowed - {"refresh_only"}
    missing = sorted(required - keys)
    if missing:
        raise RelocationTargetMutationError(
            "correction requires exact prior identity/site selection; missing fields: "
            f"{missing}"
        )
    result: dict[str, Any] = {}
    for field in (
        "prior_target_symbol_id",
        "prior_target_object_symbol",
        "source_symbol_id",
        "source_object_symbol",
    ):
        item = value.get(field)
        if not isinstance(item, str) or not item.strip():
            raise RelocationTargetMutationError(
                f"correction selector {field} must be non-empty"
            )
        result[field] = item.strip()
    source_address = value.get("source_address")
    if not isinstance(source_address, str):
        raise RelocationTargetMutationError(
            "correction selector source_address must be an address string"
        )
    result["source_address"] = normalize_address(source_address)
    offset = value.get("offset")
    if not isinstance(offset, int) or isinstance(offset, bool) or offset < 0:
        raise RelocationTargetMutationError(
            "correction selector offset must be a non-negative JSON integer"
        )
    result["offset"] = offset
    if "refresh_only" in value:
        if value.get("refresh_only") is not True:
            raise RelocationTargetMutationError(
                "correction selector refresh_only must be true when present"
            )
        result["refresh_only"] = True
    return result


def _normalize_provider_object_proof(value: Any) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        raise RelocationTargetMutationError("provider_object_proof must be an object")
    keys = {str(key) for key in value}
    candidate_fields = sorted(key for key in keys if "candidate" in key.casefold())
    if candidate_fields:
        raise RelocationTargetMutationError(
            "candidate-derived provider object proof fields are forbidden: "
            f"{candidate_fields}"
        )
    unknown = sorted(keys - {"object_path"})
    if unknown:
        raise RelocationTargetMutationError(
            f"provider_object_proof has unsupported fields: {unknown}"
        )
    object_path = value.get("object_path")
    if not isinstance(object_path, str) or not object_path:
        raise RelocationTargetMutationError(
            "provider_object_proof object_path must be a non-empty normalized "
            "VC5SP3-relative path"
        )
    if "\x00" in object_path or "\\" in object_path:
        raise RelocationTargetMutationError(
            "provider_object_proof object_path must use normalized forward-slash "
            "VC5SP3-relative form"
        )
    relative = PurePosixPath(object_path)
    if (
        relative.is_absolute()
        or object_path != relative.as_posix()
        or object_path in {".", ".."}
        or any(part in {"", ".", ".."} for part in relative.parts)
        or (relative.parts and ":" in relative.parts[0])
    ):
        raise RelocationTargetMutationError(
            "provider_object_proof object_path must be a normalized VC5SP3-relative path"
        )
    return {"object_path": relative.as_posix()}


def normalize_reviewed_target_request(value: Mapping[str, Any]) -> dict[str, Any]:
    allowed = {
        "reviewed",
        "source_object_symbol",
        "offset",
        "target_object_symbol",
        "target_owner_id",
        "reason",
        "evidence_ids",
        "create_missing_data",
        "target_end_exclusive",
        "target_name",
        "correction",
        "provider_object_proof",
    }
    keys = {str(key) for key in value}
    candidate_fields = sorted(key for key in keys if "candidate" in key.casefold())
    if candidate_fields:
        raise RelocationTargetMutationError(
            f"candidate-derived relocation target fields are forbidden: {candidate_fields}"
        )
    forbidden_target_facts = sorted(
        keys
        & {
            "source_address",
            "source_end_exclusive",
            "type",
            "target_address",
            "retail_target",
            "target_symbol_id",
            "target_size",
            "size",
            "output_section_id",
        }
    )
    if forbidden_target_facts:
        raise RelocationTargetMutationError(
            "retail-derived/source-context fields are not accepted in the reviewed payload: "
            f"{forbidden_target_facts}"
        )
    unknown = sorted(keys - allowed)
    if unknown:
        raise RelocationTargetMutationError(
            f"reviewed relocation target payload has unsupported fields: {unknown}"
        )
    if value.get("reviewed") is not True:
        raise RelocationTargetMutationError("reviewed relocation target must set reviewed=true")
    result: dict[str, Any] = {"reviewed": True}
    for field in (
        "source_object_symbol",
        "target_object_symbol",
        "target_owner_id",
        "reason",
    ):
        item = value.get(field)
        if not isinstance(item, str) or not item.strip():
            raise RelocationTargetMutationError(f"{field} must be non-empty")
        result[field] = item.strip()
    offset = value.get("offset")
    if not isinstance(offset, int) or isinstance(offset, bool) or offset < 0:
        raise RelocationTargetMutationError("offset must be a non-negative JSON integer")
    result["offset"] = offset
    evidence_ids = value.get("evidence_ids")
    if (
        not isinstance(evidence_ids, list)
        or not evidence_ids
        or any(not isinstance(item, str) or not item for item in evidence_ids)
    ):
        raise RelocationTargetMutationError("evidence_ids must be a non-empty string list")
    result["evidence_ids"] = sorted(set(evidence_ids))
    create_missing_data = value.get("create_missing_data", False)
    if not isinstance(create_missing_data, bool):
        raise RelocationTargetMutationError("create_missing_data must be boolean")
    result["create_missing_data"] = create_missing_data
    if create_missing_data:
        end_exclusive = value.get("target_end_exclusive")
        target_name = value.get("target_name")
        if not isinstance(end_exclusive, str):
            raise RelocationTargetMutationError(
                "target_end_exclusive is required when create_missing_data=true"
            )
        if not isinstance(target_name, str) or not target_name.strip():
            raise RelocationTargetMutationError(
                "target_name is required when create_missing_data=true"
            )
        result["target_end_exclusive"] = normalize_address(end_exclusive)
        result["target_name"] = target_name.strip()
    elif "target_end_exclusive" in value or "target_name" in value:
        raise RelocationTargetMutationError(
            "target_end_exclusive and target_name are valid only for missing-data creation"
        )
    if "correction" in value:
        if create_missing_data:
            raise RelocationTargetMutationError(
                "correction cannot be combined with missing-data creation"
            )
        result["correction"] = _normalize_correction_selector(value.get("correction"))
    if "provider_object_proof" in value:
        if create_missing_data:
            raise RelocationTargetMutationError(
                "provider_object_proof cannot be combined with missing-data creation"
            )
        if "correction" in value:
            raise RelocationTargetMutationError(
                "provider_object_proof cannot be combined with correction"
            )
        result["provider_object_proof"] = _normalize_provider_object_proof(
            value.get("provider_object_proof")
        )
    return result


def _source_row(
    document: ProgressDocument,
    *,
    source_symbol_id: str,
    source_address: str,
) -> Mapping[str, Any]:
    row = document.collection("symbols").get(source_symbol_id)
    if not isinstance(row, Mapping) or row.get("binary") != "recoil":
        raise RelocationTargetMutationError(
            f"unknown Recoil physical source symbol {source_symbol_id!r}"
        )
    if row.get("kind") not in {"function", "provider-function", "compiler-function"}:
        raise RelocationTargetMutationError("relocation source must be a physical function row")
    tracker_address = normalize_address(row.get("address"))
    requested_address = normalize_address(source_address)
    if tracker_address != requested_address:
        raise RelocationTargetMutationError(
            f"source address {requested_address} does not match {source_symbol_id} at {tracker_address}"
        )
    if not isinstance(row.get("end_exclusive"), str):
        raise RelocationTargetMutationError("source symbol has no known nonempty extent")
    if address_value(str(row["end_exclusive"])) <= address_value(tracker_address):
        raise RelocationTargetMutationError("source symbol has no known nonempty extent")
    return row


def _validate_owner_evidence(
    document: ProgressDocument,
    *,
    owner_id: str,
    evidence_ids: Sequence[str],
) -> Mapping[str, Any]:
    owner = document.collection("owners").get(owner_id)
    if not isinstance(owner, Mapping) or owner.get("binary") != "recoil":
        raise RelocationTargetMutationError(f"unknown Recoil target owner {owner_id!r}")
    owner_evidence = set(str(item) for item in owner.get("evidence_ids", ()))
    evidence = document.collection("evidence")
    failures: list[str] = []
    for evidence_id in evidence_ids:
        row = evidence.get(evidence_id)
        scopes = (
            set(str(item) for item in row.get("scope_ids", ()))
            if isinstance(row, Mapping)
            else set()
        )
        if evidence_id not in owner_evidence or owner_id not in scopes:
            failures.append(evidence_id)
    if failures:
        raise RelocationTargetMutationError(
            "target owner evidence is missing or not exactly owner-scoped: " + str(failures)
        )
    return owner


def _retail_section(
    document: ProgressDocument,
    *,
    reference: Path,
    start: int,
    end_exclusive: int,
) -> str:
    image = reference.read_bytes()
    headers = parse_pe_headers(image, source=str(reference))
    if end_exclusive <= start:
        raise RelocationTargetMutationError("target extent must be nonempty")
    matches = []
    for section in headers.sections:
        section_start = headers.image_base + section.virtual_address
        section_end = section_start + max(section.virtual_size, section.raw_size)
        if section_start <= start and end_exclusive <= section_end:
            matches.append(section)
    if len(matches) != 1:
        raise RelocationTargetMutationError(
            "target extent does not resolve to exactly one immutable-retail PE section"
        )
    section_id = f"recoil:section:{matches[0].name}"
    section_row = document.collection("output_sections").get(section_id)
    if not isinstance(section_row, Mapping) or section_row.get("binary") != "recoil":
        raise RelocationTargetMutationError(
            f"retail target section {section_id!r} is not registered in the tracker"
        )
    return section_id


def _target_candidates(document: ProgressDocument, retail_target: int) -> list[tuple[str, Mapping[str, Any]]]:
    result: list[tuple[str, Mapping[str, Any]]] = []
    for symbol_id, row in document.collection("symbols").items():
        if not isinstance(row, Mapping) or row.get("binary") != "recoil":
            continue
        if not isinstance(row.get("address"), str) or not isinstance(
            row.get("end_exclusive"), str
        ):
            continue
        try:
            start = address_value(str(row["address"]))
            end = address_value(str(row["end_exclusive"]))
        except ValueError:
            continue
        if start <= retail_target < end:
            result.append((str(symbol_id), row))
    return sorted(result, key=lambda item: item[0])


def _direct_existing_object_identity(row: Mapping[str, Any], object_symbol: str) -> bool:
    if row.get("navigation_name") == object_symbol or row.get("object_symbol") == object_symbol:
        return True
    provider = row.get("linked_provider_binding")
    if isinstance(provider, Mapping) and provider.get("map_symbol") == object_symbol:
        return True
    aliases = row.get("logical_aliases")
    if isinstance(aliases, Mapping) and any(
        isinstance(alias, Mapping) and alias.get("object_symbol") == object_symbol
        for alias in aliases.values()
    ):
        return True
    reviewed = row.get("relocation_target_binding")
    rows = [reviewed] if isinstance(reviewed, Mapping) else reviewed if isinstance(reviewed, list) else []
    return any(
        isinstance(item, Mapping)
        and item.get("reviewed") is True
        and item.get("object_symbol") == object_symbol
        for item in rows
    )


def _registered_manifest_path(manifest_dir: Path, value: Any) -> Path | None:
    if not isinstance(value, str) or not value:
        return None
    path = Path(value)
    if not path.is_absolute():
        path = REPO_ROOT / path
    try:
        resolved = path.resolve()
        resolved.relative_to(manifest_dir.resolve())
    except (OSError, ValueError):
        return None
    return resolved


def _manifest_exact_symbols_at_address(target: Any, row: Mapping[str, Any]) -> set[str]:
    address = normalize_address(row.get("address"))
    is_data = str(row.get("kind", "")).endswith("data") or row.get("kind") in {
        "data",
        "data-symbol",
    }
    if is_data:
        entries = list(getattr(target, "data_symbols", ()))
    else:
        entries = list(getattr(target, "functions", ()))
        entries.extend(
            function
            for order_entry in getattr(target, "translation_unit_function_order", ())
            for function in order_entry.functions
        )
        entries.extend(
            function
            for interval in getattr(target, "linked_function_intervals", ())
            for function in interval.functions
        )
    return {
        str(entry.symbol)
        for entry in entries
        if normalize_address(getattr(entry, "address", None)) == address
        and isinstance(getattr(entry, "symbol", None), str)
        and str(entry.symbol)
    }


def _synchronized_vc5_object_identity(
    document: ProgressDocument,
    *,
    row: Mapping[str, Any],
    object_symbol: str,
    manifest_dir: Path,
) -> bool:
    raw_target_ids = row.get("verification_target_ids", [])
    if not isinstance(raw_target_ids, list) or any(
        not isinstance(item, str) or not item for item in raw_target_ids
    ):
        return False
    target_ids = [str(item) for item in raw_target_ids]
    if len(target_ids) != len(set(target_ids)):
        return False

    tracker_targets = document.collection("verification_targets")
    suppliers: list[tuple[str, set[str]]] = []
    saw_vc5 = False
    row_address = normalize_address(row.get("address"))
    for target_id in target_ids:
        tracked = tracker_targets.get(target_id)
        if not isinstance(tracked, Mapping):
            return False
        if tracked.get("kind") != "vc5":
            continue
        saw_vc5 = True
        registration = tracked.get("registration")
        if not isinstance(registration, Mapping):
            return False
        manifest_path = _registered_manifest_path(
            manifest_dir, registration.get("manifest_path")
        )
        if manifest_path is None:
            return False
        try:
            current_target_id, current_target = vc5_target_registration(manifest_path)
            target = load_vc5_manifest(manifest_path, enforce_source_policy=False)
        except (OSError, ValueError):
            return False
        current_registration = current_target.get("registration")
        if (
            current_target_id != target_id
            or current_target.get("binary") != tracked.get("binary")
            or current_target.get("kind") != tracked.get("kind")
            or current_target.get("name") != tracked.get("name")
            or not isinstance(current_registration, Mapping)
            or dict(registration) != dict(current_registration)
        ):
            return False
        registered_addresses = current_target.get("registered_addresses")
        if not isinstance(registered_addresses, list) or row_address not in {
            normalize_address(item)
            for item in registered_addresses
            if isinstance(item, str)
        }:
            return False
        exact_symbols = _manifest_exact_symbols_at_address(target, row)
        if exact_symbols:
            suppliers.append((target_id, exact_symbols))

    if not saw_vc5 or not suppliers:
        return False
    return all(exact_symbols == {object_symbol} for _target_id, exact_symbols in suppliers)


def _existing_object_identity(
    document: ProgressDocument,
    *,
    row: Mapping[str, Any],
    object_symbol: str,
    manifest_dir: Path,
) -> bool:
    return _direct_existing_object_identity(
        row, object_symbol
    ) or _synchronized_vc5_object_identity(
        document,
        row=row,
        object_symbol=object_symbol,
        manifest_dir=manifest_dir,
    )


def _immutable_retail_extent_bytes(
    reference: Path,
    *,
    start: int,
    end_exclusive: int,
) -> bytes:
    if end_exclusive <= start:
        raise RelocationTargetMutationError(
            "provider object proof target extent must be known and nonempty"
        )
    image = reference.read_bytes()
    headers = parse_pe_headers(image, source=str(reference))
    if start < headers.image_base:
        raise RelocationTargetMutationError(
            "provider object proof target extent starts below the immutable retail image"
        )
    length = end_exclusive - start
    matching_sections = [
        section
        for section in headers.sections
        if headers.image_base + section.virtual_address <= start
        and end_exclusive
        <= headers.image_base + section.virtual_address + section.raw_size
    ]
    if len(matching_sections) != 1:
        raise RelocationTargetMutationError(
            "provider object proof target extent is not wholly file-backed by exactly "
            "one immutable retail section"
        )
    offset = rva_to_offset(start - headers.image_base, headers.sections)
    if offset is None or offset + length > len(image):
        raise RelocationTargetMutationError(
            "provider object proof target extent cannot be read from immutable retail"
        )
    return image[offset : offset + length]


def _resolve_provider_object_path(
    *,
    vc5_root: Path,
    relative_path: str,
) -> Path:
    try:
        resolved_root = vc5_root.resolve()
        resolved = resolved_root.joinpath(*PurePosixPath(relative_path).parts).resolve()
        resolved.relative_to(resolved_root)
    except (OSError, ValueError) as exc:
        raise RelocationTargetMutationError(
            "provider_object_proof object_path escapes DEFAULT_VC5_ROOT"
        ) from exc
    if not resolved.is_file():
        raise RelocationTargetMutationError(
            "provider_object_proof object_path does not name an existing VC5SP3 "
            f"COFF object: {relative_path}"
        )
    return resolved


def _validate_provider_object_proof(
    *,
    proof: Mapping[str, Any],
    vc5_root: Path,
    reference: Path,
    owner: Mapping[str, Any],
    owner_id: str,
    target: Mapping[str, Any],
    target_symbol_id: str,
    retail_target: int,
    object_symbol: str,
) -> dict[str, Any]:
    if owner.get("kind") != "provider-boundary":
        raise RelocationTargetMutationError(
            "provider_object_proof is valid only for a provider-boundary owner"
        )
    gates = owner.get("gates")
    if (
        owner.get("provider_state") != "accepted"
        or owner.get("lifecycle_state") != "accepted"
        or not isinstance(gates, Mapping)
        or gates.get("boundary") != "accepted"
        or gates.get("source") != "accepted"
    ):
        raise RelocationTargetMutationError(
            "provider_object_proof requires an accepted provider-boundary owner"
        )
    if target.get("kind") not in {
        "function",
        "provider-function",
        "compiler-function",
    }:
        raise RelocationTargetMutationError(
            "provider_object_proof requires an existing function target"
        )
    target_address = normalize_address(target.get("address"))
    if target_address != normalize_address(retail_target):
        raise RelocationTargetMutationError(
            "provider_object_proof requires the immutable retail target to equal "
            "the existing function start"
        )
    if not isinstance(target.get("end_exclusive"), str):
        raise RelocationTargetMutationError(
            "provider_object_proof target has no known function extent"
        )
    target_start = address_value(target_address)
    target_end = address_value(str(target["end_exclusive"]))
    if target_end <= target_start:
        raise RelocationTargetMutationError(
            "provider_object_proof target extent must be known and nonempty"
        )
    if not _exact_provider_relationship(
        owner,
        kind="primary-function",
        symbol_id=target_symbol_id,
        address=target_address,
    ):
        raise RelocationTargetMutationError(
            "provider_object_proof target lacks exactly one matching provider-owner "
            "primary-function relationship"
        )

    relative_path = str(proof["object_path"])
    object_path = _resolve_provider_object_path(
        vc5_root=vc5_root,
        relative_path=relative_path,
    )
    try:
        object_bytes = object_path.read_bytes()
        if len(object_bytes) < 2 or int.from_bytes(object_bytes[:2], "little") != 0x14C:
            raise ValueError("COFF machine is not IMAGE_FILE_MACHINE_I386")
        coff = CoffObject.from_bytes(object_bytes)
    except (OSError, ValueError) as exc:
        raise RelocationTargetMutationError(
            f"provider_object_proof COFF object is malformed: {exc}"
        ) from exc
    symbol_matches = [
        symbol for symbol in coff.symbols if symbol.name == object_symbol
    ]
    if len(symbol_matches) != 1:
        raise RelocationTargetMutationError(
            "provider_object_proof COFF object defines "
            f"{len(symbol_matches)} matching symbols; expected exactly one"
        )
    symbol = symbol_matches[0]
    if symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL:
        raise RelocationTargetMutationError(
            "provider_object_proof matching COFF symbol is not external"
        )
    if symbol.type != 0x20:
        raise RelocationTargetMutationError(
            "provider_object_proof matching COFF symbol is not a function symbol"
        )
    try:
        section = coff.section(symbol.section_number)
    except ValueError as exc:
        raise RelocationTargetMutationError(
            f"provider_object_proof matching COFF symbol has an invalid section: {exc}"
        ) from exc
    if (section.characteristics & IMAGE_SCN_CNT_CODE) == 0:
        raise RelocationTargetMutationError(
            "provider_object_proof matching COFF symbol is not in a code section"
        )

    body_size = target_end - target_start
    try:
        body = coff.function_bytes(object_symbol, byte_length=body_size)
    except ValueError as exc:
        raise RelocationTargetMutationError(
            f"provider_object_proof function extent mismatch: {exc}"
        ) from exc

    relocation_spans: list[tuple[int, int]] = []
    for relocation in coff.relocations_by_section.get(section.index, ()):
        try:
            size = relocation_size(relocation.type)
        except ValueError as exc:
            if body.start <= relocation.offset < body.end:
                raise RelocationTargetMutationError(
                    f"provider_object_proof has an unsupported body relocation: {exc}"
                ) from exc
            continue
        relocation_end = relocation.offset + size
        if relocation_end <= body.start or relocation.offset >= body.end:
            continue
        if relocation.offset < body.start or relocation_end > body.end:
            raise RelocationTargetMutationError(
                "provider_object_proof relocation field overlaps a function extent boundary"
            )
        relocation_spans.append((relocation.offset, relocation_end))
    relocation_spans.sort()
    for prior, current in zip(relocation_spans, relocation_spans[1:]):
        if current[0] < prior[1]:
            raise RelocationTargetMutationError(
                "provider_object_proof has overlapping COFF relocation fields"
            )

    retail_bytes = _immutable_retail_extent_bytes(
        reference,
        start=target_start,
        end_exclusive=target_end,
    )
    if len(retail_bytes) != len(body.data) or len(body.relocation_mask) != len(body.data):
        raise RelocationTargetMutationError(
            "provider_object_proof object and immutable retail extents do not match"
        )
    mismatches = [
        index
        for index, (object_byte, retail_byte, masked) in enumerate(
            zip(body.data, retail_bytes, body.relocation_mask)
        )
        if not masked and object_byte != retail_byte
    ]
    if mismatches:
        first = mismatches[0]
        raise RelocationTargetMutationError(
            "provider_object_proof immutable retail byte mismatch outside COFF "
            f"relocation fields at body offset 0x{first:x}"
        )
    masked_byte_count = sum(1 for masked in body.relocation_mask if masked)
    return {
        "schema": "recoil-provider-object-proof-v1",
        "validation_mode": "immutable-retail-vc5sp3-provider-object",
        "candidate_independent": True,
        "result": "passed",
        "owner_id": owner_id,
        "target_symbol_id": target_symbol_id,
        "target_address": target_address,
        "target_end_exclusive": normalize_address(target_end),
        "object_path": relative_path,
        "object_symbol": object_symbol,
        "section_name": body.section_name,
        "body_size": len(body.data),
        "relocation_count": len(body.relocations),
        "masked_byte_count": masked_byte_count,
        "unmasked_byte_count": len(body.data) - masked_byte_count,
    }


def _find_owner_relationship(
    owner: Mapping[str, Any],
    *,
    symbol_id: str,
    address: str,
    target: Mapping[str, Any],
) -> dict[str, Any] | None:
    target_kind = target.get("kind")
    if target_kind in {"function", "provider-function", "compiler-function"}:
        expected_kind = "primary-function"
    elif target_kind in {"data", "data-symbol", "provider-data", "compiler-data"}:
        expected_kind = "primary-data"
    else:
        raise RelocationTargetMutationError(
            f"existing target kind {target_kind!r} has no primary owner relationship contract"
        )
    matches = [
        dict(item)
        for item in owner.get("relationships", ())
        if isinstance(item, Mapping)
        and item.get("symbol_id") == symbol_id
        and normalize_address(item.get("address")) == address
    ]
    if len(matches) > 1:
        raise RelocationTargetMutationError(
            f"target symbol {symbol_id!r} has duplicate owner relationships"
        )
    if matches and matches[0].get("kind") != expected_kind:
        raise RelocationTargetMutationError(
            f"target symbol {symbol_id!r} owner relationship kind does not match "
            f"target row kind; expected {expected_kind}"
        )
    return matches[0] if matches else None


def _exact_provider_relationship(
    owner: Mapping[str, Any],
    *,
    kind: str,
    symbol_id: str,
    address: str,
) -> bool:
    matches: list[Mapping[str, Any]] = []
    for item in owner.get("relationships", ()):
        if not isinstance(item, Mapping) or item.get("symbol_id") != symbol_id:
            continue
        try:
            item_address = normalize_address(item.get("address"))
        except (TypeError, ValueError):
            continue
        if item_address == address:
            matches.append(item)
    return len(matches) == 1 and matches[0].get("kind") == kind


def _select_existing_target_candidate(
    document: ProgressDocument,
    *,
    candidates: Sequence[tuple[str, Mapping[str, Any]]],
    retail_target: int,
    owner: Mapping[str, Any],
    object_symbol: str,
    manifest_dir: Path,
) -> tuple[str, Mapping[str, Any]]:
    """Select one target, allowing only the typed function/data view of an IAT slot.

    A registered imported-function provider target intentionally has a one-byte
    callable provider-function view and a co-addressed four-byte primary-data
    storage view.  That pair is not a relocation-target ambiguity when the
    requested VC5 object identity uniquely selects the provider-function and
    every co-addressed data row is exactly linked to the same provider owner.
    No other overlap is canonicalized here.
    """

    if len(candidates) == 1:
        return candidates[0]
    if owner.get("kind") != "provider-boundary":
        raise RelocationTargetMutationError(
            f"immutable retail target resolves to {len(candidates)} existing symbols; expected one"
        )
    target_address = normalize_address(retail_target)
    function_candidates: list[tuple[str, Mapping[str, Any]]] = []
    for symbol_id, row in candidates:
        if row.get("kind") != "provider-function":
            continue
        if normalize_address(row.get("address")) != target_address:
            continue
        if not _exact_provider_relationship(
            owner,
            kind="primary-function",
            symbol_id=symbol_id,
            address=target_address,
        ):
            continue
        if _existing_object_identity(
            document,
            row=row,
            object_symbol=object_symbol,
            manifest_dir=manifest_dir,
        ):
            function_candidates.append((symbol_id, row))
    if len(function_candidates) != 1:
        raise RelocationTargetMutationError(
            "co-addressed provider target does not have exactly one owner-linked "
            "provider-function matching the reviewed object identity"
        )
    selected_id, selected = function_candidates[0]
    for symbol_id, row in candidates:
        if symbol_id == selected_id:
            continue
        if row.get("kind") not in {"data", "data-symbol", "provider-data"}:
            raise RelocationTargetMutationError(
                "co-addressed provider target contains a non-data competing symbol"
            )
        if normalize_address(row.get("address")) != target_address or not _exact_provider_relationship(
            owner,
            kind="primary-data",
            symbol_id=symbol_id,
            address=target_address,
        ):
            raise RelocationTargetMutationError(
                "co-addressed provider data is not exactly linked to the selected provider owner"
            )
    return selected_id, selected


def _pending_data_symbol(
    *,
    address: int,
    end_exclusive: int,
    name: str,
    output_section_id: str,
    evidence_ids: Sequence[str],
) -> dict[str, Any]:
    dimensions = (
        "object_byte",
        "relocation_identity",
        "linked_presence",
        "linked_address",
        "linked_target_identity",
        "linked_targets",
        "linked_body_byte",
        "linked_byte",
    )
    return {
        "accepted_byte_facts": None,
        "accepted_order_facts": None,
        "address": normalize_address(address),
        "binary": "recoil",
        "binary_state": {
            dimension: state_record(
                result="pending",
                disposition="observed",
                freshness="historical",
                evidence_ids=evidence_ids,
                gating=False,
                validation_mode="historical-observation",
            )
            for dimension in dimensions
        },
        "binary_state_diagnostics": {
            "legacy_order": state_record(
                result="pending",
                disposition="observed",
                freshness="historical",
                evidence_ids=(),
                gating=False,
                validation_mode="historical-observation",
            )
        },
        "disposition": "authored",
        "end_exclusive": normalize_address(end_exclusive),
        "evidence_ids": sorted(set(evidence_ids)),
        "extent_state": "known",
        "kind": "data",
        "navigation_name": name,
        "output_section_id": output_section_id,
        "ownership_state": "primary-owned",
        "physical_block_id": None,
        "semantic_span_ids": [],
        "size": end_exclusive - address,
        "storage_contribution_ids": [],
        "verification_target_ids": [],
    }


def _append_binding(row: dict[str, Any], binding: Mapping[str, Any]) -> None:
    current = row.get("relocation_target_binding")
    if current is None:
        rows: list[Any] = []
    elif isinstance(current, Mapping):
        rows = [current]
    elif isinstance(current, list):
        rows = list(current)
    else:
        raise RelocationTargetMutationError("relocation_target_binding has an invalid shape")
    for existing in rows:
        if not isinstance(existing, Mapping):
            raise RelocationTargetMutationError("existing relocation target binding is not an object")
        try:
            existing_normalized = normalize_relocation_target_binding(existing)
        except RelocationExpectationError as exc:
            raise RelocationTargetMutationError(
                f"existing relocation target binding is invalid: {exc}"
            ) from exc
        if existing_normalized == dict(binding):
            raise RelocationTargetMutationError(
                "identical reviewed relocation target binding is already present"
            )
        if existing_normalized["object_symbol"] == binding["object_symbol"]:
            raise RelocationTargetMutationError(
                "conflicting reviewed relocation target binding already exists"
            )
    rows.append(dict(binding))
    row["relocation_target_binding"] = rows[0] if len(rows) == 1 else rows


def _binding_rows(row: Mapping[str, Any]) -> list[Any]:
    current = row.get("relocation_target_binding")
    if current is None:
        return []
    if isinstance(current, Mapping):
        return [current]
    if isinstance(current, list):
        return list(current)
    raise RelocationTargetMutationError("relocation_target_binding has an invalid shape")


def _select_binding_for_correction(
    row: Mapping[str, Any],
    *,
    selector: Mapping[str, Any],
    request: Mapping[str, Any],
    source_symbol_id: str,
    source_address: str,
    source_binding: Mapping[str, Any],
    relocation_context: Mapping[str, Any],
    target_symbol_id: str,
    owner_id: str,
    relationship: Mapping[str, Any] | None,
    document: ProgressDocument,
    bindings: Mapping[str, Sequence[Any]],
    reference: Path,
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    expected_selector = {
        "prior_target_symbol_id": target_symbol_id,
        "source_symbol_id": source_symbol_id,
        "source_address": normalize_address(source_address),
        "source_object_symbol": request["source_object_symbol"],
        "offset": request["offset"],
    }
    mismatches = [
        field
        for field, expected in expected_selector.items()
        if selector.get(field) != expected
    ]
    if mismatches:
        raise RelocationTargetMutationError(
            "correction selector does not match the current command/site: "
            f"{sorted(mismatches)}"
        )
    prior_object_symbol = str(selector["prior_target_object_symbol"])
    refresh_only = selector.get("refresh_only") is True
    if refresh_only and prior_object_symbol != request["target_object_symbol"]:
        raise RelocationTargetMutationError(
            "refresh-only correction must preserve the prior target object symbol"
        )
    if not refresh_only and prior_object_symbol == request["target_object_symbol"]:
        raise RelocationTargetMutationError(
            "correction must replace the prior target object symbol with a different symbol"
        )
    normalized_rows: list[dict[str, Any]] = []
    for existing in _binding_rows(row):
        if not isinstance(existing, Mapping):
            raise RelocationTargetMutationError(
                "existing relocation target binding is not an object"
            )
        try:
            normalized_rows.append(normalize_relocation_target_binding(existing))
        except RelocationExpectationError as exc:
            raise RelocationTargetMutationError(
                f"existing relocation target binding is invalid: {exc}"
            ) from exc
    matches = []
    for existing in normalized_rows:
        context = existing["binding_context"]
        stored_source = context["source_binding"]
        stored_relocation = context["relocation"]
        stored_target = context["target"]
        if (
            existing["object_symbol"] == prior_object_symbol
            and stored_source["symbol_id"] == selector["source_symbol_id"]
            and stored_source["address"] == selector["source_address"]
            and stored_source["object_symbol"] == selector["source_object_symbol"]
            and stored_relocation["offset"] == selector["offset"]
            and stored_target["symbol_id"] == selector["prior_target_symbol_id"]
        ):
            matches.append(existing)
    if len(matches) != 1:
        raise RelocationTargetMutationError(
            "correction selector resolves to "
            f"{len(matches)} existing reviewed bindings; expected exactly one"
        )
    prior = matches[0]
    try:
        _normalized, stale = relocation_target_binding_staleness(
            prior,
            document=document,
            bindings=bindings,
            target_symbol_id=target_symbol_id,
            reference=reference,
        )
    except (RelocationExpectationError, OSError, ValueError) as exc:
        raise RelocationTargetMutationError(
            f"existing relocation target binding cannot be revalidated: {exc}"
        ) from exc
    if refresh_only and not stale:
        raise RelocationTargetMutationError(
            "refresh-only correction requires an existing stale relocation target binding"
        )
    if not refresh_only and stale:
        raise RelocationTargetMutationError(
            "existing relocation target binding is stale; correction refused: "
            + json.dumps(stale)
        )
    if not refresh_only:
        context = prior["binding_context"]
        exact_context_mismatches: list[str] = []
        if context["source_binding"] != dict(source_binding):
            exact_context_mismatches.append("source_binding")
        if context["relocation"] != dict(relocation_context):
            exact_context_mismatches.append("relocation")
        if context["target"]["symbol_id"] != target_symbol_id:
            exact_context_mismatches.append("target_symbol_id")
        if context["owner"]["owner_id"] != owner_id:
            exact_context_mismatches.append("owner_id")
        if context["relationship"] != relationship:
            exact_context_mismatches.append("relationship")
        if prior["evidence_ids"] != request["evidence_ids"]:
            exact_context_mismatches.append("evidence_ids")
        if exact_context_mismatches:
            raise RelocationTargetMutationError(
                "correction would cross the existing binding context: "
                f"{sorted(exact_context_mismatches)}"
            )
    return prior, stale


def _replace_binding(
    row: dict[str, Any],
    *,
    prior: Mapping[str, Any],
    replacement: Mapping[str, Any],
) -> None:
    rows = _binding_rows(row)
    normalized_rows: list[dict[str, Any]] = []
    for existing in rows:
        if not isinstance(existing, Mapping):
            raise RelocationTargetMutationError(
                "existing relocation target binding is not an object"
            )
        try:
            normalized_rows.append(normalize_relocation_target_binding(existing))
        except RelocationExpectationError as exc:
            raise RelocationTargetMutationError(
                f"existing relocation target binding is invalid: {exc}"
            ) from exc
    matches = [index for index, existing in enumerate(normalized_rows) if existing == dict(prior)]
    if len(matches) != 1:
        raise RelocationTargetMutationError(
            "the fully revalidated prior binding is no longer uniquely replaceable"
        )
    replace_index = matches[0]
    for index, existing in enumerate(normalized_rows):
        if index == replace_index:
            continue
        if existing == dict(replacement) or existing["object_symbol"] == replacement["object_symbol"]:
            raise RelocationTargetMutationError(
                "replacement would conflict with another reviewed relocation target binding"
            )
    rows[replace_index] = dict(replacement)
    row["relocation_target_binding"] = rows[0] if len(rows) == 1 else rows


def bind_relocation_target(
    *,
    progress: Path,
    reference: Path,
    manifest_dir: Path,
    source_symbol_id: str,
    source_address: str,
    payload: Mapping[str, Any],
    expected_revision: int,
    apply: bool,
    bindings: Mapping[str, Sequence[Any]] | None = None,
    vc5_root: Path | None = None,
) -> dict[str, Any]:
    request = normalize_reviewed_target_request(payload)
    store = ProgressStore(progress)
    try:
        document = store.load()
    except ProgressError as exc:
        raise RelocationTargetMutationError(str(exc)) from exc
    if document.revision != expected_revision:
        raise RelocationTargetMutationError(
            f"revision changed: expected {expected_revision}, found {document.revision}"
        )
    if bindings is None:
        from _recoil.commands.live_byte_verify import _bindings

        try:
            bindings = _bindings(document, manifest_dir)
        except (RuntimeError, OSError, ValueError) as exc:
            raise RelocationTargetMutationError(str(exc)) from exc
    source = _source_row(
        document,
        source_symbol_id=source_symbol_id,
        source_address=source_address,
    )
    try:
        source_binding = build_object_binding_snapshot(
            document,
            bindings,
            symbol_id=source_symbol_id,
            object_symbol=str(request["source_object_symbol"]),
        )
        relocation = decode_retail_relocation_at_offset(
            row=source,
            offset=int(request["offset"]),
            reference=reference,
        )
    except (RelocationExpectationError, OSError, ValueError) as exc:
        raise RelocationTargetMutationError(str(exc)) from exc
    relocation_context = {
        field: relocation[field]
        for field in (
            "offset",
            "type",
            "type_name",
            "retail_target",
            "instruction_offset",
            "opcode",
        )
    }
    retail_target = int(relocation["retail_target"])
    owner_id = str(request["target_owner_id"])
    owner = _validate_owner_evidence(
        document,
        owner_id=owner_id,
        evidence_ids=request["evidence_ids"],
    )
    candidates = _target_candidates(document, retail_target)
    create = bool(request["create_missing_data"])
    if create and candidates:
        raise RelocationTargetMutationError(
            "create_missing_data requested but immutable retail target already has a symbol"
        )
    selected_candidate: tuple[str, Mapping[str, Any]] | None = None
    if not create:
        selected_candidate = _select_existing_target_candidate(
            document,
            candidates=candidates,
            retail_target=retail_target,
            owner=owner,
            object_symbol=str(request["target_object_symbol"]),
            manifest_dir=manifest_dir,
        )

    proposed = deepcopy(document.data)
    proposed_symbols = proposed["symbols"]
    proposed_owners = proposed["owners"]
    relationship: dict[str, Any] | None = None
    creation_mode = "existing-symbol"
    prior_binding: dict[str, Any] | None = None
    prior_staleness: list[dict[str, Any]] = []
    refresh_only = False
    provider_object_proof: dict[str, Any] | None = None
    if create:
        if owner.get("kind") == "provider-boundary":
            raise RelocationTargetMutationError(
                "missing authored data cannot be created as a provider-owner primary symbol"
            )
        end_exclusive = address_value(str(request["target_end_exclusive"]))
        if end_exclusive <= retail_target:
            raise RelocationTargetMutationError("created data extent must be known and nonempty")
        output_section_id = _retail_section(
            document,
            reference=reference,
            start=retail_target,
            end_exclusive=end_exclusive,
        )
        for other_id, other in document.collection("symbols").items():
            if not isinstance(other, Mapping) or other.get("binary") != "recoil":
                continue
            if not isinstance(other.get("address"), str) or not isinstance(
                other.get("end_exclusive"), str
            ):
                continue
            other_start = address_value(str(other["address"]))
            other_end = address_value(str(other["end_exclusive"]))
            if retail_target < other_end and other_start < end_exclusive:
                raise RelocationTargetMutationError(
                    f"created data extent overlaps current symbol {other_id!r}"
                )
        target_symbol_id = f"recoil:data:0x{retail_target:x}"
        target_name = str(request["target_name"])
        proposed_symbols[target_symbol_id] = _pending_data_symbol(
            address=retail_target,
            end_exclusive=end_exclusive,
            name=target_name,
            output_section_id=output_section_id,
            evidence_ids=request["evidence_ids"],
        )
        relationship = {
            "kind": "primary-data",
            "address": normalize_address(retail_target),
            "symbol_id": target_symbol_id,
            "name": target_name,
        }
        owner_copy = proposed_owners[owner_id]
        relationships = owner_copy.setdefault("relationships", [])
        if any(
            isinstance(item, Mapping)
            and (
                item.get("symbol_id") == target_symbol_id
                or (
                    item.get("kind") == "primary-data"
                    and normalize_address(item.get("address")) == relationship["address"]
                )
            )
            for item in relationships
        ):
            raise RelocationTargetMutationError(
                "created data owner relationship already exists or conflicts"
            )
        relationships.append(relationship)
        reimplementation = owner_copy.setdefault("reimplementation", {})
        entries = reimplementation.setdefault("entries", {})
        if target_symbol_id in entries:
            raise RelocationTargetMutationError("created data tier entry already exists")
        entries[target_symbol_id] = {"kind": "data", "tier": "X", "evidence_ids": []}
        creation_mode = "created-data-symbol"
    else:
        if selected_candidate is None:
            raise RelocationTargetMutationError("existing target candidate selection failed")
        target_symbol_id, target = selected_candidate
        target_start = address_value(str(target["address"]))
        target_end = address_value(str(target["end_exclusive"]))
        _retail_section(
            document,
            reference=reference,
            start=target_start,
            end_exclusive=target_end,
        )
        if owner.get("kind") == "provider-boundary":
            relationship = None
        else:
            relationship = _find_owner_relationship(
                owner,
                symbol_id=target_symbol_id,
                address=normalize_address(target_start),
                target=target,
            )
            if relationship is None:
                raise RelocationTargetMutationError(
                    "non-provider existing target lacks an exact owner relationship"
                )
        correction = request.get("correction")
        if correction is None:
            registered_object_identity = _existing_object_identity(
                document,
                row=target,
                object_symbol=str(request["target_object_symbol"]),
                manifest_dir=manifest_dir,
            )
            proof = request.get("provider_object_proof")
            if registered_object_identity and proof is not None:
                raise RelocationTargetMutationError(
                    "provider_object_proof is valid only for a previously unregistered "
                    "target object symbol"
                )
            if not registered_object_identity and proof is None:
                raise RelocationTargetMutationError(
                    "reviewed target object symbol does not match current exact tracker identity"
                )
            if not registered_object_identity:
                provider_object_proof = _validate_provider_object_proof(
                    proof=proof,
                    vc5_root=DEFAULT_VC5_ROOT if vc5_root is None else vc5_root,
                    reference=reference,
                    owner=owner,
                    owner_id=owner_id,
                    target=target,
                    target_symbol_id=target_symbol_id,
                    retail_target=retail_target,
                    object_symbol=str(request["target_object_symbol"]),
                )
        else:
            refresh_only = correction.get("refresh_only") is True
            prior_binding, prior_staleness = _select_binding_for_correction(
                target,
                selector=correction,
                request=request,
                source_symbol_id=source_symbol_id,
                source_address=source_address,
                source_binding=source_binding,
                relocation_context=relocation_context,
                target_symbol_id=target_symbol_id,
                owner_id=owner_id,
                relationship=relationship,
                document=document,
                bindings=bindings,
                reference=reference,
            )
            if not refresh_only:
                creation_mode = str(prior_binding["binding_context"]["creation_mode"])

    proposed_document = ProgressDocument(proposed)
    target_row = proposed_document.collection("symbols")[target_symbol_id]
    owner_row = proposed_document.collection("owners")[owner_id]
    raw_binding = {
        "reviewed": True,
        "object_symbol": request["target_object_symbol"],
        "reason": request["reason"],
        "evidence_ids": request["evidence_ids"],
        "binding_context": {
            "source_binding": source_binding,
            "relocation": relocation_context,
            "target": relocation_target_row_context(
                symbol_id=target_symbol_id,
                row=target_row,
                object_symbol=str(request["target_object_symbol"]),
            ),
            "owner": relocation_target_owner_context(
                owner_id=owner_id,
                owner=owner_row,
                evidence_ids=request["evidence_ids"],
            ),
            "relationship": relationship,
            "creation_mode": creation_mode,
        },
    }
    try:
        normalized_binding = normalize_relocation_target_binding(raw_binding)
    except RelocationExpectationError as exc:
        raise RelocationTargetMutationError(str(exc)) from exc
    target_row_copy = proposed_symbols[target_symbol_id]
    if prior_binding is None:
        _append_binding(target_row_copy, normalized_binding)
    else:
        _replace_binding(
            target_row_copy,
            prior=prior_binding,
            replacement=normalized_binding,
        )
    proposed_document = ProgressDocument(proposed)
    try:
        _normalized, stale = relocation_target_binding_staleness(
            normalized_binding,
            document=proposed_document,
            bindings=bindings,
            target_symbol_id=target_symbol_id,
            reference=reference,
        )
    except (RelocationExpectationError, OSError, ValueError) as exc:
        raise RelocationTargetMutationError(str(exc)) from exc
    if stale:
        raise RelocationTargetMutationError(
            "proposed relocation target binding is stale before commit: " + json.dumps(stale)
        )
    try:
        commit = store.commit(
            proposed,
            expected_revision=expected_revision,
            apply=apply,
        )
    except (ConcurrentProgressUpdate, ProgressError) as exc:
        raise RelocationTargetMutationError(str(exc)) from exc
    return {
        "report_version": 1,
        "kind": "relocation-target-binding-mutation",
        "validation_mode": "immutable-retail-and-current-tracker",
        "candidate_independent": True,
        "source_symbol_id": source_symbol_id,
        "source_address": normalize_address(source_address),
        "target_symbol_id": target_symbol_id,
        "target_created": create,
        "mutation_mode": (
            "refresh"
            if refresh_only
            else "correction"
            if prior_binding is not None
            else "append"
        ),
        "replaced_binding": prior_binding,
        "prior_staleness": prior_staleness,
        "target_address": normalize_address(retail_target),
        "reference": display_path(reference),
        "binding": normalized_binding,
        "provider_object_proof": provider_object_proof,
        "commit": commit.to_dict(),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Bind one immutable-retail relocation target to reviewed tracker identity."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    child = subparsers.add_parser("bind")
    child.add_argument("--source-symbol-id", required=True)
    child.add_argument("--source-address", required=True)
    child.add_argument("--payload-json", required=True)
    child.add_argument("--progress", type=Path, default=DEFAULT_TRACKER)
    child.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    child.add_argument("--manifest-dir", type=Path, default=DEFAULT_MANIFEST_DIR)
    child.add_argument("--expected-revision", type=int, required=True)
    mode = child.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    child.add_argument("--json", action="store_true")
    return parser


def run(args: argparse.Namespace) -> dict[str, Any]:
    if args.command != "bind":
        raise RelocationTargetMutationError(f"unsupported operation {args.command!r}")
    return bind_relocation_target(
        progress=args.progress,
        reference=args.reference,
        manifest_dir=args.manifest_dir,
        source_symbol_id=args.source_symbol_id,
        source_address=args.source_address,
        payload=_payload(args.payload_json),
        expected_revision=args.expected_revision,
        apply=bool(args.apply),
    )


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        report = run(args)
    except (OSError, ValueError, RelocationTargetMutationError) as exc:
        print(f"relocation target mutation error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        mode = "APPLIED" if report["commit"]["applied"] else "DRY-RUN"
        created = "created" if report["target_created"] else "existing"
        print(
            f"Relocation target {mode}: {report['source_symbol_id']} -> "
            f"{report['target_symbol_id']} ({created})"
        )
        print(
            f"revision {report['commit']['previous_revision']} -> {report['commit']['revision']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
