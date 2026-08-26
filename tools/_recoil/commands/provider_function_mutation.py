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


ATLIMPL_CLUSTER_RECIPE_ID = "vc5sp3-atlimpl-provider-cluster-v1"
ATLIMPL_CANONICAL_SOURCE = "VC/ATL/INCLUDE/ATLIMPL.CPP"
ATLIMPL_CLUSTER_OWNER_ID = "recoil:owner:provider.vc5.atlimpl"
ATLIMPL_CLUSTER_OWNER_NAME = "VC5SP3 ATLIMPL canonical provider cluster"
ATLIMPL_CLUSTER_LEGACY_OWNER_ID = (
    "recoil:owner:legacy.provider_platform.namespace_zcom"
)
ATLIMPL_CLUSTER_COMPILE_FLAGS = (
    "/nologo",
    "/c",
    "/TP",
    "/MD",
    "/G5",
    "/O2",
    "/Ob1",
    "/GX",
    "/Zp4",
    "/FAcs",
)
ATLIMPL_CLUSTER_PROBE_SOURCE = (
    "#include <atlbase.h>\r\n"
    "extern CComModule _Module;\r\n"
    "#include <atlcom.h>\r\n"
    "#ifndef ATL_NO_NAMESPACE\r\n"
    "namespace ATL\r\n"
    "{\r\n"
    "#endif\r\n"
    "ATLAPI AtlInternalQueryInterface(void* pThis,\r\n"
    "\tconst _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)\r\n"
    "{\r\n"
    "\t_ASSERTE(pThis != NULL);\r\n"
    "\t_ASSERTE(pEntries->pFunc == _ATL_SIMPLEMAPENTRY);\r\n"
    "\tif (ppvObject == NULL)\r\n"
    "\t\treturn E_POINTER;\r\n"
    "\t*ppvObject = NULL;\r\n"
    "\tif (InlineIsEqualUnknown(iid))\r\n"
    "\t{\r\n"
    "\t\tIUnknown* pUnk = (IUnknown*)((int)pThis+pEntries->dw);\r\n"
    "\t\tpUnk->AddRef();\r\n"
    "\t\t*ppvObject = pUnk;\r\n"
    "\t\treturn S_OK;\r\n"
    "\t}\r\n"
    "\twhile (pEntries->pFunc != NULL)\r\n"
    "\t{\r\n"
    "\t\tBOOL bBlind = (pEntries->piid == NULL);\r\n"
    "\t\tif (bBlind || InlineIsEqualGUID(*(pEntries->piid), iid))\r\n"
    "\t\t{\r\n"
    "\t\t\tif (pEntries->pFunc == _ATL_SIMPLEMAPENTRY)\r\n"
    "\t\t\t{\r\n"
    "\t\t\t\t_ASSERTE(!bBlind);\r\n"
    "\t\t\t\tIUnknown* pUnk = (IUnknown*)((int)pThis+pEntries->dw);\r\n"
    "\t\t\t\tpUnk->AddRef();\r\n"
    "\t\t\t\t*ppvObject = pUnk;\r\n"
    "\t\t\t\treturn S_OK;\r\n"
    "\t\t\t}\r\n"
    "\t\t\telse\r\n"
    "\t\t\t{\r\n"
    "\t\t\t\tHRESULT hRes = pEntries->pFunc(pThis,\r\n"
    "\t\t\t\t\tiid, ppvObject, pEntries->dw);\r\n"
    "\t\t\t\tif (hRes == S_OK || (!bBlind && FAILED(hRes)))\r\n"
    "\t\t\t\t\treturn hRes;\r\n"
    "\t\t\t}\r\n"
    "\t\t}\r\n"
    "\t\tpEntries++;\r\n"
    "\t}\r\n"
    "\treturn E_NOINTERFACE;\r\n"
    "}\r\n"
    "ATLAPI AtlAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw)\r\n"
    "{\r\n"
    "\tCComPtr<IConnectionPointContainer> pCPC;\r\n"
    "\tCComPtr<IConnectionPoint> pCP;\r\n"
    "\tHRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);\r\n"
    "\tif (SUCCEEDED(hRes))\r\n"
    "\t\thRes = pCPC->FindConnectionPoint(iid, &pCP);\r\n"
    "\tif (SUCCEEDED(hRes))\r\n"
    "\t\thRes = pCP->Advise(pUnk, pdw);\r\n"
    "\treturn hRes;\r\n"
    "}\r\n"
    "ATLAPI AtlUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw)\r\n"
    "{\r\n"
    "\tCComPtr<IConnectionPointContainer> pCPC;\r\n"
    "\tCComPtr<IConnectionPoint> pCP;\r\n"
    "\tHRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);\r\n"
    "\tif (SUCCEEDED(hRes))\r\n"
    "\t\thRes = pCPC->FindConnectionPoint(iid, &pCP);\r\n"
    "\tif (SUCCEEDED(hRes))\r\n"
    "\t\thRes = pCP->Unadvise(dw);\r\n"
    "\treturn hRes;\r\n"
    "}\r\n"
    "#ifndef ATL_NO_NAMESPACE\r\n"
    "}\r\n"
    "#endif\r\n"
)
ATLIMPL_CLUSTER_MEMBERS: tuple[dict[str, Any], ...] = (
    {
        "address": "0x42db50",
        "end_exclusive": "0x42dc30",
        "size": 224,
        "symbol_id": "recoil:function:0x42db50",
        "canonical_function": "ATL::AtlInternalQueryInterface",
        "compiled_object_symbol": (
            "?AtlInternalQueryInterface@ATL@@YGJPAXPBU_ATL_INTMAP_ENTRY@1@"
            "ABU_GUID@@PAPAX@Z"
        ),
        "expected_relocations": (),
        "expected_retail_relocation_values": (),
    },
    {
        "address": "0x42dc30",
        "end_exclusive": "0x42dcf0",
        "size": 192,
        "symbol_id": "recoil:function:0x42dc30",
        "canonical_function": "ATL::AtlAdvise",
        "compiled_object_symbol": "?AtlAdvise@ATL@@YGJPAUIUnknown@@0ABU_GUID@@PAK@Z",
        "expected_relocations": (
            (3, "DIR32", 6, 4, "$L34183"),
            (9, "DIR32", 6, 4, "__except_list"),
            (17, "DIR32", 6, 4, "__except_list"),
            (50, "DIR32", 6, 4, "_IID_IConnectionPointContainer"),
            (168, "DIR32", 6, 4, "__except_list"),
        ),
        "expected_retail_relocation_values": (
            (3, "0x4c9cf0"),
            (9, "0x0"),
            (17, "0x0"),
            (50, "0x4d43a0"),
            (168, "0x0"),
        ),
    },
    {
        "address": "0x42dcf0",
        "end_exclusive": "0x42dda0",
        "size": 176,
        "symbol_id": "recoil:function:0x42dcf0",
        "canonical_function": "ATL::AtlUnadvise",
        "compiled_object_symbol": "?AtlUnadvise@ATL@@YGJPAUIUnknown@@ABU_GUID@@K@Z",
        "expected_relocations": (
            (3, "DIR32", 6, 4, "$L34206"),
            (9, "DIR32", 6, 4, "__except_list"),
            (17, "DIR32", 6, 4, "__except_list"),
            (50, "DIR32", 6, 4, "_IID_IConnectionPointContainer"),
            (163, "DIR32", 6, 4, "__except_list"),
        ),
        "expected_retail_relocation_values": (
            (3, "0x4c9d10"),
            (9, "0x0"),
            (17, "0x0"),
            (50, "0x4d43a0"),
            (163, "0x0"),
        ),
    },
)

