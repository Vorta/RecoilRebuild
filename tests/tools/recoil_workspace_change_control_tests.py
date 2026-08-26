from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

from tools._recoil.lib import git_change_control

from tools._recoil.lib.git_change_control import (
    GIT_WORKSPACE_BASELINE_SCHEMA,
    GitChangeControlError,
    capture_clean_git_baseline,
    capture_git_closeout,
    parse_name_status_z,
    parse_porcelain_v2_z,
    parse_unmerged_index_z,
    reauthenticate_clean_git_baseline,
    validate_git_baseline_descriptor,
)


def run_git(root: Path, *arguments: str) -> str:
    completed = subprocess.run(
        ["git", *arguments], cwd=root, check=True, capture_output=True,
        text=True, encoding="utf-8",
    )
    return completed.stdout


class NativeGitChangeControlTests(unittest.TestCase):
    def repository(self, files: dict[str, str] | None = None):
        temporary = tempfile.TemporaryDirectory()
        root = Path(temporary.name)
        run_git(root, "init", "-q")
        run_git(root, "config", "user.email", "recoil-tests@example.invalid")
        run_git(root, "config", "user.name", "Recoil Tests")
        for relative, content in (files or {"source.txt": "retail\n"}).items():
            path = root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8")
        run_git(root, "add", ".")
        run_git(root, "commit", "-q", "-m", "reviewed baseline")
        return temporary, root

    def baseline(self, root: Path, *writable: str) -> dict[str, object]:
        return capture_clean_git_baseline(
            root, packet_id="issue:work:test:native-git", writable_paths=writable
        )

    def closeout(
        self, root: Path, baseline: dict[str, object], *writable: str
    ) -> dict[str, object]:
        return capture_git_closeout(
            root, baseline, packet_id="issue:work:test:native-git",
            writable_paths=writable,
        )

    def test_porcelain_type_two_retains_both_endpoints(self) -> None:
        primary = "2 R. N... 100644 100644 100644 a a R100 dst.py"
        rows, operations = parse_porcelain_v2_z(primary + "\0src.py\0")
        self.assertEqual((primary + "\0src.py",), rows)
        self.assertEqual(("src.py", "dst.py"), operations[0].paths)

    def test_porcelain_truncated_type_two_fails_closed(self) -> None:
        with self.assertRaisesRegex(GitChangeControlError, "continuation"):
            parse_porcelain_v2_z(
                "2 R. N... 100644 100644 100644 a a R100 dst.py\0"
            )

    def test_name_status_retains_rename_and_copy_endpoints(self) -> None:
        _, operations = parse_name_status_z(
            "R100\0old.py\0new.py\0C100\0source.py\0copy.py\0"
        )
        self.assertEqual(("old.py", "new.py"), operations[0].paths)
        self.assertEqual(("source.py", "copy.py"), operations[1].paths)

    def test_name_status_truncated_record_fails_closed(self) -> None:
        with self.assertRaisesRegex(GitChangeControlError, "missing a path"):
            parse_name_status_z("R100\0old.py\0")

    def test_all_unmerged_porcelain_states_remain_categorical(self) -> None:
        for status in ("UU", "AA", "DD", "AU", "UA", "DU", "UD"):
            with self.subTest(status=status):
                primary = (
                    f"u {status} N... 100644 100644 100644 100644 "
                    "base ours theirs conflict.txt"
                )
                rows, operations = parse_porcelain_v2_z(primary + "\0")
                self.assertEqual((primary,), rows)
                self.assertEqual(status, operations[0].status)
                self.assertTrue(
                    git_change_control._is_unmerged_operation(operations[0])
                )

        _, operations = parse_name_status_z("U\0conflict.txt\0")
        self.assertTrue(git_change_control._is_unmerged_operation(operations[0]))

    def test_unmerged_index_parser_retains_opaque_stage_evidence(self) -> None:
        raw = (
            "100644 opaque-base 1\tconflict.txt\0"
            "100644 opaque-ours 2\tconflict.txt\0"
            "100644 opaque-theirs 3\tconflict.txt\0"
        )
        rows, entries = parse_unmerged_index_z(raw)
        self.assertEqual(3, len(rows))
        self.assertEqual([1, 2, 3], [entry.stage for entry in entries])
        self.assertEqual("opaque-ours", entries[1].object_id)
        self.assertEqual("conflict.txt", entries[1].path)

    def test_unmerged_index_parser_fails_closed_on_malformed_rows(self) -> None:
        malformed_rows = (
            "100644 object 2 conflict.txt\0",
            "100644 object 2\t/absolute.txt\0",
            "100644 object 2\t../escape.txt\0",
            "10064x object 2\tconflict.txt\0",
            "100644 object 0\tconflict.txt\0",
            "100644 object 2\tconflict.txt",
            (
                "100644 object-a 2\tconflict.txt\0"
                "100644 object-b 2\tconflict.txt\0"
            ),
        )
        for raw in malformed_rows:
            with self.subTest(raw=raw), self.assertRaises(GitChangeControlError):
                parse_unmerged_index_z(raw)

    def test_dirty_worktree_blocks_packet_start(self) -> None:
        temporary, root = self.repository()
        self.addCleanup(temporary.cleanup)
        (root / "source.txt").write_text("changed\n", encoding="utf-8")
        with self.assertRaisesRegex(GitChangeControlError, "clean Git worktree"):
            self.baseline(root, "source.txt")

    def test_handoff_reauthentication_requires_exact_git_top_level(self) -> None:
        temporary, root = self.repository({"nested/source.txt": "source\n"})
        self.addCleanup(temporary.cleanup)
        baseline = self.baseline(root, "nested/source.txt")
        with self.assertRaisesRegex(GitChangeControlError, "Git top level"):
            reauthenticate_clean_git_baseline(
                root / "nested",
                baseline,
                packet_id="issue:work:test:native-git",
                writable_paths=["nested/source.txt"],
            )

    def test_in_closure_modification_addition_and_deletion_pass(self) -> None:
        temporary, root = self.repository({"modify.txt": "a", "delete.txt": "b"})
        self.addCleanup(temporary.cleanup)
        writable = ("modify.txt", "delete.txt", "add.txt")
        baseline = self.baseline(root, *writable)
        (root / "modify.txt").write_text("changed", encoding="utf-8")
        (root / "delete.txt").unlink()
        (root / "add.txt").write_text("new", encoding="utf-8")
        report = self.closeout(root, baseline, *writable)
        self.assertTrue(report["passed"])
        self.assertEqual(set(writable), set(report["changed_paths"]))

    def test_each_out_of_closure_operation_fails(self) -> None:
        for operation in ("modify", "delete", "add"):
            with self.subTest(operation=operation):
                temporary, root = self.repository({"outside.txt": "a"})
                self.addCleanup(temporary.cleanup)
                baseline = self.baseline(root, "inside.txt")
                if operation == "modify":
                    (root / "outside.txt").write_text("changed", encoding="utf-8")
                elif operation == "delete":
                    (root / "outside.txt").unlink()
                else:
                    (root / "new-outside.txt").write_text("new", encoding="utf-8")
                report = self.closeout(root, baseline, "inside.txt")
                self.assertFalse(report["passed"])
                self.assertTrue(report["unexpected_paths"])

    def test_rename_requires_both_endpoints_writable(self) -> None:
        temporary, root = self.repository({"old.txt": "same\n"})
        self.addCleanup(temporary.cleanup)
        both = self.baseline(root, "old.txt", "new.txt")
        os.replace(root / "old.txt", root / "new.txt")
        self.assertTrue(self.closeout(root, both, "old.txt", "new.txt")["passed"])
        destination_only = dict(both, writable_paths=["new.txt"])
        report = self.closeout(root, destination_only, "new.txt")
        self.assertFalse(report["passed"])
        self.assertIn("old.txt", report["unexpected_paths"])

    def test_ordinary_exact_copy_requires_only_destination(self) -> None:
        temporary, root = self.repository({"outside.txt": "same\n"})
        self.addCleanup(temporary.cleanup)
        baseline = self.baseline(root, "copy.txt")
        (root / "copy.txt").write_bytes((root / "outside.txt").read_bytes())
        report = self.closeout(root, baseline, "copy.txt")
        self.assertTrue(report["passed"])
        self.assertEqual(["copy.txt"], report["changed_paths"])
        self.assertFalse(report["read_dependencies_are_write_authority"])

    def test_ordinary_nonexact_copy_requires_only_destination(self) -> None:
        temporary, root = self.repository({"outside.txt": "source\n"})
        self.addCleanup(temporary.cleanup)
        baseline = self.baseline(root, "copy.txt")
        (root / "copy.txt").write_text("source then edited\n", encoding="utf-8")
        self.assertTrue(self.closeout(root, baseline, "copy.txt")["passed"])

    def test_copy_source_is_suppressed_only_when_independently_unchanged(self) -> None:
        temporary, root = self.repository({"source.txt": "source\n"})
        self.addCleanup(temporary.cleanup)
        baseline = self.baseline(root, "copy.txt")
        original = git_change_control._run_git_text

        def report_for(extra_status: str = "", *, unmerged: bool = False):
            diff = "C100\0source.txt\0copy.txt\0" + (
                f"{extra_status}\0source.txt\0" if extra_status else ""
            )

            def synthetic(repo: Path, arguments: list[str]) -> str:
                if arguments[:4] == [
                    "diff", "--no-ext-diff", "--name-status", "-z",
                ]:
                    return diff
                if arguments == ["status", "--porcelain=v2", "-z", "--untracked-files=all"]:
                    return ""
                if arguments == ["ls-files", "-u", "-z"]:
                    return (
                        "100644 opaque-source 2\tsource.txt\0" if unmerged else ""
                    )
                if arguments == ["ls-files", "-z", "--others", "--exclude-standard"]:
                    return ""
                if arguments[:3] == ["diff", "--no-ext-diff", "--stat"]:
                    return ""
                return original(repo, arguments)

            with mock.patch.object(
                git_change_control, "_run_git_text", side_effect=synthetic
            ):
                return self.closeout(root, baseline, "copy.txt")

        copy_only = report_for()
        self.assertTrue(copy_only["passed"])
        self.assertEqual(["copy.txt"], copy_only["changed_paths"])

        for status in ("M", "D", "T"):
            with self.subTest(status=status):
                report = report_for(status)
                self.assertFalse(report["passed"])
                self.assertEqual(
                    ["copy.txt", "source.txt"], report["changed_paths"]
                )
                self.assertEqual(["source.txt"], report["unexpected_paths"])
                self.assertTrue(any(
                    row["destination_path"] == "source.txt"
                    for row in report["endpoint_violations"]
                ))

        unmerged = report_for("U", unmerged=True)
        self.assertFalse(unmerged["passed"])
        self.assertEqual(["copy.txt", "source.txt"], unmerged["changed_paths"])
        self.assertEqual(["source.txt"], unmerged["unmerged_paths"])
        self.assertIn("source.txt", unmerged["unexpected_paths"])

    def test_copy_with_independently_modified_writable_source_passes(self) -> None:
        temporary, root = self.repository({"source.txt": "source\n"})
        self.addCleanup(temporary.cleanup)
        baseline = self.baseline(root, "copy.txt", "source.txt")
        original = git_change_control._run_git_text

        def synthetic(repo: Path, arguments: list[str]) -> str:
            if arguments[:4] == ["diff", "--no-ext-diff", "--name-status", "-z"]:
                return (
                    "C100\0source.txt\0copy.txt\0"
                    "M\0source.txt\0"
                )
            if arguments == ["status", "--porcelain=v2", "-z", "--untracked-files=all"]:
                return ""
            if arguments in (
                ["ls-files", "-u", "-z"],
                ["ls-files", "-z", "--others", "--exclude-standard"],
            ):
                return ""
            if arguments[:3] == ["diff", "--no-ext-diff", "--stat"]:
                return ""
            return original(repo, arguments)

        with mock.patch.object(git_change_control, "_run_git_text", side_effect=synthetic):
            report = self.closeout(root, baseline, "copy.txt", "source.txt")
        self.assertTrue(report["passed"])
        self.assertEqual(["copy.txt", "source.txt"], report["changed_paths"])

    def test_copy_source_used_by_independent_rename_is_retained(self) -> None:
        temporary, root = self.repository({"source.txt": "source\n"})
        self.addCleanup(temporary.cleanup)
        baseline = self.baseline(root, "copy.txt")
        original = git_change_control._run_git_text

        def synthetic(repo: Path, arguments: list[str]) -> str:
            if arguments[:4] == ["diff", "--no-ext-diff", "--name-status", "-z"]:
                return (
                    "C100\0source.txt\0copy.txt\0"
                    "R100\0source.txt\0moved.txt\0"
                )
            if arguments == ["status", "--porcelain=v2", "-z", "--untracked-files=all"]:
                return ""
            if arguments in (
                ["ls-files", "-u", "-z"],
                ["ls-files", "-z", "--others", "--exclude-standard"],
            ):
                return ""
            if arguments[:3] == ["diff", "--no-ext-diff", "--stat"]:
                return ""
            return original(repo, arguments)

        with mock.patch.object(git_change_control, "_run_git_text", side_effect=synthetic):
            report = self.closeout(root, baseline, "copy.txt")
        self.assertFalse(report["passed"])
        self.assertEqual(
            ["copy.txt", "moved.txt", "source.txt"], report["changed_paths"]
        )
        self.assertEqual(
            ["moved.txt", "source.txt"], report["unexpected_paths"]
        )

    def test_nonempty_unmerged_index_blocks_even_if_other_views_are_clean(self) -> None:
        temporary, root = self.repository()
        self.addCleanup(temporary.cleanup)
        baseline = self.baseline(root, "source.txt")
        original = git_change_control._run_git_text

        def unmerged_only(repo: Path, arguments: list[str]) -> str:
            if arguments == ["ls-files", "-u", "-z"]:
                return "100644 opaque-object 2\tsource.txt\0"
            return original(repo, arguments)

        with mock.patch.object(
            git_change_control, "_run_git_text", side_effect=unmerged_only
        ):
            report = self.closeout(root, baseline, "source.txt")

        self.assertFalse(report["passed"])
        self.assertEqual(["source.txt"], report["unmerged_paths"])
        self.assertEqual(["source.txt"], report["changed_paths"])
        self.assertEqual(["source.txt"], report["unexpected_paths"])
        self.assertEqual(2, report["unmerged_index_entries"][0]["stage"])

    def test_generated_ignored_churn_is_nonauthoritative_and_nonblocking(self) -> None:
        temporary, root = self.repository()
        self.addCleanup(temporary.cleanup)
        (root / ".gitignore").write_text(
            "/build/\n*.pyc\n/retained.lock\n/new.lock\n", encoding="utf-8"
        )
        run_git(root, "add", ".gitignore")
        run_git(root, "commit", "-q", "-m", "ignored generated paths")
        retained = root / "retained.lock"
        retained.write_text("generated\n", encoding="utf-8")
        baseline = self.baseline(root, "source.txt")

        retained.unlink()
        for relative in (
            "build/unowned-audit-output/candidate.obj",
            "local.pyc",
            "new.lock",
        ):
            path = root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("generated\n", encoding="utf-8")

        report = self.closeout(root, baseline, "source.txt")
        self.assertTrue(report["passed"])
        self.assertEqual([], report["ignored_added_paths"])
        self.assertEqual([], report["ignored_removed_paths"])
        self.assertTrue(report["ignored_delta_fields_deprecated"])
        self.assertEqual([], report["changed_paths"])
        self.assertEqual([], report["unexpected_paths"])
        self.assertFalse(report["ignored_generated_paths_packet_gated"])
        self.assertFalse(report["ignored_generated_paths_inspected"])
        self.assertEqual(
            "nonauthoritative-generated-state",
            report["ignored_generated_policy"],
        )
        self.assertFalse(report["ignored_content_comparison_performed"])

    def test_reserve_and_closeout_never_invoke_ignored_path_census(self) -> None:
        temporary, root = self.repository()
        self.addCleanup(temporary.cleanup)
        original = git_change_control._run_git_text
        calls: list[tuple[str, ...]] = []

        def observe(repo: Path, arguments: list[str]) -> str:
            calls.append(tuple(arguments))
            return original(repo, arguments)

        with mock.patch.object(
            git_change_control, "_run_git_text", side_effect=observe
        ):
            baseline = self.baseline(root, "source.txt")
            report = self.closeout(root, baseline, "source.txt")

        self.assertTrue(report["passed"])
        self.assertFalse(any(
            call[:4] == ("ls-files", "--others", "--ignored", "--exclude-standard")
            for call in calls
        ))

    def test_baseline_contains_no_duplicate_file_layer(self) -> None:
        temporary, root = self.repository()
        self.addCleanup(temporary.cleanup)
        baseline = self.baseline(root, "source.txt")
        self.assertEqual(
            {"schema", "packet_id", "baseline_commit", "branch",
             "writable_paths", "status_porcelain_v2", "ignored_paths",
             "git_object_ids_are_opaque"},
            set(baseline),
        )
        self.assertEqual("recoil-git-workspace-baseline-v2", baseline["schema"])
        text = repr(baseline).casefold()
        for forbidden in (
            "authored_baseline", "primary", "seal", "cleanup_debt",
            "file_bytes", "file_sizes", "timestamps", "physical_identities",
            "hash", "checksum", "tree_summary",
        ):
            self.assertNotIn(forbidden, text)

    def test_new_baseline_keeps_deprecated_ignored_paths_empty(self) -> None:
        temporary, root = self.repository({"source.txt": "retail\n"})
        self.addCleanup(temporary.cleanup)
        (root / ".gitignore").write_text(
            "build/\n*.lock\n.agent/.recoil-cross-ledger-reservation.revision.lock\n",
            encoding="utf-8",
        )
        run_git(root, "add", ".gitignore")
        run_git(root, "commit", "-q", "-m", "ignore generated paths")
        for relative in (
            "build/z.obj",
            "build/a.obj",
            "other.lock",
            ".agent/.recoil-cross-ledger-reservation.revision.lock",
        ):
            path = root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("generated\n", encoding="utf-8")

        baseline = self.baseline(root, "source.txt")

        self.assertEqual(GIT_WORKSPACE_BASELINE_SCHEMA, baseline["schema"])
        self.assertEqual([], baseline["ignored_paths"])

    def test_v2_descriptor_missing_or_malformed_ignored_paths_fails_closed(self) -> None:
        temporary, root = self.repository()
        self.addCleanup(temporary.cleanup)
        baseline = self.baseline(root, "source.txt")
        packet_id = "issue:work:test:native-git"

        missing = dict(baseline)
        missing.pop("ignored_paths")
        with self.assertRaisesRegex(GitChangeControlError, "fields"):
            validate_git_baseline_descriptor(
                missing, packet_id=packet_id, writable_paths=["source.txt"]
            )

        historical = dict(baseline, schema="recoil-git-workspace-baseline-v1")
        with self.assertRaisesRegex(GitChangeControlError, "unsupported"):
            validate_git_baseline_descriptor(
                historical, packet_id=packet_id, writable_paths=["source.txt"]
            )

        compatible = dict(
            baseline,
            ignored_paths=["build/a.obj", "build/preexisting.obj"],
        )
        validated = validate_git_baseline_descriptor(
            compatible, packet_id=packet_id, writable_paths=["source.txt"]
        )
        self.assertEqual(compatible["ignored_paths"], validated["ignored_paths"])

        mutex_compatible = dict(
            baseline,
            ignored_paths=[
                ".agent/.recoil-cross-ledger-reservation.revision.lock"
            ],
        )
        self.assertEqual(
            mutex_compatible["ignored_paths"],
            validate_git_baseline_descriptor(
                mutex_compatible,
                packet_id=packet_id,
                writable_paths=["source.txt"],
            )["ignored_paths"],
        )

        for value in (
            ["/absolute.obj"],
            ["../escape.obj"],
            ["build\\not-normalized.obj"],
            ["build/z.obj", "build/a.obj"],
            ["build/a.obj", "build/a.obj"],
        ):
            with self.subTest(value=value):
                malformed = dict(baseline, ignored_paths=value)
                with self.assertRaises(GitChangeControlError):
                    validate_git_baseline_descriptor(
                        malformed,
                        packet_id=packet_id,
                        writable_paths=["source.txt"],
                    )

    def test_maintained_authored_surfaces_are_git_visible(self) -> None:
        project_root = Path(__file__).resolve().parents[2]
        maintained = (
            "src", "tools", "tests/tools", "docs", "cmake", "AGENTS.md",
            "CLAUDE.md", ".codex/agents", ".codex/skills",
            ".claude/agents", ".claude/skills", ".agent/AGENTS.md",
        )
        for path in maintained:
            with self.subTest(path=path):
                self.assertTrue(run_git(project_root, "ls-files", "--", path).strip())

        probes = (
            "tools/recoil_visibility_probe.py",
            "tests/tools/recoil_visibility_probe_tests.py",
            ".agent/recoil_visibility_policy.md",
        )
        for path in probes:
            with self.subTest(path=path):
                result = subprocess.run(
                    ["git", "check-ignore", "-q", "--", path],
                    cwd=project_root, check=False, capture_output=True,
                )
                self.assertEqual(1, result.returncode)

        rules = {
            line.strip()
            for line in (project_root / ".gitignore").read_text(
                encoding="utf-8"
            ).splitlines()
            if line.strip() and not line.lstrip().startswith("#")
        }
        for forbidden in (
            "/tools/", "/tests/tools/", "/src/", ".agent/", "/.agent/",
            "*.py", "*.cpp", "*.h",
        ):
            self.assertNotIn(forbidden, rules)


if __name__ == "__main__":
    unittest.main()
