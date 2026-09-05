"""Exact dependency checks for simple leaf x86 C++ constructors.

Only a fully decoded straight-line MOV/RET constructor is admitted. This proves
the vptr write through entry ECX; finding a table relocation somewhere in an
object is not enough. More complex constructors must use a separate CFG proof.
"""
from __future__ import annotations

import struct
from typing import Sequence


def leaf_constructor_vptr_write(instructions: Sequence[bytes]) -> tuple[int, int]:
    aliases = {1}  # x86 encoding for entry ECX / this
    writes: list[tuple[int, int]] = []
    offset = 0
    returned = False
    for data in instructions:
        if returned or not data:
            raise ValueError("constructor has trailing or empty decoded instructions")
        if len(data) == 2 and data[0] in {0x8B, 0x89} and data[1] >= 0xC0:
            destination, source = (data[1] >> 3) & 7, data[1] & 7
            if data[0] == 0x89:
                destination, source = source, destination
            if source in aliases:
                aliases.add(destination)
            else:
                aliases.discard(destination)
        elif data[0] == 0xC7 and len(data) >= 6:
            modrm = data[1]
            mode, opcode_extension, base = modrm >> 6, (modrm >> 3) & 7, modrm & 7
            if opcode_extension or base == 4 or (mode == 0 and base == 5) or mode == 3:
                raise ValueError("constructor store is not a direct this-relative immediate")
            displacement_size = (0, 1, 4)[mode]
            if len(data) != 6 + displacement_size or base not in aliases:
                raise ValueError("constructor store lacks exact this provenance")
            displacement = int.from_bytes(data[2:2 + displacement_size], "little", signed=True)
            if displacement == 0:
                writes.append((offset + 2 + displacement_size, struct.unpack("<I", data[-4:])[0]))
        elif data in {b"\xc3", b"\xc2\x00\x00"}:
            returned = True
        else:
            raise ValueError("constructor dependency requires a leaf MOV/RET body")
        offset += len(data)
    if not returned or len(writes) != 1:
        raise ValueError("constructor requires exactly one complete-object vptr store")
    return writes[0]


def exact_table_slots(data: bytes, relocations: Sequence[object], slot_count: int) -> tuple[str, ...]:
    """Every table cell must have one DIR32 relocation and a zero addend."""
    if slot_count <= 0 or len(data) != slot_count * 4 or any(data):
        raise ValueError("dispatch table has wrong extent or nonzero addends")
    by_offset: dict[int, str] = {}
    for relocation in relocations:
        offset = relocation.offset
        if relocation.type != 6 or offset % 4 or not 0 <= offset < len(data) or offset in by_offset:
            raise ValueError("dispatch table has invalid, overlapping, or non-DIR32 relocation")
        if not relocation.symbol_name:
            raise ValueError("dispatch table has unnamed target")
        by_offset[offset] = relocation.symbol_name
    if set(by_offset) != set(range(0, len(data), 4)):
        raise ValueError("dispatch table has an unbound slot")
    return tuple(by_offset[offset] for offset in range(0, len(data), 4))


def validate_leaf_listing_body(instructions: Sequence[bytes], data: bytes,
                               relocations: Sequence[object]) -> None:
    """Accept only the complete leaf body and bounded post-return alignment."""
    leaf_constructor_vptr_write(instructions)
    body = b"".join(instructions)
    tail = data[len(body):]
    if (not data.startswith(body) or len(tail) >= 16
            or (tail and (len(data) % 16 or any(byte != 0x90 for byte in tail)))
            or any(row.offset < 0 or row.offset + 4 > len(body) for row in relocations)):
        raise ValueError("constructor listing/COFF mismatch or unexplained post-return bytes")


