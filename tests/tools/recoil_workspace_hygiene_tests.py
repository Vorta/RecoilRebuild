from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_workspace_hygiene import DEFAULT_ALLOWED_ROOTS, find_offenders  # noqa: E402


class RecoilWorkspaceHygieneTests(unittest.TestCase):
    def test_root_artifact_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "NUL.obj").write_bytes(b"obj")
            (root / "src").mkdir()
            (root / "src" / "ok.cpp").write_text("int ok;\n", encoding="utf-8")

            offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

        self.assertEqual(["NUL.obj"], [path.name for path in offenders])

    def test_build_artifacts_under_build_are_allowed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build_dir = root / "build"
            build_dir.mkdir()
            (build_dir / "sample.obj").write_bytes(b"obj")

            offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

        self.assertEqual([], offenders)

    def test_upgrade_log_is_reported_outside_allowed_roots(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "UpgradeLog.htm").write_text("generated\n", encoding="utf-8")

            offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

        self.assertEqual(["UpgradeLog.htm"], [path.name for path in offenders])


if __name__ == "__main__":
    unittest.main()
