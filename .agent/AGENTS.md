# AGENTS.md

> Compatibility pointer for tools that search this folder first.

- The authoritative agent instructions are in `../AGENTS.md`.
- `.agent/RECONSTRUCTION_PROGRESS.sqlite3` is the only reconstruction-progress
  tracker; do not hand-edit it or infer state from raw SQLite reads.
- `.agent/WORKSPACE_ISSUES.sqlite3` is the independent workspace-process issue
  authority; access both ledgers only through `python tools/recoil.py`.
- With no explicit target, run `python tools/recoil.py progress next` first; no
  owner, work-item, section-tag, final, or companion-binary view may outrank it.
- Inspect an assigned address, owner, physical block, semantic span, or work item
  with `python tools/recoil.py progress show <selector>` or locate one with
  `python tools/recoil.py progress find <query>`.
- For documentation, tooling, skill, role, or instruction work, follow root
  `AGENTS.md`'s targeted non-address workflow; do not select an address or
  require Binary Ninja unless the task itself depends on current binary facts.
- Root `AGENTS.md` owns the governed Git/worktree policy; branch, worktree,
  integration, retirement, and hygiene operations are orchestrator-only.
- Do not maintain a second full instruction copy here. Update root `AGENTS.md`.
- This pointer file defines no independent Git or worktree procedure.