def resolve_table_weak_target(relocation: object, symbols: Sequence[object]) -> str:
    """Follow one exact COFF weak-external default, never a spelling heuristic."""
    rows = [row for row in symbols if row.index == relocation.symbol_index]
    if len(rows) != 1 or rows[0].name != relocation.symbol_name:
        raise ValueError("dispatch relocation lacks its exact COFF symbol")
    row = rows[0]
    if row.storage_class != 105:
        if row.storage_class != 2 or row.symbol_type != 0x20:
            raise ValueError("dispatch target is not an external function")
        return row.name
    defaults = [item for item in symbols if item.index == row.weak_external_tag_index]
    if (row.section_number != 0 or row.value != 0 or row.symbol_type != 0x20
            or row.aux_count != 1 or row.weak_external_characteristics != 2
            or len(defaults) != 1):
        raise ValueError("dispatch weak external lacks an exact search-library default")
    default = defaults[0]
    definitions = [item for item in symbols if item.name == default.name and item.section_number > 0]
    if (default.section_number != 0 or default.storage_class != 2
            or default.symbol_type != 0x20 or default.value != 0 or default.aux_count != 0
            or len(definitions) != 1 or definitions[0].storage_class != 2
            or definitions[0].symbol_type != 0x20
            or definitions[0].section_name != ".text"
            or not definitions[0].section_characteristics & 0x20):
        raise ValueError("dispatch weak default lacks its unique emitted function")
    return default.name


def straight_constructor_member_store(instructions: Sequence[bytes], member: int) -> tuple[int, int]:
    """Prove a single this-relative immediate store in a bounded VC5 constructor.

    Decode the complete straight-line normal path, retaining only exact register
    aliases of entry ECX across calls. Unknown instructions, control flow, or
    non-stack/non-this stores fail closed. Calls kill all volatile aliases.
    """
    aliases = {1: 0}
    writes = []
    offset = 0
    returned = False
    for data in instructions:
        if returned or not data:
            raise ValueError("member constructor has trailing/empty instructions")
        if data == b"\xc3":
            returned = True
        elif data[:1] == b"\xe8" and len(data) == 5:
            for register in (0, 1, 2):
                aliases.pop(register, None)
        elif ((len(data) == 1 and 0x50 <= data[0] <= 0x57)
              or (len(data) == 2 and data[0] == 0x6A)
              or (len(data) == 5 and data[0] == 0x68)
              or (len(data) == 3 and data[:2] in {b"\x83\xec", b"\x83\xc4"})):
            pass
        elif len(data) == 1 and 0x58 <= data[0] <= 0x5F:
            aliases.pop(data[0] - 0x58, None)
        elif data == bytes.fromhex("64 a1 00 00 00 00"):
            aliases.pop(0, None)
        elif data in {bytes.fromhex("64 89 25 00 00 00 00"), bytes.fromhex("64 89 0d 00 00 00 00")}:
            pass
        elif len(data) == 2 and data[0] in {0x8B, 0x89} and data[1] >= 0xC0:
            destination, source = (data[1] >> 3) & 7, data[1] & 7
            if data[0] == 0x89:
                destination, source = source, destination
            if source in aliases:
                aliases[destination] = aliases[source]
            else:
                aliases.pop(destination, None)
        elif data[0] in {0x8B, 0x89, 0x8D, 0xC6, 0xC7} and len(data) >= 2:
            mode, register, base = data[1] >> 6, (data[1] >> 3) & 7, data[1] & 7
            if mode == 3 or (mode == 0 and base == 5):
                raise ValueError("unproved constructor memory addressing")
            displacement_size = (0, 1, 4)[mode]
            stack = base == 4 and len(data) >= 3 and data[2] == 0x24
            address_end = 2 + (1 if stack else 0) + displacement_size
            immediate_size = 1 if data[0] == 0xC6 else 4 if data[0] == 0xC7 else 0
            if (base == 4 and not stack) or len(data) != address_end + immediate_size:
                raise ValueError("unproved constructor memory instruction extent")
            displacement = int.from_bytes(data[address_end - displacement_size:address_end], "little", signed=True) if displacement_size else 0
            if data[0] in {0xC6, 0xC7} and register:
                raise ValueError("unproved immediate-store opcode extension")
            if data[0] == 0x8D:
                if not stack and base in aliases:
                    aliases[register] = aliases[base] + displacement
                else:
                    aliases.pop(register, None)
            elif data[0] == 0x8B:
                if not stack:
                    raise ValueError("constructor memory load is not stack-local")
                aliases.pop(register, None)
            elif not stack:
                if data[0] != 0xC7 or base not in aliases:
                    raise ValueError("constructor store lacks exact this provenance")
                if aliases[base] + displacement == member:
                    writes.append((offset + address_end, int.from_bytes(data[-4:], "little")))
        else:
            raise ValueError("unsupported member-constructor instruction")
        offset += len(data)
    if not returned or len(writes) != 1:
        raise ValueError("constructor requires one unambiguous selected member store")
    return writes[0]


