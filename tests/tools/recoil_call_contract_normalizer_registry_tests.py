from __future__ import annotations

from copy import deepcopy
from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.lib.call_contract_generations import (
    CALL_CONTRACT_VERIFIER_COMPONENT_PATHS,
    CALL_CONTRACT_VERIFIER_GENERATION,
    EXPECTED_FACT_COMPONENT_PATHS,
    EXPECTED_FACT_SCHEMA_VERSION,
    NORMALIZER_REGISTRY_COMPONENT_PATHS,
    NORMALIZER_REGISTRY_GENERATION,
    GenerationError,
    required_call_contract_verifier_component_findings,
    required_call_contract_verifier_component_graph,
)
from _recoil.lib.call_contract_normalizers import (
    LIVE_CALL_CONTRACT_NORMALIZER_REGISTRY,
    NormalizerDefinitionError,
    NormalizerRegistry,
    NormalizerUseReceiptError,
    REQUIRED_CALL_CONTRACT_VERIFIER_COMPONENTS,
    UntrackedNormalizerUseError,
    current_call_contract_verifier_components,
    normalize_emitted_call_rows,
)


REGISTRY_ID = "recoil:call-contract:normalizer-registry:test:v2"
NORMALIZER_ID = "recoil:call-contract:normalizer:fixture:v2"


def _fixture_normalizer(value: int) -> int:
    return value + 1


