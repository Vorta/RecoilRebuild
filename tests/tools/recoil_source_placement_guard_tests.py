from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools._recoil.commands import source_placement_guard as guard


class SourcePlacementGuardTests(unittest.TestCase):
    def make_repo(self) -> tuple[tempfile.TemporaryDirectory[str], Path, Path]:
        temp = tempfile.TemporaryDirectory()
        repo_root = Path(temp.name)
        src = repo_root / "src"
        src.mkdir()
        return temp, repo_root, src

    def test_rejects_ambiguous_provenance_labels_in_production_source(self) -> None:
        temp, repo_root, src = self.make_repo()
        self.addCleanup(temp.cleanup)
        source = src / "sample.cpp"

        for label in (
            "Original source path:",
            "Original source file:",
            "Original file:",
        ):
            with self.subTest(label=label):
                source.write_text(f"/** {label} D:\\Proj\\sample.cpp. */\n", encoding="utf-8")
                violations = guard.find_violations(src, repo_root)
                self.assertEqual(
                    ["ambiguous source-provenance label"],
                    [violation[2] for violation in violations],
                )

    def test_accepts_exact_physical_and_provisional_source_labels(self) -> None:
        temp, repo_root, src = self.make_repo()
        self.addCleanup(temp.cleanup)
        (src / "sample.cpp").write_text(
            "/**\n"
            " * Retail literal-backed physical source block: D:\\Proj\\sample.cpp.\n"
            " * Provisional source-placement hypothesis: D:\\Proj\\sample.cpp.\n"
            " */\n",
            encoding="utf-8",
        )

        self.assertEqual([], guard.find_violations(src, repo_root))

    def test_rejects_each_retired_path_with_slash_or_backslash(self) -> None:
        temp, repo_root, src = self.make_repo()
        self.addCleanup(temp.cleanup)
        source = src / "sample.cpp"

        for retired_path in guard.RETIRED_LAYOUT_PATHS:
            for rendered in (retired_path, retired_path.replace("/", "\\")):
                with self.subTest(path=rendered):
                    source.write_text(f'#include "{rendered}"\n', encoding="utf-8")
                    violations = guard.find_violations(src, repo_root)
                    self.assertEqual(
                        ["retired source-layout path"],
                        [violation[2] for violation in violations],
                    )

    def test_scans_all_active_configuration_surfaces(self) -> None:
        temp, repo_root, src = self.make_repo()
        self.addCleanup(temp.cleanup)
        config_paths = [
            repo_root / "CMakeLists.txt",
            repo_root / "CMakePresets.json",
            repo_root / "cmake" / "layout.cmake",
            repo_root / "tools" / "_recoil" / "config" / "layout.json",
            repo_root / "tools" / "functional_verify_targets" / "layout.json",
            repo_root / "tools" / "vc5_verify_targets" / "layout.json",
            repo_root / "tests" / "native" / "CMakeLists.txt",
            src / "GameZRecoil" / "CMakeLists.txt",
        ]
        for path in config_paths:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("src/GameZRecoil/mission.h\n", encoding="utf-8")

        violations = guard.find_violations(src, repo_root)

        self.assertEqual(len(config_paths), len(violations))
        self.assertEqual(
            {guard.display_path(path, repo_root) for path in config_paths},
            {violation[0] for violation in violations},
        )
        self.assertEqual(
            {"retired source-layout path"},
            {violation[2] for violation in violations},
        )

    def test_excludes_historical_surfaces_and_tests_native_source(self) -> None:
        temp, repo_root, src = self.make_repo()
        self.addCleanup(temp.cleanup)
        excluded_paths = [
            repo_root / ".agent" / "evidence.json",
            repo_root / ".devspace" / "probe.cmake",
            repo_root / "docs" / "history.md",
            repo_root / "export" / "retail.txt",
            repo_root / "tests" / "native" / "fixture.cpp",
        ]
        for path in excluded_paths:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(
                "Original source path: src/GameZRecoil/mission.h\n",
                encoding="utf-8",
            )

        self.assertEqual([], guard.find_violations(src, repo_root))

    def test_preserves_existing_misplacement_and_stale_include_checks(self) -> None:
        temp, repo_root, src = self.make_repo()
        self.addCleanup(temp.cleanup)
        misplaced = src / "Battlesport" / "zUtil" / "bad.cpp"
        misplaced.parent.mkdir(parents=True)
        misplaced.write_text("void bad() {}\n", encoding="utf-8")
        include = src / "GameZRecoil" / "bad.cpp"
        include.parent.mkdir(parents=True)
        include.write_text(
            '#include "Battlesport/zUtil/zutil.h"\n',
            encoding="utf-8",
        )

        violations = guard.find_violations(src, repo_root)

        self.assertEqual(
            ["misplaced Recoil engine source", "stale zUtil include path"],
            [violation[2] for violation in violations],
        )


if __name__ == "__main__":
    unittest.main()
