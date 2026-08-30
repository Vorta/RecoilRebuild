from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest

REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.final_image_catalog_audit import audit_catalog  # noqa: E402
from _recoil.lib.pe import parse_pe_headers  # noqa: E402
from _recoil.lib.progress import empty_progress_document  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402


def canonical_retail_reference() -> Path:
    return REPO_ROOT / "support" / "Recoil.exe"

class FinalImageCatalogAuditTests(unittest.TestCase):
    @staticmethod
    def _sqlite_tracker(path: Path, fragment: dict[str, object]) -> Path:
        document = empty_progress_document()
        document.update(fragment)
        ProgressSQLiteStore.create_from_mapping(
            path,
            document,
            cutover_pair_id="final-image-catalog-audit-test",
        )
        return path

    def test_missing_catalog_is_a_precise_fail_closed_result(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tracker = self._sqlite_tracker(
                root / "progress.sqlite3",
                {
                    "revision": 10,
                    "binaries": {
                        "recoil": {
                            "final_image_catalog_state": {
                                "state": "blocked",
                                "next_command": "python tools/recoil.py audit final-image-catalog --json",
                            }
                        }
                    },
                },
            )
            result = audit_catalog(tracker=tracker, reference=root / "missing.exe")
        self.assertFalse(result["passed"])
        self.assertEqual(result["catalog_path"], "binaries.recoil.final_image_catalog")
        self.assertIn("live validation deliberately does not fabricate", result["failures"][0])

    def test_sqlite_tracker_is_loaded_semantically(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tracker = self._sqlite_tracker(
                root / "progress.sqlite3",
                {
                    "revision": 27,
                    "binaries": {
                        "recoil": {
                            "final_image_catalog_state": {
                                "state": "blocked",
                                "next_command": (
                                    "python tools/recoil.py audit final-image-catalog --json"
                                ),
                            }
                        }
                    },
                },
            )
            result = audit_catalog(tracker=tracker, reference=root / "missing.exe")
        self.assertFalse(result["passed"])
        self.assertEqual(result["tracker_schema"], 6)
        self.assertEqual(result["tracker_revision"], 27)
        self.assertEqual(result["catalog_state"]["state"], "blocked")
        self.assertIn("live validation deliberately does not fabricate", result["failures"][0])

    def test_legacy_catalog_is_diagnostic_and_does_not_supply_live_coverage(self) -> None:
        reference = canonical_retail_reference()
        headers = parse_pe_headers(reference.read_bytes(), source=str(reference))
        sections: dict[str, object] = {}
        for section in headers.sections:
            entity: dict[str, object] = {
                "id": f"fake:{section.name}",
                "file_start": 0,
                "file_end": section.raw_size,
                "virtual_start": 0,
                "virtual_end": section.virtual_size,
            }
            if section.name == ".text":
                entity.update(
                    kind="address-group",
                    identities=[
                        {
                            "symbol_id": "recoil:function:fake",
                            "map_symbol": "fake",
                            "object": "fake.obj",
                            "source_block_id": "recoil:block:fake",
                            "contribution_class": "authored",
                            "comdat": False,
                        }
                    ],
                    relocations=[],
                )
            elif section.name in {".rdata", ".data"}:
                entity.update(kind="initialized-data", source_id="recoil:storage:fake")
            elif section.name == ".rsrc":
                entity["kind"] = "resource"
            elif section.name == ".reloc":
                entity["kind"] = "relocations"
            else:
                entity["kind"] = "section-payload"
            if section.raw_size == 0:
                entity.pop("file_start")
                entity.pop("file_end")
            if section.virtual_size == 0:
                entity.pop("virtual_start")
                entity.pop("virtual_end")
            sections[section.name] = {"entities": [entity]}
        catalog = {
            "version": 1,
            "binary": "recoil",
            "directories": "exact-including-absence",
            "overlay": {"mode": "exact"},
            "sections": sections,
        }
        with tempfile.TemporaryDirectory() as temporary:
            tracker = self._sqlite_tracker(
                Path(temporary) / "progress.sqlite3",
                {
                    "revision": 11,
                    "binaries": {"recoil": {"final_image_catalog": catalog}},
                },
            )
            result = audit_catalog(tracker=tracker, reference=reference)
        self.assertFalse(result["passed"])
        self.assertTrue(result["legacy_catalog_present"])
        self.assertFalse(result["legacy_catalog_required"])
        self.assertTrue(any("does not resolve" in row for row in result["legacy_catalog_diagnostics"]))
        self.assertFalse(any("legacy catalog" in row for row in result["failures"]))


if __name__ == "__main__":
    unittest.main()
