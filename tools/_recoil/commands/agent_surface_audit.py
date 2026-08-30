#!/usr/bin/env python3
"""Audit agent-facing docs, skills, roles, and tool command surfaces."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from collections import Counter
from dataclasses import dataclass
import json
from pathlib import Path
import re
import shlex
import sys
from typing import Any

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - Python 3.10 fallback is not expected locally.
    tomllib = None  # type: ignore[assignment]

import recoil
from _recoil.commands.artifact_audit import (
    DurableReference,
    find_durable_devspace_references,
    load_progress_tracker_data,
    progress_tracker_path,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


VALID_CATEGORIES = {
    "audit",
    "binja",
    "build",
    "diagnostic",
    "docs",
    "guard",
    "issue",
    "navigation",
    "owner",
    "progress",
    "style",
    "validation",
    "verification",
}



REQUIRED_PHYSICAL_DATA_COMMANDS = {
    ("progress", "output-section", "show"),
    ("progress", "storage", "show"),
    ("audit", "final-image-catalog"),
    ("verify", "final-image"),
}











BYTE_MATCH_TRIPLET_REQUIREMENTS: tuple[str, ...] = ()





# Canonical policy ownership.  Narrow roles and skills reference their owner and
# carry only their role-specific action; they must not copy the full policy body.
EXPECTED_ROLES = {
    "recoil-bn-fact-mapper.toml": {"name": "recoil_bn_fact_mapper", "sandbox": "read-only", "required": ("complete address-labeled assembly", "completeness/truncation status", "Do not decide source owner", "Never mutate BN", "target binary", "global phase", "cursor/range", "frontier relation")},
    "recoil-bn-reconstructor.toml": {"name": "recoil_bn_reconstructor", "sandbox": "read-only", "required": ("sole write-capable Binary Ninja reconstruction role", "parent-named Recoil.bndb or messages.bndb", "maintained analysis artifact", "exclusive writer lease", "no BN reader or second writer", "filesystem", "BN MCP state", "reanalyze", "save the database", "Do not choose work", "target binary", "global phase", "cursor/range", "frontier relation")},
    "recoil-source-owner-mapper.toml": {"name": "recoil_source_owner_mapper", "sandbox": "read-only", "required": ("complete source-shaped owner", "recoil-source-model-recovery", "included and excluded functions", "competing source models", "Registered live-order comparison", "request kind `source-discovery`", "must never invoke `chatgpt-pro-line`", "target binary", "global phase", "cursor/range", "physical block", "frontier relation")},
    "recoil-source-owner-scrutinizer.toml": {"name": "recoil_source_owner_scrutinizer", "sandbox": "read-only", "required": ("Try to disprove", "Return exactly ALLOW or BLOCK", "Do not edit anything", "competing source models", "Registered live-order comparison", "request kind `source-discovery`", "must never invoke `chatgpt-pro-line`", "target binary", "global phase", "cursor/range", "frontier relation")},
    "recoil-provider-data-classifier.toml": {"name": "recoil_provider_data_classifier", "sandbox": "read-only", "required": ("authored-candidate", "route authored-primary-versus-auxiliary", "`pipeline_class`", "`authored_order_role`", "progress symbol set-pipeline-class-batch", "`progress owner replace-batch`", "`progress owner downgrade`", "`progress verification-target sync`", "workspace-issue candidates", "Never route row classification through an owner command", "Do not edit anything", "target binary", "global phase", "cursor/range", "frontier relation")},
    "recoil-scaffold-auditor.toml": {"name": "recoil_scaffold_auditor", "sandbox": "read-only", "required": ("failed source-faithful VC5SP3 variants AND", "Purpose docblock", ".agent/RAW_ASSEMBLY_ALLOWLIST.txt", "mechanically non-blocking", "`authored-body`/`authored-lifecycle-body`", "selected linked contributions as blocking", "order-gate debt", "Do not decide owner", "target binary", "global phase", "cursor/range", "frontier relation")},
    "recoil-verifier.toml": {"name": "recoil_verifier", "sandbox": "workspace-write", "required": ("schema/mode", "packet id", "first typed divergence", "Raw object extras remain diagnostic", "selected full-order", "Missing expected facts or ambiguous coverage blocks", "falling back to persisted summaries", "Never advance or accept ledger state", "never creates a packet commit", "read-only Git inspection")},
    "recoil-workspace-librarian.toml": {"name": "recoil_workspace_librarian", "sandbox": "read-only", "required": ("task_class: non-address-maintenance", "Mechanically locate already-accepted facts", "Do not turn docs/catalog/history into a new placement", "Do not edit anything")},
    "recoil-tool-maintainer.toml": {"name": "recoil_tool_maintainer", "sandbox": "workspace-write", "required": ("task_class: non-address-maintenance", "allowed/forbidden paths", "Never mutate progress-tracker/process-issue state", "issue-resolution candidate text", "stage only the exact writable closure", "exactly one nonaccepting commit", "parent owns every other Git operation")},
    "recoil-source-worker.toml": {"name": "recoil_source_worker", "sandbox": "workspace-write", "required": ("schema/mode", "packet id", "exact writable paths", "exact validation command", "order-edit-v1", "verify vc5-order", "needs no Binary Ninja", "no Binary Ninja, byte work", "Never broaden into another lane", "Never mutate Binary Ninja", "Do not claim owner", "stage only the exact writable closure", "exactly one nonaccepting commit", "parent owns every other Git operation")},
}

QUIET_ROLE_REQUIREMENTS = ()

ORCHESTRATION_REQUIREMENTS = {
    "AGENTS.md": (
        "Use subagents by default for bounded source",
        "The parent owns scheduling, integration, acceptance",
        "Use `recoil_tool_maintainer` by default",
    )
}

START_CONTRACT_REQUIREMENTS = {
    "AGENTS.md": (
        "bare `Start` request",
        "--lane all",
        "--max-packets",
        "without waiting",
        "primary lane",
        "call-contract",
        "full authored byte",
        "blocked primary does not suppress",
        "--packet-id",
        "--lane <primary|authored|object>",
    ),
    "docs/reconstruction/agent_launch_checklist.md": (
        "bare `Start` is enough",
        "--lane all",
        "--max-packets",
        "without waiting",
        "primary lane",
        "call-contract",
        "full authored byte",
        "blocked primary does not suppress",
        "--packet-id",
    ),
    "docs/reconstruction/retail_executable_reproduction.md": (
        "bare `Start` is a complete",
        "--lane all",
        "--max-packets",
        "without another user confirmation",
        "primary lane",
        "call-contract",
        "full authored byte",
        "blocked primary does not suppress",
        "--packet-id",
    ),
    "tools/README.md": (
        "bare `Start` is sufficient",
        "--lane all",
        "--max-packets",
        "without waiting",
        "primary lane",
        "call-contract",
        "full authored byte",
        "blocked primary does not suppress",
        "--packet-id",
    ),
    ".codex/skills/recoil-address-handoff/SKILL.md": (
        "bare `Start`",
        "--lane all",
        "--max-packets",
        "without waiting",
        "primary lane",
        "call-contract",
        "full authored byte",
        "blocked primary does not suppress",
        "--packet-id",
    ),
    ".codex/skills/recoil-progress-tracker/SKILL.md": (
        "bare `Start`",
        "--lane all",
        "--max-packets",
        "without waiting",
        "primary lane",
        "call-contract",
        "full authored byte",
        "blocked primary does not suppress",
        "--packet-id",
    ),
    ".codex/skills/recoil-workspace-audit/SKILL.md": (
        "bare `Start` needs no extra prompt",
        "--lane all",
        "--max-packets",
        "primary lane",
        "call contract",
        "authored, object",
        "blocked primary does not suppress",
        "--packet-id",
    ),
}

QUIET_POLICY_REQUIREMENTS: dict[str, tuple[str, ...]] = {}

PRIMARY_LANE_CONTRACT_REQUIREMENTS = {
    "AGENTS.md": (
        "Six Live Stages",
        "`authored-function-order`",
        "`authored-call-contract`",
        "`full-function-order`",
        "after every authored call contract is current",
        "do not wait for authored bytes",
    ),
    "docs/reconstruction/retail_executable_reproduction.md": (
        "primary source lane",
        "`authored-function-order`",
        "`authored-call-contract`",
        "`full-function-order`",
        "call-contract",
        "without waiting for authored-byte",
    ),
    ".codex/skills/recoil-workspace-audit/SKILL.md": (
        "primary lane runs `authored-function-order`",
        "`authored-call-contract`",
        "starts `full-function-order`",
        "every call-contract slice is current",
        "without waiting for authored-byte completion",
        "`authored-byte-match` is an independent",
        "`linked-byte-match` starts only after both full order and authored bytes",
    ),
}

CALL_CONTRACT_STAGE_REQUIREMENTS = {
    "AGENTS.md": (
        "authored-call-contract",
        "advance-live-call-contract",
        "live reviewed census",
        "one-time migration census",
        "census",
        "call_contract",
    ),
    "docs/reconstruction/retail_executable_reproduction.md": (
        "authored-call-contract",
        "advance-live-call-contract",
        "accepted authored-order",
        "census",
        "capped at 160",
    ),
    "docs/reconstruction/agent_launch_checklist.md": (
        "authored-call-contract",
        "advance-live-call-contract",
    ),
    "tools/README.md": (
        "authored-call-contract",
        "advance-live-call-contract",
        "accepted authored-order",
        "census",
        "call_contract",
    ),
    ".codex/skills/recoil-progress-tracker/SKILL.md": (
        "authored-call-contract",
        "advance-live-call-contract",
        "current qualifying population derives dynamically",
        "one-time migration",
        "census",
        "call_contract",
    ),
    ".codex/skills/recoil-validation/SKILL.md": (
        "Authored Call Contracts",
        "advance-live-call-contract",
        "call_contract",
    ),
    ".codex/skills/recoil-source-model-recovery/SKILL.md": (
        "six-stage",
        "authored-call-contract",
    ),
}

PRO_BROKER_CONTRACT_REQUIREMENTS = {
    "AGENTS.md": ("ChatGPT Pro: Escalation Only", "competing source-owner/block/order models", "routine registered order", "Workers return a scoped request packet"),
    ".codex/skills/recoil-tier-verification/SKILL.md": ("Direct hard-byte iteration does not require ChatGPT Pro", "`hard-byte-raw-assembly`", "Workers must never invoke"),
    ".codex/skills/recoil-source-model-recovery/SKILL.md": ("ambiguity escalation", "Registered `vc5-order`", "session-scoped `source-discovery` request id"),
    ".codex/skills/recoil-source-owner-scrutiny/SKILL.md": ("registered live-order comparison", "credible competing source models", "request kind `source-discovery`"),
}

RESOURCE_LEASE_CONTRACT_REQUIREMENTS = {
    "AGENTS.md": ("progress work leases --json", "packet_id", "read/read may overlap", "read/write", "write/write", "real active reservation"),
    ".codex/skills/recoil-address-handoff/SKILL.md": ("progress work leases", "read/read", "read/write", "write/write"),
}

BN_LEASE_CONTRACT_REQUIREMENTS = {
    ".codex/skills/recoil-binary-ninja-workflow/SKILL.md": ("read reservation/lease id", "stable saved view", "no writer lease is active", "no reader", "reanalysis", "propagation checks", "saves the database", "returns its packet", "releases the writer lease"),
    ".codex/skills/recoil-binary-ninja-reconstruction/SKILL.md": ("exclusive writer", "reservation/lease id", "No reader or second writer", "reanalysis", "checks propagation", "saves the database", "returns the packet", "releases the writer lease", "stable saved view"),
}

SOURCE_MODEL_TRACKER_AUTHORITY_REQUIREMENTS = {
    "AGENTS.md": ("The parent owns scheduling, integration, acceptance", "Manual semantic mutations", "dry-run-first"),
    ".codex/skills/recoil-source-model-recovery/SKILL.md": ("return exact proposed owner", "do not mutate the tracker", "Only the parent may dry-run and apply", "expected revision", "structured handoff before a source worker edits"),
}

OBJECT_CURSOR_NOTE_REQUIREMENTS = {
    ".codex/skills/recoil-durable-notes/SKILL.md": ("`parallel_authored_object_byte_cursor` only when returned", "evidence preparation only", "not a phase or peer scheduler", "recompute `python tools/recoil.py progress next`", "do not carry or automatically prepare the next object row"),
}

TOOL_MAINTAINER_REQUIREMENTS = {
    "AGENTS.md": ("Use `recoil_tool_maintainer` by default", "Allowed agent-surface evolution paths"),
    ".codex/skills/recoil-tool-maintainer/SKILL.md": ("root `AGENTS.md`", "non-address", "allowed write paths"),
}

AGENT_SURFACE_GOVERNANCE_REQUIREMENTS = {
    "AGENTS.md": ("Allowed agent-surface evolution paths", "Do not change production source", "owner/tier criteria"),
    ".codex/skills/recoil-tool-maintainer/SKILL.md": ("root `AGENTS.md`", "agent-surface evolution"),
}

SESSION_SCRATCH_POLICY_REQUIREMENTS = {
    "AGENTS.md": ("durable facts depend on a concrete `.devspace` path",),
    "docs/reconstruction/agent_launch_checklist.md": ("never clear", "durably depend on `.devspace`"),
    ".codex/skills/recoil-durable-notes/SKILL.md": ("temporary session scratch",),
}


# Git governs authored workspace changes; it is never reconstruction evidence.
# Each tuple names one semantic rule and the accepted wording alternatives that
# express it on the canonical surface. This avoids requiring one brittle
# sentence while still making deletion of a policy dimension fail.
GIT_GOVERNANCE_REQUIREMENT_GROUPS: dict[
    str, tuple[tuple[str, tuple[str, ...]], ...]
] = {
    "AGENTS.md": (
        ("workspace-change-control", ("Git is the authored workspace change-control mechanism",)),
        ("clean-reviewed-branch", ("clean reviewed branch",)),
        ("exact-write-closure", ("exact writable closure",)),
        ("orchestrator-worktree-governance", ("parent/tool orchestrator owns packet branch creation",)),
        ("reviewed-integration", ("temporary integration worktree",)),
        ("worker-write-boundary", ("worker edits only packet paths",)),
        ("worker-one-commit", ("exactly one nonaccepting packet commit",)),
        ("worktree-hygiene", ("workspace worktree hygiene",)),
        ("absolute-path-nonauthority", ("diagnostic provenance, not retail expected truth",)),
        ("progress-adapter-native", ("progress reconstruction-packet worktree adapter is\n`native-git-v1`",)),
        ("nonaccepting-commit", ("Commit existence is never reconstruction acceptance",)),
        ("opaque-git-state", ("Git object ids remain opaque implementation details",)),
        ("git-is-not-retail-truth", ("Git never supplies retail expected truth",)),
        ("destructive-primary-worktree-ban", ("Destructive primary-worktree Git operations remain prohibited",)),
        ("sqlite-authority", ("independent SQLite authorities",)),
        ("direct-reconstruction-evidence", ("directly compares every typed entity",)),
    ),
    "tools/README.md": (
        ("workspace-change-control", ("Git is the authored-workspace change-control mechanism",)),
        ("clean-reviewed-branch", ("clean reviewed branch",)),
        ("exact-write-closure", ("exact writable closure",)),
        ("native-git-closeout", ("porcelain-v2 plus commit-relative name-status/diff",)),
        ("nonaccepting-commit", ("Git commits are nonaccepting workspace change bundles",)),
        ("opaque-git-state", ("native object IDs remain opaque repository state",)),
        ("sqlite-cas-authority", ("CAS guards every mutation",)),
        ("direct-reconstruction-evidence", ("directly compares each selected body",)),
        ("maintained-authored-inputs", ("Git governs maintained authored inputs",)),
        ("ignored-generated-nonauthority", ("Ignored paths are generated or machine-local and are nonauthoritative",)),
        ("ignored-churn-not-closeout", ("ignored generated-file churn is not packet-closeout evidence",)),
        ("external-output-hygiene", ("Validation and build output should normally use external or isolated roots",)),
        ("unmerged-blocker", ("unresolved Git state is an unconditional blocker",)),
        ("linked-worktree-lifecycle", ("Each active issue packet owns one `packet/` branch",)),
        ("exact-worktree-handoff", ("Handoff and closeout resolve the stored branch to that exact worktree",)),
        ("progress-adapter-native", ("progress\nworktree adapter is `native-git-v1`",)),
    ),
    "docs/reconstruction/retail_executable_reproduction.md": (
        ("workspace-change-control", ("Git is the sole authored-workspace change-control mechanism",)),
        ("clean-reviewed-branch", ("clean reviewed branch",)),
        ("governed-packet-branch", ("creates one packet branch",)),
        ("closure-authorized-edits", ("make only closure-authorized changes",)),
        ("native-git-closeout", ("porcelain-v2 status plus commit-relative name-status/diff",)),
        ("reviewed-integration", ("integrates first in a temporary worktree",)),
        (
            "opaque-nonaccepting-git-state",
            ("Git commit and object ids are opaque workspace state and never retail expected truth",),
        ),
        ("sqlite-authority", ("schema-v5 progress database",)),
        ("direct-reconstruction-evidence", ("directly compares every body in the slice",)),
        ("maintained-authored-inputs", ("Git governs maintained authored inputs",)),
        ("ignored-generated-nonauthority", ("Ignored paths are generated or machine-local and are nonauthoritative",)),
        ("ignored-churn-not-closeout", ("ignored generated-file churn is not packet-closeout evidence",)),
        ("external-output-hygiene", ("Validation and build output should normally use external or isolated roots",)),
        ("unmerged-blocker", ("Unresolved Git state is an unconditional blocker",)),
        ("retirement-hygiene", ("retirement removes the merged packet branch",)),
        ("absolute-path-nonauthority", ("diagnostic provenance, never semantic identities",)),
        ("progress-adapter-native", ("progress worktree\nadapter is `native-git-v1`",)),
    ),
}

STARTUP_BINARY_TARGET_REQUIREMENTS = {
    "AGENTS.md": ("python tools/recoil.py progress next", "function-order", "linked-byte-match", "final-validation", "COFF timestamp"),
    "docs/reconstruction/agent_launch_checklist.md": ("python tools/recoil.py progress next", "function-order", "linked-byte-match", "final-validation", "deferred context"),
    ".codex/skills/recoil-address-handoff/SKILL.md": ("python tools/recoil.py progress next", "deferred"),
}

GLOBAL_TEXT_PIPELINE_REQUIREMENTS = {
    "AGENTS.md": ("function-order", "linked-byte-match", "final-validation", "COFF timestamp", "No-Introduced-Debt Closeout", "docs readme-progress"),
    "README.md": ("authored-function-order", "authored-byte-match", "full-function-order", "linked-byte-match", "final-validation", "independently", "RECOIL_PROGRESS:START", "sole progress authority"),
    "docs/reconstruction/agent_launch_checklist.md": ("python tools/recoil.py progress next", "sole Recoil.exe scheduler"),
    "docs/reconstruction/retail_executable_reproduction.md": ("support/Recoil.exe", "0x401000", "expected gating identity", "unlisted raw definition", "selected linked address groups", "linked-byte-match", "final-validation"),
    "tools/README.md": ("python tools/recoil.py progress next", "deferred context", "docs readme-progress", "automatically"),
    ".codex/skills/recoil-binary-ninja-reconstruction/SKILL.md": ("python tools/recoil.py progress next", "authored-function-order", "authored-byte-match", "full-function-order", "linked-byte-match", "final-validation", "independent"),
    ".codex/skills/recoil-source-model-recovery/SKILL.md": ("python tools/recoil.py progress next", "authored-function-order", "authored-byte-match", "full-function-order", "linked-byte-match", "final-validation", "independent"),
    ".codex/agents/recoil-source-worker.toml": ("order-edit-v1", "verify vc5-order", "Never broaden into another lane"),
    ".codex/agents/recoil-verifier.toml": ("first typed divergence", "Never advance or accept ledger state"),
    ".codex/agents/recoil-tool-maintainer.toml": ("progress next", "peer schedulers"),
    ".codex/agents/recoil-workspace-librarian.toml": ("progress-next state", "never invent or advance a cursor"),
}

SOURCE_BLOCK_ORDER_REQUIREMENTS = {
    "AGENTS.md": ("translation-unit contribution blocks", "generated VC5 COFF function order", "retail Binary Ninja address order"),
    ".codex/skills/recoil-source-model-recovery/SKILL.md": ("physical source-file block order", "semantic names do not override a proven block"),
    ".codex/agents/recoil-source-owner-mapper.toml": ("physical .cpp/.h shape", "neighbor order"),
    ".codex/agents/recoil-source-worker.toml": ("natural VC5SP3 order", "never force order"),
    ".codex/agents/recoil-verifier.toml": ("first missing, duplicate", "Raw object extras remain diagnostic", "selected full-order"),
    "tools/vc5_verify_targets/README.md": ("`check_function_order`", "manifest function list order", "retail BN address order", "generated VC5 COFF section/value order"),
}

SOURCE_SHAPE_CONTRACT_REQUIREMENTS = {
    "AGENTS.md": ("BN function names and comments are provisional navigation labels", "current production `src/` tree is implementation state, not", "not source-shape proof"),
    ".codex/skills/recoil-source-model-recovery/SKILL.md": ("BN function names and comments are provisional", "current production `src/` tree is implementation state", "not source-shape proof"),
    ".codex/agents/recoil-source-worker.toml": ("original-era source shape", "never force order"),
}

HARD_BYTE_MATCH_CHATGPT_PRO_REQUIREMENTS = {
    "AGENTS.md": ("recoil-tier-verification", "Raw assembly remains exception-only", "triggered Pro pass"),
    ".codex/skills/recoil-tier-verification/SKILL.md": ("Direct hard-byte iteration does not require ChatGPT Pro", "credible VC5SP3 C/C++ variants", "source-faithful-inline-asm"),
}

BYTE_MATCH_TRIPLET_PRODUCER_REQUIREMENTS = {
    ".codex/agents/recoil-bn-fact-mapper.toml": ("complete address-labeled assembly", "complete current-BN-body assembly", "completeness/truncation status", "one triplet member only"),
}

SOURCE_DISCOVERY_CHATGPT_PRO_REQUIREMENTS = {
    ".codex/skills/recoil-source-model-recovery/SKILL.md": ("ambiguity escalation", "Registered `vc5-order`", "alternative hypotheses", "advisory evidence only", "session-scoped `source-discovery` request id"),
    ".codex/agents/recoil-source-owner-mapper.toml": ("competing source models", "Registered live-order comparison", "request kind `source-discovery`"),
    ".codex/agents/recoil-source-owner-scrutinizer.toml": ("recoil-source-model-recovery", "request kind `source-discovery`"),
    ".codex/agents/recoil-bn-fact-mapper.toml": ("make no recommendation", "route interpretation"),
    ".codex/agents/recoil-workspace-librarian.toml": ("already-accepted facts", "Do not turn docs/catalog/history into a new placement"),
}

OBJECT_ORDER_GATE_REQUIREMENTS = {
    "AGENTS.md": ("expected gating identity", "resolve exactly once", "retail relative order", "unlisted raw definition", "mechanically non-blocking", "selected linked address groups", "seams remain exact"),
    "docs/reconstruction/retail_executable_reproduction.md": ("expected gating identity", "resolve exactly once", "retail relative order", "unlisted raw definition", "mechanically non-blocking", "selected linked address groups"),
    ".codex/skills/recoil-source-model-recovery/SKILL.md": ("expected gating identity", "resolve exactly once", "retail relative order", "unlisted raw", "mechanically non-blocking"),
    ".codex/skills/recoil-validation/SKILL.md": ("expected gating identity", "resolve exactly once", "retail relative order", "unlisted raw definition", "mechanically non-blocking"),
    ".codex/agents/recoil-verifier.toml": ("first missing, duplicate", "expected and candidate neighbors", "Raw object extras remain diagnostic"),
}

BN_MAINTAINED_ARTIFACT_REQUIREMENTS = {
    "AGENTS.md": ("Binary Ninja", "maintained analysis artifact", "parent-assigned `recoil_bn_reconstructor`"),
    "docs/reconstruction/agent_launch_checklist.md": ("maintained analysis artifact", "parent-assigned", "writer lease", "save"),
    ".codex/skills/recoil-binary-ninja-workflow/SKILL.md": ("maintained analysis artifact", "without separate user approval or a tracker mutation", "filesystem sandbox", "BN MCP state"),
    ".codex/skills/recoil-binary-ninja-reconstruction/SKILL.md": ("maintained analysis artifact", "without separate user approval or a tracker mutation", "filesystem writes", "BN MCP state"),
    ".codex/agents/recoil-bn-reconstructor.toml": ("maintained analysis artifact", "without separate user approval or a tracker mutation", "filesystem", "BN MCP state"),
}

CANONICAL_MFC_HEADER_REQUIREMENTS = {
    "docs/reconstruction/retail_executable_reproduction.md": ("D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE", "header trees are evidence only", "diagnostic library profile"),
    ".codex/skills/recoil-provider-boundary/SKILL.md": ("D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE/AFXWIN.H", "header tree are evidence only", "diagnostic only"),
}

PROVIDER_TARGET_REGISTRATION_REQUIREMENTS = {
    ".codex/skills/recoil-provider-boundary/SKILL.md": (
        "progress provider-target register",
        "four-byte IAT storage",
        "one-byte callable provider-function",
        "progress relocation-target bind",
        "never candidate output",
    ),
    "docs/reconstruction/provider_abi_notes.md": (
        "progress provider-target",
        "four-byte IAT storage",
        "one-byte callable provider view",
        "progress relocation-target bind",
        "never candidate output",
    ),
    "docs/reconstruction/retail_executable_reproduction.md": (
        "progress provider-target register",
        "four-byte storage",
        "one-byte callable provider views",
        "relocation-target bind",
        "never candidate output",
    ),
}

SAFETY_CRITICAL_SEMANTIC_REQUIREMENTS = {
    ".codex/skills/recoil-address-handoff/SKILL.md": (
        "deprecated aliases",
        "no accepted-prefix prerequisite",
        "Only subordinate",
    ),
    ".codex/skills/recoil-validation/SKILL.md": (
        "`authored-body`",
        "`authored-lifecycle-body`",
        "compiler-generated deleting variants",
        "mechanically non-blocking diagnostic",
        "selected linked groups",
    ),
    ".codex/skills/recoil-provider-boundary/SKILL.md": (
        "`pipeline_class`",
        "`authored_order_role`",
        "progress symbol set-pipeline-class-batch",
        "never route a row classification through `progress owner`",
    ),
    "docs/reconstruction/owner_led_workflow.md": (
        "compatibility alias",
        "no accepted-prefix prerequisite",
    ),
    "docs/reconstruction/retail_executable_reproduction.md": (
        "authored-byte lane independently",
        "deprecated compatibility alias",
        "not an accepted-prefix prerequisite",
    ),
    "docs/reconstruction/agent_launch_checklist.md": (
        "deprecated aliases",
    ),
}

DIRECT_SCRIPT_RE = re.compile(
    r"(?P<cmd>(?:python\s+)?tools[/\\](?P<script>(?:recoil_[A-Za-z0-9_]+|update_readme_progress)\.py))(?![A-Za-z0-9_])"
)
GATE_COMMAND_RE = re.compile(r"python\s+tools[/\\]recoil\.py\s+(?P<args>[^\r\n`]+)")
README_INDEX_HEADER_RE = re.compile(r"^## Complete Command Index\s*$", re.MULTILINE)
README_TABLE_ROW_RE = re.compile(r"^\|(?P<cells>.*)\|\s*$")
FRONTMATTER_RE = re.compile(r"\A---\r?\n(?P<body>.*?)\r?\n---\r?\n", re.DOTALL)
EMPTY_POWERSHELL_FENCE_RE = re.compile(r"^```powershell[ \t]*\r?\n[ \t\r\n]*```", re.MULTILINE)
VOLATILE_ONBOARDING_RE = re.compile(r"(?:\bCurrent active frontier:|\bnext frontier follows\b)", re.IGNORECASE)
OBSOLETE_BACKTICK_COMMAND_RE = re.compile(
    r"`(?:owner\s+(?:show|relationships|audit(?:-membership|-acceptance)?)|status|frontier)(?:\s+[^`]*)?`",
    re.IGNORECASE,
)
OBSOLETE_LANE_RE = re.compile(r"--lane\s+binary\b", re.IGNORECASE)
RETIRED_WORK_VOCAB_RE = re.compile(
    r"\b(?:WIP group|temporary group|generated JSON/action artifacts?)\b",
    re.IGNORECASE,
)
RETIRED_TEXT_NEXT_RE = re.compile(r"\btext[ \t]+next\b", re.IGNORECASE)
OBSOLETE_GLOBAL_SEQUENTIAL_PIPELINE_PATTERNS = (
    re.compile(r"\bwork\s+proceeds\s+globally\s+and\s+sequentially\b", re.IGNORECASE),
    re.compile(
        r"\bauthored-function-order\b(?:(?!\n\s*\n).){0,240}"
        r"\bthen\b(?:(?!\n\s*\n).){0,80}\bauthored-byte-match\b"
        r"(?:(?!\n\s*\n).){0,160}\bfull-function-order\b",
        re.IGNORECASE | re.DOTALL,
    ),
    re.compile(
        r"\bglobal(?:\s+phase)?\s+order(?:\s+proof)?\b"
        r"(?:(?!\n\s*\n).){0,320}"
        r"\b(?:sequential\s+linked\s+bytes|linked\s+bytes\s+(?:are\s+)?visited\s+sequentially)\b"
        r"(?:(?!\n\s*\n).){0,240}\bthen\b"
        r"(?:(?!\n\s*\n).){0,120}\b(?:final|semantic image|unrestricted)\b",
        re.IGNORECASE | re.DOTALL,
    ),
)
STALE_CALL_CONTRACT_CENSUS_RE = re.compile(r"\b(?:3,?370)\b")
REVIEWED_MIGRATION_CENSUS_RE = re.compile(r"\b(?:3,?380)\b")
VOLATILE_ONBOARDING_SURFACES = {
    "AGENTS.md",
    "CLAUDE.md",
    ".agent/AGENTS.md",
    "docs/reconstruction/agent_launch_checklist.md",
    "docs/reconstruction/knowledge_index.md",
    "docs/reconstruction/messages_dll.md",
}
ISSUE_LEDGER_DELEGATION_RE = re.compile(
    r"(?:WORKSPACE_ISSUES\.(?:json|sqlite3)|workspace issues?)[^\r\n]{0,200}\bunless\b[^\r\n]{0,120}\bparent\b[^\r\n]{0,120}\b(?:delegat\w*|assign\w*)",
    re.IGNORECASE,
)
OWNER_EDIT_RE = re.compile(r"\bowner\s+edit\b", re.IGNORECASE)
SET_TIER_BATCH_OVERREACH_RE = re.compile(
    r"set-tier-batch(?:(?!set-gates-batch|set-address-meta).){0,240}\b(?:apply|applies|update|updates|set|sets)\b(?!-).{0,100}\b(?:gates?|metadata)\b",
    re.IGNORECASE | re.DOTALL,
)
BN_NAME_AUTHORITY_PATTERNS = (
    re.compile(
        r"\b(?:BN|Binary Ninja)\s+(?:function\s+)?names?(?:\s+and\s+comments?)?\s+(?:are|remain)\s+(?:authoritative|the\s+source\s+of\s+truth)\b",
        re.IGNORECASE,
    ),
    re.compile(
        r"\b(?:Recoil|messages)\.bndb\b[^\r\n.]{0,200}\bsource\s+of\s+truth\b[^\r\n.]{0,200}\b(?:names?|comments?)\b",
        re.IGNORECASE,
    ),
)
RETIRED_PLAN_COMMAND_PATTERNS = (
    (
        re.compile(r"\bRECOIL_(?:MESSAGES_)?PLAN\.md\b"),
        "Retired plan files must not be referenced from active guidance.",
    ),
    (
        re.compile(r"\b(?:python\s+tools[/\\]recoil\.py|recoil\.py)\s+plan\b"),
        "Retired owner-entry commands must not be referenced from active guidance.",
    ),
    (
        re.compile(r"`plan\s+(?:show|find|group|next|batch|set|set-meta|add-provider-boundary|add-data|reclassify)\b[^`]*`"),
        "Retired shorthand owner-entry commands must not be referenced from active guidance.",
    ),
)
RETIRED_PLAN_VOCAB_PATTERNS = (
    (
        re.compile(
            (
                r"\b(?:"
                r"plan " r"marker" r"s?|plan " r"row" r"s?|plan " r"entr" r"ies|plan " r"Target|"
                r"plan-" r"track" r"ed|update plan " r"marker" r"s|plan " r"CLI|plan " r"command|"
                r"plan " r"address|plan " r"fallback|focused plan " r"lookup|pending plan " r"rows|"
                r"whole " r"plan|address " r"plan"
                r")\b"
            ),
            re.IGNORECASE,
        ),
        "Retired plan vocabulary must not be used for active guidance.",
    ),
)
RETIRED_PLAN_ACTIVE_DOCS = {
    "AGENTS.md",
    "CLAUDE.md",
    ".agent/AGENTS.md",
    "tools/README.md",
    "docs/reconstruction/agent_launch_checklist.md",
}
GIT_STATE_RE = re.compile(
    r"\b(?:git(?:\.exe)?|stage\s+(?:(?:this|these)\s+)?files?|staged\s+files?|staging|checkout)\b",
    re.IGNORECASE,
)
GIT_NEGATION_RE = re.compile(
    r"\b(?:do not|does not|don't|must not|never|forbidden|prohibited|ban|bans)\b",
    re.IGNORECASE,
)
GIT_MUTATION_RE = re.compile(
    r"(?ix)(?:"
    r"\bgit(?:\.exe)?\s+(?:add|commit|reset|checkout|restore|clean|stash|"
    r"switch|worktree|update-index|hash-object|gc|repack)\b|"
    r"\bstag(?:e|ed|ing)\b"
    r")"
)
READ_ONLY_GIT_WORDS = frozenset(
    {"status", "diff", "ls-files", "check-ignore", "commands", "path", "state", "object"}
)
LEGACY_SCRIPT_COMMANDS = {
    "recoil_agent_surface_audit.py": "audit agent-surface",
    "recoil_asm_verify.py": "verify asm",
    "recoil_binja_preflight.py": "binja preflight",
    "recoil_doctor.py": "doctor",
    "recoil_env_check.py": "env",
    "recoil_frontier.py": "frontier",
    "recoil_function_docblock_audit.py": "audit docblocks",
    "recoil_functional_verify.py": "verify functional",
    "recoil_groups_audit.py": "audit groups",
    "recoil_handoff.py": "handoff",
    "recoil_msvc_eh_dump.py": "msvc eh-dump",
    "recoil_msvc_x86_run.py": "build msvc-x86",
    "recoil_multiline_style_guard.py": "guard multiline",
    "recoil_no_modern_cpp_constructs.py": "guard modern-cpp",
    "recoil_no_raw_assembly.py": "guard raw-assembly",
    "recoil_no_raw_image_addresses.py": "guard raw-image",
    "recoil_no_reinterpret_cast.py": "guard reinterpret-cast",
    "recoil_no_source_shape_scaffolds.py": "guard source-shape",
    "recoil_original_source_symbol_guard.py": "guard original-symbol",
    "recoil_pe_reference.py": "verify pe",
    "recoil_provenance_audit.py": "audit provenance",
    "recoil_provider_boundary_guard.py": "guard provider",
    "recoil_raw_offset_guard.py": "guard raw-offset",
    "recoil_resource_extract.py": "build resource",
    "recoil_source_data_initializer_guard.py": "guard source-data",
    "recoil_source_file_map.py": "audit source-map",
    "recoil_source_placement_guard.py": "guard source-placement",
    "recoil_status.py": "status",
    "recoil_strict_multiline_style_fix.py": "style fix-multiline",
    "recoil_task_packet.py": "packet",
    "recoil_vc5_build.py": "verify final-build",
    "recoil_vc5_manifest_source_guard.py": "guard vc5-manifest",
    "recoil_vc5_verify.py": "verify vc5",
    "recoil_verification_backlog.py": "audit backlog",
    "recoil_workspace_hygiene.py": "audit workspace",
    "recoil_workspace_issues.py": "issue",
    "recoil_zinterp_dispatch_audit.py": "audit zinterp",
    "update_readme_progress.py": "docs readme-progress",
}

# `.codex` is the single canonical agent surface.  The Claude harness reads its
# own directory layout, so it gets pointer stubs that carry routing metadata,
# the verbatim canonical description, and the canonical path to read.  A stub
# that grows past this line budget is copying policy instead of pointing at it.
CLAUDE_STUB_MAX_LINES = 40
CLAUDE_MEMORY_IMPORT_RE = re.compile(r"^@AGENTS\.md[ \t]*$", re.MULTILINE)
CLAUDE_REQUIRED_DENY_RULES = (
    "Edit(/.agent/RECONSTRUCTION_PROGRESS.sqlite3)",
    "Edit(/.agent/WORKSPACE_ISSUES.sqlite3)",
    "Edit(/support/**)",
)
# Claude Code consults only `Edit(path)` and `Read(path)` file rules; a path rule
# written for another file-editing tool is accepted and never applied.
INEFFECTIVE_FILE_RULE_RE = re.compile(r"^(?:Write|MultiEdit|NotebookEdit)\(")


@dataclass(frozen=True)
class Finding:
    severity: str
    surface: str
    line: int
    kind: str
    message: str
    suggestion: str = ""

    def to_json(self) -> dict[str, object]:
        return {
            "severity": self.severity,
            "surface": self.surface,
            "line": self.line,
            "kind": self.kind,
            "message": self.message,
            "suggestion": self.suggestion,
        }


def line_for_offset(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def has_required_phrase(text: str, phrase: str) -> bool:
    normalized_text = " ".join(text.split()).casefold()
    normalized_phrase = " ".join(phrase.split()).casefold()
    return normalized_phrase in normalized_text


def gate_suggestion(script: str) -> str:
    command = LEGACY_SCRIPT_COMMANDS.get(script)
    if command is None:
        return "Use `python tools/recoil.py help` and add a gate command if this script is agent-facing."
    return f"Use `python tools/recoil.py {command}`."


def is_active_retired_plan_surface(surface: str) -> bool:
    normalized = surface.replace("\\", "/")
    return (
        normalized in RETIRED_PLAN_ACTIVE_DOCS
        or normalized.startswith(".codex/skills/recoil-")
        or normalized.startswith(".codex/agents/")
    )


def is_active_command_surface(surface: str) -> bool:
    normalized = surface.replace("\\", "/")
    return (
        normalized in {
            "AGENTS.md",
            "CLAUDE.md",
            "README.md",
            ".agent/AGENTS.md",
            "tools/README.md",
            "docs/reconstruction/agent_launch_checklist.md",
            "docs/reconstruction/provider_abi_notes.md",
        }
        or normalized.startswith(".codex/skills/recoil-")
        or normalized.startswith(".codex/agents/recoil-")
    )


def is_generated_readme_progress_offset(text: str, surface: str, offset: int) -> bool:
    """Treat the tracker-generated public README projection as live state, not prose."""

    if surface.replace("\\", "/") != "README.md":
        return False
    start = text.find("<!-- RECOIL_PROGRESS:START -->")
    end = text.find("<!-- RECOIL_PROGRESS:END -->")
    return start >= 0 and end >= start and start <= offset <= end


def split_command_args(raw: str) -> list[str]:
    cleaned = raw.strip()
    while cleaned and cleaned[-1] in ".,;)":
        cleaned = cleaned[:-1].rstrip()
    try:
        values = shlex.split(cleaned, posix=False)
        return [
            value[1:-1] if len(value) >= 2 and value[0] == value[-1] and value[0] in {'"', "'"} else value
            for value in values
        ]
    except ValueError:
        return cleaned.split()


def is_placeholder_gate_invocation(raw: str, args: list[str]) -> bool:
    return bool(args) and ("<" in raw or "..." in raw)


def registered_prefix_before_placeholder(raw: str, args: list[str]) -> bool:
    """Return whether a placeholder follows a real registered command/group prefix."""

    if not is_placeholder_gate_invocation(raw, args):
        return False
    placeholder_index = next(
        (
            index
            for index, token in enumerate(args)
            if "<" in token or "..." in token
        ),
        len(args),
    )
    prefix = args[:placeholder_index]
    if not prefix:
        return False
    item, rest = recoil.resolve_command(prefix)
    if item is not None:
        return True
    return bool(recoil.group_children(tuple(prefix)))


def is_known_gate_invocation(args: list[str], *, raw: str = "") -> bool:
    if not args:
        return True
    if args[0] in {"-h", "--help"}:
        return True
    if args[0] == "help":
        if len(args) == 1:
            return True
        item, rest = recoil.resolve_command(args[1:])
        if item is not None:
            return True
        if recoil.group_children(tuple(args[1:])):
            return True
        help_raw = raw.partition(" ")[2]
        return registered_prefix_before_placeholder(help_raw, args[1:])
    if args[0] == "commands":
        return args[1:] == ["--json"]
    item, _rest = recoil.resolve_command(args)
    if item is not None:
        return True
    if recoil.group_children(tuple(args)):
        return True
    return registered_prefix_before_placeholder(raw, args)


def retired_owner_mutation_suggestion(args: list[str]) -> str:
    """Route obsolete positive/negative owner examples to current governed paths."""

    canonical = args[1:] if args[:1] == ["progress"] else args
    subcommand = canonical[1] if canonical[:1] == ["owner"] and len(canonical) > 1 else ""
    lowered = {item.casefold() for item in canonical}
    if subcommand == "set-address-meta" and "--target" in canonical:
        return (
            "Use registered `progress verification-target sync` for reviewed target "
            "registration/source-policy synchronization."
        )
    negative_states = {"blocked", "pending", "deferred", "none"}
    if subcommand in {
        "set-gate",
        "set-gates",
        "set-gates-batch",
        "set-entry-tier",
        "set-entry-tier-batch",
        "set-tier-batch",
    } and (negative_states.intersection(lowered) or "downgrade" in lowered):
        return (
            "Use the registered dry-run-first `progress owner downgrade` route for "
            "conservative negative gate or primary-entry tier changes."
        )
    if subcommand in {
        "add",
        "remove",
        "link-address",
        "unlink-address",
        "link-data",
        "unlink-data",
        "link-anchor",
        "unlink-anchor",
        "link-address-batch",
        "unlink-address-batch",
        "link-data-batch",
        "unlink-data-batch",
        "move-anchor",
        "dependencies",
        "set-source-paths",
        "set-section",
        "set-blocker",
        "set-lifecycle",
        "prune-address-meta",
        "move-primary",
    }:
        return (
            "Use registered dry-run-first `progress owner replace-batch` with exact "
            "reviewed owner snapshots and guarded primary memberships."
        )
    return (
        "No registered positive metadata/gate/tier mutation supports this operation; "
        "return a workspace-issue candidate instead of advertising a nonexistent command."
    )


def audit_text(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    normalized_surface = surface.replace("\\", "/")
    lines = text.splitlines()
    for index in range(1, len(lines)):
        current = lines[index].strip()
        previous = lines[index - 1].strip()
        if (
            current
            and current == previous
            and "python tools/recoil.py " in current
        ):
            findings.append(
                Finding(
                    "error",
                    surface,
                    index + 1,
                    "duplicate-adjacent-command",
                    "Agent-facing surface repeats the exact same gate command on adjacent lines.",
                    "Keep one canonical command line and remove the duplicate.",
                )
            )
    if normalized_surface == "docs/reconstruction/provider_abi_notes.md" and "☑" in text:
        findings.append(
            Finding(
                "error",
                surface,
                line_for_offset(text, text.index("☑")),
                "legacy-checkmark-marker",
                "Provider guidance contains a legacy checkmark acceptance marker.",
                "Use unified owner gates and per-primary-entry Reimplemented tiers only.",
            )
        )
    if is_active_command_surface(surface):
        for match in re.finditer(r"accepted-prefix\s+fallback", text, re.IGNORECASE):
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "stale-fallback-semantics",
                    "Active guidance describes the full authored-byte compatibility alias as an accepted-prefix fallback.",
                    "State that the deprecated fallback alias has no accepted-prefix prerequisite.",
                )
            )
    if is_active_command_surface(surface):
        for match in RETIRED_TEXT_NEXT_RE.finditer(text):
            line_start = text.rfind("\n", 0, match.start()) + 1
            line_end = text.find("\n", match.end())
            if line_end < 0:
                line_end = len(text)
            line = text[line_start:line_end]
            if line_is_explicitly_non_scheduling(line) or any(
                phrase in line.lower()
                for phrase in ("obsolete", "retired", "nonexistent", "unsupported")
            ):
                continue
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "retired-text-next",
                    "Active guidance recommends the nonexistent `text next` scheduler.",
                    "Use `python tools/recoil.py progress next`, the sole no-target Recoil.exe scheduler.",
                )
            )
        for pattern in OBSOLETE_GLOBAL_SEQUENTIAL_PIPELINE_PATTERNS:
            for match in pattern.finditer(text):
                if line_is_explicitly_non_scheduling(match.group(0)):
                    continue
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_for_offset(text, match.start()),
                        "obsolete-global-sequential-pipeline",
                        "Active guidance collapses the dual-lane six-stage scheduler into one global sequential order/byte/final pipeline.",
                        "Describe the primary authored-function-order -> authored-call-contract -> full-function-order lane, independent authored-byte work, the linked-byte join, and final-validation.",
                    )
                )
        for match in OBSOLETE_BACKTICK_COMMAND_RE.finditer(text):
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "obsolete-command-vocabulary",
                    f"Active guidance uses obsolete command shorthand {match.group(0)!r}.",
                    "Use the registered `python tools/recoil.py progress ...` command or describe the concept without command formatting.",
                )
            )
        for match in OBSOLETE_LANE_RE.finditer(text):
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "obsolete-command-option",
                    "Active guidance uses the retired `--lane binary` option.",
                    "Use `python tools/recoil.py progress next`; functional verification is a targeted dependency activity.",
                )
            )
        for match in RETIRED_WORK_VOCAB_RE.finditer(text):
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "retired-work-vocabulary",
                    f"Active guidance uses retired work-item terminology {match.group(0)!r}.",
                    "Use `structured work item`; imports create evidence and diagnostic reports, not action batches.",
                )
            )
    if normalized_surface == "docs/reconstruction/agent_launch_checklist.md" and len(text.splitlines()) > 200:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "launch-checklist-length",
                f"Agent launch checklist has {len(text.splitlines())} lines; the compact reminder limit is 200.",
                "Move full policy narratives to root `AGENTS.md` and keep only launch triggers and command reminders here.",
            )
        )
    for match in EMPTY_POWERSHELL_FENCE_RE.finditer(text):
        findings.append(
            Finding(
                "error",
                surface,
                line_for_offset(text, match.start()),
                "empty-powershell-fence",
                "Agent-facing surface contains an empty PowerShell command fence.",
                "Remove the obsolete fence or populate it with a registered `python tools/recoil.py` command.",
            )
        )
    if normalized_surface in VOLATILE_ONBOARDING_SURFACES:
        for match in VOLATILE_ONBOARDING_RE.finditer(text):
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "volatile-onboarding-state",
                    f"Agent-facing surface hard-codes volatile queue state: `{match.group(0)}`.",
                    "Route agents to `python tools/recoil.py progress next`; use block views only as mapping context.",
                )
            )
    for match in ISSUE_LEDGER_DELEGATION_RE.finditer(text):
        findings.append(
            Finding(
                "error",
                surface,
                line_for_offset(text, match.start()),
                "subagent-issue-ledger-authority",
                "Agent-facing surface lets a parent delegate workspace-issue ledger mutation to a subagent.",
                "Make filing, resolving, reopening, and direct WORKSPACE_ISSUES edits unconditionally parent-only; subagents return candidates.",
            )
        )
    for match in OWNER_EDIT_RE.finditer(text):
        findings.append(
            Finding(
                "error",
                surface,
                line_for_offset(text, match.start()),
                "retired-owner-command",
                "Agent-facing surface references the unregistered `owner edit` command.",
                "Use `progress owner replace-batch` or `progress owner downgrade` when their fail-closed contracts apply; otherwise return a workspace-issue candidate.",
            )
        )
    for match in SET_TIER_BATCH_OVERREACH_RE.finditer(text):
        findings.append(
            Finding(
                "error",
                surface,
                line_for_offset(text, match.start()),
                "owner-command-semantics",
                "Guidance assigns gate or metadata semantics to the retired, unregistered `owner set-tier-batch` command.",
                "Use `progress owner downgrade` for conservative negative gate/tier changes, `progress verification-target sync` for reviewed target registration, or return a workspace-issue candidate for unsupported positive metadata/gate/tier mutation.",
            )
        )
    if is_active_command_surface(surface):
        for match in STALE_CALL_CONTRACT_CENSUS_RE.finditer(text):
            if is_generated_readme_progress_offset(text, surface, match.start()):
                continue
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "stale-call-contract-census",
                    f"Active guidance hard-codes stale authored call-contract census `{match.group(0)}`.",
                    "Derive the current census from accepted authored-order gating rows; do not introduce a permanent live count.",
                )
            )
        for match in REVIEWED_MIGRATION_CENSUS_RE.finditer(text):
            if is_generated_readme_progress_offset(text, surface, match.start()):
                continue
            paragraph_start = text.rfind("\n\n", 0, match.start()) + 2
            paragraph_end = text.find("\n\n", match.end())
            if paragraph_end < 0:
                paragraph_end = len(text)
            context = text[paragraph_start:paragraph_end].casefold()
            migration_context = (
                "reviewed" in context
                and "migration" in context
                and ("one-time" in context or "initial" in context)
            )
            if migration_context:
                continue
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "permanent-call-contract-census",
                    f"Active guidance presents `{match.group(0)}` outside reviewed one-time migration context.",
                    "Keep the historical 3,380 value only as the reviewed one-time migration census; derive every live census from accepted authored-order gating rows.",
                )
            )
    findings.extend(audit_semantic_contradictions(text, surface))
    for pattern in BN_NAME_AUTHORITY_PATTERNS:
        for match in pattern.finditer(text):
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "bn-name-authority",
                    "Guidance describes Binary Ninja names or comments as authoritative binary facts.",
                    "State that BN names/comments are provisional navigation labels; assembly, xrefs, literals, layouts, and order are authoritative binary evidence.",
                )
            )
    for pattern, message in RETIRED_PLAN_COMMAND_PATTERNS:
        for match in pattern.finditer(text):
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "retired-plan-guidance",
                    message,
                    "Use `.agent/RECONSTRUCTION_PROGRESS.sqlite3` through `python tools/recoil.py progress ...` and its owner gates/tiers.",
                )
            )
    if is_active_retired_plan_surface(surface):
        for pattern, message in RETIRED_PLAN_VOCAB_PATTERNS:
            for match in pattern.finditer(text):
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_for_offset(text, match.start()),
                        "retired-plan-guidance",
                        message,
                        "Use `.agent/RECONSTRUCTION_PROGRESS.sqlite3` through `python tools/recoil.py progress ...` and its owner gates/tiers.",
                    )
                )
    for match in DIRECT_SCRIPT_RE.finditer(text):
        script = match.group("script")
        findings.append(
            Finding(
                "error",
                surface,
                line_for_offset(text, match.start()),
                "legacy-command",
                f"Agent-facing surface uses direct legacy script `{match.group('cmd')}`.",
                gate_suggestion(script),
            )
        )
    for match in GATE_COMMAND_RE.finditer(text):
        raw_args = match.group("args")
        args = split_command_args(raw_args)
        if args and args[0] == "plan":
            continue
        known_invocation = is_known_gate_invocation(args, raw=raw_args)
        if not known_invocation:
            entered = " ".join(args)
            canonical_args = args[1:] if args[:1] == ["progress"] else args
            is_retired_owner_mutation = canonical_args[:1] == ["owner"] and len(canonical_args) > 1
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "retired-owner-mutation" if is_retired_owner_mutation else "unknown-gate-command",
                    f"Unknown `python tools/recoil.py` command `{entered}`.",
                    retired_owner_mutation_suggestion(args)
                    if is_retired_owner_mutation
                    else "Run `python tools/recoil.py help` or register the command in tools/recoil.py.",
                )
            )
        canonical_args = args[1:] if args[:1] == ["progress"] else args
        if canonical_args[:2] == ["owner", "show"] and "--binary" in canonical_args:
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "owner-command-options",
                    "`owner show` does not accept `--binary`; owner ids and address links are already target-qualified.",
                    "Use `python tools/recoil.py progress owner show <owner-id-or-address>`.",
                )
            )
        if canonical_args[:2] == ["owner", "set-gates"]:
            unsupported = sorted({flag for flag in ("--kind", "--parent", "--owner-state") if flag in args})
            if unsupported:
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_for_offset(text, match.start()),
                        "owner-command-options",
                        f"`owner set-gates` does not accept {', '.join(f'`{flag}`' for flag in unsupported)}.",
                        retired_owner_mutation_suggestion(args),
                    )
                )
        if canonical_args[:2] == ["owner", "set-address-meta"]:
            has_example_arguments = any(
                token.startswith("0x") or token.startswith("<address") or token == "--evidence" for token in args[2:]
            )
            metadata_flags = {"--name", "--source-path", "--target", "--group"}
            if has_example_arguments and not metadata_flags.intersection(args):
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_for_offset(text, match.start()),
                        "owner-command-options",
                        "`owner set-address-meta` example does not set any metadata field.",
                        retired_owner_mutation_suggestion(args),
                    )
                )
        if args[:2] == ["progress", "audit"] and "--binja" in args:
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "progress-command-options",
                    "`progress audit` does not accept `--binja`.",
                    "Use `python tools/recoil.py doctor --quick --binja` for BN-backed readiness.",
                )
            )
        if canonical_args[:1] == ["owner"]:
            owner_subcommand = canonical_args[1] if len(canonical_args) > 1 else ""
            unsupported: list[str] = []
            if owner_subcommand == "add" and "--anchor" in canonical_args:
                unsupported.append("--anchor")
            if owner_subcommand == "dependencies" and "--replace" in canonical_args:
                unsupported.append("--replace")
            if "--evidence" in canonical_args:
                unsupported.append("--evidence")
            if unsupported:
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_for_offset(text, match.start()),
                        "owner-command-options",
                        f"Owner command uses unsupported option(s): {', '.join(f'`{item}`' for item in sorted(set(unsupported)))}.",
                        "Use `--evidence-id`, add anchors through `owner link-anchor`, and update dependencies through `--add`/`--remove`.",
                    )
                )
            mutating_owner_commands = {"downgrade", "replace-batch"}
            if owner_subcommand in mutating_owner_commands and (
                "--apply" in canonical_args or "--dry-run" in canonical_args
            ):
                missing = [flag for flag in ("--expected-revision",) if flag not in canonical_args]
                if missing:
                    findings.append(
                        Finding(
                            "error",
                            surface,
                            line_for_offset(text, match.start()),
                            "owner-mutation-contract",
                            f"Owner mutation example is missing {', '.join(f'`{flag}`' for flag in missing)}.",
                            "Every dry-run/apply example must use `--expected-revision` as its concurrency guard.",
                        )
                    )
    return findings


ACTIVE_SCHEDULER_SURFACES = {
    "AGENTS.md",
    "CLAUDE.md",
    "README.md",
    ".agent/AGENTS.md",
    "tools/README.md",
    "docs/reconstruction/agent_launch_checklist.md",
    "docs/reconstruction/owner_led_workflow.md",
    "docs/reconstruction/final_executable_repro.md",
    "docs/reconstruction/knowledge_index.md",
    "docs/reconstruction/source_file_layout_audit.md",
}


def line_is_explicitly_non_scheduling(line: str) -> bool:
    lowered = line.lower()
    return any(
        phrase in lowered
        for phrase in (
            "historical",
            "legacy",
            "deferred_by_pipeline_phase",
            "deferred context",
            "non-scheduling",
            "not a scheduler",
            "must not schedule",
            "do not schedule",
            "do not let",
            "does not choose",
            "never choose",
            "not a work unit",
            "not work units",
            "no work unit",
            "evidence producer",
            "observed evidence",
        )
    )


def audit_semantic_contradictions(text: str, surface: str) -> list[Finding]:
    """Reject policy text that coexists with, but contradicts, required phrases."""
    normalized = surface.replace("\\", "/")
    findings: list[Finding] = []
    scheduler_surface = normalized in ACTIVE_SCHEDULER_SURFACES or normalized.startswith(
        (".codex/skills/recoil-", ".codex/agents/recoil-")
    )
    scheduler_patterns = (
        re.compile(r"\bowner next\b.*\b(?:choose|select|schedule|priority|first|authoritative)\b", re.I),
        re.compile(r"\b(?:choose|select|schedule)\b.*\bowner next\b", re.I),
        re.compile(r"\b(?:choose|select|resume)\b.*\bearliest\b.*\bsource[- ](?:file )?block\b", re.I),
        re.compile(r"\b(?:map\.cpp|mission\.cpp)\b.*\b(?:current|next|active)\b.*\bfrontier\b", re.I),
        re.compile(r"\b(?:final-repro|final-data|messages\.dll)\b.*\b(?:next|priority|peer queue|schedule)\b", re.I),
    )
    raw_asm_or_bypass = re.compile(
        r"(?:C/C\+\+|source-faithful)[^\n]{0,100}\b(?:failed|fail)[^\n]{0,80}\bor\b[^\n]{0,80}(?:ChatGPT Pro|chatgpt-pro)",
        re.I,
    )
    worker_pro_surfaces = {
        ".codex/agents/recoil-source-worker.toml",
        ".codex/agents/recoil-verifier.toml",
        ".codex/agents/recoil-source-owner-mapper.toml",
        ".codex/agents/recoil-source-owner-scrutinizer.toml",
    }
    for line_number, line in enumerate(text.splitlines(), start=1):
        lowered_line = line.lower()
        if "mixed source-discovery/byte-codegen" in lowered_line:
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_number,
                    "mixed-pro-request-kind",
                    "Surface permits or describes a mixed source-discovery/byte-codegen Pro prompt.",
                    "Use distinct `source-discovery` and `hard-byte-raw-assembly` request kinds that never share a prompt, package, transcript, or call.",
                )
            )
        if normalized in worker_pro_surfaces:
            negated = any(
                phrase in lowered_line
                for phrase in ("must never", "never invoke", "must not", "do not", "don't")
            )
            if "chatgpt-pro-line" in lowered_line and not negated:
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_number,
                        "worker-direct-pro-call",
                        "Worker role directly invokes `chatgpt-pro-line` instead of returning a broker package.",
                        "Return a complete synchronized `needs_pro` package, release the slot, and let the parent broker the call.",
                    )
                )
            direct_upload = "upload-file" in lowered_line or "perform a live upload" in lowered_line
            parent_owned = "parent" in lowered_line and any(
                word in lowered_line for word in ("upload", "broker", "supplied")
            )
            if direct_upload and not negated and not parent_owned:
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_number,
                        "worker-live-upload",
                        "Worker role owns or performs a live ChatGPT Pro upload.",
                        "Limit the worker to local artifact checks and a complete synchronized package; the parent owns uploads.",
                    )
                )
        if scheduler_surface and not line_is_explicitly_non_scheduling(line):
            if any(pattern.search(line) for pattern in scheduler_patterns):
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_number,
                        "competing-scheduler-policy",
                        "Surface presents a legacy owner/source-block/final queue as an active scheduler.",
                        "Make `python tools/recoil.py progress next` the only no-target Recoil.exe scheduler and label every other view deferred context.",
                    )
                )
        if raw_asm_or_bypass.search(line) and "not sufficient" not in line.lower():
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_number,
                    "raw-assembly-or-bypass",
                    "Raw assembly is allowed by failed C/C++ OR ChatGPT Pro instead of the binding evidence conjunction.",
                    "Require failed source-faithful variants and the canonical ChatGPT Pro/raw-assembly evidence gate whenever a hard trigger applies.",
                )
            )
        if not normalized.endswith("final_executable_repro_history.md"):
            lowered_line = line.lower()
            if "--owner-actions-json" in lowered_line or "--plan-actions-json" in lowered_line:
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_number,
                        "retired-final-action-batch",
                        "Surface references a retired final-data owner/plan action batch.",
                        "Emit/import the tracker-bound report and review explicit storage/section acceptance instead.",
                    )
                )
            if "final-data-layout" in lowered_line:
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_number,
                        "retired-final-work-unit",
                        "Surface references the retired final-data-layout work unit.",
                        "Describe audit final-data as a tracker-bound observed-evidence producer only.",
                    )
                )
            fabricated_extent = re.search(
                r"(?:unknown|unresolved)[^\n]{0,80}(?:address\s*\+\s*1|one[- ]byte)|one[- ]byte[^\n]{0,80}(?:unknown|unresolved)",
                lowered_line,
            )
            if fabricated_extent and not any(token in lowered_line for token in ("never", "do not", "must not", "reject", "forbid", "flag")):
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_number,
                        "fabricated-unknown-extent",
                        "Surface represents an unknown extent as a fabricated one-byte range.",
                        "Keep extent_state=unknown and omit size/end until exact evidence exists.",
                    )
                )

    lowered = text.lower()
    if normalized.endswith("recoil-binary-ninja-workflow/SKILL.md"):
        mutation_patterns = (
            r"\b(?:may|can|should|must)\s+(?:edit|rename|save|reanaly[sz]e|change)\b",
            r"\bsave_database\b",
        )
        if any(re.search(pattern, text, re.I) for pattern in mutation_patterns):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "bn-workflow-write-overlap",
                    "The Binary Ninja workflow skill grants mutation that belongs only to the BN reconstruction skill/role.",
                    "Keep this workflow read-only and route every BN edit, reanalysis, and save to recoil-binary-ninja-reconstruction/recoil_bn_reconstructor.",
                )
            )
    if normalized.endswith("recoil-provider-data-classifier.toml"):
        if re.search(r"\b(?:decide|assign|classify|accept)\b[^\n]{0,80}\bauthored (?:source )?owner\b", text, re.I) and "do not decide authored" not in lowered:
            findings.append(
                Finding("error", surface, 1, "provider-role-overlap", "Provider classifier decides authored source ownership.", "Return provider/non-authored/raw-extent facts or authored-candidate, then route authored owner boundaries to the source-owner mapper.")
            )
    if normalized.endswith("recoil-workspace-librarian.toml"):
        if re.search(r"\b(?:recommend|determine|choose|correct)\b[^\n]{0,100}\b(?:placement|source[- ]block|function order)\b", text, re.I) and not re.search(r"do not (?:recommend|determine|choose)", text, re.I):
            findings.append(
                Finding("error", surface, 1, "librarian-role-overlap", "Workspace librarian makes a new placement/order recommendation.", "Limit it to mechanical lookup of accepted durable facts and route interpretation to the source-owner mapper/parent.")
            )
    return findings


def parse_skill_frontmatter(text: str, surface: str) -> tuple[dict[str, str], list[Finding]]:
    match = FRONTMATTER_RE.match(text)
    if match is None:
        return {}, [
            Finding(
                "error",
                surface,
                1,
                "skill-frontmatter",
                "Skill is missing YAML-style frontmatter.",
                "Start SKILL.md with name and description frontmatter.",
            )
        ]

    metadata: dict[str, str] = {}
    findings: list[Finding] = []
    for index, line in enumerate(match.group("body").splitlines(), start=2):
        if not line.strip():
            continue
        if ":" not in line:
            findings.append(
                Finding("error", surface, index, "skill-frontmatter", f"Invalid frontmatter line `{line}`.")
            )
            continue
        key, value = line.split(":", 1)
        metadata[key.strip()] = value.strip()
    extra_keys = set(metadata) - {"name", "description"}
    if extra_keys:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "skill-frontmatter",
                f"Skill frontmatter has unsupported keys: {', '.join(sorted(extra_keys))}.",
                "Use only `name` and `description`.",
            )
        )
    for key in ("name", "description"):
        if not metadata.get(key):
            findings.append(
                Finding("error", surface, 1, "skill-frontmatter", f"Skill frontmatter missing `{key}`.")
            )
    return metadata, findings


def audit_skill(path: Path, root: Path) -> list[Finding]:
    surface = display_path(path, root)
    text = path.read_text(encoding="utf-8")
    metadata, findings = parse_skill_frontmatter(text, surface)
    findings.extend(audit_text(text, surface))
    description = metadata.get("description", "")
    if "Use when" not in description and "when" not in description:
        findings.append(
            Finding(
                "warning",
                surface,
                1,
                "skill-description",
                "Skill description does not clearly describe trigger conditions.",
                "Include concise trigger conditions because only metadata is loaded before skill selection.",
            )
        )
    if "AGENTS.md" not in text:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "skill-authority",
                "Skill does not point back to root AGENTS.md authority.",
                "Add a short root `AGENTS.md` authority note near the top.",
            )
        )
    if not (path.parent / "agents" / "openai.yaml").exists():
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "skill-agent-metadata",
                "Repo-local Recoil skill is missing `agents/openai.yaml` metadata.",
                "Add `.codex/skills/<skill>/agents/openai.yaml` with display metadata for local discovery.",
            )
        )
    if "## 0. Mission" in text or len(text) > 30000:
        findings.append(
            Finding(
                "warning",
                surface,
                1,
                "skill-duplication",
                "Skill appears to duplicate broad root instructions.",
                "Keep skills focused on workflow policy and defer full criteria to AGENTS.md.",
            )
        )
    return findings


def audit_role(path: Path, root: Path) -> list[Finding]:
    surface = display_path(path, root)
    text = path.read_text(encoding="utf-8")
    findings = audit_text(text, surface)
    if tomllib is None:
        findings.append(Finding("error", surface, 1, "role-toml", "tomllib is unavailable."))
        return findings
    try:
        data: dict[str, Any] = tomllib.loads(text)
    except tomllib.TOMLDecodeError as exc:  # type: ignore[union-attr]
        findings.append(Finding("error", surface, exc.lineno, "role-toml", f"Role TOML parse failed: {exc}"))
        return findings

    expected = EXPECTED_ROLES.get(path.name)
    if expected is None:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "role-unknown",
                "Unexpected Recoil role file.",
                "Route intentional role creation through agent-surface governance, reject broad/general roles, and register a narrow packet role in the audit.",
            )
        )
        return findings

    if data.get("name") != expected["name"]:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "role-name",
                f"Expected role name `{expected['name']}`, found `{data.get('name')}`.",
            )
        )
    if data.get("sandbox_mode") != expected["sandbox"]:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "role-sandbox",
                f"Expected sandbox `{expected['sandbox']}`, found `{data.get('sandbox_mode')}`.",
            )
        )
    for phrase in expected["required"]:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "role-boundary",
                    f"Role is missing boundary phrase `{phrase}`.",
                    "Keep packet role ownership explicit and non-overlapping.",
                )
            )
    for phrase in QUIET_ROLE_REQUIREMENTS:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "role-quiet-policy",
                    f"Role is missing quiet policy phrase `{phrase}`.",
                    "Role packets should suppress routine progress reports.",
                )
            )
    return findings


GIT_UNGOVERNED_MUTATION_RE = re.compile(
    r"(?ix)(?:"
    r"\bgit(?:\.exe)?\s+(?:add|commit|branch|switch|merge|cherry-pick)\b|"
    r"\bstag(?:e|ed|ing)?\s+(?:exact|reviewed|these|the|authored|packet|files?|paths?)\b"
    r")"
)


GIT_FORBIDDEN_POLICY_PATTERNS: tuple[tuple[str, re.Pattern[str]], ...] = (
    (
        "worker-git-control",
        re.compile(
            r"(?i)\bworkers?\b.{0,120}\b(?:may|can|is allowed to|are allowed to)\b"
            r".{0,120}\b(?:stage|commit|branch|merge|cherry-pick|reset|clean|restore|"
            r"stash|rebase|push|control git)\b"
        ),
    ),
    (
        "out-of-closure-staging",
        re.compile(r"(?i)\bstag(?:e|ed|ing)\b.{0,100}\boutside\b.{0,60}\bclosure\b"),
    ),
    ("destructive-reset", re.compile(r"(?i)\b(?:git\s+)?reset\s+--hard\b")),
    (
        "destructive-clean",
        re.compile(r"(?i)(?:\bgit\s+clean\b|\buse\s+clean\s+to\s+remove\b)"),
    ),
    (
        "discard-with-restore-checkout",
        re.compile(
            r"(?i)(?:\b(?:git\s+)?(?:restore|checkout)\b|\brestore/checkout\b)"
            r".{0,100}\b(?:discard|overwrite|remove)\b"
        ),
    ),
    (
        "stash-workflow-state",
        re.compile(r"(?i)(?:\bgit\s+stash\b|\b(?:hide|store)\b.{0,80}\bstash\b)"),
    ),
    (
        "ungoverned-rebase",
        re.compile(r"(?i)(?:\bgit\s+rebase\b|\brebase\b.{0,80}\bgoverned\s+history\b)"),
    ),
    (
        "force-push",
        re.compile(r"(?i)(?:\bgit\s+push\s+--force\b|\bforce[- ]push\b)"),
    ),
    (
        "direct-index-ref-manipulation",
        re.compile(
            r"(?i)(?:\bgit\s+(?:update-index|update-ref)\b|"
            r"\bdirect\s+(?:index|ref|index/ref)\s+manipulation\b)"
        ),
    ),
    ("git-hash-object", re.compile(r"(?i)\bgit\s+hash-object\b")),
    (
        "ignored-authored-surface",
        re.compile(
            r"(?i)\bignore\b.{0,100}\b(?:authored|maintained)\b.{0,80}"
            r"\b(?:source|tools?|tests?|polic(?:y|ies)|manifests?|configuration)\b"
        ),
    ),
    (
        "ignored-artifact-reconstruction-truth",
        re.compile(
            r"(?i)\b(?:use|rely\s+on|treat)\b.{0,100}\bignored\b.{0,80}"
            r"\b(?:artifact|file|output)s?\b.{0,100}"
            r"\b(?:retail|candidate|expected\s+truth|acceptance|manifest|profile)\b"
        ),
    ),
    (
        "git-retail-truth",
        re.compile(
            r"(?i)(?:\b(?:commit|object)(?:\s+(?:hash|id|identity))?\b.{0,100}"
            r"\b(?:prov(?:e|es|ed|ing)|establish(?:es|ed|ing)?|suppl(?:y|ies|ied)|qualif(?:y|ies|ied))\b"
            r".{0,80}\b(?:retail|equivalence|expected fact)\b|"
            r"\b(?:prov(?:e|es|ed|ing)|establish(?:es|ed|ing)?)\b.{0,80}"
            r"\bretail\s+equivalence\b.{0,80}\bcommit\b)"
        ),
    ),
    (
        "git-reconstruction-acceptance",
        re.compile(
            r"(?i)(?:\bpacket\s+commit\b.{0,100}\baccepts?\b.{0,100}"
            r"\b(?:call contracts?|function order|bytes?|profiles?|owners?|tiers?|providers?|phase)\b|"
            r"\baccepts?\b.{0,100}\b(?:call contracts?|function order|bytes?|profiles?|owners?|tiers?|providers?|phase)\b"
            r".{0,100}\b(?:because|from)\b.{0,40}\bcommit\b)"
        ),
    ),
    (
        "discard-user-changes",
        re.compile(
            r"(?i)\b(?:silently\s+)?discard(?:ing)?\b.{0,100}\b(?:user|pre-existing)\s+changes\b"
        ),
    ),
)


def _line_is_explicit_git_prohibition(lines: list[str], index: int) -> bool:
    line = lines[index]
    if GIT_NEGATION_RE.search(line):
        return True
    previous = lines[index - 1] if index > 0 else ""
    return bool(
        re.search(
            r"(?i)\b(?:prohibited|forbidden|must not|never|reject(?:s|ed)?)\b",
            previous,
        )
        and line.lstrip().startswith(("-", "*", "`"))
    )


def _parent_tool_git_context(lines: list[str], index: int) -> bool:
    context = " ".join(lines[max(0, index - 8): min(len(lines), index + 9)])
    return bool(
        re.search(
            r"(?i)(?:\bparent/tool\b|\bparent[- ]controlled\b|"
            r"\bparent(?:\s+reviews?|\s+may|\s+owns?)\b|\btool[- ]governed\b)",
            context,
        )
    )


def _worker_packet_commit_context(lines: list[str], index: int) -> bool:
    context = " ".join(lines[max(0, index - 8): min(len(lines), index + 9)])
    return bool(
        re.search(r"(?i)worker", context)
        and re.search(r"(?i)exact(?: handed-off)? writable closure", context)
        and re.search(r"(?i)(?:exactly one|one) nonaccepting packet(?:-id)? commit", context)
        and re.search(r"(?i)packet id", context)
    )


def _audit_git_governance_requirements(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    normalized_surface = surface.replace("\\", "/")
    groups = GIT_GOVERNANCE_REQUIREMENT_GROUPS.get(normalized_surface, ())
    for group_name, alternatives in groups:
        if any(has_required_phrase(text, phrase) for phrase in alternatives):
            continue
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "git-policy-required-group",
                f"Surface is missing governed Git policy group `{group_name}`.",
                "Preserve native Git workspace control, parent/tool ownership, worker write boundaries, SQLite CAS authority, direct retail evidence, and nonaccepting opaque Git state.",
            )
        )
    return findings


def audit_git_policy(text: str, surface: str) -> list[Finding]:
    findings = _audit_git_governance_requirements(text, surface)
    lines = text.splitlines()
    for zero_index, line in enumerate(lines):
        prohibited = _line_is_explicit_git_prohibition(lines, zero_index)
        for rule_name, pattern in GIT_FORBIDDEN_POLICY_PATTERNS:
            if pattern.search(line) is None or prohibited:
                continue
            if rule_name == "worker-git-control" and _worker_packet_commit_context(
                lines, zero_index
            ):
                continue
            findings.append(
                Finding(
                    "error",
                    surface,
                    zero_index + 1,
                    "git-policy",
                    f"Agent-facing surface permits unsafe Git policy `{rule_name}`.",
                    "Keep Git mutation parent/tool-governed, closure-bounded, nondestructive, and nonaccepting; preserve user changes and direct retail evidence.",
                )
            )
        if (
            GIT_UNGOVERNED_MUTATION_RE.search(line)
            and not prohibited
            and not _parent_tool_git_context(lines, zero_index)
            and not _worker_packet_commit_context(lines, zero_index)
        ):
            findings.append(
                Finding(
                    "error",
                    surface,
                    zero_index + 1,
                    "git-policy",
                    "Agent-facing surface contains an ungoverned Git mutation instruction.",
                    "Assign packet branch, exact staging, nonaccepting commit, and reviewed integration operations to parent/tool governance.",
                )
            )
    return findings


def default_surfaces(root: Path, *, active_only: bool) -> list[Path]:
    surfaces = [
        root / "AGENTS.md",
        root / "CLAUDE.md",
        root / "README.md",
        root / ".agent" / "AGENTS.md",
        root / "tools" / "README.md",
        root / "tools" / "vc5_verify_targets" / "README.md",
        root / "docs" / "reconstruction" / "agent_launch_checklist.md",
    ]
    if not active_only:
        surfaces.extend(sorted((root / "docs" / "reconstruction").glob("*.md")))
    surfaces.extend(sorted((root / ".codex" / "skills").glob("recoil-*/SKILL.md")))
    surfaces.extend(sorted((root / ".codex" / "agents").glob("*.toml")))

    result: list[Path] = []
    seen: set[Path] = set()
    for path in surfaces:
        resolved = path.resolve()
        if resolved in seen or not path.exists():
            continue
        seen.add(resolved)
        result.append(path)
    return result


def audit_required_physical_data_commands(specs: Any) -> list[Finding]:
    present = {tuple(item.path) for item in specs}
    return [
        Finding(
            "error",
            "tools/recoil.py",
            1,
            "required-physical-data-command",
            f"Required physical-data command `{' '.join(path)}` is not registered.",
            "Register the reviewed typed-catalog view or fresh live final-image command before documenting it.",
        )
        for path in sorted(REQUIRED_PHYSICAL_DATA_COMMANDS - present)
    ]


def audit_registry(root: Path) -> list[Finding]:
    findings: list[Finding] = []
    findings.extend(audit_required_physical_data_commands(recoil.COMMAND_SPECS))
    seen_paths: set[tuple[str, ...]] = set()
    seen_backends: dict[tuple[str, tuple[str, ...]], recoil.CommandSpec] = {}
    for item in recoil.COMMAND_SPECS:
        if item.path in seen_paths:
            findings.append(
                Finding("error", "tools/recoil.py", 1, "registry", f"Duplicate command path `{item.name}`.")
            )
        seen_paths.add(item.path)
        backend = (item.module, item.prepend_args)
        existing_backend = seen_backends.get(backend)
        if existing_backend is not None:
            findings.append(
                Finding(
                    "error",
                    "tools/recoil.py",
                    1,
                    "registry",
                    f"Commands `{existing_backend.name}` and `{item.name}` duplicate internal backend `{item.module}`.",
                    "Keep one canonical gate command for each exact backend/prepend pair.",
                )
            )
        seen_backends[backend] = item
        if item.category not in VALID_CATEGORIES:
            findings.append(
                Finding(
                    "error",
                    "tools/recoil.py",
                    1,
                    "registry",
                    f"Command `{item.name}` has invalid category `{item.category}`.",
                )
            )
        module_path = root / "tools" / "_recoil" / "commands" / f"{item.module}.py"
        if not module_path.exists():
            findings.append(
                Finding(
                    "error",
                    "tools/recoil.py",
                    1,
                    "registry",
                    f"Command `{item.name}` maps to missing internal module `{display_path(module_path, root)}`.",
                )
            )
        if not item.summary:
            findings.append(
                Finding("error", "tools/recoil.py", 1, "registry", f"Command `{item.name}` has no summary.")
            )
        for example in item.examples:
            if not example.startswith("python tools/recoil.py "):
                findings.append(
                    Finding(
                        "error",
                        "tools/recoil.py",
                        1,
                        "registry",
                        f"Command `{item.name}` has non-gate example `{example}`.",
                    )
                )
                continue
            args = split_command_args(example[len("python tools/recoil.py ") :])
            resolved, _rest = recoil.resolve_command(args)
            if resolved != item:
                findings.append(
                    Finding(
                        "error",
                        "tools/recoil.py",
                        1,
                        "registry",
                        f"Example for `{item.name}` resolves to `{resolved.name if resolved else '<none>'}`.",
                    )
                )
    return findings


def audit_orchestration_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = ORCHESTRATION_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "orchestration-policy",
                    f"Surface is missing orchestration policy phrase `{phrase}`.",
                    "Keep the parent as orchestrator and require `recoil_source_worker` for implementation.",
                )
            )
    return findings


def audit_start_contract_policy(text: str, surface: str) -> list[Finding]:
    """Keep bare-Start guidance autonomous and multi-lane on every launch surface."""

    findings: list[Finding] = []
    required = START_CONTRACT_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "start-contract",
                    f"Surface is missing bare-Start contract phrase `{phrase}`.",
                    "Document autonomous claim-current --lane all launch, runtime slot capacity, fixed lane priority, packet-id handoff, and tool-owned conflict skips.",
                )
            )
    if "--lane primary" in text and "--lane all" not in text:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "stale-primary-only-start",
                "Launch guidance still exposes only the primary lane for a no-target Start.",
                "Use claim-current --lane all for bare Start; retain individual lane claims only for focused retries or explicit assignments.",
            )
        )
    return findings


def audit_quiet_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = QUIET_POLICY_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "quiet-policy",
                    f"Surface is missing quiet policy phrase `{phrase}`.",
                    "Keep routine progress narration suppressed for Recoil reconstruction work.",
                )
            )
    return findings


def audit_tool_maintainer_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = TOOL_MAINTAINER_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "tool-maintainer-policy",
                    f"Surface is missing tool maintainer policy phrase `{phrase}`.",
                    "Require `recoil_tool_maintainer` delegation for workspace issues and tool upgrades.",
                )
            )
    return findings


def audit_agent_surface_governance_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = AGENT_SURFACE_GOVERNANCE_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "agent-surface-governance",
                    f"Surface is missing agent-surface governance phrase `{phrase}`.",
                    "Document governed skill/role/tool evolution without weakening evidence, marker, provider, source, or subagent boundaries.",
                )
            )
    return findings


def audit_session_scratch_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = SESSION_SCRATCH_POLICY_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "session-scratch-policy",
                    f"Surface is missing session scratch phrase `{phrase}`.",
                    "Keep .devspace parent-owned and temporary, promote material evidence, and clean it at session boundaries after every consumer exits.",
                )
            )
    return findings


def audit_startup_binary_target_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = STARTUP_BINARY_TARGET_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "startup-binary-target-policy",
                    f"Surface is missing global text-pipeline phrase `{phrase}`.",
                    "Require progress next, independent authored-order and authored-byte lanes, the full-order restart, the linked-byte join, exact typed final-image semantics with a diagnostic COFF timestamp, and deferred ordinary messages.dll work.",
                )
            )
    return findings


def audit_global_text_pipeline_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = GLOBAL_TEXT_PIPELINE_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "global-text-pipeline-policy",
                    f"Surface is missing global pipeline phrase `{phrase}`.",
                    "Keep progress next, the canonical six stages with authored call contracts between authored and full order, independent authored-byte traversal, exact retail identity, role-specific first-divergence behavior, and debt-free closeout aligned.",
                )
            )
    return findings


def audit_source_block_order_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = SOURCE_BLOCK_ORDER_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "source-block-order-policy",
                    f"Surface is missing source block-order phrase `{phrase}`.",
                    "Preserve guidance that BN source-path literal xrefs can prove physical source-file blocks and that VC5 COFF function order must match retail BN order for byte-ready block work.",
                )
            )
    return findings


def audit_source_shape_contract_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = SOURCE_SHAPE_CONTRACT_REQUIREMENTS.get(surface)
    if surface == ".codex/agents/recoil-source-worker.toml" and "BN function names and comments are provisional navigation labels" in text:
        required = (
            "BN function names and comments are provisional navigation labels",
            "current production `src/` tree as implementation state, not original-source authority",
            "rely on declared object order",
            "Passing smokes, byte checks, or ABI call-shape checks are evidence candidates",
        )
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "source-shape-contract-policy",
                    f"Surface is missing source-shape contract phrase `{phrase}`.",
                    "Preserve BN evidence authority, provisional BN labels, source-block-led `src` reshaping, natural VC5 function-order matching, and no forced placement tricks.",
                )
            )
    return findings


def audit_required_policy_phrases(
    text: str,
    surface: str,
    requirements: dict[str, tuple[str, ...]],
    kind: str,
    remediation: str,
) -> list[Finding]:
    """Check one canonical cross-surface policy without copying its full body."""

    findings: list[Finding] = []
    required = requirements.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    kind,
                    f"Surface is missing required policy phrase `{phrase}`.",
                    remediation,
                )
            )
    return findings


def audit_current_policy_updates(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = audit_start_contract_policy(text, surface)
    for requirements, kind, remediation in (
        (
            CALL_CONTRACT_STAGE_REQUIREMENTS,
            "call-contract-stage",
            "Document the accepted-authored-order-derived call-contract census, <=160-body verifier slices, narrow call_contract acceptance, and parent CAS command; keep 3,380 only as reviewed one-time migration history.",
        ),
        (
            PRIMARY_LANE_CONTRACT_REQUIREMENTS,
            "primary-lane-contract",
            "Document the six-stage pipeline: the primary lane runs authored order, authored call contracts, then full order without waiting for authored bytes; join full order and authored bytes before linked bytes.",
        ),
        (
            PRO_BROKER_CONTRACT_REQUIREMENTS,
            "pro-broker-contract",
            "Workers package and release; only the parent deduplicates, uploads, calls Pro single-flight, and resumes distinct request kinds.",
        ),
        (
            RESOURCE_LEASE_CONTRACT_REQUIREMENTS,
            "resource-lease-contract",
            "Require normalized claims, reservation/lease ids, current work-leases inspection, and symmetric conflict handling across every shared resource.",
        ),
        (
            BN_LEASE_CONTRACT_REQUIREMENTS,
            "bn-lease-contract",
            "Allow readers only on a stable saved view with no writer, and serialize one writer through reanalysis, propagation, save, return, and lease release.",
        ),
        (
            SOURCE_MODEL_TRACKER_AUTHORITY_REQUIREMENTS,
            "source-model-tracker-authority",
            "Workers propose exact changes; only the parent dry-runs/applies revision-bound tracker mutations and returns the structured handoff before source edits.",
        ),
        (
            OBJECT_CURSOR_NOTE_REQUIREMENTS,
            "object-cursor-note-contract",
            "Record the subordinate object cursor only when returned and recompute progress next after each object-only packet.",
        ),
    ):
        findings.extend(
            audit_required_policy_phrases(
                text,
                surface,
                requirements,
                kind,
                remediation,
            )
        )
    findings.extend(
        audit_required_policy_phrases(
            text,
            surface,
            OBJECT_ORDER_GATE_REQUIREMENTS,
            "object-order-gate-policy",
            "Require expected object identities exactly once in relative order, inventory raw extras without gating them, and keep selected linked groups/seams exact.",
        )
    )
    findings.extend(
        audit_required_policy_phrases(
            text,
            surface,
            BN_MAINTAINED_ARTIFACT_REQUIREMENTS,
            "bn-maintained-artifact-policy",
            "State that bounded parent-assigned BN reconstruction may edit/reanalyze/save the already-open database without broadening decision authority.",
        )
    )
    findings.extend(
        audit_required_policy_phrases(
            text,
            surface,
            CANONICAL_MFC_HEADER_REQUIREMENTS,
            "canonical-mfc-header-policy",
            "Keep the official project/build AFXWIN.H under the VC5SP3 Compiler root and treat other MFC header trees/libraries as evidence or diagnostics only.",
        )
    )
    findings.extend(
        audit_required_policy_phrases(
            text,
            surface,
            PROVIDER_TARGET_REGISTRATION_REQUIREMENTS,
            "provider-target-registration-policy",
            "Use the registered dry-run-first provider-target command for retail-proven named IAT slots, preserve exact provider views, and bind call sites separately without candidate-derived truth.",
        )
    )
    findings.extend(
        audit_required_policy_phrases(
            text,
            surface,
            SAFETY_CRITICAL_SEMANTIC_REQUIREMENTS,
            "safety-critical-semantic-policy",
            "Restore canonical authored-role gates, raw-extra handling, classifier routing, independent byte-lane aliases, and serialized BN/Pro coordination.",
        )
    )
    return findings


def audit_hard_byte_match_chatgpt_pro_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = HARD_BYTE_MATCH_CHATGPT_PRO_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "hard-byte-match-chatgpt-pro-policy",
                    f"Surface is missing hard byte-match ChatGPT Pro policy phrase `{phrase}`.",
                    "Require hard byte-match work to use a ChatGPT Pro reasoning pass while preserving source-faithful C/C++ first, narrow inline-asm exceptions, and owner/tier evidence boundaries.",
                )
            )
    return findings


def audit_byte_match_triplet_producer_policy(text: str, surface: str) -> list[Finding]:
    """Require complete retail-assembly production without broadening fact-only roles."""

    findings: list[Finding] = []
    required = BYTE_MATCH_TRIPLET_PRODUCER_REQUIREMENTS.get(surface)
    if surface == ".codex/agents/recoil-bn-fact-mapper.toml" and "complete address-labeled retail assembly" in text:
        required = (
            "complete address-labeled retail assembly",
            "correct retail binary",
            "current BN function body",
            "target binary",
            "address extent",
            "truncation caveat",
            "do not substitute",
            "exact current C/C++ reimplementation",
            "complete current compiled assembly",
            "parent/source worker/verifier",
        )
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "byte-match-triplet-producer-policy",
                    f"Surface is missing byte-match triplet producer phrase `{phrase}`.",
                    "Require the BN workflow/fact mapper to provide complete address-labeled retail assembly and route exact current source plus complete compiled assembly to the parent/source worker/verifier.",
                )
            )
    return findings


def audit_source_discovery_chatgpt_pro_policy(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    required = SOURCE_DISCOVERY_CHATGPT_PRO_REQUIREMENTS.get(surface)
    if required is None:
        return findings
    for phrase in required:
        if not has_required_phrase(text, phrase):
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "source-discovery-chatgpt-pro-policy",
                    f"Surface is missing source-discovery ChatGPT Pro policy phrase `{phrase}`.",
                    "Require source-discovery owner/block/order determinations to use ChatGPT Pro while preserving lookup and raw BN fact exemptions, transcript evidence, and advisory-only evidence boundaries.",
                )
            )
    return findings


def audit_root_tool_entrypoints(root: Path) -> list[Finding]:
    findings: list[Finding] = []
    for path in sorted((root / "tools").glob("*.py")):
        if path.name == "recoil.py":
            continue
        findings.append(
            Finding(
                "error",
                "tools",
                1,
                "root-tool-entrypoint",
                f"Root-level Python tool `{display_path(path, root)}` bypasses the unified gate.",
                "Move implementation code under `tools/_recoil` and expose it through `python tools/recoil.py`.",
            )
        )
    return findings


def audit_tool_readme_coverage(path: Path, root: Path) -> list[Finding]:
    surface = display_path(path, root)
    text = path.read_text(encoding="utf-8")
    findings: list[Finding] = []
    for item in recoil.COMMAND_SPECS:
        command_ref = f"python tools/recoil.py {item.name}"
        shorthand_ref = f"recoil.py {item.name}"
        if command_ref in text or shorthand_ref in text:
            continue
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "readme-command-coverage",
                f"`tools/README.md` does not document registered command `{item.name}`.",
                f"Add `{command_ref}` to the complete command index or a command-specific section.",
            )
        )
    return findings


def readme_command_index_rows(text: str, surface: str) -> tuple[list[dict[str, object]], list[Finding]]:
    match = README_INDEX_HEADER_RE.search(text)
    if match is None:
        return [], [
            Finding(
                "error",
                surface,
                1,
                "readme-command-index",
                "`tools/README.md` is missing the Complete Command Index section.",
                "Add a table generated from `python tools/recoil.py commands --json`.",
            )
        ]

    rows: list[dict[str, object]] = []
    findings: list[Finding] = []
    in_table = False
    for line_number, line in enumerate(text[match.end() :].splitlines(), start=line_for_offset(text, match.end())):
        if not line.strip():
            if in_table:
                break
            continue
        row_match = README_TABLE_ROW_RE.match(line)
        if row_match is None:
            if in_table:
                break
            continue
        cells = [cell.strip() for cell in row_match.group("cells").split("|")]
        if cells == ["Command", "Category", "Mutates", "BN", "Purpose"]:
            in_table = True
            continue
        if len(cells) == 5 and set(cells[0]) <= {"-", " "}:
            continue
        if not in_table:
            continue
        if len(cells) != 5:
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_number,
                    "readme-command-index",
                    "Complete Command Index row does not have five columns.",
                    "Use Command, Category, Mutates, BN, and Purpose columns.",
                )
            )
            continue
        command = cells[0]
        if command.startswith("`") and command.endswith("`"):
            command = command[1:-1]
        rows.append(
            {
                "line": line_number,
                "command": command,
                "category": cells[1],
                "mutates": cells[2],
                "bn": cells[3],
                "summary": cells[4],
            }
        )
    if not in_table:
        findings.append(
            Finding(
                "error",
                surface,
                line_for_offset(text, match.start()),
                "readme-command-index",
                "Complete Command Index does not contain a command table.",
                "Add a table generated from `python tools/recoil.py commands --json`.",
            )
        )
    return rows, findings


def audit_tool_readme_index(path: Path, root: Path) -> list[Finding]:
    surface = display_path(path, root)
    text = path.read_text(encoding="utf-8")
    rows, findings = readme_command_index_rows(text, surface)
    if not rows:
        return findings

    expected = [
        {
            "command": f"python tools/recoil.py {item.name}",
            "category": item.category,
            "mutates": "yes" if item.mutates else "no",
            "bn": "yes" if item.needs_binja else "no",
            "summary": item.summary,
        }
        for item in sorted(recoil.COMMAND_SPECS, key=lambda spec_item: spec_item.path)
    ]
    actual_by_command = {str(row["command"]): row for row in rows}
    expected_by_command = {str(row["command"]): row for row in expected}

    for command in expected_by_command:
        if command not in actual_by_command:
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "readme-command-index-missing",
                    f"Complete Command Index is missing `{command}`.",
                    "Regenerate the Complete Command Index from `python tools/recoil.py commands --json`.",
                )
            )
    for command, row in actual_by_command.items():
        if command not in expected_by_command:
            findings.append(
                Finding(
                    "error",
                    surface,
                    int(row["line"]),
                    "readme-command-index-extra",
                    f"Complete Command Index contains unregistered command `{command}`.",
                    "Remove stale rows or register the command in tools/recoil.py.",
                )
            )

    for index, expected_row in enumerate(expected):
        if index >= len(rows):
            break
        actual_row = rows[index]
        if actual_row["command"] != expected_row["command"]:
            findings.append(
                Finding(
                    "error",
                    surface,
                    int(actual_row["line"]),
                    "readme-command-index-order",
                    f"Complete Command Index row {index + 1} is `{actual_row['command']}`, expected `{expected_row['command']}`.",
                    "Match the ordering from `python tools/recoil.py commands --json`.",
                )
            )

    for command, actual_row in actual_by_command.items():
        expected_row = expected_by_command.get(command)
        if expected_row is None:
            continue
        for key, label in (("category", "Category"), ("mutates", "Mutates"), ("bn", "BN"), ("summary", "Purpose")):
            if actual_row[key] != expected_row[key]:
                findings.append(
                    Finding(
                        "error",
                        surface,
                        int(actual_row["line"]),
                        "readme-command-index-metadata",
                        f"`{command}` {label} is `{actual_row[key]}`, expected `{expected_row[key]}`.",
                        "Regenerate the Complete Command Index from `python tools/recoil.py commands --json`.",
                    )
                )
    return findings


def audit_readme_address_led_examples(text: str, surface: str) -> list[Finding]:
    findings: list[Finding] = []
    for match in GATE_COMMAND_RE.finditer(text):
        line_start = text.rfind("\n", 0, match.start()) + 1
        line_end = text.find("\n", match.start())
        if line_end == -1:
            line_end = len(text)
        if text[line_start:line_end].lstrip().startswith("|"):
            continue
        raw_args = match.group("args")
        args = split_command_args(raw_args)
        if not args:
            continue
        if args[0] == "status" and not any(re.fullmatch(r"0x[0-9A-Fa-fNn]+", arg) for arg in args[1:]):
            findings.append(
                Finding(
                    "error",
                    surface,
                    line_for_offset(text, match.start()),
                    "readme-address-led-example",
                    "`tools/README.md` shows a bare `status` example.",
                    "Use an address-bearing example such as `python tools/recoil.py progress show 0xNNNNNN`.",
                )
            )
        if args[0] == "packet":
            address_index = args.index("--address") + 1 if "--address" in args else -1
            has_address = address_index > 0 and address_index < len(args) and re.fullmatch(
                r"0x[0-9A-Fa-fNn]+", args[address_index]
            )
            if not has_address:
                findings.append(
                    Finding(
                        "error",
                        surface,
                        line_for_offset(text, match.start()),
                        "readme-address-led-example",
                        "`tools/README.md` shows a bare `packet` example.",
                        "Render the real active reservation with `python tools/recoil.py progress handoff --packet-id <packet-id> --json`.",
                    )
                )
    return findings


def audit_expected_roles(root: Path, role_paths: list[Path]) -> list[Finding]:
    findings: list[Finding] = []
    present = {path.name for path in role_paths}
    for filename in EXPECTED_ROLES:
        if filename not in present:
            findings.append(
                Finding(
                    "error",
                    ".codex/agents",
                    1,
                    "role-missing",
                    f"Expected role file `{filename}` is missing.",
                )
            )
    return findings


def parse_frontmatter_scalars(text: str) -> dict[str, str] | None:
    """Parse a leading YAML frontmatter block into raw scalar strings.

    Values are returned exactly as written, including any surrounding quotes, so
    a mirror comparison rejects a requoted description as drift.
    """

    match = FRONTMATTER_RE.match(text)
    if match is None:
        return None
    scalars: dict[str, str] = {}
    for line in match.group("body").splitlines():
        if not line.strip() or ":" not in line:
            continue
        key, value = line.split(":", 1)
        scalars[key.strip()] = value.strip()
    return scalars


def audit_claude_pointer_stub(
    path: Path,
    surface: str,
    *,
    expected_name: str,
    canonical_reference: str,
    expected_description: str | None,
    kind_prefix: str,
) -> list[Finding]:
    """Check one Claude pointer stub against its canonical `.codex` source."""

    findings: list[Finding] = []
    text = path.read_text(encoding="utf-8")
    scalars = parse_frontmatter_scalars(text)
    if scalars is None:
        return [
            Finding(
                "error",
                surface,
                1,
                f"{kind_prefix}-frontmatter",
                "Claude pointer stub is missing YAML-style frontmatter.",
                "Start the stub with `name` and `description` frontmatter so the harness can route it.",
            )
        ]
    if scalars.get("name") != expected_name:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                f"{kind_prefix}-name",
                f"Claude pointer stub declares name `{scalars.get('name')}`, expected `{expected_name}`.",
                "Mirror the canonical identity with `_` replaced by `-`; identity comes only from the `name` field.",
            )
        )
    if expected_description is not None and scalars.get("description", "") != expected_description:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                f"{kind_prefix}-description-drift",
                "Claude pointer stub description differs from the canonical description.",
                f"Copy the canonical `description:` value verbatim from `{canonical_reference}`.",
            )
        )
    if canonical_reference not in text:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                f"{kind_prefix}-pointer",
                f"Claude pointer stub does not name its canonical source `{canonical_reference}`.",
                "Point the body at the canonical path and require reading it verbatim.",
            )
        )
    if "AGENTS.md" not in text:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                f"{kind_prefix}-authority",
                "Claude pointer stub does not point back to root AGENTS.md authority.",
                "Add a short root `AGENTS.md` authority note.",
            )
        )
    line_count = len(text.splitlines())
    if line_count > CLAUDE_STUB_MAX_LINES:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                f"{kind_prefix}-stub-bloat",
                f"Claude pointer stub has {line_count} lines; the pointer limit is {CLAUDE_STUB_MAX_LINES}.",
                "Keep policy in the canonical `.codex` surface; the stub may only route to it.",
            )
        )
    if kind_prefix == "claude-role":
        match = FRONTMATTER_RE.match(text)
        assert match is not None
        expected_body = (
            "\nRoot `AGENTS.md` is authoritative. Your complete operating contract is the\n"
            f"`developer_instructions` value in `{canonical_reference}`.\n"
            "Read that file first and follow it verbatim; it is the only definition of your\n"
            "scope, stop condition, and return format. This stub adds no policy of its own.\n"
        )
        body = text[match.end() :].replace("\r\n", "\n")
        if body != expected_body:
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "claude-role-thick-pointer",
                    "Claude role mirror contains body text beyond the exact canonical pointer.",
                    "Keep only the root authority sentence, canonical developer_instructions pointer, and no-policy statement.",
                )
            )
    return findings


def audit_claude_settings(root: Path) -> list[Finding]:
    """Require the mechanical Claude-side gates over ledgers and retail input."""

    surface = ".claude/settings.json"
    path = root / ".claude" / "settings.json"
    if not path.exists():
        return [
            Finding(
                "error",
                surface,
                1,
                "claude-settings-missing",
                "Claude project settings are missing.",
                "Add `.claude/settings.json` with deny rules over the progress tracker, the issue ledger, and `support/`.",
            )
        ]
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        return [
            Finding(
                "error",
                surface,
                1,
                "claude-settings-json",
                f"Claude project settings could not be parsed: {exc}",
                "Keep `.claude/settings.json` valid JSON.",
            )
        ]

    findings: list[Finding] = []
    permissions = data.get("permissions")
    permissions = permissions if isinstance(permissions, dict) else {}
    deny = permissions.get("deny")
    deny = [item for item in deny if isinstance(item, str)] if isinstance(deny, list) else []
    missing = [rule for rule in CLAUDE_REQUIRED_DENY_RULES if rule not in deny]
    if missing:
        findings.append(
            Finding(
                "error",
                surface,
                1,
                "claude-settings-deny",
                f"Claude project settings are missing deny rule(s): {', '.join(missing)}.",
                "Mutate the tracker only through `python tools/recoil.py progress ...` and keep retail input immutable.",
            )
        )
    for section in ("allow", "ask", "deny"):
        rules = permissions.get(section)
        if not isinstance(rules, list):
            continue
        for rule in rules:
            if isinstance(rule, str) and INEFFECTIVE_FILE_RULE_RE.match(rule):
                findings.append(
                    Finding(
                        "error",
                        surface,
                        1,
                        "claude-settings-ineffective-rule",
                        f"Permission rule `{rule}` is accepted but never applied to file checks.",
                        "Use an `Edit(path)` rule, which covers every file-editing tool.",
                    )
                )
    return findings


def audit_claude_mirror(root: Path) -> list[Finding]:
    """Prove the Claude pointer surface still mirrors the canonical Codex one."""

    findings: list[Finding] = []

    canonical_skills = {
        path.parent.name: path
        for path in sorted((root / ".codex" / "skills").glob("recoil-*/SKILL.md"))
    }
    mirror_skills = {
        path.parent.name: path
        for path in sorted((root / ".claude" / "skills").glob("*/SKILL.md"))
    }
    for name, canonical in canonical_skills.items():
        surface = f".claude/skills/{name}/SKILL.md"
        mirror = mirror_skills.get(name)
        if mirror is None:
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "claude-skill-missing",
                    f"Canonical skill `{name}` has no Claude pointer stub.",
                    "Add the stub with the verbatim canonical description and a pointer to the canonical SKILL.md.",
                )
            )
            continue
        canonical_scalars = parse_frontmatter_scalars(canonical.read_text(encoding="utf-8"))
        findings.extend(
            audit_claude_pointer_stub(
                mirror,
                surface,
                expected_name=name,
                canonical_reference=f".codex/skills/{name}/SKILL.md",
                expected_description=(
                    None if canonical_scalars is None else canonical_scalars.get("description", "")
                ),
                kind_prefix="claude-skill",
            )
        )
    for name in sorted(set(mirror_skills) - set(canonical_skills)):
        findings.append(
            Finding(
                "error",
                f".claude/skills/{name}/SKILL.md",
                1,
                "claude-skill-orphan",
                f"Claude skill `{name}` has no canonical `.codex/skills/{name}/SKILL.md` source.",
                "Remove the orphaned stub or add the canonical skill it should point at.",
            )
        )

    canonical_roles = {
        path.stem: path for path in sorted((root / ".codex" / "agents").glob("*.toml"))
    }
    mirror_roles = {
        path.stem: path for path in sorted((root / ".claude" / "agents").glob("*.md"))
    }
    for stem, canonical in canonical_roles.items():
        surface = f".claude/agents/{stem}.md"
        mirror = mirror_roles.get(stem)
        if mirror is None:
            findings.append(
                Finding(
                    "error",
                    surface,
                    1,
                    "claude-role-missing",
                    f"Canonical role `{canonical.name}` has no Claude subagent definition.",
                    "Add the subagent stub that defers to the canonical `developer_instructions`.",
                )
            )
            continue
        expected_name = stem
        expected_description: str | None = None
        if tomllib is not None:
            try:
                role = tomllib.loads(canonical.read_text(encoding="utf-8"))
            except tomllib.TOMLDecodeError:  # type: ignore[union-attr]
                role = {}
            if isinstance(role.get("name"), str):
                expected_name = role["name"].replace("_", "-")
            if isinstance(role.get("description"), str):
                expected_description = role["description"]
        findings.extend(
            audit_claude_pointer_stub(
                mirror,
                surface,
                expected_name=expected_name,
                canonical_reference=f".codex/agents/{canonical.name}",
                expected_description=expected_description,
                kind_prefix="claude-role",
            )
        )
    for stem in sorted(set(mirror_roles) - set(canonical_roles)):
        findings.append(
            Finding(
                "error",
                f".claude/agents/{stem}.md",
                1,
                "claude-role-orphan",
                f"Claude subagent `{stem}` has no canonical `.codex/agents/{stem}.toml` contract.",
                "Remove the orphaned definition or add the canonical role contract it should defer to.",
            )
        )

    memory = root / "CLAUDE.md"
    if not memory.exists():
        findings.append(
            Finding(
                "error",
                "CLAUDE.md",
                1,
                "claude-md-import",
                "Claude Code reads CLAUDE.md, not AGENTS.md, and CLAUDE.md is missing.",
                "Add a root `CLAUDE.md` whose first line is the `@AGENTS.md` import.",
            )
        )
    elif CLAUDE_MEMORY_IMPORT_RE.search(memory.read_text(encoding="utf-8")) is None:
        findings.append(
            Finding(
                "error",
                "CLAUDE.md",
                1,
                "claude-md-import",
                "CLAUDE.md does not import root AGENTS.md on its own line.",
                "Import the canonical instructions with a bare `@AGENTS.md` line instead of restating them.",
            )
        )

    findings.extend(audit_claude_settings(root))
    return findings


def _count_known_references(value: Any, references: set[str]) -> Counter[str]:
    """Count exact references without interpreting unrelated tracker strings."""

    counts: Counter[str] = Counter()
    if isinstance(value, str):
        for reference in references:
            counts[reference] += value.count(reference)
    elif isinstance(value, dict):
        for child in value.values():
            counts.update(_count_known_references(child, references))
    elif isinstance(value, list):
        for child in value:
            counts.update(_count_known_references(child, references))
    return counts


def find_active_durable_devspace_references(root: Path) -> list[DurableReference]:
    """Keep active scratch dependencies while ignoring non-gating tracker history.

    The tracker can retain migrated observations and evidence tombstones for
    semantic history.  Those records cannot qualify current validation and do
    not make their former scratch paths active dependencies.  Any occurrence
    in current tracker state or a current/gating evidence record still fails.
    """

    findings = find_durable_devspace_references(root)
    tracker_path = progress_tracker_path(root).resolve()
    tracker_findings = [item for item in findings if item.path.resolve() == tracker_path]
    if not tracker_findings:
        return findings

    tracker = load_progress_tracker_data(root)
    if tracker is None:
        return findings

    known_references = {item.reference for item in tracker_findings}
    active_counts = _count_known_references(
        {
            key: value
            for key, value in tracker.items()
            if key not in {"evidence", "tombstones"}
        },
        known_references,
    )
    historical_counts: Counter[str] = Counter()

    evidence = tracker.get("evidence")
    if isinstance(evidence, dict):
        for record in evidence.values():
            is_historical_non_gating = (
                isinstance(record, dict)
                and record.get("gating") is False
                and (
                    record.get("freshness") == "historical"
                    or record.get("validation_mode") == "historical-observation"
                )
            )
            counts = _count_known_references(record, known_references)
            if is_historical_non_gating:
                historical_counts.update(counts)
            else:
                active_counts.update(counts)

    tombstones = tracker.get("tombstones")
    if isinstance(tombstones, dict):
        historical_counts.update(_count_known_references(tombstones, known_references))

    filtered = [item for item in findings if item.path.resolve() != tracker_path]
    for reference in sorted(known_references):
        matching = [item for item in tracker_findings if item.reference == reference]
        active_count = min(active_counts[reference], len(matching))
        historical_count = min(
            historical_counts[reference],
            len(matching) - active_count,
        )
        filtered.extend(matching[:active_count])
        filtered.extend(matching[active_count + historical_count :])
    return filtered


def audit_surfaces(root: Path, surfaces: list[Path], *, require_expected_roles: bool = True) -> list[Finding]:
    findings = audit_registry(root)
    findings.extend(audit_root_tool_entrypoints(root))
    role_paths: list[Path] = []
    for path in surfaces:
        if not path.exists():
            findings.append(
                Finding("error", display_path(path, root), 1, "surface-missing", "Configured surface is missing.")
            )
            continue
        if path.name == "SKILL.md" and ".codex" in path.parts:
            findings.extend(audit_skill(path, root))
            surface = display_path(path, root)
            text = path.read_text(encoding="utf-8")
            findings.extend(audit_agent_surface_governance_policy(text, surface))
            findings.extend(audit_session_scratch_policy(text, surface))
            findings.extend(audit_startup_binary_target_policy(text, surface))
            findings.extend(audit_source_block_order_policy(text, surface))
            findings.extend(audit_source_shape_contract_policy(text, surface))
            findings.extend(audit_hard_byte_match_chatgpt_pro_policy(text, surface))
            findings.extend(audit_byte_match_triplet_producer_policy(text, surface))
            findings.extend(audit_source_discovery_chatgpt_pro_policy(text, surface))
            findings.extend(audit_current_policy_updates(text, surface))
        elif path.suffix == ".toml" and path.parent.name == "agents":
            role_paths.append(path)
            findings.extend(audit_role(path, root))
            surface = display_path(path, root)
            text = path.read_text(encoding="utf-8")
            findings.extend(audit_agent_surface_governance_policy(text, surface))
            findings.extend(audit_session_scratch_policy(text, surface))
            findings.extend(audit_source_block_order_policy(text, surface))
            findings.extend(audit_source_shape_contract_policy(text, surface))
            findings.extend(audit_hard_byte_match_chatgpt_pro_policy(text, surface))
            findings.extend(audit_byte_match_triplet_producer_policy(text, surface))
            findings.extend(audit_source_discovery_chatgpt_pro_policy(text, surface))
            findings.extend(audit_current_policy_updates(text, surface))
        else:
            surface = display_path(path, root)
            text = path.read_text(encoding="utf-8")
            findings.extend(audit_text(text, surface))
            findings.extend(audit_git_policy(text, surface))
            findings.extend(audit_orchestration_policy(text, surface))
            findings.extend(audit_quiet_policy(text, surface))
            findings.extend(audit_tool_maintainer_policy(text, surface))
            findings.extend(audit_agent_surface_governance_policy(text, surface))
            findings.extend(audit_session_scratch_policy(text, surface))
            findings.extend(audit_startup_binary_target_policy(text, surface))
            findings.extend(audit_source_block_order_policy(text, surface))
            findings.extend(audit_source_shape_contract_policy(text, surface))
            findings.extend(audit_hard_byte_match_chatgpt_pro_policy(text, surface))
            findings.extend(audit_byte_match_triplet_producer_policy(text, surface))
            findings.extend(audit_source_discovery_chatgpt_pro_policy(text, surface))
            findings.extend(audit_current_policy_updates(text, surface))
            if path.resolve() == (root / "tools" / "README.md").resolve():
                findings.extend(audit_tool_readme_coverage(path, root))
                findings.extend(audit_tool_readme_index(path, root))
                findings.extend(audit_readme_address_led_examples(text, surface))
        findings.extend(audit_global_text_pipeline_policy(text, surface))
    for reference in find_active_durable_devspace_references(root):
        findings.append(
            Finding(
                "error",
                display_path(reference.path, root),
                reference.line,
                "durable-devspace-reference",
                f"Durable surface depends on temporary session artifact `{reference.reference}`.",
                "Promote the material conclusion and transcript path or semantic evidence id, then remove the concrete .devspace path.",
            )
        )
    if require_expected_roles:
        findings.extend(audit_expected_roles(root, role_paths))
        findings.extend(audit_claude_mirror(root))
    return sorted(findings, key=lambda item: (item.severity, item.surface, item.line, item.kind, item.message))


def print_findings(findings: list[Finding], *, summary: bool) -> None:
    errors = sum(1 for item in findings if item.severity == "error")
    warnings = sum(1 for item in findings if item.severity == "warning")
    if summary:
        print(f"agent surface findings: errors={errors} warnings={warnings}")
    if not findings:
        print("Agent surface alignment OK.")
        return
    for item in findings:
        location = f"{item.surface}:{item.line}" if item.line else item.surface
        print(f"{item.severity.upper()}: {location}: {item.kind}: {item.message}")
        if item.suggestion:
            print(f"  suggestion: {item.suggestion}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit agent-facing Recoil tool, doc, skill, and role alignment."
    )
    parser.add_argument("--root", default=str(REPO_ROOT), help="Repository root to scan.")
    parser.add_argument(
        "--surface",
        action="append",
        default=[],
        help="Specific file to scan; may be repeated. Defaults to agent-facing surfaces.",
    )
    parser.add_argument(
        "--active-only",
        action="store_true",
        help="Scan only startup/tool/skill/role surfaces, excluding durable reconstruction docs.",
    )
    parser.add_argument("--summary", action="store_true", help="Print a compact count summary.")
    parser.add_argument("--strict", action="store_true", help="Return nonzero for warnings as well as errors.")
    parser.add_argument("--json", action="store_true", help="Emit findings as JSON.")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    surfaces = [root / item for item in args.surface] if args.surface else default_surfaces(root, active_only=args.active_only)
    findings = audit_surfaces(root, surfaces, require_expected_roles=not args.surface)
    if args.json:
        print(json.dumps([item.to_json() for item in findings], indent=2))
    else:
        print_findings(findings, summary=args.summary)
    has_errors = any(item.severity == "error" for item in findings)
    has_warnings = any(item.severity == "warning" for item in findings)
    return 1 if has_errors or (args.strict and has_warnings) else 0


if __name__ == "__main__":
    raise SystemExit(main())
