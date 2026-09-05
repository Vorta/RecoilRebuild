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


def test_lifecycle_icf_requires_every_fresh_comdat_and_exact_relocation_semantics() -> None:
    from types import SimpleNamespace as Row
    from _recoil.lib.authored_icf import validate_lifecycle_object_members
    from _recoil.lib.progress import ProgressError

    names = ("??1First@@UAE@XZ", "??1Second@@UAE@XZ")
    definitions = [Row(name=name, index=index, section_number=index, storage_class=2, type=0x20,
                       section_definition_selection=None, section_definition_association=None)
                   for index, name in enumerate(names, 1)]
    sections = [Row(name=".text", section_number=index, storage_class=3, type=0,
                    section_definition_selection=2, section_definition_association=0)
                for index in (1, 2)]
    data = b"\xc7\x01\0\0\0\0\xc3"
    relocation = Row(offset=2, type=6, symbol_name="??_7Root@@6B@")
    bodies = {name: Row(start=0, end=7, data=data, relocations=(relocation,)) for name in names}
    obj = Row(symbols=definitions + sections, function_bytes=bodies.__getitem__, relocations_by_section={},
              section=lambda index: Row(index=index, name=".text", characteristics=0x1020, raw_data=data))
    validate_lifecycle_object_members(obj, names)
    for mutation in ("missing", "different-bytes", "different-target", "different-addend", "association", "selection"):
        from copy import deepcopy
        bad = deepcopy(obj)
        bad_bodies = deepcopy(bodies)
        bad.function_bytes = bad_bodies.__getitem__
        if mutation == "missing":
            bad.symbols.pop(1)
        elif mutation == "different-bytes":
            bad_bodies[names[1]].data = b"\xc7\x02\0\0\0\0\xc3"
        elif mutation == "different-target":
            bad_bodies[names[1]].relocations = (Row(offset=2, type=6, symbol_name="??_7Other@@6B@"),)
        elif mutation == "different-addend":
            bad_bodies[names[1]].data = b"\xc7\x01\x04\0\0\0\xc3"
        elif mutation == "association":
            bad.symbols.append(Row(name=".xdata", section_number=3, storage_class=3, type=0,
                                  section_definition_selection=5, section_definition_association=2))
        else:
            bad.symbols[-1].section_definition_selection = 1
        with pytest.raises(ProgressError):
            validate_lifecycle_object_members(bad, names)
    with pytest.raises(ProgressError, match="fresh"):
        validate_lifecycle_object_members(None, names)

    # FPO records must differ only in their exact self-reference relocation.
    from copy import deepcopy
    fpo_obj = deepcopy(obj)
    for index in (1, 2):
        fpo_obj.symbols.append(Row(name=".debug$F", section_number=index + 2, storage_class=3, type=0,
                                  section_definition_selection=5, section_definition_association=index))
        fpo_obj.relocations_by_section[index + 2] = (Row(offset=0, type=7, symbol_name=names[index - 1], symbol_index=index),)
    fpo_obj.section = lambda index: (obj.section(index) if index <= 2 else
                                    Row(index=index, name=".debug$F", characteristics=0x42101048, raw_data=b"\0" * 16))
    validate_lifecycle_object_members(fpo_obj, names)
    fpo_obj.relocations_by_section[4][0].symbol_name = names[0]
    with pytest.raises(ProgressError, match="FPO"):
        validate_lifecycle_object_members(fpo_obj, names)


