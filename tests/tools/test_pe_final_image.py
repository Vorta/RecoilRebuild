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


def test_synthetic_pe32_headers_sections_and_rva_mapping() -> None:
    headers = parse_pe_headers(minimal_pe(), source="unit")
    assert headers.machine == 0x14C
    assert headers.entry_point_rva == 0x1000
    assert headers.sections[0].name == ".text"
    assert rva_to_offset(0x1002, headers.sections) == 0x202
    assert data_directory(headers, 2).size == 0


def test_timestamp_is_diagnostic_but_every_other_image_byte_is_retained() -> None:
    data = minimal_pe()
    headers = parse_pe_headers(data)
    normalized = _complete_image_bytes_without_timestamp(data, headers)
    assert normalized[headers.pe_offset + 8:headers.pe_offset + 12] == b"\0" * 4
    assert normalized[:headers.pe_offset + 8] == data[:headers.pe_offset + 8]
    assert normalized[headers.pe_offset + 12:] == data[headers.pe_offset + 12:]


def test_semantic_and_raw_difference_reporting_is_bounded_and_typed() -> None:
    assert _semantic_differences({"a": [1, 2]}, {"a": [1, 3]}) == [
        {"path": "a[1]", "expected": 2, "candidate": 3}
    ]
    assert _byte_difference_ranges(b"abXXefY", b"abZZefQ") == [
        {"start": 2, "end_exclusive": 4},
        {"start": 6, "end_exclusive": 7},
    ]


def test_invalid_image_fails_closed() -> None:
    with pytest.raises(PeFormatError):
        parse_pe_headers(b"not a PE")
