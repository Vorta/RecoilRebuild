from __future__ import annotations

from copy import deepcopy
from pathlib import Path
from typing import Any, Mapping

from _recoil.lib.progress import ProgressError, normalize_address
from _recoil.lib.source_traceability import parse_source_trace_text
from _recoil.lib.tooling import REPO_ROOT


AUTHORED_ICF_GROUP_MODEL = "authored-linker-coalesced-v1"
AUTHORED_ICF_PROOF_SCHEMA_V1 = "recoil-authored-icf-proof-v1"
AUTHORED_ICF_PROOF_SCHEMA_V2 = "recoil-authored-icf-proof-v2"
AUTHORED_ICF_PROOF_SCHEMA = AUTHORED_ICF_PROOF_SCHEMA_V2
AUTHORED_ICF_MEMBER_GATE_MODE = "physical-body-only"
AUTHORED_ICF_CANDIDATE_ROLE = "corroborating-source-link-mechanism-only"
AUTHORED_ICF_ICF_PROFILE = "vc5sp3_ref_icf"
AUTHORED_ICF_NOICF_PROFILE = "vc5sp3_ref_noicf"
AUTHORED_ICF_LIFECYCLE_SOURCE_MODEL = "header-lifecycle-v1"
_IMAGE_COMDAT_SELECTION_VALUES = frozenset(range(1, 8))


def _exact_mapping(value: Any, fields: set[str], *, label: str) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        raise ProgressError(f"{label} must be an object")
    if set(value) != fields:
        raise ProgressError(
            f"{label} fields must be exactly {sorted(fields)!r}; "
            f"found {sorted(str(field) for field in value)!r}"
        )
    return deepcopy(dict(value))


def _string(value: Any, *, label: str) -> str:
    if not isinstance(value, str) or not value or value.strip() != value:
        raise ProgressError(f"{label} must be a non-empty trimmed string")
    return value


def _string_array(value: Any, *, label: str, allow_empty: bool = False) -> list[str]:
    if not isinstance(value, list) or (not value and not allow_empty):
        qualifier = "an array" if allow_empty else "a non-empty array"
        raise ProgressError(f"{label} must be {qualifier} of strings")
    result: list[str] = []
    for index, item in enumerate(value):
        result.append(_string(item, label=f"{label}[{index}]"))
    if len(result) != len(set(result)):
        raise ProgressError(f"{label} contains duplicates")
    return result


def _decorated_symbol(value: Any, *, label: str) -> str:
    symbol = _string(value, label=label)
    if symbol[0] not in {"?", "_", "@"} or any(character.isspace() for character in symbol):
        raise ProgressError(f"{label} must be one exact decorated VC5 object symbol")
    return symbol


def _comdat_selection(value: Any, *, comdat_eligible: bool, label: str) -> int | None:
    """Return one exact IMAGE_COMDAT_SELECT_* value, or null for a non-COMDAT."""

    if not comdat_eligible:
        if value is not None:
            raise ProgressError(f"{label} must be null when comdat_eligible=false")
        return None
    if (
        not isinstance(value, int)
        or isinstance(value, bool)
        or value not in _IMAGE_COMDAT_SELECTION_VALUES
    ):
        raise ProgressError(
            f"{label} must be an exact IMAGE_COMDAT_SELECT_* integer from 1 through 7"
        )
    return value


def _repository_report_path(value: Any, *, label: str) -> str:
    path = _string(value, label=label).replace("\\", "/")
    if (
        path != value
        or Path(path).is_absolute()
        or ".." in Path(path).parts
        or not path.startswith("build/reconstruction-evidence/runs/")
        or Path(path).suffix.lower() not in {".json", ".txt", ".map"}
    ):
        raise ProgressError(
            f"{label} must be a normalized durable build/reconstruction-evidence/runs/ "
            "JSON, text, or MAP transcript path"
        )
    return path


