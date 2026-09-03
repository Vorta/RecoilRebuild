from __future__ import annotations

from copy import deepcopy
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
    ProviderFunctionMutationError,
    _provider_header_comdat_proof,
    _provider_object_proof,
    normalize_provider_function_request,
    parse_archive_members,
    register_provider_function,
)
from _recoil.commands.call_contract_verify import build_identity_indexes  # noqa: E402
from _recoil.lib.pe import parse_pe_headers, rva_to_offset  # noqa: E402
from _recoil.lib.progress import ProgressStore  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402


def canonical_retail_reference() -> Path:
    return REPO_ROOT / "support" / "Recoil.exe"

def canonical_progress_path() -> Path:
    return REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"

VC5_ROOT = canonical_retail_reference().parents[2] / "Compiler" / "VC5SP3"
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
VECTOR_POINTER_DESTROY_OBJECT_BYTES = bytes.fromhex(
    "c2080090909090909090909090909090"
)

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
VECTOR_POINTER_DESTROY_REQUEST = _header_request(
    recipe="vc5-vector-pointer-destroy-ob1-v1",
    header="VC/INCLUDE/vector",
    symbol="?_Destroy@?$vector@PAHV?$allocator@PAH@std@@@std@@IAEXPAPAH0@Z",
    owner_suffix="vector_pointer_destroy",
)

CURRENT_HEADER_COMDAT_CASES = (
    (CONSTRUCT_OB1_REQUEST, CONSTRUCT_OB1_OBJECT_BYTES),
    (VECTOR_UFILL_REQUEST, VECTOR_UFILL_OBJECT_BYTES),
    (VECTOR_UCOPY_REQUEST, VECTOR_UCOPY_OBJECT_BYTES),
    (VECTOR_INT_UFILL_REQUEST, VECTOR_INT_UFILL_OBJECT_BYTES),
    (VECTOR_POINTER_DESTROY_REQUEST, VECTOR_POINTER_DESTROY_OBJECT_BYTES),
)


def _write_tracker(path: Path, data: dict[str, object]) -> None:
    ProgressSQLiteStore.create_from_mapping(
        path,
        data,
        cutover_pair_id="provider-function-test",
        overwrite=path.exists(),
    )


def _read_tracker(path: Path) -> dict[str, object]:
    return ProgressStore(path).load().data


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
    _write_tracker(path, data)
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
    _write_tracker(path, data)
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


class ProviderFunctionMutationTests(unittest.TestCase):
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

    def test_pointer_destroy_registration_accepts_only_detached_icf_row(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.sqlite3"
            data = ProgressStore(canonical_progress_path()).load().data
            function_id = "recoil:function:0x40bdf0"
            owner_id = "recoil:owner:legacy.hud_ui.struct_stdptrvector"
            provider_owner_id = (
                "recoil:owner:provider.vc5_stl.pointer_vector_destroy"
            )
            function = data["symbols"][function_id]
            provider_object = function.pop("provider_object_identity", None)
            provider_evidence_id = (
                provider_object.get("evidence_id")
                if isinstance(provider_object, dict)
                else None
            )
            function.update({"kind": "function", "disposition": "unresolved"})
            function.pop("object_symbol", None)
            if provider_evidence_id is not None:
                function["evidence_ids"] = [
                    evidence_id
                    for evidence_id in function["evidence_ids"]
                    if evidence_id != provider_evidence_id
                ]
                data["evidence"].pop(provider_evidence_id, None)
            data["owners"].pop(provider_owner_id, None)
            owner = data["owners"][owner_id]
            owner["reimplementation"]["entries"][function_id] = {
                "evidence_ids": ["recoil:evidence:r725:004640"],
                "kind": "function",
                "tier": "B",
            }
            owner["address_metadata"]["0x40bdf0"] = {
                "group": "ui.zhud",
                "name": "StdPtrVector::ClearNoOpDestroy",
                "source_path": "src/GameZRecoil/zHud/zhud_ui.cpp",
                "target": "std_ptr_vector_clear_no_op_destroy",
            }
            owner["relationships"].append(
                {
                    "kind": "primary-function",
                    "address": "0x40bdf0",
                    "symbol_id": function_id,
                }
            )

            with self.assertRaisesRegex(
                ProviderFunctionMutationError,
                "still claimed by primary source owners",
            ):
                _write_tracker(tracker, data)
                register_provider_function(
                    progress=tracker,
                    reference=canonical_retail_reference(),
                    address="0x40bdf0",
                    payload=VECTOR_POINTER_DESTROY_REQUEST,
                    expected_revision=int(data["revision"]),
                    apply=False,
                    vc5_root=VC5_ROOT,
                )

            replacement = deepcopy(owner)
            replacement["reimplementation"]["entries"].pop(function_id)
            replacement["address_metadata"].pop("0x40bdf0")
            replacement["relationships"] = [
                relationship
                for relationship in replacement["relationships"]
                if not (
                    relationship.get("kind") == "primary-function"
                    and relationship.get("symbol_id") == function_id
                )
            ]
            data["owners"][owner_id] = replacement
            _write_tracker(tracker, data)
            report = register_provider_function(
                progress=tracker,
                reference=canonical_retail_reference(),
                address="0x40bdf0",
                payload=VECTOR_POINTER_DESTROY_REQUEST,
                expected_revision=int(data["revision"]),
                apply=True,
                vc5_root=VC5_ROOT,
            )
            self.assertTrue(report["commit"]["applied"])
            function = report["records"]["function"]
            self.assertEqual(
                "compiler-generated-icf-representative",
                function["authored_order_role"],
            )
            self.assertEqual("provider", function["disposition"])
            indexes = build_identity_indexes(ProgressStore(tracker).load())
            self.assertEqual(
                "provider:recoil:function:0x40bdf0",
                indexes.by_candidate_name[
                    "?_Destroy@?$vector@PAHV?$allocator@PAH@std@@@std@@IAEXPAPAH0@Z"
                ],
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
            tracker = Path(temporary) / "progress.sqlite3"
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
            tracker = Path(temporary) / "progress.sqlite3"
            revision = _header_tracker_fixture(tracker)
            data = _read_tracker(tracker)
            source = deepcopy(
                ProgressStore(canonical_progress_path()).load().data["symbols"][
                    f"recoil:function:{CONSTRUCT_ADDRESS}"
                ]
            )
            data["symbols"].pop(f"recoil:function:{ATEXIT_ADDRESS}")
            data["symbols"][f"recoil:function:{CONSTRUCT_ADDRESS}"] = source
            _write_tracker(tracker, data)
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
            tracker = Path(temporary) / "progress.sqlite3"
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
            tracker = Path(temporary) / "progress.sqlite3"
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
            data = _read_tracker(tracker)
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
            tracker = Path(temporary) / "progress.sqlite3"
            revision = _tracker_fixture(tracker)
            data = _read_tracker(tracker)
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
            _write_tracker(tracker, data)
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
                    tracker = Path(temporary) / "progress.sqlite3"
                    revision = _tracker_fixture(tracker)
                    data = _read_tracker(tracker)
                    data["symbols"]["recoil:function:0x4c60e0"][field] = value
                    _write_tracker(tracker, data)
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
