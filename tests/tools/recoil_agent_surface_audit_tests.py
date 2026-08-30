from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands.agent_surface_audit import (  # noqa: E402
    HISTORICAL_BANNER,
    _retired_language_findings,
    audit_agent_surface,
)


VALID_AGENT_POINTER = """# AGENTS.md

> Compatibility pointer for tools that search this folder first.

`../AGENTS.md` is the sole authoritative instruction file.
Read it now and follow it verbatim.
This pointer adds no policy or procedure. Update `../AGENTS.md`, not this file.
"""


class AgentSurfaceAuditTests(unittest.TestCase):
    def test_live_surface_is_single_agent_and_mirrored(self) -> None:
        result = audit_agent_surface(ROOT)
        self.assertTrue(result["passed"], result["findings"])
        self.assertEqual(0, result["role_file_count"])
        self.assertEqual(result["canonical_skill_count"], result["mirror_skill_count"])
        self.assertGreater(result["canonical_skill_count"], 0)

    def test_contextual_matcher_allows_runtime_terms_and_explicit_prohibitions(self) -> None:
        allowed = (
            "There is no worker handoff, work packet, or linked worktree.",
            "The network packet assigns the player color.",
            "The parent node returns its only child.",
            "A worker thread validates provider I/O.",
            "The legacy dependency-frontier lane argument selects a comparison mode.",
            "There is no work-item collection.",
            "There are no work items.",
            "Do not create or restore work items.",
            "Work items do not exist.",
            "Work-item state has been removed.",
            (
                "There is no scheduler revision, work-item collection, "
                "packet/reservation schema, current-metadata cache, JSON backend, "
                "mirror, or export."
            ),
            (
                "There is no scheduler revision, work-item collection, packet, "
                "claim, lease, reservation, lane allocator, or generated-current cache."
            ),
            (
                "progress schema 6 has no work-item collection and issue schema 2 "
                "has no packet/reservation tables"
            ),
        )
        for text in allowed:
            with self.subTest(text=text):
                self.assertEqual(
                    [],
                    _retired_language_findings(text, location="unit.md"),
                )

    def test_contextual_matcher_rejects_roles_lanes_and_invalid_commands(self) -> None:
        rejected = {
            "Assign a source worker to the block.": "source-worker-role",
            "The parent reviews the worker result.": "parent-process",
            "There are no functional-lane blockers.": "functional-lane",
            "python tools/recoil.py verify functional --target demo --json": "invalid-command",
            "Inspect the assigned work item.": "structured-work-item",
            (
                "Companion work items remain structured, binary-qualified state."
            ): "structured-work-item",
            "Work-item views never select the next work.": "structured-work-item",
            "No work item may outrank the scheduler.": "structured-work-item",
            "Do not bypass the work item.": "structured-work-item",
        }
        for text, category in rejected.items():
            with self.subTest(text=text):
                findings = _retired_language_findings(text, location="unit.md")
                self.assertTrue(any(f"[{category}]" in finding for finding in findings), findings)

    def test_work_item_rejection_is_occurrence_scoped(self) -> None:
        cases = (
            "There is no lease; inspect the assigned work item.",
            "There is no work-item collection, but inspect the work item.",
        )
        for text in cases:
            with self.subTest(text=text):
                findings = _retired_language_findings(text, location="unit.md")
                self.assertTrue(
                    any(
                        "[structured-work-item]" in finding
                        and "'work item'" in finding
                        for finding in findings
                    ),
                    findings,
                )
        contrast_findings = _retired_language_findings(cases[1], location="unit.md")
        self.assertFalse(
            any(
                "[structured-work-item]" in finding
                and "'work-item'" in finding
                for finding in contrast_findings
            ),
            contrast_findings,
        )


class AgentSurfaceFixtureTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self._write("AGENTS.md", "There is no worker handoff.\n")
        self._write("CLAUDE.md", "There are no work packets.\n")
        self._write(".agent/AGENTS.md", VALID_AGENT_POINTER)
        self._write(
            "README.md",
            "\n".join(
                (
                    "# Unit",
                    "authored-function-order",
                    "authored-call-contract",
                    "authored-byte-match",
                    "full-function-order begins only after every authored call contract, its fresh closeout, and every authored byte group are current",
                    "linked-byte-match",
                    "final-validation",
                )
            ),
        )
        self._write("tools/README.md", "Direct serial commands.\n")
        self._write(
            "tools/functional_verify_targets/README.md",
            "Tracked governed functional manifests.\n",
        )
        self._write(
            "tools/vc5_verify_targets/README.md",
            "Tracked governed VC5 manifests.\n",
        )
        self._write(
            "tools/functional_verify_targets/network.json",
            '{"description": "The network packet assigns the player color."}\n',
        )
        self._write(
            "docs/reconstruction/retail_executable_reproduction.md",
            "# Current executable procedure\n",
        )
        self._write(
            "docs/reconstruction/final_executable_repro.md",
            "# Current final procedure\n",
        )
        self._write_historical(
            "docs/reconstruction/cryptographic_content_verification_removal.md",
            "docs/reconstruction/retail_executable_reproduction.md",
        )
        self._write_historical(
            "docs/reconstruction/final_executable_repro_history.md",
            "docs/reconstruction/final_executable_repro.md",
        )
        description = "Perform a bounded unit workspace audit."
        self._write(
            ".codex/skills/recoil-unit/SKILL.md",
            f"---\nname: recoil-unit\ndescription: {description}\n---\n\n# Unit\n",
        )
        self._write(
            ".codex/skills/recoil-unit/agents/openai.yaml",
            "interface:\n"
            '  display_name: "Recoil Unit"\n'
            '  short_description: "Audit one Recoil unit workspace"\n'
            '  default_prompt: "Use $recoil-unit to audit the unit workspace."\n',
        )
        self._write(
            ".claude/skills/recoil-unit/SKILL.md",
            f"---\nname: recoil-unit\ndescription: {description}\n---\n\n"
            "# Recoil Unit (Claude surface)\n\n"
            "Root `AGENTS.md` is authoritative. This stub adds no policy.\n\n"
            "The canonical procedure is `.codex/skills/recoil-unit/SKILL.md`.\n"
            "Read that file now and follow it verbatim.\n",
        )

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def _write(self, relative: str, text: str) -> None:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def _write_historical(self, relative: str, superseded_by: str) -> None:
        self._write(
            relative,
            "---\n"
            "document_status: historical-process-record\n"
            "operational_guidance: false\n"
            f"superseded_by: {superseded_by}\n"
            "---\n\n"
            f"{HISTORICAL_BANNER}\n\n"
            "A source worker once received a packet.\n"
            "A structured tracker work item once existed.\n",
        )

    def test_minimal_surface_passes_with_explicit_historical_records(self) -> None:
        result = audit_agent_surface(self.root)
        self.assertTrue(result["passed"], result["findings"])

    def test_agent_compatibility_pointer_is_scanned(self) -> None:
        self._write(
            ".agent/AGENTS.md",
            "# AGENTS.md\n\nIntegration operations are orchestrator-only.\n",
        )
        result = audit_agent_surface(self.root)
        self.assertFalse(result["passed"])
        self.assertTrue(
            any(
                ".agent/AGENTS.md" in finding
                and "[orchestrator-role]" in finding
                for finding in result["findings"]
            ),
            result["findings"],
        )

    def test_agent_compatibility_pointer_is_required(self) -> None:
        (self.root / ".agent/AGENTS.md").unlink()
        findings = audit_agent_surface(self.root)["findings"]
        self.assertTrue(
            any(
                "active agent surface file is missing: .agent/AGENTS.md" in finding
                for finding in findings
            ),
            findings,
        )

    def test_agent_compatibility_pointer_rejects_local_procedure(self) -> None:
        self._write(
            ".agent/AGENTS.md",
            VALID_AGENT_POINTER + "\nRun a local maintenance procedure here.\n",
        )
        findings = audit_agent_surface(self.root)["findings"]
        self.assertTrue(
            any(
                ".agent/AGENTS.md" in finding
                and "[compatibility-pointer]" in finding
                for finding in findings
            ),
            findings,
        )

    def test_agent_compatibility_pointer_normalizes_text_formatting(self) -> None:
        lines = VALID_AGENT_POINTER.rstrip("\n").split("\n")
        payload = "\r\n".join(
            f"{line}  " if line else line
            for line in lines
        ) + "\r\n   \r\n"
        (self.root / ".agent/AGENTS.md").write_bytes(payload.encode("utf-8"))
        result = audit_agent_surface(self.root)
        self.assertTrue(result["passed"], result["findings"])

    def test_historical_record_requires_visible_banner(self) -> None:
        path = (
            self.root
            / "docs/reconstruction/cryptographic_content_verification_removal.md"
        )
        path.write_text(
            path.read_text(encoding="utf-8").replace(HISTORICAL_BANNER, ""),
            encoding="utf-8",
        )
        findings = audit_agent_surface(self.root)["findings"]
        self.assertTrue(any("[historical-banner]" in finding for finding in findings))

    def test_json_findings_name_the_pointer(self) -> None:
        self._write(
            "tools/functional_verify_targets/stale.json",
            '{"description": "Use a source-worker slice."}\n',
        )
        findings = audit_agent_surface(self.root)["findings"]
        self.assertTrue(
            any("stale.json#/description" in finding for finding in findings),
            findings,
        )

    def test_thick_mirror_is_rejected(self) -> None:
        path = self.root / ".claude/skills/recoil-unit/SKILL.md"
        path.write_text(
            path.read_text(encoding="utf-8")
            + "\n```powershell\npython tools/recoil.py progress next\n```\n",
            encoding="utf-8",
        )
        findings = audit_agent_surface(self.root)["findings"]
        self.assertTrue(any("[thick-mirror]" in finding for finding in findings))


if __name__ == "__main__":
    unittest.main()