class CallContractNormalizerRegistryTests(unittest.TestCase):
    def _registry(self) -> tuple[NormalizerRegistry, object]:
        registry = NormalizerRegistry(REGISTRY_ID)
        wrapped = registry.register_normalizer(NORMALIZER_ID, _fixture_normalizer)
        return registry, wrapped

    def test_identity_uses_reviewed_integer_generation(self):
        registry, _wrapped = self._registry()
        self.assertEqual(
            {
                "kind": "call-contract-normalizer-generation",
                "component_id": NORMALIZER_ID,
                "component_generation": NORMALIZER_REGISTRY_GENERATION,
            },
            registry.normalizer_identity(NORMALIZER_ID),
        )
        self.assertEqual(12, NORMALIZER_REGISTRY_GENERATION)

    def test_catalog_is_sorted_and_generation_scoped(self):
        registry = NormalizerRegistry(REGISTRY_ID)
        registry.register_normalizer(
            "recoil:call-contract:normalizer:zeta:v2", _fixture_normalizer
        )
        registry.register_normalizer(
            "recoil:call-contract:normalizer:alpha:v2", _fixture_normalizer
        )
        catalog = registry.catalog_identity()
        self.assertEqual("call-contract-normalizer-catalog-v2", catalog["kind"])
        self.assertEqual(
            NORMALIZER_REGISTRY_GENERATION,
            catalog["registry_generation"],
        )
        self.assertEqual(
            [
                "recoil:call-contract:normalizer:alpha:v2",
                "recoil:call-contract:normalizer:zeta:v2",
            ],
            [row["component_id"] for row in catalog["normalizers"]],
        )

    def test_runtime_use_record_is_body_scoped_unique_and_sorted(self):
        registry = NormalizerRegistry(REGISTRY_ID)
        second_id = "recoil:call-contract:normalizer:second:v2"
        first = registry.register_normalizer(NORMALIZER_ID, _fixture_normalizer)
        second = registry.register_normalizer(second_id, _fixture_normalizer)
        with registry.record_body("recoil:function:0x401000") as use:
            second(1)
            first(4)
            first(8)
        record = use.receipt()
        self.assertEqual("call-contract-normalizer-use-record", record["kind"])
        self.assertEqual(2, record["contract_version"])
        self.assertEqual(
            NORMALIZER_REGISTRY_GENERATION,
            record["registry_generation"],
        )
        self.assertEqual(
            sorted([NORMALIZER_ID, second_id]),
            [row["normalizer_id"] for row in record["normalizers"]],
        )
        self.assertEqual(
            [NORMALIZER_REGISTRY_GENERATION, NORMALIZER_REGISTRY_GENERATION],
            [row["component_generation"] for row in record["normalizers"]],
        )
        self.assertEqual(record, registry.validate_use_receipt(record))

    def test_untracked_or_failed_scope_cannot_issue_a_record(self):
        registry, wrapped = self._registry()
        with self.assertRaises(UntrackedNormalizerUseError):
            wrapped(1)
        captured = None
        with self.assertRaisesRegex(RuntimeError, "fixture failure"):
            with registry.record_body("recoil:function:0x401000") as use:
                captured = use
                wrapped(1)
                raise RuntimeError("fixture failure")
        self.assertIsNotNone(captured)
        with self.assertRaisesRegex(NormalizerUseReceiptError, "unavailable"):
            captured.receipt()

    def test_nested_body_scopes_are_rejected(self):
        registry, _wrapped = self._registry()
        with registry.record_body("recoil:function:0x401000"):
            with self.assertRaisesRegex(NormalizerUseReceiptError, "cannot nest"):
                with registry.record_body("recoil:function:0x401010"):
                    pass

    def test_duplicate_and_unknown_normalizer_fail_closed(self):
        registry, _wrapped = self._registry()
        with self.assertRaisesRegex(NormalizerDefinitionError, "duplicate normalizer"):
            registry.register_normalizer(NORMALIZER_ID, _fixture_normalizer)
        with self.assertRaisesRegex(NormalizerDefinitionError, "unknown normalizer"):
            registry.normalizer_identity(
                "recoil:call-contract:normalizer:not-registered:v2"
            )

    def test_record_validation_rejects_stale_generation(self):
        registry, wrapped = self._registry()
        with registry.record_body("recoil:function:0x401000") as use:
            wrapped(4)
        record = use.receipt()
        stale = deepcopy(record)
        stale["registry_generation"] = NORMALIZER_REGISTRY_GENERATION - 1
        self.assertEqual(11, stale["registry_generation"])
        with self.assertRaisesRegex(NormalizerUseReceiptError, "stale or malformed"):
            registry.validate_use_receipt(stale)
        stale_row = deepcopy(record)
        stale_row["normalizers"][0]["component_generation"] = (
            NORMALIZER_REGISTRY_GENERATION - 1
        )
        self.assertEqual(
            11,
            stale_row["normalizers"][0]["component_generation"],
        )
        with self.assertRaisesRegex(NormalizerUseReceiptError, "stale or malformed"):
            registry.validate_use_receipt(stale_row)

    def test_verifier_identity_exposes_integer_generations_and_paths(self):
        registry, _wrapped = self._registry()
        identity = registry.verifier_core_identity()
        self.assertEqual("call-contract-verifier-generation", identity["kind"])
        self.assertEqual(2, identity["contract_version"])
        self.assertEqual(
            CALL_CONTRACT_VERIFIER_GENERATION,
            identity["call_contract_verifier_generation"],
        )
        self.assertEqual(
            NORMALIZER_REGISTRY_GENERATION,
            identity["normalizer_registry_generation"],
        )
        self.assertEqual(
            EXPECTED_FACT_SCHEMA_VERSION,
            identity["expected_fact_schema_version"],
        )
        self.assertEqual(
            sorted(CALL_CONTRACT_VERIFIER_COMPONENT_PATHS),
            identity["component_paths"],
        )
        self.assertIn(
            "tools/_recoil/commands/progress_cli.py",
            CALL_CONTRACT_VERIFIER_COMPONENT_PATHS,
        )
        self.assertIn(
            "tools/_recoil/lib/repository_paths.py",
            CALL_CONTRACT_VERIFIER_COMPONENT_PATHS,
        )
        self.assertIn(
            "tools/_recoil/lib/repository_paths.py",
            EXPECTED_FACT_COMPONENT_PATHS,
        )
        self.assertNotIn(
            "tools/_recoil/lib/repository_paths.py",
            NORMALIZER_REGISTRY_COMPONENT_PATHS,
        )
        self.assertNotIn(
            "tools/_recoil/commands/workspace_packet_handoff.py",
            CALL_CONTRACT_VERIFIER_COMPONENT_PATHS,
        )

    def test_live_component_projection_has_no_content_summary(self):
        projection = current_call_contract_verifier_components()
        self.assertEqual(2, projection["contract_version"])
        self.assertEqual(
            CALL_CONTRACT_VERIFIER_GENERATION,
            projection["verifier_generation"],
        )
        self.assertEqual(
            NORMALIZER_REGISTRY_GENERATION,
            projection["normalizer_registry_generation"],
        )
        self.assertEqual(
            ["recoil:call-contract:normalizer:emitted-call-rows:v2"],
            [row["id"] for row in projection["normalizers"]],
        )
        self.assertEqual(
            [NORMALIZER_REGISTRY_GENERATION],
            [row["generation"] for row in projection["normalizers"]],
        )
        self.assertEqual(
            sorted(CALL_CONTRACT_VERIFIER_COMPONENT_PATHS),
            projection["component_paths"],
        )
        self.assertEqual(projection, current_call_contract_verifier_components())

    def test_live_generation_constants_are_twelve(self):
        self.assertEqual(12, CALL_CONTRACT_VERIFIER_GENERATION)
        self.assertEqual(12, NORMALIZER_REGISTRY_GENERATION)
        self.assertEqual(12, EXPECTED_FACT_SCHEMA_VERSION)

    def test_required_component_graph_is_shared_and_operational(self):
        graph = required_call_contract_verifier_component_graph()
        self.assertEqual(
            sorted(CALL_CONTRACT_VERIFIER_COMPONENT_PATHS),
            [row["path"] for row in graph],
        )
        self.assertEqual(
            list(REQUIRED_CALL_CONTRACT_VERIFIER_COMPONENTS), list(graph)
        )
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for row in graph:
                path = root / Path(*row["path"].split("/"))
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("pass\n", encoding="utf-8")
            self.assertEqual(
                [], required_call_contract_verifier_component_findings(root)
            )
            projection = current_call_contract_verifier_components(str(root))
            self.assertEqual([row["path"] for row in graph], projection["component_paths"])

            missing = root / Path(*graph[0]["path"].split("/"))
            missing.unlink()
            findings = required_call_contract_verifier_component_findings(root)
            self.assertEqual("missing", findings[0]["kind"])
            with self.assertRaisesRegex(GenerationError, "is missing"):
                current_call_contract_verifier_components(str(root))

    def test_required_component_check_reports_unreadable_and_unparseable(self):
        graph = required_call_contract_verifier_component_graph()
        for expected_kind, payload in (
            ("unreadable", b"\xff\xfe"),
            ("unparseable", b"def broken(:\n"),
        ):
            with self.subTest(kind=expected_kind), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                for row in graph:
                    path = root / Path(*row["path"].split("/"))
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text("pass\n", encoding="utf-8")
                target = root / Path(*graph[0]["path"].split("/"))
                target.write_bytes(payload)
                findings = required_call_contract_verifier_component_findings(root)
                self.assertEqual(expected_kind, findings[0]["kind"])

    def test_emitted_rows_are_copied_as_deterministic_structure(self):
        rows = [
            {"target": "provider:USER32!MessageBoxA", "ordinal": 1},
            {"target": "recoil:function:0x401000", "ordinal": 0},
        ]
        with LIVE_CALL_CONTRACT_NORMALIZER_REGISTRY.record_body(
            "recoil:function:0x402000"
        ) as use:
            normalized = normalize_emitted_call_rows(rows)
        self.assertEqual(rows, normalized)
        self.assertIsNot(rows, normalized)
        self.assertEqual(
            ["recoil:call-contract:normalizer:emitted-call-rows:v2"],
            [row["normalizer_id"] for row in use.receipt()["normalizers"]],
        )


if __name__ == "__main__":
    unittest.main()
