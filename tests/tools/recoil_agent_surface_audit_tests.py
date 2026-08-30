from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands.agent_surface_audit import (  # noqa: E402
    CLAUDE_REQUIRED_DENY_RULES,
    CLAUDE_STUB_MAX_LINES,
    PROVIDER_TARGET_REGISTRATION_REQUIREMENTS,
    audit_claude_mirror,
    audit_required_policy_phrases,
    audit_git_policy,
    audit_start_contract_policy,
    audit_text,
    audit_tool_readme_index,
    find_active_durable_devspace_references,
    has_required_phrase,
    is_known_gate_invocation,
    parse_frontmatter_scalars,
    split_command_args,
)
from _recoil.commands.live_validation_surface_audit import audit_paths  # noqa: E402
from _recoil.lib.progress import empty_progress_document  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402
import recoil  # noqa: E402


class AgentSurfaceAuditTests(unittest.TestCase):
    def test_generation12_and_pre_fast_forward_validation_policy_are_current(self) -> None:
        coordinate_surfaces = (
            "AGENTS.md",
            "tools/README.md",
            ".codex/skills/recoil-progress-tracker/SKILL.md",
            ".codex/skills/recoil-tool-maintainer/SKILL.md",
            ".codex/skills/recoil-validation/SKILL.md",
            ".codex/skills/recoil-workspace-audit/SKILL.md",
        )
        required_coordinates = (
            "CALL_CONTRACT_VERIFIER_GENERATION = 12",
            "NORMALIZER_REGISTRY_GENERATION = 12",
            "EXPECTED_FACT_SCHEMA_VERSION = 12",
        )
        for relative in coordinate_surfaces:
            with self.subTest(surface=relative):
                text = (REPO_ROOT / relative).read_text(encoding="utf-8")
                for coordinate in required_coordinates:
                    self.assertIn(coordinate, text)

        validation = (
            REPO_ROOT / ".codex/skills/recoil-validation/SKILL.md"
        ).read_text(encoding="utf-8")
        self.assertIn("before `master` advances", validation)
        self.assertIn("deterministic Git, topology, tag, and physical-identity", validation)
        self.assertIn("<canonical-control-root>/support/Recoil.exe", validation)
        self.assertIn("`.agent/REFERENCE_EXECUTABLE.json`", validation)
        self.assertIn("<canonical-control-root>/.agent/<live-database>", validation)
        self.assertNotIn("and again on canonical `master`", validation)

    def test_explicit_packet_validation_uses_exact_public_registry_and_parser(self) -> None:
        resolved = recoil.validate_nonmutating_public_command(
            ["audit", "workflow-contracts", "--strict"]
        )
        self.assertEqual("audit workflow-contracts", resolved["command"])
        with self.assertRaisesRegex(ValueError, "unknown public validation command"):
            recoil.validate_nonmutating_public_command(
                ["verify", "definitely-not-a-command"]
            )
        with self.assertRaisesRegex(ValueError, "mutates authoritative"):
            recoil.validate_nonmutating_public_command(["verify", "final-image"])
        with self.assertRaisesRegex(ValueError, "required packet resources"):
            recoil.validate_nonmutating_public_command(
                [
                    "verify",
                    "call-contract",
                    "--slice",
                    "recoil:call-contract-slice:test",
                    "--build-root",
                    "build/test",
                ]
            )
        resolved = recoil.validate_nonmutating_public_command(
            [
                "verify",
                "call-contract",
                "--slice",
                "recoil:call-contract-slice:test",
                "--build-root",
                "build/test",
            ],
            resource_claims=[
                {
                    "kind": "binary-ninja-db",
                    "id": "Recoil.bndb",
                    "access": "read",
                }
            ],
        )
        self.assertTrue(resolved["needs_binja"])

    def test_provider_target_registration_policy_is_guarded(self) -> None:
        surface = ".codex/skills/recoil-provider-boundary/SKILL.md"
        required = PROVIDER_TARGET_REGISTRATION_REQUIREMENTS[surface]
        complete = "\n".join(required)
        self.assertEqual(
            [],
            audit_required_policy_phrases(
                complete,
                surface,
                PROVIDER_TARGET_REGISTRATION_REQUIREMENTS,
                "provider-target-registration-policy",
                "unit remediation",
            ),
        )
        findings = audit_required_policy_phrases(
            complete.replace("never candidate output", "candidate output"),
            surface,
            PROVIDER_TARGET_REGISTRATION_REQUIREMENTS,
            "provider-target-registration-policy",
            "unit remediation",
        )
        self.assertEqual(1, len(findings))
        self.assertEqual("provider-target-registration-policy", findings[0].kind)

    def write_tracker(self, root: Path, payload: dict[str, object]) -> None:
        tracker_path = root / ".agent" / "RECONSTRUCTION_PROGRESS.json"
        tracker_path.parent.mkdir(parents=True)
        tracker_path.write_text(json.dumps(payload), encoding="utf-8")

    def test_ordinary_commit_language_is_not_git_policy(self) -> None:
        findings = audit_git_policy(
            "The command commits the reviewed semantic revision.\n",
            "example.md",
        )
        self.assertEqual([], findings)

    def test_policy_reference_matching_ignores_case_and_wrapped_whitespace(self) -> None:
        self.assertTrue(
            has_required_phrase(
                "A REAL active\nreservation is required.",
                "real active reservation",
            )
        )

    def test_explicit_git_and_staging_instructions_remain_rejected(self) -> None:
        for text in ("Run git commit now.\n", "Stage these files before handoff.\n"):
            with self.subTest(text=text):
                findings = audit_git_policy(text, "example.md")
                self.assertEqual(1, len(findings))
                self.assertEqual("git-policy", findings[0].kind)

    def test_governed_native_git_policy_is_accepted(self) -> None:
        text = """
Git is the authored workspace change-control mechanism. Read-only Git status,
diff, path inventory, ignore inspection, and history are routine.

Parent/tool governance may create and switch to a reviewed packet branch,
stage exact reviewed paths, and create an explicitly nonaccepting packet
commit. After validation, the parent reviews and merges or cherry-picks it.

Workers edit only their handed-off writable closure. Workers do not stage,
commit, branch, merge, cherry-pick, reset, clean, restore, stash, rebase, push,
or otherwise control Git history. Destructive primary-worktree operations are
prohibited and pre-existing user changes must not be discarded.

Git never supplies retail expected truth, and commits never accept
reconstruction state. Git object IDs are opaque repository state. SQLite
revisions and CAS govern packet and reconstruction state; retail, Binary Ninja,
and direct comparison supply expected facts and semantic or byte verification.

Git governs maintained authored inputs. Ignored paths are generated or
machine-local and are nonauthoritative; ignored generated-file churn is not
packet-closeout evidence. Validation and build output should normally use
external or isolated roots. Unresolved Git state is an unconditional blocker.
"""
        self.assertEqual([], audit_git_policy(text, "example.md"))

    def test_exact_one_packet_commit_worker_policy_is_accepted(self) -> None:
        text = """
The orchestrator owns branch and linked-worktree creation, switching,
integration, retirement, and master. The worker edits and may stage only the
exact handed-off writable closure. The worker may create exactly one
nonaccepting packet commit, and the commit message contains the exact packet id.
The worker may not create, switch, merge, rebase, or delete branches or
worktrees and may not modify master or remove the external build root.
"""
        self.assertEqual([], audit_git_policy(text, "example.md"))

    def test_current_native_git_surfaces_satisfy_semantic_policy_groups(self) -> None:
        for relative in (
            "AGENTS.md",
            "tools/README.md",
            "docs/reconstruction/retail_executable_reproduction.md",
        ):
            with self.subTest(relative=relative):
                text = (REPO_ROOT / relative).read_text(encoding="utf-8")
                self.assertEqual([], audit_git_policy(text, relative))

    def test_unsafe_git_guidance_is_rejected(self) -> None:
        cases = {
            "worker-commit": "Worker may commit anything.",
            "worker-outside-stage": "Worker may stage files outside its closure.",
            "reset-hard": "Use git reset --hard to recover.",
            "clean": "Use git clean to remove unknown files.",
            "restore": "Use git restore to discard user work.",
            "checkout": "Use checkout to discard user work.",
            "stash": "Hide dirty work with git stash.",
            "rebase": "Rebase governed history after validation.",
            "force-push": "Force-push governed history.",
            "hash-object": "Use git hash-object for expected facts.",
            "retail-equivalence": "A packet commit proves retail equivalence.",
            "call-contract-acceptance": "A packet commit accepts call contracts.",
            "ignored-authored-tools": "Ignore maintained authored tools and tests.",
            "ignored-retail-truth": "Use ignored artifacts as retail expected truth.",
        }
        for name, text in cases.items():
            with self.subTest(name=name):
                findings = audit_git_policy(text, "example.md")
                self.assertTrue(findings)
                self.assertTrue(all(row.kind == "git-policy" for row in findings))

    def test_negative_git_prohibitions_are_not_reported_as_permissions(self) -> None:
        text = """
Do not run git reset --hard.
Git clean is forbidden.
Workers must not stage or commit files.
Never use git hash-object as expected-fact identity.
"""
        self.assertEqual([], audit_git_policy(text, "example.md"))

    def test_historical_non_gating_tracker_observation_is_not_active_dependency(self) -> None:
        reference = ".devspace/runs/old-session/receipt.json"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.write_tracker(
                root,
                {
                    "evidence": {
                        "evidence:old": {
                            "gating": False,
                            "freshness": "historical",
                            "validation_mode": "historical-observation",
                            "former_path": reference,
                        }
                    },
                    "tombstones": {},
                },
            )
            findings = find_active_durable_devspace_references(root)
        self.assertEqual([], findings)

    def test_current_tracker_dependency_remains_rejected(self) -> None:
        reference = ".devspace/runs/current-session/receipt.json"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.write_tracker(
                root,
                {
                    "binaries": {"recoil": {"current_validation_path": reference}},
                    "evidence": {},
                    "tombstones": {},
                },
            )
            findings = find_active_durable_devspace_references(root)
        self.assertEqual(1, len(findings))
        self.assertEqual(reference, findings[0].reference)

    def test_current_sqlite_tracker_dependency_is_read_semantically(self) -> None:
        reference = ".devspace/runs/current-sqlite-session/receipt.json"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tracker_path = root / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
            tracker_path.parent.mkdir(parents=True)
            payload = empty_progress_document()
            payload["binaries"]["recoil"] = {"current_validation_path": reference}
            ProgressSQLiteStore.create_from_mapping(
                tracker_path,
                payload,
                cutover_pair_id="agent-surface-test",
            )
            findings = find_active_durable_devspace_references(root)
        self.assertEqual(1, len(findings))
        self.assertEqual(0, findings[0].line)
        self.assertEqual(reference, findings[0].reference)

    def test_historical_label_does_not_exempt_gating_evidence(self) -> None:
        reference = ".devspace/runs/gating-session/receipt.json"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.write_tracker(
                root,
                {
                    "evidence": {
                        "evidence:gating": {
                            "gating": True,
                            "freshness": "historical",
                            "validation_mode": "historical-observation",
                            "path": reference,
                        }
                    },
                    "tombstones": {},
                },
            )
            findings = find_active_durable_devspace_references(root)
        self.assertEqual(1, len(findings))
        self.assertEqual(reference, findings[0].reference)

    def test_readme_command_index_matches_registry(self) -> None:
        findings = audit_tool_readme_index(REPO_ROOT / "tools" / "README.md", REPO_ROOT)
        self.assertEqual([], findings)

    def test_primary_only_bare_start_guidance_is_rejected(self) -> None:
        findings = audit_start_contract_policy(
            "With no explicit target, claim with --lane primary and wait.\n",
            "AGENTS.md",
        )
        self.assertIn("stale-primary-only-start", {finding.kind for finding in findings})

    def test_placeholder_never_excuses_an_unregistered_command_prefix(self) -> None:
        raw = "audit vc5-comdat --run <diagnostic-root> --json"
        self.assertFalse(
            is_known_gate_invocation(split_command_args(raw), raw=raw)
        )
        findings = audit_text(
            f"python tools/recoil.py {raw}\n",
            "tools/README.md",
        )
        self.assertIn("unknown-gate-command", {finding.kind for finding in findings})

    def test_registered_command_and_group_prefixes_accept_placeholder_args(self) -> None:
        examples = (
            "progress handoff --packet-id <packet-id> --json",
            "progress show <selector>",
            "progress owner <operation> ... --dry-run",
        )
        for raw in examples:
            with self.subTest(raw=raw):
                self.assertTrue(
                    is_known_gate_invocation(split_command_args(raw), raw=raw)
                )

    def test_reviewed_migration_census_is_historical_not_a_live_invariant(self) -> None:
        accepted = (
            "The reviewed one-time migration census was 3,380 bodies; the live census "
            "is derived from accepted authored-order gating rows.\n"
        )
        self.assertNotIn(
            "permanent-call-contract-census",
            {finding.kind for finding in audit_text(accepted, "AGENTS.md")},
        )
        rejected = (
            "The permanent authored-call-contract census is exactly 3,380 bodies.\n"
        )
        self.assertIn(
            "permanent-call-contract-census",
            {finding.kind for finding in audit_text(rejected, "AGENTS.md")},
        )

    def test_stale_3370_call_contract_census_is_rejected(self) -> None:
        findings = audit_text(
            "The live authored-call-contract census is 3,370 bodies.\n",
            "AGENTS.md",
        )
        self.assertIn(
            "stale-call-contract-census",
            {finding.kind for finding in findings},
        )

    def test_generated_readme_progress_may_project_the_live_dynamic_census(self) -> None:
        findings = audit_text(
            "<!-- RECOIL_PROGRESS:START -->\n"
            "Current call-contract census: 3370\n"
            "<!-- RECOIL_PROGRESS:END -->\n",
            "README.md",
        )
        self.assertNotIn(
            "stale-call-contract-census",
            {finding.kind for finding in findings},
        )

    def test_retired_owner_mutation_routes_to_current_governed_commands(self) -> None:
        cases = (
            (
                "python tools/recoil.py progress owner set-address-meta "
                "<owner> 0xNNNNNN --target <target>\n",
                "progress verification-target sync",
            ),
            (
                "python tools/recoil.py progress owner set-gates <owner> "
                "source=accepted --expected-revision <revision> --dry-run\n",
                "workspace-issue candidate",
            ),
            (
                "python tools/recoil.py progress owner remove <owner> "
                "--expected-revision <revision> --dry-run\n",
                "progress owner replace-batch",
            ),
        )
        for text, remediation in cases:
            with self.subTest(text=text):
                findings = [
                    finding
                    for finding in audit_text(text, "AGENTS.md")
                    if finding.kind == "retired-owner-mutation"
                ]
                self.assertEqual(1, len(findings))
                self.assertIn(remediation, findings[0].suggestion)

    def test_cryptographic_and_retired_validation_mechanisms_stay_rejected(self) -> None:
        source_rows = (
            "import ha" + "shlib",
            "value.hexdi" + "gest()",
            "mer" + "kle = True",
            "finger" + "print = True",
            "content_" + "hash = True",
            "tree_" + "hash = True",
            "body_" + "outcomes = True",
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "active_validation.py"
            path.write_text("\n".join(source_rows), encoding="utf-8")
            findings = audit_paths([path])
        self.assertEqual(7, len(findings))
        self.assertEqual(set(range(1, 8)), {finding.line for finding in findings})


class ClaudeMirrorAuditTests(unittest.TestCase):
    """The Claude pointer surface must stay a complete, thin mirror of `.codex`."""

    SKILL = "recoil-example"
    ROLE_STEM = "recoil-example-role"
    DESCRIPTION = "Example canonical skill. Use when you need the example procedure."
    ROLE_DESCRIPTION = "Read-only example role for one parent-assigned packet."

    def build_mirror(self, root: Path) -> None:
        canonical_skill = root / ".codex" / "skills" / self.SKILL / "SKILL.md"
        canonical_skill.parent.mkdir(parents=True)
        canonical_skill.write_text(
            f"---\nname: {self.SKILL}\ndescription: {self.DESCRIPTION}\n---\n\n"
            "# Example\n\nRoot `AGENTS.md` is authoritative.\n",
            encoding="utf-8",
        )
        canonical_role = root / ".codex" / "agents" / f"{self.ROLE_STEM}.toml"
        canonical_role.parent.mkdir(parents=True)
        canonical_role.write_text(
            'name = "recoil_example_role"\n'
            f'description = "{self.ROLE_DESCRIPTION}"\n'
            'sandbox_mode = "read-only"\n'
            'developer_instructions = """\nRoot AGENTS.md is authoritative.\n"""\n',
            encoding="utf-8",
        )
        self.write_skill_stub(root)
        self.write_role_stub(root)
        (root / "CLAUDE.md").write_text(
            "@AGENTS.md\n\n## Claude Code Harness Notes\n\nHarness mapping only.\n",
            encoding="utf-8",
        )
        settings = root / ".claude" / "settings.json"
        settings.parent.mkdir(parents=True, exist_ok=True)
        settings.write_text(
            json.dumps({"permissions": {"deny": list(CLAUDE_REQUIRED_DENY_RULES)}}),
            encoding="utf-8",
        )

    def write_skill_stub(self, root: Path, *, description: str | None = None, body: str | None = None) -> Path:
        path = root / ".claude" / "skills" / self.SKILL / "SKILL.md"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            f"---\nname: {self.SKILL}\n"
            f"description: {self.DESCRIPTION if description is None else description}\n---\n\n"
            + (
                body
                if body is not None
                else "Root `AGENTS.md` is authoritative. This stub adds no policy.\n\n"
                f"The canonical procedure is `.codex/skills/{self.SKILL}/SKILL.md`.\n"
            ),
            encoding="utf-8",
        )
        return path

    def write_role_stub(self, root: Path, *, name: str | None = None) -> Path:
        path = root / ".claude" / "agents" / f"{self.ROLE_STEM}.md"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            f"---\nname: {self.ROLE_STEM if name is None else name}\n"
            f"description: {self.ROLE_DESCRIPTION}\n"
            "disallowedTools: Edit, Write, NotebookEdit, Agent\n---\n\n"
            "Root `AGENTS.md` is authoritative. Your operating contract is the\n"
            f"`developer_instructions` value in `.codex/agents/{self.ROLE_STEM}.toml`.\n",
            encoding="utf-8",
        )
        return path

    def kinds(self, root: Path) -> list[str]:
        return [finding.kind for finding in audit_claude_mirror(root)]

    def test_frontmatter_scalars_keep_raw_values(self) -> None:
        scalars = parse_frontmatter_scalars('---\nname: a\ndescription: "b: c"\n---\n\nbody\n')
        self.assertEqual({"name": "a", "description": '"b: c"'}, scalars)
        self.assertIsNone(parse_frontmatter_scalars("no frontmatter here\n"))

    def test_complete_mirror_has_no_findings(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.build_mirror(root)
            self.assertEqual([], audit_claude_mirror(root))

    def test_missing_and_orphaned_stubs_are_reported(self) -> None:
        cases = (
            (lambda root: (root / ".claude" / "skills" / self.SKILL / "SKILL.md").unlink(), "claude-skill-missing"),
            (lambda root: (root / ".claude" / "agents" / f"{self.ROLE_STEM}.md").unlink(), "claude-role-missing"),
            (lambda root: (root / ".codex" / "skills" / self.SKILL / "SKILL.md").unlink(), "claude-skill-orphan"),
            (lambda root: (root / ".codex" / "agents" / f"{self.ROLE_STEM}.toml").unlink(), "claude-role-orphan"),
        )
        for mutate, kind in cases:
            with self.subTest(kind=kind), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                self.build_mirror(root)
                mutate(root)
                self.assertEqual([kind], self.kinds(root))

    def test_description_drift_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.build_mirror(root)
            self.write_skill_stub(root, description="Example canonical skill. Use when you feel like it.")
            self.assertEqual(["claude-skill-description-drift"], self.kinds(root))

    def test_stub_must_name_its_canonical_source_and_authority(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.build_mirror(root)
            self.write_skill_stub(root, body="Follow whatever seems reasonable.\n")
            self.assertEqual(
                ["claude-skill-authority", "claude-skill-pointer"],
                sorted(self.kinds(root)),
            )

    def test_thickened_stub_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.build_mirror(root)
            self.write_skill_stub(
                root,
                body=(
                    "Root `AGENTS.md` is authoritative.\n"
                    f"The canonical procedure is `.codex/skills/{self.SKILL}/SKILL.md`.\n"
                    + "Copied policy line.\n" * (CLAUDE_STUB_MAX_LINES + 1)
                ),
            )
            self.assertEqual(["claude-skill-stub-bloat"], self.kinds(root))

    def test_role_stub_name_must_mirror_the_canonical_identity(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.build_mirror(root)
            self.write_role_stub(root, name="recoil_example_role")
            self.assertEqual(["claude-role-name"], self.kinds(root))

    def test_claude_md_must_import_agents_md(self) -> None:
        cases = (
            (lambda root: (root / "CLAUDE.md").unlink(), "missing file"),
            (
                lambda root: (root / "CLAUDE.md").write_text(
                    "See `@AGENTS.md` for the canonical instructions.\n", encoding="utf-8"
                ),
                "code span is not an import",
            ),
        )
        for mutate, label in cases:
            with self.subTest(case=label), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                self.build_mirror(root)
                mutate(root)
                self.assertEqual(["claude-md-import"], self.kinds(root))

    def test_settings_must_keep_effective_ledger_and_retail_gates(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.build_mirror(root)
            settings = root / ".claude" / "settings.json"
            settings.write_text(
                json.dumps(
                    {"permissions": {"deny": list(CLAUDE_REQUIRED_DENY_RULES[1:])}}
                ),
                encoding="utf-8",
            )
            findings = audit_claude_mirror(root)
            self.assertEqual(["claude-settings-deny"], [item.kind for item in findings])
            self.assertIn(CLAUDE_REQUIRED_DENY_RULES[0], findings[0].message)

            settings.write_text(
                json.dumps(
                    {
                        "permissions": {
                            "deny": [*CLAUDE_REQUIRED_DENY_RULES, "Write(/support/**)"]
                        }
                    }
                ),
                encoding="utf-8",
            )
            self.assertEqual(["claude-settings-ineffective-rule"], self.kinds(root))

            settings.unlink()
            self.assertEqual(["claude-settings-missing"], self.kinds(root))


if __name__ == "__main__":
    unittest.main()
