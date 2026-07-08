# Source File Map

Generated from address-backed `Reimplements 0xNNNNNN: Name (original/source/path)` provenance docblocks/comments in `src/`.
Refresh with `python tools/recoil.py audit source-map --update --output docs/reconstruction/source_file_map.md`.
Binary Ninja remains authoritative; this map is an agent navigation aid.
It contains address-backed provenance docblocks, plus legacy line comments until touched source is converted.
It excludes helpers fully inlined by the retail compiler.

Entries: 3709

## Case-insensitive source path collisions

These original-source labels differ only by case on Windows; confirm placement against Binary Ninja before adding new code.

- `Battlesport/HudUi.cpp`, `Battlesport/hudui.cpp`
- `Battlesport/zOpt.cpp`, `Battlesport/zopt.cpp`
- `GameZRecoil/zEffect/zEffect.cpp`, `GameZRecoil/zEffect/zeffect.cpp`

## Battlesport/ai_net.cpp

- `0x402fd0` `AINet::LoadAllFromZrd` -> `src/Battlesport/ai_net.cpp:269`
- `0x402ff0` `AINet::Alloc` -> `src/Battlesport/ai_net.cpp:279`
- `0x403040` `AINet::LoadFromZrd` -> `src/Battlesport/ai_net.cpp:302`
- `0x403510` `AINet::FindByNetId` -> `src/Battlesport/ai_net.cpp:600`
- `0x403530` `AINet::FindNodeByIndex` -> `src/Battlesport/ai_net.cpp:618`
- `0x403550` `AINet::ResolveNeighborLinksAndBuildProbeFans` -> `src/Battlesport/ai_net.cpp:637`
- `0x403620` `AINetPathProbeFan::InitFromSegment` -> `src/Battlesport/ai_net.cpp:680`
- `0x4036f0` `AINet::FindNearestNode` -> `src/Battlesport/ai_net.cpp:761`
- `0x4037c0` `AINetNode::Free` -> `src/Battlesport/ai_net.cpp:816`
- `0x403800` `AINet::Free` -> `src/Battlesport/ai_net.cpp:837`
- `0x403870` `AINet::FreeAll` -> `src/Battlesport/ai_net.cpp:879`
- `0x4340c0` `OptCatalog::AltGunDispatchAllocRuntimeGateCallback` -> `src/GameZRecoil/zWeapon/zwep_init.c:1865`

## Battlesport/ai_net.h

- `0x401060` `AINet::TickAiMode2TopLevel` -> `src/Battlesport/ai_net.h:763`
- `0x401180` `AINet::TickAiMode2PathFollow` -> `src/Battlesport/ai_net.h:844`
- `0x401420` `AINet::AiMode2ForwardProbeRequiresAutoTurn` -> `src/Battlesport/ai_net.h:973`
- `0x401580` `AINet::AiAdvancePathCursorAndComputeTargetVec` -> `src/Battlesport/ai_net.h:1032`
- `0x4016a0` `AINet::AiChooseNextPathBranchIndex` -> `src/Battlesport/ai_net.h:1112`
- `0x401710` `AINet::TickAiMode2SteeringSubstate` -> `src/Battlesport/ai_net.h:1151`
- `0x401970` `AINet::UpdateAiMode2MoveAndTurnTowardTarget` -> `src/Battlesport/ai_net.h:1268`
- `0x401a40` `AINet::TickAiMode2OffsetTargetSteering` -> `src/Battlesport/ai_net.h:1311`
- `0x401ab0` `AINet::TickAiMode2DynamicOffsetTargetSteering` -> `src/Battlesport/ai_net.h:1348`
- `0x401b20` `AINet::AiTryEnterMode2AttackPursuitIfLineOfSight` -> `src/Battlesport/ai_net.h:1385`
- `0x401c00` `AINet::AiAlertAttackBuddies` -> `src/Battlesport/ai_net.h:1426`
- `0x401c60` `AINet::AiEnterMode2SteeringPursuit` -> `src/Battlesport/ai_net.h:1449`
- `0x401f60` `AINet::AiRebuildSyntheticPathToNodeIfFar` -> `src/Battlesport/ai_net.h:1637`
- `0x402090` `AINet::UpdateAiMode2TurnTowardPlayerNoThrottle` -> `src/Battlesport/ai_net.h:1695`
- `0x402170` `AINet::UpdateAiMode2TurnInPlaceTowardPlayer` -> `src/Battlesport/ai_net.h:1741`
- `0x402250` `AINet::TickAiMode2AltGunAttackWindow` -> `src/Battlesport/ai_net.h:1787`
- `0x4024a0` `AINet::SolveAltGunLeadTargetPoint` -> `src/Battlesport/ai_net.h:1895`
- `0x4026d0` `AINet::UpdateAiMode2MoveAndTurnTowardOffsetTarget` -> `src/Battlesport/ai_net.h:1954`
- `0x4028c0` `AINet::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget` -> `src/Battlesport/ai_net.h:2018`
- `0x402b70` `AINet::TickAiMode2TimedPathSteering` -> `src/Battlesport/ai_net.h:2109`
- `0x402be0` `AINet::AiSteerTowardPathNodeForward` -> `src/Battlesport/ai_net.h:2135`
- `0x402d60` `AINet::AiSteerTowardPathNodeReverse` -> `src/Battlesport/ai_net.h:2207`

## Battlesport/AiPropertyDlg.cpp

- `0x41c0c0` `AiPropertyDlg::OnDestroy` -> `src/Battlesport/ai_property_dlg_body.h:89`
- `0x41c130` `AiPropertyDlg::OnSelChange` -> `src/Battlesport/ai_property_dlg_body.h:129`
- `0x41c170` `AiPropertyDlg::UpdatePropertyLabels` -> `src/Battlesport/ai_property_dlg_body.h:151`

## Battlesport/Briefing.cpp

- `0x403930` `HudUiBriefingRuntime::HudUiBriefingRuntime` -> `src/Battlesport/Briefing.cpp:461`
- `0x403c10` `HudUiBriefingLocatorPanel::HudUiBriefingLocatorPanel` -> `src/Battlesport/Briefing.cpp:602`
- `0x403c80` `HudUiCircle::DrawDirtyForwarder` -> `src/Battlesport/Briefing.cpp:622`
- `0x403c90` `HudUiBriefingLocatorPanel::BlitDirtyRect` -> `src/Battlesport/Briefing.cpp:631`
- `0x403cb0` `HudUiBriefingLocatorPanel::Update` -> `src/Battlesport/Briefing.cpp:657`
- `0x403d90` `HudUiBriefingRuntime::ScalarDeletingDestructor` -> `src/Battlesport/Briefing.cpp:700`
- `0x403ed0` `HudUiBriefingRuntime::Destructor` -> `src/Battlesport/Briefing.cpp:735`
- `0x404070` `HudUiBriefingRuntime::Update` -> `src/Battlesport/Briefing.cpp:773`
- `0x404140` `zInput_WaitForAnyKeyPressWithTimeoutMs` -> `src/Battlesport/Briefing.cpp:806`
- `0x404180` `Briefing::StartForMission` -> `src/Battlesport/Briefing.cpp:834`
- `0x404280` `Briefing::ThreadMain` -> `src/Battlesport/Briefing.cpp:877`
- `0x404bd0` `Briefing::StopAndShutdownThread` -> `src/Battlesport/Briefing.cpp:1280`

## Battlesport/CZRecoilFrame.cpp

- `0x430c30` `CZRecoilFrame::OnMenuAbout` -> `src/Battlesport/cz_recoil_frame_body.h:1069`

## Battlesport/GameNet.cpp

- `0x41b8ac` `NetSessionBrowserDialog::kHelpDocsFindExecutableErrorClassTable` -> `src/Battlesport/game_net_body.h:437`

## Battlesport/hud.cpp

- `0x404ca0` `HudUiElement::Draw` -> `src/Battlesport/hud.cpp:296`
- `0x404cb0` `HudUiElement::DrawBase` -> `src/Battlesport/hud.cpp:305`
- `0x404cd0` `HudUiElement::SetPos` -> `src/Battlesport/hud.cpp:322`
- `0x404cf0` `HudUiElement::SetX` -> `src/Battlesport/hud.cpp:336`
- `0x404d00` `HudUiElement::SetY` -> `src/Battlesport/hud.cpp:348`
- `0x404d10` `HudUiElement::HitTestTrue` -> `src/Battlesport/hud.cpp:360`
- `0x404d20` `HudUiElement::SetVisible` -> `src/Battlesport/hud.cpp:374`
- `0x404d50` `HudUiElement::GetX` -> `src/Battlesport/hud.cpp:391`
- `0x404d60` `HudUiElement::GetY` -> `src/Battlesport/hud.cpp:400`
- `0x404d70` `HudUiElement::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:409`
- `0x404d90` `HudUiWidget::GetCenterX` -> `src/Battlesport/hud.cpp:428`
- `0x404dd0` `HudUiWidget::GetCenterY` -> `src/Battlesport/hud.cpp:442`
- `0x404e10` `HudUiWidget::RebuildBltRectFromImage` -> `src/Battlesport/hud.cpp:456`
- `0x404e60` `HudUiCircle::HitTest` -> `src/Battlesport/hud.cpp:477`
- `0x406af0` `HudCheat::ExecuteCommandString` -> `src/Battlesport/hud.cpp:1871`
- `0x406cf0` `HudCheat::ClearNanitePanelCheatSentinel` -> `src/Battlesport/hud.cpp:2009`
- `0x407100` `HudUiCallback::QueueExitCurrentState` -> `src/Battlesport/hud.cpp:2235`
- `0x407110` `HudUiCallback::QueueCheatCodeState` -> `src/Battlesport/hud.cpp:2244`
- `0x4089c0` `HudUiMgr::ScreenToWorld` -> `src/GameZRecoil/zUI/zui.cpp:4255`
- `0x40bef0` `HudUiPanel::DestructorThunk` -> `src/Battlesport/hud_command_binding_layer_body.h:1394`
- `0x40bf00` `HudUtil::FreeFieldPtr` -> `src/Battlesport/hud_command_binding_layer_body.h:1404`
- `0x40d330` `HudLayoutHW::GlobalDestructor` -> `src/GameZRecoil/zUI/zui.cpp:1000`
- `0x40d400` `HudUiMgr::StaticInitAndRegisterAtExit` -> `src/GameZRecoil/zUI/zui.cpp:2427`
- `0x40d7e0` `HudUiMgr::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:2566`
- `0x40dbf0` `HudUiCounterTextPanel::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:2704`
- `0x40dcd0` `HudUiTriplet::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:2742`
- `0x40eb00` `HudUiShieldMessageWidget::ApplyLayout` -> `src/GameZRecoil/zUI/zui.cpp:3180`
- `0x40f2e0` `HudUiNanitePanel::InitLayout` -> `src/GameZRecoil/zUI/zui.cpp:3584`
- `0x40fe30` `HudUiShieldMessageWidget::Destructor` -> `src/GameZRecoil/zUI/zui.cpp:4110`
- `0x40ff50` `HudUiMgr::ActivateHud` -> `src/GameZRecoil/zUI/zui.cpp:4141`
- `0x40ff80` `HudUiMgr::OnViewportChanged` -> `src/GameZRecoil/zUI/zui.cpp:4160`
- `0x410160` `HudUiMgr::EnsureHudLoaded` -> `src/Battlesport/hud_runtime_layer_body.h:229`
- `0x410e90` `HudUiMgr::EnableHud` -> `src/Battlesport/hud_runtime_layer_body.h:2147`
- `0x410fe0` `HudUiMgr::UpdateFrame` -> `src/Battlesport/hud_runtime_layer_body.h:2211`
- `0x411170` `HudUiMgr::ProjectPointToNormalizedClamped` -> `src/Battlesport/hud_runtime_layer_body.h:2283`
- `0x411760` `HudUiMgrObjective::SetVisibleAndResetMeterFill` -> `src/Battlesport/hud_runtime_layer_body.h:2641`
- `0x4117f0` `HudUiMgrObjective::TickMeterFillAnimation` -> `src/Battlesport/hud_runtime_layer_body.h:2666`
- `0x4118b0` `HudUiMgrObjective::UpdateMeterXPoints` -> `src/Battlesport/hud_runtime_layer_body.h:2690`
- `0x411900` `HudUiMgrObjective::Show` -> `src/Battlesport/hud_runtime_layer_body.h:2705`
- `0x411a20` `HudUiMgrObjective::Begin` -> `src/Battlesport/hud_runtime_layer_body.h:2752`
- `0x411ac0` `HudUiMgrObjective::StartHide` -> `src/Battlesport/hud_runtime_layer_body.h:2783`
- `0x411f10` `HudUiMgrSensor::SetShieldMessageRatio` -> `src/Battlesport/hud_runtime_layer_body.h:2889`
- `0x412050` `HudUiMgrObjective::RefreshCounterText` -> `src/Battlesport/hud_runtime_layer_body.h:2931`
- `0x4124b0` `HudUiMgrTarget::UpdateSelectedProgressMeter` -> `src/Battlesport/hud_runtime_layer_body.h:3180`
- `0x412620` `HudUiMgr::HideTrackedProgressMeterIfOwnerMatches` -> `src/Battlesport/hud_runtime_layer_body.h:3254`
- `0x412650` `HudUiMessage::SetValueIfOwnerMatches` -> `src/Battlesport/hud_runtime_layer_body.h:3276`
- `0x412820` `HudUiMessage::UpdateSelectedWeaponDisplay` -> `src/Battlesport/hud_runtime_layer_body.h:3365`
- `0x412c10` `HudLayoutSW::LoadTypeIFromZarRoot` -> `src/Battlesport/hud_runtime_layer_body.h:3467`
- `0x412c60` `HudLayoutSW::SetActive` -> `src/Battlesport/hud_runtime_layer_body.h:3493`
- `0x412db0` `HudLayout::ApplyViewportRect` -> `src/Battlesport/hud_runtime_layer_body.h:3562`
- `0x412f70` `HudLayoutHW::LoadTypeIIFromZarRoot` -> `src/Battlesport/hud_runtime_layer_body.h:3662`
- `0x4130d0` `HudLayoutHW::SetActive` -> `src/Battlesport/hud_runtime_layer_body.h:3745`
- `0x4132b0` `HudLayoutHW::UpdateObjectiveDirtyRect` -> `src/Battlesport/hud_runtime_layer_body.h:3854`
- `0x413340` `HudLayoutHW::OnActivated` -> `src/Battlesport/hud_runtime_layer_body.h:3878`
- `0x4134e0` `HudUiMessage::Draw` -> `src/Battlesport/hud_runtime_layer_body.h:3948`
- `0x413500` `HudLayoutHW::UpdateAll` -> `src/Battlesport/hud_runtime_layer_body.h:3958`
- `0x413540` `HudLayoutHW::Enable` -> `src/Battlesport/hud_runtime_layer_body.h:3976`
- `0x4135f0` `HudLayoutHW::Disable` -> `src/Battlesport/hud_runtime_layer_body.h:4011`
- `0x413600` `zOpt::ToggleHudTypeForCurrentHwMode` -> `src/Battlesport/hud_runtime_layer_body.h:4021`
- `0x413640` `HudUiMgr::ToggleHud` -> `src/Battlesport/hud_runtime_layer_body.h:4050`
- `0x413660` `HudUiMgr::SwitchActiveDialog` -> `src/Battlesport/hud_runtime_layer_body.h:4064`
- `0x4136b0` `HudUiMgr::ApplyHudModeSwitch` -> `src/Battlesport/hud_runtime_layer_body.h:4091`
- `0x4136f0` `HudUiSensorWindow::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud_runtime_layer_body.h:833`
- `0x413700` `HudUiSensorWindow::StaticInit` -> `src/Battlesport/hud_runtime_layer_body.h:844`
- `0x413710` `HudUiSensorWindow::RegisterAtExit` -> `src/Battlesport/hud_runtime_layer_body.h:853`
- `0x413720` `HudUiSensorWindow::AtExitDestructor` -> `src/Battlesport/hud_runtime_layer_body.h:863`
- `0x413770` `HudUiMgr::SetFloatTimerVisible` -> `src/Battlesport/hud_runtime_layer_body.h:896`
- `0x4137a0` `HudUiMgr::SetAuxOverlayVisible` -> `src/Battlesport/hud_runtime_layer_body.h:911`
- `0x4138d0` `HudUi::ShowTopMessageLine` -> `src/Battlesport/hud_runtime_layer_body.h:979`
- `0x4138f0` `HudUi::ShowChatLine` -> `src/Battlesport/hud_runtime_layer_body.h:997`
- `0x413990` `HudUiLayoutNode::ApplyTextLabel` -> `src/Battlesport/hud_runtime_layer_body.h:1041`
- `0x413a10` `HudUiLayoutNode::ReadRectOffsetAndSize` -> `src/Battlesport/hud_runtime_layer_body.h:1076`
- `0x413aa0` `HudUiLayoutNode::ReadRect` -> `src/Battlesport/hud_runtime_layer_body.h:1116`
- `0x413ad0` `HudUiLayoutNode::ReadInt3` -> `src/Battlesport/hud_runtime_layer_body.h:1137`
- `0x413b10` `HudUiLayoutNode::ApplyCornerTextQuad` -> `src/Battlesport/hud_runtime_layer_body.h:1168`
- `0x413c10` `HudUiLayoutNode::ApplyMeterQuad` -> `src/Battlesport/hud_runtime_layer_body.h:1231`
- `0x413d30` `HudUiLayoutNode::ApplyImageWidget` -> `src/Battlesport/hud_runtime_layer_body.h:1301`
- `0x414070` `HudUiMessage::RebuildWeaponLayout` -> `src/Battlesport/hud_runtime_layer_body.h:1461`
- `0x414300` `HudUiMgrSensor::GetFxRect` -> `src/Battlesport/hud_runtime_layer_body.h:1704`
- `0x414330` `GameNet::ShowPlayerKillMessage` -> `src/Battlesport/hud_runtime_layer_body.h:1717`
- `0x414390` `GameNet::RefreshPlayerListMenu` -> `src/Battlesport/hud_runtime_layer_body.h:1748`
- `0x4143a0` `HudUiMgr::IsLocalPlayerFirstInStatsList` -> `src/Battlesport/hud_runtime_layer_body.h:1762`
- `0x4143b0` `HudUi::RefreshScoreboardEntryRow` -> `src/Battlesport/hud_runtime_layer_body.h:1773`
- `0x4143c0` `HudUi::RemoveScoreboardEntryRow` -> `src/Battlesport/hud_runtime_layer_body.h:1784`
- `0x4143d0` `GameNet::BeginChatCompose` -> `src/Battlesport/hud_runtime_layer_body.h:1797`
- `0x414550` `GameNet::ChatComposeKeyCallback` -> `src/Battlesport/hud_runtime_layer_body.h:1837`
- `0x414590` `GameNet::EndChatComposeAndSend` -> `src/Battlesport/hud_runtime_layer_body.h:1858`
- `0x414660` `GameNet::EndChatComposeAndSendThunk` -> `src/Battlesport/hud_runtime_layer_body.h:1899`
- `0x414670` `HudUiTripletEntries::GetCount` -> `src/Battlesport/hud_runtime_layer_body.h:1909`
- `0x4146a0` `HudUiTripletEntries::CopyRange` -> `src/Battlesport/hud_runtime_layer_body.h:1922`
- `0x4146e0` `HudUiTripletEntries::FillN` -> `src/Battlesport/hud_runtime_layer_body.h:1944`
- `0x414710` `HudUiListMenuEntry::SortRange` -> `src/Battlesport/hud_runtime_layer_body.h:1965`
- `0x414930` `HudUiListMenuEntry::InsertPivotIntoSortedPrefix` -> `src/Battlesport/hud_runtime_layer_body.h:2028`
- `0x414980` `HudUiListMenuEntry::InsertionSortRange` -> `src/Battlesport/hud_runtime_layer_body.h:2051`
- `0x4184e0` `HudSensorTracker::AdvanceObjectiveState` -> `src/Battlesport/hud_sensor_tracker_body.h:3717`
- `0x418620` `HudSensorTracker::SetObjectiveReviewVisible` -> `src/Battlesport/hud_sensor_tracker_body.h:3658`
- `0x418760` `HudSensorTracker::SetObjectivePanelVisible` -> `src/Battlesport/hud_sensor_tracker_body.h:3881`
- `0x418c30` `HudSensorTracker::FindAndHighlightFirstIncompleteObjective` -> `src/Battlesport/hud_sensor_tracker_body.h:3622`
- `0x419380` `HudSensorTracker::OnObjectiveReadSoundEvent` -> `src/Battlesport/hud_sensor_tracker_body.h:3698`
- `0x42bf40` `HudUi::PlayPowerupSfx` -> `src/GameZRecoil/zUI/zui.cpp:13466`
- `0x4348b0` `HudUiSaveLoadGameNameInput::OnActivate` -> `src/Battlesport/recoil_app_late_body.h:855`
- `0x4348f0` `HudUiSaveLoadGameNameInput::OnRawKeyboardEvent` -> `src/Battlesport/recoil_app_late_body.h:866`
- `0x434950` `HudUiSaveLoadListItem::Draw` -> `src/Battlesport/recoil_app_late_body.h:772`
- `0x4349a0` `HudUiSaveLoadDialog::Destructor` -> `src/Battlesport/recoil_app_late_body.h:1759`
- `0x434a80` `HudUiSaveGameDialog::Destructor` -> `src/Battlesport/recoil_app_late_body.h:1670`
- `0x435220` `HudUiSaveGamePrimaryActionButton::OnActivate` -> `src/Battlesport/recoil_app_late_body.h:925`
- `0x4bc480` `HudUiCircle::HudUiCircle` -> `src/GameZRecoil/zUI/zui.cpp:4738`
- `0x4bcf80` `HudUiBar::SetPointXY` -> `src/GameZRecoil/zUI/zui.cpp:11117`
- `0x4bd280` `HudUi::PushTopMessageLine` -> `src/GameZRecoil/zUI/zui.cpp:13484`
- `0x4bdc70` `HudWeatherFx::Constructor` -> `src/Battlesport/hud.cpp:2310`
- `0x4bde20` `HudWeatherFx::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:2401`
- `0x4bde40` `HudWeatherFx::Destructor` -> `src/Battlesport/hud.cpp:2417`
- `0x4bdee0` `HudWeatherFx::ResetParticleSlot` -> `src/Battlesport/hud.cpp:2476`
- `0x4bdfd0` `HudWeatherFx::ApplyPass3` -> `src/Battlesport/hud.cpp:2505`
- `0x4be210` `HudWeatherFx::ArePointBatchInsideRect` -> `src/Battlesport/hud.cpp:2445`
- `0x4be280` `HudWeatherFxSnow::Constructor` -> `src/Battlesport/hud.cpp:2607`
- `0x4be2c0` `HudWeatherFxSnow::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:2623`
- `0x4be2e0` `HudWeatherFxSnow::Destructor` -> `src/Battlesport/hud.cpp:2639`
- `0x4be2f0` `HudWeatherFxSnow::Update` -> `src/Battlesport/hud.cpp:2648`
- `0x4be810` `HudWeatherFxRain::Constructor` -> `src/Battlesport/hud.cpp:2809`
- `0x4be850` `HudWeatherFxRain::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:2825`
- `0x4be870` `HudWeatherFxRain::Destructor` -> `src/Battlesport/hud.cpp:2841`
- `0x4be880` `HudWeatherFxRain::Update` -> `src/Battlesport/hud.cpp:2850`

## Battlesport/hud_ui_dialogs.cpp

- `0x408a30` `HudUiControlsDialog::Constructor` -> `src/Battlesport/hud.cpp:4013`
- `0x408c20` `HudUiControlsDialog_CommandsWidget::OnActivate` -> `src/Battlesport/hud.cpp:4083`
- `0x408c40` `HudUiControlsDialog::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:4094`
- `0x408c70` `HudUiControlsDialog::Destructor` -> `src/Battlesport/hud.cpp:4112`

## Battlesport/HudCmdBindButton.cpp

- `0x40a940` `HudCmdCommandList::Destructor` -> `src/Battlesport/hud_command_binding_layer_body.h:155`
- `0x40aa30` `HudCmdKeyAButton::Destructor` -> `src/Battlesport/hud_command_binding_layer_body.h:201`
- `0x40ab20` `HudCmdKeyBButton::Destructor` -> `src/Battlesport/hud_command_binding_layer_body.h:247`
- `0x40ac10` `HudCmdJoyButton::Destructor` -> `src/Battlesport/hud_command_binding_layer_body.h:293`
- `0x40ad00` `HudCmdMouseButton::Destructor` -> `src/Battlesport/hud_command_binding_layer_body.h:339`
- `0x40bdc0` `zUtil_StdPtrVector_Clear` -> `src/Battlesport/hud_command_binding_layer_body.h:1276`
- `0x40be60` `HudCmdBindingEntry::CopyRange` -> `src/Battlesport/hud_command_binding_layer_body.h:1334`
- `0x40c280` `HudCmdBindButtonBase::DestructorCore` -> `src/Battlesport/hud_command_binding_layer_body.h:1497`
- `0x4b8de0` `HudCmdBindButtonBase::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:10379`
- `0x4b90e0` `HudCmdBindButtonBase::RebuildBindingSlotWidgets` -> `src/GameZRecoil/zUI/zui.cpp:10318`
- `0x4b9320` `HudCmdBindButtonBase::OnSelectedIndexChanged` -> `src/GameZRecoil/zUI/zui.cpp:10221`
- `0x4b9330` `HudCmdBindButtonBase::SetSelectedEntry` -> `src/GameZRecoil/zUI/zui.cpp:10232`

## Battlesport/HudCmdDialog.cpp

- `0x40a5b0` `HudCmdDialog::Constructor` -> `src/Battlesport/hud_command_binding_layer_body.h:5`
- `0x40a920` `HudCmdDialog::ScalarDeletingDestructor` -> `src/Battlesport/hud_command_binding_layer_body.h:138`
- `0x40adf0` `HudCmdDialog::Destructor` -> `src/Battlesport/hud_command_binding_layer_body.h:385`
- `0x40b140` `HudCmdDialog::UpdateCaptureState` -> `src/Battlesport/hud_command_binding_layer_body.h:495`
- `0x40b3e0` `HudCmdDialog::ApplyPrimaryKeyRebind` -> `src/Battlesport/hud_command_binding_layer_body.h:603`
- `0x40b460` `HudCmdDialog::ApplySecondaryKeyRebind` -> `src/Battlesport/hud_command_binding_layer_body.h:639`
- `0x40b4e0` `HudCmdDialog::ApplyJoystickButtonRebind` -> `src/Battlesport/hud_command_binding_layer_body.h:675`
- `0x40b560` `HudCmdDialog::ApplyMouseButtonRebind` -> `src/Battlesport/hud_command_binding_layer_body.h:708`
- `0x40b5e0` `HudCmdDialog::SelectGroupRelative` -> `src/Battlesport/hud_command_binding_layer_body.h:741`
- `0x40b630` `HudCmdDialog::SelectCommandRelative` -> `src/Battlesport/hud_command_binding_layer_body.h:762`
- `0x40b680` `HudCmdDialog::RebuildCommandBindingListsForGroup` -> `src/Battlesport/hud_command_binding_layer_body.h:791`
- `0x40b930` `HudCmdResetButton::OnActivate` -> `src/Battlesport/hud_command_binding_layer_body.h:941`
- `0x40b960` `HudCmdSetListWidget::OnActivate` -> `src/Battlesport/hud_command_binding_layer_body.h:954`
- `0x40b980` `HudCmdDialog::OnCommandSelectionChanged` -> `src/Battlesport/hud_command_binding_layer_body.h:965`
- `0x40ba30` `HudCmdKeyAButton::OnBeginCapture` -> `src/Battlesport/hud_command_binding_layer_body.h:998`
- `0x40ba60` `HudCmdKeyAButton::OnClearBinding` -> `src/Battlesport/hud_command_binding_layer_body.h:1009`
- `0x40ba90` `HudCmdBindButtonBase::OnSelectionChangedRefresh` -> `src/Battlesport/hud_command_binding_layer_body.h:1023`
- `0x40bab0` `HudCmdKeyBButton::OnBeginCapture` -> `src/Battlesport/hud_command_binding_layer_body.h:1034`
- `0x40bae0` `HudCmdKeyBButton::OnClearBinding` -> `src/Battlesport/hud_command_binding_layer_body.h:1045`
- `0x40bb00` `HudCmdJoyButton::OnBeginCapture` -> `src/Battlesport/hud_command_binding_layer_body.h:1057`
- `0x40bb30` `HudCmdJoyButton::OnClearBinding` -> `src/Battlesport/hud_command_binding_layer_body.h:1068`
- `0x40bb50` `HudCmdMouseButton::OnBeginCapture` -> `src/Battlesport/hud_command_binding_layer_body.h:1081`
- `0x40bb80` `HudCmdMouseButton::OnClearBinding` -> `src/Battlesport/hud_command_binding_layer_body.h:1096`
- `0x40bba0` `HudCmdNextSetButton::OnActivate` -> `src/Battlesport/hud_command_binding_layer_body.h:1112`
- `0x40bbc0` `HudCmdPrevSetButton::OnActivate` -> `src/Battlesport/hud_command_binding_layer_body.h:1122`
- `0x40bbe0` `HudCmdNextCommandButton::OnActivate` -> `src/Battlesport/hud_command_binding_layer_body.h:1132`
- `0x40bc00` `HudCmdPrevCommandButton::OnActivate` -> `src/Battlesport/hud_command_binding_layer_body.h:1142`
- `0x40bc20` `HudCmdDialogState::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud_command_binding_layer_body.h:1152`
- `0x40bc30` `HudCmdDialogState::StaticInit` -> `src/Battlesport/hud_command_binding_layer_body.h:1162`
- `0x40bc40` `HudCmdDialogState::RegisterAtExit` -> `src/Battlesport/hud_command_binding_layer_body.h:1171`
- `0x40bc50` `HudCmdDialogState::AtExitDestructor` -> `src/Battlesport/hud_command_binding_layer_body.h:1180`
- `0x40bc60` `HudCmdDialogState::HudCmdDialogState` -> `src/Battlesport/hud_command_binding_layer_body.h:1198`
- `0x40bc90` `HudCmdDialogState::~HudCmdDialogState` -> `src/Battlesport/hud_command_binding_layer_body.h:1207`
- `0x40bcf0` `HudCmdDialogState::OnTryBecomeCurrent` -> `src/Battlesport/hud_command_binding_layer_body.h:1220`
- `0x40bd60` `HudCmdDialogState::OnDeactivate` -> `src/Battlesport/hud_command_binding_layer_body.h:1238`
- `0x40bda0` `HudCmdDialogState::QueueEnter` -> `src/Battlesport/hud_command_binding_layer_body.h:1264`
- `0x40be00` `HudCmdBinding::DestroyRange` -> `src/Battlesport/hud_command_binding_layer_body.h:1301`

## Battlesport/HudConfirmQuitDialog.cpp

- `0x415740` `HudUiConfirmQuitOkButton::OnActivate` -> `src/Battlesport/hud.cpp:4529`
- `0x415810` `RecoilStateConfirmQuit::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:4574`
- `0x415820` `RecoilStateConfirmQuit::StaticInit` -> `src/Battlesport/hud.cpp:4584`
- `0x415830` `RecoilStateConfirmQuit::RegisterAtExit` -> `src/Battlesport/hud.cpp:4593`
- `0x415840` `RecoilStateConfirmQuit::AtExitDestructor` -> `src/Battlesport/hud.cpp:4602`
- `0x415850` `RecoilStateConfirmQuit::RecoilStateConfirmQuit` -> `src/Battlesport/hud.cpp:4611`
- `0x4158f0` `RecoilStateConfirmQuit::OnTryBecomeCurrent` -> `src/Battlesport/hud.cpp:4639`
- `0x415960` `RecoilStateConfirmQuit::OnDeactivate` -> `src/Battlesport/hud.cpp:4657`
- `0x4159b0` `RecoilStateConfirmQuit::QueueEnter` -> `src/Battlesport/hud.cpp:4684`

## Battlesport/HudOptionsDialog.cpp

- `0x40c6e0` `HudUiOptionsPanelBackButton::OnActivate` -> `src/Battlesport/hud.cpp:3234`
- `0x40c720` `HudOptionsDialog::HudOptionsDialog` -> `src/Battlesport/hud.cpp:3249`
- `0x40cf60` `HudOptionsDialog::~HudOptionsDialog` -> `src/Battlesport/hud.cpp:3892`
- `0x40d070` `HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:3901`
- `0x40d080` `HudUiOptionsPanelOverlayOwner::StaticInit` -> `src/Battlesport/hud.cpp:3911`
- `0x40d090` `HudUiOptionsPanelOverlayOwner::RegisterAtExit` -> `src/Battlesport/hud.cpp:3920`
- `0x40d0a0` `HudUiOptionsPanelOverlayOwner::AtExitDestructor` -> `src/Battlesport/hud.cpp:3929`
- `0x40d0b0` `HudUiOptionsPanelOverlayOwner::HudUiOptionsPanelOverlayOwner` -> `src/Battlesport/hud.cpp:3938`
- `0x40d0e0` `HudUiOptionsPanelOverlayOwner::~HudUiOptionsPanelOverlayOwner` -> `src/Battlesport/hud.cpp:3947`
- `0x40d150` `HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent` -> `src/Battlesport/hud.cpp:3966`
- `0x40d1c0` `HudUiOptionsPanelOverlayOwner::QueueEnter` -> `src/Battlesport/hud.cpp:3978`

## Battlesport/HudScoreboard.cpp

- `0x40eab0` `HudScoreboard::SetScaleAndRebuild` -> `src/GameZRecoil/zUI/zui.cpp:3156`
- `0x40eae0` `HudScoreboard::DispatchSetScale` -> `src/GameZRecoil/zUI/zui.cpp:3168`

## Battlesport/hudui.cpp

- `0x426150` `HudUi::HandleHotkeyCommand` -> `src/GameZRecoil/zUI/zui.cpp:13362`
- `0x4b3e90` `HudUiWidget::InvalidateRect` -> `src/GameZRecoil/zUI/zui.cpp:7286`
- `0x4b3fb0` `HudUiWidget::Draw` -> `src/GameZRecoil/zUI/zui.cpp:10555`

## Battlesport/HudUi.cpp

- `0x4bc810` `HudUiContainer::FindChildWithPrev` -> `src/GameZRecoil/zUI/zui.cpp:5345`
- `0x4bc860` `HudUiContainer::RemoveChild` -> `src/GameZRecoil/zUI/zui.cpp:5381`

## Battlesport/hudui_background.cpp

- `0x4b9540` `HudUiBackground::HudUiBackground` -> `src/GameZRecoil/zUI/zui.cpp:6380`
- `0x4b9740` `HudUiBackground::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:6450`
- `0x4b9760` `HudUiBackground::~HudUiBackground` -> `src/GameZRecoil/zUI/zui.cpp:6428`
- `0x4b98d0` `HudUiBackground::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:6519`
- `0x4b9900` `HudUiBackground::LoadZrdAndSection` -> `src/GameZRecoil/zUI/zui.cpp:6542`
- `0x4ba020` `HudUiTransitionTextPanel::HudUiTransitionTextPanel` -> `src/GameZRecoil/zUI/zui.cpp:6862`
- `0x4bf980` `HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget` -> `src/GameZRecoil/zUI/zui.cpp:7351`
- `0x4bfa20` `HudUiBackgroundCursorWidget::~HudUiBackgroundCursorWidget` -> `src/GameZRecoil/zUI/zui.cpp:7371`
- `0x4bfa50` `HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh` -> `src/GameZRecoil/zUI/zui.cpp:7382`
- `0x4bfa70` `HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged` -> `src/GameZRecoil/zUI/zui.cpp:7395`
- `0x4bfa90` `HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh` -> `src/GameZRecoil/zUI/zui.cpp:7408`
- `0x4bfae0` `HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh` -> `src/GameZRecoil/zUI/zui.cpp:7432`
- `0x4bfb70` `HudUiBackgroundCursorWidget::SetPos` -> `src/GameZRecoil/zUI/zui.cpp:7472`
- `0x4bfba0` `HudUiBackgroundCursorWidget::RebuildCapturedImage` -> `src/GameZRecoil/zUI/zui.cpp:7491`
- `0x4bfc50` `HudUiBackgroundCursorWidget::Draw` -> `src/GameZRecoil/zUI/zui.cpp:7529`
- `0x4bfc60` `HudUiBackgroundCursorWidget::DrawBase` -> `src/GameZRecoil/zUI/zui.cpp:7538`

## Battlesport/hudui_element.cpp

- `0x4b4070` `HudUiElement::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:4289`

## Battlesport/HudUi_NetExit.cpp

- `0x41bd80` `HudUiNetExitPanel::Constructor` -> `src/Battlesport/hud_ui_net_exit_panel_body.h:25`
- `0x41be70` `HudUiNetExitPanel_ExitButton::OnActivate` -> `src/Battlesport/hud_ui_net_exit_panel_body.h:101`
- `0x41beb0` `HudUiNetExitPanel::Destructor` -> `src/Battlesport/hud_ui_net_exit_panel_body.h:68`
- `0x41bf10` `HudUiNetExitPanel_ResumeWidget::OnActivate` -> `src/Battlesport/hud_ui_net_exit_panel_body.h:113`
- `0x41bf40` `HudUiNetExitPanel_ResumeWidget::OnShowPreview` -> `src/Battlesport/hud_ui_net_exit_panel_body.h:125`
- `0x41bfa0` `HudUiNetExitPanel_ResumeWidget::OnHidePreview` -> `src/Battlesport/hud_ui_net_exit_panel_body.h:158`
- `0x41c000` `HudUiNetExitPanel::CreateGlobal` -> `src/Battlesport/hud_ui_net_exit_panel_body.h:185`
- `0x41c070` `HudUiNetExitPanel::Show` -> `src/Battlesport/hud_ui_net_exit_panel_body.h:202`
- `0x41c080` `HudUiNetExitPanel::Tick` -> `src/Battlesport/hud_ui_net_exit_panel_body.h:211`
- `0x41c0a0` `HudUiNetExitPanel::DestroyGlobal` -> `src/Battlesport/hud_ui_net_exit_panel_body.h:221`

## Battlesport/hudui_saveload.cpp

- `0x434680` `HudUiSaveGameDialog::HudUiSaveGameDialog` -> `src/Battlesport/recoil_app_late_body.h:1606`
- `0x434920` `HudUiSaveLoadListItem::HudUiSaveLoadListItem` -> `src/Battlesport/recoil_app_late_body.h:757`
- `0x434ee0` `HudUiSaveLoadDialog::InitializeFileEntries` -> `src/Battlesport/recoil_app_late_body.h:1327`
- `0x4353f0` `HudUiSaveLoadDialog::SetSelectedEntryIndex` -> `src/Battlesport/recoil_app_late_body.h:1364`
- `0x4355e0` `HudUiSaveLoadDialog::RefreshSaveFileList` -> `src/Battlesport/recoil_app_late_body.h:1156`
- `0x4362f0` `SortEntryRange` -> `src/Battlesport/recoil_app_late_body.h:1010`
- `0x436530` `InsertEntryIntoSortedPrefix` -> `src/Battlesport/recoil_app_late_body.h:953`
- `0x436580` `PartitionEntriesByPivot` -> `src/Battlesport/recoil_app_late_body.h:974`

## Battlesport/HudUiBackgroundConfirmQuit.cpp

- `0x415680` `HudUiBackgroundConfirmQuit::Constructor` -> `src/Battlesport/hud.cpp:4497`
- `0x415790` `HudUiBackgroundConfirmQuit::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:4546`
- `0x4157b0` `HudUiBackgroundConfirmQuit::Destructor` -> `src/Battlesport/hud.cpp:4563`

## Battlesport/HudUiCheatCode.cpp

- `0x406d20` `HudUiCheatCodeDialog::HudUiCheatCodeDialog` -> `src/Battlesport/hud.cpp:2041`
- `0x406e10` `HudUiCheatCodeDialog::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:2069`
- `0x406e30` `HudUiCheatCodeDialog::~HudUiCheatCodeDialog` -> `src/Battlesport/hud.cpp:2086`
- `0x406e90` `RecoilStateCheatCode::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:2093`
- `0x406ea0` `RecoilStateCheatCode::ConstructGlobal` -> `src/Battlesport/hud.cpp:2103`
- `0x406eb0` `RecoilStateCheatCode::StaticInit` -> `src/Battlesport/hud.cpp:2112`
- `0x406ec0` `RecoilStateCheatCode::AtExitDestructor` -> `src/Battlesport/hud.cpp:2121`
- `0x406ed0` `RecoilStateCheatCode::RecoilStateCheatCode` -> `src/Battlesport/hud.cpp:2130`
- `0x406ee0` `RecoilStateCheatCode::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:2140`
- `0x4070e0` `HudUiCheatCodeTitleWidget::OnActivate` -> `src/Battlesport/hud.cpp:2225`

## Battlesport/HudUiCreditsPanel.cpp

- `0x409040` `HudUiCreditsPanel::HudUiCreditsPanel` -> `src/GameZRecoil/zUI/zui.cpp:5526`
- `0x409570` `HudUiZrdScrollingText::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:5767`
- `0x409b90` `HudUiPanelSpan::InsertN` -> `src/GameZRecoil/zUI/zui.cpp:5992`
- `0x409f00` `HudUiPanelSpanVec::InsertN` -> `src/GameZRecoil/zUI/zui.cpp:6094`

## Battlesport/HudUiElement.cpp

- `0x4b41e0` `HudUiElement::Update` -> `src/GameZRecoil/zUI/zui.cpp:4384`
- `0x4b4280` `HudUiElement::SetTimer` -> `src/GameZRecoil/zUI/zui.cpp:4436`

## Battlesport/HudUiListMenu.cpp

- `0x40d220` `HudUiListMenuEntry::CompareSortKey` -> `src/GameZRecoil/zUI/zui.cpp:857`

## Battlesport/HudUiLoadGameDialog.cpp

- `0x434dc0` `HudUiLoadGameDialog::ProcessDialogResult` -> `src/Battlesport/recoil_app_late_body.h:1523`
- `0x434df0` `HudUiLoadGameDialog::Destructor` -> `src/Battlesport/recoil_app_late_body.h:1782`

## Battlesport/HudUiMainMenuDialog.cpp

- `0x414b60` `HudUiMainMenuDialog::CanLoadGame` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:200`
- `0x414b90` `HudUiMainMenuDialog::CanSaveGame` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:225`
- `0x414bc0` `HudUiMainMenuDialog::HudUiMainMenuDialog` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:250`
- `0x414f40` `HudUiMainMenuDialog_CreditsButton::OnActivate` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:430`
- `0x414f60` `HudUiMainMenuDialog_SaveButton::OnActivate` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:440`
- `0x414f80` `HudUiMainMenuDialog_NewGameButton::OnActivate` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:452`
- `0x414fa0` `HudUiMenuBackButton::OnActivate` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:462`
- `0x414fc0` `HudUiMainMenuDialog_OptionsButton::OnActivate` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:473`
- `0x414fe0` `HudUiMainMenuDialog_QuitButton::OnActivate` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:483`
- `0x415000` `HudUiMainMenuDialog_ControlsButton::OnActivate` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:493`
- `0x415020` `HudUiMainMenuDialog::ScalarDeletingDestructor` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:503`
- `0x415040` `HudUiMainMenuDialog::~HudUiMainMenuDialog` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:516`
- `0x415140` `HudUiMainMenuDialog_LoadButton::OnActivate` -> `src/Battlesport/hud_ui_main_menu_dialog_body.h:12`

## Battlesport/HudUiMessageBoxDialog.cpp

- `0x438350` `HudUi::ShowMessageBox` -> `src/Battlesport/hud_ui_message_box_dialog_body.h:480`
- `0x4bf060` `HudUiMessageBoxDialog::Constructor` -> `src/Battlesport/hud_ui_message_box_dialog_body.h:103`
- `0x4bf630` `HudUiMessageBoxDialog::RunModal` -> `src/Battlesport/hud_ui_message_box_dialog_body.h:318`
- `0x4bf800` `HudUiMessageBoxOkButton::OnActivate` -> `src/Battlesport/hud_ui_message_box_dialog_body.h:441`
- `0x4bf820` `HudUiMessageBoxCancelButton::OnActivate` -> `src/Battlesport/hud_ui_message_box_dialog_body.h:460`

## Battlesport/HudUiMgrSensor.cpp

- `0x412070` `HudUiMgrSensor::PlaceTrackCounterWidget` -> `src/Battlesport/hud_runtime_layer_body.h:2951`
- `0x4122c0` `HudUiMgrSensor::PlaceTrackMarker` -> `src/Battlesport/hud_runtime_layer_body.h:3079`
- `0x41ebd0` `HudUiMgrSensor::TrackList_Reset` -> `src/GameZRecoil/zUI/zui.cpp:1992`
- `0x438920` `HudUiMgrSensor::TrackList_Add` -> `src/GameZRecoil/zUI/zui.cpp:2015`
- `0x439690` `HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag` -> `src/GameZRecoil/zUI/zui.cpp:2049`

## Battlesport/HudUiMpExitDialog.cpp

- `0x419500` `HudUiMpExitDialog::LoadLayout` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:86`
- `0x419650` `HudUiMpExitDialog::UnloadLayout` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:27`
- `0x419690` `HudUiMpExitDialog::Update` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:45`
- `0x419740` `RecoilApp_MpExitDialogState::OnEnter` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:222`
- `0x419800` `HudUiMpExitDialog_MpNewGameButton::OnActivate` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:168`
- `0x419830` `HudUiMpExitDialog_MpExitButton::OnActivate` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:182`
- `0x419850` `HudUiMpExitDialog::ScalarDeletingDestructorThunk` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:206`
- `0x419870` `HudUiMpExitDialog::Destructor` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:195`
- `0x4198d0` `RecoilApp_MpExitDialogState::OnTryBecomeCurrent` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:242`
- `0x419940` `RecoilApp_MpExitDialogState::OnDeactivate` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:269`
- `0x419990` `RecoilApp_MpExitDialogState::OnUpdateShouldQuit` -> `src/Battlesport/hud_ui_mp_exit_dialog_body.h:289`

## Battlesport/HudUiNewGamePanel.cpp

- `0x41c270` `HudUiNewGamePanel_StartButton::OnActivate` -> `src/Battlesport/hud.cpp:3209`
- `0x41c290` `HudUiNewGamePanel::HudUiNewGamePanel` -> `src/Battlesport/hud.cpp:3100`
- `0x41c3b0` `HudUiNewGamePanel_NameInput::OnActivate` -> `src/Battlesport/hud.cpp:3140`
- `0x41c3e0` `HudUiNewGamePanel::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:3165`
- `0x41c400` `HudUiNewGamePanel::Destructor` -> `src/Battlesport/hud.cpp:3152`
- `0x41c4e0` `HudUiNewGamePanel::SyncIntensityFromDifficulty` -> `src/Battlesport/hud.cpp:3181`
- `0x41c500` `HudUiNewGamePanel::StartGameFromFields` -> `src/Battlesport/hud.cpp:3190`
- `0x41c560` `HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent` -> `src/Battlesport/hud.cpp:3087`
- `0x41c5e0` `HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:3020`
- `0x41c5f0` `HudUiNewGamePanelOverlayOwner::StaticInit` -> `src/Battlesport/hud.cpp:3030`
- `0x41c630` `HudUiNewGamePanelOverlayOwner::~HudUiNewGamePanelOverlayOwner` -> `src/Battlesport/hud.cpp:3068`
- `0x41c6a0` `HudUiNewGamePanelOverlayOwner::RegisterAtExit` -> `src/Battlesport/hud.cpp:3039`
- `0x41c6b0` `HudUiNewGamePanelOverlayOwner::AtExitDestructor` -> `src/Battlesport/hud.cpp:3048`
- `0x41c6c0` `HudUiNewGamePanelOverlayOwner::QueueEnter` -> `src/Battlesport/hud.cpp:3008`

## Battlesport/HudUiPanel.cpp

- `0x409910` `HudUiPanelSpan::Clear` -> `src/GameZRecoil/zUI/zui.cpp:5939`
- `0x409b60` `HudUiPanelLayoutEntry::DestroyRange` -> `src/GameZRecoil/zUI/zui.cpp:5976`
- `0x40a170` `HudUiPanelLayoutEntry::CopyAssignRange` -> `src/GameZRecoil/zUI/zui.cpp:6184`
- `0x40a1e0` `HudUiPanelLayoutEntry::CopyAssign` -> `src/GameZRecoil/zUI/zui.cpp:6207`
- `0x40a210` `HudUiPanelLayoutEntry::CopyConstruct` -> `src/GameZRecoil/zUI/zui.cpp:6221`
- `0x40a240` `HudUiPanelSpan::CopyInit` -> `src/GameZRecoil/zUI/zui.cpp:6235`
- `0x40a300` `HudUiPanelSpan::CopyFrom` -> `src/GameZRecoil/zUI/zui.cpp:6265`
- `0x4ba850` `HudUiPanel::CopyConstructCore` -> `src/GameZRecoil/zUI/zui.cpp:12385`
- `0x4ba9e0` `HudUiPanel::ConstructorCopy` -> `src/GameZRecoil/zUI/zui.cpp:12434`
- `0x4babb0` `HudUiPanel::SetFont` -> `src/GameZRecoil/zUI/zui.cpp:12627`
- `0x4bb440` `HudUiPanel::GetLastTextPtr` -> `src/GameZRecoil/zUI/zui.cpp:12581`
- `0x4bc9b0` `HudUiTransitionTextPanel::SetFlashColorAndRate` -> `src/GameZRecoil/zUI/zui.cpp:9260`

## Battlesport/HudUiSaveLoadDialog.cpp

- `0x434970` `HudUiLoadGameDialog::OnPrimaryActionThunk` -> `src/Battlesport/recoil_app_late_body.h:1532`
- `0x434b90` `HudUiLoadGameDialog::HudUiLoadGameDialog` -> `src/Battlesport/recoil_app_late_body.h:1695`
- `0x434fb0` `HudUiSaveLoadDialog::DeleteSaveFile` -> `src/Battlesport/recoil_app_late_body.h:794`
- `0x435140` `HudUiSaveLoadDeleteButton::OnActivate` -> `src/Battlesport/recoil_app_late_body.h:884`
- `0x435160` `HudUiSaveLoadNextButton::OnActivate` -> `src/Battlesport/recoil_app_late_body.h:895`
- `0x4351b0` `HudUiSaveLoadPrevButton::OnActivate` -> `src/Battlesport/recoil_app_late_body.h:910`
- `0x435200` `HudUiLoadGamePrimaryActionButton::OnActivate` -> `src/Battlesport/recoil_app_late_body.h:939`
- `0x435240` `HudUiLoadGameDialog::OnPrimaryAction` -> `src/Battlesport/recoil_app_late_body.h:1541`
- `0x435a10` `HudUiSaveLoadListItem::OnActivate` -> `src/Battlesport/recoil_app_late_body.h:782`
- `0x435a70` `HudUiSaveLoadDialog::ProcessDialogResult` -> `src/Battlesport/recoil_app_late_body.h:1454`

## Battlesport/HudUiTextLabel.cpp

- `0x4bcbe0` `HudUiTextLabel::CopyConstructor` -> `src/GameZRecoil/zUI/zui.cpp:12125`
- `0x4bcc80` `HudUiTextLabel::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:12147`

## Battlesport/HudUiTextStack4.cpp

- `0x4bd110` `HudUiTextStack4::SetFontAll` -> `src/GameZRecoil/zUI/zui.cpp:13528`
- `0x4bd160` `HudUiTextStack4::PushLine` -> `src/GameZRecoil/zUI/zui.cpp:13135`

## Battlesport/HudUiTransitionTextPanel.cpp

- `0x4bc9f0` `HudUiTransitionTextPanel::Update` -> `src/GameZRecoil/zUI/zui.cpp:9144`

## Battlesport/HudUiTriplet.cpp

- `0x40d1e0` `HudUiTriplet::StaticInitWndClassNameAndRegisterAtExit` -> `src/GameZRecoil/zUI/zui.cpp:801`
- `0x40d1f0` `HudUiTriplet::ConstructWndClassName` -> `src/GameZRecoil/zUI/zui.cpp:812`
- `0x40d200` `HudUiTriplet::RegisterWndClassNameDtorAtExit` -> `src/GameZRecoil/zUI/zui.cpp:822`
- `0x40d210` `HudUiTriplet::DestroyWndClassName` -> `src/GameZRecoil/zUI/zui.cpp:832`
- `0x40e070` `HudUiTriplet::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:2829`
- `0x40e140` `HudUiTriplet::RebuildDisplay` -> `src/GameZRecoil/zUI/zui.cpp:2867`
- `0x40e590` `HudUiTriplet::AddEntry` -> `src/GameZRecoil/zUI/zui.cpp:3032`
- `0x40e800` `HudUiTriplet::UpdateEntryData` -> `src/GameZRecoil/zUI/zui.cpp:3065`
- `0x40e880` `HudUiTriplet::RemoveEntry` -> `src/GameZRecoil/zUI/zui.cpp:3088`
- `0x40e910` `HudUiTriplet::InterpolateLayout` -> `src/GameZRecoil/zUI/zui.cpp:3118`
- `0x40ea60` `HudUiTriplet::IsLocalPlayerFirstEntry` -> `src/GameZRecoil/zUI/zui.cpp:3137`

## Battlesport/HudUiZrdWidget.cpp

- `0x4b7020` `HudUiCheckToggleWidget::~HudUiCheckToggleWidget` -> `src/GameZRecoil/zUI/zui.cpp:8555`
- `0x4b7340` `HudUiCheckToggleWidget::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:8737`
- `0x4b7de0` `HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget` -> `src/GameZRecoil/zUI/zui.cpp:8923`

## Battlesport/map.cpp

- `0x4176f0` `HudSensorTracker::ResetMissionState` -> `src/Battlesport/hud_sensor_tracker_body.h:2449`
- `0x417810` `HudSensorTracker::LoadMissionCoreResources` -> `src/Battlesport/hud_sensor_tracker_body.h:2711`
- `0x417a00` `HudSensorTracker::InitMissionGameplaySystems` -> `src/Battlesport/hud_sensor_tracker_body.h:2791`
- `0x417ca0` `HudSensorTracker::OnObjectiveCommand` -> `src/Battlesport/hud_sensor_tracker_body.h:3768`
- `0x417d40` `HudSensorTracker::ShutdownMissionGameplaySystems` -> `src/Battlesport/hud_sensor_tracker_body.h:2916`
- `0x418730` `HudSensorTracker::Command_ToggleObjectivePanel` -> `src/Battlesport/hud_sensor_tracker_body.h:3870`
- `0x4188f0` `HudSensorTracker::Command_ShowObjectivePickupInfo` -> `src/Battlesport/hud_sensor_tracker_body.h:3969`
- `0x418940` `HudSensorTracker::ShowObjectivePickupInfo` -> `src/Battlesport/hud_sensor_tracker_body.h:3988`
- `0x418d40` `HudSensorTracker::UpdateObjectiveFlow` -> `src/Battlesport/hud_sensor_tracker_body.h:4104`
- `0x418fb0` `HudSensorTracker::SaveAndQueueMissionState` -> `src/Battlesport/hud_sensor_tracker_body.h:3539`
- `0x419050` `HudSensorTracker::LoadMissionWeatherFx` -> `src/Battlesport/hud_sensor_tracker_body.h:3005`
- `0x4193c0` `HudSensorTracker::LoadRaceCheckpointMeta` -> `src/Battlesport/hud_sensor_tracker_body.h:3423`

## Battlesport/mission.cpp

- `0x417ee0` `HudSensorTracker::UnloadObjectives` -> `src/Battlesport/hud_sensor_tracker_body.h:3133`
- `0x417f90` `HudSensorTracker::LoadObjectivesFromPath` -> `src/Battlesport/hud_sensor_tracker_body.h:3160`
- `0x418230` `HudSensorTracker::LoadObjectivesFromZrd` -> `src/Battlesport/hud_sensor_tracker_body.h:3315`

## Battlesport/pickup.cpp

- `0x41cc10` `PickupSpawnList::Primary_Init` -> `src/Battlesport/pickup.cpp:982`
- `0x41cc40` `PickupSpawnList::NetCopy_Init` -> `src/Battlesport/pickup.cpp:993`
- `0x41cc70` `PickupRespawnQueue::Init` -> `src/Battlesport/pickup.cpp:1004`
- `0x41cca0` `PickupTypeTable::FreeOptMeta` -> `src/Battlesport/pickup.cpp:2992`
- `0x41ccd0` `Pickup::Shutdown` -> `src/Battlesport/pickup.cpp:2980`
- `0x41ccf0` `Pickup::Init` -> `src/Battlesport/pickup.cpp:1449`
- `0x41ceb0` `zClass_Node::ClearPickupFlagsRecursive` -> `src/Battlesport/pickup.cpp:820`
- `0x41cef0` `zClass_Node::SetPickupFlagsRecursive` -> `src/Battlesport/pickup.cpp:837`
- `0x41cf30` `Pickup::ResolveOwnerFromBvolHit` -> `src/Battlesport/pickup.cpp:2450`
- `0x41cf50` `Pickup::RemoveObject` -> `src/Battlesport/pickup.cpp:2212`
- `0x41d0c0` `Pickup::OnCollected` -> `src/Battlesport/pickup.cpp:2296`
- `0x41d220` `Pickup::ApplyEffect` -> `src/Battlesport/pickup.cpp:2676`
- `0x41d650` `Pickup::GrantAmmoOrWeapon` -> `src/Battlesport/pickup.cpp:2560`
- `0x41d8a0` `PickupSpawnList::RemoveAndFreeNode` -> `src/Battlesport/pickup.cpp:1028`
- `0x41d920` `Pickup::CreateSpawnDefAndLink` -> `src/Battlesport/pickup.cpp:1819`
- `0x41da20` `Pickup::SpawnAt` -> `src/Battlesport/pickup.cpp:1982`
- `0x41dab0` `Pickup::CreateObjectInstance` -> `src/Battlesport/pickup.cpp:1770`
- `0x41db40` `PickupType::GetByIndex_Pure` -> `src/Battlesport/pickup.cpp:704`
- `0x41db60` `Pickup::AssignBvolGroupAndId` -> `src/Battlesport/pickup.cpp:1719`
- `0x41dc30` `Pickup::SpawnFromParsedZrdEntry` -> `src/Battlesport/pickup.cpp:2075`
- `0x41dc60` `Pickup::SpawnWithAirdropChute` -> `src/Battlesport/pickup.cpp:1925`
- `0x41dcf0` `Pickup::RegisterExistingObject` -> `src/Battlesport/pickup.cpp:1884`
- `0x41dd60` `PickupType::FindByLogicalName` -> `src/Battlesport/pickup.cpp:732`
- `0x41ddf0` `Pickup::SelectPuppiesZrdByDifficulty` -> `src/Battlesport/pickup.cpp:1193`
- `0x41de30` `Net::IsOptEntryActiveInAnySlot` -> `src/Battlesport/pickup.cpp:795`
- `0x41de70` `Pickup::InitAndLoadPuppySpawns` -> `src/Battlesport/pickup.cpp:1218`
- `0x41e1a0` `PickupTypeMeta::FindByName` -> `src/Battlesport/pickup.cpp:778`
- `0x41e1c0` `PickupType::GetByIndex` -> `src/Battlesport/pickup.cpp:718`
- `0x41e1e0` `PickupTypeKeyTable::FindIndex` -> `src/Battlesport/pickup.cpp:756`
- `0x41e240` `PickupSpawnList::Clear` -> `src/Battlesport/pickup.cpp:1077`
- `0x41e270` `PickupRespawnQueue::ClearAndFree` -> `src/Battlesport/pickup.cpp:1097`
- `0x41e2f0` `Pickup::RemoveOtherSpawnsWithSameOptEntry` -> `src/Battlesport/pickup.cpp:2539`
- `0x41e330` `Pickup::SetVariantFromTerrain` -> `src/Battlesport/pickup.cpp:2095`
- `0x41e430` `Pickup::SpawnListHasEntryNearXZ` -> `src/Battlesport/pickup.cpp:1367`
- `0x41e480` `Pickup::SelectNextVTOLSpawnTypeIndex` -> `src/Battlesport/pickup.cpp:1402`
- `0x41e540` `Pickup::MapVTOLDropGroupVariantToTypeIndex` -> `src/Battlesport/pickup.cpp:1387`
- `0x41e5d0` `PickupRespawnQueue::Update` -> `src/Battlesport/pickup.cpp:1140`
- `0x41e6c0` `Pickup::RespawnSpawnDef` -> `src/Battlesport/pickup.cpp:2149`
- `0x41e780` `Pickup::ArchiveWriteAll` -> `src/Battlesport/pickup.cpp:2883`
- `0x41e840` `Pickup::ArchiveReadRecord` -> `src/Battlesport/pickup.cpp:2924`
- `0x41e890` `Pickup::ReconcilePrimaryAndNetworkCopySpawnLists` -> `src/Battlesport/pickup.cpp:2388`
- `0x41e900` `Pickup::SpawnListContainsPickupId` -> `src/Battlesport/pickup.cpp:2419`
- `0x41e930` `Pickup::FindSpawnByPickupId` -> `src/Battlesport/pickup.cpp:2468`
- `0x41e950` `Pickup::GetSpawnDefFromNode` -> `src/Battlesport/pickup.cpp:2488`
- `0x41e960` `Pickup::SetNextPickupId` -> `src/Battlesport/pickup.cpp:2960`
- `0x41e970` `Pickup::GetNextPickupId` -> `src/Battlesport/pickup.cpp:2972`
- `0x41e980` `Pickup::FindDroppableTypeForPlayerCurrentWeapon` -> `src/Battlesport/pickup.cpp:2516`
- `0x41ea00` `Pickup::FindOptMetaImageByOptEntry` -> `src/Battlesport/pickup.cpp:2498`
- `0x41ea30` `Pickup::SpawnAtCarrierNodeByName` -> `src/Battlesport/pickup.cpp:2036`
- `0x433e40` `Pickup::SendPkt11_Flag2Delta` -> `src/Battlesport/pickup.cpp:1559`
- `0x433e70` `Pickup::SendPkt11_Flag8Delta` -> `src/Battlesport/pickup.cpp:1573`
- `0x433ea0` `Pickup::SendPkt11_CreateDelta` -> `src/Battlesport/pickup.cpp:1586`
- `0x433f40` `Pickup::HandlePkt11_SpawnDelta` -> `src/Battlesport/pickup.cpp:1618`
- `0x434050` `Pickup::SendPkt12_AirdropSpawnChuteRelay` -> `src/Battlesport/pickup.cpp:1687`
- `0x4340a0` `Pickup::HandlePkt12_AirdropSpawnChuteRelay` -> `src/Battlesport/pickup.cpp:1703`
- `0x438990` `PickupAirdropSpawnRef::InitNodesFromCarrierNodeName` -> `src/Battlesport/pickup.cpp:854`
- `0x4389c0` `PickupAirdropSpawnRef::SpawnPickupTypeAndRelay` -> `src/Battlesport/pickup.cpp:907`
- `0x438a20` `PickupAirdropSpawnRef::CanSpawnWithClearance` -> `src/Battlesport/pickup.cpp:884`
- `0x438a70` `PickupAirdropSpawnRef::GetWorldPos` -> `src/Battlesport/pickup.cpp:872`
- `0x438a90` `PickupAirdropSpawnRef::InitGlobalFromCarrierNodeName` -> `src/Battlesport/pickup.cpp:937`
- `0x438b10` `PickupAirdropSpawnRef::ShutdownGlobal` -> `src/Battlesport/pickup.cpp:954`
- `0x438b30` `PickupAirdropSpawnRef::TrySpawnRandomPickupFromGlobal` -> `src/Battlesport/pickup.cpp:967`

## Battlesport/player.cpp

- `0x41bb30` `Player::DestroyedStateRespawnCallback` -> `src/Battlesport/player.cpp:8427`
- `0x41bbf0` `Player::DestroyedStateResetCallback` -> `src/Battlesport/player.cpp:8351`
- `0x41bca0` `Player::DestroyedStateResetFinalizeCallback` -> `src/Battlesport/player.cpp:8315`
- `0x41bd10` `Player::ClearRespawnTransitionFlagCallback` -> `src/Battlesport/player.cpp:8411`
- `0x41bd20` `Player::DestroyedStateResetLocalFinalize` -> `src/Battlesport/player.cpp:8286`
- `0x41ecd0` `Player::RecordNodeFlagsForRestore` -> `src/Battlesport/player.cpp:10942`
- `0x41ef30` `PlayerNodeFlagRestore::InitGlobals` -> `src/Battlesport/player.cpp:3660`
- `0x41ef40` `PlayerNodeFlagRestore::InitInstance` -> `src/Battlesport/player.cpp:3671`
- `0x41ef60` `PlayerNodeFlagRestore::RegisterAtExit` -> `src/Battlesport/player.cpp:3684`
- `0x41ef70` `PlayerNodeFlagRestore::ShutdownInstance` -> `src/Battlesport/player.cpp:3694`
- `0x41efa0` `Player::RestoreRecordedNodeFlags` -> `src/Battlesport/player.cpp:10994`
- `0x41fe90` `Player::InitMissionRuntimeFromWorldAndCamera` -> `src/Battlesport/player.cpp:4169`
- `0x421a40` `Player::CloneType6NodeFromTemplateAndRename` -> `src/Battlesport/player.cpp:3919`
- `0x421ab0` `Player::CreateFromNamesAtPose` -> `src/Battlesport/player.cpp:3975`
- `0x422170` `Player::LoadMasterCommonDataFromNode` -> `src/Battlesport/player.cpp:5339`
- `0x4226d0` `Player::LoadMasterModalDataFromNode` -> `src/Battlesport/player.cpp:5636`
- `0x4231b0` `Player::RefreshHudFromState` -> `src/Battlesport/player.cpp:11565`
- `0x423380` `Player::IsMissionProbeType1EnabledById` -> `src/Battlesport/player.cpp:9109`
- `0x424bf0` `Player::Vec3_FastNormalize` -> `src/Battlesport/player.cpp:12178`
- `0x424c90` `Player::ConstrainToUnitDistanceFrom` -> `src/Battlesport/player.cpp:12219`
- `0x425920` `Player::RegisterGameplayCommandCallbacksAndCreateFfEffects` -> `src/Battlesport/player.cpp:9538`
- `0x4266b0` `Player::TickMasterTypeAndForceFeedback` -> `src/Battlesport/player.cpp:12693`
- `0x427140` `Player::UpdateMasterTypeHover` -> `src/Battlesport/player.cpp:15533`
- `0x427440` `Player::UpdateMasterTypeHover_FromModalProbe` -> `src/Battlesport/player.cpp:15101`
- `0x4279f0` `Player::UpdateMasterTypeAmphib` -> `src/Battlesport/player.cpp:15379`
- `0x427ec0` `Player::UpdateMasterTypeAmphib_FromModalProbe` -> `src/Battlesport/player.cpp:15279`
- `0x428120` `Player::UpdateMasterTypeBasic` -> `src/Battlesport/player.cpp:15634`
- `0x428350` `Player::UpdateMasterTypeBasicOrTrack_FromModalProbe` -> `src/Battlesport/player.cpp:15055`
- `0x428520` `Player::UpdateMasterTypeSub` -> `src/Battlesport/player.cpp:12556`
- `0x4289f0` `Player::UpdateSubModeWaterProbeState` -> `src/Battlesport/player.cpp:12295`
- `0x428d60` `Player::ProbeModalSampleHeights` -> `src/Battlesport/player.cpp:13785`
- `0x4290f0` `Player::SelectProbeSampleHeightFromCandidates` -> `src/Battlesport/player.cpp:13715`
- `0x429240` `Player::ApplyAmphibSpeedOscillation` -> `src/Battlesport/player.cpp:10492`
- `0x429750` `Player::UpdateAutoTurnAndSteerFromTarget` -> `src/Battlesport/player.cpp:10357`
- `0x42a9f0` `Player::AddScaledHudCounterValue` -> `src/Battlesport/player.cpp:7325`
- `0x42aa50` `Player::UpdateDebugOverlayHud` -> `src/Battlesport/player.cpp:11452`
- `0x42ac90` `Player::TransitionToMasterTypeTrack` -> `src/Battlesport/player.cpp:6273`
- `0x42aeb0` `Player::TransitionToMasterTypeAmphib` -> `src/Battlesport/player.cpp:6408`
- `0x42b0f0` `Player::TransitionToMasterTypeHover` -> `src/Battlesport/player.cpp:6664`
- `0x42b2a0` `Player::TransitionToMasterTypeSub` -> `src/Battlesport/player.cpp:6537`
- `0x42b4a0` `Player::StopBftBubbleFxHandle` -> `src/Battlesport/player.cpp:6222`
- `0x42b4c0` `Player::TransitionToMasterTypeFly` -> `src/Battlesport/player.cpp:6239`
- `0x42b520` `Player::ApplyMasterTypeTransition` -> `src/Battlesport/player.cpp:6765`
- `0x42b5a0` `Player::ReactivateCopterSndNodesIfHealthy` -> `src/Battlesport/player.cpp:6164`
- `0x42b630` `Player::CacheDisableCopterSndNodesAndStopSample` -> `src/Battlesport/player.cpp:6107`
- `0x42b810` `Player::SyncLocalPoseFromRootNode` -> `src/Battlesport/player.cpp:4565`
- `0x42b8c0` `Player::RebuildSteerBasisRawFromRef` -> `src/Battlesport/player.cpp:10468`
- `0x42bab0` `Player::SetAutoTurnTargetDirFromWorldPoint` -> `src/Battlesport/player.cpp:10677`
- `0x42bb30` `Player::AsyncCommandCallback` -> `src/Battlesport/player.cpp:9601`
- `0x42be00` `Player::SetWorldPoseAndRestartAnchor` -> `src/Battlesport/player.cpp:9772`
- `0x42be70` `Player::CaptureCurrentObjectPoseAsRestartAnchor` -> `src/Battlesport/player.cpp:9800`
- `0x42bed0` `Player::ResetMotionTransientState` -> `src/Battlesport/player.cpp:10300`
- `0x42bf90` `Player::UpdatePostMoveEnvironment` -> `src/Battlesport/player.cpp:14690`
- `0x42c0d0` `Player::ProcessEnvProbeResults` -> `src/Battlesport/player.cpp:14764`
- `0x42c2e0` `Player::UpdateVerticalVelocityAndTransform` -> `src/Battlesport/player.cpp:15005`
- `0x42c420` `Player::AccumulateSlopeForces` -> `src/Battlesport/player.cpp:14975`
- `0x42c520` `Player::ComputeSurfaceFrom1Probe` -> `src/Battlesport/player.cpp:14411`
- `0x42c640` `Player::ComputeSurfaceFrom2Probes` -> `src/Battlesport/player.cpp:14454`
- `0x42c8d0` `Player::ApplyTerrainTilt` -> `src/Battlesport/player.cpp:14322`
- `0x42ca40` `Player::ComputeSurfaceFrom3Probes` -> `src/Battlesport/player.cpp:14648`
- `0x42cb50` `Player::ResetTerrainContactImpulsesAndPlayImpactSfx` -> `src/Battlesport/player.cpp:14293`
- `0x42cbd0` `Player::CheckProbeSampleMaskOverlap` -> `src/Battlesport/player.cpp:14533`
- `0x42cc00` `Player::SelectBestProbesByDotProduct` -> `src/Battlesport/player.cpp:14565`
- `0x42cde0` `Player::SolveHeightOnSurface` -> `src/Battlesport/player.cpp:14272`
- `0x42ce50` `Player::ComputeTriangleNormal` -> `src/Battlesport/player.cpp:14374`
- `0x42cf60` `Player::RebuildAboveGroundIndices` -> `src/Battlesport/player.cpp:14549`
- `0x42cf90` `Player::BuildEnvironmentProbeResult` -> `src/Battlesport/player.cpp:13945`
- `0x42d320` `Player::FindThirdProbeAndComputeNormal` -> `src/Battlesport/player.cpp:14901`
- `0x42d5c0` `Player::ApplyEnvironmentProbeResult` -> `src/Battlesport/player.cpp:14084`
- `0x42da40` `Player::RebuildOrientationFromNormal` -> `src/Battlesport/player.cpp:14861`
- `0x439460` `Player::HandlePrimaryWeaponVariantToggleInput` -> `src/Battlesport/player.cpp:13516`
- `0x43a900` `Player::DecayAndApplyAltFireSlotOffsetToNode` -> `src/Battlesport/player.cpp:16046`
- `0x43a980` `Player::ApplyGunFireSlotOffsetToNode` -> `src/Battlesport/player.cpp:16075`
- `0x43acf0` `Player::SelectPrimaryGunFirePointAndSlot` -> `src/Battlesport/player.cpp:16191`
- `0x43b730` `Player::RecordRecentHitFeedback` -> `src/Battlesport/player.cpp:8231`
- `0x43b790` `Player::UpdateTimedHitStatusFromHitSource` -> `src/Battlesport/player.cpp:8723`
- `0x43b800` `Player::ClearDestroyedRespawnEffectHandleCallback` -> `src/Battlesport/player.cpp:8268`
- `0x43b810` `Player::HitCallback_RecordNetContextAndTimedStatus` -> `src/Battlesport/player.cpp:8761`
- `0x43b870` `Player::HitCallback_RecordContextAndTimedStatus` -> `src/Battlesport/player.cpp:8925`
- `0x43bc40` `Player::EnterLocalInactiveDestroyedLifecycle` -> `src/Battlesport/player.cpp:8495`
- `0x43bcc0` `Player::EnterDestroyedState` -> `src/Battlesport/player.cpp:8538`
- `0x43c010` `Player::ApplyDamageLocal` -> `src/Battlesport/player.cpp:8803`
- `0x43c0c0` `Player::StartDestroyedStateVehicleEffect` -> `src/Battlesport/player.cpp:9125`
- `0x43c630` `Player::IsAltWeaponAllowedInCurrentMasterMode` -> `src/Battlesport/player.cpp:13058`
- `0x43c660` `Player::AutoSwitchToNextUsableAltWeapon` -> `src/Battlesport/player.cpp:13097`
- `0x43c800` `Player::ResetAltGunDoorAnimationState` -> `src/Battlesport/player.cpp:13240`

## Battlesport/recoil_state.cpp

- `0x408d20` `RecoilStateControls::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:4130`
- `0x408d30` `RecoilStateControls::StaticInit` -> `src/Battlesport/hud.cpp:4181`
- `0x408d40` `RecoilStateControls::RegisterAtExit` -> `src/Battlesport/hud.cpp:4190`
- `0x408d50` `RecoilStateControls::AtExitDestructor` -> `src/Battlesport/hud.cpp:4199`
- `0x408d60` `RecoilStateControls::RecoilStateControls` -> `src/Battlesport/hud.cpp:4208`
- `0x408d90` `RecoilStateControls::Destructor` -> `src/Battlesport/hud.cpp:4217`
- `0x408df0` `RecoilStateControls::OnTryBecomeCurrent` -> `src/Battlesport/hud.cpp:4231`
- `0x408ec0` `RecoilStateControls::OnDeactivate` -> `src/Battlesport/hud.cpp:4260`
- `0x408fa0` `RecoilStateControls::OnResume` -> `src/Battlesport/hud.cpp:4298`
- `0x408ff0` `RecoilStateControls::QueueEnter` -> `src/Battlesport/hud.cpp:4330`

## Battlesport/RecoilApp.cpp

- `0x42f9d0` `RecoilApp_LeaveNetworkState::OnTryBecomeCurrent` -> `src/Battlesport/recoil_app_late_body.h:3599`
- `0x434660` `operator<` -> `src/Battlesport/recoil_app_late_body.h:742`
- `0x435d20` `RecoilStateSaveLoadTransition::OnTryBecomeCurrent` -> `src/Battlesport/recoil_app_late_body.h:1807`
- `0x435e80` `RecoilStateSaveLoadTransition::OnUpdateShouldQuit` -> `src/Battlesport/recoil_app_late_body.h:1863`
- `0x435ed0` `RecoilStateSaveLoadTransition::OnDeactivate` -> `src/Battlesport/recoil_app_late_body.h:1897`
- `0x435f50` `RecoilStateSaveLoadTransition::QueueOpenSaveDialog` -> `src/Battlesport/recoil_app_late_body.h:1942`
- `0x435f80` `RecoilStateSaveLoadTransition::QueueOpenLoadDialog` -> `src/Battlesport/recoil_app_late_body.h:1962`
- `0x4428b0` `RecoilApp_MfcOleModule::~RecoilApp_MfcOleModule` -> `src/Battlesport/recoil_app_late_body.h:2916`
- `0x442c70` `RecoilApp_MfcOleModule::RecoilApp_MfcOleModule` -> `src/Battlesport/recoil_app_late_body.h:2869`
- `0x442c70` `RecoilApp_MfcOleModule::RecoilApp_MfcOleModule` -> `src/Battlesport/recoil_app_late_body.h:2891`

## Battlesport/RecoilStateCheatCode.cpp

- `0x406f60` `RecoilStateCheatCode::OnTryBecomeCurrent` -> `src/Battlesport/hud.cpp:2154`
- `0x407010` `RecoilStateCheatCode::OnDeactivate` -> `src/Battlesport/hud.cpp:2184`

## Battlesport/RecoilStateConfirmQuit.cpp

- `0x415880` `RecoilStateConfirmQuit::~RecoilStateConfirmQuit` -> `src/Battlesport/hud.cpp:4620`

## Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp

- `0x441750` `WestwoodOnlineUpgradeConfigDialog::WestwoodOnlineUpgradeConfigDialog` -> `src/Battlesport/wol_config_dialog_body.h:150`
- `0x441c60` `WestwoodOnlineUpgradeConfigDialog::GetSelectedProfileValues` -> `src/Battlesport/wol_config_dialog_body.h:461`
- `0x441cb0` `WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues` -> `src/Battlesport/wol_config_dialog_body.h:480`

## Battlesport/WestwoodOnlineUpgradeDialog.cpp

- `0x43f440` `WestwoodOnlineUpgradeProgressDialog::Destructor` -> `src/Battlesport/wol_progress_dialog_body.h:124`

## Battlesport/WestwoodOnlineUpgradeProgressDialog.cpp

- `0x442220` `WestwoodOnlineUpgradeProgressDialog::Constructor` -> `src/Battlesport/wol_progress_dialog_body.h:113`
- `0x442320` `WestwoodOnlineUpgradeProgressDialog::DlgProc` -> `src/Battlesport/wol_progress_dialog_body.h:147`
- `0x442530` `WestwoodOnlineUpgradeDialog::ShowDownloadReadyList` -> `src/Battlesport/wol_progress_dialog_body.h:258`

## Battlesport/zimage.cpp

- `0x46d4d0` `zImage::FindTexDirEntryByName` -> `src/GameZRecoil/zImage/zimg_texture.cpp:391`

## Battlesport/zNetwork/zNetwork.cpp

- `0x489f70` `zNetwork_GetLocalPlayerKey` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:365`
- `0x489f80` `zNetwork::IsHost` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:482`

## Battlesport/zOpt.cpp

- `0x408230` `zOpt::SetNetworkEnabled` -> `src/GameZRecoil/zGame/zgame_opt.c:2230`
- `0x408240` `zOpt::SetNetworkModemEnabled` -> `src/GameZRecoil/zGame/zgame_opt.c:2241`
- `0x408250` `zOpt::SetNetworkListenEnabled` -> `src/GameZRecoil/zGame/zgame_opt.c:2252`
- `0x408260` `zOpt::GetNetworkEnabled` -> `src/GameZRecoil/zGame/zgame_opt.c:2263`
- `0x408270` `zOpt::GetNetworkModemEnabled` -> `src/GameZRecoil/zGame/zgame_opt.c:2272`

## Battlesport/zopt.cpp

- `0x408360` `zOpt::GetHudTypeForCurrentHwMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2352`

## Battlesport/zrndr_span.cpp

- `0x490610` `zRndr::SpanOcclusionSubmitOccluderRect` -> `src/GameZRecoil/zRender/zrndr_draw.c:2417`

## Battlesport/zVideo.cpp

- `0x4a59a0` `zVid::SetCachedClientRectUpdateMask` -> `src/GameZRecoil/zVideo/zvid_main.c:3080`
- `0x4a59b0` `zVid::QueryCachedClientRectUpdateMaskIf3dfx` -> `src/GameZRecoil/zVideo/zvid_main.c:2306`
- `0x4a7700` `zVideo::UpdateCachedClientRectScreenCoords` -> `src/GameZRecoil/zVideo/zvid_main.c:5760`
- `0x4a7740` `zVideo::ShutdownVideoSystem` -> `src/GameZRecoil/zVideo/zvid_main.c:5740`

## Battlesport/zWeapon.cpp

- `0x439260` `Player::HandleAltWeaponBankSelectInput` -> `src/Battlesport/player.cpp:13403`

## GameZRecoil/GameNet.cpp

- `0x434240` `OptCatalog::SendPkt0A_RemoveRuntimeRelay` -> `src/GameZRecoil/zWeapon/zwep_init.c:1905`
- `0x4342d0` `OptCatalog::HandlePkt0A_RemoveRuntimeRelay` -> `src/GameZRecoil/zWeapon/zwep_init.c:1944`

## GameZRecoil/HudSensorTracker.cpp

- `0x417770` `HudSensorTracker::InitMissionIdAndFlags` -> `src/Battlesport/hud_sensor_tracker_body.h:2651`
- `0x4177d0` `HudSensorTracker::SetZbdPath` -> `src/Battlesport/hud_sensor_tracker_body.h:2670`

## GameZRecoil/mission.cpp

- `0x417350` `Mission::InitObjectives` -> `src/Battlesport/mission_gamez_impl_body.h:7`

## GameZRecoil/player.cpp

- `0x426390` `PlayerMgr::TickAllPlayers` -> `src/Battlesport/player.cpp:9854`
- `0x4283f0` `Player::UpdateBankVelocityFromSteerInput` -> `src/Battlesport/player.cpp:10330`
- `0x428490` `Player::IntegrateYawAndWrapFromYawVelocity` -> `src/Battlesport/player.cpp:10407`

## GameZRecoil/Player/player_camera.c

- `0x404e90` `Player::TickActiveCameraState` -> `src/Battlesport/hud.cpp:523`
- `0x405040` `Player::UpdateChaseCameraFromInput` -> `src/Battlesport/hud.cpp:602`
- `0x4057d0` `Player::UpdateTopDownCameraState` -> `src/Battlesport/hud.cpp:893`
- `0x405870` `Player::UpdateCameraFromStoredTargetTowardPlayer` -> `src/Battlesport/hud.cpp:924`
- `0x4059a0` `Player::UpdateFirstPersonCameraFromInput` -> `src/Battlesport/hud.cpp:971`
- `0x405ec0` `Player::ToggleSteeringModeAndResetMouseLook` -> `src/Battlesport/hud.cpp:1203`
- `0x405ee0` `Player::AdjustThirdPersonCameraByOffsetProbes` -> `src/Battlesport/hud.cpp:1216`
- `0x406110` `Player::AdjustThirdPersonCameraBySideProbes` -> `src/Battlesport/hud.cpp:1326`
- `0x4063f0` `Player::RestoreThirdPersonCameraFromObstructionState` -> `src/Battlesport/hud.cpp:1476`
- `0x406430` `Player::UnbindCurrentSaveStateIfSinglePlayer` -> `src/Battlesport/hud.cpp:1493`
- `0x406450` `Player::BindActiveGameStateAsCurrentSaveState` -> `src/Battlesport/hud.cpp:1509`
- `0x406470` `Player::UpdateCameraVariantFromCameraPos` -> `src/Battlesport/hud.cpp:1524`
- `0x406510` `Player::UpdateCameraVariantFromAnchor` -> `src/Battlesport/hud.cpp:1576`
- `0x406610` `Player::UpdateCameraWeatherFxEmitterVisibility` -> `src/Battlesport/hud.cpp:1636`
- `0x406730` `Player::FilterCameraProbeBlockingHits` -> `src/Battlesport/hud.cpp:1708`
- `0x4067a0` `Player::AdjustSubCameraFocusForObstruction` -> `src/Battlesport/hud.cpp:1753`
- `0x42b6e0` `Player::FindNearestThirdPersonCameraProbePoint` -> `src/Battlesport/player.cpp:7395`

## GameZRecoil/Player/player_status.cpp

- `0x43b5d0` `Player::ApplyStatusMeterChange` -> `src/Battlesport/player.cpp:11686`
- `0x43b660` `Player::UpdateStatusMeter` -> `src/Battlesport/player.cpp:11725`

## GameZRecoil/recoilapp.cpp

- `0x419010` `HudSensorTracker::QueueMissionFmvStateForMissionId` -> `src/Battlesport/hud_sensor_tracker_body.h:3522`
- `0x42edb0` `RecoilApp_MissionFmvState::OnTryBecomeCurrent` -> `src/Battlesport/recoil_app_late_body.h:3799`
- `0x42ee50` `RecoilApp_MissionFmvState::OnDeactivate` -> `src/Battlesport/recoil_app_late_body.h:3838`
- `0x42ee70` `RecoilApp_MissionFmvState::OnUpdateShouldQuit` -> `src/Battlesport/recoil_app_late_body.h:3851`

## GameZRecoil/RecoilApp/RecoilApp.cpp

- `0x42e330` `RecoilApp::InitializeDisplay` -> `src/Battlesport/recoil_app_late_body.h:2378`

## GameZRecoil/RecoilApp/RecoilStateSaveLoadTransition.cpp

- `0x435a30` `RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit` -> `src/Battlesport/recoil_app_late_body.h:677`
- `0x435a40` `RecoilStateSaveLoadTransition::StaticInit` -> `src/Battlesport/recoil_app_late_body.h:687`
- `0x435a50` `RecoilStateSaveLoadTransition::RegisterAtExit` -> `src/Battlesport/recoil_app_late_body.h:696`
- `0x435a60` `RecoilStateSaveLoadTransition::AtExitDestructor` -> `src/Battlesport/recoil_app_late_body.h:705`
- `0x435c80` `RecoilStateSaveLoadTransition::Constructor` -> `src/Battlesport/recoil_app_late_body.h:714`
- `0x435cc0` `RecoilStateSaveLoadTransition::Destructor` -> `src/Battlesport/recoil_app_late_body.h:725`

## GameZRecoil/westwoodonline/WolapiConfigDialog.cpp

- `0x4418b0` `WestwoodOnlineUpgradeConfigDialog::~WestwoodOnlineUpgradeConfigDialog` -> `src/Battlesport/wol_config_dialog_body.h:185`
- `0x4419a0` `WestwoodOnlineUpgradeConfigDialog::DoDataExchange` -> `src/Battlesport/wol_config_dialog_body.h:204`
- `0x441a20` `WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear` -> `src/Battlesport/wol_config_dialog_body.h:235`
- `0x441a40` `WestwoodOnlineUpgradeConfigDialog::OnInitDialog` -> `src/Battlesport/wol_config_dialog_body.h:267`
- `0x441f40` `WestwoodOnlineUpgradeConfigDialog::OnOK` -> `src/Battlesport/wol_config_dialog_body.h:350`
- `0x442010` `WestwoodOnlineUpgradeConfigDialog::OnProfileComboKillFocus` -> `src/Battlesport/wol_config_dialog_body.h:392`
- `0x442080` `WestwoodOnlineUpgradeConfigDialog::OnProfileComboSelChange` -> `src/Battlesport/wol_config_dialog_body.h:418`
- `0x4420c0` `WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange` -> `src/Battlesport/wol_config_dialog_body.h:434`
- `0x4420d0` `WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown` -> `src/Battlesport/wol_config_dialog_body.h:443`
- `0x4420e0` `WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked` -> `src/Battlesport/wol_config_dialog_body.h:452`
- `0x442100` `WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus` -> `src/Battlesport/wol_config_dialog_body.h:249`

## GameZRecoil/westwoodonline/WolapiProgressDialog.cpp

- `0x442240` `WestwoodOnlineUpgradeProgressDialog::ScalarDeletingDestructor` -> `src/Battlesport/wol_progress_dialog_body.h:132`
- `0x442260` `WestwoodOnlineUpgradeProgressDialog::GetMessageMap` -> `src/Battlesport/wol_progress_dialog_body.h:78`
- `0x442270` `WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt` -> `src/Battlesport/wol_progress_dialog_body.h:86`

## GameZRecoil/zClass/List.c

- `0x44f630` `zClass_List::RenderActiveCameras` -> `src/GameZRecoil/zClass/render_active_cameras_impl.h:17`

## GameZRecoil/zDEClient/zdec_init.c

- `0x455dd0` `zDEClient::LoadOrCreateMaterialFromTexturePath` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:854`
- `0x455e40` `zDEClient::ShutdownGlobals` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:885`

## GameZRecoil/zDEClient/zdec_init.cpp

- `0x4558f0` `zDEClient::LoadConfigResources` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:560`
- `0x457650` `zDEClient::InitFeatureSystem` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:557`
- `0x4576a0` `zDEClient::RegisterFeatureSystemCleanupAtExit` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:596`
- `0x4576b0` `zDEClient::ShutdownFeatureSystem` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:605`
- `0x457750` `zDEClient::ClearFeatureDisplayNodes` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:654`
- `0x457b40` `zDEClient::WriteFeatureSectionsToZAR` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:809`
- `0x457c10` `zDEClient::ApplyFeatureEntry` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:874`
- `0x457c50` `zDEClient::DispatchFeatureEventTemplates` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:901`
- `0x458a30` `zDEClient::CopyFeatureEntriesForward` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1331`
- `0x458a70` `zDEClient::FillFeatureEntries` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1353`

## GameZRecoil/zEffect/eff_runtime.c

- `0x461f00` `zEffect::SpawnRuntimeInstanceAt` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1548`
- `0x461f50` `zEffect::ActivateRuntimeEntryAtPosition` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1581`
- `0x462050` `zEffect::ComputeDistanceSqToListener` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1641`
- `0x4620d0` `zEffect::AcquireRuntimeEntryByIndex` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1662`
- `0x462130` `zEffect::CloneRuntimeEntryFromTemplate` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1692`
- `0x4621b0` `zEffect::RuntimeNodeActionCallback` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1732`

## GameZRecoil/zEffect/Effect.c

- `0x462280` `zEffect::FindTemplateIndexByName` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1798`

## GameZRecoil/zEffect/zeff.c

- `0x45e200` `zEffect::SetWorldNode` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:71`
- `0x45e270` `zEffect::SetResourceNode` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:108`

## GameZRecoil/zEffect/zeff_anim.c

- `0x45d6c0` `zEffectAnim::ResetForNode` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4007`
- `0x45d7a0` `zEffectAnim::ResetActivationPrereqCount` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4074`
- `0x45d7b0` `zEffectAnim::SetTransformRotAndVelocity` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4085`
- `0x45d930` `zEffectAnim::ActivateRuntime` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4180`
- `0x45dc70` `zEffectAnim::SetTransformRotAndVelocity_Thunk` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4389`
- `0x45dcb0` `zEffectAnim::SetVelocity` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4423`
- `0x45dde0` `zEffectAnim::SetVelocity_Thunk` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4474`
- `0x45de00` `zEffectAnim::SetPositionRefAndVelocity` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4496`
- `0x45df70` `zEffectAnim::SetPositionRefAndVelocity_Thunk` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4555`
- `0x45df90` `zEffectAnim::SetTransformRefs` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4577`
- `0x45e0b0` `zEffectAnim::SetTransformRefs_Thunk` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4623`
- `0x45e380` `zEffectAnim::FindOrCreateSoundRef` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:162`
- `0x45e4a0` `zEffectAnim::FindOrCreateLightRef` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:237`
- `0x45e6d0` `zEffectAnim::EnsureCopiedRootTree` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:392`
- `0x45ffa0` `zEffectAnim::FindNextAsyncEntry` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:1490`

## GameZRecoil/zEffect/zeff_anim_activation.c

- `0x461970` `zEffectAnim::QueueCmdType1TransformRotVelocity` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1196`
- `0x461aa0` `zEffectAnim::QueueCmdType2Velocity` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1280`
- `0x461ba0` `zEffectAnim::QueueCmdType3PositionRefAndVelocity` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1343`
- `0x461d00` `zEffectAnim::QueueCmdType4TransformRefs` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1423`

## GameZRecoil/zEffect/zeff_anim_init.c

- `0x45e100` `zEffect_Anim::Init` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:6`
- `0x45e210` `zEffect_Anim::SetZbdFilename` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:82`
- `0x45e730` `zEffectAnim::CloneEntryForNode` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:428`
- `0x45ed80` `zEffectAnim::RebindEntryToNode` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:735`
- `0x45efb0` `zEffect_Anim::LoadZbd` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:844`
- `0x45fb30` `zEffect_Anim::LoadAndInstantiate` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:1251`
- `0x45fd10` `zEffectAnim::ShutdownEntry` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:1370`
- `0x45fe50` `zEffect_Anim::Shutdown` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:1418`
- `0x45fef0` `zEffect_Anim::ShutdownIfLoaded` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:1452`

## GameZRecoil/zEffect/zeff_anim_run.c

- `0x458e10` `zEffect::HandleSampleRefOffsetEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:241`
- `0x458eb0` `zEffect::HandleEffectTemplateOffsetEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:273`
- `0x458f70` `zEffect::HandleSoundEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:324`
- `0x459080` `zEffect::HandleLightEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:394`
- `0x459280` `zEffect::HandleLightAnimEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:552`
- `0x459510` `zEffect::HandleFogEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:660`
- `0x459580` `zEffect::HandleCameraParamsEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:704`
- `0x4596c0` `zEffect::AnimateCameraParamsOverTime` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:811`
- `0x459ae0` `zEffect::HandleRotationEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:1105`
- `0x459cb0` `zEffect::HandleNodeScaleEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:1193`
- `0x459ce0` `zEffect::HandlePositionEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:1211`
- `0x459e30` `zEffect::HandleActivateEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:1296`
- `0x459e70` `zEffect::HandleNodeAnimEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:1322`
- `0x45a920` `zEffect::FindNearestPickCandidateBelowPoint` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:1664`
- `0x45a9d0` `zEffect::AnimateNodeOverTime` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:1704`
- `0x45ae30` `zEffect_Anim::AdvanceKeyframeSample` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:1897`
- `0x45ae90` `zEffect_Anim::AnimateKeyframeSample` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:1933`
- `0x45b120` `zEffect_Anim::AdvanceKeyframe` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2085`
- `0x45b210` `zEffect_Anim::EvaluateKeyframe` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2141`
- `0x45b280` `zEffect_Anim::RunKeyframes` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2175`
- `0x45b3b0` `zEffect::HandleAddChildEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2255`
- `0x45b410` `zEffect::HandleRemoveChildEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2284`
- `0x45b440` `zEffect::HandleAttachEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2301`
- `0x45b4a0` `zEffect::HandleDetachEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2332`
- `0x45b8b0` `zEffect::HandleTransformRefsEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2544`
- `0x45bb00` `zEffect::HandleSurfaceStopEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2647`
- `0x45bbb0` `zEffect::HandleSurfacePlayEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2667`
- `0x45bc60` `zEffect::HandleSurfaceRefEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2687`
- `0x45bf60` `zEffect::CleanupLightRefs` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2799`
- `0x45bfd0` `zEffect::CleanupSoundRefs` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2839`
- `0x45c040` `zEffectAnim::Stop` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2879`
- `0x45c100` `zEffect::HandleNamedAnimStopEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2922`
- `0x45c1a0` `zEffect::HandleEmitterPlayEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2939`
- `0x45c240` `zEffect::HandleEmitterStopEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2959`
- `0x45c2f0` `zEffect::HandleEmitterResetEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:2980`
- `0x45c310` `zEffect::HandleEmitterLoopEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3000`
- `0x45c3c0` `zEffect::HandleConditionalChainEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3038`
- `0x45c530` `zEffect::TraceUpwardHitFromNodeOrPos` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3133`
- `0x45c640` `zEffect::GetConditionalRefPosDistanceSq` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3192`
- `0x45c6b0` `zEffect::SkipConditionalChainToEnd` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3215`
- `0x45c6e0` `zEffect::HandleNoOpMarkerEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3239`
- `0x45c6f0` `zEffect::HandleCallbackEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3252`
- `0x45c710` `zEffect::HandleScreenColorFxEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3274`
- `0x45c920` `zEffect::HandleScreenOverlayFxEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3329`
- `0x45cbc0` `zEffect::HandleTopMessageEvent` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3435`
- `0x45cc00` `zEffect_Anim::RunSequenceEvents` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3463`
- `0x45d000` `zEffect::SetAnimDebugFrameTag` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3559`
- `0x45d010` `zEffect_Anim::RunSequence` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3570`
- `0x45d240` `zEffect_Anim::CaptureNodeStates` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3678`
- `0x45d310` `zEffect_Anim::RestoreNodeStates` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3738`
- `0x45d3d0` `zEffectAnim::FinalizeStop` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3801`
- `0x45d4c0` `zEffectAnim::RunStopSequenceCallback` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3857`
- `0x45d570` `zEffectAnim::StopAndCleanup` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3910`
- `0x45d6b0` `zEffect_Anim::NodeActionCallback` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:3990`
- `0x45d770` `zEffectAnim::RunStopDelayCallback` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4050`

## GameZRecoil/zEffect/zeff_anim_save.c

- `0x4603d0` `zEffect_Anim::ClearActivationRecords` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:6`
- `0x460400` `zEffect_Anim::HasActivationRecord` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:20`
- `0x460470` `zEffect_Anim::GetActivationRecordCount` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:44`
- `0x460480` `zEffect_Anim::GetActivationRecordAt` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:53`
- `0x460490` `zEffect_Anim::SaveActivationRecords` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:64`
- `0x4606d0` `zEffect_Anim::LoadActivationRecords` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:190`
- `0x460ae0` `zEffect_Anim::AllocActivationRecord` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:402`
- `0x460bc0` `zEffect_Anim::SaveRunningAnimRecord` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:449`
- `0x460f80` `zEffect_Anim::SaveRunningAnimRecords` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:606`
- `0x461040` `zEffect_Anim::LoadRunningAnimRecords` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:647`
- `0x461430` `zEffect_Anim::SaveAnimRecords` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:877`
- `0x461670` `zEffect_Anim::LoadAnimRecords` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:988`
- `0x461800` `zEffect_Anim::GetActivationRecordPackedSize` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1098`
- `0x461840` `zEffect_Anim::ResetFromActivationRecord` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1118`
- `0x461870` `zEffect_Anim::ProcessActivationRecord` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1132`
- `0x461a90` `zEffect_Anim::DiscardLastActivationRecord` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1271`

## GameZRecoil/zEffect/zeff_detach.c

- `0x458c10` `zEffect::UpdateBeamNodeBetweenPoints` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:117`
- `0x458ce0` `zEffect::UpdateBeamNodeBetweenFractions` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:174`

## GameZRecoil/zEffect/zeff_init.c

- `0x460020` `zEffect::Init` -> `src/GameZRecoil/zEffect/zeff_init.c:6`
- `0x460060` `zEffect::ShutdownAll` -> `src/GameZRecoil/zEffect/zeff_init.c:25`
- `0x460070` `zEffect::InitFromPath` -> `src/GameZRecoil/zEffect/zeff_init.c:36`
- `0x460330` `zEffect::Reset` -> `src/GameZRecoil/zEffect/zeff_init.c:201`
- `0x461ec0` `zEffect::FindNodeUserDataRecursive` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1520`

## GameZRecoil/zEffect/zeffect.cpp

- `0x458af0` `zEffect::SetConditionalRefPos` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:7`
- `0x458b20` `zEffect::SetVariantOverridePackedIdsIfComplete` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:22`
- `0x45e0f0` `zEffect::SetConditionalEffectLevel` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4663`

## GameZRecoil/zEffect/zEffect.cpp

- `0x461eb0` `zEffect_Anim::SetActivationDispatchContext` -> `src/GameZRecoil/zEffect/zeff_anim_save.c:1507`

## GameZRecoil/zGame/Player/Player_Camera.cpp

- `0x426330` `Player::ResetMouseControlStateAndRecenterCursor` -> `src/Battlesport/player.cpp:9836`

## GameZRecoil/zGame/zGame.cpp

- `0x408300` `zOpt::SetReplicateMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2320`
- `0x408500` `zOpt::RenderSection_SetSize` -> `src/GameZRecoil/zGame/zgame_opt.c:2471`
- `0x408530` `zOpt::RenderSection_SetPosition` -> `src/GameZRecoil/zGame/zgame_opt.c:2501`
- `0x4085e0` `zOpt::DisplaySection_SetPosition` -> `src/GameZRecoil/zGame/zgame_opt.c:2591`
- `0x408620` `zOpt::DisplaySection_SetSize` -> `src/GameZRecoil/zGame/zgame_opt.c:2627`
- `0x408680` `zOpt::DisplaySection_SetBitsPerPixel` -> `src/GameZRecoil/zGame/zgame_opt.c:2686`
- `0x4086e0` `zOpt::WindowSection_SetSize` -> `src/GameZRecoil/zGame/zgame_opt.c:2737`
- `0x408700` `zOpt::WindowSection_SetPosition` -> `src/GameZRecoil/zGame/zgame_opt.c:2757`
- `0x4b3090` `zGame_OptionsRuntimeConfig::CopyDefault` -> `src/GameZRecoil/zGame/zgame_opt.c:564`
- `0x4b30b0` `zGame_OptionsRuntimeConfig::InitFromSystem` -> `src/GameZRecoil/zGame/zgame_opt.c:637`
- `0x4b3160` `zGame_OptionsRuntimeConfig::LoadCpuVendorString` -> `src/GameZRecoil/zGame/zgame_opt.c:582`

## GameZRecoil/zGame/zGame_Options.cpp

- `0x407e20` `zOpt::SetGameControlOptions` -> `src/GameZRecoil/zGame/zgame_opt.c:1893`
- `0x407e30` `zOpt::SetThrottleMode` -> `src/GameZRecoil/zGame/zgame_opt.c:1904`
- `0x407e50` `zOpt::GetThrottleMode` -> `src/GameZRecoil/zGame/zgame_opt.c:1919`
- `0x407e60` `zOpt::SetSteeringMode` -> `src/GameZRecoil/zGame/zgame_opt.c:1928`
- `0x407e80` `zOpt::GetSteeringMode` -> `src/GameZRecoil/zGame/zgame_opt.c:1943`
- `0x407e90` `zOpt::SetCursorMode` -> `src/GameZRecoil/zGame/zgame_opt.c:1952`
- `0x407eb0` `zOpt::GetCursorMode` -> `src/GameZRecoil/zGame/zgame_opt.c:1967`
- `0x407ec0` `zOpt::SetCameraMode` -> `src/GameZRecoil/zGame/zgame_opt.c:1976`
- `0x407ef0` `zOpt::GetCameraModeAsPlayerCameraState` -> `src/GameZRecoil/zGame/zgame_opt.c:1993`
- `0x4081a0` `zOpt::SetGraphicsFlagsForCurrentHwMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2198`
- `0x4081f0` `zOpt::GetGraphicsFlagsForCurrentHwMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2221`

## GameZRecoil/zGame/zopt.c

- `0x407190` `zOpt::LookupNamedValueAsInt` -> `src/GameZRecoil/zGame/zgame_opt.c:735`
- `0x4071f0` `zOpt::ReadScalarValueAsInt` -> `src/GameZRecoil/zGame/zgame_opt.c:758`
- `0x407220` `zOpt::EvalIntCompareOp` -> `src/GameZRecoil/zGame/zgame_opt.c:779`
- `0x407470` `zOpt::EvaluateProfileMetricCondition` -> `src/GameZRecoil/zGame/zgame_opt.c:838`
- `0x407680` `zOpt::SelectProfileValueForSystem` -> `src/GameZRecoil/zGame/zgame_opt.c:903`

## GameZRecoil/zHud/HudUiBackground.cpp

- `0x4b9850` `HudUiBackground::SetEnabled` -> `src/GameZRecoil/zUI/zui.cpp:6482`

## GameZRecoil/zhud_ui.cpp

- `0x410d10` `HudUiMgrSensor::SetViewportRect` -> `src/Battlesport/hud_runtime_layer_body.h:2092`

## GameZRecoil/zImage/zimg_fonts.cpp

- `0x46efc0` `zImage_Font::GetByIndexOrDefault` -> `src/GameZRecoil/zImage/zimg_fonts.cpp:6`
- `0x46efe0` `zImage::FontsLoadFromPath` -> `src/GameZRecoil/zImage/zimg_fonts.cpp:27`
- `0x46f130` `zImage_Font::BuildGlyphRects` -> `src/GameZRecoil/zImage/zimg_fonts.cpp:103`
- `0x46f210` `zImage_Font::IsImageColumnTransparent` -> `src/GameZRecoil/zImage/zimg_fonts.cpp:170`
- `0x46f260` `zImage_Font::MeasureString` -> `src/GameZRecoil/zImage/zimg_fonts.cpp:205`
- `0x4c7f00` `zImage_Font::BlitStringToActiveTarget` -> `src/GameZRecoil/zImage/zimg_texture.cpp:915`

## GameZRecoil/zImage/zimg_texture.cpp

- `0x46d310` `zImage::TexDirEntryToIndex` -> `src/GameZRecoil/zImage/zimg_texture.cpp:187`
- `0x46d340` `zImage::TexIndexToDirEntry` -> `src/GameZRecoil/zImage/zimg_texture.cpp:207`
- `0x46d360` `zImage::WriteTextureDirectory` -> `src/GameZRecoil/zImage/zimg_texture.cpp:227`
- `0x46d420` `zImage::ReadTextureDirectory` -> `src/GameZRecoil/zImage/zimg_texture.cpp:280`
- `0x46d4c0` `zImage::GetDefaultImageRefPtr` -> `src/GameZRecoil/zImage/zimg_texture.cpp:337`
- `0x46d550` `zImage::InitTextureDirectory` -> `src/GameZRecoil/zImage/zimg_texture.cpp:417`
- `0x46d5a0` `zVid_Image::ReleaseIfNotDefault` -> `src/GameZRecoil/zVideo/zvid_main.c:6608`
- `0x46d730` `zImage::ShutdownTextureDirectoryRuntime` -> `src/GameZRecoil/zVideo/zvid_main.c:8018`
- `0x46e250` `zImage::InvalidateLoadedVariantChain` -> `src/GameZRecoil/zImage/zimg_texture.cpp:580`
- `0x46e290` `zImage_TexDirEntryPartial::GetVariantImageAtIndex` -> `src/GameZRecoil/zImage/zimg_texture.cpp:603`
- `0x46e2c0` `zImage::SetPathExtension` -> `src/GameZRecoil/zImage/zimg_texture.cpp:633`
- `0x46e380` `zImage::TexDirSetBaseNameFromPath` -> `src/GameZRecoil/zImage/zimg_texture.cpp:693`
- `0x46e3e0` `zImage_TexDirEntry::BuildMipChain` -> `src/GameZRecoil/zImage/zimg_texture.cpp:733`
- `0x46eb90` `zImage::ShutdownSubsystem` -> `src/GameZRecoil/zImage/zimg_texture.cpp:835`
- `0x46eba0` `zImg::Init` -> `src/GameZRecoil/zImage/zimg_texture.cpp:851`
- `0x46ebb0` `zImage::Shutdown` -> `src/GameZRecoil/zImage/zimg_texture.cpp:867`
- `0x46ebd0` `zImage_InitMissionResources` -> `src/GameZRecoil/zImage/zimg_texture.cpp:885`
- `0x46ec00` `zVid_Image::Create` -> `src/GameZRecoil/zVideo/zvid_main.c:6568`
- `0x46ecc0` `zVid_Image::Destroy` -> `src/GameZRecoil/zVideo/zvid_main.c:6583`
- `0x4902b0` `zVid_Image::CalcPow2ScratchFields` -> `src/GameZRecoil/zVideo/zvid_main.c:6935`

## GameZRecoil/zImage/zvid_buff.c

- `0x48d3e0` `zVid::Noise_ShutdownBuffers` -> `src/GameZRecoil/zVideo/zvid_main.c:5946`
- `0x48d910` `zVid::DrawNoiseRect` -> `src/GameZRecoil/zVideo/zvid_main.c:5968`
- `0x48ff60` `zVid::ShutdownFrameScratchBuffers` -> `src/GameZRecoil/zVideo/zvid_main.c:6036`
- `0x48ff70` `zVid::InitFrameScratchBuffers` -> `src/GameZRecoil/zVideo/zvid_main.c:6025`
- `0x4a6800` `zVideo::GetPrimarySurfaceWidth` -> `src/GameZRecoil/zVideo/zvid_main.c:3881`
- `0x4a6fe0` `zVideo_buff::CopySurfaceRectToImage` -> `src/GameZRecoil/zVideo/zvid_main.c:3385`

## GameZRecoil/zInput/zin_bindmap.cpp

- `0x42a4e0` `zInput::BindMap_GetCommandLabel` -> `src/GameZRecoil/zInput/zInput.cpp:1233`
- `0x42a4f0` `zInput::BindMap_GetCommandHint` -> `src/GameZRecoil/zInput/zInput.cpp:1246`

## GameZRecoil/zInput/zin_cmd.cpp

- `0x42a000` `zInput_BindGroupInfo::Destroy` -> `src/GameZRecoil/zInput/zInput.cpp:1066`
- `0x42a2c0` `zInput::BindGroupList_AddCommandToGroup` -> `src/GameZRecoil/zInput/zInput.cpp:1127`
- `0x42a480` `zInput::BindGroupList_GetCount` -> `src/GameZRecoil/zInput/zInput.cpp:1166`
- `0x42a4a0` `zInput::BindGroupList_GetGroupTitle` -> `src/GameZRecoil/zInput/zInput.cpp:1182`
- `0x42a4b0` `zInput::BindGroupList_GetGroupCommandCount` -> `src/GameZRecoil/zInput/zInput.cpp:1197`
- `0x42a4d0` `zInput::BindGroupList_GetGroupCommandId` -> `src/GameZRecoil/zInput/zInput.cpp:1217`
- `0x42a9d0` `zInput_BindGroupInfoVec::Count` -> `src/GameZRecoil/zInput/zInput.cpp:1367`

## GameZRecoil/zInput/zin_ff.cpp

- `0x42f9f0` `zInput_DI_InitForceFeedbackEffectSet` -> `src/GameZRecoil/zInput/zInput.cpp:1440`
- `0x42fa80` `zInput_DI_IsForceFeedbackEnabled` -> `src/GameZRecoil/zInput/zInput.cpp:1473`
- `0x42faa0` `zInput_DI_RestartPrimaryFireEffect` -> `src/GameZRecoil/zInput/zInput.cpp:1487`
- `0x42fac0` `zInput_DI_PlayAltFireEffect` -> `src/GameZRecoil/zInput/zInput.cpp:1507`
- `0x42fb50` `zInputDI::PlayCollisionImpactEffect` -> `src/GameZRecoil/zInput/zInput.cpp:1545`
- `0x42fc90` `zInputDI::PlayDamageHitEffect` -> `src/GameZRecoil/zInput/zInput.cpp:1577`
- `0x42fdc0` `zInput_DI_UpdateSteerAndPitchForceEffects` -> `src/GameZRecoil/zInput/zInput.cpp:1643`
- `0x42ffa0` `zInput_DI_CreateConstantForceEffectScaled` -> `src/GameZRecoil/zInput/zInput.cpp:1857`
- `0x430070` `zInput_DI_CreateConstantForceEffectWithDirection` -> `src/GameZRecoil/zInput/zInput.cpp:1885`
- `0x430100` `zInput_DI_CreateSineEffectScaled` -> `src/GameZRecoil/zInput/zInput.cpp:1912`
- `0x472450` `zInput_DI_CreateForceFeedbackEffect` -> `src/GameZRecoil/zInput/zin_joystick.cpp:472`
- `0x472480` `zInput_DI_HasForceFeedback` -> `src/GameZRecoil/zInput/zin_joystick.cpp:647`

## GameZRecoil/zInput/zin_init.cpp

- `0x429f10` `zInput::BindGroupList_StaticInitAndRegisterAtExit` -> `src/GameZRecoil/zInput/zInput.cpp:952`
- `0x429f20` `zInput::BindGroupListStaticInit` -> `src/GameZRecoil/zInput/zInput.cpp:964`
- `0x429f40` `zInput::BindGroupListRegisterAtExit` -> `src/GameZRecoil/zInput/zInput.cpp:983`
- `0x429f50` `zInput::BindGroupListAtExitDestructor` -> `src/GameZRecoil/zInput/zInput.cpp:993`
- `0x4719e0` `zInput::GlobalStateStaticInitAndRegisterAtExit` -> `src/GameZRecoil/zInput/zin_init.cpp:6`
- `0x4719f0` `zInput::GlobalStateStaticInit` -> `src/GameZRecoil/zInput/zin_init.cpp:30`
- `0x471a00` `zInput::GlobalStateRegisterAtExit` -> `src/GameZRecoil/zInput/zin_init.cpp:41`
- `0x471a10` `zInput::GlobalStateAtExitDestructor` -> `src/GameZRecoil/zInput/zin_init.cpp:52`
- `0x471a20` `zInput_GlobalState::Destructor` -> `src/GameZRecoil/zInput/zin_init.cpp:63`
- `0x471ab0` `zInput_GlobalState::Constructor` -> `src/GameZRecoil/zInput/zin_init.cpp:83`
- `0x471ae0` `zInput::OnAppDeactivate` -> `src/GameZRecoil/zInput/zin_init.cpp:104`
- `0x471b20` `zInput::OnAppActivate` -> `src/GameZRecoil/zInput/zin_init.cpp:130`
- `0x471b50` `zInput::Init` -> `src/GameZRecoil/zInput/zin_init.cpp:152`
- `0x471c10` `zInput::Shutdown` -> `src/GameZRecoil/zInput/zin_init.cpp:198`
- `0x471c60` `zInput::Mouse_IsUnsuspended` -> `src/GameZRecoil/zInput/zin_init.cpp:236`
- `0x471c70` `zInput::Joystick_IsUnsuspended` -> `src/GameZRecoil/zInput/zin_init.cpp:246`
- `0x471c80` `zInput_Keyboard_IsUnsuspended` -> `src/GameZRecoil/zInput/zin_init.cpp:266`
- `0x471c90` `zInput::Mouse_ResumeFromSuspend` -> `src/GameZRecoil/zInput/zin_init.cpp:331`
- `0x471cb0` `zInput::Joystick_ResumeFromSuspend` -> `src/GameZRecoil/zInput/zin_init.cpp:348`
- `0x471cd0` `zInput::Keyboard_ResumeFromSuspend` -> `src/GameZRecoil/zInput/zin_init.cpp:365`
- `0x471cf0` `zInput::Mouse_Suspend` -> `src/GameZRecoil/zInput/zin_init.cpp:382`
- `0x471d00` `zInput::Joystick_Suspend` -> `src/GameZRecoil/zInput/zin_init.cpp:391`
- `0x471d10` `zInput::Keyboard_Suspend` -> `src/GameZRecoil/zInput/zin_init.cpp:400`
- `0x471d20` `zInput::Keyboard_AddRef` -> `src/GameZRecoil/zInput/zin_init.cpp:409`
- `0x471d50` `zInput::DI_AddJoystickRef` -> `src/GameZRecoil/zInput/zin_init.cpp:426`
- `0x471d80` `zInput::DI_ReleaseJoystickRef` -> `src/GameZRecoil/zInput/zin_init.cpp:443`
- `0x471da0` `zInput::Mouse_AddRef` -> `src/GameZRecoil/zInput/zin_init.cpp:458`
- `0x471dd0` `zInput::DI_GetJoystickRefCount` -> `src/GameZRecoil/zInput/zin_init.cpp:475`

## GameZRecoil/zInput/zin_joystick.cpp

- `0x42e170` `zInput::DI_SetJoystickEnabled` -> `src/GameZRecoil/zInput/zInput.cpp:1402`
- `0x471e40` `zInput::DI_InitJoystickDevice` -> `src/GameZRecoil/zInput/zin_joystick.cpp:6`
- `0x471f60` `zInput::DI_EnumDevicesCallback_SelectFirstJoystick` -> `src/GameZRecoil/zInput/zin_joystick.cpp:81`
- `0x471fd0` `zInput::DI_ApplyAxisConfig` -> `src/GameZRecoil/zInput/zin_joystick.cpp:122`
- `0x4721a0` `zInput::DI_SetAxisDeadzone` -> `src/GameZRecoil/zInput/zin_joystick.cpp:223`
- `0x4721e0` `zInput::DI_SetAxisRange` -> `src/GameZRecoil/zInput/zin_joystick.cpp:251`
- `0x472230` `zInput::DI_GetAxisRange` -> `src/GameZRecoil/zInput/zin_joystick.cpp:282`
- `0x472280` `zInput::Joystick_ShutdownDevice` -> `src/GameZRecoil/zInput/zin_joystick.cpp:315`
- `0x4722b0` `zInput::DI_IsJoystickDeviceReady` -> `src/GameZRecoil/zInput/zin_joystick.cpp:336`

## GameZRecoil/zInput/zin_kbd.cpp

- `0x46f300` `zInput::Keyboard_InitDevice` -> `src/GameZRecoil/zInput/zin_kbd.cpp:6`
- `0x46f420` `zInput::Keyboard_ShutdownDevice` -> `src/GameZRecoil/zInput/zin_kbd.cpp:99`
- `0x46f980` `zInput::Keyboard_GetKeyTransitionState` -> `src/GameZRecoil/zInput/zin_kbd.cpp:286`

## GameZRecoil/zInput/zin_mouse.cpp

- `0x470020` `zInput::Mouse_ApplyClientCursorPosToOS` -> `src/GameZRecoil/zInput/zin_mouse.cpp:6`
- `0x470060` `zInput::Mouse_UpdateClientRectAndCenter` -> `src/GameZRecoil/zInput/zin_mouse.cpp:26`
- `0x4700a0` `zInput::Mouse_SetNormalizedCursorPos` -> `src/GameZRecoil/zInput/zin_mouse.cpp:46`
- `0x470150` `zInput::Mouse_RecenterCursor` -> `src/GameZRecoil/zInput/zin_mouse.cpp:78`
- `0x470180` `zInput::Mouse_RecenterCursorX` -> `src/GameZRecoil/zInput/zin_mouse.cpp:92`
- `0x470190` `zInput::Mouse_IsInitialized` -> `src/GameZRecoil/zInput/zin_mouse.cpp:103`
- `0x4701a0` `zInput::Mouse_SetClientSizeAndCenter` -> `src/GameZRecoil/zInput/zin_mouse.cpp:112`
- `0x4701f0` `zInput::Mouse_InitDevice` -> `src/GameZRecoil/zInput/zin_mouse.cpp:130`
- `0x470360` `zInput::Mouse_ShutdownDevice` -> `src/GameZRecoil/zInput/zin_mouse.cpp:242`
- `0x4705f0` `zInput::Mouse_GetStateSnapshot` -> `src/GameZRecoil/zInput/zin_mouse.cpp:383`
- `0x470670` `zInput::Mouse_SetCooperativeLevelFlags` -> `src/GameZRecoil/zInput/zin_mouse.cpp:431`
- `0x470680` `zInput::Mouse_WaitForButtonPress` -> `src/GameZRecoil/zInput/zin_mouse.cpp:445`

## GameZRecoil/zInput/zin_opt.cpp

- `0x408390` `zInp::SetJoystickOption` -> `src/GameZRecoil/zInput/zInput.cpp:902`
- `0x4083a0` `zInp::SetJoystickAxesCountOption` -> `src/GameZRecoil/zInput/zInput.cpp:915`
- `0x4083b0` `zInp::SetJoystickButtonCountOption` -> `src/GameZRecoil/zInput/zInput.cpp:926`
- `0x4083c0` `zInp::GetJoystickOption` -> `src/GameZRecoil/zInput/zInput.cpp:937`

## GameZRecoil/zInput/zinput.cpp

- `0x4710a0` `zInput::BindMapSystem_Init` -> `src/GameZRecoil/zInput/zInput.cpp:2642`
- `0x471660` `zInput::BindMapSystem_Shutdown` -> `src/GameZRecoil/zInput/zInput.cpp:2823`

## GameZRecoil/zMath.cpp

- `0x472670` `zMath::Vec3DeltaLengthSq` -> `src/GameZRecoil/zMath/zmth_main.c:1103`
- `0x4726d0` `zMath::Vec3DeltaLength` -> `src/GameZRecoil/zMath/zmth_main.c:1085`

## GameZRecoil/zMath/zmath_matrix.cpp

- `0x473690` `zMath_Mat_Scale` -> `src/GameZRecoil/zMath/zmth_main.c:1789`

## GameZRecoil/zMath/zmath_proj.cpp

- `0x4743e0` `zMath_SetScreenSize` -> `src/GameZRecoil/zMath/zmth_main.c:496`
- `0x474400` `zMath_Setup_Projection` -> `src/GameZRecoil/zMath/zmth_main.c:508`

## GameZRecoil/zMath/zmath_vec.cpp

- `0x474f40` `zMath::Vec3RotateY` -> `src/GameZRecoil/zMath/zmth_main.c:1527`

## GameZRecoil/zMath/zmath_vec2.cpp

- `0x472cc0` `zMath::Vec3Perp2D` -> `src/GameZRecoil/zMath/zmth_main.c:776`

## GameZRecoil/zMath/zmath_vec3.cpp

- `0x42d560` `zMath::Vec3Midpoint` -> `src/GameZRecoil/zMath/zmth_main.c:1061`
- `0x472730` `zMath::Vec3DistSqXZ` -> `src/GameZRecoil/zMath/zmth_main.c:1120`
- `0x472770` `zMath::Vec3ScaleAdd` -> `src/GameZRecoil/zMath/zmth_main.c:810`
- `0x4727a0` `zMath_Vec3_DivScalar` -> `src/GameZRecoil/zMath/zmth_main.c:2451`
- `0x4727f0` `zMath::Vec3NormalizeXZ` -> `src/GameZRecoil/zMath/zmth_main.c:754`
- `0x472860` `zMath::Vec3Reflect` -> `src/GameZRecoil/zMath/zmth_main.c:827`
- `0x472960` `zMath::Vec3Lerp` -> `src/GameZRecoil/zMath/zmth_main.c:863`
- `0x4729b0` `zMath::Vec3DirectionTo` -> `src/GameZRecoil/zMath/zmth_main.c:895`
- `0x4729f0` `zMath::Vec3LerpNormalize` -> `src/GameZRecoil/zMath/zmth_main.c:878`
- `0x472a10` `zMath::Vec3Slerp` -> `src/GameZRecoil/zMath/zmth_main.c:915`

## GameZRecoil/zModel/zModel_Display.cpp

- `0x476370` `VariantTag::TagsOverlap` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1323`
- `0x476400` `VariantTag::CurrentAllowsId` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1362`
- `0x478c70` `zVideo_FrustumTestSphereClipMask` -> `src/GameZRecoil/zVideo/zvid_main.c:2175`

## GameZRecoil/zNetwork.cpp

- `0x48b980` `zNetwork_GetLocalPlayerColorIndex` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:374`
- `0x48b9a0` `zNetwork_GetPlayerColorIndexByKey` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:387`

## GameZRecoil/zNetwork/znet_dplay.cpp

- `0x489f90` `zNetwork::SetFatalDisconnectCallback` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2651`
- `0x48a0d0` `zNetwork_DPlay::RefreshServiceProviderList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:844`
- `0x48a130` `zNetworkDPlay::RefreshAndGetServiceProviderList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:878`
- `0x48a520` `zNetworkDPlay::OpenSelectedSessionAndReadStatusFields` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1286`
- `0x48a9c0` `zNetwork_DPlay::CreateLocalPlayerRecordAndRegister` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1506`
- `0x48acf0` `zNetwork_DPlay_SendUnreliable` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:514`
- `0x48ad30` `zNetwork_DPlay_SendReliable` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:541`
- `0x48ad70` `zNetwork_DPlay_SendExUnreliableTracked` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:568`
- `0x48ae10` `zNetwork_DPlay_SendExReliable` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:613`
- `0x48b3a0` `zNetworkDPlay::EnumConnectionsCallback_AddServiceProviderInfo` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:801`
- `0x48bbe0` `zNetworkDPlay::SelectTcpIpProviderAndEnumSessions` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1952`
- `0x48be10` `zNetworkDPlay::CreateLobby3AInterface` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2021`
- `0x48be70` `zNetworkDPlay::EnumSessionsForCurrentApp` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2053`
- `0x48c060` `zNetwork_SendPacketUnreliable` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:645`
- `0x48c080` `zNetwork_SendPacketReliable` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:667`

## GameZRecoil/zNetwork/zNetwork.cpp

- `0x489d00` `zNetwork::InitSessionRuntime` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2662`
- `0x48b9d0` `zNetwork_GetPlayerRecordCount` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:410`
- `0x48bab0` `zNetwork_ExtractStatusFieldsFromSessionDesc` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:419`
- `0x48bb20` `zNetwork_ApplyStatusFieldsToSessionDesc` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:445`
- `0x48bf40` `zNetwork::DeleteAllDispatchHandlers` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2086`
- `0x48c0a0` `zNetwork::RegisterPacketHandler` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2233`

## GameZRecoil/zOptions/zopt.cpp

- `0x408120` `zOpt::SetPlayerName` -> `src/GameZRecoil/zGame/zgame_opt.c:2157`
- `0x408190` `zOpt::GetPlayerName` -> `src/GameZRecoil/zGame/zgame_opt.c:2187`
- `0x4082a0` `zOpt::SetFullscreenOption` -> `src/GameZRecoil/zGame/zgame_opt.c:2281`
- `0x408330` `zOpt::GetFullscreenOption` -> `src/GameZRecoil/zGame/zgame_opt.c:2335`
- `0x408650` `zOpt::GetDisplaySection` -> `src/GameZRecoil/zGame/zgame_opt.c:2657`
- `0x408690` `zOpt::GetDisplaySectionBitsPerPixel` -> `src/GameZRecoil/zGame/zgame_opt.c:2701`
- `0x4086a0` `zOpt::GetVideoStrideValue` -> `src/GameZRecoil/zGame/zgame_opt.c:2710`
- `0x4086c0` `zOpt::GetWindowSection` -> `src/GameZRecoil/zGame/zgame_opt.c:2719`
- `0x4086d0` `zOpt::GetWindowSectionHeight` -> `src/GameZRecoil/zGame/zgame_opt.c:2728`

## GameZRecoil/zReader/zreader.cpp

- `0x48cda0` `zReader_AllocateNode` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1566`
- `0x48cdc0` `zReader::LoadNodeFromPath` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1858`
- `0x48ce40` `zReader::FreeLoadedTree` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1895`
- `0x48ce60` `zReader_FreeNodeRecursive` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1713`
- `0x48cec0` `zReader_FindChildRecursive` -> `src/GameZRecoil/zReader/zreader_lookup_impl_body.h:7`
- `0x48cf70` `zReader_GetNamedNode` -> `src/GameZRecoil/zReader/zreader_lookup_impl_body.h:51`
- `0x48d080` `zReader_ReadNode` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1620`
- `0x48d1c0` `zReader_OpenFileFromMountedArchives` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1738`
- `0x4a6110` `zReader_ReadString` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1579`

## GameZRecoil/zRender/zrndr_draw.c

- `0x499a20` `zRndr_SubmitPolyWithSpanList` -> `src/GameZRecoil/zRender/zrndr_draw.c:10688`
- `0x499c40` `zRndr_SubmitTexturedPolyUniformAlphaOrShade` -> `src/GameZRecoil/zRender/zrndr_draw.c:10784`
- `0x499ec0` `zRndr_SubmitTexturedPolyPerVertexAlphaOrShade` -> `src/GameZRecoil/zRender/zrndr_draw.c:10924`

## GameZRecoil/zRndr/zRndr_Draw.cpp

- `0x4903e0` `zRndr::SetVideoStrideMirrors` -> `src/GameZRecoil/zRender/zrndr_draw.c:2368`
- `0x490430` `zRndr::SetPerspectiveTextureDeltaX` -> `src/GameZRecoil/zRender/zrndr_draw.c:1689`
- `0x4904a0` `zRndr::SetPerspectiveTextureFarZ` -> `src/GameZRecoil/zRender/zrndr_draw.c:1721`
- `0x490520` `zRndr::SpanOcclusionInit` -> `src/GameZRecoil/zRender/zrndr_draw.c:2460`
- `0x490590` `zRndr::SpanOcclusionBuildColumnHeadTable` -> `src/GameZRecoil/zRender/zrndr_draw.c:2493`
- `0x490600` `zRndr::SpanOcclusionResetFrame` -> `src/GameZRecoil/zRender/zrndr_draw.c:2849`
- `0x490710` `zRndr::SpanOcclusionAddPolygon` -> `src/GameZRecoil/zRender/zrndr_draw.c:2380`
- `0x490780` `zRndr::SpanOcclusionShutdown` -> `src/GameZRecoil/zRender/zrndr_draw.c:2860`
- `0x4907c0` `zRndr_SpanOcclusion_TestSpanDepthOrderPair` -> `src/GameZRecoil/zRender/zrndr_draw.c:9513`
- `0x490ae0` `zRndr_SpanOcclusion_InsertSpanNode_Local` -> `src/GameZRecoil/zRender/zrndr_draw.c:8586`
- `0x4912a0` `zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest` -> `src/GameZRecoil/zRender/zrndr_draw.c:8808`
- `0x491840` `zRndr_SpanOcclusion_BuildSpanList` -> `src/GameZRecoil/zRender/zrndr_draw.c:8829`
- `0x491da0` `zRndr_SpanOcclusion_BuildSpanListFast` -> `src/GameZRecoil/zRender/zrndr_draw.c:8851`
- `0x491dd0` `zRndr_SpanOcclusion_TestColumnVisibility` -> `src/GameZRecoil/zRender/zrndr_draw.c:8872`
- `0x492000` `zRndr_RasterizePolyWithSpanList` -> `src/GameZRecoil/zRender/zrndr_draw.c:10127`
- `0x4927d0` `zRndr::SpanOcclusionRasterizeOccluderPoly` -> `src/GameZRecoil/zRender/zrndr_draw.c:2530`
- `0x492f00` `zRndr_DrawFlatImmediate` -> `src/GameZRecoil/zRender/zrndr_draw.c:10378`
- `0x4936d0` `zRndr_RasterizePoly` -> `src/GameZRecoil/zRender/zrndr_draw.c:10541`
- `0x493df0` `zRndr_DrawFlatQueued` -> `src/GameZRecoil/zRender/zrndr_draw.c:11515`
- `0x498c40` `zRndr_SpanOcclusion_TestPointVisibility` -> `src/GameZRecoil/zRender/zrndr_draw.c:8945`
- `0x498f90` `zRndr_SpanOcclusion_TestSample` -> `src/GameZRecoil/zRender/zrndr_draw.c:8972`
- `0x499130` `zRndr_TextureMip_SelectVariantImage` -> `src/GameZRecoil/zRender/zrndr_draw.c:11456`

## GameZRecoil/zRndr/zRndr_Fog.cpp

- `0x49b780` `zRndr::BlendPackedColor565WithFogInPlace` -> `src/GameZRecoil/zRender/zrndr_draw.c:8283`

## GameZRecoil/zRndr/zRndr_LensFlare.cpp

- `0x49aa90` `zRndr_LensFlare_DrawSampleStageClipped` -> `src/GameZRecoil/zRender/zrndr_draw.c:12751`
- `0x49b020` `zRndr_LensFlare_DrawVisibleSampleStages` -> `src/GameZRecoil/zRender/zrndr_draw.c:12915`

## GameZRecoil/zRndr/zRndr_Overlay.cpp

- `0x48d6d0` `zRndr_OverlayRect_Submit` -> `src/GameZRecoil/zRender/zrndr_draw.c:11093`
- `0x48d7a0` `zRndr_OverlayRect_FlushSw` -> `src/GameZRecoil/zRender/zrndr_draw.c:11138`

## GameZRecoil/zRndr/zRndr_Span.cpp

- `0x499930` `zRndr_SetPaletteRemapKey` -> `src/GameZRecoil/zRender/zrndr_draw.c:13091`
- `0x499990` `zRndr_SetPaletteRemapKeyFromRgb01` -> `src/GameZRecoil/zRender/zrndr_draw.c:13117`
- `0x499a00` `zRndr_SetPaletteShadeRecipeIndex` -> `src/GameZRecoil/zRender/zrndr_draw.c:13147`
- `0x49b7e0` `zRndr::SpanMasked16FromTex16SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:5900`
- `0x49bbf0` `zRndr::SpanMasked16FromPal8SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:7125`
- `0x49e6c0` `zRndr::SpanCopy16FromTex16SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:5478`
- `0x49edc0` `zRndr::SpanCopy16FromPal8SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:6670`
- `0x49f180` `zRndr::SpanShade16FromPal8SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:7646`

## GameZRecoil/zSound/zsnd.cpp

- `0x4a0990` `zSnd::FindSampleByName` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:478`
- `0x4a0ec0` `zSndSampleSet::FindSampleByName` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:248`

## GameZRecoil/zSound/zsnd_3d.cpp

- `0x4a2e70` `zSnd_GetSpeedOfSoundMps` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1388`
- `0x4a2e80` `zSnd::SetSpeedOfSoundMps` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1397`

## GameZRecoil/zSound/zsnd_grp.cpp

- `0x4a44c0` `zSndPendingList_FindByName` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:231`
- `0x4a44e0` `zSndPendingList_MatchNamePredicate` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:216`

## GameZRecoil/zSys/zsys_cpu.cpp

- `0x4b3050` `zSys::CheckCpuSignatureMask` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:95`
- `0x4b33f0` `zSys::HasCpuidSupport` -> `src/GameZRecoil/zSys/zSys.cpp:266`
- `0x4b33f0` `zSys::HasCpuidSupport` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:45`
- `0x4b3480` `zSys::ReadCpuidFeatureFlags` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:123`
- `0x4b3510` `zSys::ProbeDivZeroFlagBehavior` -> `src/GameZRecoil/zSys/zSys.cpp:429`
- `0x4b3510` `zSys::ProbeDivZeroFlagBehavior` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:359`
- `0x4b3550` `zSys::DetectIs8086ByEflagsHiBits` -> `src/GameZRecoil/zSys/zSys.cpp:437`
- `0x4b3550` `zSys::DetectIs8086ByEflagsHiBits` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:385`
- `0x4b35a0` `zSys::DetectIs80286ByEflagsHiBits` -> `src/GameZRecoil/zSys/zSys.cpp:445`
- `0x4b35a0` `zSys::DetectIs80286ByEflagsHiBits` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:414`
- `0x4b35f0` `zSys::DetectIs80386ByAcFlag` -> `src/GameZRecoil/zSys/zSys.cpp:453`
- `0x4b35f0` `zSys::DetectIs80386ByAcFlag` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:442`
- `0x4b3640` `zSys::ReadCpuidVendorAndFamily` -> `src/GameZRecoil/zSys/zSys.cpp:274`
- `0x4b3640` `zSys::ReadCpuidVendorAndFamily` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:187`
- `0x4b3b00` `zSys::ReadCmosRtcSecondsBcd` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:273`
- `0x4b3b20` `zSys::ReadTsc64` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:289`
- `0x4b3ca0` `zSys::Sub64` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:311`

## GameZRecoil/zVideo/zVid.cpp

- `0x408280` `zVid::SetAccelerationOption` -> `src/GameZRecoil/zVideo/zvid_main.c:2698`
- `0x408290` `zVid::SetHwApiOption` -> `src/GameZRecoil/zVideo/zvid_main.c:2715`
- `0x408720` `zVid::SetVideoModeIndex` -> `src/GameZRecoil/zVideo/zvid_main.c:2798`

## GameZRecoil/zVideo/zvid_dd.c

- `0x4a6930` `zVideo_dd::PrepareWindowForMode` -> `src/GameZRecoil/zVideo/zvid_main.c:11676`
- `0x4a6b60` `zVideo_dd3d::SetPendingWireframeState` -> `src/GameZRecoil/zVideo/zvid_main.c:8158`
- `0x4a7b40` `zVideo_dd::StartupEnumerateAndDefaultSelect` -> `src/GameZRecoil/zVideo/zvid_main.c:11877`
- `0x4a7d20` `zVideo_dd::OpenVideoMode` -> `src/GameZRecoil/zVideo/zvid_main.c:11726`
- `0x4a7d40` `zVideo_dd::ShutdownVideoSystem` -> `src/GameZRecoil/zVideo/zvid_main.c:11894`
- `0x4a7d70` `zVideo_dd::FlipToGDIIfAttached` -> `src/GameZRecoil/zVideo/zvid_main.c:12929`
- `0x4a7d90` `zVideo_dd::BltSwToPrimaryRectDirect` -> `src/GameZRecoil/zVideo/zvid_main.c:12408`
- `0x4a7dd0` `zVideo_dd::BltPrimaryToSwRectDirect` -> `src/GameZRecoil/zVideo/zvid_main.c:12439`
- `0x4a7e10` `zVideo_dd::BltSwToPrimaryRect` -> `src/GameZRecoil/zVideo/zvid_main.c:12589`
- `0x4a8060` `zVideo_dd::LockDirectDrawSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:11914`
- `0x4a80c0` `zVideo_dd::UnlockDirectDrawSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:11963`
- `0x4a8100` `zVideo_dd::LockSurface_WaitRestore` -> `src/GameZRecoil/zVideo/zvid_main.c:11999`
- `0x4a8160` `zVideo_dd::UnlockSurface_WaitRestore` -> `src/GameZRecoil/zVideo/zvid_main.c:12043`
- `0x4a83d0` `zVideo_dd::Image_LazyCreateBackingSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:12129`
- `0x4a84c0` `zVideo_dd::Image_LazyCreateVideoMemorySurface` -> `src/GameZRecoil/zVideo/zvid_main.c:12276`
- `0x4a8500` `zVideo_dd::Image_PopulateSurfaceFromHeapPixels` -> `src/GameZRecoil/zVideo/zvid_main.c:12184`
- `0x4a8650` `zVideo_dd::Image_EnsureSurfaceForCurrentDevice` -> `src/GameZRecoil/zVideo/zvid_main.c:12306`
- `0x4a8680` `zVideo_dd::Image_UploadPixelsToSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:12329`
- `0x4a86f0` `zVideo_dd::Image_ReleaseSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:12377`
- `0x4a8720` `zVideo_dd::SetDisplayMode` -> `src/GameZRecoil/zVideo/zvid_main.c:12941`
- `0x4a8790` `zVideo_dd::SetVideoMode` -> `src/GameZRecoil/zVideo/zvid_main.c:12985`
- `0x4a8800` `zVideo_dd::CreateDirectDraw2ForSelectedDevice` -> `src/GameZRecoil/zVideo/zvid_main.c:11773`
- `0x4a88b0` `zVideo_dd::CreateSurface3FromDesc` -> `src/GameZRecoil/zVideo/zvid_main.c:13179`
- `0x4a88f0` `zVideo_dd::CreateFullscreenSurfacesForRenderer` -> `src/GameZRecoil/zVideo/zvid_main.c:13217`
- `0x4a8920` `zVideo_dd::CreateHalfResBackbufferSurfaces` -> `src/GameZRecoil/zVideo/zvid_main.c:13238`
- `0x4a8b20` `zVideo_dd::CreateFullscreenSoftwareSurfaces` -> `src/GameZRecoil/zVideo/zvid_main.c:13358`
- `0x4a8dc0` `zVideo_dd::CreateFullscreenHardwareSurfaces` -> `src/GameZRecoil/zVideo/zvid_main.c:13507`
- `0x4a8f80` `zVideo_dd::InitFullscreenSoftwarePixelPack` -> `src/GameZRecoil/zVideo/zvid_main.c:13107`
- `0x4a9060` `zVideo_dd::VerifyFullscreenSurfaceLocks` -> `src/GameZRecoil/zVideo/zvid_main.c:13029`
- `0x4a90e0` `zVideo_dd::RestoreDisplaySurfaces` -> `src/GameZRecoil/zVideo/zvid_main.c:13060`
- `0x4a9160` `zVideo_dd::VerifySurfaceStateLocking` -> `src/GameZRecoil/zVideo/zvid_main.c:13699`
- `0x4a91b0` `zVideo_dd::ReleaseAllInterfacesAndSurfaces` -> `src/GameZRecoil/zVideo/zvid_main.c:13616`
- `0x4a9300` `zVideo_dd::TeardownVideoSubsystem` -> `src/GameZRecoil/zVideo/zvid_main.c:13730`
- `0x4a9390` `zVideo_dd::RunDirectDrawDeviceEnumeration` -> `src/GameZRecoil/zVideo/zvid_main.c:11745`
- `0x4a93d0` `zVideo_dd::EnumDirectDrawDeviceCallback` -> `src/GameZRecoil/zVideo/zvid_main.c:11449`
- `0x4a95e0` `zVideo_dd::EnumerateDirect3DDevicesForRecord` -> `src/GameZRecoil/zVideo/zvid_main.c:11817`
- `0x4a96b0` `zVideo_dd::EnumDirect3DDeviceCallback` -> `src/GameZRecoil/zVideo/zvid_main.c:11579`
- `0x4a9890` `zVideo_dd::PaletteSetEntries` -> `src/GameZRecoil/zVideo/zvid_main.c:13766`
- `0x4a9900` `zVideo_dd::GetAcceptedDirectDrawDeviceCountCached` -> `src/GameZRecoil/zVideo/zvid_main.c:11383`
- `0x4a9920` `zVideo_dd::GetHwApiDeviceFeatureFlags` -> `src/GameZRecoil/zVideo/zvid_main.c:13803`
- `0x4a9950` `zVid::QueryDeviceVideoMemoryBytes` -> `src/GameZRecoil/zVideo/zvid_main.c:3001`
- `0x4a9a30` `zVid::QueryTextureMemoryBytes` -> `src/GameZRecoil/zVideo/zvid_main.c:3044`
- `0x4ad6a0` `zVideo_dd::ReportError` -> `src/GameZRecoil/zVideo/zvid_main.c:13819`

## GameZRecoil/zVideo/zvid_ddd3d.c

- `0x4a6b70` `zVideo_dd3d::SetPendingDitherEnable` -> `src/GameZRecoil/zVideo/zvid_main.c:8172`
- `0x4a9940` `zVid::GetSelectedD3DDeviceNameOrDefault` -> `src/GameZRecoil/zVideo/zvid_main.c:3105`
- `0x4a9ac0` `zVideo_dd3d::BeginSceneAndFlushPendingRenderStates` -> `src/GameZRecoil/zVideo/zvid_main.c:8186`
- `0x4a9b40` `zVideo_dd3d::EndScene` -> `src/GameZRecoil/zVideo/zvid_main.c:8232`
- `0x4a9b70` `zVideo_dd3d::PresentDisplayModeSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:8696`
- `0x4a9c20` `zVideo_dd3d::CreateDeviceState` -> `src/GameZRecoil/zVideo/zvid_main.c:9034`
- `0x4aa0f0` `zVideo_dd3d::CreateTextureRecord` -> `src/GameZRecoil/zVideo/zvid_main.c:8766`
- `0x4aa600` `zVideo_dd3d::UploadImageToSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:11162`
- `0x4aa6f0` `zVideo_dd3d::ConvertImagePixelsForTexture` -> `src/GameZRecoil/zVideo/zvid_main.c:11084`
- `0x4aa8b0` `zVideo_dd3d::TextureRecord_LockUploadSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:11054`
- `0x4aa8f0` `zVideo_dd3d::TextureRecord_UnlockUploadSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:11223`
- `0x4aa900` `zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef` -> `src/GameZRecoil/zVideo/zvid_main.c:11243`
- `0x4aa920` `zVideo_dd3d::TextureRecord_FinalizeUpload` -> `src/GameZRecoil/zVideo/zvid_main.c:11261`
- `0x4aa980` `zVideo_dd3d::TextureRecord_Destroy` -> `src/GameZRecoil/zVideo/zvid_main.c:11307`
- `0x4aa9d0` `zVideo_dd3d::TextureRecord_Create` -> `src/GameZRecoil/zVideo/zvid_main.c:11039`
- `0x4aa9e0` `zVideo_dd3d::SetFogEnable` -> `src/GameZRecoil/zVideo/zvid_main.c:9288`
- `0x4aaa30` `zVideo_dd3d::SetFogStart` -> `src/GameZRecoil/zVideo/zvid_main.c:9319`
- `0x4aaa60` `zVideo_dd3d::SetFogEnd` -> `src/GameZRecoil/zVideo/zvid_main.c:9342`
- `0x4aaa90` `zVideo_dd3d::ApplyFogStateFromGlobals` -> `src/GameZRecoil/zVideo/zvid_main.c:9366`
- `0x4aab30` `zVideo_dd3d::UpdateFogColor` -> `src/GameZRecoil/zVideo/zvid_main.c:9412`
- `0x4accc0` `zVideo_dd3d::SetQuadBatchDepthAndRhw` -> `src/GameZRecoil/zVideo/zvid_main.c:9434`
- `0x4ad680` `zVideo_dd3d::FloorPowerOfTwo` -> `src/GameZRecoil/zVideo/zvid_main.c:11014`

## GameZRecoil/zVideo/zvid_init.c

- `0x4a75f0` `zVideo::InitVideoSystem` -> `src/GameZRecoil/zVideo/zvid_main.c:5661`
- `0x4a7af0` `zVideo::SetVideoMode` -> `src/GameZRecoil/zVideo/zvid_main.c:4108`

## GameZRecoil/zVideo/zVideo.cpp

- `0x437ef0` `zVideo::HandleSoftwareModeHotkeyCommand` -> `src/GameZRecoil/zVideo/zvid_main.c:3751`
- `0x44d600` `zVideo_sw::RenderFrame` -> `src/GameZRecoil/zVideo/zvid_main.c:1894`
- `0x46d5d0` `zVid_TexDir::Shutdown` -> `src/GameZRecoil/zVideo/zvid_main.c:8083`
- `0x46d810` `zImage::TexDir_FindOrAppendByPath` -> `src/GameZRecoil/zImage/zimg_texture.cpp:445`
- `0x46de50` `zImage::TexDir_LoadPendingEntries` -> `src/GameZRecoil/zImage/zimg_texture.cpp:508`
- `0x46df50` `zVid_TexturePack_EnsureBuiltinTexturePacksLoaded` -> `src/GameZRecoil/zVideo/zvid_main.c:7739`
- `0x46e720` `zVid_PaletteRemap_BuildPaletteVariant` -> `src/GameZRecoil/zVideo/zvid_main.c:7434`
- `0x46e9b0` `zVid_Image::ResampleSquare` -> `src/GameZRecoil/zVideo/zvid_main.c:7284`
- `0x46eb20` `zImage_Init` -> `src/GameZRecoil/zImage/zimg_texture.cpp:799`
- `0x479ce0` `zVideo_SetActiveViewContext` -> `src/GameZRecoil/zVideo/zvid_main.c:1774`
- `0x47a0c0` `zVideo_UpdateProjectionStateFromCameraData` -> `src/GameZRecoil/zVideo/zvid_main.c:2064`
- `0x48ea20` `zVideo_FxSurface::ApplyBlueTintRect` -> `src/GameZRecoil/zVideo/zvid_main.c:6169`
- `0x48eb80` `zVideo_FxSurface::ApplyGreenMaskRect` -> `src/GameZRecoil/zVideo/zvid_main.c:6236`
- `0x48ec90` `zVideo_FxSurface::DrawColoredLinesBatch` -> `src/GameZRecoil/zVideo/zvid_main.c:6514`
- `0x48ed60` `zVideo_FxSurface::DrawAlphaBlendedLine` -> `src/GameZRecoil/zVideo/zvid_main.c:6297`
- `0x4a66e0` `zVideo::GetDisplayModeBpp` -> `src/GameZRecoil/zVideo/zvid_main.c:3914`
- `0x4a6710` `zVideo::GetSwSurfacePixels` -> `src/GameZRecoil/zVideo/zvid_main.c:3818`
- `0x4a6720` `zVideo::GetSwSurfaceWidth` -> `src/GameZRecoil/zVideo/zvid_main.c:3829`
- `0x4a6730` `zVideo::GetSwSurfaceHeight` -> `src/GameZRecoil/zVideo/zvid_main.c:3840`
- `0x4a6740` `zVideo::GetSwSurfacePitch` -> `src/GameZRecoil/zVideo/zvid_main.c:3851`
- `0x4a6750` `zVideo_dd3d::CallClearZBufferRect` -> `src/GameZRecoil/zVideo/zvid_main.c:8146`
- `0x4a6770` `zVideo::RunPostprocessOnSwBuffer` -> `src/GameZRecoil/zVideo/zvid_main.c:5233`
- `0x4a67e0` `zVideo::GetSwSurfaceLockedFlag` -> `src/GameZRecoil/zVideo/zvid_main.c:3862`
- `0x4a6810` `zVideo::GetPrimarySurfaceHeight` -> `src/GameZRecoil/zVideo/zvid_main.c:3890`
- `0x4a6820` `zVideo::GetPrimarySurfacePitch` -> `src/GameZRecoil/zVideo/zvid_main.c:3902`
- `0x4a6900` `zVideo::PresentOrAdjustSurfacesIfEnabled` -> `src/GameZRecoil/zVideo/zvid_main.c:5295`
- `0x4a6b40` `zVideo::SetRendererTypeAndActivePath` -> `src/GameZRecoil/zVideo/zvid_main.c:3701`
- `0x4a6b90` `zVideo::PixelPack_GetRgbBits` -> `src/GameZRecoil/zVideo/zvid_main.c:5873`
- `0x4a6bd0` `zVideo::PixelPack_GetPackingParams` -> `src/GameZRecoil/zVideo/zvid_main.c:5902`
- `0x4a6bf0` `zVideo::PixelPack_SetupFromMasks` -> `src/GameZRecoil/zVideo/zvid_main.c:3625`
- `0x4a6db0` `zVideo::TexturePixelPack_SetupFromMasks` -> `src/GameZRecoil/zVideo/zvid_main.c:3657`
- `0x4a71c0` `zVideo::SetHalfResAdjustMode` -> `src/GameZRecoil/zVideo/zvid_main.c:3720`
- `0x4a7200` `zVideo::GetPrimarySurfaceRectScratch` -> `src/GameZRecoil/zVideo/zvid_main.c:3802`
- `0x4a7220` `zVideo::SetFogColorFromRgb01` -> `src/GameZRecoil/zVideo/zvid_main.c:5801`
- `0x4a7250` `zVideo_SetPendingFogTargetColorFromRgb01` -> `src/GameZRecoil/zVideo/zvid_main.c:1741`
- `0x4a7300` `zVideo::SetFogTargetColorFromRgb01` -> `src/GameZRecoil/zVideo/zvid_main.c:5816`
- `0x4a7490` `zVideo::SelectHwApiDeviceOrFallback` -> `src/GameZRecoil/zVideo/zvid_main.c:5414`
- `0x4a7520` `zVideo::AtExitReleaseAllInterfacesAndSurfaces` -> `src/GameZRecoil/zVideo/zvid_main.c:5786`
- `0x4a7530` `zVideo::ModuleInit` -> `src/GameZRecoil/zVideo/zvid_main.c:5610`
- `0x4a75e0` `zVideo::ReturnSuccessStub` -> `src/GameZRecoil/zVideo/zvid_main.c:5442`
- `0x4a77a0` `zVideo::BindRendererDispatch` -> `src/GameZRecoil/zVideo/zvid_main.c:5320`
- `0x4a7990` `zVideo::Init_SetSurfaceGeometryFromModeIndex` -> `src/GameZRecoil/zVideo/zvid_main.c:4020`
- `0x4a8870` `zVideo::CommitHwApiDeviceSelection` -> `src/GameZRecoil/zVideo/zvid_main.c:5391`
- `0x4bdc00` `zVideoFxPass3Slot::SetRectAndPayload` -> `src/GameZRecoil/zVideo/zvid_main.c:3215`
- `0x4bed30` `zVideo::zVideoFxPass3Config_UpdateLocal` -> `src/GameZRecoil/zVideo/zvid_main.c:5073`
- `0x4bed50` `zVideo::zVideoFxPass3Config_SetPrimaryElementParamsLocal` -> `src/GameZRecoil/zVideo/zvid_main.c:5089`
- `0x4bee00` `zVideoFxPass3Config::SetInputRectByIndex` -> `src/GameZRecoil/zVideo/zvid_main.c:3348`
- `0x4bef40` `zVideo::FxPass3_SetInputRectByIndex` -> `src/GameZRecoil/zVideo/zvid_main.c:5204`
- `0x4bef70` `zVideo::FxPass3_UpdateLocal` -> `src/GameZRecoil/zVideo/zvid_main.c:5219`
- `0x4c7fd0` `zVideo::LoadPaletteFileAndApplyBrightness` -> `src/GameZRecoil/zVideo/zvid_main.c:3926`
- `0x4c8070` `zVideo::ApplyBrightnessToPaletteEntries` -> `src/GameZRecoil/zVideo/zvid_main.c:3964`

## GameZRecoil/zWeapon/zWeapon.cpp

- `0x4ae380` `OptCatalog::BlendDirectionTowardTarget` -> `src/GameZRecoil/zWeapon/zwep_init.c:1661`
- `0x4ae3c0` `OptCatalog::FindEntryByName` -> `src/GameZRecoil/zWeapon/zwep_init.c:1680`
- `0x4ae450` `OptCatalog::FindEntryById` -> `src/GameZRecoil/zWeapon/zwep_init.c:1700`
- `0x4ae4a0` `OptCatalog::SetPendingSpawnTargetOverrides` -> `src/GameZRecoil/zWeapon/zwep_init.c:1850`
- `0x4ae4b0` `OptCatalog::AllocOrReuseAttachNodeChildClone` -> `src/GameZRecoil/zWeapon/zwep_init.c:2110`
- `0x4ae4e0` `OptCatalog::RecycleAttachNodeClone` -> `src/GameZRecoil/zWeapon/zwep_init.c:2147`
- `0x4ae520` `OptCatalog::ClearRuntimeInstanceAsyncFxHandleCallback` -> `src/GameZRecoil/zWeapon/zwep_init.c:2133`
- `0x4ae530` `OptCatalog::AllocOrReuseAttachNodeClone` -> `src/GameZRecoil/zWeapon/zwep_init.c:2174`
- `0x4ae590` `OptCatalog::RecycleRuntimeInstanceStorage` -> `src/GameZRecoil/zWeapon/zwep_init.c:2597`
- `0x4ae660` `OptCatalog::AllocRuntimeInstance` -> `src/GameZRecoil/zWeapon/zwep_init.c:2211`
- `0x4aeaa0` `OptCatalog::SpawnRuntimeInstanceAt` -> `src/GameZRecoil/zWeapon/zwep_init.c:2430`
- `0x4aeb50` `OptCatalog::RecycleRuntimeInstance` -> `src/GameZRecoil/zWeapon/zwep_init.c:2473`
- `0x4aebc0` `OptCatalog::ClearRuntimeInstances` -> `src/GameZRecoil/zWeapon/zwep_init.c:2520`
- `0x4aebf0` `OptCatalog::RemoveRuntimeInstance` -> `src/GameZRecoil/zWeapon/zwep_init.c:2539`
- `0x4b1ec0` `OptCatalog::CreateTrailRuntimeState` -> `src/GameZRecoil/zWeapon/zwep_init.c:1741`
- `0x4b2130` `OptCatalog::CreateTrailSegmentNodeFromTemplate` -> `src/GameZRecoil/zWeapon/zwep_init.c:1717`

## HudUiFillBitmap.cpp

- `0x4b84d0` `HudUiFillBitmap::~HudUiFillBitmap` -> `src/GameZRecoil/zUI/zui.cpp:9539`

## Similar to TransformNormalBatch but with different stride/layout

- `0x474710` `zMath_Mat_TransformNormalBatchToOut` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:717`

## src/Battlesport/player.cpp

- `0x41bab0` `Player::UpdateGunDispatchRequestsFromTriggerLatches` -> `src/Battlesport/player.cpp:7343`
- `0x421ea0` `Player::CreateFromNamesAtPoseGetState` -> `src/Battlesport/player.cpp:4140`
- `0x423460` `Player::ProcessPendingContactQueues` -> `src/Battlesport/player.cpp:9422`
- `0x423530` `Player::ClearPendingContactQueues` -> `src/Battlesport/player.cpp:7376`
- `0x4236b0` `Player::BuildPendingContactQueues` -> `src/Battlesport/player.cpp:9193`
- `0x423b10` `Player::CollectPendingContactsForSegments` -> `src/Battlesport/player.cpp:7542`
- `0x423c20` `Player::ClassifyPendingContactsForSegment` -> `src/Battlesport/player.cpp:7460`
- `0x423fc0` `Player::SelectAndResolvePreferredPendingCollisionContact` -> `src/Battlesport/player.cpp:7792`
- `0x424010` `PlayerPendingContact::SelectPreferred` -> `src/Battlesport/player.cpp:6989`
- `0x424110` `Player::ResolvePendingWorldCollisionContact` -> `src/Battlesport/player.cpp:7878`
- `0x424150` `PlayerPickupContact::PassesCollectionTest` -> `src/Battlesport/player.cpp:7634`
- `0x424210` `Player::ProcessPendingPickupContacts` -> `src/Battlesport/player.cpp:7596`
- `0x424270` `Player::ResolvePendingCollisionContact` -> `src/Battlesport/player.cpp:7942`
- `0x4248e0` `Player::PreparePendingWorldCollisionResponse` -> `src/Battlesport/player.cpp:7816`
- `0x424ac0` `Player::ResolvePendingPlayerCollisionContact` -> `src/Battlesport/player.cpp:8115`
- `0x424d00` `Player::ProcessTransferContactQueue` -> `src/Battlesport/player.cpp:8174`
- `0x424ed0` `Player::TryResolvePendingCollisionProbeSweep` -> `src/Battlesport/player.cpp:7752`
- `0x4251f0` `Player::CollectPendingCollisionContactsForQuadProbe` -> `src/Battlesport/player.cpp:7693`
- `0x425770` `Player::ApplyPendingCollisionProbeVelocity` -> `src/Battlesport/player.cpp:9479`
- `0x426350` `Player::FloatSign` -> `src/Battlesport/player.cpp:12077`
- `0x426770` `Player::UpdateMasterTypeTrack` -> `src/Battlesport/player.cpp:12747`
- `0x429430` `Player::ApplyPitchRollVelocityImpulseFromDirection` -> `src/Battlesport/player.cpp:7905`
- `0x4294d0` `Player::RebuildSteerBasisFromMotionBasis` -> `src/Battlesport/player.cpp:10441`
- `0x429560` `Player::RebuildSteerBasisFromMotionAxes` -> `src/Battlesport/player.cpp:10584`
- `0x429870` `Player::UpdateYawVelocityFromSteerInput` -> `src/Battlesport/player.cpp:12452`
- `0x429b40` `Player::UpdateBankAndTurnDynamics` -> `src/Battlesport/player.cpp:12126`
- `0x429d30` `Player::ComputeTurnSlipDelta` -> `src/Battlesport/player.cpp:12244`
- `0x429ed0` `Player::StartSlipSfx` -> `src/Battlesport/player.cpp:12097`
- `0x429ef0` `Player::StopSlipSfx` -> `src/Battlesport/player.cpp:12113`
- `0x42b970` `Player::RebuildMotionBasisFromSteerBasis` -> `src/Battlesport/player.cpp:10548`

## Time.cpp

- `0x4a56d0` `Time::Tick` -> `src/GameZRecoil/zSys/zsys_time_impl.h:64`

## unknown original source

- `0x10001010` `ZLocGetID` -> `src/Messages/messages.c:6`
- `0x401000` `CAboutDlg::Constructor` -> `src/Battlesport/about.cpp:4`
- `0x401020` `CAboutDlg::DoDataExchange` -> `src/Battlesport/about.cpp:18`
- `0x401030` `CAboutDlg::GetMessageMap` -> `src/Battlesport/about.cpp:29`
- `0x401040` `CWnd::BeginModalState` -> `src/Battlesport/about.cpp:34`
- `0x401050` `CWnd::EndModalState` -> `src/Battlesport/about.cpp:35`
- `0x401d50` `AINet::HasLineOfSightFromLocalPlayerFxOffset` -> `src/Battlesport/ai_net.h:1497`
- `0x401e50` `AINet::HasLineOfSightFromCameraTarget` -> `src/Battlesport/ai_net.h:1563`
- `0x402080` `AINet::AiRestoreSavedTopLevelState` -> `src/Battlesport/ai_net.h:1682`
- `0x402f10` `AINet::AiFinalizeMode2State1ForAllPlayers` -> `src/Battlesport/ai_net.h:2283`
- `0x402f60` `zMath::Vec3Normalize` -> `src/GameZRecoil/zMath/zmth.h:11`
- `0x403620` `AINetPathProbeFan::InitFromSegment path-width store` -> `src/Battlesport/ai_net.cpp:747`
- `0x403750` `AINet::BuildAiPeerRingsByAiNetId` -> `src/Battlesport/ai_net.cpp:787`
- `0x403830` `AINet::AiDiscardNegativeBranchPathNodes` -> `src/Battlesport/ai_net.cpp:856`
- `0x4038a0` `HudUiBriefingObjectivePicture::DrawWithNoiseOverlay` -> `src/Battlesport/Briefing.cpp:434`
- `0x403e20` `HudUiCompositePanel::Destructor` -> `src/Battlesport/Briefing.cpp:716`
- `0x404400` `Briefing::BuildObjectiveActionsFromIndex` -> `src/Battlesport/Briefing.cpp:945`
- `0x4045b0` `Briefing_ActionQueue::AddHideElement` -> `src/Battlesport/Briefing.cpp:1061`
- `0x404620` `BriefingAction_HideElement::Tick` -> `src/Battlesport/Briefing.cpp:1072`
- `0x404640` `Briefing_ActionQueue::AddShowElement` -> `src/Battlesport/Briefing.cpp:1083`
- `0x4046b0` `BriefingAction_ShowElement::Tick` -> `src/Battlesport/Briefing.cpp:1094`
- `0x4046d0` `Briefing_ActionQueue::AddFadeInElement` -> `src/Battlesport/Briefing.cpp:1106`
- `0x404740` `BriefingAction_FadeInElement::Tick` -> `src/Battlesport/Briefing.cpp:1117`
- `0x404780` `Briefing_ActionQueue::AddSetPanelText` -> `src/Battlesport/Briefing.cpp:1131`
- `0x404850` `BriefingAction_SetPanelText::Tick` -> `src/Battlesport/Briefing.cpp:1144`
- `0x4048a0` `Briefing_ActionQueue::AddSetWidgetImageTimed` -> `src/Battlesport/Briefing.cpp:1158`
- `0x404960` `BriefingAction_SetWidgetImageTimed::Tick` -> `src/Battlesport/Briefing.cpp:1171`
- `0x4049d0` `Briefing_ActionQueue::AddPlaySampleByName` -> `src/Battlesport/Briefing.cpp:1190`
- `0x404aa0` `BriefingAction_PlaySample::Tick` -> `src/Battlesport/Briefing.cpp:1210`
- `0x404b30` `Briefing::SampleEventCallback` -> `src/Battlesport/Briefing.cpp:1245`
- `0x404b40` `Briefing_ActionQueue::AddDelayUntilProgress` -> `src/Battlesport/Briefing.cpp:1257`
- `0x404bb0` `BriefingAction_DelayUntilProgress::Tick` -> `src/Battlesport/Briefing.cpp:1269`
- `0x404c50` `Briefing::SetProgressAndSleep` -> `src/Battlesport/Briefing.cpp:1326`
- `0x404c80` `Briefing::BuildObjectiveActionsGlobal` -> `src/Battlesport/Briefing.cpp:1341`
- `0x404e80` `zError::ReportOldNoOp` -> `src/Battlesport/hud.cpp:493`
- `0x405650` `Player::UpdateThirdPersonCamera` -> `src/Battlesport/hud.cpp:826`
- `0x405c90` `Player::ApplyCameraState` -> `src/Battlesport/hud.cpp:1064`
- `0x406890` `MfcThreeFloatDialog::OnKillFocusValue0` -> `src/Battlesport/mfc_three_float_dialog_body.h:215`
- `0x4068c0` `MfcThreeFloatDialog::OnKillFocusValue1` -> `src/Battlesport/mfc_three_float_dialog_body.h:230`
- `0x4068f0` `MfcThreeFloatDialog::OnKillFocusValue2` -> `src/Battlesport/mfc_three_float_dialog_body.h:245`
- `0x406920` `MfcThreeFloatDialog::OnDeltaposSpinValue0` -> `src/Battlesport/mfc_three_float_dialog_body.h:260`
- `0x406960` `MfcThreeFloatDialog::OnDeltaposSpinValue1` -> `src/Battlesport/mfc_three_float_dialog_body.h:283`
- `0x4069a0` `MfcThreeFloatDialog::OnDeltaposSpinValue2` -> `src/Battlesport/mfc_three_float_dialog_body.h:306`
- `0x4069e0` `MfcThreeFloatDialog::OnMove` -> `src/Battlesport/mfc_three_float_dialog_body.h:329`
- `0x4069f0` `MfcThreeFloatDialog::OnCreate` -> `src/Battlesport/mfc_three_float_dialog_body.h:342`
- `0x406a00` `zStr::ContainsCaseInsensitive` -> `src/Battlesport/hud.cpp:1812`
- `0x406a00` `zStr::ContainsCaseInsensitive` -> `src/Battlesport/zstr_body.h:11`
- `0x406f00` `RecoilStateCheatCode::Destructor` -> `src/Battlesport/hud.cpp:2139`
- `0x407130` `zStub::ReturnOneNoArgs` -> `src/Battlesport/cls_stubs_body.h:4`
- `0x407140` `zStub::ReturnZeroNoArgs` -> `src/Battlesport/cls_stubs_body.h:14`
- `0x407150` `zStub::NoOp1Arg` -> `src/Battlesport/cls_stubs_body.h:25`
- `0x407160` `zStub::ReturnOne2Args` -> `src/Battlesport/cls_stubs_body.h:36`
- `0x4076f0` `zGame::ReturnOnlyStub` -> `src/GameZRecoil/zGame/zgame_opt.c:1071`
- `0x407700` `zGame::Options_LoadGameOptions` -> `src/GameZRecoil/zGame/zgame_opt.c:1391`
- `0x407e00` `zGame::Options_SaveGameOptions` -> `src/GameZRecoil/zGame/zgame_opt.c:1825`
- `0x407f10` `zOpt::SetGameDifficultyMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2002`
- `0x407f20` `zOpt::GetGameDifficultyMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2013`
- `0x407f30` `zOpt::SetEffectsLevelForCurrentHwMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2022`
- `0x407f80` `zOpt::GetEffectsLevelForCurrentHwMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2040`
- `0x407fa0` `zOpt::SetObjectLODForCurrentHwMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2048`
- `0x408030` `zOpt::GetObjectLODForCurrentHwMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2075`
- `0x408050` `zOpt::SetMuteSoundOption` -> `src/GameZRecoil/zGame/zgame_opt.c:2083`
- `0x408060` `zOpt::GetMuteSoundOption` -> `src/GameZRecoil/zGame/zgame_opt.c:2094`
- `0x408070` `zOpt::SetSoundVolumeOption` -> `src/GameZRecoil/zGame/zgame_opt.c:2102`
- `0x408090` `zOpt::GetSoundVolumeOption` -> `src/GameZRecoil/zGame/zgame_opt.c:2113`
- `0x4080a0` `zSnd::SetAudioApiOption` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:164`
- `0x4080b0` `zSnd::GetAudioApiOption` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:176`
- `0x4080c0` `zOpt::SetSoundLODOption` -> `src/GameZRecoil/zGame/zgame_opt.c:2121`
- `0x4080d0` `zOpt::GetSoundLODOption` -> `src/GameZRecoil/zGame/zgame_opt.c:2131`
- `0x4080e0` `zOpt::SetTextureMemoryForCurrentHwMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2139`
- `0x408100` `zOpt::GetTextureMemoryForCurrentHwMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2149`
- `0x408210` `zSnd::SetCDAudioOption` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:185`
- `0x408220` `zSnd::GetCDAudioOption` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:195`
- `0x4082b0` `zOpt::SetHudVisibilityOption` -> `src/GameZRecoil/zGame/zgame_opt.c:2292`
- `0x4082d0` `zOpt::SetHudTypeForCurrentHwMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2302`
- `0x408310` `zVid::GetAccelerationOption` -> `src/GameZRecoil/zVideo/zvid_main.c:2730`
- `0x408320` `zVid::GetHwApiOption` -> `src/GameZRecoil/zVideo/zvid_main.c:2738`
- `0x408340` `zOpt::GetHudVisibilityOption` -> `src/GameZRecoil/zGame/zgame_opt.c:2344`
- `0x408380` `zOpt::GetReplicateMode` -> `src/GameZRecoil/zGame/zgame_opt.c:2361`
- `0x4083d0` `zOpt_ViewRectSection::SetPosition` -> `src/GameZRecoil/zGame/zgame_opt.c:2369`
- `0x408400` `zOpt_ViewRectSection::SetSize` -> `src/GameZRecoil/zGame/zgame_opt.c:2386`
- `0x408430` `zOpt::ViewRectSection_ClampPointToInclusiveBounds` -> `src/GameZRecoil/zGame/zgame_opt.c:2403`
- `0x408480` `zOpt::CameraSection_SetActiveCamera` -> `src/GameZRecoil/zGame/zgame_opt.c:2424`
- `0x4084e0` `zOpt_CameraSection_GetActiveCamera` -> `src/GameZRecoil/zGame/zgame_opt.c:2457`
- `0x408570` `zOpt::RenderSection_SetTargetWindow` -> `src/GameZRecoil/zGame/zgame_opt.c:2537`
- `0x4085a0` `zOpt::GetRenderSection` -> `src/GameZRecoil/zGame/zgame_opt.c:2560`
- `0x4085b0` `zOpt::DisplaySection_SetTargetDisplay` -> `src/GameZRecoil/zGame/zgame_opt.c:2568`
- `0x408660` `zOpt_DisplaySection_GetWidth` -> `src/GameZRecoil/zGame/zgame_opt.c:2668`
- `0x408670` `zOpt_DisplaySection_GetHeight` -> `src/GameZRecoil/zGame/zgame_opt.c:2676`
- `0x4086b0` `zVid::GetVideoModeIndexFromOptions` -> `src/GameZRecoil/zVideo/zvid_main.c:2790`
- `0x408a10` `zOpt::SetWolPasswordFlag` -> `src/GameZRecoil/zGame/zgame_opt.c:2777`
- `0x408a20` `zOpt_GetWolPasswordFlagValue` -> `src/GameZRecoil/zGame/zgame_opt.c:2789`
- `0x408f50` `RecoilStateDialogHost::OnSuspend` -> `src/Battlesport/recoil_state_dialog_host_on_suspend_body.h:7`
- `0x409010` `HudUiZrdWidgetEx17C::EnableChildAtIndex` -> `src/GameZRecoil/zUI/zui.cpp:10091`
- `0x409160` `HudUiCreditsBackButton::OnActivate` -> `src/Battlesport/hud.cpp:3990`
- `0x409180` `HudUiCreditsQuitButton::OnActivate` -> `src/Battlesport/hud.cpp:3999`
- `0x4091c0` `HudUiCreditsPanel::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:5569`
- `0x4091e0` `HudUiZrdScrollingText::Destructor` -> `src/GameZRecoil/zUI/zui.cpp:5586`
- `0x4092a0` `HudUiCreditsPanel::Destructor` -> `src/GameZRecoil/zUI/zui.cpp:5615`
- `0x4092a0` `HudUiCreditsPanel::Destructor` -> `src/GameZRecoil/zUI/zui.cpp:5684`
- `0x409360` `HudUiZrdScrollingText::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:5638`
- `0x409380` `HudUiCreditsPanel::UpdateFadeAndExit` -> `src/GameZRecoil/zUI/zui.cpp:5655`
- `0x409410` `HudUiZrdScrollingText::Update` -> `src/GameZRecoil/zUI/zui.cpp:5694`
- `0x409470` `HudUiZrdScrollingText::UpdateScrollPositions` -> `src/GameZRecoil/zUI/zui.cpp:5716`
- `0x409550` `HudUiZrdScrollingText::OnActivateResetOwnerFade` -> `src/GameZRecoil/zUI/zui.cpp:5750`
- `0x409570` `HudUiZrdScrollingText::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:5759`
- `0x409950` `RecoilStateCredits::StaticInitAndRegisterAtExit` -> `src/Battlesport/recoil_state_credits_body.h:23`
- `0x409960` `RecoilStateCredits::StaticInit` -> `src/Battlesport/recoil_state_credits_body.h:43`
- `0x409970` `RecoilStateCredits::RegisterAtExit` -> `src/Battlesport/recoil_state_credits_body.h:53`
- `0x409980` `RecoilStateCredits::AtExitDestructor` -> `src/Battlesport/recoil_state_credits_body.h:63`
- `0x409990` `RecoilStateCredits::RecoilStateCredits` -> `src/Battlesport/recoil_state_credits_body.h:72`
- `0x4099a0` `RecoilStateDialogHost::OnWndActivate` -> `src/Battlesport/recoil_state_credits_body.h:82`
- `0x4099f0` `RecoilStateCredits::~RecoilStateCredits` -> `src/Battlesport/recoil_state_credits_body.h:103`
- `0x409a60` `RecoilStateCredits::OnTryBecomeCurrent` -> `src/Battlesport/recoil_state_credits_body.h:125`
- `0x409ad0` `RecoilStateDialogHost::OnDeactivate` -> `src/Battlesport/recoil_state_credits_body.h:143`
- `0x409b00` `RecoilStateCredits::QueuePush` -> `src/Battlesport/recoil_state_credits_body.h:163`
- `0x409b20` `HudUiPanelSpan::DestroyAndFree` -> `src/GameZRecoil/zUI/zui.cpp:5957`
- `0x409ef0` `HudUiPanel::DestructorCallback` -> `src/GameZRecoil/zUI/zui.cpp:6083`
- `0x40a590` `HudUiPanel::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:12611`
- `0x40bdf0` `StdPtrVector::ClearNoOpDestroy` -> `src/Battlesport/hud_command_binding_layer_body.h:1289`
- `0x40be90` `HudUiPanel::Invalidate` -> `src/Battlesport/hud_command_binding_layer_body.h:1356`
- `0x40bea0` `HudUiPanel::GetFont` -> `src/Battlesport/hud_command_binding_layer_body.h:1365`
- `0x40beb0` `HudUiPanel::SetFontHandle` -> `src/Battlesport/hud_command_binding_layer_body.h:1373`
- `0x40bec0` `HudUiPanel::EnableWordWrapWithRect` -> `src/Battlesport/hud_command_binding_layer_body.h:1383`
- `0x40bf20` `HudCmdBindingEntry::DeleteAndReturnNull` -> `src/Battlesport/hud_command_binding_layer_body.h:1416`
- `0x40bf80` `HudCmdBindButtonBase::AddBindingEntry` -> `src/Battlesport/hud_command_binding_layer_body.h:1432`
- `0x40c1d0` `HudCmdBindButtonBase::ClearBindingEntries` -> `src/Battlesport/hud_command_binding_layer_body.h:1456`
- `0x40c370` `zSys::ProbePlatformAndVideoCaps` -> `src/GameZRecoil/zSys/zsys_probe_platform.inl:2`
- `0x40c9c0` `HudUiOptionsPanel_Lighting::InitFromOptions` -> `src/Battlesport/hud.cpp:3341`
- `0x40c9c0` `HudUiOptionsPanel_Lighting::InitFromOptions` -> `src/Battlesport/hud.cpp:3352`
- `0x40c9e0` `HudUiOptionsPanel_Lighting::SyncFromOptions` -> `src/Battlesport/hud.cpp:3360`
- `0x40ca20` `HudUiOptionsPanel_Perspective::InitFromOptions` -> `src/Battlesport/hud.cpp:3383`
- `0x40ca20` `HudUiOptionsPanel_Perspective::InitFromOptions` -> `src/Battlesport/hud.cpp:3394`
- `0x40ca40` `HudUiOptionsPanel_Perspective::SyncFromOptions` -> `src/Battlesport/hud.cpp:3402`
- `0x40ca80` `HudUiOptionsPanel_FullHud::InitFromOptions` -> `src/Battlesport/hud.cpp:3415`
- `0x40ca80` `HudUiOptionsPanel_FullHud::InitFromOptions` -> `src/Battlesport/hud.cpp:3426`
- `0x40caa0` `HudUiOptionsPanel_FullHud::OnActivate` -> `src/Battlesport/hud.cpp:3434`
- `0x40caa0` `HudUiCheckToggleWidget::OnActivateThunk` -> `src/GameZRecoil/zUI/zui.cpp:8729`
- `0x40cab0` `HudUiOptionsPanel_ObjectDetail::InitFromOptions` -> `src/Battlesport/hud.cpp:3453`
- `0x40cab0` `HudUiOptionsPanel_ObjectDetail::InitFromOptions` -> `src/Battlesport/hud.cpp:3464`
- `0x40cad0` `HudUiOptionsPanel_ObjectDetail::SyncFromOptions` -> `src/Battlesport/hud.cpp:3472`
- `0x40cad0` `HudUiOptionsPanel_ObjectDetail::SyncFromOptions` -> `src/Battlesport/hud.cpp:3481`
- `0x40caf0` `HudUiOptionsPanel_TextureMemory::InitFromOptions` -> `src/Battlesport/hud.cpp:3489`
- `0x40caf0` `HudUiOptionsPanel_TextureMemory::InitFromOptions` -> `src/Battlesport/hud.cpp:3499`
- `0x40cb10` `HudUiOptionsPanel_TextureMemory::SyncFromOptions` -> `src/Battlesport/hud.cpp:3507`
- `0x40cb10` `HudUiOptionsPanel_TextureMemory::SyncFromOptions` -> `src/Battlesport/hud.cpp:3516`
- `0x40cb30` `HudUiOptionsPanel_Effects::InitFromOptions` -> `src/Battlesport/hud.cpp:3524`
- `0x40cb30` `HudUiOptionsPanel_Effects::InitFromOptions` -> `src/Battlesport/hud.cpp:3534`
- `0x40cb70` `HudUiOptionsPanel_Effects::SyncFromOptions` -> `src/Battlesport/hud.cpp:3553`
- `0x40cb70` `HudUiOptionsPanel_Effects::SyncFromOptions` -> `src/Battlesport/hud.cpp:3562`
- `0x40cb90` `HudUiOptionsPanel_SoundActive::InitFromOptions` -> `src/Battlesport/hud.cpp:3570`
- `0x40cb90` `HudUiOptionsPanel_SoundActive::InitFromOptions` -> `src/Battlesport/hud.cpp:3580`
- `0x40cbb0` `HudUiOptionsPanel_SoundActive::SyncFromOptions` -> `src/Battlesport/hud.cpp:3588`
- `0x40cbb0` `HudUiOptionsPanel_SoundActive::SyncFromOptions` -> `src/Battlesport/hud.cpp:3597`
- `0x40cbd0` `HudUiOptionsPanel_SoundQuality::InitFromOptions` -> `src/Battlesport/hud.cpp:3605`
- `0x40cbd0` `HudUiOptionsPanel_SoundQuality::InitFromOptions` -> `src/Battlesport/hud.cpp:3615`
- `0x40cbf0` `HudUiOptionsPanel_SoundQuality::SyncFromOptions` -> `src/Battlesport/hud.cpp:3623`
- `0x40cc10` `HudUiOptionsPanel_SoundVolume::SyncFromOptions` -> `src/Battlesport/hud.cpp:3632`
- `0x40cc10` `HudUiOptionsPanel_SoundVolume::SyncFromOptions` -> `src/Battlesport/hud.cpp:3642`
- `0x40cc30` `HudUiOptionsPanel_SoundVolume::OnActivate` -> `src/Battlesport/hud.cpp:3650`
- `0x40cc60` `HudUiOptionsPanel_MusicEnable::SyncFromOptions` -> `src/Battlesport/hud.cpp:3660`
- `0x40cc60` `HudUiOptionsPanel_MusicEnable::SyncFromOptions` -> `src/Battlesport/hud.cpp:3670`
- `0x40cc80` `HudUiOptionsPanel_MusicEnable::OnActivate` -> `src/Battlesport/hud.cpp:3678`
- `0x40ccc0` `HudUiOptionsPanel_MusicVolume::SyncFromOptions` -> `src/Battlesport/hud.cpp:3696`
- `0x40ccc0` `HudUiOptionsPanel_MusicVolume::SyncFromOptions` -> `src/Battlesport/hud.cpp:3706`
- `0x40cd00` `HudUiOptionsPanel_MusicVolume::OnActivate` -> `src/Battlesport/hud.cpp:3720`
- `0x40cd30` `HudUiOptionsPanel_Resolution::SyncFromOptions` -> `src/Battlesport/hud.cpp:3733`
- `0x40cd30` `HudUiOptionsPanel_Resolution::SyncFromOptions` -> `src/Battlesport/hud.cpp:3743`
- `0x40ce80` `HudUiOptionsPanel_Resolution::OnActivate` -> `src/Battlesport/hud.cpp:3847`
- `0x40d260` `HudLayoutSW::CrtInitGlobalSingleton` -> `src/GameZRecoil/zUI/zui.cpp:913`
- `0x40d270` `HudLayoutSW::GlobalInit` -> `src/GameZRecoil/zUI/zui.cpp:922`
- `0x40d280` `HudLayoutSW::RegisterAtExit` -> `src/GameZRecoil/zUI/zui.cpp:930`
- `0x40d290` `HudLayoutSW::AtExitDestructor` -> `src/GameZRecoil/zUI/zui.cpp:938`
- `0x40d2a0` `HudLayoutSW::GlobalDestructor` -> `src/GameZRecoil/zUI/zui.cpp:946`
- `0x40d2f0` `HudLayoutHW::CrtInitGlobalSingleton` -> `src/GameZRecoil/zUI/zui.cpp:967`
- `0x40d300` `HudLayoutHW::GlobalInit` -> `src/GameZRecoil/zUI/zui.cpp:976`
- `0x40d310` `HudLayoutHW::RegisterAtExit` -> `src/GameZRecoil/zUI/zui.cpp:984`
- `0x40d320` `HudLayoutHW::AtExitDestructor` -> `src/GameZRecoil/zUI/zui.cpp:992`
- `0x40d3b0` `HudLayoutBase::Destructor` -> `src/GameZRecoil/zUI/zui.cpp:1013`
- `0x40d410` `HudUiMgr::StaticInit` -> `src/GameZRecoil/zUI/zui.cpp:2439`
- `0x40d420` `HudUiMgr::RegisterAtExit` -> `src/GameZRecoil/zUI/zui.cpp:2447`
- `0x40d430` `HudUiMgr::AtExitDestructor` -> `src/GameZRecoil/zUI/zui.cpp:2455`
- `0x40d440` `HudUiMgr::StaticDestructor` -> `src/GameZRecoil/zUI/zui.cpp:2463`
- `0x40d440` `HudUiMgrData::~HudUiMgrData` -> `src/GameZRecoil/zUI/zui.cpp:4244`
- `0x40d590` `HudUiMessage::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.h:1619`
- `0x40d590` `HudUiMessage::Destructor` -> `src/GameZRecoil/zUI/zui.cpp:2476`
- `0x40d600` `HudUiTripletPanel::UnwindDestructFirstItem` -> `src/GameZRecoil/zUI/zui.cpp:2491`
- `0x40d610` `HudUiTripletPanel::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:2499`
- `0x40d660` `HudUiMgrObjectiveBlock::~HudUiMgrObjectiveBlock` -> `src/GameZRecoil/zUI/zui.cpp:2512`
- `0x40d660` `HudUiMgrObjectiveBlock::~HudUiMgrObjectiveBlock` -> `src/GameZRecoil/zUI/zui.cpp:11063`
- `0x40d6e0` `HudUiMgrSensorBlock::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.h:1779`
- `0x40d6e0` `HudUiMgrSensorBlock::Destructor` -> `src/GameZRecoil/zUI/zui.cpp:2523`
- `0x40d780` `HudUiSlot::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.h:1964`
- `0x40d780` `HudUiSlot::~HudUiSlot` -> `src/GameZRecoil/zUI/zui.cpp:2558`
- `0x40d7e0` `HudUiMgrDataPrefix::HudUiMgrDataPrefix` -> `src/GameZRecoil/zUI/zui.cpp:2535`
- `0x40d7e0` `HudUiMgrData::HudUiMgrData` -> `src/GameZRecoil/zUI/zui.cpp:2654`
- `0x40d9d0` `HudUiContainer::SetEnabled` -> `src/GameZRecoil/zUI/zui.cpp:2579`
- `0x40d9e0` `HudUiMeter::ConstructorEx` -> `src/GameZRecoil/zUI/zui.cpp:2589`
- `0x40da00` `HudUiMessage::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:2600`
- `0x40da00` `HudUiMessage::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:2619`
- `0x40daa0` `HudUiMessage::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:2629`
- `0x40dac0` `HudUiCounter::HudUiCounter` -> `src/GameZRecoil/zUI/zui.cpp:2644`
- `0x40db20` `HudUiSlot::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:2666`
- `0x40db20` `HudUiSlot::HudUiSlot` -> `src/GameZRecoil/zUI/zui.cpp:11071`
- `0x40db90` `HudUiSlot::Draw` -> `src/GameZRecoil/zUI/zui.cpp:2675`
- `0x40dbd0` `HudUiSlot::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:2689`
- `0x40e010` `HudUiPanel::SetTextColorsAndMarkDirty` -> `src/GameZRecoil/zUI/zui.cpp:2800`
- `0x40e040` `HudUiPanel::SetShadow` -> `src/GameZRecoil/zUI/zui.cpp:2813`
- `0x40ec90` `HudLayoutBase::Shutdown_Stub` -> `src/GameZRecoil/zUI/zui.cpp:3250`
- `0x40eca0` `HudUiTimerPanel::SetRunning` -> `src/GameZRecoil/zUI/zui.cpp:3258`
- `0x40ecc0` `HudUiTimerPanel::SetElapsedSeconds` -> `src/GameZRecoil/zUI/zui.cpp:3269`
- `0x40ece0` `HudUiTimerPanel::SetSeconds` -> `src/GameZRecoil/zUI/zui.cpp:3280`
- `0x40ed10` `HudUiTimerPanel::GetSeconds` -> `src/GameZRecoil/zUI/zui.cpp:3293`
- `0x40ed20` `HudUiTimerPanel::Update` -> `src/GameZRecoil/zUI/zui.cpp:3302`
- `0x40ed80` `HudUiTimerPanel::ConstructorDefault` -> `src/GameZRecoil/zUI/zui.cpp:3322`
- `0x40ee60` `HudUiTimerPanel::UpdateHMSFromSeconds` -> `src/GameZRecoil/zUI/zui.cpp:3362`
- `0x40ef00` `HudUiTimerPanel::SetTimeSeconds` -> `src/GameZRecoil/zUI/zui.cpp:3384`
- `0x40ef60` `HudUiTimerPanelFloat::ConstructorDefault` -> `src/GameZRecoil/zUI/zui.cpp:3408`
- `0x40f040` `HudUiTimerPanelFloat::Draw` -> `src/GameZRecoil/zUI/zui.cpp:3448`
- `0x40f070` `HudUiCounter::ApplyFromLayoutNode` -> `src/GameZRecoil/zUI/zui.cpp:3462`
- `0x40f0f0` `HudUiCounter::ReleaseStateImages` -> `src/GameZRecoil/zUI/zui.cpp:3486`
- `0x40f130` `HudUiCounter::UpdateLayoutPosition` -> `src/GameZRecoil/zUI/zui.cpp:3500`
- `0x40f1a0` `HudUiMgr::SetModeCounterState` -> `src/GameZRecoil/zUI/zui.cpp:3519`
- `0x40f200` `HudUiTripletPanel::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:3537`
- `0x40f2b0` `HudUiTripletPanel::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:3561`
- `0x40f2d0` `HudUiWidget::HudUiWidget` -> `src/GameZRecoil/zUI/zui.cpp:3576`
- `0x40f2d0` `HudUiWidget::HudUiWidget` -> `src/GameZRecoil/zUI/zui.cpp:10525`
- `0x40f3e0` `HudUiTripletPanel::ShutdownItems_Stub` -> `src/GameZRecoil/zUI/zui.cpp:3653`
- `0x40f400` `HudUiTripletPanel::Draw` -> `src/GameZRecoil/zUI/zui.cpp:3663`
- `0x40f460` `HudUiTripletPanel::SetVisibleCount` -> `src/GameZRecoil/zUI/zui.cpp:3683`
- `0x40f4c0` `HudUiMgr::InitHudLayouts / InitHudLayouts` -> `src/GameZRecoil/zUI/zui.cpp:3719`
- `0x40f9e0` `HudUiPanel::SetTextColor` -> `src/GameZRecoil/zUI/zui.cpp:3857`
- `0x40fa10` `HudUiStatsListElement::Update` -> `src/GameZRecoil/zUI/zui.cpp:3871`
- `0x40fa20` `HudUiStatsListElement::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:3881`
- `0x40fa40` `HudUiStatsListElement::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:3896`
- `0x40fab0` `HudUiPanelSimple::ConstructorDefaultThunk` -> `src/GameZRecoil/zUI/zui.cpp:3911`
- `0x40fac0` `HudUiPanelSimple::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:3923`
- `0x40fb70` `HudUiMeter::HudUiMeter` -> `src/GameZRecoil/zUI/zui.cpp:3955`
- `0x40fb90` `HudUiTimerPanel::ZarWriteTimerDataCallback` -> `src/GameZRecoil/zUI/zui.cpp:3964`
- `0x40fbb0` `HudUiTimerPanel::ZarReadTimerData` -> `src/GameZRecoil/zUI/zui.cpp:3981`
- `0x40fbd0` `HudUiMgr::ShutdownResources` -> `src/GameZRecoil/zUI/zui.cpp:3997`
- `0x40fdd0` `HudUiStringMenu::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:4095`
- `0x40fe90` `HudUiTopMessageStack::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:4125`
- `0x40fef0` `HudUiChatMessageStack::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:4133`
- `0x410140` `HudUiMgr::TickLayoutDelay` -> `src/GameZRecoil/zUI/zui.cpp:4229`
- `0x410ed0` `HudUiMgr::DisableHud` -> `src/Battlesport/hud_runtime_layer_body.h:2164`
- `0x411270` `HudUiMgr::UpdateTargetReticleFromCursor` -> `src/Battlesport/hud_runtime_layer_body.h:2325`
- `0x411710` `HudUiMgr::ReticleStaticAtexitStub` -> `src/Battlesport/hud_runtime_layer_body.h:2517`
- `0x411720` `HudUiMgr::CopyReticleProjection` -> `src/Battlesport/hud_runtime_layer_body.h:2523`
- `0x411740` `HudUiMgr::SetReticleMode` -> `src/Battlesport/hud_runtime_layer_body.h:2538`
- `0x411750` `HudUiMgr::SetNanitePanelCount` -> `src/Battlesport/hud_runtime_layer_body.h:2548`
- `0x411eb0` `HudUiMgrObjective::Update` -> `src/Battlesport/hud_runtime_layer_body.h:2860`
- `0x4126e0` `HudUiMessage::SelectVariantDisplay` -> `src/Battlesport/hud_runtime_layer_body.h:3303`
- `0x412790` `HudUiMessage::ApplySideImageSwap` -> `src/Battlesport/hud_runtime_layer_body.h:3335`
- `0x4127d0` `HudUiMessage::ClearDisplay` -> `src/Battlesport/hud_runtime_layer_body.h:3350`
- `0x412b60` `HudLayoutSW::Constructor` -> `src/Battlesport/hud_runtime_layer_body.h:3418`
- `0x412bd0` `HudLayoutBase::SetActive` -> `src/Battlesport/hud_runtime_layer_body.h:3431`
- `0x412be0` `HudLayoutBase::UpdateAll` -> `src/Battlesport/hud_runtime_layer_body.h:3441`
- `0x412bf0` `HudLayoutBase::Enable` -> `src/Battlesport/hud_runtime_layer_body.h:3451`
- `0x412c00` `HudLayoutBase::Disable` -> `src/Battlesport/hud_runtime_layer_body.h:3459`
- `0x412ea0` `HudLayoutHW::Constructor` -> `src/Battlesport/hud_runtime_layer_body.h:3638`
- `0x413080` `HudLayoutHW::ReleaseImages` -> `src/Battlesport/hud_runtime_layer_body.h:3729`
- `0x413630` `HudUiMgr::TriggerCurrentLayoutOnActivated` -> `src/Battlesport/hud_runtime_layer_body.h:4040`
- `0x413730` `HudUiMgr::DestroySensorWindow` -> `src/Battlesport/hud_runtime_layer_body.h:874`
- `0x4137c0` `HudUiAuxOverlay::ClearTextLines` -> `src/Battlesport/hud_runtime_layer_body.h:924`
- `0x4137f0` `HudUiAuxOverlay::ApplyTextLineOp` -> `src/Battlesport/hud_runtime_layer_body.h:945`
- `0x413910` `HudUiMgr::EnableTopAndChatStacks` -> `src/Battlesport/hud_runtime_layer_body.h:1017`
- `0x413950` `HudUiMgr::DisableTopAndChatStacks` -> `src/Battlesport/hud_runtime_layer_body.h:1028`
- `0x413ec0` `HudUiMessage::LoadWeaponLayoutFromNode` -> `src/Battlesport/hud_runtime_layer_body.h:1387`
- `0x413ff0` `HudUiMessage::ReleaseImages` -> `src/Battlesport/hud_runtime_layer_body.h:1439`
- `0x414180` `HudUiLoadingCheckpoint::AdvanceAndLog` -> `src/Battlesport/hud_runtime_layer_body.h:1513`
- `0x414210` `HudUiLoadingCheckpoint::InitTable` -> `src/Battlesport/hud_runtime_layer_body.h:1548`
- `0x414330` `GameNet::ShowPlayerKillMessage` -> `src/Battlesport/game_net_body.h:3907`
- `0x414390` `GameNet::RefreshPlayerListMenu` -> `src/Battlesport/game_net_body.h:3908`
- `0x4143d0` `GameNet::BeginChatCompose` -> `src/Battlesport/game_net_body.h:3909`
- `0x414550` `GameNet::ChatComposeKeyCallback` -> `src/Battlesport/game_net_body.h:3910`
- `0x414590` `GameNet::EndChatComposeAndSend` -> `src/Battlesport/game_net_body.h:3911`
- `0x414660` `GameNet::EndChatComposeAndSendThunk` -> `src/Battlesport/game_net_body.h:3912`
- `0x414a60` `zInterp_GlobalContext::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:4390`
- `0x414a70` `zInterp_GlobalContext::StaticInit` -> `src/Battlesport/hud.cpp:4402`
- `0x414a80` `zInterp_GlobalContext::RegisterAtExit` -> `src/Battlesport/hud.cpp:4412`
- `0x414a90` `zInterp_GlobalContext::AtExitDestructor` -> `src/Battlesport/hud.cpp:4422`
- `0x414ab0` `zInterp_GlobalContext::zInterp_GlobalContext` -> `src/Battlesport/hud.cpp:4432`
- `0x414ad0` `zInterp_GlobalContext::DispatchHook` -> `src/Battlesport/hud.cpp:4446`
- `0x414b50` `shared.authored_ret4_noop_414b50` -> `src/Battlesport/hud.cpp:4470`
- `0x415100` `RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit` -> `src/Battlesport/recoil_state_main_menu_transition_body.h:60`
- `0x415110` `RecoilStateMainMenuTransition::StaticInit` -> `src/Battlesport/recoil_state_main_menu_transition_body.h:71`
- `0x415120` `RecoilStateMainMenuTransition::RegisterAtExit` -> `src/Battlesport/recoil_state_main_menu_transition_body.h:81`
- `0x415130` `RecoilStateMainMenuTransition::AtExitDestructor` -> `src/Battlesport/recoil_state_main_menu_transition_body.h:91`
- `0x415170` `RecoilStateMainMenuTransition::RecoilStateMainMenuTransition` -> `src/Battlesport/recoil_state_main_menu_transition_body.h:8`
- `0x4151b0` `RecoilStateMainMenuTransition::~RecoilStateMainMenuTransition` -> `src/Battlesport/recoil_state_main_menu_transition_body.h:21`
- `0x415220` `RecoilStateMainMenuTransition::OnTryBecomeCurrent` -> `src/Battlesport/recoil_state_main_menu_transition_on_try_become_current_body.h:47`
- `0x415370` `RecoilStateMainMenuTransition::OnResume` -> `src/Battlesport/recoil_state_main_menu_transition_on_resume_body.h:6`
- `0x4153d0` `RecoilStateMainMenuTransition::OnDeactivate` -> `src/Battlesport/recoil_state_main_menu_transition_on_deactivate_body.h:65`
- `0x415630` `RecoilStateMainMenuTransition::ClearPausedAudioSnapshot` -> `src/Battlesport/recoil_state_main_menu_transition_clear_paused_audio_snapshot_body.h:4`
- `0x415650` `RecoilStateMainMenuTransition::QueueEnter` -> `src/Battlesport/recoil_state_main_menu_transition_queue_enter_body.h:4`
- `0x415670` `RecoilStateMainMenuTransition::SetDeferredVideoModeIndex` -> `src/Battlesport/recoil_state_main_menu_transition_set_deferred_video_mode_index_body.h:4`
- `0x4159d0` `zFMV_Action::Update` -> `src/Battlesport/hud.cpp:4696`
- `0x4159e0` `zFMV_Action::RunBlockingTimed` -> `src/Battlesport/hud.cpp:4706`
- `0x415aa0` `zFMV_Action::~zFMV_Action` -> `src/Battlesport/hud.cpp:4722`
- `0x415ab0` `HudSensorMapNode::Init` -> `src/Battlesport/hud_sensor_tracker_body.h:1247`
- `0x415ac0` `HudSensorMapNode::FreePointArray` -> `src/Battlesport/hud_sensor_tracker_body.h:1257`
- `0x415ae0` `HudSensorMapNode::SetEnabled` -> `src/Battlesport/hud_sensor_tracker_body.h:1268`
- `0x415b10` `HudSensorMapNode::SelectPoint` -> `src/Battlesport/hud_sensor_tracker_body.h:1286`
- `0x415b40` `HudSensorMapNode::InitDefaults` -> `src/Battlesport/hud_sensor_tracker_body.h:1303`
- `0x415b70` `HudSensorMapNode::SetColorRgb` -> `src/Battlesport/hud_sensor_tracker_body.h:1323`
- `0x415bd0` `HudSensorMapNode::LoadFromStream` -> `src/Battlesport/hud_sensor_tracker_body.h:1354`
- `0x415c90` `HudSensorMapNode::UpdateCachedBounds` -> `src/Battlesport/hud_sensor_tracker_body.h:1406`
- `0x415d30` `HudSensorMapNode::DrawOnTracker` -> `src/Battlesport/hud_sensor_tracker_body.h:1951`
- `0x415f40` `HudSensorTracker::DrawDiamondMarker` -> `src/Battlesport/hud_sensor_tracker_body.h:1841`
- `0x415fb0` `HudRectI::ClipOrSplitSegment` -> `src/Battlesport/hud_sensor_tracker_body.h:1076`
- `0x416240` `HudRectI::CalcOutcode` -> `src/Battlesport/hud_sensor_tracker_body.h:967`
- `0x416290` `HudRectI::IsCornerOutcode` -> `src/Battlesport/hud_sensor_tracker_body.h:991`
- `0x4162b0` `HudRectI::SegmentIntersectsEdge` -> `src/Battlesport/hud_sensor_tracker_body.h:1002`
- `0x416390` `HudGeom2D::ClassifyPointAgainstSegment` -> `src/Battlesport/hud_sensor_tracker_body.h:746`
- `0x416480` `HudSensorMapNode::DrawProjectedPath` -> `src/Battlesport/hud_sensor_tracker_body.h:1875`
- `0x416650` `HudSensorTracker::InitNoBounds` -> `src/Battlesport/hud_sensor_tracker_body.h:1484`
- `0x416660` `HudSensorTracker::Init` -> `src/Battlesport/hud_sensor_tracker_body.h:1450`
- `0x4166e0` `HudSensorTracker::SetBounds` -> `src/Battlesport/hud_sensor_tracker_body.h:1553`
- `0x416790` `HudSensorTracker::MapShutdownAndResetThunk` -> `src/Battlesport/hud_sensor_tracker_body.h:2404`
- `0x4167a0` `HudSensorTracker::MapShutdownAndReset` -> `src/Battlesport/hud_sensor_tracker_body.h:2413`
- `0x4167e0` `HudSensorTracker::MapRemoveNode` -> `src/Battlesport/hud_sensor_tracker_body.h:2223`
- `0x416840` `HudSensorTracker::MapInsertNodeAndGrowBounds` -> `src/Battlesport/hud_sensor_tracker_body.h:2261`
- `0x4168d0` `HudSensorTracker::LoadMapFromStream` -> `src/Battlesport/hud_sensor_tracker_body.h:2295`
- `0x4169d0` `HudSensorTracker::LoadMapFromPath` -> `src/Battlesport/hud_sensor_tracker_body.h:2356`
- `0x416a30` `HudSensorTracker::MapOverlayBeginShow` -> `src/Battlesport/hud_sensor_tracker_body.h:1644`
- `0x416ad0` `HudSensorTracker::MapOverlayEndShow` -> `src/Battlesport/hud_sensor_tracker_body.h:1620`
- `0x416b30` `HudSensorTracker::MapOverlayRefToggle` -> `src/Battlesport/hud_sensor_tracker_body.h:1673`
- `0x416b80` `HudSensorTracker::MapZoomIn` -> `src/Battlesport/hud_sensor_tracker_body.h:1697`
- `0x416bb0` `HudSensorTracker::MapZoomOut` -> `src/Battlesport/hud_sensor_tracker_body.h:1709`
- `0x416be0` `HudSensorTracker::UpdateMapScaleLerp` -> `src/Battlesport/hud_sensor_tracker_body.h:1721`
- `0x416c90` `HudSensorTracker::ProjectWorldPointsToOverlay` -> `src/Battlesport/hud_sensor_tracker_body.h:1743`
- `0x416d50` `HudSensorTracker::DrawTrackedSaveStateMarker` -> `src/Battlesport/hud_sensor_tracker_body.h:2046`
- `0x416dd0` `HudSensorTracker::DrawMarkerCross` -> `src/Battlesport/hud_sensor_tracker_body.h:1802`
- `0x416e50` `HudSensorTracker::GetSaveStateRelativeVectorLen` -> `src/Battlesport/hud_sensor_tracker_body.h:1774`
- `0x416ef0` `HudSensorTracker::SetSaveStateMarkerMaxDistance` -> `src/Battlesport/hud_sensor_tracker_body.h:1584`
- `0x416f10` `HudSensorTracker::DrawSaveStateMarker` -> `src/Battlesport/hud_sensor_tracker_body.h:2082`
- `0x417130` `HudSensorTracker::Update` -> `src/Battlesport/hud_sensor_tracker_body.h:2175`
- `0x417220` `HudSensorTracker::SetTrackedSaveState` -> `src/Battlesport/hud_sensor_tracker_body.h:1596`
- `0x417260` `HudSensorTracker::LoadMissionMapAndSfx` -> `src/Battlesport/hud_sensor_tracker_body.h:2382`
- `0x4172c0` `HudSensorTracker::SetObjectiveMarkerEnabledAndColor` -> `src/Battlesport/hud_sensor_tracker_body.h:3575`
- `0x417300` `HudSensorTracker::SetObjectiveMarkerColorBlink` -> `src/Battlesport/hud_sensor_tracker_body.h:3598`
- `0x417360` `HudSensorTracker::ConstructGlobal` -> `src/Battlesport/hud_sensor_tracker_body.h:1517`
- `0x417370` `HudSensorTracker::RegisterGlobalOnExit` -> `src/Battlesport/hud_sensor_tracker_body.h:1530`
- `0x417380` `HudSensorTracker::ShutdownGlobal` -> `src/Battlesport/hud_sensor_tracker_body.h:1541`
- `0x417390` `HudSensorTracker::Constructor` -> `src/Battlesport/hud_sensor_tracker_body.h:1494`
- `0x417430` `HudSensorTracker::WriteMissionDataSection` -> `src/Battlesport/hud_sensor_tracker_body.h:2507`
- `0x4174f0` `HudSensorTracker::ApplyMissionDataAndReload` -> `src/Battlesport/hud_sensor_tracker_body.h:2547`
- `0x417640` `HudSensorTracker::RegisterMissionSectionHandlers` -> `src/Battlesport/hud_sensor_tracker_body.h:2626`
- `0x417680` `HudSensorTracker::ZarMission_SaveCallback` -> `src/Battlesport/hud_sensor_tracker_body.h:4219`
- `0x417690` `HudSensorTracker::ZarMission_RestoreCallback` -> `src/Battlesport/hud_sensor_tracker_body.h:4233`
- `0x4176b0` `HudSensorTracker::ZarMissionLate_SaveCallback` -> `src/Battlesport/hud_sensor_tracker_body.h:4256`
- `0x4176d0` `HudSensorTracker::ZarMissionLate_RestoreCallback` -> `src/Battlesport/hud_sensor_tracker_body.h:4276`
- `0x4177a0` `HudSensorTracker::SetMissionId` -> `src/Battlesport/hud_sensor_tracker_body.h:2687`
- `0x417800` `HudSensorTracker::GetMissionId` -> `src/Battlesport/hud_sensor_tracker_body.h:2702`
- `0x417f60` `HudSensorObjectiveSlot::Reset` -> `src/Battlesport/hud_sensor_tracker_body.h:2485`
- `0x4186f0` `HudSensorTracker::GetObjectiveBriefingStringsAndImageRef` -> `src/Battlesport/hud_sensor_tracker_body.h:3557`
- `0x418c70` `HudSensorTracker::ResetHudForMissionStart` -> `src/Battlesport/hud_sensor_tracker_body.h:3817`
- `0x4192d0` `HudSensorTracker::RunStartAnimsFromZrd` -> `src/Battlesport/hud_sensor_tracker_body.h:3459`
- `0x419470` `HudSensorTracker::SetRuntimeTimerSecAndGoalValue` -> `src/Battlesport/hud_sensor_tracker_body.h:3645`
- `0x419490` `HudSensorTracker::Shutdown` -> `src/Battlesport/hud_sensor_tracker_body.h:2432`
- `0x419aa0` `HudUiNetGameSetupPanel::Constructor` -> `src/Battlesport/hud_ui_net_game_setup_body.h:272`
- `0x41a160` `HudUiNetGameSetupPanel_CancelButton::OnActivate` -> `src/Battlesport/hud_ui_net_game_setup_body.h:576`
- `0x41a190` `HudUiNumericTextInput::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:11458`
- `0x41a190` `HudUiNumericTextInput::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:11467`
- `0x41a200` `HudUiClampedIntTextInput::HudUiClampedIntTextInput` -> `src/GameZRecoil/zUI/zui.cpp:11482`
- `0x41a290` `HudUiNumericTextInput::OnAcceptForwardToCommit` -> `src/GameZRecoil/zUI/zui.cpp:11767`
- `0x41a2a0` `HudUiClampedIntTextInput::OnRawKeyboardChar` -> `src/GameZRecoil/zUI/zui.cpp:11775`
- `0x41a2a0` `HudUiClampedIntTextInput::OnRawKeyboardChar` -> `src/GameZRecoil/zUI/zui.cpp:11786`
- `0x41a2d0` `HudUiClampedIntTextInput::CommitAndGetValue` -> `src/GameZRecoil/zUI/zui.cpp:11803`
- `0x41a350` `HudUiClampedIntStepButton::OnActivate` -> `src/GameZRecoil/zUI/zui.cpp:11844`
- `0x41a3f0` `HudUiNumericTextInput::DestructorThunk` -> `src/GameZRecoil/zUI/zui.cpp:11645`
- `0x41a3f0` `HudUiNumericTextInput::DestructorThunk` -> `src/GameZRecoil/zUI/zui.cpp:11655`
- `0x41a400` `HudUiNetGameSetupPanel::Destructor` -> `src/Battlesport/hud_ui_net_game_setup_body.h:531`
- `0x41a570` `HudUiCycleSelectorWidget::ScalarDeletingDestructorThunk` -> `src/GameZRecoil/zUI/zui.cpp:8969`
- `0x41a5b0` `HudUiNetGameSetupPanel_LaunchButton::OnActivate` -> `src/Battlesport/hud_ui_net_game_setup_body.h:590`
- `0x41a7b0` `HudUiNetGameSetupTextInput::OnActivateFocusAndCursor` -> `src/GameZRecoil/zUI/zui.cpp:11880`
- `0x41a820` `HudUiNetGameSetupPanel_NextWorldButton::OnActivate` -> `src/Battlesport/hud_ui_net_game_setup_body.h:668`
- `0x41a9c0` `HudUiNetGameSetupPanel_PrevWorldButton::OnActivate` -> `src/Battlesport/hud_ui_net_game_setup_body.h:749`
- `0x41ab60` `HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit` -> `src/GameZRecoil/zUI/zui.cpp:11904`
- `0x41ab60` `HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit` -> `src/GameZRecoil/zUI/zui.cpp:11912`
- `0x41ab70` `HudUiNetGameSetupOverlayOwner::StaticInit` -> `src/GameZRecoil/zUI/zui.cpp:11923`
- `0x41ab80` `HudUiNetGameSetupOverlayOwner::RegisterAtExit` -> `src/GameZRecoil/zUI/zui.cpp:11933`
- `0x41ab90` `HudUiNetGameSetupOverlayOwner::AtExitDestructor` -> `src/GameZRecoil/zUI/zui.cpp:11943`
- `0x41aba0` `HudUiNetGameSetupOverlayOwner::HudUiNetGameSetupOverlayOwner` -> `src/GameZRecoil/zUI/zui.cpp:11962`
- `0x41abe0` `HudUiNetGameSetupOverlayOwner::~HudUiNetGameSetupOverlayOwner` -> `src/GameZRecoil/zUI/zui.cpp:11973`
- `0x41ac50` `HudUiNetGameSetupOverlayOwner::OnTryBecomeCurrent` -> `src/GameZRecoil/zUI/zui.cpp:11993`
- `0x41ad20` `HudUiNetGameSetupOverlayOwner::OnDeactivate` -> `src/GameZRecoil/zUI/zui.cpp:12034`
- `0x41ad80` `HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag` -> `src/GameZRecoil/zUI/zui.cpp:12065`
- `0x41ada0` `NetSessionBrowserDialog::Constructor` -> `src/Battlesport/game_net_body.h:1047`
- `0x41ae90` `NetSessionBrowserDialog::ScalarDeletingDestructor` -> `src/Battlesport/game_net_body.h:1059`
- `0x41aeb0` `NetSessionBrowserDialog::Destructor` -> `src/Battlesport/game_net_body.h:1083`
- `0x41af50` `NetSessionBrowserDialog::DoDataExchange` -> `src/Battlesport/game_net_body.h:1092`
- `0x41afd0` `NetSessionBrowserDialog::GetMessageMap` -> `src/Battlesport/game_net_body.h:701`
- `0x41afe0` `NetSessionBrowserDialog::OnInitDialog` -> `src/Battlesport/game_net_body.h:710`
- `0x41b150` `NetSessionBrowserDialog::RefreshSessionList` -> `src/Battlesport/game_net_body.h:1137`
- `0x41b2f0` `NetSessionBrowserDialog::ConnectSelectedProvider` -> `src/Battlesport/game_net_body.h:1231`
- `0x41b510` `NetSessionBrowserDialog::OnOK` -> `src/Battlesport/game_net_body.h:1386`
- `0x41b5a0` `NetSessionBrowserDialog::OnCreateSession` -> `src/Battlesport/game_net_body.h:1434`
- `0x41b660` `NetSessionBrowserDialog::OnTimer` -> `src/Battlesport/game_net_body.h:1338`
- `0x41b680` `NetSessionBrowserDialog::OnDestroy` -> `src/Battlesport/game_net_body.h:1475`
- `0x41b6a0` `NetSessionBrowserDialog::ValidatePlayerName` -> `src/Battlesport/game_net_body.h:1350`
- `0x41b780` `NetSessionBrowserDialog::OnHelpDocs` -> `src/Battlesport/game_net_body.h:1488`
- `0x41b950` `Player::TickRemoteNetworkPlayer` -> `src/Battlesport/player.cpp:8860`
- `0x41c480` `HudUiZrdWidget::ScalarDeletingDestructorThunk` -> `src/GameZRecoil/zUI/zui.cpp:8258`
- `0x41c4a0` `HudUiNumericTextInput::ScalarDeletingDestructorThunk` -> `src/GameZRecoil/zUI/zui.cpp:11679`
- `0x41c4c0` `HudUiZrdWidgetEx17C::ScalarDeletingDestructorThunk` -> `src/GameZRecoil/zUI/zui.cpp:10033`
- `0x41c6e0` `NetSessionConfigDialog::NetSessionConfigDialog` -> `src/Battlesport/game_net_body.h:1560`
- `0x41c7f0` `NetSessionConfigDialog::~NetSessionConfigDialog` -> `src/Battlesport/game_net_body.h:1598`
- `0x41c880` `NetSessionConfigDialog::DoDataExchange` -> `src/Battlesport/game_net_body.h:1616`
- `0x41c970` `NetSessionConfigDialog::GetMessageMap` -> `src/Battlesport/game_net_body.h:1694`
- `0x41c980` `Mission::RegisterMultiplayerMaps` -> `src/Battlesport/game_net_body.h:1837`
- `0x41c990` `NetSessionConfigDialog::InitMapNameStrings` -> `src/Battlesport/game_net_body.h:1848`
- `0x41ca00` `NetSessionConfigDialog::RegisterMapNameCleanup` -> `src/Battlesport/game_net_body.h:1863`
- `0x41ca10` `NetSessionConfigDialog::CleanupMapNameStringsOnExit` -> `src/Battlesport/game_net_body.h:1872`
- `0x41ca30` `NetSessionConfigDialog::OnInitDialog` -> `src/Battlesport/game_net_body.h:1703`
- `0x41cb50` `NetSessionConfigDialog::OnDestroy` -> `src/Battlesport/game_net_body.h:1782`
- `0x41cb90` `NetSessionConfigDialog::OnMapChanged` -> `src/Battlesport/game_net_body.h:1804`
- `0x41ea90` `Player::InitMasterCommonDataList` -> `src/Battlesport/player.cpp:3710`
- `0x41eac0` `Player::InitMasterModalDataList` -> `src/Battlesport/player.cpp:3721`
- `0x41eaf0` `Player::InitAndRegisterUnderwaterFxPass3UiSingleton` -> `src/Battlesport/player.cpp:3732`
- `0x41eb00` `Player::InitUnderwaterFxPass3UiSingleton` -> `src/Battlesport/player.cpp:3742`
- `0x41eb10` `Player::RegisterUnderwaterFxPass3UiOnExit` -> `src/Battlesport/player.cpp:3751`
- `0x41eb20` `Player::ResetUnderwaterFxPass3UiSingleton` -> `src/Battlesport/player.cpp:3760`
- `0x41eb30` `Player_UnderwaterFxPass3Ui::Constructor` -> `src/Battlesport/player.cpp:3525`
- `0x41eb50` `Player::InitAndRegisterProjectileCameraFxPass3UiSingleton` -> `src/Battlesport/player.cpp:3769`
- `0x41eb60` `Player::InitProjectileCameraFxPass3UiSingleton` -> `src/Battlesport/player.cpp:3779`
- `0x41eb70` `Player::RegisterProjectileCameraFxPass3UiCleanup` -> `src/Battlesport/player.cpp:3787`
- `0x41eb80` `Player::ResetProjectileCameraFxPass3UiSingleton` -> `src/Battlesport/player.cpp:3796`
- `0x41eb90` `Player_ProjectileCameraFxPass3Ui::Constructor` -> `src/Battlesport/player.cpp:3549`
- `0x41ec00` `Player::InitSaveStateList` -> `src/Battlesport/player.cpp:3805`
- `0x41ec30` `Player::InitAndRegisterTopMsgPanel1` -> `src/Battlesport/player.cpp:3819`
- `0x41ec40` `Player_TopMsgPanel1::Constructor` -> `src/Battlesport/player.cpp:3610`
- `0x41ec60` `Player::RegisterTopMsgPanel1OnExit` -> `src/Battlesport/player.cpp:3829`
- `0x41ec70` `Player_TopMsgPanel1::Destructor` -> `src/Battlesport/player.cpp:3623`
- `0x41ec80` `Player::InitAndRegisterTopMsgPanel2` -> `src/Battlesport/player.cpp:3838`
- `0x41ec90` `Player_TopMsgPanel2::Constructor` -> `src/Battlesport/player.cpp:3635`
- `0x41ecb0` `Player::RegisterTopMsgPanel2Cleanup` -> `src/Battlesport/player.cpp:3848`
- `0x41ecc0` `Player_TopMsgPanel2::Destructor` -> `src/Battlesport/player.cpp:3648`
- `0x41f010` `Player::BuildMissionSaveData` -> `src/Battlesport/player.cpp:10704`
- `0x41f1d0` `Player::ApplyMissionSaveData` -> `src/Battlesport/player.cpp:10779`
- `0x41f5b0` `Player::ZAR_RegisterSections` -> `src/Battlesport/player.cpp:11058`
- `0x41f5f0` `Player::ZAR_WriteMissionSaveDataSection` -> `src/Battlesport/player.cpp:11084`
- `0x41f640` `Player::ZAR_ReadMissionSaveDataSection` -> `src/Battlesport/player.cpp:11026`
- `0x41f6a0` `Player::ZAR_WriteVehicleListSection` -> `src/Battlesport/player.cpp:11264`
- `0x41f850` `Player::ZAR_ReadVehicleListSection` -> `src/Battlesport/player.cpp:11109`
- `0x41fb80` `Player::ShutdownMissionRuntime` -> `src/Battlesport/player.cpp:16829`
- `0x41fd20` `Player::DestroySaveGameState` -> `src/Battlesport/player.cpp:16760`
- `0x41fe40` `Player::GetAivZrdPath` -> `src/Battlesport/player.cpp:3880`
- `0x41fe50` `zVehicle::SelectZrdByDifficulty` -> `src/Battlesport/player.cpp:3579`
- `0x420be0` `zReader::LoadMoversFromZrd` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1910`
- `0x420c60` `Checkpoint::InstantiateNamedObjects` -> `src/Battlesport/player.cpp:6907`
- `0x420d10` `Player::InitStateFromNameAndMasterCommonData` -> `src/Battlesport/player.cpp:4647`
- `0x421470` `Player::BindModalStateFromMasterModalData` -> `src/Battlesport/player.cpp:4955`
- `0x421790` `Player::InitSpawnStateFromPrimaryModalData` -> `src/Battlesport/player.cpp:5091`
- `0x421830` `Player::SampleGroundAndAlignRootToSurface` -> `src/Battlesport/player.cpp:5129`
- `0x421d60` `zClass_Node::MaskExtraFlagsRecursive` -> `src/GameZRecoil/zClass/Class.c:2970`
- `0x421da0` `zClass_Node::PropagateExtraFlagsRecursive` -> `src/GameZRecoil/zClass/Class.c:2992`
- `0x421de0` `zClass_Node::PropagateFlagsRecursive` -> `src/GameZRecoil/zClass/Class.c:3014`
- `0x421e20` `zReader::BuildResolvedParentDir` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1820`
- `0x421ed0` `Player::BuildCollisionPointsFromModel` -> `src/Battlesport/player.cpp:5234`
- `0x4220f0` `Player::BuildSupportPointsFromModel` -> `src/Battlesport/player.cpp:5293`
- `0x423150` `Player::ExtractVehicleNameFromAivName` -> `src/Battlesport/player.cpp:3890`
- `0x423440` `Player_UnderwaterFxPass3Ui::ApplyBlueTint` -> `src/Battlesport/player.cpp:3559`
- `0x423450` `Player_ProjectileCameraFxPass3Ui::ApplyGreenMask` -> `src/Battlesport/player.cpp:3568`
- `0x425060` `HudSensorTracker::ParseCheckpointNumberFromNode` -> `src/Battlesport/player.cpp:6820`
- `0x425150` `Checkpoint::UpdatePlayerLapProgressAndNotifyNet` -> `src/Battlesport/player.cpp:6945`
- `0x425a20` `Player::TickLocalPlayerControls` -> `src/Battlesport/player.cpp:10043`
- `0x428c20` `Player::UpdateSubVerticalDamping` -> `src/Battlesport/player.cpp:12405`
- `0x429f80` `zInput::BindGroupList_Clear` -> `src/GameZRecoil/zInput/zInput.cpp:1016`
- `0x42a070` `zInput::BindGroupList_AddGroup` -> `src/GameZRecoil/zInput/zInput.cpp:1083`
- `0x42a500` `zInput::BindMap_AddDefaultBinding` -> `src/GameZRecoil/zInput/zInput.cpp:1260`
- `0x42a550` `zInput::BindMap_InitDefaultBindings` -> `src/GameZRecoil/zInput/zInput.cpp:1288`
- `0x42aa40` `Player::GetSaveStateListHead` -> `src/Battlesport/player.cpp:4552`
- `0x42ba50` `zClass_cls_di::SnapProbePointYToBestCandidate` -> `src/GameZRecoil/zClass/cls_di.c:1549`
- `0x42db50` `zCom::QueryInterfaceFromInterfaceMap` -> `src/Battlesport/zcom_body.h:23`
- `0x42dc30` `zCom::ConnectionPointContainer_Advise` -> `src/Battlesport/zcom_body.h:89`
- `0x42dcf0` `zCom::ConnectionPointContainer_Unadvise` -> `src/Battlesport/zcom_body.h:125`
- `0x42dda0` `WestwoodOnlineUpgradeApiInitState::Init` -> `src/Battlesport/wol_api_body.h:236`
- `0x42de10` `RecoilApp::GetMessageMap` -> `src/Battlesport/recoil_app_late_body.h:2980`
- `0x42de60` `RecoilApp::~RecoilApp` -> `src/Battlesport/recoil_app_late_body.h:2970`
- `0x42df10` `RecoilApp_AttractFmvState::~RecoilApp_AttractFmvState` -> `src/Battlesport/recoil_app_late_body.h:3938`
- `0x42df50` `RecoilApp_IntroFmvState::~RecoilApp_IntroFmvState` -> `src/Battlesport/recoil_app_late_body.h:3940`
- `0x42df90` `RecoilApp_IState::~RecoilApp_IState` -> `src/Battlesport/recoil_app.h:47`
- `0x42dfa0` `RecoilApp::RecoilApp` -> `src/Battlesport/recoil_app_late_body.h:2943`
- `0x42e070` `RecoilApp_MissionFmvState::~RecoilApp_MissionFmvState` -> `src/Battlesport/recoil_app_late_body.h:3942`
- `0x42e110` `RecoilApp::CreateMainWnd` -> `src/Battlesport/recoil_app_late_body.h:2253`
- `0x42e220` `RecoilApp::StartEngine` -> `src/Battlesport/recoil_app_late_body.h:2436`
- `0x42e430` `RecoilApp::ShutdownEngine` -> `src/Battlesport/recoil_app_late_body.h:2562`
- `0x42e490` `RecoilApp::LoadZbdAndStartEngine` -> `src/Battlesport/recoil_app_late_body.h:2591`
- `0x42e4d0` `RecoilApp::LoadZbdAndSetupSensorTracker` -> `src/Battlesport/recoil_app_late_body.h:2608`
- `0x42e520` `RecoilApp::InitInstance` -> `src/Battlesport/recoil_app_late_body.h:2062`
- `0x42e930` `RecoilApp::ExitInstance` -> `src/Battlesport/recoil_app_late_body.h:2035`
- `0x42e990` `RecoilApp::ActivateExistingInstance` -> `src/Battlesport/recoil_app_late_body.h:2491`
- `0x42e9f0` `RecoilApp::PreTranslateMessage` -> `src/Battlesport/recoil_app_late_body.h:2514`
- `0x42ea20` `RecoilApp_IntroFmvState::OnTryBecomeCurrent` -> `src/Battlesport/recoil_app_late_body.h:3619`
- `0x42eac0` `RecoilApp_IntroFmvState::OnUpdateShouldQuit` -> `src/Battlesport/recoil_app_late_body.h:3657`
- `0x42eb00` `RecoilApp_FmvState::OnIdleOrDispatch` -> `src/Battlesport/recoil_app_late_body.h:3682`
- `0x42eb10` `RecoilApp_IntroFmvState::OnDeactivate` -> `src/Battlesport/recoil_app_late_body.h:3693`
- `0x42eb20` `RecoilApp_MainMenuPrepState::OnTryBecomeCurrent` -> `src/Battlesport/recoil_app_late_body.h:3701`
- `0x42eb60` `RecoilApp_MainMenuPrepState::OnUpdateShouldQuit` -> `src/Battlesport/recoil_app_late_body.h:3716`
- `0x42eb70` `RecoilApp_AttractFmvState::Constructor` -> `src/Battlesport/recoil_app_late_body.h:3612`
- `0x42ebf0` `RecoilApp_AttractFmvState::OnTryBecomeCurrent` -> `src/Battlesport/recoil_app_late_body.h:3725`
- `0x42ec80` `RecoilApp_AttractFmvState::OnUpdateShouldQuit` -> `src/Battlesport/recoil_app_late_body.h:3765`
- `0x42eca0` `RecoilApp_AttractFmvState::OnDeactivate` -> `src/Battlesport/recoil_app_late_body.h:3782`
- `0x42ecb0` `zUtil::SetMissionZrdrPathsAndMountZbd` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1084`
- `0x42ed30` `RecoilApp_MissionFmvState::Constructor` -> `src/Battlesport/recoil_app_late_body.h:3790`
- `0x42ee40` `HudUiBackgroundContainer::SetEnabled` -> `src/GameZRecoil/zUI/zui.cpp:5464`
- `0x42ee40` `HudUiBackgroundContainer::SetEnabled` -> `src/GameZRecoil/zUI/zui.cpp:5474`
- `0x42eea0` `RecoilApp_PlayState::RecoilApp_PlayState` -> `src/Battlesport/recoil_app_late_body.h:3264`
- `0x42eec0` `RecoilApp_PlayState::OnWndActivate` -> `src/Battlesport/recoil_app_late_body.h:3273`
- `0x42eed0` `RecoilApp_PlayState::OnTryBecomeCurrent` -> `src/Battlesport/recoil_app_late_body.h:3285`
- `0x42f280` `RecoilApp_PlayState::TickAndRenderFrame` -> `src/Battlesport/recoil_app_play_state_tick_and_render_frame_body.h:18`
- `0x42f5e0` `RecoilApp_PlayState::OnUpdateShouldQuit` -> `src/Battlesport/recoil_app_late_body.h:3438`
- `0x42f8a0` `RecoilApp_PlayState::OnResume` -> `src/Battlesport/recoil_app_late_body.h:3535`
- `0x42f8e0` `RecoilApp_PlayState::OnDeactivate` -> `src/Battlesport/recoil_app_late_body.h:3552`
- `0x4301e0` `CZRecoilFrame::CreateObject` -> `src/Battlesport/cz_recoil_frame_body.h:417`
- `0x430230` `CZRecoilFrame::GetBaseRuntimeClass` -> `src/Battlesport/cz_recoil_frame_body.h:407`
- `0x430240` `CZRecoilFrame::GetRuntimeClass` -> `src/Battlesport/cz_recoil_frame_body.h:437`
- `0x430250` `CZRecoilFrame::CZRecoilFrame` -> `src/Battlesport/cz_recoil_frame_body.h:490`
- `0x430610` `CZRecoilFrame::~CZRecoilFrame` -> `src/Battlesport/cz_recoil_frame_body.h:656`
- `0x430680` `CZRecoilFrame::SetMenuBarVisibility` -> `src/Battlesport/cz_recoil_frame_body.h:665`
- `0x4306d0` `CZRecoilFrame::GetBaseMessageMap` -> `src/Battlesport/cz_recoil_frame_body.h:447`
- `0x4306e0` `CZRecoilFrame::GetMessageMap` -> `src/Battlesport/cz_recoil_frame_body.h:467`
- `0x4306f0` `CZRecoilFrame::BuildWindowTitle` -> `src/Battlesport/cz_recoil_frame_body.h:696`
- `0x430740` `CZRecoilFrame::OnMenuStartSinglePlayer` -> `src/Battlesport/cz_recoil_frame_body.h:714`
- `0x430760` `CZRecoilFrame::OnMenuOpenCampaign` -> `src/Battlesport/cz_recoil_frame_body.h:725`
- `0x430770` `CZRecoilFrame::OnOpenFileDialog` -> `src/Battlesport/cz_recoil_frame_body.h:735`
- `0x4308a0` `CZRecoilFrame::OnMenuExitGame` -> `src/Battlesport/cz_recoil_frame_body.h:923`
- `0x4308c0` `CZRecoilFrame::ConfigureModeFeatureFlags` -> `src/Battlesport/cz_recoil_frame_body.h:796`
- `0x4309b0` `CZRecoilFrame::OnMenuSetVideoMode2` -> `src/Battlesport/cz_recoil_frame_body.h:863`
- `0x4309d0` `CZRecoilFrame::OnMenuSetVideoMode3` -> `src/Battlesport/cz_recoil_frame_body.h:873`
- `0x4309f0` `CZRecoilFrame::OnMenuSetVideoMode4` -> `src/Battlesport/cz_recoil_frame_body.h:883`
- `0x430a10` `CZRecoilFrame::OnMenuSetVideoMode5` -> `src/Battlesport/cz_recoil_frame_body.h:893`
- `0x430a30` `CZRecoilFrame::OnMenuSetVideoMode6` -> `src/Battlesport/cz_recoil_frame_body.h:903`
- `0x430a50` `CZRecoilFrame::OnMenuSetVideoMode7` -> `src/Battlesport/cz_recoil_frame_body.h:913`
- `0x430a70` `CZRecoilFrame::OnMenuToggleHud` -> `src/Battlesport/cz_recoil_frame_body.h:937`
- `0x430a90` `CZRecoilFrame::OnUpdateHudCmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:946`
- `0x430ab0` `CZRecoilFrame::OnMenuToggleFullscreen` -> `src/Battlesport/cz_recoil_frame_body.h:958`
- `0x430ad0` `CZRecoilFrame::OnMenuOpenHelpDocs` -> `src/Battlesport/cz_recoil_frame_body.h:967`
- `0x430c90` `RecoilApp::FatalErrorAndExit` -> `src/Battlesport/recoil_app_late_body.h:1991`
- `0x430d80` `CZRecoilFrame::OnMenuOpenMultiplayerSessionBrowser` -> `src/Battlesport/cz_recoil_frame_body.h:1079`
- `0x431270` `CZRecoilFrame::OnMenuStartMultiplayer` -> `src/Battlesport/cz_recoil_frame_body.h:1165`
- `0x431290` `CZRecoilFrame::OnMenuStartCampaignMode` -> `src/Battlesport/cz_recoil_frame_body.h:1179`
- `0x4312b0` `CZRecoilFrame::OnMenuStartCampaignMode2` -> `src/Battlesport/cz_recoil_frame_body.h:1193`
- `0x4312d0` `CZRecoilFrame::OnMenuStartCampaignMode3` -> `src/Battlesport/cz_recoil_frame_body.h:1207`
- `0x4312f0` `CZRecoilFrame::OnMenuStartCampaignMode4` -> `src/Battlesport/cz_recoil_frame_body.h:1221`
- `0x431310` `CZRecoilFrame::OnMenuStartCampaignMode5` -> `src/Battlesport/cz_recoil_frame_body.h:1235`
- `0x431330` `CZRecoilFrame::OnMenuToggleArchiveBanks` -> `src/Battlesport/cz_recoil_frame_body.h:1297`
- `0x431380` `CZRecoilFrame::OnMenuToggleTexturePacks` -> `src/Battlesport/cz_recoil_frame_body.h:1313`
- `0x4313d0` `CZRecoilFrame::OnUpdateVideoMode2CmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1337`
- `0x431430` `CZRecoilFrame::OnUpdateVideoMode3CmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1351`
- `0x431490` `CZRecoilFrame::OnUpdateVideoMode4CmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1365`
- `0x4314f0` `CZRecoilFrame::OnUpdateVideoMode5CmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1379`
- `0x431550` `CZRecoilFrame::OnUpdateVideoMode6CmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1393`
- `0x4315b0` `CZRecoilFrame::OnUpdateVideoMode7CmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1407`
- `0x431610` `CZRecoilFrame::SetHwApiAndInitMode` -> `src/Battlesport/cz_recoil_frame_body.h:1682`
- `0x431680` `CZRecoilFrame::InitFallbackMode` -> `src/Battlesport/cz_recoil_frame_body.h:1703`
- `0x4316c0` `CZRecoilFrame::EnsureHwApiInitialized` -> `src/Battlesport/cz_recoil_frame_body.h:1716`
- `0x431730` `CZRecoilFrame::InitStartupHwApiFromOptions` -> `src/Battlesport/cz_recoil_frame_body.h:1746`
- `0x431790` `CZRecoilFrame::OnMenuSelectHwApi0` -> `src/Battlesport/cz_recoil_frame_body.h:1421`
- `0x4317a0` `CZRecoilFrame::OnMenuSelectHwApi1` -> `src/Battlesport/cz_recoil_frame_body.h:1430`
- `0x4317b0` `CZRecoilFrame::OnMenuSelectHwApi2` -> `src/Battlesport/cz_recoil_frame_body.h:1439`
- `0x4317c0` `CZRecoilFrame::OnMenuSelectHwApi3` -> `src/Battlesport/cz_recoil_frame_body.h:1448`
- `0x4317d0` `CZRecoilFrame::UpdateHwApiMenuItem` -> `src/Battlesport/cz_recoil_frame_body.h:1457`
- `0x431870` `CZRecoilFrame::OnUpdateHwApi0CmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1487`
- `0x4318b0` `CZRecoilFrame::OnUpdateHwApi1CmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1499`
- `0x4318c0` `CZRecoilFrame::OnUpdateHwApi2CmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1513`
- `0x4318d0` `CZRecoilFrame::OnUpdateHwApi3CmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1527`
- `0x4318e0` `CZRecoilFrame::OnUpdateFullscreenCmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1541`
- `0x431900` `CZRecoilFrame::OnMenuToggleCDAudio` -> `src/Battlesport/cz_recoil_frame_body.h:1577`
- `0x431920` `CZRecoilFrame::OnUpdateCDAudioCmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1586`
- `0x431950` `CZRecoilFrame::OnMenuToggleJoystick` -> `src/Battlesport/cz_recoil_frame_body.h:1598`
- `0x431970` `CZRecoilFrame::OnUpdateJoystickCmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1607`
- `0x4319a0` `CZRecoilFrame::OnMenuWestwoodOnlineUpgrade` -> `src/Battlesport/cz_recoil_frame_body.h:1249`
- `0x431a80` `MfcCmdUI::EnableAlways` -> `src/Battlesport/cz_recoil_frame_body.h:478`
- `0x431a90` `CZRecoilFrame::OnMenuSelectDirectSound` -> `src/Battlesport/cz_recoil_frame_body.h:1619`
- `0x431aa0` `CZRecoilFrame::OnUpdateDirectSoundCmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1628`
- `0x431ad0` `CZRecoilFrame::OnMenuSelectA3D` -> `src/Battlesport/cz_recoil_frame_body.h:1640`
- `0x431ae0` `CZRecoilFrame::OnUpdateA3DCmdUI` -> `src/Battlesport/cz_recoil_frame_body.h:1649`
- `0x431b10` `CZRecoilFrame::OnSize` -> `src/Battlesport/cz_recoil_frame_body.h:1661`
- `0x431bf0` `GameNetSpawnPointList::InitGlobals` -> `src/Battlesport/game_net_body.h:3826`
- `0x431c20` `GameNetPlayerRowList::Reset` -> `src/Battlesport/game_net_body.h:3840`
- `0x431c50` `GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks` -> `src/Battlesport/game_net_body.h:2756`
- `0x431dd0` `Net::InitFromZrd` -> `src/Battlesport/game_net_body.h:829`
- `0x4320b0` `GameNet::WaitForLocalPlayerColorIndex` -> `src/Battlesport/game_net_body.h:2901`
- `0x4320f0` `GameNet::ResetRemotePlayersAndSpawnLists` -> `src/Battlesport/game_net_body.h:2862`
- `0x4321b0` `GameNet::UnregisterGameplayPacketHandlers` -> `src/Battlesport/game_net_body.h:2687`
- `0x4322a0` `GameNet::ResetHudTimerPanelNetStateLongCountdown` -> `src/Battlesport/game_net_body.h:2926`
- `0x432300` `GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer` -> `src/Battlesport/game_net_body.h:2063`
- `0x4327e0` `GameNet::HandlePkt06_PlayerStateSnapshot` -> `src/Battlesport/game_net_body.h:2493`
- `0x432830` `GameNet::FindPlayerRowByKey` -> `src/Battlesport/game_net_body.h:1884`
- `0x432860` `GameNet::SpawnRemotePlayerFromPkt06_PlayerStateSnapshot` -> `src/Battlesport/game_net_body.h:2334`
- `0x432ae0` `GameNet::ApplyPkt06_PlayerStateSnapshotToRow` -> `src/Battlesport/game_net_body.h:2239`
- `0x432d60` `GameNet::UpdateRemotePlayerHudWidgetScreenPos` -> `src/Battlesport/game_net_body.h:2983`
- `0x432e70` `GameNet::ReassignPlayerColorsAndRefreshRows` -> `src/Battlesport/game_net_body.h:3046`
- `0x432ed0` `GameNet::HandlePkt03_RemoveRemotePlayer` -> `src/Battlesport/game_net_body.h:3075`
- `0x433000` `GameNet::SendPkt08_PlayerKillEvent` -> `src/Battlesport/game_net_body.h:3371`
- `0x433060` `GameNet::HandlePkt08_PlayerKillEvent` -> `src/Battlesport/game_net_body.h:3326`
- `0x4330f0` `GameNet::SendPkt0E_PlayerLapProgress` -> `src/Battlesport/game_net_body.h:3399`
- `0x433170` `GameNet::HandlePkt0E_PlayerLapProgress` -> `src/Battlesport/game_net_body.h:3584`
- `0x433200` `GameNet::AreAllPlayersAtLapTarget` -> `src/Battlesport/game_net_body.h:1958`
- `0x433250` `GameNet::HandlePkt0D_HudTimerPanelState` -> `src/Battlesport/game_net_body.h:3270`
- `0x433310` `GameNet::SendPkt0D_HudTimerPanelState` -> `src/Battlesport/game_net_body.h:3544`
- `0x433390` `GameNet::SendPkt0C_HudTimerStatusBits` -> `src/Battlesport/game_net_body.h:3694`
- `0x433410` `GameNet::HandlePkt0C_HudTimerStatusBits` -> `src/Battlesport/game_net_body.h:3151`
- `0x4334f0` `GameNet::SendPkt09_PlayerScoreboardSnapshot` -> `src/Battlesport/game_net_body.h:3429`
- `0x4335b0` `GameNet::HandlePkt09_PlayerScoreboardSnapshot` -> `src/Battlesport/game_net_body.h:3476`
- `0x4336f0` `GameNet::GetLocalPlayerColorIndexOrZero` -> `src/Battlesport/game_net_body.h:1904`
- `0x433710` `GameNet::SetStatusBitsFromFlags` -> `src/Battlesport/game_net_body.h:2953`
- `0x433730` `GameNet::GetStatusBitAllowMaps` -> `src/Battlesport/game_net_body.h:2965`
- `0x433740` `GameNet::GetStatusBitNameTags` -> `src/Battlesport/game_net_body.h:2974`
- `0x433750` `GameNet::SendPkt0B_ChatMessage` -> `src/Battlesport/game_net_body.h:3237`
- `0x4337e0` `GameNet::HandlePkt0B_ChatMessage` -> `src/Battlesport/game_net_body.h:3206`
- `0x433840` `GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed` -> `src/Battlesport/game_net_body.h:1981`
- `0x4339d0` `GameNet::GetNearestOtherPlayerDistanceToSpawnPoint` -> `src/Battlesport/game_net_body.h:1924`
- `0x433a40` `HudTimerPanelNetState::ClearTailFlagsLocal` -> `src/Battlesport/game_net_body.h:814`
- `0x433a50` `GameNetPlayerRow::ApplyPlayerColorTint` -> `src/Battlesport/game_net_body.h:779`
- `0x433ad0` `zDEClient_Crater::Execute` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:919`
- `0x433b70` `zDEClient_Crater::NetRelayCallback` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:957`
- `0x433c30` `GameNet::HostSendPkt0F_CraterFeature` -> `src/Battlesport/game_net_body.h:2665`
- `0x433ca0` `GameNet::SendPkt10_QSandEvent` -> `src/Battlesport/game_net_body.h:2609`
- `0x433d40` `zDEClient_QSand::NetRelayCallback` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:993`
- `0x433de0` `GameNet::HostSendPkt10_QSandFeature` -> `src/Battlesport/game_net_body.h:2645`
- `0x434130` `GameNet::SendPkt07_AltGunDispatch` -> `src/Battlesport/game_net_body.h:2575`
- `0x434190` `GameNet::HandlePkt07_AltGunDispatch` -> `src/Battlesport/game_net_body.h:2530`
- `0x434230` `GameNet::AltGunDispatchNoOpCallback` -> `src/Battlesport/game_net_body.h:2595`
- `0x434370` `GameNet::SendPkt13_EffectAnimActivationRecord` -> `src/Battlesport/game_net_body.h:3621`
- `0x4343f0` `GameNet::HandlePkt13_EffectAnimActivationRecord` -> `src/Battlesport/game_net_body.h:3673`
- `0x434430` `GameNet::SendAllPkt13_EffectAnimActivationRecords` -> `src/Battlesport/game_net_body.h:3656`
- `0x434460` `GameNet::SendPkt14_HudTimerAndFlagsSync` -> `src/Battlesport/game_net_body.h:3730`
- `0x4344b0` `GameNet::HandlePkt14_HudTimerAndFlagsSync` -> `src/Battlesport/game_net_body.h:3749`
- `0x434550` `GameNet::HostUpdateSessionDescStatusFields` -> `src/Battlesport/game_net_body.h:3797`
- `0x4345a0` `GameNetPlayerRowList::AppendNewRow` -> `src/Battlesport/game_net_body.h:3852`
- `0x434650` `GameNetPlayerRow::DestroyEmbeddedPanel` -> `src/Battlesport/game_net_body.h:805`
- `0x435e80` `RecoilStateSaveLoadTransition::OnUpdateShouldQuit` -> `src/Battlesport/recoil_state_dialog_host_body.h:10`
- `0x435e80` `RecoilStateSaveLoadTransition::OnUpdateShouldQuit` -> `src/Battlesport/recoil_state_main_menu_transition_on_update_should_quit_body.h:9`
- `0x436630` `zTurret_Runtime::InitDefaults` -> `src/Battlesport/turret.cpp:353`
- `0x4367a0` `zTurret_Runtime::InitFromReaderNode` -> `src/Battlesport/turret.cpp:434`
- `0x436e00` `zTurret_Runtime::Shutdown` -> `src/Battlesport/turret.cpp:1438`
- `0x436e20` `zTurret_Runtime::HasActiveNode` -> `src/Battlesport/turret.cpp:1451`
- `0x436e40` `zTurret_Runtime::Tick` -> `src/Battlesport/turret.cpp:1191`
- `0x437430` `zTurret_Runtime::UpdateFirePositionFromParts` -> `src/Battlesport/turret.cpp:930`
- `0x4374a0` `zTurret_Runtime::UpdateAimAndPartMatrices` -> `src/Battlesport/turret.cpp:956`
- `0x437730` `zTurret_Runtime::SelectFirePointAndAimAtTarget` -> `src/Battlesport/turret.cpp:1052`
- `0x437820` `zTurret_Runtime::FireWeapon` -> `src/Battlesport/turret.cpp:1089`
- `0x437990` `zTurret_Runtime::UpdateFireBurstTimer` -> `src/Battlesport/turret.cpp:1171`
- `0x4379f0` `zTurret_Runtime::ApplyDamageAndHandleDestruction` -> `src/Battlesport/turret.cpp:1365`
- `0x437aa0` `zTurret_System::ResetIterationState` -> `src/Battlesport/turret.cpp:1465`
- `0x437ab0` `zTurret_System::Shutdown` -> `src/Battlesport/turret.cpp:1678`
- `0x437ac0` `zTurret_System::LoadDefinitionsFromPath` -> `src/Battlesport/turret.cpp:1476`
- `0x437ca0` `zTurret_System::TickAllRuntimesRoundRobin` -> `src/Battlesport/turret.cpp:1581`
- `0x437d40` `zTurret_System::DisableTickCallback` -> `src/Battlesport/turret.cpp:1623`
- `0x437d50` `zTurret_System::EnableTickCallback` -> `src/Battlesport/turret.cpp:1635`
- `0x437d60` `zTurret_Runtime::OnDamage` -> `src/Battlesport/turret.cpp:1410`
- `0x437dc0` `zTurret_System::FreeAllRuntimes` -> `src/Battlesport/turret.cpp:1647`
- `0x437e50` `zTurret_Runtime::FireWeaponCallback` -> `src/Battlesport/turret.cpp:1350`
- `0x437e60` `zClass_Node::SetContextRecursive` -> `src/GameZRecoil/zClass/Class.c:3036`
- `0x437ea0` `zClass_Node::SetDiFlagBit0Recursive` -> `src/GameZRecoil/zClass/Class.c:3062`
- `0x438000` `zClass_Object3D_ModelRefLerpQueue::ClearGlobalState` -> `src/GameZRecoil/zClass/Object3d.c:1234`
- `0x438020` `zClass_Object3D_ModelRefLerpQueue::Add` -> `src/GameZRecoil/zClass/Object3d.c:1247`
- `0x438180` `zClass_Object3D_ModelRefLerpQueue::Reset` -> `src/GameZRecoil/zClass/Object3d.c:1400`
- `0x4381d0` `zClass_Object3D_ModelRefLerpQueue::Update` -> `src/GameZRecoil/zClass/Object3d.c:1310`
- `0x4383e0` `zUtil_SaveGameStateList_Init` -> `src/Battlesport/zsave_game_body.h:11`
- `0x438430` `zUtil_SaveGameState::FreeOwnedResources` -> `src/Battlesport/zsave_game_body.h:77`
- `0x4384e0` `zUtil_SaveGameStateList_AllocAppend` -> `src/Battlesport/zsave_game_body.h:40`
- `0x438540` `Player::SelectModalStateByMasterType. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched` -> `src/Battlesport/player.cpp:3371`
- `0x4385a0` `Player::StartMasterTypeLoopSfxHandle` -> `src/Battlesport/player.cpp:3295`
- `0x4385f0` `Player::StartModalLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched` -> `src/Battlesport/player.cpp:3337`
- `0x438630` `Player::EnsureMasterTypeLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched` -> `src/Battlesport/player.cpp:3318`
- `0x438660` `Player::StopMasterTypeLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched` -> `src/Battlesport/player.cpp:3398`
- `0x438690` `Player::StopModalLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched` -> `src/Battlesport/player.cpp:3356`
- `0x4386c0` `Player::UpdateModalLoopSfx. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; reads accepted g_FrameDeltaTimeSec and original inline helpers PlayerFloatFromBits/PlayerClamp01` -> `src/Battlesport/player.cpp:3413`
- `0x438980` `RecoilVersion::GetString` -> `src/Battlesport/version.cpp:13`
- `0x438b60` `Player::FreeAltWeaponTrailRuntimeStates` -> `src/Battlesport/player.cpp:11794`
- `0x438ba0` `Player::LoadWeaponBanksAndSelectDefaults` -> `src/Battlesport/player.cpp:11817`
- `0x4390d0` `Player::CacheGunHardpointsAndDetachDisplays` -> `src/Battlesport/player.cpp:4600`
- `0x439540` `Player::ApplyAltWeaponSwitch` -> `src/Battlesport/player.cpp:13194`
- `0x439600` `Player::ApplyPrimaryWeaponSwitch` -> `src/Battlesport/player.cpp:13149`
- `0x439990` `Player::ResetDamageStateAndTimedHitStatus` -> `src/Battlesport/player.cpp:13587`
- `0x4399c0` `Player::ResetDamageVisualsAndTimedStatus` -> `src/Battlesport/player.cpp:13605`
- `0x439b20` `HudLowMeterLoopSound::SetLoopActive` -> `src/Battlesport/hud.cpp:4346`
- `0x439b70` `HudLowMeterLoopSound::Disable` -> `src/Battlesport/hud.cpp:4369`
- `0x439ba0` `Player::TickAltGunRuntimeState` -> `src/Battlesport/player.cpp:16624`
- `0x43a400` `Player::ProcessPrimaryGunDispatchTick` -> `src/Battlesport/player.cpp:16551`
- `0x43a4f0` `Player::UpdateGunAndTurretAimNodes` -> `src/Battlesport/player.cpp:15828`
- `0x43a600` `Player::UpdateAltGunAimDirection` -> `src/Battlesport/player.cpp:15885`
- `0x43aa30` `Player::SelectAltGunFirePointAndSlot` -> `src/Battlesport/player.cpp:16118`
- `0x43afd0` `Player::ComposeAimBasisWorldMatrix` -> `src/Battlesport/player.cpp:15989`
- `0x43b1b0` `Player::BuildGunFireTransform` -> `src/Battlesport/player.cpp:15719`
- `0x43b3e0` `Player::UpdateAltGunAimBasisOrigin` -> `src/Battlesport/player.cpp:15785`
- `0x43b500` `Player::ApplyAimPitchToDirection` -> `src/Battlesport/player.cpp:13684`
- `0x43c190` `Player::ProcessAltGunDispatchRequest` -> `src/Battlesport/player.cpp:16471`
- `0x43c2d0` `Player::UpdateContinuousAltGunFireController` -> `src/Battlesport/player.cpp:16272`
- `0x43c330` `Player::EnsureGunAuxEffectActive` -> `src/Battlesport/player.cpp:16304`
- `0x43c430` `Player::AltGunLaunchProjectile` -> `src/Battlesport/player.cpp:16356`
- `0x43c550` `Player::AltGunFireSimpleProjectile` -> `src/Battlesport/player.cpp:16429`
- `0x43c850` `Player::ResetAltGunRuntimeState` -> `src/Battlesport/player.cpp:13310`
- `0x43c950` `Player::RemoveAllDeployedMines` -> `src/Battlesport/player.cpp:13354`
- `0x43c9c0` `Player::FindAltGunFireControllerForWeaponId` -> `src/Battlesport/player.cpp:13030`
- `0x43ca20` `zWeapon_OptCatalog::LoadKillVerbString` -> `src/GameZRecoil/zWeapon/zwep_init.c:1059`
- `0x43ca90` `Player::CheckMissionWeaponAvailability` -> `src/Battlesport/player.cpp:12031`
- `0x43cc70` `Player::WriteMinesZarSection` -> `src/Battlesport/player.cpp:11371`
- `0x43cdf0` `Player::Mines_ZAR_ReadEntryOrReset` -> `src/Battlesport/player.cpp:11321`
- `0x43ce80` `NetUi::VerifyWinsock2OrPromptContinue` -> `src/Battlesport/net_ui_body.h:11`
- `0x43cf40` `Net::FormatIpv4Address` -> `src/Battlesport/game_net_body.h:994`
- `0x43cf90` `WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls` -> `src/Battlesport/wol_dialog_body.h:1045`
- `0x43d060` `WestwoodOnlineUpgradeDialog::AppendStatusTextFmt` -> `src/Battlesport/wol_dialog_body.h:766`
- `0x43d130` `WestwoodOnlineUpgradeApi::CreateInstanceAndLoadConfig` -> `src/Battlesport/wol_api_body.h:404`
- `0x43d280` `WestwoodOnlineUpgradeApi::Shutdown` -> `src/Battlesport/wol_api_body.h:473`
- `0x43d2e0` `WestwoodOnlineUpgradeApi::Init` -> `src/Battlesport/wol_api_body.h:268`
- `0x43d650` `WestwoodOnlineUpgradeDialog::AppendConnectStatusAndRefreshList` -> `src/Battlesport/wol_dialog_body.h:1864`
- `0x43d6a0` `WestwoodOnlineUpgradeDialog::SetAbortAndClose` -> `src/Battlesport/wol_dialog_body.h:1883`
- `0x43d6b0` `WestwoodOnlineUpgradeDialog::EnableQueryControls` -> `src/Battlesport/wol_dialog_body.h:1801`
- `0x43d720` `WestwoodOnlineUpgradeDialog::EnableConnectButton` -> `src/Battlesport/wol_dialog_body.h:1819`
- `0x43d740` `WestwoodOnlineUpgradeDialog::WestwoodOnlineUpgradeDialog` -> `src/Battlesport/wol_dialog_body.h:563`
- `0x43d980` `WestwoodOnlineUpgradeDialog::ScalarDeletingDestructor` -> `src/Battlesport/wol_dialog_body.h:630`
- `0x43d9a0` `WestwoodOnlineUpgradeDialog::Destructor` -> `src/Battlesport/wol_dialog_body.h:618`
- `0x43db20` `WestwoodOnlineUpgradeDialog::DoDataExchange` -> `src/Battlesport/wol_dialog_body.h:646`
- `0x43dcc0` `WestwoodOnlineUpgradeDialog::GetMessageMap` -> `src/Battlesport/wol_dialog_body.h:433`
- `0x43dcd0` `WestwoodOnlineUpgradeDialog::OnInitDialog` -> `src/Battlesport/wol_dialog_body.h:442`
- `0x43dfe0` `WestwoodOnlineUpgradeDialog::OnRefreshListTimer` -> `src/Battlesport/wol_dialog_body.h:887`
- `0x43e040` `WestwoodOnlineUpgradeDialog::OnBrowseRecordListDblClk` -> `src/Battlesport/wol_dialog_body.h:1507`
- `0x43e160` `WestwoodOnlineUpgradeDialog::OnDestroy` -> `src/Battlesport/wol_dialog_body.h:1777`
- `0x43e1c0` `WestwoodOnlineUpgradeDialog::SubmitVisibleSessionRequestsAndStatusText` -> `src/Battlesport/wol_dialog_body.h:1299`
- `0x43e3b0` `WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList` -> `src/Battlesport/wol_dialog_body.h:1830`
- `0x43e450` `WestwoodOnlineUpgradeDialog::BeginDisconnectAndShowProgress` -> `src/Battlesport/wol_dialog_body.h:913`
- `0x43e4b0` `WestwoodOnlineUpgradeDialog::BeginConnect` -> `src/Battlesport/wol_dialog_body.h:939`
- `0x43e520` `WestwoodOnlineUpgradeDialog::CheckAndApplyUpgrade` -> `src/Battlesport/wol_dialog_body.h:969`
- `0x43e550` `WestwoodOnlineUpgradeDialog::QueryStatus` -> `src/Battlesport/wol_dialog_body.h:987`
- `0x43e680` `WestwoodOnlineUpgradeDialog::RequestActiveListMode` -> `src/Battlesport/wol_dialog_body.h:1077`
- `0x43e6a0` `WestwoodOnlineUpgradeDialog::OnRefreshCurrentQuery` -> `src/Battlesport/wol_dialog_body.h:1089`
- `0x43e900` `WestwoodOnlineUpgradeDialog::OnQuerySessionsByName` -> `src/Battlesport/wol_dialog_body.h:1186`
- `0x43ebd0` `WestwoodOnlineUpgradeDialog::ClearStatusList` -> `src/Battlesport/wol_dialog_body.h:832`
- `0x43ec00` `WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequests` -> `src/Battlesport/wol_dialog_body.h:1388`
- `0x43ed10` `WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequestsAndLookupBrowseRecords` -> `src/Battlesport/wol_dialog_body.h:1445`
- `0x43ee40` `WestwoodOnlineUpgradeDialog::RequestListMode0` -> `src/Battlesport/wol_dialog_body.h:1559`
- `0x43ee60` `WestwoodOnlineUpgradeDialog::RequestListMode11` -> `src/Battlesport/wol_dialog_body.h:1572`
- `0x43ee80` `WestwoodOnlineUpgradeDialog::OnSessionModeComboSelChange` -> `src/Battlesport/wol_dialog_body.h:1585`
- `0x43ef10` `WestwoodOnlineUpgradeDialog::SubmitPendingSessionListFromResults` -> `src/Battlesport/wol_dialog_body.h:1626`
- `0x43efc0` `WestwoodOnlineUpgradeDialog::OnQueryControlsChanged` -> `src/Battlesport/wol_dialog_body.h:1672`
- `0x43efd0` `WestwoodOnlineUpgradeDialog::OnMaxPlayersEditChange` -> `src/Battlesport/wol_dialog_body.h:1681`
- `0x43efe0` `WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex` -> `src/Battlesport/wol_dialog_body.h:1893`
- `0x43f450` `WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus` -> `src/Battlesport/wol_dialog_body.h:1690`
- `0x43f4d0` `WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus` -> `src/Battlesport/wol_dialog_body.h:1719`
- `0x43f550` `WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus` -> `src/Battlesport/wol_dialog_body.h:1748`
- `0x43f5d0` `WestwoodOnlineUpgrade::TruncateStringAtFirstSpace` -> `src/Battlesport/wol_dialog_body.h:544`
- `0x43f610` `WestwoodOnlineUpgradeApiEventSink::CreateInstance` -> `src/Battlesport/wol_api_event_sink_body.h:160`
- `0x43f6b0` `WestwoodOnlineUpgradeApiEventSink::OnBootstrapServerList` -> `src/Battlesport/wol_api_event_sink_body.h:262`
- `0x43f830` `WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult` -> `src/Battlesport/wol_api_event_sink_body.h:337`
- `0x43f9d0` `WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved` -> `src/Battlesport/wol_api_event_sink_body.h:1697`
- `0x43fa70` `WestwoodOnlineUpgradeApiEventSink::OnServerError` -> `src/Battlesport/wol_api_event_sink_body.h:401`
- `0x43fa90` `WestwoodOnlineUpgradeApiEventSink::OnApiStatus` -> `src/Battlesport/wol_api_event_sink_body.h:418`
- `0x43fde0` `WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived` -> `src/Battlesport/wol_api_event_sink_body.h:516`
- `0x43fe50` `WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded` -> `src/Battlesport/wol_api_event_sink_body.h:547`
- `0x43ff80` `WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved` -> `src/Battlesport/wol_api_event_sink_body.h:605`
- `0x4401d0` `WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished` -> `src/Battlesport/wol_api_event_sink_body.h:716`
- `0x4402c0` `WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated` -> `src/Battlesport/wol_api_event_sink_body.h:861`
- `0x4404c0` `WestwoodOnlineUpgradeApiEventSink::LaunchSelectedSession` -> `src/Battlesport/wol_api_event_sink_body.h:952`
- `0x4407e0` `WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString1` -> `src/Battlesport/wol_api_event_sink_body.h:1155`
- `0x440a30` `WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString0` -> `src/Battlesport/wol_api_event_sink_body.h:1092`
- `0x440c80` `WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301B` -> `src/Battlesport/wol_api_event_sink_body.h:1219`
- `0x440ce0` `WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C` -> `src/Battlesport/wol_api_event_sink_body.h:1246`
- `0x440d40` `WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301D` -> `src/Battlesport/wol_api_event_sink_body.h:1452`
- `0x440d90` `WestwoodOnlineUpgradeApiEventSink::AppendConnectStatus301E_3021` -> `src/Battlesport/wol_api_event_sink_body.h:1474`
- `0x440e10` `WestwoodOnlineUpgradeApiEventSink::AppendBrowseRecordStatus3022_3025` -> `src/Battlesport/wol_api_event_sink_body.h:1497`
- `0x440ef0` `WestwoodOnlineUpgradeApiEventSink::AppendValueStatus3026` -> `src/Battlesport/wol_api_event_sink_body.h:1533`
- `0x440f40` `WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged` -> `src/Battlesport/wol_api_event_sink_body.h:1557`
- `0x441040` `WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordListReceived` -> `src/Battlesport/wol_api_event_sink_body.h:1610`
- `0x4411c0` `WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt0` -> `src/Battlesport/wol_api_event_sink_body.h:1272`
- `0x441200` `WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt1` -> `src/Battlesport/wol_api_event_sink_body.h:1293`
- `0x441240` `WestwoodOnlineUpgradeApiEventSink::CallbackNoOp0` -> `src/Battlesport/wol_api_event_sink_body.h:1315`
- `0x441250` `WestwoodOnlineUpgradeApiEventSink::CallbackNoOp1` -> `src/Battlesport/wol_api_event_sink_body.h:1327`
- `0x441260` `WestwoodOnlineUpgradeApiEventSink::AppendTimeStatus302A` -> `src/Battlesport/wol_api_event_sink_body.h:1338`
- `0x4412c0` `WestwoodOnlineUpgradeApiEventSink::AppendValueStatus302B_302C` -> `src/Battlesport/wol_api_event_sink_body.h:1363`
- `0x441350` `WestwoodOnlineUpgradeApiEventSink::UpdateSessionResultItemFlags` -> `src/Battlesport/wol_api_event_sink_body.h:1386`
- `0x441480` `WestwoodOnlineUpgradeApiEventSink::OnSessionLaunchResult` -> `src/Battlesport/wol_api_event_sink_body.h:782`
- `0x441600` `WestwoodOnlineUpgradeRefCountAndLock::Init` -> `src/Battlesport/wol_ref_count_and_lock_body.h:4`
- `0x441620` `WestwoodOnlineUpgradeApiEventSink::Release` -> `src/Battlesport/wol_api_event_sink_body.h:234`
- `0x441660` `WestwoodOnlineUpgradeApiEventSink::QueryInterface` -> `src/Battlesport/wol_api_event_sink_body.h:180`
- `0x441680` `WestwoodOnlineUpgradeApiEventSink::Destructor` -> `src/Battlesport/wol_api_event_sink_body.h:252`
- `0x4416f0` `WestwoodOnlineUpgradeDialog::GetSelectedProfilePlayerName` -> `src/Battlesport/wol_dialog_body.h:869`
- `0x441720` `WestwoodOnlineUpgradeDialog::GetSelectedProfileConnectString` -> `src/Battlesport/wol_dialog_body.h:878`
- `0x442180` `WestwoodOnlineUpgradeDialog::SetSelectedProfilePlayerName` -> `src/Battlesport/wol_dialog_body.h:847`
- `0x4421d0` `WestwoodOnlineUpgradeDialog::SetSelectedProfileConnectString` -> `src/Battlesport/wol_dialog_body.h:858`
- `0x4422a0` `WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise` -> `src/Battlesport/wol_download_body.h:226`
- `0x4422f0` `WestwoodOnlineUpgradeDownload::UnadviseAndRelease` -> `src/Battlesport/wol_download_body.h:249`
- `0x4425c0` `WestwoodOnlineUpgradeDownloadEventSink::CreateInstance` -> `src/Battlesport/wol_download_body.h:205`
- `0x442660` `WestwoodOnlineUpgradeDownloadEventSink::OnDownloadFinished` -> `src/Battlesport/wol_download_body.h:71`
- `0x442680` `WestwoodOnlineUpgradeDownloadEventSink::OnDownloadError` -> `src/Battlesport/wol_download_body.h:81`
- `0x4426b0` `WestwoodOnlineUpgradeDownloadEventSink::OnDownloadProgress` -> `src/Battlesport/wol_download_body.h:94`
- `0x442720` `WestwoodOnlineUpgradeDownloadEventSink::OnStateChanged` -> `src/Battlesport/wol_download_body.h:128`
- `0x442770` `WestwoodOnlineUpgradeSharedComAddRef` -> `src/Battlesport/wol_download_body.h:145`
- `0x442790` `WestwoodOnlineUpgradeDownloadEventSink::Release` -> `src/Battlesport/wol_download_body.h:164`
- `0x4427d0` `WestwoodOnlineUpgradeDownloadEventSink::QueryInterface` -> `src/Battlesport/wol_download_body.h:179`
- `0x4427f0` `WestwoodOnlineUpgradeDownloadEventSink::~WestwoodOnlineUpgradeDownloadEventSink` -> `src/Battlesport/wol_download_body.h:195`
- `0x4428a0` `RecoilApp_MfcOleModule::GetMessageMap` -> `src/Battlesport/recoil_app_late_body.h:2961`
- `0x4429d0` `RecoilApp_MfcOleModule::InitInstance` -> `src/Battlesport/recoil_app_late_body.h:2266`
- `0x442a10` `RecoilApp::TakeSkipWaitMessage` -> `src/Battlesport/recoil_app_late_body.h:3125`
- `0x442a30` `RecoilApp::MarkSkipWaitMessage` -> `src/Battlesport/recoil_app_late_body.h:3135`
- `0x442a50` `RecoilApp::EngineInit` -> `src/Battlesport/recoil_app_late_body.h:2321`
- `0x442bc0` `RecoilApp::ShutdownSubsystems` -> `src/Battlesport/recoil_app_late_body.h:2543`
- `0x442c00` `RecoilApp::GetMainWnd` -> `src/Battlesport/recoil_app_late_body.h:2988`
- `0x442c10` `RecoilApp::StartEngineAndQueueStartupState` -> `src/Battlesport/recoil_app_late_body.h:3081`
- `0x442d00` `RecoilApp_MfcOleModule::Run` -> `src/Battlesport/recoil_app_late_body.h:3161`
- `0x443140` `RecoilApp::GetCurrentState` -> `src/Battlesport/recoil_app_late_body.h:2996`
- `0x443160` `RecoilApp::QueueSwitchCurrentState` -> `src/Battlesport/recoil_app_late_body.h:3012`
- `0x443310` `RecoilApp::QueuePushState` -> `src/Battlesport/recoil_app_late_body.h:3037`
- `0x4434b0` `RecoilApp::QueueExitCurrentState` -> `src/Battlesport/recoil_app_late_body.h:3058`
- `0x443650` `RecoilApp::OnIdleOrDispatch` -> `src/Battlesport/recoil_app_late_body.h:3102`
- `0x443690` `RecoilApp_StateQueue::GrowAndCenterChunkBaseList` -> `src/Battlesport/recoil_app_late_body.h:2696`
- `0x443700` `RecoilApp_StateQueueBlock::InitFromCursor` -> `src/Battlesport/recoil_app_late_body.h:2680`
- `0x443730` `CZGameFrame::CreateObject` -> `src/CZGameFrame/CZGameFrame.cpp:141`
- `0x443790` `CZGameFrame::GetBaseRuntimeClass` -> `src/CZGameFrame/CZGameFrame.cpp:131`
- `0x4437a0` `CZGameFrame::GetRuntimeClass` -> `src/CZGameFrame/CZGameFrame.cpp:185`
- `0x4437b0` `CZGameFrame::GetBaseMessageMap` -> `src/CZGameFrame/CZGameFrame.cpp:195`
- `0x4437c0` `CZGameFrame::GetMessageMap` -> `src/CZGameFrame/CZGameFrame.cpp:215`
- `0x4437d0` `CZGameFrame::CZGameFrame` -> `src/CZGameFrame/CZGameFrame.cpp:162`
- `0x443830` `CZGameFrame::~CZGameFrame` -> `src/CZGameFrame/CZGameFrame.cpp:242`
- `0x4438a0` `CZGameFrame::IsWindowValid` -> `src/CZGameFrame/CZGameFrame.cpp:225`
- `0x4438c0` `CZGameFrame::BuildWindowTitle` -> `src/CZGameFrame/CZGameFrame.cpp:251`
- `0x4438f0` `CZGameFrame::OnClose` -> `src/CZGameFrame/CZGameFrame.cpp:288`
- `0x443900` `CZGameFrame::OnPaint` -> `src/CZGameFrame/CZGameFrame.cpp:297`
- `0x443a20` `CZGameFrame::OnSize` -> `src/CZGameFrame/CZGameFrame.cpp:392`
- `0x443a40` `zVid::UpdateCachedClientRectIfUpdateMaskEnabled` -> `src/GameZRecoil/zVideo/zvid_main.c:2323`
- `0x443a50` `CZGameFrame::OnMove` -> `src/CZGameFrame/CZGameFrame.cpp:411`
- `0x443a60` `CZGameFrame::OnCreate` -> `src/CZGameFrame/CZGameFrame.cpp:264`
- `0x443ab0` `CZGameFrame::OnDestroy` -> `src/CZGameFrame/CZGameFrame.cpp:345`
- `0x443ae0` `CZGameFrame::OnActivate` -> `src/CZGameFrame/CZGameFrame.cpp:359`
- `0x443b50` `CZGameFrame::OnAppIdleDispatchMessage` -> `src/CZGameFrame/CZGameFrame.cpp:425`
- `0x443c50` `zClass_cls_di::SetBreakOnFirstCandidate` -> `src/GameZRecoil/zClass/cls_di.c:1388`
- `0x443c60` `zClass_cls_di::SetStopAfterFirstHit` -> `src/GameZRecoil/zClass/cls_di.c:1398`
- `0x443c70` `zClass_cls_di::FindBestPickCandidateBelowPoint` -> `src/GameZRecoil/zClass/cls_di.c:1408`
- `0x443d20` `zClass_cls_di::BuildPickCandidateListBelowPoint` -> `src/GameZRecoil/zClass/cls_di.c:1448`
- `0x443f80` `zClass_cls_di::BuildPickCandidateList` -> `src/GameZRecoil/zClass/cls_di.c:2089`
- `0x444310` `zClass_cls_di::BuildPickCandidatesRecursive` -> `src/GameZRecoil/zClass/cls_di.c:2259`
- `0x4443e0` `zClass_cls_di::BuildPickCandidatesForLight` -> `src/GameZRecoil/zClass/cls_di.c:2305`
- `0x4444b0` `zClass_cls_di::BuildPickCandidatesForPointBatch` -> `src/GameZRecoil/zClass/cls_di.c:1938`
- `0x444890` `zClass_cls_di::BuildPickCandidatesForPoints` -> `src/GameZRecoil/zClass/cls_di.c:1579`
- `0x444c50` `zClass_cls_di::BuildPickCandidatesForPointsRecursive` -> `src/GameZRecoil/zClass/cls_di.c:1809`
- `0x444d10` `zClass_cls_di::BuildPickCandidatesForPointsForLight` -> `src/GameZRecoil/zClass/cls_di.c:1875`
- `0x444de0` `zClass_cls_di::RaycastSelectClosestHitBetweenPoints` -> `src/GameZRecoil/zClass/cls_di.c:3732`
- `0x444e90` `zClass_cls_di::RaycastFindClosest` -> `src/GameZRecoil/zClass/cls_di.c:3796`
- `0x4455f0` `zClass_cls_di::BuildPickCandidatesForSegment` -> `src/GameZRecoil/zClass/cls_di.c:3700`
- `0x445650` `zClass_cls_di::BuildPickCandidatesForSegmentChildFallback` -> `src/GameZRecoil/zClass/cls_di.c:4691`
- `0x445a00` `zClass_cls_di::BuildPickCandidatesForSegmentRecursive` -> `src/GameZRecoil/zClass/cls_di.c:4001`
- `0x445b20` `zClass_cls_di::BuildPickCandidatesForSegmentForCamera` -> `src/GameZRecoil/zClass/cls_di.c:4052`
- `0x445c20` `zClass_cls_di::BuildPickCandidatesForSegmentForLight` -> `src/GameZRecoil/zClass/cls_di.c:4088`
- `0x445d40` `zClass_cls_di::BuildProbeHitBatchesForSegments` -> `src/GameZRecoil/zClass/cls_di.c:4253`
- `0x445f60` `zClass_cls_di::BuildPickCandidatesForSegmentsInGridWindow` -> `src/GameZRecoil/zClass/cls_di.c:4341`
- `0x446440` `zClass_cls_di::BuildPickCandidatesForSegmentsRecursive` -> `src/GameZRecoil/zClass/cls_di.c:4476`
- `0x446880` `zClass_cls_di::BuildPickCandidatesForSegmentsForAnimate` -> `src/GameZRecoil/zClass/cls_di.c:4137`
- `0x446970` `zClass_cls_di::BuildPickCandidatesForSegmentsForLight` -> `src/GameZRecoil/zClass/cls_di.c:4196`
- `0x446a80` `zClass_cls_di::FilterRegionsAgainstSphere` -> `src/GameZRecoil/zClass/cls_di.c:3609`
- `0x446ed0` `BBox::ExpandToCorners` -> `src/GameZRecoil/zClass/cls_di.c:1349`
- `0x446f60` `zClass_cls_di::FilterRegions_TryAppendNode` -> `src/GameZRecoil/zClass/cls_di.c:3508`
- `0x4472c0` `zClass_cls_di::IsPickQueryPointOutsideViewBBoxXZ` -> `src/GameZRecoil/zClass/cls_di.c:2344`
- `0x4473e0` `zClass_cls_di::PickTestBBox2D` -> `src/GameZRecoil/zClass/cls_di.c:2385`
- `0x447540` `zClass_cls_di::FilterPointsBBox` -> `src/GameZRecoil/zClass/cls_di.c:2436`
- `0x4476f0` `zClass_cls_di::FrustumTestAndPick` -> `src/GameZRecoil/zClass/cls_di.c:2486`
- `0x4478c0` `zClass_Class::AllocNodeFromFreeList` -> `src/GameZRecoil/zClass/Class.c:622`
- `0x447980` `zClass_Class::DeleteNodeByType` -> `src/GameZRecoil/zClass/Class.c:674`
- `0x447a70` `zClass_Class::FreeNodeToFreeList` -> `src/GameZRecoil/zClass/Class.c:732`
- `0x447b60` `zClass_Class::TryFreeNode` -> `src/GameZRecoil/zClass/Class.c:781`
- `0x447bc0` `zClass_Class::FindNodeRecursiveByName` -> `src/GameZRecoil/zClass/Class.c:2065`
- `0x447c60` `zClass_Class::gwNodeSetActive` -> `src/GameZRecoil/zClass/Class.c:1360`
- `0x447d20` `zClass_Class::gwNodeSetFlag16` -> `src/GameZRecoil/zClass/Class.c:1408`
- `0x447d70` `zClass_Class::gwNodeSetFlag17` -> `src/GameZRecoil/zClass/Class.c:1433`
- `0x447dc0` `zClass_Class::gwNodeSetName` -> `src/GameZRecoil/zClass/Class.c:1507`
- `0x447e30` `zClass_Class::gwNodeGetName` -> `src/GameZRecoil/zClass/Class.c:1542`
- `0x447e60` `zClass_Class::gwNodeSetDisplayInstance` -> `src/GameZRecoil/zClass/Class.c:1458`
- `0x447f00` `zClass_Class::gwNodeGetUserData` -> `src/GameZRecoil/zClass/Class.c:1558`
- `0x447f30` `zClass_Class::gwNodeSetActionCallback` -> `src/GameZRecoil/zClass/Class.c:1582`
- `0x447fe0` `zClass_Class::gwNodeSetActionCallbackTail` -> `src/GameZRecoil/zClass/Class.c:1634`
- `0x448090` `zClass_Class::gwNodeSetPriority` -> `src/GameZRecoil/zClass/Class.c:1685`
- `0x448100` `zClass_Class::gwNodeSetCellPickable` -> `src/GameZRecoil/zClass/Class.c:1721`
- `0x448140` `zClass_Class::gwNodeGetCellPickable` -> `src/GameZRecoil/zClass/Class.c:1746`
- `0x448180` `zClass_Class::gwNodeGetNodeType` -> `src/GameZRecoil/zClass/Class.c:1766`
- `0x4481b0` `zClass_Class::gwNodeSetRaycastable` -> `src/GameZRecoil/zClass/Class.c:1786`
- `0x4481f0` `zClass_Class::gwNodeGetRaycastable` -> `src/GameZRecoil/zClass/Class.c:1811`
- `0x448230` `zClass_Class::gwNodeSetPickable` -> `src/GameZRecoil/zClass/Class.c:1831`
- `0x448270` `zClass_Class::gwNodeGetPickable` -> `src/GameZRecoil/zClass/Class.c:1856`
- `0x4482b0` `zClass_Class::gwNodeSetHasHitCallback` -> `src/GameZRecoil/zClass/Class.c:1876`
- `0x4482f0` `zClass_Class::gwNodeSetBypassFarClip` -> `src/GameZRecoil/zClass/Class.c:1902`
- `0x448330` `zClass_Class::gwNodeSetNodeType` -> `src/GameZRecoil/zClass/Class.c:1927`
- `0x448360` `zClass_Class::gwNodeClearVariantGate` -> `src/GameZRecoil/zClass/Class.c:1951`
- `0x4483a0` `zClass_Class::gwNodeSetVertexAlphaOverride` -> `src/GameZRecoil/zClass/Class.c:1975`
- `0x4483f0` `zClass_Class::AddChild` -> `src/GameZRecoil/zClass/Class.c:2245`
- `0x4484d0` `zClass_Class::AddChildGeneric` -> `src/GameZRecoil/zClass/Class.c:2449`
- `0x448570` `zClass_Class::RemoveChild` -> `src/GameZRecoil/zClass/Class.c:2337`
- `0x448660` `zClass_Class::RemoveChildGeneric` -> `src/GameZRecoil/zClass/Class.c:2494`
- `0x448760` `zClass_Class::gwNodeGetBBox` -> `src/GameZRecoil/zClass/Class.c:942`
- `0x4487c0` `zClass_Class::gwNodeGetWorldBBoxCorners` -> `src/GameZRecoil/zClass/Class.c:978`
- `0x448920` `zClass_Class::gwNodeGetViewBBoxCorners` -> `src/GameZRecoil/zClass/Class.c:1048`
- `0x448cc0` `zClass_Class::gwNodeUpdate` -> `src/GameZRecoil/zClass/Class.c:805`
- `0x448e90` `zClass_Class::gwNodeRecalcBBox` -> `src/GameZRecoil/zClass/Class.c:1247`
- `0x4491b0` `zClass_Class::gwNodeComputeChildBBox` -> `src/GameZRecoil/zClass/Class.c:1167`
- `0x449420` `zClass_Class::gwNodeUpdateDisplayInstance` -> `src/GameZRecoil/zClass/Class.c:914`
- `0x449480` `gwNode::BuildNodeToAncestorMatrix` -> `src/GameZRecoil/zClass/Class.c:2567`
- `0x4497b0` `gwNode::GetWorldPosition` -> `src/GameZRecoil/zClass/Class.c:2752`
- `0x449850` `gwNode::TransformPoint` -> `src/GameZRecoil/zClass/Class.c:2794`
- `0x4498e0` `gwNode::GetWorldPosAndOrientation` -> `src/GameZRecoil/zClass/Class.c:2830`
- `0x449ab0` `zClass_Class::gwNodeGetRoot` -> `src/GameZRecoil/zClass/Class.c:1999`
- `0x449af0` `zClass_Class::gwNodeGetWorldChild` -> `src/GameZRecoil/zClass/Class.c:2105`
- `0x449b40` `zClass_Class::SetSingleParentFlagRecursive` -> `src/GameZRecoil/zClass/Class.c:2140`
- `0x449ba0` `zClass_Camera::SetViewDistance` -> `src/GameZRecoil/zClass/Camera.c:1243`
- `0x449be0` `zClass_Camera::gwCameraNew` -> `src/GameZRecoil/zClass/Camera.c:443`
- `0x449c90` `zClass_Camera::gwCameraAddChild` -> `src/GameZRecoil/zClass/Camera.c:484`
- `0x449cd0` `zClass_Camera::gwCameraRemoveChild` -> `src/GameZRecoil/zClass/Camera.c:515`
- `0x449d20` `zClass_Camera::gwCameraSetFlagBit0` -> `src/GameZRecoil/zClass/Camera.c:550`
- `0x449da0` `zClass_Camera::SetTargetNode` -> `src/GameZRecoil/zClass/Camera.c:582`
- `0x449db0` `zClass_Camera::SetActiveCamera` -> `src/GameZRecoil/zClass/Camera.c:592`
- `0x449dc0` `zClass_Camera::SetObjectHseTestEnabled` -> `src/GameZRecoil/zClass/Camera.c:604`
- `0x449dd0` `zClass_Camera::gwCameraSetWorld` -> `src/GameZRecoil/zClass/Camera.c:615`
- `0x449e80` `zClass_Camera::gwCameraGetWorld` -> `src/GameZRecoil/zClass/Camera.c:680`
- `0x449e90` `zClass_Camera::gwCameraSetWindow` -> `src/GameZRecoil/zClass/Camera.c:691`
- `0x449ea0` `zClass_Camera::gwCameraSetPosition` -> `src/GameZRecoil/zClass/Camera.c:730`
- `0x449f50` `zClass_Camera::ActivateChildren` -> `src/GameZRecoil/zClass/Camera.c:704`
- `0x449fb0` `zClass_Camera::gwCameraTranslate` -> `src/GameZRecoil/zClass/Camera.c:770`
- `0x44a060` `zClass_Camera::gwCameraGetPosition` -> `src/GameZRecoil/zClass/Camera.c:807`
- `0x44a0f0` `zClass_Camera::gwCameraSetTarget` -> `src/GameZRecoil/zClass/Camera.c:836`
- `0x44a1a0` `zClass_Camera::gwCameraTranslateTarget` -> `src/GameZRecoil/zClass/Camera.c:874`
- `0x44a250` `zClass_Camera::gwCameraGetTarget` -> `src/GameZRecoil/zClass/Camera.c:911`
- `0x44a2f0` `zClass_Camera::gwCameraSetNearFarClip` -> `src/GameZRecoil/zClass/Camera.c:941`
- `0x44a380` `zClass_Camera::gwCameraGetNearFarClip` -> `src/GameZRecoil/zClass/Camera.c:969`
- `0x44a410` `zClass_Camera::gwCameraSetViewport` -> `src/GameZRecoil/zClass/Camera.c:996`
- `0x44a580` `zClass_Camera::gwCameraGetViewport` -> `src/GameZRecoil/zClass/Camera.c:1044`
- `0x44a610` `zClass_Camera::gwCameraSetFOV` -> `src/GameZRecoil/zClass/Camera.c:1101`
- `0x44a760` `zClass_Camera::gwCameraGetFOV` -> `src/GameZRecoil/zClass/Camera.c:1070`
- `0x44a7f0` `zClass_Camera::gwCameraGetClipDistance` -> `src/GameZRecoil/zClass/Camera.c:1143`
- `0x44a870` `zClass_Camera::gwCameraSetClipDistance` -> `src/GameZRecoil/zClass/Camera.c:1168`
- `0x44a910` `zClass_Camera::gwCameraSetHorizon` -> `src/GameZRecoil/zClass/Camera.c:1194`
- `0x44a980` `zClass_Camera::gwCameraSetHorizonXZ` -> `src/GameZRecoil/zClass/Camera.c:1219`
- `0x44a9f0` `zClass_Camera::gwCameraUpdate` -> `src/GameZRecoil/zClass/Camera.c:2204`
- `0x44aa30` `zClass_Camera::UpdateImpl` -> `src/GameZRecoil/zClass/Camera.c:2130`
- `0x44abf0` `zClass_Camera::BuildWorldTransform` -> `src/GameZRecoil/zClass/Camera.c:2046`
- `0x44ada0` `zClass_Camera::RenderTraverse` -> `src/GameZRecoil/zClass/Camera.c:2278`
- `0x44af60` `zClass_Sound::RenderTraverse` -> `src/GameZRecoil/zClass/Sound.c:558`
- `0x44b140` `zClass_Light::RenderTraverse` -> `src/GameZRecoil/zClass/Light.c:863`
- `0x44b300` `zClass_Object3D::RenderTraverse` -> `src/GameZRecoil/zClass/Object3d.c:390`
- `0x44b710` `zClass_Animate::RenderTraverse` -> `src/GameZRecoil/zClass/Animate.c:370`
- `0x44b8c0` `zClass_Lod::RenderTraverse` -> `src/GameZRecoil/zClass/lod_impl_body.h:121`
- `0x44bea0` `zClass_Sequence::RenderTraverse` -> `src/GameZRecoil/zClass/Seq.c:480`
- `0x44bfb0` `zClass_Switch::RenderTraverse` -> `src/GameZRecoil/zClass/Switch.c:62`
- `0x44c0e0` `zClass_Class::gwNodeRenderDispatch` -> `src/GameZRecoil/zClass/Class.c:2902`
- `0x44c1b0` `zClass_Camera::FastAngleXZ` -> `src/GameZRecoil/zClass/Camera.c:1259`
- `0x44c230` `zClass_Camera::FindConvexHullXZ` -> `src/GameZRecoil/zClass/Camera.c:1291`
- `0x44c3c0` `zClass_Camera::BuildFrustumGridTiles` -> `src/GameZRecoil/zClass/Camera.c:1367`
- `0x44c8e0` `zClass_Camera::BuildFrustumGridTilesFromParams` -> `src/GameZRecoil/zClass/Camera.c:1520`
- `0x44ce70` `zClass_Camera::RenderFrustumGridTiles` -> `src/GameZRecoil/zClass/Camera.c:1710`
- `0x44d200` `zClass_Camera::RenderOverlayNodes` -> `src/GameZRecoil/zClass/Camera.c:1866`
- `0x44d240` `zClass_Camera::RenderWorld` -> `src/GameZRecoil/zClass/Camera.c:1881`
- `0x44d260` `zClass_Camera::gwCameraSetVariantTagOverride` -> `src/GameZRecoil/zClass/Camera.c:1900`
- `0x44d320` `zClass_Camera::SyncViewContextPositions` -> `src/GameZRecoil/zClass/Camera.c:2232`
- `0x44d3a0` `zClass_Camera::RenderScene` -> `src/GameZRecoil/zClass/Camera.c:1936`
- `0x44d990` `zClass_Node::PropagateTransformDirtyRecursive` -> `src/GameZRecoil/zClass/Object3d.c:1443`
- `0x44d9e0` `zClass_Object3D::PropagateTransformDirty` -> `src/GameZRecoil/zClass/Object3d.c:614`
- `0x44daa0` `zClass_Object3D::gwObject3DInit` -> `src/GameZRecoil/zClass/Object3d.c:488`
- `0x44db00` `zClass_Object3D::DeleteNode` -> `src/GameZRecoil/zClass/Object3d.c:604`
- `0x44db10` `zClass_Object3D::gwObject3DAddChild` -> `src/GameZRecoil/zClass/Object3d.c:515`
- `0x44db60` `zClass_Object3D::RemoveChild` -> `src/GameZRecoil/zClass/Object3d.c:560`
- `0x44dbb0` `zClass_Object3D::gwObject3DSetVisibleFlag` -> `src/GameZRecoil/zClass/Object3d.c:674`
- `0x44dc30` `zClass_Object3D::gwObject3DSetColorAlpha` -> `src/GameZRecoil/zClass/Object3d.c:702`
- `0x44dd90` `zClass_Object3D::gwObject3DSetAlphaScale` -> `src/GameZRecoil/zClass/Object3d.c:734`
- `0x44de10` `zClass_Object3D::gwObject3DGetAlphaScale` -> `src/GameZRecoil/zClass/Object3d.c:782`
- `0x44de80` `zClass_Object3D::gwObject3DSetLitFlag` -> `src/GameZRecoil/zClass/Object3d.c:829`
- `0x44df00` `zClass_Object3D::gwObject3DSetScale` -> `src/GameZRecoil/zClass/Object3d.c:909`
- `0x44dfd0` `zClass_Object3D::gwObject3DGetScale` -> `src/GameZRecoil/zClass/Object3d.c:882`
- `0x44e030` `zClass_Object3D::gwObject3DSetRotation` -> `src/GameZRecoil/zClass/Object3d.c:975`
- `0x44e110` `zClass_Object3D::gwObject3DGetRotation` -> `src/GameZRecoil/zClass/Object3d.c:948`
- `0x44e170` `zClass_Object3D::gwObject3DTranslateRotation` -> `src/GameZRecoil/zClass/Object3d.c:1017`
- `0x44e270` `zClass_Object3D::gwObject3DGetPosition` -> `src/GameZRecoil/zClass/Object3d.c:1059`
- `0x44e300` `zClass_Object3D::gwObject3DSetPosition` -> `src/GameZRecoil/zClass/Object3d.c:1088`
- `0x44e3d0` `zClass_Object3D::gwObject3DTranslatePosition` -> `src/GameZRecoil/zClass/Object3d.c:1127`
- `0x44e4f0` `zClass_Object3D::gwObject3DSetMatrix` -> `src/GameZRecoil/zClass/Object3d.c:1187`
- `0x44e5b0` `zClass_Object3D::gwObject3DGetMatrixPtr` -> `src/GameZRecoil/zClass/Object3d.c:1166`
- `0x44e630` `zClass_TypeList::AllocLink` -> `src/GameZRecoil/zClass/List.c:628`
- `0x44e690` `zClass_TypeList::FreeLink` -> `src/GameZRecoil/zClass/List.c:661`
- `0x44e6d0` `zClass_TypeList::FreeAll` -> `src/GameZRecoil/zClass/List.c:683`
- `0x44e700` `zClass_TypeList::ProcessPendingRemovals` -> `src/GameZRecoil/zClass/List.c:697`
- `0x44e920` `zClass::ProcessDeferredWork` -> `src/GameZRecoil/zClass/List.c:1091`
- `0x44ea70` `zClass_TypeList::UpdateAllBuckets` -> `src/GameZRecoil/zClass/List.c:900`
- `0x44eaa0` `zClass_TypeList::UpdateBucket` -> `src/GameZRecoil/zClass/List.c:915`
- `0x44eb00` `gwNode::UpdateSubtree` -> `src/GameZRecoil/zClass/List.c:1014`
- `0x44eb50` `gwNode::UpdateTree` -> `src/GameZRecoil/zClass/List.c:1035`
- `0x44eba0` `zClass_TypeList::UpdateQueuedTrees` -> `src/GameZRecoil/zClass/List.c:938`
- `0x44ebe0` `zClass_TypeList::UpdateSequences` -> `src/GameZRecoil/zClass/List.c:960`
- `0x44ec30` `zClass_TypeList::UpdateAnimations` -> `src/GameZRecoil/zClass/List.c:986`
- `0x44ec80` `zClass_Class::gwNodeUpdateAll` -> `src/GameZRecoil/zClass/List.c:1245`
- `0x44ec90` `zClass_TypeList::CountNodes` -> `src/GameZRecoil/zClass/List.c:751`
- `0x44ecb0` `zClass_TypeList::PrintBucket` -> `src/GameZRecoil/zClass/List.c:764`
- `0x44ecf0` `zClass::FindByTypeAndName` -> `src/GameZRecoil/zClass/List.c:1132`
- `0x44ed50` `zClass_TypeList::GetBucketHead` -> `src/GameZRecoil/zClass/List.c:781`
- `0x44ed60` `zClass_NodeList::Insert` -> `src/GameZRecoil/zClass/List.c:1056`
- `0x44ed90` `zClass_TypeList::Insert` -> `src/GameZRecoil/zClass/List.c:820`
- `0x44ee10` `zClass_TypeList::InsertChildNodes` -> `src/GameZRecoil/zClass/List.c:859`
- `0x44eea0` `zClass_NodeList::ProcessPendingFrees` -> `src/GameZRecoil/zClass/List.c:1074`
- `0x44eed0` `zClass_TypeList::MarkPendingRemoval` -> `src/GameZRecoil/zClass/List.c:789`
- `0x44f000` `zClass_List::DeleteNodeFromLists` -> `src/GameZRecoil/zClass/List.c:193`
- `0x44f120` `zClass_List::DeleteAllOfType` -> `src/GameZRecoil/zClass/List.c:571`
- `0x44f1d0` `zClass_List::gwListDeleteANode` -> `src/GameZRecoil/zClass/List.c:312`
- `0x44f690` `zClass_List::IterateBucketFiltered` -> `src/GameZRecoil/zClass/List.c:283`
- `0x44f6f0` `zClass::FindNextByTypePrefix` -> `src/GameZRecoil/zClass/List.c:1165`
- `0x44f720` `zClass::FindNextByTypePrefix_Predicate` -> `src/GameZRecoil/zClass/List.c:1153`
- `0x44f740` `zClass_Class::gwNodeFindNextByName` -> `src/GameZRecoil/zClass/List.c:1267`
- `0x44f750` `zClass_Class::gwNodeFindNextByName_Predicate` -> `src/GameZRecoil/zClass/List.c:1255`
- `0x44f7a0` `zClass_Window::gwWindowNew` -> `src/GameZRecoil/zClass/Window.c:132`
- `0x44f870` `zClass::RemoveChildChecked` -> `src/GameZRecoil/zClass/List.c:1209`
- `0x44f8b0` `zClass_Window::gwWindowSetResolution` -> `src/GameZRecoil/zClass/Window.c:190`
- `0x44f930` `zClass_Window::gwWindowGetResolution` -> `src/GameZRecoil/zClass/Window.c:216`
- `0x44f9c0` `zClass_Window::gwWindowSetSize` -> `src/GameZRecoil/zClass/Window.c:243`
- `0x44fa40` `zClass_Window::gwWindowGetSize` -> `src/GameZRecoil/zClass/Window.c:269`
- `0x44fad0` `zClass_Window::gwWindowSetBuffer` -> `src/GameZRecoil/zClass/Window.c:297`
- `0x44fb40` `zClass_Window::gwWindowSetClearPolygon` -> `src/GameZRecoil/zClass/Window.c:324`
- `0x44fbd0` `zClass_Window::gwWindowAddClearPolygonVertex` -> `src/GameZRecoil/zClass/Window.c:356`
- `0x44fcf0` `zClass_Window::gwWindowCloseClearPolygon` -> `src/GameZRecoil/zClass/Window.c:413`
- `0x44fdd0` `zClass_Display::gwDisplayInit` -> `src/GameZRecoil/zClass/Display.c:135`
- `0x44fe50` `zClass_Display::RemoveChild` -> `src/GameZRecoil/zClass/Display.c:178`
- `0x44fe90` `zClass_Display::gwDisplaySetSize` -> `src/GameZRecoil/zClass/Display.c:216`
- `0x44ff10` `zClass_Display::gwDisplaySetPosition` -> `src/GameZRecoil/zClass/Display.c:242`
- `0x44ff90` `zClass_Display::gwDisplaySetBackgroundColor` -> `src/GameZRecoil/zClass/Display.c:268`
- `0x450030` `zClass_World::QueueAreaUpdate` -> `src/GameZRecoil/zClass/cls_world.c:832`
- `0x4500b0` `zClass_World::RebuildAreaBounds` -> `src/GameZRecoil/zClass/cls_world.c:867`
- `0x4501c0` `zClass_World::gwWorldNew` -> `src/GameZRecoil/zClass/cls_world.c:314`
- `0x450240` `zClass_World::DeleteNode` -> `src/GameZRecoil/zClass/cls_world.c:801`
- `0x4502b0` `zClass_World::InitVirtualAreaPartitions` -> `src/GameZRecoil/zClass/cls_world.c:678`
- `0x450510` `zClass_World::SetVirtualPartition` -> `src/GameZRecoil/zClass/cls_world.c:732`
- `0x450530` `zClass_World::ApplyPendingFogSettings` -> `src/GameZRecoil/zClass/cls_world.c:937`
- `0x450650` `zClass_World::WorldToGridCoordsClampedEx` -> `src/GameZRecoil/zClass/cls_world.c:1087`
- `0x450790` `zClass_World::WorldToGridCoordsClamped` -> `src/GameZRecoil/zClass/cls_world.c:1138`
- `0x450840` `zClass_World::WorldRectToGridIndex` -> `src/GameZRecoil/zClass/cls_world.c:1012`
- `0x450a00` `zClass_World::GetAreaPartitionAtGrid` -> `src/GameZRecoil/zClass/cls_world.c:1179`
- `0x450a70` `zClass_World::EnsureGridCellDisplayPosition` -> `src/GameZRecoil/zClass/cls_world.c:1218`
- `0x450ae0` `zClass_World::SetPendingFogState` -> `src/GameZRecoil/zClass/cls_world.c:350`
- `0x450af0` `zClass_World::SetPendingFogColorRgb01` -> `src/GameZRecoil/zClass/cls_world.c:366`
- `0x450b20` `zClass_World::SetPendingFogAltitudeRange` -> `src/GameZRecoil/zClass/cls_world.c:386`
- `0x450b40` `zClass_World::SetPendingFogRange` -> `src/GameZRecoil/zClass/cls_world.c:404`
- `0x450b60` `zClass_World::SetPendingFogDensity` -> `src/GameZRecoil/zClass/cls_world.c:502`
- `0x450b80` `zClass_World::GetPendingFogDensity` -> `src/GameZRecoil/zClass/cls_world.c:422`
- `0x450b90` `zClass_World::GetPendingFogState` -> `src/GameZRecoil/zClass/cls_world.c:436`
- `0x450ba0` `zClass_World::GetPendingFogColorRgb01` -> `src/GameZRecoil/zClass/cls_world.c:450`
- `0x450bc0` `zClass_World::GetPendingFogRange` -> `src/GameZRecoil/zClass/cls_world.c:468`
- `0x450be0` `zClass_World::GetPendingFogAltitudeRange` -> `src/GameZRecoil/zClass/cls_world.c:485`
- `0x450c00` `zClass_World::gwWorldSetOrigin` -> `src/GameZRecoil/zClass/cls_world.c:519`
- `0x450c30` `zClass_World::gwWorldSetSize` -> `src/GameZRecoil/zClass/cls_world.c:539`
- `0x450c60` `zClass_World::gwWorldSetVirtualAreaPartition` -> `src/GameZRecoil/zClass/cls_world.c:601`
- `0x450e40` `zClass_World::FreeVirtualAreaPartitions` -> `src/GameZRecoil/zClass/cls_world.c:750`
- `0x450f00` `zClass_World::gwWorldSetPartitionInclusionTolerance` -> `src/GameZRecoil/zClass/cls_world.c:557`
- `0x450f20` `zClass_World::gwWorldSetMaxDecFeatures` -> `src/GameZRecoil/zClass/cls_world.c:574`
- `0x450f60` `zClass_World::AddChildToGridCell` -> `src/GameZRecoil/zClass/cls_world.c:1334`
- `0x4510e0` `zClass_World::AddChildAtGrid` -> `src/GameZRecoil/zClass/cls_world.c:1262`
- `0x451240` `zClass_World::RemoveChildAtGrid` -> `src/GameZRecoil/zClass/cls_world.c:1426`
- `0x451360` `zClass_World::AddLight` -> `src/GameZRecoil/zClass/cls_world.c:1502`
- `0x451410` `zClass_World::RemoveLight` -> `src/GameZRecoil/zClass/cls_world.c:1540`
- `0x451540` `zClass_World::InitLightPointInPolygonXZ` -> `src/GameZRecoil/zClass/cls_world.c:1609`
- `0x451560` `zClass_World::UpdateAllLights` -> `src/GameZRecoil/zClass/cls_world.c:1625`
- `0x451590` `zClass_World::AddSound` -> `src/GameZRecoil/zClass/cls_world.c:1642`
- `0x451640` `zClass_World::RemoveSound` -> `src/GameZRecoil/zClass/cls_world.c:1680`
- `0x451770` `zClass_World::UpdateAllSounds` -> `src/GameZRecoil/zClass/cls_world.c:1749`
- `0x4517a0` `zClass_World::WriteSettingsSection` -> `src/GameZRecoil/zClass/cls_world.c:201`
- `0x451840` `zClass_World::ReadSettingsSection` -> `src/GameZRecoil/zClass/cls_world.c:259`
- `0x4518b0` `zClass::SetNodeArraySize` -> `src/GameZRecoil/zClass/cls_util.c:320`
- `0x4518e0` `zClass::Shutdown` -> `src/GameZRecoil/zClass/cls_util.c:429`
- `0x4518f0` `zClass::IsInitialized` -> `src/GameZRecoil/zClass/cls_util.c:340`
- `0x451900` `zClass::Init` -> `src/GameZRecoil/zClass/cls_util.c:349`
- `0x451a00` `zClass::ShutdownCore` -> `src/GameZRecoil/zClass/cls_util.c:404`
- `0x451a60` `zClass_Util::DestroyNodeRecursive` -> `src/GameZRecoil/zClass/cls_util.c:442`
- `0x451b20` `zClass_cls_util::CopyNodeDisplayInstance` -> `src/GameZRecoil/zClass/cls_util.c:509`
- `0x451bd0` `zClass_cls_util::CopyNodeBaseData` -> `src/GameZRecoil/zClass/cls_util.c:579`
- `0x451f70` `zClass_cls_util::CopyCameraNode` -> `src/GameZRecoil/zClass/cls_util.c:845`
- `0x4520c0` `zClass_cls_util::CopyLightNode_Unimplemented` -> `src/GameZRecoil/zClass/cls_util.c:1029`
- `0x4520e0` `zClass_cls_util::CopySoundNode_Unimplemented` -> `src/GameZRecoil/zClass/cls_util.c:1046`
- `0x452100` `zClass_cls_util::CopyObject3DNode` -> `src/GameZRecoil/zClass/cls_util.c:937`
- `0x452230` `zClass_cls_util::CopyAnimateNode_Unimplemented` -> `src/GameZRecoil/zClass/cls_util.c:1063`
- `0x452250` `zClass_cls_util::CopyLodNode` -> `src/GameZRecoil/zClass/cls_util.c:1080`
- `0x4523c0` `zClass_cls_util::CopySequenceNode_Unimplemented` -> `src/GameZRecoil/zClass/cls_util.c:1149`
- `0x4523e0` `zClass_cls_util::CopySwitchNode_Stub` -> `src/GameZRecoil/zClass/cls_util.c:1166`
- `0x452400` `zClass_cls_util::CopyNodeDispatch` -> `src/GameZRecoil/zClass/cls_util.c:1181`
- `0x452500` `zClass_cls_util::CopyNodeWithCloneOptions` -> `src/GameZRecoil/zClass/cls_util.c:1245`
- `0x452560` `zClass_cls_util::CopyNode` -> `src/GameZRecoil/zClass/cls_util.c:1276`
- `0x4525d0` `BBox::MinMaxToBoundingSphere` -> `src/GameZRecoil/zClass/cls_util.c:1313`
- `0x452650` `BBox::CornersToBoundingSphere` -> `src/GameZRecoil/zClass/cls_util.c:272`
- `0x452770` `zClass_Class::FindSubNodeByName` -> `src/GameZRecoil/zClass/Class.c:2032`
- `0x4527f0` `zClass_Node::HasRenderableDiPredicate` -> `src/GameZRecoil/zClass/Object3d.c:1424`
- `0x452810` `zClass::AnyNodeMatchesPredicateRecursive` -> `src/GameZRecoil/zClass/List.c:1184`
- `0x452860` `zClass_Node::SetMaterialFlagBit9ForFlagBit0EntriesRecursive` -> `src/GameZRecoil/zClass/Class.c:3097`
- `0x4528a0` `zClass_Node::LoadFlagBit8MaterialImagesAndTexturePack` -> `src/GameZRecoil/zClass/Class.c:3142`
- `0x4528b0` `zClass_Node::InvalidateFlagBit8MaterialImagesRecursive` -> `src/GameZRecoil/zClass/Class.c:3123`
- `0x4528e0` `zClass_Node::AssignInt32ToDiRecursive` -> `src/GameZRecoil/zClass/Class.c:3159`
- `0x452920` `zClass_Class::AddChildValidated` -> `src/GameZRecoil/zClass/Class.c:2173`
- `0x452970` `zClass_Class::RemoveChildValidated` -> `src/GameZRecoil/zClass/Class.c:2200`
- `0x4529c0` `zClass_Sound::gwSoundNew` -> `src/GameZRecoil/zClass/Sound.c:103`
- `0x452ab0` `zClass_Sound::DeleteNode` -> `src/GameZRecoil/zClass/Sound.c:160`
- `0x452b80` `zClass_Sound::RemoveChild` -> `src/GameZRecoil/zClass/Sound.c:220`
- `0x452bc0` `zClass_Sound::SetSampleSetByName` -> `src/GameZRecoil/zClass/Sound.c:257`
- `0x452c60` `zClass_Sound::gwSoundSetActive` -> `src/GameZRecoil/zClass/Sound.c:311`
- `0x452d00` `zClass_Sound::gwSoundSetPosition` -> `src/GameZRecoil/zClass/Sound.c:363`
- `0x452d60` `zClass_Sound::gwSoundGetPosition` -> `src/GameZRecoil/zClass/Sound.c:405`
- `0x452dc0` `zClass_Sound::UpdatePlayback` -> `src/GameZRecoil/zClass/Sound.c:445`
- `0x452ec0` `zClass_Sound::ComputeWorldTransform` -> `src/GameZRecoil/zClass/Sound.c:520`
- `0x452fd0` `zClass_Light::gwLightNew` -> `src/GameZRecoil/zClass/Light.c:275`
- `0x453110` `zClass_Light::DeleteNode` -> `src/GameZRecoil/zClass/Light.c:349`
- `0x4531c0` `zClass_Light::RemoveChild` -> `src/GameZRecoil/zClass/Light.c:397`
- `0x453200` `zClass_Light::gwLightSetIntensity` -> `src/GameZRecoil/zClass/Light.c:433`
- `0x453250` `zClass_Light::gwLightSetFalloff` -> `src/GameZRecoil/zClass/Light.c:457`
- `0x4532a0` `zClass_Light::gwLightSetConeAngle` -> `src/GameZRecoil/zClass/Light.c:481`
- `0x4532f0` `zClass_Light::gwLightSetPointMode` -> `src/GameZRecoil/zClass/Light.c:509`
- `0x453350` `zClass_Light::gwLightSetDirectionalMode` -> `src/GameZRecoil/zClass/Light.c:531`
- `0x4533b0` `zClass_Light::gwLightSetParam` -> `src/GameZRecoil/zClass/Light.c:553`
- `0x453400` `zClass_Light::gwLightSetRange` -> `src/GameZRecoil/zClass/Light.c:577`
- `0x453500` `zClass_Light::gwLightGetRange` -> `src/GameZRecoil/zClass/Light.c:616`
- `0x453560` `zClass_Light::gwLightSetPosition` -> `src/GameZRecoil/zClass/Light.c:641`
- `0x4535c0` `zClass_Light::gwLightSetRotation` -> `src/GameZRecoil/zClass/Light.c:671`
- `0x453620` `zClass_Light::ComputeWorldTransform` -> `src/GameZRecoil/zClass/Light.c:701`
- `0x453880` `zClass_Light::gwLightUpdate` -> `src/GameZRecoil/zClass/Light.c:747`
- `0x453a40` `zClass_Light::gwLightGetSpecularColor` -> `src/GameZRecoil/zClass/Light.c:808`
- `0x453aa0` `zClass_Light::gwLightSetSpecularColor` -> `src/GameZRecoil/zClass/Light.c:834`
- `0x453b10` `zClass_Animate::DeleteNode` -> `src/GameZRecoil/zClass/Animate.c:302`
- `0x453b40` `zClass_Animate::AddChild` -> `src/GameZRecoil/zClass/Animate.c:266`
- `0x453b80` `zClass_Animate::RemoveChild` -> `src/GameZRecoil/zClass/Animate.c:324`
- `0x453bd0` `zClass_Animate::UpdateNode` -> `src/GameZRecoil/zClass/Animate.c:206`
- `0x453c90` `zClass_Animate::AdvanceTime` -> `src/GameZRecoil/zClass/Animate.c:127`
- `0x453d20` `zClass_Animate::SampleTransform` -> `src/GameZRecoil/zClass/Animate.c:162`
- `0x453ee0` `zClass_Sequence::gwSequenceNew` -> `src/GameZRecoil/zClass/Seq.c:60`
- `0x453f40` `zClass_Sequence::gwSequenceAddChild` -> `src/GameZRecoil/zClass/Seq.c:94`
- `0x454000` `zClass_Sequence::RemoveChild` -> `src/GameZRecoil/zClass/Seq.c:330`
- `0x4540c0` `zClass_Sequence::SetActive` -> `src/GameZRecoil/zClass/Seq.c:166`
- `0x454100` `zClass_Sequence::SetRepeat` -> `src/GameZRecoil/zClass/Seq.c:206`
- `0x454140` `zClass_Sequence::SetLoop` -> `src/GameZRecoil/zClass/Seq.c:247`
- `0x454180` `zClass_Sequence::SetPause` -> `src/GameZRecoil/zClass/Seq.c:288`
- `0x4541c0` `zClass_Sequence::Update` -> `src/GameZRecoil/zClass/Seq.c:400`
- `0x4542a0` `zClass_Lod::gwLodNew` -> `src/GameZRecoil/zClass/lod_impl_body.h:302`
- `0x454310` `zClass_Lod::gwLodAddChild` -> `src/GameZRecoil/zClass/lod_impl_body.h:327`
- `0x454320` `zClass_Lod::RemoveChild` -> `src/GameZRecoil/zClass/lod_impl_body.h:345`
- `0x454330` `zClass_Lod::SetComputeOwnDistance` -> `src/GameZRecoil/zClass/lod_impl_body.h:364`
- `0x454340` `zClass_Lod::SetTargetNodeAndRange` -> `src/GameZRecoil/zClass/lod_impl_body.h:380`
- `0x454360` `zClass::ResetCurrentZbdPath` -> `src/GameZRecoil/zClass/cls_util.c:395`
- `0x454370` `GameZ_ZBD::NodePtrToIndex` -> `src/GameZRecoil/zClass/cls_zbd.c:633`
- `0x4543a0` `zClass::NodePtrToValidatedIndex` -> `src/GameZRecoil/zClass/cls_zbd.c:617`
- `0x4543d0` `GameZ_ZBD::NodeIndexToPtr` -> `src/GameZRecoil/zClass/cls_zbd.c:645`
- `0x4543f0` `GameZ_ZBD::WriteNodeRefListIndices` -> `src/GameZRecoil/zClass/cls_zbd.c:658`
- `0x4544b0` `GameZ_ZBD::WriteSingleNodeClassData` -> `src/GameZRecoil/zClass/cls_zbd.c:711`
- `0x454890` `GameZ_ZBD::WriteNodeTable` -> `src/GameZRecoil/zClass/cls_zbd.c:965`
- `0x454a50` `GameZ::WriteZBDFile` -> `src/GameZRecoil/zClass/cls_zbd.c:377`
- `0x454bf0` `GameZ_ZBD::ReadNodeRefListIndices` -> `src/GameZRecoil/zClass/cls_zbd.c:1062`
- `0x454c60` `GameZ_ZBD::ReadSingleNodeClassData` -> `src/GameZRecoil/zClass/cls_zbd.c:1101`
- `0x455350` `GameZ_ZBD::ReadNodeTable` -> `src/GameZRecoil/zClass/cls_zbd.c:1518`
- `0x455520` `GameZ::ReadZBDFile` -> `src/GameZRecoil/zClass/cls_zbd.c:461`
- `0x4556a0` `GameZ::OpenAndReadZBDHeader` -> `src/GameZRecoil/zClass/cls_zbd.c:567`
- `0x455730` `GameZ_ZBD::ReloadDisplayInstancesFromCurrentPath_Local` -> `src/GameZRecoil/zClass/cls_zbd.c:1607`
- `0x4557a0` `GameZ_ZBD::ReloadDisplayInstancesRecursive_Local` -> `src/GameZRecoil/zClass/cls_zbd.c:1641`
- `0x455ea0` `zDEClient_QSand::DestroyFeature` -> `src/GameZRecoil/zDEClient/zdec_qsand.cpp:6`
- `0x455ed0` `zDEClient::CopyQSandEventTemplateDefaults` -> `src/GameZRecoil/zDEClient/zdec_qsand.cpp:31`
- `0x455ef0` `zDEClient_QSand::InstanceEventMaybeRelay` -> `src/GameZRecoil/zDEClient/zdec_qsand.cpp:48`
- `0x456010` `zDEClient_QSand::InitFeatureFromEventTemplate` -> `src/GameZRecoil/zDEClient/zdec_qsand.cpp:119`
- `0x4563d0` `zDEClient_QSand::CreateFeatureStructFromEventTemplate` -> `src/GameZRecoil/zDEClient/zdec_qsand.cpp:271`
- `0x456450` `zDEClient_QSand::Build` -> `src/GameZRecoil/zDEClient/zdec_qsand.cpp:308`
- `0x4564b0` `zDEClient_QSand::CreateFeature` -> `src/GameZRecoil/zDEClient/zdec_qsand.cpp:346`
- `0x456ad0` `zDEClient_Crater::DestroyFeature` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:6`
- `0x456b00` `zDEClient_Crater::InitEventTemplateDefaults` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:31`
- `0x456b20` `zDEClient_Crater::InstanceEvent` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:48`
- `0x456c50` `zDEClient_Crater::InstanceEventMaybeRelay` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:126`
- `0x456c80` `zDEClient_Crater::InitFeatureFromEventTemplate` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:149`
- `0x457040` `zDEClient_Crater::CreateFeatureStructFromEventTemplate` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:300`
- `0x4570e0` `zDEClient_Crater::Build` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:344`
- `0x457140` `zDEClient_Crater::CreateFeature` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:380`
- `0x4575f0` `zDEClient::SubmitFeatureGeometry` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:531`
- `0x457660` `zDEClient::InitFeatureEntryListAndMapTree` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:576`
- `0x4576e0` `zDEClient_MapTreeState::Destroy` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:620`
- `0x457840` `zDEClient::AppendFeatureEntry` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:723`
- `0x457ae0` `zDEClient::ClearFeatureEntriesAndMapTree` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:789`
- `0x457cc0` `zDEClient_MapTreeState::InitState` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:933`
- `0x457d90` `zDEClient_MapTreeState::FindOrInsertKey` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:973`
- `0x457e80` `zDEClient_MapTreeState::EraseRange` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1059`
- `0x457fe0` `zDEClient_MapTreeState::EraseAndAdvance` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1091`
- `0x458510` `zDEClient_MapTreeState::DestroySubtree` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1151`
- `0x4585a0` `zDEClient_MapTreeState::InsertAt` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1170`
- `0x4588c0` `zDEClient_MapTreeState::IterNextNodeRef` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1268`
- `0x458970` `zDEClient_MapTreeState::IterPrevNodeRef` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1299`
- `0x458aa0` `zDEClient::SetCameraNode` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1373`
- `0x458ac0` `zDEClient::GetFeatureGridCell` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1388`
- `0x458ae0` `zDEClient::GetCameraNode` -> `src/GameZRecoil/zDEClient/zdec_crater.cpp:1407`
- `0x458b50` `zEffect::TickResetDelayOnTimer` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:52`
- `0x458bb0` `zEffect::TickResetDelayOnHit` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:83`
- `0x4622f0` `zError::EmitDebugBuffer` -> `src/GameZRecoil/zError/zerr_old.c:25`
- `0x462310` `RecoilError::InitOutputContext` -> `src/GameZRecoil/zError/zerr_old.c:10`
- `0x462330` `zFMV_Playback::Constructor` -> `src/GameZRecoil/zFMV/fmv.h:47`
- `0x462330` `zFMV_Playback::Constructor` -> `src/GameZRecoil/zFMV/fmv_main.cpp:6`
- `0x462360` `zFMV_Playback::Destructor` -> `src/GameZRecoil/zFMV/fmv_main.cpp:19`
- `0x462370` `zFMV_Playback::OpenAndPlay` -> `src/GameZRecoil/zFMV/fmv_main.cpp:27`
- `0x4624f0` `zFMV_Playback::StopAndClose` -> `src/GameZRecoil/zFMV/fmv_main.cpp:140`
- `0x462540` `zFMV_Playback::SetDestRect` -> `src/GameZRecoil/zFMV/fmv_main.cpp:166`
- `0x462570` `zFMV_Playback::ReportMciError` -> `src/GameZRecoil/zFMV/fmv_main.cpp:179`
- `0x4625e0` `zFMV_Script::Init` -> `src/GameZRecoil/zFMV/fmv_script.cpp:364`
- `0x462630` `zFMV_Script::Cleanup` -> `src/GameZRecoil/zFMV/fmv_script.cpp:390`
- `0x462660` `zFMV_Script::Reset` -> `src/GameZRecoil/zFMV/fmv_script.cpp:403`
- `0x4626b0` `zFMV_Script::LoadActionsFromZrd` -> `src/GameZRecoil/zFMV/fmv_script.cpp:432`
- `0x462e30` `zFMV_Action::RunBlockingImmediate` -> `src/GameZRecoil/zFMV/fmv_script.cpp:610`
- `0x462e90` `zFMV_ActionPlaySound::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:621`
- `0x462ed0` `zFMV_ActionWait::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:637`
- `0x462ee0` `zFMV_ActionWait::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:647`
- `0x462f00` `zFMV_Action::FlipSurfaces` -> `src/GameZRecoil/zFMV/fmv_script.cpp:666`
- `0x462f10` `zFMV_Script::AppendAction` -> `src/GameZRecoil/zFMV/fmv_script.cpp:679`
- `0x462f50` `zFMV_Script::RunBlocking` -> `src/GameZRecoil/zFMV/fmv_script.cpp:703`
- `0x462f90` `zFMV_Script::BeginCurrentAction` -> `src/GameZRecoil/zFMV/fmv_script.cpp:721`
- `0x463000` `zFMV_Script::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:745`
- `0x4630a0` `zFMV_Script::BeginAtTime` -> `src/GameZRecoil/zFMV/fmv_script.cpp:780`
- `0x4630e0` `zFMV_Script::UpdateAtTime` -> `src/GameZRecoil/zFMV/fmv_script.cpp:788`
- `0x463120` `zFMV_Script::BeginNow` -> `src/GameZRecoil/zFMV/fmv_script.cpp:796`
- `0x463130` `zFMV_ActionImage::ConstructorWithScreenRect` -> `src/GameZRecoil/zFMV/fmv.h:99`
- `0x463130` `zFMV_ActionImage::ConstructorWithScreenRect` -> `src/GameZRecoil/zFMV/fmv_script.cpp:806`
- `0x4631f0` `zFMV_ActionImage::ConstructorScaled` -> `src/GameZRecoil/zFMV/fmv.h:109`
- `0x4631f0` `zFMV_ActionImage::ConstructorScaled` -> `src/GameZRecoil/zFMV/fmv_script.cpp:829`
- `0x4632a0` `zFMV_ActionImage::~zFMV_ActionImage` -> `src/GameZRecoil/zFMV/fmv_script.cpp:859`
- `0x463300` `zFMV_ActionImage::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:871`
- `0x463320` `zFMV_ActionImage::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:879`
- `0x4633a0` `zFMV_ActionImage::End` -> `src/GameZRecoil/zFMV/fmv_script.cpp:924`
- `0x4633c0` `zFMV_ActionFade::Constructor` -> `src/GameZRecoil/zFMV/fmv.h:138`
- `0x4633c0` `zFMV_ActionFade::Constructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:935`
- `0x463410` `zFMV_ActionFade::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:957`
- `0x463440` `zFMV_ActionFade::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:970`
- `0x463550` `zFMV_ActionFade::End` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1042`
- `0x463570` `zFMV_ActionPlayAvi::Constructor` -> `src/GameZRecoil/zFMV/fmv.h:170`
- `0x463570` `zFMV_ActionPlayAvi::Constructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1053`
- `0x463670` `zFMV_ActionPlayAvi::~zFMV_ActionPlayAvi` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1093`
- `0x4636d0` `zFMV_ActionPlayAvi::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1104`
- `0x463790` `zFMV_ActionPlayAvi::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1155`
- `0x463820` `zFMV_ActionPlayAvi::End` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1184`
- `0x463850` `zFMV_ActionBlur::Constructor` -> `src/GameZRecoil/zFMV/fmv.h:243`
- `0x463850` `zFMV_ActionBlur::Constructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1197`
- `0x463870` `zFMV_ActionBlur::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1209`
- `0x463920` `zFMV_ActionBlur::End` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1250`
- `0x463950` `zFMV_ActionBlur::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1272`
- `0x4639e0` `zFMV_ActionBlurH::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1323`
- `0x463a70` `zFMV_ActionBlurV::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1374`
- `0x463b00` `zFMV_ActionPlayMci::Constructor` -> `src/GameZRecoil/zFMV/fmv.h:197`
- `0x463b00` `zFMV_ActionPlayMci::Constructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1424`
- `0x463c10` `zFMV_ActionPlayMci::~zFMV_ActionPlayMci` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1462`
- `0x463c90` `zFMV_ActionPlayMci::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1480`
- `0x463ca0` `zFMV_ActionPlayMci::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1490`
- `0x463cc0` `zFMV_ActionPlayMci::End` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1506`
- `0x463d50` `zFMV_Stream::Init` -> `src/GameZRecoil/zFMV/fmv_stream.cpp:6`
- `0x463dd0` `zFMV_Stream::Destructor` -> `src/GameZRecoil/zFMV/fmv_stream.cpp:36`
- `0x463ef0` `zFMV_Stream::Constructor` -> `src/GameZRecoil/zFMV/fmv_stream.cpp:90`
- `0x4641a0` `zFMV_Stream::OpenAudio` -> `src/GameZRecoil/zFMV/fmv_stream.cpp:263`
- `0x4643a0` `zFMV_Stream::ReadAndDecodeFrame` -> `src/GameZRecoil/zFMV/fmv_stream.cpp:400`
- `0x464540` `zFMV_Stream::FillAudioBuffer` -> `src/GameZRecoil/zFMV/fmv_stream.cpp:490`
- `0x464670` `zGeometry_Weiler::GetInputContourAPointList` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1402`
- `0x464680` `zGeometry_Weiler::Init` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1420`
- `0x464790` `zGeometry_ClipPolygon::ResetWeilerStateFromContourPoints` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4352`
- `0x4647d0` `zGeometry_Weiler::DestroyState` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4167`
- `0x464810` `zGeometry_Weiler::ClipPointList` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1584`
- `0x464b30` `zGeometry_WeilerClipOutput::Destroy` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4188`
- `0x464b90` `zGeometry_Weiler::InitInputContourPair` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1508`
- `0x464c90` `zGeometry_Weiler::ClassifyInputContourPairBounds` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2678`
- `0x464ea0` `zGeometry_Weiler::OutputPreclassifiedContourPairResult` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2617`
- `0x464f70` `zGeometry_Weiler::PreclassifyInputContourPair` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3187`
- `0x465ac0` `zGeometry_Weiler::ClassifyContainedContour` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3562`
- `0x467600` `zGeometry_WeilerBuffer::Init` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1238`
- `0x467630` `zGeometry_WeilerBuffer::Destroy` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1303`
- `0x467660` `zGeometry_WeilerBuffer::GetAppendSpace` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1259`
- `0x4676c0` `zGeometry_Weiler::EnsureContourOutput` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1838`
- `0x467710` `zGeometry_Weiler::MergeContours` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1874`
- `0x4680b0` `zGeometry_Weiler::NewContour` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2460`
- `0x4681a0` `zGeometry_Weiler::OutputContoursForClipMode` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2837`
- `0x4682c0` `zGeometry_Weiler::OutputContourToPolygonSet` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2780`
- `0x4683a0` `zGeometry_Weiler::TogglePointAxesForContourSource` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4034`
- `0x468410` `zGeometry_WeilerContourSegment::UpdateBounds` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1323`
- `0x468470` `zGeometry_Weiler::BuildPointSideTablesForContourPair` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3162`
- `0x468580` `zGeometry_Weiler::DivideContourSegmentAtPoint` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2319`
- `0x468650` `zGeometry_Weiler::CreateForwardSegmentPairAtPoint` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2383`
- `0x468700` `zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2734`
- `0x4687b0` `zGeometry_Weiler::GenerateOutsideResults` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2970`
- `0x468a10` `zGeometry_Weiler::ClassifyPointInContourPointListXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2533`
- `0x468c40` `zGeometry_Weiler::Intersect2d` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3759`
- `0x468fa0` `zGeometry_Weiler::ClassifyIntersect2d` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3658`
- `0x4693a0` `zGeometry_WeilerContourSegmentArray::UpdateBounds` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1355`
- `0x4693c0` `zGeometry_WeilerContourSegmentArray::InitFromPointList` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1369`
- `0x469430` `zGeometry_Weiler::GetNextContourSegmentForTraversal` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2437`
- `0x469450` `zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3890`
- `0x469560` `zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3931`
- `0x469960` `zGeometry_Weiler::RecenterPointSetsIfOutOfRange` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4056`
- `0x469a30` `zGeometry_Weiler::PreclassifyInputContourAAdjacentEdgePairs` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3090`
- `0x469ae0` `zGeometry_WeilerBuffer::SetCountAndAppendPtr` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1290`
- `0x469af0` `zGeometry_Weiler::RestorePointTranslation` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4096`
- `0x469b60` `zGeometry_Weiler::RestoreOutputZFromInputPlane` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4123`
- `0x469ca0` `zGeometry_Vec3::IsBetweenEndpointsXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1104`
- `0x469d60` `zGeometry_Weiler::SelectForwardStartPointInContourA` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2917`
- `0x469e50` `zGeometry_Vec3::IsNearEqualXY` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:678`
- `0x469e90` `zGeometry_Vec3::SnapPointToSegmentXYIfNear` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:695`
- `0x46a080` `zGeometry_Vec3Array::RemoveAdjacentDuplicatePointsXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1131`
- `0x46a130` `zGeometry_Polygon::SnapPointsXYIfNear` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:781`
- `0x46a1f0` `zGeometry_Weiler::ValidateXings` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3984`
- `0x46a5e0` `zGeometry_Vec3Array::RotateNeg90AroundX` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:834`
- `0x46a600` `zGeometry_Vec3Array::RotatePos90AroundX` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1181`
- `0x46a620` `zGeometry_Bounds2D::OverlapsWithUnitMargin` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:139`
- `0x46a690` `zGeometry_Model::FindOrCreateRandomDebugMaterial` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1035`
- `0x46a770` `zGeometry_Model::AddPolygonToDi` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1058`
- `0x46a7f0` `zGeometry_Model::BuildPolygonUvList` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1104`
- `0x46a8e0` `zGeometry_Polygon::SolveUvAxisCoefficientsXZ` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:748`
- `0x46a9c0` `zGeometry_Vec3Array::ComputeBoundsXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1201`
- `0x46aa40` `zGeometry_ClipPolygon::CreateFromPointList` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:964`
- `0x46aab0` `zGeometry_ClipPolygon::CopyPointsOutRotatedBack` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1003`
- `0x46ab10` `zGeometry_ClipPolygon::FinalizeAndDestroy` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4378`
- `0x46ab40` `zGeometry_ClipPolygon::FindPointIndexXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4223`
- `0x46ab90` `zGeometry_ClipPolygon::UpsertPointListXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4293`
- `0x46ac80` `zGeometry_ClipPolygon::FindPointInsertionEdgeXYIndex` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4245`
- `0x46ae40` `zGeometry_ClipPatchOutput::ApplyNodeDiPairs` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1027`
- `0x46af00` `zGeometry_ClipPatchOutput::Create` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1075`
- `0x46af20` `zGeometry_ClipPatchOutput::Destroy` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1092`
- `0x46af40` `zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1109`
- `0x46b030` `zGeometry_ClipPolygon::SnapPointsNearNodeModelXY` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:856`
- `0x46b1f0` `zGeometry_Model::ClipPatch` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:425`
- `0x46b550` `zGeometry_ClipPolygon::ProcessNodePolygonSetXY` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:630`
- `0x46b650` `zGeometry_Model::GetLinearBufferOfPolygonVertices` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1078`
- `0x46b6d0` `zGeometry_Model::ProcessClipPatchNode` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:170`
- `0x46ba90` `zGeometry_Model::AddPointListPolygonToDi` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1155`
- `0x46bb30` `zGeometry_Model::AddIndexedPolygonToDi` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1214`
- `0x46bb90` `zGeometry_Model::IsFullyInsideClipPolygonXY` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1252`
- `0x46be20` `zGeometry_Segment::IntersectsSegmentXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1040`
- `0x46d5b0` `zVid::SetTexturePackLoadState` -> `src/GameZRecoil/zVideo/zvid_main.c:2780`
- `0x46d5c0` `zVid::GetTexturePackLoadState` -> `src/GameZRecoil/zVideo/zvid_main.c:2772`
- `0x46d6b0` `zVid_TexturePack::ShutdownBuiltinPacks` -> `src/GameZRecoil/zVideo/zvid_main.c:8045`
- `0x46d870` `zVid_Image::ClearZeroAlphaPixelsInPlace` -> `src/GameZRecoil/zVideo/zvid_main.c:7067`
- `0x46d900` `zImage::TexDir_FindOrCreateByPath` -> `src/GameZRecoil/zImage/zimg_texture.cpp:487`
- `0x46d940` `zVid_TexturePack_LoadImageByName` -> `src/GameZRecoil/zVideo/zvid_main.c:7948`
- `0x46da40` `zVid_TexturePack_EnsureDefaultImagePackLoaded` -> `src/GameZRecoil/zVideo/zvid_main.c:7701`
- `0x46dae0` `zVid_TexturePackEntry_LoadFromFile` -> `src/GameZRecoil/zVideo/zvid_main.c:7596`
- `0x46dd30` `zVid_TexturePack_LoadBuiltinImageByName` -> `src/GameZRecoil/zVideo/zvid_main.c:7966`
- `0x46dd30` `zVid_TexturePack_LoadBuiltinImageByName` -> `src/GameZRecoil/zVideo/zvid_main.c:7969`
- `0x46e4e0` `zVid_PaletteRemap::ApplyRecipeToPaletteVariant` -> `src/GameZRecoil/zVideo/zvid_main.c:7371`
- `0x46e680` `zVid_PaletteRemap::FindRecipeIndex` -> `src/GameZRecoil/zVideo/zvid_main.c:7345`
- `0x46e8d0` `zVid_PaletteRemap_BuildAllRecipeVariantsForPalette` -> `src/GameZRecoil/zVideo/zvid_main.c:7530`
- `0x46e960` `zVid_PaletteRemap_FindRecipeIndexFromRgb` -> `src/GameZRecoil/zVideo/zvid_main.c:7573`
- `0x46ec20` `zVid_Image::QueryBytesPerPixel` -> `src/GameZRecoil/zVideo/zvid_main.c:6885`
- `0x46ec30` `zVid_Image::SetHeaderFlagsByte` -> `src/GameZRecoil/zVideo/zvid_main.c:6895`
- `0x46ec40` `zVid_Image::QueryPixelDataBytes` -> `src/GameZRecoil/zVideo/zvid_main.c:7053`
- `0x46ec60` `zVid_Image::SetFormatCode` -> `src/GameZRecoil/zVideo/zvid_main.c:6907`
- `0x46ec70` `zVid_Image_SetPixels` -> `src/GameZRecoil/zVideo/zvid_main.c:6965`
- `0x46ec90` `zVid_Image::SetSize` -> `src/GameZRecoil/zVideo/zvid_main.c:6919`
- `0x46ecf0` `zVid_Image::ReleaseOwnedBuffers` -> `src/GameZRecoil/zVideo/zvid_main.c:6856`
- `0x46ed70` `zVid_Image::ReadHeader` -> `src/GameZRecoil/zVideo/zvid_main.c:7129`
- `0x46ede0` `zVid_Image::ReadData` -> `src/GameZRecoil/zVideo/zvid_main.c:7166`
- `0x46ef70` `zVid_Image::ReadFromFile` -> `src/GameZRecoil/zVideo/zvid_main.c:7259`
- `0x46f450` `zInput::Keyboard_ResetTransitionState` -> `src/GameZRecoil/zInput/zin_kbd.cpp:122`
- `0x46f690` `zInput::Keyboard_PollState` -> `src/GameZRecoil/zInput/zin_kbd.cpp:222`
- `0x46f970` `zInput::Keyboard_SetRawEventCallback` -> `src/GameZRecoil/zInput/zin_kbd.cpp:274`
- `0x46f9b0` `zInput::Keyboard_RegisterKeyCallback` -> `src/GameZRecoil/zInput/zin_kbd.cpp:307`
- `0x46f9d0` `zInput::Keyboard_UnregisterKeyCallback` -> `src/GameZRecoil/zInput/zin_kbd.cpp:324`
- `0x46f9f0` `zInput::Keyboard_ClearKeyCallbackTable` -> `src/GameZRecoil/zInput/zin_kbd.cpp:336`
- `0x46fa10` `zInput::Keyboard_WaitForAnyKeyPress` -> `src/GameZRecoil/zInput/zin_kbd.cpp:350`
- `0x46fba0` `zInput::Keyboard_TranslateDikToAscii` -> `src/GameZRecoil/zInput/zin_kbd.cpp:438`
- `0x46fd20` `zInput::Keyboard_InitDikToAsciiTable` -> `src/GameZRecoil/zInput/zin_kbd.cpp:508`
- `0x4702e0` `zInput::Mouse_GetButtonTransitionState` -> `src/GameZRecoil/zInput/zin_mouse.cpp:182`
- `0x470310` `zInput::Mouse_UpdateAcquireState` -> `src/GameZRecoil/zInput/zin_mouse.cpp:213`
- `0x4703a0` `zInput::Mouse_GetStateSnapshotPtr` -> `src/GameZRecoil/zInput/zin_mouse.cpp:266`
- `0x4703b0` `zInput::Mouse_PollAndStoreState` -> `src/GameZRecoil/zInput/zin_mouse.cpp:274`
- `0x4703c0` `zInput::Mouse_PollState` -> `src/GameZRecoil/zInput/zin_mouse.cpp:284`
- `0x4704f0` `zInput::Mouse_ApplyAccumulatedDelta` -> `src/GameZRecoil/zInput/zin_mouse.cpp:331`
- `0x470610` `zInput::Mouse_ResetTransitionState` -> `src/GameZRecoil/zInput/zin_mouse.cpp:403`
- `0x4706c0` `zInput_BindMapContext::InitFromTemplate` -> `src/GameZRecoil/zInput/zInput.cpp:1950`
- `0x4707a0` `zInput_BindMapContext::FreeAllBuffers` -> `src/GameZRecoil/zInput/zInput.cpp:2002`
- `0x470820` `zInput_BindMapContext::RebuildLookupIndices` -> `src/GameZRecoil/zInput/zInput.cpp:2029`
- `0x4708f0` `zInput_BindMapContext::InitCommandMap` -> `src/GameZRecoil/zInput/zInput.cpp:2070`
- `0x470960` `zInput_BindMapContext::FreeNonOwnedBuffers` -> `src/GameZRecoil/zInput/zInput.cpp:2106`
- `0x4709d0` `zInput_BindMapContext::ResetAllBindings` -> `src/GameZRecoil/zInput/zInput.cpp:2130`
- `0x470a10` `zInput::BindMap_PackBindingCode` -> `src/GameZRecoil/zInput/zInput.cpp:2148`
- `0x470a40` `zInput_BindMapContext::GetPrimaryKeyboardKey` -> `src/GameZRecoil/zInput/zInput.cpp:2162`
- `0x470a60` `zInput_BindMapContext::GetSecondaryKeyboardKey` -> `src/GameZRecoil/zInput/zInput.cpp:2172`
- `0x470a80` `zInput_BindMapContext::GetJoystickButtonSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2182`
- `0x470aa0` `zInput_BindMapContext::GetMouseButtonSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2192`
- `0x470ac0` `zInput_BindMapContext::GetCommandByPrimaryKey` -> `src/GameZRecoil/zInput/zInput.cpp:2202`
- `0x470ad0` `zInput_BindMapContext::GetCommandBySecondaryKey` -> `src/GameZRecoil/zInput/zInput.cpp:2213`
- `0x470ae0` `zInput_BindMapContext::GetCommandByAnyKeyboardKey` -> `src/GameZRecoil/zInput/zInput.cpp:2224`
- `0x470b00` `zInput_BindMapContext::GetCommandByJoystickSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2240`
- `0x470b10` `zInput_BindMapContext::GetCommandByMouseSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2251`
- `0x470b20` `zInput_BindMapContext::SetPrimaryKeyBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2261`
- `0x470b80` `zInput_BindMapContext::SetSecondaryKeyBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2283`
- `0x470bf0` `zInput_BindMapContext::SetJoystickBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2305`
- `0x470c60` `zInput_BindMapContext::SetMouseBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2327`
- `0x470cd0` `zInput_BindMapContext::SetBindingRecord` -> `src/GameZRecoil/zInput/zInput.cpp:2349`
- `0x470d40` `zInput_BindMapContext::DispatchMouseButtonCallbacks` -> `src/GameZRecoil/zInput/zInput.cpp:2388`
- `0x470db0` `zInput_BindMapContext::DispatchJoystickButtonCallbacks` -> `src/GameZRecoil/zInput/zInput.cpp:2417`
- `0x470df0` `zInput_BindMapContext::SetCommandCallback` -> `src/GameZRecoil/zInput/zInput.cpp:2435`
- `0x470e80` `zInput_BindMapContext_DispatchFromKeyboardEvent` -> `src/GameZRecoil/zInput/zInput.cpp:2468`
- `0x470eb0` `zInput_BindMapContext::ReadCommandInputState` -> `src/GameZRecoil/zInput/zInput.cpp:2482`
- `0x470f50` `zInput_BindMapContext::CopyCommandLabel` -> `src/GameZRecoil/zInput/zInput.cpp:2519`
- `0x470f80` `zInput::BindMap_FormatKeyComboName` -> `src/GameZRecoil/zInput/zInput.cpp:2545`
- `0x471040` `zInput::BindMap_CopyJoystickButtonName` -> `src/GameZRecoil/zInput/zInput.cpp:2596`
- `0x471070` `zInput::BindMap_CopyMouseButtonName` -> `src/GameZRecoil/zInput/zInput.cpp:2619`
- `0x471120` `zInput::BindMap_InitDikKeyNameTable` -> `src/GameZRecoil/zInput/zInput.cpp:2665`
- `0x4715e0` `zInput::BindMap_InitJoystickButtonNameTable` -> `src/GameZRecoil/zInput/zInput.cpp:2794`
- `0x471640` `zInput::BindMap_InitMouseButtonNameTable` -> `src/GameZRecoil/zInput/zInput.cpp:2811`
- `0x4716b0` `zInput::BindMap_Current_RebuildLookupIndices` -> `src/GameZRecoil/zInput/zInput.cpp:2850`
- `0x4716c0` `zInput::BindMapCurrent_ResetAllBindings` -> `src/GameZRecoil/zInput/zInput.cpp:2858`
- `0x4716d0` `zInput::BindMapCurrent_GetPrimaryKeyboardKey` -> `src/GameZRecoil/zInput/zInput.cpp:2866`
- `0x4716e0` `zInput::BindMapCurrent_GetSecondaryKeyboardKey` -> `src/GameZRecoil/zInput/zInput.cpp:2878`
- `0x4716f0` `zInput::BindMapCurrent_GetJoystickButtonSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2890`
- `0x471700` `zInput::BindMapCurrent_GetMouseButtonSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2902`
- `0x471710` `zInput::BindMapCurrent_GetCommandByPrimaryKey` -> `src/GameZRecoil/zInput/zInput.cpp:2914`
- `0x471720` `zInput::BindMapCurrent_GetCommandBySecondaryKey` -> `src/GameZRecoil/zInput/zInput.cpp:2926`
- `0x471730` `zInput::BindMapCurrent_GetCommandByJoystickSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2938`
- `0x471740` `zInput::BindMapCurrent_GetCommandByMouseSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2950`
- `0x471750` `zInput::BindMapCurrent_SetPrimaryKeyBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2962`
- `0x471760` `zInput::BindMapCurrent_SetSecondaryKeyBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2978`
- `0x471770` `zInput::BindMapCurrent_SetJoystickBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2994`
- `0x471780` `zInput::BindMapCurrent_SetMouseBinding` -> `src/GameZRecoil/zInput/zInput.cpp:3010`
- `0x471790` `zInput::BindMap_Current_SetBindingRecord` -> `src/GameZRecoil/zInput/zInput.cpp:3026`
- `0x4717c0` `zInput::BindMapCurrent_SetCommandCallback` -> `src/GameZRecoil/zInput/zInput.cpp:3051`
- `0x4717d0` `zInput::BindMapCurrent_ReadCommandInputState` -> `src/GameZRecoil/zInput/zInput.cpp:3068`
- `0x4717e0` `zInput::BindMapCurrent_CopyCommandLabel` -> `src/GameZRecoil/zInput/zInput.cpp:3081`
- `0x471800` `zInput::BindMapCurrent_FormatKeyComboName` -> `src/GameZRecoil/zInput/zInput.cpp:3100`
- `0x471820` `zInput::BindMapCurrent_CopyJoystickButtonName` -> `src/GameZRecoil/zInput/zInput.cpp:3118`
- `0x471840` `zInput::BindMapCurrent_CopyMouseButtonName` -> `src/GameZRecoil/zInput/zInput.cpp:3136`
- `0x471860` `zInput::BindMapContext_Push` -> `src/GameZRecoil/zInput/zInput.cpp:3154`
- `0x471950` `zInput::BindMapContext_Pop` -> `src/GameZRecoil/zInput/zInput.cpp:3199`
- `0x471c50` `zInput::ResetAllTransitionState` -> `src/GameZRecoil/zInput/zin_init.cpp:221`
- `0x471de0` `zInput::PollActiveDevices` -> `src/GameZRecoil/zInput/zin_init.cpp:484`
- `0x471fb0` `zInput::DI_AcquireJoystickDevice` -> `src/GameZRecoil/zInput/zin_joystick.cpp:109`
- `0x4722c0` `zInput::DI_PollJoystickState` -> `src/GameZRecoil/zInput/zin_joystick.cpp:346`
- `0x472390` `zInput::DI_GetCurrentState` -> `src/GameZRecoil/zInput/zin_joystick.cpp:396`
- `0x4723a0` `zInput::DI_GetButtonTransitionState` -> `src/GameZRecoil/zInput/zin_joystick.cpp:405`
- `0x4723d0` `zInput::DI_WaitForButtonPress` -> `src/GameZRecoil/zInput/zin_joystick.cpp:420`
- `0x472410` `zInput::DI_ResetTransitionState` -> `src/GameZRecoil/zInput/zin_joystick.cpp:446`
- `0x472490` `zInput::DI_ReportError` -> `src/GameZRecoil/zInput/zin_joystick.cpp:656`
- `0x4726d0` `zMath_Vec3_Distance` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:83`
- `0x472730` `zMath_Vec3_DistSqXZ` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:97`
- `0x472770` `zMath_Vec3_ScaleAdd` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:110`
- `0x4727a0` `zMath_Vec3_DivScalar` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:125`
- `0x4727f0` `zMath_Vec3_NormalizeXZ` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:146`
- `0x472860` `zMath_Vec3_Reflect` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:163`
- `0x472960` `zMath_Vec3_Lerp` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:179`
- `0x4729b0` `zMath_Vec3_DirectionTo` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:194`
- `0x4729f0` `zMath_Vec3_LerpNormalize` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:210`
- `0x472a10` `zMath_Vec3_Slerp` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:226`
- `0x472cc0` `zMath_Vec3_Perp2D` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:260`
- `0x472d30` `zMath::CrtMatherrHandler` -> `src/GameZRecoil/zMath/zmth_main.c:183`
- `0x472d30` `zMath_Main_MathErrHandler` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:386`
- `0x472ed0` `zMath_Project_GetLastScreenScaleXY` -> `src/GameZRecoil/zMath/zmth_main.c:1833`
- `0x472ed0` `zMath_GetProjectedScreenSize` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:402`
- `0x472ef0` `zMath::MatStackPushAndCloneParent` -> `src/GameZRecoil/zMath/zmth_main.c:686`
- `0x472ef0` `zMath_MatStack_Push` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:411`
- `0x472f30` `zMath::MatStackPushPtr` -> `src/GameZRecoil/zMath/zmth_main.c:672`
- `0x472f30` `zMath_MatStack_PushNew` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:429`
- `0x472f60` `zMath::MatStackPopPtr` -> `src/GameZRecoil/zMath/zmth_main.c:705`
- `0x472f60` `zMath_MatStack_Pop` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:441`
- `0x472f90` `zMath::MatLoadCameraScratchB` -> `src/GameZRecoil/zMath/zmth_main.c:714`
- `0x472f90` `zMath_Mat_LoadProjection` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:452`
- `0x472fa0` `zMath::MatLoadCameraScratchA` -> `src/GameZRecoil/zMath/zmth_main.c:723`
- `0x472fa0` `zMath_Mat_LoadView` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:467`
- `0x472fb0` `zMath_Mat_LoadProjection` -> `src/GameZRecoil/zMath/zmth_main.c:1876`
- `0x472fb0` `zMath_Mat_SetupCamera` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:485`
- `0x473060` `zMath_Mat_LoadView` -> `src/GameZRecoil/zMath/zmth_main.c:1977`
- `0x473060` `zMath_Mat_TransformPoint` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:492`
- `0x4731f0` `zMath_Mat_SetupCamera` -> `src/GameZRecoil/zMath/zmth_main.c:1862`
- `0x473210` `zMath::MatCopyCurrentTo` -> `src/GameZRecoil/zMath/zmth_main.c:1135`
- `0x473230` `zMath_Mat_GetCurrent` -> `src/GameZRecoil/zMath/zmth_main.c:2056`
- `0x473240` `zMath_Mat_IsCurrentIdentity` -> `src/GameZRecoil/zMath/zmth_main.c:2064`
- `0x473250` `zMath::MatLoadCurrentFrom` -> `src/GameZRecoil/zMath/zmth_main.c:1151`
- `0x473250` `zMath_Mat_Load` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:506`
- `0x473280` `zMath::MatLoadRotationFrom3x3` -> `src/GameZRecoil/zMath/zmth_main.c:1167`
- `0x4732f0` `zMath::MatLoadIdentity` -> `src/GameZRecoil/zMath/zmth_main.c:732`
- `0x4732f0` `zMath_Mat_LoadIdentity` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:522`
- `0x473370` `zMath::MatMultiply` -> `src/GameZRecoil/zMath/zmth_main.c:1190`
- `0x473370` `zMath_Mat_Multiply` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:541`
- `0x473690` `zMath_Mat_Scale` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:549`
- `0x4737e0` `zMath::MatTranslate` -> `src/GameZRecoil/zMath/zmth_main.c:1237`
- `0x4737e0` `zMath_Mat_Translate` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:559`
- `0x473970` `zMath::MatRotateX` -> `src/GameZRecoil/zMath/zmth_main.c:1263`
- `0x473970` `zMath_Mat_RotateX` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:569`
- `0x473b10` `zMath::MatRotateY` -> `src/GameZRecoil/zMath/zmth_main.c:1299`
- `0x473b10` `zMath_Mat_RotateY` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:575`
- `0x473cc0` `zMath::MatRotateZ` -> `src/GameZRecoil/zMath/zmth_main.c:1348`
- `0x473cc0` `zMath_Mat_RotateZ` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:581`
- `0x473e60` `zMath_Camera_StageInverseRotation` -> `src/GameZRecoil/zMath/zmth_main.c:2173`
- `0x473e60` `zMath_Mat_BuildViewFromWorld` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:591`
- `0x473fc0` `zMath::Vec3ArrayProjectToCachedY` -> `src/GameZRecoil/zMath/zmth_main.c:1510`
- `0x473fc0` `zMath_Mat_TransformPointBatch` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:601`
- `0x474010` `zMath::MatApplyLocalTRS` -> `src/GameZRecoil/zMath/zmth_main.c:1384`
- `0x474010` `zMath_Mat_BuildFromEulerTRS` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:612`
- `0x474260` `zMath::MatBuildEulerRotation3x3` -> `src/GameZRecoil/zMath/zmth_main.c:1449`
- `0x474260` `zMath_Mat_BuildFromAngles` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:630`
- `0x4743e0` `zMath_SetScreenSize` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:641`
- `0x474400` `zMath_SetViewport` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:650`
- `0x4744f0` `zMath_Vec3Array_AddScaled` -> `src/GameZRecoil/zMath/zmth_main.c:2364`
- `0x4744f0` `zMath_Vec3_ScaleAddBatch` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:286`
- `0x474580` `zMath_Vec3_DirFromYaw` -> `src/GameZRecoil/zMath/zmth_main.c:2151`
- `0x474580` `zMath_Mat_ExtractAnglesDown` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:664`
- `0x4745c0` `zMath::Vec3PerpXZ` -> `src/GameZRecoil/zMath/zmth_main.c:797`
- `0x4745c0` `zMath_Vec3_PerpXZ` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:273`
- `0x4745e0` `zMath_Vec3Array_UntransformDirection` -> `src/GameZRecoil/zMath/zmth_main.c:470`
- `0x4745e0` `zMath_Mat_TransformNormalBatch` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:695`
- `0x474670` `zMath::Vec3ArrayTransformDirection` -> `src/GameZRecoil/zMath/zmth_main.c:1543`
- `0x474670` `zMath_Mat_TransformNormalBatchInPlace` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:707`
- `0x474710` `zMath_Mat_TransformNormalBatch` -> `src/GameZRecoil/zMath/zmth_main.c:437`
- `0x4747d0` `zMath::MatTransformPointBatchInPlace` -> `src/GameZRecoil/zMath/zmth_main.c:1565`
- `0x474870` `zMath_Mat_TransformBBoxToCorners` -> `src/GameZRecoil/zMath/zmth_main.c:2510`
- `0x474870` `zMath_BBox_Transform` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:729`
- `0x474b20` `zMath::ProjectPointBatch` -> `src/GameZRecoil/zMath/zmth_main.c:1590`
- `0x474b20` `zMath_ProjectPointBatch` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:746`
- `0x474b70` `zMath_ProjectSphereBatch` -> `src/GameZRecoil/zMath/zmth_main.c:2475`
- `0x474b70` `zMath_ProjectPointBatchZBuf` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:766`
- `0x474bc0` `zMath_UnprojectPointBatch` -> `src/GameZRecoil/zMath/zmth_main.c:1914`
- `0x474bc0` `zMath_UnprojectPointBatch` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:786`
- `0x474c20` `zMath_UnprojectPointBatchZBuf` -> `src/GameZRecoil/zMath/zmth_main.c:1932`
- `0x474d10` `zMath::Vec3DirectionAnglesBetweenPoints` -> `src/GameZRecoil/zMath/zmth_main.c:1485`
- `0x474d10` `zMath_Vec3_DirectionAngles` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:339`
- `0x474d90` `zMath_Vec3_ElevationAngleBetweenPoints` -> `src/GameZRecoil/zMath/zmth_main.c:1844`
- `0x474d90` `zMath_Vec3_ElevationAngle` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:364`
- `0x474de0` `zMath_Mat_ExtractYaw` -> `src/GameZRecoil/zMath/zmth_main.c:2072`
- `0x474de0` `zMath_Mat_ExtractYaw` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:674`
- `0x474e10` `zMath_Mat_ExtractEulerAngles` -> `src/GameZRecoil/zMath/zmth_main.c:2090`
- `0x474e10` `zMath_Mat_ExtractEulerAngles` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:681`
- `0x474ec0` `zMath_Vec3_RotateX` -> `src/GameZRecoil/zMath/zmth_main.c:2135`
- `0x474ec0` `zMath_Vec3_RotateAroundX` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:323`
- `0x474f40` `zMath_Vec3_RotateAroundY` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:307`
- `0x474fc0` `zMath::ApproxExpNeg` -> `src/GameZRecoil/zMath/zmth_main.c:1761`
- `0x474fc0` `zMath_FogTableLookup` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:814`
- `0x475070` `zMath_Vec3_TriangleNormal` -> `src/GameZRecoil/zMath/zmth_main.c:2384`
- `0x475070` `zMath_TriangleNormal` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:846`
- `0x475130` `zMath_SolveLinearGradient2D` -> `src/GameZRecoil/zMath/zmth_main.c:2411`
- `0x475130` `zMath_SolveLinear2x2` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:872`
- `0x475210` `zMath::LineVsSphereHit` -> `src/GameZRecoil/zMath/zmth_main.c:982`
- `0x475210` `zMath_RaySphereIntersect` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:892`
- `0x4753e0` `zMath_BuildPerspectiveTextureInterpolants` -> `src/GameZRecoil/zMath/zmth_main.c:543`
- `0x4753e0` `zMath_TriangleScreenGradients` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:974`
- `0x4757c0` `zMath_Quat_FromEuler` -> `src/GameZRecoil/zMath/zmth_main.c:2229`
- `0x4757c0` `zMath_Quat_FromEuler` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:996`
- `0x475910` `zMath_Quat_Multiply` -> `src/GameZRecoil/zMath/zmth_main.c:2260`
- `0x475910` `zMath_Quat_Multiply` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:1022`
- `0x4759d0` `zMath_Quat_MultiplyInverse` -> `src/GameZRecoil/zMath/zmth_main.c:2280`
- `0x4759d0` `zMath_Quat_MultiplyInverse` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:1038`
- `0x475a80` `zMath_Quat_ToMatrix` -> `src/GameZRecoil/zMath/zmth_main.c:2300`
- `0x475a80` `zMath_Quat_ToMatrix` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:1055`
- `0x475b80` `zMath_Quat_FromRotationVector` -> `src/GameZRecoil/zMath/zmth_main.c:2334`
- `0x475b80` `zMath_Quat_FromRotationVector` -> `src/GameZRecoil/zMath/zmth_main_legacy_impl_body.h:1089`
- `0x475c40` `zModel_Display_Init` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:605`
- `0x475e60` `zModel_Display::ShutdownThunk` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1291`
- `0x475e70` `zModel::Init` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1984`
- `0x475f60` `zModel_Display::Reset` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1257`
- `0x475fa0` `zModel_Display::Shutdown` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1272`
- `0x475ff0` `zModel::SetDisplayInstancePoolCapacity` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2036`
- `0x476020` `zModel::SetSoftwarePathActive` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2058`
- `0x476030` `zModel::SetVertexShadingEnabled` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2025`
- `0x476040` `zModel_FogTargetColorOverride_SetCurrent` -> `src/GameZRecoil/zModel/gmod_light.c:478`
- `0x476070` `zModel_RenderAlphaScale_SetCurrent` -> `src/GameZRecoil/zModel/gmod_light.c:494`
- `0x476080` `zModel_RenderVertexAlphaEnabled_SetCurrent` -> `src/GameZRecoil/zModel/gmod_light.c:505`
- `0x476090` `zModel::SetTextureWorldPerMeter` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2071`
- `0x4760b0` `zModel::SetTextureWorldBase` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2084`
- `0x4760d0` `zModel::SetDiTextureWorldPerMeter` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2097`
- `0x476120` `zClipAlt::SetSourceRect` -> `src/GameZRecoil/zModel/zclip_alt_impl_body.h:157`
- `0x476170` `zModel_Fog_SetEnabled` -> `src/GameZRecoil/zModel/gmod_light.c:343`
- `0x476180` `zModel_Fog_IsEnabled` -> `src/GameZRecoil/zModel/gmod_light.c:354`
- `0x476190` `zModel_Fog_SetDistanceStart` -> `src/GameZRecoil/zModel/gmod_light.c:363`
- `0x4761d0` `zModel_Fog_GetDistanceStart` -> `src/GameZRecoil/zModel/gmod_light.c:377`
- `0x4761e0` `zModel_Fog_SetDistanceEnd` -> `src/GameZRecoil/zModel/gmod_light.c:386`
- `0x476220` `zModel_Fog_SetHeightHigh` -> `src/GameZRecoil/zModel/gmod_light.c:400`
- `0x476260` `zModel_Fog_SetHeightLow` -> `src/GameZRecoil/zModel/gmod_light.c:414`
- `0x4762a0` `zModel_Fog_SetDensity` -> `src/GameZRecoil/zModel/gmod_light.c:428`
- `0x4762b0` `zModel_Fog_SetLinearModeEnabled` -> `src/GameZRecoil/zModel/gmod_light.c:439`
- `0x4762c0` `zModel_Fog_SetColorRgb01` -> `src/GameZRecoil/zModel/gmod_light.c:450`
- `0x4762f0` `zModel_Fog_ApplyCurrentColor` -> `src/GameZRecoil/zModel/gmod_light.c:469`
- `0x476300` `zRndr::SetInverseZTolerance` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:466`
- `0x476320` `zTag4::Clear` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1303`
- `0x476340` `zDi::SetVariantTagIfUnset` -> `src/GameZRecoil/zModel/gmod_matl.c:86`
- `0x476460` `zModel::SetBackfaceEliminationToleranceScalar` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:444`
- `0x476470` `zModel::GetBackfaceEliminationToleranceScalar` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:455`
- `0x476480` `zMath::ProjectPointAndClampToScreenClip` -> `src/GameZRecoil/zMath/zmth_main.c:1682`
- `0x4766a0` `zClipAlt::RemapPointXYInPlace` -> `src/GameZRecoil/zModel/zclip_alt_impl_body.h:215`
- `0x476700` `zScene::TestProjectedSphereVisible` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:482`
- `0x476a50` `zDi::EvalBoundingSphereLightingFlags` -> `src/GameZRecoil/zModel/gmod_impl_body.h:3065`
- `0x476cf0` `zModel::RenderNodeSoftware` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2124`
- `0x477b30` `zModel::RenderNodeHardware` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2502`
- `0x478fc0` `zModel_Instance_UpdateScrollingTexturesIfNeeded` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1948`
- `0x479020` `zModel_RenderPointQueueEntry` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2720`
- `0x4791c0` `zModel_Instance_UpdateScrollingTextures` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1867`
- `0x479660` `OptCatalog::ApplyDamageMaskStampOnHit` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:736`
- `0x479c50` `OptCatalog::SetDamageMaskSlotIndex` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:713`
- `0x479c60` `OptCatalog::RegisterDamageMaskSlotPtr` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:724`
- `0x479c80` `OptCatalog_IsDamageMaskEnabled` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:703`
- `0x479c90` `OptCatalog_SetDamageMaskUv` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:690`
- `0x479cb0` `OptCatalog_SetDamageMaskEnabled` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:884`
- `0x479cc0` `OptCatalog_IsDamageMaskSlotPtrRegistered` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:895`
- `0x479f90` `zClipAlt::SetTargetRect` -> `src/GameZRecoil/zModel/zclip_alt_impl_body.h:175`
- `0x47a1d0` `zClipAlt_BuildFrustumPlanes` -> `src/GameZRecoil/zModel/zclip_alt_impl_body.h:130`
- `0x47a200` `zClipRect::ClipPolyZRange_NoUV` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:1634`
- `0x47a4e0` `zClipRect::ClipPolyZRange_NoUV_WithAttribs` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:1738`
- `0x47aa80` `zClipRect::ClipPolyNearZ` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:1387`
- `0x47af60` `zClipRect::ClipPolyNearZ_WithAttr0` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:1505`
- `0x47b540` `zClipRect::ClipPoly_NoUV_Alt` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:2043`
- `0x47bd30` `zClipRect::ClipPoly_NoUV_WithAttr012_Alt` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:3708`
- `0x47cdc0` `zClipRect::ClipPoly_NoUV` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:2304`
- `0x47d3f0` `zClipRect::ClipPoly` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:2557`
- `0x47dfb0` `zClipRect::ClipPoly_NoUV_WithAttr0_Alt` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:3391`
- `0x47e900` `zClipRect::ClipPolyZRange_WithAttr012` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:1882`
- `0x47efd0` `zClipRect::ClipPoly_WithAttr012` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:2890`
- `0x4803b0` `zClipRect::TrivialRejectPolyXY` -> `src/GameZRecoil/zModel/zclip_rect_impl_body.h:4137`
- `0x4804c0` `zModel::UpdateSmallPolyRejectThresholds` -> `src/GameZRecoil/zModel/gmod_scene_impl_body.h:16`
- `0x4804e0` `zReader::FindGlobalStringPrefixIndex` -> `src/GameZRecoil/zReader/zreader_lookup_impl_body.h:67`
- `0x4805b0` `zModel_MatlSlot::IndexFromPtrOrMinus1` -> `src/GameZRecoil/zModel/gmod_matl.c:112`
- `0x4805e0` `zModel_Matl::GetPoolEntry` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1240`
- `0x480600` `zModel_MatlBuffer::WriteGameZ` -> `src/GameZRecoil/zModel/gmod_matl.c:162`
- `0x4808c0` `zModel_MatlBuffer::ReadGameZ` -> `src/GameZRecoil/zModel/gmod_matl.c:329`
- `0x480ae0` `zModel_Matl::InitGlobals` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1207`
- `0x480bf0` `zModel_MatlBuffer::SetArraySize` -> `src/GameZRecoil/zModel/gmod_matl.c:129`
- `0x480c40` `zModel_Material::ResetDefaults` -> `src/GameZRecoil/zModel/gdi_impl_body.h:563`
- `0x480c80` `zModel_Material::HasAuxData` -> `src/GameZRecoil/zModel/gdi_impl_body.h:582`
- `0x480ca0` `zModel_Material::FindOrClone` -> `src/GameZRecoil/zModel/gdi_impl_body.h:658`
- `0x480d20` `zModel_Material::CompareForReuse` -> `src/GameZRecoil/zModel/gdi_impl_body.h:594`
- `0x480d80` `zModel_MatlBuffer::ReleaseAllActive` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1136`
- `0x480dc0` `zModel_MatlSlot::Release` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1077`
- `0x480ec0` `zRndr::GlobalStringTable_ReleaseDynamicEntries` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:913`
- `0x480f10` `zModel_MatlBuffer::Shutdown` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1184`
- `0x480f60` `zModel_Material::SetFlagBit9` -> `src/GameZRecoil/zModel/gdi_impl_body.h:971`
- `0x480f80` `zModel_Material::InvalidateImagesIfEligible` -> `src/GameZRecoil/zModel/gdi_impl_body.h:990`
- `0x480fd0` `zModel_MatlBuffer::ReleaseTextureSurfaces` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1154`
- `0x481040` `zModel_Material::SetUserTag` -> `src/GameZRecoil/zModel/gdi_impl_body.h:691`
- `0x481050` `zModel_Material::SetCycleTextureCount` -> `src/GameZRecoil/zModel/gdi_impl_body.h:708`
- `0x481100` `zModel_Material::AddCycleTexture` -> `src/GameZRecoil/zModel/gdi_impl_body.h:751`
- `0x481140` `zModel_Material::UpdateCycleIfNeeded` -> `src/GameZRecoil/zModel/gdi_impl_body.h:781`
- `0x481220` `zModel_Material::SetCycleTextureLoop` -> `src/GameZRecoil/zModel/gdi_impl_body.h:820`
- `0x481260` `zModel_Material::SetCycleTextureSpeed` -> `src/GameZRecoil/zModel/gdi_impl_body.h:849`
- `0x4812b0` `zModel_Material::Clone` -> `src/GameZRecoil/zModel/gdi_impl_body.h:958`
- `0x4812c0` `zModel_MatlBuffer::CloneToActiveSlot` -> `src/GameZRecoil/zModel/gdi_impl_body.h:884`
- `0x481420` `zModel_Material::FindByTexDirEntry` -> `src/GameZRecoil/zModel/gdi_impl_body.h:633`
- `0x481460` `zRndr_GlobalStringTable::LoadDynamicEntriesFromPath` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:929`
- `0x481530` `zModel_Const::GetVertexMergeEpsilon` -> `src/GameZRecoil/zModel/gmod_const.c:247`
- `0x481540` `zModel_Const::SetVertexMergeEpsilon` -> `src/GameZRecoil/zModel/gmod_const.c:256`
- `0x481550` `zModel_Const::SetCoplanarTolerance` -> `src/GameZRecoil/zModel/gmod_const.c:275`
- `0x481560` `zModel_Const::SetColinearTolerance` -> `src/GameZRecoil/zModel/gmod_const.c:284`
- `0x481570` `zDi::PtrToIndexOrMinus1` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:984`
- `0x4815a0` `zDi::IndexToPtrOrNull` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:999`
- `0x4815c0` `zModel_DiPool::WriteToStream` -> `src/GameZRecoil/zModel/gmod_const.c:295`
- `0x481aa0` `zModel_DiPool::ReadEntryByIndexFromStream` -> `src/GameZRecoil/zModel/gmod_const.c:781`
- `0x481bc0` `zModel_DiPool::ReadHeaderFromStream` -> `src/GameZRecoil/zModel/gmod_const.c:532`
- `0x481c50` `zModel_DiPool::ReadEntryDynamicDataFromStream` -> `src/GameZRecoil/zModel/gmod_const.c:585`
- `0x481fa0` `zModel_DiPool::ReadFromStream` -> `src/GameZRecoil/zModel/gmod_const.c:862`
- `0x482080` `zModel_DiPool::AllocFromFreeList` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1016`
- `0x4820f0` `zModel_DiPool::FreeIfUnreferenced` -> `src/GameZRecoil/zModel/gmod_display_impl_body.h:1045`
- `0x482160` `zDi::FreeContents` -> `src/GameZRecoil/zModel/gdi_impl_body.h:135`
- `0x482270` `zDi::CloneToInstance` -> `src/GameZRecoil/zModel/gdi_impl_body.h:221`
- `0x4826a0` `zUtil::StoreInt32` -> `src/GameZRecoil/zModel/zutil_impl_body.h:5`
- `0x4826b0` `zDi::SetClonedFlag` -> `src/GameZRecoil/zModel/gdi_impl_body.h:207`
- `0x4826d0` `zDi::SetFlagBit0` -> `src/GameZRecoil/zModel/gdi_impl_body.h:193`
- `0x4826f0` `zDi::AddRef` -> `src/GameZRecoil/zModel/gdi_impl_body.h:106`
- `0x482700` `zDi::Release` -> `src/GameZRecoil/zModel/gdi_impl_body.h:116`
- `0x482710` `zDi::GetRefCount` -> `src/GameZRecoil/zModel/gdi_impl_body.h:126`
- `0x482720` `zModel_Const::AddOrMergeVertex` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1557`
- `0x482860` `zModel_Const::AddOrMergeVertexAndNormal` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1603`
- `0x482a10` `zModel_Const::FindOrAppendNormalIndex` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1665`
- `0x482b40` `zModel_Const::RemoveColinearVerticesInPlace` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1414`
- `0x482c60` `zModel_Const::SetNormalizedCrossFromVertexTriplet` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1375`
- `0x482db0` `zModel_Const::IsPolygonCoplanar` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1525`
- `0x482e30` `zModel_Const::ComputePolygonPlaneEquation` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1477`
- `0x482fe0` `zModel_Const::SplitPolygonChunkedByVertexLimit` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1785`
- `0x483240` `zDi::AddPolygonSplitByVertexLimit` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1160`
- `0x483510` `zModel_Const::QuantizeAndNormalizeUvPairs` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1746`
- `0x483610` `zDi::AddPolygon` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1126`
- `0x483650` `zDi::AddPolygonEx` -> `src/GameZRecoil/zModel/gmod_impl_body.h:926`
- `0x483a60` `zDi::HasSpecialFlagsOrAuxMaterialData` -> `src/GameZRecoil/zModel/gdi_impl_body.h:382`
- `0x483ad0` `zDi::RebuildBounds` -> `src/GameZRecoil/zModel/gdi_impl_body.h:521`
- `0x483b80` `zDi::BuildAabb` -> `src/GameZRecoil/zModel/gdi_impl_body.h:405`
- `0x483e60` `zDi::BuildOriginSymmetricAabb` -> `src/GameZRecoil/zModel/gdi_impl_body.h:463`
- `0x483f80` `zDi::BuildBlendVertsFromConnectivity` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2920`
- `0x484140` `zDi::SetEntryValueForAllEntries` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2787`
- `0x484170` `zDi::SetShowBackFaceForAllEntries` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2805`
- `0x4841b0` `zDi::SetMaterialFlagBit9ForFlagBit0Entries` -> `src/GameZRecoil/zModel/gdi_impl_body.h:1015`
- `0x4841f0` `zDi::InvalidateImagesForFlagBit8Materials` -> `src/GameZRecoil/zModel/gdi_impl_body.h:1038`
- `0x484230` `zDi::ResetCurrentVariant` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2821`
- `0x484250` `zDi::SetCurrentVariantCycleTextureCount` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2837`
- `0x4842b0` `zDi::SetCurrentVariant` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2875`
- `0x4842f0` `zModel_Instance::SetCycleTextureLoop` -> `src/GameZRecoil/zModel/gmod_impl_body.h:3024`
- `0x484310` `zDi::SetCurrentVariantCycleTextureSpeed` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2901`
- `0x484330` `zModel_Instance::AddCycleTexture` -> `src/GameZRecoil/zModel/gmod_impl_body.h:3043`
- `0x484350` `zDi::SetObject3DColorModeForMaterials` -> `src/GameZRecoil/zModel/gmod_impl_body.h:2997`
- `0x4843b0` `zDi::RebuildGeneratedUvPairsForEntry` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1266`
- `0x484860` `zModel_Const::SolveTriScalarGradient2D` -> `src/GameZRecoil/zModel/gmod_impl_body.h:1711`
- `0x484960` `zDi::BuildPickCandidateForQueryPoint` -> `src/GameZRecoil/zClass/cls_di.c:3237`
- `0x484b70` `zModelConst::AddFaceToPlayerProbeSampleBuckets` -> `src/GameZRecoil/zClass/cls_di.c:3304`
- `0x484e00` `zClass_cls_di::PickTestMeshAtQueryXZ` -> `src/GameZRecoil/zClass/cls_di.c:3389`
- `0x484fc0` `zClass_cls_di::AppendPickCandidatesForFace` -> `src/GameZRecoil/zClass/cls_di.c:3142`
- `0x485380` `zClass_cls_di::BuildPickCandidatesForSegmentVsBBoxFaces` -> `src/GameZRecoil/zClass/cls_di.c:2558`
- `0x4856d0` `zClass_cls_di::TryGetPolygonHitAtQueryXZ` -> `src/GameZRecoil/zClass/cls_di.c:3027`
- `0x4857f0` `zClass_cls_di::BuildPickCandidateForSegmentVsPolygon` -> `src/GameZRecoil/zClass/cls_di.c:3072`
- `0x485d10` `zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv` -> `src/GameZRecoil/zClass/cls_di.c:3099`
- `0x486290` `zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygon` -> `src/GameZRecoil/zClass/cls_di.c:2818`
- `0x4869a0` `zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv` -> `src/GameZRecoil/zClass/cls_di.c:2907`
- `0x487350` `zClass_cls_di::FilterRegionsAgainstPolygon` -> `src/GameZRecoil/zClass/cls_di.c:2746`
- `0x487540` `zClass_cls_di::FilterRegionsAgainstPolygonWithDamageMaskUv` -> `src/GameZRecoil/zClass/cls_di.c:2629`
- `0x487900` `zClass_cls_di::FilterRegionsAgainstMeshFaces` -> `src/GameZRecoil/zClass/cls_di.c:3447`
- `0x4879c0` `zClass_cls_di::FilterRegionsAgainstHexahedronFaces` -> `src/GameZRecoil/zClass/cls_di.c:3481`
- `0x487a30` `zModel_Light_PointInPolygonInitXZ` -> `src/GameZRecoil/zModel/gmod_light.c:989`
- `0x487c50` `zModel_Light::PointInPolygonTestRadiusXZ` -> `src/GameZRecoil/zModel/gmod_light.c:1436`
- `0x487f10` `zModel_Light::SetActiveLights` -> `src/GameZRecoil/zModel/gmod_light.c:1068`
- `0x488d60` `zModel_Light::BuildLightWeights` -> `src/GameZRecoil/zModel/gmod_light.c:792`
- `0x4894f0` `zModel_Light::EvalDistanceWeight` -> `src/GameZRecoil/zModel/gmod_light.c:518`
- `0x489540` `zModel_Light::EvalSphereFogFade` -> `src/GameZRecoil/zModel/gmod_light.c:539`
- `0x4896d0` `zModel_Light::BuildAttr0DepthFade` -> `src/GameZRecoil/zModel/gmod_light.c:579`
- `0x489920` `zModel_Light::EvalBatchSphereFade` -> `src/GameZRecoil/zModel/gmod_light.c:750`
- `0x489a90` `zModel_Light::BuildAttr1Falloff` -> `src/GameZRecoil/zModel/gmod_light.c:667`
- `0x489e10` `zNetwork::ShutdownSessionRuntime` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2721`
- `0x489f30` `zNetwork::ClearEnumeratedSessionList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2559`
- `0x489fa0` `zNetwork::ClearServiceProviderList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2579`
- `0x48a030` `zNetwork::ClearPlayerRecordList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2611`
- `0x48a140` `zNetworkDPlay::InitializeConnectionFromProviderInfo` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:928`
- `0x48a180` `zNetworkDPlay::SelectServiceProviderAndInitConnection` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:888`
- `0x48a220` `zNetwork_DPlay::EnumSessions` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1155`
- `0x48a2c0` `zNetworkDPlay::GetEnumeratedSessionNameByIndex` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:957`
- `0x48a2e0` `zNetworkDPlay::GetEnumeratedSessionPlayerCountsByIndex` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:976`
- `0x48a310` `zNetwork_DPlay::EnumPlayers` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1199`
- `0x48a350` `zNetworkDPlay::QueryCapsAndConfigureSendMode` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1025`
- `0x48a410` `zNetwork_DPlay::CreateSessionFromStatusFields` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1223`
- `0x48a980` `zNetwork_DPlay_DestroyCachedLocalPlayer` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:689`
- `0x48ae70` `zNetworkDPlay::ReceivePendingMessages` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1657`
- `0x48afa0` `zNetwork::GetPlayerNameByKey` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:491`
- `0x48afe0` `zNetworkDPlay::PumpIncomingMessages` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1731`
- `0x48b5e0` `zNetworkDPlay::EnumSessionCallback_AddSessionDescCache` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:996`
- `0x48b660` `zNetworkDPlay::EnumPlayerCallback_AddPlayerRecord` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1110`
- `0x48b730` `zNetwork_DPlay::CreateInterfaceAndCoInitialize` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1874`
- `0x48b7f0` `zNetwork_DPlay::CloseReleaseAndCoUninitialize` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1932`
- `0x48b820` `zNetwork_ApplyPkt01_PlayerColorAssignments` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2174`
- `0x48b860` `zNetwork::HostSendPlayerColorAssignmentsPacket` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:750`
- `0x48b940` `zNetwork::AllocFreePlayerColorIndex` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:733`
- `0x48b9e0` `zNetwork::RemovePlayerRecordByKey` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1074`
- `0x48ba60` `zNetwork_FindPlayerRecordByKey` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:712`
- `0x48bee0` `zNetworkDPlay::FreeServiceProviderInfoBuffers` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2198`
- `0x48bfa0` `zNetwork_InitMessageHandlers` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2165`
- `0x48bfb0` `zNetwork_CreateEmptyDispatchHandlerList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2145`
- `0x48bfe0` `zNetwork_RegisterDispatchHandlerListShutdown` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2137`
- `0x48bff0` `zNetwork_DestroyDispatchHandlerList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2110`
- `0x48c120` `zNetwork::UnregisterPacketHandler` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2272`
- `0x48c200` `zNetwork_DPlay::DispatchPacketToHandlers` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2334`
- `0x48c250` `zNetwork_DPlay_ReportError` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2363`
- `0x48c7d0` `zUtil::ZRDR_PreallocNodePool` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:242`
- `0x48c800` `zUtil_ZRDR_GrowFreePool` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:231`
- `0x48c820` `zUtil_ZRDR_PushFreeNode` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:202`
- `0x48c890` `zUtil_ZRDR_FreeNodePool` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1009`
- `0x48c8e0` `zUtil_ZRDR_PopFreeNode` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:261`
- `0x48c950` `zArchiveList_CreateEmpty` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:171`
- `0x48c970` `zArchiveList_Destroy` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:185`
- `0x48c9a0` `zArchiveList_LinkNodeBetween` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:156`
- `0x48c9c0` `zArchiveList_PushFrontPayload` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:307`
- `0x48ca10` `zUtil_ZRDR_AllocNodeWithPayload` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:295`
- `0x48ca30` `zArchiveList_PushBackPayload` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:338`
- `0x48ca70` `zArchiveList_RemovePayload` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:368`
- `0x48cae0` `zArchiveList_FreeNode` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:407`
- `0x48cb00` `zArchiveList_FindNodeByPayload` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:424`
- `0x48cb30` `zArchiveList_GetAt` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:493`
- `0x48cb70` `zArchiveList_PopFrontPayload` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:454`
- `0x48cbd0` `zArchiveList_FindPayloadByPredicate` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:520`
- `0x48cc20` `zArchiveList_FindPayloadByValue` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:559`
- `0x48cc50` `zArchiveList_FindPayloadByPredicate_Thunk` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:590`
- `0x48cc60` `zArchiveList_GetCount` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:479`
- `0x48cc70` `zUtil_ZRDR_Init` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:92`
- `0x48cc70` `zUtil::ZRDR_Init` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1068`
- `0x48cca0` `zUtil_ZRDR_SetSearchPath` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1028`
- `0x48cce0` `zUtil_ZRDR_AppendSearchPath` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1048`
- `0x48cd10` `zUtil_ZRDR_Shutdown` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:996`
- `0x48cd40` `zReader_TryResolvePath` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:108`
- `0x48cd40` `zReader::TryResolvePath` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1786`
- `0x48cda0` `zReader_AllocateNode` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:144`
- `0x48cdc0` `zReader_LoadNodeFromPath` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:158`
- `0x48ce40` `zReader_FreeLoadedTree` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:197`
- `0x48ce60` `zReader_FreeNodeRecursive` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:210`
- `0x48cec0` `zReader_FindChildRecursive` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:234`
- `0x48cf70` `zReader_GetCurrentRootNode` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:279`
- `0x48cf80` `zReader_ReadNamedString` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:295`
- `0x48cf80` `zReader::ReadNamedString` -> `src/GameZRecoil/zReader/zreader_lookup_impl_body.h:116`
- `0x48cfb0` `zReader_ReadNamedFloat` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:325`
- `0x48cfb0` `zReader::ReadNamedFloat` -> `src/GameZRecoil/zReader/zreader_lookup_impl_body.h:148`
- `0x48d030` `zReader_ReadNamedInt` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:369`
- `0x48d030` `zReader::ReadNamedInt` -> `src/GameZRecoil/zReader/zreader_lookup_impl_body.h:193`
- `0x48d080` `zReader_ReadNode` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:404`
- `0x48d1c0` `zReader_OpenFileFromMountedArchives` -> `src/GameZRecoil/zReader/zreader_legacy_impl_body.h:482`
- `0x48d210` `zArchive::MountIndexArchive` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1534`
- `0x48d2c0` `zUtil_ZRDR_UnloadMountedArchives` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:962`
- `0x48d340` `zVid::Noise_InitBuffers` -> `src/GameZRecoil/zVideo/zvid_main.c:5919`
- `0x48d420` `zVideo::Fx_SetSurfaceState` -> `src/GameZRecoil/zVideo/zvid_main.c:4210`
- `0x48d450` `zRndr::OverlayBlendRow555_Scalar` -> `src/GameZRecoil/zRender/zrndr_draw.c:1837`
- `0x48d4b0` `zRndr::OverlayBlendRow565_Scalar` -> `src/GameZRecoil/zRender/zrndr_draw.c:1867`
- `0x48d510` `zRndr::OverlayBlendRow555_Mmx` -> `src/GameZRecoil/zRender/zrndr_draw.c:1894`
- `0x48d510` `zRndr::OverlayBlendRow555_Mmx` -> `src/GameZRecoil/zRender/zrndr_draw.c:1907`
- `0x48d510` `zRndr::OverlayBlendRow555_Mmx` -> `src/GameZRecoil/zRender/zrndr_draw.c:1987`
- `0x48d5f0` `zRndr::OverlayBlendRow565_Mmx` -> `src/GameZRecoil/zRender/zrndr_draw.c:2020`
- `0x48d5f0` `zRndr::OverlayBlendRow565_Mmx` -> `src/GameZRecoil/zRender/zrndr_draw.c:2032`
- `0x48d5f0` `zRndr::OverlayBlendRow565_Mmx` -> `src/GameZRecoil/zRender/zrndr_draw.c:2112`
- `0x48da60` `zVideo::FxPass3_CopySurfacePixelToScratchClipped` -> `src/GameZRecoil/zVideo/zvid_main.c:4227`
- `0x48daf0` `zVideo::FxPass3_ApplyToCurrentSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:4518`
- `0x48e380` `zVideo::buff_BlurRegionCombined` -> `src/GameZRecoil/zVideo/zvid_main.c:4677`
- `0x48e670` `zVideo::buff_BlurRegionVertical` -> `src/GameZRecoil/zVideo/zvid_main.c:4822`
- `0x48e870` `zVideo::buff_BlurRegionHorizontal` -> `src/GameZRecoil/zVideo/zvid_main.c:4929`
- `0x48ea00` `zVideo::buff_BlurRegionByMode` -> `src/GameZRecoil/zVideo/zvid_main.c:5027`
- `0x48f500` `zVid_Image::BlitToActiveTarget` -> `src/GameZRecoil/zVideo/zvid_main.c:6628`
- `0x48f560` `zVid_Image::BlitToFramebufferClipped` -> `src/GameZRecoil/zVideo/zvid_main.c:6660`
- `0x48fd80` `zRndr::InitGlobals` -> `src/GameZRecoil/zRender/zrndr_draw.c:1765`
- `0x48ff80` `zRndr::SelectSpanRoutines` -> `src/GameZRecoil/zRender/zrndr_draw.c:2186`
- `0x490330` `zFloat::Set255f` -> `src/GameZRecoil/zMath/zmth_main.c:2496`
- `0x490340` `zRndr::SetFrameBufferRegion` -> `src/GameZRecoil/zRender/zrndr_draw.c:2324`
- `0x4903c0` `zRndr::SetActiveRegionSizeFromRect` -> `src/GameZRecoil/zRender/zrndr_draw.c:2352`
- `0x4903f0` `zRndr::GetActiveRegionState` -> `src/GameZRecoil/zRender/zrndr_draw.c:2304`
- `0x490480` `zRndr::SetPerspectiveAdaptiveSpanParams` -> `src/GameZRecoil/zRender/zrndr_draw.c:1751`
- `0x4904d0` `zRndr::SetPerspectiveAdaptiveCorrection` -> `src/GameZRecoil/zRender/zrndr_draw.c:1734`
- `0x494af0` `Renderer_DrawPolyTLV` -> `src/GameZRecoil/zRender/zrndr_draw.c:11953`
- `0x495850` `zRndr_DrawTexturedQueued` -> `src/GameZRecoil/zRender/zrndr_draw.c:11726`
- `0x4969d0` `zRndr_DrawTexturedQueuedAlpha` -> `src/GameZRecoil/zRender/zrndr_draw.c:12161`
- `0x497ac0` `zRndr_DrawTexturedFanTri` -> `src/GameZRecoil/zRender/zrndr_draw.c:12364`
- `0x498bd0` `zRndr_DrawImmediateLine` -> `src/GameZRecoil/zRender/zrndr_draw.c:12566`
- `0x498c00` `zRndr_DrawClippedImmediateLineStrip` -> `src/GameZRecoil/zRender/zrndr_draw.c:12588`
- `0x498cb0` `zRndr::LensFlare_DrawQueuedSample16_ClippedFramebuffer` -> `src/GameZRecoil/zRender/zrndr_draw.c:8397`
- `0x4992b0` `zRndr_PlotPixel16` -> `src/GameZRecoil/zRender/zrndr_draw.c:9110`
- `0x4992d0` `zRndr_DrawLine16` -> `src/GameZRecoil/zRender/zrndr_draw.c:9124`
- `0x4993a0` `zRndr_DrawLine16_Segmented` -> `src/GameZRecoil/zRender/zrndr_draw.c:9187`
- `0x499500` `zRndr_DrawLine16_Clipped` -> `src/GameZRecoil/zRender/zrndr_draw.c:9274`
- `0x4997d0` `zRndr_FillSpan16Opaque` -> `src/GameZRecoil/zRender/zrndr_draw.c:9413`
- `0x499810` `zRndr_FillSpan555Solid` -> `src/GameZRecoil/zRender/zrndr_draw.c:9437`
- `0x4998a0` `zRndr_FillSpan565Solid` -> `src/GameZRecoil/zRender/zrndr_draw.c:9475`
- `0x49a2b0` `zRndr_FlushTransparentQueue` -> `src/GameZRecoil/zRender/zrndr_draw.c:11210`
- `0x49a490` `zRndr_FlushOverwriteQueue` -> `src/GameZRecoil/zRender/zrndr_draw.c:11310`
- `0x49a830` `zRndr_LensFlare_QueueProjectedSample` -> `src/GameZRecoil/zRender/zrndr_draw.c:12621`
- `0x49a8b0` `zRndr_LensFlare_GetQueuedSampleCount` -> `src/GameZRecoil/zRender/zrndr_draw.c:12647`
- `0x49a8c0` `zRndr::LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer` -> `src/GameZRecoil/zRender/zrndr_draw.c:8470`
- `0x49a910` `zRndr::LensFlare_ResetSampleQueue` -> `src/GameZRecoil/zRender/zrndr_draw.c:8314`
- `0x49a920` `zRndr_LensFlare_DrawQueuedSamples16_AndBuildVisibleList` -> `src/GameZRecoil/zRender/zrndr_draw.c:12655`
- `0x49a9c0` `zRndr_LensFlare::BuildVisibleSampleListFromQueue` -> `src/GameZRecoil/zRender/zrndr_draw.c:12697`
- `0x49aa30` `zRndr_SpanOcclusion_FilterSampleList` -> `src/GameZRecoil/zRender/zrndr_draw.c:13023`
- `0x49aa40` `zRndr_LensFlare_SetVisibleSampleStage` -> `src/GameZRecoil/zRender/zrndr_draw.c:12729`
- `0x49afb0` `zRndr_LensFlare_DrawVisibleSample` -> `src/GameZRecoil/zRender/zrndr_draw.c:12972`
- `0x49b1a0` `zRndr_LensFlare_DrawVisibleSamples` -> `src/GameZRecoil/zRender/zrndr_draw.c:13007`
- `0x49b1e0` `zRndr::FogColor_SetRgb01Clamped` -> `src/GameZRecoil/zRender/zrndr_draw.c:8159`
- `0x49b350` `zRndr::SetFogTargetColorRgb01Clamped` -> `src/GameZRecoil/zRender/zrndr_draw.c:8209`
- `0x49b4c0` `zRndr::CommitDirectFogParamsIfChanged` -> `src/GameZRecoil/zRender/zrndr_draw.c:8259`
- `0x49b530` `zRndr::CommitFogColorParamsIfChanged` -> `src/GameZRecoil/zRender/zrndr_draw.c:8267`
- `0x49b5a0` `zRndr_FogTargetColorStaged_SetRgb01Clamped` -> `src/GameZRecoil/zRender/zrndr_draw.c:13040`
- `0x49b710` `zRndr::CommitStagedFogParamsIfChanged` -> `src/GameZRecoil/zRender/zrndr_draw.c:8275`
- `0x49b7e0` `zRndr::SpanMasked16FromTex16SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:5913`
- `0x49b7e0` `zRndr::SpanMasked16FromTex16SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:6250`
- `0x49bbf0` `zRndr::SpanMasked16FromPal8SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:7140`
- `0x49bbf0` `zRndr::SpanMasked16FromPal8SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:7493`
- `0x49c020` `zRndr::SpanMasked16FromPal8To565` -> `src/GameZRecoil/zRender/zrndr_draw.c:2990`
- `0x49c150` `zRndr::SpanMasked16FromTex16To565` -> `src/GameZRecoil/zRender/zrndr_draw.c:3050`
- `0x49c230` `zRndr::SpanAlphaBlend565ConstAlphaFromPal8` -> `src/GameZRecoil/zRender/zrndr_draw.c:2946`
- `0x49c360` `zRndr::SpanAlphaBlend565FromTex16Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:3084`
- `0x49c560` `zRndr::SpanAlphaBlend555FromTex16Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:3170`
- `0x49c760` `zRndr::SpanAlphaBlend565ConstAlphaFromTex16` -> `src/GameZRecoil/zRender/zrndr_draw.c:4492`
- `0x49c860` `zRndr::SpanAlphaBlend555ConstAlphaFromTex16` -> `src/GameZRecoil/zRender/zrndr_draw.c:4534`
- `0x49c970` `zRndr::SpanAlphaBlend565ConstAlphaFromTex16Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:3255`
- `0x49ca90` `zRndr::SpanAlphaBlend555ConstAlphaFromTex16Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:3307`
- `0x49cbb0` `zRndr::SpanAlphaBlend565MmxFromTex16Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:3357`
- `0x49cea0` `zRndr::SpanAlphaBlend555MmxFromTex16Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:3570`
- `0x49d1a0` `zRndr::SpanAlphaBlend565FromPal8Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:3782`
- `0x49d3b0` `zRndr::SpanAlphaBlend555FromPal8Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:3864`
- `0x49d5c0` `zRndr::SpanAlphaBlend565ConstAlphaFastFromPal8` -> `src/GameZRecoil/zRender/zrndr_draw.c:4575`
- `0x49d6e0` `zRndr::SpanAlphaBlend555ConstAlphaFastFromPal8` -> `src/GameZRecoil/zRender/zrndr_draw.c:4617`
- `0x49d810` `zRndr::SpanAlphaBlend565ConstAlphaFromPal8Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:3946`
- `0x49d950` `zRndr::SpanAlphaBlend555ConstAlphaFromPal8Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:3999`
- `0x49da80` `zRndr::SpanAlphaBlend565MmxFromPal8Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:4051`
- `0x49ddb0` `zRndr::SpanAlphaBlend555MmxFromPal8Alpha8` -> `src/GameZRecoil/zRender/zrndr_draw.c:4272`
- `0x49e0e0` `zRndr::FogTarget565_SetPackedColorAndRamp` -> `src/GameZRecoil/zRender/zrndr_draw.c:2883`
- `0x49e140` `zRndr::SpanMmxSetPixelFormatMasks` -> `src/GameZRecoil/zRender/zrndr_draw.c:2145`
- `0x49e200` `zRndr::FogBlendSpan565Scalar` -> `src/GameZRecoil/zRender/zrndr_draw.c:5069`
- `0x49e300` `zRndr::FogBlendSpan555Scalar` -> `src/GameZRecoil/zRender/zrndr_draw.c:5109`
- `0x49e400` `zRndr::FogBlendSpan565Mmx` -> `src/GameZRecoil/zRender/zrndr_draw.c:5149`
- `0x49e560` `zRndr::FogBlendSpan555Mmx` -> `src/GameZRecoil/zRender/zrndr_draw.c:5314`
- `0x49e6c0` `zRndr::SpanCopy16FromTex16SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:5490`
- `0x49e6c0` `zRndr::SpanCopy16FromTex16SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:5763`
- `0x49ea40` `zRndr::SpanMmxSetTexUvMasksAndVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:6427`
- `0x49ea80` `zRndr::SpanCopy16FromTex16` -> `src/GameZRecoil/zRender/zrndr_draw.c:6445`
- `0x49ec20` `zRndr::SpanCopy16FromTex16ExplicitVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:6558`
- `0x49edc0` `zRndr::SpanCopy16FromPal8SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:6683`
- `0x49edc0` `zRndr::SpanCopy16FromPal8SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:6972`
- `0x49f180` `zRndr::SpanShade16FromPal8SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:7661`
- `0x49f180` `zRndr::SpanShade16FromPal8SwitchVShift` -> `src/GameZRecoil/zRender/zrndr_draw.c:7998`
- `0x49f620` `zSnd::Tick` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:375`
- `0x49f6d0` `zSndSample::AcquirePlayHandleDispatch` -> `src/GameZRecoil/zSound/zsnd_play.cpp:405`
- `0x49f6f0` `zSndSample::AcquireA3dVoice` -> `src/GameZRecoil/zSound/zsnd_play.cpp:492`
- `0x49f830` `zSndSample::AcquireVoice` -> `src/GameZRecoil/zSound/zsnd_play.cpp:421`
- `0x49f960` `zSndSample::PlayA3DSimple` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1846`
- `0x49f9a0` `zSnd::GainScaleToDirectSoundAttenuation` -> `src/GameZRecoil/zSound/zsnd_play.cpp:575`
- `0x49fa00` `zSndSample_PlaySimple` -> `src/GameZRecoil/zSound/zsnd_play.cpp:696`
- `0x49fa10` `zSndSample::PlayOnActiveBackend` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1606`
- `0x49fa60` `zSndSample::PlayOnA3D` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1637`
- `0x49fbb0` `zSndSample::PlayOnDirectSound` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1712`
- `0x49fcf0` `zSndSample::PlayA3D` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1780`
- `0x49fd50` `zSndSample::PlayDirectSound` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1816`
- `0x49fda0` `zSndPlayHandle::StopIfActive` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1119`
- `0x49fec0` `zSndSample::StopActiveVoicesIfPlaying` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1212`
- `0x49fff0` `zSndPlayHandleSnapshot::CreateFromActiveSamples` -> `src/GameZRecoil/zSound/zsnd_play.cpp:834`
- `0x4a0300` `zSndPlayHandleSnapshotPayload::CaptureFromPlayHandle` -> `src/GameZRecoil/zSound/zsnd_play.cpp:769`
- `0x4a0380` `zSndPlayHandle::PlayWithDelta_A3D` -> `src/GameZRecoil/zSound/zsnd_play.cpp:930`
- `0x4a0400` `zSndPlayHandle::PlayWithDelta_DirectSound` -> `src/GameZRecoil/zSound/zsnd_play.cpp:968`
- `0x4a0490` `zSndPlayHandle::PlayWithDelta_BackendDispatch` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1019`
- `0x4a0500` `zSndPlayHandleSnapshot::StopAllIfPlaying` -> `src/GameZRecoil/zSound/zsnd_play.cpp:707`
- `0x4a0590` `zSndPlayHandleSnapshot::RestoreAllWithGlobalVolumeDelta` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1057`
- `0x4a05f0` `zSndPlayHandleSnapshot::Destroy` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1086`
- `0x4a0670` `zSnd::ApplyMuteStateToActiveVoices` -> `src/GameZRecoil/zSound/zsnd_play.cpp:643`
- `0x4a07a0` `zSnd::IsMuted` -> `src/GameZRecoil/zSound/zsnd_play.cpp:594`
- `0x4a07c0` `zSndPlayHandleSnapshot::NewNode` -> `src/GameZRecoil/zSound/zsnd_play.cpp:751`
- `0x4a07f0` `zSnd::SetUseArchiveBanksFlag` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:203`
- `0x4a0800` `zSnd_SetUseArchiveBanksAndRegisterAtExit` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:130`
- `0x4a0810` `zSnd_SetUseArchiveBanks` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:98`
- `0x4a0830` `zSndSampleSetRegistry_RegisterAtExit` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:122`
- `0x4a0840` `zSndSampleSetRegistry_Shutdown` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:111`
- `0x4a0860` `zSndSampleSet_InitByName` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:200`
- `0x4a0870` `zSndSampleSet_DestroyByName` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:190`
- `0x4a0880` `zSndSampleSetRegistry_DestroyAll` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:460`
- `0x4a08d0` `zSndSampleSetRegistry_GetByIndex` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:149`
- `0x4a0900` `zSndSampleSetRegistry_GetCount` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:141`
- `0x4a0920` `zSndSampleSetRegistry_FindByName` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:168`
- `0x4a09e0` `zSndSampleSet::RegistryAddEntry` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:211`
- `0x4a0c00` `zSndSampleSet::DestroyOwnedData` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:445`
- `0x4a0c40` `zSndSampleSet::Init` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:319`
- `0x4a0e40` `zSndSampleSet::Destroy` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:429`
- `0x4a0e90` `zSndSampleSet::GetSampleAt` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:234`
- `0x4a0fb0` `zSndSampleSet::LoadSamplesFromIndexArchive` -> `src/GameZRecoil/zSound/zsnd_sample_set_impl.h:273`
- `0x4a1090` `zSnd::SetGlobalVolumeScale` -> `src/GameZRecoil/zSound/zsnd_play.cpp:606`
- `0x4a10b0` `zSnd::MulGlobalVolumeScaleAndGetPrev` -> `src/GameZRecoil/zSound/zsnd_play.cpp:620`
- `0x4a10d0` `zSnd::SetFlag10PlaybackEnabled` -> `src/GameZRecoil/zSound/zsnd_play.cpp:633`
- `0x4a10e0` `zSndPlayHandle::SetFreqScaled` -> `src/GameZRecoil/zSound/zsnd_parm.cpp:31`
- `0x4a11d0` `zSndPlayHandle::SetEnableScale` -> `src/GameZRecoil/zSound/zsnd_parm.cpp:85`
- `0x4a1240` `zSndSample::SetPlaybackEventHandler` -> `src/GameZRecoil/zSound/zsnd_parm.cpp:124`
- `0x4a1250` `zSndPlayHandle_TryEnableManaged` -> `src/GameZRecoil/zSound/zsnd_parm.cpp:139`
- `0x4a1270` `zSndPlayHandle_TryDisableManaged` -> `src/GameZRecoil/zSound/zsnd_parm.cpp:157`
- `0x4a1290` `zSnd::SetActiveBackendPreInit` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:141`
- `0x4a12b0` `zSnd::GetActiveBackend` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:156`
- `0x4a12c0` `zSnd_PreInitializeRuntimeState` -> `src/GameZRecoil/zSound/zsnd_init.cpp:868`
- `0x4a13d0` `zSndSystem::Shutdown` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:747`
- `0x4a1420` `zSndSystem_Init` -> `src/GameZRecoil/zSound/zsnd_init.cpp:926`
- `0x4a1510` `zSndSystem_InitLegacySetsSyntax` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:774`
- `0x4a1870` `zSndSystem_InitNamedSetsSyntax` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:436`
- `0x4a1d10` `zSndBackend_InitA3D` -> `src/GameZRecoil/zSound/zsnd_init.cpp:1056`
- `0x4a1e50` `zSndBackend_InitDirectSound` -> `src/GameZRecoil/zSound/zsnd_init.cpp:993`
- `0x4a1f40` `zSndBackend::Shutdown` -> `src/GameZRecoil/zSound/zsnd_init.cpp:1169`
- `0x4a2010` `zSndCdTrackList::StaticInit` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:121`
- `0x4a2020` `zSndCdTrackList::StaticConstructor` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:80`
- `0x4a2050` `zSndCdTrackList::RegisterAtExitDestructor` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:113`
- `0x4a2060` `zSndCdTrackList::StaticDestructor` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:92`
- `0x4a20d0` `zSndCd::Init` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:244`
- `0x4a2490` `zSndCd::ResetTrackState` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:416`
- `0x4a24d0` `zSndCd::Shutdown` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:689`
- `0x4a25e0` `zSndCd::PlayTrackWithMode` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:624`
- `0x4a2600` `zSndCd::ApplyPlaybackMode` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:524`
- `0x4a26b0` `zSndCd::OnMciNotify` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:641`
- `0x4a26f0` `zSndCd::Stop` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:660`
- `0x4a2750` `zSndCd::PlayTrack` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:589`
- `0x4a27d0` `zSndCd::IsStereoAuxEnabled` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:428`
- `0x4a27f0` `zSndCd::GetVolume` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:444`
- `0x4a2880` `zSndCd::SetVolume` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:487`
- `0x4a2930` `zSndCd::GetTrackCount` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:577`
- `0x4a2950` `zSnd_UpdateListenerState` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1327`
- `0x4a2a30` `zSndPlayHandle::Update3DDispatch` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1409`
- `0x4a2a70` `zSndPlayHandle::Update3D_A3D` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1554`
- `0x4a2b40` `zSndPlayHandle::Update3D` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1437`
- `0x4a2ea0` `zSndSample::InitFromWaveData` -> `src/GameZRecoil/zSound/zsnd_create.cpp:533`
- `0x4a2ec0` `zSndSample::InitFromWaveData_A3D` -> `src/GameZRecoil/zSound/zsnd_create.cpp:245`
- `0x4a3180` `zSndSample::InitFromWaveData_DirectSound` -> `src/GameZRecoil/zSound/zsnd_create.cpp:47`
- `0x4a34e0` `zSndSample::LockBackendBuffers` -> `src/GameZRecoil/zSound/zsnd_create.cpp:409`
- `0x4a3590` `zSndSample::UnlockBackendBuffers` -> `src/GameZRecoil/zSound/zsnd_create.cpp:477`
- `0x4a3620` `zSndSample::GetPlayCursorBytes` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1295`
- `0x4a3690` `zSndSample::DestroyOwnedData` -> `src/GameZRecoil/zSound/zsnd_play.cpp:323`
- `0x4a3850` `zSndSample_CreateQueuedStreamingSample` -> `src/GameZRecoil/zSound/zsnd_create.cpp:559`
- `0x4a3910` `zSndSample::Destroy` -> `src/GameZRecoil/zSound/zsnd_play.cpp:369`
- `0x4a3930` `zSndFadeLists::Init` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:163`
- `0x4a3940` `zSndFadeLists::InitGlobals` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:85`
- `0x4a39a0` `zSndFadeLists::RegisterShutdownAtExit` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:155`
- `0x4a39b0` `zSndFadeLists::ShutdownAtExit` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:102`
- `0x4a3a80` `zSndFadeDispatchList::PushBack` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:174`
- `0x4a3ad0` `zSndFadeEntry::UpdateAndQueueCompletion` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:208`
- `0x4a3c20` `zSndFadeActiveList::TickAll` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:279`
- `0x4a3d20` `zSndFadeLists::StopAllAndShutdown` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:676`
- `0x4a3e50` `zSndFadeList::DeleteNodeAndAdvanceCursor` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:341`
- `0x4a3e90` `zSndFadeListCursor::PopFrontCursor` -> `src/GameZRecoil/zSound/zsnd_fade.cpp:358`
- `0x4a3ea0` `zSnd::ReportMciError` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:214`
- `0x4a3ef0` `zSnd::ReportA3DError` -> `src/GameZRecoil/zSound/zsnd_init.cpp:296`
- `0x4a4330` `zSnd::ReportDirectSoundError` -> `src/GameZRecoil/zSound/zsnd_init.cpp:679`
- `0x4a4530` `zSndGroup_QueuePendingLoadsFromConfigNode` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:981`
- `0x4a4590` `zSndGroup_LoadFromConfigNode` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:820`
- `0x4a49b0` `zSndGroup_LoadConfigBlock` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:695`
- `0x4a4c40` `zSndStreamMgr::UpdateActiveRequestPredicate` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:147`
- `0x4a4cb0` `zSndStreamRequest::StateBeginGroup` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:326`
- `0x4a4d10` `zSndGroup::SelectWeightedEntry` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:250`
- `0x4a4ea0` `zSndStreamRequest::StatePlayCurrentEntry` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:347`
- `0x4a4fd0` `zSndStreamRequest::StateWaitRepeatDelay` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:417`
- `0x4a5020` `zSndStreamRequest::StateWaitTerminationDelay` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:311`
- `0x4a5050` `zSndStreamMgr::RecycleFinishedRequest` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:667`
- `0x4a50a0` `zSndStreamMgr::Shutdown` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:548`
- `0x4a51e0` `zSndStreamRequest::MatchRequestPredicate` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:132`
- `0x4a51f0` `zSndStreamRequest::StopIfActive` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:183`
- `0x4a5220` `zSndStreamRequest_MatchGroupPredicate` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:204`
- `0x4a5230` `zSndGroup::QueueStreamRequestSimple` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:635`
- `0x4a5250` `zSndGroup::QueueStreamRequest` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:570`
- `0x4a5350` `zSndStreamMgr_EnsureInit` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:433`
- `0x4a53d0` `zSndGroup::QueueStreamRequestWithWorldPos` -> `src/GameZRecoil/zSound/zsnd_grp.cpp:650`
- `0x4a53f0` `zSndWaveData::zSndWaveData` -> `src/GameZRecoil/zSound/zsnd.cpp:18`
- `0x4a5440` `zSndWaveData::Destructor` -> `src/GameZRecoil/zSound/zsnd.cpp:43`
- `0x4a5460` `zSndWaveData::ParseLoadedWaveFile` -> `src/GameZRecoil/zSound/zsnd.cpp:55`
- `0x4a5540` `zSndWaveData::LoadAndParseIfNeeded` -> `src/GameZRecoil/zSound/zsnd.cpp:119`
- `0x4a55c0` `zSndWaveData::Reset` -> `src/GameZRecoil/zSound/zsnd.cpp:196`
- `0x4a5600` `zSndWaveData::LoadAndParseFromIndexArchiveIfNeeded` -> `src/GameZRecoil/zSound/zsnd.cpp:159`
- `0x4a5670` `Time::Reset` -> `src/GameZRecoil/zSys/zsys_time_impl.h:48`
- `0x4a5780` `RecoilApp::InitStdLogFiles` -> `src/Battlesport/RecoilApp.cpp:27`
- `0x4a5980` `zSys::ExitProcessWithCleanup` -> `src/GameZRecoil/zSys/zSys.cpp:1387`
- `0x4a59e0` `zSys::FindFileOnDriveType` -> `src/GameZRecoil/zSys/zSys.cpp:172`
- `0x4a5ad0` `zLoc::LoadMessagesDll` -> `src/GameZRecoil/zSys/zsys_zloc_impl.h:20`
- `0x4a5b00` `zLoc::UnloadMessagesDll` -> `src/GameZRecoil/zSys/zsys_zloc_impl.h:41`
- `0x4a5b20` `zLoc::GetMessageId` -> `src/GameZRecoil/zSys/zsys_zloc_impl.h:54`
- `0x4a5b40` `zLoc::ResolveMessageKeyOrFallback` -> `src/GameZRecoil/zSys/zsys_zloc_impl.h:68`
- `0x4a5b60` `zLoc::FormatMessage` -> `src/GameZRecoil/zSys/zsys_zloc_impl.h:83`
- `0x4a5bf0` `zLoc::GetMessageString` -> `src/GameZRecoil/zSys/zsys_zloc_impl.h:125`
- `0x4a5c20` `zReader::FileExists` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1771`
- `0x4a5c40` `zReader_FileExists_Wrapper` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1950`
- `0x4a5c50` `zUtil::ZRDR_GetFileSize` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:642`
- `0x4a5ca0` `zUtil_ZRDR_CreateSearchPathList` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:893`
- `0x4a5cc0` `zUtil_ZRDR_FreeSearchPathList` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:929`
- `0x4a5ce0` `zUtil::ZRDR_AddSearchPaths` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:836`
- `0x4a5da0` `zUtil_ZRDR_StrCmpPredicate` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:606`
- `0x4a5df0` `zUtil_ZRDR_FreeScratchSearchPathList` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:941`
- `0x4a5e10` `zUtil_ZRDR_FreePathList` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:908`
- `0x4a5e50` `zUtil_ZRDR_ResolvePathInSearchPathList` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:670`
- `0x4a5f20` `zUtil_ZRDR_SearchPathContainsFilePredicate` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:624`
- `0x4a5f50` `zUtil_ZRDR_OpenFileResolved` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:727`
- `0x4a5f90` `zUtil_ZRDR::InitWildcardPath` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:766`
- `0x4a6070` `zUtil_ZRDR::NextWildcardPath` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:805`
- `0x4a6100` `zUtil_ZRDR_ShutdownWildcardPath` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:953`
- `0x4a6190` `zIndexArchive::Reset` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1125`
- `0x4a61b0` `zIndexArchive::Destroy` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1139`
- `0x4a61d0` `zIndexArchive::Init` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1150`
- `0x4a6270` `zIndexArchive::OpenCreateWrite` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1199`
- `0x4a62b0` `zIndexArchive::CloseAndFreeRecords` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1221`
- `0x4a62f0` `zIndexArchive::EnsureCapacity` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1363`
- `0x4a6330` `zIndexArchive::FreeRecordsAndReset` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1240`
- `0x4a6360` `zIndexArchive::FlushIndexToTail` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1258`
- `0x4a63f0` `zIndexArchive::LoadIndexFromTail` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1297`
- `0x4a64d0` `zIndexArchive::AddFileRecord` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1387`
- `0x4a65d0` `zIndexArchive::FindRecordByNameCI` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1450`
- `0x4a6630` `zIndexArchive::OpenFileByName` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1470`
- `0x4a6670` `zIndexArchive::ReadFileByName` -> `src/GameZRecoil/zReader/zreader_load_impl_body.h:1496`
- `0x4a66f0` `zVideo::Init_ApplyModeIndex` -> `src/GameZRecoil/zVideo/zvid_main.c:4097`
- `0x4a6760` `zVideo::CallClearSwSurfaceAndZBuffer` -> `src/GameZRecoil/zVideo/zvid_main.c:4129`
- `0x4a67d0` `zVideo::Dispatch_UnlockSwSurfaceState` -> `src/GameZRecoil/zVideo/zvid_main.c:4194`
- `0x4a67f0` `zVideo::GetPrimarySurfacePixels` -> `src/GameZRecoil/zVideo/zvid_main.c:3873`
- `0x4a6830` `zVideo::CallClearPrimarySurfaceAndZBuffer` -> `src/GameZRecoil/zVideo/zvid_main.c:4144`
- `0x4a6840` `zVideo::RunPostprocessOnPrimaryBuffer` -> `src/GameZRecoil/zVideo/zvid_main.c:5260`
- `0x4a68d0` `zVideo::Dispatch_UnlockPrimarySurfaceState` -> `src/GameZRecoil/zVideo/zvid_main.c:4202`
- `0x4a68e0` `zVideo::Dispatch_LockDisplayModeSurfaceState` -> `src/GameZRecoil/zVideo/zvid_main.c:4178`
- `0x4a68f0` `zVideo::Dispatch_UnlockDisplayModeSurfaceState` -> `src/GameZRecoil/zVideo/zvid_main.c:4186`
- `0x4a69c0` `zVideo_buff::ClipCoordToRange` -> `src/GameZRecoil/zVideo/zvid_main.c:3363`
- `0x4a69e0` `zVideo_buff::BltSourceToPrimaryClipped` -> `src/GameZRecoil/zVideo/zvid_main.c:3499`
- `0x4a6b80` `zVideo::SetClearColorPacked16` -> `src/GameZRecoil/zVideo/zvid_main.c:1728`
- `0x4a6bb0` `zVideo::PixelPack_GetRgbMasks` -> `src/GameZRecoil/zVideo/zvid_main.c:5888`
- `0x4a6ca0` `zVid_PackColor00RRGGBB` -> `src/GameZRecoil/zVideo/zvid_main.c:1693`
- `0x4a6cf0` `zVid_PackColorRGB` -> `src/GameZRecoil/zVideo/zvid_main.c:1677`
- `0x4a6d40` `zVid_PackColorRgbFloats` -> `src/GameZRecoil/zVideo/zvid_main.c:1709`
- `0x4a6e80` `zVideo_buff_CaptureSurfaceToImage` -> `src/GameZRecoil/zVideo/zvid_main.c:6983`
- `0x4a7330` `zVideo::CommitFogColorIfChanged` -> `src/GameZRecoil/zVideo/zvid_main.c:5831`
- `0x4a73a0` `zVideo::CommitFogTargetColorIfChanged` -> `src/GameZRecoil/zVideo/zvid_main.c:5852`
- `0x4a7410` `zVid::GetSelectedHwApiDescriptionOrDefault` -> `src/GameZRecoil/zVideo/zvid_main.c:3094`
- `0x4a7430` `zVid::GetHwApiDescription` -> `src/GameZRecoil/zVideo/zvid_main.c:3117`
- `0x4a7450` `zVid::GetHwApiDriverName` -> `src/GameZRecoil/zVideo/zvid_main.c:3127`
- `0x4a7480` `zVid::GetAcceptedDirectDrawDeviceCount` -> `src/GameZRecoil/zVideo/zvid_main.c:2746`
- `0x4a74d0` `zVideoD3D::SceneEnter` -> `src/GameZRecoil/zVideo/zvid_main.c:11344`
- `0x4a74f0` `zVideoD3D::SceneLeave` -> `src/GameZRecoil/zVideo/zvid_main.c:11360`
- `0x4a7770` `zVideo_RestoreIconicFullscreenWindowIfNeeded` -> `src/GameZRecoil/zVideo/zvid_main.c:2292`
- `0x4a7b20` `zVideo::ExchangeClearScreenBufferEnabled` -> `src/GameZRecoil/zVideo/zvid_main.c:4158`
- `0x4a7b30` `zVideo::GetClearScreenBufferEnabled` -> `src/GameZRecoil/zVideo/zvid_main.c:4170`
- `0x4a7b60` `zVideo_dd::PresentDisplayModeSurface` -> `src/GameZRecoil/zVideo/zvid_main.c:12471`
- `0x4a7fc0` `zVideo_dd::LockSurfaceState` -> `src/GameZRecoil/zVideo/zvid_main.c:12074`
- `0x4a8030` `zVideo_dd::UnlockSurfaceState` -> `src/GameZRecoil/zVideo/zvid_main.c:12108`
- `0x4a81a0` `zVideo_dd::ZBuffer_DepthFillRect` -> `src/GameZRecoil/zVideo/zvid_main.c:12725`
- `0x4a8220` `zVideo_dd::ClearScreenAndZBufferRect` -> `src/GameZRecoil/zVideo/zvid_main.c:12773`
- `0x4a82f0` `zVideo_dd::ClearSwBackbufferAndZBufferRects` -> `src/GameZRecoil/zVideo/zvid_main.c:12851`
- `0x4a9910` `zVid::GetAcceptedHardwareRendererCount_Cached` -> `src/GameZRecoil/zVideo/zvid_main.c:2756`
- `0x4aab90` `zVideo_dd3d::SubmitPolyFlatColor16` -> `src/GameZRecoil/zVideo/zvid_main.c:9461`
- `0x4aaef0` `zVideo_dd3d::SubmitPolyGouraudColor16` -> `src/GameZRecoil/zVideo/zvid_main.c:9606`
- `0x4ab320` `zVideo_dd3d::SubmitPolyColorAttr` -> `src/GameZRecoil/zVideo/zvid_main.c:9749`
- `0x4ab6d0` `zVideo_dd3d::SubmitPolyRenderClass` -> `src/GameZRecoil/zVideo/zvid_main.c:9853`
- `0x4abb20` `zVideo_dd3d::SubmitPolygon` -> `src/GameZRecoil/zVideo/zvid_main.c:10027`
- `0x4ac370` `zVideo_dd3d::SubmitPolygonLit` -> `src/GameZRecoil/zVideo/zvid_main.c:10225`
- `0x4acbd0` `zVideo_dd3d::DrawPointColor16` -> `src/GameZRecoil/zVideo/zvid_main.c:10421`
- `0x4acd00` `zVideo_dd3d::QueueSolidQuad` -> `src/GameZRecoil/zVideo/zvid_main.c:10476`
- `0x4ace30` `zVideo_dd3d::FlushSortedPolys` -> `src/GameZRecoil/zVideo/zvid_main.c:10530`
- `0x4ad120` `zVideo_dd3d::FlushQuadBatch` -> `src/GameZRecoil/zVideo/zvid_main.c:10677`
- `0x4ad250` `zVideo_dd3d::FlushOverwritePolys` -> `src/GameZRecoil/zVideo/zvid_main.c:10753`
- `0x4aed00` `OptCatalog::ProcessRuntimeInstance` -> `src/GameZRecoil/zWeapon/zwep_init.c:3524`
- `0x4aee40` `OptCatalog::ActivateTrailRuntimeState` -> `src/GameZRecoil/zWeapon/zwep_init.c:2808`
- `0x4aefb0` `OptCatalog::DeactivateTrailRuntimeState` -> `src/GameZRecoil/zWeapon/zwep_init.c:2746`
- `0x4af060` `OptCatalog::ProcessRuntimeInstances` -> `src/GameZRecoil/zWeapon/zwep_init.c:3600`
- `0x4b0530` `OptCatalog::ComputeAimPitchForTarget` -> `src/GameZRecoil/zWeapon/zwep_init.c:2905`
- `0x4b0600` `OptCatalog::PlayTriggerInactiveWarning` -> `src/GameZRecoil/zWeapon/zwep_init.c:2956`
- `0x4b0620` `OptCatalog::PlayWeaponInactiveWarning` -> `src/GameZRecoil/zWeapon/zwep_init.c:2965`
- `0x4b0640` `OptCatalog::PlayNoAmmoWarning` -> `src/GameZRecoil/zWeapon/zwep_init.c:2974`
- `0x4b0660` `OptCatalog::EmitQSandImpactEvent` -> `src/GameZRecoil/zWeapon/zwep_init.c:3170`
- `0x4b0710` `OptCatalog::EmitCraterImpactEvent` -> `src/GameZRecoil/zWeapon/zwep_init.c:3127`
- `0x4b07d0` `OptCatalog::HandleImpactEvent` -> `src/GameZRecoil/zWeapon/zwep_init.c:3263`
- `0x4b0980` `OptCatalog::HandleImpactEventFromRuntimeState` -> `src/GameZRecoil/zWeapon/zwep_init.c:3384`
- `0x4b09d0` `OptCatalog::BuildImpactHitList` -> `src/GameZRecoil/zWeapon/zwep_init.c:3414`
- `0x4b0a50` `OptCatalog::HandleImpactFromRuntimeProbe` -> `src/GameZRecoil/zWeapon/zwep_init.c:3466`
- `0x4b0ba0` `OptCatalog::CanSpawnThroughRay` -> `src/GameZRecoil/zWeapon/zwep_init.c:3917`
- `0x4b0ca0` `OptCatalog::ReflectAndSortImpactTraceList` -> `src/GameZRecoil/zWeapon/zwep_init.c:3971`
- `0x4b0e20` `OptCatalog::ComputeTrailImpactResponse` -> `src/GameZRecoil/zWeapon/zwep_init.c:4036`
- `0x4b0f70` `OptCatalog::UpdateTrailSegmentVisual` -> `src/GameZRecoil/zWeapon/zwep_init.c:4119`
- `0x4b0fd0` `OptCatalog::PlayImpactSound` -> `src/GameZRecoil/zWeapon/zwep_init.c:3211`
- `0x4b1030` `OptCatalog::PlayBounceSound` -> `src/GameZRecoil/zWeapon/zwep_init.c:3237`
- `0x4b1090` `zWepInit` -> `src/GameZRecoil/zWeapon/zwep_weapon_impl.h:56`
- `0x4b1140` `zWeapon::OnWeaponsSectionPreLoad` -> `src/GameZRecoil/zWeapon/zwep_weapon_impl.h:101`
- `0x4b1160` `zWeapon::OnWeaponsSectionDataReady` -> `src/GameZRecoil/zWeapon/zwep_weapon_impl.h:121`
- `0x4b1180` `OptCatalog::Shutdown` -> `src/GameZRecoil/zWeapon/zwep_init.c:2726`
- `0x4b1190` `zWeapon::LoadOptCatalogFromPath` -> `src/GameZRecoil/zWeapon/zwep_init.c:1098`
- `0x4b1d80` `zWeapon::SetMaxTetherAltitude` -> `src/GameZRecoil/zWeapon/zwep_weapon_impl.h:139`
- `0x4b1d90` `OptCatalog::ShutdownCore` -> `src/GameZRecoil/zWeapon/zwep_init.c:2664`
- `0x4b1f90` `OptCatalog::FreeTrailRuntimeStateStorage` -> `src/GameZRecoil/zWeapon/zwep_init.c:2736`
- `0x4b1fa0` `OptCatalog::LoadFxSpecFromReaderNode` -> `src/GameZRecoil/zWeapon/zwep_init.c:1983`
- `0x4b2160` `Light::InitThermalGlowPool` -> `src/GameZRecoil/zClass/Light.c:170`
- `0x4b21c0` `PlayerTimedHitStatus::ResetFields` -> `src/GameZRecoil/zWeapon/zwep_weapon_impl.h:161`
- `0x4b21e0` `Light::DestroyThermalGlowPool` -> `src/GameZRecoil/zClass/Light.c:201`
- `0x4b2210` `HitSource::UpdateTimedStatus` -> `src/GameZRecoil/zWeapon/zwep_weapon_impl.h:273`
- `0x4b22d0` `PlayerTimedHitStatus::ClearLightAndReset` -> `src/GameZRecoil/zWeapon/zwep_weapon_impl.h:176`
- `0x4b2300` `PlayerTimedHitStatus::TickAndUpdateLight` -> `src/GameZRecoil/zWeapon/zwep_weapon_impl.h:194`
- `0x4b2520` `Light::AllocFromFreeListAndAttach` -> `src/GameZRecoil/zClass/Light.c:220`
- `0x4b2570` `Light::ReturnToFreeList` -> `src/GameZRecoil/zClass/Light.c:253`
- `0x4b25a0` `zClass_Node::SetDamageHitCallback` -> `src/GameZRecoil/zWeapon/zwep_init.c:955`
- `0x4b25f0` `zClass_Node::AssignDamageHandlerRecursiveIfMissing` -> `src/GameZRecoil/zWeapon/zwep_init.c:906`
- `0x4b2630` `zClass_Node::ClearDamageHandler` -> `src/GameZRecoil/zWeapon/zwep_init.c:991`
- `0x4b2670` `zClass_Node::ClearDamageHandlerRecursive` -> `src/GameZRecoil/zWeapon/zwep_init.c:932`
- `0x4b26b0` `zClass_Node::SetDamageTimerCallback` -> `src/GameZRecoil/zWeapon/zwep_init.c:1021`
- `0x4b26f0` `OptCatalog::InvokeDamageFeedbackAndHitCallback` -> `src/GameZRecoil/zWeapon/zwep_init.c:2983`
- `0x4b2880` `OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback` -> `src/GameZRecoil/zWeapon/zwep_init.c:3088`
- `0x4b28e0` `OptCatalog::SetDamageContext` -> `src/GameZRecoil/zWeapon/zwep_init.c:3070`
- `0x4b2900` `DamageFeedback::SetIntensityScalar` -> `src/GameZRecoil/zWeapon/zwep_init.c:4160`
- `0x4b2910` `OptCatalog::GetCapturedHitSourcePtr` -> `src/GameZRecoil/zWeapon/zwep_init.c:3116`
- `0x4b2920` `HitContext::GetCurrentOwnerOrCtx` -> `src/GameZRecoil/zWeapon/zwep_init.c:4173`
- `0x4b2930` `OptCatalog_MineIterator::Begin` -> `src/GameZRecoil/zWeapon/zwep_init.c:1811`
- `0x4b2940` `OptCatalog_MineIterator::Next` -> `src/GameZRecoil/zWeapon/zwep_init.c:1829`
- `0x4b2960` `zGame::Options_LoadFromRegistry` -> `src/GameZRecoil/zGame/zgame_opt.c:1172`
- `0x4b2bf0` `zGame::Options_SaveToRegistry` -> `src/GameZRecoil/zGame/zgame_opt.c:1290`
- `0x4b2e80` `zGame::Options_GetOrCreateOption` -> `src/GameZRecoil/zGame/zgame_opt.c:1097`
- `0x4b2f50` `zSnd::AcquireCachedDirectSound` -> `src/GameZRecoil/zSound/zsnd_init.cpp:810`
- `0x4b2fa0` `zSnd::ReleaseCachedDirectSound` -> `src/GameZRecoil/zSound/zsnd_init.cpp:841`
- `0x4b2fc0` `zSnd::CachedDirectSound_GetCaps` -> `src/GameZRecoil/zSound/zsnd_init.cpp:854`
- `0x4b2fe0` `zSys::HasCpuidSupportRuntimeOptions` -> `src/GameZRecoil/zSys/zSys.cpp:258`
- `0x4b2fe0` `zSys::HasCpuidSupportRuntimeOptions` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:68`
- `0x4b3020` `zCpu::HasMmxSupport` -> `src/GameZRecoil/zSys/zSys.cpp:1337`
- `0x4b3020` `zCpu::HasMmxSupport` -> `src/GameZRecoil/zSys/zsys_cpu_asm.inl:19`
- `0x4b3050` `zSys::CheckCpuSignatureMask` -> `src/GameZRecoil/zSys/zSys.cpp:243`
- `0x4b31b0` `zSys::GetCpuClass` -> `src/GameZRecoil/zSys/zsys_cpu_get_class.inl:2`
- `0x4b31c0` `zSys::GetCpuMhz` -> `src/GameZRecoil/zSys/zSys.cpp:1353`
- `0x4b31f0` `zSnd::HasMmxMixerSupport` -> `src/GameZRecoil/zSound/zsnd_init.cpp:796`
- `0x4b3210` `zSys::ReturnZeroStub` -> `src/GameZRecoil/zSys/zSys.cpp:1368`
- `0x4b3220` `zVid::HasAcceptedHardwareRenderer` -> `src/GameZRecoil/zVideo/zvid_main.c:2764`
- `0x4b3230` `zSys::GetTotalPhysKb` -> `src/GameZRecoil/zSys/zSys.cpp:1376`
- `0x4b3260` `zGame::Options_InitRegistryContext` -> `src/GameZRecoil/zGame/zgame_opt.c:1155`
- `0x4b32c0` `zGame::Options_ShutdownRegistryContext` -> `src/GameZRecoil/zGame/zgame_opt.c:1836`
- `0x4b3380` `zGame::Options_FindOption` -> `src/GameZRecoil/zGame/zgame_opt.c:1077`
- `0x4b3420` `zSys::DetectCpuClassAndFeatures` -> `src/GameZRecoil/zSys/zsys_cpu_detect.inl:2`
- `0x4b3480` `zSys::ReadCpuidFeatureFlags` -> `src/GameZRecoil/zSys/zSys.cpp:322`
- `0x4b36f0` `CpuBenchmarkResolver::ResolveCpuBenchmarkPacket` -> `src/GameZRecoil/zSys/zSys.cpp:1283`
- `0x4b37f0` `CpuBenchmarkResolver::MeasureMhzViaBsfLoop_Qpc` -> `src/GameZRecoil/zSys/zSys.cpp:472`
- `0x4b37f0` `CpuBenchmarkResolver::MeasureMhzViaBsfLoop_Qpc` -> `src/GameZRecoil/zSys/zSys.cpp:595`
- `0x4b38e0` `CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc` -> `src/GameZRecoil/zSys/zSys.cpp:666`
- `0x4b38e0` `CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc` -> `src/GameZRecoil/zSys/zSys.cpp:917`
- `0x4b3b00` `zSys::ReadCmosRtcSecondsBcd` -> `src/GameZRecoil/zSys/zSys.cpp:375`
- `0x4b3b20` `zSys::ReadTsc64` -> `src/GameZRecoil/zSys/zSys.cpp:386`
- `0x4b3b50` `CpuBenchmarkResolver::MeasureCpuMhz_CmosRtc` -> `src/GameZRecoil/zSys/zSys.cpp:1049`
- `0x4b3b50` `CpuBenchmarkResolver::MeasureCpuMhz_CmosRtc` -> `src/GameZRecoil/zSys/zSys.cpp:1198`
- `0x4b3ca0` `zSys::Sub64` -> `src/GameZRecoil/zSys/zSys.cpp:406`
- `0x4b3ce0` `HudUiWidget::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:10657`
- `0x4b3d00` `HudUiWidget::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:7253`
- `0x4b3d00` `HudUiWidget::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:7275`
- `0x4b3d50` `HudUiWidget::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:2483`
- `0x4b3da0` `HudUiWidget::ReleaseImageIfOwned` -> `src/GameZRecoil/zUI/zui.cpp:10604`
- `0x4b3dd0` `HudUiWidget::SetPos` -> `src/GameZRecoil/zUI/zui.cpp:10672`
- `0x4b3e30` `HudUiWidget::SetImageByPathOwned` -> `src/GameZRecoil/zUI/zui.cpp:10636`
- `0x4b3e70` `HudUiWidget::SetImageBorrowedAndInvalidate` -> `src/GameZRecoil/zUI/zui.cpp:10617`
- `0x4b4030` `HudUiWidget::HitTest` -> `src/GameZRecoil/zUI/zui.cpp:10533`
- `0x4b4070` `HudUiElement::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:4314`
- `0x4b40c0` `HudUiElement::CopyConstructor` -> `src/GameZRecoil/zUI/zui.cpp:4329`
- `0x4b4120` `HudUiElement::CopyFrom` -> `src/GameZRecoil/zUI/zui.cpp:4348`
- `0x4b4180` `HudUiElement::Invalidate` -> `src/GameZRecoil/zUI/zui.cpp:4376`
- `0x4b4190` `HudUiElement::SetBltSourceAndClipRect` -> `src/GameZRecoil/zUI/zui.cpp:4453`
- `0x4b41b0` `HudUiElement::SetClipRect` -> `src/GameZRecoil/zUI/zui.cpp:4465`
- `0x4b42c0` `HudUiElement::GetTextRect` -> `src/GameZRecoil/zUI/zui.cpp:4497`
- `0x4b42f0` `HudUiTextInput::HudUiTextInput` -> `src/GameZRecoil/zUI/zui.cpp:10824`
- `0x4b4370` `HudUiTextInput::~HudUiTextInput` -> `src/GameZRecoil/zUI/zui.cpp:10766`
- `0x4b4370` `HudUiTextInput::~HudUiTextInput` -> `src/GameZRecoil/zUI/zui.cpp:10776`
- `0x4b4390` `HudUiTextInput::AllocTextBuffer` -> `src/GameZRecoil/zUI/zui.cpp:10798`
- `0x4b43d0` `HudUiTextInput::SetContents` -> `src/GameZRecoil/zUI/zui.cpp:10881`
- `0x4b4410` `HudUiTextInput::GetBuffer` -> `src/GameZRecoil/zUI/zui.cpp:10897`
- `0x4b4420` `HudUiTextInput::SetCursorPosition` -> `src/GameZRecoil/zUI/zui.cpp:10857`
- `0x4b4420` `HudUiTextInput::SetCursorPosition` -> `src/GameZRecoil/zUI/zui.cpp:10868`
- `0x4b4460` `HudUiTextInput::DispatchKeyAction` -> `src/GameZRecoil/zUI/zui.cpp:11011`
- `0x4b44e0` `HudUiTextInput::InsertCharAtCursor` -> `src/GameZRecoil/zUI/zui.cpp:10990`
- `0x4b4530` `HudUiTextInput::BackspaceDeleteChar` -> `src/GameZRecoil/zUI/zui.cpp:10976`
- `0x4b4550` `HudUiTextInput::DeleteCharForward` -> `src/GameZRecoil/zUI/zui.cpp:10944`
- `0x4b4560` `HudUiTextInput::MoveCursorLeft` -> `src/GameZRecoil/zUI/zui.cpp:10955`
- `0x4b4570` `HudUiTextInput::MoveCursorRight` -> `src/GameZRecoil/zUI/zui.cpp:10965`
- `0x4b4590` `HudUiTextInput::ShiftTextRight` -> `src/GameZRecoil/zUI/zui.cpp:10905`
- `0x4b45e0` `HudUiTextInput::ShiftTextLeft` -> `src/GameZRecoil/zUI/zui.cpp:10926`
- `0x4b4620` `HudUiSliderBorder::HudUiSliderBorder` -> `src/GameZRecoil/zUI/zui.cpp:11240`
- `0x4b47a0` `HudUiElement::~HudUiElement` -> `src/GameZRecoil/zHud/zhud_ui.h:129`
- `0x4b47a0` `HudUiElement::~HudUiElement` -> `src/GameZRecoil/zHud/zhud_ui.h:141`
- `0x4b47a0` `HudUiElement::~HudUiElement` -> `src/GameZRecoil/zUI/zui.cpp:4367`
- `0x4b47b0` `HudUiSliderBorder::Update` -> `src/GameZRecoil/zUI/zui.cpp:11321`
- `0x4b47b0` `HudUiSliderBorder::Update` -> `src/GameZRecoil/zUI/zui.cpp:11330`
- `0x4b4810` `HudUiSliderBorder::SetBounds` -> `src/GameZRecoil/zUI/zui.cpp:11355`
- `0x4b49e0` `HudUiNumericTextInput::HudUiNumericTextInput` -> `src/GameZRecoil/zUI/zui.cpp:11437`
- `0x4b4a90` `HudUiNumericTextInput::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:11664`
- `0x4b4ab0` `HudUiTextInput::DestructorCoreThunk` -> `src/GameZRecoil/zUI/zui.cpp:10789`
- `0x4b4ac0` `HudUiNumericTextInput::~HudUiNumericTextInput` -> `src/GameZRecoil/zUI/zui.cpp:11634`
- `0x4b4b30` `HudUiNumericTextInput::RawKeyboardCallback` -> `src/GameZRecoil/zUI/zui.cpp:11695`
- `0x4b4b50` `HudUiNumericTextInput::OnRawKeyboardChar` -> `src/GameZRecoil/zUI/zui.cpp:11745`
- `0x4b4ba0` `HudUiNumericTextInput::SetInputActive` -> `src/GameZRecoil/zUI/zui.cpp:11710`
- `0x4b4c50` `HudUiNumericTextInput::SetRawKeyboardCapture` -> `src/GameZRecoil/zUI/zui.cpp:11599`
- `0x4b4c90` `HudUiNumericTextInput::OnActivate` -> `src/GameZRecoil/zUI/zui.cpp:11625`
- `0x4b4ca0` `HudUiNumericTextInput::UpdateCaptureUiAndClip` -> `src/GameZRecoil/zUI/zui.cpp:11534`
- `0x4b4e40` `HudUiNumericTextInput::AllocTextBuffer` -> `src/GameZRecoil/zUI/zui.cpp:11497`
- `0x4b4e60` `HudUiNumericTextInput::Update` -> `src/GameZRecoil/zUI/zui.cpp:11515`
- `0x4b4ed0` `HudUiNumericTextInput::GetBuffer` -> `src/GameZRecoil/zUI/zui.cpp:11507`
- `0x4b4ee0` `HudUiZrdWidget::HudUiZrdWidget` -> `src/GameZRecoil/zUI/zui.cpp:7736`
- `0x4b4ee0` `HudUiZrdWidget::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:7774`
- `0x4b50a0` `HudUiZrdWidget::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:8230`
- `0x4b50a0` `HudUiZrdWidget::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:8242`
- `0x4b50c0` `HudUiZrdWidget::~HudUiZrdWidget` -> `src/GameZRecoil/zUI/zui.cpp:8120`
- `0x4b52f0` `HudUiZrdWidget::DeleteChildIfPresent` -> `src/GameZRecoil/zUI/zui.cpp:8105`
- `0x4b5310` `HudUiZrdWidget::Invalidate` -> `src/GameZRecoil/zUI/zui.cpp:8274`
- `0x4b5350` `HudUiZrdWidget::GetBoundsRectOrNull` -> `src/GameZRecoil/zUI/zui.cpp:8293`
- `0x4b5630` `HudUiZrdWidget::ShowPreview` -> `src/GameZRecoil/zUI/zui.cpp:8415`
- `0x4b5740` `HudUiZrdWidget::RefreshState` -> `src/GameZRecoil/zUI/zui.cpp:8364`
- `0x4b5860` `HudUiZrdWidget::HidePreview` -> `src/GameZRecoil/zUI/zui.cpp:8502`
- `0x4b5900` `HudUiZrdWidget::OnActivate` -> `src/GameZRecoil/zUI/zui.cpp:8458`
- `0x4b59f0` `HudUiZrdWidget::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:7783`
- `0x4b6fc0` `HudUiCheckToggleWidget::HudUiCheckToggleWidget` -> `src/GameZRecoil/zUI/zui.cpp:8533`
- `0x4b6fc0` `HudUiCheckToggleWidget::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:8546`
- `0x4b7000` `HudUiCheckToggleWidget::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:8586`
- `0x4b70b0` `HudUiCheckToggleWidget::GetBoundsRectOrNull` -> `src/GameZRecoil/zUI/zui.cpp:8618`
- `0x4b70c0` `HudUiCheckToggleWidget::RefreshState` -> `src/GameZRecoil/zUI/zui.cpp:8626`
- `0x4b7210` `HudUiCheckToggleWidget::ShowPreview` -> `src/GameZRecoil/zUI/zui.cpp:8683`
- `0x4b7250` `HudUiCheckToggleWidget::HidePreview` -> `src/GameZRecoil/zUI/zui.cpp:8699`
- `0x4b7290` `HudUiCheckToggleWidget::OnActivate` -> `src/GameZRecoil/zUI/zui.cpp:8716`
- `0x4b72c0` `HudUiCheckToggleWidget::SetChecked` -> `src/GameZRecoil/zUI/zui.cpp:8861`
- `0x4b7d60` `HudUiCycleSelectorWidget::HudUiCycleSelectorWidget` -> `src/GameZRecoil/zUI/zui.cpp:8895`
- `0x4b7d60` `HudUiCycleSelectorWidget::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:8914`
- `0x4b7dc0` `HudUiCycleSelectorWidget::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:8954`
- `0x4b7e60` `HudUiCycleSelectorWidget::Update` -> `src/GameZRecoil/zUI/zui.cpp:9066`
- `0x4b7ee0` `HudUiCycleSelectorWidget::AdvanceSelectionAndActivate` -> `src/GameZRecoil/zUI/zui.cpp:8984`
- `0x4b7f20` `HudUiCycleSelectorWidget::SetIndexClamped` -> `src/GameZRecoil/zUI/zui.cpp:9006`
- `0x4b7f80` `HudUiCycleSelectorWidget::SetVisibleRange` -> `src/GameZRecoil/zUI/zui.cpp:9041`
- `0x4b7fd0` `HudUiCycleSelectorWidget::AddTextEntry` -> `src/GameZRecoil/zUI/zui.cpp:9088`
- `0x4b8100` `HudUiCycleSelectorWidget::ApplyFontStyleForEntry` -> `src/GameZRecoil/zUI/zui.cpp:9283`
- `0x4b8200` `HudUiCycleSelectorWidget::AddBitmapEntry` -> `src/GameZRecoil/zUI/zui.cpp:9354`
- `0x4b82e0` `HudUiCycleSelectorWidget::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:9392`
- `0x4b8450` `HudUiFillBitmap::HudUiFillBitmap` -> `src/GameZRecoil/zUI/zui.cpp:9521`
- `0x4b84b0` `HudUiFillBitmap::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:9572`
- `0x4b8520` `HudUiFillBitmap::Draw` -> `src/GameZRecoil/zUI/zui.cpp:9587`
- `0x4b85c0` `HudUiFillBitmap::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:9619`
- `0x4b8650` `HudUiFillBitmap::UpdateNormalizedFromCursor` -> `src/GameZRecoil/zUI/zui.cpp:9671`
- `0x4b86b0` `HudUiFillBitmap::SetNormalizedValueAndRebuild` -> `src/GameZRecoil/zUI/zui.cpp:9704`
- `0x4b8760` `HudUiZrdWidgetEx17C_Item::HudUiZrdWidgetEx17C_Item` -> `src/GameZRecoil/zUI/zui.cpp:9741`
- `0x4b87a0` `HudUiZrdWidgetEx17C_Item::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:9770`
- `0x4b87c0` `HudUiZrdWidgetEx17C_Item::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:9753`
- `0x4b87c0` `HudUiZrdWidgetEx17C_Item::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:9762`
- `0x4b87d0` `HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected` -> `src/GameZRecoil/zUI/zui.cpp:9785`
- `0x4b87e0` `HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected` -> `src/GameZRecoil/zUI/zui.cpp:9795`
- `0x4b87e0` `HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected` -> `src/GameZRecoil/zUI/zui.cpp:9803`
- `0x4b87f0` `HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf` -> `src/GameZRecoil/zUI/zui.cpp:9813`
- `0x4b87f0` `HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf` -> `src/GameZRecoil/zUI/zui.cpp:9821`
- `0x4b8850` `HudUiZrdWidgetEx17C_Item::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:9838`
- `0x4b8850` `HudUiZrdWidgetEx17C_Item::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:9846`
- `0x4b8a90` `HudUiZrdWidgetEx17C_Item::SetSelected` -> `src/GameZRecoil/zUI/zui.cpp:9940`
- `0x4b8af0` `HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds` -> `src/GameZRecoil/zUI/zui.cpp:9964`
- `0x4b8b10` `HudUiZrdWidgetEx17C::HudUiZrdWidgetEx17C` -> `src/GameZRecoil/zUI/zui.cpp:9972`
- `0x4b8b40` `HudUiZrdWidgetEx17C::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:10017`
- `0x4b8b60` `HudUiZrdWidgetEx17C::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:9987`
- `0x4b8b60` `HudUiZrdWidgetEx17C::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:9996`
- `0x4b8be0` `HudUiZrdWidgetEx17C::LoadFromZrd` -> `src/GameZRecoil/zUI/zui.cpp:10049`
- `0x4b8cf0` `HudUiZrdWidgetEx17C::SetSelectedIndex` -> `src/GameZRecoil/zUI/zui.cpp:10108`
- `0x4b8cf0` `HudUiZrdWidgetEx17C::SetSelectedIndex` -> `src/GameZRecoil/zUI/zui.cpp:10120`
- `0x4b8d30` `HudCmdBindButtonBase::HudCmdBindButtonBase` -> `src/GameZRecoil/zUI/zui.cpp:10192`
- `0x4b92a0` `HudUiListSelectorItem::HudUiListSelectorItem` -> `src/GameZRecoil/zHud/zhud_ui.h:1242`
- `0x4b92a0` `HudUiListSelectorItem::HudUiListSelectorItem` -> `src/GameZRecoil/zUI/zui.cpp:10142`
- `0x4b9520` `HudUiListSelectorItem::OnActivate` -> `src/GameZRecoil/zUI/zui.cpp:10141`
- `0x4ba070` `HudUiBackground::BindButtonsNodeToWidgetByName` -> `src/GameZRecoil/zUI/zui.cpp:6886`
- `0x4ba0c0` `HudUiBackground::BindWidgetByName` -> `src/GameZRecoil/zUI/zui.cpp:6916`
- `0x4ba0e0` `HudUiBackground::BindPrimitiveNodeToElement` -> `src/GameZRecoil/zUI/zui.cpp:6932`
- `0x4ba350` `HudUiBackground::FreeLoadedTreeRoots` -> `src/GameZRecoil/zUI/zui.cpp:7094`
- `0x4ba380` `HudUiDialogController::BlitOwnedSurfaceToPrimary` -> `src/GameZRecoil/zUI/zui.cpp:6466`
- `0x4ba3a0` `HudUiContainer::InvalidateChildren` -> `src/GameZRecoil/zUI/zui.cpp:5454`
- `0x4ba3c0` `HudUiFillBitmap::SetNormalizedValue` -> `src/GameZRecoil/zUI/zui.cpp:9683`
- `0x4ba3e0` `HudUiOwnedTextInput::OnAccept` -> `src/GameZRecoil/zUI/zui.cpp:11054`
- `0x4ba400` `HudUiPanel::GetWrapRect` -> `src/GameZRecoil/zUI/zui.cpp:12550`
- `0x4ba410` `HudUiListSelectorItem::Draw` -> `src/GameZRecoil/zUI/zui.cpp:10164`
- `0x4ba470` `StdPtrVector::FreeBufferAndReset` -> `src/GameZRecoil/zUI/zui.cpp:10209`
- `0x4ba4a0` `HudFontStyle::HudFontStyle` -> `src/GameZRecoil/zUI/zui.cpp:7221`
- `0x4ba4c0` `HudFontStyle::Destructor` -> `src/GameZRecoil/zUI/zui.cpp:7235`
- `0x4ba4c0` `HudFontStyle::Destructor` -> `src/GameZRecoil/zUI/zui.cpp:7245`
- `0x4ba4d0` `HudUiPanelPtrVector::EraseRange` -> `src/GameZRecoil/zUI/zui.cpp:7950`
- `0x4ba510` `HudUiPanelPtrVector::InsertN` -> `src/GameZRecoil/zUI/zui.cpp:7981`
- `0x4ba740` `HudUiPanel::HudUiPanel` -> `src/GameZRecoil/zUI/zui.cpp:12320`
- `0x4bab40` `HudUiPanel::~HudUiPanel` -> `src/GameZRecoil/zUI/zui.cpp:12480`
- `0x4bac10` `HudUiPanel::RebuildTextRect` -> `src/GameZRecoil/zUI/zui.cpp:12775`
- `0x4bb0c0` `HudUiFlashPanel::ComputeFlashBlendColor` -> `src/GameZRecoil/zUI/zui.cpp:5267`
- `0x4bb1c0` `HudUiPanel::MeasureTextPrefixRect` -> `src/GameZRecoil/zUI/zui.cpp:13066`
- `0x4bb2a0` `HudUiPanel::UpdateTextBoundsFromContent` -> `src/GameZRecoil/zUI/zui.cpp:13004`
- `0x4bb3d0` `HudUiPanel::HitTest` -> `src/GameZRecoil/zUI/zui.cpp:12558`
- `0x4bb460` `HudUiPanel::Draw` -> `src/GameZRecoil/zUI/zui.cpp:12494`
- `0x4bb540` `HudUiPanel::SetTextFmt` -> `src/GameZRecoil/zUI/zui.cpp:12661`
- `0x4bb5e0` `HudUiPanel::SetTextFmtV` -> `src/GameZRecoil/zUI/zui.cpp:12682`
- `0x4bb680` `HudUiPanel::SetText` -> `src/GameZRecoil/zUI/zui.cpp:12730`
- `0x4bb710` `HudUiPanel::QueryTextHeight` -> `src/GameZRecoil/zUI/zui.cpp:13119`
- `0x4bb740` `HudUiPanel::GetTextRect` -> `src/GameZRecoil/zUI/zui.cpp:12594`
- `0x4bb790` `HudUiCompositePanel::HudUiCompositePanel` -> `src/GameZRecoil/zUI/zui.cpp:4911`
- `0x4bb960` `HudUiCompositePanel::ScalarDeletingDestructor` -> `src/GameZRecoil/zUI/zui.cpp:4948`
- `0x4bb980` `HudUiCompositePanel::Update` -> `src/GameZRecoil/zUI/zui.cpp:4964`
- `0x4bb9f0` `HudUiCompositePanel::SetPos` -> `src/GameZRecoil/zUI/zui.cpp:4983`
- `0x4bbaa0` `HudUiCompositePanel::SetTextFmt` -> `src/GameZRecoil/zUI/zui.cpp:5053`
- `0x4bbac0` `HudUiCompositePanel::SetTextFmtV` -> `src/GameZRecoil/zUI/zui.cpp:5073`
- `0x4bbb20` `HudUiCompositePanel::ScrollHistory` -> `src/GameZRecoil/zUI/zui.cpp:5091`
- `0x4bbbe0` `HudUiCompositePanel::SetFont` -> `src/GameZRecoil/zUI/zui.cpp:5116`
- `0x4bbca0` `HudUiCompositePanel::ResizeEntryVectorAndRelayout` -> `src/GameZRecoil/zUI/zui.cpp:5161`
- `0x4bbe90` `HudUiCompositePanel::ReapplyEntryCount` -> `src/GameZRecoil/zUI/zui.cpp:5011`
- `0x4bbed0` `HudUiCompositePanel::ResizeEntryCount` -> `src/GameZRecoil/zUI/zui.cpp:5022`
- `0x4bbfa0` `HudUiCompositePanelVector::Clear` -> `src/GameZRecoil/zUI/zui.cpp:4794`
- `0x4bbff0` `HudUiCompositePanelVector::InsertCopies` -> `src/GameZRecoil/zUI/zui.cpp:4821`
- `0x4bc320` `HudUiCompositePanelEntry::ConstructorCopyRange` -> `src/GameZRecoil/zUI/zui.cpp:5241`
- `0x4bc3a0` `HudUiCompositePanelEntry::AssignCopy` -> `src/GameZRecoil/zUI/zui.cpp:5202`
- `0x4bc410` `HudUiCompositePanelEntry::ConstructorCopy` -> `src/GameZRecoil/zUI/zui.cpp:5220`
- `0x4bc480` `HudUiCircle::HudUiCircle` -> `src/GameZRecoil/zHud/zhud_ui.h:669`
- `0x4bc4c0` `HudUiCircle::Draw` -> `src/GameZRecoil/zUI/zui.cpp:4765`
- `0x4bc4e0` `HudUiCircle::HitTestCore` -> `src/GameZRecoil/zUI/zui.cpp:4780`
- `0x4bc510` `HudUiBackgroundContainer::HudUiBackgroundContainer` -> `src/GameZRecoil/zUI/zui.cpp:6343`
- `0x4bc540` `HudUiBackgroundContainer::~HudUiBackgroundContainer` -> `src/GameZRecoil/zUI/zui.cpp:6354`
- `0x4bc550` `HudUiBackgroundContainer::SetInputFocus` -> `src/GameZRecoil/zUI/zui.cpp:6362`
- `0x4bc560` `HudUiBackgroundContainer::GetInputFocus` -> `src/GameZRecoil/zUI/zui.cpp:6372`
- `0x4bc570` `HudUiBackgroundContainer::UpdateAll` -> `src/GameZRecoil/zUI/zui.cpp:7110`
- `0x4bc760` `HudUi::SetInvalidateMode` -> `src/GameZRecoil/zUI/zui.cpp:13352`
- `0x4bc780` `HudUiContainer::HudUiContainer` -> `src/GameZRecoil/zUI/zui.cpp:5297`
- `0x4bc7b0` `HudUiContainer::~HudUiContainer` -> `src/GameZRecoil/zUI/zui.cpp:5308`
- `0x4bc7b0` `HudUiContainer::DestructorCore` -> `src/GameZRecoil/zUI/zui.cpp:5316`
- `0x4bc7c0` `HudUiContainer::AddChild` -> `src/GameZRecoil/zUI/zui.cpp:5325`
- `0x4bc8d0` `HudUiContainer::SetChildFlags` -> `src/GameZRecoil/zUI/zui.cpp:5415`
- `0x4bc900` `HudUiContainer::UpdateAll` -> `src/GameZRecoil/zUI/zui.cpp:5438`
- `0x4bc930` `HudUiTransitionTextPanel::ResetFlashState` -> `src/GameZRecoil/zUI/zui.cpp:9214`
- `0x4bc980` `HudUiTransitionTextPanel::SetFlashRate` -> `src/GameZRecoil/zUI/zui.cpp:9240`
- `0x4bcb50` `HudUiTextLabel::HudUiTextLabel` -> `src/GameZRecoil/zUI/zui.cpp:12081`
- `0x4bccf0` `HudUiTextLabel::SetTextFmt` -> `src/GameZRecoil/zUI/zui.cpp:12169`
- `0x4bcd40` `HudUiPanel::SetClip` -> `src/GameZRecoil/zUI/zui.cpp:4481`
- `0x4bcd80` `HudUiTextLabel::RebuildTextBounds` -> `src/GameZRecoil/zUI/zui.cpp:12206`
- `0x4bcdc0` `HudUiTextLabel::MeasureTextWidth` -> `src/GameZRecoil/zUI/zui.cpp:12223`
- `0x4bcdf0` `HudUiTextLabel::UpdateTextExtents` -> `src/GameZRecoil/zUI/zui.cpp:12304`
- `0x4bce30` `HudUiTextLabel::OnDraw` -> `src/GameZRecoil/zUI/zui.cpp:12239`
- `0x4bcea0` `HudUiTextLabel::HitTest` -> `src/GameZRecoil/zUI/zui.cpp:12275`
- `0x4bcf20` `HudUiBar::HudUiBar` -> `src/GameZRecoil/zUI/zui.cpp:11082`
- `0x4bcff0` `HudUiBar::Draw` -> `src/GameZRecoil/zUI/zui.cpp:11099`
- `0x4bd020` `HudUiTopMessageStack::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:13581`
- `0x4bd100` `HudUiPanel::ConstructorDefaultThunk` -> `src/GameZRecoil/zUI/zui.cpp:12373`
- `0x4bd2a0` `HudUiTextStack4::Clear` -> `src/GameZRecoil/zUI/zui.cpp:13516`
- `0x4bd2d0` `HudUiChatMessageStack::Constructor` -> `src/GameZRecoil/zUI/zui.cpp:13616`
- `0x4bd3d0` `HudUiTextStack4::SetTextColors` -> `src/GameZRecoil/zUI/zui.cpp:13500`
- `0x4bd410` `HudUiTextStack4::SetXAll` -> `src/GameZRecoil/zUI/zui.cpp:13553`
- `0x4bd440` `HudUiTextStack4::SetYDescending` -> `src/GameZRecoil/zUI/zui.cpp:13566`
- `0x4bd470` `zTimedTask::RemoveFromActiveList` -> `src/GameZRecoil/zUI/zui.cpp:13178`
- `0x4bd4d0` `zTimedTask::RunImmediateAction` -> `src/GameZRecoil/zUI/zui.cpp:13211`
- `0x4bd660` `zTimedTask::TickActiveList` -> `src/GameZRecoil/zUI/zui.cpp:13322`
- `0x4bd6f0` `HudLineClip::SetCurrentBoundsFromRectI` -> `src/Battlesport/hud_sensor_tracker_body.h:810`
- `0x4bd720` `zMath::ClipLineSegmentToZRange` -> `src/GameZRecoil/zMath/zmth_main.c:1629`
- `0x4bd800` `zMath::ClipLineSegmentPointToZ` -> `src/GameZRecoil/zMath/zmth_main.c:1610`
- `0x4bd840` `HudLineClip::ClipSegmentToCurrentBounds` -> `src/Battlesport/hud_sensor_tracker_body.h:938`
- `0x4bd880` `HudLineClip::ClipSegmentToCurrentXBounds` -> `src/Battlesport/hud_sensor_tracker_body.h:824`
- `0x4bd9c0` `HudLineClip::ClipEndpointToX` -> `src/Battlesport/hud_sensor_tracker_body.h:780`
- `0x4bd9f0` `HudLineClip::ClipSegmentToCurrentYBounds` -> `src/Battlesport/hud_sensor_tracker_body.h:881`
- `0x4bdb30` `HudLineClip::ClipEndpointToY` -> `src/Battlesport/hud_sensor_tracker_body.h:795`
- `0x4bdb60` `zVideoFxPass3Element::Draw` -> `src/GameZRecoil/zVideo/zvid_main.c:3138`
- `0x4bdbc0` `zVideoFxPass3RootElement::ApplyPass3` -> `src/GameZRecoil/zVideo/zvid_main.c:3185`
- `0x4bdbe0` `zVideoFxPass3Slot::Constructor` -> `src/GameZRecoil/zVideo/zvid_main.c:3199`
- `0x4bdc40` `zVideoFxPass3Slot::ApplyPass3` -> `src/GameZRecoil/zVideo/zvid_main.c:3241`
- `0x4bed90` `zVideo::zVideoFxPass3Config_QueueElementLocal` -> `src/GameZRecoil/zVideo/zvid_main.c:5124`
- `0x4bee20` `zVideoFxPass3Config::QueuePrimitiveRaw` -> `src/GameZRecoil/zVideo/zvid_main.c:5055`
- `0x4bee40` `zVideoFxPass3Config::CrtInitGlobalSingleton` -> `src/GameZRecoil/zVideo/zvid_main.c:3339`
- `0x4bee50` `zVideoFxPass3Config::ConstructGlobalSingleton` -> `src/GameZRecoil/zVideo/zvid_main.c:3315`
- `0x4bee60` `zVideoFxPass3Config::RegisterDestroyAtExit` -> `src/GameZRecoil/zVideo/zvid_main.c:3331`
- `0x4bee70` `zVideoFxPass3Config::DestroyGlobalSingleton` -> `src/GameZRecoil/zVideo/zvid_main.c:3323`
- `0x4bee80` `zVideoFxPass3Config::Destructor` -> `src/GameZRecoil/zVideo/zvid_main.c:3301`
- `0x4beee0` `zVideo::FxPass3_SetPrimaryElementParamsLocal` -> `src/GameZRecoil/zVideo/zvid_main.c:5109`
- `0x4bef10` `zVideo::FxPass3_QueueElementLocal` -> `src/GameZRecoil/zVideo/zvid_main.c:5161`
- `0x4bef50` `zVideo::FxPass3_QueuePrimitive` -> `src/GameZRecoil/zVideo/zvid_main.c:5186`
- `0x4bef90` `zVideoFxPass3Config::Constructor` -> `src/GameZRecoil/zVideo/zvid_main.c:3260`
- `0x4bf540` `HudUiMessageBoxDialog::ScalarDeletingDestructor` -> `src/Battlesport/hud_ui_message_box_dialog_body.h:271`
- `0x4bf560` `HudUiMessageBoxDialog::Destructor` -> `src/Battlesport/hud_ui_message_box_dialog_body.h:290`
- `0x4bf7c0` `HudUiMessageBoxDialog::OnOk` -> `src/Battlesport/hud_ui_message_box_dialog_body.h:417`
- `0x4bf7e0` `HudUiMessageBoxDialog::OnCancel` -> `src/Battlesport/hud_ui_message_box_dialog_body.h:429`
- `0x4bf840` `HudUiPolyline::HudUiPolyline` -> `src/GameZRecoil/zUI/zui.cpp:11149`
- `0x4bf8b0` `HudUiPolyline::SetPoint` -> `src/GameZRecoil/zUI/zui.cpp:11214`
- `0x4bf900` `HudUiPolyline::Draw` -> `src/GameZRecoil/zUI/zui.cpp:11168`
- `0x4bf900` `HudUiPolyline::Draw` -> `src/GameZRecoil/zUI/zui.cpp:11177`
- `0x4bfc80` `HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget` -> `src/GameZRecoil/zUI/zui.cpp:7555`
- `0x4bfcd0` `HudUiBackgroundVideoWidget::Destructor` -> `src/GameZRecoil/zUI/zui.cpp:7580`
- `0x4bfd40` `HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh` -> `src/GameZRecoil/zUI/zui.cpp:7588`
- `0x4bfe20` `HudUiBackgroundVideoWidget::SetColorKey565` -> `src/GameZRecoil/zUI/zui.cpp:7637`
- `0x4bfe40` `HudUiBackgroundVideoWidget::Update` -> `src/GameZRecoil/zUI/zui.cpp:7651`
- `0x4bfe90` `HudUiBackgroundVideoWidget::Draw` -> `src/GameZRecoil/zUI/zui.cpp:7671`
- `0x4bfec0` `HudUiBackgroundVideoWidget::DrawBase` -> `src/GameZRecoil/zUI/zui.cpp:7689`
- `0x4bff00` `HudUiBackgroundVideoWidget::RebuildBltRect` -> `src/GameZRecoil/zUI/zui.cpp:7708`
- `0x4bffb0` `HudUiPrimitiveBindTarget::SetSegmentEndpoints` -> `src/GameZRecoil/zUI/zui.cpp:4720`
- `0x4bffe0` `zUtil_ZAR::RegisterSectionHandler` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:682`
- `0x4c0010` `zUtil_ZAR::WriteSectionBlob` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:706`
- `0x4c0030` `zUtil::ZBD_LoadEntriesGlobal` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:50`
- `0x4c0050` `zUtil::ZAR_LoadFileGlobal` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:66`
- `0x4c0070` `zUtil::ZAR_RequestStopGlobal` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:82`
- `0x4c0080` `zUtil_ZBD::OpenTempWriteStream` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:727`
- `0x4c00a0` `zUtil_ZBD::FlushTempWriteStreamToSectionRecord` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:760`
- `0x4c00c0` `zUtil_ZBD::OpenTempReadStream` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:740`
- `0x4c00e0` `zUtil_ZBD::CloseTempReadStream` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:780`
- `0x4c0100` `zUtil::ZBD_Init` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:94`
- `0x4c0180` `zUtil::ZBD_DestroyGlobalManager` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:120`
- `0x4c01b0` `zZbdManager::Destroy` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:261`
- `0x4c0260` `zZbdSectionHandler::CompareSortOrderLessThan` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:137`
- `0x4c0280` `zZbdManager::RegisterSectionHandler` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:290`
- `0x4c0370` `zZbdManager::LoadEntries` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:331`
- `0x4c0400` `zZbdManager::LoadZarFile` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:358`
- `0x4c0620` `zZbdManager::RequestStop` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:452`
- `0x4c0630` `zZbdManager::WriteSectionRecord` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:461`
- `0x4c06a0` `zZbdSectionHandler::InvokePreLoad` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:561`
- `0x4c06c0` `zZbdSectionHandler::InvokeDataReady` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:583`
- `0x4c0700` `zZbdManager::FlushTempStreamToSectionRecord` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:488`
- `0x4c0780` `zZbdManager::CreateTempReadStreamFromBuffer` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:527`
- `0x4c07c0` `zZbdManager::RemoveTempFiles` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:548`
- `0x4c07d0` `zZbdManager::SortSectionHandlers` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:612`
- `0x4c0b60` `zZbdSectionHandlerList::Front` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:162`
- `0x4c0b70` `zZbdSectionHandlerList::Constructor` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:149`
- `0x4c0ba0` `zZbdSectionHandlerList::Swap` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:173`
- `0x4c0bd0` `zZbdSectionHandlerList::Merge` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:216`
- `0x4c0ce0` `zZbdSectionHandlerList::SpliceThreeNodes` -> `src/GameZRecoil/zUtil/zutl_zbd.cpp:190`
- `0x4c0d20` `zInterp_Context::Constructor` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:617`
- `0x4c0e50` `zInterp_Context::Destructor` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:717`
- `0x4c0f70` `zInterp_Context::Destroy` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:679`
- `0x4c1020` `zInterp_Context::RunString` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1433`
- `0x4c1090` `zInterp_Context::RunStream` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1469`
- `0x4c1160` `zInterp_Context::ReadLineOrPreparedTokens` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1614`
- `0x4c1250` `zInterp_Context::ExpandMacroRefs` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:881`
- `0x4c13c0` `zInterp_Context::TokenizeLine` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1524`
- `0x4c1500` `zInterp_Context::RunScriptFile` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1369`
- `0x4c15f0` `zInterp_Context::FindMacroValue` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:491`
- `0x4c1670` `zInterp_Context::ClearMacroTable` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:580`
- `0x4c16c0` `zInterp_Context::ClearVarTable` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:599`
- `0x4c1710` `zInterp_Context::IsMacroTrue` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:517`
- `0x4c1780` `zInterp_Context::SetMacro` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:536`
- `0x4c1870` `zInterp_Context::EchoTokens` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1597`
- `0x4c18c0` `zInterp_Context::PushFileFrame` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1712`
- `0x4c1940` `zInterp_Context::PopFileFrame` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1695`
- `0x4c1960` `zInterp_Context::ClearFileFrameStack` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1681`
- `0x4c1990` `zInterp_Context::NextToken` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:961`
- `0x4c19c0` `zInterp_Context::ParseBoolToken` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:983`
- `0x4c1a00` `zInterp_Context::ParseFloatToken` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1004`
- `0x4c1a20` `zInterp_Context::ParseIntToken` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1019`
- `0x4c1a40` `zInterp_Context::FindVarEntry` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1034`
- `0x4c1ab0` `zInterp_Context::DumpVarEntry` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1056`
- `0x4c1b20` `zInterp_Context::IncErrorCount` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:468`
- `0x4c1b30` `zInterp_Context::Logf` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:417`
- `0x4c1b50` `zInterp_Context::EvalConditionExpr` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:827`
- `0x4c1c50` `zInterp_Context::HandleBuiltinCommand` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1737`
- `0x4c2030` `zInterp_Context::PrintNodeTree` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:4156`
- `0x4c2090` `zInterp_Context::ReportParseError` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:478`
- `0x4c20a0` `zInterp_Context::DispatchCoreCommand` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1924`
- `0x4c5480` `zInterp_Context::CommandEqualsPrefix` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1101`
- `0x4c54b0` `zInterp_Context::CommandEquals` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1123`
- `0x4c5510` `zInterp_Context::GetCurrentCommand` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1143`
- `0x4c5520` `zInterp_Context::ReportErrorf` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:442`
- `0x4c5550` `zInterp_Context::LoadPreparedScriptIndex` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1207`
- `0x4c5740` `zInterp_Context::OpenPreparedScriptStream` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1314`
- `0x4c5820` `zInterp_Context::ValidateArgsAndNodeType` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1157`
- `0x4c58c0` `zInterp_Context::DefaultDispatchHook` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:355`
- `0x4c58e0` `zInterp_Context::RegisterScrollAlwaysNode` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:761`
- `0x4c59e0` `zInterp_Object3D::DefaultRenderAction` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:376`
- `0x4c5a00` `zInterp_Object3D::ScrollAlwaysTickAction` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:393`

## WestwoodOnlineUpgradeConfigDialog.cpp

- `0x441a10` `WestwoodOnlineUpgradeConfigDialog::GetMessageMap` -> `src/Battlesport/wol_config_dialog_body.h:141`

## WinMain.cpp

- `0x4c81c0` `WinMain` -> `src/WinMain.cpp:15`

## zeff_anim.c

- `0x45db20` `zEffectAnim::CheckActivationPrereqs` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4323`
- `0x45e0d0` `zEffectAnimEntry::SetOnStateDoneCallback` -> `src/GameZRecoil/zEffect/zeff_anim_run.c:4647`
- `0x45e280` `zEffectAnim::FindSoundRefIndexByName` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:120`
- `0x45e300` `zEffectAnim::FindLightRefIndexByName` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:141`
- `0x45e5c0` `zEffectAnim::ResolveNodeByName` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:308`
- `0x45e650` `zEffectAnim::FindNodeRecursiveByName` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:360`
- `0x45ff10` `zEffectAnim::FindEntryByName` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:1466`
- `0x460010` `zEffectAnim::GetRootNodeOrNull` -> `src/GameZRecoil/zEffect/zeff_anim_init.c:1513`

## zRndr_Draw.cpp

- `0x498fb0` `zRndr_DrawCircleOutline16_Framebuffer` -> `src/GameZRecoil/zRender/zrndr_draw.c:9060`
- `0x499020` `zRndr_DrawCircleOctants16_Framebuffer` -> `src/GameZRecoil/zRender/zrndr_draw.c:8995`
