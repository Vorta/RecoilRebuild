from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_provenance_audit import audit_final_build, audit_manifests, load_profiles  # noqa: E402


def write_profiles(root: Path) -> Path:
    path = root / "profiles.json"
    path.write_text(
        json.dumps(
            {
                "schema": 1,
                "final_build": {
                    "name": "final",
                    "description": "final",
                    "compiler_env": "${RECOIL_VC6_ROOT}/vc6-env.cmd",
                    "compiler_version_prefix": "Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 12.00.8168",
                    "compile_flags": ["/nologo", "/TP"],
                    "resource_flags": ["/r"],
                    "link_flags": ["/nologo", "/MACHINE:IX86"],
                },
                "verification_profiles": [
                    {
                        "name": "default",
                        "description": "default",
                        "compiler_env": "${RECOIL_VC6_ROOT}/vc6-env.cmd",
                        "compiler_version_prefix": "Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 12.00.8168",
                        "compiler_flags": ["/nologo", "/TP", "/O2"],
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    return path


class RecoilProvenanceAuditTests(unittest.TestCase):
    def test_matching_final_build_and_manifest_pass(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            final_profile, profiles = load_profiles(write_profiles(root))
            final_build = root / "vc6_final_build.json"
            final_build.write_text(
                json.dumps(
                    {
                        "vc6_env": "${RECOIL_VC6_ROOT}/vc6-env.cmd",
                        "compile_flags": ["/nologo", "/TP"],
                        "resource_flags": ["/r"],
                        "link_flags": ["/nologo", "/MACHINE:IX86"],
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


if __name__ == "__main__":
    unittest.main()
