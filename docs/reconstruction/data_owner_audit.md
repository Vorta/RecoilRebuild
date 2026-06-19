# Data Owner Audit

`Data reimplemented` is a whole-owner gate. A function or source group can keep
`Data reimplemented ✅` only when every touched authored `.data`, `.rdata`, and
BSS owner has complete source data-owner evidence. Field-level recovery inside a
larger global, source macros, static offset asserts, and functional smokes are
not enough.

The plan's data-entry progress rows are narrower: they track canonical `.data`
owner-range globals only. Do not add `.rdata` entries, BSS-only entries, or
member/field rows inside a larger global. For example, `0x4f0cc0
g_HudSensorTracker` is one data plan entry for the owner range, not one row per
BN-labeled member. `.rdata` and BSS facts still belong in the source-owner data
gate evidence when a function's `Data reimplemented` marker depends on them.

## Acceptance Packet

For each accepted data owner, record:

- owner symbol, BN address/range, section, size, alignment, and source symbol;
- full storage extent, including adjacent fields that BN labels separately;
- initializer/lifecycle behavior for BSS or runtime-initialized records;
- initialized byte facts: order, strings, constants, GUIDs, flags, table slots,
  pointer/symbol identity, and nullness;
- VC5 `data_symbols` output when available, including relocation identity
  review for pointers;
- caller/function entries whose `Data reimplemented ✅` depends on this owner.

For each plan-tracked `.data` global definition in source, put an immediately
preceding docblock:

```cpp
/**
 * Reimplements data 0xNNNNNN: g_Symbol.
 * Purpose: Describes the source-level role of this data owner.
 */
```

If any item is missing, dependent functions must use `Data reimplemented ❌`.
If current BN/source evidence proves no authored globals are touched, use
`Data reimplemented ❎`.

## Current Audit Baseline

The 2026-06-17 data-gate hardening pass found 751 plan entries marked
`Data reimplemented ✅`. Existing VC manifests contain 415 `data_symbols`
entries across 103 manifests, so most green data markers cannot be justified by
data-symbol evidence alone.

The same pass downgraded 718 stale `Data reimplemented ✅` markers to `❌`
through `python tools/recoil.py plan set ... data ❌`. The dry run succeeded
for the 717 entries that lacked any `data_symbols` target coverage, then the
same updates were applied. A later verification pass found that
`0x4bab40 HudUiPanel::~HudUiPanel` did not emit an accepted data-symbol evidence
block for its target, so it was also downgraded.

The remaining `Data reimplemented ✅` entries are limited to entries whose VC5
verification target emitted data-symbol evidence with zero unmasked data-byte
mismatches during this audit. Several of those full target commands still
returned nonzero because unrelated function byte comparisons in the same
manifest drifted; that does not by itself preserve tier `S` evidence.

