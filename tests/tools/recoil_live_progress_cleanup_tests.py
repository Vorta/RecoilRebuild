from __future__ import annotations

import argparse
from contextlib import redirect_stderr, redirect_stdout
from copy import deepcopy
import io
import json
from pathlib import Path
from types import SimpleNamespace
import tempfile
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
import sys

if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands import progress_cli  # noqa: E402
from _recoil.commands.workspace_issues import (  # noqa: E402
    empty_ledger as empty_issue_ledger,
    normalize_entry as normalize_issue_entry,
    validate_issue_document,
)
from _recoil.lib.issue_sqlite import (  # noqa: E402
    create_issue_database,
)
from _recoil.lib.progress import (  # noqa: E402
    AUTHORED_BYTE_DIMENSIONS,
    AUTHORED_ORDER_DIMENSIONS,
    EXACT_LINK_DIMENSIONS,
    FULL_ORDER_DIMENSIONS,
    ProgressDocument,
    ProgressError,
    empty_progress_document,
    normalize_resource_claims,
    state_record,
)


PRIMARY_FIXTURE_SOURCE = "src/Battlesport/ai_net.cpp"
SECONDARY_FIXTURE_SOURCE = "src/Battlesport/Briefing.cpp"
TERTIARY_FIXTURE_SOURCE = "src/Battlesport/RecoilApp.cpp"
THIRD_TARGET_FIXTURE_SOURCE = "src/Battlesport/map.cpp"
MULTI_TU_PRIMARY_SOURCE = "src/Battlesport/WOL.cpp"
MULTI_TU_INSERTED_SOURCE = "src/Battlesport/WinSock.cpp"
HUD_FIXTURE_SOURCE = "src/Battlesport/hud.cpp"
TRACKED_FIXTURE_SOURCES = (
    PRIMARY_FIXTURE_SOURCE,
    SECONDARY_FIXTURE_SOURCE,
    TERTIARY_FIXTURE_SOURCE,
    THIRD_TARGET_FIXTURE_SOURCE,
    MULTI_TU_PRIMARY_SOURCE,
    MULTI_TU_INSERTED_SOURCE,
    HUD_FIXTURE_SOURCE,
)


def git_inventory_aware_run(verifier_result: SimpleNamespace):
    """Model authenticated Git inventory calls before one mocked verifier run."""

    inventory = b"".join(
        path.encode("utf-8") + b"\0" for path in TRACKED_FIXTURE_SOURCES
    )

    def run(command, *_args, **_kwargs):
        git_prefix = ["git", "-C", str(REPO_ROOT.resolve())]
        if command == git_prefix + ["rev-parse", "--show-toplevel"]:
            return SimpleNamespace(
                returncode=0,
                stdout=(str(REPO_ROOT.resolve()) + "\n").encode("utf-8"),
                stderr=b"",
            )
        if command == git_prefix + ["ls-files", "-z"]:
            return SimpleNamespace(returncode=0, stdout=inventory, stderr=b"")
        if command[:1] == ["git"]:
            raise AssertionError(f"unexpected Git inventory command: {command!r}")
        return verifier_result

    return run


def pending(names):
    return {
        name: state_record("pending", "observed", "historical", [], gating=False)
        for name in names
    }


def accepted(names, evidence_id="recoil:evidence:r0:000001"):
    return {
        name: state_record(
            "passed",
            "accepted",
            "current",
            [evidence_id],
            gating=True,
            validation_mode="live",
        )
        for name in names
    }


def fixture_document(*, linked_phase: bool = False) -> dict:
    data = empty_progress_document()
    data["binaries"] = {
        "recoil": {
            "binary": "recoil",
            "text": {"start": "0x401000", "end_exclusive": "0x401020"},
        }
    }
    evidence_id = "recoil:evidence:r0:000001"
    data["evidence"][evidence_id] = {
        "kind": "fixture-live-observation",
        "summary": "fixture",
        "scope_ids": ["recoil:block:0x401000"],
        "result": "passed",
        "disposition": "accepted",
        "freshness": "current",
        "gating": True,
        "validation_mode": "live",
        "artifacts": [],
        "provenance": {},
    }
    data["physical_blocks"] = {
        "recoil:block:0x401000": {
            "binary": "recoil",
            "start": "0x401000",
            "end_exclusive": "0x401020",
            "source_path": PRIMARY_FIXTURE_SOURCE,
            "agent_source_path": PRIMARY_FIXTURE_SOURCE,
            "contribution_ids": [
                "recoil:function:0x401000",
                "recoil:function:0x401010",
            ],
            "order_targets": {
                "object": "sample_authored_order",
                "linked": "sample_full_order",
            },
            "order": {
                "authored": (
                    accepted(AUTHORED_ORDER_DIMENSIONS)
                    if linked_phase
                    else pending(AUTHORED_ORDER_DIMENSIONS)
                ),
                "full": (
                    accepted(FULL_ORDER_DIMENSIONS)
                    if linked_phase
                    else pending(FULL_ORDER_DIMENSIONS)
                ),
            },
        }
    }
    authored_state = (
        accepted(AUTHORED_BYTE_DIMENSIONS)
        if linked_phase
        else pending(AUTHORED_BYTE_DIMENSIONS)
    )
    data["symbols"] = {
        "recoil:function:0x401000": {
            "binary": "recoil",
            "kind": "function",
            "address": "0x401000",
            "end_exclusive": "0x401010",
            "pipeline_class": "authored",
            "physical_block_id": "recoil:block:0x401000",
            "binary_state": {**deepcopy(authored_state), **pending(EXACT_LINK_DIMENSIONS)},
        },
        "recoil:function:0x401010": {
            "binary": "recoil",
            "kind": "provider-function",
            "address": "0x401010",
            "end_exclusive": "0x401020",
            "pipeline_class": "non-authored",
            "physical_block_id": "recoil:block:0x401000",
            "binary_state": {**deepcopy(authored_state), **pending(EXACT_LINK_DIMENSIONS)},
        },
    }
    data["verification_targets"] = {
        "recoil:vc5-target:sample_authored_order": {
            "binary": "recoil",
            "kind": "vc5",
            "name": "sample_authored_order",
            "registration": {
                "binary": "recoil",
                "name": "sample_authored_order",
                "translation_unit_function_order": [
                    {
                        "functions": [
                            {
                                "address": "0x401000",
                                "symbol": "?Authored@@YAXXZ",
                                "pipeline_class": "authored",
                                "authored_order_role": "authored-body",
                            },
                            {
                                "address": "0x401010",
                                "symbol": "?Provider@@YAXXZ",
                                "pipeline_class": "non-authored",
                                "authored_order_role": "non-authored",
                            },
                        ]
                    }
                ],
            },
        },
        "recoil:vc5-target:sample_full_order": {
            "binary": "recoil",
            "kind": "vc5",
            "name": "sample_full_order",
            "registration": {
                "binary": "recoil",
                "name": "sample_full_order",
                "linked_function_intervals": [
                    {
                        "functions": [
                            {"address": "0x401000", "symbol": "?Authored@@YAXXZ"},
                            {"address": "0x401010", "symbol": "?Provider@@YAXXZ"},
                        ]
                    }
                ],
            },
        },
    }
    return data


def multi_block_order_fixture() -> dict:
    data = fixture_document()
    data["binaries"]["recoil"]["text"]["end_exclusive"] = "0x401040"
    first = data["physical_blocks"]["recoil:block:0x401000"]
    first["contribution_ids"] = ["recoil:function:0x401000"]
    first["end_exclusive"] = "0x401010"
    fixture_sources = {
        "0x401010": SECONDARY_FIXTURE_SOURCE,
        "0x401020": TERTIARY_FIXTURE_SOURCE,
    }
    for address, end in (("0x401010", "0x401020"), ("0x401020", "0x401040")):
        block_id = f"recoil:block:{address}"
        symbol_id = f"recoil:function:{address}"
        data["physical_blocks"][block_id] = {
            "binary": "recoil",
            "start": address,
            "end_exclusive": end,
            "source_path": fixture_sources[address],
            "agent_source_path": fixture_sources[address],
            "contribution_ids": [symbol_id],
            "order_targets": {
                "object": "sample_authored_order",
                "linked": "sample_full_order",
            },
            "order": {
                "authored": pending(AUTHORED_ORDER_DIMENSIONS),
                "full": pending(FULL_ORDER_DIMENSIONS),
            },
        }
        data["symbols"][symbol_id] = {
            "binary": "recoil",
            "kind": "function",
            "address": address,
            "end_exclusive": end,
            "pipeline_class": "authored",
            "physical_block_id": block_id,
            "binary_state": {
                **pending(AUTHORED_BYTE_DIMENSIONS),
                **pending(EXACT_LINK_DIMENSIONS),
            },
        }
    functions = [
        {
            "address": address,
            "symbol": f"?Authored{index}@@YAXXZ",
            "authored_order_role": "authored-body",
        }
        for index, address in enumerate(("0x401000", "0x401010", "0x401020"), 1)
    ]
    target = data["verification_targets"]["recoil:vc5-target:sample_authored_order"]
    target["registration"]["translation_unit_function_order"] = [
        {"source_from": PRIMARY_FIXTURE_SOURCE, "functions": functions}
    ]
    return data