ATLIMPL_CLUSTER_LEGACY_OWNER_SNAPSHOT: dict[str, Any] = {
    "address_metadata": {
        "0x42db50": {
            "group": "engine.zcom",
            "name": "zCom::QueryInterfaceFromInterfaceMap",
            "source_path": "src/GameZRecoil/zCom/zCom.cpp",
            "target": "zcom_query_interface_from_interface_map",
        },
        "0x42dc30": {
            "group": "engine.zcom",
            "name": "zCom::ConnectionPointContainer_Advise",
            "source_path": "src/GameZRecoil/zCom/zCom.cpp",
            "target": "zcom_m01_helpers",
        },
        "0x42dcf0": {
            "group": "engine.zcom",
            "name": "zCom::ConnectionPointContainer_Unadvise",
            "source_path": "src/GameZRecoil/zCom/zCom.cpp",
            "target": "zcom_m01_helpers",
        },
    },
    "binary": "recoil",
    "blocker": "none",
    "evidence_ids": [
        "recoil:evidence:r725:002983",
        "recoil:evidence:r725:004042",
        "recoil:evidence:r725:007900",
        "recoil:evidence:r725:010348",
        "recoil:evidence:r725:011592",
        "recoil:evidence:r725:011677",
        "recoil:evidence:r725:015600",
        "recoil:evidence:r725:016908",
        "recoil:evidence:r725:019121",
        "recoil:evidence:r725:019895",
        "recoil:evidence:r725:019914",
        "recoil:evidence:r725:020220",
        "recoil:evidence:r725:020889",
    ],
    "gates": {
        "boundary": "accepted",
        "byte": "accepted",
        "data": "none",
        "functional": "accepted",
        "owner_linkage": "accepted",
        "source": "accepted",
    },
    "kind": "subsystem",
    "legacy_id": "legacy.provider_platform.namespace_zcom",
    "lifecycle_state": "accepted",
    "name": "zCom",
    "provider_state": "pending",
    "reimplementation": {
        "entries": {
            "recoil:function:0x42db50": {
                "evidence_ids": ["recoil:evidence:r725:026833"],
                "kind": "function",
                "tier": "S",
            },
            "recoil:function:0x42dc30": {
                "evidence_ids": ["recoil:evidence:r725:002719"],
                "kind": "function",
                "tier": "S",
            },
            "recoil:function:0x42dcf0": {
                "evidence_ids": ["recoil:evidence:r725:018623"],
                "kind": "function",
                "tier": "S",
            },
        }
    },
    "relationships": [
        {"address": "0x42db50", "kind": "anchor-address"},
        {
            "address": "0x42db50",
            "kind": "primary-function",
            "symbol_id": "recoil:function:0x42db50",
        },
        {
            "address": "0x42dc30",
            "kind": "primary-function",
            "symbol_id": "recoil:function:0x42dc30",
        },
        {
            "address": "0x42dcf0",
            "kind": "primary-function",
            "symbol_id": "recoil:function:0x42dcf0",
        },
    ],
    "section": "provider_platform",
    "source_paths": ["src/GameZRecoil/zCom/zCom.cpp"],
}


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


