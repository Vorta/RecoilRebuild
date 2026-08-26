from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
import sys

if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.provider_function_mutation import (  # noqa: E402
    ATLIMPL_CLUSTER_LEGACY_OWNER_ID,
    ATLIMPL_CLUSTER_LEGACY_OWNER_SNAPSHOT,
    ATLIMPL_CLUSTER_MEMBERS,
    ATLIMPL_CLUSTER_OWNER_ID,
    ATLIMPL_CLUSTER_OWNER_NAME,
    ATLIMPL_CLUSTER_RECIPE_ID,
    ProviderFunctionMutationError,
    _provider_header_comdat_proof,
    _provider_object_proof,
    normalize_atlimpl_cluster_request,
    normalize_provider_function_request,
    parse_archive_members,
    register_atlimpl_provider_cluster,
    register_provider_function,
)
from _recoil.commands.call_contract_verify import build_identity_indexes  # noqa: E402
from _recoil.lib.pe import parse_pe_headers, rva_to_offset  # noqa: E402
from _recoil.lib.progress import ProgressStore  # noqa: E402
from _recoil.lib.worktree_control import resolve_canonical_control_root  # noqa: E402


def canonical_retail_reference() -> Path:
    resolution = resolve_canonical_control_root(
        executing_worktree_root=REPO_ROOT,
        required_machine_local_paths=("support/Recoil.exe",),
    )
    return resolution.canonical_control_root / "support" / "Recoil.exe"


def canonical_progress_path() -> Path:
    resolution = resolve_canonical_control_root(
        executing_worktree_root=REPO_ROOT,
        required_machine_local_paths=(
            ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
        ),
    )
    return (
        resolution.canonical_control_root
        / ".agent"
        / "RECONSTRUCTION_PROGRESS.sqlite3"
    )


VC5_ROOT = REPO_ROOT.parent / "Compiler" / "VC5SP3"
ATEXIT_ADDRESS = "0x4c60e0"
CONSTRUCT_ADDRESS = "0x40c1c0"
REQUEST = {
    "reviewed": True,
    "library_path": "VC/LIB/MSVCRT.LIB",
    "archive_member": "build/intel/dll_obj/atonexit.obj",
    "object_symbol": "_atexit",
    "owner_id": "recoil:owner:provider.vc5_crt.atexit",
    "owner_name": "VC5 CRT atexit provider",
    "reason": "Exact canonical VC5 archive member and immutable retail body proof.",
}
ATEXIT_OBJECT_BYTES = bytes.fromhex(
    "8b44240450e80000000083c404f7d81bc0f7d848c3"
    "9090909090909090909090"
)
HEADER_REQUEST = {
    "reviewed": True,
    "proof_mode": "canonical-header-comdat",
    "canonical_header": "VC/INCLUDE/xmemory",
    "probe_recipe": "vc5-xmemory-construct-int-v1",
    "object_symbol": "?_Construct@std@@YIXPAHABH@Z",
    "semantic_provider": "vc5-stl",
    "physical_emitter_state": "winner-unknown",
    "retail_icf_winner_status": "winner-unknown",
    "retail_icf_logical_symbols": ["?_Construct@std@@YIXPAHABH@Z"],
    "owner_id": "recoil:owner:provider.vc5_stl.construct_int",
    "owner_name": "VC5 STL std::_Construct<int, int> provider",
    "reason": "Exact canonical VC5 xmemory probe and immutable retail body proof.",
}
CONSTRUCT_OBJECT_BYTES = bytes.fromhex(
    "56518bf26a04e80000000083c40885c074048b0e89085ec3"
)
CONSTRUCT_OB1_OBJECT_BYTES = bytes.fromhex(
    "85c974048b028901c390909090909090"
)
VECTOR_UFILL_OBJECT_BYTES = bytes.fromhex(
    "8b4c240885c976328b54240c8b44240457565385c0741a"
    "8bf28bf88b1e891f8b5e04895f048b5e08895f088b760c"
    "89770c83c0104975dc5b5e5fc20c00909090"
)
VECTOR_UCOPY_OBJECT_BYTES = bytes.fromhex(
    "8b4c24048b5424083bca74358b44240c57565385c0741a"
    "8bf18bf88b1e891f8b5e04895f048b5e08895f088b760c"
    "89770c83c11083c0103bca75d85b5e5fc20c008b44240c"
    "c20c009090909090909090"
)
VECTOR_INT_UFILL_OBJECT_BYTES = bytes.fromhex(
    "8b4c240885c976188b54240c8b4424045685c074048b328930"
    "83c0044975f25ec20c0090909090909090909090909090"
)
ATLIMPL_CLUSTER_REQUEST = {
    "schema": "recoil-vc5-atlimpl-provider-cluster-v1",
    "reviewed": True,
    "recipe_id": ATLIMPL_CLUSTER_RECIPE_ID,
    "retired_owner_id": ATLIMPL_CLUSTER_LEGACY_OWNER_ID,
    "owner_id": ATLIMPL_CLUSTER_OWNER_ID,
    "owner_name": ATLIMPL_CLUSTER_OWNER_NAME,
    "members": [
        {
            "address": member["address"],
            "symbol_id": member["symbol_id"],
            "canonical_function": member["canonical_function"],
        }
        for member in ATLIMPL_CLUSTER_MEMBERS
    ],
    "original_translation_unit": None,
    "retail_coff_symbols": None,
    "reason": (
        "Reviewed canonical VC5SP3 ATLIMPL source proof for the exact provider cluster."
    ),
}


