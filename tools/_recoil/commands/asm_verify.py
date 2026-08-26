from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from dataclasses import dataclass
from difflib import SequenceMatcher, unified_diff
from pathlib import Path
import re
import struct
import sys

from _recoil.lib.binja import DEFAULT_BRIDGE_URL, BinaryNinjaBridge, BridgeError
from _recoil.lib.owner_entries import normalize_address


HEX_BYTE_RE = re.compile(r"^[0-9A-Fa-f]{2}$")
BN_ADDR_RE = re.compile(r"^[0-9A-Fa-f]{8}$")
COD_OFFSET_RE = re.compile(r"^[0-9A-Fa-f]{5}$")
DECIMAL_RE = re.compile(r"^-?\d+$")
HEX_RE = re.compile(r"^0x[0-9a-fA-F]+$")
COD_C_SYMBOL_RE = re.compile(r"(?<![\w?$@])_[A-Za-z][A-Za-z0-9_]*(?:@[0-9]+)?(?:\+\d+)?(?![\w$])")
HEXDUMP_BYTES_RE = re.compile(r"^\s*[0-9A-Fa-f]{6,8}:?\s+(.*)$")


@dataclass(frozen=True)
class Instruction:
    text: str
    raw_text: str
    bytes: tuple[str, ...]
    source_line: str


@dataclass(frozen=True)
class AssemblyComparison:
    address: str
    bn_path: Path
    cod_path: Path
    diff_path: Path
    classified_path: Path
    diff_count: int
    mismatch_count: int


@dataclass(frozen=True)
class CoffSection:
    index: int
    name: str
    raw_data: bytes
    relocation_offset: int
    relocation_count: int
    characteristics: int


@dataclass(frozen=True)
class CoffSymbol:
    index: int
    name: str
    value: int
    section_number: int
    type: int
    storage_class: int
    aux_count: int
    aux_records: tuple[bytes, ...] = ()

    @property
    def weak_external_tag_index(self) -> int | None:
        if self.storage_class != 105 or not self.aux_records:
            return None
        return struct.unpack_from("<I", self.aux_records[0], 0)[0]

    @property
    def weak_external_characteristics(self) -> int | None:
        if self.storage_class != 105 or not self.aux_records:
            return None
        return struct.unpack_from("<I", self.aux_records[0], 4)[0]

    @property
    def section_definition_selection(self) -> int | None:
        if self.storage_class != IMAGE_SYM_CLASS_STATIC or self.section_number <= 0 or not self.aux_records:
            return None
        return self.aux_records[0][14]

    @property
    def section_definition_association(self) -> int | None:
        if self.storage_class != IMAGE_SYM_CLASS_STATIC or self.section_number <= 0 or not self.aux_records:
            return None
        return struct.unpack_from("<H", self.aux_records[0], 12)[0]


@dataclass(frozen=True)
class CoffRelocation:
    offset: int
    symbol_index: int
    type: int
    symbol_name: str


@dataclass(frozen=True)
class CoffFunctionBytes:
    symbol: str
    section_name: str
    section_index: int
    start: int
    end: int
    natural_end: int
    data: bytes
    relocations: tuple[CoffRelocation, ...]
    relocation_mask: tuple[bool, ...]
    excluded_tail_relocation_count: int

    @property
    def excluded_tail_size(self) -> int:
        return self.natural_end - self.end


@dataclass(frozen=True)
class CoffDataSymbolBytes:
    symbol: str
    section_name: str
    section_index: int
    symbol_start: int
    object_offset: int
    start: int
    end: int
    data: bytes
    relocations: tuple[CoffRelocation, ...]
    relocation_mask: tuple[bool, ...]


@dataclass(frozen=True)
class ByteMismatch:
    offset: int
    bn_byte: int | None
    vc5_byte: int | None
    masked: bool = False


@dataclass(frozen=True)
class MaskedByteComparison:
    bn_size: int
    vc5_size: int
    compared_size: int
    relocation_masked_bytes: int
    trailing_bn_nops_trimmed: int
    trailing_vc5_nops_trimmed: int
    mismatches: tuple[ByteMismatch, ...]

    @property
    def mismatch_count(self) -> int:
        return len(self.mismatches)


@dataclass(frozen=True)
class ObjectByteComparison:
    address: str
    symbol: str
    obj_path: Path
    bn_path: Path
    vc5_path: Path
    mask_path: Path
    diff_path: Path
    triage_path: Path
    text_diff_path: Path | None
    classified_text_path: Path | None
    mismatch_count: int
    relocation_masked_bytes: int
    bn_size: int
    vc5_size: int
    trailing_bn_nops_trimmed: int
    trailing_vc5_nops_trimmed: int
    relocation_identity_path: Path | None = None
    natural_vc5_size: int = 0
    excluded_vc5_tail_bytes: int = 0
    excluded_vc5_tail_relocations: int = 0


def normalize_pointer_spelling(text: str) -> str:
    text = re.sub(r"\bBYTE PTR\b", "byte", text, flags=re.IGNORECASE)
    text = re.sub(r"\bWORD PTR\b", "word", text, flags=re.IGNORECASE)
    text = re.sub(r"\bDWORD PTR\b", "dword", text, flags=re.IGNORECASE)
    text = re.sub(r"\bSHORT\b", "", text, flags=re.IGNORECASE)
    return " ".join(text.split())


def register_width_bits(operand: str) -> int | None:
    operand = operand.strip().lower()
    if operand.startswith("byte "):
        return 8
    if operand.startswith("word "):
        return 16
    if operand.startswith("dword "):
        return 32
    if operand in {"al", "ah", "bl", "bh", "cl", "ch", "dl", "dh"}:
        return 8
    if operand in {"ax", "bx", "cx", "dx", "si", "di", "sp", "bp"}:
        return 16
    if operand in {"eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp"}:
        return 32
    return None


def format_immediate(value: int, width_bits: int | None = None) -> str:
    if value < 0:
        if width_bits is None:
            width_bits = 32
        value &= (1 << width_bits) - 1
    return f"0x{value:x}"


def normalize_immediate_operand(operand: str, width_bits: int | None) -> str:
    stripped = operand.strip()
    if DECIMAL_RE.match(stripped):
        return format_immediate(int(stripped, 10), width_bits)
    if HEX_RE.match(stripped):
        return format_immediate(int(stripped, 16), width_bits)
    return operand


def normalize_numeric_operands(text: str) -> str:
    if " " not in text:
        return text

    mnemonic, operand_text = text.split(" ", 1)
    mnemonic_lower = mnemonic.lower()
    operands = [operand.strip() for operand in operand_text.split(",")]
    if mnemonic_lower in {"ret", "retn"}:
        if not operands or operands == [""] or operands == ["0"] or operands == ["0x0"]:
            return "retn"
        return f"retn {normalize_immediate_operand(operands[0], 16)}"

    if not operands:
        return text
    width_bits = register_width_bits(operands[0])
    normalized_operands: list[str] = []
    for index, operand in enumerate(operands):
        normalized_operands.append(normalize_immediate_operand(operand, width_bits if index > 0 else None))
    return f"{mnemonic_lower} {', '.join(normalized_operands)}"


def normalize_instruction_text(parts: list[str]) -> tuple[str, str]:
    raw_text = " ".join(parts).replace("\t", " ")
    raw_text = normalize_pointer_spelling(raw_text)
    text = normalize_numeric_operands(raw_text)
    return text, raw_text


def normalize_instruction_line(line: str, *, source: str) -> str | None:
    instructions = parse_assembly(line, source=source)
    if not instructions:
        return None
    return instructions[0].text


def parse_assembly(text: str, *, source: str) -> list[Instruction]:
    result: list[Instruction] = []
    pending_cod_bytes: list[str] = []
    for line in text.splitlines():
        parsed = parse_instruction_record(line, source=source, pending_cod_bytes=pending_cod_bytes)
        if parsed is not None:
            result.append(parsed)
    return result


