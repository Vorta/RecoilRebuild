from __future__ import annotations

import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_frontier import Node, extract_call_tokens, is_indirect_call_token, recommendation  # noqa: E402
from recoil_plan import PlanEntry  # noqa: E402


class RecoilFrontierTests(unittest.TestCase):
    def test_indirect_vtable_call_is_not_treated_as_named_symbol(self) -> None:
        tokens = extract_call_tokens(
            "00408f69  ff 50 04        call    dword [eax+0x4]",
            "",
        )

        self.assertEqual(["dword [eax+0x4]"], tokens)
        self.assertTrue(is_indirect_call_token(tokens[0]))

    def test_direct_named_call_is_not_indirect(self) -> None:
        self.assertFalse(is_indirect_call_token("zOpt::GetWindowSection"))

    def test_register_call_is_indirect(self) -> None:
        self.assertTrue(is_indirect_call_token("eax"))

    def test_progress_lane_accepts_functionally_accepted_binary_blocker(self) -> None:
        entry = PlanEntry(
            address="0x401000",
            reconstructed_status="✅",
            source_dependencies_status="✅",
            reimplemented_status="✅",
            binary_verified_status="❌",
            functional_equivalent_status="✅",
            functional_target="sample_functional",
        )
        node = Node(address="0x401000", name="Sample", plan=entry)

        self.assertTrue(node.blocks_binary_verification(lane="strict"))
        self.assertFalse(node.blocks_binary_verification(lane="progress"))

    def test_progress_recommendation_skips_functionally_accepted_callee(self) -> None:
        root = Node(
            address="0x401000",
            name="Root",
            plan=PlanEntry(
                address="0x401000",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="✅",
                binary_verified_status="✅",
            ),
            callees=["0x401020"],
        )
        callee = Node(
            address="0x401020",
            name="Callee",
            plan=PlanEntry(
                address="0x401020",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="✅",
                binary_verified_status="❌",
                functional_equivalent_status="✅",
                functional_target="callee_functional",
            ),
        )
        nodes = {root.address: root, callee.address: callee}

        self.assertIn("binary verification blocks", recommendation(root.address, nodes))
        self.assertEqual(
            "No blocking dependency visible at requested depth.",
            recommendation(root.address, nodes, lane="progress"),
        )


if __name__ == "__main__":
    unittest.main()
