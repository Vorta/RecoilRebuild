# Implementation Groups

Use this tracked file for temporary dependency-group notes during
reconstruction. `.agent/SOURCE_OWNERS.json` is the durable owner-scope ledger;
this file lists only active multi-function, source-readiness, owner, or data
groups currently being coordinated. Pure tier `S` verification groups are active only after
`tier_s_priority_ready=true` or explicit user direction. Active groups are the
default no-address startup queue: new agents should resume actionable WIP here
before selecting new work with
`python tools/recoil.py plan next --lane binary`. Keep the header and template
available even when no groups are active.

## Rules

- Create or update a group before editing when a task touches more than one
  function or a shared type/global/vtable.
- Create or update the matching source-owner record with `python tools/recoil.py
  owner ...` before accepting `Source owner`, `Data reimplemented`, or tier
  `B`/`A`/`S` plan markers. This file is not source-owner evidence.
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
- Use `python tools/recoil.py audit handoff --path .agent/IMPLEMENTATION_GROUPS.md
  --strict` before launching workers from live handoff blocks.

## Active Group Template

```text
### Group: short descriptive name

- Anchor: 0xNNNNNN Name
- Owner id:
- Section:
- Queue: ready owner/data work / blocked pending evidence or policy / shared blocker / deferred verify-only debt
- Reason: dependency closure / class cluster / recursive cycle / shared ABI layout / source file cluster
- Source blockers:
  - 0xNNNNNN Name
- Next action:
  - python tools/recoil.py status 0xNNNNNN
```

## Source Worker Handoff Template

```text
### Parent batch card: short batch name

- Task kind: active WIP / address-led owner-data work / validation handoff
- Active group or address:
- Evidence packets required:
- Evidence packets received:
- Worker allocation:
- Validation scope:
- Exit criteria:

### Source-worker handoff: short scope name

- Section:
- Owner/source scope:
- Owner id:
- Anchor addresses/groups:
- Allowed write paths:
- Forbidden paths:
- Evidence inputs:
  - BN fact packet:
  - source-owner packet:
  - provider/data packet:
  - scaffold audit packet:
  - workspace/librarian packet:
- Expected source model:
- Validation commands:
- Return packet:
  - changed files
  - evidence used and caveats
  - commands run with pass/fail
  - blockers and overlap warnings
  - non-authoritative marker recommendations only
```

## Verifier Handoff Template

```text
### Verifier handoff: short scope name

- Section:
- Validation scope:
- Anchor addresses/groups:
- Exact commands:
- Evidence inputs:
  - source worker packet:
  - BN fact packet:
  - provider/data packet:
- Forbidden paths:
- Return packet:
  - exact command lines
  - pass/fail results
  - key output lines
  - failure category
  - next narrow verification command
```

## Active Groups

Active queue sections:

- Ready owner/data work: HudUi base element/widget owner-data cleanup is the
  current actionable HUD follow-up after the older Player create-from-names
  slice cleared to tier-S-only debt.
- Blocked pending evidence or policy: zVideo/zRndr renderer dispatch remains
  blocked on the ESP-pivot span-family source model unless new evidence or
  explicit policy direction appears.
- Shared blockers: the former zVideo adjust-surfaces cleanup is folded into
  the renderer dispatch/global owner audit because both route through 0x48ff70
  and the 0x42e330 caller/data path.
- Deferred verify-only debt: keep tier S-only zVideo/zRndr and HUD addresses
  in plan/VC manifests, not as active groups, while `tier_s_priority_ready=false`.

### Group: HudUi base element/widget owner-data cleanup

- Anchor: 0x4b3d00 HudUiWidget::Constructor, with base dependency 0x4b4070
  HudUiElement::Constructor.
- Owner ids: hud_ui.hud_ui_element_base, hud_ui.invalidate_mask_global,
  hud_ui.hud_ui_widget_geometry_invalidation,
  hud_ui.hud_ui_widget_destructor_wrapper, hud_ui.hud_ui_bar_class,
  hud_ui.hud_ui_meter_class, hud_ui.hud_ui_slot_class,
  hud_ui.hud_ui_message_class.
- Section: hud_ui
- Queue: ready owner/data work; new plan-next follow-up after active-group
  refresh found the older GameNet/Player groups verify-only and zVideo blocked
  by the zRndr ESP-pivot source-model policy.
- Reason: the primary HudOptionsDialog candidate is blocked by
  zRndr::SelectSpanRoutines, while the next HUD constructor candidates route
  through HudUiBackground::Constructor -> HudUiBackgroundCursorWidget ->
  HudUiWidget -> HudUiElement. The HudUiElement/invalidate-mask gates and the
  narrowed HudUiWidget geometry/invalidation owner are now accepted; remaining
  HUD owner/data debt is above this slice, especially the background/cursor
  constructors.
- Current evidence:
  - Added data owner `hud_ui.invalidate_mask_global` for
    `g_HudUi_InvalidateMask`. BN declares a 4-byte uint32 initialized
    to 0x0c; xrefs are limited to 0x4b3e90, 0x4b4180, and 0x4bc760.
    `python tools/recoil.py verify vc5 hud_ui_invalidate_mask_global` passed
    with zero unmasked data-byte mismatches.
  - Added source owner `hud_ui.hud_ui_element_base` for the accepted
    HudUiElement base-class method cluster. Direct authored data is limited to
    `g_HudUi_InvalidateMask`; owner byte evidence remains deferred by global
    tier-S policy.
  - Promoted 0x4b4180 `HudUiElement::Invalidate`, 0x4bc760
    `HudUi::SetInvalidateMode`, 0x4b3e90 `HudUiWidget::InvalidateRect`,
    0x4b4070 `HudUiElement::Constructor`, and 0x4b40c0
    `HudUiElement::CopyConstructor` to tier B after current functional/VC5
    data evidence. 0x4b4070 and 0x4b3d00 have same-session VC5 function-byte
    passes, but tier S remains deferred.
  - Added accepted source owner
    `hud_ui.hud_ui_widget_geometry_invalidation` for the narrowed
    HudUiWidget constructor/RebuildBltRectFromImage/SetPos/HitTest/
    InvalidateRect slice. Current BN/source review found no direct authored
    data beyond the accepted inherited invalidate-mask path; functional
    verification passed for 0x4b3d00, 0x404e10, 0x4b3dd0, 0x4b4030, and the
    existing 0x4b3e90 target.
  - Promoted 0x4b3d00 `HudUiWidget::Constructor`, 0x404e10
    `HudUiWidget::RebuildBltRectFromImage`, 0x4b3dd0 `HudUiWidget::SetPos`,
    and 0x4b4030 `HudUiWidget::HitTest` to tier B through
    `hud_ui.hud_ui_widget_geometry_invalidation`.
  - Registered `zhud_widget_release_and_destructor_core_smoke` in
    `recoil_native_smoke`, added owner
    `hud_ui.hud_ui_widget_destructor_wrapper`, and promoted 0x4b3ce0
    `HudUiWidget::ScalarDeletingDestructor` to tier B. The wrapper directly
    touches no authored globals; tier S remains deferred because no VC5SP3
    byte target covers 0x4b3ce0 yet.
  - The lower HudUiTripletPanel class owner
    `hud_ui.hud_ui_triplet_panel_class` is now accepted for boundary/source
    and functional gates after registering the missing triplet-panel native
    smokes and repairing provenance docblocks for the constructor/destructor
    cluster. Data remains blocked for constructor/shutdown paths because they
    touch `g_HudUiMgr`/`g_HudUiMgrNanitePanel`.
  - The old `hud_ui.hud_ui_mgr_data` owner has been clarified as a data owner,
    not the source-owner parent for HudUiMgr methods. BN models
    `g_HudUiMgr` as a 0x7844 authored singleton object ending before
    `g_HudLayoutHW`; current source still models only the prefix and split
    sibling globals for weapon slots, mode counters, messages, loading
    checkpoint storage, tail bar, timer pointers, and related state.
    `hud_ui.hud_ui_mgr_class` now records the blocked class owner route for
    0x40d7e0/static init/destructor/0x411750.
  - Added accepted source owners `hud_ui.hud_ui_bar_class` and
    `hud_ui.hud_ui_meter_class`. 0x4bcf20 `HudUiBar::HudUiBar`,
    0x40d9e0 `HudUiMeter::ConstructorEx`, and 0x40fb70
    `HudUiMeter::HudUiMeter` are promoted to tier B after current functional
    evidence, accepted inherited invalidate-mask data, and source-faithful
    class owner gates. 0x4bcf20 and 0x40fb70 have same-session VC5 function
    byte passes, but new tier S promotion remains globally deferred.
  - Added accepted source owner `hud_ui.hud_ui_slot_class` for
    0x40db20 `HudUiSlot::Constructor`, 0x40d780
    `HudUiSlot::Destructor`, 0x40dbd0
    `HudUiSlot::ScalarDeletingDestructor`, and existing 0x40db90
    `HudUiSlot::Draw`. The stale `zhud_slot_destructors_smoke`
    functional target was repaired and registered, and the constructor,
    destructor, and scalar-deleting destructor are now tier B with
    no authored globals touched. Added missing provider-boundary
    0x40d5f0 `HudUiWidget::DestructorCoreEhThunk` as MSVC
    EH/array-cleanup forwarding glue to 0x4b3d50.
  - Added accepted source owner `hud_ui.hud_ui_message_class` for the
    HudUiMessage constructor/destructor/deleting destructor, draw, layout,
    image-release, and static weapon-message update methods. Ported and
    registered the `zhud_message_*` native smokes from the unlinked test
    source into `recoil_native_smoke`; all twelve HudUiMessage functional
    targets pass. Data remains blocked because the owner still touches
    split `g_HudUiMgrMessages`/`g_HudUiMgr` state and the full 0x7844
    `g_HudUiMgr` data owner remains incomplete.
