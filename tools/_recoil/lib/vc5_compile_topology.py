from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
from typing import Any, Iterable

from _recoil.commands.asm_verify import CoffObject, IMAGE_SCN_CNT_CODE


IMAGE_SCN_MEM_EXECUTE = 0x20000000
IMAGE_SCN_LNK_COMDAT = 0x00001000
IMAGE_SYM_DTYPE_FUNCTION = 0x20
COMDAT_SELECTION_NAMES = {
    0: "none",
    1: "noduplicates",
    2: "any",
    3: "same-size",
    4: "exact-match",
    5: "associative",
    6: "largest",
    7: "newest",
}
PCH_FLAG_RE = re.compile(r"^/(?:Yc|Yu|Fp|FI)", re.IGNORECASE)
INCLUDE_TRACE_RE = re.compile(r"including file:\s*(.+?)\s*$", re.IGNORECASE)
PREPROCESS_LINE_RE = re.compile(r'^\s*#(?:line)?\s*\d+\s+"([^"]+)"')


@dataclass(frozen=True)
class SourcePchRole:
    source: Path
    role: str


@dataclass(frozen=True)
class PchTopology:
    header: str
    pch_path: Path
    roles: tuple[SourcePchRole, ...]


@dataclass(frozen=True)
class CanonicalMfc:
    include_root: Path
    lib_root: Path
    required_headers: tuple[str, ...]
    required_libs: tuple[str, ...]
    require_include_trace: bool
    allowed_custom_headers: tuple[Path, ...] = ()


def ensure_inside(path: Path, root: Path, *, label: str) -> None:
    try:
        path.resolve().relative_to(root.resolve())
    except ValueError as exc:
        raise ValueError(f"{label} must stay inside build root {root}: {path}") from exc


def reject_raw_topology_flags(flags: Iterable[str], *, label: str) -> None:
    conflicts = [flag for flag in flags if PCH_FLAG_RE.match(flag)]
    if conflicts:
        raise ValueError(f"{label} contains topology-owned VC5 flags: {', '.join(conflicts)}")


def parse_pch_topology(
    data: object,
    *,
    sources: tuple[Path, ...],
    build_dir: Path,
    resolve_path,
    manifest_path: Path,
) -> PchTopology | None:
    if data is None:
        return None
    if not isinstance(data, dict):
        raise ValueError(f"{manifest_path}: pch must be an object")
    header = data.get("header")
    pch_file = data.get("pch_file")
    raw_roles = data.get("roles")
    if not isinstance(header, str) or not header:
        raise ValueError(f"{manifest_path}: pch.header must be a non-empty string")
    if not isinstance(pch_file, str) or not pch_file:
        raise ValueError(f"{manifest_path}: pch.pch_file must be a non-empty string")
    if not isinstance(raw_roles, list):
        raise ValueError(f"{manifest_path}: pch.roles must be a list")
    pch_path = build_dir / pch_file
    ensure_inside(pch_path, build_dir, label="PCH output")
    source_keys = {str(path.resolve()).lower(): path for path in sources}
    roles: list[SourcePchRole] = []
    seen: set[str] = set()
    for index, item in enumerate(raw_roles):
        if not isinstance(item, dict) or set(item) != {"source", "role"}:
            raise ValueError(f"{manifest_path}: pch.roles[{index}] must contain source and role")
        source_raw = item["source"]
        role = item["role"]
        if not isinstance(source_raw, str) or not isinstance(role, str):
            raise ValueError(f"{manifest_path}: pch.roles[{index}] source/role must be strings")
        source = resolve_path(source_raw)
        key = str(source.resolve()).lower()
        if key not in source_keys:
            raise ValueError(f"{manifest_path}: PCH role references unknown source {source_raw}")
        if key in seen:
            raise ValueError(f"{manifest_path}: duplicate PCH role for {source_raw}")
        if role not in {"create", "use", "none"}:
            raise ValueError(f"{manifest_path}: invalid PCH role {role!r} for {source_raw}")
        seen.add(key)
        roles.append(SourcePchRole(source_keys[key], role))
    missing = [str(source) for source in sources if str(source.resolve()).lower() not in seen]
    if missing:
        raise ValueError(f"{manifest_path}: PCH topology does not cover sources: {', '.join(missing)}")
    creators = [item for item in roles if item.role == "create"]
    consumers = [item for item in roles if item.role == "use"]
    if len(creators) != 1:
        raise ValueError(f"{manifest_path}: PCH topology requires exactly one creator, found {len(creators)}")
    source_positions = {str(source.resolve()).lower(): index for index, source in enumerate(sources)}
    creator_position = source_positions[str(creators[0].source.resolve()).lower()]
    if any(source_positions[str(item.source.resolve()).lower()] <= creator_position for item in consumers):
        raise ValueError(f"{manifest_path}: PCH creator must precede every consumer")
    return PchTopology(header=header, pch_path=pch_path, roles=tuple(roles))


