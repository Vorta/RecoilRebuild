from __future__ import annotations

from pathlib import Path
from typing import Any

from _recoil.commands.vc5_verify import (
    function_authored_order_gate,
    function_authored_relative_order_gate,
    load_manifest as load_vc5_manifest,
)
from _recoil.lib.target_binary import validated_target_binary
from _recoil.lib.tooling import REPO_ROOT, display_path


def _manifest_path(path: Path) -> str:
    resolved = path.resolve()
    try:
        return display_path(resolved.relative_to(REPO_ROOT))
    except ValueError:
        return resolved.as_posix()


def _linked_interval_registration(interval: Any) -> dict[str, Any]:
    predecessor_address = interval.predecessor.address if interval.predecessor is not None else ""
    boundary_address = (
        interval.predecessor_section_boundary.address
        if interval.predecessor_section_boundary is not None
        else ""
    )
    return {
        "name": interval.name,
        "order_scope": interval.order_scope,
        "predecessor_address": predecessor_address,
        "predecessor_section_boundary_address": boundary_address,
        "function_addresses": [function.address for function in interval.functions],
        "successor_address": interval.successor.address,
        "functions": [
            {
                "address": function.address,
                "symbol": function.symbol,
                "symbol_regex": function.symbol_regex,
                "name": function.name,
                "pipeline_class": function.pipeline_class,
                "authored_order_role": function.authored_order_role,
                "authored_order_gate": function_authored_order_gate(function),
                "authored_relative_order_gate": function_authored_relative_order_gate(function),
                "required_presence": function.required_presence,
                "full_order_gate": function.full_order_gate,
                "logical_identity_key": function.logical_identity_key,
                "icf_fold_status": function.icf_fold_status,
            }
            for function in interval.functions
        ],
        "candidate_only_extras": [
            {
                "name": extra.name,
                "pipeline_class": extra.pipeline_class,
                "symbol": extra.symbol,
                "symbol_regex": extra.symbol_regex,
            }
            for extra in interval.candidate_only_extras
        ],
    }


def vc5_target_registration(path: Path) -> tuple[str, dict[str, Any]]:
    target = load_vc5_manifest(path, enforce_source_policy=False)
    validation_addresses = [function.address for function in target.functions]
    validation_addresses.extend(symbol.address for symbol in target.data_symbols)
    for entry in target.translation_unit_function_order:
        validation_addresses.extend(function.address for function in entry.functions)
    for interval in target.linked_function_intervals:
        if interval.predecessor is not None:
            validation_addresses.append(interval.predecessor.address)
        if interval.predecessor_section_boundary is not None:
            validation_addresses.append(interval.predecessor_section_boundary.address)
        validation_addresses.extend(function.address for function in interval.functions)
        validation_addresses.append(interval.successor.address)

    target_binary = validated_target_binary(
        source_from=target.source_from,
        addresses=validation_addresses,
        explicit=target.target_binary,
        context=str(path),
    )
    target_id = f"{target_binary}:vc5-target:{target.name}"
    function_addresses = [function.address for function in target.functions]
    data_addresses = [symbol.address for symbol in target.data_symbols]
    order_addresses = [
        function.address
        for entry in target.translation_unit_function_order
        for function in entry.functions
    ]
    linked_addresses = [
        function.address
        for interval in target.linked_function_intervals
        for function in interval.functions
    ]
    registration = {
        "binary": target_binary,
        "check_function_order": target.check_function_order,
        "function_order_scope": target.function_order_scope,
        "check_translation_unit_function_order": target.check_translation_unit_function_order,
        "compiler_profile": target.compiler_profile,
        "data_addresses": data_addresses,
        "function_addresses": function_addresses,
        "functions": [
            {
                "address": function.address,
                "pipeline_class": function.pipeline_class,
                "authored_order_role": function.authored_order_role,
                "authored_order_gate": function_authored_order_gate(function),
                "authored_relative_order_gate": function_authored_relative_order_gate(function),
                "required_presence": function.required_presence,
                "full_order_gate": function.full_order_gate,
                "logical_identity_key": function.logical_identity_key,
                "icf_fold_status": function.icf_fold_status,
            }
            for function in target.functions
        ],
        "linked_function_intervals": [
            _linked_interval_registration(interval) for interval in target.linked_function_intervals
        ],
        "manifest_path": _manifest_path(target.manifest_path),
        "name": target.name,
        "source_from": target.source_from,
        "translation_unit_function_order": [
            {
                "function_addresses": [function.address for function in entry.functions],
                "source_from": entry.source_from,
                "order_scope": entry.order_scope,
                "inventory_only": entry.inventory_only,
                "functions": [
                    {
                        "address": function.address,
                        "symbol": function.symbol,
                        "symbol_regex": function.symbol_regex,
                        "name": function.name,
                        "pipeline_class": function.pipeline_class,
                        "authored_order_role": function.authored_order_role,
                        "authored_order_gate": function_authored_order_gate(function),
                        "authored_relative_order_gate": function_authored_relative_order_gate(function),
                        "required_presence": function.required_presence,
                        "full_order_gate": function.full_order_gate,
                        "logical_identity_key": function.logical_identity_key,
                        "icf_fold_status": function.icf_fold_status,
                    }
                    for function in entry.functions
                ],
                "candidate_only_extras": [
                    {
                        "name": extra.name,
                        "pipeline_class": extra.pipeline_class,
                        "symbol": extra.symbol,
                        "symbol_regex": extra.symbol_regex,
                    }
                    for extra in entry.candidate_only_extras
                ],
            }
            for entry in target.translation_unit_function_order
        ],
    }
    if target.order_edit_paths:
        registration["order_edit_paths"] = list(target.order_edit_paths)
    return target_id, {
        "binary": target_binary,
        "kind": "vc5",
        "name": target.name,
        "registration": registration,
        "registered_addresses": list(
            dict.fromkeys([*function_addresses, *data_addresses, *order_addresses, *linked_addresses])
        ),
    }


def load_target_registrations(
    *,
    vc5_manifest_dir: Path,
) -> dict[str, dict[str, Any]]:
    registrations: dict[str, dict[str, Any]] = {}
    for path in sorted(vc5_manifest_dir.glob("*.json")):
        target_id, record = vc5_target_registration(path)
        if target_id in registrations:
            raise ValueError(f"duplicate verification target id {target_id}")
        registrations[target_id] = record
    return registrations
