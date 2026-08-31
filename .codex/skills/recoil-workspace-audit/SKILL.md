---
name: recoil-workspace-audit
description: Audit RecoilRebuild instructions, Codex skills, command reachability, ledger health, and non-address process clarity for the serial single-agent workflow.
---

# Recoil Workspace Audit

This is a read-only audit unless the user also asks for fixes. Review the canonical root instructions, serial reconstruction runbook, concise tool manual, active local Codex skills, command registry, tests, and ledger metadata.

The required architecture is one current agent, one canonical checkout, one current reconstruction task, and direct validation/acceptance commands. There must be no agent-role inventory, subagent routing, orchestrator/worker split, work packet, claim, lease, reservation, linked-worktree lifecycle, scheduler-revision domain, generated-current cache, or multiple compatible work lanes.

Check that:

- `progress next --json` emits exactly one `recoil-current-task-v2` task and a three-field revision vector;
- public commands contain no retired allocation, handoff, worktree, or current-metadata routes;
- progress schema 6 has no work-item collection and issue schema 2 has no packet/reservation tables;
- `.codex/skills/recoil-*` is the only repository-local skill surface;
- no role files exist;
- documentation names only executable current commands;
- tool and ledger failures are reported rather than hidden as reconstruction workarounds.

Use `recoil-validation` for the canonical infrastructure matrix rather than
duplicating it here. Run focused inspection first so a failure can be tied to an
exact surface.

Report findings by severity with exact files or commands and distinguish a static reference failure from an unreachable live transition.
