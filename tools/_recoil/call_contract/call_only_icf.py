"""Fresh local-code proof for the deliberately narrow call-only ICF route."""
from __future__ import annotations

from pathlib import Path
import struct

from _recoil.lib.call_only_icf import require
from _recoil.lib.progress import address_value, normalize_address
from _recoil.lib.tooling import REPO_ROOT


def prove_call(contract, *, source, caller_identity, caller_start, instructions, index,
               instruction_addresses, caller_definition, callee_definitions,
               local_control_flow_indices=frozenset(), local_control_flow_targets=None):
    """Prove current ABI shape without recording body or owner acceptance.

    V1 conservatively requires the caller's complete retail instruction shape
    and relocation positions. This is stronger than ordinary call-contract
    eligibility; source variants with changed shape need a separate ABI proof.
    The surrounding call verifier still owns every invocation target and order.
    """
    from _recoil.commands.live_byte_verify import _pe_bytes, DEFAULT_REFERENCE
    from _recoil.commands.relocation_expectations import decode_x86_operand_sites
    from _recoil.call_contract import cfg, receiver_candidate, current_callees
    require(caller_identity == "symbol:" + contract["caller_id"]
            and caller_start == address_value(contract["caller_address"]), "wrong live caller")
    site = address_value(contract["call_address"])
    offset = site - caller_start
    size = address_value(contract["caller_end_exclusive"]) - caller_start
    retail = _pe_bytes(DEFAULT_REFERENCE, caller_start, size)
    require(retail[offset] == 0xe8 and site + 5 + struct.unpack_from('<i', retail, offset + 1)[0]
            == address_value(contract["physical_address"]), "retail direct target differs")
    row = instructions[index]
    require(cfg._instruction_mnemonic(row) == "call" and len(row.bytes) == 5
            and row.bytes[0].lower() == "e8"
            and instruction_addresses[index] == site, "call position/form changed")
    # The reviewed void register-argument arm does no stack cleanup. Return use
    # and ECX value flow are retained by the full current instruction-shape proof.
    prove_no_cleanup(instructions, index=index, source=source, addresses=instruction_addresses,
        caller_start=caller_start, caller_end=caller_start + size,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=local_control_flow_targets)
    if source == "bn":
        require(bytes.fromhex(" ".join(row.bytes)) == retail[offset:offset + 5],
                "BN call does not agree with immutable retail")
        return True
    require(source == "cod" and caller_definition is not None,
            "fresh candidate COFF caller required")
    caller = caller_definition
    require(caller.symbol == contract["caller_object_symbol"] and len(caller.data) == size
            and receiver_candidate._candidate_listing_matches_coff(instructions,
                addresses=instruction_addresses, caller_start=caller_start, definition=caller),
            "current caller definition/listing or complete extent differs")
    sites, unresolved = decode_x86_operand_sites(retail, function_address=caller_start)
    require(not unresolved, "retail caller operand decoding is incomplete")
    expected = expected_operand_positions(retail, sites, caller_start=caller_start)
    actual = [(item.offset, item.type) for item in caller.relocations]
    require(len(actual) == len(set(actual)) and set(actual) == expected,
            "caller relocation positions/types changed")
    mask = [False] * size
    for at, _kind in expected:
        require(0 <= at <= size - 4, "operand outside caller")
        mask[at:at + 4] = [True] * 4
    require(tuple(mask) == tuple(caller.relocation_mask)
            and all(mask[at] or left == right for at, (left, right) in enumerate(zip(retail, caller.data))),
            "caller instruction/ABI shape changed; independent proof required")
    for reference in caller.relocations:
        at = reference.offset
        if reference.type != 6:
            continue
        retail_target = struct.unpack_from('<I', retail, at)[0]
        if caller_start <= retail_target < caller_start + size:
            local = [item for item in caller.coff_symbols if item.index == reference.symbol_index]
            require(len(local) == 1 and local[0].section_number == caller.section_index
                    and local[0].value + struct.unpack_from('<I', caller.data, at)[0]
                    - caller.section_start == retail_target - caller_start,
                    "local switch/control-flow relocation changed")
    name = contract["object_symbol"]
    references = [item for item in caller.relocations if offset <= item.offset < offset + 5]
    symbols = [item for item in caller.coff_symbols if item.name.casefold() == name.casefold()]
    require(len(references) == len(symbols) == 1 and symbols[0].name == name
            and symbols[0].storage_class == 2 and symbols[0].symbol_type == 0x20
            and symbols[0].section_number > 0 and symbols[0].value == 0
            and references[0].offset == offset + 1 and references[0].type == 20
            and references[0].symbol_name == name and references[0].symbol_index == symbols[0].index
            and caller.data[offset + 1:offset + 5] == bytes(4),
            "call must resolve to one exact local external authored definition")
    definition = (callee_definitions or {}).get(name)
    require(definition is not None and definition.symbol == name
            and caller.object_path and definition.object_path == caller.object_path
            and definition.symbol_index == references[0].symbol_index == symbols[0].index
            and definition.section_number == symbols[0].section_number
            and definition.symbol_value == symbols[0].value
            and definition.storage_class == symbols[0].storage_class
            and definition.symbol_type == symbols[0].symbol_type
            and definition.section_external_functions == (name,)
            and definition.data == symbols[0].section_data
            and not definition.relocations
            and current_callees.exact_callee_return_cleanup(definition) == 0,
            "current local target definition/return ABI is missing or ambiguous")
    # This v1 route binds the independently observed empty logical callback.
    require(definition.data[:1] == b'\xc3' and len(definition.data) <= 16
            and all(value in {0x90, 0xcc} for value in definition.data[1:]),
            "target is not the reviewed empty local callback")
    path = contract["source_traceability"]["source_edges"][0]["emission_context"]["translation_unit"]
    require(definition.current_source_path == path,
            "candidate target is emitted from an unexpected source")
    return True