def multi_tu_retail_contiguous_order_fixture() -> dict:
    data = multi_block_order_fixture()
    target = data["verification_targets"][
        "recoil:vc5-target:sample_authored_order"
    ]
    functions = target["registration"]["translation_unit_function_order"][0][
        "functions"
    ]
    target["registration"]["translation_unit_function_order"] = [
        {
            "source_from": MULTI_TU_PRIMARY_SOURCE,
            "order_scope": "authored",
            "functions": [functions[0], functions[2]],
        },
        {
            "source_from": MULTI_TU_INSERTED_SOURCE,
            "order_scope": "authored",
            "functions": [functions[1]],
        },
    ]
    return data


def parallel_lane_fixture(*, overlapping_byte_sources: bool = False) -> dict:
    data = multi_block_order_fixture()
    first_id = "recoil:block:0x401000"
    second_id = "recoil:block:0x401010"
    third_id = "recoil:block:0x401020"
    data["physical_blocks"][first_id]["order"]["authored"] = accepted(
        AUTHORED_ORDER_DIMENSIONS
    )
    data["physical_blocks"][second_id]["order"]["authored"] = accepted(
        AUTHORED_ORDER_DIMENSIONS
    )
    data["symbols"]["recoil:function:0x401000"]["binary_state"]["object_byte"] = accepted(
        ("object_byte",)
    )["object_byte"]
    if overlapping_byte_sources:
        data["physical_blocks"][second_id]["agent_source_path"] = (
            PRIMARY_FIXTURE_SOURCE
        )
        data["physical_blocks"][second_id]["source_path"] = PRIMARY_FIXTURE_SOURCE
    data["physical_blocks"][third_id]["order_targets"]["object"] = "third_order"
    data["verification_targets"]["recoil:vc5-target:third_order"] = {
        "binary": "recoil",
        "kind": "vc5",
        "name": "third_order",
        "registration": {
            "binary": "recoil",
            "name": "third_order",
            "source_from": THIRD_TARGET_FIXTURE_SOURCE,
            "translation_unit_function_order": [
                {
                    "source_from": THIRD_TARGET_FIXTURE_SOURCE,
                    "order_scope": "authored",
                    "functions": [
                        {
                            "address": "0x401020",
                            "symbol": "?Authored3@@YAXXZ",
                            "pipeline_class": "authored",
                            "authored_order_role": "authored-body",
                        }
                    ],
                }
            ],
        },
    }
    return data


def unresolved_primary_order_fixture() -> dict:
    data = parallel_lane_fixture()
    row = data["verification_targets"]["recoil:vc5-target:third_order"][
        "registration"
    ]["translation_unit_function_order"][0]["functions"][0]
    row.update(
        {
            "name": "UnresolvedPrimaryRow",
            "pipeline_class": "unresolved",
            "authored_order_role": "unresolved",
            "authored_order_gate": False,
            "authored_relative_order_gate": False,
        }
    )
    return data


def ready_byte_preflight(_document, *, lane, cursor, **_kwargs):
    return {
        "passed": True,
        "reason_code": "live-byte-preflight-ready",
        "reason": "fixture byte preflight ready",
        "target_ids": [],
        "source_paths": [],
        "binding_count": 1,
    }


CLOSURE_SOURCE_ID = "recoil:function:0x401180"
CLOSURE_SOURCE_SYMBOL = "?ClosureSource@@YAXXZ"
CLOSURE_TARGET_ID = "recoil:data:0x4cc814"
CLOSURE_VC5_ID = "recoil:vc5-target:ainet_text_block_order"
CLOSURE_MANIFEST = "tools/vc5_verify_targets/ainet_text_block_order.json"


def focused_binding_closure_fixture() -> dict:
    data = empty_progress_document()
    data["symbols"] = {
        CLOSURE_SOURCE_ID: {
            "binary": "recoil",
            "kind": "function",
            "address": "0x401180",
            "end_exclusive": "0x401420",
            "pipeline_class": "authored",
            "verification_target_ids": [CLOSURE_VC5_ID],
        },
        CLOSURE_TARGET_ID: {
            "binary": "recoil",
            "kind": "data",
            "address": "0x4cc814",
            "end_exclusive": "0x4cc818",
            "navigation_name": "g_ClosureLiteral",
            "output_section_id": "recoil:section:.rdata",
            "ownership_state": "primary-owned",
            "pipeline_class": None,
            "verification_target_ids": [],
            "relocation_target_binding": {
                "reviewed": True,
                "object_symbol": "$TClosure",
                "reason": "unit-test focused source registration closure",
                "evidence_ids": ["unit:evidence:closure-owner"],
                "binding_context": {
                    "source_binding": {
                        "symbol_id": CLOSURE_SOURCE_ID,
                        "address": "0x401180",
                        "end_exclusive": "0x401420",
                        "object_symbol": CLOSURE_SOURCE_SYMBOL,
                        "physical_pipeline_class": "authored",
                        "object_pipeline_class": "authored",
                        "registration_ids": [
                            "vc5:ainet_text_block_order:source:src/Battlesport/ai_net.cpp"
                        ],
                        "evidence_ids": [],
                    },
                    "relocation": {
                        "offset": 1,
                        "type": 6,
                        "type_name": "DIR32",
                        "retail_target": 0x4CC814,
                        "instruction_offset": 0,
                        "opcode": "d8",
                    },
                    "target": {
                        "symbol_id": CLOSURE_TARGET_ID,
                        "address": "0x4cc814",
                        "end_exclusive": "0x4cc818",
                        "kind": "data",
                        "navigation_name": "g_ClosureLiteral",
                        "object_symbol": "$TClosure",
                        "output_section_id": "recoil:section:.rdata",
                        "pipeline_class": None,
                        "ownership_state": "primary-owned",
                    },
                    "owner": {
                        "owner_id": "recoil:owner:unit.closure",
                        "kind": "subsystem",
                        "provider_state": "pending",
                        "lifecycle_state": "pending",
                        "binding_evidence_ids": ["unit:evidence:closure-owner"],
                    },
                    "relationship": {
                        "kind": "primary-data",
                        "address": "0x4cc814",
                        "symbol_id": CLOSURE_TARGET_ID,
                        "name": "g_ClosureLiteral",
                    },
                    "creation_mode": "created-data-symbol",
                },
            },
        },
    }
    data["verification_targets"] = {
        CLOSURE_VC5_ID: {
            "binary": "recoil",
            "kind": "vc5",
            "name": "ainet_text_block_order",
            "registration": {
                "binary": "recoil",
                "name": "ainet_text_block_order",
                "manifest_path": CLOSURE_MANIFEST,
            },
        }
    }
    return data


def focused_closure_manifest(
    *,
    symbols: tuple[str, ...] = (CLOSURE_SOURCE_SYMBOL,),
    duplicate_source: bool = False,
) -> SimpleNamespace:
    functions = tuple(
        SimpleNamespace(
            address="0x401180",
            symbol=symbol,
            logical_identity_key="",
        )
        for symbol in symbols
    )
    translation_entries = (
        (
            SimpleNamespace(
                source_from="src/Battlesport/closure_alias.cpp",
                functions=(
                    SimpleNamespace(
                        address="0x401180",
                        symbol=CLOSURE_SOURCE_SYMBOL,
                        logical_identity_key="",
                    ),
                ),
            ),
        )
        if duplicate_source
        else ()
    )
    return SimpleNamespace(
        name="ainet_text_block_order",
        source_from="src/Battlesport/ai_net.cpp",
        functions=functions,
        data_symbols=(),
        translation_unit_function_order=translation_entries,
        linked_function_intervals=(),
    )


def focused_current_registration(data: dict) -> dict:
    current = deepcopy(data["verification_targets"][CLOSURE_VC5_ID])
    current["registered_addresses"] = ["0x401180"]
    return current


