from __future__ import annotations

import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.current_metadata_audit import (  # noqa: E402
    REFRESH_COMMAND,
    audit_current_metadata,
)
from _recoil.commands.current_metadata_mutation import (  # noqa: E402
    CurrentMetadataMutationError,
    build_parser,
    refresh_current_metadata,
)
from _recoil.lib.progress import (  # noqa: E402
    ProgressDocument,
    ProgressStore,
    empty_progress_document,
)


WORK_A = "recoil:work:required_dependency_hud_matrix"
WORK_B = "recoil:work:required_dependency_hud_remote_order"
NARRATIVE = (
    "Whether changing only the projected remote HudUiElement definition order to retail "
    "order repairs A/B natural selected order while the actual-source C control retains "
    "the current divergence."
)


def fixture_data(*, revision: int = 7) -> dict[str, object]:
    data = empty_progress_document()
    data["revision"] = revision
    data["migration"] = {"unrelated": {"preserve": [1, 2, 3]}}
    data["binaries"] = {
        "recoil": {
            "text": {"start": "0x401000", "end_exclusive": "0x4cb9e8"},
            "source_layout_context": {
                "provenance_status_summary": {
                    "remaining_blocker": "Dynamic scheduler state: consult generated_current.",
                    "remaining_blocker_history": [
                        {
                            "historicalized_at_revision": 4,
                            "narrative": "Earlier source-layout conclusion.",
                        }
                    ],
                    "generated_current": {
                        "metadata_kind": "generated-current",
                        "tracker_revision": 3,
                        "command": "python tools/recoil.py progress next --json",
                        "lane": "primary",
                        "phase": "authored-function-order",
                        "cursor": "0x404ca0",
                        "reason_code": "old",
                        "required_parent_action": "old",
                        "narrative": "Old generated display row.",
                    },
                }
            },
        }
    }
    data["work_items"] = {
        WORK_A: {
            "state": "closed",
            "first_unresolved_item": NARRATIVE,
            "first_unresolved_item_history": [
                {
                    "historicalized_at_revision": 2,
                    "narrative": "Pre-existing historical question.",
                }
            ],
            "unrelated": {"keep": True},
        },
        WORK_B: {
            "state": "closed",
            "first_unresolved_item": NARRATIVE,
            "unrelated": "also keep",
        },
        "recoil:work:unrelated": {
            "state": "closed",
            "first_unresolved_item": "Historical question with no live-cursor claim.",
            "unrelated": ["untouched"],
        },
    }
    return data


