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


def test_vc5_manifests_are_json_and_have_no_retired_scaffold() -> None:
    manifests = sorted((ROOT / "tools" / "vc5_verify_targets").glob("*.json"))
    assert manifests
    for path in manifests:
        data = json.loads(path.read_text(encoding="utf-8"))
        assert "authored_order_scaffold" not in data


def test_source_policy_is_one_fail_fast_sequence_without_duplicate_routes() -> None:
    assert len(POLICY_COMMANDS) == len(set(POLICY_COMMANDS))
    assert POLICY_COMMANDS[-1] == ("audit", "provenance", "--strict")
    assert all(command[0] in {"guard", "audit"} for command in POLICY_COMMANDS)


def test_call_contract_surface_has_no_hash_or_normalizer_mechanism() -> None:
    paths = [
        ROOT / "tools" / "_recoil" / "commands" / "call_contract_verify.py",
        ROOT / "tools" / "_recoil" / "commands" / "progress_cli.py",
        ROOT / "tools" / "_recoil" / "lib" / "call_contract_generations.py",
    ]
    text = "\n".join(path.read_text(encoding="utf-8") for path in paths)
    for token in ("hashlib", "sha256", "NORMALIZER_REGISTRY_GENERATION", "profile-matrix"):
        assert token not in text


def test_scheduler_requests_terminal_sized_call_contract_diagnostics() -> None:
    source = (ROOT / "tools" / "_recoil" / "lib" / "progress.py").read_text(
        encoding="utf-8"
    )
    assert 'f"--slice {slice_id} --build-root <fresh-root> --json --summary"' in source
