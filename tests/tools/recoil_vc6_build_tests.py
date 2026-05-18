from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))
CLI = REPO_ROOT / "tools" / "recoil_vc6_build.py"

from recoil_vc6_build import (  # noqa: E402
    build_paths,
    compile_response_args,
    link_response_args,
    load_config,
    make_compile_command,
    make_link_command,
    make_resource_command,
    object_path,
)


def write_manifest(directory: Path) -> Path:
    vc6_env = directory / "vc6-env.cmd"
    vc6_env.write_text("@echo off\n", encoding="ascii")
    source = directory / "src" / "sample.cpp"
    source.parent.mkdir(parents=True)
    source.write_text("int sample;\n", encoding="ascii")
    resource = directory / "src" / "sample.rc"
    resource.write_text("1 VERSIONINFO\nBEGIN\nEND\n", encoding="ascii")
    reference = directory / "support" / "Recoil.exe"
    reference.parent.mkdir(parents=True)
    reference.write_bytes(b"MZ")
    manifest_json = directory / ".agent" / "REFERENCE_EXECUTABLE.json"
    manifest_json.parent.mkdir(parents=True)
    manifest_json.write_text("{}\n", encoding="ascii")

    path = directory / "vc6_final_build.json"
    path.write_text(
        json.dumps(
            {
                "name": "sample",
                "description": "sample",
                "vc6_env": str(vc6_env),
                "build_dir": str(directory / "build" / "vc6-final"),
                "output_exe": "Recoil.exe",
                "output_map": "Recoil.map",
                "resource_script": str(resource),
                "resource_output": "Recoil.res",
                "reference_exe": str(reference),
                "reference_manifest": str(manifest_json),
                "defines": ["WIN32", "_AFXDLL"],
                "include_dirs": [str(source.parent)],
                "lib_dirs": [str(directory / "lib")],
                "compile_flags": ["/nologo", "/TP", "/O2", "/MD"],
                "resource_flags": ["/nologo", "/r"],
                "link_flags": ["/nologo", "/MACHINE:IX86", "/SUBSYSTEM:WINDOWS"],
                "libs": ["MSVCRT.LIB", "KERNEL32.LIB"],
                "sources": [str(source)],
            }
        ),
        encoding="utf-8",
    )
    (directory / "lib").mkdir()
    return path


class RecoilVc6BuildTests(unittest.TestCase):
    def test_load_config_and_object_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            config = load_config(write_manifest(Path(tmp)))
            paths = build_paths(config)
            obj = object_path(config, paths, config.sources[0])

        self.assertEqual("sample", config.name)
        self.assertTrue(str(obj).endswith("sample.obj"))

    def test_compile_response_contains_flags_defines_includes_and_object(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            config = load_config(write_manifest(Path(tmp)))
            paths = build_paths(config)
            obj = object_path(config, paths, config.sources[0])
            args = compile_response_args(config, config.sources[0], obj)

        rendered = "\n".join(args)
        self.assertIn("/TP", rendered)
        self.assertIn("/DWIN32", rendered)
        self.assertIn("/D_AFXDLL", rendered)
        self.assertIn("/I", rendered)
        self.assertIn("/Fo", rendered)
        self.assertIn("/c", rendered)

    def test_resource_and_link_commands_use_response_files(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            config = load_config(write_manifest(Path(tmp)))
            paths = build_paths(config)
            paths.rsp_dir.mkdir(parents=True)
            paths.logs_dir.mkdir(parents=True)
            compile_command, obj = make_compile_command(config, paths, config.sources[0])
            resource_command = make_resource_command(config, paths)
            link_command = make_link_command(config, paths, [obj])

        self.assertIn("cl @", compile_command.command)
        self.assertIn(" rc ", resource_command.command)
        self.assertIn("sample.rc", resource_command.command)
        self.assertIn("link @", link_command.command)

    def test_link_response_contains_outputs_objects_resource_and_libs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            config = load_config(write_manifest(Path(tmp)))
            paths = build_paths(config)
            obj = object_path(config, paths, config.sources[0])
            args = link_response_args(config, paths, [obj])

        rendered = "\n".join(args)
        self.assertIn("/OUT:", rendered)
        self.assertIn("/MAP:", rendered)
        self.assertIn("Recoil.res", rendered)
        self.assertIn("MSVCRT.LIB", rendered)
        self.assertIn("KERNEL32.LIB", rendered)

    def test_cli_dry_run_on_real_manifest(self) -> None:
        result = subprocess.run(
            [sys.executable, str(CLI), "--dry-run"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("VC6 final build manifest", result.stdout)
        self.assertIn("compile:src/Battlesport/RecoilApp.cpp", result.stdout)
        self.assertIn("resource:", result.stdout)
        self.assertIn("link:", result.stdout)


if __name__ == "__main__":
    unittest.main()
