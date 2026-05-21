from __future__ import annotations

from dataclasses import dataclass
import json
from urllib.parse import urlencode
from urllib.request import urlopen


DEFAULT_BRIDGE_URL = "http://localhost:9009"


@dataclass(frozen=True)
class Symbol:
    address: str
    name: str
    raw_name: str = ""
    full_name: str = ""
    kind: str = "function"


class BridgeError(RuntimeError):
    pass


class BinaryNinjaBridge:
    def __init__(self, base_url: str = DEFAULT_BRIDGE_URL, timeout: float = 15.0) -> None:
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self._symbols_by_address: dict[str, Symbol] | None = None
        self._symbols_by_name: dict[str, Symbol] | None = None

    def get_json(self, endpoint: str, **params: object) -> dict:
        query = urlencode({key: value for key, value in params.items() if value is not None})
        url = f"{self.base_url}/{endpoint.lstrip('/')}"
        if query:
            url = f"{url}?{query}"
        try:
            with urlopen(url, timeout=self.timeout) as response:
                return json.loads(response.read().decode("utf-8"))
        except Exception as exc:  # pragma: no cover - useful CLI diagnostic
            raise BridgeError(f"Binary Ninja bridge request failed: {url}: {exc}") from exc

    def function_info(self, address_or_name: str) -> dict:
        key = "address" if address_or_name.lower().startswith("0x") else "name"
        return self.get_json("functionInfo", **{key: address_or_name})

    def assembly(self, address_or_name: str) -> str:
        return str(self.get_json("assembly", name=address_or_name).get("assembly", ""))

    def hexdump(self, address: str, length: int) -> str:
        query = urlencode({"address": address, "length": str(length)})
        url = f"{self.base_url}/hexdump?{query}"
        try:
            with urlopen(url, timeout=self.timeout) as response:
                return response.read().decode("utf-8")
        except Exception as exc:  # pragma: no cover - useful CLI diagnostic
            raise BridgeError(f"Binary Ninja bridge request failed: {url}: {exc}") from exc

    def il(self, address_or_name: str, view: str = "mlil") -> str:
        key = "address" if address_or_name.lower().startswith("0x") else "name"
        return str(self.get_json("il", **{key: address_or_name, "view": view}).get("il", ""))

    def symbols(self) -> tuple[dict[str, Symbol], dict[str, Symbol]]:
        if self._symbols_by_address is not None and self._symbols_by_name is not None:
            return self._symbols_by_address, self._symbols_by_name

        by_address: dict[str, Symbol] = {}
        by_name: dict[str, Symbol] = {}
        for endpoint, collection, kind in (
            ("methods", "functions", "function"),
            ("imports", "imports", "import"),
        ):
            offset = 0
            limit = 1000
            while True:
                data = self.get_json(endpoint, offset=offset, limit=limit)
                for item in data.get(collection, []):
                    address = _normalize_address(str(item["address"]))
                    symbol = Symbol(
                        address=address,
                        name=str(item.get("name", "")),
                        raw_name=str(item.get("raw_name", "")),
                        full_name=str(item.get("full_name", "")),
                        kind=kind,
                    )
                    by_address[address] = symbol
                    for name in {symbol.name, symbol.raw_name, symbol.full_name}:
                        if name:
                            by_name[name] = symbol
                offset += limit
                if offset >= int(data.get("total", 0)):
                    break

        self._symbols_by_address = by_address
        self._symbols_by_name = by_name
        return by_address, by_name


def _normalize_address(value: str) -> str:
    value = value.strip()
    if value.lower().startswith("0x"):
        return f"0x{int(value, 16):x}"
    return f"0x{int(value, 16):x}"
