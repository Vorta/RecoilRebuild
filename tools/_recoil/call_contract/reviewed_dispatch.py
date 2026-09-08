"""Recoil call-contract reviewed dispatch evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import recoil_hud_layout as _cc_recoil_hud_layout
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        IdentityIndexes,
        ReviewedLoopVptrStorageBridge,
        ReviewedRegisterCallStorageBridge,
    )

import struct
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import Instruction
from _recoil.commands.provider_target_mutation import _retail_import_targets
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import address_value, normalize_address


def _reviewed_r3994_retail_register_storage_bridges(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    retail_import_targets: Sequence[Any] | None = None,
) -> dict[str, ReviewedRegisterCallStorageBridge]:
    """Publish the two finite reviewed absolute-load register call chains.

    These calls cannot be assigned a static function target: one is a mutable
    application callback and the other is an IAT callable.  The reviewed fact
    is only the exact storage reaching the exact register call.  Every byte,
    address, extent, register write and intervening control-flow condition is
    checked here so an adjacent absolute load cannot become provenance.
    """
    from _recoil.call_contract.records import ReviewedRegisterCallStorageBridge

    start = normalize_address(caller_start)
    end = normalize_address(caller_end_exclusive)
    specs: Mapping[str, Mapping[str, Any]] = {
        "0x48afe0": {
            "end": "0x48b3a0",
            "body_end": "0x48b297",
            "load": "0x48b1f4",
            "load_bytes": bytes.fromhex("a1 fc aa 56 00"),
            "register": "eax",
            "storage": "0x56aafc",
            "call": "0x48b200",
            "call_bytes": bytes.fromhex("ff d0"),
            "required": {
                "0x48affc": bytes.fromhex("33 ed"),
                "0x48b009": bytes.fromhex("0f 87 b9 01 00 00"),
                "0x48b1c8": bytes.fromhex("83 c0 cf"),
                "0x48b1cb": bytes.fromhex("3d dc 00 00 00"),
                "0x48b1d0": bytes.fromhex("0f 87 4a fe ff ff"),
                "0x48b1d6": bytes.fromhex("33 c9"),
                "0x48b1d8": bytes.fromhex("8a 88 b4 b2 48 00"),
                "0x48b1de": bytes.fromhex("ff 24 8d 98 b2 48 00"),
                "0x48b1f9": bytes.fromhex("3b c5"),
                "0x48b1fb": bytes.fromhex("74 05"),
                "0x48b1fd": bytes.fromhex("83 c9 ff"),
                "0x48b292": bytes.fromhex("e9 a5 fd ff ff"),
            },
            "iat": False,
        },
        "0x4c20a0": {
            "end": "0x4c5480",
            "body_end": "0x4c53e9",
            "load": "0x4c20e6",
            "load_bytes": bytes.fromhex("8b 3d cc c5 4c 00"),
            "register": "edi",
            "storage": "0x4cc5cc",
            "call": "0x4c20f4",
            "calls": (
                "0x4c20f4",
                "0x4c2189",
                "0x4c21c3",
                "0x4c21fd",
            ),
            "call_bytes": bytes.fromhex("ff d7"),
            "cleanup": 12,
            "required": {
                "0x4c20da": bytes.fromhex("39 5e 08"),
                "0x4c20dd": bytes.fromhex("76 05"),
                "0x4c20df": bytes.fromhex("8b 46 20"),
                "0x4c20e2": bytes.fromhex("eb 02"),
                "0x4c20e4": bytes.fromhex("33 c0"),
                "0x4c20e6": bytes.fromhex("8b 3d cc c5 4c 00"),
                "0x4c20ec": bytes.fromhex("6a 08"),
                "0x4c20ee": bytes.fromhex("68 48 59 4e 00"),
                "0x4c20f3": bytes.fromhex("50"),
                "0x4c20f6": bytes.fromhex("83 c4 0c"),
                "0x4c20f9": bytes.fromhex("85 c0"),
                "0x4c20fb": bytes.fromhex("75 78"),
                "0x4c2175": bytes.fromhex("39 5e 08"),
                "0x4c2178": bytes.fromhex("76 05"),
                "0x4c217a": bytes.fromhex("8b 46 20"),
                "0x4c217d": bytes.fromhex("eb 02"),
                "0x4c217f": bytes.fromhex("33 c0"),
                "0x4c2181": bytes.fromhex("6a 10"),
                "0x4c2183": bytes.fromhex("68 dc 58 4e 00"),
                "0x4c2188": bytes.fromhex("50"),
                "0x4c2189": bytes.fromhex("ff d7"),
                "0x4c218b": bytes.fromhex("83 c4 0c"),
                "0x4c218e": bytes.fromhex("85 c0"),
                "0x4c2190": bytes.fromhex("75 1d"),
                "0x4c21af": bytes.fromhex("39 5e 08"),
                "0x4c21b2": bytes.fromhex("76 05"),
                "0x4c21b4": bytes.fromhex("8b 46 20"),
                "0x4c21b7": bytes.fromhex("eb 02"),
                "0x4c21b9": bytes.fromhex("33 c0"),
                "0x4c21bb": bytes.fromhex("6a 0e"),
                "0x4c21bd": bytes.fromhex("68 cc 58 4e 00"),
                "0x4c21c2": bytes.fromhex("50"),
                "0x4c21c3": bytes.fromhex("ff d7"),
                "0x4c21c5": bytes.fromhex("83 c4 0c"),
                "0x4c21c8": bytes.fromhex("85 c0"),
                "0x4c21ca": bytes.fromhex("75 1d"),
                "0x4c21e9": bytes.fromhex("39 5e 08"),
                "0x4c21ec": bytes.fromhex("76 05"),
                "0x4c21ee": bytes.fromhex("8b 46 20"),
                "0x4c21f1": bytes.fromhex("eb 02"),
                "0x4c21f3": bytes.fromhex("33 c0"),
                "0x4c21f5": bytes.fromhex("6a 11"),
                "0x4c21f7": bytes.fromhex("68 b8 58 4e 00"),
                "0x4c21fc": bytes.fromhex("50"),
                "0x4c21fd": bytes.fromhex("ff d7"),
                "0x4c21ff": bytes.fromhex("83 c4 0c"),
                "0x4c2202": bytes.fromhex("85 c0"),
                "0x4c2204": bytes.fromhex("0f 85 c9 31 00 00"),
                "0x4c53e6": bytes.fromhex("c2 04 00"),
            },
            "iat": True,
        },
        "0x4c1b30": {
            "end": "0x4c1b50",
            "load": "0x4c1b34",
            "load_bytes": bytes.fromhex("8b 40 70"),
            "register": "eax",
            "call": "0x4c1b45",
            "call_bytes": bytes.fromhex("ff d0"),
            "cleanup": 8,
            "required": {
                "0x4c1b30": bytes.fromhex("8b 44 24 04"),
                "0x4c1b37": bytes.fromhex("85 c0"),
                "0x4c1b39": bytes.fromhex("74 0f"),
                "0x4c1b43": bytes.fromhex("51"),
                "0x4c1b44": bytes.fromhex("52"),
            },
            "dynamic": "dynamic:zInterp_Context+0x70-callback",
            "iat": False,
        },
        "0x4c5520": {
            "end": "0x4c5550",
            "load": "0x4c552b",
            "load_bytes": bytes.fromhex("8b 40 70"),
            "register": "eax",
            "call": "0x4c553c",
            "call_bytes": bytes.fromhex("ff d0"),
            "cleanup": 8,
            "required": {
                "0x4c5520": bytes.fromhex("8b 44 24 04"),
                "0x4c5524": bytes.fromhex("c7 40 10 01 00 00 00"),
                "0x4c552e": bytes.fromhex("85 c0"),
                "0x4c5530": bytes.fromhex("74 0f"),
                "0x4c553a": bytes.fromhex("51"),
                "0x4c553b": bytes.fromhex("52"),
            },
            "dynamic": "dynamic:zInterp_Context+0x70-callback",
            "iat": False,
        },
    }
    spec = specs.get(start)
    if spec is None:
        return {}
    if end != spec["end"]:
        raise ValueError("reviewed r3994 register-storage caller extent drifted")
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(start),
    )
    rows = {
        normalize_address(address): instruction
        for address, instruction in zip(addresses, retail_instructions)
        if address is not None
    }

    body_end = spec.get("body_end")
    if body_end is not None:
        instruction_ends = [
            address_value(normalize_address(address)) + len(instruction.bytes)
            for address, instruction in zip(addresses, retail_instructions)
            if address is not None and instruction.bytes
        ]
        if not instruction_ends or max(instruction_ends) != address_value(body_end):
            raise ValueError(
                "reviewed zInterp effective body extent drifted inside its "
                "selected call-contract extent"
            )

    def body_at(address: str) -> bytes:
        row = rows.get(address)
        if row is None:
            return b""
        try:
            return bytes(int(item, 16) for item in row.bytes)
        except (TypeError, ValueError):
            return b""

    call_addresses = tuple(
        str(address) for address in spec.get("calls", (spec["call"],))
    )
    register = str(spec["register"])
    for call_address in call_addresses:
        call = rows.get(call_address)
        if (
            call is None
            or body_at(call_address) != spec["call_bytes"]
            or _cc_cfg._instruction_mnemonic(call) != "call"
            or _cc_cfg._instruction_operand(call).strip().lower() != register
        ):
            raise ValueError("reviewed r3994 register-storage call bytes drifted")
    call_address = call_addresses[0]
    for address, expected_body in spec.get("required", {}).items():
        if body_at(address) != expected_body:
            raise ValueError(
                "reviewed r3994 callback guard/argument bytes drifted"
            )
    separate_zinterp_strncmp_calls = (
        "0x4c2238",
        "0x4c2341",
        "0x4c2383",
        "0x4c23cb",
        "0x4c2434",
        "0x4c2482",
        "0x4c24d0",
        "0x4c2519",
        "0x4c2672",
        "0x4c26ae",
        "0x4c26fc",
        "0x4c274a",
        "0x4c27b3",
        "0x4c27fa",
        "0x4c2841",
        "0x4c2883",
        "0x4c28cc",
        "0x4c2993",
        "0x4c29dc",
    )
    if start == "0x4c20a0":
        for literal_start, literal_end, expected_literal in (
            ("0x4e5868", "0x4e5878", b"CameraSetActive\0"),
            ("0x4e58a8", "0x4e58b5", b"CameraRotate\0"),
            ("0x4e58b8", "0x4e58ca", b"AnimSetDebugFrame\0"),
            ("0x4e58cc", "0x4e58db", b"AnimSetZBDFile\0"),
            ("0x4e58dc", "0x4e58ed", b"AddEnhancerImage\0"),
        ):
            if _cc_identity._immutable_retail_interval(
                _cc_catalog.DEFAULT_REFERENCE,
                start=address_value(literal_start),
                end_exclusive=address_value(literal_end),
            ) != expected_literal:
                raise ValueError(
                    "reviewed zInterp strncmp literal storage drifted"
                )
        branch_targets = {
            normalize_address(address): normalize_address(target)
            for address, instruction in zip(addresses, retail_instructions)
            if address is not None
            and _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            and (
                target := _cc_cfg._rendered_numeric_branch_target(
                    instruction,
                    source="bn",
                    caller_start=address_value(start),
                )
            )
            is not None
        }
        join_entries = sorted(
            address
            for address, target in branch_targets.items()
            if target == "0x4c20e6"
        )
        zero_entries = sorted(
            address
            for address, target in branch_targets.items()
            if target == "0x4c20e4"
        )
        zero_row = rows.get("0x4c20e4")
        zero_falls_through = (
            zero_row is not None
            and address_value("0x4c20e4") + len(zero_row.bytes)
            == address_value("0x4c20e6")
        )
        if (
            join_entries != ["0x4c20e2"]
            or zero_entries != ["0x4c20dd"]
            or not zero_falls_through
        ):
            raise ValueError(
                "reviewed zInterp strncmp two-arm CFG join drifted"
            )
        for (
            region_entry,
            region_start,
            zero_branch,
            zero_block,
            join_branch,
            join,
        ) in (
            (
                "0x4c20fb", "0x4c2175", "0x4c2178",
                "0x4c217f", "0x4c217d", "0x4c2181",
            ),
            (
                "0x4c2190", "0x4c21af", "0x4c21b2",
                "0x4c21b9", "0x4c21b7", "0x4c21bb",
            ),
            (
                "0x4c21ca", "0x4c21e9", "0x4c21ec",
                "0x4c21f3", "0x4c21f1", "0x4c21f5",
            ),
        ):
            region_entries = sorted(
                address
                for address, target in branch_targets.items()
                if target == region_start
            )
            zero_entries = sorted(
                address
                for address, target in branch_targets.items()
                if target == zero_block
            )
            join_entries = sorted(
                address
                for address, target in branch_targets.items()
                if target == join
            )
            zero_row = rows.get(zero_block)
            zero_falls_through = (
                zero_row is not None
                and address_value(zero_block) + len(zero_row.bytes)
                == address_value(join)
            )
            if (
                region_entries != [region_entry]
                or zero_entries != [zero_branch]
                or join_entries != [join_branch]
                or not zero_falls_through
            ):
                raise ValueError(
                    "reviewed zInterp strncmp four-call CFG route drifted"
                )
        separate_required = {
            "0x4c20d3": bytes.fromhex("ff 24 8d ec 53 4c 00"),
            "0x4c221e": bytes.fromhex("39 5e 08"),
            "0x4c2221": bytes.fromhex("76 05"),
            "0x4c2223": bytes.fromhex("8b 46 20"),
            "0x4c2226": bytes.fromhex("eb 02"),
            "0x4c2228": bytes.fromhex("33 c0"),
            "0x4c222a": bytes.fromhex("8b 1d cc c5 4c 00"),
            "0x4c2230": bytes.fromhex("6a 0c"),
            "0x4c2232": bytes.fromhex("68 a8 58 4e 00"),
            "0x4c2237": bytes.fromhex("50"),
            "0x4c2238": bytes.fromhex("ff d3"),
            "0x4c223a": bytes.fromhex("83 c4 0c"),
            "0x4c223d": bytes.fromhex("85 c0"),
            "0x4c223f": bytes.fromhex("75 4a"),
            "0x4c2281": bytes.fromhex("5f"),
            "0x4c2282": bytes.fromhex("5e"),
            "0x4c2283": bytes.fromhex("5d"),
            "0x4c2284": bytes.fromhex("5b"),
            "0x4c2285": bytes.fromhex("83 c4 28"),
            "0x4c2288": bytes.fromhex("c2 04 00"),
            "0x4c228b": bytes.fromhex("8b 6e 08"),
            "0x4c228e": bytes.fromhex("85 ed"),
            "0x4c2290": bytes.fromhex("76 05"),
            "0x4c2292": bytes.fromhex("8b 46 20"),
            "0x4c2295": bytes.fromhex("eb 02"),
            "0x4c2297": bytes.fromhex("33 c0"),
            "0x4c2299": bytes.fromhex("bf 94 58 4e 00"),
            "0x4c229e": bytes.fromhex("8a 10"),
            "0x4c22a2": bytes.fromhex("3a 17"),
            "0x4c22a4": bytes.fromhex("75 1c"),
            "0x4c22a6": bytes.fromhex("84 c9"),
            "0x4c22a8": bytes.fromhex("74 14"),
            "0x4c22aa": bytes.fromhex("8a 50 01"),
            "0x4c22af": bytes.fromhex("3a 57 01"),
            "0x4c22b2": bytes.fromhex("75 0e"),
            "0x4c22b4": bytes.fromhex("83 c0 02"),
            "0x4c22b7": bytes.fromhex("83 c7 02"),
            "0x4c22bc": bytes.fromhex("75 e0"),
            "0x4c22be": bytes.fromhex("33 c0"),
            "0x4c22c0": bytes.fromhex("eb 05"),
            "0x4c22c2": bytes.fromhex("1b c0"),
            "0x4c22c4": bytes.fromhex("83 d8 ff"),
            "0x4c22c7": bytes.fromhex("85 c0"),
            "0x4c22c9": bytes.fromhex("75 63"),
            "0x4c2324": bytes.fromhex("5f"),
            "0x4c2325": bytes.fromhex("5e"),
            "0x4c2326": bytes.fromhex("5d"),
            "0x4c2327": bytes.fromhex("5b"),
            "0x4c2328": bytes.fromhex("83 c4 28"),
            "0x4c232b": bytes.fromhex("c2 04 00"),
            "0x4c232e": bytes.fromhex("85 ed"),
            "0x4c2330": bytes.fromhex("76 05"),
            "0x4c2332": bytes.fromhex("8b 46 20"),
            "0x4c2335": bytes.fromhex("eb 02"),
            "0x4c2337": bytes.fromhex("33 c0"),
            "0x4c2339": bytes.fromhex("6a 0f"),
            "0x4c233b": bytes.fromhex("68 68 58 4e 00"),
            "0x4c2340": bytes.fromhex("50"),
            "0x4c2341": bytes.fromhex("ff d3"),
            "0x4c2343": bytes.fromhex("83 c4 0c"),
            "0x4c2346": bytes.fromhex("85 c0"),
            "0x4c2348": bytes.fromhex("75 23"),
            "0x4c2366": bytes.fromhex("5b"),
            "0x4c236a": bytes.fromhex("c2 04 00"),
            "0x4c236d": bytes.fromhex("8b 46 08"),
            "0x4c2370": bytes.fromhex("85 c0"),
            "0x4c2372": bytes.fromhex("76 05"),
            "0x4c2374": bytes.fromhex("8b 46 20"),
            "0x4c2377": bytes.fromhex("eb 02"),
            "0x4c2379": bytes.fromhex("33 c0"),
            "0x4c237b": bytes.fromhex("6a 13"),
            "0x4c237d": bytes.fromhex("68 54 58 4e 00"),
            "0x4c2382": bytes.fromhex("50"),
            "0x4c2383": bytes.fromhex("ff d3"),
            "0x4c2385": bytes.fromhex("83 c4 0c"),
            "0x4c2388": bytes.fromhex("85 c0"),
            "0x4c238a": bytes.fromhex("75 29"),
            "0x4c23ab": bytes.fromhex("5f"),
            "0x4c23ac": bytes.fromhex("5e"),
            "0x4c23ad": bytes.fromhex("5d"),
            "0x4c23ae": bytes.fromhex("5b"),
            "0x4c23af": bytes.fromhex("83 c4 28"),
            "0x4c23b2": bytes.fromhex("c2 04 00"),
            "0x4c23b5": bytes.fromhex("8b 46 08"),
            "0x4c23b8": bytes.fromhex("85 c0"),
            "0x4c23ba": bytes.fromhex("76 05"),
            "0x4c23bc": bytes.fromhex("8b 46 20"),
            "0x4c23bf": bytes.fromhex("eb 02"),
            "0x4c23c1": bytes.fromhex("33 c0"),
            "0x4c23c3": bytes.fromhex("6a 0c"),
            "0x4c23c5": bytes.fromhex("68 44 58 4e 00"),
            "0x4c23ca": bytes.fromhex("50"),
            "0x4c23cb": bytes.fromhex("ff d3"),
            "0x4c23cd": bytes.fromhex("83 c4 0c"),
            "0x4c23d0": bytes.fromhex("85 c0"),
            "0x4c23d2": bytes.fromhex("75 4a"),
            "0x4c2417": bytes.fromhex("5b"),
            "0x4c241b": bytes.fromhex("c2 04 00"),
            "0x4c241e": bytes.fromhex("8b 46 08"),
            "0x4c2421": bytes.fromhex("85 c0"),
            "0x4c2423": bytes.fromhex("76 05"),
            "0x4c2425": bytes.fromhex("8b 46 20"),
            "0x4c2428": bytes.fromhex("eb 02"),
            "0x4c242a": bytes.fromhex("33 c0"),
            "0x4c242c": bytes.fromhex("6a 12"),
            "0x4c242e": bytes.fromhex("68 30 58 4e 00"),
            "0x4c2433": bytes.fromhex("50"),
            "0x4c2434": bytes.fromhex("ff d3"),
            "0x4c2436": bytes.fromhex("83 c4 0c"),
            "0x4c2439": bytes.fromhex("85 c0"),
            "0x4c243b": bytes.fromhex("75 2f"),
            "0x4c2465": bytes.fromhex("5b"),
            "0x4c2469": bytes.fromhex("c2 04 00"),
            "0x4c246c": bytes.fromhex("8b 46 08"),
            "0x4c246f": bytes.fromhex("85 c0"),
            "0x4c2471": bytes.fromhex("76 05"),
            "0x4c2473": bytes.fromhex("8b 46 20"),
            "0x4c2476": bytes.fromhex("eb 02"),
            "0x4c2478": bytes.fromhex("33 c0"),
            "0x4c247a": bytes.fromhex("6a 10"),
            "0x4c247c": bytes.fromhex("68 1c 58 4e 00"),
            "0x4c2481": bytes.fromhex("50"),
            "0x4c2482": bytes.fromhex("ff d3"),
            "0x4c2484": bytes.fromhex("83 c4 0c"),
            "0x4c2487": bytes.fromhex("85 c0"),
            "0x4c2489": bytes.fromhex("75 2f"),
            "0x4c24b3": bytes.fromhex("5b"),
            "0x4c24b7": bytes.fromhex("c2 04 00"),
            "0x4c24ba": bytes.fromhex("8b 46 08"),
            "0x4c24bd": bytes.fromhex("85 c0"),
            "0x4c24bf": bytes.fromhex("76 05"),
            "0x4c24c1": bytes.fromhex("8b 46 20"),
            "0x4c24c4": bytes.fromhex("eb 02"),
            "0x4c24c6": bytes.fromhex("33 c0"),
            "0x4c24c8": bytes.fromhex("6a 16"),
            "0x4c24ca": bytes.fromhex("68 04 58 4e 00"),
            "0x4c24cf": bytes.fromhex("50"),
            "0x4c24d0": bytes.fromhex("ff d3"),
            "0x4c24d2": bytes.fromhex("83 c4 0c"),
            "0x4c24d5": bytes.fromhex("85 c0"),
            "0x4c24d7": bytes.fromhex("75 2a"),
            "0x4c24fc": bytes.fromhex("5b"),
            "0x4c2500": bytes.fromhex("c2 04 00"),
            "0x4c2503": bytes.fromhex("8b 46 08"),
            "0x4c2506": bytes.fromhex("85 c0"),
            "0x4c2508": bytes.fromhex("76 05"),
            "0x4c250a": bytes.fromhex("8b 46 20"),
            "0x4c250d": bytes.fromhex("eb 02"),
            "0x4c250f": bytes.fromhex("33 c0"),
            "0x4c2511": bytes.fromhex("6a 14"),
            "0x4c2513": bytes.fromhex("68 ec 57 4e 00"),
            "0x4c2518": bytes.fromhex("50"),
            "0x4c2519": bytes.fromhex("ff d3"),
            "0x4c251b": bytes.fromhex("83 c4 0c"),
            "0x4c251e": bytes.fromhex("85 c0"),
            "0x4c2520": bytes.fromhex("75 3a"),
            "0x4c2555": bytes.fromhex("5b"),
            "0x4c2559": bytes.fromhex("c2 04 00"),
            "0x4c255c": bytes.fromhex("8b 6e 08"),
            "0x4c255f": bytes.fromhex("85 ed"),
            "0x4c2561": bytes.fromhex("76 05"),
            "0x4c2563": bytes.fromhex("8b 46 20"),
            "0x4c2566": bytes.fromhex("eb 02"),
            "0x4c2568": bytes.fromhex("33 c0"),
            "0x4c256a": bytes.fromhex("bf d8 57 4e 00"),
            "0x4c256f": bytes.fromhex("8a 10"),
            "0x4c2573": bytes.fromhex("3a 17"),
            "0x4c2575": bytes.fromhex("75 1c"),
            "0x4c2577": bytes.fromhex("84 c9"),
            "0x4c2579": bytes.fromhex("74 14"),
            "0x4c257b": bytes.fromhex("8a 50 01"),
            "0x4c2580": bytes.fromhex("3a 57 01"),
            "0x4c2583": bytes.fromhex("75 0e"),
            "0x4c2585": bytes.fromhex("83 c0 02"),
            "0x4c2588": bytes.fromhex("83 c7 02"),
            "0x4c258d": bytes.fromhex("75 e0"),
            "0x4c258f": bytes.fromhex("33 c0"),
            "0x4c2591": bytes.fromhex("eb 05"),
            "0x4c2593": bytes.fromhex("1b c0"),
            "0x4c2595": bytes.fromhex("83 d8 ff"),
            "0x4c2598": bytes.fromhex("85 c0"),
            "0x4c259a": bytes.fromhex("75 43"),
            "0x4c25d8": bytes.fromhex("5b"),
            "0x4c25dc": bytes.fromhex("c2 04 00"),
            "0x4c25df": bytes.fromhex("85 ed"),
            "0x4c25e1": bytes.fromhex("76 05"),
            "0x4c25e3": bytes.fromhex("8b 46 20"),
            "0x4c25e6": bytes.fromhex("eb 02"),
            "0x4c25e8": bytes.fromhex("33 c0"),
            "0x4c25ea": bytes.fromhex("bf c4 57 4e 00"),
            "0x4c25ef": bytes.fromhex("8a 10"),
            "0x4c25f3": bytes.fromhex("3a 17"),
            "0x4c25f5": bytes.fromhex("75 1c"),
            "0x4c25f7": bytes.fromhex("84 c9"),
            "0x4c25f9": bytes.fromhex("74 14"),
            "0x4c25fb": bytes.fromhex("8a 50 01"),
            "0x4c2600": bytes.fromhex("3a 57 01"),
            "0x4c2603": bytes.fromhex("75 0e"),
            "0x4c2605": bytes.fromhex("83 c0 02"),
            "0x4c2608": bytes.fromhex("83 c7 02"),
            "0x4c260d": bytes.fromhex("75 e0"),
            "0x4c260f": bytes.fromhex("33 c0"),
            "0x4c2611": bytes.fromhex("eb 05"),
            "0x4c2613": bytes.fromhex("1b c0"),
            "0x4c2615": bytes.fromhex("83 d8 ff"),
            "0x4c2618": bytes.fromhex("85 c0"),
            "0x4c261a": bytes.fromhex("75 43"),
            "0x4c2658": bytes.fromhex("5b"),
            "0x4c265c": bytes.fromhex("c2 04 00"),
            "0x4c265f": bytes.fromhex("85 ed"),
            "0x4c2661": bytes.fromhex("76 05"),
            "0x4c2663": bytes.fromhex("8b 46 20"),
            "0x4c2666": bytes.fromhex("eb 02"),
            "0x4c2668": bytes.fromhex("33 c0"),
            "0x4c266a": bytes.fromhex("6a 16"),
            "0x4c266c": bytes.fromhex("68 ac 57 4e 00"),
            "0x4c2671": bytes.fromhex("50"),
            "0x4c2672": bytes.fromhex("ff d3"),
            "0x4c2674": bytes.fromhex("83 c4 0c"),
            "0x4c2677": bytes.fromhex("85 c0"),
            "0x4c2679": bytes.fromhex("75 1d"),
        }
        complete_ebx9_extension = (
            # fork, count, literal, call, post-call branch bytes/target
            ("0x4c2698", 0x0F, "0x4e579c", "0x4c26ae", b"\x75\x2f", "0x4c26e6"),
            ("0x4c26e6", 0x0E, "0x4e578c", "0x4c26fc", b"\x75\x2f", "0x4c2734"),
            ("0x4c2734", 0x0F, "0x4e577c", "0x4c274a", b"\x75\x4a", "0x4c279d"),
            ("0x4c279d", 0x0E, "0x4e5768", "0x4c27b3", b"\x75\x28", "0x4c27e4"),
            ("0x4c27e4", 0x0E, "0x4e5738", "0x4c27fa", b"\x75\x28", "0x4c282b"),
            ("0x4c282b", 0x16, "0x4e5700", "0x4c2841", b"\x75\x23", "0x4c286d"),
            ("0x4c286d", 0x12, "0x4e56ec", "0x4c2883", b"\x75\x2a", "0x4c28b6"),
            (
                "0x4c28b6", 0x11, "0x4e56d8", "0x4c28cc",
                bytes.fromhex("0f 85 a4 00 00 00"), "0x4c297d",
            ),
            ("0x4c297d", 0x14, "0x4e55dc", "0x4c2993", b"\x75\x2a", "0x4c29c6"),
            (
                "0x4c29c6", 0x11, "0x4e55c8", "0x4c29dc",
                bytes.fromhex("0f 85 ea 29 00 00"), "0x4c53d5",
            ),
        )
        for (
            fork_address,
            count,
            literal_address,
            extension_call,
            continuation_body,
            _continuation_target,
        ) in complete_ebx9_extension:
            fork = address_value(fork_address)
            call = address_value(extension_call)
            if call != fork + 0x16:
                raise ValueError(
                    "reviewed zInterp strncmp complete EBX#9 fork extent drifted"
                )
            separate_required.update({
                f"0x{fork:x}": bytes.fromhex("8b 46 08"),
                f"0x{fork + 3:x}": bytes.fromhex("85 c0"),
                f"0x{fork + 5:x}": bytes.fromhex("76 05"),
                f"0x{fork + 7:x}": bytes.fromhex("8b 46 20"),
                f"0x{fork + 10:x}": bytes.fromhex("eb 02"),
                f"0x{fork + 12:x}": bytes.fromhex("33 c0"),
                f"0x{fork + 14:x}": bytes((0x6A, count)),
                f"0x{fork + 16:x}": b"\x68" + struct.pack(
                    "<I", address_value(literal_address)
                ),
                f"0x{fork + 21:x}": bytes.fromhex("50"),
                extension_call: bytes.fromhex("ff d3"),
                f"0x{call + 2:x}": bytes.fromhex("83 c4 0c"),
                (
                    "0x4c29e5"
                    if extension_call == "0x4c29dc"
                    else f"0x{call + 7:x}"
                ): continuation_body,
            })
            if extension_call == "0x4c29dc":
                separate_required.update({
                    "0x4c29e1": bytes.fromhex("8b ce"),
                    "0x4c29e3": bytes.fromhex("85 c0"),
                })
            else:
                separate_required[f"0x{call + 5:x}"] = bytes.fromhex("85 c0")
        separate_required.update({
            "0x4c2691": bytes.fromhex("5b"),
            "0x4c2695": bytes.fromhex("c2 04 00"),
            "0x4c26df": bytes.fromhex("5b"),
            "0x4c26e3": bytes.fromhex("c2 04 00"),
            "0x4c272d": bytes.fromhex("5b"),
            "0x4c2731": bytes.fromhex("c2 04 00"),
            "0x4c2796": bytes.fromhex("5b"),
            "0x4c279a": bytes.fromhex("c2 04 00"),
            "0x4c27dd": bytes.fromhex("5b"),
            "0x4c27e1": bytes.fromhex("c2 04 00"),
            "0x4c2824": bytes.fromhex("5b"),
            "0x4c2828": bytes.fromhex("c2 04 00"),
            "0x4c2866": bytes.fromhex("5b"),
            "0x4c286a": bytes.fromhex("c2 04 00"),
            "0x4c28af": bytes.fromhex("5b"),
            "0x4c28b3": bytes.fromhex("c2 04 00"),
            "0x4c2910": bytes.fromhex("5b"),
            "0x4c29bf": bytes.fromhex("5b"),
            "0x4c29c3": bytes.fromhex("c2 04 00"),
            "0x4c29ff": bytes.fromhex("5b"),
            "0x4c2a03": bytes.fromhex("c2 04 00"),
            "0x4c2b12": bytes.fromhex("5b"),
            "0x4c2b16": bytes.fromhex("c2 04 00"),
            "0x4c53e2": bytes.fromhex("5b"),
            "0x4c53e6": bytes.fromhex("c2 04 00"),
            "0x4c2a12": bytes.fromhex("8b 3d cc c5 4c 00"),
        })
        if any(
            body_at(address) != expected
            for address, expected in separate_required.items()
        ):
            raise ValueError(
                "reviewed zInterp strncmp split dispatch route drifted"
            )
        if _cc_identity._immutable_retail_interval(
            _cc_catalog.DEFAULT_REFERENCE,
            start=address_value("0x4c53f0"),
            end_exclusive=address_value("0x4c53f4"),
        ) != struct.pack("<I", address_value("0x4c221e")):
            raise ValueError(
                "reviewed zInterp strncmp split dispatch table entry drifted"
            )
        extension_local_edges_list: list[tuple[str, str]] = []
        for (
            fork_address,
            _count,
            _literal_address,
            extension_call,
            _continuation_body,
            continuation_target,
        ) in complete_ebx9_extension:
            fork = address_value(fork_address)
            extension_local_edges_list.extend((
                (f"0x{fork + 5:x}", f"0x{fork + 12:x}"),
                (f"0x{fork + 10:x}", f"0x{fork + 14:x}"),
            ))
            if continuation_target != "0x4c53d5":
                extension_local_edges_list.append((
                    f"0x{address_value(extension_call) + 7:x}",
                    continuation_target,
                ))
        extension_local_edges = tuple(extension_local_edges_list)
        for branch_address, target_address in (
            ("0x4c223f", "0x4c228b"),
            ("0x4c2290", "0x4c2297"),
            ("0x4c2295", "0x4c2299"),
            ("0x4c22c9", "0x4c232e"),
            ("0x4c2330", "0x4c2337"),
            ("0x4c2335", "0x4c2339"),
            ("0x4c2348", "0x4c236d"),
            ("0x4c2372", "0x4c2379"),
            ("0x4c2377", "0x4c237b"),
            ("0x4c238a", "0x4c23b5"),
            ("0x4c23ba", "0x4c23c1"),
            ("0x4c23bf", "0x4c23c3"),
            ("0x4c23d2", "0x4c241e"),
            ("0x4c2423", "0x4c242a"),
            ("0x4c2428", "0x4c242c"),
            ("0x4c243b", "0x4c246c"),
            ("0x4c2471", "0x4c2478"),
            ("0x4c2476", "0x4c247a"),
            ("0x4c2489", "0x4c24ba"),
            ("0x4c24bf", "0x4c24c6"),
            ("0x4c24c4", "0x4c24c8"),
            ("0x4c24d7", "0x4c2503"),
            ("0x4c2508", "0x4c250f"),
            ("0x4c250d", "0x4c2511"),
            ("0x4c2520", "0x4c255c"),
            ("0x4c2561", "0x4c2568"),
            ("0x4c2566", "0x4c256a"),
            ("0x4c2579", "0x4c258f"),
            ("0x4c258d", "0x4c256f"),
            ("0x4c2591", "0x4c2598"),
            ("0x4c259a", "0x4c25df"),
            ("0x4c25e1", "0x4c25e8"),
            ("0x4c25e6", "0x4c25ea"),
            ("0x4c25f9", "0x4c260f"),
            ("0x4c260d", "0x4c25ef"),
            ("0x4c2611", "0x4c2618"),
            ("0x4c261a", "0x4c265f"),
            ("0x4c2661", "0x4c2668"),
            ("0x4c2666", "0x4c266a"),
            ("0x4c2679", "0x4c2698"),
        ) + extension_local_edges:
            entries = sorted(
                address
                for address, target in branch_targets.items()
                if target == target_address
            )
            if entries != [branch_address]:
                raise ValueError(
                    "reviewed zInterp strncmp split continuation CFG drifted"
                )
        comparison_mismatch_entries = {
            target_address: sorted(
                address
                for address, target in branch_targets.items()
                if target == target_address
            )
            for target_address in ("0x4c2593", "0x4c2613")
        }
        if comparison_mismatch_entries != {
            "0x4c2593": ["0x4c2575", "0x4c2583"],
            "0x4c2613": ["0x4c25f5", "0x4c2603"],
        }:
                raise ValueError(
                    "reviewed zInterp strncmp split comparison CFG drifted"
                )
        for terminal_branch, terminal_target in (
            ("0x4c2961", "0x4c53da"),
            ("0x4c2978", "0x4c2af8"),
            ("0x4c29e5", "0x4c53d5"),
        ):
            terminal_row = rows.get(terminal_branch)
            terminal_observed_target = (
                _cc_cfg._rendered_numeric_branch_target(
                    terminal_row,
                    source="bn",
                    caller_start=address_value(start),
                )
                if terminal_row is not None
                else None
            )
            if (
                terminal_observed_target is None
                or normalize_address(terminal_observed_target) != terminal_target
            ):
                raise ValueError(
                    "reviewed zInterp strncmp split terminal CFG drifted"
                )
        extension_zero_joins = tuple(
            (
                f"0x{address_value(fork_address) + 12:x}",
                f"0x{address_value(fork_address) + 14:x}",
            )
            for fork_address, *_rest in complete_ebx9_extension
        )
        for zero_block, join in (
            ("0x4c2297", "0x4c2299"),
            ("0x4c2337", "0x4c2339"),
            ("0x4c2379", "0x4c237b"),
            ("0x4c23c1", "0x4c23c3"),
            ("0x4c242a", "0x4c242c"),
            ("0x4c2478", "0x4c247a"),
            ("0x4c24c6", "0x4c24c8"),
            ("0x4c250f", "0x4c2511"),
            ("0x4c2568", "0x4c256a"),
            ("0x4c25e8", "0x4c25ea"),
            ("0x4c2668", "0x4c266a"),
        ) + extension_zero_joins:
            zero_row = rows.get(zero_block)
            if (
                zero_row is None
                or address_value(zero_block) + len(zero_row.bytes)
                != address_value(join)
            ):
                raise ValueError(
                    "reviewed zInterp strncmp split operand fork drifted"
                )
    if start == "0x48afe0":
        index_by_address = {
            normalize_address(address): index
            for index, address in enumerate(addresses)
            if address is not None
        }
        route_segments = (
            ("0x48affc", "0x48b009"),
            ("0x48b1c8", "0x48b1de"),
            ("0x48b1f4", "0x48b1f9"),
        )
        route_indices: list[int] = []
        route_valid = True
        for segment_start, segment_end in route_segments:
            first = index_by_address.get(segment_start, -1)
            last = index_by_address.get(segment_end, -1)
            if first < 0 or last < first:
                route_valid = False
                break
            route_indices.extend(range(first, last + 1))
        if (
            not route_valid
            or any(
                _cc_instructions.may_clobber_register(retail_instructions[index], 'ebp')
                for index in route_indices
                if normalize_address(addresses[index]) != "0x48affc"
            )
            or _cc_identity._immutable_retail_interval(
                _cc_catalog.DEFAULT_REFERENCE,
                start=address_value("0x48b298"),
                end_exclusive=address_value("0x48b29c"),
            )
            != struct.pack("<I", address_value("0x48b1f4"))
            or _cc_identity._immutable_retail_interval(
                _cc_catalog.DEFAULT_REFERENCE,
                start=address_value("0x48b2b4"),
                end_exclusive=address_value("0x48b2b5"),
            )
            != b"\x00"
        ):
            raise ValueError(
                "reviewed callback exact lookup/jump EBP-zero route drifted"
            )

    if "dynamic" in spec:
        load_address = str(spec["load"])
        if body_at(load_address) != spec["load_bytes"]:
            raise ValueError("reviewed zInterp ctx callback field load drifted")
        load_indices = [
            index for index, address in enumerate(addresses)
            if address is not None and normalize_address(address) == load_address
        ]
        call_indices = [
            index for index, address in enumerate(addresses)
            if address is not None and normalize_address(address) == call_address
        ]
        if len(load_indices) != 1 or len(call_indices) != 1:
            raise ValueError("reviewed zInterp ctx callback population drifted")
        for instruction in retail_instructions[load_indices[0] + 1 : call_indices[0]]:
            if (
                _cc_cfg._instruction_mnemonic(instruction)
                not in {"cmp", "push", "test"}
                and _cc_instructions.may_clobber_register(instruction, register)
            ):
                raise ValueError("reviewed zInterp ctx callback was clobbered")
        if _cc_cfg._cleanup_after(retail_instructions, call_indices[0]) != spec["cleanup"]:
            raise ValueError("reviewed zInterp ctx callback cleanup drifted")
        return {}

    storage_address = str(spec["storage"])
    identity = indexes.storage_by_address.get(storage_address, "")
    expected_identity = (
        "iat:strncmp"
        if spec["iat"]
        else "storage:recoil:data:0x56aafc"
    )
    if identity != expected_identity:
        raise ValueError(
            "reviewed r3994 register storage lacks its exact tracker identity"
        )
    aliases = [
        address
        for address, current_identity in indexes.storage_by_address.items()
        if current_identity == identity
    ]
    if aliases != [storage_address]:
        raise ValueError(
            "reviewed r3994 register storage identity is aliased or colliding"
        )

    if start == "0x48afe0":
        exact_containers = [
            row
            for row in indexes.storage_containers
            if row.start == address_value(storage_address)
            or row.start
            < address_value(storage_address)
            < row.end_exclusive
        ]
        if (
            len(exact_containers) != 1
            or exact_containers[0].start != address_value(storage_address)
            or exact_containers[0].end_exclusive
            != address_value(storage_address) + 4
            or exact_containers[0].identity != identity
        ):
            raise ValueError(
                "reviewed callback storage lacks its exact four-byte extent"
            )
        _cc_recoil_hud_layout._require_exact_retail_storage_layout(
            address=storage_address,
            size=4,
            section_name=".data",
            zero_filled=True,
        )

    if "load" in spec:
        load_address = str(spec["load"])
        if body_at(load_address) != spec["load_bytes"]:
            raise ValueError("reviewed r3994 callback storage load drifted")
    if spec["iat"]:
        import_rows = [
            row
            for row in (
                retail_import_targets
                if retail_import_targets is not None
                else _retail_import_targets(_cc_catalog.DEFAULT_REFERENCE)[0]
            )
            if getattr(row, "address", "") == storage_address
        ]
        if (
            len(import_rows) != 1
            or getattr(import_rows[0], "dll", "") != "MSVCRT.dll"
            or getattr(import_rows[0], "import_name", "") != "strncmp"
            or getattr(import_rows[0], "import_ordinal", None) is not None
            or getattr(import_rows[0], "iat_end_rva", "") != "0xcc5d0"
        ):
            raise ValueError(
                "reviewed zInterp strncmp immutable import tuple drifted"
            )
        _cc_recoil_hud_layout._require_exact_retail_storage_layout(
            address=storage_address,
            size=4,
            section_name=".rdata",
        )
        # The strncmp load is deliberately after the join at 0x4c20e6.  Check
        # that exact post-join definition and its uninterrupted reach to the
        # call; a predecessor-only backward scan incorrectly stopped at the
        # 0x4c20e2 jump and lost this immutable retail fact.
        call_indices_by_address = {
            selected_call: [
                index
                for index, address in enumerate(addresses)
                if address is not None
                and normalize_address(address) == selected_call
            ]
            for selected_call in call_addresses
        }
        load_address = str(spec["load"])
        load_indices = [
            index for index, address in enumerate(addresses)
            if address is not None and normalize_address(address) == load_address
        ]
        if len(load_indices) != 1 or body_at(load_address) != spec["load_bytes"]:
            raise ValueError(
                "reviewed zInterp strncmp post-join IAT load drifted"
            )
        load_index = load_indices[0]
        call_indices = tuple(
            call_indices_by_address[selected_call][0]
            for selected_call in call_addresses
        )
        if (
            any(len(indices) != 1 for indices in call_indices_by_address.values())
            or call_indices != tuple(sorted(call_indices))
            or load_index >= call_indices[0]
        ):
            raise ValueError(
                "reviewed zInterp strncmp IAT load no longer dominates call"
            )
        group_call_addresses = tuple(
            normalize_address(addresses[index])
            for index in range(load_index + 1, call_indices[-1] + 1)
            if addresses[index] is not None
            and _cc_cfg._instruction_mnemonic(retail_instructions[index]) == "call"
            and _cc_cfg._instruction_operand(retail_instructions[index]).strip().lower()
            == register
        )
        if group_call_addresses != call_addresses:
            raise ValueError(
                "reviewed zInterp strncmp register-call population drifted"
            )
        alternate_exit_writes = {
            "0x4c2104",
            "0x4c2131",
            "0x4c2155",
            "0x4c216b",
            "0x4c21a5",
            "0x4c21df",
        }
        load_runtime_address = address_value(load_address)
        last_group_call_address = address_value(call_addresses[-1])
        observed_group_writes = {
            normalize_address(address)
            for address, row in zip(addresses, retail_instructions)
            if address is not None
            and load_runtime_address < address < last_group_call_address
            and normalize_address(address) not in call_addresses
            and _cc_instructions.may_clobber_register(row, register)
        }
        continuing_ranges = (
            ("0x4c20e6", "0x4c20f4"),
            ("0x4c2175", "0x4c2189"),
            ("0x4c21af", "0x4c21c3"),
            ("0x4c21e9", "0x4c21fd"),
        )
        continuing_write_addresses = {
            normalize_address(address)
            for address, row in zip(addresses, retail_instructions)
            if address is not None
            and any(
                address_value(range_start) < address < address_value(range_end)
                for range_start, range_end in continuing_ranges
            )
            and _cc_instructions.may_clobber_register(row, register)
        }
        if continuing_write_addresses:
            raise ValueError(
                "reviewed zInterp strncmp continuing-route provenance was clobbered"
            )
        if observed_group_writes != alternate_exit_writes:
            raise ValueError(
                "reviewed zInterp strncmp alternate-exit write population drifted"
            )
        if any(
            _cc_cfg._cleanup_after(retail_instructions, call_index) != spec["cleanup"]
            for call_index in call_indices
        ):
            raise ValueError("reviewed zInterp strncmp cleanup drifted")
        separate_load_indices = [
            index
            for index, address in enumerate(addresses)
            if address is not None
            and normalize_address(address) == "0x4c222a"
        ]
        separate_call_indices_by_address = {
            selected_call: [
                index
                for index, address in enumerate(addresses)
                if address is not None
                and normalize_address(address) == selected_call
            ]
            for selected_call in separate_zinterp_strncmp_calls
        }
        if (
            len(separate_load_indices) != 1
            or any(
                len(indices) != 1
                for indices in separate_call_indices_by_address.values()
            )
        ):
            raise ValueError(
                "reviewed zInterp strncmp split definition population drifted"
            )
        separate_load_index = separate_load_indices[0]
        separate_call_indices = tuple(
            separate_call_indices_by_address[selected_call][0]
            for selected_call in separate_zinterp_strncmp_calls
        )
        if (
            separate_call_indices != tuple(sorted(separate_call_indices))
            or separate_load_index >= separate_call_indices[0]
        ):
            raise ValueError(
                "reviewed zInterp strncmp split definition population drifted"
            )
        separate_group_calls = tuple(
            normalize_address(addresses[index])
            for index in range(
                separate_load_index + 1, separate_call_indices[-1] + 1
            )
            if addresses[index] is not None
            and _cc_cfg._instruction_mnemonic(retail_instructions[index]) == "call"
            and _cc_cfg._instruction_operand(retail_instructions[index]).strip().lower()
            == "ebx"
        )
        if separate_group_calls != separate_zinterp_strncmp_calls:
            raise ValueError(
                "reviewed zInterp strncmp split register-call population drifted"
            )
        observed_split_writes = {
            normalize_address(address)
            for address, row in zip(addresses, retail_instructions)
            if address is not None
            and address_value("0x4c222a") < address < address_value("0x4c29dc")
            and _cc_instructions.may_clobber_register(row, 'ebx')
        }
        if observed_split_writes != {
            "0x4c2284",
            "0x4c2327",
            "0x4c2366",
            "0x4c23ae",
            "0x4c2417",
            "0x4c2465",
            "0x4c24b3",
            "0x4c24fc",
            "0x4c2555",
            "0x4c25d8",
            "0x4c2658",
            "0x4c2691",
            "0x4c26df",
            "0x4c272d",
            "0x4c2796",
            "0x4c27dd",
            "0x4c2824",
            "0x4c2866",
            "0x4c28af",
            "0x4c2910",
            "0x4c29bf",
        }:
            raise ValueError(
                "reviewed zInterp strncmp split alternate-return write "
                "population drifted"
            )
        continuing_comparison_ranges = (
            (address_value("0x4c255c"), address_value("0x4c259c")),
            (address_value("0x4c25df"), address_value("0x4c261c")),
            (address_value("0x4c265f"), address_value("0x4c2672")),
        )
        continuing_route_calls = tuple(
            normalize_address(address)
            for address, row in zip(addresses, retail_instructions)
            if address is not None
            and any(
                range_start <= address < range_end
                for range_start, range_end in continuing_comparison_ranges
            )
            and _cc_cfg._instruction_mnemonic(row) == "call"
        )
        continuing_route_ebx_writes = tuple(
            normalize_address(address)
            for address, row in zip(addresses, retail_instructions)
            if address is not None
            and any(
                range_start <= address < range_end
                for range_start, range_end in continuing_comparison_ranges
            )
            and _cc_instructions.may_clobber_register(row, 'ebx')
        )
        if continuing_route_calls or continuing_route_ebx_writes:
            raise ValueError(
                "reviewed zInterp strncmp split continuing comparison route "
                "gained a call or EBX write"
            )
        expected_terminal_comparison_calls = {
            "0x4c2524": "0x4c1a00",
            "0x4c252f": "0x4c1a00",
            "0x4c2548": "0x44a2f0",
            "0x4c25ab": "0x44a380",
            "0x4c25b2": "0x4c1a00",
            "0x4c25cb": "0x44a2f0",
            "0x4c262b": "0x44a380",
            "0x4c2632": "0x4c1a00",
            "0x4c264b": "0x44a2f0",
        }
        strict_comparison_call_rows = [
            (normalize_address(address), row)
            for address, row in zip(addresses, retail_instructions)
            if address is not None
            and address_value("0x4c2519") < address < address_value("0x4c2672")
            and _cc_cfg._instruction_mnemonic(row) == "call"
        ]
        if tuple(
            address for address, _row in strict_comparison_call_rows
        ) != tuple(expected_terminal_comparison_calls):
            raise ValueError(
                "reviewed zInterp strncmp split terminal comparison-call "
                "population drifted"
            )
        for call_site, instruction in strict_comparison_call_rows:
            expected_target = address_value(
                expected_terminal_comparison_calls[call_site]
            )
            expected_body = b"\xe8" + struct.pack(
                "<i", expected_target - (address_value(call_site) + 5)
            )
            instruction_body = bytes(
                int(item, 16) for item in instruction.bytes
            )
            decoded_rel32_target = (
                address_value(call_site)
                + 5
                + struct.unpack_from("<i", instruction_body, 1)[0]
                if len(instruction_body) == 5
                and instruction_body[:1] == b"\xe8"
                else None
            )
            rendered_target = _cc_cfg._rendered_numeric_branch_target(
                instruction,
                source="bn",
                caller_start=address_value(start),
            )
            if (
                instruction_body != expected_body
                or decoded_rel32_target != expected_target
                or (
                    rendered_target is not None
                    and rendered_target != decoded_rel32_target
                )
            ):
                raise ValueError(
                    "reviewed zInterp strncmp split terminal comparison-call "
                    "target drifted"
                )
        terminal_comparison_routes = (
            ("0x4c2520", "0x4c2522", "0x4c255c", "0x4c2555", "0x4c2559"),
            ("0x4c259a", "0x4c259c", "0x4c25df", "0x4c25d8", "0x4c25dc"),
            ("0x4c261a", "0x4c261c", "0x4c265f", "0x4c2658", "0x4c265c"),
        )
        for branch, route_start, route_end, pop_address, return_address in (
            terminal_comparison_routes
        ):
            branch_row = rows.get(branch)
            return_row = rows.get(return_address)
            if (
                branch_row is None
                or address_value(branch) + len(branch_row.bytes)
                != address_value(route_start)
                or body_at(pop_address) != bytes.fromhex("5b")
                or return_row is None
                or body_at(return_address) != bytes.fromhex("c2 04 00")
                or address_value(return_address) + len(return_row.bytes)
                != address_value(route_end)
            ):
                raise ValueError(
                    "reviewed zInterp strncmp split terminal comparison route "
                    "no longer returns"
                )
            inbound_entries = sorted(
                source_address
                for source_address, target_address in branch_targets.items()
                if address_value(route_start)
                <= address_value(target_address)
                < address_value(route_end)
                and not (
                    address_value(route_start)
                    <= address_value(source_address)
                    < address_value(route_end)
                )
            )
            if inbound_entries:
                raise ValueError(
                    "reviewed zInterp strncmp split terminal comparison route "
                    "gained an alternate entry"
                )
        if any(
            _cc_cfg._cleanup_after(retail_instructions, call_index) != spec["cleanup"]
            for call_index in separate_call_indices
        ):
            raise ValueError(
                "reviewed zInterp strncmp split route cleanup drifted"
            )

        # Finish the same reviewed IAT census with finite SSA components.  The
        # component roots and call sites below are retail facts; provisional BN
        # names never participate in the identity proof.
        early_edi48_calls = (
            ("0x4c2a20", 0x0B, "0x4e55bc", "0x4c2a7d"),
            ("0x4c2a91", 0x0A, "0x4e5584", "0x4c2ab7"),
            ("0x4c2acb", 0x0A, "0x4e5578", "0x4c2b2f"),
            ("0x4c2b43", 0x0D, "0x4e5540", "0x4c2b79"),
            ("0x4c2b8d", 0x11, "0x4e552c", "0x4c2bc3"),
            ("0x4c2bd7", 0x14, "0x4e5514", "0x4c53d5"),
        )
        remaining_slot_components = (
            ("0x4c2cb0", "edi", (
                ("0x4c2cbe", 0x08, "0x4e54f8", "0x4c2d08"),
                ("0x4c2d1c", 0x0B, "0x4e54d8", "0x4c2d67"),
                ("0x4c2d7b", 0x08, "0x4e54b4", "0x4c53d5"),
            )),
            ("0x4c2e11", "edi", (
                ("0x4c2e1f", 0x10, "0x4e5464", "0x4c2e5f"),
                ("0x4c2e73", 0x11, "0x4e5440", "0x4c2ea5"),
                ("0x4c2eb9", 0x0F, "0x4e5430", "0x4c53d3"),
            )),
            ("0x4c2efa", "edi", (
                ("0x4c2f08", 0x10, "0x4e5408", "0x4c2f40"),
                ("0x4c2f54", 0x08, "0x4e53fc", "0x4c2f67"),
                ("0x4c2f7b", 0x0E, "0x4e53ec", "0x4c2fa7"),
                ("0x4c2fbb", 0x0F, "0x4e53dc", "0x4c2fee"),
                ("0x4c3002", 0x0D, "0x4e53cc", "0x4c3055"),
                ("0x4c3069", 0x0F, "0x4e53bc", "0x4c309c"),
                ("0x4c30b0", 0x16, "0x4e53a4", "0x4c30d3"),
                ("0x4c30e7", 0x13, "0x4e5390", "0x4c3113"),
                ("0x4c3127", 0x13, "0x4e537c", "0x4c319f"),
                ("0x4c31b3", 0x13, "0x4e5368", "0x4c31d6"),
                ("0x4c31ea", 0x0E, "0x4e5358", "0x4c322d"),
                ("0x4c3241", 0x13, "0x4e5344", "0x4c3292"),
                ("0x4c32a6", 0x11, "0x4e5314", "0x4c32f9"),
                ("0x4c330d", 0x09, "0x4e5308", "0x4c3333"),
                ("0x4c3347", 0x0B, "0x4e52fc", "0x4c337f"),
                ("0x4c3393", 0x0B, "0x4e52f0", "0x4c53d5"),
            )),
            ("0x4c33fd", "edi", (
                ("0x4c340b", 0x0D, "0x4e52e0", "0x4c346b"),
                ("0x4c347f", 0x07, "0x4e52d8", "0x4c34a2"),
                ("0x4c34b6", 0x0B, "0x4e52cc", "0x4c3508"),
                ("0x4c351c", 0x08, "0x4e52c0", "0x4c35cf"),
                ("0x4c357f", 0x06, "0x4e52b8", "0x4c35ad"),
                ("0x4c35e3", 0x11, "0x4e52a4", "0x4c361c"),
                ("0x4c3630", 0x0F, "0x4e5294", "0x4c36c4"),
                ("0x4c36d8", 0x0E, "0x4e5284", "0x4c3737"),
                ("0x4c374b", 0x12, "0x4e5270", "0x4c53d3"),
            )),
            ("0x4c37c7", "edi", (
                ("0x4c37d5", 0x09, "0x4e5264", "0x4c37e8"),
                ("0x4c37fc", 0x0A, "0x4e5258", "0x4c380f"),
                ("0x4c3823", 0x06, "0x4e5250", "0x4c3836"),
                ("0x4c384a", 0x07, "0x4e5248", "0x4c385d"),
                ("0x4c3871", 0x0B, "0x4e523c", "0x4c3884"),
                ("0x4c3898", 0x06, "0x4e5234", "0x4c38cf"),
                ("0x4c38e3", 0x09, "0x4e5228", "0x4c38f6"),
                ("0x4c390a", 0x08, "0x4e521c", "0x4c391d"),
                ("0x4c3931", 0x0D, "0x4e520c", "0x4c395d"),
                ("0x4c3971", 0x12, "0x4e51f8", "0x4c399d"),
                ("0x4c39b1", 0x10, "0x4e51e4", "0x4c39dd"),
                ("0x4c39f1", 0x0F, "0x4e51d4", "0x4c3a1d"),
                ("0x4c3a31", 0x10, "0x4e51c0", "0x4c53d5"),
            )),
            ("0x4c3a6d", "ebx", (
                ("0x4c3a7b", 0x10, "0x4e51ac", "0x4c3ad7"),
                ("0x4c3b8d", 0x1F, "0x4e5144", "0x4c3bfa"),
                ("0x4c3c10", 0x0E, "0x4e5134", "0x4c3c88"),
                ("0x4c3c9e", 0x0D, "0x4e5124", "0x4c3cf1"),
                ("0x4c3d07", 0x19, "0x4e5108", "0x4c3d33"),
                ("0x4c3d49", 0x11, "0x4e50f4", "0x4c3d75"),
                ("0x4c3d8b", 0x10, "0x4e50e0", "0x4c3dcf"),
                ("0x4c3de5", 0x11, "0x4e50cc", "0x4c3e23"),
                ("0x4c3e39", 0x17, "0x4e50b4", "0x4c3e65"),
                ("0x4c3e7b", 0x12, "0x4e50a0", "0x4c3eae"),
                ("0x4c3ec4", 0x11, "0x4e508c", "0x4c3f02"),
                ("0x4c3f18", 0x13, "0x4e5078", "0x4c3f60"),
                ("0x4c3f76", 0x17, "0x4e5060", "0x4c401d"),
                ("0x4c4033", 0x11, "0x4e500c", "0x4c40ec"),
                ("0x4c4102", 0x17, "0x4e4fc4", "0x4c4148"),
                ("0x4c415e", 0x26, "0x4e4f9c", "0x4c419b"),
                ("0x4c41b1", 0x25, "0x4e4f74", "0x4c41ee"),
                ("0x4c4204", 0x16, "0x4e4f5c", "0x4c4262"),
                ("0x4c4278", 0x11, "0x4e4f48", "0x4c53d5"),
            )),
            ("0x4c42db", "ebx", (
                ("0x4c42e9", 0x12, "0x4e4f34", "0x4c430d"),
                ("0x4c4323", 0x0E, "0x4e4f24", "0x4c437d"),
                ("0x4c4407", 0x0E, "0x4e4ee0", "0x4c53d3"),
            )),
            ("0x4c4439", "edi", (
                ("0x4c4447", 0x0A, "0x4e4ed4", "0x4c446d"),
                ("0x4c4481", 0x0A, "0x4e4ec8", "0x4c53d5"),
            )),
            ("0x4c44b7", "edi", (
                ("0x4c44c5", 0x03, "0x4e4ec4", "0x4c468b"),
                ("0x4c44e8", 0x0B, "0x4e4eb8", "0x4c453c"),
                ("0x4c4550", 0x06, "0x4e4eb0", "0x4c4587"),
                ("0x4c459b", 0x0C, "0x4e4ea0", "0x4c45c7"),
                ("0x4c45db", 0x0A, "0x4e4e94", "0x4c4607"),
                ("0x4c461b", 0x0C, "0x4e4e88", "0x4c4647"),
                ("0x4c465b", 0x0C, "0x4e4e78", "0x4c53da"),
                ("0x4c469e", 0x12, "0x4e4e64", "0x4c46ca"),
                ("0x4c46de", 0x0F, "0x4e4e54", "0x4c471e"),
                ("0x4c4732", 0x14, "0x4e4e3c", "0x4c475f"),
                ("0x4c4773", 0x14, "0x4e4e24", "0x4c47a0"),
                ("0x4c47b4", 0x15, "0x4e4e0c", "0x4c47da"),
                ("0x4c47ee", 0x14, "0x4e4df4", "0x4c4814"),
                ("0x4c4828", 0x13, "0x4e4de0", "0x4c484e"),
                ("0x4c4862", 0x10, "0x4e4dcc", "0x4c488e"),
                ("0x4c48a2", 0x13, "0x4e4db8", "0x4c48ce"),
                ("0x4c48e2", 0x0B, "0x4e4dac", "0x4c490e"),
                ("0x4c4922", 0x0E, "0x4e4d9c", "0x4c4948"),
                ("0x4c495c", 0x11, "0x4e4d88", "0x4c4982"),
                ("0x4c4996", 0x20, "0x4e4d64", "0x4c49d9"),
                ("0x4c49ed", 0x1B, "0x4e4d48", "0x4c4a13"),
                ("0x4c4a27", 0x14, "0x4e4d30", "0x4c4a54"),
                ("0x4c4a68", 0x14, "0x4e4d10", "0x4c4a95"),
                ("0x4c4aa9", 0x19, "0x4e4cf4", "0x4c4ad6"),
                ("0x4c4aea", 0x0C, "0x4e4ce4", "0x4c4b16"),
                ("0x4c4b25", 0x19, "0x4e4cc8", None),
            )),
            ("0x4c5286", "ebx", (
                ("0x4c5296", 0x06, "0x4e4a98", "0x4c52be"),
                ("0x4c52c6", 0x0B, "0x4e4a8c", "0x4c52ee"),
                ("0x4c52f6", 0x03, "0x4e2108", "0x4c531b"),
            )),
        )
        shifted_remainder_result_sites = {
            "0x4c2d7b": (bytes.fromhex("0f 85 4b 26 00 00"), "0x4c2e02"),
            "0x4c3393": (bytes.fromhex("0f 85 33 20 00 00"), "0x4c33ee"),
            "0x4c3a31": (bytes.fromhex("0f 85 95 19 00 00"), "0x4c3a5e"),
            "0x4c4278": (bytes.fromhex("0f 85 4e 11 00 00"), "0x4c42cc"),
            "0x4c4481": (bytes.fromhex("0f 85 45 0f 00 00"), "0x4c44a8"),
        }
        post_test_move_remainder_site = "0x4c44c5"

        def runtime_address_plus(address: str, delta: int) -> str:
            return normalize_address(f"0x{address_value(address) + delta:x}")

        def exact_jne_target(branch_address: str) -> str:
            body = body_at(branch_address)
            if len(body) == 2 and body[:1] == b"\x75":
                delta = struct.unpack("<b", body[1:])[0]
            elif len(body) == 6 and body[:2] == b"\x0f\x85":
                delta = struct.unpack_from("<i", body, 2)[0]
            else:
                return ""
            return normalize_address(
                f"0x{address_value(branch_address) + len(body) + delta:x}"
            )

        def require_exact_slot_call(
            call_site: str,
            argument_length: int,
            literal_address: str,
            call_register: str,
            source_register: str,
            continuation_target: str | None,
            *,
            shifted_final_test: bool = False,
            post_test_move: bool = False,
        ) -> None:
            call_opcode = b"\xff\xd7" if call_register == "edi" else b"\xff\xd3"
            source_opcode = b"\x50" if source_register == "eax" else b"\x57"
            call_row = rows.get(call_site)
            noncontiguous_source_producer = call_site == "0x4c4b25"
            if noncontiguous_source_producer:
                producer_call = body_at("0x4c4b1f")
                producer_target = (
                    address_value("0x4c4b24")
                    + struct.unpack_from("<i", producer_call, 1)[0]
                    if len(producer_call) == 5 and producer_call[:1] == b"\xe8"
                    else None
                )
                if (
                    body_at("0x4c4b16") != b"\x6a\x19"
                    or body_at("0x4c4b18")
                    != bytes.fromhex("68 c8 4c 4e 00")
                    or body_at("0x4c4b1d") != b"\x8b\xce"
                    or producer_target != address_value("0x4c5510")
                    or _cc_identity._immutable_retail_interval(
                        _cc_catalog.DEFAULT_REFERENCE,
                        start=address_value("0x4c5510"),
                        end_exclusive=address_value("0x4c551e"),
                    )
                    != bytes.fromhex("8b 41 08 85 c0 76 04 8b 41 20 c3 33 c0 c3")
                    or body_at("0x4c4b24") != b"\x50"
                ):
                    raise ValueError(
                        "reviewed zInterp strncmp noncontiguous source producer drifted"
                    )
            elif (
                body_at(runtime_address_plus(call_site, -8))
                != bytes((0x6A, argument_length))
                or body_at(runtime_address_plus(call_site, -6))
                != b"\x68" + struct.pack("<I", address_value(literal_address))
                or body_at(runtime_address_plus(call_site, -1)) != source_opcode
            ):
                raise ValueError(
                    "reviewed zInterp strncmp remainder argument/call/cleanup drifted"
                )
            if (
                body_at(runtime_address_plus(call_site, -1)) != source_opcode
                or body_at(call_site) != call_opcode
                or call_row is None
                or _cc_cfg._instruction_mnemonic(call_row) != "call"
                or _cc_cfg._instruction_operand(call_row).strip().lower()
                != call_register
                or body_at(runtime_address_plus(call_site, 2))
                != bytes.fromhex("83 c4 0c")
            ):
                raise ValueError(
                    "reviewed zInterp strncmp remainder argument/call/cleanup drifted"
                )
            if continuation_target is None:
                if (
                    body_at(runtime_address_plus(call_site, 5)) != b"\xf7\xd8"
                    or body_at(runtime_address_plus(call_site, 7)) != b"\x1b\xc0"
                    or body_at(runtime_address_plus(call_site, 9)) != b"\x40"
                    or body_at(runtime_address_plus(call_site, 10))
                    != bytes.fromhex("89 44 24 3c")
                    or body_at(runtime_address_plus(call_site, 14)) != b"\x74\x24"
                ):
                    raise ValueError(
                        "reviewed zInterp strncmp remainder special result gate drifted"
                    )
                return
            test_delta = 7 if shifted_final_test else 5
            branch_delta = 10 if post_test_move else (9 if shifted_final_test else 7)
            if (
                shifted_final_test
                and body_at(runtime_address_plus(call_site, 5)) != b"\x8b\xce"
            ):
                raise ValueError(
                    "reviewed zInterp strncmp remainder shifted result move drifted"
                )
            if (
                post_test_move
                and body_at(runtime_address_plus(call_site, 7))
                != bytes.fromhex("8b 46 08")
            ):
                raise ValueError(
                    "reviewed zInterp strncmp remainder post-test move drifted"
                )
            if (
                body_at(runtime_address_plus(call_site, test_delta)) != b"\x85\xc0"
                or exact_jne_target(runtime_address_plus(call_site, branch_delta))
                != continuation_target
            ):
                raise ValueError(
                    "reviewed zInterp strncmp remainder continuation CFG drifted"
                )

        if body_at("0x4c2a12") != bytes.fromhex("8b 3d cc c5 4c 00"):
            raise ValueError("reviewed zInterp strncmp EDI#48 root drifted")
        for call_site, length, literal, target in early_edi48_calls:
            require_exact_slot_call(
                call_site, length, literal, "edi", "eax", target,
                shifted_final_test=call_site == "0x4c2bd7",
            )
        early_edi_calls = tuple(
            normalize_address(address)
            for address, row in zip(addresses, retail_instructions)
            if address is not None
            and address_value("0x4c2a12") < address < address_value("0x4c2c42")
            and _cc_cfg._instruction_mnemonic(row) == "call"
            and _cc_cfg._instruction_operand(row).strip().lower() == "edi"
        )
        if early_edi_calls != tuple(row[0] for row in early_edi48_calls):
            raise ValueError(
                "reviewed zInterp strncmp EDI#48 call population drifted"
            )
        for range_start, range_end in (
            ("0x4c2a7d", "0x4c2a91"),
            ("0x4c2ab7", "0x4c2acb"),
            ("0x4c2b2f", "0x4c2b43"),
            ("0x4c2b79", "0x4c2b8d"),
            ("0x4c2bc3", "0x4c2bd7"),
        ):
            if any(
                address is not None
                and address_value(range_start) <= address < address_value(range_end)
                and _cc_instructions.may_clobber_register(row, 'edi')
                for address, row in zip(addresses, retail_instructions)
            ):
                raise ValueError(
                    "reviewed zInterp strncmp EDI#48 continuing route was clobbered"
                )

        direct_slot_calls = tuple(
            normalize_address(address)
            for address, row in zip(addresses, retail_instructions)
            if address is not None
            and address_value("0x4c2a12") <= address < address_value("0x4c2cb0")
            and body_at(normalize_address(address)) == bytes.fromhex("ff 15 cc c5 4c 00")
        )
        if direct_slot_calls != ("0x4c2c42", "0x4c2c7d"):
            raise ValueError(
                "reviewed zInterp strncmp direct-memory slot population drifted"
            )
        for call_site, literal, branch_address, branch_body, return_address in (
            (
                "0x4c2c42", "0x4e550c", "0x4c2c4f",
                bytes.fromhex("0f 85 80 27 00 00"), "0x4c2c66",
            ),
            (
                "0x4c2c7d", "0x4e5504", "0x4c2c8a",
                bytes.fromhex("0f 85 45 27 00 00"), "0x4c2ca1",
            ),
        ):
            if (
                body_at(runtime_address_plus(call_site, -8)) != b"\x6a\x04"
                or body_at(runtime_address_plus(call_site, -6))
                != b"\x68" + struct.pack("<I", address_value(literal))
                or body_at(runtime_address_plus(call_site, -1)) != b"\x50"
                or body_at(runtime_address_plus(call_site, 6))
                != bytes.fromhex("83 c4 0c")
                or body_at(runtime_address_plus(call_site, 9)) != b"\x8b\xce"
                or body_at(runtime_address_plus(call_site, 11)) != b"\x85\xc0"
                or body_at(branch_address) != branch_body
                or exact_jne_target(branch_address) != "0x4c53d5"
            ):
                raise ValueError(
                    "reviewed zInterp strncmp direct-memory slot route drifted"
                )
            zero_start = address_value(branch_address) + len(branch_body)
            zero_end = address_value(return_address)
            zero_calls = []
            for address, row in zip(addresses, retail_instructions):
                if (
                    address is None
                    or not zero_start <= address < zero_end
                    or _cc_cfg._instruction_mnemonic(row) != "call"
                ):
                    continue
                body = body_at(normalize_address(address))
                target = (
                    address + 5 + struct.unpack_from("<i", body, 1)[0]
                    if len(body) == 5 and body[:1] == b"\xe8"
                    else None
                )
                zero_calls.append(target)
            if (
                zero_calls != [address_value("0x4c1870")]
                or body_at(return_address) != bytes.fromhex("c2 04 00")
            ):
                raise ValueError(
                    "reviewed zInterp strncmp direct-memory zero route drifted"
                )

        expected_slot_loads = tuple(
            component[0] for component in remaining_slot_components
        )
        observed_slot_loads = tuple(
            normalize_address(address)
            for address, row in zip(addresses, retail_instructions)
            if address is not None
            and address_value("0x4c2cb0") <= address < address_value("0x4c53e9")
            and body_at(normalize_address(address)) in {
                bytes.fromhex("8b 3d cc c5 4c 00"),
                bytes.fromhex("8b 1d cc c5 4c 00"),
            }
        )
        if observed_slot_loads != expected_slot_loads:
            raise ValueError(
                "reviewed zInterp strncmp remainder slot-load population drifted"
            )
        remaining_register_calls: list[str] = []
        for load_address, component_register, component_calls in (
            remaining_slot_components
        ):
            expected_load = (
                bytes.fromhex("8b 3d cc c5 4c 00")
                if component_register == "edi"
                else bytes.fromhex("8b 1d cc c5 4c 00")
            )
            if body_at(load_address) != expected_load:
                raise ValueError(
                    "reviewed zInterp strncmp remainder component root drifted"
                )
            source_register = "edi" if load_address == "0x4c5286" else "eax"
            for call_site, length, literal, target in component_calls:
                require_exact_slot_call(
                    call_site,
                    length,
                    literal,
                    component_register,
                    source_register,
                    target,
                    shifted_final_test=call_site in shifted_remainder_result_sites,
                    post_test_move=call_site == post_test_move_remainder_site,
                )
                remaining_register_calls.append(call_site)
        if len(remaining_register_calls) != 97 or len(set(remaining_register_calls)) != 97:
            raise ValueError(
                "reviewed zInterp strncmp remainder component population drifted"
            )
        contiguous_edi_eax_sites = {
            call_site
            for load_address, component_register, component_calls
            in remaining_slot_components
            for call_site, _length, _literal, _target in component_calls
            if component_register == "edi"
            and load_address != "0x4c5286"
            and call_site != "0x4c4b25"
        }
        contiguous_ebx_eax_sites = {
            call_site
            for load_address, component_register, component_calls
            in remaining_slot_components
            for call_site, _length, _literal, _target in component_calls
            if component_register == "ebx" and load_address != "0x4c5286"
        }
        contiguous_ebx_edi_sites = {
            call_site
            for load_address, component_register, component_calls
            in remaining_slot_components
            for call_site, _length, _literal, _target in component_calls
            if component_register == "ebx" and load_address == "0x4c5286"
        }
        if (
            len(contiguous_edi_eax_sites) != 71
            or len(contiguous_ebx_eax_sites) != 22
            or len(contiguous_ebx_edi_sites) != 3
            or contiguous_edi_eax_sites | contiguous_ebx_eax_sites
            | contiguous_ebx_edi_sites | {"0x4c4b25"}
            != set(remaining_register_calls)
        ):
            raise ValueError(
                "reviewed zInterp strncmp remainder source-producer population drifted"
            )
        observed_shifted_result_sites = {
            call_site for call_site in remaining_register_calls
            if body_at(runtime_address_plus(call_site, 5)) == b"\x8b\xce"
        }
        observed_canonical_result_sites = {
            call_site for call_site in remaining_register_calls
            if body_at(runtime_address_plus(call_site, 5)) == b"\x85\xc0"
            and exact_jne_target(runtime_address_plus(call_site, 7))
        }
        observed_post_test_move_sites = {
            call_site for call_site in remaining_register_calls
            if body_at(runtime_address_plus(call_site, 5)) == b"\x85\xc0"
            and body_at(runtime_address_plus(call_site, 7))
            == bytes.fromhex("8b 46 08")
        }
        if (
            observed_shifted_result_sites != set(shifted_remainder_result_sites)
            or observed_post_test_move_sites != {post_test_move_remainder_site}
            or len(observed_canonical_result_sites) != 90
            or observed_shifted_result_sites & observed_canonical_result_sites
            or observed_shifted_result_sites | observed_canonical_result_sites
            | observed_post_test_move_sites
            != set(remaining_register_calls) - {"0x4c4b25"}
        ):
            raise ValueError(
                "reviewed zInterp strncmp remainder result-prefix population drifted"
            )
        for call_site, (branch_body, return_address) in (
            shifted_remainder_result_sites.items()
        ):
            if (
                body_at(runtime_address_plus(call_site, 9)) != branch_body
                or exact_jne_target(runtime_address_plus(call_site, 9))
                != "0x4c53d5"
                or body_at(return_address) != bytes.fromhex("c2 04 00")
            ):
                raise ValueError(
                    "reviewed zInterp strncmp remainder shifted terminal route drifted"
                )
        if (
            body_at("0x4c44c5") != b"\xff\xd7"
            or body_at("0x4c44c7") != bytes.fromhex("83 c4 0c")
            or body_at("0x4c44ca") != b"\x85\xc0"
            or body_at("0x4c44cc") != bytes.fromhex("8b 46 08")
            or body_at("0x4c44cf") != bytes.fromhex("0f 85 b6 01 00 00")
            or exact_jne_target("0x4c44cf") != "0x4c468b"
        ):
            raise ValueError(
                "reviewed zInterp strncmp remainder post-test branch drifted"
            )
        supplied_printf_identity = indexes.storage_by_address.get("0x4cc4dc", "")
        printf_register_calls = ("0x4c2dd9", "0x4c2df1")
        printf_required_bodies = {
            "0x4c2d9f": bytes.fromhex("8b 3d dc c4 4c 00"),
            "0x4c2da5": bytes.fromhex("8b 41 34"),
            "0x4c2da8": bytes.fromhex("8d 50 ff"),
            "0x4c2dab": bytes.fromhex("83 fa 04"),
            "0x4c2dae": bytes.fromhex("77 23"),
            "0x4c2db0": bytes.fromhex("ff 24 95 6c 54 4c 00"),
            "0x4c2dbc": bytes.fromhex("eb 25"),
            "0x4c2dc3": bytes.fromhex("eb 1e"),
            "0x4c2dca": bytes.fromhex("eb 17"),
            "0x4c2dd1": bytes.fromhex("eb 10"),
            "0x4c2dd3": b"\x50",
            "0x4c2dd4": bytes.fromhex("68 94 54 4e 00"),
            "0x4c2dd9": b"\xff\xd7",
            "0x4c2ddb": bytes.fromhex("83 c4 08"),
            "0x4c2dde": bytes.fromhex("b8 01 00 00 00"),
            "0x4c2de3": bytes.fromhex("3b c3"),
            "0x4c2de5": bytes.fromhex("0f 84 ef 25 00 00"),
            "0x4c2deb": b"\x56",
            "0x4c2dec": bytes.fromhex("68 78 54 4e 00"),
            "0x4c2df1": b"\xff\xd7",
            "0x4c2df3": bytes.fromhex("83 c4 08"),
            "0x4c2df6": bytes.fromhex("b8 01 00 00 00"),
            "0x4c2dfb": b"\x5f",
            "0x4c2dfc": b"\x5e",
            "0x4c2dfd": b"\x5d",
            "0x4c2dfe": b"\x5b",
            "0x4c2dff": bytes.fromhex("83 c4 28"),
            "0x4c2e02": bytes.fromhex("c2 04 00"),
        }
        if any(
            body_at(address) != expected_body
            for address, expected_body in printf_required_bodies.items()
        ):
            raise ValueError("reviewed zInterp printf component bytes drifted")
        if _cc_identity._immutable_retail_interval(
            _cc_catalog.DEFAULT_REFERENCE,
            start=address_value("0x4c546c"),
            end_exclusive=address_value("0x4c5480"),
        ) != b"".join(
            struct.pack("<I", address_value(target))
            for target in (
                "0x4c2db7", "0x4c2dbe", "0x4c2dc5", "0x4c2dd3", "0x4c2dcc",
            )
        ):
            raise ValueError("reviewed zInterp printf switch table drifted")
        expected_printf_branches = {
            "0x4c2dae": "0x4c2dd3",
            "0x4c2dbc": "0x4c2de3",
            "0x4c2dc3": "0x4c2de3",
            "0x4c2dca": "0x4c2de3",
            "0x4c2dd1": "0x4c2de3",
            "0x4c2de5": "0x4c53da",
        }
        if any(
            branch_targets.get(branch) != target
            for branch, target in expected_printf_branches.items()
        ):
            raise ValueError("reviewed zInterp printf switch CFG drifted")
        expected_direct_targets = {
            "0x4c2db7": "0x44db00",
            "0x4c2dbe": "0x450240",
            "0x4c2dc5": "0x44db00",
            "0x4c2dcc": "0x44db00",
        }
        for call_site, expected_target in expected_direct_targets.items():
            call_body = body_at(call_site)
            decoded_target = (
                address_value(runtime_address_plus(call_site, 5))
                + struct.unpack_from("<i", call_body, 1)[0]
                if len(call_body) == 5 and call_body[:1] == b"\xe8"
                else None
            )
            if decoded_target != address_value(expected_target):
                raise ValueError(
                    "reviewed zInterp printf switch direct-call route drifted"
                )
        if any(
            address is not None
            and address_value("0x4c2da5") <= address < address_value("0x4c2df1")
            and normalize_address(address) != "0x4c2d9f"
            and _cc_instructions.may_clobber_register(row, 'edi')
            for address, row in zip(addresses, retail_instructions)
        ):
            raise ValueError("reviewed zInterp printf continuing EDI was clobbered")
        printf_direct_calls = {
            "0x4c27cc": (
                8,
                {
                    "0x4c27c6": b"\x50",
                    "0x4c27c7": bytes.fromhex("68 48 57 4e 00"),
                    "0x4c27d5": bytes.fromhex("b8 01 00 00 00"),
                },
            ),
            "0x4c2813": (
                8,
                {
                    "0x4c280d": b"\x50",
                    "0x4c280e": bytes.fromhex("68 18 57 4e 00"),
                    "0x4c281c": bytes.fromhex("b8 01 00 00 00"),
                },
            ),
            "0x4c4365": (
                12,
                {
                    "0x4c435e": b"\x56",
                    "0x4c435f": b"\x57",
                    "0x4c4360": bytes.fromhex("68 0c 4f 4e 00"),
                    "0x4c436e": bytes.fromhex("b8 01 00 00 00"),
                },
            ),
            "0x4c5321": (
                8,
                {
                    "0x4c531b": b"\x57",
                    "0x4c531c": bytes.fromhex("68 74 4a 4e 00"),
                    "0x4c532a": bytes.fromhex("b8 01 00 00 00"),
                },
            ),
        }
        observed_printf_slot_xrefs = tuple(
            normalize_address(address)
            for address in addresses
            if address is not None
            and body_at(normalize_address(address)) in {
                bytes.fromhex("8b 3d dc c4 4c 00"),
                bytes.fromhex("ff 15 dc c4 4c 00"),
            }
        )
        if observed_printf_slot_xrefs != (
            "0x4c27cc", "0x4c2813", "0x4c2d9f", "0x4c4365", "0x4c5321",
        ):
            raise ValueError("reviewed zInterp printf slot xref population drifted")
        if _cc_identity._immutable_retail_interval(
            _cc_catalog.DEFAULT_REFERENCE,
            start=address_value("0x4cc4dc"),
            end_exclusive=address_value("0x4cc4e0"),
        ) != bytes.fromhex("1c 86 0d 00"):
            raise ValueError("reviewed zInterp printf raw storage cell drifted")
        if any(
            body_at(call_site) != bytes.fromhex("ff 15 dc c4 4c 00")
            or body_at(runtime_address_plus(call_site, 6))
            != bytes((0x83, 0xC4, cleanup))
            or any(
                body_at(address) != expected_body
                for address, expected_body in required_bodies.items()
            )
            for call_site, (cleanup, required_bodies) in printf_direct_calls.items()
        ):
            raise ValueError("reviewed zInterp printf direct-memory route drifted")
        printf_containers = tuple(
            container
            for container in indexes.storage_containers
            if container.start < address_value("0x4cc4e0")
            and container.end_exclusive > address_value("0x4cc4dc")
        )
        printf_named_aliases = tuple(
            name for name, candidate_identity in indexes.storage_by_name.items()
            if candidate_identity == "iat:printf"
        )
        if (
            supplied_printf_identity not in {"", "iat:printf"}
            or printf_containers
            or printf_named_aliases
        ):
            raise ValueError(
                "reviewed zInterp printf storage representation is conflicting"
            )
        printf_identity = "iat:printf"
        observed_remainder_register_calls = tuple(
            normalize_address(address)
            for address, row in zip(addresses, retail_instructions)
            if address is not None
            and address_value("0x4c2cb0") <= address < address_value("0x4c53e9")
            and _cc_cfg._instruction_mnemonic(row) == "call"
            and _cc_cfg._instruction_operand(row).strip().lower()
            in {"eax", "ebx", "ecx", "edx", "esi", "edi"}
        )
        expected_remainder_register_calls = tuple(sorted(
            (*remaining_register_calls, *printf_register_calls),
            key=address_value,
        ))
        if observed_remainder_register_calls != expected_remainder_register_calls:
            raise ValueError(
                "reviewed zInterp 99=97+2 slot-register census drifted"
            )
        if any(
            address is not None
            and address_value("0x4c2cb0") <= address < address_value("0x4c53e9")
            and body_at(normalize_address(address)) == bytes.fromhex("ff 15 cc c5 4c 00")
            for address in addresses
        ):
            raise ValueError(
                "reviewed zInterp strncmp remainder gained direct-memory slot call"
            )
    else:
        call_index = next(
            (
                index
                for index, address in enumerate(addresses)
                if address is not None
                and normalize_address(address) == call_address
            ),
            -1,
        )
        if call_index < 0 or _cc_cfg._cleanup_after(retail_instructions, call_index) is not None:
            raise ValueError("reviewed callback gained caller cleanup")
    result = {
        selected_call: ReviewedRegisterCallStorageBridge(
            register=register,
            storage_identity=identity,
            identity_kind="iat" if spec["iat"] else "callback",
        )
        for selected_call in call_addresses
    }
    if start == "0x4c20a0":
        result.update({
            selected_call: ReviewedRegisterCallStorageBridge(
                register="ebx",
                storage_identity=identity,
                identity_kind="iat",
            )
            for selected_call in separate_zinterp_strncmp_calls
        })
        result.update({
            selected_call: ReviewedRegisterCallStorageBridge(
                register=component_register,
                storage_identity=identity,
                identity_kind="iat",
            )
            for _load, component_register, component_calls in (
                remaining_slot_components
            )
            for selected_call, _length, _literal, _target in component_calls
        })
        result.update({
            selected_call: ReviewedRegisterCallStorageBridge(
                register="edi",
                storage_identity=identity,
                identity_kind="iat",
            )
            for selected_call, _length, _literal, _target in early_edi48_calls
        })
        result.update({
            selected_call: ReviewedRegisterCallStorageBridge(
                register="edi",
                storage_identity=printf_identity,
                identity_kind="iat",
            )
            for selected_call in printf_register_calls
        })
    return result


def _reviewed_r3994_dynamic_vptr_dispatch_bridges(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes | None = None,
    bridge: BinaryNinjaBridge | None = None,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Retain exact runtime object/vptr slots without inventing a target."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge

    specs: Mapping[str, tuple[str, tuple[tuple[Any, ...], ...]]] = {
        "0x49fbb0": (
            "0x49fcf0",
            tuple(
                (address, register, slot)
                for address, register, slot in (
                    ("0x49fc0a", "ecx", 0x24), ("0x49fc1a", "ecx", 0x50),
                    ("0x49fc5d", "ecx", 0x3c), ("0x49fc69", "edx", 0x3c),
                    ("0x49fc77", "edx", 0x34), ("0x49fcc7", "edx", 0x30),
                )
            ),
        ),

    }
    start = normalize_address(caller_start)
    selected = specs.get(start)
    if selected is None:
        return {}
    if normalize_address(caller_end_exclusive) != selected[0]:
        raise ValueError("reviewed r3994 dynamic-vptr caller extent drifted")
    runtime = _cc_cfg._instruction_runtime_addresses(
        retail_instructions, source="bn", caller_start=address_value(start)
    )
    by_address = {
        normalize_address(address): instruction
        for address, instruction in zip(runtime, retail_instructions)
        if address is not None
    }
    default_family = "dynamic:zSndPlayHandle-backendBuffer-vptr"
    result: dict[str, ReviewedLoopVptrStorageBridge] = {}
    for row in selected[1]:
        call_address, register, slot = row[:3]
        family = str(row[3]) if len(row) == 4 else default_family
        call = by_address.get(call_address)
        if call is None:
            raise ValueError("reviewed r3994 dynamic-vptr call site is missing")
        expression, displacement = _cc_targets._memory_slot(_cc_cfg._instruction_operand(call))
        if (
            _cc_cfg._instruction_mnemonic(call) != "call"
            or not _cc_cfg._exact_invocation_encoding(call, mnemonic="call")
            or expression not in {register, f"{register}+0x{slot:x}", f"{register}+{slot}"}
            or displacement not in ({None, 0} if slot == 0 else {slot})
        ):
            raise ValueError("reviewed r3994 dynamic-vptr call shape drifted")
        # The immediately reaching vptr definition must be a MOV of the same
        # register from a memory object.  Stop at control flow or another
        # register definition; this intentionally proves no table address.
        call_index = retail_instructions.index(call)
        vptr_loads: list[int] = []
        for index in range(call_index - 1, max(-1, call_index - 33), -1):
            prior = retail_instructions[index]
            mnemonic = _cc_cfg._instruction_mnemonic(prior)
            if mnemonic.startswith("j") or mnemonic in {"loop", "loope", "loopne", "jecxz"}:
                break
            if mnemonic == "call":
                if register in {"eax", "ecx", "edx"}:
                    break
                continue
            if _cc_instructions.may_clobber_register(prior, register):
                operand = _cc_cfg._instruction_operand(prior).split(",", 1)
                if mnemonic == "mov" and len(operand) == 2 and "[" in operand[1]:
                    vptr_loads.append(index)
                break
        if len(vptr_loads) != 1:
            raise ValueError("reviewed r3994 dynamic-vptr load is missing or clobbered")
        result[call_address] = ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=family,
            slot_displacement=slot,
            assembly_source="bn",
        )
    return result
