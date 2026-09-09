from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass, replace
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import subprocess
import struct
import sys
from typing import Any, Iterable, Mapping, Sequence

from _recoil.commands.asm_verify import (
    CoffObject,
    IMAGE_REL_I386_DIR32,
    IMAGE_SYM_CLASS_EXTERNAL,
    IMAGE_SYM_CLASS_STATIC,
    relocation_size,
    relocation_type_name,
)
from _recoil.commands.relocation_expectations import (
    PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY,
    RelocationExpectationError,
    build_target_identity_state,
    decode_retail_target_sites,
    derive_relocation_expectations,
)
from _recoil.commands.vc5_build import (
    DEFAULT_MANIFEST as DEFAULT_FINAL_CONFIG,
    build_paths,
    load_config,
    object_path,
    parse_link_map,
    with_explicit_build_dir,
)
from _recoil.commands.vc5_verify import (
    DEFAULT_MANIFEST_DIR,
    load_manifests,
    resolve_coff_symbol_regex,
    resolve_function_symbol_for_coff,
)
from _recoil.commands.byte_symbol_selectors import object_selector, resolve_target_definition
from _recoil.lib.progress import (
    DEFAULT_PROGRESS_PATH,
    ProgressDocument,
    address_value,
    normalize_address,
    symbol_authored_order_gate,
)
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path
from _recoil.lib.function_match import MATCH_VERSION, compare_instructions
from _recoil.lib.match_evidence import source_context, review_current


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
DEFAULT_REFERENCE = REPO_ROOT / "support" / "Recoil.exe"
BYTE_MODES = ("object", "authored", "linked")
IMAGE_SCN_LNK_COMDAT = 0x00001000
IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_SCN_MEM_WRITE = 0x80000000
IMAGE_SYM_CLASS_LABEL = 6
IMAGE_SYM_DTYPE_FUNCTION = 0x20


class LiveByteError(RuntimeError):
    pass


@dataclass(frozen=True)
class ArtifactSignature:
    present: bool
    size: int | None
    mtime_ns: int | None


@dataclass(frozen=True)
class TargetBinding:
    target: Any
    function: Any
    target_id: str = ""
    scope_id: str = ""
    source_from: str = ""


@dataclass(frozen=True)
class CanonicalRelocationTarget:
    symbol_name: str
    coff_addend: int
    canonicalized: bool
    reason: str
    compiler_local_ordinal_canonicalized: bool = False
    compiler_local_ordinal_reason: str = "not-applicable"
    target_symbol_id: str = ""
    target_extent: int | None = None
    expected_target_base: int | None = None
    expected_target_bytes: bytes = b""
    registered_symbol_name: str = ""


@dataclass(frozen=True)
class CandidateTargetIdentity:
    target_bases: frozenset[int]
    identities: frozenset[str]
    source: str
    reason: str
    section_base: int | None = None
    anchor_symbols: tuple[str, ...] = ()


@dataclass(frozen=True, order=True)
class RetailObjectReader:
    source_symbol_id: str
    source_address: int
    object_symbol: str
    function_offset: int
    relocation_type: int
    coff_addend: int
    retail_target: int
    object_symbol_regex: str = ""

    @property
    def candidate_key(self) -> tuple[str, int, int, int]:
        return (
            self.object_symbol,
            self.function_offset,
            self.relocation_type,
            self.coff_addend,
        )


def _canonicalize_registered_target_selectors(function_bytes, candidates, catalog, load_definition):
    by_site = {}
    for expected in catalog:
        if expected.get("registered_target_selector") is not None:
            key = (int(expected["offset"]), int(expected["type"]))
            by_site.setdefault(key, []).append(expected)
    result = []
    for relocation, canonical in candidates:
        expected = by_site.get((relocation.offset-function_bytes.start, relocation.type), [])
        if expected:
            try:
                if len(expected) != 1:
                    raise ValueError("ambiguous registered target selector at relocation")
                item = expected[0]
                selector = item["registered_target_selector"]
                definition = load_definition(selector["source_from"])
                name = resolve_target_definition(definition, selector)
                if name != relocation.symbol_name:
                    raise ValueError("relocation does not name the unique registered definition")
                canonical = replace(canonical, symbol_name=item["target_symbol"],
                    registered_symbol_name=name, canonicalized=True,
                    reason="fresh-definition-for-registered-symbol-selector")
            except (OSError, KeyError, TypeError, ValueError) as exc:
                canonical = replace(canonical, reason="registered-symbol-selector: "+str(exc))
        result.append((relocation, canonical))
    return result


def _source_backed_path(value: Any) -> str:
    """Return a usable source path, excluding source-less order registrations."""
    source_from = str(value or "").strip()
    if not source_from or not Path(source_from).name:
        return ""
    return source_from


def _binding_source_from(binding: TargetBinding) -> str:
    return _source_backed_path(
        binding.source_from or getattr(binding.target, "source_from", "")
    )


def _canonicalize_same_comdat_local_label(
    *,
    coff_object: CoffObject,
    function_bytes: Any,
    relocation: Any,
    raw_addend: int,
) -> CanonicalRelocationTarget:
    """Normalize an exact VC5 local-label DIR32 target to its function symbol.

    VC5 may represent ``function + body_offset`` as ``$Lnnnnn + 0`` in one
    function COMDAT.  The retail-derived catalog intentionally records the
    stable physical function symbol plus its body-relative addend.  Normalize
    only when the COFF symbol class, section, COMDAT, selected body range, and
    unique containing function contribution all prove those representations
    equivalent.  Every incomplete or ambiguous case remains unchanged so the
    ordinary exact relocation comparison fails closed.
    """

    def unchanged(reason: str) -> CanonicalRelocationTarget:
        return CanonicalRelocationTarget(
            symbol_name=str(relocation.symbol_name),
            coff_addend=raw_addend,
            canonicalized=False,
            reason=reason,
        )
    if relocation.type != IMAGE_REL_I386_DIR32:
        return unchanged("unsupported-relocation-type")
    target = coff_object.symbols_by_index.get(relocation.symbol_index)
    if target is None or target.name != relocation.symbol_name:
        return unchanged("missing-or-inconsistent-target-symbol")
    if (
        target.storage_class != IMAGE_SYM_CLASS_LABEL
        or target.type != 0
        or not target.name.startswith("$L")
    ):
        return unchanged("not-vc5-local-label")
    if target.section_number != function_bytes.section_index:
        return unchanged("cross-section-local-label")
    section = coff_object.section(function_bytes.section_index)
    if (section.characteristics & IMAGE_SCN_LNK_COMDAT) == 0:
        return unchanged("selected-section-is-not-comdat")
    if not function_bytes.start <= target.value < function_bytes.end:
        return unchanged("local-label-outside-selected-body")

    containers = []
    for symbol in coff_object.symbols:
        if (
            symbol.section_number != function_bytes.section_index
            or symbol.type != IMAGE_SYM_DTYPE_FUNCTION
            or symbol.storage_class
            not in {IMAGE_SYM_CLASS_EXTERNAL, IMAGE_SYM_CLASS_STATIC}
        ):
            continue
        try:
            natural_end = coff_object.function_end(symbol, section)
        except (TypeError, ValueError):
            return unchanged("invalid-containing-contribution")
        if symbol.value <= target.value < natural_end:
            containers.append((symbol, natural_end))
    if len(containers) != 1:
        return unchanged("ambiguous-containing-function")
    container, natural_end = containers[0]
    if (
        container.name != function_bytes.symbol
        or container.value != function_bytes.start
        or natural_end != function_bytes.natural_end
    ):
        return unchanged("containing-function-is-not-selected-contribution")

    signed_raw_addend = (
        raw_addend if raw_addend < 0x80000000 else raw_addend - 0x100000000
    )
    canonical_addend = target.value - container.value + signed_raw_addend
    if not 0 <= canonical_addend < function_bytes.end - function_bytes.start:
        return unchanged("effective-target-outside-selected-body")
    return CanonicalRelocationTarget(
        symbol_name=container.name,
        coff_addend=canonical_addend & 0xFFFFFFFF,
        canonicalized=True,
        reason="same-comdat-local-label",
    )


_VC5_TEMPORARY_DATA_SYMBOL = re.compile(r"^\$T[0-9]+$")
_VC5_NAMED_STATIC_DATA_SYMBOL = re.compile(r"^(.+)\$S[0-9]+$")


def _vc5_compiler_local_family(symbol_name: str) -> tuple[str, str] | None:
    if _VC5_TEMPORARY_DATA_SYMBOL.fullmatch(symbol_name):
        return ("temporary", "$T")
    named_static = _VC5_NAMED_STATIC_DATA_SYMBOL.fullmatch(symbol_name)
    if named_static is not None:
        return ("named-static", named_static.group(1))
    return None


def _static_data_reader_mismatch(
    *, coff_object: CoffObject, target: Any, universe: Sequence[RetailObjectReader],
    reviewed_object_symbol: str, site_offsets: Sequence[int], reviewed_retail_target: int,
    require_all_readers: bool = False,
) -> str | None:
    """Check registered reader identity/counts, optionally requiring every reader."""
    incoming = sorted(
        (
            (int(section_index), relocation)
            for section_index, relocations in coff_object.relocations_by_section.items()
            for relocation in relocations
            if int(relocation.symbol_index) == int(target.index)
        ),
        key=lambda item: (
            item[0],
            int(item[1].offset),
            int(item[1].type),
            int(item[1].symbol_index),
        ),
    )
    universe = tuple(universe)
    if not universe:
        return "physical-witness-retail-reader-universe-is-missing"
    if len(universe) != len(set(universe)):
        return "physical-witness-retail-reader-universe-is-ambiguous"
    # The registered selector is expected identity; the fresh object's spelling
    # is only its witness. Never derive reader sites or populations from it.
    try:
        resolved_universe = tuple(
            replace(reader, object_symbol=resolve_coff_symbol_regex(
                coff_object, reader.object_symbol_regex,
                item_label=reader.source_symbol_id, require_defined_code=True,
            ), object_symbol_regex="") if reader.object_symbol_regex else reader
            for reader in universe
        )
    except (ValueError, re.error):
        return "physical-witness-retail-reader-selector-is-not-unique"
    from _recoil.commands.byte_symbol_selectors import PREFIX
    if reviewed_object_symbol.startswith(PREFIX):
        pattern = reviewed_object_symbol[len(PREFIX):]
        witnesses = {
            resolved.object_symbol
            for registered, resolved in zip(universe, resolved_universe)
            if registered.object_symbol_regex == pattern
        }
        if len(witnesses) != 1:
            return "physical-witness-reviewed-reader-selector-is-not-retail-proved"
        reviewed_object_symbol = next(iter(witnesses))
    # Exact and regex registrations may denote the same retail reader. Merge
    # only after unique resolution and only when every retail site fact agrees.
    universe = tuple(sorted(set(resolved_universe)))
    universe_by_key: dict[
        tuple[str, int, int, int],
        list[RetailObjectReader],
    ] = {}
    for reader in universe:
        universe_by_key.setdefault(reader.candidate_key, []).append(reader)
    if any(len(readers) != 1 for readers in universe_by_key.values()):
        return "physical-witness-retail-reader-universe-is-ambiguous"

    reviewed_reader_keys = {
        (
            reviewed_object_symbol,
            offset,
            IMAGE_REL_I386_DIR32,
            0,
        )
        for offset in site_offsets
    }
    if not reviewed_reader_keys.issubset(universe_by_key):
        return "physical-witness-reviewed-reader-is-not-retail-proved"

    if any(reader.retail_target != reviewed_retail_target for reader in universe):
        return "physical-witness-retail-reader-target-drift"

    registration_identities_by_object_symbol: dict[
        str,
        set[tuple[str, int]],
    ] = {}
    for reader in universe:
        registration_identities_by_object_symbol.setdefault(
            reader.object_symbol,
            set(),
        ).add((reader.source_symbol_id, reader.source_address))
    if any(
        len(identities) != 1
        for identities in registration_identities_by_object_symbol.values()
    ):
        return "physical-witness-retail-reader-universe-is-ambiguous"

    retail_selected_reader_keys = Counter(
        reader.candidate_key
        for reader in universe
        if reader.object_symbol == reviewed_object_symbol
    )
    reviewed_reader_population = Counter(reviewed_reader_keys)
    if retail_selected_reader_keys != reviewed_reader_population:
        return "physical-witness-selected-retail-reader-population-drift"
    retail_unselected_reader_populations: dict[
        str,
        Counter[tuple[int, int]],
    ] = {}
    for reader in universe:
        if reader.object_symbol == reviewed_object_symbol:
            continue
        retail_unselected_reader_populations.setdefault(
            reader.object_symbol,
            Counter(),
        )[(reader.relocation_type, reader.coff_addend)] += 1

    candidate_reader_keys: list[tuple[str, int, int, int]] = []
    for section_index, incoming_relocation in incoming:
        if incoming_relocation.type != IMAGE_REL_I386_DIR32:
            return "physical-witness-reader-type-or-addend-drift"
        try:
            source_section = coff_object.section(section_index)
        except ValueError:
            return "physical-witness-reader-section-is-unregistered"
        source_offset = int(incoming_relocation.offset)
        if source_offset < 0 or source_offset + 4 > len(source_section.raw_data):
            return "physical-witness-reader-offset-is-invalid"
        source_addend = int.from_bytes(
            source_section.raw_data[source_offset : source_offset + 4],
            "little",
            signed=False,
        )
        if source_addend != 0:
            return "physical-witness-reader-type-or-addend-drift"
        containing_functions = []
        for symbol in coff_object.symbols:
            if (
                symbol.section_number != section_index
                or symbol.type != IMAGE_SYM_DTYPE_FUNCTION
                or symbol.storage_class
                not in {IMAGE_SYM_CLASS_EXTERNAL, IMAGE_SYM_CLASS_STATIC}
            ):
                continue
            try:
                natural_end = coff_object.function_end(symbol, source_section)
            except (TypeError, ValueError):
                return "physical-witness-reader-function-extent-is-invalid"
            if symbol.value <= source_offset < natural_end:
                containing_functions.append(symbol)
        if not containing_functions:
            return "physical-witness-reader-function-is-unregistered"
        if len(containing_functions) != 1:
            return "physical-witness-reader-function-mapping-is-ambiguous"
        source_function = containing_functions[0]
        candidate_reader_keys.append(
            (
                str(source_function.name),
                source_offset - int(source_function.value),
                int(incoming_relocation.type),
                source_addend,
            )
        )
    if not candidate_reader_keys:
        return "physical-witness-object-wide-incoming-population-drift"
    if len(candidate_reader_keys) != len(set(candidate_reader_keys)):
        return "physical-witness-reader-population-has-duplicates"
    candidate_selected_reader_population = Counter(
        key for key in candidate_reader_keys if key[0] == reviewed_object_symbol
    )
    if not reviewed_reader_population <= candidate_selected_reader_population:
        return "physical-witness-reviewed-reader-is-missing"
    if candidate_selected_reader_population != reviewed_reader_population:
        return "physical-witness-selected-reader-offset-or-population-drift"

    candidate_unselected_reader_populations: dict[
        str,
        Counter[tuple[int, int]],
    ] = {}
    for object_symbol, _, relocation_type, coff_addend in candidate_reader_keys:
        if object_symbol == reviewed_object_symbol:
            continue
        candidate_unselected_reader_populations.setdefault(
            object_symbol,
            Counter(),
        )[(relocation_type, coff_addend)] += 1
    for (
        object_symbol,
        candidate_population,
    ) in candidate_unselected_reader_populations.items():
        retail_population = retail_unselected_reader_populations.get(object_symbol)
        if retail_population is None or not candidate_population <= retail_population:
            return "physical-witness-reader-is-not-retail-proved"
        if candidate_population != retail_population:
            return "physical-witness-unselected-reader-population-drift"
    if require_all_readers and candidate_unselected_reader_populations != retail_unselected_reader_populations:
        return "physical-witness-incomplete-retail-reader-population"
    return None


