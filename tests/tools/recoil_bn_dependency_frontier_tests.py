from __future__ import annotations

import argparse
import contextlib
import io
import sys
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.lib.bn_dependency_frontier import (  # noqa: E402
    Node,
    build_frontier,
    extract_call_tokens,
    extract_function_reference_tokens,
    inferred_symbol_kind,
    is_indirect_call_token,
    load_frontier_addresses,
    load_metadata_node,
    main as frontier_main,
    node_to_dict,
    recommendation,
    functional_coverage_by_address,
    vc5_coverage_by_address,
    verification_coverage_by_address,
)
from _recoil.lib.owner_entries import OwnerEntry  # noqa: E402
from _recoil.lib.binja import Symbol  # noqa: E402


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

    def test_owner_entry_compiler_provider_does_not_block_functional_lane(self) -> None:
        entry = OwnerEntry(
            address="0x4c83db",
            reconstructed_status="✅",
            reconstructed_name="__ehhandler_Briefing_StartForMission",
            provider_boundary_status="✅",
            provider_kind="compiler-generated glue",
            provider_name="__ehhandler_Briefing_StartForMission",
            provider_origin="VC5SP3 EH",
            provider_file="external",
            provider_target="pending",
        )
        node = Node(
            address="0x4c83db",
            name="__ehhandler_Briefing_StartForMission",
            kind="function",
            entry=entry,
        )

        self.assertFalse(node.blocks_source())
        self.assertFalse(node.blocks_verification(lane="functional"))
        self.assertFalse(node.blocks_verification(lane="binary"))
        self.assertEqual("", node.verification_block_reason(lane="binary"))

    def test_msvc_helper_symbol_kind_is_inferred_from_name(self) -> None:
        symbol = Symbol(
            address="0x4c6000",
            name="MSVC_EH_ArrayConstructor",
            kind="function",
        )

        self.assertEqual("compiler", inferred_symbol_kind(symbol.address, symbol))

    def test_alloca_probe_symbol_kind_is_inferred_as_compiler_helper(self) -> None:
        symbol = Symbol(
            address="0x4c6100",
            name="__alloca_probe",
            kind="function",
        )

        self.assertEqual("compiler", inferred_symbol_kind(symbol.address, symbol))

    def test_functional_lane_accepts_functionally_equivalent_binary_blocker(self) -> None:
        entry = OwnerEntry(
            address="0x401000",
            reconstructed_status="✅",
            source_dependencies_status="✅",
            reimplemented_status="✅",
            reimplementation_tier="B",
            functional_target="sample_functional",
            source_owner_status="✅",
            data_state="✅",
        )
        node = Node(address="0x401000", name="Sample", entry=entry)

        self.assertTrue(node.blocks_verification(lane="binary"))
        self.assertFalse(node.blocks_verification(lane="functional"))

    def test_functional_recommendation_skips_functionally_equivalent_callee(self) -> None:
        root = Node(
            address="0x401000",
            name="Root",
            entry=OwnerEntry(
                address="0x401000",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="✅",
                reimplementation_tier="S",
                source_owner_status="✅",
                data_state="✅",
            ),
            callees=["0x401020"],
        )
        callee = Node(
            address="0x401020",
            name="Callee",
            entry=OwnerEntry(
                address="0x401020",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="✅",
                reimplementation_tier="B",
                functional_target="callee_functional",
                source_owner_status="✅",
                data_state="✅",
            ),
        )
        nodes = {root.address: root, callee.address: callee}

        self.assertIn("tier S blocks", recommendation(root.address, nodes, lane="binary"))
        self.assertEqual(
            "No blocking dependency visible at requested depth.",
            recommendation(root.address, nodes),
        )

    def test_binary_recommendation_prefers_owner_dependency_over_verify_dependency(self) -> None:
        root = Node(
            address="0x401000",
            name="Root",
            entry=OwnerEntry(
                address="0x401000",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="✅",
                reimplementation_tier="S",
                source_owner_status="✅",
                data_state="✅",
            ),
            callees=["0x401020", "0x401040"],
        )
        verify = Node(
            address="0x401020",
            name="VerifyOnly",
            entry=OwnerEntry(
                address="0x401020",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="✅",
                reimplementation_tier="B",
                source_owner_status="✅",
                data_state="✅",
            ),
        )
        owner = Node(
            address="0x401040",
            name="NeedsOwner",
            entry=OwnerEntry(
                address="0x401040",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="✅",
                reimplementation_tier="C",
                source_owner_status="❌",
                data_state="❌",
            ),
        )
        nodes = {root.address: root, verify.address: verify, owner.address: owner}

        self.assertIn("0x401040 NeedsOwner", recommendation(root.address, nodes, lane="binary"))

    def test_binary_source_reason_uses_owner_before_missing_implementation(self) -> None:
        node = Node(
            address="0x401040",
            name="NeedsOwner",
            entry=OwnerEntry(
                address="0x401040",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="❌",
                reimplementation_tier="X",
                source_owner_status="❌",
                source_owner="class:NeedsOwner",
                data_state="❌",
            ),
        )

        self.assertTrue(node.blocks_source(lane="binary"))
        self.assertEqual("Source owner=❌/class:NeedsOwner", node.source_block_reason(lane="binary"))
        self.assertEqual("impl=❌", node.source_block_reason(lane="functional"))
        self.assertEqual(
            "Source owner=❌/class:NeedsOwner",
            node_to_dict(node, lane="binary")["source_block_reason"],
        )

    def test_binary_recommendation_names_owner_dependency_before_missing_impl(self) -> None:
        root = Node(
            address="0x401000",
            name="Root",
            entry=OwnerEntry(
                address="0x401000",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="✅",
                reimplementation_tier="S",
                source_owner_status="✅",
                data_state="✅",
            ),
            callees=["0x401040"],
        )
        owner = Node(
            address="0x401040",
            name="NeedsOwner",
            entry=OwnerEntry(
                address="0x401040",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="❌",
                reimplementation_tier="X",
                source_owner_status="❌",
                source_owner="class:NeedsOwner",
                data_state="❌",
            ),
        )
        nodes = {root.address: root, owner.address: owner}

        self.assertIn(
            "Source owner=❌/class:NeedsOwner",
            recommendation(root.address, nodes, lane="binary"),
        )
        self.assertIn("impl=❌", recommendation(root.address, nodes, lane="functional"))

    def test_binary_recommendation_prefers_deps_source_blocker_over_owner(self) -> None:
        root = Node(
            address="0x401000",
            name="Root",
            entry=OwnerEntry(
                address="0x401000",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="✅",
                reimplementation_tier="S",
                source_owner_status="✅",
                data_state="✅",
            ),
            callees=["0x401040", "0x401020"],
        )
        owner = Node(
            address="0x401040",
            name="NeedsOwner",
            entry=OwnerEntry(
                address="0x401040",
                reconstructed_status="✅",
                source_dependencies_status="✅",
                reimplemented_status="❌",
                reimplementation_tier="X",
                source_owner_status="❌",
                source_owner="class:NeedsOwner",
                data_state="❌",
            ),
        )
        deps = Node(
            address="0x401020",
            name="NeedsDeps",
            entry=OwnerEntry(
                address="0x401020",
                reconstructed_status="✅",
                source_dependencies_status="❌",
                reimplemented_status="❌",
                reimplementation_tier="X",
                source_owner_status="❌",
                source_owner="class:NeedsDeps",
                data_state="❌",
            ),
        )
        nodes = {root.address: root, owner.address: owner, deps.address: deps}

        self.assertIn("0x401020 NeedsDeps", recommendation(root.address, nodes, lane="binary"))

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

    def test_untracked_forwarder_to_accepted_entry_does_not_block_functional_lane(self) -> None:
        target = OwnerEntry(
            address="0x401020",
            reconstructed_status="✅",
            reconstructed_name="Sample::Target",
            source_dependencies_status="✅",
            reimplemented_status="✅",
            reimplemented_name="Sample::Target",
            reimplementation_tier="B",
            data_state="✅",
        )
        symbol = Symbol(
            address="0x401000",
            name="Sample::Target_Forwarder",
            kind="function",
        )

        node = load_metadata_node(
            "0x401000",
            {target.address: target},
            {symbol.address: symbol},
            {target.reconstructed_name: target},
        )

        self.assertEqual("forwarder", node.kind)
        self.assertEqual("0x401020", node.forwarded_to)
        self.assertFalse(node.blocks_source())
        self.assertFalse(node.blocks_verification(lane="functional"))
        self.assertTrue(node.blocks_verification(lane="binary"))

    def test_vc5_coverage_skips_vanished_manifest_files(self) -> None:
        class FakeFunction:
            address = "0x401000"

        class FakeManifest:
            name = "sample"
            functions = (FakeFunction(),)

        def fake_load_manifest(path: Path, *, enforce_source_policy: bool = True):
            if path.name == "missing.json":
                raise FileNotFoundError(path)
            self.assertFalse(enforce_source_policy)
            return FakeManifest()

        with patch("pathlib.Path.glob", return_value=[Path("missing.json"), Path("sample.json")]):
            with patch("_recoil.lib.bn_dependency_frontier.load_vc5_manifest", side_effect=fake_load_manifest):
                coverage = vc5_coverage_by_address(Path("unused"))

        self.assertEqual({"0x401000": ["sample"]}, coverage)

    def test_functional_coverage_includes_manifest_address_and_covered_addresses(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            manifest_path = Path(temp_dir) / "sample_functional.json"
            manifest_path.write_text(
                json.dumps(
                    {
                        "name": "sample_functional",
                        "description": "sample functional coverage",
                        "address": "0x401000",
                        "covered_addresses": ["0x401020"],
                        "source_from": "src/sample.cpp",
                        "smoke_tests": ["sample_smoke"],
                        "known_limits": ["tier S not recorded"],
                    }
                ),
                encoding="utf-8",
            )

            coverage = functional_coverage_by_address(Path(temp_dir))

        self.assertEqual(
            {
                "0x401000": ["sample_functional"],
                "0x401020": ["sample_functional"],
            },
            coverage,
        )

    def test_verification_coverage_merges_functional_and_vc5_targets(self) -> None:
        with patch(
            "_recoil.lib.bn_dependency_frontier.functional_coverage_by_address",
            return_value={"0x401000": ["sample_functional"]},
        ), patch(
            "_recoil.lib.bn_dependency_frontier.vc5_coverage_by_address",
            return_value={"0x401000": ["sample_vc5"], "0x401020": ["callee_vc5"]},
        ):
            coverage = verification_coverage_by_address(
                Path("functional"),
                Path("vc5"),
            )

        self.assertEqual(
            {
                "0x401000": ["sample_functional", "sample_vc5"],
                "0x401020": ["callee_vc5"],
            },
            coverage,
        )

    def test_load_frontier_addresses_rejects_duplicates(self) -> None:
        args = argparse.Namespace(addresses=["0x401000", "0x401000"])

        with self.assertRaisesRegex(ValueError, "frontier received duplicate address: 0x401000"):
            load_frontier_addresses(args)

    def test_main_prints_batch_frontier_for_multiple_addresses(self) -> None:
        calls: list[str] = []

        def fake_build(root: str, depth: int, bridge: object, plan: dict[str, OwnerEntry]) -> dict[str, Node]:
            calls.append(root)
            return {root: Node(address=root, name=f"Func_{root[-2:]}")}

        stdout = io.StringIO()
        stderr = io.StringIO()
        with tempfile.TemporaryDirectory() as tmp:
            plan = Path(tmp) / "SOURCE_OWNERS.json"
            plan.write_text("# test\n", encoding="utf-8")
            with patch("_recoil.lib.bn_dependency_frontier.load_owner_entries", return_value={}), patch(
                "_recoil.lib.bn_dependency_frontier.BinaryNinjaBridge",
                return_value=object(),
            ), patch("_recoil.lib.bn_dependency_frontier.build_frontier", side_effect=fake_build), patch(
                "_recoil.lib.bn_dependency_frontier.verification_coverage_by_address",
                return_value={},
            ), contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                rc = frontier_main(["0x401000", "0x401020", "--depth", "1", "--progress", str(plan)])

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertEqual(["0x401000", "0x401020"], calls)
        output = stdout.getvalue()
        self.assertIn("# Recoil Dependency Frontier Batch", output)
        self.assertIn("Entries: 2", output)
        self.assertIn("## Entry 1/2: 0x401000", output)
        self.assertIn("## Entry 2/2: 0x401020", output)
        self.assertIn("Anchor: `0x401000` `Func_00`", output)
        self.assertIn("Anchor: `0x401020` `Func_20`", output)

    def test_main_uses_target_qualified_bridge_for_messages_binary(self) -> None:
        bridge_sentinel = object()

        def fake_build(root: str, depth: int, bridge: object, plan: dict[str, OwnerEntry]) -> dict[str, Node]:
            self.assertIs(bridge_sentinel, bridge)
            return {root: Node(address=root, name="ZLocGetID")}

        stdout = io.StringIO()
        stderr = io.StringIO()
        with tempfile.TemporaryDirectory() as tmp:
            plan = Path(tmp) / "SOURCE_OWNERS.json"
            plan.write_text("# test\n", encoding="utf-8")
            with patch("_recoil.lib.bn_dependency_frontier.load_owner_entries", return_value={}), patch(
                "_recoil.lib.bn_dependency_frontier.BinaryNinjaBridge",
                return_value=bridge_sentinel,
            ) as bridge_ctor, patch("_recoil.lib.bn_dependency_frontier.build_frontier", side_effect=fake_build), patch(
                "_recoil.lib.bn_dependency_frontier.verification_coverage_by_address",
                return_value={},
            ), contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                rc = frontier_main(["--binary", "messages", "0x10001010", "--depth", "1", "--progress", str(plan)])

        self.assertEqual(0, rc, stderr.getvalue())
        bridge_ctor.assert_called_once_with("http://127.0.0.1:9009", binary="messages.bndb")
        self.assertIn("Anchor: `0x10001010` `ZLocGetID`", stdout.getvalue())

    def test_main_rejects_explicit_missing_owner_path(self) -> None:
        stdout = io.StringIO()
        stderr = io.StringIO()
        with tempfile.TemporaryDirectory() as tmp:
            missing_owners = Path(tmp) / "SOURCE_OWNERS.json"
            with patch("_recoil.lib.bn_dependency_frontier.load_owner_entries") as load_owner_entries_mock, contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                rc = frontier_main(["0x401000", "--progress", str(missing_owners)])

        self.assertEqual(2, rc)
        load_owner_entries_mock.assert_not_called()
        self.assertEqual("", stdout.getvalue())
        self.assertIn(f"Owners file not found: {missing_owners}", stderr.getvalue())


if __name__ == "__main__":
    unittest.main()
