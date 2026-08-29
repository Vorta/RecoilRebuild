from __future__ import annotations

from dataclasses import dataclass
import json
import os
from pathlib import Path
import tempfile
from typing import Any, Mapping
from urllib.parse import urlencode
from urllib.request import urlopen

DEFAULT_BRIDGE_URL = "http://127.0.0.1:9009"
DEFAULT_BN_CALL_BUDGET = 200
BN_CALL_BUDGET_FILE_ENV = "RECOIL_BN_CALL_BUDGET_FILE"
GOVERNED_READ_PLAN_SCHEMA = "recoil-governed-binja-read-plan-v1"
GOVERNED_READ_PLAN_MAX_BYTES = 64 * 1024
GOVERNED_READ_PLAN_MAX_REQUESTS = 256
GOVERNED_TRANSCRIPT_MAX_BYTES = 4 * 1024 * 1024
GOVERNED_HEXDUMP_MAX_BYTES = 1024 * 1024
GOVERNED_SELECTOR_OVERRIDE_KEYS = frozenset(
    {
        "binary",
        "binary_name",
        "database",
        "database_path",
        "file",
        "filename",
        "saved_view",
        "target",
        "target_binary",
        "view",
    }
)


@dataclass(frozen=True)
class Symbol:
    address: str
    name: str
    raw_name: str = ""
    full_name: str = ""
    kind: str = "function"


@dataclass(frozen=True)
class DataVariable:
    address: str
    name: str
    raw_name: str
    type_text: str
    size: int


class BridgeError(RuntimeError):
    pass


class BridgeBudgetExceeded(BridgeError):
    pass


@dataclass(frozen=True)
class BridgeBudgetState:
    used: int
    limit: int

    @property
    def remaining(self) -> int:
        if self.limit <= 0:
            return 2**31 - 1
        return max(0, self.limit - self.used)


@dataclass(frozen=True)
class BinaryNinjaSnapshot:
    """Trustworthy provider-owned BN snapshot identity, when available.

    A path, file timestamp, bridge URL, or locally computed query hash is not
    a Binary Ninja database revision and is deliberately never substituted.
    """

    available: bool
    generation_token: str = ""
    revision: str = ""
    reason: str = ""
    schema: str = ""
    authenticated: bool = False
    provider: str = ""
    capability_version: str = ""
    saved_view: str = ""

    def as_dict(self) -> dict[str, object]:
        return {
            "available": self.available,
            "generation_token": self.generation_token,
            "revision": self.revision,
            "reason": self.reason,
            "schema": self.schema,
            "authenticated": self.authenticated,
            "provider": self.provider,
            "capability_version": self.capability_version,
            "saved_view": self.saved_view,
        }


AUTHENTICATED_SNAPSHOT_SCHEMA = "recoil-binja-authenticated-snapshot-v2"
AUTHENTICATED_SNAPSHOT_CAPABILITY_VERSION = "2"
MAINTAINED_RECOIL_SAVED_VIEW = "Recoil.bndb"
AUTHENTICATED_RECOIL_SNAPSHOT_RECEIPT_FIELDS = frozenset(
    {
        "saved_view",
        "generation_token",
        "revision",
        "schema",
        "authenticated",
        "provider",
        "capability_version",
    }
)


