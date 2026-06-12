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
    fallback. 0x4a8920 is now tier S after adding the local VC5SP3 target,
    matching the retail local initialization order, zvid_dd.c literal
    source-file arguments, final SetClipper success/error branch layout, and
    primary-backbuffer flag vs attached-caps store order; `verify vc5
    0x4a8920` passes with zero unmasked byte mismatches after COFF relocation
    masking, and `verify functional 0x4a8920` passes. 0x4a8b20 now has local
    VC5SP3 target `zvideo_dd_create_fullscreen_software_surfaces`: source-shape
    repairs for the one-field GfxFlags fallback, zvid_dd.c literal source-file
    arguments, LockSurfaceState fallback branch layout, and final SetClipper
    branch layout improved the COFF compare from 491 to 7 unmasked mismatches,
    with 200 relocation-masked bytes, 11 trimmed VC NOP bytes, and the
    functional smoke still passing. Remaining 0x4a8b20 tier S drift is the
    fallback CreateSurface3FromDesc edx-load versus push scheduling; local
    fallbackDesc/fallbackSurface pointer probes were byte-neutral at the same 7
    mismatches and were reverted. 0x4a8dc0
    is now tier S after adding the local VC5SP3 target and matching the retail
    attached-caps local initialization order, zvid_dd.c literal source-file
    arguments, and final SetClipper success/error layout; `verify vc5
    0x4a8dc0` passes with zero unmasked mismatches after 128 relocation-masked
    bytes, and `verify functional 0x4a8dc0` passes. 0x4a88f0 is now tier S
    after adding local VC5SP3 target
    `zvideo_dd_create_fullscreen_surfaces_for_renderer`; `verify vc5
    zvideo_dd_create_fullscreen_surfaces_for_renderer` matches with zero
    unmasked byte mismatches after 20 relocation-masked bytes and 15 trimmed
    VC NOP bytes, and `verify functional 0x4a88f0` passes.
    0x4a9300 zVideo_dd::TeardownVideoSubsystem is now tier S after adding a
    local VC5SP3 target and passing zero-mismatch COFF bytes for
    ?TeardownVideoSubsystem@zVideo_dd@@YAXXZ. The refreshed 0x4a8f80 frontier
    now leaves 0x4ad6a0 zVideo_dd::ReportError and 0x4a6bf0
    zVideo_PixelPack::SetupFromMasks as its visible tier S blockers.
    A same-session ReportError source-shape probe split the monolithic HRESULT
    switch into numeric DDERR/D3DERR range switches to try to match the retail
    compare-chain/table boundaries; it worsened the VC5SP3 COFF compare from
    1610 to 2645 unmasked mismatches and grew the VC body from 3360 to 3552
    bytes, so the probe was reverted. The restored baseline is again 1610
    unmasked mismatches after 929 relocation-masked bytes and 4 trimmed VC NOP
    bytes, with the functional smoke passing.
    A same-session source-order experiment for 0x4a6bf0 that moved the mask
    stores before the shifted-mask temporaries worsened the VC5SP3 COFF compare
    from 109 to 113 unmasked mismatches and increased trimmed VC NOPs from 9
    to 13, so the source body was restored to the prior source-faithful shape.
    A later same-session pass refreshed the same 109-mismatch / 48
    relocation-masked-byte / 9-trimmed-NOP result, confirmed the functional
    smoke still passes, and tried two narrower rBits/direct-global-write
    source-order variants; both were byte-neutral at 109 mismatches, so the
    original source body remains the accepted source-faithful shape for now.
    0x4a8f80 now has a local VC5SP3 target
    `zvideo_dd_init_fullscreen_software_pixel_pack`: changing its two
    zvid_dd.c report call sites from the shared source-file pointer to the
    retail literal-source-file form improved the compare from 163 to 161
    unmasked mismatches, with 32 relocation-masked bytes, 8 trimmed VC NOPs,
    and the functional smoke still passing. Remaining tier S drift is COM
    vtable load shape, DDPIXELFORMAT dwSize scheduling, and supported-mask
    branch argument scheduling. Same-session source-shape probes that changed
    the GetPixelFormat call to use g_zVideo_DisplayModeSurfaceState.surf
    worsened the compare to 174 unmasked mismatches / 36 relocation-masked
    bytes / 5 trimmed VC NOP bytes, and changing DDPIXELFORMAT initialization
    to a first-member aggregate worsened it to 162 unmasked mismatches; both
    probes were reverted.
  - For the zRndr InitGlobals dependency chain, 0x4904d0
    zRndr::SetPerspectiveAdaptiveCorrection is now tier B after accepting the
    leaf namespace owner, the three span-depth-bias float BSS globals by symbol
    name, and the existing functional target; tier S
    remains blocked by VC5SP3 FPU zero/NaN reciprocal branch drift. The
    same-session explicit guarded `if`/`else` source-shape experiment for the
    reciprocal zero/NaN path was byte-neutral at the same 20 unmasked
    mismatches and was reverted, so the ternary remains the source-faithful
    behavior form while the retail fall-through zero-store branch shape remains
    unresolved. A later same-session `goto`/out-of-line reciprocal branch probe
    was also byte-neutral at 20 unmasked mismatches; functional still passed and
    the probe was reverted. The
    installed software-blit callback 0x48f560
    zVid_Image::BlitToFramebufferClipped is also tier B after registering the
    stale native smoke in the compiled primary-blit smoke unit, adding focused
    helper/function docblocks, accepting the typed zRndr framebuffer-region
    globals, and rerunning functional/native validation. A new local VC5SP3
    target `zvid_image_blit_to_framebuffer_clipped` now covers 0x48f560, but
    it fails with 2005 unmasked mismatches after 36 relocation-masked bytes and
    3 trimmed VC NOP bytes, BN size 2067 bytes and VC5 symbol size 1200 bytes,
    so 0x48f560 remains tier B pending a coherent zImage/zVideo software-blit
    source-cluster tier S pass. 0x49b1e0
    zRndr::FogColor_SetRgb01Clamped is now tier S after changing the clamped
    blue packing temporary to unsigned so VC5SP3 emits BN's logical right shift
    for the packed blue component, with `verify vc5 0x49b1e0` passing zero
    unmasked mismatches and `verify functional 0x49b1e0` passing. 0x48fd80 remains
    owner/data-pending on its own InitGlobals span/dispatch/global BSS audit
    rather than on those two lower leaves.
  - Image-surface helper source ownership and data gates are accepted for
    0x4a83d0, 0x4a84c0, 0x4a8500, 0x4a8650, 0x4a8680, and 0x4a86f0. These are
    tier B after accepting the zVidImagePartial surface/pixel/pitch ownership
    model, DirectDraw provider interfaces, selected-device caps, and ReportError
    source strings. A local VC5SP3 target now covers 0x4a8500
    `zvideo_dd_image_populate_surface_from_heap_pixels`; `verify functional
    0x4a8500` passes, but `verify vc5
    zvideo_dd_image_populate_surface_from_heap_pixels` remains blocked at 173
    unmasked mismatches after 20 relocation-masked bytes and 8 trimmed VC NOP
    bytes, with remaining drift in DirectDraw Lock/Unlock lost-surface retry
    control-flow/scheduling and descriptor/pitch-store ordering. Tier S remains
    deferred to a coherent DirectDraw image/surface source-cluster pass.
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
    0x4ad680 is tier B/no-globals. The zvid_ddd3d.c quad/flush slice
    0x4acd00, 0x4ad120, 0x4ace30, and 0x4ad250 is now tier B after recovering
    the shared BN-backed D3D render-state cache as one
    zVideo_D3DRenderStateCacheLive BSS record, accepting the typed quad,
    sorted, and overwrite queue globals, and passing the focused functional
    targets plus native CTest. Tier S remains deferred to a coherent
    zvid_ddd3d.c device-state/render-state source-cluster byte-verification
    pass.
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
    span-selection/global table owner. 0x49e140 is now tier S after typing
    g_mmxMaskGreenPacked as signed 16-bit lanes and clearing the former
    zero-extended versus sign-extended 0xffe0 immediate drift; 0x49ea40 is now
    tier S after the focused functional smoke and zero-mismatch VC5SP3 check.
    The
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
    not include it. Same-session BN assembly/HLIL for 0x49b7e0 confirms the
    texVShift 10..17 jump table, gRndr_SavedEspSlot save, ESP pivot to
    gRndr_CurrentSpanBaseAddr + count, 16-bit active-texel sampling, and
    push-word/sub-two transparent-zero write shape; current C++ remains normal
    descending pointer stores for tier C behavior. Same-session BN assembly/HLIL
    for 0x49e6c0 confirms the copy sibling uses the same texVShift 10..17
    jump-table and ESP-pivot setup but unconditionally pushes every sampled
    16-bit texel, so current C++ remains normal descending pointer stores for
    tier C behavior rather than recovered retail source shape. Same-session BN
    assembly/HLIL for 0x49edc0 confirms the pal8 copy sibling uses the same
    texVShift 10..17 switch, saves through gRndr_SavedEspSlot, pivots ESP to
    gRndr_CurrentSpanBaseAddr + count, samples 8-bit indices from
    gRndr_ActiveTexPixels, expands through gRndr_ActiveTexPalette, and
    push-writes the resulting 16-bit palette words backward; current C++ models
    this with normal descending pointer stores for tier C behavior. Same-session
    BN assembly/HLIL for 0x49bbf0 confirms the pal8 masked sibling uses the same
    texVShift 10..17 switch and ESP-pivot setup, skips source index zero by
    subtracting two bytes from ESP, and push-writes expanded nonzero
    gRndr_ActiveTexPalette words backward; current C++ models this as a
    transparent-zero guarded descending pointer store for tier C behavior. The active
    fixed-20 U/V step globals and current span-base pointer are renamed in
    source/tests to the BN active-texture model. The active texture pixel slot
    is now named and documented as the BN gRndr_ActiveTexPixels mixed pal8/tex16
    buffer, and 0x49e6c0 has been expanded from a compact helper loop into the
    BN-observed texVShift 10..17 switch-shaped reverse span source. VC5 still
    fails for 0x49e6c0 because the retail code uses the gRndr_SavedEspSlot
    ESP-pivot cursor while the source uses ordinary C++ pointer stores.
    Fresh 0x49b7e0 data-symbol checks pass for gRndr_ActiveTexVMask,
    gRndr_ActiveTexUMask, gRndr_ActiveTexPixels,
    gRndr_ActiveTexUStepFixed20, gRndr_ActiveTexVStepFixed20,
    gRndr_CurrentSpanBaseAddr, and gRndr_SavedEspSlot, but the plan data marker
    remains blocked behind the unresolved ESP-pivot source-owner shape.
    The sibling 0x49e6c0 target now carries the same seven data-symbol checks
    with zero unmasked mismatches; its function bytes still fail with 698
    unmasked mismatches because the source is ordinary C++ descending stores
    rather than the retail ESP-pivot push-write loop.
    Pal8 ESP-pivot siblings now carry matching active-data evidence:
    0x49edc0 and 0x49bbf0 verify gRndr_ActiveTexVMask,
    gRndr_ActiveTexUMask, gRndr_ActiveTexPixels, gRndr_ActiveTexPalette,
    gRndr_ActiveTexUStepFixed20, gRndr_ActiveTexVStepFixed20, and
    gRndr_CurrentSpanBaseAddr with zero unmasked data-byte mismatches. Their
    function bytes still fail, respectively, with 779 and 819 unmasked
    mismatches because the source is ordinary C++ descending stores rather than
    the retail ESP-pivot push/sub-esp loop. The shared gRndr_SavedEspSlot BSS
    pointer remains covered by the tex16 sibling data targets.
    The remaining minimum ESP-pivot leaves now have matching local data
    evidence as well: 0x4997d0 verifies gRndr_CurrentSpanBaseAddr with zero
    unmasked data-byte mismatches, while 0x49f180 verifies the seven pal8/span
    globals plus the split zrndr_span_shade_globals target for
    gRndr_ActiveShadeFixed16 and gRndr_ActiveShadeStepFixed16 with zero
    unmasked data-byte mismatches. Their function bytes still fail,
    respectively, with 63 and 829 unmasked mismatches because the retail
    routines use ESP/push reverse writes; owner/data markers remain blocked
    until the source-faithful ESP-pivot span-family model is recovered.
    The zVideo noise/scratch data subset for 0x48d340/0x48ff70 has been
    corrected to BN's BSS order from g_zVid_NoiseByteTableSize through
    g_zVideo_FxSurfacePitchPixels16, with 0x48ff70/0x42e330 still data-gated by
    the zRndr selector callback/global owner. Same-session refresh confirms
    `verify functional 0x48d340` passes and `verify vc5 0x48d340` remains
    zero-mismatch after 72 relocation-masked bytes, but its data marker remains
    blocked by the gRndr_pfnOverlayBlendRow write into the unresolved zRndr
    overlay/span callback-global owner. The zVideo clear-dispatch side
    gate is now resolved: 0x4a6760, 0x4a6830, and 0x4a7b20 are tier S after
    the focused native smoke and VC5SP3 zero-mismatch target. 0x4a7b30 is now
    tier S after adding it to the same clear-dispatch VC5SP3 target and
    matching the BN leaf load/return with zero unmasked mismatches after COFF
    relocation masking.
    Same-session frontier for 0x48d340 also shows its default
    gRndr_pfnOverlayBlendRow write is data-gated by
    0x48d450 zRndr::OverlayBlendRow555_Scalar; the scalar overlay row pair is
    part of the same zRndr callback/global owner audit. Same-session BN
    assembly/HLIL and functional evidence for 0x48d6d0
    zRndr_OverlayRect_Submit now classify it with this zRndr_Overlay.cpp
    owner: it records software overlay bounds/color/alpha or forwards the
    inclusive rectangle to zVideo_dd3d::QueueSolidQuad with right+1. A local
    VC5SP3 target `zrndr_overlay_rect_submit` builds production zRndr.cpp but
    fails COFF bytes with 131 unmasked mismatches after 48 relocation-masked
    bytes and 6 trimmed VC NOPs, so it remains tier C and owner/data-pending
    with the shared overlay callback/global group.
    Same-session BN
    HLIL/assembly for 0x48d7a0 confirms zRndr_OverlayRect_FlushSw selects the
    555/565 scalar or MMX row callback, precomputes the software overlay
    premul/destination-scale globals through x87/_ftol, and calls the selected
    row callback for each FX-surface row; it stays owner/data-pending with the
    row callback family instead of being promoted in isolation. Same-session BN
    assembly/HLIL for 0x48d510 and 0x48d5f0 confirms the selected MMX overlay
    row leaves build replicated mask, premul RGB-pair, and destination-scale
    vectors on the stack, process four 16-bit pixels per MMX qword, and finish
    with emms. Source now models the BN premul RGB-pair BSS globals written by
    0x48d7a0 and consumes them in four-pixel grouped scalar-emulated MMX row
    callbacks; 0x48d7a0, 0x48d510, and 0x48d5f0 are reconstructed, but
    owner/data stay pending with the overlay/span callback-global family because
    the retail MMX instruction source shape is not recovered. A read-only BN
    fact packet confirms all four row callbacks share the typed
    `void (__fastcall *)(uint16_t *, int32_t)` gRndr_pfnOverlayBlendRow ABI,
    have no meaningful xrefs except selector/global assignment, and consume the
    same precompute globals produced by 0x48d7a0; the local VC5SP3 include tree
    has no MMX intrinsic header for `_m_pmullw`/`_m_paddw`-style source, and
    production raw assembly remains forbidden. A source-owner mapping pass
    rejects splitting 0x48d450/0x48d4b0 into a scalar-only owner: the shared
    0x48d7a0 selector, gRndr_pfnOverlayBlendRow, and producer globals make the
    whole zRndr_Overlay.cpp row callback/global cluster the owner-sized unit.
    Refreshed same-session verification keeps this cluster saturated rather than
    promotable: functional targets for 0x48d450, 0x48d510, 0x48d5f0, and
    0x48d7a0 pass, but `verify vc5 0x48d450` still reports 63 unmasked scalar
    row mismatches and `verify vc5 0x48d510` reports 207 unmasked mismatches
    with BN's MMX prologue/stack-vector body versus the portable C row loop.
    The VC5SP3 include tree still exposes no MMX intrinsic header, and adding
    non-CPU-probe raw assembly remains forbidden without explicit approval, so
    the overlay row callback/global owner should not be reselected for B/S
    promotion until new approved MMX source evidence exists.
    0x4997d0
    zRndr::FillSpan16Opaque is another same-family push-write span blocker from
    the 0x48ff80 frontier even though older group membership output did not
    include it. Same-session status for 0x49edc0, 0x49bbf0, and 0x49f180
    shows the pal8 copy/masked/shade switch-vshift callbacks are also tier C
    owner/data-pending members of this zRndr_Span.cpp ESP-pivot family, even
    though older group membership output omitted them. Same-session BN HLIL for
    0x49c230 confirms the constant-alpha pal8 565 partial-alpha path uses the
    current destination word as the palette index before channel blending; source
    now documents that quirk locally, but 0x49c230 and the 0x49c020 wrapper
    remain owner/data-pending members of the same span callback/global audit.
    Same-session BN HLIL for 0x49c150 confirms the tex16-to-565 partial-alpha
    branch reaches destination-preserving math after the nonzero-source gate;
    source now documents the intentional empty partial-alpha branch instead of
    treating it as a missing blend. Same-session BN HLIL for 0x49c760 and
    0x49c860 confirms the neighboring tex16 constant-alpha loops share the
    active U/V texture sampling model, use alpha >3 for 565 and alpha >7 for
    555, copy for alpha >= 0xfc, and otherwise blend the respective channel
    masks; source now records those gates locally while owner/data stay pending.
    Same-session BN HLIL for 0x49d5c0 and 0x49d6e0 confirms the fast pal8
    constant-alpha loops expand sampled 8-bit texels through the active palette
    before the alpha gate, then use the same 565 alpha >3 and 555 alpha >7
    channel-blend gates; source now records those gates locally. Same-session
    BN HLIL for 0x49c970/0x49ca90 and 0x49d810/0x49d950 confirms the scaled
    alpha-map loops share the active U/V texture and alpha-map index, scale the
    alpha byte by the float stored in gRndr_ActiveConstAlphaBits, and then use
    the same 565 alpha >3 and 555 alpha >7 blend/copy gates for tex16 and pal8
    variants; source now records those gates locally. Same-session BN HLIL for
    0x49cbb0/0x49cea0 and 0x49da80/0x49ddb0 confirms the MMX-selected
    alpha-map loops allocate stack scratch storage, stage paired texel and alpha
    samples through the MMX U/V mask/step globals, run the packed group blend,
    then finish with scalar tails using the 565 alpha >3 or 555 alpha >7 gates;
    source now records that the current implementation scalar-emulates this
    retail MMX body while owner/data stay pending.
    The MMX scratch globals used by 0x49ea40 now use the BN zMmxQword lo/hi
    record name in source, and the initialized gRndr_QueuedTexAlphaMap startup
    sentinel is documented by symbol name rather than a raw image address.
    BN xrefs now show data_57dab8/data_57dabc as unreferenced zero BSS dwords
    after gRndr_MmxMask_BlueBits, so source leaves them as an unmodeled BSS
    gap instead of authored span/MMX state. Continue the shared zRndr_Span.cpp
    callback/global data owner audit for remaining zRndr span-family owner
    acceptance, including callback-family source shape, before returning to
    0x48ff70 or 0x42e330. The fog/MMX helper chain used by 0x49e400 and
    0x49e560 is now `static inline` with observed-caller provenance and both
    functional targets still pass, but those entries remain source-owner/data
    blocked because the source scalar-emulates the retail inline MMX quad body;
    VC5SP3 `vc5_o2_ob0_md_facs` still emits a 32-byte wrapper with 332
    mismatches. The fixed-vshift tex16 copy helper used by 0x49ea80 and
    0x49ec20 now uses an aligned packed two-pixel store after the scalar
    prologue, and both functional targets still pass; those entries remain
    owner/data blocked because retail still uses the MMX packed-index loop over
    gRndr_Mmx_* scratch records. Same-session BN HLIL for 0x49ea80 and
    0x49ec20 confirms the optional unaligned leading texel, paired U/V scratch
    setup, two-texel MMX index sampling, packed destination write, and odd-tail
    path now recorded in source comments. The scalar alpha-map callback slice
    0x49c360, 0x49c560, 0x49d1a0, and 0x49d3b0 is now tier B after accepting
    the recovered inline/static-inline helper provenance, the authored active
    texture globals, and callback-bank data for the zRndr_Span.cpp subsystem.
    Same-session BN assembly for those four callbacks confirms the alpha >= 8
    gate, alpha >= 0xf8 copy path, tex16 or pal8 palette-expanded source
    sampling, and 565/555-specific packed pair masks; VC5SP3 tier S remains
    blocked by helper inlining/codegen drift rather than owner/data evidence.
    The scalar constant-alpha callback slice 0x49c760, 0x49c860, 0x49d5c0,
    and 0x49d6e0 is now tier B after same-session BN/source review accepted
    the zRndr_Span.cpp subsystem owner and touched active-texture/constant-alpha
    globals. BN assembly/HLIL confirms the tex16 and pal8 U/V sampling, palette
    expansion where applicable, alpha >3 or >7 skip gates, alpha >= 0xfc direct
    copy paths, and 565/555 channel masks; VC5SP3 tier S remains blocked by
    loop/codegen drift rather than owner/data evidence.
    The scaled alpha-map constant-alpha callback slice 0x49c970, 0x49ca90,
    0x49d810, and 0x49d950 is also tier B. Same-session BN and subagent packets
    classify these as scalar zRndr_Span.cpp callbacks installed by
    SelectSpanRoutines with no provider/import, ESP-pivot, or MMX blocker; the
    touched authored data are the active texture/palette/alpha-map globals,
    constant-alpha bits, U/V masks and steps, current span pointer, and callback
    bank slots. The x87 float-to-int bias qword is a compiler/codegen literal,
    not authored mutable span data; tier S remains blocked by the fully inlined
    x87-bias/codegen shape.
    The remaining scalar generic-V-shift leaves 0x49c230 and 0x49c150 are tier B
    after same-session BN/source review accepted them as conventional
    zRndr_Span.cpp callbacks with no provider, ESP-pivot, MMX, or helper
    provenance blocker. 0x49c230 covers the pal8 565 constant-alpha body with
    the destination-word palette lookup quirk; 0x49c150 covers the tex16-to-565
    masked body whose partial-alpha branch preserves the destination. 0x49c020
    is now tier B after replacing the behavior-equivalent helper delegation with
    its own generic-V-shift pal8 565 loop; BN confirms the nonzero texel gate,
    alpha >3/>=0xfc gates, high-alpha palette copy, and destination-word palette
    lookup partial-alpha path.
    Fresh tier S checks for 0x49c230 and 0x49c150 remain blocked: 0x49c230
    reports 243 unmasked mismatches after 44 relocation-masked bytes and 15
    trimmed VC NOP bytes, while 0x49c150 reports 193 unmasked mismatches after
    28 relocation-masked bytes and 2 trimmed VC NOP bytes. Keep them tier B
    unless new source/codegen evidence explains the generic-V-shift frame and
    partial-alpha channel-math drift.
    The overlay row scalar callbacks now use the retail no-preentry-guard
    paired-loop shape: `zrndr_overlay_blend_rows` improves 0x48d450 from 72 to
    63 mismatches and 0x48d4b0 from 72 to 66 mismatches, with both functional
    smokes still passing. A scalar expression-order probe that computed both
    scaled lanes before adding premul globals was byte-neutral at the same
    63/66 mismatch counts and was reverted. Source owner/Data still remain
    pending for the shared zRndr_Overlay.cpp callback/global owner because
    0x48d510 and 0x48d5f0 are MMX row source-shape blockers.
    Same-session BN assembly/HLIL for 0x49f180
    confirms the shade switch-vshift body uses the texVShift 10..17 jump table,
    saves through gRndr_SavedEspSlot, pivots ESP to
    gRndr_CurrentSpanBaseAddr + count, samples 8-bit active texels, adds the
    high-five-bit shade mask from gRndr_ActiveShadeFixed16, advances
    gRndr_ActiveShadeFixed16 by gRndr_ActiveShadeStepFixed16, and push-writes
    shade-adjusted palette words backward; source now records those local facts
    while owner/data remain pending for the shared ESP-pivot family. A
    scaffold audit found the old unused
    unavailable-callback fallback helpers had no source/test/doc references and
    falsely claimed selector installation; those no-op helpers were removed
    from production source instead of preserving them as source-shape debt.
    A source-owner mapping pass for 0x49b7e0 confirms it must not be accepted
    alone: 0x4997d0, 0x49b7e0, 0x49bbf0, 0x49e6c0, 0x49edc0, and 0x49f180
    remain tier C/data-equivalent-only because current BN assembly proves real
    ESP-pivot or push-write framebuffer stores, and production raw assembly,
    naked helpers, and ABI scaffolds remain forbidden. Do not loop on isolated
    ESP-pivot leaves for source-owner promotion. The MMX scratch/mask setup
    data family is accepted for the two setup
    anchors: zRndr::SpanMmxSetPixelFormatMasks (0x49e140) is tier S after the
    signed green-packed lane typing, and zRndr::SpanMmxSetTexUvMasksAndVShift
    (0x49ea40) is tier S. The next
    non-ESP-pivot zRndr route is therefore the consumer source-shape audit for
    0x49ea80, 0x49ec20, 0x49e400, 0x49e560, 0x49cbb0, 0x49cea0, 0x49da80,
    and 0x49ddb0.
    Same-session status refresh for that consumer slice still reports all eight
    anchors as `Model: data-equivalent-only` with Source owner/Data blocked by
    retail MMX scratch, packed-index, packed-blend, or fog quad source shape;
    no source-faithful C++ path has been identified under the current
    no-production-raw-assembly rule. Do not reselect this slice for marker
    promotion without new BN/source evidence for the original MMX source model.
    0x492000 zRndr_RasterizePolyWithSpanList is no longer an owner blocker:
    same-session BN disassembly/decompilation for zRndr_Draw.cpp shows the
    two 64-entry fixed-point ScanConvertEdge tables, per-scanline edge cursor
    advance, gRndr_SpanAllocCursor staging, and dispatch through
    gRndr_pfnBuildSpanList/gRndr_pfnSelectedSpanOp. Production source now uses
    that edge-table shape, `verify functional 0x492000` passes, original-symbol
    guard and zRndr docblock audit pass, and the plan records Source owner
    source-file zRndr_Draw.cpp with `Model: source-faithful`. The 0x492000
    touched data gate is now accepted and the entry is tier B: BN/source review
    classifies the compiler literal pool separately and verifies the writable
    globals as individual zero-initialized 4-byte zRndr BSS scalars/pointers/
    callbacks. Local VC5 data-symbol checks cover g_frameBuffer (632050h),
    g_pitchBytes (63205ch), g_bytesPerPixel (632060h), g_scanConvertMode
    (57dac8h), g_inverseDepthBias (57dac0h), g_inverseDepthScale (57dac4h),
    g_spanAllocCursor (57dae0h), g_pfnBuildSpanList (6320a4h),
    g_pfnSelectedSpanOp (6320b0h), and g_spanCurrentSpanBaseAddr (56b270h)
    with zero unmasked data-byte mismatches. Tier S remains blocked by
    `verify vc5 0x492000` with 1872 unmasked function-byte mismatches after
    104 relocation-masked bytes (BN 1990 bytes, VC5 1072 bytes).
    Keep the remaining zVideo work scoped to coherent tier S passes unless a
    fresh frontier exposes another owner/data blocker.

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

