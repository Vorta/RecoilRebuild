from __future__ import annotations

from copy import deepcopy
from pathlib import Path
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.call_contract_verify import build_identity_indexes
from _recoil.lib.progress import (
    AUTHORED_ORDER_DIMENSIONS,
    ProgressDocument,
    empty_progress_document,
    state_record,
)


CALLER_ID = "recoil:function:0x401000"
CALLER_OBJECT_SYMBOL = "?Caller@@YAXXZ"
DATA_ID = "recoil:data:0x4cc738"
DATA_ADDRESS = "0x4cc738"
DATA_END = "0x4cc810"
DATA_NAME = "g_CAboutDlg_Vtbl"
DATA_OBJECT_SYMBOL = "??_7CAboutDlg@@6B@"


def source_binding() -> dict:
    return {
        "symbol_id": CALLER_ID,
        "address": "0x401000",
        "end_exclusive": "0x401020",
        "object_symbol": CALLER_OBJECT_SYMBOL,
        "physical_pipeline_class": "authored",
        "object_pipeline_class": "authored",
        "registration_ids": ["unit:registration:caller"],
        "evidence_ids": [],
    }


def target_binding(
    *,
    symbol_id: str,
    address: str,
    end_exclusive: str,
    kind: str,
    navigation_name: str,
    object_symbol: str,
    output_section_id: str,
    pipeline_class: str | None,
    ownership_state: str | None,
    provider: bool = False,
) -> dict:
    evidence_id = "unit:evidence:target-binding"
    relationship = (
        None
        if provider
        else {
            "kind": "primary-data",
            "address": address,
            "symbol_id": symbol_id,
            "name": navigation_name,
        }
    )
    return {
        "reviewed": True,
        "object_symbol": object_symbol,
        "reason": "unit-test reviewed existing target identity",
        "evidence_ids": [evidence_id],
        "binding_context": {
            "source_binding": source_binding(),
            "relocation": {
                "offset": 1,
                "type": 0x0006 if kind not in {"function", "provider-function"} else 0x0014,
                "type_name": (
                    "DIR32"
                    if kind not in {"function", "provider-function"}
                    else "REL32"
                ),
                "retail_target": int(address, 16),
                "instruction_offset": 0,
                "opcode": "c7" if kind not in {"function", "provider-function"} else "e8",
            },
            "target": {
                "symbol_id": symbol_id,
                "address": address,
                "end_exclusive": end_exclusive,
                "kind": kind,
                "navigation_name": navigation_name,
                "object_symbol": object_symbol,
                "output_section_id": output_section_id,
                "pipeline_class": pipeline_class,
                "ownership_state": ownership_state,
            },
            "owner": {
                "owner_id": (
                    "recoil:owner:provider.unit"
                    if provider
                    else "recoil:owner:misc_unresolved.cabout_dlg"
                ),
                "kind": "provider-boundary" if provider else "class",
                "provider_state": "accepted" if provider else "pending",
                "lifecycle_state": "accepted" if provider else "blocked",
                "binding_evidence_ids": [evidence_id],
            },
            "relationship": relationship,
            "creation_mode": "existing-symbol",
        },
    }


def data_document() -> dict:
    data = empty_progress_document()
    data["symbols"][CALLER_ID] = {
        "binary": "recoil",
        "kind": "function",
        "address": "0x401000",
        "start": "0x401000",
        "end_exclusive": "0x401020",
        "pipeline_class": "authored",
    }
    data["symbols"][DATA_ID] = {
        "binary": "recoil",
        "kind": "data",
        "address": DATA_ADDRESS,
        "end_exclusive": DATA_END,
        "navigation_name": DATA_NAME,
        "output_section_id": "recoil:section:.rdata",
        "pipeline_class": None,
        "ownership_state": "primary-owned",
        "relocation_target_binding": target_binding(
            symbol_id=DATA_ID,
            address=DATA_ADDRESS,
            end_exclusive=DATA_END,
            kind="data",
            navigation_name=DATA_NAME,
            object_symbol=DATA_OBJECT_SYMBOL,
            output_section_id="recoil:section:.rdata",
            pipeline_class=None,
            ownership_state="primary-owned",
        ),
    }
    return data


