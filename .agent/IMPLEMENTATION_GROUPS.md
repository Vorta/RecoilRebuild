# Implementation Groups

Use this tracked file for temporary dependency-group notes during
reconstruction. The plan remains address-based; this file lists only active
multi-function, source-readiness, owner, or data groups currently being
coordinated. Pure tier `S` verification groups are active only after
`tier_s_priority_ready=true` or explicit user direction. Active groups are the
default no-address startup queue: new agents should resume actionable WIP here
before selecting new work with
`python tools/recoil.py plan next --lane binary`. Keep the header and template
available even when no groups are active.

## Rules

- Create or update a group before editing when a task touches more than one
  function or a shared type/global/vtable.
- When launching without a user-specified address or source group, inspect
  active groups first and resume the first actionable one. Start unrelated new
  work only when active groups are absent, stale, contradicted, completed, or
  explicitly deprioritized by the user.
- Keep groups scoped. Prefer one class, one source file cluster, one recursive
  cycle, or one call-chain frontier.
- Do not mark plan entries done from this file alone. Plan markers still
  require current source/build/Binary Ninja evidence.
- Keep notes concise and temporary. Move durable facts into source comments,
  Binary Ninja comments, tests, `docs/reconstruction/`, or narrow subsystem
  docs before pruning.
- Stage this file only when an active group update belongs with a qualifying
  source checkpoint under the root `AGENTS.md` git rules. Do not commit stale
  or group-only bookkeeping.
- Verification-only queues that no longer carry source, owner, or data blockers
  should not live in this active working file while global owner/data blockers
  remain. Use `.agent/RECOIL_PLAN.md`, `python tools/recoil.py status
  0xNNNNNN`, VC verification manifests, and `python tools/recoil.py audit
  backlog --lane binary --include-deferred-verify` for deferred verification
  state.
- Normal binary-lane planning prioritizes owner structure blockers before
  isolated implementation/behavior work and prioritizes global owner/data
  blockers before verify-only tier `S` work. Active verify-only groups should
  condense or move out of this file while any authored `Source owner ❌` or
  `Data reimplemented ❌` marker remains.
- Recompute verification scope with `python tools/recoil.py status 0xNNNNNN`
  or `python tools/recoil.py frontier 0xNNNNNN --depth 1` after source blockers
  clear.
- Use `python tools/recoil.py audit groups --summary --wip-limit 4` to check
  for stale, completed, or overgrown groups.

## Active Group Template

```text
### Group: short descriptive name

- Anchor: 0xNNNNNN Name
- Reason: dependency closure / class cluster / recursive cycle / shared ABI layout / source file cluster
- Source blockers:
  - 0xNNNNNN Name
- Next action:
  - python tools/recoil.py status 0xNNNNNN
```

## Active Groups

### Group: zVideo renderer dispatch/global owner audit

- Anchor: 0x4a77a0 zVideo::BindRendererDispatch
- Reason: renderer dispatch globals and DirectDraw hardware-device data shared
  by memory-query, surface, palette, mode-setting, restore, and teardown
  callers.
- Current blockers:
  - Work remains owner/data-led while `tier_s_priority_ready=false`. Do not use
    this group as a verify-only queue unless the user explicitly directs tier S
    work.
  - Source-owner blockers reported by `audit groups --summary`: 0x48ff80,
    0x49b7e0, 0x49e6c0, 0x49edc0, 0x49bbf0, 0x4997d0, 0x49f180,
    0x48d450, plus related zVideo/zRndr renderer-dispatch owners in the same
    source cluster.
  - Data blockers reported by `audit groups --summary`: 0x42e330, 0x48ff70,
    and 0x48d340. The 0x42e330 caller path currently routes through 0x48ff70,
    and 0x48ff70 remains data-blocked by downstream zRndr
    SelectSpanRoutines callback/global ownership.
  - Same-session BN/source-worker packets for the 0x49b7e0-led switch-vshift
    span family confirm the retail source shape intentionally pivots ESP
    through gRndr_SavedEspSlot and writes destination words with push/sub-esp.
    No safe VC5-era production C++ model was found under the current no raw
    assembly/scaffold rules, so this owner/data gate remains blocked until a
    policy-approved source model is identified or the raw-assembly prohibition
    is explicitly changed.
  - Accepted recent work is durable elsewhere: zRndr queue/lens/fog/palette
    slices, cached-client-rect mask helpers, DirectDraw present/clear/data
    passes, and circle helpers have source/plan/verification evidence. Keep
    this file focused on the remaining owner/data routing.
  - Deferred verify-only addresses include current tier B/S-ready zRndr/zVideo
    byte-comparison debt such as 0x499a20, 0x499c40, 0x49a2b0, 0x49aa90,
    0x49b020, 0x49b780, 0x46e720, and 0x4a8790; revisit them only after the
    global owner/data gate opens or explicit user direction.
