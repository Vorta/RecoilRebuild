from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.provenance_audit import audit_final_build, audit_manifests, load_profiles  # noqa: E402

VC5_ENV = "D:/Recoil Project/Compiler/VC5SP3/vc5sp3-env.cmd"


def write_profiles(root: Path) -> Path:
    path = root / "profiles.json"
    path.write_text(
        json.dumps(
            {
                "schema": 1,
                "final_build": {
                    "name": "final",
                    "description": "final",
                    "compiler_env": VC5_ENV,
                    "compiler_version_prefix": "Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 11.00.7022",
                    "compile_flags": ["/nologo", "/TP"],
                    "resource_flags": ["/r"],
                    "link_flags": ["/nologo", "/MACHINE:IX86"],
                },
                "verification_profiles": [
                    {
                        "name": "default",
                        "description": "default",
                        "compiler_env": VC5_ENV,
                        "compiler_version_prefix": "Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 11.00.7022",
                        "compiler_flags": ["/nologo", "/TP", "/O2"],
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    return path


class RecoilProvenanceAuditTests(unittest.TestCase):
    def test_manifest_may_inherit_documented_final_build_compile_context(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            final_profile, profiles = load_profiles(write_profiles(root))
            final_build = root / "vc5_final_build.json"
            final_build.write_text(
                json.dumps(
                    {
                        "vc5_env": VC5_ENV,
                        "compile_flags": ["/nologo", "/TP"],
                        "defines": ["WIN32"],
                        "include_dirs": ["tools/_recoil/compat/include"],
                    }
                ),
                encoding="utf-8",
            )
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            (manifest_dir / "sample.json").write_text(
                json.dumps(
                    {
                        "name": "sample",
                        "source_from": "src/sample.cpp",
                        "compile_context_from": "vc5_final_build.json",
                    }
                ),
                encoding="utf-8",
            )

            mismatches, counts = audit_manifests(
                manifest_dir,
                profiles,
                final_profile=final_profile,
                compile_context_root=root,
            )

        self.assertEqual([], mismatches)
        self.assertEqual({"default": 0, "final-build-context": 1}, counts)

    def test_linked_only_manifest_is_exempt_from_compiler_profile_audit(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _final_profile, profiles = load_profiles(write_profiles(root))
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            (manifest_dir / "linked_only.json").write_text(
                json.dumps(
                    {
                        "name": "linked_only",
                        "description": "final-link metadata only",
                        "linked_function_intervals": [
                            {
                                "name": "sample",
                                "predecessor": {"address": "0x400ff0", "symbol": "?Before@@YAHXZ"},
                                "functions": [{"address": "0x401000", "symbol": "?Body@@YAHXZ"}],
                                "successor": {"address": "0x401010", "symbol": "?After@@YAHXZ"},
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )

            mismatches, counts = audit_manifests(manifest_dir, profiles)

        self.assertEqual([], mismatches)
        self.assertEqual({"default": 0}, counts)

    def test_mixed_compile_and_linked_manifest_still_requires_compiler_profile(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _final_profile, profiles = load_profiles(write_profiles(root))
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            (manifest_dir / "mixed.json").write_text(
                json.dumps(
                    {
                        "name": "mixed",
                        "functions": [{"address": "0x401000", "symbol": "?Body@@YAHXZ"}],
                        "linked_function_intervals": [
                            {
                                "name": "sample",
                                "predecessor": {"address": "0x400ff0", "symbol": "?Before@@YAHXZ"},
                                "functions": [{"address": "0x401000", "symbol": "?Body@@YAHXZ"}],
                                "successor": {"address": "0x401010", "symbol": "?After@@YAHXZ"},
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )

            mismatches, _counts = audit_manifests(manifest_dir, profiles)

        self.assertEqual(1, len(mismatches))
        self.assertIn("compiler_flags", mismatches[0])

    def test_matching_final_build_and_manifest_pass(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            final_profile, profiles = load_profiles(write_profiles(root))
            final_build = root / "vc5_final_build.json"
            final_build.write_text(
                json.dumps(
                    {
                        "vc5_env": VC5_ENV,
                        "compile_flags": ["/nologo", "/TP"],
                        "resource_flags": ["/r"],
                        "link_flags": ["/nologo", "/MACHINE:IX86"],
                        "include_dirs": ["D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE"],
                        "lib_dirs": ["D:/Recoil Project/Compiler/VC5SP3/VC/MFC/LIB"],
                        "canonical_mfc": {
                            "include_root": "D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE",
                            "lib_root": "D:/Recoil Project/Compiler/VC5SP3/VC/MFC/LIB",
                        },
                    }
                ),
                encoding="utf-8",
            )
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            (manifest_dir / "sample.json").write_text(
                json.dumps(
                    {
                        "name": "sample",
                        "compiler_flags": ["/nologo", "/TP", "/O2"],
                    }
                ),
                encoding="utf-8",
            )

            self.assertEqual([], audit_final_build(final_profile, final_build))
            mismatches, counts = audit_manifests(manifest_dir, profiles)

        self.assertEqual([], mismatches)
        self.assertEqual({"default": 1}, counts)

    def test_final_build_rejects_old_support_mfc_provider_paths(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            final_profile, _profiles = load_profiles(write_profiles(root))
            final_build = root / "vc5_final_build.json"
            final_build.write_text(
                json.dumps(
                    {
                        "vc5_env": VC5_ENV,
                        "compile_flags": ["/nologo", "/TP"],
                        "resource_flags": ["/r"],
                        "link_flags": ["/nologo", "/MACHINE:IX86"],
                        "include_dirs": ["support/sdk/MFC42/Include"],
                        "lib_dirs": ["support/sdk/MFC42/Lib/x86"],
                        "canonical_mfc": {
                            "include_root": "support/sdk/MFC42/Include",
                            "lib_root": "support/sdk/MFC42/Lib/x86",
                        },
                    }
                ),
                encoding="utf-8",
            )

            mismatches = audit_final_build(final_profile, final_build)

        self.assertTrue(any("forbidden active MFC paths" in item for item in mismatches))
        self.assertTrue(any("omit canonical VC5SP3 MFC root" in item for item in mismatches))

    def test_unknown_manifest_profile_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _final_profile, profiles = load_profiles(write_profiles(root))
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            (manifest_dir / "sample.json").write_text(
                json.dumps(
                    {
                        "name": "sample",
                        "compiler_flags": ["/nologo", "/TP", "/Ob2"],
                    }
                ),
                encoding="utf-8",
            )

            mismatches, _counts = audit_manifests(manifest_dir, profiles)

        self.assertEqual(1, len(mismatches))
        self.assertIn("undocumented compiler profile", mismatches[0])

    def test_named_manifest_profile_is_counted(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _final_profile, profiles = load_profiles(write_profiles(root))
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            (manifest_dir / "sample.json").write_text(
                json.dumps(
                    {
                        "name": "sample",
                        "compiler_profile": "default",
                    }
                ),
                encoding="utf-8",
            )

            mismatches, counts = audit_manifests(manifest_dir, profiles)

        self.assertEqual([], mismatches)
        self.assertEqual({"default": 1}, counts)

    def test_named_manifest_profile_rejects_raw_flags(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _final_profile, profiles = load_profiles(write_profiles(root))
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            (manifest_dir / "sample.json").write_text(
                json.dumps(
                    {
                        "name": "sample",
                        "compiler_profile": "default",
                        "compiler_flags": ["/nologo", "/TP", "/O2"],
                    }
                ),
                encoding="utf-8",
            )

            mismatches, _counts = audit_manifests(manifest_dir, profiles)

        self.assertEqual(1, len(mismatches))
        self.assertIn("mutually exclusive", mismatches[0])


if __name__ == "__main__":
    unittest.main()