def parse_instruction_record(line: str, *, source: str, pending_cod_bytes: list[str]) -> Instruction | None:
    source_line = line.rstrip("\r\n")
    line = line.split(";", 1)[0].strip()
    if not line:
        return None
    if line.startswith("#"):
        return None
    if source == "cod" and (
        line.startswith("PUBLIC")
        or line.startswith("_TEXT")
        or line.startswith("INCLUDE")
        or line.startswith("INCLUDELIB")
        or line.startswith("TITLE")
        or line.startswith(".")
        or line.startswith("END")
    ):
        return None

    parts = line.split()
    if not parts:
        return None

    if source == "bn":
        if not BN_ADDR_RE.match(parts[0]):
            return None
        parts = parts[1:]
        byte_parts: list[str] = []
        while parts and HEX_BYTE_RE.match(parts[0]):
            byte_parts.append(parts.pop(0).lower())
    elif source == "cod":
        if parts[0].endswith("PROC") or parts[0].endswith("ENDP"):
            return None
        if COD_OFFSET_RE.match(parts[0]):
            parts = parts[1:]
            byte_parts = []
            while parts and HEX_BYTE_RE.match(parts[0]):
                byte_parts.append(parts.pop(0).lower())
        elif HEX_BYTE_RE.match(parts[0]):
            byte_parts = []
            while parts and HEX_BYTE_RE.match(parts[0]):
                byte_parts.append(parts.pop(0).lower())
        else:
            return None
        if not parts:
            pending_cod_bytes.extend(byte_parts)
            return None
        if pending_cod_bytes:
            byte_parts = [*pending_cod_bytes, *byte_parts]
            pending_cod_bytes.clear()
    else:
        return None

    if not parts:
        return None
    normalized, raw_text = normalize_instruction_text(parts)
    return Instruction(
        text=normalized,
        raw_text=raw_text,
        bytes=tuple(byte_parts),
        source_line=source_line,
    )


def normalize_assembly(text: str, *, source: str) -> list[str]:
    return [instruction.text for instruction in parse_assembly(text, source=source)]


COFF_FILE_HEADER_SIZE = 20
COFF_SECTION_HEADER_SIZE = 40
COFF_SYMBOL_SIZE = 18
COFF_RELOCATION_SIZE = 10
IMAGE_SCN_CNT_CODE = 0x00000020
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_REL_I386_DIR32 = 0x0006
IMAGE_REL_I386_DIR32NB = 0x0007
IMAGE_REL_I386_REL32 = 0x0014
IMAGE_REL_I386_SECTION = 0x000A
IMAGE_REL_I386_SECREL = 0x000B
IMAGE_SYM_CLASS_EXTERNAL = 2
IMAGE_SYM_CLASS_STATIC = 3


COFF_RELOCATION_TYPE_NAMES = {
    IMAGE_REL_I386_DIR32: "DIR32",
    IMAGE_REL_I386_DIR32NB: "DIR32NB",
    IMAGE_REL_I386_REL32: "REL32",
    IMAGE_REL_I386_SECTION: "SECTION",
    IMAGE_REL_I386_SECREL: "SECREL",
}


COFF_RELOCATION_SIZES = {
    IMAGE_REL_I386_DIR32: 4,
    IMAGE_REL_I386_DIR32NB: 4,
    IMAGE_REL_I386_REL32: 4,
    IMAGE_REL_I386_SECTION: 2,
    IMAGE_REL_I386_SECREL: 4,
}


def relocation_type_name(relocation_type: int) -> str:
    return COFF_RELOCATION_TYPE_NAMES.get(relocation_type, f"0x{relocation_type:04x}")


def relocation_size(relocation_type: int) -> int:
    try:
        return COFF_RELOCATION_SIZES[relocation_type]
    except KeyError as exc:
        raise ValueError(f"Unsupported COFF relocation type: {relocation_type_name(relocation_type)}") from exc


def decode_coff_name(raw_name: bytes, string_table: bytes) -> str:
    if len(raw_name) != 8:
        raise ValueError("COFF name fields must be 8 bytes")
    if raw_name[:4] == b"\x00\x00\x00\x00":
        offset = struct.unpack_from("<I", raw_name, 4)[0]
        if offset < 4 or offset >= len(string_table):
            raise ValueError(f"COFF string-table offset out of range: {offset}")
        end = string_table.find(b"\x00", offset)
        if end == -1:
            end = len(string_table)
        return string_table[offset:end].decode("utf-8", errors="replace")
    return raw_name.split(b"\x00", 1)[0].decode("ascii", errors="replace")


def decode_coff_section_name(raw_name: bytes, string_table: bytes) -> str:
    short_name = raw_name.split(b"\x00", 1)[0]
    if short_name.startswith(b"/") and len(short_name) > 1:
        try:
            offset = int(short_name[1:].decode("ascii"))
        except ValueError:
            pass
        else:
            if 4 <= offset < len(string_table):
                end = string_table.find(b"\x00", offset)
                if end == -1:
                    end = len(string_table)
                return string_table[offset:end].decode("utf-8", errors="replace")
    return decode_coff_name(raw_name, string_table)


