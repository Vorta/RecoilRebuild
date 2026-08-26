from __future__ import annotations

from pathlib import Path
import contextlib
from dataclasses import replace
import io
import json
import sys
import unittest
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.lib.bn_dependency_frontier import Node  # noqa: E402
from _recoil.commands.provider_closure_audit import (  # noqa: E402
    audit_frontier_addresses,
    audit_node,
    audit_provider_owner_entries,
    main as provider_closure_main,
)
from _recoil.lib.binja import Symbol  # noqa: E402
from _recoil.lib.owner_entries import OwnerEntry, OwnerEntryIndex  # noqa: E402
from _recoil.lib.provider_closure import (  # noqa: E402
    audit_provider_owner_entry,
    classify_untracked_symbol,
)
from _recoil.lib.source_owners import SourceOwnerDocument  # noqa: E402
from tests.tools.owner_fixture import ledger_payload, owner_record  # noqa: E402


def provider_entry(
    *,
    address: str = "0x4c6000",
    recon: str = "✅",
    provider: str = "✅",
    kind: str = "VC5 compiler runtime",
    name: str = "MSVC_EH_ArrayConstructor",
    origin: str = "VC5 compiler runtime helper",
    file: str = "external",
    target: str = "msvc_array_constructor",
    group: str = "provider.compiler",
) -> OwnerEntry:
    record = owner_record(
        "provider.compiler",
        kind="provider-boundary",
        functions=(address,),
        name=name,
        section=group,
        source_paths=(),
        address_metadata={address: {"name": name, "target": target}},
    )
    record["origin"] = origin
    doc = SourceOwnerDocument(Path("SOURCE_OWNERS.json"), ledger_payload(record))
    entry = OwnerEntryIndex.from_source_owners(doc, binary="recoil").entries[address]
    return replace(
        entry,
        reconstructed_status=recon,
        provider_boundary_status=provider,
        provider_kind=kind,
        provider_origin=origin,
        provider_file=file,
    )


