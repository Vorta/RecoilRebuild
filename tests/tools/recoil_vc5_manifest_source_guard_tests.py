import contextlib
import io
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands import vc5_manifest_source_guard as source_guard  # noqa: E402
from _recoil.commands import vc5_verify  # noqa: E402
from _recoil.commands.vc5_verify import SourceEmissionWarning  # noqa: E402
from _recoil.lib.progress import empty_progress_document  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402


def _commit_guard_fixture(repository_root: Path) -> None:
    if not (repository_root / ".git").exists():
        subprocess.run(
            ["git", "init", "--quiet"],
            cwd=repository_root,
            check=True,
            capture_output=True,
        )
    subprocess.run(
        ["git", "add", "-A"],
        cwd=repository_root,
        check=True,
        capture_output=True,
    )
    status = subprocess.run(
        ["git", "diff", "--cached", "--quiet"],
        cwd=repository_root,
        check=False,
        capture_output=True,
    )
    if status.returncode == 1:
        subprocess.run(
            [
                "git",
                "-c",
                "user.name=Recoil Tests",
                "-c",
                "user.email=recoil-tests@example.invalid",
                "commit",
                "--quiet",
                "-m",
                "tracked VC5 guard fixture",
            ],
            cwd=repository_root,
            check=True,
            capture_output=True,
        )
    elif status.returncode != 0:
        raise RuntimeError("cannot inspect staged VC5 guard fixture")


def guard_main(argv: list[str]) -> int:
    """Run temporary guard fixtures as authenticated isolated Git inputs."""

    if "--help" in argv:
        return source_guard.main(argv)
    manifest_path: Path | None = None
    manifest_dir: Path | None = None
    final_build: Path | None = None
    for index, argument in enumerate(argv):
        if argument == "--path":
            manifest_path = Path(argv[index + 1]).absolute()
        elif argument == "--manifest-dir":
            manifest_dir = Path(argv[index + 1]).absolute()
        elif argument == "--final-build-manifest":
            final_build = Path(argv[index + 1]).absolute()
    manifest_anchor = manifest_path.parent if manifest_path is not None else manifest_dir
    if manifest_anchor is None:
        return source_guard.main(argv)
    patched_vc5_root = Path(vc5_verify.REPO_ROOT).absolute()
    try:
        manifest_anchor.relative_to(patched_vc5_root)
    except ValueError:
        uses_patched_vc5_root = False
    else:
        uses_patched_vc5_root = patched_vc5_root != REPO_ROOT.absolute()
    repository_anchor = (
        patched_vc5_root
        if uses_patched_vc5_root
        else (manifest_anchor.parent if manifest_anchor.name == "targets" else manifest_anchor)
    )
    roots = [repository_anchor]
    if final_build is not None:
        roots.append(final_build.parent)
    repository_root = Path(os.path.commonpath([str(path) for path in roots]))
    _commit_guard_fixture(repository_root)
    effective_argv = list(argv)
    if final_build is None:
        effective_argv.extend(
            ["--final-build-manifest", str(repository_root / "absent-final-build.json")]
        )
    source_guard._guard_git_inventory.cache_clear()
    with (
        patch.object(source_guard, "REPO_ROOT", repository_root),
        patch.object(vc5_verify, "REPO_ROOT", repository_root),
    ):
        return source_guard.main(effective_argv)


def write_source_manifest(directory: Path) -> None:
    source_path = directory / "source.cpp"
    source_path.write_text(
        "// Reimplements 0x401000: Sample\nint __cdecl Sample() { return 0; }\n",
        encoding="utf-8",
    )
    (directory / "sample.json").write_text(
        json.dumps(
            {
                "name": "sample",
                "description": "sample",
                "source_filename": "source.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP"],
                "functions": [
                    {"address": "0x401000", "symbol": "?Sample@@YAHXZ", "name": "Sample"}
                ],
            }
        ),
        encoding="utf-8",
    )


