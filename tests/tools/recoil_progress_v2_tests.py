from __future__ import annotations

from pathlib import Path
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.progress_v2 import accept_live_byte_groups  # noqa: E402
from _recoil.lib.progress import (  # noqa: E402
    ProgressDocument,
    ProgressError,
    accepted_byte_mode,
    empty_progress_document,
    invalidate_order_dependencies,
)


class AcceptedByteModeTests(unittest.TestCase):
    def test_decoder_accepts_canonical_and_legacy_modes(self) -> None:
        cases = (
            ({"mode": "authored"}, "authored"),
            ({"mode": "linked"}, "linked"),
            ({"lane": "authored"}, "authored"),
            ({"lane": "linked"}, "linked"),
            ({"mode": "linked", "lane": "linked"}, "linked"),
            ({}, None),
            (None, None),
        )
        for facts, expected in cases:
            with self.subTest(facts=facts):
                self.assertEqual(expected, accepted_byte_mode(facts))

    def test_decoder_rejects_conflicts_unknowns_and_malformed_facts(self) -> None:
        cases = (
            {"mode": "linked", "lane": "authored"},
            {"mode": "object"},
            {"lane": "binary"},
            [],
        )
        for facts in cases:
            with self.subTest(facts=facts):
                with self.assertRaises(ProgressError):
                    accepted_byte_mode(facts)

    @staticmethod
    def tracker() -> tuple[dict[str, object], str]:
        data = empty_progress_document()
        symbol_id = "recoil:function:unit-byte"
        data["symbols"][symbol_id] = {"binary": "recoil", "binary_state": {}}
        return data, symbol_id

    def test_writer_persists_only_canonical_live_mode_and_preserves_provenance(self) -> None:
        data, symbol_id = self.tracker()
        accepted_live = accept_live_byte_groups(
            data,
            mode="linked",
            groups=((symbol_id,),),
            evidence_id="recoil:evidence:r1:000001",
            facts={"source": "unit"},
        )
        self.assertEqual([symbol_id], accepted_live)
        facts = data["symbols"][symbol_id]["accepted_byte_facts"]
        self.assertEqual(
            {"source": "unit", "validation_mode": "live", "mode": "linked"},
            facts,
        )
        self.assertNotIn("lane", facts)

    def test_writer_rejects_legacy_conflicting_and_non_live_input(self) -> None:
        cases = (
            ("linked", {"lane": "linked"}),
            ("linked", {"mode": "authored"}),
            ("linked", {"validation_mode": "saved"}),
            ("object", {}),
        )
        for mode, facts in cases:
            data, symbol_id = self.tracker()
            with self.subTest(mode=mode, facts=facts):
                with self.assertRaises(ProgressError):
                    accept_live_byte_groups(
                        data,
                        mode=mode,
                        groups=((symbol_id,),),
                        evidence_id="recoil:evidence:r1:000001",
                        facts=facts,
                    )

    def test_progress_audit_accepts_legacy_live_facts_and_rejects_bad_provenance(self) -> None:
        data, symbol_id = self.tracker()
        data["symbols"][symbol_id]["accepted_byte_facts"] = {
            "validation_mode": "live",
            "lane": "authored",
        }
        self.assertFalse(
            any(
                finding.code.startswith("symbol.accepted-byte")
                for finding in ProgressDocument(data).audit()
            )
        )

        data["symbols"][symbol_id]["accepted_byte_facts"] = {
            "validation_mode": "saved",
            "mode": "linked",
            "lane": "authored",
        }
        findings = ProgressDocument(data).audit()
        self.assertTrue(
            any(finding.code == "symbol.accepted-byte-facts" for finding in findings)
        )

    def test_invalidation_removes_accepted_byte_facts(self) -> None:
        data, symbol_id = self.tracker()
        accept_live_byte_groups(
            data,
            mode="authored",
            groups=((symbol_id,),),
            evidence_id="recoil:evidence:r1:000001",
            facts={},
        )
        invalidate_order_dependencies(data, symbol_ids=(symbol_id,))
        self.assertNotIn("accepted_byte_facts", data["symbols"][symbol_id])


if __name__ == "__main__":
    unittest.main()
