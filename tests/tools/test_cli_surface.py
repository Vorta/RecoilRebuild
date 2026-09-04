from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))

import recoil  # noqa: E402


RETIRED_ROUTES = {
    "progress status",
    "progress owner find",
    "progress owner relationships",
    "audit call-contract-readiness",
    "audit zinterp",
    "binja data-overlap",
    "verify authored-order scaffold",
    "verify authored-order sweep",
    "verify vc5-abi-equivalence",
    "verify zui-inline-context",
}


def test_registry_is_unique_reachable_and_free_of_retired_routes() -> None:
    names = [item.name for item in recoil.COMMAND_SPECS]
    assert len(names) == len(set(names))
    assert not RETIRED_ROUTES.intersection(names)
    for item in recoil.COMMAND_SPECS:
        assert (TOOLS / "_recoil" / "commands" / f"{item.module}.py").is_file()


def test_machine_readable_command_inventory_matches_registry() -> None:
    completed = subprocess.run(
        [sys.executable, str(TOOLS / "recoil.py"), "commands", "--json"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    assert completed.returncode == 0, completed.stderr
    payload = json.loads(completed.stdout)
    assert {row["command"] for row in payload} == {
        item.name for item in recoil.COMMAND_SPECS
    }


def test_dispatch_preserves_prepend_arguments() -> None:
    item = recoil.COMMANDS[("verify", "vc5-order")]
    command = recoil.build_command(item, ["unit", "--build-root", "scratch"])
    assert command[-4:] == ["--order-only", "unit", "--build-root", "scratch"]


def test_vc5_smoke_is_on_the_retained_verifier_route() -> None:
    item = recoil.COMMANDS[("verify", "vc5")]
    assert "--smoke" in " ".join(item.examples)
    assert ("audit", "source-policy") in recoil.COMMANDS


def test_source_path_relocation_is_one_public_global_cas_route() -> None:
    item = recoil.COMMANDS[("progress", "source-path", "relocate")]
    assert item.prepend_args == ("source-path", "relocate")
    assert item.required_revision_domains == ("global",)
    assert item.mutation_scope == "source-path"
