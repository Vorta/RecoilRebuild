from __future__ import annotations

from pathlib import Path
import os
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.lib.repository_paths import (  # noqa: E402
    RepositoryPathError,
    diagnose_historical_repository_path,
    load_repository_path_inventory,
    normalize_generated_repository_path,
    resolve_repository_file,
    validate_repository_relative_path,
)


class RepositoryPathsTests(unittest.TestCase):
    def make_repository(self) -> tuple[tempfile.TemporaryDirectory[str], Path]:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        (root / "Src").mkdir()
        (root / "Src" / "Unit.cpp").write_text("int unit;\n", encoding="utf-8")
        (root / "README.md").write_text("fixture\n", encoding="utf-8")
        return temporary, root

    def inventory(self, root: Path):
        return load_repository_path_inventory(
            root,
            allowed_roots=("Src",),
            allowed_paths=("README.md",),
        )

    def test_inventory_scans_only_authorized_roots_and_paths(self) -> None:
        _, root = self.make_repository()
        (root / "build").mkdir()
        (root / "build" / "generated.cpp").write_text("", encoding="utf-8")
        inventory = self.inventory(root)
        self.assertEqual(
            frozenset({"Src/Unit.cpp", "README.md"}), inventory.exact_paths
        )
        self.assertNotIn("build/generated.cpp", inventory.exact_paths)

    def test_exact_spelling_resolves_physical_file(self) -> None:
        _, root = self.make_repository()
        inventory = self.inventory(root)
        result = resolve_repository_file(
            "Src/Unit.cpp",
            repository_root=root,
            inventory=inventory,
            context="fixture source",
            allowed_suffixes={".cpp"},
        )
        self.assertEqual("Src/Unit.cpp", result.repository_path)
        self.assertEqual((root / "Src" / "Unit.cpp").resolve(), result.physical_path)

    def test_wrong_case_and_unknown_paths_fail_closed(self) -> None:
        _, root = self.make_repository()
        inventory = self.inventory(root)
        with self.assertRaises(RepositoryPathError) as wrong_case:
            resolve_repository_file(
                "src/unit.cpp",
                repository_root=root,
                inventory=inventory,
                context="fixture source",
            )
        self.assertEqual("wrong-case", wrong_case.exception.kind)
        self.assertEqual(
            "Src/Unit.cpp", wrong_case.exception.expected_repository_path
        )
        with self.assertRaises(RepositoryPathError) as unknown:
            resolve_repository_file(
                "Src/Missing.cpp",
                repository_root=root,
                inventory=inventory,
                context="fixture source",
            )
        self.assertEqual("unknown-path", unknown.exception.kind)

    def test_machine_local_root_cannot_be_authorized(self) -> None:
        _, root = self.make_repository()
        (root / "build").mkdir()
        with self.assertRaises(RepositoryPathError) as raised:
            load_repository_path_inventory(root, allowed_roots=("build",))
        self.assertEqual("machine-local-root", raised.exception.kind)

    def test_casefold_collision_fails_inventory(self) -> None:
        if os.path.normcase("A") == os.path.normcase("a"):
            self.skipTest("filesystem cannot create case-distinct fixture paths")
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        (root / "src").mkdir()
        (root / "src" / "Unit.cpp").write_text("", encoding="utf-8")
        (root / "src" / "unit.cpp").write_text("", encoding="utf-8")
        with self.assertRaises(RepositoryPathError) as raised:
            load_repository_path_inventory(root, allowed_roots=("src",))
        self.assertEqual("casefold-collision", raised.exception.kind)

    def test_historical_case_alias_is_diagnostic_only(self) -> None:
        _, root = self.make_repository()
        inventory = self.inventory(root)
        result = diagnose_historical_repository_path(
            "src/unit.cpp", inventory=inventory
        )
        self.assertEqual("historical-case-alias", result.status)
        self.assertEqual("Src/Unit.cpp", result.current_repository_path)
        self.assertFalse(result.current)
        self.assertFalse(result.tracker_mutated)

    def test_lexical_hazards_and_generated_root_ambiguity_fail(self) -> None:
        for value in ("", "../src/a.cpp", "src\\a.cpp", "/src/a.cpp"):
            with self.subTest(value=value), self.assertRaises(RepositoryPathError):
                validate_repository_relative_path(value, context="fixture")
        with self.assertRaises(RepositoryPathError):
            normalize_generated_repository_path(
                "build/output/a.obj",
                allowed_roots=("build", "build/output"),
                context="generated output",
            )


if __name__ == "__main__":
    unittest.main()