- Next action:
  - Continue at the HudUiMgr owner/data boundary. Refreshed
    `frontier 0x40d7e0 --depth 1 --lane binary` now has no lower source-owner
    blocker visible and recommends 0x40d7e0 `HudUiMgr::Constructor`; its
    direct data blockers remain HudUiMessage, HudUiTripletPanel, HudUiPanel,
    and the incomplete 0x7844 `g_HudUiMgr` data owner. Do not promote 0x40d7e0,
    0x411750, 0x4118b0, or 0x4132b0 data gates until the complete 0x7844
    `g_HudUiMgr` owner is source-faithfully modeled and verified. Keep the
    zVideo/zRndr shared renderer callback/surface-state blockers separate.

### Group: zVideo renderer dispatch/global owner audit

- Anchor: 0x4a77a0 zVideo::BindRendererDispatch
- Section: render_video
- Queue: blocked pending evidence or policy; shared blocker.
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
  - Folded adjust-surfaces status: 0x4a6900 has accepted direct renderer
    adjust-helper source and VC5 byte evidence, and `verify functional`
    evidence for 0x42e330 and 0x48ff70 has been repaired. The remaining
    reason to keep that path active is this shared 0x48ff70 data gate, so it
    should not live as a separate group.
  - 2026-06-19 update: `render_video.zvideo_renderer_dispatch` now has
    accepted boundary/source/data/functional gates for 0x4a77a0 and 0x4a6b40.
    VC5SP3 data-symbol manifests
    `zvideo_renderer_dispatch_core_globals`,
    `zvideo_renderer_dispatch_surface_globals`,
    `zvideo_renderer_dispatch_image_globals`,
    `zvideo_renderer_dispatch_texture_globals`,
    `zvideo_renderer_dispatch_fog_poly_globals`, and
    `zvideo_renderer_dispatch_flush_globals` cover the complete renderer
    selection/fullscreen/dispatch callback global set, including
    `g_zVideo_pfnImageLazyCreateVideoMemorySurface`,
    `g_zVideo_pfnImageEnsureSurfaceForCurrentDevice`,
    `g_zVideo_pfnQueryDeviceVideoMemoryBytes`, and
    `g_zVideo_pfnQueryTextureMemoryBytes`. Plan entries 0x4a77a0 and 0x4a6b40
    are now tier B; tier S remains globally deferred.
  - 2026-06-19 update: `render_video.zvideo_fog_color_globals`,
    `render_video.zvideo_d3d_device_globals`, and
    `render_video.zvideo_dd3d_fog_state` now have accepted data evidence.
    Source-worker pass reordered the zVideo fog/color scalar declarations to
    match BN storage; VC5SP3 data-symbol manifests
    `zvideo_fog_color_pending_bias`, `zvideo_fog_color_target_applied`,
    `zvideo_d3d_fog_cache`, `zvideo_d3d_device_globals_a`, and
    `zvideo_d3d_device_globals_b` passed. Plan entries 0x4a7220, 0x4a7250,
    0x4a7300, 0x4a7330, 0x4a73a0, 0x4aa9e0, 0x4aaa30, 0x4aaa60, 0x4aaa90,
    and 0x4aab30 are now tier B; tier S remains globally deferred.
  - 2026-06-19 update: `render_video.zvideo_pixel_pack_state`,
    `render_video.zvideo_dd3d_submit_queue_storage`, and
    `render_video.zvideo_dd3d_submit_queue` now have accepted data evidence.
    VC5SP3 data-symbol manifests `zvideo_pixel_pack_state_global`,
    `zvideo_dd3d_submit_temp_vertices`, `zvideo_dd3d_submit_sorted_queue`,
    `zvideo_dd3d_submit_overwrite_queue`,
    `zvideo_dd3d_submit_queue_counts`, and
    `zvideo_d3d_render_state_cache_global` passed for the complete
    pixel-pack, temp-vertex, sorted/overwrite queue, queue-count, and
    render-state cache storage. Plan entries 0x4a6b90, 0x4a6bb0, 0x4a6bd0,
    0x4a6bf0, 0x4aab90, 0x4aaef0, 0x4ab320, 0x4ab6d0, 0x4abb20, 0x4ac370,
    0x4acbd0, and 0x4ace30 are now tier B; tier S remains globally deferred.
  - 2026-06-19 update: `render_video.zvideo_dd_hw_api_feature_flags`
    now has accepted data evidence for the complete
    `g_zVideo_HwApiDeviceTable` storage. VC5SP3 data-symbol manifest
    `zvideo_hw_api_device_table_global` passed for the four 0x6ec-byte
    hardware API device records, and 0x4a9920 is now tier B; tier S remains
    globally deferred.
  - 2026-06-19 update: `render_video.zvideo_dd_surface_state_globals`,
    `render_video.zvideo_selected_hw_api_device_record`, and
    `render_video.zvideo_dd_primary_sw_blit` now have accepted data evidence.
    Existing VC5SP3 target `zvideo_dd_present_display_mode_surface_data`
    covers the four DirectDraw surface-state globals, new target
    `zvideo_selected_hw_api_device_record_global` covers
    `g_zVideo_pSelectedHwApiDeviceRecord`, and plan entries 0x4a7d90,
    0x4a7dd0, and 0x4a7e10 are now tier B; tier S remains globally deferred.
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
    implementation strategy. Prefer another active owner/data WIP while that
    slice remains blocked by the current source rules.
  - If the caller/data path is resumed, start with
    `python tools/recoil.py status 0x42e330 --lane binary`, then route through
    the 0x48ff70 data blocker before assigning any source worker.
  - Re-run `python tools/recoil.py audit groups --summary --wip-limit 4` after
    each owner/data update and prune this group again when it becomes
    verify-only.

### Group: GameNet launch/session-sync owner-data cleanup

- Anchor: 0x433710 GameNet::SetStatusBitsFromFlags, with launch-path
  dependencies 0x434460, 0x434550, 0x4321b0, 0x4320f0, 0x419470, and
  0x417770.
- Section: network_online
- Queue: resolved owner/data cleanup for the GameNet launch/session-start
  dependency cluster under HudUiNetGameSetupPanel; remaining visible debt is
  tier-S-only and is deferred by the global owner/data gate policy.
- Reason: HudUiNetGameSetupPanel_LaunchButton now routes past zOpt and
  zNetwork session-runtime blockers; the current lowest visible authored
  blocker is GameNet/HUD launch status, packet, handler, row-list, and
  mission-timer state.
