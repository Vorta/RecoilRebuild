"""Bind an authored reference to a retail and canonical VC5 import thunk.

The source-site proof preserves the existing non-authored target, its unresolved
or explicitly reviewed accepted provider ownership, and its extent tail. It
accepts no provider body, padding, storage, or owner.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import struct
import sys

from _recoil.commands.asm_verify import CoffObject
from _recoil.commands.native_eh_relocations import require
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressError, ProgressStore, address_value
from _recoil.lib.tooling import DEFAULT_VC5_ROOT, configure_stdio

FIELD = "native_import_bindings"
SCHEMA = "recoil-native-import-reference-v1"


def import_at(image, address):
    from _recoil.commands.live_byte_verify import _pe_bytes
    from _recoil.commands.provider_target_mutation import _retail_import_targets
    body = _pe_bytes(image, address, 6)
    require(body[:2] == b"\xff\x25", "target is not an exact absolute IAT jump")
    iat = struct.unpack_from("<I", body, 2)[0]
    imports, _ = _retail_import_targets(image)
    matches = [i for i in imports if address_value(i.address) == iat]
    require(len(matches) == 1, "thunk IAT identity is missing or ambiguous")
    item = matches[0]
    return dict(dll=item.dll, name=item.import_name, ordinal=item.import_ordinal), iat


def prove_import_member(obj, object_symbol, import_name, *, import_ordinal=None,
                        descriptor_name="__IMPORT_DESCRIPTOR_MSVCRT"):
    """Prove a long-format VC5 import member's exact name or ordinal tables."""
    def symbol(name):
        found = [s for s in obj.symbols if s.name == name]
        require(len(found) == 1, "import member symbol is missing or ambiguous: " + name)
        return found[0]
    entry = symbol(object_symbol)
    require(entry.storage_class == 2 and entry.type == 0x20 and entry.section_number > 0
            and entry.value == 0 and entry.aux_count == 0, "invalid import thunk definition")
    code = obj.section(entry.section_number)
    require(code.name == ".text" and code.raw_data == b"\xff\x25" + bytes(4)
            and code.characteristics & 0x20001020 == 0x20001020,
            "canonical import thunk code/COMDAT drift")
    iat_symbol = symbol("__imp_" + object_symbol)
    require(iat_symbol.storage_class == 2 and iat_symbol.type == 0 and iat_symbol.value == 0
            and iat_symbol.section_number > 0 and iat_symbol.aux_count == 0,
            "invalid canonical imported-address definition")
    relocs = obj.relocations_by_section.get(code.index, ())
    require(len(relocs) == 1 and (relocs[0].offset, relocs[0].type, relocs[0].symbol_index)
            == (2, 6, iat_symbol.index), "canonical thunk has the wrong IAT relocation")
    iat = obj.section(iat_symbol.section_number)
    require(iat.name == ".idata$5", "canonical IAT section drift")
    names = [s for s in obj.sections if s.name == ".idata$6"]
    lookup = [s for s in obj.sections if s.name == ".idata$4"]
    require(len(lookup) == 1, "canonical import lookup table is ambiguous")
    if import_ordinal is None:
        require(len(names) == 1, "canonical import name table is ambiguous")
        name = names[0]
        require(name.raw_data[2:] == import_name.encode("ascii") + b"\0"
                and not obj.relocations_by_section.get(name.index), "canonical import name drift")
        for section in (iat, lookup[0]):
            refs = obj.relocations_by_section.get(section.index, ())
            require(section.raw_data == bytes(4) and len(refs) == 1
                    and refs[0].offset == 0 and refs[0].type == 7, "canonical import lookup relocation drift")
            target = obj.symbols_by_index[refs[0].symbol_index]
            require(target.section_number == name.index and target.value == 0
                    and target.storage_class == 3 and target.type == 0,
                    "canonical import lookup does not target the exact name table")
    else:
        require(type(import_ordinal) is int and 0 <= import_ordinal <= 0xffff
                and import_name == f"#{import_ordinal}", "invalid canonical import ordinal identity")
        require(not names, "ordinal import unexpectedly contains a name table")
        encoded = struct.pack("<I", 0x80000000 | import_ordinal)
        for section in (iat, lookup[0]):
            require(section.raw_data == encoded and not obj.relocations_by_section.get(section.index),
                    "canonical ordinal import table drift")
    descriptor = symbol(descriptor_name)
    require(descriptor.storage_class == 2 and descriptor.type == 0 and descriptor.value == 0
            and descriptor.section_number == 0, "canonical runtime descriptor dependency drift")
    proof = dict(symbol=object_symbol, iat_symbol=iat_symbol.name, code_size=6,
                relocation=dict(offset=2, type=6, addend=0), import_name=import_name)
    if import_ordinal is not None:
        proof.update(import_ordinal=import_ordinal, descriptor=descriptor_name)
    return proof


