from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.authored_order import (  # noqa: E402
    ScaffoldGap,
    census,
    scaffold,
    sweep,
    write_manifest_candidate,
)
from _recoil.commands.progress_cli import (  # noqa: E402
    _parse_logical_alias_group_payload,
    _parse_physical_block_replace_payload,
    _replace_physical_block,
    _set_symbol_logical_alias_group,
)
from _recoil.commands.vc5_verify import load_manifest  # noqa: E402
from _recoil.lib.progress import (  # noqa: E402
    AUTHORED_ORDER_DIMENSIONS,
    BLOCK_ORDER_DIMENSIONS,
    ORDER_GATE_POLICY,
    ProgressDocument,
    empty_progress_document,
    state_record,
)
from tests.tools.recoil_live_progress_tests import (  # noqa: E402
    util_physical_block_replace_fixture,
)


def pending(names):
    return {name: state_record() for name in names}


def fixture_document(*, unresolved: bool = False, include_zero_block: bool = False) -> ProgressDocument:
    data = empty_progress_document()
    data["binaries"] = {
        "recoil": {
            "binary": "recoil",
            "scheduler": "global-sequential",
            "primary_scheduler": True,
            "text": {"start": "0x401000", "end_exclusive": "0x401030"},
            "reference": {},
            "final_validation": state_record(),
            "final_repro": state_record(),
            "order_policy": dict(ORDER_GATE_POLICY),
        }
    }
    block_id = "recoil:block:0x401000"
    data["physical_blocks"][block_id] = {
        "binary": "recoil",
        "start": "0x401000",
        "end_exclusive": "0x401020",
        "source_path": "sample.cpp",
        "agent_source_path": "sample.cpp",
        "contribution_ids": ["recoil:function:0x401000"],
        "semantic_span_ids": [],
        "order": {
            "authored": pending(AUTHORED_ORDER_DIMENSIONS),
            "full": pending(BLOCK_ORDER_DIMENSIONS),
        },
    }
    data["symbols"]["recoil:function:0x401000"] = {
        "binary": "recoil",
        "kind": "function",
        "address": "0x401000",
        "end_exclusive": "0x401020",
        "navigation_name": "Body",
        "pipeline_class": "unresolved" if unresolved else "authored-lifecycle",
        "physical_block_id": block_id,
        "semantic_span_ids": [],
    }
    data["symbols"]["recoil:function:0x401020"] = {
        "binary": "recoil",
        "kind": "function",
        "address": "0x401020",
        "end_exclusive": "0x401030",
        "navigation_name": "Next",
        "pipeline_class": "non-authored",
        "physical_block_id": "recoil:block:0x401020",
        "semantic_span_ids": [],
    }
    if include_zero_block:
        data["physical_blocks"]["recoil:block:0x401020"] = {
            "binary": "recoil",
            "start": "0x401020",
            "end_exclusive": "0x401030",
            "source_path": "provider:test",
            "agent_source_path": "provider:test",
            "contribution_ids": ["recoil:function:0x401020"],
            "semantic_span_ids": [],
            "order": {
                "authored": pending(AUTHORED_ORDER_DIMENSIONS),
                "full": pending(BLOCK_ORDER_DIMENSIONS),
            },
        }
    return ProgressDocument(data)


