"""Current VC5 callee return effects used only for candidate stack lineage."""
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Mapping

from _recoil.call_contract import candidate as candidates
from _recoil.call_contract import cfg, identity, listing, receiver_candidate
from _recoil.commands.asm_verify import IMAGE_REL_I386_REL32
from _recoil.commands.vc5_verify import load_manifest
from _recoil.lib.tooling import REPO_ROOT


def exact_callee_return_cleanup(definition: Any) -> int | None:
    """Require complete current COFF/COD agreement and one return effect.

    Neither the callee's retail identity nor its declared convention supplies
    the result. Unknown control flow, tail exits and conflicting RET immediates
    leave the stack effect unknown.
    """
    rows = definition.instructions
    if (not rows or not definition.section_is_comdat
            or definition.comdat_selection is None
            or definition.section_external_functions != (definition.symbol,)
            or definition.section_size != len(definition.data)):
        return None
    addresses = cfg._instruction_runtime_addresses(rows, source="cod", caller_start=0)
    if (len(set(addresses)) != len(addresses) or None in addresses
            or not receiver_candidate._candidate_listing_matches_coff(
                rows, addresses=addresses, caller_start=0, definition=definition)):
        return None
    code_end = addresses[-1] + len(rows[-1].bytes)
    padding = definition.data[code_end:]
    if padding and (len(padding) > 15 or len(definition.data) % 16
            or any(value not in {0x90, 0xCC} for value in padding)
            or any(item.offset >= code_end for item in definition.relocations)):
        return None
    for row, address in zip(rows, addresses):
        mnemonic = cfg._instruction_mnemonic(row)
        if (mnemonic.startswith(("j", "loop", "ret"))
                and any(address <= item.offset < address + len(row.bytes)
                        for item in definition.relocations)):
            return None
    successors, unresolved = cfg._exact_invocation_cfg(rows,
        instruction_addresses=addresses,
        instruction_index_by_address={at: index for index, at in enumerate(addresses)},
        source="cod", caller_start=0, caller_end=len(definition.data),
        local_control_flow_indices=definition.local_control_flow_indices,
        local_control_flow_targets=definition.local_control_flow_targets)
    reachable = cfg._reachable_cfg_indices(successors, (0,))
    terminals = [index for index in reachable if not successors.get(index, ())]
    if unresolved & reachable or not terminals:
        return None
    cleanups = {identity._exact_ret_cleanup_bytes(rows[index]) for index in terminals}
    if None in cleanups or len(cleanups) != 1:
        return None
    return next(iter(cleanups))


def candidate_direct_cleanup_map(instructions, caller, definitions) -> dict[int, int]:
    """Bind each callee effect to its exact current caller E8/REL32 operand."""
    if caller is None or not definitions:
        return {}
    addresses = cfg._instruction_runtime_addresses(instructions, source="cod", caller_start=0)
    if not receiver_candidate._candidate_listing_matches_coff(
            instructions, addresses=addresses, caller_start=0, definition=caller):
        return {}
    result = {}
    cache = {}
    for index, (row, offset) in enumerate(zip(instructions, addresses)):
        if cfg._instruction_mnemonic(row) != "call" or bytes.fromhex(" ".join(row.bytes)) != b"\xe8\0\0\0\0":
            continue
        name = cfg._instruction_operand(row).strip()
        definition = definitions.get(name)
        references = [item for item in caller.relocations if offset <= item.offset < offset + 5]
        symbols = [item for item in caller.coff_symbols if item.name == name]
        if (definition is None or definition.symbol != name or len(references) != 1
                or len(symbols) != 1 or references[0].offset != offset + 1
                or references[0].type != IMAGE_REL_I386_REL32
                or references[0].symbol_name != name
                or references[0].symbol_index != symbols[0].index
                or not all(caller.relocation_mask[offset + 1:offset + 5])):
            continue
        if name not in cache:
            cache[name] = exact_callee_return_cleanup(definition)
        cleanup = cache[name]
        if cleanup is not None:
            result[index] = cleanup
    return result


class CalleeDefinitions(dict):
    """Invocation-local functions with their native table/COFF dependencies."""
    def __init__(self, functions, table_units):
        super().__init__(functions)
        self.table_units = tuple(table_units)


def same_current_callee_definition(left, right):
    """Object-local symbol indices do not identify relocation targets across TUs."""
    def relocations(definition):
        return tuple((row.offset, row.type, row.symbol_name) for row in definition.relocations)
    return (left is not None and right is not None and left.symbol == right.symbol
            and left.data == right.data and relocations(left) == relocations(right))


def compatible_current_callee_return_effect(left, right):
    """Different current implementations may establish the same stack effect.

    Each complete COFF/COD body must prove that effect independently. This
    joins only its return ABI; it establishes no body, target or data equality.
    """
    if left is None or right is None or left.symbol != right.symbol:
        return False
    effect = exact_callee_return_cleanup(left)
    return effect is not None and effect == exact_callee_return_cleanup(right)


