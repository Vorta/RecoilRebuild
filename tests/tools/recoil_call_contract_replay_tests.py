from __future__ import annotations

from contextlib import redirect_stderr, redirect_stdout
from copy import deepcopy
import io
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands import call_contract_replay as replay  # noqa: E402
from _recoil.commands import call_contract_verify  # noqa: E402
from _recoil.commands import progress_cli  # noqa: E402
from _recoil.commands.call_contract_verify import (  # noqa: E402
    CallContractSourceClosure,
)
from _recoil.lib.call_contract_generations import current_generations  # noqa: E402
from _recoil.lib.progress import ProgressError  # noqa: E402


class FakeVector:
    def __init__(self, transaction: int, semantic: int, evidence: int) -> None:
        self.value = {
            "transaction_revision": transaction,
            "semantic_revision": semantic,
            "evidence_generation_revision": evidence,
        }

    def to_dict(self) -> dict[str, int]:
        return dict(self.value)


class FakeStore:
    def __init__(self, rows: list[tuple[dict[str, object], FakeVector]]) -> None:
        self.rows = list(rows)

    def materialize_with_revision_vector(
        self,
    ) -> tuple[dict[str, object], FakeVector]:
        if not self.rows:
            raise AssertionError("unexpected progress materialization")
        return self.rows.pop(0)


class FakeDocument:
    def __init__(
        self,
        slices: list[dict[str, object]],
        *,
        current_index: int,
        current_symbols: set[str] | None = None,
    ) -> None:
        self.data: dict[str, object] = {"revision": 10}
        self.path = REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
        self._slices = slices
        self.current_index = current_index
        self.current_symbols = set(current_symbols or ())

    @property
    def revision(self) -> int:
        return int(self.data["revision"])

    def authored_call_contract_slices(self) -> list[dict[str, object]]:
        return deepcopy(self._slices)

    def call_contract_body_currentness(self, symbol_id: str) -> dict[str, bool]:
        return {"current": symbol_id in self.current_symbols}

    def pipeline(self, *_args: object, **_kwargs: object) -> dict[str, object]:
        slice_id = (
            str(self._slices[self.current_index]["id"])
            if self.current_index < len(self._slices)
            else ""
        )
        return {
            "phase": "authored-call-contract",
            "authored_call_contract_slice_id": slice_id,
        }


def make_slice(index: int) -> dict[str, object]:
    address = f"0x{0x401000 + index * 0x10:x}"
    return {
        "id": f"recoil:call-contract-slice:{address}-{address}",
        "body_count": 1,
        "symbol_ids": [f"symbol-{index}"],
        "addresses": [address],
        "target_ids": [f"target-{index}"],
        "physical_block_ids": [f"block-{index}"],
        "source_paths": [f"src/source-{index}.cpp"],
    }


def passing_projection(slice_row: dict[str, object]) -> dict[str, object]:
    symbol_id = str(slice_row["symbol_ids"][0])
    return {
        "slice_row": deepcopy(slice_row),
        "closure": Mock(),
        "result": {
            "passed": True,
            "passing_symbol_ids": [symbol_id],
            "body_results": [],
            "exact_fact_transcript": [],
            "provider_fact_transcript": [],
            "first_divergence": None,
        },
    }


