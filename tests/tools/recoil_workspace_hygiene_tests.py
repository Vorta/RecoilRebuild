from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.workspace_hygiene import (  # noqa: E402
    AUTHORED_SCAN_ROOTS,
    find_offenders,
    is_generated_artifact,
)


class WorkspaceHygieneTests(unittest.TestCase):
    def make_root(self) -> tuple[tempfile.TemporaryDirectory[str], Path]:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        for name in AUTHORED_SCAN_ROOTS:
            (root / name).mkdir(parents=True)
        return temporary, root

    def test_generated_artifact_classifier(self) -> None:
        self.assertTrue(is_generated_artifact(Path("unit.obj")))
        self.assertTrue(is_generated_artifact(Path("UpgradeLog.htm")))
        self.assertTrue(is_generated_artifact(Path("module.pyc")))
        self.assertFalse(is_generated_artifact(Path("unit.cpp")))

    def test_authored_root_artifact_is_reported(self) -> None:
        _, root = self.make_root()
        offender = root / "src" / "unit.obj"
        offender.write_bytes(b"obj")
        self.assertEqual([offender], find_offenders(root))

    def test_machine_local_and_output_roots_are_not_traversed(self) -> None:
        _, root = self.make_root()
        for name in (".devspace", ".agent", "build", "support"):
            directory = root / name
            directory.mkdir()
            (directory / "unit.obj").write_bytes(b"obj")
        self.assertEqual([], find_offenders(root))

    def test_ignored_python_bytecode_cache_is_not_traversed(self) -> None:
        _, root = self.make_root()
        cache = root / "tools" / "__pycache__"
        cache.mkdir()
        (cache / "module.pyc").write_bytes(b"bytecode")
        (cache / "nested.obj").write_bytes(b"fixture")
        self.assertEqual([], find_offenders(root))

    def test_nested_authored_artifacts_are_found_once(self) -> None:
        _, root = self.make_root()
        nested = root / "tools" / "nested"
        nested.mkdir()
        first = nested / "a.pdb"
        second = nested / "b.res"
        first.write_bytes(b"")
        second.write_bytes(b"")
        self.assertEqual([first, second], find_offenders(root))


if __name__ == "__main__":
    unittest.main()
