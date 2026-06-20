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

Data entries use `Reimplemented [X/F/C/B/A/S]`: `X` means no accepted
source-level data implementation, `F` means the canonical source definition or
declaration builds and matches basic BN symbol facts, `C` means the complete
source data-owner model is accepted, `B` means the linked owner data gate is
accepted, `A` means reviewed relocation-masked data-symbol evidence is
near-byte-equivalent, and `S` means accepted data-symbol bytes plus relocation
identity and linked owner byte gate acceptance.

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

## 2026-06-21 Accepted Data Owners

### render_video.zrndr_overlay_rect_staging_globals

- Owner symbol/scope: zRndr overlay rectangle buffered draw globals in
  `src/GameZRecoil/zRndr/zRndr.cpp`.
- BN/source data: `gRndr_OverlayBlendEnabled` at 0x62e9dc,
  `gRndr_OverlayBlendRectLeft` at 0x62e9e0,
  `gRndr_OverlayBlendRectTop` at 0x62e9e4,
  `gRndr_OverlayBlendRectRight` at 0x62e9e8,
  `gRndr_OverlayBlendRectBottom` at 0x62e9ec,
  `gRndr_OverlayBlendPackedColor16` at 0x62e9f0, and
  `gRndr_OverlayBlendAlpha` at 0x62e9f8.
- Extent/section/nullness: the accepted bank spans 0x62e9dc..0x62e9ff.
  The scalar fields are 4-byte zero-initialized values, 0x62e9f4 is an
  unreferenced alignment gap, and the alpha value is an 8-byte double whose
  high dword overlaps stale BN field views at 0x62e9fc.
- Lifecycle/xrefs: overlay submit writes the bank, overlay flush reads it, and
  lens-flare draw paths read the buffered enabled/color/alpha state. This data
  owner is independent of the unresolved ESP-pivot span family and does not
  clear the separate overlay MMX row source-shape blocker.
- VC5 evidence: `python tools/recoil.py verify vc5
  zrndr_overlay_rect_staging_globals` passed with zero unmasked data-byte
  mismatches for all seven non-padding data symbols using
  `vc5_o2_ob0_md_facs`.
- Dependent plan entries: none promoted in this pass; shared overlay
  row/submit/flush entries remain owner/source-blocked.

### network_online.gamenet_net_zrd_path_literal

- Owner symbol/scope: GameNet mission startup `net.zrd` path literal in
  `src/Battlesport/GameNet.cpp`.
- BN/source data: `g_GameNet_NetZrdPathLiteral` at 0x4dcfb4.
- Extent/section/nullness: independent 8-byte null-terminated `.rdata`
  string literal.
- Lifecycle/xrefs: 0x431dd0 `Net::InitFromZrd` passes the literal to the
  zReader load path.
- VC5 evidence: `python tools/recoil.py verify vc5
  gamenet_net_zrd_path_literal` passed with zero unmasked data-byte mismatches
  for the generated VC5 string symbol using
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`.
- Dependent plan entries: data entry 0x4dcfb4 was accepted; 0x431dd0 remains
  blocked on broader GameNet owner/data routing.

### network_online.gamenet_pkt06_local_replication_data

- Owner symbol/scope: GameNet local pkt06 replication snapshot buffer and input
  latch globals in `src/Battlesport/GameNet.cpp`.
- BN/source data: `g_NetPkt06_PlayerStateSnapshotBuf` at 0x4dcdb0,
  `g_GameNetPkt06InputBit17Latch` at 0x4f3f6c, and
  `g_GameNetPkt06InputBit16Latch` at 0x4f3f70.
- Extent/section/nullness: the pkt06 snapshot buffer is a 192-byte initialized
  record; the two latch globals are independent 4-byte zero-initialized BSS
  values.
- Lifecycle/xrefs: 0x432300 fills/sends the pkt06 snapshot and consumes/clears
  both input latches during local-player replication.
- VC5 evidence: `python tools/recoil.py verify vc5
  gamenet_pkt06_local_replication_data` passed with zero unmasked data-byte
  mismatches for all three data symbols using
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`.
- Dependent plan entries: data entries 0x4dcdb0, 0x4f3f6c, and 0x4f3f70 were
  accepted; 0x432300 remains below promotion because its function
  reconstruction/source-owner routing is broader than this data owner.

### network_online.gamenet_startgate_effect_literal

- Owner symbol/scope: GameNet startgate effect-name literal in
  `src/Battlesport/GameNet.cpp`.
- BN/source data: `g_GameNet_StartGateEffectLiteral` at 0x4dcfbc.
- Extent/section/nullness: independent 10-byte null-terminated `.rdata`
  string literal.
- Lifecycle/xrefs: 0x432300 uses the literal when looking up the startgate
  effect during pkt06/HUD timer replication.
