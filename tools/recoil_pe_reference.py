from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
from hashlib import sha256
import json
from pathlib import Path
import struct
import sys


DIRECTORY_NAMES = [
    "export",
    "import",
    "resource",
    "exception",
    "certificate",
    "base_reloc",
    "debug",
    "architecture",
    "global_ptr",
    "tls",
    "load_config",
    "bound_import",
    "iat",
    "delay_import",
    "com_descriptor",
    "reserved",
]


@dataclass(frozen=True)
class SectionInfo:
    name: str
    virtual_address: str
    virtual_size: str
    raw_pointer: str
    raw_size: str
    characteristics: str


@dataclass(frozen=True)
class DataDirectoryInfo:
    name: str
    rva: str
    size: str
    file_offset: str | None


@dataclass(frozen=True)
class ImportInfo:
    dll: str
    count: int
    symbols: list[str]


@dataclass(frozen=True)
class PeReferenceInfo:
    path: str
    size: int
    sha256: str
    pe_offset: str
    machine: str
    section_count: int
    timestamp: int
    characteristics: str
    optional_header_magic: str
    entry_point_rva: str
    entry_point_va: str
    image_base: str
    section_alignment: str
    file_alignment: str
    subsystem: int
    size_of_image: str
    checksum: str
    number_of_rva_and_sizes: int
    sections: list[SectionInfo]
    data_directories: list[DataDirectoryInfo]
    imports: list[ImportInfo]


class PeFormatError(ValueError):
    pass


def hex32(value: int) -> str:
    return f"0x{value:x}"


def read_c_string(data: bytes, offset: int) -> str:
    end = data.index(b"\0", offset)
    return data[offset:end].decode("ascii", errors="replace")


def rva_to_offset(rva: int, sections: list[tuple[str, int, int, int, int]]) -> int | None:
    for _name, virtual_address, virtual_size, raw_pointer, raw_size in sections:
        size = max(virtual_size, raw_size)
        if virtual_address <= rva < virtual_address + size:
            return raw_pointer + (rva - virtual_address)
    return None


def parse_imports(
    data: bytes, import_rva: int, sections: list[tuple[str, int, int, int, int]]
) -> list[ImportInfo]:
    if import_rva == 0:
        return []
    import_offset = rva_to_offset(import_rva, sections)
    if import_offset is None:
        raise PeFormatError(f"import directory RVA cannot be mapped: {hex32(import_rva)}")

    imports: list[ImportInfo] = []
    descriptor_index = 0
    while True:
        descriptor_offset = import_offset + descriptor_index * 20
        original_first_thunk, _time_date, _forwarder, name_rva, first_thunk = struct.unpack_from(
            "<IIIII", data, descriptor_offset
        )
        if not any((original_first_thunk, name_rva, first_thunk)):
            break
        name_offset = rva_to_offset(name_rva, sections)
        if name_offset is None:
            raise PeFormatError(f"import DLL name RVA cannot be mapped: {hex32(name_rva)}")
        dll = read_c_string(data, name_offset)
        thunk_offset = rva_to_offset(original_first_thunk or first_thunk, sections)
        if thunk_offset is None:
            raise PeFormatError(f"import thunk RVA cannot be mapped for {dll}")

        symbols: list[str] = []
        thunk_index = 0
        while True:
            thunk = struct.unpack_from("<I", data, thunk_offset + thunk_index * 4)[0]
            if thunk == 0:
                break
            if thunk & 0x80000000:
                symbols.append(f"#{thunk & 0xffff}")
            else:
                hint_name_offset = rva_to_offset(thunk, sections)
                if hint_name_offset is None:
                    raise PeFormatError(f"import hint/name RVA cannot be mapped: {hex32(thunk)}")
                symbols.append(read_c_string(data, hint_name_offset + 2))
            thunk_index += 1
        imports.append(ImportInfo(dll=dll, count=len(symbols), symbols=symbols))
        descriptor_index += 1
    return imports


