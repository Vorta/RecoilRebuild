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

from _recoil.lib.pe import PeFormatError, PeSection, data_directory, hex32, parse_pe_headers, read_c_string, rva_to_offset


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
class ExportInfo:
    ordinal: int
    name: str
    rva: str


@dataclass(frozen=True)
class PeReferenceInfo:
    path: str
    size: int
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
    exports: list[ExportInfo]
    imports: list[ImportInfo]


def parse_imports(data: bytes, import_rva: int, sections: tuple[PeSection, ...]) -> list[ImportInfo]:
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


def parse_exports(data: bytes, export_rva: int, sections: tuple[PeSection, ...]) -> list[ExportInfo]:
    if export_rva == 0:
        return []
    export_offset = rva_to_offset(export_rva, sections)
    if export_offset is None:
        raise PeFormatError(f"export directory RVA cannot be mapped: {hex32(export_rva)}")

    (
        _characteristics,
        _time_date,
        _major,
        _minor,
        _name_rva,
        ordinal_base,
        function_count,
        name_count,
        address_of_functions,
        address_of_names,
        address_of_name_ordinals,
    ) = struct.unpack_from("<IIHHIIIIIII", data, export_offset)

    functions_offset = rva_to_offset(address_of_functions, sections)
    names_offset = rva_to_offset(address_of_names, sections)
    ordinals_offset = rva_to_offset(address_of_name_ordinals, sections)
    if functions_offset is None or names_offset is None or ordinals_offset is None:
        raise PeFormatError(f"export table arrays cannot be mapped: {hex32(export_rva)}")

    exports: list[ExportInfo] = []
    for index in range(name_count):
        name_rva = struct.unpack_from("<I", data, names_offset + index * 4)[0]
        name_offset = rva_to_offset(name_rva, sections)
        if name_offset is None:
            raise PeFormatError(f"export name RVA cannot be mapped: {hex32(name_rva)}")
        ordinal_index = struct.unpack_from("<H", data, ordinals_offset + index * 2)[0]
        if ordinal_index >= function_count:
            raise PeFormatError(f"export ordinal index out of range: {ordinal_index}")
        function_rva = struct.unpack_from("<I", data, functions_offset + ordinal_index * 4)[0]
        exports.append(
            ExportInfo(
                ordinal=ordinal_base + ordinal_index,
                name=read_c_string(data, name_offset),
                rva=hex32(function_rva),
            )
        )

    exports.sort(key=lambda item: (item.ordinal, item.name))
    return exports


def parse_pe(path: Path) -> PeReferenceInfo:
    data = path.read_bytes()
    headers = parse_pe_headers(data, source=str(path))

    sections: list[SectionInfo] = []
    for section in headers.sections:
        sections.append(
            SectionInfo(
                name=section.name,
                virtual_address=hex32(section.virtual_address),
                virtual_size=hex32(section.virtual_size),
                raw_pointer=hex32(section.raw_pointer),
                raw_size=hex32(section.raw_size),
                characteristics=hex32(section.characteristics),
            )
        )

    directories: list[DataDirectoryInfo] = []
    for directory in headers.data_directories:
        directories.append(
            DataDirectoryInfo(
                name=directory.name,
                rva=hex32(directory.rva),
                size=hex32(directory.size),
                file_offset=hex32(directory.file_offset) if directory.file_offset is not None else None,
            )
        )

    export_rva = data_directory(headers, 0).rva
    import_rva = data_directory(headers, 1).rva

    return PeReferenceInfo(
        path=str(path),
        size=len(data),
        pe_offset=hex32(headers.pe_offset),
        machine=hex32(headers.machine),
        section_count=headers.section_count,
        timestamp=headers.timestamp,
        characteristics=hex32(headers.characteristics),
        optional_header_magic=hex32(headers.optional_header_magic),
        entry_point_rva=hex32(headers.entry_point_rva),
        entry_point_va=hex32(headers.image_base + headers.entry_point_rva),
        image_base=hex32(headers.image_base),
        section_alignment=hex32(headers.section_alignment),
        file_alignment=hex32(headers.file_alignment),
        subsystem=headers.subsystem,
        size_of_image=hex32(headers.size_of_image),
        checksum=hex32(headers.checksum),
        number_of_rva_and_sizes=headers.number_of_rva_and_sizes,
        sections=sections,
        data_directories=directories,
        exports=parse_exports(data, export_rva, headers.sections),
        imports=parse_imports(data, import_rva, headers.sections),
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
    print(f"Machine: {info.machine}")
    print(f"Image base: {info.image_base}")
    print(f"Entry point: {info.entry_point_va} (RVA {info.entry_point_rva})")
    print(f"Sections: {', '.join(section.name for section in info.sections)}")
    print("Exports:")
    for export_info in info.exports:
        print(f"- {export_info.ordinal}: {export_info.name} @ {export_info.rva}")
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
                # The linker-written COFF TimeDateStamp is diagnostic only for
                # a fresh unrestricted candidate. Every other parsed PE fact,
                # including the PE CheckSum field, remains exact.
                ignored_keys={"path", "timestamp"},
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