- Current evidence:
  - Source-owner mapper classifies the GameNet launch blockers as a narrow
    `GameNet` source-file/subsystem owner in `src/Battlesport/GameNet.cpp` and
    `GameNet.h`, not standalone leaves. The status-bit subowner
    0x433710/0x433730/0x433740 is accepted at tier B after functional,
    source-owner, and data-symbol evidence.
  - Data classifier reports 0x433710 touches `g_GameNetStatus_AllowMaps` and
    `g_GameNetStatus_NameTags`; 0x434460 is accepted at tier B after
    `g_NetPkt14_HudTimerAndFlagsSyncBuf` data-symbol and zNetwork reliable-send
    owner evidence; 0x4321b0 touches
    `g_GameNet_HandlersRegistered`; 0x4320f0 touches GameNet player-row and
    spawn-point list globals and reads `g_HudUiTopMessageStack`.
  - 0x419470 and 0x417770 are accepted at tier B after no-authored-globals
    review; 0x417770 also exposed and resolved a missing MFC42
    provider-boundary entry for `CString::Empty` at 0x4c5bc4.
  - Minimal linked native smokes now cover the existing GameNet/HUD launch
    functional targets. Ignored local VC5 data-symbol target
    `gamenet_launch_session_globals` passes for the GameNet status, packet,
    handler, row-list, and spawn-list globals with zero unmasked mismatches.
  - GameNet row/spawn list source shape is now typed as
    `GameNetPlayerRowListState` and `GameNetSpawnPointListState`, and the
    pkt06 snapshot model is narrowed to the BN-backed 0xc0-byte packet with ten
    progress target points. The player-row methods 0x4345a0, 0x433a50, and
    0x434650 are accepted at tier B after row-method native smokes,
    source-owner review, and data/no-globals gates; BN still records a limited
    EH-decompilation note on 0x4345a0, but its typed row/list contract is
    verified by assembly.
  - The compiler intrinsic provider-boundary entry for BN symbolic
    `__builtin_memset` at 0x7c8ed4 is accepted for the inline `rep stos`
    zero/fill pattern used by 0x4320f0 and 0x4345a0.
  - 0x431c50/0x4321b0 are now narrowed to
    `network_online.gamenet_launch_session_sync`: a register/unregister
    coordinator owner with owned data `g_GameNet_HandlersRegistered`.
    Source-owner markers are accepted for both after focused owner mapping and
    passing registration/unregistration functional smokes. Data is now accepted
    after routing the registered callback band to accepted owners and verifying
    the owned `g_GameNet_HandlersRegistered` scalar through
    `gamenet_launch_session_globals`.
  - The Pickup spawn-list pkt11 synchronization dependency reached from
    0x432860 is accepted at tier B. Owner
    `battlesport_gameplay.pickup_spawn_list_pkt11_sync` covers 0x41e890,
    0x41e900, 0x433e40, 0x433e70, and 0x433ea0, with registered native
    functional smokes and VC5 data-symbol evidence for the two spawn-list
    globals plus the paired pkt11 short-delta globals.
  - The HUD timer tail-flags and pkt0D dependencies reached from 0x432860 are
    accepted at tier B. Owner `hud_ui.hud_timer_panel_net_state_tail_flags`
    covers 0x433a40 with no authored globals. Owner
    `network_online.gamenet_pkt0d_hud_timer_panel_state` covers 0x433250 and
    0x433310, with registered native functional smokes and VC5 data-symbol
    evidence for `g_HudTimerPanelNetState` plus
    `g_NetPkt0D_HudTimerPanelStateBuf`.
  - The stale zEffect lookup gate that blocked pkt0D handling is accepted at
    tier B under `engine.zeffect.anim_entry_name_lookup`; 0x45ff10 now has
    linked source/data gates for `g_zEffectAnim_EntryCount` and
    `g_zEffectAnim_EntryList`.
  - The crater relay callback chain reached from GameNet registration is now
    accepted at tier B. Owners `engine.zeffect.zdeclient_feature_map_tree`,
    `engine.zeffect.zdeclient_crater_create_feature`,
    `engine.zgeometry.clip_patch_output_apply_node_di_pairs`,
    `engine.zeffect.zdeclient_crater_instance_event`, and
    `engine.zeffect.zdeclient_crater_relay_callbacks` clear the former
    0x456b20/0x456c50/0x433ad0/0x433b70 source/data blockers, with VC5
    data-symbol evidence for the feature map-tree globals and current
    functional smokes.
  - The QSand relay chain reached from GameNet registration is now accepted at
    tier B. 0x455ed0 is
    accepted at tier B under
    `engine.zeffect.zdeclient_qsand_event_template_defaults`, backed by BN
    typing of `g_zDEClient_QuickSandEventTemplateDefaults` and VC5 data-symbol
    target `zdeclient_qsand_event_template_defaults`. Owners
    `engine.zeffect.zdeclient_qsand_feature_pipeline`,
    `engine.zeffect.zdeclient_feature_node_clip_partition`,
    `engine.zeffect.zdeclient_qsand_create_feature_display`, and
    `engine.zeffect.zdeclient_qsand_relay_callbacks` now have accepted
    source/data gates for 0x455ea0/0x4563d0/0x456010/0x456450, 0x46af40,
    0x4564b0, 0x455ef0, and 0x433d40. The QSand material pointer globals are
    accepted under `engine.zeffect.zdeclient_qsand_material_globals`, with VC5
    target `zdeclient_qsand_material_globals`; the QSand enabled flag
    `g_zDEClient_QuickSandEnabled` is accepted with VC5 target
    `zdeclient_qsand_enabled_global`. The former 0x4564b0/0x46af40 blockers
    through 0x44db00 `zClass_Object3D::DeleteNode`, 0x447e60
    `gwNodeSetDisplayInstance`, and the zDi bounds helpers are cleared at
    tier B/no-data or accepted data.
  - The Pickup pkt11/pkt12 handler chain reached from GameNet registration is
    now source-routed under
    `battlesport_gameplay.pickup_pkt11_pkt12_handlers`. Source-owner markers
    are accepted for 0x433f40, 0x4340a0, 0x41e930, 0x41e1c0, 0x41dc30,
    0x41dc60, 0x41e960, 0x41d8a0, 0x41ceb0, 0x41cf50, 0x41da20, 0x41dab0,
    0x41e330, and 0x41dcf0 after linked native functional smokes and focused
    docblocks. Data is now accepted at tier B: `g_PickupTypes`,
    `g_NextPickupId`, and the pkt12 relay buffer are covered by
    `pickup_pkt11_pkt12_handler_globals`; reused spawn-list storage is covered
    by `pickup_spawn_sync_globals`; downstream zClass/zEffect/zSound/zError
    data gates are accepted or no-data.
  - The zNetwork dispatch-handler list runtime reached from GameNet
    unregistration is now accepted at tier B under
    `engine.znetwork.dispatch_handler_list_runtime`. The owner covers
    0x48bf40, 0x48bfa0, 0x48bfb0, 0x48bfe0, 0x48bff0, 0x48c0a0, 0x48c120,
    and 0x48c200, with native functional smokes for the lifecycle,
    register/unregister, and dispatch paths plus VC5 data-symbol evidence for
    the dispatch-handler flag, sentinel, and count globals.
