"""Source and native VC5 emission checks for Recoil's implicit copy members.

The source binding permits compilation. Only the current compiler's expanded
translation unit and its own COD/COFF output establish an implicit emission.
Neither check changes authored classification or accepts reconstruction facts.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
from typing import Any, Mapping

from _recoil.commands.asm_verify import CoffObject, IMAGE_SCN_CNT_CODE
from _recoil.lib.source_emission_markers import EmissionAnchor, _mask_comments_and_literals


IMPLICIT_MEMBER_KINDS = frozenset({"copy-constructor", "copy-assignment"})


@dataclass(frozen=True)
class ImplicitCopyBinding:
    kind: str
    symbol: str
    artifact_id: str
    anchor: EmissionAnchor
    translation_unit: str


def validate_member_metadata(function: Any) -> None:
    kind = getattr(function, "implicit_member_kind", "")
    if not kind:
        return
    if kind not in IMPLICIT_MEMBER_KINDS:
        raise ValueError(f"unsupported implicit_member_kind {kind!r}")
    anchor = function.emission_anchor
    if anchor is None or anchor.kind != "type-definition":
        raise ValueError("implicit copy member requires a type-definition emission_anchor")
    if (function.pipeline_class not in {"authored", "authored-lifecycle"}
            or function.authored_order_role not in {"authored-body", "authored-lifecycle-body"}
            or not function.source_order_gate or not function.required_presence or not function.full_order_gate
            or function.provenance or function.logical_identity_key):
        raise ValueError("implicit copy member must retain authored body classification and every presence/order gate")
    if function.symbol_regex is not None or function.listing_label_regex is not None:
        raise ValueError("implicit copy member requires one exact decorated COFF symbol")
    # These are the native VC5 public __thiscall copy signatures emitted for
    # Recoil's global class/struct owners. A different ABI requires a reviewed
    # extension; a matching name prefix alone is insufficient.
    if not re.fullmatch(r"[A-Za-z_]\w*", anchor.name):
        raise ValueError("implicit copy member requires an exact global class/struct name")
    owner = re.escape(anchor.name)
    pattern = (rf"\?\?0{owner}@@QAE@A[AB][UV]0@@Z" if kind == "copy-constructor"
               else rf"\?\?4{owner}@@QAEAA[UV]0@A[AB][UV]0@@Z")
    if not re.fullmatch(pattern, function.symbol):
        raise ValueError("implicit_member_kind, type owner and exact native VC5 copy signature disagree")


def validate_source_binding(*, function: Any, artifact: Any,
                            translation_unit: str,
                            tracker_source_trace: Mapping[str, Any] | None) -> ImplicitCopyBinding:
    validate_member_metadata(function)
    anchor = function.emission_anchor
    if (artifact is None or artifact.relation != "emits" or artifact.section != ".text"
            or not artifact.direct or artifact.construct is None
            or artifact.construct.kind != "type" or artifact.construct.name != anchor.name
            or artifact.path != anchor.path or not artifact.anchor_id):
        raise ValueError("implicit copy member requires its unique attached class emits .text artifact")
    if tracker_source_trace is not None:
        edges = tracker_source_trace.get("source_edges")
        if (tracker_source_trace.get("state") != "resolved" or not isinstance(edges, list)
                or len(edges) != 1 or not isinstance(edges[0], Mapping)
                or edges[0].get("relation") != "emits"
                or edges[0].get("anchor_id") != artifact.anchor_id
                or edges[0].get("emission_context") != {"translation_unit": translation_unit}):
            raise ValueError("implicit copy member requires the exact resolved tracker class/TU emission edge")
    return ImplicitCopyBinding(function.implicit_member_kind, function.symbol,
                               artifact.artifact_id, anchor, translation_unit)


def _matching_brace(text: str, start: int) -> int:
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return index
    raise ValueError("expanded source has an unterminated type definition")


def validate_expanded_source(text: str, binding: ImplicitCopyBinding, *,
                             compilation_directory: Path,
                             compiled_source: Path) -> dict[str, Any]:
    """Reject explicit members after macros and includes have been expanded."""
    masked = _mask_comments_and_literals(text)
    # #line filenames are strings, already masked; discard directives without
    # deleting newlines so all diagnostic locations remain compiler locations.
    masked = re.sub(r"(?m)^\s*#[^\r\n]*", lambda m: " " * len(m[0]), masked)
    owner = re.escape(binding.anchor.name)
    declarations = list(re.finditer(rf"\b(?:class|struct)\s+{owner}\b[^;{{}}]*\{{", masked))
    if len(declarations) != 1:
        raise ValueError(f"expanded source must contain exactly one definition of {binding.anchor.name}; found {len(declarations)}")
    declaration = declarations[0]
    from _recoil.lib.tooling import REPO_ROOT
    expanded_line = masked.count("\n", 0, declaration.start()) + 1
    origin_path, origin_line = None, None
    for line_number, line in enumerate(text.splitlines(), start=1):
        directive = re.match(r'^\s*#(?:line\s+|\s*)(\d+)\s+"([^"]+)"', line)
        if directive:
            origin_path = Path(directive[2])
            origin_line = int(directive[1]) - 1
        elif origin_line is not None:
            origin_line += 1
        if line_number == expanded_line:
            break
    if origin_path is None:
        raise ValueError("expanded class definition lacks compiler source-origin directives")
    physical_origin = (compilation_directory / origin_path).resolve()
    logical_origin = ((REPO_ROOT / binding.translation_unit).resolve()
                      if physical_origin == compiled_source.resolve() else physical_origin)
    if logical_origin != (REPO_ROOT / binding.anchor.path).resolve():
        raise ValueError("expanded class definition originates outside its attached source anchor")
    opening = declaration.end() - 1
    end = _matching_brace(masked, opening)
    # Only declarations at the class's top level count. Calls in method bodies
    # and declarations in nested types do not declare this class's copy member.
    body = list(masked[opening + 1:end])
    cursor = 0
    while cursor < len(body):
        if body[cursor] == "{":
            close = _matching_brace("".join(body), cursor)
            for index in range(cursor, close + 1):
                if body[index] not in "\r\n":
                    body[index] = " "
            cursor = close
        cursor += 1
    class_declarations = "".join(body)
    member = owner if binding.kind == "copy-constructor" else r"operator\s*="
    for match in re.finditer(rf"(?<![\w~]){member}\s*\(([^()]*)\)", class_declarations):
        first_parameter = match[1].split(",", 1)[0]
        if not first_parameter.strip() or first_parameter.strip() == "void":
            continue
        if re.search(rf"\b{owner}\b", first_parameter) and "*" not in first_parameter:
            raise ValueError(f"expanded source explicitly declares the selected {binding.kind}")
        # A typedef can hide the self type. Fail closed for reference/unknown
        # class parameters instead of calling an unrecognized signature implicit.
        if "&" in first_parameter and not re.search(
                r"\b(?:bool|char|short|int|long|float|double|wchar_t)\b", first_parameter):
            raise ValueError("expanded source has an unresolved copy-like member parameter")
    qualified = rf"\b{owner}\s*::\s*{member}\s*\(([^()]*)\)"
    outside = masked[:declaration.start()] + masked[end + 1:]
    for match in re.finditer(qualified, outside):
        first_parameter = match[1].split(",", 1)[0]
        if re.search(rf"\b{owner}\b", first_parameter) or "&" in first_parameter:
            raise ValueError("expanded source contains an out-of-class selected member declaration/definition")
    return {"type": binding.anchor.name, "implicit_member_kind": binding.kind,
            "expanded_type_line": expanded_line, "source_path": binding.anchor.path,
            "source_line": origin_line,
            "explicit_selected_member": False}


def validate_native_emission(*, binding: ImplicitCopyBinding, coff: CoffObject,
                             cod_text: str, source_from: str) -> dict[str, Any]:
    if source_from != binding.translation_unit:
        raise ValueError("implicit copy member was compiled in the wrong translation unit")
    definitions = [row for row in coff.symbols if row.name == binding.symbol and row.section_number > 0]
    if len(definitions) != 1:
        raise ValueError(f"implicit copy member must have one native COFF definition; found {len(definitions)}")
    symbol = definitions[0]
    section = next((row for row in coff.sections if row.index == symbol.section_number), None)
    if (symbol.storage_class != 2 or symbol.value != 0 or symbol.type != 0x20 or section is None
            or not section.raw_data or not section.characteristics & IMAGE_SCN_CNT_CODE
            or not section.characteristics & 0x1000 or not section.name.startswith(".text")):
        raise ValueError("implicit copy member is not a native code COMDAT definition")
    if [row.name for row in coff.symbols if row.section_number == section.index
            and getattr(row, "storage_class", None) == 2 and getattr(row, "type", None) == 0x20] != [binding.symbol]:
        raise ValueError("implicit copy member shares its native COMDAT with another external function")
    section_definitions = [row for row in coff.symbols
                           if row.section_number == section.index and row.section_definition_selection is not None]
    if len(section_definitions) != 1 or section_definitions[0].section_definition_selection != 2:
        raise ValueError("implicit copy member requires the native VC5 ANY COMDAT selection")
    procs = re.findall(r"(?m)^\s*(\S+)\s+PROC\b", cod_text)
    if procs.count(binding.symbol) != 1:
        raise ValueError("implicit copy member requires one matching native COD PROC")
    ends = re.findall(r"(?m)^\s*(\S+)\s+ENDP\b", cod_text)
    if ends.count(binding.symbol) != 1:
        raise ValueError("implicit copy member requires one matching native COD ENDP")
    relocations = coff.relocations_by_section.get(section.index, ())
    prior_end = -1
    for relocation in sorted(relocations, key=lambda row: row.offset):
        if (relocation.type not in {6, 7, 20} or relocation.offset < prior_end
                or relocation.offset + 4 > len(section.raw_data)
                or relocation.symbol_index not in coff.symbols_by_index
                or not relocation.symbol_name):
            raise ValueError("implicit copy member contains an invalid/overlapping relocation")
        prior_end = relocation.offset + 4
    associations = []
    pending = [section.index]
    while pending:
        parent = pending.pop()
        for row in coff.symbols:
            if row.section_definition_selection != 5 or row.section_definition_association != parent:
                continue
            associated = next((item for item in coff.sections if item.index == row.section_number), None)
            if (associated is None or associated.index == section.index or associated.index in associations
                    or not associated.characteristics & 0x1000):
                raise ValueError("implicit copy member has an invalid/duplicate associated COMDAT")
            prior_end = -1
            for relocation in sorted(coff.relocations_by_section.get(associated.index, ()), key=lambda item: item.offset):
                if (relocation.type not in {6, 7, 20} or relocation.offset < prior_end
                        or relocation.offset + 4 > len(associated.raw_data)
                        or relocation.symbol_index not in coff.symbols_by_index or not relocation.symbol_name):
                    raise ValueError("implicit copy member has an invalid associated COMDAT relocation")
                prior_end = relocation.offset + 4
            associations.append(associated.index)
            pending.append(associated.index)
    return {"kind": binding.kind, "symbol": binding.symbol, "translation_unit": source_from,
            "section_index": section.index, "comdat_selection": "ANY",
            "associated_sections": sorted(associations), "body_extent": len(section.raw_data),
            "relocations": [{"offset": row.offset, "type": row.type, "target": row.symbol_name}
                            for row in relocations], "native_emission": True}