def _atlimpl_cluster_payload_members() -> list[dict[str, str]]:
    return [
        {
            "address": str(member["address"]),
            "symbol_id": str(member["symbol_id"]),
            "canonical_function": str(member["canonical_function"]),
        }
        for member in ATLIMPL_CLUSTER_MEMBERS
    ]


def normalize_atlimpl_cluster_request(value: Mapping[str, Any]) -> dict[str, Any]:
    fields = {
        "schema",
        "reviewed",
        "recipe_id",
        "retired_owner_id",
        "owner_id",
        "owner_name",
        "members",
        "original_translation_unit",
        "retail_coff_symbols",
        "reason",
    }
    keys = {str(key) for key in value}
    candidate_fields = sorted(key for key in keys if "candidate" in key.casefold())
    if candidate_fields:
        raise ProviderFunctionMutationError(
            "candidate-derived ATLIMPL cluster fields are forbidden: "
            f"{candidate_fields}"
        )
    missing = sorted(fields - keys)
    unknown = sorted(keys - fields)
    if missing or unknown:
        raise ProviderFunctionMutationError(
            "reviewed ATLIMPL cluster payload fields differ from the fixed contract: "
            f"missing={missing}, unsupported={unknown}"
        )
    expected_scalars = {
        "schema": "recoil-vc5-atlimpl-provider-cluster-v1",
        "reviewed": True,
        "recipe_id": ATLIMPL_CLUSTER_RECIPE_ID,
        "retired_owner_id": ATLIMPL_CLUSTER_LEGACY_OWNER_ID,
        "owner_id": ATLIMPL_CLUSTER_OWNER_ID,
        "owner_name": ATLIMPL_CLUSTER_OWNER_NAME,
        "original_translation_unit": None,
        "retail_coff_symbols": None,
    }
    drift = {
        field: value.get(field)
        for field, expected in expected_scalars.items()
        if value.get(field) != expected
    }
    if drift:
        raise ProviderFunctionMutationError(
            "reviewed ATLIMPL cluster payload does not name the fixed recipe, owners, "
            f"or unknown retail identities: {drift}"
        )
    expected_members = _atlimpl_cluster_payload_members()
    if value.get("members") != expected_members:
        raise ProviderFunctionMutationError(
            "ATLIMPL cluster members must be exactly 0x42db50, 0x42dc30, and "
            "0x42dcf0 in natural order; 0x42de00 and every additional or partial "
            "membership are forbidden"
        )
    reason = value.get("reason")
    if not isinstance(reason, str) or not reason.strip() or reason != reason.strip():
        raise ProviderFunctionMutationError(
            "ATLIMPL cluster reason must be a non-empty string without outer whitespace"
        )
    return {
        **expected_scalars,
        "members": expected_members,
        "reason": reason,
    }


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