class CoffObject:
    def __init__(
        self,
        *,
        sections: tuple[CoffSection, ...],
        symbols: tuple[CoffSymbol, ...],
        symbols_by_index: dict[int, CoffSymbol],
        relocations_by_section: dict[int, tuple[CoffRelocation, ...]],
    ) -> None:
        self.sections = sections
        self.symbols = symbols
        self.symbols_by_index = symbols_by_index
        self.relocations_by_section = relocations_by_section
        self.symbols_by_name = {symbol.name: symbol for symbol in symbols}

    @classmethod
    def from_path(cls, path: Path) -> "CoffObject":
        return cls.from_bytes(path.read_bytes())

    @classmethod
    def from_bytes(cls, data: bytes) -> "CoffObject":
        if len(data) < COFF_FILE_HEADER_SIZE:
            raise ValueError("COFF file is too small for a file header")

        (
            _machine,
            section_count,
            _timestamp,
            symbol_table_offset,
            symbol_count,
            optional_header_size,
            _characteristics,
        ) = struct.unpack_from("<HHIIIHH", data, 0)

        section_table_offset = COFF_FILE_HEADER_SIZE + optional_header_size
        symbol_table_size = symbol_count * COFF_SYMBOL_SIZE
        string_table_offset = symbol_table_offset + symbol_table_size
        if string_table_offset + 4 > len(data):
            raise ValueError("COFF string table is missing")
        string_table_size = struct.unpack_from("<I", data, string_table_offset)[0]
        if string_table_size < 4 or string_table_offset + string_table_size > len(data):
            raise ValueError("COFF string table size is invalid")
        string_table = data[string_table_offset : string_table_offset + string_table_size]

        sections: list[CoffSection] = []
        for index in range(section_count):
            offset = section_table_offset + index * COFF_SECTION_HEADER_SIZE
            if offset + COFF_SECTION_HEADER_SIZE > len(data):
                raise ValueError("COFF section table is truncated")
            raw_name = data[offset : offset + 8]
            name = decode_coff_section_name(raw_name, string_table)
            raw_size = struct.unpack_from("<I", data, offset + 16)[0]
            raw_offset = struct.unpack_from("<I", data, offset + 20)[0]
            relocation_offset = struct.unpack_from("<I", data, offset + 24)[0]
            relocation_count = struct.unpack_from("<H", data, offset + 32)[0]
            characteristics = struct.unpack_from("<I", data, offset + 36)[0]
            raw_data_is_file_backed = raw_size == 0 or (raw_offset != 0 and raw_offset + raw_size <= len(data))
            if raw_data_is_file_backed:
                raw_data = data[raw_offset : raw_offset + raw_size]
            elif characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA:
                raw_data = b"\x00" * raw_size
            else:
                raise ValueError(f"COFF section {name} raw data is truncated")
            sections.append(
                CoffSection(
                    index=index + 1,
                    name=name,
                    raw_data=raw_data,
                    relocation_offset=relocation_offset,
                    relocation_count=relocation_count,
                    characteristics=characteristics,
                )
            )

        symbols: list[CoffSymbol] = []
        symbols_by_index: dict[int, CoffSymbol] = {}
        symbol_index = 0
        while symbol_index < symbol_count:
            offset = symbol_table_offset + symbol_index * COFF_SYMBOL_SIZE
            if offset + COFF_SYMBOL_SIZE > len(data):
                raise ValueError("COFF symbol table is truncated")
            name = decode_coff_name(data[offset : offset + 8], string_table)
            value = struct.unpack_from("<I", data, offset + 8)[0]
            section_number = struct.unpack_from("<h", data, offset + 12)[0]
            symbol_type = struct.unpack_from("<H", data, offset + 14)[0]
            storage_class = data[offset + 16]
            aux_count = data[offset + 17]
            aux_records: list[bytes] = []
            for aux_index in range(aux_count):
                aux_offset = offset + (aux_index + 1) * COFF_SYMBOL_SIZE
                if aux_offset + COFF_SYMBOL_SIZE > string_table_offset:
                    raise ValueError("COFF auxiliary symbol table is truncated")
                aux_records.append(data[aux_offset : aux_offset + COFF_SYMBOL_SIZE])
            symbol = CoffSymbol(
                index=symbol_index,
                name=name,
                value=value,
                section_number=section_number,
                type=symbol_type,
                storage_class=storage_class,
                aux_count=aux_count,
                aux_records=tuple(aux_records),
            )
            symbols.append(symbol)
            symbols_by_index[symbol_index] = symbol
            symbol_index += 1 + aux_count

        relocations_by_section: dict[int, tuple[CoffRelocation, ...]] = {}
        for section in sections:
            relocations: list[CoffRelocation] = []
            for relocation_index in range(section.relocation_count):
                offset = section.relocation_offset + relocation_index * COFF_RELOCATION_SIZE
                if offset + COFF_RELOCATION_SIZE > len(data):
                    raise ValueError(f"COFF relocation table for {section.name} is truncated")
                relocation_address, relocation_symbol_index, relocation_type = struct.unpack_from("<IIH", data, offset)
                relocation_symbol = symbols_by_index.get(relocation_symbol_index)
                symbol_name = relocation_symbol.name if relocation_symbol is not None else f"<symbol {relocation_symbol_index}>"
                relocations.append(
                    CoffRelocation(
                        offset=relocation_address,
                        symbol_index=relocation_symbol_index,
                        type=relocation_type,
                        symbol_name=symbol_name,
                    )
                )
            relocations_by_section[section.index] = tuple(relocations)

        return cls(
            sections=tuple(sections),
            symbols=tuple(symbols),
            symbols_by_index=symbols_by_index,
            relocations_by_section=relocations_by_section,
        )

    def section(self, section_number: int) -> CoffSection:
        if section_number <= 0 or section_number > len(self.sections):
            raise ValueError(f"COFF section number out of range: {section_number}")
        return self.sections[section_number - 1]

    def function_bytes(
        self,
        symbol_name: str,
        *,
        byte_length: int | None = None,
    ) -> CoffFunctionBytes:
        symbol = self.symbols_by_name.get(symbol_name)
        if symbol is None:
            raise ValueError(f"Symbol not found in COFF object: {symbol_name}")
        section = self.section(symbol.section_number)
        if (section.characteristics & IMAGE_SCN_CNT_CODE) == 0:
            raise ValueError(f"Symbol is not in a code section: {symbol_name}")
        start = symbol.value
        if start >= len(section.raw_data):
            raise ValueError(f"Symbol starts outside its section: {symbol_name}")
        natural_end = self.function_end(symbol, section)
        if byte_length is None:
            end = natural_end
        else:
            if byte_length <= 0:
                raise ValueError(f"Function byte length must be positive: {symbol_name}")
            end = start + byte_length
            if end > natural_end:
                raise ValueError(
                    f"Function byte range extends past natural COFF extent: {symbol_name} "
                    f"0x{start:x}..0x{end:x} natural_end=0x{natural_end:x}"
                )
        function_data = section.raw_data[start:end]
        relocation_mask = [False] * len(function_data)
        function_relocations: list[CoffRelocation] = []
        excluded_tail_relocation_count = 0
        for relocation in self.relocations_by_section.get(section.index, ()):
            size = relocation_size(relocation.type)
            if relocation.offset < start or relocation.offset >= natural_end:
                continue
            natural_relative_offset = relocation.offset - start
            if natural_relative_offset + size > natural_end - start:
                raise ValueError(
                    f"Relocation at 0x{relocation.offset:x} extends past {symbol_name} natural COFF extent"
                )
            if relocation.offset >= end:
                excluded_tail_relocation_count += 1
                continue
            relative_offset = relocation.offset - start
            if relative_offset + size > len(relocation_mask):
                raise ValueError(
                    f"Relocation at 0x{relocation.offset:x} extends past {symbol_name} byte range"
                )
            for index in range(relative_offset, relative_offset + size):
                relocation_mask[index] = True
            function_relocations.append(relocation)
        return CoffFunctionBytes(
            symbol=symbol_name,
            section_name=section.name,
            section_index=section.index,
            start=start,
            end=end,
            natural_end=natural_end,
            data=function_data,
            relocations=tuple(function_relocations),
            relocation_mask=tuple(relocation_mask),
            excluded_tail_relocation_count=excluded_tail_relocation_count,
        )

    def data_symbol_bytes(
        self,
        symbol_name: str,
        *,
        byte_length: int | None = None,
        object_offset: int = 0,
    ) -> CoffDataSymbolBytes:
        symbol = self.symbols_by_name.get(symbol_name)
        if symbol is None:
            raise ValueError(f"Symbol not found in COFF object: {symbol_name}")
        section = self.section(symbol.section_number)
        if section.characteristics & IMAGE_SCN_CNT_CODE:
            raise ValueError(f"Symbol is in a code section, not a data section: {symbol_name}")
        if isinstance(object_offset, bool) or not isinstance(object_offset, int) or object_offset < 0:
            raise ValueError(f"Data symbol object offset must be a non-negative integer: {symbol_name}")
        symbol_start = symbol.value
        natural_end = self.symbol_end(symbol, section)
        start = symbol_start + object_offset
        if start >= len(section.raw_data):
            raise ValueError(f"Symbol starts outside its section: {symbol_name}")
        if object_offset and start >= natural_end:
            raise ValueError(
                f"Data symbol object offset is outside its natural COFF extent: {symbol_name} "
                f"offset=0x{object_offset:x} natural_size=0x{natural_end - symbol_start:x}"
            )
        if byte_length is None:
            end = natural_end
        else:
            if byte_length <= 0:
                raise ValueError(f"Data symbol byte length must be positive: {symbol_name}")
            end = start + byte_length
        if end > len(section.raw_data):
            raise ValueError(
                f"Data symbol range extends outside section: {symbol_name} "
                f"0x{start:x}..0x{end:x} section_size=0x{len(section.raw_data):x}"
            )
        if object_offset and end > natural_end:
            raise ValueError(
                f"Data symbol slice extends past its natural COFF extent: {symbol_name} "
                f"0x{start:x}..0x{end:x} natural_end=0x{natural_end:x}"
            )
        symbol_data = section.raw_data[start:end]
        relocation_mask = [False] * len(symbol_data)
        symbol_relocations: list[CoffRelocation] = []
        for relocation in self.relocations_by_section.get(section.index, ()):
            size = relocation_size(relocation.type)
            relocation_end = relocation.offset + size
            if relocation_end <= start or relocation.offset >= end:
                continue
            if relocation.offset < start or relocation_end > end:
                raise ValueError(
                    f"Relocation at 0x{relocation.offset:x} crosses {symbol_name} data slice boundary "
                    f"0x{start:x}..0x{end:x}"
                )
            relative_offset = relocation.offset - start
            for index in range(relative_offset, relative_offset + size):
                relocation_mask[index] = True
            symbol_relocations.append(relocation)
        return CoffDataSymbolBytes(
            symbol=symbol_name,
            section_name=section.name,
            section_index=section.index,
            symbol_start=symbol_start,
            object_offset=object_offset,
            start=start,
            end=end,
            data=symbol_data,
            relocations=tuple(symbol_relocations),
            relocation_mask=tuple(relocation_mask),
        )

    def function_end(self, symbol: CoffSymbol, section: CoffSection) -> int:
        return self.symbol_end(symbol, section)

    def symbol_end(self, symbol: CoffSymbol, section: CoffSection) -> int:
        candidates = [
            other.value
            for other in self.symbols
            if other.section_number == symbol.section_number
            and other.value > symbol.value
            and other.value <= len(section.raw_data)
            and other.storage_class in {IMAGE_SYM_CLASS_EXTERNAL, IMAGE_SYM_CLASS_STATIC}
            and other.name != section.name
            and not other.name.startswith(".")
        ]
        if candidates:
            return min(candidates)
        return len(section.raw_data)


def byte_string(instruction: Instruction) -> str:
    return " ".join(instruction.bytes) if instruction.bytes else "<no bytes>"


def mnemonic(text: str) -> str:
    return text.split(" ", 1)[0].lower()


REGISTER_RE = re.compile(
    r"\b(eax|ebx|ecx|edx|esi|edi|esp|ebp|ax|bx|cx|dx|si|di|sp|bp|"
    r"al|ah|bl|bh|cl|ch|dl|dh)\b",
    re.IGNORECASE,
)

