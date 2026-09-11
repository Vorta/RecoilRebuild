"""Site-bound logical call eligibility; no order, owner-gate or byte authority."""
from __future__ import annotations

from copy import deepcopy
from pathlib import Path
import re
from typing import Mapping

from _recoil.lib.progress import ProgressError, address_value, is_current_accepted_state
from _recoil.lib.tooling import REPO_ROOT

SCHEMA = "winner-unknown-icf-call-only-extension-v1"
FIELD = "call_only_extension"
ACCEPTED = ["logical-call-contract", "semantic-camera-owner"]
WITHHELD = ["original-spelling", "original-definition-location", "authored-order",
            "icf-winner", "object-bytes", "linked-bytes", "owner-tier",
            "full-source-provenance", "full-linkage-provenance"]
FIELDS = {"schema", "reviewed", "physical_symbol_id", "physical_address", "logical_id",
          "owner_id", "object_symbol", "caller_id", "caller_address", "caller_end_exclusive",
          "caller_object_symbol", "call_address", "operand_address", "abi", "source_traceability",
          "review_evidence_id", "review_evidence", "group_context", "accepted_dimensions",
          "withheld_dimensions", "candidate_independent", "candidate_output_used",
          "membership_exhaustive", "order_authority"}


def require(condition, message):
    if not condition:
        raise ProgressError("call-only ICF: " + message)


def group_context(symbol):
    """Retain typed group topology, excluding stage result and this eligibility."""
    keys = ("address", "end_exclusive", "size", "binary", "kind", "pipeline_class",
            "authored_order_role", "physical_block_id", "icf_address_group", "logical_aliases")
    result = {key: deepcopy(symbol.get(key)) for key in keys}
    require(isinstance(result["logical_aliases"], Mapping), "missing physical alias population")
    for alias in result["logical_aliases"].values():
        alias.pop(FIELD, None)
    return result


def validate_inventory(data, validated_by_site):
    """Every stored occurrence must have passed the complete group authority path."""
    expected = {}
    for site, contract in validated_by_site.items():
        key = (contract["physical_symbol_id"], contract["logical_id"])
        require(key not in expected and site == contract["call_address"], "duplicate validated contract")
        expected[key] = contract
    found = {}
    for physical_id, row in data["symbols"].items():
        require(FIELD not in row, "contract attached to a physical symbol instead of an alias")
        for logical_id, alias in row.get("logical_aliases", {}).items():
            if FIELD in alias:
                key = (physical_id, logical_id)
                require(key in expected and alias[FIELD] == expected[key],
                        "orphan, skipped or misattached call-only contract")
                found[key] = alias[FIELD]
    require(found == expected, "validated contract has no exact stored occurrence")