def external_fold_alias_fixture(*, fold_status: str = "proven-fold-alias") -> dict:
    data = fixture_document()
    data["binaries"]["recoil"]["text"]["end_exclusive"] = "0x401030"
    first = data["physical_blocks"]["recoil:block:0x401000"]
    first["end_exclusive"] = "0x401010"
    first["contribution_ids"] = ["recoil:function:0x401000"]
    first["order"]["authored"] = accepted(AUTHORED_ORDER_DIMENSIONS)
    data["symbols"].pop("recoil:function:0x401010")
    data["physical_blocks"]["recoil:block:0x401010"] = {
        "binary": "recoil",
        "start": "0x401010",
        "end_exclusive": "0x401030",
        "source_path": HUD_FIXTURE_SOURCE,
        "agent_source_path": HUD_FIXTURE_SOURCE,
        "contribution_ids": [
            "recoil:function:0x401010",
            "recoil:function:0x401020",
        ],
        "order_targets": {"object": "", "linked": ""},
        "order": {
            "authored": pending(AUTHORED_ORDER_DIMENSIONS),
            "full": pending(FULL_ORDER_DIMENSIONS),
        },
    }
    for address, end in (("0x401010", "0x401020"), ("0x401020", "0x401030")):
        data["symbols"][f"recoil:function:{address}"] = {
            "binary": "recoil",
            "kind": "function",
            "address": address,
            "end_exclusive": end,
            "pipeline_class": "authored",
            "authored_order_role": "authored-body",
            "physical_block_id": "recoil:block:0x401010",
            "binary_state": {
                **pending(AUTHORED_BYTE_DIMENSIONS),
                **pending(EXACT_LINK_DIMENSIONS),
            },
        }
    alias = {
        "address": "0x401000",
        "symbol": "?FoldedAlias@@YAXXZ",
        "name": "FoldedAlias",
        "pipeline_class": "authored",
        "authored_order_role": "authored-body",
        "authored_order_gate": True,
        "authored_relative_order_gate": fold_status != "proven-fold-alias",
        "required_presence": True,
        "full_order_gate": False,
        "logical_identity_key": "recoil:logical-function:0x401000:folded-alias",
        "icf_fold_status": fold_status,
    }
    data["verification_targets"]["recoil:vc5-target:hud_order"] = {
        "binary": "recoil",
        "kind": "vc5",
        "name": "hud_order",
        "registration": {
            "binary": "recoil",
            "name": "hud_order",
            "translation_unit_function_order": [
                {
                    "order_scope": "authored",
                    "functions": [
                        {
                            "address": "0x401010",
                            "symbol": "?FirstHud@@YAXXZ",
                            "pipeline_class": "authored",
                            "authored_order_role": "authored-body",
                        },
                        alias,
                        {
                            "address": "0x401020",
                            "symbol": "?SecondHud@@YAXXZ",
                            "pipeline_class": "authored",
                            "authored_order_role": "authored-body",
                        },
                    ],
                }
            ],
        },
    }
    return data


def write_tracker(path: Path, data: dict) -> None:
    path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")


def write_issue_ledger(
    path: Path,
    *,
    resource_claims: list[dict[str, str]] | None = None,
    active: bool = True,
) -> Path:
    document = empty_issue_ledger()
    claims = normalize_resource_claims(resource_claims or [])
    if claims:
        issue_id = "WSI-20260816-999"
        packet_id = "issue:work:test-live-progress-conflict"
        reservation_id = f"{packet_id}:attempt:1"
        timestamp = "2026-08-16T00:00:00Z"
        document["issues"] = [
            normalize_issue_entry(
                {
                    "severity": "medium",
                    "summary": "synthetic live progress conflict",
                    "area": "tools",
                    "impact": "scheduler conflict coverage requires an exact active lease",
                    "next_action": "exercise deterministic cross-ledger scheduling",
                    "actual": "a synthetic issue reservation owns the selected resource",
                    "expected": "only an exact active overlap blocks its lane",
                    "repro": "run recoil_live_progress_cleanup_tests",
                    "commands": [
                        "python -m unittest tests.tools.recoil_live_progress_cleanup_tests"
                    ],
                    "files": ["tests/tools/recoil_live_progress_cleanup_tests.py"],
                    "tags": ["scheduler", "issue-ledger"],
                },
                issue_id=issue_id,
                kind="tool-error",
            )
        ]
        document["work_packets"] = [
            {
                "id": packet_id,
                "issue_id": issue_id,
                "state": "active" if active else "closed",
                "handoff_role": "recoil_tool_maintainer",
                "scope": "exercise deterministic cross-ledger scheduling",
                "next_command": (
                    "python -m unittest "
                    "tests.tools.recoil_live_progress_cleanup_tests"
                ),
                "allowed_paths": [
                    "tests/tools/recoil_live_progress_cleanup_tests.py"
                ],
                "forbidden_paths": ["src"],
                "validation_commands": [
                    "python -m unittest "
                    "tests.tools.recoil_live_progress_cleanup_tests"
                ],
                "required_return_fields": ["changed_paths"],
                "resource_claims": claims,
                "reservation_id": reservation_id if active else None,
                "outcome": None if active else "returned",
                "created": timestamp,
                "updated": timestamp,
                "semantic_contract_version": 1,
                "scope_versions": [],
                "role_contract_version": 1,
            }
        ]
        document["reservations"] = [
            {
                "id": reservation_id,
                "packet_id": packet_id,
                "state": "active" if active else "released",
                "created": timestamp,
                "released": None if active else timestamp,
                "outcome": None if active else "returned",
                "evidence_ids": ["transcript:test:live-progress-conflict"],
                "resource_claims": claims,
                "expires": None,
                "semantic_contract_version": 1,
                **({
                    "git_workspace_baseline": {
                        "schema": "recoil-git-workspace-baseline-v2",
                        "packet_id": packet_id,
                        "baseline_commit": "opaque-test-commit",
                        "branch": "test-packet",
                        "writable_paths": [
                            "tests/tools/recoil_live_progress_cleanup_tests.py"
                        ],
                        "status_porcelain_v2": [],
                        "ignored_paths": [],
                        "git_object_ids_are_opaque": True,
                    },
                } if active else {}),
            }
        ]
    validate_issue_document(document)
    create_issue_database(
        path,
        document,
        cutover_pair_id="pair:test:live-progress-cleanup",
    )
    return path


