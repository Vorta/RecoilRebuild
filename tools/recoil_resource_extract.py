from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
from hashlib import sha256
import json
from pathlib import Path
import struct
import sys
from typing import NamedTuple

from recoil_pe import PeFormatError, PeSection, data_directory, hex32, parse_pe_headers, rva_to_offset
from recoil_tooling import write_if_changed, write_text_if_changed


RESOURCE_TYPE_NAMES = {
    1: "cursor",
    2: "bitmap",
    3: "icon",
    4: "menu",
    5: "dialog",
    6: "string",
    9: "accelerator",
    12: "group_cursor",
    14: "group_icon",
    16: "version",
    24: "manifest",
}


RC_RESOURCE_TYPES = {
    2: "BITMAP",
    3: "ICON",
    4: "MENU",
    5: "DIALOG",
    6: "STRINGTABLE",
    14: "GROUP_ICON",
    16: "VERSION",
}


DEFAULT_RESOURCE_MEMORY_FLAGS = 0x1030
DS_SETFONT = 0x40


MENU_FLAG_NAMES = [
    (0x0001, "GRAYED"),
    (0x0002, "INACTIVE"),
    (0x0008, "CHECKED"),
    (0x0020, "MENUBARBREAK"),
    (0x0040, "MENUBREAK"),
]


STANDARD_DIALOG_CLASSES = {
    0x80: "Button",
    0x81: "Edit",
    0x82: "Static",
    0x83: "ListBox",
    0x84: "ScrollBar",
    0x85: "ComboBox",
}


@dataclass(frozen=True)
class ResourceEntry:
    type_id: int | str
    name: int | str
    language: int
    rva: int
    file_offset: int
    size: int
    codepage: int
    raw_path: str
    sha256: str


class ResourceExtractError(ValueError):
    pass


class DialogControl(NamedTuple):
    style: int
    exstyle: int
    x: int
    y: int
    cx: int
    cy: int
    control_id: int
    control_class: int | str
    title: int | str | None
    creation_data: bytes


class DialogTemplate(NamedTuple):
    style: int
    exstyle: int
    x: int
    y: int
    cx: int
    cy: int
    menu: int | str | None
    window_class: int | str | None
    caption: str | None
    font_size: int | None
    font_face: str | None
    controls: list[DialogControl]


class MenuItem(NamedTuple):
    flags: int
    text: str
    item_id: int | None
    children: list["MenuItem"]


class ResPayload(NamedTuple):
    type_id: int | str
    name: int | str
    language: int
    payload: bytes


def resource_label(value: int | str) -> str:
    if isinstance(value, str):
        return value
    return f"#{value}"


def resource_file_token(value: int | str) -> str:
    if isinstance(value, str):
        return "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in value)
    return str(value)


def resource_type_label(type_id: int | str) -> str:
    if isinstance(type_id, str):
        return resource_file_token(type_id).lower()
    return RESOURCE_TYPE_NAMES.get(type_id, f"type_{type_id}")


def rc_name(value: int | str) -> str:
    if isinstance(value, str):
        return f'"{value}"' if not value.isidentifier() else value
    return str(value)


def rc_type(value: int | str) -> str:
    if isinstance(value, str):
        return f'"{value}"' if not value.isidentifier() else value
    return str(value)


def rc_int(value: int) -> str:
    if value == 0xFFFF:
        return "-1"
    return str(value)


def rc_hex(value: int) -> str:
    return f"0x{value:x}"


def rc_string(value: str) -> str:
    escaped: list[str] = []
    for ch in value:
        if ch == "\\":
            escaped.append("\\\\")
        elif ch == '"':
            escaped.append('""')
        elif ch == "\0":
            escaped.append("\\0")
        elif ch == "\n":
            escaped.append("\\n")
        elif ch == "\r":
            escaped.append("\\r")
        elif ord(ch) < 0x20 or ord(ch) > 0x7E:
            escaped.append(f"\\{ord(ch):03o}")
        else:
            escaped.append(ch)
    return '"' + "".join(escaped) + '"'


def rc_text_or_ordinal(value: int | str | None) -> str:
    if value is None:
        return '""'
    if isinstance(value, int):
        return str(value)
    return rc_string(value)


def rc_options(flags: int, ignored: int = 0) -> str:
    remaining = flags & ~ignored
    options: list[str] = []
    for bit, name in MENU_FLAG_NAMES:
        if remaining & bit:
            options.append(name)
            remaining &= ~bit
    if remaining:
        options.append(rc_hex(remaining))
    return ", " + ", ".join(options) if options else ""


