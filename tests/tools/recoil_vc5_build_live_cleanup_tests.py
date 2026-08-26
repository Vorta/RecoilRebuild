from __future__ import annotations

from contextlib import redirect_stderr, redirect_stdout
import dataclasses
import io
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands import vc5_build


RETIRED_SURFACE_TOKENS = (
    "ha" + "sh",
    "sha" + "256",
    "di" + "gest",
    "receipt",
    "cache",
    "reuse",
    "snapshot",
    "immutable",
    "compile_bundle",
    "--no-pe-compare",
    "--no-resource-compare",
)
TEST_OUTPUT_ROOT = ROOT / "build" / "tool-maintenance" / "playtest-deploy-dry-run" / "test-scratch"


class Vc5BuildLiveCleanupTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        TEST_OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)

    def temporary_directory(self) -> tempfile.TemporaryDirectory[str]:
        return tempfile.TemporaryDirectory(dir=TEST_OUTPUT_ROOT)

    def test_linked_catalog_reads_keyed_logical_alias_records(self) -> None:
        target = vc5_build.VerifyTarget(
            name="unit",
            description="",
            source_filename="",
            source_text="",
            source_from="",
            compare_mode="bytes",
            trim_trailing_nops=False,
            compiler_profile="unit",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("unit.json"),
        )
        identity = "recoil:logical-function:0x401020:unit-alias"
        document = SimpleNamespace(
            revision=731,
            pipeline=lambda _binary: {"authored_order_prefix_end": "0x401060"},
            collection=lambda name: {
                "recoil:function:0x401020": {
                    "binary": "recoil",
                    "kind": "function",
                    "address": "0x401020",
                    "logical_aliases": {
                        identity: {
                            "object_symbol": "?UnitAlias@@YAXXZ",
                            "original_name": "UnitAlias",
                            "pipeline_class": "authored",
                            "authored_order_role": "authored-body",
                            "fold_status": "proven-fold-alias",
                        }
                    },
                }
            }
            if name == "symbols"
            else {},
        )
        with patch.object(vc5_build.ProgressDocument, "load", return_value=document):
            catalog = vc5_build.build_linked_retail_identity_catalog(
                target=target,
                progress_path=Path("progress.json"),
            )
        self.assertEqual(1, len(catalog.functions))
        self.assertEqual(identity, catalog.functions[0].logical_identity_key)
        self.assertEqual("?UnitAlias@@YAXXZ", catalog.functions[0].symbol)

    def test_retired_validation_and_compile_cache_surface_is_absent(self) -> None:
        source = Path(vc5_build.__file__).read_text(encoding="utf-8").casefold()
        for token in RETIRED_SURFACE_TOKENS:
            with self.subTest(token=token):
                self.assertNotIn(token, source)

    def test_final_build_config_has_no_reference_or_compile_bundle_state(self) -> None:
        fields = {field.name for field in dataclasses.fields(vc5_build.FinalBuildConfig)}
        self.assertNotIn("reference_exe", fields)
        self.assertNotIn("reference_manifest", fields)
        self.assertNotIn("compile_bundle_dir", fields)
        self.assertNotIn("compile_bundle_explicit", fields)

    def test_playtest_manifest_field_is_optional_and_repo_local(self) -> None:
        canonical = vc5_build.load_config(vc5_build.DEFAULT_MANIFEST)
        self.assertEqual(
            "playground/Recoil-rebuild.exe",
            json.loads(vc5_build.DEFAULT_MANIFEST.read_text(encoding="utf-8"))[
                "playtest_output_exe"
            ],
        )
        self.assertEqual(
            vc5_build.DEFAULT_PLAYTEST_OUTPUT.resolve(),
            canonical.playtest_output_exe.resolve(),
        )

        with self.temporary_directory() as temporary:
            manifest_path = Path(temporary) / "manifest.json"
            manifest = json.loads(
                vc5_build.DEFAULT_MANIFEST.read_text(encoding="utf-8")
            )
            manifest.pop("playtest_output_exe")
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            self.assertIsNone(vc5_build.load_config(manifest_path).playtest_output_exe)

            for invalid in (17, "", "../outside-Recoil-rebuild.exe"):
                with self.subTest(invalid=invalid):
                    manifest["playtest_output_exe"] = invalid
                    manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
                    with self.assertRaisesRegex(ValueError, "playtest_output_exe"):
                        vc5_build.load_config(manifest_path)

    def test_playtest_deployment_eligibility_matrix(self) -> None:
        canonical = vc5_build.load_config(vc5_build.DEFAULT_MANIFEST)
        normal_invocation = {
            "dry_run": False,
            "compile_only": False,
            "linked_order_only": False,
            "diagnostic_isolation_applied": False,
        }
        self.assertTrue(
            vc5_build.playtest_deployment_eligible(
                canonical,
                **normal_invocation,
            )
        )

        cases = (
            ("dry-run", canonical, {"dry_run": True}),
            ("compile-only", canonical, {"compile_only": True}),
            ("linked-order-only", canonical, {"linked_order_only": True}),
            (
                "diagnostic-isolation",
                canonical,
                {"diagnostic_isolation_applied": True},
            ),
            (
                "custom-manifest",
                dataclasses.replace(canonical, manifest_path=Path("custom.json")),
                {},
            ),
            (
                "messages-dll",
                dataclasses.replace(canonical, output_exe="messages.dll"),
                {},
            ),
            (
                "wrong-destination",
                dataclasses.replace(
                    canonical,
                    playtest_output_exe=ROOT / "playground" / "other.exe",
                ),
                {},
            ),
            (
                "missing-destination",
                dataclasses.replace(canonical, playtest_output_exe=None),
                {},
            ),
            (
                "diagnostic-only",
                dataclasses.replace(canonical, diagnostic_only=True),
                {},
            ),
            (
                "compile-profile",
                dataclasses.replace(canonical, compile_profile="probe"),
                {},
            ),
            (
                "link-profile",
                dataclasses.replace(canonical, link_profile="probe"),
                {},
            ),
            (
                "library-profile",
                dataclasses.replace(canonical, library_profile="probe"),
                {},
            ),
        )
        for name, config, invocation_override in cases:
            with self.subTest(name=name):
                invocation = {**normal_invocation, **invocation_override}
                self.assertFalse(
                    vc5_build.playtest_deployment_eligible(
                        config,
                        **invocation,
                    )
                )

    def test_playtest_deploy_atomically_replaces_destination(self) -> None:
        with self.temporary_directory() as temporary:
            root = Path(temporary)
            candidate = root / "candidate.exe"
            destination = root / "Recoil-rebuild.exe"
            candidate.write_bytes(b"new candidate")
            destination.write_bytes(b"old playtest")
            real_replace = vc5_build.os.replace
            replacements: list[tuple[Path, Path]] = []

            def observed_replace(source: str | Path, target: str | Path) -> None:
                replacements.append((Path(source), Path(target)))
                real_replace(source, target)

            stdout = io.StringIO()
            with patch.object(
                vc5_build.os,
                "replace",
                side_effect=observed_replace,
            ), redirect_stdout(stdout):
                result = vc5_build.deploy_playtest_candidate(candidate, destination)

            self.assertEqual(
                {"attempted", "updated", "destination", "error"},
                set(result),
            )
            self.assertEqual(
                {
                    "attempted": True,
                    "updated": True,
                    "destination": str(destination.resolve()),
                    "error": None,
                },
                result,
            )
            self.assertEqual(b"new candidate", destination.read_bytes())
            self.assertEqual(1, len(replacements))
            self.assertEqual(destination, replacements[0][1])
            self.assertEqual(destination.parent, replacements[0][0].parent)
            self.assertFalse(replacements[0][0].exists())
            self.assertIn("Play-test binary updated (play-test only):", stdout.getvalue())

    def test_playtest_deploy_failure_preserves_target_and_cleans_temporary(self) -> None:
        with self.temporary_directory() as temporary:
            root = Path(temporary)
            candidate = root / "candidate.exe"
            destination = root / "Recoil-rebuild.exe"
            candidate.write_bytes(b"new candidate")
            destination.write_bytes(b"old playtest")
            stderr = io.StringIO()

            with patch.object(
                vc5_build.os,
                "replace",
                side_effect=PermissionError("locked"),
            ), redirect_stderr(stderr):
                result = vc5_build.deploy_playtest_candidate(candidate, destination)

            self.assertTrue(result["attempted"])
            self.assertFalse(result["updated"])
            self.assertIn("PermissionError", result["error"])
            self.assertEqual(b"old playtest", destination.read_bytes())
            self.assertEqual(
                {candidate.name, destination.name},
                {path.name for path in root.iterdir()},
            )
            self.assertIn("non-gating", stderr.getvalue())
            self.assertIn("existing target preserved", stderr.getvalue())

    def test_playtest_copy_failure_preserves_target_and_cleans_temporary(self) -> None:
        with self.temporary_directory() as temporary:
            root = Path(temporary)
            candidate = root / "candidate.exe"
            destination = root / "Recoil-rebuild.exe"
            candidate.write_bytes(b"new candidate")
            destination.write_bytes(b"old playtest")
            stderr = io.StringIO()

            with patch.object(
                vc5_build.shutil,
                "copyfile",
                side_effect=OSError("copy failed"),
            ), redirect_stderr(stderr):
                result = vc5_build.deploy_playtest_candidate(candidate, destination)

            self.assertTrue(result["attempted"])
            self.assertFalse(result["updated"])
            self.assertIn("OSError", result["error"])
            self.assertEqual(b"old playtest", destination.read_bytes())
            self.assertEqual(
                {candidate.name, destination.name},
                {path.name for path in root.iterdir()},
            )
            self.assertIn("non-gating", stderr.getvalue())

    def test_playtest_deploy_missing_directory_is_non_gating(self) -> None:
        with self.temporary_directory() as temporary:
            root = Path(temporary)
            candidate = root / "candidate.exe"
            candidate.write_bytes(b"new candidate")
            destination = root / "missing" / "Recoil-rebuild.exe"
            stderr = io.StringIO()

            with redirect_stderr(stderr):
                result = vc5_build.deploy_playtest_candidate(candidate, destination)

            self.assertEqual(
                {
                    "attempted": True,
                    "updated": False,
                    "destination": str(destination.resolve()),
                    "error": f"play-test directory is missing: {destination.parent.resolve()}",
                },
                result,
            )
            self.assertFalse(destination.parent.exists())
            self.assertIn("non-gating", stderr.getvalue())

    def test_normal_final_summary_records_non_gating_playtest_failure(self) -> None:
        with self.temporary_directory() as temporary:
            build_root = Path(temporary) / "run"
            canonical = vc5_build.load_config(vc5_build.DEFAULT_MANIFEST)
            config = dataclasses.replace(
                canonical,
                build_dir=build_root,
                build_dir_explicit=True,
                canonical_mfc=None,
                sources=(ROOT / "src" / "Battlesport" / "ai_net.cpp",),
                coff_alias_sources=(),
                source_compile_profiles=(),
            )
            paths = vc5_build.build_paths(config)
            deploy_result = {
                "attempted": True,
                "updated": False,
                "destination": str(vc5_build.DEFAULT_PLAYTEST_OUTPUT.resolve()),
                "error": "PermissionError: locked",
            }

            def fake_run_command(spec: vc5_build.CommandSpec) -> vc5_build.CommandResult:
                spec.stdout_log.parent.mkdir(parents=True, exist_ok=True)
                spec.stdout_log.write_text("", encoding="utf-8")
                spec.stderr_log.write_text("", encoding="utf-8")
                if spec.name == "link":
                    paths.exe_path.write_bytes(b"candidate")
                    paths.map_path.write_text("map", encoding="utf-8")
                return vc5_build.CommandResult(
                    name=spec.name,
                    returncode=0,
                    stdout_log=spec.stdout_log,
                    stderr_log=spec.stderr_log,
                )

            def fake_deploy(_candidate: Path, _destination: Path) -> dict[str, object]:
                print(
                    "WARNING: play-test deployment was not updated (non-gating)",
                    file=sys.stderr,
                )
                return deploy_result

            stderr = io.StringIO()
            with patch.object(
                vc5_build,
                "ensure_inputs_exist",
                return_value=[],
            ), patch.object(
                vc5_build,
                "run_command",
                side_effect=fake_run_command,
            ), patch.object(
                vc5_build,
                "run_linked_order_targets",
                return_value=0,
            ), patch.object(
                vc5_build,
                "deploy_playtest_candidate",
                side_effect=fake_deploy,
            ) as deploy, redirect_stderr(stderr):
                returncode = vc5_build.run_build(
                    config,
                    clean=False,
                    dry_run=False,
                    compile_only=False,
                    keep_going=False,
                    required_order_targets_override=(),
                )

            self.assertEqual(0, returncode)
            deploy.assert_called_once_with(
                paths.exe_path,
                canonical.playtest_output_exe,
            )
            summary = json.loads(paths.summary_path.read_text(encoding="utf-8"))
            self.assertTrue(summary["success"])
            self.assertEqual(deploy_result, summary["playtest_deploy"])
            self.assertEqual(str(paths.exe_path.resolve()), summary["candidate_path"])
            self.assertEqual(
                {"attempted", "updated", "destination", "error"},
                set(summary["playtest_deploy"]),
            )
            self.assertIn("non-gating", stderr.getvalue())

    def test_failed_final_build_never_attempts_playtest_deploy(self) -> None:
        with self.temporary_directory() as temporary:
            build_root = Path(temporary) / "run"
            canonical = vc5_build.load_config(vc5_build.DEFAULT_MANIFEST)
            config = dataclasses.replace(
                canonical,
                build_dir=build_root,
                build_dir_explicit=True,
                canonical_mfc=None,
                sources=(ROOT / "src" / "Battlesport" / "ai_net.cpp",),
                coff_alias_sources=(),
                source_compile_profiles=(),
            )

            def fake_run_command(spec: vc5_build.CommandSpec) -> vc5_build.CommandResult:
                spec.stdout_log.parent.mkdir(parents=True, exist_ok=True)
                spec.stdout_log.write_text("", encoding="utf-8")
                spec.stderr_log.write_text("", encoding="utf-8")
                return vc5_build.CommandResult(
                    name=spec.name,
                    returncode=9 if spec.name == "link" else 0,
                    stdout_log=spec.stdout_log,
                    stderr_log=spec.stderr_log,
                )

            deploy = Mock()
            with patch.object(
                vc5_build,
                "ensure_inputs_exist",
                return_value=[],
            ), patch.object(
                vc5_build,
                "run_command",
                side_effect=fake_run_command,
            ), patch.object(
                vc5_build,
                "deploy_playtest_candidate",
                deploy,
            ):
                returncode = vc5_build.run_build(
                    config,
                    clean=False,
                    dry_run=False,
                    compile_only=False,
                    keep_going=False,
                    required_order_targets_override=(),
                )

            self.assertEqual(9, returncode)
            deploy.assert_not_called()

    def test_build_paths_keep_fresh_outputs_under_one_selected_root(self) -> None:
        config = vc5_build.load_config(vc5_build.DEFAULT_MANIFEST)
        config = vc5_build.with_explicit_build_dir(
            config,
            Path("build/live-validation/vc5-build-live-cleanup-unit"),
        )
        paths = vc5_build.build_paths(config)
        root = paths.build_dir.resolve()
        for output in (
            paths.obj_dir,
            paths.logs_dir,
            paths.rsp_dir,
            paths.exe_path,
            paths.map_path,
            paths.resource_path,
            paths.message_dir,
            paths.summary_path,
        ):
            with self.subTest(output=output):
                self.assertTrue(output.resolve().is_relative_to(root))

    def test_explicit_clean_proves_selected_root_was_removed(self) -> None:
        with self.temporary_directory() as temporary:
            build_root = Path(temporary) / "run"
            stale = build_root / "obj" / "stale.obj"
            stale.parent.mkdir(parents=True)
            stale.write_bytes(b"stale")
            config = dataclasses.replace(
                vc5_build.load_config(vc5_build.DEFAULT_MANIFEST),
                build_dir=build_root,
                build_dir_explicit=True,
            )
            paths = vc5_build.build_paths(config)

            vc5_build.prepare_build_root(paths, clean=True, dry_run=False)

            self.assertFalse(build_root.exists())

    def test_explicit_clean_fails_closed_when_removal_does_not_take_effect(self) -> None:
        with self.temporary_directory() as temporary:
            build_root = Path(temporary) / "run"
            stale = build_root / "obj" / "stale.obj"
            stale.parent.mkdir(parents=True)
            stale.write_bytes(b"stale")
            config = dataclasses.replace(
                vc5_build.load_config(vc5_build.DEFAULT_MANIFEST),
                build_dir=build_root,
                build_dir_explicit=True,
            )
            paths = vc5_build.build_paths(config)

            with patch.object(vc5_build.shutil, "rmtree", return_value=None):
                with self.assertRaisesRegex(ValueError, "did not remove"):
                    vc5_build.prepare_build_root(paths, clean=True, dry_run=False)

    def test_linked_order_report_is_live_semantic_report(self) -> None:
        check = vc5_build.LinkedOrderCheck(
            target_name="unit-target",
            interval_name="unit-interval",
            contributions=(),
            diagnostics=(),
        )
        report = vc5_build.linked_order_report_data(check)
        self.assertEqual(report["report_version"], 1)
        self.assertEqual(report["kind"], "linked-function-order-report")
        serialized = repr(report).casefold()
        for token in RETIRED_SURFACE_TOKENS:
            with self.subTest(token=token):
                self.assertNotIn(token, serialized)

    def test_parser_exposes_only_fresh_build_and_live_order_controls(self) -> None:
        option_strings = {
            option
            for action in vc5_build.build_parser()._actions
            for option in action.option_strings
        }
        self.assertIn("--build-dir", option_strings)
        self.assertIn("--clean", option_strings)
        self.assertIn("--compile-only", option_strings)
        self.assertIn("--order-target", option_strings)
        self.assertIn("--linked-order-only", option_strings)
        self.assertNotIn("--no-pe-compare", option_strings)
        self.assertNotIn("--no-resource-compare", option_strings)
        self.assertNotIn("--reuse" + "-compile", option_strings)
        self.assertNotIn("--compile-bundle-dir", option_strings)
        self.assertNotIn("--order-receipt-dir", option_strings)


if __name__ == "__main__":
    unittest.main()