JUMP_MNEMONICS = {
    "ja",
    "jae",
    "jb",
    "jbe",
    "jc",
    "je",
    "jg",
    "jge",
    "jl",
    "jle",
    "jmp",
    "jna",
    "jnae",
    "jnb",
    "jnbe",
    "jnc",
    "jne",
    "jng",
    "jnge",
    "jnl",
    "jnle",
    "jno",
    "jnp",
    "jns",
    "jnz",
    "jo",
    "jp",
    "jpe",
    "jpo",
    "js",
    "jz",
}


def normalize_decimal_literals_in_text(text: str) -> str:
    def replace_match(match: re.Match[str]) -> str:
        return format_immediate(int(match.group(0), 10))

    return re.sub(r"(?<![A-Za-z0-9_$])\d+(?![A-Za-z0-9_$])", replace_match, text)


def register_shape_text(text: str) -> str:
    text = normalize_decimal_literals_in_text(text.lower())

    def replace_register(match: re.Match[str]) -> str:
        width = register_width_bits(match.group(0))
        return f"r{width if width is not None else '?'}"

    return REGISTER_RE.sub(replace_register, text)


def has_symbol_relocation_text(instruction: Instruction) -> bool:
    raw_text = instruction.raw_text
    return (
        "?" in raw_text
        or "$L" in raw_text
        or "OFFSET " in raw_text.upper()
        or "__imp" in raw_text.lower()
        or COD_C_SYMBOL_RE.search(raw_text) is not None
    )


def is_branch_displacement_difference(bn_instruction: Instruction, cod_instruction: Instruction) -> bool:
    if not bn_instruction.bytes or not cod_instruction.bytes:
        return False
    if len(bn_instruction.bytes) != len(cod_instruction.bytes):
        return False
    if bn_instruction.bytes == cod_instruction.bytes:
        return False
    if mnemonic(bn_instruction.raw_text) != mnemonic(cod_instruction.raw_text):
        return False
    if mnemonic(bn_instruction.raw_text) not in JUMP_MNEMONICS:
        return False
    return bn_instruction.bytes[0] == cod_instruction.bytes[0]


def is_register_allocation_difference(bn_instruction: Instruction, cod_instruction: Instruction) -> bool:
    if not bn_instruction.bytes or not cod_instruction.bytes:
        return False
    if bn_instruction.bytes == cod_instruction.bytes:
        return False
    if mnemonic(bn_instruction.raw_text) != mnemonic(cod_instruction.raw_text):
        return False
    if not REGISTER_RE.search(bn_instruction.raw_text) or not REGISTER_RE.search(cod_instruction.raw_text):
        return False
    return register_shape_text(bn_instruction.text) == register_shape_text(cod_instruction.text)


