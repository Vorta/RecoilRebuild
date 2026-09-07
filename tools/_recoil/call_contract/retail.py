"""Recoil call-contract retail evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import flow as _cc_flow
from _recoil.call_contract import receiver_cursor as _cc_receiver_cursor
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import storage_identity as _cc_storage_identity
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedCallResultBridge,
        ReviewedDirectIatStorageCallSpec,
        ReviewedDynamicExportCallSpec,
        ReviewedEhArrayDestructorCallSpec,
        ReviewedRegisterIatBridgeSpec,
        ReviewedVftableStorageBridgeSpec,
        ReviewedVptrStorageBridge,
    )

import re
import struct
from collections import Counter
from copy import deepcopy
from dataclasses import replace
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    Instruction,
)
from _recoil.commands.provider_target_mutation import retail_import_target
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import (
    CALL_CONTRACT_SLICE_MAX_BODIES,
    ProgressDocument,
    ProgressError,
    address_value,
    normalize_address,
)
from _recoil.lib.reference_images import reference_image


def _call_contract_bn_call_budget(body_count: int) -> int:
    """Return one bounded deterministic budget for the full retail pipeline."""

    if (
        not isinstance(body_count, int)
        or isinstance(body_count, bool)
        or body_count < 1
        or body_count > _cc_catalog.CALL_CONTRACT_BN_MAX_FULL_CENSUS_BODIES
    ):
        raise ProgressError(
            "call-contract Binary Ninja budget requires 1..4096 census bodies"
        )
    scaled_count = max(CALL_CONTRACT_SLICE_MAX_BODIES, body_count)
    return (
        _cc_catalog.CALL_CONTRACT_BN_FIXED_CALLS
        + scaled_count * _cc_catalog.CALL_CONTRACT_BN_CALLS_PER_BODY
    )


def _new_call_contract_bridge(
    *,
    bridge_url: str,
    body_count: int,
) -> BinaryNinjaBridge:
    """Create the target-qualified bridge used for direct retail fact reads."""

    image = reference_image("recoil")
    return BinaryNinjaBridge(
        bridge_url,
        binary=Path(image.bndb_path).name,
        call_budget=_call_contract_bn_call_budget(body_count),
    )




class _CallContractBinaryNinjaFactCache:
    """Invocation-local immutable read cache for a full-census proof."""

    def __init__(self, bridge: BinaryNinjaBridge) -> None:
        self._bridge = bridge
        self._json_rows: dict[tuple[str, tuple[tuple[str, str], ...]], dict[str, Any]] = {}
        self._assemblies: dict[str, str] = {}
        self._hexdumps: dict[tuple[str, int], str] = {}
        self._hits = 0
        self._misses = 0

    def __getattr__(self, name: str) -> Any:
        return getattr(self._bridge, name)

    def get_json(self, endpoint: str, **params: object) -> dict[str, Any]:
        key = (
            endpoint.lstrip("/"),
            tuple(sorted((str(name), repr(value)) for name, value in params.items())),
        )
        cached = self._json_rows.get(key)
        if cached is not None:
            self._hits += 1
            return deepcopy(cached)
        result = self._bridge.get_json(endpoint, **params)
        self._json_rows[key] = deepcopy(result)
        self._misses += 1
        return deepcopy(result)

    def assembly(self, address_or_name: str) -> str:
        key = str(address_or_name)
        if key in self._assemblies:
            self._hits += 1
            return self._assemblies[key]
        result = str(self._bridge.assembly(key))
        self._assemblies[key] = result
        self._misses += 1
        return result

    def function_info(self, address_or_name: str) -> dict[str, Any]:
        key = (
            "address"
            if str(address_or_name).lower().startswith("0x")
            else "name"
        )
        return self.get_json("functionInfo", **{key: address_or_name})

    def hexdump(self, address: str, length: int) -> str:
        key = (str(address), int(length))
        if key in self._hexdumps:
            self._hits += 1
            return self._hexdumps[key]
        result = str(self._bridge.hexdump(address, length))
        self._hexdumps[key] = result
        self._misses += 1
        return result

    def il(self, address_or_name: str, view: str = "mlil") -> str:
        key = (
            "address"
            if str(address_or_name).lower().startswith("0x")
            else "name"
        )
        return str(
            self.get_json(
                "il",
                **{key: address_or_name, "view": view},
            ).get("il", "")
        )

    def preload_assemblies(self, addresses: Sequence[str]) -> None:
        for address in addresses:
            self.assembly(str(address))

    def metrics(self) -> dict[str, int]:
        return {
            "json_request_count": len(self._json_rows),
            "assembly_count": len(self._assemblies),
            "hexdump_count": len(self._hexdumps),
            "cache_hit_count": self._hits,
            "bridge_read_count": self._misses,
        }


def _target_function_rows(target: Any) -> tuple[Any, ...]:
    rows: list[Any] = list(getattr(target, "functions", ()))
    for entry in getattr(target, "translation_unit_function_order", ()):
        rows.extend(getattr(entry, "functions", ()))
    for interval in getattr(target, "linked_function_intervals", ()):
        predecessor = getattr(interval, "predecessor", None)
        successor = getattr(interval, "successor", None)
        if predecessor is not None:
            rows.append(predecessor)
        rows.extend(getattr(interval, "functions", ()))
        if successor is not None:
            rows.append(successor)
    unique: dict[
        tuple[str, str, str, str, str, bool, bool],
        Any,
    ] = {}
    for row in rows:
        key = (
            str(getattr(row, "address", "")),
            str(getattr(row, "symbol", "")),
            str(getattr(row, "provenance", "")),
            str(getattr(row, "pipeline_class", "")),
            str(getattr(row, "authored_order_role", "")),
            bool(getattr(row, "required_presence", False)),
            bool(getattr(row, "full_order_gate", False)),
        )
        unique[key] = row
    return tuple(unique.values())


def _candidate_unresolved_complete_destructors(
    candidate: CandidateAssembly,
    *,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> tuple[str, ...]:
    names: set[str] = set()
    for index, instruction in enumerate(candidate.instructions):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        if mnemonic == "jmp" and index in candidate.local_control_flow_indices:
            continue
        operand = _cc_cfg._instruction_operand(instruction)
        match = _cc_catalog.DECORATED_RE.search(operand)
        if match is None:
            continue
        name = match.group(0)
        if _cc_catalog.MSVC_COMPLETE_DESTRUCTOR_RE.fullmatch(name) is None:
            continue
        if name in indexes.by_candidate_name or name in bridge_names:
            continue
        names.add(name)
    return tuple(sorted(names))


def _candidate_has_unresolved_eh_array_destructor(
    candidate: CandidateAssembly,
    *,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> bool:
    if (
        _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL in indexes.by_candidate_name
        or _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL in bridge_names
    ):
        return False
    for index, instruction in enumerate(candidate.instructions):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        if mnemonic == "jmp" and index in candidate.local_control_flow_indices:
            continue
        match = _cc_catalog.DECORATED_RE.search(_cc_cfg._instruction_operand(instruction))
        if match is not None and match.group(0) == _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL:
            return True
    return False


def _reviewed_eh_array_destructor_call_context(
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> ReviewedEhArrayDestructorCallSpec | None:
    spec = _cc_catalog.BRIEFING_RUNTIME_EH_ARRAY_DESTRUCTOR_CALL_BRIDGE
    if (
        caller_identity != spec.caller_identity
        or normalize_address(caller_start) != spec.caller_start
        or normalize_address(caller_end_exclusive) != spec.caller_end_exclusive
    ):
        return None
    definition = candidate.caller_definition
    if definition is None or definition.symbol != spec.caller_symbol:
        raise ValueError(
            f"{spec.label} requires the exact candidate caller symbol "
            f"{spec.caller_symbol!r}"
        )
    helper_rows: list[int] = []
    exact_call_rows: list[int] = []
    for index, instruction in enumerate(candidate.instructions):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        if mnemonic == "jmp" and index in candidate.local_control_flow_indices:
            continue
        operand = _cc_cfg._instruction_operand(instruction)
        match = _cc_catalog.DECORATED_RE.search(operand)
        if match is None or match.group(0) != _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL:
            continue
        helper_rows.append(index)
        trailing_text = operand[match.end() :].strip()
        if (
            mnemonic == "call"
            and match.start() == 0
            and (not trailing_text or trailing_text.startswith(";"))
        ):
            exact_call_rows.append(index)
    if (
        len(helper_rows) != spec.invocation_count
        or exact_call_rows != helper_rows
    ):
        raise ValueError(
            f"{spec.label} requires exactly {spec.invocation_count} "
            "main-procedure exact direct helper call"
        )
    return spec


def _reviewed_eh_array_destructor_operand_bridges(
    candidate: CandidateAssembly,
    identity: str,
) -> dict[str, str]:
    return {
        _cc_cfg._instruction_operand(instruction): identity
        for instruction in candidate.instructions
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            and (
                match := _cc_catalog.DECORATED_RE.search(
                    _cc_cfg._instruction_operand(instruction)
                )
            )
            is not None
            and match.start() == 0
            and match.group(0) == _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL
            and (
                not _cc_cfg._instruction_operand(instruction)[match.end() :].strip()
                or _cc_cfg._instruction_operand(instruction)[
                    match.end() :
                ].strip().startswith(";")
            )
        )
    }


def _reviewed_register_iat_bridge(
    spec: ReviewedRegisterIatBridgeSpec,
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge_data_rows: Sequence[Any],
) -> tuple[dict[str, str], dict[str, str]]:
    """Prove one fixed tracker-backed register-loaded IAT call and lifetime."""
    if (
        caller_identity != spec.caller_identity
        or normalize_address(caller_start) != spec.caller_start
        or normalize_address(caller_end_exclusive) != spec.caller_end_exclusive
    ):
        return {}, {}

    iat_identity = f"iat:{spec.import_name}"
    function_id = f"recoil:function:{spec.retail_storage_address}"
    data_id = f"recoil:data:{spec.retail_storage_address}"
    storage_id = f"recoil:storage:va:{spec.retail_storage_address}"
    owner = document.collection("owners").get(spec.owner_id)
    function = document.collection("symbols").get(function_id)
    data = document.collection("symbols").get(data_id)
    storage = document.collection("storage_contributions").get(storage_id)
    end_exclusive = normalize_address(
        address_value(spec.retail_storage_address) + 4
    )
    function_end = normalize_address(
        address_value(spec.retail_storage_address) + 1
    )
    expected_relationships = [
        {
            "kind": "anchor-address",
            "address": spec.retail_storage_address,
        },
        {
            "kind": "primary-function",
            "address": spec.retail_storage_address,
            "symbol_id": function_id,
        },
        {
            "kind": "primary-data",
            "address": spec.retail_storage_address,
            "symbol_id": data_id,
            "name": f"{spec.import_dll}!{spec.import_name} IAT",
        },
    ]
    if (
        not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "provider-boundary"
        or owner.get("blocker") != "none"
        or owner.get("lifecycle_state") != "accepted"
        or owner.get("provider_state") != "accepted"
        or owner.get("relationships") != expected_relationships
        or owner.get("source_paths") != []
    ):
        raise ValueError(
            f"{spec.label} requires its exact accepted tracker provider owner"
        )
    if (
        not isinstance(function, Mapping)
        or function.get("binary") != "recoil"
        or function.get("kind") != "provider-function"
        or function.get("disposition") != "provider"
        or function.get("pipeline_class") != "non-authored"
        or function.get("authored_order_role") != "non-authored"
        or function.get("address") != spec.retail_storage_address
        or function.get("end_exclusive") != function_end
        or function.get("extent_state") != "known"
        or function.get("size") != 1
        or function.get("import_dll") != spec.import_dll
        or function.get("import_name") != spec.import_name
        or "import_ordinal" in function
        or function.get("object_symbol") != spec.candidate_import_symbol
        or function.get("output_section_id") != "recoil:section:.rdata"
        or function.get("ownership_state") != "primary-owned"
        or function.get("physical_block_id") is not None
        or function.get("storage_contribution_ids") != []
    ):
        raise ValueError(
            f"{spec.label} requires its exact tracker provider-function view"
        )
    if (
        not isinstance(data, Mapping)
        or data.get("binary") != "recoil"
        or data.get("kind") != "data"
        or data.get("disposition") != "provider"
        or data.get("address") != spec.retail_storage_address
        or data.get("end_exclusive") != end_exclusive
        or data.get("extent_state") != "known"
        or data.get("size") != 4
        or data.get("import_dll") != spec.import_dll
        or data.get("import_name") != spec.import_name
        or "import_ordinal" in data
        or data.get("output_section_id") != "recoil:section:.rdata"
        or data.get("ownership_state") != "primary-owned"
        or data.get("physical_block_id") is not None
        or data.get("storage_contribution_ids") != [storage_id]
    ):
        raise ValueError(
            f"{spec.label} requires its exact four-byte tracker provider-data view"
        )
    reference = storage.get("reference") if isinstance(storage, Mapping) else None
    extent = (
        storage.get("verification", {}).get("extent")
        if isinstance(storage, Mapping)
        and isinstance(storage.get("verification"), Mapping)
        else None
    )
    if (
        not isinstance(storage, Mapping)
        or storage.get("binary") != "recoil"
        or storage.get("kind") != "provider-data"
        or storage.get("output_section_id") != "recoil:section:.rdata"
        or storage.get("overlap") != "none"
        or storage.get("owner_ids") != [spec.owner_id]
        or storage.get("parent_contribution_id") is not None
        or storage.get("symbol_ids") != [data_id]
        or not isinstance(reference, Mapping)
        or reference.get("address") != spec.retail_storage_address
        or reference.get("end_exclusive") != end_exclusive
        or reference.get("extent_state") != "known"
        or reference.get("size") != 4
        or not isinstance(extent, Mapping)
        or extent.get("result") != "passed"
        or extent.get("disposition") != "accepted"
        or extent.get("freshness") != "current"
        or extent.get("validation_mode") != "live"
        or extent.get("gating") is not True
    ):
        raise ValueError(
            f"{spec.label} requires its exact current tracker IAT storage extent"
        )
    shared_evidence = set(owner.get("evidence_ids", ()))
    for row in (function, data, storage, reference, extent):
        shared_evidence &= set(row.get("evidence_ids", ()))
    expected_scope_ids = {
        owner_id
        for owner_id in (spec.owner_id, function_id, data_id, storage_id)
    }
    current_evidence = [
        row
        for evidence_id in shared_evidence
        if isinstance(
            row := document.collection("evidence").get(evidence_id),
            Mapping,
        )
        and row.get("kind") == "provider-target-registration"
        and row.get("result") == "passed"
        and row.get("disposition") == "accepted"
        and row.get("freshness") == "current"
        and row.get("validation_mode") == "live"
        and row.get("gating") is True
        and set(row.get("scope_ids", ())) == expected_scope_ids
        and isinstance(row.get("provenance"), Mapping)
        and row["provenance"].get("candidate_independent") is True
        and row["provenance"].get("address")
        == spec.retail_storage_address
        and row["provenance"].get("dll") == spec.import_dll
        and row["provenance"].get("import_name") == spec.import_name
        and row["provenance"].get("import_ordinal") is None
        and row["provenance"].get("object_symbol")
        == spec.candidate_import_symbol
        and row["provenance"].get("object_symbol_basis")
        == "reviewed-vc5-provider-declaration"
    ]
    if len(current_evidence) != 1:
        raise ValueError(
            f"{spec.label} requires one shared current candidate-independent "
            "provider-registration evidence row"
        )
    if (
        indexes.storage_by_address.get(spec.retail_storage_address)
        != iat_identity
        or indexes.storage_by_name.get(spec.import_name) != iat_identity
        or indexes.storage_by_name.get(spec.candidate_import_symbol)
        != iat_identity
    ):
        raise ValueError(
            f"{spec.label} tracker package is not the exact indexed IAT identity"
        )

    definition = candidate.caller_definition
    if definition is None or definition.symbol != spec.caller_symbol:
        raise ValueError(
            f"{spec.label} requires exact candidate caller symbol "
            f"{spec.caller_symbol!r}"
        )

    retail_data_rows = [
        row
        for row in bridge_data_rows
        if normalize_address(str(getattr(row, "address", "")))
        == spec.retail_storage_address
    ]
    if len(retail_data_rows) != 1:
        raise ValueError(
            f"{spec.label} requires one exact live retail data row at "
            f"{spec.retail_storage_address}"
        )
    retail_data = retail_data_rows[0]
    retail_type = re.sub(
        r"\s+",
        " ",
        str(getattr(retail_data, "type_text", "")).strip(),
    )
    if (
        getattr(retail_data, "name", "") != spec.import_name
        or getattr(retail_data, "raw_name", "") not in {"", spec.import_name}
        or int(getattr(retail_data, "size", 0)) != 4
        or retail_type != spec.retail_storage_type
    ):
        raise ValueError(
            f"{spec.label} retail storage is not the exact typed "
            f"{spec.import_name} import pointer"
        )

    def exact_instruction(
        instructions: Sequence[Instruction],
        *,
        address: str,
        mnemonic: str,
        operand: str | None,
        body: bytes,
        allow_wrapped_cod_address: bool = False,
    ) -> Instruction:
        rows = [
            instruction
            for instruction in instructions
            if (
                _cc_cfg._source_instruction_address(instruction) == address
                or (
                    allow_wrapped_cod_address
                    and not _cc_cfg._source_instruction_address(instruction)
                )
            )
            and _cc_cfg._instruction_mnemonic(instruction) == mnemonic
            and (
                operand is None
                or _cc_cfg._instruction_operand(instruction).lower() == operand
            )
            and tuple(value.lower() for value in instruction.bytes)
            == tuple(f"{value:02x}" for value in body)
        ]
        if len(rows) != 1:
            raise ValueError(
                f"{spec.label} requires one exact {address} "
                f"{mnemonic} {operand} instruction"
            )
        return rows[0]

    retail_load = exact_instruction(
        retail_instructions,
        address=spec.retail_load_address,
        mnemonic="mov",
        operand=f"{spec.register}, dword [{spec.retail_storage_address}]",
        body=spec.retail_load_body,
    )
    retail_call = exact_instruction(
        retail_instructions,
        address=spec.retail_call_address,
        mnemonic="call",
        operand=spec.register,
        body=spec.retail_call_body,
    )
    candidate_load = exact_instruction(
        candidate.instructions,
        address=spec.candidate_load_address,
        mnemonic="mov",
        operand=(
            f"{spec.register}, dword "
            f"{spec.candidate_import_symbol.lower()}"
        ),
        body=spec.candidate_load_body,
        allow_wrapped_cod_address=True,
    )
    candidate_call = exact_instruction(
        candidate.instructions,
        address=spec.candidate_call_address,
        mnemonic="call",
        operand=spec.register,
        body=spec.candidate_call_body,
    )
    retail_backedge = exact_instruction(
        retail_instructions,
        address=spec.retail_backedge_address,
        mnemonic=spec.retail_backedge_mnemonic,
        operand=None,
        body=spec.retail_backedge_body,
    )
    candidate_backedge = exact_instruction(
        candidate.instructions,
        address=spec.candidate_backedge_address,
        mnemonic=spec.candidate_backedge_mnemonic,
        operand=None,
        body=spec.candidate_backedge_body,
    )

    def writes_reviewed_register(instruction: Instruction) -> bool:
        return _cc_cfg._instruction_may_clobber_register(instruction, spec.register)

    for label, instructions, load, call, backedge in (
        (
            "retail",
            retail_instructions,
            retail_load,
            retail_call,
            retail_backedge,
        ),
        (
            "candidate",
            candidate.instructions,
            candidate_load,
            candidate_call,
            candidate_backedge,
        ),
    ):
        load_index = next(
            index
            for index, instruction in enumerate(instructions)
            if instruction is load
        )
        call_index = next(
            index
            for index, instruction in enumerate(instructions)
            if instruction is call
        )
        backedge_index = next(
            index
            for index, instruction in enumerate(instructions)
            if instruction is backedge
        )
        if (
            call_index <= load_index
            or backedge_index <= call_index
            or any(
                writes_reviewed_register(instruction)
                for instruction in instructions[
                    load_index + 1 : backedge_index + 1
                ]
            )
        ):
            raise ValueError(
                f"{spec.label} {label} register provenance is "
                "clobbered or ambiguous before the indirect call or "
                "before the fixed loop backedge"
            )

    retail_register_calls = [
        instruction
        for instruction in retail_instructions
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction).lower() == spec.register
    ]
    candidate_register_calls = [
        instruction
        for instruction in candidate.instructions
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction).lower() == spec.register
    ]
    if (
        len(retail_register_calls) != spec.invocation_count
        or len(candidate_register_calls) != spec.invocation_count
    ):
        raise ValueError(
            f"{spec.label} requires exactly {spec.invocation_count} "
            f"retail and candidate call {spec.register} invocation"
        )

    references = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name == spec.candidate_import_symbol
    )
    if (
        definition.undefined_external_functions.count(
            spec.candidate_import_symbol
        )
        != 1
        or len(references) != 1
        or references[0].type != IMAGE_REL_I386_DIR32
        or references[0].offset != spec.candidate_relocation_offset
        or references[0].offset < 2
        or references[0].offset + 4 > len(definition.data)
        or definition.data[
            references[0].offset - 2 : references[0].offset
        ]
        != spec.candidate_load_body[:2]
        or struct.unpack_from(
            "<I", definition.data, references[0].offset
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(
                references[0].offset,
                references[0].offset + 4,
            )
        )
    ):
        raise ValueError(
            f"{spec.label} requires one exact undefined external "
            f"{spec.candidate_import_symbol} mov-load DIR32 relocation at "
            f"+{hex(spec.candidate_relocation_offset)}"
        )

    retail_bridges = {
        spec.retail_storage_address: iat_identity,
    }
    candidate_bridges = {
        spec.candidate_import_symbol.lower(): iat_identity,
    }
    retail_contract = _cc_extraction.extract_invocation_contract(
        retail_instructions,
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        reviewed_register_storage_bridges=retail_bridges,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        reviewed_register_storage_bridges=candidate_bridges,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    required_row = {
        "ordinal": spec.ordinal,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "iat",
        "target_identity": iat_identity,
        "storage_identity": iat_identity,
        "slot_displacement": None,
        "cleanup_bytes": spec.cleanup_bytes,
    }
    if (
        spec.ordinal >= len(retail_contract)
        or spec.ordinal >= len(candidate_contract)
        or retail_contract[spec.ordinal] != required_row
        or candidate_contract[spec.ordinal] != required_row
        or sum(
            row.get("target_identity") == iat_identity
            for row in retail_contract
        )
        != spec.invocation_count
        or sum(
            row.get("target_identity") == iat_identity
            for row in candidate_contract
        )
        != spec.invocation_count
    ):
        raise ValueError(
            f"{spec.label} lacks the exact same ordinal, indirect IAT "
            "call form, provenance, and cleanup"
        )
    return retail_bridges, candidate_bridges


def _zinput_wait_sleep_register_iat_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    **kwargs: Any,
) -> tuple[dict[str, str], dict[str, str]]:
    return _reviewed_register_iat_bridge(
        _cc_catalog.ZINPUT_WAIT_SLEEP_REGISTER_IAT_BRIDGE,
        retail_instructions,
        candidate,
        **kwargs,
    )


def _briefing_start_sleep_register_iat_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    **kwargs: Any,
) -> tuple[dict[str, str], dict[str, str]]:
    return _reviewed_register_iat_bridge(
        _cc_catalog.BRIEFING_START_SLEEP_REGISTER_IAT_BRIDGE,
        retail_instructions,
        candidate,
        **kwargs,
    )


def _zsnd_destroy_owned_data_free_candidate_iat_provenance(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> frozenset[str]:
    """Prove only the retained candidate's unique EBP-loaded free provenance.

    Retail expected calls are deliberately absent from this proof.  The
    current candidate-independent tracker package supplies the IAT identity;
    exact COD/COFF, relocation, CFG dominance, and register lifetime establish
    only that the candidate's ten ``CALL EBP`` sites consume that identity.
    """

    if normalize_address(caller_start) != _cc_catalog.ZSND_DESTROY_OWNED_DATA_CALLER_START:
        return frozenset()
    if (
        caller_identity != _cc_catalog.ZSND_DESTROY_OWNED_DATA_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.ZSND_DESTROY_OWNED_DATA_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "zSnd DestroyOwnedData free provenance requires the exact "
            "reviewed caller identity and extent"
        )
    target = candidate.target
    definition = candidate.caller_definition
    if (
        target is None
        or getattr(target, "name", "") != _cc_catalog.ZSND_DESTROY_OWNED_DATA_TARGET_NAME
        or definition is None
        or definition.symbol != _cc_catalog.ZSND_DESTROY_OWNED_DATA_CALLER_SYMBOL
        or definition.section_start != 0
        or definition.section_end != len(definition.data)
        or len(definition.data) != _cc_catalog.ZSND_DESTROY_OWNED_DATA_CANDIDATE_SIZE
    ):
        raise ValueError(
            "zSnd DestroyOwnedData free provenance requires the exact selected "
            "target and complete candidate caller COFF contribution"
        )

    packages = [
        package
        for package in _cc_storage_identity._current_tracker_iat_storage_packages(document)
        if package.address == _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_IAT_ADDRESS
    ]
    if (
        len(packages) != 1
        or packages[0].import_dll != "MSVCRT.dll"
        or packages[0].import_name != "free"
        or packages[0].import_ordinal is not None
        or packages[0].object_symbol
        != _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_IMPORT_SYMBOL
        or packages[0].identity != "iat:free"
        or indexes.storage_by_address.get(
            _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_IAT_ADDRESS
        )
        != "iat:free"
        or indexes.storage_by_name.get("free") != "iat:free"
        or indexes.storage_by_name.get(
            _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_IMPORT_SYMBOL
        )
        != "iat:free"
    ):
        raise ValueError(
            "zSnd DestroyOwnedData free provenance requires the exact current "
            "candidate-independent MSVCRT free IAT package"
        )

    references = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name == _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_IMPORT_SYMBOL
    )
    relocation = references[0] if len(references) == 1 else None
    if (
        definition.undefined_external_functions.count(
            _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_IMPORT_SYMBOL
        )
        != 1
        or _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_IMPORT_SYMBOL
        in (
            definition.defined_external_functions
            + definition.undefined_external_data
            + definition.defined_external_data
        )
        or relocation is None
        or relocation.type != IMAGE_REL_I386_DIR32
        or relocation.offset
        != _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_RELOCATION_OFFSET
        or definition.data[
            _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_LOAD_OFFSET :
            _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_RELOCATION_OFFSET
        ]
        != b"\x8b\x2d"
        or struct.unpack_from(
            "<I",
            definition.data,
            _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_RELOCATION_OFFSET,
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_RELOCATION_OFFSET,
                _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_RELOCATION_OFFSET + 4,
            )
        )
    ):
        raise ValueError(
            "zSnd DestroyOwnedData free provenance requires one exact "
            "zero-addend fully masked MOV EBP DIR32 relocation"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    encoded = bytearray()
    natural_offsets: list[int] = []
    for instruction in candidate.instructions:
        natural_offsets.append(len(encoded))
        try:
            encoded.extend(int(value, 16) for value in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "zSnd DestroyOwnedData free provenance requires exact COD bytes"
            ) from exc
    if tuple(natural_offsets) != offsets or bytes(encoded) != definition.data:
        raise ValueError(
            "zSnd DestroyOwnedData free provenance requires one complete "
            "contiguous COD/COFF instruction cover"
        )
    by_offset = {
        offset: (index, instruction)
        for index, (offset, instruction) in enumerate(
            zip(offsets, candidate.instructions)
        )
        if offset is not None
    }
    load_rows = [
        (offset, index, instruction)
        for offset, (index, instruction) in by_offset.items()
        if _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_IMPORT_SYMBOL
        in _cc_cfg._instruction_operand(instruction)
    ]
    if (
        len(load_rows) != 1
        or load_rows[0][0] != _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_LOAD_OFFSET
        or _cc_cfg._instruction_mnemonic(load_rows[0][2]) != "mov"
        or _cc_cfg._instruction_operand(load_rows[0][2])
        != "ebp, dword __imp__free"
        or tuple(value.lower() for value in load_rows[0][2].bytes)
        != ("8b", "2d", "00", "00", "00", "00")
    ):
        raise ValueError(
            "zSnd DestroyOwnedData free provenance requires one unique exact "
            "COD MOV EBP absolute-IAT load"
        )

    call_rows = [
        (offset, index, instruction)
        for offset, (index, instruction) in by_offset.items()
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction) == "ebp"
    ]
    if (
        tuple(offset for offset, _index, _instruction in call_rows)
        != _cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_CALL_OFFSETS
        or any(
            tuple(value.lower() for value in instruction.bytes)
            != ("ff", "d5")
            for _offset, _index, instruction in call_rows
        )
    ):
        raise ValueError(
            "zSnd DestroyOwnedData free provenance requires the exact finite "
            "ten-site CALL EBP candidate census"
        )

    instruction_addresses = tuple(
        int(offset) if offset is not None else None for offset in offsets
    )
    address_counts = {
        address: instruction_addresses.count(address)
        for address in instruction_addresses
        if address is not None
    }
    instruction_index_by_address = {
        address: index
        for index, address in enumerate(instruction_addresses)
        if address is not None and address_counts[address] == 1
    }
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        candidate.instructions,
        instruction_addresses=instruction_addresses,
        instruction_index_by_address=instruction_index_by_address,
        source="cod",
        caller_start=0,
        caller_end=len(definition.data),
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    load_index = load_rows[0][1]
    entry_reachable = _cc_cfg._reachable_cfg_indices(successors, (0,))
    after_load = _cc_cfg._reachable_cfg_indices(
        successors,
        successors.get(load_index, ()),
    )
    without_load = _cc_cfg._reachable_cfg_indices(
        successors,
        (0,),
        blocked=frozenset({load_index}),
    )
    for _offset, call_index, _instruction in call_rows:
        proof_nodes = (
            after_load
            & _cc_cfg._indices_reaching_cfg_target(successors, call_index)
        )
        if (
            call_index not in entry_reachable
            or call_index not in after_load
            or call_index in without_load
            or unresolved & proof_nodes
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    candidate.instructions[index],
                    "ebp",
                )
                for index in proof_nodes
                if index != call_index
            )
        ):
            raise ValueError(
                "zSnd DestroyOwnedData free provenance rejects an ambiguous, "
                "bypassed, unresolved, or clobbered EBP reaching definition"
            )
    return frozenset(
        {normalize_address(_cc_catalog.ZSND_DESTROY_OWNED_DATA_FREE_LOAD_OFFSET)}
    )


def _zeffect_save_running_anim_record_fwrite_candidate_iat_provenance(
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
) -> tuple[frozenset[str], dict[str, str]]:
    """Prove the exact candidate EDI-cached ``fwrite`` reaching definition.

    Retail expected calls remain owned by the ordinary candidate-independent
    comparison.  This proof authorizes only the candidate COFF symbol spelling
    and the one wrapped COD definition after independently checking the retail
    import route, the complete finite fwrite relocation/call population, caller
    cleanup, and the exact CFG reaching definition for both ``CALL EDI`` sites.
    """

    if (
        normalize_address(caller_start)
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CALLER_START
    ):
        return frozenset(), {}
    if (
        caller_identity != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "zEffect SaveRunningAnimRecord fwrite provenance requires the "
            "exact reviewed caller identity and extent"
        )

    target = candidate.target
    definition = candidate.caller_definition
    if (
        target is None
        or getattr(target, "name", "")
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_TARGET_MANIFEST.resolve()
        or getattr(target, "source_from", "")
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_SOURCE_PATH
        or definition is None
        or definition.symbol
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CALLER_SYMBOL
        or definition.section_start != 0
        or definition.section_end != len(definition.data)
        or len(definition.data)
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CANDIDATE_COFF_SIZE
    ):
        raise ValueError(
            "zEffect SaveRunningAnimRecord fwrite provenance requires the "
            "exact selected target and complete candidate caller COFF contribution"
        )

    retail_import, _directory_context = retail_import_target(
        reference=reference,
        address=_cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IAT_ADDRESS,
        dll="MSVCRT.dll",
        import_name="fwrite",
    )
    if (
        retail_import.address
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IAT_ADDRESS
        or retail_import.dll != "MSVCRT.dll"
        or retail_import.import_name != "fwrite"
        or retail_import.import_ordinal is not None
        or indexes.storage_by_address.get(
            _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IAT_ADDRESS
        )
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IAT_IDENTITY
        or indexes.storage_by_name.get("fwrite")
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IAT_IDENTITY
        or indexes.storage_by_name.get(
            _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IMPORT_SYMBOL,
            _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IAT_IDENTITY,
        )
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IAT_IDENTITY
    ):
        raise ValueError(
            "zEffect SaveRunningAnimRecord fwrite provenance requires the "
            "exact immutable MSVCRT fwrite IAT route and comparison indexes"
        )

    references = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name
        == _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IMPORT_SYMBOL
    )
    if (
        definition.undefined_external_functions.count(
            _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IMPORT_SYMBOL
        )
        != 1
        or _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IMPORT_SYMBOL
        in (
            definition.defined_external_functions
            + definition.undefined_external_data
            + definition.defined_external_data
        )
        or tuple(relocation.offset for relocation in references)
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_RELOCATION_OFFSETS
        or any(
            relocation.type != IMAGE_REL_I386_DIR32
            for relocation in references
        )
        or any(
            struct.unpack_from("<I", definition.data, relocation.offset)[0]
            != 0
            for relocation in references
        )
        or any(
            not all(
                definition.relocation_mask[index]
                for index in range(relocation.offset, relocation.offset + 4)
            )
            for relocation in references
        )
        or definition.data[
            _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_LOAD_OFFSET :
            _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_LOAD_OFFSET + 2
        ]
        != b"\x8b\x3d"
        or any(
            definition.data[offset : offset + 2] != b"\xff\x15"
            for offset in _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_DIRECT_CALL_OFFSETS
        )
    ):
        raise ValueError(
            "zEffect SaveRunningAnimRecord fwrite provenance requires the "
            "exact four-site zero-addend fully masked DIR32 relocation census"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    encoded = bytearray()
    natural_offsets: list[int] = []
    for instruction in candidate.instructions:
        natural_offsets.append(len(encoded))
        try:
            encoded.extend(int(value, 16) for value in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "zEffect SaveRunningAnimRecord fwrite provenance requires "
                "exact COD bytes"
            ) from exc
    if (
        offsets[0] is not None
        or tuple(natural_offsets[1:]) != offsets[1:]
        or bytes(encoded)
        != definition.data[:_cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CANDIDATE_CODE_SIZE]
        or len(encoded) != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CANDIDATE_CODE_SIZE
        or definition.data[_cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CANDIDATE_CODE_SIZE:]
        != b"\x90" * (
            _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CANDIDATE_COFF_SIZE
            - _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CANDIDATE_CODE_SIZE
        )
    ):
        raise ValueError(
            "zEffect SaveRunningAnimRecord fwrite provenance requires one "
            "complete contiguous COD/COFF instruction cover"
        )
    by_offset = {
        offset: (index, instruction)
        for index, (offset, instruction) in enumerate(
            zip(offsets, candidate.instructions)
        )
        if offset is not None
    }

    symbol_rows = [
        (offset, index, instruction)
        for offset, (index, instruction) in by_offset.items()
        if _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IMPORT_SYMBOL
        in _cc_cfg._instruction_operand(instruction)
    ]
    expected_symbol_offsets = tuple(
        sorted(
            (
                _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_LOAD_OFFSET,
                *_cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_DIRECT_CALL_OFFSETS,
            )
        )
    )
    if tuple(offset for offset, _index, _instruction in symbol_rows) != (
        expected_symbol_offsets
    ):
        raise ValueError(
            "zEffect SaveRunningAnimRecord fwrite provenance rejects COD "
            "fwrite symbol population drift"
        )
    load_rows = [
        row
        for row in symbol_rows
        if row[0] == _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_LOAD_OFFSET
    ]
    if (
        len(load_rows) != 1
        or _cc_cfg._instruction_mnemonic(load_rows[0][2]) != "mov"
        or _cc_cfg._instruction_operand(load_rows[0][2])
        != "edi, dword __imp__fwrite"
        or tuple(value.lower() for value in load_rows[0][2].bytes)
        != ("8b", "3d", "00", "00", "00", "00")
    ):
        raise ValueError(
            "zEffect SaveRunningAnimRecord fwrite provenance requires one "
            "unique exact COD MOV EDI absolute-IAT load"
        )
    direct_call_rows = [
        row
        for row in symbol_rows
        if _cc_cfg._instruction_mnemonic(row[2]) == "call"
    ]
    if (
        tuple(offset for offset, _index, _instruction in direct_call_rows)
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_DIRECT_CALL_OFFSETS
        or any(
            _cc_cfg._instruction_operand(instruction) != "dword __imp__fwrite"
            or tuple(value.lower() for value in instruction.bytes)
            != ("ff", "15", "00", "00", "00", "00")
            for _offset, _index, instruction in direct_call_rows
        )
    ):
        raise ValueError(
            "zEffect SaveRunningAnimRecord fwrite provenance requires the "
            "exact finite three-site direct fwrite COD census"
        )
    cached_call_rows = [
        (offset, index, instruction)
        for offset, (index, instruction) in by_offset.items()
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction) == "edi"
    ]
    if (
        tuple(offset for offset, _index, _instruction in cached_call_rows)
        != _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_CACHED_CALL_OFFSETS
        or any(
            tuple(value.lower() for value in instruction.bytes) != ("ff", "d7")
            for _offset, _index, instruction in cached_call_rows
        )
    ):
        raise ValueError(
            "zEffect SaveRunningAnimRecord fwrite provenance requires the "
            "exact finite two-site CALL EDI candidate census"
        )
    if any(
        _cc_cfg._cleanup_after(candidate.instructions, index) != 16
        for _offset, index, _instruction in (
            direct_call_rows + cached_call_rows
        )
    ):
        raise ValueError(
            "zEffect SaveRunningAnimRecord fwrite provenance requires exact "
            "16-byte caller cleanup at all five fwrite calls"
        )

    instruction_addresses = tuple(
        int(offset) if offset is not None else None for offset in offsets
    )
    address_counts = {
        address: instruction_addresses.count(address)
        for address in instruction_addresses
        if address is not None
    }
    instruction_index_by_address = {
        address: index
        for index, address in enumerate(instruction_addresses)
        if address is not None and address_counts[address] == 1
    }
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        candidate.instructions,
        instruction_addresses=instruction_addresses,
        instruction_index_by_address=instruction_index_by_address,
        source="cod",
        caller_start=0,
        caller_end=len(definition.data),
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    load_index = load_rows[0][1]
    entry_reachable = _cc_cfg._reachable_cfg_indices(successors, (0,))
    successors_before_reload = {
        index: tuple(
            target for target in targets if target != load_index
        )
        for index, targets in successors.items()
    }
    after_load_before_reload = _cc_cfg._reachable_cfg_indices(
        successors_before_reload,
        successors.get(load_index, ()),
    )
    without_load = _cc_cfg._reachable_cfg_indices(
        successors,
        (0,),
        blocked=frozenset({load_index}),
    )
    for _offset, call_index, _instruction in cached_call_rows:
        proof_nodes = (
            after_load_before_reload
            & _cc_cfg._indices_reaching_cfg_target(
                successors_before_reload,
                call_index,
            )
        )
        if (
            call_index not in entry_reachable
            or call_index not in after_load_before_reload
            or call_index in without_load
            or unresolved & proof_nodes
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    candidate.instructions[index],
                    "edi",
                )
                for index in proof_nodes
                if index != call_index
            )
        ):
            raise ValueError(
                "zEffect SaveRunningAnimRecord fwrite provenance rejects an "
                "ambiguous, bypassed, unresolved, or clobbered EDI reaching "
                "definition"
            )

    return (
        frozenset(
            {
                normalize_address(
                    _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_LOAD_OFFSET
                )
            }
        ),
        {
            _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IMPORT_SYMBOL:
            _cc_catalog.ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IAT_IDENTITY
        },
    )


def _checked_result_reaches_transfer(
    instructions: Sequence[Instruction], *, source: str, caller_start: int,
    result_index: int, transfer_index: int, register: str,
    local_control_flow_targets: Mapping[int, Sequence[int]] | None = None,
) -> bool:
    """Require a checked callee result on every path to its invocation.

    Returned failure arms have no edge to the use. Their calls cannot clobber
    the success path, while a failure arm that rejoins it still blocks proof.
    The owning producer authenticates the result-producing call independently.
    """
    addresses = _cc_cfg._instruction_runtime_addresses(instructions, source=source, caller_start=caller_start)
    counts = Counter(addresses)
    indices = {address: index for index, address in enumerate(addresses)
               if address is not None and counts[address] == 1}
    end = max((address + len(row.bytes) for address, row in zip(addresses, instructions)
               if address is not None), default=caller_start)
    targets = dict(local_control_flow_targets or {})
    successors, unresolved = _cc_cfg._exact_invocation_cfg(instructions,
        instruction_addresses=addresses, instruction_index_by_address=indices,
        source=source, caller_start=caller_start, caller_end=end,
        local_control_flow_indices=frozenset(targets), local_control_flow_targets=targets)
    uses = _cc_flow.reaching_definition_uses(instructions,
        _cc_flow.ControlFlow.from_edges(len(instructions), successors, unresolved),
        definition_index=result_index, register=register)
    return transfer_index in uses


def _reviewed_dynamic_export_call_bridges(
    spec: ReviewedDynamicExportCallSpec,
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge_data_rows: Sequence[Any],
    bridge: BinaryNinjaBridge,
) -> tuple[
    dict[str, ReviewedCallResultBridge],
    dict[str, ReviewedCallResultBridge],
]:
    """Prove one fixed GetProcAddress result used as an exact provider call."""
    from _recoil.call_contract.records import (
        CurrentIatStoragePackage,
        ReviewedCallResultBridge,
    )
    if (
        caller_identity != spec.caller_identity
        or normalize_address(caller_start) != spec.caller_start
        or normalize_address(caller_end_exclusive) != spec.caller_end_exclusive
    ):
        return {}, {}

    definition = candidate.caller_definition
    if definition is None or definition.symbol != spec.caller_symbol:
        raise ValueError(
            f"{spec.label} requires exact candidate caller symbol "
            f"{spec.caller_symbol!r}"
        )

    iat_identity = f"iat:{spec.getproc_import_name}"
    packages = [
        package
        for package in _cc_storage_identity._current_tracker_iat_storage_packages(document)
        if package.address == spec.getproc_iat_address
    ]
    if (
        packages
        != [
            CurrentIatStoragePackage(
                address=spec.getproc_iat_address,
                import_dll=spec.getproc_import_dll,
                import_name=spec.getproc_import_name,
                import_ordinal=None,
                object_symbol=spec.getproc_import_symbol,
                identity=iat_identity,
            )
        ]
        or indexes.storage_by_address.get(spec.getproc_iat_address)
        != iat_identity
        or indexes.storage_by_name.get(spec.getproc_import_symbol)
        != iat_identity
    ):
        raise ValueError(
            f"{spec.label} requires the exact current GetProcAddress IAT package"
        )

    symbols = document.collection("symbols")
    owners = document.collection("owners")
    export_data = symbols.get(spec.export_data_id)
    if (
        not isinstance(export_data, Mapping)
        or export_data.get("binary") != "recoil"
        or export_data.get("kind") != "data"
        or export_data.get("disposition") != "authored"
        or export_data.get("address") != spec.export_address
        or export_data.get("navigation_name") != spec.export_navigation_name
        or export_data.get("output_section_id") != "recoil:section:.data"
        or export_data.get("source_traceability", {}).get("state")
        != "resolved"
    ):
        raise ValueError(
            f"{spec.label} requires its exact authored export-name data identity"
        )

    provider = symbols.get(spec.provider_function_id)
    owner = owners.get(spec.provider_owner_id)
    expected_provider_relationship = {
        "address": spec.provider_address,
        "kind": "primary-function",
        "symbol_id": spec.provider_function_id,
    }
    if (
        not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "provider-boundary"
        or owner.get("lifecycle_state") != "accepted"
        or owner.get("provider_state") != "accepted"
        or owner.get("source_paths") != []
        or owner.get("gates", {}).get("boundary") != "accepted"
        or owner.get("gates", {}).get("source") != "accepted"
        or owner.get("relationships", []).count(
            expected_provider_relationship
        )
        != 1
        or not isinstance(provider, Mapping)
        or provider.get("binary") != "recoil"
        or provider.get("kind") != "function"
        or provider.get("address") != spec.provider_address
        or provider.get("end_exclusive") != spec.provider_end_exclusive
        or provider.get("extent_state") != "known"
        or provider.get("pipeline_class") != "non-authored"
        or provider.get("authored_order_role") != "non-authored"
        or provider.get("ownership_state") != "primary-owned"
        or provider.get("physical_block_id")
        != spec.provider_physical_block_id
        or provider.get("navigation_name")
        != spec.provider_navigation_name
    ):
        raise ValueError(
            f"{spec.label} requires its exact accepted provider owner/function"
        )
    provider_identity = f"provider:{spec.provider_function_id}"
    if provider_identity not in indexes.provider_ids:
        raise ValueError(
            f"{spec.label} provider function is not the indexed provider identity"
        )

    export_rows = [
        row
        for row in bridge_data_rows
        if normalize_address(str(getattr(row, "address", "")))
        == spec.export_address
    ]
    export_bytes = spec.export_value.encode("ascii") + b"\0"
    if (
        len(export_rows) != 1
        or getattr(export_rows[0], "name", "")
        != spec.export_navigation_name
        or int(getattr(export_rows[0], "size", 0)) != len(export_bytes)
        or _cc_cfg._hexdump_bytes(
            bridge.hexdump(spec.export_address, len(export_bytes))
        )
        != export_bytes
    ):
        raise ValueError(
            f"{spec.label} requires exact live retail export-name bytes and name"
        )

    def exact_instruction(
        instructions: Sequence[Instruction],
        address: str,
        mnemonic: str,
        body: bytes,
        *,
        source: str,
        operand: str | None = None,
    ) -> tuple[int, Instruction]:
        expected_address = address_value(address)
        if source == "cod":
            expected_address += address_value(caller_start)
        runtime_addresses = _cc_cfg._instruction_runtime_addresses(
            instructions,
            source=source,
            caller_start=address_value(caller_start),
        )
        rows = [
            (index, instruction)
            for index, instruction in enumerate(instructions)
            if runtime_addresses[index] == expected_address
            and _cc_cfg._instruction_mnemonic(instruction) == mnemonic
            and (
                operand is None
                or _cc_cfg._instruction_operand(instruction).lower()
                == operand.lower()
            )
            and tuple(value.lower() for value in instruction.bytes)
            == tuple(f"{value:02x}" for value in body)
        ]
        if len(rows) != 1:
            raise ValueError(
                f"{spec.label} requires one exact {address} {mnemonic} row"
            )
        return rows[0]

    retail_getproc = exact_instruction(
        retail_instructions,
        spec.retail_getproc_call_address,
        "call",
        spec.retail_getproc_call_body,
        source="bn",
        operand=f"dword [{spec.getproc_import_name}]",
    )
    retail_test = exact_instruction(
        retail_instructions,
        spec.retail_test_address,
        "test",
        spec.retail_test_body,
        source="bn",
        operand=f"{spec.register}, {spec.register}",
    )
    retail_branch = exact_instruction(
        retail_instructions,
        spec.retail_branch_address,
        "jne",
        spec.retail_branch_body,
        source="bn",
    )
    retail_dynamic = exact_instruction(
        retail_instructions,
        spec.retail_dynamic_call_address,
        "call",
        spec.retail_dynamic_call_body,
        source="bn",
        operand=spec.register,
    )
    candidate_getproc = exact_instruction(
        candidate.instructions,
        spec.candidate_getproc_call_address,
        "call",
        spec.candidate_getproc_call_body,
        source="cod",
        operand=f"dword {spec.getproc_import_symbol}",
    )
    candidate_test = exact_instruction(
        candidate.instructions,
        spec.candidate_test_address,
        "test",
        spec.candidate_test_body,
        source="cod",
        operand=f"{spec.register}, {spec.register}",
    )
    candidate_branch = exact_instruction(
        candidate.instructions,
        spec.candidate_branch_address,
        "jne",
        spec.candidate_branch_body,
        source="cod",
    )
    candidate_dynamic = exact_instruction(
        candidate.instructions,
        spec.candidate_dynamic_call_address,
        "call",
        spec.candidate_dynamic_call_body,
        source="cod",
        operand=spec.register,
    )
    retail_getproc_setup = [
        exact_instruction(
            retail_instructions,
            "0x40c490",
            "push",
            b"\x68\x78\xab\x4d\x00",
            source="bn",
            operand=spec.export_address,
        ),
        exact_instruction(
            retail_instructions,
            "0x40c495",
            "push",
            b"\x53",
            source="bn",
            operand="ebx",
        ),
    ]
    candidate_getproc_setup = [
        exact_instruction(
            candidate.instructions,
            "0x120",
            "push",
            b"\x68\x00\x00\x00\x00",
            source="cod",
        ),
        exact_instruction(
            candidate.instructions,
            "0x125",
            "push",
            b"\x53",
            source="cod",
            operand="ebx",
        ),
    ]
    retail_provider_setup = [
        exact_instruction(
            retail_instructions, "0x40c4c2", "lea", b"\x8d\x4c\x24\x10",
            source="bn", operand="ecx, [esp+0x10]",
        ),
        exact_instruction(
            retail_instructions, "0x40c4c6", "push", b"\x6a\x00",
            source="bn", operand="0x0",
        ),
        exact_instruction(
            retail_instructions, "0x40c4c8", "push", b"\x51",
            source="bn", operand="ecx",
        ),
        exact_instruction(
            retail_instructions, "0x40c4c9", "push", b"\x6a\x00",
            source="bn", operand="0x0",
        ),
    ]
    candidate_provider_setup = [
        exact_instruction(
            candidate.instructions, "0x152", "lea", b"\x8d\x4c\x24\x10",
            source="cod", operand="ecx, dword _pddraw$[esp+292]",
        ),
        exact_instruction(
            candidate.instructions, "0x156", "push", b"\x6a\x00",
            source="cod", operand="0",
        ),
        exact_instruction(
            candidate.instructions, "0x158", "push", b"\x51",
            source="cod", operand="ecx",
        ),
        exact_instruction(
            candidate.instructions, "0x159", "push", b"\x6a\x00",
            source="cod", operand="0",
        ),
    ]
    if (
        _cc_cfg._instruction_operand(retail_getproc[1]).lower()
        != f"dword [{spec.getproc_import_name.lower()}]"
        or _cc_cfg._instruction_operand(candidate_getproc[1]).lower()
        != f"dword {spec.getproc_import_symbol.lower()}"
        or _cc_receiver_cursor._exact_test_same_register(retail_test[1]) != spec.register
        or _cc_receiver_cursor._exact_test_same_register(candidate_test[1]) != spec.register
        or _cc_cfg._instruction_operand(retail_dynamic[1]).lower() != spec.register
        or _cc_cfg._instruction_operand(candidate_dynamic[1]).lower() != spec.register
        or not (
            retail_getproc_setup[0][0] + 1
            == retail_getproc_setup[1][0]
            and retail_getproc_setup[1][0] + 1 == retail_getproc[0]
            and candidate_getproc_setup[0][0] + 1
            == candidate_getproc_setup[1][0]
            and candidate_getproc_setup[1][0] + 1
            == candidate_getproc[0]
            and [row[0] for row in retail_provider_setup]
            == list(range(retail_provider_setup[0][0], retail_dynamic[0]))
            and [row[0] for row in candidate_provider_setup]
            == list(range(candidate_provider_setup[0][0], candidate_dynamic[0]))
            and
            retail_getproc[0] + 1 == retail_test[0]
            and retail_test[0] + 1 == retail_branch[0]
            and candidate_getproc[0] + 1 == candidate_test[0]
            and candidate_test[0] + 1 == candidate_branch[0]
        )
    ):
        raise ValueError(
            f"{spec.label} GetProcAddress result test/call topology drifted"
        )

    for label, instructions, getproc_row, branch_row, dynamic_row in (
        ("retail", retail_instructions, retail_getproc, retail_branch, retail_dynamic),
        ("candidate", candidate.instructions, candidate_getproc, candidate_branch, candidate_dynamic),
    ):
        if (
            dynamic_row[0] <= branch_row[0]
            or not _checked_result_reaches_transfer(instructions,
                source="bn" if label == "retail" else "cod",
                caller_start=address_value(spec.caller_start), result_index=getproc_row[0],
                transfer_index=dynamic_row[0], register=spec.register,
                local_control_flow_targets=candidate.local_control_flow_targets if label == "candidate" else {})
            or sum(
                _cc_cfg._instruction_mnemonic(instruction) == "call"
                and _cc_cfg._instruction_operand(instruction).lower() == spec.register
                for instruction in instructions
            ) != spec.invocation_count
        ):
            raise ValueError(
                f"{spec.label} {label} checked result is clobbered or ambiguous"
            )

    def exact_dir32_relocation(symbol: str, offset: int) -> bool:
        rows = [
            row
            for row in definition.relocations
            if row.symbol_name == symbol and row.offset == offset
        ]
        return (
            len(rows) == 1
            and rows[0].type == IMAGE_REL_I386_DIR32
            and rows[0].offset == offset
            and offset + 4 <= len(definition.data)
            and struct.unpack_from("<I", definition.data, offset)[0] == 0
            and all(definition.relocation_mask[index] for index in range(offset, offset + 4))
        )

    export_relocation_symbols = {
        row.symbol_name
        for row in definition.relocations
        if (
            row.offset == spec.candidate_export_relocation_offset
            and re.fullmatch(
                spec.candidate_export_symbol_pattern,
                row.symbol_name,
            )
        )
    }
    if (
        len(export_relocation_symbols) != 1
        or not exact_dir32_relocation(
            next(iter(export_relocation_symbols), ""),
            spec.candidate_export_relocation_offset,
        )
        or not exact_dir32_relocation(
            spec.getproc_import_symbol,
            spec.candidate_getproc_relocation_offset,
        )
        or definition.undefined_external_functions.count(
            spec.getproc_import_symbol
        )
        != 1
        or definition.data[
            spec.candidate_export_relocation_offset - 1
            : spec.candidate_export_relocation_offset
        ]
        != b"\x68"
        or definition.data[
            spec.candidate_getproc_relocation_offset - 2
            : spec.candidate_getproc_relocation_offset
        ]
        != b"\xff\x15"
    ):
        raise ValueError(
            f"{spec.label} requires exact export-name and GetProcAddress "
            "candidate DIR32 relocations"
        )

    provenance = f"call-result({iat_identity})"
    retail_bridges = {
        spec.retail_dynamic_call_address: ReviewedCallResultBridge(
            register=spec.register,
            provenance=provenance,
            target_identity=provider_identity,
        )
    }
    candidate_bridges = {
        spec.candidate_dynamic_call_address: ReviewedCallResultBridge(
            register=spec.register,
            provenance=provenance,
            target_identity=provider_identity,
        )
    }
    retail_contract = _cc_extraction.extract_invocation_contract(
        retail_instructions,
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        reviewed_call_result_bridges=retail_bridges,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        reviewed_call_result_bridges=candidate_bridges,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    required_row = {
        "ordinal": spec.ordinal,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "provider",
        "target_identity": provider_identity,
        "storage_identity": provenance,
        "slot_displacement": None,
        "cleanup_bytes": spec.cleanup_bytes,
    }
    if (
        spec.ordinal >= len(retail_contract)
        or spec.ordinal >= len(candidate_contract)
        or retail_contract[spec.ordinal] != required_row
        or candidate_contract[spec.ordinal] != required_row
        or sum(row == required_row for row in retail_contract)
        != spec.invocation_count
        or sum(row == required_row for row in candidate_contract)
        != spec.invocation_count
    ):
        raise ValueError(
            f"{spec.label} lacks its exact ordinal provider call contract"
        )
    return retail_bridges, candidate_bridges


def _zsys_directdrawcreate_dynamic_export_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    **kwargs: Any,
) -> tuple[
    dict[str, ReviewedCallResultBridge],
    dict[str, ReviewedCallResultBridge],
]:
    return _reviewed_dynamic_export_call_bridges(
        _cc_catalog.ZSYS_DIRECTDRAWCREATE_DYNAMIC_EXPORT_BRIDGE,
        retail_instructions,
        candidate,
        **kwargs,
    )


def _reviewed_direct_iat_storage_call_bridge(
    spec: ReviewedDirectIatStorageCallSpec,
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge_data_rows: Sequence[Any],
) -> None:
    """Prove one exact tracker-backed direct call through the CRT IAT slot."""
    if (
        caller_identity != spec.caller_identity
        or normalize_address(caller_start) != spec.caller_start
        or normalize_address(caller_end_exclusive) != spec.caller_end_exclusive
    ):
        return

    iat_identity = f"iat:{spec.import_name}"
    function_id = f"recoil:function:{spec.retail_storage_address}"
    data_id = f"recoil:data:{spec.retail_storage_address}"
    storage_id = f"recoil:storage:va:{spec.retail_storage_address}"
    owner = document.collection("owners").get(spec.owner_id)
    function = document.collection("symbols").get(function_id)
    data = document.collection("symbols").get(data_id)
    storage = document.collection("storage_contributions").get(storage_id)
    end_exclusive = normalize_address(
        address_value(spec.retail_storage_address) + 4
    )
    function_end = normalize_address(
        address_value(spec.retail_storage_address) + 1
    )
    expected_relationships = [
        {
            "kind": "anchor-address",
            "address": spec.retail_storage_address,
        },
        {
            "kind": "primary-function",
            "address": spec.retail_storage_address,
            "symbol_id": function_id,
        },
        {
            "kind": "primary-data",
            "address": spec.retail_storage_address,
            "symbol_id": data_id,
            "name": f"{spec.import_dll}!{spec.import_name} IAT",
        },
    ]
    if (
        not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "provider-boundary"
        or owner.get("blocker") != "none"
        or owner.get("lifecycle_state") != "accepted"
        or owner.get("provider_state") != "accepted"
        or owner.get("relationships") != expected_relationships
        or owner.get("source_paths") != []
    ):
        raise ValueError(
            f"{spec.label} requires its exact accepted tracker provider owner"
        )
    if (
        not isinstance(function, Mapping)
        or function.get("binary") != "recoil"
        or function.get("kind") != "provider-function"
        or function.get("disposition") != "provider"
        or function.get("pipeline_class") != "non-authored"
        or function.get("authored_order_role") != "non-authored"
        or function.get("address") != spec.retail_storage_address
        or function.get("end_exclusive") != function_end
        or function.get("extent_state") != "known"
        or function.get("size") != 1
        or function.get("import_dll") != spec.import_dll
        or function.get("import_name") != spec.import_name
        or "import_ordinal" in function
        or function.get("object_symbol") != spec.candidate_import_symbol
        or function.get("output_section_id") != "recoil:section:.rdata"
        or function.get("ownership_state") != "primary-owned"
        or function.get("physical_block_id") is not None
        or function.get("storage_contribution_ids") != []
    ):
        raise ValueError(
            f"{spec.label} requires its exact tracker provider-function view"
        )
    if (
        not isinstance(data, Mapping)
        or data.get("binary") != "recoil"
        or data.get("kind") != "data"
        or data.get("disposition") != "provider"
        or data.get("address") != spec.retail_storage_address
        or data.get("end_exclusive") != end_exclusive
        or data.get("extent_state") != "known"
        or data.get("size") != 4
        or data.get("import_dll") != spec.import_dll
        or data.get("import_name") != spec.import_name
        or "import_ordinal" in data
        or data.get("output_section_id") != "recoil:section:.rdata"
        or data.get("ownership_state") != "primary-owned"
        or data.get("physical_block_id") is not None
        or data.get("storage_contribution_ids") != [storage_id]
    ):
        raise ValueError(
            f"{spec.label} requires its exact four-byte tracker provider-data view"
        )
    reference = storage.get("reference") if isinstance(storage, Mapping) else None
    extent = (
        storage.get("verification", {}).get("extent")
        if isinstance(storage, Mapping)
        and isinstance(storage.get("verification"), Mapping)
        else None
    )
    if (
        not isinstance(storage, Mapping)
        or storage.get("binary") != "recoil"
        or storage.get("kind") != "provider-data"
        or storage.get("output_section_id") != "recoil:section:.rdata"
        or storage.get("overlap") != "none"
        or storage.get("owner_ids") != [spec.owner_id]
        or storage.get("parent_contribution_id") is not None
        or storage.get("symbol_ids") != [data_id]
        or not isinstance(reference, Mapping)
        or reference.get("address") != spec.retail_storage_address
        or reference.get("end_exclusive") != end_exclusive
        or reference.get("extent_state") != "known"
        or reference.get("size") != 4
        or not isinstance(extent, Mapping)
        or extent.get("result") != "passed"
        or extent.get("disposition") != "accepted"
        or extent.get("freshness") != "current"
        or extent.get("validation_mode") != "live"
        or extent.get("gating") is not True
    ):
        raise ValueError(
            f"{spec.label} requires its exact current tracker IAT storage extent"
        )
    shared_evidence = set(owner.get("evidence_ids", ()))
    for row in (function, data, storage, reference, extent):
        shared_evidence &= set(row.get("evidence_ids", ()))
    current_evidence = [
        row
        for evidence_id in shared_evidence
        if isinstance(
            row := document.collection("evidence").get(evidence_id),
            Mapping,
        )
        and row.get("result") == "passed"
        and row.get("disposition") == "accepted"
        and row.get("freshness") == "current"
        and row.get("validation_mode") == "live"
        and row.get("gating") is True
        and isinstance(row.get("provenance"), Mapping)
        and row["provenance"].get("candidate_independent") is True
        and row["provenance"].get("address")
        == spec.retail_storage_address
        and row["provenance"].get("dll") == spec.import_dll
        and row["provenance"].get("import_name") == spec.import_name
        and row["provenance"].get("import_ordinal") is None
        and row["provenance"].get("object_symbol")
        == spec.candidate_import_symbol
        and row["provenance"].get("object_symbol_basis")
        == "reviewed-vc5-provider-declaration"
    ]
    if len(current_evidence) != 1:
        raise ValueError(
            f"{spec.label} requires one shared current candidate-independent "
            "provider-registration evidence row"
        )
    if (
        indexes.storage_by_address.get(spec.retail_storage_address)
        != iat_identity
        or indexes.storage_by_name.get(spec.import_name) != iat_identity
        or indexes.storage_by_name.get(spec.candidate_import_symbol)
        != iat_identity
    ):
        raise ValueError(
            f"{spec.label} tracker package is not the exact indexed IAT identity"
        )

    retail_data_rows = [
        row
        for row in bridge_data_rows
        if normalize_address(str(getattr(row, "address", "")))
        == spec.retail_storage_address
    ]
    if len(retail_data_rows) != 1:
        raise ValueError(
            f"{spec.label} requires one exact live retail data row at "
            f"{spec.retail_storage_address}"
        )
    retail_data = retail_data_rows[0]
    retail_type = re.sub(
        r"\s+",
        " ",
        str(getattr(retail_data, "type_text", "")).strip(),
    )
    if (
        getattr(retail_data, "name", "") != spec.import_name
        or getattr(retail_data, "raw_name", "")
        not in {"", spec.import_name}
        or int(getattr(retail_data, "size", 0)) != 4
        or retail_type != spec.retail_storage_type
    ):
        raise ValueError(
            f"{spec.label} retail storage is not the exact typed "
            f"{spec.import_name} import pointer"
        )

    retail_calls = [
        instruction
        for instruction in retail_instructions
        if _cc_cfg._source_instruction_address(instruction)
        == spec.retail_call_address
        and _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction).lower()
        == f"dword [{spec.import_name.lower()}]"
        and tuple(value.lower() for value in instruction.bytes)
        == tuple(
            f"{value:02x}"
            for value in (
                b"\xff\x15"
                + struct.pack(
                    "<I",
                    address_value(spec.retail_storage_address),
                )
            )
        )
    ]
    candidate_calls = [
        instruction
        for instruction in candidate.instructions
        if (
            _cc_cfg._source_instruction_address(instruction)
            == spec.candidate_call_address
            or not _cc_cfg._source_instruction_address(instruction)
        )
        and _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction).lower()
        == f"dword {spec.candidate_import_symbol.lower()}"
        and tuple(value.lower() for value in instruction.bytes)
        == ("ff", "15", "00", "00", "00", "00")
    ]
    if len(retail_calls) != 1 or len(candidate_calls) != 1:
        raise ValueError(
            f"{spec.label} requires one exact retail "
            f"{spec.retail_call_address} and candidate COD "
            f"+{spec.candidate_call_address} FF15 call"
        )
    retail_call = retail_calls[0]
    candidate_call = candidate_calls[0]
    if (
        sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            and spec.import_name.lower()
            in _cc_cfg._instruction_operand(instruction).lower()
            for instruction in retail_instructions
        )
        != spec.invocation_count
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            and spec.candidate_import_symbol.lower()
            in _cc_cfg._instruction_operand(instruction).lower()
            for instruction in candidate.instructions
        )
        != spec.invocation_count
    ):
        raise ValueError(
            f"{spec.label} requires exactly one retail and candidate invocation"
        )

    definition = candidate.caller_definition
    if definition is None or definition.symbol != spec.caller_symbol:
        raise ValueError(
            f"{spec.label} requires exact candidate caller symbol "
            f"{spec.caller_symbol!r}"
        )
    references = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name == spec.candidate_import_symbol
    )
    if (
        definition.undefined_external_functions.count(
            spec.candidate_import_symbol
        )
        != 1
        or len(references) != 1
        or references[0].type != IMAGE_REL_I386_DIR32
        or references[0].offset != spec.candidate_relocation_offset
        or references[0].offset < 2
        or references[0].offset + 4 > len(definition.data)
        or definition.data[
            references[0].offset - 2 : references[0].offset
        ]
        != b"\xff\x15"
        or struct.unpack_from(
            "<I", definition.data, references[0].offset
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(
                references[0].offset,
                references[0].offset + 4,
            )
        )
    ):
        raise ValueError(
            f"{spec.label} requires one exact undefined external "
            f"{spec.candidate_import_symbol} FF15 DIR32 relocation at "
            f"+{hex(spec.candidate_relocation_offset)}"
        )

    retail_call_index = next(
        index
        for index, instruction in enumerate(retail_instructions)
        if instruction is retail_call
    )
    candidate_call_index = next(
        index
        for index, instruction in enumerate(candidate.instructions)
        if instruction is candidate_call
    )
    retail_contract = _cc_extraction.extract_invocation_contract(
        retail_instructions[: retail_call_index + 2],
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        candidate.instructions[: candidate_call_index + 2],
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        local_control_flow_indices=frozenset(
            index
            for index in candidate.local_control_flow_indices
            if index <= candidate_call_index + 1
        ),
        local_control_flow_targets={
            index: targets
            for index, targets in candidate.local_control_flow_targets.items()
            if (
                index <= candidate_call_index + 1
                and all(target <= candidate_call_index + 1 for target in targets)
            )
        },
    )
    required_row = {
        "ordinal": spec.ordinal,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "iat",
        "target_identity": iat_identity,
        "storage_identity": iat_identity,
        "slot_displacement": None,
        "cleanup_bytes": spec.cleanup_bytes,
    }
    if (
        len(retail_contract) != spec.ordinal + 1
        or len(candidate_contract) != spec.ordinal + 1
        or retail_contract[spec.ordinal] != required_row
        or candidate_contract[spec.ordinal] != required_row
        or sum(
            row.get("target_identity") == iat_identity
            for row in retail_contract
        )
        != spec.invocation_count
        or sum(
            row.get("target_identity") == iat_identity
            for row in candidate_contract
        )
        != spec.invocation_count
    ):
        raise ValueError(
            f"{spec.label} lacks the exact same ordinal {spec.ordinal}, "
            "direct-IAT FF15 form, tracker identity, and caller cleanup "
            f"+{spec.cleanup_bytes}"
        )


def _briefing_beginthread_direct_iat_storage_bridge(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    **kwargs: Any,
) -> None:
    _reviewed_direct_iat_storage_call_bridge(
        _cc_catalog.BRIEFING_BEGINTHREAD_DIRECT_IAT_STORAGE_BRIDGE,
        retail_instructions,
        candidate,
        **kwargs,
    )


def _briefing_strerror_direct_iat_storage_bridge(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    **kwargs: Any,
) -> None:
    _reviewed_direct_iat_storage_call_bridge(
        _cc_catalog.BRIEFING_STRERROR_DIRECT_IAT_STORAGE_BRIDGE,
        retail_instructions,
        candidate,
        **kwargs,
    )


def _contract_shape_without_target(row: Mapping[str, Any]) -> dict[str, Any]:
    return {
        key: value
        for key, value in row.items()
        if key not in {"identity_kind", "target_identity"}
    }


def _contract_shape_without_storage(row: Mapping[str, Any]) -> dict[str, Any]:
    return {
        key: value
        for key, value in row.items()
        if key != "storage_identity"
    }


def _hud_ui_element_constructor_absolute_table_candidate_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, str]:
    """Pair the exact HudUiElement constructor FF15 with retail storage.

    VC5 folds the constructor's immediate Invalidate dispatch into an FF15
    relocation against the same COMDAT vftable symbol that the constructor
    has just installed.  That candidate expression is neither an IAT nor a
    statically resolved callee.  Publish only the comparison-scoped storage
    name after the complete caller/vftable COFF relation proves the exact
    retail targetless constructor-table row.
    """
    from _recoil.call_contract.records import StorageContainer

    start = normalize_address(caller_start)
    if start != _cc_catalog.HUD_ELEMENT_CONSTRUCTOR_CALLER_START:
        return {}
    if (
        normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_ELEMENT_CONSTRUCTOR_CALLER_END_EXCLUSIVE
        or not caller_identity
        or indexes.by_address.get(start) != caller_identity
    ):
        raise ValueError(
            "HudUiElement constructor-table candidate bridge is restricted "
            "to its exact reviewed caller"
        )

    required_expected = [
        {
            "ordinal": 0,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "callback",
            "target_identity": "",
            "storage_identity": _cc_catalog.HUD_ELEMENT_VFTABLE_STORAGE_IDENTITY,
            "slot_displacement": _cc_catalog.HUD_ELEMENT_INVALIDATE_SLOT_DISPLACEMENT,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 1,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": _cc_catalog.HUD_ELEMENT_SET_BLT_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
    ]
    storage_start = address_value(_cc_catalog.HUD_ELEMENT_VFTABLE_RETAIL_ADDRESS)
    exact_containers = [
        row
        for row in indexes.storage_containers
        if row.start < storage_start + _cc_catalog.HUD_ELEMENT_VFTABLE_SIZE
        and storage_start < row.end_exclusive
    ]
    if (
        list(expected) != required_expected
        or indexes.storage_by_address.get(
            _cc_catalog.HUD_ELEMENT_VFTABLE_RETAIL_ADDRESS,
            "",
        )
        or _cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL in indexes.storage_by_name
        or exact_containers
        != [
            StorageContainer(
                start=storage_start,
                end_exclusive=storage_start + _cc_catalog.HUD_ELEMENT_VFTABLE_SIZE,
                identity=_cc_catalog.HUD_ELEMENT_VFTABLE_STORAGE_IDENTITY,
            )
        ]
    ):
        raise ValueError(
            "HudUiElement constructor-table candidate bridge lacks the "
            "exact targetless retail storage row and extent: "
            f"expected={list(expected)!r}, "
            f"storage_by_address="
            f"{indexes.storage_by_address.get(_cc_catalog.HUD_ELEMENT_VFTABLE_RETAIL_ADDRESS, '')!r}, "
            f"storage_by_name="
            f"{indexes.storage_by_name.get(_cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL, '')!r}, "
            f"overlapping_containers={exact_containers!r}"
        )

    caller = candidate.caller_definition
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_ELEMENT_CONSTRUCTOR_SYMBOL
        or len(caller.data) <= _cc_catalog.HUD_ELEMENT_INVALIDATE_CALL_RELOCATION_OFFSET + 3
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HudUiElement constructor-table candidate bridge lacks the "
            "exact complete constructor COFF body"
        )
    vftable_casefold = _cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL.casefold()
    references = [
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name.casefold() == vftable_casefold
    ]
    set_blt_references = [
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name == _cc_catalog.HUD_ELEMENT_SET_BLT_SYMBOL
    ]
    if (
        tuple(
            (relocation.offset, relocation.type, relocation.symbol_name)
            for relocation in references
        )
        != (
            (
                _cc_catalog.HUD_ELEMENT_VPTR_STORE_RELOCATION_OFFSET,
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL,
            ),
            (
                _cc_catalog.HUD_ELEMENT_INVALIDATE_CALL_RELOCATION_OFFSET,
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL,
            ),
        )
        or caller.data[
            _cc_catalog.HUD_ELEMENT_VPTR_STORE_RELOCATION_OFFSET - 2 :
            _cc_catalog.HUD_ELEMENT_VPTR_STORE_RELOCATION_OFFSET
        ]
        != b"\xc7\x06"
        or struct.unpack_from(
            "<I",
            caller.data,
            _cc_catalog.HUD_ELEMENT_VPTR_STORE_RELOCATION_OFFSET,
        )[0]
        != 0
        or caller.data[
            _cc_catalog.HUD_ELEMENT_INVALIDATE_CALL_OFFSET :
            _cc_catalog.HUD_ELEMENT_INVALIDATE_CALL_OFFSET + 6
        ]
        != b"\xff\x15\x20\x00\x00\x00"
        or struct.unpack_from(
            "<I",
            caller.data,
            _cc_catalog.HUD_ELEMENT_INVALIDATE_CALL_RELOCATION_OFFSET,
        )[0]
        != _cc_catalog.HUD_ELEMENT_INVALIDATE_SLOT_DISPLACEMENT
        or any(
            not all(
                caller.relocation_mask[index]
                for index in range(relocation.offset, relocation.offset + 4)
            )
            for relocation in references
        )
        or tuple(
            (relocation.offset, relocation.type, relocation.symbol_name)
            for relocation in set_blt_references
        )
        != (
            (
                _cc_catalog.HUD_ELEMENT_SET_BLT_CALL_RELOCATION_OFFSET,
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_ELEMENT_SET_BLT_SYMBOL,
            ),
        )
        or caller.data[
            _cc_catalog.HUD_ELEMENT_SET_BLT_CALL_OFFSET : _cc_catalog.HUD_ELEMENT_SET_BLT_CALL_OFFSET + 5
        ]
        != b"\xe8\x00\x00\x00\x00"
        or not all(
            caller.relocation_mask[index]
            for index in range(
                _cc_catalog.HUD_ELEMENT_SET_BLT_CALL_RELOCATION_OFFSET,
                _cc_catalog.HUD_ELEMENT_SET_BLT_CALL_RELOCATION_OFFSET + 4,
            )
        )
        or indexes.by_address.get("0x4b4190")
        != _cc_catalog.HUD_ELEMENT_SET_BLT_IDENTITY
        or indexes.by_candidate_name.get(_cc_catalog.HUD_ELEMENT_SET_BLT_SYMBOL)
        != _cc_catalog.HUD_ELEMENT_SET_BLT_IDENTITY
    ):
        raise ValueError(
            "HudUiElement constructor-table candidate bridge requires the "
            "same exact vftable zero-addend store, slot-0x20 FF15 DIR32, and "
            "authored SetBltSourceAndClipRect REL32 call relation"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    offset_counts = Counter(offset for offset in offsets if offset is not None)
    instruction_by_offset = {
        offset: instruction
        for offset, instruction in zip(offsets, candidate.instructions)
        if offset is not None and offset_counts[offset] == 1
    }
    call = instruction_by_offset.get(_cc_catalog.HUD_ELEMENT_INVALIDATE_CALL_OFFSET)
    set_blt_call = instruction_by_offset.get(_cc_catalog.HUD_ELEMENT_SET_BLT_CALL_OFFSET)
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        call is None
        or set_blt_call is None
        or len(invocation_indices) != 2
        or tuple(candidate.instructions[index] for index in invocation_indices)
        != (call, set_blt_call)
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or bytes(int(value, 16) for value in call.bytes)
        != b"\xff\x15\x20\x00\x00\x00"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {
            f"{_cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL}+32",
            f"{_cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL}+0x20",
        }
        or _cc_targets._memory_slot(_cc_cfg._instruction_operand(call))[1]
        != _cc_catalog.HUD_ELEMENT_INVALIDATE_SLOT_DISPLACEMENT
        or _cc_cfg._instruction_mnemonic(set_blt_call) != "call"
        or bytes(int(value, 16) for value in set_blt_call.bytes)
        != b"\xe8\x00\x00\x00\x00"
        or _cc_cfg._instruction_operand(set_blt_call) != _cc_catalog.HUD_ELEMENT_SET_BLT_SYMBOL
    ):
        raise ValueError(
            "HudUiElement constructor-table candidate bridge requires its "
            "exact ordinal-0 body+0x25 vftable-slot call followed only by "
            "the ordinal-1 body+0x36 authored call"
        )

    definitions = [
        (name, definition)
        for name, definition in candidate.vftable_definitions.items()
        if name.casefold() == vftable_casefold
    ]
    if len(definitions) != 1 or definitions[0][0] != _cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL:
        raise ValueError(
            "HudUiElement constructor-table candidate bridge rejects "
            "missing, aliased, or ambiguous vftable definitions"
        )
    vftable = definitions[0][1]
    if (
        vftable.symbol != _cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL
        or vftable.section_name != ".rdata"
        or not vftable.section_is_comdat
        or vftable.comdat_selection != 2
        or vftable.section_external_symbols != (_cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL,)
        or vftable.section_size != _cc_catalog.HUD_ELEMENT_VFTABLE_SIZE
        or vftable.data != b"\x00" * _cc_catalog.HUD_ELEMENT_VFTABLE_SIZE
        or len(vftable.relocation_mask) != _cc_catalog.HUD_ELEMENT_VFTABLE_SIZE
        or not all(vftable.relocation_mask)
        or len(vftable.relocations) != _cc_catalog.HUD_ELEMENT_VFTABLE_SIZE // 4
        or tuple(relocation.offset for relocation in vftable.relocations)
        != tuple(range(0, _cc_catalog.HUD_ELEMENT_VFTABLE_SIZE, 4))
        or any(
            relocation.type != IMAGE_REL_I386_DIR32
            for relocation in vftable.relocations
        )
    ):
        raise ValueError(
            "HudUiElement constructor-table candidate bridge requires one "
            "exact 116-byte VC5 .rdata COMDAT SELECT_ANY vftable with 29 "
            "DIR32 rows"
        )
    invalidate_rows = [
        relocation
        for relocation in vftable.relocations
        if relocation.symbol_name.casefold()
        == _cc_catalog.HUD_ELEMENT_INVALIDATE_SYMBOL.casefold()
    ]
    slot_rows = [
        relocation
        for relocation in vftable.relocations
        if relocation.offset == _cc_catalog.HUD_ELEMENT_INVALIDATE_SLOT_DISPLACEMENT
    ]
    conflicting_slot_names = [
        name
        for name in indexes.by_candidate_name
        if name.casefold() == _cc_catalog.HUD_ELEMENT_INVALIDATE_SYMBOL.casefold()
        and name != _cc_catalog.HUD_ELEMENT_INVALIDATE_SYMBOL
    ]
    if (
        len(slot_rows) != 1
        or invalidate_rows != slot_rows
        or slot_rows[0].symbol_name != _cc_catalog.HUD_ELEMENT_INVALIDATE_SYMBOL
        or conflicting_slot_names
    ):
        raise ValueError(
            "HudUiElement constructor-table candidate bridge requires the "
            "unique exact slot-0x20 Invalidate relocation without a name "
            "collision"
        )
    return {
        _cc_catalog.HUD_ELEMENT_VFTABLE_SYMBOL: _cc_catalog.HUD_ELEMENT_VFTABLE_STORAGE_IDENTITY,
    }


def _hud_ui_container_constructor_absolute_table_candidate_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, str]:
    """Bind the exact HudUiContainer slot-4 FF15 to its retail table."""
    from _recoil.call_contract.records import StorageContainer

    start = normalize_address(caller_start)
    if start != _cc_catalog.HUD_CONTAINER_CONSTRUCTOR_CALLER_START:
        return {}
    required_expected = [{
        "ordinal": 0,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "callback",
        "target_identity": "",
        "storage_identity": _cc_catalog.HUD_CONTAINER_VFTABLE_STORAGE_IDENTITY,
        "slot_displacement": 4,
        "cleanup_bytes": None,
    }]
    storage_start = address_value(_cc_catalog.HUD_CONTAINER_VFTABLE_RETAIL_ADDRESS)
    containers = [
        row for row in indexes.storage_containers
        if row.start < storage_start + _cc_catalog.HUD_CONTAINER_VFTABLE_SIZE
        and storage_start < row.end_exclusive
    ]
    caller = candidate.caller_definition
    if (
        normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_CONTAINER_CONSTRUCTOR_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(start) != caller_identity
        or caller_identity in indexes.provider_ids
        or list(expected) != required_expected
        or indexes.storage_by_address.get(
            _cc_catalog.HUD_CONTAINER_VFTABLE_RETAIL_ADDRESS, ""
        ) != _cc_catalog.HUD_CONTAINER_VFTABLE_STORAGE_IDENTITY
        or _cc_catalog.HUD_CONTAINER_VFTABLE_SYMBOL in indexes.storage_by_name
        or containers != [StorageContainer(
            start=storage_start,
            end_exclusive=storage_start + _cc_catalog.HUD_CONTAINER_VFTABLE_SIZE,
            identity=_cc_catalog.HUD_CONTAINER_VFTABLE_STORAGE_IDENTITY,
        )]
        or caller is None
        or caller.symbol != _cc_catalog.HUD_CONTAINER_CONSTRUCTOR_SYMBOL
        or len(caller.data) != 0x30
        or tuple(
            (row.offset, row.type, row.symbol_name)
            for row in caller.relocations
        ) != (
            (0x07, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_CONTAINER_VFTABLE_SYMBOL),
            (0x0D, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_CONTAINER_VFTABLE_SYMBOL),
        )
        or len(caller.relocation_mask) != len(caller.data)
        or any(
            caller.relocation_mask[index]
            != (0x07 <= index < 0x0B or 0x0D <= index < 0x11)
            for index in range(len(caller.data))
        )
        or caller.data[0x05:0x0B] != b"\xc7\x06\x00\x00\x00\x00"
        or caller.data[0x0B:0x11] != b"\xff\x15\x04\x00\x00\x00"
    ):
        raise ValueError(
            "HudUiContainer constructor-table candidate bridge rejects "
            "retail storage, exact caller, FF15 slot, or relocation drift: "
            f"expected={list(expected)!r}, "
            f"caller_identity={caller_identity!r}, "
            f"tracker_identity={indexes.by_address.get(start)!r}, "
            f"storage_at_table={indexes.storage_by_address.get(_cc_catalog.HUD_CONTAINER_VFTABLE_RETAIL_ADDRESS, '')!r}, "
            f"storage_by_name={indexes.storage_by_name.get(_cc_catalog.HUD_CONTAINER_VFTABLE_SYMBOL)!r}, "
            f"containers={containers!r}, "
            f"caller_symbol={getattr(caller, 'symbol', None)!r}, "
            f"caller_size={len(caller.data) if caller is not None else None!r}, "
            f"relocations={tuple((row.offset, row.type, row.symbol_name) for row in caller.relocations) if caller is not None else None!r}"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    invocations = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        len(invocations) != 1
        or offsets[invocations[0]] != 0x0B
        or bytes(
            int(value, 16)
            for value in candidate.instructions[invocations[0]].bytes
        ) != b"\xff\x15\x04\x00\x00\x00"
        or _cc_targets._memory_slot(
            _cc_cfg._instruction_operand(candidate.instructions[invocations[0]])
        )[1] != 4
    ):
        raise ValueError(
            "HudUiContainer constructor-table candidate bridge requires "
            "one exact body+0xb slot-4 FF15 invocation"
        )

    definitions = [
        (name, definition)
        for name, definition in candidate.vftable_definitions.items()
        if name.casefold() == _cc_catalog.HUD_CONTAINER_VFTABLE_SYMBOL.casefold()
    ]
    if len(definitions) != 1 or definitions[0][0] != _cc_catalog.HUD_CONTAINER_VFTABLE_SYMBOL:
        raise ValueError(
            "HudUiContainer constructor-table candidate bridge rejects a "
            "missing, aliased, or ambiguous vftable definition"
        )
    vftable = definitions[0][1]
    if (
        vftable.symbol != _cc_catalog.HUD_CONTAINER_VFTABLE_SYMBOL
        or vftable.section_name != ".rdata"
        or vftable.section_size != _cc_catalog.HUD_CONTAINER_VFTABLE_SIZE
        or vftable.data != b"\x00" * _cc_catalog.HUD_CONTAINER_VFTABLE_SIZE
        or vftable.relocation_mask != (True,) * _cc_catalog.HUD_CONTAINER_VFTABLE_SIZE
        or not vftable.section_is_comdat
        or vftable.comdat_selection != 2
        or vftable.section_external_symbols != (_cc_catalog.HUD_CONTAINER_VFTABLE_SYMBOL,)
        or tuple(
            (row.offset, row.type, row.symbol_name)
            for row in vftable.relocations
        ) != (
            (0, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_CONTAINER_UPDATE_ALL_SYMBOL),
            (4, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_CONTAINER_SET_ENABLED_SYMBOL),
        )
    ):
        raise ValueError(
            "HudUiContainer constructor-table candidate bridge requires the "
            "exact two-slot SELECT_ANY vftable definition"
        )
    return {
        _cc_catalog.HUD_CONTAINER_VFTABLE_SYMBOL: _cc_catalog.HUD_CONTAINER_VFTABLE_STORAGE_IDENTITY,
    }


def _prove_reviewed_vftable_storage_bridge(
    spec: ReviewedVftableStorageBridgeSpec,
    storage_identity: str,
    *,
    invocation_count: int,
    document: ProgressDocument,
    candidate: CandidateAssembly,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> None:
    if storage_identity != spec.storage_identity:
        raise ValueError(
            f"{spec.label} target is not the "
            "reviewed retail storage identity"
        )
    storage_addresses = sorted(
        address
        for address, identity in indexes.storage_by_address.items()
        if identity == storage_identity
    )
    if storage_addresses != [spec.storage_address]:
        raise ValueError(
            f"{spec.label} storage must resolve to exactly "
            f"{spec.storage_address}"
        )
    data_symbol_id = storage_identity.removeprefix("storage:")
    data_symbol = document.collection("symbols").get(data_symbol_id)
    storage = document.collection("storage_contributions").get(spec.storage_id)
    owner = document.collection("owners").get(spec.owner_id)
    if (
        not isinstance(data_symbol, Mapping)
        or data_symbol.get("binary") != "recoil"
        or data_symbol.get("kind") != "data"
        or data_symbol.get("disposition") != "authored"
        or normalize_address(str(data_symbol.get("address", "")))
        != spec.storage_address
        or data_symbol.get("output_section_id") != "recoil:section:.rdata"
        or data_symbol.get("storage_contribution_ids") != [spec.storage_id]
    ):
        raise ValueError(
            f"{spec.label} lacks the exact "
            "reviewed tracker data entity"
        )
    reference = storage.get("reference") if isinstance(storage, Mapping) else None
    if (
        not isinstance(storage, Mapping)
        or storage.get("binary") != "recoil"
        or storage.get("kind") != "data-symbol"
        or storage.get("output_section_id") != "recoil:section:.rdata"
        or storage.get("overlap") != "none"
        or storage.get("parent_contribution_id") is not None
        or storage.get("owner_ids") != [spec.owner_id]
        or storage.get("symbol_ids") != [data_symbol_id]
        or not isinstance(reference, Mapping)
        or normalize_address(str(reference.get("address", "")))
        != spec.storage_address
        or reference.get("extent_state") not in {"unknown", "known"}
    ):
        raise ValueError(
            f"{spec.label} lacks the exact "
            "reviewed storage contribution"
        )
    gates = owner.get("gates") if isinstance(owner, Mapping) else None
    relationships = (
        owner.get("relationships") if isinstance(owner, Mapping) else None
    )
    primary_rows = [
        row
        for row in relationships or ()
        if isinstance(row, Mapping)
        and row.get("kind") == "primary-data"
        and row.get("symbol_id") == data_symbol_id
        and normalize_address(str(row.get("address", "")))
        == spec.storage_address
    ]
    if (
        not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != spec.owner_kind
        or owner.get("lifecycle_state") != "accepted"
        or owner.get("source_paths") != [spec.source_path]
        or not isinstance(gates, Mapping)
        or any(
            gates.get(key) != "accepted"
            for key in ("boundary", "data", "owner_linkage", "source")
        )
        or len(primary_rows) != 1
    ):
        raise ValueError(
            f"{spec.label} lacks one exact "
            "accepted data-owner relationship"
        )

    definition = candidate.vftable_definitions.get(spec.vftable_symbol)
    if (
        definition is None
        or definition.symbol != spec.vftable_symbol
        or definition.section_name != ".rdata"
        or not definition.section_is_comdat
        or definition.comdat_selection != 2
        or definition.section_external_symbols != (spec.vftable_symbol,)
        or len(definition.data) != definition.section_size
        or definition.section_size != spec.table_size
        or any(definition.data)
        or len(definition.relocations) != spec.table_size // 4
        or any(
            relocation.type != IMAGE_REL_I386_DIR32
            for relocation in definition.relocations
        )
        or tuple(relocation.offset for relocation in definition.relocations)
        != tuple(range(0, spec.table_size, 4))
        or not all(definition.relocation_mask)
    ):
        raise ValueError(
            f"{spec.label} candidate is not one "
            "exact full-section VC5 COMDAT SELECT_ANY vftable"
        )
    slot_rows = [
        relocation
        for relocation in definition.relocations
        if relocation.offset == spec.slot_displacement
    ]
    if (
        len(slot_rows) != 1
        or slot_rows[0].symbol_name != spec.slot_symbol
    ):
        raise ValueError(
            f"{spec.label} slot {spec.slot_displacement} is not the "
            f"exact {spec.slot_symbol} relocation"
        )

    caller = candidate.caller_definition
    if caller is None:
        raise ValueError(
            f"{spec.label} has no current "
            "candidate caller definition"
        )
    references = tuple(
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name == spec.vftable_symbol
    )
    call_references = tuple(
        relocation
        for relocation in references
        if relocation.offset >= 2
        and caller.data[relocation.offset - 2 : relocation.offset]
        == b"\xff\x15"
    )
    store_references = tuple(
        relocation
        for relocation in references
        if relocation.offset >= 2
        and caller.data[relocation.offset - 2 : relocation.offset]
        == spec.vptr_store_prefix
    )
    if (
        invocation_count != spec.invocation_count
        or len(references) != invocation_count + 1
        or len(call_references) != invocation_count
        or len(store_references) != 1
        or any(
            relocation.type != IMAGE_REL_I386_DIR32
            for relocation in references
        )
        or any(
            struct.unpack_from("<I", caller.data, relocation.offset)[0]
            != spec.slot_displacement
            for relocation in call_references
        )
        or struct.unpack_from(
            "<I", caller.data, store_references[0].offset
        )[0]
        != 0
        or any(
            not all(
                caller.relocation_mask[index]
                for index in range(relocation.offset, relocation.offset + 4)
            )
            for relocation in references
        )
    ):
        raise ValueError(
            f"{spec.label} requires one exact "
            "same-caller vptr store and one exact-slot DIR32 indirect call "
            "per invocation"
        )

    containers = [
        row
        for row in indexes.storage_containers
        if row.identity == storage_identity
    ]
    storage_start = address_value(spec.storage_address)
    if (
        len(containers) != 1
        or containers[0].start != storage_start
        or containers[0].end_exclusive != storage_start + spec.table_size
        or containers[0].end_exclusive - containers[0].start
        != definition.section_size
    ):
        raise ValueError(
            f"{spec.label} lacks one exact "
            f"{spec.table_size}-byte live retail storage extent"
        )
    retail_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(spec.storage_address, definition.section_size)
    )
    if len(retail_bytes) != definition.section_size:
        raise ValueError(
            f"{spec.label} retail storage "
            "returned incomplete bytes"
        )
    retail_slot_address = normalize_address(
        hex(
            struct.unpack_from(
                "<I",
                retail_bytes,
                spec.slot_displacement,
            )[0]
        )
    )
    candidate_slot_identity = indexes.by_candidate_name.get(
        spec.slot_symbol,
        "",
    )
    retail_slot_identity = indexes.by_address.get(retail_slot_address, "")
    if (
        retail_slot_address != spec.slot_target_address
        or not candidate_slot_identity
        or candidate_slot_identity != retail_slot_identity
    ):
        raise ValueError(
            f"{spec.label} slot {spec.slot_displacement} target "
            "identity does not exactly match live retail"
        )


def _reviewed_vftable_storage_bridges(
    spec: ReviewedVftableStorageBridgeSpec,
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    compiler_generated_bridges: Mapping[str, str] | None = None,
) -> dict[str, str]:
    candidate_name = spec.vftable_symbol.lower()
    matching_instructions = [
        instruction
        for instruction in candidate.instructions
        if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and candidate_name in _cc_cfg._instruction_operand(instruction).lower()
    ]
    if not matching_instructions:
        return {}
    caller_definition = candidate.caller_definition
    if (
        normalize_address(caller_start) != spec.caller_start
        or normalize_address(caller_end_exclusive)
        != spec.caller_end_exclusive
        or caller_definition is None
        or caller_definition.symbol != spec.caller_symbol
    ):
        raise ValueError(
            f"{spec.label} is restricted to its exact current "
            "reviewed caller candidate"
        )
    provisional_bridges = {
        candidate_name: spec.provisional_identity
    }
    provisional = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges=compiler_generated_bridges,
        candidate_storage_bridges=provisional_bridges,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    rows = [
        row
        for row in provisional
        if row.get("storage_identity") == spec.provisional_identity
    ]
    if (
        len(rows) != spec.invocation_count
        or len(rows) != len(matching_instructions)
    ):
        raise ValueError(
            f"{spec.label} has no exact "
            "candidate invocation population"
        )
    for row in rows:
        ordinal = int(row["ordinal"])
        if ordinal >= len(expected):
            raise ValueError(
                f"{spec.label} has no retail "
                "invocation at the same ordinal"
            )
        expected_row = expected[ordinal]
        if (
            row.get("form") != "call"
            or row.get("dispatch") != "indirect"
            or row.get("identity_kind") != "callback"
            or row.get("slot_displacement") != spec.slot_displacement
            or _contract_shape_without_storage(expected_row)
            != _contract_shape_without_storage(row)
            or expected_row.get("storage_identity") != spec.storage_identity
        ):
            raise ValueError(
                f"{spec.label} is not the same "
                "ordinal, indirect callback form, slot, and cleanup as the "
                "exact retail storage"
            )
    _prove_reviewed_vftable_storage_bridge(
        spec,
        spec.storage_identity,
        invocation_count=len(rows),
        document=document,
        candidate=candidate,
        indexes=indexes,
        bridge=bridge,
    )
    return {
        candidate_name: spec.storage_identity
    }


def _briefing_objective_picture_vftable_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
) -> dict[str, str]:
    return _reviewed_vftable_storage_bridges(
        _cc_catalog.BRIEFING_OBJECTIVE_PICTURE_VFTABLE_BRIDGE,
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        bridge=bridge,
    )


def _briefing_locator_panel_vftable_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
) -> dict[str, str]:
    return _reviewed_vftable_storage_bridges(
        _cc_catalog.BRIEFING_LOCATOR_PANEL_VFTABLE_BRIDGE,
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        bridge=bridge,
    )


def _briefing_runtime_vftable_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
) -> dict[str, str]:
    preliminary_compiler_bridges: dict[str, str] = {}
    ordinary_unresolved_helper = (
        _candidate_has_unresolved_eh_array_destructor(
            candidate,
            indexes=indexes,
            bridge_names=bridge_names,
        )
    )
    empty_candidate_identity_helper = (
        _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL in indexes.by_candidate_name
        and indexes.by_candidate_name[_cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL] == ""
        and _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL not in bridge_names
    )
    if ordinary_unresolved_helper or empty_candidate_identity_helper:
        reviewed_spec = _reviewed_eh_array_destructor_call_context(
            candidate,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
        )
        if reviewed_spec is not None:
            preliminary_compiler_bridges = (
                _reviewed_eh_array_destructor_operand_bridges(
                    candidate,
                    (
                        "compiler-candidate-helper:"
                        f"{_cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL}"
                    ),
                )
            )
    return _reviewed_vftable_storage_bridges(
        _cc_catalog.BRIEFING_RUNTIME_VFTABLE_BRIDGE,
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        bridge=bridge,
        compiler_generated_bridges=preliminary_compiler_bridges,
    )


def _hud_timer_panel_float_retail_storage_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> IdentityIndexes:
    """Publish one tracker-proven retail vftable extent for one absolute call.

    This resolver is deliberately narrower than the ordinary BN data-container
    index.  It accepts only the reviewed HudUiTimerPanelFloat constructor,
    exact immutable-retail store/call bytes, and a complete current tracker
    data/storage/primary-owner package.  Candidate output supplies none of the
    retail identity, extent, slot, or target facts.
    """
    from _recoil.call_contract.records import StorageContainer
    caller_start = normalize_address(caller_start)
    caller_end_exclusive = normalize_address(caller_end_exclusive)
    if caller_start != _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_START:
        return indexes
    if (
        caller_identity != _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_IDENTITY
        or caller_end_exclusive
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(_cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_START)
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_IDENTITY
        or _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD timer-panel-float storage resolver requires the exact "
            "reviewed authored caller 0x40ef60..0x40f040"
        )

    def exact_retail_instruction(
        address: str,
        *,
        mnemonic: str,
        body: bytes,
    ) -> Instruction:
        rows = [
            instruction
            for instruction in retail_instructions
            if _cc_cfg._source_instruction_address(instruction) == address
        ]
        if (
            len(rows) != 1
            or _cc_cfg._instruction_mnemonic(rows[0]) != mnemonic
            or tuple(value.lower() for value in rows[0].bytes)
            != tuple(f"{value:02x}" for value in body)
        ):
            raise ValueError(
                "HUD timer-panel-float storage resolver requires exact "
                f"retail {address} {body.hex(' ')} bytes"
            )
        return rows[0]

    store = exact_retail_instruction(
        _cc_catalog.HUD_TIMER_PANEL_FLOAT_RETAIL_STORE_ADDRESS,
        mnemonic="mov",
        body=_cc_catalog.HUD_TIMER_PANEL_FLOAT_RETAIL_STORE_BODY,
    )
    call = exact_retail_instruction(
        _cc_catalog.HUD_TIMER_PANEL_FLOAT_RETAIL_CALL_ADDRESS,
        mnemonic="call",
        body=_cc_catalog.HUD_TIMER_PANEL_FLOAT_RETAIL_CALL_BODY,
    )
    store_addresses = {
        normalize_address(raw)
        for raw in _cc_catalog.ADDRESS_RE.findall(_cc_cfg._instruction_operand(store))
    }
    call_addresses = {
        normalize_address(raw)
        for raw in _cc_catalog.ADDRESS_RE.findall(_cc_cfg._instruction_operand(call))
    }
    exact_slot_calls = [
        instruction
        for instruction in retail_instructions
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            and _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_ADDRESS
            in {
                normalize_address(raw)
                for raw in _cc_catalog.ADDRESS_RE.findall(
                    _cc_cfg._instruction_operand(instruction)
                )
            }
        )
    ]
    if (
        store_addresses != {_cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ADDRESS}
        or call_addresses != {_cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_ADDRESS}
        or len(exact_slot_calls) != 1
        or exact_slot_calls[0] is not call
        or _cc_cfg._cleanup_after(
            retail_instructions,
            retail_instructions.index(call),
        )
        is not None
    ):
        raise ValueError(
            "HUD timer-panel-float storage resolver requires one exact "
            "retail base store and slot-0x60 absolute indirect call"
        )

    symbols = document.collection("symbols")
    storages = document.collection("storage_contributions")
    owners = document.collection("owners")
    data = symbols.get(_cc_catalog.HUD_TIMER_PANEL_FLOAT_DATA_ID)
    storage = storages.get(_cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ID)
    owner = owners.get(_cc_catalog.HUD_TIMER_PANEL_FLOAT_OWNER_ID)
    if (
        not isinstance(data, Mapping)
        or data.get("binary") != "recoil"
        or data.get("kind") != "data"
        or data.get("disposition") != "authored"
        or data.get("ownership_state") != "primary-owned"
        or normalize_address(str(data.get("address", "")))
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ADDRESS
        or normalize_address(str(data.get("end_exclusive", "")))
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_END_EXCLUSIVE
        or data.get("extent_state") != "known"
        or data.get("size") != _cc_catalog.HUD_TIMER_PANEL_FLOAT_TABLE_SIZE
        or data.get("output_section_id") != "recoil:section:.rdata"
        or data.get("storage_contribution_ids")
        != [_cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ID]
        or data.get("logical_aliases")
        or data.get("provider_identity")
        or data.get("import_name")
        or data.get("import_dll")
        or data.get("import_ordinal") is not None
    ):
        raise ValueError(
            "HUD timer-panel-float storage resolver requires the exact "
            "authored known-extent tracker data entity"
        )
    reference = storage.get("reference") if isinstance(storage, Mapping) else None
    if (
        not isinstance(storage, Mapping)
        or storage.get("binary") != "recoil"
        or storage.get("kind") != "data-symbol"
        or storage.get("output_section_id") != "recoil:section:.rdata"
        or storage.get("overlap") != "none"
        or storage.get("parent_contribution_id") is not None
        or storage.get("owner_ids") != [_cc_catalog.HUD_TIMER_PANEL_FLOAT_OWNER_ID]
        or storage.get("symbol_ids") != [_cc_catalog.HUD_TIMER_PANEL_FLOAT_DATA_ID]
        or storage.get("logical_aliases")
        or storage.get("provider_identity")
        or storage.get("import_name")
        or storage.get("import_dll")
        or storage.get("import_ordinal") is not None
        or not isinstance(reference, Mapping)
        or normalize_address(str(reference.get("address", "")))
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ADDRESS
        or normalize_address(str(reference.get("end_exclusive", "")))
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_END_EXCLUSIVE
        or reference.get("extent_state") != "known"
        or reference.get("size") != _cc_catalog.HUD_TIMER_PANEL_FLOAT_TABLE_SIZE
    ):
        raise ValueError(
            "HUD timer-panel-float storage resolver requires exact tracker "
            "storage linkage and known table extent"
        )

    def exact_primary_rows(
        candidate_owner: Mapping[str, Any],
        *,
        kind: str,
        symbol_id: str,
        address: str,
    ) -> list[Mapping[str, Any]]:
        return [
            row
            for row in candidate_owner.get("relationships", ())
            if (
                isinstance(row, Mapping)
                and row.get("kind") == kind
                and row.get("symbol_id") == symbol_id
                and normalize_address(str(row.get("address", "")))
                == address
            )
        ]

    matching_primary_owners = [
        str(owner_id)
        for owner_id, candidate_owner in owners.items()
        if (
            isinstance(candidate_owner, Mapping)
            and exact_primary_rows(
                candidate_owner,
                kind="primary-data",
                symbol_id=_cc_catalog.HUD_TIMER_PANEL_FLOAT_DATA_ID,
                address=_cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ADDRESS,
            )
        )
    ]
    if (
        not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "class"
        or owner.get("name") != "HudUiTimerPanelFloat"
        or len(
            exact_primary_rows(
                owner,
                kind="primary-data",
                symbol_id=_cc_catalog.HUD_TIMER_PANEL_FLOAT_DATA_ID,
                address=_cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ADDRESS,
            )
        )
        != 1
        or len(
            exact_primary_rows(
                owner,
                kind="primary-function",
                symbol_id="recoil:function:0x40ef60",
                address=_cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_START,
            )
        )
        != 1
        or matching_primary_owners != [_cc_catalog.HUD_TIMER_PANEL_FLOAT_OWNER_ID]
    ):
        raise ValueError(
            "HUD timer-panel-float storage resolver requires one unique "
            "HudUiTimerPanelFloat primary owner"
        )

    if (
        indexes.storage_by_address.get(
            _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ADDRESS
        )
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_IDENTITY
        or _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_ADDRESS in indexes.storage_by_address
        or [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_IDENTITY
        ]
        != [_cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ADDRESS]
        or _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_IDENTITY.startswith("iat:")
    ):
        raise ValueError(
            "HUD timer-panel-float storage resolver rejects missing, "
            "duplicate, alias, provider, or IAT storage identities"
        )

    storage_start = address_value(_cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ADDRESS)
    storage_end = address_value(
        _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_END_EXCLUSIVE
    )
    overlapping_containers = [
        row
        for row in indexes.storage_containers
        if row.start < storage_end and storage_start < row.end_exclusive
    ]
    exact_container = StorageContainer(
        start=storage_start,
        end_exclusive=storage_end,
        identity=_cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_IDENTITY,
    )
    if any(row != exact_container for row in overlapping_containers):
        raise ValueError(
            "HUD timer-panel-float storage resolver rejects a conflicting "
            "or wrong-extent retail storage container"
        )

    retail_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(
            _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ADDRESS,
            _cc_catalog.HUD_TIMER_PANEL_FLOAT_TABLE_SIZE,
        )
    )
    if (
        len(retail_bytes) != _cc_catalog.HUD_TIMER_PANEL_FLOAT_TABLE_SIZE
        or normalize_address(
            hex(
                struct.unpack_from(
                    "<I",
                    retail_bytes,
                    _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_DISPLACEMENT,
                )[0]
            )
        )
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_TARGET_ADDRESS
        or indexes.by_address.get(
            _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_TARGET_ADDRESS
        )
        in {"", None}
        or indexes.by_address[
            _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_TARGET_ADDRESS
        ]
        in indexes.provider_ids
    ):
        raise ValueError(
            "HUD timer-panel-float storage resolver requires the exact "
            "148-byte retail table and authored slot-0x60 target 0x404d20"
        )

    if exact_container in indexes.storage_containers:
        return indexes
    return replace(
        indexes,
        storage_containers=tuple(
            sorted(
                (*indexes.storage_containers, exact_container),
                key=lambda row: (
                    row.start,
                    row.end_exclusive,
                    row.identity,
                ),
            )
        ),
    )


def _hud_timer_panel_float_candidate_vptr_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedVptrStorageBridge]:
    """Bridge one proven real-constructor vptr call to retail authority.

    The current constructor may still contain a direct ``SetVisible`` call.
    In that state this resolver deliberately withholds the candidate-only
    vptr bridge so ordinary extraction reports the direct/indirect semantic
    divergence.  If a slot-0x60 call is present, the bridge is emitted only
    after proving its real-constructor vtable installation and reaching
    ``load(this)`` topology.
    """
    from _recoil.call_contract.records import ReviewedVptrStorageBridge
    caller_start = normalize_address(caller_start)
    caller_end_exclusive = normalize_address(caller_end_exclusive)
    if caller_start != _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_START:
        return {}
    caller_addresses = [
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    ]
    if (
        caller_identity != _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_IDENTITY
        or caller_end_exclusive
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_START]
        or _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires the exact "
            "reviewed authored caller 0x40ef60..0x40f040"
        )
    caller_name_rows = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold() == _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_SYMBOL.casefold()
    ]
    if caller_name_rows not in (
        [],
        [
            (
                _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_SYMBOL,
                _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_IDENTITY,
            )
        ],
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge rejects a conflicting, "
            "duplicate, or aliased real constructor symbol"
        )

    definition = candidate.caller_definition
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_TIMER_PANEL_FLOAT_CALLER_SYMBOL
        or len(definition.data)
        < _cc_catalog.HUD_TIMER_PANEL_FLOAT_VPTR_RELOCATION_OFFSET + 4
        or len(definition.relocation_mask) != len(definition.data)
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires exact "
            "real HudUiTimerPanelFloat constructor contribution"
        )

    slot_invocations: list[tuple[int, Instruction, str]] = []
    for index, instruction in enumerate(candidate.instructions):
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        expression = _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(instruction)
        )
        match = re.fullmatch(
            r"(eax|ebx|ecx|edx|esi|edi|ebp)"
            r"(?:\+0x60|\+96)",
            expression,
            flags=re.IGNORECASE,
        )
        if (
            match is not None
            and _cc_targets._memory_slot(_cc_cfg._instruction_operand(instruction))[1]
            == _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_DISPLACEMENT
        ):
            slot_invocations.append(
                (index, instruction, match.group(1).lower())
            )

    # A direct SetVisible call is an ordinary candidate contract.  With no
    # matching vptr invocation there is no candidate-only fact to bridge.
    if not slot_invocations:
        return {}
    if len(slot_invocations) != 1:
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires one unique "
            "slot-0x60 vptr invocation"
        )
    call_index, call, call_register = slot_invocations[0]
    raw_call_offset = _cc_cfg._source_instruction_address(call)
    if (
        _cc_cfg._instruction_mnemonic(call) != "call"
        or not call.bytes
        or call.bytes[0].lower() != "ff"
        or call_register != "eax"
        or not raw_call_offset
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires one offset-"
            "bearing indirect slot-0x60 call with callee cleanup"
        )
    call_offset = address_value(raw_call_offset)
    call_body = bytes(int(value, 16) for value in call.bytes)
    if (
        call_body != b"\xff\x50\x60"
        or call_offset + len(call_body) > len(definition.data)
        or definition.data[
            call_offset : call_offset + len(call_body)
        ]
        != call_body
        or any(
            relocation.offset < call_offset + len(call_body)
            and relocation.offset + 4 > call_offset
            for relocation in definition.relocations
        )
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires the exact "
            "relocation-free candidate slot-call bytes"
        )

    definitions = [
        (name, row)
        for name, row in candidate.vftable_definitions.items()
        if name.casefold() == _cc_catalog.HUD_TIMER_PANEL_FLOAT_VFTABLE_SYMBOL.casefold()
    ]
    if len(definitions) != 1 or definitions[0][0] != (
        _cc_catalog.HUD_TIMER_PANEL_FLOAT_VFTABLE_SYMBOL
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge rejects missing, "
            "duplicate, or aliased vftable definitions"
        )
    vftable = definitions[0][1]
    if (
        vftable.symbol != _cc_catalog.HUD_TIMER_PANEL_FLOAT_VFTABLE_SYMBOL
        or vftable.section_name != ".rdata"
        or not vftable.section_is_comdat
        or vftable.comdat_selection != 2
        or vftable.section_external_symbols
        != (_cc_catalog.HUD_TIMER_PANEL_FLOAT_VFTABLE_SYMBOL,)
        or vftable.section_size != _cc_catalog.HUD_TIMER_PANEL_FLOAT_TABLE_SIZE
        or len(vftable.data) != _cc_catalog.HUD_TIMER_PANEL_FLOAT_TABLE_SIZE
        or any(vftable.data)
        or len(vftable.relocations)
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_TABLE_SIZE // 4
        or tuple(row.offset for row in vftable.relocations)
        != tuple(range(0, _cc_catalog.HUD_TIMER_PANEL_FLOAT_TABLE_SIZE, 4))
        or any(
            row.type != IMAGE_REL_I386_DIR32
            for row in vftable.relocations
        )
        or len(vftable.relocation_mask)
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_TABLE_SIZE
        or not all(vftable.relocation_mask)
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires one exact "
            "148-byte VC5 SELECT_ANY COMDAT with 37 DIR32 relocations"
        )
    slot_rows = [
        row
        for row in vftable.relocations
        if row.offset == _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_DISPLACEMENT
    ]
    target_identity = indexes.by_address.get(
        _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_TARGET_ADDRESS,
        "",
    )
    target_addresses = [
        address
        for address, identity in indexes.by_address.items()
        if identity == target_identity
    ]
    target_name_rows = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold() == _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_SYMBOL.casefold()
    ]
    if (
        len(slot_rows) != 1
        or slot_rows[0].symbol_name
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_SYMBOL
        or not target_identity
        or target_identity in indexes.provider_ids
        or target_addresses
        != [_cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_TARGET_ADDRESS]
        or target_name_rows
        != [(_cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_SYMBOL, target_identity)]
        or _cc_catalog.HUD_TIMER_PANEL_FLOAT_VFTABLE_SYMBOL
        in indexes.storage_by_name
        or _cc_catalog.HUD_TIMER_PANEL_FLOAT_VFTABLE_SYMBOL.lower()
        in indexes.storage_by_name
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires exact "
            "slot-0x60 relocation convergence on authored target 0x404d20"
        )

    vftable_casefold = _cc_catalog.HUD_TIMER_PANEL_FLOAT_VFTABLE_SYMBOL.casefold()
    references = [
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name.casefold() == vftable_casefold
    ]
    field_references = [
        relocation
        for relocation in definition.relocations
        if relocation.offset
        == _cc_catalog.HUD_TIMER_PANEL_FLOAT_VPTR_RELOCATION_OFFSET
    ]
    if (
        len(references) != 1
        or field_references != references
        or references[0].symbol_name
        != _cc_catalog.HUD_TIMER_PANEL_FLOAT_VFTABLE_SYMBOL
        or references[0].type != IMAGE_REL_I386_DIR32
        or definition.data[
            _cc_catalog.HUD_TIMER_PANEL_FLOAT_VPTR_STORE_OFFSET:
            _cc_catalog.HUD_TIMER_PANEL_FLOAT_VPTR_RELOCATION_OFFSET
        ]
        != b"\xc7\x06"
        or struct.unpack_from(
            "<I",
            definition.data,
            _cc_catalog.HUD_TIMER_PANEL_FLOAT_VPTR_RELOCATION_OFFSET,
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.HUD_TIMER_PANEL_FLOAT_VPTR_RELOCATION_OFFSET,
                _cc_catalog.HUD_TIMER_PANEL_FLOAT_VPTR_RELOCATION_OFFSET + 4,
            )
        )
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires one unique "
            "zero-addend fully masked C706/DIR32 real-constructor vptr "
            "installation at +0x4b"
        )

    instruction_offsets = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions,
        source="cod",
        caller_start=0,
    )
    store_indexes = [
        index
        for index, offset in enumerate(instruction_offsets)
        if offset == _cc_catalog.HUD_TIMER_PANEL_FLOAT_VPTR_STORE_OFFSET
    ]
    if len(store_indexes) != 1:
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires one unique "
            "real-constructor vptr installation instruction"
        )
    store_index = store_indexes[0]
    store = candidate.instructions[store_index]
    if (
        tuple(value.lower() for value in store.bytes)
        != ("c7", "06", "00", "00", "00", "00")
        or _cc_cfg._instruction_mnemonic(store) != "mov"
        or not store_index < call_index
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires the exact "
            "+0x4b C706 vtable installation before the slot call"
        )

    if (
        any(
            _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            or _cc_cfg._instruction_mnemonic(instruction)
            in {"loop", "loope", "loopne", "loopnz", "loopz"}
            for instruction in candidate.instructions[: call_index + 1]
        )
        or any(
            target <= call_index
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or any(
            index <= call_index
            for index in candidate.local_control_flow_indices
        )
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge rejects branches or "
            "alternate entries before the reviewed call"
        )

    vptr_write_mnemonics = {
        "adc", "add", "and", "btc", "btr", "bts", "cmpxchg", "dec",
        "inc", "mov", "neg", "not", "or", "sbb", "sub", "xchg", "xor",
    }
    provenance = _cc_cfg._empty_register_state()
    provenance.update(
        {
            "ecx": "this",
            "esp": "stack",
            "ebp": "frame",
        }
    )
    proven_load_indexes: list[int] = []
    for index, instruction in enumerate(
        candidate.instructions[: call_index + 1]
    ):
        operands = [
            item.strip()
            for item in _cc_cfg._instruction_operand(instruction).split(",")
        ]
        if index == store_index and provenance.get("esi") != "this":
            raise ValueError(
                "HUD timer-panel-float candidate bridge rejects a vtable "
                "installation through a non-this base"
            )
        if (
            store_index < index < call_index
            and _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and len(operands) == 2
            and operands[0].lower() == call_register
            and "[" in operands[1]
        ):
            expression, displacement = _cc_targets._memory_slot(operands[1])
            base_match = re.fullmatch(
                r"(eax|ebx|ecx|edx|esi|edi|ebp)",
                expression,
                flags=re.IGNORECASE,
            )
            if (
                displacement in {None, 0}
                and base_match is not None
                and provenance.get(base_match.group(1).lower())
                in {"this", "address(this)"}
            ):
                proven_load_indexes.append(index)
        if (
            store_index < index < call_index
            and _cc_cfg._instruction_mnemonic(instruction)
            in vptr_write_mnemonics
            and operands
            and "[" in operands[0]
        ):
            expression, displacement = _cc_targets._memory_slot(operands[0])
            base_match = re.fullmatch(
                r"(eax|ebx|ecx|edx|esi|edi|ebp)",
                expression,
                flags=re.IGNORECASE,
            )
            base_provenance = (
                provenance.get(base_match.group(1).lower(), "")
                if base_match is not None
                else ""
            )
            if displacement in {None, 0} and base_provenance in {
                "this",
                "address(this)",
            }:
                raise ValueError(
                    "HUD timer-panel-float candidate bridge rejects an "
                    "overwritten vptr through a same-object alias"
                )
        if index == call_index:
            if (
                provenance.get(call_register) != "load(this)"
                or len(proven_load_indexes) != 1
                or tuple(
                    value.lower()
                    for value in candidate.instructions[
                        proven_load_indexes[0]
                    ].bytes
                )
                != ("8b", "06")
            ):
                raise ValueError(
                    "HUD timer-panel-float candidate bridge requires one "
                    "unique reaching vptr load from this"
                )
            continue
        if _cc_cfg._instruction_mnemonic(instruction) == "call":
            for volatile in ("eax", "ecx", "edx"):
                provenance[volatile] = ""
            continue
        _cc_receiver_storage._update_register_state(
            instruction,
            provenance,
            assembly_source="cod",
            indexes=indexes,
            reviewed_register_storage_bridges={},
        )

    expected_rows = [
        row
        for row in expected
        if row.get("storage_identity")
        == _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_IDENTITY
    ]
    if (
        len(expected_rows) != 1
        or expected_rows[0]
        != {
            "ordinal": _cc_catalog.HUD_TIMER_PANEL_FLOAT_RETAIL_ORDINAL,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "callback",
            "target_identity": "",
            "storage_identity": _cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_IDENTITY,
            "slot_displacement": _cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_DISPLACEMENT,
            "cleanup_bytes": None,
        }
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge requires one exact "
            "retail absolute slot-0x60 callback contract"
        )

    # Re-read the current document entity here so this candidate-side bridge
    # cannot outlive or silently replace the retail-side tracker authority.
    data = document.collection("symbols").get(
        _cc_catalog.HUD_TIMER_PANEL_FLOAT_DATA_ID
    )
    if (
        not isinstance(data, Mapping)
        or data.get("storage_contribution_ids")
        != [_cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_ID]
        or data.get("disposition") != "authored"
        or data.get("extent_state") != "known"
    ):
        raise ValueError(
            "HUD timer-panel-float candidate bridge cannot use stale retail "
            "tracker authority"
        )
    return {
        normalize_address(hex(call_offset)): ReviewedVptrStorageBridge(
            register=call_register,
            provenance="load(this)",
            storage_identity=_cc_catalog.HUD_TIMER_PANEL_FLOAT_STORAGE_IDENTITY,
            slot_displacement=_cc_catalog.HUD_TIMER_PANEL_FLOAT_SLOT_DISPLACEMENT,
        )
    }