def constant_return(data: bytes, relocations: Sequence[object] = ()) -> int:
    """A complete nonrelocatable constant-return callback, with bounded padding."""
    if data.startswith(b"\x33\xc0\xc3"):
        end, value = 3, 0
    elif len(data) >= 6 and data[0] == 0xB8 and data[5] == 0xC3:
        end, value = 6, int.from_bytes(data[1:5], "little")
    else:
        raise ValueError("callback is not a complete constant-return body")
    tail = data[end:]
    if relocations or len(tail) >= 16 or (tail and (len(data) % 16 or any(byte != 0x90 for byte in tail))):
        raise ValueError("callback has unexplained relocation, code, or padding")
    return value


def constructor_vptr_store(instructions: Sequence[bytes]) -> tuple[int, int]:
    """Prove the same explicit complete-object stamp on every normal return.

    Bounded forward CFG, exact entry-this register/LEA provenance, and a small
    decoded x86 vocabulary. Calls clobber volatile registers. This proves the
    constructor's own stores, not arbitrary effects inside its callees (whose
    invocation contracts are checked separately). No table-name search stands
    in for the receiver proof. Unknown instructions/addressing fail closed.
    """
    offsets = []
    end = 0
    for data in instructions:
        offsets.append(end)
        end += len(data)
    by_offset = {value: index for index, value in enumerate(offsets)}
    pending = [(0, {1: 0}, None)]
    visited = set()
    returns = set()
    while pending:
        index, aliases, stamp = pending.pop()
        key = (index, tuple(sorted(aliases.items())), stamp)
        if key in visited:
            continue
        visited.add(key)
        if len(visited) > 4096 or index >= len(instructions):
            raise ValueError("constructor CFG is incomplete or exceeds proof bound")
        data, offset = instructions[index], offsets[index]
        if not data:
            raise ValueError("empty constructor instruction")
        aliases = dict(aliases)
        if data == b"\xc3" or (len(data) == 3 and data[0] == 0xC2):
            if stamp is None:
                raise ValueError("constructor return lacks a proven this-vptr stamp")
            returns.add(stamp)
            continue
        if ((len(data) == 2 and (0x70 <= data[0] <= 0x7F or data[0] == 0xEB))
                or (len(data) == 6 and data[0] == 0x0F and 0x80 <= data[1] <= 0x8F)
                or (len(data) == 5 and data[0] == 0xE9)):
            width = 1 if len(data) == 2 else 4
            target = offset + len(data) + int.from_bytes(data[-width:], "little", signed=True)
            if target not in by_offset or target <= offset:
                raise ValueError("constructor branch lacks an exact forward CFG target")
            pending.append((by_offset[target], aliases, stamp))
            if data[0] in {0xEB, 0xE9}:
                continue
        elif ((data[0] == 0xE8 and len(data) == 5)
              or (data[0] == 0xFF and len(data) >= 2 and (data[1] >> 3) & 7 == 2)):
            for register in (0, 1, 2):
                aliases.pop(register, None)
        elif ((len(data) == 1 and 0x50 <= data[0] <= 0x57)
              or (len(data) == 2 and data[0] == 0x6A)
              or (len(data) == 5 and data[0] == 0x68)
              or (len(data) == 3 and data[:2] in {b"\x83\xec", b"\x83\xc4"})
              or (len(data) == 6 and data[:2] in {b"\x81\xec", b"\x81\xc4"})):
            pass
        elif len(data) == 1 and 0x58 <= data[0] <= 0x5F:
            aliases.pop(data[0] - 0x58, None)
        elif len(data) == 5 and 0xB8 <= data[0] <= 0xBF:
            aliases.pop(data[0] - 0xB8, None)
        elif data == bytes.fromhex("64 a1 00 00 00 00"):
            aliases.pop(0, None)
        elif data in {bytes.fromhex("64 89 25 00 00 00 00"), bytes.fromhex("64 89 0d 00 00 00 00")}:
            pass
        elif len(data) == 2 and data[0] in {0x33, 0x85} and data[1] >= 0xC0:
            if data[0] == 0x33:
                aliases.pop((data[1] >> 3) & 7, None)
        elif len(data) == 2 and data[0] in {0x8B, 0x89} and data[1] >= 0xC0:
            dst, src = (data[1] >> 3) & 7, data[1] & 7
            if data[0] == 0x89:
                dst, src = src, dst
            aliases.pop(dst, None) if src not in aliases else aliases.update({dst: aliases[src]})
        elif data[0] in {0x8A, 0x88, 0x8B, 0x89, 0x8D, 0xC6, 0xC7} and len(data) >= 2:
            mode, register, base = data[1] >> 6, (data[1] >> 3) & 7, data[1] & 7
            if mode == 3:
                raise ValueError("unsupported constructor register instruction")
            stack = base == 4 and len(data) >= 3 and data[2] == 0x24
            absolute = mode == 0 and base == 5
            if base == 4 and not stack:
                raise ValueError("unsupported constructor indexed addressing")
            width = 4 if absolute or mode == 2 else 1 if mode == 1 else 0
            address_end = 2 + int(stack) + width
            immediate = 4 if data[0] == 0xC7 else 1 if data[0] == 0xC6 else 0
            if len(data) != address_end + immediate or (immediate and register):
                raise ValueError("constructor instruction extent/extension mismatch")
            displacement = int.from_bytes(data[address_end-width:address_end], "little", signed=True) if width else 0
            member = aliases.get(base) if not stack and not absolute else None
            if member is not None:
                member += displacement
            if data[0] == 0x8D:
                aliases.pop(register, None) if member is None else aliases.update({register: member})
            elif data[0] in {0x8A, 0x8B}:
                aliases.pop(register if data[0] == 0x8B else register % 4, None)
            elif (member is not None and member < 4
                  and member + (1 if data[0] in {0x88, 0xC6} else 4) > 0):
                if member or data[0] != 0xC7 or stamp is not None:
                    raise ValueError("ambiguous or overwritten constructor vptr")
                stamp = (offset + address_end, int.from_bytes(data[-4:], "little"))
            elif stamp is not None and member is None and not stack and not absolute:
                raise ValueError("post-stamp store has unknown receiver provenance")
        else:
            raise ValueError(f"unsupported constructor instruction: {data.hex()}")
        pending.append((index + 1, aliases, stamp))
    if len(returns) != 1:
        raise ValueError("constructor normal returns disagree on concrete vptr")
    return returns.pop()


