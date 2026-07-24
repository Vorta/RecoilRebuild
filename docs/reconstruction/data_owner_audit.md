# Data Owner Audit

The owner `data` gate is whole-owner acceptance. A source owner can keep
`data=accepted` only when every touched authored `.data`, `.rdata`, and BSS
packet has complete source data evidence. Field-level recovery inside a
larger global, source macros, static offset asserts, and functional smokes are
not enough.

Distinguish primary source-shaped owners from auxiliary data packets. A primary
source-shaped owner is the original source construct being reconstructed:
class/interface, source-file cluster, subsystem, authored callback table,
authored record/table/global object/static class-member group, provider
boundary, or true standalone leaf. An auxiliary data packet merely groups
globals, literals, constants, or storage ranges to prove data prerequisites and
byte-readiness for a primary owner. Treat a data packet as a primary
source-owner target only when BN/source evidence proves the original source had
that exact authored data construct; otherwise link it upward to the primary
source-shaped owner and keep orphan packets as parent-reconciliation blockers.

Primary-data tracker entries are narrower: they track canonical `.data`
owner-range globals only. Do not add `.rdata` entries, BSS-only entries, or
member/field rows inside a larger global. For example, `0x4f0cc0
g_HudSensorTracker` is one data entry for the owner range, not one row per
BN-labeled member. `.rdata` and BSS facts still belong in the source-owner data
gate evidence when owner acceptance depends on them.

Data entries use `Reimplemented [X/C/B/A/S]`: `X` means no accepted
source-level data implementation, `C` means the complete source data-packet
model is accepted, `B` means the linked data and linkage gates are
accepted, `A` means reviewed relocation-masked data-symbol evidence is
near-byte-equivalent, and `S` means accepted data-symbol bytes plus relocation
identity, linked data-packet byte gate acceptance, and no current final
executable `.data` section/layout contradiction for that data row or owner byte
gate. Data-entry `S` means the packet is byte-ready as a dependency; it does
not complete the parent/source-owner tier `S` gate unless the packet is itself
proven to be the primary original authored construct.

## Final Executable Data Audit

VC5 `verify vc5` `data_symbols` evidence compares generated COFF symbol bytes
and relocation identity. It does not by itself prove that the final linked
`Recoil.exe` places the same `.data` raw bytes, virtual extent, zero-fill tail,
or map symbols as retail. When `python tools/recoil.py verify final-build`
produces `build/vc5-final/Recoil.exe` and the PE comparison reports `.data`
section drift, audit the final linked data layout before preserving or accepting
data `Reimplemented [S]`:

```powershell
python tools/recoil.py audit final-data --include-owners --strict --json-out build/vc5-final/final_data_diff.json
python tools/recoil.py progress output-section show recoil:section:.data
```

Final executable validation is Phase 5 and is documented in
`final_executable_repro.md`. Final-data reports are diagnostics, not work units,
queues, acceptance tokens, or peer schedulers.
Inspect them only when selected by `progress next`, explicitly requested, or
required by the current cursor. Their reports/imports accept nothing and never
mutate owner gates or tiers.

`--strict` returns nonzero when section deltas are present. For this audit that
is evidence that final data byte identity is blocked, not necessarily a tool
failure. `--include-owners` correlates final-data issues with current data `S`
rows for navigation only; it emits no owner-action batch. Inspect the live
result and `progress storage show`; storage or section acceptance remains a
separate parent-reviewed operation only after its own semantic gates pass.

Data symbols, source-owner data gates, physical `storage_contributions`, PE
`output_sections`, and final-image acceptance are distinct. A symbol address
does not prove its physical storage extent. When extent evidence is absent,
record `extent_state=unknown` and omit size/end; never invent a one-byte range.

The 2026-06-26 final-data audit downgraded 2,239 data rows from `S` to `B`.
Of those, 2,170 had direct final-data audit issues while at tier `S`: candidate
address drift or missing candidate map symbols. The remaining 69 were expanded
through affected owner byte gates. This was not a global blanket downgrade, but
it also was not byte-for-byte proof that every affected initializer's contents
mismatch. It was a conservative owner-byte-gate block caused by final executable
`.data` raw/virtual layout drift, not a block on unrelated source-owner tier
`S` work whose owner/data byte gates are ready.

## Acceptance Packet

For each accepted data packet, record:

