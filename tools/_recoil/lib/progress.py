from __future__ import annotations

from copy import deepcopy
from dataclasses import dataclass
from datetime import datetime, timezone
import importlib.util
import json
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import sys
from typing import Any, Iterable, Mapping, Sequence

from _recoil.lib.live_progress import (
    ConcurrentRevisionUpdate,
    LiveProgressError,
    RevisionStore,
    validate_tracker_v5,
)
from _recoil.lib.tooling import REPO_ROOT
from _recoil.lib.call_contract_generations import (
    current_generations,
    evidence_generations_current,
)
from _recoil.lib.windows_identity import physical_identity
from _recoil.lib.repository_paths import (
    GitTrackedPathInventory,
    RepositoryPathError,
    load_git_tracked_path_inventory,
    normalize_generated_repository_path,
    resolve_tracked_repository_file,
    validate_repository_relative_path,
)


SCHEMA_VERSION = 5
DEFAULT_PROGRESS_PATH = REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
SQLITE_PROGRESS_SUFFIXES = frozenset({".sqlite", ".sqlite3", ".db"})
TOP_LEVEL_COLLECTIONS = (
    "binaries",
    "physical_blocks",
    "semantic_spans",
    "symbols",
    "output_sections",
    "storage_contributions",
    "owners",
    "verification_targets",
    "work_items",
    "blockers",
    "evidence",
    "tombstones",
)
TOP_LEVEL_KEYS = ("schema_version", "revision", "id_sequences", "migration", *TOP_LEVEL_COLLECTIONS)

