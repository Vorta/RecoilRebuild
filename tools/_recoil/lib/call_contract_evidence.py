"""Strict JSON-native boundary for persisted call-contract evidence."""

from __future__ import annotations

import math
from typing import Any, Mapping


class CallContractEvidenceError(TypeError):
    """Raised when verifier evidence is not representable as stable JSON data."""


def json_evidence_value(value: Any, *, path: str = "$") -> Any:
    """Return an alias-free JSON-native copy, rejecting implicit coercions."""

    if value is None or isinstance(value, (str, bool, int)):
        return value
    if isinstance(value, float):
        if not math.isfinite(value):
            raise CallContractEvidenceError(f"{path}: non-finite float is not JSON evidence")
        return value
    if isinstance(value, Mapping):
        result: dict[str, Any] = {}
        for key, item in value.items():
            if not isinstance(key, str):
                raise CallContractEvidenceError(
                    f"{path}: mapping key {key!r} is not a string"
                )
            result[key] = json_evidence_value(item, path=f"{path}.{key}")
        return result
    if isinstance(value, (list, tuple)):
        return [
            json_evidence_value(item, path=f"{path}[{index}]")
            for index, item in enumerate(value)
        ]
    raise CallContractEvidenceError(
        f"{path}: {type(value).__name__} is not a JSON evidence value"
    )


__all__ = ["CallContractEvidenceError", "json_evidence_value"]