- VC5 evidence: `python tools/recoil.py verify vc5
  gamenet_startgate_effect_literal` passed with zero unmasked data-byte
  mismatches using `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`.
- Dependent plan entries: data entry 0x4dcfbc was accepted; 0x432300 remains
  separately source-owner/reconstruction gated.

### shared.fatal_shutdown_printf_literal

- Owner symbol/scope: shared fatal shutdown printf format literal pooled across
  `src/Battlesport/GameNet.cpp`, `src/Battlesport/RecoilApp.cpp`, and
  `src/Battlesport/HudUiMpExitDialog.cpp`.
- BN/source data: `g_SharedFatalShutdownPrintfFormat` at 0x4db4ac.
- Extent/section/nullness: independent 8-byte null-terminated `.rdata`
  string literal.
- Lifecycle/xrefs: GameNet mission init failure, RecoilApp fatal exit, and
  HudUi multiplayer exit dialog failure paths all reference this pooled
  literal before calling `printf`.
- VC5 evidence: `python tools/recoil.py verify vc5
  shared_fatal_shutdown_printf_literal` passed with zero unmasked data-byte
  mismatches from the GameNet translation unit using
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`.
- Dependent plan entries: data entry 0x4db4ac was accepted; function-level
  promotions remain controlled by their respective source-owner gates.

### app_shell.czrecoil_frame_mfc_runtime_message_maps

- Owner symbol/scope: CZRecoilFrame/CZGameFrame MFC runtime-class and
  message-map metadata in `src/Battlesport/CZRecoilFrame.cpp` and
  `src/Battlesport/CZGameFrame.cpp`.
- BN/source data: `g_CZRecoilFrame_RuntimeClass` at 0x4d0bf0,
  `g_CZRecoilFrame_MessageMap` at 0x4d0c08,
  `g_CZRecoilFrame_MessageEntries` at 0x4d0c10,
  `g_CZRecoilFrame_RuntimeClassName` at 0x4dccf0,
  `g_CZGameFrame_RuntimeClass` at 0x4d20e0,
  `g_CZGameFrame_MessageMap` at 0x4d20f8,
  `g_CZGameFrame_MessageEntries` at 0x4d2100, and
  `g_CZGameFrame_RuntimeClassName` at 0x4dd8dc.
- Extent/section/nullness: the runtime-class records are 24-byte
  `CRuntimeClass` records; the message maps are 8-byte `AFX_MSGMAP` records;
  the message-entry tables are 1320-byte and 216-byte `AFX_MSGMAP_ENTRY`
  arrays with terminal zero entries; the class-name strings are 14-byte and
  12-byte null-terminated `.data` string COMDATs. BN typing for
  `g_CZGameFrame_RuntimeClass` was corrected from a 4-byte pointer view to
  `zMfcRuntimeClass24` and saved.
- Provider exclusions: MFC42 CFrameWnd runtime-class/message-map pointers at
  0x4cc25c and 0x4cc260 are provider data, not authored app-shell data.
  CZRecoilFrame/CZGameFrame vtables at 0x4d1140 and 0x4d21d8 remain separate
  class/vtable owner work.
- VC5 evidence: `python tools/recoil.py verify vc5 czframe_mfc_metadata` and
  `python tools/recoil.py verify vc5 czgame_frame_mfc_metadata` passed with
  zero unmasked data-byte mismatches for all eight linked metadata/string data
  symbols using `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`.
- Dependent plan entries: 0x4301e0, 0x443730, 0x430240, 0x4306e0, 0x443790,
  0x4437a0, 0x4437b0, and 0x4437c0. Standalone string data entries
  0x4dccf0 and 0x4dd8dc have accepted source ownership and data-equivalent
  metadata, but their `Reimplemented [S]` marker remains blocked while the
  linked owner byte gate is deferred.

### effects_weapons.zweapon_tether_config_data

- Owner symbol/scope: zWeapon maximum tether altitude runtime config global in
  `src/GameZRecoil/zWeapon/zWeapon.cpp`.
- BN/source data: `g_zWeapon_MaxTetherAltitude` at 0x779a98.
- Extent/section/nullness: independent 4-byte `.data` float initialized to
  zero.
- Lifecycle/xrefs: `zWeapon::Init` restores the startup default,
  `zWeapon::SetMaxTetherAltitude` updates the value, and OptCatalog runtime
  processing reads it for tether checks.
- VC5 evidence: `python tools/recoil.py verify vc5
  zweapon_tether_config_data` passed with zero unmasked data-byte mismatches
  for the generated `_g_zWeapon_MaxTetherAltitude` symbol using
  `vc5_o2_ob0_md_facs`.
- Dependent plan entries: supports the accepted data gate for 0x4b1090
  `zWepInit`; broader zWeapon/OptCatalog source-cluster tier S remains
  deferred.

### effects_weapons.optcatalog_warning_config_data

- Owner symbol/scope: OptCatalog warning sample pointers, crater radius
  configuration, and config-key literals in
  `src/GameZRecoil/zWeapon/OptCatalog.c`.
- BN/source data: `g_OptCatalogMaxCraterRadius` at 0x779a7c,
  `g_OptCatalogSndTriggerInactive` at 0x779a8c,
  `g_OptCatalogSndWeaponInactive` at 0x779a90,
  `g_OptCatalogSndNoAmmoWarning` at 0x779a94, plus the authored config-key
  string literals at 0x4e4548, 0x4e455c, 0x4e456c, 0x4e4580, and 0x4e4590.
- Extent/section/nullness: the four scalar globals are independent 4-byte
  `.data` values; the config keys are independent null-terminated `.rdata`
  strings used by the OptCatalog load/config path.
- Lifecycle/xrefs: the warning playback helpers at 0x4b0600, 0x4b0620, and
  0x4b0640 consume the warning sample pointers. The crater and remaining key
  users are still broader zWeapon/OptCatalog owner work and are not promoted by
  this owner acceptance alone.
- VC5 evidence: `python tools/recoil.py verify vc5
  optcatalog_warning_config_data` passed with zero unmasked data-byte
  mismatches for all nine data symbols using `vc5_o2_ob0_md_facs`.
- Dependent plan entries: 0x4b0600, 0x4b0620, and 0x4b0640 are promoted to
  tier B from this owner plus `effects_weapons.optcatalog_warning_sample_playback`.

### effects_weapons.zweapon_zar_section_data

- Owner symbol/scope: zWeapon ZAR handler registration flag and archive/section
  name literals in `src/GameZRecoil/zWeapon/zWeapon.cpp`.
- BN/source data: `g_zWeapon_ZarHandlerRegistered` at 0x4e42ec,
  `g_zWeapon_ArchiveName` at 0x4e42f0, and `g_zWeapon_DataSectionName` at
  0x4e42f8.
- Extent/section/nullness: the registration flag is independent 4-byte `.data`
  storage initialized to one; the archive and section names are independent
  null-terminated `.rdata` strings.
- Lifecycle/xrefs: the zWeapon init/load callbacks register and consume this
  ZAR section state. Broader load-path promotion remains blocked by separate
  zWeapon owner and verification gaps.
- VC5 evidence: `python tools/recoil.py verify vc5
  zweapon_zar_warning_data` passed with zero unmasked data-byte mismatches for
  all three data symbols using `vc5_o2_ob0_md_facs`.
- Dependent plan entries: no broad zWeapon init/load entry is promoted from
  this narrow data-owner acceptance in this pass.

### effects_weapons.optcatalog_runtime_tick_data

- Owner symbol/scope: OptCatalog runtime tick timing and lock-on warning
  globals in `src/GameZRecoil/zWeapon/OptCatalog.c`.
- BN/source data: `g_OptCatalogRuntimeDeltaTime` at 0x56bca8,
  `g_OptCatalogRuntimeNowSec` at 0x56bcac,
  `g_OptCatalogSndLockOnWarning` at 0x779a74, and
  `g_OptCatalogLockOnWarningGateTimeSec` at 0x779a78.
- Extent/section/nullness: each accepted item is an independent 4-byte
  zero-initialized global. The warning sample is pointer storage in source,
  while current BN exposes the same 4-byte storage as `int32_t`.
- Lifecycle/xrefs: 0x4af060 `OptCatalog::ProcessRuntimeInstances` writes the
  runtime delta/now values and consumes the lock-on warning sample/gate;
  0x4b0e20 `OptCatalog::ComputeTrailImpactResponse` consumes runtime delta
  time; `zWeapon::OnWeaponsSectionDataReady` resets the gate and the load path
  initializes the warning sample.
- VC5 evidence: address-scoped `python tools/recoil.py verify vc5` passed for
  0x56bca8, 0x56bcac, 0x779a74, and 0x779a78 through
  `optcatalog_runtime_callback_globals` using `vc5_o2_ob0_md_facs`, with zero
  unmasked data-byte mismatches.
- Dependent plan entries: 0x4af060 and 0x4b0e20.

### engine.zmath.vec3_scalar_rdata_constants

- Owner symbol/scope: zMath vector scalar read-only constants used by
  `Vec3Perp2D` and `Vec3Slerp` in
  `src/GameZRecoil/zMath/zMath.cpp`.
- BN/source data: `g_zMath_Vec3ZeroFloat` at 0x4d2918,
  `g_zMath_Vec3UnitFloat` at 0x4d291c,
  `g_zMath_Vec3DirectionDotNegThreshold` at 0x4d2930,
  `g_zMath_DirectionToPiFloat` at 0x4d2938, and
  `g_zMath_Vec3DirectionDotPosThreshold` at 0x4d2948.
- Extent/section/nullness: each accepted item is an independent `.rdata`
  scalar constant; the zero and unit values are 4-byte floats, the dot
  thresholds are 8-byte doubles, and the pi scalar is a 4-byte float.
- Lifecycle/xrefs: 0x472cc0 `zMath::Vec3Perp2D` reads zero/unit constants;
  0x472a10 `zMath::Vec3Slerp` reads zero/unit plus the three authored Slerp
  constants. The 0x4d2940 x87 trig range threshold read by Slerp is classified
  under `compiler.vc5.x87_trig_intrinsic_constants`, not authored zMath data.
- VC5 evidence: `python tools/recoil.py verify vc5
  zmath_vec3_scalar_rdata_constants` passed with zero unmasked data-byte
  mismatches for all five authored constants using `vc5_o2_ob0_md_facs`.
- Dependent plan entries: 0x472cc0 and 0x472a10.

### network_online.gamenet_mission_pkt06_timer_globals

- Owner symbol/scope: GameNet mission startup and pkt06/HUD timer scalar
  globals in `src/Battlesport/GameNet.cpp`.
- BN/source data: `g_GameNetHostHudTimerInitFlag` at 0x4f3f98,
  `g_GameNetPkt06NextSendTimeSec` at 0x4f3f9c,
  `g_GameNetHudTimerTenSecondWarningArmed` at 0x4dce70, and
  `g_GameNetHudTimerPendingSaveReminderArmed` at 0x4dce74.
- Extent/section/nullness: each accepted item is a 4-byte authored scalar;
  the two BSS values initialize to zero, and the two HUD timer warning flags
  initialize to one in `.data`.
- Lifecycle/xrefs: 0x431dd0 initializes the host HUD timer flag and pkt06
  next-send deadline; 0x432300 consumes and updates the pkt06 deadline and
  HUD timer warning flags during local-player replication.
- VC5 evidence: `python tools/recoil.py verify vc5
  gamenet_mission_pkt06_timer_globals` passed with zero unmasked data-byte
  mismatches for all four data symbols using
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`.
- Dependent plan entries: 0x431dd0 and 0x432300 still need complete
  source-owner/data routing before promotion, including the `net.zrd` path
  literal and shared fatal printf literal for 0x431dd0.

