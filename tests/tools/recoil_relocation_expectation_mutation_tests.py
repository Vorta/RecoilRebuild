from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.asm_verify import (  # noqa: E402
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
)
from _recoil.commands.relocation_expectation_mutation import (  # noqa: E402
    RelocationExceptionMutationError,
    prepare_reviewed_exception,
    set_reviewed_exception,
)
from _recoil.commands.relocation_expectations import (  # noqa: E402
    PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY,
    RelocationExpectationError,
    bind_reviewed_exception_context,
    derive_relocation_expectations,
    normalize_reviewed_exception,
    reviewed_exception_staleness,
)
from _recoil.lib.progress import ProgressDocument, empty_progress_document  # noqa: E402
from _recoil.lib.worktree_control import resolve_canonical_control_root  # noqa: E402


REFERENCE: Path
SOURCE_ID = "recoil:function:0x401000"
TARGET_ID = "recoil:function:0x4c5b64"
SOURCE_OBJECT = "??0CAboutDlg@@QAE@I@Z"
TARGET_OBJECT = "??0CDialog@@QAE@IPAVCWnd@@@Z"
EVIDENCE_ID = "recoil:evidence:r7:000001"
PHYSICAL_SOURCE_ID = "recoil:function:0x402250"
PHYSICAL_TARGET_ID = "recoil:data:0x4cc820"
PHYSICAL_SOURCE_OBJECT = (
    "?TickAiMode2AltGunAttackWindow@AINet@@SIXPAUzUtil_SaveGameState@@MM@Z"
)
MISSING_SOURCE_ID = "recoil:function:0x4024a0"
MISSING_SOURCE_OBJECT = (
    "?SolveAltGunLeadTargetPoint@AINet@@SIXPAUzUtil_SaveGameState@@0PAUzVec3@@@Z"
)
MISSING_OWNER_ID = (
    "recoil:owner:battlesport_gameplay.player_ai_mode2_top_level_steering"
)
MISSING_OWNER_EVIDENCE = "recoil:evidence:r725:000465"


def canonical_retail_reference() -> Path:
    resolution = resolve_canonical_control_root(
        executing_worktree_root=REPO_ROOT,
        required_machine_local_paths=("support/Recoil.exe",),
    )
    return resolution.canonical_control_root / "support" / "Recoil.exe"


def target_binding(symbol: str, *, name: str) -> SimpleNamespace:
    return SimpleNamespace(
        target=SimpleNamespace(name=name),
        function=SimpleNamespace(symbol=symbol, logical_identity_key=""),
        source_from=f"{name}.cpp",
    )


def fixture_data(
    *,
    revision: int = 7,
    target_address: str = "0x4c5b64",
    target_end_exclusive: str = "0x4c5b6a",
) -> dict[str, object]:
    data = empty_progress_document()
    data["revision"] = revision
    data["symbols"] = {
        SOURCE_ID: {
            "binary": "recoil",
            "kind": "function",
            "address": "0x401000",
            "end_exclusive": "0x401020",
            "pipeline_class": "authored-lifecycle",
            "authored_order_role": "authored-lifecycle-body",
        },
        TARGET_ID: {
            "binary": "recoil",
            "kind": "function",
            "address": target_address,
            "end_exclusive": target_end_exclusive,
            "pipeline_class": "non-authored",
            "authored_order_role": "non-authored",
        },
    }
    data["evidence"] = {
        EVIDENCE_ID: {
            "freshness": "historical",
            "validation_mode": "historical-observation",
        }
    }
    return data


def bindings() -> dict[str, list[SimpleNamespace]]:
    return {
        SOURCE_ID: [target_binding(SOURCE_OBJECT, name="source")],
        TARGET_ID: [target_binding(TARGET_OBJECT, name="target")],
    }


def payload(**updates: object) -> dict[str, object]:
    value: dict[str, object] = {
        "reviewed": True,
        "object_symbol": SOURCE_OBJECT,
        "offset": 11,
        "type": IMAGE_REL_I386_REL32,
        "target_symbol": TARGET_OBJECT,
        "target_symbol_id": TARGET_ID,
        "coff_addend": 0,
        "resolved_target_addend": 0,
        "retail_target": "0x4c5b64",
        "reason": "reviewed ambiguous unit-test target",
        "evidence_ids": [EVIDENCE_ID],
    }
    value.update(updates)
    return value


