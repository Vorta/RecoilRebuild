from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.final_image_catalog_audit import audit_catalog  # noqa: E402
from _recoil.lib.progress import empty_progress_document  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402


class FinalImageCatalogAuditTests(unittest.TestCase):
    @staticmethod
    def sqlite_tracker(path: Path, *, revision: int) -> Path:
        document = empty_progress_document()
        document["revision"] = revision
        ProgressSQLiteStore.create_from_mapping(
            path,
            document,
            cutover_pair_id="final-image-catalog-audit-test",
        )
        return path

    def test_missing_reference_fails_without_legacy_catalog_fields(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tracker = self.sqlite_tracker(root / "progress.sqlite3", revision=10)
            result = audit_catalog(tracker=tracker, reference=root / "missing.exe")
        self.assertFalse(result["passed"])
        self.assertIn("live validation deliberately does not fabricate", result["failures"][0])
        self.assertNotIn("catalog_state", result)
        self.assertNotIn("legacy_catalog_present", result)
        self.assertNotIn("legacy_catalog_diagnostics", result)

    def test_sqlite_tracker_is_loaded_semantically(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tracker = self.sqlite_tracker(root / "progress.sqlite3", revision=27)
            result = audit_catalog(tracker=tracker, reference=root / "missing.exe")
        self.assertEqual(6, result["tracker_schema"])
        self.assertEqual(27, result["tracker_revision"])
        self.assertIsNone(result["coverage"])


if __name__ == "__main__":
    unittest.main()