def write_inputs(root: Path) -> tuple[Path, Path, Path]:
    source = root / "sample.cpp"
    source.write_text(
        "/** Reimplements 0x401000: Body. Purpose: fixture. */\nvoid Body() {}\n",
        encoding="utf-8",
    )
    context = root / "vc5_final_build.json"
    context.write_text(
        json.dumps(
            {
                "vc5_env": "vc5sp3-env.cmd",
                "compile_flags": ["/nologo", "/TP", "/O2", "/Ob0"],
                "defines": [],
                "include_dirs": [str(root)],
                "sources": [str(source)],
                "canonical_mfc": {
                    "include_root": "D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE"
                },
            }
        ),
        encoding="utf-8",
    )
    manifests = root / "manifests"
    manifests.mkdir()
    (manifests / "identity.json").write_text(
        json.dumps(
            {
                "name": "identity",
                "description": "identity fixture",
                "target_binary": "recoil",
                "source_filename": source.name,
                "source_from": str(source),
                "compiler_env": "vc5sp3-env.cmd",
                "compiler_flags": ["/nologo", "/TP", "/O2", "/Ob0", "/FAcs"],
                "include_dirs": [str(root)],
                "check_translation_unit_function_order": True,
                "translation_unit_function_order": [
                    {
                        "source_from": str(source),
                        "order_scope": "full",
                        "functions": [
                            {
                                "address": "0x401000",
                                "symbol": "?Body@@YAXXZ",
                                "name": "Body",
                                "pipeline_class": "authored-lifecycle",
                            },
                            {
                                "address": "0x401020",
                                "symbol": "?Next@@YAXXZ",
                                "name": "Next",
                                "pipeline_class": "non-authored",
                            },
                        ],
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    return source, context, manifests


def bind_compile_host(document: ProgressDocument, source: Path) -> ProgressDocument:
    document.data["physical_blocks"]["recoil:block:0x401000"]["agent_source_path"] = str(source)
    return document


class RecoilAuthoredOrderTests(unittest.TestCase):
    def test_current_util_split_exposes_only_the_first_physical_slice_to_census(self) -> None:
        data, raw_payload = util_physical_block_replace_fixture()
        payload = _parse_physical_block_replace_payload(json.dumps(raw_payload))
        _replace_physical_block(data, payload)

        result = census(ProgressDocument(data), "0x437e60")

        self.assertEqual("recoil:block:0x437e60", result["block_id"])
        self.assertEqual("0x437e60", result["start"])
        self.assertEqual("0x437ef0", result["end_exclusive"])
        self.assertEqual(
            ["recoil:function:0x437e60", "recoil:function:0x437ea0"],
            result["authored_projection"],
        )
        self.assertEqual(
            ["recoil:semantic:0x437e60-0x437ef0"],
            result["semantic_span_ids"],
        )

    def test_census_excludes_explicit_compiler_generated_lifecycle_role_from_authored_projection(self) -> None:
        document = fixture_document()
        document.data["symbols"]["recoil:function:0x401000"]["authored_order_role"] = (
            "compiler-generated-deleting-variant"
        )
        result = census(document, "0x401000")
        self.assertEqual([], result["authored_projection"])
        self.assertFalse(result["symbols"][0]["authored_order_gate"])
        self.assertEqual(
            "compiler-generated-deleting-variant",
            result["symbols"][0]["authored_order_role"],
        )

    def test_scaffold_writes_deterministic_runnable_exact_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, context, manifests = write_inputs(root)
            document = bind_compile_host(fixture_document(), source)
            payload = scaffold(
                document,
                "0x401000",
                manifest_dir=manifests,
                final_build_manifest=context,
            )
            manifest = payload["manifest_candidate"]
            self.assertEqual("recoil", manifest["target_binary"])
            self.assertEqual("0x401000", manifest["retail_start"])
            self.assertEqual("0x401020", manifest["retail_end_exclusive"])
            self.assertEqual(str(source), manifest["source_from"])
            function = manifest["translation_unit_function_order"][0]["functions"][0]
            self.assertEqual("?Body@@YAXXZ", function["symbol"])
            self.assertEqual("authored-lifecycle", function["pipeline_class"])
            interval = manifest["linked_function_intervals"][0]
            self.assertEqual("authored", interval["order_scope"])
            self.assertEqual("0x401000", interval["retail_start"])
            self.assertEqual("0x401020", interval["retail_end_exclusive"])
            self.assertEqual("?Next@@YAXXZ", interval["successor"]["symbol"])
            self.assertEqual(
                "D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE",
                manifest["authored_order_scaffold"]["canonical_mfc_include_root"],
            )

            output = root / "candidate.json"
            write_manifest_candidate(output, payload)
            first = output.read_bytes()
            write_manifest_candidate(output, payload)
            self.assertEqual(first, output.read_bytes())
            loaded = load_manifest(output, enforce_source_policy=True)
            self.assertEqual("authored_order_401000_401020_candidate", loaded.name)
            self.assertEqual("authored", loaded.linked_function_intervals[0].order_scope)

    def test_scaffold_excludes_phase_deferred_lifecycle_roles_without_identity(self) -> None:
        for role in ("compiler-generated-eh-helper", "compiler-generated-thunk"):
            with self.subTest(role=role), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                source, context, manifests = write_inputs(root)
                document = bind_compile_host(fixture_document(), source)
                block = document.data["physical_blocks"]["recoil:block:0x401000"]
                block["contribution_ids"].append("recoil:function:0x401010")
                document.data["symbols"]["recoil:function:0x401010"] = {
                    "binary": "recoil",
                    "kind": "function",
                    "address": "0x401010",
                    "end_exclusive": "0x401020",
                    "navigation_name": "Deferred lifecycle contribution",
                    "pipeline_class": "authored-lifecycle",
                    "authored_order_role": role,
                    "authored_order_gate": False,
                    "physical_block_id": "recoil:block:0x401000",
                    "semantic_span_ids": [],
                }

                payload = scaffold(
                    document,
                    "0x401000",
                    manifest_dir=manifests,
                    final_build_manifest=context,
                )

                functions = payload["manifest_candidate"]["translation_unit_function_order"][0]["functions"]
                self.assertEqual(["0x401000"], [function["address"] for function in functions])
                self.assertNotIn(
                    "0x401010",
                    [item["address"] for item in payload["identity_provenance"]],
                )

                result = sweep(
                    document,
                    from_current=True,
                    manifest_dir=manifests,
                    final_build_manifest=context,
                )
                self.assertEqual(1, result["ready_count"])
                self.assertEqual([], result["blocks"][0]["gaps"])

    def test_scaffold_emits_independent_icf_alias_presence_without_internal_order_claim(self) -> None:
        from tests.tools.recoil_cli_tests import RecoilCliTests

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, context, manifests = write_inputs(root)
            document = bind_compile_host(fixture_document(), source)
            symbol = document.data["symbols"]["recoil:function:0x401000"]
            symbol["pipeline_class"] = "non-authored"
            symbol["authored_order_role"] = "compiler-generated-icf-representative"
            alias_fixture = RecoilCliTests.logical_alias_group_document()
            document.data["evidence"].update(alias_fixture["evidence"])
            document.data["owners"].update(alias_fixture["owners"])
            raw_alias_payload = RecoilCliTests.logical_alias_group_payload()
            alias_payload = _parse_logical_alias_group_payload(json.dumps(raw_alias_payload))
            _set_symbol_logical_alias_group(document.data, alias_payload)
            expected_keys = sorted(raw_alias_payload["logical_aliases"])
            expected_fold_statuses = [
                raw_alias_payload["logical_aliases"][identity_key]["fold_status"]
                for identity_key in expected_keys
            ]

            payload = scaffold(
                document,
                "0x401000",
                manifest_dir=manifests,
                final_build_manifest=context,
            )
            functions = payload["manifest_candidate"]["translation_unit_function_order"][0]["functions"]
            self.assertEqual(["0x401000", "0x401000"], [row["address"] for row in functions])
            self.assertEqual(expected_keys, [row["logical_identity_key"] for row in functions])
            self.assertEqual(
                expected_fold_statuses,
                [row["icf_fold_status"] for row in functions],
            )
            self.assertTrue(all(row["required_presence"] for row in functions))
            self.assertTrue(all(not row["full_order_gate"] for row in functions))

            output = root / "icf-candidate.json"
            write_manifest_candidate(output, payload)
            loaded = load_manifest(output, enforce_source_policy=True)
            loaded_functions = loaded.translation_unit_function_order[0].functions
            self.assertEqual(2, len(loaded_functions))
            self.assertEqual(expected_keys[0], loaded_functions[0].logical_identity_key)
            self.assertEqual(expected_keys[1], loaded_functions[1].logical_identity_key)

    def test_scaffold_emits_neutral_fold_unknown_authored_identity_as_relative_order_gate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, context, manifests = write_inputs(root)
            document = bind_compile_host(fixture_document(), source)
            symbol = document.data["symbols"]["recoil:function:0x401000"]
            identity_key = "recoil:logical-function:0x401000:hud-member"
            symbol["pipeline_class"] = "unresolved"
            symbol["authored_order_role"] = "unresolved"
            symbol["linked_address_group"] = {
                "group_kind": "neutral-linked-address-group",
                "fold_state": "not-established",
                "winner_status": "not-established",
                "winner_identity_key": None,
                "evidence_ids": ["fixture-group"],
            }
            symbol["logical_aliases"] = {
                identity_key: {
                    "object_symbol": "?ProvisionalMember@HudUiWidget@@QAEXXZ",
                    "original_name": "HudUiWidget::ProvisionalMember",
                    "original_name_status": "provisional",
                    "source_owner_status": "authored-owner",
                    "owner_id": "fixture-owner",
                    "pipeline_class": "authored",
                    "authored_order_role": "authored-body",
                    "fold_status": "not-established",
                    "evidence_ids": ["fixture-alias"],
                }
            }

            payload = scaffold(
                document,
                "0x401000",
                manifest_dir=manifests,
                final_build_manifest=context,
            )
            functions = payload["manifest_candidate"]["translation_unit_function_order"][0]["functions"]
            self.assertEqual(1, len(functions))
            self.assertEqual(identity_key, functions[0]["logical_identity_key"])
            self.assertEqual("not-established", functions[0]["icf_fold_status"])
            self.assertFalse(functions[0]["full_order_gate"])

            output = root / "neutral-candidate.json"
            write_manifest_candidate(output, payload)
            loaded = load_manifest(output, enforce_source_policy=True)
            loaded_function = loaded.translation_unit_function_order[0].functions[0]
            self.assertEqual("not-established", loaded_function.icf_fold_status)
            self.assertEqual(identity_key, loaded_function.logical_identity_key)

    def test_scaffold_collapses_same_exact_symbol_from_different_manifest_sources(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, context, manifests = write_inputs(root)
            duplicate_source = root / "duplicate.cpp"
            duplicate_source.write_text("/** Duplicate identity provenance fixture. */\n", encoding="utf-8")
            duplicate = json.loads((manifests / "identity.json").read_text(encoding="utf-8"))
            duplicate["name"] = "identity_duplicate"
            duplicate["source_from"] = str(duplicate_source)
            duplicate["translation_unit_function_order"][0]["source_from"] = str(duplicate_source)
            (manifests / "identity_duplicate.json").write_text(json.dumps(duplicate), encoding="utf-8")
            document = bind_compile_host(fixture_document(), source)

            payload = scaffold(
                document,
                "0x401000",
                manifest_dir=manifests,
                final_build_manifest=context,
            )

            functions = payload["manifest_candidate"]["translation_unit_function_order"][0]["functions"]
            self.assertEqual("?Body@@YAXXZ", functions[0]["symbol"])
            evidence = next(
                row for row in payload["identity_provenance"] if row["address"] == "0x401000"
            )
            self.assertGreaterEqual(len(evidence["identity_evidence"]), 2)

    def test_scaffold_fails_before_writing_when_classification_is_unresolved(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _source, context, manifests = write_inputs(root)
            output = root / "candidate.json"
            with self.assertRaisesRegex(ScaffoldGap, "explicit classifications") as caught:
                scaffold(
                    fixture_document(unresolved=True),
                    "0x401000",
                    manifest_dir=manifests,
                    final_build_manifest=context,
                )
            self.assertEqual("unresolved", caught.exception.kind)
            self.assertFalse(output.exists())

    def test_sweep_reports_ready_and_zero_authored_without_writing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _source, context, manifests = write_inputs(root)
            before = sorted(path.relative_to(root) for path in root.rglob("*"))
            result = sweep(
                bind_compile_host(fixture_document(include_zero_block=True), _source),
                from_current=True,
                manifest_dir=manifests,
                final_build_manifest=context,
            )
            after = sorted(path.relative_to(root) for path in root.rglob("*"))
            self.assertEqual(before, after)
            self.assertEqual(2, result["block_count"])
            self.assertEqual(1, result["ready_count"])
            self.assertEqual("zero_authored", result["blocks"][1]["gaps"][0]["kind"])


if __name__ == "__main__":
    unittest.main()