def grouped_source(document: ProgressDocument) -> dict[str, object]:
    source = document.collection("symbols")[SOURCE_ID]
    return {
        **source,
        "symbol_id": SOURCE_ID,
        "scope_ids": [SOURCE_ID],
        "physical_rows": [source],
    }


def physical_fixture_data() -> dict[str, object]:
    data = empty_progress_document()
    data["revision"] = 2209
    data["symbols"] = {
        PHYSICAL_SOURCE_ID: {
            "binary": "recoil",
            "kind": "function",
            "address": "0x402250",
            "end_exclusive": "0x4024a0",
            "size": 592,
            "extent_state": "known",
            "output_section_id": "recoil:section:.text",
            "pipeline_class": "authored",
            "authored_order_role": "authored-body",
            "ownership_state": "primary-owned",
        },
        PHYSICAL_TARGET_ID: {
            "binary": "recoil",
            "kind": "data",
            "address": "0x4cc820",
            "end_exclusive": "0x4cc824",
            "size": 4,
            "extent_state": "known",
            "output_section_id": "recoil:section:.rdata",
            "ownership_state": "primary-owned",
        },
    }
    data["evidence"] = {
        EVIDENCE_ID: {
            "freshness": "historical",
            "validation_mode": "historical-observation",
        }
    }
    return data


def physical_bindings() -> dict[str, list[SimpleNamespace]]:
    return {
        PHYSICAL_SOURCE_ID: [
            target_binding(PHYSICAL_SOURCE_OBJECT, name="physical-source")
        ]
    }


def physical_payload(**updates: object) -> dict[str, object]:
    value: dict[str, object] = {
        "reviewed": True,
        "exception_mode": PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY,
        "object_symbol": PHYSICAL_SOURCE_OBJECT,
        "offsets": [245, 276],
        "type": IMAGE_REL_I386_DIR32,
        "target_symbol_id": PHYSICAL_TARGET_ID,
        "coff_addend": 0,
        "resolved_target_addend": 0,
        "retail_target": "0x4cc820",
        "reason": (
            "retail proves the exact physical target while original VC5 "
            "object-symbol provenance remains unresolved"
        ),
        "evidence_ids": [EVIDENCE_ID],
    }
    value.update(updates)
    return value


def missing_data_fixture_data() -> dict[str, object]:
    data = empty_progress_document()
    data["revision"] = 2245
    data["output_sections"] = {
        "recoil:section:.text": {"binary": "recoil", "name": ".text"},
        "recoil:section:.rdata": {"binary": "recoil", "name": ".rdata"},
    }
    data["symbols"] = {
        MISSING_SOURCE_ID: {
            "binary": "recoil",
            "kind": "function",
            "address": "0x4024a0",
            "end_exclusive": "0x4026d0",
            "size": 560,
            "extent_state": "known",
            "output_section_id": "recoil:section:.text",
            "pipeline_class": "authored",
            "authored_order_role": "authored-body",
            "ownership_state": "primary-owned",
        }
    }
    data["owners"] = {
        MISSING_OWNER_ID: {
            "binary": "recoil",
            "kind": "subsystem",
            "provider_state": "pending",
            "lifecycle_state": "discovered",
            "evidence_ids": [MISSING_OWNER_EVIDENCE],
            "relationships": [
                {
                    "kind": "primary-function",
                    "address": "0x4024a0",
                    "symbol_id": MISSING_SOURCE_ID,
                }
            ],
            "gates": {
                "boundary": "blocked",
                "byte": "blocked",
                "data": "blocked",
                "functional": "accepted",
                "owner_linkage": "blocked",
                "source": "blocked",
            },
            "reimplementation": {
                "entries": {
                    MISSING_SOURCE_ID: {
                        "kind": "function",
                        "tier": "C",
                        "evidence_ids": [],
                    }
                }
            },
        }
    }
    data["evidence"] = {
        MISSING_OWNER_EVIDENCE: {
            "freshness": "historical",
            "validation_mode": "historical-observation",
            "scope_ids": [MISSING_OWNER_ID],
        }
    }
    return data