def validate_authenticated_recoil_snapshot_receipt(
    value: object,
    *,
    stage: str,
) -> dict[str, object]:
    """Validate the exact persisted authenticated Recoil snapshot receipt.

    This is the single mapping boundary used for both freshly produced typed
    snapshots and persisted/currentness evidence.  Shape-only legacy mappings,
    projections with extra fields, and caller-invented availability flags are
    intentionally rejected.
    """

    label = str(stage).strip() or "provider"
    if not isinstance(value, Mapping):
        raise BridgeError(
            f"Binary Ninja {label} snapshot receipt is not a mapping"
        )
    keys = set(value)
    if keys != AUTHENTICATED_RECOIL_SNAPSHOT_RECEIPT_FIELDS:
        missing = sorted(AUTHENTICATED_RECOIL_SNAPSHOT_RECEIPT_FIELDS - keys)
        extra = sorted(keys - AUTHENTICATED_RECOIL_SNAPSHOT_RECEIPT_FIELDS)
        detail: list[str] = []
        if missing:
            detail.append("missing " + ", ".join(missing))
        if extra:
            detail.append("unexpected " + ", ".join(extra))
        raise BridgeError(
            f"Binary Ninja {label} snapshot receipt has the wrong exact shape: "
            + "; ".join(detail)
        )

    string_fields = (
        "saved_view",
        "generation_token",
        "revision",
        "schema",
        "provider",
        "capability_version",
    )
    wrong_types = [
        field for field in string_fields if not isinstance(value.get(field), str)
    ]
    if wrong_types or not isinstance(value.get("authenticated"), bool):
        fields = [*wrong_types]
        if not isinstance(value.get("authenticated"), bool):
            fields.append("authenticated")
        raise BridgeError(
            f"Binary Ninja {label} snapshot receipt has wrong field types: "
            + ", ".join(fields)
        )
    if (
        value.get("schema") != AUTHENTICATED_SNAPSHOT_SCHEMA
        or value.get("authenticated") is not True
        or value.get("provider") != "binary-ninja"
        or value.get("capability_version")
        != AUTHENTICATED_SNAPSHOT_CAPABILITY_VERSION
    ):
        raise BridgeError(
            f"Binary Ninja {label} snapshot receipt lacks the typed "
            "authenticated provider capability"
        )
    required = {
        "provider": value["provider"],
        "generation token": value["generation_token"],
        "revision": value["revision"],
        "saved view": value["saved_view"],
    }
    missing_values = [
        name for name, item in required.items() if not str(item).strip()
    ]
    if missing_values:
        raise BridgeError(
            f"Binary Ninja {label} snapshot receipt is incomplete: missing "
            + ", ".join(missing_values)
        )
    if value.get("saved_view") != MAINTAINED_RECOIL_SAVED_VIEW:
        raise BridgeError(
            f"Binary Ninja {label} snapshot receipt is for "
            f"{value.get('saved_view')!r}, not {MAINTAINED_RECOIL_SAVED_VIEW!r}"
        )
    return {field: value[field] for field in (
        "saved_view",
        "generation_token",
        "revision",
        "schema",
        "authenticated",
        "provider",
        "capability_version",
    )}


def require_authenticated_recoil_snapshot(
    value: object,
    *,
    stage: str,
) -> dict[str, object]:
    """Validate one provider-owned snapshot through the shared strict contract.

    Only the typed value returned by :class:`BinaryNinjaBridge` is accepted.
    In particular, caller-provided mappings, path/mtime stand-ins, legacy
    availability booleans, and initial-token-only projections cannot cross this
    boundary. Both expected-fact production and standalone
    governed readers use this function so their trust requirements cannot
    drift independently.
    """

    label = str(stage).strip() or "provider"
    if not isinstance(value, BinaryNinjaSnapshot):
        raise BridgeError(
            f"Binary Ninja {label} snapshot is not a typed provider snapshot"
        )
    if value.available is not True:
        raise BridgeError(
            f"Binary Ninja {label} snapshot unavailable: "
            f"{value.reason or 'authenticated snapshot capability unavailable'}"
        )
    return validate_authenticated_recoil_snapshot_receipt({
        "saved_view": value.saved_view,
        "generation_token": value.generation_token,
        "revision": value.revision,
        "schema": value.schema,
        "authenticated": value.authenticated,
        "provider": value.provider,
        "capability_version": value.capability_version,
    }, stage=label)


GOVERNED_READ_ENDPOINTS = frozenset(
    {
        "assembly",
        "binaries",
        "codeReferences",
        "data",
        "dataReferences",
        "functionAt",
        "functionInfo",
        "functionReturnRegs",
        "getStackFrameVars",
        "getXrefsTo",
        "get_xrefs_to",
        "il",
        "imports",
        "methods",
        "references",
        "sections",
        "status",
        "xrefs",
    }
)
_GOVERNED_RECEIPT_SEAL = object()