def _compile_atlimpl_cluster_object(
    *,
    vc5_root: Path,
) -> tuple[bytes, tuple[str, ...]]:
    canonical_source = _resolve_library(vc5_root, ATLIMPL_CANONICAL_SOURCE)
    try:
        source_identity = physical_identity(canonical_source)
    except OSError as exc:
        raise ProviderFunctionMutationError(
            f"canonical VC5SP3 ATLIMPL source is unreadable: {exc}"
        ) from exc
    if source_identity.file_size <= 0:
        raise ProviderFunctionMutationError("canonical VC5SP3 ATLIMPL source is empty")
    env_script = (vc5_root / "vc5sp3-env.cmd").resolve()
    if not env_script.is_file():
        raise ProviderFunctionMutationError(
            "canonical VC5SP3 environment script is unavailable"
        )
    with tempfile.TemporaryDirectory(prefix="recoil-provider-atlimpl-") as temporary:
        root = Path(temporary)
        source_path = root / "provider_atlimpl_probe.cpp"
        object_path = root / "provider_atlimpl_probe.obj"
        listing_path = root / "provider_atlimpl_probe.cod"
        source_path.write_text(ATLIMPL_CLUSTER_PROBE_SOURCE, encoding="ascii", newline="")
        include_root = vc5_root / "VC" / "INCLUDE"
        atl_include_root = vc5_root / "VC" / "ATL" / "INCLUDE"
        flags = ATLIMPL_CLUSTER_COMPILE_FLAGS
        command = " ".join(
            [
                "call",
                quote_cmd_arg(env_script),
                ">nul",
                "&&",
                "cl.exe",
                *flags,
                f"/I{quote_cmd_arg(include_root)}",
                f"/I{quote_cmd_arg(atl_include_root)}",
                f"/Fa{quote_cmd_arg(listing_path)}",
                f"/Fo{quote_cmd_arg(object_path)}",
                quote_cmd_arg(source_path),
            ]
        )
        completed = run_cmd_script(
            command,
            cwd=root,
            script_name="_provider_atlimpl_probe.cmd",
            capture_output=True,
        )
        if completed.returncode != 0 or not object_path.is_file():
            detail = (completed.stderr.strip() or completed.stdout.strip())[-2000:]
            raise ProviderFunctionMutationError(
                f"canonical VC5SP3 ATLIMPL probe compilation failed: {detail}"
            )
        if not listing_path.is_file():
            raise ProviderFunctionMutationError(
                "canonical VC5SP3 ATLIMPL /FAcs listing was not produced"
            )
        return object_path.read_bytes(), flags