class CallContractExistingTargetBindingTests(unittest.TestCase):
    def test_reviewed_existing_data_binding_indexes_unique_storage_name(self) -> None:
        indexes = build_identity_indexes(ProgressDocument(data_document()))

        identity = f"storage:{DATA_ID}"
        self.assertEqual(identity, indexes.storage_by_address[DATA_ADDRESS])
        self.assertEqual(identity, indexes.storage_by_name[DATA_OBJECT_SYMBOL])
        self.assertNotIn(DATA_OBJECT_SYMBOL, indexes.by_candidate_name)

    def test_reviewed_existing_function_binding_keeps_callable_indexing(self) -> None:
        data = empty_progress_document()
        function_id = "recoil:function:0x4c5b64"
        object_symbol = "??0CDialog@@QAE@IPAVCWnd@@@Z"
        data["symbols"][function_id] = {
            "binary": "recoil",
            "kind": "function",
            "address": "0x4c5b64",
            "start": "0x4c5b64",
            "end_exclusive": "0x4c5b6a",
            "pipeline_class": "non-authored",
            "relocation_target_binding": target_binding(
                symbol_id=function_id,
                address="0x4c5b64",
                end_exclusive="0x4c5b6a",
                kind="function",
                navigation_name=object_symbol,
                object_symbol=object_symbol,
                output_section_id="recoil:section:.text",
                pipeline_class="unresolved",
                ownership_state="unresolved",
                provider=True,
            ),
        }

        indexes = build_identity_indexes(ProgressDocument(data))

        self.assertEqual(
            f"provider:{function_id}",
            indexes.by_candidate_name[object_symbol],
        )
        self.assertNotIn(object_symbol, indexes.storage_by_name)

    def test_reviewed_existing_data_binding_rejects_coaddressed_storage(self) -> None:
        data = data_document()
        data["symbols"]["recoil:data:0x4cc738:alias"] = {
            "binary": "recoil",
            "kind": "data",
            "address": DATA_ADDRESS,
            "end_exclusive": DATA_END,
            "navigation_name": "g_Coaddressed",
            "output_section_id": "recoil:section:.rdata",
        }

        with self.assertRaisesRegex(
            ValueError,
            "missing, ambiguous, or colliding storage identity",
        ):
            build_identity_indexes(ProgressDocument(data))

    def test_reviewed_existing_data_binding_rejects_object_name_collision(self) -> None:
        data = data_document()
        data["symbols"]["recoil:data:0x4cc900"] = {
            "binary": "recoil",
            "kind": "data",
            "address": "0x4cc900",
            "end_exclusive": "0x4cc904",
            "navigation_name": DATA_OBJECT_SYMBOL,
            "output_section_id": "recoil:section:.rdata",
        }

        with self.assertRaisesRegex(
            ValueError,
            "conflicting storage identity",
        ):
            build_identity_indexes(ProgressDocument(data))

    def test_reviewed_existing_data_binding_rejects_provider_data(self) -> None:
        data = data_document()
        row = data["symbols"][DATA_ID]
        row["kind"] = "provider-data"
        row["relocation_target_binding"] = target_binding(
            symbol_id=DATA_ID,
            address=DATA_ADDRESS,
            end_exclusive=DATA_END,
            kind="provider-data",
            navigation_name=DATA_NAME,
            object_symbol=DATA_OBJECT_SYMBOL,
            output_section_id="recoil:section:.rdata",
            pipeline_class="non-authored",
            ownership_state="primary-owned",
            provider=True,
        )

        with self.assertRaisesRegex(ValueError, "unsupported target kind 'provider-data'"):
            build_identity_indexes(ProgressDocument(data))

    def test_reviewed_existing_data_binding_rejects_import_metadata(self) -> None:
        data = data_document()
        data["symbols"][DATA_ID]["import_name"] = "CAboutDlg"

        with self.assertRaisesRegex(ValueError, "provider/import data identity"):
            build_identity_indexes(ProgressDocument(data))

    def test_reviewed_existing_data_binding_rejects_unknown_kind(self) -> None:
        data = data_document()
        data["symbols"][DATA_ID]["kind"] = "mystery"

        with self.assertRaisesRegex(ValueError, "unsupported target kind 'mystery'"):
            build_identity_indexes(ProgressDocument(data))

    def test_reviewed_existing_data_binding_rejects_stale_target_snapshot(self) -> None:
        data = data_document()
        stale = deepcopy(data["symbols"][DATA_ID]["relocation_target_binding"])
        data["symbols"][DATA_ID]["navigation_name"] = "g_RenamedAfterReview"
        data["symbols"][DATA_ID]["relocation_target_binding"] = stale

        with self.assertRaisesRegex(ValueError, "stale data target snapshot"):
            build_identity_indexes(ProgressDocument(data))