def parse_sections(data: bytes) -> tuple[int, int, list[PeSection]]:
    try:
        headers = parse_pe_headers(data)
    except PeFormatError as exc:
        raise ResourceExtractError(str(exc)) from exc
    resource_directory = data_directory(headers, 2)
    return resource_directory.rva, resource_directory.size, list(headers.sections)


def read_resource_name(data: bytes, base_offset: int, relative_offset: int) -> str:
    offset = base_offset + relative_offset
    length = struct.unpack_from("<H", data, offset)[0]
    raw = data[offset + 2 : offset + 2 + length * 2]
    return raw.decode("utf-16le", errors="replace")


def decode_directory_name(data: bytes, base_offset: int, raw_name: int) -> int | str:
    if raw_name & 0x80000000:
        return read_resource_name(data, base_offset, raw_name & 0x7FFFFFFF)
    return raw_name


def align4_offset(offset: int) -> int:
    return (offset + 3) & ~3


def read_utf16_or_ordinal(data: bytes, offset: int) -> tuple[int | str | None, int]:
    marker = struct.unpack_from("<H", data, offset)[0]
    if marker == 0:
        return None, offset + 2
    if marker == 0xFFFF:
        return struct.unpack_from("<H", data, offset + 2)[0], offset + 4

    chars: list[int] = []
    while True:
        value = struct.unpack_from("<H", data, offset)[0]
        offset += 2
        if value == 0:
            break
        chars.append(value)
    text = bytes().join(char.to_bytes(2, "little") for char in chars).decode(
        "utf-16le", errors="replace"
    )
    return text, offset


def read_utf16_text(data: bytes, offset: int) -> tuple[str, int]:
    value, offset = read_utf16_or_ordinal(data, offset)
    if value is None:
        return "", offset
    if not isinstance(value, str):
        raise ResourceExtractError("expected UTF-16 text field")
    return value, offset


def walk_resource_directory(
    data: bytes,
    base_offset: int,
    sections: list[PeSection],
    relative_offset: int,
    stack: list[int | str],
) -> list[tuple[list[int | str], int, int, int, int]]:
    directory_offset = base_offset + relative_offset
    _characteristics, _timestamp, _major, _minor, named_count, id_count = struct.unpack_from(
        "<IIHHHH", data, directory_offset
    )
    entries: list[tuple[list[int | str], int, int, int, int]] = []
    for index in range(named_count + id_count):
        entry_offset = directory_offset + 16 + index * 8
        raw_name, raw_target = struct.unpack_from("<II", data, entry_offset)
        name = decode_directory_name(data, base_offset, raw_name)
        target = raw_target & 0x7FFFFFFF
        if raw_target & 0x80000000:
            entries.extend(
                walk_resource_directory(data, base_offset, sections, target, stack + [name])
            )
            continue

        data_rva, size, codepage, _reserved = struct.unpack_from("<IIII", data, base_offset + target)
        data_offset = rva_to_offset(data_rva, sections)
        if data_offset is None:
            raise ResourceExtractError(f"resource data RVA cannot be mapped: {hex32(data_rva)}")
        entries.append((stack + [name], data_rva, data_offset, size, codepage))
    return entries


def parse_resources(path: Path) -> tuple[int, int, list[ResourceEntry], bytes]:
    data = path.read_bytes()
    rsrc_rva, rsrc_size, sections = parse_sections(data)
    rsrc_offset = rva_to_offset(rsrc_rva, sections)
    if rsrc_rva == 0 or rsrc_offset is None:
        raise ResourceExtractError("PE has no mapped resource directory")

    entries: list[ResourceEntry] = []
    for parts, data_rva, data_offset, size, codepage in walk_resource_directory(
        data, rsrc_offset, sections, 0, []
    ):
        if len(parts) != 3:
            raise ResourceExtractError(f"unexpected resource path depth: {parts!r}")
        payload = data[data_offset : data_offset + size]
        type_id, name, language = parts
        if not isinstance(language, int):
            raise ResourceExtractError(f"resource language is not numeric: {parts!r}")

        entries.append(
            ResourceEntry(
                type_id=type_id,
                name=name,
                language=language,
                rva=data_rva,
                file_offset=data_offset,
                size=size,
                codepage=codepage,
                raw_path="",
                sha256=sha256(payload).hexdigest(),
            )
        )

    entries.sort(
        key=lambda item: (
            str(item.type_id) if isinstance(item.type_id, str) else f"{item.type_id:08d}",
            str(item.name) if isinstance(item.name, str) else f"{item.name:08d}",
            item.language,
        )
    )
    return rsrc_rva, rsrc_size, entries, data