- Current blockers:
  - Resolved for owner/data: 0x431c50 and 0x4321b0 are accepted at tier B under
    `network_online.gamenet_launch_session_sync`; 0x432ed0, 0x433ca0,
    0x4340c0, 0x4344b0, and 0x461eb0 are also accepted at tier B after
    docblock/smoke activation, owner-ledger routing, and data-gate review.
    `python tools/recoil.py frontier 0x431c50 --depth 1 --lane binary` now
    shows only pure tier-S blockers in this registration band.
  - The pkt06 data gate is now accepted at tier B under
    `network_online.gamenet_pkt06_player_state_snapshot` for 0x4327e0,
    0x432860, and 0x432ae0. Direct `g_GameNetPkt06InitialSyncGate` storage is
    backed by BN declaration/xref evidence and VC5 target
    `gamenet_pkt06_globals`; linked row-list/color, HUD timer,
    top-message-stack, zVideo frame-tick, runtime DI scene, HudSensorTracker,
    and Player master-type data all route to accepted data owners.
  - The zClass copy-node dependency through 0x452500 is accepted at tier B
    after correcting `g_zClass_CopyNodeCloneDiMode` to the BN initial value 1,
    wiring the existing zClass copy smokes into `recoil_native_smoke`, and
    verifying ignored local VC5 target `zclass_copy_node_globals` with zero
    unmasked data mismatches.
  - The zClass metadata/name accessor dependency through 0x447dc0 is accepted
    at tier B after adding CRT provider-boundary entries for `strncpy` and
    `sprintf`, adding `zClass_NodePartial::name` layout asserts, documenting
    `ReportNullNode` helper provenance plus accessor docblocks, and wiring the
    existing metadata accessor smoke into `recoil_native_smoke`.
  - The Player clone helper dependency is accepted at tier B after
    correcting the Player bootstrap source shape, adding Player bootstrap
    provenance docblocks, adding a CRT `strstr` provider-boundary entry, wiring
    the existing Player bootstrap smokes into `recoil_native_smoke`, and
    confirming the direct data touch is only `g_Player_RuntimeDiScene`.
  - The zUtil save-state-list helpers 0x4383e0/0x4384e0 are accepted at tier B
    after wiring the existing save-state-list smoke into `recoil_native_smoke`,
    adding a CRT `malloc` provider-boundary entry, and confirming both helpers
    touch only caller-owned/heap-owned save-state storage with no authored
    globals.
  - The HudUiContainer removal dependency through 0x4bc810/0x4bc860 is
    accepted at tier B after adding immediate provenance docblocks, confirming
    no authored globals are touched, and rerunning the existing
    `zhud_container_child_list_smoke` functional targets. The remaining
    0x4320f0 caller blocker is now the scoreboard removal wrapper/triplet data
    chain, not the container detach helper.
  - The Object3D transform setters 0x44e300/0x44e030 are accepted at tier B
    after wiring the existing transform setter smoke into
    `recoil_native_smoke` and confirming both mutate caller-owned Object3D
    state with no authored globals.
  - The zClass child-link dispatcher dependency through 0x4483f0 is accepted at
    tier B under source owner `engine.zclass.add_child_dispatch`; the pass
    routes the stale camera/LOD functional manifests to registered native
    smokes, accepts `g_zError_DebugMsgBuffer` through
    zError::EmitDebugBuffer data evidence, and keeps RemoveChild outside this
    add-child owner scope.
  - Route zNetwork send/session-desc helpers and HUD row-removal/container
    dependencies as separate owner/data blockers; do not fold them into the
    GameNet owner.
  - Route the remaining 0x4143c0/0x40e880 data gates through the HUD
    stats-list and `HudUiTriplet::RebuildDisplay` owner/data slices. Do not
    broaden into the unrelated `zhud_ui.cpp` docblock backlog.
  - The Player bootstrap owner routes to
    `Player::GetSaveStateListHead`; treat the save-state list globals as a
    Player save-state/bootstrap record-global subsystem before promoting the
    launch caller. The zUtil save-state creation helper and Player save-state
    accessor are now accepted at tier B after
    source-order, functional-smoke, and VC5 data-symbol evidence for the
    save-state list globals. The Object3D getter/accessor blockers
    0x44dfd0/0x44e110/0x44e270/0x44e5b0 are accepted at tier B after helper
    provenance, getter-smoke, and no-authored-globals review. The zClass
    recursive name traversal blocker 0x452770 is accepted at tier B after
    documenting the source-faithful zClass traversal cluster owner, confirming
    no authored globals are touched, and rerunning the existing
    `zclass_find_sub_node_by_name` functional target. The zClass node
    pick-flag accessor mini-cluster 0x448100/0x448140/0x4481b0/0x4481f0/
    0x448230/0x448270 is accepted at tier B after documenting the typed
    zClass flag-accessor owner, registering and running
    `zclass_node_pick_flag_accessors_smoke`, and accepting the shared Class.c
    null-node/source-file string data with local VC5 data-symbol evidence.
    The direct registration helper blockers 0x48c0a0
    `zNetwork::RegisterPacketHandler` and 0x461eb0
    `zEffect_Anim::SetActivationDispatchContext` are accepted at tier B after
    functional coverage, owner/data review, zNetwork dispatch-list VC5 data
    evidence, and BN zero-data evidence for the zEffect activation-dispatch
    globals; neither helper has accepted tier S byte evidence.
  - The Player master-type transition dependency cluster 0x42b520/0x42ac90/
    0x42aeb0/0x42b0f0/0x42b2a0/0x42b4c0 is now linked to
    `battlesport_gameplay.player_master_type_transition` with accepted
    boundary/source/data/functional gates and source-faithful plan metadata;
    all six members are promoted to tier B. The data gate is backed by
    accepted owners for `g_Time_AccumulatedTimeSec`
    (`engine.time_runtime_globals`), the Player underwater pass-3 singleton
    (`battlesport_gameplay.player_underwater_fx_pass3_ui_singleton`), the
    horizon-follow node/flag pair
    (`battlesport_gameplay.player_horizon_follow_globals`), the copter sound
    cache (`battlesport_gameplay.player_copter_sound_cache`), and
    `g_GameStateOrMapTable` (`engine.zinput.game_state_or_map_table_data`).
  - The pkt06 row-apply/spawn/handler slice is split out as
    `network_online.gamenet_pkt06_player_state_snapshot`: 0x4327e0,
    0x432860, and 0x432ae0 now have accepted source-owner markers and
    source-faithful metadata after focused owner mapping, BN/source review,
    and passing functional targets. The data gate remains blocked while direct
    GameNet data and dependency-owner data are reviewed; accepted dependency
    data includes row color, HUD timer, Player runtime DI scene,
    HudSensorTracker, HUD top-message stack, zVideo frame-tick owners, and the
    Player master-type transition owner.
    0x431c50 remains in `network_online.gamenet_launch_session_sync` as a
    registration/callback-band owner, and 0x433de0/0x433c30 remain excluded
    pkt10/pkt0f feature-relay work.
  - `g_zVideo_FrameTick` is now accepted as
    `render_video.zvideo_frame_tick_global`: BN shows a standalone 4-byte
    zero-initialized int32, source declares it in the zVideo global
    block, and `zvideo_frame_tick_global` VC5 data-symbol verification passes
    with zero unmasked mismatches.
  - `g_HudUiTopMessageStack` is now accepted as
    `hud_ui.hud_ui_top_message_stack_global`: BN shows a standalone 4-byte
    zero-initialized `HudUiTextStack4 *`, source declares/externs
    it in the HUD UI sources with init/teardown lifecycle writes, and
    `hud_ui_top_message_stack_global` VC5 data-symbol verification passes with
    zero unmasked mismatches.
  - The zWeapon OptCatalog lookup/pending-spawn leaf repair registered existing
    `zweapon_optcatalog_find_entry_by_id_smoke` and
    `zweapon_optcatalog_pending_spawn_override_smoke`, added immediate
    provenance docblocks for 0x4ae450 and 0x4ae4a0, and reran both functional
    targets successfully. Owner `effects_weapons.optcatalog_entry_lookup`
    now accepts source/functional/data gates for 0x4ae3c0/0x4ae450 after
    accepted data owner `effects_weapons.optcatalog_entry_table_data` covered
    g_OptCatalog_EntryCount/g_OptCatalog_EntryTable with
    BN xrefs, source lifecycle, and VC5SP3 data-symbol checks; both lookup
    helpers are promoted to tier B. 0x4ae4a0 is linked into accepted owner
    `effects_weapons.optcatalog_runtime_instances` and promoted to tier B.
  - Owner `network_online.gamenet_pkt07_altgun_dispatch` covers
    0x434130/0x434190/0x434230 and now accepts boundary/source/functional/data
    gates. Bounded pkt07 smokes in `gamenet_launch_smokes.cpp` cover the
    sender, remote handler, and no-op callback; all three functional targets
    passed. The OptCatalog, Player alt-gun, and zNetwork reliable-send
    dependency data gates are now accepted, and direct packet-buffer owner
    `network_online.gamenet_pkt07_packet_buffer_data` accepts
    `g_NetPkt07_AltGunDispatchBuf` after BN/source review and
    `gamenet_pkt07_globals` VC5 data-symbol verification. All three pkt07
    members are promoted to tier B; tier S remains deferred to a coherent
    GameNet packet source-cluster pass.
  - Owner `battlesport_gameplay.player_alt_gun_runtime_dispatch` now accepts
    boundary/source/functional gates for 0x43c9c0, 0x43c190, 0x43aa30,
    0x43afd0, 0x43c330, 0x43c2d0, 0x43c430, and 0x43c550. The new bounded
    `player_alt_gun_runtime_smokes.cpp` native smoke source links into
    `recoil_native_smoke`, and all eight functional targets passed. The owner
    now accepts data after direct owners for g_GameStateOrMapTable,
    g_HudSensorTracker, and g_zInputFfEffectSet plus dependency owners
    OptCatalog entry lookup/runtime instances and Player weapon-bank/Mines ZAR
    were accepted; all eight members are promoted to tier B. Tier S remains
    deferred for a coherent Player alt-gun source-cluster pass.
  - Data owner `engine.zinput.force_feedback_effect_set_global` now accepts
    boundary/source/data gates for `g_zInputFfEffectSet`; VC5
    data-symbol target `zinput_force_feedback_effect_set_global` passed with
    zero unmasked mismatches.
  - The zDEClient crater relay callback smoke/docblock gap is repaired for
    0x433ad0, 0x433b70, 0x456b00, and 0x456c50. Data owners
    `engine.zeffect.zdeclient_crater_event_template_defaults` and
    `engine.zeffect.zdeclient_net_relay_callback_globals` now accept the
    crater default-template global and paired relay callback slots with VC5SP3
    data-symbol evidence. Leaf owner
    `engine.zeffect.zdeclient_crater_event_template_defaults_access` promotes
    0x456b00 to tier B. The broader owner
    `engine.zeffect.zdeclient_crater_relay_callbacks` remains blocked on
    adjacent crater instancing/material-pool owner dependencies.
  - zModel crater-instancing dependencies 0x4805e0
    `zModel_Matl::GetPoolEntry` and 0x481530/0x481540
    `zModel_Const` vertex-merge epsilon accessors are now accepted at tier B.
    The material-pool data owner was already accepted; the vertex-merge
    epsilon pass registered the existing zModel constants smoke and added
    VC5SP3 data-symbol evidence for `g_zModel_ConstVertexMergeEpsilon`.
  - The zDEClient crater init caveat through 0x456c80 is resolved. Camera
    accessors 0x458aa0/0x458ac0/0x458ae0 are accepted at tier B after camera
    global VC5 data-symbol evidence and smoke registration; zClass world-grid
    helpers 0x450650/0x450790 are accepted at tier B with no authored globals
    after registering the world-grid smoke. 0x456c80 now has accepted
    source-owner and no-data gates and is promoted to tier B.
  - The zGeometry ClipPatch dependency chain from 0x4570e0 is narrowed.
    Source-worker passes repaired missing zGeometry smoke registrations and
    provenance docblocks. Owners
    `engine.zgeometry.xy_vector_leaf_helpers`,
    `engine.zgeometry.weiler_buffer_lifecycle`,
    `engine.zgeometry.weiler_state_lifecycle`,
    `engine.zgeometry.clip_polygon_lifecycle_helpers`,
    `engine.zgeometry.model_polygon_snap_buffer_helpers`, and
    `engine.zgeometry.clip_polygon_snap_near_node_model` now have accepted
    source/no-data/functional gates; their members are promoted to tier B.
  - The Weiler init contour-source slice is now accepted under owner
    `engine.zgeometry.weiler_init_contour_source`. Members 0x464680, 0x4683a0,
    0x469960, 0x464b90, 0x4693c0, 0x4676c0, 0x4693a0, and 0x468410 have
    accepted source/no-data gates and tier B markers; source-map and owner
    audits are current after the docblock repair.
  - The next Weiler clip-point-list subowners are also accepted:
    `engine.zgeometry.weiler_point_in_contour_classifier` promotes 0x468a10,
    `engine.zgeometry.weiler_preclassified_contour_pair_bounds` promotes
    0x464ea0/0x464c90,
    `engine.zgeometry.weiler_selected_input_contour_output` promotes 0x468700,
    `engine.zgeometry.weiler_clip_point_list_auxiliary_helpers` promotes
    0x469af0/0x469a30,
    `engine.zgeometry.weiler_point_side_table_builder` promotes 0x468470,
    `engine.zgeometry.vec3_between_endpoints_xy` promotes 0x469ca0,
    `engine.zgeometry.weiler_forward_segment_pair_at_point` promotes 0x468650,
    `engine.zgeometry.weiler_preclassify_input_contour_pair` promotes 0x464f70,
    `engine.zgeometry.weiler_intersect2d_tables` promotes 0x468fa0/0x468c40,
    `engine.zgeometry.weiler_divide_contour_segment_at_point` promotes
    0x468580, and
    `engine.zgeometry.weiler_adjacent_edge_pair_against_segment` promotes
    0x469450.
  - The remaining Weiler clip-point-list caveat is now cleared through accepted
    source/no-data/functional gates for
    `engine.zgeometry.weiler_adjacent_edge_pair_against_pair` (0x469560),
    `engine.zgeometry.weiler_classify_contained_contour` (0x465ac0),
    `engine.zgeometry.weiler_validate_xings` (0x46a1f0),
    `engine.zgeometry.weiler_merge_contours` (0x467710),
    `engine.zgeometry.weiler_get_next_contour_segment_for_traversal` (0x469430),
    `engine.zgeometry.weiler_new_contour` (0x4680b0),
    `engine.zgeometry.weiler_select_forward_start_point_in_contour_a`
    (0x469d60),
    `engine.zgeometry.weiler_generate_outside_results` (0x4687b0),
    `engine.zgeometry.weiler_clip_output_destroy` (0x464b30),
    `engine.zgeometry.weiler_restore_output_z_from_input_plane` (0x469b60),
    `engine.zgeometry.weiler_output_contour_to_polygon_set` (0x4682c0),
    `engine.zgeometry.weiler_output_contours_for_clip_mode` (0x4681a0), and
    `engine.zgeometry.weiler_clip_point_list` (0x464810). These are
    source-faithful tier B slices with functional evidence; only the intersect2d
    table owner carries accepted authored data, while the rest are
    no-authored-global slices. Owner-level tier S remains deferred.
  - The zGeometry polygon convexification caveat is now cleared under
    `engine.zgeometry.polygon_convexification`. 0x46ced0
    `zGeometry_Polygon::TrySplitPointDwordOffsetsAtBestDiagonal`, 0x46cb50
    `zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive`, 0x46c720
    `zGeometry_ConvexPolygonSet::Destroy`, and 0x46c760
    `zGeometry_Polygon::Convexify` have accepted source/no-data/functional
    gates and tier B markers. The 0x46cb50 source now uses the BN-visible
    split-helper recursion instead of the prior ear-clipping caveat, and the
    functional manifest caveat was reduced to the tier-S deferral.
  - The zGeometry triangulate-hole caveat is now cleared under
    `engine.zgeometry.triangulate_hole_source_cluster`. 0x46c070
    `zGeometry::TriangulatePolygonWithHole`, 0x46c390
    `zGeometry_TriangulateHole::CacheCombinedPlane`, 0x46c570
    `zGeometry_TriangulateHole::ProjectInnerRingOntoCachedPlane`, 0x46bd50
    `zGeometry_TriangulateHole::TryAppendBridgeEdge`, 0x46bf30
    `zGeometry_TriangulateHole::CollectActiveEdgeIndicesForVertex`, 0x46bfc0
    `zGeometry_TriangulateHole::TryEmitTriangleFromEdgePair`, 0x46bf70
    `zGeometry_TriangulateHole::FindActiveEdgeState`, 0x46c620
    `zGeometry_Vec3Array::EnsurePositiveCrossZ`, 0x46c5b0
    `zGeometry_Vec3Array::ReversePoints`, and 0x46c3a0
    `zGeometry_Vec3Array::ComputeNewellPlane` have accepted
    source/data/functional gates and tier B markers. The authored scratch
    globals are covered by `zgeometry_triangulate_hole_globals`; owner-level
    tier S remains deferred.
  - The zGeometry ClipPatch caller chain from the crater build path is now
    cleared through tier B. Owners
    `engine.zmodel.dipool_runtime_globals`,
    `engine.zutil.store_int32_leaf`,
    `engine.zgeometry.model_process_clip_patch_node`,
    `engine.zgeometry.clip_polygon_process_node_polygon_set_xy`, and
    `engine.zgeometry.model_clip_patch` accept boundary/source/data/functional
    gates. This promotes 0x482080, 0x4820f0, 0x4826a0, 0x46b6d0, 0x46b550,
    and 0x46b1f0 through tier B. The zDEClient crater build wrapper 0x4570e0
    is also accepted as `engine.zeffect.zdeclient_crater_build` with no direct
    authored globals after repairing its smoke registration and provenance
    docblock; unrelated zdec_init.cpp docblock backlog remains outside this
    slice.