def topology_args(topology: PchTopology | None, source: Path) -> list[str]:
    if topology is None:
        return []
    key = str(source.resolve()).lower()
    role = next(item.role for item in topology.roles if str(item.source.resolve()).lower() == key)
    if role == "none":
        return []
    prefix = "/Yc" if role == "create" else "/Yu"
    return [f'{prefix}"{topology.header}"', f"/Fp{topology.pch_path}"]


def parse_canonical_mfc(
    data: object,
    *,
    resolve_path,
    manifest_path: Path,
    repository_root: Path,
) -> CanonicalMfc | None:
    if data is None:
        return None
    if not isinstance(data, dict):
        raise ValueError(f"{manifest_path}: canonical_mfc must be an object")
    include_root = data.get("include_root")
    lib_root = data.get("lib_root")
    headers = data.get("required_headers", [])
    libs = data.get("required_libs", [])
    trace = data.get("require_include_trace", True)
    custom_headers = data.get("allowed_custom_headers", [])
    if not isinstance(include_root, str) or not isinstance(lib_root, str):
        raise ValueError(f"{manifest_path}: canonical_mfc include_root/lib_root must be strings")
    if not isinstance(headers, list) or not all(isinstance(item, str) and item for item in headers):
        raise ValueError(f"{manifest_path}: canonical_mfc.required_headers must be strings")
    if not isinstance(libs, list) or not all(isinstance(item, str) and item for item in libs):
        raise ValueError(f"{manifest_path}: canonical_mfc.required_libs must be strings")
    if not isinstance(trace, bool):
        raise ValueError(f"{manifest_path}: canonical_mfc.require_include_trace must be boolean")
    if not isinstance(custom_headers, list) or not all(
        isinstance(item, str) and item for item in custom_headers
    ):
        raise ValueError(f"{manifest_path}: canonical_mfc.allowed_custom_headers must be strings")

    include_root_path = resolve_path(include_root)
    lib_root_path = resolve_path(lib_root)
    for field, names, root in (
        ("required_headers", headers, include_root_path),
        ("required_libs", libs, lib_root_path),
    ):
        for index, name in enumerate(names):
            candidate = Path(name)
            if candidate.is_absolute():
                raise ValueError(
                    f"{manifest_path}: canonical_mfc.{field}[{index}] must be relative to its canonical root"
                )
            try:
                (root / candidate).resolve().relative_to(root.resolve())
            except ValueError as exc:
                raise ValueError(
                    f"{manifest_path}: canonical_mfc.{field}[{index}] must stay inside its canonical root"
                ) from exc

    resolved_custom_headers: list[Path] = []
    seen_custom_headers: set[str] = set()
    for index, raw_path in enumerate(custom_headers):
        path = Path(raw_path)
        if path.is_absolute():
            raise ValueError(
                f"{manifest_path}: canonical_mfc.allowed_custom_headers[{index}] must be repository-relative"
            )
        if any(character in raw_path for character in "*?[]{}$%"):
            raise ValueError(
                f"{manifest_path}: canonical_mfc.allowed_custom_headers[{index}] must be an exact repository-relative path"
            )
        if path.suffix.lower() not in {".h", ".inl"}:
            raise ValueError(
                f"{manifest_path}: canonical_mfc.allowed_custom_headers[{index}] must name a .h or .inl file"
            )
        resolved = resolve_path(raw_path).resolve()
        try:
            resolved.relative_to(repository_root.resolve())
        except ValueError as exc:
            raise ValueError(
                f"{manifest_path}: canonical_mfc.allowed_custom_headers[{index}] must stay inside the repository"
            ) from exc
        try:
            resolved.relative_to(include_root_path.resolve())
        except ValueError:
            pass
        else:
            raise ValueError(
                f"{manifest_path}: canonical_mfc.allowed_custom_headers[{index}] is inside the canonical provider root"
            )
        key = str(resolved).casefold()
        if key in seen_custom_headers:
            raise ValueError(
                f"{manifest_path}: canonical_mfc.allowed_custom_headers contains a duplicate normalized path: {raw_path}"
            )
        seen_custom_headers.add(key)
        resolved_custom_headers.append(resolved)
    return CanonicalMfc(
        include_root=include_root_path,
        lib_root=lib_root_path,
        required_headers=tuple(headers),
        required_libs=tuple(libs),
        require_include_trace=trace,
        allowed_custom_headers=tuple(resolved_custom_headers),
    )


