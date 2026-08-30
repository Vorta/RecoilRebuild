# CLAUDE.md

`AGENTS.md` is the authoritative workspace contract. Read it before acting.

The workspace uses one serial direct-work scheduler:

```powershell
python tools/recoil.py progress next --json
```

Perform the returned task directly in the canonical checkout. The Recoil
workspace has no local role registry, worker handoff, packet allocator,
reservation, lease, linked-worktree, or multi-lane process.

Canonical procedures live in `.codex/skills/recoil-*/SKILL.md`. Claude skill
files under `.claude/skills/recoil-*/SKILL.md` are thin pointers to those
canonical procedures and add no policy.

ChatGPT Pro remains an allowed advisory line under the ambiguity triggers in
`AGENTS.md`; invoke it directly when a trigger applies.

Use current source, the live SQLite tracker, immutable retail, fresh governed
VC5 output, and the already-open Binary Ninja databases according to their
respective procedures. Preserve unrelated user changes and never hand-edit the
SQLite authorities.
