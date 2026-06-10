# Implementation Groups

Use this tracked file for temporary dependency-group notes during
reconstruction. The plan remains address-based; this file lists only active
multi-function, source-readiness, or coherent tier `S` groups currently being
coordinated. Active groups are the default no-address startup queue: new agents
should resume actionable WIP here before selecting new work with
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
- Verification-only queues that no longer carry source blockers should not
  live in this active working file unless they are coordinating a current
  coherent tier `S` pass. Use `.agent/RECOIL_PLAN.md`,
  `python tools/recoil.py status 0xNNNNNN`, VC verification manifests, and
  `python tools/recoil.py audit backlog` for current verification state.
- Normal binary-lane planning prioritizes owner structure blockers before
  isolated implementation/behavior work and prioritizes owner/data blockers
  before verify-only tier `S` work. Active verify-only groups should condense
  or move out of this file when any nearby class/source-file/global owner debt
  remains.
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
  - The renderer dispatch/global owner is accepted for 0x4a77a0, and many
    DirectDraw/video helpers are tier B or tier S-ready for callers. The
    fullscreen surface-builder data pass is accepted; image-surface and later
    teardown/restore/mode-setting data clusters remain pending.
  - Fullscreen surface-builder source ownership is accepted for 0x4a8f80,
    0x4a88f0, 0x4a8920, 0x4a8b20, and 0x4a8dc0. The shared surface-state BSS
    records g_zVideo_SwSurfaceState (632200h), g_zVideo_PrimarySurfaceState
    (632220h), and g_zVideo_DisplayModeSurfaceState (632240h) are now accepted for 0x4a8f80,
    and 0x4a8920 has been corrected to match BN by leaving its software-surface
    caps independent of g_zVideo_RendererType and by reading
    g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags without a null
    fallback. 0x4a88f0, 0x4a8920, 0x4a8b20, and 0x4a8dc0 are tier B after
    accepting the typed surface-state, selected-device, DirectDraw/clipper, and
    renderer-flag globals; tier S remains deferred to a coherent DirectDraw
    display/mode-management source-cluster pass.
  - Image-surface helper source ownership and data gates are accepted for
    0x4a83d0, 0x4a84c0, 0x4a8500, 0x4a8650, 0x4a8680, and 0x4a86f0. These are
    tier B after accepting the zVidImagePartial surface/pixel/pitch ownership
    model, DirectDraw provider interfaces, selected-device caps, and ReportError
    source strings; tier S remains deferred to a coherent DirectDraw
    image/surface source-cluster pass.
  - The hardware-device table/selection and renderer-flag cluster is no longer
    a data-owner blocker for 0x4a6b40, 0x4a7990, 0x4a7490, 0x4a8870, 0x4a7b40,
    0x4a93d0, 0x4a95e0, and 0x4a96b0. 0x4a7b40 is tier S after the VC5SP3
    zero-mismatch check; 0x4a93d0, 0x4a95e0, and 0x4a96b0 remain tier B with
    tier S deferred to a coherent zvid_dd.c DirectDraw/Direct3D enumeration
    cluster pass. DirectDraw/Win32 pointees remain provider-owned, but the
    zVideo COM handle globals, selected-device table/pointers, option strings,
    source-file string, primary-backbuffer flag, and surface-state records are
    authored data for remaining surface/fullscreen callers.
  - D3D-state consumers 0x4a9c20, 0x4aa9e0, and 0x4accc0 are now tier B after
    accepting the typed DirectDraw/Direct3D provider pointers, z-buffer attach
    state, selected-device pointer, viewport/material handles, caps records,
    render-state/fog caches, wireframe state, and quad-batch BSS data.
    0x4ad680 is tier B/no-globals. Tier S remains deferred to a coherent
    zvid_ddd3d.c device-state/render-state source-cluster pass.
  - D3D texture-record source ownership is accepted for 0x4aa0f0, 0x4aa8b0,
    0x4aa8f0, 0x4aa900, 0x4aa920, 0x4aa980, and 0x4aa9d0. The
    texture-record upload/pixel-pack data pass accepted g_zVideo_DisplayModeBpp,
    g_zVideo_PixelPack, and g_zVideo_TexturePixelPack BSS ownership; 0x4a66e0,
    0x4a6db0, 0x4aa0f0, 0x4aa600, 0x4aa6f0, and 0x4aa920 are now tier B.
    Remaining texture-record work is coherent tier S verification, not a data
    blocker.
  - The zVideo init/shutdown source-file cluster was active because
    RecoilApp::InitializeDisplay (0x42e330) depends on 0x4a75f0 and the
    frontier ranked that owner before caller data propagation. Scope:
    0x4a7520, 0x4a7530, 0x4a75e0, 0x4a75f0, 0x4a7700, 0x4a7740, and 0x4a7af0.
    Current evidence accepted 0x4a7520, 0x4a75e0, 0x4a7700, 0x4a7740, and
    0x4a7af0 to tier B after registering the stale native smokes and documenting
    the recovered zImage default-texture helper. 0x4a75f0 is tier B after
    accepting the hardware default texture and quad-batch state, and 0x4a7530
    is tier B after matching BN's module-global zero from g_zVideo_RendererType
    through g_zVideo_OverwriteQueueBase[0x180].
  - RecoilApp::InitializeDisplay's display-option getter blockers
    0x408650, 0x408690, 0x4086a0, 0x4086c0, and 0x4086d0 are now tier S after
    accepting the typed zOpt display/window/stride option-pointer BSS globals
    and passing zopt_section_accessor_smoke plus VC5SP3 zero-mismatch checks.
    0x4903e0 zRndr::SetVideoStrideMirrors is now tier S after accepting the
    typed gRndr_VideoStrideMirror0/1 BSS globals and restoring the VC5SP3
    provider include order for zRndr.cpp. Refreshing 0x42e330 now routes its
    lowest visible data blocker to 0x48ff70 zVid::InitFrameScratchBuffers,
    followed by the zVideo clear dispatch helpers; 0x490520 remains tier B
    verify debt rather than the current owner/data blocker.
  - Tier S remains pending for the broader teardown, restore, palette,
    mode-setting, and surface-helper source clusters after the owner/data pass.
