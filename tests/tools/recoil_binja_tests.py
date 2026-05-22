from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_binja import BinaryNinjaBridge, BridgeBudgetExceeded  # noqa: E402


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
    def test_tenth_bridge_call_is_allowed_and_eleventh_is_blocked(self) -> None:
        bridge = BinaryNinjaBridge("http://example.invalid", call_budget=10)

        with patch("recoil_binja.urlopen", side_effect=fake_json_response) as opener:
            for _index in range(10):
                bridge.get_json("status")
            with self.assertRaises(BridgeBudgetExceeded):
                bridge.get_json("status")

        self.assertEqual(10, opener.call_count)

    def test_hexdump_counts_against_bridge_budget(self) -> None:
        bridge = BinaryNinjaBridge("http://example.invalid", call_budget=1)

        with patch("recoil_binja.urlopen", side_effect=fake_hexdump_response) as opener:
            self.assertIn("c3", bridge.hexdump("0x401000", 1))
            with self.assertRaises(BridgeBudgetExceeded):
                bridge.hexdump("0x401000", 1)

        self.assertEqual(1, opener.call_count)

    def test_shared_budget_file_is_used_by_multiple_bridge_instances(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            budget_file = Path(tmp) / "budget.json"
            first = BinaryNinjaBridge("http://example.invalid", call_budget=2, budget_file=budget_file)
            second = BinaryNinjaBridge("http://example.invalid", call_budget=2, budget_file=budget_file)

            with patch("recoil_binja.urlopen", side_effect=fake_json_response) as opener:
                first.get_json("status")
                second.get_json("status")
                with self.assertRaises(BridgeBudgetExceeded):
                    first.get_json("status")

        self.assertEqual(2, opener.call_count)


if __name__ == "__main__":
    unittest.main()