### Group: RecoilApp owner EH tier S

- Anchor: 0x42de60 RecoilApp::Destructor and 0x42dfa0 RecoilApp::Constructor
- Reason: class owner and VC5SP3 constructor/destructor cleanup-state model.
- Current status:
  - The RecoilApp lifecycle owner blocker is resolved for 0x42de60, 0x42dfa0,
    0x42df10, 0x42df50, and 0x42e070. Current source removes the explicit root
    `RecoilApp::~RecoilApp` body and the three explicit FMV-state destructor
    bodies, adds `RecoilApp_FmvScript::~RecoilApp_FmvScript` as the original
    inline member cleanup helper, and lets VC5SP3 synthesize the owner
    destructors over embedded state members and the MFC/OLE base.
  - `python tools/recoil.py verify vc5 0x42de60 --build-root
    build/vc5-final-implicit-recoilapp-dtor-42de60` passes with zero unmasked
    mismatches under `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`; 36
    relocation bytes masked, 8 trailing VC NOP bytes trimmed, BN size 168,
    VC5 size 176.
  - `python tools/recoil.py verify vc5 0x42dfa0 --build-root
    build/vc5-final-implicit-recoilapp-dtor-42dfa0` passes with zero unmasked
    mismatches under the same profile; 60 relocation bytes masked, 14 trailing
    VC NOP bytes trimmed, BN size 194, VC5 size 208.
  - `python tools/recoil.py verify vc5 recoil_app_fmv_state_destructors
    --build-root build/vc5-probe-implicit-recoilapp-dtor-fmv-dtors` passes for
    0x42df10, 0x42df50, and 0x42e070 with zero unmasked mismatches; each masks
    24 relocation bytes, trims 2 trailing VC NOP bytes, and compares BN size 62
    to VC5 size 64.
  - The local `g_RecoilApp` BSS data-symbol VC5 check accepts the authored
    singleton data gate: BN types the symbol as `struct RecoilApp`, size 552,
    all-zero BSS; COFF symbol
    `?g_RecoilApp@@3VRecoilApp@@A` compares zero unmasked data-byte
    mismatches. The three direct FMV-state destructors touch no authored
    mutable globals beyond object fields and provider/compiler EH/vtable data,
    so their data gate is `❎`.
  - `python tools/recoil.py verify functional 0x42de60`, `0x42dfa0`,
    `0x42df10`, `0x42df50`, and `0x42e070` pass against the registered native
    smokes. Plan entries for these five addresses are now `Reimplemented [S]`
    with `Model: source-faithful`.
  - Reclassified the FMV scalar deleting destructors at 0x42ebd0, 0x42e0d0,
    and 0x42ed90 as accepted MSVC compiler provider-boundary glue. The authored
    source targets remain the direct destructors at 0x42df10, 0x42df50, and
    0x42e070.
  - Lower zFMV action-script blocker refreshed: the stale
    `0x4626b0` manual-vtable/scalar-deleting scaffold blocker was cleared after
    BN/source owner review, `guard source-shape --root src/GameZRecoil/zFMV`
    reported zero scaffold occurrences, and native functional targets passed.
    Same-session BN/data review accepted 0x48cf80 zReader::ReadNamedString as
    tier S/no-globals, then accepted 0x4626b0
    zFMV_Script::LoadActionsFromZrd as tier B/data `✅` from its matched
    FMV_PATH/IMAGE_PATH/action-tag/source-path/error string `.rdata` and
    accepted action-script owner. Same-session include-order cleanup now
    includes `Battlesport/RecoilApp.h` before Win32/DirectX-bearing zFMV/video
    headers in `fmv_script.cpp`, repairing the MFC42 `WINDOWS.H already
    included` VC5 diagnostic for `zfmv_script_cleanup_reset`.
    `python tools/recoil.py verify vc5 zfmv_script_cleanup_reset --build-root
    build/vc5-verify-fmv-cleanup-reset-include-fixed` now reaches byte
    comparison; 0x462f10 zFMV_Script::AppendAction and 0x463120
    zFMV_Script::BeginNow are tier S with zero unmasked mismatches. The target
    still reports real byte drift for 0x4626b0, 0x462f90, and 0x463000; the
    former 0x4630a0/0x4630e0 time-scale drift is cleared by the narrower
    `zfmv_script_time_wrappers` VC5SP3 target after recovering the named
    float global. 0x462630 and 0x462660 were already tier S.
    0x463130, 0x4631f0, 0x4633c0, 0x463570, 0x463b00, and 0x463850 remain
    `Reimplemented [B]` unless separately verified.
    A later VC5SP3 source-shape pass made `DuplicateCString` a true
    `static inline` no-standalone helper and changed the two
    zFMV_ActionImage constructors to emit direct CRT `_strdup` calls; the
    scaled constructor also leaves the dummy active-region output uninitialized,
    matching BN. This improves `zfmv_script_cleanup_reset` 0x4626b0 from 1821
    to 1630 unmasked mismatches, `zfmv_action_constructors` 0x463130 from 87
    to 60, and 0x4631f0 from 94 to 59. Functional smokes still pass, but tier S
    remains blocked by constructor vtable/next store order and copy scheduling.
    A follow-up owner pass recovered the inline zFMV_Action base constructor
    that clears `next` before derived constructors install their concrete
    action vtable. This promotes 0x463850 zFMV_ActionBlur::Constructor to
    tier S via `zfmv_action_constructors` zero-mismatch VC5SP3 evidence and
    improves 0x463130 to 38 mismatches, 0x4631f0 to 36, and 0x463570 to
    one commutative LEA mismatch; 0x4633c0 and 0x463b00 remain tier-S blocked.
    Current follow-up recovered `zFMV_ActionImage::blitRect` as the existing
    `zVidRect32` source type and accepted 0x463570
    zFMV_ActionPlayAvi::Constructor as tier A only: grouped VC5SP3 evidence
    leaves exactly one unmasked SIB base/index encoding byte in the allocation
    size LEA, so tier S remains blocked.
    Follow-up recovered `zVid_PackColorRGB` as byte-channel source parameters
    and reordered the fade constructor stores to match retail scheduling. The
    grouped `zfmv_action_constructors` VC5SP3 target now accepts 0x4633c0
    zFMV_ActionFade::Constructor as tier S with zero unmasked mismatches after
    relocation masking; 0x4a6cf0 zVid_PackColorRGB itself remains tier B and
    now records 69 unmasked S-drift bytes under its standalone VC target.
    Same-session MCI follow-up recovered 0x462330 as
    zFMV_Playback::Constructor instead of Init, updated BN/source/functional
    target naming, and changed 0x463b00 to use
    `new zFMV_Playback(mediaPath, hwnd)` with an uninitialized dummy
    active-region output. `zfmv_action_constructors` now leaves 0x463b00 at 7
    unmasked bytes after 68 relocation-masked bytes and 1 trimmed VC NOP:
    stack-slot displacement bytes plus the known calloc-size LEA SIB
    operand-order encoding drift shared with 0x463570. 0x463b00 remains tier B
    and source-faithful, not tier S.
  - Follow-up on 0x462f90 zFMV_Script::BeginCurrentAction routed through
    zSndSampleSet::InitByName. Same-session zSound sample-set review registered
    the existing native smokes and accepted 0x4a0920
    zSndSampleSetRegistry::FindByName as tier B/data `✅`,
    0x4a0fb0 zSndSampleSet::LoadSamplesFromIndexArchive as tier B/data `❎`,
    and the source-owner/no-direct-globals gates for 0x4a0860
    zSndSampleSet::InitByName. Follow-up data review accepted the 0x4a0c40
    zSndSampleSet::Init authored globals: BN's initialized archive-bank
    selector is matched by source initializing `g_zSnd_UseArchiveBanksFlag` to
    1, BN's SoundLOD pointer/default pair is matched, and BN's search-path
    list pointer lifecycle through preinit/config/shutdown is matched.
    0x4a0c40 is now tier B/data
    `✅`; 0x4a07f0 zSnd::SetUseArchiveBanksFlag is tier S from focused
    functional and zero-mismatch VC5 evidence. 0x4a0860 remains tier B because
    direct callees 0x4a0920 and 0x4a0c40 remain below tier S.
  - Same-session zReader/zUtil dependency cleanup for 0x4a0c40 accepted the
    zIndexArchive helper chain and ZRDR path resolver. 0x4a6190 Reset,
    0x4a61b0 Destroy, 0x4a61d0 Init, 0x4a62b0 CloseAndFreeRecords, 0x4a62f0
    EnsureCapacity, 0x4a6330 FreeRecordsAndReset, and 0x4a63f0
    LoadIndexFromTail are now tier S/source-faithful; 0x4a6360
    FlushIndexToTail is tier B with 50 unmasked VC5 byte mismatches remaining.
    0x4a5e50 zUtil_ZRDR_ResolvePathInSearchPathList and 0x4a5f20
    zUtil_ZRDR_SearchPathContainsFilePredicate are tier S/data `✅` after
    correcting `g_zRdr_SplitDriveBuf` to BN's 4-byte BSS shape.
  - Same-session zSound wave-data follow-up registered the existing native
    wave-data and sample-init smokes, corrected the test A3D fake-provider
    vtable so `SetTransformMode` lands at the SDK slot, and promoted
    0x4a53f0, 0x4a5440, 0x4a5540, 0x4a5460, 0x4a5c50, and 0x4a2ea0 to tier S
    from focused functional and VC5 evidence. 0x4a55c0 Reset is tier B/data
    `❎` with no VC5 byte target. A follow-up zReader/archive dependency pass
    promoted 0x4a65d0 zIndexArchive::FindRecordByNameCI, 0x4a6670
    zIndexArchive::ReadFileByName, and 0x4a5600
    zSndWaveData::LoadAndParseFromIndexArchiveIfNeeded to tier S from focused
    functional and VC5 evidence. A follow-up lifetime recovery added inline
    0x4a53f0 as the real zSndWaveData constructor, converted the queued
    streaming stack wrapper and tests to automatic lifetime, registered the
    queued streaming smoke, and kept 0x4a53f0/0x4a5440 tier S. The refreshed
    0x4a0c40 frontier still exposes 0x4a0fb0
    zSndSampleSet::LoadSamplesFromIndexArchive as the direct tier S blocker.
    The local VC5 target for 0x4a0fb0 still fails, but the source now carries
    the retail EH frame, constructor new-expression, and EBX/EDI sample/flag
    cursor shape; current best remains 74 unmasked mismatches, 40
    relocation-masked bytes, BN size 223, VC5 size 224. Same-session
    flag-update expression probes did not improve that byte shape, and the
    remaining drift is flag-update/store scheduling around the delete cleanup
    path.
    The zFMV LoadActionsFromZrd zReader callee frontier is now narrower:
    0x48d1c0 zReader::OpenFileFromMountedArchives and 0x48cda0
    zReader::AllocateNode are tier S after current zreader_load_node_from_path
    functional evidence and zero-mismatch VC5SP3 COFF bytes. Follow-up VC5
    range repair set the local 0x48d080 zReader::ReadNode target to compare
    BN's full 320-byte range through the post-ret npad, four-entry switch jump
    table, and trailing NOP padding; `verify vc5 0x48d080` now passes with
    zero unmasked mismatches, and 0x48d080 is tier S. The unblocked
    0x48cdc0 zReader::LoadNodeFromPath and 0x48ce40
    zReader::FreeLoadedTree targets also pass zero-mismatch VC5SP3 COFF byte
    verification and are tier S.
  - Same-session zFMV action-constructor source-shape follow-up replaced the
    scaffold-style `Constructor*` methods for 0x463130, 0x4631f0, 0x4633c0,
    0x463570, 0x463b00, and 0x463850 with real C++ constructors and converted
    LoadActionsFromZrd/RecoilApp/tests to constructor new-expressions or
    placement construction. Native build and all focused constructor
    functional targets pass. The local `zfmv_action_constructors` VC5SP3
    target now uses `vc5_o2_ob1_md_gx_facs`; tier S remains blocked by real
    byte drift, with 0x463850 narrowed to 7 unmasked mismatches caused by
    retail storing `next` before the action vtable while VC5 C++ constructors
    emit the vtable store first. Do not promote the action constructors above
    tier B from this attempt.
  - Same-session FMV caller follow-up registered the missing
    `zfmv_script_begin_current_action_smoke` in the native smoke binary,
    reran the focused functional target, and accepted 0x462f90
    zFMV_Script::BeginCurrentAction to tier B/source-faithful with class owner
    `zFMV_Script` and the authored `"FMV"` string data gate accepted. Tier S
    remains deferred: `python tools/recoil.py verify vc5 0x462f90` still reports
    60 unmasked mismatches in the FMV script cluster virtual-call/codegen shape.
  - Follow-up on 0x463000 zFMV_Script::Update cleared the zSound snapshot stop
    dependency: 0x4a0500 zSndPlayHandleSnapshot::StopAllIfPlaying is now tier S
    after source-owner/data review, accepted g_zSnd_ActiveBackend data, focused
    functional evidence, and zero-mismatch VC5SP3 COFF verification. The
    refreshed frontier now routes through zInput::PollActiveDevices into the
    zInput polling/source-state owner. A source-owner packet classifies this as
    the static zInput subsystem/source-file cluster rather than a C++ class or
    isolated dispatcher. The first true mouse leaf, 0x470310
    zInput::Mouse_UpdateAcquireState, is now tier B after accepting the
    zin_mouse.cpp acquire-state leaf owner and the g_zInput_MouseActive /
    g_zInput_MouseDevice BSS data-symbol checks; tier S remains blocked by the
    known 54 unmasked VC5SP3 byte mismatches in global-load/branch scheduling.
  - zInput mouse-state leaf follow-up accepted 0x4704f0
    zInput::Mouse_ApplyAccumulatedDelta to tier B after BN review of the
    sensitivity/client/snapshot globals and zero-mismatch VC5SP3 data-symbol
    checks for every touched mouse-state global; tier S remains blocked by the
    known 93 unmasked function-byte mismatches. 0x4702e0
    zInput::Mouse_GetButtonTransitionState is now tier S after restoring the
    BN-observed current-first button byte-load shape, adding the function
    docblock, and passing VC5SP3 COFF bytes plus current/previous state data
    symbols.
  - zInput bind-map callback ABI follow-up recovered `zInputCommandCallbackFn`
    as `__fastcall (int commandId)`. 0x470b10, 0x470d40, 0x470db0, and
    0x470e80 are now source-faithful/tier S after focused functional smokes
    and zero-mismatch VC5SP3 COFF verification; 0x470d40 and 0x470e80 keep
    `Reconstructed ☑️` only for BN's documented indirect-call/tail-jump
    limitations.
  - zInput mouse polling owner/data follow-up accepted 0x4703c0
    zInput::Mouse_PollState and 0x4703b0
    zInput::Mouse_PollAndStoreState to tier B after restoring the BN-observed
    global raw mouse DI state buffer, correcting
    g_zInputMouseLastPollResult's initialized value, accepting the
    zin_mouse.cpp mouse polling source-state owner, and passing focused
    functional/data-symbol checks. Tier S remains blocked: 0x4703c0 still has
    142 unmasked VC5SP3 function-byte mismatches, and 0x4703b0 has no VC
    function-byte target yet.
  - zInput joystick polling owner/data follow-up accepted 0x471fb0
    zInput::DI_AcquireJoystickDevice and 0x4722c0
    zInput::DI_PollJoystickState to tier B, and promoted 0x472390
    zInput::DI_GetCurrentState to tier S, after restoring the BN-observed
    global raw joystick DI state buffer and passing focused functional and
    data-symbol checks. Tier S remains blocked for 0x471fb0 by the known
    boolean-return codegen drift and for 0x4722c0 by remaining function-byte
    mismatches. The follow-up joystick transition consumers 0x4723a0
    zInput::DI_GetButtonTransitionState and 0x4723d0
    zInput::DI_WaitForButtonPress are now tier B/source-faithful under the
    same zin_joystick.cpp owner after adding docblocks, passing focused
    functional smokes, and verifying current/previous joystick state data
    symbols with the narrow local `zinput_joystick_transition_wait` VC5SP3
    target; tier S remains blocked by 26 and 43 unmasked function-byte
    mismatches respectively.
  - zInput keyboard DIK ASCII-table follow-up accepted 0x46fd20
    zInput::Keyboard_InitDikToAsciiTable to tier S and 0x46fba0
    zInput::Keyboard_TranslateDikToAscii to tier B after matching the
    BN-observed numpad store order, adding required docblocks, and passing
    focused functional plus table/ready-flag data-symbol checks. Tier S remains
    blocked for 0x46fba0 by shifted-punctuation switch-layout codegen drift.
  - zInput keyboard polling owner/data follow-up is active for 0x46f690
    zInput::Keyboard_PollState. Same-session BN evidence identifies the owner
    as the zin_kbd.cpp keyboard source-state slice and the touched BSS globals
    as g_zInput_KbdDevice, g_zInput_KbdEventBuffer,
    g_zInput_KbdModifierState, g_zInputKbdKeyDispatchTable,
    g_zInput_KbdRawEventCallback, and g_zInput_KbdRawEventCallbackCtx.
    Source owner/model are now accepted after adding recovered-helper
    docblocks, removing stale unused keyboard helper/table artifacts, and
    passing the focused functional target. The local VC5SP3 target
    `zinput_keyboard_poll_state` verifies those six BSS data symbols with zero
    unmasked mismatches. The direct callee 0x472490 zInput::DI_ReportError is
    now accepted through tier B from source-owner and DirectInput HRESULT rdata
    evidence; its remaining tier S debt is a VC5SP3 compare failure with 170
    unmasked mismatches after 84 relocation-masked bytes and 7 trimmed VC NOPs.
    Therefore 0x46f690's remaining blocker is tier S function-byte drift, not
    owner/data coverage. 0x471de0 zInput::PollActiveDevices is now accepted
    through tier A after adding a VC5 target for the dispatcher and its six
    active-device flag/refcount globals; the data symbols are byte-identical
    and the remaining function-byte drift is only EBX versus ESI register
    allocation for saving/reloading dispatchCallbacks. A follow-up probe that
    inverted only the final
    DIERR_ALREADYINITIALIZED/DI_OK/unknown branch to force the
    already-initialized case out of line worsened the 0x472490 VC5SP3 compare
    to 175 unmasked mismatches and was reverted. A second probe that made the
    high-range S_OK test fall through directly before the unknownError label
    matched the retail `je`/`test` order but forced a larger tail reshuffle,
    worsening the compare to 347 unmasked mismatches; it was also reverted.
  - MpExit lifecycle dependency follow-up routes 0x4198d0/0x419940 through
    zInput bind-map overlay push/pop. Current BN/source review classifies the
    lower dependency slice as the authored `zInput_BindMapContext` record/class
    owner plus the zin_kbd.cpp keyboard callback table helper. The typed
    bind-map context layout is already recovered in `zInput.h`, and same-session
    BN assembly confirms `InitFromTemplate`, `FreeAllBuffers`,
    `RebuildLookupIndices`, `GetPrimaryKeyboardKey`, `GetSecondaryKeyboardKey`,
    `SetCommandCallback`, and `Keyboard_RegisterKeyCallback` match that owner.
    The current `zinput_keyboard_poll_state` VC5SP3 data-symbol check keeps
    `g_zInputKbdKeyDispatchTable` byte-identical; the broader keyboard polling
    function still has tier-S byte drift and is not promoted from that target.
    Follow-up review accepted the push/pop overlay globals
    (`g_zInput_BindMap_Current`, free list, stack head, depth) through
    `zinput_bindmap_overlay_globals`, promoted 0x471860/0x471950 and the
    reset chain 0x470a10/0x4709d0/0x4716c0 to tier B, and repaired the
    `recoil_app_mp_exit_dialog_state_on_try_become_current` smoke registration
    by migrating the existing smoke into `zhud_widget_tests.cpp`. 0x4198d0 is
    now dependency-ready and tier C; owner/data remain blocked at the broader
    MpExit lifecycle source-file/class gate.
  - Historical EH probes and rejected source shapes are captured in
    `docs/reconstruction/recoil_app_destructor_tier_s.md`.