- owner symbol, BN address/range, section, size, alignment, and source symbol;
- full storage extent, including adjacent fields that BN labels separately;
- initializer/lifecycle behavior for BSS or runtime-initialized records;
- initialized byte facts: order, strings, constants, GUIDs, flags, table slots,
  pointer/symbol identity, and nullness;
- VC5 `data_symbols` output when available, including relocation identity
  review for pointers;
- caller/function owners whose accepted data gate depends on this packet.
- primary source-shaped parent owner when known, or an explicit
  parent-reconciliation blocker when the packet is orphaned.

For each owner-tracked `.data` global definition in source, put an immediately
preceding docblock:

```cpp
/**
 * Reimplements data 0xNNNNNN: g_Symbol.
 * Purpose: Describes the source-level role of this data owner.
 */
```

If any item is missing, the dependent owner data gate stays pending or blocked.
If current BN/source evidence proves no authored globals are touched, use
`data=none`.

## Current Audit Baseline

The 2026-06-17 data-gate hardening pass found 751 owner-ledger entries carrying
accepted data claims. Existing VC manifests contain 415 `data_symbols` entries
across 103 manifests, so most of those claims could not be justified by
data-symbol evidence alone.

The same pass downgraded 718 stale accepted-data claims by setting the affected
primary entries to `X` through controlled owner-ledger
updates. The dry run succeeded for the 717 entries that lacked any
`data_symbols` target coverage, then the same updates were applied. A later verification pass found that
`0x4bab40 HudUiPanel::~HudUiPanel` did not emit an accepted data-symbol evidence
block for its target, so it was also downgraded.

The remaining accepted data entries are limited to entries whose VC5
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
  objective and nanite fields. The owner ledger had an accepted data claim and tier
  `B`, but the complete `g_HudUiMgr`/`HudUiMgrData` owner is not accepted:
  `HudUiMgr::Constructor` and `HudUiMgr::SetNanitePanelCount` remain
  owner/data-blocked, and no VC data-symbol manifest covers the complete owner.
The correct state is data-blocked until the full HUD manager data owner is
recovered and verified.

Future data-owner acceptances should append compact entries here rather than
relying only on address-local verification notes.

## 2026-06-24 Accepted Data Owners

### render_video.zrndr_span_esp_pivot_saved_esp_slot

- Owner symbol/scope: zRndr ESP-pivot saved-stack scratch pointer used by the
  switch-vshift span callback family in `src/GameZRecoil/zRndr/zRndr.cpp`.
- BN/source data: `gRndr_SavedEspSlot` at 0x57da38, implemented as
  `zRndr::g_spanSavedEspSlot`.
- Extent/section/nullness: independent 4-byte `.data` pointer storage,
  initialized to null. Source types the pointee as `zRndr_SpanEspPivotSave`,
  the saved register frame used by the narrow ESP-pivot write loops.
- Lifecycle/xrefs: the five switch-vshift span leaves
  `0x49b7e0`, `0x49bbf0`, `0x49e6c0`, `0x49edc0`, and `0x49f180` save the
  real ESP through this pointer before pivoting ESP to
  `gRndr_CurrentSpanBaseAddr + count`, then restore ESP before returning.
- VC5 evidence: `python tools/recoil.py verify vc5
  zrndr_span_esp_pivot_saved_esp_slot` passed with zero unmasked data-byte
  mismatches for the generated symbol
  `?g_spanSavedEspSlot@zRndr@@3PAUzRndr_SpanEspPivotSave@1@A` using
  `vc5_o2_ob0_md_zrndr_esp_pivot_raw_asm_facs`; the report includes the COFF
  relocation mask, relocation identity, byte diff, triage, object, and listing
  artifacts under `build/vc5-verify/zrndr_span_esp_pivot_saved_esp_slot/`.
- Dependent owner-ledger entries: supports the data gate for the ESP-pivot span leaves
  `0x49b7e0`, `0x49bbf0`, `0x49e6c0`, `0x49edc0`, and `0x49f180`, plus the
  standalone data row `0x57da38`.

## 2026-06-22 Data Evidence Candidates

### legacy.audio_fmv.data_zfmv_action_tag_strings

- Owner symbol/scope: packed zFMV script action-tag string set in
  `src/GameZRecoil/zFMV/fmv_script.cpp`, consumed only by
  `zFMV_Script::LoadActionsFromZrd` comparisons.
