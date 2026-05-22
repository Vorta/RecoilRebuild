from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
from typing import Iterable


DONE_STATUS = "✅"
LIMITED_STATUS = "☑️"
NOT_DONE_STATUS = "❌"
UNKNOWN_STATUS = "❓"
VALID_STATUSES = {DONE_STATUS, LIMITED_STATUS, NOT_DONE_STATUS, UNKNOWN_STATUS}
ACCEPTED_STATUSES = {DONE_STATUS, LIMITED_STATUS}

STATUS_RE = re.compile(r"\[([^\]]+)\]")
ENTRY_RE = re.compile(r"^- (0x[0-9A-Fa-f]+):\s*$")
NAME_RE = re.compile(r"Name:\s*(.*?)(?:\s+File:|\))")
FILE_RE = re.compile(r"File:\s*([^)]+)")
PROVIDER_RE = re.compile(r"Provided by\s*([^;]+);")
FUNCTIONAL_LABEL = "Functional-equivalent"
FUNCTIONAL_TARGET_RE = re.compile(r"Target:\s*([^)]+)")
FUNCTIONAL_LANE = "functional"
BINARY_LANE = "binary"
LANE_ALIASES = {
    "functional": FUNCTIONAL_LANE,
    "progress": FUNCTIONAL_LANE,
    "binary": BINARY_LANE,
    "strict": BINARY_LANE,
}
LANE_CHOICES = sorted(LANE_ALIASES)

REQUIRED_FIELD_LABELS = {
    "recon": "Reconstructed",
    "deps": "Source dependencies satisfied",
    "impl": "Reimplemented",
    "verify": "Binary-safe",
}

FIELD_LABELS = {
    **REQUIRED_FIELD_LABELS,
    "functional": FUNCTIONAL_LABEL,
}

FIELD_ALIASES = {
    "reconstructed": "recon",
    "source-dependencies": "deps",
    "source_dependencies": "deps",
    "dependencies": "deps",
    "dependency": "deps",
    "reimplemented": "impl",
    "implementation": "impl",
    "binary-safe": "verify",
    "binary_safe": "verify",
    "binary": "verify",
    "verified": "verify",
    "functional": "functional",
    "functional-equivalent": "functional",
    "functional_equivalent": "functional",
    "functionally-accepted": "functional",
    "functionally_accepted": "functional",
}


@dataclass(frozen=True)
class PlanEntry:
    address: str
    reconstructed_status: str = "?"
    reconstructed_name: str = ""
    source_dependencies_status: str = "?"
    reimplemented_status: str = "?"
    reimplemented_name: str = ""
    reimplemented_file: str = ""
    provider: str = ""
    binary_verified_status: str = "?"
    functional_equivalent_status: str = "?"
    functional_target: str = ""
    milestone: str = ""

    @property
    def is_provider(self) -> bool:
        return bool(self.provider) or self.reimplemented_file == "external"

    @property
    def is_reimplemented(self) -> bool:
        return self.reimplemented_status == DONE_STATUS

    @property
    def is_source_dependencies_satisfied(self) -> bool:
        return self.source_dependencies_status in {DONE_STATUS, LIMITED_STATUS}

    @property
    def is_binary_verified(self) -> bool:
        return self.binary_verified_status == DONE_STATUS

    @property
    def is_functionally_accepted(self) -> bool:
        return self.functional_equivalent_status == DONE_STATUS

    @property
    def is_functionally_equivalent(self) -> bool:
        return self.functional_equivalent_status == DONE_STATUS


@dataclass(frozen=True)
class PlanEntryBlock:
    entry: PlanEntry
    start_line: int
    end_line: int
    marker_lines: dict[str, int]


