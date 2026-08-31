from __future__ import annotations

import argparse
from copy import deepcopy
from dataclasses import asdict, dataclass
import json
from pathlib import Path, PurePosixPath
import re
import struct
import sys
import tempfile
from typing import Any, Mapping

from _recoil.commands.asm_verify import (
    CoffObject,
    IMAGE_SCN_CNT_CODE,
    IMAGE_SYM_CLASS_EXTERNAL,
    relocation_size,
    relocation_type_name,
)
from _recoil.commands.progress_v2 import add_live_evidence
from _recoil.lib.pe import PeFormatError, parse_pe_headers, rva_to_offset
from _recoil.lib.progress import (
    ConcurrentProgressUpdate,
    DEFAULT_PROGRESS_PATH,
    ProgressDocument,
    ProgressError,
    ProgressStore,
    address_value,
    normalize_address,
)
from _recoil.lib.tooling import (
    DEFAULT_VC5_ROOT,
    REPO_ROOT,
    configure_stdio,
    display_path,
    quote_cmd_arg,
    run_cmd_script,
)
from _recoil.lib.windows_identity import StableReadHandle, physical_identity


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
DEFAULT_REFERENCE = REPO_ROOT / "support" / "Recoil.exe"
IMAGE_FILE_MACHINE_I386 = 0x14C
IMAGE_SCN_LNK_COMDAT = 0x00001000
OWNER_ID_RE = re.compile(r"^recoil:owner:provider\.[a-z0-9][a-z0-9_.-]*$")
OBJECT_SYMBOL_RE = re.compile(r"^[?@_$A-Za-z][?@_$A-Za-z0-9]*$")
ARCHIVE_MAGIC = b"!<arch>\n"
ARCHIVE_HEADER_SIZE = 60
IMAGE_COMDAT_SELECT_ANY = 2


VECTOR_PLAYER_NODE_RESTORE_PROBE_SOURCE = (
    "#include <vector>\r\n"
    "struct PlayerNodeFlagRestoreEntry\r\n"
    "{\r\n"
    "    void *node;\r\n"
    "    int wasCellPickable;\r\n"
    "    int wasRaycastable;\r\n"
    "    int wasPickable;\r\n"
    "};\r\n"
    "struct RecoilProviderVectorProbe\r\n"
    "    : std::vector<PlayerNodeFlagRestoreEntry>\r\n"
    "{\r\n"
    "    typedef std::vector<PlayerNodeFlagRestoreEntry> Base;\r\n"
    "    typedef void (Base::*FillFn)(\r\n"
    "        iterator, size_type, const PlayerNodeFlagRestoreEntry &);\r\n"
    "    typedef iterator (Base::*CopyFn)(\r\n"
    "        const_iterator, const_iterator, iterator);\r\n"
    "    static FillFn fill;\r\n"
    "    static CopyFn copy;\r\n"
    "};\r\n"
    "RecoilProviderVectorProbe::FillFn RecoilProviderVectorProbe::fill =\r\n"
    "    &RecoilProviderVectorProbe::_Ufill;\r\n"
    "RecoilProviderVectorProbe::CopyFn RecoilProviderVectorProbe::copy =\r\n"
    "    &RecoilProviderVectorProbe::_Ucopy;\r\n"
)


VECTOR_INT_UFILL_PROBE_SOURCE = (
    "#include <vector>\r\n"
    "struct RecoilProviderVectorIntProbe : std::vector<int>\r\n"
    "{\r\n"
    "    typedef std::vector<int> Base;\r\n"
    "    typedef void (Base::*FillFn)(iterator, size_type, const int &);\r\n"
    "    static FillFn fill;\r\n"
    "};\r\n"
    "RecoilProviderVectorIntProbe::FillFn RecoilProviderVectorIntProbe::fill =\r\n"
    "    &RecoilProviderVectorIntProbe::_Ufill;\r\n"
)