@dataclass
class CurrentCalleeDefinitions:
    """Lazily compile registered source routes inside the live dependency set.

    These extra definitions never become selected bodies, provider evidence or
    acceptance results. They are isolated from TU-local provider inventories.
    The owning invocation checks the source closure and compiler stability.
    """
    document: Any
    dependency_paths: frozenset[str]
    build_root: Path
    vc5_env: Path
    toolchain_receipts: list
    _routes: Any = field(default=None, init=False)
    _definitions: dict = field(default_factory=dict, init=False)
    _compiled_targets: set = field(default_factory=set, init=False)
    _table_units: list = field(default_factory=list, init=False)
    _listing_index: Any = field(default_factory=listing._CodListingInvocationIndex, init=False)

    def _index_routes(self):
        routes = {}
        active_targets = {block.get("accepted_order_facts", {}).get("target_id")
            for block in self.document.collection("physical_blocks").values()
            if isinstance(block.get("accepted_order_facts"), Mapping)}
        registrations = self.document.collection("verification_targets")
        for target_id in sorted(value for value in active_targets if value):
            registration_row = registrations[target_id]
            registration = registration_row.get("registration", {})
            path = registration.get("manifest_path")
            if registration_row.get("kind") != "vc5" or not path:
                continue
            target = load_manifest(REPO_ROOT / path)
            if target.target_binary != "recoil":
                continue
            units = target.translation_unit_function_order
            sources = {unit.source_from for unit in units} if units else {target.source_from}
            if not sources or not sources <= self.dependency_paths:
                continue
            # Inline authored entries may exist only in a target's selected
            # translation-unit order population, not its top-level functions.
            functions = (*target.functions, *(function for unit in units for function in unit.functions))
            for function in functions:
                owners = [unit.source_from for unit in units if any(row.address == function.address for row in unit.functions)]
                source = owners[0] if len(owners) == 1 else target.source_from if not units else ""
                if source and function.symbol:
                    routes.setdefault(function.symbol, {}).setdefault(source, target)
        self._routes = {name: next(iter(by_source.values())) for name, by_source in routes.items() if len(by_source) == 1}

    def for_caller(self, candidate) -> Mapping[str, Any]:
        if not candidate.instructions or candidate.caller_definition is None:
            return {}
        # An external return effect is needed only for a receiver-bearing body
        # which actually reads stack storage. Local definitions cost no build.
        if not (any(cfg._instruction_mnemonic(row) == "call" and "[" in cfg._instruction_operand(row)
                    for row in candidate.instructions)
                and any("[esp" in cfg._instruction_operand(row).lower() for row in candidate.instructions)):
            return candidate.tu_local_function_definitions
        if self._routes is None:
            self._index_routes()
        selected = dict(candidate.tu_local_function_definitions)
        names = {cfg._instruction_operand(row).strip() for row in candidate.instructions
                 if cfg._instruction_mnemonic(row) == "call"}
        if candidate.caller_definition.symbol.startswith('?RebuildBindingSlotWidgets@HudCmdBindButtonBase@@'):
            names.add('?SetPos@HudUiElement@@UAEXHH@Z')
        if candidate.caller_definition.symbol.startswith(
                ('?LoadFromZrd@HudUiCheckToggleWidget@@', '?LoadFromZrd@HudUiZrdWidget@@')):
            names.update({'?SetPos@HudUiElement@@UAEXHH@Z', '?SetVisible@HudUiElement@@UAEXH@Z'})
        if candidate.caller_definition.symbol.startswith('?LoadFromZrd@HudCmdBindButtonBase@@'):
            names.add('?SetVisible@HudUiElement@@UAEXH@Z')
        if candidate.caller_definition.symbol.startswith("?PushLine@HudUiTextStack4@@"):
            # Dependency discovery only. The virtual-call proof must separately
            # bind the native tables, exact receiver and current return effects.
            names.update({"?SetEnabled@HudUiContainer@@UAEXH@Z",
                          "?SetVisible@HudUiElement@@UAEXH@Z",
                          "?SetTextFmt@HudUiPanel@@UAAXPBDZZ"})
        for name in sorted(names - selected.keys()):
            target = self._routes.get(name)
            if target is None:
                continue
            if target.name not in self._compiled_targets:
                units = candidates._compile_call_contract_target_units(target,
                    build_root=self.build_root / "current-callee-definitions", vc5_env=self.vc5_env,
                    toolchain_receipts=self.toolchain_receipts)
                for _unit, cod, coff in units:
                    token = listing._ACTIVE_COD_LISTING_INDEX.set(self._listing_index)
                    try:
                        definitions = candidates._candidate_tu_local_function_definitions(coff, cod)
                    finally:
                        listing._ACTIVE_COD_LISTING_INDEX.reset(token)
                    self._table_units.append((candidates._candidate_vftable_definitions(coff),
                        candidates._candidate_coff_symbol_definitions(coff), definitions))
                    for symbol, definition in definitions.items():
                        # Private constant labels and implementation details
                        # can differ across TUs. Retain a representative only
                        # when every current variant proves the same return
                        # effect; no body or relocation equality follows.
                        prior = self._definitions.get(symbol)
                        if (symbol in self._definitions
                                and not same_current_callee_definition(prior, definition)
                                and not compatible_current_callee_return_effect(prior, definition)):
                            self._definitions[symbol] = None
                        else:
                            self._definitions[symbol] = definition
                self._compiled_targets.add(target.name)
            definition = self._definitions.get(name)
            if definition is not None:
                selected[name] = definition
        # Native tables can select a companion-TU body absent from the caller's
        # direct operands. Retain every unambiguous fresh definition; table
        # lookup still proves its exact relocation target and current cleanup.
        # The caller's own TU definition remains authoritative when present.
        for name in self._definitions.keys() - selected.keys():
            definition = self._definitions[name]
            if definition is not None:
                selected[name] = definition
        return CalleeDefinitions(selected, self._table_units)
