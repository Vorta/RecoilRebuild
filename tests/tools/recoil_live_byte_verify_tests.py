from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.asm_verify import (  # noqa: E402
    CoffObject,
    CoffRelocation,
    CoffSection,
    CoffSymbol,
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
)
from _recoil.commands.live_byte_verify import (  # noqa: E402
    LiveByteError,
    RetailObjectReader,
    TargetBinding,
    _bindings,
    _compare_row,
    _compare_provider_row,
    _canonicalize_same_comdat_local_label,
    _canonicalize_vc5_local_data_ordinals,
    _candidate_target_identity,
    _matched_group_contract,
    _retail_relocation_catalog,
    _rows,
    _run_fresh_build,
    _select_bindings,
    run,
)
from _recoil.lib.worktree_control import resolve_canonical_control_root  # noqa: E402


def canonical_retail_reference() -> Path:
    resolution = resolve_canonical_control_root(
        executing_worktree_root=REPO_ROOT,
        required_machine_local_paths=("support/Recoil.exe",),
    )
    return resolution.canonical_control_root / "support" / "Recoil.exe"


class FakeDocument:
    def __init__(self, symbols: dict[str, object], prefix: str, revision: int = 42) -> None:
        self.symbols = symbols
        self.prefix = prefix
        self.revision = revision

    def collection(self, name: str) -> dict[str, object]:
        return self.symbols if name == "symbols" else {}

    def pipeline(self, binary: str) -> dict[str, object]:
        return {"authored_order_prefix_end": self.prefix}


class BindingDocument:
    def __init__(self, symbols: dict[str, object], targets: dict[str, object]) -> None:
        self.rows = {"symbols": symbols, "verification_targets": targets}

    def collection(self, name: str) -> dict[str, object]:
        return self.rows.get(name, {})


def authored(address: str, end: str) -> dict[str, object]:
    return {
        "binary": "recoil",
        "kind": "function",
        "address": address,
        "end_exclusive": end,
        "pipeline_class": "authored",
        "authored_order_role": "authored-body",
    }


def provider(address: str, end: str) -> dict[str, object]:
    return {
        "binary": "recoil",
        "kind": "provider-function",
        "address": address,
        "end_exclusive": end,
        "pipeline_class": "non-authored",
    }


