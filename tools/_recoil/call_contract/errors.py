"""Recoil call-contract errors evidence and checks."""

from __future__ import annotations

from copy import deepcopy
from typing import Any, Mapping, Sequence

from _recoil.lib.cpp_definition_closure import DecodedCallableIdentity
from _recoil.lib.progress import normalize_address


class CallContractBodyError(ValueError):
    def __init__(
        self,
        *,
        symbol_id: str,
        address: str,
        side: str,
        message: str,
    ) -> None:
        super().__init__(message)
        self.symbol_id = symbol_id
        self.address = address
        self.side = side


class CandidateCallTargetBridgeError(ValueError):
    """Exact candidate-observed decorated identity needing owner routing."""

    def __init__(
        self,
        *,
        decorated_identity: str,
        decoded_identity: DecodedCallableIdentity | None,
        call_site_address: str | int | None,
        message: str,
        call_site_unique: bool | None = None,
    ) -> None:
        super().__init__(message)
        self.decorated_identity = decorated_identity
        self.decoded_identity = decoded_identity
        self.call_site_address = (
            normalize_address(call_site_address)
            if call_site_address is not None
            else None
        )
        self.call_site_unique = call_site_unique is True


class CandidateCallContractEvidenceError(ValueError):
    """Candidate-only COFF/COD proof failure discovered before comparison."""


class CandidateCallbackAuthorityError(CandidateCallContractEvidenceError):
    """Exact callback identity whose reviewed authority package is incomplete."""

    def __init__(self, *, provenance: Mapping[str, Any], message: str) -> None:
        super().__init__(message)
        self.provenance = deepcopy(dict(provenance))


class CandidateBodyIdentityAcquisitionError(ValueError):
    """Typed body-ABI mismatch with bounded header/definition provenance."""

    def __init__(
        self,
        *,
        target_id: str,
        routes: Sequence[Mapping[str, Any]],
    ) -> None:
        declaration_paths = {
            str(path)
            for route in routes
            if route.get("routing_mode") == "exact"
            for path in route.get("declaration_paths", [])
        }
        definition_paths = {
            str(path)
            for route in routes
            if route.get("routing_mode") == "exact"
            for path in route.get("definition_paths", [])
        }
        exact_paths = declaration_paths | definition_paths
        route_keys: list[tuple[str, str, str]] = []
        exact_route_partition = bool(routes)
        if exact_route_partition:
            for route in routes:
                expected = (
                    route.get("expected_identity")
                    if isinstance(route, Mapping)
                    else None
                )
                route_declarations = (
                    route.get("declaration_paths")
                    if isinstance(route, Mapping)
                    else None
                )
                route_definitions = (
                    route.get("definition_paths")
                    if isinstance(route, Mapping)
                    else None
                )
                route_sources = (
                    route.get("source_edit_paths")
                    if isinstance(route, Mapping)
                    else None
                )
                route_key = (
                    str(route.get("symbol_id", ""))
                    if isinstance(route, Mapping)
                    else "",
                    str(route.get("address", ""))
                    if isinstance(route, Mapping)
                    else "",
                    str(expected.get("decorated_identity", ""))
                    if isinstance(expected, Mapping)
                    else "",
                )
                route_keys.append(route_key)
                if not (
                    isinstance(route, Mapping)
                    and route.get("target_id") == target_id
                    and route.get("routing_mode") == "exact"
                    and isinstance(route_declarations, list)
                    and len(route_declarations) == 1
                    and isinstance(route_declarations[0], str)
                    and route_declarations[0]
                    and isinstance(route_definitions, list)
                    and len(route_definitions) == 1
                    and isinstance(route_definitions[0], str)
                    and route_definitions[0]
                    and isinstance(route_sources, list)
                    and set(route_sources)
                    == set(route_declarations) | set(route_definitions)
                    and len(route_sources) == 2
                    and len(
                        {str(path).casefold() for path in route_sources}
                    )
                    == 2
                ):
                    exact_route_partition = False
                    break
        exact_route_partition = bool(
            exact_route_partition
            and all(all(value) for value in route_keys)
            and len(route_keys) == len(set(route_keys))
        )
        exact = bool(
            exact_route_partition
            and declaration_paths
            and definition_paths
            and len({path.casefold() for path in exact_paths})
            == len(exact_paths)
        )
        self.target_id = target_id
        self.provenance = {
            "kind": "call-contract-candidate-body-declaration-header-routing",
            "contract_version": 1,
            "target_id": target_id,
            "routing_mode": "exact" if exact else "blocked",
            "source_edit_paths": sorted(exact_paths) if exact else [],
            "declaration_paths": (
                sorted(declaration_paths) if exact else []
            ),
            "definition_paths": sorted(definition_paths) if exact else [],
            "routes": [deepcopy(dict(route)) for route in routes],
            "expected_identity_role": (
                "candidate-independent-registered-retail-body-identity"
            ),
            "candidate_identity_role": (
                "candidate-coff-cod-repair-provenance-only"
            ),
            "candidate_expected_truth": False,
            "ambiguity_policy": "fail-closed",
        }
        identities = ", ".join(
            str(route.get("expected_identity", {}).get("decorated_identity", ""))
            for route in routes
        )
        super().__init__(
            "call-contract candidate body ABI/decorated identity acquisition "
            "requires coordinated declaration-header/definition routing for "
            f"{target_id}: {identities}"
        )