- Next action:
  - Continue the RecoilApp/zFMV caller path after the 0x463000 follow-up:
    0x46fa10 zInput::Keyboard_WaitForAnyKeyPress is now source-faithful tier S
    after replacing the helper-call wait-event body with the recovered inline
    zin_kbd.cpp loop shape; `python tools/recoil.py verify functional
    zinput_keyboard_wait_for_any_key_press` passes and `python tools/recoil.py
    verify vc5 zinput_keyboard_wait_for_any_key_press` reports zero unmasked
    function-byte mismatches plus zero unmasked data-byte mismatches for the
    keyboard device, event-buffer, modifier-state, and dispatch-table globals.
    `python tools/recoil.py verify functional 0x463000` passes, and 0x463000
    zFMV_Script::Update is tier B/source-faithful with no authored globals
    touched. The former data/tier S blockers for 0x4630a0
    zFMV_Script::BeginAtTime and 0x4630e0 zFMV_Script::UpdateAtTime are now
    cleared: BN confirms the named float
    g_zFMV_ScriptTimeGetTimeToSecondsScale at data RVA 4d2580h with xrefs only from
    those wrappers, and the narrow `zfmv_script_time_wrappers` VC5SP3 target
    reports zero unmasked function-byte mismatches for both wrappers plus zero
    unmasked data-byte mismatches for the float. The remaining zFMV blocker is
    tier S/codegen only for 0x463000 (74 unmasked byte mismatches, narrowed by
    classified asm drift to 2 unaccepted instruction-shape mismatches). Same-session
    refresh confirmed the direct 0x471de0 zInput::PollActiveDevices callee is
    already tier A; retail saves/reloads only CL/BL while the accepted source
    and VC target use the existing int fastcall signature, so the caller's
    `mov ecx, 1` versus retail `mov cl, 1` mismatch should not be chased by a
    local zFMV cast without an owner-level zInput ABI review.

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
  - Same-session functional-target repair registered and implemented the
    missing native smokes for `recoil_state_cheat_code_constructor`,
    `recoil_state_controls_lifecycle`, and
    `recoil_state_confirm_quit_constructor`; all three targets now pass.
    Current BN evidence for 0x406ed0, 0x408d60, and 0x415850 shows only
    compiler vtable installation and dialog-pointer clearing with no authored
    globals touched directly. Follow-up VC5SP3 evidence added the local
    `recoil_state_hud_leaf_constructors` target; moving the 0x406ed0 and
    0x408d60 dialog-pointer clears from member initializers into constructor
    bodies matched retail vtable-first store order, and both constructors now
    compare with zero unmasked byte mismatches after COFF relocation masking.
    0x406ed0, 0x408d60, and 0x415850 are tier S/data `❎`; moving the
    0x415850 dialog-pointer clear from the member initializer into the
    constructor body matched the retail vtable-first store order and cleared
    the former 7 unmasked VC5SP3 mismatches.
  - Same-session HUD options owner/data audit cleared the stale
    HudOptionsDialog/HudUiOptionsPanelOverlayOwner table-factory blocker.
    BN and functional evidence promoted 0x40cf60, 0x40cf00, 0x40d0a0,
    0x40d0b0, 0x40d0e0, and 0x40d150 to tier B/source-faithful; no tier S
    promotion was made because these entries still lack VC5 byte targets or
    remain verify-only B debt. Follow-up same-session queue evidence repaired
    the missing `recoil_app_queue_push_state_smoke` registration, promoted
    0x443310 RecoilApp::QueuePushState to tier B with `Data reimplemented`
    `❎`, and cleared the 0x40d1c0 QueueEnter data gate to tier B/`✅`; both
    remain tier-S verify debt rather than owner/data blockers.
  - Follow-up zOpt/HUD queue cleanup cleared stale lower blockers: 0x407f10
    zOpt::SetGameDifficultyMode and 0x407f20 zOpt::GetGameDifficultyMode now
    have source-faithful namespace owner/data gates and VC5SP3 tier S byte
    evidence; 0x408190 zOpt::GetPlayerName was already tier S. Same-session
    smoke-registry repair made `hud_ui_new_game_panel_overlay_owner_queue_enter`
    and `recoil_state_confirm_quit_queue_enter` runnable, and current BN/data
    evidence promoted 0x41c6c0 and 0x4159b0 to tier B/data `✅`.
  - Same-session option-selector cleanup cleared the new-game difficulty-sync
    owner blocker: `zhud_zrd_widget_ex17c_item_core_smoke` was registered in
    the compiled native smoke runner, 0x4b8a90
    HudUiZrdWidgetEx17C_Item::SetSelected and 0x4b8cf0
    HudUiZrdWidgetEx17C::SetSelectedIndex now have accepted class owner/data
    gates and `Model: source-faithful`, and 0x41c4e0
    HudUiNewGamePanel::SyncIntensityFromDifficulty is now tier B/data `✅`
    using the accepted zOpt difficulty and option-selector dependencies.
  - Same-session new-game cleanup repaired the missing
    `zhud_text_input_destructor_core_smoke` registration, refreshed VC5SP3
    byte evidence for 0x4b4370 HudUiTextInput::DestructorCore, and promoted
    0x4b4370 to tier S. Current frontier/functional evidence then promoted
    0x4b4ac0 HudUiNumericTextInput::Destructor, 0x4b8b60
    HudUiZrdWidgetEx17C::DestructorCore, 0x41c400
    HudUiNewGamePanel::Destructor, 0x41c3e0
    HudUiNewGamePanel::ScalarDeletingDestructor, 0x41c4c0
    HudUiZrdWidgetEx17C::ScalarDeletingDestructorThunk, and 0x41c3b0
    HudUiNewGamePanel_NameInput::OnActivate to tier B/source-faithful. These
    remain tier-S verify debt where no VC5 byte target exists. 0x40cf20
    HudUiZrdWidget::DestructorCoreThunk is now an accepted provider-boundary
    forwarding thunk; 0x41c480 should be rechecked against its current lowest
    binary-lane blocker before promotion.
  - 0x41c290 HudUiNewGamePanel::Constructor now maps to a real C++
    HudUiNewGamePanel constructor in source rather than a manual
    `Constructor()` initializer. Same-session member-owner cleanup changed the
    BACK member to `HudUiMenuBackButton` and the INTENSITY member to a
    panel-owned `HudUiNewGamePanel_Intensity`, matching the BN table writes
    inside the constructor. Same-session validation passes the native x86 build,
    `recoil_native_smoke`,
    `hud_ui_new_game_panel_constructor_cluster`, and
    `hud_ui_new_game_panel_overlay_owner_on_try_become_current`. Local VC5
    target `hud_ui_new_game_panel_constructor` now passes 0x41c290 with zero
    unmasked mismatches after 112 relocation-masked bytes, so 0x41c290 is tier
    S/source-faithful with data `✅`. The accepted constructor-owned
    table/string references include `g_HudUiNewGamePanel_FTableHeader`, child
    widget vtables, and `"dialog.zrd"`, `"NEWGAMEPANEL"`, `"BACK"`, `"START"`,
    `"NAME"`, and `"INTENSITY"`. Follow-up BN body review of 0x41c560 shows
    no authored global data access beyond compiler EH glue; with the accepted
    constructor dependency and passing
    `hud_ui_new_game_panel_overlay_owner_on_try_become_current` target,
    0x41c560 is tier B/data `❎` and remains verify-only tier-S debt.
  - Same-session HudUiZrdWidget scalar destructor cleanup repaired the stale
    `zhud_zrd_widget_helpers_smoke` registration by moving stable helper,
    destructor-core, scalar-delete, invalidation, and bounds coverage into the
    compiled native smoke runner. `python tools/recoil.py verify functional
    0x4b50a0` now passes again, and the already-passing
    `hud_ui_new_game_panel_constructor_cluster` target still covers 0x41c480.
    Current BN/source evidence identifies both entries as HudUiZrdWidget class
    members with no direct authored globals touched, so 0x4b50a0 and 0x41c480
    are now tier B/source-faithful with `Data reimplemented` `❎`. They remain
    tier-S verify debt; initialized dispatch-table references are still part
    of the broader HUD table/data audit, not direct local data blockers.
  - Same-session MpExit lower-blocker cleanup repaired stale native smoke
    registrations for `hud_ui_triplet_is_local_player_first_entry` and
    `hud_ui_mgr_is_local_player_first_in_stats_list`, added local VC5SP3 data
    coverage for `g_HudUiMgrStatsList` at data RVA 4ed4e0h, and promoted
    0x4143a0 HudUiMgr::IsLocalPlayerFirstInStatsList plus 0x40eab0
    HudScoreboard::SetScaleAndRebuild to tier S/data `✅`. The direct
    0x419500 HudUiMpExitDialog::LoadLayout frontier no longer has visible
    owner/data blockers from the local-player/stats-list path; remaining
    visible direct blockers are tier-S-only dependencies such as 0x4138d0,
    0x413910, 0x4a5bf0, 0x4a6e80, and 0x4bd410.
  - Same-session follow-up repaired the compiled native registration for
    `hud_ui_mp_exit_dialog_load_layout`, verified the target with
    `python tools/recoil.py verify functional 0x419500`, and promoted
    0x419500 to Reimplemented [C] with `Reconstructed` and source
    dependencies accepted. The owner/data gates remain open on the
    HudUiMpExitDialog/HudUiBackground/HudUiElement dispatch source model and
    touched data audit; `python tools/recoil.py verify vc5 0x419500` still
    fails with 81 unmasked byte mismatches after relocation masking.
  - Same-session MpExit class/source-owner follow-up repaired and registered
    the compiled `hud_ui_mp_exit_dialog_table_cluster` native smoke, replacing
    stale table-slot assertions with source-level behavior checks for
    UnloadLayout, Update, button activation state transitions, and destructor
    cleanup. The source now recovers inline HudUiMpExitDialog/button
    constructors so VC5 emits the dialog and button dispatch tables instead of
    relying on manual base/member construction. `python tools/recoil.py verify
    functional hud_ui_mp_exit_dialog_table_cluster`,
    `hud_ui_mp_exit_dialog_load_layout`, and
    `recoil_app_mp_exit_dialog_state_on_enter` pass. Local VC5 data-symbol
    checks match the dialog vtable, both button vtables, and singleton BSS with
    zero unmasked data-byte mismatches. 0x419500, 0x419650, 0x419690,
    0x419740, 0x419800, 0x419830, and 0x419870 are now source-faithful tier B;
    tier S remains open for the cluster because the broad VC5 function compare
    still fails for 0x419500/0x419650/0x419690/0x419800/0x419870.
  - Same-session OnDeactivate follow-up recovered the inherited
    HudUiBackground scalar-deleting destructor slot so
    RecoilApp_MpExitDialogState::OnDeactivate emits the retail virtual delete
    dispatch at vtable slot +8. `python tools/recoil.py verify functional
    0x419940` and `python tools/recoil.py verify vc5 0x419940` pass; 0x419940
    is now source-faithful tier S with owner/data accepted. The shared
    HudUiBackground constructor VC5 target 0x4b9540 still passes after the
    slot/signature cleanup.
  - Same-session follow-up repaired the missing functional bookkeeping for
    0x4bc760 HudUi::SetInvalidateMode by adding the direct
    `hud_ui_set_invalidate_mode_smoke` runner entry and local functional
    manifest. `python tools/recoil.py verify functional
    hud_ui_set_invalidate_mode` passes, the existing VC5SP3 target still reports
    zero unmasked byte mismatches, and 0x4bc760 is now tier S. The refreshed
    0x4198d0 frontier no longer routes through SetInvalidateMode.
- Next action:
  - Continue class-first cleanup on the remaining MpExit lifecycle entries:
    0x4198d0 and 0x419940 are tier S, while 0x419990
    RecoilApp_MpExitDialogState::OnUpdateShouldQuit is tier B with owner/data
    accepted. Its current frontier recommends 0x471de0 zInput::PollActiveDevices
    first, but that entry is already tier A with EBX/ESI register-allocation
    drift only; the next practical direct blockers visible from 0x4198d0 are
    0x4a0860 zSndSampleSet::InitByName, 0x471860
    zInput::PushBindMapContextOverlay, 0x4716c0
    zInput::BindMapCurrent_ResetAllBindings, and 0x419500
    HudUiMpExitDialog::LoadLayout.
  - Use focused status/frontier checks for the current anchor and source-shape
    guards over touched HUD files before editing.
