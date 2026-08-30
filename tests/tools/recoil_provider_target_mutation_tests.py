from __future__ import annotations

from copy import deepcopy
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
from typing import Callable
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.provider_target_mutation import (  # noqa: E402
    ProviderTargetMutationError,
    normalize_provider_target_request,
    register_provider_target,
    retail_import_target,
)
from _recoil.lib.progress import ProgressStore, empty_progress_document  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402


def canonical_retail_reference() -> Path:
    return REPO_ROOT / "support" / "Recoil.exe"

ADDRESS = "0x4cc5d8"
OWNER_ID = "recoil:owner:provider.crt.rand_import"
FUNCTION_ID = "recoil:function:0x4cc5d8"
DATA_ID = "recoil:data:0x4cc5d8"
STORAGE_ID = "recoil:storage:va:0x4cc5d8"
ORDINAL_ADDRESS = "0x4cc3e0"
ORDINAL_OWNER_ID = "recoil:owner:provider.mfc42.afxwinmain_ordinal_1576_import"
LEGACY_ADDRESS = "0x4cc5c4"
LEGACY_OWNER_ID = "recoil:owner:provider.crt.sprintf_import"
LEGACY_FUNCTION_ID = "recoil:function:0x4cc5c4"
LEGACY_DATA_ID = "recoil:data:0x4cc5c4"
LEGACY_STORAGE_ID = "recoil:storage:va:0x4cc5c4"
LEGACY_EVIDENCE_ID = "recoil:evidence:r7:000001"


def request(**updates: object) -> dict[str, object]:
    value: dict[str, object] = {
        "reviewed": True,
        "dll": "MSVCRT.dll",
        "import_name": "rand",
        "object_symbol": "__imp__rand",
        "owner_id": OWNER_ID,
        "owner_name": "VC5 CRT rand import provider",
        "reason": "Retail IAT and VC5 CRT declaration identify the rand import target.",
    }
    value.update(updates)
    return value


def ordinal_request(**updates: object) -> dict[str, object]:
    value: dict[str, object] = {
        "reviewed": True,
        "dll": "MFC42.DLL",
        "import_name": "#1576",
        "import_ordinal": 1576,
        "object_symbol": "__imp_?AfxWinMain@@YGHPAUHINSTANCE__@@0PADH@Z",
        "owner_id": ORDINAL_OWNER_ID,
        "owner_name": "MFC42 AfxWinMain ordinal 1576 import",
        "reason": "Immutable retail IAT and reviewed VC5 symbol identify ordinal 1576.",
    }
    value.update(updates)
    return value


def legacy_request(**updates: object) -> dict[str, object]:
    value: dict[str, object] = {
        "reviewed": True,
        "dll": "MSVCRT.dll",
        "import_name": "sprintf",
        "object_symbol": "__imp__sprintf",
        "owner_id": LEGACY_OWNER_ID,
        "owner_name": "VC5 CRT sprintf import provider",
        "reason": (
            "Immutable retail IAT and reviewed VC5 CRT declaration identify "
            "the sprintf imported-address symbol."
        ),
    }
    value.update(updates)
    return value


def tracker(*, revision: int = 7) -> dict[str, object]:
    value = empty_progress_document()
    value["revision"] = revision
    value["output_sections"] = {
        "recoil:section:.rdata": {"binary": "recoil", "name": ".rdata"}
    }
    return value


