import json
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_vc6_verify import (  # noqa: E402
    SourcePolicyBaseline,
    VerifySelection,
    VerifyTarget,
    build_compile_command,
    covering_targets,
    find_target,
    group_selections_by_compile_key,
    load_manifest,
    load_manifests,
    manifest_skeleton,
    selected_targets_for_source_from,
)


def write_manifest(directory: Path, name: str = "sample") -> Path:
    source_path = directory / "sample.cpp"
    source_path.write_text(
        "// Reimplements 0x401000: Sample\nint __cdecl Sample() { return 1; }\n",
        encoding="utf-8",
    )
    path = directory / f"{name}.json"
    path.write_text(
        json.dumps(
            {
                "name": name,
                "description": "sample target",
                "source_filename": "sample_verify.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP", "/O2", "/FAcs"],
                "include_dirs": ["src"],
                "source_files": ["src/sample.cpp"],
                "functions": [
                    {
                        "address": "0x00401000",
                        "symbol": "?Sample@@YAHXZ",
                        "name": "Sample",
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    return path


def write_inline_manifest(directory: Path, name: str = "sample_inline") -> Path:
    path = directory / f"{name}.json"
    path.write_text(
        json.dumps(
            {
                "name": name,
                "description": "sample inline target",
                "source_filename": "sample_inline.cpp",
                "compiler_flags": ["/nologo", "/TP", "/O2", "/FAcs"],
                "include_dirs": [],
                "source_files": ["src/sample.cpp"],
                "functions": [
                    {
                        "address": "0x00401000",
                        "symbol": "?Sample@@YAHXZ",
                        "name": "Sample",
                    }
                ],
                "source": ["int __cdecl Sample() { return 1; }"],
            }
        ),
        encoding="utf-8",
    )
    return path


class RecoilVc6VerifyTests(unittest.TestCase):
    def test_load_manifest_normalizes_function_addresses(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = load_manifest(write_manifest(Path(tmp)))

        self.assertEqual("sample", manifest.name)
        self.assertEqual("sample_verify.cpp", manifest.source_filename)
        self.assertTrue(manifest.source_from.endswith("sample.cpp"))
        self.assertEqual("coff_bytes", manifest.compare_mode)
        self.assertTrue(manifest.trim_trailing_nops)
        self.assertEqual("", manifest.compiler_env)
        self.assertEqual(("src/sample.cpp",), manifest.source_files)
        self.assertEqual((), manifest.generated_files)
        self.assertEqual("0x401000", manifest.functions[0].address)
        self.assertEqual("", manifest.source_text)

    def test_load_manifest_accepts_source_from_and_generated_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["generated_files"] = {
                "sample.h": ["#pragma once", "int sample;"],
            }
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertTrue(manifest.source_from.endswith("sample.cpp"))
        self.assertEqual("", manifest.source_text)
        self.assertEqual((("sample.h", "#pragma once\nint sample;\n"),), manifest.generated_files)

    def test_inline_source_is_rejected_without_baseline_or_explicit_provider_escape(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_inline_manifest(Path(tmp))

            with self.assertRaisesRegex(ValueError, "inline source bodies are not allowed"):
                load_manifest(path)

    def test_inline_source_can_load_when_recorded_as_legacy_debt(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_inline_manifest(Path(tmp))
            baseline = SourcePolicyBaseline(
                inline_manifests=frozenset({str(path.resolve()).replace("\\", "/")}),
                generated_project_files=frozenset(),
            )

            manifest = load_manifest(path, source_policy_baseline=baseline)

        self.assertIn("Sample", manifest.source_text)

    def test_source_from_requires_provenance_comment(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text("int __cdecl Sample() { return 1; }\n", encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "does not contain provenance comment"):
                load_manifest(path)

    def test_source_from_accepts_provenance_in_included_inl(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text('#include "sample.inl"\n', encoding="utf-8")
            (source_path.parent / "sample.inl").write_text(
                "// Reimplements 0x401000: Sample\nint __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )

            manifest = load_manifest(path)

        self.assertTrue(manifest.source_from.endswith("sample.cpp"))

    def test_generated_project_header_shadow_is_rejected_without_baseline(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["generated_files"] = {
                "GameZRecoil/RecoilApp/RecoilStateBase.h": ["#pragma once"],
            }
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "generated file shadows project header"):
                load_manifest(path)

    def test_find_target_by_name_selects_all_functions(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifests = load_manifests(Path(tmp))
            self.assertEqual([], manifests)
            manifest = load_manifest(write_manifest(Path(tmp)))

        target, functions, selector = find_target([manifest], "sample")

        self.assertIs(target, manifest)
        self.assertEqual(manifest.functions, functions)
        self.assertEqual("sample", selector)

    def test_find_target_by_address_selects_one_function(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = load_manifest(write_manifest(Path(tmp)))

        target, functions, selector = find_target([manifest], "0x401000")

        self.assertIs(target, manifest)
        self.assertEqual(1, len(functions))
        self.assertEqual("0x401000", functions[0].address)
        self.assertEqual("0x401000", selector)

    def test_build_compile_command_records_flags_and_include_paths(self):
        target = VerifyTarget(
            name="sample",
            description="sample target",
            source_filename="sample_verify.cpp",
            source_text="int main() { return 0; }\n",
            source_from="",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=("/nologo", "/TP", "/O2", "/FAcs"),
            include_dirs=("src",),
            source_files=("src/sample.cpp",),
            generated_files=(),
            functions=(),
            manifest_path=Path("sample.json"),
        )

        command = build_compile_command(
            target,
            Path("C:/work/RecoilRebuild/build/vc6-verify/sample/sample_verify.cpp"),
            Path("C:/toolchains/VC6/vc6-env.cmd"),
            Path("C:/work/RecoilRebuild/build/vc6-verify/sample"),
        )

        self.assertIn('call "C:\\toolchains\\VC6\\vc6-env.cmd"', command)
        self.assertIn("/nologo /TP /O2 /FAcs", command)
        self.assertIn("/I ", command)
        self.assertIn("\\src", command)
        self.assertTrue(command.endswith('/c "sample_verify.cpp"'))

    def test_duplicate_addresses_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"].append(dict(data["functions"][0]))
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaises(ValueError):
                load_manifest(path)

    def test_load_manifest_accepts_text_compare_mode_and_no_nop_trim(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["compare_mode"] = "text"
            data["trim_trailing_nops"] = False
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual("text", manifest.compare_mode)
        self.assertFalse(manifest.trim_trailing_nops)

    def test_load_manifest_accepts_function_bn_byte_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0]["bn_byte_length"] = 1088
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual(1088, manifest.functions[0].bn_byte_length)

    def test_load_manifest_rejects_invalid_bn_byte_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0]["bn_byte_length"] = 0
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "bn_byte_length"):
                load_manifest(path)

    def test_coverage_helpers_report_address_matches_and_skeleton(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = load_manifest(write_manifest(Path(tmp)))

        matches = covering_targets([manifest], "0x401000")
        skeleton = manifest_skeleton("0x401234")

        self.assertEqual(1, len(matches))
        self.assertIs(matches[0][0], manifest)
        self.assertEqual("0x401234", skeleton["functions"][0]["address"])
        self.assertEqual("verify_401234", skeleton["name"])
        self.assertEqual("vc5_o2_ob0_facs", skeleton["compiler_profile"])

    def test_load_manifest_accepts_compiler_profile(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data.pop("compiler_flags")
            data["compiler_profile"] = "vc5_o2_ob0_facs"
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual("vc5_o2_ob0_facs", manifest.compiler_profile)
        self.assertIn("VC5SP3", manifest.compiler_env)
        self.assertIn("/O2", manifest.compiler_flags)

    def test_compiler_profile_rejects_raw_env_or_flags(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["compiler_profile"] = "vc5_o2_ob0_facs"
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "mutually exclusive"):
                load_manifest(path)

    def test_group_selections_by_compile_key_reuses_identical_source_compiles(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            first = load_manifest(write_manifest(directory, "first"))
            second = load_manifest(write_manifest(directory, "second"))

        groups = group_selections_by_compile_key(
            [
                VerifySelection(target=first, functions=first.functions),
                VerifySelection(target=second, functions=second.functions),
            ],
            Path("C:/toolchains/VC6/vc6-env.cmd"),
        )

        self.assertEqual(1, len(groups))
        self.assertEqual(2, len(groups[0][1]))

    def test_selected_targets_for_source_from_matches_resolved_paths(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            manifest = load_manifest(write_manifest(directory, "sample"))
            source_path = Path(manifest.source_from)

            matches = selected_targets_for_source_from([manifest], str(source_path.resolve()))

        self.assertEqual([manifest.name], [selection.target.name for selection in matches])


if __name__ == "__main__":
    unittest.main()