def write_inline_manifest(directory: Path) -> None:
    (directory / "sample.json").write_text(
        json.dumps(
            {
                "name": "sample",
                "description": "sample",
                "source_filename": "source.cpp",
                "compiler_flags": ["/nologo", "/TP"],
                "functions": [
                    {"address": "0x401000", "symbol": "?Sample@@YAHXZ", "name": "Sample"}
                ],
                "source": ["int __cdecl Sample() { return 0; }"],
            }
        ),
        encoding="utf-8",
    )


def write_progress(path: Path, source_paths: list[str]) -> None:
    document = empty_progress_document()
    document["physical_blocks"] = {
        f"recoil:block:{index}": {
            "binary": "recoil",
            "row_kind": "physical-block",
            "agent_source_path": source_path,
        }
        for index, source_path in enumerate(source_paths)
    }
    ProgressSQLiteStore.create_from_mapping(
        path,
        document,
        cutover_pair_id="vc5-manifest-source-guard-test",
    )


def linked_interval() -> dict[str, object]:
    return {
        "name": "sample_interval",
        "predecessor": {
            "address": "0x400ff0",
            "symbol": "?Before@@YAHXZ",
            "name": "Before",
        },
        "functions": [
            {
                "address": "0x401000",
                "symbol": "?Body@@YAHXZ",
                "name": "Body",
            }
        ],
        "successor": {
            "address": "0x401010",
            "symbol": "?After@@YAHXZ",
            "name": "After",
        },
    }


def write_linked_only_manifest(directory: Path, *, generated_files: bool = False) -> None:
    data: dict[str, object] = {
        "name": "linked_only",
        "description": "final-link metadata only",
        "linked_function_intervals": [linked_interval()],
    }
    if generated_files:
        data["generated_files"] = {"src/forbidden_shadow.h": "int shadow;\n"}
    (directory / "linked_only.json").write_text(json.dumps(data), encoding="utf-8")


def linked_order_diagnostic_mode() -> dict[str, object]:
    return {
        "kind": "ref-noicf-controlled-identity-order",
        "required_link_profile": "vc5sp3_ref_noicf",
        "nonblocking_reason": "noicf_expands_declared_icf_fold_families",
        "nonblocking_predicates": [
            "block_precedence",
            "boundary_sentinels",
            "declared_icf_fold_family_geometry",
        ],
        "controlled_identities": [
            {
                "name": "Body A",
                "symbol": "?BodyA@@YAHXZ",
                "expected_object": "sample.obj",
            },
            {
                "name": "Body B",
                "symbol": "?BodyB@@YAHXZ",
                "expected_object": "sample.obj",
            },
        ],
        "forbidden_objects": ["forbidden.obj"],
    }


def write_diagnostic_overlay(directory: Path) -> tuple[Path, Path]:
    base_path = directory / "linked_base.json"
    base_path.write_text(
        json.dumps(
            {
                "name": "linked_base",
                "description": "linked base",
                "target_binary": "recoil",
                "linked_function_intervals": [linked_interval()],
            }
        ),
        encoding="utf-8",
    )
    overlay_path = directory / "linked_overlay.json"
    overlay_path.write_text(
        json.dumps(
            {
                "name": "linked_overlay",
                "description": "diagnostic overlay",
                "target_binary": "recoil",
                "linked_order_base_target": "linked_base",
                "linked_order_diagnostic_mode": linked_order_diagnostic_mode(),
            }
        ),
        encoding="utf-8",
    )
    return base_path, overlay_path


def write_provider_boundary_manifest(directory: Path, *, provenance: str = "provider-boundary") -> None:
    source_path = directory / "provider.cpp"
    source_path.write_text(
        "// Provider boundary 0x401000: Sample provider thunk.\nvoid __cdecl Sample() {}\n",
        encoding="utf-8",
    )
    (directory / "sample.json").write_text(
        json.dumps(
            {
                "name": "sample",
                "description": "sample",
                "source_filename": "provider.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP"],
                "functions": [
                    {
                        "address": "0x401000",
                        "symbol": "?Sample@@YAXXZ",
                        "name": "Sample",
                        "provenance": provenance,
                    }
                ],
            }
        ),
        encoding="utf-8",
    )