def canonical_import_proof(object_symbol, identity):
    from _recoil.commands.provider_function_mutation import parse_archive_members
    dll, ordinal = identity["dll"].casefold(), identity["ordinal"]
    if dll == "msvcrt.dll" and ordinal is None:
        library, member_name, descriptor = "VC/LIB/MSVCRT.LIB", "MSVCRT.dll", "__IMPORT_DESCRIPTOR_MSVCRT"
    elif dll == "avifil32.dll" and ordinal is None:
        library, member_name, descriptor = "VC/LIB/VFW32.LIB", "AVIFIL32.dll", "__IMPORT_DESCRIPTOR_AVIFIL32"
    elif dll == "mfc42.dll" and type(ordinal) is int:
        library, member_name, descriptor = "VC/MFC/LIB/MFC42.LIB", "MFC42.DLL", "__IMPORT_DESCRIPTOR_MFC42"
    else:
        raise ValueError("native import proof requires a named MSVCRT/AVIFIL32 or ordinal MFC42 canonical import")
    matches = []
    for member in parse_archive_members((DEFAULT_VC5_ROOT / library).read_bytes()):
        if object_symbol.encode("ascii") not in member.data:
            continue
        obj = CoffObject.from_bytes(member.data)
        if any(s.name == object_symbol and s.section_number > 0 for s in obj.symbols):
            require(member.name == member_name, "canonical import member DLL differs")
            matches.append(prove_import_member(obj, object_symbol, identity["name"],
                           import_ordinal=ordinal, descriptor_name=descriptor))
    require(len(matches) == 1, "canonical import definition is missing or ambiguous")
    return dict(library=library, member=member_name, **matches[0])


def import_owner_context(document, target_id, target_row, provider_owner_id=None):
    """Retain an unowned target or one explicitly selected accepted provider."""
    require(not any(target_row.get(k) for k in
                    ("object_symbol", "logical_aliases", "relocation_target_binding", "provider_object_identity")),
            "native import target already has a typed identity")
    owners = [(key, owner) for key, owner in document.collection("owners").items()
              if any(r.get("kind") == "primary-function" and
                     (r.get("symbol_id") == target_id or r.get("address") == target_row["address"])
                     for r in owner.get("relationships", []))]
    if provider_owner_id is None:
        require(target_row.get("ownership_state") in {None, "unresolved"} and not owners,
                "native import target ownership requires an explicit accepted provider")
        return None
    require(isinstance(provider_owner_id, str) and provider_owner_id
            and len(owners) == 1 and owners[0][0] == provider_owner_id
            and target_row.get("ownership_state") == "primary-owned",
            "native import provider must be the exclusive existing primary owner")
    owner = owners[0][1]
    require(owner.get("binary") == "recoil" and owner.get("kind") == "provider-boundary"
            and owner.get("provider_state") == "accepted" and owner.get("lifecycle_state") == "accepted"
            and owner.get("gates", {}).get("boundary") == "accepted"
            and owner.get("gates", {}).get("source") == "accepted",
            "native import provider boundary and source must already be accepted")
    relationships = [r for r in owner.get("relationships", []) if r.get("kind") == "primary-function"
                     and (r.get("symbol_id") == target_id or r.get("address") == target_row["address"])]
    require(len(relationships) == 1 and relationships[0].get("symbol_id") == target_id
            and relationships[0].get("address") == target_row["address"],
            "native import provider requires one exact existing primary relationship")
    # Capture the full owner record so later ownership, gate, recipe, or evidence
    # edits require renewed review. This dependency proof accepts none of them.
    return dict(owner_id=provider_owner_id, owner=deepcopy(owner))