- Next action:
  - Continue owner/data routing from the lowest visible crater relay callback
    dependency blocker. The 0x4570e0 -> 0x46b1f0 ClipPatch chain is now tier B;
    `frontier 0x433b70 --depth 1 --lane binary` now recommends 0x456c50
    `zDEClient_Crater::InstanceEventWithNetRelay` as the lowest visible source
    owner blocker for the crater relay callback owner.
    Keep pkt06 data blockers separate, especially the Player master-type
    transition data reached from 0x432ae0; do not absorb unrelated HUD,
    zNetwork, Pickup, zDEClient, zEffect, or pkt0f/pkt10 feature-relay targets.

### Group: Player create-from-names bootstrap owner-data

- Anchor: 0x421ab0 Player::CreateFromNamesAtPose, with wrapper 0x421ea0
  Player::CreateFromNamesAtPoseGetState.
- Section: battlesport_gameplay
- Queue: deferred verify-only debt; owner/data dependency slice for the GameNet
  pkt06 remote spawn path is cleared at the current 0x420d10 depth-1 frontier.
- Reason: Player class bootstrap/save-state creation owner and touched
  Player/HUD/zClass/zEffect data gates block GameNet pkt06 tier B promotion.
- Current evidence:
  - The GameNet pkt06 remote-spawn frontier recommends 0x421ea0, and
    `frontier 0x421ea0` routes directly to 0x421ab0.
  - 0x421ab0 still has accepted dependencies for Object3D pose setters and
    zUtil save-state-list allocation/append helpers. zClass type/name lookup,
    clone helpers, AddChild, Player::CloneType6NodeFromTemplateAndRename, and
    the local Player bootstrap methods 0x421ed0/0x4220f0/0x421830 remain
    owner/data-blocked.
  - The zOpt network-mode leaf was re-audited after stale positive
    source-owner markers were downgraded. It is now linked to
    `engine.zgame.zopt_network_options`; 0x408230/0x408240/0x408250/0x408260/
    0x408270 are accepted at tier B after docblock cleanup, accepted owner
    source/data/functional gates, functional coverage, and `zopt_network_option_globals`
    VC5 data-symbol evidence. Owner-level tier S remains deferred.
  - The linked Player bootstrap functional targets now pass for 0x420d10,
    0x421470, 0x421790, 0x421ed0, 0x4220f0, 0x421830, 0x421ab0, 0x421ea0,
    0x421a40, and 0x42aa40 after registering the existing native smokes in
    `recoil_native_smoke`.
  - The missing linked smoke for 0x407700 `zGame::Options_LoadGameOptions` is
    repaired: `zgame_options_load_game_options_minimal_smoke` is registered in
    `recoil_native_smoke` and `verify functional 0x407700` passes. 0x407700 is
    now linked to `engine.zgame.options_load_game_options` and accepted at tier
    B after clearing all direct owner/data blockers, including zVideo option
    globals, WOL password flag, bind-map current rebuild, zReader load-node
    globals, and the option-load-only stride/camera globals. Remaining visible
    blockers below 0x407700 are tier-S-only and remain deferred by the global
    authored owner/data policy.
  - The visible 0x407700 option-helper frontier was narrowed: the
    zOpt sound option owner `engine.zgame.zopt_sound_options`, the fullscreen
    pair owner `engine.zgame.zopt_fullscreen_option`, and the zInput joystick
    option owner `engine.zinput.joystick_option_accessors` are now accepted at
    tier B after linked owner/data gates, targeted functional checks, local
    VC5 function checks, and matching data-owner evidence. The 0x42a550
    `zInput::BindMap_InitDefaultBindings` route is also accepted at tier B
    after recording the `zInput_BindMapContext` class owner, the current
    binding-record wrapper owner, bind-group/default setup owner, and their
    zero-initialized data owners. Owner-level tier S remains deferred by the
    global owner/data policy; 0x42a550 still has known default-table/loop VC5
    byte drift. A refreshed `frontier 0x407700 --depth 1 --lane binary` now
    routed to 0x4080b0 `zSnd::GetAudioApiOption`; the zSound backend/CD-audio
    option owner `engine.zsound.backend_option_accessors` and data owner
    `engine.zsound.backend_option_globals` are now accepted at tier B for
    0x4080a0, 0x4080b0, 0x408210, and 0x408220. The same frontier then routed
    through the remaining direct data blockers: game difficulty/player name,
    zVideo option and video-mode globals, WOL password flag, bind-map current
    rebuild, and zReader load-node data gates. Those are now accepted at tier B
    with linked owners and current functional/VC5 data-symbol evidence.
  - 0x44ecf0 `zClass::FindByTypeAndName` is now accepted at tier B after
    linking the narrow `engine.zclass.typelist_find_by_type_and_name` owner,
    verifying the exact-name lookup smoke, accepting the `List.c` source shape,
    and adding `zclass_find_by_type_and_name_data` VC5 data-symbol evidence for
    `g_zClass_TypeList_HeadSlotPtrs` and `g_zClass_TypeList_Buckets`. Tier S
    remains deferred because the function-byte target still has the known
    inline-strcmp byte diff.
  - Refreshed `frontier 0x421ab0 --depth 1 --lane binary` shows the direct
    create-from-names frontier at tier B. Prior owner/data blockers for
    zClass clone/name/add-child/damage/camera/material helpers, Player
    modal/spawn/hit/destroyed-state helpers, HudSensorTracker::SetTrackedSaveState,
    OptCatalog damage-mask lookup, and zEffectAnim::FindEntryByName are now
    cleared to tier B; remaining visible 0x421ab0 dependency blockers are
    tier-S-only and deferred by the global owner/data policy.
  - The zEffect leaf lookup 0x45ff10 `zEffectAnim::FindEntryByName` is
    accepted at tier B after a linked native smoke, source-owner review, and
    BN/source zero-data evidence for `g_zEffectAnim_EntryCount` and
    `g_zEffectAnim_EntryList`.
  - Source-owner mapping for the remaining zEffect velocity path classifies
    0x45d930/0x45dcb0/0x45dde0/0x461aa0 as a zEffect animation
    runtime/activation-record source cluster in `zEffect.cpp`/`zEffect.h`;
    do not fold it into Player. The slice now has linked native-smoke
    coverage for activation runtime, velocity, thunk dispatch, and type-2
    activation-record queue behavior; the record/dispatch queue globals are
    corrected to BN-matching enabled initializers. The activation-record queue
    owner slice 0x4603d0/0x460400/0x460470/0x460480/0x460ae0/0x461800/
    0x461970/0x461aa0/0x461a90/0x461ba0/0x461d00 is accepted at tier B after
    docblock cleanup, functional coverage, `strncmp` provider-boundary
    classification, and BN/source data review for the queue/dispatch globals.
    The remaining zEffect velocity path still stays below tier B because
    `zEffectAnim::ActivateRuntime` and the zClass camera setter dependencies
    remain owner/data blockers.
  - The zClass callback-priority/type-list cluster 0x447f30
    `zClass_Class::gwNodeSetActionCallback` and 0x448090
    `zClass_Class::gwNodeSetPriority` is accepted at tier B after shared
    recovered-helper provenance, functional coverage, owner review, and
    no-direct-authored-global data review. 0x447fe0 remained tier B after the
    shared helper cleanup and functional recheck.
  - zMath data gates found through the same launch/bootstrap frontier were
    resolved for 0x472670 `zMath::Vec3DeltaLengthSq` and 0x474260
    `zMath::MatBuildEulerRotation3x3`; both are accepted at tier B, with
    0x474260 using `Data reimplemented ❎` because it touches no authored
    globals and only reads compiler/CRT x87 threshold rdata.
  - The ZBD node-array leaf slice 0x454370 `GameZ_ZBD::NodePtrToIndex`,
    0x4543a0 `zClass::NodePtrToValidatedIndex`, and 0x4543d0
    `GameZ_ZBD::NodeIndexToPtr` is accepted at tier B after touched
    docblocks, linked native smoke coverage, source-owner review, and shared
    zClass node-array/free-list data evidence.
  - Separate zClass blockers remain for camera setters and zClass light/sound
    clone helpers under `zEffectAnim::CloneEntryForNode`.
  - Source-owner mapping for 0x452fd0 `zClass_Light::gwLightNew`
    expands the blocker to the `zClass_LightDataPartial`/`zClass_Light`
    record-source cluster in `Light.c`, not a standalone leaf or C++ table
    owner. The first cleanup slice repaired Light.c recovered-helper
    provenance and split-signature docblock placement, moved the existing
    `zclass_light_new_smoke` into the linked zClass native-smoke source, and
    reran `verify functional 0x452fd0` successfully. The 0x453110
    `zClass_Light::DeleteNode` path is now accepted at tier B after the
    zClass_Light record-source owner audit, direct Light.c diagnostic-string
    VC5 data evidence, functional coverage, and accepted transitive
    `zClass_Class::TryFreeNode`/zError data/provider gates. 0x452fd0 is also
    accepted at tier B after the source-shape repair from direct active-flag
    write to `zClass_Class::gwNodeSetActive(node, 1)`, functional coverage,
    direct Light.c diagnostic-string data evidence, and accepted node/type-list
    data/provider gates.
  - The 0x447b60 `TryFreeNode` path now routes through 0x44f000
    `zClass_List::DeleteNodeFromLists`. Owner mapping classifies 0x44f000 as
    part of a small authored `List.c` deferred-removal source cluster, not a
    standalone leaf: 0x44e700 `zClass_TypeList::ProcessPendingRemovals` and
    0x44e920 `zClass::ProcessDeferredWork` are now source-owner accepted with
    0x44eea0 `zClass_NodeList::ProcessPendingFrees` and 0x44f000. Broader
    0x44f120 `DeleteAllOfType` and 0x44f1d0
    `gwListDeleteANode` are now accepted at tier B after registering their
    existing native smokes, rerunning functional coverage, linking 0x44f120
    to `engine.zclass.node_free_and_deferred_work`, linking 0x44f1d0 to
    `engine.zclass.remove_child_delete_dispatch`, and accepting their
    owner/data gates; tier S remains deferred. Data classification proves
    0x44f000 is not
    no-globals: it touches List.c error/source strings, `g_zError_DebugMsgBuffer`,
    and, through `MarkPendingRemoval`, the authored type-list head-slot table
    and bucket dirty fields. 0x44e690 `zClass_TypeList::FreeLink`,
    0x44e700 `ProcessPendingRemovals`, and 0x44f000
    `DeleteNodeFromLists` are now accepted at tier B after BN repaired the
    `g_zClass_TypeList_Buckets` aggregate as `zClass_TypeListBucket[16]`,
    ignored local VC5 data-symbol evidence passed for the List.c typed
    globals/error strings, and the 0x44f000 error path was corrected to BN's
    direct `sprintf` + `zError::EmitDebugBuffer` shape. 0x447a70
    `zClass_Class::FreeNodeToFreeList` is now source-owner/data accepted and
    tier B after the Class.c node-pool globals were matched to BN/VC5 data
    evidence (`g_zClass_NodeFreeHeadIndex` initializes to -1), the Class.c
    strings were rechecked, and `verify functional 0x447a70` passed through
    `zclass_node_free_and_deferred_work_smoke`. With that callee cleared,
    0x44ed60 `zClass_NodeList::Insert`, 0x44eea0
    `zClass_NodeList::ProcessPendingFrees`, 0x44e920
    `zClass::ProcessDeferredWork`, and 0x447b60
    `zClass_Class::TryFreeNode` are also accepted at tier B after their
    functional targets passed with the same registered smoke and their direct
    data/callee data gates were accepted.
  - Current zEffect route from 0x420d10 is the velocity/activation and
    stop/cleanup record-source subsystem in `zEffect.cpp`: 0x45dde0 ->
    0x45dcb0 -> 0x45d930 -> 0x45d570 -> 0x45c040. Source-owner evidence maps
    the slice to `zEffectAnimEntry`/runtime records, not Player and not a C++
    class owner. Local provenance/docblock blockers for the stop/cleanup helper
    slice were repaired, and 0x45e730/0x45ed80 clone/rebind data symbols are
    now accepted at tier B. The zClass world light/sound add/remove
    attachment-list subcluster 0x451360/0x451410/0x451590/0x451640 is also
    accepted at tier B, and Object3D transform setters 0x44df00/0x44e4f0 are
    accepted at tier B after repairing the SetScale double-literal source drift.
    The small zEffect reset helper slice 0x45c2f0 `HandleEmitterResetEvent`,
    0x45d240 `CaptureNodeStates`, and 0x45d310 `RestoreNodeStates` is accepted
    at tier B after registering the existing native smokes, rerunning the
    functional targets, repairing touched docblock provenance, and confirming
    no direct authored globals are touched. The stop cleanup refs 0x45bf60
    `CleanupLightRefs` and 0x45bfd0 `CleanupSoundRefs` are accepted at tier B
    after registering `zeffect_cleanup_light_sound_refs_smoke`, rerunning their
    functional targets, and verifying `g_zEffect_World`/stop-cleanup globals
    with local VC5 data-symbol evidence. Local VC5 data-symbol evidence now
    covers the shared `zeffect_stop_cleanup_globals` data owner
    (`g_zEffect_World`, `g_zEffect_FrameDeltaRemainingSec`, and related
    zeff_anim_run.c diagnostics), the zClass Class.c root-lookup diagnostic
    rdata, and the zeff_anim_init/copy-node diagnostic data owner used by
    rebind/clone helpers. 0x449ab0 `gwNodeGetRoot`, 0x45d6c0
    `ResetForNode`, and 0x45ed80 `RebindEntryToNode` are accepted at tier B
    from current owner/data/functional evidence. Local VC5 data-symbol
    evidence also confirms the shared Time frame-delta global
    `g_FrameDeltaTimeSec`. The previously remaining 0x45d010 `RunSequence`
    route through 0x45c640 `GetConditionalRefPosDistanceSq` -> external zClass
    0x4497b0 `GetWorldPosition` -> 0x449480 `BuildNodeToAncestorMatrix` plus
    zMath matrix-stack data gates is now accepted at tier B. Durable owners
    added/accepted in the 2026-06-19 pass: `engine.zmath.matrix_stack_current`,
    `engine.zclass.node_world_transform_helpers`,
    `engine.zeffect.conditional_ref_pos`,
    `engine.zmodel.variant_tag_current_filter`,
    `engine.zeffect.anim_runtime_sequence_core`, and
    `engine.zeffect.anim_activate_runtime`. 0x45c040 `Stop`, 0x45cc00
    `RunSequenceEvents`, 0x45d010 `RunSequence`, and 0x45d930
    `ActivateRuntime` are now tier B; tier S remains deferred.
    The zEffect runtime-ref creation helpers 0x45e380
    `FindOrCreateSoundRef` and 0x45e4a0 `FindOrCreateLightRef` are accepted
    at tier B after registering their shared native smoke and verifying the
    zeff_anim_init.c source-path plus sound/light overflow-format rdata with
    local VC5 data-symbol evidence. zClass 0x452d00
    `zClass_Sound::gwSoundSetPosition` is also accepted at tier B after the
    existing sound leaf smoke passed and the Sound.c diagnostic/source rdata
    target reverified.
