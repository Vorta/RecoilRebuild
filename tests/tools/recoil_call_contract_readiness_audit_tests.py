from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace
import sys
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.call_contract_readiness_audit import (  # noqa: E402
    audit_call_contract_readiness,
    build_parser,
)
from _recoil.commands.call_contract_verify import (  # noqa: E402
    CallContractSourceClosure,
    call_contract_registration_path_reconciliation,
)
from _recoil.lib.repository_paths import (  # noqa: E402
    load_repository_path_inventory,
)


SLICE_ID = "recoil:call-contract-slice:0x401000-0x401010"


class FixtureDocument:
    def authored_call_contract_slices(self, binary: str):
        assert binary == "recoil"
        return [
            {
                "id": SLICE_ID,
                "ordinal": 1,
                "body_count": 1,
                "target_ids": ["recoil:vc5-target:unit"],
                "source_paths": ["tools/recoil.py"],
            }
        ]

    def collection(self, name: str):
        assert name == "verification_targets"
        return {
            "recoil:vc5-target:unit": {
                "kind": "vc5",
                "registration": {
                    "manifest_path": "tools/vc5_verify_targets/unit.json",
                    "order_edit_paths": [
                        "src/GameZRecoil/zUI/zui_widgets.cpp"
                    ],
                },
            }
        }