### network_online.net_session_config_dialog

- Owner symbol/scope: NetSessionConfigDialog authored MFC dialog class/static
  metadata and multiplayer map-name storage in `src/Battlesport/GameNet.cpp`.
- BN/source data: `g_NetSessionConfigDialog_MessageMap` at 0x4d03d0,
  `g_NetSessionConfigDialog_MessageMapEntries` at 0x4d03d8,
  `g_NetSessionConfigDialog_Vtbl` at 0x4d0420,
  `g_NetSessionConfigDialog_MapNameStrings` at 0x4f32d8, the seven
  map-name literals at 0x4db6cc, 0x4db6bc, 0x4db6b0, 0x4db6a4,
  0x4db694, 0x4db684, and 0x4db674, and the Exercise name format at
  0x4db6d8.
- Extent/section/nullness: the message map is an 8-byte MFC map record, the
  message-entry table is a 72-byte three-entry table, the vftable is a
  216-byte `CDialogVTable`, the CString storage is seven zero-initialized
  4-byte CString slots, and the map/session literals are independent
  null-terminated static string objects.
- Lifecycle/xrefs: 0x41c6e0 constructs the derived dialog and installs the
  authored vftable, 0x41c970 returns the message map, 0x41c990 constructs the
  map-name CString table, 0x41ca10 destroys the CString table, and
  0x41ca30/0x41cb50/0x41cb90 consume the map-name/session state while the
  dialog is active.
- VC5 evidence: `python tools/recoil.py verify vc5
  net_session_config_dialog_data` passed with zero unmasked data-byte
  mismatches for the message map, message entries, vftable, and CString
  storage. `python tools/recoil.py verify vc5
  net_session_config_dialog_map_literals` passed with zero unmasked data-byte
  mismatches for all seven map-name literals and the Exercise session-name
  format using `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`.
- Source-shape note: the accepted class model uses a real VC5-era
  MFC-derived constructor/destructor so the compiler emits
  `??_7NetSessionConfigDialog@@6B@`; the earlier placement-CDialog
  pseudo-constructor shape did not emit the derived vftable and was not
  accepted.
- Dependent plan entries: 0x41c6e0, 0x41c7f0, 0x41c880, 0x41c970,
  0x41c990, 0x41ca00, 0x41ca10, 0x41ca30, 0x41cb50, and 0x41cb90.
  The standalone 0x4f32d8 data row records data-equivalent evidence but keeps
  `Reimplemented [S]` blocked while the class byte gate remains deferred.

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