@dataclass(frozen=True, init=False)
class GovernedBinaryNinjaReadReceipt:
    packet_id: str
    reservation_id: str
    reference_image: Mapping[str, str]
    issue_ledger_identity: Mapping[str, object]
    begin_snapshot: Mapping[str, object]
    end_snapshot: Mapping[str, object]
    fact_reads: tuple[Mapping[str, object], ...]
    resource_claims: tuple[Mapping[str, object], ...]
    nonaccepting: bool
    candidate_expected_truth: bool

    def __init__(
        self,
        *,
        _seal: object,
        packet_id: str,
        reservation_id: str,
        reference_image: Mapping[str, str],
        issue_ledger_identity: Mapping[str, object],
        begin_snapshot: Mapping[str, object],
        end_snapshot: Mapping[str, object],
        fact_reads: tuple[Mapping[str, object], ...],
        resource_claims: tuple[Mapping[str, object], ...],
    ) -> None:
        if _seal is not _GOVERNED_RECEIPT_SEAL:
            raise BridgeError(
                "governed Binary Ninja receipts are constructed only by a completed session"
            )
        object.__setattr__(self, "packet_id", packet_id)
        object.__setattr__(self, "reservation_id", reservation_id)
        object.__setattr__(self, "reference_image", dict(reference_image))
        object.__setattr__(self, "issue_ledger_identity", dict(issue_ledger_identity))
        object.__setattr__(self, "begin_snapshot", dict(begin_snapshot))
        object.__setattr__(self, "end_snapshot", dict(end_snapshot))
        object.__setattr__(self, "fact_reads", tuple(dict(row) for row in fact_reads))
        object.__setattr__(self, "resource_claims", tuple(dict(row) for row in resource_claims))
        object.__setattr__(self, "nonaccepting", True)
        object.__setattr__(self, "candidate_expected_truth", False)

    def as_dict(self) -> dict[str, object]:
        return {
            "packet_id": self.packet_id,
            "reservation_id": self.reservation_id,
            "reference_image": dict(self.reference_image),
            "issue_ledger_identity": dict(self.issue_ledger_identity),
            "begin_snapshot": dict(self.begin_snapshot),
            "end_snapshot": dict(self.end_snapshot),
            "snapshot_equal": True,
            "fact_reads": [dict(row) for row in self.fact_reads],
            "fact_read_count": len(self.fact_reads),
            "resource_claims": [dict(row) for row in self.resource_claims],
            "nonaccepting": True,
            "candidate_expected_truth": False,
        }