- BN/source data: retail labels the owner as `g_zFMV_ActionPlaySoundTag` at
  0x4dfb94, `g_zFMV_ActionBlurVTag` at 0x4dfba0,
  `g_zFMV_ActionBlurHTag` at 0x4dfba8, `g_zFMV_ActionBlurTag` at 0x4dfbb0,
  `g_zFMV_ActionPlayMciTag` at 0x4dfbb8, `g_zFMV_ActionPlayAviTag` at
  0x4dfbc0, `g_zFMV_ActionFadeOutTag` at 0x4dfbc8,
  `g_zFMV_ActionFadeInTag` at 0x4dfbd0, `g_zFMV_ActionWaitTag` at 0x4dfbd8,
  `g_zFMV_ActionLoadImageTag` at 0x4dfbe0,
  `g_zFMV_ActionBlitImageTag` at 0x4dfbec, and
  `g_zFMV_ActionShowImageTag` at 0x4dfbf8. Source models the full packed
  owner as `g_zFMV_ActionTagStrings`.
- Extent/section/nullness: the accepted owner candidate is the 112-byte
  initialized `.data` range 0x4dfb94..0x4dfc03, including inter-string zero
  padding. The adjacent `IMAGE_PATH` key at 0x4dfc04 is excluded.
- Lifecycle/xrefs: `zFMV_Script::LoadActionsFromZrd` compares action node tag
  strings against this set before constructing SHOWIMAGE, BLITIMAGE,
  LOADIMAGE, WAIT, FADEIN, FADEOUT, PLAYAVI, PLAYMCI, BLUR, BLURH, BLURV, and
  PLAYSOUND actions.
- VC5 evidence candidate: `python tools/recoil.py verify vc5
  zfmv_action_tag_strings_data` passed with zero unmasked data-byte mismatches
  for the complete 112-byte owner range using `vc5_o2_ob1_md_gx_facs`.
- Parent-owned gate status: no owner gate or tier was updated by this
  evidence candidate; parent still owns source-owner/data/byte acceptance for
  the twelve owner-tracked data entries.

### network_online.westwood_online_upgrade_api_source_file

- Owner symbol/scope: Westwood Online ActiveX API startup source-file globals
  in `src/Battlesport/WestwoodOnlineUpgradeApi.cpp`, separate from the accepted
  `network_online.westwood_online_upgrade_config_dialog` class owner and from
  the dialog/session-browser data owner.
- BN/source data: `g_WestwoodOnlineUpgradeInitWaitEvents` at 0x4f4220
  (12-byte `HANDLE[3]`), `g_WestwoodOnlineUpgradeFailureEvent` at 0x4f52c4,
  `g_WestwoodOnlineUpgradeSelectedBootstrapServer` at 0x4f52c8,
  `g_WestwoodOnlineUpgradeProcessCallbacksFlag` at 0x4f53cc,
  `g_WestwoodOnlineUpgradeApiInitState` starting at 0x4f53d0,
  `g_WestwoodOnlineUpgradeStatusTextEvent` at 0x4f5438,
  `g_WestwoodOnlineUpgradeBootstrapServerListEvent` at 0x4f5440,
  `g_pWestwoodOnlineUpgradeApiEventSink` at 0x538560,
  `g_WestwoodOnlineUpgradeApiAsyncErrorFlag` at 0x538564,
  `g_pWestwoodOnlineUpgradeApi` at 0x538574, and
  `g_WestwoodOnlineUpgradeApiAdviseCookie` at 0x538570. Additional source-file
  data now covered by the extra VC5 target is
  `g_WestwoodOnlineUpgradeApiInitReservedZero` at 0x4f5434
  (source symbol `g_WestwoodOnlineUpgradeApiShutdownState`),
  `g_WestwoodOnlineUpgradeAbortFlag` at 0x4dd24c, initialized to 1,
  and `g_WestwoodOnlineUpgradeCachedBrowseRecord` at 0x4f5448. Immutable COM
  identity data is `g_IID_WestwoodOnlineUpgradeApi` at 0x4d1838,
  `IID_WestwoodOnlineUpgradeApiEventSink` at 0x4d1848, and
  `g_CLSID_WestwoodOnlineUpgradeApi` at 0x4d18d8.
- Extent/section/nullness: all runtime globals above are zero-initialized
  independent 4-byte or typed aggregate symbols except the API init-state
  aggregate (100 bytes from source/0x42dda0 behavior), the selected bootstrap
  server record (260 bytes from source/VC5), the abort flag's initialized 1
  value, and the three immutable 16-byte GUIDs in `.rdata`. Parent BN/owner-ledger
  repair is still needed where current metadata lags the 260-byte 0x4f52c8
  record and 100-byte 0x4f53d0 init-state record. BN renders
  `g_WestwoodOnlineUpgradeCachedBrowseRecord` as a full 0x10c-byte
  `WestwoodOnlineUpgradeBrowseRecord`; 0x43d2e0 assembly clears that full
  extent before WOL startup, but the broader browse/cache lifecycle belongs to
  the separate dialog/session-browser owner.
