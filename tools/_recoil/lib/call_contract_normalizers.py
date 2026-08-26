"""Explicit-generation call-contract normalizer registry.

The registry records which reviewed normalizers were used for each body. Its
currentness coordinate is a reviewed integer; no implementation content
summary is created or persisted.
"""

from __future__ import annotations

from contextlib import contextmanager
from contextvars import ContextVar
from copy import deepcopy
from dataclasses import dataclass, field
from functools import wraps
import json
import re
from typing import Any, Callable, Iterator, Mapping, Sequence, TypeVar, cast

from _recoil.lib.call_contract_generations import (
    CALL_CONTRACT_VERIFIER_COMPONENT_PATHS,
    CALL_CONTRACT_VERIFIER_GENERATION,
    NORMALIZER_REGISTRY_GENERATION,
    current_generations,
)

NORMALIZER_REGISTRY_CONTRACT_VERSION = 2
NORMALIZER_COMPONENT_CONTRACT_VERSION = 2
NORMALIZER_USE_RECEIPT_CONTRACT_VERSION = 2
VERIFIER_CORE_IDENTITY_CONTRACT_VERSION = 2
NORMALIZER_USE_RECEIPT_KIND = "call-contract-normalizer-use-record"
VERIFIER_CORE_IDENTITY_KIND = "call-contract-verifier-generation"
NORMALIZER_COMPONENT_IDENTITY_KIND = "call-contract-normalizer-generation"
_COMPONENT_ID_RE = re.compile(
    r"recoil:call-contract:(?:normalizer|helper):[a-z0-9][a-z0-9._-]*:v[1-9][0-9]*"
)
_R = TypeVar("_R")


class NormalizerRegistryError(ValueError):
    pass


class NormalizerDefinitionError(NormalizerRegistryError):
    pass


class UntrackedNormalizerUseError(NormalizerRegistryError):
    pass


class NormalizerUseReceiptError(NormalizerRegistryError):
    pass


@dataclass
class BodyNormalizerUse:
    _registry: "NormalizerRegistry"
    body_id: str
    _normalizer_ids: set[str] = field(default_factory=set)
    _finished: bool = False
    _failed: bool = False

    def _record(self, normalizer_id: str) -> None:
        if self._finished:
            raise UntrackedNormalizerUseError("normalizer use occurred after body scope")
        self._normalizer_ids.add(normalizer_id)

    def receipt(self) -> dict[str, Any]:
        if not self._finished or self._failed:
            raise NormalizerUseReceiptError("normalizer-use record is unavailable")
        return {
            "kind": NORMALIZER_USE_RECEIPT_KIND,
            "contract_version": NORMALIZER_USE_RECEIPT_CONTRACT_VERSION,
            "registry_id": self._registry.registry_id,
            "registry_generation": NORMALIZER_REGISTRY_GENERATION,
            "body_id": self.body_id,
            "normalizers": [
                {
                    "normalizer_id": component_id,
                    "component_generation": NORMALIZER_REGISTRY_GENERATION,
                }
                for component_id in sorted(self._normalizer_ids)
            ],
        }