def is_relocation_sensitive(bn_instruction: Instruction, cod_instruction: Instruction) -> bool:
    if not bn_instruction.bytes or not cod_instruction.bytes:
        return False
    if len(bn_instruction.bytes) != len(cod_instruction.bytes):
        return False
    if bn_instruction.bytes == cod_instruction.bytes:
        return False
    if mnemonic(bn_instruction.raw_text) != mnemonic(cod_instruction.raw_text):
        return False

    cod_bytes = cod_instruction.bytes
    bn_bytes = bn_instruction.bytes
    if cod_bytes[0] == bn_bytes[0] == "e8" and cod_bytes[1:] == ("00", "00", "00", "00"):
        return True
    if cod_bytes[0] == bn_bytes[0] == "e9" and cod_bytes[1:] == ("00", "00", "00", "00"):
        return True
    if cod_bytes[:2] == bn_bytes[:2] == ("ff", "15") and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[:2] == bn_bytes[:2] == ("ff", "25") and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[:2] == bn_bytes[:2] == ("ff", "05") and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[0] == bn_bytes[0] in {"68", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf"} and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[0] == bn_bytes[0] == "a3" and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[0] == bn_bytes[0] == "a1" and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[:2] == bn_bytes[:2] and cod_bytes[0] == "89" and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[:2] == bn_bytes[:2] and cod_bytes[0] == "c7" and cod_bytes[-4:] == ("00", "00", "00", "00"):
        return True
    if cod_bytes[:2] == bn_bytes[:2] and cod_bytes[0] == "8b" and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[:2] == bn_bytes[:2] and cod_bytes[0] == "39" and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[:2] == bn_bytes[:2] and cod_bytes[0] == "81" and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[:2] == bn_bytes[:2] == ("8b", "0d") and cod_bytes[2:] == ("00", "00", "00", "00"):
        return True
    if cod_bytes[:3] == bn_bytes[:3] == ("0f", "af", "05") and has_symbol_relocation_text(cod_instruction):
        return True
    return False


def is_stack_arg_vtable_load_schedule_pair(first: Instruction, second: Instruction) -> bool:
    """Accept VC5SP3/VC5 scheduling around a one-argument virtual call."""
    texts = {first.text, second.text}
    return "push 0x0" in texts and "mov eax, dword [ecx]" in texts


def instruction_alignment_key(instruction: Instruction) -> str:
    """Coarse diagnostic key for resyncing drift reports; not an acceptance rule."""
    instruction_mnemonic = mnemonic(instruction.raw_text)
    byte_prefix = instruction.bytes[0] if instruction.bytes else "no-bytes"
    if instruction_mnemonic in JUMP_MNEMONICS:
        return f"jump:{instruction_mnemonic}:{byte_prefix}:{len(instruction.bytes)}"
    if is_relocation_like_instruction(instruction):
        return f"reloc:{instruction_mnemonic}:{register_shape_text(instruction.text)}"
    if instruction.bytes:
        return f"bytes:{byte_string(instruction)}"
    if REGISTER_RE.search(instruction.raw_text):
        return f"shape:{instruction_mnemonic}:{register_shape_text(instruction.text)}"
    return f"text:{instruction.text}"


def is_relocation_like_instruction(instruction: Instruction) -> bool:
    if has_symbol_relocation_text(instruction):
        return True
    instruction_mnemonic = mnemonic(instruction.raw_text)
    if instruction_mnemonic in {"call", "jmp"} and instruction.bytes:
        return instruction.bytes[0] in {"e8", "e9"}
    return False


def append_instruction_block(
    lines: list[str],
    label: str,
    instructions: list[Instruction],
    start: int,
    end: int,
) -> None:
    lines.append(f"  {label} instructions {start + 1}-{end}:")
    for instruction in instructions[start:end]:
        lines.append(f"    {byte_string(instruction):<23} {instruction.raw_text}")


def classify_alignment_drift(
    bn_instructions: list[Instruction],
    cod_instructions: list[Instruction],
) -> tuple[list[str], int]:
    bn_keys = [instruction_alignment_key(instruction) for instruction in bn_instructions]
    cod_keys = [instruction_alignment_key(instruction) for instruction in cod_instructions]
    matcher = SequenceMatcher(None, bn_keys, cod_keys, autojunk=False)
    lines: list[str] = [
        "",
        "Diagnostic LCS alignment drift (shape-based, not acceptance evidence):",
    ]
    drift_blocks = 0
    equal_groups = 0
    for tag, bn_start, bn_end, cod_start, cod_end in matcher.get_opcodes():
        if tag == "equal":
            equal_groups += 1
            continue
        if tag == "replace" and (bn_end - bn_start) == (cod_end - cod_start):
            continue
        drift_blocks += 1
        lines.append(f"[alignment-{tag}] BN {bn_start + 1}-{bn_end}, COD {cod_start + 1}-{cod_end}")
        if bn_start != bn_end:
            append_instruction_block(lines, "BN", bn_instructions, bn_start, bn_end)
        if cod_start != cod_end:
            append_instruction_block(lines, "COD", cod_instructions, cod_start, cod_end)
    if drift_blocks == 0:
        lines.append("  no diagnostic alignment drift after shape-key resync")
    return [
        f"lcs_equal_instruction_groups: {equal_groups}",
        f"lcs_drift_blocks: {drift_blocks}",
        *lines,
    ], drift_blocks


def classify_instruction_differences(
    bn_instructions: list[Instruction], cod_instructions: list[Instruction]
) -> tuple[list[str], int]:
    lines: list[str] = []
    mismatches = 0
    max_len = max(len(bn_instructions), len(cod_instructions))
    exact = 0
    normalized_spelling = 0
    byte_identical = 0
    relocation_sensitive = 0
    schedule_equivalent = 0
    branch_displacement = 0
    register_allocation = 0

    index = 0
    while index < max_len:
        bn_instruction = bn_instructions[index] if index < len(bn_instructions) else None
        cod_instruction = cod_instructions[index] if index < len(cod_instructions) else None
        if bn_instruction is None or cod_instruction is None:
            mismatches += 1
            lines.append(f"[mismatch] instruction count diverges at index {index + 1}")
            lines.append(f"  BN:  {bn_instruction.text if bn_instruction else '<missing>'}")
            lines.append(f"  COD: {cod_instruction.text if cod_instruction else '<missing>'}")
            index += 1
            continue

        if bn_instruction.text == cod_instruction.text:
            if bn_instruction.raw_text == cod_instruction.raw_text:
                exact += 1
            else:
                normalized_spelling += 1
            index += 1
            continue

        next_bn = bn_instructions[index + 1] if index + 1 < len(bn_instructions) else None
        next_cod = cod_instructions[index + 1] if index + 1 < len(cod_instructions) else None
        if (
            next_bn is not None
            and next_cod is not None
            and bn_instruction.text == next_cod.text
            and next_bn.text == cod_instruction.text
            and is_stack_arg_vtable_load_schedule_pair(bn_instruction, next_bn)
            and is_stack_arg_vtable_load_schedule_pair(cod_instruction, next_cod)
        ):
            schedule_equivalent += 2
            lines.append(f"[schedule-equivalent] instructions {index + 1}-{index + 2}")
            lines.append("  accepted: stack argument push and vtable load are adjacent and independent")
            lines.append(f"  BN:        {bn_instruction.raw_text} / {next_bn.raw_text}")
            lines.append(f"  COD:       {cod_instruction.raw_text} / {next_cod.raw_text}")
            index += 2
            continue

        if bn_instruction.bytes and bn_instruction.bytes == cod_instruction.bytes:
            byte_identical += 1
            lines.append(f"[byte-identical spelling] instruction {index + 1}")
            lines.append(f"  bytes: {byte_string(bn_instruction)}")
            lines.append(f"  BN:    {bn_instruction.raw_text}")
            lines.append(f"  COD:   {cod_instruction.raw_text}")
            index += 1
            continue

        if is_relocation_sensitive(bn_instruction, cod_instruction):
            relocation_sensitive += 1
            lines.append(f"[relocation-sensitive] instruction {index + 1}")
            lines.append(f"  BN bytes:  {byte_string(bn_instruction)}")
            lines.append(f"  COD bytes: {byte_string(cod_instruction)}")
            lines.append(f"  BN:        {bn_instruction.raw_text}")
            lines.append(f"  COD:       {cod_instruction.raw_text}")
            index += 1
            continue

        if is_branch_displacement_difference(bn_instruction, cod_instruction):
            mismatches += 1
            branch_displacement += 1
            lines.append(f"[branch-displacement mismatch] instruction {index + 1}")
            lines.append(f"  BN bytes:  {byte_string(bn_instruction)}")
            lines.append(f"  COD bytes: {byte_string(cod_instruction)}")
            lines.append(f"  BN:        {bn_instruction.raw_text}")
            lines.append(f"  COD:       {cod_instruction.raw_text}")
            index += 1
            continue

        if is_register_allocation_difference(bn_instruction, cod_instruction):
            mismatches += 1
            register_allocation += 1
            lines.append(f"[register-allocation mismatch] instruction {index + 1}")
            lines.append(f"  BN bytes:  {byte_string(bn_instruction)}")
            lines.append(f"  COD bytes: {byte_string(cod_instruction)}")
            lines.append(f"  shape:     {register_shape_text(bn_instruction.text)}")
            lines.append(f"  BN:        {bn_instruction.raw_text}")
            lines.append(f"  COD:       {cod_instruction.raw_text}")
            index += 1
            continue

        mismatches += 1
        lines.append(f"[mismatch] instruction {index + 1}")
        lines.append(f"  BN bytes:  {byte_string(bn_instruction)}")
        lines.append(f"  COD bytes: {byte_string(cod_instruction)}")
        lines.append(f"  BN:        {bn_instruction.raw_text}")
        lines.append(f"  COD:       {cod_instruction.raw_text}")
        index += 1

    alignment_lines, alignment_drift_blocks = classify_alignment_drift(bn_instructions, cod_instructions)
    summary = [
        f"exact_or_normalized_matches: {exact + normalized_spelling}",
        f"  exact_text_matches: {exact}",
        f"  normalized_spelling_matches: {normalized_spelling}",
        f"byte_identical_spelling_differences: {byte_identical}",
        f"relocation_sensitive_differences: {relocation_sensitive}",
        f"schedule_equivalent_differences: {schedule_equivalent}",
        f"branch_displacement_mismatches: {branch_displacement}",
        f"register_allocation_mismatches: {register_allocation}",
        f"lcs_drift_blocks: {alignment_drift_blocks}",
        f"mismatches: {mismatches}",
        "",
    ]
    return [*summary, *lines, *alignment_lines], mismatches


def extract_cod_function(cod_text: str, symbol: str | None) -> str:
    if not symbol:
        return cod_text

    lines = cod_text.splitlines()
    begin = None
    end = None
    proc_markers = (f"{symbol} PROC", f"_{symbol} PROC")
    end_markers = (f"{symbol} ENDP", f"_{symbol} ENDP")
    for index, line in enumerate(lines):
        stripped = line.strip()
        if begin is None and any(marker in stripped for marker in proc_markers):
            begin = index
            continue
        if begin is not None and stripped.replace("\t", " ") == "_TEXT ENDS":
            end = index + 1
            break
        if begin is not None and any(marker in stripped for marker in end_markers):
            end = index + 1
            break

    if begin is None:
        raise ValueError(f"Symbol not found in COD listing: {symbol}")
    return "\n".join(lines[begin:end])


def write_lines(path: Path, lines: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")


def instructions_to_bytes(instructions: list[Instruction]) -> bytes:
    return bytes(int(byte, 16) for instruction in instructions for byte in instruction.bytes)


def bytes_from_hexdump(hexdump_text: str, *, expected_length: int | None = None) -> bytes:
    values: list[int] = []
    for line in hexdump_text.splitlines():
        match = HEXDUMP_BYTES_RE.match(line)
        if match is None:
            continue
        body = match.group(1)
        offset = 0
        line_values: list[int] = []
        while offset < len(body):
            whitespace_start = offset
            while offset < len(body) and body[offset].isspace():
                offset += 1
            if line_values and offset - whitespace_start >= 2:
                break
            if offset + 2 > len(body):
                break
            byte_text = body[offset : offset + 2]
            if not HEX_BYTE_RE.match(byte_text):
                break
            if offset + 2 < len(body) and not body[offset + 2].isspace():
                break
            line_values.append(int(byte_text, 16))
            offset += 2
        values.extend(line_values)
    data = bytes(values)
    if expected_length is not None and len(data) != expected_length:
        raise ValueError(f"BN hexdump yielded {len(data)} byte(s), expected {expected_length}")
    return data


def trim_trailing_nops(data: bytes, mask: tuple[bool, ...]) -> tuple[bytes, tuple[bool, ...], int]:
    if len(data) != len(mask):
        raise ValueError("Byte data and relocation mask lengths differ")
    trimmed = 0
    end = len(data)
    while end > 0 and data[end - 1] == 0x90 and not mask[end - 1]:
        end -= 1
        trimmed += 1
    return data[:end], mask[:end], trimmed


def compare_masked_byte_sequences(
    bn_bytes: bytes,
    vc5_bytes: bytes,
    vc5_relocation_mask: tuple[bool, ...],
    *,
    trim_padding_nops: bool = True,
) -> MaskedByteComparison:
    if len(vc5_bytes) != len(vc5_relocation_mask):
        raise ValueError("VC5SP3 byte data and relocation mask lengths differ")

    original_bn_size = len(bn_bytes)
    original_vc5_size = len(vc5_bytes)
    bn_mask = (False,) * len(bn_bytes)
    trailing_bn_nops = 0
    trailing_vc5_nops = 0
    if trim_padding_nops:
        bn_bytes, _bn_mask, trailing_bn_nops = trim_trailing_nops(bn_bytes, bn_mask)
        vc5_bytes, vc5_relocation_mask, trailing_vc5_nops = trim_trailing_nops(vc5_bytes, vc5_relocation_mask)

    mismatches: list[ByteMismatch] = []
    compared_size = max(len(bn_bytes), len(vc5_bytes))
    relocation_masked_bytes = 0
    for offset in range(compared_size):
        bn_byte = bn_bytes[offset] if offset < len(bn_bytes) else None
        vc5_byte = vc5_bytes[offset] if offset < len(vc5_bytes) else None
        masked = offset < len(vc5_relocation_mask) and vc5_relocation_mask[offset]
        if masked and bn_byte is not None and vc5_byte is not None:
            relocation_masked_bytes += 1
            continue
        if bn_byte != vc5_byte:
            mismatches.append(ByteMismatch(offset=offset, bn_byte=bn_byte, vc5_byte=vc5_byte, masked=masked))

    return MaskedByteComparison(
        bn_size=original_bn_size,
        vc5_size=original_vc5_size,
        compared_size=compared_size,
        relocation_masked_bytes=relocation_masked_bytes,
        trailing_bn_nops_trimmed=trailing_bn_nops,
        trailing_vc5_nops_trimmed=trailing_vc5_nops,
        mismatches=tuple(mismatches),
    )


def format_byte_dump(data: bytes, *, base: int = 0, mask: tuple[bool, ...] | None = None) -> list[str]:
    if mask is not None and len(mask) != len(data):
        raise ValueError("Byte dump mask length differs from data length")
    lines: list[str] = []
    for offset in range(0, len(data), 16):
        chunk = data[offset : offset + 16]
        parts: list[str] = []
        for index, value in enumerate(chunk):
            absolute = offset + index
            if mask is not None and mask[absolute]:
                parts.append(f"{value:02x}*")
            else:
                parts.append(f"{value:02x}")
        lines.append(f"{base + offset:08x}: {' '.join(parts)}")
    return lines


def format_mask_lines(function: CoffFunctionBytes) -> list[str]:
    lines = [
        f"symbol: {function.symbol}",
        f"section: {function.section_name}#{function.section_index}",
        f"compared_range: 0x{function.start:x}..0x{function.end:x}",
        f"natural_coff_range: 0x{function.start:x}..0x{function.natural_end:x}",
        f"excluded_natural_tail_bytes: {function.excluded_tail_size}",
        f"excluded_natural_tail_relocations: {function.excluded_tail_relocation_count}",
        "excluded_tail_disposition: inventory-only; not compared or accepted",
        f"relocations: {len(function.relocations)}",
        "",
    ]
    for relocation in function.relocations:
        size = relocation_size(relocation.type)
        relative_offset = relocation.offset - function.start
        lines.append(
            f"0x{relative_offset:08x} size={size} type={relocation_type_name(relocation.type)} "
            f"symbol={relocation.symbol_name}"
        )
    return lines


def format_data_mask_lines(data_symbol: CoffDataSymbolBytes) -> list[str]:
    lines = [
        f"symbol: {data_symbol.symbol}",
        f"section: {data_symbol.section_name}#{data_symbol.section_index}",
        f"symbol_start: 0x{data_symbol.symbol_start:x}",
        f"object_offset: {data_symbol.object_offset} (0x{data_symbol.object_offset:x})",
        f"data_range: 0x{data_symbol.start:x}..0x{data_symbol.end:x}",
        f"relocations: {len(data_symbol.relocations)}",
        "",
    ]
    for relocation in data_symbol.relocations:
        size = relocation_size(relocation.type)
        relative_offset = relocation.offset - data_symbol.start
        lines.append(
            f"0x{relative_offset:08x} size={size} type={relocation_type_name(relocation.type)} "
            f"symbol={relocation.symbol_name}"
        )
    return lines


def format_relocation_identity_lines(
    *,
    address: str,
    symbol: str,
    symbol_bytes: CoffDataSymbolBytes,
    bn_bytes: bytes,
    bridge: BinaryNinjaBridge | None,
) -> list[str]:
    _by_name: dict[str, object]
    by_address: dict[str, object] = {}
    if symbol_bytes.relocations and bridge is not None:
        by_address, _by_name = bridge.symbols()
    base_address = int(normalize_address(address), 16)
    lines = [
        f"address: {normalize_address(address)}",
        f"symbol: {symbol}",
        f"section: {symbol_bytes.section_name}#{symbol_bytes.section_index}",
        f"symbol_start: 0x{symbol_bytes.symbol_start:x}",
        f"object_offset: {symbol_bytes.object_offset} (0x{symbol_bytes.object_offset:x})",
        f"range: 0x{symbol_bytes.start:x}..0x{symbol_bytes.end:x}",
        f"relocations: {len(symbol_bytes.relocations)}",
        "",
        "offset  original  type  size  vc5_symbol  bn_pointer  bn_symbol",
    ]
    for relocation in symbol_bytes.relocations:
        size = relocation_size(relocation.type)
        relative_offset = relocation.offset - symbol_bytes.start
        pointer_text = "<not-32-bit>"
        bn_symbol = ""
        if size == 4 and relative_offset + 4 <= len(bn_bytes):
            pointer = int.from_bytes(bn_bytes[relative_offset : relative_offset + 4], "little")
            pointer_text = f"0x{pointer:x}"
            resolved = by_address.get(pointer_text)
            if resolved is not None:
                bn_symbol = getattr(resolved, "name", "")
            elif symbol_bytes.relocations and bridge is None:
                bn_symbol = "<not resolved from cache>"
        lines.append(
            f"0x{relative_offset:04x}  "
            f"0x{base_address + relative_offset:x}  "
            f"{relocation_type_name(relocation.type)}  "
            f"{size}  "
            f"{relocation.symbol_name}  "
            f"{pointer_text}  "
            f"{bn_symbol}"
        )
    return lines


def format_byte_diff_lines(
    *,
    address: str,
    symbol: str,
    comparison: MaskedByteComparison,
    bn_bytes: bytes,
    vc5_bytes: bytes,
) -> list[str]:
    base_address = int(normalize_address(address), 16)
    lines = [
        f"address: {normalize_address(address)}",
        f"symbol: {symbol}",
        f"bn_size: {comparison.bn_size}",
        f"vc5_size: {comparison.vc5_size}",
        f"compared_size_after_trim: {comparison.compared_size}",
        f"relocation_masked_bytes: {comparison.relocation_masked_bytes}",
        f"trailing_bn_nops_trimmed: {comparison.trailing_bn_nops_trimmed}",
        f"trailing_vc5_nops_trimmed: {comparison.trailing_vc5_nops_trimmed}",
        f"mismatches: {comparison.mismatch_count}",
        "",
    ]
    if comparison.mismatch_count == 0:
        lines.append("Byte sequences match after masking COFF relocation fields.")
        return lines

    for mismatch in comparison.mismatches[:200]:
        bn_text = "<missing>" if mismatch.bn_byte is None else f"{mismatch.bn_byte:02x}"
        vc5_text = "<missing>" if mismatch.vc5_byte is None else f"{mismatch.vc5_byte:02x}"
        context_start = max(0, mismatch.offset - 8)
        context_end = mismatch.offset + 9
        bn_context = " ".join(f"{value:02x}" for value in bn_bytes[context_start:context_end])
        vc5_context = " ".join(f"{value:02x}" for value in vc5_bytes[context_start:context_end])
        lines.append(f"[mismatch] offset=0x{mismatch.offset:08x} original=0x{base_address + mismatch.offset:x}")
        lines.append(f"  BN:  {bn_text}    {bn_context}")
        lines.append(f"  VC5SP3: {vc5_text}    {vc5_context}")
    if comparison.mismatch_count > 200:
        lines.append(f"... {comparison.mismatch_count - 200} additional mismatches omitted")
    return lines


def mismatch_clusters(
    mismatches: tuple[ByteMismatch, ...],
    *,
    max_gap: int = 4,
) -> list[tuple[int, int, int]]:
    if not mismatches:
        return []

    clusters: list[tuple[int, int, int]] = []
    start = mismatches[0].offset
    end = start
    count = 1
    for mismatch in mismatches[1:]:
        if mismatch.offset <= end + max_gap:
            end = mismatch.offset
            count += 1
            continue
        clusters.append((start, end, count))
        start = mismatch.offset
        end = mismatch.offset
        count = 1
    clusters.append((start, end, count))
    return clusters


def format_byte_triage_lines(
    *,
    address: str,
    symbol: str,
    comparison: MaskedByteComparison,
) -> list[str]:
    base_address = int(normalize_address(address), 16)
    size_delta = comparison.vc5_size - comparison.bn_size
    lines = [
        f"address: {normalize_address(address)}",
        f"symbol: {symbol}",
        f"status: {'OK' if comparison.mismatch_count == 0 else 'FAIL'}",
        f"bn_size: {comparison.bn_size}",
        f"vc5_size: {comparison.vc5_size}",
        f"size_delta_vc5_minus_bn: {size_delta}",
        f"compared_size_after_trim: {comparison.compared_size}",
        f"relocation_masked_bytes: {comparison.relocation_masked_bytes}",
        f"trailing_bn_nops_trimmed: {comparison.trailing_bn_nops_trimmed}",
        f"trailing_vc5_nops_trimmed: {comparison.trailing_vc5_nops_trimmed}",
        f"mismatches: {comparison.mismatch_count}",
        "",
    ]

    if comparison.mismatch_count == 0:
        lines.append("Triage: byte sequences match after relocation masking and optional trailing NOP trimming.")
        return lines

    first = comparison.mismatches[0]
    lines.extend(
        [
            f"first_mismatch_offset: 0x{first.offset:x}",
            f"first_mismatch_original: 0x{base_address + first.offset:x}",
            "",
            "Mismatch clusters:",
        ]
    )
    for start, end, count in mismatch_clusters(comparison.mismatches)[:20]:
        original_start = base_address + start
        original_end = base_address + end
        if start == end:
            span = f"offset 0x{start:x} original 0x{original_start:x}"
        else:
            span = f"offsets 0x{start:x}..0x{end:x} originals 0x{original_start:x}..0x{original_end:x}"
        lines.append(f"- {span}: {count} mismatch byte(s)")
    clusters = mismatch_clusters(comparison.mismatches)
    if len(clusters) > 20:
        lines.append(f"- ... {len(clusters) - 20} additional cluster(s) omitted")

    lines.extend(["", "Likely next checks:"])
    if first.offset == 0:
        lines.append("- Function entry differs; inspect calling convention, prologue, and compiler flags first.")
    if size_delta != 0:
        lines.append("- Function size differs; inspect control-flow shape, helper inlining, EH cleanup, and epilogue emission.")
    if comparison.relocation_masked_bytes:
        lines.append("- COFF relocation bytes were masked; focus on the unmasked mismatch clusters above.")
    if comparison.trailing_bn_nops_trimmed or comparison.trailing_vc5_nops_trimmed:
        lines.append("- Trailing NOP padding was trimmed; remaining mismatches are not explained by end padding.")
    lines.append("- Compare the normalized asm text diff beside the byte artifacts to identify instruction-level drift.")
    return lines


def write_optional_text_reports(
    *,
    bn_instructions: list[Instruction],
    cod_path: Path | None,
    symbol: str | None,
    out_dir: Path,
    safe_address: str,
) -> tuple[Path | None, Path | None]:
    if cod_path is None or not cod_path.exists():
        return None, None
    try:
        cod_text = extract_cod_function(cod_path.read_text(encoding="utf-8", errors="replace"), symbol)
    except ValueError as exc:
        error_path = out_dir / f"{safe_address}_asm_text_diff_error.txt"
        write_lines(error_path, [str(exc)])
        return error_path, None
    cod_instructions = parse_assembly(cod_text, source="cod")
    bn_lines = [instruction.text for instruction in bn_instructions]
    cod_lines = [instruction.text for instruction in cod_instructions]
    cod_out = out_dir / f"{safe_address}_msvc.norm.asm"
    write_lines(cod_out, cod_lines)
    diff_lines = list(
        unified_diff(
            bn_lines,
            cod_lines,
            fromfile=f"BN {safe_address}",
            tofile=f"MSVC {cod_path.name}",
            lineterm="",
        )
    )
    diff_path = out_dir / f"{safe_address}_asm_text_diff.txt"
    write_lines(diff_path, diff_lines)
    classified_lines, _mismatch_count = classify_instruction_differences(bn_instructions, cod_instructions)
    classified_path = out_dir / f"{safe_address}_asm_classified_diff.txt"
    write_lines(classified_path, classified_lines)
    return diff_path, classified_path


def compare_bn_to_obj(
    *,
    address: str,
    obj_path: Path,
    symbol: str,
    out_dir: Path,
    bridge_url: str,
    bridge: BinaryNinjaBridge | None = None,
    cod_path: Path | None = None,
    trim_padding_nops: bool = True,
    bn_byte_length: int | None = None,
    vc5_byte_length: int | None = None,
    bn_hexdump_path: Path | None = None,
) -> ObjectByteComparison:
    address = normalize_address(address)
    safe_address = address[2:]

    if bn_hexdump_path is not None:
        bn_bytes = bytes_from_hexdump(
            bn_hexdump_path.read_text(encoding="utf-8"),
            expected_length=bn_byte_length,
        )
        bn_instructions: list[Instruction] = []
        bn_lines = [
            f"; Binary Ninja bytes loaded from cache: {bn_hexdump_path}",
            "; Normalized assembly was not fetched in cached mode.",
        ]
    else:
        bridge = bridge or BinaryNinjaBridge(bridge_url)
        bn_asm = bridge.assembly(address)
        bn_instructions = parse_assembly(bn_asm, source="bn")
        bn_lines = [instruction.text for instruction in bn_instructions]
        if bn_byte_length is None:
            bn_bytes = instructions_to_bytes(bn_instructions)
        else:
            bn_bytes = bytes_from_hexdump(bridge.hexdump(address, bn_byte_length), expected_length=bn_byte_length)
    bn_path = out_dir / f"{safe_address}_bn.bytes"
    write_lines(bn_path, format_byte_dump(bn_bytes, base=int(address, 16)))
    bn_asm_path = out_dir / f"{safe_address}_bn.norm.asm"
    write_lines(bn_asm_path, bn_lines)

    coff = CoffObject.from_path(obj_path)
    function = coff.function_bytes(symbol, byte_length=vc5_byte_length)
    vc5_path = out_dir / f"{safe_address}_vc5.bytes"
    write_lines(vc5_path, format_byte_dump(function.data, mask=function.relocation_mask))

    mask_path = out_dir / f"{safe_address}_reloc_mask.txt"
    write_lines(mask_path, format_mask_lines(function))

    comparison = compare_masked_byte_sequences(
        bn_bytes,
        function.data,
        function.relocation_mask,
        trim_padding_nops=trim_padding_nops,
    )
    diff_path = out_dir / f"{safe_address}_byte_diff.txt"
    write_lines(
        diff_path,
        format_byte_diff_lines(
            address=address,
            symbol=symbol,
            comparison=comparison,
            bn_bytes=bn_bytes,
            vc5_bytes=function.data,
        ),
    )
    triage_path = out_dir / f"{safe_address}_byte_triage.txt"
    write_lines(
        triage_path,
        format_byte_triage_lines(
            address=address,
            symbol=symbol,
            comparison=comparison,
        ),
    )
    text_diff_path, classified_text_path = write_optional_text_reports(
        bn_instructions=bn_instructions,
        cod_path=None if bn_hexdump_path is not None else cod_path,
        symbol=symbol,
        out_dir=out_dir,
        safe_address=safe_address,
    )

    return ObjectByteComparison(
        address=address,
        symbol=symbol,
        obj_path=obj_path,
        bn_path=bn_path,
        vc5_path=vc5_path,
        mask_path=mask_path,
        diff_path=diff_path,
        triage_path=triage_path,
        text_diff_path=text_diff_path,
        classified_text_path=classified_text_path,
        mismatch_count=comparison.mismatch_count,
        relocation_masked_bytes=comparison.relocation_masked_bytes,
        bn_size=comparison.bn_size,
        vc5_size=comparison.vc5_size,
        trailing_bn_nops_trimmed=comparison.trailing_bn_nops_trimmed,
        trailing_vc5_nops_trimmed=comparison.trailing_vc5_nops_trimmed,
        natural_vc5_size=function.natural_end - function.start,
        excluded_vc5_tail_bytes=function.excluded_tail_size,
        excluded_vc5_tail_relocations=function.excluded_tail_relocation_count,
    )


def compare_bn_data_to_obj(
    *,
    address: str,
    obj_path: Path,
    symbol: str,
    byte_length: int,
    out_dir: Path,
    bridge_url: str,
    bridge: BinaryNinjaBridge | None = None,
    name: str | None = None,
    bn_hexdump_path: Path | None = None,
    object_offset: int = 0,
) -> ObjectByteComparison:
    address = normalize_address(address)
    safe_address = address[2:]

    if bn_hexdump_path is not None:
        bn_bytes = bytes_from_hexdump(
            bn_hexdump_path.read_text(encoding="utf-8"),
            expected_length=byte_length,
        )
    else:
        bridge = bridge or BinaryNinjaBridge(bridge_url)
        bn_bytes = bytes_from_hexdump(bridge.hexdump(address, byte_length), expected_length=byte_length)
    bn_path = out_dir / f"{safe_address}_bn_data.bytes"
    write_lines(bn_path, format_byte_dump(bn_bytes, base=int(address, 16)))

    coff = CoffObject.from_path(obj_path)
    data_symbol = coff.data_symbol_bytes(
        symbol,
        byte_length=byte_length,
        object_offset=object_offset,
    )
    vc5_path = out_dir / f"{safe_address}_vc5_data.bytes"
    write_lines(
        vc5_path,
        format_byte_dump(
            data_symbol.data,
            base=data_symbol.start,
            mask=data_symbol.relocation_mask,
        ),
    )

    mask_path = out_dir / f"{safe_address}_data_reloc_mask.txt"
    write_lines(mask_path, format_data_mask_lines(data_symbol))

    comparison = compare_masked_byte_sequences(
        bn_bytes,
        data_symbol.data,
        data_symbol.relocation_mask,
        trim_padding_nops=False,
    )
    diff_path = out_dir / f"{safe_address}_data_byte_diff.txt"
    write_lines(
        diff_path,
        format_byte_diff_lines(
            address=address,
            symbol=symbol,
            comparison=comparison,
            bn_bytes=bn_bytes,
            vc5_bytes=data_symbol.data,
        ),
    )
    triage_path = out_dir / f"{safe_address}_data_byte_triage.txt"
    write_lines(
        triage_path,
        format_byte_triage_lines(
            address=address,
            symbol=symbol,
            comparison=comparison,
        ),
    )
    identity_path = out_dir / f"{safe_address}_data_reloc_identity.txt"
    write_lines(
        identity_path,
        format_relocation_identity_lines(
            address=address,
            symbol=name or symbol,
            symbol_bytes=data_symbol,
            bn_bytes=bn_bytes,
            bridge=bridge,
        ),
    )

    return ObjectByteComparison(
        address=address,
        symbol=symbol,
        obj_path=obj_path,
        bn_path=bn_path,
        vc5_path=vc5_path,
        mask_path=mask_path,
        diff_path=diff_path,
        triage_path=triage_path,
        text_diff_path=None,
        classified_text_path=None,
        mismatch_count=comparison.mismatch_count,
        relocation_masked_bytes=comparison.relocation_masked_bytes,
        bn_size=comparison.bn_size,
        vc5_size=comparison.vc5_size,
        trailing_bn_nops_trimmed=0,
        trailing_vc5_nops_trimmed=0,
        relocation_identity_path=identity_path,
    )


def compare_bn_to_cod(
    *,
    address: str,
    cod_path: Path,
    symbol: str | None,
    out_dir: Path,
    bridge_url: str,
    bridge: BinaryNinjaBridge | None = None,
) -> AssemblyComparison:
    address = normalize_address(address)
    safe_address = address[2:]
    bridge = bridge or BinaryNinjaBridge(bridge_url)

    bn_asm = bridge.assembly(address)
    bn_instructions = parse_assembly(bn_asm, source="bn")
    bn_lines = [instruction.text for instruction in bn_instructions]
    bn_path = out_dir / f"{safe_address}_bn.norm.asm"
    write_lines(bn_path, bn_lines)

    cod_text = extract_cod_function(cod_path.read_text(encoding="utf-8", errors="replace"), symbol)
    cod_instructions = parse_assembly(cod_text, source="cod")
    cod_lines = [instruction.text for instruction in cod_instructions]
    cod_out = out_dir / f"{safe_address}_msvc.norm.asm"
    write_lines(cod_out, cod_lines)

    diff_lines = list(unified_diff(
        bn_lines,
        cod_lines,
        fromfile=f"BN {address}",
        tofile=f"MSVC {cod_path.name}",
        lineterm="",
    ))
    diff_path = out_dir / f"{safe_address}_diff.txt"
    write_lines(diff_path, diff_lines)

    classified_lines, mismatch_count = classify_instruction_differences(bn_instructions, cod_instructions)
    classified_path = out_dir / f"{safe_address}_classified_diff.txt"
    write_lines(classified_path, classified_lines)

    return AssemblyComparison(
        address=address,
        bn_path=bn_path,
        cod_path=cod_out,
        diff_path=diff_path,
        classified_path=classified_path,
        diff_count=len(diff_lines),
        mismatch_count=mismatch_count,
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Extract and compare Binary Ninja and MSVC assembly evidence.")
    parser.add_argument("address", help="Binary Ninja function address, e.g. 0x407170")
    parser.add_argument("--cod", help="MSVC .cod listing to compare against")
    parser.add_argument("--obj", help="MSVC COFF .obj file to compare against using relocation-masked bytes")
    parser.add_argument("--symbol", help="MSVC symbol name to extract from the .obj/.cod listing")
    parser.add_argument("--out-dir", default="build/verification", help="Directory for normalized evidence")
    parser.add_argument("--bridge-url", default=DEFAULT_BRIDGE_URL, help="Binary Ninja bridge URL")
    parser.add_argument("--fail-on-diff", action="store_true", help="Return non-zero when the selected comparison differs")
    parser.add_argument("--no-trim-trailing-nops", action="store_true", help="Keep compiler-emitted trailing NOP padding")
    args = parser.parse_args(argv)

    out_dir = Path(args.out_dir)

    if args.obj:
        if not args.symbol:
            print("--symbol is required with --obj", file=sys.stderr)
            return 2
        obj_path = Path(args.obj)
        if not obj_path.exists():
            print(f"OBJ file not found: {obj_path}", file=sys.stderr)
            return 2
        cod_path = Path(args.cod) if args.cod else None
        try:
            comparison = compare_bn_to_obj(
                address=args.address,
                obj_path=obj_path,
                symbol=args.symbol,
                out_dir=out_dir,
                bridge_url=args.bridge_url,
                cod_path=cod_path,
                trim_padding_nops=not args.no_trim_trailing_nops,
            )
        except (BridgeError, ValueError) as exc:
            print(str(exc), file=sys.stderr)
            return 1
        print(f"Wrote {comparison.bn_path}")
        print(f"Wrote {comparison.vc5_path}")
        print(f"Wrote {comparison.mask_path}")
        print(f"Wrote {comparison.diff_path}")
        if comparison.text_diff_path is not None:
            print(f"Wrote {comparison.text_diff_path}")
        if comparison.mismatch_count:
            print("Object bytes have unmasked mismatches; review the byte diff before marking tier S.")
            return 1 if args.fail_on_diff else 0
        print("Object bytes match after masking COFF relocation fields.")
        return 0

    if not args.cod:
        address = normalize_address(args.address)
        safe_address = address[2:]
        bridge = BinaryNinjaBridge(args.bridge_url)
        try:
            bn_asm = bridge.assembly(address)
        except BridgeError as exc:
            print(str(exc), file=sys.stderr)
            return 1

        bn_instructions = parse_assembly(bn_asm, source="bn")
        bn_lines = [instruction.text for instruction in bn_instructions]
        bn_path = out_dir / f"{safe_address}_bn.norm.asm"
        write_lines(bn_path, bn_lines)
        print(f"Wrote {bn_path}")
        print("No --cod listing provided; BN assembly extraction complete.")
        return 0

    cod_path = Path(args.cod)
    if not cod_path.exists():
        print(f"COD listing not found: {cod_path}", file=sys.stderr)
        return 2
    try:
        comparison = compare_bn_to_cod(
            address=args.address,
            cod_path=cod_path,
            symbol=args.symbol,
            out_dir=out_dir,
            bridge_url=args.bridge_url,
        )
    except BridgeError as exc:
        print(str(exc), file=sys.stderr)
        return 1
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    print(f"Wrote {comparison.bn_path}")
    print(f"Wrote {comparison.cod_path}")
    print(f"Wrote {comparison.diff_path}")
    print(f"Wrote {comparison.classified_path}")

    if comparison.mismatch_count:
        print("Assembly has instruction mismatches; review the classified diff before marking tier S.")
        return 1 if args.fail_on_diff else 0
    if comparison.diff_count:
        print(
            "No instruction mismatches found; normalized assembly differs by spelling or "
            "relocation-sensitive items. Review the classified diff before marking tier S."
        )
        return 1 if args.fail_on_diff else 0
    print("Normalized assembly matches.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
