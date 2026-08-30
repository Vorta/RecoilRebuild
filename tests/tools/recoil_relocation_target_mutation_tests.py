from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.relocation_expectations import (  # noqa: E402
    derive_relocation_expectations,
    relocation_target_binding_staleness,
)
from _recoil.commands.relocation_target_mutation import (  # noqa: E402
    RelocationTargetMutationError,
    bind_relocation_target,
    normalize_reviewed_target_request,
)
from _recoil.lib.pe import parse_pe_headers, rva_to_offset  # noqa: E402
from _recoil.lib.progress import (  # noqa: E402
    ProgressDocument,
    ProgressStore,
    empty_progress_document,
)
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402
from _recoil.lib.verification_targets import vc5_target_registration  # noqa: E402


REFERENCE: Path
SOURCE_ID = "recoil:function:0x401000"
SOURCE_OBJECT = "??0CAboutDlg@@QAE@I@Z"
PROVIDER_TARGET_ID = "recoil:function:0x4c5b64"
PROVIDER_OBJECT = "??0CDialog@@QAE@IPAVCWnd@@@Z"
PROVIDER_OWNER = "recoil:owner:provider.mfc42"
PROVIDER_EVIDENCE = "recoil:evidence:r725:006916"
DATA_TARGET_ID = "recoil:data:0x4cc738"
DATA_OBJECT = "??_7CAboutDlg@@6B@"
CORRECTED_DATA_OBJECT = "??_7CAboutDlgCorrected@@6B@"
DATA_OWNER = "recoil:owner:misc_unresolved.cabout_dlg"
DATA_EVIDENCE = "recoil:evidence:r725:008435"


def canonical_retail_reference() -> Path:
    return REPO_ROOT / "support" / "Recoil.exe"


def write_progress(path: Path, data: dict[str, object]) -> None:
    ProgressSQLiteStore.create_from_mapping(
        path,
        data,
        cutover_pair_id="relocation-target-test",
        overwrite=path.exists(),
    )


def read_progress(path: Path) -> dict[str, object]:
    return ProgressStore(path).load().data

def target_binding(symbol: str, *, source_from: str = "about.cpp") -> SimpleNamespace:
    return SimpleNamespace(
        target=SimpleNamespace(name="cabout-source"),
        function=SimpleNamespace(symbol=symbol, logical_identity_key=""),
        source_from=source_from,
    )


def bindings() -> dict[str, list[SimpleNamespace]]:
    return {SOURCE_ID: [target_binding(SOURCE_OBJECT)]}


def write_vc5_identity_manifest(
    root: Path,
    *,
    name: str,
    address: str,
    symbol: str,
) -> tuple[str, dict[str, object], Path]:
    path = root / f"{name}.json"
    path.write_text(
        json.dumps(
            {
                "name": name,
                "description": f"Unit-test exact identity target {name}",
                "source_filename": f"{name}.cpp",
                "compiler_env": "",
                "compiler_flags": [],
                "include_dirs": [],
                "source_files": [],
                "functions": [
                    {
                        "address": address,
                        "symbol": symbol,
                        "name": f"unit-test {name}",
                    }
                ],
                "source_from": f"src/Battlesport/{name}.cpp",
            },
            indent=2,
        ),
        encoding="utf-8",
    )
    target_id, record = vc5_target_registration(path)
    return target_id, record, path


def registered_provider_fixture(
    root: Path,
    *,
    target_specs: list[tuple[str, str, str]],
) -> tuple[dict[str, object], dict[str, list[SimpleNamespace]], list[Path]]:
    data = fixture_data()
    provider = data["symbols"][PROVIDER_TARGET_ID]
    provider["navigation_name"] = "CDialog::CDialog"
    provider["verification_target_ids"] = []
    manifests: list[Path] = []
    current_bindings = bindings()
    current_bindings[PROVIDER_TARGET_ID] = []
    for name, address, symbol in target_specs:
        target_id, record, path = write_vc5_identity_manifest(
            root,
            name=name,
            address=address,
            symbol=symbol,
        )
        data["verification_targets"][target_id] = record
        provider["verification_target_ids"].append(target_id)
        manifests.append(path)
        current_bindings[PROVIDER_TARGET_ID].append(target_binding(symbol))
    return data, current_bindings, manifests


def fixture_data(*, revision: int = 7) -> dict[str, object]:
    data = empty_progress_document()
    data["revision"] = revision
    data["output_sections"] = {
        "recoil:section:.text": {"binary": "recoil", "name": ".text"},
        "recoil:section:.rdata": {"binary": "recoil", "name": ".rdata"},
    }
    data["symbols"] = {
        SOURCE_ID: {
            "binary": "recoil",
            "kind": "function",
            "address": "0x401000",
            "end_exclusive": "0x401020",
            "extent_state": "known",
            "pipeline_class": "authored-lifecycle",
            "authored_order_role": "authored-lifecycle-body",
            "navigation_name": "CAboutDlg::CAboutDlg",
            "output_section_id": "recoil:section:.text",
            "ownership_state": "primary-owned",
        },
        PROVIDER_TARGET_ID: {
            "binary": "recoil",
            "kind": "function",
            "address": "0x4c5b64",
            "end_exclusive": "0x4c5b6a",
            "extent_state": "known",
            "pipeline_class": "unresolved",
            "authored_order_role": "unresolved",
            "navigation_name": PROVIDER_OBJECT,
            "output_section_id": "recoil:section:.text",
            "ownership_state": "unresolved",
        },
    }
    data["owners"] = {
        PROVIDER_OWNER: {
            "binary": "recoil",
            "kind": "provider-boundary",
            "provider_state": "accepted",
            "lifecycle_state": "accepted",
            "evidence_ids": [PROVIDER_EVIDENCE],
            "relationships": [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                }
            ],
            "reimplementation": {"entries": {}},
            "gates": {
                "boundary": "accepted",
                "byte": "deferred",
                "data": "none",
                "functional": "none",
                "owner_linkage": "none",
                "source": "accepted",
            },
        },
        DATA_OWNER: {
            "binary": "recoil",
            "kind": "class",
            "provider_state": "pending",
            "lifecycle_state": "blocked",
            "evidence_ids": [DATA_EVIDENCE],
            "relationships": [
                {
                    "kind": "primary-function",
                    "address": "0x401000",
                    "symbol_id": SOURCE_ID,
                }
            ],
            "reimplementation": {
                "entries": {
                    SOURCE_ID: {"kind": "function", "tier": "B", "evidence_ids": []}
                }
            },
            "gates": {
                "boundary": "accepted",
                "byte": "deferred",
                "data": "none",
                "functional": "accepted",
                "owner_linkage": "accepted",
                "source": "accepted",
            },
        },
    }
    data["evidence"] = {
        PROVIDER_EVIDENCE: {
            "freshness": "historical",
            "validation_mode": "historical-observation",
            "scope_ids": [PROVIDER_OWNER],
        },
        DATA_EVIDENCE: {
            "freshness": "historical",
            "validation_mode": "historical-observation",
            "scope_ids": [DATA_OWNER],
        },
    }
    return data


