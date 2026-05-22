from __future__ import annotations

from dataclasses import dataclass
import json
import os
from pathlib import Path
import tempfile
from urllib.parse import urlencode
from urllib.request import urlopen


DEFAULT_BRIDGE_URL = "http://localhost:9009"
DEFAULT_BN_CALL_BUDGET = 10
BN_CALL_BUDGET_FILE_ENV = "RECOIL_BN_CALL_BUDGET_FILE"


@dataclass(frozen=True)
class Symbol:
    address: str
    name: str
    raw_name: str = ""
    full_name: str = ""
    kind: str = "function"


class BridgeError(RuntimeError):
    pass


class BridgeBudgetExceeded(BridgeError):
    pass


@dataclass(frozen=True)
class BridgeBudgetState:
    used: int
    limit: int

    @property
    def remaining(self) -> int:
        return max(0, self.limit - self.used)


class BinaryNinjaBridge:
    def __init__(
        self,
        base_url: str = DEFAULT_BRIDGE_URL,
        timeout: float = 15.0,
        *,
        call_budget: int = DEFAULT_BN_CALL_BUDGET,
        budget_file: str | Path | None = None,
    ) -> None:
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self.call_budget = call_budget
        self.budget_file = Path(
            budget_file if budget_file is not None else os.environ.get(BN_CALL_BUDGET_FILE_ENV, "")
        ) if (budget_file is not None or os.environ.get(BN_CALL_BUDGET_FILE_ENV)) else None
        self._calls_used = 0
        self._symbols_by_address: dict[str, Symbol] | None = None
        self._symbols_by_name: dict[str, Symbol] | None = None

    def budget_state(self) -> BridgeBudgetState:
        if self.budget_file is None:
            return BridgeBudgetState(used=self._calls_used, limit=self.call_budget)
        return self._read_budget_state()

    def _read_budget_state(self) -> BridgeBudgetState:
        if self.budget_file is None:
            return BridgeBudgetState(used=self._calls_used, limit=self.call_budget)
        try:
            data = json.loads(self.budget_file.read_text(encoding="utf-8"))
            used = int(data.get("used", 0))
            limit = int(data.get("limit", self.call_budget))
            return BridgeBudgetState(used=used, limit=limit)
        except FileNotFoundError:
            return BridgeBudgetState(used=0, limit=self.call_budget)
        except (OSError, ValueError, TypeError, json.JSONDecodeError) as exc:
            raise BridgeError(f"Binary Ninja bridge budget file is invalid: {self.budget_file}: {exc}") from exc

    def _write_budget_state(self, state: BridgeBudgetState) -> None:
        if self.budget_file is None:
            self._calls_used = state.used
            return
        self.budget_file.parent.mkdir(parents=True, exist_ok=True)
        self.budget_file.write_text(
            json.dumps({"used": state.used, "limit": state.limit}, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    def _claim_call(self, endpoint: str) -> None:
        state = self.budget_state()
        if state.used >= state.limit:
            raise BridgeBudgetExceeded(
                "Binary Ninja bridge call budget exhausted before "
                f"{endpoint}: used {state.used}/{state.limit} call(s)"
            )
        self._write_budget_state(BridgeBudgetState(used=state.used + 1, limit=state.limit))

    def get_json(self, endpoint: str, **params: object) -> dict:
        query = urlencode({key: value for key, value in params.items() if value is not None})
        url = f"{self.base_url}/{endpoint.lstrip('/')}"
        if query:
            url = f"{url}?{query}"
        self._claim_call(endpoint)
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
        self._claim_call("hexdump")
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
            limit = 100000
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


def create_shared_budget_file() -> Path:
    handle = tempfile.NamedTemporaryFile(
        prefix="recoil_bn_budget_",
        suffix=".json",
        delete=False,
        mode="w",
        encoding="utf-8",
    )
    path = Path(handle.name)
    with handle:
        handle.write(json.dumps({"used": 0, "limit": DEFAULT_BN_CALL_BUDGET}, sort_keys=True) + "\n")
    return path


def env_with_shared_budget(budget_file: Path, base_env: dict[str, str] | None = None) -> dict[str, str]:
    env = dict(base_env if base_env is not None else os.environ)
    env[BN_CALL_BUDGET_FILE_ENV] = str(budget_file)
    return env