def _canonicalize_native_relocation_roles(coff_object, function_bytes, candidates, relocation_catalog):
    from _recoil.commands.native_eh_relocations import HANDLER, prove_object_handler
    from _recoil.commands.native_array_cleanup import ROLE, prove_object_cleanup

    expected_sites = {}
    for expected in relocation_catalog:
        key = (int(expected["offset"]), int(expected["type"]))
        if key in expected_sites:
            return candidates
        expected_sites[key] = expected
    result = []
    for relocation, canonical in candidates:
        expected = expected_sites.get((relocation.offset-function_bytes.start, relocation.type), {})
        if expected.get("native_eh_role") == HANDLER:
            try:
                prove_object_handler(coff_object, function_bytes, relocation, expected)
            except (ValueError, KeyError, TypeError) as exc:
                canonical = replace(canonical, reason="native-eh-role-rejected: "+str(exc))
            else:
                canonical = replace(canonical, symbol_name=expected["target_symbol"],
                                    canonicalized=True, reason=HANDLER)
        elif expected.get("native_cleanup"):
            try:
                prove_object_cleanup(coff_object, function_bytes, relocation, expected)
            except (ValueError, KeyError, TypeError) as exc:
                canonical = replace(canonical, reason="native-cleanup-rejected: "+str(exc))
            else:
                canonical = replace(canonical, symbol_name=expected["target_symbol"], canonicalized=True, reason=ROLE)
        result.append((relocation, canonical))
    return result