def raw_resource_filename(entry: ResourceEntry) -> str:
    type_part = resource_type_label(entry.type_id)
    name_part = resource_file_token(entry.name)
    return f"{type_part}_{name_part}_{entry.language:04x}.bin"


def bitmap_file_from_dib(dib: bytes) -> bytes:
    if len(dib) < 4:
        raise ResourceExtractError("bitmap resource is too small")
    dib_header_size = struct.unpack_from("<I", dib, 0)[0]
    pixel_offset = dib_header_size
    if dib_header_size >= 40 and len(dib) >= dib_header_size:
        bit_count = struct.unpack_from("<H", dib, 14)[0]
        colors_used = struct.unpack_from("<I", dib, 32)[0]
        if bit_count <= 8:
            color_count = colors_used if colors_used != 0 else 1 << bit_count
            pixel_offset = dib_header_size + color_count * 4
    file_header_size = 14
    total_size = file_header_size + len(dib)
    return b"BM" + struct.pack("<IHHI", total_size, 0, 0, file_header_size + pixel_offset) + dib


def align4_size(size: int) -> int:
    return (size + 3) & ~3


def append_aligned(buffer: bytearray, payload: bytes) -> None:
    buffer.extend(payload)
    while len(buffer) % 4 != 0:
        buffer.append(0)


def res_id_or_string(value: int | str) -> bytes:
    if isinstance(value, int):
        return struct.pack("<HH", 0xFFFF, value)
    return value.encode("utf-16le") + b"\0\0"


def res_record(type_id: int | str, name: int | str, language: int, payload: bytes) -> bytes:
    header_tail = bytearray()
    append_aligned(header_tail, res_id_or_string(type_id))
    append_aligned(header_tail, res_id_or_string(name))
    header_tail.extend(
        struct.pack(
            "<IHHII",
            0,
            DEFAULT_RESOURCE_MEMORY_FLAGS,
            language,
            0,
            0,
        )
    )
    header_size = 8 + len(header_tail)
    record = bytearray(struct.pack("<II", len(payload), header_size))
    record.extend(header_tail)
    record.extend(payload)
    while len(record) % 4 != 0:
        record.append(0)
    return bytes(record)


def make_res_file(entries: list[ResourceEntry], data: bytes) -> bytes:
    output = bytearray()
    output.extend(res_record(0, 0, 0, b""))
    for entry in entries:
        payload = data[entry.file_offset : entry.file_offset + entry.size]
        output.extend(res_record(entry.type_id, entry.name, entry.language, payload))
    return bytes(output)


def read_res_id_or_string(data: bytes, offset: int) -> tuple[int | str, int]:
    marker = struct.unpack_from("<H", data, offset)[0]
    if marker == 0xFFFF:
        return struct.unpack_from("<H", data, offset + 2)[0], offset + 4

    chars: list[int] = []
    while True:
        value = struct.unpack_from("<H", data, offset)[0]
        offset += 2
        if value == 0:
            break
        chars.append(value)
    return (
        bytes().join(char.to_bytes(2, "little") for char in chars).decode(
            "utf-16le", errors="replace"
        ),
        offset,
    )


def parse_res_payloads(path: Path) -> list[ResPayload]:
    data = path.read_bytes()
    offset = 0
    payloads: list[ResPayload] = []
    while offset < len(data):
        if offset + 8 > len(data):
            raise ResourceExtractError("truncated .res record header")
        data_size, header_size = struct.unpack_from("<II", data, offset)
        header_end = offset + header_size
        if header_end > len(data):
            raise ResourceExtractError(".res record header extends past file")
        cursor = offset + 8
        type_id, cursor = read_res_id_or_string(data, cursor)
        cursor = align4_offset(cursor)
        name, cursor = read_res_id_or_string(data, cursor)
        cursor = align4_offset(cursor)
        _data_version, _memory_flags, language, _version, _characteristics = struct.unpack_from(
            "<IHHII", data, cursor
        )
        payload_start = header_end
        payload_end = payload_start + data_size
        if payload_end > len(data):
            raise ResourceExtractError(".res record payload extends past file")
        if type_id != 0 or name != 0:
            payloads.append(ResPayload(type_id, name, language, data[payload_start:payload_end]))
        offset = align4_offset(payload_end)
    return payloads