| Address | Target | Group | Name |
| --- | --- | --- | --- |
| 0x4bffe0 | zutil_zar_register_section_handler | engine.zutil | zUtil_ZAR::RegisterSectionHandler |
| 0x453400 | zclass_light_set_range | engine.zclass.core | zClass_Light::gwLightSetRange |
| 0x451240 | zclass_world_remove_child_at_grid | engine.zclass.core | zClass_World::RemoveChildAtGrid |
| 0x451410 | zclass_world_remove_light | engine.zclass.core | zClass_World::RemoveLight |
| 0x451640 | zclass_world_remove_sound | engine.zclass.core | zClass_World::RemoveSound |
| 0x471de0 | zinput_poll_active_devices | engine.zinput | zInput::PollActiveDevices |
| 0x46f690 | zinput_keyboard_poll_state | engine.zinput | zInput::Keyboard_PollState |
| 0x46fa10 | zinput_keyboard_wait_for_any_key_press | engine.zinput | zInput::Keyboard_WaitForAnyKeyPress |
| 0x4702e0 | zinput_mouse_get_button_transition_state | engine.zinput | zInput::Mouse_GetButtonTransitionState |
| 0x470310 | zinput_mouse_update_acquire_state | engine.zinput | zInput::Mouse_UpdateAcquireState |
| 0x4704f0 | zinput_mouse_apply_accumulated_delta | engine.zinput | zInput::Mouse_ApplyAccumulatedDelta |
| 0x471fb0 | zinput_joystick_acquire_device | engine.zinput | zInput::DI_AcquireJoystickDevice |
| 0x4760d0 | zmodel_set_di_texture_world_per_meter | engine.zmodel | zModel::SetDiTextureWorldPerMeter |
| 0x4a08d0 | zsnd_sample_set_registry_get_count | engine.zsound | zSndSampleSetRegistry_GetByIndex |
| 0x4a0900 | zsnd_sample_set_registry_get_count | engine.zsound | zSndSampleSetRegistry_GetCount |
| 0x4a44c0 | zsnd_pending_list_find_by_name | engine.zsound | zSndPendingList_FindByName |
| 0x4a3ea0 | zsnd_report_mci_error | engine.zsound | zSnd::ReportMciError |
| 0x489f70 | znetwork_get_local_player_key | engine.znetwork | zNetwork::GetLocalPlayerKey |
| 0x46d5c0 | zvid_texture_pack_load_state_getter | engine.zvideo | zVid::GetTexturePackLoadState |
| 0x492000 | zrndr_rasterize_poly_with_span_list | engine.zrndr | zRndr_RasterizePolyWithSpanList |
| 0x49a910 | zrndr_lens_flare_reset_sample_queue | engine.zrndr | zRndr::LensFlare_ResetSampleQueue |
| 0x45db20 | zeffect_anim_check_activation_prereqs | engine.zeffect | zEffectAnim::CheckActivationPrereqs |
| 0x45e730 | zeffect_anim_clone_entry_for_node | engine.zeffect | zEffectAnim::CloneEntryForNode |
| 0x406d20 | hud_ui_cheat_code_dialog_constructor | ui.zhud | HudUiCheatCodeDialog::HudUiCheatCodeDialog |
| 0x40eab0 | hud_scoreboard_set_scale_and_rebuild | ui.zhud | HudScoreboard::SetScaleAndRebuild |
| 0x4bb790 | hud_ui_composite_panel_constructor_with_entry_count | ui.zhud | HudUiCompositePanel::ConstructorWithEntryCount |
| 0x4143a0 | hud_ui_mgr_is_local_player_first_in_stats_list | ui.zhud | HudUiMgr::IsLocalPlayerFirstInStatsList |
| 0x419650 | hud_ui_mp_exit_dialog_table_cluster | ui.zhud | HudUiMpExitDialog::UnloadLayout |
| 0x419690 | hud_ui_mp_exit_dialog_table_cluster | ui.zhud | HudUiMpExitDialog::Update |
| 0x419800 | hud_ui_mp_exit_dialog_table_cluster | ui.zhud | HudUiMpExitDialog_MpNewGameButton::OnActivate |
| 0x419830 | hud_ui_mp_exit_dialog_table_cluster | ui.zhud | HudUiMpExitDialog_MpExitButton::OnActivate |
| 0x419870 | hud_ui_mp_exit_dialog_table_cluster | ui.zhud | HudUiMpExitDialog::Destructor |
| 0x4622f0 | zerror_emit_debug_buffer | misc.authored_stubs | zError::EmitDebugBuffer |

Known false/stale pattern:

- `0x4132b0 HudLayoutHW::UpdateObjectiveDirtyRect` touches `g_HudUiMgr`
  objective and nanite fields. The plan had `Data reimplemented ✅` and tier
  `B`, but the complete `g_HudUiMgr`/`HudUiMgrData` owner is not accepted:
  `HudUiMgr::Constructor` and `HudUiMgr::SetNanitePanelCount` remain
  owner/data-blocked, and no VC data-symbol manifest covers the complete owner.
The correct state is data-blocked until the full HUD manager data owner is
recovered and verified.

Future data-owner acceptances should append compact entries here rather than
relying only on per-function plan markers.

## 2026-06-18 Accepted Data Owners

### engine.zgame.zopt_network_options

- Owner symbol/scope: zOpt network option pointer globals used by the
  network-enabled, network-modem, and network-listen accessors in
  `src/GameZRecoil/zGame/zGame.cpp`.
- BN data: `g_zOpt_NetworkEnabledOption` at 0x4e5d74,
  `g_zOpt_NetworkListenOption` at 0x4e5d78, and
  `g_zOpt_NetworkModemOption` at 0x4e5d90.
- Source symbols: `ZOPT_NETWORK_ENABLED`,
  `g_zOpt_NetworkListenOption`, and `g_zOpt_NetworkModemOption`.