class PlanDocument:
    def __init__(
        self,
        path: Path,
        lines: list[str],
        entries: dict[str, PlanEntry],
        order: list[str],
        blocks: dict[str, PlanEntryBlock],
        newline: str = "\n",
    ) -> None:
        self.path = path
        self.lines = lines
        self.entries = entries
        self.order = order
        self.blocks = blocks
        self.newline = newline

    @classmethod
    def load(cls, path: str | Path) -> "PlanDocument":
        plan_path = Path(path)
        text = plan_path.read_text(encoding="utf-8")
        newline = "\r\n" if "\r\n" in text else "\n"
        lines = text.splitlines()
        return cls._parse_lines(plan_path, lines, newline)

    def save(self, path: str | Path | None = None) -> None:
        target = Path(path) if path is not None else self.path
        target.write_text(self.newline.join(self.lines) + self.newline, encoding="utf-8")

    def entry_text(self, address: str) -> str:
        address = normalize_address(address)
        block = self._block(address)
        return "\n".join(self.lines[block.start_line : block.end_line])

    def find(self, query: str) -> list[PlanEntry]:
        needle = query.lower()
        matches: list[PlanEntry] = []
        for address in self.order:
            entry = self.entries[address]
            haystack = " ".join(
                [
                    entry.address,
                    entry.milestone,
                    entry.reconstructed_name,
                    entry.reimplemented_name,
                    entry.reimplemented_file,
                    entry.provider,
                ]
            ).lower()
            if needle in haystack:
                matches.append(entry)
        return matches

    def milestone_entries(self, milestone_query: str) -> list[PlanEntry]:
        query = milestone_query.lower()
        return [
            self.entries[address]
            for address in self.order
            if self.entries[address].milestone.lower().startswith(query)
            or query in self.entries[address].milestone.lower()
        ]

    def first_unfinished(self, *, lane: str = FUNCTIONAL_LANE) -> PlanEntry | None:
        for address in self.order:
            entry = self.entries[address]
            if blocker_field(entry, lane=lane):
                return entry
        return None

    def update_marker(
        self,
        address: str,
        field: str,
        status: str,
        *,
        name: str = "",
        file: str = "",
        provider: str = "",
        evidence: str = "",
    ) -> tuple[str, str]:
        address = normalize_address(address)
        field = normalize_field(field)
        if status not in VALID_STATUSES:
            raise ValueError(f"invalid status {status!r}; expected one of {sorted(VALID_STATUSES)}")
        if status in ACCEPTED_STATUSES and not evidence:
            raise ValueError(f"setting {field} to {status} requires --evidence")

        before = self.entry_text(address)
        block = self._block(address)
        line_index = block.marker_lines.get(field, -1)
        entry = block.entry
        self._validate_marker_order(entry, field, status)

        if field == "recon":
            marker_name = name or entry.reconstructed_name
            if not marker_name:
                raise ValueError("recon updates require --name or an existing name")
            new_line = f"  - [{status}] Reconstructed (Name: {marker_name})"
        elif field == "deps":
            new_line = f"  - [{status}] Source dependencies satisfied"
        elif field == "impl":
            new_line = self._reimplemented_line(status, entry, name, file, provider)
        elif field == "verify":
            new_line = f"  - [{status}] Binary-safe"
        elif field == "functional":
            target = file or entry.functional_target
            if status in ACCEPTED_STATUSES and not target:
                raise ValueError("functional updates require --target as the functional target")
            if status in {NOT_DONE_STATUS, UNKNOWN_STATUS}:
                target = target or "pending"
            new_line = f"  - [{status}] {FUNCTIONAL_LABEL} (Target: {target})"
        else:
            raise AssertionError(f"unhandled field {field}")

        if field == "functional" and line_index < 0:
            self.lines.insert(block.marker_lines["verify"], new_line)
        else:
            self.lines[line_index] = new_line
        updated = PlanDocument.load_from_lines(self.path, self.lines, self.newline)
        self.entries = updated.entries
        self.order = updated.order
        self.blocks = updated.blocks
        self.newline = updated.newline
        return before, self.entry_text(address)

    @classmethod
    def load_from_lines(cls, path: Path, lines: list[str], newline: str = "\n") -> "PlanDocument":
        temp = cls(path, lines[:], {}, [], {}, newline)
        return cls._parse_lines(path, temp.lines, newline)

    @classmethod
    def _parse_lines(cls, path: Path, lines: list[str], newline: str = "\n") -> "PlanDocument":
        entries: dict[str, PlanEntry] = {}
        blocks: dict[str, PlanEntryBlock] = {}
        order: list[str] = []
        milestone = ""
        i = 0
        while i < len(lines):
            line = lines[i]
            if line.startswith("## M"):
                milestone = line[3:].strip()
                i += 1
                continue
            match = ENTRY_RE.match(line)
            if not match:
                i += 1
                continue
            start = i
            address = normalize_address(match.group(1))
            if address in entries:
                raise ValueError(f"duplicate plan entry for {address}")
            block: list[tuple[int, str]] = []
            i += 1
            while i < len(lines) and not ENTRY_RE.match(lines[i]) and not lines[i].startswith("## "):
                block.append((i, lines[i]))
                i += 1
            marker_lines: dict[str, int] = {}
            for field, label in REQUIRED_FIELD_LABELS.items():
                matches = [(line_no, text) for line_no, text in block if label in text]
                if len(matches) != 1:
                    raise ValueError(
                        f"{address} expected one {label!r} marker, found {len(matches)}"
                    )
                marker_lines[field] = matches[0][0]
            functional_matches = [
                (line_no, text) for line_no, text in block if FUNCTIONAL_LABEL in text
            ]
            if len(functional_matches) > 1:
                raise ValueError(
                    f"{address} expected at most one {FUNCTIONAL_LABEL!r} marker, "
                    f"found {len(functional_matches)}"
                )
            if functional_matches:
                marker_lines["functional"] = functional_matches[0][0]
            reconstructed = lines[marker_lines["recon"]]
            source_dependencies = lines[marker_lines["deps"]]
            reimplemented = lines[marker_lines["impl"]]
            verified = lines[marker_lines["verify"]]
            functional = lines[marker_lines["functional"]] if "functional" in marker_lines else ""
            entry = PlanEntry(
                address=address,
                reconstructed_status=_status(reconstructed),
                reconstructed_name=_name(reconstructed),
                source_dependencies_status=_status(source_dependencies),
                reimplemented_status=_status(reimplemented),
                reimplemented_name=_name(reimplemented),
                reimplemented_file=_file(reimplemented),
                provider=_provider(reimplemented),
                binary_verified_status=_status(verified),
                functional_equivalent_status=_status(functional) if functional else "?",
                functional_target=_functional_target(functional) if functional else "",
                milestone=milestone,
            )
            entries[address] = entry
            order.append(address)
            blocks[address] = PlanEntryBlock(entry, start, i, marker_lines)
        return cls(path, lines, entries, order, blocks, newline)

    def _block(self, address: str) -> PlanEntryBlock:
        try:
            return self.blocks[address]
        except KeyError as exc:
            raise ValueError(f"plan entry not found: {address}") from exc

    def _reimplemented_line(
        self,
        status: str,
        entry: PlanEntry,
        name: str,
        file: str,
        provider: str,
    ) -> str:
        if status in {NOT_DONE_STATUS, UNKNOWN_STATUS}:
            return f"  - [{status}] Reimplemented (Name: pending File: pending)"

        if provider:
            impl_name = name or entry.reimplemented_name
            impl_file = file or entry.reimplemented_file or "external"
            if not impl_name:
                raise ValueError("provider impl updates require --name or an existing name")
            return (
                f"  - [{status}] Reimplemented (Provided by {provider}; "
                f"Name: {impl_name} File: {impl_file})"
            )

        impl_name = name or entry.reimplemented_name
        impl_file = file or entry.reimplemented_file
        if not impl_name or not impl_file or impl_name == "pending" or impl_file == "pending":
            raise ValueError("authored impl updates require --name and --file")
        return f"  - [{status}] Reimplemented (Name: {impl_name} File: {impl_file})"

    def _validate_marker_order(self, entry: PlanEntry, field: str, status: str) -> None:
        if status not in ACCEPTED_STATUSES:
            return

        def require(condition: bool, message: str) -> None:
            if not condition:
                raise ValueError(f"setting {field} to {status} requires {message}")

        if field == "recon":
            return
        if field == "deps":
            require(
                entry.reconstructed_status in ACCEPTED_STATUSES,
                "Reconstructed to be ✅ or ☑️",
            )
            return
        if field == "impl":
            require(
                entry.reconstructed_status in ACCEPTED_STATUSES,
                "Reconstructed to be ✅ or ☑️",
            )
            require(
                entry.source_dependencies_status in ACCEPTED_STATUSES,
                "Source dependencies satisfied to be ✅ or ☑️",
            )
            return
        if field == "functional":
            require(
                entry.reconstructed_status in ACCEPTED_STATUSES,
                "Reconstructed to be ✅ or ☑️",
            )
            require(
                entry.source_dependencies_status in ACCEPTED_STATUSES,
                "Source dependencies satisfied to be ✅ or ☑️",
            )
            require(entry.reimplemented_status == DONE_STATUS, "Reimplemented to be ✅")
            return
        if field == "verify":
            require(
                entry.reconstructed_status in ACCEPTED_STATUSES,
                "Reconstructed to be ✅ or ☑️",
            )
            require(
                entry.source_dependencies_status in ACCEPTED_STATUSES,
                "Source dependencies satisfied to be ✅ or ☑️",
            )
            require(entry.reimplemented_status == DONE_STATUS, "Reimplemented to be ✅")
            require(
                entry.functional_equivalent_status == DONE_STATUS,
                "Functional-equivalent to be ✅",
            )


