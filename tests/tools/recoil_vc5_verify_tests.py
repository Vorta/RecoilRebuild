import contextlib
from copy import deepcopy
from dataclasses import replace
import io
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands import vc5_verify as vc5_verify_module  # noqa: E402
from _recoil.commands.asm_verify import IMAGE_SCN_CNT_CODE, ObjectByteComparison  # noqa: E402
from _recoil.commands.vc5_verify import (  # noqa: E402
    CompiledTranslationUnit,
    CompiledTarget,
    FunctionOrderCheck,
    FunctionOrderRow,
    LinkedFunctionInterval,
    SourceEmissionWarning,
    VerificationResult,
    VerifyDataSymbol,
    VerifyFunction,
    VerifySelection,
    VerifyTarget,
    _live_order_sequences,
    build_compile_command,
    binja_binary_selector,
    check_function_order,
    check_translation_unit_function_order,
    compare_compiled_selections,
    compare_compiled_selections_in_chunks,
    covering_data_symbols,
    covering_targets,
    effective_source_compile_context,
    find_target,
    function_order_first_divergence,
    function_tracker_identity,
    function_authored_order_gate,
    function_authored_relative_order_gate,
    group_selections_by_compile_key,
    load_manifest,
    load_manifests,
    live_linked_feedback_result,
    main as vc5_main,
    manifest_skeleton,
    parse_cod_listing_label_index,
    parse_targets_json,
    print_function_order_check,
    print_live_order_result,
    print_missing_explanation,
    resolve_coff_symbol_regex,
    resolve_owner_vc5_scope,
    run_vc5_script,
    run_profile_sweep,
    run_target,
    run_translation_unit_function_order_checks,
    selected_targets_for_all,
    selected_targets_for_source_from,
    target_compile_key,
    with_compiler_profile_override,
    translation_unit_order_report_data,
    translation_unit_source_path,
    validate_generated_source_emission_policy,
)
from _recoil.lib.owner_entries import OwnerEntryIndex  # noqa: E402
from _recoil.lib.progress import ProgressError, empty_progress_document  # noqa: E402
from _recoil.lib.repository_paths import load_repository_path_inventory  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402
from _recoil.lib.source_owners import SourceOwnerDocument  # noqa: E402
from _recoil.lib.tooling import CommandScriptResult  # noqa: E402
from _recoil.lib.verification_targets import vc5_target_registration  # noqa: E402
from tests.tools.owner_fixture import owner_record, write_ledger  # noqa: E402


def replace_function(target: VerifyTarget, address: str, name: str) -> VerifyFunction:
    return VerifyFunction(address=address, symbol=f"?{name}@@YAHXZ", name=name)