def compare_res_payloads(reference: Path, candidate_res: Path) -> list[str]:
    _rsrc_rva, _rsrc_size, entries, data = parse_resources(reference)
    expected = {
        (entry.type_id, entry.name, entry.language): data[
            entry.file_offset : entry.file_offset + entry.size
        ]
        for entry in entries
    }
    actual = {
        (payload.type_id, payload.name, payload.language): payload.payload
        for payload in parse_res_payloads(candidate_res)
    }
    mismatches: list[str] = []
    for key in sorted(set(expected) | set(actual), key=lambda item: tuple(str(part) for part in item)):
        if key not in expected:
            mismatches.append(f"{key}: unexpected candidate resource")
            continue
        if key not in actual:
            mismatches.append(f"{key}: missing candidate resource")
            continue
        expected_payload = expected[key]
        actual_payload = actual[key]
        if expected_payload != actual_payload:
            mismatches.append(
                f"{key}: payload differs "
                f"expected size={len(expected_payload)} sha256={sha256(expected_payload).hexdigest()} "
                f"actual size={len(actual_payload)} sha256={sha256(actual_payload).hexdigest()}"
            )
    return mismatches


def parse_menu_resource(payload: bytes) -> list[MenuItem]:
    if len(payload) < 4 or struct.unpack_from("<HH", payload, 0) != (0, 0):
        raise ResourceExtractError("only standard MENU resources are supported")

    def parse_items(offset: int) -> tuple[list[MenuItem], int]:
        items: list[MenuItem] = []
        while offset < len(payload):
            flags = struct.unpack_from("<H", payload, offset)[0]
            offset += 2
            is_popup = (flags & 0x10) != 0
            is_end = (flags & 0x80) != 0
            if is_popup:
                text, offset = read_utf16_text(payload, offset)
                children, offset = parse_items(offset)
                items.append(MenuItem(flags, text, None, children))
            else:
                item_id = struct.unpack_from("<H", payload, offset)[0]
                offset += 2
                text, offset = read_utf16_text(payload, offset)
                items.append(MenuItem(flags, text, item_id, []))
            if is_end:
                return items, offset
        return items, offset

    items, offset = parse_items(4)
    if offset != len(payload):
        raise ResourceExtractError("MENU resource parse did not consume the payload")
    return items


def render_menu_items(items: list[MenuItem], indent: int) -> list[str]:
    lines: list[str] = []
    prefix = " " * indent
    for item in items:
        if item.children:
            lines.append(
                f"{prefix}POPUP {rc_string(item.text)}{rc_options(item.flags, ignored=0x90)}"
            )
            lines.append(f"{prefix}BEGIN")
            lines.extend(render_menu_items(item.children, indent + 4))
            lines.append(f"{prefix}END")
            continue

        if item.item_id == 0 and item.text == "":
            lines.append(f"{prefix}MENUITEM SEPARATOR")
        else:
            lines.append(
                f"{prefix}MENUITEM {rc_string(item.text)}, {item.item_id}"
                f"{rc_options(item.flags, ignored=0x80)}"
            )
    return lines


def render_menu(name: int | str, payload: bytes) -> list[str]:
    lines = [f"{rc_name(name)} MENU", "BEGIN"]
    lines.extend(render_menu_items(parse_menu_resource(payload), 4))
    lines.append("END")
    return lines


def dialog_class_name(value: int | str | None) -> str:
    if value is None:
        return '""'
    if isinstance(value, int):
        return rc_string(STANDARD_DIALOG_CLASSES.get(value, f"#{value}"))
    return rc_string(value)