class BinaryNinjaBridge:
    def __init__(
        self,
        base_url: str = DEFAULT_BRIDGE_URL,
        timeout: float = 15.0,
        *,
        binary: str | None = None,
        call_budget: int = DEFAULT_BN_CALL_BUDGET,
        budget_file: str | Path | None = None,
        use_environment_budget_file: bool = True,
    ) -> None:
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self.binary = binary
        self.call_budget = call_budget
        environment_budget_file = (
            os.environ.get(BN_CALL_BUDGET_FILE_ENV, "")
            if use_environment_budget_file
            else ""
        )
        self.budget_file = Path(
            budget_file if budget_file is not None else environment_budget_file
        ) if (budget_file is not None or environment_budget_file) else None
        self._calls_used = 0
        self._symbols_by_address: dict[str, Symbol] | None = None
        self._symbols_by_name: dict[str, Symbol] | None = None
        self._data_variables: tuple[DataVariable, ...] | None = None
        self._snapshot: BinaryNinjaSnapshot | None = None

    def budget_state(self) -> BridgeBudgetState:
        if self.budget_file is None:
            return BridgeBudgetState(used=self._calls_used, limit=self.call_budget)
        return self._read_budget_state()

    def _read_budget_state(self) -> BridgeBudgetState:
        if self.budget_file is None:
            return BridgeBudgetState(used=self._calls_used, limit=self.call_budget)
        try:
            data = json.loads(self.budget_file.read_text(encoding="utf-8"))
            used = int(data.get("used", 0))
            limit = int(data.get("limit", self.call_budget))
            return BridgeBudgetState(used=used, limit=limit)
        except FileNotFoundError:
            return BridgeBudgetState(used=0, limit=self.call_budget)
        except (OSError, ValueError, TypeError, json.JSONDecodeError) as exc:
            raise BridgeError(f"Binary Ninja bridge budget file is invalid: {self.budget_file}: {exc}") from exc

    def _write_budget_state(self, state: BridgeBudgetState) -> None:
        if self.budget_file is None:
            self._calls_used = state.used
            return
        self.budget_file.parent.mkdir(parents=True, exist_ok=True)
        self.budget_file.write_text(
            json.dumps({"used": state.used, "limit": state.limit}, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    def _claim_call(self, endpoint: str) -> None:
        state = self.budget_state()
        if state.limit > 0 and state.used >= state.limit:
            raise BridgeBudgetExceeded(
                "Binary Ninja bridge call budget exhausted before "
                f"{endpoint}: used {state.used}/{state.limit} call(s)"
            )
        self._write_budget_state(BridgeBudgetState(used=state.used + 1, limit=state.limit))

    def _request_params(self, endpoint: str, params: dict[str, object]) -> dict[str, object]:
        result = {key: value for key, value in params.items() if value is not None}
        normalized_endpoint = endpoint.lstrip("/")
        if (
            self.binary
            and "binary" not in result
            and normalized_endpoint not in {"binaries", "selectBinary", "platforms", "convertNumber"}
        ):
            result["binary"] = self.binary
        return result

    def get_json(self, endpoint: str, **params: object) -> dict:
        query = urlencode(self._request_params(endpoint, params))
        url = f"{self.base_url}/{endpoint.lstrip('/')}"
        if query:
            url = f"{url}?{query}"
        self._claim_call(endpoint)
        try:
            with urlopen(url, timeout=self.timeout) as response:
                return json.loads(response.read().decode("utf-8"))
        except Exception as exc:  # pragma: no cover - useful CLI diagnostic
            raise BridgeError(f"Binary Ninja bridge request failed: {url}: {exc}") from exc

    def function_info(self, address_or_name: str) -> dict:
        key = "address" if address_or_name.lower().startswith("0x") else "name"
        return self.get_json("functionInfo", **{key: address_or_name})

    def snapshot(self) -> BinaryNinjaSnapshot:
        """Return the bridge provider's snapshot token or unavailable state.

        The optional ``snapshotInfo`` bridge endpoint owns the identity.  Old
        bridge providers remain usable for read-only diagnostics: absence or a
        malformed response is reported explicitly and must make expected-fact
        verification ineligible rather than failing ordinary reads.
        """

        if self._snapshot is not None:
            return self._snapshot
        try:
            payload = self.get_json("snapshotInfo")
        except BridgeError as exc:
            self._snapshot = BinaryNinjaSnapshot(
                available=False,
                reason=f"snapshotInfo unavailable: {exc}",
            )
            return self._snapshot
        row = payload.get("snapshot") if isinstance(payload, dict) else None
        if not isinstance(row, dict):
            row = payload if isinstance(payload, dict) else {}
        generation_token = str(row.get("generation_token", "") or "")
        revision = str(row.get("revision", "") or "")
        schema = str(row.get("schema", "") or "")
        authenticated = row.get("authenticated") is True
        provider = str(row.get("provider", "") or "")
        capability_version = str(row.get("capability_version", "") or "")
        saved_view = str(row.get("saved_view", "") or "")
        available = bool(
            row.get("available", True)
            and generation_token
            and revision
            and saved_view
        )
        reason = "" if available else str(
            row.get(
                "reason",
                "snapshotInfo lacks provider generation token, revision, or saved view",
            )
        )
        self._snapshot = BinaryNinjaSnapshot(
            available=available,
            generation_token=generation_token if available else "",
            revision=revision if available else "",
            reason=reason,
            schema=schema,
            authenticated=authenticated,
            provider=provider,
            capability_version=capability_version,
            saved_view=saved_view if available else "",
        )
        return self._snapshot

    def fresh_snapshot(self) -> BinaryNinjaSnapshot:
        """Read a new provider snapshot without reusing process cache.

        Explicit expected-fact producers must compare one fresh snapshot before
        extraction with another after extraction while holding a governed BN
        read reservation.  Ordinary query and scheduler paths must never call
        this method.
        """

        self._snapshot = None
        return self.snapshot()

    def assembly(self, address_or_name: str) -> str:
        return str(self.get_json("assembly", name=address_or_name).get("assembly", ""))

    def hexdump(self, address: str, length: int) -> str:
        query = urlencode(self._request_params("hexdump", {"address": address, "length": str(length)}))
        url = f"{self.base_url}/hexdump?{query}"
        self._claim_call("hexdump")
        try:
            with urlopen(url, timeout=self.timeout) as response:
                return response.read().decode("utf-8")
        except Exception as exc:  # pragma: no cover - useful CLI diagnostic
            raise BridgeError(f"Binary Ninja bridge request failed: {url}: {exc}") from exc

    def il(self, address_or_name: str, view: str = "mlil") -> str:
        key = "address" if address_or_name.lower().startswith("0x") else "name"
        return str(self.get_json("il", **{key: address_or_name, "view": view}).get("il", ""))

    def symbols(self) -> tuple[dict[str, Symbol], dict[str, Symbol]]:
        if self._symbols_by_address is not None and self._symbols_by_name is not None:
            return self._symbols_by_address, self._symbols_by_name

        by_address: dict[str, Symbol] = {}
        by_name: dict[str, Symbol] = {}
        for endpoint, collection, kind in (
            ("methods", "functions", "function"),
            ("imports", "imports", "import"),
        ):
            offset = 0
            limit = 100000
            while True:
                data = self.get_json(endpoint, offset=offset, limit=limit)
                for item in data.get(collection, []):
                    address = _normalize_address(str(item["address"]))
                    symbol = Symbol(
                        address=address,
                        name=str(item.get("name", "")),
                        raw_name=str(item.get("raw_name", "")),
                        full_name=str(item.get("full_name", "")),
                        kind=kind,
                    )
                    by_address[address] = symbol
                    for name in {symbol.name, symbol.raw_name, symbol.full_name}:
                        if name:
                            by_name[name] = symbol
                offset += limit
                if offset >= int(data.get("total", 0)):
                    break

        self._symbols_by_address = by_address
        self._symbols_by_name = by_name
        return by_address, by_name

    def data_variables(self) -> tuple[DataVariable, ...]:
        if self._data_variables is not None:
            return self._data_variables

        rows: list[DataVariable] = []
        offset = 0
        limit = 1000
        while True:
            data = self.get_json("data", offset=offset, limit=limit)
            items = data.get("data", [])
            if not isinstance(items, list):
                raise BridgeError("Binary Ninja bridge /data response has no data row list")
            for item in items:
                if not isinstance(item, dict) or "address" not in item:
                    raise BridgeError("Binary Ninja bridge /data response has an invalid row")
                raw_size = item.get("size", 0)
                try:
                    size = int(str(raw_size), 0)
                except (TypeError, ValueError):
                    size = 0
                rows.append(
                    DataVariable(
                        address=_normalize_address(str(item["address"])),
                        name=str(item.get("name", "")),
                        raw_name=str(item.get("raw_name", "")),
                        type_text=str(item.get("type", "")),
                        size=size,
                    )
                )
            total = int(data.get("total", len(rows)))
            offset += len(items)
            if offset >= total:
                break
            if not items:
                raise BridgeError(
                    "Binary Ninja bridge /data pagination stopped before total rows"
                )

        self._data_variables = tuple(rows)
        return self._data_variables


class GovernedBinaryNinjaReadSession:
    """Reservation-bound, target-qualified and fully transcribed BN reader.

    Construction authenticates an already-active explicit maintenance packet
    before the bridge factory is touched.  The public facade exposes only a
    reviewed read endpoint set and hexdump.  A receipt exists only after an
    equal terminal provider snapshot and independent reservation postflight.
    """

    def __init__(
        self,
        progress_path: str | Path,
        packet_id: str,
    ) -> None:
        self.progress_path = Path(progress_path)
        self.packet_id = str(packet_id)
        authorization = self._authenticate_reservation()
        self.issue_ledger_path = Path(str(authorization["issue_ledger_path"]))
        self.reservation_id = str(authorization["reservation_id"])
        self._claims = tuple(dict(row) for row in authorization["resource_claims"])
        self._reads: list[dict[str, object]] = []
        self._finished = False
        self._receipt: GovernedBinaryNinjaReadReceipt | None = None

        # This is intentionally the first bridge construction point.  An
        # ambient shared budget file would make a governed read write outside
        # its output root, so it is always disabled.
        self._bridge = BinaryNinjaBridge(
            DEFAULT_BRIDGE_URL,
            timeout=15.0,
            binary="Recoil.bndb",
            call_budget=DEFAULT_BN_CALL_BUDGET,
            use_environment_budget_file=False,
        )
        self._begin_snapshot = self._required_snapshot("begin")
        self._reference = self._validate_reference_view()

    def _authenticate_reservation(self) -> dict[str, object]:
        from _recoil.lib.progress import (
            EXPLICIT_MAINTENANCE_PACKET_TYPE,
            ProgressError,
            ProgressStore,
            authenticate_explicit_output_root,
            normalize_resource_claims,
            work_resource_claims,
        )

        try:
            document = ProgressStore(self.progress_path).load()
        except (OSError, ValueError, ProgressError) as exc:
            raise BridgeError(f"cannot authenticate BN reader reservation: {exc}") from exc
        work = document.collection("work_items").get(self.packet_id)
        if not isinstance(work, Mapping) or work.get("packet_type") != EXPLICIT_MAINTENANCE_PACKET_TYPE:
            raise BridgeError("governed BN reader requires an explicit maintenance packet")
        reservation = work.get("reservation")
        if (
            work.get("state") != "active"
            or not isinstance(reservation, Mapping)
            or reservation.get("state") != "active"
            or not str(reservation.get("id", ""))
        ):
            raise BridgeError("governed BN reader packet has no active reservation")
        claims, complete, _source = work_resource_claims(work)
        raw_reservation_claims = reservation.get("resource_claims")
        if not complete or not isinstance(raw_reservation_claims, list):
            raise BridgeError("governed BN reader reservation claims are incomplete")
        try:
            captured = normalize_resource_claims(
                row for row in raw_reservation_claims if isinstance(row, Mapping)
            )
        except ProgressError as exc:
            raise BridgeError("governed BN reader reservation claims are malformed") from exc
        if captured != claims or len(captured) != len(raw_reservation_claims):
            raise BridgeError("governed BN reader packet and reservation claims differ")
        if {"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"} not in claims:
            raise BridgeError("governed BN reader reservation lacks Recoil.bndb read access")
        if not any(
            row["kind"] == "output-root" and row["access"] == "write"
            for row in claims
        ):
            raise BridgeError("governed BN reader reservation lacks an isolated output root")
        try:
            authenticate_explicit_output_root(
                work,
                progress_path=self.progress_path,
            )
        except ProgressError as exc:
            raise BridgeError(
                f"governed BN reader output root is not authenticated: {exc}"
            ) from exc
        provenance = work.get("explicit_provenance")
        identity = (
            provenance.get("issue_ledger_identity")
            if isinstance(provenance, Mapping)
            else None
        )
        expected_identity_keys = {
            "path",
            "application_id",
            "user_version",
            "schema_version",
            "ledger_version",
            "cutover_pair_id",
        }
        if not isinstance(identity, Mapping) or set(identity) != expected_identity_keys:
            raise BridgeError("governed BN reader packet lacks its bound issue-ledger identity")
        issue_ledger_path = Path(str(identity.get("path", "")))
        if not issue_ledger_path.is_absolute() or not issue_ledger_path.is_file():
            raise BridgeError("governed BN reader bound issue ledger is unavailable")
        from _recoil.lib.issue_sqlite import read_issue_metadata
        from _recoil.commands.workspace_issues import (
            workspace_issue_reservation_conflicts,
        )

        try:
            metadata = read_issue_metadata(issue_ledger_path)
        except Exception as exc:
            raise BridgeError(f"cannot authenticate bound issue ledger: {exc}") from exc
        observed_identity = {
            "path": str(issue_ledger_path.resolve()),
            "application_id": metadata.application_id,
            "user_version": metadata.user_version,
            "schema_version": metadata.schema_version,
            "ledger_version": metadata.ledger_version,
            "cutover_pair_id": metadata.cutover_pair_id,
        }
        if observed_identity != dict(identity):
            raise BridgeError("governed BN reader bound issue-ledger identity changed")
        try:
            conflicts = workspace_issue_reservation_conflicts(
                issue_ledger_path, self.packet_id, claims
            )
        except Exception as exc:
            raise BridgeError(f"cannot authenticate cross-ledger BN lease: {exc}") from exc
        if conflicts:
            raise BridgeError("governed BN reader conflicts with an active issue reservation")
        return {
            "reservation_id": str(reservation["id"]),
            "resource_claims": claims,
            "issue_ledger_path": str(issue_ledger_path.resolve()),
            "issue_ledger_identity": observed_identity,
        }

    def _required_snapshot(self, stage: str) -> dict[str, object]:
        try:
            snapshot = self._bridge.fresh_snapshot()
        except Exception as exc:
            raise BridgeError(f"Binary Ninja {stage} snapshot unavailable: {exc}") from exc
        return require_authenticated_recoil_snapshot(snapshot, stage=stage)

    def _record(self, transport: str, endpoint: str, params: Mapping[str, object], payload: object) -> None:
        canonical_params = {
            str(key): params[key]
            for key in sorted(params)
            if params[key] is not None
        }
        if endpoint not in {"binaries"} and "binary" not in canonical_params:
            canonical_params["binary"] = "Recoil.bndb"
        payload_bytes = (
            payload.encode("utf-8")
            if isinstance(payload, str)
            else json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8")
        )
        row = {
            "ordinal": len(self._reads),
            "transport": transport,
            "endpoint": endpoint,
            "parameters": canonical_params,
            "payload": payload,
            "payload_size": len(payload_bytes),
        }
        prospective = [*self._reads, row]
        transcript_size = len(
            json.dumps(prospective, sort_keys=True, separators=(",", ":")).encode("utf-8")
        )
        if transcript_size > GOVERNED_TRANSCRIPT_MAX_BYTES:
            raise BridgeError(
                "governed Binary Ninja transcript exceeds the bounded exact-payload limit"
            )
        self._reads.append(row)

    def _validate_reference_view(self) -> dict[str, str]:
        from _recoil.commands.binja_preflight import (
            validate_binaries,
            validate_status,
        )
        from _recoil.lib.reference_images import reference_image

        image = reference_image("recoil")
        status = self.get_json("status")
        binaries = self.get_json("binaries")
        errors = [
            *validate_status(
                status,
                expected_file=image.bndb_path,
                expected_platform=image.platform,
                expected_arch=image.arch,
            ),
            *validate_binaries(
                binaries,
                expected_file=image.bndb_path,
                require_active=False,
            ),
        ]
        if errors:
            raise BridgeError("governed Binary Ninja saved-view mismatch: " + "; ".join(errors))
        return {
            "key": image.key,
            "logical_saved_view": "Recoil.bndb",
            "saved_view_path": image.bndb_path,
            "retail_reference_path": image.reference_path,
            "platform": image.platform,
            "architecture": image.arch,
        }

    def get_json(self, endpoint: str, **params: object) -> dict:
        normalized = endpoint.lstrip("/")
        if self._finished:
            raise BridgeError("governed Binary Ninja read session is already finished")
        if normalized not in GOVERNED_READ_ENDPOINTS:
            raise BridgeError(f"Binary Ninja endpoint is not in the governed read registry: {normalized}")
        forbidden = sorted(
            key for key in params if key.casefold() in GOVERNED_SELECTOR_OVERRIDE_KEYS
        )
        if forbidden:
            raise BridgeError(
                "governed Binary Ninja reads forbid caller target/view selectors: "
                + ", ".join(forbidden)
            )
        payload = self._bridge.get_json(normalized, **params)
        if not isinstance(payload, dict):
            raise BridgeError(f"Binary Ninja endpoint {normalized} returned non-object JSON")
        self._record("json", normalized, params, payload)
        return payload

    def hexdump(self, address: str, length: int) -> str:
        if self._finished:
            raise BridgeError("governed Binary Ninja read session is already finished")
        if (
            isinstance(length, bool)
            or not isinstance(length, int)
            or length <= 0
            or length > GOVERNED_HEXDUMP_MAX_BYTES
        ):
            raise BridgeError(
                "governed Binary Ninja hexdump length must be positive and bounded"
            )
        payload = self._bridge.hexdump(address, length)
        self._record("text", "hexdump", {"address": address, "length": length}, payload)
        return payload

    def finish(self) -> GovernedBinaryNinjaReadReceipt:
        if self._receipt is not None:
            return self._receipt
        if self._finished:
            raise BridgeError("governed Binary Ninja read session did not complete")
        end = self._required_snapshot("terminal")
        if end != self._begin_snapshot:
            self._finished = True
            raise BridgeError("Binary Ninja snapshot changed during governed read session")
        postflight = self._authenticate_reservation()
        if (
            postflight["reservation_id"] != self.reservation_id
            or tuple(postflight["resource_claims"]) != self._claims
        ):
            self._finished = True
            raise BridgeError("governed Binary Ninja reservation changed during read session")
        transcript = tuple(dict(row) for row in self._reads)
        self._receipt = GovernedBinaryNinjaReadReceipt(
            _seal=_GOVERNED_RECEIPT_SEAL,
            packet_id=self.packet_id,
            reservation_id=self.reservation_id,
            reference_image=self._reference,
            issue_ledger_identity=postflight["issue_ledger_identity"],
            begin_snapshot=self._begin_snapshot,
            end_snapshot=end,
            fact_reads=transcript,
            resource_claims=self._claims,
        )
        self._finished = True
        return self._receipt

    @property
    def receipt(self) -> GovernedBinaryNinjaReadReceipt:
        if self._receipt is None:
            raise BridgeError("governed Binary Ninja receipt is unavailable before finish")
        return self._receipt

    def __enter__(self) -> "GovernedBinaryNinjaReadSession":
        return self

    def __exit__(self, exc_type: object, exc: object, traceback: object) -> None:
        if exc_type is None:
            self.finish()
        else:
            self._finished = True


def _normalize_address(value: str) -> str:
    value = value.strip()
    if value.lower().startswith("0x"):
        return f"0x{int(value, 16):x}"
    return f"0x{int(value, 16):x}"


def create_shared_budget_file() -> Path:
    handle = tempfile.NamedTemporaryFile(
        prefix="recoil_bn_budget_",
        suffix=".json",
        delete=False,
        mode="w",
        encoding="utf-8",
    )
    path = Path(handle.name)
    with handle:
        handle.write(json.dumps({"used": 0, "limit": DEFAULT_BN_CALL_BUDGET}, sort_keys=True) + "\n")
    return path


def env_with_shared_budget(budget_file: Path, base_env: dict[str, str] | None = None) -> dict[str, str]:
    env = dict(base_env if base_env is not None else os.environ)
    env[BN_CALL_BUDGET_FILE_ENV] = str(budget_file)
    return env