HEADER_PROBE_RECIPES: dict[str, dict[str, Any]] = {
    "vc5-xmemory-construct-int-v1": {
        "canonical_header": "VC/INCLUDE/xmemory",
        "semantic_provider": "vc5-stl",
        "object_symbol": "?_Construct@std@@YIXPAHABH@Z",
        "source": (
            "#include <xmemory>\r\n"
            "void recoil_provider_probe(int *dst, const int &value)\r\n"
            "{\r\n"
            "    std::_Construct(dst, value);\r\n"
            "}\r\n"
        ),
        "compile_flags": (
            "/nologo",
            "/c",
            "/TP",
            "/Gy",
            "/O2",
            "/Ob0",
            "/Gr",
            "/Zl",
            "/X",
        ),
        "comdat_selection": IMAGE_COMDAT_SELECT_ANY,
    },
    "vc5-xmemory-construct-int-ob1-v1": {
        "canonical_header": "VC/INCLUDE/xmemory",
        "semantic_provider": "vc5-stl",
        "object_symbol": "?_Construct@std@@YIXPAHABH@Z",
        "source": (
            "#include <xmemory>\r\n"
            "#pragma inline_depth(1)\r\n"
            "inline void recoil_provider_inline(int *dst, const int &value)\r\n"
            "{\r\n"
            "    std::_Construct(dst, value);\r\n"
            "}\r\n"
            "void recoil_provider_probe(int *dst, const int &value)\r\n"
            "{\r\n"
            "    recoil_provider_inline(dst, value);\r\n"
            "}\r\n"
        ),
        "compile_flags": (
            "/nologo",
            "/c",
            "/TP",
            "/Gy",
            "/O2",
            "/Ob1",
            "/Gr",
            "/Zl",
            "/X",
        ),
        "comdat_selection": IMAGE_COMDAT_SELECT_ANY,
    },
    "vc5-vector-player-node-restore-ufill-ob1-v1": {
        "canonical_header": "VC/INCLUDE/vector",
        "semantic_provider": "vc5-stl",
        "object_symbol": "?_Ufill@?$vector@UPlayerNodeFlagRestoreEntry@@V?$allocator@UPlayerNodeFlagRestoreEntry@@@std@@@std@@IAEXPAUPlayerNodeFlagRestoreEntry@@IABU3@@Z",
        "source": VECTOR_PLAYER_NODE_RESTORE_PROBE_SOURCE,
        "compile_flags": (
            "/nologo",
            "/c",
            "/TP",
            "/Gy",
            "/O2",
            "/Ob1",
            "/Gr",
            "/Zl",
            "/X",
        ),
        "comdat_selection": IMAGE_COMDAT_SELECT_ANY,
    },
    "vc5-vector-player-node-restore-ucopy-ob1-v1": {
        "canonical_header": "VC/INCLUDE/vector",
        "semantic_provider": "vc5-stl",
        "object_symbol": "?_Ucopy@?$vector@UPlayerNodeFlagRestoreEntry@@V?$allocator@UPlayerNodeFlagRestoreEntry@@@std@@@std@@IAEPAUPlayerNodeFlagRestoreEntry@@PBU3@0PAU3@@Z",
        "source": VECTOR_PLAYER_NODE_RESTORE_PROBE_SOURCE,
        "compile_flags": (
            "/nologo",
            "/c",
            "/TP",
            "/Gy",
            "/O2",
            "/Ob1",
            "/Gr",
            "/Zl",
            "/X",
        ),
        "comdat_selection": IMAGE_COMDAT_SELECT_ANY,
    },
    "vc5-vector-int-ufill-ob1-v1": {
        "canonical_header": "VC/INCLUDE/vector",
        "semantic_provider": "vc5-stl",
        "object_symbol": "?_Ufill@?$vector@HV?$allocator@H@std@@@std@@IAEXPAHIABH@Z",
        "retail_body_size": 0x30,
        "source": VECTOR_INT_UFILL_PROBE_SOURCE,
        "compile_flags": (
            "/nologo",
            "/c",
            "/TP",
            "/Gy",
            "/O2",
            "/Ob1",
            "/Gr",
            "/Zl",
            "/X",
        ),
        "comdat_selection": IMAGE_COMDAT_SELECT_ANY,
    },
}


class ProviderFunctionMutationError(RuntimeError):
    pass


@dataclass(frozen=True)
class ArchiveMember:
    name: str
    raw_name: str
    data: bytes
    header_offset: int


@dataclass(frozen=True)
class ProviderObjectProof:
    proof_mode: str
    library_path: str
    archive_member: str
    archive_member_raw: str
    object_symbol: str
    section_name: str
    section_index: int
    section_size: int
    body_size: int
    compared_byte_count: int
    masked_byte_count: int
    relocations: tuple[dict[str, Any], ...]
    comdat_selection: int | None = None
    comdat_selection_name: str | None = None
    canonical_header: str | None = None
    probe_recipe: str | None = None
    compile_flags: tuple[str, ...] = ()
    semantic_provider: str | None = None
    physical_emitter_state: str | None = None
    retail_icf_winner_status: str | None = None
    retail_icf_logical_symbols: tuple[str, ...] = ()


def _payload(value: str) -> dict[str, Any]:
    try:
        parsed = json.loads(value)
    except json.JSONDecodeError as exc:
        raise ProviderFunctionMutationError(f"invalid --payload-json: {exc}") from exc
    if not isinstance(parsed, dict):
        raise ProviderFunctionMutationError("--payload-json must decode to an object")
    return parsed


def _canonical_relative_path(value: Any, *, field: str) -> str:
    if not isinstance(value, str) or not value:
        raise ProviderFunctionMutationError(f"{field} must be a non-empty string")
    if "\\" in value or "\x00" in value or ":" in value:
        raise ProviderFunctionMutationError(
            f"{field} must use one canonical relative forward-slash path"
        )
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in {"", ".", ".."} for part in path.parts):
        raise ProviderFunctionMutationError(
            f"{field} must be a normalized relative path without traversal"
        )
    normalized = path.as_posix()
    if normalized != value:
        raise ProviderFunctionMutationError(f"{field} is not canonically normalized")
    return normalized


