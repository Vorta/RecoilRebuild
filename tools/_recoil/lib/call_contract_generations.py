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


CALL_CONTRACT_VERIFIER_GENERATION = 13
NORMALIZER_REGISTRY_GENERATION = 12
EXPECTED_FACT_SCHEMA_VERSION = 12


CALL_CONTRACT_VERIFIER_COMPONENT_PATHS = frozenset(
    {
        "tools/_recoil/commands/binja_preflight.py",
        "tools/_recoil/commands/asm_verify.py",
        "tools/_recoil/commands/call_contract_convergence.py",
        "tools/_recoil/commands/call_contract_continuation.py",
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
    return [
        {"kind": kind, "path": path, "detail": detail}
        for kind, path, detail in _required_component_findings_cached(
            str(root), signature
        )
    ]


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
    "NORMALIZER_REGISTRY_COMPONENT_PATHS",
    "NORMALIZER_REGISTRY_GENERATION",
    "current_generations",
    "evidence_generations_current",
    "generation_increment_findings",
    "required_call_contract_verifier_component_findings",
    "required_call_contract_verifier_component_graph",
    "require_call_contract_verifier_components",
]