- Extent/section/nullness: each owner item is an independent 4-byte pointer
  global with zero-initialized bytes in BN/source; no adjacent field slice is
  being accepted as part of a larger struct.
- Lifecycle/xrefs: 0x407700 `zGame::Options_LoadGameOptions` initializes the
  pointers from `Options_GetOrCreateOption`; 0x408230/0x408240/0x408250 store
  through them; 0x408260/0x408270 read through them.
- VC5 evidence: `python tools/recoil.py verify vc5
  zopt_network_option_globals` passed with zero unmasked data-byte mismatches
  for 0x4e5d74, 0x4e5d78, and 0x4e5d90 using
  `vc5_o2_ob0_md_facs`.
- Dependent plan entries: 0x408230, 0x408240, 0x408250, 0x408260, and
  0x408270.

### engine.zclass.typelist_find_by_type_and_name

- Owner symbol/scope: zClass type-list exact-name lookup data used by
  0x44ecf0 `zClass::FindByTypeAndName` in
  `src/GameZRecoil/zClass/List.c`.
- BN data: `g_zClass_TypeList_HeadSlotPtrs` at 0x4ddef8 and
  `g_zClass_TypeList_Buckets` at 0x539bac.
- Source symbols: `g_zClass_TypeList_HeadSlotPtrs` and
  `g_zClass_TypeList_Buckets`.
- Extent/section/nullness: the head-slot pointer table is 64 bytes and points
  into the recovered 192-byte `zClass_TypeListBucket[16]` aggregate; the
  bucket aggregate is accepted as the full backing owner, not a field slice.
- Lifecycle/xrefs: 0x44ecf0 reads the selected head-slot pointer and walks the
  linked bucket chain without mutating global state; adjacent type-list
  mutators remain separate owner work.
- VC5 evidence: `python tools/recoil.py verify vc5
  zclass_find_by_type_and_name_data` passed with zero unmasked data-byte
  mismatches for 0x4ddef8 and 0x539bac using `vc5_o2_ob0_md_facs`.
- Dependent plan entries: 0x44ecf0.

### engine.zclass.copy_node_clone_options

- Owner symbol/scope: zClass copy-node clone-option globals used by
  `CopyNodeDisplayInstance`, `CopyNodeWithCloneOptions`, and `CopyNode` in
  `src/GameZRecoil/zClass/cls_util.c`; `CopyNodeDispatch` is included as the
  immediate wrapper dependency for 0x452500.
- BN data: `g_zClass_CopyNodeCloneDiMode` at 0x4de4cc,
  `g_zClass_CopyNodeDiArg0` at 0x539c9c, and `g_zClass_CopyNodeDiArg1` at
  0x539ca0.
- Source symbols: `g_zClass_CopyNodeCloneDiMode`,
  `g_zClass_CopyNodeDiArg0`, and `g_zClass_CopyNodeDiArg1`.
- Extent/section/nullness: each owner item is an independent 4-byte `int`
  global; BN/source initial bytes are 1 for clone mode and zero for both DI
  arguments.
- Lifecycle/xrefs: 0x451b20 reads the clone-option globals while cloning or
  reusing display instances; 0x452500 saves, installs, and restores clone mode
  plus DI arg 0 around dispatch; 0x452560 saves, installs, and restores all
  three globals around dispatch.
- VC5 evidence: `python tools/recoil.py verify vc5 zclass_copy_node_globals`
  passed with zero unmasked data-byte mismatches for 0x4de4cc, 0x539c9c, and
  0x539ca0 using `vc5_o2_ob0_md_facs`.
- Dependent plan entries: 0x451b20, 0x452400, 0x452500, and 0x452560.

### battlesport_gameplay.player_create_from_names_bootstrap

- Owner symbol/scope: Player create-from-names bootstrap/save-state record
  owner in `src/Battlesport/player.cpp`.
- BN/source data: Player master-modal list globals at 0x4f3688..0x4f3694,
  master-common list globals at 0x4f3a68..0x4f3a74, save-state list globals at
  0x4f3a78..0x4f3a84, `g_Player_NextOrdinal` at 0x4f3a94,
  `g_Player_NominalGravity` at 0x4f3ac8, `g_Player_LocalFxOffsetWorldPtr` at
  0x779aa8, accepted shared aim-origin rdata at 0x4dc998, and accepted
  `g_Player_RuntimeDiScene` at 0x4f36b8.