def validate_contract(data, contract):
    require(isinstance(contract, Mapping) and set(contract) == FIELDS, "exact typed contract required")
    require(contract["schema"] == SCHEMA and contract["reviewed"] is True,
            "unknown or unreviewed contract")
    require(contract["accepted_dimensions"] == ACCEPTED and contract["withheld_dimensions"] == WITHHELD
            and contract["candidate_independent"] is True and contract["candidate_output_used"] is False
            and contract["membership_exhaustive"] is False and contract["order_authority"] is False,
            "authority dimensions differ")
    physical_id, logical_id = contract["physical_symbol_id"], contract["logical_id"]
    physical = data["symbols"].get(physical_id, {})
    require(group_context(physical) == contract["group_context"], "physical group context changed")
    group = physical.get("icf_address_group", {})
    require(physical.get("address") == contract["physical_address"]
            and physical.get("pipeline_class") == "non-authored"
            and physical.get("authored_order_role") == "compiler-generated-icf-representative"
            and group.get("winner_status") == "winner-unknown"
            and group.get("winner_identity_key") is None
            and not physical.get("linked_provider_binding"), "physical group has conflicting authority")
    alias = physical["logical_aliases"].get(logical_id, {})
    require(alias.get("original_name_status") == "provisional"
            and alias.get("pipeline_class") == "authored"
            and alias.get("authored_order_role") == "authored-body"
            and alias.get("fold_status") == "proven-fold-alias"
            and alias.get("source_owner_status") == "authored-owner"
            and alias.get("owner_id") == contract["owner_id"]
            and alias.get("object_symbol") == contract["object_symbol"]
            and alias.get("source_traceability") == contract["source_traceability"]
            and not alias.get("linked_provider_binding"), "logical identity or source context changed")
    owner = data["owners"].get(contract["owner_id"], {})
    require(owner.get("binary") == "recoil" and owner.get("kind") != "provider-boundary"
            and owner.get("provider_state") in {"pending", "unresolved", "non-provider"}
            and not owner.get("linked_provider_binding")
            and all(owner.get("gates", {}).get(key) in {"pending", "accepted"}
                    for key in ("source", "owner_linkage")), "owner has conflicting source/provider status")
    evidence_id = contract["review_evidence_id"]
    evidence = data["evidence"].get(evidence_id, {})
    require(evidence == contract["review_evidence"] and is_current_accepted_state(evidence)
            and evidence.get("gating") is True
            and evidence.get("kind") == "authored-order-icf-logical-alias-review"
            and evidence_id in alias.get("evidence_ids", []), "review is missing, changed or non-gating")
    provenance = evidence.get("provenance", {})
    context = provenance.get("validation_context", {})
    reviews = provenance.get("provisional_alias_reviews", {})
    require(set(reviews) == {logical_id} and reviews[logical_id].get("reviewed") is True
            and reviews[logical_id].get("original_spelling") == "unknown"
            and reviews[logical_id].get("spelling_kind") == "reconstruction-only"
            and provenance.get("candidate_independent") is True
            and context.get("candidate_output_used") is False
            and context.get("accepted_dimensions") == ACCEPTED
            and context.get("source_contract") == "void fastcall(saveState*)"
            and provenance.get("original_spelling") == "unknown"
            and provenance.get("original_definition_location") == "unknown"
            and {"original-spelling", "original-definition-location", "icf-winner",
                 "object-bytes", "linked-bytes", "owner-tier"}
                <= set(context.get("unaccepted_dimensions", []))
            and provenance.get("physical_target") == contract["physical_address"]
            and provenance.get("retail_call_site") == contract["call_address"]
            and provenance.get("retail_operand") == contract["operand_address"],
            "review does not select this exact logical call")
    require({physical_id, logical_id, contract["owner_id"]} <= set(evidence.get("scope_ids", [])),
            "review scope does not include group, alias and owner")
    abi = contract["abi"]
    require(isinstance(abi, Mapping) and set(abi) == {"convention", "return_type", "parameter_type",
            "parameter_register", "stack_argument_bytes", "caller_cleanup_bytes", "return_consumed",
            "form", "dispatch", "namespace", "function_name"}, "exact ABI required")
    require(abi["convention"] == "fastcall" and abi["return_type"] == "void"
            and abi["parameter_register"] == "ecx" and abi["stack_argument_bytes"] == 0
            and abi["caller_cleanup_bytes"] == 0 and abi["return_consumed"] is False
            and type(abi["stack_argument_bytes"]) is int and type(abi["caller_cleanup_bytes"]) is int
            and abi["form"] == "call" and abi["dispatch"] == "direct"
            and all(isinstance(abi[key], str) and abi[key].isidentifier()
                    for key in ("namespace", "function_name", "parameter_type")),
            "unsupported or changed call ABI")
    expected_name = f'?{abi["function_name"]}@{abi["namespace"]}@@YIXPAU{abi["parameter_type"]}@@@Z'
    require(contract["object_symbol"] == expected_name, "decorated symbol disagrees with namespace/ABI")
    caller = data["symbols"].get(contract["caller_id"], {})
    require(caller.get("address") == contract["caller_address"]
            and caller.get("binary") == "recoil" and caller.get("kind") == "function"
            and caller.get("extent_state") == "known"
            and type(caller.get("size")) is int and caller["size"] > 0
            and caller["size"] == address_value(contract["caller_end_exclusive"]) - address_value(contract["caller_address"])
            and caller.get("end_exclusive") == contract["caller_end_exclusive"]
            and caller.get("pipeline_class") in {"authored", "authored-lifecycle"}
            and not caller.get("linked_provider_binding")
            and address_value(contract["caller_address"]) <= address_value(contract["call_address"])
            < address_value(contract["caller_end_exclusive"])
            and address_value(contract["operand_address"]) == address_value(contract["call_address"]) + 1,
            "caller identity or operand changed")
    for owner_id, other_owner in data["owners"].items():
        for relationship in other_owner.get("relationships", []):
            if relationship.get("symbol_id") == logical_id or relationship.get("logical_identity_key") == logical_id:
                require(owner_id == contract["owner_id"], "logical identity has another owner")
    # No identity outside this exact alias may claim its reconstruction name.
    for sid, row in data["symbols"].items():
        for rid, member in [(sid, row), *row.get("logical_aliases", {}).items()]:
            name = member.get("object_symbol", "")
            require(not isinstance(name, str) or name.casefold() != expected_name.casefold()
                    or (sid == physical_id and rid == logical_id and name == expected_name),
                    "duplicate or case-conflicting object identity")
            require(rid != logical_id or sid == physical_id, "duplicate logical identity")
    # The call extension must never be selected by any order target/fact.
    def reject_order(value):
        if isinstance(value, Mapping):
            require(value.get("logical_identity_key") != logical_id,
                    "call-only alias leaked into an order row")
            for key, item in value.items():
                if key in {"matched_identities", "accepted_occurrences"}:
                    require(logical_id not in str(item), "call-only alias leaked into accepted order")
                reject_order(item)
        elif isinstance(value, (list, tuple)):
            for item in value:
                reject_order(item)
    reject_order(data.get("verification_targets", {}))
    reject_order(data.get("physical_blocks", {}))
    return alias


