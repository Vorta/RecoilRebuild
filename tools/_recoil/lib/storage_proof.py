"""Invocation-local typed storage comparison; no ledger writes or candidate expectations."""
from __future__ import annotations

import struct
import re
from pathlib import Path

from _recoil.commands.asm_verify import CoffObject
from _recoil.commands.byte_symbol_selectors import resolve_target_definition
from _recoil.commands.relocation_expectations import _resolve_identity
from _recoil.commands.vc5_build import object_path
from _recoil.commands.vc5_verify import resolve_coff_symbol_regex
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.progress import ProgressError, STORAGE_DIMENSIONS
from _recoil.lib.tooling import REPO_ROOT


def require(condition, message):
    if not condition:
        raise ProgressError(message)


def integer(value):
    require(type(value) in (str, int), "expected an exact integer")
    return int(value, 0) if isinstance(value, str) else value


def extent(row):
    require(row.get("extent_state") == "known", "storage extent is unresolved")
    start, end = integer(row["address"]), integer(row["end_exclusive"])
    require(end > start and integer(row["size"]) == end - start, "storage extent is inconsistent")
    return start, end


def image_storage(image, start, size):
    """Read the loader's initialized/zero-fill partition without inventing bytes."""
    pe = parse_pe_headers(image, source="live storage image")
    matches = [s for s in pe.sections if pe.image_base + s.virtual_address <= start
               and start + size <= pe.image_base + s.virtual_address + s.virtual_size]
    require(len(matches) == 1, "storage must lie within exactly one loaded section")
    section = matches[0]
    relative = start - pe.image_base - section.virtual_address
    initialized = max(0, min(size, section.raw_size - relative))
    offset = section.raw_pointer + relative
    require(not initialized or offset + initialized <= len(image), "truncated initialized storage")
    payload = image[offset:offset + initialized] if initialized else b""
    return payload + bytes(size - initialized), initialized, section.name


def pointer_fields(image, start, size):
    """Derive complete HIGHLOW fields from the immutable PE relocation directory."""
    pe = parse_pe_headers(image, source="storage relocations")
    directory = pe.data_directories[5]
    if not directory.rva and not directory.size:
        return set()
    require(directory.rva > 0 and directory.size >= 8, "invalid base relocation directory")
    first = rva_to_offset(directory.rva, pe.sections)
    last = rva_to_offset(directory.rva + directory.size - 1, pe.sections)
    require(last == first + directory.size - 1 and last < len(image), "relocation directory is not contiguous")
    cursor, fields = first, set()
    while cursor < last + 1:
        require(cursor + 8 <= last + 1, "truncated relocation block")
        page, length = struct.unpack_from("<II", image, cursor)
        require(length >= 8 and length % 2 == 0 and cursor + length <= last + 1, "invalid relocation block extent")
        for offset in range(cursor + 8, cursor + length, 2):
            value = struct.unpack_from("<H", image, offset)[0]
            kind, address = value >> 12, pe.image_base + page + (value & 0xfff)
            if kind == 0:
                continue
            if address < start + size and address + 4 > start:
                require(kind == 3 and start <= address and address + 4 <= start + size,
                        "unsupported or straddling storage relocation")
                require(address - start not in fields, "duplicate storage relocation")
                fields.add(address - start)
        cursor += length
    return fields