def parse_dialog_resource(payload: bytes) -> DialogTemplate:
    if len(payload) >= 4 and struct.unpack_from("<HH", payload, 0) == (1, 0xFFFF):
        raise ResourceExtractError("DIALOGEX resources are not currently supported")

    offset = 0
    dialog_style, dialog_exstyle, control_count, dialog_x, dialog_y, dialog_cx, dialog_cy = (
        struct.unpack_from("<IIHhhhh", payload, offset)
    )
    offset += 18
    menu, offset = read_utf16_or_ordinal(payload, offset)
    window_class, offset = read_utf16_or_ordinal(payload, offset)
    caption, offset = read_utf16_or_ordinal(payload, offset)
    if caption is not None and not isinstance(caption, str):
        raise ResourceExtractError("dialog caption cannot be an ordinal")

    font_size: int | None = None
    font_face: str | None = None
    if dialog_style & DS_SETFONT:
        font_size = struct.unpack_from("<H", payload, offset)[0]
        offset += 2
        font_face_value, offset = read_utf16_or_ordinal(payload, offset)
        if not isinstance(font_face_value, str):
            raise ResourceExtractError("dialog font face cannot be an ordinal")
        font_face = font_face_value

    controls: list[DialogControl] = []
    offset = align4_offset(offset)
    for _index in range(control_count):
        if offset + 18 > len(payload):
            raise ResourceExtractError("dialog control header extends past payload")
        control_style, control_exstyle, control_x, control_y, control_cx, control_cy, control_id = struct.unpack_from(
            "<IIhhhhH", payload, offset
        )
        offset += 18
        control_class, offset = read_utf16_or_ordinal(payload, offset)
        title, offset = read_utf16_or_ordinal(payload, offset)
        extra_size = struct.unpack_from("<H", payload, offset)[0]
        offset += 2
        creation_data = payload[offset : offset + extra_size]
        offset += extra_size
        offset = align4_offset(offset)
        controls.append(
            DialogControl(
                control_style,
                control_exstyle,
                control_x,
                control_y,
                control_cx,
                control_cy,
                control_id,
                control_class if control_class is not None else "",
                title,
                creation_data,
            )
        )

    if offset - len(payload) not in (0, 2):
        raise ResourceExtractError("dialog resource parse did not consume the payload")
    if any(control.creation_data for control in controls):
        raise ResourceExtractError("dialog creation data is not currently supported")

    return DialogTemplate(
        dialog_style,
        dialog_exstyle,
        dialog_x,
        dialog_y,
        dialog_cx,
        dialog_cy,
        menu,
        window_class,
        caption,
        font_size,
        font_face,
        controls,
    )


def render_dialog(name: int | str, payload: bytes) -> list[str]:
    dialog = parse_dialog_resource(payload)
    lines = [
        f"{rc_name(name)} DIALOG {dialog.x}, {dialog.y}, {dialog.cx}, {dialog.cy}",
        f"STYLE {rc_hex(dialog.style)}",
    ]
    if dialog.exstyle:
        lines.append(f"EXSTYLE {rc_hex(dialog.exstyle)}")
    if dialog.menu is not None:
        lines.append(f"MENU {rc_text_or_ordinal(dialog.menu)}")
    if dialog.window_class is not None:
        lines.append(f"CLASS {rc_text_or_ordinal(dialog.window_class)}")
    if dialog.caption is not None:
        lines.append(f"CAPTION {rc_string(dialog.caption)}")
    if dialog.font_size is not None and dialog.font_face is not None:
        lines.append(f"FONT {dialog.font_size}, {rc_string(dialog.font_face)}")
    lines.append("BEGIN")
    for control in dialog.controls:
        exstyle = f", {rc_hex(control.exstyle)}" if control.exstyle else ""
        lines.append(
            f"    CONTROL {rc_text_or_ordinal(control.title)}, {rc_int(control.control_id)}, "
            f"{dialog_class_name(control.control_class)}, {rc_hex(control.style)}, "
            f"{control.x}, {control.y}, {control.cx}, {control.cy}{exstyle}"
        )
    lines.append("END")
    return lines


def parse_string_table(payload: bytes, block_id: int) -> list[tuple[int, str]]:
    entries: list[tuple[int, str]] = []
    offset = 0
    first_id = (block_id - 1) * 16
    for index in range(16):
        if offset + 2 > len(payload):
            raise ResourceExtractError("STRINGTABLE block is truncated")
        length = struct.unpack_from("<H", payload, offset)[0]
        offset += 2
        text = payload[offset : offset + length * 2].decode("utf-16le", errors="replace")
        offset += length * 2
        if text:
            entries.append((first_id + index, text))
    if offset != len(payload):
        raise ResourceExtractError("STRINGTABLE parse did not consume the payload")
    return entries


def render_string_table(name: int | str, payload: bytes) -> list[str]:
    if not isinstance(name, int):
        raise ResourceExtractError("STRINGTABLE block name must be numeric")
    lines = ["STRINGTABLE", "BEGIN"]
    for string_id, text in parse_string_table(payload, name):
        lines.append(f"    {string_id} {rc_string(text)}")
    lines.append("END")
    return lines