def expected_operand_positions(retail, sites, *, caller_start):
    """Distinguish address relocations from immediates and local branch encoding."""
    from _recoil.lib.pe import parse_pe_headers
    from _recoil.commands.live_byte_verify import DEFAULT_REFERENCE
    headers = parse_pe_headers(DEFAULT_REFERENCE.read_bytes(), source=str(DEFAULT_REFERENCE))
    result = set()
    for item in sites:
        if item.relocation_type == 20:
            target = caller_start + item.offset + 4 + struct.unpack_from('<i', retail, item.offset)[0]
            if caller_start <= target < caller_start + len(retail) and item.opcode != 'e8':
                continue  # Preserve these bytes exactly in the instruction-shape check.
        elif item.kind == 'potential-absolute32':
            target = struct.unpack_from('<I', retail, item.offset)[0]
            if not headers.image_base <= target < headers.image_base + headers.size_of_image:
                continue  # Non-address constants remain exact, never masked.
        result.add((item.offset, item.relocation_type))
    return result


def prove_no_cleanup(instructions, *, index, source, addresses, caller_start, caller_end,
                     local_control_flow_indices=frozenset(), local_control_flow_targets=None):
    """Prove zero caller cleanup on every path up to the next invocation/RET."""
    from _recoil.call_contract import cfg
    successors, unresolved = cfg._exact_invocation_cfg(instructions,
        instruction_addresses=addresses,
        instruction_index_by_address={at: i for i, at in enumerate(addresses) if at is not None},
        source=source, caller_start=caller_start, caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=local_control_flow_targets or {})
    todo = list(successors.get(index, ()))
    require(todo, "no post-call control-flow proof")
    seen = set()
    while todo:
        at = todo.pop()
        if at in seen:
            continue
        seen.add(at)
        require(at not in unresolved, "post-call control flow is unknown")
        row = instructions[at]
        mnemonic = cfg._instruction_mnemonic(row)
        if mnemonic == 'call' or mnemonic in {'ret', 'retn'}:
            continue
        operands = [part.strip().lower() for part in cfg._instruction_operand(row).split(',')]
        require((mnemonic in {'mov', 'lea', 'cmp', 'test', 'nop'} or mnemonic.startswith('j'))
                and (not operands or operands[0] not in {'esp', 'sp'}),
                "post-call stack effect is not independently proved zero")
        require(successors.get(at), "post-call path has an unknown terminal")
        todo.extend(successors[at])