def legacy_tracker(*, revision: int = 7) -> dict[str, object]:
    value = tracker(revision=revision)
    value["evidence"][LEGACY_EVIDENCE_ID] = {
        "artifacts": [],
        "disposition": "observed",
        "freshness": "historical",
        "gating": False,
        "kind": "legacy-owner",
        "provenance": {},
        "result": "passed",
        "scope_ids": [LEGACY_OWNER_ID, LEGACY_FUNCTION_ID],
        "summary": "Legacy accepted provider owner and function stub.",
        "validation_mode": "historical-observation",
    }
    value["owners"][LEGACY_OWNER_ID] = {
        "address_metadata": {
            LEGACY_ADDRESS: {
                "group": "provider.imports",
                "name": "sprintf",
                "target": "pending",
            }
        },
        "binary": "recoil",
        "blocker": "pending",
        "evidence_ids": [LEGACY_EVIDENCE_ID],
        "gates": {
            "boundary": "accepted",
            "byte": "deferred",
            "data": "accepted",
            "functional": "none",
            "owner_linkage": "none",
            "source": "accepted",
        },
        "kind": "provider-boundary",
        "legacy_id": "provider.crt.sprintf_import",
        "legacy_marker": {"preserve": True},
        "lifecycle_state": "accepted",
        "name": "VC5 CRT sprintf import provider",
        "provider_state": "accepted",
        "reimplementation": {"entries": {}},
        "relationships": [
            {"address": LEGACY_ADDRESS, "kind": "anchor-address"},
            {
                "address": LEGACY_ADDRESS,
                "kind": "primary-function",
                "symbol_id": LEGACY_FUNCTION_ID,
            },
        ],
        "section": "provider_platform",
        "source_paths": [],
    }
    value["symbols"][LEGACY_FUNCTION_ID] = {
        "address": LEGACY_ADDRESS,
        "binary": "recoil",
        "binary_state": {},
        "binary_state_diagnostics": {},
        "disposition": "provider",
        "end_exclusive": "0x4cc5c5",
        "evidence_ids": [LEGACY_EVIDENCE_ID],
        "extent_state": "known",
        "kind": "provider-function",
        "legacy_marker": {"preserve": True},
        "navigation_name": "VC5 CRT sprintf import provider",
        "output_section_id": "recoil:section:.rdata",
        "ownership_state": "primary-owned",
        "physical_block_id": None,
        "pipeline_class": "non-authored",
        "semantic_span_ids": [],
        "size": 1,
        "storage_contribution_ids": [],
        "verification_target_ids": [],
    }
    return value