class CallContractReadinessAuditTests(unittest.TestCase):
    @staticmethod
    def target(*paths: str):
        return SimpleNamespace(
            order_edit_paths=paths,
            source_from="",
            source_files=(),
            translation_unit_function_order=(),
        )

    def test_all_original_slices_require_exact_dependency_closure(self) -> None:
        closure = CallContractSourceClosure(
            source_edit_paths=("tools/recoil.py",),
            registered_source_paths=("tools/recoil.py",),
            header_paths=(),
            definition_source_paths=(),
            dependency_paths=("tools/recoil.py",),
        )
        with (
            patch(
                "_recoil.commands.call_contract_readiness_audit._call_contract_slice_targets",
                return_value={
                    "recoil:vc5-target:unit": self.target(
                        "src/GameZRecoil/zUI/zui_widgets.cpp"
                    )
                },
            ),
            patch(
                "_recoil.commands.call_contract_readiness_audit.call_contract_source_closure",
                return_value=closure,
            ),
        ):
            report = audit_call_contract_readiness(
                all_slices=True,
                document=FixtureDocument(),
            )

        self.assertTrue(report["passed"], report["blockers"])
        self.assertTrue(report["candidate_independent_slice_membership"])
        self.assertEqual("accepted-target-registration", report["source_authority"])
        self.assertEqual(1, report["original_slice_count"])
        self.assertEqual(1, report["ready_slice_count"])
        self.assertEqual([], report["blockers"])

    def test_missing_exact_implementation_root_fails_closed(self) -> None:
        closure = CallContractSourceClosure(
            source_edit_paths=(),
            registered_source_paths=(),
            header_paths=(),
            definition_source_paths=(),
            dependency_paths=(),
        )
        with (
            patch(
                "_recoil.commands.call_contract_readiness_audit._call_contract_slice_targets",
                return_value={
                    "recoil:vc5-target:unit": self.target(
                        "src/GameZRecoil/zUI/zui_widgets.cpp"
                    )
                },
            ),
            patch(
                "_recoil.commands.call_contract_readiness_audit.call_contract_source_closure",
                return_value=closure,
            ),
        ):
            report = audit_call_contract_readiness(
                all_slices=True,
                document=FixtureDocument(),
            )

        self.assertFalse(report["passed"])
        self.assertEqual(1, report["blocked_slice_count"])
        self.assertIn(
            "no implementation roots", report["blockers"][0]["message"]
        )

    def test_unknown_original_slice_fails_closed(self) -> None:
        report = audit_call_contract_readiness(
            slice_id="recoil:call-contract-slice:unknown",
            document=FixtureDocument(),
        )
        self.assertFalse(report["passed"])
        self.assertEqual(0, report["selected_slice_count"])
        self.assertEqual("slice-selection", report["blockers"][0]["kind"])

    def test_cli_requires_one_selection_and_accepts_strict_json(self) -> None:
        args = build_parser().parse_args(
            ["--all-slices", "--strict", "--json"]
        )
        self.assertTrue(args.all_slices)
        self.assertTrue(args.strict)
        self.assertTrue(args.json)

    def test_historical_wrong_case_is_diagnostic_only(self) -> None:
        inventory = load_repository_path_inventory(REPO_ROOT)
        result = call_contract_registration_path_reconciliation(
            target_id="recoil:vc5-target:unit",
            registration={
                "order_edit_paths": ["src/GameZRecoil/zSys/zSys.cpp"]
            },
            current_target=self.target("src/GameZRecoil/zSys/zsys.cpp"),
            inventory=inventory,
        )

        self.assertEqual("historical-case-alias", result["status"])
        self.assertTrue(result["case_only"])
        self.assertFalse(result["current"])
        self.assertFalse(result["tracker_mutated"])
        self.assertEqual(
            [
                {
                    "status": "historical-case-alias",
                    "historical_path": "src/GameZRecoil/zSys/zSys.cpp",
                    "current_repository_path": "src/GameZRecoil/zSys/zsys.cpp",
                    "current": False,
                    "tracker_mutated": False,
                }
            ],
            result["historical_case_aliases"],
        )

    def test_absent_order_edit_projection_compares_source_from_like_for_like(
        self,
    ) -> None:
        inventory = load_repository_path_inventory(REPO_ROOT)
        current_target = self.target(
            "src/Battlesport/Briefing.cpp",
            "src/Battlesport/briefing.h",
        )
        current_target.source_from = "src/Battlesport/Briefing.cpp"

        result = call_contract_registration_path_reconciliation(
            target_id="recoil:vc5-target:briefing",
            registration={"source_from": "src/Battlesport/Briefing.cpp"},
            current_target=current_target,
            inventory=inventory,
        )

        self.assertEqual("current", result["status"])
        self.assertIsNone(result["blocker_kind"])
        self.assertEqual(
            "source_from-and-translation-unit-sources",
            result["registration_projection_field"],
        )
        self.assertEqual(
            ["src/Battlesport/Briefing.cpp"], result["current_manifest"]
        )
        self.assertEqual(
            ["src/Battlesport/Briefing.cpp"], result["unchanged"]
        )

    def test_zui_registration_drift_has_exact_field_projection(self) -> None:
        inventory = load_repository_path_inventory(REPO_ROOT)
        result = call_contract_registration_path_reconciliation(
            target_id="recoil:vc5-target:zui_438920_438980_authored_order",
            registration={
                "order_edit_paths": [
                    "src/GameZRecoil/zUI/zui.cpp",
                    "src/GameZRecoil/zHud/zhud_ui.h",
                ]
            },
            current_target=self.target(
                "src/GameZRecoil/zUI/zui_widgets.cpp",
                "src/GameZRecoil/zHud/zhud_ui.h",
            ),
            inventory=inventory,
        )

        self.assertEqual("manifest-registration-drift", result["status"])
        self.assertEqual("manifest-registration-drift", result["blocker_kind"])
        self.assertEqual(
            ["src/GameZRecoil/zUI/zui.cpp"], result["removed"]
        )
        self.assertEqual(
            ["src/GameZRecoil/zUI/zui_widgets.cpp"], result["added"]
        )
        self.assertEqual(
            ["src/GameZRecoil/zHud/zhud_ui.h"], result["unchanged"]
        )
        self.assertFalse(result["case_only"])
        self.assertFalse(result["tracker_mutated"])

    def test_typed_registration_drift_is_operational_not_ready(self) -> None:
        document = FixtureDocument()
        document.collection = lambda name: {
            "recoil:vc5-target:unit": {
                "kind": "vc5",
                "registration": {
                    "manifest_path": "tools/vc5_verify_targets/unit.json",
                    "order_edit_paths": ["src/GameZRecoil/zUI/zui.cpp"],
                },
            }
        }
        with (
            patch(
                "_recoil.commands.call_contract_readiness_audit._call_contract_slice_targets",
                return_value={
                    "recoil:vc5-target:unit": self.target(
                        "src/GameZRecoil/zUI/zui_widgets.cpp"
                    )
                },
            ),
            patch(
                "_recoil.commands.call_contract_readiness_audit.call_contract_source_closure"
            ) as closure,
        ):
            report = audit_call_contract_readiness(
                all_slices=True,
                document=document,
            )

        closure.assert_not_called()
        self.assertTrue(report["producer_operational"])
        self.assertTrue(report["passed"])
        self.assertEqual([], report["infrastructure_findings"])
        self.assertFalse(report["candidate_readiness"])
        self.assertEqual(
            "manifest-registration-drift",
            report["stale_registration_blockers"][0]["kind"],
        )


if __name__ == "__main__":
    unittest.main()