def provider_payload(**updates: object) -> dict[str, object]:
    value: dict[str, object] = {
        "reviewed": True,
        "source_object_symbol": SOURCE_OBJECT,
        "offset": 11,
        "target_object_symbol": PROVIDER_OBJECT,
        "target_owner_id": PROVIDER_OWNER,
        "reason": "reviewed MFC CDialog constructor target",
        "evidence_ids": [PROVIDER_EVIDENCE],
    }
    value.update(updates)
    return value


def data_payload(**updates: object) -> dict[str, object]:
    value: dict[str, object] = {
        "reviewed": True,
        "source_object_symbol": SOURCE_OBJECT,
        "offset": 17,
        "target_object_symbol": DATA_OBJECT,
        "target_owner_id": DATA_OWNER,
        "reason": "reviewed compiler-emitted CAboutDlg vtable target",
        "evidence_ids": [DATA_EVIDENCE],
        "create_missing_data": True,
        "target_end_exclusive": "0x4cc810",
        "target_name": "g_CAboutDlg_Vtbl",
    }
    value.update(updates)
    return value


def correction_payload(
    *, selector_updates: dict[str, object] | None = None, **updates: object
) -> dict[str, object]:
    selector: dict[str, object] = {
        "prior_target_symbol_id": DATA_TARGET_ID,
        "prior_target_object_symbol": DATA_OBJECT,
        "source_symbol_id": SOURCE_ID,
        "source_address": "0x401000",
        "source_object_symbol": SOURCE_OBJECT,
        "offset": 17,
    }
    if selector_updates:
        selector.update(selector_updates)
    value: dict[str, object] = {
        "reviewed": True,
        "source_object_symbol": SOURCE_OBJECT,
        "offset": 17,
        "target_object_symbol": CORRECTED_DATA_OBJECT,
        "target_owner_id": DATA_OWNER,
        "reason": "reviewed correction of the CAboutDlg vtable object identity",
        "evidence_ids": [DATA_EVIDENCE],
        "correction": selector,
    }
    value.update(updates)
    return value


def refresh_payload(
    *, selector_updates: dict[str, object] | None = None, **updates: object
) -> dict[str, object]:
    selector = {"refresh_only": True}
    if selector_updates:
        selector.update(selector_updates)
    value = correction_payload(selector_updates=selector)
    value.update(
        target_object_symbol=DATA_OBJECT,
        reason="reviewed refresh of stale CAboutDlg vtable binding context",
    )
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


def immutable_retail_bytes(start: int, end_exclusive: int) -> bytes:
    image = REFERENCE.read_bytes()
    headers = parse_pe_headers(image, source=str(REFERENCE))
    offset = rva_to_offset(start - headers.image_base, headers.sections)
    if offset is None:
        raise AssertionError(f"fixture address 0x{start:x} is not file-backed")
    return image[offset : offset + (end_exclusive - start)]


def write_coff_object(
    path: Path,
    *,
    symbol: str,
    body: bytes,
    duplicate_count: int = 1,
    machine: int = 0x14C,
    storage_class: int = 2,
    symbol_type: int = 0x20,
    section_characteristics: int = 0x20,
    relocations: tuple[tuple[int, int], ...] = (),
) -> None:
    encoded_symbol = symbol.encode("ascii") + b"\x00"
    string_table = struct.pack("<I", 4 + len(encoded_symbol)) + encoded_symbol
    raw_offset = 20 + 40
    relocation_offset = raw_offset + len(body)
    symbol_table_offset = relocation_offset + len(relocations) * 10
    file_header = struct.pack(
        "<HHIIIHH",
        machine,
        1,
        0,
        symbol_table_offset,
        duplicate_count,
        0,
        0,
    )
    section_header = struct.pack(
        "<8sIIIIIIHHI",
        b".text\x00\x00\x00",
        0,
        0,
        len(body),
        raw_offset,
        relocation_offset if relocations else 0,
        0,
        len(relocations),
        0,
        section_characteristics,
    )
    relocation_rows = b"".join(
        struct.pack("<IIH", offset, 0, relocation_type)
        for offset, relocation_type in relocations
    )
    symbol_rows = b"".join(
        struct.pack(
            "<8sIhHBB",
            struct.pack("<II", 0, 4),
            0,
            1,
            symbol_type,
            storage_class,
            0,
        )
        for _ in range(duplicate_count)
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        file_header
        + section_header
        + body
        + relocation_rows
        + symbol_rows
        + string_table
    )


class RelocationTargetMutationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        global REFERENCE
        REFERENCE = canonical_retail_reference()

    def write_fixture(self, root: Path, *, revision: int = 7) -> Path:
        path = root / "progress.sqlite3"
        write_progress(path, fixture_data(revision=revision))
        return path

    def create_data_binding(self, progress: Path) -> dict[str, object]:
        return bind_relocation_target(
            progress=progress,
            reference=REFERENCE,
            manifest_dir=Path("unused"),
            source_symbol_id=SOURCE_ID,
            source_address="0x401000",
            payload=data_payload(),
            expected_revision=7,
            apply=True,
            bindings=bindings(),
        )

    def test_existing_provider_target_dry_run_uses_exact_retail_and_owner_context(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            before = progress.read_bytes()
            report = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=provider_payload(),
                expected_revision=7,
                apply=False,
                bindings=bindings(),
            )
            self.assertFalse(report["commit"]["applied"])
            self.assertEqual(PROVIDER_TARGET_ID, report["target_symbol_id"])
            self.assertEqual("0x4c5b64", report["target_address"])
            self.assertFalse(report["target_created"])
            context = report["binding"]["binding_context"]
            self.assertEqual("0x4c5b6a", context["target"]["end_exclusive"])
            self.assertEqual(PROVIDER_OBJECT, context["target"]["object_symbol"])
            self.assertEqual(PROVIDER_OWNER, context["owner"]["owner_id"])
            self.assertEqual([PROVIDER_EVIDENCE], context["owner"]["binding_evidence_ids"])
            self.assertIsNone(context["relationship"])
            self.assertEqual(before, progress.read_bytes())

    def test_existing_nonprovider_function_uses_exact_primary_function_relationship(self) -> None:
        with TemporaryDirectory() as temp:
            root = Path(temp)
            data = fixture_data()
            relationship = {
                "kind": "primary-function",
                "address": "0x4c5b64",
                "symbol_id": PROVIDER_TARGET_ID,
            }
            data["owners"][DATA_OWNER]["relationships"].append(relationship)
            progress = root / "progress.sqlite3"
            write_progress(progress, data)
            payload = provider_payload(
                target_owner_id=DATA_OWNER,
                evidence_ids=[DATA_EVIDENCE],
                reason="reviewed authored function target relationship",
            )
            current_bindings = bindings()
            current_bindings[PROVIDER_TARGET_ID] = [target_binding(PROVIDER_OBJECT)]
            report = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=root,
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=payload,
                expected_revision=7,
                apply=False,
                bindings=current_bindings,
            )
            self.assertEqual(
                relationship,
                report["binding"]["binding_context"]["relationship"],
            )

            duplicate = deepcopy(data)
            duplicate["owners"][DATA_OWNER]["relationships"].append(
                deepcopy(relationship)
            )
            _normalized, stale = relocation_target_binding_staleness(
                report["binding"],
                document=ProgressDocument(duplicate),
                bindings=current_bindings,
                target_symbol_id=PROVIDER_TARGET_ID,
                reference=REFERENCE,
            )
            relationship_stale = [
                item for item in stale if item["field"] == "relationship"
            ]
            self.assertEqual(1, len(relationship_stale), stale)
            self.assertEqual(
                "target-owner-relationship-duplicate",
                relationship_stale[0]["reason"],
            )

    def test_existing_nonprovider_function_rejects_bad_relationship_state(self) -> None:
        relationship_cases: dict[str, list[dict[str, object]]] = {
            "wrong-kind": [
                {
                    "kind": "primary-data",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                    "name": "wrong-kind",
                }
            ],
            "unsupported-kind": [
                {
                    "kind": "secondary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                }
            ],
            "wrong-symbol": [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": "recoil:function:wrong",
                }
            ],
            "wrong-address": [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b65",
                    "symbol_id": PROVIDER_TARGET_ID,
                }
            ],
            "candidate-derived": [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                    "candidate_symbol": "?Candidate@@YAXXZ",
                }
            ],
            "duplicate": [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                },
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                },
            ],
        }
        for name, relationships in relationship_cases.items():
            with self.subTest(name=name), TemporaryDirectory() as temp:
                root = Path(temp)
                data = fixture_data()
                data["owners"][DATA_OWNER]["relationships"].extend(relationships)
                progress = root / "progress.sqlite3"
                write_progress(progress, data)
                payload = provider_payload(
                    target_owner_id=DATA_OWNER,
                    evidence_ids=[DATA_EVIDENCE],
                    reason="reviewed authored function target relationship",
                )
                current_bindings = bindings()
                current_bindings[PROVIDER_TARGET_ID] = [target_binding(PROVIDER_OBJECT)]
                before = progress.read_bytes()
                with self.assertRaises(RelocationTargetMutationError):
                    bind_relocation_target(
                        progress=progress,
                        reference=REFERENCE,
                        manifest_dir=root,
                        source_symbol_id=SOURCE_ID,
                        source_address="0x401000",
                        payload=payload,
                        expected_revision=7,
                        apply=False,
                        bindings=current_bindings,
                    )
                self.assertEqual(before, progress.read_bytes())

    def test_existing_target_accepts_one_exact_synchronized_vc5_registration(self) -> None:
        with TemporaryDirectory() as temp:
            root = Path(temp)
            data, current_bindings, _manifests = registered_provider_fixture(
                root,
                target_specs=[("provider_exact", "0x4c5b64", PROVIDER_OBJECT)],
            )
            progress = root / "progress.sqlite3"
            write_progress(progress, data)
            before = progress.read_bytes()
            report = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=root,
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=provider_payload(),
                expected_revision=7,
                apply=False,
                bindings=current_bindings,
            )
            self.assertFalse(report["commit"]["applied"])
            self.assertEqual(PROVIDER_OBJECT, report["binding"]["object_symbol"])
            self.assertEqual(before, progress.read_bytes())

    def test_existing_target_rejects_non_authoritative_vc5_registration_states(self) -> None:
        cases: list[tuple[str, list[tuple[str, str, str]]]] = [
            (
                "conflicting",
                [
                    ("provider_exact", "0x4c5b64", PROVIDER_OBJECT),
                    ("provider_conflict", "0x4c5b64", "?ConflictingProvider@@YAXXZ"),
                ],
            ),
            (
                "wrong-address",
                [("provider_wrong_address", "0x4c5b65", PROVIDER_OBJECT)],
            ),
            (
                "ambiguous",
                [
                    ("provider_exact_a", "0x4c5b64", PROVIDER_OBJECT),
                    ("provider_exact_b", "0x4c5b64", PROVIDER_OBJECT),
                ],
            ),
        ]
        for name, target_specs in cases:
            with self.subTest(name=name), TemporaryDirectory() as temp:
                root = Path(temp)
                data, current_bindings, _manifests = registered_provider_fixture(
                    root,
                    target_specs=target_specs,
                )
                progress = root / "progress.sqlite3"
                write_progress(progress, data)
                before = progress.read_bytes()
                with self.assertRaisesRegex(
                    RelocationTargetMutationError,
                    "does not match current exact tracker identity",
                ):
                    bind_relocation_target(
                        progress=progress,
                        reference=REFERENCE,
                        manifest_dir=root,
                        source_symbol_id=SOURCE_ID,
                        source_address="0x401000",
                        payload=provider_payload(),
                        expected_revision=7,
                        apply=False,
                        bindings=current_bindings,
                    )
                self.assertEqual(before, progress.read_bytes())

    def test_existing_target_rejects_stale_unregistered_and_functional_only_targets(self) -> None:
        with TemporaryDirectory() as temp:
            root = Path(temp)
            data, current_bindings, manifests = registered_provider_fixture(
                root,
                target_specs=[("provider_stale", "0x4c5b64", PROVIDER_OBJECT)],
            )
            manifest_data = json.loads(manifests[0].read_text(encoding="utf-8"))
            manifest_data["source_from"] = "src/Battlesport/provider_stale_moved.cpp"
            manifests[0].write_text(json.dumps(manifest_data, indent=2), encoding="utf-8")
            progress = root / "stale-progress.sqlite3"
            write_progress(progress, data)
            with self.assertRaisesRegex(
                RelocationTargetMutationError,
                "does not match current exact tracker identity",
            ):
                bind_relocation_target(
                    progress=progress,
                    reference=REFERENCE,
                    manifest_dir=root,
                    source_symbol_id=SOURCE_ID,
                    source_address="0x401000",
                    payload=provider_payload(),
                    expected_revision=7,
                    apply=False,
                    bindings=current_bindings,
                )

        for name, target_id, target_row in (
            ("unregistered", "recoil:vc5-target:missing", None),
            (
                "functional-only",
                "recoil:functional-target:provider",
                {
                    "binary": "recoil",
                    "kind": "functional",
                    "name": "provider",
                    "registration": {},
                    "registered_addresses": ["0x4c5b64"],
                },
            ),
        ):
            with self.subTest(name=name), TemporaryDirectory() as temp:
                root = Path(temp)
                data = fixture_data()
                provider = data["symbols"][PROVIDER_TARGET_ID]
                provider["navigation_name"] = "CDialog::CDialog"
                provider["verification_target_ids"] = [target_id]
                if target_row is not None:
                    data["verification_targets"][target_id] = target_row
                current_bindings = bindings()
                current_bindings[PROVIDER_TARGET_ID] = [target_binding(PROVIDER_OBJECT)]
                progress = root / "progress.sqlite3"
                write_progress(progress, data)
                before = progress.read_bytes()
                with self.assertRaisesRegex(
                    RelocationTargetMutationError,
                    "does not match current exact tracker identity",
                ):
                    bind_relocation_target(
                        progress=progress,
                        reference=REFERENCE,
                        manifest_dir=root,
                        source_symbol_id=SOURCE_ID,
                        source_address="0x401000",
                        payload=provider_payload(),
                        expected_revision=7,
                        apply=False,
                        bindings=current_bindings,
                    )
                self.assertEqual(before, progress.read_bytes())

    def test_missing_data_apply_creates_exact_pending_symbol_owner_relationship_and_tier_x(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            before_gates = fixture_data()["owners"][DATA_OWNER]["gates"]
            report = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=data_payload(),
                expected_revision=7,
                apply=True,
                bindings=bindings(),
            )
            self.assertTrue(report["commit"]["applied"])
            self.assertTrue(report["target_created"])
            self.assertEqual(DATA_TARGET_ID, report["target_symbol_id"])
            stored = read_progress(progress)
            self.assertEqual(8, stored["revision"])
            symbol = stored["symbols"][DATA_TARGET_ID]
            self.assertEqual("0x4cc738", symbol["address"])
            self.assertEqual("0x4cc810", symbol["end_exclusive"])
            self.assertEqual(216, symbol["size"])
            self.assertEqual("g_CAboutDlg_Vtbl", symbol["navigation_name"])
            self.assertEqual("recoil:section:.rdata", symbol["output_section_id"])
            self.assertEqual("pending", symbol["binary_state"]["linked_byte"]["result"])
            owner = stored["owners"][DATA_OWNER]
            relationship = {
                "kind": "primary-data",
                "address": "0x4cc738",
                "symbol_id": DATA_TARGET_ID,
                "name": "g_CAboutDlg_Vtbl",
            }
            self.assertIn(relationship, owner["relationships"])
            self.assertEqual("X", owner["reimplementation"]["entries"][DATA_TARGET_ID]["tier"])
            self.assertEqual(before_gates, owner["gates"])
            binding = symbol["relocation_target_binding"]
            self.assertEqual(DATA_OBJECT, binding["object_symbol"])
            self.assertEqual(17, binding["binding_context"]["relocation"]["offset"])
            self.assertEqual("0x4cc738", report["target_address"])

    def test_rejects_candidate_and_caller_supplied_retail_target_facts(self) -> None:
        with self.assertRaisesRegex(RelocationTargetMutationError, "candidate-derived"):
            normalize_reviewed_target_request(provider_payload(candidate_target="0x4c5b64"))
        for field, value in (
            ("target_address", "0x4c5b64"),
            ("retail_target", "0x4c5b64"),
            ("target_symbol_id", PROVIDER_TARGET_ID),
            ("type", 20),
            ("size", 6),
        ):
            with self.subTest(field=field), self.assertRaisesRegex(
                RelocationTargetMutationError, "not accepted"
            ):
                normalize_reviewed_target_request(provider_payload(**{field: value}))

    def test_stale_target_and_owner_context_are_typed_preflight_blockers(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            report = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=provider_payload(),
                expected_revision=7,
                apply=False,
                bindings=bindings(),
            )
            for drift in ("target", "owner"):
                with self.subTest(drift=drift):
                    data = fixture_data()
                    data["symbols"][PROVIDER_TARGET_ID]["relocation_target_binding"] = report[
                        "binding"
                    ]
                    if drift == "target":
                        data["symbols"][PROVIDER_TARGET_ID]["pipeline_class"] = "non-authored"
                    else:
                        data["owners"][PROVIDER_OWNER]["provider_state"] = "pending"
                    document = ProgressDocument(data)
                    derived = derive_relocation_expectations(
                        document=document,
                        row=grouped_source(document),
                        object_symbol=SOURCE_OBJECT,
                        bindings=bindings(),
                        reference=REFERENCE,
                    )
                    stale = [
                        item
                        for item in derived["unresolved"]
                        if item["kind"] == "stale-relocation-target-binding"
                    ]
                    self.assertEqual(1, len(stale), derived)
                    self.assertTrue(stale[0]["stale_fields"])

    def test_missing_data_creation_rejects_empty_or_overlapping_extent(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            for end in ("0x4cc738", "0x4c5b66"):
                with self.subTest(end=end), self.assertRaises(RelocationTargetMutationError):
                    bind_relocation_target(
                        progress=progress,
                        reference=REFERENCE,
                        manifest_dir=Path("unused"),
                        source_symbol_id=SOURCE_ID,
                        source_address="0x401000",
                        payload=data_payload(target_end_exclusive=end),
                        expected_revision=7,
                        apply=False,
                        bindings=bindings(),
                    )

    def test_exact_source_address_and_owner_scoped_evidence_are_required(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            with self.assertRaisesRegex(RelocationTargetMutationError, "does not match"):
                bind_relocation_target(
                    progress=progress,
                    reference=REFERENCE,
                    manifest_dir=Path("unused"),
                    source_symbol_id=SOURCE_ID,
                    source_address="0x401020",
                    payload=provider_payload(),
                    expected_revision=7,
                    apply=False,
                    bindings=bindings(),
                )
            bad = fixture_data()
            bad["evidence"][PROVIDER_EVIDENCE]["scope_ids"] = [DATA_OWNER]
            write_progress(progress, bad)
            with self.assertRaisesRegex(RelocationTargetMutationError, "owner-scoped"):
                bind_relocation_target(
                    progress=progress,
                    reference=REFERENCE,
                    manifest_dir=Path("unused"),
                    source_symbol_id=SOURCE_ID,
                    source_address="0x401000",
                    payload=provider_payload(),
                    expected_revision=7,
                    apply=False,
                    bindings=bindings(),
                )

    def test_explicit_correction_revalidates_and_replaces_exact_binding_only(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            self.create_data_binding(progress)
            before = progress.read_bytes()
            dry_run = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=correction_payload(),
                expected_revision=8,
                apply=False,
                bindings=bindings(),
            )
            self.assertEqual("correction", dry_run["mutation_mode"])
            self.assertEqual(DATA_OBJECT, dry_run["replaced_binding"]["object_symbol"])
            self.assertEqual(CORRECTED_DATA_OBJECT, dry_run["binding"]["object_symbol"])
            self.assertEqual(
                "created-data-symbol",
                dry_run["binding"]["binding_context"]["creation_mode"],
            )
            self.assertEqual(before, progress.read_bytes())

            applied = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=correction_payload(),
                expected_revision=8,
                apply=True,
                bindings=bindings(),
            )
            self.assertTrue(applied["commit"]["applied"])
            stored = read_progress(progress)
            self.assertEqual(9, stored["revision"])
            binding = stored["symbols"][DATA_TARGET_ID]["relocation_target_binding"]
            self.assertIsInstance(binding, dict)
            self.assertEqual(CORRECTED_DATA_OBJECT, binding["object_symbol"])
            self.assertEqual("0x4cc738", binding["binding_context"]["target"]["address"])
            self.assertEqual("0x4cc810", binding["binding_context"]["target"]["end_exclusive"])

    def test_refresh_replaces_one_stale_binding_with_current_context_only(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            self.create_data_binding(progress)
            data = read_progress(progress)
            data["symbols"][DATA_TARGET_ID]["pipeline_class"] = "authored"
            write_progress(progress, data)
            current_bindings = {
                SOURCE_ID: [
                    target_binding(SOURCE_OBJECT, source_from="refreshed-about.cpp")
                ]
            }
            before = progress.read_bytes()

            dry_run = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=refresh_payload(),
                expected_revision=8,
                apply=False,
                bindings=current_bindings,
            )

            self.assertEqual("refresh", dry_run["mutation_mode"])
            self.assertEqual(DATA_OBJECT, dry_run["replaced_binding"]["object_symbol"])
            self.assertEqual(DATA_OBJECT, dry_run["binding"]["object_symbol"])
            self.assertEqual(
                {"source_binding", "target"},
                {item["field"] for item in dry_run["prior_staleness"]},
            )
            context = dry_run["binding"]["binding_context"]
            self.assertIn(
                "vc5:cabout-source:source:refreshed-about.cpp",
                context["source_binding"]["registration_ids"],
            )
            self.assertEqual("authored", context["target"]["pipeline_class"])
            self.assertEqual("existing-symbol", context["creation_mode"])
            self.assertEqual(before, progress.read_bytes())

            applied = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=refresh_payload(),
                expected_revision=8,
                apply=True,
                bindings=current_bindings,
            )
            self.assertTrue(applied["commit"]["applied"])
            stored = read_progress(progress)
            binding = stored["symbols"][DATA_TARGET_ID]["relocation_target_binding"]
            self.assertEqual(DATA_OBJECT, binding["object_symbol"])
            self.assertIn(
                "vc5:cabout-source:source:refreshed-about.cpp",
                binding["binding_context"]["source_binding"]["registration_ids"],
            )
            _normalized, stale = relocation_target_binding_staleness(
                binding,
                document=ProgressDocument(stored),
                bindings=current_bindings,
                target_symbol_id=DATA_TARGET_ID,
                reference=REFERENCE,
            )
            self.assertEqual([], stale)

    def test_refresh_rejects_nonstale_prior_and_any_object_identity_change(self) -> None:
        with self.assertRaisesRegex(
            RelocationTargetMutationError,
            "refresh_only must be true",
        ):
            normalize_reviewed_target_request(
                refresh_payload(selector_updates={"refresh_only": False})
            )

        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            self.create_data_binding(progress)
            before = progress.read_bytes()
            with self.assertRaisesRegex(
                RelocationTargetMutationError,
                "requires an existing stale",
            ):
                bind_relocation_target(
                    progress=progress,
                    reference=REFERENCE,
                    manifest_dir=Path("unused"),
                    source_symbol_id=SOURCE_ID,
                    source_address="0x401000",
                    payload=refresh_payload(),
                    expected_revision=8,
                    apply=False,
                    bindings=bindings(),
                )
            with self.assertRaisesRegex(
                RelocationTargetMutationError,
                "must preserve the prior target object symbol",
            ):
                bind_relocation_target(
                    progress=progress,
                    reference=REFERENCE,
                    manifest_dir=Path("unused"),
                    source_symbol_id=SOURCE_ID,
                    source_address="0x401000",
                    payload=refresh_payload(
                        target_object_symbol=CORRECTED_DATA_OBJECT,
                    ),
                    expected_revision=8,
                    apply=False,
                    bindings=bindings(),
                )
            self.assertEqual(before, progress.read_bytes())

    def test_refresh_still_requires_exactly_one_selected_stale_prior(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            self.create_data_binding(progress)
            base = read_progress(progress)
            base["symbols"][DATA_TARGET_ID]["pipeline_class"] = "authored"
            binding = base["symbols"][DATA_TARGET_ID]["relocation_target_binding"]
            cases = {
                "absent": {
                    **deepcopy(base),
                    "symbols": {
                        **deepcopy(base["symbols"]),
                        DATA_TARGET_ID: {
                            key: value
                            for key, value in deepcopy(
                                base["symbols"][DATA_TARGET_ID]
                            ).items()
                            if key != "relocation_target_binding"
                        },
                    },
                },
                "multiple": {
                    **deepcopy(base),
                    "symbols": {
                        **deepcopy(base["symbols"]),
                        DATA_TARGET_ID: {
                            **deepcopy(base["symbols"][DATA_TARGET_ID]),
                            "relocation_target_binding": [
                                binding,
                                deepcopy(binding),
                            ],
                        },
                    },
                },
            }
            for name, data in cases.items():
                with self.subTest(name=name):
                    write_progress(progress, data)
                    before = progress.read_bytes()
                    with self.assertRaisesRegex(
                        RelocationTargetMutationError,
                        "resolves to .* existing reviewed bindings; expected exactly one",
                    ):
                        bind_relocation_target(
                            progress=progress,
                            reference=REFERENCE,
                            manifest_dir=Path("unused"),
                            source_symbol_id=SOURCE_ID,
                            source_address="0x401000",
                            payload=refresh_payload(),
                            expected_revision=8,
                            apply=False,
                            bindings=bindings(),
                        )
                    self.assertEqual(before, progress.read_bytes())

    def test_ordinary_bind_cannot_rewrite_an_existing_reviewed_identity(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            self.create_data_binding(progress)
            payload = correction_payload()
            payload.pop("correction")
            with self.assertRaisesRegex(
                RelocationTargetMutationError, "does not match current exact tracker identity"
            ):
                bind_relocation_target(
                    progress=progress,
                    reference=REFERENCE,
                    manifest_dir=Path("unused"),
                    source_symbol_id=SOURCE_ID,
                    source_address="0x401000",
                    payload=payload,
                    expected_revision=8,
                    apply=False,
                    bindings=bindings(),
                )
            stored = read_progress(progress)
            self.assertEqual(
                DATA_OBJECT,
                stored["symbols"][DATA_TARGET_ID]["relocation_target_binding"]["object_symbol"],
            )

    def test_correction_requires_complete_exact_prior_identity_and_site_selector(self) -> None:
        required = (
            "prior_target_symbol_id",
            "prior_target_object_symbol",
            "source_symbol_id",
            "source_address",
            "source_object_symbol",
            "offset",
        )
        for field in required:
            with self.subTest(field=field):
                payload = correction_payload()
                del payload["correction"][field]
                with self.assertRaisesRegex(
                    RelocationTargetMutationError, "exact prior identity/site selection"
                ):
                    normalize_reviewed_target_request(payload)

        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            self.create_data_binding(progress)
            mismatches: dict[str, object] = {
                "prior_target_symbol_id": "recoil:data:wrong",
                "source_symbol_id": "recoil:function:wrong",
                "source_address": "0x401004",
                "source_object_symbol": "wrong-source-object",
                "offset": 18,
            }
            for field, value in mismatches.items():
                with self.subTest(mismatched_field=field), self.assertRaisesRegex(
                    RelocationTargetMutationError,
                    "correction selector does not match the current command/site",
                ):
                    bind_relocation_target(
                        progress=progress,
                        reference=REFERENCE,
                        manifest_dir=Path("unused"),
                        source_symbol_id=SOURCE_ID,
                        source_address="0x401000",
                        payload=correction_payload(selector_updates={field: value}),
                        expected_revision=8,
                        apply=False,
                        bindings=bindings(),
                    )

    def test_correction_fails_closed_for_absent_multiple_and_mismatched_prior_binding(self) -> None:
        with TemporaryDirectory() as temp:
            progress = self.write_fixture(Path(temp))
            self.create_data_binding(progress)
            base = read_progress(progress)
            cases: list[tuple[str, dict[str, object], dict[str, object]]] = []

            absent = deepcopy(base)
            absent["symbols"][DATA_TARGET_ID].pop("relocation_target_binding")
            cases.append(("absent", absent, correction_payload()))

            multiple = deepcopy(base)
            binding = multiple["symbols"][DATA_TARGET_ID]["relocation_target_binding"]
            multiple["symbols"][DATA_TARGET_ID]["relocation_target_binding"] = [
                binding,
                deepcopy(binding),
            ]
            cases.append(("multiple", multiple, correction_payload()))

            cases.append(
                (
                    "mismatched",
                    deepcopy(base),
                    correction_payload(
                        selector_updates={"prior_target_object_symbol": "wrong-prior-symbol"}
                    ),
                )
            )

            for name, data, payload in cases:
                with self.subTest(name=name):
                    write_progress(progress, data)
                    before = progress.read_bytes()
                    with self.assertRaisesRegex(
                        RelocationTargetMutationError,
                        "resolves to .* existing reviewed bindings; expected exactly one",
                    ):
                        bind_relocation_target(
                            progress=progress,
                            reference=REFERENCE,
                            manifest_dir=Path("unused"),
                            source_symbol_id=SOURCE_ID,
                            source_address="0x401000",
                            payload=payload,
                            expected_revision=8,
                            apply=False,
                            bindings=bindings(),
                        )
                    self.assertEqual(before, progress.read_bytes())

    def test_correction_fails_closed_for_source_target_owner_and_retail_drift(self) -> None:
        with TemporaryDirectory() as temp:
            root = Path(temp)
            progress = self.write_fixture(root)
            self.create_data_binding(progress)
            base = read_progress(progress)

            drift_cases: list[tuple[str, dict[str, object], dict[str, list[SimpleNamespace]]]] = []
            source = deepcopy(base)
            source_bindings = {
                SOURCE_ID: [target_binding(SOURCE_OBJECT, source_from="drifted-about.cpp")]
            }
            drift_cases.append(("source-registration", source, source_bindings))

            extent = deepcopy(base)
            extent["symbols"][DATA_TARGET_ID]["end_exclusive"] = "0x4cc814"
            extent["symbols"][DATA_TARGET_ID]["size"] = 220
            drift_cases.append(("target-extent", extent, bindings()))

            owner = deepcopy(base)
            owner["owners"][DATA_OWNER]["relationships"][1]["name"] = "drifted-name"
            drift_cases.append(("owner-relationship", owner, bindings()))

            owner_state = deepcopy(base)
            owner_state["owners"][DATA_OWNER]["provider_state"] = "accepted"
            drift_cases.append(("owner-state", owner_state, bindings()))

            for name, data, current_bindings in drift_cases:
                with self.subTest(name=name):
                    write_progress(progress, data)
                    before = progress.read_bytes()
                    with self.assertRaises(RelocationTargetMutationError):
                        bind_relocation_target(
                            progress=progress,
                            reference=REFERENCE,
                            manifest_dir=Path("unused"),
                            source_symbol_id=SOURCE_ID,
                            source_address="0x401000",
                            payload=correction_payload(),
                            expected_revision=8,
                            apply=False,
                            bindings=current_bindings,
                        )
                    self.assertEqual(before, progress.read_bytes())

            write_progress(progress, base)
            image = bytearray(REFERENCE.read_bytes())
            headers = parse_pe_headers(image, source=str(REFERENCE))
            operand_offset = rva_to_offset(0x1000 + 17, headers.sections)
            self.assertIsNotNone(operand_offset)
            image[operand_offset] ^= 1
            drifted_reference = root / "drifted-Recoil.exe"
            drifted_reference.write_bytes(image)
            before = progress.read_bytes()
            with self.assertRaises(RelocationTargetMutationError):
                bind_relocation_target(
                    progress=progress,
                    reference=drifted_reference,
                    manifest_dir=Path("unused"),
                    source_symbol_id=SOURCE_ID,
                    source_address="0x401000",
                    payload=correction_payload(),
                    expected_revision=8,
                    apply=False,
                    bindings=bindings(),
                )
            self.assertEqual(before, progress.read_bytes())

    def test_provider_function_and_primary_data_views_select_exact_callable_target(self) -> None:
        with TemporaryDirectory() as temp:
            root = Path(temp)
            data = fixture_data()
            function = data["symbols"][PROVIDER_TARGET_ID]
            function.update(
                kind="provider-function",
                end_exclusive="0x4c5b65",
                size=1,
                object_symbol=PROVIDER_OBJECT,
                disposition="provider",
                pipeline_class="non-authored",
                authored_order_role="non-authored",
                ownership_state="primary-owned",
            )
            provider_data_id = "recoil:data:0x4c5b64"
            data["symbols"][provider_data_id] = {
                "binary": "recoil",
                "kind": "data",
                "address": "0x4c5b64",
                "end_exclusive": "0x4c5b68",
                "extent_state": "known",
                "size": 4,
                "navigation_name": "unit provider IAT storage",
                "output_section_id": "recoil:section:.text",
                "ownership_state": "primary-owned",
                "disposition": "provider",
            }
            data["owners"][PROVIDER_OWNER]["relationships"] = [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                },
                {
                    "kind": "primary-data",
                    "address": "0x4c5b64",
                    "symbol_id": provider_data_id,
                    "name": "unit provider IAT storage",
                },
            ]
            progress = root / "progress.sqlite3"
            write_progress(progress, data)

            report = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=Path("unused"),
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=provider_payload(),
                expected_revision=7,
                apply=False,
                bindings=bindings(),
            )

            self.assertEqual(PROVIDER_TARGET_ID, report["target_symbol_id"])
            self.assertEqual("provider-function", report["binding"]["binding_context"]["target"]["kind"])
            self.assertIsNone(report["binding"]["binding_context"]["relationship"])

    def test_provider_function_and_data_overlap_rejects_unlinked_data_view(self) -> None:
        with TemporaryDirectory() as temp:
            root = Path(temp)
            data = fixture_data()
            function = data["symbols"][PROVIDER_TARGET_ID]
            function.update(
                kind="provider-function",
                end_exclusive="0x4c5b65",
                size=1,
                object_symbol=PROVIDER_OBJECT,
                disposition="provider",
                pipeline_class="non-authored",
                authored_order_role="non-authored",
                ownership_state="primary-owned",
            )
            data["symbols"]["recoil:data:0x4c5b64"] = {
                "binary": "recoil",
                "kind": "data",
                "address": "0x4c5b64",
                "end_exclusive": "0x4c5b68",
                "extent_state": "known",
                "size": 4,
            }
            data["owners"][PROVIDER_OWNER]["relationships"] = [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                }
            ]
            progress = root / "progress.sqlite3"
            write_progress(progress, data)

            with self.assertRaisesRegex(
                RelocationTargetMutationError,
                "co-addressed provider data is not exactly linked",
            ):
                bind_relocation_target(
                    progress=progress,
                    reference=REFERENCE,
                    manifest_dir=Path("unused"),
                    source_symbol_id=SOURCE_ID,
                    source_address="0x401000",
                    payload=provider_payload(),
                    expected_revision=7,
                    apply=False,
                    bindings=bindings(),
                )

    def test_provider_object_proof_authorizes_only_relocation_masked_retail_match(self) -> None:
        proof_symbol = "?ProviderObjectProof@@YGXXZ"
        with TemporaryDirectory() as temp:
            root = Path(temp)
            vc5_root = root / "VC5SP3"
            relative_object = "VC/CRT/SRC/INTEL/XST_LIB/PROOF.OBJ"
            object_path = vc5_root / Path(*Path(relative_object).parts)
            retail = immutable_retail_bytes(0x4C5B64, 0x4C5B6A)
            object_body = b"\x00\x00\x00\x00" + retail[4:] + b"\x90\x90"
            write_coff_object(
                object_path,
                symbol=proof_symbol,
                body=object_body,
                relocations=((0, 0x0006),),
            )
            data = fixture_data()
            target = data["symbols"][PROVIDER_TARGET_ID]
            target["navigation_name"] = "unregistered provider navigation label"
            data["owners"][PROVIDER_OWNER]["relationships"] = [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                }
            ]
            progress = root / "progress.sqlite3"
            write_progress(progress, data)
            before = progress.read_bytes()
            report = bind_relocation_target(
                progress=progress,
                reference=REFERENCE,
                manifest_dir=root / "manifests",
                source_symbol_id=SOURCE_ID,
                source_address="0x401000",
                payload=provider_payload(
                    target_object_symbol=proof_symbol,
                    provider_object_proof={"object_path": relative_object},
                ),
                expected_revision=7,
                apply=False,
                bindings=bindings(),
                vc5_root=vc5_root,
            )

            proof = report["provider_object_proof"]
            self.assertEqual("passed", proof["result"])
            self.assertTrue(proof["candidate_independent"])
            self.assertEqual(relative_object, proof["object_path"])
            self.assertEqual(proof_symbol, proof["object_symbol"])
            self.assertEqual(6, proof["body_size"])
            self.assertEqual(1, proof["relocation_count"])
            self.assertEqual(4, proof["masked_byte_count"])
            self.assertEqual(2, proof["unmasked_byte_count"])
            self.assertEqual(
                proof_symbol,
                report["binding"]["object_symbol"],
            )
            self.assertNotIn(
                "provider_object_proof",
                report["binding"],
            )
            self.assertEqual(before, progress.read_bytes())

    def test_provider_object_proof_rejection_matrix_is_fail_closed(self) -> None:
        proof_symbol = "?ProviderObjectProof@@YGXXZ"
        with TemporaryDirectory() as temp:
            root = Path(temp)
            vc5_root = root / "VC5SP3"
            relative_object = "VC/CRT/SRC/INTEL/XST_LIB/PROOF.OBJ"
            object_path = vc5_root / Path(*Path(relative_object).parts)
            data = fixture_data()
            data["symbols"][PROVIDER_TARGET_ID][
                "navigation_name"
            ] = "unregistered provider navigation label"
            data["owners"][PROVIDER_OWNER]["relationships"] = [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                }
            ]
            progress = root / "progress.sqlite3"
            write_progress(progress, data)
            before = progress.read_bytes()
            retail = immutable_retail_bytes(0x4C5B64, 0x4C5B6A)
            cases = [
                (
                    "malformed",
                    lambda: (
                        object_path.parent.mkdir(parents=True, exist_ok=True),
                        object_path.write_bytes(b"not coff"),
                    ),
                    "malformed",
                ),
                (
                    "missing-symbol",
                    lambda: write_coff_object(
                        object_path,
                        symbol="?DifferentProvider@@YGXXZ",
                        body=retail,
                    ),
                    "0 matching symbols",
                ),
                (
                    "wrong-machine",
                    lambda: write_coff_object(
                        object_path,
                        symbol=proof_symbol,
                        body=retail,
                        machine=0x8664,
                    ),
                    "IMAGE_FILE_MACHINE_I386",
                ),
                (
                    "duplicate-symbol",
                    lambda: write_coff_object(
                        object_path,
                        symbol=proof_symbol,
                        body=retail,
                        duplicate_count=2,
                    ),
                    "2 matching symbols",
                ),
                (
                    "non-external",
                    lambda: write_coff_object(
                        object_path,
                        symbol=proof_symbol,
                        body=retail,
                        storage_class=3,
                    ),
                    "not external",
                ),
                (
                    "non-function",
                    lambda: write_coff_object(
                        object_path,
                        symbol=proof_symbol,
                        body=retail,
                        symbol_type=0,
                    ),
                    "not a function symbol",
                ),
                (
                    "non-code",
                    lambda: write_coff_object(
                        object_path,
                        symbol=proof_symbol,
                        body=retail,
                        section_characteristics=0x40,
                    ),
                    "not in a code section",
                ),
                (
                    "extent-mismatch",
                    lambda: write_coff_object(
                        object_path,
                        symbol=proof_symbol,
                        body=retail[:-1],
                    ),
                    "extent mismatch",
                ),
                (
                    "relocation-overlap",
                    lambda: write_coff_object(
                        object_path,
                        symbol=proof_symbol,
                        body=retail,
                        relocations=((0, 0x0006), (2, 0x0006)),
                    ),
                    "overlapping COFF relocation fields",
                ),
                (
                    "byte-mismatch",
                    lambda: write_coff_object(
                        object_path,
                        symbol=proof_symbol,
                        body=bytes([retail[0] ^ 0xFF]) + retail[1:],
                    ),
                    "byte mismatch outside COFF relocation fields",
                ),
            ]
            for label, prepare, message in cases:
                with self.subTest(label=label):
                    prepare()
                    with self.assertRaisesRegex(
                        RelocationTargetMutationError,
                        message,
                    ):
                        bind_relocation_target(
                            progress=progress,
                            reference=REFERENCE,
                            manifest_dir=root / "manifests",
                            source_symbol_id=SOURCE_ID,
                            source_address="0x401000",
                            payload=provider_payload(
                                target_object_symbol=proof_symbol,
                                provider_object_proof={"object_path": relative_object},
                            ),
                            expected_revision=7,
                            apply=False,
                            bindings=bindings(),
                            vc5_root=vc5_root,
                        )
                    self.assertEqual(before, progress.read_bytes())

    def test_provider_object_proof_rejects_scope_path_and_candidate_shortcuts(self) -> None:
        proof_symbol = "?ProviderObjectProof@@YGXXZ"
        with self.assertRaisesRegex(
            RelocationTargetMutationError,
            "candidate-derived",
        ):
            normalize_reviewed_target_request(
                provider_payload(
                    target_object_symbol=proof_symbol,
                    provider_object_proof={
                        "object_path": "VC/CRT/PROOF.OBJ",
                        "candidate_bytes": "90",
                    },
                )
            )
        for object_path in (
            "../PROOF.OBJ",
            "VC\\CRT\\PROOF.OBJ",
            "C:/VC5SP3/PROOF.OBJ",
            "VC/./CRT/PROOF.OBJ",
        ):
            with self.subTest(object_path=object_path), self.assertRaisesRegex(
                RelocationTargetMutationError,
                "normalized",
            ):
                normalize_reviewed_target_request(
                    provider_payload(
                        target_object_symbol=proof_symbol,
                        provider_object_proof={"object_path": object_path},
                    )
                )

        with TemporaryDirectory() as temp:
            root = Path(temp)
            vc5_root = root / "VC5SP3"
            relative_object = "VC/CRT/PROOF.OBJ"
            object_path = vc5_root / "VC" / "CRT" / "PROOF.OBJ"
            write_coff_object(
                object_path,
                symbol=proof_symbol,
                body=immutable_retail_bytes(0x4C5B64, 0x4C5B6A),
            )
            base = fixture_data()
            base["symbols"][PROVIDER_TARGET_ID][
                "navigation_name"
            ] = "unregistered provider navigation label"
            proof_payload = provider_payload(
                target_object_symbol=proof_symbol,
                provider_object_proof={"object_path": relative_object},
            )
            scoped_cases = [
                (
                    "missing-owner-relationship",
                    {
                        **deepcopy(base),
                        "owners": {
                            **deepcopy(base["owners"]),
                            PROVIDER_OWNER: {
                                **deepcopy(base["owners"][PROVIDER_OWNER]),
                                "relationships": [],
                            },
                        },
                    },
                    proof_payload,
                    "primary-function relationship",
                ),
                (
                    "unaccepted-provider",
                    {
                        **deepcopy(base),
                        "owners": {
                            **deepcopy(base["owners"]),
                            PROVIDER_OWNER: {
                                **deepcopy(base["owners"][PROVIDER_OWNER]),
                                "provider_state": "pending",
                                "relationships": [
                                    {
                                        "kind": "primary-function",
                                        "address": "0x4c5b64",
                                        "symbol_id": PROVIDER_TARGET_ID,
                                    }
                                ],
                            },
                        },
                    },
                    proof_payload,
                    "accepted provider-boundary",
                ),
            ]
            nonprovider = deepcopy(base)
            nonprovider["owners"][DATA_OWNER]["relationships"].append(
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                }
            )
            scoped_cases.append(
                (
                    "non-provider-owner",
                    nonprovider,
                    provider_payload(
                        target_object_symbol=proof_symbol,
                        target_owner_id=DATA_OWNER,
                        evidence_ids=[DATA_EVIDENCE],
                        provider_object_proof={"object_path": relative_object},
                    ),
                    "provider-boundary owner",
                )
            )
            registered = deepcopy(base)
            registered["symbols"][PROVIDER_TARGET_ID]["navigation_name"] = proof_symbol
            registered["owners"][PROVIDER_OWNER]["relationships"] = [
                {
                    "kind": "primary-function",
                    "address": "0x4c5b64",
                    "symbol_id": PROVIDER_TARGET_ID,
                }
            ]
            scoped_cases.append(
                (
                    "already-registered",
                    registered,
                    proof_payload,
                    "previously unregistered",
                )
            )
            for label, data, payload, message in scoped_cases:
                with self.subTest(label=label):
                    progress = root / f"{label}.sqlite3"
                    write_progress(progress, data)
                    before = progress.read_bytes()
                    with self.assertRaisesRegex(
                        RelocationTargetMutationError,
                        message,
                    ):
                        bind_relocation_target(
                            progress=progress,
                            reference=REFERENCE,
                            manifest_dir=root / "manifests",
                            source_symbol_id=SOURCE_ID,
                            source_address="0x401000",
                            payload=payload,
                            expected_revision=7,
                            apply=False,
                            bindings=bindings(),
                            vc5_root=vc5_root,
                        )
                    self.assertEqual(before, progress.read_bytes())


if __name__ == "__main__":
    unittest.main()