class ProviderTargetMutationTests(unittest.TestCase):
    def write_tracker(self, root: Path, value: dict[str, object] | None = None) -> Path:
        path = root / "progress.sqlite3"
        ProgressSQLiteStore.create_from_mapping(
            path,
            value or tracker(),
            cutover_pair_id="provider-target-test",
        )
        return path

    def register(
        self,
        progress: Path,
        *,
        payload: dict[str, object] | None = None,
        revision: int = 7,
        apply: bool = False,
        address: str = ADDRESS,
    ) -> dict[str, object]:
        return register_provider_target(
            progress=progress,
            reference=canonical_retail_reference(),
            address=address,
            payload=payload or request(),
            expected_revision=revision,
            apply=apply,
        )

    def register_legacy(
        self,
        progress: Path,
        *,
        payload: dict[str, object] | None = None,
        revision: int = 7,
        apply: bool = False,
    ) -> dict[str, object]:
        return register_provider_target(
            progress=progress,
            reference=canonical_retail_reference(),
            address=LEGACY_ADDRESS,
            payload=payload or legacy_request(),
            expected_revision=revision,
            apply=apply,
        )

    def test_retail_import_identity_is_exact_and_derives_iat_coordinates(self) -> None:
        target, directories = retail_import_target(
            reference=canonical_retail_reference(),
            address=ADDRESS,
            dll="MSVCRT.dll",
            import_name="rand",
        )

        self.assertEqual("0x4cc5d8", target.address)
        self.assertEqual(3, target.descriptor_index)
        self.assertEqual(84, target.thunk_index)
        self.assertEqual("0xcc5d8", target.iat_rva)
        self.assertEqual("0xcc5dc", target.iat_end_rva)
        self.assertEqual("0xcc000", directories["iat_directory_rva"])

    def test_retail_import_identity_rejects_wrong_address_dll_or_name(self) -> None:
        cases = (
            ("0x4cc5d4", "MSVCRT.dll", "rand"),
            (ADDRESS, "KERNEL32.dll", "rand"),
            (ADDRESS, "MSVCRT.dll", "malloc"),
        )
        for address, dll, import_name in cases:
            with self.subTest(address=address, dll=dll, import_name=import_name):
                with self.assertRaisesRegex(ProviderTargetMutationError, "immutable retail"):
                    retail_import_target(
                        reference=canonical_retail_reference(),
                        address=address,
                        dll=dll,
                        import_name=import_name,
                    )

    def test_retail_ordinal_import_identity_is_exact(self) -> None:
        target, _directories = retail_import_target(
            reference=canonical_retail_reference(),
            address=ORDINAL_ADDRESS,
            dll="MFC42.DLL",
            import_name="#1576",
            import_ordinal=1576,
        )
        self.assertEqual(1576, target.import_ordinal)
        self.assertEqual(2, target.descriptor_index)
        self.assertEqual(155, target.thunk_index)
        self.assertEqual("0xcc3e0", target.iat_rva)

    def test_payload_is_strict_reviewed_and_candidate_independent(self) -> None:
        self.assertEqual(request(), normalize_provider_target_request(request()))
        invalid = (
            ({**request(), "reviewed": False}, "reviewed=true"),
            ({**request(), "candidate_symbol": "__imp__rand"}, "candidate-derived"),
            ({**request(), "target_end_exclusive": "0x4cc5dc"}, "unsupported"),
            ({**request(), "import_name": "#123"}, "requires exact reviewed import_ordinal"),
            ({**ordinal_request(), "import_ordinal": 1575}, "do not match exactly"),
            ({**request(), "import_ordinal": 1576}, "valid only"),
            ({**request(), "object_symbol": "_rand"}, "beginning __imp_"),
            ({**request(), "owner_id": "recoil:owner:authored.rand"}, "provider"),
        )
        for payload, message in invalid:
            with self.subTest(payload=payload):
                with self.assertRaisesRegex(ProviderTargetMutationError, message):
                    normalize_provider_target_request(payload)

    def test_ordinal_payload_and_immutable_unknown_ordinal_fail_closed(self) -> None:
        self.assertEqual(
            ordinal_request(), normalize_provider_target_request(ordinal_request())
        )
        with TemporaryDirectory() as temp:
            progress = self.write_tracker(Path(temp))
            with self.assertRaisesRegex(ProviderTargetMutationError, "immutable retail"):
                self.register(
                    progress,
                    payload=ordinal_request(import_name="#1575", import_ordinal=1575),
                    address=ORDINAL_ADDRESS,
                )

    def test_ordinal_dry_run_emits_exact_provider_package(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_tracker(Path(temp))
            report = self.register(
                progress,
                payload=ordinal_request(),
                address=ORDINAL_ADDRESS,
            )
            self.assertFalse(report["commit"]["applied"])
            self.assertEqual(1576, report["retail_import"]["import_ordinal"])
            self.assertEqual(
                1576, report["records"]["function"]["import_ordinal"]
            )
            self.assertEqual(1576, report["records"]["data"]["import_ordinal"])
            self.assertEqual(
                "__imp_?AfxWinMain@@YGHPAUHINSTANCE__@@0PADH@Z",
                report["records"]["function"]["object_symbol"],
            )

    def test_dry_run_proposes_exact_typed_package_without_writing(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_tracker(Path(temp))
            before = progress.read_bytes()

            report = self.register(progress)

            self.assertFalse(report["commit"]["applied"])
            self.assertEqual(7, report["commit"]["previous_revision"])
            self.assertEqual(8, report["commit"]["revision"])
            self.assertEqual(before, progress.read_bytes())
            self.assertTrue(report["candidate_independent"])
            retail = report["retail_import"]
            self.assertEqual("0x4cc5dc", retail["storage_end_exclusive"])
            self.assertEqual("recoil:section:.rdata", retail["output_section_id"])
            records = report["records"]
            self.assertEqual("0x4cc5d9", records["function"]["end_exclusive"])
            self.assertEqual(1, records["function"]["size"])
            self.assertEqual("__imp__rand", records["function"]["object_symbol"])
            self.assertEqual("0x4cc5dc", records["data"]["end_exclusive"])
            self.assertEqual(4, records["data"]["size"])
            self.assertEqual([STORAGE_ID], records["data"]["storage_contribution_ids"])
            self.assertEqual([DATA_ID], records["storage"]["symbol_ids"])
            self.assertEqual(
                {OWNER_ID, FUNCTION_ID, DATA_ID, STORAGE_ID},
                set(records["evidence"]["scope_ids"]),
            )
            self.assertEqual("accepted", records["owner"]["provider_state"])
            self.assertEqual({}, records["owner"]["reimplementation"]["entries"])
            relationships = records["owner"]["relationships"]
            self.assertEqual(
                ["anchor-address", "primary-function", "primary-data"],
                [item["kind"] for item in relationships],
            )
            self.assertEqual(
                "accepted", records["storage"]["verification"]["extent"]["disposition"]
            )
            self.assertEqual(
                "claim", records["storage"]["verification"]["raw"]["disposition"]
            )

    def test_apply_commits_all_records_once_under_one_revision(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_tracker(Path(temp))

            report = self.register(progress, apply=True)
            applied = ProgressStore(progress).load().data

            self.assertTrue(report["commit"]["applied"])
            self.assertEqual(8, applied["revision"])
            self.assertIn(OWNER_ID, applied["owners"])
            self.assertIn(FUNCTION_ID, applied["symbols"])
            self.assertIn(DATA_ID, applied["symbols"])
            self.assertIn(STORAGE_ID, applied["storage_contributions"])
            evidence_id = report["entity_ids"]["evidence_id"]
            self.assertEqual("recoil:evidence:r8:000001", evidence_id)
            self.assertIn(evidence_id, applied["evidence"])
            self.assertEqual([evidence_id], applied["owners"][OWNER_ID]["evidence_ids"])

    def test_revision_drift_fails_before_proposal(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_tracker(Path(temp))
            with self.assertRaisesRegex(ProviderTargetMutationError, "revision changed"):
                self.register(progress, revision=6)

    def test_missing_exact_output_section_fails_closed(self) -> None:
        with TemporaryDirectory() as temp:
            value = tracker()
            value["output_sections"] = {
                "recoil:section:.rdata": {"binary": "recoil", "name": ".data"}
            }
            progress = self.write_tracker(Path(temp), value)
            with self.assertRaisesRegex(ProviderTargetMutationError, "not exactly registered"):
                self.register(progress)

    def test_existing_ids_and_overlaps_fail_closed(self) -> None:
        cases: list[tuple[str, dict[str, object]]] = []

        owner = tracker()
        owner["owners"][OWNER_ID] = {"binary": "recoil", "kind": "provider-boundary"}
        cases.append(("owner already exists", owner))

        symbol = tracker()
        symbol["symbols"]["recoil:data:overlap"] = {
            "binary": "recoil",
            "kind": "data",
            "address": "0x4cc5d4",
            "end_exclusive": "0x4cc5dc",
            "extent_state": "known",
        }
        cases.append(("overlaps", symbol))

        unknown_symbol = tracker()
        unknown_symbol["symbols"]["recoil:data:unknown"] = {
            "binary": "recoil",
            "kind": "data",
            "address": ADDRESS,
            "extent_state": "unknown",
        }
        cases.append(("overlaps", unknown_symbol))

        storage = tracker()
        storage["storage_contributions"]["recoil:storage:unit"] = {
            "binary": "recoil",
            "reference": {
                "address": "0x4cc5d4",
                "end_exclusive": "0x4cc5dc",
                "extent_state": "known",
            },
        }
        cases.append(("overlaps", storage))

        relationship = tracker()
        relationship["owners"]["recoil:owner:provider.other"] = {
            "binary": "recoil",
            "kind": "provider-boundary",
            "relationships": [{"kind": "anchor-address", "address": ADDRESS}],
        }
        cases.append(("overlaps", relationship))

        for message, value in cases:
            with self.subTest(message=message):
                with TemporaryDirectory() as temp:
                    progress = self.write_tracker(Path(temp), deepcopy(value))
                    with self.assertRaisesRegex(ProviderTargetMutationError, message):
                        self.register(progress)

    def test_matching_accepted_legacy_stub_completes_exact_provider_package(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_tracker(Path(temp), legacy_tracker())

            report = self.register_legacy(progress, apply=True)
            applied = ProgressStore(progress).load().data

            self.assertEqual("legacy-provider-completion", report["operation"])
            self.assertEqual("completed", report["completion_state"])
            self.assertTrue(report["commit"]["applied"])
            evidence_id = report["entity_ids"]["evidence_id"]
            self.assertEqual("recoil:evidence:r8:000001", evidence_id)

            owner = applied["owners"][LEGACY_OWNER_ID]
            self.assertEqual({"preserve": True}, owner["legacy_marker"])
            self.assertEqual("none", owner["blocker"])
            self.assertEqual(
                "accepted",
                owner["address_metadata"][LEGACY_ADDRESS]["target"],
            )
            self.assertEqual(
                [LEGACY_EVIDENCE_ID, evidence_id],
                owner["evidence_ids"],
            )
            self.assertEqual(
                {
                    "kind": "primary-data",
                    "address": LEGACY_ADDRESS,
                    "symbol_id": LEGACY_DATA_ID,
                    "name": "MSVCRT.dll!sprintf IAT",
                },
                owner["relationships"][-1],
            )

            function = applied["symbols"][LEGACY_FUNCTION_ID]
            self.assertEqual({"preserve": True}, function["legacy_marker"])
            self.assertEqual("non-authored", function["authored_order_role"])
            self.assertEqual("MSVCRT.dll", function["import_dll"])
            self.assertEqual("sprintf", function["import_name"])
            self.assertEqual("__imp__sprintf", function["object_symbol"])
            self.assertEqual(
                [LEGACY_EVIDENCE_ID, evidence_id],
                function["evidence_ids"],
            )

            data = applied["symbols"][LEGACY_DATA_ID]
            self.assertEqual("0x4cc5c8", data["end_exclusive"])
            self.assertEqual(4, data["size"])
            self.assertEqual("known", data["extent_state"])
            self.assertEqual([LEGACY_STORAGE_ID], data["storage_contribution_ids"])
            self.assertEqual("MSVCRT.dll", data["import_dll"])
            self.assertEqual("sprintf", data["import_name"])

            storage = applied["storage_contributions"][LEGACY_STORAGE_ID]
            self.assertEqual("provider-data", storage["kind"])
            self.assertEqual([LEGACY_OWNER_ID], storage["owner_ids"])
            self.assertEqual([LEGACY_DATA_ID], storage["symbol_ids"])
            self.assertEqual("0x4cc5c8", storage["reference"]["end_exclusive"])
            self.assertEqual(4, storage["reference"]["size"])
            self.assertEqual(
                "accepted",
                storage["verification"]["extent"]["disposition"],
            )
            self.assertEqual(
                "live",
                storage["verification"]["extent"]["validation_mode"],
            )
            self.assertEqual(
                {
                    LEGACY_OWNER_ID,
                    LEGACY_FUNCTION_ID,
                    LEGACY_DATA_ID,
                    LEGACY_STORAGE_ID,
                },
                set(applied["evidence"][evidence_id]["scope_ids"]),
            )

    def test_legacy_completion_dry_run_preserves_tracker_bytes(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_tracker(Path(temp), legacy_tracker())
            before = progress.read_bytes()

            report = self.register_legacy(progress)

            self.assertFalse(report["commit"]["applied"])
            self.assertEqual("completed", report["completion_state"])
            self.assertEqual(before, progress.read_bytes())

    def test_legacy_completion_is_idempotent_without_revision_or_evidence_churn(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_tracker(Path(temp), legacy_tracker())
            first = self.register_legacy(progress, apply=True)
            before = progress.read_bytes()

            second = self.register_legacy(progress, revision=8, apply=True)

            self.assertEqual("already-current", second["completion_state"])
            self.assertTrue(second["idempotent"])
            self.assertFalse(second["commit"]["applied"])
            self.assertEqual(8, second["commit"]["previous_revision"])
            self.assertEqual(8, second["commit"]["revision"])
            self.assertIsNone(second["entity_ids"]["evidence_id"])
            self.assertEqual(before, progress.read_bytes())
            self.assertEqual(
                first["entity_ids"]["evidence_id"],
                second["records"]["owner"]["evidence_ids"][-1],
            )

    def test_legacy_completion_conflict_matrix_fails_without_mutation(self) -> None:
        cases: list[tuple[str, Callable[[dict[str, object]], None]]] = [
            (
                "owner",
                lambda value: value["owners"][LEGACY_OWNER_ID].update(
                    {"name": "conflicting owner"}
                ),
            ),
            (
                "function",
                lambda value: value["symbols"][LEGACY_FUNCTION_ID].update(
                    {"kind": "function"}
                ),
            ),
            (
                "address",
                lambda value: value["symbols"][LEGACY_FUNCTION_ID].update(
                    {"address": "0x4cc5c0"}
                ),
            ),
            (
                "DLL",
                lambda value: value["symbols"][LEGACY_FUNCTION_ID].update(
                    {"import_dll": "KERNEL32.dll"}
                ),
            ),
            (
                "import",
                lambda value: value["symbols"][LEGACY_FUNCTION_ID].update(
                    {"import_name": "printf"}
                ),
            ),
            (
                "object-symbol",
                lambda value: value["symbols"][LEGACY_FUNCTION_ID].update(
                    {"object_symbol": "__imp__printf"}
                ),
            ),
            (
                "extent",
                lambda value: value["symbols"][LEGACY_FUNCTION_ID].update(
                    {"end_exclusive": "0x4cc5c6", "size": 2}
                ),
            ),
            (
                "data",
                lambda value: value["symbols"].update(
                    {
                        LEGACY_DATA_ID: {
                            "address": LEGACY_ADDRESS,
                            "binary": "recoil",
                            "extent_state": "unknown",
                            "kind": "function",
                            "output_section_id": "recoil:section:.rdata",
                        }
                    }
                ),
            ),
            (
                "storage",
                lambda value: value["storage_contributions"].update(
                    {
                        LEGACY_STORAGE_ID: {
                            "applicability": {
                                dimension: True
                                for dimension in (
                                    "extent",
                                    "object",
                                    "relocation",
                                    "order",
                                    "link",
                                    "raw",
                                    "zero-fill",
                                )
                            },
                            "binary": "recoil",
                            "candidate": {"evidence_ids": [], "state": "missing"},
                            "evidence_ids": [],
                            "kind": "data-symbol",
                            "output_section_id": "recoil:section:.rdata",
                            "overlap": "none",
                            "owner_ids": ["recoil:owner:provider.other"],
                            "parent_contribution_id": None,
                            "reference": {
                                "address": LEGACY_ADDRESS,
                                "evidence_ids": [],
                                "extent_state": "unknown",
                            },
                            "symbol_ids": [LEGACY_DATA_ID],
                            "verification": {
                                dimension: {
                                    "disposition": "claim",
                                    "evidence_ids": [],
                                    "freshness": "current-unhashed",
                                    "result": "pending",
                                }
                                for dimension in (
                                    "extent",
                                    "object",
                                    "relocation",
                                    "order",
                                    "link",
                                    "raw",
                                    "zero-fill",
                                )
                            },
                        }
                    }
                ),
            ),
        ]
        for label, mutate in cases:
            with self.subTest(conflict=label), TemporaryDirectory() as temp:
                value = legacy_tracker()
                mutate(value)
                progress = self.write_tracker(Path(temp), value)
                before = progress.read_bytes()

                with self.assertRaisesRegex(
                    ProviderTargetMutationError,
                    "legacy completion failed",
                ):
                    self.register_legacy(progress)

                self.assertEqual(before, progress.read_bytes())


if __name__ == "__main__":
    unittest.main()
