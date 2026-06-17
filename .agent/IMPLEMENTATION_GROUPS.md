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
- Use `python tools/recoil.py audit handoff --path .agent/IMPLEMENTATION_GROUPS.md
  --strict` before launching workers from live handoff blocks.

## Active Group Template

```text
### Group: short descriptive name

- Anchor: 0xNNNNNN Name
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

- Ready owner/data work: GameNet launch/session-sync and Player
  create-from-names bootstrap owner/data cleanup remain actionable through
  shared 0x420d10/0x432860 blockers.
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
- Queue: active owner/data cleanup for the GameNet launch/session-start
  dependency cluster under HudUiNetGameSetupPanel.
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
  - 0x431c50 source/data audit confirms the registration body is source-shaped
    and its functional target passes. Local VC5 data-symbol targets now cover
    the zDEClient relay callback slots and OptCatalog runtime callback slots
    with zero unmasked mismatches, in addition to the existing GameNet,
    zEffect activation-dispatch, and zNetwork dispatch-list data evidence.
    Keep 0x431c50 owner/data pending until the installed callback owner band is
    accepted; do not reroute this as missing callback-slot data evidence.
- Current blockers:
  - 0x4321b0 is accepted at tier B after registering/rerunning the existing
    packet-handler unregister smoke, documenting the paired 0x431c50
    registration function, accepting the GameNet registration subsystem owner
    for unregister, and reusing `gamenet_launch_session_globals` VC5
    data-symbol evidence for `g_GameNet_HandlersRegistered`. 0x4320f0 is
    accepted at tier B after resolving the HUD row-removal chain, accepting
    `g_HudUiTopMessageStack` with local VC5 data-symbol evidence, and reusing
    `gamenet_launch_session_globals` for the player-row and spawn-point list
    globals. 0x431c50 remains data-blocked by zDEClient/OptCatalog/zEffect
    callback-slot globals and source-blocked by zNetwork/zEffect registration
    helpers.
    0x432830 is accepted at tier B after row-list data-symbol evidence;
    0x431c50, 0x4327e0, 0x432860, and 0x432ae0 still need broader
    pkt06/player/HUD/zVideo and registered-callback owner-data routing before
    tier B.
  - The pkt06 data correction sets `g_GameNetPkt06InitialSyncGate` to the BN
    initial value 1. Ignored local VC5 target `gamenet_pkt06_globals` now
    covers `g_GameNetPlayerRowStyleColors_00RRGGBB`,
    `g_GameNetPkt06InitialSyncGate`, and `g_HudTimerPanelNetState` with zero
    unmasked mismatches, but callers that touch `g_HudUiTopMessageStack`,
    `g_Player_RuntimeDiScene`, `g_HudSensorTracker`, or `g_zVideo_FrameTick`
    remain cross-owner data blocked.
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
  - The Player clone helper dependency 0x421a40 is accepted at tier B after
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
    tier B after documenting the data-driven child-link subsystem owner,
    wiring the existing add-child smokes into `recoil_native_smoke`, adding CRT
    provider-boundary entries for `realloc` and `calloc`, accepting
    `g_zError_DebugMsgBuffer` through zError::EmitDebugBuffer data evidence,
    and clearing the AddChildGeneric/SetSingleParentFlagRecursive data gates.
  - Route zNetwork send/session-desc helpers and HUD row-removal/container
    dependencies as separate owner/data blockers; do not fold them into the
    GameNet owner.
  - Route the remaining 0x4143c0/0x40e880 data gates through the HUD
    stats-list and `HudUiTriplet::RebuildDisplay` owner/data slices. Do not
    broaden into the unrelated `zhud_ui.cpp` docblock backlog.
  - The Player bootstrap frontier routes through 0x420d10 to 0x42aa40
    `Player::GetSaveStateListHead`; treat the save-state list globals as a
    Player save-state/bootstrap record-global subsystem before promoting the
    launch caller. 0x41ec00 and 0x42aa40 are now accepted at tier B after
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
- Next action:
  - Continue at the GameNet registered-callback owner band. Start with
    `python tools/recoil.py status 0x4327e0 --lane binary`, then inspect
    `python tools/recoil.py frontier 0x4327e0 --depth 1 --lane binary` before
    assigning a source worker. Keep OptCatalog, Pickup, zDEClient, and zEffect
    callback targets routed to their own owner sections rather than absorbing
    them into the GameNet source-file owner.

### Group: Player create-from-names bootstrap owner-data

- Anchor: 0x421ab0 Player::CreateFromNamesAtPose, with wrapper 0x421ea0
  Player::CreateFromNamesAtPoseGetState.
- Section: battlesport_gameplay
- Queue: ready owner/data work; dependency slice for the GameNet pkt06 remote
  spawn path through 0x432860.
- Reason: Player class bootstrap/save-state creation owner and touched
  Player/HUD/zClass/zEffect data gates block GameNet pkt06 tier B promotion.
- Current evidence:
  - `python tools/recoil.py frontier 0x432860 --depth 1 --lane binary`
    recommends 0x421ea0, and `frontier 0x421ea0` routes directly to
    0x421ab0.
  - 0x421ab0 has accepted dependencies for zOpt network mode, zClass type/name
    lookup and clone helpers, Object3D pose setters, zClass AddChild,
    Player::CloneType6NodeFromTemplateAndRename, and zUtil save-state-list
    allocation/append helpers.
  - Remaining visible blockers under 0x421ab0 include 0x420d10
    `Player::InitStateFromNameAndMasterCommonData`,
    Player modal/spawn/hit/destroyed-state helpers, zClass damage/camera/material
    helpers, HudSensorTracker::SetTrackedSaveState, OptCatalog damage-mask
    lookup, and zEffectAnim::FindEntryByName.
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
    `gwListDeleteANode` are adjacent deletion-path work, not required for the
    0x44f000 owner gate. Data classification proves 0x44f000 is not
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
    with local VC5 data-symbol evidence. Local VC5 data-symbol evidence also
    confirms the shared Time frame-delta global `g_FrameDeltaTimeSec`, but the
    remaining zEffect velocity/stop route still needs the
    recursive 0x45c040 `Stop` / 0x45d3d0 `FinalizeStop` /
    0x45d770 `RunStopDelayCallback` / 0x45d6b0 `NodeActionCallback` /
    0x45d570 `StopAndCleanup` owner loop plus 0x45d4c0
    `RunStopSequenceCallback`, 0x45cc00 `RunSequenceEvents`/event-dispatch
    ownership, 0x45d6c0 `ResetForNode` through external 0x449ab0
    `gwNodeGetRoot`, and the zClass sound/light/camera setter gates before
    0x45d930/0x45dcb0/0x45dde0 promotion.
    The zEffect runtime-ref creation helpers 0x45e380
    `FindOrCreateSoundRef` and 0x45e4a0 `FindOrCreateLightRef` are accepted
    at tier B after registering their shared native smoke and verifying the
    zeff_anim_init.c source-path plus sound/light overflow-format rdata with
    local VC5 data-symbol evidence. zClass 0x452d00
    `zClass_Sound::gwSoundSetPosition` is also accepted at tier B after the
    existing sound leaf smoke passed and the Sound.c diagnostic/source rdata
    target reverified.
- Next action:
  - Start with `python tools/recoil.py status 0x420d10` and
    `python tools/recoil.py frontier 0x420d10 --depth 1 --lane binary`.
    The visible route through 0x45dde0 `SetVelocity_Thunk`, 0x45dcb0
    `SetVelocity`, 0x45d930 `ActivateRuntime`, and 0x45e730
    `CloneEntryForNode` has cleared 0x452fd0 `zClass_Light::gwLightNew`.
    The 0x4390d0 `Player::CacheGunHardpointsAndDetachDisplays` slice is now
    accepted at tier B after BN/source owner review, functional smoke coverage,
    original-helper guard, and local VC5 data-symbol evidence for the four
    hardpoint-name strings.
  - Current 0x420d10 frontier now routes to 0x438ba0
    `Player::LoadWeaponBanksAndSelectDefaults`. 0x4b1f90
    `OptCatalog::FreeTrailRuntimeStateStorage` is now accepted at tier B after
    adding the required provenance docblock, rerunning functional evidence, and
    accepting no-authored-globals data; tier S remains blocked by the known
    VC5 free-call byte diff. 0x438b60
    `Player::FreeAltWeaponTrailRuntimeStates` is also accepted at tier B after
    BN/source owner review, functional coverage, and no-authored-globals
    review.
  - Continue 0x438ba0 through 0x43ca90
    `Player::CheckMissionWeaponAvailability`, now the lowest visible owner
    blocker. Keep its remaining OptCatalog and zUtil callees routed to their
    own owner sections rather than absorbing them into the Player class pass.
