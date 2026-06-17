# Data Owner Audit

`Data reimplemented` is a whole-owner gate. A function or source group can keep
`Data reimplemented ✅` only when every touched authored `.data`, `.rdata`, and
BSS owner has complete source data-owner evidence. Field-level recovery inside a
larger global, source macros, static offset asserts, and functional smokes are
not enough.

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
