"""Reviewed integer generations for call-contract verification.

These values are deliberately simple schema coordinates.  They are advanced
whenever their governed implementation closure changes and are used only for
conservative invalidation; they are not derived from file contents.
"""

from __future__ import annotations

import ast
from functools import lru_cache
from pathlib import Path
from typing import Iterable, Mapping

from _recoil.lib.tooling import REPO_ROOT


CALL_CONTRACT_VERIFIER_GENERATION = 150
EXPECTED_FACT_SCHEMA_VERSION = 93


CALL_CONTRACT_VERIFIER_COMPONENT_PATHS = frozenset(
    {
        "tools/_recoil/commands/scoped_acceptance.py",
        "tools/_recoil/commands/owner_entry_repair.py",
        "tools/_recoil/lib/storage_proof.py",
        "tools/_recoil/lib/function_match.py",
        "tools/_recoil/lib/match_evidence.py",
        "tools/_recoil/commands/match_progress.py",
        "tools/_recoil/commands/asm_verify.py",
        "tools/_recoil/commands/call_contract_verify.py",
        "tools/_recoil/call_contract/__init__.py",
        "tools/_recoil/call_contract/abi.py",
        "tools/_recoil/call_contract/callable_identity.py",
        "tools/_recoil/call_contract/callbacks.py",
        "tools/_recoil/call_contract/candidate.py",
        "tools/_recoil/call_contract/candidate_imports.py",
        "tools/_recoil/call_contract/candidate_session.py",
        "tools/_recoil/call_contract/catalog.py",
        "tools/_recoil/call_contract/cfg.py",
        "tools/_recoil/call_contract/cli.py",
        "tools/_recoil/call_contract/comparison.py",
        "tools/_recoil/call_contract/contributions.py",
        "tools/_recoil/call_contract/current_callees.py",
        "tools/_recoil/call_contract/virtual_callees.py",
        "tools/_recoil/call_contract/allocation_callees.py",
        "tools/_recoil/call_contract/selector_callees.py",
        "tools/_recoil/call_contract/receiver_stack_loops.py",
        "tools/_recoil/call_contract/dispatch.py",
        "tools/_recoil/call_contract/errors.py",
        "tools/_recoil/call_contract/extraction.py",
        "tools/_recoil/call_contract/flow.py",
        "tools/_recoil/call_contract/iat.py",
        "tools/_recoil/call_contract/identity.py",
        "tools/_recoil/call_contract/instructions.py",
        "tools/_recoil/call_contract/lifecycle.py",
        "tools/_recoil/call_contract/listing.py",
        "tools/_recoil/call_contract/logical_identity.py",
        "tools/_recoil/call_contract/proofs.py",
        "tools/_recoil/call_contract/providers.py",
        "tools/_recoil/call_contract/receiver_candidate.py",
        "tools/_recoil/call_contract/receiver_cursor.py",
        "tools/_recoil/call_contract/receiver_equivalence.py",
        "tools/_recoil/call_contract/receiver_fields.py",
        "tools/_recoil/call_contract/receiver_instructions.py",
        "tools/_recoil/call_contract/receiver_proofs.py",
        "tools/_recoil/call_contract/receiver_retail.py",
        "tools/_recoil/call_contract/receiver_saved_this.py",
        "tools/_recoil/call_contract/receiver_storage.py",
        "tools/_recoil/call_contract/recoil_application.py",
        "tools/_recoil/call_contract/recoil_audio.py",
        "tools/_recoil/call_contract/recoil_callbacks.py",
        "tools/_recoil/call_contract/recoil_hud_construction.py",
        "tools/_recoil/call_contract/recoil_hud_fonts.py",
        "tools/_recoil/call_contract/recoil_hud_frame.py",
        "tools/_recoil/call_contract/recoil_hud_layout.py",
        "tools/_recoil/call_contract/recoil_hud_layout_active.py",
        "tools/_recoil/call_contract/recoil_hud_lifetimes.py",
        "tools/_recoil/call_contract/recoil_hud_messages.py",
        "tools/_recoil/call_contract/recoil_hud_objectives.py",
        "tools/_recoil/call_contract/recoil_hud_panels.py",
        "tools/_recoil/call_contract/recoil_hud_reticle.py",
        "tools/_recoil/call_contract/recoil_hud_sensor.py",
        "tools/_recoil/call_contract/recoil_hud_sensor_track.py",
        "tools/_recoil/call_contract/recoil_hud_shield.py",
        "tools/_recoil/call_contract/recoil_hud_stats.py",
        "tools/_recoil/call_contract/recoil_hud_timers_fonts.py",
        "tools/_recoil/call_contract/recoil_hud_visibility.py",
        "tools/_recoil/call_contract/recoil_hud_widgets.py",
        "tools/_recoil/call_contract/recoil_input.py",
        "tools/_recoil/call_contract/recoil_lifecycle.py",
        "tools/_recoil/call_contract/recoil_math.py",
        "tools/_recoil/call_contract/recoil_mfc.py",
        "tools/_recoil/call_contract/recoil_network.py",
        "tools/_recoil/call_contract/recoil_player.py",
        "tools/_recoil/call_contract/recoil_weapons.py",
        "tools/_recoil/call_contract/recoil_world.py",
        "tools/_recoil/call_contract/records.py",
        "tools/_recoil/call_contract/reporting.py",
        "tools/_recoil/call_contract/retail.py",
        "tools/_recoil/call_contract/retail_imports.py",
        "tools/_recoil/call_contract/reviewed_dispatch.py",
        "tools/_recoil/call_contract/session.py",
        "tools/_recoil/call_contract/phase_retail_storage.py",
        "tools/_recoil/call_contract/phase_retail_receivers.py",
        "tools/_recoil/call_contract/phase_candidate_iat_and_abi.py",
        "tools/_recoil/call_contract/phase_candidate_providers.py",
        "tools/_recoil/call_contract/phase_candidate_dispatch.py",
        "tools/_recoil/call_contract/phase_candidate_storage.py",
        "tools/_recoil/call_contract/phase_candidate_receivers.py",
        "tools/_recoil/call_contract/phase_candidate_lifecycle.py",
        "tools/_recoil/call_contract/phase_selected_dependencies.py",
        "tools/_recoil/call_contract/work.py",
        "tools/_recoil/call_contract/source.py",
        "tools/_recoil/call_contract/storage_identity.py",
        "tools/_recoil/call_contract/targets.py",
        "tools/_recoil/lib/implicit_copy_members.py",
        "tools/_recoil/lib/source_emission_markers.py",
        "tools/_recoil/lib/source_traceability.py",
        "tools/_recoil/commands/startup_contract.py",
        "tools/_recoil/lib/initializer_contract.py",
        "tools/_recoil/lib/path_contract.py",
        "tools/_recoil/commands/progress_cli.py",
        "tools/_recoil/commands/provider_function_mutation.py",
        "tools/_recoil/commands/provider_target_mutation.py",
        "tools/_recoil/commands/progress_v2.py",
        "tools/_recoil/commands/vc5_build.py",
        "tools/_recoil/commands/vc5_verify.py",
        "tools/_recoil/lib/binja.py",
        "tools/_recoil/lib/call_contract_evidence.py",
        "tools/_recoil/lib/authored_icf.py",
        "tools/_recoil/lib/call_argument_bits.py",
        "tools/_recoil/lib/constructor_dispatch.py",
        "tools/_recoil/lib/call_contract_generations.py",
        "tools/_recoil/lib/progress.py",
        "tools/_recoil/lib/progress_sqlite.py",
        "tools/_recoil/lib/reference_images.py",
        "tools/_recoil/lib/repository_paths.py",
        "tools/_recoil/lib/windows_identity.py",
        "tools/_recoil/lib/zeroarg_abi.py",
    }
)
EXPECTED_FACT_COMPONENT_PATHS = frozenset(
    {
        "tools/_recoil/lib/storage_proof.py",
        "tools/_recoil/lib/function_match.py",
        "tools/_recoil/lib/match_evidence.py",
        "tools/_recoil/lib/path_contract.py",
        "tools/_recoil/lib/initializer_contract.py",
        "tools/_recoil/commands/startup_contract.py",
        "tools/_recoil/commands/call_contract_verify.py",
        "tools/_recoil/call_contract/__init__.py",
        "tools/_recoil/call_contract/abi.py",
        "tools/_recoil/call_contract/callable_identity.py",
        "tools/_recoil/call_contract/callbacks.py",
        "tools/_recoil/call_contract/candidate.py",
        "tools/_recoil/call_contract/candidate_imports.py",
        "tools/_recoil/call_contract/candidate_session.py",
        "tools/_recoil/call_contract/catalog.py",
        "tools/_recoil/call_contract/cfg.py",
        "tools/_recoil/call_contract/cli.py",
        "tools/_recoil/call_contract/comparison.py",
        "tools/_recoil/call_contract/contributions.py",
        "tools/_recoil/call_contract/current_callees.py",
        "tools/_recoil/call_contract/virtual_callees.py",
        "tools/_recoil/call_contract/allocation_callees.py",
        "tools/_recoil/call_contract/selector_callees.py",
        "tools/_recoil/call_contract/receiver_stack_loops.py",
        "tools/_recoil/call_contract/dispatch.py",
        "tools/_recoil/call_contract/errors.py",
        "tools/_recoil/call_contract/extraction.py",
        "tools/_recoil/call_contract/flow.py",
        "tools/_recoil/call_contract/iat.py",
        "tools/_recoil/call_contract/identity.py",
        "tools/_recoil/call_contract/instructions.py",
        "tools/_recoil/call_contract/lifecycle.py",
        "tools/_recoil/call_contract/listing.py",
        "tools/_recoil/call_contract/logical_identity.py",
        "tools/_recoil/call_contract/proofs.py",
        "tools/_recoil/call_contract/providers.py",
        "tools/_recoil/call_contract/receiver_candidate.py",
        "tools/_recoil/call_contract/receiver_cursor.py",
        "tools/_recoil/call_contract/receiver_equivalence.py",
        "tools/_recoil/call_contract/receiver_fields.py",
        "tools/_recoil/call_contract/receiver_instructions.py",
        "tools/_recoil/call_contract/receiver_proofs.py",
        "tools/_recoil/call_contract/receiver_retail.py",
        "tools/_recoil/call_contract/receiver_saved_this.py",
        "tools/_recoil/call_contract/receiver_storage.py",
        "tools/_recoil/call_contract/recoil_application.py",
        "tools/_recoil/call_contract/recoil_audio.py",
        "tools/_recoil/call_contract/recoil_callbacks.py",
        "tools/_recoil/call_contract/recoil_hud_construction.py",
        "tools/_recoil/call_contract/recoil_hud_fonts.py",
        "tools/_recoil/call_contract/recoil_hud_frame.py",
        "tools/_recoil/call_contract/recoil_hud_layout.py",
        "tools/_recoil/call_contract/recoil_hud_layout_active.py",
        "tools/_recoil/call_contract/recoil_hud_lifetimes.py",
        "tools/_recoil/call_contract/recoil_hud_messages.py",
        "tools/_recoil/call_contract/recoil_hud_objectives.py",
        "tools/_recoil/call_contract/recoil_hud_panels.py",
        "tools/_recoil/call_contract/recoil_hud_reticle.py",
        "tools/_recoil/call_contract/recoil_hud_sensor.py",
        "tools/_recoil/call_contract/recoil_hud_sensor_track.py",
        "tools/_recoil/call_contract/recoil_hud_shield.py",
        "tools/_recoil/call_contract/recoil_hud_stats.py",
        "tools/_recoil/call_contract/recoil_hud_timers_fonts.py",
        "tools/_recoil/call_contract/recoil_hud_visibility.py",
        "tools/_recoil/call_contract/recoil_hud_widgets.py",
        "tools/_recoil/call_contract/recoil_input.py",
        "tools/_recoil/call_contract/recoil_lifecycle.py",
        "tools/_recoil/call_contract/recoil_math.py",
        "tools/_recoil/call_contract/recoil_mfc.py",
        "tools/_recoil/call_contract/recoil_network.py",
        "tools/_recoil/call_contract/recoil_player.py",
        "tools/_recoil/call_contract/recoil_weapons.py",
        "tools/_recoil/call_contract/recoil_world.py",
        "tools/_recoil/call_contract/records.py",
        "tools/_recoil/call_contract/reporting.py",
        "tools/_recoil/call_contract/retail.py",
        "tools/_recoil/call_contract/retail_imports.py",
        "tools/_recoil/call_contract/reviewed_dispatch.py",
        "tools/_recoil/call_contract/session.py",
        "tools/_recoil/call_contract/phase_retail_storage.py",
        "tools/_recoil/call_contract/phase_retail_receivers.py",
        "tools/_recoil/call_contract/phase_candidate_iat_and_abi.py",
        "tools/_recoil/call_contract/phase_candidate_providers.py",
        "tools/_recoil/call_contract/phase_candidate_dispatch.py",
        "tools/_recoil/call_contract/phase_candidate_storage.py",
        "tools/_recoil/call_contract/phase_candidate_receivers.py",
        "tools/_recoil/call_contract/phase_candidate_lifecycle.py",
        "tools/_recoil/call_contract/phase_selected_dependencies.py",
        "tools/_recoil/call_contract/work.py",
        "tools/_recoil/call_contract/source.py",
        "tools/_recoil/call_contract/storage_identity.py",
        "tools/_recoil/call_contract/targets.py",
        "tools/_recoil/lib/implicit_copy_members.py",
        "tools/_recoil/lib/source_emission_markers.py",
        "tools/_recoil/lib/source_traceability.py",
        "tools/_recoil/lib/authored_icf.py",
        "tools/_recoil/lib/binja.py",
        "tools/_recoil/lib/call_contract_evidence.py",
        "tools/_recoil/lib/call_argument_bits.py",
        "tools/_recoil/lib/constructor_dispatch.py",
        "tools/_recoil/lib/call_contract_generations.py",
        "tools/_recoil/lib/repository_paths.py",
        "tools/_recoil/lib/windows_identity.py",
    }
)