class CallContractSliceSourceAuthorityTests(unittest.TestCase):
    @staticmethod
    def document(
        *,
        order_edit_paths: list[str],
        source_from: str | None,
        agent_source_path: str,
        source_path: str,
    ) -> ProgressDocument:
        data = empty_progress_document()
        data["binaries"] = {
            "recoil": {
                "text": {
                    "start": "0x401000",
                    "end_exclusive": "0x401010",
                }
            }
        }
        block_id = "recoil:block:0x401000"
        symbol_id = "recoil:function:0x401000"
        target_id = "recoil:vc5-target:unit"
        accepted = state_record(
            "passed",
            "accepted",
            "current",
            [],
            validation_mode="live",
        )
        data["physical_blocks"] = {
            block_id: {
                "binary": "recoil",
                "start": "0x401000",
                "end_exclusive": "0x401010",
                "agent_source_path": agent_source_path,
                "source_path": source_path,
                "order": {
                    "authored": {
                        dimension: deepcopy(accepted)
                        for dimension in AUTHORED_ORDER_DIMENSIONS
                    }
                },
                "accepted_order_facts": {
                    "phase": "authored-function-order",
                    "target_id": target_id,
                    "matched_identities": [symbol_id],
                },
            }
        }
        data["symbols"] = {
            symbol_id: {
                "binary": "recoil",
                "kind": "function",
                "address": "0x401000",
                "end_exclusive": "0x401010",
                "pipeline_class": "authored",
                "authored_order_role": "authored-body",
                "physical_block_id": block_id,
                "binary_state": {},
            }
        }
        registration: dict[str, object] = {
            "function_addresses": ["0x401000"],
            "order_edit_paths": order_edit_paths,
        }
        if source_from is not None:
            registration["source_from"] = source_from
        data["verification_targets"] = {
            target_id: {
                "name": "unit",
                "registered_addresses": ["0x401000"],
                "registration": registration,
            }
        }
        return ProgressDocument(data)

    def test_exact_target_source_paths_override_semantic_block_metadata(self) -> None:
        document = self.document(
            order_edit_paths=["src/Unit/Main.cpp", "src/Unit/Main.h"],
            source_from="src/Unit/Main.cpp",
            agent_source_path="src/Legacy/AppFrame.cpp",
            source_path="semantic:late-appframe-cluster",
        )
        self.assertEqual(
            ["src/Unit/Main.cpp", "src/Unit/Main.h"],
            document.authored_call_contract_slices()[0]["source_paths"],
        )

    def test_player_target_initializer_source_propagates_to_slice(self) -> None:
        initializer_source = "src/GameZRecoil/zInput/zin_init.cpp"
        document = self.document(
            order_edit_paths=[
                "src/Battlesport/player.cpp",
                "src/GameZRecoil/zInput/zInput.cpp",
                initializer_source,
            ],
            source_from="src/Battlesport/player.cpp",
            agent_source_path="src/Battlesport/player.cpp",
            source_path="src/Battlesport/player.cpp",
        )

        source_paths = document.authored_call_contract_slices()[0]["source_paths"]
        self.assertEqual(1, source_paths.count(initializer_source))

    def test_exact_target_casing_wins_case_insensitively(self) -> None:
        document = self.document(
            order_edit_paths=["src/Game/zInput.cpp"],
            source_from="src/Game/zInput.cpp",
            agent_source_path="src/Game/zinput.cpp",
            source_path="src/Game/zinput.cpp",
        )
        self.assertEqual(
            ["src/Game/zInput.cpp"],
            document.authored_call_contract_slices()[0]["source_paths"],
        )

    def test_missing_exact_implementation_root_preserves_fail_closed_fallback(self) -> None:
        document = self.document(
            order_edit_paths=["src/Unit/Main.h"],
            source_from=None,
            agent_source_path="semantic:unknown-source-host",
            source_path="semantic:unknown-source-host",
        )
        self.assertEqual(
            ["semantic:unknown-source-host", "src/Unit/Main.h"],
            document.authored_call_contract_slices()[0]["source_paths"],
        )


if __name__ == "__main__":
    unittest.main()
