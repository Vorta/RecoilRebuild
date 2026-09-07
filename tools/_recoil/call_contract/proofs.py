"""Immutable proof obligations and contradiction-preserving composition."""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Any, Iterable, Mapping, TypeVar


class ProofStatus(str, Enum):
    PROVEN = "proven"
    NOT_APPLICABLE = "not-applicable"
    UNRESOLVED = "unresolved"
    CONFLICT = "conflict"


def _immutable(value: Any) -> Any:
    if isinstance(value, Mapping):
        return tuple((str(key), _immutable(item)) for key, item in sorted(value.items()))
    if isinstance(value, (list, tuple)):
        return tuple(_immutable(item) for item in value)
    if value is None or isinstance(value, (str, int, bool)):
        return value
    raise TypeError(f"unsupported proof fact type: {type(value).__name__}")


@dataclass(frozen=True)
class Fact:
    side: str
    dimension: str
    value: Any
    location: str

    def __post_init__(self) -> None:
        if self.side not in {"retail", "candidate"}:
            raise ValueError("proof fact must identify retail or candidate origin")
        object.__setattr__(self, "value", _immutable(self.value))


@dataclass(frozen=True)
class Obligation:
    family: str
    expected: Fact
    required: bool = True

    def __post_init__(self) -> None:
        if self.expected.side != "retail":
            raise ValueError("only independently recovered retail facts create obligations")


@dataclass(frozen=True)
class ProofResult:
    obligation: Obligation
    observed: Fact | None
    status: ProofStatus
    reason: str = ""
    dependencies: tuple[str, ...] = ()

    def as_json(self) -> dict[str, Any]:
        expected = self.obligation.expected
        return {"family": self.obligation.family, "dimension": expected.dimension,
                "status": self.status.value, "required": self.obligation.required,
                "retail_location": expected.location,
                "candidate_location": self.observed.location if self.observed else None,
                "expected": expected.value,
                "observed": self.observed.value if self.observed else None,
                "reason": self.reason, "dependencies": list(self.dependencies)}


def prove(obligation: Obligation, observed: Fact | None,
          *, dependencies: Iterable[str] = ()) -> ProofResult:
    if observed is not None and (observed.side != "candidate"
                                or observed.dimension != obligation.expected.dimension):
        raise ValueError("candidate fact belongs to a different obligation")
    status = (ProofStatus.NOT_APPLICABLE if not obligation.required else
              ProofStatus.UNRESOLVED if observed is None else
              ProofStatus.PROVEN if observed.value == obligation.expected.value else ProofStatus.CONFLICT)
    return ProofResult(obligation, observed, status, dependencies=tuple(dependencies))


class ProofConflict(ValueError):
    def __init__(self, *, family: str, location: object, existing: object, incoming: object):
        self.diagnostic = {"family": family, "status": "conflict", "location": str(location),
                           "existing": str(existing), "incoming": str(incoming)}
        super().__init__(f"{family} proof conflict at {location}: existing={existing!r}, incoming={incoming!r}")


K = TypeVar("K")
V = TypeVar("V")


def checked_merge(*proof_sets: Mapping[K, V], family: str) -> dict[K, V]:
    result: dict[K, V] = {}
    for proof_set in proof_sets:
        for location, proof in proof_set.items():
            if location in result and result[location] != proof:
                raise ProofConflict(family=family, location=location, existing=result[location], incoming=proof)
            result[location] = proof
    return result


def merge_into(destination: dict[K, V], incoming: Mapping[K, V], *, family: str) -> None:
    """Publish a producer's conclusions atomically without precedence rules."""
    merged = checked_merge(destination, incoming, family=family)
    destination.update(merged)


class ProofMap(dict[K, V]):
    """Mutable construction of a proof set; a conclusion cannot be replaced.

    Repeated identical claims are harmless. A refined claim must be produced
    from explicit prerequisites before composition, never by map precedence.
    """

    def __init__(self, family: str, initial: Mapping[K, V] = ()):
        self.family = family
        super().__init__(initial)

    def __setitem__(self, location: K, proof: V) -> None:
        if location in self and self[location] != proof:
            raise ProofConflict(family=self.family, location=location, existing=self[location], incoming=proof)
        super().__setitem__(location, proof)

    def update(self, values: Mapping[K, V] = (), **kwargs: V) -> None:
        merged = checked_merge(self, dict(values), kwargs, family=self.family)
        super().update(merged)