def validate_caller_authority(data, contract):
    """Use the accepted caller's synchronized order target, never candidate names."""
    from _recoil.call_contract.identity import (_mapping_target_function_rows,
        _mapping_target_function_rows_with_views, _finite_literal_symbol_regex_alternatives)
    from _recoil.lib.progress import normalize_address
    from _recoil.lib.verification_targets import vc5_target_registration
    caller_id = contract["caller_id"]
    caller = data["symbols"][caller_id]
    block_id = caller.get("physical_block_id")
    block = data["physical_blocks"].get(block_id, {})
    facts = block.get("accepted_order_facts", {})
    target_id = facts.get("target_id")
    require(target_id in caller.get("verification_target_ids", []),
            "caller is absent from current target membership")
    require(facts.get("phase") == "authored-function-order"
            and facts.get("validation_mode") == "live"
            and facts.get("matched_identities", []).count(caller_id) == 1
            and facts.get("covered_block_ids", []).count(block_id) == 1
            and block.get("order", {}).get("authored")
            and all(is_current_accepted_state(state)
                    for state in block["order"]["authored"].values()), "caller order authority is not current")
    target = data["verification_targets"].get(target_id, {})
    require(target.get("binary") == "recoil" and target.get("kind") == "vc5",
            "caller has no governed target")
    registration = target.get("registration", {})
    manifest = registration.get("manifest_path")
    require(isinstance(manifest, str) and manifest, "caller target manifest is missing")
    path = (REPO_ROOT / manifest).resolve()
    require(path.is_relative_to((REPO_ROOT / 'tools/vc5_verify_targets').resolve()),
            "caller target manifest is outside the governed directory")
    current_id, current = vc5_target_registration(path)
    require(current_id == target_id and current.get("registration") == registration,
            "caller target registration is stale")
    require(all(current.get(key) == target.get(key) for key in ("binary", "kind", "name"))
            and isinstance(target.get("name"), str) and target["name"]
            and current["registration"].get("manifest_path") == manifest,
            "caller current target metadata differs")
    address = normalize_address(contract["caller_address"])

    def caller_rows(value):
        rows = [(view, {**row, "address": normalize_address(row["address"])})
                for view, row in _mapping_target_function_rows_with_views(value)
                if isinstance(row.get("address"), str)
                and normalize_address(row["address"]) == address]
        # A single semantic row may be mirrored once in each recognized view.
        require(rows and len({view for view, _row in rows}) == len(rows)
                and all(view in {"functions", "translation_unit_function_order", "linked_function_intervals"}
                        and row == rows[0][1] for view, row in rows),
                "caller row population is missing, duplicated or conflicting")
        return sorted(rows, key=lambda item: item[0])

    stored_rows, current_rows = caller_rows(target), caller_rows(current)
    require(current_rows == stored_rows, "caller current row population differs")
    row = current_rows[0][1]
    require(row.get("symbol") == contract["caller_object_symbol"]
            and not row.get("symbol_regex")
            and row.get("logical_identity_key") in {None, "", caller_id}
            and row.get("pipeline_class") == caller.get("pipeline_class")
            and row.get("authored_order_gate") is True
            and row.get("required_presence") is True, "caller symbol differs from its accepted target")
    name = contract["caller_object_symbol"]
    for other in data["verification_targets"].values():
        for claim in _mapping_target_function_rows(other):
            other_name, pattern = claim.get("symbol"), claim.get("symbol_regex")
            alternatives = _finite_literal_symbol_regex_alternatives(pattern) or ()
            names = [other_name, *alternatives]
            matches = any(isinstance(item, str) and item.casefold() == name.casefold() for item in names)
            if isinstance(pattern, str) and pattern:
                try:
                    matches |= re.fullmatch(pattern, name, re.IGNORECASE) is not None
                except re.error as exc:
                    require(False, "invalid competing caller regex: " + str(exc))
            if matches:
                # Separate order scopes can use different gate flags, but may
                # only mirror the exact callable identity and classification.
                require(other.get("binary") == "recoil" and other.get("kind") == "vc5"
                        and other_name == name and not pattern
                        and normalize_address(claim.get("address", "")) == address
                        and claim.get("logical_identity_key") in {None, "", caller_id}
                        and all(claim.get(key) == row.get(key) for key in
                                ("pipeline_class", "authored_order_role", "required_presence",
                                 "icf_fold_status", "linked_provider_binding")),
                        "caller symbol has conflicting registered identities")
    for sid, row in data["symbols"].items():
        for identity, member in [(sid, row), *row.get("logical_aliases", {}).items()]:
            other_name = member.get("object_symbol")
            if isinstance(other_name, str) and other_name.casefold() == name.casefold():
                require(identity == caller_id and other_name == name, "caller symbol has another typed identity")


