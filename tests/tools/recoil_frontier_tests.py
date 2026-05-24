from __future__ import annotations

import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_frontier import (  # noqa: E402
    Node,
    build_frontier,
    extract_call_tokens,
    extract_function_reference_tokens,
    inferred_symbol_kind,
    is_indirect_call_token,
    recommendation,
)
from recoil_plan import PlanEntry  # noqa: E402
from recoil_binja import Symbol  # noqa: E402


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

    def test_non_call_function_pointer_immediates_are_extracted(self) -> None:
        tokens = extract_function_reference_tokens(
            "\n".join(
                [
                    "0041f5b4  68 50 f8 41 00   push    0x41f850",
                    "0041f5b9  ba a0 f6 41 00   mov     edx, 0x41f6a0",
                    "0041f5cd  e8 0e 0a 0a 00   call    zUtil_ZAR::RegisterSectionHandler",
                ]
            )
        )

        self.assertEqual(["0x41f850", "0x41f6a0"], tokens)

    def test_msvc_helper_symbol_is_provider_not_source_blocker(self) -> None:
        node = Node(address="0x4c6000", name="MSVC_EH_ArrayConstructor", kind="compiler")

        self.assertEqual("provided by compiler/runtime", node.status_summary())
        self.assertFalse(node.blocks_source())
        self.assertFalse(node.blocks_binary_verification())

    def test_msvc_helper_symbol_kind_is_inferred_from_name(self) -> None:
        symbol = Symbol(
            address="0x4c6000",
            name="MSVC_EH_ArrayConstructor",
            kind="function",
        )

        self.assertEqual("compiler", inferred_symbol_kind(symbol.address, symbol))

    def test_functional_lane_accepts_functionally_equivalent_binary_blocker(self) -> None:
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

        self.assertTrue(node.blocks_verification(lane="binary"))
        self.assertFalse(node.blocks_verification(lane="functional"))
        self.assertFalse(node.blocks_verification(lane="progress"))

    def test_functional_recommendation_skips_functionally_equivalent_callee(self) -> None:
        root = Node(
            address="0x401000",
            name="Root",
            plan=PlanEntry(
                address="0x401000",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="✅",
                binary_verified_status="✅",
                functional_equivalent_status="✅",
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

        self.assertIn("binary verification blocks", recommendation(root.address, nodes, lane="binary"))
        self.assertEqual(
            "No blocking dependency visible at requested depth.",
            recommendation(root.address, nodes),
        )

    def test_depth_leaf_nodes_do_not_fetch_assembly_or_il(self) -> None:
        class FakeBridge:
            assembly_calls: list[str] = []
            il_calls: list[str] = []

            def symbols(self):
                root = Symbol(address="0x401000", name="Root", kind="function")
                callee = Symbol(address="0x401020", name="Callee", kind="function")
                return (
                    {root.address: root, callee.address: callee},
                    {root.name: root, callee.name: callee},
                )

            def assembly(self, address: str) -> str:
                self.assembly_calls.append(address)
                return "00401000  e8 1b 00 00 00 call    Callee"

            def il(self, address: str, view: str = "mlil") -> str:
                self.il_calls.append(address)
                return ""

        bridge = FakeBridge()

        nodes = build_frontier("0x401000", 1, bridge, {})  # type: ignore[arg-type]

        self.assertIn("0x401020", nodes)
        self.assertEqual(["0x401000"], bridge.assembly_calls)
        self.assertEqual(["0x401000"], bridge.il_calls)
        self.assertEqual([], nodes["0x401020"].callees)

    def test_function_pointer_references_are_frontier_dependencies(self) -> None:
        class FakeBridge:
            def symbols(self):
                root = Symbol(address="0x401000", name="Root", kind="function")
                callback = Symbol(address="0x401080", name="Callback", kind="function")
                return (
                    {root.address: root, callback.address: callback},
                    {root.name: root, callback.name: callback},
                )

            def assembly(self, address: str) -> str:
                return "00401000  68 80 10 40 00 push    0x401080"

            def il(self, address: str, view: str = "mlil") -> str:
                return ""

        nodes = build_frontier("0x401000", 1, FakeBridge(), {})  # type: ignore[arg-type]

        self.assertEqual(["0x401080"], nodes["0x401000"].function_refs)
        self.assertIn("0x401080", nodes)


if __name__ == "__main__":
    unittest.main()
