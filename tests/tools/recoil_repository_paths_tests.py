import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.lib import repository_paths as repository_paths_module  # noqa: E402
from _recoil.lib.repository_paths import (  # noqa: E402
    GitTrackedPathInventory,
    RepositoryPathError,
    diagnose_historical_repository_path,
    load_git_tracked_path_inventory,
    normalize_generated_repository_path,
    resolve_tracked_repository_file,
)


class RepositoryPathTests(unittest.TestCase):
    def setUp(self):
        self.inventory = load_git_tracked_path_inventory(REPO_ROOT)

    def test_exact_path_is_accepted_and_wrong_case_is_diagnostic_only(self):
        exact = resolve_tracked_repository_file(
            "src/Battlesport/Briefing.h",
            repository_root=REPO_ROOT,
            inventory=self.inventory,
            context="fixture",
            allowed_suffixes={".h"},
        )
        self.assertEqual("src/Battlesport/Briefing.h", exact.git_path)
        self.assertEqual(REPO_ROOT.resolve(), exact.repository_root)
        self.assertTrue(exact.physical_path.is_file())

        with self.assertRaises(RepositoryPathError) as raised:
            resolve_tracked_repository_file(
                "src/Battlesport/briefing.h",
                repository_root=REPO_ROOT,
                inventory=self.inventory,
                context="fixture",
            )
        self.assertEqual("wrong-case", raised.exception.kind)
        self.assertEqual(
            "src/Battlesport/Briefing.h",
            raised.exception.expected_git_path,
        )

    @unittest.skipUnless(sys.platform == "win32", "Windows case projection control")
    def test_git_spelling_does_not_depend_on_ntfs_display_case(self):
        claimed = "src/Battlesport/BRIEFING.h"
        inventory = GitTrackedPathInventory(
            repository_root=REPO_ROOT.resolve(),
            exact_paths=frozenset({claimed}),
            casefolded_paths={claimed.casefold(): (claimed,)},
        )
        result = resolve_tracked_repository_file(
            claimed,
            repository_root=REPO_ROOT,
            inventory=inventory,
            context="fixture",
        )
        self.assertEqual(claimed, result.git_path)
        self.assertEqual("briefing.h", result.physical_path.name.casefold())

    def test_lexical_missing_suffix_and_synthetic_collision_fail_closed(self):
        cases = (
            ("", "empty-path"),
            ("C:/outside.h", "absolute-path"),
            ("//server/share.h", "absolute-path"),
            ("src\\unit.h", "backslash-path"),
            ("src/./unit.h", "nonnormalized-path"),
            ("src/../unit.h", "nonnormalized-path"),
            ("src//unit.h", "nonnormalized-path"),
            ("src/missing.h", "untracked-path"),
        )
        empty = GitTrackedPathInventory(REPO_ROOT.resolve(), frozenset(), {})
        for path_text, kind in cases:
            with self.subTest(path_text=path_text), self.assertRaises(
                RepositoryPathError
            ) as raised:
                resolve_tracked_repository_file(
                    path_text,
                    repository_root=REPO_ROOT,
                    inventory=empty,
                    context="fixture",
                )
            self.assertEqual(kind, raised.exception.kind)

        with self.assertRaises(RepositoryPathError) as raised:
            resolve_tracked_repository_file(
                "tools/README.md",
                repository_root=REPO_ROOT,
                inventory=self.inventory,
                context="fixture",
                allowed_suffixes={".h"},
            )
        self.assertEqual("disallowed-suffix", raised.exception.kind)

        collision = GitTrackedPathInventory(
            REPO_ROOT.resolve(),
            frozenset({"src/Unit.h", "src/unit.H"}),
            {"src/unit.h": ("src/Unit.h", "src/unit.H")},
        )
        for supplied in ("src/UNIT.H", "src/Unit.h"):
            with self.subTest(supplied=supplied), self.assertRaises(
                RepositoryPathError
            ) as raised:
                resolve_tracked_repository_file(
                    supplied,
                    repository_root=REPO_ROOT,
                    inventory=collision,
                    context="fixture",
                )
            self.assertEqual("casefold-collision", raised.exception.kind)

    def _mock_inventory_load(self, inventory_output: bytes, *, allow_empty=False):
        root_output = (str(REPO_ROOT.resolve()) + "\n").encode("utf-8")
        with patch.object(
            repository_paths_module,
            "_git_path_command",
            side_effect=(
                subprocess.CompletedProcess(["git"], 0, root_output, b""),
                subprocess.CompletedProcess(["git"], 0, inventory_output, b""),
            ),
        ):
            return load_git_tracked_path_inventory(
                REPO_ROOT,
                allow_empty=allow_empty,
            )

    def test_inventory_git_and_strict_parse_failures_are_typed(self):
        with patch.object(
            repository_paths_module,
            "_git_path_command",
            return_value=subprocess.CompletedProcess(["git"], 1, b"", b"failure"),
        ), self.assertRaises(RepositoryPathError) as raised:
            load_git_tracked_path_inventory(REPO_ROOT)
        self.assertEqual("git-command-failed", raised.exception.kind)

        failures = (
            (b"src/unit.h", "truncated-inventory"),
            (b"src/\xff.h\0", "invalid-inventory-encoding"),
            (b"src/a.h\0\0", "empty-inventory-row"),
            (b"src/a.h\0src/a.h\0", "duplicate-inventory-path"),
            (b"src/A.h\0src/a.H\0", "casefold-collision"),
        )
        for output, kind in failures:
            with self.subTest(kind=kind), self.assertRaises(
                RepositoryPathError
            ) as raised:
                self._mock_inventory_load(output)
            self.assertEqual(kind, raised.exception.kind)

        with self.assertRaises(RepositoryPathError) as raised:
            self._mock_inventory_load(b"")
        self.assertEqual("empty-inventory", raised.exception.kind)
        empty = self._mock_inventory_load(b"", allow_empty=True)
        self.assertEqual(frozenset(), empty.exact_paths)

    def test_inventory_root_output_requires_one_exact_line_terminator(self):
        valid_inventory = b"README.md\0"
        malformed_roots = (
            str(REPO_ROOT.resolve()).encode("utf-8"),
            (str(REPO_ROOT.resolve()) + "\n\n").encode("utf-8"),
            (str(REPO_ROOT.resolve()) + "\0\n").encode("utf-8"),
        )
        for root_output in malformed_roots:
            with self.subTest(root_output=root_output), patch.object(
                repository_paths_module,
                "_git_path_command",
                side_effect=(
                    subprocess.CompletedProcess(["git"], 0, root_output, b""),
                    subprocess.CompletedProcess(["git"], 0, valid_inventory, b""),
                ),
            ), self.assertRaises(RepositoryPathError) as raised:
                load_git_tracked_path_inventory(REPO_ROOT)
            self.assertEqual("malformed-root-output", raised.exception.kind)

    def test_repository_root_reparse_and_inspection_failures_are_closed(self):
        with patch.object(
            repository_paths_module,
            "_absolute_path_reparse_component",
            return_value=REPO_ROOT,
        ), self.assertRaises(RepositoryPathError) as raised:
            load_git_tracked_path_inventory(REPO_ROOT)
        self.assertEqual("reparse-component", raised.exception.kind)

        inspection_error = RepositoryPathError(
            "path-inspection-failed",
            "inspection failed",
            context="tracked repository root",
        )
        with patch.object(
            repository_paths_module,
            "_absolute_path_reparse_component",
            side_effect=inspection_error,
        ), self.assertRaises(RepositoryPathError) as raised:
            load_git_tracked_path_inventory(REPO_ROOT)
        self.assertEqual("path-inspection-failed", raised.exception.kind)

    def test_exact_root_and_inventory_worktree_binding_are_required(self):
        with self.assertRaises(RepositoryPathError) as raised:
            load_git_tracked_path_inventory(REPO_ROOT / "tools")
        self.assertEqual("repository-root-not-worktree-root", raised.exception.kind)

        with self.assertRaises(RepositoryPathError) as raised:
            resolve_tracked_repository_file(
                "README.md",
                repository_root=REPO_ROOT / "tools",
                inventory=self.inventory,
                context="fixture",
            )
        self.assertEqual("inventory-worktree-mismatch", raised.exception.kind)

    def test_physical_reparse_inspection_and_file_type_fail_closed(self):
        candidate = REPO_ROOT / "src" / "Battlesport" / "Briefing.h"
        with patch.object(
            repository_paths_module,
            "_path_has_reparse_component",
            return_value=candidate,
        ), self.assertRaises(RepositoryPathError) as raised:
            resolve_tracked_repository_file(
                "src/Battlesport/Briefing.h",
                repository_root=REPO_ROOT,
                inventory=self.inventory,
                context="fixture",
            )
        self.assertEqual("reparse-component", raised.exception.kind)

        inspection_error = RepositoryPathError(
            "path-inspection-failed",
            "inspection failed",
            context="fixture",
        )
        with patch.object(
            repository_paths_module,
            "_path_has_reparse_component",
            side_effect=inspection_error,
        ), self.assertRaises(RepositoryPathError) as raised:
            resolve_tracked_repository_file(
                "src/Battlesport/Briefing.h",
                repository_root=REPO_ROOT,
                inventory=self.inventory,
                context="fixture",
            )
        self.assertEqual("path-inspection-failed", raised.exception.kind)

        directory_inventory = GitTrackedPathInventory(
            REPO_ROOT.resolve(),
            frozenset({"tools"}),
            {"tools": ("tools",)},
        )
        with self.assertRaises(RepositoryPathError) as raised:
            resolve_tracked_repository_file(
                "tools",
                repository_root=REPO_ROOT,
                inventory=directory_inventory,
                context="fixture",
            )
        self.assertEqual("not-ordinary-file", raised.exception.kind)

    def test_generated_paths_are_strict_and_not_git_membership_checked(self):
        generated = normalize_generated_repository_path(
            "build/live-validation/run/result.json",
            allowed_roots={"build/live-validation"},
            context="fixture output",
        )
        self.assertEqual("build/live-validation/run/result.json", generated.logical_path)
        self.assertEqual("build/live-validation", generated.allowed_root)
        for bad in (
            "Build/live-validation/result.json",
            "build\\live-validation\\result.json",
            "../build/live-validation/result.json",
            "D:/build/live-validation/result.json",
        ):
            with self.subTest(bad=bad), self.assertRaises(RepositoryPathError):
                normalize_generated_repository_path(
                    bad,
                    allowed_roots={"build/live-validation"},
                    context="fixture output",
                )

    def test_historical_paths_are_diagnostic_and_never_current(self):
        exact = diagnose_historical_repository_path(
            "src/GameZRecoil/zSys/zSys.cpp",
            inventory=self.inventory,
        )
        alias = diagnose_historical_repository_path(
            "src/GameZRecoil/zSys/zsys.cpp",
            inventory=self.inventory,
        )
        missing = diagnose_historical_repository_path(
            "src/GameZRecoil/zSys/missing.cpp",
            inventory=self.inventory,
        )
        self.assertEqual("exact-historical", exact.status)
        self.assertEqual("historical-case-alias", alias.status)
        self.assertEqual("src/GameZRecoil/zSys/zSys.cpp", alias.current_git_path)
        self.assertEqual("missing", missing.status)
        self.assertFalse(exact.current)
        self.assertFalse(alias.current)
        self.assertFalse(alias.tracker_mutated)

        collision = GitTrackedPathInventory(
            REPO_ROOT.resolve(),
            frozenset({"src/Unit.h", "src/unit.H"}),
            {"src/unit.h": ("src/Unit.h", "src/unit.H")},
        )
        ambiguous = diagnose_historical_repository_path(
            "src/UNIT.h",
            inventory=collision,
        )
        self.assertEqual("ambiguous", ambiguous.status)
        self.assertFalse(ambiguous.current)

        with self.assertRaises(RepositoryPathError) as raised:
            diagnose_historical_repository_path(
                "src/GameZRecoil/zSys/zsys.cpp",
                inventory=self.inventory,
                current_allowed_paths={"src/not-tracked.cpp"},
            )
        self.assertEqual("untracked-current-allowed-path", raised.exception.kind)

        restricted = diagnose_historical_repository_path(
            "src/GameZRecoil/zSys/zsys.cpp",
            inventory=self.inventory,
            current_allowed_paths={"src/GameZRecoil/zSys/zSys.cpp"},
        )
        self.assertEqual("historical-case-alias", restricted.status)
        self.assertEqual(
            "src/GameZRecoil/zSys/zSys.cpp",
            restricted.current_git_path,
        )

    def test_inventory_map_is_immutable(self):
        with self.assertRaises(TypeError):
            self.inventory.casefolded_paths["new"] = ("new",)  # type: ignore[index]

    def test_vc5_delegates_instead_of_reimplementing_case_authority(self):
        from _recoil.commands import vc5_verify

        self.assertIs(
            vc5_verify.resolve_tracked_repository_file,
            repository_paths_module.resolve_tracked_repository_file,
        )
        self.assertIs(
            vc5_verify.load_git_tracked_path_inventory,
            repository_paths_module.load_git_tracked_path_inventory,
        )
        source = (REPO_ROOT / "tools/_recoil/commands/vc5_verify.py").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("class GitTrackedPathInventory", source)
        self.assertNotIn("def resolve_tracked_repository_file", source)
        self.assertNotIn("def _git_path_command", source)

    def test_linked_worktrees_use_distinct_inventories(self):
        with tempfile.TemporaryDirectory(prefix="repository_paths_linked_") as temporary:
            parent = Path(temporary)
            repository = parent / "repository"
            linked = parent / "linked"
            repository.mkdir()
            for arguments in (
                ("init", "--quiet"),
                ("config", "user.email", "recoil-tests@example.invalid"),
                ("config", "user.name", "Recoil Tests"),
            ):
                subprocess.run(
                    ["git", *arguments],
                    cwd=repository,
                    check=True,
                    capture_output=True,
                )
            tracked = repository / "Src" / "Unit.h"
            tracked.parent.mkdir()
            tracked.write_text("// fixture\n", encoding="utf-8")
            subprocess.run(
                ["git", "add", "Src/Unit.h"],
                cwd=repository,
                check=True,
                capture_output=True,
            )
            subprocess.run(
                ["git", "commit", "--quiet", "-m", "fixture"],
                cwd=repository,
                check=True,
                capture_output=True,
            )
            subprocess.run(
                ["git", "worktree", "add", "--quiet", "--detach", str(linked), "HEAD"],
                cwd=repository,
                check=True,
                capture_output=True,
            )
            try:
                canonical_inventory = load_git_tracked_path_inventory(repository)
                linked_inventory = load_git_tracked_path_inventory(linked)
                linked_result = resolve_tracked_repository_file(
                    "Src/Unit.h",
                    repository_root=linked,
                    inventory=linked_inventory,
                    context="linked fixture",
                )
                self.assertEqual(linked.resolve(), linked_result.repository_root)
                with self.assertRaises(RepositoryPathError) as raised:
                    resolve_tracked_repository_file(
                        "Src/Unit.h",
                        repository_root=linked,
                        inventory=canonical_inventory,
                        context="linked fixture",
                    )
                self.assertEqual("inventory-worktree-mismatch", raised.exception.kind)
            finally:
                subprocess.run(
                    ["git", "worktree", "remove", str(linked)],
                    cwd=repository,
                    check=True,
                    capture_output=True,
                )


if __name__ == "__main__":
    unittest.main()
