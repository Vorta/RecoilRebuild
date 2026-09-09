"""Scoped physical identity for a native implicit array cleanup callback.

This proves one native array dependency, not an original destructor alias census,
source owner, ICF winner, or authored-body classification.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import re
import struct
import sys

from _recoil.commands.native_eh_relocations import require
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressError, ProgressStore, address_value
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


FIELD = "native_array_cleanup_bindings"
SCHEMA = "recoil-native-array-cleanup-v1"
ROLE = "native-array-implicit-cleanup"


def _u32(data, offset=0):
    return struct.unpack_from("<I", data, offset)[0]


def retail_selector(reference, row, offset):
    from _recoil.commands.live_byte_verify import _pe_bytes

    start, end = address_value(row["address"]), address_value(row["end_exclusive"])
    require(type(offset) is int and 1 <= offset <= end-start-25, "cleanup selector is outside the parent")
    data = _pe_bytes(reference, start+offset-1, 26)
    if (data[0] == 0x68 and data[5] == 0x6a and data[7:9] == b"\x8d\x86"
            and data[13] == 0x6a and data[15:19] == b"\x50\xc6\x44\x24"
            and data[21] == 0xe8 and 0 < data[6] < 128 and 0 < data[14] < 128):
        return dict(kind="destruction", offset=offset, target=_u32(data, 1),
                    count=data[6], stride=data[14], member_offset=_u32(data, 9),
                    helper=start+offset+25+struct.unpack_from("<i", data, 22)[0])
    require(offset <= end-start-30, "construction selector is outside the parent")
    data = _pe_bytes(reference, start+offset-1, 31)
    require(data[0] == 0x68 and data[5] == 0x68 and data[10] == 0x6a
            and data[12:15] == b"\xc6\x44\x24" and data[17:19] == b"\x8d\x86"
            and data[23] == 0x6a and data[25:27] == b"\x50\xe8"
            and 0 < data[11] < 128 and 0 < data[24] < 128,
            "retail selector is not a supported native array sequence")
    return dict(offset=offset, target=_u32(data, 1), constructor=_u32(data, 6),
                count=data[11], stride=data[24], member_offset=_u32(data, 19),
                helper=start+offset+30+struct.unpack_from("<i", data, 27)[0])


def prove_destructor_helper_abi(reference, helper):
    from _recoil.commands.live_byte_verify import _pe_bytes

    code = _pe_bytes(reference, helper, 115)
    require(code[:3] == b"\x55\x8b\xec" and code[40:77] == bytes.fromhex(
            "8b4d10 8b7d0c 0fafcf 8b7508 03f1 897508 8945fc "
            "ff4d10 780c 2bf7 897508 8bce ff5514 ebef")
            and code[91] == 0xe8 and code[112:] == b"\xc2\x10\x00",
            "retail array-destructor parameter/iteration ABI drift")
    cleanup = helper+96+struct.unpack_from("<i", code, 92)[0]
    code = _pe_bytes(reference, cleanup, 23)
    require(code[:18] == bytes.fromhex("8b45e4 85c0 750f 8b5514 52 8b4510 50 57 56 e8")
            and code[22] == 0xc3, "retail array destruction forwarding ABI drift")
    checker = cleanup+22+struct.unpack_from("<i", code, 18)[0]
    code = _pe_bytes(reference, checker, 106)
    require(code[:3] == b"\x55\x8b\xec" and code[45:64] == bytes.fromhex(
            "ff4d10 781e 8b4d08 2b4d0c 894d08 ff5514 ebed")
            and code[103:] == b"\xc2\x10\x00", "retail cleanup callback ECX/stack ABI drift")
    return dict(destructor=helper, cleanup=cleanup, checker=checker,
                callback_receiver="complete-element-in-ecx", callback_stack_arguments=0)


def prove_helper_abi(reference, helper):
    """Read the retail iterator and its cleanup chain, including ECX and RET ABI."""
    from _recoil.commands.live_byte_verify import _pe_bytes

    code = _pe_bytes(reference, helper, 111)
    require(code[:3] == b"\x55\x8b\xec" and code[46:73] == bytes.fromhex(
            "8b5d0c 8b7d08 3b7510 7d10 8bcf ff5514 03fb 897d08 46 8975e4 ebeb")
            and code[87] == 0xe8 and code[108:] == b"\xc2\x14\x00",
            "retail array-constructor parameter/iteration ABI drift")
    cleanup = helper+92+struct.unpack_from("<i", code, 88)[0]
    code = _pe_bytes(reference, cleanup, 20)
    require(code[:15] == bytes.fromhex("8b45e0 85c0 750c 8b4518 50 56 53 57 e8")
            and code[19] == 0xc3, "retail array cleanup forwarding ABI drift")
    checker = cleanup+19+struct.unpack_from("<i", code, 15)[0]
    code = _pe_bytes(reference, checker, 106)
    require(code[:3] == b"\x55\x8b\xec" and code[45:64] == bytes.fromhex(
            "ff4d10 781e 8b4d08 2b4d0c 894d08 ff5514 ebed")
            and code[103:] == b"\xc2\x10\x00", "retail cleanup callback ECX/stack ABI drift")
    return dict(constructor=helper, cleanup=cleanup, checker=checker,
                callback_receiver="complete-element-in-ecx", callback_stack_arguments=0)


def source_generation(source_path, parent_symbol, constructor_symbol, table_symbol, count):
    from _recoil.lib.source_emission_markers import collect_source_closure
    from _recoil.lib.source_constructs import matching_brace, mask_comments_and_literals

    parent_match = re.fullmatch(r"\?\?0([A-Za-z_][A-Za-z_0-9]*)@@QAE@.*", parent_symbol)
    if parent_match is None:
        parent_match = re.fullmatch(r"\?\?1([A-Za-z_][A-Za-z_0-9]*)@@UAE@XZ", parent_symbol)
    element_match = re.fullmatch(r"\?\?0([A-Za-z_][A-Za-z_0-9]*)@@QAE@XZ", constructor_symbol)
    table_match = re.fullmatch(r"\?\?_7([A-Za-z_][A-Za-z_0-9]*)@@6B@", table_symbol)
    require(parent_match and element_match and table_match, "native array class identities are unsupported")
    parent, element, base = parent_match[1], element_match[1], table_match[1]
    types = {}
    for file in collect_source_closure(source_path, repo_root=REPO_ROOT):
        masked = mask_comments_and_literals(file.text)
        for match in re.finditer(r"\b(?:struct|class)\s+([A-Za-z_]\w*)\s*(?:\:[^;{}]+)?\{", masked):
            end = matching_brace(masked, match.end()-1)
            types.setdefault(match[1], []).append((file.repo_path, masked[match.start():end]))

    def class_body(name):
        matches = types.get(name, [])
        require(len(matches) == 1, "native array class definition is missing or ambiguous: "+name)
        return matches[0]

    parent_path, parent_body = class_body(parent)
    members = [m[1] for m in re.finditer(r"\b"+re.escape(element)+r"\s+([A-Za-z_]\w*)\s*\[\s*"+str(count)+r"\s*\]\s*;", parent_body)
               if parent_body[:m.start()].count("{")-parent_body[:m.start()].count("}") == 1]
    require(len(members) == 1, "native array requires one exact direct member array of the selected element type")
    hierarchy = []
    current = element
    while True:
        require(current not in [item["class_name"] for item in hierarchy], "cyclic native cleanup hierarchy")
        path, body = class_body(current)
        hierarchy.append(dict(class_name=current, path=path))
        if current == base:
            require(re.search(r"\bvirtual\s+~\s*"+re.escape(base)+r"\s*\(\s*\)\s*\{\s*\}", body),
                    "native cleanup base must retain its empty inline virtual destructor")
            break
        require(not re.search(r"~\s*"+re.escape(current)+r"\s*\(", body),
                "native array cleanup class declares an explicit destructor")
        match = re.match(r"(?:struct|class)\s+"+re.escape(current)+r"\s*:\s*(?:(?:public|protected|private)\s+)?([A-Za-z_]\w*)\s*\{", body)
        require(match, "native cleanup requires the reviewed single nonvirtual base chain")
        current = match[1]
    return dict(parent_class=parent, parent_header=parent_path, element_class=element,
                member=members[0], count=count, hierarchy=hierarchy,
                destructor_symbol=f"??1{element}@@UAE@XZ", source_path=source_path)


def paired_construction(document, bindings, object_symbol, selector, reference, state):
    """Derive element identity from a current reviewed constructor of this class."""
    parent = re.fullmatch(r"\?\?1([A-Za-z_]\w*)@@UAE@XZ", object_symbol)
    require(parent, "native array destruction requires its native parent destructor")
    matches = []
    for source_id, row in document.collection("symbols").items():
        for binding in row.get(FIELD, []):
            saved = binding["context"]
            if (saved["selector"].get("kind") is not None
                    or saved["generation"]["parent_class"] != parent[1]
                    or any(saved["selector"][key] != selector[key]
                           for key in ("target", "count", "stride", "member_offset"))):
                continue
            require(binding.get("schema") == SCHEMA and binding.get("reviewed") is True
                    and isinstance(binding.get("reason"), str) and binding["reason"].strip()
                    and saved["source"]["symbol_id"] == source_id
                    and saved["source"]["object_symbol"].startswith("??0"+parent[1]+"@@QAE@"),
                    "native destruction has an invalid reviewed construction dependency")
            current = binding_context(document, bindings, source_id, saved["source"]["object_symbol"],
                saved["selector"]["offset"], saved["owner_id"], saved["evidence_ids"], reference, state)
            require(current == saved, "native destruction's reviewed construction dependency is stale")
            matches.append(current)
    require(len(matches) == 1, "native destruction requires one unambiguous reviewed construction dependency")
    return matches[0]


def binding_context(document, bindings, source_id, object_symbol, offset, owner_id, evidence_ids, reference,
                    target_identity_state=None):
    from _recoil.commands.live_byte_verify import _pe_bytes, _binding_source_from
    from _recoil.commands.relocation_expectations import (
        build_object_binding_snapshot, build_target_identity_state, _resolve_identity,
    )
    source = document.collection("symbols").get(source_id, {})
    require(source.get("binary") == "recoil" and source.get("pipeline_class") in {"authored", "authored-lifecycle"},
            "native cleanup parent is not authored")
    selector = retail_selector(reference, source, offset)
    target_id = f"recoil:function:0x{selector['target']:x}"
    target = document.collection("symbols").get(target_id, {})
    require(target.get("pipeline_class") == "authored-lifecycle"
            and target.get("authored_order_role") == "compiler-generated-implicit-cleanup"
            and target.get("kind") == "function" and target.get("ownership_state") == "primary-owned"
            and address_value(target["address"]) == selector["target"]
            and address_value(target["end_exclusive"]) >= selector["target"]+7,
            "native cleanup target classification/extent is not the reviewed generated lifecycle shape")
    leaf = _pe_bytes(reference, selector["target"], 7)
    require(leaf[:2] == b"\xc7\x01" and leaf[6] == 0xc3, "retail cleanup is not the exact offset-zero table reset and RET")
    state = target_identity_state or build_target_identity_state(document, bindings, reference=reference)
    identities = state[0]
    destruction = selector.get("kind") == "destruction"
    pair = paired_construction(document, bindings, object_symbol, selector, reference, state) if destruction else None
    constructor = pair["selector"]["constructor"] if pair else selector["constructor"]
    resolved = {}
    for name, address in (("table", _u32(leaf, 2)), ("constructor", constructor), ("helper", selector["helper"])):
        identity, addend, _ = _resolve_identity(address, identities)
        require(identity is not None and addend == 0 and len(identity.object_symbols) == 1,
                "native cleanup dependency lacks one independent typed identity: "+name)
        resolved[name] = dict(symbol_id=identity.symbol_id, object_symbol=identity.object_symbols[0], address=address)
        require(not any(b.get("target_symbol_id") == identity.symbol_id for b in state[1]),
                "native cleanup dependency has stale target bindings: "+name)
    helper_symbol = "??_M@YGXPAXIHP6EX0@Z@Z" if destruction else "??_L@YGXPAXIHP6EX0@Z1@Z"
    require(resolved["helper"]["object_symbol"] == helper_symbol,
            "native cleanup selector does not call its accepted array helper")
    paths = {_binding_source_from(b) for b in bindings.get(source_id, ()) if b.function.symbol == object_symbol}
    require(len(paths) == 1 and bool(next(iter(paths))), "native cleanup source emission TU is ambiguous")
    generation = source_generation(next(iter(paths)), object_symbol, resolved["constructor"]["object_symbol"],
                                   resolved["table"]["object_symbol"], selector["count"])
    owner = document.collection("owners").get(owner_id, {})
    relations = [r for r in owner.get("relationships", ()) if r.get("kind") == "primary-function"
                 and r.get("symbol_id") == target_id and address_value(r["address"]) == selector["target"]]
    require(owner.get("kind") == "class" and len(relations) == 1
            and isinstance(evidence_ids, list) and evidence_ids and len(evidence_ids) == len(set(evidence_ids))
            and set(evidence_ids) <= set(owner.get("evidence_ids", ()))
            and all(e in document.collection("evidence") for e in evidence_ids),
            "native cleanup requires the existing primary class owner and scoped evidence")
    bound_symbols = sorted({name for identity in identities if identity.symbol_id == target_id for name in identity.object_symbols})
    return dict(source=build_object_binding_snapshot(document, bindings, symbol_id=source_id, object_symbol=object_symbol),
                selector=selector, dependencies=resolved, generation=generation, helper_abi=(prove_destructor_helper_abi if destruction else prove_helper_abi)(reference, selector["helper"]),
                target_id=target_id, target={k: target.get(k) for k in ("address", "end_exclusive", "kind", "pipeline_class", "authored_order_role", "ownership_state")},
                owner_id=owner_id, relationship=relations[0], evidence_ids=evidence_ids, bound_object_symbols=bound_symbols)


def derive_cleanup_expectations(document, bindings, row, object_symbol, reference, target_identity_state=None):
    result = {}
    stored = row.get(FIELD, [])
    require(isinstance(stored, list), "native cleanup bindings must be a list")
    for binding in stored:
        require(isinstance(binding, dict) and set(binding) == {"schema", "reviewed", "reason", "context"}
                and binding["schema"] == SCHEMA and binding["reviewed"] is True
                and isinstance(binding["reason"], str) and binding["reason"].strip(), "invalid reviewed native cleanup binding")
        saved = binding["context"]
        current = binding_context(document, bindings, saved["source"]["symbol_id"], object_symbol,
            saved["selector"]["offset"], saved["owner_id"], saved["evidence_ids"], reference, target_identity_state)
        require(current == saved and current["source"]["address"] == row["address"], "reviewed native cleanup binding is stale")
        key = (current["selector"]["offset"], 6)
        require(key not in result, "duplicate native cleanup selector")
        result[key] = dict(object_symbol=object_symbol, offset=key[0], type=6, type_name="DIR32",
            target_symbol="@native-cleanup:"+current["target_id"], target_symbol_id=current["target_id"],
            coff_addend=0, resolved_target_addend=0, retail_target=current["selector"]["target"],
            derivation="reviewed-native-array-cleanup", native_cleanup=current)
    return result


def prove_object_cleanup(obj, parent, relocation, expected):
    context = expected["native_cleanup"]
    symbol_name = context["generation"]["destructor_symbol"]
    require(relocation.type == 6 and relocation.offset-parent.start == expected["offset"]
            and relocation.symbol_name == symbol_name and parent.data[expected["offset"]:expected["offset"]+4] == bytes(4),
            "native cleanup parent has wrong destructor/type/offset/addend")
    symbols = [s for s in obj.symbols if s.name == symbol_name]
    require(len(symbols) == 1 and obj.symbols_by_index[relocation.symbol_index] == symbols[0]
            and symbols[0].storage_class == 2 and symbols[0].type == 0x20 and symbols[0].section_number > 0,
            "native cleanup destructor lacks one exact function definition")
    body = obj.function_bytes(symbol_name)
    definitions = [s for s in obj.symbols if s.section_number == body.section_index and s.section_definition_selection is not None]
    require(len(definitions) == 1 and definitions[0].section_definition_selection == 2
            and obj.section(body.section_index).characteristics & 0x1000,
            "native implicit destructor is not an eligible inline COMDAT")
    require(7 <= len(body.data) <= 22 and body.data[:7] == b"\xc7\x01"+bytes(4)+b"\xc3"
            and all(b in {0x90, 0xcc} for b in body.data[7:]), "native cleanup body/ABI or trailing padding differs")
    relocs = body.relocations
    require(len(relocs) == 1 and relocs[0].offset-body.start == 2 and relocs[0].type == 6
            and relocs[0].symbol_name == context["dependencies"]["table"]["object_symbol"],
            "native cleanup table relocation identity differs")
    destruction = context.get("selector", {}).get("kind") == "destruction"
    offset = expected["offset"]+(21 if destruction else 5)
    role = "helper" if destruction else "constructor"
    refs = [r for r in parent.relocations if r.offset-parent.start == offset]
    require(len(refs) == 1 and refs[0].type == (20 if destruction else 6)
            and refs[0].symbol_name == context["dependencies"][role]["object_symbol"]
            and parent.data[offset:offset+4] == bytes(4),
            "native array selects a different element constructor or destruction helper")
    return body


def prove_linked_cleanup(obj, parent, relocation, expected, parsed_map, image, candidate_target):
    from _recoil.commands.live_byte_verify import _pe_bytes
    body = prove_object_cleanup(obj, parent, relocation, expected)
    def address(name):
        matches = [int(m.address) for m in parsed_map.symbols if m.symbol == name]
        require(len(matches) == 1, "native cleanup dependency has missing or ambiguous linked identity: "+name)
        return matches[0]
    require(candidate_target == address(body.symbol), "parent cleanup operand does not resolve to its exact native destructor")
    for name in expected["native_cleanup"]["bound_object_symbols"]:
        matches = [int(m.address) for m in parsed_map.symbols if m.symbol == name]
        require(not matches or matches == [candidate_target], "native cleanup conflicts with an existing physical target binding")
    table = address(expected["native_cleanup"]["dependencies"]["table"]["object_symbol"])
    require(_pe_bytes(image, candidate_target, 7) == b"\xc7\x01"+struct.pack("<I", table)+b"\xc3",
            "linked native cleanup body/table relocation differs")
    return body.symbol


def record_physical_mapping(retail_base, candidate_base, is_cleanup, targets, cleanup_targets):
    """Keep one request's proven cleanup identity consistent in both directions."""
    strict = is_cleanup or retail_base in cleanup_targets
    if strict and targets.get(retail_base, set()) - {candidate_base}:
        return False
    if any(candidate_base in values and other != retail_base and (is_cleanup or other in cleanup_targets)
           for other, values in targets.items()):
        return False
    targets.setdefault(retail_base, set()).add(candidate_base)
    if is_cleanup:
        cleanup_targets.add(retail_base)
    return True


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
        require(set(payload) == {"reviewed", "source_symbol_id", "object_symbol", "offset", "owner_id", "evidence_ids", "reason"}
                and payload["reviewed"] is True and isinstance(payload["reason"], str) and payload["reason"].strip(),
                "native cleanup requires the exact reviewed selector payload")
        store = ProgressStore(DEFAULT_PROGRESS_PATH)
        document = store.load()
        require(document.revision == args.expected_revision, "tracker revision changed")
        context = binding_context(document, _bindings(document, DEFAULT_MANIFEST_DIR), payload["source_symbol_id"],
            payload["object_symbol"], payload["offset"], payload["owner_id"], payload["evidence_ids"], DEFAULT_REFERENCE)
        proposed = deepcopy(document.data)
        entries = proposed["symbols"][payload["source_symbol_id"]].setdefault(FIELD, [])
        require(not any(b["context"]["selector"]["offset"] == payload["offset"] for b in entries),
                "native cleanup selector already has a reviewed binding")
        binding = dict(schema=SCHEMA, reviewed=True, reason=payload["reason"], context=context)
        entries.append(binding)
        commit = store.commit(proposed, expected_revision=args.expected_revision, apply=args.apply)
        print(json.dumps(dict(kind="native-array-cleanup-binding", binding=binding,
                              accepted_alias_census=False, accepted_source_model=False, commit=commit.to_dict()), indent=2))
        return 0
    except (OSError, ValueError, KeyError, TypeError, ProgressError) as exc:
        print(f"native array cleanup binding error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