- Next action:
  - For RecoilApp caller propagation, the refreshed 0x48ff70 frontier now
    routes through 0x48ff80 zRndr::SelectSpanRoutines into the zRndr
    span-selection/global table owner. 0x49e140 is tier B, and the
    switch-vshift helper/docblock provenance has been refreshed for the
    selector-installed span family. The shared gRndr_SavedEspSlot BSS pointer
    is now modeled as zRndr::g_spanSavedEspSlot with the BN
    zRndr_SpanEspPivotSave stack-pivot save record, and the span callback
    dispatch bank now includes
    gRndr_pfnSelectedSpanOp_Mode1 in BN BSS order with flat/poly selected-pair
    stores restored. The active span texture globals are partially modeled:
    BN now distinguishes the initialized queued-alpha slot
    gRndr_QueuedTexAlphaMap from the BSS active-alpha slot
    gRndr_ActiveTexAlphaMap, and the draw paths now preserve
    gRndr_ActiveTexShift, U mask, and V mask in BN order with refreshed smokes
    for the queued-alpha and fan-triangle paths. The selector-installed
    switch-vshift span family has now been refreshed against BN
    zRndr_Span.cpp evidence for the ESP-pivot source shape; 0x49b7e0
    zRndr::SpanMasked16FromTex16SwitchVShift is part of this same
    selector-installed owner pass even though older group membership output did
    not include it. The active
    fixed-20 U/V step globals and current span-base pointer are renamed in
    source/tests to the BN active-texture model. The active texture pixel slot
    is now named and documented as the BN gRndr_ActiveTexPixels mixed pal8/tex16
    buffer, and 0x49e6c0 has been expanded from a compact helper loop into the
    BN-observed texVShift 10..17 switch-shaped reverse span source. VC5 still
    fails for 0x49e6c0 because the retail code uses the gRndr_SavedEspSlot
    ESP-pivot cursor while the source uses ordinary C++ pointer stores.
    The zVideo noise/scratch data subset for 0x48d340/0x48ff70 has been
    corrected to BN's BSS order from g_zVid_NoiseByteTableSize through
    g_zVideo_FxSurfacePitchPixels16, with 0x48ff70/0x42e330 still data-gated by
    the zRndr selector callback/global owner and clear-dispatch data gates.
    The MMX scratch globals used by 0x49ea40 now use the BN zMmxQword lo/hi
    record name in source, and the initialized gRndr_QueuedTexAlphaMap startup
    sentinel is documented by symbol name rather than a raw image address.
    Continue the shared
    zRndr_Span.cpp callback/global data owner audit for remaining zRndr
    span-family owner acceptance, including the MMX qword global shape, before
    returning to 0x48ff70 or 0x42e330.
    Keep the remaining zVideo work scoped to coherent tier S passes unless a
    fresh frontier exposes another owner/data blocker.

### Group: zVideo blur-region source-shape cleanup

- Anchor: 0x48ea00 zVideo::buff_BlurRegionByMode
- Reason: source-file-local blur dispatcher and leaves over the typed FX surface
  globals.