class GenerationError(ValueError):
    pass


def required_call_contract_verifier_component_graph() -> tuple[dict[str, str], ...]:
    """Return the single reviewed deterministic verifier-component graph."""

    return tuple(
        {
            "component_id": (
                f"recoil:call-contract:verifier-component:{index:03d}:v2"
            ),
            "path": path,
        }
        for index, path in enumerate(
            sorted(CALL_CONTRACT_VERIFIER_COMPONENT_PATHS), start=1
        )
    )


@lru_cache(maxsize=16)
def _required_component_findings_cached(
    repository_root_text: str,
    _filesystem_signature: tuple[tuple[object, ...], ...],
) -> tuple[tuple[str, str, str], ...]:
    root = Path(repository_root_text)
    findings: list[tuple[str, str, str]] = []
    for row in required_call_contract_verifier_component_graph():
        relative = row["path"]
        path = root / Path(*relative.split("/"))
        try:
            exists = path.exists()
            is_file = path.is_file()
        except OSError as exc:
            findings.append(("unreadable", relative, str(exc)))
            continue
        if not exists or not is_file:
            findings.append(("missing", relative, "required component is absent"))
            continue
        try:
            source = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as exc:
            findings.append(("unreadable", relative, str(exc)))
            continue
        try:
            ast.parse(source, filename=relative)
        except SyntaxError as exc:
            location = f"line {exc.lineno}" if exc.lineno is not None else "unknown line"
            findings.append(("unparseable", relative, f"{location}: {exc.msg}"))
    return tuple(findings)


