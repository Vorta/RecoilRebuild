from __future__ import annotations

import argparse
from dataclasses import dataclass
from difflib import unified_diff
from pathlib import Path
import re
import sys

from recoil_binja import BinaryNinjaBridge, BridgeError
from recoil_plan import normalize_address


HEX_BYTE_RE = re.compile(r"^[0-9A-Fa-f]{2}$")
BN_ADDR_RE = re.compile(r"^[0-9A-Fa-f]{8}$")
COD_OFFSET_RE = re.compile(r"^[0-9A-Fa-f]{5}$")
DECIMAL_RE = re.compile(r"^-?\d+$")
HEX_RE = re.compile(r"^0x[0-9a-fA-F]+$")


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


def byte_string(instruction: Instruction) -> str:
    return " ".join(instruction.bytes) if instruction.bytes else "<no bytes>"


def mnemonic(text: str) -> str:
    return text.split(" ", 1)[0].lower()


def has_symbol_relocation_text(instruction: Instruction) -> bool:
    raw_text = instruction.raw_text
    return (
        "?" in raw_text
        or "OFFSET " in raw_text.upper()
        or "__imp" in raw_text.lower()
    )


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
    if cod_bytes[0] == bn_bytes[0] in {"68", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf"} and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[0] == bn_bytes[0] == "a3" and cod_bytes[1:] == ("00", "00", "00", "00"):
        return True
    if cod_bytes[0] == bn_bytes[0] == "a1" and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[:2] == bn_bytes[:2] and cod_bytes[0] == "89" and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[:2] == bn_bytes[:2] and cod_bytes[0] == "c7" and cod_bytes[-4:] == ("00", "00", "00", "00"):
        return True
    if cod_bytes[:2] == bn_bytes[:2] and cod_bytes[0] == "8b" and has_symbol_relocation_text(cod_instruction):
        return True
    if cod_bytes[:2] == bn_bytes[:2] == ("8b", "0d") and cod_bytes[2:] == ("00", "00", "00", "00"):
        return True
    return False


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

    for index in range(max_len):
        bn_instruction = bn_instructions[index] if index < len(bn_instructions) else None
        cod_instruction = cod_instructions[index] if index < len(cod_instructions) else None
        if bn_instruction is None or cod_instruction is None:
            mismatches += 1
            lines.append(f"[mismatch] instruction count diverges at index {index + 1}")
            lines.append(f"  BN:  {bn_instruction.text if bn_instruction else '<missing>'}")
            lines.append(f"  COD: {cod_instruction.text if cod_instruction else '<missing>'}")
            continue

        if bn_instruction.text == cod_instruction.text:
            if bn_instruction.raw_text == cod_instruction.raw_text:
                exact += 1
            else:
                normalized_spelling += 1
            continue

        if bn_instruction.bytes and bn_instruction.bytes == cod_instruction.bytes:
            byte_identical += 1
            lines.append(f"[byte-identical spelling] instruction {index + 1}")
            lines.append(f"  bytes: {byte_string(bn_instruction)}")
            lines.append(f"  BN:    {bn_instruction.raw_text}")
            lines.append(f"  COD:   {cod_instruction.raw_text}")
            continue

        if is_relocation_sensitive(bn_instruction, cod_instruction):
            relocation_sensitive += 1
            lines.append(f"[relocation-sensitive] instruction {index + 1}")
            lines.append(f"  BN bytes:  {byte_string(bn_instruction)}")
            lines.append(f"  COD bytes: {byte_string(cod_instruction)}")
            lines.append(f"  BN:        {bn_instruction.raw_text}")
            lines.append(f"  COD:       {cod_instruction.raw_text}")
            continue

        mismatches += 1
        lines.append(f"[mismatch] instruction {index + 1}")
        lines.append(f"  BN bytes:  {byte_string(bn_instruction)}")
        lines.append(f"  COD bytes: {byte_string(cod_instruction)}")
        lines.append(f"  BN:        {bn_instruction.raw_text}")
        lines.append(f"  COD:       {cod_instruction.raw_text}")

    summary = [
        f"exact_or_normalized_matches: {exact + normalized_spelling}",
        f"  exact_text_matches: {exact}",
        f"  normalized_spelling_matches: {normalized_spelling}",
        f"byte_identical_spelling_differences: {byte_identical}",
        f"relocation_sensitive_differences: {relocation_sensitive}",
        f"mismatches: {mismatches}",
        "",
    ]
    return [*summary, *lines], mismatches


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
        if begin is not None and any(marker in stripped for marker in end_markers):
            end = index + 1
            break

    if begin is None:
        raise ValueError(f"Symbol not found in COD listing: {symbol}")
    return "\n".join(lines[begin:end])


def write_lines(path: Path, lines: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")


def compare_bn_to_cod(
    *,
    address: str,
    cod_path: Path,
    symbol: str | None,
    out_dir: Path,
    bridge_url: str,
) -> AssemblyComparison:
    address = normalize_address(address)
    safe_address = address[2:]
    bridge = BinaryNinjaBridge(bridge_url)

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
    parser = argparse.ArgumentParser(description="Extract and normalize BN/MSVC assembly evidence for a function.")
    parser.add_argument("address", help="Binary Ninja function address, e.g. 0x407170")
    parser.add_argument("--cod", help="MSVC .cod listing to compare against")
    parser.add_argument("--symbol", help="MSVC symbol name to extract from the .cod listing")
    parser.add_argument("--out-dir", default="build/verification", help="Directory for normalized evidence")
    parser.add_argument("--bridge-url", default="http://localhost:9009", help="Binary Ninja bridge URL")
    parser.add_argument("--fail-on-diff", action="store_true", help="Return non-zero when normalized assembly differs")
    args = parser.parse_args(argv)

    out_dir = Path(args.out_dir)

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
        print("Assembly has instruction mismatches; review the classified diff before marking Binary-safe verified.")
        return 1 if args.fail_on_diff else 0
    if comparison.diff_count:
        print(
            "No instruction mismatches found; normalized assembly differs by spelling or "
            "relocation-sensitive items. Review the classified diff before marking Binary-safe verified."
        )
        return 1 if args.fail_on_diff else 0
    print("Normalized assembly matches.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