def normalize_provider_function_request(value: Mapping[str, Any]) -> dict[str, Any]:
    common = {
        "reviewed",
        "proof_mode",
        "object_symbol",
        "owner_id",
        "owner_name",
        "reason",
    }
    mode = value.get("proof_mode", "archive-member")
    if mode not in {"archive-member", "canonical-header-comdat"}:
        raise ProviderFunctionMutationError(
            "proof_mode must be 'archive-member' or 'canonical-header-comdat'"
        )
    mode_fields = (
        {"library_path", "archive_member"}
        if mode == "archive-member"
        else {
            "canonical_header",
            "probe_recipe",
            "semantic_provider",
            "physical_emitter_state",
            "retail_icf_winner_status",
            "retail_icf_logical_symbols",
        }
    )
    allowed = common | mode_fields
    keys = {str(key) for key in value}
    candidate_fields = sorted(key for key in keys if "candidate" in key.casefold())
    if candidate_fields:
        raise ProviderFunctionMutationError(
            f"candidate-derived provider function fields are forbidden: {candidate_fields}"
        )
    unknown = sorted(keys - allowed)
    if unknown:
        raise ProviderFunctionMutationError(
            f"reviewed provider function payload has unsupported fields: {unknown}"
        )
    if value.get("reviewed") is not True:
        raise ProviderFunctionMutationError(
            "reviewed provider function must set reviewed=true"
        )
    result: dict[str, Any] = {"reviewed": True, "proof_mode": mode}
    if mode == "archive-member":
        result.update(
            {
                "library_path": _canonical_relative_path(
                    value.get("library_path"), field="library_path"
                ),
                "archive_member": _canonical_relative_path(
                    value.get("archive_member"), field="archive_member"
                ),
            }
        )
        if not result["library_path"].casefold().endswith(".lib"):
            raise ProviderFunctionMutationError(
                "library_path must name one VC5 .LIB archive"
            )
        if not result["archive_member"].casefold().endswith(".obj"):
            raise ProviderFunctionMutationError(
                "archive_member must name one exact COFF .obj member"
            )
    for field in ("object_symbol", "owner_id", "owner_name", "reason"):
        item = value.get(field)
        if not isinstance(item, str) or not item.strip():
            raise ProviderFunctionMutationError(f"{field} must be a non-empty string")
        if item != item.strip():
            raise ProviderFunctionMutationError(f"{field} must not have outer whitespace")
        result[field] = item
    if OBJECT_SYMBOL_RE.fullmatch(result["object_symbol"]) is None:
        raise ProviderFunctionMutationError(
            "object_symbol must be one exact decorated external COFF symbol"
        )
    if result["object_symbol"].startswith("__imp_"):
        raise ProviderFunctionMutationError(
            "provider-function registration rejects imported-address symbols"
        )
    if OWNER_ID_RE.fullmatch(result["owner_id"]) is None:
        raise ProviderFunctionMutationError(
            "owner_id must be a canonical recoil:owner:provider.* id"
        )
    if mode == "canonical-header-comdat":
        recipe_id = value.get("probe_recipe")
        if not isinstance(recipe_id, str) or recipe_id not in HEADER_PROBE_RECIPES:
            raise ProviderFunctionMutationError(
                "probe_recipe must name one registered canonical-header recipe"
            )
        recipe = HEADER_PROBE_RECIPES[recipe_id]
        canonical_header = _canonical_relative_path(
            value.get("canonical_header"), field="canonical_header"
        )
        if canonical_header != recipe["canonical_header"]:
            raise ProviderFunctionMutationError(
                "canonical_header does not match the registered probe recipe"
            )
        semantic_provider = value.get("semantic_provider")
        if semantic_provider != recipe["semantic_provider"]:
            raise ProviderFunctionMutationError(
                "semantic_provider does not match the registered probe recipe"
            )
        if result["object_symbol"] != recipe["object_symbol"]:
            raise ProviderFunctionMutationError(
                "object_symbol does not match the registered probe recipe"
            )
        if value.get("physical_emitter_state") != "winner-unknown":
            raise ProviderFunctionMutationError(
                "canonical-header physical_emitter_state must be 'winner-unknown'"
            )
        if value.get("retail_icf_winner_status") != "winner-unknown":
            raise ProviderFunctionMutationError(
                "canonical-header retail_icf_winner_status must be 'winner-unknown'"
            )
        logical_symbols = value.get("retail_icf_logical_symbols")
        if (
            not isinstance(logical_symbols, list)
            or not logical_symbols
            or any(
                not isinstance(item, str) or OBJECT_SYMBOL_RE.fullmatch(item) is None
                for item in logical_symbols
            )
            or len(set(logical_symbols)) != len(logical_symbols)
            or result["object_symbol"] not in logical_symbols
        ):
            raise ProviderFunctionMutationError(
                "retail_icf_logical_symbols must be a unique non-empty decorated-symbol "
                "list containing object_symbol"
            )
        result.update(
            {
                "canonical_header": canonical_header,
                "probe_recipe": recipe_id,
                "semantic_provider": semantic_provider,
                "physical_emitter_state": "winner-unknown",
                "retail_icf_winner_status": "winner-unknown",
                "retail_icf_logical_symbols": list(logical_symbols),
            }
        )
    return result


def _resolve_library(root: Path, relative: str) -> Path:
    try:
        root_resolved = root.resolve(strict=True)
        library = (root_resolved / Path(*PurePosixPath(relative).parts)).resolve(
            strict=True
        )
        library.relative_to(root_resolved)
    except (OSError, ValueError) as exc:
        raise ProviderFunctionMutationError(
            "library_path does not resolve to one file under DEFAULT_VC5_ROOT"
        ) from exc
    if not library.is_file():
        raise ProviderFunctionMutationError("library_path is not a regular file")
    return library


def _decode_archive_header_name(raw: bytes) -> str:
    try:
        return raw.decode("ascii")
    except UnicodeDecodeError as exc:
        raise ProviderFunctionMutationError(
            "COFF archive member header name is not ASCII"
        ) from exc


def _resolve_long_member_name(table: bytes, offset: int) -> str:
    if offset < 0 or offset >= len(table):
        raise ProviderFunctionMutationError(
            "COFF archive long-name offset is outside the unique // table"
        )
    nul = table.find(b"\x00", offset)
    slash_newline = table.find(b"/\n", offset)
    ends = [end for end in (nul, slash_newline) if end >= 0]
    if not ends:
        raise ProviderFunctionMutationError(
            "COFF archive long-name entry has no supported terminator"
        )
    end = min(ends)
    raw = table[offset:end]
    if not raw:
        raise ProviderFunctionMutationError("COFF archive long-name entry is empty")
    try:
        name = raw.decode("ascii")
    except UnicodeDecodeError as exc:
        raise ProviderFunctionMutationError(
            "COFF archive long-name entry is not ASCII"
        ) from exc
    return name.replace("\\", "/")