def _provider_file_shape(path: Path) -> dict[str, object]:
    """Describe provider identity using path, format, and visible markers."""

    if not path.is_file():
        return {"exists": False, "size": None, "format": "missing", "markers": []}
    data = path.read_bytes()
    markers: list[str] = []
    suffix = path.suffix.lower()
    if suffix in {".h", ".inl"}:
        text = data.decode("latin-1", errors="replace")
        for marker in (
            "Copyright (C) 1992-1997 Microsoft Corporation",
            "#define __AFXWIN_H__",
            "#define __AFX_H__",
        ):
            if marker in text:
                markers.append(marker)
        file_format = "vc5-header"
    elif data.startswith(b"!<arch>\n"):
        file_format = "coff-library"
        if b"MFC42" in data.upper():
            markers.append("MFC42")
    else:
        file_format = "unknown"
    return {
        "exists": True,
        "size": len(data),
        "format": file_format,
        "markers": markers,
    }


def canonical_mfc_files(spec: CanonicalMfc) -> dict[str, dict[str, object]]:
    result: dict[str, dict[str, object]] = {}
    for name, root in [*((name, spec.include_root) for name in spec.required_headers), *((name, spec.lib_root) for name in spec.required_libs)]:
        path = root / name
        result[str(path.resolve())] = _provider_file_shape(path)
    return result


def validate_canonical_mfc_roots(spec: CanonicalMfc, include_dirs: tuple[Path, ...], lib_dirs: tuple[Path, ...]) -> None:
    if spec.include_root.resolve() not in {path.resolve() for path in include_dirs}:
        raise ValueError(f"canonical VC5SP3 MFC include root is not active: {spec.include_root}")
    if spec.lib_root.resolve() not in {path.resolve() for path in lib_dirs}:
        raise ValueError(f"canonical VC5SP3 MFC lib root is not active: {spec.lib_root}")
    for include_dir in include_dirs:
        candidate = include_dir / "AFXWIN.H"
        if candidate.is_file() and include_dir.resolve() != spec.include_root.resolve():
            raise ValueError(f"competing AFXWIN.H include root is active: {candidate}")
    files = canonical_mfc_files(spec)
    for raw_path, shape in files.items():
        path = Path(raw_path)
        if shape["exists"] is not True:
            raise ValueError(f"canonical VC5SP3 MFC provider file is missing: {path}")
        if path.suffix.lower() in {".h", ".inl"} and shape["format"] != "vc5-header":
            raise ValueError(f"canonical VC5SP3 MFC header has an unexpected format: {path}")
        if path.suffix.lower() == ".lib" and shape["format"] != "coff-library":
            raise ValueError(f"canonical VC5SP3 MFC library is not a COFF archive: {path}")
    afxwin = files.get(str((spec.include_root / "AFXWIN.H").resolve()))
    if afxwin is not None:
        required_markers = {
            "Copyright (C) 1992-1997 Microsoft Corporation",
            "#define __AFXWIN_H__",
        }
        if not required_markers.issubset(set(afxwin.get("markers", []))):
            raise ValueError(
                f"canonical VC5SP3 AFXWIN.H lacks VC5-era provider markers: {spec.include_root / 'AFXWIN.H'}"
            )
    for path in spec.allowed_custom_headers:
        if not path.is_file():
            raise ValueError(f"configured MFC custom header is missing: {path}")


