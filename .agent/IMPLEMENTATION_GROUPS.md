# Implementation Groups

Use this file for temporary dependency-group notes during reconstruction. The
plan remains address-based; this file lists only active multi-function,
source-readiness, or coherent binary-safe groups currently being coordinated.

## Rules

- Create or update a group before editing when a task touches more than one
  function or a shared type/global/vtable.
- Keep groups scoped. Prefer one class, one source file cluster, one recursive
  cycle, or one call-chain frontier.
- Do not mark plan entries done from this file alone. Plan markers still require
  current source/build/Binary Ninja evidence.
- Keep notes concise and temporary. Move durable facts into source comments,
  Binary Ninja comments, tests, `docs/reconstruction/`, or narrow subsystem docs before
  pruning.
- Verification-only queues that no longer carry source blockers should not live
  in this active working file unless they are coordinating a current coherent
  binary-safe pass. Use `.agent/RECOIL_PLAN.md`, `python tools/recoil_status.py
  0xNNNNNN`, VC verification manifests, and
  `python tools/recoil_verification_backlog.py` for current verification state.
- Recompute verification scope with `python tools/recoil_status.py 0xNNNNNN` or
  `python tools/recoil_frontier.py 0xNNNNNN --depth 1` after source blockers
  clear.
- Use `python tools/recoil_groups_audit.py --summary --wip-limit 4` to check
  for stale, completed, or overgrown groups.

## Active Group Template

```text
### Group: short descriptive name

- Anchor: 0xNNNNNN Name
- Reason: dependency closure / class cluster / recursive cycle / shared ABI layout / source file cluster
- Source blockers:
  - 0xNNNNNN Name
- Next action:
  - python tools/recoil_status.py 0xNNNNNN
```

## Active Groups
