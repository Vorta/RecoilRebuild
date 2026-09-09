from __future__ import annotations

import struct
from pathlib import Path
import sys

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands.live_final_verify import (  # noqa: E402
    _byte_difference_ranges,
    _complete_image_bytes_without_timestamp,
    _semantic_differences,
)
from _recoil.lib.pe import PeFormatError, data_directory, parse_pe_headers, rva_to_offset  # noqa: E402


def minimal_pe() -> bytes:
    data = bytearray(0x400)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", data, 0x84, 0x14C, 1, 0x12345678, 0, 0, 0xE0, 0x10F)
    optional = 0x98
    struct.pack_into("<H", data, optional, 0x10B)
    struct.pack_into("<I", data, optional + 16, 0x1000)
    struct.pack_into("<I", data, optional + 28, 0x400000)
    struct.pack_into("<II", data, optional + 32, 0x1000, 0x200)
    struct.pack_into("<I", data, optional + 56, 0x2000)
    struct.pack_into("<I", data, optional + 64, 0xABC)
    struct.pack_into("<H", data, optional + 68, 2)
    struct.pack_into("<I", data, optional + 92, 16)
    section = optional + 0xE0
    data[section:section + 8] = b".text\0\0\0"
    struct.pack_into("<IIIIIIHHI", data, section + 8, 0x20, 0x1000, 0x200, 0x200, 0, 0, 0, 0, 0x60000020)
    data[0x200:0x203] = b"\x90\x90\xC3"
    return bytes(data)


def test_synthetic_pe32_headers_sections_and_rva_mapping(tmp_path, monkeypatch) -> None:
    with pytest.raises(PeFormatError):
        parse_pe_headers(b"not a PE")
    headers = parse_pe_headers(minimal_pe(), source="unit")
    assert headers.machine == 0x14C
    assert headers.entry_point_rva == 0x1000
    assert headers.sections[0].name == ".text"
    assert rva_to_offset(0x1002, headers.sections) == 0x202
    assert data_directory(headers, 2).size == 0
    _exercise_storage_comparison(tmp_path, monkeypatch)


