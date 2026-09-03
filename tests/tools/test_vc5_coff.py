from __future__ import annotations

from pathlib import Path
import struct
import sys

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands import vc5_verify  # noqa: E402
from _recoil.lib.coff_alias import (  # noqa: E402
    CoffAlias,
    CoffAliasSource,
    parse_alias_source_text,
    validate_alias_object,
)


def alias_object(alias: str = "_alias", target: str = "_target") -> bytes:
    def name(value: str) -> bytes:
        return value.encode("ascii").ljust(8, b"\0")

    target_row = name(target) + struct.pack("<IhHBB", 0, 0, 0, 2, 0)
    alias_row = name(alias) + struct.pack("<IhHBB", 0, 0, 0, 105, 1)
    alias_aux = struct.pack("<II", 0, 3) + b"\0" * 10
    header = struct.pack("<HHIIIHH", 0x14C, 0, 0, 20, 3, 0, 0)
    return header + target_row + alias_row + alias_aux + struct.pack("<I", 4)


def test_alias_source_accepts_only_noncontributing_weak_alias_directives() -> None:
    rows = ".386\n.model flat\nEXTERN _target:PROC\nALIAS <_alias> = <_target>\nEND\n"
    externs, aliases = parse_alias_source_text(rows, path=Path("unit.asm"))
    assert externs == {"_target"}
    assert aliases == (CoffAlias("_alias", "_target"),)
    with pytest.raises(ValueError, match="forbidden"):
        parse_alias_source_text(".code\nEND\n", path=Path("bad.asm"))


def test_alias_object_must_be_i386_zero_section_and_zero_contribution(tmp_path: Path) -> None:
    source = tmp_path / "unit.asm"
    source.write_text(
        ".386\n.model flat\nEXTERN _target:PROC\nALIAS <_alias> = <_target>\nEND\n",
        encoding="ascii",
    )
    obj = tmp_path / "unit.obj"
    obj.write_bytes(alias_object())
    report = validate_alias_object(
        obj,
        CoffAliasSource(source, (CoffAlias("_alias", "_target"),), Path("unit.cpp")),
    )
    assert report["validated"] is True
    assert report["section_count"] == 0
    assert report["contribution_bytes"] == 0


def test_vc5_parser_exposes_smoke_without_profile_matrix() -> None:
    parser = vc5_verify.build_parser()
    args = parser.parse_args(["--smoke", "--json", "--build-root", "scratch"])
    assert args.smoke is True and args.json is True
    assert "profile_matrix" not in vars(args)