def parse_archive_members(data: bytes) -> tuple[ArchiveMember, ...]:
    if not data.startswith(ARCHIVE_MAGIC):
        raise ProviderFunctionMutationError("malformed COFF archive magic")
    raw_members: list[tuple[str, str, bytes, int]] = []
    offset = len(ARCHIVE_MAGIC)
    long_tables: list[bytes] = []
    while offset < len(data):
        if offset + ARCHIVE_HEADER_SIZE > len(data):
            raise ProviderFunctionMutationError("truncated COFF archive member header")
        header = data[offset : offset + ARCHIVE_HEADER_SIZE]
        if header[58:60] != b"`\n":
            raise ProviderFunctionMutationError("malformed COFF archive member terminator")
        raw_header_name = _decode_archive_header_name(header[:16]).rstrip()
        try:
            size_text = header[48:58].decode("ascii").strip()
            if not size_text or not size_text.isdecimal():
                raise ValueError
            size = int(size_text, 10)
        except (UnicodeDecodeError, ValueError) as exc:
            raise ProviderFunctionMutationError(
                "malformed COFF archive member size"
            ) from exc
        data_offset = offset + ARCHIVE_HEADER_SIZE
        data_end = data_offset + size
        if data_end > len(data):
            raise ProviderFunctionMutationError("truncated COFF archive member payload")
        payload = data[data_offset:data_end]
        if raw_header_name == "//":
            long_tables.append(payload)
        raw_members.append((raw_header_name, raw_header_name, payload, offset))
        offset = data_end
        if size & 1:
            if offset >= len(data) or data[offset : offset + 1] != b"\n":
                raise ProviderFunctionMutationError(
                    "malformed COFF archive odd-member padding"
                )
            offset += 1
    if offset != len(data):
        raise ProviderFunctionMutationError("COFF archive has trailing malformed bytes")
    if len(long_tables) > 1:
        raise ProviderFunctionMutationError("COFF archive has ambiguous // name tables")
    long_table = long_tables[0] if long_tables else None
    result: list[ArchiveMember] = []
    for raw_name, _display, payload, header_offset in raw_members:
        if raw_name in {"/", "//"}:
            continue
        if raw_name.startswith("/") and raw_name[1:].isdecimal():
            if long_table is None:
                raise ProviderFunctionMutationError(
                    "COFF archive long-name reference has no // table"
                )
            name = _resolve_long_member_name(long_table, int(raw_name[1:], 10))
        elif raw_name.endswith("/"):
            name = raw_name[:-1]
        else:
            raise ProviderFunctionMutationError(
                f"unsupported or ambiguous COFF archive member name {raw_name!r}"
            )
        if not name or "\x00" in name:
            raise ProviderFunctionMutationError("invalid COFF archive member name")
        result.append(
            ArchiveMember(
                name=name.replace("\\", "/"),
                raw_name=raw_name,
                data=payload,
                header_offset=header_offset,
            )
        )
    return tuple(result)


def _known_range(value: Any) -> tuple[int, int] | None:
    if not isinstance(value, Mapping):
        return None
    raw_start = value.get("address", value.get("start"))
    raw_end = value.get("end_exclusive")
    if not isinstance(raw_start, (str, int)) or not isinstance(raw_end, (str, int)):
        return None
    try:
        start = address_value(raw_start)
        end = address_value(raw_end)
    except (TypeError, ValueError):
        return None
    return (start, end) if end > start else None


def _validate_existing_function(
    document: ProgressDocument,
    *,
    function_id: str,
    address: str,
) -> tuple[dict[str, Any], int, int]:
    source = document.collection("symbols").get(function_id)
    if not isinstance(source, Mapping):
        raise ProviderFunctionMutationError(
            "provider-function registration requires one existing exact function row"
        )
    row = deepcopy(dict(source))
    required = {
        "binary": "recoil",
        "kind": "function",
        "address": address,
        "extent_state": "known",
        "pipeline_class": "non-authored",
        "authored_order_role": "non-authored",
        "ownership_state": "unresolved",
        "output_section_id": "recoil:section:.text",
    }
    mismatches = {
        key: row.get(key)
        for key, expected in required.items()
        if row.get(key) != expected
    }
    if mismatches:
        raise ProviderFunctionMutationError(
            "existing row is not an exact known-extent unresolved non-authored "
            f"Recoil function: {mismatches}"
        )
    if row.get("disposition") not in {"unresolved", "non-authored"}:
        raise ProviderFunctionMutationError(
            "existing function disposition is authored, provider, or otherwise owned"
        )
    forbidden_fields = (
        "import_dll",
        "import_name",
        "import_ordinal",
        "object_symbol",
        "provider_object_identity",
        "relocation_target_binding",
        "logical_aliases",
    )
    present = [field for field in forbidden_fields if row.get(field) not in {None, ""}]
    if present:
        raise ProviderFunctionMutationError(
            f"existing function already has conflicting identity fields: {present}"
        )
    end_raw = row.get("end_exclusive")
    size = row.get("size")
    if not isinstance(end_raw, (str, int)) or isinstance(size, bool) or not isinstance(size, int):
        raise ProviderFunctionMutationError("existing function extent is incomplete")
    start = address_value(address)
    end = address_value(end_raw)
    if end <= start or size != end - start:
        raise ProviderFunctionMutationError("existing function extent and size disagree")
    return row, start, end


