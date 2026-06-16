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
  - Same-session zRndr queued submit/flush owner-data slice accepted
    0x499a20, 0x499c40, and 0x49a2b0 to tier B in the same `zrndr_draw.c`
    source-file cluster. Current BN evidence confirmed the transparent and
    overwrite queue records/counts, transparent sort-index bank, saved
    inverse-depth/scan-convert globals, active palette remap key, and CRT
    `memcpy` provider; source now carries queue-bank address notes and offset
    guards for the touched record fields.
  - Same-session zRndr lens-stage owner-data slice accepted 0x49aa90 and
    0x49b020 to tier B in `zRndr_LensFlare.cpp`. Current BN evidence confirmed
    the active-region width/height/rect globals, four visible-stage texture
    pointers, hardware renderer dispatch globals, and the accepted software
    textured-submit dependency; source now records the BN source-file evidence
    in both docblocks and uses the recovered zVideo submit-proc typedef.
  - Same-session zRndr fog-color leaf pass accepted 0x49b780 to tier B in
    `zRndr_Fog.cpp`. Current BN evidence confirmed the active fog record,
    packed channel fields, and pixel-pack masks; source now records the BN
    source-file evidence and mask-bank addresses.
  - Same-session zVideo palette-remap owner/data slice accepted 0x46e720 to
    tier B. Current BN evidence confirmed the recipe count/array, variant
    table count/array, image texture-directory table, CRT `realloc` provider,
    and accepted palette-variant helpers; the zRndr palette setters 0x499930,
    0x499990, and 0x499a00 remain the next palette bridge blockers.
  - Same-session zRndr palette bridge owner/data slice accepted 0x499930,
    0x499990, and 0x499a00 to tier B in the zRndr span source-file cluster.
    Current BN evidence confirmed active remap-key globals, the accepted zVideo
    palette-remap builder, and `_ftol` provider; source now records the
    original zRndr_Span.cpp evidence and global addresses.
  - Cached-client-rect update-mask mini-owner covers 0x4a59a0, 0x4a59b0,
    and 0x443a40 plus `g_zVid_CachedClientRectUpdateMask` (data 56b564h). BN shows
    only the setter and query touch the zero-initialized mask. The namespace
    source shape is restored for 0x4a59b0/0x443a40, old comments are converted
    to docblocks, and the functional smoke passes. 0x4a59a0 and 0x443a40 are
    tier S after the existing/local VC5SP3 targets pass zero unmasked byte
    mismatches. 0x4a59b0 is also tier S after the direct renderer-path
    predicate restores BN's `sub eax, 2` source shape and the grouped
    VC5SP3 target passes for 0x4a59b0/0x443a40 with zero unmasked
    mismatches.
  - The renderer dispatch/global owner is accepted for 0x4a77a0, and many
    DirectDraw/video helpers are tier B or tier S-ready for callers. The
    fullscreen surface-builder data pass is accepted; image-surface and later
    teardown/restore/mode-setting data clusters remain pending.
    A follow-up source-file owner/data pass accepted
    0x4a8790 zVideo_dd::SetVideoMode and
    0x4a9b70 zVideo_dd3d::PresentDisplayModeSurface at tier B after current
    functional smoke evidence and zVideo surface-state global review. A
    same-session pass registered and passed the
    `zvideo_dd_present_display_mode_surface` smoke, converted
    0x4a7b60 zVideo_dd::PresentDisplayModeSurface to source-faithful namespace
    ownership, and left the touched surface-state/half-res globals plus
    coherent zVideo DirectDraw source-cluster tier S verification pending. A
    follow-up data pass accepted 0x4a7b60 to tier B after current BN data
    decls confirmed the two half-res flags, the three zVideo surface-state
    BSS records, and g_zVideo_SurfaceStateSwapScratch, and local VC5SP3 data
    target `zvideo_dd_present_display_mode_surface_data` reported zero
    unmasked data-byte mismatches for all six symbols. 0x4a7b60 now remains
    blocked only on coherent DirectDraw present-cluster tier S verification.
    A follow-up function-byte target
    `zvideo_dd_present_display_mode_surface` now covers 0x4a7b60; the source
    shape was adjusted to compute the present flags before null-surface checks,
    use the additive DDBLT_WAIT/DDBLT_ASYNC spelling matching BN's mask/add
    sequence, and reload DirectDraw surface globals around swap/restore paths.
    `verify vc5 0x4a7b60` improves from the initial 351 unmasked mismatches to
    270 unmasked mismatches after 100 relocation-masked bytes (BN 446 bytes,
    VC5 416 bytes), leaving broad DirectDraw branch/control-flow and
    provider-call scheduling drift.
    A follow-up clear-cluster pass registered and passed the missing
    functional smokes for 0x4a81a0 zVideo_dd::ZBuffer_DepthFillRect,
    0x4a8220 zVideo_dd::ClearScreenAndZBufferRect, and 0x4a82f0
    zVideo_dd::ClearSwBackbufferAndZBufferRects, documented the repeated
    Blt/Restore loop as an original inline helper, and accepted their
    namespace source ownership. Their touched clear/Z-buffer globals remain
    data-gate blockers before tier B or coherent clear-cluster tier S work.
    A later data pass accepted 0x4a6b80 zVideo::SetClearColorPacked16 to tier
    B after current BN confirmed the leaf store into zero-initialized
    g_zVideo_ClearColorPacked16 (6321cch), and a follow-up local VC5SP3 target
    `zvideo_set_clear_color_packed16` promotes it to tier S with zero unmasked
    byte mismatches after 4 relocation-masked bytes. It also accepted data for
    0x4a81a0/0x4a8220/0x4a82f0 after BN/source review of
    g_zVideo_pZBufferSurface (6333f4h), g_zVideo_ClearScreenBufferEnabled
    (632130h), g_zVideo_ClearColorPacked16 (6321cch), and
    g_zVideo_SwSurfaceState (632200h); the clear cluster is now tier B and
    remains blocked only on coherent VC5SP3 tier S verification.
    A follow-up clear-cluster VC pass corrected the local `DDBLTFX` source
    shape to BN-visible field-only initialization instead of full provider
    record zeroing and kept the recovered Blt/Restore helper as an inline
    retry-label helper. A later 0x4a81a0 pass inlined the BN-shaped
    Z-buffer Blt/Restore retry loop and moved DDBLTFX size initialization
    before the null-surface branch, improving `verify vc5 0x4a81a0` from
    115 to 102 unmasked mismatches after 12 relocation-masked bytes and
    9 trimmed VC NOPs (BN 114, VC5 144). The local VC5SP3 `/Ob1` clear
    targets still remain below tier S:
    `verify vc5 0x4a8220` fails with 203 after 20 relocation-masked bytes
    (BN 208, VC5 256), and `verify vc5 0x4a82f0` fails with 215 after 24
    relocation-masked bytes (BN 212, VC5 256). Functional smokes for all three
    still pass.
    A later 0x4a81a0 byte-shape pass changed the Z-buffer clear retry loop to
    a VC5-shaped `HRESULT hresult = DD_OK; while (hresult == DD_OK)` spelling,
    reducing `verify vc5 0x4a81a0` from 102 to 10 unmasked mismatches after
    20 relocation-masked bytes and 5 trimmed VC NOPs (BN 114, VC5 112).
    `verify functional 0x4a81a0` still passes; tier S remains blocked by
    branch displacement and success-epilogue drift.
    A fresh
    local VC5SP3 target for 0x4a9b70 previously failed with 108 unmasked
    mismatches after 28 relocation-masked bytes and 6 trimmed VC NOP bytes; a
    display-surface local/retry-label probe worsened to 115 mismatches, and a
    tail-return success epilogue probe was byte-neutral. A display-surface
    local/reload source shape now improves the compare to 94 unmasked
    mismatches after 36 relocation-masked bytes and 10 trimmed VC NOP bytes,
    but remains below tier S because the VC5 symbol grows to 192 bytes and the
    initial retry-loop jump/reload layout plus provider-call scheduling still
    drift. A post-restore no-reload variant regressed to 146 unmasked
    mismatches and was reverted.
    A later mode-setting byte pass promoted 0x4a8720
    zVideo_dd::SetDisplayMode, 0x4a90e0
    zVideo_dd::RestoreDisplaySurfaces, 0x4a91b0
    zVideo_dd::ReleaseAllInterfacesAndSurfaces, and 0x4a9060
    zVideo_dd::VerifyFullscreenSurfaceLocks to tier S with focused VC5SP3
    zero-mismatch targets. The shared zvid_dd.c source-file string is now a
    `const char[]` so VC5 passes the direct string address, and
    ReleaseAllInterfacesAndSurfaces now spells the BN-observed inline COM
    release/page-unlock sequence. 0x4a8790 remains B because 0x4a9c20
    zVideo_dd3d::CreateDeviceState remains tier B. Same-session BN-backed
    source-shape repair removed the absent system-memory z-buffer retry,
    removed the absent AddAttachedSurface z-buffer fallback, changed the
    viewport to use g_zVideo_DisplayModeSurfaceState width/height, recovered
    the direct zvid_ddd3d.c source-file string array, hoisted provider-record
    declarations, matched the second zBufferDesc memset, used DWORD viewport
    dimensions for VC5's unsigned float-conversion path, and matched material
    b/g/r store order. The functional smoke passes with coverage for the
    single CreateSurface attempt, single AddAttachedSurface attempt, and
    display-mode viewport dimensions, but `verify vc5 0x4a9c20` still fails
    with 30 unmasked mismatches after 320 relocation-masked bytes and 9
    trimmed VC NOP bytes, BN size 1223 and VC size 1232. Remaining tier S
    drift is SetRenderState this/vtable-load scheduling plus final xor/pop
    scheduling; declaration-order, zero-sentinel, direct-dereference, and
    scoped-device-pointer source-shape probes that did not improve the compare
    were reverted. A fresh scoped `IDirect3DDevice2 *const` probe preserved
    behavior but regressed the compare to 837 unmasked mismatches; a
    `(*g_zVideo_pD3DDevice).SetRenderState` probe and a final `hresult = DD_OK`
    return spelling were byte-neutral at 30 mismatches. All three were reverted.
    A later official `IDirect3DDevice2_SetRenderState` macro spelling for the
    nine drifted calls, assigning those HRESULT results to the existing local,
    and `return DD_OK` were each byte-neutral at 30 mismatches and reverted.
    A same-session render-state mini-owner pass accepted
    0x4a6b60 zVideo_dd3d::SetPendingWireframeState,
    0x4a6b70 zVideo_dd3d::SetPendingDitherEnable,
    0x4a9ac0 zVideo_dd3d::BeginSceneAndFlushPendingRenderStates, and
    0x4a9b40 zVideo_dd3d::EndScene at tier B. BN confirms
    g_zVideo_PendingWireframeState (632138h) and
    g_zVideo_PendingDitherEnable (63213ch) are int32 BSS globals whose xrefs
    are limited to the setter/init/flush/reset paths; ModuleInit already
    carries accepted data evidence for the dither sentinel initialization.
    The stale local functional target smokes are now registered in
    tests/native/smoke.cpp, and `verify functional` passes for the two
    setters, BeginSceneAndFlushPendingRenderStates, and EndScene.
    Local VC5SP3 target `zvideo_dd3d_render_state_scene` now covers
    0x4a6b60, 0x4a6b70, 0x4a9ac0, and 0x4a9b40. It proves zero-unmasked-byte
    tier S evidence for 0x4a6b60, 0x4a6b70, 0x4a9ac0, and 0x4a9b40 after COFF
    relocation masking; those entries are promoted to tier S. The remaining
    0x4a9ac0 SetRenderState mismatch was closed by using the pending dither
    BSS global directly as the second render-state argument, matching BN's
    register choice without introducing provider shims or raw slot dispatch.
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
    fallbackDesc/fallbackSurface pointer probes, including a register
    fallbackDesc variant, were byte-neutral at the same 7 mismatches and were
    reverted. A later focused pass confirmed
    assignment-in-condition, fallback directDraw local, function-level desc
    pointer, display-surface output-slot alias, and comma-expression
    source-shape probes were also byte-neutral at 7 mismatches; moving the
    fallback caps assignment before Release worsened the compare to 23
    mismatches and was reverted. A temporary VC5 profile sweep found
    vc5_o2_ob0_md_facs and vc5_o2_ob1_md_gx_facs both remain at 7 mismatches,
    while non-/MD profiles do not compile this MFC-dependent source. Same-session follow-up recovered
    0x4a8030 zVideo_dd::UnlockSurfaceState as the direct early-return guard
    shape; `verify vc5 0x4a8030` now passes with zero unmasked mismatches after
    12 relocation-masked bytes and 3 trimmed VC NOP bytes, `verify functional
    0x4a8030` passes, and the entry is tier S. 0x4a8dc0
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
    left 0x4ad6a0 zVideo_dd::ReportError and 0x4a6bf0
    zVideo_PixelPack::SetupFromMasks as visible tier S blockers before
    ReportError was resolved.
    A same-session ReportError source-shape probe split the monolithic HRESULT
    switch into numeric DDERR/D3DERR range switches to try to match the retail
    compare-chain/table boundaries; it worsened the VC5SP3 COFF compare from
    1610 to 2645 unmasked mismatches and grew the VC body from 3360 to 3552
    bytes, so the broad split was reverted. A narrower explicit `case DD_OK`
    repair matches the retail zero-return tail shape, and the local VC5
    manifest now uses `bn_byte_length` 3296 to include the compiler switch
    tables that BN keeps outside the function body. `verify vc5 0x4ad6a0`
    now passes with zero unmasked byte mismatches after 952 relocation-masked
    bytes, no trimmed VC NOPs, BN size 3296, and VC size 3296; the functional
    smoke still passes, and 0x4ad6a0 is tier S.
    A same-session source-order experiment for 0x4a6bf0 that moved the mask
    stores before the shifted-mask temporaries worsened the VC5SP3 COFF compare
    from 109 to 113 unmasked mismatches and increased trimmed VC NOPs from 9
    to 13, so the source body was restored to the prior source-faithful shape.
    A later same-session pass refreshed the same 109-mismatch / 48
    relocation-masked-byte / 9-trimmed-NOP result, confirmed the functional
    smoke still passes, and tried two narrower rBits/direct-global-write
    source-order variants; both were byte-neutral at 109 mismatches, so the
    original source body remains the accepted source-faithful shape for now. A
    delayed redMaskShifted computation plus blueBits-plus-greenBits source
    spelling was also byte-neutral at 109 mismatches and was reverted. A
    MakeShiftedMask inline-helper spelling for all shifted-mask writes preserved
    the functional smoke but stayed byte-neutral at 109 mismatches; the unused
    anonymous helper left behind by that probe was removed, with the functional
    smoke still passing and the VC5SP3 compare unchanged at 109 mismatches.
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
    were reverted. A later pass retained the BN-backed literal 0x03e0
    green-mask argument in the 5-5-5 branch; it is byte-neutral at 161
    mismatches but matches current BN HLIL. The same pass rechecked 0x4a8b20
    after a fallbackDirectDraw/fallbackDesc retry-call source probe; the probe
    was byte-neutral at the same 7 mismatches and was reverted, leaving the
    known fallback CreateSurface3FromDesc edx-load versus push scheduling
    blocker unchanged.
    A follow-up source-shape check confirmed the
    restored baseline at 161 unmasked mismatches; introducing a local
    DDPIXELFORMAT pointer and then introducing branch-invariant red/green/blue
    mask locals were both byte-neutral at the same 161 mismatches, so the
    later probes were reverted. A per-branch red/blue mask local-cache spelling
    also preserved the functional smoke but remained byte-neutral at 161
    mismatches, so that source probe was reverted too.
  - For the zRndr InitGlobals dependency chain, 0x4904d0
    zRndr::SetPerspectiveAdaptiveCorrection is now tier S after simplifying the
    reciprocal guard to the assembly-visible zero compare. The
    same-session explicit guarded `if`/`else` source-shape experiment for the
    reciprocal zero/NaN path was byte-neutral at the same 20 unmasked
    mismatches and was reverted. A later same-session `goto`/out-of-line
    reciprocal branch probe was also byte-neutral at 20 unmasked mismatches;
    functional still passed and the probe was reverted. The final zero-compare
    source shape passes VC5SP3 byte verification with zero unmasked mismatches
    after 28 relocation-masked bytes and 8 trimmed VC NOP bytes. The
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
    unmasked mismatches and `verify functional 0x49b1e0` passing. 0x48fd80
    zRndr::InitGlobals is now tier S after replacing the local FloatFromBits
    helper-call constants with VC5-visible float literals and spelling the
    identical fog color assignments in BN store order; `verify functional
    0x48fd80` passes and `verify vc5 0x48fd80` passes with zero unmasked
    mismatches after 260 relocation-masked bytes and 6 trimmed VC NOP bytes.
  - Image-surface helper source ownership and data gates are accepted for
    0x4a83d0, 0x4a84c0, 0x4a8500, 0x4a8650, 0x4a8680, and 0x4a86f0. These are
    at least tier B after accepting the zVidImagePartial surface/pixel/pitch
    ownership model, DirectDraw provider interfaces, selected-device caps, and
    ReportError source strings. 0x4a8650 is tier S after adding local VC5SP3
    target `zvideo_dd_image_ensure_surface_for_current_device`; `verify vc5
    0x4a8650` passes zero unmasked mismatches after 4 relocation-masked bytes,
    and `verify functional 0x4a8650` passes. 0x4a86f0 is tier S after
    recovering the null-surface return/source-file literal shape and adding
    local VC5SP3 target `zvideo_dd_image_release_surface`; `verify vc5
    0x4a86f0` passes zero unmasked mismatches after 8 relocation-masked bytes
    and 3 trimmed VC NOPs, and `verify functional 0x4a86f0` passes. 0x4a8680
    now has local VC5SP3 target `zvideo_dd_image_upload_pixels_to_surface`;
    matching BN's branchy selected-device caps selection, null lazy-create
    return, and literal zvid_dd.c ReportError argument leaves `verify vc5
    0x4a8680` blocked at 69 unmasked mismatches after 20 relocation-masked
    bytes and 9 trimmed VC NOP bytes, with the functional smoke passing.
    Remaining drift is return-block layout around renderer/lazy-create zero
    returns and GetDC success/error epilogue ordering. A local
    VC5SP3 target also covers 0x4a8500
    `zvideo_dd_image_populate_surface_from_heap_pixels`; `verify functional
    0x4a8500` passes, and explicit C retry labels improved `verify vc5
    zvideo_dd_image_populate_surface_from_heap_pixels` to 287 unmasked
    mismatches after 23 relocation-masked bytes and 11 trimmed VC NOP bytes.
    Remaining drift is DirectDraw Lock/Unlock lost-surface retry call
    scheduling, descriptor dwSize store scheduling, row-copy local scheduling,
    and descriptor/pitch-store ordering. Tier S remains deferred to a coherent
    DirectDraw image/surface source-cluster pass. A full DDSURFACEDESC memset
    plus dwSize store worsened the compare to 292 mismatches, and spelling
    descriptor initialization as a dwFlags-tail memset plus dwSize store was
    byte-neutral at 290 mismatches; both probes were reverted. A later
    lost-surface-first branch spelling preserved behavior but regressed the
    compare to 290 unmasked mismatches after 20 relocation-masked bytes and 7
    trimmed VC NOP bytes, so it was reverted. Explicit Restore-success
    `goto retryLock`/`goto retryUnlock` branches were byte-neutral at the
    287-mismatch baseline and were reverted. A `while (hresult != DD_OK)`
    retry-loop spelling for both Lock and Unlock preserved behavior but regressed
    the compare to 320 unmasked mismatches with a 400-byte VC body, so it was
    reverted.
  - The hardware-device table/selection and renderer-flag cluster is no longer
    a data-owner blocker for 0x4a6b40, 0x4a7990, 0x4a7490, 0x4a8870, 0x4a7b40,
    0x4a93d0, 0x4a95e0, and 0x4a96b0. 0x4a7b40 is tier S after the VC5SP3
    zero-mismatch check. 0x4a96b0 is tier S after matching the Direct3D device
    callback's fatal ReportOld source-file literal and null-GUID-first branch
    shape; local target `zvideo_dd_enum_direct3d_device_callback` passes zero
    unmasked mismatches after 84 relocation-masked bytes and 5 trimmed VC NOPs,
    and the functional smoke passes. 0x4a8800 is now tier S after adding local
    target `zvideo_dd_create_directdraw2_for_selected_device`, dropping the
    temporary DirectDraw pointer zero-initializer, and using literal zvid_dd.c
    ReportError arguments; `verify vc5 0x4a8800` passes with zero unmasked
    mismatches after 32 relocation-masked bytes and 11 trimmed VC NOPs, and
    `verify functional 0x4a8800` passes. 0x4a95e0 is now tier S after adding local
    target `zvideo_dd_enumerate_direct3d_devices_for_record`, recovering the
    retail 0x6c stack scratch with a 0x68-byte zeroed span, literal zvid_dd.c
    ReportError argument, and inline IDirect3D2 Release/nulling shape; `verify
    vc5 0x4a95e0` passes with zero unmasked mismatches after 56
    relocation-masked bytes and 9 trimmed VC NOPs, and `verify functional
    0x4a95e0` passes. 0x4a93d0 is now tier S after adding local target
    `zvideo_dd_enum_directdraw_device_callback`, matching the retail early row
    pointer computation before logging/capacity, post-increment ordinal shape,
    null-GUID fallthrough branch, literal zvid_dd.c ReportError argument, and
    reused memory-caps local; `verify vc5 0x4a93d0` passes with zero unmasked
    mismatches after 120 relocation-masked bytes and 5 trimmed VC NOPs, and
    `verify functional 0x4a93d0` passes.
    DirectDraw/Win32 pointees remain provider-owned, but the
    zVideo COM handle globals, selected-device table/pointers, option strings,
    source-file string, primary-backbuffer flag, and surface-state records are
    authored data for remaining surface/fullscreen callers.
  - D3D-state consumers 0x4a9c20, 0x4aa9e0, and 0x4accc0 are now tier B after
    accepting the typed DirectDraw/Direct3D provider pointers, z-buffer attach
    state, selected-device pointer, viewport/material handles, caps records,
    render-state/fog caches, wireframe state, and quad-batch BSS data.
    0x4aa9e0 zVideo_dd3d::SetFogEnable is now tier S after adding local target
    `zvideo_dd3d_set_fog_enable`; `verify vc5 0x4aa9e0` passes with zero
    unmasked mismatches after 24 relocation-masked bytes and 12 trimmed VC
    NOPs, and `verify functional 0x4aa9e0` passes. 0x4accc0
    zVideo_dd3d::SetQuadBatchDepthAndRhw is now tier S after restoring the
    BN-observed per-item bottom-left, bottom-right, top-right, top-left store
    order for z and rhw while staying on typed TL vertex fields; `verify vc5
    0x4accc0` passes with zero unmasked mismatches after 8 relocation-masked
    bytes and 15 trimmed VC NOPs, and `verify functional 0x4accc0` passes.
    Current status shows 0x4ad6a0 zVideo_dd::ReportError is now tier S, so
    0x4a9c20's remaining blocker is its own SetRenderState provider-call
    scheduling and final xor/pop byte drift.
    0x4ad680 is tier S/no-globals after adding local target
    `zvideo_dd3d_floor_power_of_two`; `verify vc5 0x4ad680` passes zero
    unmasked mismatches with no relocation-masked bytes and 13 trimmed VC NOPs,
    and `verify functional 0x4ad680` passes. The zvid_ddd3d.c quad/flush slice
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
    0x4aa9d0 and 0x4aa900 are tier S/no-globals after local VC5 target
    `zvideo_dd3d_texture_record_lifecycle` passed zero unmasked mismatches
    for both helpers, with 4 relocation-masked bytes and 2 trimmed VC NOPs for
    0x4aa9d0 and no relocation-masked bytes plus 9 trimmed VC NOPs for
    0x4aa900; both focused functional targets pass. 0x4aa980 is tier S after
    matching BN's field-hoisting source shape; the same VC5 target passes with
    zero unmasked mismatches, 8 relocation-masked bytes, and 13 trimmed VC
    NOPs, and its focused functional target passes. The lifecycle target also
    byte-matches 0x4aa8b0 and 0x4aa8f0. 0x4aa8b0 now matches BN's callee-owned
    DDSURFACEDESC initialization and success-fallthrough branch shape, but its
    marker stays tier B because direct callee 0x4a8100
    zVideo_dd::LockSurface_WaitRestore remains tier B; 0x4aa8f0 stays tier B
    because direct callee 0x4a8160 zVideo_dd::UnlockSurface_WaitRestore remains
    tier B. A same-session 0x4a8160 pass confirmed behavior but found that a
    do-while shared-tail spelling only improved 46 to 45 VC5 mismatches and an
    explicit retry-label spelling stayed at 46; both loop-shape probes were
    reverted because VC5 still duplicated the Unlock retry block. 0x4aa6f0 has local target
    `zvideo_dd3d_convert_image_pixels_for_texture`; recovering field-based
    width/height loop bounds improved the VC5SP3 compare, but it still fails
    with 362 unmasked byte mismatches, 20 relocation-masked bytes, and 5
    trimmed VC NOPs. 0x4aa920 remains tier B because direct callee 0x4aa600
    remains tier B. Remaining texture-record work is coherent tier S
    verification/source-shape repair, not a data blocker.
  - The zVideo init/shutdown source-file cluster was active because
    RecoilApp::InitializeDisplay (0x42e330) depends on 0x4a75f0 and the
    frontier ranked that owner before caller data propagation. Scope:
    0x4a7520, 0x4a7530, 0x4a75e0, 0x4a75f0, 0x4a7700, 0x4a7740, and 0x4a7af0.
    Current evidence accepted 0x4a7700, 0x4a7740, and 0x4a7af0 to
    tier B after registering the stale native smokes and documenting the
    recovered zImage default-texture helper. 0x4a75e0 is tier S/no-globals:
    `verify vc5 0x4a75e0` passes zero unmasked mismatches with no
    relocation-masked bytes and 13 trimmed VC NOPs, and `verify functional
    0x4a75e0` passes. 0x4a7520 is now tier S/no-globals after adding the
    local VC5SP3 target `zvideo_at_exit_release_all_interfaces_and_surfaces`;
    `verify vc5 0x4a7520` passes zero unmasked mismatches after 4
    relocation-masked bytes and 11 trimmed VC NOPs, and `verify functional
    0x4a7520` passes. 0x4a75f0 is tier B after
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
    0x490780 zRndr::SpanOcclusionShutdown is now tier S after changing the
    helper to return int zero, matching BN's xor eax epilogue while the sole
    current caller ignores the return value; `verify functional 0x490780`
    passes and `verify vc5 0x490780` reports zero unmasked mismatches after
    20 relocation-masked bytes and 3 trimmed VC NOP bytes.
    A later span-occlusion insertion pass expanded 0x490ae0
    `zRndr_SpanOcclusion_InsertSpanNode_Local` from a collapsed forwarding
    wrapper into the BN-visible insertion body. `verify functional 0x490ae0`
    still passes and `verify vc5 0x490ae0` improves from 1967 to 1822
    unmasked mismatches after 124 relocation-masked bytes and 14 trimmed VC
    NOPs (BN 1971, VC5 832). A local ignored VC5 target now covers 0x4907c0
    `zRndr_SpanOcclusion_TestSpanDepthOrderPair`; it fails with 726 unmasked
    mismatches after 60 relocation-masked bytes and 11 trimmed VC NOPs (BN
    793, VC5 528), leaving dense x87 depth-order source-shape drift as the
    direct blocker for the insertion body.
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
    A follow-up source-shape pass kept the active texture and destination-end
    loads case-local inside the texVShift 10..17 switch cases, matching BN's
    post-dispatch global-load shape more closely; functional still passes and
    VC5SP3 COFF drift improves from 834 to 784 unmasked mismatches after 196
    relocation-masked bytes and 8 trimmed VC NOPs, but the function remains
    blocked by the retail ESP-pivot push/sub-esp write loop. A later
    negative-byte-countdown spelling for the per-case loop counter preserves
    behavior and improves the current compare to 779 unmasked mismatches after
    196 relocation-masked bytes and 4 trimmed VC NOPs; the remaining blocker is
    still the retail ESP-pivot push/sub-esp write loop.
    The sibling 0x49e6c0 target now carries the same seven data-symbol checks
    with zero unmasked mismatches; the same case-local load spelling improves
    function-byte drift from 698 to 640 unmasked mismatches after 196
    relocation-masked bytes and no trimmed VC NOPs. A positive-count do-while
    probe regressed to 641 mismatches and was reverted; a later negative-byte
    countdown probe regressed to 645 mismatches with 12 trimmed VC NOPs and was
    also reverted. The remaining blocker is still ordinary C++ descending
    stores versus the retail ESP-pivot push-write loop.
    Pal8 ESP-pivot siblings now carry matching active-data evidence:
    0x49edc0 and 0x49bbf0 verify gRndr_ActiveTexVMask,
    gRndr_ActiveTexUMask, gRndr_ActiveTexPixels, gRndr_ActiveTexPalette,
    gRndr_ActiveTexUStepFixed20, gRndr_ActiveTexVStepFixed20, and
    gRndr_CurrentSpanBaseAddr with zero unmasked data-byte mismatches. A
    0x49edc0 source-shape pass removed the out-of-line
    SpanPal8SampleExpanded call from the switch cases, inlining the pal8 byte
    sample plus palette expansion and keeping the destination end case-local;
    functional still passes and VC5SP3 COFF drift improves from 779 to 680
    unmasked mismatches after 228 relocation-masked bytes and 8 trimmed VC
    NOPs. A later BN-shaped negative-byte-countdown loop spelling preserves
    behavior, keeps the same zero-mismatch data-symbol evidence, and improves
    0x49edc0 again to 676 unmasked mismatches after 228 relocation-masked
    bytes and 4 trimmed VC NOPs. A case-local texture/palette/V-mask local
    probe regressed to 895 mismatches and was reverted. A 0x49bbf0
    source-shape pass inlined the pal8 source-index expression inside each
    switch case and kept the destination-end load case-local; functional still
    passes and VC5SP3 COFF drift improves from 819 to 782 unmasked mismatches
    after 228 relocation-masked bytes and 8 trimmed VC NOPs. A positive-count do-while
    probe regressed to 790 mismatches and a negative-byte-countdown probe
    regressed to 792 mismatches with 4 trimmed VC NOPs, so both were reverted.
    0x49bbf0 remains
    blocked because the source is ordinary C++ descending stores rather than
    the retail ESP-pivot push/sub-esp loop. The shared gRndr_SavedEspSlot BSS
    pointer remains covered by the tex16 sibling data targets.
    The remaining minimum ESP-pivot leaves now have matching local data
    evidence as well: 0x4997d0 verifies gRndr_CurrentSpanBaseAddr with zero
    unmasked data-byte mismatches, while 0x49f180 verifies the seven pal8/span
    globals plus the split zrndr_span_shade_globals target for
    gRndr_ActiveShadeFixed16 and gRndr_ActiveShadeStepFixed16 with zero
    unmasked data-byte mismatches. A 0x4997d0 source-shape pass retained the
    unmasked packed-color value and unsigned pair-count spelling, improving the
    opaque fill leaf from 63 to 54 unmasked VC5SP3 mismatches after 4
    relocation-masked bytes and 4 trimmed VC NOPs, with functional still
    passing. A bit-2/quad-count spelling regressed to 55 mismatches and an
    explicit stack-local spelling was byte-neutral at 54, so both probes were
    rejected. A 0x49f180 source-shape pass inlined the per-case pal8
    source-index expression and kept the destination-end load case-local;
    functional still passes and VC5SP3 COFF drift improves from 829 to 795
    unmasked mismatches after 324 relocation-masked bytes. Unsigned texV
    mask-shift and shade-bucket mask-shift probes were byte-neutral at 795
    mismatches and were reverted. These leaves remain owner/data blocked
    because the retail routines use ESP/push reverse writes until the
    source-faithful ESP-pivot span-family model is recovered.
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
    0x48d7a0 pass, but `verify vc5 zrndr_overlay_blend_rows` still fails all
    four row callbacks. Current 0x48d450 scalar baseline is 45 unmasked
    mismatches after 12 relocation-masked bytes and 12 trimmed VC NOPs; local
    source-shape probes that split BN's low/high lane expression order or
    computed the high-lane expression first were byte-neutral at 45, while a
    BN-described `uint16_t *` pointer/pair-store spelling regressed to 49
    unmasked mismatches and an explicit low/high variable copy spelling
    regressed to 72. A later staged low/high scaled-lane local probe was also
    byte-neutral at the same 45 mismatches and was reverted. A 0x48d4b0
    rightDelta pre-shift/pairCount copy spelling preserved behavior but was
    byte-neutral at 67 unmasked mismatches and was reverted; BN still shifts
    EDX before copying the pair count while VC5 keeps the shifted local in ESI.
    0x48d510/0x48d5f0 still report 207 unmasked mismatches
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
    gap instead of authored span/MMX state. Local VC5SP3 data-symbol targets
    now cover the 0x48ff80 selector's pixel-pack/graphics input globals and the
    selector-installed gRndr_pfn callback bank, all with zero unmasked data-byte
    mismatches; the plan data markers still remain blocked because Source owner
    acceptance is coupled to the unresolved ESP-pivot and MMX span source-shape
    model. Continue the shared zRndr_Span.cpp callback/global data owner audit
    for remaining zRndr span-family owner acceptance, including callback-family
    source shape, before returning to 0x48ff70 or 0x42e330. The fog/MMX pair
    at 0x49e400 and 0x49e560 now spells the scalar edge handling, aligned
    quad loop, and scalar tail directly in each function after removing the
    shared `FogBlendSpanMmxCore` helper; both functional targets still pass,
    and VC5SP3 `vc5_o2_ob0_md_facs` improves from a 32-byte wrapper with 332
    unmasked mismatches to a 128-byte body with 324 unmasked mismatches, 12
    relocation-masked bytes, and 11 trimmed VC NOPs. These entries remain
    source-owner/data blocked because the source still scalar-emulates the
    retail inline MMX quad body. The fixed-vshift tex16 copy helper used by 0x49ea80 and
    0x49ec20 now uses an aligned packed two-pixel store after the scalar
    prologue, and both functional targets still pass; those entries remain
    owner/data blocked because retail still uses the MMX packed-index loop over
    gRndr_Mmx_* scratch records. Same-session BN HLIL for 0x49ea80 and
    0x49ec20 confirms the optional unaligned leading texel, paired U/V scratch
    setup, two-texel MMX index sampling, packed destination write, and odd-tail
    path now recorded in source comments. A follow-up source-shape pass replaced
    the scalar leading-edge `SpanTex16Sample` calls with the BN-observed inline
    source-index expression in both tex16 copy siblings; functional still
    passes, and VC5SP3 COFF drift improves from 344 to 342 unmasked mismatches
    for 0x49ea80 and from 349 to 347 for 0x49ec20 after 68 relocation-masked
    bytes and 6 trimmed VC NOPs each. A later source-shape pass removed the
    scalar SpanCopy16FromTex16Forward helper and inlined the packed two-pixel
    scalar loop into both retail functions; functional still passes, and
    VC5SP3 COFF drift improves further to 308 unmasked mismatches for 0x49ea80
    and 312 for 0x49ec20 after 96 relocation-masked bytes and 9 trimmed VC NOPs
    each. Same-session parent BN cleanup applied explicit fastcall prototypes
    for 0x49ea80, 0x49ec20, 0x49e400, and 0x49e560, refreshed analysis, and
    saved the database; decompilation still carries local stack artifacts, but
    call cleanup/prototype evidence is now explicit. The remaining blocker is
    still the retail MMX packed-index loop and packed destination write, not
    behavior coverage.
    A same-session source-worker probe for 0x49b7e0 left source unchanged:
    functional still passes, `verify vc5 0x49b7e0` remains at 779 unmasked
    mismatches after 196 relocation-masked bytes and 4 trimmed VC NOPs, and no
    safe VC5-era C++ spelling was found for the retail ESP-pivot
    push-word/sub-esp write loop without forbidden raw assembly, naked code, or
    ESP-manipulation scaffolding.
    The scalar alpha-map callback slice
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
    A later 0x49c020 source-shape pass restored BN's do-style loop, cached
    active texture/palette/alpha globals, unsigned V-shift, and unsigned alpha
    gates; `verify vc5 0x49c020` improves from 243 to 110 unmasked mismatches
    after 40 relocation-masked bytes, with functional still passing. The
    remaining blocker is partial-alpha channel-math register ordering and
    loop-tail scheduling drift, so 0x49c020 stays tier B. A same-session
    green-contribution precompute probe preserved behavior but regressed the
    compare from 110 to 117 unmasked mismatches, so it was reverted.
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
    while owner/data remain pending for the shared ESP-pivot family. Focused
    status/frontier refresh finds no lower authored callee blocker for
    0x49f180, the functional target still passes, and the current VC5SP3 check
    still fails with 795 unmasked bytes plus 324 relocation-masked bytes after
    the per-case source-index/destination-end source-shape improvement, so
    the blocker remains source-owner shape rather than behavior coverage. A
    scaffold audit found the old unused
    unavailable-callback fallback helpers had no source/test/doc references and
    falsely claimed selector installation; those no-op helpers were removed
    from production source instead of preserving them as source-shape debt.
    A source-owner mapping pass for 0x49b7e0 confirms it must not be accepted
    alone: 0x4997d0, 0x49b7e0, 0x49bbf0, 0x49e6c0, 0x49edc0, and 0x49f180
    remain tier C/data-equivalent-only because current BN assembly proves real
    ESP-pivot or push-write framebuffer stores, and production raw assembly,
    naked helpers, and ABI scaffolds remain forbidden. Do not loop on isolated
    ESP-pivot leaves for source-owner promotion. Current read-only BN mapping
    reconfirmed 0x49b7e0 saves real ESP through `gRndr_SavedEspSlot`, pivots
    ESP to the active span end, and emits word `push` stores or `sub esp, 2`
    skips in each texVShift case. Follow-up BN cleanup corrected the local
    `RndrSpanLenShiftFn` typedef from a two-argument pointer to the
    four-argument `__fastcall` texU/texV/pixelCount/texVShift span callback ABI
    used by the selector-installed dispatch bank, refreshed the selector,
    switch-vshift targets, and consumers, and saved `Recoil.bndb`; the source
    owner blocker remains the retail ESP-pivot source-family shape. The MMX
    scratch/mask setup
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
    A same-session BN/source-owner refresh for the ESP-pivot family
    reconfirmed 0x4997d0, 0x49b7e0, 0x49bbf0, 0x49e6c0, 0x49edc0, and
    0x49f180 as real stack-pivot/push-write span callbacks rather than normal
    C++ descending pointer stores; no local original-source macro or handwritten
    span evidence was found, and non-CPU-probe raw assembly/scaffolding remains
    forbidden. Work therefore shifted to independent zRndr_Draw.cpp namespace
    owner leaves: 0x490430 and 0x4904a0 are now tier S/source-faithful after
    same-session BN leaf review, passing functional targets, and the local
    `zrndr_perspective_texture_setter_data` VC5SP3 data-symbol target, which
    reports zero unmasked data-byte mismatches for the touched perspective
    texture globals and gRndr_BytesPerPixel. 0x4904a0 is now tier S after
    changing the zero comparison to a double literal so VC5SP3 emits BN's
    `fcomp` against the shared qword zero; `verify functional 0x4904a0`
    passes and `verify vc5 0x4904a0` reports zero unmasked mismatches after
    12 relocation-masked bytes and 12 trimmed VC NOP bytes. 0x490430 is now
    tier S after reshaping the source to emit BN's pre-loop shift sentinel
    store and post-float-conversion byte-stride store; `verify functional
    0x490430` passes and `verify vc5 0x490430` reports zero unmasked
    mismatches after 32 relocation-masked bytes and 2 trimmed VC NOP bytes.
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
    A same-session 0x492000 source-shape pass removed non-BN edge-table
    zeroing and widened the local visible-span pointer scratch to match BN's
    0x504-byte stack region. Functional still passes and `verify vc5 0x492000`
    improves slightly to 1867 unmasked mismatches after 104 relocation-masked
    bytes, BN size 1990, VC5 size 1008, and 3 trimmed VC NOPs. The remaining
    blocker is still the in-function duplicated edge-builder/codegen shape;
    BN confirms four physical edge-builder loops and no direct helper calls.
    Same-session 0x492f00 zRndr_DrawFlatImmediate owner/data pass replaced the
    behavior-only intersection/sort body with the recovered zRndr_Draw.cpp
    fixed-point ScanConvertEdge table model: scan-convert-mode side selection,
    per-scanline edge cursor advance, gRndr_SpanAllocCursor staging,
    gRndr_pfnBuildSpanListSecondary clipping, gRndr_CurrentSpanBaseAddr update,
    and gRndr_pfnFlatImmediateSpanOp dispatch. `verify functional 0x492f00`
    passes. Local VC5 data-symbol checks `zrndr_draw_flat_immediate_data` and
    `zrndr_draw_flat_immediate_data_b` pass with zero unmasked bytes for
    g_frameBuffer (632050h), g_pitchBytes (63205ch), g_bytesPerPixel (632060h),
    g_scanConvertMode (57dac8h), g_inverseDepthBias (57dac0h),
    g_inverseDepthScale (57dac4h), g_spanAllocCursor (57dae0h),
    g_pfnBuildSpanListSecondary (6320a8h), g_pfnFlatImmediateSpanOp (6320b4h),
    and g_spanCurrentSpanBaseAddr (56b270h). The plan now records source-file
    owner zRndr_Draw.cpp, `Model: source-faithful`, Data reimplemented, and
    tier B. Tier S remains blocked by `verify vc5 0x492f00` with 1875
    unmasked function-byte mismatches after 104 relocation-masked bytes and
    3 trimmed VC NOP bytes (BN 1995 bytes, VC5 1088 bytes), consistent with the
    broader zRndr_Draw.cpp scan-conversion helper inlining/codegen gap.
    Same-session 0x4936d0 zRndr_RasterizePoly owner/data pass removed the
    unsupported defensive entry guard and kept the BN-backed zRndr_Draw.cpp
    scan-conversion shape: duplicate vertex reduction, two 64-entry
    ScanConvertEdge tables, scan-convert-mode side selection, framebuffer row
    stepping, gRndr_CurrentSpanBaseAddr update, and gRndr_pfnSelectedSpanOp
    dispatch. `verify functional 0x4936d0` passes. Local VC5 data-symbol target
    `zrndr_rasterize_poly_data` passes with zero unmasked bytes for
    g_frameBuffer (632050h), g_pitchBytes (63205ch), g_bytesPerPixel (632060h),
    g_scanConvertMode (57dac8h), g_pfnSelectedSpanOp (6320b0h), and
    g_spanCurrentSpanBaseAddr (56b270h). The plan now records source-file owner
    zRndr_Draw.cpp, `Model: source-faithful`, Data reimplemented, and tier B.
    Tier S remains blocked by `verify vc5 0x4936d0` with 1750 unmasked
    function-byte mismatches after 48 relocation-masked bytes and 4 trimmed VC
    NOP bytes (BN 1822 bytes, VC5 752 bytes), consistent with the same broader
    zRndr_Draw.cpp scan-conversion helper inlining/codegen gap.
    Same-session dependency pass for queued draw mip selection promoted
    0x46e290 zImage_TexDirEntryPartial::GetVariantImageAtIndex to tier S after
    adding the required member docblock, accepting the zImage_TexDirEntryPartial
    owner, and verifying default-image data with local VC5 target
    `zimage_default_image_data`; `verify vc5 0x46e290` reports zero unmasked
    function-byte mismatches after 4 relocation-masked bytes and 13 trimmed
    VC NOP bytes. With that callee accepted, 0x499130
    zRndr_TextureMip_SelectVariantImage is now tier B/source-faithful under the
    zRndr_Draw.cpp owner: `verify functional 0x499130` passes, local data target
    `zrndr_texture_mip_select_variant_data` passes for
    gRndr_TextureMipSelectionEnabled (63209ch), and `verify vc5 0x499130`
    remains the tier S blocker after selected-vertex loop and local mip-delta
    scratch source-shape passes improve the compare from 272 to 239 unmasked
    mismatches after 24 relocation-masked bytes and 15 trimmed VC NOP bytes
    (BN 372 bytes, VC5 384 bytes). Remaining debt is post-loop x87
    mip-metric expression scheduling/reduction versus BN's stack-slot reuse.
    Same-session invZ/source-order and selected-loop spelling
    probes preserved behavior but stayed byte-neutral at 272 mismatches, so both
    were reverted.
    Same-session immediate-line callback-bank pass typed
    gRndr_pfnImmediateRaster4, gRndr_pfnImmediateRasterReserved, and
    gRndr_pfnImmediateRaster5 as distinct zRndr_Draw.cpp raster callbacks
    instead of generic span callbacks. BN confirms 0x498bd0 dispatches through
    gRndr_pfnImmediateRaster4 with gRndr_pFrameBuffer and scalar endpoints, and
    0x498c00 dispatches adjacent clipped line-strip segments through
    gRndr_pfnImmediateRaster5. Functional smokes pass for both wrappers, and
    current VC5SP3 data-symbol checks pass with zero unmasked bytes for
    gRndr_pFrameBuffer plus the three immediate raster callback globals, so
    0x498bd0 and 0x498c00 are now source-file zRndr_Draw.cpp owner/data
    accepted at tier B. Their remaining debt is coherent immediate-line
    dispatch function-byte VC5 coverage before tier S.
    Same-session lens-flare queue/visible-list owner pass added source evidence
    comments and offset guards for zRndr_LensFlareSource,
    zRndr_LensFlareVisibleSampleDef, and LensFlareSamplePartial plus the
    lens-flare frame-state globals. BN confirms the zero-initialized queue
    count at 62ea00h, the 0x28a-entry sample queue at 62ea04h, visible count at
    631ccch, visible pointer list at 631cd0h, visibility-active flag at
    56b248h, and four stage texture pointers at 56b250h. Functional targets
    pass for 0x49a830, 0x49a8b0, 0x49a920, 0x49a9c0, 0x49aa40, 0x49afb0, and
    0x49b1a0, so these entries are now source-file owner/data accepted under
    src/GameZRecoil/zRndr/zRndr.cpp; 0x49a8b0 is tier S after VC5 COFF byte
    verification reports zero unmasked mismatches. The same pass accepted
    0x49aa30 data and tier S after functional and VC5 COFF byte verification.
    Follow-up clipped-framebuffer owner/data pass accepted 0x498cb0 and
    0x49a8c0 at tier B after BN confirmed the active framebuffer bank
    (632050h/632054h/632058h/63205ch), pixel-pack selector (57de38h),
    overlay blend state (62e9dch/62e9f0h/62e9f8h), and lens sample
    count/queue (62ea00h/62ea04h); functional smokes pass and no VC5 COFF
    byte target exists yet. Broader stage-submit entries 0x49aa90 and
    0x49b020 intentionally remain below tier B until their
    provider-dispatch/active-region data gates are audited.
    Same-session 0x493df0 zRndr_DrawFlatQueued source-shape pass removed the
    behavior-only intersection/sort scanline body and unsupported fallback
    span-builder guards, added the BN-backed zRndr_Draw.cpp source-file
    docblock line, and now uses the fixed ScanConvertEdge table model with
    direct gRndr_pfnBuildSpanListSecondary dispatch. `verify functional
    0x493df0` passes. Local data targets
    `zrndr_draw_flat_queued_active_texture_data` and
    `zrndr_draw_flat_queued_active_texture_data_tail` pass with zero unmasked
    data-byte mismatches for the active texture globals
    gRndr_ActiveTexPixels, gRndr_ActiveTexPalette,
    gRndr_ActiveTexUStepFixed20, gRndr_ActiveTexVStepFixed20,
    gRndr_CurrentSpanBaseAddr, gRndr_ActiveTexAlphaMap,
    gRndr_ActiveTexShift, gRndr_ActiveTexVMask, and
    gRndr_ActiveTexUMask. Same-session refresh of the shared raster and flat
    queued callback data targets accepted the remaining touched globals, so
    0x493df0 is now tier B/source-faithful under the zRndr_Draw.cpp owner.
    `verify vc5 0x493df0` still fails with 3111 unmasked function-byte
    mismatches after 160 relocation-masked bytes and 13 trimmed VC NOP bytes
    (BN 3314 bytes, VC5 1376 bytes), leaving only coherent zRndr_Draw.cpp
    scan-conversion/x87/codegen tier S debt for this entry.
    Same-session 0x4927d0 zRndr::SpanOcclusionRasterizeOccluderPoly source-
    shape pass retained an explicit source/destination cursor spelling for the
    duplicate-filter reduced-vertex loop. This matches BN's polygon cursor and
    reduced scratch cursor shape better than indexed access, preserves the
    functional smoke, and improves `verify vc5 0x4927d0` from 1723 to 1675
    unmasked mismatches with 204 relocation-masked bytes, no trimmed VC NOPs,
    BN size 1828, and VC5 size 2000. An explicit lastReducedIndex local
    regressed to 1742 mismatches, a delayed reducedCount declaration regressed
    to 1725, and y-before-x cursor assignment plus explicit sourcePoly
    pointer-local spellings were byte-neutral at 1675; those probes were
    reverted. Remaining tier S drift is reduced-vertex register/stack shape
    plus the duplicated edge-builder loop codegen.
    Same-session 0x4904d0 zRndr::SetPerspectiveAdaptiveCorrection tier-S pass
    simplified the reciprocal guard to the assembly-visible zero compare. The
    previous explicit NaN guard compiled to the opposite fallthrough branch
    layout; the zero-compare source matches BN's fcom/fnstsw C3 path, preserves
    the functional smoke, and `verify vc5 0x4904d0` now passes with zero
    unmasked mismatches after 28 relocation-masked bytes and 8 trimmed VC NOP
    bytes (BN size 72, VC5 size 80).
    Same-session 0x4992d0 zRndr_DrawLine16 tier-S probe kept behavior passing
    but did not improve the VC5SP3 baseline: removing the packed-color local
    and explicit basePixels saved-pointer spellings were byte-neutral at 189
    unmasked mismatches, uninitialized xStep spelling regressed to 192
    mismatches, and a byte-cursor/explicit byte-step spelling regressed to 198
    unmasked mismatches with 11 trimmed VC NOP bytes, so these probes were
    reverted. The remaining blocker is still BN's saved fastcall dstPixels
    stack local and branch-local Bresenham cursor/register shape.
    Same-session 0x4993a0 zRndr_DrawLine16_Segmented source-shape pass moved
    drawSegment initialization before dy/dx normalization and gave each major
    axis branch its own cursor lifetime, matching BN's delayed cursor setup.
    `verify vc5 0x4993a0` improved from 323 to 310 unmasked mismatches. Later
    post-increment segment-counter and segmentCount-as-limit spellings improve
    the current compare to 304 unmasked mismatches with 4 relocation-masked
    bytes, 2 trimmed VC NOP bytes, BN size 342, and VC size 352; `verify
    functional 0x4993a0` still passes. A branch-local segmentCounter probe
    regressed to 323 unmasked mismatches, and consumed-y0 loop-counter reuse
    was byte-neutral at 304, so both were reverted. A segmented-only saved
    dstPixels/basePixels local also stayed byte-neutral at 304 and was
    reverted. The remaining drift is register/stack-shape across the segmented
    Bresenham loops and toggle counter.
    Same-session 0x4992d0 zRndr_DrawLine16 startIndex-before-rowStep
    initialization-order probe kept the functional smoke passing but regressed
    `verify vc5 0x4992d0` from the restored 189-mismatch baseline to 193
    unmasked mismatches with VC size 208 and 11 trimmed VC NOP bytes, so it
    was reverted.
    Same-session 0x499810 zRndr_FillSpan555Solid source-shape pass retained the
    565-style red-adjusted staging because it preserves behavior and improves
    `verify vc5 0x499810` from 132 to 104 unmasked mismatches, with 4
    relocation-masked bytes, 12 trimmed VC NOPs, BN size 133, and VC size 144.
    Follow-up matched BN's blue-delta source bits to the original destination
    word, improving the earlier compare before a same-session refresh now
    reports 84 unmasked mismatches with 4 relocation-masked bytes, 9 trimmed
    VC NOPs, BN size 133, and VC size 144.
    A final compound-add writeback probe regressed to 144 unmasked mismatches,
    a delayed raw-channel-delta spelling regressed to 132, and same-session
    red/green calculation-order plus final-expression-order probes preserved
    behavior but stayed at or worsened the current 84-mismatch state, so those
    probes were reverted. A later minimal packed-delta compound-add probe also
    preserved behavior but regressed to 130 unmasked mismatches by spilling the
    cursor to a stack local, so it was reverted. The refreshed 0x4998a0
    zRndr_FillSpan565Solid compare now reports 93 unmasked mismatches with 4
    relocation-masked bytes, 12 trimmed VC NOPs, BN size 134, and VC size 144;
    a final-expression-order probe preserved behavior but stayed byte-neutral
    at the 93-mismatch state and was reverted. The remaining drift is
    cursor/register allocation and
    channel-order/partial-register shape in the solid span loops. Current
    same-session refresh reports 0x499810 at 92 unmasked mismatches and
    0x4998a0 at 87 unmasked mismatches; a 0x4998a0 green-before-red
    contribution-order probe preserved behavior but regressed the compare to 89
    mismatches, so it was reverted.
    The stale circle-helper VC5 compile blocker is resolved: the grouped
    `zrndr_draw_circle_helpers` target now compiles and passes zero-unmasked-byte
    COFF comparison for 0x498fb0 and 0x499020. Functional smokes for
    `zrndr_draw_circle_outline16_framebuffer` and
    `zrndr_draw_circle_octants16_framebuffer` pass, and both entries are now
    tier S.
    Same-session 0x499500 zRndr_DrawLine16_Clipped tier-S pass did not retain
    a source edit: the VC5SP3 compare remains at 649 unmasked mismatches with
    40 relocation-masked bytes and 13 trimmed VC NOP bytes (BN size 717, VC5
    size 736), while `verify functional 0x499500` passes. A branch-local
    raster cursor probe regressed to 666 unmasked mismatches; a local
    clipRect pointer and explicit outcode1 bitmask spelling were byte-neutral,
    so all three probes were reverted. The remaining drift is broad control-
    flow/register allocation and x87 clipping source-shape across the
    Cohen-Sutherland clipping and final Bresenham rasterization.
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
  - The RecoilApp lifecycle owner cluster is accepted at tier S for 0x42de60,
    0x42dfa0, 0x42df10, 0x42df50, and 0x42e070. The FMV scalar deleting
    destructors are provider-boundary glue, and the g_RecoilApp singleton data
    gate is accepted.
  - zFMV, zSound, and zInput dependency cleanup has closed the owner/data
    blockers needed by the original RecoilApp EH pass. Historical EH/source-
    shape details are captured in `docs/reconstruction/recoil_app_destructor_tier_s.md`.
  - 2026-06-15: `0x4626b0` `zFMV_Script::LoadActionsFromZrd` is promoted to
    tier S after a typed loop/argument-scheduling source pass in
    `src/GameZRecoil/zFMV/fmv_script.cpp`; VC5 byte verification reports zero
    unmasked mismatches after 244 relocation-masked bytes, and the functional
    smoke plus docblock audit pass.
  - Remaining active value is caller-path tier S verification only; no broad
    RecoilApp owner/data cleanup is currently pending in this group.
