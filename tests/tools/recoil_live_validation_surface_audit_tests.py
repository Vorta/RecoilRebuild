from __future__ import annotations

from pathlib import Path
import contextlib
import io
import sys
import tempfile
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands import live_validation_surface_audit as surface_audit  # noqa: E402


audit_paths = surface_audit.audit_paths


class LiveValidationSurfaceAuditTests(unittest.TestCase):
    def test_strict_audit_fails_closed_for_missing_required_component(self) -> None:
        with (
            patch.object(surface_audit, "audit_paths", return_value=[]),
            patch.object(
                surface_audit, "_targeted_direct_evidence_findings", return_value=[]
            ),
            patch.object(
                surface_audit,
                "_registered_repository_path_authority_findings",
                return_value=[],
            ),
            patch.object(
                surface_audit,
                "_machine_local_authority_routing_findings",
                return_value=[],
            ),
            patch.object(
                surface_audit,
                "required_call_contract_verifier_component_findings",
                return_value=[
                    {
                        "kind": "missing",
                        "path": "tools/_recoil/lib/binja.py",
                        "detail": "required component is absent",
                    }
                ],
            ),
            contextlib.redirect_stdout(io.StringIO()),
        ):
            result = surface_audit.main(["--strict", "--json"])
        self.assertEqual(1, result)

    def test_registered_verifier_closure_uses_shared_repository_path_authority(self) -> None:
        self.assertEqual(
            [],
            surface_audit._registered_repository_path_authority_findings(),
        )

    def test_live_authority_defaults_bind_directly_to_canonical_root(self) -> None:
        self.assertEqual([], surface_audit._machine_local_authority_routing_findings())

    def test_noncanonical_live_authority_default_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            consumer = root / "tools" / "_recoil" / "commands" / "progress_cli.py"
            consumer.parent.mkdir(parents=True)
            consumer.write_text(
                "from pathlib import Path\n"
                "DEFAULT_PROGRESS = Path('.agent/RECONSTRUCTION_PROGRESS.sqlite3')\n",
                encoding="utf-8",
            )
            findings = surface_audit._machine_local_authority_routing_findings(
                repository_root=root,
                consumers=((
                    "tools/_recoil/commands/progress_cli.py",
                    "DEFAULT_PROGRESS",
                    ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
                ),),
            )
        self.assertEqual(
            ["noncanonical-ledger-default"],
            [row.token for row in findings],
        )

    def test_routed_library_default_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            consumer = root / "tools" / "_recoil" / "lib" / "progress.py"
            consumer.parent.mkdir(parents=True)
            consumer.write_text(
                "from _recoil.lib.worktree_control import routed_machine_local_path\n"
                "DEFAULT_PROGRESS_PATH = routed_machine_local_path(\n"
                "    executing_worktree_root=REPO_ROOT,\n"
                "    relative_path='.agent/RECONSTRUCTION_PROGRESS.sqlite3',\n"
                ")\n",
                encoding="utf-8",
            )
            findings = surface_audit._machine_local_authority_routing_findings(
                repository_root=root,
                consumers=((
                    "tools/_recoil/lib/progress.py",
                    "DEFAULT_PROGRESS_PATH",
                    ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
                ),),
            )
        self.assertEqual(
            ["noncanonical-ledger-default"],
            [row.token for row in findings],
        )

    def test_executable_content_binding_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "tool.py"
            module_name = "hash" + "lib"
            algorithm = "sha" + "256"
            summary_method = "hex" + "di" + "gest"
            path.write_text(
                f"import {module_name}\nvalue = {module_name}.{algorithm}(data).{summary_method}()\n",
                encoding="utf-8",
            )
            findings = audit_paths([path])
        self.assertGreaterEqual(len(findings), 1)
        self.assertEqual(module_name, findings[0].token)

    def test_gameplay_hash_concept_is_not_treated_as_validation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "game.py"
            path.write_text("gameplay_hash_table = build_runtime_lookup()\n", encoding="utf-8")
            findings = audit_paths([path])
        self.assertEqual([], findings)

    def test_retired_documented_route_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "runbook.md"
            retired_option = "--expected-" + "sha" + "256"
            path.write_text(f"Run `tool {retired_option} VALUE`.\n", encoding="utf-8")
            findings = audit_paths([path])
        self.assertEqual(1, len(findings))

    def test_relative_live_authority_and_stale_domain_guidance_are_reported(self) -> None:
        rows = {
            "relative ledger": (
                "Run `python tools/recoil.py progress next --progress "
                ".agent/RECONSTRUCTION_PROGRESS.sqlite3`.\n",
                "--progress .agent/RECONSTRUCTION_PROGRESS.sqlite3",
            ),
            "stale scheduler guard": (
                "Run `python tools/recoil.py progress work claim-current "
                "--expected-revision 7 --apply`.\n",
                "progress work claim-current --expected-revision",
            ),
        }
        for label, (source, token) in rows.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as temporary:
                path = Path(temporary) / "guidance.md"
                path.write_text(source, encoding="utf-8")
                findings = audit_paths([path])
            self.assertEqual(1, len(findings))
            self.assertIn(token, findings[0].token)

    def test_no_project_owned_currentness_path_is_exempt(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "retired_currentness.py"
            module_name = "hash" + "lib"
            algorithm = "sha" + "256"
            summary_method = "hex" + "di" + "gest"
            leaf_field = "leaf_" + algorithm
            object_field = "object_" + "hash" + "_role"
            pattern_value = "candidate-" + "finger" + "print-only"
            path.write_text(
                f"import {module_name}\n"
                f"{leaf_field} = {module_name}.{algorithm}(canonical_inputs).{summary_method}()\n"
                "policy = {'candidate_expected_truth': False, "
                f"'{object_field}': '{pattern_value}'}}\n",
                encoding="utf-8",
            )
            with patch.object(
                surface_audit,
                "_relative_path",
                return_value="tools/_recoil/lib/retired_currentness.py",
            ):
                findings = audit_paths([path])
        self.assertGreaterEqual(len(findings), 1)

    def test_candidate_artifact_cannot_become_expected_truth(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "unsafe.py"
            object_field = "object_" + "sha" + "256"
            producer = "candidate_object_" + "di" + "gest"
            path.write_text(
                f"{object_field} = {producer}()\nexpected_truth = {object_field}\n",
                encoding="utf-8",
            )
            findings = audit_paths([path])
        self.assertTrue(
            any(item.token == "candidate-artifact-expected-truth" for item in findings)
        )

    def test_unsafe_acceptance_policy_flips_are_reported(self) -> None:
        unsafe_rows = {
            "candidate truth": "policy = {'candidate_expected_truth': True}\n",
            "worker acceptance": "policy = {'worker_acceptance_allowed': True}\n",
            "lease staleness": "policy = {'lease_stales_semantic_evidence': True}\n",
            "optional closeout": "policy = {'phase_closeout_required': False}\n",
            "reused closeout": "policy = {'phase_closeout_no_reuse': False}\n",
            "nonclean closeout": "policy = {'phase_closeout_global_clean': False}\n",
        }
        for label, source in unsafe_rows.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as temporary:
                path = Path(temporary) / "unsafe.py"
                path.write_text(source, encoding="utf-8")
                findings = audit_paths([path])
                self.assertGreaterEqual(len(findings), 1)

    def test_targeted_sqlite_whole_file_read_is_rejected(self) -> None:
        cases = {
            "read": ("before = progress.read_bytes()\n", "whole-sqlite-read_bytes"),
            "compare": ("same = filecmp.cmp(progress, copied_database)\n", "whole-sqlite-file-equivalence"),
            "copy": ("shutil.copyfile(progress, copied_database)\n", "whole-sqlite-file-equivalence"),
            "binary-open": ("stream = open(progress_database, 'rb')\n", "whole-sqlite-binary-open"),
        }
        for label, (source, expected) in cases.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as temporary:
                path = Path(temporary) / "routine_db_test.py"
                path.write_text(source, encoding="utf-8")
                with patch.object(
                    surface_audit, "STRUCTURED_SQLITE_TEST_PATHS", (path,)
                ), patch.object(
                    surface_audit,
                    "CALL_CONTRACT_AUTHORITY_PATH",
                    Path(temporary) / "missing.py",
                ):
                    findings = surface_audit._targeted_direct_evidence_findings()
            self.assertEqual([expected], [row.token for row in findings])

    def test_targeted_call_contract_cache_authority_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            authority = Path(temporary) / "call_contract_verify.py"
            authority.write_text(
                "root = 'tools/' + 'vc5_verify_bn_cache'\n",
                encoding="utf-8",
            )
            with patch.object(
                surface_audit, "STRUCTURED_SQLITE_TEST_PATHS", ()
            ), patch.object(
                surface_audit, "CALL_CONTRACT_AUTHORITY_PATH", authority
            ):
                findings = surface_audit._targeted_direct_evidence_findings()
        self.assertEqual(
            ["persisted-bn-cache-authority"], [row.token for row in findings]
        )

    def test_registered_repository_path_guard_requires_shared_authority(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            consumer = root / "tools" / "_recoil" / "commands" / "vc5_verify.py"
            consumer.parent.mkdir(parents=True)
            consumer.write_text("def verify():\n    return True\n", encoding="utf-8")
            findings = surface_audit._registered_repository_path_authority_findings(
                repository_root=root,
                component_paths=("tools/_recoil/commands/vc5_verify.py",),
            )
        self.assertEqual(
            ["missing-shared-repository-path-authority"],
            [row.token for row in findings],
        )

    def test_registered_repository_path_guard_rejects_independent_definition(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            consumer = root / "tools" / "_recoil" / "commands" / "vc5_verify.py"
            consumer.parent.mkdir(parents=True)
            consumer.write_text(
                "from _recoil.lib.repository_paths import resolve_tracked_repository_file\n"
                "def validate_repository_relative_path(value):\n"
                "    return value\n",
                encoding="utf-8",
            )
            findings = surface_audit._registered_repository_path_authority_findings(
                repository_root=root,
                component_paths=("tools/_recoil/commands/vc5_verify.py",),
            )
        self.assertEqual(
            ["independent-repository-path-authority"],
            [row.token for row in findings],
        )

    def test_registered_repository_path_guard_rejects_physical_case_projection(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            consumer = root / "tools" / "_recoil" / "commands" / "vc5_verify.py"
            consumer.parent.mkdir(parents=True)
            consumer.write_text(
                "from _recoil.lib.repository_paths import resolve_tracked_repository_file\n"
                "def logical(path, root):\n"
                "    return path.resolve().relative_to(root.resolve()).as_posix()\n",
                encoding="utf-8",
            )
            findings = surface_audit._registered_repository_path_authority_findings(
                repository_root=root,
                component_paths=("tools/_recoil/commands/vc5_verify.py",),
            )
        self.assertEqual(
            ["physical-spelling-projected-to-logical-path"],
            [row.token for row in findings],
        )

    def test_registered_repository_path_guard_allows_physical_only_resolution(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            component = root / "tools" / "_recoil" / "lib" / "windows_identity.py"
            component.parent.mkdir(parents=True)
            component.write_text(
                "def physical_identity(path):\n"
                "    return path.resolve(strict=True)\n",
                encoding="utf-8",
            )
            findings = surface_audit._registered_repository_path_authority_findings(
                repository_root=root,
                component_paths=("tools/_recoil/lib/windows_identity.py",),
            )
        self.assertEqual([], findings)

    def test_registered_repository_path_guard_allows_reviewed_sqlite_rendering(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            component = root / "tools" / "_recoil" / "commands" / "progress_cli.py"
            component.parent.mkdir(parents=True)
            component.write_text(
                "from _recoil.lib.repository_paths import resolve_tracked_repository_file\n"
                "REPO_ROOT = object()\n"
                "def _progress_command_path(path):\n"
                "    resolved = path.resolve()\n"
                "    return resolved.relative_to(REPO_ROOT).as_posix()\n",
                encoding="utf-8",
            )
            findings = surface_audit._registered_repository_path_authority_findings(
                repository_root=root,
                component_paths=("tools/_recoil/commands/progress_cli.py",),
            )
        self.assertEqual([], findings)


if __name__ == "__main__":
    unittest.main()