def _canonicalize_vc5_local_data_ordinals(
    *,
    coff_object: CoffObject,
    function_bytes: Any,
    relocation_catalog: Sequence[Mapping[str, Any]],
    target_rows: Mapping[str, Any] | None,
    reference: Path,
    retail_reader_universes: Mapping[str, Sequence[RetailObjectReader]] | None = None,
) -> list[tuple[Any, CanonicalRelocationTarget]]:
    """Canonicalize proven VC5 compiler-local data ordinal drift.

    The retail relocation catalog supplies every expected site, type, addend,
    stable symbol family, target id, and retail target.  Candidate COFF is used
    only to prove that one raw STATIC/type-0 symbol occupies one exact ``.rdata``
    extent whose bytes equal the immutable retail extent.  Canonicalization is
    all-or-nothing across the mismatched compiler-local population, including
    repeated-use grouping and a one-to-one raw/canonical symbol mapping.
    """

    candidates: list[tuple[Any, CanonicalRelocationTarget]] = []
    raw_addends: dict[tuple[int, int], int] = {}
    candidate_by_site: dict[tuple[int, int], int] = {}
    duplicate_candidate_site = False
    for relocation in function_bytes.relocations:
        size = relocation_size(relocation.type)
        relative = relocation.offset - function_bytes.start
        raw_addend = int.from_bytes(
            function_bytes.data[relative : relative + size],
            "little",
            signed=False,
        )
        candidate_by_site[(relative, relocation.type)] = (
            candidate_by_site.get((relative, relocation.type), 0) + 1
        )
        duplicate_candidate_site |= candidate_by_site[(relative, relocation.type)] != 1
        raw_addends[(relative, relocation.type)] = raw_addend
        candidates.append(
            (
                relocation,
                _canonicalize_same_comdat_local_label(
                    coff_object=coff_object,
                    function_bytes=function_bytes,
                    relocation=relocation,
                    raw_addend=raw_addend,
                ),
            )
        )

    expected_by_site: dict[tuple[int, int], Mapping[str, Any]] = {}
    duplicate_expected_site = False
    for expected in relocation_catalog:
        site = (
            _integer(expected.get("offset"), field="relocation offset"),
            _integer(expected.get("type"), field="relocation type"),
        )
        duplicate_expected_site |= site in expected_by_site
        expected_by_site[site] = expected

    def reject(reason: str) -> list[tuple[Any, CanonicalRelocationTarget]]:
        return [
            (
                relocation,
                replace(
                    canonical,
                    compiler_local_ordinal_reason=reason,
                ),
            )
            for relocation, canonical in candidates
        ]

    candidate_sites = set(candidate_by_site)
    if (
        duplicate_candidate_site
        or duplicate_expected_site
        or len(candidates) != len(relocation_catalog)
        or candidate_sites != set(expected_by_site)
    ):
        return reject("relocation-site-type-population-drift")

    candidates = _canonicalize_native_relocation_roles(
        coff_object, function_bytes, candidates, relocation_catalog)

    physical_candidate_storage_keys: set[tuple[int, int]] = set()
    physical_expected_storage_bases: set[int] = set()
    physical_raw_symbols: set[str] = set()
    physical_groups: dict[
        tuple[str, tuple[int, ...]],
        list[tuple[int, Any, CanonicalRelocationTarget, Mapping[str, Any]]],
    ] = {}
    for index, (relocation, canonical) in enumerate(candidates):
        relative = relocation.offset - function_bytes.start
        expected = expected_by_site[(relative, relocation.type)]
        if (
            expected.get("provenance_mode")
            != PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY
        ):
            continue
        raw_offsets = expected.get("witness_site_offsets")
        if (
            not isinstance(raw_offsets, list)
            or any(not isinstance(item, int) or isinstance(item, bool) for item in raw_offsets)
        ):
            return reject("physical-witness-site-contract-invalid")
        site_offsets = tuple(sorted(set(raw_offsets)))
        if len(site_offsets) != len(raw_offsets) or not site_offsets:
            return reject("physical-witness-site-contract-invalid")
        target_symbol_id = str(expected.get("target_symbol_id", ""))
        if not target_symbol_id:
            return reject("physical-witness-target-id-missing")
        physical_groups.setdefault((target_symbol_id, site_offsets), []).append(
            (index, relocation, canonical, expected)
        )

    for (target_symbol_id, site_offsets), group in physical_groups.items():
        group_sites = tuple(
            sorted(relocation.offset - function_bytes.start for _, relocation, _, _ in group)
        )
        if group_sites != site_offsets:
            return reject("physical-witness-site-type-population-drift")
        expected_token = f"@physical-target:{target_symbol_id}"
        raw_symbols: set[str] = set()
        expected_contract: Mapping[str, Any] | None = None
        expected_binding: Mapping[str, Any] | None = None
        for _, relocation, canonical, expected in group:
            relative = relocation.offset - function_bytes.start
            if (
                relocation.type != IMAGE_REL_I386_DIR32
                or canonical.canonicalized
                or str(expected.get("target_symbol", "")) != expected_token
                or _integer(expected.get("coff_addend"), field="COFF relocation addend")
                != 0
                or _integer(
                    expected.get("resolved_target_addend", 0),
                    field="resolved target addend",
                )
                != 0
                or raw_addends[(relative, relocation.type)] != 0
            ):
                return reject("physical-witness-site-type-or-addend-drift")
            raw_symbol = str(relocation.symbol_name)
            if _VC5_TEMPORARY_DATA_SYMBOL.fullmatch(raw_symbol) is None:
                return reject("physical-witness-symbol-family-drift")
            raw_symbols.add(raw_symbol)
            contract = expected.get("witness_contract")
            binding = expected.get("physical_target_binding")
            if not isinstance(contract, Mapping) or not isinstance(binding, Mapping):
                return reject("physical-witness-reviewed-context-missing")
            if expected_contract is None:
                expected_contract = contract
                expected_binding = binding
            elif dict(contract) != dict(expected_contract) or dict(binding) != dict(
                expected_binding
            ):
                return reject("physical-witness-reviewed-context-drift")
        if len(raw_symbols) != 1:
            return reject("physical-witness-repeated-symbol-drift")
        raw_symbol = next(iter(raw_symbols))
        if raw_symbol in physical_raw_symbols:
            return reject("physical-witness-bijection-drift")
        physical_raw_symbols.add(raw_symbol)
        raw_symbol_sites = {
            relocation.offset - function_bytes.start
            for relocation, _ in candidates
            if str(relocation.symbol_name) == raw_symbol
        }
        if raw_symbol_sites != set(site_offsets):
            return reject("physical-witness-repeated-symbol-drift")

        assert expected_contract is not None
        fixed_contract = {
            "kind": "vc5-temporary-static-data",
            "symbol_family": "$T<digits>",
            "storage_class": IMAGE_SYM_CLASS_STATIC,
            "symbol_type": 0,
            "section_name": ".rdata",
            "requires_initialized_data": True,
            "forbids_uninitialized_data": True,
            "forbids_writable_data": True,
            "one_symbol_for_all_sites": True,
        }
        if dict(expected_contract) != fixed_contract:
            return reject("physical-witness-contract-drift")
        exact_candidate_symbols = [
            symbol for symbol in coff_object.symbols if symbol.name == raw_symbol
        ]
        if len(exact_candidate_symbols) != 1:
            return reject("physical-witness-candidate-symbol-is-not-unique")
        target = exact_candidate_symbols[0]
        if any(
            coff_object.symbols_by_index.get(relocation.symbol_index) != target
            for _, relocation, _, _ in group
        ):
            return reject("physical-witness-relocation-symbol-binding-drift")
        reviewed_symbols = {str(expected.get("object_symbol", "")) for _, _, _, expected in group}
        reviewed_targets = {_integer(expected.get("retail_target"), field="retail target") for _, _, _, expected in group}
        if len(reviewed_symbols) != 1 or not next(iter(reviewed_symbols), "") or len(reviewed_targets) != 1:
            return reject("physical-witness-reviewed-reader-context-drift")
        reader_failure = _static_data_reader_mismatch(
            coff_object=coff_object, target=target,
            universe=(retail_reader_universes or {}).get(target_symbol_id, ()),
            reviewed_object_symbol=next(iter(reviewed_symbols)), site_offsets=site_offsets,
            reviewed_retail_target=next(iter(reviewed_targets)),
        )
        if reader_failure is not None:
            return reject(reader_failure)
        if target.storage_class != IMAGE_SYM_CLASS_STATIC or target.type != 0:
            return reject("physical-witness-storage-or-type-drift")
        if target.section_number <= 0:
            return reject("physical-witness-symbol-is-undefined")
        section = coff_object.section(target.section_number)
        if section.name != ".rdata":
            return reject("physical-witness-section-name-drift")
        if (
            (section.characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) == 0
            or (section.characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) != 0
        ):
            return reject("physical-witness-section-is-not-initialized-data")
        if (section.characteristics & IMAGE_SCN_MEM_WRITE) != 0:
            return reject("physical-witness-section-is-writable")

        assert expected_binding is not None
        target_row = (
            target_rows.get(target_symbol_id) if target_rows is not None else None
        )
        if not isinstance(target_row, Mapping):
            return reject("physical-witness-retail-target-row-is-missing")
        current_binding = {
            "symbol_id": target_symbol_id,
            "binary": target_row.get("binary"),
            "kind": target_row.get("kind"),
            "address": target_row.get("address"),
            "end_exclusive": target_row.get("end_exclusive"),
            "size": target_row.get("size"),
            "extent_state": target_row.get("extent_state"),
            "output_section_id": target_row.get("output_section_id"),
            "ownership_state": target_row.get("ownership_state"),
            "retail_content_hex": expected_binding.get("retail_content_hex"),
        }
        if current_binding != dict(expected_binding):
            return reject("physical-witness-retail-target-binding-drift")
        try:
            expected_target_base = address_value(str(expected_binding["address"]))
            expected_target_end = address_value(
                str(expected_binding["end_exclusive"])
            )
            expected_target_bytes = bytes.fromhex(
                str(expected_binding["retail_content_hex"])
            )
        except (KeyError, TypeError, ValueError):
            return reject("physical-witness-retail-target-contract-invalid")
        target_extent = expected_target_end - expected_target_base
        if (
            target_extent <= 0
            or target_extent != int(expected_binding.get("size", -1))
            or len(expected_target_bytes) != target_extent
            or any(
                _integer(expected.get("retail_target"), field="retail resolved target")
                != expected_target_base
                for _, _, _, expected in group
            )
            or _pe_bytes(
                reference,
                expected_target_base,
                target_extent,
            )
            != expected_target_bytes
        ):
            return reject("physical-witness-retail-target-contract-drift")
        try:
            candidate_end = coff_object.symbol_end(target, section)
        except (TypeError, ValueError):
            return reject("physical-witness-candidate-extent-is-invalid")
        if (
            target.value < 0
            or candidate_end - target.value != target_extent
            or candidate_end > len(section.raw_data)
        ):
            return reject("physical-witness-candidate-extent-drift")
        if any(
            target.value <= item.offset < candidate_end
            for item in coff_object.relocations_by_section.get(section.index, ())
        ):
            return reject("physical-witness-candidate-data-is-not-immutable")
        if section.raw_data[target.value:candidate_end] != expected_target_bytes:
            return reject("physical-witness-candidate-content-drift")
        candidate_storage_key = (target.section_number, target.value)
        if (
            candidate_storage_key in physical_candidate_storage_keys
            or expected_target_base in physical_expected_storage_bases
        ):
            return reject("physical-witness-duplicate-storage-collision")
        physical_candidate_storage_keys.add(candidate_storage_key)
        physical_expected_storage_bases.add(expected_target_base)
        for index, _relocation, canonical, _expected in group:
            candidates[index] = (
                candidates[index][0],
                replace(
                    canonical,
                    symbol_name=expected_token,
                    canonicalized=True,
                    reason="reviewed-physical-target-unresolved-provenance",
                    compiler_local_ordinal_canonicalized=True,
                    compiler_local_ordinal_reason=(
                        "reviewed-physical-target-vc5-temporary-witness"
                    ),
                    target_symbol_id=target_symbol_id,
                    target_extent=target_extent,
                    expected_target_base=expected_target_base,
                    expected_target_bytes=expected_target_bytes,
                ),
            )

    proposals: list[
        tuple[int, Any, CanonicalRelocationTarget, Mapping[str, Any], str, str]
    ] = []
    compiler_local_population: list[tuple[str, str]] = []
    for index, (relocation, canonical) in enumerate(candidates):
        relative = relocation.offset - function_bytes.start
        expected = expected_by_site[(relative, relocation.type)]
        expected_symbol = str(expected.get("target_symbol", ""))
        # Same-COMDAT label normalization is a separate exact source-symbol
        # equivalence and must not be reinterpreted as data ordinal drift.
        if canonical.canonicalized:
            continue
        raw_symbol = str(relocation.symbol_name)
        candidate_family = _vc5_compiler_local_family(raw_symbol)
        expected_family = _vc5_compiler_local_family(expected_symbol)
        if expected_family is None and candidate_family == ("named-static", expected_symbol):
            # A reviewed source stem is stable; the compiler ordinal is not identity.
            # The complete data and reader proof below remains mandatory.
            expected_family = candidate_family
        if canonical.symbol_name == expected_symbol:
            if candidate_family is not None or expected_family is not None:
                if (
                    candidate_family is None
                    or expected_family is None
                    or candidate_family != expected_family
                ):
                    return reject("compiler-local-symbol-family-drift")
                compiler_local_population.append((raw_symbol, expected_symbol))
            continue
        if candidate_family is None or expected_family is None:
            return reject("unsupported-compiler-local-symbol-family")
        if candidate_family != expected_family:
            return reject("compiler-local-symbol-family-drift")
        expected_addend = _integer(
            expected.get("coff_addend"), field="COFF relocation addend"
        )
        if raw_addends[(relative, relocation.type)] != expected_addend:
            return reject("compiler-local-relocation-addend-drift")
        compiler_local_population.append((raw_symbol, expected_symbol))
        proposals.append(
            (
                index,
                relocation,
                canonical,
                expected,
                raw_symbol,
                expected_symbol,
            )
        )

    if not proposals:
        return candidates
    if target_rows is None:
        return reject("compiler-local-target-rows-not-supplied")

    raw_to_expected: dict[str, set[str]] = {}
    expected_to_raw: dict[str, set[str]] = {}
    for raw_symbol, expected_symbol in compiler_local_population:
        raw_to_expected.setdefault(raw_symbol, set()).add(expected_symbol)
        expected_to_raw.setdefault(expected_symbol, set()).add(raw_symbol)
    if (
        any(len(values) != 1 for values in raw_to_expected.values())
        or any(len(values) != 1 for values in expected_to_raw.values())
        or len(raw_to_expected) != len(expected_to_raw)
    ):
        return reject("compiler-local-repeated-use-or-one-to-one-population-drift")

    pair_contracts: dict[
        tuple[str, str], tuple[str, int, int, bytes, int, int]
    ] = {}
    candidate_storage_keys: set[tuple[int, int]] = set(
        physical_candidate_storage_keys
    )
    expected_storage_bases: set[int] = set(physical_expected_storage_bases)
    for _, relocation, _, expected, raw_symbol, expected_symbol in proposals:
        pair = (raw_symbol, expected_symbol)
        if pair in pair_contracts:
            continue
        exact_candidate_symbols = [
            symbol for symbol in coff_object.symbols if symbol.name == raw_symbol
        ]
        if len(exact_candidate_symbols) != 1:
            return reject("compiler-local-candidate-symbol-is-not-unique")
        target = exact_candidate_symbols[0]
        if coff_object.symbols_by_index.get(relocation.symbol_index) != target:
            return reject("compiler-local-relocation-target-symbol-is-inconsistent")
        if target.storage_class != IMAGE_SYM_CLASS_STATIC or target.type != 0:
            return reject("compiler-local-candidate-storage-or-type-drift")
        if target.section_number <= 0:
            return reject("compiler-local-candidate-symbol-is-undefined")
        section = coff_object.section(target.section_number)
        if section.name != ".rdata":
            return reject("compiler-local-candidate-section-drift")

        target_symbol_id = str(expected.get("target_symbol_id", ""))
        target_row = target_rows.get(target_symbol_id)
        if not target_symbol_id or not isinstance(target_row, Mapping):
            return reject("compiler-local-retail-target-row-is-missing")
        if (
            target_row.get("binary") != "recoil"
            or target_row.get("kind") != "data"
            or target_row.get("extent_state") != "known"
            or target_row.get("output_section_id") != "recoil:section:.rdata"
        ):
            return reject("compiler-local-retail-target-contract-drift")
        try:
            expected_target_base = address_value(str(target_row["address"]))
            expected_target_end = address_value(str(target_row["end_exclusive"]))
        except (KeyError, TypeError, ValueError):
            return reject("compiler-local-retail-target-extent-is-invalid")
        target_extent = expected_target_end - expected_target_base
        if target_extent <= 0:
            return reject("compiler-local-retail-target-extent-is-invalid")
        if target_row.get("size") not in {None, target_extent}:
            return reject("compiler-local-retail-target-size-drift")
        resolved_target_addend = _integer(
            expected.get("resolved_target_addend", 0),
            field="resolved target addend",
        )
        expected_retail_target = _integer(
            expected.get("retail_target"), field="retail resolved target"
        )
        if expected_retail_target - resolved_target_addend != expected_target_base:
            return reject("compiler-local-retail-target-address-drift")
        binding = target_row.get("relocation_target_binding")
        if (
            not isinstance(binding, Mapping)
            or binding.get("object_symbol") != expected_symbol
        ):
            return reject("compiler-local-retail-object-symbol-binding-drift")

        try:
            candidate_end = coff_object.symbol_end(target, section)
        except (TypeError, ValueError):
            return reject("compiler-local-candidate-extent-is-invalid")
        if (
            target.value < 0
            or candidate_end - target.value != target_extent
            or candidate_end > len(section.raw_data)
        ):
            return reject("compiler-local-candidate-extent-drift")
        if any(
            target.value <= item.offset < candidate_end
            for item in coff_object.relocations_by_section.get(section.index, ())
        ):
            return reject("compiler-local-candidate-data-is-not-immutable")
        retail_target_bytes = _pe_bytes(
            reference,
            expected_target_base,
            target_extent,
        )
        candidate_object_bytes = section.raw_data[target.value:candidate_end]
        if candidate_object_bytes != retail_target_bytes:
            return reject("compiler-local-candidate-content-drift")

        if _vc5_compiler_local_family(raw_symbol) == ("named-static", expected_symbol):
            if (
                not section.characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA
                or section.characteristics & (IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_WRITE)
                or target_row.get("ownership_state") != "primary-owned"
                or _integer(expected.get("coff_addend"), field="COFF relocation addend") != 0 or resolved_target_addend != 0
            ):
                return reject("named-static-storage-or-addend-drift")
            named_sites = tuple(sorted(
                _integer(item.get("offset"), field="relocation offset")
                for item in relocation_catalog if item.get("target_symbol") == expected_symbol
            ))
            reader_failure = _static_data_reader_mismatch(
                coff_object=coff_object, target=target,
                universe=(retail_reader_universes or {}).get(target_symbol_id, ()),
                reviewed_object_symbol=str(expected.get("object_symbol", "")),
                site_offsets=named_sites, reviewed_retail_target=expected_target_base,
                require_all_readers=True,
            )
            if reader_failure is not None:
                return reject(reader_failure)

        candidate_storage_key = (target.section_number, target.value)
        if (
            candidate_storage_key in candidate_storage_keys
            or expected_target_base in expected_storage_bases
        ):
            return reject("compiler-local-duplicate-storage-collision")
        candidate_storage_keys.add(candidate_storage_key)
        expected_storage_bases.add(expected_target_base)
        pair_contracts[pair] = (
            target_symbol_id,
            target_extent,
            expected_target_base,
            retail_target_bytes,
            target.section_number,
            target.value,
        )

    canonicalized = list(candidates)
    for (
        index,
        _relocation,
        canonical,
        _expected,
        raw_symbol,
        expected_symbol,
    ) in proposals:
        target_symbol_id, target_extent, target_base, target_bytes, _, _ = pair_contracts[
            (raw_symbol, expected_symbol)
        ]
        canonicalized[index] = (
            candidates[index][0],
            replace(
                canonical,
                symbol_name=expected_symbol,
                compiler_local_ordinal_canonicalized=True,
                compiler_local_ordinal_reason="vc5-compiler-local-data-ordinal",
                target_symbol_id=target_symbol_id,
                target_extent=target_extent,
                expected_target_base=target_base,
                expected_target_bytes=target_bytes,
            ),
        )
    return canonicalized


def _masked_equal(first: bytes, second: bytes, mask: tuple[bool, ...]) -> bool:
    return len(first) == len(second) == len(mask) and all(
        masked or first[index] == second[index]
        for index, masked in enumerate(mask)
    )


def _map_target_identities(parsed_map: Any, address: int) -> set[str]:
    return {item.symbol for item in parsed_map.symbols if item.address == address}


def _map_object_name(value: Any) -> str:
    return str(value or "").replace("\\", "/").rsplit("/", 1)[-1]


