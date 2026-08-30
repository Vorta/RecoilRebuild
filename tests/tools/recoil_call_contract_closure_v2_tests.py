from __future__ import annotations

from copy import deepcopy
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.call_contract_verify import (  # noqa: E402
    CallContractSourceClosure,
    _LazyCallContractCandidateSession,
    _resolve_target_all_authored_bodies,
    build_parser,
)
from _recoil.commands.progress_cli import (  # noqa: E402
    _validate_call_contract_result,
)
from _recoil.lib.call_contract_generations import (  # noqa: E402
    current_generations,
)
from _recoil.lib.progress import (  # noqa: E402
    CALL_CONTRACT_CONTRACT_VERSION,
    ProgressError,
)
from _recoil.lib.cpp_definition_closure import (  # noqa: E402
    CallableKey,
    decode_vc5_zero_argument_callable_identity,
    decode_vc5_zero_argument_lifecycle_identity,
    resolve_dependent_callable_owner,
    resolve_reviewed_definition_sources,
)


TARGET_ID = "recoil:vc5-target:unit"
OTHER_TARGET_ID = "recoil:vc5-target:other"


class FixtureDocument:
    def __init__(self) -> None:
        self._collections = {
            "verification_targets": {
                TARGET_ID: {
                    "kind": "vc5",
                    "registration": {"manifest_path": "unit-target.json"},
                },
                OTHER_TARGET_ID: {
                    "kind": "vc5",
                    "registration": {"manifest_path": "other-target.json"},
                },
            },
            "symbols": {
                "recoil:function:0x401000": {
                    "pipeline_class": "authored",
                    "authored_order_role": "authored-body",
                    "physical_block_id": "recoil:block:0x401000",
                },
                "recoil:function:0x401010": {
                    "pipeline_class": "authored",
                    "authored_order_role": "authored-body",
                    "physical_block_id": "recoil:block:0x401010",
                },
                "recoil:function:0x401020": {
                    "pipeline_class": "authored-lifecycle",
                    "authored_order_role": "authored-lifecycle-body",
                    "physical_block_id": "recoil:block:0x401020",
                },
            },
            "physical_blocks": {
                "recoil:block:0x401000": {
                    "accepted_order_facts": {"target_id": TARGET_ID}
                },
                "recoil:block:0x401010": {
                    "accepted_order_facts": {"target_id": OTHER_TARGET_ID}
                },
                "recoil:block:0x401020": {
                    "accepted_order_facts": {"target_id": TARGET_ID}
                },
            },
        }

    def collection(self, name: str):
        return self._collections[name]

    def authored_call_contract_slices(self):
        return [
            {
                "id": "recoil:call-contract-slice:first",
                "symbol_ids": [
                    "recoil:function:0x401000",
                    "recoil:function:0x401010",
                ],
                "addresses": ["0x401000", "0x401010"],
            },
            {
                "id": "recoil:call-contract-slice:second",
                "symbol_ids": ["recoil:function:0x401020"],
                "addresses": ["0x401020"],
            },
        ]