def parse_version_value(payload: bytes, offset: int) -> tuple[str, int, int, bytes, int, int]:
    start = offset
    length, value_length, value_type = struct.unpack_from("<HHH", payload, offset)
    offset += 6
    key, offset = read_utf16_text(payload, offset)
    offset = align4_offset(offset)
    value_offset = offset
    value_size = value_length * 2 if value_type == 1 else value_length
    value = payload[value_offset : value_offset + value_size]
    offset = align4_offset(value_offset + value_size)
    return key, value_length, value_type, value, offset, start + length


def render_version_info(name: int | str, payload: bytes) -> list[str]:
    key, value_length, _value_type, value, child_offset, end_offset = parse_version_value(payload, 0)
    if key != "VS_VERSION_INFO" or value_length != 52:
        raise ResourceExtractError("unexpected VERSIONINFO root")
    (
        signature,
        _struct_version,
        file_version_ms,
        file_version_ls,
        product_version_ms,
        product_version_ls,
        flags_mask,
        flags,
        file_os,
        file_type,
        file_subtype,
        _date_ms,
        _date_ls,
    ) = struct.unpack("<13I", value)
    if signature != 0xFEEF04BD:
        raise ResourceExtractError("VERSIONINFO fixed file info signature mismatch")

    def version_tuple(ms: int, ls: int) -> tuple[int, int, int, int]:
        return (ms >> 16, ms & 0xFFFF, ls >> 16, ls & 0xFFFF)

    strings: list[tuple[str, str]] = []
    translations: list[tuple[int, int]] = []
    offset = child_offset
    while offset < end_offset:
        child_key, _child_len, _child_type, _child_value, grandchild_offset, child_end = (
            parse_version_value(payload, offset)
        )
        if child_key == "StringFileInfo":
            table_offset = grandchild_offset
            while table_offset < child_end:
                _table_key, _table_len, _table_type, _table_value, string_offset, table_end = (
                    parse_version_value(payload, table_offset)
                )
                while string_offset < table_end:
                    string_key, _string_len, string_type, string_value, _next, string_end = (
                        parse_version_value(payload, string_offset)
                    )
                    if string_type != 1:
                        raise ResourceExtractError("VERSIONINFO string value is not text")
                    strings.append(
                        (
                            string_key,
                            string_value.decode("utf-16le", errors="replace").rstrip("\0"),
                        )
                    )
                    string_offset = align4_offset(string_end)
                table_offset = align4_offset(table_end)
        elif child_key == "VarFileInfo":
            var_offset = grandchild_offset
            while var_offset < child_end:
                var_key, _var_len, _var_type, var_value, _next, var_end = parse_version_value(
                    payload, var_offset
                )
                if var_key == "Translation":
                    for index in range(0, len(var_value), 4):
                        language, codepage = struct.unpack_from("<HH", var_value, index)
                        translations.append((language, codepage))
                var_offset = align4_offset(var_end)
        offset = align4_offset(child_end)

    file_version = version_tuple(file_version_ms, file_version_ls)
    product_version = version_tuple(product_version_ms, product_version_ls)
    lines = [
        f"{rc_name(name)} VERSIONINFO",
        "FILEVERSION " + ",".join(str(part) for part in file_version),
        "PRODUCTVERSION " + ",".join(str(part) for part in product_version),
        f"FILEFLAGSMASK {rc_hex(flags_mask)}L",
        f"FILEFLAGS {rc_hex(flags)}L",
        f"FILEOS {rc_hex(file_os)}L",
        f"FILETYPE {rc_hex(file_type)}L",
        f"FILESUBTYPE {rc_hex(file_subtype)}L",
        "BEGIN",
        '    BLOCK "StringFileInfo"',
        "    BEGIN",
        '        BLOCK "040904b0"',
        "        BEGIN",
    ]
    for string_key, string_value in strings:
        lines.append(f"            VALUE {rc_string(string_key)}, {rc_string(string_value + chr(0))}")
    lines.extend(
        [
            "        END",
            "    END",
            '    BLOCK "VarFileInfo"',
            "    BEGIN",
        ]
    )
    for language, codepage in translations:
        lines.append(f'        VALUE "Translation", {rc_hex(language)}, {codepage}')
    lines.extend(["    END", "END"])
    return lines