def _candidate_target_identity(
    *,
    coff_object: CoffObject,
    obj_path: Path,
    parsed_map: Any,
    symbol_name: str,
    candidate_target_base: int | None,
) -> CandidateTargetIdentity:
    """Resolve a linked target identity without treating candidate output as truth.

    The MAP remains authoritative whenever it contains the exact target symbol.
    VC5 omits some defined static/compiler-local data symbols from its MAP.  Only
    in that absence, recover the target address from the current COFF object's
    section-relative symbol value and one uniquely anchored same-object section
    base.  Incomplete or conflicting placement evidence remains unresolved.
    """

    direct_bases = frozenset(
        int(item.address)
        for item in parsed_map.symbols
        if item.symbol == symbol_name
    )
    if direct_bases:
        identities = (
            frozenset(_map_target_identities(parsed_map, candidate_target_base))
            if candidate_target_base is not None
            else frozenset()
        )
        return CandidateTargetIdentity(
            target_bases=direct_bases,
            identities=identities,
            source="direct-map",
            reason="exact-target-symbol-present-in-map",
        )
    if candidate_target_base is None:
        return CandidateTargetIdentity(
            target_bases=frozenset(),
            identities=frozenset(),
            source="unresolved",
            reason="candidate-target-is-unresolved",
        )

    exact_targets = [
        item for item in coff_object.symbols if item.name == symbol_name
    ]
    if not exact_targets:
        return CandidateTargetIdentity(
            target_bases=frozenset(),
            identities=frozenset(),
            source="unresolved",
            reason="exact-target-symbol-absent-from-coff-object",
        )
    if len(exact_targets) != 1:
        return CandidateTargetIdentity(
            target_bases=frozenset(),
            identities=frozenset(),
            source="unresolved",
            reason="exact-target-symbol-is-ambiguous-in-coff-object",
        )
    target = exact_targets[0]
    if target.section_number <= 0:
        return CandidateTargetIdentity(
            target_bases=frozenset(),
            identities=frozenset(),
            source="unresolved",
            reason="exact-target-symbol-is-undefined-in-coff-object",
        )

    selected_object = obj_path.name.casefold()
    anchors = [
        item
        for item in coff_object.symbols
        if item.section_number == target.section_number
        and item.section_number > 0
        and item.storage_class == IMAGE_SYM_CLASS_EXTERNAL
    ]
    section_bases: set[int] = set()
    anchor_symbols: set[str] = set()
    cross_object_anchor_seen = False
    for anchor in anchors:
        for map_row in parsed_map.symbols:
            if map_row.symbol != anchor.name:
                continue
            if _map_object_name(getattr(map_row, "object", "")).casefold() != selected_object:
                cross_object_anchor_seen = True
                continue
            section_bases.add(int(map_row.address) - int(anchor.value))
            anchor_symbols.add(anchor.name)
    if not section_bases:
        return CandidateTargetIdentity(
            target_bases=frozenset(),
            identities=frozenset(),
            source="unresolved",
            reason=(
                "only-cross-object-section-anchors-were-found"
                if cross_object_anchor_seen
                else "same-object-section-is-unanchored"
            ),
        )
    if len(section_bases) != 1:
        return CandidateTargetIdentity(
            target_bases=frozenset(),
            identities=frozenset(),
            source="unresolved",
            reason="same-object-section-anchors-imply-conflicting-bases",
            anchor_symbols=tuple(sorted(anchor_symbols)),
        )
    section_base = next(iter(section_bases))
    derived_target_base = section_base + int(target.value)
    if derived_target_base != candidate_target_base:
        return CandidateTargetIdentity(
            target_bases=frozenset(),
            identities=frozenset(),
            source="unresolved",
            reason="coff-section-placement-does-not-resolve-candidate-target",
            section_base=section_base,
            anchor_symbols=tuple(sorted(anchor_symbols)),
        )
    return CandidateTargetIdentity(
        target_bases=frozenset((derived_target_base,)),
        identities=frozenset((symbol_name,)),
        source="same-object-coff-section-placement",
        reason="unique-same-object-section-base-resolves-exact-coff-target",
        section_base=section_base,
        anchor_symbols=tuple(sorted(anchor_symbols)),
    )


def _candidate_static_target_from_readers(
    *, coff_object: CoffObject, obj_path: Path, parsed_map: Any, image: Path,
    symbol_name: str, candidate_target_base: int | None,
    universe: Sequence[RetailObjectReader], reviewed_object_symbol: str,
    site_offsets: Sequence[int], reviewed_retail_target: int,
) -> CandidateTargetIdentity:
    """Place an already-proved static scalar through every mapped COFF reader.

    Retail supplies the complete reader identities and scalar semantics. Fresh
    linked reader bodies witness the placement of that exact COFF definition;
    their operands must all resolve to one base, with no missing reader.
    """
    def fail(reason: str) -> CandidateTargetIdentity:
        return CandidateTargetIdentity(frozenset(), frozenset(), "unresolved", reason)

    targets = [symbol for symbol in coff_object.symbols if symbol.name == symbol_name]
    if len(targets) != 1 or candidate_target_base is None:
        return fail("static-reader-target-is-not-unique")
    target = targets[0]
    mismatch = _static_data_reader_mismatch(
        coff_object=coff_object, target=target, universe=universe,
        reviewed_object_symbol=reviewed_object_symbol, site_offsets=site_offsets,
        reviewed_retail_target=reviewed_retail_target, require_all_readers=True,
    )
    if mismatch:
        return fail(mismatch)
    anchors: set[str] = set()
    bases: set[int] = set()
    for section_index, relocations in coff_object.relocations_by_section.items():
        for relocation in relocations:
            if relocation.symbol_index != target.index:
                continue
            section = coff_object.section(section_index)
            readers = [symbol for symbol in coff_object.symbols
                if symbol.section_number == section_index
                and symbol.type == IMAGE_SYM_DTYPE_FUNCTION
                and symbol.storage_class in {IMAGE_SYM_CLASS_EXTERNAL, IMAGE_SYM_CLASS_STATIC}
                and symbol.value <= relocation.offset < coff_object.function_end(symbol, section)]
            if len(readers) != 1:
                return fail("static-linked-reader-is-not-unique")
            reader = readers[0]
            placements = [row for row in parsed_map.symbols if row.symbol == reader.name]
            if len(placements) != 1 or _map_object_name(placements[0].object).casefold() != obj_path.name.casefold():
                return fail("static-linked-reader-map-identity-is-not-unique")
            body = coff_object.function_bytes(reader.name)
            try:
                linked = _pe_bytes(image, int(placements[0].address), len(body.data))
            except LiveByteError:
                return fail("static-linked-reader-body-is-unavailable")
            if not _masked_equal(body.data, linked, body.relocation_mask):
                return fail("static-linked-reader-body-differs-from-object")
            offset = relocation.offset - body.start
            bases.add(int.from_bytes(linked[offset:offset + 4], "little"))
            anchors.add(reader.name)
    if bases != {candidate_target_base} or not anchors:
        return fail("static-linked-readers-disagree-on-target")
    return CandidateTargetIdentity(
        frozenset(bases), frozenset((symbol_name,)), "registered-coff-reader-placement",
        "all-registered-readers-resolve-the-proved-static-definition",
        anchor_symbols=tuple(sorted(anchors)),
    )


def _symbolic_target_identity_passed(symbol_name: str, identities: set[str]) -> bool:
    return bool(symbol_name) and symbol_name in identities


def _pe_bytes(
    path: Path, address: int, length: int, *, allow_zero_fill: bool = False
) -> bytes:
    data = path.read_bytes()
    headers = parse_pe_headers(data, source=str(path))
    rva = address - headers.image_base
    sections = [
        section for section in headers.sections
        if section.virtual_address <= rva
        and rva + length <= section.virtual_address + max(section.virtual_size, section.raw_size)
    ]
    if length < 0 or len(sections) != 1:
        raise LiveByteError(
            f"address range [0x{address:x},0x{address + length:x}) does not lie "
            f"within one PE section in {display_path(path)}"
        )
    section = sections[0]
    relative = rva - section.virtual_address
    file_length = min(length, max(0, section.raw_size - relative))
    if file_length != length and not allow_zero_fill:
        raise LiveByteError(
            f"address range [0x{address:x},0x{address + length:x}) is not file-backed in "
            f"{display_path(path)}"
        )
    offset = section.raw_pointer + relative
    if file_length and (offset < 0 or offset + file_length > len(data)):
        raise LiveByteError(f"PE section file bytes are truncated in {display_path(path)}")
    initialized = data[offset : offset + file_length] if file_length else b""
    return initialized + bytes(length - file_length)


def _resolved_target(relocation_type: int, operand: bytes, instruction_end: int) -> int | None:
    if len(operand) != 4:
        return None
    if relocation_type == 0x14:
        return instruction_end + struct.unpack("<i", operand)[0]
    if relocation_type == 0x06:
        return struct.unpack("<I", operand)[0]
    return None


def _unique_root(mode: str) -> Path:
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    return Path("build") / "live-validation" / mode / f"run-{stamp}-{os_getpid()}"


def os_getpid() -> int:
    # Kept behind a tiny function so tests can replace the run-id source.
    import os

    return os.getpid()


def _rows(
    document: ProgressDocument,
    mode: str,
    at: str | None,
    *,
    authored_order_prefix_end: str | None = None,
) -> list[dict[str, Any]]:
    selected: list[dict[str, Any]] = []
    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping) or symbol.get("binary") != "recoil":
            continue
        if symbol.get("kind") not in {"function", "provider-function", "compiler-function"}:
            continue
        if not isinstance(symbol.get("end_exclusive"), str):
            continue
        if mode in {"object", "authored"} and not symbol_authored_order_gate(symbol):
            continue
        address = symbol.get("address")
        if not isinstance(address, str) or not address:
            raise LiveByteError(
                f"{symbol_id}: live byte row lacks its canonical address"
            )
        row = dict(symbol)
        row["symbol_id"] = str(symbol_id)
        selected.append(row)
    selected.sort(key=lambda item: (address_value(str(item["address"])), str(item["symbol_id"])))
    if mode == "object":
        prefix_text = authored_order_prefix_end
        if prefix_text is None:
            # Standalone verifier calls do not already have scheduler state.
            # Scheduler/preflight callers pass the one request-local derivation
            # explicitly so object-row selection does not derive it again.
            derived_prefix = document.pipeline("recoil").get(
                "authored_order_prefix_end"
            )
            prefix_text = derived_prefix if isinstance(derived_prefix, str) else None
        if not isinstance(prefix_text, str) or not prefix_text:
            selected = []
        else:
            prefix_end = address_value(prefix_text)
            selected = [
                row
                for row in selected
                if address_value(str(row["end_exclusive"])) <= prefix_end
            ]
    grouped: list[dict[str, Any]] = []
    for row in selected:
        key = (normalize_address(row["address"]), normalize_address(row["end_exclusive"]))
        if grouped and grouped[-1]["physical_range"] == key:
            grouped[-1]["scope_ids"].append(row["symbol_id"])
            grouped[-1]["physical_rows"].append(row)
        else:
            grouped.append(
                {
                    **row,
                    "physical_range": key,
                    "scope_ids": [row["symbol_id"]],
                    "physical_rows": [row],
                }
            )
    if at is not None:
        address = normalize_address(at)
        grouped = [row for row in grouped if normalize_address(row["address"]) == address]
        if not grouped:
            raise LiveByteError(f"no {mode} mode row exists at {address}")
    return grouped


def _bindings(
    document: ProgressDocument,
    manifest_dir: Path,
) -> dict[str, list[TargetBinding]]:
    # Byte-mode binding only needs symbol/source placement from manifests. Strict
    # source-policy provenance is an order/owner gate and must not block an
    # unrelated address when another target in the catalog is mid-migration.
    manifests = load_manifests(manifest_dir, enforce_source_policy=False)
    by_manifest = {
        target.manifest_path.resolve(): target
        for target in manifests
    }
    by_name = {target.name: target for target in manifests}
    result: dict[str, list[TargetBinding]] = {}
    targets = document.collection("verification_targets")

    def target_functions(target: Any) -> list[tuple[Any, str]]:
        rows: list[tuple[Any, str]] = [
            (function, _source_backed_path(target.source_from))
            for function in target.functions
        ]
        rows.extend(
            (function, _source_backed_path(entry.source_from))
            for entry in target.translation_unit_function_order
            for function in entry.functions
        )
        rows.extend(
            (function, _source_backed_path(target.source_from))
            for interval in target.linked_function_intervals
            for function in interval.functions
        )
        result: list[tuple[Any, str]] = []
        seen: set[tuple[str, str, str, str]] = set()
        for function, source_from in rows:
            if not source_from:
                continue
            key = (
                normalize_address(function.address),
                object_selector(function),
                str(function.logical_identity_key or ""),
                source_from,
            )
            if key not in seen:
                seen.add(key)
                result.append((function, source_from))
        return result

    def target_data_symbols(target: Any) -> list[tuple[Any, str]]:
        source_from = _source_backed_path(target.source_from)
        if not source_from:
            return []
        return [
            (data_symbol, source_from)
            for data_symbol in getattr(target, "data_symbols", ())
        ]

    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        registered: list[tuple[str, Any]] = []
        for target_id in symbol.get("verification_target_ids", []):
            target_row = targets.get(str(target_id))
            if not isinstance(target_row, Mapping) or target_row.get("kind") != "vc5":
                continue
            registration = target_row.get("registration", {})
            if not isinstance(registration, Mapping):
                continue
            manifest_text = registration.get("manifest_path")
            target = None
            if isinstance(manifest_text, str) and manifest_text:
                target = by_manifest.get((REPO_ROOT / manifest_text).resolve())
            if target is None:
                target = by_name.get(str(registration.get("name") or target_row.get("name") or ""))
            if target is not None and all(target != item[1] for item in registered):
                registered.append((str(target_id), target))
        address = normalize_address(str(symbol.get("address", "")))
        is_data = str(symbol.get("kind", "")).endswith("data") or symbol.get("kind") in {
            "data",
            "data-symbol",
        }
        for target_id, target in registered:
            matches = [
                (function, source_from)
                for function, source_from in (
                    target_data_symbols(target) if is_data else target_functions(target)
                )
                if normalize_address(function.address) == address
            ]
            for function, source_from in matches:
                result.setdefault(str(symbol_id), []).append(
                    TargetBinding(
                        target=target,
                        function=function,
                        target_id=str(target_id),
                        scope_id=str(symbol_id),
                        source_from=source_from,
                    )
                )
    return result


