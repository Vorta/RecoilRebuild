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


@pytest.fixture
def bridge_http_server():
    import json
    import threading
    from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
    from urllib.parse import parse_qs, urlsplit

    requests = []

    class Handler(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"
        disable_nagle_algorithm = True

        def log_message(self, *_):
            pass

        def do_GET(self):
            target = urlsplit(self.path)
            query = parse_qs(target.query)
            requests.append((self.client_address, target.path, query))
            if target.path == "/drop":
                self.close_connection = True
                return
            body = json.dumps(query).encode()
            if target.path == "/hexdump":
                body = b"00001000  90 c3\n"
            elif target.path == "/malformed":
                body = b"{"
            elif target.path == "/error":
                body = b'{"error":"read_busy"}'
            self.send_response(503 if target.path == "/error" else 200)
            self.send_header("Content-Length", str(len(body) + (5 if target.path == "/truncated" else 0)))
            self.end_headers()
            self.wfile.write(body)
            self.wfile.flush()
            if target.path in {"/idle-close", "/truncated"}:
                self.close_connection = True

    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        yield f"http://127.0.0.1:{server.server_port}", requests
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)


def test_bridge_reuses_one_connection_per_worker_and_preserves_qualification_and_budget(bridge_http_server):
    from concurrent.futures import ThreadPoolExecutor
    from _recoil.lib.binja import BinaryNinjaBridge

    url, requests = bridge_http_server

    def run(worker):
        bridge = BinaryNinjaBridge(url, binary="unit.bndb", call_budget=17, use_environment_budget_file=False)
        try:
            for index in range(16):
                assert bridge.get_json("facts", worker=worker, index=index) == {
                    "binary": ["unit.bndb"], "worker": [str(worker)], "index": [str(index)]}
            assert bridge.hexdump("0x1000", 2) == "00001000  90 c3\n"
            with pytest.raises(BridgeBudgetExceeded):
                bridge.get_json("facts")
        finally:
            bridge.close()

    with ThreadPoolExecutor(max_workers=8) as executor:
        list(executor.map(run, range(8)))
    assert len(requests) == 8 * 17
    assert len({address for address, _, _ in requests}) == 8
    shared = BinaryNinjaBridge(url, call_budget=16, use_environment_budget_file=False)
    try:
        with ThreadPoolExecutor(max_workers=8) as executor:
            rows = list(executor.map(lambda index: shared.get_json("facts", index=index), range(16)))
        assert rows == [{"index": [str(index)]} for index in range(16)]
        assert len({address for address, _, _ in requests}) == 9
    finally:
        shared.close()


def test_bridge_rejects_http_errors_malformed_json_and_partial_responses_without_retries(bridge_http_server):
    from _recoil.lib.binja import BinaryNinjaBridge

    url, requests = bridge_http_server
    bridge = BinaryNinjaBridge(url, use_environment_budget_file=False)
    try:
        for endpoint, message in (("error", "HTTP 503.*read_busy"), ("malformed", "invalid JSON"), ("truncated", "IncompleteRead")):
            with pytest.raises(BridgeError, match=message):
                bridge.get_json(endpoint)
            assert bridge.get_json("facts", after=endpoint) == {"after": [endpoint]}
        for endpoint in ("error", "malformed", "truncated"):
            assert sum(path == "/" + endpoint for _, path, _ in requests) == 1
    finally:
        bridge.close()


def test_bridge_reconnects_once_for_idle_close_and_charges_retry_budget(bridge_http_server):
    from _recoil.lib.binja import BinaryNinjaBridge

    url, requests = bridge_http_server
    bridge = BinaryNinjaBridge(url, call_budget=5, use_environment_budget_file=False)
    try:
        assert bridge.get_json("idle-close") == {}
        assert bridge.get_json("facts", resumed=True) == {"resumed": ["True"]}
        assert bridge.budget_state().used == 3
        # A reused connection that repeatedly drops gets exactly one reconnect.
        with pytest.raises(BridgeError):
            bridge.get_json("drop")
        assert sum(path == "/drop" for _, path, _ in requests) == 2
        assert bridge.budget_state().used == 5
        with pytest.raises(BridgeBudgetExceeded):
            bridge.get_json("facts")
    finally:
        bridge.close()
    fresh = BinaryNinjaBridge(url, call_budget=1, use_environment_budget_file=False)
    try:
        before = len(requests)
        with pytest.raises(BridgeError):
            fresh.get_json("drop")
        assert len(requests) == before + 1
    finally:
        fresh.close()
    limited = BinaryNinjaBridge(url, call_budget=2, use_environment_budget_file=False)
    try:
        assert limited.get_json("idle-close") == {}
        with pytest.raises(BridgeBudgetExceeded):
            limited.get_json("facts", exhausted=True)
        assert limited.budget_state().used == 2
        assert not any("exhausted" in query for _, _, query in requests)
    finally:
        limited.close()


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
