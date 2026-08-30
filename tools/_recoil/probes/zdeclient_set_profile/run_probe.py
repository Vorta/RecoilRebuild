#!/usr/bin/env python3
"""Characterize the exact VC5 default-set profile needed by zDEClient."""

from __future__ import annotations

import argparse
from collections import Counter
import json
import os
from pathlib import Path
import re
import subprocess
import sys


REPO_ROOT = Path(__file__).resolve().parents[4]
TOOLS_ROOT = REPO_ROOT / "tools"
sys.dont_write_bytecode = True
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.asm_verify import (  # noqa: E402
    CoffObject,
    IMAGE_SCN_CNT_CODE,
    IMAGE_SCN_CNT_UNINITIALIZED_DATA,
)


SOURCE = Path(__file__).with_name("zdeclient_set_profile.cpp")
DEFAULT_VC5_ROOT = Path("D:/Recoil Project/Compiler/VC5SP3")
DEFAULT_OUTPUT = REPO_ROOT / "build/vc5-probes/zdeclient-set-profile"
COMPILER_VERSION = "Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 11.00.7022"
PROBE_ID = "recoil:diagnostic-probe:zdeclient-set-profile-r3966"
SCHEMA = "diagnostic-vc5-provider-probe-v1"

VECTOR_GLOBAL = (
    "?g_probeFeatureEntries@@3V?$vector@UProbeFeatureEntry@@"
    "V?$allocator@UProbeFeatureEntry@@@std@@@std@@A"
)
SET_GLOBAL = (
    "?g_probeFeatureNodes@@3V?$set@PAUzGeometry_ClipPatchNodeView@@"
    "U?$less@PAUzGeometry_ClipPatchNodeView@@@std@@"
    "V?$allocator@PAUzGeometry_ClipPatchNodeView@@@3@@std@@A"
)
INSERT_PROBE = "?InsertNodeProbe@@YIHPAUzGeometry_ClipPatchNodeView@@@Z"
VISIT_PROBE = "?VisitNodesProbe@@YIHP6IXPAUzGeometry_ClipPatchNodeView@@@Z@Z"
CLEAR_PROBE = "?ClearNodesProbe@@YAXXZ"
ERASE_RANGE_PROBE = "?EraseNodeRangeProbe@@YAXXZ"

IMAGE_SYM_DTYPE_FUNCTION = 0x20
IMAGE_SYM_CLASS_EXTERNAL = 2
IMAGE_SYM_CLASS_STATIC = 3
COMDAT_SELECTION_NAMES = {
    0: "none",
    1: "no-duplicates",
    2: "any",
    3: "same-size",
    4: "exact-match",
    5: "associative",
    6: "largest",
}

EXPECTED_SECTION_SIZES = {
    "crt_initializer_dispatch": 0x10,
    "combined_global_initializer": 0x40,
    "atexit_registration": 0x10,
    "combined_global_cleanup": 0x30,
    "set_destructor": 0x70,
    "set_constructor": 0xD0,
    "tree_insert": 0xF0,
    "tree_erase_range": 0x160,
    "tree_erase_one": 0x530,
    "tree_erase_subtree": 0x90,
    "tree_insert_node": 0x320,
    "iterator_increment": 0xB0,
    "iterator_decrement": 0xC0,
    "construct_pointer_value": 0x10,
}