def validate_authored_icf_source_mirrors(
    aliases: Mapping[str, Mapping[str, Any]],
    *,
    repo_root: Path | None = None,
) -> dict[str, str]:
    """Require each logical defining edge to exist as an attached source mirror."""

    root = (repo_root or REPO_ROOT).resolve()
    validated: dict[str, str] = {}
    for alias_id, alias in aliases.items():
        source_trace = alias.get("source_traceability")
        edges = source_trace.get("source_edges") if isinstance(source_trace, Mapping) else None
        if not isinstance(edges, list) or len(edges) != 1 or not isinstance(edges[0], Mapping):
            raise ProgressError(
                f"authored ICF logical member {alias_id!r} has no exclusive source edge"
            )
        edge = edges[0]
        context = edge.get("emission_context")
        translation_unit = (
            str(context.get("translation_unit", ""))
            if isinstance(context, Mapping)
            else ""
        )
        if (
            not translation_unit.startswith("src/")
            or Path(translation_unit).suffix.lower() not in {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp"}
        ):
            raise ProgressError(
                f"authored ICF logical member {alias_id!r} translation unit must be "
                "a normalized production source/header path under src/"
            )
        path = (root / translation_unit).resolve()
        try:
            path.relative_to(root)
        except ValueError as exc:
            raise ProgressError(
                f"authored ICF logical member {alias_id!r} source path escapes the repository"
            ) from exc
        if not path.is_file():
            raise ProgressError(
                f"authored ICF logical member {alias_id!r} source mirror does not exist: "
                f"{translation_unit}"
            )
        try:
            text = path.read_text(encoding="utf-8")
            document = parse_source_trace_text(
                text,
                path=translation_unit,
                legacy_binary="recoil",
            )
        except (OSError, UnicodeError, ValueError) as exc:
            raise ProgressError(
                f"authored ICF logical member {alias_id!r} source mirror cannot be parsed: {exc}"
            ) from exc
        if document.findings:
            details = "; ".join(
                f"{finding.path}:{finding.line}:{finding.code}"
                for finding in document.findings[:6]
            )
            raise ProgressError(
                f"authored ICF logical member {alias_id!r} source mirror is invalid: {details}"
            )
        anchor_id = str(edge.get("anchor_id", ""))
        generation = alias.get("source_generation")
        implicit_destructor = False
        if generation is not None:
            generation = _exact_mapping(
                generation, {"kind", "class_name", "translation_units"},
                label=f"authored ICF lifecycle source {alias_id!r}",
            )
            if generation["kind"] not in {"inline-destructor", "implicit-destructor"}:
                raise ProgressError("authored ICF lifecycle source kind is unsupported")
            class_name = _string(generation["class_name"], label="lifecycle class name")
            if not class_name.isidentifier() or alias.get("object_symbol") != f"??1{class_name}@@UAE@XZ":
                raise ProgressError("authored ICF lifecycle class does not bind its complete destructor")
            units = _string_array(generation["translation_units"], label="lifecycle emission TUs")
            for unit in units:
                if (
                    not unit.startswith("src/") or "\\" in unit or ".." in Path(unit).parts
                    or Path(unit).suffix.lower() not in {".cpp", ".cc", ".cxx"}
                    or not (root / unit).is_file()
                ):
                    raise ProgressError("authored ICF lifecycle emission TU is not current production source")
            implicit_destructor = generation["kind"] == "implicit-destructor"
        definitions = [
            artifact
            for artifact in document.artifacts
            if artifact.artifact_id == alias_id
            and artifact.relation == ("emits" if implicit_destructor else "defines")
            and artifact.section == ".text"
            and artifact.anchor_id == anchor_id
            and artifact.direct
            and artifact.entity_kind == "function"
            and artifact.construct is not None
            and artifact.construct.kind == ("type" if implicit_destructor else "function")
        ]
        if len(definitions) != 1:
            raise ProgressError(
                f"authored ICF logical member {alias_id!r} source mirror requires exactly one attached "
                f"'@recoil-artifact defines .text {alias_id}: ...' mirror at {anchor_id!r} "
                f"in {translation_unit}; found {len(definitions)}"
            )
        if generation is not None:
            construct = definitions[0].construct
            assert construct is not None
            if implicit_destructor:
                if construct.name != class_name:
                    raise ProgressError("authored ICF implicit destructor is attached to another class")
                # Use the parsed class extent, then reject any destructor declaration
                # or definition in that class. This is a generation edge, not a
                # license to attach an unrelated function to a type declaration.
                from _recoil.lib.source_constructs import parse_source_constructs
                parsed = parse_source_constructs(text)
                if any(
                    member.kind == "function"
                    and construct.start < member.start < construct.end
                    and "~" in member.name
                    for member in parsed
                ):
                    raise ProgressError("authored ICF implicit destructor class has an explicit destructor")
                import re
                if re.search(r"~\s*" + re.escape(class_name) + r"\s*\(", text[construct.start:construct.end]):
                    raise ProgressError("authored ICF implicit destructor class declares a destructor")
            elif construct.name.split("::")[-1] != "~" + class_name:
                raise ProgressError("authored ICF inline destructor source names another function")
            if edge.get("relation") != ("emits" if implicit_destructor else "defines"):
                raise ProgressError("authored ICF lifecycle source relation disagrees with its generation kind")
        validated[str(alias_id)] = translation_unit
    return validated


def validate_authored_icf_physical_source_artifacts(
    data: Mapping[str, Any],
    *,
    physical_symbol_id: str,
    documents: tuple[Any, ...],
    select_single_logical_member: bool = False,
    object_witness: str | None = None,
) -> tuple[str, ...] | None:
    """Validate one physical ICF gate through its exact logical source mirrors.

    Return ``None`` when the physical row is not a reviewed authored-ICF group.
    A matching group is revalidated through the canonical audit before its
    source closure is required to contain exactly one defining artifact for
    every current logical member whose reviewed source edge names one of the
    supplied source documents, and no other function artifact at the address.
    """

    symbols = data.get("symbols")
    symbol = symbols.get(physical_symbol_id) if isinstance(symbols, Mapping) else None
    group = symbol.get("icf_address_group") if isinstance(symbol, Mapping) else None
    if not isinstance(group, Mapping) or group.get("model") != AUTHORED_ICF_GROUP_MODEL:
        return None

    scoped = deepcopy(dict(data))
    scoped["symbols"] = {physical_symbol_id: deepcopy(dict(symbol))}
    findings = audit_authored_icf_groups(scoped)
    if findings:
        code, message = findings[0]
        raise ProgressError(f"{code}: {message}")

    aliases = symbol.get("logical_aliases")
    if not isinstance(aliases, Mapping):
        raise ProgressError("authored ICF logical member population is incomplete")
    lifecycle = group.get("source_model") == AUTHORED_ICF_LIFECYCLE_SOURCE_MODEL
    physical_address = normalize_address(str(symbol.get("address", "")))
    same_address = [
        artifact
        for document in documents
        for artifact in document.artifacts
        if artifact.entity_kind == "function"
        and artifact.address == physical_address
    ]
    actual_ids = [str(artifact.artifact_id) for artifact in same_address]
    logical_ids = set(str(alias_id) for alias_id in aliases)
    if lifecycle:
        # Header attachment and actual emission are distinct facts. Require the
        # header mirrors in the checked closure as well as a registered emission
        # TU; retain the exact-population/duplicate/physical-artifact checks below.
        document_paths = {str(document.path).replace("\\", "/") for document in documents}
        expected_ids = {
            str(alias_id) for alias_id, alias in aliases.items()
            if document_paths.intersection(alias["source_generation"]["translation_units"])
        }
        if not expected_ids:
            raise ProgressError("authored ICF lifecycle source closure has no proven emission TU")
        if len([
            alias_id for alias_id in expected_ids
            if aliases[alias_id].get("object_symbol") == object_witness
        ]) != 1:
            raise ProgressError("authored ICF lifecycle target requires an explicit member witness")
    elif select_single_logical_member:
        expected_ids = set(actual_ids) & logical_ids
        if len(expected_ids) != 1:
            raise ProgressError(
                "authored ICF secondary target source closure must select exactly one "
                f"current logical member; found {sorted(expected_ids)!r}"
            )
    else:
        document_paths = {
            str(getattr(document, "path", "")).replace("\\", "/")
            for document in documents
        }
        expected_ids: set[str] = set()
        for alias_id, alias in aliases.items():
            source_trace = (
                alias.get("source_traceability")
                if isinstance(alias, Mapping)
                else None
            )
            edges = (
                source_trace.get("source_edges")
                if isinstance(source_trace, Mapping)
                else None
            )
            if (
                not isinstance(edges, list)
                or len(edges) != 1
                or not isinstance(edges[0], Mapping)
            ):
                raise ProgressError(
                    f"authored ICF logical member {alias_id!r} source edge is not exclusive"
                )
            context = edges[0].get("emission_context")
            translation_unit = (
                str(context.get("translation_unit", "")).replace("\\", "/")
                if isinstance(context, Mapping)
                else ""
            )
            if translation_unit in document_paths:
                expected_ids.add(str(alias_id))
        if not expected_ids:
            raise ProgressError(
                "authored ICF physical source closure must select a nonempty current "
                "logical defining population from its supplied source documents"
            )
    counts = {artifact_id: actual_ids.count(artifact_id) for artifact_id in set(actual_ids)}
    if set(actual_ids) != expected_ids or any(
        counts.get(alias_id) != 1 for alias_id in expected_ids
    ):
        raise ProgressError(
            "authored ICF physical source closure must contain exactly the current "
            f"logical defining population {sorted(expected_ids)!r}; found "
            f"{sorted(actual_ids)!r}"
        )

    for alias_id in expected_ids:
        alias = aliases[alias_id]
        source_trace = alias.get("source_traceability") if isinstance(alias, Mapping) else None
        edges = source_trace.get("source_edges") if isinstance(source_trace, Mapping) else None
        if not isinstance(edges, list) or len(edges) != 1 or not isinstance(edges[0], Mapping):
            raise ProgressError(
                f"authored ICF logical member {alias_id!r} source edge is not exclusive"
            )
        edge = edges[0]
        context = edge.get("emission_context")
        translation_unit = (
            str(context.get("translation_unit", ""))
            if isinstance(context, Mapping)
            else ""
        )
        artifact = next(
            row for row in same_address if row.artifact_id == alias_id
        )
        implicit = lifecycle and alias["source_generation"]["kind"] == "implicit-destructor"
        if (
            artifact.relation != ("emits" if implicit else "defines")
            or artifact.section != ".text"
            or not artifact.direct
            or artifact.construct is None
            or artifact.construct.kind != ("type" if implicit else "function")
            or artifact.anchor_id != edge.get("anchor_id")
            or artifact.path != translation_unit
        ):
            raise ProgressError(
                f"authored ICF logical member {alias_id!r} source artifact does not "
                "match its exclusive defining edge"
            )
    return tuple(sorted(expected_ids))


def select_authored_icf_translation_unit_object_symbol(
    data: Mapping[str, Any],
    *,
    physical_symbol_id: str,
    translation_unit: str,
    object_witness: str | None = None,
    coff_object: Any = None,
) -> tuple[str, str] | None:
    """Select the one proof-bound logical object symbol defined by one TU.

    Return ``None`` when the physical row is not a reviewed authored-ICF
    group.  A matching group is revalidated in full before its exclusive
    logical source edges are joined to the requested translation unit.  The
    returned logical identity is diagnostic only; the caller must retain the
    physical address/order gate and use only the decorated symbol for candidate
    COFF lookup.
    """

    symbols = data.get("symbols")
    symbol = symbols.get(physical_symbol_id) if isinstance(symbols, Mapping) else None
    group = symbol.get("icf_address_group") if isinstance(symbol, Mapping) else None
    if not isinstance(group, Mapping) or group.get("model") != AUTHORED_ICF_GROUP_MODEL:
        return None

    scoped = deepcopy(dict(data))
    scoped["symbols"] = {physical_symbol_id: deepcopy(dict(symbol))}
    findings = audit_authored_icf_groups(scoped)
    if findings:
        code, message = findings[0]
        raise ProgressError(f"{code}: {message}")

    requested_tu = _string(
        translation_unit,
        label="authored ICF translation-unit object-symbol projection path",
    ).replace("\\", "/")
    aliases = symbol.get("logical_aliases")
    if not isinstance(aliases, Mapping):
        raise ProgressError("authored ICF logical member population is incomplete")

    matches: list[tuple[str, str]] = []
    for alias_id, alias in aliases.items():
        source_trace = alias.get("source_traceability") if isinstance(alias, Mapping) else None
        edges = source_trace.get("source_edges") if isinstance(source_trace, Mapping) else None
        if not isinstance(edges, list) or len(edges) != 1 or not isinstance(edges[0], Mapping):
            raise ProgressError(
                f"authored ICF logical member {alias_id!r} source edge is not exclusive"
            )
        context = edges[0].get("emission_context")
        defining_tu = (
            str(context.get("translation_unit", "")).replace("\\", "/")
            if isinstance(context, Mapping)
            else ""
        )
        generation = alias.get("source_generation") if isinstance(alias, Mapping) else None
        defining_tus = generation.get("translation_units", []) if isinstance(generation, Mapping) else [defining_tu]
        if requested_tu not in defining_tus:
            continue
        object_symbol = _decorated_symbol(
            alias.get("object_symbol"),
            label=f"authored ICF logical member {alias_id!r} object_symbol",
        )
        matches.append((str(alias_id), object_symbol))

    if group.get("source_model") == AUTHORED_ICF_LIFECYCLE_SOURCE_MODEL:
        witnesses = [match for match in matches if match[1] == object_witness]
        if len(witnesses) != 1:
            raise ProgressError("authored ICF same-TU group requires one explicit proven object witness")
        validate_lifecycle_object_members(coff_object, tuple(symbol for _, symbol in matches))
        return witnesses[0]
    if len(matches) != 1:
        raise ProgressError(
            "authored ICF translation-unit object-symbol projection requires exactly "
            f"one current proven logical member for {requested_tu!r}; found "
            f"{[alias_id for alias_id, _symbol in matches]!r}"
        )
    return matches[0]


def validate_lifecycle_object_members(coff_object: Any, symbols: tuple[str, ...]) -> None:
    """Recheck the entire selected same-TU fold family from fresh COFF.

    This is candidate-mechanism corroboration, not retail expected truth. A
    convenient order witness cannot hide a missing member or a changed cleanup
    body. This deliberately narrow lifecycle route rejects recursive/associative
    folding except VC5's self-referencing FPO record: other cases need their
    own typed proof, not a guessed equivalence.
    """
    if coff_object is None or not symbols or len(set(symbols)) != len(symbols):
        raise ProgressError("authored ICF lifecycle projection requires fresh complete COFF members")
    signatures = []
    for name in symbols:
        definitions = [
            row for row in coff_object.symbols
            if row.name == name and row.section_number > 0
            and row.storage_class == 2 and row.type == 0x20
        ]
        if len(definitions) != 1:
            raise ProgressError(f"authored ICF lifecycle member {name!r} lacks one fresh definition")
        definition = definitions[0]
        body = coff_object.function_bytes(name)
        section = coff_object.section(definition.section_number)
        selections = [
            row.section_definition_selection for row in coff_object.symbols
            if row.section_number == definition.section_number
            and row.section_definition_selection is not None
        ]
        if (
            section.name != ".text" or not section.characteristics & 0x1000
            or selections != [2] or body.start != 0
            or body.end != len(section.raw_data) or not body.data
        ):
            raise ProgressError(f"authored ICF lifecycle member {name!r} is not one independent ANY COMDAT")
        relocations = tuple(sorted(
            (row.offset, row.type, row.symbol_name)
            for row in body.relocations
        ))
        associations = [
            row for row in coff_object.symbols
            if row.section_definition_selection == 5
            and row.section_definition_association == definition.section_number
        ]
        fpo = None
        if associations:
            if len(associations) != 1:
                raise ProgressError("authored ICF lifecycle has unsupported associative sections")
            associated = coff_object.section(associations[0].section_number)
            associated_relocations = coff_object.relocations_by_section.get(associated.index, ())
            if (
                associated.name != ".debug$F" or associated.characteristics != 0x42101048
                or len(associated.raw_data) != 16 or associated.raw_data[:4] != b"\0" * 4
                or len(associated_relocations) != 1
                or associated_relocations[0].offset != 0
                or associated_relocations[0].type != 7
                or associated_relocations[0].symbol_name != name
                or associated_relocations[0].symbol_index != definition.index
            ):
                raise ProgressError("authored ICF lifecycle has unsupported FPO association")
            fpo = associated.raw_data
        # The raw relocation fields preserve exact addends; names preserve exact
        # target identity. Never equate unrelated targets just because they fold.
        signatures.append((body.data, relocations, fpo))
    if any(signature != signatures[0] for signature in signatures[1:]):
        raise ProgressError("authored ICF lifecycle members no longer have identical complete bytes and relocations")


def _link_observation(
    value: Any,
    *,
    label: str,
    profile: str,
    required_flag: str,
    forbidden_flag: str,
    alias_ids: set[str],
) -> dict[str, Any]:
    row = _exact_mapping(
        value,
        {"link_profile", "effective_link_flags", "member_addresses", "transcript_path"},
        label=label,
    )
    if row["link_profile"] != profile:
        raise ProgressError(f"{label}.link_profile must be {profile!r}")
    flags = _string_array(row["effective_link_flags"], label=f"{label}.effective_link_flags")
    upper_flags = {flag.upper() for flag in flags}
    if "/OPT:REF" not in upper_flags or required_flag not in upper_flags:
        raise ProgressError(
            f"{label} requires effective /OPT:REF and {required_flag} flags"
        )
    if forbidden_flag in upper_flags or "/OPT:NOREF" in upper_flags:
        raise ProgressError(f"{label} rejects {forbidden_flag} or /OPT:NOREF")
    addresses = row["member_addresses"]
    if not isinstance(addresses, Mapping) or set(addresses) != alias_ids:
        raise ProgressError(
            f"{label}.member_addresses must cover the exact logical member population"
        )
    row["member_addresses"] = {
        str(alias_id): normalize_address(str(address))
        for alias_id, address in addresses.items()
    }
    row["effective_link_flags"] = flags
    row["transcript_path"] = _repository_report_path(
        row["transcript_path"], label=f"{label}.transcript_path"
    )
    return row


def validate_authored_icf_proof(
    value: Any,
    *,
    physical_address: str,
    aliases: Mapping[str, Mapping[str, Any]],
) -> dict[str, Any]:
    """Validate the reviewed retail-truth/candidate-mechanism evidence split.

    Candidate OBJ/MAP observations are mandatory corroboration that current source
    naturally emits independently eligible definitions which split under NOICF and
    fold under ICF.  They never supply retail target identity, retail addresses, or
    selector truth.
    """

    proof = _exact_mapping(
        value,
        {"schema", "retail_truth", "candidate_mechanism"},
        label="authored ICF proof",
    )
    proof_schema = proof["schema"]
    if proof_schema not in {
        AUTHORED_ICF_PROOF_SCHEMA_V1,
        AUTHORED_ICF_PROOF_SCHEMA_V2,
    }:
        raise ProgressError(
            "authored ICF proof.schema must be "
            f"{AUTHORED_ICF_PROOF_SCHEMA_V1!r} or {AUTHORED_ICF_PROOF_SCHEMA_V2!r}"
        )
    requires_comdat_selection = proof_schema == AUTHORED_ICF_PROOF_SCHEMA_V2
    alias_ids = set(aliases)
    if len(alias_ids) < 2:
        raise ProgressError("authored ICF proof requires at least two logical members")

    retail = _exact_mapping(
        proof["retail_truth"],
        {
            "candidate_independent",
            "candidate_output_used_as_expected",
            "physical_address",
            "logical_identity_keys",
            "selector_source",
        },
        label="authored ICF proof.retail_truth",
    )
    if retail["candidate_independent"] is not True:
        raise ProgressError("authored ICF retail truth requires candidate_independent=true")
    if retail["candidate_output_used_as_expected"] is not False:
        raise ProgressError(
            "authored ICF retail truth requires candidate_output_used_as_expected=false"
        )
    if normalize_address(str(retail["physical_address"])) != normalize_address(
        physical_address
    ):
        raise ProgressError("authored ICF retail truth physical_address is stale")
    retail_aliases = set(
        _string_array(
            retail["logical_identity_keys"],
            label="authored ICF proof.retail_truth.logical_identity_keys",
        )
    )
    if retail_aliases != alias_ids:
        raise ProgressError(
            "authored ICF retail truth must cover the exact logical member population"
        )
    retail["logical_identity_keys"] = sorted(retail_aliases)
    retail["physical_address"] = normalize_address(str(retail["physical_address"]))
    retail["selector_source"] = _string(
        retail["selector_source"],
        label="authored ICF proof.retail_truth.selector_source",
    )

    candidate = _exact_mapping(
        proof["candidate_mechanism"],
        {
            "candidate_output_used",
            "candidate_output_role",
            "generated_from_current_source",
            "same_object_inputs",
            "object_members",
            "selector_bindings",
            "noicf_link",
            "icf_link",
            "negative_control",
        },
        label="authored ICF proof.candidate_mechanism",
    )
    if candidate["candidate_output_used"] is not True:
        raise ProgressError("authored ICF candidate mechanism requires candidate_output_used=true")
    if candidate["candidate_output_role"] != AUTHORED_ICF_CANDIDATE_ROLE:
        raise ProgressError(
            "authored ICF candidate output role must be corroborating source/link mechanism only"
        )
    if candidate["generated_from_current_source"] is not True:
        raise ProgressError(
            "authored ICF candidate mechanism requires generated_from_current_source=true"
        )
    if candidate["same_object_inputs"] is not True:
        raise ProgressError(
            "authored ICF ICF/NOICF observations require the same object inputs"
        )

    raw_members = candidate["object_members"]
    if not isinstance(raw_members, Mapping) or set(raw_members) != alias_ids:
        raise ProgressError(
            "authored ICF object_members must cover the exact logical member population"
        )
    members: dict[str, dict[str, Any]] = {}
    object_paths: set[str] = set()
    object_symbols: set[str] = set()
    for alias_id, raw_member in raw_members.items():
        label = f"authored ICF proof.candidate_mechanism.object_members[{alias_id!r}]"
        member_fields = {
            "object_path",
            "object_symbol",
            "comdat_eligible",
            "definition_count",
            "relocation_partition",
            "relocation_partition_complete",
            "object_report_path",
        }
        if requires_comdat_selection:
            member_fields.add("comdat_selection")
        member = _exact_mapping(
            raw_member,
            member_fields,
            label=label,
        )
        object_path = _string(member["object_path"], label=f"{label}.object_path").replace(
            "\\", "/"
        )
        if Path(object_path).is_absolute() or ".." in Path(object_path).parts:
            raise ProgressError(f"{label}.object_path must be repository-relative")
        if Path(object_path).suffix.lower() != ".obj":
            raise ProgressError(f"{label}.object_path must name one VC5 object file")
        object_symbol = _decorated_symbol(
            member["object_symbol"], label=f"{label}.object_symbol"
        )
        expected_symbol = str(aliases[alias_id].get("object_symbol", ""))
        if object_symbol != expected_symbol:
            raise ProgressError(f"{label}.object_symbol does not match its logical member")
        if member["comdat_eligible"] is not True or member["definition_count"] != 1:
            raise ProgressError(
                f"{label} requires one independently eligible COMDAT definition"
            )
        if requires_comdat_selection:
            member["comdat_selection"] = _comdat_selection(
                member["comdat_selection"],
                comdat_eligible=True,
                label=f"{label}.comdat_selection",
            )
        raw_partition = member["relocation_partition"]
        if not isinstance(raw_partition, list):
            raise ProgressError(f"{label}.relocation_partition must be an explicit array")
        if member["relocation_partition_complete"] is not True:
            raise ProgressError(
                f"{label} requires relocation_partition_complete=true"
            )
        partition: list[dict[str, Any]] = []
        offsets: set[int] = set()
        for index, raw_relocation in enumerate(raw_partition):
            relocation_label = f"{label}.relocation_partition[{index}]"
            relocation = _exact_mapping(
                raw_relocation,
                {"offset", "type", "target_identity", "addend"},
                label=relocation_label,
            )
            offset = relocation["offset"]
            addend = relocation["addend"]
            if not isinstance(offset, int) or isinstance(offset, bool) or offset < 0:
                raise ProgressError(f"{relocation_label}.offset must be a non-negative integer")
            if offset in offsets:
                raise ProgressError(f"{label}.relocation_partition has duplicate offsets")
            if not isinstance(addend, int) or isinstance(addend, bool):
                raise ProgressError(f"{relocation_label}.addend must be an integer")
            relocation["type"] = _string(relocation["type"], label=f"{relocation_label}.type")
            relocation["target_identity"] = _string(
                relocation["target_identity"],
                label=f"{relocation_label}.target_identity",
            )
            offsets.add(offset)
            partition.append(relocation)
        if object_symbol in object_symbols:
            raise ProgressError(
                "authored ICF proof requires distinct decorated member symbols"
            )
        object_paths.add(object_path)
        object_symbols.add(object_symbol)
        member["object_path"] = object_path
        member["object_symbol"] = object_symbol
        member["relocation_partition"] = partition
        member["object_report_path"] = _repository_report_path(
            member["object_report_path"], label=f"{label}.object_report_path"
        )
        members[str(alias_id)] = member

    raw_bindings = candidate["selector_bindings"]
    if not isinstance(raw_bindings, Mapping) or set(raw_bindings) != alias_ids:
        raise ProgressError(
            "authored ICF selector_bindings must cover the exact logical member population"
        )
    selector_bindings: dict[str, list[dict[str, Any]]] = {}
    for alias_id, alias in aliases.items():
        selectors = alias.get("retail_target_selectors")
        if not isinstance(selectors, Mapping):
            raise ProgressError(f"authored ICF logical member {alias_id!r} has no selectors")
        expected_selectors: set[tuple[str, str, str, int, str | None]] = {
            ("direct-call", str(call_site), "", -1, None)
            for call_site in selectors.get("direct_call_sites", [])
        }
        expected_selectors.update(
            (
                "vtable-entry",
                "",
                str(selector.get("storage_identity", "")),
                int(selector.get("slot_index", -1)),
                (
                    str(selector.get("entry_address"))
                    if selector.get("entry_address") is not None
                    else None
                ),
            )
            for selector in selectors.get("vtable_entries", [])
            if isinstance(selector, Mapping)
        )
        raw_observations = raw_bindings[alias_id]
        if not isinstance(raw_observations, list) or not raw_observations:
            raise ProgressError(
                f"authored ICF selector_bindings[{alias_id!r}] must be a non-empty array"
            )
        observations: list[dict[str, Any]] = []
        observed_selectors: set[tuple[str, str, str, int, str | None]] = set()
        for index, raw_observation in enumerate(raw_observations):
            label = f"authored ICF selector_bindings[{alias_id!r}][{index}]"
            observation = _exact_mapping(
                raw_observation,
                {
                    "selector_kind",
                    "direct_call_site",
                    "storage_identity",
                    "slot_index",
                    "entry_address",
                    "object_path",
                    "object_report_path",
                    "relocation_partition_complete",
                    "relocations",
                },
                label=label,
            )
            kind = observation["selector_kind"]
            if kind == "direct-call":
                call_site = normalize_address(str(observation["direct_call_site"]))
                if any(
                    observation[field] is not None
                    for field in ("storage_identity", "slot_index", "entry_address")
                ):
                    raise ProgressError(f"{label} direct-call requires null vtable fields")
                selector_key = (kind, call_site, "", -1, None)
                observation["direct_call_site"] = call_site
            elif kind == "vtable-entry":
                if observation["direct_call_site"] is not None:
                    raise ProgressError(f"{label} vtable-entry requires direct_call_site=null")
                storage_identity = _string(
                    observation["storage_identity"], label=f"{label}.storage_identity"
                )
                slot_index = observation["slot_index"]
                if not isinstance(slot_index, int) or isinstance(slot_index, bool) or slot_index < 0:
                    raise ProgressError(f"{label}.slot_index must be non-negative")
                entry_address = observation["entry_address"]
                if entry_address is not None:
                    entry_address = normalize_address(str(entry_address))
                selector_key = (kind, "", storage_identity, slot_index, entry_address)
                observation["storage_identity"] = storage_identity
                observation["entry_address"] = entry_address
            else:
                raise ProgressError(
                    f"{label}.selector_kind must be direct-call or vtable-entry"
                )
            if selector_key in observed_selectors:
                raise ProgressError(f"{label} duplicates a selector observation")
            observed_selectors.add(selector_key)
            object_path = _string(
                observation["object_path"], label=f"{label}.object_path"
            ).replace("\\", "/")
            if (
                Path(object_path).is_absolute()
                or ".." in Path(object_path).parts
                or Path(object_path).suffix.lower() != ".obj"
            ):
                raise ProgressError(f"{label}.object_path must name a repository-relative OBJ")
            observation["object_path"] = object_path
            observation["object_report_path"] = _repository_report_path(
                observation["object_report_path"], label=f"{label}.object_report_path"
            )
            if observation["relocation_partition_complete"] is not True:
                raise ProgressError(f"{label} requires relocation_partition_complete=true")
            raw_relocations = observation["relocations"]
            if not isinstance(raw_relocations, list) or not raw_relocations:
                raise ProgressError(f"{label}.relocations must be a non-empty array")
            relocations: list[dict[str, Any]] = []
            offsets: set[int] = set()
            for relocation_index, raw_relocation in enumerate(raw_relocations):
                relocation_label = f"{label}.relocations[{relocation_index}]"
                relocation = _exact_mapping(
                    raw_relocation,
                    {"offset", "type", "target_logical_identity", "target_object_symbol", "addend"},
                    label=relocation_label,
                )
                offset = relocation["offset"]
                addend = relocation["addend"]
                if not isinstance(offset, int) or isinstance(offset, bool) or offset < 0:
                    raise ProgressError(f"{relocation_label}.offset must be non-negative")
                if offset in offsets:
                    raise ProgressError(f"{label}.relocations has duplicate offsets")
                if not isinstance(addend, int) or isinstance(addend, bool):
                    raise ProgressError(f"{relocation_label}.addend must be an integer")
                if relocation["target_logical_identity"] != alias_id:
                    raise ProgressError(
                        f"{relocation_label} must target logical member {alias_id!r}"
                    )
                if relocation["target_object_symbol"] != alias.get("object_symbol"):
                    raise ProgressError(
                        f"{relocation_label} target symbol does not match its logical member"
                    )
                relocation["type"] = _string(
                    relocation["type"], label=f"{relocation_label}.type"
                )
                offsets.add(offset)
                relocations.append(relocation)
            observation["relocations"] = relocations
            observations.append(observation)
        if observed_selectors != expected_selectors:
            raise ProgressError(
                f"authored ICF selector bindings for {alias_id!r} do not exactly match "
                "its immutable-retail selector population"
            )
        selector_bindings[str(alias_id)] = observations

    noicf = _link_observation(
        candidate["noicf_link"],
        label="authored ICF proof.candidate_mechanism.noicf_link",
        profile=AUTHORED_ICF_NOICF_PROFILE,
        required_flag="/OPT:NOICF",
        forbidden_flag="/OPT:ICF",
        alias_ids=alias_ids,
    )
    icf = _link_observation(
        candidate["icf_link"],
        label="authored ICF proof.candidate_mechanism.icf_link",
        profile=AUTHORED_ICF_ICF_PROFILE,
        required_flag="/OPT:ICF",
        forbidden_flag="/OPT:NOICF",
        alias_ids=alias_ids,
    )
    noicf_addresses = set(noicf["member_addresses"].values())
    icf_addresses = set(icf["member_addresses"].values())
    if len(noicf_addresses) != len(alias_ids):
        raise ProgressError("authored ICF NOICF proof must split every logical member")
    if len(icf_addresses) != 1:
        raise ProgressError("authored ICF ICF proof must fold every logical member")

    control_fields = {
        "role",
        "object_symbol",
        "object_path",
        "object_report_path",
        "comdat_eligible",
        "noicf_address",
        "icf_address",
        "folded_with_members",
        "fold_exclusion_proof",
    }
    if requires_comdat_selection:
        control_fields.add("comdat_selection")
    control = _exact_mapping(
        candidate["negative_control"],
        control_fields,
        label="authored ICF proof.candidate_mechanism.negative_control",
    )
    if control["role"] != "base-implementation":
        raise ProgressError("authored ICF negative control role must be 'base-implementation'")
    control["object_symbol"] = _decorated_symbol(
        control["object_symbol"],
        label="authored ICF proof.candidate_mechanism.negative_control.object_symbol",
    )
    if control["object_symbol"] in object_symbols:
        raise ProgressError("authored ICF negative control must be distinct from every member")
    control_object_path = _string(
        control["object_path"],
        label="authored ICF proof.candidate_mechanism.negative_control.object_path",
    ).replace("\\", "/")
    if (
        Path(control_object_path).is_absolute()
        or ".." in Path(control_object_path).parts
        or Path(control_object_path).suffix.lower() != ".obj"
        or control_object_path in object_paths
    ):
        raise ProgressError(
            "authored ICF negative control must name a distinct repository-relative OBJ"
        )
    control["object_path"] = control_object_path
    control["object_report_path"] = _repository_report_path(
        control["object_report_path"],
        label="authored ICF proof.candidate_mechanism.negative_control.object_report_path",
    )
    if not isinstance(control["comdat_eligible"], bool):
        raise ProgressError("authored ICF negative control comdat_eligible must be boolean")
    if requires_comdat_selection:
        control["comdat_selection"] = _comdat_selection(
            control["comdat_selection"],
            comdat_eligible=control["comdat_eligible"],
            label=(
                "authored ICF proof.candidate_mechanism.negative_control."
                "comdat_selection"
            ),
        )
    control["noicf_address"] = normalize_address(str(control["noicf_address"]))
    control["icf_address"] = normalize_address(str(control["icf_address"]))
    if control["folded_with_members"] is not False:
        raise ProgressError("authored ICF negative control must remain outside the fold group")
    if (
        control["noicf_address"] in noicf_addresses
        or control["icf_address"] in icf_addresses
    ):
        raise ProgressError("authored ICF negative control collides with a member address")

    exclusion = _exact_mapping(
        control["fold_exclusion_proof"],
        {
            "control_section_length",
            "member_section_lengths",
            "raw_fold_relevant_bytes_equal",
            "relocation_partitions_equal",
            "associative_sections_equal",
            "difference_reasons",
        },
        label="authored ICF proof.candidate_mechanism.negative_control.fold_exclusion_proof",
    )
    control_length = exclusion["control_section_length"]
    if not isinstance(control_length, int) or isinstance(control_length, bool) or control_length <= 0:
        raise ProgressError("authored ICF negative control section length must be positive")
    raw_member_lengths = exclusion["member_section_lengths"]
    if not isinstance(raw_member_lengths, Mapping) or set(raw_member_lengths) != alias_ids:
        raise ProgressError("authored ICF negative control requires exact member section lengths")
    member_lengths: dict[str, int] = {}
    for alias_id, raw_length in raw_member_lengths.items():
        if not isinstance(raw_length, int) or isinstance(raw_length, bool) or raw_length <= 0:
            raise ProgressError("authored ICF member section lengths must be positive")
        member_lengths[str(alias_id)] = raw_length
    boolean_fields = (
        "raw_fold_relevant_bytes_equal",
        "relocation_partitions_equal",
        "associative_sections_equal",
    )
    if any(not isinstance(exclusion[field], bool) for field in boolean_fields):
        raise ProgressError("authored ICF negative-control fold comparisons must be boolean")
    reasons = _string_array(
        exclusion["difference_reasons"],
        label="authored ICF negative control difference_reasons",
    )
    demonstrated_difference = bool(
        control["comdat_eligible"] is False
        or any(length != control_length for length in member_lengths.values())
        or any(exclusion[field] is False for field in boolean_fields)
        or (
            requires_comdat_selection
            and any(
                member["comdat_selection"] != control["comdat_selection"]
                for member in members.values()
            )
        )
    )
    if not demonstrated_difference:
        raise ProgressError(
            "authored ICF negative control requires a demonstrated fold-relevant difference"
        )
    exclusion["member_section_lengths"] = member_lengths
    exclusion["difference_reasons"] = reasons
    control["fold_exclusion_proof"] = exclusion

    candidate["object_members"] = members
    candidate["selector_bindings"] = selector_bindings
    candidate["noicf_link"] = noicf
    candidate["icf_link"] = icf
    candidate["negative_control"] = control
    proof["retail_truth"] = retail
    proof["candidate_mechanism"] = candidate
    return proof


def authored_icf_vtable_selector_index(
    symbols: Mapping[str, Any],
) -> dict[tuple[str, int], str]:
    """Return fail-closed reviewed vtable selector -> logical identity mappings."""

    result: dict[tuple[str, int], str] = {}
    for symbol in symbols.values():
        if not isinstance(symbol, Mapping):
            continue
        group = symbol.get("icf_address_group")
        aliases = symbol.get("logical_aliases")
        if (
            not isinstance(group, Mapping)
            or group.get("model") != AUTHORED_ICF_GROUP_MODEL
            or not isinstance(aliases, Mapping)
        ):
            continue
        for alias_id, alias in aliases.items():
            if not isinstance(alias, Mapping):
                continue
            selectors = alias.get("retail_target_selectors")
            if not isinstance(selectors, Mapping):
                raise ProgressError(f"authored ICF logical member {alias_id!r} has no selectors")
            for selector in selectors.get("vtable_entries", []):
                if not isinstance(selector, Mapping):
                    raise ProgressError(f"authored ICF logical member {alias_id!r} has invalid vtable selector")
                # Tracker selector facts use source-level x86 vtable slot indexes;
                # call-contract extraction records the byte displacement.
                key = (
                    str(selector.get("storage_identity", "")),
                    int(selector.get("slot_index", -1)) * 4,
                )
                identity = f"logical:{alias_id}"
                prior = result.get(key)
                if prior not in {None, identity}:
                    raise ProgressError(
                        f"authored ICF vtable selector {key!r} selects conflicting logical identities"
                    )
                result[key] = identity
    return result


def audit_authored_icf_groups(data: Mapping[str, Any]) -> list[tuple[str, str]]:
    """Return fail-closed authored-ICF finding code/message pairs."""

    findings: list[tuple[str, str]] = []
    symbols = data.get("symbols")
    owners = data.get("owners")
    evidence = data.get("evidence")
    if not isinstance(symbols, Mapping):
        return [("authored-icf.symbols", "symbols collection must be an object")]
    if not isinstance(owners, Mapping) or not isinstance(evidence, Mapping):
        return [
            (
                "authored-icf.collections",
                "authored ICF audit requires owner and evidence collections",
            )
        ]
    for symbol_id, symbol in symbols.items():
        if not isinstance(symbol, Mapping):
            continue
        group = symbol.get("icf_address_group")
        if not isinstance(group, Mapping) or group.get("model") != AUTHORED_ICF_GROUP_MODEL:
            continue
        label = str(symbol_id)
        aliases = symbol.get("logical_aliases")
        try:
            if group.get("source_model") not in {None, AUTHORED_ICF_LIFECYCLE_SOURCE_MODEL}:
                raise ProgressError("authored ICF source model is unsupported")
            if group.get("physical_gate_symbol_id") != symbol_id:
                raise ProgressError("physical gate symbol id is stale")
            if (
                symbol.get("pipeline_class"),
                symbol.get("authored_order_role"),
            ) not in {
                ("authored", "authored-body"),
                ("authored-lifecycle", "authored-lifecycle-body"),
            }:
                raise ProgressError("physical row is not one authored gating body")
            if not isinstance(aliases, Mapping) or len(aliases) < 2:
                raise ProgressError("logical member population is incomplete")
            owner_ids: list[str] = []
            source_edges: set[tuple[str, str]] = set()
            direct_sites: set[str] = set()
            for alias_id, alias in aliases.items():
                if not isinstance(alias, Mapping):
                    raise ProgressError(f"logical member {alias_id!r} is not an object")
                lifecycle = group.get("source_model") == AUTHORED_ICF_LIFECYCLE_SOURCE_MODEL
                if lifecycle != isinstance(alias.get("source_generation"), Mapping):
                    raise ProgressError("authored ICF source generation requires the lifecycle model on every member")
                if alias.get("gate_mode") != AUTHORED_ICF_MEMBER_GATE_MODE:
                    raise ProgressError(f"logical member {alias_id!r} is a duplicate gate")
                owner_id = str(alias.get("owner_id", ""))
                if not owner_id:
                    raise ProgressError(f"logical member {alias_id!r} has no owner edge")
                owner_ids.append(owner_id)
                source_trace = alias.get("source_traceability")
                if not isinstance(source_trace, Mapping):
                    raise ProgressError(f"logical member {alias_id!r} has no source trace")
                raw_edges = source_trace.get("source_edges")
                if source_trace.get("state") != "resolved" or not isinstance(raw_edges, list) or len(raw_edges) != 1:
                    raise ProgressError(f"logical member {alias_id!r} source edge is not exclusive")
                edge = raw_edges[0]
                expected_relation = (
                    "emits" if lifecycle and alias.get("source_generation", {}).get("kind") == "implicit-destructor"
                    else "defines"
                )
                if not isinstance(edge, Mapping) or edge.get("relation") != expected_relation:
                    raise ProgressError(f"logical member {alias_id!r} lacks a defining edge")
                context = edge.get("emission_context")
                edge_key = (
                    str(edge.get("anchor_id", "")),
                    str(context.get("translation_unit", "")) if isinstance(context, Mapping) else "",
                )
                if not all(edge_key) or edge_key in source_edges:
                    raise ProgressError("logical member source edges are missing or overlapping")
                source_edges.add(edge_key)
                selectors = alias.get("retail_target_selectors")
                if not isinstance(selectors, Mapping):
                    raise ProgressError(f"logical member {alias_id!r} has no retail selectors")
                for raw_site in selectors.get("direct_call_sites", []):
                    site = normalize_address(str(raw_site))
                    if site in direct_sites:
                        raise ProgressError(f"retail call selector {site} is ambiguous")
                    direct_sites.add(site)
            if group.get("source_model") != AUTHORED_ICF_LIFECYCLE_SOURCE_MODEL and len(owner_ids) != len(set(owner_ids)):
                raise ProgressError("logical member owner edges are not exclusive")
            for owner_id in owner_ids:
                owner = owners.get(owner_id)
                gates = owner.get("gates") if isinstance(owner, Mapping) else None
                if (
                    not isinstance(owner, Mapping)
                    or owner.get("binary") != "recoil"
                    or owner.get("kind") == "provider-boundary"
                    or owner.get("provider_state")
                    in {"accepted", "provider-boundary", "provider-owned"}
                    or not isinstance(gates, Mapping)
                    or gates.get("source") != "accepted"
                    or gates.get("owner_linkage") != "accepted"
                ):
                    raise ProgressError(
                        f"logical member owner {owner_id!r} is absent, provider-owned, or stale"
                    )
            validate_authored_icf_source_mirrors(aliases)
            primary_owner_ids: list[str] = []
            for owner_id, owner in owners.items():
                if not isinstance(owner, Mapping):
                    continue
                relationships = owner.get("relationships")
                if not isinstance(relationships, list):
                    continue
                if any(
                    isinstance(relationship, Mapping)
                    and relationship.get("kind") == "primary-function"
                    and relationship.get("symbol_id") == symbol_id
                    for relationship in relationships
                ):
                    primary_owner_ids.append(str(owner_id))
            if len(primary_owner_ids) != 1 or primary_owner_ids[0] not in owner_ids:
                raise ProgressError("physical primary ownership is not exact and address-exclusive")
            evidence_ids = group.get("evidence_ids")
            if not isinstance(evidence_ids, list) or len(evidence_ids) != 1:
                raise ProgressError("group requires one current governed evidence record")
            evidence_row = evidence.get(evidence_ids[0])
            provenance = evidence_row.get("provenance") if isinstance(evidence_row, Mapping) else None
            if (
                not isinstance(evidence_row, Mapping)
                or evidence_row.get("kind") != "authored-icf-logical-member-review"
                or evidence_row.get("freshness") != "current"
                or evidence_row.get("validation_mode") != "live"
                or not isinstance(provenance, Mapping)
                or provenance.get("evidence_contract")
                != "authored-linker-coalesced-logical-members-v1"
            ):
                raise ProgressError("group evidence is absent, stale, or has the wrong contract")
            validate_authored_icf_proof(
                provenance.get("authored_icf_proof"),
                physical_address=str(symbol.get("address", "")),
                aliases=aliases,
            )
        except (ProgressError, TypeError, ValueError) as exc:
            findings.append(("authored-icf.invalid", f"{label}: {exc}"))
    try:
        authored_icf_vtable_selector_index(symbols)
    except (ProgressError, TypeError, ValueError) as exc:
        findings.append(("authored-icf.vtable-selector", str(exc)))
    return findings


def require_valid_authored_icf_groups(data: Mapping[str, Any]) -> None:
    findings = audit_authored_icf_groups(data)
    if findings:
        code, message = findings[0]
        raise ProgressError(f"{code}: {message}")