def icon_file_from_group_icon(group_icon: bytes, icon_payloads: list[bytes]) -> bytes:
    if len(group_icon) < 6:
        raise ResourceExtractError("group icon resource is too small")
    reserved, icon_type, count = struct.unpack_from("<HHH", group_icon, 0)
    if reserved != 0 or icon_type != 1:
        raise ResourceExtractError("group icon resource is not an icon directory")
    if count != len(icon_payloads):
        raise ResourceExtractError("group icon count does not match RT_ICON payload count")

    icon_dir_entry_size = 16
    group_entry_size = 14
    header_size = 6 + icon_dir_entry_size * count
    output = bytearray(struct.pack("<HHH", reserved, icon_type, count))
    data_offset = header_size
    for index in range(count):
        entry_offset = 6 + group_entry_size * index
        width, height, colors, _reserved, planes, bit_count, size, _resource_id = struct.unpack_from(
            "<BBBBHHIH", group_icon, entry_offset
        )
        output.extend(
            struct.pack(
                "<BBBBHHII",
                width,
                height,
                colors,
                0,
                planes,
                bit_count,
                size,
                data_offset,
            )
        )
        data_offset += size

    for payload in icon_payloads:
        output.extend(payload)
    return bytes(output)


def manifest_for_entries(
    reference: Path, rsrc_rva: int, rsrc_size: int, entries: list[ResourceEntry]
) -> dict:
    manifest_entries = []
    for entry in entries:
        entry_data = asdict(entry)
        if not entry_data.get("raw_path"):
            entry_data.pop("raw_path", None)
        manifest_entries.append(
            {
                **entry_data,
                "type": resource_label(entry.type_id),
                "type_name": resource_type_label(entry.type_id),
                "name_label": resource_label(entry.name),
                "language_hex": hex32(entry.language),
                "rva_hex": hex32(entry.rva),
                "file_offset_hex": hex32(entry.file_offset),
            }
        )

    return {
        "reference": str(reference).replace("\\", "/"),
        "resource_directory": {
            "rva": hex32(rsrc_rva),
            "size": hex32(rsrc_size),
        },
        "entries": manifest_entries,
    }


def make_resource_header(entries: list[ResourceEntry]) -> str:
    lines = [
        "#pragma once",
        "",
        "// Generated from support/Recoil.exe by tools/recoil_resource_extract.py.",
        "// Keep these IDs aligned with the original PE resource table.",
        "",
    ]

    for entry in entries:
        if isinstance(entry.type_id, int) and entry.type_id == 4 and isinstance(entry.name, str):
            lines.append(f'#define IDR_{entry.name} "{entry.name}"')
        elif isinstance(entry.type_id, int) and entry.type_id == 2 and isinstance(entry.name, str):
            lines.append(f'#define IDB_{entry.name} "{entry.name}"')
        elif isinstance(entry.type_id, int) and entry.type_id == 5 and isinstance(entry.name, int):
            lines.append(f"#define IDD_RECOIL_DIALOG_{entry.name} {entry.name}")
        elif isinstance(entry.type_id, int) and entry.type_id == 14 and isinstance(entry.name, int):
            lines.append(f"#define IDI_RECOIL {entry.name}")

    lines.extend(
        [
            "#define IDS_GAME_FILE_FILTER 200",
            "#define IDS_ARIAL_BLACK 206",
            "",
        ]
    )
    return "\n".join(lines)


def path_for_rc(path: Path, repo_root: Path) -> str:
    try:
        return path.relative_to(repo_root).as_posix()
    except ValueError:
        return path.as_posix()


def make_rc_file(entries: list[ResourceEntry], img_dir: Path, repo_root: Path, data: bytes) -> str:
    lines = [
        '#include "Resource.h"',
        "",
        "// Generated from support/Recoil.exe by tools/recoil_resource_extract.py.",
        "// Reconstructed from the original PE resource table into normal VC6 .rc source.",
        "",
        "LANGUAGE 9, 1",
        "",
    ]

    for entry in entries:
        payload = data[entry.file_offset : entry.file_offset + entry.size]
        if entry.type_id == 2 and isinstance(entry.name, str):
            lines.append(
                f'{rc_name(entry.name)} BITMAP "{path_for_rc(img_dir / (entry.name + ".bmp"), repo_root)}"'
            )
        elif entry.type_id == 14 and entry.name == 151:
            lines.append(f'151 ICON "{path_for_rc(img_dir / "RECOIL.ico", repo_root)}"')
        elif entry.type_id == 3:
            continue
        elif entry.type_id == 4:
            lines.extend(render_menu(entry.name, payload))
        elif entry.type_id == 5:
            lines.extend(render_dialog(entry.name, payload))
        elif entry.type_id == 6:
            lines.extend(render_string_table(entry.name, payload))
        elif entry.type_id == 16:
            lines.extend(render_version_info(entry.name, payload))
        else:
            raise ResourceExtractError(f"unsupported resource type for .rc output: {entry.type_id!r}")
        lines.append("")
    lines.append("")
    return "\n".join(lines)


