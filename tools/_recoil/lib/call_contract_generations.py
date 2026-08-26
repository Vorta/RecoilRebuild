"""Reviewed integer generations for call-contract verification.

These values are deliberately simple schema coordinates.  They are advanced
whenever their governed implementation closure changes and are used only for
conservative invalidation; they are not derived from file contents.
"""

from __future__ import annotations

from pathlib import Path
from typing import Iterable, Mapping


CALL_CONTRACT_VERIFIER_GENERATION = 8
NORMALIZER_REGISTRY_GENERATION = 8
EXPECTED_FACT_SCHEMA_VERSION = 8


CALL_CONTRACT_VERIFIER_COMPONENT_PATHS = frozenset(
    {
        "tools/_recoil/commands/binja_preflight.py",
        "tools/_recoil/commands/asm_verify.py",
        "tools/_recoil/commands/call_contract_convergence.py",
        "tools/_recoil/commands/call_contract_readiness_audit.py",
        "tools/_recoil/commands/call_contract_verify.py",
        "tools/_recoil/commands/progress_cli.py",
        "tools/_recoil/commands/provider_function_mutation.py",
        "tools/_recoil/commands/provider_target_mutation.py",
        "tools/_recoil/commands/progress_v2.py",
        "tools/_recoil/commands/vc5_verify.py",
        "tools/_recoil/lib/binja.py",
        "tools/_recoil/lib/call_contract_generations.py",
        "tools/_recoil/lib/call_contract_normalizers.py",
        "tools/_recoil/lib/progress.py",
        "tools/_recoil/lib/progress_sqlite.py",
        "tools/_recoil/lib/reference_images.py",
        "tools/_recoil/lib/repository_paths.py",
        "tools/_recoil/lib/windows_identity.py",
    }
)
NORMALIZER_REGISTRY_COMPONENT_PATHS = frozenset(
    {
        "tools/_recoil/lib/call_contract_generations.py",
        "tools/_recoil/lib/call_contract_normalizers.py",
    }
)
EXPECTED_FACT_COMPONENT_PATHS = frozenset(
    {
        "tools/_recoil/commands/binja_preflight.py",
        "tools/_recoil/commands/call_contract_verify.py",
        "tools/_recoil/lib/binja.py",
        "tools/_recoil/lib/call_contract_generations.py",
        "tools/_recoil/lib/repository_paths.py",
        "tools/_recoil/lib/windows_identity.py",
    }
)


class GenerationError(ValueError):
    pass


def current_generations() -> dict[str, int]:
    return {
        "call_contract_verifier_generation": CALL_CONTRACT_VERIFIER_GENERATION,
        "normalizer_registry_generation": NORMALIZER_REGISTRY_GENERATION,
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
        ("normalizer_registry_generation", NORMALIZER_REGISTRY_COMPONENT_PATHS),
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


def evidence_generations_current(value: Mapping[str, object]) -> bool:
    return all(value.get(field) == expected for field, expected in current_generations().items())


__all__ = [
    "CALL_CONTRACT_VERIFIER_COMPONENT_PATHS",
    "CALL_CONTRACT_VERIFIER_GENERATION",
    "EXPECTED_FACT_COMPONENT_PATHS",
    "EXPECTED_FACT_SCHEMA_VERSION",
    "GenerationError",
    "NORMALIZER_REGISTRY_COMPONENT_PATHS",
    "NORMALIZER_REGISTRY_GENERATION",
    "current_generations",
    "evidence_generations_current",
    "generation_increment_findings",
]