- Lifecycle/xrefs: 0x43d130 initializes COM/control hosting, fills the
  init-state block, sets `g_WestwoodOnlineUpgradeProcessCallbacksFlag`, creates
  the event sink, and stores the advise cookie. 0x43d2e0 creates the three wait
  events, installs the wait-event array, clears the cached browse record, and
  requests bootstrap/list mode through the API provider. 0x43d280 unadvises,
  releases the API object, clears the API pointer, and closes the three named
  startup event handles.
- Verification state: functional targets
  `westwood_online_upgrade_api_init_state`,
  `westwood_online_upgrade_api_create_instance_load_config`,
  `westwood_online_upgrade_api_shutdown`, and
  `westwood_online_upgrade_api_init` cover the behavior. VC5 data-symbol
  targets `westwood_online_upgrade_api_data` and
  `westwood_online_upgrade_api_extra_data` pass with zero unmasked data-byte
  mismatches for the runtime symbols listed in those manifests using
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`. The extra
  target also covers immutable COM identity data at 0x4d1838, 0x4d1848, and
  0x4d18d8 by comparing the source symbols
  `g_WestwoodOnlineUpgradeApi_IID`,
  `g_WestwoodOnlineUpgradeApiEventSink_IID`, and
  `g_WestwoodOnlineUpgradeApi_CLSID` against the BN symbols named above. The
  broader WestwoodOnline dialog/download subsystem data remains outside this
  owner scope; that includes source-file globals whose current xrefs are in the
  dialog/download/event-sink slices rather than the API startup anchors, such
  as `g_WestwoodOnlineUpgradeVisibleSessionResultCount` at 0x538580 and
  `g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag` at 0x538584. Those two
  symbols were removed from `westwood_online_upgrade_api_extra_data` so this
  API-owner evidence does not link the session-browser data owner.
- Dependent owner-ledger entries: 0x42dda0, 0x43d130, 0x43d280, and 0x43d2e0.

## 2026-06-21 Data Evidence Candidates

### app_shell.czgame_frame_class

- Owner symbol/scope: CZGameFrame class-owned string data in
  `src/Battlesport/CZGameFrame.cpp`.
- BN/source data: `g_CZGameFrame_DefaultAppId` at 0x4dd8e8 and
  `g_CZGameFrame_GameBmpResourceName` at 0x4dd904.
- Extent/section/nullness: the app id is the six-byte null-terminated
  `"gamez"` string consumed by `CZGameFrame::CreateObject`; the bitmap resource
  name is the eight-byte null-terminated `"GAMEBMP"` string consumed twice by
  `CZGameFrame::OnCreate`.
- Lifecycle/xrefs: 0x443730 passes the app id to the CZGameFrame constructor,
  and 0x443a60 passes the bitmap name to `AfxFindResourceHandle` and
  `LoadBitmapA`.
- VC5 evidence candidate: `python tools/recoil.py verify vc5
  czgameframe_class_data` passed with zero unmasked data-byte mismatches for
  both string data symbols using `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`.
- Open limit: `g_CZGameFrame_VTable` at 0x4d21d8 remains separate class/vtable
  owner work. A probe in the same VC5 target could not find
  `??_7CZGameFrame@@6B@` because the current public source shape still exposes
  several CZGameFrame/CZRecoilFrame MFC callbacks as static helpers for the
  existing smoke surface instead of the real owner-wide virtual class shape.
  Parent should keep the owner data gate pending until that vtable is recovered
  or explicitly accepted as a bounded blocker.
- Parent-owned gate status: no owner gate or primary-entry tier was updated by this
  evidence candidate.

### hud_ui.hud_low_meter_loop_sound_globals

- Owner symbol/scope: HudLowMeterLoopSound low-meter warning sample pointers,
  active-state flag, and one-shot beep timing globals in
  `src/Battlesport/hud.cpp`.
- BN/source data: `g_Hud_LowMeterBeepSample` at 0x4f3748,
  `g_Hud_LowMeterLoopSample` at 0x4f374c,
  `g_Hud_LowMeterLoopActive` at 0x4f3750,
  `g_Hud_LowMeterBeepInterval` at 0x4f3758, and
  `g_Hud_LowMeterNextBeepTime` at 0x4f375c.
- Extent/section/nullness: each low-meter HUD item is an independent
  4-byte zero-initialized `.data` symbol. Retail places the accepted Player
  status-meter ratio `g_PlayerStatusMeterRatio` at 0x4f3754 between the
  low-meter active flag and beep-timing floats; that Player scalar remains
  owned by its existing Player data owner and is not part of this HUD owner.
- Lifecycle/xrefs: `Player::InitMissionRuntimeFromWorldAndCamera` loads the
  sample pointers and beep interval from the `low_shield_snd` ZRD node and
  clears the next-beep time. `HudLowMeterLoopSound::SetLoopActive` consumes
  the loop sample and active flag, `HudLowMeterLoopSound::Disable` consumes
  both samples and clears the active flag, and
  `Player::ResetDamageVisualsAndTimedStatus` consumes the beep sample,
  interval, and next-beep time when the status meter is low.
- VC5 evidence candidate: `python tools/recoil.py verify vc5
  hud_low_meter_loop_sound_globals` passed with zero unmasked data-byte
  mismatches for all five HUD symbols using
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`. Parent review still owns
  acceptance of the source-owner/data/byte gates.