def validate_source(data, contract, *, root=REPO_ROOT):
    """Current placement only; the compiler separately proves C++ identity/ABI."""
    from _recoil.lib.source_traceability import parse_source_trace_text, SOURCE_TRACE_SUFFIXES
    from _recoil.commands.vc5_build import load_config
    trace = contract["source_traceability"]
    edges = trace.get("source_edges", [])
    require(trace.get("state") == "resolved" and len(edges) == 1
            and edges[0].get("relation") == "defines", "one resolved defining edge required")
    edge = edges[0]
    path = edge.get("emission_context", {}).get("translation_unit")
    require(isinstance(path, str) and path.startswith("src/") and "\\" not in path
            and ".." not in Path(path).parts and Path(path).suffix == ".cpp"
            and (root / path).resolve().is_relative_to((root / "src").resolve()), "invalid current TU")
    require(path in data["owners"][contract["owner_id"]].get("source_paths", []),
            "defining path is absent from current owner")
    require((root / path).resolve() in {p.resolve() for p in load_config().sources},
            "defining TU is not in the canonical production build")
    anchors, artifacts = [], []
    for source in (root / "src").rglob("*"):
        if source.suffix.lower() not in SOURCE_TRACE_SUFFIXES or not source.is_file():
            continue
        text = source.read_text(encoding="utf-8")
        if edge["anchor_id"] not in text and contract["logical_id"] not in text:
            continue
        parsed = parse_source_trace_text(text, path=source.relative_to(root).as_posix())
        require(not parsed.findings, "invalid matching source directives")
        anchors.extend(a for a in parsed.anchors if a.anchor_id == edge["anchor_id"])
        artifacts.extend((a, text) for a in parsed.artifacts if a.artifact_id == contract["logical_id"])
    require(len(anchors) == len(artifacts) == 1, "anchor/artifact is not repository-wide unique")
    artifact, text = artifacts[0]
    require(anchors[0].path == artifact.path and anchors[0].construct == artifact.construct,
            "anchor and artifact do not attach to the same source construct")
    require(artifact.path == path and artifact.anchor_id == edge["anchor_id"]
            and artifact.relation == "defines" and artifact.section == ".text"
            and artifact.direct and artifact.construct is not None
            and artifact.construct.kind == "function", "annotation is not attached to its function")
    abi = contract["abi"]
    require(qualified_function_name(text, artifact.construct) == abi["namespace"] + "::" + abi["function_name"],
            "source definition namespace differs")
    from _recoil.lib.source_constructs import parse_source_constructs
    constructs = parse_source_constructs(text)
    require(len([item for item in constructs if item.kind == "function"
                 and item.name.split("::")[-1] == abi["function_name"]]) == 1,
            "function basename is ambiguous in the defining TU")
    signature = text[artifact.construct.start:artifact.construct.end].split("{", 1)[0]
    require(re.fullmatch(r'\s*void\s+__fastcall\s+' + re.escape(abi["function_name"])
            + r'\s*\(\s*' + re.escape(abi["parameter_type"])
            + r'\s*\*\s*(?:[A-Za-z_]\w*)?\s*\)\s*', signature) is not None,
            "attached definition has the wrong function signature")
    return path