def _exercise_storage_comparison(tmp_path, monkeypatch):
    from types import SimpleNamespace as Row
    from _recoil.lib import storage_proof as storage
    from _recoil.commands import live_byte_verify as byte
    from _recoil.lib.progress import ProgressError
    data = bytearray(minimal_pe())
    struct.pack_into("<I", data, 0x178 + 8, 0x400)
    assert storage.image_storage(data, 0x4011fe, 4) == (bytes(4), 2, ".text")
    for address, size in ((0x4013ff, 2), (0x400fff, 2)):
        with pytest.raises(ProgressError): storage.image_storage(data, address, size)
    # The relocation directory defines complete pointer fields independently
    # of the candidate COFF. A field crossing the selection must fail closed.
    struct.pack_into("<II", data, 0x98+96+5*8, 0x1080, 12)
    struct.pack_into("<IIHH", data, 0x280, 0x1000, 12, 0x3010, 0)
    struct.pack_into("<I", data, 0x210, 0x402000)
    assert storage.pointer_fields(data, 0x401010, 4) == {0}
    with pytest.raises(ProgressError): storage.pointer_fields(data, 0x401011, 3)
    broken = bytearray(data); struct.pack_into("<H", broken, 0x28a, 0x3010)
    with pytest.raises(ProgressError): storage.pointer_fields(broken, 0x401010, 4)
    broken = bytearray(data); struct.pack_into("<H", broken, 0x288, 0x2010)
    with pytest.raises(ProgressError): storage.pointer_fields(broken, 0x401010, 4)
    reference, candidate = tmp_path/"retail.exe", tmp_path/"candidate.exe"
    reference.write_bytes(data); candidate.write_bytes(data)
    sid, oid, storage_id = "recoil:data:0x401010", "owner", "storage"
    extent = dict(address="0x401010", end_exclusive="0x401014", size=4, extent_state="known")
    symbol = dict(extent, kind="data", binary="recoil", disposition="authored", output_section_id="recoil:section:.text", storage_contribution_ids=[storage_id])
    contribution = dict(binary="recoil", reference=extent, symbol_ids=[sid], owner_ids=[oid], output_section_id=symbol["output_section_id"])
    rows = dict(symbols={sid: symbol}, owners={oid: {"kind": "data-owner", "relationships": [{"kind": "primary-data", "symbol_id": sid}]}}, storage_contributions={storage_id: contribution})
    document = Row(collection=lambda name: rows[name])
    declaration = Row(byte_length=4, object_offset=0, symbol="_data", symbol_regex=None)
    binding = Row(function=declaration, source_from="unit.cpp", target=Row(source_from="unit.cpp"))
    relocation = Row(offset=0, type=6, symbol_name="_target")
    body = Row(start=0, data=bytes(4), relocations=[relocation])
    obj = Row(data_symbol_bytes=lambda *a, **kw: body)
    target = Row(object_symbols=["_target"], registered_selector=None)
    monkeypatch.setattr(storage, "REPO_ROOT", tmp_path)
    monkeypatch.setattr(storage, "object_path", lambda *a: tmp_path/"unit.obj")
    monkeypatch.setattr(storage.CoffObject, "from_path", lambda *a: obj)
    monkeypatch.setattr(storage, "_resolve_identity", lambda *a: (target, 0, []))
    def placement(**kw):
        return Row(target_bases=frozenset({0x401010 if kw["symbol_name"] == "_data" else 0x402000}), reason="")
    monkeypatch.setattr(byte, "_candidate_target_identity", placement)
    maps = [Row(symbol="_data", object="unit.obj", is_function=False, address=0x401010)]
    def compare():
        return storage.compare_storage(document, storage_id, bindings={sid: [binding]}, identities=[],
            config=Row(sources=[tmp_path/"unit.cpp"]), paths=Row(exe_path=candidate),
            parsed_map=Row(symbols=maps), reference=reference)
    assert all(compare()["dimensions"].values())
    body.data = bytes(8)
    with pytest.raises(ProgressError, match="allocation extent"): compare()
    body.data = bytes(4)
    maps[0].object = "other.obj"
    with pytest.raises(ProgressError, match="different object"): compare()
    maps[0].object = "unit.obj"
    maps.append(Row(symbol="_different_allocation", object="unit.obj", is_function=False, address=0x401010))
    with pytest.raises(ProgressError, match="merged"): compare()
    maps.pop()
    relocation.type = 20
    assert not compare()["dimensions"]["relocation"]
    relocation.type = 6
    relocation.symbol_name = "_wrong"
    assert not compare()["dimensions"]["link"]
    relocation.symbol_name = "_target"
    body.data = b"\x01\0\0\0"
    assert not compare()["dimensions"]["relocation"]
    body.data = bytes(4)
    changed = bytearray(data); struct.pack_into("<I", changed, 0x210, 0x403000)
    candidate.write_bytes(changed)
    assert not compare()["dimensions"]["raw"] and not compare()["dimensions"]["link"]
    monkeypatch.setattr(storage, "_resolve_identity", lambda *a: (None, 0, []))
    with pytest.raises(ProgressError, match="unresolved typed"): compare()

    # Provider data needs a complete typed loader-pointer census even when
    # raw bytes and addresses are identical in both images.
    candidate.write_bytes(data)
    operand = dict(offset=0, width=4, kind="absolute32", target_symbol="_target",
                   retail_target=0x402000, target_addend=0)
    provider = dict(symbol_id=sid, map_symbol="_data", object="runtime:unit.obj",
                    provider="runtime", archive_member="unit.obj", operands=[operand])
    provider_row = dict(extent, symbol_id=sid, linked_provider_binding=provider)
    provider_maps = [Row(symbol="_data", object="runtime:unit.obj", is_function=False, address=0x401010),
                     Row(symbol="_target", object="runtime:target.obj", is_function=False, address=0x402000)]
    def compare_provider():
        return byte._compare_provider_row(row=provider_row, paths=Row(exe_path=candidate),
            reference=reference, parsed_map=Row(symbols=provider_maps), data_body=True)
    assert compare_provider()["passed"]
    provider["operands"] = []
    assert not compare_provider()["passed"]
    provider["operands"] = [operand]
    provider_maps[0].is_function = True
    assert not compare_provider()["passed"]
    provider_maps[0].is_function = False
    provider_maps[1].address += 4
    assert not compare_provider()["passed"]
    provider_maps[1].address -= 4
    changed = bytearray(data)
    struct.pack_into("<II", changed, 0x98+96+5*8, 0, 0)
    candidate.write_bytes(changed)
    assert not compare_provider()["passed"]