def _validate_tracker_ownership(
    document: ProgressDocument,
    *,
    owner_id: str,
    function_id: str,
    start: int,
    end: int,
) -> None:
    if owner_id in document.collection("owners"):
        raise ProviderFunctionMutationError(f"provider owner already exists: {owner_id}")
    conflicts: list[str] = []
    for symbol_id, symbol in document.collection("symbols").items():
        if symbol_id == function_id or not isinstance(symbol, Mapping):
            continue
        extent = _known_range(symbol)
        if extent is not None and start < extent[1] and extent[0] < end:
            conflicts.append(f"symbol:{symbol_id}")
            continue
        raw_address = symbol.get("address", symbol.get("start"))
        if isinstance(raw_address, (str, int)):
            try:
                symbol_address = address_value(raw_address)
            except (TypeError, ValueError):
                pass
            else:
                if start <= symbol_address < end:
                    conflicts.append(f"symbol:{symbol_id}")
    for contribution_id, contribution in document.collection(
        "storage_contributions"
    ).items():
        if not isinstance(contribution, Mapping):
            continue
        extent = _known_range(contribution.get("reference"))
        if extent is not None and start < extent[1] and extent[0] < end:
            conflicts.append(f"storage:{contribution_id}")
            continue
        reference = contribution.get("reference")
        if isinstance(reference, Mapping):
            raw_address = reference.get("address", reference.get("start"))
            if isinstance(raw_address, (str, int)):
                try:
                    contribution_address = address_value(raw_address)
                except (TypeError, ValueError):
                    pass
                else:
                    if start <= contribution_address < end:
                        conflicts.append(f"storage:{contribution_id}")
    for existing_owner_id, owner in document.collection("owners").items():
        if not isinstance(owner, Mapping):
            continue
        address_metadata = owner.get("address_metadata")
        if isinstance(address_metadata, Mapping):
            for raw_address in address_metadata:
                try:
                    metadata_address = address_value(raw_address)
                except (TypeError, ValueError):
                    continue
                if start <= metadata_address < end:
                    conflicts.append(f"owner:{existing_owner_id}:metadata")
        for relationship in owner.get("relationships", ()):
            if not isinstance(relationship, Mapping):
                continue
            if relationship.get("symbol_id") == function_id:
                conflicts.append(f"owner:{existing_owner_id}:symbol")
            raw_address = relationship.get("address")
            if isinstance(raw_address, (str, int)):
                try:
                    relationship_address = address_value(raw_address)
                except (TypeError, ValueError):
                    continue
                if start <= relationship_address < end:
                    conflicts.append(f"owner:{existing_owner_id}:address")
    if conflicts:
        raise ProviderFunctionMutationError(
            "function extent is overlapping or already owned: "
            + str(sorted(set(conflicts)))
        )


def _retail_body(
    document: ProgressDocument,
    *,
    reference: Path,
    start: int,
    end: int,
) -> tuple[bytes, str]:
    try:
        with StableReadHandle(reference) as stable_reference:
            image = stable_reference.read()
        headers = parse_pe_headers(image, source=str(reference))
    except (OSError, PeFormatError, ValueError) as exc:
        raise ProviderFunctionMutationError(
            f"cannot parse immutable retail PE: {exc}"
        ) from exc
    if headers.machine != IMAGE_FILE_MACHINE_I386:
        raise ProviderFunctionMutationError("immutable retail PE is not i386")
    matches = []
    for section in headers.sections:
        section_start = headers.image_base + section.virtual_address
        section_end = section_start + section.raw_size
        if section_start <= start and end <= section_end:
            matches.append(section)
    if len(matches) != 1 or matches[0].name != ".text":
        raise ProviderFunctionMutationError(
            "function extent must resolve exactly to file-backed retail .text"
        )
    section = matches[0]
    section_id = "recoil:section:.text"
    section_row = document.collection("output_sections").get(section_id)
    if (
        not isinstance(section_row, Mapping)
        or section_row.get("binary") != "recoil"
        or section_row.get("name") != ".text"
    ):
        raise ProviderFunctionMutationError(
            "retail .text output section is not exactly registered"
        )
    rva = start - headers.image_base
    offset = rva_to_offset(rva, headers.sections)
    size = end - start
    if offset is None or offset + size > len(image):
        raise ProviderFunctionMutationError("retail function bytes are not file-backed")
    return image[offset : offset + size], section_id