- Next action:
  - Do not reassign the 0x49b7e0-led ESP-pivot span-family slice without new
    BN/source-model evidence or explicit user approval for a lower-level
    implementation strategy. Prefer another active owner/data WIP while this
    group remains blocked by the current source rules.
  - Re-run `python tools/recoil.py audit groups --summary --wip-limit 4` after
    each owner/data update and prune this group again when it becomes
    verify-only.

### Group: zVideo adjust-surfaces dispatch cleanup

- Anchor: 0x4a6900 zVideo::PresentOrAdjustSurfacesIfEnabled
- Reason: source-file owner/data readiness for renderer-present dispatch used
  by display initialization and per-frame callers.
- Current blockers:
  - The obsolete production adapter was removed and RecoilApp display
    initialization now calls the direct renderer adjust helper.
  - Renderer dispatch globals are accepted through the typed renderer dispatch
    owner, and VC5SP3 verification for 0x4a6900 has zero unmasked byte
    mismatches.
  - Remaining group value is caller/data propagation, especially around
    0x42e330.
- Next action:
  - Recheck `python tools/recoil.py status 0x42e330 --lane binary` before any
    further source or marker work. Same-session refresh found the
    `recoil_app_initialize_display` functional manifest's smoke was implemented
    but not registered in the native smoke table; the registry was repaired and
    `python tools/recoil.py verify functional 0x42e330` now passes again. The
    binary frontier still routes the data gate through 0x48ff70, and
    `python tools/recoil.py verify functional 0x48ff70` passes; 0x48ff70 remains
    data-blocked only by the downstream zRndr SelectSpanRoutines
    callback/global owner.

### Group: HUD app-state class cleanup

- Anchor: 0x406ed0 RecoilStateCheatCode::RecoilStateCheatCode,
  0x408d60 RecoilStateControls::RecoilStateControls, 0x415850
  RecoilStateConfirmQuit::RecoilStateConfirmQuit, 0x41c560
  HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent, and 0x40d150
  HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent
- Reason: HUD app-state, overlay, and dialog class source-shape cleanup.
- Current blockers:
  - Selected app-state and overlay owners no longer rely on local
    RecoilApp_IState vtable globals, local virtual dispatch views, or manual
    scalar-deleting-destructor source.
  - Broader HUD widget/callback table debt remains in HUD source files, so
    affected constructor/destructor/dialog owners stay below accepted source
    owner/data/tier S until the wider class-family owner is recovered.
  - Active affected families include cheat-code, controls, confirm-quit,
    new-game/options overlay, HudCmd dialog state, net-game setup overlay,
    credits panel, command dialog, options dialog, and clamped-int step-button
    owners.
  - Current data blocker reported by `audit groups --summary`: 0x40a5b0.
    HudCmdDialog/HudCmd* table emission remains the known source-shape/data
    blocker for dependent HudCmd dialog state entries. The latest pass replaced
    placement-new member construction with explicit concrete/base constructor
    calls and improved VC5 drift from 636 to 633 mismatches, but Data/B/S stay
    blocked because VC5 still emits out-of-line concrete constructor calls
    instead of BN's inline base-constructor plus generated table-store sequence.
  - The HudCmdBindButtonBase class pass is no longer active owner/data WIP:
    0x40c280, 0x4b90e0, and 0x4b8de0 are accepted at tier B with
    HudCmdBindButtonBase class ownership and generated/table string data
    classified; remaining work there is verify-only and deferred by the global
    owner/data gate.
  - Deferred verify-only addresses reported by `audit groups --summary`
    include 0x40cf60, 0x443310, 0x4b8b60, 0x41c400, 0x406d20, 0x4b5900,
    0x4138d0, and 0x4a6e80. Do not route those ahead of 0x40a5b0 or other
    current owner/data blockers while `tier_s_priority_ready=false`.
  - Historical smoke-registration, VC5 probe, and queue/helper details are
    captured in functional manifests, VC5 manifests, source docblocks, and plan
    markers; this temporary group should stay focused on current routing.
- Next action:
  - Do not reselect 0x40a5b0 for another isolated constructor-shape probe
    without new BN/source evidence for the larger real C++ constructor body.
    Recheck `python tools/recoil.py status 0x40a5b0 --lane binary` and
    `python tools/recoil.py frontier 0x40a5b0 --depth 1 --lane binary` before
    any future HudCmdDialog work.
  - Use focused status/frontier checks for the current owner/data anchor and
    source-shape guards over touched HUD files before editing.
  - Defer the Time::Tick and zNetwork::ShutdownSessionRuntime tier S blockers
    discovered through the MpExit frontier until `tier_s_priority_ready=true`
    or until the user explicitly directs tier S work.
