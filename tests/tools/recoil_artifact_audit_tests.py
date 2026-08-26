from __future__ import annotations

import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.artifact_audit import (  # noqa: E402
    collect_artifacts,
    collect_session_artifacts,
    delete_selected,
    find_durable_devspace_references,
    main,
    validate_session_scratch,
)


class RecoilArtifactAuditTests(unittest.TestCase):
    def make_dir(self, root: Path, rel: str, *, age_days: int = 30) -> Path:
        path = root / rel
        path.mkdir(parents=True)
        (path / "sample.obj").write_bytes(b"obj")
        timestamp = 1_700_000_000 - age_days * 86400
        os.utime(path / "sample.obj", (timestamp, timestamp))
        os.utime(path, (timestamp, timestamp))
        return path

    def test_collects_one_aggregate_build_target_with_every_direct_entry(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_dir(root, "build/vc5-verify-sample", age_days=30)
            self.make_dir(root, "build/vc5-verify", age_days=30)
            loose = root / "build/loose.log"
            loose.write_bytes(b"loose")

            artifacts = collect_artifacts(root, older_than_days=14, now=1_700_000_000)

        self.assertEqual(1, len(artifacts))
        build = artifacts[0]
        self.assertEqual(root / "build", build.path)
        self.assertTrue(build.selected)
        self.assertTrue(build.retain_root)
        self.assertEqual(3, build.direct_entry_count)
        self.assertEqual(11, build.size_bytes)
        self.assertIn("name and age filters do not apply", build.reason)

    def test_build_selection_ignores_age_and_all_switch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_dir(root, "build/arbitrary-fresh-name", age_days=0)

            default = collect_artifacts(root, older_than_days=999, now=1_700_000_000)
            all_ages = collect_artifacts(root, older_than_days=999, all_ages=True, now=1_700_000_000)

        self.assertTrue(default[0].selected)
        self.assertTrue(all_ages[0].selected)

    def test_vs_and_playground_require_explicit_include(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_dir(root, ".vs", age_days=30)
            self.make_dir(root, "playground", age_days=30)

            default_artifacts = collect_artifacts(root, now=1_700_000_000)
            included_artifacts = collect_artifacts(
                root,
                include_vs=True,
                include_playground=True,
                now=1_700_000_000,
            )

        self.assertFalse(any(item.selected for item in default_artifacts))
        self.assertEqual(2, sum(1 for item in included_artifacts if item.selected))

    def test_delete_selected_empties_build_and_retains_real_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            removed = self.make_dir(root, "build/vc5-probe-old", age_days=30)
            former_protected = self.make_dir(root, "build/vc5-verify", age_days=30)
            loose = root / "build/loose.obj"
            loose.write_bytes(b"loose")
            artifacts = collect_artifacts(root, now=1_700_000_000)

            errors = delete_selected(root, artifacts)

            self.assertEqual([], errors)
            self.assertFalse(removed.exists())
            self.assertFalse(former_protected.exists())
            self.assertFalse(loose.exists())
            self.assertTrue((root / "build").is_dir())
            self.assertEqual([], list((root / "build").iterdir()))

    def test_cli_dry_run_does_not_delete(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            candidate = self.make_dir(root, "build/vc5-probe-old", age_days=30)

            rc = main(["--root", str(root), "--all"])

            self.assertEqual(0, rc)
            self.assertTrue(candidate.exists())

    def test_normal_absent_build_is_idempotent(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)

            self.assertEqual(0, main(["--root", str(root)]))
            self.assertEqual(0, main(["--root", str(root), "--delete"]))
            self.assertEqual(0, main(["--root", str(root), "--delete"]))
            self.assertFalse((root / "build").exists())

    def test_normal_empty_build_is_idempotent_and_retained(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build = root / "build"
            build.mkdir()

            self.assertEqual(0, main(["--root", str(root)]))
            self.assertEqual(0, main(["--root", str(root), "--delete"]))
            self.assertEqual(0, main(["--root", str(root), "--delete"]))
            self.assertTrue(build.is_dir())
            self.assertEqual([], list(build.iterdir()))

    def test_cli_delete_empties_build_but_leaves_unselected_local_roots(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            removed = self.make_dir(root, "build/arbitrary/fresh", age_days=0)
            local = self.make_dir(root, ".vs/keep", age_days=30)

            rc = main(["--root", str(root), "--delete", "--older-than-days", "999"])

            self.assertEqual(0, rc)
            self.assertFalse(removed.exists())
            self.assertTrue((root / "build").is_dir())
            self.assertEqual([], list((root / "build").iterdir()))
            self.assertTrue(local.exists())

    def test_age_options_only_control_opted_in_local_roots(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_dir(root, "build/fresh", age_days=0)
            self.make_dir(root, ".vs", age_days=0)

            age_filtered = collect_artifacts(
                root,
                older_than_days=999,
                include_vs=True,
                now=1_700_000_000,
            )
            all_ages = collect_artifacts(
                root,
                older_than_days=999,
                all_ages=True,
                include_vs=True,
                now=1_700_000_000,
            )

        age_by_name = {item.path.name: item for item in age_filtered}
        all_by_name = {item.path.name: item for item in all_ages}
        self.assertTrue(age_by_name["build"].selected)
        self.assertFalse(age_by_name[".vs"].selected)
        self.assertTrue(all_by_name["build"].selected)
        self.assertTrue(all_by_name[".vs"].selected)

    def test_cli_rejects_build_file_before_cleanup(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "build").write_text("not a directory", encoding="utf-8")

            rc = main(["--root", str(root), "--delete"])

            self.assertEqual(1, rc)
            self.assertTrue((root / "build").is_file())

    def test_cli_rejects_build_symlink_without_touching_target(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            outside = root / "outside"
            outside.mkdir()
            sentinel = outside / "keep.obj"
            sentinel.write_bytes(b"keep")
            try:
                os.symlink(outside, root / "build", target_is_directory=True)
            except OSError as exc:
                self.skipTest(f"directory symlink unavailable: {exc}")

            rc = main(["--root", str(root), "--delete"])

            self.assertEqual(1, rc)
            self.assertTrue(sentinel.exists())

    def test_cli_rejects_symlink_repository_root_without_touching_target(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            parent = Path(tmp)
            target = parent / "repository"
            target.mkdir()
            sentinel = self.make_dir(target, "build/keep")
            selected_root = parent / "repository-link"
            try:
                os.symlink(target, selected_root, target_is_directory=True)
            except OSError as exc:
                self.skipTest(f"directory symlink unavailable: {exc}")

            rc = main(["--root", str(selected_root), "--delete"])

            self.assertEqual(1, rc)
            self.assertTrue(sentinel.exists())

    def test_cli_rejects_opted_in_local_root_symlink(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            outside = root / "outside"
            outside.mkdir()
            sentinel = outside / "keep.obj"
            sentinel.write_bytes(b"keep")
            try:
                os.symlink(outside, root / ".vs", target_is_directory=True)
            except OSError as exc:
                self.skipTest(f"directory symlink unavailable: {exc}")

            rc = main(["--root", str(root), "--include-vs", "--all", "--delete"])

            self.assertEqual(1, rc)
            self.assertTrue(sentinel.exists())

    def test_nested_symlink_is_unlinked_without_following(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build = root / "build"
            build.mkdir()
            outside = root / "outside"
            outside.mkdir()
            sentinel = outside / "keep.obj"
            sentinel.write_bytes(b"keep")
            try:
                os.symlink(outside, build / "linked", target_is_directory=True)
            except OSError as exc:
                self.skipTest(f"directory symlink unavailable: {exc}")

            errors = delete_selected(root, collect_artifacts(root))

            self.assertEqual([], errors)
            self.assertTrue(sentinel.exists())
            self.assertEqual([], list(build.iterdir()))

    def test_nested_windows_junction_is_unlinked_without_following(self) -> None:
        if os.name != "nt":
            self.skipTest("Windows junction coverage requires Windows")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build = root / "build"
            build.mkdir()
            outside = root / "outside"
            outside.mkdir()
            sentinel = outside / "keep.obj"
            sentinel.write_bytes(b"keep")
            junction = build / "linked-junction"
            completed = subprocess.run(
                ["cmd", "/c", "mklink", "/J", str(junction), str(outside)],
                text=True,
                capture_output=True,
            )
            if completed.returncode != 0:
                self.skipTest(
                    "junction creation unavailable: "
                    + (completed.stderr.strip() or completed.stdout.strip())
                )

            errors = delete_selected(root, collect_artifacts(root))

            self.assertEqual([], errors)
            self.assertFalse(os.path.lexists(junction))
            self.assertTrue(sentinel.exists())
            self.assertEqual([], list(build.iterdir()))

    def test_unknown_nested_reparse_preflight_blocks_all_targets_before_deletion(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build_sentinel = self.make_dir(root, "build/remove-me")
            unknown = self.make_dir(root, ".vs/unknown")
            from _recoil.commands import artifact_audit

            original = artifact_audit._reparse_kind

            def classify(path, stat_result=None):
                if Path(path) == unknown:
                    return "unknown-reparse"
                return original(path, stat_result)

            with mock.patch("_recoil.commands.artifact_audit._reparse_kind", side_effect=classify):
                rc = main(["--root", str(root), "--include-vs", "--all", "--delete"])

            self.assertEqual(1, rc)
            self.assertTrue(build_sentinel.exists())
            self.assertTrue(unknown.exists())

    def test_read_only_local_entries_are_removed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build = root / "build"
            build.mkdir()
            readonly = build / "readonly.obj"
            readonly.write_bytes(b"obj")
            readonly.chmod(stat.S_IREAD)

            errors = delete_selected(root, collect_artifacts(root))

            self.assertEqual([], errors)
            self.assertTrue(build.is_dir())
            self.assertEqual([], list(build.iterdir()))

    def test_session_dry_run_retains_fresh_devspace_and_ignores_age(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            scratch = self.make_dir(root, ".devspace", age_days=0)

            rc = main(["--root", str(root), "--session-only"])
            artifacts, errors = collect_session_artifacts(root, now=1_700_000_000)

            self.assertEqual(0, rc)
            self.assertEqual([], errors)
            self.assertEqual([scratch], [item.path for item in artifacts])
            self.assertTrue(scratch.exists())

    def test_session_delete_removes_only_devspace(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            scratch = self.make_dir(root, ".devspace", age_days=0)
            sentinels = [
                self.make_dir(root, "build/vc5-probe-old"),
                self.make_dir(root, "artifacts/keep"),
                self.make_dir(root, "support/keep"),
                self.make_dir(root, ".vs"),
                self.make_dir(root, "playground"),
            ]

            rc = main(["--root", str(root), "--session-only", "--delete"])

            self.assertEqual(0, rc)
            self.assertFalse(scratch.exists())
            self.assertTrue(all(path.exists() for path in sentinels))

    def test_session_absent_is_idempotent_success(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertEqual(0, main(["--root", str(root), "--session-only"]))
            self.assertEqual(0, main(["--root", str(root), "--session-only", "--delete"]))

    def test_session_rejects_conflicting_selection_flags(self) -> None:
        conflicts = (
            ["--all"],
            ["--include-vs"],
            ["--include-playground"],
            ["--older-than-days", "15"],
        )
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for conflict in conflicts:
                with self.subTest(conflict=conflict):
                    self.assertEqual(
                        2,
                        main(["--root", str(root), "--session-only", *conflict]),
                    )

    def test_normal_audit_never_collects_devspace(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.make_dir(root, ".devspace", age_days=30)
            artifacts = collect_artifacts(root, all_ages=True)
            self.assertFalse(any(item.path.name == ".devspace" for item in artifacts))

    def test_session_rejects_symlink_or_escape_target(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            outside = Path(tmp).parent / f"{root.name}-outside"
            outside.mkdir(exist_ok=True)
            try:
                try:
                    os.symlink(outside, root / ".devspace", target_is_directory=True)
                except OSError as exc:
                    self.skipTest(f"directory symlink unavailable: {exc}")
                _artifacts, errors = collect_session_artifacts(root)
                self.assertTrue(any("symlink, junction, or reparse" in item for item in errors))
            finally:
                if outside.exists():
                    outside.rmdir()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            errors = validate_session_scratch(root, root / "nested" / ".devspace")
            self.assertTrue(any("direct .devspace child" in item for item in errors))

    def test_durable_concrete_reference_blocks_even_when_scratch_absent(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            doc = root / "docs/reconstruction/evidence.md"
            doc.parent.mkdir(parents=True)
            doc.write_text(
                "receipt: `.devspace/runs/2026-07-11T12-00-00-chatgpt-call/receipt.json`\n"
                "state: `.devspace/state.json`\n"
                "profile: `.devspace/chrome-profiles/session/Preferences`\n",
                encoding="utf-8",
            )

            findings = find_durable_devspace_references(root)
            rc = main(["--root", str(root), "--session-only"])

            self.assertEqual(3, len(findings))
            self.assertEqual(1, findings[0].line)
            self.assertEqual(1, rc)

    def test_placeholder_policy_and_historical_issue_text_do_not_block(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            policy = root / "docs/reconstruction/policy.md"
            policy.parent.mkdir(parents=True)
            policy.write_text(
                "Use `.devspace/runs/<run-id>/receipt.json` only during a session.\n",
                encoding="utf-8",
            )
            issue = root / ".agent/WORKSPACE_ISSUES.json"
            issue.parent.mkdir(parents=True)
            issue.write_text(
                '{"status":"resolved","path":".devspace/runs/old/receipt.json"}',
                encoding="utf-8",
            )

            self.assertEqual([], find_durable_devspace_references(root))
            self.assertEqual(0, main(["--root", str(root), "--session-only"]))

    def test_delete_verifies_retained_build_is_empty(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            remaining = self.make_dir(root, "build/remains")
            artifacts = collect_artifacts(root)
            with mock.patch(
                "_recoil.commands.artifact_audit._remove_tree_contents", return_value=[]
            ):
                errors = delete_selected(root, artifacts)
            self.assertTrue(remaining.exists())
            self.assertTrue(any("directory is not empty" in item for item in errors))


if __name__ == "__main__":
    unittest.main()