def _coff_provider_object_proof(
    *,
    object_data: bytes,
    proof_mode: str,
    library_path: str,
    archive_member: str,
    archive_member_raw: str,
    object_symbol: str,
    body_size: int,
    retail_body: bytes,
    expected_comdat_selection: int | None = None,
    canonical_header: str | None = None,
    probe_recipe: str | None = None,
    compile_flags: tuple[str, ...] = (),
    semantic_provider: str | None = None,
    physical_emitter_state: str | None = None,
    retail_icf_winner_status: str | None = None,
    retail_icf_logical_symbols: tuple[str, ...] = (),
    allow_trailing_nop_padding: bool = False,
) -> ProviderObjectProof:
    if len(object_data) < 20:
        raise ProviderFunctionMutationError(
            "provider proof object is truncated before the COFF file header"
        )
    machine, section_count = struct.unpack_from("<HH", object_data, 0)
    optional_header_size = struct.unpack_from("<H", object_data, 16)[0]
    if machine != IMAGE_FILE_MACHINE_I386:
        raise ProviderFunctionMutationError(
            "provider proof COFF machine is not i386"
        )
    if section_count == 0 or optional_header_size != 0:
        raise ProviderFunctionMutationError(
            "provider proof is not one ordinary relocatable COFF object"
        )
    try:
        coff = CoffObject.from_bytes(object_data)
    except ValueError as exc:
        raise ProviderFunctionMutationError(
            f"provider proof is not a valid COFF object: {exc}"
        ) from exc
    exact_symbols = [symbol for symbol in coff.symbols if symbol.name == object_symbol]
    if len(exact_symbols) != 1:
        nearby = sorted(
            symbol.name
            for symbol in coff.symbols
            if "construct" in symbol.name.casefold()
        )
        raise ProviderFunctionMutationError(
            "object_symbol does not resolve to exactly one COFF symbol; "
            f"nearby symbols={nearby}"
        )
    symbol = exact_symbols[0]
    if (
        symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or symbol.type != 0x20
        or symbol.section_number <= 0
    ):
        raise ProviderFunctionMutationError(
            "object_symbol must be one defined external function symbol"
        )
    section = coff.section(symbol.section_number)
    if (
        section.characteristics & IMAGE_SCN_CNT_CODE == 0
        or section.characteristics & IMAGE_SCN_LNK_COMDAT == 0
    ):
        raise ProviderFunctionMutationError(
            "object_symbol must reside in one code COMDAT section"
        )
    section_definitions = [
        item
        for item in coff.symbols
        if item.section_number == section.index
        and item.section_definition_selection is not None
    ]
    if len(section_definitions) != 1:
        raise ProviderFunctionMutationError(
            "object_symbol COMDAT section must have one exact section definition"
        )
    comdat_selection = section_definitions[0].section_definition_selection
    if (
        expected_comdat_selection is not None
        and comdat_selection != expected_comdat_selection
    ):
        raise ProviderFunctionMutationError(
            "object_symbol COMDAT selection does not match the registered probe recipe"
        )
    try:
        body = coff.function_bytes(object_symbol, byte_length=body_size)
    except ValueError as exc:
        raise ProviderFunctionMutationError(
            f"COFF function extent or relocation mismatch: {exc}"
        ) from exc
    exact_extent = body.start == symbol.value and body.end - body.start == body_size
    trailing = section.raw_data[body.end : body.natural_end]
    accepted_padding = (
        allow_trailing_nop_padding
        and body.excluded_tail_relocation_count == 0
        and all(value == 0x90 for value in trailing)
    )
    if not exact_extent or (
        body.natural_end != body.end and not accepted_padding
    ) or body.excluded_tail_relocation_count != 0:
        section_symbols = sorted(
            (item.name, item.value, item.storage_class, item.type)
            for item in coff.symbols
            if item.section_number == symbol.section_number
        )
        section_relocations = [
            (item.offset, item.type, item.symbol_name)
            for item in coff.relocations_by_section.get(section.index, ())
        ]
        raise ProviderFunctionMutationError(
            "COFF symbol natural extent does not exactly match retail function extent: "
            f"start={body.start}, end={body.end}, natural_end={body.natural_end}, "
            f"retail_size={body_size}, excluded_tail_relocations="
            f"{body.excluded_tail_relocation_count}, section_symbols={section_symbols}, "
            f"section_relocations={section_relocations}, tail={section.raw_data[body.end:].hex()}"
        )
    if len(body.data) != len(retail_body):
        raise ProviderFunctionMutationError("COFF and retail function extents differ")
    differences = [
        index
        for index, (expected, actual) in enumerate(zip(body.data, retail_body))
        if not body.relocation_mask[index] and expected != actual
    ]
    if differences:
        first = differences[0]
        raise ProviderFunctionMutationError(
            "immutable retail bytes differ outside supported COFF relocation fields "
            f"at body offset 0x{first:x}"
        )
    relocation_rows: list[dict[str, Any]] = []
    masked_offsets: set[int] = set()
    for relocation in body.relocations:
        try:
            width = relocation_size(relocation.type)
        except ValueError as exc:
            raise ProviderFunctionMutationError(
                f"unsupported COFF relocation in provider function: {exc}"
            ) from exc
        relative_offset = relocation.offset - body.start
        if relative_offset < 0 or relative_offset + width > body_size:
            raise ProviderFunctionMutationError(
                "COFF relocation crosses the exact function extent"
            )
        relocation_offsets = set(range(relative_offset, relative_offset + width))
        if masked_offsets & relocation_offsets:
            raise ProviderFunctionMutationError(
                "COFF relocations overlap inside the exact function extent"
            )
        if relocation.symbol_name.startswith("<symbol "):
            raise ProviderFunctionMutationError(
                "COFF relocation target symbol index is unresolved"
            )
        masked_offsets.update(relocation_offsets)
        relocation_rows.append(
            {
                "offset": relative_offset,
                "type": relocation_type_name(relocation.type),
                "type_value": relocation.type,
                "width": width,
                "target_symbol": relocation.symbol_name,
            }
        )
    return ProviderObjectProof(
        proof_mode=proof_mode,
        library_path=library_path,
        archive_member=archive_member,
        archive_member_raw=archive_member_raw,
        object_symbol=object_symbol,
        section_name=body.section_name,
        section_index=body.section_index,
        section_size=len(section.raw_data),
        body_size=body_size,
        compared_byte_count=body_size - sum(body.relocation_mask),
        masked_byte_count=sum(body.relocation_mask),
        relocations=tuple(relocation_rows),
        comdat_selection=comdat_selection,
        comdat_selection_name=(
            "any" if comdat_selection == IMAGE_COMDAT_SELECT_ANY else f"selection-{comdat_selection}"
        ),
        canonical_header=canonical_header,
        probe_recipe=probe_recipe,
        compile_flags=compile_flags,
        semantic_provider=semantic_provider,
        physical_emitter_state=physical_emitter_state,
        retail_icf_winner_status=retail_icf_winner_status,
        retail_icf_logical_symbols=retail_icf_logical_symbols,
    )


def _provider_object_proof(
    *,
    vc5_root: Path,
    library_path: str,
    archive_member: str,
    object_symbol: str,
    body_size: int,
    retail_body: bytes,
) -> ProviderObjectProof:
    library = _resolve_library(vc5_root, library_path)
    try:
        archive = library.read_bytes()
    except OSError as exc:
        raise ProviderFunctionMutationError(f"cannot read VC5 library: {exc}") from exc
    members = parse_archive_members(archive)
    exact = [member for member in members if member.name == archive_member]
    if len(exact) != 1:
        raise ProviderFunctionMutationError(
            "archive_member does not resolve to exactly one archive member"
        )
    member = exact[0]
    return _coff_provider_object_proof(
        object_data=member.data,
        proof_mode="archive-member",
        library_path=library_path,
        archive_member=archive_member,
        archive_member_raw=member.raw_name,
        object_symbol=object_symbol,
        body_size=body_size,
        retail_body=retail_body,
    )


