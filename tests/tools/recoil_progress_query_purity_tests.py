from __future__ import annotations

from copy import deepcopy
import io
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from _recoil.commands import call_contract_verify, progress_cli  # noqa: E402
from _recoil.lib.call_contract_generations import current_generations  # noqa: E402
from _recoil.lib.progress import (  # noqa: E402
    CALL_CONTRACT_DIMENSION,
    ProgressDocument,
    ProgressError,
    empty_progress_document,
    state_record,
)


class ProgressQueryPurityTests(unittest.TestCase):
    def _document(self, population: str) -> tuple[ProgressDocument, dict[str, object]]:
        data = empty_progress_document()
        symbols = ("recoil:function:0x401000", "recoil:function:0x401010")
        for index, symbol_id in enumerate(symbols):
            address = f"0x{0x401000 + index * 0x10:x}"
            evidence_id = f"recoil:evidence:query-purity:{index}"
            data["symbols"][symbol_id] = {
                "binary": "recoil",
                "kind": "function",
                "address": address,
                "end_exclusive": f"0x{0x401010 + index * 0x10:x}",
                "physical_block_id": "recoil:block:fixture",
                "binary_state": {
                    CALL_CONTRACT_DIMENSION: state_record(
                        "passed",
                        "accepted",
                        "current",
                        [evidence_id],
                        validation_mode="live",
                    )
                },
            }
            snapshot = {
                "saved_view": "Recoil.bndb",
                "generation_token": "fixture-generation-7",
                "revision": "fixture-revision-11",
                "schema": "recoil-binja-authenticated-snapshot-v2",
                "authenticated": True,
                "provider": "binary-ninja",
                "capability_version": "2",
            }
            evidence = {
                "kind": "live-authored-call-contract-validation",
                "result": "passed",
                "disposition": "accepted",
                "freshness": "current",
                "validation_mode": "live",
                "gating": True,
                "scope_ids": [symbol_id],
                "provenance": {
                    **current_generations(),
                    "symbol_id": symbol_id,
                    "address": address,
                    "physical_block_id": "recoil:block:fixture",
                    "comparison_passed": True,
                    "expected_contract": [{"form": "call"}],
                    "candidate_contract": [{"form": "call"}],
                    "binary_ninja_session": {
                        "begin": deepcopy(snapshot),
                        "end": deepcopy(snapshot),
                        "snapshot_equal": True,
                        "exact_fact_transcript": [
                            {"symbol_id": symbol_id, "calls": [{"form": "call"}]}
                        ],
                    },
                },
            }
            if population == "valid":
                data["evidence"][evidence_id] = evidence
            elif population == "stale-generation":
                evidence["provenance"]["call_contract_verifier_generation"] -= 1
                data["evidence"][evidence_id] = evidence
            elif population == "session-changed":
                evidence["provenance"]["binary_ninja_session"]["end"] = {
                    **snapshot,
                    "revision": "fixture-revision-12",
                }
                data["evidence"][evidence_id] = evidence
            elif population == "legacy-session":
                legacy = {
                    "provider_identity": "binary-ninja",
                    "provider_generation": 7,
                    "saved_view_revision": 11,
                }
                evidence["provenance"]["binary_ninja_session"]["begin"] = legacy
                evidence["provenance"]["binary_ninja_session"]["end"] = deepcopy(legacy)
                data["evidence"][evidence_id] = evidence
            elif population == "wrong-provider":
                evidence["provenance"]["binary_ninja_session"]["begin"][
                    "provider"
                ] = "bridge-proxy"
                evidence["provenance"]["binary_ninja_session"]["end"][
                    "provider"
                ] = "bridge-proxy"
                data["evidence"][evidence_id] = evidence
        return ProgressDocument(data), {"id": "slice", "symbol_ids": list(symbols)}

    def _side_effect_guards(self):
        return (
            mock.patch.object(
                call_contract_verify,
                "live_call_contract_result",
                side_effect=AssertionError("live verifier invoked"),
            ),
            mock.patch.object(
                call_contract_verify,
                "BinaryNinjaBridge",
                side_effect=AssertionError("Binary Ninja invoked"),
            ),
            mock.patch("subprocess.run", side_effect=AssertionError("subprocess invoked")),
            mock.patch(
                "tempfile.TemporaryDirectory",
                side_effect=AssertionError("temporary build created"),
            ),
        )

    def test_body_and_slice_queries_are_local_only(self) -> None:
        expected = {
            "missing": (False, "evidence-missing"),
            "valid": (True, "accepted-and-not-invalidated"),
            "stale-generation": (False, "verifier-generation-changed"),
            "session-changed": (False, "body-evidence-binding-invalid"),
            "legacy-session": (False, "binary-ninja-snapshot-invalid"),
            "wrong-provider": (False, "binary-ninja-snapshot-invalid"),
        }
        for population, (current, reason) in expected.items():
            with self.subTest(population=population):
                document, slice_row = self._document(population)
                guards = self._side_effect_guards()
                with guards[0], guards[1], guards[2], guards[3]:
                    body = document.call_contract_body_currentness(
                        "recoil:function:0x401000"
                    )
                    status = document._call_contract_slice_status(slice_row)
                    document.show("recoil:function:0x401000")
                    document.summary()
                self.assertEqual(current, body["current"])
                self.assertEqual(reason, body["reason"])
                self.assertEqual(current, status["current"])
                self.assertEqual(
                    "accepted-state-explicit-invalidation", status["storage_mode"]
                )

    def test_binary_ninja_exact_snapshot_receipt_and_transcript_bind_state(self) -> None:
        document, _slice = self._document("valid")
        current = document.call_contract_body_currentness(
            "recoil:function:0x401000"
        )
        self.assertTrue(current["current"])
        evidence = document.collection("evidence")[current["evidence_id"]]
        session = evidence["provenance"]["binary_ninja_session"]
        self.assertEqual("binary-ninja", session["begin"]["provider"])
        self.assertEqual("fixture-generation-7", session["begin"]["generation_token"])
        self.assertEqual("fixture-revision-11", session["begin"]["revision"])
        self.assertEqual(session["begin"], session["end"])
        self.assertTrue(session["snapshot_equal"])
        self.assertEqual(1, len(session["exact_fact_transcript"]))

    def test_missing_required_verifier_component_invalidates_currentness(self) -> None:
        document, _slice = self._document("valid")
        with mock.patch(
            "_recoil.lib.progress.required_call_contract_verifier_component_findings",
            return_value=[
                {
                    "kind": "missing",
                    "path": "tools/_recoil/lib/binja.py",
                    "detail": "required component is absent",
                }
            ],
        ):
            current = document.call_contract_body_currentness(
                "recoil:function:0x401000"
            )
        self.assertFalse(current["current"])
        self.assertEqual("verifier-component-unavailable", current["reason"])

    def test_same_document_freshly_observes_required_component_failures(self) -> None:
        for kind in ("missing", "unreadable", "unparseable"):
            with self.subTest(kind=kind):
                document, _slice = self._document("valid")
                finding = {
                    "kind": kind,
                    "path": "tools/_recoil/lib/binja.py",
                    "detail": f"injected {kind} component",
                }
                with mock.patch(
                    "_recoil.lib.progress."
                    "required_call_contract_verifier_component_findings",
                    side_effect=[[], [finding]],
                ) as current_findings:
                    valid = document.call_contract_body_currentness(
                        "recoil:function:0x401000"
                    )
                    invalid = document.call_contract_body_currentness(
                        "recoil:function:0x401000"
                    )
                self.assertTrue(valid["current"])
                self.assertFalse(invalid["current"])
                self.assertEqual("verifier-component-unavailable", invalid["reason"])
                self.assertEqual([finding], invalid["component_findings"])
                self.assertEqual(2, current_findings.call_count)

    def test_removed_scheduler_output_cache_has_no_runtime_entry_points(self) -> None:
        for name in (
            "scheduler_cache",
            "_scheduler_output_cache_state",
            "_probe_scheduler_output_cache",
            "_store_scheduler_output_cache",
        ):
            self.assertFalse(hasattr(progress_cli, name), name)

    def test_expensive_verification_fails_before_work_without_reservation(self) -> None:
        document, _slice = self._document("missing")
        build_root = ROOT / "build" / "query-purity-no-reservation"
        with mock.patch(
            "_recoil.lib.binja.BinaryNinjaBridge",
            side_effect=AssertionError("Binary Ninja constructed before reservation"),
        ), mock.patch.object(
            progress_cli,
            "_run_json_process",
            side_effect=AssertionError("compiler invoked before reservation"),
        ):
            with self.assertRaisesRegex(ProgressError, "existing work packet"):
                progress_cli._preflight_call_contract_expensive_operation(
                    document,
                    packet_id="missing",
                    slice_id="slice",
                    build_root=build_root,
                    acceptance=False,
                )

    def test_expensive_verification_requires_exact_active_resources(self) -> None:
        document, _slice = self._document("missing")
        build_root = ROOT / "build" / "query-purity-owned-root"
        claims = [
            {"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"},
            {"kind": "reference", "id": "support/Recoil.exe", "access": "read"},
            {"kind": "tracker", "id": "recoil", "access": "read"},
            {
                "kind": "output-root",
                "id": "build/query-purity-owned-root",
                "access": "write",
            },
        ]
        work = {
            "packet_type": "call-contract-edit-v1",
            "state": "active",
            "slice_id": "slice",
            "reservation": {
                "id": "reservation:1",
                "state": "active",
                "resource_claims": deepcopy(claims),
            },
            "resource_claims": deepcopy(claims),
        }
        document.data["work_items"]["packet"] = work
        with mock.patch.object(
            progress_cli, "authenticate_explicit_output_root", return_value={}
        ):
            receipt = progress_cli._preflight_call_contract_expensive_operation(
                document,
                packet_id="packet",
                slice_id="slice",
                build_root=build_root,
                acceptance=False,
            )
        self.assertEqual("reservation:1", receipt["reservation_id"])
        work["resource_claims"] = work["resource_claims"][:-1]
        work["reservation"]["resource_claims"] = deepcopy(work["resource_claims"])
        with mock.patch.object(
            progress_cli, "authenticate_explicit_output_root", return_value={}
        ), self.assertRaisesRegex(ProgressError, "required verification resources"):
            progress_cli._preflight_call_contract_expensive_operation(
                document,
                packet_id="packet",
                slice_id="slice",
                build_root=build_root,
                acceptance=False,
            )

    def test_acceptance_parser_requires_packet_id(self) -> None:
        base = [
            "advance-live-call-contract",
            "--slice",
            "slice",
            "--build-root",
            "build/accept",
            "--expected-semantic-revision",
            "1",
            "--expected-evidence-generation-revision",
            "1",
            "--apply",
        ]
        with mock.patch("sys.stderr", new=io.StringIO()), self.assertRaises(SystemExit):
            progress_cli._parser().parse_args(base)
        parsed = progress_cli._parser().parse_args([*base, "--packet-id", "packet"])
        self.assertEqual("packet", parsed.packet_id)

    def test_query_command_dispatch_is_repeatable_and_nonwriting(self) -> None:
        class QueryDocument:
            revision = 17

            def __init__(self) -> None:
                self.data = {"work_items": {"packet": {"state": "ready"}}}

            def pipeline(self, _binary: str):
                return {"phase": "authored-call-contract", "cursor": "0x401000"}

            def show(self, selector: str):
                return {"selector": selector, "record": {"state": "stored"}}

            def summary(self):
                return {"pipeline": self.pipeline("recoil"), "summary": "stored"}

            def next_work(self, _binary: str):
                return {"phase": "authored-call-contract", "cursor": "0x401000"}

            def collection(self, name: str):
                return self.data[name]

            def scheduler_output(self, payload):
                return deepcopy(payload)

        document = QueryDocument()
        before = deepcopy(document.data)
        invocations = (
            ["status", "--json"],
            ["next", "--json"],
            ["show", "recoil:function:0x401000", "--json"],
            ["report", "--json"],
            ["work", "show", "--json"],
        )
        first: list[object] = []
        second: list[object] = []
        guards = self._side_effect_guards()
        with mock.patch.object(progress_cli, "_load", return_value=document), mock.patch.object(
            progress_cli,
            "_scheduler_domain_guarded_call_contract_commands",
            side_effect=lambda _document, payload: payload,
        ), mock.patch.object(
            progress_cli,
            "read_issue_metadata",
            return_value=SimpleNamespace(revision=1),
        ), guards[0], guards[1], guards[2], guards[3]:
            for target in (first, second):
                for argv in invocations:
                    with mock.patch.object(
                        progress_cli,
                        "_print_json",
                        side_effect=lambda value: target.append(deepcopy(value)),
                    ):
                        self.assertEqual(0, progress_cli.main(list(argv)))
        self.assertEqual(first, second)
        self.assertEqual(before, document.data)


if __name__ == "__main__":
    unittest.main()