class NormalizerRegistry:
    def __init__(self, registry_id: str) -> None:
        if not isinstance(registry_id, str) or not registry_id:
            raise NormalizerDefinitionError("registry id must be non-empty")
        self.registry_id = registry_id
        self._components: dict[str, Callable[..., Any]] = {}
        self._active: ContextVar[BodyNormalizerUse | None] = ContextVar(
            f"normalizer-use:{registry_id}", default=None
        )

    def register_normalizer(
        self,
        component_id: str,
        function: Callable[..., _R],
        *,
        helper_ids: Sequence[str] = (),
    ) -> Callable[..., _R]:
        del helper_ids
        if not _COMPONENT_ID_RE.fullmatch(component_id):
            raise NormalizerDefinitionError(f"invalid normalizer id {component_id!r}")
        if component_id in self._components:
            raise NormalizerDefinitionError(f"duplicate normalizer id {component_id}")

        @wraps(function)
        def wrapped(*args: Any, **kwargs: Any) -> _R:
            active = self._active.get()
            if active is None:
                raise UntrackedNormalizerUseError(
                    f"normalizer {component_id} requires a body scope"
                )
            active._record(component_id)
            return function(*args, **kwargs)

        self._components[component_id] = cast(Callable[..., Any], wrapped)
        return wrapped

    def normalizer(
        self, component_id: str, *, helper_ids: Sequence[str] = ()
    ) -> Callable[[Callable[..., _R]], Callable[..., _R]]:
        def decorate(function: Callable[..., _R]) -> Callable[..., _R]:
            return self.register_normalizer(component_id, function, helper_ids=helper_ids)

        return decorate

    @contextmanager
    def record_body(self, body_id: str) -> Iterator[BodyNormalizerUse]:
        if not isinstance(body_id, str) or not body_id:
            raise NormalizerUseReceiptError("body id must be non-empty")
        if self._active.get() is not None:
            raise NormalizerUseReceiptError("normalizer body scopes cannot nest")
        use = BodyNormalizerUse(self, body_id)
        token = self._active.set(use)
        try:
            yield use
        except BaseException:
            use._failed = True
            raise
        finally:
            use._finished = True
            self._active.reset(token)

    def normalizer_identity(self, component_id: str) -> dict[str, Any]:
        if component_id not in self._components:
            raise NormalizerDefinitionError(f"unknown normalizer {component_id}")
        return {
            "kind": NORMALIZER_COMPONENT_IDENTITY_KIND,
            "component_id": component_id,
            "component_generation": NORMALIZER_REGISTRY_GENERATION,
        }

    def catalog_identity(self) -> dict[str, Any]:
        return {
            "kind": "call-contract-normalizer-catalog-v2",
            "registry_id": self.registry_id,
            "registry_generation": NORMALIZER_REGISTRY_GENERATION,
            "normalizers": [
                self.normalizer_identity(component_id)
                for component_id in sorted(self._components)
            ],
        }

    def validate_use_receipt(self, value: Mapping[str, Any]) -> dict[str, Any]:
        if (
            value.get("kind") != NORMALIZER_USE_RECEIPT_KIND
            or value.get("contract_version") != NORMALIZER_USE_RECEIPT_CONTRACT_VERSION
            or value.get("registry_id") != self.registry_id
            or value.get("registry_generation") != NORMALIZER_REGISTRY_GENERATION
        ):
            raise NormalizerUseReceiptError("normalizer-use record is stale or malformed")
        rows = value.get("normalizers")
        if not isinstance(rows, list):
            raise NormalizerUseReceiptError("normalizer-use rows must be a list")
        for row in rows:
            if (
                not isinstance(row, Mapping)
                or row.get("normalizer_id") not in self._components
                or row.get("component_generation") != NORMALIZER_REGISTRY_GENERATION
            ):
                raise NormalizerUseReceiptError("normalizer-use row is stale or malformed")
        return deepcopy(dict(value))

    def verifier_core_identity(self, *_args: Any, **_kwargs: Any) -> dict[str, Any]:
        return {
            "kind": VERIFIER_CORE_IDENTITY_KIND,
            "contract_version": VERIFIER_CORE_IDENTITY_CONTRACT_VERSION,
            **current_generations(),
            "component_paths": sorted(CALL_CONTRACT_VERIFIER_COMPONENT_PATHS),
        }


LIVE_CALL_CONTRACT_NORMALIZER_REGISTRY = NormalizerRegistry(
    "recoil:call-contract:normalizer-registry:live-verifier:v2"
)


@LIVE_CALL_CONTRACT_NORMALIZER_REGISTRY.normalizer(
    "recoil:call-contract:normalizer:emitted-call-rows:v2"
)
def normalize_emitted_call_rows(
    rows: Sequence[Mapping[str, Any]],
) -> list[dict[str, Any]]:
    """Return a deterministic structured copy for direct row comparison."""

    return json.loads(json.dumps([dict(row) for row in rows], sort_keys=True))


REQUIRED_CALL_CONTRACT_VERIFIER_COMPONENTS: tuple[Mapping[str, str], ...] = tuple(
    {"component_id": f"recoil:call-contract:verifier-component:{index:03d}:v2", "path": path}
    for index, path in enumerate(sorted(CALL_CONTRACT_VERIFIER_COMPONENT_PATHS), start=1)
)


def current_call_contract_verifier_components(_root: str | None = None) -> dict[str, Any]:
    return {
        "contract_version": 2,
        "verifier_generation": CALL_CONTRACT_VERIFIER_GENERATION,
        "normalizer_registry_generation": NORMALIZER_REGISTRY_GENERATION,
        "component_paths": sorted(CALL_CONTRACT_VERIFIER_COMPONENT_PATHS),
        "normalizers": [
            {
                "id": row["component_id"],
                "generation": row["component_generation"],
            }
            for row in LIVE_CALL_CONTRACT_NORMALIZER_REGISTRY.catalog_identity()["normalizers"]
        ],
    }


__all__ = [
    "BodyNormalizerUse",
    "LIVE_CALL_CONTRACT_NORMALIZER_REGISTRY",
    "NORMALIZER_COMPONENT_CONTRACT_VERSION",
    "NORMALIZER_COMPONENT_IDENTITY_KIND",
    "NORMALIZER_REGISTRY_CONTRACT_VERSION",
    "NORMALIZER_USE_RECEIPT_CONTRACT_VERSION",
    "NORMALIZER_USE_RECEIPT_KIND",
    "NormalizerDefinitionError",
    "NormalizerRegistry",
    "NormalizerRegistryError",
    "NormalizerUseReceiptError",
    "REQUIRED_CALL_CONTRACT_VERIFIER_COMPONENTS",
    "UntrackedNormalizerUseError",
    "VERIFIER_CORE_IDENTITY_CONTRACT_VERSION",
    "VERIFIER_CORE_IDENTITY_KIND",
    "current_call_contract_verifier_components",
    "normalize_emitted_call_rows",
]