def write_compiler_emitted_manifest(
    directory: Path,
    *,
    source_marker: str = "// Compiler-emitted 0x401000: Sample compiler glue.",
) -> None:
    source_path = directory / "compiler.cpp"
    source_path.write_text(
        f"{source_marker}\nvoid __cdecl Sample() {{}}\n",
        encoding="utf-8",
    )
    (directory / "sample.json").write_text(
        json.dumps(
            {
                "name": "sample",
                "description": "sample",
                "source_filename": "compiler.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP"],
                "functions": [
                    {
                        "address": "0x401000",
                        "symbol": "?Sample@@YAXXZ",
                        "name": "Sample",
                        "provenance": "compiler-emitted-noncovering",
                    }
                ],
            }
        ),
        encoding="utf-8",
    )


def write_generated_role_manifest(directory: Path, *, include_anchor: bool) -> None:
    source_dir = directory.parent / "src"
    source_dir.mkdir()
    source_path = source_dir / "generated.cpp"
    source_path.write_text('#include "generated.h"\n', encoding="utf-8")
    marker = (
        "/**\n"
        " * @recoil-anchor recoil:anchor:generated.type\n"
        " * @recoil-artifact emits .text recoil:function:0x401000: "
        "VC5 scalar deleting destructor for Generated.\n"
        " */"
        if include_anchor
        else "/** Emits 0x401000: VC5 scalar deleting destructor for Generated. */"
    )
    (source_dir / "generated.h").write_text(
        f"{marker}\nclass Generated {{ public: virtual ~Generated(); }};\n",
        encoding="utf-8",
    )
    function: dict[str, object] = {
        "address": "0x401000",
        "symbol": "??_GGenerated@@UAEPAXI@Z",
        "name": "Generated scalar deleting destructor",
        "pipeline_class": "authored-lifecycle",
        "authored_order_role": "compiler-generated-deleting-variant",
        "provenance": "compiler-emitted-noncovering",
    }
    if include_anchor:
        function["emission_anchor"] = {
            "path": "src/generated.h",
            "kind": "type-definition",
            "name": "Generated",
        }
    (directory / "generated.json").write_text(
        json.dumps(
            {
                "name": "generated",
                "description": "generated",
                "source_filename": "generated.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP"],
                "functions": [function],
            }
        ),
        encoding="utf-8",
    )


def write_data_thunk_manifest(
    directory: Path,
    *,
    relation: str = "emits",
    anchor_name: str = "g_Generated",
) -> None:
    source_dir = directory.parent / "src"
    source_dir.mkdir()
    source_path = source_dir / "generated.cpp"
    source_path.write_text(
        "/**\n"
        " * @recoil-anchor recoil:anchor:generated.data\n"
        f" * @recoil-artifact {relation} .text recoil:function:0x401000: "
        "VC5 static-lifetime thunk for g_Generated.\n"
        " */\n"
        "int g_Generated;\n",
        encoding="utf-8",
    )
    (directory / "generated.json").write_text(
        json.dumps(
            {
                "name": "generated",
                "description": "data-defined generated thunk",
                "source_filename": "generated.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP"],
                "functions": [
                    {
                        "address": "0x401000",
                        "symbol": "_$E1",
                        "name": "g_Generated static-lifetime thunk",
                        "pipeline_class": "authored-lifecycle",
                        "authored_order_role": "compiler-generated-thunk",
                        "provenance": "compiler-emitted-noncovering",
                        "emission_anchor": {
                            "path": "src/generated.cpp",
                            "kind": "data-definition",
                            "name": anchor_name,
                        },
                    }
                ],
            }
        ),
        encoding="utf-8",
    )