def test_lifecycle_icf_header_mirrors_bind_generated_and_explicit_destructors(tmp_path: Path) -> None:
    from _recoil.lib.authored_icf import validate_authored_icf_source_mirrors
    from _recoil.lib.progress import ProgressError

    source = tmp_path / "src"
    source.mkdir()
    (source / "unit.cpp").write_text('#include "unit.h"\n', encoding="utf-8")
    header = source / "unit.h"
    identity = "recoil:logical-function:0x401000:unit"
    anchor = "recoil:anchor:unit"
    alias = {
        "object_symbol": "??1Unit@@UAE@XZ",
        "source_traceability": {"source_edges": [{
            "relation": "emits", "anchor_id": anchor,
            "emission_context": {"translation_unit": "src/unit.h"},
        }]},
        "source_generation": {"kind": "implicit-destructor", "class_name": "Unit",
                              "translation_units": ["src/unit.cpp"]},
    }
    comment = f"/**\n * @recoil-anchor {anchor}\n * @recoil-artifact emits .text {identity}: Generated lifetime.\n */\n"
    header.write_text(comment + "struct Unit { virtual void Method(); };\n", encoding="utf-8")
    assert validate_authored_icf_source_mirrors({identity: alias}, repo_root=tmp_path) == {identity: "src/unit.h"}
    for declaration in ("~Unit();", "virtual ~Unit() {}"):
        header.write_text(comment + "struct Unit { " + declaration + " };\n", encoding="utf-8")
        with pytest.raises(ProgressError, match="destructor"):
            validate_authored_icf_source_mirrors({identity: alias}, repo_root=tmp_path)
    alias["source_generation"]["kind"] = "inline-destructor"
    alias["source_traceability"]["source_edges"][0]["relation"] = "defines"
    header.write_text("struct Unit {\n" + comment.replace("emits", "defines") + "virtual ~Unit() {}\n};\n", encoding="utf-8")
    assert validate_authored_icf_source_mirrors({identity: alias}, repo_root=tmp_path)
    alias["source_generation"]["class_name"] = "Other"
    with pytest.raises(ProgressError, match="bind"):
        validate_authored_icf_source_mirrors({identity: alias}, repo_root=tmp_path)


def test_lifecycle_icf_projection_requires_explicit_witness_and_checks_whole_tu(monkeypatch: pytest.MonkeyPatch) -> None:
    from _recoil.lib import authored_icf as icf
    from _recoil.lib.progress import ProgressError

    physical = "recoil:function:0x401000"
    aliases = {"first": {"object_symbol": "??1First@@UAE@XZ"},
               "second": {"object_symbol": "??1Second@@UAE@XZ"}}
    for name, alias in aliases.items():
        alias["source_generation"] = {"translation_units": ["src/unit.cpp"]}
        alias["source_traceability"] = {"source_edges": [{"relation": "emits", "emission_context": {"translation_unit": "src/unit.h"}}]}
    group = {"model": icf.AUTHORED_ICF_GROUP_MODEL, "source_model": icf.AUTHORED_ICF_LIFECYCLE_SOURCE_MODEL}
    data = {"symbols": {physical: {"icf_address_group": group, "logical_aliases": aliases}}}
    monkeypatch.setattr(icf, "audit_authored_icf_groups", lambda data: [])
    checked = []
    monkeypatch.setattr(icf, "validate_lifecycle_object_members", lambda obj, names: checked.append((obj, names)))
    selector = icf.select_authored_icf_translation_unit_object_symbol
    for witness in (None, "??1Absent@@UAE@XZ"):
        with pytest.raises(ProgressError, match="witness"):
            selector(data, physical_symbol_id=physical, translation_unit="src/unit.cpp", object_witness=witness)
    assert selector(data, physical_symbol_id=physical, translation_unit="src/unit.cpp",
                    object_witness=aliases["second"]["object_symbol"], coff_object="fresh") == ("second", aliases["second"]["object_symbol"])
    assert checked == [("fresh", tuple(alias["object_symbol"] for alias in aliases.values()))]
    with pytest.raises(ProgressError):
        selector(data, physical_symbol_id=physical, translation_unit="src/other.cpp",
                 object_witness=aliases["second"]["object_symbol"], coff_object="fresh")