def _status(line: str) -> str:
    match = STATUS_RE.search(line)
    return match.group(1) if match else "?"


def _name(line: str) -> str:
    match = NAME_RE.search(line)
    return match.group(1).strip() if match else ""


def _file(line: str) -> str:
    match = FILE_RE.search(line)
    return match.group(1).strip() if match else ""


def _provider(line: str) -> str:
    match = PROVIDER_RE.search(line)
    return match.group(1).strip() if match else ""


def _functional_target(line: str) -> str:
    match = FUNCTIONAL_TARGET_RE.search(line)
    return match.group(1).strip() if match else ""


def load_plan(path: str | Path) -> dict[str, PlanEntry]:
    return PlanDocument.load(path).entries


def normalize_address(value: str) -> str:
    value = value.strip()
    if value.lower().startswith("sub_"):
        value = "0x" + value[4:]
    if value.lower().startswith("0x"):
        return f"0x{int(value, 16):x}"
    return f"0x{int(value, 16):x}"


def normalize_field(value: str) -> str:
    key = value.strip().lower().replace(" ", "-")
    key = FIELD_ALIASES.get(key, key)
    if key not in FIELD_LABELS:
        valid = ", ".join(sorted(FIELD_LABELS | FIELD_ALIASES))
        raise ValueError(f"invalid field {value!r}; expected one of: {valid}")
    return key