def _select_bindings(
    bindings: Mapping[str, list[TargetBinding]],
    row: Mapping[str, Any],
) -> list[TargetBinding]:
    address = normalize_address(row["address"])
    candidates: list[TargetBinding] = []
    seen: set[tuple[str, str, str, str]] = set()
    scope_ids = [str(scope_id) for scope_id in row.get("scope_ids", [])]
    source_backed_by_scope = {
        scope_id: [
            binding
            for binding in bindings.get(scope_id, [])
            if _binding_source_from(binding)
        ]
        for scope_id in scope_ids
    }
    missing_scope_ids = [
        scope_id
        for scope_id, scope_bindings in source_backed_by_scope.items()
        if not scope_bindings
    ]
    if missing_scope_ids:
        raise LiveByteError(
            f"{address}: physical address group scope has no source-backed byte target "
            f"among its exact tracker-registered VC5 target bindings: {missing_scope_ids}"
        )
    for scope_id in scope_ids:
        for candidate in source_backed_by_scope[scope_id]:
            source_from = _binding_source_from(candidate)
            key = (
                candidate.target.name,
                object_selector(candidate.function),
                str(getattr(candidate.function, "logical_identity_key", "") or ""),
                source_from,
            )
            if key not in seen:
                seen.add(key)
                candidates.append(candidate)
    if not candidates:
        names = sorted({item.target.name for item in candidates})
        raise LiveByteError(
            f"{address}: the physical address group has no source-backed byte target "
            "among its exact tracker-registered VC5 target bindings"
            + (f" ({', '.join(names)})" if names else "")
        )
    return candidates


def _is_provider_or_compiler_row(row: Mapping[str, Any]) -> bool:
    return (
        row.get("kind") in {"provider-function", "compiler-function"}
        or row.get("pipeline_class") == "non-authored"
        or str(row.get("authored_order_role", "")).startswith("compiler-generated-")
    )


def _registered_retail_reader_universe(
    *,
    document: ProgressDocument,
    bindings: Mapping[str, Sequence[TargetBinding]],
    binding: TargetBinding,
    target_symbol_id: str,
    reference: Path,
) -> tuple[RetailObjectReader, ...]:
    target_row = document.collection("symbols").get(target_symbol_id)
    if not isinstance(target_row, Mapping):
        raise LiveByteError(
            f"{target_symbol_id}: physical witness retail target row is missing"
        )
    try:
        retail_target = address_value(str(target_row["address"]))
    except (KeyError, TypeError, ValueError) as exc:
        raise LiveByteError(
            f"{target_symbol_id}: physical witness retail target address is invalid"
        ) from exc

    registered_target = str(binding.target.name)
    registered_source = _binding_source_from(binding)
    if not registered_target or not registered_source:
        raise LiveByteError(
            f"{target_symbol_id}: physical witness object registration is incomplete"
        )
    readers: list[RetailObjectReader] = []
    seen_registrations: set[tuple[str, str, str, int]] = set()
    symbol_rows = document.collection("symbols")
    for source_symbol_id, candidate_bindings in bindings.items():
        source_row = symbol_rows.get(source_symbol_id)
        if not isinstance(source_row, Mapping):
            continue
        try:
            source_address = address_value(str(source_row["address"]))
        except (KeyError, TypeError, ValueError):
            continue
        for candidate_binding in candidate_bindings:
            # A fresh object contains the complete TU, including readers
            # registered by a different synchronized verification target.
            if _binding_source_from(candidate_binding) != registered_source:
                continue
            object_symbol_regex = str(getattr(candidate_binding.function, "symbol_regex", "") or "")
            object_symbol = "" if object_symbol_regex else str(candidate_binding.function.symbol)
            registration_key = (
                str(source_symbol_id),
                object_symbol,
                object_symbol_regex,
                source_address,
            )
            if registration_key in seen_registrations:
                continue
            seen_registrations.add(registration_key)
            if source_row.get("kind") not in {"function", "provider-function", "compiler-function"}:
                # Registered data definitions are not x86 instruction streams.
                # An embedded target address still blocks this function-reader
                # proof rather than silently dropping a possible data reader.
                try:
                    source_end = address_value(str(source_row.get("end_exclusive", "")))
                except ValueError as exc:
                    raise LiveByteError(
                        f"{target_symbol_id}: registered data {source_symbol_id} has an unknown extent; "
                        "its possible data-reader population is unresolved"
                    ) from exc
                if source_end <= source_address:
                    raise LiveByteError(f"{source_symbol_id}: registered data extent is invalid")
                data = _pe_bytes(
                    reference, source_address, source_end - source_address, allow_zero_fill=True
                )
                if retail_target.to_bytes(4, "little") in data:
                    raise LiveByteError(
                        f"{target_symbol_id}: registered data {source_symbol_id} contains "
                        "the target address; its data-reader semantics are unresolved"
                    )
                continue
            retail_sites = decode_retail_target_sites(
                row=source_row,
                retail_target=retail_target,
                reference=reference,
            )
            if not retail_sites:
                continue
            if (
                source_row.get("kind") != "function"
                or _is_provider_or_compiler_row(source_row)
            ):
                raise LiveByteError(
                    f"{target_symbol_id}: registered retail reader {source_symbol_id} "
                    "is provider/compiler/non-authored"
                )
            for site in retail_sites:
                relocation_type = _integer(
                    site.get("type"),
                    field="retail reader relocation type",
                )
                if relocation_type != IMAGE_REL_I386_DIR32:
                    raise LiveByteError(
                        f"{target_symbol_id}: registered retail reader {source_symbol_id} "
                        f"uses unsupported relocation type 0x{relocation_type:04x}"
                    )
                readers.append(
                    RetailObjectReader(
                        source_symbol_id=str(source_symbol_id),
                        source_address=source_address,
                        object_symbol=object_symbol,
                        object_symbol_regex=object_symbol_regex,
                        function_offset=_integer(
                            site.get("offset"),
                            field="retail reader function offset",
                        ),
                        relocation_type=relocation_type,
                        coff_addend=0,
                        retail_target=_integer(
                            site.get("retail_target"),
                            field="retail reader target",
                        ),
                    )
                )
    return tuple(sorted(readers))


def _needs_retail_reader_universe(item, target_row, named_static_stems):
    if item.get("provenance_mode") == PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY:
        return True
    return (
        "reviewed-relocation-target-binding" in str(item.get("derivation", "")).split("+")
        and target_row.get("kind") == "data"
        and target_row.get("output_section_id") == "recoil:section:.rdata"
        and item.get("target_symbol") in named_static_stems
    )


def _provider_object_identity(value: str) -> tuple[str, str]:
    text = value.strip()
    if ":" in text:
        provider, member = text.split(":", 1)
        return provider, member
    if text.endswith(")") and "(" in text:
        provider, member = text[:-1].split("(", 1)
        return provider, member
    return text, ""


def _provider_binding(row: Mapping[str, Any]) -> Mapping[str, Any] | None:
    value = row.get("linked_provider_binding")
    return value if isinstance(value, Mapping) else None


def _provider_operand_target(
    operand: bytes,
    *,
    width: int,
    kind: str,
    operand_address: int,
    image_base: int,
) -> int | None:
    if width not in {1, 2, 4} or len(operand) != width:
        return None
    raw = int.from_bytes(operand, "little", signed=kind == "rel32")
    if kind == "rel32":
        return operand_address + width + raw
    if kind == "rva32":
        return image_base + raw
    if kind == "absolute32":
        return raw
    return None


def _compare_provider_row(
    *,
    row: Mapping[str, Any],
    paths: Any,
    reference: Path,
    parsed_map: Any,
    data_body: bool = False,
) -> dict[str, Any]:
    address = normalize_address(row["address"])
    scope_id = str(row.get("symbol_id", ""))
    binding = _provider_binding(row)
    if binding is None:
        return {
            "passed": False,
            "stage": "missing-provider-binding",
            "address": address,
            "scope_ids": [scope_id] if scope_id else [],
            "message": (
                "tracker provider/compiler row lacks linked_provider_binding with exact MAP "
                "symbol/object/provider/archive-member identity and resolved operand catalog"
            ),
        }
    required_strings = ("symbol_id", "map_symbol", "object", "provider", "archive_member")
    missing = [field for field in required_strings if not isinstance(binding.get(field), str)]
    operands = binding.get("operands")
    if missing or not isinstance(operands, list):
        return {
            "passed": False,
            "stage": "missing-provider-binding",
            "address": address,
            "scope_ids": [scope_id] if scope_id else [],
            "message": (
                "linked_provider_binding is incomplete: required strings="
                f"{list(required_strings)}, operands=list; missing={missing}"
            ),
        }
    if binding.get("symbol_id") != scope_id:
        return {
            "passed": False,
            "stage": "provider-binding-identity",
            "address": address,
            "scope_ids": [scope_id],
            "message": "linked_provider_binding.symbol_id does not match the tracker row",
        }
    map_symbol = str(binding["map_symbol"])
    map_object = str(binding["object"])
    matches = [
        item
        for item in parsed_map.symbols
        if item.is_function is not data_body and item.symbol == map_symbol and item.object == map_object
    ]
    if len(matches) != 1:
        return {
            "passed": False,
            "stage": "provider-map-identity",
            "address": address,
            "scope_ids": [scope_id],
            "message": f"selected provider MAP identity count is {len(matches)}, expected exactly one",
            "map_symbol": map_symbol,
            "object": map_object,
        }
    map_row = matches[0]
    provider, archive_member = _provider_object_identity(map_row.object)
    if provider != binding.get("provider") or archive_member != binding.get("archive_member"):
        return {
            "passed": False,
            "stage": "provider-map-identity",
            "address": address,
            "scope_ids": [scope_id],
            "message": "selected MAP provider/archive-member identity differs from tracker binding",
            "candidate_provider": provider,
            "candidate_archive_member": archive_member,
        }
    retail_start = address_value(address)
    retail_end = address_value(str(row["end_exclusive"]))
    extent = retail_end - retail_start
    candidate_start = int(map_row.address)
    retail_bytes = _pe_bytes(reference, retail_start, extent, allow_zero_fill=data_body)
    candidate_bytes = _pe_bytes(paths.exe_path, candidate_start, extent, allow_zero_fill=data_body)
    candidate_headers = parse_pe_headers(paths.exe_path.read_bytes(), source=str(paths.exe_path))
    retail_headers = parse_pe_headers(reference.read_bytes(), source=str(reference))
    operand_results: list[dict[str, Any]] = []
    operands_passed = True
    if data_body:
        from _recoil.lib.storage_proof import image_storage, pointer_fields
        reference_image, candidate_image = reference.read_bytes(), paths.exe_path.read_bytes()
        expected_fields = pointer_fields(reference_image, retail_start, extent)
        observed_fields = pointer_fields(candidate_image, candidate_start, extent)
        declared_fields = {item.get("offset") for item in operands if isinstance(item, Mapping)
                           and item.get("kind") == "absolute32" and item.get("width") == 4}
        # Every loader relocation needs a typed target in the accepted provider
        # contract; identical raw pointer bytes alone are insufficient.
        operands_passed = (expected_fields == observed_fields == declared_fields
            and image_storage(reference_image, retail_start, extent)[1:] ==
                image_storage(candidate_image, candidate_start, extent)[1:])
    seen_offsets: set[int] = set()
    for index, operand_row in enumerate(operands):
        if not isinstance(operand_row, Mapping):
            return {
                "passed": False,
                "stage": "missing-provider-binding",
                "address": address,
                "scope_ids": [scope_id],
                "message": f"linked_provider_binding.operands[{index}] must be an object",
            }
        offset = _integer(operand_row.get("offset"), field="provider operand offset")
        width = _integer(operand_row.get("width"), field="provider operand width")
        kind = operand_row.get("kind")
        target_symbol = operand_row.get("target_symbol")
        retail_target_expected = _integer(
            operand_row.get("retail_target"), field="provider retail target"
        )
        addend = _integer(operand_row.get("target_addend", 0), field="provider target addend")
        if (
            offset in seen_offsets
            or offset < 0
            or width not in {1, 2, 4}
            or offset + width > extent
            or kind not in {"rel32", "rva32", "absolute32"}
            or not isinstance(target_symbol, str)
            or not target_symbol
        ):
            return {
                "passed": False,
                "stage": "missing-provider-binding",
                "address": address,
                "scope_ids": [scope_id],
                "message": f"linked_provider_binding.operands[{index}] is incomplete or invalid",
            }
        seen_offsets.add(offset)
        candidate_target = _provider_operand_target(
            candidate_bytes[offset : offset + width],
            width=width,
            kind=str(kind),
            operand_address=candidate_start + offset,
            image_base=candidate_headers.image_base,
        )
        retail_target = _provider_operand_target(
            retail_bytes[offset : offset + width],
            width=width,
            kind=str(kind),
            operand_address=retail_start + offset,
            image_base=retail_headers.image_base,
        )
        candidate_symbol_targets = {
            int(item.address) + addend
            for item in parsed_map.symbols
            if item.symbol == target_symbol
        }
        passed = (
            candidate_target == retail_target == retail_target_expected
            and candidate_target in candidate_symbol_targets
        )
        operands_passed &= passed
        operand_results.append(
            {
                "offset": offset,
                "width": width,
                "kind": kind,
                "target_symbol": target_symbol,
                "target_addend": addend,
                "candidate_target": (
                    f"0x{candidate_target:x}" if candidate_target is not None else None
                ),
                "retail_target": f"0x{retail_target:x}" if retail_target is not None else None,
                "passed": passed,
            }
        )
    exact_address = candidate_start == retail_start
    exact_bytes = candidate_bytes == retail_bytes
    passed = exact_address and exact_bytes and operands_passed
    result: dict[str, Any] = {
        "passed": passed,
        "stage": "complete" if passed else "provider-linked-byte",
        "address": address,
        "end_exclusive": normalize_address(row["end_exclusive"]),
        "scope_ids": [scope_id],
        "binding_kind": "provider/compiler",
        "map_symbol": map_symbol,
        "object": map_object,
        "provider": provider,
        "archive_member": archive_member,
        "candidate_address": f"0x{candidate_start:x}",
        "retail_address": address,
        "exact_linked_address": exact_address,
        "exact_linked_bytes": exact_bytes,
        "resolved_operands": operand_results,
        "resolved_operands_equal": operands_passed,
    }
    if not exact_bytes:
        result["first_difference"] = _first_byte_difference(retail_bytes, candidate_bytes)
    return result


