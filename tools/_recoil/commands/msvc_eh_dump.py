from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from dataclasses import asdict, dataclass
import json
from pathlib import Path
import struct
import sys

from _recoil.lib.pe import PeFormatError, hex32, parse_pe_headers, rva_to_offset


EH_MAGIC_VC5SP3 = 0x19930520


@dataclass(frozen=True)
class UnwindEntry:
    state: int
    to_state: int
    action_va: int


@dataclass(frozen=True)
class FuncInfo:
    va: int
    magic: int
    max_state: int
    unwind_map_va: int
    try_block_count: int
    try_block_map_va: int
    ip_map_count: int
    ip_to_state_map_va: int
    es_type_list_va: int
    unwind: list[UnwindEntry]


def parse_int(text: str) -> int:
    return int(text, 0)


def va_to_offset(va: int, image_base: int, sections) -> int:
    rva = va - image_base
    offset = rva_to_offset(rva, sections)
    if offset is None:
        raise PeFormatError(f"VA cannot be mapped to file offset: {hex32(va)}")
    return offset


def read_func_info(data: bytes, image_base: int, sections, va: int) -> FuncInfo:
    offset = va_to_offset(va, image_base, sections)
    (
        magic,
        max_state,
        unwind_map_va,
        try_block_count,
        try_block_map_va,
        ip_map_count,
        ip_to_state_map_va,
        es_type_list_va,
    ) = struct.unpack_from("<IiIIIIII", data, offset)

    unwind: list[UnwindEntry] = []
    if unwind_map_va != 0 and max_state > 0:
        unwind_offset = va_to_offset(unwind_map_va, image_base, sections)
        for state in range(max_state):
            to_state, action_va = struct.unpack_from("<iI", data, unwind_offset + state * 8)
            unwind.append(
                UnwindEntry(
                    state=state,
                    to_state=to_state,
                    action_va=action_va,
                )
            )

    return FuncInfo(
        va=va,
        magic=magic,
        max_state=max_state,
        unwind_map_va=unwind_map_va,
        try_block_count=try_block_count,
        try_block_map_va=try_block_map_va,
        ip_map_count=ip_map_count,
        ip_to_state_map_va=ip_to_state_map_va,
        es_type_list_va=es_type_list_va,
        unwind=unwind,
    )


def info_to_jsonable(info: FuncInfo) -> dict:
    result = asdict(info)
    for key in (
        "va",
        "magic",
        "unwind_map_va",
        "try_block_map_va",
        "ip_to_state_map_va",
        "es_type_list_va",
    ):
        result[key] = hex32(result[key])
    for entry in result["unwind"]:
        entry["action_va"] = hex32(entry["action_va"]) if entry["action_va"] else "0x0"
    return result


def print_text(info: FuncInfo) -> None:
    print(f"FuncInfo {hex32(info.va)}")
    print(f"  magic: {hex32(info.magic)}")
    if info.magic != EH_MAGIC_VC5SP3:
        print(f"  warning: unexpected VC FuncInfo magic, expected {hex32(EH_MAGIC_VC5SP3)}")
    print(f"  max_state: {info.max_state}")
    print(f"  unwind_map: {hex32(info.unwind_map_va)}")
    print(f"  try_blocks: {info.try_block_count} map={hex32(info.try_block_map_va)}")
    print(f"  ip_map_entries: {info.ip_map_count} map={hex32(info.ip_to_state_map_va)}")
    print(f"  es_type_list: {hex32(info.es_type_list_va)}")
    if not info.unwind:
        print("  unwind: <empty>")
        return
    print("  unwind:")
    for entry in info.unwind:
        action = hex32(entry.action_va) if entry.action_va else "0x0"
        print(f"    state {entry.state:2d} -> {entry.to_state:2d}: action {action}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Dump VC-era MSVC C++ EH FuncInfo and unwind map records from the reference PE."
    )
    parser.add_argument(
        "func_info",
        nargs="+",
        type=parse_int,
        help="FuncInfo VA(s), for example 0x4d5e68.",
    )
    parser.add_argument(
        "--reference",
        type=Path,
        default=Path("support/Recoil.exe"),
        help="reference PE path (default: support/Recoil.exe)",
    )
    parser.add_argument("--json", action="store_true", help="emit JSON")
    args = parser.parse_args(argv)

    data = args.reference.read_bytes()
    headers = parse_pe_headers(
        data,
        source=str(args.reference),
    )
    infos = [
        read_func_info(
            data,
            headers.image_base,
            headers.sections,
            va,
        )
        for va in args.func_info
    ]

    if args.json:
        json.dump(
            [info_to_jsonable(info) for info in infos],
            sys.stdout,
            indent=2,
        )
        print()
    else:
        for index, info in enumerate(infos):
            if index:
                print()
            print_text(info)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