def _validate_atlimpl_cluster_tracker(
    document: ProgressDocument,
) -> list[dict[str, Any]]:
    owners = document.collection("owners")
    legacy_owner = owners.get(ATLIMPL_CLUSTER_LEGACY_OWNER_ID)
    if legacy_owner != ATLIMPL_CLUSTER_LEGACY_OWNER_SNAPSHOT:
        raise ProviderFunctionMutationError(
            "legacy zCom owner differs from the complete reviewed r4677 snapshot"
        )
    if ATLIMPL_CLUSTER_OWNER_ID in owners:
        raise ProviderFunctionMutationError(
            f"ATLIMPL provider owner already exists: {ATLIMPL_CLUSTER_OWNER_ID}"
        )

    functions: list[dict[str, Any]] = []
    expected_ids = {str(member["symbol_id"]) for member in ATLIMPL_CLUSTER_MEMBERS}
    cluster_start = address_value(ATLIMPL_CLUSTER_MEMBERS[0]["address"])
    cluster_end = address_value(ATLIMPL_CLUSTER_MEMBERS[-1]["end_exclusive"])
    for member in ATLIMPL_CLUSTER_MEMBERS:
        symbol_id = str(member["symbol_id"])
        source = document.collection("symbols").get(symbol_id)
        if not isinstance(source, Mapping):
            raise ProviderFunctionMutationError(
                f"ATLIMPL cluster member is missing: {symbol_id}"
            )
        function = deepcopy(dict(source))
        required = {
            "address": member["address"],
            "authored_order_role": "non-authored",
            "binary": "recoil",
            "disposition": "unresolved",
            "end_exclusive": member["end_exclusive"],
            "extent_state": "known",
            "kind": "function",
            "navigation_name": {
                "0x42db50": "zCom::QueryInterfaceFromInterfaceMap",
                "0x42dc30": "zCom::ConnectionPointContainer_Advise",
                "0x42dcf0": "zCom::ConnectionPointContainer_Unadvise",
            }[str(member["address"])],
            "output_section_id": "recoil:section:.text",
            "ownership_state": "primary-owned",
            "physical_block_id": "recoil:block:0x41ea90",
            "pipeline_class": "non-authored",
            "semantic_span_ids": ["recoil:semantic:0x42db50-0x42dda0"],
            "size": member["size"],
            "source_traceability": {
                "reason_code": "detached-or-unsupported-source-topology",
                "source_edges": [],
                "state": "unresolved",
            },
            "storage_contribution_ids": [],
        }
        drift = {
            field: function.get(field)
            for field, expected in required.items()
            if function.get(field) != expected
        }
        if drift:
            raise ProviderFunctionMutationError(
                f"post-classification ATLIMPL member snapshot drift for {symbol_id}: "
                f"{drift}"
            )
        forbidden_identity = sorted(
            field
            for field in (
                "import_dll",
                "import_name",
                "import_ordinal",
                "object_symbol",
                "provider_object_identity",
                "relocation_target_binding",
                "logical_aliases",
            )
            if function.get(field) is not None and function.get(field) != ""
        )
        if forbidden_identity:
            raise ProviderFunctionMutationError(
                f"ATLIMPL member {symbol_id} already has conflicting provider, import, "
                f"or retail COFF identity fields: {forbidden_identity}"
            )
        functions.append(function)

    overlap_ids: set[str] = set()
    for symbol_id, raw_symbol in document.collection("symbols").items():
        if not isinstance(raw_symbol, Mapping):
            continue
        extent = _known_range(raw_symbol)
        if extent is not None and cluster_start < extent[1] and extent[0] < cluster_end:
            overlap_ids.add(str(symbol_id))
            continue
        raw_address = raw_symbol.get("address", raw_symbol.get("start"))
        if isinstance(raw_address, (str, int)):
            try:
                symbol_address = address_value(raw_address)
            except (TypeError, ValueError):
                continue
            if cluster_start <= symbol_address < cluster_end:
                overlap_ids.add(str(symbol_id))
    if overlap_ids != expected_ids:
        raise ProviderFunctionMutationError(
            "ATLIMPL physical cluster census differs from exactly the three reviewed "
            f"function rows: {sorted(overlap_ids)}"
        )

    owner_collisions: list[str] = []
    for owner_id, raw_owner in owners.items():
        if owner_id == ATLIMPL_CLUSTER_LEGACY_OWNER_ID or not isinstance(
            raw_owner, Mapping
        ):
            continue
        for relationship in raw_owner.get("relationships", ()):
            if not isinstance(relationship, Mapping):
                continue
            if relationship.get("symbol_id") in expected_ids:
                owner_collisions.append(f"{owner_id}:symbol")
                continue
            raw_address = relationship.get("address")
            if isinstance(raw_address, (str, int)):
                try:
                    relationship_address = address_value(raw_address)
                except (TypeError, ValueError):
                    continue
                if cluster_start <= relationship_address < cluster_end:
                    owner_collisions.append(f"{owner_id}:address")
    if owner_collisions:
        raise ProviderFunctionMutationError(
            "ATLIMPL cluster is referenced by an owner outside the exact legacy "
            f"snapshot: {sorted(owner_collisions)}"
        )
    return functions