- Next action:
  - Work the remaining tier B verification blockers only after refreshing
    focused status/frontier: 0x4a6cf0 zVid_PackColorRGB, 0x4a0fb0
    zSndSampleSet::LoadSamplesFromIndexArchive, 0x4a0c40 zSndSampleSet::Init,
    0x4704f0 zInput::Mouse_ApplyAccumulatedDelta, 0x46f690
    zInput::Keyboard_PollState, and 0x49fff0
    zSndPlayHandleSnapshot::CreateFromActiveSamples.
  - Delegate source-worker tasks from this group only when the refreshed
    frontier proves non-overlapping source files and owner scopes.

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
    0x40d0b0, and 0x40d0e0 to tier B/source-faithful. Follow-up VC5SP3
    evidence recovered 0x40d0b0 with the constructor body clearing `m_panel`
    after the vtable write, matching the retail vtable-first store order with
    zero unmasked byte mismatches; 0x40d0b0 is now tier S/data `❎`. Follow-up
    VC5SP3 evidence recovered 0x40d150 as an ordinary `new HudOptionsDialog`
    expression that stores through `m_panel`, matching the retail EH-frame
    allocation/enable path with zero unmasked byte mismatches; 0x40d150 is now
    tier S/data `❎`. Follow-up RecoilApp_IState source-shape evidence made the
    empty base destructor header-visible in `RecoilApp.h`, letting VC5 inline
    the base-vftable restore into `hud.cpp`; the local 0x40d0e0
    destructor-core VC5SP3 target now compares
    `??1HudUiOptionsPanelOverlayOwner@@UAE@XZ` with zero unmasked byte
    mismatches, and 0x40d0e0 is tier S/data `❎`. Follow-up same-session queue
    evidence repaired
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
    gates, `Model: source-faithful`, and VC5SP3 tier S byte evidence. 0x41c4e0
    HudUiNewGamePanel::SyncIntensityFromDifficulty is now tier S/data `✅`
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
    0x41c560 is tier S/data `❎`. Follow-up source-shape cleanup changed
    OnTryBecomeCurrent to call through the stored `m_panel`, matching the retail
    EH-frame/register shape and clearing the former VC5SP3 byte mismatch.
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
  - Current cheat-code dialog pass converted 0x406d20 from the stale
    `Constructor()` initializer scaffold into a real C++ HudUiCheatCodeDialog
    constructor, following the accepted 0x41c290 HudUiNewGamePanel owner
    pattern and updating RecoilStateCheatCode plus focused smokes to use typed
    construction. Follow-up table-owner review corrected 0x4070e0 to
    `HudUiCheatCodeTitleWidget::OnActivate`: BN xrefs place it in the GO/title
    widget table at 4ccca0h slot 30h, while the cheat-code input widget table
    inherits `HudUiNumericTextInput::OnActivate`. The local functional target
    is now `hud_ui_cheat_code_title_widget_on_activate` and passes against the
    linked x86 native smoke. `python tools/recoil.py verify functional
    0x406d20` still passes; source deps/owner/data are accepted and the entry is
    tier B/model source-faithful. Local VC5SP3 data-symbol checks pass with
    zero unmasked mismatches for `g_HudUiCheatCodeDialog_FTable` 4ccc00h,
    `g_HudUiCheatCodeTitleWidget_FTable` 4ccca0h,
    `g_HudUiCheatCodeInputWidget_FTable` 4ccc10h, `k_EmptyString` 4e5ce0h,
    `"CHEAT_CODE_DIALOG"` 4da3c4h, `"dialog.zrd"` 4da3b8h, `"GO"` 4da3b4h,
    and `"CHEATCODE"` 4da3a8h; constructor EH metadata remains
    compiler/provider-owned. The constructor remains tier-S debt because
    `hud_ui_cheat_code_dialog_constructor` still fails function bytes due
    constructor EH/vtable scheduling, especially retail's delayed dialog vtable
    write versus normal C++ constructor body order.
  - Follow-up HUD activation/data audit accepted 0x4b5900
    HudUiZrdWidget::OnActivate as a source-faithful HudUiZrdWidget member with
    no direct authored globals (`Data reimplemented` `❎`) and accepted
    0x4070e0 HudUiCheatCodeTitleWidget::OnActivate data from the existing
    zero-mismatch `g_RecoilApp` BSS data-symbol evidence. Both entries are now
    tier B/model source-faithful; remaining visible blockers are tier-S-only
    dependencies (`RecoilApp::QueueExitCurrentState` and
    HudUiZrdWidget::OnActivate byte verification).
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
  - Current MpExit VC5SP3 follow-up resolved the remaining 0x419500 byte
    blocker in the shared HudUiWidget/HudUiZrdWidget virtual owner model.
    Focused BN table evidence from 4d3630h, 4cf0b0h, and 4cf028h shows slot
    +78h is `HudUiZrdWidget::RefreshState`, +7ch is
    `HudUiZrdWidget::LoadFromZrd`, and +80h is `zError::ReportOldNoOp`; source
    now models HudUiWidget center-position accessors in the inherited virtual
    slots so generated `m_mpNewGameButton.RefreshState` dispatches through
    +78h. `python tools/recoil.py verify functional 0x419500` and
    `python tools/recoil.py verify vc5 0x419500` pass, promoting 0x419500 to
    tier S. The grouped center-accessor VC5 target now verifies 0x404d90
    `HudUiWidget::GetCenterX` as tier S/data `❎`; 0x404dd0
    `HudUiWidget::GetCenterY` remains tier-S debt with a 29-byte arithmetic
    scheduling drift around the signed half-height computation.
  - Same-session HudUiPanelPtrVector::EraseRange refresh confirmed 0x4ba4d0
    remains a tier-S-only leaf blocker: `verify functional 0x4ba4d0` passes,
    while `verify vc5 0x4ba4d0` still fails with 14 unmasked epilogue bytes,
    zero relocation bytes, BN size 50, VC5 size 48, and 5 trailing VC NOPs
    trimmed. A source-level dead endpoint assignment probe that preserved the
    return iterator was byte-neutral at the same 14 mismatches and was
    reverted; the remaining drift is still BN's retained reload of the old
    end iterator and dead stack-parameter write after `end = write`.
  - Same-session HudUiPanelPtrVector::InsertN refresh closed the sibling owner
    blocker: `python tools/recoil.py verify functional 0x4ba510` passes, BN
    HLIL/MLIL shows the recovered VC5 vector insert only touches
    HudUiPanelPtrVector storage plus provider new/delete, source-shape guard is
    clean, and 0x4ba510 is now tier B with `Source owner ✅` for
    `HudUiPanelPtrVector`, `Data reimplemented ❎`, and `Model:
    source-faithful`. Follow-up VC5 target
    `hud_ui_panel_ptr_vector_insert_n` now runs; after recovering allocator-style
    guarded construction checks and the fuller in-place/reallocation split,
    `verify vc5 0x4ba510` improves from 526 unmasked mismatches / 336-byte VC
    body to 516 unmasked mismatches / 480-byte VC body, but still fails against
    the 547-byte retail body. Tier S remains open on residual VC5 vector-template
    branch/local scheduling.
  - Same-session flash-panel cleanup repaired the missing compiled
    `zhud_transition_text_panel_flash_rate_smoke` registration and retargeted
    `hud_ui_flash_panel_set_flash_color_and_rate` to that direct smoke.
    `python tools/recoil.py verify functional 0x4bc930`, `0x4bc980`, and
    `0x4bc9b0` pass; `python tools/recoil.py verify vc5 0x4bc980` passes with
    zero unmasked byte mismatches. Plan now records 0x4bc930 and 0x4bc9b0 as
    tier B with `HudUiTransitionTextPanel` owner/data `❎`, and 0x4bc980 as
    tier S. Follow-up ported the legacy
    `zhud_zrd_widget_load_from_zrd_smoke` into the compiled native runner;
    `python tools/recoil.py verify functional 0x4b59f0` now passes. Same-session
    BN/source owner audit accepted the self-contained HudUiZrdWidget slice:
    0x4b5310 Invalidate, 0x4b5740 RefreshState, 0x4b5860 HidePreview, and
    0x4b59f0 LoadFromZrd are tier B with `Source owner ✅` for class
    `HudUiZrdWidget`, `Data reimplemented ❎`, and `Model: source-faithful`.
    Tier S remains open pending VC5 byte targets/compares; the refreshed zSound
    path now records 0x4a0990 zSnd::FindSampleByName as tier B/data `✅`, with
    remaining verify debt routed through 0x4a0ec0
    zSndSampleSet::FindSampleByName rather than a HudUiZrdWidget owner/data
    blocker.
  - Same-session credits scrolling-text update repair registered the existing
    `hud_ui_zrd_scrolling_text_update` functional target in the compiled
    native smoke runner by adding `zhud_scrolling_text_update_smoke` to
    `zhud_widget_tests.cpp`/`smoke.cpp`. Debug and release x86 native builds
    pass, direct smoke execution passes, and `python tools/recoil.py verify
    functional 0x409410` passes. BN/source review confirms 0x409410 calls
    `HudUiElement::Update` and then dispatches every row panel through virtual
    slot +24h over the recovered `HudUiZrdScrollingText::rows` spans. The plan
    now records 0x409410 as tier B with `Source owner ✅` for class
    `HudUiCreditsPanel`, `Data reimplemented ❎`, and `Model:
    source-faithful`. Tier S remains open pending a VC5 byte target/compare.
  - Same-session 0x40a170 HudUiPanelLayoutEntry::CopyAssignRange audit cleared
    the stale owner/data blocker for the credits scrolling-text LoadFromZrd
    chain. BN assembly/HLIL shows a half-open HudUiPanelLayoutEntry range copy
    that calls the accepted HudUiPanel copy constructor and copies layoutX/Y;
    sibling layout-entry helpers 0x40a210, 0x40a1e0, and 0x409b60 already carry
    accepted HudUiPanelLayoutEntry ownership. The source docblock was upgraded,
    `verify functional 0x40a170` passes, source-shape and original-symbol
    guards pass, and the plan now records 0x40a170 as tier B with `Source owner
    ✅` for `HudUiPanelLayoutEntry`, `Data reimplemented ❎`, and `Model:
    source-faithful`. Tier S remains open because no VC5 COFF target covers
    0x40a170 yet.
  - Same-session 0x4babb0 HudUiPanel::SetFont owner/data audit repaired the
    stale compiled smoke coverage by registering `zhud_panel_set_font_smoke` in
    the native smoke runner and moving stable SetFont coverage into
    `zhud_composite_panel_smokes.cpp`. BN assembly/HLIL shows the function only
    replaces the provider-owned font handle, sets `textDirty`, and touches no
    authored globals. `verify functional 0x4babb0` passes after the native x86
    build, the source docblock was upgraded, and the plan now records 0x4babb0
    as tier B with `Source owner ✅` for `HudUiPanel`, `Data reimplemented ❎`,
    and `Model: source-faithful`. Tier S remains open because no VC5 COFF target
    covers 0x4babb0 yet.
  - Same-session 0x409570 HudUiZrdScrollingText::LoadFromZrd pass registered
    `zhud_scrolling_text_load_from_zrd_smoke` in the compiled native runner,
    repaired `HudUiPanel::CopyConstructCore` so raw vector-storage copies get the
    BN-proven HudUiPanel dispatch identity, and registered
    `zhud_panel_copy_construct_core_smoke` for the panel copy manifests. Native
    x86 build passes, direct smokes pass, `verify functional 0x409570`,
    `0x40a170`, `0x4ba850`, and `0x4ba9e0` pass, and the current VC5 compares
    leave 0x4ba850 at 343 unmasked mismatches/24 relocation bytes/10 trimmed NOPs
    and 0x4ba9e0 at 316/16/9. The plan now records 0x409570 as tier B with
    `Source owner ✅` for `HudUiCreditsPanel`, `Data reimplemented ❎`, and
    `Model: source-faithful`; tier S remains open because no VC5 COFF target
    covers 0x409570 yet.
  - Same-session HudCmd bind-button pass accepted 0x40ba60
    HudCmdKeyAButton::OnClearBinding at tier S. BN shows a typed
    selected-index clear through HudCmdDialog::ApplyPrimaryKeyRebind followed by
    HudCmdBindButtonBase::SetSelectedEntry; both direct callees already have
    accepted tier B owner/data gates. The stale FTable/table-factory blocker is
    clear for this method through the recovered HudCmdBindButtonBase/
    HudCmdKeyAButton class model, the repaired
    `zhud_cmd_key_a_button_on_clear_binding_smoke` target passes, and the
    VC5SP3 compare has zero unmasked byte mismatches.
  - Follow-up same-session HudCmd bind-button pass accepted 0x40bae0
    HudCmdKeyBButton::OnClearBinding at tier S. The compiled native smoke now
    registers `zhud_cmd_key_b_button_on_clear_binding_smoke`, which patches the
    owner rebind method and verifies the leaf forwards key code zero plus the
    selected index to HudCmdDialog::ApplySecondaryKeyRebind. `verify functional
    0x40bae0` passes, and `verify vc5 0x40bae0` reports zero unmasked bytes
    after 4 relocation-masked bytes and 11 trimmed VC NOPs.
  - Same bind-button leaf cleanup accepted 0x40bb30
    HudCmdJoyButton::OnClearBinding at tier S. The compiled native smoke now
    registers `zhud_cmd_joy_button_on_clear_binding_smoke`, which patches the
    owner rebind method and verifies forwarding of button code zero plus the
    selected index to HudCmdDialog::ApplyJoystickButtonRebind. `verify
    functional 0x40bb30` passes, and `verify vc5 0x40bb30` reports zero
    unmasked bytes after 4 relocation-masked bytes and 11 trimmed VC NOPs.
  - Same bind-button leaf cleanup accepted 0x40bb80
    HudCmdMouseButton::OnClearBinding at tier S. The compiled native smoke now
    registers `zhud_cmd_mouse_button_on_clear_binding_smoke`, verifies the
    debounce no-op path, and verifies forwarding of button code zero plus the
    selected index to HudCmdDialog::ApplyMouseButtonRebind when debounce is
    inactive. `verify functional 0x40bb80` passes, and `verify vc5 0x40bb80`
    reports zero unmasked bytes after 8 relocation-masked bytes and 2 trimmed
    VC NOPs.
  - Current HudCmdDialog dependency cleanup is auditing
    0x4b8100 HudUiCycleSelectorWidget::ApplyFontStyleForEntry as a tier-S
    blocker for 0x40a5b0 HudCmdDialog::Constructor and downstream
    HudCmdDialogState entries. BN shows a branchless font-style valid-marker
    mask and a fixed SetFont/field-copy order; same-session VC5 refresh starts
    at 146 unmasked mismatches with no relocation masking. An explicit
    `validMarker` local preserved behavior but was byte-neutral at the same
    146 mismatches and was reverted; the remaining drift is the missing
    post-mask `test esi,esi` plus downstream alignment/register scheduling.
  - Current HudCmdDialog dependency cleanup is also auditing
    0x4ba740 HudUiPanel::ConstructorDefault because 0x4b7fd0 and the
    HudCmdDialog constructor frontier route through this panel constructor.
    Owner/data are accepted and direct base constructor dependency
    0x4bcb50 is tier S; refreshed VC5SP3 compare starts at 121 unmasked
    mismatches after 24 relocation-masked bytes and one trimmed VC NOP, with
    remaining drift local to the post-font textRect/wrapRect/shadow/metric
    zero-store schedule. A `RECT *` text-bounds view preserved source behavior
    but was byte-neutral at the same 121 mismatches and was reverted.
  - Same-session HudUiCycleSelectorWidget::AddTextEntry pass promoted
    0x4b7fd0 to tier S. Source now uses normal VC5-era
    `new HudUiTransitionTextPanel`, reloads calls through `entriesA[index]`,
    and orders the inlined HudUiTransitionTextPanel constructor stores to match
    BN. `verify functional 0x4b7fd0` passes, and `verify vc5 0x4b7fd0`
    reports zero unmasked mismatches after 32 relocation-masked bytes and
    5 trimmed VC NOPs (BN 299, VC5 304). 0x4ba740 remains a separate
    constructor-default tier-S blocker for the HudCmdDialog constructor chain.
  - Same-session 0x40a5b0 HudCmdDialog::Constructor source-shape pass added
    the required constructor docblock and matched BN's command-group population
    loop by calling zInput::BindGroupList_GetCount at loop entry and backedge.
    `verify functional 0x40a5b0` still passes, and `verify vc5 0x40a5b0`
    improves from 640 to 636 unmasked mismatches after 224 relocation-masked
    bytes and 12 trimmed VC NOPs (BN 878 bytes, VC5 672 bytes). Data/B/S remain
    blocked by the constructor-cluster codegen gap: BN emits inline generated
    table stores for the HudCmdDialog/HudCmd* subobjects while current source
    still compiles as placement-new/member construction.
  - Follow-up focused 0x40a5b0 worker and BN fact passes found no safe
    source-faithful edit inside `zhud_ui.cpp`/`zhud_ui.h`: BN confirms the
    exact construction order and generated table stores for resume/reset,
    command-list, bind-button, set-list, navigation, prompt, and final dialog
    identities, while the current source already matches member order and loop
    behavior. Replacing the current spelling with explicit table stores,
    helper factories, raw addresses, or fake construction shims remains
    forbidden; keep 0x40a5b0 at C/data blocked unless a new source-faithful C++
    constructor spelling is proven.
  - 0x4038a0 `HudUiBriefingObjectivePicture::DrawWithNoiseOverlay` owner/data
    cleanup is now tier B: source now models BN's slot target as the
    `HudUiBriefingObjectivePicture::Draw` override, and
    `HudUiBriefingRuntime::Constructor` constructs the embedded
    `objectivePicture` as the derived C++ class instead of calling the base
    widget constructor directly. Functional smoke
    `briefing_objective_picture_draw_noise_overlay_smoke` is registered and
    passes. Data is accepted as `❎` because the only BN data read is a shared
    `.rdata` +0.0 literal pool constant, not authored Briefing global data.
    Tier S remains blocked by the refreshed VC5SP3
    compare: 74 unmasked mismatches after 12 relocation-masked bytes and
    5 trimmed VC NOPs.
  - HudCmdDialogState static lifecycle data gate is now accepted for
    `g_HudCmdDialogState`: BN/source/VC5SP3 data-symbol evidence identifies
    the BSS object as an 8-byte typed HudCmdDialogState with zero
    data-byte mismatches and no relocations. 0x40bc20 StaticInitAndRegisterAtExit,
    0x40bc40 RegisterAtExit, and 0x40bc50 AtExitDestructor are tier S with
    zero unmasked VC5 mismatches. 0x40bc30 StaticInit and 0x40bc60 Constructor
    are tier B and remain tier-S blocked by the placement-new tail-jump and
    constructor vptr/member store-order byte drift respectively.
  - 0x443700 RecoilApp_StateQueueBlock::InitFromCursor is now tier S: source
    assigns `m_cursor` before `m_chunkBaseSlot`, matching the nearby recovered
    inline queue-block assignment shape, and VC5SP3 now reports zero unmasked
    mismatches after 14 trimmed VC NOP bytes. 0x443310 QueuePushState remains
    a separate tier-S blocker at 351 unmasked mismatches.