def binding_context(document, bindings, source_id, object_symbol, offset, target_symbol, evidence_ids, reference,
                    provider_owner_id=None):
    from _recoil.commands.live_byte_verify import _pe_bytes
    from _recoil.commands.relocation_expectations import build_object_binding_snapshot, decode_x86_operand_sites
    row = document.collection("symbols").get(source_id, {})
    require(row.get("binary") == "recoil" and row.get("kind") == "function"
            and row.get("pipeline_class") in {"authored", "authored-lifecycle"}, "import source is not authored")
    require(type(offset) is int and offset >= 1, "import offset must be an integer relocation-field offset")
    start = address_value(row["address"])
    body = _pe_bytes(reference, start, address_value(row["end_exclusive"])-start)
    sites, unresolved = decode_x86_operand_sites(body, function_address=start)
    found = [s for s in sites if s.offset == offset and s.relocation_type == 20 and s.opcode in {"e8", "e9"}]
    require(not unresolved and len(found) == 1, "import site is not one decoded direct call/tail relocation")
    target = start + offset + 4 + struct.unpack_from("<i", body, offset)[0]
    target_id = f"recoil:function:0x{target:x}"
    target_row = document.collection("symbols").get(target_id, {})
    require(target_row.get("binary") == "recoil" and target_row.get("kind") == "function"
            and target_row.get("pipeline_class") == "non-authored"
            and target_row.get("extent_state") == "known" and target_row.get("output_section_id") == "recoil:section:.text"
            and address_value(target_row["address"]) == target
            and target_row.get("size") == address_value(target_row["end_exclusive"])-target >= 6,
            "target is not an existing known non-authored thunk inventory row")
    owner_context = import_owner_context(document, target_id, target_row, provider_owner_id)
    require(isinstance(evidence_ids, list) and evidence_ids and len(set(evidence_ids)) == len(evidence_ids)
            and set(evidence_ids) <= set(row.get("evidence_ids", []))
            and all(e in document.collection("evidence") for e in evidence_ids), "import evidence is not current source evidence")
    identity, iat = import_at(reference, target)
    proof = canonical_import_proof(target_symbol, identity)
    context = dict(source_binding=build_object_binding_snapshot(document, bindings, symbol_id=source_id,
                object_symbol=object_symbol), offset=offset, opcode=found[0].opcode, target=target,
                target_id=target_id, target_context={k: target_row.get(k) for k in
                    ("address", "end_exclusive", "size", "kind", "pipeline_class", "ownership_state", "output_section_id")},
                identity=identity, iat=iat, canonical=proof, evidence_ids=evidence_ids)
    if owner_context is not None:
        context["provider_owner"] = owner_context
    return context


def derive_import_expectations(document, bindings, row, object_symbol, reference):
    result = {}
    for source_id in row.get("scope_ids", (row.get("symbol_id"),)):
        for binding in document.collection("symbols").get(source_id, {}).get(FIELD, []):
            require(binding.get("schema") == SCHEMA and binding.get("reviewed") is True, "invalid native import binding")
            old = binding["context"]
            if old["source_binding"]["object_symbol"] != object_symbol:
                continue
            context = binding_context(document, bindings, source_id, object_symbol, old["offset"],
                                      old["canonical"]["symbol"], old["evidence_ids"], reference,
                                      old.get("provider_owner", {}).get("owner_id"))
            require(context == old, "native import binding is stale")
            key = (old["offset"], 20)
            require(key not in result, "duplicate native import selector")
            result[key] = dict(object_symbol=object_symbol, offset=key[0], type=20, type_name="REL32",
                target_symbol=old["canonical"]["symbol"], target_symbol_id=old["target_id"], coff_addend=0,
                resolved_target_addend=0, retail_target=old["target"], derivation=SCHEMA, native_import=context)
    return result