def extract_resources(
    reference: Path,
    raw_dir: Path | None,
    img_dir: Path,
    manifest_path: Path,
    res_path: Path | None,
    rc_path: Path | None,
    header_path: Path | None,
) -> list[ResourceEntry]:
    repo_root = Path.cwd()
    rsrc_rva, rsrc_size, entries, data = parse_resources(reference)

    icon_payloads: list[bytes] = []
    written_entries: list[ResourceEntry] = []
    for entry in entries:
        payload = data[entry.file_offset : entry.file_offset + entry.size]
        raw_name = raw_resource_filename(entry)
        if raw_dir is not None:
            write_if_changed(raw_dir / raw_name, payload)
        written_entry = ResourceEntry(
            **{
                **asdict(entry),
                "raw_path": raw_name if raw_dir is not None else "",
            }
        )
        written_entries.append(written_entry)

        if entry.type_id == 2 and isinstance(entry.name, str):
            write_if_changed(img_dir / f"{entry.name}.bmp", bitmap_file_from_dib(payload))
        elif entry.type_id == 3:
            icon_payloads.append(payload)

    for entry in written_entries:
        if entry.type_id != 14:
            continue
        payload = data[entry.file_offset : entry.file_offset + entry.size]
        icon_name = "RECOIL" if entry.name == 151 else resource_file_token(entry.name)
        write_if_changed(img_dir / f"{icon_name}.ico", icon_file_from_group_icon(payload, icon_payloads))

    manifest = manifest_for_entries(reference, rsrc_rva, rsrc_size, written_entries)
    write_text_if_changed(manifest_path, json.dumps(manifest, indent=2, sort_keys=True) + "\n")

    if res_path is not None:
        write_if_changed(res_path, make_res_file(written_entries, data))
    if rc_path is not None:
        write_text_if_changed(rc_path, make_rc_file(written_entries, img_dir, repo_root, data))
    if header_path is not None:
        write_text_if_changed(header_path, make_resource_header(written_entries))

    return written_entries


def print_summary(entries: list[ResourceEntry]) -> None:
    totals: dict[int | str, int] = {}
    for entry in entries:
        totals[entry.type_id] = totals.get(entry.type_id, 0) + 1
    for type_id, count in sorted(totals.items(), key=lambda item: str(item[0])):
        print(f"{resource_label(type_id)} {resource_type_label(type_id)}: {count}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Extract Recoil.exe resources for rebuilt .rc use.")
    parser.add_argument("--reference", default="support/Recoil.exe")
    parser.add_argument("--raw-dir", default="build/resource-reconstruct/raw")
    parser.add_argument("--img-dir", default="img")
    parser.add_argument("--manifest", default=".agent/RESOURCE_MANIFEST.json")
    parser.add_argument("--res", default="build/resource-reconstruct/Recoil-original.res")
    parser.add_argument("--rc", default="src/Battlesport/Recoil.rc")
    parser.add_argument("--header", default="src/Battlesport/Resource.h")
    parser.add_argument("--write-raw", action="store_true")
    parser.add_argument("--write-res", action="store_true")
    parser.add_argument("--no-rc", action="store_true")
    parser.add_argument("--compare-res", help="Compare a compiled .res file to the reference resources.")
    args = parser.parse_args(argv)

    try:
        if args.compare_res:
            mismatches = compare_res_payloads(Path(args.reference), Path(args.compare_res))
            if mismatches:
                print("Compiled resources differ from reference:", file=sys.stderr)
                for mismatch in mismatches:
                    print(f"- {mismatch}", file=sys.stderr)
                return 1
            print("Compiled resources match reference payloads.")
            return 0

        entries = extract_resources(
            Path(args.reference),
            Path(args.raw_dir) if args.write_raw else None,
            Path(args.img_dir),
            Path(args.manifest),
            Path(args.res) if args.write_res else None,
            None if args.no_rc else Path(args.rc),
            None if args.no_rc else Path(args.header),
        )
    except (OSError, ResourceExtractError, struct.error) as exc:
        print(str(exc), file=sys.stderr)
        return 1

    print_summary(entries)
    print(f"Extracted {len(entries)} resource payloads.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