def _provider_header_comdat_proof(
    *,
    vc5_root: Path,
    request: Mapping[str, Any],
    body_size: int,
    retail_body: bytes,
) -> ProviderObjectProof:
    recipe_id = str(request["probe_recipe"])
    recipe = HEADER_PROBE_RECIPES[recipe_id]
    fixed_retail_body_size = recipe.get("retail_body_size")
    if fixed_retail_body_size is not None and (
        body_size != int(fixed_retail_body_size)
        or len(retail_body) != int(fixed_retail_body_size)
    ):
        raise ProviderFunctionMutationError(
            "registered probe recipe requires exact retail function extent"
        )
    header_path = _resolve_library(vc5_root, str(request["canonical_header"]))
    if not header_path.is_file():
        raise ProviderFunctionMutationError(
            "canonical_header is not one exact file below DEFAULT_VC5_ROOT"
        )
    object_data, flags = _compile_header_probe_object(
        vc5_root=vc5_root,
        recipe_id=recipe_id,
    )
    return _coff_provider_object_proof(
        object_data=object_data,
        proof_mode="canonical-header-comdat",
        library_path="",
        archive_member="",
        archive_member_raw="",
        object_symbol=str(request["object_symbol"]),
        body_size=body_size,
        retail_body=retail_body,
        expected_comdat_selection=int(recipe["comdat_selection"]),
        canonical_header=str(request["canonical_header"]),
        probe_recipe=recipe_id,
        compile_flags=flags,
        semantic_provider=str(request["semantic_provider"]),
        physical_emitter_state=str(request["physical_emitter_state"]),
        retail_icf_winner_status=str(request["retail_icf_winner_status"]),
        retail_icf_logical_symbols=tuple(request["retail_icf_logical_symbols"]),
        allow_trailing_nop_padding=True,
    )


def _compile_header_probe_object(
    *,
    vc5_root: Path,
    recipe_id: str,
) -> tuple[bytes, tuple[str, ...]]:
    recipe = HEADER_PROBE_RECIPES[recipe_id]
    env_script = (vc5_root / "vc5sp3-env.cmd").resolve()
    if not env_script.is_file():
        raise ProviderFunctionMutationError(
            "canonical VC5SP3 environment script is unavailable"
        )
    with tempfile.TemporaryDirectory(prefix="recoil-provider-header-") as temporary:
        root = Path(temporary)
        source_path = root / "provider_probe.cpp"
        object_path = root / "provider_probe.obj"
        source_path.write_text(str(recipe["source"]), encoding="ascii", newline="")
        include_root = vc5_root / "VC" / "INCLUDE"
        flags = tuple(str(item) for item in recipe["compile_flags"])
        command = " ".join(
            [
                "call",
                quote_cmd_arg(env_script),
                ">nul",
                "&&",
                "cl.exe",
                *flags,
                f"/I{quote_cmd_arg(include_root)}",
                f"/Fo{quote_cmd_arg(object_path)}",
                quote_cmd_arg(source_path),
            ]
        )
        completed = run_cmd_script(
            command,
            cwd=root,
            script_name="_provider_header_probe.cmd",
            capture_output=True,
        )
        if completed.returncode != 0 or not object_path.is_file():
            detail = (completed.stderr.strip() or completed.stdout.strip())[-2000:]
            raise ProviderFunctionMutationError(
                f"canonical VC5SP3 header probe compilation failed: {detail}"
            )
        return object_path.read_bytes(), flags


