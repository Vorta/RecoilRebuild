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
  - none; 0x40c720, 0x40c9c0, 0x40c9e0, 0x40ca20, 0x40ca40, 0x40ca80, 0x40cab0, 0x40cad0, 0x40caf0, 0x40cb10, 0x40cb30, 0x40cb70, 0x40cb90, 0x40cbb0, 0x40cbd0, 0x40cbf0, 0x40cc10, 0x40cc30, 0x40cc60, 0x40cc80, 0x40ccc0, 0x40cd00, 0x40cd30, 0x40ce80, 0x40cf00, and 0x40cf60 are functional-equivalent. Supporting fill-bitmap blockers 0x4b8650 and 0x4b86b0 are functional-equivalent.
- Next action:
  - Condense or close this options-dialog group before starting the next HUD dialog-flow cluster.

### Group: HUD options panel overlay owner

- Anchor: 0x40d070 HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit
- Reason: overlay-owner state init/register/destructor cluster; 0x40d0e0 was reclassified from stale provider-boundary to authored source after current Binary Ninja audit.
- Source blockers:
  - none; 0x40d070, 0x40d080, 0x40d090, 0x40d0a0, 0x40d0b0, 0x40d0c0, 0x40d0e0, and 0x40d150 are functional-equivalent.
- Next action:
  - Continue with the next overlay-owner lifecycle function in M03.

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