class CallContractReplayTests(unittest.TestCase):
    def setUp(self) -> None:
        self.progress = REPO_ROOT / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
        self.slices = [make_slice(0), make_slice(1)]
        self.initial_vector = FakeVector(100, 90, 80)
        self.direct_root = (
            REPO_ROOT
            / "build"
            / "live-validation"
            / "call-contract"
            / "401000-401000-r101"
        )
        self.replay_root = self.direct_root.with_name(
            self.direct_root.name + "-replay-001"
        )

    def args(self, *, apply: bool) -> SimpleNamespace:
        return SimpleNamespace(
            progress=self.progress,
            apply=apply,
            dry_run=not apply,
            json=True,
        )

    def action(self, *, closeout: bool = False) -> dict[str, object]:
        if closeout:
            return {
                "kind": "closeout",
                "acceptance_command": "python tools/recoil.py progress call-contract close-live ...",
                "revision_vector": self.initial_vector.to_dict(),
            }
        return {
            "kind": "slice",
            "slice_id": self.slices[0]["id"],
            "direct_build_root": self.direct_root,
            "acceptance_command": "direct",
            "revision_vector": self.initial_vector.to_dict(),
        }

    def test_dry_run_plans_full_census_without_proof_or_mutation(self) -> None:
        document = FakeDocument(self.slices, current_index=0)
        store = FakeStore([({"doc": document}, self.initial_vector)])
        with (
            patch.object(progress_cli, "ProgressSQLiteStore", return_value=store),
            patch.object(
                progress_cli.ProgressDocument,
                "_from_owned_data",
                return_value=document,
            ),
            patch.object(
                progress_cli,
                "_call_contract_replay_current_action",
                return_value=self.action(),
            ),
            patch.object(
                progress_cli,
                "_call_contract_replay_root",
                return_value=self.replay_root,
            ) as root,
            patch.object(progress_cli, "live_call_contract_result") as verifier,
            patch.object(progress_cli, "_commit_validated_call_contract_slice") as commit,
        ):
            returncode, payload = progress_cli.replay_live_call_contract(
                self.args(apply=False)
            )
        self.assertEqual(returncode, 0)
        self.assertEqual(payload["schema"], "recoil-authored-call-contract-stage-replay-v2")
        self.assertEqual(payload["status"], "planned")
        self.assertEqual(payload["proof_session"]["body_count"], 2)
        self.assertEqual(payload["slice_count"], 2)
        root.assert_called_once_with(self.direct_root, create=False)
        verifier.assert_not_called()
        commit.assert_not_called()

    def test_one_phase_proof_commits_slice_projections_serially(self) -> None:
        documents = [
            FakeDocument(self.slices, current_index=0),
            FakeDocument(self.slices, current_index=1, current_symbols={"symbol-0"}),
            FakeDocument(
                self.slices,
                current_index=2,
                current_symbols={"symbol-0", "symbol-1"},
            ),
        ]
        final_vector = FakeVector(102, 92, 82)
        store = FakeStore(
            [
                ({"doc": documents[0]}, self.initial_vector),
                ({"doc": documents[2]}, final_vector),
            ]
        )
        phase_result = {
            "passed": True,
            "attempted_target_ids": ["target-0", "target-1"],
            "compiled_target_ids": ["target-0", "target-1"],
            "definition_compile_results": [],
            "dependency_states_after": [],
            "binary_ninja_fact_cache": {"bridge_read_count": 2},
            "candidate_cod_index": {},
            "timings_ms": {"total": 10.0},
        }
        phase_closure = CallContractSourceClosure((), (), (), (), ())
        projections = [passing_projection(row) for row in self.slices]
        calls: list[str] = []

        def commit_side_effect(**kwargs: object) -> tuple[int, dict[str, object], FakeDocument]:
            index = len(calls)
            slice_id = str(kwargs["slice_row"]["id"])
            calls.append(slice_id)
            prior = {
                "transaction_revision": 100 + index,
                "semantic_revision": 90 + index,
                "evidence_generation_revision": 80 + index,
            }
            following = {key: value + 1 for key, value in prior.items()}
            return (
                0,
                {
                    "status": "passed",
                    "passing_symbol_ids": [f"symbol-{index}"],
                    "first_divergence": None,
                    "commit": {
                        "applied": True,
                        "previous_revision_vector": prior,
                        "revision_vector": following,
                    },
                },
                documents[index + 1],
            )

        with (
            patch.object(progress_cli, "ProgressSQLiteStore", return_value=store),
            patch.object(
                progress_cli.ProgressDocument,
                "_from_owned_data",
                side_effect=[documents[0], documents[2]],
            ),
            patch.object(
                progress_cli,
                "_call_contract_replay_current_action",
                side_effect=[self.action(), self.action(closeout=True)],
            ),
            patch.object(
                progress_cli,
                "_call_contract_replay_root",
                return_value=self.replay_root,
            ),
            patch.object(
                progress_cli,
                "_call_contract_replay_component_state",
                return_value={"stable": True},
            ),
            patch.object(progress_cli, "live_call_contract_result", return_value=phase_result) as verifier,
            patch.object(
                progress_cli,
                "_project_call_contract_phase_result",
                return_value=(projections, phase_closure),
            ),
            patch.object(progress_cli, "_call_contract_replay_require_unchanged"),
            patch.object(
                progress_cli,
                "_commit_validated_call_contract_slice",
                side_effect=commit_side_effect,
            ) as commit,
        ):
            returncode, payload = progress_cli.replay_live_call_contract(
                self.args(apply=True)
            )
        self.assertEqual(returncode, 0)
        self.assertEqual(payload["status"], "closeout-ready")
        self.assertEqual(calls, [row["id"] for row in self.slices])
        self.assertEqual(verifier.call_count, 1)
        self.assertEqual(commit.call_count, 2)
        self.assertEqual(payload["proof_session"]["compiled_target_count"], 2)
        self.assertIsNotNone(payload["closeout_command"])

    def test_stale_source_or_component_state_fails_before_any_commit(self) -> None:
        document = FakeDocument(self.slices, current_index=0)
        store = FakeStore([({"doc": document}, self.initial_vector)])
        phase_result = {
            "passed": True,
            "attempted_target_ids": [],
            "compiled_target_ids": [],
            "definition_compile_results": [],
            "dependency_states_after": [],
        }
        with (
            patch.object(progress_cli, "ProgressSQLiteStore", return_value=store),
            patch.object(progress_cli.ProgressDocument, "_from_owned_data", return_value=document),
            patch.object(progress_cli, "_call_contract_replay_current_action", return_value=self.action()),
            patch.object(progress_cli, "_call_contract_replay_root", return_value=self.replay_root),
            patch.object(progress_cli, "_call_contract_replay_component_state", return_value={"stable": True}),
            patch.object(progress_cli, "live_call_contract_result", return_value=phase_result),
            patch.object(
                progress_cli,
                "_project_call_contract_phase_result",
                return_value=(
                    [passing_projection(row) for row in self.slices],
                    CallContractSourceClosure((), (), (), (), ()),
                ),
            ),
            patch.object(
                progress_cli,
                "_call_contract_replay_require_unchanged",
                side_effect=ProgressError("state changed"),
            ),
            patch.object(progress_cli, "_commit_validated_call_contract_slice") as commit,
        ):
            with self.assertRaisesRegex(ProgressError, "state changed"):
                progress_cli.replay_live_call_contract(self.args(apply=True))
        commit.assert_not_called()

    def test_first_divergent_slice_commits_its_passing_subset_then_stops(self) -> None:
        documents = [FakeDocument(self.slices, current_index=0)] * 2
        store = FakeStore([({"doc": documents[0]}, self.initial_vector)])
        divergence = {"kind": "mismatch", "symbol_id": "symbol-0"}
        projections = [passing_projection(row) for row in self.slices]
        projections[0]["result"].update(
            {
                "passed": False,
                "first_divergence": divergence,
                "passing_symbol_ids": ["symbol-0"],
            }
        )
        phase_result = {
            "passed": False,
            "attempted_target_ids": ["target-0", "target-1"],
            "compiled_target_ids": ["target-0", "target-1"],
            "definition_compile_results": [],
            "dependency_states_after": [],
        }
        next_vector = {
            "transaction_revision": 101,
            "semantic_revision": 91,
            "evidence_generation_revision": 81,
        }
        with (
            patch.object(progress_cli, "ProgressSQLiteStore", return_value=store),
            patch.object(progress_cli.ProgressDocument, "_from_owned_data", return_value=documents[0]),
            patch.object(progress_cli, "_call_contract_replay_current_action", return_value=self.action()),
            patch.object(progress_cli, "_call_contract_replay_root", return_value=self.replay_root),
            patch.object(progress_cli, "_call_contract_replay_component_state", return_value={"stable": True}),
            patch.object(progress_cli, "live_call_contract_result", return_value=phase_result),
            patch.object(
                progress_cli,
                "_project_call_contract_phase_result",
                return_value=(projections, CallContractSourceClosure((), (), (), (), ())),
            ),
            patch.object(progress_cli, "_call_contract_replay_require_unchanged"),
            patch.object(
                progress_cli,
                "_commit_validated_call_contract_slice",
                return_value=(
                    1,
                    {
                        "status": "diverged",
                        "passing_symbol_ids": ["symbol-0"],
                        "first_divergence": divergence,
                        "commit": {
                            "applied": True,
                            "previous_revision_vector": self.initial_vector.to_dict(),
                            "revision_vector": next_vector,
                        },
                    },
                    documents[1],
                ),
            ) as commit,
        ):
            returncode, payload = progress_cli.replay_live_call_contract(
                self.args(apply=True)
            )
        self.assertEqual(returncode, 1)
        self.assertEqual(payload["status"], "diverged")
        self.assertEqual(commit.call_count, 1)
        self.assertEqual(payload["slice_count"], 1)
        self.assertEqual(payload["passing_body_count"], 1)

    def test_phase_projection_is_equivalent_to_existing_slice_validator(self) -> None:
        slices = [make_slice(0), make_slice(1)]
        document = FakeDocument(slices, current_index=0)
        phase_scope = {
            "body_count": 2,
            "symbol_ids": ["symbol-0", "symbol-1"],
            "target_ids": ["target-0", "target-1"],
            "physical_block_ids": ["block-0", "block-1"],
            "original_slice_ids": [row["id"] for row in slices],
            "slice_boundaries": [
                {"slice_id": slices[0]["id"], "start_index": 0, "end_index_exclusive": 1, "body_count": 1},
                {"slice_id": slices[1]["id"], "start_index": 1, "end_index_exclusive": 2, "body_count": 1},
            ],
        }
        phase_closure = CallContractSourceClosure(
            ("src/a.cpp", "src/b.cpp"),
            ("src/a.cpp", "src/b.cpp"),
            (),
            ("src/b.cpp", "defs/a.cpp"),
            ("dep/a.h", "dep/b.h"),
        )
        closures = {
            str(slices[0]["id"]): CallContractSourceClosure(
                ("src/a.cpp",),
                ("src/a.cpp",),
                (),
                ("src/b.cpp", "defs/a.cpp"),
                ("dep/a.h",),
            ),
            str(slices[1]["id"]): CallContractSourceClosure(
                ("src/b.cpp",), ("src/b.cpp",), (), (), ("dep/b.h",)
            ),
            "phase": phase_closure,
        }

        def body(symbol_id: str, address: str, target_id: str) -> dict[str, object]:
            expected = {
                "symbol_id": symbol_id,
                "address": address,
                "calls": [],
                **current_generations(),
            }
            return {
                "symbol_id": symbol_id,
                "address": address,
                "target_id": target_id,
                "status": "passed",
                "comparison_passed": True,
                "divergence": None,
                "expected_fact_row": expected,
                "expected_contract": [],
                "candidate_contract": [],
                "normalizers": [],
                **current_generations(),
            }

        bodies = [
            body("symbol-0", str(slices[0]["addresses"][0]), "target-0"),
            body("symbol-1", str(slices[1]["addresses"][0]), "target-1"),
        ]
        states = [
            {"path": "dep/a.h", "exists": True},
            {"path": "dep/b.h", "exists": True},
        ]
        phase_result = {
            "kind": "authored-call-contract-phase-replay-result",
            "contract_version": 3,
            "phase_all_authored_bodies": True,
            "nonaccepting": True,
            "acceptance_eligible": False,
            "acceptance_route": "project-to-original-slices",
            "all_caller_divergences_collected": True,
            "candidate_expected_truth": False,
            **phase_scope,
            "attempted_target_ids": ["target-0", "target-1"],
            "compiled_target_ids": ["target-0", "target-1"],
            "source_changed_during_validation": False,
            "dependency_states_before": states,
            "dependency_states_after": deepcopy(states),
            "source_edit_paths": list(phase_closure.source_edit_paths),
            "definition_source_paths": list(phase_closure.definition_source_paths),
            "dependency_paths": list(phase_closure.dependency_paths),
            "definition_compile_results": [{"source": "defs/a.cpp", "returncode": 0}],
            "body_results": bodies,
            "exact_fact_transcript": [
                {"symbol_id": row["symbol_id"], "address": row["address"], "expected_fact_row": row["expected_fact_row"]}
                for row in bodies
            ],
            "provider_fact_transcript": [],
            "caller_divergences": [],
            "passed": True,
            "first_divergence": None,
        }

        def closure_for(_document: object, row: dict[str, object], **_kwargs: object) -> CallContractSourceClosure:
            return closures.get(str(row.get("id")), phase_closure)

        with (
            patch.object(progress_cli, "_resolve_phase_all_authored_bodies", return_value=phase_scope),
            patch.object(progress_cli, "load_repository_path_inventory", return_value=Mock()),
            patch.object(progress_cli, "call_contract_source_closure", side_effect=closure_for),
        ):
            projections, selected_phase_closure = (
                progress_cli._project_call_contract_phase_result(document, phase_result)
            )
        self.assertEqual(selected_phase_closure, phase_closure)
        self.assertEqual(len(projections), 2)
        self.assertTrue(all(row["result"]["passed"] for row in projections))
        self.assertEqual(
            projections[0]["result"]["passing_symbol_ids"], ["symbol-0"]
        )
        self.assertEqual(
            projections[1]["result"]["passing_symbol_ids"], ["symbol-1"]
        )

    def test_interruption_returns_130_and_existing_root_selects_resume_sibling(self) -> None:
        with (
            patch.object(replay, "replay_live", side_effect=KeyboardInterrupt),
            redirect_stdout(io.StringIO()),
            redirect_stderr(io.StringIO()),
        ):
            self.assertEqual(replay.main(["--apply", "--json"]), 130)

        with tempfile.TemporaryDirectory() as temporary:
            temporary_root = Path(temporary)
            with patch.object(progress_cli, "REPO_ROOT", temporary_root):
                direct = (
                    temporary_root
                    / "build"
                    / "live-validation"
                    / "call-contract"
                    / "scheduler-root"
                )
                first = progress_cli._call_contract_replay_root(direct, create=True)
                second = progress_cli._call_contract_replay_root(direct, create=True)
        self.assertTrue(first.name.endswith("replay-001"))
        self.assertTrue(second.name.endswith("replay-002"))

    def test_full_census_binary_ninja_facts_are_read_once_per_request(self) -> None:
        class Bridge:
            def __init__(self) -> None:
                self.assembly_calls = 0
                self.json_calls = 0
                self.hexdump_calls = 0

            def assembly(self, value: str) -> str:
                self.assembly_calls += 1
                return f"assembly:{value}"

            def get_json(self, endpoint: str, **params: object) -> dict[str, object]:
                self.json_calls += 1
                return {"endpoint": endpoint, "params": params}

            def hexdump(self, address: str, length: int) -> str:
                self.hexdump_calls += 1
                return f"{address}:{length}"

        bridge = Bridge()
        cache = call_contract_verify._CallContractBinaryNinjaFactCache(bridge)
        cache.preload_assemblies(["0x401000", "0x401000"])
        self.assertEqual(cache.assembly("0x401000"), "assembly:0x401000")
        self.assertEqual(
            cache.get_json("xrefs", address="0x401000"),
            cache.get_json("xrefs", address="0x401000"),
        )
        self.assertEqual(
            cache.hexdump("0x401000", 8),
            cache.hexdump("0x401000", 8),
        )
        self.assertEqual(bridge.assembly_calls, 1)
        self.assertEqual(bridge.json_calls, 1)
        self.assertEqual(bridge.hexdump_calls, 1)
        self.assertEqual(cache.metrics()["bridge_read_count"], 3)
        self.assertEqual(cache.metrics()["cache_hit_count"], 4)


if __name__ == "__main__":
    unittest.main()