def _matched_group_contract(
    row: Mapping[str, Any],
    bindings: Mapping[str, list[TargetBinding]],
) -> dict[str, Any]:
    """Describe one completely matched physical group in stable tracker terms."""
    target_bindings: list[dict[str, Any]] = []
    logical_alias_ids: list[str] = []
    seen_bindings: set[tuple[str, str, str, str, str, str]] = set()
    seen_aliases: set[str] = set()
    scope_ids = [str(scope_id) for scope_id in row.get("scope_ids", [])]
    physical_by_id = {
        str(item.get("symbol_id")): item
        for item in row.get("physical_rows", [])
        if isinstance(item, Mapping)
    }
    for scope_id in scope_ids:
        physical_row = physical_by_id.get(scope_id)
        if isinstance(physical_row, Mapping) and _is_provider_or_compiler_row(physical_row):
            provider_binding = _provider_binding(physical_row)
            if provider_binding is None:
                raise LiveByteError(
                    f"{normalize_address(row['address'])}: cannot report matched provider scope "
                    f"{scope_id!r} without linked_provider_binding"
                )
            target_bindings.append(
                {
                    "scope_id": scope_id,
                    "binding_kind": "provider/compiler",
                    "map_symbol": str(provider_binding.get("map_symbol", "")),
                    "object": str(provider_binding.get("object", "")),
                    "provider": str(provider_binding.get("provider", "")),
                    "archive_member": str(provider_binding.get("archive_member", "")),
                }
            )
            continue
        scope_bindings = [
            binding
            for binding in bindings.get(scope_id, [])
            if _binding_source_from(binding)
        ]
        if not scope_bindings:
            raise LiveByteError(
                f"{normalize_address(row['address'])}: cannot report a matched group because "
                f"scope {scope_id!r} has no source-backed byte target among its exact "
                "tracker-registered VC5 target bindings"
            )
        for binding in scope_bindings:
            logical_identity = str(getattr(binding.function, "logical_identity_key", "") or "")
            source_from = _binding_source_from(binding)
            key = (
                scope_id,
                str(binding.target_id),
                str(binding.target.name),
                object_selector(binding.function),
                logical_identity,
                source_from,
            )
            if key not in seen_bindings:
                seen_bindings.add(key)
                target_bindings.append(
                    {
                        "scope_id": scope_id,
                        "target_id": str(binding.target_id),
                        "target_name": str(binding.target.name),
                        "symbol": object_selector(binding.function),
                        "source_from": source_from,
                        "logical_identity_key": logical_identity or None,
                    }
                )
            if logical_identity and logical_identity not in seen_aliases:
                seen_aliases.add(logical_identity)
                logical_alias_ids.append(logical_identity)
    return {
        "address": normalize_address(row["address"]),
        "end_exclusive": normalize_address(row["end_exclusive"]),
        "scope_ids": scope_ids,
        "logical_alias_ids": logical_alias_ids,
        "target_bindings": target_bindings,
    }


def _artifact_signature(path: Path) -> ArtifactSignature:
    try:
        stat = path.stat()
    except FileNotFoundError:
        return ArtifactSignature(False, None, None)
    return ArtifactSignature(True, stat.st_size, stat.st_mtime_ns)


def _fresh_artifacts(
    before: Mapping[Path, ArtifactSignature],
    required: Sequence[Path],
) -> tuple[list[Path], list[Path]]:
    missing: list[Path] = []
    unchanged: list[Path] = []
    for path in required:
        after = _artifact_signature(path)
        if not after.present:
            missing.append(path)
        elif after == before[path]:
            unchanged.append(path)
    return missing, unchanged