- Dependent owner-ledger entries: 0x439b20 and 0x439b70 for direct loop-sound control;
  this evidence may also support the narrow low-meter portion of
  `Player::InitMissionRuntimeFromWorldAndCamera` without broadening the HUD
  data owner over 0x4f3754.

### battlesport_gameplay.player_zrd_runtime_tuning_globals

- Owner symbol/scope: Player player.zrd runtime tuning globals in
  `src/Battlesport/player.cpp`, written by
  `Player::InitMissionRuntimeFromWorldAndCamera` / recovered helper
  `PlayerLoadPlayerZrdTuning`.
- BN/source data: `g_Player_MaxCamYawRate` at 0x4f36f8,
  `g_Player_MousePushX` at 0x4f36fc, `g_Player_MousePushY` at 0x4f3700,
  `g_Player_CameraElastic` at 0x4f3704,
  `g_Player_MaxCamTetherAngleRad` at 0x4f3708,
  `g_Player_FpCamElevationRate` at 0x4f370c,
  `g_Player_FpCamElevationMax` at 0x4f3710,
  `g_Player_FpCamElevationMin` at 0x4f3714,
  `g_Player_UnderwaterCamDistance` at 0x4f371c,
  `g_Player_UnderwaterCamHeight` at 0x4f3720,
  `g_Player_UnderwaterCamStepCount` at 0x4f3724,
  `g_Player_UnderwaterCamFar` at 0x4f3728,
  `g_Player_UnderwaterCamPackedColor` at 0x4f372c,
  `g_Player_UnderwaterCamAlpha` at 0x4f3730,
  `g_Player_MakeHotOptEntry` at 0x4f3734,
  `g_Player_MakeColdOptEntry` at 0x4f3738,
  `g_Player_LavaSinkRate` at 0x4f3698, `g_Player_MaxSlope` at 0x4f3338,
  `g_Player_QuicksandSinkRate` at 0x4f376c,
  `g_Player_WaterGravity` at 0x4f3ab8, and
  `g_Player_QuicksandGravity` at 0x4f3ac0.
- Extent/section/nullness: each item is an independent 4-byte zero-initialized
  `.data` symbol. BN types the camera/gravity/sink/slope items as floats except
  `g_Player_UnderwaterCamStepCount` as `int32_t`,
  `g_Player_UnderwaterCamPackedColor` as `uint32_t`, and the heat/cold caches
  as `OptCatalogEntryDef *`.
- Lifecycle/xrefs: 0x41fe90 loads `player.zrd`, probes named nodes such as
  `max_cam_yaw_rate`, `mouse_push`, `fp_cam_el_rate`, `fp_cam_el_lim`,
  `underwater_cam`, `camera_elastic`, `max_cam_tether_angle`, `wat_gravity`,
  `qsd_gravity`, `qsand_sink`, `lava_sink`, `max_slope`, `make_hot`, and
  `make_cold`, then writes these globals from ZRD payloads or the retail
  fallback constants. The HUD low-meter sample/timing globals and accepted
  0x4f36f0/0x4f36f4 camera-zone pair remain separate owner evidence.