class CallContractClosureV2Tests(unittest.TestCase):
    def test_live_verifier_has_no_unreachable_phase_convergence_mode(self) -> None:
        source = (
            REPO_ROOT / "tools/_recoil/commands/call_contract_verify.py"
        ).read_text(encoding="utf-8")
        self.assertNotIn("phase_convergence_mode", source)

    @staticmethod
    def _direct_result_fixture(body_count: int = 2) -> tuple[dict, dict]:
        symbol_ids = [
            f"recoil:function:0x{0x401000 + index * 0x10:06x}"
            for index in range(body_count)
        ]
        addresses = [
            f"0x{0x401000 + index * 0x10:06x}"
            for index in range(body_count)
        ]
        target_ids = [TARGET_ID for _index in range(body_count)]
        physical_block_ids = [
            f"recoil:block:{address}" for address in addresses
        ]
        expected_fact_rows = [
            {
                **current_generations(),
                "symbol_id": symbol_id,
                "address": address,
                "calls": [],
            }
            for symbol_id, address in zip(symbol_ids, addresses)
        ]
        slice_row = {
            "id": "recoil:call-contract-slice:fixture",
            "body_count": body_count,
            "symbol_ids": symbol_ids,
            "addresses": addresses,
            "target_ids": target_ids,
            "physical_block_ids": physical_block_ids,
        }
        result = {
            "kind": "authored-call-contract-live-result",
            "contract_version": CALL_CONTRACT_CONTRACT_VERSION,
            "all_caller_divergences_collected": True,
            "slice_id": slice_row["id"],
            "body_count": body_count,
            "symbol_ids": symbol_ids,
            "target_ids": target_ids,
            "physical_block_ids": physical_block_ids,
            "candidate_expected_truth": False,
            "source_edit_paths": [],
            "definition_source_paths": [],
            "definition_compile_results": [],
            "dependency_paths": [],
            "dependency_states_before": [],
            "dependency_states_after": [],
            "source_changed_during_validation": False,
            "passed": True,
            "first_divergence": None,
            "exact_fact_transcript": [
                {
                    "symbol_id": symbol_id,
                    "address": address,
                    "expected_fact_row": expected_fact_row,
                }
                for symbol_id, address, expected_fact_row in zip(
                    symbol_ids, addresses, expected_fact_rows
                )
            ],
            "provider_fact_transcript": [],
            "body_results": [
                {
                    **current_generations(),
                    "symbol_id": symbol_id,
                    "address": address,
                    "target_id": target_id,
                    "status": "passed",
                    "comparison_passed": True,
                    "divergence": None,
                    "expected_fact_row": expected_fact_row,
                    "expected_contract": [],
                    "candidate_contract": [],
                    "normalizers": [],
                }
                for symbol_id, address, target_id, expected_fact_row in zip(
                    symbol_ids, addresses, target_ids, expected_fact_rows
                )
            ],
        }
        return slice_row, result

    def test_direct_result_uses_ordered_fact_transcript_without_session_receipt(
        self,
    ) -> None:
        slice_row, result = self._direct_result_fixture()
        validated = _validate_call_contract_result(
            result,
            expected_slice=slice_row,
            expected_source_write_paths=[],
            expected_definition_source_paths=[],
            expected_compiled_definition_sources=[],
            expected_dependency_paths=[],
        )

        self.assertEqual(
            slice_row["symbol_ids"],
            [row["symbol_id"] for row in validated["exact_fact_transcript"]],
        )
        self.assertNotIn("binary_ninja_session", validated)

    def test_direct_result_rejects_inexact_fact_transcript_population(self) -> None:
        slice_row, result = self._direct_result_fixture()
        cases = {
            "missing": result["exact_fact_transcript"][:-1],
            "extra": [
                *result["exact_fact_transcript"],
                deepcopy(result["exact_fact_transcript"][0]),
            ],
            "reordered": list(reversed(result["exact_fact_transcript"])),
            "duplicated": [
                deepcopy(result["exact_fact_transcript"][0]),
                deepcopy(result["exact_fact_transcript"][0]),
            ],
        }
        for label, transcript in cases.items():
            drifted = deepcopy(result)
            drifted["exact_fact_transcript"] = transcript
            with (
                self.subTest(label=label),
                self.assertRaisesRegex(ProgressError, "expected-fact transcript"),
            ):
                _validate_call_contract_result(
                    drifted,
                    expected_slice=slice_row,
                    expected_source_write_paths=[],
                    expected_definition_source_paths=[],
                    expected_compiled_definition_sources=[],
                    expected_dependency_paths=[],
                )

    def test_direct_result_rejects_transcript_address_or_fact_drift(self) -> None:
        slice_row, result = self._direct_result_fixture()
        cases = {
            "address": ("address", "0x401999"),
            "fact-address": ("expected_fact_row.address", "0x401999"),
            "fact-calls": ("expected_fact_row.calls", [{"kind": "call"}]),
        }
        for label, (field, value) in cases.items():
            drifted = deepcopy(result)
            if field == "address":
                drifted["exact_fact_transcript"][0]["address"] = value
            elif field == "expected_fact_row.address":
                drifted["exact_fact_transcript"][0]["expected_fact_row"][
                    "address"
                ] = value
                drifted["body_results"][0]["expected_fact_row"][
                    "address"
                ] = value
            else:
                drifted["exact_fact_transcript"][0]["expected_fact_row"][
                    "calls"
                ] = value
                drifted["body_results"][0]["expected_fact_row"]["calls"] = value
            with self.subTest(label=label), self.assertRaises(ProgressError):
                _validate_call_contract_result(
                    drifted,
                    expected_slice=slice_row,
                    expected_source_write_paths=[],
                    expected_definition_source_paths=[],
                    expected_compiled_definition_sources=[],
                    expected_dependency_paths=[],
                )

    def test_lifecycle_decoder_records_exact_complete_ctor_and_dtor(self) -> None:
        constructor = decode_vc5_zero_argument_lifecycle_identity(
            "??0HudUiMenuBackButton@@QAE@XZ"
        )
        destructor = decode_vc5_zero_argument_lifecycle_identity(
            "??1HudUiMenuBackButton@@UAE@XZ"
        )

        self.assertIsNotNone(constructor)
        self.assertIsNotNone(destructor)
        assert constructor is not None and destructor is not None
        self.assertEqual(
            CallableKey(
                "HudUiMenuBackButton::HudUiMenuBackButton", ()
            ),
            constructor.callable_key,
        )
        self.assertEqual(
            CallableKey(
                "HudUiMenuBackButton::~HudUiMenuBackButton", ()
            ),
            destructor.callable_key,
        )
        self.assertEqual("__thiscall", destructor.calling_convention)
        self.assertEqual(
            "msvc-cpp-complete-destructor", destructor.identity_format
        )
        self.assertIsNone(
            decode_vc5_zero_argument_lifecycle_identity(
                "??_GHudUiMenuBackButton@@UAEPAXI@Z"
            )
        )

    def test_dependent_owner_decoder_records_exact_cpp_fastcall_abi(self) -> None:
        result = decode_vc5_zero_argument_callable_identity(
            "?MatLoadIdentity@zMath@@YIXXZ"
        )

        self.assertIsNotNone(result)
        assert result is not None
        self.assertEqual(
            CallableKey("zMath::MatLoadIdentity", ()), result.callable_key
        )
        self.assertEqual("__fastcall", result.calling_convention)
        self.assertEqual(0, result.parameter_bytes)
        self.assertEqual("void", result.return_shape)
        self.assertEqual("msvc-cpp-global", result.identity_format)

    def test_dependent_owner_decoder_records_exact_cpp_int_scalar_abi(self) -> None:
        for calling_code, calling_convention in (
            ("A", "__cdecl"),
            ("I", "__fastcall"),
        ):
            identity = (
                "?ResetIterationState@zTurret_System@@Y"
                f"{calling_code}HXZ"
            )
            with self.subTest(identity=identity):
                result = decode_vc5_zero_argument_callable_identity(identity)

                self.assertIsNotNone(result)
                assert result is not None
                self.assertEqual(
                    CallableKey(
                        "zTurret_System::ResetIterationState", ()
                    ),
                    result.callable_key,
                )
                self.assertEqual(calling_convention, result.calling_convention)
                self.assertEqual(0, result.parameter_bytes)
                self.assertEqual("int", result.return_shape)
                self.assertEqual("msvc-cpp-global", result.identity_format)

    def test_dependent_owner_decoder_records_bounded_pad_char_pointer_abi(
        self,
    ) -> None:
        for calling_code, calling_convention in (
            ("A", "__cdecl"),
            ("I", "__fastcall"),
        ):
            identity = (
                "?GetSelectedHwApiDescriptionOrDefault@zVid@@Y"
                f"{calling_code}PADXZ"
            )
            with self.subTest(identity=identity):
                result = decode_vc5_zero_argument_callable_identity(identity)

                self.assertIsNotNone(result)
                assert result is not None
                self.assertEqual(
                    CallableKey(
                        "zVid::GetSelectedHwApiDescriptionOrDefault", ()
                    ),
                    result.callable_key,
                )
                self.assertEqual(calling_convention, result.calling_convention)
                self.assertEqual(0, result.parameter_bytes)
                self.assertEqual("char *", result.return_shape)
                self.assertEqual("msvc-cpp-global", result.identity_format)

    def test_dependent_owner_decoder_records_exact_c_fastcall_abi(self) -> None:
        result = decode_vc5_zero_argument_callable_identity(
            "@zArchiveList_CreateEmpty@0"
        )

        self.assertIsNotNone(result)
        assert result is not None
        self.assertEqual(
            CallableKey("zArchiveList_CreateEmpty", ()), result.callable_key
        )
        self.assertEqual("__fastcall", result.calling_convention)
        self.assertEqual(0, result.parameter_bytes)
        self.assertEqual("unknown-not-encoded", result.return_shape)
        self.assertEqual("msvc-c-fastcall", result.identity_format)

    def test_dependent_owner_decoder_rejects_unsupported_shapes(self) -> None:
        for identity in (
            "?Run@Unit@@QAEHXZ",
            "?Run@Unit@@YAHH@Z",
            "?Run@Unit@@YIDXZ",
            "?Run@Unit@@YIHHXZ",
            "?Run@Unit@@YIHXZ@0",
            "?Run@Unit@@YGPADXZ",
            "?Run@Unit@@YIPBDXZ",
            "?Run@Unit@@YIPADH@Z",
            "?Run@Unit@@YIPADXZ@0",
            "@Run@4",
            "_Run@0",
        ):
            with self.subTest(identity=identity):
                self.assertIsNone(
                    decode_vc5_zero_argument_callable_identity(identity)
                )

    def test_dependent_owner_requires_unique_declaration_and_definition(self) -> None:
        result = resolve_dependent_callable_owner(
            callable_key=CallableKey("zMath::MatLoadIdentity", ()),
            header_texts=(
                (
                    "src/GameZRecoil/zMath/zmth_decls.h",
                    "namespace zMath { void __fastcall MatLoadIdentity(); }",
                    (),
                ),
            ),
            source_texts=(
                (
                    "src/GameZRecoil/zMath/zmth_main.c",
                    "void __fastcall zMath::MatLoadIdentity() {}",
                    (),
                ),
            ),
        )

        self.assertEqual("exact", result.mode)
        self.assertEqual(
            ("src/GameZRecoil/zMath/zmth_decls.h",),
            result.declaration_paths,
        )
        self.assertEqual(
            ("src/GameZRecoil/zMath/zmth_main.c",), result.definition_paths
        )
        self.assertEqual(
            (
                "src/GameZRecoil/zMath/zmth_decls.h",
                "src/GameZRecoil/zMath/zmth_main.c",
            ),
            result.source_edit_paths,
        )
        self.assertFalse(result.candidate_independent)

    def test_dependent_owner_ambiguity_and_unresolved_fail_closed(self) -> None:
        key = CallableKey("Unit::Run", ())
        ambiguous = resolve_dependent_callable_owner(
            callable_key=key,
            header_texts=(
                ("src/One.h", "namespace Unit { void Run(); }", ()),
                ("src/Two.h", "namespace Unit { void Run(); }", ()),
            ),
            source_texts=(
                ("src/Unit.cpp", "void Unit::Run() {}", ()),
            ),
        )
        unresolved = resolve_dependent_callable_owner(
            callable_key=key,
            header_texts=(),
            source_texts=(),
        )

        self.assertEqual("ambiguous", ambiguous.mode)
        self.assertEqual((), ambiguous.source_edit_paths)
        self.assertEqual("unresolved", unresolved.mode)
        self.assertEqual((), unresolved.source_edit_paths)

    def test_reviewed_callable_identity_resolves_one_exact_definition_tu(self) -> None:
        key = CallableKey("Unit::Run", ("int",), const_member=True)
        result = resolve_reviewed_definition_sources(
            reviewed_callables=(("symbol:recoil:function:0x401000", key),),
            header_texts=(("src/Unit.h", "int Run(int) const;", ()),),
            source_texts=(
                ("src/Unit.cpp", "int Unit::Run(int) const { return 1; }", ()),
                ("src/Other.cpp", "int Other::Run(int) { return 2; }", ()),
            ),
        )

        self.assertEqual("reviewed-call-edge-exact", result.mode)
        self.assertEqual(("src/Unit.cpp",), result.source_paths)
        self.assertEqual((), result.unresolved_target_identities)

    def test_reviewed_callable_identity_ambiguity_fails_closed(self) -> None:
        key = CallableKey("Unit::Run", ("int",))
        with self.assertRaisesRegex(ValueError, "ambiguous source ownership"):
            resolve_reviewed_definition_sources(
                reviewed_callables=(("symbol:recoil:function:0x401000", key),),
                header_texts=(),
                source_texts=(
                    ("src/One.cpp", "int Unit::Run(int) { return 1; }", ()),
                    ("src/Two.cpp", "int Unit::Run(int) { return 2; }", ()),
                ),
            )

    def test_unresolved_reviewed_identity_uses_typed_full_fallback(self) -> None:
        result = resolve_reviewed_definition_sources(
            reviewed_callables=(
                (
                    "symbol:recoil:function:0x401000",
                    CallableKey("Missing::Run", ()),
                ),
            ),
            header_texts=(("src/Shared.h", "int Shared();", ()),),
            source_texts=(
                ("src/Shared.cpp", "int Shared() { return 1; }", ()),
            ),
        )

        self.assertEqual("conservative-full-closure", result.mode)
        self.assertEqual(("src/Shared.cpp",), result.source_paths)
        self.assertEqual(
            ("symbol:recoil:function:0x401000",),
            result.unresolved_target_identities,
        )

    def test_schema_separates_edit_definition_and_dependency_paths(self) -> None:
        closure = CallContractSourceClosure(
            source_edit_paths=("src/Unit.cpp", "src/Unit.h"),
            registered_source_paths=("src/Unit.cpp",),
            header_paths=("src/Unit.h", "src/Shared.h"),
            definition_source_paths=("src/Shared.cpp",),
            dependency_paths=(
                "src/Shared.cpp",
                "src/Shared.h",
                "src/Unit.cpp",
                "src/Unit.h",
            ),
            definition_resolution={
                "kind": "call-contract-definition-source-resolution",
                "contract_version": 2,
                "mode": "conservative-full-closure",
                "ambiguity_policy": "fail-closed",
            },
        )

        self.assertNotIn("src/Shared.cpp", closure.source_edit_paths)
        self.assertIn("src/Shared.cpp", closure.definition_source_paths)
        self.assertIn("src/Shared.cpp", closure.dependency_paths)
        self.assertEqual(
            "conservative-full-closure",
            closure.definition_resolution["mode"],
        )

    def test_target_scope_joins_unique_bodies_across_original_slices(self) -> None:
        with patch(
            "_recoil.commands.call_contract_verify._call_contract_cached_manifest",
            return_value=SimpleNamespace(order_edit_paths=("src/Unit.cpp",)),
        ):
            result = _resolve_target_all_authored_bodies(
                FixtureDocument(), TARGET_ID
            )

        self.assertEqual(2, result["body_count"])
        self.assertEqual(
            ["recoil:function:0x401000", "recoil:function:0x401020"],
            result["symbol_ids"],
        )
        self.assertEqual(
            [
                "recoil:call-contract-slice:first",
                "recoil:call-contract-slice:second",
            ],
            result["original_slice_ids"],
        )
        self.assertEqual([TARGET_ID], result["target_ids"])

    def test_target_scope_fails_closed_for_unknown_target(self) -> None:
        with self.assertRaisesRegex(ProgressError, "exact registered VC5"):
            _resolve_target_all_authored_bodies(
                FixtureDocument(), "recoil:vc5-target:missing"
            )

    def test_lazy_target_session_compiles_selected_target_once(self) -> None:
        document = FixtureDocument()
        with patch(
            "_recoil.commands.call_contract_verify._call_contract_cached_manifest",
            return_value=SimpleNamespace(order_edit_paths=("src/Unit.cpp",)),
        ):
            scope = _resolve_target_all_authored_bodies(document, TARGET_ID)
        sentinel_first = object()
        sentinel_second = object()
        with patch(
            "_recoil.commands.call_contract_verify._compile_slice_candidates",
            return_value={
                "0x401000": sentinel_first,
                "0x401020": sentinel_second,
            },
        ) as compile_candidates:
            session = _LazyCallContractCandidateSession(
                document,
                scope,
                build_root=Path("build/unit-call-contract"),
                vc5_env=Path("build/unit-vc5-env.cmd"),
                preloaded_targets={TARGET_ID: object()},
            )
            self.assertIs(sentinel_first, session.candidate("0x401000"))
            self.assertIs(sentinel_second, session.candidate("0x401020"))

        compile_candidates.assert_called_once()
        self.assertEqual([TARGET_ID], session.compiled_target_ids)

    def test_target_cli_requires_explicit_nonaccepting_all_body_flag(self) -> None:
        args = build_parser().parse_args(
            [
                "--target",
                TARGET_ID,
                "--all-authored-bodies",
                "--build-root",
                "build/unit-call-contract",
                "--json",
            ]
        )
        self.assertEqual(TARGET_ID, args.target)
        self.assertTrue(args.all_authored_bodies)

    def test_target_result_kind_cannot_feed_slice_acceptance(self) -> None:
        with self.assertRaisesRegex(ProgressError, "wrong governed direct result"):
            _validate_call_contract_result(
                {
                    "kind": "authored-call-contract-target-convergence-result",
                    "contract_version": 1,
                    "candidate_expected_truth": False,
                },
                expected_slice={"id": "slice"},
                expected_source_write_paths=[],
                expected_definition_source_paths=[],
                expected_compiled_definition_sources=[],
                expected_dependency_paths=[],
            )


if __name__ == "__main__":
    unittest.main()
