from __future__ import annotations

from copy import deepcopy
from dataclasses import replace
import json
from pathlib import Path
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.authored_order import census  # noqa: E402
from _recoil.commands.call_contract_verify import (  # noqa: E402
    _canonical_direct_identity,
    build_identity_indexes,
)
from _recoil.commands.progress_cli import (  # noqa: E402
    _load_logical_alias_group_payload,
    _parse_logical_alias_group_payload,
    _set_symbol_logical_alias_group,
    _owner_view,
)
from _recoil.lib.progress import (  # noqa: E402
    ProgressDocument,
    ProgressError,
    logical_alias_authored_order_gate,
)
from _recoil.lib.authored_icf import (  # noqa: E402
    select_authored_icf_translation_unit_object_symbol,
    validate_authored_icf_physical_source_artifacts,
    validate_authored_icf_proof,
)
from _recoil.lib.source_traceability import parse_source_trace_path  # noqa: E402
from tests.tools.recoil_cli_tests import RecoilCliTests, empty_progress_document  # noqa: E402


def canonical_retail_reference() -> Path:
    return REPO_ROOT / "support" / "Recoil.exe"

class AuthoredIcfLogicalAliasTests(unittest.TestCase):
    symbol_id = "recoil:function:0x401000"
    block_id = "recoil:block:0x401000"
    alias_a = "recoil:logical-function:0x401000:mission-fmv-state-set-enabled"
    alias_b = "recoil:logical-function:0x401000:hud-background-set-enabled"
    owner_a = "recoil:owner:fixture.mission"
    owner_b = "recoil:owner:fixture.hud"

    def setUp(self) -> None:
        evidence_root = REPO_ROOT / "build" / "reconstruction-evidence" / "runs"
        evidence_root.mkdir(parents=True, exist_ok=True)
        self.tempdir = tempfile.TemporaryDirectory(
            prefix="authored-icf-test-", dir=evidence_root
        )
        self.report_paths: list[str] = []
        for name in (
            "member-a.json",
            "member-b.json",
            "ref-noicf.map",
            "ref-icf.map",
            "selector-a.json",
            "selector-b.json",
            "negative-control.json",
        ):
            path = Path(self.tempdir.name) / name
            path.write_text(f"governed authored ICF fixture: {name}\n", encoding="utf-8")
            self.report_paths.append(path.relative_to(REPO_ROOT).as_posix())
        self.source_repo = tempfile.TemporaryDirectory(prefix="authored-icf-source-")
        self.source_root = Path(self.source_repo.name)
        (self.source_root / "src").mkdir()
        self._write_source_mirrors()
        self.source_root_patch = patch(
            "_recoil.lib.authored_icf.REPO_ROOT", self.source_root
        )
        self.source_root_patch.start()

    def tearDown(self) -> None:
        self.source_root_patch.stop()
        self.source_repo.cleanup()
        self.tempdir.cleanup()

    def _write_source_mirrors(self) -> None:
        rows = (
            (
                "mission.cpp",
                "recoil:anchor:fixture-mission-set-enabled",
                self.alias_a,
                "MissionSetter",
            ),
            (
                "hud.cpp",
                "recoil:anchor:fixture-hud-set-enabled",
                self.alias_b,
                "HudSetter",
            ),
        )
        for filename, anchor_id, alias_id, function_name in rows:
            (self.source_root / "src" / filename).write_text(
                "/**\n"
                f" * @recoil-anchor {anchor_id}\n"
                f" * @recoil-artifact defines .text {alias_id}: Fixture authored member.\n"
                " * Purpose: Fixture source mirror for governed authored ICF tests.\n"
                " */\n"
                f"void {function_name}() {{}}\n",
                encoding="utf-8",
            )

    def document(self) -> dict[str, object]:
        data = empty_progress_document()
        data["revision"] = 9
        data["physical_blocks"][self.block_id] = {
            "binary": "recoil",
            "start": "0x401000",
            "end_exclusive": "0x401010",
            "source_path": "src/fixture.cpp",
            "agent_source_path": "src/fixture.cpp",
            "contribution_ids": [self.symbol_id],
            "semantic_span_ids": [],
            "order": {},
        }
        data["symbols"][self.symbol_id] = {
            "binary": "recoil",
            "kind": "function",
            "address": "0x401000",
            "end_exclusive": "0x401010",
            "navigation_name": "FoldedAuthoredBody",
            "pipeline_class": "authored",
            "authored_order_role": "authored-body",
            "ownership_state": "primary-owned",
            "physical_block_id": self.block_id,
            "semantic_span_ids": [],
        }
        for owner_id, relationships in (
            (
                self.owner_a,
                [
                    {
                        "kind": "primary-function",
                        "symbol_id": self.symbol_id,
                        "address": "0x401000",
                    }
                ],
            ),
            (self.owner_b, []),
        ):
            data["owners"][owner_id] = {
                "binary": "recoil",
                "kind": "class",
                "provider_state": "pending",
                "gates": {"source": "accepted", "owner_linkage": "accepted"},
                "relationships": relationships,
            }
        return data

    def payload(self) -> dict[str, object]:
        object_symbols = {
            self.alias_a: "?SetEnabled@MissionFmvState@@QAEXH@Z",
            self.alias_b: "?SetEnabled@HudBackground@@UAEXH@Z",
        }
        aliases = {
            self.alias_a: {
                "object_symbol": object_symbols[self.alias_a],
                "original_name": "MissionFmvState::SetEnabled",
                "original_name_status": "recovered",
                "source_owner_status": "authored-owner",
                "owner_id": self.owner_a,
                "pipeline_class": "authored",
                "authored_order_role": "authored-body",
                "fold_status": "proven-fold-alias",
                "gate_mode": "physical-body-only",
                "source_traceability": {
                    "state": "resolved",
                    "source_edges": [
                        {
                            "relation": "defines",
                            "anchor_id": "recoil:anchor:fixture-mission-set-enabled",
                            "emission_context": {"translation_unit": "src/mission.cpp"},
                            "evidence_ids": [],
                        }
                    ],
                    "reason_code": None,
                },
                "retail_target_selectors": {
                    "direct_call_sites": ["0x402000"],
                    "vtable_entries": [],
                },
            },
            self.alias_b: {
                "object_symbol": object_symbols[self.alias_b],
                "original_name": "HudBackground::SetEnabled",
                "original_name_status": "recovered",
                "source_owner_status": "authored-owner",
                "owner_id": self.owner_b,
                "pipeline_class": "authored",
                "authored_order_role": "authored-body",
                "fold_status": "proven-fold-alias",
                "gate_mode": "physical-body-only",
                "source_traceability": {
                    "state": "resolved",
                    "source_edges": [
                        {
                            "relation": "defines",
                            "anchor_id": "recoil:anchor:fixture-hud-set-enabled",
                            "emission_context": {"translation_unit": "src/hud.cpp"},
                            "evidence_ids": [],
                        }
                    ],
                    "reason_code": None,
                },
                "retail_target_selectors": {
                    "direct_call_sites": [],
                    "vtable_entries": [
                        {
                            "storage_identity": "storage:recoil:data:0x4f0000",
                            "slot_index": 1,
                            "entry_address": "0x4f0004",
                        }
                    ],
                },
            },
        }
        member_addresses_noicf = {
            self.alias_a: "0x510000",
            self.alias_b: "0x510020",
        }
        member_addresses_icf = {
            self.alias_a: "0x520000",
            self.alias_b: "0x520000",
        }
        object_members = {
            alias_id: {
                "object_path": f"build/vc5/{index}.obj",
                "object_symbol": object_symbols[alias_id],
                "comdat_eligible": True,
                "comdat_selection": 1,
                "definition_count": 1,
                "relocation_partition": [],
                "relocation_partition_complete": True,
                "object_report_path": self.report_paths[index],
            }
            for index, alias_id in enumerate((self.alias_a, self.alias_b))
        }
        selector_bindings = {
            self.alias_a: [
                {
                    "selector_kind": "direct-call",
                    "direct_call_site": "0x402000",
                    "storage_identity": None,
                    "slot_index": None,
                    "entry_address": None,
                    "object_path": "build/vc5/mission-caller.obj",
                    "object_report_path": self.report_paths[4],
                    "relocation_partition_complete": True,
                    "relocations": [
                        {
                            "offset": 3,
                            "type": "IMAGE_REL_I386_REL32",
                            "target_logical_identity": self.alias_a,
                            "target_object_symbol": object_symbols[self.alias_a],
                            "addend": 0,
                        }
                    ],
                }
            ],
            self.alias_b: [
                {
                    "selector_kind": "vtable-entry",
                    "direct_call_site": None,
                    "storage_identity": "storage:recoil:data:0x4f0000",
                    "slot_index": 1,
                    "entry_address": "0x4f0004",
                    "object_path": "build/vc5/hud-vtable.obj",
                    "object_report_path": self.report_paths[5],
                    "relocation_partition_complete": True,
                    "relocations": [
                        {
                            "offset": 4,
                            "type": "IMAGE_REL_I386_DIR32",
                            "target_logical_identity": self.alias_b,
                            "target_object_symbol": object_symbols[self.alias_b],
                            "addend": 0,
                        }
                    ],
                }
            ],
        }
        artifacts = [
            {
                "path": "support/Recoil.exe",
                "size": canonical_retail_reference().stat().st_size,
                "role": "immutable-retail-truth",
            },
            *[
                {
                    "path": path,
                    "size": (REPO_ROOT / path).stat().st_size,
                    "role": "candidate-mechanism-transcript",
                }
                for path in self.report_paths
            ],
        ]
        return {
            "schema": "recoil-logical-alias-group-v4",
            "reviewed": True,
            "reviewed": True,
            "reason": "Retail selectors and fresh VC5 diagnostics prove authored coalescing.",
            "symbol_id": self.symbol_id,
            "address": "0x401000",
            "current": {
                "pipeline_class": "authored",
                "authored_order_role": "authored-body",
                "physical_block_id": self.block_id,
                "linked_address_group": None,
                "icf_address_group": None,
                "logical_aliases": None,
            },
            "icf_address_group": {
                "model": "authored-linker-coalesced-v1",
                "physical_gate_symbol_id": self.symbol_id,
                "winner_status": "winner-unknown",
                "winner_identity_key": None,
            },
            "logical_aliases": aliases,
            "new_evidence": {
                "summary": "Immutable retail selection plus fresh VC5 fold mechanism proof.",
                "provenance": {
                    "candidate_independent_retail_truth": True,
                    "retail_source": "support/Recoil.exe and saved Binary Ninja evidence",
                },
                "artifacts": artifacts,
                "validation_context": {
                    "candidate_output_used_as_expected": False,
                    "candidate_output_used_for_mechanism_proof": True,
                },
            },
            "authored_icf_proof": {
                "schema": "recoil-authored-icf-proof-v2",
                "retail_truth": {
                    "candidate_independent": True,
                    "candidate_output_used_as_expected": False,
                    "physical_address": "0x401000",
                    "logical_identity_keys": [self.alias_a, self.alias_b],
                    "selector_source": "immutable-retail-assembly-xrefs-and-vtables",
                },
                "candidate_mechanism": {
                    "candidate_output_used": True,
                    "candidate_output_role": "corroborating-source-link-mechanism-only",
                    "generated_from_current_source": True,
                    "same_object_inputs": True,
                    "object_members": object_members,
                    "selector_bindings": selector_bindings,
                    "noicf_link": {
                        "link_profile": "vc5sp3_ref_noicf",
                        "effective_link_flags": ["/OPT:REF", "/OPT:NOICF"],
                        "member_addresses": member_addresses_noicf,
                        "transcript_path": self.report_paths[2],
                    },
                    "icf_link": {
                        "link_profile": "vc5sp3_ref_icf",
                        "effective_link_flags": ["/OPT:REF", "/OPT:ICF"],
                        "member_addresses": member_addresses_icf,
                        "transcript_path": self.report_paths[3],
                    },
                    "negative_control": {
                        "role": "base-implementation",
                        "object_symbol": "?SetEnabled@BaseControl@@UAEXH@Z",
                        "object_path": "build/vc5/base-control.obj",
                        "object_report_path": self.report_paths[6],
                        "comdat_eligible": True,
                        "comdat_selection": 2,
                        "noicf_address": "0x510040",
                        "icf_address": "0x520040",
                        "folded_with_members": False,
                        "fold_exclusion_proof": {
                            "control_section_length": 16,
                            "member_section_lengths": {
                                self.alias_a: 16,
                                self.alias_b: 16,
                            },
                            "raw_fold_relevant_bytes_equal": True,
                            "relocation_partitions_equal": True,
                            "associative_sections_equal": True,
                            "difference_reasons": [
                                "base IMAGE_COMDAT_SELECT_ANY differs from member "
                                "IMAGE_COMDAT_SELECT_NODUPLICATES"
                            ],
                        },
                    },
                },
            },
        }

    def test_payload_file_uses_the_inline_parser_unchanged(self) -> None:
        payload_json = json.dumps(self.payload())
        payload_path = Path(self.tempdir.name) / "logical-alias-v4.json"
        payload_path.write_text(payload_json, encoding="utf-8")

        self.assertEqual(
            _parse_logical_alias_group_payload(payload_json),
            _load_logical_alias_group_payload(
                SimpleNamespace(payload_json=None, payload_file=payload_path)
            ),
        )

    def test_payload_file_rejects_malformed_and_ungoverned_paths(self) -> None:
        invalid_json_path = Path(self.tempdir.name) / "invalid.json"
        invalid_json_path.write_text("{", encoding="utf-8")
        with self.assertRaisesRegex(
            ProgressError, "--payload-file is not valid JSON"
        ):
            _load_logical_alias_group_payload(
                SimpleNamespace(payload_json=None, payload_file=invalid_json_path)
            )

        with self.assertRaisesRegex(ProgressError, "not a regular file"):
            _load_logical_alias_group_payload(
                SimpleNamespace(payload_json=None, payload_file=Path(self.tempdir.name))
            )

        outside_path = self.source_root / "logical-alias-v4.json"
        outside_path.write_text(json.dumps(self.payload()), encoding="utf-8")
        with self.assertRaisesRegex(
            ProgressError, "must resolve under workspace build/"
        ):
            _load_logical_alias_group_payload(
                SimpleNamespace(payload_json=None, payload_file=outside_path)
            )

    def test_v4_preserves_one_physical_gate_and_indexes_logical_selection(self) -> None:
        data = self.document()
        parsed = _parse_logical_alias_group_payload(json.dumps(self.payload()))
        details = _set_symbol_logical_alias_group(data, parsed)

        physical = data["symbols"][self.symbol_id]
        self.assertEqual("authored", physical["pipeline_class"])
        self.assertEqual("authored-body", physical["authored_order_role"])
        self.assertTrue(details["physical_authored_gate_preserved"])
        self.assertTrue(details["logical_owner_edges_exclusive"])
        self.assertTrue(details["retail_selector_truth_candidate_independent"])
        proof = parsed["authored_icf_proof"]
        candidate = proof["candidate_mechanism"]
        self.assertEqual("recoil-authored-icf-proof-v2", proof["schema"])
        self.assertEqual(
            {1},
            {
                member["comdat_selection"]
                for member in candidate["object_members"].values()
            },
        )
        self.assertEqual(2, candidate["negative_control"]["comdat_selection"])
        self.assertTrue(
            all(
                not logical_alias_authored_order_gate(alias)
                for alias in physical["logical_aliases"].values()
            )
        )
        projection = census(ProgressDocument(data), self.block_id)["authored_projection"]
        self.assertEqual([self.symbol_id], projection)

        indexes = build_identity_indexes(ProgressDocument(data))
        self.assertEqual(
            ("0x401000", f"logical:{self.alias_a}"),
            indexes.reviewed_authored_icf_by_call_site["0x402000"],
        )
        self.assertEqual(
            f"logical:{self.alias_b}",
            indexes.reviewed_authored_icf_by_vtable_selector[
                ("storage:recoil:data:0x4f0000", 4)
            ],
        )
        self.assertEqual(
            ("direct", f"logical:{self.alias_a}"),
            _canonical_direct_identity(
                "0x401000",
                source="bn",
                caller_identity="authored:caller",
                caller_start=0x400000,
                caller_end=0x400100,
                indexes=indexes,
                bridge_names={},
                compiler_generated_bridges={},
                call_site_address="0x402000",
            ),
        )
        self.assertEqual([], ProgressDocument(data).audit())

    def test_v4_accepts_distinct_selection_one_symbols_from_one_object(self) -> None:
        payload = self.payload()
        members = payload["authored_icf_proof"]["candidate_mechanism"][
            "object_members"
        ]
        for member in members.values():
            member["object_path"] = "build/vc5/Seq.obj"

        parsed = _parse_logical_alias_group_payload(json.dumps(payload))
        parsed_members = parsed["authored_icf_proof"]["candidate_mechanism"][
            "object_members"
        ]
        self.assertEqual(
            {"build/vc5/Seq.obj"},
            {member["object_path"] for member in parsed_members.values()},
        )
        self.assertEqual(
            2,
            len({member["object_symbol"] for member in parsed_members.values()}),
        )

        data = self.document()
        _set_symbol_logical_alias_group(data, parsed)
        self.assertEqual([], ProgressDocument(data).audit())

    def test_v4_same_object_keeps_symbol_and_negative_control_separation(self) -> None:
        payload = self.payload()
        members = payload["authored_icf_proof"]["candidate_mechanism"][
            "object_members"
        ]
        shared_path = "build/vc5/Seq.obj"
        for member in members.values():
            member["object_path"] = shared_path

        duplicate_symbol = members[self.alias_a]["object_symbol"]
        payload["logical_aliases"][self.alias_b]["object_symbol"] = duplicate_symbol
        members[self.alias_b]["object_symbol"] = duplicate_symbol
        payload["authored_icf_proof"]["candidate_mechanism"]["selector_bindings"][
            self.alias_b
        ][0]["relocations"][0]["target_object_symbol"] = duplicate_symbol
        with self.assertRaisesRegex(
            ProgressError, "distinct decorated member symbols"
        ):
            validate_authored_icf_proof(
                payload["authored_icf_proof"],
                physical_address="0x401000",
                aliases=payload["logical_aliases"],
            )

        payload = self.payload()
        members = payload["authored_icf_proof"]["candidate_mechanism"][
            "object_members"
        ]
        for member in members.values():
            member["object_path"] = shared_path
        payload["authored_icf_proof"]["candidate_mechanism"]["negative_control"][
            "object_path"
        ] = shared_path
        with self.assertRaisesRegex(ProgressError, "distinct repository-relative OBJ"):
            _parse_logical_alias_group_payload(json.dumps(payload))

    def test_v4_fails_closed_on_fold_owner_source_and_selector_drift(self) -> None:
        cases = []
        payload = self.payload()
        payload["authored_icf_proof"]["candidate_mechanism"]["noicf_link"][
            "member_addresses"
        ][self.alias_b] = "0x510000"
        cases.append(payload)

        payload = self.payload()
        payload["logical_aliases"][self.alias_b]["owner_id"] = self.owner_a
        cases.append(payload)

        payload = self.payload()
        payload["logical_aliases"][self.alias_b]["source_traceability"][
            "source_edges"
        ][0]["anchor_id"] = "recoil:anchor:fixture-mission-set-enabled"
        payload["logical_aliases"][self.alias_b]["source_traceability"][
            "source_edges"
        ][0]["emission_context"]["translation_unit"] = "src/mission.cpp"
        cases.append(payload)

        payload = self.payload()
        payload["logical_aliases"][self.alias_b]["retail_target_selectors"] = {
            "direct_call_sites": ["0x402000"],
            "vtable_entries": [],
        }
        cases.append(payload)

        for payload in cases:
            with self.subTest(payload=payload):
                with self.assertRaises(ProgressError):
                    _parse_logical_alias_group_payload(json.dumps(payload))

    def test_v4_provisional_name_is_candidate_only_and_legacy_schemas_stay_strict(self) -> None:
        data = self.document()
        payload = self.payload()
        payload["logical_aliases"][self.alias_a]["original_name_status"] = "provisional"
        parsed = _parse_logical_alias_group_payload(json.dumps(payload))
        _set_symbol_logical_alias_group(data, parsed)
        indexes = build_identity_indexes(ProgressDocument(data))
        object_symbol = payload["logical_aliases"][self.alias_a]["object_symbol"]

        self.assertIn(object_symbol, indexes.candidate_only_names)
        self.assertEqual(
            (self.symbol_id, f"logical:{self.alias_a}"),
            indexes.reviewed_authored_icf_provisional_candidate_by_name[
                object_symbol
            ],
        )
        alias = next(
            row
            for row in indexes.reviewed_logical_aliases_by_address["0x401000"]
            if row.identity == f"logical:{self.alias_a}"
        )
        self.assertEqual("", alias.original_name)
        self.assertEqual("", alias.object_symbol)
        self.assertEqual(
            ("direct", f"logical:{self.alias_a}"),
            _canonical_direct_identity(
                object_symbol,
                source="cod",
                caller_identity="authored:caller",
                caller_start=0x400000,
                caller_end=0x400100,
                indexes=indexes,
                bridge_names={},
                compiler_generated_bridges={},
            ),
        )
        with self.assertRaisesRegex(
            ValueError, "incomplete physical/logical population"
        ):
            _canonical_direct_identity(
                object_symbol,
                source="cod",
                caller_identity="authored:caller",
                caller_start=0x400000,
                caller_end=0x400100,
                indexes=replace(
                    indexes,
                    reviewed_authored_icf_physical_by_logical_identity={},
                ),
                bridge_names={},
                compiler_generated_bridges={},
            )
        bridge = {object_symbol: SimpleNamespace(address="0x401000", name=object_symbol)}
        with self.assertRaisesRegex(ValueError, "missing exact reviewed logical alias"):
            _canonical_direct_identity(
                object_symbol,
                source="bn",
                caller_identity="authored:caller",
                caller_start=0x400000,
                caller_end=0x400100,
                indexes=indexes,
                bridge_names=bridge,
                compiler_generated_bridges={},
                call_site_address="0x402100",
            )
        self.assertEqual(
            ("direct", f"logical:{self.alias_a}"),
            _canonical_direct_identity(
                object_symbol,
                source="bn",
                caller_identity="authored:caller",
                caller_start=0x400000,
                caller_end=0x400100,
                indexes=indexes,
                bridge_names=bridge,
                compiler_generated_bridges={},
                call_site_address="0x402000",
            ),
        )

        for legacy_payload in (
            RecoilCliTests.logical_alias_group_payload(),
            RecoilCliTests.logical_alias_group_v2_payload(),
        ):
            first_alias = next(iter(legacy_payload["logical_aliases"].values()))
            first_alias["original_name_status"] = "provisional"
            with self.assertRaisesRegex(ProgressError, "must be 'recovered'"):
                _parse_logical_alias_group_payload(json.dumps(legacy_payload))

    def test_v4_rejects_incomplete_selector_binding_and_unproved_control(self) -> None:
        payload = self.payload()
        payload["authored_icf_proof"]["candidate_mechanism"]["selector_bindings"][
            self.alias_a
        ][0]["relocations"][0]["target_logical_identity"] = self.alias_b
        with self.assertRaisesRegex(ProgressError, "must target logical member"):
            _parse_logical_alias_group_payload(json.dumps(payload))

        payload = self.payload()
        exclusion = payload["authored_icf_proof"]["candidate_mechanism"][
            "negative_control"
        ]["fold_exclusion_proof"]
        exclusion["control_section_length"] = 16
        exclusion["raw_fold_relevant_bytes_equal"] = True
        exclusion["relocation_partitions_equal"] = True
        exclusion["associative_sections_equal"] = True
        payload["authored_icf_proof"]["candidate_mechanism"]["negative_control"][
            "comdat_selection"
        ] = 1
        with self.assertRaisesRegex(ProgressError, "demonstrated fold-relevant difference"):
            _parse_logical_alias_group_payload(json.dumps(payload))

    def test_v4_v2_comdat_selection_is_exact_and_v1_remains_compatible(self) -> None:
        for value in (None, True, 0, 8, "2"):
            payload = self.payload()
            payload["authored_icf_proof"]["candidate_mechanism"]["object_members"][
                self.alias_a
            ]["comdat_selection"] = value
            with self.subTest(member_selection=value):
                with self.assertRaisesRegex(ProgressError, "IMAGE_COMDAT_SELECT"):
                    _parse_logical_alias_group_payload(json.dumps(payload))

            payload = self.payload()
            payload["authored_icf_proof"]["candidate_mechanism"]["negative_control"][
                "comdat_selection"
            ] = value
            with self.subTest(control_selection=value):
                with self.assertRaisesRegex(ProgressError, "IMAGE_COMDAT_SELECT"):
                    _parse_logical_alias_group_payload(json.dumps(payload))

        payload = self.payload()
        del payload["authored_icf_proof"]["candidate_mechanism"]["negative_control"][
            "comdat_selection"
        ]
        with self.assertRaisesRegex(ProgressError, "fields must be exactly"):
            _parse_logical_alias_group_payload(json.dumps(payload))

        legacy = self.payload()
        legacy["authored_icf_proof"]["schema"] = "recoil-authored-icf-proof-v1"
        candidate = legacy["authored_icf_proof"]["candidate_mechanism"]
        for member in candidate["object_members"].values():
            del member["comdat_selection"]
        control = candidate["negative_control"]
        del control["comdat_selection"]
        control["fold_exclusion_proof"]["control_section_length"] = 20
        control["fold_exclusion_proof"]["raw_fold_relevant_bytes_equal"] = False
        parsed = _parse_logical_alias_group_payload(json.dumps(legacy))
        self.assertEqual(
            "recoil-authored-icf-proof-v1", parsed["authored_icf_proof"]["schema"]
        )

    def test_v4_source_mirrors_fail_closed_on_missing_wrong_and_detached_rows(self) -> None:
        corruptions = (
            lambda path: path.unlink(),
            lambda path: path.write_text(
                path.read_text(encoding="utf-8").replace(self.alias_a, self.alias_b),
                encoding="utf-8",
            ),
            lambda path: path.write_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:fixture-mission-set-enabled\n"
                f" * @recoil-artifact defines .text {self.alias_a}: Detached.\n"
                " * Purpose: Detached negative fixture.\n"
                " */\n",
                encoding="utf-8",
            ),
        )
        for corrupt in corruptions:
            self._write_source_mirrors()
            mission_path = self.source_root / "src" / "mission.cpp"
            corrupt(mission_path)
            data = self.document()
            parsed = _parse_logical_alias_group_payload(json.dumps(self.payload()))
            before = deepcopy(data)
            with self.assertRaisesRegex(ProgressError, "source mirror"):
                _set_symbol_logical_alias_group(data, parsed)
            self.assertEqual(before, data)

        self._write_source_mirrors()
        data = self.document()
        _set_symbol_logical_alias_group(
            data,
            _parse_logical_alias_group_payload(json.dumps(self.payload())),
        )
        (self.source_root / "src" / "mission.cpp").unlink()
        findings = ProgressDocument(data).audit()
        self.assertTrue(
            any(
                finding.code == "authored-icf.invalid"
                and "source mirror" in finding.message
                for finding in findings
            )
        )

    def test_v4_physical_source_gate_uses_exact_document_scoped_logical_population(self) -> None:
        data = self.document()
        _set_symbol_logical_alias_group(
            data,
            _parse_logical_alias_group_payload(json.dumps(self.payload())),
        )
        documents = tuple(
            parse_source_trace_path(path, repo_root=self.source_root)
            for path in (
                self.source_root / "src" / "mission.cpp",
                self.source_root / "src" / "hud.cpp",
            )
        )
        self.assertEqual(
            tuple(sorted((self.alias_a, self.alias_b))),
            validate_authored_icf_physical_source_artifacts(
                data,
                physical_symbol_id=self.symbol_id,
                documents=documents,
            ),
        )
        self.assertEqual(
            (self.alias_a,),
            validate_authored_icf_physical_source_artifacts(
                data,
                physical_symbol_id=self.symbol_id,
                documents=documents[:1],
            ),
        )

        self.assertEqual(
            (self.alias_a,),
            validate_authored_icf_physical_source_artifacts(
                data,
                physical_symbol_id=self.symbol_id,
                documents=documents[:1],
                select_single_logical_member=True,
            ),
        )
        with self.assertRaisesRegex(ProgressError, "must select exactly one"):
            validate_authored_icf_physical_source_artifacts(
                data,
                physical_symbol_id=self.symbol_id,
                documents=documents,
                select_single_logical_member=True,
            )
        with self.assertRaisesRegex(ProgressError, "must select exactly one"):
            validate_authored_icf_physical_source_artifacts(
                data,
                physical_symbol_id=self.symbol_id,
                documents=(),
                select_single_logical_member=True,
            )

        with self.assertRaisesRegex(ProgressError, "nonempty current logical"):
            validate_authored_icf_physical_source_artifacts(
                data,
                physical_symbol_id=self.symbol_id,
                documents=(),
            )

        with self.assertRaisesRegex(ProgressError, "exactly the current logical"):
            validate_authored_icf_physical_source_artifacts(
                data,
                physical_symbol_id=self.symbol_id,
                documents=(replace(documents[0], artifacts=()),),
            )

        extra_id = "recoil:logical-function:0x401000:unexpected-extra"
        extra_path = self.source_root / "src" / "extra.cpp"
        extra_path.write_text(
            "/**\n"
            " * @recoil-anchor recoil:anchor:fixture-unexpected-extra\n"
            f" * @recoil-artifact defines .text {extra_id}: Unexpected extra.\n"
            " * Purpose: Negative authored ICF source-closure fixture.\n"
            " */\n"
            "void UnexpectedExtra() {}\n",
            encoding="utf-8",
        )
        extra_document = parse_source_trace_path(
            extra_path,
            repo_root=self.source_root,
        )
        with self.assertRaisesRegex(ProgressError, "exactly the current logical"):
            validate_authored_icf_physical_source_artifacts(
                data,
                physical_symbol_id=self.symbol_id,
                documents=(*documents, extra_document),
            )

        same_tu = deepcopy(data)
        same_tu["symbols"][self.symbol_id]["logical_aliases"][self.alias_b][
            "source_traceability"
        ]["source_edges"][0]["emission_context"]["translation_unit"] = "src/mission.cpp"
        mission_path = self.source_root / "src" / "mission.cpp"
        hud_path = self.source_root / "src" / "hud.cpp"
        mission_path.write_text(
            mission_path.read_text(encoding="utf-8")
            + hud_path.read_text(encoding="utf-8"),
            encoding="utf-8",
        )
        same_tu_document = parse_source_trace_path(
            mission_path,
            repo_root=self.source_root,
        )
        self.assertEqual(
            tuple(sorted((self.alias_a, self.alias_b))),
            validate_authored_icf_physical_source_artifacts(
                same_tu,
                physical_symbol_id=self.symbol_id,
                documents=(same_tu_document,),
            ),
        )
        with self.assertRaisesRegex(ProgressError, "exactly the current logical"):
            validate_authored_icf_physical_source_artifacts(
                same_tu,
                physical_symbol_id=self.symbol_id,
                documents=(
                    replace(
                        same_tu_document,
                        artifacts=tuple(
                            artifact
                            for artifact in same_tu_document.artifacts
                            if artifact.artifact_id != self.alias_b
                        ),
                    ),
                ),
            )

    def test_v4_physical_source_gate_rejects_stale_or_nonexclusive_group(self) -> None:
        documents = tuple(
            parse_source_trace_path(path, repo_root=self.source_root)
            for path in (
                self.source_root / "src" / "mission.cpp",
                self.source_root / "src" / "hud.cpp",
            )
        )

        data = self.document()
        _set_symbol_logical_alias_group(
            data,
            _parse_logical_alias_group_payload(json.dumps(self.payload())),
        )
        evidence_id = data["symbols"][self.symbol_id]["icf_address_group"][
            "evidence_ids"
        ][0]
        data["evidence"][evidence_id]["freshness"] = "historical"
        with self.assertRaisesRegex(ProgressError, "absent, stale, or has the wrong"):
            validate_authored_icf_physical_source_artifacts(
                data,
                physical_symbol_id=self.symbol_id,
                documents=documents,
            )

        data = self.document()
        _set_symbol_logical_alias_group(
            data,
            _parse_logical_alias_group_payload(json.dumps(self.payload())),
        )
        data["symbols"][self.symbol_id]["logical_aliases"][self.alias_b][
            "owner_id"
        ] = self.owner_a
        with self.assertRaisesRegex(ProgressError, "owner edges are not exclusive"):
            validate_authored_icf_physical_source_artifacts(
                data,
                physical_symbol_id=self.symbol_id,
                documents=documents,
            )

    def test_v4_translation_unit_object_symbol_projection_is_exact_and_fail_closed(self) -> None:
        data = self.document()
        _set_symbol_logical_alias_group(
            data,
            _parse_logical_alias_group_payload(json.dumps(self.payload())),
        )
        self.assertEqual(
            (self.alias_a, "?SetEnabled@MissionFmvState@@QAEXH@Z"),
            select_authored_icf_translation_unit_object_symbol(
                data,
                physical_symbol_id=self.symbol_id,
                translation_unit="src/mission.cpp",
            ),
        )
        self.assertEqual(
            (self.alias_b, "?SetEnabled@HudBackground@@UAEXH@Z"),
            select_authored_icf_translation_unit_object_symbol(
                data,
                physical_symbol_id=self.symbol_id,
                translation_unit="src\\hud.cpp",
            ),
        )
        with self.assertRaisesRegex(ProgressError, "requires exactly one current proven"):
            select_authored_icf_translation_unit_object_symbol(
                data,
                physical_symbol_id=self.symbol_id,
                translation_unit="src/other.cpp",
            )

        multiple = deepcopy(data)
        multiple["symbols"][self.symbol_id]["logical_aliases"][self.alias_b][
            "source_traceability"
        ]["source_edges"][0]["emission_context"]["translation_unit"] = "src/mission.cpp"
        mission_path = self.source_root / "src" / "mission.cpp"
        hud_path = self.source_root / "src" / "hud.cpp"
        mission_path.write_text(
            mission_path.read_text(encoding="utf-8")
            + hud_path.read_text(encoding="utf-8"),
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ProgressError, "requires exactly one current proven"):
            select_authored_icf_translation_unit_object_symbol(
                multiple,
                physical_symbol_id=self.symbol_id,
                translation_unit="src/mission.cpp",
            )

        self._write_source_mirrors()
        stale = deepcopy(data)
        evidence_id = stale["symbols"][self.symbol_id]["icf_address_group"][
            "evidence_ids"
        ][0]
        stale["evidence"][evidence_id]["freshness"] = "historical"
        with self.assertRaisesRegex(ProgressError, "absent, stale, or has the wrong"):
            select_authored_icf_translation_unit_object_symbol(
                stale,
                physical_symbol_id=self.symbol_id,
                translation_unit="src/mission.cpp",
            )

        unproven = deepcopy(data)
        unproven["symbols"][self.symbol_id]["logical_aliases"][self.alias_a][
            "object_symbol"
        ] = "?Unproven@MissionFmvState@@QAEXH@Z"
        with self.assertRaisesRegex(ProgressError, "does not match its logical member"):
            select_authored_icf_translation_unit_object_symbol(
                unproven,
                physical_symbol_id=self.symbol_id,
                translation_unit="src/mission.cpp",
            )

        ordinary = self.document()
        self.assertIsNone(
            select_authored_icf_translation_unit_object_symbol(
                ordinary,
                physical_symbol_id=self.symbol_id,
                translation_unit="src/mission.cpp",
            )
        )

    def test_owner_views_expose_logical_members_without_duplicate_physical_primary(self) -> None:
        data = self.document()
        parsed = _parse_logical_alias_group_payload(json.dumps(self.payload()))
        _set_symbol_logical_alias_group(data, parsed)
        document = ProgressDocument(data)

        owner_a = _owner_view(document, self.owner_a)["owners"][0]
        owner_b = _owner_view(document, self.owner_b)["owners"][0]
        self.assertEqual(
            [self.alias_a],
            [row["logical_identity_key"] for row in owner_a["logical_function_members"]],
        )
        self.assertEqual(
            [self.alias_b],
            [row["logical_identity_key"] for row in owner_b["logical_function_members"]],
        )
        address_view = _owner_view(document, "0x401000")
        self.assertEqual({self.owner_a, self.owner_b}, {row["id"] for row in address_view["owners"]})
        physical_primary_rows = [
            relationship
            for owner in data["owners"].values()
            for relationship in owner["relationships"]
            if relationship.get("kind") == "primary-function"
            and relationship.get("symbol_id") == self.symbol_id
        ]
        self.assertEqual(1, len(physical_primary_rows))
        self.assertEqual([], document.audit())

    def test_v4_mutation_rejects_missing_physical_primary_owner_without_mutation(self) -> None:
        data = self.document()
        data["owners"][self.owner_a]["relationships"] = []
        before = deepcopy(data)
        parsed = _parse_logical_alias_group_payload(json.dumps(self.payload()))

        with self.assertRaisesRegex(ProgressError, "exactly one address-exclusive"):
            _set_symbol_logical_alias_group(data, parsed)

        self.assertEqual(before, data)


if __name__ == "__main__":
    unittest.main()