def storage_scope(document, storage_id):
    storage = document.collection("storage_contributions").get(storage_id)
    require(isinstance(storage, dict) and storage.get("binary") == "recoil", "unknown Recoil storage contribution")
    start, end = extent(storage.get("reference", {}))
    identities = storage.get("symbol_ids")
    require(isinstance(identities, list) and identities and len(set(identities)) == len(identities),
            "storage requires a unique nonempty symbol census")
    reverse = {key for key, symbol in document.collection("symbols").items()
               if storage_id in symbol.get("storage_contribution_ids", [])}
    require(reverse == set(identities), "storage symbol census is not reciprocal")
    owner_ids = set()
    for identity in identities:
        symbol = document.collection("symbols").get(identity, {})
        require(symbol.get("kind") in {"data", "data-symbol"} and symbol.get("disposition") == "authored",
                "storage acceptance requires existing authored data")
        require(extent(symbol) == (start, end) and symbol.get("output_section_id") == storage.get("output_section_id")
                and storage_id in symbol.get("storage_contribution_ids", []), "storage symbol/section/extent links disagree")
        owners = [(key, owner) for key, owner in document.collection("owners").items()
                  if any(edge.get("kind") == "primary-data" and edge.get("symbol_id") == identity
                         for edge in owner.get("relationships", []))]
        require(len(owners) == 1 and owners[0][1].get("kind") != "provider-boundary", "storage lacks one authored primary owner")
        owner_ids.add(owners[0][0])
    require(set(storage.get("owner_ids", [])) == owner_ids, "storage owner census disagrees with primary ownership")
    for other_id, other in document.collection("storage_contributions").items():
        if other_id == storage_id or other.get("binary") != "recoil":
            continue
        reference = other.get("reference", {})
        if reference.get("extent_state") != "known":
            address = reference.get("address")
            require(address is None or integer(address) >= end or
                    other.get("output_section_id") != storage.get("output_section_id"),
                    "unresolved earlier storage may overlap the selected contribution")
            continue
        a, b = extent(reference)
        if max(a, start) < min(b, end):
            require((a, b) == (start, end) and storage.get("overlap") not in (None, "none")
                    and other.get("overlap") not in (None, "none"), "unmodelled overlapping storage")
    return storage, identities, start, end


