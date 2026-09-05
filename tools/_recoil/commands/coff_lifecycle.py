"""Read-only COFF lifecycle inventory; reports observations, never acceptance."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from _recoil.commands.asm_verify import CoffObject
from _recoil.commands.vc5_build import parse_link_map
from _recoil.lib.tooling import configure_stdio


def inventory(path: Path, names: list[str]) -> dict[str, object]:
    obj = CoffObject.from_path(path)
    members = {}
    for name in names:
        definitions = [row for row in obj.symbols if row.name == name and row.section_number > 0]
        rows = []
        for definition in definitions:
            body = obj.function_bytes(name)
            section = obj.section(definition.section_number)
            rows.append({
                "symbol_index": definition.index,
                "section_number": definition.section_number,
                "section_name": section.name,
                "characteristics": section.characteristics,
                "start": body.start,
                "end": body.end,
                "bytes": body.data.hex(),
                "comdat_selections": [row.section_definition_selection for row in obj.symbols
                                      if row.section_number == definition.section_number
                                      and row.section_definition_selection is not None],
                "associative_sections": [
                    {"section_number": row.section_number,
                     "name": obj.section(row.section_number).name,
                     "characteristics": obj.section(row.section_number).characteristics}
                    for row in obj.symbols if row.section_definition_selection == 5
                    and row.section_definition_association == definition.section_number
                ],
                "relocations": [{"offset": row.offset, "type": row.type, "target": row.symbol_name,
                                 "symbol_index": row.symbol_index}
                                for row in body.relocations],
            })
        members[name] = {
            "definition_count": len(definitions),
            "definitions": rows,
            "inbound_relocations": [
                {"section_number": index, "offset": row.offset, "type": row.type,
                 "target": row.symbol_name, "symbol_index": row.symbol_index,
                 "section_symbols": [symbol.name for symbol in obj.symbols
                                     if symbol.section_number == index and symbol.storage_class == 2]}
                for index, relocations in obj.relocations_by_section.items()
                for row in relocations if row.symbol_name == name
            ],
        }
    return {"kind": "coff-lifecycle-inventory", "object": path.as_posix(),
            "acceptance": False, "candidate_expected_truth": False, "members": members}


def main() -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--object", type=Path, required=True)
    parser.add_argument("--symbol", action="append", required=True)
    parser.add_argument("--map", type=Path, action="append", default=[])
    args = parser.parse_args()
    try:
        report = inventory(args.object, args.symbol)
        report["maps"] = {}
        for path in args.map:
            parsed = parse_link_map(path)
            report["maps"][path.as_posix()] = {
                name: [hex(row.address) for row in parsed.symbols
                       if row.symbol == name and row.is_function]
                for name in args.symbol
            }
        print(json.dumps(report, indent=2))
    except (OSError, ValueError) as exc:
        parser.exit(1, f"COFF lifecycle inventory: {exc}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
