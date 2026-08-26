from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.no_source_shape_scaffolds import (  # noqa: E402
    find_occurrences_in_file,
    owner_claims_for_locations,
)
from tests.tools.owner_fixture import owner_record, write_ledger  # noqa: E402


class RecoilSourceShapeGuardTests(unittest.TestCase):
    def test_guard_flags_dispatch_view_and_slot_helper_scaffolds(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "src" / "Hud.cpp"
            source.parent.mkdir()
            source.write_text(
                "struct HudDispatchView { DWORD slots[4]; };\n"
                "static DWORD HudRawSlots[4];\n"
                "void Click(void) { InvokeHudSlot(); }\n",
                encoding="utf-8",
            )

            locations = find_occurrences_in_file(source, root, root)

        labels = {label for _rel, _line_no, label, _line in locations}
        self.assertIn("dispatch-view owner-shape scaffold", labels)
        self.assertIn("raw table storage scaffold", labels)
        self.assertIn("slot-dispatch helper scaffold", labels)

    def test_owner_claims_report_accepted_entries_in_scaffolded_files(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            owners = root / "SOURCE_OWNERS.json"
            write_ledger(
                owners,
                owner_record(
                    "hud",
                    kind="class",
                    anchors=("0x401000",),
                    functions=("0x401000",),
                    tiers={"0x401000": "B"},
                    gates={
                        "boundary": "accepted",
                        "source": "accepted",
                        "data": "none",
                        "functional": "accepted",
                        "linkage": "accepted",
                        "byte": "pending",
                    },
                    address_metadata={
                        "0x401000": {
                            "name": "Hud::Click",
                            "source_path": "src/Hud.cpp",
                            "target": "hud_click",
                        }
                    },
                ),
            )

            claims = owner_claims_for_locations(
                [("src/Hud.cpp", 1, "dispatch-view owner-shape scaffold", "struct HudDispatchView")],
                owners_path=owners,
            )

        self.assertEqual(1, len(claims))
        self.assertEqual("0x401000", claims[0]["address"])
        self.assertEqual("B", claims[0]["tier"])


if __name__ == "__main__":
    unittest.main()