def test_timestamp_is_diagnostic_but_every_other_image_byte_is_retained(tmp_path, monkeypatch) -> None:
    data = minimal_pe()
    headers = parse_pe_headers(data)
    normalized = _complete_image_bytes_without_timestamp(data, headers)
    assert normalized[headers.pe_offset + 8:headers.pe_offset + 12] == b"\0" * 4
    assert normalized[:headers.pe_offset + 8] == data[:headers.pe_offset + 8]
    assert normalized[headers.pe_offset + 12:] == data[headers.pe_offset + 12:]
    _exercise_final_instruction_matches(tmp_path, monkeypatch)


def _exercise_final_instruction_matches(tmp_path, monkeypatch):
    from copy import deepcopy
    from _recoil.commands import live_final_verify as final
    from _recoil.lib import match_evidence as evidence
    from _recoil.lib.function_match import compare_instructions, MATCH_VERSION
    source = tmp_path / "unit.cpp"
    source.write_text("int f() { return 2; }\n")
    monkeypatch.setattr(evidence, "REPO_ROOT", tmp_path)
    context = {"source": "unit.cpp", "current": True}
    monkeypatch.setattr(evidence, "source_context", lambda *a: context)
    first = bytes.fromhex("ba01000000 42 8bc2 c3")
    second = bytes.fromhex("b901000000 41 8bc1 c3")
    symbol = "?f@@YAHXZ"
    proof = compare_instructions(first, second, symbol=symbol)
    assert proof["passed"]
    retail = bytearray(minimal_pe()); retail[0x200:0x200 + len(first)] = first
    candidate = bytearray(retail); candidate[0x200:0x200 + len(second)] = second
    state = {"version": MATCH_VERSION, "level": "instruction", "validation_mode": "live", "freshness": "current",
             "evidence_ids": ["proof"], "review_evidence_id": "review",
             "dependencies": evidence.dependency_states(["unit.cpp"]), "retail_relocations": []}
    review = {"version": MATCH_VERSION, "evidence_id": "review", "decision": "compiler-register-allocation-only",
              "no_remaining_credible_source_options": True, "context": context, "differences": proof["differences"]}
    identity = {"symbol_id": "recoil:function:0x401000", "map_symbol": symbol, "match_level": "instruction",
                "function_match": state, "instruction_match_review": review}
    coverage = {"complete": True, "sections": [{"name": ".text", "typed_entities": [
        {"start": 0, "end": len(first), "identities": [identity]}]}]}
    monkeypatch.setattr(final, "validate_coverage_view", lambda *a, **kw: [])
    monkeypatch.setattr(final, "_validate_coverage_text_population", lambda *a: [])
    def verify(code, catalog=coverage):
        return final._compare_image_data(source, source, candidate_data=bytes(code), reference_data=bytes(retail),
                                         coverage=catalog, candidate_map_rows=[])
    report = verify(candidate)
    assert report["passed"], report["semantic_failures"]
    assert not report["normalized_complete_file_equal"] and report["permitted_complete_file_equal"]
    assert report["contains_instruction_matches"] and not report["sections"][0]["exact_bytes"]
    for offset in (0x201, 0x210, 0x300, 0x98 + 64):
        changed = bytearray(candidate); changed[offset] ^= 1
        assert not verify(changed)["passed"], hex(offset)
    for field, value in (("decision", "not-approved"), ("no_remaining_credible_source_options", False),
                         ("differences", []), ("context", {"source": "unit.cpp", "current": False})):
        invalid = deepcopy(coverage)
        invalid["sections"][0]["typed_entities"][0]["identities"][0]["instruction_match_review"][field] = value
        assert not verify(candidate, invalid)["passed"]
    # An exact alias sharing the same physical interval cannot inherit a
    # weaker alias's permission for differing bytes.
    invalid = deepcopy(coverage)
    invalid["sections"][0]["typed_entities"][0]["identities"].append({"symbol_id": "alias", "match_level": "byte"})
    assert not verify(candidate, invalid)["passed"]


def test_semantic_and_raw_difference_reporting_is_bounded_and_typed() -> None:
    assert _semantic_differences({"a": [1, 2]}, {"a": [1, 3]}) == [
        {"path": "a[1]", "expected": 2, "candidate": 3}
    ]
    assert _byte_difference_ranges(b"abXXefY", b"abZZefQ") == [
        {"start": 2, "end_exclusive": 4},
        {"start": 6, "end_exclusive": 7},
    ]