class CurrentMetadataMutationTests(unittest.TestCase):
    def write_fixture(self, root: Path, *, revision: int = 7) -> Path:
        path = root / "progress.json"
        path.write_text(
            json.dumps(fixture_data(revision=revision), indent=2),
            encoding="utf-8",
        )
        return path

    def test_stale_audit_dry_run_apply_and_strict_audit_pass(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            stale = audit_current_metadata(ProgressDocument.load(progress))
            codes = {item["code"] for item in stale}
            self.assertIn("generated-current-stale", codes)
            self.assertEqual(
                2,
                sum(item["code"] == "static-current-narrative" for item in stale),
            )

            before = progress.read_text(encoding="utf-8")
            dry_run = refresh_current_metadata(
                progress=progress,
                expected_revision=7,
                apply=False,
            )
            self.assertFalse(dry_run["commit"]["applied"])
            self.assertEqual(8, dry_run["commit"]["revision"])
            self.assertEqual(before, progress.read_text(encoding="utf-8"))

            applied = refresh_current_metadata(
                progress=progress,
                expected_revision=7,
                apply=True,
            )
            self.assertTrue(applied["commit"]["applied"])
            self.assertEqual({WORK_A, WORK_B}, {
                item["work_item_id"] for item in applied["historicalized_work_items"]
            })

            stored = json.loads(progress.read_text(encoding="utf-8"))
            self.assertEqual(8, stored["revision"])
            self.assertEqual([], audit_current_metadata(ProgressDocument(stored)))
            generated = stored["binaries"]["recoil"]["source_layout_context"][
                "provenance_status_summary"
            ]["generated_current"]
            self.assertEqual(8, generated["tracker_revision"])
            self.assertEqual("0x4cb9e8", generated["cursor"])
            self.assertEqual("final-validation", generated["phase"])
            self.assertEqual("0x4cb9e8", generated["scope_identity"]["cursor"])

    def test_material_history_and_unrelated_fields_are_preserved(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            original = json.loads(progress.read_text(encoding="utf-8"))
            refresh_current_metadata(
                progress=progress,
                expected_revision=7,
                apply=True,
            )
            stored = json.loads(progress.read_text(encoding="utf-8"))

            self.assertEqual(original["migration"], stored["migration"])
            self.assertEqual(
                original["work_items"]["recoil:work:unrelated"],
                stored["work_items"]["recoil:work:unrelated"],
            )
            self.assertEqual(
                {"keep": True}, stored["work_items"][WORK_A]["unrelated"]
            )
            self.assertIn(
                "projected remote HudUiElement definition order",
                stored["work_items"][WORK_A]["first_unresolved_item"],
            )
            self.assertIn(
                "divergence observed at that time",
                stored["work_items"][WORK_A]["first_unresolved_item"],
            )
            history = stored["work_items"][WORK_A]["first_unresolved_item_history"]
            self.assertEqual("Pre-existing historical question.", history[0]["narrative"])
            self.assertEqual(NARRATIVE, history[1]["narrative"])
            self.assertEqual(8, history[1]["historicalized_at_revision"])
            self.assertEqual(
                original["binaries"]["recoil"]["source_layout_context"][
                    "provenance_status_summary"
                ]["remaining_blocker_history"],
                stored["binaries"]["recoil"]["source_layout_context"][
                    "provenance_status_summary"
                ]["remaining_blocker_history"],
            )

    def test_revision_guard_rejects_stale_expected_revision(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            before = progress.read_text(encoding="utf-8")
            with self.assertRaisesRegex(
                CurrentMetadataMutationError,
                "revision changed: expected 6, found 7",
            ):
                refresh_current_metadata(
                    progress=progress,
                    expected_revision=6,
                    apply=True,
                )
            self.assertEqual(before, progress.read_text(encoding="utf-8"))

            refresh_current_metadata(
                progress=progress,
                expected_revision=7,
                apply=True,
            )
            with self.assertRaisesRegex(
                CurrentMetadataMutationError,
                "revision changed: expected 7, found 8",
            ):
                refresh_current_metadata(
                    progress=progress,
                    expected_revision=7,
                    apply=True,
                )
            self.assertEqual(8, json.loads(progress.read_text(encoding="utf-8"))["revision"])

    def test_backend_parser_requires_refresh_revision_and_mode(self) -> None:
        args = build_parser().parse_args(
            ["refresh", "--expected-revision", "7", "--dry-run", "--json"]
        )
        self.assertEqual("refresh", args.command)
        self.assertEqual(7, args.expected_revision)
        self.assertTrue(args.dry_run)
        self.assertEqual(
            "python tools/recoil.py progress current-metadata refresh",
            REFRESH_COMMAND,
        )

    def test_progress_store_refreshes_generated_current_in_the_same_commit(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            store = ProgressStore(progress)
            proposed = store.load().data
            proposed["migration"]["same_cas_probe"] = True

            result = store.commit(proposed, expected_revision=7, apply=True)

            self.assertTrue(result.applied)
            stored = ProgressDocument.load(progress)
            generated = stored.data["binaries"]["recoil"]["source_layout_context"][
                "provenance_status_summary"
            ]["generated_current"]
            self.assertEqual(8, stored.revision)
            self.assertEqual(8, generated["tracker_revision"])
            self.assertEqual("final-validation", generated["phase"])


if __name__ == "__main__":
    unittest.main()
