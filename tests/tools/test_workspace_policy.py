from __future__ import annotations

import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands.source_policy import POLICY_COMMANDS  # noqa: E402


EXPECTED_DOCS = {
    "compiler_linker_provenance.md",
    "messages_dll.md",
    "provider_abi_notes.md",
    "retail_executable_reproduction.md",
    "source_naming_conventions.md",
}
EXPECTED_SKILLS = {
    "recoil-binary-ninja-reconstruction",
    "recoil-binary-ninja-workflow",
    "recoil-progress-tracker",
    "recoil-provider-boundary",
    "recoil-source-model-recovery",
    "recoil-source-owner-scrutiny",
    "recoil-tier-verification",
    "recoil-tool-maintainer",
    "recoil-validation",
}


def test_workspace_has_one_compact_operational_document_set() -> None:
    docs = ROOT / "docs" / "reconstruction"
    assert {path.name for path in docs.glob("*.md")} == EXPECTED_DOCS
    assert not (ROOT / "tests" / "native").exists()
    assert not (ROOT / "tests" / "zbd_viewer").exists()
    assert not (ROOT / "tools" / "functional_verify_targets").exists()


def test_workspace_has_only_the_nine_direct_serial_skills() -> None:
    skill_root = ROOT / ".codex" / "skills"
    skills = {
        path.parent.name for path in skill_root.glob("recoil-*/SKILL.md")
    }
    assert skills == EXPECTED_SKILLS
    for name in skills:
        text = (skill_root / name / "agents" / "openai.yaml").read_text(encoding="utf-8")
        assert "default_prompt:" in text
        assert "role:" not in text


def test_parent_process_rule_does_not_join_unrelated_sentences() -> None:
    from _recoil.commands.agent_surface_audit import _retired_language_findings

    for text in (
        "Reject extra parent calls and malformed indices. The verifier reconciles the table.",
        "The parent function contains catch handlers; direct review checks their ownership.",
    ):
        assert not _retired_language_findings(text, location="generic audit")
    for text in (
        "The parent reviews the proposed acceptance.",
        "The parent agent must reconcile the result.",
        "The parent\naccepts the result.",
    ):
        assert any("[parent-process]" in item for item in
                   _retired_language_findings(text, location="generic audit"))


def test_vc5_manifests_are_json_and_have_no_retired_scaffold() -> None:
    manifests = sorted((ROOT / "tools" / "vc5_verify_targets").glob("*.json"))
    assert manifests
    for path in manifests:
        data = json.loads(path.read_text(encoding="utf-8"))
        assert "authored_order_scaffold" not in data


def test_source_policy_is_one_fail_fast_sequence_without_duplicate_routes(tmp_path) -> None:
    assert len(POLICY_COMMANDS) == len(set(POLICY_COMMANDS))
    assert POLICY_COMMANDS[-1] == ("audit", "provenance", "--strict")
    assert all(command[0] in {"guard", "audit"} for command in POLICY_COMMANDS)
    from _recoil.commands.raw_offset_guard import find_raw_offset_locations

    source = tmp_path / "Probe.cpp"
    source.write_text("\n".join([
        "char *p = (char *)(calloc(strlen(a) + strlen(b) + 0x1b, 1));",
        "char *q = (char *)(allocate(size(a) + 0x10));",
        "char *r = (char *)(object) + 0x10;",
        "char *s = (char *)(allocate(size(a))) + 0x10;",
        "take((char *)(object) + 0x10);",
        "char *t = (char *)((pointer()) + 0x10);",
        "char *u = (char *)(allocate((char *)(object) + 0x10));",
    ]))
    found = find_raw_offset_locations(tmp_path, tmp_path)
    assert {row.line_no for row in found} == {3, 4, 5, 6, 7}


def test_call_contract_surface_has_no_hash_or_normalizer_mechanism() -> None:
    paths = [
        ROOT / "tools" / "_recoil" / "commands" / "call_contract_verify.py",
        ROOT / "tools" / "_recoil" / "commands" / "progress_cli.py",
        ROOT / "tools" / "_recoil" / "lib" / "call_contract_generations.py",
    ]
    paths.extend((ROOT / "tools/_recoil/call_contract").glob("*.py"))
    text = "\n".join(path.read_text(encoding="utf-8") for path in paths)
    for token in ("hashlib", "sha256", "NORMALIZER_REGISTRY_GENERATION", "profile-matrix"):
        assert token not in text


def test_source_trace_spelling_checks_directory_names_before_publishing(tmp_path, monkeypatch) -> None:
    from types import SimpleNamespace
    import pytest
    from _recoil.lib import repository_paths
    from _recoil.lib.source_traceability import parse_source_trace_path
    source = tmp_path / "Probe.cpp"
    source.write_text("struct Probe {};", encoding="utf-8")
    assert parse_source_trace_path(source, repo_root=tmp_path).path == "Probe.cpp"
    # A filesystem spelling collision cannot become a source anchor identity.
    monkeypatch.setattr(repository_paths.os, "scandir", lambda path:
                        [SimpleNamespace(name="Probe.cpp"), SimpleNamespace(name="probe.cpp")])
    with pytest.raises(ValueError, match="unique current directory spelling"):
        parse_source_trace_path(source, repo_root=tmp_path)


def test_scheduler_requests_terminal_sized_call_contract_diagnostics() -> None:
    source = (ROOT / "tools" / "_recoil" / "lib" / "progress.py").read_text(
        encoding="utf-8"
    )
    assert 'f"--slice {slice_id} --build-root <fresh-root> --json --summary"' in source


def test_source_inventory_keeps_directive_continuations_out_of_function_signatures() -> None:
    from _recoil.lib.source_constructs import adjacent_comment, function_constructs

    source = (
        '#define EXPAND(owner, values) \\\n'
        '    APPEND_VALUES(owner, values)\n'
        '/** Original inline helper hypothesis; observed caller 0x1000. */\n'
        'inline void Apply(int value) { Consume(value); }\n'
        '#if ENABLED\n'
        'void Enabled() { Apply(1); }\n'
        '#endif\n'
        '// #define COMMENT_ONLY\n'
        'void AfterComment() {}\n'
        '#define DEFINE_FAKE() \\\n'
        '    void NotAFunctionHere() {}\n'
    )
    prefix = 'static const char *note = "continued\\\ntext";\n'
    for text in (source, source.replace('\n', '\r\n'), prefix + source,
                 (prefix + source).replace('\n', '\r\n')):
        functions = function_constructs(text)
        assert [item.name for item in functions] == ['Apply', 'Enabled', 'AfterComment']
        first = functions[0]
        assert first.start == text.index('inline void Apply')
        assert first.line == (6 if text.startswith('static') else 4)
        assert adjacent_comment(text, first.start).startswith('/** Original inline helper')
