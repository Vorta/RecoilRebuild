# messages.dll Implementation Groups

Use this tracked file for temporary dependency-group notes during companion
`messages.dll` reconstruction. `.agent/SOURCE_OWNERS.json` is the durable
owner-scope ledger; this file lists only active multi-function,
source-readiness, owner, or data groups currently being coordinated for
`messages.dll`.

## Rules

- Use `python tools/recoil.py owner <subcommand> --binary messages ...`,
  `python tools/recoil.py status --binary messages ...`, and
  `python tools/recoil.py frontier --binary messages ...` for companion work.
- Create or update a group before editing when a task touches more than one
  function, the generated lookup table, the message-table resource, or a shared
  data owner.
- Do not mark owner entries done from this file alone. Owner gates and tiers
  still require current source/build/Binary Ninja evidence.
- Keep notes concise and temporary. Move durable facts into source comments,
  Binary Ninja comments, tests, or `docs/reconstruction/messages_dll.md` before
  pruning.
- Use `python tools/recoil.py audit groups --binary messages --summary
  --wip-limit 4` to check for stale, completed, or overgrown groups.
- Use `python tools/recoil.py audit handoff --path
  .agent/IMPLEMENTATION_GROUPS_MESSAGES.md --strict` before launching workers
  from live handoff blocks.

## Active Group Template

```text
### Group: short descriptive name

- Anchor: 0xNNNNNN Name
- Owner id:
- Section:
- Queue: ready owner/data work / blocked pending evidence or policy / shared blocker / deferred verify-only debt
- Reason: dependency closure / source file cluster / generated data owner / resource owner
- Source blockers:
  - 0xNNNNNN Name
- Next action:
  - python tools/recoil.py status --binary messages 0xNNNNNN --lane binary
```

## Active Groups

No active companion implementation groups.