- Next action:
  - Do not schedule this Player slice as active owner/data work unless a deeper
    refreshed frontier exposes a new non-tier-S blocker or the user explicitly
    directs tier S work. Current `frontier 0x420d10 --depth 1 --lane binary`
    reports only tier-S-only direct-callee debt for the visible slice.
  - Historical route note: an earlier 0x420d10 frontier routed to 0x438ba0
    `Player::LoadWeaponBanksAndSelectDefaults`. 0x4b1f90
    `OptCatalog::FreeTrailRuntimeStateStorage` is now accepted at tier B after
    adding the required provenance docblock, rerunning functional evidence, and
    accepting no-authored-globals data; tier S remains blocked by the known
    VC5 free-call byte diff. 0x438b60
    `Player::FreeAltWeaponTrailRuntimeStates` is also accepted at tier B after
    BN/source owner review, functional coverage, and no-authored-globals
    review.
  - Continue 0x438ba0 through 0x43ca90
    `Player::CheckMissionWeaponAvailability` is now accepted at tier B after
    BN/source owner review, `player_check_mission_weapon_availability`
    functional coverage, clean focused source-shape/original-symbol/raw-offset
    guards for `src/Battlesport`, and no-direct-authored-global data review.
    0x44de80 `zClass_Object3D::gwObject3DSetLitFlag` is also accepted at tier B
    after registering the existing alpha/lit native smoke in the built zClass
    smoke translation unit, rerunning functional evidence, and verifying its
    shared zClass/Object3D diagnostic strings with VC5 data-symbol evidence.
    0x4b1ec0 `OptCatalog::CreateTrailRuntimeState` is accepted at tier B after
    its functional smoke passed, the 0x44de80 callee cleared, and VC5
    data-symbol evidence covered `g_OptCatalogRuntimeWorld` plus the local
    ignored `BeamReflect_%d` string manifest entry. Refreshed
    `frontier 0x438ba0 --depth 1 --lane binary` routed next to 0x439540
    `Player::ApplyAltWeaponSwitch`.
  - 0x4385a0 `Player::StartMasterTypeLoopSfxHandle` is accepted at tier B
    after adding the required provenance docblock, registering its native
    smoke in the built bootstrap-smoke translation unit, rerunning functional
    evidence, and accepting no-authored-globals data. 0x453400
    `zClass_Light::gwLightSetRange` and 0x4b2570 `Light::ReturnToFreeList`
    are accepted at tier B after focused source/data review; `ReturnToFreeList`
    has passing VC5 function-byte evidence but tier S remains globally
    deferred. 0x4aefb0 `OptCatalog::DeactivateTrailRuntimeState` is accepted
    at tier B after registering its existing native smoke, repairing its
    provenance docblock, rerunning functional evidence, and accepting
    no-direct-authored-global data. 0x439540 `Player::ApplyAltWeaponSwitch`
    is accepted at tier B after its functional target passed and
    `g_GameStateOrMapTable` reverified with VC5 data-symbol evidence.
    0x439600 `Player::ApplyPrimaryWeaponSwitch` is accepted at tier B after
    BN/source owner review, functional coverage, and no-direct-authored-global
    data review. 0x4b21c0 `PlayerTimedHitStatus::ResetFields` is accepted at
    tier B after functional coverage, typed `PlayerTimedHitStatus` owner
    review, and no-direct-authored-global data review. The zUtil ZAR
    registration route 0x4c0280 `zZbdManager::RegisterSectionHandler` and
    0x4bffe0 `zUtil_ZAR::RegisterSectionHandler` is accepted at tier B after
    registering the existing native smoke, converting `zZbd.cpp` provenance
    comments to docblocks, and verifying `g_zUtil_ZbdManager` with local VC5
    data-symbol evidence.
  - The Player weapon-bank/Mines ZAR dependency owner
    `battlesport_gameplay.player_weapon_bank_mines_zar` is now linked and
    accepted for source/data/functional gates. 0x439540, 0x43c950, 0x43cc70,
    and 0x43cdf0 are accepted at tier B after the zInput game-state pointer
    data owner, Player weapon/Mines rdata VC5 targets, registered native
    smokes, and focused source-shape/original-symbol guard checks passed.
    0x439600 remains accepted at tier B. 0x438ba0
    `Player::LoadWeaponBanksAndSelectDefaults` is now accepted at tier B after
    linking the accepted Player weapon-bank/Mines owner and accepted zInput and
    Player literal data owners; its `Reconstructed` marker remains limited
    (`☑️`) only for the documented BN HLIL pointer-cursor limitation. Refreshed
    `frontier 0x420d10 --depth 1 --lane binary` then routed to 0x420d10
    `Player::InitStateFromNameAndMasterCommonData` itself; a later refreshed
    frontier exposed remaining direct-callee owner/data blockers including
    0x4727f0 `zMath::Vec3NormalizeXZ`, 0x403510 `AINet::FindByNetId`,
    0x4390d0 `Player::CacheGunHardpointsAndDetachDisplays`, and 0x438920
    `HudUiMgrSensor::TrackList_Add`. 0x4727f0 is now accepted at tier B after
    linking `engine.zmath.vec3_normalize_xz_helper`, adding accepted data-owner
    `engine.zmath.vec3_normalize_xz_constants` for g_zMath_Vec3UnitFloat and
    g_zMath_DoubleZero, passing `zmath_vec3_normalize_xz`, and verifying the
    two constants with `zmath_vec3_normalize_xz_constants`; tier S remains
    deferred by its known function-byte mismatch. 0x403510
    `AINet::FindByNetId` is now accepted at tier B after linking
    `battlesport_ai.ainet_find_by_net_id_lookup`, accepting data-owner
    `battlesport_ai.ainet_global_list_head_data` for g_AINetListHead, and
    rerunning functional/VC5 function/data-symbol evidence. Current refreshed
    frontier recommends 0x4390d0
    `Player::CacheGunHardpointsAndDetachDisplays` as the next visible
    source-owner blocker. 0x4390d0 is now accepted at tier B after clearing
    lower zClass data gates for 0x447bc0 and 0x447f00 through accepted
    `engine.zclass.class_error_strings`, linking
    `battlesport_gameplay.player_gun_hardpoint_cache`, and accepting
    `battlesport_gameplay.player_gun_hardpoint_name_strings` with
    `player_gun_hardpoint_name_strings` VC5 data-symbol evidence. 0x438920
    `HudUiMgrSensor::TrackList_Add` is now accepted at tier B after linking
    `hud_ui.hud_ui_mgr_sensor_track_list_append`, accepting the complete
    `hud_ui.hud_ui_mgr_sensor_track_list_global` BSS data owner,
    and rerunning `hud_ui_mgr_sensor_track_list_add` functional evidence. The
    refreshed `frontier 0x420d10 --depth 1 --lane binary` now reports only
    tier-S-only direct-callee debt for this visible slice, so it is deferred by
    the global owner/data policy. The durable
    `battlesport_gameplay.player_create_from_names_bootstrap` owner has been
    pruned with supported owner CLI commands: the GameNet pkt06 caller and zEffect
    0x45d930 are no longer members, the local Player bootstrap methods
    0x421ed0/0x4220f0/0x421830 are now linked, and shared aim-origin constants
    were moved to
    `battlesport_gameplay.player_shared_aim_origin_rdata`.
    `player_bootstrap_globals`, `player_bootstrap_runtime_globals`,
    `player_shared_aim_origin_rdata`,
    `optcatalog_runtime_callback_globals`,
    `zclass_player_runtime_di_scene_global`, and
    `czrecoilframe_hud_sensor_tracker_global` provide current VC5 data-symbol
    evidence for the modal/common list globals, `g_Player_NextOrdinal`,
    `g_Player_LocalFxOffsetWorldPtr`, `g_Player_NominalGravity`,
    `kPlayerDefaultAltGunAimOrigin`, `g_OptCatalogDamageFeedbackTrackedNode`,
    `g_Player_RuntimeDiScene`, and `g_HudSensorTracker`. The shared aim-origin
    owner `battlesport_gameplay.player_shared_aim_origin_rdata` is accepted
    after BN xref review, source-shape repair, passing 0x420d10/0x43b500
    functional targets, and zero-mismatch 12-byte VC5 data-symbol evidence.
    The Player alt-gun aim-direction owner
    `battlesport_gameplay.player_alt_gun_aim_direction` is now accepted for
    boundary/source/data/functional gates. It covers 0x43b1b0, 0x43b3e0,
    0x43a4f0, 0x43a600, and 0x43b500 after docblock repair, current
    functional evidence for all five members, accepted zMath and OptCatalog
    dependency gates, and existing Player aim-origin/time data owners; these
    entries are tier B with tier S deferred.
    The broader `battlesport_gameplay.player_create_from_names_bootstrap`
    owner is now accepted for boundary/source/data/functional gates after the
    assigned bootstrap docblocks were repaired in `player.cpp`, all ten owner
    member functional targets passed, and the Player bootstrap/save-state list,
    runtime-global, shared aim-origin, nominal-gravity, local-FX pointer, and
    runtime-DI-scene data-symbol targets passed with zero unmasked data-byte
    mismatches. Plan entries 0x420d10, 0x421a40, 0x421470, 0x421790,
    0x421830, 0x421ab0, 0x421ea0, 0x421ed0, 0x4220f0, and 0x42aa40 are
    accepted at tier B; tier S remains deferred for the coherent Player
    bootstrap/source-cluster pass.