def missing_data_bindings() -> dict[str, list[SimpleNamespace]]:
    return {
        MISSING_SOURCE_ID: [
            target_binding(MISSING_SOURCE_OBJECT, name="missing-data-source")
        ]
    }


def missing_data_payload(
    *,
    offset: int = 517,
    target_address: str = "0x4cc838",
    target_end_exclusive: str = "0x4cc83c",
    target_name: str = "g_AINetSolveAltGunLeadTargetPoint_FloatLiteral_4CC838",
    **updates: object,
) -> dict[str, object]:
    value: dict[str, object] = {
        "reviewed": True,
        "exception_mode": PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY,
        "object_symbol": MISSING_SOURCE_OBJECT,
        "offsets": [offset],
        "type": IMAGE_REL_I386_DIR32,
        "target_symbol_id": f"recoil:data:{target_address}",
        "coff_addend": 0,
        "resolved_target_addend": 0,
        "retail_target": target_address,
        "reason": (
            "retail proves one exact anonymous four-byte physical float target "
            "while original VC5 object-symbol provenance remains unresolved"
        ),
        "evidence_ids": [MISSING_OWNER_EVIDENCE],
        "create_missing_data": {
            "target_owner_id": MISSING_OWNER_ID,
            "target_end_exclusive": target_end_exclusive,
            "target_name": target_name,
        },
    }
    value.update(updates)
    return value


class RelocationExpectationMutationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        global REFERENCE
        REFERENCE = canonical_retail_reference()

    def write_fixture(self, root: Path, *, revision: int = 7) -> Path:
        path = root / "progress.json"
        path.write_text(json.dumps(fixture_data(revision=revision), indent=2), encoding="utf-8")
        return path

    def test_dry_run_validates_without_mutating_tracker(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            before = progress.read_text(encoding="utf-8")
            report = set_reviewed_exception(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=payload(),
                expected_revision=7,
                apply=False,
                bindings=bindings(),
            )
            self.assertFalse(report["commit"]["applied"])
            self.assertEqual(7, report["commit"]["previous_revision"])
            self.assertEqual(8, report["commit"]["revision"])
            self.assertEqual(before, progress.read_text(encoding="utf-8"))

    def test_rel32_raw_coff_addend_is_exact_symbol_relative_addend(self) -> None:
        for target_address, target_addend in (("0x4c5b64", 0), ("0x4c5b60", 4)):
            with self.subTest(target_addend=target_addend):
                document = ProgressDocument(
                    fixture_data(target_address=target_address)
                )
                normalized, _decoded = prepare_reviewed_exception(
                    document=document,
                    bindings=bindings(),
                    source_symbol_id=SOURCE_ID,
                    source_address="0x401000",
                    payload=payload(
                        coff_addend=target_addend,
                        resolved_target_addend=target_addend,
                    ),
                    reference=REFERENCE,
                )
                self.assertEqual(target_addend, normalized["coff_addend"])
                self.assertEqual(
                    target_addend, normalized["resolved_target_addend"]
                )

    def test_rel32_zero_addend_rejects_obsolete_minus_four_encoding(self) -> None:
        with self.assertRaisesRegex(
            RelocationExceptionMutationError, "coff_addend does not match"
        ):
            prepare_reviewed_exception(
                document=ProgressDocument(fixture_data()),
                bindings=bindings(),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=payload(coff_addend=0xFFFFFFFC),
                reference=REFERENCE,
            )

    def test_dir32_addend_contract_is_unchanged(self) -> None:
        target_addend = 4
        document = ProgressDocument(
            fixture_data(
                target_address="0x4cc734",
                target_end_exclusive="0x4cc73c",
            )
        )
        normalized, _decoded = prepare_reviewed_exception(
            document=document,
            bindings=bindings(),
            source_symbol_id=SOURCE_ID,
            source_address="0x401000",
            payload=payload(
                offset=17,
                type=IMAGE_REL_I386_DIR32,
                coff_addend=target_addend,
                resolved_target_addend=target_addend,
                retail_target="0x4cc738",
            ),
            reference=REFERENCE,
        )
        self.assertEqual(target_addend, normalized["coff_addend"])
        self.assertEqual(target_addend, normalized["resolved_target_addend"])

    def test_apply_increments_revision_and_stores_only_normalized_context(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            report = set_reviewed_exception(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=payload(),
                expected_revision=7,
                apply=True,
                bindings=bindings(),
            )
            self.assertTrue(report["commit"]["applied"])
            stored = json.loads(progress.read_text(encoding="utf-8"))
            self.assertEqual(8, stored["revision"])
            rows = stored["symbols"][SOURCE_ID]["relocation_expectation_exceptions"]
            self.assertEqual(1, len(rows))
            self.assertEqual("0x401000", rows[0]["source_binding"]["address"])
            self.assertEqual("0x4c5b64", rows[0]["target_binding"]["address"])
            self.assertNotIn("candidate", json.dumps(rows[0]).casefold())

    def test_source_and_target_tracker_drift_become_typed_stale_blockers(self) -> None:
        original_data = fixture_data()
        original = ProgressDocument(original_data)
        normalized, _decoded = prepare_reviewed_exception(
            document=original,
            bindings=bindings(),
            source_symbol_id=SOURCE_ID,
            source_address="0x401000",
            payload=payload(),
            reference=REFERENCE,
        )
        for changed_id in (SOURCE_ID, TARGET_ID):
            with self.subTest(changed_id=changed_id):
                drifted_data = deepcopy(original_data)
                drifted_data["symbols"][SOURCE_ID]["relocation_expectation_exceptions"] = [
                    normalized
                ]
                drifted_data["symbols"][changed_id]["pipeline_class"] = "unresolved"
                drifted = ProgressDocument(drifted_data)
                report = derive_relocation_expectations(
                    document=drifted,
                    row=grouped_source(drifted),
                    object_symbol=SOURCE_OBJECT,
                    bindings=bindings(),
                    reference=REFERENCE,
                )
                self.assertFalse(report["passed"])
                stale = [
                    item
                    for item in report["unresolved"]
                    if item["kind"] == "stale-reviewed-exception"
                ]
                self.assertEqual(1, len(stale), report)
                self.assertTrue(stale[0]["stale_fields"])

    def test_conflicting_same_site_exceptions_block_derivation(self) -> None:
        data = fixture_data()
        document = ProgressDocument(data)
        first, _decoded = prepare_reviewed_exception(
            document=document,
            bindings=bindings(),
            source_symbol_id=SOURCE_ID,
            source_address="0x401000",
            payload=payload(),
            reference=REFERENCE,
        )
        second = {**first, "reason": "conflicting review conclusion"}
        data["symbols"][SOURCE_ID]["relocation_expectation_exceptions"] = [first, second]
        conflicted = ProgressDocument(data)
        report = derive_relocation_expectations(
            document=conflicted,
            row=grouped_source(conflicted),
            object_symbol=SOURCE_OBJECT,
            bindings=bindings(),
            reference=REFERENCE,
        )
        self.assertFalse(report["passed"])
        self.assertIn(
            "conflicting-reviewed-exception",
            {item["kind"] for item in report["unresolved"]},
        )

    def test_candidate_fields_and_unsupported_forms_are_rejected(self) -> None:
        with self.assertRaisesRegex(RelocationExpectationError, "candidate-derived"):
            normalize_reviewed_exception(payload(candidate_target="0x4c5b64"))
        with self.assertRaisesRegex(RelocationExpectationError, "unsupported"):
            normalize_reviewed_exception(payload(type=0xFFFF))

    def test_wrong_retail_operand_and_source_address_are_rejected(self) -> None:
        document = ProgressDocument(fixture_data())
        with self.assertRaisesRegex(
            RelocationExceptionMutationError, "immutable retail operand"
        ):
            prepare_reviewed_exception(
                document=document,
                bindings=bindings(),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=payload(retail_target="0x4c5b65"),
                reference=REFERENCE,
            )
        with self.assertRaisesRegex(RelocationExceptionMutationError, "does not match"):
            prepare_reviewed_exception(
                document=document,
                bindings=bindings(),
                source_symbol_id=SOURCE_ID,
                source_address="0x401020",
                payload=payload(),
                reference=REFERENCE,
            )

    def test_duplicate_exact_set_is_rejected_without_revision_change(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            set_reviewed_exception(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=payload(),
                expected_revision=7,
                apply=True,
                bindings=bindings(),
            )
            with self.assertRaisesRegex(
                RelocationExceptionMutationError, "already present"
            ):
                set_reviewed_exception(
                    progress=progress,
                    reference=REFERENCE,
                    manifest_dir=Path("unused"),
                    source_symbol_id=SOURCE_ID,
                    source_address="0x401000",
                    payload=payload(),
                    expected_revision=8,
                    apply=True,
                    bindings=bindings(),
                )
            stored = json.loads(progress.read_text(encoding="utf-8"))
            self.assertEqual(8, stored["revision"])

    def test_physical_target_mode_derives_pair_context_without_candidate_ordinal(
        self,
    ) -> None:
        normalized, decoded = prepare_reviewed_exception(
            document=ProgressDocument(physical_fixture_data()),
            bindings=physical_bindings(),
            source_symbol_id=PHYSICAL_SOURCE_ID,
            source_address="0x402250",
            payload=physical_payload(),
            reference=REFERENCE,
        )

        self.assertEqual([245, 276], normalized["offsets"])
        self.assertNotIn("target_symbol", normalized)
        self.assertNotIn("target_binding", normalized)
        self.assertEqual(
            {
                "symbol_id": PHYSICAL_TARGET_ID,
                "binary": "recoil",
                "kind": "data",
                "extent_state": "known",
                "output_section_id": "recoil:section:.rdata",
                "ownership_state": "primary-owned",
                "address": "0x4cc820",
                "end_exclusive": "0x4cc824",
                "size": 4,
                "retail_content_hex": "0000003f",
            },
            normalized["physical_target_binding"],
        )
        self.assertEqual(
            {
                "kind": "vc5-temporary-static-data",
                "symbol_family": "$T<digits>",
                "storage_class": 3,
                "symbol_type": 0,
                "section_name": ".rdata",
                "requires_initialized_data": True,
                "forbids_uninitialized_data": True,
                "forbids_writable_data": True,
                "one_symbol_for_all_sites": True,
            },
            normalized["witness_contract"],
        )
        self.assertEqual([245, 276], [item["offset"] for item in decoded["sites"]])
        serialized = json.dumps(normalized)
        self.assertNotIn("$T75938", serialized)
        self.assertNotIn("$S73099", serialized)
        self.assertNotIn("candidate", serialized.casefold())

    def test_physical_target_mode_requires_exact_full_retail_site_population(
        self,
    ) -> None:
        for offsets in ([245], [245, 277], [245, 276, 289]):
            with self.subTest(offsets=offsets):
                with self.assertRaises(
                    (RelocationExpectationError, RelocationExceptionMutationError)
                ):
                    prepare_reviewed_exception(
                        document=ProgressDocument(physical_fixture_data()),
                        bindings=physical_bindings(),
                        source_symbol_id=PHYSICAL_SOURCE_ID,
                        source_address="0x402250",
                        payload=physical_payload(offsets=offsets),
                        reference=REFERENCE,
                    )

    def test_physical_target_mode_expands_group_to_stable_physical_tokens(
        self,
    ) -> None:
        data = physical_fixture_data()
        document = ProgressDocument(data)
        normalized, _decoded = prepare_reviewed_exception(
            document=document,
            bindings=physical_bindings(),
            source_symbol_id=PHYSICAL_SOURCE_ID,
            source_address="0x402250",
            payload=physical_payload(),
            reference=REFERENCE,
        )
        data["symbols"][PHYSICAL_SOURCE_ID][
            "relocation_expectation_exceptions"
        ] = [normalized]
        source = data["symbols"][PHYSICAL_SOURCE_ID]
        report = derive_relocation_expectations(
            document=ProgressDocument(data),
            row={
                **source,
                "symbol_id": PHYSICAL_SOURCE_ID,
                "scope_ids": [PHYSICAL_SOURCE_ID],
                "physical_rows": [source],
            },
            object_symbol=PHYSICAL_SOURCE_OBJECT,
            bindings=physical_bindings(),
            reference=REFERENCE,
        )
        rows = [
            item
            for item in report["expectations"]
            if item.get("provenance_mode")
            == PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY
        ]
        self.assertEqual([245, 276], [item["offset"] for item in rows])
        self.assertEqual(
            {"@physical-target:recoil:data:0x4cc820"},
            {item["target_symbol"] for item in rows},
        )
        self.assertEqual(
            {PHYSICAL_TARGET_ID},
            {item["target_symbol_id"] for item in rows},
        )
        self.assertNotIn("$S73099", json.dumps(rows))
        self.assertNotIn("$T75938", json.dumps(rows))

    def test_physical_target_mode_rejects_caller_supplied_provenance_or_witness(
        self,
    ) -> None:
        cases = (
            {"target_symbol": "_kPlayerAiForwardProbeLengthHalfScale$S73099"},
            {"candidate_symbol": "$T75938"},
            {"candidate_ordinal": 75938},
            {"witness_contract": {"symbol_regex": r"^\$T[0-9]+$"}},
            {"physical_target_binding": {"candidate_address": "0x1234"}},
        )
        for updates in cases:
            with self.subTest(updates=updates):
                with self.assertRaisesRegex(
                    RelocationExpectationError,
                    (
                        "candidate-derived|must not set target_symbol|derived by the "
                        "mutation tool|fixed VC5|unsupported fields"
                    ),
                ):
                    normalized = normalize_reviewed_exception(
                        physical_payload(**updates)
                    )
                    bind_reviewed_exception_context(
                        normalized,
                        document=ProgressDocument(physical_fixture_data()),
                        bindings=physical_bindings(),
                        source_symbol_id=PHYSICAL_SOURCE_ID,
                        reference=REFERENCE,
                    )

    def test_physical_target_mode_rejects_nonzero_addends_before_binding(
        self,
    ) -> None:
        for updates in (
            {"coff_addend": 1},
            {"resolved_target_addend": 1},
        ):
            with self.subTest(updates=updates):
                with self.assertRaisesRegex(
                    RelocationExpectationError,
                    "require zero coff_addend and resolved_target_addend",
                ):
                    normalize_reviewed_exception(physical_payload(**updates))

    def test_physical_target_mode_staleness_tracks_source_and_physical_data_not_named_binding(
        self,
    ) -> None:
        original_data = physical_fixture_data()
        normalized, _decoded = prepare_reviewed_exception(
            document=ProgressDocument(original_data),
            bindings=physical_bindings(),
            source_symbol_id=PHYSICAL_SOURCE_ID,
            source_address="0x402250",
            payload=physical_payload(),
            reference=REFERENCE,
        )
        named_binding_only = deepcopy(original_data)
        named_binding_only["symbols"][PHYSICAL_TARGET_ID][
            "relocation_target_binding"
        ] = {
            "reviewed": True,
            "object_symbol": "_unrelatedNamedReader$S12345",
        }
        _fresh, differences = reviewed_exception_staleness(
            normalized,
            document=ProgressDocument(named_binding_only),
            bindings=physical_bindings(),
            reference=REFERENCE,
        )
        self.assertEqual([], differences)

        for field, value in (
            ("output_section_id", "recoil:section:.data"),
            ("end_exclusive", "0x4cc828"),
            ("size", 8),
            ("extent_state", "unknown"),
            ("ownership_state", "unresolved"),
        ):
            with self.subTest(field=field):
                drifted = deepcopy(original_data)
                drifted["symbols"][PHYSICAL_TARGET_ID][field] = value
                _fresh, differences = reviewed_exception_staleness(
                    normalized,
                    document=ProgressDocument(drifted),
                    bindings=physical_bindings(),
                    reference=REFERENCE,
                )
                self.assertTrue(differences)
                self.assertEqual(
                    "physical_target_binding",
                    differences[-1]["field"],
                )

    def test_physical_target_mode_normalizes_one_exact_site(self) -> None:
        normalized = normalize_reviewed_exception(
            physical_payload(offsets=[245])
        )
        self.assertEqual([245], normalized["offsets"])

    def test_create_missing_data_dry_run_is_atomic_and_candidate_independent(
        self,
    ) -> None:
        with TemporaryDirectory() as temp:
            progress = Path(temp) / "progress.json"
            progress.write_text(
                json.dumps(missing_data_fixture_data(), indent=2),
                encoding="utf-8",
            )
            before = progress.read_text(encoding="utf-8")
            report = set_reviewed_exception(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=MISSING_SOURCE_ID,
                source_address="0x4024a0",
                payload=missing_data_payload(),
                expected_revision=2245,
                apply=False,
                bindings=missing_data_bindings(),
            )

            self.assertFalse(report["commit"]["applied"])
            self.assertTrue(report["target_created"])
            self.assertEqual(before, progress.read_text(encoding="utf-8"))
            self.assertEqual(
                "00010038",
                report["exception"]["physical_target_binding"][
                    "retail_content_hex"
                ],
            )
            self.assertEqual([517], report["exception"]["offsets"])
            self.assertNotIn("create_missing_data", report["exception"])
            self.assertNotIn("target_symbol", report["exception"])
            self.assertEqual(
                MISSING_OWNER_ID,
                report["exception"]["physical_target_owner_binding"][
                    "owner_id"
                ],
            )
            serialized = json.dumps(report["exception"])
            self.assertNotIn("candidate", serialized.casefold())
            self.assertNotRegex(serialized, r"\$T[0-9]+")

    def test_create_missing_data_apply_adds_symbol_relationship_and_exception_without_tier_change(
        self,
    ) -> None:
        initial = missing_data_fixture_data()
        before_gates = deepcopy(initial["owners"][MISSING_OWNER_ID]["gates"])
        before_reimplementation = deepcopy(
            initial["owners"][MISSING_OWNER_ID]["reimplementation"]
        )
        with TemporaryDirectory() as temp:
            progress = Path(temp) / "progress.json"
            progress.write_text(json.dumps(initial, indent=2), encoding="utf-8")
            report = set_reviewed_exception(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=MISSING_SOURCE_ID,
                source_address="0x4024a0",
                payload=missing_data_payload(),
                expected_revision=2245,
                apply=True,
                bindings=missing_data_bindings(),
            )
            self.assertTrue(report["commit"]["applied"])
            stored = json.loads(progress.read_text(encoding="utf-8"))
            target_id = "recoil:data:0x4cc838"
            target = stored["symbols"][target_id]
            self.assertEqual("0x4cc83c", target["end_exclusive"])
            self.assertEqual(4, target["size"])
            self.assertEqual("recoil:section:.rdata", target["output_section_id"])
            owner = stored["owners"][MISSING_OWNER_ID]
            self.assertEqual(before_gates, owner["gates"])
            self.assertEqual(before_reimplementation, owner["reimplementation"])
            self.assertIn(
                {
                    "kind": "primary-data",
                    "address": "0x4cc838",
                    "symbol_id": target_id,
                    "name": (
                        "g_AINetSolveAltGunLeadTargetPoint_FloatLiteral_4CC838"
                    ),
                },
                owner["relationships"],
            )
            exceptions = stored["symbols"][MISSING_SOURCE_ID][
                "relocation_expectation_exceptions"
            ]
            self.assertEqual(1, len(exceptions))
            _normalized, stale = reviewed_exception_staleness(
                exceptions[0],
                document=ProgressDocument(stored),
                bindings=missing_data_bindings(),
                reference=REFERENCE,
            )
            self.assertEqual([], stale)

    def test_create_missing_data_rejection_matrix(self) -> None:
        cases = (
            (
                missing_data_payload(
                    target_symbol_id="recoil:data:0x4cc839"
                ),
                "deterministic physical id",
                None,
            ),
            (
                missing_data_payload(target_end_exclusive="0x4cc840"),
                "exactly four bytes",
                None,
            ),
            (
                missing_data_payload(
                    create_missing_data={
                        "target_owner_id": MISSING_OWNER_ID,
                        "target_end_exclusive": "0x4cc83c",
                        "target_name": (
                            "g_AINetSolveAltGunLeadTargetPoint_FloatLiteral_4CC838"
                        ),
                        "candidate_symbol": "$T123",
                    }
                ),
                "candidate-derived",
                None,
            ),
            (
                missing_data_payload(
                    create_missing_data={
                        "target_owner_id": MISSING_OWNER_ID,
                        "target_end_exclusive": "0x4cc83c",
                        "target_name": "$T75938",
                    }
                ),
                "ordinals, patterns, and regex",
                None,
            ),
            (
                missing_data_payload(
                    create_missing_data={
                        "target_owner_id": MISSING_OWNER_ID,
                        "target_end_exclusive": "0x4cc83c",
                        "target_name": r"^\$T[0-9]+$",
                    }
                ),
                "ordinals, patterns, and regex",
                None,
            ),
            (
                missing_data_payload(
                    create_missing_data={
                        "target_owner_id": "recoil:owner:missing",
                        "target_end_exclusive": "0x4cc83c",
                        "target_name": (
                            "g_AINetSolveAltGunLeadTargetPoint_FloatLiteral_4CC838"
                        ),
                    }
                ),
                "unknown Recoil target owner",
                None,
            ),
            (
                missing_data_payload(),
                "owner evidence",
                lambda data: data["owners"][MISSING_OWNER_ID].update(
                    evidence_ids=[]
                ),
            ),
            (
                missing_data_payload(),
                "overlaps current symbol",
                lambda data: data["symbols"].update(
                    {
                        "recoil:data:overlap": {
                            "binary": "recoil",
                            "kind": "data",
                            "address": "0x4cc83a",
                            "end_exclusive": "0x4cc83e",
                        }
                    }
                ),
            ),
            (
                missing_data_payload(),
                "non-provider owner",
                lambda data: data["owners"][MISSING_OWNER_ID].update(
                    kind="provider-boundary"
                ),
            ),
        )
        for request, message, mutate in cases:
            with self.subTest(message=message):
                data = missing_data_fixture_data()
                if mutate is not None:
                    mutate(data)
                with TemporaryDirectory() as temp:
                    progress = Path(temp) / "progress.json"
                    progress.write_text(
                        json.dumps(data, indent=2),
                        encoding="utf-8",
                    )
                    with self.assertRaisesRegex(
                        (RelocationExpectationError, RelocationExceptionMutationError),
                        message,
                    ):
                        set_reviewed_exception(
                            progress=progress,
                            reference=REFERENCE,
                            manifest_dir=Path("unused"),
                            source_symbol_id=MISSING_SOURCE_ID,
                            source_address="0x4024a0",
                            payload=request,
                            expected_revision=2245,
                            apply=False,
                            bindings=missing_data_bindings(),
                        )

    def test_created_target_owner_and_relationship_drift_are_stale(self) -> None:
        initial = missing_data_fixture_data()
        with TemporaryDirectory() as temp:
            progress = Path(temp) / "progress.json"
            progress.write_text(json.dumps(initial, indent=2), encoding="utf-8")
            set_reviewed_exception(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=MISSING_SOURCE_ID,
                source_address="0x4024a0",
                payload=missing_data_payload(),
                expected_revision=2245,
                apply=True,
                bindings=missing_data_bindings(),
            )
            stored = json.loads(progress.read_text(encoding="utf-8"))
        exception = stored["symbols"][MISSING_SOURCE_ID][
            "relocation_expectation_exceptions"
        ][0]
        for mutate in (
            lambda data: data["owners"][MISSING_OWNER_ID].update(
                lifecycle_state="accepted"
            ),
            lambda data: data["owners"][MISSING_OWNER_ID]["relationships"][-1].update(
                name="drifted"
            ),
            lambda data: data["owners"][MISSING_OWNER_ID].update(
                evidence_ids=[]
            ),
            lambda data: data["evidence"][MISSING_OWNER_EVIDENCE].update(
                scope_ids=[]
            ),
        ):
            with self.subTest(mutate=mutate):
                drifted = deepcopy(stored)
                mutate(drifted)
                _normalized, stale = reviewed_exception_staleness(
                    exception,
                    document=ProgressDocument(drifted),
                    bindings=missing_data_bindings(),
                    reference=REFERENCE,
                )
                self.assertTrue(stale)


if __name__ == "__main__":
    unittest.main()