- VC5 evidence candidate: `python tools/recoil.py verify vc5
  player_zrd_runtime_tuning_globals` compiled with `vc5_o2_ob0_md_facs` and
  verified the first ten symbols listed in the manifest with zero unmasked
  data-byte mismatches before the tool's ten-call BN budget stopped the
  remaining data-symbol comparison. A compile-only rerun with
  `--skip-bn-compare` covered the full manifest, but parent review still needs
  complete BN-backed data-symbol evidence before any byte-tier claim.
- Dependent owner-ledger entry: 0x41fe90
  `Player::InitMissionRuntimeFromWorldAndCamera`, specifically the
  `battlesport_gameplay.player_mission_runtime_bootstrap` data gate.

### battlesport_gameplay.player_mission_runtime_bootstrap

- Owner symbol/scope: Player mission-runtime bootstrap globals in
  `src/Battlesport/player.cpp`.
- BN/source data: `g_Player_MissionInitFirstRunFlag` at 0x4dc268,
  `g_Player_LocalControlEnabled` at 0x4f36b0,
  `g_Player_BftSplashAnimEntry` at 0x4f3740, `g_Player2SaveState` at
  0x4f3770, and `g_Player_AivParentDir` at 0x4e5b50.
- Extent/section/nullness: the first-run flag is an initialized 4-byte `.data`
  `int` with value 1; the local-control flag is a zero-filled 4-byte `.data`
  `int`; the BFT splash and Player2 save-state entries are zero-filled 4-byte
  pointer globals; the AIV parent directory is a zero-filled 260-byte `.data`
  character buffer.
- Lifecycle/xrefs: 0x41fe90 clears the first-run flag after one-time HUD panel
  registration, seeds the local-control flag from the network option, caches
  the `bftsplash` animation entry, assigns the hidden/stealth save-state
  pointer, and asks `zReader::BuildResolvedParentDir` to fill the AIV parent
  directory after `aiv.zrd` loads. Later local-control, camera/HUD, save/load,
  and gameplay FX paths read the same globals.
- VC5 evidence: `python tools/recoil.py verify vc5
  player_mission_runtime_bootstrap_globals` passed with zero unmasked
  data-byte mismatches for all five bootstrap symbols using
  `vc5_o2_ob0_md_facs`. The same manifest also carries already-routed
  `g_Player_CameraZone` and `g_Player_CameraZoneInvRange` symbols for local
  compile/data comparison; those two remain under the accepted Player ZRD
  tuning owner rather than this bootstrap owner.
- Dependent owner-ledger entry: 0x41fe90
  `Player::InitMissionRuntimeFromWorldAndCamera`.

### hud_ui.hud_layout_classes

- Owner symbol/scope: HudLayout Base/SW/HW class compiler-emitted vtables,
  singleton storage, and TYPEI/TYPEII ZRD section-name data in
  `src/GameZRecoil/zHud/zhud_ui.cpp`.
- BN/source data: `g_HudLayoutSW_FTable` at 0x4ce968,
  `g_HudLayoutBase_FTable` at 0x4ce988, `g_HudLayoutHW_FTable` at
  0x4ce9a8, `g_HudLayoutHW` at 0x4ed718, `g_HudLayoutSW` at 0x4eda68,
  `g_HudLayout_TypeISectionName` at 0x4dae0c, and
  `g_HudLayout_TypeIISectionName` at 0x4dae14.
- Extent/section/nullness: the vtables are 28, 28, and 32-byte
  compiler-emitted dispatch tables for the typed C++ classes; the hardware and
  software layout singleton objects are 844 and 236-byte zero-initialized
  globals; the TYPEI and TYPEII names are six and seven-byte initialized
  strings.
- Lifecycle/xrefs: the global init helpers construct `g_HudLayoutHW` and
  `g_HudLayoutSW`, register at-exit destructors, and the layout load helpers
  use the TYPEI/TYPEII section names for ZRD traversal. The vtable entries are
  compiler data for the recovered virtual class model, not authored production
  FTable arrays.
- VC5 evidence candidate: `python tools/recoil.py verify vc5
  hud_layout_class_data` passed with zero unmasked data-byte mismatches for all
  seven data symbols using `vc5_o2_ob0_md_facs`.
- Parent-owned gate status: no owner gate or primary-entry tier was updated by this
  evidence candidate; broader HudLayout source/functional/data gate acceptance
  remains parent-owned.

## 2026-06-22 Data Evidence Candidates

### render_video.zrndr_textured_finalize_dispatch_data

