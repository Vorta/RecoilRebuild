"""Recoil call-contract callable identity evidence and checks."""

from __future__ import annotations

from _recoil.call_contract import targets as _cc_targets

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import dispatch as _cc_dispatch
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import recoil_lifecycle as _cc_recoil_lifecycle
from _recoil.call_contract import recoil_mfc as _cc_recoil_mfc
from _recoil.call_contract import reporting as _cc_reporting

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateCallerDefinition,
        CandidateCoffSymbolDefinition,
        IdentityIndexes,
        ReviewedInboundEntryRegisterTargetBridge,
    )

import json
import re
import struct
from collections import Counter
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_REL32,
    IMAGE_SYM_CLASS_EXTERNAL,
    CoffRelocation,
    Instruction,
)
from _recoil.commands.vc5_build import DEFAULT_MANIFEST as DEFAULT_FINAL_BUILD_MANIFEST
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.coff_alias import CoffAliasSource
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address
from _recoil.lib.tooling import REPO_ROOT


def _candidate_complete_instruction_offsets(
    candidate: CandidateAssembly,
) -> tuple[int | None, ...]:
    """Recover only listing offsets proved by adjacent instruction lengths.

    VC5's COD output puts some long instruction mnemonics on a continuation
    line without repeating the leading offset.  The ordinary parser therefore
    leaves those instructions without an address.  A missing offset is safe to
    recover only when the nearest explicit offsets on both sides independently
    imply the same value; data gaps and malformed/reordered listings remain
    unresolved.
    """

    offsets = list(
        _cc_cfg._instruction_runtime_addresses(
            candidate.instructions,
            source="cod",
            caller_start=0,
        )
    )
    explicit = [
        index for index, offset in enumerate(offsets) if offset is not None
    ]
    for index, offset in enumerate(offsets):
        if offset is not None:
            continue
        previous = next(
            (
                explicit_index
                for explicit_index in reversed(explicit)
                if explicit_index < index
            ),
            None,
        )
        following = next(
            (
                explicit_index
                for explicit_index in explicit
                if explicit_index > index
            ),
            None,
        )
        if previous is None or following is None:
            continue
        forward = int(offsets[previous]) + sum(
            len(candidate.instructions[item].bytes)
            for item in range(previous, index)
        )
        backward = int(offsets[following]) - sum(
            len(candidate.instructions[item].bytes)
            for item in range(index, following)
        )
        if forward == backward:
            offsets[index] = forward
    return tuple(offsets)


def _candidate_dynamic_probe_setup_matches(
    candidate: CandidateAssembly,
    offsets: Sequence[int | None],
    setup_rows: Sequence[tuple[int, bytes]],
) -> bool:
    """Authenticate probe setup in its current COFF definition.

    Frame-slot allocation and section placement are compiler observations.
    A saved ESP may move to another aligned EBP local; every EAX-producing
    opcode, call-relative coordinate, and nonrelocated setup byte stays exact.
    The caller separately proves the helper call population and relocations.
    """
    definition = candidate.caller_definition
    if (definition is None or not definition.data or not setup_rows
            or definition.section_index <= 0 or definition.section_start != 0
            or definition.section_end != len(definition.data)
            or len(definition.relocation_mask) != len(definition.data)
            or not _cc_receiver_candidate._candidate_listing_matches_coff(
                candidate.instructions, addresses=offsets, caller_start=0,
                definition=definition)):
        return False
    by_offset = dict(zip(offsets, candidate.instructions))
    for offset, reviewed in setup_rows:
        instruction = by_offset.get(offset)
        if instruction is None:
            return False
        observed = bytes(int(value, 16) for value in instruction.bytes)
        local_esp_save = (
            len(reviewed) == len(observed) == 3
            and reviewed[:2] == observed[:2] == b'\x89\x65'
            and all(0x80 <= body[2] <= 0xfc and body[2] % 4 == 0
                for body in (reviewed, observed)))
        if (observed != reviewed and not local_esp_save
                or any(definition.relocation_mask[offset:offset + len(observed)])
                or any(row.offset < offset + len(observed)
                    and offset < row.offset + 4 for row in definition.relocations)):
            return False
    return True