def _atlimpl_cluster_proofs(
    document: ProgressDocument,
    *,
    reference: Path,
    vc5_root: Path,
) -> list[dict[str, Any]]:
    object_data, compile_flags = _compile_atlimpl_cluster_object(vc5_root=vc5_root)
    try:
        coff = CoffObject.from_bytes(object_data)
    except ValueError as exc:
        raise ProviderFunctionMutationError(
            f"canonical ATLIMPL proof object is invalid: {exc}"
        ) from exc
    defined_external_functions = sorted(
        symbol.name
        for symbol in coff.symbols
        if symbol.storage_class == IMAGE_SYM_CLASS_EXTERNAL
        and symbol.type == 0x20
        and symbol.section_number > 0
    )
    expected_defined_external_functions = sorted(
        [str(member["compiled_object_symbol"]) for member in ATLIMPL_CLUSTER_MEMBERS]
        + [
            "??1?$CComPtr@UIConnectionPoint@@@ATL@@QAE@XZ",
            "??1?$CComPtr@UIConnectionPointContainer@@@ATL@@QAE@XZ",
        ]
    )
    if defined_external_functions != expected_defined_external_functions:
        raise ProviderFunctionMutationError(
            "canonical ATLIMPL probe external-function census drifted from the fixed "
            f"three members and two template-support emissions: {defined_external_functions}"
        )

    proofs: list[dict[str, Any]] = []
    for member in ATLIMPL_CLUSTER_MEMBERS:
        start = address_value(member["address"])
        end = address_value(member["end_exclusive"])
        retail_body, output_section_id = _retail_body(
            document, reference=reference, start=start, end=end
        )
        proof = _coff_provider_object_proof(
            object_data=object_data,
            proof_mode="canonical-provider-source",
            library_path="",
            archive_member="",
            archive_member_raw="",
            object_symbol=str(member["compiled_object_symbol"]),
            body_size=int(member["size"]),
            retail_body=retail_body,
            expected_comdat_selection=1,
            canonical_header=ATLIMPL_CANONICAL_SOURCE,
            probe_recipe=ATLIMPL_CLUSTER_RECIPE_ID,
            compile_flags=compile_flags,
            semantic_provider="vc5-atl",
        )
        actual_relocations = tuple(
            (
                row["offset"],
                row["type"],
                row["type_value"],
                row["width"],
                row["target_symbol"],
            )
            for row in proof.relocations
        )
        if actual_relocations != member["expected_relocations"]:
            raise ProviderFunctionMutationError(
                f"canonical ATLIMPL relocation rows drifted for {member['address']}: "
                f"{actual_relocations}"
            )
        if (
            proof.section_name != ".text"
            or proof.section_size != member["size"]
            or proof.body_size != member["size"]
            or proof.comdat_selection != 1
        ):
            raise ProviderFunctionMutationError(
                f"canonical ATLIMPL natural extent or COMDAT contract drifted for "
                f"{member['address']}"
            )
        retail_relocation_values = tuple(
            (
                offset,
                normalize_address(struct.unpack_from("<I", retail_body, offset)[0]),
            )
            for offset, _name, _type_value, width, _target in actual_relocations
            if width == 4
        )
        if retail_relocation_values != member["expected_retail_relocation_values"]:
            raise ProviderFunctionMutationError(
                f"immutable retail relocation operands drifted for {member['address']}: "
                f"{retail_relocation_values}"
            )
        proof_record = asdict(proof)
        proof_record.update(
            {
                "address": member["address"],
                "end_exclusive": member["end_exclusive"],
                "function_symbol_id": member["symbol_id"],
                "canonical_function": member["canonical_function"],
                "compiled_object_symbol_is_retail_identity": False,
                "natural_extent_exact": True,
                "output_section_id": output_section_id,
                "retail_relocation_values": retail_relocation_values,
            }
        )
        proofs.append(proof_record)
    return proofs