def _header_request(
    *, recipe: str, header: str, symbol: str, owner_suffix: str
) -> dict[str, object]:
    return {
        "reviewed": True,
        "proof_mode": "canonical-header-comdat",
        "canonical_header": header,
        "probe_recipe": recipe,
        "object_symbol": symbol,
        "semantic_provider": "vc5-stl",
        "physical_emitter_state": "winner-unknown",
        "retail_icf_winner_status": "winner-unknown",
        "retail_icf_logical_symbols": [symbol],
        "owner_id": f"recoil:owner:provider.vc5_stl.{owner_suffix}",
        "owner_name": f"VC5 STL {owner_suffix} provider",
        "reason": "Exact canonical VC5 header probe and immutable retail body proof.",
    }


CONSTRUCT_OB1_REQUEST = _header_request(
    recipe="vc5-xmemory-construct-int-ob1-v1",
    header="VC/INCLUDE/xmemory",
    symbol="?_Construct@std@@YIXPAHABH@Z",
    owner_suffix="construct_int_ob1",
)
VECTOR_UFILL_REQUEST = _header_request(
    recipe="vc5-vector-player-node-restore-ufill-ob1-v1",
    header="VC/INCLUDE/vector",
    symbol=(
        "?_Ufill@?$vector@UPlayerNodeFlagRestoreEntry@@"
        "V?$allocator@UPlayerNodeFlagRestoreEntry@@@std@@@std@@"
        "IAEXPAUPlayerNodeFlagRestoreEntry@@IABU3@@Z"
    ),
    owner_suffix="player_node_restore_ufill",
)
VECTOR_UCOPY_REQUEST = _header_request(
    recipe="vc5-vector-player-node-restore-ucopy-ob1-v1",
    header="VC/INCLUDE/vector",
    symbol=(
        "?_Ucopy@?$vector@UPlayerNodeFlagRestoreEntry@@"
        "V?$allocator@UPlayerNodeFlagRestoreEntry@@@std@@@std@@"
        "IAEPAUPlayerNodeFlagRestoreEntry@@PBU3@0PAU3@@Z"
    ),
    owner_suffix="player_node_restore_ucopy",
)
VECTOR_INT_UFILL_REQUEST = _header_request(
    recipe="vc5-vector-int-ufill-ob1-v1",
    header="VC/INCLUDE/vector",
    symbol="?_Ufill@?$vector@HV?$allocator@H@std@@@std@@IAEXPAHIABH@Z",
    owner_suffix="vector_int_ufill",
)

CURRENT_HEADER_COMDAT_CASES = (
    (CONSTRUCT_OB1_REQUEST, CONSTRUCT_OB1_OBJECT_BYTES),
    (VECTOR_UFILL_REQUEST, VECTOR_UFILL_OBJECT_BYTES),
    (VECTOR_UCOPY_REQUEST, VECTOR_UCOPY_OBJECT_BYTES),
    (VECTOR_INT_UFILL_REQUEST, VECTOR_INT_UFILL_OBJECT_BYTES),
)