- Next action:
  - Continue class-first cleanup on the remaining MpExit lifecycle entries:
    0x4198d0, 0x419940, and 0x419990 are tier S with owner/data accepted.
    Refreshed `0x419990` frontier now routes through 0x4a56d0 Time::Tick and
    0x489e10 zNetwork::ShutdownSessionRuntime as direct tier-S blockers.
    Time::Tick remains tier B/data `✅`: `verify vc5 0x4a56d0` still fails
    with 85 unmasked mismatches after 84 relocation-masked bytes and one
    trimmed VC NOP byte. A source-order probe that kept the old time-scale
    factor in a local and reset `g_Time_TimeScaleFactor` before derived stores
    preserved behavior but regressed the compare to 101 unmasked mismatches,
    80 relocation-masked bytes, and a 192-byte VC body, so it was reverted.
    zNetwork::ShutdownSessionRuntime remains tier B/data `✅`: refreshed
    status/frontier shows direct tier-S blockers 0x48c120
    `zNetwork::UnregisterPacketHandler`, 0x489fa0
    `zNetwork::ClearServiceProviderList`, and 0x48a030
    `zNetwork::ClearPlayerRecordList`. `verify functional` passes for all
    three list/dispatch helpers. A source-level `UnregisterPacketHandler`
    probe that made sentinel-loop state explicit and used 16-bit packet-type
    comparisons preserved behavior but regressed from 193 to 194 unmasked
    mismatches with the same 144-byte VC body; a `ClearPlayerRecordList`
    `bool hasNode` probe preserved behavior but regressed from 132 to 137
    unmasked mismatches and grew the VC body from 112 to 144 bytes; a
    `ClearServiceProviderList` split iterator/clear-slot pointer probe
    preserved behavior but regressed from 98 to 112 unmasked mismatches and
    grew the VC body from 128 to 144 bytes. All three probes were reverted.
  - Use focused status/frontier checks for the current anchor and source-shape
    guards over touched HUD files before editing.
