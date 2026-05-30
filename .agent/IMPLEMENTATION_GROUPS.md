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

### Group: HUD options panel constructor

- Anchor: 0x40c720 HudOptionsDialog::Constructor
- Reason: options-dialog class/table cluster; constructor installs authored option-widget ftables and owns the dialog layout binding sequence.
- Source blockers:
  - none; 0x40c720, 0x40c9c0, and 0x40c9e0 are functional-equivalent.
- Next action:
  - Verify the small HudUiOptionsPanel option-sync methods already modeled in `src/GameZRecoil/zHud/zhud_ui.cpp`, then recompute 0x40cf60 destructor/source-file follow-up.

### Group: Hud command dialog constructor

- Anchor: 0x40a5b0 HudCmdDialog::Constructor
- Reason: class constructor/source-file cluster used by HudCmdDialogState activation/deactivation/queue entry; source work is functional-equivalent through 0x40bda0, binary-safe verification remains a later cluster pass.
- Source blockers:
  - none; 0x40a5b0, 0x40bcf0, 0x40bd60, and 0x40bda0 are functional-equivalent.
- Next action:
  - Recompute the next HUD command-dialog caller with `python tools/recoil_plan_cli.py next` or queue a coherent binary-safe pass for the HudCmdDialog source-file/table cluster.

### Group: Hud command bind button destructor

- Anchor: 0x40c280 HudCmdBindButtonBase::DestructorCore
- Reason: class destructor/source-file cluster shared by command/key/joystick/mouse bind buttons.
- Source blockers:
  - none; 0x40c280 is functional-equivalent.
- Next action:
  - Recompute the next HUD command-bind-button destructor dependency with `python tools/recoil_plan_cli.py next` or fold derived destructors into this recovered base owner when Binary Ninja evidence permits.
