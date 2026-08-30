from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.lib.binja import (  # noqa: E402
    DEFAULT_BN_CALL_BUDGET,
    BinaryNinjaBridge,
    BridgeBudgetExceeded,
    create_shared_budget_file,
)


class FakeResponse:
    def __init__(self, text: str) -> None:
        self.text = text

    def __enter__(self) -> "FakeResponse":
        return self

    def __exit__(self, *_args: object) -> None:
        return None

    def read(self) -> bytes:
        return self.text.encode("utf-8")


def fake_json_response(*_args: object, **_kwargs: object) -> FakeResponse:
    return FakeResponse(json.dumps({"ok": True}))


def fake_hexdump_response(*_args: object, **_kwargs: object) -> FakeResponse:
    return FakeResponse("401000  c3  .")


class RecoilBinjaTests(unittest.TestCase):
    def test_bridge_has_no_session_receipt_api_or_endpoint(self) -> None:
        bridge = BinaryNinjaBridge("http://example.invalid")
        self.assertFalse(hasattr(bridge, "snapshot"))
        self.assertFalse(hasattr(bridge, "fresh_snapshot"))
        for relative in (
            "tools/_recoil/lib/binja.py",
            "tools/_recoil/lib/progress.py",
            "tools/_recoil/commands/call_contract_verify.py",
            "tools/_recoil/commands/progress_cli.py",
        ):
            source = (REPO_ROOT / relative).read_text(encoding="utf-8")
            self.assertNotIn("snapshot" + "Info", source)
            self.assertNotIn("binary_ninja_session", source)

    def test_default_bridge_call_budget_is_200(self) -> None:
        self.assertEqual(200, DEFAULT_BN_CALL_BUDGET)
        self.assertEqual(200, BinaryNinjaBridge("http://example.invalid").call_budget)

    def test_shared_budget_file_uses_default_bridge_call_budget(self) -> None:
        budget_file = create_shared_budget_file()
        try:
            budget_state = json.loads(budget_file.read_text(encoding="utf-8"))
        finally:
            budget_file.unlink(missing_ok=True)

        self.assertEqual(200, budget_state["limit"])
        self.assertEqual(0, budget_state["used"])

    def test_tenth_bridge_call_is_allowed_and_eleventh_is_blocked(self) -> None:
        bridge = BinaryNinjaBridge(
            "http://example.invalid",
            call_budget=10,
            use_environment_budget_file=False,
        )

        with patch("_recoil.lib.binja.urlopen", side_effect=fake_json_response) as opener:
            for _index in range(10):
                bridge.get_json("status")
            with self.assertRaises(BridgeBudgetExceeded):
                bridge.get_json("status")

        self.assertEqual(10, opener.call_count)

    def test_hexdump_counts_against_bridge_budget(self) -> None:
        bridge = BinaryNinjaBridge(
            "http://example.invalid",
            call_budget=1,
            use_environment_budget_file=False,
        )

        with patch("_recoil.lib.binja.urlopen", side_effect=fake_hexdump_response) as opener:
            self.assertIn("c3", bridge.hexdump("0x401000", 1))
            with self.assertRaises(BridgeBudgetExceeded):
                bridge.hexdump("0x401000", 1)

        self.assertEqual(1, opener.call_count)

    def test_zero_bridge_call_budget_is_unlimited(self) -> None:
        bridge = BinaryNinjaBridge(
            "http://example.invalid",
            call_budget=0,
            use_environment_budget_file=False,
        )

        with patch("_recoil.lib.binja.urlopen", side_effect=fake_json_response) as opener:
            for _index in range(12):
                bridge.get_json("status")

        self.assertEqual(12, opener.call_count)
        self.assertGreater(bridge.budget_state().remaining, 1000)

    def test_explicit_isolation_ignores_ambient_budget_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            ambient_budget_file = Path(tmp) / "ambient-budget.json"
            ambient_state = {"limit": 1, "used": 1}
            ambient_budget_file.write_text(
                json.dumps(ambient_state) + "\n",
                encoding="utf-8",
            )

            with patch.dict(
                "os.environ",
                {"RECOIL_BN_CALL_BUDGET_FILE": str(ambient_budget_file)},
            ):
                bridge = BinaryNinjaBridge(
                    "http://example.invalid",
                    call_budget=2,
                    use_environment_budget_file=False,
                )
                with patch(
                    "_recoil.lib.binja.urlopen",
                    side_effect=fake_json_response,
                ) as opener:
                    bridge.get_json("status")
                    bridge.get_json("status")
                    with self.assertRaises(BridgeBudgetExceeded):
                        bridge.get_json("status")

            self.assertEqual(2, opener.call_count)
            self.assertEqual(
                ambient_state,
                json.loads(ambient_budget_file.read_text(encoding="utf-8")),
            )

    def test_binary_selector_is_added_to_bridge_requests(self) -> None:
        bridge = BinaryNinjaBridge("http://example.invalid", binary="messages.bndb", call_budget=2)

        with patch("_recoil.lib.binja.urlopen", side_effect=fake_json_response) as opener:
            bridge.get_json("sections", offset=0, limit=5)
            bridge.hexdump("0x10001010", 4)

        first_url = opener.call_args_list[0].args[0]
        second_url = opener.call_args_list[1].args[0]
        self.assertIn("binary=messages.bndb", first_url)
        self.assertIn("binary=messages.bndb", second_url)

    def test_binary_selector_is_not_added_to_binaries_endpoint(self) -> None:
        bridge = BinaryNinjaBridge("http://example.invalid", binary="messages.bndb", call_budget=1)

        with patch("_recoil.lib.binja.urlopen", side_effect=fake_json_response) as opener:
            bridge.get_json("binaries")

        self.assertNotIn("binary=messages.bndb", opener.call_args.args[0])

    def test_shared_budget_file_is_used_by_multiple_bridge_instances(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            budget_file = Path(tmp) / "budget.json"
            first = BinaryNinjaBridge("http://example.invalid", call_budget=2, budget_file=budget_file)
            second = BinaryNinjaBridge("http://example.invalid", call_budget=2, budget_file=budget_file)

            with patch("_recoil.lib.binja.urlopen", side_effect=fake_json_response) as opener:
                first.get_json("status")
                second.get_json("status")
                with self.assertRaises(BridgeBudgetExceeded):
                    first.get_json("status")

        self.assertEqual(2, opener.call_count)

    def test_data_variables_are_paginated_typed_and_cached(self) -> None:
        bridge = BinaryNinjaBridge("http://example.invalid", call_budget=2)
        pages = (
            {
                "data": [
                    {
                        "address": "0x4cc898",
                        "name": "g_Table",
                        "raw_name": "_g_Table",
                        "type": "void*[30]",
                        "size": "0x78",
                    }
                ],
                "total": 2,
            },
            {
                "data": [
                    {
                        "address": "0x4cc918",
                        "name": "g_Other",
                        "raw_name": "_g_Other",
                        "type": "uint32_t",
                        "size": 4,
                    }
                ],
                "total": 2,
            },
        )
        with patch.object(bridge, "get_json", side_effect=pages) as getter:
            first = bridge.data_variables()
            second = bridge.data_variables()

        self.assertIs(first, second)
        self.assertEqual(2, getter.call_count)
        self.assertEqual(0, getter.call_args_list[0].kwargs["offset"])
        self.assertEqual(1, getter.call_args_list[1].kwargs["offset"])
        self.assertEqual("0x4cc898", first[0].address)
        self.assertEqual("_g_Table", first[0].raw_name)
        self.assertEqual("void*[30]", first[0].type_text)
        self.assertEqual(0x78, first[0].size)


if __name__ == "__main__":
    unittest.main()
