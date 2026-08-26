from __future__ import annotations

from pathlib import Path
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.bn_data_evidence import collect_bn_data_evidence  # noqa: E402
from _recoil.lib.binja import BridgeError, Symbol  # noqa: E402


class FakeBridge:
    def __init__(self, *, xrefs_supported: bool = True, xrefs_endpoint: str = "xrefs") -> None:
        self.xrefs_supported = xrefs_supported
        self.xrefs_endpoint = xrefs_endpoint

    def get_json(self, endpoint: str, **params: object) -> dict:
        if endpoint == "data":
            return {
                "items": [
                    {
                        "address": "0x4e5950",
                        "name": "g_Previous",
                        "size": 4,
                        "section": ".data",
                        "type": "int",
                    },
                    {
                        "address": "0x4e5954",
                        "name": "g_Target",
                        "size": 8,
                        "section": ".data",
                        "type": "float[2]",
                    },
                    {
                        "address": "0x4e595c",
                        "name": "g_Next",
                        "size": 4,
                        "section": ".data",
                        "type": "int",
                    },
                ],
                "total": 3,
            }
        if endpoint in {"getXrefsTo", "get_xrefs_to", "xrefs", "references", "dataReferences", "codeReferences"}:
            if not self.xrefs_supported:
                raise BridgeError(f"{endpoint} unsupported")
            if endpoint != self.xrefs_endpoint:
                raise BridgeError(f"{endpoint} unsupported")
            if str(params.get("address", "")).lower() == "0x4e5954":
                if endpoint == "getXrefsTo":
                    return {
                        "data_references": [
                            {
                                "source_addr": "0x401234",
                                "source_function_name": "UsesTarget",
                                "instruction_text": "mov eax, [0x4e5954]",
                            }
                        ],
                        "code_references": [],
                    }
                return {
                    "xrefs": [
                        {
                            "source": "0x401234",
                            "source_name": "UsesTarget",
                            "kind": "data",
                            "text": "mov eax, [0x4e5954]",
                        }
                    ]
                }
            return {"xrefs": []}
        raise BridgeError(f"unexpected endpoint {endpoint}")

    def hexdump(self, address: str, length: int) -> str:
        self.assert_hexdump_args = (address, length)
        return "4e5954  00 00 80 3f 54 59 4e 00  ...?TYN."

    def symbols(self) -> tuple[dict[str, Symbol], dict[str, Symbol]]:
        symbol = Symbol(address="0x402000", name="ScansTarget", kind="function")
        return {symbol.address: symbol}, {symbol.name: symbol}

    def assembly(self, address_or_name: str) -> str:
        if address_or_name == "0x402000":
            return "mov eax, [0x4e5958]\ncmp eax, 0x3f800000"
        return ""


class BnDataEvidenceTests(unittest.TestCase):
    def test_collects_data_shape_xrefs_hexdump_and_unsupported_relocations(self) -> None:
        result = collect_bn_data_evidence(
            bridge=FakeBridge(),
            binary="recoil",
            address="0x4e5954",
            size=8,
            nearby=0x10,
            constants="float",
            xref_limit=20,
            constant_limit=10,
            max_assembly_functions=10,
            assembly_hit_limit=10,
        ).as_dict()

        self.assertEqual("0x4e5954", result["address"])
        self.assertEqual("g_Target", result["data"]["exact"]["name"])
        self.assertEqual(2, len(result["adjacent_data"]))
        self.assertIn("00 00 80 3f", result["hexdump"])
        self.assertEqual("supported", result["direct_xrefs"]["status"])
        self.assertEqual("0x401234", result["direct_xrefs"]["hits"][0]["source_address"])
        self.assertEqual("supported", result["assembly_address_scan"]["status"])
        self.assertEqual("0x402000", result["assembly_address_scan"]["hits"][0]["function_address"])
        self.assertEqual("float", result["derived_constants"][0]["kind"])
        self.assertEqual("supported", result["global_constant_search"]["status"])
        self.assertEqual("unsupported", result["relocations"]["status"])

    def test_collects_xrefs_from_get_xrefs_to_endpoint(self) -> None:
        result = collect_bn_data_evidence(
            bridge=FakeBridge(xrefs_endpoint="getXrefsTo"),
            binary="recoil",
            address="0x4e5954",
            size=8,
            nearby=0,
            constants="none",
            xref_limit=20,
            constant_limit=10,
            max_assembly_functions=10,
            assembly_hit_limit=10,
        ).as_dict()

        self.assertEqual("supported", result["direct_xrefs"]["status"])
        self.assertEqual("getXrefsTo", result["direct_xrefs"]["endpoint"])
        self.assertEqual("0x401234", result["direct_xrefs"]["hits"][0]["source_address"])

    def test_reports_direct_xrefs_as_unsupported_when_bridge_lacks_endpoint(self) -> None:
        result = collect_bn_data_evidence(
            bridge=FakeBridge(xrefs_supported=False),
            binary="recoil",
            address="0x4e5954",
            size=8,
            nearby=0,
            constants="none",
            xref_limit=20,
            constant_limit=10,
            max_assembly_functions=10,
            assembly_hit_limit=10,
        ).as_dict()

        self.assertEqual("unsupported", result["direct_xrefs"]["status"])
        self.assertIn("Direct xref enumeration is unavailable", "\n".join(result["limitations"]))
        self.assertEqual("not_requested", result["global_constant_search"]["status"])


if __name__ == "__main__":
    unittest.main()