def parse_pe(path: Path) -> PeReferenceInfo:
    data = path.read_bytes()
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise PeFormatError(f"not a DOS/PE executable: {path}")
    pe_offset = struct.unpack_from("<I", data, 0x3c)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise PeFormatError(f"PE signature not found at {hex32(pe_offset)}")

    coff_offset = pe_offset + 4
    (
        machine,
        section_count,
        timestamp,
        _symbol_table,
        _symbol_count,
        optional_header_size,
        characteristics,
    ) = struct.unpack_from("<HHIIIHH", data, coff_offset)
    optional_offset = coff_offset + 20
    optional_magic = struct.unpack_from("<H", data, optional_offset)[0]
    if optional_magic != 0x10B:
        raise PeFormatError(f"expected PE32 optional header, found {hex32(optional_magic)}")

    entry_point_rva = struct.unpack_from("<I", data, optional_offset + 16)[0]
    image_base = struct.unpack_from("<I", data, optional_offset + 28)[0]
    section_alignment, file_alignment = struct.unpack_from("<II", data, optional_offset + 32)
    size_of_image = struct.unpack_from("<I", data, optional_offset + 56)[0]
    checksum = struct.unpack_from("<I", data, optional_offset + 64)[0]
    subsystem = struct.unpack_from("<H", data, optional_offset + 68)[0]
    number_of_rva_and_sizes = struct.unpack_from("<I", data, optional_offset + 92)[0]

    section_offset = optional_offset + optional_header_size
    raw_sections: list[tuple[str, int, int, int, int]] = []
    sections: list[SectionInfo] = []
    for index in range(section_count):
        current = section_offset + index * 40
        name = data[current : current + 8].rstrip(b"\0").decode("ascii", errors="replace")
        virtual_size, virtual_address, raw_size, raw_pointer, _reloc, _line, _nrel, _nline, sect_chars = (
            struct.unpack_from("<IIIIIIHHI", data, current + 8)
        )
        raw_sections.append((name, virtual_address, virtual_size, raw_pointer, raw_size))
        sections.append(
            SectionInfo(
                name=name,
                virtual_address=hex32(virtual_address),
                virtual_size=hex32(virtual_size),
                raw_pointer=hex32(raw_pointer),
                raw_size=hex32(raw_size),
                characteristics=hex32(sect_chars),
            )
        )

    directories: list[DataDirectoryInfo] = []
    directory_count = min(number_of_rva_and_sizes, len(DIRECTORY_NAMES))
    for index in range(directory_count):
        rva, size = struct.unpack_from("<II", data, optional_offset + 96 + index * 8)
        file_offset = rva_to_offset(rva, raw_sections) if rva else None
        directories.append(
            DataDirectoryInfo(
                name=DIRECTORY_NAMES[index],
                rva=hex32(rva),
                size=hex32(size),
                file_offset=hex32(file_offset) if file_offset is not None else None,
            )
        )

    import_rva = 0
    if number_of_rva_and_sizes > 1:
        import_rva = struct.unpack_from("<I", data, optional_offset + 96 + 8)[0]

    return PeReferenceInfo(
        path=str(path),
        size=len(data),
        sha256=sha256(data).hexdigest(),
        pe_offset=hex32(pe_offset),
        machine=hex32(machine),
        section_count=section_count,
        timestamp=timestamp,
        characteristics=hex32(characteristics),
        optional_header_magic=hex32(optional_magic),
        entry_point_rva=hex32(entry_point_rva),
        entry_point_va=hex32(image_base + entry_point_rva),
        image_base=hex32(image_base),
        section_alignment=hex32(section_alignment),
        file_alignment=hex32(file_alignment),
        subsystem=subsystem,
        size_of_image=hex32(size_of_image),
        checksum=hex32(checksum),
        number_of_rva_and_sizes=number_of_rva_and_sizes,
        sections=sections,
        data_directories=directories,
        imports=parse_imports(data, import_rva, raw_sections),
    )


def manifest_dict(info: PeReferenceInfo) -> dict:
    data = asdict(info)
    data["path"] = data["path"].replace("\\", "/")
    return data


def read_manifest(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def compare_dicts(
    expected: dict, actual: dict, prefix: str = "", ignored_keys: set[str] | None = None
) -> list[str]:
    ignored_keys = ignored_keys or set()
    mismatches: list[str] = []
    for key in sorted(set(expected) | set(actual)):
        name = f"{prefix}.{key}" if prefix else key
        if name in ignored_keys or key in ignored_keys:
            continue
        if key not in expected:
            mismatches.append(f"{name}: unexpected value {actual[key]!r}")
            continue
        if key not in actual:
            mismatches.append(f"{name}: missing expected value {expected[key]!r}")
            continue
        left = expected[key]
        right = actual[key]
        if isinstance(left, dict) and isinstance(right, dict):
            mismatches.extend(compare_dicts(left, right, name, ignored_keys))
        elif left != right:
            mismatches.append(f"{name}: expected {left!r}, got {right!r}")
    return mismatches


def print_summary(info: PeReferenceInfo) -> None:
    print(f"Reference: {info.path}")
    print(f"Size: {info.size}")
    print(f"SHA-256: {info.sha256}")
    print(f"Machine: {info.machine}")
    print(f"Image base: {info.image_base}")
    print(f"Entry point: {info.entry_point_va} (RVA {info.entry_point_rva})")
    print(f"Sections: {', '.join(section.name for section in info.sections)}")
    print("Imports:")
    for import_info in info.imports:
        print(f"- {import_info.dll}: {import_info.count}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Inspect and verify the original Recoil PE.")
    parser.add_argument("--reference", default="support/Recoil.exe", help="Original executable path.")
    parser.add_argument("--manifest", default=".agent/REFERENCE_EXECUTABLE.json")
    parser.add_argument("--verify", action="store_true", help="Compare reference facts to manifest.")
    parser.add_argument("--candidate", help="Optional rebuilt executable to compare to the reference.")
    parser.add_argument("--write-manifest", action="store_true", help="Write manifest JSON for reference.")
    args = parser.parse_args(argv)

    try:
        reference_info = parse_pe(Path(args.reference))
        reference_data = manifest_dict(reference_info)
        manifest_path = Path(args.manifest)

        if args.write_manifest:
            manifest_path.write_text(
                json.dumps(reference_data, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            print(f"Wrote {manifest_path}")

        print_summary(reference_info)

        if args.verify:
            expected = read_manifest(manifest_path)
            mismatches = compare_dicts(expected, reference_data)
            if mismatches:
                print("Manifest verification failed:", file=sys.stderr)
                for mismatch in mismatches:
                    print(f"- {mismatch}", file=sys.stderr)
                return 1
            print(f"Manifest verified: {manifest_path}")

        if args.candidate:
            candidate_info = parse_pe(Path(args.candidate))
            mismatches = compare_dicts(
                reference_data,
                manifest_dict(candidate_info),
                ignored_keys={"path"},
            )
            if mismatches:
                print("Candidate differs from reference:")
                for mismatch in mismatches:
                    print(f"- {mismatch}")
                return 1
            print("Candidate matches reference PE facts.")
    except (OSError, PeFormatError, json.JSONDecodeError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