def write_manifest(directory: Path, name: str = "sample") -> Path:
    source_path = directory / "sample.cpp"
    source_path.write_text(
        "/**\n"
        " * Reimplements 0x401000: Sample.\n"
        " * Purpose: Exercises VC5 verification provenance detection.\n"
        " */\n"
        "int __cdecl Sample() { return 1; }\n",
        encoding="utf-8",
    )
    path = directory / f"{name}.json"
    path.write_text(
        json.dumps(
            {
                "name": name,
                "description": "sample target",
                "source_filename": "sample_verify.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP", "/O2", "/FAcs"],
                "include_dirs": ["src"],
                "source_files": ["src/sample.cpp"],
                "functions": [
                    {
                        "address": "0x00401000",
                        "symbol": "?Sample@@YAHXZ",
                        "name": "Sample",
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    return path


def mark_manifest_function_authored(path: Path) -> None:
    data = json.loads(path.read_text(encoding="utf-8"))
    data["functions"][0]["pipeline_class"] = "authored"
    path.write_text(json.dumps(data), encoding="utf-8")


def write_generated_role_manifest(
    directory: Path,
    *,
    include_anchor: bool,
    anchor_kind: str = "type-definition",
    role: str = "compiler-generated-deleting-variant",
    marker: str = "Emits 0x401000: VC5 scalar deleting destructor for Sample.",
) -> Path:
    source_dir = directory / "src"
    source_dir.mkdir(exist_ok=True)
    source_path = source_dir / "sample.cpp"
    source_path.write_text('#include "sample.h"\n', encoding="utf-8")
    marker_block = marker if marker.lstrip().startswith("/**") else f"/** {marker} */"
    (source_dir / "sample.h").write_text(
        f"{marker_block}\nclass Sample {{ public: virtual ~Sample(); }};\n",
        encoding="utf-8",
    )
    function: dict[str, object] = {
        "address": "0x401000",
        "symbol": "??_GSample@@UAEPAXI@Z",
        "name": "Sample scalar deleting destructor",
        "pipeline_class": "authored-lifecycle",
        "authored_order_role": role,
        "provenance": "compiler-emitted-noncovering",
    }
    if include_anchor:
        function["emission_anchor"] = {
            "path": "src/sample.h",
            "kind": anchor_kind,
            "name": "Sample",
        }
    path = directory / "generated.json"
    path.write_text(
        json.dumps(
            {
                "name": "generated",
                "description": "generated contribution",
                "source_filename": "sample.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP"],
                "include_dirs": ["src"],
                "source_files": ["src/sample.cpp"],
                "functions": [function],
            }
        ),
        encoding="utf-8",
    )
    return path


def canonical_generated_marker(anchor_id: str = "sample.type") -> str:
    return (
        "/**\n"
        f" * @recoil-anchor recoil:anchor:{anchor_id}\n"
        " * @recoil-artifact emits .text recoil:function:0x401000: "
        "VC5 scalar deleting destructor for Sample.\n"
        " */"
    )


def write_data_thunk_manifest(
    directory: Path,
    *,
    relation: str = "emits",
    section: str = ".text",
    include_canonical_artifact: bool = True,
    duplicate_artifact: bool = False,
    detached_artifact: bool = False,
) -> Path:
    source_dir = directory / "src"
    source_dir.mkdir(exist_ok=True)
    source_path = source_dir / "sample.cpp"
    directive = (
        f" * @recoil-artifact {relation} {section} recoil:function:0x401000: "
        "VC5 static-lifetime thunk for g_Sample.\n"
        if include_canonical_artifact
        else " * Emits 0x401000: Legacy static-lifetime thunk inventory.\n"
    )
    marker = (
        "/**\n"
        " * @recoil-anchor recoil:anchor:sample.data\n"
        f"{directive}"
        " */\n"
    )
    if detached_artifact:
        source_text = "struct Sample {};\nSample g_Sample;\n" + marker
    else:
        source_text = "struct Sample {};\n" + marker + "Sample g_Sample;\n"
    if duplicate_artifact:
        source_text = '#include "duplicate.h"\n' + source_text
        (source_dir / "duplicate.h").write_text(
            "/**\n"
            " * @recoil-anchor recoil:anchor:sample.duplicate\n"
            " * @recoil-artifact emits .text recoil:function:0x401000: "
            "Duplicate static-lifetime thunk claim.\n"
            " */\n"
            "int g_DuplicateSample;\n",
            encoding="utf-8",
        )
    source_path.write_text(source_text, encoding="utf-8")
    path = directory / "data_thunk.json"
    path.write_text(
        json.dumps(
            {
                "name": "data_thunk",
                "description": "data-defined compiler thunk",
                "source_filename": "sample.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP"],
                "include_dirs": ["src"],
                "source_files": ["src/sample.cpp"],
                "functions": [
                    {
                        "address": "0x401000",
                        "symbol": "_$E1",
                        "name": "g_Sample static-lifetime thunk",
                        "pipeline_class": "authored-lifecycle",
                        "authored_order_role": "compiler-generated-thunk",
                        "provenance": "compiler-emitted-noncovering",
                        "emission_anchor": {
                            "path": "src/sample.cpp",
                            "kind": "data-definition",
                            "name": "g_Sample",
                        },
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    return path


def write_data_manifest(directory: Path, name: str = "sample_data") -> Path:
    source_path = directory / "sample.cpp"
    source_path.write_text(
        "struct SampleWithTable {\n"
        "    virtual int Value();\n"
        "};\n"
        "int SampleWithTable::Value() { return 1; }\n",
        encoding="utf-8",
    )
    path = directory / f"{name}.json"
    path.write_text(
        json.dumps(
            {
                "name": name,
                "description": "sample data target",
                "source_filename": "sample_verify.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP", "/O2", "/FAcs"],
                "include_dirs": ["src"],
                "source_files": ["src/sample.cpp"],
                "data_symbols": [
                    {
                        "address": "0x00402000",
                        "symbol": "??_7SampleWithTable@@6B@",
                        "name": "g_SampleWithTable_FTable",
                        "bn_name": "g_SampleWithTable_FTable",
                        "byte_length": 4,
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    return path


def write_data_progress_fixture(directory: Path) -> Path:
    path = directory / "fixture-progress.sqlite3"
    data = empty_progress_document()
    data["symbols"] = {
        "recoil:data:0x402000": {
            "binary": "recoil",
            "kind": "data",
            "address": "0x402000",
            "end_exclusive": "0x402004",
            "size": 4,
            "extent_state": "known",
            "output_section_id": "recoil:section:.rdata",
            "ownership_state": "primary-owned",
        }
    }
    data["output_sections"] = {
        "recoil:section:.rdata": {
            "binary": "recoil",
            "name": ".rdata",
        }
    }
    ProgressSQLiteStore.create_from_mapping(
        path,
        data,
        cutover_pair_id="vc5-verify-test",
    )
    return path


def write_address_manifest(directory: Path, name: str, address: str, function_name: str) -> Path:
    normalized_address = address.replace("0x00", "0x")
    source_path = directory / f"{name}.cpp"
    source_path.write_text(
        "/**\n"
        f" * Reimplements {normalized_address}: {function_name}.\n"
        " * Purpose: Exercises VC5 verification batch selection.\n"
        " */\n"
        f"int __cdecl {function_name}() {{ return 1; }}\n",
        encoding="utf-8",
    )
    path = directory / f"{name}.json"
    path.write_text(
        json.dumps(
            {
                "name": name,
                "description": f"{name} target",
                "source_filename": f"{name}_verify.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP", "/O2", "/FAcs"],
                "include_dirs": ["src"],
                "source_files": [f"src/{name}.cpp"],
                "functions": [
                    {
                        "address": address,
                        "symbol": f"?{function_name}@@YAHXZ",
                        "name": function_name,
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    return path


def linked_feedback_targets() -> tuple[VerifyTarget, VerifyTarget]:
    functions = tuple(
        VerifyFunction(address=address, symbol=symbol, name=name, pipeline_class=kind)
        for address, symbol, name, kind in (
            ("0x401000", "??0CAboutDlg@@QAE@PAVCWnd@@@Z", "CAboutDlg::CAboutDlg", "authored-lifecycle"),
            ("0x401020", "?DoDataExchange@CWnd@@MAEXPAVCDataExchange@@@Z", "CWnd::DoDataExchange", "non-authored"),
            ("0x401030", "?GetMessageMap@CAboutDlg@@MBEPBUAFX_MSGMAP@@XZ", "CAboutDlg::GetMessageMap", "authored"),
            ("0x401040", "?BeginModalState@CWnd@@UAEXXZ", "CWnd::BeginModalState", "non-authored"),
            ("0x401050", "?EndModalState@CWnd@@UAEXXZ", "CWnd::EndModalState", "non-authored"),
        )
    )
    common = {
        "description": "CAbout linked feedback fixture",
        "source_filename": "about.cpp",
        "source_text": "",
        "source_from": "src/Battlesport/about.cpp",
        "compare_mode": "coff_bytes",
        "trim_trailing_nops": True,
        "compiler_profile": "",
        "compiler_env": "",
        "compiler_flags": (),
        "include_dirs": (),
        "source_files": ("src/Battlesport/about.cpp",),
        "generated_files": (),
        "data_symbols": (),
        "target_binary": "recoil",
        "retail_start": "0x401000",
        "retail_end_exclusive": "0x401060",
    }
    object_target = VerifyTarget(
        name="cabout_prelude_provider_order_current_shape",
        functions=functions,
        manifest_path=Path("tools/vc5_verify_targets/cabout_prelude_provider_order_current_shape.json"),
        check_function_order=True,
        function_order_scope="authored",
        compile_context_from="tools/_recoil/config/vc5_final_build.json",
        **common,
    )
    interval = LinkedFunctionInterval(
        name="cabout_retail_interval",
        predecessor=None,
        functions=functions,
        successor=VerifyFunction(
            address="0x401060",
            symbol="?TickAiMode2TopLevel@AINet@@SIXPAUzUtil_SaveGameState@@@Z",
            name="AINet::TickAiMode2TopLevel",
            pipeline_class="authored",
        ),
        order_scope="full",
        retail_start="0x401000",
        retail_end_exclusive="0x401060",
    )
    linked_target = VerifyTarget(
        name="cabout_retail_interval_linked_order",
        functions=(),
        manifest_path=Path("tools/vc5_verify_targets/cabout_retail_interval_linked_order.json"),
        linked_function_intervals=(interval,),
        **common,
    )
    return object_target, linked_target


def write_owner_ledger(
    path: Path,
    addresses: list[str],
    *,
    data: bool = False,
    provider: bool = False,
) -> None:
    address_metadata = {
        address: {
            "name": f"g_TestItem{index}" if data else f"TestItem{index}",
            "target": "test_owner_data" if data else "test_owner",
            **({"section": ".data", "size": 4, "type": "int"} if data else {}),
        }
        for index, address in enumerate(addresses)
    }
    write_ledger(
        path,
        owner_record(
            "test.owner",
            kind="provider-boundary" if provider else "data-owner" if data else "class",
            anchors=(addresses[0],) if addresses else (),
            functions=() if data else addresses,
            data=((address, f"data_{address[2:]}") for address in addresses) if data else (),
            tiers=None if provider else {address: "B" for address in addresses},
            gates={
                "boundary": "accepted",
                "source": "accepted",
                "data": "accepted" if data else "none",
                "functional": "accepted",
                "linkage": "accepted",
                "byte": "deferred",
            },
            name="Test Owner",
            section="test",
            source_paths=() if provider else ("src/Test.cpp",),
            address_metadata=address_metadata,
            blocker="byte verification pending",
        ),
    )


def write_inline_manifest(directory: Path, name: str = "sample_inline") -> Path:
    path = directory / f"{name}.json"
    path.write_text(
        json.dumps(
            {
                "name": name,
                "description": "sample inline target",
                "source_filename": "sample_inline.cpp",
                "compiler_flags": ["/nologo", "/TP", "/O2", "/FAcs"],
                "include_dirs": [],
                "source_files": ["src/sample.cpp"],
                "functions": [
                    {
                        "address": "0x00401000",
                        "symbol": "?Sample@@YAHXZ",
                        "name": "Sample",
                    }
                ],
                "source": ["int __cdecl Sample() { return 1; }"],
            }
        ),
        encoding="utf-8",
    )
    return path


def write_source_profile_order_manifest(
    directory: Path,
    *,
    name: str = "source_profiles",
    compile_context_from: str = "",
    source_compile_profiles: dict[str, str] | None = None,
) -> Path:
    hud_source = "src/Battlesport/hud.cpp"
    about_source = "src/Battlesport/about.cpp"
    data = {
        "name": name,
        "description": "per-source compiler-profile test target",
        "source_filename": "source_profiles_verify.cpp",
        "compiler_profile": "vc5_o2_ob0_md_gx_facs",
        "include_dirs": [],
        "source_files": [hud_source, about_source],
        "check_translation_unit_function_order": True,
        "translation_unit_function_order": [
            {
                "source_from": hud_source,
                "functions": [
                    {
                        "address": "0x401000",
                        "symbol": "?HudSourceProfileProbe@@YAXXZ",
                        "name": "HudSourceProfileProbe",
                    }
                ],
            },
            {
                "source_from": about_source,
                "functions": [
                    {
                        "address": "0x401010",
                        "symbol": "?AboutSourceProfileProbe@@YAXXZ",
                        "name": "AboutSourceProfileProbe",
                    }
                ],
            },
        ],
    }
    if compile_context_from:
        data.pop("compiler_profile")
        data["compile_context_from"] = compile_context_from
    if source_compile_profiles is not None:
        data["source_compile_profiles"] = source_compile_profiles
    path = directory / f"{name}.json"
    path.write_text(json.dumps(data), encoding="utf-8")
    return path


def add_about_scoped_profile_guard(path: Path) -> Path:
    data = json.loads(path.read_text(encoding="utf-8"))
    data["profile_guard"] = {
        "scope": "src/Battlesport/about.cpp",
        "accepted_profiles": ["vc5_o2_ob0_md_gx_facs"],
        "disqualified_profiles": ["vc5_o2_ob1_md_gx_facs"],
    }
    path.write_text(json.dumps(data), encoding="utf-8")
    return path


def write_translation_unit_icf_source_policy_manifest(
    directory: Path,
    *,
    icf_fold_status: str | None,
) -> Path:
    source_path = directory / "logical_alias.cpp"
    source_path.write_text(
        "void LogicalAlias() {}\n",
        encoding="utf-8",
    )
    path = write_manifest(directory)
    data = json.loads(path.read_text(encoding="utf-8"))
    function = {
        "address": "0x401020",
        "symbol": "?LogicalAlias@@YAXXZ",
        "name": "LogicalAlias",
        "pipeline_class": "authored",
        "authored_order_role": "authored-body",
    }
    if icf_fold_status is not None:
        function.update(
            {
                "full_order_gate": False,
                "logical_identity_key": "recoil:logical-function:0x401020:logical-alias",
                "icf_fold_status": icf_fold_status,
            }
        )
    data["check_translation_unit_function_order"] = True
    data["translation_unit_function_order"] = [
        {
            "source_from": str(source_path),
            "order_scope": "authored",
            "functions": [function],
        }
    ]
    path.write_text(json.dumps(data), encoding="utf-8")
    return path


class RecoilVc5VerifyTests(unittest.TestCase):
    def test_canonical_tracker_consumers_read_sqlite_default_progress(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.sqlite3"
            data = empty_progress_document()
            data["revision"] = 17
            data["symbols"]["recoil:function:0x401000"] = {
                "kind": "function",
                "pipeline_class": "authored",
                "output_section_id": "recoil:section:.text",
            }
            ProgressSQLiteStore.create_from_mapping(
                path,
                data,
                cutover_pair_id="vc5-verify-test",
            )
            vc5_verify_module.canonical_tracker_data.cache_clear()
            vc5_verify_module.canonical_tracker_artifact_index.cache_clear()
            try:
                with patch.object(vc5_verify_module, "DEFAULT_PROGRESS_PATH", path):
                    tracker_data = vc5_verify_module.canonical_tracker_data()
                    artifact_index = vc5_verify_module.canonical_tracker_artifact_index()
            finally:
                vc5_verify_module.canonical_tracker_data.cache_clear()
                vc5_verify_module.canonical_tracker_artifact_index.cache_clear()

        self.assertEqual(17, tracker_data["revision"])
        self.assertEqual(
            "recoil:function:0x401000",
            artifact_index.resolve("recoil:function:0x401000").physical_id,
        )

    def test_top_level_physical_authored_icf_function_uses_logical_mirrors(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            source_path = Path(data["source_from"])
            alias_id = "recoil:logical-function:0x401000:member-a"
            source_path.write_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:member-a\n"
                f" * @recoil-artifact defines .text {alias_id}: Member A.\n"
                " */\n"
                "void MemberA() {}\n",
                encoding="utf-8",
            )
            data["functions"][0].update(
                {
                    "pipeline_class": "authored",
                    "authored_order_role": "authored-body",
                }
            )
            path.write_text(json.dumps(data), encoding="utf-8")

            with (
                patch(
                    "_recoil.commands.vc5_verify.canonical_tracker_data_for_manifest",
                    return_value={"symbols": {}},
                ),
                patch(
                    "_recoil.commands.vc5_verify."
                    "validate_authored_icf_physical_source_artifacts",
                    return_value=(alias_id,),
                ) as validator,
            ):
                target = load_manifest(path, strict_source_traceability=True)

            self.assertEqual("0x401000", target.functions[0].address)
            self.assertEqual("", target.functions[0].logical_identity_key)
            validator.assert_called_once()
            self.assertEqual(
                "recoil:function:0x401000",
                validator.call_args.kwargs["physical_symbol_id"],
            )
            self.assertTrue(
                validator.call_args.kwargs["select_single_logical_member"]
            )

            with (
                patch(
                    "_recoil.commands.vc5_verify.canonical_tracker_data_for_manifest",
                    return_value={"symbols": {}},
                ),
                patch(
                    "_recoil.commands.vc5_verify."
                    "validate_authored_icf_physical_source_artifacts",
                    side_effect=ProgressError("logical mirror is stale"),
                ),
                self.assertRaisesRegex(
                    ValueError,
                    "function physical authored-ICF source mirrors are invalid",
                ),
            ):
                load_manifest(path, strict_source_traceability=True)

    def test_physical_authored_icf_order_row_uses_reviewed_logical_mirrors(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            source_path = Path(data["source_from"])
            alias_a = "recoil:logical-function:0x401000:member-a"
            alias_b = "recoil:logical-function:0x401000:member-b"
            source_path.write_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:member-a\n"
                f" * @recoil-artifact defines .text {alias_a}: Member A.\n"
                " */\n"
                "void MemberA() {}\n"
                "/**\n"
                " * @recoil-anchor recoil:anchor:member-b\n"
                f" * @recoil-artifact defines .text {alias_b}: Member B.\n"
                " */\n"
                "void MemberB() {}\n",
                encoding="utf-8",
            )
            data["functions"] = []
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": str(source_path),
                    "order_scope": "authored",
                    "functions": [
                        {
                            "address": "0x401000",
                            "symbol": "?PhysicalGate@@YAXXZ",
                            "name": "PhysicalGate",
                            "pipeline_class": "authored",
                            "authored_order_role": "authored-body",
                        }
                    ],
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            with (
                patch(
                    "_recoil.commands.vc5_verify.canonical_tracker_data_for_manifest",
                    return_value={"symbols": {}},
                ),
                patch(
                    "_recoil.commands.vc5_verify."
                    "validate_authored_icf_physical_source_artifacts",
                    return_value=(alias_a, alias_b),
                ) as validator,
            ):
                target = load_manifest(path, strict_source_traceability=True)

            function = target.translation_unit_function_order[0].functions[0]
            self.assertEqual("0x401000", function.address)
            self.assertEqual("", function.logical_identity_key)
            validator.assert_called_once()
            self.assertEqual(
                "recoil:function:0x401000",
                validator.call_args.kwargs["physical_symbol_id"],
            )
            self.assertFalse(
                validator.call_args.kwargs["select_single_logical_member"]
            )

            with (
                patch(
                    "_recoil.commands.vc5_verify.canonical_tracker_data_for_manifest",
                    return_value={"symbols": {}},
                ),
                patch(
                    "_recoil.commands.vc5_verify."
                    "validate_authored_icf_physical_source_artifacts",
                    side_effect=ProgressError("group evidence is stale"),
                ),
                self.assertRaisesRegex(
                    ValueError,
                    "physical authored-ICF source mirrors are invalid",
                ),
            ):
                load_manifest(path, strict_source_traceability=True)

    def test_authored_icf_order_manifest_rejects_duplicate_physical_gate(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = write_manifest(Path(temporary))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"] = []
            row = {
                "address": "0x401000",
                "symbol": "?PhysicalGate@@YAXXZ",
                "name": "PhysicalGate",
                "pipeline_class": "authored",
                "authored_order_role": "authored-body",
            }
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": data["source_from"],
                    "order_scope": "authored",
                    "functions": [row],
                },
                {
                    "source_from": data["source_from"],
                    "order_scope": "authored",
                    "functions": [row],
                },
            ]
            path.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "duplicate function identity at 0x401000"):
                load_manifest(path, enforce_source_policy=False)

    def test_order_edit_paths_are_normalized_existing_metadata_only(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = write_manifest(Path(temporary))
            baseline = load_manifest(path)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["order_edit_paths"] = [
                "src/Battlesport/ai_net.cpp",
                "src/Battlesport/ai_net.h",
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            target = load_manifest(path)

            self.assertEqual(
                ("src/Battlesport/ai_net.cpp", "src/Battlesport/ai_net.h"),
                target.order_edit_paths,
            )
            self.assertEqual(
                target_compile_key(replace(target, order_edit_paths=()), Path("unused-env.cmd")),
                target_compile_key(target, Path("unused-env.cmd")),
            )
            self.assertEqual(
                baseline,
                replace(target, order_edit_paths=()),
            )

    def test_order_edit_paths_fail_closed_on_invalid_entries(self):
        cases = {
            "not-list": ("src/Battlesport/ai_net.h", "must be a list"),
            "backslash": (["src\\Battlesport\\ai_net.h"], "must use forward slashes"),
            "absolute": ([str(REPO_ROOT / "src/Battlesport/ai_net.h")], "must use forward slashes"),
            "traversal": (["../RecoilRebuild/src/Battlesport/ai_net.h"], "empty, dot, or dot-dot"),
            "missing": (["src/Battlesport/missing_order_header.h"], "outside the authorized repository inventory"),
            "wrong-extension": (["tools/README.md"], "has a disallowed suffix"),
            "duplicate": (
                ["src/Battlesport/ai_net.h", "src/Battlesport/ai_net.h"],
                "duplicate order_edit_paths entry",
            ),
        }
        for name, (order_edit_paths, error) in cases.items():
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temporary:
                path = write_manifest(Path(temporary))
                data = json.loads(path.read_text(encoding="utf-8"))
                data["order_edit_paths"] = order_edit_paths
                path.write_text(json.dumps(data), encoding="utf-8")
                with self.assertRaisesRegex(ValueError, error):
                    load_manifest(path)

    def test_registered_and_unregistered_manifest_paths_are_exact(self):
        inventory = load_repository_path_inventory(REPO_ROOT)
        registered_path = (
            REPO_ROOT
            / "tools/vc5_verify_targets/briefing_text_block_order_current_shape.json"
        )
        registered = load_manifest(
            registered_path,
            enforce_source_policy=False,
            repository_path_inventory=inventory,
            strict_tracked_paths=True,
        )
        self.assertEqual("src/Battlesport/Briefing.cpp", registered.source_from)

        with tempfile.TemporaryDirectory() as temporary:
            wrong_path = Path(temporary) / registered_path.name
            wrong_data = json.loads(registered_path.read_text(encoding="utf-8"))
            wrong_data["source_from"] = "src/Battlesport/briefing.cpp"
            wrong_path.write_text(json.dumps(wrong_data), encoding="utf-8")
            with self.assertRaisesRegex(
                ValueError,
                r"incorrect repository path case.*expected 'src/Battlesport/Briefing\.cpp'",
            ):
                load_manifest(
                    wrong_path,
                    enforce_source_policy=False,
                    repository_path_inventory=inventory,
                    strict_tracked_paths=True,
                )

        historical_path = (
            REPO_ROOT
            / "tools/vc5_verify_targets/"
            "hud_confirmed_layout_runtime_layer_authored_order_current_shape.json"
        )
        historical_repository_path = historical_path.relative_to(REPO_ROOT).as_posix()
        self.assertNotIn(
            historical_repository_path,
            vc5_verify_module.registered_vc5_manifest_paths(),
        )
        historical = load_manifest(
            historical_path,
            enforce_source_policy=False,
            repository_path_inventory=inventory,
            strict_tracked_paths=False,
        )
        self.assertIn("src/GameZRecoil/zSys/zsys.cpp", historical.source_files)
        exact_historical = load_manifest(
            historical_path,
            enforce_source_policy=False,
            repository_path_inventory=inventory,
            strict_tracked_paths=True,
        )
        self.assertEqual(historical.source_files, exact_historical.source_files)

    def test_generated_role_warns_without_emission_anchor_in_compatibility_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_generated_role_manifest(root, include_anchor=False)

            manifest = load_manifest(path)

        self.assertEqual(1, len(manifest.source_emission_warnings))
        self.assertEqual("missing-source-emission-anchor", manifest.source_emission_warnings[0].code)
        self.assertEqual("0x401000", manifest.source_emission_warnings[0].address)
        self.assertFalse(manifest.source_emission_policy_strict)

    def test_authored_function_accepts_canonical_defines_in_strict_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            mark_manifest_function_authored(path)
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:sample.function\n"
                " * @recoil-artifact defines .text recoil:function:0x401000: "
                "Direct authored body.\n"
                " */\n"
                "int __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )

            manifest = load_manifest(path, strict_source_traceability=True)

        self.assertTrue(manifest.source_traceability_policy_strict)

    def test_authored_function_legacy_reimplements_is_nonqualifying_in_strict_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            mark_manifest_function_authored(path)

            with self.assertRaisesRegex(
                ValueError,
                "legacy Reimplements markers are migration inventory only",
            ):
                load_manifest(path, strict_source_traceability=True)

    def test_unresolved_function_cannot_claim_inline_canonical_source_edge(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:sample.unresolved\n"
                " * @recoil-artifact defines .text recoil:function:0x401000: "
                "Premature source claim.\n"
                " */\n"
                "int __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )

            with self.assertRaisesRegex(
                ValueError,
                "cannot claim a canonical authored source edge until its authored identity is resolved",
            ):
                load_manifest(path)

    def test_authored_function_canonical_relation_section_and_id_are_exact(self):
        cases = {
            "relation": (
                "@recoil-artifact emits .text recoil:function:0x401000: Wrong relation.",
                "must use 'defines'",
            ),
            "section": (
                "@recoil-artifact defines .rdata recoil:function:0x401000: Wrong section.",
                "exact output section '.text'",
            ),
            "identity": (
                "@recoil-artifact defines .text messages:function:0x401000: Wrong binary.",
                "requires exact canonical artifact id",
            ),
        }
        for name, (directive, expected) in cases.items():
            with self.subTest(name=name), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                path = write_manifest(root)
                mark_manifest_function_authored(path)
                source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
                source_path.write_text(
                    "/**\n"
                    " * @recoil-anchor recoil:anchor:sample.function\n"
                    f" * {directive}\n"
                    " */\n"
                    "int __cdecl Sample() { return 1; }\n",
                    encoding="utf-8",
                )

                with self.assertRaisesRegex(ValueError, expected):
                    load_manifest(path, strict_source_traceability=True)

    def test_authored_function_canonical_directive_must_be_attached_doxygen(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            mark_manifest_function_authored(path)
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text(
                "// @recoil-anchor recoil:anchor:sample.function\n"
                "// @recoil-artifact defines .text recoil:function:0x401000: "
                "Wrong comment style.\n"
                "int __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "invalid-comment-style"):
                load_manifest(path, strict_source_traceability=True)

    def test_data_symbol_accepts_attached_canonical_defines_in_strict_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_data_manifest(root)
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:sample.table\n"
                " * @recoil-artifact defines .rdata recoil:data:0x402000: "
                "Authored table storage.\n"
                " */\n"
                "const int g_SampleWithTable_FTable = 1;\n",
                encoding="utf-8",
            )
            tracker = write_data_progress_fixture(root)

            with patch("_recoil.commands.vc5_verify.DEFAULT_PROGRESS_PATH", tracker):
                manifest = load_manifest(path, strict_source_traceability=True)

        self.assertEqual("0x402000", manifest.data_symbols[0].address)

    def test_data_symbol_rejects_non_data_output_section_in_strict_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_data_manifest(root)
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:sample.table\n"
                " * @recoil-artifact defines .text recoil:data:0x402000: "
                "Wrong output section.\n"
                " */\n"
                "int g_SampleWithTable_FTable = 1;\n",
                encoding="utf-8",
            )
            tracker = write_data_progress_fixture(root)

            with (
                patch("_recoil.commands.vc5_verify.DEFAULT_PROGRESS_PATH", tracker),
                self.assertRaisesRegex(ValueError, "exact output section"),
            ):
                load_manifest(path, strict_source_traceability=True)

    def test_generated_role_missing_anchor_fails_in_strict_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_generated_role_manifest(root, include_anchor=False)

            with self.assertRaisesRegex(ValueError, "strict source-emission policy"):
                load_manifest(path, strict_source_emissions=True)

    def test_generated_role_canonical_edge_supersedes_legacy_emission_anchor(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_generated_role_manifest(
                root,
                include_anchor=False,
                marker=canonical_generated_marker(),
            )

            with patch("_recoil.commands.vc5_verify.REPO_ROOT", root):
                manifest = load_manifest(path, strict_source_emissions=True)

        self.assertEqual((), manifest.source_emission_warnings)
        self.assertTrue(manifest.source_emission_policy_strict)

    def test_generated_role_accepts_attached_emits_anchor_in_strict_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_generated_role_manifest(
                root,
                include_anchor=True,
                marker=canonical_generated_marker(),
            )

            with patch("_recoil.commands.vc5_verify.REPO_ROOT", root):
                manifest = load_manifest(path, strict_source_emissions=True)

        self.assertEqual((), manifest.source_emission_warnings)
        self.assertTrue(manifest.source_emission_policy_strict)
        self.assertEqual("src/sample.h", manifest.functions[0].emission_anchor.path)

    def test_generated_thunk_accepts_data_definition_with_exact_canonical_emits(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_data_thunk_manifest(root)

            with patch("_recoil.commands.vc5_verify.REPO_ROOT", root):
                manifest = load_manifest(
                    path,
                    strict_source_emissions=True,
                    strict_source_traceability=True,
                )

        self.assertEqual((), manifest.source_emission_warnings)
        self.assertEqual("data-definition", manifest.functions[0].emission_anchor.kind)

    def test_generated_thunk_preserves_direct_function_definition_with_canonical_defines(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0].update(
                {
                    "pipeline_class": "authored-lifecycle",
                    "authored_order_role": "compiler-generated-thunk",
                    "provenance": "compiler-emitted-noncovering",
                    "emission_anchor": {
                        "path": "sample.cpp",
                        "kind": "function-definition",
                        "name": "Sample",
                    },
                }
            )
            path.write_text(json.dumps(data), encoding="utf-8")
            (root / "sample.cpp").write_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:sample.function\n"
                " * @recoil-artifact defines .text recoil:function:0x401000: "
                "Direct authored thunk body.\n"
                " */\n"
                "int __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )

            with patch("_recoil.commands.vc5_verify.REPO_ROOT", root):
                manifest = load_manifest(path, strict_source_traceability=True)

        self.assertEqual("function-definition", manifest.functions[0].emission_anchor.kind)

    def test_generated_thunk_data_anchor_requires_emits_relation_and_text_section(self):
        cases = {
            "defines": (
                {"relation": "defines"},
                "wrong-relation: 'defines' function artifact is attached to data definition",
            ),
            "wrong-section": ({"section": ".rdata"}, "exact output section '.text'"),
        }
        for label, (kwargs, expected) in cases.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                path = write_data_thunk_manifest(root, **kwargs)

                with (
                    patch("_recoil.commands.vc5_verify.REPO_ROOT", root),
                    self.assertRaisesRegex(ValueError, expected),
                ):
                    load_manifest(path, strict_source_traceability=True)

    def test_generated_thunk_data_anchor_requires_exact_kind_path_and_name(self):
        cases = {
            "kind": (
                lambda data: data["functions"][0]["emission_anchor"].__setitem__(
                    "kind", "function-definition"
                ),
                "not emission_anchor.kind 'function-definition'",
            ),
            "path": (
                lambda data: data["functions"][0]["emission_anchor"].__setitem__(
                    "path", "src/other.h"
                ),
                "not emission_anchor.path 'src/other.h'",
            ),
            "name": (
                lambda data: data["functions"][0]["emission_anchor"].__setitem__(
                    "name", "g_Other"
                ),
                "not emission_anchor.name 'g_Other'",
            ),
        }
        for label, (mutate, expected) in cases.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                path = write_data_thunk_manifest(root)
                (root / "src" / "other.h").write_text(
                    "/** Purpose: alternate data definition. */\nint g_Other;\n",
                    encoding="utf-8",
                )
                source_path = root / "src" / "sample.cpp"
                source_path.write_text(
                    '#include "other.h"\n' + source_path.read_text(encoding="utf-8"),
                    encoding="utf-8",
                )
                data = json.loads(path.read_text(encoding="utf-8"))
                mutate(data)
                path.write_text(json.dumps(data), encoding="utf-8")

                with (
                    patch("_recoil.commands.vc5_verify.REPO_ROOT", root),
                    self.assertRaisesRegex(ValueError, expected),
                ):
                    load_manifest(path, strict_source_traceability=True)

    def test_generated_thunk_data_anchor_rejects_missing_duplicate_or_detached_artifact(self):
        cases = {
            "missing": (
                {"include_canonical_artifact": False},
                "uses legacy Emits inventory",
            ),
            "duplicate": (
                {"duplicate_artifact": True},
                "duplicate-artifact-id|occur exactly once",
            ),
            "detached": (
                {"detached_artifact": True},
                "eof-anchor: canonical source-trace directives must be immediately attached",
            ),
        }
        for label, (kwargs, expected) in cases.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                path = write_data_thunk_manifest(root, **kwargs)

                with (
                    patch("_recoil.commands.vc5_verify.REPO_ROOT", root),
                    self.assertRaisesRegex(ValueError, expected),
                ):
                    load_manifest(path, strict_source_traceability=True)

    def test_generated_role_accepts_attached_anchor_without_legacy_emits_in_strict_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_generated_role_manifest(
                root,
                include_anchor=True,
                marker="Purpose: Declares the class whose virtual lifetime emits the helper.",
            )

            with patch("_recoil.commands.vc5_verify.REPO_ROOT", root):
                manifest = load_manifest(path, strict_source_emissions=True)

        self.assertEqual((), manifest.source_emission_warnings)
        self.assertTrue(manifest.source_emission_policy_strict)

    def test_legacy_generated_emits_remains_valid_for_old_strict_emission_alias(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_generated_role_manifest(root, include_anchor=True)

            with patch("_recoil.commands.vc5_verify.REPO_ROOT", root):
                manifest = load_manifest(path, strict_source_emissions=True)

        self.assertEqual((), manifest.source_emission_warnings)
        self.assertTrue(manifest.source_emission_policy_strict)

    def test_generated_role_anchor_name_must_resolve_without_legacy_emits(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_generated_role_manifest(
                root,
                include_anchor=True,
                marker="Purpose: Declares the class whose virtual lifetime emits the helper.",
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0]["emission_anchor"]["name"] = "MissingSample"
            path.write_text(json.dumps(data), encoding="utf-8")

            with patch("_recoil.commands.vc5_verify.REPO_ROOT", root):
                with self.assertRaisesRegex(ValueError, "must resolve exactly once"):
                    load_manifest(path)

    def test_generated_role_rejects_wrong_anchor_kind(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_generated_role_manifest(
                Path(tmp), include_anchor=True, anchor_kind="function-definition"
            )

            with self.assertRaisesRegex(ValueError, "requires emission_anchor.kind"):
                load_manifest(path, enforce_source_policy=False)

    def test_non_generated_role_rejects_emission_anchor(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0]["emission_anchor"] = {
                "path": "src/sample.h",
                "kind": "function-definition",
                "name": "Sample",
            }
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "supported only for compiler-generated"):
                load_manifest(path, enforce_source_policy=False)

    def test_function_address_refs_preserve_emission_anchor(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_generated_role_manifest(
                root,
                include_anchor=True,
                marker=canonical_generated_marker(),
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": data["source_from"],
                    "function_address_refs": ["0x401000"],
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            with patch("_recoil.commands.vc5_verify.REPO_ROOT", root):
                manifest = load_manifest(path, strict_source_emissions=True)

        referenced = manifest.translation_unit_function_order[0].functions[0]
        self.assertEqual(manifest.functions[0].emission_anchor, referenced.emission_anchor)
        self.assertEqual("compiler-emitted-noncovering", referenced.provenance)

    def test_function_address_refs_preserve_symbol_regex_with_representative_symbol(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            representative_symbol = data["functions"][0]["symbol"]
            symbol_regex = r"\?Sample@[0-9]+@@YAHXZ"
            data["functions"][0]["symbol_regex"] = symbol_regex
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": data["source_from"],
                    "function_address_refs": ["0x401000"],
                }
            ]
            data["linked_function_intervals"] = [
                {
                    "name": "sample_interval",
                    "predecessor": {
                        "address": "0x400ff0",
                        "symbol": "?Before@@YAHXZ",
                        "name": "Before",
                    },
                    "function_address_refs": ["0x401000"],
                    "successor": {
                        "address": "0x401010",
                        "symbol": "?After@@YAHXZ",
                        "name": "After",
                    },
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path, enforce_source_policy=False)

        referenced = manifest.translation_unit_function_order[0].functions[0]
        linked = manifest.linked_function_intervals[0].functions[0]
        self.assertEqual(representative_symbol, referenced.symbol)
        self.assertEqual(symbol_regex, referenced.symbol_regex)
        self.assertEqual(representative_symbol, linked.symbol)
        self.assertEqual(symbol_regex, linked.symbol_regex)

        fresh_symbol = "?Sample@3253710299@@YAHXZ"
        defined_symbol = SimpleNamespace(name=fresh_symbol, section_number=3, value=0x20)

        class FakeCoff:
            symbols = (defined_symbol,)
            symbols_by_name = {fresh_symbol: defined_symbol}

            def section(self, section_number):
                if section_number == 3:
                    return SimpleNamespace(name="SECT34B", characteristics=IMAGE_SCN_CNT_CODE)
                raise ValueError(f"bad section {section_number}")

        order = check_function_order(
            target=manifest,
            functions=(referenced,),
            coff_object=FakeCoff(),
        )
        self.assertTrue(order.ok)
        self.assertEqual(fresh_symbol, order.rows[0].symbol)

    def test_function_address_ref_source_emission_warnings_are_deduplicated(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_generated_role_manifest(root, include_anchor=False)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": data["source_from"],
                    "function_address_refs": ["0x401000"],
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual(1, len(manifest.source_emission_warnings))

    def test_generated_role_anchor_kind_matrix(self):
        allowed = {
            "compiler-generated-deleting-variant": {"type-definition"},
            "compiler-generated-eh-helper": {
                "function-definition",
                "data-definition",
                "type-definition",
            },
            "compiler-generated-thunk": {
                "function-definition",
                "data-definition",
                "type-definition",
            },
            "compiler-generated-implicit-cleanup": {
                "function-definition",
                "data-definition",
                "type-definition",
            },
            "compiler-generated-icf-representative": {"function-definition"},
        }
        all_kinds = {"type-definition", "function-definition", "data-definition"}
        for role, allowed_kinds in allowed.items():
            for kind in all_kinds:
                with self.subTest(role=role, kind=kind):
                    with tempfile.TemporaryDirectory() as tmp:
                        path = write_generated_role_manifest(
                            Path(tmp), include_anchor=True, anchor_kind=kind, role=role
                        )
                        if kind in allowed_kinds:
                            manifest = load_manifest(path, enforce_source_policy=False)
                            self.assertEqual(kind, manifest.functions[0].emission_anchor.kind)
                        else:
                            with self.assertRaisesRegex(ValueError, "requires emission_anchor.kind"):
                                load_manifest(path, enforce_source_policy=False)

    def test_strict_generated_emission_treats_exact_unresolved_as_debt(self):
        function = VerifyFunction(
            address="0x401000",
            symbol="??_GSample@@UAEPAXI@Z",
            name="SampleDeletingDestructor",
            pipeline_class="authored-lifecycle",
            authored_order_role="compiler-generated-deleting-variant",
        )
        warning = validate_generated_source_emission_policy(
            function=function,
            source_from="src/sample.cpp",
            manifest_path=Path("sample.json"),
            context="function",
            strict_source_emissions=True,
            strict_source_traceability=False,
            target_binary="recoil",
            trace_documents=(),
            tracker_source_trace_state="unresolved",
        )

        self.assertIsNotNone(warning)
        self.assertEqual("missing-source-emission-anchor", warning.code)
        for state in (None, "resolved", "not-applicable"):
            with self.subTest(state=state):
                with self.assertRaisesRegex(
                    ValueError, "strict source-emission policy"
                ):
                    validate_generated_source_emission_policy(
                        function=function,
                        source_from="src/sample.cpp",
                        manifest_path=Path("sample.json"),
                        context="function",
                        strict_source_emissions=True,
                        strict_source_traceability=False,
                        target_binary="recoil",
                        trace_documents=(),
                        tracker_source_trace_state=state,
                    )

    def test_production_source_policy_text_is_cached_per_source_root(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "targets"
            manifest_dir.mkdir()
            source_path = root / "sample.cpp"
            source_path.write_text(
                "int __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )
            first_manifest = manifest_dir / "first.json"
            second_manifest = manifest_dir / "second.json"
            original_loader = (
                vc5_verify_module._source_from_policy_text_uncached
            )
            vc5_verify_module._canonical_source_from_policy_text.cache_clear()
            with (
                patch.object(
                    vc5_verify_module,
                    "DEFAULT_MANIFEST_DIR",
                    manifest_dir,
                ),
                patch.object(
                    vc5_verify_module,
                    "_source_from_policy_text_uncached",
                    wraps=original_loader,
                ) as source_loader,
            ):
                first = vc5_verify_module.source_from_policy_text(
                    str(source_path),
                    first_manifest,
                )
                second = vc5_verify_module.source_from_policy_text(
                    str(source_path),
                    second_manifest,
                )
            vc5_verify_module._canonical_source_from_policy_text.cache_clear()

        self.assertEqual(first, second)
        self.assertEqual(1, source_loader.call_count)

    def test_cli_strict_source_emissions_applies_to_selected_target(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            valid_path = write_generated_role_manifest(
                root,
                include_anchor=True,
                marker=canonical_generated_marker(),
            )
            manifest_dir = root / "tools" / "vc5_verify_targets"
            manifest_dir.mkdir(parents=True)
            valid_path = valid_path.replace(manifest_dir / valid_path.name)
            missing_data = json.loads(valid_path.read_text(encoding="utf-8"))
            missing_data["name"] = "missing_generated"
            missing_data["functions"][0].pop("emission_anchor")
            (manifest_dir / "missing.json").write_text(json.dumps(missing_data), encoding="utf-8")

            stderr = io.StringIO()
            with (
                patch("_recoil.commands.vc5_verify.REPO_ROOT", root),
                patch("_recoil.commands.vc5_verify.run_target", return_value=0) as runner,
                contextlib.redirect_stderr(stderr),
            ):
                result = vc5_main(
                    [
                        "generated",
                        "--manifest-dir",
                        str(manifest_dir),
                        "--strict-source-emissions",
                    ]
                )

        self.assertEqual(0, result, stderr.getvalue())
        self.assertTrue(runner.call_args.kwargs["target"].source_emission_policy_strict)

    def test_cli_strict_source_traceability_revalidates_only_selected_target(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            mark_manifest_function_authored(path)
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:sample.cli\n"
                " * @recoil-artifact defines .text recoil:function:0x401000: "
                "Selected CLI body.\n"
                " */\n"
                "int __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )

            with (
                patch("_recoil.commands.vc5_verify.run_target", return_value=0) as runner,
                contextlib.redirect_stderr(io.StringIO()),
            ):
                result = vc5_main(
                    [
                        "sample",
                        "--manifest-dir",
                        str(root),
                        "--strict-source-traceability",
                    ]
                )

        self.assertEqual(0, result)
        self.assertTrue(
            runner.call_args.kwargs["target"].source_traceability_policy_strict
        )

    def test_authored_order_role_defers_compiler_helper_but_keeps_full_gate(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_function_order"] = True
            data["function_order_scope"] = "authored"
            data["functions"][0].update(
                {
                    "pipeline_class": "authored-lifecycle",
                    "authored_order_role": "compiler-generated-deleting-variant",
                    "required_presence": True,
                    "full_order_gate": True,
                }
            )
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": data["source_from"],
                    "order_scope": "authored",
                    "function_address_refs": ["0x401000"],
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")
            target = load_manifest(path, enforce_source_policy=False)

        function = target.functions[0]
        self.assertEqual("compiler-generated-deleting-variant", function.authored_order_role)
        self.assertFalse(function_authored_order_gate(function))
        self.assertTrue(function.required_presence)
        self.assertTrue(function.full_order_gate)
        self.assertEqual("authored", target.function_order_scope)
        self.assertFalse(function_authored_order_gate(target.translation_unit_function_order[0].functions[0]))

    def test_compiler_generated_authored_order_role_cannot_disable_full_obligation(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0].update(
                {
                    "pipeline_class": "authored-lifecycle",
                    "authored_order_role": "compiler-generated-eh-helper",
                    "full_order_gate": False,
                }
            )
            path.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "must keep required_presence=true and full_order_gate=true"):
                load_manifest(path, enforce_source_policy=False)

    def test_current_briefing_manifest_can_defer_three_scalar_helpers_without_manifest_write(self):
        source_manifest = REPO_ROOT / "tools" / "vc5_verify_targets" / "briefing_text_block_order_current_shape.json"
        data = json.loads(source_manifest.read_text(encoding="utf-8"))
        deferred = {
            "0x403d70",
            "0x403d90",
            "0x403eb0",
        }
        for function in data["functions"]:
            if function["address"] in deferred:
                function["authored_order_role"] = "compiler-generated-deleting-variant"
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / source_manifest.name
            path.write_text(json.dumps(data), encoding="utf-8")
            target = load_manifest(path, enforce_source_policy=False)

        rows = {function.address: function for function in target.functions}
        self.assertEqual("authored", target.function_order_scope)
        self.assertEqual(deferred, {address for address in deferred if not function_authored_order_gate(rows[address])})
        self.assertTrue(all(rows[address].required_presence and rows[address].full_order_gate for address in deferred))

    def test_present_registered_vc5_manifest_paths_use_exact_repository_spelling(self):
        progress_path = REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
        tracker = ProgressSQLiteStore(progress_path, read_only=True).materialize()
        registered_manifest_paths = sorted(
            row["registration"]["manifest_path"]
            for row in tracker["verification_targets"].values()
            if isinstance(row, dict)
            and row.get("kind") == "vc5"
            and isinstance(row.get("registration"), dict)
            and isinstance(row["registration"].get("manifest_path"), str)
        )
        self.assertEqual(1497, len(registered_manifest_paths))

        inventory = load_repository_path_inventory(REPO_ROOT)
        repository_exact = set(inventory.exact_paths)
        repository_by_casefold = inventory.casefolded_paths

        absent = {
            path
            for path in registered_manifest_paths
            if path not in repository_exact
        }
        self.assertEqual(
            {
                "tools/vc5_verify_targets/hud_cmd_binding_destroy_range.json",
                "tools/vc5_verify_targets/hud_cmd_binding_vector_helpers.json",
                "tools/vc5_verify_targets/hud_ui_panel_ptr_vector_erase_range.json",
                "tools/vc5_verify_targets/hud_ui_panel_ptr_vector_insert_n.json",
                "tools/vc5_verify_targets/zsnd_set_use_archive_banks.json",
                "tools/vc5_verify_targets/"
                "zsnd_set_use_archive_banks_and_register_at_exit.json",
            },
            absent,
        )

        def string_values(value: object):
            if isinstance(value, str):
                yield value
            elif isinstance(value, list):
                for item in value:
                    yield from string_values(item)
            elif isinstance(value, dict):
                for item in value.values():
                    yield from string_values(item)

        case_mismatches: list[tuple[str, str, str]] = []
        present_count = 0
        for manifest_path in registered_manifest_paths:
            if manifest_path in absent:
                continue
            present_count += 1
            manifest = json.loads((REPO_ROOT / manifest_path).read_text(encoding="utf-8"))
            for value in string_values(manifest):
                matches = repository_by_casefold.get(value.casefold(), ())
                if len(matches) == 1 and value != matches[0]:
                    case_mismatches.append((manifest_path, value, matches[0]))

        self.assertEqual(1491, present_count)
        self.assertEqual([], case_mismatches)

    def test_current_map_shutdown_thunk_registration_is_ordinary_authored(self):
        path = (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "map_text_block_order_current_shape.json"
        )
        target = load_manifest(path, enforce_source_policy=False)
        rows = {
            function.address: function
            for contribution in target.translation_unit_function_order
            for function in contribution.functions
        }
        thunk = rows["0x416790"]
        self.assertEqual("authored", thunk.pipeline_class)
        self.assertEqual("authored-body", thunk.authored_order_role)
        self.assertTrue(function_authored_order_gate(thunk))

        _target_id, record = vc5_target_registration(path)
        registered_rows = {
            function["address"]: function
            for contribution in record["registration"][
                "translation_unit_function_order"
            ]
            for function in contribution["functions"]
        }
        self.assertEqual(
            ("authored", "authored-body"),
            (
                registered_rows["0x416790"]["pipeline_class"],
                registered_rows["0x416790"]["authored_order_role"],
            ),
        )

    def test_current_mission_queue_enter_registration_is_static_fastcall(self):
        path = (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "mission_417350_41cc10_authored_order.json"
        )
        expected_symbol = (
            "?QueueEnterWithReconfigureFlag@"
            "HudUiNetGameSetupOverlayOwner@@SIXH@Z"
        )
        target = load_manifest(path, enforce_source_policy=False)
        target_rows = [
            function
            for contribution in target.translation_unit_function_order
            for function in contribution.functions
            if function.address == "0x41ad80"
        ] + [
            function
            for interval in target.linked_function_intervals
            for function in interval.functions
            if function.address == "0x41ad80"
        ]
        self.assertEqual(2, len(target_rows))
        self.assertTrue(
            all(function.symbol == expected_symbol for function in target_rows)
        )

        _target_id, record = vc5_target_registration(path)
        registration = record["registration"]
        registered_rows = [
            function
            for contribution in registration[
                "translation_unit_function_order"
            ]
            for function in contribution["functions"]
            if function["address"] == "0x41ad80"
        ] + [
            function
            for interval in registration["linked_function_intervals"]
            for function in interval["functions"]
            if function["address"] == "0x41ad80"
        ]
        self.assertEqual(2, len(registered_rows))
        self.assertTrue(
            all(function["symbol"] == expected_symbol for function in registered_rows)
        )

    def test_load_manifest_normalizes_function_addresses(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = load_manifest(write_manifest(Path(tmp)))

        self.assertEqual("sample", manifest.name)
        self.assertEqual("sample_verify.cpp", manifest.source_filename)
        self.assertTrue(manifest.source_from.endswith("sample.cpp"))
        self.assertEqual("coff_bytes", manifest.compare_mode)
        self.assertTrue(manifest.trim_trailing_nops)
        self.assertEqual("", manifest.compiler_env)
        self.assertEqual(("src/sample.cpp",), manifest.source_files)
        self.assertEqual((), manifest.generated_files)
        self.assertEqual("0x401000", manifest.functions[0].address)
        self.assertIsNone(manifest.functions[0].vc5_byte_length)
        self.assertEqual((), manifest.data_symbols)
        self.assertEqual("", manifest.source_text)
        self.assertFalse(manifest.check_function_order)

    def test_load_manifest_accepts_function_order_check_flag(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_function_order"] = True
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertTrue(manifest.check_function_order)

    def test_load_manifest_accepts_translation_unit_function_order(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            first_source = root / "first.cpp"
            first_source.write_text(
                "/**\n"
                " * Reimplements 0x401000: First.\n"
                " */\n"
                "int __cdecl First() { return 1; }\n",
                encoding="utf-8",
            )
            second_source = root / "second.cpp"
            second_source.write_text(
                "/**\n"
                " * Reimplements 0x401010: Second.\n"
                " */\n"
                "int __cdecl Second() { return 2; }\n",
                encoding="utf-8",
            )
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": str(first_source),
                    "functions": [
                        {
                            "address": "0x401000",
                            "symbol": "?First@@YAHXZ",
                            "name": "First",
                        }
                    ],
                },
                {
                    "source_from": str(second_source),
                    "functions": [
                        {
                            "address": "0x401010",
                            "symbol": "?Second@@YAHXZ",
                            "name": "Second",
                            "source_order_gate": False,
                        }
                    ],
                },
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertTrue(manifest.check_translation_unit_function_order)
        self.assertEqual(2, len(manifest.translation_unit_function_order))
        self.assertEqual(str(first_source), manifest.translation_unit_function_order[0].source_from)
        self.assertEqual("0x401010", manifest.translation_unit_function_order[1].functions[0].address)
        self.assertFalse(manifest.translation_unit_function_order[1].functions[0].source_order_gate)

    def test_translation_unit_inventory_only_schema_is_explicit_and_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": data["source_from"],
                    "functions": [
                        {
                            "address": "0x401000",
                            "symbol": "?Sample@@YAHXZ",
                            "name": "Sample",
                        }
                    ],
                },
                {
                    "source_from": data["source_from"],
                    "inventory_only": True,
                    "functions": [],
                },
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path, enforce_source_policy=False)
            _target_id, registration = vc5_target_registration(path)

            self.assertFalse(manifest.translation_unit_function_order[0].inventory_only)
            self.assertTrue(manifest.translation_unit_function_order[1].inventory_only)
            self.assertEqual((), manifest.translation_unit_function_order[1].functions)
            registered_units = registration["registration"]["translation_unit_function_order"]
            self.assertFalse(registered_units[0]["inventory_only"])
            self.assertTrue(registered_units[1]["inventory_only"])
            self.assertEqual([], registered_units[1]["function_addresses"])

            invalid_cases = (
                (
                    {"source_from": data["source_from"], "functions": []},
                    "functions must not be empty",
                ),
                (
                    {"source_from": data["source_from"], "inventory_only": True},
                    "requires explicit functions: \\[\\]",
                ),
                (
                    {
                        "source_from": data["source_from"],
                        "inventory_only": True,
                        "functions": data["translation_unit_function_order"][0]["functions"],
                    },
                    "requires explicit functions: \\[\\]",
                ),
                (
                    {
                        "source_from": data["source_from"],
                        "inventory_only": True,
                        "functions": [],
                        "function_address_refs": ["0x401000"],
                    },
                    "cannot use function_address_refs",
                ),
            )
            for entry, expected in invalid_cases:
                with self.subTest(entry=entry):
                    invalid = dict(data)
                    invalid["translation_unit_function_order"] = [entry]
                    path.write_text(json.dumps(invalid), encoding="utf-8")
                    with self.assertRaisesRegex(ValueError, expected):
                        load_manifest(path, enforce_source_policy=False)

    def test_load_manifest_rejects_retired_exact_defined_function_set(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [{
                "source_from": data["source_from"],
                "exact_defined_function_set": True,
                "functions": [{"address": "0x401000", "symbol": "?Sample@@YAHXZ", "name": "Sample"}],
            }]
            path.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "exact_defined_function_set is retired"):
                load_manifest(path)

    def test_load_manifest_accepts_linked_function_intervals(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["linked_function_intervals"] = [
                {
                    "name": "sample_interval",
                    "predecessor": {
                        "address": "0x400ff0",
                        "symbol": "?Before@@YAHXZ",
                        "name": "Before",
                    },
                    "functions": [
                        {
                            "address": "0x401000",
                            "symbol": "?Sample@@YAHXZ",
                            "name": "Sample",
                        }
                    ],
                    "successor": {
                        "address": "0x401010",
                        "symbol_regex": r"\?After@@YAHXZ",
                        "name": "After",
                    },
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)
            compile_matches = selected_targets_for_source_from([manifest], manifest.source_from)

        self.assertEqual(1, len(manifest.linked_function_intervals))
        interval = manifest.linked_function_intervals[0]
        self.assertEqual("sample_interval", interval.name)
        self.assertEqual("0x400ff0", interval.predecessor.address)
        self.assertEqual("0x401000", interval.functions[0].address)
        self.assertEqual(r"\?After@@YAHXZ", interval.successor.symbol_regex)
        self.assertEqual([manifest.name], [selection.target.name for selection in compile_matches])

    def test_linked_only_manifest_loads_without_compile_fields_and_is_never_selected_for_vc5_compile(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_path = root / "mission.cpp"
            source_path.write_text("// intentionally has no verification docblocks\n", encoding="utf-8")
            path = root / "linked_only.json"
            path.write_text(
                json.dumps(
                    {
                        "name": "linked_only",
                        "description": "final-link order metadata only",
                        "source_from": str(source_path),
                        "linked_function_intervals": [
                            {
                                "name": "sample",
                                "predecessor": {
                                    "address": "0x400ff0",
                                    "symbol": "?Before@@YAHXZ",
                                    "name": "Before",
                                },
                                "functions": [
                                    {
                                        "address": "0x401000",
                                        "symbol": "?Body@@YAHXZ",
                                        "name": "Body",
                                    }
                                ],
                                "successor": {
                                    "address": "0x401010",
                                    "symbol": "?After@@YAHXZ",
                                    "name": "After",
                                },
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )

            manifests = load_manifests(root)
            manifest = manifests[0]
            source_matches = selected_targets_for_source_from(manifests, str(source_path))
            all_matches = selected_targets_for_all(manifests)

        self.assertEqual("linked_only", manifest.name)
        self.assertEqual("", manifest.source_filename)
        self.assertEqual("", manifest.compiler_profile)
        self.assertEqual((), manifest.compiler_flags)
        self.assertEqual([], source_matches)
        self.assertEqual([], all_matches)
        with self.assertRaisesRegex(ValueError, "linked_function_intervals only"):
            find_target([manifest], "linked_only")

    def test_translation_unit_function_order_unresolved_row_does_not_require_legacy_provenance(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            order_source = root / "order.cpp"
            order_source.write_text("int __cdecl OrderOnly() { return 1; }\n", encoding="utf-8")
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": str(order_source),
                    "functions": [
                        {
                            "address": "0x401020",
                            "symbol": "?OrderOnly@@YAHXZ",
                            "name": "OrderOnly",
                        }
                    ],
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual(
            "unresolved",
            manifest.translation_unit_function_order[0].functions[0].pipeline_class,
        )

    def test_translation_unit_source_policy_accepts_proven_fold_alias_without_representative_docblock(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_translation_unit_icf_source_policy_manifest(
                Path(tmp),
                icf_fold_status="proven-fold-alias",
            )

            manifest = load_manifest(path)

        function = manifest.translation_unit_function_order[0].functions[0]
        self.assertEqual("recoil:logical-function:0x401020:logical-alias", function.logical_identity_key)
        self.assertEqual("proven-fold-alias", function.icf_fold_status)

    def test_translation_unit_source_policy_requires_selected_winner_canonical_edge_only_when_strict(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_translation_unit_icf_source_policy_manifest(
                Path(tmp),
                icf_fold_status="selected-winner",
            )

            load_manifest(path)
            with self.assertRaisesRegex(ValueError, "requires an attached canonical"):
                load_manifest(path, strict_source_traceability=True)

    def test_translation_unit_source_policy_requires_not_established_canonical_edge_only_when_strict(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_translation_unit_icf_source_policy_manifest(
                Path(tmp),
                icf_fold_status="not-established",
            )

            load_manifest(path)
            with self.assertRaisesRegex(ValueError, "requires an attached canonical"):
                load_manifest(path, strict_source_traceability=True)

    def test_translation_unit_source_policy_requires_ordinary_authored_canonical_edge_only_when_strict(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_translation_unit_icf_source_policy_manifest(
                Path(tmp),
                icf_fold_status=None,
            )

            load_manifest(path)
            with self.assertRaisesRegex(ValueError, "requires an attached canonical"):
                load_manifest(path, strict_source_traceability=True)

    def test_manifest_bootstrap_metadata_cannot_bypass_strict_canonical_source_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_translation_unit_icf_source_policy_manifest(
                Path(tmp),
                icf_fold_status=None,
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            data["source_policy_bootstrap"] = {
                "state": "pending-source-placement",
                "registration_only": True,
            }
            path.write_text(json.dumps(data), encoding="utf-8")

            load_manifest(path)
            with self.assertRaisesRegex(ValueError, "requires an attached canonical"):
                load_manifest(path, strict_source_traceability=True)

    def test_translation_unit_function_order_accepts_provider_boundary_provenance(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            provider_source = root / "provider.cpp"
            provider_source.write_text("int __cdecl ProviderPrelude() { return 1; }\n", encoding="utf-8")
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": str(provider_source),
                    "functions": [
                        {
                            "address": "0x401040",
                            "symbol": "?ProviderPrelude@@YAHXZ",
                            "name": "ProviderPrelude",
                            "provenance": "provider-boundary",
                        }
                    ],
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        function = manifest.translation_unit_function_order[0].functions[0]
        self.assertEqual("0x401040", function.address)
        self.assertEqual("?ProviderPrelude@@YAHXZ", function.symbol)
        self.assertEqual("ProviderPrelude", function.name)
        self.assertEqual("provider-boundary", function.provenance)

    def test_translation_unit_function_order_accepts_compiler_emitted_noncovering_provenance(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_path = root / "current_shape.cpp"
            source_path.write_text(
                "void __cdecl AuthoredNeighbor() {}\n",
                encoding="utf-8",
            )
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": str(source_path),
                    "functions": [
                        {
                            "address": "0x401050",
                            "symbol": "??_GCurrentShape@@UAEPAXI@Z",
                            "name": "CurrentShape::ScalarDeletingDestructor",
                            "provenance": "compiler-emitted-noncovering",
                            "listing_label_regex": "\\$L100",
                        }
                    ],
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        function = manifest.translation_unit_function_order[0].functions[0]
        self.assertEqual("0x401050", function.address)
        self.assertEqual("compiler-emitted-noncovering", function.provenance)
        self.assertEqual("\\$L100", function.listing_label_regex)

    def test_top_level_functions_reject_listing_label_regex(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0]["listing_label_regex"] = "\\$L100"
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "listing_label_regex is only supported"):
                load_manifest(path)

    def test_translation_unit_function_order_rejects_unknown_provenance(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            provider_source = root / "provider.cpp"
            provider_source.write_text("int __cdecl ProviderPrelude() { return 1; }\n", encoding="utf-8")
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["check_translation_unit_function_order"] = True
            data["translation_unit_function_order"] = [
                {
                    "source_from": str(provider_source),
                    "functions": [
                        {
                            "address": "0x401040",
                            "symbol": "?ProviderPrelude@@YAHXZ",
                            "name": "ProviderPrelude",
                            "provenance": "provider",
                        }
                    ],
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "unknown function provenance marker 'provider'"):
                load_manifest(path)

    def test_top_level_provider_boundary_without_source_edge_needs_no_legacy_marker(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            source_path = Path(data["source_from"])
            source_path.write_text(
                "int __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )
            data["functions"][0]["provenance"] = "provider-boundary"
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual("provider-boundary", manifest.functions[0].provenance)

    def test_top_level_compiler_emitted_without_source_edge_needs_no_legacy_marker(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            source_path = Path(data["source_from"])
            source_path.write_text(
                "int __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )
            data["functions"][0]["provenance"] = "compiler-emitted-noncovering"
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual(
            "compiler-emitted-noncovering",
            manifest.functions[0].provenance,
        )

    def test_translation_unit_function_order_rejects_entries_without_flag(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            order_source = root / "order.cpp"
            order_source.write_text(
                "/** Reimplements 0x401020: OrderOnly. */\n"
                "int __cdecl OrderOnly() { return 1; }\n",
                encoding="utf-8",
            )
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["translation_unit_function_order"] = [
                {
                    "source_from": str(order_source),
                    "functions": [
                        {
                            "address": "0x401020",
                            "symbol": "?OrderOnly@@YAHXZ",
                            "name": "OrderOnly",
                        }
                    ],
                }
            ]
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "requires 'check_translation_unit_function_order'"):
                load_manifest(path)

    def test_function_order_check_accepts_increasing_coff_order(self):
        target = VerifyTarget(
            name="sample_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
        )
        functions = (
            VerifyFunction(address="0x401000", symbol="?First@@YAHXZ", name="First"),
            VerifyFunction(address="0x401010", symbol="?Second@@YAHXZ", name="Second"),
        )

        class FakeCoff:
            symbols_by_name = {
                "?First@@YAHXZ": SimpleNamespace(section_number=1, value=0),
                "?Second@@YAHXZ": SimpleNamespace(section_number=2, value=0),
            }

            def section(self, _section_number):
                return SimpleNamespace(characteristics=IMAGE_SCN_CNT_CODE)

        result = check_function_order(
            target=target,
            functions=functions,
            coff_object=FakeCoff(),
        )

        self.assertTrue(result.ok)
        self.assertEqual((), result.breaks)
        report = translation_unit_order_report_data(result)
        self.assertEqual(2, report["expected_count"])
        self.assertEqual(2, report["actual_count"])
        self.assertEqual([], report["missing_contributions"])
        self.assertEqual(
            ["?First@@YAHXZ", "?Second@@YAHXZ"],
            [row["decorated_identities"][0] for row in report["actual_contributions"]],
        )

    def test_function_symbol_regex_ignores_undefined_duplicate_for_code_resolution(self):
        symbol_name = "??_GSample@@UAEPAXI@Z"

        class FakeCoff:
            symbols = (
                SimpleNamespace(name=symbol_name, section_number=0, value=0),
                SimpleNamespace(name=symbol_name, section_number=3, value=0x20),
            )

            def section(self, section_number):
                if section_number == 3:
                    return SimpleNamespace(name="SECT34B", characteristics=IMAGE_SCN_CNT_CODE)
                raise ValueError(f"bad section {section_number}")

        resolved = resolve_coff_symbol_regex(
            FakeCoff(),
            r"\?\?_GSample@@UAEPAXI@Z",
            item_label="0x401000",
            require_defined_code=True,
        )

        self.assertEqual(symbol_name, resolved)

    def test_function_symbol_regex_keeps_multiple_defined_matches_ambiguous(self):
        class FakeCoff:
            symbols = (
                SimpleNamespace(name="?SampleA@@YAHXZ", section_number=1, value=0),
                SimpleNamespace(name="?SampleB@@YAHXZ", section_number=2, value=0),
                SimpleNamespace(name="?SampleC@@YAHXZ", section_number=0, value=0),
            )

            def section(self, section_number):
                return SimpleNamespace(name=f"SECT{section_number}", characteristics=IMAGE_SCN_CNT_CODE)

        with self.assertRaisesRegex(ValueError, "defined code matches"):
            resolve_coff_symbol_regex(
                FakeCoff(),
                r"\?Sample[A-C]@@YAHXZ",
                item_label="0x401000",
                require_defined_code=True,
            )

    def test_function_order_symbol_regex_ignores_undefined_duplicate(self):
        symbol_name = "??_GSample@@UAEPAXI@Z"
        target = VerifyTarget(
            name="sample_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
        )
        function = VerifyFunction(
            address="0x401000",
            symbol="",
            symbol_regex=r"\?\?_GSample@@UAEPAXI@Z",
            name="Sample::~Sample",
        )
        defined_symbol = SimpleNamespace(name=symbol_name, section_number=3, value=0x20)

        class FakeCoff:
            symbols = (
                SimpleNamespace(name=symbol_name, section_number=0, value=0),
                defined_symbol,
            )
            symbols_by_name = {symbol_name: defined_symbol}

            def section(self, section_number):
                if section_number == 3:
                    return SimpleNamespace(name="SECT34B", characteristics=IMAGE_SCN_CNT_CODE)
                raise ValueError(f"bad section {section_number}")

        result = check_function_order(
            target=target,
            functions=(function,),
            coff_object=FakeCoff(),
        )

        self.assertTrue(result.ok)
        self.assertEqual(symbol_name, result.rows[0].symbol)
        self.assertEqual(3, result.rows[0].section_number)

    def test_function_order_exact_symbol_ignores_undefined_duplicate(self):
        symbol_name = "??_GSample@@UAEPAXI@Z"
        target = VerifyTarget(
            name="sample_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
        )
        function = VerifyFunction(
            address="0x401000",
            symbol=symbol_name,
            name="Sample::~Sample",
        )
        undefined_symbol = SimpleNamespace(name=symbol_name, section_number=0, value=0)
        defined_symbol = SimpleNamespace(name=symbol_name, section_number=3, value=0x20)

        class FakeCoff:
            symbols = (undefined_symbol, defined_symbol)
            symbols_by_name = {symbol_name: undefined_symbol}

            def section(self, section_number):
                if section_number == 3:
                    return SimpleNamespace(name="SECT34B", characteristics=IMAGE_SCN_CNT_CODE)
                raise ValueError(f"bad section {section_number}")

        result = check_function_order(
            target=target,
            functions=(function,),
            coff_object=FakeCoff(),
        )

        self.assertTrue(result.ok)
        self.assertEqual(symbol_name, result.rows[0].symbol)
        self.assertEqual(3, result.rows[0].section_number)

    def test_function_order_reports_unresolved_undefined_symbol_without_crashing(self):
        symbol_name = "?UndefinedOnly@@YAHXZ"
        target = VerifyTarget(
            name="sample_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
        )
        function = VerifyFunction(address="0x401000", symbol=symbol_name, name="UndefinedOnly")

        class FakeCoff:
            symbols = (SimpleNamespace(name=symbol_name, section_number=0, value=0),)
            symbols_by_name = {symbol_name: symbols[0]}

            def section(self, section_number):
                raise ValueError(f"bad section {section_number}")

        result = check_function_order(
            target=target,
            functions=(function,),
            coff_object=FakeCoff(),
        )

        self.assertFalse(result.ok)
        self.assertEqual((), result.rows)
        self.assertIn("UNDEF", result.diagnostics[0])

    def test_function_order_non_gating_unresolved_does_not_fail(self):
        symbol_name = "?Auxiliary@@YAHXZ"
        target = VerifyTarget(
            name="sample_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
        )
        function = VerifyFunction(
            address="0x401000",
            symbol=symbol_name,
            name="Auxiliary",
            source_order_gate=False,
            required_presence=False,
        )

        class FakeCoff:
            symbols = (SimpleNamespace(name=symbol_name, section_number=0, value=0),)
            symbols_by_name = {symbol_name: symbols[0]}

            def section(self, section_number):
                raise ValueError(f"bad section {section_number}")

        result = check_function_order(
            target=target,
            functions=(function,),
            coff_object=FakeCoff(),
        )

        self.assertTrue(result.ok)
        self.assertEqual((), result.rows)
        self.assertEqual(1, len(result.diagnostics))
        self.assertIn("legacy_source_order_gate=false", result.diagnostics[0])
        self.assertIn("UNDEF", result.diagnostics[0])

    def test_function_order_check_reports_order_breaks(self):
        target = VerifyTarget(
            name="sample_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
        )
        functions = (
            VerifyFunction(address="0x401000", symbol="?First@@YAHXZ", name="First"),
            VerifyFunction(address="0x401010", symbol="?Second@@YAHXZ", name="Second"),
        )

        class FakeCoff:
            symbols_by_name = {
                "?First@@YAHXZ": SimpleNamespace(section_number=2, value=0),
                "?Second@@YAHXZ": SimpleNamespace(section_number=1, value=0),
            }

            def section(self, _section_number):
                return SimpleNamespace(characteristics=IMAGE_SCN_CNT_CODE)

        result = check_function_order(
            target=target,
            functions=functions,
            coff_object=FakeCoff(),
        )

        self.assertFalse(result.ok)
        self.assertEqual(1, len(result.breaks))
        self.assertEqual("First", result.breaks[0].previous.function.name)
        self.assertEqual("Second", result.breaks[0].current.function.name)
        divergence = function_order_first_divergence(result)
        self.assertIsNotNone(divergence)
        self.assertEqual("reordered", divergence["kind"])
        self.assertEqual("First", divergence["expected"]["name"])
        self.assertEqual("Second", divergence["actual"]["name"])
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            print_function_order_check(result, summary_only=True)
        summary = output.getvalue()
        self.assertIn("result: FAIL", summary)
        self.assertIn("first blocking divergence [reordered]", summary)
        self.assertIn("expected neighbors:", summary)
        self.assertIn("candidate neighbors:", summary)
        self.assertNotIn("manifest retail order:", summary)

    def test_order_only_rejects_non_order_target_before_compilation(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            manifest_dir = Path(temp_dir)
            write_manifest(manifest_dir)
            with self.assertRaises(SystemExit) as raised:
                vc5_main([
                    "--order-only",
                    "sample",
                    "--manifest-dir",
                    str(manifest_dir),
                ])
        self.assertEqual(2, raised.exception.code)

    def test_linked_target_requires_order_only(self):
        with self.assertRaises(SystemExit) as raised:
            vc5_main(["sample", "--linked-target", "sample_linked"])

        self.assertEqual(2, raised.exception.code)

    def test_live_linked_feedback_surfaces_cabout_r1380_divergence(self):
        object_target, linked_target = linked_feedback_targets()
        object_result = {
            "kind": "vc5-order-live-result",
            "target_id": object_target.name,
            "phase": "authored-function-order",
            "physical_block_id": "recoil:block:0x401000",
            "passed": True,
            "expected_sequence": [
                f"recoil:function:{function.address}" for function in object_target.functions
            ],
            "candidate_sequence": [
                f"recoil:function:{function.address}" for function in object_target.functions
            ],
            "matched_prefix_count": 5,
            "first_divergence": None,
        }
        divergence = {
            "kind": "unexpected-selected-contribution",
            "message": "manifest rows resolve to a reordered selected linked function sequence",
            "expected": {
                "retail_address": "0x401020",
                "name": "shared ret-4 address group",
                "identity": "?DoDataExchange@CWnd@@MAEXPAVCDataExchange@@@Z",
            },
            "actual": {
                "linked_address": "0x401020",
                "identities": ["??3CObject@@SGXPAX@Z"],
                "providers": ["about.obj"],
                "disposition": "selected-unlisted-function",
            },
            "expected_neighbors": [
                {
                    "retail_address": "0x401000",
                    "name": "CAboutDlg::CAboutDlg",
                    "identity": "??0CAboutDlg@@QAE@PAVCWnd@@@Z",
                },
                {
                    "retail_address": "0x401020",
                    "name": "shared ret-4 address group",
                    "identity": "?DoDataExchange@CWnd@@MAEXPAVCDataExchange@@@Z",
                },
                {
                    "retail_address": "0x401030",
                    "name": "CAboutDlg::GetMessageMap",
                    "identity": "?GetMessageMap@CAboutDlg@@MBEPBUAFX_MSGMAP@@XZ",
                },
            ],
            "candidate_neighbors": [
                {
                    "linked_address": "0x401000",
                    "identities": ["??0CAboutDlg@@QAE@PAVCWnd@@@Z"],
                    "providers": ["about.obj"],
                },
                {
                    "linked_address": "0x401020",
                    "identities": ["??3CObject@@SGXPAX@Z"],
                    "providers": ["about.obj"],
                },
                {
                    "linked_address": "0x401030",
                    "identities": [
                        "??1CAboutDlg@@UAE@XZ",
                        "??1WestwoodOnlineUpgradeProgressDialog@@UAE@XZ",
                    ],
                    "providers": ["about.obj"],
                },
            ],
        }

        def linked_run(command, **kwargs):
            self.assertEqual(REPO_ROOT, kwargs["cwd"])
            self.assertEqual(
                [
                    "verify",
                    "linked-order",
                    linked_target.name,
                    "--scope",
                    "full",
                ],
                command[2:7],
            )
            self.assertIn("--manifest", command)
            self.assertEqual(
                object_target.compile_context_from,
                command[command.index("--manifest") + 1],
            )
            self.assertEqual(
                "fixture-progress.json",
                command[command.index("--progress") + 1],
            )
            linked_root = REPO_ROOT / command[command.index("--build-root") + 1]
            linked_root.mkdir(parents=True)
            report_path = linked_root / "linked_order_cabout.json"
            report_path.write_text(
                json.dumps(
                    {
                        "kind": "linked-function-order-report",
                        "target": linked_target.name,
                        "interval": "cabout_retail_interval",
                        "order_scope": "full",
                        "binary": "recoil",
                        "retail_start": "0x401000",
                        "retail_end_exclusive": "0x401060",
                        "passed": False,
                        "linked_exact_selected_population_evaluated": True,
                        "linked_seams_and_rvas_evaluated": True,
                        "first_divergence": divergence,
                    }
                ),
                encoding="utf-8",
            )
            (linked_root / "summary.json").write_text(
                json.dumps(
                    {
                        "kind": "linked-function-order-run",
                        "success": False,
                        "binary": "recoil",
                        "order_scope": "full",
                        "order_reports": [{"path": str(report_path)}],
                    }
                ),
                encoding="utf-8",
            )
            return SimpleNamespace(returncode=1, stdout="", stderr="")

        with tempfile.TemporaryDirectory(dir=REPO_ROOT / "build") as temp_dir:
            with patch(
                "_recoil.commands.vc5_verify.subprocess.run",
                side_effect=linked_run,
            ) as run:
                result = live_linked_feedback_result(
                    object_target=object_target,
                    linked_target=linked_target,
                    object_result=object_result,
                    build_root=Path(temp_dir) / "worker",
                    progress_path=Path("fixture-progress.json"),
                )

        run.assert_called_once()
        self.assertFalse(result["passed"])
        self.assertTrue(result["object_order_passed"])
        self.assertEqual("full-function-order", result["phase"])
        self.assertEqual("unexpected-selected-contribution", result["first_divergence"]["kind"])
        self.assertEqual(
            ["??3CObject@@SGXPAX@Z"],
            result["first_divergence"]["actual"]["identities"],
        )
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            print_live_order_result(result)
        rendered = output.getvalue()
        self.assertIn("object projection PASS 5/5", rendered)
        self.assertIn("first linked divergence [unexpected-selected-contribution]", rendered)
        self.assertIn("0x401020 ??3CObject@@SGXPAX@Z (about.obj)", rendered)
        self.assertIn("?GetMessageMap@CAboutDlg@@MBEPBUAFX_MSGMAP@@XZ", rendered)
        self.assertIn("??1CAboutDlg@@UAE@XZ", rendered)

    def test_order_only_routes_object_pass_into_linked_feedback(self):
        object_target, linked_target = linked_feedback_targets()
        object_result = {
            "kind": "vc5-order-live-result",
            "target_id": object_target.name,
            "phase": "authored-function-order",
            "physical_block_id": "recoil:block:0x401000",
            "passed": True,
            "expected_sequence": ["a", "b", "c", "d", "e"],
            "candidate_sequence": ["a", "b", "c", "d", "e"],
            "matched_prefix_count": 5,
            "first_divergence": None,
        }
        linked_result = {
            **object_result,
            "phase": "full-function-order",
            "passed": False,
            "feedback_dimension": "linked-selected-population-and-seams",
            "object_order_passed": True,
            "linked_target_id": linked_target.name,
            "first_divergence": {
                "kind": "unexpected-selected-contribution",
                "message": "unexpected CObject delete",
                "expected_neighbors": [{"retail_address": "0x401020", "name": "expected"}],
                "candidate_neighbors": [{"linked_address": "0x401020", "name": "actual"}],
            },
        }
        stdout = io.StringIO()
        with patch(
            "_recoil.commands.vc5_verify.load_manifests",
            return_value=[object_target, linked_target],
        ), patch(
            "_recoil.commands.vc5_verify.live_order_result",
            return_value=object_result,
        ) as object_live, patch(
            "_recoil.commands.vc5_verify.live_linked_feedback_result",
            return_value=linked_result,
        ) as linked_live, contextlib.redirect_stdout(stdout):
            rc = vc5_main(
                [
                    "--order-only",
                    object_target.name,
                    "--linked-target",
                    linked_target.name,
                    "--build-root",
                    "build/unit/linked-feedback",
                    "--progress",
                    "fixture-progress.json",
                ]
            )

        self.assertEqual(1, rc)
        object_live.assert_called_once()
        linked_live.assert_called_once()
        self.assertEqual(
            object_target.name,
            linked_live.call_args.kwargs["object_target"].name,
        )
        self.assertTrue(
            linked_live.call_args.kwargs["object_target"].source_emission_policy_strict
        )
        self.assertEqual(
            linked_target.name,
            linked_live.call_args.kwargs["linked_target"].name,
        )
        self.assertEqual(Path("fixture-progress.json"), linked_live.call_args.kwargs["progress_path"])
        self.assertIn("object projection PASS 5/5", stdout.getvalue())
        self.assertIn("unexpected CObject delete", stdout.getvalue())

    def test_live_order_sequences_use_stable_tracker_identities_and_typed_divergences(self):
        target = VerifyTarget(
            name="sample_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_function_order=True,
            function_order_scope="full",
            retail_start="0x401000",
            retail_end_exclusive="0x401030",
        )
        first = VerifyFunction(
            address="0x401000",
            symbol="?First@@YAHXZ",
            name="First",
            pipeline_class="authored",
            logical_identity_key="recoil:logical-function:first",
        )
        second = VerifyFunction(
            address="0x401010",
            symbol="?Second@@YAHXZ",
            name="Second",
            pipeline_class="authored",
        )
        extra = VerifyFunction(
            address="0x401020",
            symbol="?Extra@@YAHXZ",
            name="Extra",
            pipeline_class="authored",
        )

        self.assertEqual(
            "recoil:logical-function:first",
            function_tracker_identity(target, first),
        )
        self.assertEqual(
            "recoil:function:0x401010",
            function_tracker_identity(target, second),
        )

        def row(function, manifest_index, value):
            return FunctionOrderRow(
                manifest_index=manifest_index,
                function=function,
                symbol=function.symbol,
                section_number=1,
                value=value,
            )

        cases = (
            (
                (first, second),
                (row(first, 0, 2), row(second, 1, 1)),
                "reordered",
            ),
            (
                (first, second),
                (row(first, 0, 1),),
                "missing",
            ),
            (
                (first,),
                (row(first, 0, 1), row(first, 0, 2)),
                "duplicate",
            ),
            (
                (first,),
                (row(first, 0, 1), row(extra, 1, 2)),
                "extra",
            ),
        )
        for expected_functions, rows, expected_kind in cases:
            with self.subTest(expected_kind=expected_kind):
                check = FunctionOrderCheck(
                    target=target,
                    rows=rows,
                    breaks=(),
                    expected_functions=expected_functions,
                    order_scope="full",
                    required_presence_passed=True,
                    full_relative_order_passed=True,
                )
                expected, candidate, prefix, divergence = _live_order_sequences(check)
                self.assertIsNotNone(divergence)
                self.assertEqual(expected_kind, divergence["kind"])
                self.assertEqual(
                    sum(1 for left, right in zip(expected, candidate) if left == right),
                    prefix,
                )

    def test_order_only_json_emits_exactly_one_live_result_object(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            path = write_manifest(root)
            manifest = json.loads(path.read_text(encoding="utf-8"))
            manifest["check_function_order"] = True
            manifest["retail_start"] = "0x401000"
            manifest["retail_end_exclusive"] = "0x401010"
            path.write_text(json.dumps(manifest), encoding="utf-8")
            result = {
                "kind": "vc5-order-live-result",
                "target_id": "sample",
                "phase": "full-function-order",
                "physical_block_id": "recoil:block:0x401000",
                "passed": True,
                "expected_sequence": ["recoil:function:0x401000"],
                "candidate_sequence": ["recoil:function:0x401000"],
                "matched_prefix_count": 1,
                "first_divergence": None,
            }
            stdout = io.StringIO()
            with patch(
                "_recoil.commands.vc5_verify.live_order_result",
                return_value=result,
            ) as live:
                with contextlib.redirect_stdout(stdout):
                    rc = vc5_main(
                        [
                            "--order-only",
                            "sample",
                            "--manifest-dir",
                            str(root),
                            "--build-root",
                            str(root / "build"),
                            "--json",
                        ]
                    )

        self.assertEqual(0, rc)
        self.assertEqual([result], [json.loads(line) for line in stdout.getvalue().splitlines()])
        live.assert_called_once()

    def test_failed_vc5_compile_surfaces_captured_diagnostics_and_stable_logs(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            captured = CommandScriptResult(
                returncode=2,
                script_path=root / "_run_vc5_verify.cmd",
                stdout="sample.cpp(7) : error C2065: 'missing' : undeclared identifier\n",
                stderr="fatal compiler stderr\n",
            )
            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch(
                "_recoil.commands.vc5_verify.run_tool_cmd_script",
                return_value=captured,
            ) as run_script:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    completed = run_vc5_script(
                        "call vc5sp3-env.cmd && cl /c sample.cpp",
                        cwd=root,
                        diagnostic_stem="translation-unit-00-sample",
                    )

            stdout_log = root / "logs" / "translation-unit-00-sample.out.log"
            stderr_log = root / "logs" / "translation-unit-00-sample.err.log"
            self.assertEqual(2, completed.returncode)
            self.assertEqual("", stdout.getvalue())
            self.assertIn("VC5 compiler stdout:", stderr.getvalue())
            self.assertIn("error C2065", stderr.getvalue())
            self.assertIn("VC5 compiler stderr:", stderr.getvalue())
            self.assertIn("fatal compiler stderr", stderr.getvalue())
            self.assertIn(stdout_log.as_posix(), stderr.getvalue())
            self.assertIn(stderr_log.as_posix(), stderr.getvalue())
            self.assertEqual(captured.stdout, stdout_log.read_text(encoding="utf-8"))
            self.assertEqual(captured.stderr, stderr_log.read_text(encoding="utf-8"))
            run_script.assert_called_once_with(
                "call vc5sp3-env.cmd && cl /c sample.cpp",
                cwd=root,
                script_name="_run_vc5_verify.cmd",
                capture_output=True,
            )

    def test_successful_vc5_compile_preserves_stream_and_return_semantics(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            captured = CommandScriptResult(
                returncode=0,
                script_path=root / "_run_vc5_verify.cmd",
                stdout="Microsoft (R) 32-bit C/C++ Optimizing Compiler\n",
                stderr="success warning\n",
            )
            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch(
                "_recoil.commands.vc5_verify.run_tool_cmd_script",
                return_value=captured,
            ):
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    completed = run_vc5_script(
                        "call vc5sp3-env.cmd && cl /c sample.cpp",
                        cwd=root,
                        diagnostic_stem="translation-unit-00-sample",
                    )

            self.assertEqual(0, completed.returncode)
            self.assertEqual(captured.stdout, stdout.getvalue())
            self.assertEqual(captured.stderr, stderr.getvalue())
            self.assertEqual(
                captured.stdout,
                (root / "logs" / "translation-unit-00-sample.out.log").read_text(encoding="utf-8"),
            )
            self.assertEqual(
                captured.stderr,
                (root / "logs" / "translation-unit-00-sample.err.log").read_text(encoding="utf-8"),
            )

    def test_translation_unit_function_order_reports_object_index(self):
        first = VerifyFunction(address="0x401000", symbol="?First@@YAHXZ", name="First")
        second = VerifyFunction(address="0x401010", symbol="?Second@@YAHXZ", name="Second")
        target = VerifyTarget(
            name="sample_tu_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(source_from="src/first.cpp", functions=(first,)),
                SimpleNamespace(source_from="src/second.cpp", functions=(second,)),
            ),
        )

        class FirstCoff:
            symbols_by_name = {
                "?First@@YAHXZ": SimpleNamespace(
                    name="?First@@YAHXZ", section_number=2, value=0x10,
                    type=0x20, storage_class=2, index=1,
                ),
            }
            symbols = tuple(symbols_by_name.values())

            def section(self, _section_number):
                return SimpleNamespace(characteristics=IMAGE_SCN_CNT_CODE)

        class SecondCoff:
            symbols_by_name = {
                "?Second@@YAHXZ": SimpleNamespace(
                    name="?Second@@YAHXZ", section_number=1, value=0,
                    type=0x20, storage_class=2, index=1,
                ),
            }
            symbols = tuple(symbols_by_name.values())

            def section(self, _section_number):
                return SimpleNamespace(characteristics=IMAGE_SCN_CNT_CODE)

        result = check_translation_unit_function_order(
            target=target,
            coff_objects=(FirstCoff(), SecondCoff()),
        )

        self.assertTrue(result.ok)
        self.assertEqual(0, result.rows[0].object_index)
        self.assertEqual(1, result.rows[1].object_index)
        self.assertEqual("src/second.cpp", result.rows[1].source_from)

    def test_authored_order_icf_aliases_gate_presence_without_internal_relative_order(self):
        selected = VerifyFunction(
            address="0x401000",
            symbol="?SelectedAlias@@YAXXZ",
            name="SelectedAlias",
            pipeline_class="authored",
            authored_order_role="authored-body",
            full_order_gate=False,
            logical_identity_key="recoil:logical-function:0x401000:selected",
            icf_fold_status="selected-winner",
        )
        cofolded = VerifyFunction(
            address="0x401000",
            symbol="?CofoldedAlias@@YAXXZ",
            name="CofoldedAlias",
            pipeline_class="authored",
            authored_order_role="authored-body",
            full_order_gate=False,
            logical_identity_key="recoil:logical-function:0x401000:cofolded",
            icf_fold_status="proven-fold-alias",
        )
        after = VerifyFunction(
            address="0x401010",
            symbol="?After@@YAXXZ",
            name="After",
            pipeline_class="authored",
            authored_order_role="authored-body",
        )
        target = VerifyTarget(
            name="sample_icf_alias_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(
                    source_from="src/sample.cpp",
                    functions=(selected, cofolded, after),
                    order_scope="authored",
                ),
            ),
        )

        def function(name, value, index):
            return SimpleNamespace(
                name=name,
                section_number=1,
                value=value,
                type=0x20,
                storage_class=2,
                index=index,
            )

        fake_symbols = (
            function(selected.symbol, 0x20, 1),
            function(cofolded.symbol, 0x10, 2),
            function(after.symbol, 0x30, 3),
        )

        class FakeCoff:
            symbols = fake_symbols
            symbols_by_name = {symbol.name: symbol for symbol in fake_symbols}

            def section(self, _section_number):
                return SimpleNamespace(name=".text", characteristics=IMAGE_SCN_CNT_CODE)

        result = check_translation_unit_function_order(target=target, coff_objects=(FakeCoff(),))

        self.assertTrue(result.ok)
        self.assertTrue(result.required_presence_passed)
        self.assertTrue(result.authored_relative_order_passed)
        self.assertEqual((), result.breaks)
        report = translation_unit_order_report_data(result)
        self.assertEqual(
            [selected.logical_identity_key, cofolded.logical_identity_key, None],
            [item["logical_identity_key"] for item in report["expected_contributions"]],
        )
        self.assertEqual(
            [True, False, True],
            [item["authored_relative_order_gate"] for item in report["expected_contributions"]],
        )

    def test_authored_order_neutral_fold_unknown_identity_is_a_relative_order_gate(self):
        neutral = VerifyFunction(
            address="0x401000",
            symbol="?ProvisionalMember@HudUiWidget@@QAEXXZ",
            name="HudUiWidget::ProvisionalMember",
            pipeline_class="authored",
            authored_order_role="authored-body",
            full_order_gate=False,
            logical_identity_key="recoil:logical-function:0x401000:hud-member",
            icf_fold_status="not-established",
        )
        cofolded = replace(
            neutral,
            logical_identity_key="recoil:logical-function:0x401000:proven-cofold",
            icf_fold_status="proven-fold-alias",
        )
        self.assertTrue(function_authored_order_gate(neutral))
        self.assertTrue(function_authored_relative_order_gate(neutral))
        self.assertTrue(function_authored_order_gate(cofolded))
        self.assertFalse(function_authored_relative_order_gate(cofolded))

    def test_live_authored_sequence_excludes_proven_fold_alias_but_keeps_selected_winner(self):
        target = VerifyTarget(
            name="sample_authored_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_function_order=True,
            function_order_scope="authored",
            retail_start="0x401000",
            retail_end_exclusive="0x401020",
        )
        selected = VerifyFunction(
            address="0x401000",
            symbol="?Selected@@YAXXZ",
            name="Selected",
            pipeline_class="authored",
            authored_order_role="authored-body",
            full_order_gate=False,
            logical_identity_key="recoil:logical-function:0x401000:selected",
            icf_fold_status="selected-winner",
        )
        cofolded = replace(
            selected,
            symbol="?Cofolded@@YAXXZ",
            name="Cofolded",
            logical_identity_key="recoil:logical-function:0x401000:cofolded",
            icf_fold_status="proven-fold-alias",
        )
        after = VerifyFunction(
            address="0x401010",
            symbol="?After@@YAXXZ",
            name="After",
            pipeline_class="authored",
            authored_order_role="authored-body",
        )
        rows = (
            FunctionOrderRow(1, cofolded, cofolded.symbol, 1, 0x10),
            FunctionOrderRow(0, selected, selected.symbol, 1, 0x20),
            FunctionOrderRow(2, after, after.symbol, 1, 0x30),
        )
        check = FunctionOrderCheck(
            target=target,
            rows=rows,
            breaks=(),
            expected_functions=(selected, cofolded, after),
            order_scope="authored",
            required_presence_passed=True,
            authored_relative_order_passed=True,
        )

        expected, candidate, prefix, divergence = _live_order_sequences(check)

        self.assertEqual(
            [selected.logical_identity_key, "recoil:function:0x401010"], expected
        )
        self.assertEqual(expected, candidate)
        self.assertEqual(2, prefix)
        self.assertIsNone(divergence)

    def test_translation_unit_order_inventories_unlisted_before_between_and_after(self):
        first = VerifyFunction(address="0x401000", symbol="?First@@YAHXZ", name="First")
        second = VerifyFunction(address="0x401010", symbol="?Second@@YAHXZ", name="Second")
        target = VerifyTarget(
            name="sample_tu_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(
                    source_from="src/sample.cpp",
                    functions=(first, second),
                ),
            ),
        )

        def function(name, section):
            return SimpleNamespace(
                name=name,
                section_number=section,
                value=0,
                type=0x20,
                storage_class=2,
                index=section,
            )

        fake_symbols = (
            function("?Before@@YAHXZ", 1),
            function(first.symbol, 2),
            function("?Between@@YAHXZ", 3),
            function(second.symbol, 4),
            function("?After@@YAHXZ", 5),
        )

        class FakeCoff:
            symbols = fake_symbols
            symbols_by_name = {symbol.name: symbol for symbol in fake_symbols}

            def section(self, section_number):
                return SimpleNamespace(name=f".text${section_number}", characteristics=IMAGE_SCN_CNT_CODE)

        result = check_translation_unit_function_order(target=target, coff_objects=(FakeCoff(),))

        self.assertTrue(result.ok)
        unlisted = [item for item in result.contributions if item.disposition == "unlisted-defined-function"]
        self.assertEqual(3, len(unlisted))
        self.assertEqual((), result.blocking_diagnostics)
        report = translation_unit_order_report_data(result)
        self.assertEqual(2, report["report_version"])
        self.assertEqual("vc5-function-order-report", report["kind"])
        self.assertEqual("full", report["order_scope"])
        self.assertFalse(report["unlisted_raw_contributions_blocking"])
        self.assertEqual(2, report["expected_count"])
        self.assertEqual(5, report["actual_count"])
        self.assertEqual([], report["missing_contributions"])
        self.assertEqual(
            ["?Before@@YAHXZ", "?Between@@YAHXZ", "?After@@YAHXZ"],
            report["extra_contributions"],
        )
        self.assertFalse(report["raw_defined_function_set_matches"])
        self.assertEqual(5, len(report["actual_contributions"]))
        self.assertTrue(report["passed"])

    def test_translation_unit_order_projects_physical_icf_gate_to_tu_object_symbol(self):
        physical = VerifyFunction(
            address="0x42ee40",
            symbol="?SetEnabled@HudUiBackgroundContainer@@UAEXH@Z",
            name="physical authored-ICF gate",
            pipeline_class="authored",
            authored_order_role="authored-body",
        )
        mission_symbol = "?SetMissionId@RecoilApp_MissionFmvState@@QAEXH@Z"
        target = VerifyTarget(
            name="sample_tu_icf_projection", description="sample",
            source_filename="RecoilApp.cpp", source_text="",
            source_from="src/Battlesport/RecoilApp.cpp", compare_mode="coff_bytes",
            trim_trailing_nops=True, compiler_profile="", compiler_env="",
            compiler_flags=(), include_dirs=(), source_files=(), generated_files=(),
            functions=(), data_symbols=(), manifest_path=Path("production-target.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(
                    source_from="src/Battlesport/RecoilApp.cpp",
                    functions=(physical,), order_scope="authored", inventory_only=False,
                ),
            ),
        )

        symbol = SimpleNamespace(
            name=mission_symbol, section_number=1, value=0,
            type=0x20, storage_class=2, index=1,
        )

        class FakeCoff:
            symbols = (symbol,)
            symbols_by_name = {symbol.name: symbol}

            def section(self, _section_number):
                return SimpleNamespace(name=".text", characteristics=IMAGE_SCN_CNT_CODE)

        with patch(
            "_recoil.commands.vc5_verify."
            "current_authored_icf_translation_unit_object_symbol",
            return_value=mission_symbol,
        ) as selector:
            result = check_translation_unit_function_order(
                target=target,
                coff_objects=(FakeCoff(),),
            )

        self.assertTrue(result.ok)
        self.assertEqual("0x42ee40", result.rows[0].function.address)
        self.assertEqual("", result.rows[0].function.logical_identity_key)
        self.assertEqual(mission_symbol, result.rows[0].symbol)
        self.assertEqual("src/Battlesport/RecoilApp.cpp", result.rows[0].source_from)
        selector.assert_called_once()

        with (
            patch(
                "_recoil.commands.vc5_verify."
                "current_authored_icf_translation_unit_object_symbol",
                side_effect=ValueError("stale authored-ICF object proof"),
            ),
        ):
            rejected = check_translation_unit_function_order(
                target=target,
                coff_objects=(FakeCoff(),),
            )
        self.assertFalse(rejected.ok)
        self.assertIn("stale authored-ICF object proof", rejected.blocking_diagnostics[0])

    def test_inventory_only_object_is_fully_inventoried_without_expected_or_order_credit(self):
        expected = VerifyFunction(
            address="0x401020",
            symbol="?OnIgnoredKey@HudUiTextInput@@UAEXH@Z",
            name="HudUiTextInput::OnIgnoredKey",
            pipeline_class="authored",
            authored_order_role="authored-body",
            full_order_gate=False,
            logical_identity_key="recoil:logical-function:0x401020:on-ignored-key",
            icf_fold_status="proven-fold-alias",
        )
        target = VerifyTarget(
            name="sample_inventory_only", description="sample", source_filename="sample.cpp",
            source_text="", source_from="src/sample.cpp", compare_mode="coff_bytes",
            trim_trailing_nops=True, compiler_profile="", compiler_env="", compiler_flags=(),
            include_dirs=(), source_files=(), generated_files=(), functions=(), data_symbols=(),
            manifest_path=Path("sample.json"), check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(
                    source_from="src/zui.cpp", functions=(expected,),
                    order_scope="authored", inventory_only=False,
                ),
                SimpleNamespace(
                    source_from="src/hud.cpp", functions=(),
                    order_scope="authored", inventory_only=True,
                ),
            ),
        )

        def symbol(index):
            return SimpleNamespace(
                name=expected.symbol, section_number=1, value=0,
                type=0x20, storage_class=2, index=index,
            )

        class FakeCoff:
            def __init__(self, item):
                self.symbols = (item,)
                self.symbols_by_name = {item.name: item}

            def section(self, _section_number):
                return SimpleNamespace(name=".text", characteristics=IMAGE_SCN_CNT_CODE)

        result = check_translation_unit_function_order(
            target=target,
            coff_objects=(FakeCoff(symbol(1)), FakeCoff(symbol(2))),
        )
        report = translation_unit_order_report_data(result)

        self.assertTrue(result.ok)
        self.assertEqual(1, report["expected_count"])
        self.assertEqual(2, report["actual_count"])
        self.assertEqual([], report["missing_contributions"])
        self.assertEqual([expected.symbol], report["duplicate_contributions"])
        self.assertEqual("unlisted-defined-function", result.contributions[1].disposition)
        self.assertFalse(report["raw_defined_function_set_matches"])
        self.assertTrue(report["required_presence_passed"])
        self.assertTrue(report["authored_relative_order_passed"])
        self.assertTrue(report["passed"])

    def test_translation_unit_order_rejects_two_expected_rows_in_one_alias_group(self):
        first = VerifyFunction(address="0x401000", symbol="?First@@YAHXZ", name="First")
        alias = VerifyFunction(address="0x401010", symbol="?Alias@@YAHXZ", name="Alias")
        target = VerifyTarget(
            name="sample_tu_alias", description="sample", source_filename="sample.cpp",
            source_text="", source_from="src/sample.cpp", compare_mode="coff_bytes",
            trim_trailing_nops=True, compiler_profile="", compiler_env="", compiler_flags=(),
            include_dirs=(), source_files=(), generated_files=(), functions=(), data_symbols=(),
            manifest_path=Path("sample.json"), check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(source_from="src/sample.cpp", functions=(first, alias)),
            ),
        )
        first_symbol = SimpleNamespace(name=first.symbol, section_number=1, value=0, type=0x20, storage_class=2, index=1)
        alias_symbol = SimpleNamespace(name=alias.symbol, section_number=1, value=0, type=0x20, storage_class=105, index=2)

        class FakeCoff:
            symbols = (first_symbol, alias_symbol)
            symbols_by_name = {first.symbol: first_symbol, alias.symbol: alias_symbol}

            def section(self, _section_number):
                return SimpleNamespace(name=".text", characteristics=IMAGE_SCN_CNT_CODE)

        result = check_translation_unit_function_order(target=target, coff_objects=(FakeCoff(),))
        self.assertFalse(result.ok)
        self.assertTrue(any("same defined-function alias group" in row for row in result.blocking_diagnostics))

    def test_translation_unit_order_groups_aliases_and_ignores_data_and_undefined(self):
        first = VerifyFunction(address="0x401000", symbol="?First@@YAHXZ", name="First")
        target = VerifyTarget(
            name="sample_tu_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(
                    source_from="src/sample.cpp",
                    functions=(first,),
                ),
            ),
        )
        defined = SimpleNamespace(
            name=first.symbol,
            section_number=1,
            value=0,
            type=0x20,
            storage_class=2,
            index=1,
        )
        alias = SimpleNamespace(
            name="?FirstAlias@@YAHXZ",
            section_number=1,
            value=0,
            type=0x20,
            storage_class=105,
            index=2,
        )
        undefined = SimpleNamespace(
            name=first.symbol,
            section_number=0,
            value=0,
            type=0x20,
            storage_class=2,
            index=3,
        )
        data = SimpleNamespace(
            name="_g_data",
            section_number=2,
            value=0,
            type=0,
            storage_class=2,
            index=4,
        )

        class FakeCoff:
            symbols = (undefined, defined, alias, data)
            symbols_by_name = {first.symbol: undefined, alias.name: alias, data.name: data}

            def section(self, section_number):
                if section_number == 1:
                    return SimpleNamespace(name=".text", characteristics=IMAGE_SCN_CNT_CODE | 0x1000)
                if section_number == 2:
                    return SimpleNamespace(name=".data", characteristics=0x40)
                raise ValueError(section_number)

        result = check_translation_unit_function_order(target=target, coff_objects=(FakeCoff(),))

        self.assertTrue(result.ok)
        self.assertEqual(1, len(result.contributions))
        self.assertEqual((first.symbol, alias.name), result.contributions[0].symbols)
        self.assertTrue(result.contributions[0].comdat)
        self.assertTrue(result.contributions[0].weak)
        self.assertEqual("manifest-function", result.contributions[0].disposition)

    def test_translation_unit_function_order_symbol_regex_ignores_undefined_duplicate(self):
        symbol_name = "??_GSample@@UAEPAXI@Z"
        function = VerifyFunction(
            address="0x401000",
            symbol="",
            symbol_regex=r"\?\?_GSample@@UAEPAXI@Z",
            name="Sample::~Sample",
        )
        target = VerifyTarget(
            name="sample_tu_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(source_from="src/sample.cpp", functions=(function,)),
            ),
        )
        defined_symbol = SimpleNamespace(name=symbol_name, section_number=3, value=0x20)

        class FakeCoff:
            symbols = (
                SimpleNamespace(name=symbol_name, section_number=0, value=0),
                defined_symbol,
            )
            symbols_by_name = {symbol_name: defined_symbol}

            def section(self, section_number):
                if section_number == 3:
                    return SimpleNamespace(name="SECT34B", characteristics=IMAGE_SCN_CNT_CODE)
                raise ValueError(f"bad section {section_number}")

        result = check_translation_unit_function_order(
            target=target,
            coff_objects=(FakeCoff(),),
        )

        self.assertTrue(result.ok)
        self.assertEqual(symbol_name, result.rows[0].symbol)
        self.assertEqual(3, result.rows[0].section_number)

    def test_translation_unit_function_order_exact_symbol_ignores_undefined_duplicate(self):
        symbol_name = "??_GSample@@UAEPAXI@Z"
        first = VerifyFunction(
            address="0x401000",
            symbol=symbol_name,
            name="Sample::~Sample",
        )
        second = VerifyFunction(address="0x401010", symbol="?After@@YAHXZ", name="After")
        target = VerifyTarget(
            name="sample_tu_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(source_from="src/sample.cpp", functions=(first, second)),
            ),
        )
        undefined_symbol = SimpleNamespace(name=symbol_name, section_number=0, value=0)
        defined_symbol = SimpleNamespace(name=symbol_name, section_number=3, value=0x20)
        after_symbol = SimpleNamespace(name="?After@@YAHXZ", section_number=4, value=0)

        class FakeCoff:
            symbols = (undefined_symbol, defined_symbol, after_symbol)
            symbols_by_name = {
                symbol_name: undefined_symbol,
                "?After@@YAHXZ": after_symbol,
            }

            def section(self, section_number):
                if section_number in (3, 4):
                    return SimpleNamespace(name=f"SECT{section_number}", characteristics=IMAGE_SCN_CNT_CODE)
                raise ValueError(f"bad section {section_number}")

        result = check_translation_unit_function_order(
            target=target,
            coff_objects=(FakeCoff(),),
        )

        self.assertTrue(result.ok)
        self.assertEqual(symbol_name, result.rows[0].symbol)
        self.assertEqual(3, result.rows[0].section_number)

    def test_translation_unit_function_order_reports_unresolved_and_order_breaks(self):
        missing = VerifyFunction(address="0x401000", symbol="?Missing@@YAHXZ", name="Missing")
        first = VerifyFunction(address="0x401010", symbol="?First@@YAHXZ", name="First")
        second = VerifyFunction(address="0x401020", symbol="?Second@@YAHXZ", name="Second")
        target = VerifyTarget(
            name="sample_tu_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(source_from="src/sample.cpp", functions=(missing, first, second)),
            ),
        )

        class FakeCoff:
            symbols = (
                SimpleNamespace(name="?Missing@@YAHXZ", section_number=0, value=0),
                SimpleNamespace(name="?First@@YAHXZ", section_number=2, value=0),
                SimpleNamespace(name="?Second@@YAHXZ", section_number=1, value=0),
            )
            symbols_by_name = {
                "?Missing@@YAHXZ": symbols[0],
                "?First@@YAHXZ": symbols[1],
                "?Second@@YAHXZ": symbols[2],
            }

            def section(self, section_number):
                if section_number in (1, 2):
                    return SimpleNamespace(characteristics=IMAGE_SCN_CNT_CODE)
                raise ValueError(f"bad section {section_number}")

        result = check_translation_unit_function_order(
            target=target,
            coff_objects=(FakeCoff(),),
        )

        self.assertFalse(result.ok)
        self.assertEqual(1, len(result.diagnostics))
        self.assertIn("UNDEF", result.diagnostics[0])
        self.assertEqual(1, len(result.breaks))
        self.assertEqual("First", result.breaks[0].previous.function.name)
        self.assertEqual("Second", result.breaks[0].current.function.name)

    def test_authored_scope_ignores_compiler_generated_helper_position_and_presence(self):
        first = VerifyFunction(
            address="0x401000",
            symbol="?First@@YAHXZ",
            name="First",
            pipeline_class="authored",
        )
        helper = VerifyFunction(
            address="0x401010",
            symbol="??_GHelper@@UAEPAXI@Z",
            name="Helper scalar deleting destructor",
            pipeline_class="authored-lifecycle",
            authored_order_role="compiler-generated-deleting-variant",
        )
        second = VerifyFunction(
            address="0x401020",
            symbol="?Second@@YAHXZ",
            name="Second",
            pipeline_class="authored",
        )
        target = VerifyTarget(
            name="sample_tu_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(
                    source_from="src/sample.cpp",
                    functions=(first, helper, second),
                    order_scope="authored",
                ),
            ),
        )

        class FakeCoff:
            symbols = (
                SimpleNamespace(name="?First@@YAHXZ", section_number=1, value=0, type=0x20),
                SimpleNamespace(name="??_GHelper@@UAEPAXI@Z", section_number=0, value=0, type=0x20),
                SimpleNamespace(name="?Second@@YAHXZ", section_number=2, value=0, type=0x20),
            )
            symbols_by_name = {item.name: item for item in symbols}

            def section(self, section_number):
                if section_number in (1, 2):
                    return SimpleNamespace(name=f"SECT{section_number}", characteristics=IMAGE_SCN_CNT_CODE)
                raise ValueError(f"bad section {section_number}")

        result = check_translation_unit_function_order(target=target, coff_objects=(FakeCoff(),))
        self.assertTrue(result.ok)
        self.assertEqual((), result.breaks)
        self.assertEqual(1, len(result.diagnostics))
        report = translation_unit_order_report_data(result)
        deferred = report["expected_contributions"][1]
        self.assertFalse(deferred["authored_order_gate"])
        self.assertIn(deferred["symbol"], report["optional_missing_contributions"])
        self.assertNotIn(deferred["symbol"], report["missing_contributions"])

    def test_translation_unit_function_order_non_gating_row_does_not_create_break(self):
        first = VerifyFunction(address="0x401000", symbol="?First@@YAHXZ", name="First")
        auxiliary = VerifyFunction(
            address="0x401010",
            symbol="?Auxiliary@@YAHXZ",
            name="Auxiliary",
            source_order_gate=False,
            full_order_gate=False,
        )
        second = VerifyFunction(address="0x401020", symbol="?Second@@YAHXZ", name="Second")
        target = VerifyTarget(
            name="sample_tu_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(source_from="src/sample.cpp", functions=(first, auxiliary, second)),
            ),
        )

        class FakeCoff:
            symbols = (
                SimpleNamespace(name="?First@@YAHXZ", section_number=1, value=0),
                SimpleNamespace(name="?Auxiliary@@YAHXZ", section_number=3, value=0),
                SimpleNamespace(name="?Second@@YAHXZ", section_number=2, value=0),
            )
            symbols_by_name = {
                "?First@@YAHXZ": symbols[0],
                "?Auxiliary@@YAHXZ": symbols[1],
                "?Second@@YAHXZ": symbols[2],
            }

            def section(self, section_number):
                return SimpleNamespace(name=f"SECT{section_number}", characteristics=IMAGE_SCN_CNT_CODE)

        result = check_translation_unit_function_order(
            target=target,
            coff_objects=(FakeCoff(),),
        )

        self.assertTrue(result.ok)
        self.assertEqual((), result.breaks)
        self.assertFalse(result.rows[1].function.source_order_gate)
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            print_function_order_check(result, translation_unit=True)
        self.assertIn("legacy_source_order_gate=false", output.getvalue())

    def test_translation_unit_function_order_non_gating_unresolved_does_not_fail(self):
        first = VerifyFunction(address="0x401000", symbol="?First@@YAHXZ", name="First")
        auxiliary = VerifyFunction(
            address="0x401010",
            symbol="?Auxiliary@@YAHXZ",
            name="Auxiliary",
            source_order_gate=False,
            required_presence=False,
            full_order_gate=False,
        )
        second = VerifyFunction(address="0x401020", symbol="?Second@@YAHXZ", name="Second")
        target = VerifyTarget(
            name="sample_tu_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(source_from="src/sample.cpp", functions=(first, auxiliary, second)),
            ),
        )

        class FakeCoff:
            symbols = (
                SimpleNamespace(name="?First@@YAHXZ", section_number=1, value=0),
                SimpleNamespace(name="?Auxiliary@@YAHXZ", section_number=0, value=0),
                SimpleNamespace(name="?Second@@YAHXZ", section_number=2, value=0),
            )
            symbols_by_name = {
                "?First@@YAHXZ": symbols[0],
                "?Auxiliary@@YAHXZ": symbols[1],
                "?Second@@YAHXZ": symbols[2],
            }

            def section(self, section_number):
                if section_number in (1, 2):
                    return SimpleNamespace(characteristics=IMAGE_SCN_CNT_CODE)
                raise ValueError(f"bad section {section_number}")

        result = check_translation_unit_function_order(
            target=target,
            coff_objects=(FakeCoff(),),
        )

        self.assertTrue(result.ok)
        self.assertEqual(1, len(result.diagnostics))
        self.assertIn("legacy_source_order_gate=false", result.diagnostics[0])
        self.assertIn("UNDEF", result.diagnostics[0])
        self.assertEqual((), result.breaks)

    def test_translation_unit_function_order_reports_break_within_object(self):
        first = VerifyFunction(address="0x401000", symbol="?First@@YAHXZ", name="First")
        second = VerifyFunction(address="0x401010", symbol="?Second@@YAHXZ", name="Second")
        target = VerifyTarget(
            name="sample_tu_order",
            description="sample",
            source_filename="sample.cpp",
            source_text="",
            source_from="src/sample.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=(),
            include_dirs=(),
            source_files=(),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
            check_translation_unit_function_order=True,
            translation_unit_function_order=(
                SimpleNamespace(source_from="src/first.cpp", functions=(first, second)),
            ),
        )

        class FakeCoff:
            symbols_by_name = {
                "?First@@YAHXZ": SimpleNamespace(
                    name="?First@@YAHXZ", section_number=2, value=0,
                    type=0x20, storage_class=2, index=1,
                ),
                "?Second@@YAHXZ": SimpleNamespace(
                    name="?Second@@YAHXZ", section_number=1, value=0,
                    type=0x20, storage_class=2, index=2,
                ),
            }
            symbols = tuple(symbols_by_name.values())

            def section(self, _section_number):
                return SimpleNamespace(characteristics=IMAGE_SCN_CNT_CODE)

        result = check_translation_unit_function_order(
            target=target,
            coff_objects=(FakeCoff(),),
        )

        self.assertFalse(result.ok)
        self.assertEqual("First", result.breaks[0].previous.function.name)
        self.assertEqual("Second", result.breaks[0].current.function.name)

    def test_translation_unit_function_order_places_cod_listing_labels(self):
        with tempfile.TemporaryDirectory() as tmp:
            cod_path = Path(tmp) / "sample.cod"
            cod_path.write_text(
                "_TEXT\tSEGMENT\n"
                "?Owner@@YAXXZ PROC NEAR\n"
                "  00000\t90\t\t nop\n"
                "$L100:\n"
                "  00001\tc3\t\t ret\t 0\n"
                "?Owner@@YAXXZ ENDP\n"
                "_TEXT\tENDS\n",
                encoding="utf-8",
            )

            class FakeCoff:
                symbols_by_name = {
                    "?Owner@@YAXXZ": SimpleNamespace(
                        name="?Owner@@YAXXZ", section_number=3, value=0,
                        type=0x20, storage_class=2, index=1,
                    ),
                    "?After@@YAXXZ": SimpleNamespace(
                        name="?After@@YAXXZ", section_number=4, value=0,
                        type=0x20, storage_class=2, index=2,
                    ),
                }
                symbols = tuple(symbols_by_name.values())

                def section(self, _section_number):
                    return SimpleNamespace(characteristics=IMAGE_SCN_CNT_CODE)

            label_index = parse_cod_listing_label_index(cod_path, FakeCoff())
            first = VerifyFunction(address="0x401000", symbol="?Owner@@YAXXZ", name="Owner")
            local = VerifyFunction(
                address="0x401010",
                symbol="",
                name="Owner::LocalCleanup",
                listing_label_regex="\\$L100",
                provenance="compiler-emitted-noncovering",
            )
            after = VerifyFunction(address="0x401020", symbol="?After@@YAXXZ", name="After")
            target = VerifyTarget(
                name="sample_tu_order",
                description="sample",
                source_filename="sample.cpp",
                source_text="",
                source_from="src/sample.cpp",
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="",
                compiler_env="",
                compiler_flags=(),
                include_dirs=(),
                source_files=(),
                generated_files=(),
                functions=(),
                data_symbols=(),
                manifest_path=Path("sample.json"),
                check_translation_unit_function_order=True,
                translation_unit_function_order=(
                    SimpleNamespace(source_from="src/sample.cpp", functions=(first, local, after)),
                ),
            )

            result = check_translation_unit_function_order(
                target=target,
                coff_objects=(FakeCoff(),),
                cod_label_indexes=(label_index,),
            )

        self.assertTrue(result.ok)
        self.assertEqual("$L100", result.rows[1].symbol)
        self.assertEqual(3, result.rows[1].section_number)
        self.assertEqual(1, result.rows[1].value)
        self.assertIn("cod-listing-label", result.rows[1].order_source)

    def test_load_manifest_accepts_source_from_and_generated_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["generated_files"] = {
                "sample.h": ["#pragma once", "int sample;"],
            }
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertTrue(manifest.source_from.endswith("sample.cpp"))
        self.assertEqual("", manifest.source_text)
        self.assertEqual((("sample.h", "#pragma once\nint sample;\n"),), manifest.generated_files)

    def test_inline_source_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_inline_manifest(Path(tmp))

            with self.assertRaisesRegex(ValueError, "expected 'source_from'"):
                load_manifest(path)

    def test_source_from_unresolved_function_does_not_require_legacy_provenance(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text("int __cdecl Sample() { return 1; }\n", encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual("unresolved", manifest.functions[0].pipeline_class)

    def test_source_from_accepts_provenance_in_included_inl(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text('#include "sample.inl"\n', encoding="utf-8")
            (source_path.parent / "sample.inl").write_text(
                "/**\n"
                " * Reimplements 0x401000: Sample.\n"
                " * Purpose: Exercises included inline provenance detection.\n"
                " */\n"
                "int __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )

            manifest = load_manifest(path)

        self.assertTrue(manifest.source_from.endswith("sample.cpp"))

    def test_source_from_accepts_provenance_in_included_header(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            source_path = Path(json.loads(path.read_text(encoding="utf-8"))["source_from"])
            source_path.write_text('#include "sample.h"\n', encoding="utf-8")
            (source_path.parent / "sample.h").write_text(
                "/**\n"
                " * Reimplements 0x401000: Sample.\n"
                " * Purpose: Exercises included header provenance detection.\n"
                " */\n"
                "inline int __cdecl Sample() { return 1; }\n",
                encoding="utf-8",
            )

            manifest = load_manifest(path)

        self.assertTrue(manifest.source_from.endswith("sample.cpp"))

    def test_generated_project_header_shadow_is_rejected_without_baseline(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["generated_files"] = {
                "GameZRecoil/RecoilApp/RecoilStateBase.h": ["#pragma once"],
            }
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "generated file shadows project header"):
                load_manifest(path)

    def test_find_target_by_name_selects_all_functions(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifests = load_manifests(Path(tmp))
            self.assertEqual([], manifests)
            manifest = load_manifest(write_manifest(Path(tmp)))

        target, functions, data_symbols, selector = find_target([manifest], "sample")

        self.assertIs(target, manifest)
        self.assertEqual(manifest.functions, functions)
        self.assertEqual((), data_symbols)
        self.assertEqual("sample", selector)

    def test_find_target_by_address_selects_one_function(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = load_manifest(write_manifest(Path(tmp)))

        target, functions, data_symbols, selector = find_target([manifest], "0x401000")

        self.assertIs(target, manifest)
        self.assertEqual(1, len(functions))
        self.assertEqual("0x401000", functions[0].address)
        self.assertEqual((), data_symbols)
        self.assertEqual("0x401000", selector)

    def test_load_manifest_accepts_data_symbols_without_functions(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = load_manifest(write_data_manifest(Path(tmp)))

        self.assertEqual((), manifest.functions)
        self.assertEqual(1, len(manifest.data_symbols))
        self.assertEqual("0x402000", manifest.data_symbols[0].address)
        self.assertEqual("??_7SampleWithTable@@6B@", manifest.data_symbols[0].symbol)
        self.assertEqual("g_SampleWithTable_FTable", manifest.data_symbols[0].bn_name)
        self.assertEqual(4, manifest.data_symbols[0].byte_length)
        self.assertEqual(0, manifest.data_symbols[0].object_offset)
        self.assertEqual("recoil", manifest.target_binary)
        self.assertEqual("Recoil.bndb", binja_binary_selector(manifest))

    def test_load_manifest_accepts_data_symbol_object_offset(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_data_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["data_symbols"][0]["object_offset"] = 12
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual(12, manifest.data_symbols[0].object_offset)

    def test_load_manifest_rejects_invalid_data_symbol_object_offset(self):
        for value in (-1, True, 1.5, "1"):
            with self.subTest(value=value):
                with tempfile.TemporaryDirectory() as tmp:
                    path = write_data_manifest(Path(tmp))
                    data = json.loads(path.read_text(encoding="utf-8"))
                    data["data_symbols"][0]["object_offset"] = value
                    path.write_text(json.dumps(data), encoding="utf-8")

                    with self.assertRaisesRegex(
                        ValueError,
                        "expected 'object_offset' to be a non-negative integer",
                    ):
                        load_manifest(path)

    def test_load_manifest_accepts_explicit_messages_target_binary(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_data_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["target_binary"] = "messages"
            data["data_symbols"][0]["address"] = "0x10006030"
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual("messages", manifest.target_binary)
        self.assertEqual("messages.bndb", binja_binary_selector(manifest))

    def test_messages_data_registration_uses_messages_stable_id(self):
        path = REPO_ROOT / "tools" / "vc5_verify_targets" / "messages_lookup_data.json"

        target_id, record = vc5_target_registration(path)

        self.assertEqual("messages:vc5-target:messages_lookup_data", target_id)
        self.assertEqual("messages", record["binary"])
        self.assertEqual("messages", record["registration"]["binary"])
        self.assertEqual(750, len(record["registered_addresses"]))

    def test_registration_rejects_explicit_binary_conflicting_with_address(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_data_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["target_binary"] = "recoil"
            data["data_symbols"][0]["address"] = "0x10006030"
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "conflicts with inferred binary"):
                vc5_target_registration(path)

    def test_load_manifest_infers_messages_target_binary_from_address(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_data_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["data_symbols"][0]["address"] = "0x10006030"
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual("messages", manifest.target_binary)

    def test_load_manifest_rejects_unknown_target_binary(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_data_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["target_binary"] = "other"
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "unknown target_binary"):
                load_manifest(path)

    def test_load_manifest_accepts_data_symbol_regex_without_exact_symbol(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_data_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["data_symbols"][0].pop("symbol")
            data["data_symbols"][0]["symbol_regex"] = r"\?\?_7SampleWithTable@@6B@"
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual("", manifest.data_symbols[0].symbol)
        self.assertEqual(r"\?\?_7SampleWithTable@@6B@", manifest.data_symbols[0].symbol_regex)

    def test_find_target_by_data_address_selects_one_data_symbol(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = load_manifest(write_data_manifest(Path(tmp)))

        target, functions, data_symbols, selector = find_target([manifest], "0x402000")

        self.assertIs(target, manifest)
        self.assertEqual((), functions)
        self.assertEqual(1, len(data_symbols))
        self.assertEqual("0x402000", data_symbols[0].address)
        self.assertEqual("0x402000", selector)

    def test_data_symbol_range_covers_interior_owner_entry_address(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_data_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["data_symbols"][0]["byte_length"] = 20
            path.write_text(json.dumps(data), encoding="utf-8")
            manifest = load_manifest(path)

        target, functions, data_symbols, selector = find_target([manifest], "0x402008")
        coverage = covering_data_symbols([manifest], "0x402008")

        self.assertIs(target, manifest)
        self.assertEqual((), functions)
        self.assertEqual(1, len(data_symbols))
        self.assertEqual("0x402000", data_symbols[0].address)
        self.assertEqual("0x402008", selector)
        self.assertEqual([(manifest, data_symbols[0])], coverage)

    def test_load_manifest_rejects_invalid_data_byte_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_data_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["data_symbols"][0]["byte_length"] = 0
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "byte_length"):
                load_manifest(path)

    def test_build_compile_command_records_flags_and_include_paths(self):
        target = VerifyTarget(
            name="sample",
            description="sample target",
            source_filename="sample_verify.cpp",
            source_text="int main() { return 0; }\n",
            source_from="",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="",
            compiler_env="",
            compiler_flags=("/nologo", "/TP", "/O2", "/FAcs"),
            include_dirs=("src",),
            source_files=("src/sample.cpp",),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("sample.json"),
        )

        command = build_compile_command(
            target,
            Path("C:/work/RecoilRebuild/build/vc5-verify/sample/sample_verify.cpp"),
            Path("C:/toolchains/VC5SP3/vc5sp3-env.cmd"),
            Path("C:/work/RecoilRebuild/build/vc5-verify/sample"),
        )

        self.assertIn('call "C:\\toolchains\\VC5SP3\\vc5sp3-env.cmd"', command)
        self.assertIn("/nologo /TP /O2 /FAcs", command)
        self.assertIn("/I ", command)
        self.assertIn("\\src", command)
        self.assertTrue(command.endswith('/c "sample_verify.cpp"'))

    def test_duplicate_addresses_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"].append(dict(data["functions"][0]))
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaises(ValueError):
                load_manifest(path)

    def test_load_manifest_rejects_text_compare_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["compare_mode"] = "text"
            data["trim_trailing_nops"] = False
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "compare_mode"):
                load_manifest(path)

    def test_load_manifest_accepts_function_bn_byte_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0]["bn_byte_length"] = 1088
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual(1088, manifest.functions[0].bn_byte_length)

    def test_load_manifest_rejects_invalid_bn_byte_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0]["bn_byte_length"] = 0
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "bn_byte_length"):
                load_manifest(path)

    def test_load_manifest_accepts_function_vc5_byte_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0]["vc5_byte_length"] = 638
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual(638, manifest.functions[0].vc5_byte_length)

    def test_load_manifest_rejects_invalid_vc5_byte_length(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["functions"][0]["vc5_byte_length"] = 0
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "vc5_byte_length"):
                load_manifest(path)

    def test_coverage_helpers_report_address_matches_and_skeleton(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = load_manifest(write_manifest(Path(tmp)))

        matches = covering_targets([manifest], "0x401000")
        skeleton = manifest_skeleton("0x401234")

        self.assertEqual(1, len(matches))
        self.assertIs(matches[0][0], manifest)
        self.assertEqual("0x401234", skeleton["functions"][0]["address"])
        self.assertEqual("verify_401234", skeleton["name"])
        self.assertEqual("vc5_o2_ob0_facs", skeleton["compiler_profile"])

    def test_missing_explanation_uses_data_symbol_skeleton_for_owner_data_entry(self):
        with tempfile.TemporaryDirectory() as tmp:
            owners_path = Path(tmp) / "SOURCE_OWNERS.json"
            write_ledger(
                owners_path,
                owner_record(
                    "zinterp.unresolved_defaults",
                    kind="data-owner",
                    data=(("0x4e5954", "g_zInterp_UnresolvedFloatDefaults"),),
                    tiers={"0x4e5954": "X"},
                    source_paths=("src/GameZRecoil/zInterp/zinterp_parse.cpp",),
                    address_metadata={
                        "0x4e5954": {
                            "name": "g_zInterp_UnresolvedFloatDefaults",
                            "section": ".data",
                            "size": 252,
                            "type": "float[0x3f]",
                        }
                    },
                    blocker="data owner audit pending",
                ),
            )
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                print_missing_explanation([], "0x4E5954", owners_path=owners_path)

        output = stdout.getvalue()
        skeleton = json.loads(output.split("Suggested starting point:\n", 1)[1])
        self.assertNotIn("functions", skeleton)
        self.assertEqual("src/GameZRecoil/zInterp/zinterp_parse.cpp", skeleton["source_from"])
        self.assertEqual("vc5_o2_ob0_facs", skeleton["compiler_profile"])
        self.assertEqual("0x4e5954", skeleton["data_symbols"][0]["address"])
        self.assertEqual("g_zInterp_UnresolvedFloatDefaults", skeleton["data_symbols"][0]["bn_name"])
        self.assertEqual(252, skeleton["data_symbols"][0]["byte_length"])
        self.assertIn("symbol_regex", skeleton["data_symbols"][0])

    def test_load_manifest_accepts_compiler_profile(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data.pop("compiler_flags")
            data["compiler_profile"] = "vc5_o2_ob0_facs"
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual("vc5_o2_ob0_facs", manifest.compiler_profile)
        self.assertIn("VC5SP3", manifest.compiler_env)
        self.assertIn("/O2", manifest.compiler_flags)

    def test_source_compile_profiles_select_only_the_exact_hud_translation_unit(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_source_profile_order_manifest(
                Path(tmp),
                source_compile_profiles={
                    "src/Battlesport/hud.cpp": "vc5_o2_ob0_md_gx_fastcall_facs"
                },
            )
            target = load_manifest(path, enforce_source_policy=False)

        hud_profile, hud_flags = effective_source_compile_context(
            target,
            "src/Battlesport/hud.cpp",
        )
        about_profile, about_flags = effective_source_compile_context(
            target,
            "src/Battlesport/about.cpp",
        )
        hud_command = build_compile_command(
            target,
            Path("hud.cpp"),
            Path("vc5sp3-env.cmd"),
            source_from="src/Battlesport/hud.cpp",
        )
        about_command = build_compile_command(
            target,
            Path("about.cpp"),
            Path("vc5sp3-env.cmd"),
            source_from="src/Battlesport/about.cpp",
        )

        self.assertEqual("vc5_o2_ob0_md_gx_fastcall_facs", hud_profile)
        self.assertIn("/Gr", hud_flags)
        self.assertIn(" /Gr ", f" {hud_command} ")
        self.assertEqual("vc5_o2_ob0_md_gx_facs", about_profile)
        self.assertNotIn("/Gr", about_flags)
        self.assertNotIn(" /Gr ", f" {about_command} ")

    def test_source_compile_profiles_reject_invalid_exact_paths_profiles_and_duplicates(self):
        invalid_cases = (
            (
                {"src/Battlesport/*.cpp": "vc5_o2_ob0_md_gx_fastcall_facs"},
                "exact source paths",
            ),
            (
                {"src/Battlesport/briefing.cpp": "vc5_o2_ob0_md_gx_fastcall_facs"},
                "unconfigured source",
            ),
            (
                {"src/Battlesport/hud.cpp": "no_such_profile"},
                "unknown source_compile_profiles profile",
            ),
            (
                {
                    "src/Battlesport/hud.cpp": "vc5_o2_ob0_md_gx_fastcall_facs",
                    "SRC\\BATTLESPORT\\HUD.CPP": "vc5_o2_ob0_md_gx_fastcall_facs",
                },
                "forward slashes",
            ),
        )
        for index, (mappings, expected) in enumerate(invalid_cases):
            with self.subTest(expected=expected):
                with tempfile.TemporaryDirectory() as tmp:
                    path = write_source_profile_order_manifest(
                        Path(tmp),
                        name=f"invalid_source_profiles_{index}",
                        source_compile_profiles=mappings,
                    )
                    with self.assertRaisesRegex(ValueError, expected):
                        load_manifest(path, enforce_source_policy=False)

    def test_source_scoped_profile_guard_does_not_reject_unrelated_source_mapping(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = add_about_scoped_profile_guard(
                write_source_profile_order_manifest(
                    Path(tmp),
                    source_compile_profiles={
                        "src/Battlesport/hud.cpp": "vc5_o2_ob0_md_gx_fastcall_facs",
                    },
                )
            )
            target = load_manifest(path, enforce_source_policy=False)

        hud_profile, hud_flags = effective_source_compile_context(
            target,
            "src/Battlesport/hud.cpp",
        )
        about_profile, about_flags = effective_source_compile_context(
            target,
            "src/Battlesport/about.cpp",
        )
        self.assertEqual("vc5_o2_ob0_md_gx_fastcall_facs", hud_profile)
        self.assertIn("/Gr", hud_flags)
        self.assertEqual("vc5_o2_ob0_md_gx_facs", about_profile)
        self.assertNotIn("/Gr", about_flags)

    def test_source_scoped_profile_guard_rejects_outside_accepted_profile_on_guarded_source(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = add_about_scoped_profile_guard(
                write_source_profile_order_manifest(
                    Path(tmp),
                    source_compile_profiles={
                        "src/Battlesport/about.cpp": "vc5_o2_ob0_md_gx_fastcall_facs",
                    },
                )
            )
            with self.assertRaisesRegex(
                ValueError,
                "not listed in profile_guard.accepted_profiles",
            ):
                load_manifest(path, enforce_source_policy=False)

    def test_source_scoped_profile_guard_rejects_disqualified_profile_on_guarded_source(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = add_about_scoped_profile_guard(
                write_source_profile_order_manifest(
                    Path(tmp),
                    source_compile_profiles={
                        "src/Battlesport/about.cpp": "vc5_o2_ob1_md_gx_facs",
                    },
                )
            )
            with self.assertRaisesRegex(ValueError, "disqualified by profile_guard"):
                load_manifest(path, enforce_source_policy=False)

    def test_source_scoped_profile_guard_keeps_unrelated_mapping_validation_strict(self):
        invalid_cases = (
            (
                {"src/Battlesport/hud.cpp": "no_such_profile"},
                "unknown source_compile_profiles profile",
            ),
            (
                {"src/Battlesport/briefing.cpp": "vc5_o2_ob0_md_gx_fastcall_facs"},
                "unconfigured source",
            ),
        )
        for index, (mappings, expected) in enumerate(invalid_cases):
            with self.subTest(expected=expected):
                with tempfile.TemporaryDirectory() as tmp:
                    path = add_about_scoped_profile_guard(
                        write_source_profile_order_manifest(
                            Path(tmp),
                            name=f"guarded_invalid_source_profiles_{index}",
                            source_compile_profiles=mappings,
                        )
                    )
                    with self.assertRaisesRegex(ValueError, expected):
                        load_manifest(path, enforce_source_policy=False)

    def test_compile_context_from_inherits_source_profiles_and_reloads_live_context(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            context_path = root / "final_context.json"
            context = {
                "sources": ["src/Battlesport/hud.cpp", "src/Battlesport/about.cpp"],
                "vc5_env": "D:/Recoil Project/Compiler/VC5SP3/vc5sp3-env.cmd",
                "compile_flags": [
                    "/nologo", "/TP", "/W3", "/G5", "/O2", "/Ob0", "/MD", "/GX", "/Zp4"
                ],
                "defines": ["WIN32"],
                "include_dirs": ["tools/_recoil/compat/include"],
                "source_compile_profiles": {
                    "src/Battlesport/hud.cpp": "vc5_o2_ob0_md_gx_fastcall_facs"
                },
            }
            context_path.write_text(json.dumps(context), encoding="utf-8")
            manifest_path = write_source_profile_order_manifest(
                root,
                compile_context_from=str(context_path),
            )
            first = load_manifest(manifest_path, enforce_source_policy=False)
            hud_profile, hud_flags = effective_source_compile_context(
                first,
                "src/Battlesport/hud.cpp",
            )

            context["source_compile_profiles"] = {
                "src/Battlesport/about.cpp": "vc5_o2_ob0_md_gx_fastcall_facs"
            }
            context_path.write_text(json.dumps(context), encoding="utf-8")
            second = load_manifest(manifest_path, enforce_source_policy=False)
            second_hud_profile, second_hud_flags = effective_source_compile_context(
                second,
                "src/Battlesport/hud.cpp",
            )

        self.assertEqual("vc5_o2_ob0_md_gx_fastcall_facs", hud_profile)
        self.assertIn("/Gr", hud_flags)
        self.assertIn("/DWIN32", hud_flags)
        self.assertIn("/FAcs", hud_flags)
        self.assertNotIn("/I", hud_flags)
        self.assertNotEqual(hud_profile, second_hud_profile)
        self.assertNotIn("/Gr", second_hud_flags)

    def test_source_compile_profiles_participate_in_compile_grouping_and_target_override_is_global(self):
        with tempfile.TemporaryDirectory() as tmp:
            mapped = load_manifest(
                write_source_profile_order_manifest(
                    Path(tmp),
                    source_compile_profiles={
                        "src/Battlesport/hud.cpp": "vc5_o2_ob0_md_gx_fastcall_facs"
                    },
                ),
                enforce_source_policy=False,
            )
        unmapped = replace(mapped, source_compile_profiles=(), source_compile_flags=())
        groups = group_selections_by_compile_key(
            [
                VerifySelection(target=mapped, functions=()),
                VerifySelection(target=unmapped, functions=()),
            ],
            Path("D:/Recoil Project/Compiler/VC5SP3/vc5sp3-env.cmd"),
        )
        overridden = with_compiler_profile_override(mapped, "vc5_o2_ob1_md_gx_facs")

        self.assertEqual(2, len(groups))
        self.assertEqual((), overridden.source_compile_profiles)
        self.assertEqual((), overridden.source_compile_flags)
        self.assertEqual(
            "vc5_o2_ob1_md_gx_facs",
            effective_source_compile_context(overridden, "src/Battlesport/hud.cpp")[0],
        )

    def test_manifest_without_source_compile_profiles_keeps_target_wide_flags(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = load_manifest(
                write_source_profile_order_manifest(Path(tmp)),
                enforce_source_policy=False,
            )

        profile, flags = effective_source_compile_context(target, "src/Battlesport/hud.cpp")
        self.assertEqual("vc5_o2_ob0_md_gx_facs", profile)
        self.assertEqual(target.compiler_flags, flags)
        self.assertEqual((), target.source_compile_profiles)

    def test_compiler_profile_rejects_raw_env_or_flags(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["compiler_profile"] = "vc5_o2_ob0_facs"
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "mutually exclusive"):
                load_manifest(path)

    def test_load_manifest_accepts_profile_guard(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["profile_guard"] = {
                "scope": "src/sample.cpp",
                "policy": "Preserve confirmed sentinel byte matches before profile overrides.",
                "accepted_profiles": [
                    {
                        "profile": "vc5_o2_ob0_facs",
                        "sentinel_addresses": ["0x401000"],
                        "evidence": "0x401000 zero mismatches",
                    }
                ],
                "disqualified_profiles": [
                    {
                        "profile": "vc5_o1_oyminus_ob0_md_facs",
                        "sentinel_addresses": ["0x401000"],
                        "evidence": "0x401000 mismatched",
                    }
                ],
            }
            path.write_text(json.dumps(data), encoding="utf-8")

            manifest = load_manifest(path)

        self.assertEqual("src/sample.cpp", manifest.profile_guard.scope)
        self.assertEqual("vc5_o2_ob0_facs", manifest.profile_guard.accepted_profiles[0].profile)
        self.assertEqual("vc5_o1_oyminus_ob0_md_facs", manifest.profile_guard.disqualified_profiles[0].profile)
        self.assertEqual(("0x401000",), manifest.profile_guard.disqualified_profiles[0].sentinel_addresses)

    def test_load_manifest_rejects_default_disqualified_profile(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data.pop("compiler_flags")
            data["compiler_profile"] = "vc5_o2_ob0_facs"
            data["profile_guard"] = {
                "disqualified_profiles": [
                    {
                        "profile": "vc5_o2_ob0_facs",
                        "sentinel_addresses": ["0x401000"],
                        "evidence": "0x401000 mismatched",
                    }
                ]
            }
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "disqualified by profile_guard"):
                load_manifest(path)

    def test_load_manifest_rejects_default_profile_outside_accepted_profile_guard(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data.pop("compiler_flags")
            data["compiler_profile"] = "vc5_o2_ob1_facs"
            data["profile_guard"] = {
                "accepted_profiles": [
                    {
                        "profile": "vc5_o2_ob0_facs",
                        "sentinel_addresses": ["0x401000"],
                        "evidence": "0x401000 matched",
                    }
                ]
            }
            path.write_text(json.dumps(data), encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "not listed in profile_guard.accepted_profiles"):
                load_manifest(path)

    def test_compiler_profile_override_rejects_disqualified_profile(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["profile_guard"] = {
                "disqualified_profiles": [
                    {
                        "profile": "vc5_o2_ob1_facs",
                        "sentinel_addresses": ["0x401000"],
                        "evidence": "0x401000 mismatched",
                    }
                ]
            }
            path.write_text(json.dumps(data), encoding="utf-8")
            manifest = load_manifest(path)

        with self.assertRaisesRegex(ValueError, "disqualified by profile_guard"):
            with_compiler_profile_override(manifest, "vc5_o2_ob1_facs")

        allowed = with_compiler_profile_override(
            manifest,
            "vc5_o2_ob1_facs",
            allow_disqualified_profile=True,
        )
        self.assertEqual("vc5_o2_ob1_facs", allowed.compiler_profile)

    def test_compiler_profile_override_rejects_profile_outside_accepted_profile_guard(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp))
            data = json.loads(path.read_text(encoding="utf-8"))
            data["profile_guard"] = {
                "accepted_profiles": [
                    {
                        "profile": "vc5_o2_ob0_facs",
                        "sentinel_addresses": ["0x401000"],
                        "evidence": "0x401000 matched",
                    }
                ]
            }
            path.write_text(json.dumps(data), encoding="utf-8")
            manifest = load_manifest(path)

        with self.assertRaisesRegex(ValueError, "not listed in profile_guard.accepted_profiles"):
            with_compiler_profile_override(manifest, "vc5_o2_ob1_facs")

        allowed = with_compiler_profile_override(
            manifest,
            "vc5_o2_ob1_facs",
            allow_disqualified_profile=True,
        )
        self.assertEqual("vc5_o2_ob1_facs", allowed.compiler_profile)

    def test_group_selections_by_compile_key_reuses_identical_source_compiles(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            first = load_manifest(write_manifest(directory, "first"))
            second = load_manifest(write_manifest(directory, "second"))

        groups = group_selections_by_compile_key(
            [
                VerifySelection(target=first, functions=first.functions),
                VerifySelection(target=second, functions=second.functions),
            ],
            Path("C:/toolchains/VC5SP3/vc5sp3-env.cmd"),
        )

        self.assertEqual(1, len(groups))
        self.assertEqual(2, len(groups[0][1]))

    def test_run_target_allows_skipped_compile_only_byte_diagnostic(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            target = load_manifest(write_manifest(root))
            compiled = CompiledTarget(
                target=target,
                build_dir=root / "build",
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )

            with patch("_recoil.commands.vc5_verify.prepare_clean_build_dir", return_value=compiled.build_dir):
                with patch("_recoil.commands.vc5_verify.compile_target", return_value=(compiled, 0)):
                    with patch("_recoil.commands.vc5_verify.print_compiled_target_info"):
                        with patch("_recoil.commands.vc5_verify.run_function_order_checks", return_value=0):
                            with patch(
                                "_recoil.commands.vc5_verify.run_translation_unit_function_order_checks",
                                return_value=0,
                            ):
                                rc = run_target(
                                    target=target,
                                    selected_functions=target.functions,
                                    selected_data_symbols=target.data_symbols,
                                    build_root=root / "build-root",
                                    vc5_env=root / "env.cmd",
                                    bridge_url="http://example.invalid",
                                    skip_bn_compare=True,
                                    bn_call_budget=10,
                                )

        self.assertEqual(0, rc)

    def test_run_target_combines_translation_unit_diagnostic_with_bn_byte_compare(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest = load_manifest(write_manifest(root))
            target = replace(
                manifest,
                check_translation_unit_function_order=True,
                translation_unit_function_order=(
                    SimpleNamespace(source_from="src/sample.cpp", functions=manifest.functions),
                ),
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root / "build",
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            result = VerificationResult(
                target=target,
                function=target.functions[0],
                item_kind="function",
                mode="bytes",
                mismatches=0,
                relocation_or_text_metric=0,
                secondary_metric=0,
                bn_size_or_normalized=1,
                vc5_size_or_diff_count=1,
                evidence_path=root / "diff.txt",
                triage_path=root / "triage.txt",
                comparison=SimpleNamespace(),
            )

            for translation_unit_rc, expected_rc in ((0, 0), (1, 1)):
                with self.subTest(translation_unit_rc=translation_unit_rc):
                    with patch(
                        "_recoil.commands.vc5_verify.prepare_clean_build_dir",
                        return_value=compiled.build_dir,
                    ):
                        with patch(
                            "_recoil.commands.vc5_verify.compile_target",
                            return_value=(compiled, 0),
                        ):
                            with patch("_recoil.commands.vc5_verify.print_compiled_target_info"):
                                with patch(
                                    "_recoil.commands.vc5_verify.run_function_order_checks",
                                    return_value=0,
                                ):
                                    with patch(
                                        "_recoil.commands.vc5_verify.run_translation_unit_function_order_checks",
                                        return_value=translation_unit_rc,
                                    ) as translation_unit_check:
                                        with patch(
                                            "_recoil.commands.vc5_verify.compare_compiled_selections",
                                            return_value=([result], 0),
                                        ) as compare:
                                            with patch(
                                                "_recoil.commands.vc5_verify.print_verification_summary"
                                            ), patch("_recoil.commands.vc5_verify.print_evidence_block"):
                                                rc = run_target(
                                                    target=target,
                                                    selected_functions=target.functions,
                                                    selected_data_symbols=(),
                                                    build_root=root / "build-root",
                                                    vc5_env=root / "env.cmd",
                                                    bridge_url="http://example.invalid",
                                                    skip_bn_compare=False,
                                                    bn_call_budget=10,
                                                )

                    self.assertEqual(expected_rc, rc)
                    translation_unit_check.assert_called_once()
                    compare.assert_called_once()

    def test_profile_sweep_runs_function_order_check_in_compile_only_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest = load_manifest(write_manifest(root))
            target = manifest.__class__(
                **{
                    **manifest.__dict__,
                    "check_function_order": True,
                }
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            stdout = io.StringIO()

            with patch("_recoil.commands.vc5_verify.compile_target", return_value=(compiled, 0)):
                with patch("_recoil.commands.vc5_verify.run_function_order_checks", return_value=1) as order_check:
                    with contextlib.redirect_stdout(stdout):
                        rc = run_profile_sweep(
                            target=target,
                            selected_functions=target.functions,
                            selected_data_symbols=(),
                            profile_names=["vc5_o2_ob0_facs"],
                            build_root=root / "build",
                            vc5_env=root / "env.cmd",
                            bridge_url="http://example.invalid",
                            skip_bn_compare=True,
                            bn_call_budget=10,
                        )

        self.assertEqual(1, rc)
        self.assertEqual(1, order_check.call_count)

    def test_profile_sweep_skips_disqualified_profiles(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["profile_guard"] = {
                "disqualified_profiles": [
                    {
                        "profile": "vc5_o2_ob0_facs",
                        "sentinel_addresses": ["0x401000"],
                        "evidence": "0x401000 mismatched",
                    }
                ]
            }
            path.write_text(json.dumps(data), encoding="utf-8")
            target = load_manifest(path)
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            stdout = io.StringIO()

            with patch("_recoil.commands.vc5_verify.compile_target", return_value=(compiled, 0)) as compile_target:
                with patch("_recoil.commands.vc5_verify.run_function_order_checks", return_value=0):
                    with contextlib.redirect_stdout(stdout):
                        rc = run_profile_sweep(
                            target=target,
                            selected_functions=target.functions,
                            selected_data_symbols=(),
                            profile_names=["vc5_o2_ob0_facs", "vc5_o2_ob1_facs"],
                            build_root=root / "build",
                            vc5_env=root / "env.cmd",
                            bridge_url="http://example.invalid",
                            skip_bn_compare=True,
                            bn_call_budget=10,
                        )

        self.assertEqual(0, rc)
        self.assertEqual(1, compile_target.call_count)
        self.assertEqual("vc5_o2_ob1_facs", compile_target.call_args.kwargs["target"].compiler_profile)
        self.assertIn("Profile sweep skipped", stdout.getvalue())

    def test_profile_sweep_skips_profiles_outside_accepted_profile_guard(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = write_manifest(root)
            data = json.loads(path.read_text(encoding="utf-8"))
            data["profile_guard"] = {
                "accepted_profiles": [
                    {
                        "profile": "vc5_o2_ob1_facs",
                        "sentinel_addresses": ["0x401000"],
                        "evidence": "0x401000 matched",
                    }
                ]
            }
            path.write_text(json.dumps(data), encoding="utf-8")
            target = load_manifest(path)
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            stdout = io.StringIO()

            with patch("_recoil.commands.vc5_verify.compile_target", return_value=(compiled, 0)) as compile_target:
                with patch("_recoil.commands.vc5_verify.run_function_order_checks", return_value=0):
                    with contextlib.redirect_stdout(stdout):
                        rc = run_profile_sweep(
                            target=target,
                            selected_functions=target.functions,
                            selected_data_symbols=(),
                            profile_names=["vc5_o2_ob0_facs", "vc5_o2_ob1_facs"],
                            build_root=root / "build",
                            vc5_env=root / "env.cmd",
                            bridge_url="http://example.invalid",
                            skip_bn_compare=True,
                            bn_call_budget=10,
                        )

        self.assertEqual(0, rc)
        self.assertEqual(1, compile_target.call_count)
        self.assertEqual("vc5_o2_ob1_facs", compile_target.call_args.kwargs["target"].compiler_profile)
        self.assertIn("not listed in profile_guard.accepted_profiles", stdout.getvalue())

    def test_profile_sweep_runs_translation_unit_order_check_in_compile_only_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest = load_manifest(write_manifest(root))
            target = manifest.__class__(
                **{
                    **manifest.__dict__,
                    "check_translation_unit_function_order": True,
                    "translation_unit_function_order": (
                        SimpleNamespace(source_from="src/sample.cpp", functions=manifest.functions),
                    ),
                }
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )

            with patch("_recoil.commands.vc5_verify.compile_target", return_value=(compiled, 0)):
                with patch("_recoil.commands.vc5_verify.run_function_order_checks", return_value=0):
                    with patch(
                        "_recoil.commands.vc5_verify.run_translation_unit_function_order_checks",
                        return_value=1,
                    ) as order_check:
                        rc = run_profile_sweep(
                            target=target,
                            selected_functions=target.functions,
                            selected_data_symbols=(),
                            profile_names=["vc5_o2_ob0_facs"],
                            build_root=root / "build",
                            vc5_env=root / "env.cmd",
                            bridge_url="http://example.invalid",
                            skip_bn_compare=True,
                            bn_call_budget=10,
                        )

        self.assertEqual(1, rc)
        self.assertEqual(1, order_check.call_count)

    def test_translation_unit_order_allows_bn_compare_in_profile_sweep(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest = load_manifest(write_manifest(root))
            target = manifest.__class__(
                **{
                    **manifest.__dict__,
                    "check_translation_unit_function_order": True,
                    "translation_unit_function_order": (
                        SimpleNamespace(source_from="src/sample.cpp", functions=manifest.functions),
                    ),
                }
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root / "build",
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            result = VerificationResult(
                target=target,
                function=target.functions[0],
                item_kind="function",
                mode="bytes",
                mismatches=0,
                relocation_or_text_metric=0,
                secondary_metric=0,
                bn_size_or_normalized=1,
                vc5_size_or_diff_count=1,
                evidence_path=root / "diff.txt",
                triage_path=root / "triage.txt",
                comparison=SimpleNamespace(),
            )

            with patch("_recoil.commands.vc5_verify.make_binja_bridge", return_value=SimpleNamespace()):
                with patch("_recoil.commands.vc5_verify.compile_target", return_value=(compiled, 0)):
                    with patch("_recoil.commands.vc5_verify.run_function_order_checks", return_value=0):
                        with patch(
                            "_recoil.commands.vc5_verify.run_translation_unit_function_order_checks",
                            return_value=0,
                        ) as translation_unit_check:
                            with patch(
                                "_recoil.commands.vc5_verify.compare_compiled_selections",
                                return_value=([result], 0),
                            ) as compare:
                                rc = run_profile_sweep(
                                    target=target,
                                    selected_functions=target.functions,
                                    selected_data_symbols=(),
                                    profile_names=["vc5_o2_ob0_facs"],
                                    build_root=root / "build-root",
                                    vc5_env=root / "env.cmd",
                                    bridge_url="http://example.invalid",
                                    skip_bn_compare=False,
                                    bn_call_budget=10,
                                )

        self.assertEqual(0, rc)
        translation_unit_check.assert_called_once()
        compare.assert_called_once()

    def test_profile_sweep_order_failure_is_not_hidden_by_byte_success(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest = load_manifest(write_manifest(root))
            target = manifest.__class__(
                **{
                    **manifest.__dict__,
                    "check_function_order": True,
                }
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            comparison = ObjectByteComparison(
                address=target.functions[0].address,
                symbol=target.functions[0].symbol,
                obj_path=compiled.obj_path,
                bn_path=root / "bn.bytes",
                vc5_path=root / "vc5.bytes",
                mask_path=root / "mask.txt",
                diff_path=root / "diff.txt",
                triage_path=root / "triage.txt",
                text_diff_path=None,
                classified_text_path=None,
                mismatch_count=0,
                relocation_masked_bytes=0,
                bn_size=1,
                vc5_size=1,
                trailing_bn_nops_trimmed=0,
                trailing_vc5_nops_trimmed=0,
                relocation_identity_path=None,
            )
            result = VerificationResult(
                target=target,
                function=target.functions[0],
                item_kind="function",
                mode="bytes",
                mismatches=0,
                relocation_or_text_metric=0,
                secondary_metric=0,
                bn_size_or_normalized=1,
                vc5_size_or_diff_count=1,
                evidence_path=root / "diff.txt",
                triage_path=root / "triage.txt",
                comparison=comparison,
            )
            stdout = io.StringIO()

            with patch("_recoil.commands.vc5_verify.compile_target", return_value=(compiled, 0)):
                with patch("_recoil.commands.vc5_verify.run_function_order_checks", return_value=1):
                    with patch("_recoil.commands.vc5_verify.compare_compiled_selections", return_value=([result], 0)):
                        with contextlib.redirect_stdout(stdout):
                            rc = run_profile_sweep(
                                target=target,
                                selected_functions=target.functions,
                                selected_data_symbols=(),
                                profile_names=["vc5_o2_ob0_facs"],
                                build_root=root / "build",
                                vc5_env=root / "env.cmd",
                                bridge_url="http://example.invalid",
                                skip_bn_compare=False,
                                bn_call_budget=10,
                            )

        self.assertEqual(1, rc)
        self.assertIn("ORDER-FAIL", stdout.getvalue())

    def test_selected_targets_for_source_from_matches_resolved_paths(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            manifest = load_manifest(write_manifest(directory, "sample"))
            source_path = Path(manifest.source_from)

            matches = selected_targets_for_source_from([manifest], str(source_path.resolve()))

        self.assertEqual([manifest.name], [selection.target.name for selection in matches])

    def test_parse_targets_json_accepts_list_and_rejects_empty_batch(self):
        self.assertEqual(["0x401000", "sample"], parse_targets_json('["0x401000", "sample"]'))

        with self.assertRaisesRegex(ValueError, "requires at least one target"):
            parse_targets_json("[]")

    def test_main_routes_multiple_explicit_selectors_to_batch_runner(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            write_address_manifest(manifest_dir, "first", "0x401000", "First")
            write_address_manifest(manifest_dir, "second", "0x401020", "Second")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_batch", return_value=0) as batch:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(
                        [
                            "0x401000",
                            "second",
                            "--skip-bn-compare",
                            "--manifest-dir",
                            str(manifest_dir),
                        ]
                    )

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertEqual("", stderr.getvalue())
        selections = batch.call_args.kwargs["selections"]
        self.assertEqual(["first", "second"], [selection.target.name for selection in selections])
        self.assertEqual(["0x401000"], [function.address for function in selections[0].functions])
        self.assertTrue(batch.call_args.kwargs["skip_bn_compare"])

    def test_main_routes_targets_json_to_batch_runner(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            write_address_manifest(manifest_dir, "first", "0x401000", "First")
            write_address_manifest(manifest_dir, "second", "0x401020", "Second")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_batch", return_value=0) as batch:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(
                        [
                            "--targets-json",
                            '["0x401000", "second"]',
                            "--skip-bn-compare",
                            "--manifest-dir",
                            str(manifest_dir),
                        ]
                    )

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertEqual(["first", "second"], [selection.target.name for selection in batch.call_args.kwargs["selections"]])

    def test_main_routes_target_alias_to_single_target_runner(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            write_manifest(manifest_dir, "sample")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_target", return_value=0) as target_runner:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(
                        [
                            "--target",
                            "sample",
                            "--skip-bn-compare",
                            "--manifest-dir",
                            str(manifest_dir),
                        ]
                    )

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertEqual("", stderr.getvalue())
        self.assertEqual("sample", target_runner.call_args.kwargs["target"].name)
        self.assertTrue(target_runner.call_args.kwargs["skip_bn_compare"])

    def test_main_routes_auto_chunk_to_batch_runner(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            write_address_manifest(manifest_dir, "first", "0x401000", "First")
            write_address_manifest(manifest_dir, "second", "0x401020", "Second")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_batch", return_value=0) as batch:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(
                        [
                            "0x401000",
                            "second",
                            "--auto-chunk",
                            "--bn-call-budget",
                            "5",
                            "--manifest-dir",
                            str(manifest_dir),
                        ]
                    )

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertIsNone(batch.call_args.kwargs["chunk_size"])
        self.assertTrue(batch.call_args.kwargs["auto_chunk"])
        self.assertEqual(5, batch.call_args.kwargs["bn_call_budget"])

    def test_main_routes_owner_selector_to_complete_batch(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            owners = root / "SOURCE_OWNERS.json"
            write_owner_ledger(owners, ["0x401000", "0x401020"])
            write_address_manifest(manifest_dir, "first", "0x401000", "First")
            write_address_manifest(manifest_dir, "second", "0x401020", "Second")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_batch", return_value=0) as batch:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(
                        [
                            "--owner",
                            "test.owner",
                            "--progress",
                            str(owners),
                            "--manifest-dir",
                            str(manifest_dir),
                            "--skip-bn-compare",
                        ]
                    )

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertIn("owner_vc5_scope", stdout.getvalue())
        self.assertIn("linked_owner_entries: 2", stdout.getvalue())
        self.assertNotIn("linked_plan_entries", stdout.getvalue())
        selections = batch.call_args.kwargs["selections"]
        self.assertEqual(["first", "second"], [selection.target.name for selection in selections])
        self.assertEqual(2, sum(len(selection.functions) for selection in selections))
        self.assertTrue(batch.call_args.kwargs["skip_bn_compare"])

    def test_main_owner_selector_can_resolve_from_address_primary_owner(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            owners = root / "SOURCE_OWNERS.json"
            write_owner_ledger(owners, ["0x401000"])
            write_address_manifest(manifest_dir, "first", "0x401000", "First")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_batch", return_value=0) as batch:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(
                        [
                            "--owner",
                            "0x401000",
                            "--progress",
                            str(owners),
                            "--manifest-dir",
                            str(manifest_dir),
                            "--skip-bn-compare",
                        ]
                    )

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertIn("owner_id: test.owner", stdout.getvalue())
        self.assertEqual("first", batch.call_args.kwargs["selections"][0].target.name)

    def test_main_routes_data_owner_selector_to_batch(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            owners = root / "SOURCE_OWNERS.json"
            write_owner_ledger(owners, ["0x402000", "0x402004"], data=True)
            manifest_path = write_data_manifest(manifest_dir, "sample_data")
            data = json.loads(manifest_path.read_text(encoding="utf-8"))
            data["data_symbols"].append(
                {
                    "address": "0x00402004",
                    "symbol": "_second",
                    "name": "g_Second",
                    "byte_length": 4,
                }
            )
            manifest_path.write_text(json.dumps(data), encoding="utf-8")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_batch", return_value=0) as batch:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(
                        [
                            "--owner",
                            "test.owner",
                            "--progress",
                            str(owners),
                            "--manifest-dir",
                            str(manifest_dir),
                            "--auto-chunk",
                        ]
                    )

        self.assertEqual(0, rc, stderr.getvalue())
        selections = batch.call_args.kwargs["selections"]
        self.assertEqual(1, len(selections))
        self.assertEqual(2, len(selections[0].data_symbols))
        self.assertTrue(batch.call_args.kwargs["auto_chunk"])

    def test_owner_scope_uses_one_data_symbol_for_covered_aggregate_fields(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            owners = root / "SOURCE_OWNERS.json"
            write_owner_ledger(owners, ["0x402000", "0x402004", "0x402008"], data=True)
            manifest_path = write_data_manifest(manifest_dir, "sample_data")
            data = json.loads(manifest_path.read_text(encoding="utf-8"))
            data["data_symbols"][0]["byte_length"] = 20
            manifest_path.write_text(json.dumps(data), encoding="utf-8")
            owner_doc = SourceOwnerDocument.load(owners)
            entry_index = OwnerEntryIndex.load(owners)
            manifests = load_manifests(manifest_dir)

            scope = resolve_owner_vc5_scope(
                owner_doc=owner_doc,
                entry_index=entry_index,
                manifests=manifests,
                owner_selector="test.owner",
            )

        self.assertEqual((), scope.issues)
        self.assertEqual(3, scope.data_entry_count)
        self.assertEqual(1, len(scope.selections))
        self.assertEqual(1, len(scope.selections[0].data_symbols))
        self.assertEqual("0x402000", scope.selections[0].data_symbols[0].address)

    def test_owner_scope_prefers_entry_target_for_duplicate_data_coverage(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            owners = root / "SOURCE_OWNERS.json"
            write_owner_ledger(owners, ["0x402000"], data=True)
            write_data_manifest(manifest_dir, "test_owner_data")
            write_data_manifest(manifest_dir, "diagnostic_data")
            owner_doc = SourceOwnerDocument.load(owners)
            entry_index = OwnerEntryIndex.load(owners)
            manifests = load_manifests(manifest_dir)

            scope = resolve_owner_vc5_scope(
                owner_doc=owner_doc,
                entry_index=entry_index,
                manifests=manifests,
                owner_selector="test.owner",
            )

        self.assertEqual((), scope.issues)
        self.assertEqual(1, len(scope.selections))
        self.assertEqual("test_owner_data", scope.selections[0].target.name)
        self.assertEqual("0x402000", scope.selections[0].data_symbols[0].address)

    def test_owner_scope_reports_missing_coverage_before_compile(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            owners = root / "SOURCE_OWNERS.json"
            write_owner_ledger(owners, ["0x401000", "0x401020"])
            write_address_manifest(manifest_dir, "first", "0x401000", "First")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_batch") as batch:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(
                        [
                            "--owner",
                            "test.owner",
                            "--progress",
                            str(owners),
                            "--manifest-dir",
                            str(manifest_dir),
                        ]
                    )

        self.assertEqual(2, rc)
        self.assertIn("missing_or_invalid: 1", stdout.getvalue())
        self.assertIn("verify vc5 --explain-missing 0x401020", stdout.getvalue())
        batch.assert_not_called()

    def test_owner_scope_rejects_multiple_manifest_coverage(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            owners = root / "SOURCE_OWNERS.json"
            write_owner_ledger(owners, ["0x401000"])
            write_address_manifest(manifest_dir, "first", "0x401000", "First")
            write_address_manifest(manifest_dir, "duplicate", "0x401000", "Duplicate")
            owner_doc = SourceOwnerDocument.load(owners)
            entry_index = OwnerEntryIndex.load(owners)
            manifests = load_manifests(manifest_dir)

            scope = resolve_owner_vc5_scope(
                owner_doc=owner_doc,
                entry_index=entry_index,
                manifests=manifests,
                owner_selector="test.owner",
            )

        self.assertEqual(1, len(scope.issues))
        self.assertEqual("multiple-coverage", scope.issues[0].kind)

    def test_owner_scope_rejects_provider_rows(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifest_dir = root / "manifests"
            manifest_dir.mkdir()
            owners = root / "SOURCE_OWNERS.json"
            write_owner_ledger(owners, ["0x401000"], provider=True)
            write_address_manifest(manifest_dir, "first", "0x401000", "First")
            owner_doc = SourceOwnerDocument.load(owners)
            entry_index = OwnerEntryIndex.load(owners)
            manifests = load_manifests(manifest_dir)

            scope = resolve_owner_vc5_scope(
                owner_doc=owner_doc,
                entry_index=entry_index,
                manifests=manifests,
                owner_selector="test.owner",
            )

        self.assertEqual("provider-boundary", scope.issues[0].kind)

    def test_main_rejects_owner_selector_with_incompatible_options(self):
        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr):
            with self.assertRaises(SystemExit) as raised:
                vc5_main(["--owner", "test.owner", "sample"])

        self.assertEqual(2, raised.exception.code)
        self.assertIn("--owner cannot be combined", stderr.getvalue())

    def test_main_routes_chunk_size_to_single_target_runner(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            write_manifest(manifest_dir, "sample")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_target", return_value=0) as target_runner:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(["sample", "--chunk-size", "3", "--manifest-dir", str(manifest_dir)])

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertEqual(3, target_runner.call_args.kwargs["chunk_size"])

    def test_main_binary_option_overrides_single_target_binary(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            write_data_manifest(manifest_dir, "sample_data")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_target", return_value=0) as target_runner:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(
                        [
                            "sample_data",
                            "--binary",
                            "messages",
                            "--manifest-dir",
                            str(manifest_dir),
                        ]
                    )

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertEqual("messages", target_runner.call_args.kwargs["target"].target_binary)

    def test_main_auto_chunks_multi_data_symbol_target_by_default(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            manifest_path = write_data_manifest(manifest_dir, "sample_data")
            data = json.loads(manifest_path.read_text(encoding="utf-8"))
            data["data_symbols"].append(
                {
                    "address": "0x00402004",
                    "symbol": "_second",
                    "name": "g_Second",
                    "byte_length": 4,
                }
            )
            manifest_path.write_text(json.dumps(data), encoding="utf-8")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_target", return_value=0) as target_runner:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(["sample_data", "--manifest-dir", str(manifest_dir)])

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertIsNone(target_runner.call_args.kwargs["chunk_size"])
        self.assertTrue(target_runner.call_args.kwargs["auto_chunk"])

    def test_main_preserves_explicit_chunk_size_for_multi_data_symbol_target(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            manifest_path = write_data_manifest(manifest_dir, "sample_data")
            data = json.loads(manifest_path.read_text(encoding="utf-8"))
            data["data_symbols"].append(
                {
                    "address": "0x00402004",
                    "symbol": "_second",
                    "name": "g_Second",
                    "byte_length": 4,
                }
            )
            manifest_path.write_text(json.dumps(data), encoding="utf-8")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_target", return_value=0) as target_runner:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(["sample_data", "--chunk-size", "1", "--manifest-dir", str(manifest_dir)])

        self.assertEqual(0, rc, stderr.getvalue())
        self.assertEqual(1, target_runner.call_args.kwargs["chunk_size"])
        self.assertFalse(target_runner.call_args.kwargs["auto_chunk"])

    def test_main_rejects_duplicate_explicit_selectors(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            write_address_manifest(manifest_dir, "first", "0x401000", "First")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with patch("_recoil.commands.vc5_verify.run_batch") as batch:
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    rc = vc5_main(["0x00401000", "0x401000", "--manifest-dir", str(manifest_dir)])

        self.assertEqual(2, rc)
        self.assertIn("duplicate selectors", stderr.getvalue())
        batch.assert_not_called()

    def test_main_rejects_single_target_options_for_explicit_multi_selector_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            write_address_manifest(manifest_dir, "first", "0x401000", "First")
            write_address_manifest(manifest_dir, "second", "0x401020", "Second")

            stdout = io.StringIO()
            stderr = io.StringIO()
            with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                with self.assertRaises(SystemExit) as raised:
                    vc5_main(
                        [
                            "0x401000",
                            "0x401020",
                            "--compiler-profile",
                            "vc5_o2_ob0_facs",
                            "--manifest-dir",
                            str(manifest_dir),
                        ]
                    )

        self.assertEqual(2, raised.exception.code)

    def test_main_rejects_invalid_chunk_options(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp)
            write_manifest(manifest_dir, "sample")

            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr):
                with self.assertRaises(SystemExit) as raised:
                    vc5_main(
                        [
                            "sample",
                            "--auto-chunk",
                            "--chunk-size",
                            "3",
                            "--manifest-dir",
                            str(manifest_dir),
                        ]
                    )

        self.assertEqual(2, raised.exception.code)
        self.assertIn("--auto-chunk cannot be combined with --chunk-size", stderr.getvalue())

    def test_compare_compiled_selections_reuses_one_bridge_for_all_functions(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            diff_path = root / "diff.txt"
            triage_path = root / "triage.txt"
            diff_path.write_text("", encoding="utf-8")
            triage_path.write_text("", encoding="utf-8")
            target = VerifyTarget(
                name="sample",
                description="sample",
                source_filename="sample.cpp",
                source_text="",
                source_from="src/sample.cpp",
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="",
                compiler_env="",
                compiler_flags=(),
                include_dirs=(),
                source_files=(),
                generated_files=(),
                functions=(),
                data_symbols=(),
                manifest_path=root / "sample.json",
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            first = replace(
                replace_function(target, "0x401000", "First"),
                vc5_byte_length=7,
            )
            second = replace_function(target, "0x401020", "Second")
            bridge_ids: list[int] = []
            vc5_byte_lengths: list[int | None] = []

            def fake_compare(**kwargs):
                bridge_ids.append(id(kwargs["bridge"]))
                vc5_byte_lengths.append(kwargs["vc5_byte_length"])
                return ObjectByteComparison(
                    address=kwargs["address"],
                    symbol=kwargs["symbol"],
                    obj_path=kwargs["obj_path"],
                    bn_path=root / "bn.bytes",
                    vc5_path=root / "vc5.bytes",
                    mask_path=root / "mask.txt",
                    diff_path=diff_path,
                    triage_path=triage_path,
                    text_diff_path=None,
                    classified_text_path=None,
                    mismatch_count=0,
                    relocation_masked_bytes=0,
                    bn_size=1,
                    vc5_size=1,
                    trailing_bn_nops_trimmed=0,
                    trailing_vc5_nops_trimmed=0,
                )

            with patch("_recoil.commands.vc5_verify.compare_bn_to_obj", side_effect=fake_compare):
                results, rc = compare_compiled_selections(
                    compiled=compiled,
                    selections=[VerifySelection(target=target, functions=(first, second))],
                    bridge_url="http://example.invalid",
                )

        self.assertEqual(0, rc)
        self.assertEqual(2, len(results))
        self.assertEqual(2, len(bridge_ids))
        self.assertEqual(1, len(set(bridge_ids)))
        self.assertEqual([7, None], vc5_byte_lengths)

    def test_compare_compiled_selections_uses_messages_bridge_selector(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            target = VerifyTarget(
                name="messages_lookup_data",
                description="messages data",
                source_filename="messages.c",
                source_text="",
                source_from="src/Messages/messages.c",
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="",
                compiler_env="",
                compiler_flags=(),
                include_dirs=(),
                source_files=(),
                generated_files=(),
                functions=(),
                data_symbols=(),
                manifest_path=root / "messages_lookup_data.json",
                target_binary="messages",
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "messages.c",
                cod_path=root / "messages.cod",
                obj_path=root / "messages.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            data_symbol = VerifyDataSymbol(
                address="0x10006030",
                symbol="_g_MessagesLookupRows",
                name="g_MessagesLookupRows",
                byte_length=4,
            )

            def fake_compare(**kwargs):
                self.assertEqual("messages.bndb", kwargs["bridge"].binary)
                return ObjectByteComparison(
                    address=kwargs["address"],
                    symbol=kwargs["symbol"],
                    obj_path=kwargs["obj_path"],
                    bn_path=root / "bn.bytes",
                    vc5_path=root / "vc5.bytes",
                    mask_path=root / "mask.txt",
                    diff_path=root / "diff.txt",
                    triage_path=root / "triage.txt",
                    text_diff_path=None,
                    classified_text_path=None,
                    mismatch_count=0,
                    relocation_masked_bytes=0,
                    bn_size=4,
                    vc5_size=4,
                    trailing_bn_nops_trimmed=0,
                    trailing_vc5_nops_trimmed=0,
                )

            with patch("_recoil.commands.vc5_verify.compare_bn_data_to_obj", side_effect=fake_compare):
                results, rc = compare_compiled_selections(
                    compiled=compiled,
                    selections=[VerifySelection(target=target, functions=(), data_symbols=(data_symbol,))],
                    bridge_url="http://example.invalid",
                )

        self.assertEqual(0, rc)
        self.assertEqual(1, len(results))

    def test_compare_compiled_selections_uses_recoil_bridge_selector_for_default_target(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            target = VerifyTarget(
                name="sample_data",
                description="sample data",
                source_filename="sample.cpp",
                source_text="",
                source_from="src/sample.cpp",
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="",
                compiler_env="",
                compiler_flags=(),
                include_dirs=(),
                source_files=(),
                generated_files=(),
                functions=(),
                data_symbols=(),
                manifest_path=root / "sample_data.json",
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            data_symbol = VerifyDataSymbol(
                address="0x4dd1c8",
                symbol="_g_SampleData",
                name="g_SampleData",
                byte_length=11,
            )

            def fake_compare(**kwargs):
                self.assertEqual("Recoil.bndb", kwargs["bridge"].binary)
                return ObjectByteComparison(
                    address=kwargs["address"],
                    symbol=kwargs["symbol"],
                    obj_path=kwargs["obj_path"],
                    bn_path=root / "bn.bytes",
                    vc5_path=root / "vc5.bytes",
                    mask_path=root / "mask.txt",
                    diff_path=root / "diff.txt",
                    triage_path=root / "triage.txt",
                    text_diff_path=None,
                    classified_text_path=None,
                    mismatch_count=0,
                    relocation_masked_bytes=0,
                    bn_size=11,
                    vc5_size=11,
                    trailing_bn_nops_trimmed=0,
                    trailing_vc5_nops_trimmed=0,
                )

            with patch("_recoil.commands.vc5_verify.compare_bn_data_to_obj", side_effect=fake_compare):
                results, rc = compare_compiled_selections(
                    compiled=compiled,
                    selections=[VerifySelection(target=target, functions=(), data_symbols=(data_symbol,))],
                    bridge_url="http://example.invalid",
                )

        self.assertEqual(0, rc)
        self.assertEqual(1, len(results))

    def test_compare_compiled_selections_in_chunks_splits_items_and_resets_bridge(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            target = VerifyTarget(
                name="sample",
                description="sample",
                source_filename="sample.cpp",
                source_text="",
                source_from="src/sample.cpp",
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="",
                compiler_env="",
                compiler_flags=(),
                include_dirs=(),
                source_files=(),
                generated_files=(),
                functions=(),
                data_symbols=(),
                manifest_path=root / "sample.json",
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            first = replace_function(target, "0x401000", "First")
            second = replace_function(target, "0x401020", "Second")
            data_symbol = VerifyDataSymbol(
                address="0x402000",
                symbol="_third",
                name="third",
                byte_length=4,
            )
            chunk_counts: list[int] = []

            def fake_compare(**kwargs):
                chunk_counts.append(
                    sum(
                        len(selection.functions) + len(selection.data_symbols)
                        for selection in kwargs["selections"]
                    )
                )
                self.assertIsNone(kwargs["bridge"])
                self.assertEqual(2, kwargs["bn_call_budget"])
                return [], 0

            stdout = io.StringIO()
            with patch("_recoil.commands.vc5_verify.compare_compiled_selections", side_effect=fake_compare):
                with contextlib.redirect_stdout(stdout):
                    results, rc = compare_compiled_selections_in_chunks(
                        compiled=compiled,
                        selections=[
                            VerifySelection(
                                target=target,
                                functions=(first, second),
                                data_symbols=(data_symbol,),
                            )
                        ],
                        bridge_url="http://example.invalid",
                        bn_call_budget=2,
                        chunk_size=2,
                    )

        self.assertEqual(0, rc)
        self.assertEqual([], results)
        self.assertEqual([2, 1], chunk_counts)
        self.assertIn("VC compare chunks: 2 chunk(s), 3 item(s)", stdout.getvalue())

    def test_compare_compiled_selections_auto_chunks_by_estimated_bn_calls(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            target = VerifyTarget(
                name="sample",
                description="sample",
                source_filename="sample.cpp",
                source_text="",
                source_from="src/sample.cpp",
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="",
                compiler_env="",
                compiler_flags=(),
                include_dirs=(),
                source_files=(),
                generated_files=(),
                functions=(),
                data_symbols=(),
                manifest_path=root / "sample.json",
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            symbols = tuple(
                VerifyDataSymbol(
                    address=f"0x{0x402000 + index * 4:x}",
                    symbol=f"_data_{index}",
                    name=f"data_{index}",
                    byte_length=4,
                    object_offset=4 if index == 0 else 0,
                )
                for index in range(7)
            )
            chunk_counts: list[int] = []
            inspected_offsets: list[int] = []

            def fake_compare(**kwargs):
                chunk_counts.append(
                    sum(
                        len(selection.functions) + len(selection.data_symbols)
                        for selection in kwargs["selections"]
                    )
                )
                self.assertIsNone(kwargs["bridge"])
                self.assertEqual(5, kwargs["bn_call_budget"])
                return [], 0

            def fake_data_symbol_bytes(*_args, **kwargs):
                inspected_offsets.append(kwargs["object_offset"])
                return SimpleNamespace(relocations=(object(),))

            coff = SimpleNamespace(data_symbol_bytes=fake_data_symbol_bytes)
            stdout = io.StringIO()
            with patch("_recoil.commands.vc5_verify.CoffObject.from_path", return_value=coff):
                with patch("_recoil.commands.vc5_verify.compare_compiled_selections", side_effect=fake_compare):
                    with contextlib.redirect_stdout(stdout):
                        results, rc = compare_compiled_selections_in_chunks(
                            compiled=compiled,
                            selections=[
                                VerifySelection(
                                    target=target,
                                    functions=(),
                                    data_symbols=symbols,
                                )
                            ],
                            bridge_url="http://example.invalid",
                            bn_call_budget=5,
                            chunk_size=None,
                            auto_chunk=True,
                        )

        self.assertEqual(0, rc)
        self.assertEqual([], results)
        self.assertEqual([3, 3, 1], chunk_counts)
        self.assertEqual([4, 0, 0, 0, 0, 0, 0], inspected_offsets)
        self.assertIn("auto chunks bounded by BN call budget 5", stdout.getvalue())

    def test_compare_compiled_selections_compares_data_symbols(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            diff_path = root / "diff.txt"
            triage_path = root / "triage.txt"
            diff_path.write_text("", encoding="utf-8")
            triage_path.write_text("", encoding="utf-8")
            target = VerifyTarget(
                name="sample",
                description="sample",
                source_filename="sample.cpp",
                source_text="",
                source_from="src/sample.cpp",
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="",
                compiler_env="",
                compiler_flags=(),
                include_dirs=(),
                source_files=(),
                generated_files=(),
                functions=(),
                data_symbols=(),
                manifest_path=root / "sample.json",
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=root / "sample.obj",
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            data_symbol = VerifyDataSymbol(
                address="0x402000",
                symbol="??_7SampleWithTable@@6B@",
                name="g_SampleWithTable_FTable",
                byte_length=4,
                object_offset=8,
                bn_name="g_SampleWithTable_FTable",
            )

            def fake_data_compare(**kwargs):
                self.assertEqual(8, kwargs["object_offset"])
                return ObjectByteComparison(
                    address=kwargs["address"],
                    symbol=kwargs["symbol"],
                    obj_path=kwargs["obj_path"],
                    bn_path=root / "bn_data.bytes",
                    vc5_path=root / "vc5_data.bytes",
                    mask_path=root / "mask.txt",
                    diff_path=diff_path,
                    triage_path=triage_path,
                    text_diff_path=None,
                    classified_text_path=None,
                    mismatch_count=0,
                    relocation_masked_bytes=4,
                    bn_size=4,
                    vc5_size=4,
                    trailing_bn_nops_trimmed=0,
                    trailing_vc5_nops_trimmed=0,
                    relocation_identity_path=root / "identity.txt",
                )

            with patch("_recoil.commands.vc5_verify.compare_bn_data_to_obj", side_effect=fake_data_compare):
                results, rc = compare_compiled_selections(
                    compiled=compiled,
                    selections=[VerifySelection(target=target, functions=(), data_symbols=(data_symbol,))],
                    bridge_url="http://example.invalid",
                )

        self.assertEqual(0, rc)
        self.assertEqual(1, len(results))
        self.assertEqual("data", results[0].item_kind)
        self.assertEqual(4, results[0].relocation_or_text_metric)

    def test_compare_compiled_selections_resolves_data_symbol_regex(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            diff_path = root / "diff.txt"
            triage_path = root / "triage.txt"
            diff_path.write_text("", encoding="utf-8")
            triage_path.write_text("", encoding="utf-8")
            obj_path = root / "sample.obj"
            obj_path.write_bytes(b"coff")
            target = VerifyTarget(
                name="sample",
                description="sample",
                source_filename="sample.cpp",
                source_text="",
                source_from="src/sample.cpp",
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="",
                compiler_env="",
                compiler_flags=(),
                include_dirs=(),
                source_files=(),
                generated_files=(),
                functions=(),
                data_symbols=(),
                manifest_path=root / "sample.json",
            )
            compiled = CompiledTarget(
                target=target,
                build_dir=root,
                source_path=root / "sample.cpp",
                cod_path=root / "sample.cod",
                obj_path=obj_path,
                compiler_env=root / "env.cmd",
                compiler_version="test",
                compile_command="cl",
            )
            data_symbol = VerifyDataSymbol(
                address="0x402000",
                symbol="",
                name="g_SampleWithTable_FTable",
                byte_length=4,
                bn_name="g_SampleWithTable_FTable",
                symbol_regex=r"\?\?_7SampleWithTable@@6B@",
            )

            def fake_data_compare(**kwargs):
                return ObjectByteComparison(
                    address=kwargs["address"],
                    symbol=kwargs["symbol"],
                    obj_path=kwargs["obj_path"],
                    bn_path=root / "bn_data.bytes",
                    vc5_path=root / "vc5_data.bytes",
                    mask_path=root / "mask.txt",
                    diff_path=diff_path,
                    triage_path=triage_path,
                    text_diff_path=None,
                    classified_text_path=None,
                    mismatch_count=0,
                    relocation_masked_bytes=4,
                    bn_size=4,
                    vc5_size=4,
                    trailing_bn_nops_trimmed=0,
                    trailing_vc5_nops_trimmed=0,
                    relocation_identity_path=root / "identity.txt",
                )

            coff = SimpleNamespace(symbols=(SimpleNamespace(name="??_7SampleWithTable@@6B@"),))
            with patch("_recoil.commands.vc5_verify.CoffObject.from_path", return_value=coff):
                with patch("_recoil.commands.vc5_verify.compare_bn_data_to_obj", side_effect=fake_data_compare):
                    results, rc = compare_compiled_selections(
                        compiled=compiled,
                        selections=[VerifySelection(target=target, functions=(), data_symbols=(data_symbol,))],
                        bridge_url="http://example.invalid",
                    )

        self.assertEqual(0, rc)
        self.assertEqual("??_7SampleWithTable@@6B@", results[0].function.symbol)

    def test_about_order_target_uses_exact_final_build_compile_context(self):
        target = load_manifest(
            REPO_ROOT / "tools" / "vc5_verify_targets" / "cabout_prelude_provider_order_current_shape.json"
        )

        self.assertEqual("tools/_recoil/config/vc5_final_build.json", target.compile_context_from)
        self.assertIn("/GX", target.compiler_flags)
        self.assertIn("/DWIN32", target.compiler_flags)
        self.assertEqual("tools/_recoil/compat/include", target.include_dirs[0])
        self.assertEqual("0x401000", target.retail_start)
        self.assertEqual("0x401060", target.retail_end_exclusive)

    def test_zutl_zbd_order_target_uses_only_its_exact_ob1_source_mapping(self):
        target = load_manifest(
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zutl_zbd_4bffe0_4c0d20_authored_order.json"
        )

        zbd_profile, zbd_flags = effective_source_compile_context(
            target,
            "src/GameZRecoil/zUtil/zutl_zbd.cpp",
        )
        zar_profile, zar_flags = effective_source_compile_context(
            target,
            "src/GameZRecoil/zUtil/zutl_zar.cpp",
        )
        zui_profile, zui_flags = effective_source_compile_context(
            target,
            "src/GameZRecoil/zUI/zui.cpp",
        )

        self.assertEqual("tools/_recoil/config/vc5_final_build.json", target.compile_context_from)
        self.assertEqual("vc5_o2_ob1_md_gx_fastcall_facs", zbd_profile)
        self.assertIn("/Ob1", zbd_flags)
        self.assertNotIn("/Ob0", zbd_flags)
        self.assertEqual("", zar_profile)
        self.assertIn("/Ob0", zar_flags)
        self.assertNotIn("/Ob1", zar_flags)
        self.assertEqual("vc5_o2_ob1_md_gx_fastcall_facs", zui_profile)
        self.assertIn("/Ob1", zui_flags)
        self.assertNotIn("/Ob0", zui_flags)

    def test_zdeclient_vector_growth_target_uses_only_crater_ob1_source_mapping(self):
        target = load_manifest(
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zdeclient_crater_456ad0_458af0_authored_order.json"
        )

        crater_profile, crater_flags = effective_source_compile_context(
            target,
            "src/GameZRecoil/zDEClient/zdec_crater.cpp",
        )
        init_profile, init_flags = effective_source_compile_context(
            target,
            "src/GameZRecoil/zDEClient/zdec_init.cpp",
        )

        self.assertEqual(
            "tools/_recoil/config/vc5_final_build.json",
            target.compile_context_from,
        )
        self.assertEqual(
            "vc5_o2_ob1_md_gx_fastcall_facs",
            crater_profile,
        )
        self.assertIn("/Ob1", crater_flags)
        self.assertNotIn("/Ob0", crater_flags)
        self.assertIn("/MD", crater_flags)
        self.assertIn("/GX", crater_flags)
        self.assertIn("/Gr", crater_flags)
        self.assertEqual("", init_profile)
        self.assertIn("/Ob0", init_flags)
        self.assertNotIn("/Ob1", init_flags)

    def test_ordered_compile_argv_preserves_flag_order_and_duplicates(self):
        target = VerifyTarget(
            name="receipt_order",
            description="ordered compiler receipt",
            source_filename="receipt.cpp",
            source_text="",
            source_from="src/receipt.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="receipt-profile",
            compiler_env="",
            compiler_flags=("/nologo", "/O2", "/Ob1", "/Ob1", "/DORDER=1"),
            include_dirs=("src", "tools/_recoil/compat/include"),
            source_files=("src/receipt.cpp",),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("receipt.json"),
        )

        argv = vc5_verify_module.ordered_compile_argv(
            target,
            Path("C:/build/receipt.cpp"),
            Path("C:/build"),
        )

        self.assertEqual(
            ("/nologo", "/O2", "/Ob1", "/Ob1", "/DORDER=1"),
            argv[1:6],
        )
        self.assertEqual(2, argv.count("/Ob1"))
        self.assertLess(argv.index("/O2"), argv.index("/Ob1"))
        self.assertEqual("/c", argv[-2])
        self.assertEqual("receipt.cpp", argv[-1])

    def test_header_input_observation_records_inactive_missing_reference(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "unit.cpp"
            conditional = root / "conditional.h"
            active = root / "active.h"
            source.write_text('#include "conditional.h"\n', encoding="ascii")
            conditional.write_text(
                "#if defined(_MAC)\n"
                "#include <missing_mac_only.h>\n"
                "#else\n"
                "#include \"active.h\"\n"
                "#endif\n",
                encoding="ascii",
            )
            active.write_text("#define ACTIVE_VALUE 1\n", encoding="ascii")
            target = VerifyTarget(
                name="conditional-headers",
                description="conditional headers",
                source_filename="unit.cpp",
                source_text="",
                source_from=str(source),
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="profile-a",
                compiler_env="",
                compiler_flags=("/O2",),
                include_dirs=(str(root),),
                source_files=(str(source),),
                generated_files=(),
                functions=(),
                data_symbols=(),
                manifest_path=root / "manifest.json",
            )

            headers, roots, unresolved, errors = (
                vc5_verify_module._governed_header_inputs(
                    source_path=source,
                    target=target,
                    environment={"INCLUDE": ""},
                )
            )

            self.assertEqual([], list(errors))
            self.assertEqual(1, len(roots))
            self.assertEqual(
                {str(conditional.resolve()), str(active.resolve())},
                {row["path"] for row in headers},
            )
            self.assertEqual(
                ({
                    "included_from": str(conditional.resolve()),
                    "include_text": "missing_mac_only.h",
                    "delimiter": "angle",
                },),
                unresolved,
            )

    def test_compiler_receipt_captures_inherited_and_response_order(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "receipt.cpp"
            source.write_text("int receipt;\n", encoding="ascii")
            manifest = root / "receipt.json"
            manifest.write_text("{}\n", encoding="utf-8")
            compiler_env = root / "env.cmd"
            compiler_env.write_text("@set PATH=C:\\VC5\\BIN\n", encoding="ascii")
            response = root / "compiler.rsp"
            response.write_text('/DRESP=1 /Ob0 /Ob0', encoding="ascii")
            target = VerifyTarget(
                name="receipt",
                description="receipt",
                source_filename="receipt.cpp",
                source_text="",
                source_from=str(source),
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="profile-a",
                compiler_env="",
                compiler_flags=("/O2", "/Ob1", "/Ob1"),
                include_dirs=(),
                source_files=(str(source),),
                generated_files=(("compiler.rsp", "/DRESP=1 /Ob0 /Ob0"),),
                functions=(),
                data_symbols=(),
                manifest_path=manifest,
            )
            components = (
                {
                    "role": "compiler-driver",
                    "path": str(root / "cl.exe"),
                    "configured_absolute_path": str(root / "cl.exe"),
                    "physical_identity": {"volume_serial": 1, "file_id": "10", "file_size": 10},
                    "size": 10,
                    "version": "11.00.7022",
                    "status": "invoked",
                    "invocation": {"invoked": True, "form": "command-line"},
                },
                {
                    "role": "compiler-front-end",
                    "path": str(root / "c1xx.dll"),
                    "configured_absolute_path": str(root / "c1xx.dll"),
                    "physical_identity": {"volume_serial": 1, "file_id": "11", "file_size": 11},
                    "size": 11,
                    "version": "11.00.7022",
                    "status": "invoked-by-driver",
                    "invocation": {"invoked": True, "form": "internal-driver-dispatch"},
                },
                {
                    "role": "compiler-back-end",
                    "path": str(root / "c2.dll"),
                    "configured_absolute_path": str(root / "c2.dll"),
                    "physical_identity": {"volume_serial": 1, "file_id": "12", "file_size": 12},
                    "size": 12,
                    "version": "11.00.7022",
                    "status": "invoked-by-driver",
                    "invocation": {"invoked": True, "form": "internal-driver-dispatch"},
                },
            )
            with (
                patch.object(
                    vc5_verify_module,
                    "_capture_vc5_environment",
                    return_value=(
                        {
                            "PATH": str(root),
                            "CL": f"/DCL=1 @{response.name}",
                            "_CL_": "/DPOST=1",
                        },
                        None,
                    ),
                ),
                patch.object(
                    vc5_verify_module,
                    "_resolve_toolchain_components",
                    return_value=(components, ()),
                ),
            ):
                receipt = vc5_verify_module.build_compiler_receipt(
                    target=target,
                    source_path=source,
                    source_from=str(source),
                    manifest_index=0,
                    compiler_env=compiler_env,
                    build_dir=root,
                    compiler_version="Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 11.00.7022",
                )

            argv = receipt["compiler"]["argv"]
            self.assertEqual("cl", argv[0])
            self.assertEqual(
                ["/DCL=1", f"@{response.name}", "/O2", "/Ob1"], argv[1:5]
            )
            self.assertEqual(2, argv.count("/Ob1"))
            self.assertEqual("/DPOST=1", argv[-1])
            self.assertEqual(2, receipt["compiler"]["expanded_argv"].count("/Ob0"))
            self.assertEqual(str(response.resolve()), receipt["compiler"]["response_files"][0]["path"])
            self.assertEqual("/DCL=1", receipt["compiler"]["inherited_options"]["CL"].split()[0])
            self.assertTrue(receipt["verification_eligible"])
            self.assertEqual(3, receipt["contract_version"])
            self.assertEqual(3, len(receipt["toolchain"]["components"]))
            self.assertEqual(
                ["10", "11", "12"],
                [
                    row["physical_identity"]["file_id"]
                    for row in receipt["toolchain"]["components"]
                ],
            )

    def test_compiler_manifest_projection_preserves_explicit_profile_and_flags(self):
        target = VerifyTarget(
            name="projection",
            description="projection",
            source_filename="projection.cpp",
            source_text="",
            source_from="src/projection.cpp",
            compare_mode="coff_bytes",
            trim_trailing_nops=True,
            compiler_profile="profile-a",
            compiler_env="",
            compiler_flags=("/O2", "/Ob1"),
            include_dirs=("src",),
            source_files=("src/projection.cpp",),
            generated_files=(),
            functions=(),
            data_symbols=(),
            manifest_path=Path("projection.json"),
        )
        projection = vc5_verify_module._target_manifest_projection(
            target, source_from="src/projection.cpp", manifest_index=3
        )
        self.assertEqual("profile-a", projection["compiler_profile"])
        self.assertEqual(["/O2", "/Ob1"], projection["compiler_flags"])
        self.assertEqual(["src"], projection["include_dirs"])
        self.assertEqual(3, projection["manifest_index"])

    def test_compiler_receipt_uses_authored_source_and_stable_build_root_projection(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            authored = root / "authored.cpp"
            authored.write_text("int authored;\n", encoding="ascii")
            manifest = root / "manifest.json"
            manifest.write_text("{}\n", encoding="utf-8")
            compiler_env = root / "env.cmd"
            compiler_env.write_text("@set PATH=C:\\VC5\\BIN\n", encoding="ascii")
            builds = (root / "build-a", root / "build-b")
            staged_paths = []
            for build in builds:
                staged = build / "staged" / "input.cpp"
                staged.parent.mkdir(parents=True)
                staged.write_bytes(authored.read_bytes())
                staged_paths.append(staged)
            target = VerifyTarget(
                name="stable-root",
                description="stable-root",
                source_filename="input.cpp",
                source_text="",
                source_from=str(authored),
                compare_mode="coff_bytes",
                trim_trailing_nops=True,
                compiler_profile="profile-a",
                compiler_env="",
                compiler_flags=("/O2", "/Ob1"),
                include_dirs=(),
                source_files=(str(authored),),
                generated_files=(),
                functions=(),
                data_symbols=(),
                manifest_path=manifest,
            )
            components = (
                {
                    "role": "compiler-driver",
                    "path": "C:/VC5/BIN/cl.exe",
                    "configured_absolute_path": "C:/VC5/BIN/cl.exe",
                    "physical_identity": {"volume_serial": 1, "file_id": "10", "file_size": 10},
                    "size": 10,
                    "version": "11.00.7022",
                    "status": "invoked",
                    "invocation": {"invoked": True, "form": "command-line"},
                },
                {
                    "role": "compiler-front-end",
                    "path": "C:/VC5/BIN/c1xx.dll",
                    "configured_absolute_path": "C:/VC5/BIN/c1xx.dll",
                    "physical_identity": {"volume_serial": 1, "file_id": "11", "file_size": 11},
                    "size": 11,
                    "version": "11.00.7022",
                    "status": "invoked-by-driver",
                    "invocation": {"invoked": True, "form": "internal-driver-dispatch"},
                },
                {
                    "role": "compiler-back-end",
                    "path": "C:/VC5/BIN/c2.dll",
                    "configured_absolute_path": "C:/VC5/BIN/c2.dll",
                    "physical_identity": {"volume_serial": 1, "file_id": "12", "file_size": 12},
                    "size": 12,
                    "version": "11.00.7022",
                    "status": "invoked-by-driver",
                    "invocation": {"invoked": True, "form": "internal-driver-dispatch"},
                },
            )

            with (
                patch.object(
                    vc5_verify_module,
                    "_capture_vc5_environment",
                    return_value=({"PATH": "C:/VC5/BIN", "CL": "", "_CL_": ""}, None),
                ),
                patch.object(
                    vc5_verify_module,
                    "_resolve_toolchain_components",
                    return_value=(components, ()),
                ),
            ):
                receipts = [
                    vc5_verify_module.build_compiler_receipt(
                        target=target,
                        source_path=staged,
                        source_from=str(authored),
                        manifest_index=0,
                        compiler_env=compiler_env,
                        build_dir=build,
                        compiler_version="11.00.7022",
                    )
                    for build, staged in zip(builds, staged_paths)
                ]

            self.assertEqual(receipts[0]["tu_context_id"], receipts[1]["tu_context_id"])
            self.assertEqual(str(authored.resolve()), receipts[0]["source"]["path"])
            self.assertEqual(
                receipts[0]["source"]["physical_identity"],
                receipts[1]["source"]["physical_identity"],
            )
            compiled_inputs = [
                next(row for row in receipt["dependencies"] if row["role"] == "compiled-input")
                for receipt in receipts
            ]
            self.assertNotEqual(
                compiled_inputs[0]["path"],
                compiled_inputs[1]["path"],
            )
            self.assertNotEqual(
                compiled_inputs[0]["physical_identity"],
                compiled_inputs[1]["physical_identity"],
            )
            self.assertEqual(receipts[0]["compiler"]["argv"], receipts[1]["compiler"]["argv"])
            self.assertEqual(
                "${RECOIL_ISOLATED_BUILD_ROOT}", receipts[0]["compiler"]["cwd"]
            )
            self.assertNotEqual(
                receipts[0]["compiler"]["observed_cwd"],
                receipts[1]["compiler"]["observed_cwd"],
            )
            self.assertTrue(receipts[0]["verification_eligible"])
            self.assertTrue(receipts[1]["verification_eligible"])

    def test_direct_compiler_observation_drift_matrix_is_ineligible(self):
        base = {
            "verification_eligible": True,
            "ineligibility_reasons": [],
            "compiler": {
                "argv": ["cl", "/O2", "/Ob1", "/Ob1", "/c", "unit.cpp"],
                "expanded_argv": ["cl", "/O2", "/Ob1", "/Ob1", "/c", "unit.cpp"],
                "inherited_options": {"CL": "/DLEFT=1", "_CL_": "/DRIGHT=1"},
                "response_files": [{"path": "C:/b/args.rsp", "tokens": ["/Ob1", "/Ob1"]}],
            },
            "toolchain": {
                "compiler_version": "11.00.7022",
                "components": [
                    {"role": "compiler-driver", "path": "C:/VC5/cl.exe", "physical_identity": {"volume_identity": 1, "file_id": 10}, "size": 20, "version": "11.00.7022"},
                    {"role": "compiler-front-end", "path": "C:/VC5/c1xx.dll", "physical_identity": {"volume_identity": 1, "file_id": 11}, "size": 21, "version": "11.00.7022"},
                    {"role": "compiler-back-end", "path": "C:/VC5/c2.dll", "physical_identity": {"volume_identity": 1, "file_id": 12}, "size": 22, "version": "11.00.7022"},
                ],
                "header_inputs": [{"path": "C:/include/direct.h", "physical_identity": {"volume_identity": 1, "file_id": 13}, "size": 23}],
            },
        }

        def mutate(row, case):
            if case == "same-path-compiler-replacement":
                row["toolchain"]["components"][0]["physical_identity"]["file_id"] = 99
            elif case == "version-drift":
                row["toolchain"]["compiler_version"] = "11.00.7023"
            elif case == "front-end-replacement":
                row["toolchain"]["components"][1]["physical_identity"]["file_id"] = 99
            elif case == "back-end-replacement":
                row["toolchain"]["components"][2]["physical_identity"]["file_id"] = 99
            elif case == "ordered-argument-drift":
                row["compiler"]["argv"][1:3] = reversed(row["compiler"]["argv"][1:3])
            elif case == "duplicate-removal":
                del row["compiler"]["argv"][3]
            elif case == "inherited-cl-drift":
                row["compiler"]["inherited_options"]["CL"] = "/DLEFT=2"
            elif case == "inherited-post-drift":
                row["compiler"]["inherited_options"]["_CL_"] = "/DRIGHT=2"
            elif case == "response-file-drift":
                row["compiler"]["response_files"][0]["tokens"][0] = "/Ob0"
            elif case == "direct-header-replacement":
                row["toolchain"]["header_inputs"][0]["physical_identity"]["file_id"] = 99
            elif case == "required-field-unavailable":
                row["verification_eligible"] = False
                row["ineligibility_reasons"] = ["compiler-driver-version-unavailable"]
                row["toolchain"]["components"][0]["version"] = "<not detected>"

        cases = (
            "same-path-compiler-replacement", "version-drift",
            "front-end-replacement", "back-end-replacement",
            "ordered-argument-drift", "duplicate-removal",
            "inherited-cl-drift", "inherited-post-drift",
            "response-file-drift", "direct-header-replacement",
            "required-field-unavailable",
        )
        for case in cases:
            with self.subTest(case=case):
                after = deepcopy(base)
                mutate(after, case)
                receipt = vc5_verify_module.compiler_receipt_stability(base, after)
                self.assertFalse(receipt["verification_eligible"])
                self.assertTrue(receipt["direct_difference_fields"])


if __name__ == "__main__":
    unittest.main()