def qualified_function_name(text, construct):
    """Recognize VC5 named namespace scopes, retaining lexical brace boundaries."""
    from _recoil.lib.source_constructs import mask_comments_and_literals, matching_brace
    masked = mask_comments_and_literals(text)
    namespaces = []
    for match in re.finditer(r'\b(namespace|class|struct)\s*([A-Za-z_]\w*)?\s*\{', masked):
        opening = match.end() - 1
        if opening < construct.start < matching_brace(masked, opening):
            require(match.group(2) is not None, "anonymous source scope is not eligible")
            namespaces.append(match.group(2))
    return '::'.join([*namespaces, construct.name])


def prove_unique_build_definitions(data, build_root):
    """Whole-program closeout proves one raw definition in the canonical TU."""
    from _recoil.commands.asm_verify import CoffObject
    from _recoil.commands.vc5_build import safe_object_stem
    contracts = [alias[FIELD] for row in data["symbols"].values()
                 for alias in row.get("logical_aliases", {}).values() if FIELD in alias]
    if not contracts:
        return
    wanted = {contract["object_symbol"].casefold(): contract for contract in contracts}
    found = {key: [] for key in wanted}
    objects = list((build_root / 'obj').rglob('*.obj'))
    require(objects, "whole-program object census is missing")
    for path in objects:
        obj = CoffObject.from_path(path)
        for symbol in obj.symbols:
            key = symbol.name.casefold()
            if key in wanted and symbol.section_number > 0 and symbol.storage_class == 2:
                found[key].append((path.resolve(), symbol))
    for key, contract in wanted.items():
        source = contract["source_traceability"]["source_edges"][0]["emission_context"]["translation_unit"]
        expected = (build_root / 'obj' / safe_object_stem(REPO_ROOT / source)).resolve()
        require(len(found[key]) == 1 and found[key][0][0] == expected
                and found[key][0][1].name == contract["object_symbol"]
                and found[key][0][1].type == 0x20, "whole-program callback definition is missing or duplicated")