def _required_component_filesystem_signature(
    root: Path,
) -> tuple[tuple[object, ...], ...]:
    """Build a transient stat key so cached parsing cannot hide disappearance."""

    signature: list[tuple[object, ...]] = []
    for row in required_call_contract_verifier_component_graph():
        relative = row["path"]
        path = root / Path(*relative.split("/"))
        try:
            stat = path.stat()
        except OSError as exc:
            signature.append((relative, "error", type(exc).__name__, str(exc)))
        else:
            signature.append(
                (
                    relative,
                    "stat",
                    stat.st_mode,
                    stat.st_size,
                    stat.st_mtime_ns,
                )
            )
    return tuple(signature)


def required_call_contract_verifier_component_findings(
    repository_root: str | Path | None = None,
) -> list[dict[str, str]]:
    """Check every registered verifier component for exact operational presence."""

    root = Path(repository_root or REPO_ROOT).absolute()
    signature = _required_component_filesystem_signature(root)
    findings = [
        {"kind": kind, "path": path, "detail": detail}
        for kind, path, detail in _required_component_findings_cached(
            str(root), signature
        )
    ]
    package_root = root / "tools/_recoil/call_contract"
    for component in sorted(package_root.rglob("*.py")):
        relative = component.relative_to(root).as_posix()
        if relative not in CALL_CONTRACT_VERIFIER_COMPONENT_PATHS or relative not in EXPECTED_FACT_COMPONENT_PATHS:
            findings.append({"kind": "ungoverned-component", "path": relative,
                             "detail": "private proof component lacks explicit verifier and expected-fact generation ownership"})
    return findings