class LiveProgressCleanupTests(unittest.TestCase):
    def test_git_inventory_aware_run_rejects_unexpected_git_command(self) -> None:
        verifier = SimpleNamespace(returncode=0, stdout="verifier", stderr="")
        run = git_inventory_aware_run(verifier)

        with self.assertRaisesRegex(AssertionError, "unexpected Git inventory command"):
            run(["git", "-C", str(REPO_ROOT.resolve()), "status"])
        with self.assertRaisesRegex(AssertionError, "unexpected Git inventory command"):
            run(
                [
                    "git",
                    "-C",
                    str(REPO_ROOT.resolve().parent),
                    "ls-files",
                    "-z",
                ]
            )
        self.assertIs(verifier, run(["python", "mock-verifier.py"]))

    def setUp(self) -> None:
        self.issue_temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.issue_temporary.cleanup)
        self.issue_ledger = write_issue_ledger(
            Path(self.issue_temporary.name) / "issues.sqlite3"
        )

    def test_focused_binding_expands_exact_registered_source_closure(self) -> None:
        data = focused_binding_closure_fixture()
        current_registration = focused_current_registration(data)
        with (
            patch(
                "_recoil.lib.verification_targets.vc5_target_registration",
                return_value=(CLOSURE_VC5_ID, current_registration),
            ),
            patch(
                "_recoil.commands.vc5_verify.load_manifest",
                return_value=focused_closure_manifest(),
            ),
        ):
            bindings = progress_cli._focused_live_byte_bindings(
                ProgressDocument(data),
                scope_ids=[],
                target_addresses=["0x4cc814"],
            )
        self.assertEqual([CLOSURE_SOURCE_ID], sorted(bindings))
        self.assertEqual(
            [CLOSURE_SOURCE_SYMBOL],
            [item.function.symbol for item in bindings[CLOSURE_SOURCE_ID]],
        )
        self.assertEqual(
            [], data["symbols"][CLOSURE_TARGET_ID]["verification_target_ids"]
        )

    def test_focused_binding_source_closure_fails_closed(self) -> None:
        def run(
            data: dict,
            *,
            target: SimpleNamespace | None = None,
            current_registration: dict | None = None,
        ) -> None:
            current_registration = current_registration or focused_current_registration(data)
            with (
                patch(
                    "_recoil.lib.verification_targets.vc5_target_registration",
                    return_value=(CLOSURE_VC5_ID, current_registration),
                ),
                patch(
                    "_recoil.commands.vc5_verify.load_manifest",
                    return_value=target or focused_closure_manifest(),
                ),
            ):
                progress_cli._focused_live_byte_bindings(
                    ProgressDocument(data),
                    scope_ids=[],
                    target_addresses=["0x4cc814"],
                )

        cases: list[tuple[str, dict, SimpleNamespace | None, dict | None]] = []

        absent_snapshot = focused_binding_closure_fixture()
        del absent_snapshot["symbols"][CLOSURE_TARGET_ID]["relocation_target_binding"][
            "binding_context"
        ]["source_binding"]
        cases.append(("absent-snapshot", absent_snapshot, None, None))

        absent_source = focused_binding_closure_fixture()
        absent_source["symbols"].pop(CLOSURE_SOURCE_ID)
        cases.append(("absent-source", absent_source, None, None))

        stale_snapshot = focused_binding_closure_fixture()
        stale_snapshot["symbols"][CLOSURE_TARGET_ID]["relocation_target_binding"][
            "binding_context"
        ]["source_binding"]["registration_ids"] = ["vc5:stale"]
        cases.append(("stale-snapshot", stale_snapshot, None, None))

        wrong_symbol = focused_binding_closure_fixture()
        cases.append(
            (
                "wrong-symbol",
                wrong_symbol,
                focused_closure_manifest(symbols=("?WrongSource@@YAXXZ",)),
                None,
            )
        )

        conflicting = focused_binding_closure_fixture()
        cases.append(
            (
                "conflicting",
                conflicting,
                focused_closure_manifest(
                    symbols=(CLOSURE_SOURCE_SYMBOL, "?ConflictingSource@@YAXXZ")
                ),
                None,
            )
        )

        ambiguous = focused_binding_closure_fixture()
        cases.append(
            (
                "ambiguous",
                ambiguous,
                focused_closure_manifest(duplicate_source=True),
                None,
            )
        )

        unregistered = focused_binding_closure_fixture()
        unregistered["symbols"][CLOSURE_SOURCE_ID]["verification_target_ids"] = [
            "recoil:vc5-target:missing"
        ]
        cases.append(("unregistered", unregistered, None, None))

        stale_registration = focused_binding_closure_fixture()
        current_registration = focused_current_registration(stale_registration)
        stale_registration["verification_targets"][CLOSURE_VC5_ID]["name"] = "stale-name"
        cases.append(
            (
                "stale-registration",
                stale_registration,
                None,
                current_registration,
            )
        )

        for name, data, target, current_registration in cases:
            with self.subTest(name=name), self.assertRaises(ProgressError):
                run(
                    data,
                    target=target,
                    current_registration=current_registration,
                )

    def test_final_build_order_helpers_require_current_live_acceptance(self) -> None:
        block = fixture_document(linked_phase=True)["physical_blocks"]["recoil:block:0x401000"]
        self.assertTrue(ProgressDocument._block_authored_order_accepted(block))
        self.assertTrue(ProgressDocument._block_full_order_accepted(block))
        stale = deepcopy(block)
        stale["order"]["authored"][AUTHORED_ORDER_DIMENSIONS[0]]["freshness"] = "historical"
        stale["order"]["full"][FULL_ORDER_DIMENSIONS[0]]["validation_mode"] = "historical-observation"
        self.assertFalse(ProgressDocument._block_authored_order_accepted(stale))
        self.assertFalse(ProgressDocument._block_full_order_accepted(stale))

    def test_scheduler_routes_current_order_through_atomic_live_advance(self):
        state = ProgressDocument(fixture_document()).next_work(
            "recoil", issue_ledger=self.issue_ledger
        )
        self.assertEqual(state["cursor"], "0x401000")
        self.assertEqual(state["phase"], "authored-function-order")
        self.assertIn("progress advance-live-order", state["next_command"])
        self.assertIn("recoil:vc5-target:sample_authored_order", state["next_command"])
        self.assertIn("--expected-revision 0", state["next_command"])

    def test_scheduler_uses_one_exact_registered_target_when_block_binding_is_empty(self):
        fixture = fixture_document()
        fixture["physical_blocks"]["recoil:block:0x401000"]["order_targets"]["object"] = ""
        fixture["verification_targets"]["recoil:vc5-target:sample_authored_order"][
            "registration"
        ]["translation_unit_function_order"][0]["order_scope"] = "authored"
        with patch.object(
            progress_cli,
            "_registered_order_interval",
            side_effect=lambda target: (
                ("0x401000", "0x401020")
                if target.get("name") == "sample_authored_order"
                else None
            ),
        ):
            state = ProgressDocument(fixture).next_work(
                "recoil", issue_ledger=self.issue_ledger
            )

        self.assertEqual("launchable", state["cursor_launchability"]["primary"]["launchability"])
        self.assertIn("--object-target recoil:vc5-target:sample_authored_order", state["next_command"])
        self.assertEqual(1, len(state["launch_plan"][0]["actions"]))

    def test_scheduler_reports_typed_blocker_instead_of_launchable_empty_command(self):
        fixture = fixture_document()
        fixture["physical_blocks"]["recoil:block:0x401000"]["order_targets"]["object"] = ""
        first = fixture["verification_targets"]["recoil:vc5-target:sample_authored_order"]
        first["registration"]["translation_unit_function_order"][0]["order_scope"] = "authored"
        second = deepcopy(first)
        second["name"] = "second_authored_order"
        second["registration"]["name"] = "second_authored_order"
        fixture["verification_targets"]["recoil:vc5-target:second_authored_order"] = second
        with patch.object(
            progress_cli,
            "_registered_order_interval",
            side_effect=lambda target: (
                ("0x401000", "0x401020")
                if target.get("name") in {"sample_authored_order", "second_authored_order"}
                else None
            ),
        ):
            state = ProgressDocument(fixture).next_work(
                "recoil", issue_ledger=self.issue_ledger
            )

        primary = state["cursor_launchability"]["primary"]
        self.assertEqual("blocked", primary["launchability"])
        self.assertEqual("order-target-ambiguous", primary["reason_code"])
        self.assertEqual("", state["next_command"])
        self.assertEqual([], state["launch_plan"][0]["actions"])
        self.assertFalse(state["launch_plan"][0]["selected_opportunity"])

    def test_tracker_contract_uses_authored_relative_fold_predicate(self):
        fixture = fixture_document()
        rows = fixture["verification_targets"]["recoil:vc5-target:sample_authored_order"][
            "registration"
        ]["translation_unit_function_order"][0]["functions"]
        rows[0].update(
            {
                "pipeline_class": "authored",
                "authored_order_role": "authored-body",
            }
        )
        cofolded = {
            **rows[0],
            "logical_identity_key": "recoil:logical-function:0x401000:cofolded",
            "icf_fold_status": "proven-fold-alias",
            "full_order_gate": False,
        }
        selected = {
            **rows[0],
            "logical_identity_key": "recoil:logical-function:0x401000:selected",
            "icf_fold_status": "selected-winner",
            "full_order_gate": False,
        }
        rows[1:1] = [cofolded, selected]

        contract = progress_cli._current_order_contract(
            ProgressDocument(fixture), "recoil:vc5-target:sample_authored_order"
        )

        self.assertEqual(
            [
                "recoil:function:0x401000",
                "recoil:logical-function:0x401000:selected",
            ],
            contract["identities"],
        )
        self.assertEqual(
            ["recoil:logical-function:0x401000:cofolded"],
            contract["required_inventory_identities"],
        )

    def test_proven_fold_alias_representative_does_not_define_target_block_coverage(self):
        fixture = external_fold_alias_fixture()
        with patch.object(
            progress_cli,
            "_registered_order_interval",
            side_effect=lambda target: (
                ("0x401010", "0x401030") if target.get("name") == "hud_order" else None
            ),
        ):
            document = ProgressDocument(fixture)
            state = document.next_work("recoil", issue_ledger=self.issue_ledger)
            contract = progress_cli._current_order_contract(
                document,
                "recoil:vc5-target:hud_order",
                override_selector="recoil:vc5-target:hud_order",
            )

        self.assertEqual(["recoil:block:0x401010"], contract["covered_block_ids"])
        self.assertEqual(
            ["recoil:function:0x401010", "recoil:function:0x401020"],
            contract["identities"],
        )
        self.assertEqual(
            ["recoil:logical-function:0x401000:folded-alias"],
            contract["required_inventory_identities"],
        )
        self.assertEqual("launchable", state["cursor_launchability"]["primary"]["launchability"])
        self.assertIn("--object-target recoil:vc5-target:hud_order", state["next_command"])

    def test_selected_winner_at_external_address_still_blocks_noncontiguous_coverage(self):
        fixture = external_fold_alias_fixture(fold_status="selected-winner")
        document = ProgressDocument(fixture)
        with patch.object(
            progress_cli,
            "_registered_order_interval",
            return_value=("0x401010", "0x401030"),
        ):
            with self.assertRaisesRegex(Exception, "re-enters physical block"):
                progress_cli._target_order_contract(
                    document,
                    "recoil:vc5-target:hud_order",
                    require_explicit_interval=True,
                )

    def test_order_pass_commits_block_with_revision_scoped_evidence(self):
        result = {
            "kind": "vc5-order-live-result",
            "target_id": "sample_authored_order",
            "phase": "authored-function-order",
            "physical_block_id": "recoil:block:0x401000",
            "passed": True,
            "expected_sequence": ["recoil:function:0x401000"],
            "candidate_sequence": ["recoil:function:0x401000"],
            "matched_prefix_count": 1,
            "first_divergence": None,
        }
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, fixture_document())
            build_root = REPO_ROOT / "build" / "live-validation" / "test-order-pass"
            if build_root.exists():
                self.skipTest("test build root unexpectedly exists")
            args = argparse.Namespace(
                progress=tracker,
                target="recoil:vc5-target:sample_authored_order",
                build_root=build_root,
                expected_revision=0,
                apply=True,
                dry_run=False,
                json=True,
            )
            completed = SimpleNamespace(
                returncode=0,
                stdout=json.dumps(result),
                stderr="",
            )
            with patch.object(
                progress_cli.subprocess,
                "run",
                side_effect=git_inventory_aware_run(completed),
            ):
                code, payload = progress_cli.advance_live_order(args)
            self.assertEqual(code, 0)
            self.assertTrue(payload["commit"]["applied"])
            self.assertEqual(payload["commit"]["revision"], 1)
            updated = ProgressDocument.load(tracker)
            authored = updated.collection("physical_blocks")["recoil:block:0x401000"]["order"]["authored"]
            self.assertTrue(all(row["freshness"] == "current" for row in authored.values()))
            evidence_id = payload["evidence_id"]
            self.assertEqual(evidence_id, "recoil:evidence:r1:000001")
            self.assertEqual(updated.collection("evidence")[evidence_id]["validation_mode"], "live")

    def test_order_divergence_does_not_accept_partial_current_block(self):
        result = {
            "kind": "vc5-order-live-result",
            "target_id": "sample_authored_order",
            "phase": "authored-function-order",
            "physical_block_id": "recoil:block:0x401000",
            "passed": False,
            "expected_sequence": ["recoil:function:0x401000"],
            "candidate_sequence": ["recoil:logical-function:0x401010:extra"],
            "matched_prefix_count": 0,
            "first_divergence": {
                "kind": "extra",
                "identity": "recoil:logical-function:0x401010:extra",
            },
        }
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, fixture_document())
            build_root = REPO_ROOT / "build" / "live-validation" / "test-order-divergence"
            if build_root.exists():
                self.skipTest("test build root unexpectedly exists")
            args = argparse.Namespace(
                progress=tracker,
                target="recoil:vc5-target:sample_authored_order",
                build_root=build_root,
                expected_revision=0,
                apply=True,
                dry_run=False,
                json=True,
            )
            completed = SimpleNamespace(returncode=1, stdout=json.dumps(result), stderr="")
            with patch.object(
                progress_cli.subprocess,
                "run",
                side_effect=git_inventory_aware_run(completed),
            ):
                code, payload = progress_cli.advance_live_order(args)
            self.assertEqual(code, 1)
            self.assertFalse(payload["mutation_planned"])
            self.assertEqual(ProgressDocument.load(tracker).revision, 0)

    def test_order_pass_accepts_every_contiguous_target_block_atomically(self):
        identities = [
            "recoil:function:0x401000",
            "recoil:function:0x401010",
            "recoil:function:0x401020",
        ]
        covered = [
            "recoil:block:0x401000",
            "recoil:block:0x401010",
            "recoil:block:0x401020",
        ]
        result = {
            "kind": "vc5-order-live-result",
            "target_id": "sample_authored_order",
            "phase": "authored-function-order",
            "physical_block_id": covered[0],
            "passed": True,
            "expected_sequence": identities,
            "candidate_sequence": identities,
            "matched_prefix_count": len(identities),
            "first_divergence": None,
        }
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, multi_block_order_fixture())
            build_root = REPO_ROOT / "build" / "live-validation" / "test-multi-order-pass"
            if build_root.exists():
                self.skipTest("test build root unexpectedly exists")
            args = argparse.Namespace(
                progress=tracker,
                target="recoil:vc5-target:sample_authored_order",
                build_root=build_root,
                expected_revision=0,
                apply=True,
                dry_run=False,
                json=True,
            )
            completed = SimpleNamespace(returncode=0, stdout=json.dumps(result), stderr="")
            with patch.object(
                progress_cli.subprocess,
                "run",
                side_effect=git_inventory_aware_run(completed),
            ) as runner:
                code, payload = progress_cli.advance_live_order(args)
            self.assertEqual(code, 0)
            self.assertEqual(runner.call_count, 3)
            self.assertEqual(payload["covered_block_ids"], covered)
            self.assertEqual(payload["accepted_block_ids"], covered)
            updated = ProgressDocument.load(tracker)
            for block_id in covered:
                authored = updated.collection("physical_blocks")[block_id]["order"]["authored"]
                self.assertTrue(all(row["freshness"] == "current" for row in authored.values()))

    def test_multi_tu_raw_sequence_maps_to_retail_sorted_atomic_block_coverage(self):
        fixture = multi_tu_retail_contiguous_order_fixture()
        document = ProgressDocument(fixture)
        contract = progress_cli._current_order_contract(
            document, "recoil:vc5-target:sample_authored_order"
        )
        raw_tu_sequence = [
            "recoil:function:0x401000",
            "recoil:function:0x401020",
            "recoil:function:0x401010",
        ]
        retail_blocks = [
            "recoil:block:0x401000",
            "recoil:block:0x401010",
            "recoil:block:0x401020",
        ]
        self.assertEqual(raw_tu_sequence, contract["identities"])
        self.assertEqual(retail_blocks, contract["covered_block_ids"])
        self.assertEqual(retail_blocks, contract["target_owned_block_ids"])
        self.assertEqual([retail_blocks], contract["target_slices"])
        resolution = document.pipeline("recoil")["order_target_resolution"]
        self.assertEqual("ready", resolution["status"])
        self.assertEqual(retail_blocks, resolution["covered_block_ids"])

        result = {
            "kind": "vc5-order-live-result",
            "target_id": "sample_authored_order",
            "phase": "authored-function-order",
            "physical_block_id": retail_blocks[0],
            "passed": True,
            "expected_sequence": raw_tu_sequence,
            "candidate_sequence": raw_tu_sequence,
            "matched_prefix_count": len(raw_tu_sequence),
            "first_divergence": None,
        }
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, fixture)
            build_root = (
                REPO_ROOT
                / "build"
                / "live-validation"
                / "test-multi-tu-retail-contiguous-order-pass"
            )
            if build_root.exists():
                self.skipTest("test build root unexpectedly exists")
            args = argparse.Namespace(
                progress=tracker,
                target="recoil:vc5-target:sample_authored_order",
                object_target=None,
                linked_target=None,
                build_root=build_root,
                expected_revision=0,
                apply=True,
                dry_run=False,
                json=True,
            )
            completed = SimpleNamespace(
                returncode=0,
                stdout=json.dumps(result),
                stderr="",
            )
            with patch.object(
                progress_cli.subprocess,
                "run",
                side_effect=git_inventory_aware_run(completed),
            ):
                code, payload = progress_cli.advance_live_order(args)
            updated = ProgressDocument.load(tracker)

        self.assertEqual(0, code)
        self.assertEqual(retail_blocks, payload["accepted_block_ids"])
        for block_id in retail_blocks:
            block = updated.collection("physical_blocks")[block_id]
            self.assertTrue(
                ProgressDocument._block_authored_order_accepted(block)
            )
            self.assertEqual(
                retail_blocks,
                block["accepted_order_facts"]["covered_block_ids"],
            )
            self.assertEqual(
                contract["identities_by_block"][block_id],
                block["accepted_order_facts"]["matched_identities"],
            )

    def test_multi_tu_physical_coverage_still_rejects_nonexact_envelopes(self):
        cases = []

        gap = multi_tu_retail_contiguous_order_fixture()
        gap["physical_blocks"]["recoil:block:0x401010"]["start"] = "0x401011"
        cases.append(("gap", gap, None, "untracked gap"))

        partial_interval = multi_tu_retail_contiguous_order_fixture()
        cases.append(
            (
                "partial-interval",
                partial_interval,
                ("0x401001", "0x401040"),
                "does not exactly cover the tracker block envelope",
            )
        )

        omitted_gating = multi_tu_retail_contiguous_order_fixture()
        omitted_gating["verification_targets"][
            "recoil:vc5-target:sample_authored_order"
        ]["registration"]["translation_unit_function_order"][1][
            "functions"
        ].clear()
        cases.append(
            (
                "unrelated-middle-block",
                omitted_gating,
                None,
                "crosses an unrelated physical block",
            )
        )

        cursor_inside = multi_tu_retail_contiguous_order_fixture()
        cursor_inside["physical_blocks"]["recoil:block:0x401000"]["order"][
            "authored"
        ] = accepted(AUTHORED_ORDER_DIMENSIONS)
        cases.append(
            (
                "cursor-not-first",
                cursor_inside,
                None,
                "current physical slice starts at recoil:block:0x401000",
            )
        )

        for label, data, explicit_interval, message in cases:
            with self.subTest(label=label), self.assertRaisesRegex(
                ProgressError, message
            ):
                document = ProgressDocument(data)
                if explicit_interval is None:
                    progress_cli._target_order_contract(
                        document,
                        "recoil:vc5-target:sample_authored_order",
                    )
                else:
                    with patch.object(
                        progress_cli,
                        "_registered_order_interval",
                        return_value=explicit_interval,
                    ):
                        progress_cli._target_order_contract(
                            document,
                            "recoil:vc5-target:sample_authored_order",
                            require_explicit_interval=True,
                        )

    def test_order_divergence_accepts_no_block_from_multi_block_target(self):
        fixture = multi_block_order_fixture()
        identities = [
            "recoil:function:0x401000",
            "recoil:function:0x401010",
            "recoil:function:0x401020",
        ]
        result = {
            "kind": "vc5-order-live-result",
            "target_id": "sample_authored_order",
            "phase": "authored-function-order",
            "physical_block_id": "recoil:block:0x401000",
            "passed": False,
            "expected_sequence": identities,
            "candidate_sequence": identities[:2],
            "matched_prefix_count": 2,
            "first_divergence": {
                "kind": "missing",
                "identity": identities[2],
            },
        }
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, fixture)
            build_root = REPO_ROOT / "build" / "live-validation" / "test-multi-order-fail"
            if build_root.exists():
                self.skipTest("test build root unexpectedly exists")
            args = argparse.Namespace(
                progress=tracker,
                target="recoil:vc5-target:sample_authored_order",
                build_root=build_root,
                expected_revision=0,
                apply=True,
                dry_run=False,
                json=True,
            )
            completed = SimpleNamespace(returncode=1, stdout=json.dumps(result), stderr="")
            with patch.object(
                progress_cli.subprocess,
                "run",
                side_effect=git_inventory_aware_run(completed),
            ):
                code, payload = progress_cli.advance_live_order(args)
            self.assertEqual(code, 1)
            self.assertEqual(payload["accepted_block_ids"], [])
            self.assertEqual(ProgressDocument.load(tracker).revision, 0)
            for block in fixture["physical_blocks"].values():
                self.assertFalse(ProgressDocument._block_authored_order_accepted(block))

    def test_order_target_cannot_cross_an_unlisted_physical_block(self):
        fixture = multi_block_order_fixture()
        target = fixture["verification_targets"]["recoil:vc5-target:sample_authored_order"]
        target["registration"]["translation_unit_function_order"][0]["functions"].pop(1)
        document = ProgressDocument(fixture)
        with self.assertRaisesRegex(Exception, "crosses an unrelated physical block"):
            progress_cli._current_order_contract(
                document, "recoil:vc5-target:sample_authored_order"
            )

    def test_claim_current_creates_and_reserves_compact_safe_order_packet(self):
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, multi_block_order_fixture())
            args = argparse.Namespace(
                progress=tracker,
                lane="primary",
                issue_ledger=self.issue_ledger,
                expected_revision=0,
                apply=True,
                dry_run=False,
                json=True,
            )
            with patch.object(
                progress_cli,
                "resolve_topology",
                side_effect=AssertionError(
                    "JSON progress fixture reached native Git topology"
                ),
            ):
                payload = progress_cli.claim_current_work(args)
            self.assertTrue(payload["commit"]["applied"])
            self.assertEqual(payload["commit"]["revision"], 1)
            packet = payload["packet"]
            self.assertEqual(packet["packet_type"], "order-edit-v1")
            self.assertTrue(packet["planned"])
            self.assertFalse(packet["handoff_visible"])
            self.assertEqual("native-git-v1", packet["progress_packet_adapter"]["adapter"])
            self.assertTrue(packet["progress_packet_adapter"]["planned"])
            self.assertFalse(
                packet["progress_packet_adapter"]["runtime_authority"]
            )
            self.assertNotIn("covered_block_ids", packet)
            self.assertNotIn("worker_command", packet)
            updated = ProgressDocument.load(tracker)
            stored = updated.collection("work_items")[packet["packet_id"]]
            self.assertEqual(
                stored["covered_block_ids"],
                [
                    "recoil:block:0x401000",
                    "recoil:block:0x401010",
                    "recoil:block:0x401020",
                ],
            )
            self.assertEqual(1, len(stored["validation_commands"]))
            worker_command = stored["validation_commands"][0]
            self.assertIn("verify vc5-order sample_authored_order", worker_command)
            self.assertNotIn("--apply", worker_command)
            self.assertNotIn("advance-live-order", worker_command)
            self.assertEqual(stored["state"], "active")
            self.assertEqual(stored["reservation"]["state"], "active")
            self.assertEqual(stored["reservation"]["id"], packet["reservation_id"])
            self.assertEqual(
                "native-git-v1-planned", stored["progress_packet_adapter"]
            )
            self.assertEqual(
                {
                    "schema_version": 1,
                    "command": "progress work claim-current",
                    "requested_lane": "primary",
                    "selected_lane": "primary",
                    "max_packets": 3,
                },
                stored["claim_provenance"],
            )
            self.assertEqual(stored["claim_provenance"], packet["claim_provenance"])

            handoff_args = argparse.Namespace(
                authored_object_byte=False,
                authored_byte=False,
                fallback_authored_byte=False,
            )
            with self.assertRaisesRegex(
                ProgressError,
                "planned native-git-v1 allocation.*not handoff-visible",
            ):
                progress_cli._handoff(updated, handoff_args)

    def test_scheduler_launch_plan_keeps_parallel_byte_lanes_when_primary_is_blocked(self):
        fixture = parallel_lane_fixture()
        fixture["physical_blocks"]["recoil:block:0x401020"]["order_targets"]["object"] = ""
        with patch.object(progress_cli, "_byte_lane_preflight", side_effect=ready_byte_preflight):
            state = ProgressDocument(fixture).next_work(
                "recoil", issue_ledger=self.issue_ledger
            )

        self.assertEqual("blocked", state["cursor_launchability"]["primary"]["launchability"])
        self.assertEqual(
            "launchable",
            state["cursor_launchability"]["parallel_authored_byte"]["launchability"],
        )
        self.assertEqual(
            "launchable",
            state["cursor_launchability"]["parallel_authored_object_byte"]["launchability"],
        )
        self.assertEqual(["authored", "object"], [
            row["lane"] for row in state["launch_plan"] if row["selected_opportunity"]
        ])
        self.assertIn("--lane all --max-packets 3", state["claim_all_command"])

    def test_next_work_injects_a_concrete_issue_ledger_into_claim_composition(self):
        document = ProgressDocument(fixture_document())
        with patch.object(
            progress_cli,
            "describe_current_claim_opportunities",
            return_value={},
        ) as describe:
            document.next_work("recoil", issue_ledger=self.issue_ledger)
        self.assertEqual(Path(self.issue_ledger), describe.call_args.kwargs["issue_ledger"])

        with patch.object(
            progress_cli,
            "describe_current_claim_opportunities",
            return_value={},
        ) as describe:
            document.next_work("recoil", issue_ledger=None)
        self.assertEqual(
            Path(progress_cli.DEFAULT_ISSUE_LEDGER),
            describe.call_args.kwargs["issue_ledger"],
        )

        with self.assertRaises(TypeError):
            progress_cli.describe_current_claim_opportunities(
                document,
                document.pipeline("recoil"),
            )

    def test_scheduler_uses_only_the_injected_active_issue_reservations(self):
        whole_project_claim = [
            {"kind": "whole-project-build", "id": "recoil", "access": "write"}
        ]
        active_ledger = write_issue_ledger(
            Path(self.issue_temporary.name) / "active.sqlite3",
            resource_claims=whole_project_claim,
        )
        released_ledger = write_issue_ledger(
            Path(self.issue_temporary.name) / "released.sqlite3",
            resource_claims=whole_project_claim,
            active=False,
        )
        disjoint_ledger = write_issue_ledger(
            Path(self.issue_temporary.name) / "disjoint.sqlite3",
            resource_claims=[
                {"kind": "path", "id": "unrelated/issue-only.cpp", "access": "write"}
            ],
        )

        def schedule(issue_ledger: Path) -> dict:
            with patch.object(
                progress_cli,
                "_byte_lane_preflight",
                side_effect=ready_byte_preflight,
            ):
                return ProgressDocument(parallel_lane_fixture()).next_work(
                    "recoil", issue_ledger=issue_ledger
                )

        active = schedule(active_ledger)
        self.assertEqual(
            "blocked",
            active["cursor_launchability"]["parallel_authored_byte"]["launchability"],
        )
        self.assertEqual(
            "active-resource-conflict",
            active["cursor_launchability"]["parallel_authored_byte"]["reason_code"],
        )
        self.assertEqual(
            "workspace-issues",
            active["cursor_launchability"]["parallel_authored_byte"]["conflicts"][0][
                "other_packet_source"
            ],
        )
        self.assertEqual(
            ["primary", "object"],
            [
                row["lane"]
                for row in active["launch_plan"]
                if row["selected_opportunity"]
            ],
        )

        for name, issue_ledger in (
            ("released", released_ledger),
            ("disjoint", disjoint_ledger),
        ):
            with self.subTest(name=name):
                state = schedule(issue_ledger)
                self.assertEqual(
                    ["primary", "authored", "object"],
                    [
                        row["lane"]
                        for row in state["launch_plan"]
                        if row["selected_opportunity"]
                    ],
                )

    def test_scheduler_fails_closed_on_missing_or_malformed_injected_sqlite(self):
        missing = Path(self.issue_temporary.name) / "missing.sqlite3"
        with patch.object(
            progress_cli,
            "_byte_lane_preflight",
            side_effect=ready_byte_preflight,
        ):
            state = ProgressDocument(parallel_lane_fixture()).next_work(
                "recoil", issue_ledger=missing
            )
        self.assertFalse(missing.exists())
        self.assertTrue(
            all(
                lane["launchability"] == "blocked"
                and lane["reason_code"] == "packet-construction-blocked"
                and "cutover migration" in lane["reason"]
                for lane in state["cursor_launchability"].values()
            )
        )

        malformed = Path(self.issue_temporary.name) / "malformed.sqlite3"
        malformed.write_text("not a SQLite issue ledger\n", encoding="utf-8")
        with (
            patch.object(
                progress_cli,
                "_byte_lane_preflight",
                side_effect=ready_byte_preflight,
            ),
            self.assertRaisesRegex(Exception, "not a database"),
        ):
            ProgressDocument(parallel_lane_fixture()).next_work(
                "recoil", issue_ledger=malformed
            )

    def test_claim_current_revalidates_the_injected_issue_ledger(self):
        active_ledger = write_issue_ledger(
            Path(self.issue_temporary.name) / "claim-active.sqlite3",
            resource_claims=[
                {"kind": "whole-project-build", "id": "recoil", "access": "write"}
            ],
        )
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, parallel_lane_fixture())
            args = argparse.Namespace(
                progress=tracker,
                lane="all",
                max_packets=3,
                issue_ledger=active_ledger,
                expected_revision=0,
                apply=False,
                dry_run=True,
                json=True,
            )
            with patch.object(
                progress_cli,
                "_byte_lane_preflight",
                side_effect=ready_byte_preflight,
            ):
                payload = progress_cli.claim_current_work(args)

        self.assertEqual(
            ["primary", "object"], [packet["lane"] for packet in payload["packets"]]
        )
        authored = next(
            blocker for blocker in payload["blockers"] if blocker["lane"] == "authored"
        )
        self.assertEqual("active-resource-conflict", authored["reason_code"])
        self.assertFalse(payload["commit"]["applied"])

    def test_unresolved_in_range_order_row_blocks_whole_block_but_not_byte_lanes(self):
        fixture = unresolved_primary_order_fixture()
        with patch.object(progress_cli, "_byte_lane_preflight", side_effect=ready_byte_preflight):
            state = ProgressDocument(fixture).next_work(
                "recoil", issue_ledger=self.issue_ledger
            )

        resolution = state["order_target_resolution"]
        self.assertEqual("blocked", resolution["status"])
        self.assertEqual("order-target-role-gate-blocked", resolution["reason_code"])
        self.assertEqual("0x401020", resolution["blocker"]["address"])
        self.assertEqual(
            "recoil:function:0x401020", resolution["blocker"]["identity"]
        )
        self.assertEqual(
            [
                "registration pipeline_class='unresolved'",
                "registration authored_order_role='unresolved'",
            ],
            resolution["blocker"]["problems"],
        )
        self.assertIn("UnresolvedPrimaryRow", resolution["reason"])
        self.assertEqual("", state["next_command"])
        self.assertEqual(
            ["authored", "object"],
            [row["lane"] for row in state["launch_plan"] if row["selected_opportunity"]],
        )

    def test_claim_all_reserves_byte_packets_and_returns_unresolved_primary_blocker(self):
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, unresolved_primary_order_fixture())
            args = argparse.Namespace(
                progress=tracker,
                lane="all",
                max_packets=3,
                issue_ledger=self.issue_ledger,
                expected_revision=0,
                apply=True,
                dry_run=False,
                json=True,
            )
            with (
                patch.object(
                    progress_cli,
                    "_byte_lane_preflight",
                    side_effect=ready_byte_preflight,
                ),
                patch.object(
                    progress_cli,
                    "resolve_topology",
                    side_effect=AssertionError(
                        "JSON progress fixture reached native Git topology"
                    ),
                ),
            ):
                payload = progress_cli.claim_current_work(args)

            self.assertEqual(["authored", "object"], [
                packet["lane"] for packet in payload["packets"]
            ])
            primary = next(
                blocker for blocker in payload["blockers"] if blocker["lane"] == "primary"
            )
            self.assertEqual("order-target-role-gate-blocked", primary["reason_code"])
            self.assertIn("0x401020", primary["reason"])
            self.assertIn("recoil:function:0x401020", primary["reason"])
            self.assertEqual(1, ProgressDocument.load(tracker).revision)

    def test_live_order_acceptance_rejects_unresolved_row_before_build(self):
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, unresolved_primary_order_fixture())
            args = argparse.Namespace(
                progress=tracker,
                target="recoil:vc5-target:third_order",
                object_target=None,
                linked_target=None,
                build_root=REPO_ROOT / "build" / "live-validation" / "never-created",
                expected_revision=0,
                apply=True,
                dry_run=False,
                json=True,
            )
            with patch.object(progress_cli.subprocess, "run") as runner:
                with self.assertRaisesRegex(
                    progress_cli.OrderTargetRoleGateError,
                    r"0x401020 .*recoil:function:0x401020",
                ):
                    progress_cli.advance_live_order(args)
            runner.assert_not_called()
            self.assertEqual(0, ProgressDocument.load(tracker).revision)

    def test_claim_all_reserves_three_compatible_packets_in_one_revision(self):
        fixture = parallel_lane_fixture()
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, fixture)
            args = argparse.Namespace(
                progress=tracker,
                lane="all",
                max_packets=3,
                issue_ledger=self.issue_ledger,
                expected_revision=0,
                apply=True,
                dry_run=False,
                json=True,
            )
            with patch.object(
                progress_cli, "_byte_lane_preflight", side_effect=ready_byte_preflight
            ):
                payload = progress_cli.claim_current_work(args)

            self.assertEqual(1, payload["commit"]["revision"])
            self.assertEqual(3, len(payload["packets"]))
            self.assertEqual(
                ["primary", "authored", "object"],
                [packet["lane"] for packet in payload["packets"]],
            )
            self.assertEqual(
                [
                    {
                        "schema_version": 1,
                        "command": "progress work claim-current",
                        "requested_lane": "all",
                        "selected_lane": lane,
                        "max_packets": 3,
                    }
                    for lane in ("primary", "authored", "object")
                ],
                [packet["claim_provenance"] for packet in payload["packets"]],
            )
            self.assertTrue(all(packet["planned"] for packet in payload["packets"]))
            self.assertTrue(
                all(not packet["handoff_visible"] for packet in payload["packets"])
            )
            self.assertTrue(
                all(
                    packet["progress_packet_adapter"]["runtime_authority"] is False
                    for packet in payload["packets"]
                )
            )
            self.assertTrue(
                all("worker_command" not in packet for packet in payload["packets"])
            )
            updated = ProgressDocument.load(tracker)
            self.assertEqual(1, updated.revision)
            self.assertEqual(3, sum(
                1
                for work in updated.collection("work_items").values()
                if work.get("state") == "active"
            ))
            stored_packets = [
                updated.collection("work_items")[packet["packet_id"]]
                for packet in payload["packets"]
            ]
            worker_commands = [
                packet["validation_commands"][0] for packet in stored_packets
            ]
            self.assertEqual(3, len({
                command.split("--build-root ", 1)[1].split()[0]
                for command in worker_commands
            }))
            self.assertTrue(all("--apply" not in command for command in worker_commands))
            self.assertTrue(
                all(
                    packet["progress_packet_adapter"] == "native-git-v1-planned"
                    for packet in stored_packets
                )
            )
            for selected in payload["packets"]:
                with self.assertRaisesRegex(
                    ProgressError,
                    "planned native-git-v1 allocation.*not handoff-visible",
                ):
                    progress_cli._handoff(
                        updated,
                        argparse.Namespace(
                            packet_id=selected["packet_id"],
                            authored_object_byte=False,
                            authored_byte=False,
                            fallback_authored_byte=False,
                        ),
                    )

    def test_claim_all_gives_authored_lane_priority_over_overlapping_object_lane(self):
        fixture = parallel_lane_fixture(overlapping_byte_sources=True)
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, fixture)
            args = argparse.Namespace(
                progress=tracker,
                lane="all",
                max_packets=3,
                issue_ledger=self.issue_ledger,
                expected_revision=0,
                apply=False,
                dry_run=True,
                json=True,
            )
            with patch.object(
                progress_cli, "_byte_lane_preflight", side_effect=ready_byte_preflight
            ):
                payload = progress_cli.claim_current_work(args)

            self.assertEqual(["primary", "authored"], [
                packet["lane"] for packet in payload["packets"]
            ])
            self.assertEqual("object", payload["skipped"][0]["lane"])
            self.assertEqual(
                "higher-priority-resource-conflict", payload["skipped"][0]["reason_code"]
            )
            self.assertEqual(0, ProgressDocument.load(tracker).revision)

    def test_claim_all_capacity_is_applied_after_priority_and_dry_run_is_nonmutating(self):
        fixture = parallel_lane_fixture()
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, fixture)
            args = argparse.Namespace(
                progress=tracker,
                lane="all",
                max_packets=2,
                issue_ledger=self.issue_ledger,
                expected_revision=0,
                apply=False,
                dry_run=True,
                json=True,
            )
            with patch.object(
                progress_cli, "_byte_lane_preflight", side_effect=ready_byte_preflight
            ):
                payload = progress_cli.claim_current_work(args)

            self.assertFalse(payload["commit"]["applied"])
            self.assertEqual(1, payload["commit"]["revision"])
            self.assertEqual(["primary", "authored"], [
                packet["lane"] for packet in payload["packets"]
            ])
            self.assertTrue(
                all(
                    packet["claim_provenance"]["max_packets"] == 2
                    for packet in payload["packets"]
                )
            )
            self.assertEqual({"lane": "object", "cursor": "0x401010", "reason_code": "capacity"}, payload["skipped"][0])
            self.assertEqual(0, ProgressDocument.load(tracker).revision)

    def test_claim_current_stale_revision_mutates_nothing(self):
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, parallel_lane_fixture())
            args = argparse.Namespace(
                progress=tracker,
                lane="all",
                max_packets=3,
                issue_ledger=self.issue_ledger,
                expected_revision=7,
                apply=True,
                dry_run=False,
                json=True,
            )
            with self.assertRaisesRegex(Exception, "revision changed"):
                progress_cli.claim_current_work(args)
            self.assertEqual(0, ProgressDocument.load(tracker).revision)

    def test_scheduler_blocks_only_the_lane_with_an_active_resource_conflict(self):
        fixture = parallel_lane_fixture()
        fixture["work_items"]["recoil:work:active-object-source"] = {
            "binary": "recoil",
            "state": "active",
            "phase": "unrelated-maintenance",
            "lane": "unrelated",
            "cursor": "0x499999",
            "resource_claims": [
                {
                    "kind": "path",
                    "id": SECONDARY_FIXTURE_SOURCE,
                    "access": "write",
                }
            ],
            "reservation": {
                "id": "recoil:work:active-object-source:attempt:1",
                "state": "active",
            },
        }
        with patch.object(progress_cli, "_byte_lane_preflight", side_effect=ready_byte_preflight):
            state = ProgressDocument(fixture).next_work(
                "recoil", issue_ledger=self.issue_ledger
            )

        self.assertEqual("launchable", state["cursor_launchability"]["primary"]["launchability"])
        self.assertEqual(
            "launchable",
            state["cursor_launchability"]["parallel_authored_byte"]["launchability"],
        )
        object_lane = state["cursor_launchability"]["parallel_authored_object_byte"]
        self.assertEqual("blocked", object_lane["launchability"])
        self.assertEqual("active-resource-conflict", object_lane["reason_code"])
        self.assertTrue(object_lane["conflicts"])

    def test_authored_preflight_blocker_does_not_suppress_object_packet(self):
        fixture = parallel_lane_fixture()

        def selective_preflight(_document, *, lane, cursor, **_kwargs):
            if lane == "authored":
                return {
                    "passed": False,
                    "reason_code": "relocation-expectations-unresolved",
                    "reason": "missing exact target identity",
                    "target_ids": [],
                    "source_paths": [],
                }
            return ready_byte_preflight(_document, lane=lane, cursor=cursor)

        with patch.object(progress_cli, "_byte_lane_preflight", side_effect=selective_preflight):
            state = ProgressDocument(fixture).next_work(
                "recoil", issue_ledger=self.issue_ledger
            )

        self.assertEqual(
            "relocation-expectations-unresolved",
            state["cursor_launchability"]["parallel_authored_byte"]["reason_code"],
        )
        self.assertTrue(
            next(row for row in state["launch_plan"] if row["lane"] == "object")[
                "selected_opportunity"
            ]
        )

    def test_packet_id_handoff_fails_closed_for_unknown_packet(self):
        args = argparse.Namespace(
            packet_id="recoil:work:missing",
            authored_object_byte=False,
            authored_byte=False,
            fallback_authored_byte=False,
        )
        with self.assertRaisesRegex(Exception, "unknown work packet"):
            progress_cli._handoff(ProgressDocument(parallel_lane_fixture()), args)

    def test_unknown_packet_handoff_does_not_mask_malformed_issue_ledger(self):
        with tempfile.TemporaryDirectory() as temporary:
            issue_ledger = Path(temporary) / "issues.json"
            issue_ledger.write_text("{}\n", encoding="utf-8")
            args = argparse.Namespace(
                packet_id="recoil:work:missing",
                issue_ledger=issue_ledger,
                authored_object_byte=False,
                authored_byte=False,
                fallback_authored_byte=False,
            )
            with self.assertRaisesRegex(
                Exception,
                "invalid workspace issue ledger",
            ):
                progress_cli._handoff(
                    ProgressDocument(parallel_lane_fixture()),
                    args,
                )

    def test_handoff_fails_closed_without_real_active_reservation(self):
        args = argparse.Namespace(
            authored_object_byte=False,
            authored_byte=False,
            fallback_authored_byte=False,
        )
        with self.assertRaisesRegex(Exception, "active reserved work item"):
            progress_cli._handoff(ProgressDocument(multi_block_order_fixture()), args)

    def test_byte_advance_fails_closed_without_explicit_matched_groups(self):
        result = {
            "report_version": 1,
            "kind": "live-byte-lane",
            "validation_mode": "live",
            "lane": "linked",
            "passed": True,
            "checked_rows": 2,
            "selected_rows": 2,
            "first_divergence": None,
        }
        with tempfile.TemporaryDirectory() as temporary:
            tracker = Path(temporary) / "progress.json"
            write_tracker(tracker, fixture_document(linked_phase=True))
            build_root = REPO_ROOT / "build" / "live-validation" / "test-byte-contract"
            if build_root.exists():
                self.skipTest("test build root unexpectedly exists")
            args = argparse.Namespace(
                progress=tracker,
                lane="linked",
                build_root=build_root,
                expected_revision=0,
                apply=True,
                dry_run=False,
                json=True,
            )
            completed = SimpleNamespace(returncode=0, stdout=json.dumps(result), stderr="")
            with patch.object(progress_cli.subprocess, "run", return_value=completed) as runner:
                code, payload = progress_cli.advance_live_byte(args)
            self.assertEqual(code, 2)
            self.assertEqual(payload["status"], "contract-blocked")
            self.assertIn("matched_groups", payload["required_verifier_change"])
            self.assertEqual(ProgressDocument.load(tracker).revision, 0)
            command = runner.call_args.args[0]
            self.assertEqual(
                tracker.resolve(),
                Path(command[command.index("--progress") + 1]).resolve(),
            )

    def test_assigned_runtime_files_have_no_retired_active_mechanisms(self):
        from _recoil.commands.live_validation_surface_audit import audit_paths

        paths = [
            REPO_ROOT / "tools" / "_recoil" / "lib" / "progress.py",
            REPO_ROOT / "tools" / "_recoil" / "lib" / "progress_migration.py",
            REPO_ROOT / "tools" / "_recoil" / "commands" / "progress_cli.py",
            REPO_ROOT / "tools" / "_recoil" / "commands" / "progress_v2.py",
            REPO_ROOT / "tools" / "_recoil" / "pipeline_context.py",
            Path(__file__),
        ]
        self.assertEqual(audit_paths(paths), [])


if __name__ == "__main__":
    unittest.main()
