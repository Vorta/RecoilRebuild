from __future__ import annotations

from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.readme_progress import (  # noqa: E402
    END_MARKER,
    START_MARKER,
    ReadmeProgressError,
    main,
    planned_readme_text,
    readme_freshness_findings,
    render_progress_block,
    synchronize_readme,
)


class ReadmeProgressTests(unittest.TestCase):
    def test_current_tracker_renders_stable_pipeline_and_restored_tables(self) -> None:
        progress = REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
        first = render_progress_block(progress)
        second = render_progress_block(progress)

        self.assertEqual(first, second)
        self.assertTrue(first.startswith(START_MARKER))
        self.assertTrue(first.endswith(END_MARKER))
        for heading in (
            "Live Reconstruction Pipeline",
            "Source-Owner Overview",
            "Source-Owner Gates",
            "Owner Reimplementation Tiers",
            "Function Reimplementation Tiers",
            "Data Reimplementation Tiers",
            "Owner Kinds",
        ):
            self.assertIn(heading, first)
        for forbidden in (
            "tracker_revision",
            "Tracker revision",
            "next_command",
            "reservation",
            "lease",
            "sha" + "256",
        ):
            self.assertNotIn(forbidden, first)

    def test_markers_preserve_all_surrounding_text_and_are_idempotent(self) -> None:
        with TemporaryDirectory() as temporary:
            readme = Path(temporary) / "README.md"
            readme.write_text(
                "# Sample\n\n## Status\n\nBefore.\n\n## License\n\nAfter.\n",
                encoding="utf-8",
            )
            block = f"{START_MARKER}\nGenerated.\n{END_MARKER}"
            before, after = planned_readme_text(readme, block)
            self.assertNotEqual(before, after)
            self.assertIn("Before.", after)
            self.assertIn("## License\n\nAfter.", after)
            readme.write_text(after, encoding="utf-8")
            current, planned = planned_readme_text(readme, block)
            self.assertEqual(current, planned)

    def test_duplicate_or_unbalanced_markers_fail_closed(self) -> None:
        malformed = (
            f"# Sample\n\n## Status\n\n{START_MARKER}\n"
            f"{START_MARKER}\n{END_MARKER}\n"
        )
        with TemporaryDirectory() as temporary:
            readme = Path(temporary) / "README.md"
            readme.write_text(malformed, encoding="utf-8")
            with self.assertRaisesRegex(ReadmeProgressError, "balanced, non-duplicated"):
                planned_readme_text(readme, f"{START_MARKER}\n{END_MARKER}")

    def test_update_then_check_uses_explicit_temporary_readme(self) -> None:
        progress = REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
        with TemporaryDirectory() as temporary:
            readme = Path(temporary) / "README.md"
            readme.write_text("# Sample\n\n## Status\n\nNarrative.\n", encoding="utf-8")
            updated = synchronize_readme(
                progress_path=progress,
                readme_path=readme,
            )
            checked = synchronize_readme(
                progress_path=progress,
                readme_path=readme,
                check=True,
            )
            self.assertTrue(updated["changed"])
            self.assertTrue(checked["current"])
            self.assertFalse(checked["changed"])

    def test_nondefault_tracker_requires_explicit_readme(self) -> None:
        with TemporaryDirectory() as temporary:
            progress = Path(temporary) / "progress.sqlite3"
            self.assertEqual(2, main(["--progress", str(progress), "--check"]))

    def test_freshness_audit_reports_stale_generated_block(self) -> None:
        progress = REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
        with TemporaryDirectory() as temporary:
            readme = Path(temporary) / "README.md"
            readme.write_text("# Sample\n\n## Status\n\nStale.\n", encoding="utf-8")
            findings = readme_freshness_findings(
                progress_path=progress,
                readme_path=readme,
            )
        self.assertEqual(1, len(findings))
        self.assertEqual("readme-progress-stale", findings[0]["code"])


if __name__ == "__main__":
    unittest.main()