def require_call_contract_verifier_components(
    repository_root: str | Path | None = None,
) -> tuple[dict[str, str], ...]:
    findings = required_call_contract_verifier_component_findings(repository_root)
    if findings:
        first = findings[0]
        raise GenerationError(
            "required call-contract verifier component is "
            f"{first['kind']}: {first['path']}: {first['detail']}"
        )
    return required_call_contract_verifier_component_graph()


def current_generations() -> dict[str, int]:
    return {
        "call_contract_verifier_generation": CALL_CONTRACT_VERIFIER_GENERATION,
        "expected_fact_schema_version": EXPECTED_FACT_SCHEMA_VERSION,
    }


def _normalized_paths(paths: Iterable[str | Path]) -> set[str]:
    normalized: set[str] = set()
    for path in paths:
        value = str(path).replace("\\", "/")
        while value.startswith("./"):
            value = value[2:]
        normalized.add(value)
    return normalized


def generation_increment_findings(
    changed_paths: Iterable[str | Path],
    previous: Mapping[str, int],
) -> list[str]:
    """Check that a reviewed component edit advances its owning coordinate."""

    changed = _normalized_paths(changed_paths)
    current = current_generations()
    requirements = (
        (
            "call_contract_verifier_generation",
            CALL_CONTRACT_VERIFIER_COMPONENT_PATHS,
        ),
        ("expected_fact_schema_version", EXPECTED_FACT_COMPONENT_PATHS),
    )
    findings: list[str] = []
    for field, closure in requirements:
        touched = sorted(changed & closure, key=str.casefold)
        if not touched:
            continue
        prior = previous.get(field)
        if not isinstance(prior, int) or isinstance(prior, bool):
            findings.append(f"{field}: previous integer value is required")
        elif current[field] <= prior:
            findings.append(
                f"{field}: component changes require an increment; touched "
                + ", ".join(touched)
            )
    return findings


def evidence_generations_current(
    value: Mapping[str, object],
) -> bool:
    return all(
        value.get(field) == expected
        for field, expected in current_generations().items()
    )


__all__ = [
    "CALL_CONTRACT_VERIFIER_COMPONENT_PATHS",
    "CALL_CONTRACT_VERIFIER_GENERATION",
    "EXPECTED_FACT_COMPONENT_PATHS",
    "EXPECTED_FACT_SCHEMA_VERSION",
    "GenerationError",
    "current_generations",
    "evidence_generations_current",
    "generation_increment_findings",
    "required_call_contract_verifier_component_findings",
    "required_call_contract_verifier_component_graph",
    "require_call_contract_verifier_components",
]
