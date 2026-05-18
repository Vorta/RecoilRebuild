import json
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_vc6_verify import (  # noqa: E402
    VerifyTarget,
    build_compile_command,
    covering_targets,
    find_target,
    load_manifest,
    load_manifests,
    manifest_skeleton,
)


def write_manifest(directory: Path, name: str = "sample") -> Path:
    path = directory / f"{name}.json"
    path.write_text(
        json.dumps(
            {
                "name": name,
                "description": "sample target",
                "source_filename": "sample_verify.cpp",
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
                "source": ["int __cdecl Sample();", "#include \"sample.cpp\""],
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
        self.assertEqual("", manifest.source_from)
        self.assertEqual("coff_bytes", manifest.compare_mode)
        self.assertTrue(manifest.trim_trailing_nops)
        self.assertEqual(("src/sample.cpp",), manifest.source_files)
        self.assertEqual((), manifest.generated_files)
        self.assertEqual("0x401000", manifest.functions[0].address)
        self.assertIn('#include "sample.cpp"', manifest.source_text)

    def test_load_manifest_accepts_source_from_and_generated_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            del data["source"]
            data["source_from"] = "src/sample.cpp"
            data["generated_files"] = {
                "sample.h": ["#pragma once", "int sample;"],
            }
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual("src/sample.cpp", manifest.source_from)
        self.assertEqual("", manifest.source_text)
        self.assertEqual((("sample.h", "#pragma once\nint sample;\n"),), manifest.generated_files)

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

    def test_coverage_helpers_report_address_matches_and_skeleton(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = load_manifest(write_manifest(Path(tmp)))

        matches = covering_targets([manifest], "0x401000")
        skeleton = manifest_skeleton("0x401234")

        self.assertEqual(1, len(matches))
        self.assertIs(matches[0][0], manifest)
        self.assertEqual("0x401234", skeleton["functions"][0]["address"])
        self.assertEqual("verify_401234", skeleton["name"])


if __name__ == "__main__":
    unittest.main()