def normalize_lane(value: str) -> str:
    key = value.strip().lower().replace("_", "-")
    lane = LANE_ALIASES.get(key)
    if lane is None:
        valid = ", ".join(LANE_CHOICES)
        raise ValueError(f"invalid lane {value!r}; expected one of: {valid}")
    return lane


def status_summary(entry: PlanEntry, *, include_functional: bool = True) -> str:
    _ = include_functional
    return (
        f"recon={entry.reconstructed_status} "
        f"deps={entry.source_dependencies_status} "
        f"impl={entry.reimplemented_status} "
        f"functional={entry.functional_equivalent_status} "
        f"binary={entry.binary_verified_status}"
    )


def blocker_field(entry: PlanEntry, *, lane: str = FUNCTIONAL_LANE) -> str | None:
    lane = normalize_lane(lane)
    if entry.reconstructed_status in {NOT_DONE_STATUS, UNKNOWN_STATUS, "?"}:
        return "recon"
    if entry.source_dependencies_status in {NOT_DONE_STATUS, UNKNOWN_STATUS, "?"}:
        return "deps"
    if entry.reimplemented_status != DONE_STATUS:
        return "impl"
    if entry.functional_equivalent_status != DONE_STATUS:
        return "functional"
    if lane == BINARY_LANE and entry.binary_verified_status != DONE_STATUS:
        return "verify"
    if entry.reconstructed_status == LIMITED_STATUS:
        return "recon"
    if entry.source_dependencies_status == LIMITED_STATUS:
        return "deps"
    return None


def first_unfinished(entries: Iterable[PlanEntry], *, lane: str = FUNCTIONAL_LANE) -> PlanEntry | None:
    for entry in entries:
        if blocker_field(entry, lane=lane):
            return entry
    return None