class RecoilVc5ManifestSourceGuardTests(unittest.TestCase):
    def test_repo_manifest_key_requires_exact_relative_git_identity(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest = root / "tools" / "vc5_verify_targets" / "Exact.json"
            manifest.parent.mkdir(parents=True)
            manifest.write_text('{"name":"exact"}\n', encoding="utf-8")
            _commit_guard_fixture(root)
            source_guard._guard_git_inventory.cache_clear()
            with patch.object(source_guard, "REPO_ROOT", root):
                self.assertEqual(
                    "tools/vc5_verify_targets/Exact.json",
                    source_guard.repo_manifest_key(
                        "tools/vc5_verify_targets/Exact.json"
                    ),
                )
                self.assertEqual(
                    "tools/vc5_verify_targets/Exact.json",
                    source_guard._repo_manifest_key_from_filesystem_path(manifest),
                )
                with self.assertRaisesRegex(ValueError, "expected 'tools/vc5_verify_targets/Exact.json'"):
                    source_guard.repo_manifest_key(
                        "tools/vc5_verify_targets/exact.json"
                    )
                with self.assertRaisesRegex(ValueError, "does not exist as a Git-tracked path"):
                    source_guard.repo_manifest_key(
                        "tools/vc5_verify_targets/missing.json"
                    )
                with self.assertRaisesRegex(ValueError, "repository-relative Git path"):
                    source_guard.repo_manifest_key(manifest)
                with self.assertRaisesRegex(ValueError, "not normalized"):
                    source_guard.repo_manifest_key("../escape.json")

    def test_strict_guard_uses_one_manifest_pass_and_does_not_fail_unresolved_warnings(
        self,
    ) -> None:
        warning = SourceEmissionWarning(
            address="0x401000",
            code="missing-source-emission-anchor",
            source_from="src/sample.cpp",
            message=(
                "compiler-generated row is explicitly unresolved tracker debt"
            ),
        )
        target = SimpleNamespace(
            manifest_path=Path("sample.json"),
            source_emission_warnings=(warning,),
        )
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            (manifest_dir / "sample.json").write_text(
                '{"name":"sample"}', encoding="utf-8"
            )
            stdout = io.StringIO()
            with (
                patch(
                    "_recoil.commands.vc5_manifest_source_guard.load_manifests",
                    return_value=[target],
                ) as load_manifests_mock,
                patch(
                    "_recoil.commands.vc5_manifest_source_guard."
                    "print_source_emission_warnings"
                ),
                patch(
                    "_recoil.commands.vc5_manifest_source_guard."
                    "target_source_fragment_findings",
                    return_value=[],
                ),
                patch(
                    "_recoil.commands.vc5_manifest_source_guard."
                    "actual_policy_debt",
                    return_value=(set(), set()),
                ),
                patch(
                    "_recoil.commands.vc5_manifest_source_guard."
                    "active_mfc_path_debt",
                    return_value=set(),
                ),
                contextlib.redirect_stdout(stdout),
            ):
                result = guard_main(
                    [
                        "--manifest-dir",
                        str(manifest_dir),
                        "--skip-final-build-source-audit",
                        "--strict-source-emissions",
                    ]
                )

        self.assertEqual(0, result)
        load_manifests_mock.assert_called_once_with(
            manifest_dir,
            strict_source_emissions=True,
            strict_source_traceability=False,
        )
        self.assertIn("1 source-emission warning(s)", stdout.getvalue())

    def test_strict_source_emissions_help_does_not_require_legacy_marker(self) -> None:
        stdout = io.StringIO()
        with (
            contextlib.redirect_stdout(stdout),
            self.assertRaises(SystemExit) as exit_context,
        ):
            guard_main(["--help"])

        self.assertEqual(0, exit_context.exception.code)
        help_text = stdout.getvalue()
        self.assertIn("valid source-anchored emission_anchor metadata", help_text)
        self.assertIn("supplied legacy emission markers", help_text)
        self.assertIn("when present", help_text)
        self.assertNotIn("legacy or canonical emission marker", help_text)

    def test_guard_rejects_transitive_repo_local_source_fragment_closure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_dir = root / "src"
            source_dir.mkdir()
            source = source_dir / "sample.cpp"
            source.write_text(
                '// Reimplements 0x401000: Sample\n#include "sample_body.h"\n',
                encoding="utf-8",
            )
            (source_dir / "sample_body.h").write_text(
                "int __cdecl Sample() { return 0; }\n",
                encoding="utf-8",
            )
            manifest = root / "sample.json"
            manifest.write_text(
                json.dumps(
                    {
                        "name": "sample",
                        "description": "sample",
                        "source_filename": "sample.cpp",
                        "source_from": str(source),
                        "compiler_flags": ["/nologo", "/TP"],
                        "functions": [
                            {"address": "0x401000", "symbol": "?Sample@@YAHXZ", "name": "Sample"}
                        ],
                    }
                ),
                encoding="utf-8",
            )
            stderr = io.StringIO()
            with (
                patch("_recoil.commands.vc5_verify.REPO_ROOT", root),
                contextlib.redirect_stderr(stderr),
            ):
                result = guard_main(["--path", str(manifest)])

        self.assertEqual(1, result)
        self.assertIn("vc5-source-fragment-fragment-include-edge", stderr.getvalue())

    def test_guard_warns_for_generated_role_debt_by_default_and_strict_mode_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_generated_role_manifest(manifest_dir, include_anchor=False)
            stderr = io.StringIO()

            with contextlib.redirect_stderr(stderr):
                compatibility_result = guard_main(
                    ["--manifest-dir", str(manifest_dir), "--skip-final-build-source-audit"]
                )
                strict_result = guard_main(
                    [
                        "--manifest-dir",
                        str(manifest_dir),
                        "--skip-final-build-source-audit",
                        "--strict-source-emissions",
                    ]
                )

        self.assertEqual(0, compatibility_result)
        self.assertEqual(1, strict_result)
        self.assertIn("source-emission-warning", stderr.getvalue())
        self.assertIn("strict-source-emission-debt", stderr.getvalue())

    def test_guard_strict_mode_accepts_valid_source_emission_anchor(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            write_generated_role_manifest(manifest_dir, include_anchor=True)

            with patch("_recoil.commands.vc5_verify.REPO_ROOT", root):
                result = guard_main(
                    [
                        "--manifest-dir",
                        str(manifest_dir),
                        "--skip-final-build-source-audit",
                        "--strict-source-emissions",
                    ]
                )

        self.assertEqual(0, result)

    def test_guard_strict_mode_accepts_data_defined_compiler_thunk(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            write_data_thunk_manifest(manifest_dir)

            with patch("_recoil.commands.vc5_verify.REPO_ROOT", root):
                result = guard_main(
                    [
                        "--manifest-dir",
                        str(manifest_dir),
                        "--skip-final-build-source-audit",
                        "--strict-source-emissions",
                        "--strict-source-traceability",
                    ]
                )

        self.assertEqual(0, result)

    def test_guard_strict_mode_rejects_data_thunk_defines_or_wrong_anchor_name(self) -> None:
        cases = {
            "defines": ({"relation": "defines"}, "wrong-relation"),
            "wrong-name": ({"anchor_name": "g_Missing"}, "emission_anchor.name"),
        }
        for label, (kwargs, expected) in cases.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                manifest_dir = root / "targets"
                manifest_dir.mkdir()
                write_data_thunk_manifest(manifest_dir, **kwargs)
                stderr = io.StringIO()

                with (
                    patch("_recoil.commands.vc5_verify.REPO_ROOT", root),
                    contextlib.redirect_stderr(stderr),
                ):
                    result = guard_main(
                        [
                            "--manifest-dir",
                            str(manifest_dir),
                            "--skip-final-build-source-audit",
                            "--strict-source-emissions",
                            "--strict-source-traceability",
                        ]
                    )

            self.assertEqual(1, result)
            self.assertIn(expected, stderr.getvalue())

    def test_guard_strict_mode_rejects_legacy_generated_emits_inventory(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            write_generated_role_manifest(manifest_dir, include_anchor=True)
            (root / "src" / "generated.h").write_text(
                "/** Emits 0x401000: Legacy generated inventory. */\n"
                "class Generated { public: virtual ~Generated(); };\n",
                encoding="utf-8",
            )
            stderr = io.StringIO()

            with (
                patch("_recoil.commands.vc5_verify.REPO_ROOT", root),
                contextlib.redirect_stderr(stderr),
            ):
                result = guard_main(
                    [
                        "--manifest-dir",
                        str(manifest_dir),
                        "--skip-final-build-source-audit",
                        "--strict-source-traceability",
                    ]
                )

        self.assertEqual(1, result)
        self.assertIn("legacy Emits inventory", stderr.getvalue())

    def test_guard_strict_mode_rejects_legacy_authored_reimplements_inventory(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_source_manifest(manifest_dir)
            manifest_path = manifest_dir / "sample.json"
            manifest_data = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest_data["functions"][0]["pipeline_class"] = "authored"
            manifest_path.write_text(json.dumps(manifest_data), encoding="utf-8")
            stderr = io.StringIO()

            with contextlib.redirect_stderr(stderr):
                result = guard_main(
                    [
                        "--manifest-dir",
                        str(manifest_dir),
                        "--skip-final-build-source-audit",
                        "--strict-source-traceability",
                    ]
                )

        self.assertEqual(1, result)
        self.assertIn("legacy Reimplements markers are migration inventory only", stderr.getvalue())

    def test_guard_accepts_linked_only_manifest_without_source_fields(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_linked_only_manifest(manifest_dir)

            directory_result = guard_main(
                ["--manifest-dir", str(manifest_dir), "--skip-final-build-source-audit"]
            )
            focused_result = guard_main(["--path", str(manifest_dir / "linked_only.json")])

        self.assertEqual(0, directory_result)
        self.assertEqual(0, focused_result)

    def test_guard_rejects_project_header_shadow_in_linked_only_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_linked_only_manifest(manifest_dir, generated_files=True)

            result = guard_main(
                ["--manifest-dir", str(manifest_dir), "--skip-final-build-source-audit"]
            )

        self.assertEqual(1, result)

    def test_guard_accepts_structurally_valid_diagnostic_overlay(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            _, overlay_path = write_diagnostic_overlay(manifest_dir)

            directory_result = guard_main(
                ["--manifest-dir", str(manifest_dir), "--skip-final-build-source-audit"]
            )
            focused_result = guard_main(["--path", str(overlay_path)])

        self.assertEqual(0, directory_result)
        self.assertEqual(0, focused_result)

    def test_guard_rejects_diagnostic_overlay_with_missing_base_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            base_path, overlay_path = write_diagnostic_overlay(manifest_dir)
            base_path.unlink()

            result = guard_main(["--path", str(overlay_path)])

        self.assertEqual(1, result)

    def test_guard_rejects_missing_empty_or_malformed_overlay_pairing(self) -> None:
        cases = {
            "missing-base-field": lambda data: data.pop("linked_order_base_target"),
            "empty-base-field": lambda data: data.__setitem__("linked_order_base_target", ""),
            "malformed-base-field": lambda data: data.__setitem__("linked_order_base_target", 7),
            "missing-diagnostic-mode": lambda data: data.pop("linked_order_diagnostic_mode"),
            "malformed-diagnostic-mode": lambda data: data.__setitem__(
                "linked_order_diagnostic_mode", "diagnostic"
            ),
        }
        for label, mutate in cases.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as tmp:
                manifest_dir = Path(tmp) / "targets"
                manifest_dir.mkdir()
                _, overlay_path = write_diagnostic_overlay(manifest_dir)
                data = json.loads(overlay_path.read_text(encoding="utf-8"))
                mutate(data)
                overlay_path.write_text(json.dumps(data), encoding="utf-8")

                result = guard_main(["--path", str(overlay_path)])

            self.assertEqual(1, result)

    def test_guard_rejects_diagnostic_overlay_compile_payload(self) -> None:
        cases: dict[str, object] = {
            "source_from": "src/Battlesport/RecoilApp.cpp",
            "functions": [
                {
                    "address": "0x401000",
                    "symbol": "?Body@@YAHXZ",
                    "name": "Body",
                }
            ],
            "linked_function_intervals": [linked_interval()],
        }
        for field, value in cases.items():
            with self.subTest(field=field), tempfile.TemporaryDirectory() as tmp:
                manifest_dir = Path(tmp) / "targets"
                manifest_dir.mkdir()
                _, overlay_path = write_diagnostic_overlay(manifest_dir)
                data = json.loads(overlay_path.read_text(encoding="utf-8"))
                data[field] = value
                overlay_path.write_text(json.dumps(data), encoding="utf-8")

                result = guard_main(["--path", str(overlay_path)])

            self.assertEqual(1, result)

    def test_guard_rejects_diagnostic_overlay_unknown_payload(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            _, overlay_path = write_diagnostic_overlay(manifest_dir)
            data = json.loads(overlay_path.read_text(encoding="utf-8"))
            data["unexpected_payload"] = {"accepted": True}
            overlay_path.write_text(json.dumps(data), encoding="utf-8")

            result = guard_main(["--path", str(overlay_path)])

        self.assertEqual(1, result)

    def test_guard_rejects_diagnostic_overlay_generated_project_header_shadow(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            _, overlay_path = write_diagnostic_overlay(manifest_dir)
            data = json.loads(overlay_path.read_text(encoding="utf-8"))
            data["generated_files"] = {"src/Battlesport/RecoilApp.h": "int shadow;\n"}
            overlay_path.write_text(json.dumps(data), encoding="utf-8")

            result = guard_main(["--path", str(overlay_path)])

        self.assertEqual(1, result)

    def test_guard_rejects_diagnostic_overlay_base_without_linked_intervals(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            base_path, overlay_path = write_diagnostic_overlay(manifest_dir)
            source_path = manifest_dir / "base.cpp"
            source_path.write_text(
                "/** Reimplements 0x401000: Body. Purpose: Supply the test body. */\n"
                "int __cdecl Body() { return 0; }\n",
                encoding="utf-8",
            )
            base_path.write_text(
                json.dumps(
                    {
                        "name": "linked_base",
                        "description": "compile target without linked intervals",
                        "target_binary": "recoil",
                        "source_filename": "base.cpp",
                        "source_from": str(source_path),
                        "compiler_flags": ["/nologo", "/TP"],
                        "functions": [
                            {
                                "address": "0x401000",
                                "symbol": "?Body@@YAHXZ",
                                "name": "Body",
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )

            result = guard_main(["--path", str(overlay_path)])

        self.assertEqual(1, result)

    def test_guard_rejects_diagnostic_overlay_target_binary_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            _, overlay_path = write_diagnostic_overlay(manifest_dir)
            data = json.loads(overlay_path.read_text(encoding="utf-8"))
            data["target_binary"] = "messages"
            overlay_path.write_text(json.dumps(data), encoding="utf-8")

            result = guard_main(["--path", str(overlay_path)])

        self.assertEqual(1, result)

    def test_guard_preserves_inline_rejection_for_mixed_compile_and_linked_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_inline_manifest(manifest_dir)
            path = manifest_dir / "sample.json"
            data = json.loads(path.read_text(encoding="utf-8"))
            data["linked_function_intervals"] = [linked_interval()]
            path.write_text(json.dumps(data), encoding="utf-8")

            result = guard_main(
                ["--manifest-dir", str(manifest_dir), "--skip-final-build-source-audit"]
            )

        self.assertEqual(1, result)

    def test_guard_accepts_source_from_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_source_manifest(manifest_dir)

            result = guard_main(["--manifest-dir", str(manifest_dir), "--skip-final-build-source-audit"])

        self.assertEqual(0, result)

    def test_guard_rejects_inline_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_inline_manifest(manifest_dir)

            result = guard_main(["--manifest-dir", str(manifest_dir), "--skip-final-build-source-audit"])

        self.assertEqual(1, result)

    def test_guard_rejects_physical_block_path_missing_from_final_build(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            write_source_manifest(manifest_dir)
            final_manifest = root / "final.json"
            final_manifest.write_text(
                json.dumps({"sources": ["src/Present.cpp"]}),
                encoding="utf-8",
            )
            progress = root / "progress.sqlite3"
            write_progress(progress, ["src/Missing.cpp"])

            result = guard_main(
                [
                    "--manifest-dir",
                    str(manifest_dir),
                    "--final-build-manifest",
                    str(final_manifest),
                    "--progress",
                    str(progress),
                ]
            )

        self.assertEqual(1, result)

    def test_guard_accepts_final_build_physical_block_exclusion_with_reason(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            write_source_manifest(manifest_dir)
            final_manifest = root / "final.json"
            final_manifest.write_text(
                json.dumps(
                    {
                        "sources": ["src/Present.cpp"],
                        "physical_block_source_exclusions": [
                            {"path": "src/Missing.cpp", "reason": "covered by split owner source"}
                        ],
                    }
                ),
                encoding="utf-8",
            )
            progress = root / "progress.sqlite3"
            write_progress(progress, ["src/Present.cpp", "src/Missing.cpp"])

            result = guard_main(
                [
                    "--manifest-dir",
                    str(manifest_dir),
                    "--final-build-manifest",
                    str(final_manifest),
                    "--progress",
                    str(progress),
                ]
            )

        self.assertEqual(0, result)

    def test_guard_rejects_old_support_mfc_include_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_source_manifest(manifest_dir)
            path = manifest_dir / "sample.json"
            data = json.loads(path.read_text(encoding="utf-8"))
            data["include_dirs"] = ["support/sdk/MFC42/Include"]
            path.write_text(json.dumps(data), encoding="utf-8")

            result = guard_main(
                ["--manifest-dir", str(manifest_dir), "--skip-final-build-source-audit"]
            )

        self.assertEqual(1, result)

    def test_guard_rejects_final_build_exclusion_without_reason(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            write_source_manifest(manifest_dir)
            final_manifest = root / "final.json"
            final_manifest.write_text(
                json.dumps(
                    {
                        "sources": ["src/Present.cpp"],
                        "physical_block_source_exclusions": [{"path": "src/Missing.cpp"}],
                    }
                ),
                encoding="utf-8",
            )
            progress = root / "progress.sqlite3"
            write_progress(progress, ["src/Missing.cpp"])

            result = guard_main(
                [
                    "--manifest-dir",
                    str(manifest_dir),
                    "--final-build-manifest",
                    str(final_manifest),
                    "--progress",
                    str(progress),
                ]
            )

        self.assertEqual(1, result)

    def test_guard_accepts_focused_manifest_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_source_manifest(manifest_dir)

            result = guard_main(["--path", str(manifest_dir / "sample.json")])

        self.assertEqual(0, result)

    def test_guard_rejects_focused_inline_manifest_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_inline_manifest(manifest_dir)

            result = guard_main(["--path", str(manifest_dir / "sample.json")])

        self.assertEqual(1, result)

    def test_guard_accepts_normal_function_provider_boundary_marker(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_provider_boundary_manifest(manifest_dir)

            result = guard_main(["--path", str(manifest_dir / "sample.json")])

        self.assertEqual(0, result)

    def test_guard_accepts_provider_boundary_without_legacy_boundary_marker(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_source_manifest(manifest_dir)
            data = json.loads((manifest_dir / "sample.json").read_text(encoding="utf-8"))
            data["functions"][0]["provenance"] = "provider-boundary"
            (manifest_dir / "sample.json").write_text(json.dumps(data), encoding="utf-8")

            result = guard_main(["--path", str(manifest_dir / "sample.json")])

        self.assertEqual(0, result)

    def test_guard_accepts_normal_function_compiler_emitted_marker(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_compiler_emitted_manifest(manifest_dir)

            result = guard_main(["--path", str(manifest_dir / "sample.json")])

        self.assertEqual(0, result)

    def test_guard_accepts_compiler_emitted_without_matching_legacy_marker(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_provider_boundary_manifest(
                manifest_dir,
                provenance="compiler-emitted-noncovering",
            )

            result = guard_main(["--path", str(manifest_dir / "sample.json")])

        self.assertEqual(0, result)


if __name__ == "__main__":
    unittest.main()
