---
name: recoil-tool-maintainer
description: Maintain RecoilRebuild local tools and workspace process surfaces. Use when you need to fix a reported `.agent/WORKSPACE_ISSUES.sqlite3` issue, upgrade `tools/recoil.py` or tools under `tools/_recoil`, repair command docs, validation paths, local Recoil skills, or local subagent roles, and should delegate the implementation to `recoil_tool_maintainer`.
---

# Recoil Tool Maintainer

This is a governed agent-surface evolution workflow and a parent-assigned
`task_class: non-address-maintenance` task. Root
`AGENTS.md` remains authoritative. Tool/surface work must preserve the global
`progress next` sequence and may not create owner, block, work-item, final, or
`messages.dll` peer schedulers.

Use this skill for one reproducible workspace/tool/docs/skill/role defect or
requested upgrade, not normal reconstruction debt. The parent stays scheduler,
integrator, validator, workspace-issue authority, and final claimant.

## Required Handoff

Require one issue id or requested upgrade, actual and expected behavior,
allowed and forbidden paths, exact validation commands, and return fields.
Allowed surface evolution is limited to explicitly assigned paths under
`.codex/skills/recoil-*`, `.codex/agents/*.toml`, `.claude/skills/recoil-*`,
`.claude/agents/*.md`, `.claude/settings.json`, `CLAUDE.md`,
`tools/recoil.py`, `tools/_recoil`, `tests/tools`, and focused docs. The
`.claude` surface and `CLAUDE.md` are harness pointers only; every canonical
procedure and role contract stays under `.codex`. Never edit production source,
support inputs, Binary Ninja state, or parent-owned tracker/process state unless a
separate governed workflow explicitly assigns that exact mutation.

The progress authority is `.agent/RECONSTRUCTION_PROGRESS.sqlite3`; the
workspace-issue authority is `.agent/WORKSPACE_ISSUES.sqlite3`. They are
independent databases with independent monotonic revisions. After the paired
cutover, tooling uses and restores SQLite only; do not create a JSON runtime
backend, mirror, or export.

Workspace-issue governance requires a clean reviewed Git branch, stores its
opaque baseline commit plus exact writable closure, and enforces closeout with
porcelain-v2 and commit-relative name-status/diff. It rejects every changed
path or rename endpoint outside that closure and never interprets Git state as
binary equivalence. Routine database
no-mutation evidence is revision, schema/user version, relevant row counts, and
`PRAGMA integrity_check`, with CAS guards for every mutation.

Orchestrated tracked-write issue and progress packets use one `packet/` branch, one linked
worktree, one external physically authenticated build root, and one central
reservation. The orchestrator owns create/integrate/retire/hygiene. A handed-off
worker may stage only the exact writable closure and create one nonaccepting
packet-id commit; it owns no branch or worktree lifecycle.

Call-contract verifier currency uses only the reviewed integer coordinates
`CALL_CONTRACT_VERIFIER_GENERATION = 12`,
`NORMALIZER_REGISTRY_GENERATION = 12`, and
`EXPECTED_FACT_SCHEMA_VERSION = 12`. Changes inside the registered component
closures require the corresponding increment and conservative invalidation;
do not introduce a content-derived identity graph.

For linked validation, tracked source, tools, tests, policies, target manifests,
and tracked `.agent` reference manifests come from the executing worktree.
Machine-local retail and live SQLite authorities come from the validated
canonical control root and are never copied or linked into the linked checkout.
All fallible compiler, test, audit, and doctor checks complete in the temporary
integration worktree before `master` advances; only deterministic Git,
topology, tag, and physical-identity assertions follow the fast-forward.

Apply the canonical policy owners rather than copying them:

- scheduling/mission/session debt: root `AGENTS.md` and
  `retail_executable_reproduction.md`;
- source discovery/source shape: `recoil-source-model-recovery`;
- hard-byte/raw-assembly acceptance: `recoil-tier-verification`;
- triplet production and local checks:
  `recoil_source_worker`;
- direct attachment role/scope checks, upload, and session-global single-flight
  Pro call: parent;
- triplet and parent-result validation: `recoil_verifier`;
- retail assembly packet: `recoil_bn_fact_mapper`;
- docblock/allowlist compliance: `recoil_scaffold_auditor`.

Do not introduce new gate/tier criteria, evidence shortcuts, fake provider
models, broad reconstruction roles, overlapping role authority, or Binary Ninja
mutation outside `recoil-binary-ninja-reconstruction`/
`recoil_bn_reconstructor`.

Keep active command guidance on registered routes: complete owner structure
replacement uses `owner replace-batch`, conservative invalidation uses `owner
downgrade`, and verification registration uses `verification-target sync`.
Unsupported positive owner metadata/gate/tier mutations are workspace issues;
do not restore add/link/set/batch or generic owner operations.

## Validation

Choose touched-surface checks, typically:

```powershell
python -m unittest tests.tools.recoil_cli_tests
python -m unittest tests.tools.recoil_agent_surface_audit_tests
python tools/recoil.py audit agent-surface --strict
python tools/recoil.py doctor --infrastructure-only
$env:PYTHONUTF8='1'
python .codex\skills\.system\skill-creator\scripts\quick_validate.py .codex\skills\<skill-name>
python -c "import pathlib,tomllib; [tomllib.loads(p.read_text(encoding='utf-8')) for p in pathlib.Path('.codex/agents').glob('*.toml')]; print('role toml parse OK')"
```

Subagents never control branches, worktrees, integration, or cleanup. A packet
worker may use Git only for its exact closure staging and one packet-id commit.
Never clear or durably depend on `.devspace`; return material
semantic conclusions and direct evidence/transcript paths with their role and
scope. Do not mutate issue state or claim owner/source/tier acceptance.

Return changed paths, exact command results, validation gaps, blockers,
out-of-scope files left untouched, and issue-resolution candidate text.