def _tracker_fixture(path: Path) -> int:
    data = ProgressStore(canonical_progress_path()).load().data
    # This archive-member fixture predates the separately reviewed canonical-header
    # provider identities. Keep its call-contract index assertion isolated from those
    # v2 rows; their indexing contract is exercised by their own governed route.
    for existing in data["symbols"].values():
        identity = existing.get("provider_object_identity")
        if isinstance(identity, dict) and identity.get("schema") == (
            "recoil-provider-function-object-v2"
        ):
            existing.pop("provider_object_identity", None)
    function_id = "recoil:function:0x4c60e0"
    function = deepcopy(data["symbols"][function_id])
    function.update(
        {
            "kind": "function",
            "disposition": "unresolved",
            "ownership_state": "unresolved",
            "pipeline_class": "non-authored",
            "authored_order_role": "non-authored",
        }
    )
    for field in (
        "import_dll",
        "import_name",
        "import_ordinal",
        "object_symbol",
        "provider_object_identity",
        "relocation_target_binding",
        "logical_aliases",
    ):
        function.pop(field, None)
    data["symbols"][function_id] = function
    data["owners"].pop(REQUEST["owner_id"], None)
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return int(data["revision"])


def _header_tracker_fixture(path: Path) -> int:
    data = ProgressStore(canonical_progress_path()).load().data
    function_id = f"recoil:function:{ATEXIT_ADDRESS}"
    function = deepcopy(data["symbols"][function_id])
    function.update(
        {
            "address": ATEXIT_ADDRESS,
            "end_exclusive": "0x4c60f8",
            "extent_state": "known",
            "size": len(CONSTRUCT_OBJECT_BYTES),
            "kind": "function",
            "disposition": "unresolved",
            "ownership_state": "unresolved",
            "pipeline_class": "non-authored",
            "authored_order_role": "non-authored",
        }
    )
    for field in (
        "object_symbol",
        "provider_object_identity",
        "relocation_target_binding",
        "logical_aliases",
        "source_traceability",
    ):
        function.pop(field, None)
    data["symbols"][function_id] = function
    data["owners"].pop(REQUEST["owner_id"], None)
    data["owners"].pop(HEADER_REQUEST["owner_id"], None)
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return int(data["revision"])


def _header_reference_fixture(path: Path) -> None:
    reference = canonical_retail_reference()
    image = bytearray(reference.read_bytes())
    headers = parse_pe_headers(bytes(image), source=str(reference))
    offset = rva_to_offset(int(ATEXIT_ADDRESS, 16) - headers.image_base, headers.sections)
    if offset is None:
        raise AssertionError("fixture address is not file-backed")
    image[offset : offset + len(CONSTRUCT_OBJECT_BYTES)] = CONSTRUCT_OBJECT_BYTES
    path.write_bytes(image)


def _atlimpl_cluster_tracker_fixture(path: Path) -> int:
    data = ProgressStore(canonical_progress_path()).load().data
    for member in ATLIMPL_CLUSTER_MEMBERS:
        function = data["symbols"][member["symbol_id"]]
        function.update(
            {
                "authored_order_role": "non-authored",
                "disposition": "unresolved",
                "kind": "function",
                "ownership_state": "primary-owned",
                "pipeline_class": "non-authored",
                "source_traceability": {
                    "reason_code": "detached-or-unsupported-source-topology",
                    "source_edges": [],
                    "state": "unresolved",
                },
            }
        )
        for field in (
            "import_dll",
            "import_name",
            "import_ordinal",
            "logical_aliases",
            "object_symbol",
            "provider_object_identity",
            "relocation_target_binding",
        ):
            function.pop(field, None)
    data["owners"][ATLIMPL_CLUSTER_LEGACY_OWNER_ID] = deepcopy(
        ATLIMPL_CLUSTER_LEGACY_OWNER_SNAPSHOT
    )
    data["owners"].pop(ATLIMPL_CLUSTER_OWNER_ID, None)
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return int(data["revision"])


def _mutated_reference(path: Path, *, address: str, offset: int, value: int) -> None:
    reference = canonical_retail_reference()
    image = bytearray(reference.read_bytes())
    headers = parse_pe_headers(bytes(image), source=str(reference))
    file_offset = rva_to_offset(int(address, 16) - headers.image_base, headers.sections)
    if file_offset is None:
        raise AssertionError("fixture address is not file-backed")
    image[file_offset + offset] = value
    path.write_bytes(image)


