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

