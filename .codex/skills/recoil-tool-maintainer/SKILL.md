---
name: recoil-tool-maintainer
description: Directly maintain RecoilRebuild tools, command contracts, validation paths, local skills, mirrors, and workspace issues without changing reconstruction source or evidence state.
---

# Recoil Tool Maintainer

Perform maintenance directly in the canonical checkout. There are no maintenance roles, packets, branches, worktrees, claims, or handoffs.

## Scope

Allowed surfaces are `tools/recoil.py`, `tools/_recoil`, `tests/tools`, focused tool/process docs, `.codex/skills/recoil-*`, their thin `.claude` mirrors, `CLAUDE.md`, and related settings. Do not change production `src/`, Binary Ninja state, provider/owner/tier criteria, or tracker semantic facts unless the user separately assigns that work.

Use `.agent/WORKSPACE_ISSUES.sqlite3` only through `python tools/recoil.py issue ...`. An issue represents a reproducible tool, environment, validation, or rule defect—not ordinary reconstruction backlog.

Before a ledger schema or migration change, create verified SQLite backup-API copies outside the live `.agent` paths and define the rollback boundary. Never hand-edit a ledger. Use revision-guarded governed mutations.

Keep `.codex` canonical. Each Claude skill file is a thin pointer containing routing metadata, the verbatim canonical description, and the canonical path; it must not duplicate procedure text. Do not add agent role files.

## Validation

Run focused tests first, then the strict infrastructure gates:

```powershell
python tools/recoil.py doctor --infrastructure-only
python tools/recoil.py audit agent-surface --strict
python tools/recoil.py audit pipeline-contracts --strict
python tools/recoil.py audit pipeline-reachability --strict
python -m unittest discover -s tests/tools -p "*_tests.py"
```

Report commands run, results, remaining failures, ledger mutations, and any material deletion or rollback limitation.
