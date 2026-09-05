from __future__ import annotations

import struct
from pathlib import Path
import sys

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands.binja_preflight import (  # noqa: E402
    DataItem,
    collect_data_overlap_findings,
    normalize_address_text,
    validate_binaries,
    validate_status,
)
from _recoil.commands.resource_extract import (  # noqa: E402
    parse_message_table,
    raw_resource_filename_for_identity,
    resource_type_label,
)
from _recoil.commands.bn_data_evidence import scan_assembly_text  # noqa: E402
from _recoil.lib.binja import BridgeBudgetExceeded, BridgeError, Symbol  # noqa: E402


@pytest.mark.parametrize("failure", [BridgeError("read failed"), BridgeBudgetExceeded("budget"), ""])
def test_data_assembly_scan_reports_incomplete_reads(failure: object) -> None:
    class Bridge:
        def symbols(self):
            return {hex(a): Symbol(hex(a), "test") for a in (0x1000, 0x2000, 0x3000)}, {}

        def assembly(self, address: str) -> str:
            if address == "0x2000":
                if isinstance(failure, Exception):
                    raise failure
                return str(failure)
            return "mov eax, [0x9000]"

    scan = scan_assembly_text(
        Bridge(), address_targets=("0x9000",), constants=(), max_functions=4, hit_limit=8
    )
    assert scan["status"] == "partial"
    assert scan["functions_failed"] == 1
    assert scan["failures"][0]["function_address"] == "0x2000"
    budget = isinstance(failure, BridgeBudgetExceeded)
    assert scan["functions_checked"] == (1 if budget else 2)
    assert scan["functions_unattempted"] == (1 if budget else 0)
    assert scan["stop_reason"] == ("bridge_call_budget_exhausted" if budget else "")


def test_data_assembly_scan_distinguishes_complete_empty_and_failed_scans() -> None:
    class Bridge:
        fail = False

        def symbols(self):
            return {"0x1000": Symbol("0x1000", "test")}, {}

        def assembly(self, address: str) -> str:
            if self.fail:
                raise BridgeError("unavailable")
            return "ret"

    bridge = Bridge()
    args = dict(address_targets=("0x9000",), constants=(), max_functions=4, hit_limit=8)
    scan = scan_assembly_text(bridge, **args)
    assert scan["status"] == "supported" and scan["hits"] == []
    bridge.fail = True
    scan = scan_assembly_text(bridge, **args)
    assert scan["status"] == "unsupported" and scan["functions_failed"] == 1


def test_binja_status_requires_exact_database_platform_and_architecture() -> None:
    status = {
        "loaded": True,
        "filename": "D:/Evidence/Recoil.bndb",
        "platform": "windows-x86",
        "arch": "x86",
        "open_binaries": 1,
    }
    assert validate_status(
        status,
        expected_file="d:\\evidence\\recoil.bndb",
        expected_platform="windows-x86",
        expected_arch="x86",
    ) == []
    assert "platform" in validate_status(
        {**status, "platform": "linux-x86"},
        expected_file=status["filename"],
        expected_platform="windows-x86",
        expected_arch="x86",
    )[0]


def test_binja_binary_inventory_requires_the_expected_active_view() -> None:
    payload = {"binaries": [{"filename": "D:/Evidence/Recoil.bndb", "active": True}]}
    assert validate_binaries(payload, expected_file="d:/evidence/recoil.bndb") == []
    assert "not active" in validate_binaries(
        {"binaries": [{"filename": "D:/Evidence/Recoil.bndb", "active": False}]},
        expected_file="D:/Evidence/Recoil.bndb",
    )[0]


def test_data_overlap_diagnostic_reports_interior_roots_only() -> None:
    outer = DataItem("0x1000", 0x1000, 0x20, "outer", "char[32]", ".data")
    inner = DataItem("0x1008", 0x1008, 4, "inner", "int", ".data")
    edge = DataItem("0x1020", 0x1020, 4, "edge", "int", ".data")
    findings = collect_data_overlap_findings([outer, inner, edge])
    assert [(row.inner.name, row.outer.name, row.offset) for row in findings] == [
        ("inner", "outer", 8)
    ]
    assert normalize_address_text("1008") == "0x1008"


def test_message_table_and_resource_names_parse_from_synthetic_payload() -> None:
    text = b"Hello\0"
    payload = (
        struct.pack("<I", 1)
        + struct.pack("<III", 7, 7, 16)
        + struct.pack("<HH", 4 + len(text), 0)
        + text
    )
    rows = parse_message_table(payload)
    assert [(row.message_id, row.text) for row in rows] == [(7, "Hello")]
    assert resource_type_label(11) == "message_table"
    assert raw_resource_filename_for_identity(11, "GAME MESSAGE", 0x409) == (
        "message_table_GAME_MESSAGE_0409.bin"
    )