class LiveByteVerifyTests(unittest.TestCase):
    def _physical_target_witness_canonicalization(
        self,
        *,
        candidate_symbols: tuple[tuple[str, int], ...] = (("$T75938", 0),),
        relocation_symbols: tuple[str, ...] = ("$T75938", "$T75938"),
        raw_addends: tuple[int, ...] = (0, 0),
        candidate_data: bytes = b"\x00\x00\x00\x3f",
        section_name: str = ".rdata",
        section_characteristics: int = 0x40000040,
        storage_class: int = 3,
        target_type: int = 0,
        catalog_offsets: tuple[int, ...] = (0, 4),
        candidate_site_offsets: tuple[int, ...] | None = None,
        target_row_updates: dict[str, object] | None = None,
        data_relocations: tuple[CoffRelocation, ...] = (),
        pooled_readers: tuple[tuple[str, int, int, int], ...] = (),
        unregistered_reader: bool = False,
        ambiguous_reader: bool = False,
        prove_pooled_readers: bool = True,
        retail_reader_universe: tuple[RetailObjectReader, ...] | None = None,
    ) -> list[tuple[CoffRelocation, object]]:
        source_symbol = "?Source@@YAXXZ"
        if candidate_site_offsets is None:
            candidate_site_offsets = tuple(
                index * 4 for index in range(len(raw_addends))
            )
        text = bytearray(max(candidate_site_offsets, default=0) + 4)
        for offset, value in zip(candidate_site_offsets, raw_addends, strict=True):
            text[offset : offset + 4] = value.to_bytes(4, "little", signed=False)
        sections = [
            CoffSection(
                index=1,
                name=".text",
                raw_data=bytes(text),
                relocation_offset=0,
                relocation_count=len(relocation_symbols),
                characteristics=0x20,
            ),
            CoffSection(
                index=2,
                name=section_name,
                raw_data=candidate_data,
                relocation_offset=0,
                relocation_count=len(data_relocations),
                characteristics=section_characteristics,
            ),
        ]
        symbols = [
            CoffSymbol(
                index=0,
                name=source_symbol,
                value=0,
                section_number=1,
                type=0x20,
                storage_class=2,
                aux_count=0,
            )
        ]
        for index, (name, value) in enumerate(candidate_symbols, start=1):
            symbols.append(
                CoffSymbol(
                    index=index,
                    name=name,
                    value=value,
                    section_number=2,
                    type=target_type,
                    storage_class=storage_class,
                    aux_count=0,
                )
            )
        symbol_indices = {symbol.name: symbol.index for symbol in symbols}
        text_relocations = tuple(
            CoffRelocation(
                offset=candidate_site_offsets[index],
                symbol_index=symbol_indices[name],
                type=IMAGE_REL_I386_DIR32,
                symbol_name=name,
            )
            for index, name in enumerate(relocation_symbols)
        )
        relocations_by_section: dict[int, tuple[CoffRelocation, ...]] = {
            1: text_relocations,
            2: data_relocations,
        }
        for object_symbol, relocation_type, raw_addend, candidate_offset in pooled_readers:
            section_index = len(sections) + 1
            function_symbol = CoffSymbol(
                index=len(symbols),
                name=object_symbol,
                value=0,
                section_number=section_index,
                type=0x20,
                storage_class=2,
                aux_count=0,
            )
            symbols.append(function_symbol)
            sections.append(
                CoffSection(
                    index=section_index,
                    name=".text",
                    raw_data=(
                        b"\x90" * candidate_offset
                        + raw_addend.to_bytes(4, "little", signed=False)
                    ),
                    relocation_offset=0,
                    relocation_count=1,
                    characteristics=0x20,
                )
            )
            relocations_by_section[section_index] = (
                CoffRelocation(
                    offset=candidate_offset,
                    symbol_index=symbol_indices[relocation_symbols[0]],
                    type=relocation_type,
                    symbol_name=relocation_symbols[0],
                ),
            )
            if ambiguous_reader:
                symbols.append(
                    CoffSymbol(
                        index=len(symbols),
                        name=f"{object_symbol}$ambiguous",
                        value=0,
                        section_number=section_index,
                        type=0x20,
                        storage_class=2,
                        aux_count=0,
                    )
                )
        if unregistered_reader:
            section_index = len(sections) + 1
            sections.append(
                CoffSection(
                    index=section_index,
                    name=".text",
                    raw_data=b"\x00\x00\x00\x00",
                    relocation_offset=0,
                    relocation_count=1,
                    characteristics=0x20,
                )
            )
            relocations_by_section[section_index] = (
                CoffRelocation(
                    offset=0,
                    symbol_index=symbol_indices[relocation_symbols[0]],
                    type=IMAGE_REL_I386_DIR32,
                    symbol_name=relocation_symbols[0],
                ),
            )
        coff_object = CoffObject(
            sections=tuple(sections),
            symbols=tuple(symbols),
            symbols_by_index={item.index: item for item in symbols},
            relocations_by_section=relocations_by_section,
        )
        target_id = "recoil:data:0x4cc820"
        binding = {
            "symbol_id": target_id,
            "binary": "recoil",
            "kind": "data",
            "extent_state": "known",
            "output_section_id": "recoil:section:.rdata",
            "ownership_state": "primary-owned",
            "address": "0x4cc820",
            "end_exclusive": "0x4cc824",
            "size": 4,
            "retail_content_hex": "0000003f",
        }
        contract = {
            "kind": "vc5-temporary-static-data",
            "symbol_family": "$T<digits>",
            "storage_class": 3,
            "symbol_type": 0,
            "section_name": ".rdata",
            "requires_initialized_data": True,
            "forbids_uninitialized_data": True,
            "forbids_writable_data": True,
            "one_symbol_for_all_sites": True,
        }
        catalog = [
            {
                "object_symbol": source_symbol,
                "offset": offset,
                "type": IMAGE_REL_I386_DIR32,
                "target_symbol": f"@physical-target:{target_id}",
                "target_symbol_id": target_id,
                "coff_addend": 0,
                "resolved_target_addend": 0,
                "retail_target": 0x4CC820,
                "provenance_mode": "physical-target-unresolved-vc5-temporary",
                "witness_site_offsets": list(catalog_offsets),
                "physical_target_binding": binding,
                "witness_contract": contract,
            }
            for offset in catalog_offsets
        ]
        target_row = {
            key: value
            for key, value in binding.items()
            if key != "retail_content_hex" and key != "symbol_id"
        }
        target_row["binary"] = "recoil"
        target_row["kind"] = "data"
        if target_row_updates:
            target_row.update(target_row_updates)
        with patch(
            "_recoil.commands.live_byte_verify._pe_bytes",
            return_value=b"\x00\x00\x00\x3f",
        ):
            if retail_reader_universe is None:
                reader_rows = [
                    RetailObjectReader(
                        source_symbol_id="recoil:function:0x402250",
                        source_address=0x402250,
                        object_symbol=source_symbol,
                        function_offset=offset,
                        relocation_type=IMAGE_REL_I386_DIR32,
                        coff_addend=0,
                        retail_target=0x4CC820,
                    )
                    for offset in catalog_offsets
                ]
                if prove_pooled_readers:
                    reader_rows.extend(
                        RetailObjectReader(
                            source_symbol_id=f"recoil:function:pooled:{index}",
                            source_address=0x4024A0 + index * 0x10,
                            object_symbol=object_symbol,
                            function_offset=candidate_offset,
                            relocation_type=IMAGE_REL_I386_DIR32,
                            coff_addend=0,
                            retail_target=0x4CC820,
                        )
                        for index, (object_symbol, _, _, candidate_offset) in enumerate(
                            pooled_readers
                        )
                    )
                retail_reader_universe = tuple(reader_rows)
            return _canonicalize_vc5_local_data_ordinals(
                coff_object=coff_object,
                function_bytes=coff_object.function_bytes(source_symbol),
                relocation_catalog=catalog,
                target_rows={target_id: target_row},
                reference=Path("support/Recoil.exe"),
                retail_reader_universes={
                    target_id: retail_reader_universe,
                },
            )

    def test_physical_target_witness_accepts_one_repeated_vc5_temporary(
        self,
    ) -> None:
        canonicalized = self._physical_target_witness_canonicalization()
        self.assertEqual(
            [
                "@physical-target:recoil:data:0x4cc820",
                "@physical-target:recoil:data:0x4cc820",
            ],
            [canonical.symbol_name for _, canonical in canonicalized],
        )
        self.assertEqual(
            ["$T75938", "$T75938"],
            [relocation.symbol_name for relocation, _ in canonicalized],
        )
        self.assertTrue(
            all(
                canonical.compiler_local_ordinal_canonicalized
                for _, canonical in canonicalized
            )
        )
        self.assertEqual(
            {
                "reviewed-physical-target-vc5-temporary-witness",
            },
            {
                canonical.compiler_local_ordinal_reason
                for _, canonical in canonicalized
            },
        )

    def test_physical_target_witness_accepts_one_exact_site(self) -> None:
        canonicalized = self._physical_target_witness_canonicalization(
            relocation_symbols=("$T75938",),
            raw_addends=(0,),
            catalog_offsets=(0,),
            candidate_site_offsets=(0,),
        )
        self.assertEqual(1, len(canonicalized))
        relocation, canonical = canonicalized[0]
        self.assertEqual("$T75938", relocation.symbol_name)
        self.assertEqual(
            "@physical-target:recoil:data:0x4cc820",
            canonical.symbol_name,
        )
        self.assertTrue(canonical.compiler_local_ordinal_canonicalized)
        self.assertEqual(
            "reviewed-physical-target-vc5-temporary-witness",
            canonical.compiler_local_ordinal_reason,
        )

    def test_physical_target_witness_accepts_registered_pooled_reader_subset(
        self,
    ) -> None:
        canonicalized = self._physical_target_witness_canonicalization(
            pooled_readers=(
                (
                    "?OtherRetailReader@AINet@@SIXPAUState@@@Z",
                    IMAGE_REL_I386_DIR32,
                    0,
                    0,
                ),
            ),
        )
        self.assertTrue(
            all(
                canonical.compiler_local_ordinal_canonicalized
                for _, canonical in canonicalized
            )
        )

    def test_physical_target_witness_accepts_unselected_reader_offset_drift(
        self,
    ) -> None:
        selected_symbol = "?Source@@YAXXZ"
        unselected_symbol = (
            "?SolveAltGunLeadTargetPoint@AINet@@"
            "SA_NPAUState@@PAVEntity@@AAVVector3@@2@Z"
        )
        universe = (
            RetailObjectReader(
                source_symbol_id="recoil:function:0x402250",
                source_address=0x402250,
                object_symbol=selected_symbol,
                function_offset=245,
                relocation_type=IMAGE_REL_I386_DIR32,
                coff_addend=0,
                retail_target=0x4CC820,
            ),
            RetailObjectReader(
                source_symbol_id="recoil:function:0x402250",
                source_address=0x402250,
                object_symbol=selected_symbol,
                function_offset=276,
                relocation_type=IMAGE_REL_I386_DIR32,
                coff_addend=0,
                retail_target=0x4CC820,
            ),
            RetailObjectReader(
                source_symbol_id="recoil:function:0x4024a0",
                source_address=0x4024A0,
                object_symbol=unselected_symbol,
                function_offset=523,
                relocation_type=IMAGE_REL_I386_DIR32,
                coff_addend=0,
                retail_target=0x4CC820,
            ),
        )
        canonicalized = self._physical_target_witness_canonicalization(
            catalog_offsets=(245, 276),
            candidate_site_offsets=(245, 276),
            pooled_readers=(
                (unselected_symbol, IMAGE_REL_I386_DIR32, 0, 524),
            ),
            retail_reader_universe=universe,
        )
        self.assertTrue(
            all(
                canonical.compiler_local_ordinal_canonicalized
                for _, canonical in canonicalized
            )
        )

    def test_physical_target_witness_accepts_unselected_reader_multiset(
        self,
    ) -> None:
        selected_symbol = "?Source@@YAXXZ"
        unselected_symbol = "?UnselectedReader@AINet@@SAXXZ"
        universe = (
            RetailObjectReader(
                "recoil:function:0x402250",
                0x402250,
                selected_symbol,
                0,
                IMAGE_REL_I386_DIR32,
                0,
                0x4CC820,
            ),
            RetailObjectReader(
                "recoil:function:0x402250",
                0x402250,
                selected_symbol,
                4,
                IMAGE_REL_I386_DIR32,
                0,
                0x4CC820,
            ),
            RetailObjectReader(
                "recoil:function:0x4024a0",
                0x4024A0,
                unselected_symbol,
                10,
                IMAGE_REL_I386_DIR32,
                0,
                0x4CC820,
            ),
            RetailObjectReader(
                "recoil:function:0x4024a0",
                0x4024A0,
                unselected_symbol,
                20,
                IMAGE_REL_I386_DIR32,
                0,
                0x4CC820,
            ),
        )
        canonicalized = self._physical_target_witness_canonicalization(
            pooled_readers=(
                (unselected_symbol, IMAGE_REL_I386_DIR32, 0, 11),
                (unselected_symbol, IMAGE_REL_I386_DIR32, 0, 21),
            ),
            retail_reader_universe=universe,
        )
        self.assertTrue(
            all(
                canonical.compiler_local_ordinal_canonicalized
                for _, canonical in canonicalized
            )
        )

    def test_physical_target_witness_ignores_absent_registered_unselected_reader(
        self,
    ) -> None:
        selected_symbol = "?Source@@YAXXZ"
        universe = (
            RetailObjectReader(
                "recoil:function:0x402250",
                0x402250,
                selected_symbol,
                0,
                IMAGE_REL_I386_DIR32,
                0,
                0x4CC820,
            ),
            RetailObjectReader(
                "recoil:function:0x402250",
                0x402250,
                selected_symbol,
                4,
                IMAGE_REL_I386_DIR32,
                0,
                0x4CC820,
            ),
            RetailObjectReader(
                "recoil:function:0x402700",
                0x402700,
                "?ReaderOfAnotherPooledTemporary@AINet@@SAXXZ",
                30,
                IMAGE_REL_I386_DIR32,
                0,
                0x4CC820,
            ),
        )
        canonicalized = self._physical_target_witness_canonicalization(
            retail_reader_universe=universe,
        )
        self.assertTrue(
            all(
                canonical.compiler_local_ordinal_canonicalized
                for _, canonical in canonicalized
            )
        )

    def test_physical_target_witness_selected_reader_offset_drift_stays_exact(
        self,
    ) -> None:
        canonicalized = self._physical_target_witness_canonicalization(
            candidate_site_offsets=(0, 5),
        )
        self.assertEqual(
            {"relocation-site-type-population-drift"},
            {
                canonical.compiler_local_ordinal_reason
                for _, canonical in canonicalized
            },
        )

    def test_physical_target_witness_reader_scope_rejection_matrix(self) -> None:
        selected_symbol = "?Source@@YAXXZ"
        unselected_symbol = "?UnselectedReader@AINet@@SAXXZ"

        def reader(
            source_symbol_id: str,
            source_address: int,
            object_symbol: str,
            offset: int,
            *,
            relocation_type: int = IMAGE_REL_I386_DIR32,
            coff_addend: int = 0,
            retail_target: int = 0x4CC820,
        ) -> RetailObjectReader:
            return RetailObjectReader(
                source_symbol_id,
                source_address,
                object_symbol,
                offset,
                relocation_type,
                coff_addend,
                retail_target,
            )

        selected = (
            reader("recoil:function:0x402250", 0x402250, selected_symbol, 0),
            reader("recoil:function:0x402250", 0x402250, selected_symbol, 4),
        )
        registered_unselected = reader(
            "recoil:function:0x4024a0",
            0x4024A0,
            unselected_symbol,
            10,
        )
        cases = (
            (
                {
                    "pooled_readers": (
                        (unselected_symbol, IMAGE_REL_I386_DIR32, 0, 11),
                    ),
                    "ambiguous_reader": True,
                    "retail_reader_universe": selected + (registered_unselected,),
                },
                "physical-witness-reader-function-mapping-is-ambiguous",
            ),
            (
                {
                    "pooled_readers": (
                        (unselected_symbol, IMAGE_REL_I386_DIR32, 0, 11),
                        (unselected_symbol, IMAGE_REL_I386_DIR32, 0, 11),
                    ),
                    "retail_reader_universe": selected + (registered_unselected,),
                },
                "physical-witness-reader-population-has-duplicates",
            ),
            (
                {
                    "pooled_readers": (
                        ("?WrongFunction@AINet@@SAXXZ", IMAGE_REL_I386_DIR32, 0, 11),
                    ),
                    "retail_reader_universe": selected + (registered_unselected,),
                },
                "physical-witness-reader-is-not-retail-proved",
            ),
            (
                {
                    "pooled_readers": (
                        (unselected_symbol, IMAGE_REL_I386_DIR32, 0, 11),
                        ("?ExcessReader@AINet@@SAXXZ", IMAGE_REL_I386_DIR32, 0, 12),
                    ),
                    "retail_reader_universe": selected + (registered_unselected,),
                },
                "physical-witness-reader-is-not-retail-proved",
            ),
            (
                {
                    "pooled_readers": (
                        (unselected_symbol, IMAGE_REL_I386_DIR32, 0, 11),
                    ),
                    "retail_reader_universe": selected
                    + (
                        registered_unselected,
                        reader(
                            "recoil:function:0x4024a0",
                            0x4024A0,
                            unselected_symbol,
                            20,
                        ),
                    ),
                },
                "physical-witness-unselected-reader-population-drift",
            ),
            (
                {
                    "retail_reader_universe": (
                        selected[0],
                    ),
                },
                "physical-witness-reviewed-reader-is-not-retail-proved",
            ),
            (
                {
                    "retail_reader_universe": selected
                    + (
                        reader(
                            "recoil:function:0x402250",
                            0x402250,
                            selected_symbol,
                            8,
                        ),
                    ),
                },
                "physical-witness-selected-retail-reader-population-drift",
            ),
            (
                {
                    "pooled_readers": (
                        (unselected_symbol, IMAGE_REL_I386_DIR32, 0, 11),
                    ),
                    "retail_reader_universe": selected
                    + (
                        reader(
                            "recoil:function:other",
                            0x402700,
                            unselected_symbol,
                            20,
                        ),
                        registered_unselected,
                    ),
                },
                "physical-witness-retail-reader-universe-is-ambiguous",
            ),
            (
                {
                    "retail_reader_universe": (
                        reader(
                            "recoil:function:0x402250",
                            0x402250,
                            selected_symbol,
                            0,
                            retail_target=0x4CC824,
                        ),
                        selected[1],
                    ),
                },
                "physical-witness-retail-reader-target-drift",
            ),
        )
        for options, expected_reason in cases:
            with self.subTest(reason=expected_reason):
                canonicalized = self._physical_target_witness_canonicalization(
                    **options
                )
                self.assertFalse(
                    any(
                        canonical.compiler_local_ordinal_canonicalized
                        for _, canonical in canonicalized
                    )
                )
                self.assertEqual(
                    {expected_reason},
                    {
                        canonical.compiler_local_ordinal_reason
                        for _, canonical in canonicalized
                    },
                )

    def test_physical_target_witness_rejection_matrix(self) -> None:
        cases = (
            (
                {
                    "candidate_symbols": (("_named$S12345", 0),),
                    "relocation_symbols": ("_named$S12345", "_named$S12345"),
                },
                "physical-witness-symbol-family-drift",
            ),
            (
                {
                    "candidate_symbols": (("$T1", 0), ("$T2", 4)),
                    "relocation_symbols": ("$T1", "$T2"),
                    "candidate_data": b"\x00\x00\x00\x3f" * 2,
                },
                "physical-witness-repeated-symbol-drift",
            ),
            (
                {"catalog_offsets": (0,)},
                "relocation-site-type-population-drift",
            ),
            (
                {
                    "candidate_symbols": (("$T75938", 0), ("$T75939", 4)),
                    "relocation_symbols": ("$T75938", "$T75938", "$T75939"),
                    "raw_addends": (0, 0, 0),
                    "candidate_data": b"\x00\x00\x00\x3f" * 2,
                },
                "relocation-site-type-population-drift",
            ),
            (
                {"raw_addends": (1, 0)},
                "physical-witness-site-type-or-addend-drift",
            ),
            (
                {"storage_class": 2},
                "physical-witness-storage-or-type-drift",
            ),
            (
                {"target_type": 0x20},
                "physical-witness-storage-or-type-drift",
            ),
            (
                {"section_name": ".data"},
                "physical-witness-section-name-drift",
            ),
            (
                {"section_characteristics": 0x40000000},
                "physical-witness-section-is-not-initialized-data",
            ),
            (
                {"section_characteristics": 0x400000C0},
                "physical-witness-section-is-not-initialized-data",
            ),
            (
                {"section_characteristics": 0xC0000040},
                "physical-witness-section-is-writable",
            ),
            (
                {"candidate_data": b"\x00\x00\x80\x3f"},
                "physical-witness-candidate-content-drift",
            ),
            (
                {"candidate_data": b"\x00\x00\x00\x3f\x00\x00\x00\x00"},
                "physical-witness-candidate-extent-drift",
            ),
            (
                {"target_row_updates": {"output_section_id": "recoil:section:.data"}},
                "physical-witness-retail-target-binding-drift",
            ),
            (
                {"unregistered_reader": True},
                "physical-witness-reader-function-is-unregistered",
            ),
            (
                {
                    "pooled_readers": (
                        (
                            "?OutsideRetailReader@AINet@@SIXPAUState@@@Z",
                            IMAGE_REL_I386_DIR32,
                            0,
                            0,
                        ),
                    ),
                    "prove_pooled_readers": False,
                },
                "physical-witness-reader-is-not-retail-proved",
            ),
            (
                {
                    "pooled_readers": (
                        (
                            "?WrongTypeReader@AINet@@SIXPAUState@@@Z",
                            IMAGE_REL_I386_REL32,
                            0,
                            0,
                        ),
                    ),
                },
                "physical-witness-reader-type-or-addend-drift",
            ),
            (
                {
                    "pooled_readers": (
                        (
                            "?WrongAddendReader@AINet@@SIXPAUState@@@Z",
                            IMAGE_REL_I386_DIR32,
                            1,
                            0,
                        ),
                    ),
                },
                "physical-witness-reader-type-or-addend-drift",
            ),
            (
                {"retail_reader_universe": ()},
                "physical-witness-retail-reader-universe-is-missing",
            ),
        )
        for options, expected_reason in cases:
            with self.subTest(reason=expected_reason):
                canonicalized = self._physical_target_witness_canonicalization(
                    **options
                )
                self.assertFalse(
                    any(
                        canonical.compiler_local_ordinal_canonicalized
                        for _, canonical in canonicalized
                    )
                )
                self.assertEqual(
                    {expected_reason},
                    {
                        canonical.compiler_local_ordinal_reason
                        for _, canonical in canonicalized
                    },
                )

    def _compiler_local_ordinal_canonicalization(
        self,
        *,
        candidate_symbols: tuple[tuple[str, int], ...] = (
            ("$T75908", 0),
            ("$T75909", 4),
            ("_kPlayerAiPathFollowMinThrottle$S73666", 8),
        ),
        relocation_symbols: tuple[str, ...] = (
            "$T75908",
            "$T75908",
            "$T75909",
            "_kPlayerAiPathFollowMinThrottle$S73666",
        ),
        expected_symbols: tuple[str, ...] = (
            "$T75268",
            "$T75268",
            "$T75269",
            "_kPlayerAiPathFollowMinThrottle$S73095",
        ),
        candidate_data: bytes = (
            b"\x00\x00\x00\x00"
            b"\x00\x00\x80\x3f"
            b"\x00\x00\x80\x3e"
        ),
        retail_data: tuple[bytes, ...] = (
            b"\x00\x00\x00\x00",
            b"\x00\x00\x80\x3f",
            b"\x00\x00\x80\x3e",
        ),
        storage_class: int = 3,
        target_type: int = 0,
        section_name: str = ".rdata",
        include_last_catalog_row: bool = True,
        raw_addends: tuple[int, ...] = (0, 0, 0, 0),
    ) -> list[tuple[CoffRelocation, object]]:
        source_symbol = "?Source@@YAXXZ"
        text = b"".join(
            value.to_bytes(4, "little", signed=False) for value in raw_addends
        )
        sections = (
            CoffSection(
                index=1,
                name=".text",
                raw_data=text,
                relocation_offset=0,
                relocation_count=len(relocation_symbols),
                characteristics=0x20,
            ),
            CoffSection(
                index=2,
                name=section_name,
                raw_data=candidate_data,
                relocation_offset=0,
                relocation_count=0,
                characteristics=0x40,
            ),
        )
        symbols = [
            CoffSymbol(
                index=0,
                name=source_symbol,
                value=0,
                section_number=1,
                type=0x20,
                storage_class=2,
                aux_count=0,
            )
        ]
        for index, (name, value) in enumerate(candidate_symbols, start=1):
            symbols.append(
                CoffSymbol(
                    index=index,
                    name=name,
                    value=value,
                    section_number=2,
                    type=target_type,
                    storage_class=storage_class,
                    aux_count=0,
                )
            )
        symbol_indices = {symbol.name: symbol.index for symbol in symbols}
        relocations = tuple(
            CoffRelocation(
                offset=index * 4,
                symbol_index=symbol_indices[name],
                type=IMAGE_REL_I386_DIR32,
                symbol_name=name,
            )
            for index, name in enumerate(relocation_symbols)
        )
        coff_object = CoffObject(
            sections=sections,
            symbols=tuple(symbols),
            symbols_by_index={item.index: item for item in symbols},
            relocations_by_section={1: relocations, 2: ()},
        )
        target_addresses = (0x4CC810, 0x4CC810, 0x4CC814, 0x4CC818)
        target_ids = (
            "recoil:data:0x4cc810",
            "recoil:data:0x4cc810",
            "recoil:data:0x4cc814",
            "recoil:data:0x4cc818",
        )
        catalog = [
            {
                "offset": index * 4,
                "type": IMAGE_REL_I386_DIR32,
                "target_symbol": expected_symbols[index],
                "target_symbol_id": target_ids[index],
                "coff_addend": 0,
                "resolved_target_addend": 0,
                "retail_target": target_addresses[index],
            }
            for index in range(len(expected_symbols))
        ]
        if not include_last_catalog_row:
            catalog.pop()
        target_contracts = {
            target_id: {
                "binary": "recoil",
                "kind": "data",
                "address": f"0x{address:x}",
                "end_exclusive": f"0x{address + 4:x}",
                "size": 4,
                "extent_state": "known",
                "output_section_id": "recoil:section:.rdata",
                "relocation_target_binding": {
                    "object_symbol": expected_symbol,
                },
            }
            for target_id, address, expected_symbol in (
                (target_ids[0], target_addresses[0], expected_symbols[0]),
                (target_ids[2], target_addresses[2], expected_symbols[2]),
                (target_ids[3], target_addresses[3], expected_symbols[3]),
            )
        }
        retail_by_address = {
            0x4CC810: retail_data[0],
            0x4CC814: retail_data[1],
            0x4CC818: retail_data[2],
        }
        with patch(
            "_recoil.commands.live_byte_verify._pe_bytes",
            side_effect=lambda _path, address, length: retail_by_address[address][:length],
        ):
            return _canonicalize_vc5_local_data_ordinals(
                coff_object=coff_object,
                function_bytes=coff_object.function_bytes(source_symbol),
                relocation_catalog=catalog,
                target_rows=target_contracts,
                reference=Path("support/Recoil.exe"),
            )

    def test_vc5_local_data_ordinals_require_structural_one_to_one_proof(self) -> None:
        canonicalized = self._compiler_local_ordinal_canonicalization()

        self.assertEqual(
            [
                "$T75268",
                "$T75268",
                "$T75269",
                "_kPlayerAiPathFollowMinThrottle$S73095",
            ],
            [canonical.symbol_name for _, canonical in canonicalized],
        )
        self.assertEqual(
            [
                "$T75908",
                "$T75908",
                "$T75909",
                "_kPlayerAiPathFollowMinThrottle$S73666",
            ],
            [relocation.symbol_name for relocation, _ in canonicalized],
        )
        self.assertTrue(
            all(
                canonical.compiler_local_ordinal_canonicalized
                for _, canonical in canonicalized
            )
        )
        self.assertEqual(
            ["recoil:data:0x4cc810", "recoil:data:0x4cc810"],
            [
                canonical.target_symbol_id
                for _, canonical in canonicalized[:2]
            ],
        )
        self.assertEqual(
            [4, 4, 4, 4],
            [canonical.target_extent for _, canonical in canonicalized],
        )

    def test_vc5_local_data_ordinal_canonicalization_fails_closed(self) -> None:
        cases = (
            (
                {
                    "candidate_symbols": (
                        ("$Q75908", 0),
                        ("$T75909", 4),
                        ("_kPlayerAiPathFollowMinThrottle$S73666", 8),
                    ),
                    "relocation_symbols": (
                        "$Q75908",
                        "$Q75908",
                        "$T75909",
                        "_kPlayerAiPathFollowMinThrottle$S73666",
                    ),
                },
                "unsupported-compiler-local-symbol-family",
            ),
            (
                {"storage_class": 2},
                "compiler-local-candidate-storage-or-type-drift",
            ),
            (
                {"target_type": 0x20},
                "compiler-local-candidate-storage-or-type-drift",
            ),
            (
                {"raw_addends": (1, 0, 0, 0)},
                "compiler-local-relocation-addend-drift",
            ),
            (
                {"section_name": ".data"},
                "compiler-local-candidate-section-drift",
            ),
            (
                {
                    "candidate_data": (
                        b"\x01\x00\x00\x00"
                        b"\x00\x00\x80\x3f"
                        b"\x00\x00\x80\x3e"
                    )
                },
                "compiler-local-candidate-content-drift",
            ),
            (
                {
                    "candidate_symbols": (
                        ("$T75908", 0),
                        ("$T75909", 8),
                        ("_kPlayerAiPathFollowMinThrottle$S73666", 12),
                    ),
                    "candidate_data": b"\x00" * 16,
                },
                "compiler-local-candidate-extent-drift",
            ),
            (
                {
                    "relocation_symbols": (
                        "$T75908",
                        "$T75908",
                        "$T75908",
                        "_kPlayerAiPathFollowMinThrottle$S73666",
                    ),
                },
                "compiler-local-repeated-use-or-one-to-one-population-drift",
            ),
            (
                {"include_last_catalog_row": False},
                "relocation-site-type-population-drift",
            ),
        )
        for options, expected_reason in cases:
            with self.subTest(reason=expected_reason):
                canonicalized = self._compiler_local_ordinal_canonicalization(
                    **options
                )
                self.assertFalse(
                    any(
                        canonical.compiler_local_ordinal_canonicalized
                        for _, canonical in canonicalized
                    )
                )
                self.assertEqual(
                    {expected_reason},
                    {
                        canonical.compiler_local_ordinal_reason
                        for _, canonical in canonicalized
                    },
                )

    def test_vc5_local_data_ordinals_reject_duplicate_candidate_storage(self) -> None:
        canonicalized = self._compiler_local_ordinal_canonicalization(
            candidate_symbols=(
                ("$T75908", 0),
                ("$T75909", 0),
                ("_kPlayerAiPathFollowMinThrottle$S73666", 4),
            ),
            candidate_data=b"\x00\x00\x00\x00\x00\x00\x80\x3e",
            retail_data=(
                b"\x00\x00\x00\x00",
                b"\x00\x00\x00\x00",
                b"\x00\x00\x80\x3e",
            ),
        )

        self.assertFalse(
            any(
                canonical.compiler_local_ordinal_canonicalized
                for _, canonical in canonicalized
            )
        )
        self.assertEqual(
            {"compiler-local-duplicate-storage-collision"},
            {
                canonical.compiler_local_ordinal_reason
                for _, canonical in canonicalized
            },
        )

    def test_vc5_local_data_ordinals_reject_mixed_exact_and_drifted_reuse(self) -> None:
        canonicalized = self._compiler_local_ordinal_canonicalization(
            candidate_symbols=(
                ("$T75268", 0),
                ("$T75908", 4),
                ("$T75909", 8),
                ("_kPlayerAiPathFollowMinThrottle$S73666", 12),
            ),
            relocation_symbols=(
                "$T75268",
                "$T75908",
                "$T75909",
                "_kPlayerAiPathFollowMinThrottle$S73666",
            ),
            candidate_data=(
                b"\x00\x00\x00\x00"
                b"\x00\x00\x00\x00"
                b"\x00\x00\x80\x3f"
                b"\x00\x00\x80\x3e"
            ),
        )

        self.assertFalse(
            any(
                canonical.compiler_local_ordinal_canonicalized
                for _, canonical in canonicalized
            )
        )
        self.assertEqual(
            {"compiler-local-repeated-use-or-one-to-one-population-drift"},
            {
                canonical.compiler_local_ordinal_reason
                for _, canonical in canonicalized
            },
        )

    def _coff_local_data_identity_fixture(
        self,
        *,
        target_section: int = 2,
        duplicate_target: bool = False,
    ) -> tuple[CoffObject, str]:
        target_symbol = "$T75268"
        sections = (
            CoffSection(
                index=1,
                name=".text",
                raw_data=b"\x90" * 8,
                relocation_offset=0,
                relocation_count=0,
                characteristics=0x20,
            ),
            CoffSection(
                index=2,
                name=".rdata",
                raw_data=b"\x00" * 0x40,
                relocation_offset=0,
                relocation_count=0,
                characteristics=0x40,
            ),
        )
        symbols = [
            CoffSymbol(
                index=0,
                name="?Source@@YAXXZ",
                value=0,
                section_number=1,
                type=0x20,
                storage_class=2,
                aux_count=0,
            ),
            CoffSymbol(
                index=1,
                name=target_symbol,
                value=0x34,
                section_number=target_section,
                type=0,
                storage_class=3,
                aux_count=0,
            ),
            CoffSymbol(
                index=2,
                name="_PublicAnchor",
                value=0,
                section_number=2,
                type=0,
                storage_class=2,
                aux_count=0,
            ),
        ]
        if duplicate_target:
            symbols.append(
                CoffSymbol(
                    index=3,
                    name=target_symbol,
                    value=0x38,
                    section_number=2,
                    type=0,
                    storage_class=3,
                    aux_count=0,
                )
            )
        return (
            CoffObject(
                sections=sections,
                symbols=tuple(symbols),
                symbols_by_index={item.index: item for item in symbols},
                relocations_by_section={1: (), 2: ()},
            ),
            target_symbol,
        )

    def test_coff_local_data_identity_uses_unique_same_object_section_base(self) -> None:
        coff_object, target_symbol = self._coff_local_data_identity_fixture()
        parsed_map = SimpleNamespace(
            symbols=[
                SimpleNamespace(
                    symbol="_PublicAnchor",
                    address=0x4B6800,
                    object="ai_net.obj",
                )
            ]
        )

        identity = _candidate_target_identity(
            coff_object=coff_object,
            obj_path=Path("build/obj/ai_net.obj"),
            parsed_map=parsed_map,
            symbol_name=target_symbol,
            candidate_target_base=0x4B6834,
        )

        self.assertEqual("same-object-coff-section-placement", identity.source)
        self.assertEqual({0x4B6834}, set(identity.target_bases))
        self.assertEqual({target_symbol}, set(identity.identities))
        self.assertEqual(0x4B6800, identity.section_base)
        self.assertEqual(("_PublicAnchor",), identity.anchor_symbols)

    def test_coff_local_data_identity_fails_closed_for_conflicting_section_bases(self) -> None:
        coff_object, target_symbol = self._coff_local_data_identity_fixture()
        parsed_map = SimpleNamespace(
            symbols=[
                SimpleNamespace(
                    symbol="_PublicAnchor",
                    address=address,
                    object="ai_net.obj",
                )
                for address in (0x4B6800, 0x4B6900)
            ]
        )

        identity = _candidate_target_identity(
            coff_object=coff_object,
            obj_path=Path("ai_net.obj"),
            parsed_map=parsed_map,
            symbol_name=target_symbol,
            candidate_target_base=0x4B6834,
        )

        self.assertEqual("unresolved", identity.source)
        self.assertEqual(
            "same-object-section-anchors-imply-conflicting-bases",
            identity.reason,
        )
        self.assertEqual(set(), set(identity.identities))

    def test_coff_local_data_identity_rejects_cross_object_anchor(self) -> None:
        coff_object, target_symbol = self._coff_local_data_identity_fixture()
        parsed_map = SimpleNamespace(
            symbols=[
                SimpleNamespace(
                    symbol="_PublicAnchor",
                    address=0x4B6800,
                    object="other.obj",
                )
            ]
        )

        identity = _candidate_target_identity(
            coff_object=coff_object,
            obj_path=Path("ai_net.obj"),
            parsed_map=parsed_map,
            symbol_name=target_symbol,
            candidate_target_base=0x4B6834,
        )

        self.assertEqual("unresolved", identity.source)
        self.assertEqual("only-cross-object-section-anchors-were-found", identity.reason)

    def test_coff_local_data_identity_requires_one_defined_exact_target(self) -> None:
        cases = (
            (
                self._coff_local_data_identity_fixture(target_section=0)[0],
                "$T75268",
                "exact-target-symbol-is-undefined-in-coff-object",
            ),
            (
                self._coff_local_data_identity_fixture(duplicate_target=True)[0],
                "$T75268",
                "exact-target-symbol-is-ambiguous-in-coff-object",
            ),
            (
                self._coff_local_data_identity_fixture()[0],
                "$Missing",
                "exact-target-symbol-absent-from-coff-object",
            ),
        )
        for coff_object, target_symbol, expected_reason in cases:
            with self.subTest(reason=expected_reason):
                identity = _candidate_target_identity(
                    coff_object=coff_object,
                    obj_path=Path("ai_net.obj"),
                    parsed_map=SimpleNamespace(symbols=[]),
                    symbol_name=target_symbol,
                    candidate_target_base=0x4B6834,
                )
                self.assertEqual("unresolved", identity.source)
                self.assertEqual(expected_reason, identity.reason)

    def test_coff_local_data_identity_rejects_unanchored_section(self) -> None:
        coff_object, target_symbol = self._coff_local_data_identity_fixture()
        identity = _candidate_target_identity(
            coff_object=coff_object,
            obj_path=Path("ai_net.obj"),
            parsed_map=SimpleNamespace(symbols=[]),
            symbol_name=target_symbol,
            candidate_target_base=0x4B6834,
        )

        self.assertEqual("unresolved", identity.source)
        self.assertEqual("same-object-section-is-unanchored", identity.reason)

    def _local_label_coff_object(
        self,
        *,
        label_value: int = 4,
        label_section: int = 1,
        label_storage_class: int = 6,
        label_type: int = 0,
        comdat: bool = True,
        ambiguous_container: bool = False,
        raw_addend: int = 0,
    ) -> tuple[CoffObject, str]:
        function_symbol = "?Source@@YAXXZ"
        section_characteristics = 0x20 | (0x1000 if comdat else 0)
        sections = [
            CoffSection(
                index=1,
                name=".text",
                raw_data=(raw_addend & 0xFFFFFFFF).to_bytes(4, "little") + b"\x90" * 4,
                relocation_offset=0,
                relocation_count=1,
                characteristics=section_characteristics,
            )
        ]
        if label_section == 2:
            sections.append(
                CoffSection(
                    index=2,
                    name=".text$other",
                    raw_data=b"\x90" * 8,
                    relocation_offset=0,
                    relocation_count=0,
                    characteristics=0x20 | 0x1000,
                )
            )
        symbols = [
            CoffSymbol(
                index=0,
                name=function_symbol,
                value=0,
                section_number=1,
                type=0x20,
                storage_class=2,
                aux_count=0,
            ),
            CoffSymbol(
                index=1,
                name="$L100",
                value=label_value,
                section_number=label_section,
                type=label_type,
                storage_class=label_storage_class,
                aux_count=0,
            ),
        ]
        if ambiguous_container:
            symbols.append(
                CoffSymbol(
                    index=2,
                    name="?Alias@@YAXXZ",
                    value=0,
                    section_number=1,
                    type=0x20,
                    storage_class=2,
                    aux_count=0,
                )
            )
        relocation = CoffRelocation(
            offset=0,
            symbol_index=1,
            type=IMAGE_REL_I386_DIR32,
            symbol_name="$L100",
        )
        return (
            CoffObject(
                sections=tuple(sections),
                symbols=tuple(symbols),
                symbols_by_index={item.index: item for item in symbols},
                relocations_by_section={1: (relocation,)},
            ),
            function_symbol,
        )

    def _compare_authored_local_label(
        self,
        **coff_options: object,
    ) -> dict[str, object]:
        coff_object, function_symbol = self._local_label_coff_object(**coff_options)
        function_bytes = coff_object.function_bytes(function_symbol)
        label = coff_object.symbols_by_index[1]
        raw_addend = int.from_bytes(function_bytes.data[:4], "little", signed=False)
        signed_raw_addend = (
            raw_addend if raw_addend < 0x80000000 else raw_addend - 0x100000000
        )
        target_addend = label.value + signed_raw_addend
        retail_start = 0x401000
        candidate_start = 0x501000
        retail_bytes = (retail_start + target_addend).to_bytes(4, "little") + b"\x90" * 4
        candidate_bytes = (
            candidate_start + target_addend
        ).to_bytes(4, "little") + b"\x90" * 4
        binding = TargetBinding(
            target=SimpleNamespace(name="unit", source_from="unit.cpp"),
            function=SimpleNamespace(symbol=function_symbol),
            source_from="unit.cpp",
        )
        row = {
            "address": f"0x{retail_start:x}",
            "end_exclusive": f"0x{retail_start + 8:x}",
            "symbol_id": "recoil:function:source",
            "scope_ids": ["recoil:function:source"],
        }
        parsed_map = SimpleNamespace(
            symbols=[
                SimpleNamespace(
                    is_function=True,
                    symbol=function_symbol,
                    address=candidate_start,
                )
            ]
        )
        catalog = [
            {
                "object_symbol": function_symbol,
                "offset": 0,
                "type": IMAGE_REL_I386_DIR32,
                "target_symbol": function_symbol,
                "coff_addend": target_addend,
                "resolved_target_addend": target_addend,
                "retail_target": retail_start + target_addend,
            }
        ]
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            obj_path = root / "unit.obj"
            obj_path.write_bytes(b"object")
            reference = root / "retail.exe"
            candidate = root / "candidate.exe"

            def pe_bytes(path: Path, address: int, length: int) -> bytes:
                self.assertEqual(8, length)
                if path == reference:
                    self.assertEqual(retail_start, address)
                    return retail_bytes
                self.assertEqual(candidate, path)
                self.assertEqual(candidate_start, address)
                return candidate_bytes

            with (
                patch("_recoil.commands.live_byte_verify.object_path", return_value=obj_path),
                patch(
                    "_recoil.commands.live_byte_verify.CoffObject.from_path",
                    return_value=coff_object,
                ),
                patch("_recoil.commands.live_byte_verify._pe_bytes", side_effect=pe_bytes),
            ):
                return _compare_row(
                    lane="authored",
                    row=row,
                    binding=binding,
                    config=object(),
                    paths=SimpleNamespace(exe_path=candidate),
                    reference=reference,
                    parsed_map=parsed_map,
                    relocation_catalog=catalog,
                )

    def test_authored_same_comdat_local_label_is_canonicalized(self) -> None:
        result = self._compare_authored_local_label(label_value=3, raw_addend=1)

        self.assertTrue(result["passed"], result)
        relocation = result["relocations"][0]
        self.assertEqual("$L100", relocation["symbolic_target"])
        self.assertEqual("?Source@@YAXXZ", relocation["canonical_symbolic_target"])
        self.assertTrue(relocation["local_label_canonicalized"])
        self.assertEqual("same-comdat-local-label", relocation["local_label_canonicalization_reason"])
        self.assertEqual(1, relocation["raw_coff_addend"])
        self.assertEqual(4, relocation["coff_addend"])
        self.assertEqual(["?Source@@YAXXZ"], relocation["candidate_target_identities"])

    def test_local_label_canonicalization_fails_closed_without_exact_evidence(self) -> None:
        cases = (
            (
                {"ambiguous_container": True},
                "ambiguous-containing-function",
            ),
            ({"label_section": 2}, "cross-section-local-label"),
            ({"label_value": 8}, "local-label-outside-selected-body"),
            ({"label_storage_class": 2}, "not-vc5-local-label"),
            ({"label_type": 0x20}, "not-vc5-local-label"),
            ({"comdat": False}, "selected-section-is-not-comdat"),
            (
                {"label_value": 7, "raw_addend": 1},
                "effective-target-outside-selected-body",
            ),
        )
        for options, expected_reason in cases:
            with self.subTest(reason=expected_reason):
                coff_object, function_symbol = self._local_label_coff_object(**options)
                function_bytes = coff_object.function_bytes(function_symbol)
                relocation = function_bytes.relocations[0]
                raw_addend = int.from_bytes(function_bytes.data[:4], "little")
                canonical = _canonicalize_same_comdat_local_label(
                    coff_object=coff_object,
                    function_bytes=function_bytes,
                    relocation=relocation,
                    raw_addend=raw_addend,
                )
                self.assertFalse(canonical.canonicalized)
                self.assertEqual("$L100", canonical.symbol_name)
                self.assertEqual(raw_addend, canonical.coff_addend)
                self.assertEqual(expected_reason, canonical.reason)

    def _compare_authored_rel32(
        self,
        *,
        raw_addend: int,
        target_addend: int,
        map_target_symbol: str = "?Target@@YAXXZ",
        candidate_tail: int = 0x90,
        object_contribution_tail: bytes = b"",
    ) -> dict[str, object]:
        source_symbol = "?Source@@YAXXZ"
        target_symbol = "?Target@@YAXXZ"
        retail_start = 0x401000
        candidate_start = 0x501000
        retail_target = 0x402000
        candidate_target = 0x502000
        retail_displacement = retail_target - (retail_start + 5)
        candidate_displacement = candidate_target - (candidate_start + 5)
        retail_bytes = (
            b"\xe8"
            + retail_displacement.to_bytes(4, "little", signed=True)
            + b"\x90" * 3
        )
        candidate_bytes = (
            b"\xe8"
            + candidate_displacement.to_bytes(4, "little", signed=True)
            + b"\x90\x90"
            + bytes((candidate_tail,))
        )
        object_bytes = (
            b"\xe8"
            + (raw_addend & 0xFFFFFFFF).to_bytes(4, "little")
            + b"\x90" * 3
        )
        relocation_mask = (False, True, True, True, True, False, False, False)
        body_function_bytes = SimpleNamespace(
            data=object_bytes,
            start=0,
            relocations=(
                CoffRelocation(
                    offset=1,
                    symbol_index=1,
                    type=IMAGE_REL_I386_REL32,
                    symbol_name=target_symbol,
                ),
            ),
            relocation_mask=relocation_mask,
            excluded_tail_relocation_count=0,
        )
        natural_function_bytes = SimpleNamespace(
            data=object_bytes + object_contribution_tail,
            start=0,
            relocations=body_function_bytes.relocations,
            relocation_mask=relocation_mask + (False,) * len(object_contribution_tail),
        )
        binding = TargetBinding(
            target=SimpleNamespace(name="unit", source_from="unit.cpp"),
            function=SimpleNamespace(symbol=source_symbol),
            source_from="unit.cpp",
        )
        row = {
            "address": f"0x{retail_start:x}",
            "end_exclusive": f"0x{retail_start + len(retail_bytes):x}",
            "symbol_id": "recoil:function:source",
            "scope_ids": ["recoil:function:source"],
        }
        parsed_map = SimpleNamespace(
            symbols=[
                SimpleNamespace(
                    is_function=True,
                    symbol=source_symbol,
                    address=candidate_start,
                ),
                SimpleNamespace(
                    is_function=True,
                    symbol=map_target_symbol,
                    address=candidate_target - target_addend,
                ),
            ]
        )
        catalog = [
            {
                "object_symbol": source_symbol,
                "offset": 1,
                "type": IMAGE_REL_I386_REL32,
                "target_symbol": target_symbol,
                "coff_addend": target_addend & 0xFFFFFFFF,
                "resolved_target_addend": target_addend,
                "retail_target": retail_target,
            }
        ]
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            obj_path = root / "unit.obj"
            obj_path.write_bytes(b"object")
            reference = root / "retail.exe"
            candidate = root / "candidate.exe"

            def pe_bytes(path: Path, address: int, length: int) -> bytes:
                self.assertEqual(len(retail_bytes), length)
                if path == reference:
                    self.assertEqual(retail_start, address)
                    return retail_bytes
                self.assertEqual(candidate, path)
                self.assertEqual(candidate_start, address)
                return candidate_bytes

            def extract_function_bytes(
                _symbol: str,
                *,
                byte_length: int | None = None,
            ) -> object:
                if byte_length is None:
                    return natural_function_bytes
                self.assertEqual(len(object_bytes), byte_length)
                return body_function_bytes

            with (
                patch("_recoil.commands.live_byte_verify.object_path", return_value=obj_path),
                patch(
                    "_recoil.commands.live_byte_verify.CoffObject.from_path",
                    return_value=SimpleNamespace(
                        function_bytes=extract_function_bytes,
                        symbols=(),
                    ),
                ),
                patch("_recoil.commands.live_byte_verify._pe_bytes", side_effect=pe_bytes),
            ):
                return _compare_row(
                    lane="authored",
                    row=row,
                    binding=binding,
                    config=object(),
                    paths=SimpleNamespace(exe_path=candidate),
                    reference=reference,
                    parsed_map=parsed_map,
                    relocation_catalog=catalog,
                )

    def test_authored_rel32_accepts_zero_positive_and_negative_raw_coff_addends(
        self,
    ) -> None:
        for addend in (0, 4, -4):
            with self.subTest(addend=addend):
                result = self._compare_authored_rel32(
                    raw_addend=addend,
                    target_addend=addend,
                )
                self.assertTrue(result["passed"], result)
                self.assertTrue(result["linked_body_equal_outside_relocations"])
                relocation = result["relocations"][0]
                self.assertTrue(relocation["passed"])
                self.assertEqual(addend & 0xFFFFFFFF, relocation["coff_addend"])
                self.assertEqual(
                    addend & 0xFFFFFFFF, relocation["expected_coff_addend"]
                )
                self.assertEqual(addend, relocation["resolved_target_addend"])
                self.assertEqual(["?Target@@YAXXZ"], relocation["candidate_target_identities"])
                self.assertEqual("direct-map", relocation["candidate_target_identity_source"])

    def test_authored_rel32_still_requires_symbolic_target_and_linked_body(self) -> None:
        wrong_target = self._compare_authored_rel32(
            raw_addend=4,
            target_addend=4,
            map_target_symbol="?Other@@YAXXZ",
        )
        self.assertFalse(wrong_target["passed"])
        self.assertFalse(wrong_target["relocations"][0]["passed"])

        wrong_body = self._compare_authored_rel32(
            raw_addend=4,
            target_addend=4,
            candidate_tail=0xCC,
        )
        self.assertFalse(wrong_body["passed"])
        self.assertFalse(wrong_body["linked_body_equal_outside_relocations"])
        self.assertTrue(wrong_body["relocations"][0]["passed"])

    def test_authored_longer_contribution_still_requires_relocation_identity(self) -> None:
        result = self._compare_authored_rel32(
            raw_addend=4,
            target_addend=4,
            map_target_symbol="?Other@@YAXXZ",
            object_contribution_tail=b"\x90" * 12,
        )
        self.assertFalse(result["passed"])
        self.assertEqual("linked-body", result["stage"])
        self.assertEqual(8, result["object_body_extent"])
        self.assertEqual(20, result["object_contribution_extent"])
        self.assertEqual(12, result["object_trailing_contribution_extent"])
        self.assertFalse(result["relocations"][0]["passed"])

    def test_object_body_accepts_a_longer_natural_coff_contribution(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            obj_path = root / "sample.obj"
            obj_path.write_bytes(b"object")
            reference = root / "retail.exe"
            binding = TargetBinding(
                target=SimpleNamespace(name="sample", source_from="src/sample.cpp"),
                function=SimpleNamespace(symbol="?Sample@@YAXXZ"),
                source_from="src/sample.cpp",
            )
            row = {
                "address": "0x401000",
                "end_exclusive": "0x401010",
                "symbol_id": "recoil:function:0x401000",
                "scope_ids": ["recoil:function:0x401000"],
            }
            body = b"\xcc" * 16
            natural = SimpleNamespace(
                data=body + b"\x90" * 12,
                relocation_mask=(False,) * 28,
            )
            typed_body = SimpleNamespace(
                data=body,
                relocation_mask=(False,) * 16,
                excluded_tail_relocation_count=0,
            )

            def extract_function_bytes(
                _symbol: str,
                *,
                byte_length: int | None = None,
            ) -> object:
                return natural if byte_length is None else typed_body

            with (
                patch("_recoil.commands.live_byte_verify.object_path", return_value=obj_path),
                patch(
                    "_recoil.commands.live_byte_verify.CoffObject.from_path",
                    return_value=SimpleNamespace(function_bytes=extract_function_bytes),
                ),
                patch("_recoil.commands.live_byte_verify._pe_bytes", return_value=body),
            ):
                result = _compare_row(
                    lane="object",
                    row=row,
                    binding=binding,
                    config=object(),
                    paths=object(),
                    reference=reference,
                    parsed_map=None,
                )

        self.assertTrue(result["passed"], result)
        self.assertEqual("object-body", result["stage"])
        self.assertEqual(16, result["object_body_extent"])
        self.assertEqual(28, result["object_contribution_extent"])
        self.assertEqual(12, result["object_trailing_contribution_extent"])

    def test_object_body_mismatch_still_fails_with_a_longer_contribution(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            obj_path = root / "sample.obj"
            obj_path.write_bytes(b"object")
            binding = TargetBinding(
                target=SimpleNamespace(name="sample", source_from="src/sample.cpp"),
                function=SimpleNamespace(symbol="?Sample@@YAXXZ"),
                source_from="src/sample.cpp",
            )
            row = {
                "address": "0x401000",
                "end_exclusive": "0x401010",
                "symbol_id": "recoil:function:0x401000",
                "scope_ids": ["recoil:function:0x401000"],
            }
            candidate_body = b"\xcc" + b"\x90" * 15
            natural = SimpleNamespace(
                data=candidate_body + b"\x90" * 12,
                relocation_mask=(False,) * 28,
            )
            typed_body = SimpleNamespace(
                data=candidate_body,
                relocation_mask=(False,) * 16,
                excluded_tail_relocation_count=0,
            )

            def extract_function_bytes(
                _symbol: str,
                *,
                byte_length: int | None = None,
            ) -> object:
                return natural if byte_length is None else typed_body

            with (
                patch("_recoil.commands.live_byte_verify.object_path", return_value=obj_path),
                patch(
                    "_recoil.commands.live_byte_verify.CoffObject.from_path",
                    return_value=SimpleNamespace(function_bytes=extract_function_bytes),
                ),
                patch(
                    "_recoil.commands.live_byte_verify._pe_bytes",
                    return_value=b"\x90" * 16,
                ),
            ):
                result = _compare_row(
                    lane="object",
                    row=row,
                    binding=binding,
                    config=object(),
                    paths=object(),
                    reference=root / "retail.exe",
                    parsed_map=None,
                )

        self.assertFalse(result["passed"])
        self.assertEqual("object-body", result["stage"])
        self.assertEqual(0, result["first_difference"]["offset"])
        self.assertEqual(28, result["object_contribution_extent"])

    def test_object_extent_divergence_identifies_the_complete_physical_group(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            obj_path = root / "sample.obj"
            obj_path.write_bytes(b"object")
            binding = TargetBinding(
                target=SimpleNamespace(name="sample", source_from="src/sample.cpp"),
                function=SimpleNamespace(symbol="?Sample@@YAXXZ"),
                source_from="src/sample.cpp",
            )
            row = {
                "address": "0x401000",
                "end_exclusive": "0x401010",
                "symbol_id": "recoil:function:0x401000",
                "scope_ids": [
                    "recoil:function:0x401000",
                    "recoil:function:0x401000:alias",
                ],
            }
            function_bytes = SimpleNamespace(data=b"\x90" * 12, relocation_mask=(False,) * 12)
            with (
                patch("_recoil.commands.live_byte_verify.object_path", return_value=obj_path),
                patch(
                    "_recoil.commands.live_byte_verify.CoffObject.from_path",
                    return_value=SimpleNamespace(
                        function_bytes=lambda _symbol: function_bytes
                    ),
                ),
            ):
                result = _compare_row(
                    lane="object",
                    row=row,
                    binding=binding,
                    config=object(),
                    paths=object(),
                    reference=root / "retail.exe",
                    parsed_map=None,
                )

        self.assertFalse(result["passed"])
        self.assertEqual("object-extent", result["stage"])
        self.assertEqual("0x401000", result["address"])
        self.assertEqual("0x401010", result["end_exclusive"])
        self.assertEqual(row["scope_ids"], result["scope_ids"])
        self.assertEqual("src/sample.cpp", result["source"])
        self.assertEqual("sample", result["target"])
        self.assertEqual("?Sample@@YAXXZ", result["symbol"])
        self.assertEqual(12, result["candidate_extent"])
        self.assertEqual(16, result["retail_extent"])
        self.assertTrue(result["object_path"].endswith("sample.obj"))

    def test_fresh_link_captures_independent_order_diagnostics(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            exe_path = root / "Recoil.exe"
            map_path = root / "Recoil.map"
            summary_path = root / "summary.json"
            paths = SimpleNamespace(
                obj_dir=root / "obj",
                exe_path=exe_path,
                map_path=map_path,
                summary_path=summary_path,
            )
            args = SimpleNamespace(
                lane="authored",
                final_config=Path("tools/_recoil/config/vc5_final_build.json"),
            )
            def complete_build(*_args: object, **_kwargs: object) -> SimpleNamespace:
                exe_path.write_bytes(b"candidate")
                map_path.write_text("map", encoding="ascii")
                summary_path.write_text(
                    json.dumps(
                        {
                            "kind": "final-build",
                            "failure_stage": "linked-order",
                            "compile_succeeded": True,
                            "coff_alias_sources_succeeded": True,
                            "link_succeeded": True,
                            "candidate_available": True,
                            "authored_byte_eligible": True,
                        }
                    ),
                    encoding="utf-8",
                )
                return SimpleNamespace(
                    returncode=1,
                    stdout="independent order divergence\n",
                    stderr="",
                )
            with (
                patch(
                    "_recoil.commands.live_byte_verify.load_config",
                    return_value=SimpleNamespace(sources=()),
                ),
                patch(
                    "_recoil.commands.live_byte_verify.with_explicit_build_dir",
                    return_value=SimpleNamespace(sources=()),
                ),
                patch("_recoil.commands.live_byte_verify.build_paths", return_value=paths),
                patch(
                    "_recoil.commands.live_byte_verify.subprocess.run",
                    side_effect=complete_build,
                ) as run_process,
            ):
                returncode, _, returned_paths = _run_fresh_build(args, root)
            self.assertEqual(1, returncode)
            self.assertIs(paths, returned_paths)
            kwargs = run_process.call_args.kwargs
            self.assertEqual(subprocess.PIPE, kwargs["stdout"])
            self.assertEqual(subprocess.PIPE, kwargs["stderr"])
            self.assertTrue(kwargs["text"])

    def test_fresh_build_rejects_unchanged_stale_artifacts_after_failure(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            obj_path = root / "obj" / "sample.obj"
            obj_path.parent.mkdir(parents=True)
            obj_path.write_bytes(b"stale")
            summary_path = root / "summary.json"
            summary_path.write_text('{"kind":"compile-only-diagnostic","success":false}', encoding="utf-8")
            paths = SimpleNamespace(
                obj_dir=obj_path.parent,
                exe_path=root / "Recoil.exe",
                map_path=root / "Recoil.map",
                summary_path=summary_path,
            )
            config = SimpleNamespace(sources=(Path("sample.cpp"),))
            args = SimpleNamespace(
                lane="object",
                final_config=Path("tools/_recoil/config/vc5_final_build.json"),
            )
            completed = SimpleNamespace(returncode=2, stdout="", stderr="compile failed")
            with (
                patch("_recoil.commands.live_byte_verify.load_config", return_value=config),
                patch("_recoil.commands.live_byte_verify.with_explicit_build_dir", return_value=config),
                patch("_recoil.commands.live_byte_verify.build_paths", return_value=paths),
                patch("_recoil.commands.live_byte_verify.object_path", return_value=obj_path),
                patch("_recoil.commands.live_byte_verify.subprocess.run", return_value=completed),
            ):
                with self.assertRaisesRegex(
                    LiveByteError,
                    "unchanged from pre-build signature",
                ):
                    _run_fresh_build(args, root)

    def test_nonzero_build_requires_linked_order_only_failure_summary(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            exe_path = root / "Recoil.exe"
            map_path = root / "Recoil.map"
            summary_path = root / "summary.json"
            paths = SimpleNamespace(
                obj_dir=root / "obj",
                exe_path=exe_path,
                map_path=map_path,
                summary_path=summary_path,
            )
            config = SimpleNamespace(sources=())
            args = SimpleNamespace(
                lane="authored",
                final_config=Path("tools/_recoil/config/vc5_final_build.json"),
            )

            def incomplete_build(*_args: object, **_kwargs: object) -> SimpleNamespace:
                exe_path.write_bytes(b"candidate")
                map_path.write_text("map", encoding="ascii")
                summary_path.write_text(
                    json.dumps(
                        {
                            "kind": "final-build",
                            "compile_succeeded": True,
                            "coff_alias_sources_succeeded": True,
                            "link_succeeded": True,
                            "candidate_available": True,
                            "authored_byte_eligible": True,
                        }
                    ),
                    encoding="utf-8",
                )
                return SimpleNamespace(returncode=2, stdout="", stderr="driver failure")

            with (
                patch("_recoil.commands.live_byte_verify.load_config", return_value=config),
                patch("_recoil.commands.live_byte_verify.with_explicit_build_dir", return_value=config),
                patch("_recoil.commands.live_byte_verify.build_paths", return_value=paths),
                patch("_recoil.commands.live_byte_verify.subprocess.run", side_effect=incomplete_build),
            ):
                with self.assertRaisesRegex(LiveByteError, "independent linked-order diagnostic"):
                    _run_fresh_build(args, root)

    def test_object_lane_stops_at_current_semantic_prefix_and_groups_aliases(self) -> None:
        document = FakeDocument(
            {
                "recoil:function:a": authored("0x401000", "0x401010"),
                "recoil:function:alias": authored("0x401000", "0x401010"),
                "recoil:function:b": authored("0x401010", "0x401020"),
            },
            "0x401010",
        )
        rows = _rows(document, "object", None)
        self.assertEqual(1, len(rows))
        self.assertEqual(
            ["recoil:function:a", "recoil:function:alias"],
            rows[0]["scope_ids"],
        )
        self.assertEqual(("0x401000", "0x401010"), rows[0]["physical_range"])

    def test_rows_fail_closed_when_live_function_lacks_canonical_address(
        self,
    ) -> None:
        legacy_row = authored("0x401000", "0x401010")
        legacy_row["start"] = legacy_row.pop("address")
        document = FakeDocument(
            {"recoil:function:legacy": legacy_row}, "0x401010"
        )

        with self.assertRaisesRegex(
            LiveByteError,
            "recoil:function:legacy: live byte row lacks its canonical address",
        ):
            _rows(document, "object", None)

    def test_binding_requires_exact_registered_scope_and_preserves_alias_members(self) -> None:
        first = TargetBinding(
            target=type("T", (), {"name": "first"})(),
            function=type("F", (), {"symbol": "?A"})(),
            source_from="src/first.cpp",
        )
        second = TargetBinding(
            target=type("T", (), {"name": "second"})(),
            function=type("F", (), {"symbol": "?B"})(),
            source_from="src/second.cpp",
        )
        row = {"address": "0x401000", "scope_ids": ["a", "b"]}
        self.assertEqual([first, second], _select_bindings({"a": [first], "b": [second]}, row))
        with self.assertRaisesRegex(LiveByteError, "no source-backed byte target"):
            _select_bindings({}, row)
        with self.assertRaisesRegex(LiveByteError, "no source-backed byte target"):
            _select_bindings({"a": [first]}, row)

    def test_bindings_exclude_source_less_linked_only_registration_from_byte_contract(
        self,
    ) -> None:
        symbol_id = "recoil:function:0x401000"
        source_target_id = "recoil:vc5-target:source-backed"
        linked_target_id = "recoil:vc5-target:linked-only"
        source_function = SimpleNamespace(
            address="0x401000",
            symbol="??0CAboutDlg@@QAE@PAVCWnd@@@Z",
            logical_identity_key="",
        )
        linked_function = SimpleNamespace(
            address="0x401000",
            symbol="??0CAboutDlg@@QAE@PAVCWnd@@@Z",
            logical_identity_key="",
        )
        source_target = SimpleNamespace(
            name="source-backed",
            manifest_path=REPO_ROOT / "tools" / "vc5_verify_targets" / "source.json",
            source_from="src/GameZRecoil/about.cpp",
            functions=(source_function,),
            translation_unit_function_order=(),
            linked_function_intervals=(),
        )
        linked_target = SimpleNamespace(
            name="linked-only",
            manifest_path=REPO_ROOT / "tools" / "vc5_verify_targets" / "linked.json",
            source_from="",
            functions=(),
            translation_unit_function_order=(),
            linked_function_intervals=(
                SimpleNamespace(functions=(linked_function,)),
            ),
        )
        document = BindingDocument(
            {
                symbol_id: {
                    "address": "0x401000",
                    "verification_target_ids": [source_target_id, linked_target_id],
                }
            },
            {
                source_target_id: {
                    "kind": "vc5",
                    "name": "source-backed",
                    "registration": {"name": "source-backed"},
                },
                linked_target_id: {
                    "kind": "vc5",
                    "name": "linked-only",
                    "registration": {"name": "linked-only"},
                },
            },
        )
        with patch(
            "_recoil.commands.live_byte_verify.load_manifests",
            return_value=[source_target, linked_target],
        ):
            bindings = _bindings(document, Path("tools/vc5_verify_targets"))

        self.assertEqual([source_target_id], [item.target_id for item in bindings[symbol_id]])
        row = {
            "address": "0x401000",
            "end_exclusive": "0x401020",
            "scope_ids": [symbol_id],
            "physical_rows": [{"symbol_id": symbol_id}],
        }
        selected = _select_bindings(bindings, row)
        self.assertEqual([source_target_id], [item.target_id for item in selected])
        contract = _matched_group_contract(row, bindings)
        self.assertEqual(
            [source_target_id],
            [item["target_id"] for item in contract["target_bindings"]],
        )
        self.assertEqual(
            ["src/GameZRecoil/about.cpp"],
            [item["source_from"] for item in contract["target_bindings"]],
        )

    def test_source_less_only_scope_fails_closed_before_object_lookup(self) -> None:
        source_less = TargetBinding(
            target=SimpleNamespace(name="linked-only", source_from=""),
            function=SimpleNamespace(symbol="??0CAboutDlg@@QAE@PAVCWnd@@@Z"),
            target_id="recoil:vc5-target:linked-only",
            scope_id="recoil:function:0x401000",
        )
        row = {
            "address": "0x401000",
            "end_exclusive": "0x401020",
            "scope_ids": ["recoil:function:0x401000"],
            "physical_rows": [{"symbol_id": "recoil:function:0x401000"}],
        }
        bindings = {"recoil:function:0x401000": [source_less]}

        with self.assertRaisesRegex(LiveByteError, "no source-backed byte target"):
            _select_bindings(bindings, row)
        with self.assertRaisesRegex(LiveByteError, "no source-backed byte target"):
            _matched_group_contract(row, bindings)

    def test_run_reports_ordered_matched_groups_with_aliases_and_revision(self) -> None:
        document = FakeDocument(
            {
                "recoil:function:a": authored("0x401000", "0x401010"),
                "recoil:function:alias": authored("0x401000", "0x401010"),
            },
            "0x401010",
        )
        target = SimpleNamespace(name="target")
        first = TargetBinding(
            target=target,
            function=SimpleNamespace(symbol="?A", logical_identity_key=""),
            target_id="recoil:vc5-target:target",
            scope_id="recoil:function:a",
            source_from="src/target.cpp",
        )
        alias_id = "recoil:logical-function:0x401000:alias"
        alias = TargetBinding(
            target=target,
            function=SimpleNamespace(symbol="?Alias", logical_identity_key=alias_id),
            target_id="recoil:vc5-target:target",
            scope_id="recoil:function:alias",
            source_from="src/target.cpp",
        )
        bindings = {
            "recoil:function:a": [first],
            "recoil:function:alias": [alias],
        }
        args = SimpleNamespace(
            lane="object",
            at=None,
            progress=Path("progress.json"),
            build_root=Path("build/live-validation/object/unit"),
            manifest_dir=Path("tools/vc5_verify_targets"),
            final_config=Path("tools/_recoil/config/vc5_final_build.json"),
            reference=Path("support/Recoil.exe"),
        )
        with (
            patch("_recoil.commands.live_byte_verify.ProgressDocument.load", return_value=document),
            patch("_recoil.commands.live_byte_verify._run_fresh_build", return_value=(0, object(), object())),
            patch("_recoil.commands.live_byte_verify._bindings", return_value=bindings),
            patch("_recoil.commands.live_byte_verify._compare_row", return_value={"passed": True}),
        ):
            report = run(args)
        self.assertTrue(report["passed"])
        self.assertEqual(42, report["tracker_revision"])
        self.assertEqual(1, len(report["matched_groups"]))
        group = report["matched_groups"][0]
        self.assertEqual(
            ["recoil:function:a", "recoil:function:alias"],
            group["scope_ids"],
        )
        self.assertEqual([alias_id], group["logical_alias_ids"])
        self.assertEqual(2, len(group["target_bindings"]))
        self.assertEqual(group, _matched_group_contract(_rows(document, "object", None)[0], bindings))

    def test_retail_relocation_catalog_is_bound_to_exact_object_symbol(self) -> None:
        catalog = [
            {
                "object_symbol": "?Function@@YAXXZ",
                "offset": 1,
                "type": 20,
                "target_symbol": "?Target@@YAXXZ",
                "coff_addend": 0,
                "retail_target": "0x402000",
            }
        ]
        row = {"physical_rows": [{"retail_relocations": catalog}]}
        self.assertEqual(
            catalog,
            _retail_relocation_catalog(row, "?Function@@YAXXZ"),
        )
        self.assertIsNone(_retail_relocation_catalog(row, "?Other@@YAXXZ"))

    def test_bindings_include_all_registered_logical_aliases_and_their_source_owners(self) -> None:
        target_id = "recoil:vc5-target:aliases"
        first_alias = SimpleNamespace(
            address="0x401000",
            symbol="?AliasA",
            logical_identity_key="recoil:logical-function:0x401000:a",
        )
        second_alias = SimpleNamespace(
            address="0x401000",
            symbol="?AliasB",
            logical_identity_key="recoil:logical-function:0x401000:b",
        )
        target = SimpleNamespace(
            name="aliases",
            manifest_path=REPO_ROOT / "tools" / "vc5_verify_targets" / "aliases.json",
            source_from="fallback.cpp",
            functions=(),
            translation_unit_function_order=(
                SimpleNamespace(source_from="first.cpp", functions=(first_alias,)),
                SimpleNamespace(source_from="second.cpp", functions=(second_alias,)),
            ),
            linked_function_intervals=(),
        )
        document = BindingDocument(
            {
                "recoil:function:0x401000": {
                    "address": "0x401000",
                    "verification_target_ids": [target_id],
                }
            },
            {
                target_id: {
                    "kind": "vc5",
                    "name": "aliases",
                    "registration": {"name": "aliases"},
                }
            },
        )
        with patch("_recoil.commands.live_byte_verify.load_manifests", return_value=[target]):
            bindings = _bindings(document, Path("tools/vc5_verify_targets"))
        rows = bindings["recoil:function:0x401000"]
        self.assertEqual(["?AliasA", "?AliasB"], [row.function.symbol for row in rows])
        self.assertEqual(["first.cpp", "second.cpp"], [row.source_from for row in rows])

    def test_linked_provider_row_without_typed_catalog_is_a_typed_divergence(self) -> None:
        document = FakeDocument(
            {"recoil:function:provider": provider("0x401000", "0x401010")},
            "0x401000",
        )
        args = SimpleNamespace(
            lane="linked",
            at=None,
            progress=Path("progress.json"),
            build_root=Path("build/live-validation/linked/unit"),
            manifest_dir=Path("tools/vc5_verify_targets"),
            final_config=Path("tools/_recoil/config/vc5_final_build.json"),
            reference=canonical_retail_reference(),
        )
        paths = SimpleNamespace(map_path=Path("candidate.map"), exe_path=Path("candidate.exe"))
        with (
            patch("_recoil.commands.live_byte_verify.ProgressDocument.load", return_value=document),
            patch("_recoil.commands.live_byte_verify._run_fresh_build", return_value=(0, object(), paths)),
            patch("_recoil.commands.live_byte_verify._bindings", return_value={}),
            patch(
                "_recoil.commands.live_byte_verify.parse_link_map",
                return_value=SimpleNamespace(symbols=[]),
            ),
        ):
            report = run(args)
        self.assertFalse(report["passed"])
        self.assertEqual("missing-provider-binding", report["first_divergence"]["stage"])
        self.assertEqual([], report["matched_groups"])

    def test_typed_provider_binding_checks_exact_map_identity_rva_and_body(self) -> None:
        reference = canonical_retail_reference()
        row = provider("0x401000", "0x401020")
        row.update(
            symbol_id="recoil:function:provider",
            linked_provider_binding={
                "symbol_id": "recoil:function:provider",
                "map_symbol": "?Provider@@YAXXZ",
                "object": "UNIT:unit.obj",
                "provider": "UNIT",
                "archive_member": "unit.obj",
                "operands": [],
            },
        )
        parsed_map = SimpleNamespace(
            symbols=[
                SimpleNamespace(
                    is_function=True,
                    symbol="?Provider@@YAXXZ",
                    object="UNIT:unit.obj",
                    address=0x401000,
                )
            ]
        )
        result = _compare_provider_row(
            row=row,
            paths=SimpleNamespace(exe_path=reference),
            reference=reference,
            parsed_map=parsed_map,
        )
        self.assertTrue(result["passed"], result)
        self.assertTrue(result["exact_linked_address"])
        self.assertTrue(result["exact_linked_bytes"])
        contract = _matched_group_contract(
            {
                "address": row["address"],
                "end_exclusive": row["end_exclusive"],
                "scope_ids": [row["symbol_id"]],
                "physical_rows": [row],
            },
            {},
        )
        self.assertEqual("provider/compiler", contract["target_bindings"][0]["binding_kind"])


if __name__ == "__main__":
    unittest.main()