class RecoilProviderClosureTests(unittest.TestCase):
    def test_accepted_provider_entry_with_complete_metadata_closes(self) -> None:
        findings = audit_provider_owner_entry(provider_entry())

        self.assertEqual(["info"], [finding.severity for finding in findings])

    def test_accepted_provider_entry_with_non_external_file_is_error(self) -> None:
        findings = audit_provider_owner_entry(provider_entry(file="src/FakeProvider.cpp"))

        self.assertTrue(any(finding.severity == "error" for finding in findings))
        self.assertTrue(any("File: external" in finding.message for finding in findings))

    def test_accepted_provider_entry_missing_required_metadata_is_error(self) -> None:
        findings = audit_provider_owner_entry(provider_entry(kind="", name="", origin=""))

        self.assertTrue(any(finding.severity == "error" for finding in findings))
        self.assertTrue(any("missing" in finding.message for finding in findings))

    def test_limited_provider_entry_closes_with_warning(self) -> None:
        findings = audit_provider_owner_entry(provider_entry(recon="☑️"))

        self.assertTrue(any(finding.severity == "warning" for finding in findings))
        self.assertFalse(any(finding.severity == "error" for finding in findings))

    def test_bn_import_symbol_closes_without_owner_entry(self) -> None:
        finding = classify_untracked_symbol(
            address="0x500010",
            symbol=Symbol(address="0x500010", name="CreateWindowExA", kind="import"),
        )

        self.assertIsNotNone(finding)
        self.assertEqual("info", finding.severity)
        self.assertEqual("bn-provider-symbol", finding.closure_kind)

    def test_alloca_probe_closes_as_implicit_compiler_helper(self) -> None:
        finding = classify_untracked_symbol(
            address="0x4c6100",
            symbol=Symbol(address="0x4c6100", name="__alloca_probe", kind="function"),
        )

        self.assertIsNotNone(finding)
        self.assertEqual("info", finding.severity)
        self.assertEqual("implicit-compiler-helper", finding.closure_kind)

    def test_msvc_array_helpers_close_as_implicit_compiler_helpers(self) -> None:
        for name in ("MSVC_EH_ArrayConstructor", "MSVC_EH_ArrayDestructor_OrphanEpilogue"):
            with self.subTest(name=name):
                finding = classify_untracked_symbol(
                    address="0x4c6000",
                    symbol=Symbol(address="0x4c6000", name=name, kind="function"),
                )

                self.assertIsNotNone(finding)
                self.assertEqual("info", finding.severity)

    def test_generic_untracked_msvc_symbol_is_strict_error(self) -> None:
        finding = classify_untracked_symbol(
            address="0x4c9000",
            symbol=Symbol(address="0x4c9000", name="MSVC_GenericThunk", kind="function"),
        )

        self.assertIsNotNone(finding)
        self.assertEqual("error", finding.severity)

    def test_forwarder_to_provider_target_delegates_to_target_entry(self) -> None:
        target = provider_entry(address="0x401020", name="ProviderTarget")
        node = Node(
            address="0x401000",
            name="ProviderTarget_Forwarder",
            kind="forwarder",
            forwarded_to="0x401020",
            forwarded_entry=target,
        )

        findings = audit_node(node)

        self.assertEqual("forwarder-target", findings[0].closure_kind)
        self.assertTrue(any(finding.address == "0x401020" for finding in findings))
        self.assertFalse(any(finding.severity == "error" for finding in findings))

    def test_untracked_forwarder_needs_classification(self) -> None:
        findings = audit_node(Node(address="0x401000", name="Sample_Forwarder", kind="function"))

        self.assertEqual(["error"], [finding.severity for finding in findings])
        self.assertEqual("forwarder-unclassified", findings[0].closure_kind)

    def test_owners_only_result_is_strict_failure_on_provider_errors(self) -> None:
        result = audit_provider_owner_entries(
            binary="recoil",
            entries={"0x4c6000": provider_entry(file="src/FakeProvider.cpp")},
        )

        self.assertTrue(result.strict_failure)
        self.assertEqual(1, result.checked_provider_rows)

    def test_frontier_mode_reports_untracked_generated_symbol_error(self) -> None:
        class FakeBridge:
            pass

        def fake_frontier(address: str, depth: int, bridge: object, entries: dict[str, OwnerEntry]):
            return {
                address: Node(address=address, name="Root", entry=entries[address]),
                "0x4c9000": Node(address="0x4c9000", name="MSVC_GenericThunk", kind="compiler"),
            }

        root = OwnerEntry(
            address="0x401000",
            reconstructed_status="✅",
            source_dependencies_status="✅",
            reimplemented_status="✅",
            reimplemented_name="Root",
        )
        with mock.patch("_recoil.commands.provider_closure_audit.build_frontier", side_effect=fake_frontier):
            result = audit_frontier_addresses(
                binary="recoil",
                entries={"0x401000": root},
                bridge=FakeBridge(),  # type: ignore[arg-type]
                addresses=["0x401000"],
                depth=1,
            )

        self.assertTrue(result.strict_failure)
        self.assertEqual(1, result.checked_dependencies)
        self.assertTrue(any(finding.address == "0x4c9000" for finding in result.findings))

    def test_cli_requires_owners_only_or_address(self) -> None:
        rc = provider_closure_main([])

        self.assertEqual(2, rc)

    def test_cli_owners_only_json_strict_reports_provider_entry(self) -> None:
        class FakeOwnerEntries:
            entries = {"0x4c6000": provider_entry()}

        stdout = io.StringIO()
        stderr = io.StringIO()
        with mock.patch(
            "_recoil.commands.provider_closure_audit.OwnerEntryIndex.load",
            return_value=FakeOwnerEntries(),
        ), contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            rc = provider_closure_main(["--owners-only", "--json", "--strict"])

        self.assertEqual(0, rc, stderr.getvalue())
        payload = json.loads(stdout.getvalue())
        self.assertEqual("owners-only", payload["mode"])
        self.assertEqual(1, payload["checked_provider_rows"])
        self.assertEqual(0, payload["error_count"])


if __name__ == "__main__":
    unittest.main()