def compare_storage(document, storage_id, *, bindings, identities, config, paths, parsed_map, reference):
    from _recoil.commands.live_byte_verify import _candidate_target_identity

    storage, members, start, end = storage_scope(document, storage_id)
    retail = reference.read_bytes()
    image = paths.exe_path.read_bytes()
    expected, initialized, section = image_storage(retail, start, end - start)
    require(storage["output_section_id"] == "recoil:section:" + section, "storage section differs from retail")
    fields = pointer_fields(retail, start, end - start)
    contracts = {}
    for field in sorted(fields):
        value = int.from_bytes(expected[field:field + 4], "little")
        target, addend, _ = _resolve_identity(value, identities)
        require(target is not None, f"unresolved typed storage target at +0x{field:x}")
        contracts[field] = target, addend
    objects = {}
    def load(source):
        source = (REPO_ROOT / source).resolve()
        require(source in {x.resolve() for x in config.sources}, "storage source is absent from the canonical build")
        if source not in objects:
            objects[source] = CoffObject.from_path(object_path(config, paths, source))
        return objects[source]
    results = []
    for member in members:
        selected = bindings.get(member, [])
        require(bool(selected), f"{member} has no registered native data selector")
        for binding in selected:
            declaration = binding.function
            require(integer(declaration.byte_length) == end - start, "registered data extent differs")
            source = binding.source_from or binding.target.source_from
            obj = load(source)
            name = (resolve_coff_symbol_regex(obj, declaration.symbol_regex, item_label=member)
                    if declaration.symbol_regex else declaration.symbol)
            natural = obj.data_symbol_bytes(name)
            require(declaration.object_offset == 0 and len(natural.data) == end - start,
                    "native allocation extent differs or requires an explicit parent/padding model")
            body = obj.data_symbol_bytes(name, byte_length=end - start, object_offset=declaration.object_offset)
            require(len(body.data) == end - start, "object storage extent differs")
            placement = _candidate_target_identity(coff_object=obj, obj_path=object_path(config, paths, REPO_ROOT / source),
                parsed_map=parsed_map, symbol_name=name, candidate_target_base=None)
            require(len(placement.target_bases) == 1, f"{member} linked storage identity is unresolved: {placement.reason}")
            map_definitions = [r for r in parsed_map.symbols if r.symbol == name]
            expected_object = object_path(config, paths, REPO_ROOT/source).name.casefold()
            require(map_definitions and all(not r.is_function and
                    str(r.object).replace("\\", "/").rsplit("/", 1)[-1].casefold() == expected_object for r in map_definitions),
                    "linked storage definition comes from a different object; folded identity needs explicit proof")
            candidate_start = next(iter(placement.target_bases)) + declaration.object_offset
            # A merged allocation can be observed even when only one storage
            # contribution is selected. Every linked alias must have the same
            # accepted retail allocation, not merely equal initialized bytes.
            permitted_aliases = {name}
            for other_id, alternatives in bindings.items():
                other = document.collection("symbols").get(other_id, {})
                if other.get("address") != hex(start) or other.get("end_exclusive") != hex(end):
                    continue
                for alternative in alternatives:
                    definition = alternative.function
                    for mapped in parsed_map.symbols:
                        pattern = getattr(definition, "symbol_regex", None)
                        if (re.fullmatch(pattern, mapped.symbol) if pattern else mapped.symbol == definition.symbol):
                            permitted_aliases.add(mapped.symbol)
            require(all(r.symbol in permitted_aliases for r in parsed_map.symbols
                        if not r.is_function and r.address == candidate_start),
                    "distinct or unresolved retail storage allocations were merged in the candidate")
            candidate, candidate_initialized, candidate_section = image_storage(image, candidate_start, end - start)
            observed_fields = {r.offset - body.start for r in body.relocations}
            relocation_ok = (len(observed_fields) == len(body.relocations) and observed_fields == fields
                             and pointer_fields(image, candidate_start, end - start) == fields)
            for relocation in body.relocations:
                field = relocation.offset - body.start
                if relocation.type != 6 or field not in contracts:
                    relocation_ok = False
                    continue
                target, addend = contracts[field]
                target_name = target.object_symbols[0]
                if target.registered_selector:
                    pattern, target_source, kind = target.registered_selector
                    target_name = resolve_target_definition(load(target_source),
                        {"symbol_regex": pattern, "source_from": target_source, "kind": kind})
                object_addend = int.from_bytes(body.data[field:field + 4], "little")
                linked_base = int.from_bytes(candidate[field:field + 4], "little") - addend
                linked = _candidate_target_identity(coff_object=obj, obj_path=object_path(config, paths, REPO_ROOT / source),
                    parsed_map=parsed_map, symbol_name=target_name, candidate_target_base=linked_base)
                relocation_ok &= (relocation.symbol_name == target_name and object_addend == addend
                                  and linked.target_bases == frozenset({linked_base}))
            mask = {index for field in fields for index in range(field, field + 4)}
            object_ok = all(a == b for i, (a, b) in enumerate(zip(expected, body.data)) if i not in mask)
            linked_body_ok = all(a == b for i, (a, b) in enumerate(zip(expected, candidate)) if i not in mask)
            results.append({"symbol_id": member, "source": source, "object_symbol": name,
                "candidate_address": hex(candidate_start), "dimensions": {
                    "extent": True,
                    "object": object_ok and observed_fields == fields,
                    "relocation": bool(relocation_ok),
                    "order": candidate_start == start and candidate_section == section,
                    "link": bool(relocation_ok and linked_body_ok and candidate_section == section),
                    "raw": candidate_start == start and candidate == expected,
                    "zero-fill": (initialized == candidate_initialized
                                  and body.data[initialized:] == bytes(end - start - initialized)
                                  and candidate[initialized:] == bytes(end - start - initialized)),
                }})
    require(len({item["candidate_address"] for item in results}) == 1,
            "retail storage aliases became distinct candidate allocations")
    return {"storage_id": storage_id, "symbol_ids": members, "identities": results,
            "retail_address": hex(start), "size": end-start,
            "dimensions": {name: all(item["dimensions"][name] for item in results) for name in STORAGE_DIMENSIONS}}


def preserve_storage_relationships(reports):
    """Check aliasing and separation independently of absolute placement."""
    for index, first in enumerate(reports):
        for second in reports[index+1:]:
            a, b = integer(first["retail_address"]), integer(second["retail_address"])
            ca = integer(first["identities"][0]["candidate_address"])
            cb = integer(second["identities"][0]["candidate_address"])
            overlap = max(0, min(a+first["size"], b+second["size"]) - max(a, b))
            candidate_overlap = max(0, min(ca+first["size"], cb+second["size"]) - max(ca, cb))
            require(overlap == candidate_overlap and (not overlap or a-b == ca-cb),
                    "candidate storage alias/overlap relationships differ from retail")
