---
name: recoil-tool-maintainer
description: Directly maintain RecoilRebuild tools, command contracts, validation paths, local Codex skills, and workspace issues without changing reconstruction source or evidence state.
---

# Recoil Tool Maintainer

Perform maintenance directly in the canonical checkout. There are no maintenance roles, packets, branches, worktrees, claims, or handoffs.

## Scope

Allowed surfaces are `tools/recoil.py`, `tools/_recoil`, `tests/tools`, focused tool/process docs, and `.codex/skills/recoil-*`. Do not change production `src/`, Binary Ninja state, provider/owner/tier criteria, or tracker semantic facts unless the user separately assigns that work.

Use `.agent/WORKSPACE_ISSUES.sqlite3` only through `python tools/recoil.py issue ...`. An issue represents a reproducible tool, environment, validation, or rule defect—not ordinary reconstruction backlog.

Before a ledger schema or migration change, create verified SQLite backup-API copies outside the live `.agent` paths and define the rollback boundary. Never hand-edit a ledger. Use revision-guarded governed mutations.

Keep repository-local skills under `.codex/skills/recoil-*`. Do not add agent role files or duplicate skill mirrors.

## Validation

Run focused tests first. Use `recoil-validation` for the canonical infrastructure
matrix and add the full tool unit suite only when the change spans shared command
or ledger infrastructure.

Report commands run, results, remaining failures, ledger mutations, and any material deletion or rollback limitation.
