"""Reviewed VC5 x86 native EH relocation roles, without compiler label ordinals.

This binds references from one authored body to an already classified EH helper.
It does not accept the provider's bytes, storage, layout, or original source model.
An existing provider boundary must be accepted and exclusive; missing ownership stays missing.
The absolute runtime symbol has no image extent and is never catalogued as data.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import struct
import sys

from _recoil.commands.asm_verify import CoffObject
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressError, ProgressStore, address_value
from _recoil.lib.tooling import DEFAULT_VC5_ROOT, REPO_ROOT, configure_stdio


FIELD = "native_eh_relocation_binding"
SCHEMA = "recoil-vc5-native-eh-relocations-v1"
HANDLER = "vc5-associated-eh-handler"
ABSOLUTE = "vc5-runtime-exception-list"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def runtime_absolute_proof():
    """Reopen the canonical runtime; a saved proof never substitutes for it."""
    from _recoil.commands.provider_function_mutation import parse_archive_members

    library = DEFAULT_VC5_ROOT / "VC/LIB/MSVCRT.LIB"
    definitions = []
    for member in parse_archive_members(library.read_bytes()):
        if b"__except_list" not in member.data:
            continue
        obj = CoffObject.from_bytes(member.data)
        definitions.extend((member.name, symbol) for symbol in obj.symbols
                           if symbol.name == "__except_list" and symbol.section_number != 0)
    require(len(definitions) == 1, "canonical runtime absolute definition is missing or ambiguous")
    member, symbol = definitions[0]
    require(member == "build/intel/dll_obj/dllsupp.obj" and symbol.section_number == -1
            and symbol.storage_class == 2 and symbol.type == 0 and symbol.value == 0
            and symbol.aux_count == 0, "canonical __except_list absolute definition drifted")
    return dict(library="VC/LIB/MSVCRT.LIB", member=member, symbol="__except_list",
                section_number=-1, storage_class=2, symbol_type=0, value=0)


def frame_handler_import(image, address):
    from _recoil.commands.live_byte_verify import _pe_bytes
    from _recoil.commands.provider_target_mutation import _retail_import_targets

    thunk = _pe_bytes(image, address, 6)
    require(thunk[:2] == b"\xff\x25", "EH dispatcher is not an IAT jump thunk")
    iat = struct.unpack_from("<I", thunk, 2)[0]
    imports, _ = _retail_import_targets(image)
    matches = [item for item in imports if address_value(item.address) == iat]
    require(len(matches) == 1 and matches[0].dll.casefold() == "msvcrt.dll"
            and matches[0].import_name == "__CxxFrameHandler"
            and matches[0].import_ordinal is None, "EH dispatcher has the wrong DLL/import identity")
    return iat


def prologue_offsets(body):
    """VC5 schedules the exception-list load on either side of the two pushes."""
    if (body[:3] == b"\x6a\xff\x68" and body[7:9] == b"\x64\xa1"
            and body[9:13] == bytes(4) and body[13:17] == b"\x50\x64\x89\x25"
            and body[17:21] == bytes(4)):
        return 3, 9
    if (body[:6] == b"\x64\xa1"+bytes(4) and body[6:9] == b"\x6a\xff\x68"
            and body[13:17] == b"\x50\x64\x89\x25" and body[17:21] == bytes(4)):
        return 9, 2
    raise ValueError("body lacks a supported native VC5 EH prologue")


def retail_roles(reference, row):
    from _recoil.commands.live_byte_verify import _pe_bytes
    from _recoil.commands.relocation_expectations import decode_x86_operand_sites

    start, end = address_value(row["address"]), address_value(row["end_exclusive"])
    body = _pe_bytes(reference, start, end-start)
    handler_offset, load_offset = prologue_offsets(body)
    handler = struct.unpack_from("<I", body, handler_offset)[0]
    code = _pe_bytes(reference, handler, 10)
    require(code[0] == 0xb8 and code[5] == 0xe9, "retail EH handler has an unsupported shape")
    info = struct.unpack_from("<I", code, 1)[0]
    dispatcher = handler + 10 + struct.unpack_from("<i", code, 6)[0]
    iat = frame_handler_import(reference, dispatcher)
    fields = struct.unpack("<8I", _pe_bytes(reference, info, 32))
    require(fields[0] == 0x19930520 and 0 < fields[1] < 4096
            and fields[2] != 0 and fields[3:] == (0, 0, 0, 0, 0),
            "retail EH FuncInfo is not a supported unwind-only VC5 record")
    sites, unresolved = decode_x86_operand_sites(body, function_address=start)
    require(not unresolved, "retail EH parent operand decoding is incomplete")
    offsets = []
    for site in sites:
        prefix = body[site.instruction_offset:site.offset]
        fs_memory = (prefix in (b"\x64\xa1", b"\x64\xa3") or
                     (len(prefix) == 3 and prefix[:2] in (b"\x64\x89", b"\x64\x8b")
                      and prefix[2] & 0xc7 == 5))
        if fs_memory and body[site.offset:site.offset+4] == bytes(4) and site.relocation_type == 6:
            offsets.append(site.offset)
    require(load_offset in offsets and 17 in offsets and len(offsets) >= 3,
            "retail EH exception-chain installation/restoration is incomplete")
    return dict(handler=handler, handler_offset=handler_offset, func_info=info, max_state=fields[1],
                dispatcher=dispatcher, dispatcher_iat=iat, exception_list_offsets=offsets)


def binding_context(document, bindings, source_id, object_symbol, owner_id, evidence_ids, reference):
    from _recoil.commands.relocation_expectations import build_object_binding_snapshot

    row = document.collection("symbols").get(source_id, {})
    require(row.get("binary") == "recoil" and row.get("pipeline_class") in {"authored", "authored-lifecycle"}
            and row.get("kind") == "function", "native EH binding requires an authored Recoil function")
    facts = retail_roles(reference, row)
    target_id = f"recoil:function:0x{facts['handler']:x}"
    target = document.collection("symbols").get(target_id, {})
    require(target.get("pipeline_class") == "non-authored" and target.get("kind") == "function"
            and target.get("output_section_id") == "recoil:section:.text"
            and address_value(target["address"]) == facts["handler"]
            and address_value(target["end_exclusive"]) >= facts["handler"]+10,
            "native EH target is not an existing typed non-authored handler")
    owners = [key for key, value in document.collection("owners").items()
              if any(r.get("kind") == "primary-function" and (r.get("symbol_id") == target_id
                     or r.get("address") == f"0x{facts['handler']:x}") for r in value.get("relationships", []))]
    pending_lifecycle = {}
    if owner_id is None:
        require(not owners and target.get("ownership_state") in {None, "unresolved"}
                and target.get("authored_order_role") == "compiler-generated-eh-helper",
                "unowned native EH binding requires an already classified helper with no primary owner")
        # The existing non-authored classification plus the exact retail handler
        # role is sufficient for a reference binding. Preserve missing ownership;
        # do not invent an owner or promote provider gates to name a local label.
        relationships = [None]
        allowed_evidence = set(row.get("evidence_ids", []))
    else:
        owner = document.collection("owners").get(owner_id, {})
        lifecycle = owner.get("lifecycle_state")
        boundary = owner.get("gates", {}).get("boundary")
        require(owners == [owner_id] and owner.get("kind") == "provider-boundary"
                and owner.get("provider_state") == "accepted"
                and (lifecycle == "accepted" or lifecycle == "discovered" and boundary == "accepted"),
                "native EH provider boundary is not accepted or exclusive")
        if lifecycle != "accepted":
            # A dependency binding does not finish the packet owner's lifecycle.
            # Retain this pending state so later lifecycle changes require review.
            pending_lifecycle = dict(provider_lifecycle=dict(state=lifecycle, boundary=boundary))
        relationships = [r for r in owner.get("relationships", []) if r.get("symbol_id") == target_id
                         and r.get("kind") == "primary-function" and address_value(r["address"]) == facts["handler"]]
        require(len(relationships) == 1, "native EH handler lacks one exact provider relationship")
        allowed_evidence = set(owner.get("evidence_ids", []))
    require(isinstance(evidence_ids, list) and evidence_ids and len(evidence_ids) == len(set(evidence_ids))
            and set(evidence_ids) <= allowed_evidence
            and all(e in document.collection("evidence") for e in evidence_ids),
            "native EH binding requires existing provider evidence, or source evidence for an unowned helper")
    return dict(source=build_object_binding_snapshot(document, bindings, symbol_id=source_id,
                    object_symbol=object_symbol), retail=facts, runtime=runtime_absolute_proof(),
                target={k: target.get(k) for k in ("address", "end_exclusive", "kind", "pipeline_class", "output_section_id")},
                target_id=target_id, owner_id=owner_id, relationship=relationships[0],
                evidence_ids=evidence_ids, **pending_lifecycle)


def derive_native_expectations(document, bindings, row, object_symbol, reference):
    binding = row.get(FIELD)
    if binding is None:
        return {}
    require(isinstance(binding, dict) and set(binding) == {"schema", "reviewed", "reason", "context"}
            and binding["schema"] == SCHEMA and binding["reviewed"] is True
            and isinstance(binding["reason"], str) and binding["reason"].strip(), "invalid reviewed native EH binding")
    saved = binding["context"]
    parent_symbol = saved["source"]["object_symbol"]
    current = binding_context(document, bindings, saved["source"]["symbol_id"], parent_symbol,
                              saved["owner_id"], saved["evidence_ids"], reference)
    require(current == saved and row["address"] == current["source"]["address"], "reviewed native EH binding is stale")
    if object_symbol != parent_symbol:
        import re
        from _recoil.commands.byte_symbol_selectors import PREFIX
        from _recoil.commands.relocation_expectations import build_object_binding_snapshot
        require(object_symbol.startswith(PREFIX) and re.fullmatch(object_symbol[len(PREFIX):], parent_symbol),
                "reviewed native EH binding is stale: source selector does not contain the reviewed parent identity")
        build_object_binding_snapshot(document, bindings, symbol_id=saved["source"]["symbol_id"],
                                      object_symbol=object_symbol)
    facts = current["retail"]
    result = {}
    for offset, role, symbol, target in [(facts["handler_offset"], HANDLER, "@vc5-eh-handler:"+saved["source"]["symbol_id"], facts["handler"])] + [
            (offset, ABSOLUTE, "__except_list", 0) for offset in facts["exception_list_offsets"]]:
        result[(offset, 6)] = dict(object_symbol=object_symbol, offset=offset, type=6, type_name="DIR32",
                target_symbol=symbol, coff_addend=0, resolved_target_addend=0, retail_target=target,
                derivation="reviewed-native-eh-role", native_eh_role=role,
                target_symbol_id=current["target_id"] if role == HANDLER else "vc5:absolute:__except_list",
                native_eh_max_state=facts["max_state"], native_eh_parent_symbol=parent_symbol)
    return result


def associated_section(obj, section_index, parent_index, name):
    section = obj.section(section_index)
    definitions = [s for s in obj.symbols if s.section_number == section_index and s.name == section.name
                   and s.storage_class == 3 and s.aux_count == 1]
    require(section.name == name and section.characteristics & 0x1000 and len(definitions) == 1
            and definitions[0].section_definition_selection == 5
            and definitions[0].section_definition_association == parent_index,
            "EH section does not have one associative COMDAT definition for the selected parent")
    return section


def prove_object_handler(obj, body, relocation, expected):
    """Prove a generated role; no raw label spelling is part of expected truth."""
    if "native_eh_parent_symbol" in expected:
        require(body.symbol == expected["native_eh_parent_symbol"],
                "native EH candidate is not the exact reviewed parent function")
    offset, _ = prologue_offsets(body.data)
    require(relocation.type == 6 and relocation.offset-body.start == offset == expected["offset"]
            and body.data[offset:offset+4] == bytes(4),
            "candidate native EH handler reference has wrong form/addend")
    target = obj.symbols_by_index[relocation.symbol_index]
    require(target.name == relocation.symbol_name and target.name.startswith("$L")
            and target.storage_class == 6 and target.type == 0 and target.section_number > 0,
            "candidate EH target is not a native code label")
    section = associated_section(obj, target.section_number, body.section_index, ".text$x")
    require(0 <= target.value and target.value+10 == len(section.raw_data), "native EH handler is not the final ten-byte thunk")
    require(section.raw_data[target.value:] == b"\xb8"+bytes(4)+b"\xe9"+bytes(4), "candidate EH handler opcode/addend drift")
    relocs = [r for r in obj.relocations_by_section.get(section.index, ()) if r.offset >= target.value]
    require([(r.offset-target.value, r.type) for r in relocs] == [(1, 6), (6, 20)]
            and relocs[1].symbol_name == "___CxxFrameHandler", "candidate EH handler relocation population drift")
    info = obj.symbols_by_index[relocs[0].symbol_index]
    require(info.name == relocs[0].symbol_name and info.storage_class == 3 and info.type == 0
            and info.value == 0 and info.section_number > 0, "candidate FuncInfo symbol is invalid")
    data = associated_section(obj, info.section_number, body.section_index, ".xdata$x")
    require(len(data.raw_data) >= 32, "candidate FuncInfo is truncated")
    fields = struct.unpack_from("<8I", data.raw_data)
    require(fields == (0x19930520, expected["native_eh_max_state"], 0, 0, 0, 0, 0, 0)
            and len(data.raw_data) == 32+fields[1]*8, "candidate FuncInfo shape differs from retail EH role")
    return target


def prove_linked_packet(obj, body, target, candidate_target, parsed_map, image):
    """Walk the fresh associated packet, checking bytes and every relocation.

    The parent relocation locates the packet. Association proves its source role;
    a complete section graph and external MAP identities prove its linked content.
    Disagreement, escaping local sections, duplicate sites, and overlaps fail closed.
    """
    from _recoil.commands.live_byte_verify import _pe_bytes

    bases = {target.section_number: candidate_target-target.value}
    pending = [target.section_number]
    checked = set()
    while pending:
        index = pending.pop()
        if index in checked:
            continue
        checked.add(index)
        section = obj.section(index)
        require(section.name in {".text$x", ".xdata$x"}, "unsupported EH packet section")
        associated_section(obj, index, body.section_index, section.name)
        linked = _pe_bytes(image, bases[index], len(section.raw_data))
        mask = bytearray(len(linked))
        for reloc in obj.relocations_by_section.get(index, ()):
            offset = reloc.offset
            require(reloc.type in {6, 20} and 0 <= offset <= len(linked)-4
                    and not any(mask[offset:offset+4]), "EH packet has invalid/overlapping relocations")
            mask[offset:offset+4] = b"\x01"*4
            raw = struct.unpack_from("<i", section.raw_data, offset)[0]
            actual = struct.unpack_from("<I", linked, offset)[0] if reloc.type == 6 else (
                bases[index]+offset+4+struct.unpack_from("<i", linked, offset)[0])
            symbol = obj.symbols_by_index[reloc.symbol_index]
            require(symbol.name == reloc.symbol_name, "EH packet relocation symbol is inconsistent")
            if symbol.section_number > 0 and symbol.storage_class in {3, 6}:
                target_section = obj.section(symbol.section_number)
                require(0 <= symbol.value < len(target_section.raw_data), "EH packet local target is outside its section")
                associated_section(obj, symbol.section_number, body.section_index, target_section.name)
                base = actual-symbol.value-raw
                require(symbol.section_number not in bases or bases[symbol.section_number] == base,
                        "EH packet local relocation addresses disagree")
                bases[symbol.section_number] = base
                pending.append(symbol.section_number)
            else:
                require(symbol.storage_class == 2, "EH packet target has unsupported storage class")
                addresses = [int(m.address) for m in parsed_map.symbols if m.symbol == symbol.name]
                require(len(addresses) == 1 and actual == addresses[0]+raw,
                        "EH packet external relocation has missing/ambiguous/wrong linked identity: "+symbol.name)
                if symbol.name == "___CxxFrameHandler":
                    frame_handler_import(image, actual)
        require(all(mask[i] or linked[i] == value for i, value in enumerate(section.raw_data)),
                "linked EH packet differs outside COFF relocations")
    intervals = sorted((bases[i], bases[i]+len(obj.section(i).raw_data)) for i in checked)
    require(all(left[1] <= right[0] for left, right in zip(intervals, intervals[1:])), "linked EH packet sections overlap")


def main(argv=None):
    configure_stdio()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--payload-file", type=Path, required=True)
    parser.add_argument("--expected-revision", type=int, required=True)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        from _recoil.commands.live_byte_verify import _bindings, DEFAULT_MANIFEST_DIR, DEFAULT_REFERENCE
        payload = json.loads(args.payload_file.read_text(encoding="utf-8-sig"))
        require(set(payload) == {"reviewed", "source_symbol_id", "object_symbol", "provider_owner_id", "evidence_ids", "reason"}
                and payload["reviewed"] is True and isinstance(payload["reason"], str) and payload["reason"].strip(),
                "native EH binding requires the exact reviewed source/provider payload")
        store = ProgressStore(DEFAULT_PROGRESS_PATH)
        document = store.load()
        require(document.revision == args.expected_revision, "tracker revision changed")
        bindings = _bindings(document, DEFAULT_MANIFEST_DIR)
        context = binding_context(document, bindings, payload["source_symbol_id"], payload["object_symbol"],
                                  payload["provider_owner_id"], payload["evidence_ids"], DEFAULT_REFERENCE)
        proposed = deepcopy(document.data)
        row = proposed["symbols"][payload["source_symbol_id"]]
        require(FIELD not in row, "native EH binding already exists; do not overwrite reviewed facts")
        row[FIELD] = dict(schema=SCHEMA, reviewed=True, reason=payload["reason"], context=context)
        commit = store.commit(proposed, expected_revision=args.expected_revision, apply=args.apply)
        print(json.dumps(dict(kind="native-eh-relocation-binding", binding=row[FIELD],
                              accepted_provider_bytes=False, commit=commit.to_dict()), indent=2))
        return 0
    except (OSError, ValueError, KeyError, TypeError, ProgressError) as exc:
        print(f"native EH relocation binding error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