def prove_candidate_import(obj, body, relocation, expected, parsed_map, image, candidate_target):
    symbol = obj.symbols_by_index[relocation.symbol_index]
    require(relocation.type == 20 and relocation.offset-body.start == expected["offset"]
            and symbol.name == relocation.symbol_name == expected["target_symbol"]
            and symbol.storage_class == 2 and symbol.type == 0x20 and symbol.section_number == 0
            and symbol.value == 0 and symbol.aux_count == 0
            and body.data[expected["offset"]:expected["offset"]+4] == bytes(4), "native import COFF reference drift")
    maps = [m for m in parsed_map.symbols if m.symbol == symbol.name]
    require(len(maps) == 1 and int(maps[0].address) == candidate_target, "native import MAP target is missing or ambiguous")
    identity, _ = import_at(image, candidate_target)
    require(identity == expected["native_import"]["identity"], "linked thunk resolves to a different DLL/import")


def stage_import_binding(entries, binding, expected_binding=None):
    """Refresh only registration provenance, retaining every freshly derived fact."""
    require(isinstance(entries, list), "native import bindings must be a list")
    require(all(isinstance(item, dict) and isinstance(item.get("context"), dict) for item in entries),
            "native import binding collection is malformed")
    matches = [i for i, item in enumerate(entries)
               if item["context"].get("offset") == binding["context"]["offset"]]
    if expected_binding is None:
        require(not matches, "import site already has a binding")
        entries.append(deepcopy(binding))
        return "added"
    require(isinstance(expected_binding, dict)
            and set(expected_binding) == {"schema", "reviewed", "reason", "context"}
            and expected_binding.get("schema") == SCHEMA
            and expected_binding.get("reviewed") is True,
            "native import refresh requires an exact reviewed old binding")
    require(len(matches) == 1 and entries[matches[0]] == expected_binding,
            "native import refresh old binding is missing, duplicated or stale")
    old_context, new_context = deepcopy(expected_binding["context"]), deepcopy(binding["context"])
    require(isinstance(old_context, dict) and isinstance(new_context, dict)
            and isinstance(old_context.get("source_binding"), dict)
            and isinstance(new_context.get("source_binding"), dict),
            "native import refresh source context is malformed")
    old_source, new_source = old_context.pop("source_binding"), new_context.pop("source_binding")
    require(old_context == new_context, "native import refresh cannot change target, operand or provider facts")
    old_ids, new_ids = old_source.pop("registration_ids"), new_source.pop("registration_ids")
    require(old_source == new_source, "native import refresh cannot change source identity or extent")
    for ids in (old_ids, new_ids):
        require(isinstance(ids, list) and ids and all(isinstance(item, str) and item for item in ids)
                and len(ids) == len(set(ids)), "native import refresh registration set is invalid")
    require(old_ids != new_ids, "native import refresh requires changed source registrations")
    entries[matches[0]] = deepcopy(binding)
    return "refreshed-source-registration"


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
        fields = {"reviewed", "source_symbol_id", "object_symbol", "offset", "target_symbol", "evidence_ids", "reason"}
        require(isinstance(payload, dict) and fields <= set(payload)
                and set(payload) <= fields | {"expected_binding", "provider_owner_id"}
                and payload["reviewed"] is True and isinstance(payload["reason"], str) and payload["reason"].strip(),
                "native import requires the exact reviewed source-site payload")
        require("expected_binding" not in payload or isinstance(payload["expected_binding"], dict),
                "native import refresh expected_binding must be an exact object")
        store = ProgressStore(DEFAULT_PROGRESS_PATH)
        document = store.load()
        require(document.revision == args.expected_revision, "tracker revision changed")
        context = binding_context(document, _bindings(document, DEFAULT_MANIFEST_DIR), payload["source_symbol_id"],
            payload["object_symbol"], payload["offset"], payload["target_symbol"], payload["evidence_ids"], DEFAULT_REFERENCE,
            payload.get("provider_owner_id"))
        proposed = deepcopy(document.data)
        entries = proposed["symbols"][payload["source_symbol_id"]].setdefault(FIELD, [])
        binding = dict(schema=SCHEMA, reviewed=True, reason=payload["reason"], context=context)
        action = stage_import_binding(entries, binding, payload.get("expected_binding"))
        commit = store.commit(proposed, expected_revision=args.expected_revision, apply=args.apply)
        print(json.dumps(dict(kind="native-import-binding", action=action, binding=binding, accepted_owner=False,
                              accepted_provider_bytes=False, commit=commit.to_dict()), indent=2))
        return 0
    except (OSError, ValueError, KeyError, TypeError, ProgressError) as exc:
        print(f"native import binding error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