class ProviderFunctionMutationTests(unittest.TestCase):
    def test_atlimpl_cluster_dry_run_proves_complete_fixed_recipe_without_mutation(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            revision = _atlimpl_cluster_tracker_fixture(tracker)
            before = tracker.read_bytes()
            report = register_atlimpl_provider_cluster(
                progress=tracker,
                reference=canonical_retail_reference(),
                payload=ATLIMPL_CLUSTER_REQUEST,
                expected_revision=revision,
                apply=False,
                vc5_root=VC5_ROOT,
            )
            self.assertFalse(report["commit"]["applied"])
            self.assertEqual(before, tracker.read_bytes())
            self.assertEqual(
                "immutable-retail-plus-fixed-canonical-vc5-atlimpl-source",
                report["validation_mode"],
            )
            self.assertEqual(
                {
                    "mutated": False,
                    "required_current_pipeline_class": "non-authored",
                    "required_current_authored_order_role": "non-authored",
                },
                report["classification_dimension"],
            )
            proofs = report["proof_results"]
            self.assertEqual(
                ["0x42db50", "0x42dc30", "0x42dcf0"],
                [proof["address"] for proof in proofs],
            )
            self.assertEqual([224, 172, 156], [p["compared_byte_count"] for p in proofs])
            self.assertEqual([0, 20, 20], [p["masked_byte_count"] for p in proofs])
            self.assertEqual([0, 5, 5], [len(p["relocations"]) for p in proofs])
            self.assertEqual(
                [
                    "$L34183",
                    "__except_list",
                    "__except_list",
                    "_IID_IConnectionPointContainer",
                    "__except_list",
                ],
                [row["target_symbol"] for row in proofs[1]["relocations"]],
            )
            self.assertEqual(
                ((3, "0x4c9cf0"), (9, "0x0"), (17, "0x0"), (50, "0x4d43a0"), (168, "0x0")),
                proofs[1]["retail_relocation_values"],
            )
            owner = report["records"]["owner"]
            primaries = [
                row for row in owner["relationships"] if row["kind"] == "primary-function"
            ]
            self.assertEqual(3, len(primaries))
            self.assertEqual([], owner["source_paths"])
            self.assertEqual({}, owner["reimplementation"]["entries"])
            for function in report["records"]["functions"]:
                self.assertEqual("provider-function", function["kind"])
                self.assertEqual("provider", function["disposition"])
                self.assertEqual("non-authored", function["pipeline_class"])
                self.assertEqual("non-authored", function["authored_order_role"])
                self.assertNotIn("object_symbol", function)
                self.assertNotIn("provider_object_identity", function)
                self.assertEqual(
                    {"state": "not-applicable", "reason_code": "provider-boundary"},
                    function["source_traceability"],
                )

    def test_atlimpl_cluster_apply_atomically_retires_legacy_owner(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            revision = _atlimpl_cluster_tracker_fixture(tracker)
            report = register_atlimpl_provider_cluster(
                progress=tracker,
                reference=canonical_retail_reference(),
                payload=ATLIMPL_CLUSTER_REQUEST,
                expected_revision=revision,
                apply=True,
                vc5_root=VC5_ROOT,
            )
            self.assertTrue(report["commit"]["applied"])
            data = json.loads(tracker.read_text(encoding="utf-8"))
            self.assertEqual(revision + 1, data["revision"])
            self.assertNotIn(ATLIMPL_CLUSTER_LEGACY_OWNER_ID, data["owners"])
            self.assertIn(ATLIMPL_CLUSTER_OWNER_ID, data["owners"])
            self.assertNotIn("recoil:function:0x42de00", [
                row.get("symbol_id")
                for row in data["owners"][ATLIMPL_CLUSTER_OWNER_ID]["relationships"]
            ])
            with self.assertRaisesRegex(
                ProviderFunctionMutationError, "revision changed"
            ):
                register_atlimpl_provider_cluster(
                    progress=tracker,
                    reference=canonical_retail_reference(),
                    payload=ATLIMPL_CLUSTER_REQUEST,
                    expected_revision=revision,
                    apply=False,
                    vc5_root=VC5_ROOT,
                )

    def test_atlimpl_cluster_payload_rejects_partial_extra_and_guessed_identity(
        self,
    ) -> None:
        cases = (
            {**ATLIMPL_CLUSTER_REQUEST, "members": ATLIMPL_CLUSTER_REQUEST["members"][:2]},
            {
                **ATLIMPL_CLUSTER_REQUEST,
                "members": [
                    *ATLIMPL_CLUSTER_REQUEST["members"],
                    {
                        "address": "0x42de00",
                        "symbol_id": "recoil:function:0x42de00",
                        "canonical_function": "ATL::SafeVtableRelease",
                    },
                ],
            },
            {**ATLIMPL_CLUSTER_REQUEST, "original_translation_unit": "player.obj"},
            {**ATLIMPL_CLUSTER_REQUEST, "retail_coff_symbols": ["?AtlAdvise@@..."]},
            {**ATLIMPL_CLUSTER_REQUEST, "candidate_bytes": "90"},
            {**ATLIMPL_CLUSTER_REQUEST, "recipe_id": "vc5sp3-atlimpl-v2"},
        )
        for payload in cases:
            with self.subTest(payload=payload):
                with self.assertRaises(ProviderFunctionMutationError):
                    normalize_atlimpl_cluster_request(payload)

    def test_atlimpl_cluster_rejects_owner_symbol_source_and_member_drift(self) -> None:
        mutations = (
            ("owner", "name", "zCom changed"),
            ("symbol", "end_exclusive", "0x42dc20"),
            ("symbol", "pipeline_class", "authored"),
            ("symbol", "authored_order_role", "authored-body"),
            (
                "symbol",
                "source_traceability",
                {"state": "mapped", "source_edges": ["src/guessed.cpp"]},
            ),
        )
        for kind, field, value in mutations:
            with self.subTest(kind=kind, field=field):
                with tempfile.TemporaryDirectory() as temporary:
                    tracker = Path(temporary) / "progress.json"
                    revision = _atlimpl_cluster_tracker_fixture(tracker)
                    data = json.loads(tracker.read_text(encoding="utf-8"))
                    if kind == "owner":
                        data["owners"][ATLIMPL_CLUSTER_LEGACY_OWNER_ID][field] = value
                    else:
                        data["symbols"]["recoil:function:0x42db50"][field] = value
                    tracker.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
                    with self.assertRaises(ProviderFunctionMutationError):
                        register_atlimpl_provider_cluster(
                            progress=tracker,
                            reference=canonical_retail_reference(),
                            payload=ATLIMPL_CLUSTER_REQUEST,
                            expected_revision=revision,
                            apply=False,
                            vc5_root=VC5_ROOT,
                        )

        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            revision = _atlimpl_cluster_tracker_fixture(tracker)
            data = json.loads(tracker.read_text(encoding="utf-8"))
            data["symbols"]["recoil:function:0x42db60"] = {
                "address": "0x42db60",
                "binary": "recoil",
                "end_exclusive": "0x42db61",
                "extent_state": "known",
                "kind": "function",
                "size": 1,
            }
            tracker.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
            with self.assertRaisesRegex(
                ProviderFunctionMutationError, "physical cluster census"
            ):
                register_atlimpl_provider_cluster(
                    progress=tracker,
                    reference=canonical_retail_reference(),
                    payload=ATLIMPL_CLUSTER_REQUEST,
                    expected_revision=revision,
                    apply=False,
                    vc5_root=VC5_ROOT,
                )

    def test_atlimpl_cluster_rejects_retail_body_and_relocation_operand_drift(self) -> None:
        cases = (("0x42db50", 0, 0x90), ("0x42dc30", 3, 0x00))
        for address, offset, value in cases:
            with self.subTest(address=address, offset=offset):
                with tempfile.TemporaryDirectory() as temporary:
                    tracker = Path(temporary) / "progress.json"
                    reference = Path(temporary) / "Recoil.exe"
                    revision = _atlimpl_cluster_tracker_fixture(tracker)
                    _mutated_reference(reference, address=address, offset=offset, value=value)
                    with self.assertRaises(ProviderFunctionMutationError):
                        register_atlimpl_provider_cluster(
                            progress=tracker,
                            reference=reference,
                            payload=ATLIMPL_CLUSTER_REQUEST,
                            expected_revision=revision,
                            apply=False,
                            vc5_root=VC5_ROOT,
                        )

    def test_atlimpl_cluster_rejects_empty_canonical_source_installation(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tracker = root / "progress.json"
            empty_source = root / "ATLIMPL.CPP"
            empty_source.write_bytes(b"")
            revision = _atlimpl_cluster_tracker_fixture(tracker)
            with patch(
                "_recoil.commands.provider_function_mutation._resolve_library",
                return_value=empty_source,
            ):
                with self.assertRaisesRegex(
                    ProviderFunctionMutationError, "source is empty"
                ):
                    register_atlimpl_provider_cluster(
                        progress=tracker,
                        reference=canonical_retail_reference(),
                        payload=ATLIMPL_CLUSTER_REQUEST,
                        expected_revision=revision,
                        apply=False,
                        vc5_root=VC5_ROOT,
                    )

    def test_current_header_comdat_blocker_recipes_match_exact_retail_bodies(self) -> None:
        for raw_request, retail_body in CURRENT_HEADER_COMDAT_CASES:
            request = normalize_provider_function_request(raw_request)
            with self.subTest(recipe=request["probe_recipe"]):
                proof = _provider_header_comdat_proof(
                    vc5_root=VC5_ROOT,
                    request=request,
                    body_size=len(retail_body),
                    retail_body=retail_body,
                )
                self.assertEqual(request["object_symbol"], proof.object_symbol)
                self.assertEqual(len(retail_body), proof.body_size)
                self.assertEqual(len(retail_body), proof.compared_byte_count)
                self.assertEqual(0, proof.masked_byte_count)
                self.assertEqual((), proof.relocations)
                self.assertEqual("any", proof.comdat_selection_name)
                self.assertEqual(request["probe_recipe"], proof.probe_recipe)
                self.assertEqual(
                    ("/nologo", "/c", "/TP", "/Gy", "/O2", "/Ob1", "/Gr", "/Zl", "/X"),
                    proof.compile_flags,
                )

    def test_current_header_comdat_blocker_recipes_reject_first_byte_mismatch(self) -> None:
        for raw_request, retail_body in CURRENT_HEADER_COMDAT_CASES:
            request = normalize_provider_function_request(raw_request)
            mismatched = bytes([retail_body[0] ^ 1]) + retail_body[1:]
            with self.subTest(recipe=request["probe_recipe"]):
                with self.assertRaisesRegex(
                    ProviderFunctionMutationError,
                    "immutable retail bytes differ .* body offset 0x0",
                ):
                    _provider_header_comdat_proof(
                        vc5_root=VC5_ROOT,
                        request=request,
                        body_size=len(mismatched),
                        retail_body=mismatched,
                    )

    def test_vector_int_ufill_recipe_rejects_truncated_retail_extent(self) -> None:
        request = normalize_provider_function_request(VECTOR_INT_UFILL_REQUEST)
        truncated = VECTOR_INT_UFILL_OBJECT_BYTES[:-1]
        with self.assertRaisesRegex(
            ProviderFunctionMutationError,
            "registered probe recipe requires exact retail function extent",
        ):
            _provider_header_comdat_proof(
                vc5_root=VC5_ROOT,
                request=request,
                body_size=len(truncated),
                retail_body=truncated,
            )

    def test_header_comdat_dry_run_proves_construct_without_mutation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            reference = Path(temporary) / "Recoil.exe"
            revision = _header_tracker_fixture(tracker)
            _header_reference_fixture(reference)
            before = tracker.read_bytes()
            report = register_provider_function(
                progress=tracker,
                reference=reference,
                address=ATEXIT_ADDRESS,
                payload=HEADER_REQUEST,
                expected_revision=revision,
                apply=False,
                vc5_root=VC5_ROOT,
            )
            self.assertFalse(report["commit"]["applied"])
            self.assertEqual(before, tracker.read_bytes())
            self.assertEqual(
                "immutable-retail-plus-canonical-vc5-header-comdat-probe",
                report["validation_mode"],
            )
            proof = report["provider_object_proof"]
            self.assertEqual("canonical-header-comdat", proof["proof_mode"])
            self.assertEqual("VC/INCLUDE/xmemory", proof["canonical_header"])
            self.assertEqual("vc5-xmemory-construct-int-v1", proof["probe_recipe"])
            self.assertEqual("?_Construct@std@@YIXPAHABH@Z", proof["object_symbol"])
            self.assertEqual(0x18, proof["body_size"])
            self.assertEqual("any", proof["comdat_selection_name"])
            self.assertEqual(
                (
                    {
                        "offset": 7,
                        "type": "REL32",
                        "type_value": 0x14,
                        "width": 4,
                        "target_symbol": "??2@YAPAXIPAX@Z",
                    },
                ),
                proof["relocations"],
            )
            function = report["records"]["function"]
            identity = function["provider_object_identity"]
            self.assertEqual("recoil-provider-function-object-v2", identity["schema"])
            self.assertEqual("vc5-stl", identity["semantic_provider"])
            self.assertEqual(
                {"state": "winner-unknown"}, identity["physical_emitter"]
            )
            self.assertEqual(
                {
                    "winner_status": "winner-unknown",
                    "logical_symbols": ["?_Construct@std@@YIXPAHABH@Z"],
                },
                identity["retail_icf"],
            )
            self.assertEqual(
                {"state": "not-applicable", "reason_code": "provider-boundary"},
                function["source_traceability"],
            )
            owner = report["records"]["owner"]
            self.assertEqual({}, owner["reimplementation"]["entries"])
            self.assertEqual("provider-boundary", owner["kind"])

    def test_header_comdat_recipe_does_not_prematurely_accept_construct_retail_row(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            revision = _header_tracker_fixture(tracker)
            data = json.loads(tracker.read_text(encoding="utf-8"))
            source = deepcopy(
                ProgressStore(canonical_progress_path()).load().data["symbols"][
                    f"recoil:function:{CONSTRUCT_ADDRESS}"
                ]
            )
            data["symbols"].pop(f"recoil:function:{ATEXIT_ADDRESS}")
            data["symbols"][f"recoil:function:{CONSTRUCT_ADDRESS}"] = source
            tracker.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
            with self.assertRaises(ProviderFunctionMutationError):
                register_provider_function(
                    progress=tracker,
                    reference=canonical_retail_reference(),
                    address=CONSTRUCT_ADDRESS,
                    payload=HEADER_REQUEST,
                    expected_revision=revision,
                    apply=False,
                    vc5_root=VC5_ROOT,
                )

    def test_atexit_dry_run_proves_exact_archive_member_without_mutation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            revision = _tracker_fixture(tracker)
            before = tracker.read_bytes()
            report = register_provider_function(
                progress=tracker,
                reference=canonical_retail_reference(),
                address=ATEXIT_ADDRESS,
                payload=REQUEST,
                expected_revision=revision,
                apply=False,
                vc5_root=VC5_ROOT,
            )
            self.assertFalse(report["commit"]["applied"])
            self.assertEqual(before, tracker.read_bytes())
            proof = report["provider_object_proof"]
            self.assertEqual("_atexit", proof["object_symbol"])
            self.assertEqual(0x20, proof["body_size"])
            self.assertEqual(4, proof["masked_byte_count"])
            self.assertEqual(
                (
                    {
                        "offset": 6,
                        "type": "REL32",
                        "type_value": 0x14,
                        "width": 4,
                        "target_symbol": "__onexit",
                    },
                ),
                proof["relocations"],
            )
            function = report["records"]["function"]
            self.assertEqual("provider-function", function["kind"])
            self.assertEqual("provider", function["disposition"])
            self.assertEqual("_atexit", function["object_symbol"])
            self.assertEqual(
                "recoil-provider-function-object-v1",
                function["provider_object_identity"]["schema"],
            )
            owner = report["records"]["owner"]
            self.assertEqual("provider-boundary", owner["kind"])
            self.assertEqual(
                ["anchor-address", "primary-function"],
                [row["kind"] for row in owner["relationships"]],
            )

    def test_apply_updates_only_fixture_and_revision_guards(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            revision = _tracker_fixture(tracker)
            report = register_provider_function(
                progress=tracker,
                reference=canonical_retail_reference(),
                address=ATEXIT_ADDRESS,
                payload=REQUEST,
                expected_revision=revision,
                apply=True,
                vc5_root=VC5_ROOT,
            )
            self.assertTrue(report["commit"]["applied"])
            data = json.loads(tracker.read_text(encoding="utf-8"))
            self.assertEqual(revision + 1, data["revision"])
            self.assertIn(REQUEST["owner_id"], data["owners"])
            indexes = build_identity_indexes(ProgressStore(tracker).load())
            self.assertEqual(
                "provider:recoil:function:0x4c60e0",
                indexes.by_candidate_name["_atexit"],
            )
            with self.assertRaisesRegex(
                ProviderFunctionMutationError, "revision changed"
            ):
                register_provider_function(
                    progress=tracker,
                    reference=canonical_retail_reference(),
                    address=ATEXIT_ADDRESS,
                    payload=REQUEST,
                    expected_revision=revision,
                    apply=False,
                    vc5_root=VC5_ROOT,
                )

    def test_request_rejects_candidate_import_and_path_escape_fields(self) -> None:
        cases = [
            {**REQUEST, "candidate_bytes": "90"},
            {**REQUEST, "library_path": "../MSVCRT.LIB"},
            {**REQUEST, "library_path": "VC\\LIB\\MSVCRT.LIB"},
            {**REQUEST, "archive_member": "/absolute.obj"},
            {**REQUEST, "object_symbol": "__imp__atexit"},
            {**REQUEST, "reviewed": False},
        ]
        for payload in cases:
            with self.subTest(payload=payload):
                with self.assertRaises(ProviderFunctionMutationError):
                    normalize_provider_function_request(payload)

    def test_header_comdat_route_rejects_unregistered_or_mismatched_recipes(self) -> None:
        cases = (
            {**HEADER_REQUEST, "probe_recipe": "vc5-vector-fill-n-v1"},
            {**HEADER_REQUEST, "canonical_header": "VC/INCLUDE/vector"},
            {**HEADER_REQUEST, "object_symbol": "?FillN@PlayerNodeFlagRestoreEntryVector@@QAEXXZ"},
            {**HEADER_REQUEST, "semantic_provider": "recoil-project"},
            {**HEADER_REQUEST, "physical_emitter_state": "player.obj"},
            {**HEADER_REQUEST, "retail_icf_winner_status": "selected-winner"},
            {**HEADER_REQUEST, "retail_icf_logical_symbols": []},
            {
                **HEADER_REQUEST,
                "retail_icf_logical_symbols": ["?_Other@std@@YIXXZ"],
            },
        )
        for payload in cases:
            with self.subTest(payload=payload):
                with self.assertRaises(ProviderFunctionMutationError):
                    normalize_provider_function_request(payload)

    def test_archive_parser_rejects_malformed_archives(self) -> None:
        malformed = (
            b"",
            b"!<arch>\nshort",
            b"!<arch>\n" + b"x" * 58 + b"xx",
            b"!<arch>\n"
            + b"a/              0           0     0     0       bad       `\n",
        )
        for archive in malformed:
            with self.subTest(archive=archive):
                with self.assertRaises(ProviderFunctionMutationError):
                    parse_archive_members(archive)

    def test_object_proof_rejects_member_symbol_extent_and_body_mismatch(self) -> None:
        cases = (
            {"archive_member": "build/intel/dll_obj/missing.obj"},
            {"object_symbol": "__onexit"},
            {"body_size": 0x1F, "retail_body": ATEXIT_OBJECT_BYTES[:0x1F]},
            {
                "retail_body": bytes([ATEXIT_OBJECT_BYTES[0] ^ 1])
                + ATEXIT_OBJECT_BYTES[1:]
            },
        )
        base = {
            "vc5_root": VC5_ROOT,
            "library_path": "VC/LIB/MSVCRT.LIB",
            "archive_member": "build/intel/dll_obj/atonexit.obj",
            "object_symbol": "_atexit",
            "body_size": 0x20,
            "retail_body": ATEXIT_OBJECT_BYTES,
        }
        for changes in cases:
            with self.subTest(changes=changes):
                with self.assertRaises(ProviderFunctionMutationError):
                    _provider_object_proof(**{**base, **changes})

    def test_owner_relationship_and_overlapping_symbol_collisions_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            revision = _tracker_fixture(tracker)
            data = json.loads(tracker.read_text(encoding="utf-8"))
            data["owners"]["recoil:owner:provider.conflict"] = {
                "binary": "recoil",
                "relationships": [
                    {
                        "kind": "primary-function",
                        "address": ATEXIT_ADDRESS,
                        "symbol_id": "recoil:function:0x4c60e0",
                    }
                ],
            }
            data["symbols"]["recoil:function:0x4c60f0"] = {
                "binary": "recoil",
                "kind": "function",
                "address": "0x4c60f0",
                "end_exclusive": "0x4c6104",
                "extent_state": "known",
                "size": 0x14,
            }
            tracker.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
            with self.assertRaisesRegex(
                ProviderFunctionMutationError, "overlapping or already owned"
            ):
                register_provider_function(
                    progress=tracker,
                    reference=canonical_retail_reference(),
                    address=ATEXIT_ADDRESS,
                    payload=REQUEST,
                    expected_revision=revision,
                    apply=False,
                    vc5_root=VC5_ROOT,
                )

    def test_existing_row_must_be_unowned_non_authored_exact_extent(self) -> None:
        mutations = (
            ("pipeline_class", "authored"),
            ("authored_order_role", "authored-body"),
            ("ownership_state", "primary-owned"),
            ("kind", "provider-function"),
            ("extent_state", "unknown"),
            ("size", 0x1F),
            ("import_dll", "MSVCRT.dll"),
        )
        for field, value in mutations:
            with self.subTest(field=field):
                with tempfile.TemporaryDirectory() as temporary:
                    tracker = Path(temporary) / "progress.json"
                    revision = _tracker_fixture(tracker)
                    data = json.loads(tracker.read_text(encoding="utf-8"))
                    data["symbols"]["recoil:function:0x4c60e0"][field] = value
                    tracker.write_text(
                        json.dumps(data, indent=2) + "\n", encoding="utf-8"
                    )
                    with self.assertRaises(ProviderFunctionMutationError):
                        register_provider_function(
                            progress=tracker,
                            reference=canonical_retail_reference(),
                            address=ATEXIT_ADDRESS,
                            payload=REQUEST,
                            expected_revision=revision,
                            apply=False,
                            vc5_root=VC5_ROOT,
                        )


if __name__ == "__main__":
    unittest.main()