def _run_fresh_build(args: argparse.Namespace, root: Path) -> tuple[int, Any, Any]:
    config = with_explicit_build_dir(load_config(args.final_config), root)
    paths = build_paths(config)
    object_paths = [object_path(config, paths, source) for source in config.sources]
    required = [*object_paths, paths.summary_path]
    if args.mode != "object":
        required.extend((paths.exe_path, paths.map_path))
    before = {path: _artifact_signature(path) for path in required}
    command = [
        sys.executable,
        str(REPO_ROOT / "tools" / "recoil.py"),
        "verify",
        "final-build",
        "--",
        "--manifest",
        str(args.final_config),
        "--build-dir",
        str(root),
        "--clean",
    ]
    if args.mode == "object":
        command.extend(("--compile-only", "--compile-only-skip-linked-order"))
    else:
        command.append("--linkability-only")
    # The byte verifier owns both its comparison and machine-readable stdout.
    # Its artifact producer must not evaluate another stage or deploy a candidate.
    completed = subprocess.run(
        command,
        cwd=REPO_ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    missing, unchanged = _fresh_artifacts(before, required)
    if missing or unchanged:
        detail = completed.stderr.strip() or completed.stdout.strip()
        problems: list[str] = []
        if missing:
            problems.append(
                "missing " + ", ".join(display_path(path) for path in missing[:8])
            )
        if unchanged:
            problems.append(
                "unchanged from pre-build signature "
                + ", ".join(display_path(path) for path in unchanged[:8])
            )
        raise LiveByteError(
            "fresh final-build did not produce every required artifact: "
            + "; ".join(problems)
            + "; "
            f"build exited {completed.returncode}"
            + (f": {detail[:500]}" if detail else "")
        )

    try:
        summary = json.loads(paths.summary_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise LiveByteError(f"fresh final-build summary is unreadable: {exc}") from exc
    if args.mode == "object":
        if completed.returncode != 0 or summary.get("kind") != "compile-only-diagnostic" or not summary.get("success"):
            raise LiveByteError(
                "fresh compile-only final-build did not complete successfully; "
                f"build exited {completed.returncode}"
            )
    else:
        artifacts_complete = all(
            summary.get(field) is True
            for field in (
                "compile_succeeded",
                "coff_alias_sources_succeeded",
                "resource_succeeded",
                "link_succeeded",
                "candidate_available",
            )
        )
        if not artifacts_complete:
            raise LiveByteError(
                "fresh final-build summary does not prove a complete compile/link artifact set; "
                f"build exited {completed.returncode}"
            )
        deployment = summary.get("playtest_deploy")
        diagnostic_only = (
            summary.get("kind") == "final-build-diagnostic"
            and summary.get("diagnostic_kind") == "whole-program-linkability"
            and summary.get("success") is True
            and summary.get("fresh_build") is True
            and summary.get("reuse") is False
            and summary.get("linked_order_evaluation_suppressed") is True
            and summary.get("playtest_deployment_suppressed") is True
            and all(
                summary.get(field) is False
                for field in (
                    "accepts_linked_order", "accepts_bytes", "accepts_final_image",
                    "candidate_expected_truth",
                )
            )
            and isinstance(deployment, Mapping)
            and deployment.get("attempted") is False
            and deployment.get("updated") is False
        )
        if completed.returncode != 0 or not diagnostic_only:
            raise LiveByteError(
                "fresh byte artifacts require a successful non-deploying linkability "
                f"diagnostic; build exited {completed.returncode}"
            )
    return completed.returncode, config, paths


def _map_function_addresses(parsed_map: Any, symbol: str) -> list[int]:
    return sorted(
        {
            item.address
            for item in parsed_map.symbols
            if item.is_function and item.symbol == symbol
        }
    )


def _first_byte_difference(expected: bytes, actual: bytes) -> dict[str, Any] | None:
    extent = min(len(expected), len(actual))
    for index in range(extent):
        if expected[index] != actual[index]:
            return {
                "offset": index,
                "expected": f"0x{expected[index]:02x}",
                "candidate": f"0x{actual[index]:02x}",
            }
    if len(expected) != len(actual):
        return {
            "offset": extent,
            "expected_length": len(expected),
            "candidate_length": len(actual),
        }
    return None


def _retail_relocation_catalog(
    row: Mapping[str, Any],
    object_symbol: str,
) -> list[Mapping[str, Any]] | None:
    matches: list[list[Mapping[str, Any]]] = []
    for physical_row in row.get("physical_rows", []):
        if not isinstance(physical_row, Mapping):
            continue
        catalogs = physical_row.get("retail_relocations")
        if not isinstance(catalogs, list):
            continue
        selected = [
            item
            for item in catalogs
            if isinstance(item, Mapping) and item.get("object_symbol") == object_symbol
        ]
        if selected:
            matches.append(selected)
    if not matches:
        return None
    first = matches[0]
    if any(list(item) != list(first) for item in matches[1:]):
        raise LiveByteError(f"conflicting retail relocation catalogs for {object_symbol}")
    return first


def _integer(value: Any, *, field: str) -> int:
    if isinstance(value, int) and not isinstance(value, bool):
        return value
    if isinstance(value, str):
        try:
            return int(value, 0)
        except ValueError as exc:
            raise LiveByteError(f"invalid {field} value {value!r}") from exc
    raise LiveByteError(f"missing integer {field}")


def _compare_row(
    *,
    mode: str,
    row: Mapping[str, Any],
    binding: TargetBinding,
    config: Any,
    paths: Any,
    reference: Path,
    parsed_map: Any | None,
    relocation_catalog: Sequence[Mapping[str, Any]] | None = None,
    target_rows: Mapping[str, Any] | None = None,
    retail_reader_universes: Mapping[
        str, Sequence[RetailObjectReader]
    ] | None = None,
    physical_targets: dict[int, set[int]] | None = None,
    cleanup_physical_targets: set[int] | None = None,
    allow_near_byte_review: bool = False,
) -> dict[str, Any]:
    address = normalize_address(row["address"])
    retail_start = address_value(address)
    retail_end = address_value(str(row["end_exclusive"]))
    extent = retail_end - retail_start
    source_from = str(binding.source_from or binding.target.source_from)
    source = (REPO_ROOT / source_from).resolve()
    obj_path = object_path(config, paths, source)
    if not obj_path.is_file():
        raise LiveByteError(f"{address}: fresh object is missing: {display_path(obj_path)}")
    coff_object = CoffObject.from_path(obj_path)
    try:
        if getattr(binding.function, "symbol_regex", None):
            resolved = resolve_function_symbol_for_coff(coff_object, binding.function)
            binding = replace(binding, function=replace(binding.function, symbol=resolved, symbol_regex=None))
        function_bytes = coff_object.function_bytes(binding.function.symbol)
    except ValueError as exc:
        return {
            "passed": False, "stage": "object-symbol", "address": address,
            "end_exclusive": normalize_address(row["end_exclusive"]),
            "scope_ids": list(row.get("scope_ids", [row["symbol_id"]])),
            "source": source_from, "target": binding.target.name,
            "symbol": binding.function.symbol, "object_path": display_path(obj_path),
            "message": str(exc),
        }
    contribution_extent = len(function_bytes.data)
    # A next-symbol COFF extent may include alignment padding after the typed
    # function body.  This mode owns the body; linked order/padding gates own
    # that tail.  A contribution shorter than the body is still a hard failure.
    if contribution_extent < extent:
        return {
            "passed": False,
            "stage": "object-extent",
            "address": address,
            "end_exclusive": normalize_address(row["end_exclusive"]),
            "scope_ids": list(row.get("scope_ids", [row["symbol_id"]])),
            "source": source_from,
            "target": binding.target.name,
            "symbol": binding.function.symbol,
            "object_path": display_path(obj_path),
            "candidate_extent": contribution_extent,
            "retail_extent": extent,
            "message": (
                f"natural COFF contribution extent {contribution_extent} is shorter than "
                f"typed retail body extent {extent}"
            ),
        }
    if contribution_extent > extent:
        function_bytes = coff_object.function_bytes(
            binding.function.symbol,
            byte_length=extent,
        )
    retail_bytes = _pe_bytes(reference, retail_start, extent)
    object_passed = _masked_equal(
        function_bytes.data,
        retail_bytes,
        function_bytes.relocation_mask,
    )
    result: dict[str, Any] = {
        "passed": object_passed,
        "stage": "object-body",
        "address": address,
        "end_exclusive": normalize_address(row["end_exclusive"]),
        "scope_ids": list(row.get("scope_ids", [row["symbol_id"]])),
        "source": source_from,
        "target": binding.target.name,
        "symbol": binding.function.symbol,
        "object_path": display_path(obj_path),
        "object_body_extent": extent,
        "object_contribution_extent": contribution_extent,
        "object_trailing_contribution_extent": contribution_extent - extent,
        "object_trailing_relocation_count": getattr(
            function_bytes,
            "excluded_tail_relocation_count",
            0,
        ),
        "object_body_equal_outside_relocations": object_passed,
    }
    instruction_proof = None
    if not object_passed:
        normalized_object = bytes(
            retail_bytes[index] if function_bytes.relocation_mask[index] else value
            for index, value in enumerate(function_bytes.data)
        )
        result["first_difference"] = _first_byte_difference(retail_bytes, normalized_object)
        instruction_proof = compare_instructions(retail_bytes, normalized_object,
            relocations=list(relocation_catalog or ()), symbol=binding.function.symbol)
        result["instruction_proof"] = instruction_proof
        if (not instruction_proof["passed"] and not allow_near_byte_review) or mode == "object":
            return result
        scope = str(binding.scope_id or row.get("symbol_id", ""))
        review = (target_rows or {}).get(scope, {}).get("instruction_match_review")
        context = source_context(source_from, config)
        result["instruction_review_current"] = (instruction_proof["passed"] and
            review_current(review, context, instruction_proof["differences"]))
        result["review_evidence_id"] = review.get("evidence_id") if isinstance(review, Mapping) else None
    if mode == "object":
        return result
    assert parsed_map is not None
    linked_addresses = _map_function_addresses(parsed_map, binding.function.symbol)
    if len(linked_addresses) != 1:
        result.update(
            passed=False,
            stage="linked-presence",
            message=f"selected linked address count is {len(linked_addresses)}",
            candidate_addresses=[f"0x{item:x}" for item in linked_addresses],
        )
        return result
    candidate_start = linked_addresses[0]
    candidate_bytes = _pe_bytes(paths.exe_path, candidate_start, extent)
    linked_body_passed = _masked_equal(
        candidate_bytes,
        retail_bytes,
        function_bytes.relocation_mask,
    )
    linked_instruction_proof = None
    if not linked_body_passed and instruction_proof is not None:
        normalized_linked = bytes(retail_bytes[i] if function_bytes.relocation_mask[i] else value
                                  for i, value in enumerate(candidate_bytes))
        linked_instruction_proof = compare_instructions(retail_bytes, normalized_linked,
            relocations=list(relocation_catalog or ()), symbol=binding.function.symbol)
    relocation_rows: list[dict[str, Any]] = []
    relocations_passed = True
    if relocation_catalog is None:
        result.update(
            passed=False,
            stage="relocation-expectations",
            message=(
                "candidate-independent retail relocation expectations were not supplied for "
                "this exact registered object symbol"
            ),
        )
        return result
    if mode == "authored":
        candidate_relocations = _canonicalize_vc5_local_data_ordinals(
            coff_object=coff_object,
            function_bytes=function_bytes,
            relocation_catalog=relocation_catalog,
            target_rows=target_rows,
            reference=reference,
            retail_reader_universes=retail_reader_universes,
        )
    else:
        candidate_relocations = []
        for relocation in function_bytes.relocations:
            size = relocation_size(relocation.type)
            relative = relocation.offset - function_bytes.start
            raw_addend = int.from_bytes(
                function_bytes.data[relative : relative + size],
                "little",
                signed=False,
            )
            candidate_relocations.append(
                (
                    relocation,
                    _canonicalize_same_comdat_local_label(
                        coff_object=coff_object,
                        function_bytes=function_bytes,
                        relocation=relocation,
                        raw_addend=raw_addend,
                    ),
                )
            )
    if mode == "linked":
        candidate_relocations = _canonicalize_native_relocation_roles(
            coff_object, function_bytes, candidate_relocations, relocation_catalog)
    definitions = {source.resolve(): coff_object}
    configured_sources = {item.resolve() for item in config.sources}
    def load_definition(source_from):
        definition_source = (REPO_ROOT / source_from).resolve()
        if definition_source not in configured_sources:
            raise ValueError("registered selector TU is absent from the canonical build")
        if definition_source not in definitions:
            definitions[definition_source] = CoffObject.from_path(object_path(config, paths, definition_source))
        return definitions[definition_source]
    candidate_relocations = _canonicalize_registered_target_selectors(
        function_bytes, candidate_relocations, relocation_catalog, load_definition)
    observed_keys = {
        (
            relocation.offset - function_bytes.start,
            relocation.type,
            canonical.symbol_name,
        )
        for relocation, canonical in candidate_relocations
    }
    expected_keys = {
        (
            _integer(item.get("offset"), field="relocation offset"),
            _integer(item.get("type"), field="relocation type"),
            str(item.get("target_symbol", "")),
        )
        for item in relocation_catalog
    }
    if observed_keys != expected_keys:
        result.update(
            passed=False,
            stage="relocation-population",
            message="COFF relocation offset/type/target population differs from the retail catalog",
            expected_relocations=sorted(expected_keys),
            candidate_relocations=sorted(observed_keys),
            candidate_relocation_details=[
                {
                    "offset": relocation.offset - function_bytes.start,
                    "type": relocation.type,
                    "object_symbol": relocation.symbol_name,
                    "canonical_symbol": canonical.symbol_name,
                    "raw_coff_addend": int.from_bytes(
                        function_bytes.data[
                            relocation.offset
                            - function_bytes.start : relocation.offset
                            - function_bytes.start
                            + relocation_size(relocation.type)
                        ],
                        "little",
                        signed=False,
                    ),
                    "canonical_coff_addend": canonical.coff_addend,
                    "canonicalized": canonical.canonicalized,
                    "canonicalization_reason": canonical.reason,
                    "compiler_local_ordinal_canonicalized": (
                        canonical.compiler_local_ordinal_canonicalized
                    ),
                    "compiler_local_ordinal_reason": (
                        canonical.compiler_local_ordinal_reason
                    ),
                    "target_symbol_id": canonical.target_symbol_id or None,
                    "target_extent": canonical.target_extent,
                }
                for relocation, canonical in candidate_relocations
            ],
        )
        return result
    catalog_by_key = {
        (
            _integer(item.get("offset"), field="relocation offset"),
            _integer(item.get("type"), field="relocation type"),
            str(item.get("target_symbol", "")),
        ): item
        for item in relocation_catalog
    }
    for relocation, canonical in candidate_relocations:
        size = relocation_size(relocation.type)
        relative = relocation.offset - function_bytes.start
        expected_row = catalog_by_key[(relative, relocation.type, canonical.symbol_name)]
        object_operand = function_bytes.data[relative : relative + size]
        raw_coff_addend = int.from_bytes(object_operand, "little", signed=False)
        coff_addend = canonical.coff_addend
        expected_coff_addend = _integer(
            expected_row.get("coff_addend"), field="COFF relocation addend"
        )
        candidate_operand = candidate_bytes[relative : relative + size]
        retail_operand = retail_bytes[relative : relative + size]
        candidate_target = _resolved_target(
            relocation.type,
            candidate_operand,
            candidate_start + relative + size,
        )
        retail_target = _resolved_target(
            relocation.type,
            retail_operand,
            retail_start + relative + size,
        )
        resolved_target_addend = _integer(
            expected_row.get("resolved_target_addend", 0),
            field="resolved target addend",
        )
        candidate_target_base = (
            candidate_target - resolved_target_addend
            if candidate_target is not None
            else None
        )
        placement_symbol = canonical.registered_symbol_name or (
            str(relocation.symbol_name)
            if canonical.compiler_local_ordinal_canonicalized
            else canonical.symbol_name
        )
        target_identity = _candidate_target_identity(
            coff_object=coff_object,
            obj_path=obj_path,
            parsed_map=parsed_map,
            symbol_name=placement_symbol,
            candidate_target_base=candidate_target_base,
        )
        if (canonical.compiler_local_ordinal_canonicalized
                and target_identity.reason == "same-object-section-is-unanchored"):
            target_identity = _candidate_static_target_from_readers(
                coff_object=coff_object, obj_path=obj_path, parsed_map=parsed_map,
                image=paths.exe_path, symbol_name=placement_symbol,
                candidate_target_base=candidate_target_base,
                universe=(retail_reader_universes or {}).get(canonical.target_symbol_id, ()),
                reviewed_object_symbol=binding.function.symbol,
                site_offsets=[_integer(item["offset"], field="reader offset")
                    for item in relocation_catalog if item.get("target_symbol_id") == canonical.target_symbol_id],
                reviewed_retail_target=_integer(expected_row["retail_target"], field="retail target"),
            )
        if expected_row.get("native_eh_role"):
            from _recoil.commands.native_eh_relocations import (
                ABSOLUTE, HANDLER, prove_object_handler, prove_linked_packet, require,
            )
            try:
                if expected_row["native_eh_role"] == HANDLER:
                    target = prove_object_handler(coff_object, function_bytes, relocation, expected_row)
                    require(candidate_target_base is not None, "native EH target is unresolved")
                    prove_linked_packet(coff_object, function_bytes, target, candidate_target_base,
                                        parsed_map, paths.exe_path)
                elif expected_row["native_eh_role"] == ABSOLUTE:
                    target = coff_object.symbols_by_index[relocation.symbol_index]
                    require(target.name == relocation.symbol_name == "__except_list"
                            and target.storage_class == 2 and target.type == 0
                            and target.section_number == 0 and target.value == 0
                            and raw_coff_addend == 0 and candidate_target_base == 0,
                            "native EH absolute runtime reference drift")
                else:
                    raise ValueError("unknown native EH relocation role")
                target_identity = CandidateTargetIdentity(
                    frozenset((candidate_target_base,)), frozenset((placement_symbol,)),
                    expected_row["native_eh_role"], "fresh-native-eh-role-witness")
            except (OSError, ValueError, KeyError, TypeError) as exc:
                target_identity = CandidateTargetIdentity(frozenset(), frozenset(),
                    "unresolved-native-eh-role", str(exc))
        if expected_row.get("native_cleanup"):
            from _recoil.commands.native_array_cleanup import prove_linked_cleanup
            try:
                prove_linked_cleanup(coff_object, function_bytes, relocation, expected_row,
                                     parsed_map, paths.exe_path, candidate_target_base)
                target_identity = CandidateTargetIdentity(frozenset((candidate_target_base,)),
                    frozenset((placement_symbol,)), "native-array-implicit-cleanup", "fresh-physical-cleanup-witness")
            except (OSError, ValueError, KeyError, TypeError) as exc:
                target_identity = CandidateTargetIdentity(frozenset(), frozenset(), "unresolved-native-cleanup", str(exc))
        if expected_row.get("native_import"):
            from _recoil.commands.native_import_relocations import prove_candidate_import
            try:
                prove_candidate_import(coff_object, function_bytes, relocation, expected_row,
                                       parsed_map, paths.exe_path, candidate_target_base)
                target_identity = CandidateTargetIdentity(frozenset((candidate_target_base,)),
                    frozenset((placement_symbol,)), "native-import-thunk", "fresh-canonical-import-witness")
            except (OSError, ValueError, KeyError, TypeError) as exc:
                target_identity = CandidateTargetIdentity(frozenset(), frozenset(), "unresolved-native-import", str(exc))
        target_bases = set(target_identity.target_bases)
        identities = set(target_identity.identities)
        expected_candidate_targets = {
            base + resolved_target_addend for base in target_bases
        }
        expected_retail_target = _integer(
            expected_row.get("retail_target"), field="retail resolved target"
        )
        compiler_local_data_bytes_passed = True
        if canonical.compiler_local_ordinal_canonicalized:
            compiler_local_data_bytes_passed = (
                canonical.target_extent is not None
                and canonical.expected_target_base is not None
                and candidate_target_base is not None
                and _pe_bytes(
                    paths.exe_path,
                    candidate_target_base,
                    canonical.target_extent,
                )
                == canonical.expected_target_bytes
                == _pe_bytes(
                    reference,
                    canonical.expected_target_base,
                    canonical.target_extent,
                )
            )
        if mode == "linked":
            passed = (
                candidate_operand == retail_operand
                and candidate_target == retail_target == expected_retail_target
                and coff_addend == expected_coff_addend
                and compiler_local_data_bytes_passed
                and (not (expected_row.get("native_eh_role") or expected_row.get("native_cleanup") or expected_row.get("native_import"))
                     or bool(target_identity.identities))
            )
        else:
            passed = (
                _symbolic_target_identity_passed(placement_symbol, identities)
                and candidate_target in expected_candidate_targets
                and retail_target == expected_retail_target
                and coff_addend == expected_coff_addend
                and compiler_local_data_bytes_passed
            )
        if passed and physical_targets is not None and cleanup_physical_targets is not None:
            from _recoil.commands.native_array_cleanup import record_physical_mapping
            retail_base = expected_retail_target-resolved_target_addend
            if not record_physical_mapping(retail_base, candidate_target_base,
                    bool(expected_row.get("native_cleanup")), physical_targets, cleanup_physical_targets):
                passed = False
                target_identity = replace(target_identity, reason="native-cleanup-physical-target-mapping-conflict")
        relocations_passed &= passed
        relocation_rows.append(
            {
                "offset": relative,
                "type": relocation.type,
                "type_name": relocation_type_name(relocation.type),
                "symbolic_target": relocation.symbol_name,
                "canonical_symbolic_target": canonical.symbol_name,
                "candidate_raw_symbolic_target": relocation.symbol_name,
                "local_label_canonicalized": canonical.canonicalized,
                "local_label_canonicalization_reason": canonical.reason,
                "compiler_local_ordinal_canonicalized": (
                    canonical.compiler_local_ordinal_canonicalized
                ),
                "compiler_local_ordinal_canonicalization_reason": (
                    canonical.compiler_local_ordinal_reason
                ),
                "compiler_local_target_symbol_id": (
                    canonical.target_symbol_id or None
                ),
                "compiler_local_target_extent": canonical.target_extent,
                "compiler_local_data_bytes_equal": (
                    compiler_local_data_bytes_passed
                ),
                "candidate_placement_symbol": placement_symbol,
                "candidate_target": f"0x{candidate_target:x}" if candidate_target is not None else None,
                "retail_target": f"0x{retail_target:x}" if retail_target is not None else None,
                "candidate_target_identities": sorted(identities),
                "candidate_target_identity_source": target_identity.source,
                "candidate_target_identity_reason": target_identity.reason,
                "candidate_target_section_base": (
                    f"0x{target_identity.section_base:x}"
                    if target_identity.section_base is not None
                    else None
                ),
                "candidate_target_anchor_symbols": list(target_identity.anchor_symbols),
                "raw_coff_addend": raw_coff_addend,
                "coff_addend": coff_addend,
                "expected_coff_addend": expected_coff_addend,
                "resolved_target_addend": resolved_target_addend,
                "passed": passed,
            }
        )
    exact_address = candidate_start == retail_start
    exact_bytes = candidate_bytes == retail_bytes
    instruction_passed = (instruction_proof is not None and instruction_proof["passed"]
                          and linked_instruction_proof is not None and linked_instruction_proof["passed"]
                          and instruction_proof["differences"] == linked_instruction_proof["differences"]
                          and result.get("instruction_review_current") is True)
    body_passed = (object_passed and linked_body_passed) or instruction_passed
    passed = body_passed and relocations_passed
    if mode == "linked":
        passed = passed and exact_address and (exact_bytes or instruction_passed)
    result.update(
        passed=passed,
        stage="linked-body" if not passed else "complete",
        candidate_address=f"0x{candidate_start:x}",
        retail_address=address,
        exact_linked_address=exact_address,
        linked_body_equal_outside_relocations=linked_body_passed,
        exact_linked_bytes=exact_bytes,
        relocations=relocation_rows,
        relocation_expectations_exact=relocations_passed,
        match_level=("byte" if object_passed and linked_body_passed else "instruction") if passed else None,
        instruction_equivalent=bool(instruction_proof and instruction_proof["passed"]),
        linked_instruction_proof=linked_instruction_proof,
        match_version=MATCH_VERSION,
        retail_relocations=list(relocation_catalog),
        # Advisory near-byte review may inspect code differences only after
        # typed operands and the selected current object's linked body agree.
        # This flag never changes `passed`, match levels, or stage acceptance.
        structural_comparison_complete=(relocations_passed and _masked_equal(
            function_bytes.data, candidate_bytes, function_bytes.relocation_mask)),
        # Compatibility projection for old report readers; expectations are now
        # derived live and are not a required stored tracker catalog.
    )
    if not passed and relocations_passed and not object_passed and result.get("instruction_review_current") is not True:
        result["stage"] = "instruction-review-required"
    if not exact_bytes:
        result["first_difference"] = _first_byte_difference(retail_bytes, candidate_bytes)
    return result


def run(args: argparse.Namespace) -> dict[str, Any]:
    from _recoil.lib.match_evidence import dependency_states
    # Fresh compilation is mandatory; these coordinates only reject edits that
    # race this invocation. They are never acceptance evidence or build reuse.
    input_paths = [str(path) for tree in (REPO_ROOT / "src", REPO_ROOT / "tools")
                   for path in tree.rglob("*") if path.is_file()
                   and path.suffix.lower() in {".h", ".cpp", ".c", ".rc", ".json", ".py", ".txt"}]
    input_paths.append(str(args.reference))
    inputs_before = dependency_states(input_paths)
    document = ProgressDocument.load(args.progress)
    rows = _rows(document, args.mode, args.at)
    if not rows:
        raise LiveByteError(f"the {args.mode} mode has no selected rows")
    bindings = _bindings(document, args.manifest_dir)
    # Validate the shared target census once against this fresh invocation's
    # inputs. Each function still derives and compares all its own operands.
    target_identity_state = build_target_identity_state(document, bindings, reference=args.reference)
    relocation_expectations: dict[tuple[str, str], dict[str, Any]] = {}
    retail_reader_universe_cache: dict[
        tuple[str, str, str],
        tuple[RetailObjectReader, ...],
    ] = {}

    def expectation_for(
        current_row: Mapping[str, Any], current_binding: TargetBinding
    ) -> dict[str, Any]:
        key = (
            normalize_address(current_row["address"]),
            object_selector(current_binding.function),
        )
        if key not in relocation_expectations:
            relocation_expectations[key] = derive_relocation_expectations(
                document=document,
                row=current_row,
                object_symbol=object_selector(current_binding.function),
                bindings=bindings,
                reference=args.reference,
                target_identity_state=target_identity_state,
            )
        return relocation_expectations[key]

    def retail_reader_universes_for(
        current_binding: TargetBinding,
        relocation_catalog: Sequence[Mapping[str, Any]],
    ) -> dict[str, tuple[RetailObjectReader, ...]]:
        # Candidate spelling only selects which proof is needed. The universe
        # itself is still derived exclusively from retail and registered facts.
        # Exact external symbols and IAT slots need no static-ordinal proof.
        obj = CoffObject.from_path(object_path(config, paths, REPO_ROOT / _binding_source_from(current_binding)))
        named_static_stems = {
            family[1] for symbol in obj.symbols
            if (family := _vc5_compiler_local_family(str(symbol.name))) is not None and family[0] == "named-static"
        }
        target_symbol_ids = {
            str(item.get("target_symbol_id", ""))
            for item in relocation_catalog
            if _needs_retail_reader_universe(
                item, document.collection("symbols").get(str(item.get("target_symbol_id", "")), {}),
                named_static_stems,
            )
        }
        target_symbol_ids.discard("")
        result: dict[str, tuple[RetailObjectReader, ...]] = {}
        for target_symbol_id in sorted(target_symbol_ids):
            key = (
                str(current_binding.target.name),
                _binding_source_from(current_binding),
                target_symbol_id,
            )
            if key not in retail_reader_universe_cache:
                retail_reader_universe_cache[key] = (
                    _registered_retail_reader_universe(
                        document=document,
                        bindings=bindings,
                        binding=current_binding,
                        target_symbol_id=target_symbol_id,
                        reference=args.reference,
                    )
                )
            result[target_symbol_id] = retail_reader_universe_cache[key]
        return result

    # Authored-byte validation is frequently invoked at the live cursor.  Resolve
    # its retail operand contract before paying for a whole-project link.  Later
    # groups are derived lazily from the same candidate-independent inputs.
    if args.mode == "authored" and not getattr(args, "classify_all", False):
        first_row = rows[0]
        selected_bindings = _select_bindings(bindings, first_row)
        failed_expectation: dict[str, Any] | None = None
        for binding in selected_bindings:
            expectation = expectation_for(first_row, binding)
            if not expectation["passed"]:
                failed_expectation = expectation
                break
        if failed_expectation is not None:
            return {
                "report_version": 1,
                "kind": "live-byte-mode",
                "validation_mode": "live",
                "mode": args.mode,
                "tracker_revision": document.revision,
                "passed": False,
                "checked_rows": 0,
                "selected_rows": len(rows),
                "matched_groups": [],
                "build_root": None,
                "build_returncode": None,
                "build_performed": False,
                "first_divergence": {
                    "passed": False,
                    "stage": "relocation-expectations",
                    "address": normalize_address(first_row["address"]),
                    "end_exclusive": normalize_address(first_row["end_exclusive"]),
                    "scope_ids": list(first_row.get("scope_ids", [])),
                    "message": (
                        "retail relocation expectations are unresolved; the expensive authored "
                        "candidate build was not started"
                    ),
                    "expectation_report": failed_expectation,
                },
                "historical_diagnostic_only": True,
            }
    root = args.build_root or _unique_root(args.mode)
    returncode, config, paths = _run_fresh_build(args, root)
    parsed_map = None if args.mode == "object" else parse_link_map(paths.map_path)
    checked = 0
    matched_groups: list[dict[str, Any]] = []
    first_divergence: dict[str, Any] | None = None
    classifications: list[dict[str, Any]] = []
    physical_targets: dict[int, set[int]] = {}
    cleanup_physical_targets: set[int] = set()
    for row in rows:
        physical_rows = [
            item for item in row.get("physical_rows", []) if isinstance(item, Mapping)
        ]
        provider_rows = (
            [item for item in physical_rows if _is_provider_or_compiler_row(item)]
            if args.mode == "linked"
            else []
        )
        authored_rows = [item for item in physical_rows if item not in provider_rows]
        group_results: list[dict[str, Any]] = []
        if authored_rows:
            authored_group = dict(row)
            authored_group["physical_rows"] = authored_rows
            authored_group["scope_ids"] = [str(item["symbol_id"]) for item in authored_rows]
            try:
                selected_bindings = _select_bindings(bindings, authored_group)
            except LiveByteError as exc:
                if not getattr(args, "classify_all", False):
                    raise
                selected_bindings = []
                group_results.append({"passed": False, "stage": "binding", "scope_ids": list(row["scope_ids"]),
                                      "address": row["address"], "message": str(exc)})
            for binding in selected_bindings:
                try:
                    relocation_catalog: Sequence[Mapping[str, Any]] | None = None
                    if args.mode != "object":
                        expectation = expectation_for(authored_group, binding)
                        if not expectation["passed"]:
                            group_results.append(
                                {
                                    "passed": False,
                                    "stage": "relocation-expectations",
                                    "address": normalize_address(authored_group["address"]),
                                    "end_exclusive": normalize_address(
                                        authored_group["end_exclusive"]
                                    ),
                                    "scope_ids": list(authored_group["scope_ids"]),
                                    "message": (
                                        "candidate-independent retail relocation expectations are "
                                        "unresolved"
                                    ),
                                    "expectation_report": expectation,
                                }
                            )
                            break
                        relocation_catalog = expectation["expectations"]
                    group_results.append(
                        _compare_row(
                            mode=args.mode,
                            row=authored_group,
                            binding=binding,
                            config=config,
                            paths=paths,
                            reference=args.reference,
                            parsed_map=parsed_map,
                            relocation_catalog=relocation_catalog,
                            target_rows=document.collection("symbols"),
                            physical_targets=physical_targets,
                            cleanup_physical_targets=cleanup_physical_targets,
                            retail_reader_universes=(
                                retail_reader_universes_for(binding, relocation_catalog)
                                if relocation_catalog is not None
                                else None
                            ),
                        )
                    )
                except (LiveByteError, ValueError, OSError) as exc:
                    if not getattr(args, "classify_all", False):
                        raise
                    group_results.append({"passed": False, "stage": "function-proof-unavailable",
                        "scope_ids": list(authored_group["scope_ids"]), "address": row["address"],
                        "message": str(exc), "error_type": type(exc).__name__})
        assert parsed_map is not None or not provider_rows
        group_results.extend(
            _compare_provider_row(
                row=provider_row,
                paths=paths,
                reference=args.reference,
                parsed_map=parsed_map,
            )
            for provider_row in provider_rows
        )
        failed = next((item for item in group_results if not item["passed"]), None)
        comparison = failed or {
            "passed": True,
            "stage": "complete",
            "address": normalize_address(row["address"]),
            "end_exclusive": normalize_address(row["end_exclusive"]),
            "scope_ids": list(row["scope_ids"]),
            "physical_address_group": True,
            "identity_results": group_results,
        }
        checked += 1
        if getattr(args, "classify_all", False) and (checked == 1 or checked % 100 == 0):
            print(f"Function census: {checked}/{len(rows)} groups checked, {len(matched_groups)} matched", file=sys.stderr, flush=True)
        classifications.append({"scope_ids": list(row["scope_ids"]), "address": row["address"],
                               "passed": comparison["passed"], "identity_results": group_results})
        if not comparison["passed"]:
            if first_divergence is None:
                first_divergence = comparison
            if not getattr(args, "classify_all", False):
                break
            continue
        matched = _matched_group_contract(row, bindings)
        matched["match_level"] = "instruction" if any(item.get("match_level") == "instruction" for item in group_results) else "byte"
        matched["identity_results"] = group_results
        matched_groups.append(matched)
    if dependency_states(input_paths) != inputs_before:
        raise LiveByteError("production source, proof tools or retail changed during the live function comparison")
    return {
        "report_version": 1,
        "kind": "live-byte-mode",
        "validation_mode": "live",
        "mode": args.mode,
        "tracker_revision": document.revision,
        "passed": first_divergence is None,
        "checked_rows": checked,
        "selected_rows": len(rows),
        "matched_groups": matched_groups,
        "classifications": classifications,
        "match_counts": dict(Counter(item["match_level"] for item in matched_groups)),
        "build_root": root.as_posix(),
        "build_returncode": returncode,
        "build_performed": True,
        "first_divergence": first_divergence,
        "historical_diagnostic_only": True,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Freshly build and directly scan one byte-validation mode."
    )
    parser.add_argument("mode", choices=BYTE_MODES)
    parser.add_argument("--at")
    parser.add_argument("--build-root", type=Path)
    parser.add_argument("--progress", type=Path, default=DEFAULT_TRACKER)
    parser.add_argument("--manifest-dir", type=Path, default=DEFAULT_MANIFEST_DIR)
    parser.add_argument("--final-config", type=Path, default=DEFAULT_FINAL_CONFIG)
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        report = run(args)
    except (LiveByteError, RelocationExpectationError, OSError, ValueError) as exc:
        print(f"live byte validation error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"Live {report['mode']} byte scan: {'PASS' if report['passed'] else 'FAIL'}")
        print(f"- checked rows: {report['checked_rows']}/{report['selected_rows']}")
        print(f"- build root: {report['build_root']}")
        if report["first_divergence"] is not None:
            print("- first divergence: " + json.dumps(report["first_divergence"], indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