def global_vptr_call_window(instructions: Sequence[bytes]) -> tuple[int, int, int]:
    """Bind a one-argument slot-zero call to its absolute receiver load.

    Admit only the complete MOV ECX,[global]; MOV reg,[argument]; PUSH reg;
    MOV EAX,[ECX]; CALL [EAX] window, with no branch into its interior.
    Return call index, receiver operand offset, and receiver address/addend.
    """
    offsets = []
    offset = 0
    for data in instructions:
        offsets.append(offset)
        offset += len(data)
    matches = []
    for index in range(len(instructions) - 4):
        receiver, argument, push, vptr, call = instructions[index:index + 5]
        if (len(receiver) != 6 or receiver[:2] != b"\x8b\x0d"
                or len(argument) != 6 or argument[0] != 0x8B
                or argument[1] & 0xC7 != 5):
            continue
        register = (argument[1] >> 3) & 7
        if (register in {1, 4} or push != bytes([0x50 + register])
                or vptr != b"\x8b\x01" or call != b"\xff\x10"):
            continue
        begin, end = offsets[index], offsets[index + 4] + 2
        for address, data in zip(offsets, instructions):
            width = (1 if len(data) == 2 and (0x70 <= data[0] <= 0x7F or data[0] == 0xEB)
                     else 4 if (len(data) == 6 and data[0] == 0x0F and 0x80 <= data[1] <= 0x8F)
                     or (len(data) == 5 and data[0] == 0xE9) else 0)
            if width and begin < address + len(data) + int.from_bytes(data[-width:], "little", signed=True) < end:
                raise ValueError("branch bypasses the virtual receiver definition")
        matches.append((index + 4, begin + 2, int.from_bytes(receiver[2:], "little")))
    if len(matches) != 1:
        raise ValueError("virtual update lacks one exact global-receiver window")
    return matches[0]