- Owner symbol/scope: zRndr textured queued span finalize callback globals in
  `src/GameZRecoil/zRndr/zRndr.cpp`, limited to the non-ESP finalizer slots.
- BN/source data: `gRndr_pfnTexturedQueuedFinalize` at 0x632104 maps to
  `zRndr::g_pfnTexturedQueuedFinalize`, and
  `gRndr_pfnTexturedQueuedFinalizeAlt` at 0x632108 maps to
  `zRndr::g_pfnTexturedQueuedFinalizeAlt`.
- Extent/section/nullness: two adjacent 4-byte owner-tracked `.data`
  function-pointer globals, both zero-initialized; the VC5 object emits them
  into `.bss#2`. The owner excludes the ESP-pivot scratch global at 0x57da38
  and excludes the broader SelectSpanRoutines callback bank.
- Lifecycle/xrefs: `zRndr::SelectSpanRoutines` writes the first slot to one of
  the scalar or MMX fog-blend finalizers and writes the second slot to
  `SpanMmxSetTexUvMasksAndVShift` only in the MMX path, otherwise null.
  `zRndr::DrawTexturedQueued` reads the first slot before queued textured-span
  dispatch.
- VC5 evidence candidate: `python tools/recoil.py verify vc5
  zrndr_textured_finalize_dispatch_data` passed with zero unmasked data-byte
  mismatches for both 4-byte data symbols using `vc5_o2_ob0_md_facs`; both
  zero-initialized symbols reported zero relocation bytes.
- Parent-owned gate status: no owner gate or tier was updated by this
  evidence candidate; parent still owns source-owner gate and data-entry
  promotion for 0x632104 and 0x632108.

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
- Dependent owner-ledger entries: none promoted in this pass; shared overlay
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
- Dependent owner-ledger entries: data entry 0x4dcfb4 was accepted; 0x431dd0 remains
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
- Dependent owner-ledger entries: data entries 0x4dcdb0, 0x4f3f6c, and 0x4f3f70 were
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
- Dependent owner-ledger entries: data entry 0x4dcfbc was accepted; 0x432300 remains
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
- Dependent owner-ledger entries: data entry 0x4db4ac was accepted; function-level
  promotions remain controlled by their respective source-owner gates.

### core_util_archive.shared_path_join_format_literal_data

- Owner symbol/scope: shared pooled path-join format literal used by zReader,
  zSys, and zUtil path resolution code.
- BN/source data: `g_SharedPathJoinFormatLiteral` at 0x4dc6c8.
- Extent/section/nullness: independent 5-byte null-terminated immutable string
  literal for the percent-s percent-s path join format.
- Lifecycle/xrefs: `zReader::BuildResolvedParentDir`,
  `zSys::FindFileOnDriveType`, and `zUtil_ZRDR_ResolvePathInSearchPathList`
  use the literal at their source call sites. The related zReader backslash
  join format remains a distinct literal.
- VC5 evidence: no accepted data-symbol byte target yet covers this pooled
  literal; byte evidence remains deferred.
- Dependent owner-ledger entries: data entry 0x4dc6c8 was accepted to tier B through
  owner/source/data gates only; caller function promotions remain controlled by
  their respective source-owner gates.

### app_shell.recoil_app_mfc_message_maps

- Owner symbol/scope: RecoilApp and RecoilApp_MfcOleModule static MFC
  message-map metadata under the `legacy.app_shell.class_recoilapp` class owner.
  `src/Battlesport/RecoilApp_Late.cpp` is the local included shard in the
  `src/Battlesport/RecoilApp.cpp` translation-unit path, not an original
  source-file owner boundary.
- BN/source data: `g_RecoilApp_MessageMap` at 0x4d0990,
  `g_RecoilApp_MessageEntries` at 0x4d0998,
  `g_RecoilApp_MfcOleModule_MessageMap` at 0x4d2000, and
  `g_RecoilApp_MfcOleModule_MessageEntries` at 0x4d2008.
- Extent/section/nullness: the two message maps are 8-byte `.rdata`
  `AFX_MSGMAP` records. The two message-entry arrays are 24-byte
  sentinel-only `AFX_MSGMAP_ENTRY` arrays containing all zero bytes.
  BN shows 0x4d2000 as bytes `90 28 44 00 08 20 4d 00`, pointing to the
  MFC42 base getter at 0x442890 and entries at 0x4d2008.