- Current blockers:
  - Scope is 0x48e380, 0x48e670, 0x48e870, and 0x48ea00.
  - BN shows 0x48ea00 is a tail-call router over mode 1 horizontal, mode 2
    vertical, and combined default paths; the leaves depend on already-modeled
    FX-surface and pixel-pack data.
  - Current group work is mostly docblock, functional rerun, owner/data review,
    and byte verification cleanup.
- Next action:
  - Replace legacy comments with required docblocks in the touched blur cluster,
    rerun functional targets and source guards, then review owner/data marker
    promotion before resuming dependent callers.

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
    further source or marker work.

### Group: zSound tick/fade/playhandle owner cleanup

- Anchor: 0x49f620 zSnd::Tick
- Reason: zSound source-file/subsystem owner and shared authored global data.
- Current blockers:
  - 0x49f620, 0x4a3c20, 0x4a3ad0, and 0x49fda0 are tier B from current
    source/data evidence and focused functional targets.
  - Remaining blockers are tier S byte drift in backend dispatch, fade-list
    loop/compaction, fade clamp/dispatch, and marker-timeline scheduling.
- Next action:
  - Treat the chain as B-ready for callers; pursue the coherent zSound tier S
    pass only after higher-priority binary-lane owner/data blockers are
    exhausted.

### Group: HUD UI save/load class recovery

- Anchor: 0x434680 HudUiSaveGameDialog::InitLayout
- Reason: HUD UI class hierarchy and source-shape recovery for save/load and
  related dialog/widget owners.
- Current blockers:
  - The broader HUD dispatch owner remains blocked on class/table source-shape
    recovery. Current debt includes HudUiElement/HudUiPanel-derived dispatch
    owners, save/load list-item/dialog ownership, and related data gates.
  - Several prior local FTable/VTable factories, virtual dispatch views, and
    scalar-deleting wrappers were removed or downgraded, but broader HUD widget
    callback/table scaffolding remains active debt.
  - Key active anchors include 0x434680, 0x4bc9f0, 0x4bbff0, 0x4bc410,
    0x4bc3a0, 0x4b9540, 0x4b3d00, 0x4b4070, 0x404d70, 0x4b47a0, and
    0x404ca0.
- Next action:
  - Recover the source-faithful HudUiElement/HudUiPanel owner model before
    isolated save/load method promotion.
  - Use `python tools/recoil.py status 0x434680 --lane binary`,
    `python tools/recoil.py frontier 0x434680 --depth 1 --lane binary`, and
    a focused `guard source-shape` over the HUD files before editing.

### Group: RecoilApp owner EH tier S

- Anchor: 0x42de60 RecoilApp::Destructor and 0x42dfa0 RecoilApp::Constructor
- Reason: class owner and VC5SP3 constructor/destructor cleanup-state model.
- Current blockers:
  - The paired MFC/OLE owner dependencies and zFMV cleanup/reset dependencies
    have accepted local byte evidence, but the root RecoilApp constructor and
    destructor still differ from retail EH cleanup-state shape.
  - Source-shape guards are clean for the RecoilApp owner files, but current
    VC5SP3 compares for 0x42dfa0 and 0x42de60 still fail.
  - Historical EH probes and rejected source shapes are captured in
    `docs/reconstruction/recoil_app_destructor_tier_s.md`.
- Next action:
  - Recover the smallest source-faithful RecoilApp owner model that lets VC5SP3
    emit the paired member/base constructor and destructor cleanup chains.
  - Retry `python tools/recoil.py verify vc5 0x42dfa0` and
    `python tools/recoil.py verify vc5 0x42de60` after source changes.

### Group: Main menu transition state class cleanup

- Anchor: 0x415170 RecoilStateMainMenuTransition::Constructor and 0x408f50
  RecoilStateCredits::Constructor
- Reason: app-state/dialog host class cleanup and grouped data-gate audit.
- Current blockers:
  - Main-menu transition and credits state owners are class-shaped enough for
    caller work, but data gates and tier S evidence are not fully accepted.
  - The dialog-host target has zero unmasked byte mismatches, but tier S remains
    blocked until the data gate is audited.
- Next action:
  - Continue grouped data-gate audit and VC5 tier S recovery for
    `app.main_menu_transition`.
  - Recheck with `python tools/recoil.py status 0x415170 --lane binary` before
    editing.

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
- Next action:
  - Continue class-first cleanup with the remaining HUD overlay/widget owners.
  - Use focused status/frontier checks for the current anchor and source-shape
    guards over touched HUD files before editing.