def include_trace_report(spec: CanonicalMfc, logs: Iterable[Path]) -> dict[str, object]:
    observed: dict[str, dict[str, object]] = {}
    malformed: list[str] = []
    allowed_custom_headers = {
        str(path.resolve()).casefold(): path.resolve()
        for path in spec.allowed_custom_headers
    }
    for log in logs:
        if not log.is_file():
            continue
        for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
            match = INCLUDE_TRACE_RE.search(line)
            preprocess_match = PREPROCESS_LINE_RE.match(line)
            if match:
                raw = match.group(1).strip().strip('"')
            elif preprocess_match:
                raw = preprocess_match.group(1).strip()
            else:
                continue
            path = Path(raw)
            basename = path.name.lower()
            if not path.is_absolute():
                if basename.startswith("afx") and path.suffix.lower() in {".h", ".inl"}:
                    malformed.append(raw)
                continue
            resolved = path.resolve()
            is_afx_header = (
                basename.startswith("afx")
                and path.suffix.lower() in {".h", ".inl"}
            )
            is_configured_custom_header = (
                str(resolved).casefold() in allowed_custom_headers
            )
            if not is_afx_header and not is_configured_custom_header:
                continue
            inside = True
            try:
                resolved.relative_to(spec.include_root.resolve())
            except ValueError:
                inside = False
            custom_configuration_input = (
                not inside
                and str(resolved).casefold() in allowed_custom_headers
            )
            classification = (
                "canonical-provider"
                if inside
                else "custom-configuration-input"
                if custom_configuration_input
                else "rejected-external"
            )
            observed[str(resolved)] = {
                "inside_canonical_root": inside,
                "allowed_custom_header": custom_configuration_input,
                "classification": classification,
                **_provider_file_shape(resolved),
            }
    afxwin_observed = any(
        Path(path).name.lower() == "afxwin.h" and row["inside_canonical_root"]
        for path, row in observed.items()
    )
    diagnostics = list(malformed)
    diagnostics.extend(
        path
        for path, row in observed.items()
        if not row["inside_canonical_root"] and not row["allowed_custom_header"]
    )
    if spec.require_include_trace and not afxwin_observed:
        diagnostics.append("AFXWIN.H was not observed in /showIncludes output")
    return {
        "ok": not diagnostics,
        "method": "vc5-preprocessor-line-directives",
        "showincludes_supported": False,
        "canonical_root": str(spec.include_root.resolve()),
        "allowed_custom_headers": [
            str(path)
            for path in sorted(allowed_custom_headers.values(), key=lambda item: str(item).casefold())
        ],
        "afxwin_observed": afxwin_observed,
        "observed": observed,
        "diagnostics": diagnostics,
    }


def section_aux_metadata(coff: CoffObject, section_number: int) -> dict[str, object]:
    candidates = [
        symbol for symbol in coff.symbols
        if symbol.section_number == section_number and symbol.storage_class == 3 and symbol.aux_records
    ]
    if len(candidates) != 1:
        return {"selection": None, "selection_name": "unknown", "associative_section": None}
    symbol = candidates[0]
    selection = symbol.section_definition_selection
    association = symbol.section_definition_association
    return {
        "selection": selection,
        "selection_name": COMDAT_SELECTION_NAMES.get(selection, f"unknown-{selection}"),
        "associative_section": association if selection == 5 else None,
    }


def coff_function_inventory(path: Path) -> dict[str, object]:
    coff = CoffObject.from_path(path)
    groups: dict[tuple[int, int], list[Any]] = {}
    for symbol in coff.symbols:
        if symbol.section_number <= 0 or (symbol.type & IMAGE_SYM_DTYPE_FUNCTION) == 0:
            continue
        section = coff.section(symbol.section_number)
        if (section.characteristics & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE)) == 0:
            continue
        groups.setdefault((symbol.section_number, symbol.value), []).append(symbol)
    functions: list[dict[str, object]] = []
    for (section_number, value), symbols in sorted(groups.items()):
        section = coff.section(section_number)
        names = sorted(symbol.name for symbol in symbols)
        primary = names[0]
        try:
            body = coff.function_bytes(primary)
        except ValueError:
            body_data = b""
            relocations: list[dict[str, object]] = []
        else:
            body_data = body.data
            relocations = [
                {
                    "offset": relocation.offset - body.start,
                    "type": relocation.type,
                    "symbol": relocation.symbol_name,
                }
                for relocation in body.relocations
            ]
        functions.append({
            "section_number": section_number,
            "section_name": section.name,
            "value": value,
            "symbols": names,
            "comdat": bool(section.characteristics & IMAGE_SCN_LNK_COMDAT),
            "comdat_aux": section_aux_metadata(coff, section_number),
            "weak_externals": [
                {
                    "symbol": symbol.name,
                    "tag_index": symbol.weak_external_tag_index,
                    "characteristics": symbol.weak_external_characteristics,
                }
                for symbol in symbols if symbol.storage_class == 105
            ],
            "size": len(body_data),
            # COFF bodies are small enough to expose directly, keeping any
            # mismatch actionable down to the first changed instruction byte.
            "bytes_hex": body_data.hex(),
            "relocations": relocations,
        })
    return {
        "path": str(path.resolve()),
        "size": path.stat().st_size,
        "functions": functions,
    }