- Provider exclusions: 0x4428a0 is the provider-boundary MFC generated getter
  that returns 0x4d2000; it is not authored RecoilApp behavior and owns no
  data. The returned 0x4d2000/0x4d2008 metadata remains authored class static
  data under `legacy.app_shell.class_recoilapp`.
- VC5 evidence: `python tools/recoil.py verify vc5 recoil_app_get_message_map
  --auto-chunk` passed for 0x42de10, 0x4d0990, 0x4d0998, 0x4d2000, and
  0x4d2008 with zero unmasked mismatches using
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`. The narrow
  `python tools/recoil.py verify vc5 0x4d2000 0x4d2008 --auto-chunk` check
  also passed; 0x4d2000 has eight relocation-masked bytes and 0x4d2008 has no
  relocations.
- Gate limit: this data evidence supports the RecoilApp owner data gate. It
  does not accept the owner byte gate, owner tier `S`, or final executable
  reproducibility while the linked final `.data` layout lane remains blocked.

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
- Dependent owner-ledger entries: 0x4301e0, 0x443730, 0x430240, 0x4306e0, 0x443790,
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
- Dependent owner-ledger entries: supports the accepted data gate for 0x4b1090
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
- Dependent owner-ledger entries: 0x4b0600, 0x4b0620, and 0x4b0640 are promoted to
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
- Dependent owner-ledger entries: no broad zWeapon init/load entry is promoted from
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
- Dependent owner-ledger entries: 0x4af060 and 0x4b0e20.

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
- Dependent owner-ledger entries: 0x472cc0 and 0x472a10.

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
- Dependent owner-ledger entries: 0x431dd0 and 0x432300 still need complete
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
- Dependent owner-ledger entries: 0x41c6e0, 0x41c7f0, 0x41c880, 0x41c970,
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
- Dependent owner-ledger entries: 0x408230, 0x408240, 0x408250, 0x408260, and
  0x408270.

### Open source model: 0x407700 option-pointer reset span

- Retail `zGame::Options_LoadGameOptions` at 0x407700 clears exactly 38
  dwords at `[0x4e5d00,0x4e5d98)` inline before its first real call:
  `mov ecx,0x26; xor eax,eax; mov edi,0x4e5d00; rep stosd`.
- Current source instead calls the out-of-line TU-local
  `ResetOptionPointers`, whose 37 independent null assignments omit the
  retail-cleared dword at 0x4e5d8c. This helper is not a supported original
  inline-helper recovery and must not be bridged, aliased, or hidden from the
  call-contract verifier.
- Read-only Binary Ninja inspection identifies typed four-byte globals at
  0x4e5d88 and 0x4e5d90, but gives 0x4e5d8c no type, containing object, code
  xref, or data xref. The zero byte image and runtime clear prove storage, not
  whether it was an unused record member, array element, or independent
  global. Ordinary x86 pointer alignment does not explain a four-byte hole.
- The earlier per-symbol data-owner acceptance above remains evidence for the
  three named network pointers only. It does not prove that the complete
  0x98-byte reset span was declared as independent globals, nor does it accept
  a larger aggregate.
- Competing original-source models remain one heterogeneous aggregate, a
  38-element pointer array, and linker-contiguous independent globals. A raw
  cross-global `memset`, clearing overlay, offset-named/padding member, or
  invented semantic field would be reconstruction scaffolding rather than
  source recovery.
- The narrow missing evidence is type-bearing or equivalent original
  VC5 object/PDB/linker evidence showing whether users relocate against one
  base object or distinct data symbols. Variable-indexed access would support
  an array; a single type-bearing 0x98-byte object would support an aggregate;
  distinct original object symbols would support independent globals.
- A parent-brokered source-discovery review on 2026-07-24
  (`2026-07-24T09-31-22-443Z-chatgpt-call`, no uploaded files) ranked the
  aggregate model only narrowly first and advised `BLOCK SOURCE`. Its
  transcript was recorded in session scratch under the matching run id; this
  note preserves the material conclusion and does not depend on that scratch
  artifact remaining present.

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
- Dependent owner-ledger entries: 0x44ecf0.

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
- Dependent owner-ledger entries: 0x451b20, 0x452400, 0x452500, and 0x452560.

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
- Dependent owner-ledger entries: 0x420d10, 0x421a40, 0x421470, 0x421790, 0x421830,
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
- Dependent owner-ledger entries: 0x4a0300, 0x49fff0, 0x4a0590, 0x4a0670, and
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
- Dependent owner-ledger entries: 0x4a0380, 0x4a0400, 0x4a0490, and 0x4a0590.