RESULT_STATES = frozenset({"pending", "passed", "failed", "blocked", "not-applicable"})
DISPOSITIONS = frozenset({"claim", "observed", "accepted", "stale", "superseded"})
FRESHNESS_STATES = frozenset({"current", "historical", "changed", "missing"})
OWNER_GATE_STATES = frozenset({"pending", "accepted", "blocked", "none", "deferred"})
OWNER_GATES = ("boundary", "source", "data", "functional", "owner_linkage", "byte")
OWNER_LIFECYCLE_STATES = frozenset({"discovered", "mapped", "active", "accepted", "blocked"})
TIERS = ("X", "C", "B", "A", "S")
TIER_INDEX = {tier: index for index, tier in enumerate(TIERS)}
PIPELINE_PHASES = (
    "authored-function-order",
    "authored-call-contract",
    "authored-byte-match",
    "full-function-order",
    "linked-byte-match",
    "final-validation",
)
PIPELINE_CLASSES = frozenset({"authored", "authored-lifecycle", "non-authored", "unresolved"})
AUTHORED_PIPELINE_CLASSES = frozenset({"authored", "authored-lifecycle"})
AUTHORED_ORDER_ROLES = frozenset(
    {
        "authored-body",
        "authored-lifecycle-body",
        "compiler-generated-deleting-variant",
        "compiler-generated-eh-helper",
        "compiler-generated-thunk",
        "compiler-generated-implicit-cleanup",
        "compiler-generated-icf-representative",
        "non-authored",
        "unresolved",
    }
)
AUTHORED_ORDER_GATING_ROLES = frozenset({"authored-body", "authored-lifecycle-body"})
COMPILER_GENERATED_AUTHORED_ORDER_ROLES = frozenset(
    role for role in AUTHORED_ORDER_ROLES if role.startswith("compiler-generated-")
)
AUTHORED_BYTE_DIMENSIONS = (
    "object_byte",
    "relocation_identity",
    "linked_presence",
    "linked_target_identity",
    "linked_body_byte",
)
CALL_CONTRACT_DIMENSION = "call_contract"
CALL_CONTRACT_ACCEPTANCE_EVIDENCE_KIND = "live-authored-call-contract-validation"
CALL_CONTRACT_SLICE_MAX_BODIES = 160
CALL_CONTRACT_SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx"})
CALL_CONTRACT_VALIDATION_MODE = "live"
CALL_CONTRACT_CONTRACT_VERSION = 2
CALL_CONTRACT_EXPECTED_TRUTH = (
    "retail-binary-ninja-plus-reviewed-tracker-identities"
)
EXACT_LINK_DIMENSIONS = ("linked_address", "linked_targets", "linked_byte")
SYMBOL_BINARY_DIMENSIONS = tuple(
    dict.fromkeys(
        (*AUTHORED_BYTE_DIMENSIONS, CALL_CONTRACT_DIMENSION, *EXACT_LINK_DIMENSIONS)
    )
)
AUTHORED_ORDER_DIMENSIONS = (
    "object_identity_presence",
    "authored_object_order",
    "linked_identity_presence",
    "authored_linked_order",
    "block_precedence",
)
FULL_ORDER_DIMENSIONS = ("object_order", "linked_interval", "linked_seam")
BLOCK_ORDER_DIMENSIONS = FULL_ORDER_DIMENSIONS
ORDER_GATE_POLICY = {
    "required_retail_identities": "exactly-once-in-authored-and-full-scopes",
    "authored_relative_order": "authored-body-and-authored-lifecycle-body-only",
    "unlisted_raw_definitions": "diagnostic-only",
    "authored_selected_extras": "classified-non-authored-only",
    "full_selected_linked_address_groups": "exact",
    "full_linked_predecessor_successor_seams": "exact",
}
STORAGE_DIMENSIONS = ("extent", "object", "relocation", "order", "link", "raw", "zero-fill")
SECTION_DIMENSIONS = (
    "placement",
    "characteristics",
    "raw-size",
    "virtual-size",
    "raw",
    "zero-fill",
    "order",
    "whole",
)
STORAGE_KINDS = frozenset(
    {"data-symbol", "object-section", "provider-data", "padding", "zero-fill", "embedded-data"}
)
EXTENT_STATES = frozenset({"known", "unknown"})
WORK_HANDOFF_STATES = frozenset({"ready", "current", "active", "blocked"})
WORK_LAUNCHABLE_STATES = frozenset({"ready", "current"})
WORK_HANDOFF_ROLES = frozenset({"recoil_source_worker", "recoil_verifier"})
WORK_FRONTIER_RELATIONS = frozenset(
    {
        "current-cursor",
        "authored-call-contract",
        "required-dependency",
        "parallel-authored-byte",
        "parallel-authored-object-byte",
        "accepted-prefix-fallback",
    }
)
WORK_RESOURCE_ACCESS = frozenset({"read", "write"})
WORK_PACKET_CONTRACT_VERSION = 3
CLAIM_PROVENANCE_SCHEMA_VERSION = 1
CLAIM_CURRENT_COMMAND = "progress work claim-current"
CLAIM_REQUESTED_LANES = frozenset({"primary", "authored", "object", "all"})
CLAIM_SELECTED_LANES = frozenset({"primary", "authored", "object"})
EXPLICIT_MAINTENANCE_PACKET_SCHEMA = "recoil-explicit-maintenance-packet-v1"
EXPLICIT_MAINTENANCE_PACKET_TYPE = "explicit-user-selected-maintenance-v1"
EXPLICIT_MAINTENANCE_COMMAND = "progress work create-explicit"
EXPLICIT_MAINTENANCE_KINDS = frozenset(
    {"source-maintenance", "read-only-diagnostic"}
)
EXPLICIT_MAINTENANCE_ROLES = frozenset(
    {"recoil_source_worker", "recoil_verifier", "recoil_bn_fact_mapper"}
)
EXPLICIT_SOURCE_SUFFIXES = frozenset(
    {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl"}
)
EXPLICIT_OVERRIDE_RELATIONS = frozenset(
    {
        "reviewed-cross-owner-declaration",
        "reviewed-declaration-debt",
        "reviewed-unregistered-declaration-debt",
    }
)
EXPLICIT_RESULT_MAX_BYTES = 64 * 1024
EXPLICIT_OUTPUT_MARKER_SCHEMA = "recoil-explicit-output-root-owner-v3"
EXPLICIT_OUTPUT_MARKER_NAME = ".recoil-explicit-output-root.json"
EXPLICIT_ALLOCATION_FAILURE_SCHEMA = "recoil-explicit-output-allocation-failure-v2"
EXPLICIT_CLEANUP_DEBT_SCHEMA = "recoil-explicit-output-cleanup-debt-v1"
EXPLICIT_ALLOCATION_JOURNAL_SCHEMA = "recoil-explicit-output-allocation-journal-v1"
EXPLICIT_ALLOCATION_JOURNAL_MIGRATION_KEY = "explicit_output_allocation_journals"
EXPLICIT_CLEANUP_RECOVERY_RECEIPT_SCHEMA = (
    "recoil-explicit-output-cleanup-recovery-receipt-v1"
)
REPAIR_CONTINUATION_PACKET_TYPE = "call-contract-repair-continuation-edit-v1"
REPAIR_CONTINUATION_PROVENANCE_SCHEMA_VERSION = 1
PREPARE_REPAIR_CONTINUATION_COMMAND = (
    "progress call-contract prepare-repair-continuation"
)
SCHEDULER_OUTPUT_SCHEMA_VERSION = 1
HEX_ADDRESS_RE = re.compile(r"^(?:0x|sub_)?([0-9a-fA-F]+)$")
EVIDENCE_ID_RE = re.compile(r"^recoil:evidence:r([0-9]+):([0-9]{6,})$")


class ProgressError(ValueError):
    pass


class ConcurrentProgressUpdate(ProgressError):
    pass


@dataclass(frozen=True)
class Finding:
    severity: str
    code: str
    message: str
    entity_id: str = ""

    def to_dict(self) -> dict[str, str]:
        return {
            "severity": self.severity,
            "code": self.code,
            "message": self.message,
            "entity_id": self.entity_id,
        }


@dataclass(frozen=True)
class CommitResult:
    applied: bool
    path: Path
    previous_revision: int
    revision: int

    def to_dict(self) -> dict[str, Any]:
        return {
            "applied": self.applied,
            "path": self.path.as_posix(),
            "previous_revision": self.previous_revision,
            "revision": self.revision,
        }


def normalize_address(value: str | int) -> str:
    if isinstance(value, bool):
        raise ProgressError("boolean is not an address")
    if isinstance(value, int):
        number = value
    else:
        match = HEX_ADDRESS_RE.fullmatch(str(value).strip())
        if match is None:
            raise ProgressError(f"invalid address {value!r}")
        number = int(match.group(1), 16)
    if number < 0:
        raise ProgressError("address must be non-negative")
    return f"0x{number:x}"


def address_value(value: str | int) -> int:
    return int(normalize_address(value), 16)


def _repository_path_error(exc: RepositoryPathError) -> ProgressError:
    return ProgressError(str(exc))


def _lexical_repository_resource_path(value: str, *, context: str) -> str:
    """Normalize replayed path claims without inventing current Git identity.

    Resource-claim rows are persisted concurrency identities.  Their replay
    remains lexical; current packet constructors authenticate tracked files at
    their typed input boundary before creating these rows.
    """

    try:
        return validate_repository_relative_path(str(value).strip(), context=context)
    except RepositoryPathError as exc:
        raise _repository_path_error(exc) from exc


def _generated_repository_path(
    value: str,
    *,
    context: str,
    allowed_roots: Sequence[str] = ("build",),
) -> str:
    try:
        return normalize_generated_repository_path(
            str(value).strip(),
            allowed_roots=allowed_roots,
            context=context,
        ).logical_path
    except RepositoryPathError as exc:
        raise _repository_path_error(exc) from exc


def stable_id(binary: str, kind: str, identity: str | int) -> str:
    binary_key = str(binary).strip().lower()
    kind_key = str(kind).strip().lower().replace("_", "-")
    if not binary_key or ":" in binary_key or not kind_key or ":" in kind_key:
        raise ProgressError("binary and kind must be non-empty colon-free stable-id components")
    if isinstance(identity, int) or HEX_ADDRESS_RE.fullmatch(str(identity).strip()):
        identity_key = normalize_address(identity)
    else:
        identity_key = str(identity).strip().replace(" ", "-")
    if not identity_key:
        raise ProgressError("stable-id identity must be non-empty")
    return f"{binary_key}:{kind_key}:{identity_key}"


def resolve_full_order_target_block(
    document: "ProgressDocument",
    *,
    binary: str,
    target_name: str,
    lane: str,
    retail_start: str,
    retail_end_exclusive: str,
) -> tuple[str, dict[str, Any]]:
    """Resolve one registered order target to one exact physical block."""

    if lane not in {"object", "linked"}:
        raise ProgressError(f"invalid full-order target lane {lane!r}")
    start = normalize_address(retail_start)
    end = normalize_address(retail_end_exclusive)
    matches = [
        (block_id, block)
        for block_id, block in document.collection("physical_blocks").items()
        if isinstance(block, dict)
        and block.get("binary") == binary
        and normalize_address(block.get("start", "")) == start
        and normalize_address(block.get("end_exclusive", "")) == end
        and isinstance(block.get("order_targets"), Mapping)
        and block["order_targets"].get(lane) == target_name
    ]
    if len(matches) != 1:
        raise ProgressError(
            f"expected one exact current {lane} order block for {target_name!r} "
            f"[{start},{end}), found {len(matches)}"
        )
    return matches[0]


def default_authored_order_role(pipeline_class: str) -> str:
    return {
        "authored": "authored-body",
        "authored-lifecycle": "authored-lifecycle-body",
        "non-authored": "non-authored",
        "unresolved": "unresolved",
    }.get(str(pipeline_class), "unresolved")


def symbol_authored_order_role(symbol: Mapping[str, Any]) -> str:
    role = symbol.get("authored_order_role")
    return str(role) if role in AUTHORED_ORDER_ROLES else default_authored_order_role(
        str(symbol.get("pipeline_class", "unresolved"))
    )


def symbol_authored_order_gate(symbol: Mapping[str, Any]) -> bool:
    return symbol_authored_order_role(symbol) in AUTHORED_ORDER_GATING_ROLES


def symbol_logical_aliases(symbol: Mapping[str, Any]) -> list[tuple[str, dict[str, Any]]]:
    aliases = symbol.get("logical_aliases", {})
    if not isinstance(aliases, Mapping):
        return []
    return [
        (str(identity), dict(alias))
        for identity, alias in sorted(aliases.items(), key=lambda item: str(item[0]))
        if isinstance(alias, Mapping)
    ]


def logical_alias_authored_order_role(alias: Mapping[str, Any]) -> str:
    role = alias.get("authored_order_role")
    return str(role) if role in AUTHORED_ORDER_ROLES else default_authored_order_role(
        str(alias.get("pipeline_class", "unresolved"))
    )


def logical_alias_authored_order_gate(alias: Mapping[str, Any]) -> bool:
    if alias.get("gate_mode") == "physical-body-only":
        return False
    return logical_alias_authored_order_role(alias) in AUTHORED_ORDER_GATING_ROLES


def logical_alias_authored_order_blocking(alias: Mapping[str, Any]) -> bool:
    return (
        alias.get("pipeline_class") == "unresolved"
        or logical_alias_authored_order_role(alias) == "unresolved"
    )


def symbol_has_logical_address_group(symbol: Mapping[str, Any]) -> bool:
    return bool(symbol_logical_aliases(symbol)) and isinstance(
        symbol.get("icf_address_group") or symbol.get("linked_address_group"), Mapping
    )


def validate_authored_order_role(pipeline_class: str, role: str) -> None:
    compatible = {
        "authored": {"authored-body"},
        "authored-lifecycle": {"authored-lifecycle-body", *COMPILER_GENERATED_AUTHORED_ORDER_ROLES},
        "non-authored": {"non-authored", *COMPILER_GENERATED_AUTHORED_ORDER_ROLES},
        "unresolved": {"unresolved"},
    }
    if role not in compatible.get(pipeline_class, set()):
        raise ProgressError(
            f"authored_order_role {role!r} is incompatible with pipeline_class {pipeline_class!r}"
        )


def state_record(
    result: str = "pending",
    disposition: str = "claim",
    freshness: str = "current",
    evidence_ids: Iterable[str] = (),
    *,
    gating: bool | None = None,
    validation_mode: str | None = None,
) -> dict[str, Any]:
    row: dict[str, Any] = {
        "result": result,
        "disposition": disposition,
        "freshness": freshness,
        "evidence_ids": sorted({str(item) for item in evidence_ids}),
    }
    if gating is not None:
        row["gating"] = bool(gating)
    if validation_mode is not None:
        row["validation_mode"] = str(validation_mode)
    return row


def is_accepted_state(value: Any) -> bool:
    return (
        isinstance(value, Mapping)
        and value.get("result") in {"passed", "not-applicable"}
        and value.get("disposition") == "accepted"
    )


def is_current_accepted_state(value: Any) -> bool:
    return (
        is_accepted_state(value)
        and value.get("freshness") == "current"
        and value.get("gating") is not False
        and value.get("validation_mode") == "live"
    )


def _normalized_resource_claim(value: Mapping[str, Any]) -> dict[str, str]:
    kind = str(value.get("kind", "")).strip().lower().replace("_", "-")
    identity = str(value.get("id", "")).strip()
    access = str(value.get("access", "")).strip().lower()
    if not kind or not identity or access not in WORK_RESOURCE_ACCESS:
        raise ProgressError("resource claims require non-empty kind/id and read|write access")
    if kind == "physical-block":
        kind = "block"
    if kind in {"path", "source-path", "header-path"}:
        identity = _lexical_repository_resource_path(
            identity,
            context=f"{kind} resource identity",
        )
    elif kind == "generated-output":
        identity = _generated_repository_path(
            identity,
            context=f"{kind} resource identity",
        )
    elif kind == "output-root":
        try:
            identity = _generated_repository_path(
                identity,
                context="repository-owned output-root resource identity",
            )
        except ProgressError:
            physical = Path(identity)
            if not physical.is_absolute():
                raise
            # An external packet/build root is a machine-local physical
            # resource.  Keep its absolute identity; never reinterpret it as
            # a current Git repository path.
            identity = physical.resolve().as_posix()
    elif kind == "binary-ninja-db":
        # The maintained saved view is one logical concurrency resource.  A
        # basename, slash variant, or configured absolute path must not create
        # an alias that evades the reader/writer conflict model.
        basename = PurePosixPath(identity).name.casefold()
        if basename == "recoil.bndb":
            identity = "Recoil.bndb"
        elif basename == "messages.bndb":
            identity = "messages.bndb"
    return {"kind": kind, "id": identity, "access": access}


def normalize_resource_claims(values: Iterable[Mapping[str, Any]]) -> list[dict[str, str]]:
    result: dict[tuple[str, str], dict[str, str]] = {}
    for value in values:
        claim = _normalized_resource_claim(value)
        key = (claim["kind"], claim["id"].casefold())
        prior = result.get(key)
        if prior is None or claim["access"] == "write":
            result[key] = claim
    return sorted(result.values(), key=lambda row: (row["kind"], row["id"].casefold(), row["access"]))


def work_resource_claims(work: Mapping[str, Any]) -> tuple[list[dict[str, str]], bool, str]:
    explicit = work.get("resource_claims")
    if isinstance(explicit, list):
        try:
            claims = normalize_resource_claims(
                item for item in explicit if isinstance(item, Mapping)
            )
        except ProgressError:
            return [], False, "invalid-explicit"
        return claims, len(claims) == len(explicit) and bool(claims), "explicit"
    derived: list[dict[str, str]] = []
    for key, access, kind in (
        ("allowed_paths", "write", "path"),
        ("read_only_paths", "read", "path"),
    ):
        values = work.get(key, [])
        if isinstance(values, list):
            for value in values:
                if isinstance(value, str) and value:
                    derived.append({"kind": kind, "id": value, "access": access})
    for field, kind, access in (
        ("owner_id", "owner", "read"),
        ("block_id", "physical-block", "read"),
    ):
        value = work.get(field)
        if isinstance(value, str) and value:
            derived.append({"kind": kind, "id": value, "access": access})
    try:
        claims = normalize_resource_claims(derived)
    except ProgressError:
        return [], False, "invalid-derived"
    return claims, bool(claims), "derived" if claims else "none"


def retail_fact_packet_contract_problem(
    work: Mapping[str, Any],
    claims: Iterable[Mapping[str, Any]],
) -> str:
    """Return the first typed read-only retail fact packet defect, if any."""

    from _recoil.commands.call_contract_convergence import (
        CONVERGENCE_EXPECTED_TRUTH,
        RETAIL_FACT_PACKET_TYPE,
        _valid_retail_fact_scope,
    )

    if work.get("packet_type") != RETAIL_FACT_PACKET_TYPE:
        return ""
    scope = work.get("retail_fact_scope")
    if not _valid_retail_fact_scope(scope):
        return "retail fact packet lacks one strict descriptor-derived scope"
    if (
        work.get("handoff_role") != "recoil_bn_fact_mapper"
        or work.get("read_only") is not True
        or work.get("nonaccepting") is not True
        or work.get("candidate_expected_truth") is not False
        or work.get("expected_truth") != CONVERGENCE_EXPECTED_TRUTH
    ):
        return "retail fact packet weakens its read-only/nonaccepting truth contract"
    if any(
        work.get(field) not in (None, [])
        for field in (
            "allowed_paths",
            "source_edit_paths",
            "definition_source_paths",
            "dependency_paths",
        )
    ):
        return "retail fact packet exposes a source or dependency edit path"
    try:
        normalized = normalize_resource_claims(claims)
    except ProgressError:
        return "retail fact packet has malformed resource claims"
    writes = [row for row in normalized if row["access"] == "write"]
    if writes != [
        {"kind": "lane", "id": str(scope["scope_id"]), "access": "write"}
    ]:
        return "retail fact packet requires exactly one packet-specific lane write"
    required_reads = {
        ("binary-ninja-db", "Recoil.bndb"),
        ("tracker", "recoil"),
        ("verification-target", str(scope["target_id"])),
        *(
            ("block", str(block_id))
            for block_id in scope["physical_block_ids"]
        ),
    }
    actual_reads = {
        (row["kind"], row["id"])
        for row in normalized
        if row["access"] == "read"
    }
    if actual_reads != required_reads:
        return "retail fact packet read claims differ from its exact BN/target/block scope"
    commands = work.get("validation_commands")
    if commands != [
        "python tools/recoil.py binja preflight --binary recoil --strict"
    ]:
        return "retail fact packet requires the exact nonmutating Recoil BN preflight"
    return ""


def _claim_paths_overlap(first: str, second: str) -> bool:
    one = PurePosixPath(first.casefold())
    two = PurePosixPath(second.casefold())
    return one == two or one in two.parents or two in one.parents


def resource_claim_conflicts(
    first_claims: Iterable[Mapping[str, Any]],
    second_id: str,
    second_claims: Iterable[Mapping[str, Any]],
    *,
    second_owner_id: str = "",
    second_block_id: str = "",
) -> list[dict[str, Any]]:
    first = normalize_resource_claims(first_claims)
    second = normalize_resource_claims(second_claims)
    path_kinds = {"path", "source-path", "header-path", "output-root", "generated-output"}
    conflicts: list[dict[str, Any]] = []
    for left in first:
        for right in second:
            if left["access"] == right["access"] == "read":
                continue
            same = left["kind"] == right["kind"] and left["id"].casefold() == right["id"].casefold()
            path_overlap = (
                left["kind"] in path_kinds
                and right["kind"] in path_kinds
                and _claim_paths_overlap(left["id"], right["id"])
            )
            build_window_overlap = (
                {left["kind"], right["kind"]}
                & {"whole-link-window"}
                and left["kind"] in {
                    "whole-link-window",
                    "whole-project-build",
                    "tu-build",
                }
                and right["kind"] in {
                    "whole-link-window",
                    "whole-project-build",
                    "tu-build",
                }
                and (
                    left["id"].casefold() == "recoil"
                    or right["id"].casefold() == "recoil"
                )
            )
            if not same and not path_overlap and not build_window_overlap:
                continue
            conflicts.append(
                {
                    "kind": "resource",
                    "work_item_id": second_id,
                    "owner_id": second_owner_id,
                    "block_id": second_block_id,
                    "first_claim": left,
                    "second_claim": right,
                    "paths": sorted({left["id"], right["id"]}),
                }
            )
    return conflicts


def empty_progress_document() -> dict[str, Any]:
    return {
        "schema_version": SCHEMA_VERSION,
        "revision": 0,
        "id_sequences": {},
        "migration": {},
        **{name: {} for name in TOP_LEVEL_COLLECTIONS},
    }


def _entity_binary(value: Mapping[str, Any]) -> str:
    return str(value.get("binary", ""))


def _entity_range(value: Mapping[str, Any]) -> tuple[int, int] | None:
    try:
        start = address_value(value.get("start", value.get("address", "")))
        end = address_value(value.get("end_exclusive", ""))
    except ProgressError:
        return None
    return (start, end) if end > start else None


class ProgressDocument:
    def __init__(
        self,
        data: Mapping[str, Any],
        *,
        path: Path | None = None,
        **_retired: Any,
    ) -> None:
        self.data = deepcopy(dict(data))
        self.path = path
        self._request_cache: dict[tuple[Any, ...], Any] = {}

    @classmethod
    def _from_owned_data(
        cls,
        data: dict[str, Any],
        *,
        path: Path | None = None,
    ) -> "ProgressDocument":
        """Wrap a newly decoded document without redundantly copying it.

        This is deliberately private: the public constructor and ``clone`` keep
        their defensive-copy contract for caller-owned mappings.  JSON decoders
        and revision stores already return a fresh object graph owned by this
        request, so copying that graph again only adds load latency and memory.
        """

        document = cls.__new__(cls)
        document.data = data
        document.path = path
        document._request_cache = {}
        return document

    @classmethod
    def empty(cls) -> "ProgressDocument":
        return cls(empty_progress_document())

    @classmethod
    def load(cls, path: str | Path) -> "ProgressDocument":
        progress_path = Path(path)
        if progress_path.suffix.lower() in SQLITE_PROGRESS_SUFFIXES:
            # Keep the legacy document API available while SQLite is the
            # durable authority.  The backend owns the decoded rows, so the
            # private constructor avoids a second 80+ MB defensive copy.
            from _recoil.lib.progress_sqlite import ProgressSQLiteStore

            try:
                data = ProgressSQLiteStore(progress_path, read_only=True).materialize()
            except Exception as exc:
                # Import locally to keep progress_sqlite independent from this
                # module and avoid an exception-class import cycle.
                from _recoil.lib.progress_sqlite import ProgressSQLiteError

                if isinstance(exc, ProgressSQLiteError):
                    raise ProgressError(str(exc)) from exc
                raise
            return cls._from_owned_data(data, path=progress_path)
        try:
            data = json.loads(progress_path.read_text(encoding="utf-8"))
        except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
            raise ProgressError(f"{progress_path}: unreadable progress JSON: {exc}") from exc
        if not isinstance(data, dict):
            raise ProgressError(f"{progress_path}: expected a JSON object")
        if data.get("schema_version") != SCHEMA_VERSION:
            raise ProgressError(f"{progress_path}: schema_version must be {SCHEMA_VERSION}")
        return cls._from_owned_data(data, path=progress_path)

    @property
    def revision(self) -> int:
        value = self.data.get("revision", -1)
        return value if isinstance(value, int) and not isinstance(value, bool) else -1

    def clone(self) -> "ProgressDocument":
        return ProgressDocument(self.data, path=self.path)

    def scheduler_identity(self) -> dict[str, Any]:
        revision_vector = {
            "transaction_revision": self.revision,
            "semantic_revision": self.revision,
            "evidence_generation_revision": self.revision,
            "scheduler_revision": self.revision,
        }
        if (
            self.path is not None
            and self.path.suffix.lower() in SQLITE_PROGRESS_SUFFIXES
        ):
            from _recoil.lib.progress_sqlite import (
                ProgressSQLiteError,
                read_progress_revision_vector,
            )

            try:
                live_vector = read_progress_revision_vector(self.path)
            except (OSError, ProgressSQLiteError) as exc:
                raise ProgressError(
                    f"cannot read progress revision vector from {self.path}: {exc}"
                ) from exc
            revision_vector = live_vector.to_dict()
            if revision_vector["transaction_revision"] != self.revision:
                raise ProgressError(
                    "progress tracker changed after this scheduler snapshot was loaded: "
                    f"document revision {self.revision}, metadata revision "
                    f"{revision_vector['transaction_revision']}"
                )
        return {
            "scheduler_schema_version": SCHEDULER_OUTPUT_SCHEMA_VERSION,
            "tracker_revision": self.revision,
            "revision": self.revision,
            "revision_vector": revision_vector,
            **revision_vector,
        }

    def scheduler_output(self, value: Mapping[str, Any]) -> dict[str, Any]:
        result = deepcopy(dict(value))
        result.update(self.scheduler_identity())
        return result

    def collection(self, name: str) -> dict[str, Any]:
        value = self.data.get(name)
        if not isinstance(value, dict):
            raise ProgressError(f"progress collection {name!r} must be an object")
        return value

    def owner_tier(self, owner: Mapping[str, Any]) -> str:
        entry_tiers = [
            str(item.get("tier", "X"))
            for item in owner.get("primary_entries", {}).values()
            if isinstance(item, Mapping)
        ] if isinstance(owner.get("primary_entries"), Mapping) else []
        if entry_tiers:
            return min(entry_tiers, key=lambda tier: TIER_INDEX.get(tier, 0))
        tier = str(owner.get("tier", "X"))
        return tier if tier in TIERS else "X"

    def _blocks_for_binary(self, binary: str) -> list[tuple[str, dict[str, Any]]]:
        cache_key = ("blocks-for-binary", binary)
        cached = self._request_cache.get(cache_key)
        if cached is None:
            rows = [
                (block_id, block)
                for block_id, block in self.collection("physical_blocks").items()
                if isinstance(block, dict)
                and _entity_binary(block) == binary
                and _entity_range(block)
            ]
            cached = tuple(
                sorted(rows, key=lambda item: (_entity_range(item[1])[0], item[0]))
            )
            self._request_cache[cache_key] = cached
        return list(cached)

    def _symbols_for_binary(self, binary: str) -> list[tuple[str, dict[str, Any]]]:
        cache_key = ("symbols-for-binary", binary)
        cached = self._request_cache.get(cache_key)
        if cached is None:
            rows = [
                (symbol_id, symbol)
                for symbol_id, symbol in self.collection("symbols").items()
                if isinstance(symbol, dict)
                and _entity_binary(symbol) == binary
                and symbol.get("kind")
                in {"function", "provider-function", "compiler-function"}
                and _entity_range(symbol)
            ]
            cached = tuple(
                sorted(rows, key=lambda item: (_entity_range(item[1])[0], item[0]))
            )
            self._request_cache[cache_key] = cached
        return list(cached)

    @staticmethod
    def _order_group_current(block: Mapping[str, Any], group: str, dimensions: Iterable[str]) -> bool:
        rows = block.get("order", {}).get(group, {}) if isinstance(block.get("order"), Mapping) else {}
        return isinstance(rows, Mapping) and all(
            is_current_accepted_state(rows.get(dimension)) for dimension in dimensions
        )

    @staticmethod
    def _block_authored_order_accepted(block: Mapping[str, Any]) -> bool:
        """Return whether every authored-order dimension has current live acceptance."""
        return ProgressDocument._order_group_current(
            block, "authored", AUTHORED_ORDER_DIMENSIONS
        )

    @staticmethod
    def _block_full_order_accepted(block: Mapping[str, Any]) -> bool:
        """Return whether every full-order dimension has current live acceptance."""
        return ProgressDocument._order_group_current(block, "full", FULL_ORDER_DIMENSIONS)

    @staticmethod
    def _symbol_dimensions_current(symbol: Mapping[str, Any], dimensions: Iterable[str]) -> bool:
        state = symbol.get("binary_state", {})
        return isinstance(state, Mapping) and all(
            is_current_accepted_state(state.get(dimension)) for dimension in dimensions
        )

    def _physical_groups(
        self,
        binary: str,
        *,
        gating_only: bool,
        eligible_end: int | None = None,
    ) -> list[dict[str, Any]]:
        cache_key = ("physical-groups", binary, gating_only, eligible_end)
        cached = self._request_cache.get(cache_key)
        if cached is None:
            groups: list[dict[str, Any]] = []
            for symbol_id, symbol in self._symbols_for_binary(binary):
                if gating_only and not symbol_authored_order_gate(symbol):
                    continue
                bounds = _entity_range(symbol)
                if bounds is None or (
                    eligible_end is not None and bounds[1] > eligible_end
                ):
                    continue
                if groups and groups[-1]["range"] == bounds:
                    groups[-1]["scope_ids"].append(symbol_id)
                    groups[-1]["symbols"].append(symbol)
                else:
                    groups.append(
                        {
                            "range": bounds,
                            "address": normalize_address(bounds[0]),
                            "end_exclusive": normalize_address(bounds[1]),
                            "scope_ids": [symbol_id],
                            "symbols": [symbol],
                            "physical_block_id": str(
                                symbol.get("physical_block_id", "")
                            ),
                        }
                    )
            cached = tuple(
                (
                    group["range"],
                    group["address"],
                    group["end_exclusive"],
                    tuple(group["scope_ids"]),
                    tuple(group["symbols"]),
                    group["physical_block_id"],
                )
                for group in groups
            )
            self._request_cache[cache_key] = cached
        return [
            {
                "range": bounds,
                "address": address,
                "end_exclusive": end_exclusive,
                "scope_ids": list(scope_ids),
                "symbols": list(symbols),
                "physical_block_id": physical_block_id,
            }
            for (
                bounds,
                address,
                end_exclusive,
                scope_ids,
                symbols,
                physical_block_id,
            ) in cached
        ]

    def _target_id_for_block(self, block: Mapping[str, Any], phase: str) -> str:
        targets = block.get("order_targets", {})
        if not isinstance(targets, Mapping):
            return ""
        configured = targets.get("object" if phase == "authored-function-order" else "linked")
        if not isinstance(configured, str) or not configured:
            return ""
        matches = [
            target_id
            for target_id, target in self.collection("verification_targets").items()
            if isinstance(target, Mapping)
            and (target_id == configured or target.get("name") == configured)
        ]
        return matches[0] if len(matches) == 1 else ""

    @staticmethod
    def _fresh_root(kind: str, label: str, revision: int) -> str:
        safe = re.sub(r"[^A-Za-z0-9_.-]+", "-", label).strip("-") or kind
        base = REPO_ROOT / "build" / "live-validation" / kind
        ordinal = 1
        while True:
            suffix = "" if ordinal == 1 else f"-{ordinal}"
            candidate = base / f"{safe}-r{revision + 1}{suffix}"
            if not candidate.exists():
                return candidate.relative_to(REPO_ROOT).as_posix()
            ordinal += 1

    def _order_target_resolution(self) -> dict[str, Any]:
        # Local import avoids making the schema/model layer depend on the CLI at
        # module import time while keeping scheduler and acceptance resolution
        # on one semantic contract.
        from _recoil.commands.progress_cli import resolve_current_order_target

        return resolve_current_order_target(self)

    def _live_order_command(
        self,
        phase: str,
        resolution: Mapping[str, Any],
    ) -> str:
        if resolution.get("status") != "ready":
            return ""
        target_id = str(resolution.get("target_id") or "")
        if not target_id:
            return ""
        root = self._fresh_root("order", target_id.rsplit(":", 1)[-1], self.revision)
        override_option = str(resolution.get("override_option") or "")
        override_selector = str(resolution.get("override_selector") or "")
        override = (
            f" {override_option} {override_selector}"
            if override_option and override_selector
            else ""
        )
        return (
            "python tools/recoil.py progress advance-live-order "
            f"--target {target_id}{override} --build-root {root} "
            f"--expected-revision {self.revision} --apply --json"
        )

    def _live_byte_command(self, lane: str, cursor: str) -> str:
        root = self._fresh_root(lane, cursor.replace("0x", ""), self.revision)
        return (
            "python tools/recoil.py progress advance-live-byte "
            f"--lane {lane} --build-root {root} "
            f"--expected-revision {self.revision} --apply --json"
        )

    def authored_call_contract_slices(
        self,
        binary: str = "recoil",
        *,
        max_bodies: int = CALL_CONTRACT_SLICE_MAX_BODIES,
    ) -> list[dict[str, Any]]:
        """Derive stable retail-monotonic authored invocation-contract slices.

        Slice membership comes only from gating symbol rows and their accepted
        authored-order target/block facts.  Candidate output and filesystem
        content summaries are intentionally absent from this derivation.
        """
        if (
            isinstance(max_bodies, bool)
            or not isinstance(max_bodies, int)
            or max_bodies < 1
            or max_bodies > CALL_CONTRACT_SLICE_MAX_BODIES
        ):
            raise ProgressError(
                f"authored call-contract slices require 1..{CALL_CONTRACT_SLICE_MAX_BODIES} bodies"
            )
        cache_key = ("authored-call-contract-slices", binary, max_bodies)
        cached = self._request_cache.get(cache_key)
        if cached is not None:
            return deepcopy(cached)
        targets = self.collection("verification_targets")
        blocks = self.collection("physical_blocks")
        rows: list[dict[str, Any]] = []
        seen_identities: set[str] = set()
        for symbol_id, symbol in self._symbols_for_binary(binary):
            if not symbol_authored_order_gate(symbol):
                continue
            if symbol_id in seen_identities:
                raise ProgressError(f"duplicate authored call-contract identity {symbol_id}")
            seen_identities.add(symbol_id)
            bounds = _entity_range(symbol)
            if bounds is None:
                raise ProgressError(
                    f"authored call-contract identity {symbol_id} has no known extent"
                )
            block_id = str(symbol.get("physical_block_id", ""))
            block = blocks.get(block_id)
            if not isinstance(block, Mapping):
                raise ProgressError(
                    f"authored call-contract identity {symbol_id} has no physical block"
                )
            if not self._block_authored_order_accepted(block):
                raise ProgressError(
                    f"authored call-contract identity {symbol_id} lacks accepted authored order"
                )
            facts = block.get("accepted_order_facts")
            if not isinstance(facts, Mapping) or facts.get("phase") != "authored-function-order":
                raise ProgressError(
                    f"authored call-contract block {block_id} lacks live authored-order facts"
                )
            target_id = str(facts.get("target_id", ""))
            target = targets.get(target_id)
            if not target_id or not isinstance(target, Mapping):
                raise ProgressError(
                    f"authored call-contract block {block_id} has unresolved accepted target identity"
                )
            matched = facts.get("matched_identities")
            if not isinstance(matched, list) or matched.count(symbol_id) != 1:
                raise ProgressError(
                    f"authored call-contract identity {symbol_id} must occur exactly once "
                    "in accepted authored-order facts"
                )
            registration = target.get("registration")
            if not isinstance(registration, Mapping):
                raise ProgressError(
                    f"authored call-contract target {target_id} has no registration"
                )
            address = normalize_address(bounds[0])
            raw_registered_addresses = target.get("registered_addresses")
            if not isinstance(raw_registered_addresses, list) or not raw_registered_addresses:
                raw_registered_addresses = registration.get("function_addresses", [])
            registered_addresses = {
                normalize_address(item)
                for item in raw_registered_addresses
                if isinstance(item, str)
            }
            if address not in registered_addresses:
                raise ProgressError(
                    f"authored call-contract identity {symbol_id} is absent from target {target_id}"
                )
            paths_by_key: dict[str, str] = {}

            def add_path(value: Any) -> None:
                if isinstance(value, str) and value:
                    # These tracker registration rows may be historical.  Keep
                    # their stored spelling intact so the current-manifest
                    # consumer can diagnose a stale alias; never repair them
                    # into a current-looking repository identity here.
                    paths_by_key.setdefault(value, value)

            raw_order_edit_paths = registration.get("order_edit_paths", [])
            if not isinstance(raw_order_edit_paths, list):
                raise ProgressError(
                    f"authored call-contract target {target_id} order_edit_paths must be a list"
                )
            for item in raw_order_edit_paths:
                add_path(item)
            for key in ("source_from",):
                value = registration.get(key)
                add_path(value)
            raw_translation_unit_order = registration.get(
                "translation_unit_function_order", []
            )
            if not isinstance(raw_translation_unit_order, list):
                raise ProgressError(
                    "authored call-contract target "
                    f"{target_id} translation_unit_function_order must be a list"
                )
            for entry in raw_translation_unit_order:
                if isinstance(entry, Mapping):
                    add_path(entry.get("source_from"))

            # Persisted target registration supplies a historical projection.
            # Legacy block source-shape labels are only a fail-closed fallback
            # when that projection has no implementation root.  Current Git
            # spelling is authenticated later from the executing worktree's
            # tracked manifest; this layer must not case-normalize old rows.
            exact_source_roots = {
                path
                for path in paths_by_key.values()
                if Path(path).suffix.casefold() in CALL_CONTRACT_SOURCE_SUFFIXES
            }
            if not exact_source_roots:
                for key in ("agent_source_path", "source_path"):
                    add_path(block.get(key))
                for item in block.get("source_shape_inputs", []):
                    add_path(item.get("path") if isinstance(item, Mapping) else item)
                for item in block.get("candidate_header_contributors", []):
                    add_path(item.get("path") if isinstance(item, Mapping) else item)
            if not paths_by_key:
                raise ProgressError(
                    f"authored call-contract target {target_id} has no source/header closure"
                )
            rows.append(
                {
                    "symbol_id": symbol_id,
                    "address": address,
                    "end_exclusive": normalize_address(bounds[1]),
                    "physical_block_id": block_id,
                    "target_id": target_id,
                    "source_paths": sorted(
                        paths_by_key.values(), key=lambda path: (path.casefold(), path)
                    ),
                }
            )

        slices: list[dict[str, Any]] = []
        for offset in range(0, len(rows), max_bodies):
            members = rows[offset : offset + max_bodies]
            if not members:
                continue
            first = members[0]["address"]
            last = members[-1]["address"]
            slice_id = f"recoil:call-contract-slice:{first}-{last}"
            slices.append(
                {
                    "id": slice_id,
                    "binary": binary,
                    "ordinal": len(slices) + 1,
                    "start": first,
                    "end_exclusive": members[-1]["end_exclusive"],
                    "body_count": len(members),
                    "symbol_ids": [item["symbol_id"] for item in members],
                    "addresses": [item["address"] for item in members],
                    "physical_block_ids": list(
                        dict.fromkeys(item["physical_block_id"] for item in members)
                    ),
                    "target_ids": list(
                        dict.fromkeys(item["target_id"] for item in members)
                    ),
                    "source_paths": sorted(
                        {
                            path
                            for item in members
                            for path in item["source_paths"]
                        }
                    ),
                }
            )
        self._request_cache[cache_key] = deepcopy(slices)
        return slices

    @staticmethod
    def _symbol_call_contract_current(symbol: Mapping[str, Any]) -> bool:
        return ProgressDocument._symbol_dimensions_current(
            symbol, (CALL_CONTRACT_DIMENSION,)
        )

    @staticmethod
    def _call_contract_state_evidence_id(symbol: Mapping[str, Any]) -> str:
        binary_state = symbol.get("binary_state")
        state = (
            binary_state.get(CALL_CONTRACT_DIMENSION)
            if isinstance(binary_state, Mapping)
            else None
        )
        evidence_ids = state.get("evidence_ids") if isinstance(state, Mapping) else None
        if (
            not isinstance(evidence_ids, list)
            or len(evidence_ids) != 1
            or not isinstance(evidence_ids[0], str)
            or not evidence_ids[0]
        ):
            return ""
        return evidence_ids[0]

    def call_contract_body_currentness(
        self,
        symbol_id: str,
    ) -> dict[str, Any]:
        """Return explicit-invalidation currentness for one accepted body."""
        symbol = self.collection("symbols").get(symbol_id)
        if not isinstance(symbol, Mapping):
            return {"current": False, "reason": "unknown-symbol"}
        if not self._symbol_call_contract_current(symbol):
            return {"current": False, "reason": "state-not-current"}
        evidence_id = self._call_contract_state_evidence_id(symbol)
        evidence = self.collection("evidence").get(evidence_id)
        if not evidence_id or not isinstance(evidence, Mapping):
            return {"current": False, "reason": "evidence-missing"}
        provenance = evidence.get("provenance")
        if (
            evidence.get("kind") != "live-authored-call-contract-validation"
            or evidence.get("result") != "passed"
            or evidence.get("disposition") != "accepted"
            or evidence.get("freshness") != "current"
            or evidence.get("validation_mode") != "live"
            or evidence.get("gating") is not True
            or evidence.get("scope_ids") != [symbol_id]
            or not isinstance(provenance, Mapping)
        ):
            return {"current": False, "reason": "evidence-not-current", "evidence_id": evidence_id}
        if not evidence_generations_current(provenance):
            return {"current": False, "reason": "verifier-generation-changed", "evidence_id": evidence_id}
        session = provenance.get("binary_ninja_session")
        if (
            provenance.get("symbol_id") != symbol_id
            or provenance.get("address")
            != normalize_address(str(symbol.get("address", symbol.get("start", ""))))
            or provenance.get("physical_block_id")
            != str(symbol.get("physical_block_id", ""))
            or provenance.get("comparison_passed") is not True
            or provenance.get("expected_contract") != provenance.get("candidate_contract")
            or not isinstance(session, Mapping)
            or session.get("snapshot_equal") is not True
            or session.get("begin") != session.get("end")
            or not isinstance(session.get("exact_fact_transcript"), list)
            or len(session["exact_fact_transcript"]) != 1
        ):
            return {"current": False, "reason": "body-evidence-binding-invalid", "evidence_id": evidence_id}
        return {
            "current": True,
            "reason": "accepted-and-not-invalidated",
            "evidence_id": evidence_id,
            **current_generations(),
        }

    def _call_contract_slice_status(self, slice_row: Mapping[str, Any]) -> dict[str, Any]:
        statuses = [
            {"symbol_id": str(symbol_id), **self.call_contract_body_currentness(str(symbol_id))}
            for symbol_id in slice_row["symbol_ids"]
        ]
        pending = [row for row in statuses if not row.get("current")]
        first_symbol_id = str(pending[0]["symbol_id"]) if pending else ""
        first_symbol = self.collection("symbols").get(first_symbol_id)
        return {
            "current": not pending,
            "storage_mode": "accepted-state-explicit-invalidation",
            "accepted_body_count": len(statuses) - len(pending),
            "remaining_body_count": len(pending),
            "body_statuses": statuses,
            "first_pending_symbol_id": first_symbol_id,
            "first_pending_address": str(first_symbol.get("address", first_symbol.get("start", ""))) if isinstance(first_symbol, Mapping) else "",
            "first_pending_physical_block_id": str(first_symbol.get("physical_block_id", "")) if isinstance(first_symbol, Mapping) else "",
        }

    _call_contract_slice_verification_status = _call_contract_slice_status

    def _call_contract_slice_current(self, slice_row: Mapping[str, Any]) -> bool:
        return bool(self._call_contract_slice_status(slice_row)["current"])

    def _call_contract_phase_closeout_status(self, slices: Sequence[Mapping[str, Any]]) -> dict[str, Any]:
        ordered = [str(symbol_id) for slice_row in slices for symbol_id in slice_row["symbol_ids"]]
        incomplete = [symbol_id for symbol_id in ordered if not self.call_contract_body_currentness(symbol_id).get("current")]
        migration = self.data.get("migration")
        closeout = migration.get("authored_call_contract_fresh_closeout_v2") if isinstance(migration, Mapping) else None
        valid = (
            not incomplete
            and isinstance(closeout, Mapping)
            and closeout.get("schema") == "recoil-call-contract-fresh-closeout-v2"
            and closeout.get("ordered_symbol_ids") == ordered
            and closeout.get("complete_no_reuse_zero_divergence") is True
            and evidence_generations_current(closeout)
        )
        return {
            "current": valid,
            "reason": "current-clean-closeout" if valid else "fresh-complete-scan-required",
            "ordered_symbol_ids": ordered,
            "incomplete_symbol_ids": incomplete,
            **current_generations(),
        }
    def _live_call_contract_command(self, slice_id: str) -> str:
        del slice_id
        # Scheduler projections do not own an explicit packet/reservation and
        # therefore cannot render an applying call-contract command.
        return ""

    def pipeline(
        self,
        binary: str = "recoil",
        *,
        resolve_order_target: bool = True,
    ) -> dict[str, Any]:
        binary_row = self.collection("binaries").get(binary, {})
        if not isinstance(binary_row, Mapping):
            raise ProgressError(f"unknown binary {binary}")
        text = binary_row.get("text", {})
        try:
            text_start = normalize_address(text.get("start", "0x401000"))
            text_end = normalize_address(text.get("end_exclusive", "0x4cb9e8"))
        except ProgressError as exc:
            raise ProgressError(f"binary {binary} has invalid text bounds") from exc
        blocks = self._blocks_for_binary(binary)

        authored_pending = [
            (block_id, block)
            for block_id, block in blocks
            if not self._order_group_current(block, "authored", AUTHORED_ORDER_DIMENSIONS)
        ]
        full_pending = [
            (block_id, block)
            for block_id, block in blocks
            if not self._order_group_current(block, "full", FULL_ORDER_DIMENSIONS)
        ]
        authored_order_end = (
            normalize_address(_entity_range(authored_pending[0][1])[0])
            if authored_pending
            else text_end
        )
        full_order_end = (
            normalize_address(_entity_range(full_pending[0][1])[0])
            if full_pending
            else text_end
        )

        authored_groups = self._physical_groups(binary, gating_only=True)
        authored_pending_groups = [
            group
            for group in authored_groups
            if not all(
                self._symbol_dimensions_current(symbol, AUTHORED_BYTE_DIMENSIONS)
                for symbol in group["symbols"]
            )
        ]
        authored_byte_cursor = authored_pending_groups[0]["address"] if authored_pending_groups else ""
        authored_byte_end = authored_byte_cursor or text_end

        linked_groups = self._physical_groups(binary, gating_only=False)
        linked_pending_groups = [
            group
            for group in linked_groups
            if not all(
                self._symbol_dimensions_current(symbol, EXACT_LINK_DIMENSIONS)
                for symbol in group["symbols"]
            )
        ]
        linked_byte_cursor = linked_pending_groups[0]["address"] if linked_pending_groups else ""
        linked_byte_end = linked_byte_cursor or text_end

        eligible_end = address_value(authored_order_end)
        object_groups = self._physical_groups(binary, gating_only=True, eligible_end=eligible_end)
        object_pending_groups = [
            group
            for group in object_groups
            if not all(
                self._symbol_dimensions_current(symbol, ("object_byte",))
                for symbol in group["symbols"]
            )
        ]
        object_cursor = object_pending_groups[0]["address"] if object_pending_groups else ""

        call_contract_slices: list[dict[str, Any]] = []
        call_contract_pending: list[dict[str, Any]] = []
        call_contract_closeout: dict[str, Any] = {
            "current": False,
            "reason": "not-applicable",
        }
        call_contract_closeout_pending = False
        call_contract_derivation_error = ""
        gating_symbol_rows = [
            symbol
            for _symbol_id, symbol in self._symbols_for_binary(binary)
            if symbol_authored_order_gate(symbol)
        ]
        initialized_call_contract_bodies = sum(
            1
            for symbol in gating_symbol_rows
            if isinstance(symbol.get("binary_state"), Mapping)
            and CALL_CONTRACT_DIMENSION in symbol["binary_state"]
        )
        evidence_rows = self.collection("evidence")
        verification_evidence_call_contract_bodies = sum(
            1
            for symbol in gating_symbol_rows
            if (
                (evidence_id := self._call_contract_state_evidence_id(symbol))
                and isinstance(evidence_rows.get(evidence_id), Mapping)
                and evidence_rows[evidence_id].get("kind")
                == CALL_CONTRACT_ACCEPTANCE_EVIDENCE_KIND
            )
        )
        call_contract_stage_enabled = (
            bool(gating_symbol_rows)
            and initialized_call_contract_bodies == len(gating_symbol_rows)
        )
        if initialized_call_contract_bodies not in {0, len(gating_symbol_rows)}:
            call_contract_derivation_error = (
                "authored call-contract migration is partial: "
                f"{initialized_call_contract_bodies}/{len(gating_symbol_rows)} gating bodies initialized"
            )
        if not authored_pending and (call_contract_stage_enabled or call_contract_derivation_error):
            try:
                call_contract_slices = self.authored_call_contract_slices(binary)
            except ProgressError as exc:
                call_contract_derivation_error = str(exc)
            else:
                for item in call_contract_slices:
                    verification_status = self._call_contract_slice_verification_status(
                        item
                    )
                    if verification_status["current"]:
                        continue
                    call_contract_pending.append(
                        {
                            **item,
                            "verification_status": verification_status,
                            "accepted_body_count": verification_status[
                                "accepted_body_count"
                            ],
                            "remaining_body_count": verification_status[
                                "remaining_body_count"
                            ],
                            "pending_start": verification_status[
                                "first_pending_address"
                            ],
                            "pending_physical_block_id": verification_status[
                                "first_pending_physical_block_id"
                            ],
                        }
                    )
                if not call_contract_pending and call_contract_slices:
                    call_contract_closeout = self._call_contract_phase_closeout_status(
                        call_contract_slices
                    )
                    call_contract_closeout_pending = not bool(
                        call_contract_closeout.get("current")
                    )

        call_contract_convergence: dict[str, Any] = {
            "mode": "not-applicable",
            "generation_state": "not-applicable",
            "current": False,
            "reason": "",
            "failed_target_count": 0,
            "repair_descriptor_count": 0,
            "dependent_owner_repair_target_count": 0,
            "dependent_owner_blocker_target_count": 0,
            "dependent_header_blocker_target_count": 0,
            "retail_blocker_target_count": 0,
            "retail_blocker_caller_count": 0,
            "retail_fact_packet_count": 0,
        }
        if not authored_pending and (
            call_contract_pending
            or call_contract_closeout_pending
            or call_contract_derivation_error
        ):
            if call_contract_derivation_error:
                call_contract_convergence.update(
                    {
                        "mode": "readiness-blocked",
                        "generation_state": "blocked",
                        "reason": call_contract_derivation_error,
                    }
                )
            else:
                from _recoil.commands.call_contract_convergence import (
                    convergence_scheduler_mode,
                    convergence_generation_state,
                    retail_fact_packet_scopes,
                )
                generation_state = convergence_generation_state(
                    self, copy_generation=False
                )
                generation = generation_state.get("generation")
                mode = convergence_scheduler_mode(
                    readiness_passed=True,
                    generation_state=generation_state,
                )
                repairs = (
                    generation.get("repair_descriptors", [])
                    if isinstance(generation, Mapping)
                    else []
                )
                retail_blockers = (
                    generation.get("retail_blocker_descriptors", [])
                    if isinstance(generation, Mapping)
                    else []
                )
                dependent_repairs = (
                    generation.get("dependent_owner_repair_descriptors", [])
                    if isinstance(generation, Mapping)
                    else []
                )
                dependent_blockers = (
                    generation.get("dependent_owner_blocker_descriptors", [])
                    if isinstance(generation, Mapping)
                    else []
                )
                dependent_header_blockers = (
                    generation.get("dependent_header_blocker_descriptors", [])
                    if isinstance(generation, Mapping)
                    else []
                )
                metrics = (
                    generation.get("metrics", {})
                    if isinstance(generation, Mapping)
                    else {}
                )
                call_contract_convergence.update(
                    {
                        "mode": mode,
                        "generation_state": generation_state.get("state", ""),
                        "current": bool(generation_state.get("current")),
                        "reason": str(generation_state.get("reason", "")),
                        "generation_id": (
                            str(generation.get("generation_id", ""))
                            if isinstance(generation, Mapping)
                            else ""
                        ),
                        "failed_target_count": int(
                            metrics.get("failed_target_count", 0)
                        ),
                        "repair_descriptor_count": (
                            len(repairs) if isinstance(repairs, list) else 0
                        ),
                        "dependent_owner_repair_target_count": (
                            len(dependent_repairs)
                            if isinstance(dependent_repairs, list)
                            else 0
                        ),
                        "dependent_owner_blocker_target_count": (
                            len(dependent_blockers)
                            if isinstance(dependent_blockers, list)
                            else 0
                        ),
                        "dependent_header_blocker_target_count": (
                            len(dependent_header_blockers)
                            if isinstance(dependent_header_blockers, list)
                            else 0
                        ),
                        "required_parent_action": (
                            "claim-retail-fact-packets"
                            if mode == "retail-blocked"
                            else (
                                "resolve-dependent-owner-ambiguity"
                                if mode == "dependent-owner-blocked"
                                else (
                                    "resolve-dependent-header-routing"
                                    if mode == "dependent-header-blocked"
                                    else ""
                                )
                            )
                        ),
                        "retail_blocker_target_count": (
                            len(retail_blockers)
                            if isinstance(retail_blockers, list)
                            else 0
                        ),
                        "retail_blocker_caller_count": int(
                            metrics.get("retail_blocker_caller_count", 0)
                        ),
                        "retail_fact_packet_count": (
                            len(retail_fact_packet_scopes(retail_blockers))
                            if isinstance(retail_blockers, list)
                            else 0
                        ),
                    }
                )

        if authored_pending:
            phase = "authored-function-order"
            primary_lane = "order"
            block_id, block = authored_pending[0]
            cursor = normalize_address(_entity_range(block)[0])
        elif call_contract_derivation_error:
            phase = "authored-call-contract"
            primary_lane = "call-contract"
            cursor = authored_order_end
            block_id = ""
            block = {}
        elif call_contract_pending:
            phase = "authored-call-contract"
            primary_lane = "call-contract"
            cursor = str(
                call_contract_pending[0].get(
                    "pending_start", call_contract_pending[0]["start"]
                )
            )
            block_id = str(
                call_contract_pending[0].get(
                    "pending_physical_block_id",
                    call_contract_pending[0]["physical_block_ids"][0],
                )
            )
            block = self.collection("physical_blocks").get(block_id, {})
        elif call_contract_closeout_pending:
            phase = "authored-call-contract"
            primary_lane = "call-contract"
            cursor = authored_order_end
            block_id = ""
            block = {}
        elif full_pending:
            phase = "full-function-order"
            primary_lane = "order"
            block_id, block = full_pending[0]
            cursor = normalize_address(_entity_range(block)[0])
        elif authored_pending_groups:
            phase = "authored-byte-match"
            primary_lane = "authored-byte"
            cursor = authored_byte_cursor
            block_id = str(authored_pending_groups[0]["physical_block_id"])
            block = self.collection("physical_blocks").get(block_id, {})
        elif linked_pending_groups:
            phase = "linked-byte-match"
            primary_lane = "linked-byte"
            cursor = linked_byte_cursor
            block_id = str(linked_pending_groups[0]["physical_block_id"])
            block = self.collection("physical_blocks").get(block_id, {})
        else:
            phase = "final-validation"
            primary_lane = "final"
            cursor = text_end
            block_id = ""
            block = {}

        authored_lane_state = "complete" if not authored_byte_cursor else "ready"
        authored_lane_reason = ""
        active_source_edit_blocks = {block_id} if primary_lane == "order" else set()
        if primary_lane == "call-contract" and call_contract_pending:
            if call_contract_convergence.get("mode") == "repairing/failed-targets":
                from _recoil.commands.call_contract_convergence import (
                    convergence_generation_state,
                    dependent_owner_repair_launchability,
                )

                current_generation = generation_state.get("generation")
                descriptors = (
                    current_generation.get("repair_descriptors", [])
                    if isinstance(current_generation, Mapping)
                    else []
                )
                dependent_descriptors = (
                    current_generation.get(
                        "dependent_owner_repair_descriptors", []
                    )
                    if isinstance(current_generation, Mapping)
                    else []
                )
                retail_descriptors = (
                    current_generation.get("retail_blocker_descriptors", [])
                    if isinstance(current_generation, Mapping)
                    else []
                )
                retail_blocks = {
                    str(block_id)
                    for descriptor in retail_descriptors
                    if isinstance(descriptor, Mapping)
                    for block_id in descriptor.get("physical_block_ids", [])
                }
                dependent_blockers = (
                    current_generation.get(
                        "dependent_owner_blocker_descriptors", []
                    )
                    if isinstance(current_generation, Mapping)
                    else []
                )
                dependent_blockers_by_target: dict[
                    str, list[Mapping[str, Any]]
                ] = {}
                for descriptor in dependent_blockers:
                    if isinstance(descriptor, Mapping):
                        dependent_blockers_by_target.setdefault(
                            str(descriptor.get("target_id", "")), []
                        ).append(descriptor)
                target_results_by_id = {
                    str(row.get("target_id", "")): row
                    for row in (
                        current_generation.get("target_results", [])
                        if isinstance(current_generation, Mapping)
                        else []
                    )
                    if isinstance(row, Mapping)
                }
                launchable_dependent_descriptors: list[Mapping[str, Any]] = []
                for descriptor in dependent_descriptors:
                    if not isinstance(descriptor, Mapping):
                        continue
                    target_id = str(descriptor.get("target_id", ""))
                    target_result = target_results_by_id.get(target_id)
                    target_has_divergence_partition = bool(
                        isinstance(target_result, Mapping)
                        and isinstance(
                            target_result.get("caller_divergences"), list
                        )
                    )
                    unmatched_divergences = (
                        list(target_result.get("caller_divergences", []))
                        if target_has_divergence_partition
                        else []
                    )
                    partition_malformed = False
                    if target_has_divergence_partition:
                        for routed_divergence in descriptor.get(
                            "caller_divergences", []
                        ):
                            try:
                                unmatched_divergences.remove(
                                    routed_divergence
                                )
                            except ValueError:
                                partition_malformed = True
                                break
                    launchability: list[Mapping[str, Any]] = []
                    if partition_malformed:
                        launchability.append(
                            {
                                "state": "blocked",
                                "reason_code": (
                                    "malformed-target-wide-repair-partition"
                                ),
                            }
                        )
                    elif unmatched_divergences:
                        launchability.append(
                            dependent_owner_repair_launchability(
                                descriptor,
                                {
                                    "target_id": target_id,
                                    "divergences": unmatched_divergences,
                                },
                            )
                        )
                    launchability.extend(
                        dependent_owner_repair_launchability(
                            descriptor, blocker
                        )
                        for blocker in dependent_blockers_by_target.get(
                            target_id, []
                        )
                    )
                    if all(
                        row.get("state") == "launchable"
                        for row in launchability
                    ):
                        launchable_dependent_descriptors.append(descriptor)
                active_source_edit_blocks = {
                    str(block_id)
                    for descriptor in (
                        *descriptors,
                        *launchable_dependent_descriptors,
                    )
                    if isinstance(descriptor, Mapping)
                    for block_id in descriptor.get("physical_block_ids", [])
                } - retail_blocks
            else:
                active_source_edit_blocks = set()
        if (
            authored_byte_cursor
            and primary_lane in {"order", "call-contract"}
            and str(authored_pending_groups[0].get("physical_block_id"))
            in active_source_edit_blocks
        ):
            authored_lane_state = "blocked"
            authored_lane_reason = (
                "retail byte frontier shares an active primary source-edit physical block"
            )
        parallel_authored = (
            authored_byte_cursor
            if primary_lane in {"order", "call-contract"} and authored_lane_state == "ready"
            else ""
        )
        object_lane_state = "ready" if object_cursor else "caught-up"
        object_lane_reason = "" if object_cursor else "waiting for the accepted authored-order prefix to advance"
        if (
            object_cursor
            and primary_lane in {"order", "call-contract"}
            and str(object_pending_groups[0].get("physical_block_id"))
            in active_source_edit_blocks
        ):
            object_lane_state = "blocked"
            object_lane_reason = (
                "object byte frontier shares an active primary source-edit physical block"
            )
        parallel_object = (
            object_cursor
            if primary_lane in {"order", "call-contract"} and object_lane_state == "ready"
            else ""
        )

        order_target_resolution: dict[str, Any] = {"status": "not-applicable", "phase": phase}
        if primary_lane == "order":
            if resolve_order_target and isinstance(block, Mapping):
                order_target_resolution = self._order_target_resolution()
                next_command = self._live_order_command(phase, order_target_resolution)
            else:
                next_command = ""
                order_target_resolution = {"status": "deferred", "phase": phase}
        elif primary_lane == "call-contract":
            from _recoil.commands.call_contract_convergence import (
                convergence_next_action,
            )

            convergence_mode = str(call_contract_convergence.get("mode", ""))
            next_command = convergence_next_action(
                convergence_mode,
                revision=self.revision,
                prepare_root="",
                acceptance_command=(
                    self._live_call_contract_command(
                        str(call_contract_pending[0]["id"])
                    )
                    if call_contract_pending
                    else ""
                ),
            )
        elif primary_lane == "authored-byte":
            next_command = self._live_byte_command("authored", cursor)
        elif primary_lane == "linked-byte":
            next_command = self._live_byte_command("linked", cursor)
        else:
            next_command = "python tools/recoil.py verify final-image --json"

        total_authored = len(authored_groups)
        total_object = len(object_groups)
        total_blocks = len(blocks)
        total_linked = len(linked_groups)
        accepted_authored = total_authored - len(authored_pending_groups)
        accepted_object = total_object - len(object_pending_groups)
        accepted_authored_order = total_blocks - len(authored_pending)
        accepted_full_order = total_blocks - len(full_pending)
        accepted_linked = total_linked - len(linked_pending_groups)
        total_call_contract = sum(
            int(item["body_count"]) for item in call_contract_slices
        )
        remaining_call_contract = sum(
            int(item.get("remaining_body_count", item["body_count"]))
            for item in call_contract_pending
        )
        accepted_call_contract = total_call_contract - remaining_call_contract
        complete = phase == "final-validation" and not linked_pending_groups
        call_contract_containment = (
            {
                "launchable": False,
                "next_command": None,
                "blocker": (
                    "call-contract acceptance/convergence requires an explicit "
                    "active packet and reservation"
                ),
            }
            if primary_lane == "call-contract" and not next_command
            else None
        )
        state = {
            "binary": binary,
            "phase": phase,
            "primary_lane": primary_lane,
            "cursor": cursor,
            "physical_block_id": block_id,
            "complete": complete,
            "text_start": text_start,
            "text_end_exclusive": text_end,
            "authored_order_prefix_end": authored_order_end,
            "authored_function_order_prefix_end": authored_order_end,
            "authored_object_order_prefix_end": authored_order_end,
            "authored_object_byte_eligible_order_prefix_end": authored_order_end,
            "full_order_prefix_end": full_order_end,
            "full_function_order_prefix_end": full_order_end,
            "order_prefix_end": full_order_end,
            "authored_byte_cursor": authored_byte_cursor,
            "authored_call_contract_cursor": (
                str(
                    call_contract_pending[0].get(
                        "pending_start", call_contract_pending[0]["start"]
                    )
                )
                if call_contract_pending
                else ""
            ),
            "authored_call_contract_slice_id": (
                str(call_contract_pending[0]["id"]) if call_contract_pending else ""
            ),
            "authored_byte_match_frontier": authored_byte_end,
            "byte_prefix_end": authored_byte_end,
            "exact_byte_prefix_end": linked_byte_end,
            "linked_byte_match_prefix_end": linked_byte_end,
            "parallel_authored_byte_cursor": parallel_authored,
            "fallback_authored_byte_cursor": parallel_authored,
            "parallel_authored_object_byte_cursor": parallel_object,
            "authored_function_order_counts": {
                "accepted": accepted_authored_order,
                "remaining": len(authored_pending),
                "total": total_blocks,
            },
            "full_function_order_counts": {
                "accepted": accepted_full_order,
                "remaining": len(full_pending),
                "total": total_blocks,
            },
            "authored_call_contract_counts": {
                "accepted": accepted_call_contract,
                "remaining": remaining_call_contract,
                "total": total_call_contract,
                "slice_count": len(call_contract_slices),
                "remaining_slices": len(call_contract_pending),
                "initialized_bodies": initialized_call_contract_bodies,
                "verification_evidence_bodies": (
                    verification_evidence_call_contract_bodies
                ),
                "stage_enabled": call_contract_stage_enabled,
            },
            "authored_call_contract_lane": {
                "state": (
                    "blocked"
                    if call_contract_derivation_error
                    else (
                        "ready"
                        if call_contract_pending
                        else (
                            "awaiting-global-clean-closeout"
                            if call_contract_closeout_pending
                            else "complete"
                        )
                    )
                ),
                "cursor": (
                    str(
                        call_contract_pending[0].get(
                            "pending_start", call_contract_pending[0]["start"]
                        )
                    )
                    if call_contract_pending
                    else ""
                ),
                "slice_id": (
                    str(call_contract_pending[0]["id"]) if call_contract_pending else ""
                ),
                "body_count": (
                    int(
                        call_contract_pending[0].get(
                            "remaining_body_count",
                            call_contract_pending[0]["body_count"],
                        )
                    )
                    if call_contract_pending
                    else 0
                ),
                "blocked_reason": call_contract_derivation_error,
            },
            "authored_call_contract_closeout": call_contract_closeout,
            "authored_call_contract_convergence": call_contract_convergence,
            "authored_byte_counts": {
                "accepted": accepted_authored,
                "remaining": len(authored_pending_groups),
                "total": total_authored,
            },
            "authored_object_byte_counts": {
                "accepted": accepted_object,
                "remaining": len(object_pending_groups),
                "eligible": total_object,
                "blocked_groups": 1 if object_lane_state == "blocked" else 0,
            },
            "linked_byte_counts": {
                "accepted": accepted_linked,
                "remaining": len(linked_pending_groups),
                "total": total_linked,
            },
            "authored_byte_lane": {
                "state": authored_lane_state,
                "cursor": authored_byte_cursor,
                "symbol_id": authored_pending_groups[0]["scope_ids"][0] if authored_pending_groups else "",
                "physical_block_id": str(authored_pending_groups[0]["physical_block_id"]) if authored_pending_groups else "",
                "frontier_kind": "gating-body" if authored_pending_groups else "complete",
                "blocked_reason": authored_lane_reason,
                "conflicting_order_block_id": block_id if authored_lane_state == "blocked" else "",
            },
            "authored_object_byte_lane": {
                "state": object_lane_state,
                "cursor": object_cursor,
                "symbol_id": object_pending_groups[0]["scope_ids"][0] if object_pending_groups else "",
                "physical_block_id": str(object_pending_groups[0]["physical_block_id"]) if object_pending_groups else "",
                "eligibility": "complete-block-inside-current-authored-order-prefix",
                "blocked_reason": object_lane_reason,
            },
            "next_command": next_command,
            "call_contract_containment": call_contract_containment,
            "order_target_resolution": order_target_resolution,
            "primary_phase_progress": {
                "kind": "accepted-prefix-end",
                "phase": phase,
                "value": cursor,
            },
        }
        return self.scheduler_output(state)

    def next_work(
        self,
        binary: str = "recoil",
        *,
        issue_ledger: str | Path | None = None,
    ) -> dict[str, Any]:
        state = self.pipeline(binary)
        from _recoil.commands.progress_cli import (
            DEFAULT_ISSUE_LEDGER,
            describe_current_claim_opportunities,
        )

        resolved_issue_ledger = Path(
            DEFAULT_ISSUE_LEDGER
            if issue_ledger is None
            else issue_ledger
        )
        state.update(
            describe_current_claim_opportunities(
                self,
                state,
                issue_ledger=resolved_issue_ledger,
            )
        )
        return state

    def parallel_authored_byte_work(self, binary: str = "recoil") -> dict[str, Any] | None:
        state = self.next_work(binary)
        cursor = state.get("parallel_authored_byte_cursor")
        if not cursor:
            return None
        return self.scheduler_output(
            {
                "binary": binary,
                "phase": "authored-byte-match",
                "cursor": cursor,
                "next_command": self._live_byte_command("authored", str(cursor)),
                "frontier_relation": "parallel-authored-byte",
            }
        )

    def fallback_authored_byte_work(self, binary: str = "recoil") -> dict[str, Any] | None:
        return self.parallel_authored_byte_work(binary)

    def parallel_authored_object_byte_work(self, binary: str = "recoil") -> dict[str, Any] | None:
        state = self.next_work(binary)
        cursor = state.get("parallel_authored_object_byte_cursor")
        if not cursor:
            return None
        return self.scheduler_output(
            {
                "binary": binary,
                "phase": "authored-byte-match",
                "cursor": cursor,
                "next_command": self._live_byte_command("object", str(cursor)),
                "frontier_relation": "parallel-authored-object-byte",
            }
        )

    def show(self, selector: str) -> dict[str, Any]:
        matches: list[dict[str, Any]] = []
        for collection_name in TOP_LEVEL_COLLECTIONS:
            collection = self.collection(collection_name)
            if selector in collection:
                matches.append(
                    {"collection": collection_name, "id": selector, "record": deepcopy(collection[selector])}
                )
        try:
            point = address_value(selector)
        except ProgressError:
            point = None
        if point is not None:
            for collection_name in (
                "physical_blocks",
                "semantic_spans",
                "symbols",
                "storage_contributions",
            ):
                for entity_id, row in self.collection(collection_name).items():
                    if not isinstance(row, Mapping):
                        continue
                    bounds = _entity_range(row)
                    if bounds and bounds[0] <= point < bounds[1]:
                        matches.append(
                            {"collection": collection_name, "id": entity_id, "record": deepcopy(row)}
                        )
        result = {"selector": selector, "matches": matches}
        if any(_entity_binary(item["record"]) == "recoil" for item in matches if isinstance(item["record"], Mapping)):
            result["pipeline"] = self.pipeline("recoil")
        return self.scheduler_output(result)

    def find(self, query: str, limit: int = 100) -> dict[str, Any]:
        needle = query.casefold()
        matches: list[dict[str, Any]] = []
        for collection_name in TOP_LEVEL_COLLECTIONS:
            for entity_id, row in self.collection(collection_name).items():
                if needle in entity_id.casefold() or needle in json.dumps(row, ensure_ascii=False).casefold():
                    matches.append({"collection": collection_name, "id": entity_id, "record": deepcopy(row)})
                    if len(matches) >= limit:
                        return self.scheduler_output({"query": query, "matches": matches, "truncated": True})
        return self.scheduler_output({"query": query, "matches": matches, "truncated": False})

    def summary(self) -> dict[str, Any]:
        return self.scheduler_output(
            {
                "schema_version": self.data.get("schema_version"),
                "counts": {name: len(self.collection(name)) for name in TOP_LEVEL_COLLECTIONS},
                "pipeline": self.pipeline("recoil") if "recoil" in self.collection("binaries") else None,
            }
        )

    def audit(self, scope: str = "all") -> list[Finding]:
        del scope
        findings: list[Finding] = []
        if self.data.get("schema_version") != SCHEMA_VERSION:
            findings.append(Finding("error", "schema.version", f"schema_version must be {SCHEMA_VERSION}"))
        if self.revision < 0:
            findings.append(Finding("error", "revision.invalid", "revision must be a non-negative integer"))
        for name in TOP_LEVEL_COLLECTIONS:
            if not isinstance(self.data.get(name), dict):
                findings.append(Finding("error", "collection.shape", f"{name} must be an object", name))
        evidence = self.data.get("evidence", {})
        if isinstance(evidence, Mapping):
            for evidence_id, record in evidence.items():
                if EVIDENCE_ID_RE.fullmatch(str(evidence_id)) is None:
                    findings.append(Finding("error", "evidence.id", "invalid revision-scoped evidence id", str(evidence_id)))
                if not isinstance(record, Mapping):
                    findings.append(Finding("error", "evidence.shape", "evidence record must be an object", str(evidence_id)))
                    continue
                if record.get("freshness") not in FRESHNESS_STATES:
                    findings.append(Finding("error", "evidence.freshness", "invalid evidence freshness", str(evidence_id)))
                if record.get("freshness") == "current" and record.get("validation_mode") != "live":
                    findings.append(Finding("error", "evidence.live", "current evidence must come from a live validator", str(evidence_id)))
        from _recoil.lib.authored_icf import audit_authored_icf_groups

        for code, message in audit_authored_icf_groups(self.data):
            findings.append(Finding("error", code, message))
        return findings


def audit_work_items(data: Mapping[str, Any]) -> list[Finding]:
    findings: list[Finding] = []
    rows = data.get("work_items", {})
    if not isinstance(rows, Mapping):
        return [Finding("error", "work.collection", "work_items must be an object")]
    for work_id, work in rows.items():
        if not isinstance(work, Mapping):
            findings.append(Finding("error", "work.shape", "work item must be an object", str(work_id)))
            continue
        claims, complete, _source = work_resource_claims(work)
        if work.get("state") in WORK_HANDOFF_STATES and (not complete or not claims):
            findings.append(Finding("error", "work.claims", "schedulable work requires complete resource claims", str(work_id)))
        retail_fact_problem = retail_fact_packet_contract_problem(work, claims)
        if retail_fact_problem:
            findings.append(
                Finding(
                    "error",
                    "work.retail-fact-contract",
                    retail_fact_problem,
                    str(work_id),
                )
            )
    return findings


def validate_owner_invariants(data: Mapping[str, Any]) -> None:
    """Run the canonical source-owner invariant set against unified progress data."""
    from _recoil.lib.source_owners import SourceOwnerDocument, _project_progress_owners

    try:
        projected = _project_progress_owners(deepcopy(dict(data)))
        findings = SourceOwnerDocument(Path("<proposed-unified-progress>"), projected).validate()
    except (ValueError, TypeError, KeyError) as exc:
        raise ProgressError(f"cannot project proposed source owners: {exc}") from exc
    if findings:
        raise ProgressError(
            "proposed source owners failed canonical invariants: " + "; ".join(findings[:12])
        )


def _explicit_string_list(value: Any, field: str) -> list[str]:
    if not isinstance(value, list) or any(
        not isinstance(item, str) or not item.strip() for item in value
    ):
        raise ProgressError(f"explicit maintenance {field} must be a string list")
    normalized = [item.strip() for item in value]
    if len(set(normalized)) != len(normalized):
        raise ProgressError(f"explicit maintenance {field} contains duplicates")
    return normalized


def _explicit_nonempty_text(value: Any, field: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ProgressError(f"explicit maintenance {field} must be non-empty")
    return value.strip()


def _explicit_exact_file(
    value: str,
    *,
    field: str,
    repo_root: Path,
    inventory: GitTrackedPathInventory,
    source_only: bool = False,
    allow_proposed_destination: bool = False,
) -> str:
    if any(token in value for token in ("*", "?", "[", "]")):
        raise ProgressError(f"explicit maintenance {field} forbids globs: {value!r}")
    try:
        resolved = resolve_tracked_repository_file(
            value,
            repository_root=repo_root,
            inventory=inventory,
            context=f"explicit maintenance {field}",
            allowed_suffixes=EXPLICIT_SOURCE_SUFFIXES if source_only else None,
        )
    except RepositoryPathError as exc:
        if allow_proposed_destination and exc.kind == "untracked-path":
            try:
                proposed = validate_repository_relative_path(
                    value,
                    context=f"explicit maintenance {field} proposed destination",
                )
            except RepositoryPathError as lexical_exc:
                raise _repository_path_error(lexical_exc) from lexical_exc
            candidate = repo_root.joinpath(*proposed.split("/"))
            parent = candidate.parent
            try:
                parent_resolved = parent.resolve(strict=True)
                parent_resolved.relative_to(repo_root.resolve(strict=True))
            except (OSError, ValueError) as parent_exc:
                raise ProgressError(
                    f"explicit maintenance {field} proposed destination has no safe repository parent: {proposed}"
                ) from parent_exc
            if _path_has_reparse_component(parent_resolved, stop=repo_root):
                raise ProgressError(
                    f"explicit maintenance {field} proposed destination has a reparse parent: {proposed}"
                )
            if candidate.exists() and (
                not candidate.is_file()
                or _path_has_reparse_component(candidate, stop=repo_root)
            ):
                raise ProgressError(
                    f"explicit maintenance {field} proposed destination is not an ordinary file path: {proposed}"
                )
            if source_only and (
                not proposed.startswith("src/")
                or Path(proposed).suffix.casefold() not in EXPLICIT_SOURCE_SUFFIXES
            ):
                raise ProgressError(
                    "explicit maintenance writable path is not an exact C/C++ "
                    f"source/header: {proposed}"
                )
            return proposed
        raise _repository_path_error(exc) from exc
    if resolved.git_path in {"src", "build", "support", "tools", ".agent"}:
        raise ProgressError(
            f"explicit maintenance {field} forbids broad roots: {resolved.git_path}"
        )
    if source_only and not resolved.git_path.startswith("src/"):
        raise ProgressError(
            "explicit maintenance writable path is not an exact C/C++ "
            f"source/header: {resolved.git_path}"
        )
    return resolved.git_path


def _explicit_machine_local_file(
    value: str,
    *,
    field: str,
    repo_root: Path,
) -> str:
    """Validate a configured machine-local file without assigning Git identity."""

    try:
        logical = validate_repository_relative_path(
            value,
            context=f"explicit maintenance {field}",
        )
    except RepositoryPathError as exc:
        raise _repository_path_error(exc) from exc
    candidate = repo_root.joinpath(*logical.split("/"))
    try:
        resolved = candidate.resolve(strict=True)
        resolved.relative_to(repo_root.resolve(strict=True))
    except (OSError, ValueError) as exc:
        raise ProgressError(
            f"explicit maintenance {field} is unavailable or escapes the repository: {logical}"
        ) from exc
    if not resolved.is_file() or _path_has_reparse_component(resolved, stop=repo_root):
        raise ProgressError(
            f"explicit maintenance {field} must name an ordinary non-reparse file: {logical}"
        )
    return logical


def _record_path_values(record: Mapping[str, Any], field_names: set[str]) -> list[str]:
    result: list[str] = []

    def visit(value: Any) -> None:
        if isinstance(value, Mapping):
            for key, item in value.items():
                if str(key) in field_names:
                    if isinstance(item, str) and item.strip():
                        result.append(item.strip())
                    elif isinstance(item, list):
                        result.extend(
                            child.strip()
                            for child in item
                            if isinstance(child, str) and child.strip()
                        )
                elif isinstance(item, (Mapping, list)):
                    visit(item)
        elif isinstance(value, list):
            for item in value:
                visit(item)

    visit(record)
    return result


def _path_has_reparse_component(path: Path, *, stop: Path) -> bool:
    """Reject symlink/reparse traversal between ``stop`` and ``path``."""

    current = path
    stop_resolved = stop.resolve()
    while True:
        try:
            current.relative_to(stop_resolved)
        except ValueError:
            return True
        if current.exists():
            try:
                stat_result = current.lstat()
            except OSError:
                return True
            if current.is_symlink() or (
                int(getattr(stat_result, "st_file_attributes", 0)) & 0x400
            ):
                return True
        if current == stop_resolved:
            return False
        current = current.parent


def explicit_output_sidecar_path(output_root: str) -> str:
    """Return the deterministic journal-owned sidecar for one output root."""

    normalized = PurePosixPath(
        _generated_repository_path(
            output_root,
            context="explicit output root",
        )
    )
    if not normalized.name or str(normalized.parent) in {"", "."}:
        raise ProgressError("explicit output root cannot derive an ownership sidecar")
    return str(
        normalized.parent
        / f".{normalized.name}.recoil-explicit-allocation-owner.json"
    )


def explicit_output_allocation_record(
    *,
    packet_id: str,
    reservation_id: str,
    output_root: str,
    progress_path: str | Path,
    operation_nonce: str,
    issue_ledger_identity: Mapping[str, Any],
    repo_root: Path = REPO_ROOT,
) -> dict[str, Any]:
    """Return the exact immutable marker/provenance payload for one root."""

    progress = Path(progress_path).resolve()
    try:
        from _recoil.lib.progress_sqlite import read_progress_metadata

        metadata = read_progress_metadata(progress)
        tracker_identity: dict[str, Any] = {
            "storage_kind": "sqlite",
            "path": str(progress),
            "application_id": metadata.application_id,
            "user_version": metadata.user_version,
            "schema_version": metadata.schema_version,
            "cutover_pair_id": metadata.cutover_pair_id,
        }
    except Exception as exc:
        raise ProgressError(
            f"explicit output allocation requires an authenticated SQLite tracker: {exc}"
        ) from exc
    normalized_output_root = _generated_repository_path(
        output_root,
        context="explicit output root",
    )
    if not isinstance(issue_ledger_identity, Mapping) or not issue_ledger_identity:
        raise ProgressError("explicit output allocation requires an issue-ledger identity")
    return {
        "schema": EXPLICIT_OUTPUT_MARKER_SCHEMA,
        "packet_id": packet_id,
        "reservation_id": reservation_id,
        "normalized_output_root": normalized_output_root,
        "ownership_sidecar": explicit_output_sidecar_path(normalized_output_root),
        "repository_root_identity": str(repo_root.resolve()),
        "tracker_identity": tracker_identity,
        "issue_ledger_identity": deepcopy(dict(issue_ledger_identity)),
        "operation_nonce": operation_nonce,
    }


def explicit_output_marker_record(
    allocation: Mapping[str, Any],
    output_root: str | Path,
) -> dict[str, Any]:
    """Bind an allocation to the directory's stable physical identity."""

    identity = physical_identity(output_root, directory=True)
    return {
        "allocation": deepcopy(dict(allocation)),
        "physical_directory_identity": identity.to_dict(),
    }


def authenticate_explicit_output_marker(
    marker: Any,
    allocation: Mapping[str, Any],
    output_root: str | Path,
) -> dict[str, Any]:
    """Reopen a directory and reject same-path physical replacement."""

    if not isinstance(marker, Mapping) or set(marker) != {
        "allocation",
        "physical_directory_identity",
    }:
        raise ProgressError("explicit packet output-root marker has the wrong schema")
    if marker.get("allocation") != dict(allocation):
        raise ProgressError("explicit packet output-root marker allocation changed")
    expected = marker.get("physical_directory_identity")
    if not isinstance(expected, Mapping) or set(expected) != {
        "volume_identity",
        "file_id",
        "file_size",
        "is_directory",
        "canonical_path",
    }:
        raise ProgressError("explicit packet output-root physical identity is incomplete")
    observed = physical_identity(output_root, directory=True).to_dict()
    for field in ("volume_identity", "file_id", "is_directory", "canonical_path"):
        if observed[field] != expected[field]:
            raise ProgressError("explicit packet output-root physical directory was replaced")
    return deepcopy(dict(expected))


def authenticate_explicit_output_root(
    work: Mapping[str, Any],
    *,
    progress_path: str | Path,
    repo_root: Path = REPO_ROOT,
    require_active: bool = True,
) -> dict[str, Any]:
    """Authenticate an allocated explicit packet root and ownership marker."""

    if work.get("packet_type") != EXPLICIT_MAINTENANCE_PACKET_TYPE:
        raise ProgressError("output-root authentication requires an explicit packet")
    reservation = work.get("reservation")
    if require_active:
        if (
            not isinstance(reservation, Mapping)
            or not str(reservation.get("id", ""))
            or work.get("state") != "active"
            or reservation.get("state") != "active"
        ):
            raise ProgressError("explicit packet output root is not active")
    provenance = work.get("explicit_provenance")
    allocation = (
        provenance.get("output_allocation")
        if isinstance(provenance, Mapping)
        else None
    )
    required = {
        "schema",
        "packet_id",
        "reservation_id",
        "normalized_output_root",
        "ownership_sidecar",
        "repository_root_identity",
        "tracker_identity",
        "issue_ledger_identity",
        "operation_nonce",
    }
    if not isinstance(allocation, Mapping) or set(allocation) != required:
        raise ProgressError("explicit packet lacks a complete output allocation receipt")
    allocation_row = deepcopy(dict(allocation))
    if (
        allocation_row["schema"] != EXPLICIT_OUTPUT_MARKER_SCHEMA
        or allocation_row["packet_id"] != str(work.get("id", allocation_row["packet_id"]))
        or not str(allocation_row["reservation_id"])
        or not isinstance(allocation_row["tracker_identity"], Mapping)
        or not isinstance(allocation_row["issue_ledger_identity"], Mapping)
        or not allocation_row["issue_ledger_identity"]
        or not str(allocation_row["operation_nonce"])
    ):
        raise ProgressError("explicit packet output allocation identity is stale")
    if require_active and allocation_row["reservation_id"] != str(reservation.get("id")):
        raise ProgressError("explicit packet output allocation reservation changed")
    if not require_active:
        intent = work.get("allocation_intent")
        if (
            not isinstance(intent, Mapping)
            or intent.get("reservation_id") != allocation_row["reservation_id"]
            or intent.get("operation_nonce") != allocation_row["operation_nonce"]
        ):
            raise ProgressError("explicit packet allocation intent identity changed")
    progress = Path(progress_path).resolve()
    try:
        from _recoil.lib.progress_sqlite import read_progress_metadata

        metadata = read_progress_metadata(progress)
    except Exception as exc:
        raise ProgressError(
            f"explicit packet tracker identity is unavailable: {exc}"
        ) from exc
    observed_tracker_identity = {
        "storage_kind": "sqlite",
        "path": str(progress),
        "application_id": metadata.application_id,
        "user_version": metadata.user_version,
        "schema_version": metadata.schema_version,
        "cutover_pair_id": metadata.cutover_pair_id,
    }
    if dict(allocation_row["tracker_identity"]) != observed_tracker_identity:
        raise ProgressError("explicit packet tracker storage identity changed")
    claims, complete, _source = work_resource_claims(work)
    roots = [
        row["id"]
        for row in claims
        if row["kind"] == "output-root" and row["access"] == "write"
    ]
    if not complete or roots != [allocation_row["normalized_output_root"]]:
        raise ProgressError("explicit packet output-root claim changed")
    sealed_repo_root = Path(str(allocation_row["repository_root_identity"])).resolve()
    if repo_root != REPO_ROOT and repo_root.resolve() != sealed_repo_root:
        raise ProgressError("explicit packet repository root identity changed")
    build_root = (sealed_repo_root / "build").resolve()
    lexical = sealed_repo_root / str(allocation_row["normalized_output_root"])
    try:
        lexical.resolve(strict=False).relative_to(build_root)
    except (OSError, ValueError) as exc:
        raise ProgressError("explicit packet output root escapes build/") from exc
    if not lexical.exists() or not lexical.is_dir():
        raise ProgressError("explicit packet output root is absent or not a directory")
    if _path_has_reparse_component(lexical, stop=build_root):
        raise ProgressError("explicit packet output root traverses a reparse point")
    marker_path = lexical / EXPLICIT_OUTPUT_MARKER_NAME
    sidecar_path = sealed_repo_root / str(allocation_row["ownership_sidecar"])
    try:
        sidecar_path.resolve(strict=False).relative_to(build_root)
    except (OSError, ValueError) as exc:
        raise ProgressError("explicit packet ownership sidecar escapes build/") from exc
    if (
        not sidecar_path.is_file()
        or sidecar_path.is_symlink()
        or _path_has_reparse_component(sidecar_path, stop=build_root)
    ):
        raise ProgressError("explicit packet output-root ownership sidecar is unavailable")
    try:
        sidecar = json.loads(sidecar_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ProgressError("explicit packet output-root ownership sidecar is invalid") from exc
    if sidecar != allocation_row:
        raise ProgressError("explicit packet output-root ownership sidecar changed")
    if (
        not marker_path.is_file()
        or marker_path.is_symlink()
        or _path_has_reparse_component(marker_path, stop=build_root)
    ):
        raise ProgressError("explicit packet output-root ownership marker is unavailable")
    try:
        marker = json.loads(marker_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ProgressError("explicit packet output-root ownership marker is invalid") from exc
    directory_identity = authenticate_explicit_output_marker(
        marker, allocation_row, lexical
    )
    return {
        **allocation_row,
        "physical_directory_identity": directory_identity,
    }


def _explicit_validation_command_tokens(
    command: str,
    *,
    resource_claims: Sequence[Mapping[str, str]] = (),
) -> list[str]:
    """Parse one repository public nonmutating validation command."""

    if any(token in command for token in ("\r", "\n", "&", "|", ";", ">", "<", "`", "$(")):
        raise ProgressError(
            "explicit maintenance validation command forbids shell composition"
        )
    try:
        tokens = shlex.split(command, posix=True)
    except ValueError as exc:
        raise ProgressError(
            f"explicit maintenance validation command cannot be parsed: {exc}"
        ) from exc
    if not tokens or tokens[0].casefold() not in {"python", "python.exe"}:
        raise ProgressError(
            "explicit maintenance validation command must use repository python"
        )
    cursor = 1
    if cursor < len(tokens) and tokens[cursor] == "-B":
        cursor += 1
    try:
        command_entry = validate_repository_relative_path(
            tokens[cursor] if cursor < len(tokens) else "",
            context="explicit maintenance validation command entry point",
        )
    except RepositoryPathError as exc:
        raise _repository_path_error(exc) from exc
    if command_entry != "tools/recoil.py":
        raise ProgressError(
            "explicit maintenance validation command must invoke tools/recoil.py"
        )
    public = tokens[cursor + 1 :]
    if not public or public[0] not in {"verify", "audit", "guard", "doctor"}:
        raise ProgressError(
            "explicit maintenance validation command is not an allowed public nonmutating route"
        )
    if any(token == "--apply" for token in public):
        raise ProgressError(
            "explicit maintenance validation command cannot apply a mutation"
        )
    # Resolve against the actual public command table, not a textual prefix
    # allowlist.  Loading this registry is permitted only during explicit
    # packet construction; query and scheduler projections never call here.
    entry_path = REPO_ROOT / "tools" / "recoil.py"
    try:
        module_spec = importlib.util.spec_from_file_location(
            "_recoil_explicit_public_registry", entry_path
        )
        if module_spec is None or module_spec.loader is None:
            raise RuntimeError("public command registry cannot be loaded")
        registry_module = importlib.util.module_from_spec(module_spec)
        sys.modules[module_spec.name] = registry_module
        module_spec.loader.exec_module(registry_module)
        registry_module.validate_nonmutating_public_command(
            public,
            resource_claims=resource_claims,
        )
    except Exception as exc:
        raise ProgressError(
            f"explicit maintenance public command registry is unavailable: {exc}"
        ) from exc
    finally:
        sys.modules.pop("_recoil_explicit_public_registry", None)
    return tokens


def _explicit_selected_records(
    document: ProgressDocument,
    scope: Mapping[str, Any],
) -> tuple[dict[str, list[str]], dict[str, Mapping[str, Any]]]:
    expected = {
        "verification_target_ids": "verification_targets",
        "physical_block_ids": "physical_blocks",
        "source_owner_ids": "owners",
    }
    if set(scope) != set(expected):
        raise ProgressError(
            "explicit maintenance selected_scope must contain exactly "
            "verification_target_ids, physical_block_ids, and source_owner_ids"
        )
    selected: dict[str, list[str]] = {}
    records: dict[str, Mapping[str, Any]] = {}
    for field, collection in expected.items():
        ids = _explicit_string_list(scope.get(field), f"selected_scope.{field}")
        selected[field] = ids
        rows = document.collection(collection)
        for entity_id in ids:
            row = rows.get(entity_id)
            if not isinstance(row, Mapping):
                raise ProgressError(
                    f"explicit maintenance selected unknown {collection[:-1]} {entity_id!r}"
                )
            records[entity_id] = row
    if not any(selected.values()):
        raise ProgressError("explicit maintenance requires at least one exact selected identity")
    return selected, records


def _explicit_relationship_snapshot(
    document: ProgressDocument,
    selected: Mapping[str, list[str]],
    selected_records: Mapping[str, Mapping[str, Any]],
) -> dict[str, Any]:
    symbol_ids: set[str] = set()
    block_ids: set[str] = set(selected["physical_block_ids"])
    owner_ids: set[str] = set(selected["source_owner_ids"])
    for target_id in selected["verification_target_ids"]:
        target = selected_records[target_id]
        raw_symbols = target.get("symbol_ids", target.get("registered_symbol_ids", []))
        if isinstance(raw_symbols, list):
            symbol_ids.update(str(item) for item in raw_symbols if isinstance(item, str))
    symbols = document.collection("symbols")
    for symbol_id in tuple(symbol_ids):
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, Mapping):
            continue
        block_id = symbol.get("physical_block_id", symbol.get("block_id"))
        owner_id = symbol.get("owner_id")
        if isinstance(block_id, str) and block_id:
            block_ids.add(block_id)
        if isinstance(owner_id, str) and owner_id:
            owner_ids.add(owner_id)
    for block_id in tuple(block_ids):
        block = document.collection("physical_blocks").get(block_id)
        if not isinstance(block, Mapping):
            continue
        for field in ("symbol_ids", "physical_symbol_ids"):
            values = block.get(field)
            if isinstance(values, list):
                symbol_ids.update(str(item) for item in values if isinstance(item, str))
        owner_id = block.get("owner_id")
        if isinstance(owner_id, str) and owner_id:
            owner_ids.add(owner_id)
    for owner_id, owner in document.collection("owners").items():
        if not isinstance(owner, Mapping):
            continue
        owned = set()
        for field in ("primary_entry_ids", "symbol_ids", "function_ids"):
            values = owner.get(field)
            if isinstance(values, list):
                owned.update(str(item) for item in values if isinstance(item, str))
        if owned & symbol_ids:
            owner_ids.add(str(owner_id))
    projection = {
        "selected": deepcopy(dict(selected)),
        "related_symbol_ids": sorted(symbol_ids),
        "related_physical_block_ids": sorted(block_ids),
        "related_source_owner_ids": sorted(owner_ids),
    }
    return projection


def construct_explicit_maintenance_work_item(
    document: ProgressDocument,
    payload: Mapping[str, Any],
    *,
    repo_root: Path = REPO_ROOT,
) -> tuple[str, dict[str, Any]]:
    """Validate and render one explicit user-selected packet without side effects."""

    required = {
        "schema",
        "packet_id",
        "kind",
        "selected_scope",
        "writable_paths",
        "writable_overrides",
        "read_dependencies",
        "output_root",
        "resources",
        "objective",
        "stop_condition",
        "validation_command",
        "worker_role",
        "return_schema",
        "user_selected_rationale",
        "scheduler_inappropriate_reason",
    }
    optional = {"lease_expires_at"}
    unknown = set(payload) - required - optional
    missing = required - set(payload)
    if missing or unknown:
        raise ProgressError(
            "explicit maintenance payload keys differ from v1 contract; "
            f"missing={sorted(missing)} unknown={sorted(unknown)}"
        )
    if payload.get("schema") != EXPLICIT_MAINTENANCE_PACKET_SCHEMA:
        raise ProgressError(
            f"explicit maintenance schema must be {EXPLICIT_MAINTENANCE_PACKET_SCHEMA}"
        )
    try:
        tracked_inventory = load_git_tracked_path_inventory(repo_root)
    except RepositoryPathError as exc:
        raise _repository_path_error(exc) from exc
    work_id = _explicit_nonempty_text(payload.get("packet_id"), "packet_id")
    if not work_id.startswith("recoil:explicit-work:"):
        raise ProgressError("explicit maintenance packet_id must start recoil:explicit-work:")
    kind = str(payload.get("kind", ""))
    if kind not in EXPLICIT_MAINTENANCE_KINDS:
        raise ProgressError(f"unsupported explicit maintenance kind {kind!r}")
    scope = payload.get("selected_scope")
    if not isinstance(scope, Mapping):
        raise ProgressError("explicit maintenance selected_scope must be an object")
    selected, selected_records = _explicit_selected_records(document, scope)
    relationship = _explicit_relationship_snapshot(document, selected, selected_records)

    candidate_write_fields = {
        "source_from",
        "source_path",
        "agent_source_path",
        "original_source_path",
        "provisional_original_path",
        "translation_unit_source",
        "tu_source_from",
        "order_edit_paths",
        "source_paths",
        "header_paths",
        "declaration_paths",
        "definition_paths",
    }
    derived_write_paths: set[str] = set()
    derived_read_paths: set[str] = set()
    for entity_id in [
        *selected["verification_target_ids"],
        *selected["physical_block_ids"],
        *selected["source_owner_ids"],
    ]:
        row = selected_records[entity_id]
        for raw_path in _record_path_values(row, candidate_write_fields):
            try:
                derived_write_paths.add(
                    _explicit_exact_file(
                        raw_path,
                        field="derived writable closure",
                        repo_root=repo_root,
                        inventory=tracked_inventory,
                        source_only=True,
                    )
                )
            except ProgressError:
                # Registration records can contain historical/original path
                # labels.  Only current existing repo-local source candidates
                # enter the writable closure.
                continue
        for raw_path in _record_path_values(row, {"manifest_path"}):
            derived_read_paths.add(
                _explicit_exact_file(
                    raw_path,
                    field="registered manifest dependency",
                    repo_root=repo_root,
                    inventory=tracked_inventory,
                )
            )

    # Target registration can identify related blocks and owners without the
    # operator widening the selected write scope to those entities.  Preserve
    # those relationships as read dependencies and concurrency claims.  They
    # deliberately do not expand the writable closure.
    related_rows: list[Mapping[str, Any]] = []
    blocks = document.collection("physical_blocks")
    owners = document.collection("owners")
    for block_id in relationship["related_physical_block_ids"]:
        if block_id not in selected["physical_block_ids"]:
            row = blocks.get(block_id)
            if isinstance(row, Mapping):
                related_rows.append(row)
    for owner_id in relationship["related_source_owner_ids"]:
        if owner_id not in selected["source_owner_ids"]:
            row = owners.get(owner_id)
            if isinstance(row, Mapping):
                related_rows.append(row)
    for row in related_rows:
        for raw_path in _record_path_values(row, candidate_write_fields):
            try:
                derived_read_paths.add(
                    _explicit_exact_file(
                        raw_path,
                        field="related registered read dependency",
                        repo_root=repo_root,
                        inventory=tracked_inventory,
                        source_only=True,
                    )
                )
            except ProgressError:
                continue

    writable = _explicit_string_list(payload.get("writable_paths"), "writable_paths")
    writable_paths = [
        _explicit_exact_file(
            value,
            field="writable_paths",
            repo_root=repo_root,
            inventory=tracked_inventory,
            source_only=True,
            allow_proposed_destination=True,
        )
        for value in writable
    ]
    if kind == "source-maintenance" and not writable_paths:
        raise ProgressError("source-maintenance requires a non-empty writable closure")
    if kind == "read-only-diagnostic" and writable_paths:
        raise ProgressError("read-only-diagnostic cannot acquire source write access")

    overrides = payload.get("writable_overrides")
    if not isinstance(overrides, list):
        raise ProgressError("explicit maintenance writable_overrides must be a list")
    override_paths: set[str] = set()
    cross_owner_ids: set[str] = set()
    selected_ids = {item for values in selected.values() for item in values}
    for index, override in enumerate(overrides):
        if not isinstance(override, Mapping) or set(override) != {
            "path",
            "relation",
            "selected_scope_id",
            "related_owner_id",
            "evidence",
            "rationale",
        }:
            raise ProgressError(
                f"explicit maintenance writable_overrides[{index}] has the wrong shape"
            )
        path = _explicit_exact_file(
            str(override.get("path", "")),
            field=f"writable_overrides[{index}].path",
            repo_root=repo_root,
            inventory=tracked_inventory,
            source_only=True,
            allow_proposed_destination=True,
        )
        if override.get("relation") not in EXPLICIT_OVERRIDE_RELATIONS:
            raise ProgressError(f"explicit maintenance writable_overrides[{index}] relation is invalid")
        if override.get("selected_scope_id") not in selected_ids:
            raise ProgressError(
                f"explicit maintenance writable_overrides[{index}] is not bound to selected scope"
            )
        relation = str(override.get("relation"))
        related_owner_value = override.get("related_owner_id")
        if relation == "reviewed-unregistered-declaration-debt":
            if related_owner_value != "":
                raise ProgressError(
                    f"explicit maintenance writable_overrides[{index}] unregistered declaration debt requires an empty related_owner_id"
                )
        else:
            related_owner_id = _explicit_nonempty_text(
                related_owner_value,
                f"writable_overrides[{index}].related_owner_id",
            )
            if related_owner_id not in document.collection("owners"):
                raise ProgressError(
                    f"explicit maintenance writable_overrides[{index}] names unknown related owner"
                )
            related_owner = document.collection("owners")[related_owner_id]
            related_paths = {
                normalized
                for raw in _record_path_values(related_owner, candidate_write_fields)
                for normalized in [
                    _explicit_exact_file(
                        raw,
                        field=f"writable_overrides[{index}].related_owner_path",
                        repo_root=repo_root,
                        inventory=tracked_inventory,
                        source_only=True,
                    )
                ]
            }
            if path not in related_paths:
                raise ProgressError(
                    f"explicit maintenance writable_overrides[{index}] path is not in the exact related-owner closure"
                )
            cross_owner_ids.add(related_owner_id)
        _explicit_nonempty_text(override.get("evidence"), f"writable_overrides[{index}].evidence")
        _explicit_nonempty_text(override.get("rationale"), f"writable_overrides[{index}].rationale")
        override_paths.add(path)
    if cross_owner_ids:
        relationship["related_source_owner_ids"] = sorted(
            set(relationship["related_source_owner_ids"]) | cross_owner_ids
        )
    relationship["related_source_owner_snapshot"] = [
        {
            "owner_id": owner_id,
            "record": deepcopy(document.collection("owners")[owner_id]),
            "reviewed_cross_owner_override": owner_id in cross_owner_ids,
        }
        for owner_id in relationship["related_source_owner_ids"]
        if owner_id in document.collection("owners")
    ]
    unauthorized = sorted(set(writable_paths) - derived_write_paths - override_paths)
    if unauthorized:
        raise ProgressError(
            "explicit maintenance writable paths exceed the derived/reviewed closure: "
            + ", ".join(unauthorized)
        )
    unused_overrides = sorted(override_paths - set(writable_paths))
    if unused_overrides:
        raise ProgressError(
            "explicit maintenance override does not authorize a requested writable path: "
            + ", ".join(unused_overrides)
        )

    read_dependencies = [
        _explicit_exact_file(
            value,
            field="read_dependencies",
            repo_root=repo_root,
            inventory=tracked_inventory,
        )
        for value in _explicit_string_list(payload.get("read_dependencies"), "read_dependencies")
    ]
    derived_read_paths.update(read_dependencies)
    derived_read_paths.update(derived_write_paths)
    derived_read_paths.update(writable_paths)
    output_root = _generated_repository_path(
        _explicit_nonempty_text(payload.get("output_root"), "output_root"),
        context="explicit maintenance output_root",
    )
    if not output_root.startswith("build/") or output_root == "build":
        raise ProgressError("explicit maintenance output_root must be an exact path under build/")
    if (repo_root / output_root).exists() or (
        repo_root / explicit_output_sidecar_path(output_root)
    ).exists():
        raise ProgressError("explicit maintenance output_root must be fresh and absent")

    resources = payload.get("resources")
    expected_resources = {
        "binary_ninja_saved_view_read",
        "whole_link_window",
        "tracker_read",
        "manifest_read",
        "support_read",
    }
    if not isinstance(resources, Mapping) or set(resources) != expected_resources:
        raise ProgressError(
            "explicit maintenance resources must contain exactly binary_ninja_saved_view_read, "
            "whole_link_window, tracker_read, manifest_read, and support_read"
        )
    if any(not isinstance(resources[key], bool) for key in expected_resources):
        raise ProgressError("explicit maintenance resource selectors must be Boolean")
    if not resources["tracker_read"]:
        raise ProgressError("explicit maintenance packets require tracker_read")
    if resources["manifest_read"] and not any(
        path.startswith("tools/vc5_verify_targets/") for path in derived_read_paths
    ):
        raise ProgressError("manifest_read requested without an exact registered manifest")
    if resources["support_read"]:
        support_path = _explicit_machine_local_file(
            "support/Recoil.exe",
            field="retail reference dependency",
            repo_root=repo_root,
        )
        derived_read_paths.add(support_path)

    objective = _explicit_nonempty_text(payload.get("objective"), "objective")
    stop_condition = _explicit_nonempty_text(payload.get("stop_condition"), "stop_condition")
    validation_command = _explicit_nonempty_text(
        payload.get("validation_command"), "validation_command"
    )
    validation_resource_claims: list[dict[str, str]] = []
    if resources["binary_ninja_saved_view_read"]:
        validation_resource_claims.append(
            {"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"}
        )
    validation_resource_claims.append(
        {
            "kind": "whole-project-build",
            "id": "recoil",
            "access": "write" if resources["whole_link_window"] else "read",
        }
    )
    validation_tokens = _explicit_validation_command_tokens(
        validation_command,
        resource_claims=validation_resource_claims,
    )
    recoil_index = validation_tokens.index("tools/recoil.py")
    public_validation = validation_tokens[recoil_index + 1 :]
    if (
        public_validation[:2] == ["verify", "final-build"]
        and not resources["whole_link_window"]
    ):
        raise ProgressError(
            "verify final-build requires the explicit whole-link diagnostic window"
        )
    worker_role = str(payload.get("worker_role", ""))
    if worker_role not in EXPLICIT_MAINTENANCE_ROLES:
        raise ProgressError(f"unsupported explicit maintenance worker_role {worker_role!r}")
    if kind == "source-maintenance" and worker_role != "recoil_source_worker":
        raise ProgressError("source-maintenance requires recoil_source_worker")
    if kind == "read-only-diagnostic" and worker_role == "recoil_source_worker":
        raise ProgressError("read-only-diagnostic requires a read-only worker role")
    return_schema = _explicit_string_list(payload.get("return_schema"), "return_schema")
    if not return_schema:
        raise ProgressError("explicit maintenance return_schema must not be empty")
    rationale = _explicit_nonempty_text(
        payload.get("user_selected_rationale"), "user_selected_rationale"
    )
    scheduler_reason = _explicit_nonempty_text(
        payload.get("scheduler_inappropriate_reason"),
        "scheduler_inappropriate_reason",
    )
    expires = payload.get("lease_expires_at")
    if expires is not None:
        expires = _explicit_nonempty_text(expires, "lease_expires_at")
        try:
            parsed = datetime.fromisoformat(expires.replace("Z", "+00:00"))
        except ValueError as exc:
            raise ProgressError("explicit maintenance lease_expires_at is not ISO-8601") from exc
        if parsed.tzinfo is None:
            raise ProgressError("explicit maintenance lease_expires_at requires a timezone")
        expires = parsed.astimezone(timezone.utc).isoformat().replace("+00:00", "Z")

    claims: list[dict[str, str]] = [
        {"kind": "lane", "id": f"explicit-maintenance/{work_id}", "access": "write"},
        {"kind": "output-root", "id": output_root, "access": "write"},
        {"kind": "tracker", "id": "recoil", "access": "read"},
        {
            "kind": "whole-project-build",
            "id": "recoil",
            "access": "write" if resources["whole_link_window"] else "read",
        },
    ]
    claims.extend(
        {"kind": "verification-target", "id": item, "access": "read"}
        for item in selected["verification_target_ids"]
    )
    claims.extend(
        {"kind": "block", "id": item, "access": "read"}
        for item in relationship["related_physical_block_ids"]
    )
    owner_write_ids = set(selected["source_owner_ids"]) | cross_owner_ids
    claims.extend(
        {
            "kind": "owner",
            "id": item,
            "access": "write" if item in owner_write_ids else "read",
        }
        for item in relationship["related_source_owner_ids"]
    )
    claims.extend(
        {"kind": "path", "id": item, "access": "write"}
        for item in writable_paths
    )
    claims.extend(
        {"kind": "path", "id": item, "access": "read"}
        for item in sorted(derived_read_paths - set(writable_paths))
    )
    if resources["binary_ninja_saved_view_read"]:
        claims.append({"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"})
    if resources["whole_link_window"]:
        claims.append({"kind": "whole-link-window", "id": "recoil", "access": "write"})
    claims = normalize_resource_claims(claims)
    closure = {
        "derived_writable_paths": sorted(derived_write_paths),
        "reviewed_override_paths": sorted(override_paths),
        "write_paths": sorted(writable_paths),
        "read_only_paths": sorted(derived_read_paths - set(writable_paths)),
        "output_root": output_root,
        "relationship_snapshot": relationship,
        "reviewed_cross_owner_overrides": [
            deepcopy(dict(row))
            for row in overrides
            if isinstance(row, Mapping)
            and row.get("related_owner_id") in cross_owner_ids
        ],
    }
    packet = {
        "id": work_id,
        "packet_type": EXPLICIT_MAINTENANCE_PACKET_TYPE,
        "explicit_kind": kind,
        "state": "ready",
        "binary": "recoil",
        "phase": "explicit-user-selected-maintenance",
        "lane": "explicit",
        "handoff_role": worker_role,
        "target_id": selected["verification_target_ids"][0]
        if selected["verification_target_ids"]
        else "",
        "target_ids": selected["verification_target_ids"],
        "covered_block_ids": selected["physical_block_ids"],
        "source_owner_ids": selected["source_owner_ids"],
        "related_block_ids": relationship["related_physical_block_ids"],
        "related_source_owner_ids": relationship["related_source_owner_ids"],
        "scope_ids": sorted({item for values in selected.values() for item in values}),
        "allowed_paths": sorted(writable_paths),
        "read_only_paths": closure["read_only_paths"],
        "validation_commands": [validation_command],
        "objective": objective,
        "stop_condition": stop_condition,
        "required_return_fields": return_schema,
        "resource_claims": claims,
        "explicit_provenance": {
            "schema": EXPLICIT_MAINTENANCE_PACKET_SCHEMA,
            "command": EXPLICIT_MAINTENANCE_COMMAND,
            "user_selected_rationale": rationale,
            "scheduler_inappropriate_reason": scheduler_reason,
            "selected_scope": deepcopy(selected),
            "writable_overrides": deepcopy(overrides),
            "closure": closure,
        },
        "lease_expires_at": expires,
        "read_only": kind == "read-only-diagnostic",
        "nonaccepting": True,
        "acceptance_eligible": False,
        "candidate_expected_truth": False,
        "worker_acceptance_allowed": False,
        "profile_selection_allowed": False,
        "phase_transition_allowed": False,
    }
    return work_id, bind_work_packet_contract(document, packet)


def create_and_reserve_explicit_maintenance_work_item(
    data: dict[str, Any],
    payload: Mapping[str, Any],
    *,
    repo_root: Path = REPO_ROOT,
    progress_path: str | Path,
    operation_nonce: str,
    issue_ledger_identity: Mapping[str, Any],
) -> dict[str, Any]:
    """Persist a non-work allocation journal without reserving resources."""

    document = ProgressDocument(data)
    work_id, packet = construct_explicit_maintenance_work_item(
        document, payload, repo_root=repo_root
    )
    work_items = data.get("work_items")
    if not isinstance(work_items, dict):
        raise ProgressError("progress work_items collection must be an object")
    if work_id in work_items:
        raise ProgressError(f"explicit maintenance work item already exists: {work_id}")
    migration = data.setdefault("migration", {})
    if not isinstance(migration, dict):
        raise ProgressError("progress migration collection must be an object")
    journal_registry = migration.setdefault(
        EXPLICIT_ALLOCATION_JOURNAL_MIGRATION_KEY,
        {"schema": EXPLICIT_ALLOCATION_JOURNAL_SCHEMA, "rows": {}},
    )
    if (
        not isinstance(journal_registry, dict)
        or journal_registry.get("schema") != EXPLICIT_ALLOCATION_JOURNAL_SCHEMA
        or not isinstance(journal_registry.get("rows"), dict)
    ):
        raise ProgressError("explicit output allocation journal registry is malformed")
    journals = journal_registry["rows"]
    if work_id in journals:
        raise ProgressError(f"explicit output allocation journal already exists: {work_id}")
    proposed_reservation_id = f"{work_id}:attempt:1"
    output_roots = [
        row["id"]
        for row in packet.get("resource_claims", [])
        if isinstance(row, Mapping)
        and row.get("kind") == "output-root"
        and row.get("access") == "write"
    ]
    if len(output_roots) != 1:
        raise ProgressError(
            "explicit maintenance allocation requires exactly one output root"
        )
    allocation = explicit_output_allocation_record(
        packet_id=work_id,
        reservation_id=proposed_reservation_id,
        output_root=output_roots[0],
        progress_path=progress_path,
        operation_nonce=operation_nonce,
        issue_ledger_identity=issue_ledger_identity,
        repo_root=repo_root,
    )
    packet["state"] = "allocation-candidate"
    packet["allocation_state"] = "journaled"
    packet["allocation_intent"] = {
        "schema": "recoil-explicit-output-allocation-intent-v1",
        "reservation_id": proposed_reservation_id,
        "operation_nonce": operation_nonce,
        "resource_claim_state": "inactive-until-activation",
    }
    packet["explicit_provenance"]["output_allocation"] = allocation
    packet["explicit_provenance"]["issue_ledger_identity"] = deepcopy(
        dict(issue_ledger_identity)
    )
    journal = {
        "schema": EXPLICIT_ALLOCATION_JOURNAL_SCHEMA,
        "state": "allocating",
        "recovery_state": "pending",
        "packet_id": work_id,
        "proposed_reservation_id": proposed_reservation_id,
        "operation_nonce": operation_nonce,
        "normalized_output_root": allocation["normalized_output_root"],
        "ownership_sidecar": allocation["ownership_sidecar"],
        "tracker_identity": deepcopy(allocation["tracker_identity"]),
        "issue_ledger_identity": deepcopy(dict(issue_ledger_identity)),
        "expected_ownership_marker": deepcopy(allocation),
        "candidate_packet": deepcopy(packet),
        "active_work_created": False,
        "active_reservation_created": False,
        "normal_claims_installed": False,
    }
    journals[work_id] = journal
    return {
        "work_item_id": work_id,
        "reservation_id": proposed_reservation_id,
        "reservation": None,
        "allocation_intent": deepcopy(packet["allocation_intent"]),
        "allocation_journal": deepcopy(journal),
        "work_item": deepcopy(packet),
    }


def activate_explicit_maintenance_work_item(
    data: dict[str, Any],
    work_id: str,
    *,
    progress_path: str | Path,
    repo_root: Path = REPO_ROOT,
) -> dict[str, Any]:
    """Atomically materialize one authenticated journal as active work."""

    work_items = data.get("work_items")
    if not isinstance(work_items, dict):
        raise ProgressError("progress work_items collection must be an object")
    if work_id in work_items:
        raise ProgressError("explicit allocation journal already has a work packet")
    migration = data.get("migration")
    registry = (
        migration.get(EXPLICIT_ALLOCATION_JOURNAL_MIGRATION_KEY)
        if isinstance(migration, Mapping)
        else None
    )
    journals = registry.get("rows") if isinstance(registry, Mapping) else None
    journal = journals.get(work_id) if isinstance(journals, Mapping) else None
    if (
        not isinstance(journal, dict)
        or journal.get("schema") != EXPLICIT_ALLOCATION_JOURNAL_SCHEMA
        or journal.get("state") != "allocating"
        or journal.get("recovery_state") != "pending"
    ):
        raise ProgressError(f"unknown or non-pending explicit allocation journal {work_id}")
    candidate = journal.get("candidate_packet")
    if (
        not isinstance(candidate, Mapping)
        or candidate.get("packet_type") != EXPLICIT_MAINTENANCE_PACKET_TYPE
        or candidate.get("reservation") is not None
        or candidate.get("execution_attempts") not in (None, [])
    ):
        raise ProgressError("explicit allocation journal candidate is malformed")
    work = deepcopy(dict(candidate))
    allocation = authenticate_explicit_output_root(
        work,
        progress_path=progress_path,
        repo_root=repo_root,
        require_active=False,
    )
    intent = work.get("allocation_intent")
    intended_reservation_id = (
        str(intent.get("reservation_id")) if isinstance(intent, Mapping) else ""
    )
    work["state"] = "ready"
    work["allocation_state"] = "activating"
    work_items[work_id] = work
    reserved = reserve_work_item(data, work_id)
    reservation = work.get("reservation")
    if (
        not isinstance(reservation, dict)
        or reservation.get("id") != intended_reservation_id
    ):
        raise ProgressError("explicit maintenance activation reservation identity changed")
    reservation["expires"] = work.get("lease_expires_at")
    work["allocation_state"] = "active"
    reservation["allocation_state"] = "active"
    attempts = work.get("execution_attempts")
    if isinstance(attempts, list) and attempts:
        attempts[-1]["allocation_state"] = "active"
    journal["state"] = "activated"
    journal["recovery_state"] = "active-work"
    journal["active_work_created"] = True
    journal["active_reservation_created"] = True
    journal["normal_claims_installed"] = True
    journal["activated_reservation_id"] = str(reservation["id"])
    journal.pop("candidate_packet", None)
    return {
        "work_item_id": work_id,
        "reservation_id": str(reserved["reservation"]["id"]),
        "allocation_state": "active",
        "output_allocation": deepcopy(allocation),
    }


def fail_explicit_maintenance_allocation(
    data: dict[str, Any],
    work_id: str,
    *,
    reason: str,
    cleanup_state: str = "absent",
) -> dict[str, Any]:
    """Terminalize a non-work journal while retaining an auditable receipt."""

    work_items = data.get("work_items")
    if isinstance(work_items, Mapping) and work_id in work_items:
        raise ProgressError("active or materialized work cannot fail as allocation debt")
    migration = data.get("migration")
    registry = (
        migration.get(EXPLICIT_ALLOCATION_JOURNAL_MIGRATION_KEY)
        if isinstance(migration, Mapping)
        else None
    )
    journals = registry.get("rows") if isinstance(registry, Mapping) else None
    journal = journals.get(work_id) if isinstance(journals, Mapping) else None
    if not isinstance(journal, dict) or journal.get("state") != "allocating":
        raise ProgressError(f"unknown allocating journal {work_id}")
    if cleanup_state not in {"absent", "quarantined"}:
        raise ProgressError("allocation cleanup state must be absent or quarantined")
    failure_reason = _explicit_nonempty_text(reason, "allocation failure reason")
    allocation = deepcopy(journal.get("expected_ownership_marker", {}))
    journal["state"] = "failed-allocation"
    journal["recovery_state"] = (
        "cleanup-debt" if cleanup_state == "quarantined" else "clean"
    )
    journal["allocation_failure_receipt"] = {
        "schema": EXPLICIT_ALLOCATION_FAILURE_SCHEMA,
        "reason": failure_reason,
        "reservation_created": False,
        "reservation_released": False,
        "resources_released": True,
        "cleanup_state": cleanup_state,
        "output_allocation": allocation,
    }
    if cleanup_state == "quarantined":
        journal["cleanup_debt"] = {
            "schema": EXPLICIT_CLEANUP_DEBT_SCHEMA,
            "state": "quarantined",
            "packet_id": work_id,
            "reservation_id": allocation.get("reservation_id"),
            "operation_nonce": allocation.get("operation_nonce"),
            "normalized_output_root": allocation.get("normalized_output_root"),
            "output_allocation": deepcopy(allocation),
        }
    else:
        journal.pop("cleanup_debt", None)
    return {
        "work_item_id": work_id,
        "reservation_id": str(allocation.get("reservation_id", "")),
        "allocation_state": "failed-allocation",
        "reservation_created": False,
        "cleanup_state": cleanup_state,
        "reason": failure_reason,
    }


_EXPLICIT_CLEANUP_RECOVERY_AUTHORITY = object()


class ExplicitCleanupRecoveryReceipt:
    """Opaque proof that the governed command observed and removed one root."""

    __slots__ = ("packet_id", "operation_nonce", "allocation", "outcome")

    def __init__(
        self,
        packet_id: str,
        operation_nonce: str,
        allocation: Mapping[str, Any],
        outcome: str,
        *,
        _authority: object,
    ) -> None:
        if _authority is not _EXPLICIT_CLEANUP_RECOVERY_AUTHORITY:
            raise ProgressError(
                "cleanup recovery receipts may be issued only by the governed filesystem verifier"
            )
        self.packet_id = packet_id
        self.operation_nonce = operation_nonce
        self.allocation = deepcopy(dict(allocation))
        self.outcome = outcome


def _issue_explicit_cleanup_recovery_receipt(
    *, packet_id: str, allocation: Mapping[str, Any], outcome: str
) -> ExplicitCleanupRecoveryReceipt:
    return ExplicitCleanupRecoveryReceipt(
        packet_id,
        str(allocation.get("operation_nonce", "")),
        allocation,
        outcome,
        _authority=_EXPLICIT_CLEANUP_RECOVERY_AUTHORITY,
    )


def recover_explicit_maintenance_cleanup_debt(
    data: dict[str, Any],
    work_id: str,
    *,
    recovery_receipt: ExplicitCleanupRecoveryReceipt,
) -> dict[str, Any]:
    """Finalize independently verified cleanup; caller filesystem Booleans are invalid."""

    if not isinstance(recovery_receipt, ExplicitCleanupRecoveryReceipt):
        raise ProgressError("cleanup-debt recovery requires an opaque governed receipt")
    migration = data.get("migration")
    registry = (
        migration.get(EXPLICIT_ALLOCATION_JOURNAL_MIGRATION_KEY)
        if isinstance(migration, Mapping)
        else None
    )
    journals = registry.get("rows") if isinstance(registry, Mapping) else None
    journal = journals.get(work_id) if isinstance(journals, Mapping) else None
    if not isinstance(journal, dict) or journal.get("schema") != EXPLICIT_ALLOCATION_JOURNAL_SCHEMA:
        raise ProgressError(f"unknown explicit allocation journal {work_id}")
    if journal.get("state") == "activated":
        raise ProgressError("active work roots cannot be cleared as allocation debt")
    allocation = journal.get("expected_ownership_marker")
    if not isinstance(allocation, Mapping):
        raise ProgressError("explicit allocation journal lacks ownership evidence")
    if (
        recovery_receipt.packet_id != work_id
        or recovery_receipt.operation_nonce != str(allocation.get("operation_nonce", ""))
        or recovery_receipt.allocation != dict(allocation)
        or recovery_receipt.outcome not in {"already-absent", "owned-root-removed"}
    ):
        raise ProgressError("cleanup recovery receipt does not bind this allocation journal")
    prior_debt = journal.get("cleanup_debt")
    journal["state"] = "recovered"
    journal["recovery_state"] = "clean"
    journal["cleanup_recovery_receipt"] = {
        "schema": EXPLICIT_CLEANUP_RECOVERY_RECEIPT_SCHEMA,
        "packet_id": work_id,
        "operation_nonce": recovery_receipt.operation_nonce,
        "allocation": deepcopy(recovery_receipt.allocation),
        "outcome": recovery_receipt.outcome,
        "nonaccepting": True,
    }
    if isinstance(prior_debt, Mapping):
        journal["cleanup_debt_history"] = deepcopy(dict(prior_debt))
    journal.pop("cleanup_debt", None)
    failure = journal.get("allocation_failure_receipt")
    if isinstance(failure, dict):
        failure["cleanup_state"] = "absent"
    return {
        "work_item_id": work_id,
        "outcome": "cleanup-recovered",
        "recovery_receipt": deepcopy(journal["cleanup_recovery_receipt"]),
        "acceptance_changed": False,
    }


def return_explicit_maintenance_work_item(
    data: dict[str, Any],
    work_id: str,
    result: Mapping[str, Any],
    *,
    binja_receipt: object | None = None,
    progress_path: str | Path,
) -> dict[str, Any]:
    work_items = data.get("work_items")
    work = work_items.get(work_id) if isinstance(work_items, dict) else None
    if not isinstance(work, dict) or work.get("packet_type") != EXPLICIT_MAINTENANCE_PACKET_TYPE:
        raise ProgressError(f"unknown explicit maintenance work item {work_id}")
    reservation = work.get("reservation")
    if work.get("state") != "active" or not isinstance(reservation, dict) or reservation.get("state") != "active":
        raise ProgressError("only an active explicit maintenance packet may return")
    authenticate_explicit_output_root(work, progress_path=progress_path)
    required = work.get("required_return_fields")
    if not isinstance(required, list) or set(result) != set(required):
        raise ProgressError("explicit maintenance result fields differ from the packet return schema")
    encoded = json.dumps(result, sort_keys=True, separators=(",", ":")).encode("utf-8")
    if len(encoded) > EXPLICIT_RESULT_MAX_BYTES:
        raise ProgressError("explicit maintenance result exceeds the bounded result limit")
    claims, complete, _source = work_resource_claims(work)
    if not complete:
        raise ProgressError("explicit maintenance packet claims are incomplete at return")
    requires_binja = {
        "kind": "binary-ninja-db",
        "id": "Recoil.bndb",
        "access": "read",
    } in claims
    if requires_binja:
        from _recoil.lib.binja import GovernedBinaryNinjaReadReceipt

        if not isinstance(binja_receipt, GovernedBinaryNinjaReadReceipt):
            raise ProgressError(
                "Binary Ninja explicit packet return requires an opaque governed read receipt"
            )
        receipt = binja_receipt.as_dict()
        if (
            receipt.get("packet_id") != work_id
            or receipt.get("reservation_id") != reservation.get("id")
            or receipt.get("snapshot_equal") is not True
            or receipt.get("resource_claims") != claims
            or receipt.get("nonaccepting") is not True
            or receipt.get("candidate_expected_truth") is not False
        ):
            raise ProgressError("governed Binary Ninja read receipt does not bind this reservation")
        work["binary_ninja_read_receipt"] = receipt
    elif binja_receipt is not None:
        raise ProgressError("non-Binary-Ninja packet cannot attach a BN read receipt")
    reservation["state"] = "released"
    reservation["outcome"] = "returned"
    attempts = work.get("execution_attempts")
    if isinstance(attempts, list):
        for attempt in attempts:
            if isinstance(attempt, dict) and attempt.get("id") == reservation.get("id"):
                attempt["state"] = "released"
                attempt["outcome"] = "returned"
    work["state"] = "returned"
    work["returned_result"] = deepcopy(dict(result))
    closure = work.get("explicit_provenance", {}).get("closure", {})
    work["return_receipt"] = {
        "schema": "recoil-explicit-maintenance-return-v1",
        "packet_id": work_id,
        "reservation_id": str(reservation.get("id", "")),
        "returned_result": deepcopy(dict(result)),
        "relationship_snapshot": deepcopy(
            closure.get("relationship_snapshot", {})
            if isinstance(closure, Mapping)
            else {}
        ),
        "reviewed_cross_owner_overrides": deepcopy(
            closure.get("reviewed_cross_owner_overrides", [])
            if isinstance(closure, Mapping)
            else []
        ),
        "nonaccepting": True,
    }
    work["nonaccepting"] = True
    work["acceptance_eligible"] = False
    return {
        "work_item_id": work_id,
        "outcome": "returned",
        "returned_result": deepcopy(dict(result)),
        "acceptance_changed": False,
        "return_receipt": deepcopy(work["return_receipt"]),
    }


def recover_expired_explicit_maintenance_work_item(
    data: dict[str, Any], work_id: str, *, now: datetime | None = None
) -> dict[str, Any]:
    work_items = data.get("work_items")
    work = work_items.get(work_id) if isinstance(work_items, dict) else None
    if not isinstance(work, dict) or work.get("packet_type") != EXPLICIT_MAINTENANCE_PACKET_TYPE:
        raise ProgressError(f"unknown explicit maintenance work item {work_id}")
    reservation = work.get("reservation")
    if work.get("state") != "active" or not isinstance(reservation, dict) or reservation.get("state") != "active":
        raise ProgressError("only an active explicit maintenance packet may be recovered")
    expires = reservation.get("expires")
    if not isinstance(expires, str) or not expires:
        raise ProgressError("explicit maintenance reservation has no expiry")
    try:
        expiry = datetime.fromisoformat(expires.replace("Z", "+00:00"))
    except ValueError as exc:
        raise ProgressError("explicit maintenance reservation expiry is malformed") from exc
    current = now or datetime.now(timezone.utc)
    if current.astimezone(timezone.utc) < expiry.astimezone(timezone.utc):
        raise ProgressError("explicit maintenance reservation has not expired")
    reservation["state"] = "released"
    reservation["outcome"] = "expired"
    attempts = work.get("execution_attempts")
    if isinstance(attempts, list):
        for attempt in attempts:
            if isinstance(attempt, dict) and attempt.get("id") == reservation.get("id"):
                attempt["state"] = "released"
                attempt["outcome"] = "expired"
    work["reservation"] = None
    work["state"] = "ready"
    # A recovered packet must not immediately recreate the same already-expired
    # lease when it is reserved again.  Recovery clears only the scheduling
    # deadline; it does not alter scope, evidence, or acceptance state.
    work["lease_expires_at"] = None
    return {"work_item_id": work_id, "outcome": "expired-recovered"}


def bind_work_packet_contract(document: ProgressDocument, packet: dict[str, Any]) -> dict[str, Any]:
    del document
    packet["packet_contract_version"] = WORK_PACKET_CONTRACT_VERSION
    claims, complete, _source = work_resource_claims(packet)
    if not complete:
        raise ProgressError("new packet resource claims are incomplete")
    retail_fact_problem = retail_fact_packet_contract_problem(packet, claims)
    if retail_fact_problem:
        raise ProgressError(retail_fact_problem)
    packet["resource_claims"] = claims
    packet["semantic_contract_version"] = 1
    packet.setdefault("execution_attempts", [])
    packet.setdefault("reservation", None)
    return packet


def reserve_work_item(data: dict[str, Any], work_id: str) -> dict[str, Any]:
    document = ProgressDocument(data)
    work_items = data.get("work_items")
    if not isinstance(work_items, dict):
        raise ProgressError("progress work_items collection must be an object")
    work = work_items.get(work_id)
    if not isinstance(work, dict):
        raise ProgressError(f"unknown work item {work_id}")
    if work.get("state") not in WORK_LAUNCHABLE_STATES:
        raise ProgressError("only ready/current work can be reserved")
    claims, complete, _source = work_resource_claims(work)
    if not complete:
        raise ProgressError("work item resource claims are incomplete")
    for other_id, other in document.collection("work_items").items():
        if other_id == work_id or not isinstance(other, Mapping):
            continue
        reservation = other.get("reservation")
        if not isinstance(reservation, Mapping) or reservation.get("state") != "active":
            continue
        other_claims, other_complete, _ = work_resource_claims(other)
        if not other_complete:
            raise ProgressError(
                f"cannot prove non-overlap with incomplete active reservation {other_id}"
            )
        if resource_claim_conflicts(claims, str(other_id), other_claims):
            raise ProgressError(f"work item conflicts with active reservation {other_id}")
    attempts = work.setdefault("execution_attempts", [])
    if not isinstance(attempts, list):
        raise ProgressError("work execution_attempts must be a list")
    attempt_id = f"{work_id}:attempt:{len(attempts) + 1}"
    attempt = {"id": attempt_id, "state": "active", "resource_claims": claims}
    if work.get("packet_type") == EXPLICIT_MAINTENANCE_PACKET_TYPE:
        attempt["expires"] = work.get("lease_expires_at")
    attempts.append(attempt)
    work["reservation"] = deepcopy(attempt)
    work["state"] = "active"
    return {"work_item_id": work_id, "attempt": deepcopy(attempt), "reservation": deepcopy(attempt)}


def claim_current_provenance(
    *,
    command: str,
    requested_lane: str,
    selected_lane: str,
    max_packets: int,
) -> dict[str, Any]:
    """Return the complete deterministic provenance for one claim-current selection."""
    if command != CLAIM_CURRENT_COMMAND:
        raise ProgressError(
            f"live work-item creation is authorized only by {CLAIM_CURRENT_COMMAND!r}"
        )
    if requested_lane not in CLAIM_REQUESTED_LANES:
        raise ProgressError(f"invalid claim-current requested lane {requested_lane!r}")
    if selected_lane not in CLAIM_SELECTED_LANES:
        raise ProgressError(f"invalid claim-current selected lane {selected_lane!r}")
    if requested_lane != "all" and selected_lane != requested_lane:
        raise ProgressError(
            "claim-current selected lane must equal the explicit single-lane request"
        )
    if (
        isinstance(max_packets, bool)
        or not isinstance(max_packets, int)
        or max_packets < 1
    ):
        raise ProgressError("claim-current max_packets must be a positive integer")
    return {
        "schema_version": CLAIM_PROVENANCE_SCHEMA_VERSION,
        "command": CLAIM_CURRENT_COMMAND,
        "requested_lane": requested_lane,
        "selected_lane": selected_lane,
        "max_packets": max_packets,
    }


def validate_claim_provenance(value: Any) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        raise ProgressError("work item has no claim-current provenance")
    expected_keys = {
        "schema_version",
        "command",
        "requested_lane",
        "selected_lane",
        "max_packets",
    }
    if set(value) != expected_keys:
        raise ProgressError(
            "claim_provenance must contain exactly schema_version, command, "
            "requested_lane, selected_lane, and max_packets"
        )
    if value.get("schema_version") != CLAIM_PROVENANCE_SCHEMA_VERSION:
        raise ProgressError(
            f"claim_provenance schema_version must be {CLAIM_PROVENANCE_SCHEMA_VERSION}"
        )
    return claim_current_provenance(
        command=value.get("command"),
        requested_lane=value.get("requested_lane"),
        selected_lane=value.get("selected_lane"),
        max_packets=value.get("max_packets"),
    )


def create_and_reserve_claim_current_work_item(
    data: dict[str, Any],
    *,
    work_id: str,
    work: Mapping[str, Any],
    command: str,
    requested_lane: str,
    selected_lane: str,
    max_packets: int,
) -> dict[str, Any]:
    """The sole live work-item insertion path, authorized by claim-current."""
    work_items = data.get("work_items")
    if not isinstance(work_items, dict):
        raise ProgressError("progress work_items collection must be an object")
    if not isinstance(work_id, str) or not work_id:
        raise ProgressError("claim-current work item requires a stable non-empty id")
    if work_id in work_items:
        raise ProgressError(f"claim-current work item already exists: {work_id}")
    if "claim_provenance" in work:
        raise ProgressError("claim-current candidate must not supply its own claim_provenance")
    provenance = claim_current_provenance(
        command=command,
        requested_lane=requested_lane,
        selected_lane=selected_lane,
        max_packets=max_packets,
    )
    if str(work.get("lane", "")) != selected_lane:
        raise ProgressError(
            f"claim-current packet lane {work.get('lane')!r} does not match "
            f"selected lane {selected_lane!r}"
        )
    stored = deepcopy(dict(work))
    stored["claim_provenance"] = provenance
    work_items[work_id] = stored
    try:
        return reserve_work_item(data, work_id)
    except Exception:
        del work_items[work_id]
        raise


def create_and_reserve_repair_continuation_work_item(
    data: dict[str, Any],
    *,
    work_id: str,
    work: Mapping[str, Any],
    predecessor_work_item_id: str,
    checkpoint_id: str,
) -> dict[str, Any]:
    """Insert the sole typed one-hop repair-continuation child.

    This explicit parent route is intentionally separate from claim-current:
    the child inherits one retained terminal predecessor and cannot select or
    broaden scheduler work.
    """

    work_items = data.get("work_items")
    if not isinstance(work_items, dict):
        raise ProgressError("progress work_items collection must be an object")
    if not isinstance(work_id, str) or not work_id:
        raise ProgressError("repair continuation requires a stable non-empty id")
    if work_id in work_items:
        raise ProgressError(f"repair continuation work item already exists: {work_id}")
    predecessor = work_items.get(predecessor_work_item_id)
    if not isinstance(predecessor, Mapping):
        raise ProgressError("repair continuation predecessor is not retained")
    if predecessor.get("state") != "returned-tool-blocked":
        raise ProgressError(
            "repair continuation predecessor must be terminal returned-tool-blocked"
        )
    if work.get("packet_type") != REPAIR_CONTINUATION_PACKET_TYPE:
        raise ProgressError("repair continuation child has the wrong packet type")
    if work.get("state") != "ready":
        raise ProgressError("repair continuation child must begin ready")
    if "claim_provenance" in work:
        raise ProgressError("repair continuation must not claim scheduler provenance")
    provenance = work.get("continuation_provenance")
    if not isinstance(provenance, Mapping):
        raise ProgressError("repair continuation child lacks typed provenance")
    expected_provenance = {
        "schema_version": REPAIR_CONTINUATION_PROVENANCE_SCHEMA_VERSION,
        "command": PREPARE_REPAIR_CONTINUATION_COMMAND,
        "checkpoint_id": checkpoint_id,
        "predecessor_work_item_id": predecessor_work_item_id,
        "hop": 1,
        "max_hops": 1,
    }
    if dict(provenance) != expected_provenance:
        raise ProgressError("repair continuation child provenance drifted")
    exact_inherited_fields = (
        "target_id",
        "cursor",
        "block_id",
        "covered_block_ids",
        "scope_ids",
        "target_ids",
        "original_slice_ids",
        "allowed_paths",
        "source_edit_paths",
        "definition_source_paths",
        "dependency_paths",
    )
    for field in exact_inherited_fields:
        if deepcopy(work.get(field)) != deepcopy(predecessor.get(field)):
            raise ProgressError(
                f"repair continuation child broadened predecessor {field}"
            )
    if not (
        work.get("nonaccepting") is True
        and work.get("acceptance_eligible") is False
        and work.get("candidate_expected_truth") is False
        and work.get("full_convergence_required") is True
    ):
        raise ProgressError(
            "repair continuation child must remain noncurrent and nonaccepting"
        )
    active_continuations = [
        other_id
        for other_id, other in work_items.items()
        if isinstance(other, Mapping)
        and other.get("packet_type") == REPAIR_CONTINUATION_PACKET_TYPE
        and (
            other.get("state") in WORK_HANDOFF_STATES
            or (
                isinstance(other.get("reservation"), Mapping)
                and other["reservation"].get("state") == "active"
            )
        )
    ]
    if active_continuations:
        raise ProgressError(
            "only one repair continuation may be active globally: "
            + ", ".join(sorted(active_continuations))
        )
    stored = bind_work_packet_contract(ProgressDocument(data), deepcopy(dict(work)))
    work_items[work_id] = stored
    try:
        return reserve_work_item(data, work_id)
    except Exception:
        del work_items[work_id]
        raise


def work_leases(document: ProgressDocument, work_id: str) -> dict[str, Any]:
    work = document.collection("work_items").get(work_id)
    if not isinstance(work, Mapping):
        raise ProgressError(f"unknown work item {work_id}")
    claims, complete, source = work_resource_claims(work)
    conflicts: list[dict[str, Any]] = []
    for other_id, other in document.collection("work_items").items():
        if other_id == work_id or not isinstance(other, Mapping):
            continue
        reservation = other.get("reservation")
        if not isinstance(reservation, Mapping) or reservation.get("state") != "active":
            continue
        other_claims, other_complete, _ = work_resource_claims(other)
        if other_complete:
            conflicts.extend(resource_claim_conflicts(claims, str(other_id), other_claims))
    return document.scheduler_output(
        {
            "work_item_id": work_id,
            "packet_contract_version": work.get("packet_contract_version", 1),
            "resource_claims": claims,
            "resource_claims_complete": complete,
            "resource_claim_source": source,
            "reservation": deepcopy(work.get("reservation")),
            "conflicts": conflicts,
            "expires": None,
        }
    )


class ProgressStore:
    def __init__(self, path: str | Path) -> None:
        self.path = Path(path)
        self._is_sqlite = self.path.suffix.lower() in SQLITE_PROGRESS_SUFFIXES
        if self._is_sqlite:
            # SQLite stores are deliberately fail-closed.  Creation belongs to
            # the governed importer, never an ordinary command typo.
            from _recoil.lib.progress_sqlite import ProgressSQLiteStore

            self._store = ProgressSQLiteStore(self.path)
        else:
            # Retained only for one-time import and isolated legacy fixtures.
            # Normal command routing rejects JSON progress authorities.
            self._store = RevisionStore(
                self.path,
                schema_field="schema_version",
                schema_version=SCHEMA_VERSION,
                validator=validate_tracker_v5,
                initializer=empty_progress_document,
            )

    def load(self) -> ProgressDocument:
        try:
            data = (
                self._store.materialize()
                if self._is_sqlite
                else self._store.load()
            )
            return ProgressDocument._from_owned_data(
                data, path=self.path
            )
        except LiveProgressError as exc:
            raise ProgressError(str(exc)) from exc
        except Exception as exc:
            if self._is_sqlite:
                from _recoil.lib.progress_sqlite import ProgressSQLiteError

                if isinstance(exc, ProgressSQLiteError):
                    raise ProgressError(str(exc)) from exc
            raise

    def read_revision(self) -> int:
        """Read only the CAS revision when the backend supports it."""

        try:
            if self._is_sqlite:
                return self._store.read_revision()
            return int(self._store.load()["revision"])
        except LiveProgressError as exc:
            raise ProgressError(str(exc)) from exc
        except Exception as exc:
            if self._is_sqlite:
                from _recoil.lib.progress_sqlite import ProgressSQLiteError

                if isinstance(exc, ProgressSQLiteError):
                    raise ProgressError(str(exc)) from exc
            raise

    def revision(self) -> int:
        """Compatibility alias for lightweight launcher integrations."""

        return self.read_revision()

    def commit(
        self,
        proposed: ProgressDocument | Mapping[str, Any],
        *,
        expected_revision: int,
        apply: bool,
    ) -> CommitResult:
        source = proposed.data if isinstance(proposed, ProgressDocument) else proposed
        # Commit-time metadata refresh mutates nested values, so preserve the
        # caller's proposed document/mapping.  ``mutate`` no longer performs
        # its own preceding full copy, leaving exactly one defensive copy on
        # that hot compatibility path instead of two.
        data = deepcopy(dict(source))
        summary = (
            data.get("binaries", {})
            .get("recoil", {})
            .get("source_layout_context", {})
            .get("provenance_status_summary", {})
        )
        is_authoritative = self.path.resolve() == DEFAULT_PROGRESS_PATH.resolve()
        if is_authoritative or (
            isinstance(summary, Mapping) and "remaining_blocker" in summary
        ):
            # Keep the governed scheduler display record inside the same CAS
            # commit as every tracker mutation.  The local import avoids a
            # module-import cycle: the metadata derivation itself consumes
            # ProgressDocument.
            from _recoil.commands.current_metadata_audit import (
                refresh_remaining_blocker_metadata,
            )

            try:
                refresh_remaining_blocker_metadata(data)
            except ValueError as exc:
                raise ProgressError(
                    "cannot refresh governed current metadata in tracker commit: "
                    f"{exc}"
                ) from exc
        document = ProgressDocument._from_owned_data(data, path=self.path)
        errors = [finding for finding in document.audit() if finding.severity == "error"]
        if errors:
            raise ProgressError("proposed progress document failed audit: " + "; ".join(item.message for item in errors[:8]))
        try:
            result = self._store.commit(
                data, expected_revision=expected_revision, apply=apply
            )
        except ConcurrentRevisionUpdate as exc:
            raise ConcurrentProgressUpdate(str(exc)) from exc
        except LiveProgressError as exc:
            raise ProgressError(str(exc)) from exc
        except Exception as exc:
            if self._is_sqlite:
                from _recoil.lib.progress_sqlite import (
                    ConcurrentSQLiteProgressUpdate,
                    ProgressSQLiteError,
                )

                if isinstance(exc, ConcurrentSQLiteProgressUpdate):
                    raise ConcurrentProgressUpdate(str(exc)) from exc
                if isinstance(exc, ProgressSQLiteError):
                    raise ProgressError(str(exc)) from exc
            raise
        return CommitResult(result.applied, result.path, result.previous_revision, result.revision)

    def create(self, proposed: ProgressDocument | Mapping[str, Any], *, apply: bool) -> CommitResult:
        return self.commit(proposed, expected_revision=0, apply=apply)

    def mutate(
        self,
        transform: Callable[[dict[str, Any]], None],
        *,
        expected_revision: int,
        apply: bool,
    ) -> CommitResult:
        current = self.load()
        if current.revision != expected_revision:
            raise ConcurrentProgressUpdate(
                f"revision changed: expected {expected_revision}, found {current.revision}"
            )
        # ``load`` returns a request-owned graph; mutating it does not touch
        # durable rows.  ``commit`` takes the single defensive copy required
        # before metadata refresh and validation.
        proposed = current.data
        transform(proposed)
        return self.commit(proposed, expected_revision=expected_revision, apply=apply)


def invalidate_order_dependencies(
    data: dict[str, Any],
    *,
    block_ids: Iterable[str] = (),
    symbol_ids: Iterable[str] = (),
) -> dict[str, Any]:
    changed_blocks: list[str] = []
    changed_symbols: list[str] = []
    for block_id in block_ids:
        block = data.get("physical_blocks", {}).get(block_id)
        if not isinstance(block, dict):
            continue
        order = block.get("order", {})
        if isinstance(order, dict):
            for group_name, dimensions in (
                ("authored", AUTHORED_ORDER_DIMENSIONS),
                ("full", FULL_ORDER_DIMENSIONS),
            ):
                group = order.get(group_name, {})
                if isinstance(group, dict):
                    for dimension in dimensions:
                        group[dimension] = state_record("pending", "observed", "changed", [])
        block.pop("accepted_order_facts", None)
        changed_blocks.append(str(block_id))
    for symbol_id in symbol_ids:
        symbol = data.get("symbols", {}).get(symbol_id)
        if not isinstance(symbol, dict):
            continue
        state = symbol.get("binary_state", {})
        if isinstance(state, dict):
            for dimension in SYMBOL_BINARY_DIMENSIONS:
                state[dimension] = state_record("pending", "observed", "changed", [])
        symbol.pop("accepted_byte_facts", None)
        changed_symbols.append(str(symbol_id))
    return {"block_ids": changed_blocks, "symbol_ids": changed_symbols}


def accept_live_authored_non_gating_blocks(
    data: dict[str, Any],
    *,
    block_ids: Iterable[str],
    evidence_id: str,
) -> list[str]:
    """Accept only the authored-order dimensions of reviewed zero-gate blocks.

    This deliberately does not use ``accept_live_order_block``: a non-gating
    physical block has no authored identity sequence to pass, and recording a
    normal order PASS (or accepted order facts) would overstate the comparison.
    Full-order dimensions and every symbol/byte fact remain independent.
    """

    physical_blocks = data.get("physical_blocks")
    if not isinstance(physical_blocks, dict):
        raise ProgressError("progress physical_blocks collection must be an object")
    accepted: list[str] = []
    seen: set[str] = set()
    for raw_block_id in block_ids:
        block_id = str(raw_block_id)
        if block_id in seen:
            raise ProgressError(f"duplicate authored non-gating block id {block_id!r}")
        seen.add(block_id)
        block = physical_blocks.get(block_id)
        if not isinstance(block, dict):
            raise ProgressError(f"unknown physical block {block_id}")
        order = block.get("order")
        if not isinstance(order, dict):
            raise ProgressError(f"physical block {block_id} order must be an object")
        authored = order.get("authored")
        if not isinstance(authored, dict):
            raise ProgressError(
                f"physical block {block_id} authored order must be an object"
            )
        for dimension in AUTHORED_ORDER_DIMENSIONS:
            authored[dimension] = state_record(
                "not-applicable",
                "accepted",
                "current",
                [evidence_id],
                gating=True,
                validation_mode="live",
            )
        accepted.append(block_id)
    return accepted
