"""Fail-closed scalar-store interpretation of straight-line x86 initializers.

This small vocabulary proves final constant bytes written through entry ECX.
It does not execute the game or infer omitted stores from allocator contents.
Unsupported control flow, addressing, writes, or instruction forms fail closed.
"""
from __future__ import annotations


def constant_member_bytes(data: bytes, *, object_size: int) -> dict[int, int]:
    registers = [None] * 8
    registers[1] = ("this", 0)
    memory: dict[int, int] = {}
    cursor = 0
    stack = []

    def take(count):
        nonlocal cursor
        result = data[cursor:cursor + count]
        if len(result) != count:
            raise ValueError("truncated initializer instruction")
        cursor += count
        return result

    def address():
        modrm = take(1)[0]
        mode, reg, base = modrm >> 6, modrm >> 3 & 7, modrm & 7
        if mode == 3:
            return reg, base, None
        if base == 4:
            raise ValueError("unsupported initializer indexed addressing")
        if mode == 0 and base == 5:
            take(4)
            return reg, None, ("absolute", 0)
        displacement = int.from_bytes(take((0, 1, 4)[mode]), "little", signed=True)
        value = registers[base]
        return reg, None, (value[0], value[1] + displacement) if value else None

    def write(destination, value, width):
        if destination is None or destination[0] != "this":
            raise ValueError("initializer write lacks entry-this provenance")
        offset = destination[1]
        if not 0 <= offset <= object_size - width:
            raise ValueError("initializer store exceeds object extent")
        for index in range(width):
            if value is not None and value[0] == "constant":
                memory[offset + index] = value[1] >> (8 * index) & 255
            else:
                memory.pop(offset + index, None)

    while cursor < len(data):
        opcode = take(1)[0]
        if opcode == 0xC3:
            if stack or registers[0] != ("this", 0):
                raise ValueError("initializer lacks balanced stack and return-this")
            tail = data[cursor:]
            if len(tail) >= 16 or any(byte != 0x90 for byte in tail):
                raise ValueError("unexplained initializer tail")
            return memory
        if 0x50 <= opcode <= 0x57:
            stack.append(registers[opcode - 0x50])
        elif 0x58 <= opcode <= 0x5F:
            if not stack:
                raise ValueError("initializer stack underflow")
            registers[opcode - 0x58] = stack.pop()
        elif 0xB8 <= opcode <= 0xBF:
            registers[opcode - 0xB8] = ("constant", int.from_bytes(take(4), "little"))
        elif opcode in (0x33, 0x31):
            reg, rm, operand = address()
            if reg != rm or operand is not None:
                raise ValueError("initializer XOR is not a register zero")
            registers[reg] = ("constant", 0)
        elif opcode in (0x88, 0x89, 0x8B, 0x8D, 0xC6, 0xC7):
            reg, rm, operand = address()
            if opcode == 0x88:
                if rm is not None or reg > 3:
                    raise ValueError("unsupported initializer byte register")
                write(operand, registers[reg], 1)
            elif opcode in (0xC6, 0xC7):
                width = 1 if opcode == 0xC6 else 4
                if reg or rm is not None:
                    raise ValueError("unsupported initializer immediate store")
                write(operand, ("constant", int.from_bytes(take(width), "little")), width)
            elif opcode == 0x8D:
                if rm is not None:
                    raise ValueError("invalid initializer LEA")
                registers[reg] = operand
            elif rm is not None:
                destination, source = (reg, rm) if opcode == 0x8B else (rm, reg)
                registers[destination] = registers[source]
            elif opcode == 0x89:
                write(operand, registers[reg], 4)
            else:
                registers[reg] = None
        elif opcode == 0xA1:
            take(4)
            registers[0] = None
        elif opcode in (0xD9, 0xDD):
            operation, rm, operand = address()
            if rm is not None or operation not in (0, 3):
                raise ValueError("unsupported initializer FPU instruction")
            if operation == 3:
                write(operand, None, 4 if opcode == 0xD9 else 8)
        elif opcode == 0xF3 and take(1) == b"\xab":
            count = registers[1]
            if count is None or count[0] != "constant" or not 0 <= count[1] <= object_size // 4:
                raise ValueError("initializer REP count is not bounded")
            for _ in range(count[1]):
                write(registers[7], registers[0], 4)
                registers[7] = ("this", registers[7][1] + 4)
            registers[1] = ("constant", 0)
        else:
            raise ValueError(f"unsupported initializer opcode {opcode:#x} at {cursor - 1:#x}")
    raise ValueError("initializer has no return")