def _locate_shifted_candidate_instruction_unit(
    candidate: CandidateAssembly,
    expected_rows: Mapping[int, tuple[tuple[str, ...], str]],
    *,
    label: str,
) -> tuple[
    int,
    tuple[int | None, ...],
    dict[int, Instruction],
    dict[int, int],
]:
    """Locate one reviewed candidate unit without fixing its absolute offset.

    ``expected_rows`` uses the original reviewed candidate offsets only as
    relative coordinates inside the unit.  The locator accepts a shifted unit
    only when every ordered instruction has the exact bytes and operand text,
    the COD offsets are unique, and the same bytes occupy the corresponding
    current COFF body locations.  Missing, duplicated, reordered, internally
    resized, or malformed units fail closed.
    """

    definition = candidate.caller_definition
    if (
        definition is None
        or not definition.data
        or len(definition.data) != len(definition.relocation_mask)
        or not expected_rows
    ):
        raise ValueError(
            f"{label} requires a nonempty candidate body and reviewed rows"
        )
    baseline_offsets = tuple(expected_rows)
    if baseline_offsets != tuple(sorted(baseline_offsets)):
        raise ValueError(f"{label} requires ordered reviewed row offsets")
    offsets = _candidate_complete_instruction_offsets(candidate)
    counts: dict[int, int] = {}
    for offset in offsets:
        if offset is not None:
            counts[offset] = counts.get(offset, 0) + 1
    by_offset = {
        offset: instruction
        for instruction, offset in zip(candidate.instructions, offsets)
        if offset is not None and counts.get(offset) == 1
    }
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None and counts.get(offset) == 1
    }
    baseline_anchor = baseline_offsets[0]
    anchor_body, anchor_pattern = expected_rows[baseline_anchor]
    shifts: list[int] = []
    for index, (instruction, offset) in enumerate(
        zip(candidate.instructions, offsets)
    ):
        if (
            offset is None
            or counts.get(offset) != 1
            or tuple(value.lower() for value in instruction.bytes)
            != anchor_body
            or re.fullmatch(
                anchor_pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            continue
        shift = offset - baseline_anchor
        shifted_offsets = tuple(
            baseline_offset + shift
            for baseline_offset in baseline_offsets
        )
        if any(
            shifted_offset not in by_offset
            for shifted_offset in shifted_offsets
        ):
            continue
        shifted_indices = [
            index_by_offset[shifted_offset]
            for shifted_offset in shifted_offsets
        ]
        if (
            shifted_indices != sorted(shifted_indices)
            or len(set(shifted_indices)) != len(shifted_indices)
        ):
            continue
        matched = True
        for baseline_offset, shifted_offset in zip(
            baseline_offsets,
            shifted_offsets,
        ):
            body, pattern = expected_rows[baseline_offset]
            shifted_instruction = by_offset[shifted_offset]
            encoded = bytes(
                int(value, 16) for value in shifted_instruction.bytes
            )
            if (
                tuple(
                    value.lower()
                    for value in shifted_instruction.bytes
                )
                != body
                or re.fullmatch(
                    pattern,
                    shifted_instruction.raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is None
                or definition.data[
                    shifted_offset : shifted_offset + len(encoded)
                ]
                != encoded
            ):
                matched = False
                break
        if matched:
            shifts.append(shift)
    if len(shifts) != 1:
        raise ValueError(
            f"{label} requires one unique ordered structural/COFF unit; "
            f"found {len(shifts)}"
        )
    return shifts[0], offsets, by_offset, index_by_offset


def _candidate_body_with_normalized_symbolic_stack_displacements(
    candidate: CandidateAssembly,
    *,
    unit_start: int,
    unit_end: int,
    by_address: Mapping[int, Instruction],
    specs: Mapping[int, tuple[tuple[int, ...], str, int]],
    label: str,
) -> bytes:
    """Normalize only COFF-backed symbolic local-stack displacement bytes."""

    definition = candidate.caller_definition
    if (
        definition is None
        or unit_start < 0
        or unit_end > len(definition.data)
        or unit_start >= unit_end
    ):
        raise ValueError(f"{label} requires a bounded candidate body")
    normalized = bytearray(definition.data[unit_start:unit_end])
    for address, (prefix, pattern, baseline_displacement) in specs.items():
        instruction = by_address.get(address)
        encoded = (
            bytes(int(value, 16) for value in instruction.bytes)
            if instruction is not None
            else b""
        )
        prefix_bytes = bytes(prefix)
        if (
            address < unit_start
            or address + len(encoded) > unit_end
            or len(encoded) != len(prefix_bytes) + 1
            or encoded[: len(prefix_bytes)] != prefix_bytes
            or re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
            or definition.data[address : address + len(encoded)] != encoded
        ):
            raise ValueError(
                f"{label} rejects a malformed symbolic local-stack operand"
            )
        normalized[
            address - unit_start + len(prefix_bytes)
        ] = baseline_displacement
    return bytes(normalized)


def _candidate_contiguous_coff_unit_instructions(
    candidate: CandidateAssembly,
    *,
    offsets: Sequence[int | None],
    unit_start: int,
    unit_end: int,
    label: str,
) -> tuple[Instruction, ...]:
    """Require a complete ordered COD instruction cover of one COFF unit."""

    definition = candidate.caller_definition
    if definition is None or len(offsets) != len(candidate.instructions):
        raise ValueError(f"{label} requires candidate COD/COFF evidence")
    rows = sorted(
        (
            (offset, instruction)
            for instruction, offset in zip(candidate.instructions, offsets)
            if offset is not None and unit_start <= offset < unit_end
        ),
        key=lambda row: row[0],
    )
    cursor = unit_start
    result: list[Instruction] = []
    for offset, instruction in rows:
        encoded = bytes(int(value, 16) for value in instruction.bytes)
        if (
            offset != cursor
            or not encoded
            or definition.data[offset : offset + len(encoded)] != encoded
        ):
            raise ValueError(
                f"{label} requires one contiguous ordered COD/COFF unit"
            )
        cursor += len(encoded)
        result.append(instruction)
    if cursor != unit_end:
        raise ValueError(
            f"{label} requires one contiguous ordered COD/COFF unit"
        )
    return tuple(result)


def _require_ordered_candidate_semantic_patterns(
    instructions: Sequence[Instruction],
    patterns: Sequence[str],
    *,
    label: str,
) -> None:
    """Require semantic COD operands in order without fixing local offsets."""

    next_index = 0
    for pattern_ordinal, pattern in enumerate(patterns):
        match_index = next(
            (
                index
                for index in range(next_index, len(instructions))
                if re.fullmatch(
                    pattern,
                    instructions[index].raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is not None
            ),
            None,
        )
        if match_index is None:
            raise ValueError(
                f"{label} rejects symbolic argument/result-use drift at "
                f"pattern {pattern_ordinal}: {pattern!r}"
            )
        next_index = match_index + 1


def _same_candidate_coff_symbol_identity(
    left: CandidateCoffSymbolDefinition,
    right: CandidateCoffSymbolDefinition,
) -> bool:
    """Compare the complete COFF identity bound by one exact relocation.

    Live parser snapshots may materialize equal symbol rows as distinct Python
    objects.  Host object identity is therefore not COFF identity; the symbol
    table index and its complete primary-row fields are the fail-closed join.
    """

    return (
        left.index,
        left.name,
        left.value,
        left.section_number,
        left.symbol_type,
        left.storage_class,
        left.aux_count,
    ) == (
        right.index,
        right.name,
        right.value,
        right.section_number,
        right.symbol_type,
        right.storage_class,
        right.aux_count,
    )


def _candidate_expected_direct_identity_name(
    decorated_identity: str,
) -> str:
    """Decode only identities whose exact semantic spelling is bounded here."""

    decoded = _cc_reporting._decode_candidate_dependent_callable_identity(
        decorated_identity
    )
    if decoded is not None:
        return decoded.callable_key.qualified_name
    reviewed_constructor = (
        _cc_catalog._REVIEWED_VC5_COMPLETE_CONSTRUCTOR_IDENTITY_NAMES.get(
            decorated_identity
        )
    )
    if reviewed_constructor is not None:
        return reviewed_constructor
    constructor = _cc_catalog._VC5_COMPLETE_CONSTRUCTOR_IDENTITY_RE.fullmatch(
        decorated_identity
    )
    if constructor is not None:
        components = constructor.group("scope").split("@")
        owner = "::".join(reversed(components))
        return f"{owner}::Constructor"
    destructor = _cc_catalog._VC5_COMPLETE_DESTRUCTOR_IDENTITY_RE.fullmatch(
        decorated_identity
    )
    if destructor is None:
        return ""
    components = destructor.group("scope").split("@")
    class_name = components[0]
    owner = "::".join(reversed(components))
    return f"{owner}::~{class_name}"


def _expected_direct_candidate_identity_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    existing_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Join exact unresolved VC5 names to same-ordinal retail authored truth.

    This bridge is intentionally limited to zero-argument global functions and
    ordinary complete destructors.  The immutable retail row supplies the
    physical target identity; the tracker independently requires the matching
    authored navigation name; COD/COFF supplies one exact undefined REL32
    consumer population.  Candidate output never supplies expected identity.
    """

    # This is a finite caller package, not a general decorated-name resolver.
    # Earlier reviewed RecoilApp callers already have independent bridge and
    # provenance profiles; consulting expected ordinals for those callers can
    # preempt that composition when a new contract schema refreshes the retail
    # cache.  Keep the four proven residual callers completely orthogonal.
    reviewed_profiles = {
        (
            "symbol:recoil:function:0x430250",
            "0x430250",
            "0x4305f0",
        ): frozenset({"?GetTexturePackLoadState@zVid@@YAHXZ"}),
        (
            "symbol:recoil:function:0x431380",
            "0x431380",
            "0x4313d0",
        ): frozenset({"?GetTexturePackLoadState@zVid@@YAHXZ"}),
        (
            "symbol:recoil:function:0x4349a0",
            "0x4349a0",
            "0x434a80",
        ): frozenset({"??1HudUiNumericTextInput@@UAE@XZ"}),
        (
            "symbol:recoil:function:0x434a80",
            "0x434a80",
            "0x434b70",
        ): frozenset({"??1HudUiNumericTextInput@@UAE@XZ"}),
        (
            "symbol:recoil:function:0x434df0",
            "0x434df0",
            "0x434ee0",
        ): frozenset(
            {
                "??1HudUiNumericTextInput@@UAE@XZ",
                "??1HudUiZrdWidget@@UAE@XZ",
            }
        ),
        (
            "symbol:recoil:function:0x41a400",
            "0x41a400",
            "0x41a570",
        ): frozenset({"??1HudUiNumericTextInput@@UAE@XZ"}),
        (
            "symbol:recoil:function:0x41c400",
            "0x41c400",
            "0x41c480",
        ): frozenset({
            "??1HudUiNumericTextInput@@UAE@XZ",
            "?DestructorCore@HudUiZrdWidget@@QAEXXZ",
        }),
        (
            "symbol:recoil:function:0x41bd80",
            "0x41bd80",
            "0x41be70",
        ): frozenset({"??0HudUiZrdWidget@@QAE@XZ"}),
        (
            "symbol:recoil:function:0x4bdc70",
            "0x4bdc70",
            "0x4bde20",
        ): frozenset({"?Constructor@HudUiElement@@QAEPAU1@HH@Z"}),
        (
            "symbol:recoil:function:0x4bef90",
            "0x4bef90",
            "0x4bf060",
        ): frozenset({"??0HudUiElement@@QAE@HH@Z"}),
    }
    profile_key = (
        caller_identity,
        normalize_address(caller_start),
        normalize_address(caller_end_exclusive),
    )
    allowed_names = reviewed_profiles.get(profile_key)
    if allowed_names is None:
        return {}

    # These four dependent definitions are already exact authored tracker
    # identities, but VC5 source order does not preserve the retail call
    # ordinal inside the enclosing lifecycle bodies.  Bind only the complete
    # reviewed COD/COFF call-site population to the independently indexed
    # retail target population.  This is deliberately finite: caller extent,
    # candidate function symbol, every REL32 coordinate, and physical target
    # identity are all fixed.
    fixed_population_specs = {
        (
            "symbol:recoil:function:0x41a400",
            "0x41a400",
            "0x41a570",
            "??1HudUiNumericTextInput@@UAE@XZ",
        ): (
            "??1HudUiNetGameSetupPanel@@UAE@XZ",
            (0x91, 0xC1, 0xF1, 0x131),
            "",
        ),
        (
            "symbol:recoil:function:0x41c400",
            "0x41c400",
            "0x41c480",
            "??1HudUiNumericTextInput@@UAE@XZ",
        ): (
            "??1HudUiNewGamePanel@@UAE@XZ",
            (0x41,),
            "symbol:recoil:function:0x4b4ac0",
        ),
        (
            "symbol:recoil:function:0x41c400",
            "0x41c400",
            "0x41c480",
            "?DestructorCore@HudUiZrdWidget@@QAEXXZ",
        ): (
            "??1HudUiNewGamePanel@@UAE@XZ",
            (0x70,),
            "symbol:recoil:function:0x4b50c0",
        ),
        (
            "symbol:recoil:function:0x41bd80",
            "0x41bd80",
            "0x41be70",
            "??0HudUiZrdWidget@@QAE@XZ",
        ): (
            "??0HudUiNetExitPanel@@QAE@XZ",
            (0x33, 0x4B),
            "symbol:recoil:function:0x4b4ee0",
        ),
        (
            "symbol:recoil:function:0x4bdc70",
            "0x4bdc70",
            "0x4bde20",
            "?Constructor@HudUiElement@@QAEPAU1@HH@Z",
        ): (
            "??0HudWeatherFx@@QAE@H@Z",
            (0x2E,),
            "",
        ),
        (
            "symbol:recoil:function:0x4bef90",
            "0x4bef90",
            "0x4bf060",
            "??0HudUiElement@@QAE@HH@Z",
        ): (
            "??0zVideoFxPass3Config@@QAE@XZ",
            (0x32,),
            "",
        ),
    }

    required_names = allowed_names

    caller = candidate.caller_definition
    if caller is None:
        return {}
    invocation_indices = _candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(max(1, len(caller.data))),
    )
    offsets = _candidate_complete_instruction_offsets(candidate)
    symbols = document.collection("symbols")
    sites_by_name: dict[str, list[tuple[int, int, Instruction]]] = {}
    for ordinal, instruction_index in enumerate(invocation_indices):
        instruction = candidate.instructions[instruction_index]
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        name = _cc_cfg._instruction_operand(instruction).strip()
        if name not in required_names:
            continue
        semantic_name = _candidate_expected_direct_identity_name(name)
        if (
            not semantic_name
        ):
            continue
        offset = offsets[instruction_index]
        if offset is None:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "expected-direct identity bridge lacks one exact COD offset"
            )
        sites_by_name.setdefault(name, []).append(
            (ordinal, int(offset), instruction)
        )
    if not sites_by_name:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "expected-direct identity bridge lacks its exact reviewed caller population"
        )
    if not set(sites_by_name).issubset(required_names):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "expected-direct identity bridge reviewed caller population drifted"
        )

    resolved: dict[str, str] = {}
    for name, sites in sites_by_name.items():
        semantic_name = _candidate_expected_direct_identity_name(name)
        target_identities: set[str] = set()
        fixed_population_spec = fixed_population_specs.get(
            (*profile_key, name)
        )
        if fixed_population_spec is not None:
            fixed_caller_symbol, fixed_offsets, reviewed_fixed_identity = (
                fixed_population_spec
            )
            destructor_navigation_names: frozenset[str] = frozenset()
            if "::~" in semantic_name:
                destructor_owner = semantic_name.split("::~", 1)[0]
                destructor_navigation_names = frozenset({
                    destructor_owner + "::Destructor",
                    destructor_owner + "::DestructorCore",
                    destructor_owner + "::DestructorThunk",
                })
            retail_rows = [
                row
                for row in expected
                if row.get("form") == "call"
                and row.get("dispatch") == "direct"
                and row.get("identity_kind") == "direct"
                and row.get("storage_identity") == ""
                and row.get("slot_displacement") is None
                and isinstance(row.get("target_identity"), str)
                and isinstance(
                    symbols.get(
                        str(row.get("target_identity")).removeprefix(
                            "symbol:"
                        )
                    ),
                    Mapping,
                )
                and (
                    (
                        reviewed_fixed_identity
                        and row.get("target_identity")
                        == reviewed_fixed_identity
                    )
                    or (
                        not reviewed_fixed_identity
                        and (
                            symbols[
                                str(row.get("target_identity")).removeprefix(
                                    "symbol:"
                                )
                            ].get("navigation_name")
                            == semantic_name
                            or symbols[
                                str(row.get("target_identity")).removeprefix(
                                    "symbol:"
                                )
                            ].get("navigation_name")
                            in destructor_navigation_names
                        )
                    )
                )
                and (
                    not reviewed_fixed_identity
                    or row.get("ordinal")
                    in {ordinal for ordinal, _offset, _instruction in sites}
                )
            ]
            fixed_identities = {
                str(row.get("target_identity")) for row in retail_rows
            }
            fixed_identity = reviewed_fixed_identity or (
                next(iter(fixed_identities))
                if len(fixed_identities) == 1
                else ""
            )
            exact_retail_shape = {
                "form": "call",
                "dispatch": "direct",
                "identity_kind": "direct",
                "target_identity": fixed_identity,
                "storage_identity": "",
                "slot_displacement": None,
                "cleanup_bytes": None,
            }
            if (
                caller.symbol != fixed_caller_symbol
                or tuple(offset for _ordinal, offset, _instruction in sites)
                != fixed_offsets
                or not fixed_identity
                or fixed_identities != {fixed_identity}
                or len(retail_rows) != len(fixed_offsets)
                or any(
                    {
                        key: retail_row.get(key)
                        for key in exact_retail_shape
                    }
                    != exact_retail_shape
                    for retail_row in retail_rows
                )
            ):
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "expected-direct fixed-population identity bridge rejects "
                    "caller, REL32 coordinates, or immutable retail population "
                    f"drift: name={name!r}, caller_symbol={caller.symbol!r}, "
                    f"offsets={tuple(offset for _ordinal, offset, _instruction in sites)!r}, "
                    f"fixed_identity={fixed_identity!r}, "
                    f"retail_identities={fixed_identities!r}, "
                    f"retail_rows={len(retail_rows)}"
                )
            target_identities.add(fixed_identity)
        else:
            for ordinal, _offset, instruction in sites:
                if ordinal >= len(expected):
                    raise _cc_errors.CandidateCallContractEvidenceError(
                        "expected-direct identity bridge has no retail ordinal"
                    )
                expected_row = expected[ordinal]
                if (
                    expected_row.get("form")
                    != (
                        "tail"
                        if _cc_cfg._instruction_mnemonic(instruction) == "jmp"
                        else "call"
                    )
                    or expected_row.get("dispatch") != "direct"
                    or expected_row.get("identity_kind") != "direct"
                    or expected_row.get("storage_identity") != ""
                    or expected_row.get("slot_displacement") is not None
                ):
                    target_identities.clear()
                    break
                identity = expected_row.get("target_identity")
                if not isinstance(identity, str) or not identity.startswith(
                    "symbol:recoil:function:"
                ):
                    target_identities.clear()
                    break
                target_identities.add(identity)
        if len(target_identities) != 1:
            continue
        identity = next(iter(target_identities))
        symbol_id = identity.removeprefix("symbol:")
        symbol = symbols.get(symbol_id)
        address = str(symbol.get("address", "")) if isinstance(symbol, Mapping) else ""
        navigation_name = (
            str(symbol.get("navigation_name", ""))
            if isinstance(symbol, Mapping)
            else ""
        )
        destructor_navigation_names: frozenset[str] = frozenset()
        if "::~" in semantic_name:
            destructor_owner = semantic_name.split("::~", 1)[0]
            destructor_navigation_names = frozenset(
                {
                    destructor_owner + "::Destructor",
                    destructor_owner + "::DestructorCore",
                }
            )
        compatibility_navigation_names = {
            "HudUiNumericTextInput::BaseConstructor": frozenset({
                "HudUiNumericTextInput::Constructor",
            }),
        }.get(semantic_name, frozenset())
        if (
            not isinstance(symbol, Mapping)
            or symbol.get("kind") != "function"
            or symbol.get("pipeline_class")
            not in {"authored", "authored-lifecycle"}
            or (
                navigation_name != semantic_name
                and (
                    navigation_name not in destructor_navigation_names
                    and navigation_name not in compatibility_navigation_names
                )
            )
            or not address
            or indexes.by_address.get(normalize_address(address)) != identity
            or identity in indexes.provider_ids
        ):
            continue

        raw_matches = bridge_names.get(name)
        if raw_matches is not None:
            matches = (
                list(raw_matches)
                if isinstance(raw_matches, (list, tuple, set, frozenset))
                else [raw_matches]
            )
            raw_addresses = {
                normalize_address(str(getattr(item, "address", "")))
                for item in matches
                if getattr(item, "address", "")
            }
            if raw_addresses and raw_addresses != {normalize_address(address)}:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "expected-direct identity bridge conflicts with retail name authority"
                )

        exact_symbols = [row for row in caller.coff_symbols if row.name == name]
        references = sorted(
            (row for row in caller.relocations if row.symbol_name == name),
            key=lambda row: row.offset,
        )
        expected_reference_offsets = sorted(offset + 1 for _ordinal, offset, _ in sites)
        if (
            len(exact_symbols) != 1
            or caller.undefined_external_functions.count(name) != 1
            or name in caller.defined_external_functions
            or [row.offset for row in references] != expected_reference_offsets
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "expected-direct identity bridge requires one undefined COFF population"
            )
        external = exact_symbols[0]
        if (
            external.section_number != 0
            or external.symbol_type != 0x20
            or external.storage_class != IMAGE_SYM_CLASS_EXTERNAL
            or external.aux_count != 0
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "expected-direct identity bridge rejects malformed COFF external"
            )
        reference_by_offset = {row.offset: row for row in references}
        for _ordinal, offset, instruction in sites:
            reference = reference_by_offset[offset + 1]
            opcode = 0xE9 if _cc_cfg._instruction_mnemonic(instruction) == "jmp" else 0xE8
            if (
                bytes(int(item, 16) for item in instruction.bytes)
                != bytes((opcode, 0, 0, 0, 0))
                or reference.type != IMAGE_REL_I386_REL32
                or reference.symbol_index != external.index
                or reference.offset + 4 > len(caller.data)
                or caller.data[offset] != opcode
                or struct.unpack_from("<I", caller.data, reference.offset)[0] != 0
                or not all(caller.relocation_mask[reference.offset : reference.offset + 4])
            ):
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "expected-direct identity bridge requires exact E8/E9 REL32 provenance"
                )
        resolved[name] = identity
    return resolved


def _r4564_non_source_candidate_direct_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    reviewed_provider_iat_equivalences_out: dict[str, str] | None = None,
) -> dict[str, str]:
    """Resolve retained provider/compiler spellings against retail facts."""

    profiles = {
        (
            "symbol:recoil:function:0x4643a0", "0x4643a0", "0x464540"
        ): (
            "_ICDecompress", (0x94,), "iat:ICDecompress",
        ),
    }
    key = (
        caller_identity,
        normalize_address(caller_start),
        normalize_address(caller_end_exclusive),
    )
    if key == (
        "symbol:recoil:function:0x442790", "0x442790", "0x4427d0"
    ):
        if indexes.by_address.get("0x442790") != caller_identity:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "r4564 WOL Release retired scalar-deleting-destructor bridge "
                "requires the exact authored caller identity"
            )
        _cc_recoil_lifecycle._r4564_wol_release_retired_scalar_deleting_destructor_proof(candidate)
        return {}
    profile = profiles.get(key)
    if profile is None:
        return {}
    name, raw_expected_offsets, target_requirement = profile
    expected_offsets = (
        raw_expected_offsets
        if isinstance(raw_expected_offsets, tuple)
        else (raw_expected_offsets,)
    )
    caller = candidate.caller_definition
    if caller is None or indexes.by_address.get(key[1]) != caller_identity:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "r4564 non-source bridge requires exact authored caller"
        )
    offsets = _candidate_complete_instruction_offsets(candidate)
    invocations = _candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(max(1, len(caller.data))),
    )
    sites = [
        (ordinal, index, int(offsets[index]), candidate.instructions[index])
        for ordinal, index in enumerate(invocations)
        if offsets[index] is not None
        and _cc_cfg._instruction_operand(candidate.instructions[index]).strip() == name
    ]
    if tuple(site[2] for site in sites) != expected_offsets:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "r4564 non-source bridge call-site population drifted"
        )
    retail_rows = []
    for ordinal, _index, _offset, _instruction in sites:
        if ordinal >= len(expected):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "r4564 non-source bridge lacks immutable retail ordinal"
            )
        retail_rows.append(expected[ordinal])
    targets = {row.get("target_identity") for row in retail_rows}
    if len(targets) != 1:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "r4564 non-source bridge rejects retail identity/provider drift"
        )
    target = next(iter(targets))
    expected_identity_kind = (
        "iat"
        if target_requirement.startswith("iat:")
        else "provider"
        if target_requirement == "provider"
        or target_requirement in indexes.provider_ids
        else "direct"
    )
    if any(
        retail.get("form") != "call"
        or retail.get("dispatch") != "direct"
        or retail.get("identity_kind") != expected_identity_kind
        or retail.get("storage_identity") != ""
        or retail.get("slot_displacement") is not None
        or not isinstance(target, str)
        or (
            target_requirement == "provider"
            and target not in indexes.provider_ids
        )
        or (
            target_requirement != "provider"
            and target != target_requirement
        )
        for retail in retail_rows
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "r4564 non-source bridge rejects retail identity/provider drift"
        )
    if target_requirement.startswith("symbol:"):
        target_address = target_requirement.rsplit(":", 1)[-1]
        if indexes.by_address.get(target_address) != target_requirement:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "r4564 lifecycle bridge lacks exact authored destructor identity"
            )
    if target_requirement.startswith("iat:"):
        import_name = target_requirement.removeprefix("iat:")
        object_symbol = f"__imp__{import_name}"
        if (
            indexes.storage_by_name.get(import_name) != target_requirement
            or indexes.storage_by_name.get(object_symbol) != target_requirement
            or target_requirement in indexes.provider_ids
            or reviewed_provider_iat_equivalences_out is None
            or reviewed_provider_iat_equivalences_out
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "r4564 finite IAT bridge requires exact storage authority and "
                "one empty comparison-scoped equivalence sink"
            )
        reviewed_provider_iat_equivalences_out[target_requirement] = (
            target_requirement
        )
    exact_symbols = [row for row in caller.coff_symbols if row.name == name]
    references = [row for row in caller.relocations if row.symbol_name == name]
    if (
        len(exact_symbols) != 1
        or caller.undefined_external_functions.count(name) != 1
        or name in caller.defined_external_functions
        or [row.offset for row in references]
        != [offset + 1 for _ordinal, _index, offset, _instruction in sites]
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "r4564 non-source bridge requires one exact undefined COFF population"
        )
    external = exact_symbols[0]
    if (
        external.section_number != 0
        or external.symbol_type != 0x20
        or external.storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or external.aux_count != 0
        or any(
            reference.type != IMAGE_REL_I386_REL32
            or reference.symbol_index != external.index
            or bytes(int(item, 16) for item in instruction.bytes)
            != b"\xe8\x00\x00\x00\x00"
            or caller.data[offset : offset + 5] != b"\xe8\x00\x00\x00\x00"
            or not all(caller.relocation_mask[offset + 1 : offset + 5])
            for reference, (_ordinal, _index, offset, instruction) in zip(
                references, sites
            )
        )
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "r4564 non-source bridge rejects symbol/REL32/body drift"
        )
    return {name: target}


def _candidate_only_direct_callee_cleanup_contract(
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Infer positive direct-call cleanup without consulting retail rows.

    A receipt is emitted only when the exact caller E8/REL32 site names one
    collision-free candidate COMDAT and every reachable terminal in its
    complete COD CFG is the same ``ret N``.  Retail expected identity,
    ordinal, and cleanup are intentionally absent from this interface.
    """
    from _recoil.call_contract.records import CandidateAssembly

    result = [dict(row) for row in candidate_contract]
    receipts: list[dict[str, Any]] = []
    caller = candidate.caller_definition
    if caller is None or not candidate.instructions:
        return result, receipts
    invocation_indices = _candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(len(caller.data)),
    )
    instruction_offsets = _candidate_complete_instruction_offsets(candidate)
    if len(invocation_indices) != len(result):
        return result, receipts
    prefix = "candidate-local-coff:"
    for ordinal, row in enumerate(result):
        identity = row.get("target_identity")
        if (
            row.get("form") != "call"
            or row.get("dispatch") != "direct"
            or row.get("cleanup_bytes") is not None
            or not isinstance(identity, str)
            or not identity.startswith(prefix)
        ):
            continue
        name = identity[len(prefix) :]
        helper = candidate.tu_local_function_definitions.get(name)
        instruction_index = invocation_indices[ordinal]
        instruction = candidate.instructions[instruction_index]
        call_offset = instruction_offsets[instruction_index]
        references = tuple(
            relocation
            for relocation in caller.relocations
            if relocation.offset == call_offset + 1
            and relocation.symbol_name == name
        )
        symbols = tuple(
            symbol
            for symbol in caller.coff_symbols
            if symbol.name == name
        )
        if (
            helper is None
            or len(references) != 1
            or references[0].type != IMAGE_REL_I386_REL32
            or len(symbols) != 1
            or references[0].symbol_index != symbols[0].index
            or _cc_cfg._instruction_mnemonic(instruction) != "call"
            or _cc_cfg._instruction_operand(instruction).strip() != name
            or bytes(int(value, 16) for value in instruction.bytes)
            != b"\xe8\x00\x00\x00\x00"
            or call_offset + 5 > len(caller.data)
            or caller.data[call_offset : call_offset + 5]
            != b"\xe8\x00\x00\x00\x00"
            or not all(caller.relocation_mask[call_offset + 1 : call_offset + 5])
            or not helper.section_is_comdat
            or helper.comdat_selection is None
            or helper.section_external_functions != (name,)
            or helper.section_size != len(helper.data)
            or not helper.instructions
            or re.fullmatch(r"\?.+@@(?:QAE|UAE|SG).+@Z", name) is None
        ):
            continue
        helper_assembly = CandidateAssembly(
            instructions=helper.instructions,
            local_control_flow_indices=helper.local_control_flow_indices,
            local_control_flow_targets=helper.local_control_flow_targets,
        )
        helper_addresses = _cc_cfg._instruction_runtime_addresses(
            helper.instructions,
            source="cod",
            caller_start=0,
        )
        address_counts = Counter(
            address for address in helper_addresses if address is not None
        )
        index_by_address = {
            address: index
            for index, address in enumerate(helper_addresses)
            if address is not None and address_counts[address] == 1
        }
        successors, unresolved = _cc_cfg._exact_invocation_cfg(
            helper.instructions,
            instruction_addresses=helper_addresses,
            instruction_index_by_address=index_by_address,
            source="cod",
            caller_start=0,
            caller_end=len(helper.data),
            local_control_flow_indices=helper_assembly.local_control_flow_indices,
            local_control_flow_targets=helper_assembly.local_control_flow_targets,
        )
        reachable: set[int] = set()
        work = [0]
        while work:
            index = work.pop()
            if index in reachable or index < 0 or index >= len(helper.instructions):
                continue
            reachable.add(index)
            work.extend(successors.get(index, ()))
        terminals = tuple(
            index for index in sorted(reachable) if not successors.get(index, ())
        )
        cleanup_values = tuple(
            _cc_identity._exact_ret_cleanup_bytes(helper.instructions[index])
            for index in terminals
        )
        cleanup_set = {
            value for value in cleanup_values if isinstance(value, int)
        }
        if (
            unresolved & reachable
            or reachable != set(range(len(helper.instructions)))
            or not terminals
            or len(cleanup_set) != 1
            or any(value is None for value in cleanup_values)
            or next(iter(cleanup_set)) <= 0
        ):
            continue
        cleanup = next(iter(cleanup_set))
        row["cleanup_bytes"] = cleanup
        result[ordinal] = row
        receipts.append({
            "kind": "candidate-direct-callee-cleanup-receipt",
            "contract_version": 1,
            "candidate_expected_truth": False,
            "ordinal": ordinal,
            "callsite_offset": call_offset,
            "callee_symbol": name,
            "callee_body_bytes_hex": _cc_reporting._direct_bytes(helper.data).hex(),
            "callee_relocations": [
                {
                    "offset": relocation.offset,
                    "type": relocation.type,
                    "symbol": relocation.symbol_name,
                }
                for relocation in helper.relocations
            ],
            "reachable_return_offsets": [
                helper_addresses[index] for index in terminals
            ],
            "return_cleanup_bytes": cleanup,
            "decoded_callable_parameter_bytes": cleanup,
            "caller_stack_schedule_bytes_hex": _cc_reporting._direct_bytes(
                "\n".join(
                    item.raw_text for item in candidate.instructions[: instruction_index + 1]
                ).encode("utf-8")
            ).hex(),
        })
    return result, receipts


def _reviewed_r3994_static_table_inbound_target_bridges(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> dict[int, ReviewedInboundEntryRegisterTargetBridge]:
    """Allow the one reviewed ZUI inbound FF15 table slot, never a broad FF15."""

    if normalize_address(caller_start) != "0x4b9850":
        return {}
    if normalize_address(caller_end_exclusive) != "0x4b98d0":
        raise ValueError("reviewed ZUI static-table inbound caller extent drifted")
    target_identity = indexes.by_address.get("0x4b9850", "")
    if not target_identity:
        raise ValueError("reviewed ZUI static-table inbound target is unresolved")
    if _cc_cfg._hexdump_bytes(bridge.hexdump("0x4cc888", 12)) != bytes.fromhex(
        "70 40 40 00 50 98 4b 00 90 3d 40 00"
    ):
        raise ValueError("reviewed ZUI static-table inbound table drifted")
    payload = bridge.get_json("getXrefsTo", address="0x4b9850", limit=100000)
    code_rows, _data_rows, complete = _cc_identity._bn_inbound_xref_items(payload)
    source_addresses = [
        _cc_identity._bn_xref_address(row, "source_address", "source_addr", "from_address", "address")
        for row in code_rows
    ]
    if not complete or source_addresses != ["0x403f00"]:
        raise ValueError("reviewed ZUI static-table inbound xrefs drifted")
    _function_name, function_address = _cc_identity._bn_unique_containing_function(
        bridge,
        instruction_address="0x403f00",
    )
    source_rows = _cc_listing.parse_assembly(bridge.assembly(function_address), source="bn")
    exact = [
        row for row in source_rows
        if _cc_cfg._source_instruction_address(row) == "0x403f00"
    ]
    if len(exact) != 1 or bytes(int(item, 16) for item in exact[0].bytes) != bytes.fromhex(
        "ff 15 8c c8 4c 00"
    ):
        raise ValueError("reviewed ZUI static-table inbound FF15 drifted")
    # No entry-register calls exist in 0x4b9850, so this proof is a guard-only
    # authority package: it suppresses the generic E8/E9-only inbound demand
    # without publishing a synthetic invocation inside the callee.
    return {}


def _r4564_prove_candidate_dynamic_selection_calls(
    candidate: CandidateAssembly,
    complete_offsets: Sequence[int | None],
    invocation_indices: Sequence[int],
    *,
    ordinals: Sequence[int],
    source_load_registers: Mapping[int, str],
    direct_function_store_indices: frozenset[int] = frozenset(),
) -> tuple[tuple[int, str, int], ...]:
    """Prove every reviewed use of one VC5 callback-selection local.

    The compiler is free to rebase a fixed-frame local as outgoing arguments
    change ESP.  COD preserves the local symbol and its constant symbolic-to-
    encoded displacement delta, so this proof follows that exact local across
    every resolved CFG path.  Register calls must be fed by an exact load from
    the same local; stack calls must invoke that local directly.  Every value
    admitted into the local must descend from a relocation-proven callback
    data load or reviewed same-TU callback-function address store.
    """

    caller = candidate.caller_definition
    ordinal_rows = tuple(int(value) for value in ordinals)
    if (
        caller is None
        or not ordinal_rows
        or tuple(sorted(set(ordinal_rows))) != ordinal_rows
        or any(value < 0 or value >= len(invocation_indices) for value in ordinal_rows)
        or not source_load_registers
        or any(
            index < 0
            or index >= len(candidate.instructions)
            or register not in {
                "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
            }
            for index, register in source_load_registers.items()
        )
        or any(
            index < 0 or index >= len(candidate.instructions)
            for index in direct_function_store_indices
        )
    ):
        raise ValueError(
            "r4564 candidate dynamic selection proof is malformed"
        )

    call_indices = tuple(invocation_indices[value] for value in ordinal_rows)
    call_shapes: dict[int, tuple[str, str, int]] = {}
    local_rows: set[tuple[str, int]] = set()
    for call_index in call_indices:
        call = candidate.instructions[call_index]
        call_offset = complete_offsets[call_index]
        symbolic_call = _cc_identity._exact_vc5_symbolic_stack_indirect_invocation(call)
        register_call = _cc_dispatch._exact_vc5_register_indirect_invocation(call)
        if call_offset is None or (symbolic_call is None) == (register_call is None):
            raise ValueError(
                "r4564 candidate dynamic selection requires exact register-or-"
                "symbolic-stack call sites"
            )
        if symbolic_call is not None:
            form, encoded_displacement, symbol, rendered_displacement = symbolic_call
            if form != "call":
                raise ValueError(
                    "r4564 candidate dynamic selection invocation form drifted"
                )
            local_rows.add((symbol, rendered_displacement - encoded_displacement))
            call_shapes[call_index] = (
                "esp",
                symbol,
                rendered_displacement,
            )
        else:
            assert register_call is not None
            form, register = register_call
            if form != "call":
                raise ValueError(
                    "r4564 candidate dynamic selection invocation form drifted"
                )
            call_shapes[call_index] = (register, "", 0)
    if len(local_rows) != 1:
        raise ValueError(
            "r4564 candidate dynamic selection calls do not share one exact local"
        )
    local_symbol, local_coordinate = next(iter(local_rows))
    if not -0x10000 <= local_coordinate <= 0x10000:
        raise ValueError(
            "r4564 candidate dynamic selection local coordinate is unbounded"
        )

    instruction_addresses = tuple(
        int(offset) if offset is not None else None
        for offset in complete_offsets
    )
    if (
        any(address is None for address in instruction_addresses)
        or len(set(instruction_addresses)) != len(instruction_addresses)
        or any(
            address is None or not 0 <= address < len(caller.data)
            for address in instruction_addresses
        )
    ):
        raise ValueError(
            "r4564 candidate dynamic selection lacks a complete unique "
            "instruction cover"
        )
    instruction_index_by_address = {
        int(address): index
        for index, address in enumerate(instruction_addresses)
        if address is not None
    }
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        candidate.instructions,
        instruction_addresses=instruction_addresses,
        instruction_index_by_address=instruction_index_by_address,
        source="cod",
        caller_start=0,
        caller_end=len(caller.data),
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    entry_reachable = _cc_cfg._reachable_cfg_indices(successors, (0,))
    for call_index in call_indices:
        reaching_call = _cc_cfg._indices_reaching_cfg_target(successors, call_index)
        if (
            call_index not in entry_reachable
            or unresolved & entry_reachable & reaching_call
        ):
            raise ValueError(
                "r4564 candidate dynamic selection has an unresolved or "
                "unreachable invocation path"
            )

    # Register provenance distinguishes a direct family-source load from a
    # value loaded back from the reviewed selection local.  This prevents a
    # same-family register value from bypassing the required shared local.
    work: list[tuple[int, tuple[tuple[str, str], ...], bool]] = [(0, (), False)]
    visited: set[tuple[int, tuple[tuple[str, str], ...], bool]] = set()
    observed: dict[int, set[bool]] = {index: set() for index in call_indices}
    matching_store_count = 0
    while work:
        index, register_rows, local_live = work.pop()
        state = (index, register_rows, local_live)
        if state in visited:
            continue
        visited.add(state)
        if len(visited) > 0x20000:
            raise ValueError(
                "r4564 candidate dynamic selection state is unbounded"
            )
        register_provenance = dict(register_rows)
        instruction = candidate.instructions[index]
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        symbolic_store = _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_store(instruction)
        symbolic_load = _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(instruction)
        function_store = _cc_identity._exact_vc5_symbolic_stack_function_store(instruction)

        if index in call_shapes:
            call_register, call_symbol, _call_displacement = call_shapes[index]
            if call_register == "esp":
                observed[index].add(call_symbol == local_symbol and local_live)
            else:
                observed[index].add(
                    register_provenance.get(call_register) == "local"
                )

        if index in source_load_registers:
            destination = source_load_registers[index]
            register_provenance[destination] = "source"
        elif (move := _cc_receiver_instructions._exact_register_move(instruction)) is not None:
            destination, source_register = move
            if source_register in register_provenance:
                register_provenance[destination] = register_provenance[source_register]
            else:
                register_provenance.pop(destination, None)
        elif symbolic_store is not None:
            (
                encoded_displacement,
                source_register,
                symbol,
                rendered_displacement,
            ) = symbolic_store
            if (
                symbol == local_symbol
                and rendered_displacement - encoded_displacement
                == local_coordinate
            ):
                matching_store_count += 1
                local_live = source_register in register_provenance
        elif function_store is not None:
            (
                encoded_displacement,
                symbol,
                rendered_displacement,
                _function_name,
                _immediate_delta,
            ) = function_store
            if (
                symbol == local_symbol
                and rendered_displacement - encoded_displacement
                == local_coordinate
            ):
                matching_store_count += 1
                local_live = index in direct_function_store_indices
        elif symbolic_load is not None:
            (
                encoded_displacement,
                destination,
                symbol,
                rendered_displacement,
            ) = symbolic_load
            if (
                symbol == local_symbol
                and rendered_displacement - encoded_displacement
                == local_coordinate
                and local_live
            ):
                register_provenance[destination] = "local"
            else:
                register_provenance.pop(destination, None)
        else:
            operand = _cc_cfg._instruction_operand(instruction)
            if re.search(
                rf"(?<![A-Za-z0-9_@$?]){re.escape(local_symbol)}"
                r"(?![A-Za-z0-9_@$?])",
                operand,
            ):
                if index not in call_shapes:
                    raise ValueError(
                        "r4564 candidate dynamic selection local escapes its "
                        "exact load/store/call lineage"
                    )
            for register in tuple(register_provenance):
                if _cc_cfg._instruction_may_clobber_register(instruction, register):
                    register_provenance.pop(register, None)

        next_register_rows = tuple(sorted(register_provenance.items()))
        for successor in successors.get(index, ()):
            work.append((successor, next_register_rows, local_live))

    if matching_store_count == 0 or any(values != {True} for values in observed.values()):
        raise ValueError(
            "r4564 candidate dynamic selection stack local is not assigned "
            "from the exact callback family on every reviewed invocation path"
        )
    return tuple(
        (
            int(complete_offsets[index]),
            call_shapes[index][0],
            call_shapes[index][2],
        )
        for index in call_indices
    )


def _compose_supplemental_candidate_direct_bridges(
    *bridge_maps: Mapping[str, str],
) -> dict[str, str]:
    """Compose independent caller-scoped direct proofs without overwrite.

    These maps feed a provisional complete-caller extraction.  A plain mapping
    expansion would silently let the last producer win before the ordinal
    producer can enforce its own collision gates.
    """

    result: dict[str, str] = {}
    for additions in bridge_maps:
        result.update(additions)
    return result


def _arbitrate_candidate_local_reviewed_bridges(
    candidate_local_bridges: Mapping[str, str],
    *,
    reviewed_compiler_provider_bridges: Mapping[str, str],
    exact_destructor_bridges: Mapping[str, str],
    exact_constructor_bridges: Mapping[str, str],
    exact_zwep_provider_bridges: Mapping[str, str] | None = None,
) -> dict[str, str]:
    """Reconcile independently proven same-object callable identities.

    The early candidate-local proof must remain available to downstream bridge
    producers, so complete definitions initially receive comparison-only
    ``candidate-local-coff:`` identities.  Only the exact result maps from the
    later destructor and constructor proof producers may replace those local
    sentinels.  A name match or an overlap from any other reviewed producer is
    insufficient, and the two authorized producers may not disagree.
    """

    exact_proof_bridges: dict[str, str] = {}
    exact_proof_sources: dict[str, str] = {}
    for proof_source, proof_bridges in (
        ("destructor", exact_destructor_bridges),
        ("constructor", exact_constructor_bridges),
        ("zWep-provider", exact_zwep_provider_bridges or {}),
    ):
        for name, identity in proof_bridges.items():
            prior_identity = exact_proof_bridges.get(name)
            if prior_identity is not None and prior_identity != identity:
                raise ValueError(
                    "late candidate-local arbitration received conflicting "
                    "identities from its authorized exact proof producers "
                    f"for {name!r}: "
                    f"{exact_proof_sources[name]}={prior_identity!r}, "
                    f"{proof_source}={identity!r}"
                )
            exact_proof_bridges[name] = identity
            exact_proof_sources[name] = proof_source

    for name, identity in exact_proof_bridges.items():
        if reviewed_compiler_provider_bridges.get(name) != identity:
            raise ValueError(
                "late candidate-local arbitration received an authorized "
                "exact proof identity that does not survive unchanged in "
                f"the reviewed compiler/provider population for {name!r}"
            )

    result = dict(candidate_local_bridges)
    overlaps = (
        reviewed_compiler_provider_bridges.keys()
        & candidate_local_bridges.keys()
    )
    for name in overlaps:
        local_identity = candidate_local_bridges[name]
        reviewed_identity = reviewed_compiler_provider_bridges[name]
        if local_identity == reviewed_identity:
            continue
        expected_local_identity = f"candidate-local-coff:{name}"
        exact_proof_identity = exact_proof_bridges.get(name)
        if (
            local_identity != expected_local_identity
            or exact_proof_identity is None
            or exact_proof_identity != reviewed_identity
        ):
            raise ValueError(
                "late candidate-local arbitration rejects an "
                "unproved or conflicting reviewed overlap for "
                f"{name!r}: local={local_identity!r}, "
                f"reviewed={reviewed_identity!r}, "
                f"exact-proof={exact_proof_identity!r}"
            )
        result[name] = exact_proof_identity
    return result


def _require_unique_exact_candidate_direct_external_call_row(
    candidate: CandidateAssembly,
    *,
    candidate_symbol: str,
) -> tuple[int, CoffRelocation]:
    """Prove one case-sensitive COD/COFF direct external call row.

    The caller-object relocation is the positional authority.  A source-shaped
    compiler change may move the call without changing its static invocation
    contract, so this proof joins the unique COD instruction to its unique
    REL32 row instead of assigning a reviewed identity from a fixed candidate
    offset.  Every spelling, population, opcode, addend, and mask check remains
    exact and candidate-local.
    """

    definition = candidate.caller_definition
    if definition is None:
        raise ValueError(
            "candidate direct external bridge requires one exact complete "
            "caller COFF definition"
        )
    folded = candidate_symbol.casefold()
    folded_mentions = [
        instruction
        for instruction in candidate.instructions
        if folded in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    offsets = _cc_recoil_mfc._wol_category_a_candidate_offset_map(candidate)
    exact_calls = [
        (offset, instruction)
        for offset, instruction in offsets.items()
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction).strip() == candidate_symbol
    ]
    folded_references = [
        row
        for row in definition.relocations
        if row.symbol_name.casefold() == folded
    ]
    folded_external_rows = [
        (population_name, name)
        for population_name, population in (
            ("undefined-function", definition.undefined_external_functions),
            ("undefined-data", definition.undefined_external_data),
            ("defined-function", definition.defined_external_functions),
            ("defined-data", definition.defined_external_data),
        )
        for name in population
        if name.casefold() == folded
    ]
    if (
        len(folded_mentions) != 1
        or len(exact_calls) != 1
        or len(folded_references) != 1
        or folded_references[0].symbol_name != candidate_symbol
        or folded_external_rows != [("undefined-function", candidate_symbol)]
    ):
        raise ValueError(
            "candidate direct external bridge requires one exact case-sensitive "
            "CALL/REL32/undefined-function object row"
        )

    call_offset, call = exact_calls[0]
    reference = folded_references[0]
    relocation_offset = call_offset + 1
    if (
        tuple(value.lower() for value in call.bytes)
        != ("e8", "00", "00", "00", "00")
        or call_offset < 0
        or call_offset + 5 > len(definition.data)
        or len(definition.relocation_mask) != len(definition.data)
        or definition.data[call_offset : call_offset + 5]
        != b"\xe8\0\0\0\0"
        or reference.offset != relocation_offset
        or reference.type != IMAGE_REL_I386_REL32
        or relocation_offset + 4 > len(definition.data)
        or struct.unpack_from("<I", definition.data, relocation_offset)[0]
        != 0
        or definition.relocation_mask[call_offset]
        or not all(
            definition.relocation_mask[index]
            for index in range(relocation_offset, relocation_offset + 4)
        )
    ):
        raise ValueError(
            "candidate direct external bridge requires one exact case-sensitive "
            "CALL/REL32/undefined-function object row"
        )
    return call_offset, reference


def _chkstk_compiler_helper_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    retail_instructions: Sequence[Instruction],
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
    """Bind exact finite VC5 stack-probe calls to their retail helper body.

    ``__chkstk`` is a compiler-emitted candidate spelling, not identity
    authority.  This bridge is therefore deliberately limited to the finite
    reviewed caller census and requires immutable retail call bytes, the
    complete retail helper body, and exact current tracker rows before
    publishing a candidate name.
    """

    caller_spec = _cc_catalog.MSVC_CHKSTK_CALLER_SPECS.get(caller_identity)
    if caller_spec is None:
        return {}
    if (
        normalize_address(caller_start) != caller_spec["address"]
        or normalize_address(caller_end_exclusive)
        != caller_spec["end_exclusive"]
    ):
        return {}
    candidate_symbol = str(
        caller_spec.get("candidate_symbol", _cc_catalog.MSVC_CHKSTK_CANDIDATE_SYMBOL)
    )

    mentions = [
        instruction
        for instruction in candidate.instructions
        if candidate_symbol.casefold()
        in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    if not mentions:
        return {}
    if any(
        _cc_cfg._instruction_mnemonic(instruction) != "call"
        or _cc_cfg._instruction_operand(instruction).strip()
        != candidate_symbol
        for instruction in mentions
    ):
        raise ValueError(
            "MSVC stack-probe bridge accepts only exact direct CALL operands; "
            "aliases, case drift, and indirect forms are forbidden"
        )

    caller_row = document.collection("symbols").get(
        caller_identity.removeprefix("symbol:")
    )
    target_row = document.collection("symbols").get(
        _cc_catalog.MSVC_CHKSTK_TARGET_SYMBOL_ID
    )
    target_block = document.collection("physical_blocks").get(
        _cc_catalog.MSVC_CHKSTK_TARGET_BLOCK_ID
    )
    if (
        not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("address") != caller_spec["address"]
        or caller_row.get("end_exclusive")
        != caller_spec["end_exclusive"]
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != caller_spec["size"]
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("physical_block_id")
        != caller_spec["physical_block_id"]
        or indexes.by_address.get(str(caller_spec["address"]))
        != caller_identity
    ):
        raise ValueError(
            "MSVC __chkstk bridge requires its exact governed authored caller"
        )

    if caller_spec.get("target_shape") == "wol-two-tu-authored":
        _cc_recoil_mfc._require_wol_chkstk_caller_target_provenance(
            candidate,
            document=document,
            caller_row=caller_row,
            caller_spec=caller_spec,
        )
        governed_target_id = ""
    else:
        governed_target_id = str(caller_spec.get("target_id", ""))
    if governed_target_id:
        target = candidate.target
        target_registration = document.collection("verification_targets").get(
            governed_target_id
        )
        registration = (
            target_registration.get("registration")
            if isinstance(target_registration, Mapping)
            else None
        )
        source_path = str(caller_spec["source_path"])
        target_source_path = str(caller_spec.get("target_source_from", source_path))
        source_trace = caller_row.get("source_traceability")
        source_edges = (
            source_trace.get("source_edges")
            if isinstance(source_trace, Mapping)
            else None
        )
        target_functions = tuple(getattr(target, "functions", ()))
        target_function_rows = [
            row
            for row in target_functions
            if normalize_address(str(getattr(row, "address", "")))
            == caller_spec["address"]
        ]
        if (
            target is None
            or getattr(target, "name", "") != caller_spec["target_name"]
            or getattr(target, "target_binary", "") != "recoil"
            or Path(str(getattr(target, "manifest_path", ""))).resolve()
            != Path(caller_spec["target_manifest"]).resolve()
            or getattr(target, "source_from", "") != target_source_path
            or len(target_function_rows) != 1
            or getattr(target_function_rows[0], "symbol", "")
            != caller_spec["symbol"]
            or getattr(target_function_rows[0], "pipeline_class", "")
            != "authored"
            or getattr(target_function_rows[0], "authored_order_role", "")
            != "authored-body"
            or not isinstance(target_registration, Mapping)
            or target_registration.get("binary") != "recoil"
            or target_registration.get("kind") != "vc5"
            or target_registration.get("name") != caller_spec["target_name"]
            or target_registration.get("registered_addresses", []).count(
                caller_spec["address"]
            )
            != 1
            or not isinstance(registration, Mapping)
            or registration.get("manifest_path")
            != Path(caller_spec["target_manifest"])
            .relative_to(REPO_ROOT)
            .as_posix()
            or registration.get("source_from") != target_source_path
            or not _cc_targets._target_function_definition_source_matches(
                target, registration, address=caller_spec["address"],
                symbol=caller_spec["symbol"], source_path=source_path,
            )
            or registration.get("order_edit_paths")
            != list(caller_spec.get("order_edit_paths", (source_path,)))
            or registration.get("function_addresses", []).count(
                caller_spec["address"]
            )
            != 1
            or not isinstance(source_trace, Mapping)
            or source_trace.get("state") != "resolved"
            or source_edges
            != [
                {
                    "anchor_id": caller_spec["source_anchor"],
                    "emission_context": {"translation_unit": source_path},
                    "evidence_ids": [],
                    "relation": "defines",
                }
            ]
            or tuple(caller_row.get("verification_target_ids", ()))
            != caller_spec["verification_target_ids"]
        ):
            raise ValueError(
                "MSVC __chkstk bridge requires the exact selected target, "
                "manifest registration, and caller source trace"
            )

    if (
        not isinstance(target_row, Mapping)
        or target_row.get("binary") != "recoil"
        or target_row.get("kind") != "function"
        or target_row.get("pipeline_class") != "non-authored"
        or target_row.get("authored_order_role") != "non-authored"
        or target_row.get("address") != _cc_catalog.MSVC_CHKSTK_TARGET_ADDRESS
        or target_row.get("end_exclusive")
        != _cc_catalog.MSVC_CHKSTK_TARGET_END_EXCLUSIVE
        or target_row.get("extent_state") != "known"
        or target_row.get("size") != len(_cc_catalog.MSVC_CHKSTK_RETAIL_BODY)
        or target_row.get("navigation_name") != _cc_catalog.MSVC_CHKSTK_RETAIL_NAME
        or target_row.get("output_section_id") != "recoil:section:.text"
        or target_row.get("physical_block_id")
        != _cc_catalog.MSVC_CHKSTK_TARGET_BLOCK_ID
        or target_row.get("disposition") != "unresolved"
        or target_row.get("ownership_state") != "unresolved"
        or any(
            key in target_row
            for key in (
                "import_dll",
                "import_name",
                "import_ordinal",
                "provider_object_identity",
            )
        )
        or indexes.by_address.get(_cc_catalog.MSVC_CHKSTK_TARGET_ADDRESS)
        != _cc_catalog.MSVC_CHKSTK_TARGET_IDENTITY
    ):
        raise ValueError(
            "MSVC __chkstk bridge lacks the exact distinct non-authored "
            "compiler-helper target row"
        )

    mapping = (
        target_block.get("mapping")
        if isinstance(target_block, Mapping)
        else None
    )
    if (
        not isinstance(target_block, Mapping)
        or target_block.get("binary") != "recoil"
        or target_block.get("row_kind") != "physical-source-block"
        or target_block.get("start") != _cc_catalog.MSVC_CHKSTK_TARGET_BLOCK_ID.rsplit(
            ":", 1
        )[-1]
        or target_block.get("end_exclusive") != "0x4c637c"
        or target_block.get("contribution_kind") != "provider"
        or target_block.get("agent_source_path")
        != "provider:vc5-crt-startup-runtime"
        or target_block.get("source_path")
        != "provider:vc5-crt-startup-runtime"
        or not isinstance(target_block.get("contribution_ids"), list)
        or target_block["contribution_ids"].count(
            _cc_catalog.MSVC_CHKSTK_TARGET_SYMBOL_ID
        )
        != 1
        or not isinstance(mapping, Mapping)
        or mapping.get("status") != "provider-boundary"
    ):
        raise ValueError(
            "MSVC __chkstk bridge lacks the exact containing runtime block and "
            "unique compiler-helper contribution"
        )

    retail_symbols = bridge_names.get(_cc_catalog.MSVC_CHKSTK_RETAIL_NAME)
    if (
        not isinstance(retail_symbols, Sequence)
        or isinstance(retail_symbols, (str, bytes))
        or len(retail_symbols) != 1
    ):
        raise ValueError(
            "MSVC __chkstk bridge requires one exact retail __alloca_probe "
            "function symbol"
        )
    retail_symbol = retail_symbols[0]
    if (
        getattr(retail_symbol, "address", "")
        != _cc_catalog.MSVC_CHKSTK_TARGET_ADDRESS
        or getattr(retail_symbol, "name", "") != _cc_catalog.MSVC_CHKSTK_RETAIL_NAME
        or getattr(retail_symbol, "raw_name", "")
        != _cc_catalog.MSVC_CHKSTK_RETAIL_NAME
        or getattr(retail_symbol, "full_name", "")
        not in {"", _cc_catalog.MSVC_CHKSTK_RETAIL_NAME}
        or getattr(retail_symbol, "kind", "") != "function"
        or (
            candidate_symbol != _cc_catalog.MSVC_CHKSTK_RETAIL_NAME
            and candidate_symbol in bridge_names
        )
        or candidate_symbol in indexes.by_candidate_name
        or candidate_symbol in indexes.storage_by_name
    ):
        raise ValueError(
            "MSVC __chkstk bridge has retail-name, target-address, or candidate "
            "name collision drift"
        )
    if _cc_cfg._hexdump_bytes(
        bridge.hexdump(
            _cc_catalog.MSVC_CHKSTK_TARGET_ADDRESS,
            len(_cc_catalog.MSVC_CHKSTK_RETAIL_BODY),
        )
    ) != _cc_catalog.MSVC_CHKSTK_RETAIL_BODY:
        raise ValueError(
            "MSVC __chkstk retail helper body or page-stride probing shape drifted"
        )

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(str(caller_spec["address"])),
    )
    retail_by_address = {
        address: instruction
        for address, instruction in zip(retail_addresses, retail_instructions)
        if address is not None
    }
    if "retail_call_offsets" in caller_spec:
        retail_call_offsets = tuple(
            int(value) for value in caller_spec["retail_call_offsets"]
        )
    else:
        retail_call_offsets = (int(caller_spec["retail_call_offset"]),)
    retail_call_addresses = tuple(
        address_value(str(caller_spec["address"])) + offset
        for offset in retail_call_offsets
    )
    retail_calls = tuple(
        retail_by_address.get(address) for address in retail_call_addresses
    )
    dynamic_retail_setup_rows = tuple(
        (int(offset), bytes(body))
        for offset, body in caller_spec.get("retail_setup_rows", ())
    )
    if dynamic_retail_setup_rows:
        retail_setup_exact = all(
            (
                instruction := retail_by_address.get(
                    address_value(str(caller_spec["address"])) + offset
                )
            )
            is not None
            and bytes(int(value, 16) for value in instruction.bytes) == body
            for offset, body in dynamic_retail_setup_rows
        )
    else:
        retail_setup_address = (
            address_value(str(caller_spec["address"]))
            + int(caller_spec["retail_setup_offset"])
        )
        retail_frame_size = int(caller_spec["retail_frame_size"])
        retail_setup = retail_by_address.get(retail_setup_address)
        retail_setup_exact = bool(
            retail_setup is not None
            and _cc_cfg._instruction_mnemonic(retail_setup) == "mov"
            and bytes(int(value, 16) for value in retail_setup.bytes)
            == b"\xb8" + struct.pack("<I", retail_frame_size)
        )
    if (
        not retail_setup_exact
        or not retail_calls
        or any(call is None for call in retail_calls)
        or any(
            _cc_cfg._instruction_mnemonic(call) != "call"
            or len(call.bytes) != 5
            or tuple(call.bytes[:1]) != ("e8",)
            or _cc_cfg._instruction_operand(call).strip()
            != _cc_catalog.MSVC_CHKSTK_RETAIL_NAME
            or call_address
            + 5
            + struct.unpack_from(
                "<i", bytes(int(value, 16) for value in call.bytes), 1
            )[0]
            != address_value(_cc_catalog.MSVC_CHKSTK_TARGET_ADDRESS)
            for call_address, call in zip(
                retail_call_addresses, retail_calls
            )
            if call is not None
        )
    ):
        raise ValueError(
            "MSVC __chkstk bridge retail caller setup, direct target, or call "
            "bytes drifted"
        )

    expected_rows = [
        row
        for row in expected
        if row.get("target_identity") == _cc_catalog.MSVC_CHKSTK_TARGET_IDENTITY
    ]
    if "retail_ordinals" in caller_spec:
        retail_ordinals = tuple(
            int(value) for value in caller_spec["retail_ordinals"]
        )
    elif caller_spec.get("retail_ordinal") is not None:
        retail_ordinals = (int(caller_spec["retail_ordinal"]),)
    else:
        retail_ordinals = ()
    if (
        len(expected_rows) != len(retail_call_offsets)
        or retail_ordinals
        and tuple(row.get("ordinal") for row in expected_rows)
        != retail_ordinals
        or any(
            row.get("form") != "call"
            or row.get("dispatch") != "direct"
            or row.get("identity_kind") != "provider"
            or row.get("storage_identity") != ""
            or row.get("slot_displacement") is not None
            or row.get("cleanup_bytes") is not None
            for row in expected_rows
        )
    ):
        raise ValueError(
            "MSVC __chkstk bridge lacks the exact retail direct no-cleanup "
            "compiler-helper invocation population and ordinals"
        )

    definition = candidate.caller_definition
    offsets = _candidate_complete_instruction_offsets(candidate)
    rows_by_offset = {
        offset: candidate.instructions[index]
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    if "candidate_call_offsets" in caller_spec:
        candidate_call_offsets = tuple(
            int(value) for value in caller_spec["candidate_call_offsets"]
        )
    else:
        candidate_call_offsets = (
            int(caller_spec.get("candidate_call_offset", 5)),
        )
    candidate_calls = tuple(
        rows_by_offset.get(offset) for offset in candidate_call_offsets
    )
    dynamic_candidate_setup_rows = tuple(
        (int(offset), bytes(body))
        for offset, body in caller_spec.get("candidate_setup_rows", ())
    )
    if dynamic_candidate_setup_rows:
        instruction_offsets = {id(row): offset for row, offset in zip(candidate.instructions, offsets)}
        observed_call_offsets = tuple(instruction_offsets.get(id(row)) for row in mentions)
        dynamic_candidate_setup_rows = _cc_identity._translated_dynamic_probe_setup(
            candidate_call_offsets, observed_call_offsets, dynamic_candidate_setup_rows,
        )
        candidate_call_offsets = tuple(int(offset) for offset in observed_call_offsets)
        candidate_calls = tuple(rows_by_offset.get(offset) for offset in candidate_call_offsets)
    candidate_setup_offset = int(
        caller_spec.get("candidate_setup_offset", 0)
    )
    candidate_setup = rows_by_offset.get(candidate_setup_offset)
    candidate_frame_body = b""
    if dynamic_candidate_setup_rows:
        candidate_setup_exact = _candidate_dynamic_probe_setup_matches(
            candidate, offsets, dynamic_candidate_setup_rows)
    else:
        candidate_frame_sizes = tuple(
            int(value)
            for value in caller_spec.get(
                "candidate_frame_sizes",
                (caller_spec["candidate_frame_size"],),
            )
        )
        candidate_frame_bodies = {
            struct.pack("<I", frame_size)
            for frame_size in candidate_frame_sizes
        }
        candidate_frame_body = (
            bytes(int(value, 16) for value in candidate_setup.bytes[1:])
            if candidate_setup is not None and len(candidate_setup.bytes) == 5
            else b""
        )
        candidate_setup_exact = bool(
            candidate_setup is not None
            and _cc_cfg._instruction_mnemonic(candidate_setup) == "mov"
            and tuple(candidate_setup.bytes[:1]) == ("b8",)
            and len(candidate_setup.bytes) == 5
            and candidate_frame_body in candidate_frame_bodies
        )
    if (
        definition is None
        or definition.symbol != caller_spec["symbol"]
        or len(mentions) != len(candidate_call_offsets)
        or len(candidate_calls) != len(mentions)
        or any(call is None for call in candidate_calls)
        or any(
            call is not mention
            for call, mention in zip(candidate_calls, mentions)
        )
        or not candidate_setup_exact
        or any(
            tuple(call.bytes) != ("e8", "00", "00", "00", "00")
            for call in candidate_calls
            if call is not None
        )
        or definition.undefined_external_functions.count(
            candidate_symbol
        )
        != 1
        or candidate_symbol
        in (
            definition.defined_external_functions
            + definition.undefined_external_data
            + definition.defined_external_data
        )
    ):
        raise ValueError(
            "MSVC stack-probe bridge requires the exact reviewed EAX setup, "
            "offset-five/direct or explicitly reviewed candidate call "
            "coordinates, caller definition, and unique "
            "undefined helper symbol"
        )

    references = tuple(sorted(
        (
            relocation
            for relocation in definition.relocations
            if relocation.symbol_name == candidate_symbol
        ),
        key=lambda relocation: relocation.offset,
    ))
    relocation_offsets = tuple(
        candidate_call_offset + 1
        for candidate_call_offset in candidate_call_offsets
    )
    if (
        len(references) != len(relocation_offsets)
        or tuple(reference.offset for reference in references)
        != relocation_offsets
        or any(
            reference.type != IMAGE_REL_I386_REL32
            for reference in references
        )
        or (
            caller_spec.get("candidate_relocation_symbol_index") is not None
            and any(
                reference.symbol_index
                != int(caller_spec["candidate_relocation_symbol_index"])
                for reference in references
            )
        )
        or any(
            len(definition.data) < relocation_offset + 4
            or len(definition.relocation_mask) < relocation_offset + 4
            or definition.data[
                candidate_call_offset:relocation_offset
            ] != b"\xe8"
            or struct.unpack_from(
                "<I", definition.data, relocation_offset
            )[0] != 0
            or not all(
                definition.relocation_mask[index]
                for index in range(relocation_offset, relocation_offset + 4)
            )
            or definition.relocation_mask[candidate_call_offset]
            for candidate_call_offset, relocation_offset in zip(
                candidate_call_offsets, relocation_offsets
            )
        )
        or (
            not dynamic_candidate_setup_rows
            and (
                definition.data[
                    candidate_setup_offset:candidate_setup_offset + 1
                ]
                != b"\xb8"
                or definition.data[
                    candidate_setup_offset + 1:candidate_setup_offset + 5
                ]
                != candidate_frame_body
            )
        )
    ):
        raise ValueError(
            "MSVC stack-probe bridge requires the exact zero-addend fully "
            "masked E8 REL32 relocation population at every reviewed call "
            "coordinate"
        )

    return {candidate_symbol: _cc_catalog.MSVC_CHKSTK_TARGET_IDENTITY}


def _candidate_caller_definition_interval(
    definition: CandidateCallerDefinition,
) -> tuple[int, int]:
    """Return the exact selected body interval, including legacy test bodies."""

    start = definition.section_start
    end = definition.section_end
    if start == 0 and end == 0 and definition.data:
        # Older isolated unit fixtures predate explicit COFF interval fields.
        # Live candidate construction always publishes them.  Treat only the
        # unambiguous zero-based whole-body fixture as [0, len(data)); any
        # partially populated or nonzero interval remains fail-closed.
        end = len(definition.data)
    if (
        start < 0
        or end <= start
        or end - start != len(definition.data)
        or len(definition.relocation_mask) != len(definition.data)
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "candidate proof requires one exact selected caller definition interval"
        )
    return start, end


def _candidate_instruction_section_offset(
    instruction: Instruction,
    definition: CandidateCallerDefinition,
) -> int:
    """Bind a COD instruction offset to one unambiguous COFF body field."""

    raw_address = _cc_cfg._source_instruction_address(instruction)
    if not raw_address:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "candidate instruction lacks an exact source offset"
        )
    raw_offset = address_value(raw_address)
    definition_start, definition_end = _candidate_caller_definition_interval(
        definition
    )
    body_size = definition_end - definition_start
    local_offset = (
        definition_start + raw_offset
        if 0 <= raw_offset < body_size
        else None
    )
    section_offset = (
        raw_offset
        if definition_start <= raw_offset < definition_end
        else None
    )
    candidates = {value for value in (local_offset, section_offset) if value is not None}
    if len(candidates) != 1:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "candidate instruction offset is outside or ambiguous within the "
            "selected caller definition"
        )
    return candidates.pop()


def _candidate_static_invocation_indices(
    candidate: CandidateAssembly,
    *,
    caller_start: str,
    caller_end_exclusive: str,
) -> tuple[int, ...]:
    """Count invocation sites without assigning unrelated indirect provenance."""
    start = address_value(caller_start)
    end = address_value(caller_end_exclusive)
    instructions = candidate.instructions
    instruction_addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="cod",
        caller_start=start,
    )
    address_counts: dict[int, int] = {}
    for address in instruction_addresses:
        if address is not None and start <= address < end:
            address_counts[address] = address_counts.get(address, 0) + 1
    instruction_index_by_address = {
        address: index
        for index, address in enumerate(instruction_addresses)
        if (
            address is not None
            and start <= address < end
            and address_counts.get(address) == 1
        )
    }
    result: list[int] = []
    for index, instruction in enumerate(instructions):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic == "call":
            result.append(index)
            continue
        if mnemonic != "jmp" or index in candidate.local_control_flow_indices:
            continue
        exact_branch = _cc_cfg._exact_local_direct_branch(
            instruction,
            instruction_index=index,
            instruction_addresses=instruction_addresses,
            instruction_index_by_address=instruction_index_by_address,
            source="cod",
            caller_start=start,
            caller_end=end,
        )
        if exact_branch is not None:
            continue
        operand = _cc_cfg._instruction_operand(instruction)
        address_matches = _cc_catalog.ADDRESS_RE.findall(operand)
        if (
            address_matches
            and start <= address_value(address_matches[-1]) < end
        ):
            continue
        if (
            not address_matches
            and (
                operand.startswith("$")
                or operand.lower().startswith("short ")
                or _cc_catalog.DECORATED_RE.search(operand) is None
            )
        ):
            continue
        result.append(index)
    return tuple(result)


def _known_authored_direct_target_divergence(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    indexes: IdentityIndexes,
) -> dict[str, Any] | None:
    """Reject a known physical target mismatch before legacy projections.

    This is a rejection-only check, not an alternate acceptance route. Compare
    aligned static populations and exact zero-addend E8 COFF relocations whose
    names already have retail-derived authored identities. Unknown targets,
    provider/helper expansions and malformed listings still require the normal
    fail-closed extraction. Only accepted physical/ICF identities can unify two
    names; a caller-specific candidate snapshot cannot do so.
    """
    definition = candidate.caller_definition
    if definition is None:
        return None
    sites = _candidate_static_invocation_indices(
        candidate, caller_start="0x0",
        caller_end_exclusive=hex(max(1, len(definition.data))),
    )
    if len(sites) != len(expected):
        return None
    offsets = _candidate_complete_instruction_offsets(candidate)
    authored = {
        identity for identity in indexes.by_address.values()
        if isinstance(identity, str) and identity.startswith("symbol:")
        and identity not in indexes.provider_ids
    }

    def physical(identity: str) -> str:
        identity = indexes.reviewed_authored_icf_physical_by_logical_identity.get(
            identity, identity,
        )
        return indexes.reviewed_icf_group_by_logical_identity.get(identity, identity)

    for ordinal, instruction_index in enumerate(sites):
        row = expected[ordinal]
        instruction = candidate.instructions[instruction_index]
        name = _cc_cfg._instruction_operand(instruction).strip()
        actual = indexes.by_candidate_name.get(name)
        required = row.get("target_identity")
        if (
            row.get("form") != "call" or row.get("dispatch") != "direct"
            or row.get("identity_kind") != "direct"
            or required not in authored or actual not in authored
            or _cc_cfg._instruction_mnemonic(instruction) != "call"
            or physical(actual) == physical(required)
        ):
            continue
        offset = offsets[instruction_index]
        if offset is None or offset < 0 or offset + 5 > len(definition.data):
            continue
        encoded = bytes(int(value, 16) for value in instruction.bytes)
        relocations = [
            relocation for relocation in definition.relocations
            if relocation.offset == offset + 1
        ]
        if (
            encoded != b"\xe8\0\0\0\0"
            or definition.data[offset:offset + 5] != encoded
            or len(relocations) != 1
            or relocations[0].type != IMAGE_REL_I386_REL32
            or relocations[0].symbol_name != name
        ):
            continue
        return {
            "kind": "verifier-blocked", "side": "candidate", "ordinal": ordinal,
            "message": "Known authored target mismatch before complete candidate extraction",
            "reason": "known-authored-direct-target-before-projection",
            "candidate_call_offset": offset,
            "candidate_symbol": name,
            "expected": {"target_identity": required},
            "candidate": {"target_identity": actual},
        }
    return None


def _target_authorizes_final_build_coff_aliases(
    target: Any,
    *,
    specs: Sequence[CoffAliasSource],
) -> bool:
    context = str(getattr(target, "compile_context_from", "") or "")
    if context:
        context_path = Path(context)
        if not context_path.is_absolute():
            context_path = REPO_ROOT / context_path
        if context_path.resolve() == DEFAULT_FINAL_BUILD_MANIFEST.resolve():
            return True
    manifest_path = getattr(target, "manifest_path", None)
    if not isinstance(manifest_path, Path) or not manifest_path.is_file():
        return False
    raw = json.loads(manifest_path.read_text(encoding="utf-8"))
    if not isinstance(raw, dict):
        return False
    return raw.get("coff_alias_sources") == _cc_identity._coff_alias_rows_for_comparison(specs)
