from __future__ import annotations

import argparse
from copy import deepcopy
from dataclasses import asdict, dataclass
import json
from pathlib import Path
import re
import struct
import sys
from typing import Any, Mapping

from _recoil.commands.progress_v2 import add_live_evidence
from _recoil.lib.pe import (
    PeFormatError,
    data_directory,
    parse_pe_headers,
    read_c_string,
    rva_to_offset,
)
from _recoil.lib.progress import (
    CommitResult,
    ConcurrentProgressUpdate,
    DEFAULT_PROGRESS_PATH,
    ProgressDocument,
    ProgressError,
    ProgressStore,
    address_value,
    normalize_address,
    state_record,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path
from _recoil.lib.windows_identity import StableReadHandle


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
DEFAULT_REFERENCE = REPO_ROOT / "support" / "Recoil.exe"
IMAGE_FILE_MACHINE_I386 = 0x14C
PE32_THUNK_SIZE = 4
SYMBOL_BINARY_DIMENSIONS = (
    "object_byte",
    "relocation_identity",
    "linked_presence",
    "linked_address",
    "linked_target_identity",
    "linked_targets",
    "linked_body_byte",
    "linked_byte",
)
STORAGE_DIMENSIONS = (
    "extent",
    "object",
    "relocation",
    "order",
    "link",
    "raw",
    "zero-fill",
)
OWNER_ID_RE = re.compile(r"^recoil:owner:provider\.[a-z0-9][a-z0-9_.-]*$")


class ProviderTargetMutationError(RuntimeError):
    pass


@dataclass(frozen=True)
class RetailImportTarget:
    address: str
    dll: str
    import_name: str
    import_ordinal: int | None
    descriptor_index: int
    thunk_index: int
    iat_rva: str
    iat_end_rva: str


def _payload(value: str) -> dict[str, Any]:
    try:
        parsed = json.loads(value)
    except json.JSONDecodeError as exc:
        raise ProviderTargetMutationError(f"invalid --payload-json: {exc}") from exc
    if not isinstance(parsed, dict):
        raise ProviderTargetMutationError("--payload-json must decode to an object")
    return parsed


def normalize_provider_target_request(value: Mapping[str, Any]) -> dict[str, Any]:
    allowed = {
        "reviewed",
        "dll",
        "import_name",
        "import_ordinal",
        "object_symbol",
        "owner_id",
        "owner_name",
        "reason",
    }
    keys = {str(key) for key in value}
    candidate_fields = sorted(key for key in keys if "candidate" in key.casefold())
    if candidate_fields:
        raise ProviderTargetMutationError(
            f"candidate-derived provider target fields are forbidden: {candidate_fields}"
        )
    unknown = sorted(keys - allowed)
    if unknown:
        raise ProviderTargetMutationError(
            f"reviewed provider target payload has unsupported fields: {unknown}"
        )
    if value.get("reviewed") is not True:
        raise ProviderTargetMutationError("reviewed provider target must set reviewed=true")

    result: dict[str, Any] = {"reviewed": True}
    for field in ("dll", "import_name", "object_symbol", "owner_id", "owner_name", "reason"):
        item = value.get(field)
        if not isinstance(item, str) or not item.strip():
            raise ProviderTargetMutationError(f"{field} must be a non-empty string")
        result[field] = item.strip()

    if "/" in result["dll"] or "\\" in result["dll"]:
        raise ProviderTargetMutationError("dll must be an exact retail import basename")
    ordinal = value.get("import_ordinal")
    if result["import_name"].startswith("#"):
        if isinstance(ordinal, bool) or not isinstance(ordinal, int) or not 1 <= ordinal <= 0xffff:
            raise ProviderTargetMutationError(
                "ordinal provider target requires exact reviewed import_ordinal in 1..65535"
            )
        if result["import_name"] != f"#{ordinal}":
            raise ProviderTargetMutationError(
                "import_name ordinal and reviewed import_ordinal do not match exactly"
            )
        result["import_ordinal"] = ordinal
    elif ordinal is not None:
        raise ProviderTargetMutationError(
            "import_ordinal is valid only when import_name is the exact #ordinal identity"
        )
    if any(char.isspace() for char in result["object_symbol"]):
        raise ProviderTargetMutationError("object_symbol cannot contain whitespace")
    if not result["object_symbol"].startswith("__imp_"):
        raise ProviderTargetMutationError(
            "object_symbol must be the reviewed VC5 imported-address symbol beginning __imp_"
        )
    if OWNER_ID_RE.fullmatch(result["owner_id"]) is None:
        raise ProviderTargetMutationError(
            "owner_id must be a canonical recoil:owner:provider.* id"
        )
    return result


def _bounded_unpack_from(fmt: str, image: bytes, offset: int, *, context: str) -> tuple[Any, ...]:
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(image):
        raise ProviderTargetMutationError(
            f"immutable retail import table is truncated while reading {context}"
        )
    return struct.unpack_from(fmt, image, offset)


def _retail_import_targets(reference: Path) -> tuple[list[RetailImportTarget], dict[str, Any]]:
    try:
        with StableReadHandle(reference) as stable_reference:
            image = stable_reference.read()
            reference_identity = stable_reference.identity.to_dict()
        headers = parse_pe_headers(image, source=str(reference))
    except (OSError, PeFormatError, ValueError) as exc:
        raise ProviderTargetMutationError(f"cannot parse immutable retail PE: {exc}") from exc
    if headers.machine != IMAGE_FILE_MACHINE_I386:
        raise ProviderTargetMutationError(
            f"immutable retail PE machine is 0x{headers.machine:04x}, expected i386"
        )
    import_directory = data_directory(headers, 1)
    iat_directory = data_directory(headers, 12)
    if (
        import_directory.rva == 0
        or import_directory.size < 20
        or import_directory.file_offset is None
    ):
        raise ProviderTargetMutationError("immutable retail PE has no bounded import directory")
    if iat_directory.rva == 0 or iat_directory.size < PE32_THUNK_SIZE:
        raise ProviderTargetMutationError("immutable retail PE has no bounded IAT directory")

    max_descriptors = import_directory.size // 20
    targets: list[RetailImportTarget] = []
    saw_terminator = False
    for descriptor_index in range(max_descriptors):
        descriptor_offset = import_directory.file_offset + descriptor_index * 20
        original_first_thunk, _timestamp, _forwarder, name_rva, first_thunk = _bounded_unpack_from(
            "<IIIII",
            image,
            descriptor_offset,
            context=f"descriptor {descriptor_index}",
        )
        if not any((original_first_thunk, name_rva, first_thunk)):
            saw_terminator = True
            break
        if name_rva == 0 or first_thunk == 0:
            raise ProviderTargetMutationError(
                f"immutable retail import descriptor {descriptor_index} lacks name or FirstThunk"
            )
        name_offset = rva_to_offset(name_rva, headers.sections)
        lookup_rva = original_first_thunk or first_thunk
        lookup_offset = rva_to_offset(lookup_rva, headers.sections)
        if name_offset is None or lookup_offset is None:
            raise ProviderTargetMutationError(
                f"immutable retail import descriptor {descriptor_index} cannot be mapped"
            )
        try:
            dll = read_c_string(image, name_offset)
        except (ValueError, UnicodeError) as exc:
            raise ProviderTargetMutationError(
                f"immutable retail import descriptor {descriptor_index} has an invalid DLL name"
            ) from exc
        if not dll:
            raise ProviderTargetMutationError(
                f"immutable retail import descriptor {descriptor_index} has an empty DLL name"
            )

        first_iat_rva = first_thunk
        if first_iat_rva < iat_directory.rva or first_iat_rva >= iat_directory.rva + iat_directory.size:
            raise ProviderTargetMutationError(
                f"immutable retail import descriptor {descriptor_index} FirstThunk is outside the IAT"
            )
        max_thunks = (iat_directory.rva + iat_directory.size - first_iat_rva) // PE32_THUNK_SIZE
        saw_thunk_terminator = False
        for thunk_index in range(max_thunks):
            lookup_value = _bounded_unpack_from(
                "<I",
                image,
                lookup_offset + thunk_index * PE32_THUNK_SIZE,
                context=f"descriptor {descriptor_index} thunk {thunk_index}",
            )[0]
            if lookup_value == 0:
                saw_thunk_terminator = True
                break
            if lookup_value & 0x80000000:
                import_ordinal = lookup_value & 0xffff
                import_name = f"#{import_ordinal}"
            else:
                import_ordinal = None
                hint_name_offset = rva_to_offset(lookup_value, headers.sections)
                if hint_name_offset is None:
                    raise ProviderTargetMutationError(
                        f"immutable retail import name for descriptor {descriptor_index} "
                        f"thunk {thunk_index} cannot be mapped"
                    )
                try:
                    import_name = read_c_string(image, hint_name_offset + 2)
                except (ValueError, UnicodeError) as exc:
                    raise ProviderTargetMutationError(
                        f"immutable retail import name for descriptor {descriptor_index} "
                        f"thunk {thunk_index} is invalid"
                    ) from exc
                if not import_name:
                    raise ProviderTargetMutationError(
                        f"immutable retail import name for descriptor {descriptor_index} "
                        f"thunk {thunk_index} is empty"
                    )
            iat_rva = first_thunk + thunk_index * PE32_THUNK_SIZE
            if iat_rva + PE32_THUNK_SIZE > iat_directory.rva + iat_directory.size:
                raise ProviderTargetMutationError(
                    f"immutable retail import descriptor {descriptor_index} exceeds the IAT"
                )
            targets.append(
                RetailImportTarget(
                    address=normalize_address(headers.image_base + iat_rva),
                    dll=dll,
                    import_name=import_name,
                    import_ordinal=import_ordinal,
                    descriptor_index=descriptor_index,
                    thunk_index=thunk_index,
                    iat_rva=normalize_address(iat_rva),
                    iat_end_rva=normalize_address(iat_rva + PE32_THUNK_SIZE),
                )
            )
        if not saw_thunk_terminator:
            raise ProviderTargetMutationError(
                f"immutable retail import descriptor {descriptor_index} lacks a bounded thunk terminator"
            )
    if not saw_terminator:
        raise ProviderTargetMutationError(
            "immutable retail import directory lacks a bounded null descriptor"
        )
    return targets, {
        "reference_physical_identity": reference_identity,
        "image_base": normalize_address(headers.image_base),
        "import_directory_rva": normalize_address(import_directory.rva),
        "import_directory_size": import_directory.size,
        "iat_directory_rva": normalize_address(iat_directory.rva),
        "iat_directory_size": iat_directory.size,
    }


def retail_import_target(
    *,
    reference: Path,
    address: str,
    dll: str,
    import_name: str,
    import_ordinal: int | None = None,
) -> tuple[RetailImportTarget, dict[str, Any]]:
    normalized_address = normalize_address(address)
    targets, directory_context = _retail_import_targets(reference)
    at_address = [target for target in targets if target.address == normalized_address]
    if len(at_address) != 1:
        raise ProviderTargetMutationError(
            f"immutable retail IAT address {normalized_address} resolves to "
            f"{len(at_address)} imports; expected exactly one"
        )
    target = at_address[0]
    if (
        target.dll != dll
        or target.import_name != import_name
        or target.import_ordinal != import_ordinal
    ):
        raise ProviderTargetMutationError(
            f"immutable retail IAT address {normalized_address} is "
            f"{target.dll}!{target.import_name} ordinal={target.import_ordinal!r}, not "
            f"{dll}!{import_name} ordinal={import_ordinal!r}"
        )
    exact = [
        item
        for item in targets
        if item.address == normalized_address
        and item.dll == dll
        and item.import_name == import_name
        and item.import_ordinal == import_ordinal
    ]
    if len(exact) != 1:
        raise ProviderTargetMutationError(
            "immutable retail address/DLL/import-name identity is ambiguous"
        )
    return target, directory_context


def _retail_output_section(
    document: ProgressDocument,
    *,
    reference: Path,
    start: int,
    end_exclusive: int,
) -> str:
    try:
        with StableReadHandle(reference) as stable_reference:
            image = stable_reference.read()
        headers = parse_pe_headers(image, source=str(reference))
    except (OSError, PeFormatError, ValueError) as exc:
        raise ProviderTargetMutationError(f"cannot parse immutable retail PE: {exc}") from exc
    matches = []
    for section in headers.sections:
        section_start = headers.image_base + section.virtual_address
        section_end = section_start + max(section.virtual_size, section.raw_size)
        if section_start <= start and end_exclusive <= section_end:
            matches.append(section)
    if len(matches) != 1:
        raise ProviderTargetMutationError(
            "retail IAT extent does not resolve to exactly one PE output section"
        )
    section_id = f"recoil:section:{matches[0].name}"
    section_row = document.collection("output_sections").get(section_id)
    if (
        not isinstance(section_row, Mapping)
        or section_row.get("binary") != "recoil"
        or section_row.get("name") != matches[0].name
    ):
        raise ProviderTargetMutationError(
            f"retail IAT output section {section_id!r} is not exactly registered"
        )
    return section_id


def _known_range(value: Any) -> tuple[int, int] | None:
    if not isinstance(value, Mapping):
        return None
    raw_start = value.get("address", value.get("start"))
    raw_end = value.get("end_exclusive")
    if not isinstance(raw_start, (str, int)) or not isinstance(raw_end, (str, int)):
        return None
    try:
        start = address_value(raw_start)
        end = address_value(raw_end)
    except (TypeError, ValueError):
        return None
    return (start, end) if end > start else None


def _reject_existing_or_overlapping(
    document: ProgressDocument,
    *,
    owner_id: str,
    address: int,
    end_exclusive: int,
    function_id: str,
    data_id: str,
    storage_id: str,
    allow_exact_legacy_ids: bool = False,
) -> None:
    if owner_id in document.collection("owners") and not allow_exact_legacy_ids:
        raise ProviderTargetMutationError(f"provider owner already exists: {owner_id}")
    symbols = document.collection("symbols")
    for symbol_id in (function_id, data_id):
        if symbol_id in symbols and not allow_exact_legacy_ids:
            raise ProviderTargetMutationError(f"provider target symbol already exists: {symbol_id}")
    storage_rows = document.collection("storage_contributions")
    if storage_id in storage_rows and not allow_exact_legacy_ids:
        raise ProviderTargetMutationError(f"provider IAT storage already exists: {storage_id}")

    conflicts: list[str] = []
    for symbol_id, row in symbols.items():
        if allow_exact_legacy_ids and symbol_id in (function_id, data_id):
            continue
        if not isinstance(row, Mapping) or row.get("binary") != "recoil":
            continue
        extent = _known_range(row)
        if extent is not None and address < extent[1] and extent[0] < end_exclusive:
            conflicts.append(f"symbol:{symbol_id}")
            continue
        try:
            row_address = address_value(row.get("address"))
        except (TypeError, ValueError):
            continue
        if address <= row_address < end_exclusive:
            conflicts.append(f"symbol:{symbol_id}")
    for contribution_id, row in storage_rows.items():
        if allow_exact_legacy_ids and contribution_id == storage_id:
            continue
        if not isinstance(row, Mapping) or row.get("binary") != "recoil":
            continue
        reference = row.get("reference")
        extent = _known_range(reference)
        if extent is not None and address < extent[1] and extent[0] < end_exclusive:
            conflicts.append(f"storage:{contribution_id}")
            continue
        if isinstance(reference, Mapping):
            try:
                row_address = address_value(reference.get("address"))
            except (TypeError, ValueError):
                continue
            if address <= row_address < end_exclusive:
                conflicts.append(f"storage:{contribution_id}")
    for existing_owner_id, owner in document.collection("owners").items():
        if allow_exact_legacy_ids and existing_owner_id == owner_id:
            continue
        if not isinstance(owner, Mapping) or owner.get("binary") != "recoil":
            continue
        for relationship in owner.get("relationships", ()):
            if not isinstance(relationship, Mapping) or "address" not in relationship:
                continue
            try:
                relationship_address = address_value(relationship.get("address"))
            except (TypeError, ValueError):
                continue
            if address <= relationship_address < end_exclusive:
                conflicts.append(
                    f"owner-relationship:{existing_owner_id}:{relationship.get('kind')}"
                )
    if conflicts:
        raise ProviderTargetMutationError(
            "provider target extent overlaps or conflicts with current tracker state: "
            + str(sorted(set(conflicts)))
        )


def _pending_symbol_state() -> dict[str, Any]:
    return {
        dimension: state_record(
            result="pending",
            disposition="claim",
            freshness="current-unhashed",
            evidence_ids=(),
        )
        for dimension in SYMBOL_BINARY_DIMENSIONS
    }


def _pending_storage_state() -> dict[str, Any]:
    return {
        dimension: state_record(
            result="pending",
            disposition="claim",
            freshness="current-unhashed",
            evidence_ids=(),
        )
        for dimension in STORAGE_DIMENSIONS
    }


def _legacy_conflict(entity: str, field: str, actual: Any, expected: Any) -> None:
    raise ProviderTargetMutationError(
        f"legacy provider {entity} conflict for {field}: "
        f"found {actual!r}, expected {expected!r}"
    )


def _require_exact_legacy_field(
    row: Mapping[str, Any],
    *,
    entity: str,
    field: str,
    expected: Any,
    missing_allowed: bool = False,
) -> None:
    if field not in row:
        if missing_allowed:
            return
        _legacy_conflict(entity, field, None, expected)
    if row.get(field) != expected:
        _legacy_conflict(entity, field, row.get(field), expected)


def _require_missing_or_exact_legacy_field(
    row: Mapping[str, Any],
    *,
    entity: str,
    field: str,
    expected: Any,
) -> None:
    _require_exact_legacy_field(
        row,
        entity=entity,
        field=field,
        expected=expected,
        missing_allowed=True,
    )


def _append_evidence(row: dict[str, Any], evidence_id: str) -> None:
    current = row.get("evidence_ids", [])
    if not isinstance(current, list) or any(not isinstance(item, str) for item in current):
        _legacy_conflict("record", "evidence_ids", current, "a list of evidence ids")
    if evidence_id not in current:
        row["evidence_ids"] = [*current, evidence_id]


def _legacy_import_fields_are_complete(
    row: Mapping[str, Any],
    *,
    retail: RetailImportTarget,
    object_symbol: str | None,
) -> bool:
    if row.get("import_dll") != retail.dll or row.get("import_name") != retail.import_name:
        return False
    if retail.import_ordinal is None:
        if "import_ordinal" in row:
            return False
    elif row.get("import_ordinal") != retail.import_ordinal:
        return False
    return object_symbol is None or row.get("object_symbol") == object_symbol


def _legacy_completion_is_current(
    *,
    owner: Mapping[str, Any],
    function: Mapping[str, Any],
    data: Mapping[str, Any] | None,
    storage: Mapping[str, Any] | None,
    retail: RetailImportTarget,
    object_symbol: str,
    data_id: str,
    storage_id: str,
    end_exclusive: int,
) -> bool:
    if data is None or storage is None:
        return False
    expected_end = normalize_address(end_exclusive)
    expected_data_name = f"{retail.dll}!{retail.import_name} IAT storage"
    expected_relationship_name = f"{retail.dll}!{retail.import_name} IAT"
    address_metadata_rows = owner.get("address_metadata")
    address_metadata = (
        address_metadata_rows.get(retail.address)
        if isinstance(address_metadata_rows, Mapping)
        else None
    )
    primary_data = [
        item
        for item in owner.get("relationships", ())
        if isinstance(item, Mapping) and item.get("kind") == "primary-data"
    ]
    extent_verification = storage.get("verification", {}).get("extent")
    return (
        owner.get("blocker") == "none"
        and isinstance(address_metadata, Mapping)
        and address_metadata.get("target") == "accepted"
        and len(primary_data) == 1
        and primary_data[0].get("address") == retail.address
        and primary_data[0].get("symbol_id") == data_id
        and primary_data[0].get("name") == expected_relationship_name
        and _legacy_import_fields_are_complete(
            function,
            retail=retail,
            object_symbol=object_symbol,
        )
        and function.get("authored_order_role") == "non-authored"
        and _legacy_import_fields_are_complete(data, retail=retail, object_symbol=None)
        and data.get("extent_state") == "known"
        and data.get("end_exclusive") == expected_end
        and data.get("size") == PE32_THUNK_SIZE
        and data.get("navigation_name") == expected_data_name
        and data.get("ownership_state") == "primary-owned"
        and data.get("storage_contribution_ids") == [storage_id]
        and storage.get("kind") == "provider-data"
        and storage.get("reference", {}).get("address") == retail.address
        and storage.get("reference", {}).get("end_exclusive") == expected_end
        and storage.get("reference", {}).get("extent_state") == "known"
        and storage.get("reference", {}).get("size") == PE32_THUNK_SIZE
        and isinstance(extent_verification, Mapping)
        and extent_verification.get("result") == "passed"
        and extent_verification.get("disposition") == "accepted"
        and extent_verification.get("freshness") == "current"
        and extent_verification.get("validation_mode") == "live"
    )


def _validate_legacy_owner(
    row: Any,
    *,
    owner_id: str,
    owner_name: str,
    address: str,
    function_id: str,
    data_id: str,
    dll: str,
    import_name: str,
) -> tuple[list[dict[str, Any]], int | None]:
    if not isinstance(row, Mapping):
        _legacy_conflict("owner", "record", row, "an object")
    entity = f"owner {owner_id}"
    for field, expected in (
        ("binary", "recoil"),
        ("kind", "provider-boundary"),
        ("provider_state", "accepted"),
        ("lifecycle_state", "accepted"),
        ("name", owner_name),
        ("legacy_id", owner_id.split("recoil:owner:", 1)[-1]),
    ):
        _require_exact_legacy_field(row, entity=entity, field=field, expected=expected)
    if row.get("blocker") not in ("pending", "none"):
        _legacy_conflict(entity, "blocker", row.get("blocker"), "pending or none")
    gates = row.get("gates")
    if not isinstance(gates, Mapping):
        _legacy_conflict(entity, "gates", gates, "accepted provider gates")
    for field in ("boundary", "data", "source"):
        _require_exact_legacy_field(
            gates,
            entity=f"{entity} gates",
            field=field,
            expected="accepted",
        )
    address_metadata = row.get("address_metadata")
    if not isinstance(address_metadata, Mapping):
        _legacy_conflict(entity, "address_metadata", address_metadata, "an object")
    metadata = address_metadata.get(address)
    if not isinstance(metadata, Mapping):
        _legacy_conflict(entity, f"address_metadata.{address}", metadata, "an object")
    _require_exact_legacy_field(
        metadata,
        entity=f"{entity} address metadata",
        field="group",
        expected="provider.imports",
    )
    _require_exact_legacy_field(
        metadata,
        entity=f"{entity} address metadata",
        field="name",
        expected=import_name,
    )
    if metadata.get("target") not in ("pending", "accepted"):
        _legacy_conflict(
            f"{entity} address metadata",
            "target",
            metadata.get("target"),
            "pending or accepted",
        )

    relationships = row.get("relationships")
    if not isinstance(relationships, list):
        _legacy_conflict(entity, "relationships", relationships, "a list")
    expected = {
        "anchor-address": (address, None),
        "primary-function": (address, function_id),
    }
    for kind, (expected_address, expected_symbol_id) in expected.items():
        matches = [
            item
            for item in relationships
            if isinstance(item, Mapping) and item.get("kind") == kind
        ]
        if len(matches) != 1:
            _legacy_conflict(entity, f"relationship {kind}", len(matches), 1)
        _require_exact_legacy_field(
            matches[0],
            entity=f"{entity} relationship {kind}",
            field="address",
            expected=expected_address,
        )
        if expected_symbol_id is not None:
            _require_exact_legacy_field(
                matches[0],
                entity=f"{entity} relationship {kind}",
                field="symbol_id",
                expected=expected_symbol_id,
            )

    primary_data_indexes = [
        index
        for index, item in enumerate(relationships)
        if isinstance(item, Mapping) and item.get("kind") == "primary-data"
    ]
    if len(primary_data_indexes) > 1:
        _legacy_conflict(entity, "relationship primary-data", len(primary_data_indexes), "0 or 1")
    if primary_data_indexes:
        item = relationships[primary_data_indexes[0]]
        _require_exact_legacy_field(
            item,
            entity=f"{entity} relationship primary-data",
            field="address",
            expected=address,
        )
        _require_exact_legacy_field(
            item,
            entity=f"{entity} relationship primary-data",
            field="symbol_id",
            expected=data_id,
        )
        if item.get("name") not in (
            None,
            "pending",
            f"{dll}!{import_name} IAT",
        ):
            _legacy_conflict(
                f"{entity} relationship primary-data",
                "name",
                item.get("name"),
                f"{dll}!{import_name} IAT",
            )
    return [dict(item) if isinstance(item, Mapping) else item for item in relationships], (
        primary_data_indexes[0] if primary_data_indexes else None
    )


def _validate_legacy_function(
    row: Any,
    *,
    function_id: str,
    address: str,
    end_exclusive: int,
    output_section_id: str,
    retail: RetailImportTarget,
    object_symbol: str,
    owner_name: str,
) -> None:
    if not isinstance(row, Mapping):
        _legacy_conflict("function", "record", row, "an object")
    entity = f"function {function_id}"
    for field, expected in (
        ("binary", "recoil"),
        ("kind", "provider-function"),
        ("address", address),
        ("end_exclusive", normalize_address(address_value(address) + 1)),
        ("extent_state", "known"),
        ("size", 1),
        ("disposition", "provider"),
        ("output_section_id", output_section_id),
        ("ownership_state", "primary-owned"),
        ("pipeline_class", "non-authored"),
    ):
        _require_exact_legacy_field(row, entity=entity, field=field, expected=expected)
    if row.get("authored_order_role") not in (None, "non-authored"):
        _legacy_conflict(
            entity,
            "authored_order_role",
            row.get("authored_order_role"),
            "non-authored",
        )
    if row.get("navigation_name") not in (
        owner_name,
        f"{retail.dll}!{retail.import_name} import provider",
    ):
        _legacy_conflict(
            entity,
            "navigation_name",
            row.get("navigation_name"),
            owner_name,
        )
    if address_value(str(row["end_exclusive"])) > end_exclusive:
        _legacy_conflict(entity, "extent", row.get("end_exclusive"), "one-byte function marker")
    for field, expected in (
        ("import_dll", retail.dll),
        ("import_name", retail.import_name),
        ("object_symbol", object_symbol),
    ):
        _require_missing_or_exact_legacy_field(
            row,
            entity=entity,
            field=field,
            expected=expected,
        )
    if retail.import_ordinal is None:
        if "import_ordinal" in row:
            _legacy_conflict(entity, "import_ordinal", row.get("import_ordinal"), None)
    else:
        _require_missing_or_exact_legacy_field(
            row,
            entity=entity,
            field="import_ordinal",
            expected=retail.import_ordinal,
        )


def _validate_legacy_data(
    row: Any,
    *,
    data_id: str,
    address: str,
    end_exclusive: int,
    output_section_id: str,
    storage_id: str,
    retail: RetailImportTarget,
) -> None:
    if row is None:
        return
    if not isinstance(row, Mapping):
        _legacy_conflict("data", "record", row, "an object")
    entity = f"data {data_id}"
    for field, expected in (
        ("binary", "recoil"),
        ("kind", "data"),
        ("address", address),
        ("output_section_id", output_section_id),
    ):
        _require_exact_legacy_field(row, entity=entity, field=field, expected=expected)
    if row.get("disposition") not in (None, "provider"):
        _legacy_conflict(entity, "disposition", row.get("disposition"), "provider")
    if row.get("ownership_state") not in (None, "primary-owned"):
        _legacy_conflict(entity, "ownership_state", row.get("ownership_state"), "primary-owned")
    extent_state = row.get("extent_state")
    if extent_state not in ("unknown", "known"):
        _legacy_conflict(entity, "extent_state", extent_state, "unknown or known")
    if extent_state == "known":
        _require_exact_legacy_field(
            row,
            entity=entity,
            field="end_exclusive",
            expected=normalize_address(end_exclusive),
        )
        _require_exact_legacy_field(
            row,
            entity=entity,
            field="size",
            expected=PE32_THUNK_SIZE,
        )
    elif "end_exclusive" in row or "size" in row:
        _legacy_conflict(entity, "extent", row, "unknown without explicit extent")
    storage_ids = row.get("storage_contribution_ids", [])
    if storage_ids not in ([], [storage_id]):
        _legacy_conflict(
            entity,
            "storage_contribution_ids",
            storage_ids,
            [storage_id],
        )
    navigation_name = row.get("navigation_name")
    expected_navigation = f"{retail.dll}!{retail.import_name} IAT storage"
    if navigation_name not in (None, "pending", expected_navigation):
        _legacy_conflict(entity, "navigation_name", navigation_name, expected_navigation)
    for field, expected in (("import_dll", retail.dll), ("import_name", retail.import_name)):
        _require_missing_or_exact_legacy_field(
            row,
            entity=entity,
            field=field,
            expected=expected,
        )
    if retail.import_ordinal is None:
        if "import_ordinal" in row:
            _legacy_conflict(entity, "import_ordinal", row.get("import_ordinal"), None)
    else:
        _require_missing_or_exact_legacy_field(
            row,
            entity=entity,
            field="import_ordinal",
            expected=retail.import_ordinal,
        )


def _validate_legacy_storage(
    row: Any,
    *,
    storage_id: str,
    owner_id: str,
    data_id: str,
    address: str,
    end_exclusive: int,
    output_section_id: str,
) -> None:
    if row is None:
        return
    if not isinstance(row, Mapping):
        _legacy_conflict("storage", "record", row, "an object")
    entity = f"storage {storage_id}"
    for field, expected in (
        ("binary", "recoil"),
        ("output_section_id", output_section_id),
        ("overlap", "none"),
        ("owner_ids", [owner_id]),
        ("symbol_ids", [data_id]),
        ("parent_contribution_id", None),
    ):
        _require_exact_legacy_field(row, entity=entity, field=field, expected=expected)
    if row.get("kind") not in ("data-symbol", "provider-data"):
        _legacy_conflict(entity, "kind", row.get("kind"), "data-symbol or provider-data")
    applicability = row.get("applicability")
    if not isinstance(applicability, Mapping):
        _legacy_conflict(entity, "applicability", applicability, "an object")
    for dimension in STORAGE_DIMENSIONS:
        _require_exact_legacy_field(
            applicability,
            entity=f"{entity} applicability",
            field=dimension,
            expected=True,
        )
    reference = row.get("reference")
    if not isinstance(reference, Mapping):
        _legacy_conflict(entity, "reference", reference, "an object")
    _require_exact_legacy_field(
        reference,
        entity=f"{entity} reference",
        field="address",
        expected=address,
    )
    extent_state = reference.get("extent_state")
    if extent_state not in ("unknown", "known"):
        _legacy_conflict(
            f"{entity} reference",
            "extent_state",
            extent_state,
            "unknown or known",
        )
    if extent_state == "known":
        _require_exact_legacy_field(
            reference,
            entity=f"{entity} reference",
            field="end_exclusive",
            expected=normalize_address(end_exclusive),
        )
        _require_exact_legacy_field(
            reference,
            entity=f"{entity} reference",
            field="size",
            expected=PE32_THUNK_SIZE,
        )
    elif "end_exclusive" in reference or "size" in reference:
        _legacy_conflict(
            f"{entity} reference",
            "extent",
            reference,
            "unknown without explicit extent",
        )
    verification = row.get("verification")
    if not isinstance(verification, Mapping):
        _legacy_conflict(entity, "verification", verification, "an object")
    for dimension in STORAGE_DIMENSIONS:
        if not isinstance(verification.get(dimension), Mapping):
            _legacy_conflict(
                entity,
                f"verification.{dimension}",
                verification.get(dimension),
                "a state record",
            )


def _legacy_data_record(
    *,
    address: str,
    end_exclusive: int,
    output_section_id: str,
    storage_id: str,
    retail: RetailImportTarget,
) -> dict[str, Any]:
    data = {
        "accepted_byte_facts": None,
        "accepted_order_facts": None,
        "address": address,
        "binary": "recoil",
        "binary_state": _pending_symbol_state(),
        "binary_state_diagnostics": {
            "legacy_order": state_record(
                result="pending",
                disposition="claim",
                freshness="current-unhashed",
                evidence_ids=(),
            )
        },
        "disposition": "provider",
        "end_exclusive": normalize_address(end_exclusive),
        "evidence_ids": [],
        "extent_state": "known",
        "import_dll": retail.dll,
        "import_name": retail.import_name,
        "kind": "data",
        "navigation_name": f"{retail.dll}!{retail.import_name} IAT storage",
        "output_section_id": output_section_id,
        "ownership_state": "primary-owned",
        "physical_block_id": None,
        "semantic_span_ids": [],
        "size": PE32_THUNK_SIZE,
        "storage_contribution_ids": [storage_id],
        "verification_target_ids": [],
    }
    if retail.import_ordinal is not None:
        data["import_ordinal"] = retail.import_ordinal
    return data


def _legacy_storage_record(
    *,
    owner_id: str,
    data_id: str,
    address: str,
    end_exclusive: int,
    output_section_id: str,
) -> dict[str, Any]:
    return {
        "applicability": {dimension: True for dimension in STORAGE_DIMENSIONS},
        "binary": "recoil",
        "candidate": {"evidence_ids": [], "state": "missing"},
        "evidence_ids": [],
        "kind": "provider-data",
        "output_section_id": output_section_id,
        "overlap": "none",
        "owner_ids": [owner_id],
        "parent_contribution_id": None,
        "reference": {
            "address": address,
            "end_exclusive": normalize_address(end_exclusive),
            "evidence_ids": [],
            "extent_state": "known",
            "size": PE32_THUNK_SIZE,
        },
        "symbol_ids": [data_id],
        "verification": _pending_storage_state(),
    }


def _complete_legacy_provider_target(
    *,
    store: ProgressStore,
    document: ProgressDocument,
    reference: Path,
    request: Mapping[str, Any],
    retail: RetailImportTarget,
    directory_context: Mapping[str, Any],
    output_section_id: str,
    normalized_address: str,
    start: int,
    end_exclusive: int,
    owner_id: str,
    function_id: str,
    data_id: str,
    storage_id: str,
    expected_revision: int,
    apply: bool,
) -> dict[str, Any]:
    owners = document.collection("owners")
    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    owner_source = owners.get(owner_id)
    function_source = symbols.get(function_id)
    data_source = symbols.get(data_id)
    storage_source = storage_rows.get(storage_id)

    relationships, primary_data_index = _validate_legacy_owner(
        owner_source,
        owner_id=owner_id,
        owner_name=str(request["owner_name"]),
        address=normalized_address,
        function_id=function_id,
        data_id=data_id,
        dll=retail.dll,
        import_name=retail.import_name,
    )
    _validate_legacy_function(
        function_source,
        function_id=function_id,
        address=normalized_address,
        end_exclusive=end_exclusive,
        output_section_id=output_section_id,
        retail=retail,
        object_symbol=str(request["object_symbol"]),
        owner_name=str(request["owner_name"]),
    )
    _validate_legacy_data(
        data_source,
        data_id=data_id,
        address=normalized_address,
        end_exclusive=end_exclusive,
        output_section_id=output_section_id,
        storage_id=storage_id,
        retail=retail,
    )
    _validate_legacy_storage(
        storage_source,
        storage_id=storage_id,
        owner_id=owner_id,
        data_id=data_id,
        address=normalized_address,
        end_exclusive=end_exclusive,
        output_section_id=output_section_id,
    )
    _reject_existing_or_overlapping(
        document,
        owner_id=owner_id,
        address=start,
        end_exclusive=end_exclusive,
        function_id=function_id,
        data_id=data_id,
        storage_id=storage_id,
        allow_exact_legacy_ids=True,
    )

    assert isinstance(owner_source, Mapping)
    assert isinstance(function_source, Mapping)
    if _legacy_completion_is_current(
        owner=owner_source,
        function=function_source,
        data=data_source if isinstance(data_source, Mapping) else None,
        storage=storage_source if isinstance(storage_source, Mapping) else None,
        retail=retail,
        object_symbol=str(request["object_symbol"]),
        data_id=data_id,
        storage_id=storage_id,
        end_exclusive=end_exclusive,
    ):
        try:
            # Take the same revision lock/CAS path as a mutation while keeping
            # an already-current completion strictly non-mutating.
            store.commit(
                document.data,
                expected_revision=expected_revision,
                apply=False,
            )
        except (ConcurrentProgressUpdate, ProgressError) as exc:
            raise ProviderTargetMutationError(str(exc)) from exc
        commit = CommitResult(
            applied=False,
            path=store.path,
            previous_revision=document.revision,
            revision=document.revision,
        )
        return {
            "report_version": 1,
            "kind": "provider-target-registration-mutation",
            "operation": "legacy-provider-completion",
            "completion_state": "already-current",
            "idempotent": True,
            "validation_mode": "immutable-retail-plus-reviewed-provider-identity",
            "candidate_independent": True,
            "reference": display_path(reference),
            "request": dict(request),
            "retail_import": {
                **asdict(retail),
                **dict(directory_context),
                "storage_end_exclusive": normalize_address(end_exclusive),
                "output_section_id": output_section_id,
            },
            "entity_ids": {
                "owner_id": owner_id,
                "function_symbol_id": function_id,
                "data_symbol_id": data_id,
                "storage_contribution_id": storage_id,
                "evidence_id": None,
            },
            "records": {
                "owner": deepcopy(dict(owner_source)),
                "function": deepcopy(dict(function_source)),
                "data": deepcopy(dict(data_source)),
                "storage": deepcopy(dict(storage_source)),
                "evidence": None,
            },
            "commit": commit.to_dict(),
        }

    proposed = deepcopy(document.data)
    owner = deepcopy(dict(owner_source))
    function = deepcopy(dict(function_source))
    data = (
        deepcopy(dict(data_source))
        if isinstance(data_source, Mapping)
        else _legacy_data_record(
            address=normalized_address,
            end_exclusive=end_exclusive,
            output_section_id=output_section_id,
            storage_id=storage_id,
            retail=retail,
        )
    )
    storage = (
        deepcopy(dict(storage_source))
        if isinstance(storage_source, Mapping)
        else _legacy_storage_record(
            owner_id=owner_id,
            data_id=data_id,
            address=normalized_address,
            end_exclusive=end_exclusive,
            output_section_id=output_section_id,
        )
    )
    expected_relationship = {
        "kind": "primary-data",
        "address": normalized_address,
        "symbol_id": data_id,
        "name": f"{retail.dll}!{retail.import_name} IAT",
    }
    if primary_data_index is None:
        relationships.append(expected_relationship)
    else:
        completed_relationship = deepcopy(relationships[primary_data_index])
        completed_relationship.update(expected_relationship)
        relationships[primary_data_index] = completed_relationship
    owner["relationships"] = relationships
    owner["blocker"] = "none"
    owner_address_metadata = deepcopy(dict(owner.get("address_metadata", {})))
    if normalized_address in owner_address_metadata:
        completed_metadata = deepcopy(dict(owner_address_metadata[normalized_address]))
        completed_metadata["target"] = "accepted"
        owner_address_metadata[normalized_address] = completed_metadata
        owner["address_metadata"] = owner_address_metadata

    function["authored_order_role"] = "non-authored"
    function["import_dll"] = retail.dll
    function["import_name"] = retail.import_name
    function["object_symbol"] = request["object_symbol"]
    if retail.import_ordinal is not None:
        function["import_ordinal"] = retail.import_ordinal

    data.update(
        {
            "disposition": "provider",
            "end_exclusive": normalize_address(end_exclusive),
            "extent_state": "known",
            "import_dll": retail.dll,
            "import_name": retail.import_name,
            "navigation_name": f"{retail.dll}!{retail.import_name} IAT storage",
            "ownership_state": "primary-owned",
            "size": PE32_THUNK_SIZE,
            "storage_contribution_ids": [storage_id],
        }
    )
    if retail.import_ordinal is not None:
        data["import_ordinal"] = retail.import_ordinal

    storage["kind"] = "provider-data"
    storage_reference = deepcopy(dict(storage["reference"]))
    storage_reference.update(
        {
            "address": normalized_address,
            "end_exclusive": normalize_address(end_exclusive),
            "extent_state": "known",
            "size": PE32_THUNK_SIZE,
        }
    )
    storage["reference"] = storage_reference

    scope_ids = [owner_id, function_id, data_id, storage_id]
    provenance = {
        "reference": display_path(reference),
        "producer": "pe32-import-directory",
        "candidate_independent": True,
        "operation": "legacy-provider-completion",
        "address": normalized_address,
        "dll": retail.dll,
        "import_name": retail.import_name,
        "import_ordinal": retail.import_ordinal,
        "descriptor_index": retail.descriptor_index,
        "thunk_index": retail.thunk_index,
        "iat_rva": retail.iat_rva,
        "iat_end_rva": retail.iat_end_rva,
        "object_symbol": request["object_symbol"],
        "object_symbol_basis": "reviewed-vc5-provider-declaration",
    }
    try:
        evidence_id = add_live_evidence(
            proposed,
            kind="provider-target-legacy-completion",
            summary=(
                f"Immutable retail import directory completes accepted legacy provider "
                f"{owner_id} at {normalized_address} as "
                f"{retail.dll}!{retail.import_name}; reviewed VC5 provider declaration "
                f"supplies exact COFF identity {request['object_symbol']} and the retail "
                f"IAT proves the exact {PE32_THUNK_SIZE}-byte data/storage extent."
            ),
            scope_ids=scope_ids,
            provenance=provenance,
        )
    except ProgressError as exc:
        raise ProviderTargetMutationError(str(exc)) from exc

    for row in (owner, function, data, storage):
        _append_evidence(row, evidence_id)
    reference_record = deepcopy(dict(storage["reference"]))
    reference_evidence = reference_record.get("evidence_ids", [])
    if not isinstance(reference_evidence, list):
        _legacy_conflict("storage reference", "evidence_ids", reference_evidence, "a list")
    reference_record["evidence_ids"] = [*reference_evidence, evidence_id]
    storage["reference"] = reference_record
    verification = deepcopy(dict(storage["verification"]))
    extent_state = deepcopy(dict(verification["extent"]))
    extent_evidence = extent_state.get("evidence_ids", [])
    if not isinstance(extent_evidence, list):
        _legacy_conflict(
            "storage extent verification",
            "evidence_ids",
            extent_evidence,
            "a list",
        )
    extent_state.update(
        state_record(
            result="passed",
            disposition="accepted",
            freshness="current",
            evidence_ids=[*extent_evidence, evidence_id],
            gating=True,
            validation_mode="live",
        )
    )
    verification["extent"] = extent_state
    storage["verification"] = verification

    proposed["owners"][owner_id] = owner
    proposed["symbols"][function_id] = function
    proposed["symbols"][data_id] = data
    proposed["storage_contributions"][storage_id] = storage
    try:
        commit = store.commit(
            proposed,
            expected_revision=expected_revision,
            apply=apply,
        )
    except (ConcurrentProgressUpdate, ProgressError) as exc:
        raise ProviderTargetMutationError(str(exc)) from exc
    return {
        "report_version": 1,
        "kind": "provider-target-registration-mutation",
        "operation": "legacy-provider-completion",
        "completion_state": "completed",
        "idempotent": False,
        "validation_mode": "immutable-retail-plus-reviewed-provider-identity",
        "candidate_independent": True,
        "reference": display_path(reference),
        "request": dict(request),
        "retail_import": {
            **asdict(retail),
            **dict(directory_context),
            "storage_end_exclusive": normalize_address(end_exclusive),
            "output_section_id": output_section_id,
        },
        "entity_ids": {
            "owner_id": owner_id,
            "function_symbol_id": function_id,
            "data_symbol_id": data_id,
            "storage_contribution_id": storage_id,
            "evidence_id": evidence_id,
        },
        "records": {
            "owner": owner,
            "function": function,
            "data": data,
            "storage": storage,
            "evidence": proposed["evidence"][evidence_id],
        },
        "commit": commit.to_dict(),
    }


def register_provider_target(
    *,
    progress: Path,
    reference: Path,
    address: str,
    payload: Mapping[str, Any],
    expected_revision: int,
    apply: bool,
) -> dict[str, Any]:
    request = normalize_provider_target_request(payload)
    normalized_address = normalize_address(address)
    start = address_value(normalized_address)
    end_exclusive = start + PE32_THUNK_SIZE
    retail, directory_context = retail_import_target(
        reference=reference,
        address=normalized_address,
        dll=str(request["dll"]),
        import_name=str(request["import_name"]),
        import_ordinal=request.get("import_ordinal"),
    )

    store = ProgressStore(progress)
    try:
        document = store.load()
    except ProgressError as exc:
        raise ProviderTargetMutationError(str(exc)) from exc
    if document.revision != expected_revision:
        raise ProviderTargetMutationError(
            f"revision changed: expected {expected_revision}, found {document.revision}"
        )
    output_section_id = _retail_output_section(
        document,
        reference=reference,
        start=start,
        end_exclusive=end_exclusive,
    )

    owner_id = str(request["owner_id"])
    function_id = f"recoil:function:{normalized_address}"
    data_id = f"recoil:data:{normalized_address}"
    storage_id = f"recoil:storage:va:{normalized_address}"
    if owner_id in document.collection("owners"):
        try:
            return _complete_legacy_provider_target(
                store=store,
                document=document,
                reference=reference,
                request=request,
                retail=retail,
                directory_context=directory_context,
                output_section_id=output_section_id,
                normalized_address=normalized_address,
                start=start,
                end_exclusive=end_exclusive,
                owner_id=owner_id,
                function_id=function_id,
                data_id=data_id,
                storage_id=storage_id,
                expected_revision=expected_revision,
                apply=apply,
            )
        except ProviderTargetMutationError as exc:
            raise ProviderTargetMutationError(
                f"provider owner already exists and exact legacy completion failed: {exc}"
            ) from exc
    _reject_existing_or_overlapping(
        document,
        owner_id=owner_id,
        address=start,
        end_exclusive=end_exclusive,
        function_id=function_id,
        data_id=data_id,
        storage_id=storage_id,
    )

    proposed = deepcopy(document.data)
    scope_ids = [owner_id, function_id, data_id, storage_id]
    provenance = {
        "reference": display_path(reference),
        "producer": "pe32-import-directory",
        "candidate_independent": True,
        "address": normalized_address,
        "dll": retail.dll,
        "import_name": retail.import_name,
        "import_ordinal": retail.import_ordinal,
        "descriptor_index": retail.descriptor_index,
        "thunk_index": retail.thunk_index,
        "iat_rva": retail.iat_rva,
        "iat_end_rva": retail.iat_end_rva,
        "object_symbol": request["object_symbol"],
        "object_symbol_basis": "reviewed-vc5-provider-declaration",
    }
    try:
        evidence_id = add_live_evidence(
            proposed,
            kind="provider-target-registration",
            summary=(
                f"Immutable retail import directory proves {normalized_address} as "
                f"{retail.dll}!{retail.import_name}; reviewed VC5 provider declaration "
                f"supplies exact COFF identity {request['object_symbol']}."
            ),
            scope_ids=scope_ids,
            provenance=provenance,
        )
    except ProgressError as exc:
        raise ProviderTargetMutationError(str(exc)) from exc

    owner = {
        "address_metadata": {
            normalized_address: {
                "group": "provider.imports",
                "name": retail.import_name,
                "target": "accepted",
            }
        },
        "binary": "recoil",
        "blocker": "none",
        "evidence_ids": [evidence_id],
        "gates": {
            "boundary": "accepted",
            "byte": "deferred",
            "data": "accepted",
            "owner_linkage": "none",
            "source": "accepted",
        },
        "kind": "provider-boundary",
        "legacy_id": owner_id.split("recoil:owner:", 1)[-1],
        "lifecycle_state": "accepted",
        "name": request["owner_name"],
        "provider_state": "accepted",
        "reimplementation": {"entries": {}},
        "relationships": [
            {"kind": "anchor-address", "address": normalized_address},
            {
                "kind": "primary-function",
                "address": normalized_address,
                "symbol_id": function_id,
            },
            {
                "kind": "primary-data",
                "address": normalized_address,
                "symbol_id": data_id,
                "name": f"{retail.dll}!{retail.import_name} IAT",
            },
        ],
        "section": "provider_platform",
        "source_paths": [],
    }
    function = {
        "accepted_byte_facts": None,
        "accepted_order_facts": None,
        "address": normalized_address,
        "authored_order_role": "non-authored",
        "binary": "recoil",
        "binary_state": _pending_symbol_state(),
        "binary_state_diagnostics": {
            "legacy_order": state_record(
                result="pending",
                disposition="claim",
                freshness="current-unhashed",
                evidence_ids=(),
            )
        },
        "disposition": "provider",
        "end_exclusive": normalize_address(start + 1),
        "evidence_ids": [evidence_id],
        "extent_state": "known",
        "import_dll": retail.dll,
        "import_name": retail.import_name,
        "kind": "provider-function",
        "navigation_name": f"{retail.dll}!{retail.import_name} import provider",
        "object_symbol": request["object_symbol"],
        "output_section_id": output_section_id,
        "ownership_state": "primary-owned",
        "physical_block_id": None,
        "pipeline_class": "non-authored",
        "semantic_span_ids": [],
        "size": 1,
        "storage_contribution_ids": [],
        "verification_target_ids": [],
    }
    if retail.import_ordinal is not None:
        function["import_ordinal"] = retail.import_ordinal
    data = {
        "accepted_byte_facts": None,
        "accepted_order_facts": None,
        "address": normalized_address,
        "binary": "recoil",
        "binary_state": _pending_symbol_state(),
        "binary_state_diagnostics": {
            "legacy_order": state_record(
                result="pending",
                disposition="claim",
                freshness="current-unhashed",
                evidence_ids=(),
            )
        },
        "disposition": "provider",
        "end_exclusive": normalize_address(end_exclusive),
        "evidence_ids": [evidence_id],
        "extent_state": "known",
        "import_dll": retail.dll,
        "import_name": retail.import_name,
        "kind": "data",
        "navigation_name": f"{retail.dll}!{retail.import_name} IAT storage",
        "output_section_id": output_section_id,
        "ownership_state": "primary-owned",
        "physical_block_id": None,
        "semantic_span_ids": [],
        "size": PE32_THUNK_SIZE,
        "storage_contribution_ids": [storage_id],
        "verification_target_ids": [],
    }
    if retail.import_ordinal is not None:
        data["import_ordinal"] = retail.import_ordinal
    storage_verification = _pending_storage_state()
    storage_verification["extent"] = state_record(
        result="passed",
        disposition="accepted",
        freshness="current",
        evidence_ids=[evidence_id],
        gating=True,
        validation_mode="live",
    )
    storage = {
        "applicability": {dimension: True for dimension in STORAGE_DIMENSIONS},
        "binary": "recoil",
        "candidate": {"evidence_ids": [], "state": "missing"},
        "evidence_ids": [evidence_id],
        "kind": "provider-data",
        "output_section_id": output_section_id,
        "overlap": "none",
        "owner_ids": [owner_id],
        "parent_contribution_id": None,
        "reference": {
            "address": normalized_address,
            "end_exclusive": normalize_address(end_exclusive),
            "evidence_ids": [evidence_id],
            "extent_state": "known",
            "size": PE32_THUNK_SIZE,
        },
        "symbol_ids": [data_id],
        "verification": storage_verification,
    }

    proposed["owners"][owner_id] = owner
    proposed["symbols"][function_id] = function
    proposed["symbols"][data_id] = data
    proposed["storage_contributions"][storage_id] = storage
    try:
        commit = store.commit(
            proposed,
            expected_revision=expected_revision,
            apply=apply,
        )
    except (ConcurrentProgressUpdate, ProgressError) as exc:
        raise ProviderTargetMutationError(str(exc)) from exc
    return {
        "report_version": 1,
        "kind": "provider-target-registration-mutation",
        "validation_mode": "immutable-retail-plus-reviewed-provider-identity",
        "candidate_independent": True,
        "reference": display_path(reference),
        "request": request,
        "retail_import": {
            **asdict(retail),
            **directory_context,
            "storage_end_exclusive": normalize_address(end_exclusive),
            "output_section_id": output_section_id,
        },
        "entity_ids": {
            "owner_id": owner_id,
            "function_symbol_id": function_id,
            "data_symbol_id": data_id,
            "storage_contribution_id": storage_id,
            "evidence_id": evidence_id,
        },
        "records": {
            "owner": owner,
            "function": function,
            "data": data,
            "storage": storage,
            "evidence": proposed["evidence"][evidence_id],
        },
        "commit": commit.to_dict(),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Register one immutable-retail named or reviewed ordinal-function IAT slot as an "
            "accepted provider boundary with exact typed function, data, and storage identities."
        )
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    child = subparsers.add_parser("register")
    child.add_argument("--address", required=True)
    child.add_argument("--payload-json", required=True)
    child.add_argument("--progress", type=Path, default=DEFAULT_TRACKER)
    child.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    child.add_argument("--expected-revision", type=int, required=True)
    mode = child.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    child.add_argument("--json", action="store_true")
    return parser


def run(args: argparse.Namespace) -> dict[str, Any]:
    if args.command != "register":
        raise ProviderTargetMutationError(f"unsupported operation {args.command!r}")
    return register_provider_target(
        progress=args.progress,
        reference=args.reference,
        address=args.address,
        payload=_payload(args.payload_json),
        expected_revision=args.expected_revision,
        apply=bool(args.apply),
    )


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        report = run(args)
    except (OSError, ValueError, ProviderTargetMutationError) as exc:
        print(f"provider target mutation error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        mode = (
            "UNCHANGED"
            if report.get("completion_state") == "already-current"
            else ("APPLIED" if report["commit"]["applied"] else "DRY-RUN")
        )
        retail = report["retail_import"]
        print(
            f"Provider target {mode}: {retail['address']} "
            f"{retail['dll']}!{retail['import_name']}"
        )
        print(
            f"revision {report['commit']['previous_revision']} -> "
            f"{report['commit']['revision']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
