# Recoil Tooling

`python tools/recoil.py` is the sole public tool entry point. Use
`python tools/recoil.py help`, `help <group>`, or `commands --json` for the
current command surface; backend modules are internal.

The serial scheduler is:

```powershell
python tools/recoil.py progress next --json
```

It returns one `recoil-current-task-v2` task with one stage, advisory scope,
optional diagnostic check, optional serial stage runner, direct acceptance
command when ready, blocker, and the transaction/semantic/evidence-generation
revisions. The machine-local SQLite tracker is the only current-state authority.
There is no README cache, work allocator, role registry, reservation, handoff,
or secondary progress record.

Core maintenance validation is one sequential, fail-fast infrastructure
aggregate after focused proof-kernel tests:

```powershell
python tools/recoil.py doctor
```

`doctor` validates infrastructure only; it does not compile or accept
reconstruction source. Use the canonical workflow in
`AGENTS.md`, the operational runbook at
`docs/reconstruction/retail_executable_reproduction.md`, and the focused
`.codex/skills/recoil-*` skill owning the task. Never hand-edit the SQLite
authorities.
