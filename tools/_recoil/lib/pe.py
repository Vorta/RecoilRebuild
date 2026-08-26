from __future__ import annotations

from dataclasses import dataclass
import struct


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
class PeSection:
    name: str
    virtual_address: int
    virtual_size: int
    raw_pointer: int
    raw_size: int
    characteristics: int


@dataclass(frozen=True)
class PeDataDirectory:
    name: str
    rva: int
    size: int
    file_offset: int | None


@dataclass(frozen=True)
class PeHeaders:
    pe_offset: int
    machine: int
    section_count: int
    timestamp: int
    characteristics: int
    optional_header_magic: int
    entry_point_rva: int
    image_base: int
    section_alignment: int
    file_alignment: int
    subsystem: int
    size_of_image: int
    checksum: int
    number_of_rva_and_sizes: int
    sections: tuple[PeSection, ...]
    data_directories: tuple[PeDataDirectory, ...]


class PeFormatError(ValueError):
    pass


def hex32(value: int) -> str:
    return f"0x{value:x}"


def read_c_string(data: bytes, offset: int) -> str:
    end = data.index(b"\0", offset)
    return data[offset:end].decode("ascii", errors="replace")


def rva_to_offset(rva: int, sections: tuple[PeSection, ...] | list[PeSection]) -> int | None:
    for section in sections:
        size = max(section.virtual_size, section.raw_size)
        if section.virtual_address <= rva < section.virtual_address + size:
            return section.raw_pointer + (rva - section.virtual_address)
    return None


def parse_pe_headers(data: bytes, *, source: str = "PE image") -> PeHeaders:
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise PeFormatError(f"not a DOS/PE executable: {source}")
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
    sections: list[PeSection] = []
    for index in range(section_count):
        current = section_offset + index * 40
        name = data[current : current + 8].rstrip(b"\0").decode("ascii", errors="replace")
        virtual_size, virtual_address, raw_size, raw_pointer, _reloc, _line, _nrel, _nline, sect_chars = (
            struct.unpack_from("<IIIIIIHHI", data, current + 8)
        )
        sections.append(
            PeSection(
                name=name,
                virtual_address=virtual_address,
                virtual_size=virtual_size,
                raw_pointer=raw_pointer,
                raw_size=raw_size,
                characteristics=sect_chars,
            )
        )

    directories: list[PeDataDirectory] = []
    directory_count = min(number_of_rva_and_sizes, len(DIRECTORY_NAMES))
    for index in range(directory_count):
        rva, size = struct.unpack_from("<II", data, optional_offset + 96 + index * 8)
        file_offset = rva_to_offset(rva, sections) if rva else None
        directories.append(
            PeDataDirectory(
                name=DIRECTORY_NAMES[index],
                rva=rva,
                size=size,
                file_offset=file_offset,
            )
        )

    return PeHeaders(
        pe_offset=pe_offset,
        machine=machine,
        section_count=section_count,
        timestamp=timestamp,
        characteristics=characteristics,
        optional_header_magic=optional_magic,
        entry_point_rva=entry_point_rva,
        image_base=image_base,
        section_alignment=section_alignment,
        file_alignment=file_alignment,
        subsystem=subsystem,
        size_of_image=size_of_image,
        checksum=checksum,
        number_of_rva_and_sizes=number_of_rva_and_sizes,
        sections=tuple(sections),
        data_directories=tuple(directories),
    )


def data_directory(headers: PeHeaders, index: int) -> PeDataDirectory:
    if index < 0 or index >= len(headers.data_directories):
        return PeDataDirectory(
            name=DIRECTORY_NAMES[index] if 0 <= index < len(DIRECTORY_NAMES) else f"directory_{index}",
            rva=0,
            size=0,
            file_offset=None,
        )
    return headers.data_directories[index]
