from __future__ import annotations

from copy import deepcopy
from dataclasses import dataclass
import json
from pathlib import Path
import re
from typing import Any, Callable, Iterable, Mapping, Sequence

from _recoil.lib.tooling import REPO_ROOT
from _recoil.lib.call_contract_generations import (
    current_generations,
    evidence_generations_current,
    required_call_contract_verifier_component_findings,
)
from _recoil.lib.repository_paths import (
    RepositoryPathError,
    normalize_generated_repository_path,
    validate_repository_relative_path,
)


SCHEMA_VERSION = 6
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
CALL_CONTRACT_CONTRACT_VERSION = 3
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
HEX_ADDRESS_RE = re.compile(r"^(?:0x|sub_)?([0-9a-fA-F]+)$")
EVIDENCE_ID_RE = re.compile(r"^recoil:evidence:r([0-9]+):([0-9]{6,})$")


class ProgressError(ValueError):
    pass


def accepted_byte_mode(facts: Any) -> str | None:
    """Decode canonical or legacy accepted-byte provenance fail-closed.

    New live acceptance writes ``mode``.  Existing accepted rows may retain the
    legacy ``lane`` key; it is compatibility input only and never denotes a
    schedulable workflow lane.
    """

    if facts is None:
        return None
    if not isinstance(facts, Mapping):
        raise ProgressError("accepted_byte_facts must be an object")
    canonical = facts.get("mode")
    legacy = facts.get("lane")
    if canonical is None and legacy is None:
        return None
    if canonical is not None and legacy is not None and canonical != legacy:
        raise ProgressError(
            "accepted_byte_facts mode and legacy lane must agree"
        )
    value = canonical if canonical is not None else legacy
    if value not in {"authored", "linked"}:
        raise ProgressError(
            f"accepted_byte_facts mode must be 'authored' or 'linked', got {value!r}"
        )
    return str(value)


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
    """Normalize replayed path claims without inventing current repository identity.

    Resource-claim rows are persisted concurrency identities.  Their replay
    remains lexical; current command consumers authenticate tracked files at
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
    mode: str,
    retail_start: str,
    retail_end_exclusive: str,
) -> tuple[str, dict[str, Any]]:
    """Resolve one registered order target to one exact physical block."""

    if mode not in {"object", "linked"}:
        raise ProgressError(f"invalid full-order target mode {mode!r}")
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
        and block["order_targets"].get(mode) == target_name
    ]
    if len(matches) != 1:
        raise ProgressError(
            f"expected one exact current {mode} order block for {target_name!r} "
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
        their defensive-copy contract for caller-owned mappings. The SQLite
        store already returns a fresh object graph owned by this request, so
        copying that graph again only adds load latency and memory.
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
        if progress_path.suffix.lower() not in SQLITE_PROGRESS_SUFFIXES:
            raise ProgressError("the progress authority is SQLite-only")
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

    @property
    def revision(self) -> int:
        value = self.data.get("revision", -1)
        return value if isinstance(value, int) and not isinstance(value, bool) else -1

    def clone(self) -> "ProgressDocument":
        return ProgressDocument(self.data, path=self.path)

    def revision_identity(self) -> dict[str, Any]:
        if self.path is not None and self.path.suffix.casefold() in SQLITE_PROGRESS_SUFFIXES:
            from _recoil.lib.progress_sqlite import read_progress_metadata

            metadata = read_progress_metadata(self.path)
            vector = {
                "transaction_revision": metadata.transaction_revision,
                "semantic_revision": metadata.semantic_revision,
                "evidence_generation_revision": metadata.evidence_generation_revision,
            }
        else:
            vector = {
                "transaction_revision": self.revision,
                "semantic_revision": self.revision,
                "evidence_generation_revision": self.revision,
            }
        return {
            "tracker_revision": vector["transaction_revision"],
            "revision": vector["transaction_revision"],
            "revision_vector": vector,
            **vector,
        }

    def with_revision_vector(self, value: Mapping[str, Any]) -> dict[str, Any]:
        result = deepcopy(dict(value))
        result.update(self.revision_identity())
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

    def _live_byte_command(self, mode: str, cursor: str) -> str:
        commands = {
            "authored": "advance-live-authored-byte",
            "linked": "advance-live-linked-byte",
        }
        try:
            command = commands[mode]
        except KeyError as exc:
            raise ProgressError(f"unsupported serial byte stage {mode!r}") from exc
        root = self._fresh_root(mode, cursor.replace("0x", ""), self.revision)
        return (
            f"python tools/recoil.py progress {command} "
            f"--build-root {root} --expected-revision {self.revision} "
            "--apply --json"
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
            # when that projection has no implementation root. Current repository
            # spelling is authenticated later from the canonical checkout's
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
        # Currentness is a live local observation.  Recheck the registered
        # component graph on every call so one successful query on this
        # document cannot mask a later missing, unreadable, or unparseable
        # required component.
        component_findings = required_call_contract_verifier_component_findings()
        if component_findings:
            return {
                "current": False,
                "reason": "verifier-component-unavailable",
                "evidence_id": evidence_id,
                "component_findings": deepcopy(component_findings),
            }
        if not evidence_generations_current(provenance):
            return {"current": False, "reason": "verifier-generation-changed", "evidence_id": evidence_id}
        transcript = provenance.get("exact_fact_transcript")
        transcript_row = (
            transcript[0]
            if isinstance(transcript, list) and len(transcript) == 1
            else None
        )
        expected_fact_row = (
            transcript_row.get("expected_fact_row")
            if isinstance(transcript_row, Mapping)
            else None
        )
        if (
            provenance.get("symbol_id") != symbol_id
            or provenance.get("address")
            != normalize_address(str(symbol.get("address", symbol.get("start", ""))))
            or provenance.get("physical_block_id")
            != str(symbol.get("physical_block_id", ""))
            or provenance.get("comparison_passed") is not True
            or not isinstance(transcript_row, Mapping)
            or transcript_row.get("symbol_id") != symbol_id
            or transcript_row.get("address") != provenance.get("address")
            or not isinstance(expected_fact_row, Mapping)
            or expected_fact_row.get("symbol_id") != symbol_id
            or expected_fact_row.get("address") != provenance.get("address")
            or expected_fact_row.get("calls") != provenance.get("expected_contract")
            or not evidence_generations_current(expected_fact_row)
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
        closeout = migration.get("authored_call_contract_fresh_closeout_v3") if isinstance(migration, Mapping) else None
        scan_rows = closeout.get("scan_rows") if isinstance(closeout, Mapping) else None
        scan_rows_valid = isinstance(scan_rows, list) and len(scan_rows) == len(slices)
        if scan_rows_valid:
            for slice_row, scan_row in zip(slices, scan_rows):
                transcript = (
                    scan_row.get("exact_fact_transcript")
                    if isinstance(scan_row, Mapping)
                    else None
                )
                expected_pairs = list(zip(
                    slice_row["symbol_ids"], slice_row["addresses"]
                ))
                if (
                    not isinstance(scan_row, Mapping)
                    or scan_row.get("slice_id") != slice_row.get("id")
                    or scan_row.get("body_count") != slice_row.get("body_count")
                    or not isinstance(transcript, list)
                    or len(transcript) != len(expected_pairs)
                ):
                    scan_rows_valid = False
                    break
                for (symbol_id, address), transcript_row in zip(
                    expected_pairs, transcript
                ):
                    expected_fact_row = (
                        transcript_row.get("expected_fact_row")
                        if isinstance(transcript_row, Mapping)
                        else None
                    )
                    if (
                        not isinstance(transcript_row, Mapping)
                        or transcript_row.get("symbol_id") != symbol_id
                        or transcript_row.get("address") != normalize_address(str(address))
                        or not isinstance(expected_fact_row, Mapping)
                        or not evidence_generations_current(expected_fact_row)
                    ):
                        scan_rows_valid = False
                        break
                if not scan_rows_valid:
                    break
        valid = (
            not incomplete
            and isinstance(closeout, Mapping)
            and closeout.get("schema") == "recoil-call-contract-fresh-closeout-v3"
            and closeout.get("ordered_symbol_ids") == ordered
            and closeout.get("complete_no_reuse_zero_divergence") is True
            and scan_rows_valid
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
        vector = self.revision_identity()["revision_vector"]
        root = self._fresh_root(
            "call-contract",
            slice_id.rsplit(":", 1)[-1].replace("0x", ""),
            self.revision,
        )
        return (
            "python tools/recoil.py progress advance-live-call-contract "
            f"--slice {slice_id} --build-root {root} "
            f"--expected-semantic-revision {vector['semantic_revision']} "
            "--expected-evidence-generation-revision "
            f"{vector['evidence_generation_revision']} --apply --json"
        )

    def pipeline(
        self,
        binary: str = "recoil",
        *,
        resolve_order_target: bool = True,
    ) -> dict[str, Any]:
        """Derive the strict serial reconstruction stage and its one next action."""

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
            if not self._order_group_current(
                block, "authored", AUTHORED_ORDER_DIMENSIONS
            )
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
        authored_byte_cursor = (
            str(authored_pending_groups[0]["address"])
            if authored_pending_groups
            else ""
        )
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
        linked_byte_cursor = (
            str(linked_pending_groups[0]["address"])
            if linked_pending_groups
            else ""
        )
        linked_byte_end = linked_byte_cursor or text_end

        gating_symbols = [
            symbol
            for _symbol_id, symbol in self._symbols_for_binary(binary)
            if symbol_authored_order_gate(symbol)
        ]
        initialized = sum(
            1
            for symbol in gating_symbols
            if isinstance(symbol.get("binary_state"), Mapping)
            and CALL_CONTRACT_DIMENSION in symbol["binary_state"]
        )
        call_stage_enabled = bool(gating_symbols) and initialized == len(gating_symbols)
        call_error = ""
        if initialized not in {0, len(gating_symbols)}:
            call_error = (
                "authored call-contract initialization is partial: "
                f"{initialized}/{len(gating_symbols)} bodies"
            )

        call_slices: list[dict[str, Any]] = []
        call_pending: list[dict[str, Any]] = []
        closeout = {"current": False, "reason": "not-applicable"}
        closeout_pending = False
        if not authored_pending and (call_stage_enabled or call_error):
            try:
                call_slices = self.authored_call_contract_slices(binary)
            except ProgressError as exc:
                call_error = str(exc)
            else:
                for item in call_slices:
                    status = self._call_contract_slice_status(item)
                    if status["current"]:
                        continue
                    call_pending.append(
                        {
                            **item,
                            "accepted_body_count": status["accepted_body_count"],
                            "remaining_body_count": status["remaining_body_count"],
                            "pending_start": status["first_pending_address"],
                            "pending_physical_block_id": status[
                                "first_pending_physical_block_id"
                            ],
                        }
                    )
                if call_slices and not call_pending:
                    closeout = self._call_contract_phase_closeout_status(call_slices)
                    closeout_pending = not bool(closeout["current"])

        if authored_pending:
            stage = "authored-function-order"
            block_id, block = authored_pending[0]
            cursor = normalize_address(_entity_range(block)[0])
        elif call_error:
            stage = "authored-call-contract"
            block_id, block, cursor = "", {}, authored_order_end
        elif call_pending:
            stage = "authored-call-contract"
            selected = call_pending[0]
            cursor = str(selected.get("pending_start") or selected["start"])
            block_id = str(
                selected.get("pending_physical_block_id")
                or selected["physical_block_ids"][0]
            )
            block = self.collection("physical_blocks").get(block_id, {})
        elif closeout_pending:
            stage = "authored-call-contract"
            block_id, block, cursor = "", {}, authored_order_end
        elif authored_pending_groups:
            stage = "authored-byte-match"
            block_id = str(authored_pending_groups[0]["physical_block_id"])
            block = self.collection("physical_blocks").get(block_id, {})
            cursor = authored_byte_cursor
        elif full_pending:
            stage = "full-function-order"
            block_id, block = full_pending[0]
            cursor = normalize_address(_entity_range(block)[0])
        elif linked_pending_groups:
            stage = "linked-byte-match"
            block_id = str(linked_pending_groups[0]["physical_block_id"])
            block = self.collection("physical_blocks").get(block_id, {})
            cursor = linked_byte_cursor
        else:
            stage = "final-validation"
            block_id, block, cursor = "", {}, text_end

        order_resolution: dict[str, Any] = {
            "status": "not-applicable",
            "phase": stage,
        }
        blocker = call_error
        if stage in {"authored-function-order", "full-function-order"}:
            if resolve_order_target and isinstance(block, Mapping):
                order_resolution = self._order_target_resolution()
                next_command = self._live_order_command(stage, order_resolution)
                if not next_command:
                    blocker = str(order_resolution.get("reason", ""))
            else:
                next_command = ""
                order_resolution = {"status": "deferred", "phase": stage}
        elif stage == "authored-call-contract" and call_pending:
            next_command = self._live_call_contract_command(str(call_pending[0]["id"]))
        elif stage == "authored-call-contract" and closeout_pending:
            vector = self.revision_identity()["revision_vector"]
            root = self._fresh_root("call-contract-closeout", "all", self.revision)
            next_command = (
                "python tools/recoil.py progress call-contract close-live "
                f"--build-root {root} "
                f"--expected-semantic-revision {vector['semantic_revision']} "
                "--expected-evidence-generation-revision "
                f"{vector['evidence_generation_revision']} --apply --json"
            )
        elif stage == "authored-byte-match":
            next_command = self._live_byte_command("authored", cursor)
        elif stage == "linked-byte-match":
            next_command = self._live_byte_command("linked", cursor)
        elif stage == "final-validation":
            next_command = "python tools/recoil.py verify final-image --json"
        else:
            next_command = ""

        total_call = sum(int(row["body_count"]) for row in call_slices)
        remaining_call = sum(
            int(row.get("remaining_body_count", row["body_count"]))
            for row in call_pending
        )
        complete = stage == "final-validation" and not linked_pending_groups
        state = {
            "binary": binary,
            "phase": stage,
            "cursor": cursor,
            "physical_block_id": block_id,
            "complete": complete,
            "blocker": blocker,
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
                str(call_pending[0].get("pending_start", ""))
                if call_pending
                else ""
            ),
            "authored_call_contract_slice_id": (
                str(call_pending[0]["id"]) if call_pending else ""
            ),
            "authored_byte_match_frontier": authored_byte_end,
            "byte_prefix_end": authored_byte_end,
            "exact_byte_prefix_end": linked_byte_end,
            "linked_byte_match_prefix_end": linked_byte_end,
            "authored_function_order_counts": {
                "accepted": len(blocks) - len(authored_pending),
                "remaining": len(authored_pending),
                "total": len(blocks),
            },
            "full_function_order_counts": {
                "accepted": len(blocks) - len(full_pending),
                "remaining": len(full_pending),
                "total": len(blocks),
            },
            "authored_call_contract_counts": {
                "accepted": total_call - remaining_call,
                "remaining": remaining_call,
                "total": total_call,
                "slice_count": len(call_slices),
                "remaining_slices": len(call_pending),
                "initialized_bodies": initialized,
                "stage_enabled": call_stage_enabled,
            },
            "authored_call_contract_closeout": closeout,
            "authored_byte_counts": {
                "accepted": len(authored_groups) - len(authored_pending_groups),
                "remaining": len(authored_pending_groups),
                "total": len(authored_groups),
            },
            "linked_byte_counts": {
                "accepted": len(linked_groups) - len(linked_pending_groups),
                "remaining": len(linked_pending_groups),
                "total": len(linked_groups),
            },
            "next_command": next_command,
            "order_target_resolution": order_resolution,
            "primary_phase_progress": {
                "kind": "accepted-prefix-end",
                "phase": stage,
                "value": cursor,
            },
        }
        return self.with_revision_vector(state)

    def current_task(self, binary: str = "recoil") -> dict[str, Any]:
        """Project the sole current task from the fixed serial stage order."""

        state = self.pipeline(binary)
        stage = str(state["phase"])
        cursor = str(state.get("cursor", ""))
        block_id = str(state.get("physical_block_id", ""))
        slice_id = str(state.get("authored_call_contract_slice_id", ""))
        task_id = (
            slice_id
            if stage == "authored-call-contract" and slice_id
            else block_id or cursor
        )
        source_paths: list[str] = []
        target_ids: list[str] = []
        if block_id:
            block = self.collection("physical_blocks").get(block_id, {})
            if isinstance(block, Mapping):
                for key in (
                    "agent_source_path",
                    "original_source_path",
                    "source_path",
                ):
                    value = block.get(key)
                    if isinstance(value, str) and value and value not in source_paths:
                        source_paths.append(value)
                target_ids.extend(
                    str(value) for value in block.get("order_targets", []) if value
                )
        if stage == "authored-call-contract" and slice_id:
            selected = next(
                (
                    row
                    for row in self.authored_call_contract_slices(binary)
                    if row["id"] == slice_id
                ),
                None,
            )
            if isinstance(selected, Mapping):
                target_ids = [
                    str(value) for value in selected.get("target_ids", [])
                ]
        objectives = {
            "authored-function-order": "Make the current authored order target pass exactly.",
            "authored-call-contract": (
                "Replay the authored call-contract census, diagnose any first "
                "divergent slice directly, then record a fresh complete closeout."
            ),
            "authored-byte-match": "Make the current authored physical group match retail bytes and relocations.",
            "full-function-order": "Make the current full linked-order target pass exactly.",
            "linked-byte-match": "Make the current linked physical group match retail placement, targets, and bytes.",
            "final-validation": "Complete live typed final-image validation.",
        }
        acceptance = str(state.get("next_command", "")) or None
        stage_runner_command = (
            "python tools/recoil.py progress call-contract replay-live "
            "--apply --json"
            if stage == "authored-call-contract"
            else None
        )
        check_command: str | None = None
        if stage == "authored-call-contract" and slice_id:
            check_command = (
                "python tools/recoil.py verify call-contract "
                f"--slice {slice_id} --build-root <fresh-root> --json"
            )
        elif stage == "authored-byte-match":
            check_command = "python tools/recoil.py verify authored-byte --at " + cursor
        elif stage == "linked-byte-match":
            check_command = "python tools/recoil.py verify linked-byte --at " + cursor
        return {
            "schema": "recoil-current-task-v2",
            "binary": binary,
            "stage": stage,
            "task_id": task_id,
            "state": (
                "complete"
                if state.get("complete")
                else ("ready" if acceptance else "blocked")
            ),
            "cursor": cursor,
            "scope": {
                "physical_block_id": block_id,
                "target_ids": target_ids,
                "source_paths": source_paths,
            },
            "objective": objectives[stage],
            "check_command": check_command,
            "stage_runner_command": stage_runner_command,
            "acceptance_command": acceptance,
            "blocker": None
            if acceptance
            else str(state.get("blocker", ""))
            or "no direct acceptance command is currently derivable",
            "revision_vector": self.revision_identity()["revision_vector"],
        }




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
        return self.with_revision_vector(result)

    def find(self, query: str, limit: int = 100) -> dict[str, Any]:
        needle = query.casefold()
        matches: list[dict[str, Any]] = []
        for collection_name in TOP_LEVEL_COLLECTIONS:
            for entity_id, row in self.collection(collection_name).items():
                if needle in entity_id.casefold() or needle in json.dumps(row, ensure_ascii=False).casefold():
                    matches.append({"collection": collection_name, "id": entity_id, "record": deepcopy(row)})
                    if len(matches) >= limit:
                        return self.with_revision_vector({"query": query, "matches": matches, "truncated": True})
        return self.with_revision_vector({"query": query, "matches": matches, "truncated": False})

    def summary(self) -> dict[str, Any]:
        return self.with_revision_vector(
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
        symbols = self.data.get("symbols", {})
        if isinstance(symbols, Mapping):
            for symbol_id, symbol in symbols.items():
                if not isinstance(symbol, Mapping) or symbol.get("accepted_byte_facts") is None:
                    continue
                facts = symbol.get("accepted_byte_facts")
                try:
                    mode = accepted_byte_mode(facts)
                except ProgressError as exc:
                    findings.append(
                        Finding(
                            "error",
                            "symbol.accepted-byte-facts",
                            str(exc),
                            str(symbol_id),
                        )
                    )
                    continue
                if mode is None:
                    findings.append(
                        Finding(
                            "error",
                            "symbol.accepted-byte-facts",
                            "accepted_byte_facts must identify authored or linked mode",
                            str(symbol_id),
                        )
                    )
                if not isinstance(facts, Mapping) or facts.get("validation_mode") != "live":
                    findings.append(
                        Finding(
                            "error",
                            "symbol.accepted-byte-facts-live",
                            "accepted_byte_facts must come from live validation",
                            str(symbol_id),
                        )
                    )
        from _recoil.lib.authored_icf import audit_authored_icf_groups

        for code, message in audit_authored_icf_groups(self.data):
            findings.append(Finding("error", code, message))
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






































_EXPLICIT_CLEANUP_RECOVERY_AUTHORITY = object()


























class ProgressStore:
    """Fail-closed SQLite-only progress authority."""

    def __init__(self, path: str | Path) -> None:
        self.path = Path(path)
        if self.path.suffix.casefold() not in SQLITE_PROGRESS_SUFFIXES:
            raise ProgressError("the progress authority is SQLite-only")
        from _recoil.lib.progress_sqlite import ProgressSQLiteStore

        self._store = ProgressSQLiteStore(self.path)

    def load(self) -> ProgressDocument:
        try:
            data = self._store.materialize()
            return ProgressDocument._from_owned_data(data, path=self.path)
        except Exception as exc:
            from _recoil.lib.progress_sqlite import ProgressSQLiteError

            if isinstance(exc, ProgressSQLiteError):
                raise ProgressError(str(exc)) from exc
            raise

    def read_revision(self) -> int:
        try:
            return self._store.read_revision()
        except Exception as exc:
            from _recoil.lib.progress_sqlite import ProgressSQLiteError

            if isinstance(exc, ProgressSQLiteError):
                raise ProgressError(str(exc)) from exc
            raise

    def revision(self) -> int:
        return self.read_revision()

    def commit(
        self,
        proposed: ProgressDocument | Mapping[str, Any],
        *,
        expected_revision: int,
        apply: bool,
    ) -> CommitResult:
        source = proposed.data if isinstance(proposed, ProgressDocument) else proposed
        data = deepcopy(dict(source))
        document = ProgressDocument._from_owned_data(data, path=self.path)
        errors = [
            finding
            for finding in document.audit()
            if finding.severity == "error"
        ]
        if errors:
            raise ProgressError(
                "proposed progress document failed audit: "
                + "; ".join(item.message for item in errors[:8])
            )
        try:
            result = self._store.commit(
                data,
                expected_revision=expected_revision,
                apply=apply,
            )
        except Exception as exc:
            from _recoil.lib.progress_sqlite import (
                ConcurrentSQLiteProgressUpdate,
                ProgressSQLiteError,
            )

            if isinstance(exc, ConcurrentSQLiteProgressUpdate):
                raise ConcurrentProgressUpdate(str(exc)) from exc
            if isinstance(exc, ProgressSQLiteError):
                raise ProgressError(str(exc)) from exc
            raise
        return CommitResult(
            result.applied,
            result.path,
            result.previous_revision,
            result.revision,
        )

    def create(
        self,
        proposed: ProgressDocument | Mapping[str, Any],
        *,
        apply: bool,
    ) -> CommitResult:
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
                f"revision changed: expected {expected_revision}, "
                f"found {current.revision}"
            )
        transform(current.data)
        return self.commit(
            current.data,
            expected_revision=expected_revision,
            apply=apply,
        )


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