- Extent/section/nullness: the list heads, tails, aux pointers, counts,
  ordinal, gravity, local-FX pointer, and runtime-DI-scene symbols are
  independent 4-byte globals; the aim-origin owner is a 12-byte rdata vector.
- Lifecycle/xrefs: the accepted owner covers the bootstrap member functions
  that create, bind, initialize, and return Player save-state records from
  template/object names while updating the accepted list/runtime globals.
- VC5 evidence: `python tools/recoil.py verify vc5 player_bootstrap_globals`,
  `player_save_state_globals`, `player_bootstrap_runtime_globals`,
  `player_shared_aim_origin_rdata`, and
  `zclass_player_runtime_di_scene_global` passed with zero unmasked data-byte
  mismatches for their data symbols using `vc5_o2_ob0_md_facs`.
- Dependent plan entries: 0x420d10, 0x421a40, 0x421470, 0x421790, 0x421830,
  0x421ab0, 0x421ea0, 0x421ed0, 0x4220f0, and 0x42aa40.

### engine.zsound.option_runtime_globals

- Owner symbol/scope: zSound mute and global-volume option runtime globals
  shared by preinitialization, mute-state playback, snapshot capture/restore,
  and global-volume helpers.
- BN/source data: `g_zSnd_MuteOptionDefault` at 0x56b3b4,
  `g_zSnd_MuteOptionValuePtr` at 0x56b3b8, `g_zSnd_MuteDepth` at
  0x56b3bc, `g_zSnd_VolumeScaleDefault` at 0x56b3c0, and
  `g_zSnd_GlobalVolumeScalePtr` at 0x56b3c4.
- Extent/section/nullness: each accepted item is an independent 4-byte global;
  the defaults and depth are zero-initialized data, and the option pointers are
  zero-initialized pointer globals until `zSnd_PreInitializeRuntimeState`
  binds them to the game option table or local defaults.
- Lifecycle/xrefs: 0x4a12c0 initializes mute/default volume state and option
  pointers; 0x4a0670/0x4a07a0 consume mute depth/options; 0x49fff0/0x4a0590
  snapshot and restore global-volume state through the volume pointer; 0x4a1090
  and 0x4a10b0 update global volume through the same pointer.
- VC5 evidence: `python tools/recoil.py verify vc5 0x56b3b4`,
  `0x56b3b8`, `0x56b3bc`, `0x56b3c0`, and `0x56b3c4` resolved to
  `zsnd_preinitialize_runtime_state` and passed with zero unmasked data-byte
  mismatches under `vc5_o2_ob1_md_gx_facs`.
- Dependent plan entries: 0x4a0300, 0x49fff0, 0x4a0590, 0x4a0670, and
  0x4a07a0.

### engine.zsound.zsnd_play_rdata_literals

- Owner symbol/scope: immutable `zsnd_play.cpp` rdata literals used by
  PlayWithDelta error reporting and gain/attenuation comparisons.
- BN/source data: `g_zSnd_SourceFile_ZsndPlayCpp` at 0x4e2208,
  `g_zSnd_PlayWithDeltaA3D_ZeroFloat` at 0x4d2ebc,
  `g_zSnd_PlayWithDeltaA3D_DeltaScale` at 0x4d2ec0, and
  `g_SndConst_dZero` at 0x4d2ec8.
- Extent/section/nullness: the source-file string is a 41-byte immutable rdata
  string; the remaining entries are compiler-emitted immutable rdata literals
  for `0.0f`, `10000.0f`, and double `0.0`.
- Lifecycle/xrefs: 0x4a0380 uses the source-file string and double-zero
  compare for A3D replay error/gain handling; 0x4a0400 uses the source-file
  string for DirectSound provider error reports; 0x4a0490 uses the float-zero
  and 10000.0f constants for backend dispatch gates and DirectSound attenuation
  conversion.
- VC5 evidence: `python tools/recoil.py verify vc5 0x4e2208`,
  `0x4d2ebc`, `0x4d2ec0`, and `0x4d2ec8` resolved to
  `zsnd_play_with_delta_backend_dispatch` and passed with zero unmasked
  data-byte mismatches under `vc5_o2_ob1_md_gx_facs`.
- Dependent plan entries: 0x4a0380, 0x4a0400, 0x4a0490, and 0x4a0590.
