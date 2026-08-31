from __future__ import annotations

import contextlib
import io
import json
from pathlib import Path
from types import SimpleNamespace
import sys
import tempfile
import unittest
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands import source_fragments  # noqa: E402
from _recoil.commands import progress_cli, vc5_verify  # noqa: E402
from _recoil.lib.progress import ProgressError  # noqa: E402
from _recoil.lib.source_fragments import (  # noqa: E402
    inventory_source_fragments,
    production_closure_fragment_findings,
)


class RecoilSourceFragmentsTests(unittest.TestCase):
    @staticmethod
    def initialize_fixture(root: Path, files: dict[str, str]) -> None:
        for relative_path, contents in files.items():
            path = root / relative_path
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(contents, encoding="utf-8")

    @staticmethod
    def target(path: Path) -> SimpleNamespace:
        return SimpleNamespace(
            name="unit",
            manifest_path=path.parent / "unit.json",
            source_from=str(path),
            source_files=(),
            order_edit_paths=(),
            translation_unit_function_order=(),
        )

    def make_fixture(self, root: Path) -> Path:
        source = root / "src"
        source.mkdir()
        (source / "main.cpp").write_text(
            '#include "outer.h"\n#include "split.cpp"\n',
            encoding="utf-8",
        )
        (source / "outer.h").write_text('#include "piece_body.h"\n', encoding="utf-8")
        (source / "piece_body.h").write_text("inline int Piece() { return 1; }\n", encoding="utf-8")
        (source / "ai_net.h").write_text("inline int Ordinary() { return 2; }\n", encoding="utf-8")
        (source / "split.cpp").write_text("int Split() { return 3; }\n", encoding="utf-8")
        (source / "legacy.inl").write_text("inline int Legacy() { return 4; }\n", encoding="utf-8")
        return source

    def test_inventory_classifies_exact_forbidden_forms_without_generic_body_headers(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = self.make_fixture(root)
            result = inventory_source_fragments(
                source,
                repo_root=root,
            )

        self.assertEqual(
            {
                "fragment_files": 1,
                "fragment_include_edges": 1,
                "direct_fragment_include_edges": 1,
                "nested_fragment_include_edges": 0,
                "included_source_edges": 1,
                "included_source_files": 1,
                "inl_files": 1,
                "total_findings": 4,
            },
            result["counts"],
        )
        self.assertNotIn("src/ai_net.h", result["findings"]["fragment_files"])
        self.assertEqual("direct", result["findings"]["included_source_edges"][0]["edge_scope"])

    def test_closure_reports_nested_fragment_edge_and_ignores_external_paths(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_fixture(root)
            external = root / "external.cpp"
            external.write_text('#include "src/piece_body.h"\n', encoding="utf-8")

            findings = production_closure_fragment_findings(
                ["src/main.cpp", external],
                repo_root=root,
            )

        nested = [item for item in findings if item["kind"] == "fragment-include-edge"]
        self.assertEqual(1, len(nested))
        self.assertEqual("nested", nested[0]["edge_scope"])
        self.assertTrue(any(item["kind"] == "included-source-edge" for item in findings))

    def test_json_is_complete_and_text_output_is_concise_and_strict(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = self.make_fixture(root)
            for index in range(7):
                (source / f"extra_{index}_body.h").write_text("// debt\n", encoding="utf-8")
            json_stdout = io.StringIO()
            guard_stdout = io.StringIO()
            guard_stderr = io.StringIO()
            with mock.patch.object(source_fragments, "REPO_ROOT", root):
                with contextlib.redirect_stdout(json_stdout):
                    json_rc = source_fragments.main(
                        [
                            "--root",
                            "src",
                            "--json",
                        ]
                    )
                with contextlib.redirect_stdout(guard_stdout), contextlib.redirect_stderr(guard_stderr):
                    guard_rc = source_fragments.main(
                        [
                            "--root",
                            "src",
                        ]
                    )

        report = json.loads(json_stdout.getvalue())
        self.assertEqual(1, json_rc)
        self.assertEqual(8, report["counts"]["fragment_files"])
        self.assertEqual(1, guard_rc)
        self.assertIn("fragment_files=8", guard_stdout.getvalue())
        self.assertIn("source-fragment-file: ... 3 more", guard_stderr.getvalue())
        self.assertNotIn("extra_6_body.h\nsource-fragment-file", guard_stderr.getvalue())

    def test_order_feedback_preserves_real_divergence_and_blocks_only_would_be_pass(self) -> None:
        target = self.target(Path("src/unit.cpp"))
        finding = {"kind": "fragment-file", "path": "src/unit_body.h"}
        order_divergence = {
            "kind": "reordered",
            "message": "real order mismatch",
            "expected_neighbors": ["a"],
            "candidate_neighbors": ["b"],
        }
        with mock.patch.object(vc5_verify, "target_source_fragment_findings", return_value=(finding,)):
            failed, preserved = vc5_verify.apply_source_fragment_order_gate(
                target=target,
                expected=["a"],
                candidate=["b"],
                divergence=order_divergence,
                passed=False,
            )
            blocked, blocker = vc5_verify.apply_source_fragment_order_gate(
                target=target,
                expected=["a"],
                candidate=["a"],
                divergence=None,
                passed=True,
            )

        self.assertFalse(failed)
        self.assertIs(order_divergence, preserved)
        self.assertFalse(blocked)
        self.assertEqual("source-fragment-blocker", blocker["kind"])
        self.assertEqual([finding], blocker["source_fragment_findings"])

    def test_verification_target_sync_and_bootstrap_allow_fragment_closure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "src" / "unit_body.h"
            donor = root / "src" / "donor.h"
            manifest_path = root / "tools" / "vc5_verify_targets" / "unit.json"
            self.initialize_fixture(
                root,
                {
                    "src/unit_body.h": "// debt\n",
                    "src/donor.h": "// reviewed writable closure\n",
                    "tools/vc5_verify_targets/unit.json": '{"name":"unit"}\n',
                },
            )
            target = self.target(source)
            target.manifest_path = manifest_path
            target.source_from = "src/unit_body.h"
            target_id = "recoil:vc5-target:unit"
            registration = {
                "binary": "recoil",
                "check_translation_unit_function_order": True,
                "function_order_scope": "authored",
                "manifest_path": "tools/vc5_verify_targets/unit.json",
                "name": "unit",
                "source_from": "src/unit_body.h",
                "translation_unit_function_order": [
                    {
                        "source_from": "src/unit_body.h",
                        "order_scope": "authored",
                        "inventory_only": False,
                        "functions": [
                            {
                                "address": "0x401000",
                                "pipeline_class": "authored",
                                "authored_order_role": "authored-body",
                            }
                        ],
                    }
                ],
                "linked_function_intervals": [],
                "order_edit_paths": ["src/unit_body.h", "src/donor.h"],
            }
            record = {
                "binary": "recoil",
                "kind": "vc5",
                "name": "unit",
                "registration": registration,
                "registered_addresses": ["0x401000"],
            }
            tracker = {"verification_targets": {}, "symbols": {}, "physical_blocks": {}}
            with (
                mock.patch(
                    "_recoil.lib.verification_targets.load_target_registrations",
                    return_value={target_id: record},
                ),
                mock.patch.object(progress_cli, "load_vc5_manifest", return_value=target),
                mock.patch.object(progress_cli, "REPO_ROOT", root),
                mock.patch.object(vc5_verify, "REPO_ROOT", root),
            ):
                details = progress_cli._sync_verification_targets(
                    tracker,
                    binary="recoil",
                    selectors=["unit"],
                )

            with (
                mock.patch.object(
                    progress_cli,
                    "load_vc5_manifest",
                    side_effect=ValueError(
                        "src/unit_body.h does not contain provenance docblock/comment "
                        "'Reimplements 0x401000:'"
                    ),
                ),
                mock.patch.object(progress_cli, "REPO_ROOT", root),
                mock.patch.object(vc5_verify, "REPO_ROOT", root),
            ):
                bootstrap = progress_cli._source_policy_bootstrap_metadata(
                    target_id=target_id,
                    record=record,
                    manifest_path=target.manifest_path,
                    manifest_data={
                        "retail_start": "0x401000",
                        "retail_end_exclusive": "0x401010",
                    },
                    order_edit_paths=("src/unit_body.h", "src/donor.h"),
                )

        self.assertEqual([target_id], details["source_policy_enforced"])
        self.assertIn(target_id, tracker["verification_targets"])
        self.assertEqual("pending-source-placement", bootstrap["state"])
        self.assertEqual(
            ["src/unit_body.h", "src/donor.h"],
            bootstrap["writable_closure"],
        )

    def test_parent_order_contract_rejects_fragment_before_validation(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "src" / "unit_impl.h"
            manifest_path = root / "tools" / "vc5_verify_targets" / "unit.json"
            self.initialize_fixture(
                root,
                {
                    "src/unit_impl.h": "// debt\n",
                    "tools/vc5_verify_targets/unit.json": '{"name":"unit"}\n',
                },
            )
            target = self.target(source)
            target.manifest_path = manifest_path
            target.source_from = "src/unit_impl.h"
            contract = {
                "verifier_target": {
                    "registration": {
                        "manifest_path": "tools/vc5_verify_targets/unit.json"
                    }
                }
            }
            with (
                mock.patch.object(progress_cli, "load_vc5_manifest", return_value=target),
                mock.patch.object(progress_cli, "REPO_ROOT", root),
                mock.patch.object(vc5_verify, "REPO_ROOT", root),
                self.assertRaisesRegex(
                    ProgressError,
                    "live order acceptance source-fragment blocker",
                ),
            ):
                progress_cli._require_order_contract_source_fragments_clean(contract)


if __name__ == "__main__":
    unittest.main()
