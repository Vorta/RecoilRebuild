from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import sys
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32
from _recoil.commands.relocation_expectations import (
    DEFAULT_REFERENCE,
    PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY,
    RelocationExpectationError,
    bind_reviewed_exception_context,
    decode_retail_relocation_site,
    decode_retail_target_sites,
    normalize_reviewed_exception,
    relocation_target_owner_context,
)
from _recoil.commands.relocation_target_mutation import (
    RelocationTargetMutationError,
    _pending_data_symbol,
    _retail_section,
    _target_candidates,
    _validate_owner_evidence,
)
from _recoil.lib.progress import (
    ConcurrentProgressUpdate,
    DEFAULT_PROGRESS_PATH,
    ProgressDocument,
    ProgressError,
    ProgressStore,
    address_value,
    normalize_address,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "vc5_verify_targets"


class RelocationExceptionMutationError(RuntimeError):
    pass


def _payload(value: str) -> dict[str, Any]:
    try:
        parsed = json.loads(value)
    except json.JSONDecodeError as exc:
        raise RelocationExceptionMutationError(f"invalid --payload-json: {exc}") from exc
    if not isinstance(parsed, dict):
        raise RelocationExceptionMutationError("--payload-json must decode to an object")
    return parsed


def _source_row(
    document: ProgressDocument,
    *,
    source_symbol_id: str,
    source_address: str,
) -> Mapping[str, Any]:
    row = document.collection("symbols").get(source_symbol_id)
    if not isinstance(row, Mapping) or row.get("binary") != "recoil":
        raise RelocationExceptionMutationError(
            f"unknown Recoil physical symbol {source_symbol_id!r}"
        )
    if row.get("kind") not in {"function", "provider-function", "compiler-function"}:
        raise RelocationExceptionMutationError(
            f"source physical symbol {source_symbol_id!r} is not a function row"
        )
    tracker_address = normalize_address(row.get("address"))
    requested_address = normalize_address(source_address)
    if tracker_address != requested_address:
        raise RelocationExceptionMutationError(
            f"source address {requested_address} does not match {source_symbol_id} at {tracker_address}"
        )
    if not isinstance(row.get("end_exclusive"), str):
        raise RelocationExceptionMutationError("source physical symbol has no known extent")
    if address_value(str(row["end_exclusive"])) <= address_value(tracker_address):
        raise RelocationExceptionMutationError("source physical symbol has an empty extent")
    return row


def prepare_reviewed_exception(
    *,
    document: ProgressDocument,
    bindings: Mapping[str, Sequence[Any]],
    source_symbol_id: str,
    source_address: str,
    payload: Mapping[str, Any],
    reference: Path = DEFAULT_REFERENCE,
) -> tuple[dict[str, Any], dict[str, Any]]:
    """Validate and enrich one reviewed exception without mutating tracker state."""
    source = _source_row(
        document,
        source_symbol_id=source_symbol_id,
        source_address=source_address,
    )
    try:
        normalized = normalize_reviewed_exception(payload)
        normalized = bind_reviewed_exception_context(
            normalized,
            document=document,
            bindings=bindings,
            source_symbol_id=source_symbol_id,
            reference=reference,
        )
        physical_mode = (
            normalized.get("exception_mode")
            == PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY
        )
        offsets = (
            tuple(int(item) for item in normalized["offsets"])
            if physical_mode
            else (int(normalized["offset"]),)
        )
        decoded_sites = tuple(
            decode_retail_relocation_site(
                row=source,
                offset=offset,
                relocation_type=int(normalized["type"]),
                reference=reference,
            )
            for offset in offsets
        )
    except (ProgressError, RelocationExpectationError, OSError, ValueError) as exc:
        raise RelocationExceptionMutationError(str(exc)) from exc

    retail_target = int(normalized["retail_target"])
    if any(int(decoded["retail_target"]) != retail_target for decoded in decoded_sites):
        raise RelocationExceptionMutationError(
            f"payload retail_target 0x{retail_target:x} does not match every "
            "immutable retail operand in the reviewed site group"
        )
    if physical_mode:
        try:
            all_target_sites = decode_retail_target_sites(
                row=source,
                retail_target=retail_target,
                reference=reference,
            )
        except (RelocationExpectationError, OSError, ValueError) as exc:
            raise RelocationExceptionMutationError(str(exc)) from exc
        decoded_population = tuple(
            (int(item["offset"]), int(item["type"])) for item in all_target_sites
        )
        requested_population = tuple(
            (offset, int(normalized["type"])) for offset in offsets
        )
        if decoded_population != requested_population:
            raise RelocationExceptionMutationError(
                "reviewed offsets/type do not equal the full immutable-retail "
                f"site population for target 0x{retail_target:x}: "
                f"expected {list(decoded_population)}, requested {list(requested_population)}"
            )
        target_binding = normalized["physical_target_binding"]
    else:
        target_binding = normalized["target_binding"]
    target_address = address_value(str(target_binding["address"]))
    target_end = address_value(str(target_binding["end_exclusive"]))
    if not target_address <= retail_target < target_end:
        raise RelocationExceptionMutationError(
            "immutable retail operand does not fall within the selected target symbol extent"
        )
    target_addend = retail_target - target_address
    if int(normalized["resolved_target_addend"]) != target_addend:
        raise RelocationExceptionMutationError(
            "resolved_target_addend does not match immutable retail target and tracker extent"
        )
    relocation_type = int(normalized["type"])
    if relocation_type == IMAGE_REL_I386_REL32:
        # IMAGE_REL_I386_REL32 links as S + A - (P + 4).  The decoded retail
        # target already accounts for P + 4, so the raw COFF field is A.
        expected_coff_addend = target_addend & 0xFFFFFFFF
    elif relocation_type == IMAGE_REL_I386_DIR32:
        expected_coff_addend = target_addend & 0xFFFFFFFF
    else:  # normalize_reviewed_exception already rejects this; keep the contract explicit.
        raise RelocationExceptionMutationError(
            f"unsupported relocation type 0x{relocation_type:04x}"
        )
    if int(normalized["coff_addend"]) != expected_coff_addend:
        raise RelocationExceptionMutationError(
            "coff_addend does not match immutable retail target and selected target extent"
        )
    decoded: dict[str, Any]
    if physical_mode:
        decoded = {
            "source_address": normalize_address(source["address"]),
            "source_end_exclusive": normalize_address(source["end_exclusive"]),
            "sites": [dict(item) for item in decoded_sites],
            "retail_target": retail_target,
        }
    else:
        decoded = dict(decoded_sites[0])
    return normalized, decoded


def _append_exception(
    data: dict[str, Any],
    *,
    source_symbol_id: str,
    normalized: Mapping[str, Any],
) -> None:
    symbols = data.get("symbols")
    row = symbols.get(source_symbol_id) if isinstance(symbols, dict) else None
    if not isinstance(row, dict):
        raise RelocationExceptionMutationError(
            f"source physical symbol {source_symbol_id!r} disappeared during mutation"
        )
    values = row.get("relocation_expectation_exceptions")
    if values is None:
        current: list[Any] = []
    elif isinstance(values, list):
        current = list(values)
    else:
        raise RelocationExceptionMutationError(
            "relocation_expectation_exceptions must be a list"
        )
    offsets = (
        tuple(int(item) for item in normalized["offsets"])
        if normalized.get("exception_mode")
        == PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY
        else (int(normalized["offset"]),)
    )
    keys = {
        (
            str(normalized["object_symbol"]),
            offset,
            int(normalized["type"]),
        )
        for offset in offsets
    }
    for index, existing in enumerate(current):
        if not isinstance(existing, Mapping):
            raise RelocationExceptionMutationError(
                f"existing relocation exception {index} is not an object"
            )
        try:
            existing_normalized = normalize_reviewed_exception(existing)
        except RelocationExpectationError as exc:
            raw_offsets = existing.get("offsets")
            if not isinstance(raw_offsets, list):
                raw_offsets = [existing.get("offset")]
            raw_keys = {
                (
                    str(existing.get("object_symbol", "")),
                    offset,
                    existing.get("type"),
                )
                for offset in raw_offsets
            }
            if raw_keys & keys:
                raise RelocationExceptionMutationError(
                    f"existing same-site relocation exception is invalid: {exc}"
                ) from exc
            continue
        existing_offsets = (
            tuple(int(item) for item in existing_normalized["offsets"])
            if existing_normalized.get("exception_mode")
            == PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY
            else (int(existing_normalized["offset"]),)
        )
        existing_keys = {
            (
                str(existing_normalized["object_symbol"]),
                offset,
                int(existing_normalized["type"]),
            )
            for offset in existing_offsets
        }
        if not existing_keys & keys:
            continue
        if existing_normalized == dict(normalized):
            raise RelocationExceptionMutationError(
                "identical reviewed relocation exception is already present"
            )
        raise RelocationExceptionMutationError(
            "conflicting reviewed relocation exception already exists for this source operand"
        )
    current.append(dict(normalized))
    row["relocation_expectation_exceptions"] = current


def _stage_missing_physical_target(
    *,
    document: ProgressDocument,
    proposed: dict[str, Any],
    normalized_request: Mapping[str, Any],
    reference: Path,
) -> dict[str, Any]:
    creation = normalized_request.get("create_missing_data")
    if not isinstance(creation, Mapping):
        raise RelocationExceptionMutationError(
            "create_missing_data request is missing or invalid"
        )
    retail_target = int(normalized_request["retail_target"])
    target_symbol_id = str(normalized_request["target_symbol_id"])
    expected_symbol_id = f"recoil:data:0x{retail_target:x}"
    if target_symbol_id != expected_symbol_id:
        raise RelocationExceptionMutationError(
            "create_missing_data target_symbol_id must equal the deterministic "
            f"physical id {expected_symbol_id!r}"
        )
    end_exclusive = address_value(str(creation["target_end_exclusive"]))
    if end_exclusive != retail_target + 4:
        raise RelocationExceptionMutationError(
            "create_missing_data target extent must be exactly four bytes"
        )
    owner_id = str(creation["target_owner_id"])
    evidence_ids = list(normalized_request["evidence_ids"])
    try:
        owner = _validate_owner_evidence(
            document,
            owner_id=owner_id,
            evidence_ids=evidence_ids,
        )
        if owner.get("kind") == "provider-boundary":
            raise RelocationExceptionMutationError(
                "create_missing_data requires an existing non-provider owner"
            )
        if _target_candidates(document, retail_target):
            raise RelocationExceptionMutationError(
                "create_missing_data requested but immutable retail target already has a symbol"
            )
        output_section_id = _retail_section(
            document,
            reference=reference,
            start=retail_target,
            end_exclusive=end_exclusive,
        )
    except RelocationTargetMutationError as exc:
        raise RelocationExceptionMutationError(str(exc)) from exc
    if output_section_id != "recoil:section:.rdata":
        raise RelocationExceptionMutationError(
            "create_missing_data target must be exact file-backed retail .rdata"
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
            raise RelocationExceptionMutationError(
                f"created data extent overlaps current symbol {other_id!r}"
            )

    proposed_symbols = proposed.get("symbols")
    proposed_owners = proposed.get("owners")
    if not isinstance(proposed_symbols, dict) or not isinstance(proposed_owners, dict):
        raise RelocationExceptionMutationError(
            "tracker symbols and owners collections must be objects"
        )
    if target_symbol_id in proposed_symbols:
        raise RelocationExceptionMutationError(
            f"created data symbol {target_symbol_id!r} already exists"
        )
    target_name = str(creation["target_name"])
    proposed_symbols[target_symbol_id] = _pending_data_symbol(
        address=retail_target,
        end_exclusive=end_exclusive,
        name=target_name,
        output_section_id=output_section_id,
        evidence_ids=evidence_ids,
    )
    owner_copy = proposed_owners.get(owner_id)
    if not isinstance(owner_copy, dict):
        raise RelocationExceptionMutationError(
            f"target owner {owner_id!r} disappeared during mutation"
        )
    relationships = owner_copy.get("relationships")
    if not isinstance(relationships, list):
        raise RelocationExceptionMutationError(
            "target owner relationships must be a list"
        )
    relationship = {
        "kind": "primary-data",
        "address": normalize_address(retail_target),
        "symbol_id": target_symbol_id,
        "name": target_name,
    }
    for item in relationships:
        if not isinstance(item, Mapping):
            continue
        try:
            item_address = normalize_address(item.get("address"))
        except (TypeError, ValueError):
            item_address = ""
        if (
            item.get("symbol_id") == target_symbol_id
            or (
                item.get("kind") == "primary-data"
                and item_address == relationship["address"]
            )
        ):
            raise RelocationExceptionMutationError(
                "created data owner relationship already exists or conflicts"
            )
    relationships.append(relationship)
    proposed_document = ProgressDocument(proposed)
    proposed_owner = proposed_document.collection("owners")[owner_id]
    owner_context = relocation_target_owner_context(
        owner_id=owner_id,
        owner=proposed_owner,
        evidence_ids=evidence_ids,
    )
    return {
        "target_symbol_id": target_symbol_id,
        "target_created": True,
        "target_owner_id": owner_id,
        "target_name": target_name,
        "target_address": normalize_address(retail_target),
        "target_end_exclusive": normalize_address(end_exclusive),
        "output_section_id": output_section_id,
        "owner_binding": owner_context,
        "relationship": relationship,
    }


def set_reviewed_exception(
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
) -> dict[str, Any]:
    store = ProgressStore(progress)
    try:
        document = store.load()
    except ProgressError as exc:
        raise RelocationExceptionMutationError(str(exc)) from exc
    if document.revision != expected_revision:
        raise RelocationExceptionMutationError(
            f"revision changed: expected {expected_revision}, found {document.revision}"
        )
    if bindings is None:
        # This is tracker/manifest identity lookup only; it performs no candidate build.
        from _recoil.commands.live_byte_verify import _bindings

        try:
            bindings = _bindings(document, manifest_dir)
        except (RuntimeError, OSError, ValueError) as exc:
            raise RelocationExceptionMutationError(str(exc)) from exc
    try:
        normalized_request = normalize_reviewed_exception(payload)
    except RelocationExpectationError as exc:
        raise RelocationExceptionMutationError(str(exc)) from exc
    proposed = deepcopy(document.data)
    creation_report: dict[str, Any] | None = None
    creation = normalized_request.pop("create_missing_data", None)
    if creation is not None:
        normalized_request["create_missing_data"] = creation
        creation_report = _stage_missing_physical_target(
            document=document,
            proposed=proposed,
            normalized_request=normalized_request,
            reference=reference,
        )
        normalized_request.pop("create_missing_data", None)
    proposed_document = ProgressDocument(proposed)
    normalized, decoded = prepare_reviewed_exception(
        document=proposed_document,
        bindings=bindings,
        source_symbol_id=source_symbol_id,
        source_address=source_address,
        payload=normalized_request,
        reference=reference,
    )
    if creation_report is not None:
        normalized["physical_target_owner_binding"] = dict(
            creation_report["owner_binding"]
        )
        normalized["physical_target_relationship"] = dict(
            creation_report["relationship"]
        )
        try:
            normalized = normalize_reviewed_exception(normalized)
        except RelocationExpectationError as exc:
            raise RelocationExceptionMutationError(str(exc)) from exc
    _append_exception(
        proposed,
        source_symbol_id=source_symbol_id,
        normalized=normalized,
    )
    try:
        commit = store.commit(
            proposed,
            expected_revision=expected_revision,
            apply=apply,
        )
    except (ConcurrentProgressUpdate, ProgressError) as exc:
        raise RelocationExceptionMutationError(str(exc)) from exc
    return {
        "report_version": 1,
        "kind": "relocation-exception-mutation",
        "operation": "set",
        "validation_mode": "immutable-retail-and-current-tracker",
        "candidate_independent": True,
        "source_symbol_id": source_symbol_id,
        "source_address": normalize_address(source_address),
        "reference": display_path(reference),
        "exception": normalized,
        "target_created": creation_report is not None,
        "created_target": creation_report,
        "decoded_retail_operand": decoded,
        "commit": commit.to_dict(),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Set one reviewed candidate-independent relocation ambiguity exception."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    child = subparsers.add_parser("set")
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
    if args.command != "set":
        raise RelocationExceptionMutationError(f"unsupported operation {args.command!r}")
    return set_reviewed_exception(
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
    except (OSError, ValueError, RelocationExceptionMutationError) as exc:
        print(f"relocation exception mutation error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        mode = "APPLIED" if report["commit"]["applied"] else "DRY-RUN"
        print(
            f"Relocation exception {mode}: {report['source_symbol_id']} "
            + (
                "offsets="
                + ",".join(
                    f"0x{item:x}" for item in report["exception"]["offsets"]
                )
                if "offsets" in report["exception"]
                else f"offset=0x{report['exception']['offset']:x}"
            )
        )
        print(
            f"revision {report['commit']['previous_revision']} -> {report['commit']['revision']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