def register_provider_function(
    *,
    progress: Path,
    reference: Path,
    address: str,
    payload: Mapping[str, Any],
    expected_revision: int,
    apply: bool,
    vc5_root: Path | None = None,
) -> dict[str, Any]:
    request = normalize_provider_function_request(payload)
    normalized_address = normalize_address(address)
    function_id = f"recoil:function:{normalized_address}"
    store = ProgressStore(progress)
    try:
        document = store.load()
    except ProgressError as exc:
        raise ProviderFunctionMutationError(str(exc)) from exc
    if document.revision != expected_revision:
        raise ProviderFunctionMutationError(
            f"revision changed: expected {expected_revision}, found {document.revision}"
        )
    function, start, end = _validate_existing_function(
        document, function_id=function_id, address=normalized_address
    )
    owner_id = str(request["owner_id"])
    _validate_tracker_ownership(
        document,
        owner_id=owner_id,
        function_id=function_id,
        start=start,
        end=end,
    )
    retail_body, output_section_id = _retail_body(
        document, reference=reference, start=start, end=end
    )
    effective_vc5_root = DEFAULT_VC5_ROOT if vc5_root is None else vc5_root
    if request["proof_mode"] == "archive-member":
        proof = _provider_object_proof(
            vc5_root=effective_vc5_root,
            library_path=str(request["library_path"]),
            archive_member=str(request["archive_member"]),
            object_symbol=str(request["object_symbol"]),
            body_size=end - start,
            retail_body=retail_body,
        )
    else:
        proof = _provider_header_comdat_proof(
            vc5_root=effective_vc5_root,
            request=request,
            body_size=end - start,
            retail_body=retail_body,
        )

    proposed = deepcopy(document.data)
    scope_ids = [owner_id, function_id]
    provenance: dict[str, Any] = {
        "reference": display_path(reference),
        "producer": (
            "vc5-coff-library-member-retail-byte-comparison"
            if proof.proof_mode == "archive-member"
            else "vc5-canonical-header-comdat-retail-byte-comparison"
        ),
        "candidate_independent": True,
        "address": normalized_address,
        "end_exclusive": normalize_address(end),
        **asdict(proof),
    }
    try:
        evidence_id = add_live_evidence(
            proposed,
            kind="provider-function-registration",
            summary=(
                (
                    f"Immutable retail .text and canonical VC5 archive member "
                    f"{proof.library_path}:{proof.archive_member}"
                )
                if proof.proof_mode == "archive-member"
                else (
                    f"Immutable retail .text and registered canonical VC5 header probe "
                    f"{proof.canonical_header}:{proof.probe_recipe}"
                )
            )
            + (
                f" prove exact external function {proof.object_symbol} at "
                f"{normalized_address}, exact COMDAT selection, and exact relocation "
                "facts with only supported COFF relocation fields masked."
            ),
            scope_ids=scope_ids,
            provenance=provenance,
        )
    except ProgressError as exc:
        raise ProviderFunctionMutationError(str(exc)) from exc
    owner = {
        "address_metadata": {
            normalized_address: {
                "group": "provider.static-library",
                "name": request["object_symbol"],
                "target": "accepted",
            }
        },
        "binary": "recoil",
        "blocker": "none",
        "evidence_ids": [evidence_id],
        "gates": {
            "boundary": "accepted",
            "byte": "deferred",
            "data": "none",
            "functional": "none",
            "owner_linkage": "accepted",
            "source": "accepted",
        },
        "kind": "provider-boundary",
        "legacy_id": owner_id.split("recoil:owner:", 1)[-1],
        "lifecycle_state": "accepted",
        "name": request["owner_name"],
        "provider_state": "accepted",
        "reimplementation": {"entries": {}},
        "relationships": [
            {"kind": "anchor-address", "address": normalized_address},
            {
                "kind": "primary-function",
                "address": normalized_address,
                "symbol_id": function_id,
            },
        ],
        "section": (
            "provider_platform"
            if proof.proof_mode == "archive-member"
            else "provider.compiler"
        ),
        "source_paths": [],
    }
    evidence_ids = function.get("evidence_ids", [])
    if not isinstance(evidence_ids, list):
        raise ProviderFunctionMutationError(
            "existing function evidence_ids must be a list"
        )
    function.update(
        {
            "disposition": "provider",
            "evidence_ids": [*evidence_ids, evidence_id],
            "kind": "provider-function",
            "object_symbol": request["object_symbol"],
            "output_section_id": output_section_id,
            "ownership_state": "primary-owned",
            "provider_object_identity": (
                {
                    "schema": "recoil-provider-function-object-v1",
                    "library_path": proof.library_path,
                    "archive_member": proof.archive_member,
                    "object_symbol": proof.object_symbol,
                    "section_name": proof.section_name,
                    "body_size": proof.body_size,
                    "evidence_id": evidence_id,
                }
                if proof.proof_mode == "archive-member"
                else {
                    "schema": "recoil-provider-function-object-v2",
                    "proof_mode": "canonical-header-comdat",
                    "canonical_header": proof.canonical_header,
                    "probe_recipe": proof.probe_recipe,
                    "object_symbol": proof.object_symbol,
                    "section_name": proof.section_name,
                    "body_size": proof.body_size,
                    "comdat_selection": proof.comdat_selection,
                    "semantic_provider": proof.semantic_provider,
                    "physical_emitter": {
                        "state": proof.physical_emitter_state,
                    },
                    "retail_icf": {
                        "winner_status": proof.retail_icf_winner_status,
                        "logical_symbols": list(proof.retail_icf_logical_symbols),
                    },
                    "evidence_id": evidence_id,
                }
            ),
        }
    )
    if proof.proof_mode == "canonical-header-comdat":
        function["source_traceability"] = {
            "state": "not-applicable",
            "reason_code": "provider-boundary",
        }
    proposed["owners"][owner_id] = owner
    proposed["symbols"][function_id] = function
    try:
        commit = store.commit(
            proposed, expected_revision=expected_revision, apply=apply
        )
    except (ConcurrentProgressUpdate, ProgressError) as exc:
        raise ProviderFunctionMutationError(str(exc)) from exc
    return {
        "report_version": 1,
        "kind": "provider-function-registration-mutation",
        "validation_mode": (
            "immutable-retail-plus-canonical-vc5-archive-member"
            if proof.proof_mode == "archive-member"
            else "immutable-retail-plus-canonical-vc5-header-comdat-probe"
        ),
        "candidate_independent": True,
        "reference": display_path(reference),
        "request": request,
        "provider_object_proof": asdict(proof),
        "entity_ids": {
            "owner_id": owner_id,
            "function_symbol_id": function_id,
            "evidence_id": evidence_id,
        },
        "records": {
            "owner": owner,
            "function": function,
            "evidence": proposed["evidence"][evidence_id],
        },
        "commit": commit.to_dict(),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Register one existing exact non-authored retail function as a canonical "
            "VC5 static-library or registered canonical-header COMDAT provider function."
        )
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    child = subparsers.add_parser("register")
    child.add_argument("--address", required=True)
    child.add_argument("--payload-json", required=True)
    child.add_argument("--progress", type=Path, default=DEFAULT_TRACKER)
    child.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    child.add_argument("--expected-revision", type=int, required=True)
    mode = child.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    child.add_argument("--json", action="store_true")
    return parser


def run(args: argparse.Namespace) -> dict[str, Any]:
    if args.command == "register":
        return register_provider_function(
            progress=args.progress,
            reference=args.reference,
            address=args.address,
            payload=_payload(args.payload_json),
            expected_revision=args.expected_revision,
            apply=bool(args.apply),
        )
    raise ProviderFunctionMutationError(f"unsupported operation {args.command!r}")


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        report = run(args)
    except (OSError, ValueError, ProviderFunctionMutationError) as exc:
        print(f"provider function mutation error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        mode = "APPLIED" if report["commit"]["applied"] else "DRY-RUN"
        proof = report["provider_object_proof"]
        proof_source = (
            f"{proof['library_path']}:{proof['archive_member']}"
            if proof["proof_mode"] == "archive-member"
            else f"{proof['canonical_header']}:{proof['probe_recipe']}"
        )
        print(
            f"Provider function {mode}: {args.address} "
            f"{proof_source}:{proof['object_symbol']}"
        )
        print(
            f"revision {report['commit']['previous_revision']} -> "
            f"{report['commit']['revision']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