def test_coff_lifecycle_cli_reports_map_observations_without_acceptance(monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    import json
    from types import SimpleNamespace as Row
    from _recoil.commands import coff_lifecycle

    monkeypatch.setattr(coff_lifecycle, "configure_stdio", lambda: None)
    monkeypatch.setattr(coff_lifecycle, "inventory", lambda path, names: {"acceptance": False, "members": names})
    monkeypatch.setattr(coff_lifecycle, "parse_link_map", lambda path: Row(symbols=(
        Row(symbol="_unit", address=0x402000, is_function=True),
        Row(symbol="_unit", address=0x403000, is_function=False),
    )))
    monkeypatch.setattr(sys, "argv", ["coff-lifecycle", "--object", "unit.obj", "--symbol", "_unit", "--map", "unit.map"])
    assert coff_lifecycle.main() == 0
    report = json.loads(capsys.readouterr().out)
    assert report["acceptance"] is False
    assert report["maps"] == {"unit.map": {"_unit": ["0x402000"]}}


def test_linked_presence_rejects_missing_data_and_ambiguous_function_symbols() -> None:
    from dataclasses import replace
    from _recoil.commands.vc5_build import (
        LinkedMapSymbol, ParsedLinkMap, authored_linked_presence_report,
    )
    from _recoil.commands.vc5_verify import VerifyFunction

    required = (VerifyFunction("0x401000", "_entry", "entry"),)

    def symbol(address: int, name: str = "_entry", function: bool = True) -> LinkedMapSymbol:
        return LinkedMapSymbol(1, 0, name, address, ("f",) if function else (), "unit.obj", "Publics by Value")

    # Different RVA and aliases at the same RVA are valid for presence alone.
    present = ParsedLinkMap(0x400000, (symbol(0x405000), symbol(0x405000, "_alias")))
    assert authored_linked_presence_report(required, present)["passed"]
    patterned = (replace(required[0], symbol="_old_spelling", symbol_regex="_entry"),)
    assert authored_linked_presence_report(patterned, present)["passed"]
    assert not authored_linked_presence_report(
        (replace(required[0], symbol_regex="_other"),), present
    )["passed"]
    local_a = replace(symbol(0x405000), source="Static symbols")
    local_b = replace(local_a, address=0x406000, object="other.obj")
    locals_map = ParsedLinkMap(0x400000, (local_a, local_b))
    assert not authored_linked_presence_report(required, locals_map)["passed"]
    scope = {"0x401000": {"unit.obj"}}
    assert authored_linked_presence_report(required, locals_map, static_object_scopes=scope)["passed"]
    assert not authored_linked_presence_report(
        required, ParsedLinkMap(0x400000, (local_b,)), static_object_scopes=scope
    )["passed"]
    assert not authored_linked_presence_report(
        required, ParsedLinkMap(0x400000, (symbol(0x405000), symbol(0x406000))),
        static_object_scopes=scope,
    )["passed"]
    for symbols in ((), (symbol(0x405000, function=False),), (symbol(0x405000), symbol(0x406000))):
        report = authored_linked_presence_report(required, ParsedLinkMap(0x400000, symbols))
        assert not report["passed"]
        assert report["divergences"][0]["symbol_id"] == "recoil:function:0x401000"
    with pytest.raises(ValueError, match="nonempty"):
        authored_linked_presence_report((), present)


def test_linked_presence_census_rejects_stale_or_uncovered_authority(monkeypatch: pytest.MonkeyPatch) -> None:
    from dataclasses import replace
    from types import SimpleNamespace
    from _recoil.commands import vc5_build
    from _recoil.lib import verification_targets

    function = vc5_verify.VerifyFunction("0x401000", "_entry", "entry", pipeline_class="authored", authored_order_role="authored-body")
    registration = {"manifest_path": "tools/unit.json"}
    current = {"registration": dict(registration)}
    target = SimpleNamespace(functions=(function,), translation_unit_function_order=(), linked_function_intervals=())
    document = SimpleNamespace(
        authored_call_contract_slices=lambda binary: [{"addresses": [function.address], "target_ids": ["unit"]}],
        collection=lambda name: {"unit": {"registration": registration}},
    )
    monkeypatch.setattr(vc5_build, "load_repository_path_inventory", lambda root: None)
    monkeypatch.setattr(vc5_build, "resolve_repository_file", lambda *args, **kwargs: SimpleNamespace(physical_path=Path("unit.json")))
    monkeypatch.setattr(vc5_build, "load_vc5_verify_manifest", lambda path: target)
    monkeypatch.setattr(verification_targets, "vc5_target_registration", lambda path: ("unit", current))
    assert vc5_build.required_authored_linked_functions(document) == (function,)
    # The accepted census, not a second manifest-role filter, owns membership.
    # Compiler lifecycle helpers can remain explicit census obligations.
    lifecycle = replace(function, pipeline_class="non-authored", authored_order_role="non-authored")
    outside = replace(function, address="0x401020", symbol="_outside")
    target.functions = (lifecycle, outside)
    assert vc5_build.required_authored_linked_functions(document) == (lifecycle,)
    target.functions = (replace(lifecycle, required_presence=False),)
    with pytest.raises(ValueError, match="required presence"):
        vc5_build.required_authored_linked_functions(document)
    target.functions = (function,)
    current["registration"] = {"manifest_path": "tools/stale.json"}
    with pytest.raises(ValueError, match="stale"):
        vc5_build.required_authored_linked_functions(document)
    current["registration"] = dict(registration)
    target.functions = ()
    with pytest.raises(ValueError, match="uncovered"):
        vc5_build.required_authored_linked_functions(document)
    target.functions = (replace(function, required_presence=False),)
    with pytest.raises(ValueError, match="required presence"):
        vc5_build.required_authored_linked_functions(document)


def test_playground_request_rejects_reuse_profiles_and_partial_modes(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    from types import SimpleNamespace as Row
    from _recoil.commands import vc5_build as build

    monkeypatch.setattr(build, "REPO_ROOT", tmp_path)
    config = Row(
        manifest_path=build.DEFAULT_MANIFEST, output_exe="Recoil.exe",
        playtest_output_exe=build.DEFAULT_PLAYTEST_OUTPUT, diagnostic_only=False,
        diagnostic_kind="", compile_profile="", link_profile="", library_profile="",
        build_dir=tmp_path / "build/live-validation/fresh", build_dir_explicit=True,
    )
    options = dict(clean=False, compile_only=False, linked_order_only=False,
                   linkability_only=False, keep_going=False, order_targets=(),
                   progress_path=build.DEFAULT_PROGRESS)
    build.validate_playground_build_request(config, **options)
    for name, value in (("clean", True), ("compile_only", True), ("linked_order_only", True),
                        ("linkability_only", True), ("keep_going", True),
                        ("order_targets", ("unit",)), ("progress_path", tmp_path / "other.db")):
        with pytest.raises(ValueError):
            build.validate_playground_build_request(config, **{**options, name: value})
    for name, value in (("diagnostic_only", True), ("compile_profile", "probe"),
                        ("link_profile", "probe"), ("library_profile", "probe"),
                        ("build_dir_explicit", False), ("output_exe", "messages.dll"),
                        ("playtest_output_exe", tmp_path / "elsewhere.exe"),
                        ("build_dir", tmp_path / "outside"), ("build_dir", tmp_path / "build/live-validation")):
        with pytest.raises(ValueError):
            build.validate_playground_build_request(Row(**{**vars(config), name: value}), **options)
    config.build_dir.mkdir(parents=True)
    with pytest.raises(ValueError, match="fresh absent"):
        build.validate_playground_build_request(config, **options)


def test_playground_completion_requires_presence_and_deployment_without_acceptance(monkeypatch: pytest.MonkeyPatch) -> None:
    from types import SimpleNamespace as Row
    from _recoil.commands import vc5_build as build

    config = Row(playtest_output_exe=Path("playground/candidate.exe"))
    paths = Row(map_path=Path("fresh/candidate.map"), exe_path=Path("fresh/candidate.exe"))
    reports, deployments = [], []
    presence, deployed = {"passed": False}, {"attempted": True, "updated": True}
    monkeypatch.setattr(build, "required_authored_presence_at_map", lambda *args: presence)
    monkeypatch.setattr(build, "compile_profile_rows", lambda config: [])
    monkeypatch.setattr(build, "write_summary", lambda *args, **kwargs: reports.append(kwargs["acceptance"]))
    monkeypatch.setattr(build, "deploy_playtest_candidate", lambda *args: deployments.append(args) or deployed)
    kwargs = dict(progress_path=Path("canonical.db"), required_order_targets=("later-order",), canonical_include_trace=None)
    assert build.finish_playground_build(config, paths, [], **kwargs) == 1
    assert not deployments and reports[-1]["failure_stage"] == "linked-presence"
    presence["passed"] = True
    deployed["updated"] = False
    assert build.finish_playground_build(config, paths, [], **kwargs) == 1
    assert reports[-1]["failure_stage"] == "deployment"
    deployed["updated"] = True
    assert build.finish_playground_build(config, paths, [], **kwargs) == 0
    report = reports[-1]
    assert report["success"] and report["kind"] == "playground-build"
    assert not any(report[key] for key in ("accepts_order", "accepts_bytes", "accepts_final_image", "required_order_targets_passed"))
    assert report["effective_order_targets"] == [] and report["order_reports"] == []


def test_playground_parser_is_explicit_and_presence_authority_errors_fail_closed(monkeypatch: pytest.MonkeyPatch) -> None:
    from _recoil.commands import vc5_build as build

    assert not build.build_parser().parse_args([]).playground_only
    assert build.build_parser().parse_args(["--playground-only"]).playground_only
    assert not build.build_parser().parse_args(["--linkability-only"]).playground_only
    def stale(path: Path) -> None:
        raise ValueError("stale census")
    monkeypatch.setattr(build.ProgressDocument, "load", stale)
    assert build.required_authored_presence_at_map(Path("candidate.map"), Path("canonical.db")) == {
        "passed": False, "error": "stale census",
    }