def register_atlimpl_provider_cluster(
    *,
    progress: Path,
    reference: Path,
    payload: Mapping[str, Any],
    expected_revision: int,
    apply: bool,
    vc5_root: Path | None = None,
) -> dict[str, Any]:
    request = normalize_atlimpl_cluster_request(payload)
    store = ProgressStore(progress)
    try:
        document = store.load()
    except ProgressError as exc:
        raise ProviderFunctionMutationError(str(exc)) from exc
    if document.revision != expected_revision:
        raise ProviderFunctionMutationError(
            f"revision changed: expected {expected_revision}, found {document.revision}"
        )
    functions = _validate_atlimpl_cluster_tracker(document)
    effective_vc5_root = DEFAULT_VC5_ROOT if vc5_root is None else vc5_root
    proofs = _atlimpl_cluster_proofs(
        document,
        reference=reference,
        vc5_root=effective_vc5_root,
    )

    proposed = deepcopy(document.data)
    scope_ids = [
        ATLIMPL_CLUSTER_LEGACY_OWNER_ID,
        ATLIMPL_CLUSTER_OWNER_ID,
        *[str(member["symbol_id"]) for member in ATLIMPL_CLUSTER_MEMBERS],
    ]
    try:
        evidence_id = add_live_evidence(
            proposed,
            kind="provider-function-cluster-registration",
            summary=(
                "Immutable retail .text and the fixed canonical VC5SP3 ATLIMPL "
                "source recipe prove the exact three-function provider cluster, "
                "natural extents, supported relocation rows and targets, and complete "
                "membership without claiming an original translation unit or retail "
                "COFF spelling."
            ),
            scope_ids=scope_ids,
            provenance={
                "reference": display_path(reference),
                "producer": "vc5-canonical-atlimpl-retail-cluster-comparison",
                "candidate_independent": True,
                "recipe_id": ATLIMPL_CLUSTER_RECIPE_ID,
                "canonical_provider_source": ATLIMPL_CANONICAL_SOURCE,
                "canonical_source_physical_identity": physical_identity(
                    _resolve_library(vc5_root, ATLIMPL_CANONICAL_SOURCE)
                ).to_dict(),
                "compile_flags": list(ATLIMPL_CLUSTER_COMPILE_FLAGS),
                "members": proofs,
                "original_translation_unit": None,
                "retail_coff_symbols": None,
            },
        )
    except ProgressError as exc:
        raise ProviderFunctionMutationError(str(exc)) from exc

    owner = {
        "address_metadata": {
            str(member["address"]): {
                "group": "provider.vc5.atlimpl",
                "name": member["canonical_function"],
                "target": "accepted",
            }
            for member in ATLIMPL_CLUSTER_MEMBERS
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
        "legacy_id": ATLIMPL_CLUSTER_OWNER_ID.split("recoil:owner:", 1)[-1],
        "lifecycle_state": "accepted",
        "name": ATLIMPL_CLUSTER_OWNER_NAME,
        "provider_state": "accepted",
        "reimplementation": {"entries": {}},
        "relationships": [
            {"kind": "anchor-address", "address": ATLIMPL_CLUSTER_MEMBERS[0]["address"]},
            *[
                {
                    "kind": "primary-function",
                    "address": member["address"],
                    "symbol_id": member["symbol_id"],
                }
                for member in ATLIMPL_CLUSTER_MEMBERS
            ],
        ],
        "section": "provider.compiler",
        "source_paths": [],
    }
    updated_functions: list[dict[str, Any]] = []
    for function, member in zip(functions, ATLIMPL_CLUSTER_MEMBERS):
        before_classification = {
            "pipeline_class": function["pipeline_class"],
            "authored_order_role": function["authored_order_role"],
        }
        evidence_ids = function.get("evidence_ids", [])
        if not isinstance(evidence_ids, list):
            raise ProviderFunctionMutationError(
                f"ATLIMPL member {member['symbol_id']} evidence_ids must be a list"
            )
        function.update(
            {
                "disposition": "provider",
                "evidence_ids": [*evidence_ids, evidence_id],
                "kind": "provider-function",
                "ownership_state": "primary-owned",
                "source_traceability": {
                    "state": "not-applicable",
                    "reason_code": "provider-boundary",
                },
            }
        )
        after_classification = {
            "pipeline_class": function["pipeline_class"],
            "authored_order_role": function["authored_order_role"],
        }
        if after_classification != before_classification:
            raise ProviderFunctionMutationError(
                "ATLIMPL provider registration may not mutate the independent pipeline "
                "classification dimension"
            )
        proposed["symbols"][str(member["symbol_id"])] = function
        updated_functions.append(function)

    del proposed["owners"][ATLIMPL_CLUSTER_LEGACY_OWNER_ID]
    proposed["owners"][ATLIMPL_CLUSTER_OWNER_ID] = owner
    try:
        commit = store.commit(
            proposed,
            expected_revision=expected_revision,
            apply=apply,
        )
    except (ConcurrentProgressUpdate, ProgressError) as exc:
        raise ProviderFunctionMutationError(str(exc)) from exc
    return {
        "report_version": 1,
        "kind": "provider-function-cluster-registration-mutation",
        "validation_mode": "immutable-retail-plus-fixed-canonical-vc5-atlimpl-source",
        "candidate_independent": True,
        "reference": display_path(reference),
        "request": request,
        "classification_dimension": {
            "mutated": False,
            "required_current_pipeline_class": "non-authored",
            "required_current_authored_order_role": "non-authored",
        },
        "proof_results": proofs,
        "entity_ids": {
            "retired_owner_id": ATLIMPL_CLUSTER_LEGACY_OWNER_ID,
            "owner_id": ATLIMPL_CLUSTER_OWNER_ID,
            "function_symbol_ids": [
                str(member["symbol_id"]) for member in ATLIMPL_CLUSTER_MEMBERS
            ],
            "evidence_id": evidence_id,
        },
        "records": {
            "retired_owner_snapshot": ATLIMPL_CLUSTER_LEGACY_OWNER_SNAPSHOT,
            "owner": owner,
            "functions": updated_functions,
            "evidence": proposed["evidence"][evidence_id],
        },
        "commit": commit.to_dict(),
    }


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
    cluster = subparsers.add_parser("register-atlimpl-cluster")
    cluster.add_argument("--payload-json", required=True)
    cluster.add_argument("--progress", type=Path, default=DEFAULT_TRACKER)
    cluster.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    cluster.add_argument("--expected-revision", type=int, required=True)
    cluster_mode = cluster.add_mutually_exclusive_group(required=True)
    cluster_mode.add_argument("--dry-run", action="store_true")
    cluster_mode.add_argument("--apply", action="store_true")
    cluster.add_argument("--json", action="store_true")
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
    if args.command == "register-atlimpl-cluster":
        return register_atlimpl_provider_cluster(
            progress=args.progress,
            reference=args.reference,
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
        if report["kind"] == "provider-function-cluster-registration-mutation":
            print(
                f"ATLIMPL provider cluster {mode}: "
                "0x42db50, 0x42dc30, 0x42dcf0"
            )
        else:
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