def _compile(
    *,
    compiler: Path,
    environment: dict[str, str],
    output_dir: Path,
) -> tuple[Path, Path, Path, list[str]]:
    listing = output_dir / "probe.cod"
    obj = output_dir / "probe.obj"
    log = output_dir / "compile.log"
    command = [
        str(compiler),
        "/nologo",
        "/TP",
        "/W3",
        "/G5",
        "/O2",
        "/Ob1",
        "/MD",
        "/GX",
        "/Gr",
        "/Zp4",
        "/FAcs",
        f"/Fa{listing}",
        f"/Fo{obj}",
        "/c",
        str(SOURCE),
    ]
    completed = subprocess.run(
        command,
        cwd=output_dir,
        env=environment,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    log.write_text(
        "command: " + subprocess.list2cmdline(command) + "\n"
        + "returncode: " + str(completed.returncode) + "\n"
        + "stdout:\n" + completed.stdout
        + "\nstderr:\n" + completed.stderr,
        encoding="utf-8",
    )
    if completed.returncode != 0:
        raise RuntimeError(f"VC5 compile failed; see {log}")
    if not listing.is_file() or not obj.is_file():
        raise RuntimeError("compiler omitted the expected listing or object")
    return listing, obj, log, command


def _proc_bodies(listing: str) -> tuple[list[str], dict[str, str]]:
    starts = list(re.finditer(r"(?m)^\s*(\S+)\s+PROC NEAR\b", listing))
    order: list[str] = []
    bodies: dict[str, str] = {}
    for match in starts:
        symbol = match.group(1)
        if symbol in bodies:
            raise RuntimeError(f"{symbol}: duplicate PROC body")
        end_match = re.search(
            rf"(?m)^\s*{re.escape(symbol)}\s+ENDP\b",
            listing[match.start() :],
        )
        if end_match is None:
            raise RuntimeError(f"{symbol}: missing ENDP")
        end = match.start() + end_match.end()
        order.append(symbol)
        bodies[symbol] = listing[match.start() : end]
    if not order:
        raise RuntimeError("listing contains no PROC bodies")
    return order, bodies


def _unique_name(
    names: list[str] | tuple[str, ...] | set[str],
    *,
    label: str,
    predicate,
) -> str:
    matches = [name for name in names if predicate(name)]
    if len(matches) != 1:
        raise RuntimeError(f"{label}: expected one symbol, found {matches!r}")
    return matches[0]


def _return_cleanup_bytes(body: str, symbol: str) -> int | None:
    values = {int(value, 10) for value in re.findall(r"\bret\s+(\d+)\b", body)}
    if not values:
        return None
    if len(values) != 1:
        raise RuntimeError(f"{symbol}: expected one consistent ret cleanup, got {values!r}")
    return next(iter(values))


def _transfer_sites(body: str, undefined_names: set[str]) -> list[dict[str, object]]:
    sites: list[dict[str, object]] = []
    pattern = re.compile(
        r"^\s*(?:[0-9A-Fa-f]{5}\s+)?"
        r"(?:[0-9A-Fa-f]{2}\s+)+"
        r"(call|jmp)\s+(.+?)\s*$"
    )
    registers = {
        "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp",
    }
    for raw_line in body.splitlines():
        line = raw_line.split(";", 1)[0].rstrip()
        match = pattern.match(line)
        if match is None:
            continue
        instruction = match.group(1)
        raw_target = " ".join(match.group(2).split())
        if raw_target.startswith("SHORT "):
            continue
        target = raw_target
        for prefix in ("DWORD PTR ", "WORD PTR ", "BYTE PTR "):
            if target.startswith(prefix):
                target = target[len(prefix) :]
                break
        if target.startswith("FLAT:"):
            target = target[len("FLAT:") :]
        if target.startswith("$L"):
            continue
        indirect = (
            "PTR " in raw_target
            or target.lower() in registers
            or target.startswith("[")
        )
        sites.append(
            {
                "instruction": instruction,
                "target": target,
                "dispatch": (
                    "iat-indirect"
                    if target.startswith("__imp_")
                    else "register-indirect"
                    if target.lower() in registers
                    else "memory-indirect"
                    if indirect
                    else "direct"
                ),
                "provider": target in undefined_names,
            }
        )
    return sites


def _section_selection(coff: CoffObject, section_number: int) -> int:
    candidates = [
        symbol.section_definition_selection
        for symbol in coff.symbols
        if symbol.section_number == section_number
        and symbol.storage_class == IMAGE_SYM_CLASS_STATIC
        and symbol.aux_count > 0
        and symbol.value == 0
    ]
    candidates = [value for value in candidates if value is not None]
    if not candidates:
        return 0
    if len(set(candidates)) != 1:
        raise RuntimeError(
            f"section {section_number}: conflicting COMDAT selections {candidates!r}"
        )
    return candidates[0]


def _code_definitions(coff: CoffObject) -> list[dict[str, object]]:
    definitions: list[dict[str, object]] = []
    for symbol in sorted(
        coff.symbols,
        key=lambda item: (item.section_number, item.value, item.index),
    ):
        if symbol.section_number <= 0:
            continue
        section = coff.section(symbol.section_number)
        if (section.characteristics & IMAGE_SCN_CNT_CODE) == 0:
            continue
        if (symbol.type & IMAGE_SYM_DTYPE_FUNCTION) == 0:
            continue
        if symbol.storage_class not in {IMAGE_SYM_CLASS_EXTERNAL, IMAGE_SYM_CLASS_STATIC}:
            continue
        selection = _section_selection(coff, section.index)
        definitions.append(
            {
                "symbol": symbol.name,
                "section_number": section.index,
                "section_name": section.name,
                "section_offset": symbol.value,
                "section_size": len(section.raw_data),
                "storage_class": (
                    "external"
                    if symbol.storage_class == IMAGE_SYM_CLASS_EXTERNAL
                    else "static"
                ),
                "comdat_selection": COMDAT_SELECTION_NAMES.get(
                    selection, f"unknown-{selection}"
                ),
            }
        )
    return definitions


def _role_record(
    *,
    role: str,
    symbol: str,
    definition_by_name: dict[str, dict[str, object]],
    bodies: dict[str, str],
    undefined_names: set[str],
) -> dict[str, object]:
    if symbol not in definition_by_name:
        raise RuntimeError(f"{role}: missing COFF definition for {symbol}")
    if symbol not in bodies:
        raise RuntimeError(f"{role}: missing listing body for {symbol}")
    transfers = _transfer_sites(bodies[symbol], undefined_names)
    definition = definition_by_name[symbol]
    cleanup_bytes = _return_cleanup_bytes(bodies[symbol], symbol)
    if cleanup_bytes is None:
        if role != "crt_initializer_dispatch" or not any(
            site["instruction"] == "jmp" for site in transfers
        ):
            raise RuntimeError(f"{symbol}: no ret instruction and no recognized tail-return contract")
        cleanup_bytes = 0
        return_form = "tail-call"
    else:
        return_form = "ret"
    record = {
        "role": role,
        **definition,
        "return_form": return_form,
        "return_stack_cleanup_bytes": cleanup_bytes,
        "calls": [site for site in transfers if site["instruction"] == "call"],
        "tail_jumps": [site for site in transfers if site["instruction"] == "jmp"],
    }
    record["provider_calls"] = [
        site["target"]
        for site in transfers
        if site["instruction"] == "call" and site["provider"]
    ]
    record["provider_tail_jumps"] = [
        site["target"]
        for site in transfers
        if site["instruction"] == "jmp" and site["provider"]
    ]
    return record


def run_probe(*, vc5_root: Path, output_dir: Path) -> dict[str, object]:
    compiler = vc5_root / "VC/BIN/cl.exe"
    shared_bin = vc5_root / "SHAREDIDE/BIN"
    redist = vc5_root / "VC/REDIST"
    include = vc5_root / "VC/INCLUDE"
    if not compiler.is_file() or not include.is_dir():
        raise RuntimeError(f"VC5SP3 compiler/include tree is unavailable under {vc5_root}")
    if not SOURCE.is_file():
        raise RuntimeError(f"probe source is unavailable: {SOURCE}")

    output_dir.mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ)
    environment["PATH"] = os.pathsep.join(
        (str(compiler.parent), str(shared_bin), str(redist), environment.get("PATH", ""))
    )
    environment["INCLUDE"] = str(include)

    version = subprocess.run(
        [str(compiler)],
        cwd=output_dir,
        env=environment,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    version_text = version.stdout + version.stderr
    if COMPILER_VERSION not in version_text:
        raise RuntimeError("unexpected compiler version: " + version_text.strip())

    listing_path, obj_path, log_path, command = _compile(
        compiler=compiler,
        environment=environment,
        output_dir=output_dir,
    )
    listing = listing_path.read_text(encoding="utf-8", errors="replace")
    _listing_order, bodies = _proc_bodies(listing)
    coff = CoffObject.from_path(obj_path)
    definitions = _code_definitions(coff)
    definition_names = [str(item["symbol"]) for item in definitions]
    definition_by_name = {str(item["symbol"]): item for item in definitions}
    undefined_names = {
        symbol.name
        for symbol in coff.symbols
        if symbol.section_number == 0 and symbol.storage_class == IMAGE_SYM_CLASS_EXTERNAL
    }

    set_ctor = _unique_name(
        definition_names,
        label="set constructor",
        predicate=lambda name: name.startswith(
            "??0?$set@PAUzGeometry_ClipPatchNodeView@@"
        ),
    )
    set_dtor = _unique_name(
        definition_names,
        label="set destructor",
        predicate=lambda name: name.startswith(
            "??1?$set@PAUzGeometry_ClipPatchNodeView@@"
        ),
    )
    tree_insert = _unique_name(
        definition_names,
        label="tree insert",
        predicate=lambda name: name.startswith("?insert@?$_Tree@"),
    )
    tree_erases = [name for name in definition_names if name.startswith("?erase@?$_Tree@")]
    if len(tree_erases) != 2:
        raise RuntimeError(f"tree erase overloads: expected two, found {tree_erases!r}")
    tree_erase_range = _unique_name(
        tree_erases,
        label="tree erase range",
        predicate=lambda name: "V312@0@Z" in name,
    )
    tree_erase_one = _unique_name(
        tree_erases,
        label="tree erase iterator",
        predicate=lambda name: "V312@@Z" in name,
    )
    iterator_inc = _unique_name(
        definition_names,
        label="iterator increment",
        predicate=lambda name: name.startswith("?_Inc@iterator@?$_Tree@"),
    )
    iterator_dec = _unique_name(
        definition_names,
        label="iterator decrement",
        predicate=lambda name: name.startswith("?_Dec@iterator@?$_Tree@"),
    )
    tree_erase_subtree = _unique_name(
        definition_names,
        label="tree erase subtree",
        predicate=lambda name: name.startswith("?_Erase@?$_Tree@"),
    )
    tree_insert_node = _unique_name(
        definition_names,
        label="tree insert node",
        predicate=lambda name: name.startswith("?_Insert@?$_Tree@"),
    )
    construct_value = _unique_name(
        definition_names,
        label="pointer value construction helper",
        predicate=lambda name: name.startswith(
            "?_Construct@std@@YIXPAPAUzGeometry_ClipPatchNodeView@@"
        ),
    )
    nil_symbol = _unique_name(
        [symbol.name for symbol in coff.symbols if symbol.section_number > 0],
        label="tree _Nil",
        predicate=lambda name: name.startswith("?_Nil@?$_Tree@"),
    )
    nilrefs_symbol = _unique_name(
        [symbol.name for symbol in coff.symbols if symbol.section_number > 0],
        label="tree _Nilrefs",
        predicate=lambda name: name.startswith("?_Nilrefs@?$_Tree@"),
    )

    wrapper_names = [name for name in definition_names if name.startswith("_$E")]
    if len(wrapper_names) != 4 or definition_names[:4] != wrapper_names:
        raise RuntimeError(
            "expected four compiler-internal lifecycle wrappers at the start of COFF code order; "
            f"got {wrapper_names!r} within {definition_names[:5]!r}"
        )
    wrapper_roles = (
        "crt_initializer_dispatch",
        "combined_global_initializer",
        "atexit_registration",
        "combined_global_cleanup",
    )
    roles: dict[str, str] = dict(zip(wrapper_roles, wrapper_names, strict=True))
    roles.update(
        {
            "set_destructor": set_dtor,
            "insert_probe": INSERT_PROBE,
            "visit_probe": VISIT_PROBE,
            "clear_probe": CLEAR_PROBE,
            "erase_range_probe": ERASE_RANGE_PROBE,
            "set_constructor": set_ctor,
            "tree_insert": tree_insert,
            "tree_erase_range": tree_erase_range,
            "iterator_increment": iterator_inc,
            "tree_erase_one": tree_erase_one,
            "tree_erase_subtree": tree_erase_subtree,
            "tree_insert_node": tree_insert_node,
            "iterator_decrement": iterator_dec,
            "construct_pointer_value": construct_value,
        }
    )
    expected_definition_order = [roles[role] for role in (
        "crt_initializer_dispatch",
        "combined_global_initializer",
        "atexit_registration",
        "combined_global_cleanup",
        "set_destructor",
        "insert_probe",
        "visit_probe",
        "clear_probe",
        "erase_range_probe",
        "set_constructor",
        "tree_insert",
        "tree_erase_range",
        "iterator_increment",
        "tree_erase_one",
        "tree_erase_subtree",
        "tree_insert_node",
        "iterator_decrement",
        "construct_pointer_value",
    )]
    if definition_names != expected_definition_order:
        raise RuntimeError(
            "COFF code definition population/order drifted; "
            f"expected {expected_definition_order!r}, got {definition_names!r}"
        )

    role_records = {
        role: _role_record(
            role=role,
            symbol=symbol,
            definition_by_name=definition_by_name,
            bodies=bodies,
            undefined_names=undefined_names,
        )
        for role, symbol in roles.items()
    }
    for role, expected_size in EXPECTED_SECTION_SIZES.items():
        actual_size = int(role_records[role]["section_size"])
        if actual_size != expected_size:
            raise RuntimeError(
                f"{role}: expected section size 0x{expected_size:x}, got 0x{actual_size:x}"
            )

    for role in wrapper_roles:
        if role_records[role]["return_stack_cleanup_bytes"] != 0:
            raise RuntimeError(f"{role}: lifecycle wrapper is not cdecl ret 0")
    if role_records["set_constructor"]["return_stack_cleanup_bytes"] != 8:
        raise RuntimeError("set constructor is not thiscall ret 8")
    if role_records["set_destructor"]["return_stack_cleanup_bytes"] != 0:
        raise RuntimeError("set destructor is not thiscall ret 0")

    dispatch_transfers = (
        role_records["crt_initializer_dispatch"]["calls"],
        role_records["crt_initializer_dispatch"]["tail_jumps"],
    )
    if [site["target"] for site in dispatch_transfers[0]] != [
        roles["combined_global_initializer"]
    ] or [site["target"] for site in dispatch_transfers[1]] != [
        roles["atexit_registration"]
    ]:
        raise RuntimeError("CRT initializer dispatch call/tail-jump shape drifted")
    if role_records["atexit_registration"]["provider_calls"] != ["_atexit"]:
        raise RuntimeError("atexit registration provider call drifted")
    if set_dtor not in [
        site["target"] for site in role_records["combined_global_cleanup"]["calls"]
    ]:
        raise RuntimeError("combined cleanup no longer calls the set destructor")
    if role_records["combined_global_cleanup"]["provider_calls"] != [
        "??3@YAXPAX@Z"
    ]:
        raise RuntimeError("combined cleanup vector provider call drifted")

    expected_probe_calls = {
        "insert_probe": [tree_insert],
        "visit_probe": [iterator_inc],
        "clear_probe": [tree_erase_range],
        "erase_range_probe": [tree_erase_range],
    }
    for role, expected_calls in expected_probe_calls.items():
        direct_defined_calls = [
            site["target"]
            for site in role_records[role]["calls"]
            if site["target"] in definition_by_name
        ]
        if direct_defined_calls != expected_calls:
            raise RuntimeError(
                f"{role}: expected direct defined calls {expected_calls!r}, "
                f"got {direct_defined_calls!r}"
            )

    ctor_body = bodies[set_ctor]
    layout_tokens = (
        "BYTE PTR [esi]",
        "BYTE PTR [esi+1]",
        "DWORD PTR [esi+4]",
        "BYTE PTR [esi+8]",
        "DWORD PTR [esi+12]",
    )
    if not all(token in ctor_body for token in layout_tokens):
        raise RuntimeError("set constructor no longer proves the 0/1/4/8/12 layout")
    if ctor_body.count("push\t 20") != 2:
        raise RuntimeError("set constructor no longer allocates two 20-byte tree nodes")

    vector_symbol = coff.symbols_by_name.get(VECTOR_GLOBAL)
    set_symbol = coff.symbols_by_name.get(SET_GLOBAL)
    nil = coff.symbols_by_name.get(nil_symbol)
    nilrefs = coff.symbols_by_name.get(nilrefs_symbol)
    if None in {vector_symbol, set_symbol, nil, nilrefs}:
        raise RuntimeError("one or more required data definitions are missing")
    assert vector_symbol is not None
    assert set_symbol is not None
    assert nil is not None
    assert nilrefs is not None
    if vector_symbol.section_number != set_symbol.section_number:
        raise RuntimeError("vector and set globals are not in one adjacent BSS contribution")
    globals_section = coff.section(vector_symbol.section_number)
    if (globals_section.characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == 0:
        raise RuntimeError("vector/set global contribution is not uninitialized data")
    actual_global_offsets = {
        VECTOR_GLOBAL: vector_symbol.value,
        SET_GLOBAL: set_symbol.value,
    }
    expected_global_offsets = {VECTOR_GLOBAL: 0x10, SET_GLOBAL: 0x00}
    if actual_global_offsets != expected_global_offsets or len(globals_section.raw_data) != 0x20:
        raise RuntimeError(
            "adjacent vector/set BSS shape drifted; "
            f"offsets={actual_global_offsets!r} size=0x{len(globals_section.raw_data):x}"
        )
    if not (
        nil.section_number < nilrefs.section_number
        and len(coff.section(nil.section_number).raw_data) == 4
        and len(coff.section(nilrefs.section_number).raw_data) == 4
    ):
        raise RuntimeError("_Nil/_Nilrefs COMDAT contribution order or size drifted")

    provider_call_lists = {
        role: {
            "symbol": record["symbol"],
            "calls_in_order": record["provider_calls"],
            "counts": dict(Counter(record["provider_calls"])),
            "provider_tail_jumps_in_order": record["provider_tail_jumps"],
        }
        for role, record in role_records.items()
        if record["provider_calls"] or record["provider_tail_jumps"]
    }
    code_population = []
    symbol_to_role = {symbol: role for role, symbol in roles.items()}
    for definition in definitions:
        code_population.append(
            {
                "role": symbol_to_role[str(definition["symbol"])],
                **definition,
            }
        )

    result: dict[str, object] = {
        "schema": SCHEMA,
        "probe_id": PROBE_ID,
        "kind": "zdeclient-default-set-profile-probe",
        "contract_version": 1,
        "passed": True,
        "compiler_version": COMPILER_VERSION,
        "source": SOURCE.resolve().as_posix(),
        "compile": {
            "command": command,
            "returncode": 0,
            "profile": [
                "/nologo", "/TP", "/W3", "/G5", "/O2", "/Ob1",
                "/MD", "/GX", "/Gr", "/Zp4", "/FAcs",
            ],
            "listing": listing_path.resolve().as_posix(),
            "object": obj_path.resolve().as_posix(),
            "log": log_path.resolve().as_posix(),
        },
        "model": {
            "container": "std::set<zGeometry_ClipPatchNodeView *>",
            "implementation": "default std::set over std::_Tree",
            "comparator": "std::less<zGeometry_ClipPatchNodeView *>",
            "allocator": "std::allocator<zGeometry_ClipPatchNodeView *>",
            "direct_tree_variant_compiled": False,
            "custom_comparator_variant_compiled": False,
            "concrete_ambiguities": [],
        },
        "global_storage": {
            "source_declaration_order": [VECTOR_GLOBAL, SET_GLOBAL],
            "coff_base_bss_order": [SET_GLOBAL, VECTOR_GLOBAL],
            "base_section_number": globals_section.index,
            "base_section_size": len(globals_section.raw_data),
            "objects": [
                {
                    "role": "set_global",
                    "symbol": SET_GLOBAL,
                    "section_offset": set_symbol.value,
                    "size": 16,
                },
                {
                    "role": "vector_global",
                    "symbol": VECTOR_GLOBAL,
                    "section_offset": vector_symbol.value,
                    "size": 16,
                },
            ],
            "template_static_comdat_order": [nil_symbol, nilrefs_symbol],
            "template_statics": [
                {
                    "role": "tree_nil",
                    "symbol": nil_symbol,
                    "section_number": nil.section_number,
                    "size": len(coff.section(nil.section_number).raw_data),
                    "comdat_selection": COMDAT_SELECTION_NAMES.get(
                        _section_selection(coff, nil.section_number), "unknown"
                    ),
                },
                {
                    "role": "tree_nilrefs",
                    "symbol": nilrefs_symbol,
                    "section_number": nilrefs.section_number,
                    "size": len(coff.section(nilrefs.section_number).raw_data),
                    "comdat_selection": COMDAT_SELECTION_NAMES.get(
                        _section_selection(coff, nilrefs.section_number), "unknown"
                    ),
                },
            ],
            "observed_order_fact": (
                "VC5 records the ordinary uninitialized globals in reverse COFF offset order "
                "(set at +0x00, vector at +0x10) despite source declaration and initializer "
                "execution order vector-then-set; _Nil then _Nilrefs follow as separate "
                "4-byte pick-any COMDAT contributions."
            ),
        },
        "set_object_layout": {
            "size": 16,
            "packing": 4,
            "fields": [
                {"offset": 0, "size": 1, "role": "allocator empty-base byte"},
                {"offset": 1, "size": 1, "role": "key comparator empty member byte"},
                {"offset": 4, "size": 4, "role": "tree head pointer"},
                {"offset": 8, "size": 1, "role": "multi-key flag (false for set)"},
                {"offset": 12, "size": 4, "role": "element count"},
            ],
            "tree_node_layout": {
                "size": 20,
                "fields": [
                    {"offset": 0, "size": 4, "role": "left"},
                    {"offset": 4, "size": 4, "role": "parent"},
                    {"offset": 8, "size": 4, "role": "right"},
                    {"offset": 12, "size": 4, "role": "pointer value"},
                    {"offset": 16, "size": 4, "role": "red/black color"},
                ],
            },
            "evidence": (
                "compile-time sizeof assertion plus constructor stores at +0,+1,+4,+8,+12 "
                "and two operator-new requests of 20 bytes"
            ),
        },
        "lifecycle_and_helper_population": {
            "coff_definition_order": code_population,
            "wrapper_abi": {
                "crt_initializer_dispatch": {
                    "symbol": roles["crt_initializer_dispatch"],
                    "calling_convention": "compiler-internal cdecl, no arguments",
                    "return_stack_cleanup_bytes": 0,
                    "behavior": "calls combined initializer then tail-jumps to atexit registration",
                },
                "combined_global_initializer": {
                    "symbol": roles["combined_global_initializer"],
                    "calling_convention": "compiler-internal cdecl, no arguments",
                    "return_stack_cleanup_bytes": 0,
                    "behavior": (
                        "initializes the vector's allocator byte and three pointers, then calls "
                        "the set constructor with ECX=set and two 4-byte reference arguments"
                    ),
                },
                "atexit_registration": {
                    "symbol": roles["atexit_registration"],
                    "calling_convention": "compiler-internal cdecl, no arguments",
                    "return_stack_cleanup_bytes": 0,
                    "behavior": "passes the combined cleanup wrapper to provider _atexit",
                },
                "combined_global_cleanup": {
                    "symbol": roles["combined_global_cleanup"],
                    "calling_convention": "compiler-internal cdecl, no arguments",
                    "return_stack_cleanup_bytes": 0,
                    "behavior": (
                        "calls set destructor with ECX=set, deletes the vector allocation, "
                        "then zeroes the vector's three pointer fields"
                    ),
                },
                "set_constructor": {
                    "symbol": set_ctor,
                    "calling_convention": "thiscall",
                    "this_register": "ECX",
                    "stack_argument_bytes": 8,
                    "return_stack_cleanup_bytes": 8,
                },
                "set_destructor": {
                    "symbol": set_dtor,
                    "calling_convention": "thiscall",
                    "this_register": "ECX",
                    "stack_argument_bytes": 0,
                    "return_stack_cleanup_bytes": 0,
                },
            },
            "provider_call_lists": provider_call_lists,
        },
        "operation_forms": {
            "insert_pointer": {
                "probe_symbol": INSERT_PROBE,
                "direct_helper": tree_insert,
            },
            "iterator_loop": {
                "probe_symbol": VISIT_PROBE,
                "indirect_callback_dispatch": "register-indirect",
                "increment_helper": iterator_inc,
            },
            "clear": {
                "probe_symbol": CLEAR_PROBE,
                "direct_helper": tree_erase_range,
            },
            "erase_begin_end": {
                "probe_symbol": ERASE_RANGE_PROBE,
                "direct_helper": tree_erase_range,
                "transitive_helpers": [tree_erase_one, tree_erase_subtree],
            },
        },
        "decision": (
            "The default std::set<Node*> model is unambiguous: it naturally emits a 16-byte "
            "object, _Nil then _Nilrefs, the four 0x10/0x40/0x10/0x30 lifecycle wrappers, "
            "a 0x70 destructor, and the 0xd0/0xf0/0x160/0x530/0x90/0x320/0xb0/0xc0 "
            "tree-helper extent family. Do not use direct std::_Tree or a custom comparator. "
            "Treat the reversed ordinary-BSS offset order as a separate production placement "
            "constraint."
        ),
        "recommendation": (
            "Use source-level std::set<zGeometry_ClipPatchNodeView*> with its default comparator "
            "and allocator for the zDEClient tree owner. Preserve the natural constructor, "
            "destructor, insert, iterator, clear, and erase forms; verify final production data "
            "placement explicitly because VC5 reverses the adjacent ordinary BSS symbol offsets."
        ),
    }
    result_path = output_dir / "result.json"
    result["result_path"] = result_path.resolve().as_posix()
    result_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vc5-root", type=Path, default=DEFAULT_VC5_ROOT)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args(argv)
    try:
        result = run_probe(
            vc5_root=args.vc5_root.resolve(),
            output_dir=args.output_dir.resolve(),
        )
    except (OSError, RuntimeError, ValueError) as exc:
        print(json.dumps({"schema": SCHEMA, "probe_id": PROBE_ID, "passed": False, "error": str(exc)}, indent=2))
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
