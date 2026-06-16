# Source File Map

Generated from address-backed `Reimplements 0xNNNNNN: Name (original/source/path)` provenance docblocks/comments in `src/`.
Binary Ninja remains authoritative; this map is an agent navigation aid.
It contains address-backed provenance docblocks, plus legacy line comments until touched source is converted.
It excludes helpers fully inlined by the retail compiler.

Entries: 3563

## Case-insensitive source path collisions

These original-source labels differ only by case on Windows; confirm placement against Binary Ninja before adding new code.

- `Battlesport/Hud.cpp`, `Battlesport/hud.cpp`
- `Battlesport/HudUi.cpp`, `Battlesport/hudui.cpp`
- `Battlesport/Net.cpp`, `Battlesport/net.cpp`
- `Battlesport/zOpt.cpp`, `Battlesport/zopt.cpp`
- `GameZRecoil/Player.cpp`, `GameZRecoil/player.cpp`
- `GameZRecoil/zMath/zMath.cpp`, `GameZRecoil/zMath/zmath.cpp`

## Battlesport/ai_net.cpp

- `0x402fd0` `AINet::LoadAllFromZrd` -> `src/Battlesport/ainet.cpp:22`
- `0x402ff0` `AINet::Alloc` -> `src/Battlesport/ainet.cpp:322`
- `0x403040` `AINet::LoadFromZrd` -> `src/Battlesport/ainet.cpp:32`
- `0x403510` `AINet::FindByNetId` -> `src/Battlesport/ainet.cpp:345`
- `0x403530` `AINet::FindNodeByIndex` -> `src/Battlesport/ainet.cpp:390`
- `0x403550` `AINet::ResolveNeighborLinksAndBuildProbeFans` -> `src/Battlesport/ainet.cpp:448`
- `0x403620` `AINetPathProbeFan::InitFromSegment` -> `src/Battlesport/ainet.cpp:409`
- `0x414390` `GameNet::RefreshPlayerListMenu` -> `src/Battlesport/GameNet.cpp:2877`
- `0x4143d0` `GameNet::BeginChatCompose` -> `src/Battlesport/GameNet.cpp:1584`
- `0x414550` `GameNet::ChatComposeKeyCallback` -> `src/Battlesport/GameNet.cpp:1568`
- `0x414590` `GameNet::EndChatComposeAndSend` -> `src/Battlesport/GameNet.cpp:1620`
- `0x433000` `GameNet::SendPkt08_PlayerKillEvent` -> `src/Battlesport/GameNet.cpp:3084`
- `0x433060` `GameNet::HandlePkt08_PlayerKillEvent` -> `src/Battlesport/GameNet.cpp:3044`
- `0x4340c0` `OptCatalog::AltGunDispatchAllocRuntimeGateCallback` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1286`
- `0x434130` `GameNet::SendPkt07_AltGunDispatch` -> `src/Battlesport/GameNet.cpp:2320`

## Battlesport/ainet.cpp

- `0x4036f0` `AINet::FindNearestNode` -> `src/Battlesport/ainet.cpp:363`
- `0x4037c0` `AINetNode::Free` -> `src/Battlesport/ainet.cpp:490`
- `0x403800` `AINet::Free` -> `src/Battlesport/ainet.cpp:511`
- `0x403870` `AINet::FreeAll` -> `src/Battlesport/ainet.cpp:530`

## Battlesport/AiPropertyDlg.cpp

- `0x41c0c0` `AiPropertyDlg::OnDestroy` -> `src/Battlesport/AiPropertyDlg.cpp:63`
- `0x41c130` `AiPropertyDlg::OnSelChange` -> `src/Battlesport/AiPropertyDlg.cpp:103`
- `0x41c170` `AiPropertyDlg::UpdatePropertyLabels` -> `src/Battlesport/AiPropertyDlg.cpp:125`

## Battlesport/Briefing.cpp

- `0x403930` `HudUiBriefingRuntime::Constructor` -> `src/Battlesport/Briefing.cpp:226`
- `0x403ed0` `HudUiBriefingRuntime::Destructor` -> `src/Battlesport/Briefing.cpp:377`
- `0x404070` `HudUiBriefingRuntime::Update` -> `src/Battlesport/Briefing.cpp:972`
- `0x404180` `Briefing::StartForMission` -> `src/Battlesport/Briefing.cpp:808`
- `0x404280` `Briefing::ThreadMain` -> `src/Battlesport/Briefing.cpp:855`
- `0x404bd0` `Briefing::StopAndShutdownThread` -> `src/Battlesport/Briefing.cpp:917`

## Battlesport/CZRecoilFrame.cpp

- `0x430c30` `CZRecoilFrame::OnMenuAbout` -> `src/Battlesport/CZRecoilFrame.cpp:680`
- `0x430d80` `CZRecoilFrame::OnMenuOpenMultiplayerSessionBrowser` -> `src/Battlesport/CZRecoilFrame.cpp:686`
- `0x4319a0` `CZRecoilFrame::OnMenuWestwoodOnlineUpgrade` -> `src/Battlesport/CZRecoilFrame.cpp:828`

## Battlesport/GameNet.cpp

- `0x41ada0` `NetSessionBrowserDialog::Constructor` -> `src/Battlesport/GameNet.cpp:806`
- `0x41ae90` `NetSessionBrowserDialog::ScalarDeletingDtor` -> `src/Battlesport/GameNet.cpp:828`
- `0x41aeb0` `NetSessionBrowserDialog::Destructor` -> `src/Battlesport/GameNet.cpp:840`
- `0x41af50` `NetSessionBrowserDialog::DoDataExchange` -> `src/Battlesport/GameNet.cpp:852`
- `0x41afd0` `NetSessionBrowserDialog::GetMessageMap` -> `src/Battlesport/GameNet.cpp:513`
- `0x41afe0` `NetSessionBrowserDialog::OnInitDialog` -> `src/Battlesport/GameNet.cpp:519`
- `0x41b2f0` `NetSessionBrowserDialog::ConnectSelectedProvider` -> `src/Battlesport/GameNet.cpp:984`
- `0x41b510` `NetSessionBrowserDialog::OnOK` -> `src/Battlesport/GameNet.cpp:1130`
- `0x41b5a0` `NetSessionBrowserDialog::OnCreateSession` -> `src/Battlesport/GameNet.cpp:1175`
- `0x41b660` `NetSessionBrowserDialog::OnTimer` -> `src/Battlesport/GameNet.cpp:1088`
- `0x41b680` `NetSessionBrowserDialog::OnDestroy` -> `src/Battlesport/GameNet.cpp:1213`
- `0x41b6a0` `NetSessionBrowserDialog::ValidatePlayerName` -> `src/Battlesport/GameNet.cpp:1097`
- `0x41b780` `NetSessionBrowserDialog::OnHelpDocs` -> `src/Battlesport/GameNet.cpp:1223`
- `0x41b8ac` `NetSessionBrowserDialog::kHelpDocsFindExecutableErrorClassTable` -> `src/Battlesport/GameNet.cpp:268`
- `0x41c6e0` `NetSessionConfigDialog::Constructor` -> `src/Battlesport/GameNet.cpp:1292`
- `0x41c7f0` `NetSessionConfigDialog::Destructor` -> `src/Battlesport/GameNet.cpp:1316`
- `0x41c880` `NetSessionConfigDialog::DoDataExchange` -> `src/Battlesport/GameNet.cpp:1327`
- `0x41c970` `NetSessionConfigDialog::GetMessageMap` -> `src/Battlesport/GameNet.cpp:1402`
- `0x41c980` `Mission::RegisterMultiplayerMaps` -> `src/Battlesport/GameNet.cpp:1533`
- `0x41c990` `NetSessionConfigDialog::InitMapNameStrings` -> `src/Battlesport/GameNet.cpp:1541`
- `0x41ca00` `NetSessionConfigDialog::RegisterMapNameCleanup` -> `src/Battlesport/GameNet.cpp:1553`
- `0x41ca10` `NetSessionConfigDialog::CleanupMapNameStringsOnExit` -> `src/Battlesport/GameNet.cpp:1559`
- `0x41ca30` `NetSessionConfigDialog::OnInitDialog` -> `src/Battlesport/GameNet.cpp:1408`
- `0x41cb50` `NetSessionConfigDialog::OnDestroy` -> `src/Battlesport/GameNet.cpp:1484`
- `0x41cb90` `NetSessionConfigDialog::OnMapChanged` -> `src/Battlesport/GameNet.cpp:1503`

## Battlesport/hud.cpp

- `0x404d10` `HudUiElement::HitTestTrue` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5170`
- `0x406af0` `HudCheat::ExecuteCommandString` -> `src/Battlesport/hud.cpp:1689`
- `0x406cf0` `HudCheat::ClearNanitePanelCheatSentinel` -> `src/Battlesport/hud.cpp:1811`
- `0x407100` `HudUiCallback::QueueExitCurrentState` -> `src/Battlesport/hud.cpp:1653`
- `0x407110` `HudUiCallback::QueueCheatCodeState` -> `src/Battlesport/hud.cpp:1659`
- `0x40d2a0` `HudLayoutSW::GlobalDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:228`
- `0x40d330` `HudLayoutHW::GlobalDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:271`
- `0x40d400` `HudUiMgr::StaticInitAndRegisterAtExit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3327`
- `0x40d410` `HudUiMgr::StaticInit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3334`
- `0x40d420` `HudUiMgr::RegisterAtExit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3339`
- `0x40d430` `HudUiMgr::AtExitDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3344`
- `0x40d440` `HudUiMgr::StaticDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3349`
- `0x40d7e0` `HudUiMgr::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3264`
- `0x40dcd0` `HudUiTriplet::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16729`
- `0x40eb00` `HudUiShieldMessageWidget::ApplyLayout` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14457`
- `0x40f2e0` `HudUiNanitePanel::InitLayout` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13591`
- `0x410160` `HudUiMgr::EnsureHudLoaded` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3456`
- `0x410e90` `HudUiMgr::EnableHud` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4300`
- `0x410fe0` `HudUiMgr::UpdateFrame` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4369`
- `0x411170` `HudUiMgr::ProjectPointToNormalizedClamped` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3391`
- `0x411270` `HudUiMgr::UpdateTargetReticleFromCursor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4092`
- `0x411760` `HudUiMgrObjective::SetVisibleAndResetMeterFill` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2756`
- `0x4117f0` `HudUiMgrObjective::TickMeterFillAnimation` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2792`
- `0x4118b0` `HudUiMgrObjective::UpdateMeterXPoints` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2778`
- `0x411900` `HudUiMgrObjective::Show` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2862`
- `0x411ac0` `HudUiMgrObjective::StartHide` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2939`
- `0x411f10` `HudUiMgrSensor::SetShieldMessageRatio` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2586`
- `0x412050` `HudUiMgrObjective::RefreshCounterText` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2743`
- `0x4124b0` `HudUiMgrTarget::UpdateSelectedProgressMeter` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2678`
- `0x412620` `HudUiMgr::HideTrackedProgressMeterIfOwnerMatches` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4472`
- `0x412650` `HudUiMessage::SetValueIfOwnerMatches` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14345`
- `0x412820` `HudUiMessage::UpdateSelectedWeaponDisplay` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14369`
- `0x412b60` `HudLayoutSW::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:203`
- `0x412c10` `HudLayoutSW::LoadTypeIFromZarRoot` -> `src/GameZRecoil/zHud/zhud_ui.cpp:392`
- `0x412c60` `HudLayoutSW::SetActive` -> `src/GameZRecoil/zHud/zhud_ui.cpp:327`
- `0x412db0` `HudLayout::ApplyViewportRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:416`
- `0x412ea0` `HudLayoutHW::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:235`
- `0x412f70` `HudLayoutHW::LoadTypeIIFromZarRoot` -> `src/GameZRecoil/zHud/zhud_ui.cpp:698`
- `0x4130d0` `HudLayoutHW::SetActive` -> `src/GameZRecoil/zHud/zhud_ui.cpp:592`
- `0x4132b0` `HudLayoutHW::UpdateObjectiveDirtyRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:569`
- `0x413340` `HudLayoutHW::OnActivated` -> `src/GameZRecoil/zHud/zhud_ui.cpp:501`
- `0x4134e0` `HudUiMessage::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14166`
- `0x413500` `HudLayoutHW::UpdateAll` -> `src/GameZRecoil/zHud/zhud_ui.cpp:312`
- `0x413540` `HudLayoutHW::Enable` -> `src/GameZRecoil/zHud/zhud_ui.cpp:762`
- `0x4135f0` `HudLayoutHW::Disable` -> `src/GameZRecoil/zHud/zhud_ui.cpp:794`
- `0x413600` `zOpt::ToggleHudTypeForCurrentHwMode` -> `src/Battlesport/hud.cpp:1831`
- `0x413640` `HudUiMgr::ToggleHud` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4358`
- `0x413660` `HudUiMgr::SwitchActiveDialog` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4436`
- `0x4136b0` `HudUiMgr::ApplyHudModeSwitch` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4497`
- `0x413770` `HudUiMgr::SetFloatTimerVisible` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4460`
- `0x4137a0` `HudUiMgr::SetAuxOverlayVisible` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4489`
- `0x4138d0` `HudUi::ShowTopMessageLine` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17479`
- `0x4138f0` `HudUi::ShowChatLine` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17497`
- `0x413990` `HudUiLayoutNode::ApplyTextLabel` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1672`
- `0x413a10` `HudUiLayoutNode::ReadRectOffsetAndSize` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1635`
- `0x413aa0` `HudUiLayoutNode::ReadRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1462`
- `0x413ad0` `HudUiLayoutNode::ReadInt3` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1480`
- `0x413b10` `HudUiLayoutNode::ApplyCornerTextQuad` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1508`
- `0x413c10` `HudUiLayoutNode::ApplyMeterQuad` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1568`
- `0x413d30` `HudUiLayoutNode::ApplyImageWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1704`
- `0x414070` `HudUiMessage::RebuildWeaponLayout` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14173`
- `0x414300` `HudUiMgrSensor::GetFxRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2669`
- `0x415fb0` `HudRectI::ClipOrSplitSegment` -> `src/Battlesport/HudSensorTracker.cpp:492`
- `0x416240` `HudRectI::CalcOutcode` -> `src/Battlesport/HudSensorTracker.cpp:392`
- `0x416290` `HudRectI::IsCornerOutcode` -> `src/Battlesport/HudSensorTracker.cpp:413`
- `0x4162b0` `HudRectI::SegmentIntersectsEdge` -> `src/Battlesport/HudSensorTracker.cpp:421`
- `0x416390` `HudGeom2D::ClassifyPointAgainstSegment` -> `src/Battlesport/HudSensorTracker.cpp:192`
- `0x4184e0` `HudSensorTracker::AdvanceObjectiveState` -> `src/Battlesport/HudSensorTracker.cpp:2856`
- `0x418620` `HudSensorTracker::SetObjectiveReviewVisible` -> `src/Battlesport/HudSensorTracker.cpp:2805`
- `0x418760` `HudSensorTracker::SetObjectivePanelVisible` -> `src/Battlesport/HudSensorTracker.cpp:3000`
- `0x419380` `HudSensorTracker::OnObjectiveReadSoundEvent` -> `src/Battlesport/HudSensorTracker.cpp:2841`
- `0x42bf40` `HudUi::PlayPowerupSfx` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17458`
- `0x4348b0` `HudUiSaveLoadGameNameInput::OnActivate` -> `src/Battlesport/RecoilApp.cpp:343`
- `0x4348f0` `HudUiSaveLoadGameNameInput::OnRawKeyboardEvent` -> `src/Battlesport/RecoilApp.cpp:354`
- `0x434950` `HudUiSaveLoadListItem::Draw` -> `src/Battlesport/RecoilApp.cpp:260`
- `0x4349a0` `HudUiSaveLoadDialog::Destructor` -> `src/Battlesport/RecoilApp.cpp:1194`
- `0x434a80` `HudUiSaveGameDialog::Destructor` -> `src/Battlesport/RecoilApp.cpp:1105`
- `0x435220` `HudUiSaveGamePrimaryActionButton::OnActivate` -> `src/Battlesport/RecoilApp.cpp:413`
- `0x4bcf80` `HudUiBar::SetPointXY` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14554`
- `0x4bd280` `HudUi::PushTopMessageLine` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17529`
- `0x4bd470` `zTimedTask::RemoveFromActiveList` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17165`
- `0x4bd4d0` `zTimedTask::RunImmediateAction` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17195`
- `0x4bd660` `zTimedTask::TickActiveList` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17303`
- `0x4bd6f0` `HudLineClip::SetCurrentBoundsFromRectI` -> `src/Battlesport/HudSensorTracker.cpp:247`
- `0x4bd840` `HudLineClip::ClipSegmentToCurrentBounds` -> `src/Battlesport/HudSensorTracker.cpp:366`
- `0x4bd880` `HudLineClip::ClipSegmentToCurrentXBounds` -> `src/Battlesport/HudSensorTracker.cpp:258`
- `0x4bd9c0` `HudLineClip::ClipEndpointToX` -> `src/Battlesport/HudSensorTracker.cpp:223`
- `0x4bd9f0` `HudLineClip::ClipSegmentToCurrentYBounds` -> `src/Battlesport/HudSensorTracker.cpp:312`
- `0x4bdb30` `HudLineClip::ClipEndpointToY` -> `src/Battlesport/HudSensorTracker.cpp:235`
- `0x4bdc70` `HudWeatherFx::Constructor` -> `src/Battlesport/hud.cpp:93`
- `0x4bde40` `HudWeatherFx::Destructor` -> `src/Battlesport/hud.cpp:189`
- `0x4bdee0` `HudWeatherFx::ResetParticleSlot` -> `src/Battlesport/hud.cpp:242`
- `0x4bdfd0` `HudWeatherFx::DrawParticles` -> `src/Battlesport/hud.cpp:268`
- `0x4be210` `HudWeatherFx::ArePointBatchInsideRect` -> `src/Battlesport/hud.cpp:214`
- `0x4be280` `HudWeatherFxSnow::Constructor` -> `src/Battlesport/hud.cpp:363`
- `0x4be2f0` `HudWeatherFxSnow::Update` -> `src/Battlesport/hud.cpp:393`
- `0x4be810` `HudWeatherFxRain::Constructor` -> `src/Battlesport/hud.cpp:551`
- `0x4be870` `HudWeatherFxRain::Destructor` -> `src/Battlesport/hud.cpp:575`
- `0x4be880` `HudWeatherFxRain::Update` -> `src/Battlesport/hud.cpp:581`

## Battlesport/Hud.cpp

- `0x439b20` `HudLowMeterLoopSound::SetLoopActive` -> `src/Battlesport/hud.cpp:1848`
- `0x439b70` `HudLowMeterLoopSound::Disable` -> `src/Battlesport/hud.cpp:1868`

## Battlesport/hud_ui_dialogs.cpp

- `0x408a30` `HudUiControlsDialog::Constructor` -> `src/Battlesport/hud.cpp:1562`
- `0x408c20` `HudUiControlsDialog_CommandsWidget::OnActivate` -> `src/Battlesport/hud.cpp:1555`
- `0x408c40` `HudUiControlsDialog::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:1640`
- `0x408c70` `HudUiControlsDialog::Destructor` -> `src/Battlesport/hud.cpp:1627`

## Battlesport/HudCmdBindButton.cpp

- `0x40bdc0` `zUtil_StdPtrVector_Clear` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10826`
- `0x40c280` `HudCmdBindButtonBase::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10890`
- `0x4b8de0` `HudCmdBindButtonBase::LoadFromZrd` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12807`
- `0x4b90e0` `HudCmdBindButtonBase::RebuildBindingSlotWidgets` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12746`
- `0x4b9320` `HudCmdBindButtonBase::OnSelectedIndexChanged` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10730`
- `0x4b9330` `HudCmdBindButtonBase::SetSelectedEntry` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10738`

## Battlesport/HudCmdDialog.cpp

- `0x40a5b0` `HudCmdDialog::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11292`
- `0x40a920` `HudCmdDialog::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11446`
- `0x40adf0` `HudCmdDialog::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11424`
- `0x40b140` `HudCmdDialog::UpdateCaptureState` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12514`
- `0x40b3e0` `HudCmdDialog::ApplyPrimaryKeyRebind` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12619`
- `0x40b460` `HudCmdDialog::ApplySecondaryKeyRebind` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12652`
- `0x40b4e0` `HudCmdDialog::ApplyJoystickButtonRebind` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12685`
- `0x40b560` `HudCmdDialog::ApplyMouseButtonRebind` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12715`
- `0x40b5e0` `HudCmdDialog::SelectGroupRelative` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12139`
- `0x40b630` `HudCmdDialog::SelectCommandRelative` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12157`
- `0x40b680` `HudCmdDialog::RebuildCommandBindingListsForGroup` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12334`
- `0x40b930` `HudCmdResetButton::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12183`
- `0x40b960` `HudCmdSetListWidget::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12194`
- `0x40b980` `HudCmdDialog::OnCommandSelectionChanged` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12482`
- `0x40ba30` `HudCmdKeyAButton::OnBeginCapture` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12204`
- `0x40bab0` `HudCmdKeyBButton::OnBeginCapture` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12237`
- `0x40bb00` `HudCmdJoyButton::OnBeginCapture` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12257`
- `0x40bb50` `HudCmdMouseButton::OnBeginCapture` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12278`
- `0x40bba0` `HudCmdNextSetButton::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12306`
- `0x40bbc0` `HudCmdPrevSetButton::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12313`
- `0x40bbe0` `HudCmdNextCommandButton::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12320`
- `0x40bc00` `HudCmdPrevCommandButton::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12327`
- `0x40bc20` `HudCmdDialogState::StaticInitAndRegisterAtExit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12025`
- `0x40bc30` `HudCmdDialogState::StaticInit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12035`
- `0x40bc40` `HudCmdDialogState::RegisterAtExit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12044`
- `0x40bc50` `HudCmdDialogState::AtExitDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12053`
- `0x40bc60` `HudCmdDialogState::HudCmdDialogState` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12074`
- `0x40bc90` `HudCmdDialogState::~HudCmdDialogState` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12127`
- `0x40bcf0` `HudCmdDialogState::OnTryBecomeCurrent` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12083`
- `0x40bd60` `HudCmdDialogState::OnDeactivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12101`
- `0x40bda0` `HudCmdDialogState::QueueEnter` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12062`

## Battlesport/HudConfirmQuitDialog.cpp

- `0x415740` `HudUiConfirmQuitOkButton::OnActivate` -> `src/Battlesport/hud.cpp:1093`
- `0x415810` `RecoilStateConfirmQuit::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:1067`
- `0x415820` `RecoilStateConfirmQuit::StaticInit` -> `src/Battlesport/hud.cpp:1074`
- `0x415830` `RecoilStateConfirmQuit::RegisterAtExit` -> `src/Battlesport/hud.cpp:1080`
- `0x415840` `RecoilStateConfirmQuit::AtExitDestructor` -> `src/Battlesport/hud.cpp:1086`
- `0x415850` `RecoilStateConfirmQuit::RecoilStateConfirmQuit` -> `src/Battlesport/hud.cpp:1350`
- `0x4158f0` `RecoilStateConfirmQuit::OnTryBecomeCurrent` -> `src/Battlesport/hud.cpp:1358`
- `0x415960` `RecoilStateConfirmQuit::OnDeactivate` -> `src/Battlesport/hud.cpp:1373`
- `0x4159b0` `RecoilStateConfirmQuit::QueueEnter` -> `src/Battlesport/hud.cpp:1035`

## Battlesport/HudOptionsDialog.cpp

- `0x40c6e0` `HudUiOptionsPanelBackButton::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11460`
- `0x40c720` `HudOptionsDialog::HudOptionsDialog` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11908`
- `0x40cf00` `HudOptionsDialog::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12009`
- `0x40cf60` `HudOptionsDialog::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11987`
- `0x40d070` `HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:960`
- `0x40d080` `HudUiOptionsPanelOverlayOwner::StaticInit` -> `src/Battlesport/hud.cpp:970`
- `0x40d090` `HudUiOptionsPanelOverlayOwner::RegisterAtExit` -> `src/Battlesport/hud.cpp:979`
- `0x40d0a0` `HudUiOptionsPanelOverlayOwner::AtExitDestructor` -> `src/Battlesport/hud.cpp:988`
- `0x40d0b0` `HudUiOptionsPanelOverlayOwner::HudUiOptionsPanelOverlayOwner` -> `src/Battlesport/hud.cpp:997`
- `0x40d0e0` `HudUiOptionsPanelOverlayOwner::~HudUiOptionsPanelOverlayOwner` -> `src/Battlesport/hud.cpp:1006`
- `0x40d150` `HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent` -> `src/Battlesport/hud.cpp:1025`
- `0x40d1c0` `HudUiOptionsPanelOverlayOwner::QueueEnter` -> `src/Battlesport/hud.cpp:948`

## Battlesport/HudScoreboard.cpp

- `0x40eab0` `HudScoreboard::SetScaleAndRebuild` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17097`
- `0x40eae0` `HudScoreboard::DispatchSetScale` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17109`

## Battlesport/HudSensorTracker.cpp

- `0x416650` `HudSensorTracker::InitNoBounds` -> `src/Battlesport/HudSensorTracker.cpp:861`
- `0x416660` `HudSensorTracker::Init` -> `src/Battlesport/HudSensorTracker.cpp:831`
- `0x4166e0` `HudSensorTracker::SetBounds` -> `src/Battlesport/HudSensorTracker.cpp:902`

## Battlesport/HudUi.cpp

- `0x414330` `GameNet::ShowPlayerKillMessage` -> `src/Battlesport/GameNet.cpp:2752`
- `0x4143a0` `HudUiMgr::IsLocalPlayerFirstInStatsList` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3450`
- `0x4143b0` `HudUi::RefreshScoreboardEntryRow` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17514`
- `0x4143c0` `HudUi::RemoveScoreboardEntryRow` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17521`
- `0x41ab60` `HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15277`
- `0x41ab70` `HudUiNetGameSetupOverlayOwner::StaticInit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15284`
- `0x41ab80` `HudUiNetGameSetupOverlayOwner::RegisterAtExit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15290`
- `0x41ab90` `HudUiNetGameSetupOverlayOwner::AtExitDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15296`
- `0x41aba0` `HudUiNetGameSetupOverlayOwner::HudUiNetGameSetupOverlayOwner` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15302`
- `0x41abe0` `HudUiNetGameSetupOverlayOwner::~HudUiNetGameSetupOverlayOwner` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15308`
- `0x41ac50` `HudUiNetGameSetupOverlayOwner::OnTryBecomeCurrent` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15324`
- `0x41ad20` `HudUiNetGameSetupOverlayOwner::OnDeactivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15361`

## Battlesport/hudui.cpp

- `0x426150` `HudUi::HandleHotkeyCommand` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17358`
- `0x4b3e90` `HudUiWidget::InvalidateRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7931`
- `0x4b3fb0` `HudUiWidget::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13368`
- `0x4bc760` `HudUi::SetInvalidateMode` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17330`

## Battlesport/hudui_background.cpp

- `0x4b9540` `HudUiBackground::HudUiBackground` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6984`
- `0x4b9740` `HudUiBackground::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7052`
- `0x4b9760` `HudUiBackground::~HudUiBackground` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7030`
- `0x4b98d0` `HudUiBackground::LoadFromZrd` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7114`
- `0x4b9900` `HudUiBackground::LoadZrdAndSection` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7134`
- `0x4ba020` `HudUiTransitionTextPanel::HudUiTransitionTextPanel` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5543`
- `0x4bf980` `HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7994`
- `0x4bfa20` `HudUiBackgroundCursorWidget::~HudUiBackgroundCursorWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8012`
- `0x4bfa50` `HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8022`
- `0x4bfa70` `HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8032`
- `0x4bfa90` `HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8042`
- `0x4bfae0` `HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8063`
- `0x4bfb70` `HudUiBackgroundCursorWidget::SetPos` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8100`
- `0x4bfba0` `HudUiBackgroundCursorWidget::RebuildCapturedImage` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8116`
- `0x4bfc50` `HudUiBackgroundCursorWidget::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8151`
- `0x4bfc60` `HudUiBackgroundCursorWidget::DrawBase` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8157`
- `0x4bfc80` `HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8171`
- `0x4bfcd0` `HudUiBackgroundVideoWidget::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8189`
- `0x4bfd40` `HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8195`
- `0x4bfe40` `HudUiBackgroundVideoWidget::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8256`
- `0x4bfe90` `HudUiBackgroundVideoWidget::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8274`
- `0x4bfec0` `HudUiBackgroundVideoWidget::DrawBase` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8290`
- `0x4bff00` `HudUiBackgroundVideoWidget::RebuildBltRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8307`

## Battlesport/hudui_element.cpp

- `0x4b4070` `HudUiElement::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4865`

## Battlesport/HudUi_NetExit.cpp

- `0x41bd80` `HudUiNetExitPanel::Constructor` -> `src/Battlesport/HudUiNetExitPanel.cpp:14`
- `0x41beb0` `HudUiNetExitPanel::Destructor` -> `src/Battlesport/HudUiNetExitPanel.cpp:54`
- `0x41c000` `HudUiNetExitPanel::CreateGlobal` -> `src/Battlesport/HudUiNetExitPanel.cpp:144`

## Battlesport/hudui_saveload.cpp

- `0x434680` `HudUiSaveGameDialog::HudUiSaveGameDialog` -> `src/Battlesport/RecoilApp.cpp:1041`
- `0x434920` `HudUiSaveLoadListItem::HudUiSaveLoadListItem` -> `src/Battlesport/RecoilApp.cpp:245`
- `0x434ee0` `HudUiSaveLoadDialog::InitializeFileEntries` -> `src/Battlesport/RecoilApp.cpp:762`
- `0x4353f0` `HudUiSaveLoadDialog::SetSelectedEntryIndex` -> `src/Battlesport/RecoilApp.cpp:799`
- `0x4355e0` `HudUiSaveLoadDialog::RefreshSaveFileList` -> `src/Battlesport/RecoilApp.cpp:591`
- `0x4362f0` `SortEntryRange` -> `src/Battlesport/RecoilApp.cpp:498`
- `0x436530` `InsertEntryIntoSortedPrefix` -> `src/Battlesport/RecoilApp.cpp:441`
- `0x436580` `PartitionEntriesByPivot` -> `src/Battlesport/RecoilApp.cpp:462`

## Battlesport/HudUiBackgroundConfirmQuit.cpp

- `0x415680` `HudUiBackgroundConfirmQuit::Constructor` -> `src/Battlesport/hud.cpp:1110`
- `0x415790` `HudUiBackgroundConfirmQuit::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:1153`
- `0x4157b0` `HudUiBackgroundConfirmQuit::Destructor` -> `src/Battlesport/hud.cpp:1142`

## Battlesport/HudUiCheatCode.cpp

- `0x406d20` `HudUiCheatCodeDialog::HudUiCheatCodeDialog` -> `src/Battlesport/hud.cpp:1180`
- `0x406e10` `HudUiCheatCodeDialog::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:1224`
- `0x406e30` `HudUiCheatCodeDialog::Destructor` -> `src/Battlesport/hud.cpp:1213`
- `0x406e90` `RecoilStateCheatCode::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:1240`
- `0x406ea0` `RecoilStateCheatCode::ConstructGlobal` -> `src/Battlesport/hud.cpp:1247`
- `0x406eb0` `RecoilStateCheatCode::StaticInit` -> `src/Battlesport/hud.cpp:1253`
- `0x406ec0` `RecoilStateCheatCode::AtExitDestructor` -> `src/Battlesport/hud.cpp:1259`
- `0x406ed0` `RecoilStateCheatCode::RecoilStateCheatCode` -> `src/Battlesport/hud.cpp:1266`
- `0x406f00` `RecoilStateCheatCode::Destructor` -> `src/Battlesport/hud.cpp:1338`
- `0x4070e0` `HudUiCheatCodeTitleWidget::OnActivate` -> `src/Battlesport/hud.cpp:1170`

## Battlesport/HudUiCreditsPanel.cpp

- `0x409040` `HudUiCreditsPanel::HudUiCreditsPanel` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6173`
- `0x409570` `HudUiZrdScrollingText::LoadFromZrd` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6227`
- `0x409b90` `HudUiPanelSpan::InsertN` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6751`
- `0x409f00` `HudUiPanelSpanVec::InsertN` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6842`

## Battlesport/HudUiListMenu.cpp

- `0x40d220` `HudUiListMenuEntry::CompareSortKey` -> `src/GameZRecoil/zHud/zhud_ui.cpp:953`
- `0x414710` `HudUiListMenuEntry::SortRange` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1086`
- `0x414930` `HudUiListMenuEntry::InsertPivotIntoSortedPrefix` -> `src/GameZRecoil/zHud/zhud_ui.cpp:985`
- `0x414980` `HudUiListMenuEntry::InsertionSortRange` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1008`

## Battlesport/HudUiLoadGameDialog.cpp

- `0x434dc0` `HudUiLoadGameDialog::ProcessDialogResult` -> `src/Battlesport/RecoilApp.cpp:958`
- `0x434df0` `HudUiLoadGameDialog::Destructor` -> `src/Battlesport/RecoilApp.cpp:1217`

## Battlesport/HudUiMainMenuDialog.cpp

- `0x414b60` `HudUiMainMenuDialog::CanLoadGame` -> `src/Battlesport/HudUiMainMenuDialog.cpp:473`
- `0x414b90` `HudUiMainMenuDialog::CanSaveGame` -> `src/Battlesport/HudUiMainMenuDialog.cpp:498`
- `0x414bc0` `HudUiMainMenuDialog::HudUiMainMenuDialog` -> `src/Battlesport/HudUiMainMenuDialog.cpp:523`
- `0x414f40` `HudUiMainMenuDialog_CreditsButton::OnActivate` -> `src/Battlesport/HudUiMainMenuDialog.cpp:15`
- `0x414f60` `HudUiMainMenuDialog_SaveButton::OnActivate` -> `src/Battlesport/HudUiMainMenuDialog.cpp:36`
- `0x414f80` `HudUiMainMenuDialog_NewGameButton::OnActivate` -> `src/Battlesport/HudUiMainMenuDialog.cpp:64`
- `0x414fa0` `HudUiMenuBackButton::OnActivate` -> `src/Battlesport/HudUiMainMenuDialog.cpp:25`
- `0x414fc0` `HudUiMainMenuDialog_OptionsButton::OnActivate` -> `src/Battlesport/HudUiMainMenuDialog.cpp:74`
- `0x414fe0` `HudUiMainMenuDialog_QuitButton::OnActivate` -> `src/Battlesport/HudUiMainMenuDialog.cpp:84`
- `0x415000` `HudUiMainMenuDialog_ControlsButton::OnActivate` -> `src/Battlesport/HudUiMainMenuDialog.cpp:94`
- `0x415040` `HudUiMainMenuDialog::~HudUiMainMenuDialog` -> `src/Battlesport/HudUiMainMenuDialog.cpp:725`
- `0x415140` `HudUiMainMenuDialog_LoadButton::OnActivate` -> `src/Battlesport/HudUiMainMenuDialog.cpp:48`

## Battlesport/HudUiMessageBoxDialog.cpp

- `0x438350` `HudUi::ShowMessageBox` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17337`

## Battlesport/HudUiMgrSensor.cpp

- `0x412070` `HudUiMgrSensor::PlaceTrackCounterWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2233`
- `0x4122c0` `HudUiMgrSensor::PlaceTrackMarker` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2353`
- `0x41ebd0` `HudUiMgrSensor::TrackList_Reset` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2193`
- `0x438920` `HudUiMgrSensor::TrackList_Add` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2203`
- `0x439690` `HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2443`

## Battlesport/HudUiMpExitDialog.cpp

- `0x419500` `HudUiMpExitDialog::LoadLayout` -> `src/Battlesport/HudUiMpExitDialog.cpp:81`
- `0x419650` `HudUiMpExitDialog::UnloadLayout` -> `src/Battlesport/HudUiMpExitDialog.cpp:22`
- `0x419690` `HudUiMpExitDialog::Update` -> `src/Battlesport/HudUiMpExitDialog.cpp:40`
- `0x419740` `RecoilApp_MpExitDialogState::OnEnter` -> `src/Battlesport/HudUiMpExitDialog.cpp:217`
- `0x419800` `HudUiMpExitDialog_MpNewGameButton::OnActivate` -> `src/Battlesport/HudUiMpExitDialog.cpp:163`
- `0x419830` `HudUiMpExitDialog_MpExitButton::OnActivate` -> `src/Battlesport/HudUiMpExitDialog.cpp:177`
- `0x419850` `HudUiMpExitDialog::ScalarDeletingDestructorThunk` -> `src/Battlesport/HudUiMpExitDialog.cpp:201`
- `0x419870` `HudUiMpExitDialog::Destructor` -> `src/Battlesport/HudUiMpExitDialog.cpp:190`
- `0x4198d0` `RecoilApp_MpExitDialogState::OnTryBecomeCurrent` -> `src/Battlesport/HudUiMpExitDialog.cpp:237`
- `0x419940` `RecoilApp_MpExitDialogState::OnDeactivate` -> `src/Battlesport/HudUiMpExitDialog.cpp:264`
- `0x419990` `RecoilApp_MpExitDialogState::OnUpdateShouldQuit` -> `src/Battlesport/HudUiMpExitDialog.cpp:284`

## Battlesport/HudUiNewGamePanel.cpp

- `0x41c270` `HudUiNewGamePanel_StartButton::OnActivate` -> `src/Battlesport/hud.cpp:934`
- `0x41c290` `HudUiNewGamePanel::HudUiNewGamePanel` -> `src/Battlesport/hud.cpp:826`
- `0x41c3b0` `HudUiNewGamePanel_NameInput::OnActivate` -> `src/Battlesport/hud.cpp:866`
- `0x41c3e0` `HudUiNewGamePanel::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:891`
- `0x41c400` `HudUiNewGamePanel::Destructor` -> `src/Battlesport/hud.cpp:878`
- `0x41c4e0` `HudUiNewGamePanel::SyncIntensityFromDifficulty` -> `src/Battlesport/hud.cpp:907`
- `0x41c500` `HudUiNewGamePanel::StartGameFromFields` -> `src/Battlesport/hud.cpp:916`
- `0x41c560` `HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent` -> `src/Battlesport/hud.cpp:814`
- `0x41c5e0` `HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:749`
- `0x41c5f0` `HudUiNewGamePanelOverlayOwner::StaticInit` -> `src/Battlesport/hud.cpp:759`
- `0x41c630` `HudUiNewGamePanelOverlayOwner::~HudUiNewGamePanelOverlayOwner` -> `src/Battlesport/hud.cpp:795`
- `0x41c6a0` `HudUiNewGamePanelOverlayOwner::RegisterAtExit` -> `src/Battlesport/hud.cpp:768`
- `0x41c6b0` `HudUiNewGamePanelOverlayOwner::AtExitDestructor` -> `src/Battlesport/hud.cpp:777`
- `0x41c6c0` `HudUiNewGamePanelOverlayOwner::QueueEnter` -> `src/Battlesport/hud.cpp:737`

## Battlesport/HudUiPanel.cpp

- `0x409910` `HudUiPanelSpan::Clear` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6625`
- `0x409b60` `HudUiPanelLayoutEntry::DestroyRange` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6609`
- `0x40a170` `HudUiPanelLayoutEntry::CopyAssignRange` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6586`
- `0x40a1e0` `HudUiPanelLayoutEntry::CopyAssign` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6572`
- `0x40a210` `HudUiPanelLayoutEntry::CopyConstruct` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6558`
- `0x40a240` `HudUiPanelSpan::CopyInit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6643`
- `0x40a300` `HudUiPanelSpan::CopyFrom` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6673`
- `0x4ba850` `HudUiPanel::CopyConstructCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15684`
- `0x4ba9e0` `HudUiPanel::ConstructorCopy` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15733`
- `0x4babb0` `HudUiPanel::SetFont` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16017`
- `0x4bc9b0` `HudUiTransitionTextPanel::SetFlashColorAndRate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9804`

## Battlesport/HudUiSaveLoadDialog.cpp

- `0x434970` `HudUiLoadGameDialog::OnPrimaryActionThunk` -> `src/Battlesport/RecoilApp.cpp:967`
- `0x434b90` `HudUiLoadGameDialog::HudUiLoadGameDialog` -> `src/Battlesport/RecoilApp.cpp:1130`
- `0x434fb0` `HudUiSaveLoadDialog::DeleteSaveFile` -> `src/Battlesport/RecoilApp.cpp:282`
- `0x435140` `HudUiSaveLoadDeleteButton::OnActivate` -> `src/Battlesport/RecoilApp.cpp:372`
- `0x435160` `HudUiSaveLoadNextButton::OnActivate` -> `src/Battlesport/RecoilApp.cpp:383`
- `0x4351b0` `HudUiSaveLoadPrevButton::OnActivate` -> `src/Battlesport/RecoilApp.cpp:398`
- `0x435200` `HudUiLoadGamePrimaryActionButton::OnActivate` -> `src/Battlesport/RecoilApp.cpp:427`
- `0x435240` `HudUiLoadGameDialog::OnPrimaryAction` -> `src/Battlesport/RecoilApp.cpp:976`
- `0x435a10` `HudUiSaveLoadListItem::OnActivate` -> `src/Battlesport/RecoilApp.cpp:270`
- `0x435a70` `HudUiSaveLoadDialog::ProcessDialogResult` -> `src/Battlesport/RecoilApp.cpp:889`

## Battlesport/HudUiTextLabel.cpp

- `0x4bcbe0` `HudUiTextLabel::CopyConstructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15453`
- `0x4bcc80` `HudUiTextLabel::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15475`

## Battlesport/HudUiTextStack4.cpp

- `0x4bd110` `HudUiTextStack4::SetFontAll` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17573`
- `0x4bd160` `HudUiTextStack4::PushLine` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17123`

## Battlesport/HudUiTransitionTextPanel.cpp

- `0x4bc9f0` `HudUiTransitionTextPanel::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9688`

## Battlesport/HudUiTriplet.cpp

- `0x40e070` `HudUiTriplet::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16787`
- `0x40e140` `HudUiTriplet::RebuildDisplay` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16825`
- `0x40e590` `HudUiTriplet::AddEntry` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16990`
- `0x40e800` `HudUiTriplet::UpdateEntryData` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17023`
- `0x40e880` `HudUiTriplet::RemoveEntry` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17046`
- `0x40e910` `HudUiTriplet::InterpolateLayout` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13680`
- `0x40ea60` `HudUiTriplet::IsLocalPlayerFirstEntry` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17076`

## Battlesport/HudUiZrdWidget.cpp

- `0x4b7020` `HudUiCheckToggleWidget::~HudUiCheckToggleWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9137`
- `0x4b7340` `HudUiCheckToggleWidget::LoadFromZrd` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9296`
- `0x4b7de0` `HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9473`

## Battlesport/map.cpp

- `0x415d30` `HudSensorMapNode::DrawOnTracker` -> `src/Battlesport/HudSensorTracker.cpp:1251`
- `0x415f40` `HudSensorTracker::DrawDiamondMarker` -> `src/Battlesport/HudSensorTracker.cpp:1147`
- `0x416480` `HudSensorMapNode::DrawProjectedPath` -> `src/Battlesport/HudSensorTracker.cpp:1178`
- `0x416790` `HudSensorTracker::MapShutdownAndResetThunk` -> `src/Battlesport/HudSensorTracker.cpp:1676`
- `0x4167a0` `HudSensorTracker::MapShutdownAndReset` -> `src/Battlesport/HudSensorTracker.cpp:1682`
- `0x4167e0` `HudSensorTracker::MapRemoveNode` -> `src/Battlesport/HudSensorTracker.cpp:1511`
- `0x416840` `HudSensorTracker::MapInsertNodeAndGrowBounds` -> `src/Battlesport/HudSensorTracker.cpp:1545`
- `0x4168d0` `HudSensorTracker::LoadMapFromStream` -> `src/Battlesport/HudSensorTracker.cpp:1576`
- `0x4169d0` `HudSensorTracker::LoadMapFromPath` -> `src/Battlesport/HudSensorTracker.cpp:1634`
- `0x416a30` `HudSensorTracker::MapOverlayBeginShow` -> `src/Battlesport/HudSensorTracker.cpp:978`
- `0x416ad0` `HudSensorTracker::MapOverlayEndShow` -> `src/Battlesport/HudSensorTracker.cpp:958`
- `0x416b30` `HudSensorTracker::MapOverlayRefToggle` -> `src/Battlesport/HudSensorTracker.cpp:1003`
- `0x416b80` `HudSensorTracker::MapZoomIn` -> `src/Battlesport/HudSensorTracker.cpp:1023`
- `0x416bb0` `HudSensorTracker::MapZoomOut` -> `src/Battlesport/HudSensorTracker.cpp:1031`
- `0x416be0` `HudSensorTracker::UpdateMapScaleLerp` -> `src/Battlesport/HudSensorTracker.cpp:1039`
- `0x416c90` `HudSensorTracker::ProjectWorldPointsToOverlay` -> `src/Battlesport/HudSensorTracker.cpp:1058`
- `0x416d50` `HudSensorTracker::DrawTrackedSaveStateMarker` -> `src/Battlesport/HudSensorTracker.cpp:1343`
- `0x416dd0` `HudSensorTracker::DrawMarkerCross` -> `src/Battlesport/HudSensorTracker.cpp:1111`
- `0x416e50` `HudSensorTracker::GetSaveStateRelativeVectorLen` -> `src/Battlesport/HudSensorTracker.cpp:1086`
- `0x416ef0` `HudSensorTracker::SetSaveStateMarkerMaxDistance` -> `src/Battlesport/HudSensorTracker.cpp:929`
- `0x416f10` `HudSensorTracker::DrawSaveStateMarker` -> `src/Battlesport/HudSensorTracker.cpp:1376`
- `0x417130` `HudSensorTracker::Update` -> `src/Battlesport/HudSensorTracker.cpp:1466`
- `0x417220` `HudSensorTracker::SetTrackedSaveState` -> `src/Battlesport/HudSensorTracker.cpp:938`
- `0x417260` `HudSensorTracker::LoadMissionMapAndSfx` -> `src/Battlesport/HudSensorTracker.cpp:1657`
- `0x417300` `HudSensorTracker::SetObjectiveMarkerColorBlink` -> `src/Battlesport/HudSensorTracker.cpp:2752`
- `0x417360` `HudSensorTracker::ConstructGlobal` -> `src/Battlesport/HudSensorTracker.cpp:884`
- `0x417370` `HudSensorTracker::RegisterGlobalOnExit` -> `src/Battlesport/HudSensorTracker.cpp:890`
- `0x417380` `HudSensorTracker::ShutdownGlobal` -> `src/Battlesport/HudSensorTracker.cpp:896`
- `0x417390` `HudSensorTracker::Constructor` -> `src/Battlesport/HudSensorTracker.cpp:868`
- `0x4174f0` `HudSensorTracker::ApplyMissionDataAndReload` -> `src/Battlesport/HudSensorTracker.cpp:1779`
- `0x417690` `HudSensorTracker::ZarMission_RestoreCallback` -> `src/Battlesport/HudSensorTracker.cpp:3333`
- `0x4176d0` `HudSensorTracker::ZarMissionLate_RestoreCallback` -> `src/Battlesport/HudSensorTracker.cpp:3365`
- `0x4176f0` `HudSensorTracker::ResetMissionState` -> `src/Battlesport/HudSensorTracker.cpp:1706`
- `0x417810` `HudSensorTracker::LoadMissionCoreResources` -> `src/Battlesport/HudSensorTracker.cpp:1918`
- `0x417a00` `HudSensorTracker::InitMissionGameplaySystems` -> `src/Battlesport/HudSensorTracker.cpp:1994`
- `0x417ca0` `HudSensorTracker::OnObjectiveCommand` -> `src/Battlesport/HudSensorTracker.cpp:2903`
- `0x417d40` `HudSensorTracker::ShutdownMissionGameplaySystems` -> `src/Battlesport/HudSensorTracker.cpp:2114`
- `0x418730` `HudSensorTracker::Command_ToggleObjectivePanel` -> `src/Battlesport/HudSensorTracker.cpp:2993`
- `0x4188f0` `HudSensorTracker::Command_ShowObjectivePickupInfo` -> `src/Battlesport/HudSensorTracker.cpp:3084`
- `0x418940` `HudSensorTracker::ShowObjectivePickupInfo` -> `src/Battlesport/HudSensorTracker.cpp:3099`
- `0x418c70` `HudSensorTracker::ResetHudForMissionStart` -> `src/Battlesport/HudSensorTracker.cpp:2948`
- `0x418d40` `HudSensorTracker::UpdateObjectiveFlow` -> `src/Battlesport/HudSensorTracker.cpp:3213`
- `0x418fb0` `HudSensorTracker::SaveAndQueueMissionState` -> `src/Battlesport/HudSensorTracker.cpp:2699`
- `0x419050` `HudSensorTracker::LoadMissionWeatherFx` -> `src/Battlesport/HudSensorTracker.cpp:2199`
- `0x4193c0` `HudSensorTracker::LoadRaceCheckpointMeta` -> `src/Battlesport/HudSensorTracker.cpp:2598`
- `0x419490` `HudSensorTracker::Shutdown` -> `src/Battlesport/HudSensorTracker.cpp:1697`

## Battlesport/MfcThreeFloatDialog.cpp

- `0x406890` `MfcThreeFloatDialog::OnKillFocusValue0` -> `src/Battlesport/MfcThreeFloatDialog.cpp:107`
- `0x4068c0` `MfcThreeFloatDialog::OnKillFocusValue1` -> `src/Battlesport/MfcThreeFloatDialog.cpp:116`
- `0x4068f0` `MfcThreeFloatDialog::OnKillFocusValue2` -> `src/Battlesport/MfcThreeFloatDialog.cpp:125`
- `0x406920` `MfcThreeFloatDialog::OnDeltaposSpinValue0` -> `src/Battlesport/MfcThreeFloatDialog.cpp:134`
- `0x406960` `MfcThreeFloatDialog::OnDeltaposSpinValue1` -> `src/Battlesport/MfcThreeFloatDialog.cpp:148`
- `0x4069a0` `MfcThreeFloatDialog::OnDeltaposSpinValue2` -> `src/Battlesport/MfcThreeFloatDialog.cpp:162`
- `0x4069e0` `MfcThreeFloatDialog::OnDeltaposSpin2` -> `src/Battlesport/MfcThreeFloatDialog.cpp:176`
- `0x4069f0` `MfcThreeFloatDialog::OnMove` -> `src/Battlesport/MfcThreeFloatDialog.cpp:185`

## Battlesport/mission.cpp

- `0x417f90` `HudSensorTracker::LoadObjectivesFromPath` -> `src/Battlesport/HudSensorTracker.cpp:2342`
- `0x418230` `HudSensorTracker::LoadObjectivesFromZrd` -> `src/Battlesport/HudSensorTracker.cpp:2494`
- `0x4192d0` `HudSensorTracker::RunStartAnimsFromZrd` -> `src/Battlesport/HudSensorTracker.cpp:2630`

## Battlesport/net.cpp

- `0x4339d0` `GameNet::GetNearestOtherPlayerDistanceToSpawnPoint` -> `src/Battlesport/GameNet.cpp:1699`

## Battlesport/Net.cpp

- `0x43cf40` `Net::FormatIpv4Address` -> `src/Battlesport/GameNet.cpp:782`

## Battlesport/OptCatalog.cpp

- `0x4aefb0` `OptCatalog::DeactivateTrailRuntimeState` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2101`
- `0x4b0640` `OptCatalog::PlayNoAmmoWarning` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2306`

## Battlesport/pickup.cpp

- `0x41cc10` `PickupSpawnList::Primary_Init` -> `src/Battlesport/pickup.cpp:413`
- `0x41cc40` `PickupSpawnList::NetCopy_Init` -> `src/Battlesport/pickup.cpp:421`
- `0x41cc70` `PickupRespawnQueue::Init` -> `src/Battlesport/pickup.cpp:429`
- `0x41ccd0` `Pickup::Shutdown` -> `src/Battlesport/pickup.cpp:2292`
- `0x41ccf0` `Pickup::Init` -> `src/Battlesport/pickup.cpp:836`
- `0x41ceb0` `zClass_Node::ClearPickupFlagsRecursive` -> `src/Battlesport/pickup.cpp:273`
- `0x41cef0` `zClass_Node::SetPickupFlagsRecursive` -> `src/Battlesport/pickup.cpp:286`
- `0x41cf50` `Pickup::RemoveObject` -> `src/Battlesport/pickup.cpp:1560`
- `0x41d0c0` `Pickup::OnCollected` -> `src/Battlesport/pickup.cpp:1642`
- `0x41d220` `Pickup::ApplyEffect` -> `src/Battlesport/pickup.cpp:1998`
- `0x41d650` `Pickup::GrantAmmoOrWeapon` -> `src/Battlesport/pickup.cpp:1884`
- `0x41d8a0` `PickupSpawnList::RemoveAndFreeNode` -> `src/Battlesport/pickup.cpp:437`
- `0x41d920` `Pickup::CreateSpawnDefAndLink` -> `src/Battlesport/pickup.cpp:1184`
- `0x41da20` `Pickup::SpawnAt` -> `src/Battlesport/pickup.cpp:1340`
- `0x41dab0` `Pickup::CreateObjectInstance` -> `src/Battlesport/pickup.cpp:1137`
- `0x41db40` `PickupType::GetByIndex_Pure` -> `src/Battlesport/pickup.cpp:171`
- `0x41db60` `Pickup::AssignBvolGroupAndId` -> `src/Battlesport/pickup.cpp:1088`
- `0x41dc30` `Pickup::SpawnFromParsedZrdEntry` -> `src/Battlesport/pickup.cpp:1429`
- `0x41dc60` `Pickup::SpawnWithAirdropChute` -> `src/Battlesport/pickup.cpp:1286`
- `0x41dcf0` `Pickup::RegisterExistingObject` -> `src/Battlesport/pickup.cpp:1247`
- `0x41dd60` `PickupType::FindByLogicalName` -> `src/Battlesport/pickup.cpp:193`
- `0x41ddf0` `Pickup::SelectPuppiesZrdByDifficulty` -> `src/Battlesport/pickup.cpp:590`
- `0x41de30` `Net::IsOptEntryActiveInAnySlot` -> `src/Battlesport/pickup.cpp:250`
- `0x41de70` `Pickup::InitAndLoadPuppySpawns` -> `src/Battlesport/pickup.cpp:613`
- `0x41e1a0` `PickupTypeMeta::FindByName` -> `src/Battlesport/pickup.cpp:235`
- `0x41e1c0` `PickupType::GetByIndex` -> `src/Battlesport/pickup.cpp:182`
- `0x41e1e0` `PickupTypeKeyTable::FindIndex` -> `src/Battlesport/pickup.cpp:215`
- `0x41e240` `PickupSpawnList::Clear` -> `src/Battlesport/pickup.cpp:483`
- `0x41e270` `PickupRespawnQueue::ClearAndFree` -> `src/Battlesport/pickup.cpp:500`
- `0x41e2f0` `Pickup::RemoveOtherSpawnsWithSameOptEntry` -> `src/Battlesport/pickup.cpp:1865`
- `0x41e330` `Pickup::SetVariantFromTerrain` -> `src/Battlesport/pickup.cpp:1447`
- `0x41e430` `Pickup::SpawnListHasEntryNearXZ` -> `src/Battlesport/pickup.cpp:760`
- `0x41e480` `Pickup::SelectNextVTOLSpawnTypeIndex` -> `src/Battlesport/pickup.cpp:791`
- `0x41e540` `Pickup::MapVTOLDropGroupVariantToTypeIndex` -> `src/Battlesport/pickup.cpp:778`
- `0x41e5d0` `PickupRespawnQueue::Update` -> `src/Battlesport/pickup.cpp:540`
- `0x41e6c0` `Pickup::RespawnSpawnDef` -> `src/Battlesport/pickup.cpp:1499`
- `0x41e780` `Pickup::ArchiveWriteAll` -> `src/Battlesport/pickup.cpp:2205`
- `0x41e840` `Pickup::ArchiveReadRecord` -> `src/Battlesport/pickup.cpp:2244`
- `0x41e890` `Pickup::ReconcilePrimaryAndNetworkCopySpawnLists` -> `src/Battlesport/pickup.cpp:1732`
- `0x41e900` `Pickup::SpawnListContainsPickupId` -> `src/Battlesport/pickup.cpp:1760`
- `0x41e930` `Pickup::FindSpawnByPickupId` -> `src/Battlesport/pickup.cpp:1804`
- `0x41e950` `Pickup::GetSpawnDefFromNode` -> `src/Battlesport/pickup.cpp:1821`
- `0x41e960` `Pickup::SetNextPickupId` -> `src/Battlesport/pickup.cpp:2278`
- `0x41e970` `Pickup::GetNextPickupId` -> `src/Battlesport/pickup.cpp:2287`
- `0x41e980` `Pickup::FindDroppableTypeForPlayerCurrentWeapon` -> `src/Battlesport/pickup.cpp:1844`
- `0x41ea00` `Pickup::FindOptMetaImageByOptEntry` -> `src/Battlesport/pickup.cpp:1828`
- `0x41ea30` `Pickup::SpawnAtCarrierNodeByName` -> `src/Battlesport/pickup.cpp:1392`
- `0x433e40` `Pickup::SendPkt11_Flag2Delta` -> `src/Battlesport/pickup.cpp:943`
- `0x433e70` `Pickup::SendPkt11_Flag8Delta` -> `src/Battlesport/pickup.cpp:954`
- `0x433ea0` `Pickup::SendPkt11_CreateDelta` -> `src/Battlesport/pickup.cpp:965`
- `0x433f40` `Pickup::HandlePkt11_SpawnDelta` -> `src/Battlesport/pickup.cpp:994`
- `0x434050` `Pickup::SendPkt12_AirdropSpawnChuteRelay` -> `src/Battlesport/pickup.cpp:1060`
- `0x4340a0` `Pickup::HandlePkt12_AirdropSpawnChuteRelay` -> `src/Battlesport/pickup.cpp:1074`
- `0x438990` `PickupAirdropSpawnRef::InitNodesFromCarrierNodeName` -> `src/Battlesport/pickup.cpp:300`
- `0x4389c0` `PickupAirdropSpawnRef::SpawnPickupTypeAndRelay` -> `src/Battlesport/pickup.cpp:347`
- `0x438a20` `PickupAirdropSpawnRef::CanSpawnWithClearance` -> `src/Battlesport/pickup.cpp:326`
- `0x438a70` `PickupAirdropSpawnRef::GetWorldPos` -> `src/Battlesport/pickup.cpp:316`
- `0x438a90` `PickupAirdropSpawnRef::InitGlobalFromCarrierNodeName` -> `src/Battlesport/pickup.cpp:375`
- `0x438b30` `PickupAirdropSpawnRef::TrySpawnRandomPickupFromGlobal` -> `src/Battlesport/pickup.cpp:400`

## Battlesport/player.cpp

- `0x403750` `Player::BuildAiPeerRingsByAiNetId` -> `src/Battlesport/player.cpp:3227`
- `0x405650` `Player::UpdateThirdPersonCamera` -> `src/Battlesport/player.cpp:13547`
- `0x405c90` `Player::ApplyCameraState` -> `src/Battlesport/player.cpp:13610`
- `0x41bbf0` `Player::DestroyedStateResetCallback` -> `src/Battlesport/player.cpp:7939`
- `0x41bca0` `Player::DestroyedStateResetFinalizeCallback` -> `src/Battlesport/player.cpp:7912`
- `0x41bd20` `Player::DestroyedStateResetLocalFinalize` -> `src/Battlesport/player.cpp:7892`
- `0x41ea90` `Player::InitMasterCommonDataList` -> `src/Battlesport/player.cpp:2079`
- `0x41eac0` `Player::InitMasterModalDataList` -> `src/Battlesport/player.cpp:2088`
- `0x41ec80` `Player::InitAndRegisterTopMsgPanel2` -> `src/Battlesport/player.cpp:2167`
- `0x41ec90` `Player_TopMsgPanel2::Constructor` -> `src/Battlesport/player.cpp:2025`
- `0x41ecb0` `Player::RegisterTopMsgPanel2Cleanup` -> `src/Battlesport/player.cpp:2174`
- `0x41ecc0` `Player_TopMsgPanel2::Destructor` -> `src/Battlesport/player.cpp:2035`
- `0x41ecd0` `Player::RecordNodeFlagsForRestore` -> `src/Battlesport/player.cpp:10865`
- `0x41ef30` `PlayerNodeFlagRestore::InitGlobals` -> `src/Battlesport/player.cpp:2045`
- `0x41ef40` `PlayerNodeFlagRestore::InitInstance` -> `src/Battlesport/player.cpp:2052`
- `0x41ef60` `PlayerNodeFlagRestore::RegisterAtExit` -> `src/Battlesport/player.cpp:2061`
- `0x41ef70` `PlayerNodeFlagRestore::ShutdownInstance` -> `src/Battlesport/player.cpp:2067`
- `0x41efa0` `Player::RestoreRecordedNodeFlags` -> `src/Battlesport/player.cpp:10913`
- `0x41f010` `Player::BuildMissionSaveData` -> `src/Battlesport/player.cpp:10635`
- `0x41f1d0` `Player::ApplyMissionSaveData` -> `src/Battlesport/player.cpp:10707`
- `0x41f5b0` `Player::ZAR_RegisterSections` -> `src/Battlesport/player.cpp:10967`
- `0x41f5f0` `Player::ZAR_WriteMissionSaveDataSection` -> `src/Battlesport/player.cpp:10987`
- `0x41f640` `Player::ZAR_ReadMissionSaveDataSection` -> `src/Battlesport/player.cpp:10941`
- `0x41f6a0` `Player::ZAR_WriteVehicleListSection` -> `src/Battlesport/player.cpp:11156`
- `0x41f850` `Player::ZAR_ReadVehicleListSection` -> `src/Battlesport/player.cpp:11006`
- `0x41fb80` `Player::ShutdownMissionRuntime` -> `src/Battlesport/player.cpp:16641`
- `0x41fd20` `Player::DestroySaveGameState` -> `src/Battlesport/player.cpp:16586`
- `0x41fe40` `Player::GetAivZrdPath` -> `src/Battlesport/player.cpp:2180`
- `0x41fe50` `zVehicle::SelectZrdByDifficulty` -> `src/Battlesport/player.cpp:1978`
- `0x41fe90` `Player::InitMissionRuntimeFromWorldAndCamera` -> `src/Battlesport/player.cpp:2461`
- `0x420d10` `Player::InitStateFromNameAndMasterCommonData` -> `src/Battlesport/player.cpp:2931`
- `0x421470` `Player::BindModalStateFromMasterModalData` -> `src/Battlesport/player.cpp:3248`
- `0x421790` `Player::InitSpawnStateFromPrimaryModalData` -> `src/Battlesport/player.cpp:3378`
- `0x421830` `Player::SampleGroundAndAlignRootToSurface` -> `src/Battlesport/player.cpp:3410`
- `0x421a40` `Player::CloneType6NodeFromTemplateAndRename` -> `src/Battlesport/player.cpp:2212`
- `0x421ab0` `Player::CreateFromNamesAtPose` -> `src/Battlesport/player.cpp:2268`
- `0x421ed0` `Player::BuildCollisionPointsFromModel` -> `src/Battlesport/player.cpp:3509`
- `0x4220f0` `Player::BuildSupportPointsFromModel` -> `src/Battlesport/player.cpp:3562`
- `0x422170` `Player::LoadMasterCommonDataFromNode` -> `src/Battlesport/player.cpp:3602`
- `0x4226d0` `Player::LoadMasterModalDataFromNode` -> `src/Battlesport/player.cpp:3891`
- `0x423150` `Player::ExtractVehicleNameFromAivName` -> `src/Battlesport/player.cpp:2186`
- `0x4231b0` `Player::RefreshHudFromState` -> `src/Battlesport/player.cpp:11439`
- `0x423380` `Player::IsMissionProbeType1EnabledById` -> `src/Battlesport/player.cpp:8606`
- `0x425920` `Player::RegisterGameplayCommandCallbacksAndCreateFfEffects` -> `src/Battlesport/player.cpp:9005`
- `0x4266b0` `Player::TickMasterTypeAndForceFeedback` -> `src/Battlesport/player.cpp:12477`
- `0x427140` `Player::UpdateMasterTypeHover` -> `src/Battlesport/player.cpp:15462`
- `0x427440` `Player::UpdateMasterTypeHover_FromModalProbe` -> `src/Battlesport/player.cpp:15042`
- `0x4279f0` `Player::UpdateMasterTypeAmphib` -> `src/Battlesport/player.cpp:15312`
- `0x427ec0` `Player::UpdateMasterTypeAmphib_FromModalProbe` -> `src/Battlesport/player.cpp:15216`
- `0x428120` `Player::UpdateMasterTypeBasic` -> `src/Battlesport/player.cpp:15559`
- `0x428350` `Player::UpdateMasterTypeBasicOrTrack_FromModalProbe` -> `src/Battlesport/player.cpp:15000`
- `0x428520` `Player::UpdateMasterTypeSub` -> `src/Battlesport/player.cpp:12344`
- `0x4289f0` `Player::UpdateSubModeWaterProbeState` -> `src/Battlesport/player.cpp:12094`
- `0x428d60` `Player::ProbeModalSampleHeights` -> `src/Battlesport/player.cpp:13810`
- `0x4290f0` `Player::SelectProbeSampleHeightFromCandidates` -> `src/Battlesport/player.cpp:13744`
- `0x429240` `Player::ApplyAmphibSpeedOscillation` -> `src/Battlesport/player.cpp:10441`
- `0x429750` `Player::UpdateAutoTurnAndSteerFromTarget` -> `src/Battlesport/player.cpp:10327`
- `0x42a9f0` `Player::AddScaledHudCounterValue` -> `src/Battlesport/player.cpp:5400`
- `0x42aa40` `Player::GetSaveStateListHead` -> `src/Battlesport/player.cpp:2834`
- `0x42aa50` `Player::UpdateDebugOverlayHud` -> `src/Battlesport/player.cpp:11330`
- `0x42ac90` `Player::TransitionToMasterTypeTrack` -> `src/Battlesport/player.cpp:4499`
- `0x42aeb0` `Player::TransitionToMasterTypeAmphib` -> `src/Battlesport/player.cpp:4624`
- `0x42b0f0` `Player::TransitionToMasterTypeHover` -> `src/Battlesport/player.cpp:4860`
- `0x42b2a0` `Player::TransitionToMasterTypeSub` -> `src/Battlesport/player.cpp:4744`
- `0x42b4a0` `Player::StopBftBubbleFxHandle` -> `src/Battlesport/player.cpp:4461`
- `0x42b4c0` `Player::TransitionToMasterTypeFly` -> `src/Battlesport/player.cpp:4474`
- `0x42b520` `Player::ApplyMasterTypeTransition` -> `src/Battlesport/player.cpp:4952`
- `0x42b5a0` `Player::ReactivateCopterSndNodesIfHealthy` -> `src/Battlesport/player.cpp:4407`
- `0x42b630` `Player::CacheDisableCopterSndNodesAndStopSample` -> `src/Battlesport/player.cpp:4354`
- `0x42b810` `Player::SyncLocalPoseFromRootNode` -> `src/Battlesport/player.cpp:2857`
- `0x42b8c0` `Player::RebuildSteerBasisRawFromRef` -> `src/Battlesport/player.cpp:10421`
- `0x42bab0` `Player::SetAutoTurnTargetDirFromWorldPoint` -> `src/Battlesport/player.cpp:10612`
- `0x42bb30` `Player::AsyncCommandCallback` -> `src/Battlesport/player.cpp:9064`
- `0x42be00` `Player::SetWorldPoseAndRestartAnchor` -> `src/Battlesport/player.cpp:9247`
- `0x42be70` `Player::CaptureCurrentObjectPoseAsRestartAnchor` -> `src/Battlesport/player.cpp:9271`
- `0x42bed0` `Player::ResetMotionTransientState` -> `src/Battlesport/player.cpp:10278`
- `0x42bf90` `Player::UpdatePostMoveEnvironment` -> `src/Battlesport/player.cpp:14659`
- `0x42c0d0` `Player::ProcessEnvProbeResults` -> `src/Battlesport/player.cpp:14729`
- `0x42c2e0` `Player::UpdateVerticalVelocityAndTransform` -> `src/Battlesport/player.cpp:14954`
- `0x42c420` `Player::AccumulateSlopeForces` -> `src/Battlesport/player.cpp:14928`
- `0x42c520` `Player::ComputeSurfaceFrom1Probe` -> `src/Battlesport/player.cpp:14404`
- `0x42c640` `Player::ComputeSurfaceFrom2Probes` -> `src/Battlesport/player.cpp:14443`
- `0x42c8d0` `Player::ApplyTerrainTilt` -> `src/Battlesport/player.cpp:14323`
- `0x42ca40` `Player::ComputeSurfaceFrom3Probes` -> `src/Battlesport/player.cpp:14621`
- `0x42cb50` `Player::ResetTerrainContactImpulsesAndPlayImpactSfx` -> `src/Battlesport/player.cpp:14298`
- `0x42cbd0` `Player::CheckProbeSampleMaskOverlap` -> `src/Battlesport/player.cpp:14518`
- `0x42cc00` `Player::SelectBestProbesByDotProduct` -> `src/Battlesport/player.cpp:14542`
- `0x42cde0` `Player::SolveHeightOnSurface` -> `src/Battlesport/player.cpp:14281`
- `0x42ce50` `Player::ComputeTriangleNormal` -> `src/Battlesport/player.cpp:14371`
- `0x42cf60` `Player::RebuildAboveGroundIndices` -> `src/Battlesport/player.cpp:14530`
- `0x42cf90` `Player::BuildEnvironmentProbeResult` -> `src/Battlesport/player.cpp:13962`
- `0x42d320` `Player::FindThirdProbeAndComputeNormal` -> `src/Battlesport/player.cpp:14858`
- `0x42d5c0` `Player::ApplyEnvironmentProbeResult` -> `src/Battlesport/player.cpp:14097`
- `0x42da40` `Player::RebuildOrientationFromNormal` -> `src/Battlesport/player.cpp:14822`
- `0x438540` `Player::SelectModalStateByMasterType` -> `src/Battlesport/player.cpp:1791`
- `0x4385a0` `Player::StartMasterTypeLoopSfxHandle` -> `src/Battlesport/player.cpp:1725`
- `0x438630` `Player::EnsureMasterTypeLoopSfxHandle` -> `src/Battlesport/player.cpp:1744`
- `0x438660` `Player::StopMasterTypeLoopSfxHandle` -> `src/Battlesport/player.cpp:1816`
- `0x438690` `Player::StopModalLoopSfxHandle` -> `src/Battlesport/player.cpp:1778`
- `0x4386c0` `Player::UpdateModalLoopSfx` -> `src/Battlesport/player.cpp:1829`
- `0x438b60` `Player::FreeAltWeaponTrailRuntimeStates` -> `src/Battlesport/player.cpp:11638`
- `0x438ba0` `Player::LoadWeaponBanksAndSelectDefaults` -> `src/Battlesport/player.cpp:11656`
- `0x4390d0` `Player::CacheGunHardpointsAndDetachDisplays` -> `src/Battlesport/player.cpp:2888`
- `0x439460` `Player::HandlePrimaryWeaponVariantToggleInput` -> `src/Battlesport/player.cpp:13240`
- `0x439540` `Player::ApplyAltWeaponSwitch` -> `src/Battlesport/player.cpp:12948`
- `0x439600` `Player::ApplyPrimaryWeaponSwitch` -> `src/Battlesport/player.cpp:12907`
- `0x439ba0` `Player::TickAltGunRuntimeState` -> `src/Battlesport/player.cpp:16474`
- `0x43a400` `Player::ProcessPrimaryGunDispatchTick` -> `src/Battlesport/player.cpp:16413`
- `0x43a900` `Player::DecayAndApplyAltFireSlotOffsetToNode` -> `src/Battlesport/player.cpp:15951`
- `0x43a980` `Player::ApplyGunFireSlotOffsetToNode` -> `src/Battlesport/player.cpp:15976`
- `0x43aa30` `Player::SelectAltGunFirePointAndSlot` -> `src/Battlesport/player.cpp:16015`
- `0x43acf0` `Player::SelectPrimaryGunFirePointAndSlot` -> `src/Battlesport/player.cpp:16082`
- `0x43afd0` `Player::ComposeAimBasisWorldMatrix` -> `src/Battlesport/player.cpp:15899`
- `0x43bc40` `Player::EnterLocalInactiveDestroyedLifecycle` -> `src/Battlesport/player.cpp:8055`
- `0x43c0c0` `Player::StartDestroyedStateVehicleEffect` -> `src/Battlesport/player.cpp:8614`
- `0x43c330` `Player::EnsureGunAuxEffectActive` -> `src/Battlesport/player.cpp:16186`
- `0x43c630` `Player::IsAltWeaponAllowedInCurrentMasterMode` -> `src/Battlesport/player.cpp:12829`
- `0x43c660` `Player::AutoSwitchToNextUsableAltWeapon` -> `src/Battlesport/player.cpp:12859`
- `0x43c800` `Player::ResetAltGunDoorAnimationState` -> `src/Battlesport/player.cpp:12990`
- `0x43c850` `Player::ResetAltGunRuntimeState` -> `src/Battlesport/player.cpp:13046`
- `0x43c950` `Player::RemoveAllDeployedMines` -> `src/Battlesport/player.cpp:13086`

## Battlesport/Recoil.cpp

- `0x401000` `CAboutDlg::Constructor` -> `src/Battlesport/Recoil.cpp:4`

## Battlesport/recoil_state.cpp

- `0x408d20` `RecoilStateControls::StaticInitAndRegisterAtExit` -> `src/Battlesport/hud.cpp:1413`
- `0x408d30` `RecoilStateControls::StaticInit` -> `src/Battlesport/hud.cpp:1420`
- `0x408d40` `RecoilStateControls::RegisterAtExit` -> `src/Battlesport/hud.cpp:1426`
- `0x408d50` `RecoilStateControls::AtExitDestructor` -> `src/Battlesport/hud.cpp:1432`
- `0x408d60` `RecoilStateControls::RecoilStateControls` -> `src/Battlesport/hud.cpp:1439`
- `0x408d90` `RecoilStateControls::Destructor` -> `src/Battlesport/hud.cpp:1447`
- `0x408df0` `RecoilStateControls::OnTryBecomeCurrent` -> `src/Battlesport/hud.cpp:1458`
- `0x408ec0` `RecoilStateControls::OnDeactivate` -> `src/Battlesport/hud.cpp:1484`
- `0x408fa0` `RecoilStateControls::OnResume` -> `src/Battlesport/hud.cpp:1517`
- `0x408ff0` `RecoilStateControls::QueueEnter` -> `src/Battlesport/hud.cpp:1546`

## Battlesport/RecoilApp.cpp

- `0x42eed0` `RecoilApp_PlayState::OnTryBecomeCurrent` -> `src/Battlesport/RecoilApp.cpp:2628`
- `0x42f5e0` `RecoilApp_PlayState::OnUpdateShouldQuit` -> `src/Battlesport/RecoilApp.cpp:2779`
- `0x42f8e0` `RecoilApp_PlayState::OnDeactivate` -> `src/Battlesport/RecoilApp.cpp:2888`
- `0x430c90` `RecoilApp::FatalErrorAndExit` -> `src/Battlesport/RecoilApp.cpp:1419`
- `0x434660` `operator<` -> `src/Battlesport/RecoilApp.cpp:230`
- `0x435d20` `RecoilStateSaveLoadTransition::OnTryBecomeCurrent` -> `src/Battlesport/RecoilApp.cpp:1242`
- `0x435e80` `RecoilStateSaveLoadTransition::OnUpdateShouldQuit` -> `src/Battlesport/RecoilApp.cpp:1298`
- `0x435ed0` `RecoilStateSaveLoadTransition::OnDeactivate` -> `src/Battlesport/RecoilApp.cpp:1326`
- `0x435f50` `RecoilStateSaveLoadTransition::QueueOpenSaveDialog` -> `src/Battlesport/RecoilApp.cpp:1371`
- `0x435f80` `RecoilStateSaveLoadTransition::QueueOpenLoadDialog` -> `src/Battlesport/RecoilApp.cpp:1391`
- `0x4428b0` `RecoilApp_MfcOleModule::~RecoilApp_MfcOleModule` -> `src/Battlesport/RecoilApp.cpp:2316`
- `0x442c70` `RecoilApp_MfcOleModule::RecoilApp_MfcOleModule` -> `src/Battlesport/RecoilApp.cpp:2269`
- `0x442c70` `RecoilApp_MfcOleModule::RecoilApp_MfcOleModule` -> `src/Battlesport/RecoilApp.cpp:2291`
- `0x442d00` `RecoilApp::Run` -> `src/Battlesport/RecoilApp.cpp:2514`

## Battlesport/RecoilStateCheatCode.cpp

- `0x406f60` `RecoilStateCheatCode::OnTryBecomeCurrent` -> `src/Battlesport/hud.cpp:1274`
- `0x407010` `RecoilStateCheatCode::OnDeactivate` -> `src/Battlesport/hud.cpp:1300`

## Battlesport/RecoilStateConfirmQuit.cpp

- `0x415880` `RecoilStateConfirmQuit::~RecoilStateConfirmQuit` -> `src/Battlesport/hud.cpp:1397`

## Battlesport/turret.cpp

- `0x436630` `zTurret_Runtime::InitDefaults` -> `src/GameZRecoil/zTurret/zTurret.cpp:244`
- `0x4367a0` `zTurret_Runtime::InitFromReaderNode` -> `src/GameZRecoil/zTurret/zTurret.cpp:322`
- `0x436e20` `zTurret_Runtime::HasActiveNode` -> `src/GameZRecoil/zTurret/zTurret.cpp:1305`
- `0x436e40` `zTurret_Runtime::Tick` -> `src/GameZRecoil/zTurret/zTurret.cpp:1061`
- `0x437430` `zTurret_Runtime::UpdateFirePositionFromParts` -> `src/GameZRecoil/zTurret/zTurret.cpp:815`
- `0x4374a0` `zTurret_Runtime::UpdateAimAndPartMatrices` -> `src/GameZRecoil/zTurret/zTurret.cpp:838`
- `0x437730` `zTurret_Runtime::SelectFirePointAndAimAtTarget` -> `src/GameZRecoil/zTurret/zTurret.cpp:931`
- `0x437820` `zTurret_Runtime::FireWeapon` -> `src/GameZRecoil/zTurret/zTurret.cpp:965`
- `0x437990` `zTurret_Runtime::UpdateFireBurstTimer` -> `src/GameZRecoil/zTurret/zTurret.cpp:1044`
- `0x4379f0` `zTurret_Runtime::ApplyDamageAndHandleDestruction` -> `src/GameZRecoil/zTurret/zTurret.cpp:1229`
- `0x437ac0` `zTurret_System::LoadDefinitionsFromPath` -> `src/GameZRecoil/zTurret/zTurret.cpp:1323`
- `0x437ca0` `zTurret_System::TickAllRuntimesRoundRobin` -> `src/GameZRecoil/zTurret/zTurret.cpp:1423`
- `0x437d40` `zTurret_System::DisableTickCallback` -> `src/GameZRecoil/zTurret/zTurret.cpp:1462`
- `0x437d50` `zTurret_System::EnableTickCallback` -> `src/GameZRecoil/zTurret/zTurret.cpp:1471`
- `0x437d60` `zTurret_Runtime::OnDamage` -> `src/GameZRecoil/zTurret/zTurret.cpp:1271`
- `0x437e50` `zTurret_Runtime::FireWeaponCallback` -> `src/GameZRecoil/zTurret/zTurret.cpp:1217`

## Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp

- `0x441750` `WestwoodOnlineUpgradeConfigDialog::Constructor` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:133`
- `0x441c60` `WestwoodOnlineUpgradeConfigDialog::GetSelectedProfileValues` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:413`
- `0x441cb0` `WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:428`

## Battlesport/WestwoodOnlineUpgradeDialog.cpp

- `0x43cf90` `WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:990`
- `0x43d060` `WestwoodOnlineUpgradeDialog::AppendStatusTextFmt` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:740`
- `0x43d650` `WestwoodOnlineUpgradeDialog::AppendConnectStatusAndRefreshList` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1798`
- `0x43d6a0` `WestwoodOnlineUpgradeDialog::SetAbortAndClose` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1814`
- `0x43d6b0` `WestwoodOnlineUpgradeDialog::EnableQueryControls` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1745`
- `0x43d720` `WestwoodOnlineUpgradeDialog::EnableConnectButton` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1759`
- `0x43d740` `WestwoodOnlineUpgradeDialog::Constructor` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:543`
- `0x43d980` `WestwoodOnlineUpgradeDialog::ScalarDeletingDestructor` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:610`
- `0x43d9a0` `WestwoodOnlineUpgradeDialog::Destructor` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:584`
- `0x43db20` `WestwoodOnlineUpgradeDialog::DoDataExchange` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:623`
- `0x43dcc0` `WestwoodOnlineUpgradeDialog::GetMessageMap` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:424`
- `0x43dcd0` `WestwoodOnlineUpgradeDialog::OnInitDialogBootstrap` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:430`
- `0x43dfe0` `WestwoodOnlineUpgradeDialog::OnRefreshListTimer` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:849`
- `0x43e040` `WestwoodOnlineUpgradeDialog::OnBrowseRecordListDblClk` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1493`
- `0x43e160` `WestwoodOnlineUpgradeDialog::OnDestroy` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1724`
- `0x43e1c0` `WestwoodOnlineUpgradeDialog::SubmitVisibleSessionRequestsAndStatusText` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1224`
- `0x43e3b0` `WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1767`
- `0x43e450` `WestwoodOnlineUpgradeDialog::BeginDisconnectAndShowProgress` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:871`
- `0x43e4b0` `WestwoodOnlineUpgradeDialog::BeginConnect` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:894`
- `0x43e520` `WestwoodOnlineUpgradeDialog::CheckAndApplyUpgrade` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:921`
- `0x43e550` `WestwoodOnlineUpgradeDialog::QueryStatus` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:936`
- `0x43e680` `WestwoodOnlineUpgradeDialog::RequestActiveListMode` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1018`
- `0x43e6a0` `WestwoodOnlineUpgradeDialog::OnRefreshCurrentQuery` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1027`
- `0x43e900` `WestwoodOnlineUpgradeDialog::OnQuerySessionsByName` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1115`
- `0x43ebd0` `WestwoodOnlineUpgradeDialog::ClearStatusList` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:803`
- `0x43ec00` `WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequests` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1380`
- `0x43ed10` `WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequestsAndLookupBrowseRecords` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1434`
- `0x43ee40` `WestwoodOnlineUpgradeDialog::RequestListMode0` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1540`
- `0x43ee60` `WestwoodOnlineUpgradeDialog::RequestListMode11` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1550`
- `0x43ee80` `WestwoodOnlineUpgradeDialog::OnSessionModeComboSelChange` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1560`
- `0x43ef10` `WestwoodOnlineUpgradeDialog::SubmitPendingSessionListFromResults` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1597`
- `0x43efc0` `WestwoodOnlineUpgradeDialog::OnQueryControlsChanged` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1640`
- `0x43efd0` `WestwoodOnlineUpgradeDialog::OnMaxPlayersEditChange` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1646`
- `0x43efe0` `WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1821`
- `0x43f440` `WestwoodOnlineUpgradeProgressDialog::Destructor` -> `src/Battlesport/WestwoodOnlineUpgradeProgressDialog.cpp:100`
- `0x43f450` `WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1652`
- `0x43f4d0` `WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1676`
- `0x43f550` `WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1700`
- `0x43f5d0` `WestwoodOnlineUpgrade::TruncateStringAtFirstSpace` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:528`
- `0x43f6b0` `WestwoodOnlineUpgradeDialog::OnBootstrapServerList` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:1311`
- `0x4416f0` `WestwoodOnlineUpgradeDialog::GetSelectedProfilePlayerName` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:831`
- `0x441720` `WestwoodOnlineUpgradeDialog::GetSelectedProfileConnectString` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:840`
- `0x442180` `WestwoodOnlineUpgradeDialog::SetSelectedProfilePlayerName` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:815`
- `0x4421d0` `WestwoodOnlineUpgradeDialog::SetSelectedProfileConnectString` -> `src/Battlesport/WestwoodOnlineUpgradeDialog.cpp:823`

## Battlesport/WestwoodOnlineUpgradeProgressDialog.cpp

- `0x442220` `WestwoodOnlineUpgradeProgressDialog::Constructor` -> `src/Battlesport/WestwoodOnlineUpgradeProgressDialog.cpp:88`
- `0x442320` `WestwoodOnlineUpgradeProgressDialog::DlgProc` -> `src/Battlesport/WestwoodOnlineUpgradeProgressDialog.cpp:119`
- `0x442530` `WestwoodOnlineUpgradeDialog::ShowDownloadReadyList` -> `src/Battlesport/WestwoodOnlineUpgradeProgressDialog.cpp:228`

## Battlesport/zimage.cpp

- `0x46d4d0` `zImage::FindTexDirEntryByName` -> `src/GameZRecoil/zImage/zimg_texture.cpp:321`

## Battlesport/zNetwork/zNetwork.cpp

- `0x489f70` `zNetwork_GetLocalPlayerKey` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:185`
- `0x489f80` `zNetwork::IsHost` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:302`

## Battlesport/zOpt.cpp

- `0x408230` `zOpt::SetNetworkEnabled` -> `src/GameZRecoil/zGame/zGame.cpp:1697`

## Battlesport/zopt.cpp

- `0x408270` `zOpt::GetNetworkModemEnabled` -> `src/GameZRecoil/zGame/zGame.cpp:1722`
- `0x408360` `zOpt::GetHudTypeForCurrentHwMode` -> `src/GameZRecoil/zGame/zGame.cpp:1807`

## Battlesport/zWeapon.cpp

- `0x439260` `Player::HandleAltWeaponBankSelectInput` -> `src/Battlesport/player.cpp:13131`

## GameZ/z_math.cpp

- `0x4bd720` `zMath::ClipLineSegmentToZRange` -> `src/GameZRecoil/zMath/zMath.cpp:1362`
- `0x4bd800` `zMath::ClipLineSegmentPointToZ` -> `src/GameZRecoil/zMath/zMath.cpp:1348`

## GameZRecoil/checkpoint.cpp

- `0x420c60` `Checkpoint::InstantiateNamedObjects` -> `src/Battlesport/player.cpp:5069`
- `0x425150` `Checkpoint::UpdatePlayerLapProgressAndNotifyNet` -> `src/Battlesport/player.cpp:5103`

## GameZRecoil/GameNet.cpp

- `0x434240` `OptCatalog::SendPkt0A_RemoveRuntimeRelay` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1323`
- `0x4342d0` `OptCatalog::HandlePkt0A_RemoveRuntimeRelay` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1355`
- `0x434370` `GameNet::SendPkt13_EffectAnimActivationRecord` -> `src/Battlesport/GameNet.cpp:3314`
- `0x4343f0` `GameNet::HandlePkt13_EffectAnimActivationRecord` -> `src/Battlesport/GameNet.cpp:3358`
- `0x434430` `GameNet::SendAllPkt13_EffectAnimActivationRecords` -> `src/Battlesport/GameNet.cpp:3345`

## GameZRecoil/mission.cpp

- `0x417350` `Mission::InitObjectives` -> `src/GameZRecoil/mission.cpp:7`

## GameZRecoil/Player.cpp

- `0x401580` `Player::AiAdvancePathCursorAndComputeTargetVec` -> `src/Battlesport/player.cpp:5540`
- `0x401e50` `Player::TestScenePathBetweenCameraTargetAndPoint` -> `src/Battlesport/player.cpp:13397`
- `0x401f60` `Player::AiRebuildSyntheticPathToNodeIfFar` -> `src/Battlesport/player.cpp:5877`

## GameZRecoil/player.cpp

- `0x41b950` `Player::TickRemoteNetworkPlayer` -> `src/Battlesport/player.cpp:8370`
- `0x426390` `PlayerMgr::TickAllPlayers` -> `src/Battlesport/player.cpp:9314`
- `0x4283f0` `Player::UpdateBankVelocityFromSteerInput` -> `src/Battlesport/player.cpp:10304`
- `0x428490` `Player::IntegrateYawAndWrapFromYawVelocity` -> `src/Battlesport/player.cpp:10368`
- `0x4399c0` `Player::ResetDamageVisualsAndTimedStatus` -> `src/Battlesport/player.cpp:13320`

## GameZRecoil/Player/player_camera.c

- `0x404e90` `Player::TickActiveCameraState` -> `src/Battlesport/player.cpp:9492`
- `0x405040` `Player::UpdateChaseCameraFromInput` -> `src/Battlesport/player.cpp:9567`
- `0x4057d0` `Player::UpdateTopDownCameraState` -> `src/Battlesport/player.cpp:9770`
- `0x405870` `Player::UpdateCameraFromStoredTargetTowardPlayer` -> `src/Battlesport/player.cpp:9882`
- `0x4059a0` `Player::UpdateFirstPersonCameraFromInput` -> `src/Battlesport/player.cpp:9797`
- `0x405ec0` `Player::ToggleSteeringModeAndResetMouseLook` -> `src/Battlesport/player.cpp:10271`
- `0x405ee0` `Player::AdjustThirdPersonCameraByOffsetProbes` -> `src/Battlesport/player.cpp:6784`
- `0x406110` `Player::AdjustThirdPersonCameraBySideProbes` -> `src/Battlesport/player.cpp:6994`
- `0x4063f0` `Player::RestoreThirdPersonCameraFromObstructionState` -> `src/Battlesport/player.cpp:9925`
- `0x406430` `Player::UnbindCurrentSaveStateIfSinglePlayer` -> `src/Battlesport/player.cpp:2840`
- `0x406450` `Player::BindActiveGameStateAsCurrentSaveState` -> `src/Battlesport/player.cpp:2849`
- `0x406470` `Player::UpdateCameraVariantFromCameraPos` -> `src/Battlesport/player.cpp:6946`
- `0x406510` `Player::UpdateCameraVariantFromAnchor` -> `src/Battlesport/player.cpp:6890`
- `0x406610` `Player::UpdateCameraWeatherFxEmitterVisibility` -> `src/Battlesport/player.cpp:9938`
- `0x406730` `Player::FilterCameraProbeBlockingHits` -> `src/Battlesport/player.cpp:6638`
- `0x4067a0` `Player::AdjustSubCameraFocusForObstruction` -> `src/Battlesport/player.cpp:6735`
- `0x42b6e0` `Player::FindNearestThirdPersonCameraProbePoint` -> `src/Battlesport/player.cpp:6679`

## GameZRecoil/Player/player_status.cpp

- `0x43b5d0` `Player::ApplyStatusMeterChange` -> `src/Battlesport/player.cpp:11556`
- `0x43b660` `Player::UpdateStatusMeter` -> `src/Battlesport/player.cpp:11582`

## GameZRecoil/Player/player_weapon.c

- `0x43c9c0` `Player::FindAltGunFireControllerForWeaponId` -> `src/Battlesport/player.cpp:12806`
- `0x43ca90` `Player::CheckMissionWeaponAvailability` -> `src/Battlesport/player.cpp:11868`
- `0x43cc70` `Player::WriteMinesZarSection` -> `src/Battlesport/player.cpp:11253`
- `0x43cdf0` `Player::Mines_ZAR_ReadEntryOrReset` -> `src/Battlesport/player.cpp:11207`

## GameZRecoil/recoilapp.cpp

- `0x419010` `HudSensorTracker::QueueMissionFmvStateForMissionId` -> `src/Battlesport/HudSensorTracker.cpp:2685`
- `0x42edb0` `RecoilApp_MissionFmvState::OnTryBecomeCurrent` -> `src/Battlesport/RecoilApp.cpp:3097`
- `0x42ee50` `RecoilApp_MissionFmvState::OnDeactivate` -> `src/Battlesport/RecoilApp.cpp:3130`
- `0x42ee70` `RecoilApp_MissionFmvState::OnUpdateShouldQuit` -> `src/Battlesport/RecoilApp.cpp:3140`

## GameZRecoil/RecoilApp/GameNet.cpp

- `0x431c50` `GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks` -> `src/Battlesport/GameNet.cpp:2482`
- `0x432300` `GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer` -> `src/Battlesport/GameNet.cpp:1823`
- `0x432e70` `GameNet::ReassignPlayerColorsAndRefreshRows` -> `src/Battlesport/GameNet.cpp:2779`
- `0x432ed0` `GameNet::HandlePkt03_RemoveRemotePlayer` -> `src/Battlesport/GameNet.cpp:2805`
- `0x4330f0` `GameNet::SendPkt0E_PlayerLapProgress` -> `src/Battlesport/GameNet.cpp:3108`
- `0x433170` `GameNet::HandlePkt0E_PlayerLapProgress` -> `src/Battlesport/GameNet.cpp:3281`
- `0x433200` `GameNet::AreAllPlayersAtLapTarget` -> `src/Battlesport/GameNet.cpp:1727`
- `0x433250` `GameNet::HandlePkt0D_HudTimerPanelState` -> `src/Battlesport/GameNet.cpp:2994`
- `0x433310` `GameNet::SendPkt0D_HudTimerPanelState` -> `src/Battlesport/GameNet.cpp:3244`
- `0x433390` `GameNet::SendPkt0C_HudTimerStatusBits` -> `src/Battlesport/GameNet.cpp:3375`
- `0x433410` `GameNet::HandlePkt0C_HudTimerStatusBits` -> `src/Battlesport/GameNet.cpp:2884`
- `0x4334f0` `GameNet::SendPkt09_PlayerScoreboardSnapshot` -> `src/Battlesport/GameNet.cpp:3135`
- `0x4335b0` `GameNet::HandlePkt09_PlayerScoreboardSnapshot` -> `src/Battlesport/GameNet.cpp:3179`
- `0x4336f0` `GameNet::GetLocalPlayerColorIndexOrZero` -> `src/Battlesport/GameNet.cpp:1683`
- `0x433750` `GameNet::SendPkt0B_ChatMessage` -> `src/Battlesport/GameNet.cpp:2964`
- `0x4337e0` `GameNet::HandlePkt0B_ChatMessage` -> `src/Battlesport/GameNet.cpp:2936`
- `0x433840` `GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed` -> `src/Battlesport/GameNet.cpp:1746`
- `0x433c30` `GameNet::HostSendPkt0F_CraterFeature` -> `src/Battlesport/GameNet.cpp:2394`
- `0x433ca0` `GameNet::SendPkt10_QSandEvent` -> `src/Battlesport/GameNet.cpp:2345`
- `0x433de0` `GameNet::HostSendPkt10_QSandFeature` -> `src/Battlesport/GameNet.cpp:2377`
- `0x434190` `GameNet::HandlePkt07_AltGunDispatch` -> `src/Battlesport/GameNet.cpp:2280`
- `0x434230` `GameNet::AltGunDispatchNoOpCallback` -> `src/Battlesport/GameNet.cpp:2336`
- `0x4344b0` `GameNet::HandlePkt14_HudTimerAndFlagsSync` -> `src/Battlesport/GameNet.cpp:3427`

## GameZRecoil/RecoilApp/RecoilApp.cpp

- `0x42e330` `RecoilApp::InitializeDisplay` -> `src/Battlesport/RecoilApp.cpp:1773`

## GameZRecoil/RecoilApp/RecoilStateSaveLoadTransition.cpp

- `0x435a30` `RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit` -> `src/Battlesport/RecoilApp.cpp:165`
- `0x435a40` `RecoilStateSaveLoadTransition::StaticInit` -> `src/Battlesport/RecoilApp.cpp:175`
- `0x435a50` `RecoilStateSaveLoadTransition::RegisterAtExit` -> `src/Battlesport/RecoilApp.cpp:184`
- `0x435a60` `RecoilStateSaveLoadTransition::AtExitDestructor` -> `src/Battlesport/RecoilApp.cpp:193`
- `0x435c80` `RecoilStateSaveLoadTransition::Constructor` -> `src/Battlesport/RecoilApp.cpp:202`
- `0x435cc0` `RecoilStateSaveLoadTransition::Destructor` -> `src/Battlesport/RecoilApp.cpp:213`

## GameZRecoil/RecoilApp/zDEClient_Crater.cpp

- `0x433ad0` `zDEClient_Crater::Execute` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:232`
- `0x433b70` `zDEClient_Crater::NetRelayCallback` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:265`

## GameZRecoil/RecoilApp/zDEClient_QSand.cpp

- `0x433d40` `zDEClient_QSand::NetRelayCallback` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:784`

## GameZRecoil/westwoodonline/WolapiConfigDialog.cpp

- `0x4418b0` `WestwoodOnlineUpgradeConfigDialog::Destructor` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:167`
- `0x4419a0` `WestwoodOnlineUpgradeConfigDialog::DoDataExchange` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:190`
- `0x441a20` `WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:217`
- `0x441a40` `WestwoodOnlineUpgradeConfigDialog::OnInitDialog` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:242`
- `0x441f40` `WestwoodOnlineUpgradeConfigDialog::OnOK` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:321`
- `0x442010` `WestwoodOnlineUpgradeConfigDialog::OnProfileComboKillFocus` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:359`
- `0x442080` `WestwoodOnlineUpgradeConfigDialog::OnProfileComboSelChange` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:382`
- `0x4420c0` `WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:395`
- `0x4420d0` `WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:401`
- `0x4420e0` `WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:407`
- `0x442100` `WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:228`

## GameZRecoil/westwoodonline/WolapiProgressDialog.cpp

- `0x442240` `WestwoodOnlineUpgradeProgressDialog::ScalarDeletingDestructor` -> `src/Battlesport/WestwoodOnlineUpgradeProgressDialog.cpp:106`
- `0x442260` `WestwoodOnlineUpgradeProgressDialog::GetMessageMap` -> `src/Battlesport/WestwoodOnlineUpgradeProgressDialog.cpp:57`
- `0x442270` `WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt` -> `src/Battlesport/WestwoodOnlineUpgradeProgressDialog.cpp:63`

## GameZRecoil/zClass/Camera.c

- `0x44a9f0` `zClass_Camera::gwCameraUpdate` -> `src/GameZRecoil/zClass/Camera.c:1958`
- `0x44aa30` `zClass_Camera::UpdateImpl` -> `src/GameZRecoil/zClass/Camera.c:1887`
- `0x44abf0` `zClass_Camera::BuildWorldTransform` -> `src/GameZRecoil/zClass/Camera.c:1812`
- `0x44ada0` `zClass_Camera::RenderTraverse` -> `src/GameZRecoil/zClass/Camera.c:2024`
- `0x44c1b0` `zClass_Camera::FastAngleXZ` -> `src/GameZRecoil/zClass/Camera.c:1046`
- `0x44c230` `zClass_Camera::FindConvexHullXZ` -> `src/GameZRecoil/zClass/Camera.c:1075`
- `0x44ce70` `zClass_Camera::RenderFrustumGridTiles` -> `src/GameZRecoil/zClass/Camera.c:1491`
- `0x44d200` `zClass_Camera::RenderOverlayNodes` -> `src/GameZRecoil/zClass/Camera.c:1644`
- `0x44d240` `zClass_Camera::RenderWorld` -> `src/GameZRecoil/zClass/Camera.c:1656`
- `0x44d260` `zClass_Camera::gwCameraSetVariantTagOverride` -> `src/GameZRecoil/zClass/Camera.c:1671`
- `0x44d320` `zClass_Camera::SyncViewContextPositions` -> `src/GameZRecoil/zClass/Camera.c:1983`
- `0x44d3a0` `zClass_Camera::RenderScene` -> `src/GameZRecoil/zClass/Camera.c:1704`

## GameZRecoil/zClass/Class.c

- `0x421d60` `zClass_Node::MaskExtraFlagsRecursive` -> `src/GameZRecoil/zClass/Class.c:2537`
- `0x421de0` `zClass_Node::PropagateFlagsRecursive` -> `src/GameZRecoil/zClass/Class.c:2570`
- `0x437ea0` `zClass_Node::SetDiFlagBit0Recursive` -> `src/GameZRecoil/zClass/Class.c:2606`
- `0x447bc0` `zClass_Class::FindNodeRecursiveByName` -> `src/GameZRecoil/zClass/Class.c:1565`
- `0x448760` `zClass_Class::gwNodeGetBBox` -> `src/GameZRecoil/zClass/Class.c:518`
- `0x4497b0` `gwNode::GetWorldPosition` -> `src/GameZRecoil/zClass/Class.c:2323`
- `0x449850` `gwNode::TransformPoint` -> `src/GameZRecoil/zClass/Class.c:2363`
- `0x4498e0` `gwNode::GetWorldPosAndOrientation` -> `src/GameZRecoil/zClass/Class.c:2396`
- `0x44c0e0` `zClass_Class::gwNodeRenderDispatch` -> `src/GameZRecoil/zClass/Class.c:2464`

## GameZRecoil/zClass/cls_di.c

- `0x42ba50` `zClass_cls_di::SnapProbePointYToBestCandidate` -> `src/GameZRecoil/zClass/cls_di.c:1524`
- `0x443c50` `zClass_cls_di::SetBreakOnFirstCandidate` -> `src/GameZRecoil/zClass/cls_di.c:1381`
- `0x443c60` `zClass_cls_di::SetStopAfterFirstHit` -> `src/GameZRecoil/zClass/cls_di.c:1386`
- `0x443c70` `zClass_cls_di::FindBestPickCandidateBelowPoint` -> `src/GameZRecoil/zClass/cls_di.c:1391`
- `0x443d20` `zClass_cls_di::BuildPickCandidateListBelowPoint` -> `src/GameZRecoil/zClass/cls_di.c:1427`
- `0x443f80` `zClass_cls_di::BuildPickCandidateList` -> `src/GameZRecoil/zClass/cls_di.c:2047`
- `0x444310` `zClass_cls_di::BuildPickCandidatesRecursive` -> `src/GameZRecoil/zClass/cls_di.c:2213`
- `0x4443e0` `zClass_cls_di::BuildPickCandidatesForLight` -> `src/GameZRecoil/zClass/cls_di.c:2256`
- `0x4444b0` `zClass_cls_di::BuildPickCandidatesForPointBatch` -> `src/GameZRecoil/zClass/cls_di.c:1900`
- `0x444890` `zClass_cls_di::BuildPickCandidatesForPoints` -> `src/GameZRecoil/zClass/cls_di.c:1550`
- `0x444c50` `zClass_cls_di::BuildPickCandidatesForPointsRecursive` -> `src/GameZRecoil/zClass/cls_di.c:1777`
- `0x444d10` `zClass_cls_di::BuildPickCandidatesForPointsForLight` -> `src/GameZRecoil/zClass/cls_di.c:1840`
- `0x444de0` `zClass_cls_di::RaycastSelectClosestHitBetweenPoints` -> `src/GameZRecoil/zClass/cls_di.c:3597`
- `0x444e90` `zClass_cls_di::RaycastFindClosest` -> `src/GameZRecoil/zClass/cls_di.c:3657`
- `0x4455f0` `zClass_cls_di::BuildPickCandidatesForSegment` -> `src/GameZRecoil/zClass/cls_di.c:3569`
- `0x445650` `zClass_cls_di::BuildPickCandidatesForSegmentChildFallback` -> `src/GameZRecoil/zClass/cls_di.c:4519`
- `0x445a00` `zClass_cls_di::BuildPickCandidatesForSegmentRecursive` -> `src/GameZRecoil/zClass/cls_di.c:3857`
- `0x445b20` `zClass_cls_di::BuildPickCandidatesForSegmentForCamera` -> `src/GameZRecoil/zClass/cls_di.c:3905`
- `0x445c20` `zClass_cls_di::BuildPickCandidatesForSegmentForLight` -> `src/GameZRecoil/zClass/cls_di.c:3938`
- `0x445d40` `zClass_cls_di::BuildProbeHitBatchesForSegments` -> `src/GameZRecoil/zClass/cls_di.c:4092`
- `0x445f60` `zClass_cls_di::BuildPickCandidatesForSegmentsInGridWindow` -> `src/GameZRecoil/zClass/cls_di.c:4176`
- `0x446440` `zClass_cls_di::BuildPickCandidatesForSegmentsRecursive` -> `src/GameZRecoil/zClass/cls_di.c:4308`
- `0x446880` `zClass_cls_di::BuildPickCandidatesForSegmentsForAnimate` -> `src/GameZRecoil/zClass/cls_di.c:3984`
- `0x446970` `zClass_cls_di::BuildPickCandidatesForSegmentsForLight` -> `src/GameZRecoil/zClass/cls_di.c:4039`
- `0x446a80` `zClass_cls_di::FilterRegionsAgainstSphere` -> `src/GameZRecoil/zClass/cls_di.c:3482`
- `0x446ed0` `BBox::ExpandToCorners` -> `src/GameZRecoil/zClass/cls_di.c:1345`
- `0x446f60` `zClass_cls_di::FilterRegions_TryAppendNode` -> `src/GameZRecoil/zClass/cls_di.c:3385`
- `0x4472c0` `zClass_cls_di::IsPickQueryPointOutsideViewBBoxXZ` -> `src/GameZRecoil/zClass/cls_di.c:2292`
- `0x4473e0` `zClass_cls_di::PickTestBBox2D` -> `src/GameZRecoil/zClass/cls_di.c:2329`
- `0x447540` `zClass_cls_di::FilterPointsBBox` -> `src/GameZRecoil/zClass/cls_di.c:2376`
- `0x4476f0` `zClass_cls_di::FrustumTestAndPick` -> `src/GameZRecoil/zClass/cls_di.c:2422`
- `0x484e00` `zClass_cls_di::PickTestMeshAtQueryXZ` -> `src/GameZRecoil/zClass/cls_di.c:3276`
- `0x484fc0` `zClass_cls_di::AppendPickCandidatesForFace` -> `src/GameZRecoil/zClass/cls_di.c:3042`
- `0x485380` `zClass_cls_di::BuildPickCandidatesForSegmentVsBBoxFaces` -> `src/GameZRecoil/zClass/cls_di.c:2491`
- `0x4856d0` `zClass_cls_di::TryGetPolygonHitAtQueryXZ` -> `src/GameZRecoil/zClass/cls_di.c:2940`
- `0x4857f0` `zClass_cls_di::BuildPickCandidateForSegmentVsPolygon` -> `src/GameZRecoil/zClass/cls_di.c:2980`
- `0x485d10` `zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv` -> `src/GameZRecoil/zClass/cls_di.c:3003`
- `0x486290` `zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygon` -> `src/GameZRecoil/zClass/cls_di.c:2739`
- `0x4869a0` `zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv` -> `src/GameZRecoil/zClass/cls_di.c:2824`
- `0x487350` `zClass_cls_di::FilterRegionsAgainstPolygon` -> `src/GameZRecoil/zClass/cls_di.c:2671`
- `0x487540` `zClass_cls_di::FilterRegionsAgainstPolygonWithDamageMaskUv` -> `src/GameZRecoil/zClass/cls_di.c:2558`
- `0x487900` `zClass_cls_di::FilterRegionsAgainstMeshFaces` -> `src/GameZRecoil/zClass/cls_di.c:3330`
- `0x4879c0` `zClass_cls_di::FilterRegionsAgainstHexahedronFaces` -> `src/GameZRecoil/zClass/cls_di.c:3361`

## GameZRecoil/zClass/cls_util.c

- `0x4518e0` `zClass::Shutdown` -> `src/GameZRecoil/zClass/cls_util.c:192`
- `0x451900` `zClass::Init` -> `src/GameZRecoil/zClass/cls_util.c:123`
- `0x451b20` `zClass_cls_util::CopyNodeDisplayInstance` -> `src/GameZRecoil/zClass/cls_util.c:256`
- `0x451bd0` `zClass_cls_util::CopyNodeBaseData` -> `src/GameZRecoil/zClass/cls_util.c:324`
- `0x451f70` `zClass_cls_util::CopyCameraNode` -> `src/GameZRecoil/zClass/cls_util.c:601`
- `0x4520c0` `zClass_cls_util::CopyLightNode_Unimplemented` -> `src/GameZRecoil/zClass/cls_util.c:779`
- `0x4520e0` `zClass_cls_util::CopySoundNode_Unimplemented` -> `src/GameZRecoil/zClass/cls_util.c:793`
- `0x452100` `zClass_cls_util::CopyObject3DNode` -> `src/GameZRecoil/zClass/cls_util.c:690`
- `0x452230` `zClass_cls_util::CopyAnimateNode_Unimplemented` -> `src/GameZRecoil/zClass/cls_util.c:807`
- `0x452250` `zClass_cls_util::CopyLodNode` -> `src/GameZRecoil/zClass/cls_util.c:821`
- `0x4523c0` `zClass_cls_util::CopySequenceNode_Unimplemented` -> `src/GameZRecoil/zClass/cls_util.c:887`
- `0x4523e0` `zClass_cls_util::CopySwitchNode_Stub` -> `src/GameZRecoil/zClass/cls_util.c:901`
- `0x452400` `zClass_cls_util::CopyNodeDispatch` -> `src/GameZRecoil/zClass/cls_util.c:913`
- `0x452500` `zClass_cls_util::CopyNodeWithCloneOptions` -> `src/GameZRecoil/zClass/cls_util.c:974`
- `0x452560` `zClass_cls_util::CopyNode` -> `src/GameZRecoil/zClass/cls_util.c:1003`
- `0x4525d0` `BBox::MinMaxToBoundingSphere` -> `src/GameZRecoil/zClass/cls_util.c:1038`
- `0x452650` `BBox::CornersToBoundingSphere` -> `src/GameZRecoil/zClass/cls_util.c:57`

## GameZRecoil/zClass/cls_world.c

- `0x4501c0` `zClass_World::gwWorldNew` -> `src/GameZRecoil/zClass/cls_world.c:248`
- `0x4502b0` `zClass_World::InitVirtualAreaPartitions` -> `src/GameZRecoil/zClass/cls_world.c:562`
- `0x450510` `zClass_World::SetVirtualPartition` -> `src/GameZRecoil/zClass/cls_world.c:612`
- `0x450650` `zClass_World::WorldToGridCoordsClampedEx` -> `src/GameZRecoil/zClass/cls_world.c:928`
- `0x450790` `zClass_World::WorldToGridCoordsClamped` -> `src/GameZRecoil/zClass/cls_world.c:976`
- `0x450a00` `zClass_World::GetAreaPartitionAtGrid` -> `src/GameZRecoil/zClass/cls_world.c:1014`
- `0x450ae0` `zClass_World::SetPendingFogState` -> `src/GameZRecoil/zClass/cls_world.c:280`
- `0x450af0` `zClass_World::SetPendingFogColorRgb01` -> `src/GameZRecoil/zClass/cls_world.c:293`
- `0x450b20` `zClass_World::SetPendingFogAltitudeRange` -> `src/GameZRecoil/zClass/cls_world.c:310`
- `0x450b40` `zClass_World::SetPendingFogRange` -> `src/GameZRecoil/zClass/cls_world.c:325`
- `0x450b60` `zClass_World::SetPendingFogDensity` -> `src/GameZRecoil/zClass/cls_world.c:407`
- `0x450b80` `zClass_World::GetPendingFogDensity` -> `src/GameZRecoil/zClass/cls_world.c:340`
- `0x450b90` `zClass_World::GetPendingFogState` -> `src/GameZRecoil/zClass/cls_world.c:352`
- `0x450ba0` `zClass_World::GetPendingFogColorRgb01` -> `src/GameZRecoil/zClass/cls_world.c:364`
- `0x450bc0` `zClass_World::GetPendingFogRange` -> `src/GameZRecoil/zClass/cls_world.c:379`
- `0x450be0` `zClass_World::GetPendingFogAltitudeRange` -> `src/GameZRecoil/zClass/cls_world.c:393`
- `0x450c00` `zClass_World::gwWorldSetOrigin` -> `src/GameZRecoil/zClass/cls_world.c:420`
- `0x450c30` `zClass_World::gwWorldSetSize` -> `src/GameZRecoil/zClass/cls_world.c:436`
- `0x450c60` `zClass_World::gwWorldSetVirtualAreaPartition` -> `src/GameZRecoil/zClass/cls_world.c:488`
- `0x450f00` `zClass_World::gwWorldSetPartitionInclusionTolerance` -> `src/GameZRecoil/zClass/cls_world.c:452`
- `0x450f20` `zClass_World::gwWorldSetMaxDecFeatures` -> `src/GameZRecoil/zClass/cls_world.c:465`
- `0x451360` `zClass_World::AddLight` -> `src/GameZRecoil/zClass/cls_world.c:1328`
- `0x451540` `zClass_World::InitLightPointInPolygonXZ` -> `src/GameZRecoil/zClass/cls_world.c:1429`
- `0x451560` `zClass_World::UpdateAllLights` -> `src/GameZRecoil/zClass/cls_world.c:1441`
- `0x451590` `zClass_World::AddSound` -> `src/GameZRecoil/zClass/cls_world.c:1453`
- `0x451770` `zClass_World::UpdateAllSounds` -> `src/GameZRecoil/zClass/cls_world.c:1554`
- `0x4517a0` `zClass_World::WriteSettingsSection` -> `src/GameZRecoil/zClass/cls_world.c:142`
- `0x451840` `zClass_World::ReadSettingsSection` -> `src/GameZRecoil/zClass/cls_world.c:197`

## GameZRecoil/zClass/cls_zbd.c

- `0x4544b0` `GameZ_ZBD::WriteSingleNodeClassData` -> `src/GameZRecoil/zClass/cls_zbd.c:394`
- `0x454890` `GameZ_ZBD::WriteNodeTable` -> `src/GameZRecoil/zClass/cls_zbd.c:646`
- `0x454a50` `GameZ::WriteZBDFile` -> `src/GameZRecoil/zClass/cls_zbd.c:86`
- `0x454c60` `GameZ_ZBD::ReadSingleNodeClassData` -> `src/GameZRecoil/zClass/cls_zbd.c:774`
- `0x455350` `GameZ_ZBD::ReadNodeTable` -> `src/GameZRecoil/zClass/cls_zbd.c:1189`
- `0x455520` `GameZ::ReadZBDFile` -> `src/GameZRecoil/zClass/cls_zbd.c:166`
- `0x4556a0` `GameZ::OpenAndReadZBDHeader` -> `src/GameZRecoil/zClass/cls_zbd.c:268`
- `0x455730` `GameZ_ZBD::ReloadDisplayInstancesFromCurrentPath_Local` -> `src/GameZRecoil/zClass/cls_zbd.c:1274`
- `0x4557a0` `GameZ_ZBD::ReloadDisplayInstancesRecursive_Local` -> `src/GameZRecoil/zClass/cls_zbd.c:1306`

## GameZRecoil/zClass/List.c

- `0x44f1d0` `zClass_List::gwListDeleteANode` -> `src/GameZRecoil/zClass/List.c:225`
- `0x44f630` `zClass_List::RenderActiveCameras` -> `src/GameZRecoil/zClass/List_RenderActiveCameras.cpp:17`

## GameZRecoil/zDEClient/zdec_crater.c

- `0x456ad0` `zDEClient_Crater::DestroyFeature` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:296`
- `0x456b00` `zDEClient_Crater::InitEventTemplateDefaults` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:316`
- `0x456b20` `zDEClient_Crater::InstanceEvent` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:693`
- `0x456c50` `zDEClient_Crater::InstanceEventMaybeRelay` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:766`
- `0x456c80` `zDEClient_Crater::InitFeatureFromEventTemplate` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:367`
- `0x457040` `zDEClient_Crater::CreateFeatureStructFromEventTemplate` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:328`
- `0x4570e0` `zDEClient_Crater::Build` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:514`
- `0x457140` `zDEClient_Crater::CreateFeature` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:547`

## GameZRecoil/zDEClient/zdec_init.c

- `0x455dd0` `zDEClient::LoadMaterialFromTexturePath_Local` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2157`
- `0x455ea0` `zDEClient_QSand::DestroyFeature` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:813`
- `0x455ed0` `zDEClient::CopyQSandEventTemplateDefaults` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1850`

## GameZRecoil/zDEClient/zdec_init.cpp

- `0x4558f0` `zDEClient::LoadConfigResources` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1862`
- `0x4575f0` `zDEClient::SubmitFeatureGeometry` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2313`
- `0x457650` `zDEClient::InitFeatureSystem` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1810`
- `0x457660` `zDEClient::InitFeatureEntryListAndMapTree` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1817`
- `0x4576a0` `zDEClient::RegisterFeatureSystemCleanupAtExit` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1833`
- `0x4576b0` `zDEClient::ShutdownFeatureSystem` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1839`
- `0x4576e0` `zDEClient_MapTreeState::Destroy` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1788`
- `0x457750` `zDEClient::ClearFeatureDisplayNodes` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2450`
- `0x457840` `zDEClient::AppendFeatureEntry` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2252`
- `0x457b40` `zDEClient::WriteFeatureSectionsToZAR` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2338`
- `0x457c10` `zDEClient::ApplyFeatureEntry` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2399`
- `0x457c50` `zDEClient::DispatchFeatureEventTemplates` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2422`
- `0x457cc0` `zDEClient_MapTreeState::InitState` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1756`
- `0x457d90` `zDEClient_MapTreeState::FindOrInsertKey` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1682`
- `0x4585a0` `zDEClient_MapTreeState::InsertAt` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1502`
- `0x458a30` `zDEClient::CopyFeatureEntriesForward` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2216`
- `0x458a70` `zDEClient::FillFeatureEntries` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2235`
- `0x458aa0` `zDEClient::SetCameraNode` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2184`
- `0x458ac0` `zDEClient::GetFeatureGridCell` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2195`
- `0x458ae0` `zDEClient::GetCameraNode` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2210`
- `0x46af00` `zGeometry_ClipPatchOutput::Create` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1310`
- `0x46af20` `zGeometry_ClipPatchOutput::Destroy` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1322`
- `0x46af40` `zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1377`

## GameZRecoil/zDEClient/zdec_qsand.c

- `0x455ef0` `zDEClient_QSand::InstanceEventMaybeRelay` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1244`
- `0x456010` `zDEClient_QSand::InitFeatureFromEventTemplate` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:864`
- `0x4563d0` `zDEClient_QSand::CreateFeatureStructFromEventTemplate` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:833`
- `0x456450` `zDEClient_QSand::Build` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1011`
- `0x4564b0` `zDEClient_QSand::CreateFeature` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1044`

## GameZRecoil/zEffect/eff_runtime.c

- `0x461f00` `zEffect::SpawnRuntimeInstanceAt` -> `src/GameZRecoil/zEffect/zEffect.cpp:5647`
- `0x461f50` `zEffect::ActivateRuntimeEntryAtPosition` -> `src/GameZRecoil/zEffect/zEffect.cpp:5676`
- `0x462050` `zEffect::ComputeDistanceSqToListener` -> `src/GameZRecoil/zEffect/zEffect.cpp:5732`
- `0x4620d0` `zEffect::AcquireRuntimeEntryByIndex` -> `src/GameZRecoil/zEffect/zEffect.cpp:5749`
- `0x462130` `zEffect::CloneRuntimeEntryFromTemplate` -> `src/GameZRecoil/zEffect/zEffect.cpp:5775`
- `0x4621b0` `zEffect::RuntimeNodeActionCallback` -> `src/GameZRecoil/zEffect/zEffect.cpp:5811`

## GameZRecoil/zEffect/Effect.c

- `0x462280` `zEffect::FindTemplateIndexByName` -> `src/GameZRecoil/zEffect/zEffect.cpp:5873`

## GameZRecoil/zEffect/zeff_anim.c

- `0x45e0d0` `zEffectAnimEntry::SetOnStateDoneCallback` -> `src/GameZRecoil/zEffect/zEffect.cpp:929`
- `0x45e200` `zEffect::SetWorldNode` -> `src/GameZRecoil/zEffect/zEffect.cpp:5424`
- `0x45e270` `zEffect::SetResourceNode` -> `src/GameZRecoil/zEffect/zEffect.cpp:5431`
- `0x45ffa0` `zEffectAnim::FindNextAsyncEntry` -> `src/GameZRecoil/zEffect/zEffect.cpp:2082`
- `0x460010` `zEffectAnim::GetRootNodeOrNull` -> `src/GameZRecoil/zEffect/zEffect.cpp:2805`

## GameZRecoil/zEffect/zeff_anim_activation.c

- `0x461970` `zEffectAnim::QueueCmdType1TransformRotVelocity` -> `src/GameZRecoil/zEffect/zEffect.cpp:1746`
- `0x461aa0` `zEffectAnim::QueueCmdType2Velocity` -> `src/GameZRecoil/zEffect/zEffect.cpp:1817`
- `0x461ba0` `zEffectAnim::QueueCmdType3PositionRefAndVelocity` -> `src/GameZRecoil/zEffect/zEffect.cpp:1876`
- `0x461d00` `zEffectAnim::QueueCmdType4TransformRefs` -> `src/GameZRecoil/zEffect/zEffect.cpp:1952`

## GameZRecoil/zEffect/zeff_anim_init.c

- `0x458b50` `zEffect::TickResetDelayOnTimer` -> `src/GameZRecoil/zEffect/zEffect.cpp:5438`
- `0x458bb0` `zEffect::TickResetDelayOnHit` -> `src/GameZRecoil/zEffect/zEffect.cpp:5466`
- `0x45e210` `zEffect_Anim::SetZbdFilename` -> `src/GameZRecoil/zEffect/zEffect.cpp:2818`
- `0x45e730` `zEffectAnim::CloneEntryForNode` -> `src/GameZRecoil/zEffect/zEffect.cpp:2458`
- `0x45ed80` `zEffectAnim::RebindEntryToNode` -> `src/GameZRecoil/zEffect/zEffect.cpp:2321`
- `0x45efb0` `zEffect_Anim::LoadZbd` -> `src/GameZRecoil/zEffect/zEffect.cpp:2840`
- `0x45fb30` `zEffect_Anim::LoadAndInstantiate` -> `src/GameZRecoil/zEffect/zEffect.cpp:3244`

## GameZRecoil/zEffect/zeff_anim_save.c

- `0x460490` `zEffect_Anim::SaveActivationRecords` -> `src/GameZRecoil/zEffect/zEffect.cpp:3400`
- `0x4606d0` `zEffect_Anim::LoadActivationRecords` -> `src/GameZRecoil/zEffect/zEffect.cpp:3517`
- `0x460bc0` `zEffect_Anim::SaveRunningAnimRecord` -> `src/GameZRecoil/zEffect/zEffect.cpp:3725`
- `0x460f80` `zEffect_Anim::SaveRunningAnimRecords` -> `src/GameZRecoil/zEffect/zEffect.cpp:3878`
- `0x461040` `zEffect_Anim::LoadRunningAnimRecords` -> `src/GameZRecoil/zEffect/zEffect.cpp:3915`
- `0x461430` `zEffect_Anim::SaveAnimRecords` -> `src/GameZRecoil/zEffect/zEffect.cpp:4141`
- `0x461670` `zEffect_Anim::LoadAnimRecords` -> `src/GameZRecoil/zEffect/zEffect.cpp:4240`
- `0x461800` `zEffect_Anim::GetActivationRecordPackedSize` -> `src/GameZRecoil/zEffect/zEffect.cpp:5116`
- `0x461840` `zEffect_Anim::ResetFromActivationRecord` -> `src/GameZRecoil/zEffect/zEffect.cpp:4346`
- `0x461870` `zEffect_Anim::ProcessActivationRecord` -> `src/GameZRecoil/zEffect/zEffect.cpp:4357`

## GameZRecoil/zEffect/zeff_init.c

- `0x460070` `zEffect::InitFromPath` -> `src/GameZRecoil/zEffect/zEffect.cpp:5263`
- `0x461ec0` `zEffect::FindNodeUserDataRecursive` -> `src/GameZRecoil/zEffect/zEffect.cpp:5623`

## GameZRecoil/zEffect/zeffect.cpp

- `0x458af0` `zEffect::SetConditionalRefPos` -> `src/GameZRecoil/zEffect/zEffect.cpp:5497`
- `0x458b20` `zEffect::SetVariantOverridePackedIdsIfComplete` -> `src/GameZRecoil/zEffect/zEffect.cpp:5515`

## GameZRecoil/zGame/Player/Player_Camera.cpp

- `0x426330` `Player::ResetMouseControlStateAndRecenterCursor` -> `src/Battlesport/player.cpp:9303`

## GameZRecoil/zGame/Player/Player_Damage.cpp

- `0x439990` `Player::ResetDamageStateAndTimedHitStatus` -> `src/Battlesport/player.cpp:13307`

## GameZRecoil/zGame/zGame_Options.cpp

- `0x4081a0` `zOpt::SetGraphicsFlagsForCurrentHwMode` -> `src/GameZRecoil/zGame/zGame.cpp:1659`
- `0x4081f0` `zOpt::GetGraphicsFlagsForCurrentHwMode` -> `src/GameZRecoil/zGame/zGame.cpp:1650`

## GameZRecoil/zGame/zopt.c

- `0x407190` `zOpt::LookupNamedValueAsInt` -> `src/GameZRecoil/zGame/zGame.cpp:1216`

## GameZRecoil/zGeometry/zgeo_convexify.cpp

- `0x46bd50` `zGeometry_TriangulateHole::TryAppendBridgeEdge` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:528`
- `0x46bf30` `zGeometry_TriangulateHole::CollectActiveEdgeIndicesForVertex` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:571`
- `0x46bf70` `zGeometry_TriangulateHole::FindActiveEdgeState` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:504`
- `0x46bfc0` `zGeometry_TriangulateHole::TryEmitTriangleFromEdgePair` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:593`
- `0x46c070` `zGeometry::TriangulatePolygonWithHole` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:679`
- `0x46c5b0` `zGeometry_Vec3Array::ReversePoints` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:406`
- `0x46c620` `zGeometry_Vec3Array::EnsurePositiveCrossZ` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:425`
- `0x46c720` `zGeometry_ConvexPolygonSet::Destroy` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:1134`
- `0x46c760` `zGeometry_Polygon::Convexify` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:1045`
- `0x46cb50` `zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:921`
- `0x46ced0` `zGeometry_Polygon::TrySplitPointDwordOffsetsAtBestDiagonal` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:829`

## GameZRecoil/zGeometry/zgeo_model.cpp

- `0x469e50` `zGeometry_Vec3::IsNearEqualXY` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:626`
- `0x469e90` `zGeometry_Vec3::SnapPointToSegmentXYIfNear` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:640`
- `0x46a130` `zGeometry_Polygon::SnapPointsXYIfNear` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:721`
- `0x46a5e0` `zGeometry_Vec3Array::RotateNeg90AroundX` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:771`
- `0x46a620` `zGeometry_Bounds2D::OverlapsWithUnitMargin` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:98`
- `0x46a690` `zGeometry_Model::FindOrCreateRandomDebugMaterial` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:962`
- `0x46a770` `zGeometry_Model::AddPolygonToDi` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:983`
- `0x46a7f0` `zGeometry_Model::BuildPolygonUvList` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1025`
- `0x46a8e0` `zGeometry_Polygon::SolveUvAxisCoefficientsXZ` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:690`
- `0x46aa40` `zGeometry_ClipPolygon::CreateFromPointList` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:896`
- `0x46aab0` `zGeometry_ClipPolygon::CopyPointsOutRotatedBack` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:932`
- `0x46b030` `zGeometry_ClipPolygon::SnapPointsNearNodeModelXY` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:790`
- `0x46b1f0` `zGeometry_Model::ClipPatch` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:378`
- `0x46b550` `zGeometry_ClipPolygon::ProcessNodePolygonSetXY` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:580`
- `0x46b6d0` `zGeometry_Model::ProcessClipPatchNode` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:126`
- `0x46ba90` `zGeometry_Model::AddPointListPolygonToDi` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1073`
- `0x46bb90` `zGeometry_Model::IsFullyInsideClipPolygonXY` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1162`

## GameZRecoil/zGeometry/zgeo_weiler.cpp

- `0x464670` `zGeometry_Weiler::GetInputContourAPointList` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1154`
- `0x464680` `zGeometry_Weiler::Init` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1168`
- `0x464790` `zGeometry_ClipPolygon::ResetWeilerStateFromContourPoints` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4014`
- `0x4647d0` `zGeometry_Weiler::DestroyState` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3843`
- `0x464810` `zGeometry_Weiler::ClipPointList` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1326`
- `0x464b30` `zGeometry_WeilerClipOutput::Destroy` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3861`
- `0x464b90` `zGeometry_Weiler::InitInputContourPair` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1253`
- `0x464c90` `zGeometry_Weiler::ClassifyInputContourPairBounds` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2399`
- `0x464ea0` `zGeometry_Weiler::OutputPreclassifiedContourPairResult` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2340`
- `0x464f70` `zGeometry_Weiler::PreclassifyInputContourPair` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2889`
- `0x465ac0` `zGeometry_Weiler::ClassifyContainedContour` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3262`
- `0x467600` `zGeometry_WeilerBuffer::Init` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1008`
- `0x467630` `zGeometry_WeilerBuffer::Destroy` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1064`
- `0x467660` `zGeometry_WeilerBuffer::GetAppendSpace` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1026`
- `0x4676c0` `zGeometry_Weiler::EnsureContourOutput` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1577`
- `0x467710` `zGeometry_Weiler::MergeContours` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1610`
- `0x4680b0` `zGeometry_Weiler::NewContour` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2188`
- `0x4681a0` `zGeometry_Weiler::OutputContoursForClipMode` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2551`
- `0x4682c0` `zGeometry_Weiler::OutputContourToPolygonSet` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2497`
- `0x4683a0` `zGeometry_Weiler::TogglePointAxesForContourSource` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3719`
- `0x468410` `zGeometry_WeilerContourSegment::UpdateBounds` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1081`
- `0x468470` `zGeometry_Weiler::BuildPointSideTablesForContourPair` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2866`
- `0x468580` `zGeometry_Weiler::DivideContourSegmentAtPoint` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2052`
- `0x468650` `zGeometry_Weiler::CreateForwardSegmentPairAtPoint` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2114`
- `0x468700` `zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2453`
- `0x4687b0` `zGeometry_Weiler::GenerateOutsideResults` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2679`
- `0x468a10` `zGeometry_Weiler::ClassifyPointInContourPointListXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2258`
- `0x468c40` `zGeometry_Weiler::Intersect2d` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3454`
- `0x468fa0` `zGeometry_Weiler::ClassifyIntersect2d` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3356`
- `0x4693a0` `zGeometry_WeilerContourSegmentArray::UpdateBounds` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1111`
- `0x4693c0` `zGeometry_WeilerContourSegmentArray::InitFromPointList` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1123`
- `0x469430` `zGeometry_Weiler::GetNextContourSegmentForTraversal` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2166`
- `0x469450` `zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3582`
- `0x469560` `zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3621`
- `0x469960` `zGeometry_Weiler::RecenterPointSetsIfOutOfRange` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3739`
- `0x469a30` `zGeometry_Weiler::PreclassifyInputContourAAdjacentEdgePairs` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2796`
- `0x469ae0` `zGeometry_WeilerBuffer::SetCountAndAppendPtr` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:1054`
- `0x469af0` `zGeometry_Weiler::RestorePointTranslation` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3777`
- `0x469b60` `zGeometry_Weiler::RestoreOutputZFromInputPlane` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3801`
- `0x469ca0` `zGeometry_Vec3::IsBetweenEndpointsXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:885`
- `0x469d60` `zGeometry_Weiler::SelectForwardStartPointInContourA` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:2628`
- `0x46a080` `zGeometry_Vec3Array::RemoveAdjacentDuplicatePointsXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:909`
- `0x46a1f0` `zGeometry_Weiler::ValidateXings` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3672`
- `0x46a600` `zGeometry_Vec3Array::RotatePos90AroundX` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:957`
- `0x46a9c0` `zGeometry_Vec3Array::ComputeBoundsXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:974`
- `0x46ab10` `zGeometry_ClipPolygon::FinalizeAndDestroy` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:4038`
- `0x46ab40` `zGeometry_ClipPolygon::FindPointIndexXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3893`
- `0x46ab90` `zGeometry_ClipPolygon::UpsertPointListXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3958`
- `0x46ac80` `zGeometry_ClipPolygon::FindPointInsertionEdgeXYIndex` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:3912`
- `0x46ae40` `zGeometry_ClipPatchOutput::ApplyNodeDiPairs` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1334`
- `0x46b650` `zGeometry_Model::GetLinearBufferOfPolygonVertices` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:861`
- `0x46bb30` `zGeometry_Model::AddIndexedPolygonToDi` -> `src/GameZRecoil/zGeometry/zgeo_model.cpp:1128`
- `0x46be20` `zGeometry_Segment::IntersectsSegmentXY` -> `src/GameZRecoil/zGeometry/zgeo_weiler.cpp:826`

## GameZRecoil/zGeometry/zgeometry.cpp

- `0x46c390` `zGeometry_TriangulateHole::CacheCombinedPlane` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:647`
- `0x46c3a0` `zGeometry_Vec3Array::ComputeNewellPlane` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:456`
- `0x46c570` `zGeometry_TriangulateHole::ProjectInnerRingOntoCachedPlane` -> `src/GameZRecoil/zGeometry/zgeo_convexify.cpp:661`

## GameZRecoil/zHud/HudUiBackground.cpp

- `0x4b9850` `HudUiBackground::SetEnabled` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7521`

## GameZRecoil/zHud/HudUiNetGameSetup.cpp

- `0x41ad80` `HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15388`

## GameZRecoil/zImage/zimg_fonts.cpp

- `0x4c7f00` `zImage_Font::BlitStringToActiveTarget` -> `src/GameZRecoil/zImage/zimg_texture.cpp:708`

## GameZRecoil/zImage/zimg_texture.cpp

- `0x46d310` `zImage::TexDirEntryToIndex` -> `src/GameZRecoil/zImage/zimg_texture.cpp:297`
- `0x46d340` `zImage::TexIndexToDirEntry` -> `src/GameZRecoil/zImage/zimg_texture.cpp:309`
- `0x46d360` `zImage::WriteTextureDirectory` -> `src/GameZRecoil/zImage/zimg_texture.cpp:372`
- `0x46d420` `zImage::ReadTextureDirectory` -> `src/GameZRecoil/zImage/zimg_texture.cpp:415`
- `0x46e3e0` `zImage_TexDirEntry::BuildMipChain` -> `src/GameZRecoil/zImage/zimg_texture.cpp:143`
- `0x46eba0` `zImg::Init` -> `src/GameZRecoil/zImage/zimg_texture.cpp:107`
- `0x4902b0` `zVid_Image::CalcPow2ScratchFields` -> `src/GameZRecoil/zVideo/zVideo.cpp:5354`

## GameZRecoil/zImage/zvid_buff.c

- `0x48d910` `zVid::DrawNoiseRect` -> `src/GameZRecoil/zVideo/zVideo.cpp:4415`
- `0x48ff60` `zVid::ShutdownFrameScratchBuffers` -> `src/GameZRecoil/zVideo/zVideo.cpp:4483`
- `0x48ff70` `zVid::InitFrameScratchBuffers` -> `src/GameZRecoil/zVideo/zVideo.cpp:4472`
- `0x4a6800` `zVideo::GetPrimarySurfaceWidth` -> `src/GameZRecoil/zVideo/zVideo.cpp:2595`
- `0x4a6fe0` `zVideo_buff::CopySurfaceRectToImage` -> `src/GameZRecoil/zVideo/zVideo.cpp:2111`

## GameZRecoil/zInterp/interp_context.c

- `0x4c0d20` `zInterp_Context::Constructor` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:337`
- `0x4c0e50` `zInterp_Context::Destructor` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:463`

## GameZRecoil/zInterp/zinterp_parse.cpp

- `0x414a60` `zInterp_GlobalContext::StaticInitAndRegisterAtExit` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:423`
- `0x414a70` `zInterp_GlobalContext::StaticInit` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:405`
- `0x414a80` `zInterp_GlobalContext::RegisterAtExit` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:411`
- `0x414a90` `zInterp_GlobalContext::AtExitDestructor` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:417`
- `0x414ab0` `zInterp_GlobalContext::Constructor` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:394`
- `0x414ad0` `zInterp_GlobalContext::DispatchHook` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:318`
- `0x4c0f70` `zInterp_Context::Destroy` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:430`
- `0x4c1020` `zInterp_Context::RunString` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1110`
- `0x4c1090` `zInterp_Context::RunStream` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1142`
- `0x4c1160` `zInterp_Context::ReadLineOrPreparedTokens` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1272`
- `0x4c1250` `zInterp_Context::ExpandMacroRefs` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:615`
- `0x4c13c0` `zInterp_Context::TokenizeLine` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1192`
- `0x4c1500` `zInterp_Context::RunScriptFile` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1050`
- `0x4c15f0` `zInterp_Context::FindMacroValue` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:212`
- `0x4c1670` `zInterp_Context::ClearMacroTable` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:289`
- `0x4c16c0` `zInterp_Context::ClearVarTable` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:304`
- `0x4c1710` `zInterp_Context::IsMacroTrue` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:234`
- `0x4c1780` `zInterp_Context::SetMacro` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:249`
- `0x4c1870` `zInterp_Context::EchoTokens` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1259`
- `0x4c18c0` `zInterp_Context::PushFileFrame` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1358`
- `0x4c1940` `zInterp_Context::PopFileFrame` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1345`
- `0x4c1960` `zInterp_Context::ClearFileFrameStack` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1335`
- `0x4c1990` `zInterp_Context::NextToken` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:690`
- `0x4c19c0` `zInterp_Context::ParseBoolToken` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:708`
- `0x4c1a00` `zInterp_Context::ParseFloatToken` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:725`
- `0x4c1a20` `zInterp_Context::ParseIntToken` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:736`
- `0x4c1a40` `zInterp_Context::FindVarEntry` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:747`
- `0x4c1ab0` `zInterp_Context::DumpVarEntry` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:765`
- `0x4c1b20` `zInterp_Context::IncErrorCount` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:197`
- `0x4c1b30` `zInterp_Context::Logf` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:154`
- `0x4c1b50` `zInterp_Context::EvalConditionExpr` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:565`
- `0x4c1c50` `zInterp_Context::HandleBuiltinCommand` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1379`
- `0x4c2030` `zInterp_Context::PrintNodeTree` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:3785`
- `0x4c2090` `zInterp_Context::ReportParseError` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:203`
- `0x4c20a0` `zInterp_Context::DispatchCoreCommand` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:1561`
- `0x4c5480` `zInterp_Context::CommandEqualsPrefix` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:806`
- `0x4c54b0` `zInterp_Context::CommandEquals` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:824`
- `0x4c5510` `zInterp_Context::GetCurrentCommand` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:840`
- `0x4c5520` `zInterp_Context::ReportErrorf` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:175`
- `0x4c5550` `zInterp_Context::LoadPreparedScriptIndex` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:896`
- `0x4c5740` `zInterp_Context::OpenPreparedScriptStream` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:999`
- `0x4c5820` `zInterp_Context::ValidateArgsAndNodeType` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:850`
- `0x4c58c0` `zInterp_Context::DefaultDispatchHook` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:106`
- `0x4c58e0` `zInterp_Context::RegisterScrollAlwaysNode` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:503`
- `0x4c59e0` `zInterp_Object3D::DefaultRenderAction` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:122`
- `0x4c5a00` `zInterp_Object3D::ScrollAlwaysTickAction` -> `src/GameZRecoil/zInterp/zinterp_parse.cpp:135`

## GameZRecoil/zMath.cpp

- `0x472670` `zMath::Vec3DeltaLengthSq` -> `src/GameZRecoil/zMath/zMath.cpp:897`
- `0x4726d0` `zMath::Vec3DeltaLength` -> `src/GameZRecoil/zMath/zMath.cpp:879`

## GameZRecoil/zMath/Math.c

- `0x474d90` `zMath_Vec3_ElevationAngleBetweenPoints` -> `src/GameZRecoil/zMath/zMath.cpp:1563`

## GameZRecoil/zMath/zmath.cpp

- `0x474fc0` `zMath::ApproxExpNeg` -> `src/GameZRecoil/zMath/zMath.cpp:1485`

## GameZRecoil/zMath/zMath.cpp

- `0x476480` `zMath::ProjectPointAndClampToScreenClip` -> `src/GameZRecoil/zMath/zMath.cpp:1409`

## GameZRecoil/zMath/zmath_mat.cpp

- `0x474260` `zMath::MatBuildEulerRotation3x3` -> `src/GameZRecoil/zMath/zMath.cpp:1209`

## GameZRecoil/zMath/zmath_matload.cpp

- `0x473280` `zMath::MatLoadRotationFrom3x3` -> `src/GameZRecoil/zMath/zMath.cpp:952`

## GameZRecoil/zMath/zmath_matrix.cpp

- `0x473690` `zMath_Mat_Scale` -> `src/GameZRecoil/zMath/zMath.cpp:1511`
- `0x4737e0` `zMath::MatTranslate` -> `src/GameZRecoil/zMath/zMath.cpp:1015`
- `0x473970` `zMath::MatRotateX` -> `src/GameZRecoil/zMath/zMath.cpp:1037`
- `0x473cc0` `zMath::MatRotateZ` -> `src/GameZRecoil/zMath/zMath.cpp:1115`
- `0x474710` `zMath_Mat_TransformNormalBatch` -> `src/GameZRecoil/zMath/zMath.cpp:284`

## GameZRecoil/zMath/zmath_matstack.cpp

- `0x472ef0` `zMath::MatStackPushAndCloneParent` -> `src/GameZRecoil/zMath/zMath.cpp:503`

## GameZRecoil/zMath/zmath_proj.cpp

- `0x4743e0` `zMath_SetScreenSize` -> `src/GameZRecoil/zMath/zMath.cpp:337`
- `0x474400` `zMath_Setup_Projection` -> `src/GameZRecoil/zMath/zMath.cpp:349`

## GameZRecoil/zMath/zmath_project.cpp

- `0x472ed0` `zMath_Project_GetLastScreenScaleXY` -> `src/GameZRecoil/zMath/zMath.cpp:1554`

## GameZRecoil/zMath/zMath_Quat.cpp

- `0x475910` `zMath_Quat_Multiply` -> `src/GameZRecoil/zMath/zMath.cpp:1926`
- `0x475b80` `zMath_Quat_FromRotationVector` -> `src/GameZRecoil/zMath/zMath.cpp:1997`

## GameZRecoil/zMath/zmath_vec.cpp

- `0x474580` `zMath_Vec3_DirFromYaw` -> `src/GameZRecoil/zMath/zMath.cpp:1823`
- `0x4745c0` `zMath::Vec3PerpXZ` -> `src/GameZRecoil/zMath/zMath.cpp:612`
- `0x4745e0` `zMath_Vec3Array_UntransformDirection` -> `src/GameZRecoil/zMath/zMath.cpp:313`
- `0x474670` `zMath::Vec3ArrayTransformDirection` -> `src/GameZRecoil/zMath/zMath.cpp:1292`
- `0x474f40` `zMath::Vec3RotateY` -> `src/GameZRecoil/zMath/zMath.cpp:1277`

## GameZRecoil/zMath/zmath_vec2.cpp

- `0x472cc0` `zMath::Vec3Perp2D` -> `src/GameZRecoil/zMath/zMath.cpp:592`

## GameZRecoil/zMath/zmath_vec3.cpp

- `0x42d560` `zMath::Vec3Midpoint` -> `src/GameZRecoil/zMath/zMath.cpp:858`
- `0x472730` `zMath::Vec3DistSqXZ` -> `src/GameZRecoil/zMath/zMath.cpp:914`
- `0x472770` `zMath::Vec3ScaleAdd` -> `src/GameZRecoil/zMath/zMath.cpp:624`
- `0x4727a0` `zMath_Vec3_DivScalar` -> `src/GameZRecoil/zMath/zMath.cpp:2089`
- `0x4727f0` `zMath::Vec3NormalizeXZ` -> `src/GameZRecoil/zMath/zMath.cpp:570`
- `0x472860` `zMath::Vec3Reflect` -> `src/GameZRecoil/zMath/zMath.cpp:639`
- `0x472960` `zMath::Vec3Lerp` -> `src/GameZRecoil/zMath/zMath.cpp:673`
- `0x4729b0` `zMath::Vec3DirectionTo` -> `src/GameZRecoil/zMath/zMath.cpp:703`
- `0x4729f0` `zMath::Vec3LerpNormalize` -> `src/GameZRecoil/zMath/zMath.cpp:687`
- `0x472a10` `zMath::Vec3Slerp` -> `src/GameZRecoil/zMath/zMath.cpp:721`
- `0x475070` `zMath_Vec3_TriangleNormal` -> `src/GameZRecoil/zMath/zMath.cpp:2039`

## GameZRecoil/zMath/zMathGeom.cpp

- `0x475210` `zMath::LineVsSphereHit` -> `src/GameZRecoil/zMath/zMath.cpp:787`

## GameZRecoil/zMath/zMathMisc.cpp

- `0x475130` `zMath_SolveLinearGradient2D` -> `src/GameZRecoil/zMath/zMath.cpp:2055`

## GameZRecoil/zMath/zmth_main.c

- `0x472d30` `zMath::CrtMatherrHandler` -> `src/GameZRecoil/zMath/zMath.cpp:45`

## GameZRecoil/zModel/zmodel.cpp

- `0x484960` `zDi::BuildPickCandidateForQueryPoint` -> `src/GameZRecoil/zClass/cls_di.c:3133`
- `0x484b70` `zModelConst::AddFaceToPlayerProbeSampleBuckets` -> `src/GameZRecoil/zClass/cls_di.c:3195`

## GameZRecoil/zModel/zModel_Display.cpp

- `0x476370` `VariantTag::TagsOverlap` -> `src/GameZRecoil/zModel/zModel_Display.cpp:1081`
- `0x476400` `VariantTag::CurrentAllowsId` -> `src/GameZRecoil/zModel/zModel_Display.cpp:1120`

## GameZRecoil/zNetwork.cpp

- `0x48b980` `zNetwork_GetLocalPlayerColorIndex` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:194`
- `0x48b9a0` `zNetwork_GetPlayerColorIndexByKey` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:207`

## GameZRecoil/zNetwork/znet_dplay.cpp

- `0x489f90` `zNetwork::SetFatalDisconnectCallback` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2255`
- `0x48a0d0` `zNetwork_DPlay::RefreshServiceProviderList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:863`
- `0x48a130` `zNetworkDPlay::RefreshAndGetServiceProviderList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:896`
- `0x48a520` `zNetworkDPlay::OpenSelectedSessionAndReadStatusFields` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1656`
- `0x48a9c0` `zNetwork_DPlay::CreateLocalPlayerRecordAndRegister` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1451`
- `0x48acf0` `zNetwork_DPlay_SendUnreliable` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:334`
- `0x48ad30` `zNetwork_DPlay_SendReliable` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:361`
- `0x48ad70` `zNetwork_DPlay_SendExUnreliableTracked` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:388`
- `0x48ae10` `zNetwork_DPlay_SendExReliable` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:433`
- `0x48b3a0` `zNetworkDPlay::EnumConnectionsCallback_AddServiceProviderInfo` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:820`
- `0x48bbe0` `zNetworkDPlay::SelectTcpIpProviderAndEnumSessions` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1726`
- `0x48be10` `zNetworkDPlay::CreateLobby3AInterface` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1795`
- `0x48be70` `zNetworkDPlay::EnumSessionsForCurrentApp` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1827`
- `0x48c060` `zNetwork_SendPacketUnreliable` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:465`
- `0x48c080` `zNetwork_SendPacketReliable` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:487`

## GameZRecoil/zNetwork/zNetwork.cpp

- `0x489d00` `zNetwork::InitSessionRuntime` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2266`
- `0x48b9d0` `zNetwork_GetPlayerRecordCount` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:230`
- `0x48bab0` `zNetwork_ExtractStatusFieldsFromSessionDesc` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:239`
- `0x48bb20` `zNetwork_ApplyStatusFieldsToSessionDesc` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:265`
- `0x48bf40` `zNetwork::DeleteAllDispatchHandlers` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1706`
- `0x48c0a0` `zNetwork::RegisterPacketHandler` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2055`

## GameZRecoil/zOptions/zopt.cpp

- `0x408120` `zOpt::SetPlayerName` -> `src/GameZRecoil/zGame/zGame.cpp:1622`
- `0x408190` `zOpt::GetPlayerName` -> `src/GameZRecoil/zGame/zGame.cpp:2161`
- `0x408650` `zOpt::GetDisplaySection` -> `src/GameZRecoil/zGame/zGame.cpp:1738`
- `0x408690` `zOpt::GetDisplaySectionBitsPerPixel` -> `src/GameZRecoil/zGame/zGame.cpp:1747`
- `0x4086a0` `zOpt::GetVideoStrideValue` -> `src/GameZRecoil/zGame/zGame.cpp:1756`
- `0x4086c0` `zOpt::GetWindowSection` -> `src/GameZRecoil/zGame/zGame.cpp:1765`
- `0x4086d0` `zOpt::GetWindowSectionHeight` -> `src/GameZRecoil/zGame/zGame.cpp:1774`

## GameZRecoil/zReader/zreader.cpp

- `0x48cda0` `zReader_AllocateNode` -> `src/GameZRecoil/zReader/zreader_load.cpp:1354`
- `0x48cdc0` `zReader::LoadNodeFromPath` -> `src/GameZRecoil/zReader/zreader_load.cpp:1637`
- `0x48ce40` `zReader::FreeLoadedTree` -> `src/GameZRecoil/zReader/zreader_load.cpp:1674`
- `0x48ce60` `zReader_FreeNodeRecursive` -> `src/GameZRecoil/zReader/zreader_load.cpp:1501`
- `0x48cec0` `zReader_FindChildRecursive` -> `src/GameZRecoil/zReader/zreader_lookup.cpp:7`
- `0x48cf70` `zReader_GetNamedNode` -> `src/GameZRecoil/zReader/zreader_lookup.cpp:51`
- `0x48d080` `zReader_ReadNode` -> `src/GameZRecoil/zReader/zreader_load.cpp:1408`
- `0x48d1c0` `zReader_OpenFileFromMountedArchives` -> `src/GameZRecoil/zReader/zreader_load.cpp:1526`
- `0x4a6110` `zReader_ReadString` -> `src/GameZRecoil/zReader/zreader_load.cpp:1367`

## GameZRecoil/zRndr/zRndr_Draw.cpp

- `0x4903e0` `zRndr::SetVideoStrideMirrors` -> `src/GameZRecoil/zRndr/zRndr.cpp:1883`

## GameZRecoil/zSound/zsnd_play.cpp

- `0x4a0380` `zSndPlayHandle::PlayWithDelta_A3D` -> `src/GameZRecoil/zSound/zsnd_play.cpp:807`
- `0x4a0400` `zSndPlayHandle::PlayWithDelta_DirectSound` -> `src/GameZRecoil/zSound/zsnd_play.cpp:843`
- `0x4a0490` `zSndPlayHandle::PlayWithDelta_BackendDispatch` -> `src/GameZRecoil/zSound/zsnd_play.cpp:892`
- `0x4a0590` `zSndPlayHandleSnapshot::RestoreAllWithGlobalVolumeDelta` -> `src/GameZRecoil/zSound/zsnd_play.cpp:928`

## GameZRecoil/zSound/zSound.cpp

- `0x4a3620` `zSndSample::GetPlayCursorBytes` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1158`

## GameZRecoil/zSys/zsys_cpu.cpp

- `0x4b3050` `zSys::CheckCpuSignatureMask` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:59`
- `0x4b33f0` `zSys::HasCpuidSupport` -> `src/GameZRecoil/zSys/zSys.cpp:180`
- `0x4b33f0` `zSys::HasCpuidSupport` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:9`
- `0x4b3480` `zSys::ReadCpuidFeatureFlags` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:87`
- `0x4b3510` `zSys::ProbeDivZeroFlagBehavior` -> `src/GameZRecoil/zSys/zSys.cpp:346`
- `0x4b3510` `zSys::ProbeDivZeroFlagBehavior` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:294`
- `0x4b3550` `zSys::DetectIs8086ByEflagsHiBits` -> `src/GameZRecoil/zSys/zSys.cpp:354`
- `0x4b3550` `zSys::DetectIs8086ByEflagsHiBits` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:320`
- `0x4b35a0` `zSys::DetectIs80286ByEflagsHiBits` -> `src/GameZRecoil/zSys/zSys.cpp:362`
- `0x4b35a0` `zSys::DetectIs80286ByEflagsHiBits` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:349`
- `0x4b35f0` `zSys::DetectIs80386ByAcFlag` -> `src/GameZRecoil/zSys/zSys.cpp:370`
- `0x4b35f0` `zSys::DetectIs80386ByAcFlag` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:377`
- `0x4b3640` `zSys::ReadCpuidVendorAndFamily` -> `src/GameZRecoil/zSys/zSys.cpp:188`
- `0x4b3640` `zSys::ReadCpuidVendorAndFamily` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:151`
- `0x4b3b20` `zSys::ReadTsc64` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:224`
- `0x4b3ca0` `zSys::Sub64` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:246`

## GameZRecoil/zUtil/zbd_save.c

- `0x4c0080` `zUtil_ZBD::OpenTempWriteStream` -> `src/GameZRecoil/zUtil/zZbd.cpp:581`
- `0x4c00a0` `zUtil_ZBD::FlushTempWriteStreamToSectionRecord` -> `src/GameZRecoil/zUtil/zZbd.cpp:608`
- `0x4c00c0` `zUtil_ZBD::OpenTempReadStream` -> `src/GameZRecoil/zUtil/zZbd.cpp:591`
- `0x4c00e0` `zUtil_ZBD::CloseTempReadStream` -> `src/GameZRecoil/zUtil/zZbd.cpp:625`

## GameZRecoil/zUtil/zUtil.cpp

- `0x42ecb0` `zUtil::SetMissionZrdrPathsAndMountZbd` -> `src/GameZRecoil/zReader/zreader_load.cpp:892`

## GameZRecoil/zUtil/zUtil_ZBD.cpp

- `0x4c0260` `zZbdSectionHandler::CompareSortOrderLessThan` -> `src/GameZRecoil/zUtil/zZbd.cpp:82`
- `0x4c0370` `zZbdManager::LoadEntries` -> `src/GameZRecoil/zUtil/zZbd.cpp:250`
- `0x4c07d0` `zZbdManager::SortSectionHandlers` -> `src/GameZRecoil/zUtil/zZbd.cpp:503`

## GameZRecoil/zUtil/zUtil_ZBD.cpp; MSVC 5.0 STL list support

- `0x4c0b60` `zZbdSectionHandlerList::Front` -> `src/GameZRecoil/zUtil/zZbd.cpp:101`
- `0x4c0b70` `zZbdSectionHandlerList::Constructor` -> `src/GameZRecoil/zUtil/zZbd.cpp:91`
- `0x4c0ba0` `zZbdSectionHandlerList::Swap` -> `src/GameZRecoil/zUtil/zZbd.cpp:109`
- `0x4c0bd0` `zZbdSectionHandlerList::Merge` -> `src/GameZRecoil/zUtil/zZbd.cpp:146`
- `0x4c0ce0` `zZbdSectionHandlerList::SpliceThreeNodes` -> `src/GameZRecoil/zUtil/zZbd.cpp:123`

## GameZRecoil/zUtil/zutl_zar.cpp

- `0x4a6270` `zIndexArchive::OpenCreateWrite` -> `src/GameZRecoil/zReader/zreader_load.cpp:1004`
- `0x4c0030` `zUtil::ZBD_LoadEntriesGlobal` -> `src/GameZRecoil/zUtil/zZbd.cpp:12`
- `0x4c0050` `zUtil::ZAR_LoadFileGlobal` -> `src/GameZRecoil/zUtil/zZbd.cpp:25`
- `0x4c0070` `zUtil::ZAR_RequestStopGlobal` -> `src/GameZRecoil/zUtil/zZbd.cpp:38`
- `0x4c0620` `zZbdManager::RequestStop` -> `src/GameZRecoil/zUtil/zZbd.cpp:365`

## GameZRecoil/zUtil/zutl_zrdr.cpp

- `0x4a5c50` `zUtil::ZRDR_GetFileSize` -> `src/GameZRecoil/zReader/zreader_load.cpp:481`
- `0x4a5f90` `zUtil_ZRDR::InitWildcardPath` -> `src/GameZRecoil/zReader/zreader_load.cpp:597`
- `0x4a6070` `zUtil_ZRDR::NextWildcardPath` -> `src/GameZRecoil/zReader/zreader_load.cpp:633`

## GameZRecoil/zUtil/zzbd.c

- `0x4c0700` `zZbdManager::FlushTempStreamToSectionRecord` -> `src/GameZRecoil/zUtil/zZbd.cpp:394`
- `0x4c0780` `zZbdManager::CreateTempReadStreamFromBuffer` -> `src/GameZRecoil/zUtil/zZbd.cpp:430`
- `0x4c07c0` `zZbdManager::RemoveTempFiles` -> `src/GameZRecoil/zUtil/zZbd.cpp:448`

## GameZRecoil/zVideo/zVideo.cpp

- `0x437ef0` `zVideo::HandleSoftwareModeHotkeyCommand` -> `src/GameZRecoil/zVideo/zVideo.cpp:2471`
- `0x44d600` `zVideo_sw::RenderFrame` -> `src/GameZRecoil/zVideo/zVideo.cpp:720`
- `0x46d810` `zImage::TexDir_FindOrAppendByPath` -> `src/GameZRecoil/zImage/zimg_texture.cpp:339`
- `0x46de50` `zImage::TexDir_LoadPendingEntries` -> `src/GameZRecoil/zImage/zimg_texture.cpp:196`
- `0x46df50` `zVid_TexturePack_EnsureBuiltinTexturePacksLoaded` -> `src/GameZRecoil/zVideo/zVideo.cpp:6117`
- `0x479ce0` `zVideo_SetActiveViewContext` -> `src/GameZRecoil/zVideo/zVideo.cpp:606`
- `0x47a0c0` `zVideo_UpdateProjectionStateFromCameraData` -> `src/GameZRecoil/zVideo/zVideo.cpp:883`
- `0x48ea20` `zVideo_FxSurface::ApplyBlueTintRect` -> `src/GameZRecoil/zVideo/zVideo.cpp:4591`
- `0x48eb80` `zVideo_FxSurface::ApplyGreenMaskRect` -> `src/GameZRecoil/zVideo/zVideo.cpp:4655`
- `0x48ec90` `zVideo_FxSurface::DrawColoredLinesBatch` -> `src/GameZRecoil/zVideo/zVideo.cpp:4927`
- `0x48ed60` `zVideo_FxSurface::DrawAlphaBlendedLine` -> `src/GameZRecoil/zVideo/zVideo.cpp:4713`
- `0x4a6770` `zVideo::RunPostprocessOnSwBuffer` -> `src/GameZRecoil/zVideo/zVideo.cpp:3839`
- `0x4a6900` `zVideo::PresentOrAdjustSurfacesIfEnabled` -> `src/GameZRecoil/zVideo/zVideo.cpp:3899`
- `0x4a77a0` `zVideo::BindRendererDispatch` -> `src/GameZRecoil/zVideo/zVideo.cpp:3924`
- `0x4bdc00` `zVideoFxPass3Slot::SetRectAndPayload` -> `src/GameZRecoil/zVideo/zVideo.cpp:1975`
- `0x4bed30` `zVideoFxPass3Config::UpdateLocal` -> `src/GameZRecoil/zVideo/zVideo.cpp:3710`
- `0x4bee00` `zVideoFxPass3Config::SetInputRectByIndex` -> `src/GameZRecoil/zVideo/zVideo.cpp:2080`
- `0x4bef40` `zVideo::FxPass3_SetInputRectByIndex` -> `src/GameZRecoil/zVideo/zVideo.cpp:3816`
- `0x4bef70` `zVideo::FxPass3_UpdateLocal` -> `src/GameZRecoil/zVideo/zVideo.cpp:3828`
- `0x4c7fd0` `zVideo::LoadPaletteFileAndApplyBrightness` -> `src/GameZRecoil/zVideo/zVideo.cpp:2639`
- `0x4c8070` `zVideo::ApplyBrightnessToPaletteEntries` -> `src/GameZRecoil/zVideo/zVideo.cpp:2674`

## GameZRecoil/zWeapon.cpp

- `0x43c190` `Player::ProcessAltGunDispatchRequest` -> `src/Battlesport/player.cpp:16338`
- `0x43c2d0` `Player::UpdateContinuousAltGunFireController` -> `src/Battlesport/player.cpp:16159`
- `0x43c430` `Player::AltGunLaunchProjectile` -> `src/Battlesport/player.cpp:16233`
- `0x43c550` `Player::AltGunFireSimpleProjectile` -> `src/Battlesport/player.cpp:16301`

## GameZRecoil/zWeapon/OptCatalog.c

- `0x4b26f0` `OptCatalog::InvokeDamageFeedbackAndHitCallback` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2311`

## GameZRecoil/zWeapon/zWeapon.cpp

- `0x4ae380` `OptCatalog::BlendDirectionTowardTarget` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1126`
- `0x4ae3c0` `OptCatalog::FindEntryByName` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1141`
- `0x4ae450` `OptCatalog::FindEntryById` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1157`
- `0x4ae4a0` `OptCatalog::SetPendingSpawnTargetOverrides` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1276`
- `0x4ae4b0` `OptCatalog::AllocOrReuseAttachNodeChildClone` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1515`
- `0x4ae4e0` `OptCatalog::RecycleAttachNodeClone` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1544`
- `0x4ae520` `OptCatalog::ClearRuntimeInstanceAsyncFxHandleCallback` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1534`
- `0x4ae530` `OptCatalog::AllocOrReuseAttachNodeClone` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1567`
- `0x4ae590` `OptCatalog::RecycleRuntimeInstanceStorage` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1968`
- `0x4ae660` `OptCatalog::AllocRuntimeInstance` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1600`
- `0x4aeaa0` `OptCatalog::SpawnRuntimeInstanceAt` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1817`
- `0x4aeb50` `OptCatalog::RecycleRuntimeInstance` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1856`
- `0x4aebc0` `OptCatalog::ClearRuntimeInstances` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1899`
- `0x4aebf0` `OptCatalog::RemoveRuntimeInstance` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1914`
- `0x4aed00` `OptCatalog::ProcessRuntimeInstance` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2773`
- `0x4aee40` `OptCatalog::ActivateTrailRuntimeState` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2159`
- `0x4af060` `OptCatalog::ProcessRuntimeInstances` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2840`
- `0x4b0530` `OptCatalog::ComputeAimPitchForTarget` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2245`
- `0x4b0600` `OptCatalog::PlayTriggerInactiveWarning` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2294`
- `0x4b0620` `OptCatalog::PlayWeaponInactiveWarning` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2300`
- `0x4b0660` `OptCatalog::EmitQSandImpactEvent` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2468`
- `0x4b0710` `OptCatalog::EmitCraterImpactEvent` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2433`
- `0x4b07d0` `OptCatalog::HandleImpactEvent` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2548`
- `0x4b0980` `OptCatalog::HandleImpactEventFromRuntimeState` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2659`
- `0x4b09d0` `OptCatalog::BuildImpactHitList` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2681`
- `0x4b0a50` `OptCatalog::HandleImpactFromRuntimeProbe` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2724`
- `0x4b0ba0` `OptCatalog::CanSpawnThroughRay` -> `src/GameZRecoil/zWeapon/OptCatalog.c:3085`
- `0x4b0ca0` `OptCatalog::ReflectAndSortImpactTraceList` -> `src/GameZRecoil/zWeapon/OptCatalog.c:3135`
- `0x4b0e20` `OptCatalog::ComputeTrailImpactResponse` -> `src/GameZRecoil/zWeapon/OptCatalog.c:3196`
- `0x4b0f70` `OptCatalog::UpdateTrailSegmentVisual` -> `src/GameZRecoil/zWeapon/OptCatalog.c:3274`
- `0x4b0fd0` `OptCatalog::PlayImpactSound` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2502`
- `0x4b1030` `OptCatalog::PlayBounceSound` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2525`
- `0x4b1180` `OptCatalog::Shutdown` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2090`
- `0x4b1ec0` `OptCatalog::CreateTrailRuntimeState` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1189`
- `0x4b1fa0` `OptCatalog::LoadFxSpecFromReaderNode` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1391`
- `0x4b2130` `OptCatalog::CreateTrailSegmentNodeFromTemplate` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1169`
- `0x4b28e0` `OptCatalog::SetDamageContext` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2392`
- `0x4b2900` `DamageFeedback::SetIntensityScalar` -> `src/GameZRecoil/zWeapon/OptCatalog.c:3311`
- `0x4b2910` `OptCatalog::GetCapturedHitSourcePtr` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2427`
- `0x4b2920` `HitContext::GetCurrentOwnerOrCtx` -> `src/GameZRecoil/zWeapon/OptCatalog.c:3319`
- `0x4b2930` `OptCatalog_MineIterator::Begin` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1255`
- `0x4b2940` `OptCatalog_MineIterator::Next` -> `src/GameZRecoil/zWeapon/OptCatalog.c:1264`

## GameZRecoil/zWeapon/zwep_init.c

- `0x43ca20` `zWeapon_OptCatalog::LoadKillVerbString` -> `src/GameZRecoil/zWeapon/OptCatalog.c:551`
- `0x4b1190` `zWeapon::LoadOptCatalogFromPath` -> `src/GameZRecoil/zWeapon/OptCatalog.c:587`

## GameZRecoil/zZbd/zzbd.c

- `0x4c0400` `zZbdManager::LoadZarFile` -> `src/GameZRecoil/zUtil/zZbd.cpp:274`
- `0x4c06a0` `zZbdSectionHandler::InvokePreLoad` -> `src/GameZRecoil/zUtil/zZbd.cpp:458`
- `0x4c06c0` `zZbdSectionHandler::InvokeDataReady` -> `src/GameZRecoil/zUtil/zZbd.cpp:477`

## HudSensorMapNode.cpp

- `0x415ab0` `HudSensorMapNode::Init` -> `src/Battlesport/HudSensorTracker.cpp:660`
- `0x415ac0` `HudSensorMapNode::FreePointArray` -> `src/Battlesport/HudSensorTracker.cpp:666`
- `0x415ae0` `HudSensorMapNode::SetEnabled` -> `src/Battlesport/HudSensorTracker.cpp:673`
- `0x415b10` `HudSensorMapNode::SelectPoint` -> `src/Battlesport/HudSensorTracker.cpp:687`
- `0x415b40` `HudSensorMapNode::InitDefaults` -> `src/Battlesport/HudSensorTracker.cpp:700`
- `0x415b70` `HudSensorMapNode::SetColorRgb` -> `src/Battlesport/HudSensorTracker.cpp:716`
- `0x415bd0` `HudSensorMapNode::LoadFromStream` -> `src/Battlesport/HudSensorTracker.cpp:743`
- `0x415c90` `HudSensorMapNode::UpdateCachedBounds` -> `src/Battlesport/HudSensorTracker.cpp:791`

## HudSensorTracker.cpp

- `0x4172c0` `HudSensorTracker::SetObjectiveMarkerEnabledAndColor` -> `src/Battlesport/HudSensorTracker.cpp:2732`
- `0x418c30` `HudSensorTracker::FindAndHighlightFirstIncompleteObjective` -> `src/Battlesport/HudSensorTracker.cpp:2773`

## HudUiBackground.cpp

- `0x4ba350` `HudUiBackground::FreeLoadedTreeRoots` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7762`
- `0x4bffb0` `HudUiPrimitiveBindTarget::SetSegmentEndpoints` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5332`

## HudUiFillBitmap.cpp

- `0x4b84d0` `HudUiFillBitmap::~HudUiFillBitmap` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10074`

## src/Battlesport/gamenet.cpp

- `0x432d60` `GameNet::UpdateRemotePlayerHudWidgetScreenPos` -> `src/Battlesport/GameNet.cpp:2692`

## src/Battlesport/pickup.cpp

- `0x41cf30` `Pickup::ResolveOwnerFromBvolHit` -> `src/Battlesport/pickup.cpp:1788`

## src/Battlesport/player.cpp

- `0x401060` `Player::TickAiMode2TopLevel` -> `src/Battlesport/player.cpp:5603`
- `0x401180` `Player::TickAiMode2PathFollow` -> `src/Battlesport/player.cpp:5677`
- `0x401420` `Player::AiMode2ForwardProbeRequiresAutoTurn` -> `src/Battlesport/player.cpp:5459`
- `0x4016a0` `Player::AiChooseNextPathBranchIndex` -> `src/Battlesport/player.cpp:5504`
- `0x401710` `Player::TickAiMode2SteeringSubstate` -> `src/Battlesport/player.cpp:5920`
- `0x401970` `Player::UpdateAiMode2MoveAndTurnTowardTarget` -> `src/Battlesport/player.cpp:6035`
- `0x401a40` `Player::TickAiMode2OffsetTargetSteering` -> `src/Battlesport/player.cpp:6417`
- `0x401ab0` `Player::TickAiMode2DynamicOffsetTargetSteering` -> `src/Battlesport/player.cpp:6453`
- `0x401b20` `Player::AiTryEnterMode2AttackPursuitIfLineOfSight` -> `src/Battlesport/player.cpp:5832`
- `0x401c00` `Player::AiAlertAttackBuddies` -> `src/Battlesport/player.cpp:5811`
- `0x401c60` `Player::AiEnterMode2SteeringPursuit` -> `src/Battlesport/player.cpp:5777`
- `0x401d50` `Player::HasLineOfSightFromLocalPlayerFxOffset` -> `src/Battlesport/player.cpp:13462`
- `0x402080` `Player::AiRestoreSavedTopLevelState` -> `src/Battlesport/player.cpp:6489`
- `0x402090` `Player::UpdateAiMode2TurnTowardPlayerNoThrottle` -> `src/Battlesport/player.cpp:6064`
- `0x402170` `Player::UpdateAiMode2TurnInPlaceTowardPlayer` -> `src/Battlesport/player.cpp:6096`
- `0x402250` `Player::TickAiMode2AltGunAttackWindow` -> `src/Battlesport/player.cpp:6128`
- `0x4024a0` `Player::SolveAltGunLeadTargetPoint` -> `src/Battlesport/player.cpp:6220`
- `0x4026d0` `Player::UpdateAiMode2MoveAndTurnTowardOffsetTarget` -> `src/Battlesport/player.cpp:6275`
- `0x4028c0` `Player::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget` -> `src/Battlesport/player.cpp:6337`
- `0x402b70` `Player::TickAiMode2TimedPathSteering` -> `src/Battlesport/player.cpp:6600`
- `0x402be0` `Player::AiSteerTowardPathNodeForward` -> `src/Battlesport/player.cpp:6498`
- `0x402d60` `Player::AiSteerTowardPathNodeReverse` -> `src/Battlesport/player.cpp:6547`
- `0x402f10` `Player::AiFinalizeMode2State1ForAllPlayers` -> `src/Battlesport/player.cpp:9231`
- `0x403830` `Player::AiDiscardNegativeBranchPathNodes` -> `src/Battlesport/player.cpp:5443`
- `0x41bab0` `Player::UpdateGunDispatchRequestsFromTriggerLatches` -> `src/Battlesport/player.cpp:5414`
- `0x41bb30` `Player::DestroyedStateRespawnCallback` -> `src/Battlesport/player.cpp:7997`
- `0x41bd10` `Player::ClearRespawnTransitionFlagCallback` -> `src/Battlesport/player.cpp:7989`
- `0x41eaf0` `Player::InitAndRegisterUnderwaterFxPass3UiSingleton` -> `src/Battlesport/player.cpp:2097`
- `0x41eb00` `Player::InitUnderwaterFxPass3UiSingleton` -> `src/Battlesport/player.cpp:2104`
- `0x41eb10` `Player::RegisterUnderwaterFxPass3UiOnExit` -> `src/Battlesport/player.cpp:2110`
- `0x41eb20` `Player::ResetUnderwaterFxPass3UiSingleton` -> `src/Battlesport/player.cpp:2116`
- `0x41eb50` `Player::InitAndRegisterProjectileCameraFxPass3UiSingleton` -> `src/Battlesport/player.cpp:2121`
- `0x41eb60` `Player::InitProjectileCameraFxPass3UiSingleton` -> `src/Battlesport/player.cpp:2128`
- `0x41eb70` `Player::RegisterProjectileCameraFxPass3UiCleanup` -> `src/Battlesport/player.cpp:2134`
- `0x41eb80` `Player::ResetProjectileCameraFxPass3UiSingleton` -> `src/Battlesport/player.cpp:2140`
- `0x41ec00` `Player::InitSaveStateList` -> `src/Battlesport/player.cpp:2145`
- `0x41ec30` `Player::InitAndRegisterTopMsgPanel1` -> `src/Battlesport/player.cpp:2154`
- `0x41ec40` `Player_TopMsgPanel1::Constructor` -> `src/Battlesport/player.cpp:2005`
- `0x41ec60` `Player::RegisterTopMsgPanel1OnExit` -> `src/Battlesport/player.cpp:2161`
- `0x41ec70` `Player_TopMsgPanel1::Destructor` -> `src/Battlesport/player.cpp:2015`
- `0x421ea0` `Player::CreateFromNamesAtPoseGetState` -> `src/Battlesport/player.cpp:2433`
- `0x423460` `Player::ProcessPendingContactQueues` -> `src/Battlesport/player.cpp:8897`
- `0x423530` `Player::ClearPendingContactQueues` -> `src/Battlesport/player.cpp:6624`
- `0x4236b0` `Player::BuildPendingContactQueues` -> `src/Battlesport/player.cpp:8672`
- `0x423b10` `Player::CollectPendingContactsForSegments` -> `src/Battlesport/player.cpp:7218`
- `0x423c20` `Player::ClassifyPendingContactsForSegment` -> `src/Battlesport/player.cpp:7140`
- `0x423fc0` `Player::SelectAndResolvePreferredPendingCollisionContact` -> `src/Battlesport/player.cpp:7448`
- `0x424010` `PlayerPendingContact::SelectPreferred` -> `src/Battlesport/player.cpp:5143`
- `0x424110` `Player::ResolvePendingWorldCollisionContact` -> `src/Battlesport/player.cpp:7526`
- `0x424150` `PlayerPickupContact::PassesCollectionTest` -> `src/Battlesport/player.cpp:7302`
- `0x424210` `Player::ProcessPendingPickupContacts` -> `src/Battlesport/player.cpp:7268`
- `0x424270` `Player::ResolvePendingCollisionContact` -> `src/Battlesport/player.cpp:7577`
- `0x4248e0` `Player::PreparePendingWorldCollisionResponse` -> `src/Battlesport/player.cpp:7468`
- `0x424ac0` `Player::ResolvePendingPlayerCollisionContact` -> `src/Battlesport/player.cpp:7746`
- `0x424bf0` `Player::Vec3_FastNormalize` -> `src/Battlesport/player.cpp:11999`
- `0x424c90` `Player::ConstrainToUnitDistanceFrom` -> `src/Battlesport/player.cpp:12031`
- `0x424d00` `Player::ProcessTransferContactQueue` -> `src/Battlesport/player.cpp:7801`
- `0x424ed0` `Player::TryResolvePendingCollisionProbeSweep` -> `src/Battlesport/player.cpp:7412`
- `0x425060` `HudSensorTracker::ParseCheckpointNumberFromNode` -> `src/Battlesport/player.cpp:4995`
- `0x4251f0` `Player::CollectPendingCollisionContactsForQuadProbe` -> `src/Battlesport/player.cpp:7357`
- `0x425770` `Player::ApplyPendingCollisionProbeVelocity` -> `src/Battlesport/player.cpp:8950`
- `0x426350` `Player::FloatSign` -> `src/Battlesport/player.cpp:11914`
- `0x426770` `Player::UpdateMasterTypeTrack` -> `src/Battlesport/player.cpp:12527`
- `0x428c20` `Player::UpdateSubVerticalDamping` -> `src/Battlesport/player.cpp:12200`
- `0x429430` `Player::ApplyPitchRollVelocityImpulseFromDirection` -> `src/Battlesport/player.cpp:7549`
- `0x4294d0` `Player::RebuildSteerBasisFromMotionBasis` -> `src/Battlesport/player.cpp:10398`
- `0x429560` `Player::RebuildSteerBasisFromMotionAxes` -> `src/Battlesport/player.cpp:10525`
- `0x429870` `Player::UpdateYawVelocityFromSteerInput` -> `src/Battlesport/player.cpp:12244`
- `0x429b40` `Player::UpdateBankAndTurnDynamics` -> `src/Battlesport/player.cpp:11951`
- `0x429d30` `Player::ComputeTurnSlipDelta` -> `src/Battlesport/player.cpp:12047`
- `0x429ed0` `Player::StartSlipSfx` -> `src/Battlesport/player.cpp:11930`
- `0x429ef0` `Player::StopSlipSfx` -> `src/Battlesport/player.cpp:11942`
- `0x42b970` `Player::RebuildMotionBasisFromSteerBasis` -> `src/Battlesport/player.cpp:10493`
- `0x4385f0` `Player::StartModalLoopSfxHandle` -> `src/Battlesport/player.cpp:1761`
- `0x43a4f0` `Player::UpdateGunAndTurretAimNodes` -> `src/Battlesport/player.cpp:15744`
- `0x43a600` `Player::UpdateAltGunAimDirection` -> `src/Battlesport/player.cpp:15798`
- `0x43b1b0` `Player::BuildGunFireTransform` -> `src/Battlesport/player.cpp:15640`
- `0x43b3e0` `Player::UpdateAltGunAimBasisOrigin` -> `src/Battlesport/player.cpp:15703`
- `0x43b500` `Player::ApplyAimPitchToDirection` -> `src/Battlesport/player.cpp:13519`
- `0x43b730` `Player::RecordRecentHitFeedback` -> `src/Battlesport/player.cpp:7854`
- `0x43b790` `Player::UpdateTimedHitStatusFromHitSource` -> `src/Battlesport/player.cpp:8262`
- `0x43b800` `Player::ClearDestroyedRespawnEffectHandleCallback` -> `src/Battlesport/player.cpp:7882`
- `0x43b810` `Player::HitCallback_RecordNetContextAndTimedStatus` -> `src/Battlesport/player.cpp:8291`
- `0x43b870` `Player::HitCallback_RecordContextAndTimedStatus` -> `src/Battlesport/player.cpp:8432`
- `0x43bcc0` `Player::EnterDestroyedState` -> `src/Battlesport/player.cpp:8089`
- `0x43c010` `Player::ApplyDamageLocal` -> `src/Battlesport/player.cpp:8324`

## src/zin_ff.cpp

- `0x42fb50` `zInputDI::PlayCollisionImpactEffect` -> `src/GameZRecoil/zInput/zInput.cpp:1083`
- `0x42fc90` `zInputDI::PlayDamageHitEffect` -> `src/GameZRecoil/zInput/zInput.cpp:1111`

## Time.cpp

- `0x4a56d0` `Time::Tick` -> `src/GameZRecoil/Time/Time.cpp:43`

## unknown original source

- `0x402f60` `zMath::Vec3Normalize` -> `src/GameZRecoil/zMath/zMath.cpp:553`
- `0x4038a0` `HudUiBriefingObjectivePicture::DrawWithNoiseOverlay` -> `src/Battlesport/Briefing.cpp:415`
- `0x403c10` `HudUiBriefingLocatorPanel::HudUiBriefingLocatorPanel` -> `src/Battlesport/Briefing.cpp:439`
- `0x403c80` `HudUiCircle::DrawDirtyForwarder` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5390`
- `0x403c90` `HudUiBriefingLocatorPanel::BlitDirtyRect` -> `src/Battlesport/Briefing.cpp:458`
- `0x403cb0` `HudUiBriefingLocatorPanel::Update` -> `src/Battlesport/Briefing.cpp:484`
- `0x403e20` `HudUiCompositePanel::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5593`
- `0x404140` `zInput_WaitForAnyKeyPressWithTimeoutMs` -> `src/GameZRecoil/zInput/zInput.cpp:3310`
- `0x404400` `Briefing::BuildObjectiveActionsFromIndex` -> `src/Battlesport/Briefing.cpp:1004`
- `0x4045b0` `Briefing_ActionQueue::AddHideElement` -> `src/Battlesport/Briefing.cpp:658`
- `0x404620` `BriefingAction_HideElement::Tick` -> `src/Battlesport/Briefing.cpp:543`
- `0x404640` `Briefing_ActionQueue::AddShowElement` -> `src/Battlesport/Briefing.cpp:673`
- `0x4046b0` `BriefingAction_ShowElement::Tick` -> `src/Battlesport/Briefing.cpp:554`
- `0x4046d0` `Briefing_ActionQueue::AddFadeInElement` -> `src/Battlesport/Briefing.cpp:688`
- `0x404740` `BriefingAction_FadeInElement::Tick` -> `src/Battlesport/Briefing.cpp:566`
- `0x404780` `Briefing_ActionQueue::AddSetPanelText` -> `src/Battlesport/Briefing.cpp:704`
- `0x404850` `BriefingAction_SetPanelText::Tick` -> `src/Battlesport/Briefing.cpp:582`
- `0x4048a0` `Briefing_ActionQueue::AddSetWidgetImageTimed` -> `src/Battlesport/Briefing.cpp:726`
- `0x404960` `BriefingAction_SetWidgetImageTimed::Tick` -> `src/Battlesport/Briefing.cpp:596`
- `0x4049d0` `Briefing_ActionQueue::AddPlaySampleByName` -> `src/Battlesport/Briefing.cpp:745`
- `0x404aa0` `BriefingAction_PlaySample::Tick` -> `src/Battlesport/Briefing.cpp:614`
- `0x404b30` `Briefing::SampleEventCallback` -> `src/Battlesport/Briefing.cpp:786`
- `0x404b40` `Briefing_ActionQueue::AddDelayUntilProgress` -> `src/Battlesport/Briefing.cpp:770`
- `0x404bb0` `BriefingAction_DelayUntilProgress::Tick` -> `src/Battlesport/Briefing.cpp:648`
- `0x404c50` `Briefing::SetProgressAndSleep` -> `src/Battlesport/Briefing.cpp:955`
- `0x404c80` `Briefing::BuildObjectiveActionsGlobal` -> `src/Battlesport/Briefing.cpp:796`
- `0x404ca0` `HudUiElement::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4962`
- `0x404cb0` `HudUiElement::DrawBase` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4968`
- `0x404cd0` `HudUiElement::SetPos` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4993`
- `0x404cf0` `HudUiElement::SetX` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5006`
- `0x404d00` `HudUiElement::SetY` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5017`
- `0x404d20` `HudUiElement::SetVisible` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5028`
- `0x404d50` `HudUiElement::GetX` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5184`
- `0x404d60` `HudUiElement::GetY` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5192`
- `0x404d70` `HudUiElement::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4948`
- `0x404d90` `HudUiWidget::GetCenterX` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13306`
- `0x404dd0` `HudUiWidget::GetCenterY` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13319`
- `0x404e10` `HudUiWidget::RebuildBltRectFromImage` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13350`
- `0x404e60` `HudUiCircle::HitTest` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5398`
- `0x404e80` `zError::ReportOldNoOp` -> `src/GameZRecoil/zError/zerr_old.c:25`
- `0x406a00` `zStr::ContainsCaseInsensitive` -> `src/Battlesport/zStr.cpp:11`
- `0x407130` `zStub::ReturnOneNoArgs` -> `src/GameZRecoil/zClass/cls_stubs.cpp:4`
- `0x407140` `zStub::ReturnZeroNoArgs` -> `src/GameZRecoil/zClass/cls_stubs.cpp:14`
- `0x407150` `zStub::NoOp1Arg` -> `src/GameZRecoil/zClass/cls_stubs.cpp:25`
- `0x407160` `zStub::ReturnOne2Args` -> `src/GameZRecoil/zClass/cls_stubs.cpp:36`
- `0x4071f0` `zOpt::ReadScalarValueAsInt` -> `src/GameZRecoil/zGame/zGame.cpp:1236`
- `0x407220` `zOpt::EvalIntCompareOp` -> `src/GameZRecoil/zGame/zGame.cpp:1346`
- `0x407470` `zOpt::EvaluateProfileMetricCondition` -> `src/GameZRecoil/zGame/zGame.cpp:1253`
- `0x407680` `zOpt::SelectProfileValueForSystem` -> `src/GameZRecoil/zGame/zGame.cpp:1314`
- `0x4076f0` `zGame::ReturnOnlyStub` -> `src/GameZRecoil/zGame/zGame.cpp:376`
- `0x407700` `zGame::Options_LoadGameOptions` -> `src/GameZRecoil/zGame/zGame.cpp:729`
- `0x407e00` `zGame::Options_SaveGameOptions` -> `src/GameZRecoil/zGame/zGame.cpp:676`
- `0x407e20` `zOpt::SetGameControlOptions` -> `src/GameZRecoil/zGame/zGame.cpp:1401`
- `0x407e30` `zOpt::SetThrottleMode` -> `src/GameZRecoil/zGame/zGame.cpp:1408`
- `0x407e50` `zOpt::GetThrottleMode` -> `src/GameZRecoil/zGame/zGame.cpp:1419`
- `0x407e60` `zOpt::SetSteeringMode` -> `src/GameZRecoil/zGame/zGame.cpp:1424`
- `0x407e80` `zOpt::GetSteeringMode` -> `src/GameZRecoil/zGame/zGame.cpp:1435`
- `0x407e90` `zOpt::SetCursorMode` -> `src/GameZRecoil/zGame/zGame.cpp:1440`
- `0x407eb0` `zOpt::GetCursorMode` -> `src/GameZRecoil/zGame/zGame.cpp:1451`
- `0x407ec0` `zOpt::SetCameraMode` -> `src/GameZRecoil/zGame/zGame.cpp:1456`
- `0x407ef0` `zOpt::GetCameraModePlayerState` -> `src/GameZRecoil/zGame/zGame.cpp:1469`
- `0x407f10` `zOpt::SetGameDifficultyMode` -> `src/GameZRecoil/zGame/zGame.cpp:1474`
- `0x407f20` `zOpt::GetGameDifficultyMode` -> `src/GameZRecoil/zGame/zGame.cpp:1481`
- `0x407f30` `zOpt::SetEffectsLevelForCurrentHwMode` -> `src/GameZRecoil/zGame/zGame.cpp:1487`
- `0x407f80` `zOpt::GetEffectsLevelForCurrentHwMode` -> `src/GameZRecoil/zGame/zGame.cpp:1505`
- `0x407fa0` `zOpt::SetObjectLODForCurrentHwMode` -> `src/GameZRecoil/zGame/zGame.cpp:1513`
- `0x408030` `zOpt::GetObjectLODForCurrentHwMode` -> `src/GameZRecoil/zGame/zGame.cpp:1540`
- `0x408050` `zOpt::SetMuteSoundOption` -> `src/GameZRecoil/zGame/zGame.cpp:1556`
- `0x408060` `zOpt::GetMuteSoundOption` -> `src/GameZRecoil/zGame/zGame.cpp:1548`
- `0x408070` `zOpt::SetSoundVolumeOption` -> `src/GameZRecoil/zGame/zGame.cpp:1567`
- `0x408090` `zOpt::GetSoundVolumeOption` -> `src/GameZRecoil/zGame/zGame.cpp:1578`
- `0x4080a0` `zSnd::SetAudioApiOption` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:154`
- `0x4080b0` `zSnd::GetAudioApiOption` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:162`
- `0x4080c0` `zOpt::SetSoundLODOption` -> `src/GameZRecoil/zGame/zGame.cpp:1586`
- `0x4080d0` `zOpt::GetSoundLODOption` -> `src/GameZRecoil/zGame/zGame.cpp:1596`
- `0x4080e0` `zOpt::SetTextureMemoryForCurrentHwMode` -> `src/GameZRecoil/zGame/zGame.cpp:1604`
- `0x408100` `zOpt::GetTextureMemoryForCurrentHwMode` -> `src/GameZRecoil/zGame/zGame.cpp:1614`
- `0x408210` `zSnd::SetCDAudioOption` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:168`
- `0x408220` `zSnd::GetCDAudioOption` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:178`
- `0x408240` `zOpt::SetNetworkModemEnabled` -> `src/GameZRecoil/zGame/zGame.cpp:1707`
- `0x408250` `zOpt::SetNetworkListenEnabled` -> `src/GameZRecoil/zGame/zGame.cpp:1714`
- `0x408260` `zOpt::GetNetworkEnabled` -> `src/GameZRecoil/zGame/zGame.cpp:1836`
- `0x408280` `zVid::SetAccelerationOption` -> `src/GameZRecoil/zVideo/zVideo.cpp:1512`
- `0x408290` `zVid::SetHwApiOption` -> `src/GameZRecoil/zVideo/zVideo.cpp:1529`
- `0x4082a0` `zOpt::SetFullscreenOption` -> `src/GameZRecoil/zGame/zGame.cpp:1782`
- `0x4082b0` `zOpt::SetHudVisibilityOption` -> `src/GameZRecoil/zGame/zGame.cpp:1794`
- `0x4082d0` `zOpt::SetHudTypeForCurrentHwMode` -> `src/GameZRecoil/zGame/zGame.cpp:1681`
- `0x408300` `zOpt::SetReplicateMode` -> `src/GameZRecoil/zGame/zGame.cpp:1816`
- `0x408310` `zVid::GetAccelerationOption` -> `src/GameZRecoil/zVideo/zVideo.cpp:1543`
- `0x408320` `zVid::GetHwApiOption` -> `src/GameZRecoil/zVideo/zVideo.cpp:1548`
- `0x408330` `zOpt::GetFullscreenOption` -> `src/GameZRecoil/zGame/zGame.cpp:1789`
- `0x408340` `zOpt::GetHudVisibilityOption` -> `src/GameZRecoil/zGame/zGame.cpp:1801`
- `0x408380` `zOpt::GetReplicateMode` -> `src/GameZRecoil/zGame/zGame.cpp:1830`
- `0x408390` `zInp::SetJoystickOption` -> `src/GameZRecoil/zInput/zInput.cpp:746`
- `0x4083a0` `zInp::SetJoystickAxesCountOption` -> `src/GameZRecoil/zInput/zInput.cpp:755`
- `0x4083b0` `zInp::SetJoystickButtonCountOption` -> `src/GameZRecoil/zInput/zInput.cpp:762`
- `0x4083c0` `zInp::GetJoystickOption` -> `src/GameZRecoil/zInput/zInput.cpp:769`
- `0x4083d0` `zOpt_ViewRectSection::SetPosition` -> `src/GameZRecoil/zGame/zGame.cpp:1844`
- `0x408400` `zOpt_ViewRectSection::SetSize` -> `src/GameZRecoil/zGame/zGame.cpp:1858`
- `0x408430` `zOpt::ViewRectSection_ClampPointToInclusiveBounds` -> `src/GameZRecoil/zGame/zGame.cpp:1872`
- `0x408480` `zOpt::CameraSection_SetActiveCamera` -> `src/GameZRecoil/zGame/zGame.cpp:2122`
- `0x4084e0` `zOpt_CameraSection_GetActiveCamera` -> `src/GameZRecoil/zGame/zGame.cpp:2151`
- `0x408500` `zOpt::RenderSection_SetSize` -> `src/GameZRecoil/zGame/zGame.cpp:1927`
- `0x408530` `zOpt::RenderSection_SetPosition` -> `src/GameZRecoil/zGame/zGame.cpp:1891`
- `0x408570` `zOpt::RenderSection_SetTargetWindow` -> `src/GameZRecoil/zGame/zGame.cpp:1956`
- `0x4085a0` `zOpt::GetRenderSection` -> `src/GameZRecoil/zGame/zGame.cpp:1976`
- `0x4085b0` `zOpt::DisplaySection_SetTargetDisplay` -> `src/GameZRecoil/zGame/zGame.cpp:2047`
- `0x4085e0` `zOpt::DisplaySection_SetPosition` -> `src/GameZRecoil/zGame/zGame.cpp:1982`
- `0x408620` `zOpt::DisplaySection_SetSize` -> `src/GameZRecoil/zGame/zGame.cpp:2018`
- `0x408660` `zOpt_DisplaySection_GetWidth` -> `src/GameZRecoil/zGame/zGame.cpp:2174`
- `0x408670` `zOpt_DisplaySection_GetHeight` -> `src/GameZRecoil/zGame/zGame.cpp:2179`
- `0x408680` `zOpt::DisplaySection_SetBitsPerPixel` -> `src/GameZRecoil/zGame/zGame.cpp:2068`
- `0x4086b0` `zVid::GetVideoModeIndexFromOptions` -> `src/GameZRecoil/zVideo/zVideo.cpp:1586`
- `0x4086e0` `zOpt::WindowSection_SetSize` -> `src/GameZRecoil/zGame/zGame.cpp:2103`
- `0x408700` `zOpt::WindowSection_SetPosition` -> `src/GameZRecoil/zGame/zGame.cpp:2083`
- `0x408720` `zVid::SetVideoModeIndex` -> `src/GameZRecoil/zVideo/zVideo.cpp:1592`
- `0x4089c0` `HudUiMgr::ScreenToWorld` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4514`
- `0x408a10` `zOpt::SetWolPasswordFlag` -> `src/GameZRecoil/zGame/zGame.cpp:1730`
- `0x408a20` `zOpt_GetWolPasswordFlagValue` -> `src/GameZRecoil/zGame/zGame.cpp:2169`
- `0x408f50` `RecoilStateDialogHost::OnWndActivate` -> `src/GameZRecoil/RecoilApp/RecoilStateDialogHost.cpp:4`
- `0x409010` `HudUiZrdWidgetEx17C::EnableChildAtIndex` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10563`
- `0x409160` `HudUiCreditsBackButton::OnActivate` -> `src/Battlesport/hud.cpp:1045`
- `0x409180` `HudUiCreditsQuitButton::OnActivate` -> `src/Battlesport/hud.cpp:1054`
- `0x4091c0` `HudUiCreditsPanel::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6541`
- `0x4091e0` `HudUiZrdScrollingText::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6145`
- `0x4092a0` `HudUiCreditsPanel::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6518`
- `0x409360` `HudUiZrdScrollingText::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6466`
- `0x409380` `HudUiCreditsPanel::UpdateFadeAndExit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6483`
- `0x409410` `HudUiZrdScrollingText::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6399`
- `0x409470` `HudUiZrdScrollingText::UpdateScrollPositions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6421`
- `0x409550` `HudUiZrdScrollingText::OnActivateResetOwnerFade` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6214`
- `0x409950` `RecoilStateCredits::StaticInitAndRegisterAtExit` -> `src/Battlesport/RecoilStateCredits.cpp:18`
- `0x409970` `RecoilStateCredits::StaticInit` -> `src/Battlesport/RecoilStateCredits.cpp:29`
- `0x409980` `RecoilStateCredits::RegisterAtExit` -> `src/Battlesport/RecoilStateCredits.cpp:39`
- `0x409990` `RecoilStateCredits::RecoilStateCredits` -> `src/Battlesport/RecoilStateCredits.cpp:10`
- `0x4099a0` `RecoilStateCredits::OnWndActivate` -> `src/Battlesport/RecoilStateCredits.cpp:49`
- `0x4099f0` `RecoilStateCredits::~RecoilStateCredits` -> `src/Battlesport/RecoilStateCredits.cpp:110`
- `0x409a60` `RecoilStateCredits::OnTryBecomeCurrent` -> `src/Battlesport/RecoilStateCredits.cpp:69`
- `0x409ad0` `RecoilStateCredits::OnDeactivate` -> `src/Battlesport/RecoilStateCredits.cpp:87`
- `0x409b00` `RecoilStateCredits::QueuePush` -> `src/Battlesport/RecoilStateCredits.cpp:129`
- `0x409b20` `HudUiPanelSpan::DestroyAndFree` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6932`
- `0x409ef0` `HudUiPanel::DestructorCallback` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6455`
- `0x40a590` `HudUiPanel::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15958`
- `0x40a940` `HudCmdCommandList::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10935`
- `0x40aa30` `HudCmdKeyAButton::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10998`
- `0x40ab20` `HudCmdKeyBButton::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11061`
- `0x40ac10` `HudCmdJoyButton::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11124`
- `0x40ad00` `HudCmdMouseButton::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11187`
- `0x40ba60` `HudCmdKeyAButton::OnClearBinding` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12213`
- `0x40ba90` `HudCmdBindButtonBase::OnSelectionChangedRefresh` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12227`
- `0x40bae0` `HudCmdKeyBButton::OnClearBinding` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12246`
- `0x40bb30` `HudCmdJoyButton::OnClearBinding` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12266`
- `0x40bb80` `HudCmdMouseButton::OnClearBinding` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12291`
- `0x40bdf0` `StdPtrVector::ClearNoOpDestroy` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10653`
- `0x40be00` `HudCmdBinding::DestroyRange` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10793`
- `0x40be60` `HudCmdBindingEntry::CopyRange` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10675`
- `0x40be90` `HudUiPanel::Invalidate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15919`
- `0x40bea0` `HudUiPanel::GetFont` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15925`
- `0x40beb0` `HudUiPanel::SetFontHandle` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15930`
- `0x40bec0` `HudUiPanel::EnableWordWrapWithRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15937`
- `0x40bef0` `HudUiPanel::DestructorThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15793`
- `0x40bf00` `HudUtil::FreeFieldPtr` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15946`
- `0x40bf20` `HudCmdBindingEntry::DeleteAndReturnNull` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10697`
- `0x40bf80` `HudCmdBindButtonBase::AddBindingEntry` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10712`
- `0x40c1d0` `HudCmdBindButtonBase::ClearBindingEntries` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10866`
- `0x40c370` `zSys::ProbePlatformAndVideoCaps` -> `src/GameZRecoil/zSys/zSys_probe_platform.inl:2`
- `0x40c9c0` `HudUiOptionsPanel_Lighting::InitFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11482`
- `0x40c9e0` `HudUiOptionsPanel_Lighting::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11490`
- `0x40ca20` `HudUiOptionsPanel_Perspective::InitFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11511`
- `0x40ca40` `HudUiOptionsPanel_Perspective::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11519`
- `0x40ca80` `HudUiOptionsPanel_FullHud::InitFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11537`
- `0x40caa0` `HudUiCheckToggleWidget::OnActivateThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9290`
- `0x40cab0` `HudUiOptionsPanel_ObjectDetail::InitFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11554`
- `0x40cad0` `HudUiOptionsPanel_ObjectDetail::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11562`
- `0x40caf0` `HudUiOptionsPanel_TextureMemory::InitFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11580`
- `0x40cb10` `HudUiOptionsPanel_TextureMemory::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11588`
- `0x40cb30` `HudUiOptionsPanel_Effects::InitFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11606`
- `0x40cb70` `HudUiOptionsPanel_Effects::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11625`
- `0x40cb90` `HudUiOptionsPanel_SoundActive::InitFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11643`
- `0x40cbb0` `HudUiOptionsPanel_SoundActive::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11651`
- `0x40cbd0` `HudUiOptionsPanel_SoundQuality::InitFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11669`
- `0x40cbf0` `HudUiOptionsPanel_SoundQuality::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11677`
- `0x40cc10` `HudUiOptionsPanel_SoundVolume::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11691`
- `0x40cc30` `HudUiOptionsPanel_SoundVolume::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11699`
- `0x40cc60` `HudUiOptionsPanel_MusicEnable::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11714`
- `0x40cc80` `HudUiOptionsPanel_MusicEnable::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11722`
- `0x40ccc0` `HudUiOptionsPanel_MusicVolume::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11745`
- `0x40cd00` `HudUiOptionsPanel_MusicVolume::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11759`
- `0x40cd30` `HudUiOptionsPanel_Resolution::SyncFromOptions` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11777`
- `0x40ce80` `HudUiOptionsPanel_Resolution::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:11881`
- `0x40cf30` `HudUiCheckToggleWidget::DestructorCoreThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9165`
- `0x40cf40` `HudUiCycleSelectorWidget::DestructorCoreThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9501`
- `0x40cf50` `HudUiFillBitmap::DestructorCoreThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10104`
- `0x40d270` `HudLayoutSW::GlobalInit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:213`
- `0x40d280` `HudLayoutSW::RegisterAtExit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:218`
- `0x40d290` `HudLayoutSW::AtExitDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:223`
- `0x40d2f0` `HudLayoutHW::CrtInitGlobalSingleton` -> `src/GameZRecoil/zHud/zhud_ui.cpp:281`
- `0x40d300` `HudLayoutHW::GlobalInit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:256`
- `0x40d310` `HudLayoutHW::RegisterAtExit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:261`
- `0x40d320` `HudLayoutHW::AtExitDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:266`
- `0x40d3b0` `HudLayoutBase::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:190`
- `0x40d590` `HudUiMessage::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14438`
- `0x40d600` `HudUiTripletPanel::UnwindDestructFirstItem` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13664`
- `0x40d610` `HudUiTripletPanel::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13669`
- `0x40d660` `HudUiMgrObjectiveBlock::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13987`
- `0x40d6e0` `HudUiMgrSensorBlock::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13994`
- `0x40d780` `HudUiSlot::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14005`
- `0x40d9d0` `HudUiContainer::SetEnabled` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5976`
- `0x40d9e0` `HudUiMeter::ConstructorEx` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15409`
- `0x40da00` `HudUiMessage::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14419`
- `0x40daa0` `HudUiMessage::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14445`
- `0x40dac0` `HudUiCounter::HudUiCounter` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14100`
- `0x40db20` `HudUiSlot::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14014`
- `0x40db90` `HudUiSlot::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14028`
- `0x40dbd0` `HudUiSlot::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14042`
- `0x40dbf0` `HudUiCounterTextPanel::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16695`
- `0x40e010` `HudUiPanel::SetTextColorsAndMarkDirty` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15988`
- `0x40e040` `HudUiPanel::SetShadow` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16001`
- `0x40ec90` `HudLayoutBase::Shutdown_Stub` -> `src/GameZRecoil/zHud/zhud_ui.cpp:185`
- `0x40eca0` `HudUiTimerPanel::SetRunning` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16590`
- `0x40ecc0` `HudUiTimerPanel::SetElapsedSeconds` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16597`
- `0x40ece0` `HudUiTimerPanel::SetSeconds` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16604`
- `0x40ed10` `HudUiTimerPanel::GetSeconds` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16613`
- `0x40ed20` `HudUiTimerPanel::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16618`
- `0x40ed80` `HudUiTimerPanel::ConstructorDefault` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16659`
- `0x40ee60` `HudUiTimerPanel::UpdateHMSFromSeconds` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16572`
- `0x40ef00` `HudUiTimerPanel::SetTimeSeconds` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16552`
- `0x40ef60` `HudUiTimerPanelFloat::ConstructorDefault` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17725`
- `0x40f040` `HudUiTimerPanelFloat::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17715`
- `0x40f070` `HudUiCounter::ApplyFromLayoutNode` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14124`
- `0x40f0f0` `HudUiCounter::ReleaseStateImages` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14110`
- `0x40f130` `HudUiCounter::UpdateLayoutPosition` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14148`
- `0x40f1a0` `HudUiMgr::SetModeCounterState` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4056`
- `0x40f200` `HudUiTripletPanel::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13502`
- `0x40f2b0` `HudUiTripletPanel::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13523`
- `0x40f2d0` `HudUiWidget::HudUiWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13296`
- `0x40f3e0` `HudUiTripletPanel::ShutdownItems_Stub` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13657`
- `0x40f400` `HudUiTripletPanel::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13536`
- `0x40f460` `HudUiTripletPanel::SetVisibleCount` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13556`
- `0x40f4c0` `HudUiMgr::InitHudLayouts` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4633`
- `0x40f9e0` `HudUiPanel::SetTextColor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15974`
- `0x40fa10` `HudUiStatsListElement::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14056`
- `0x40fa20` `HudUiStatsListElement::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14075`
- `0x40fa40` `HudUiStatsListElement::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14063`
- `0x40fab0` `HudUiPanelSimple::ConstructorDefaultThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16543`
- `0x40fac0` `HudUiPanelSimple::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16514`
- `0x40fb70` `HudUiMeter::HudUiMeter` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15401`
- `0x40fb90` `HudUiTimerPanel::ZarWriteTimerDataCallback` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16646`
- `0x40fbb0` `HudUiTimerPanel::ZarReadTimerData` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16634`
- `0x40fbd0` `HudUiMgr::ShutdownResources` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4768`
- `0x40fdd0` `HudUiStringMenu::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14087`
- `0x40fe30` `HudUiShieldMessageWidget::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14519`
- `0x40fe90` `HudUiTopMessageStack::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17661`
- `0x40fef0` `HudUiChatMessageStack::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17708`
- `0x40ff50` `HudUiMgr::ActivateHud` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4597`
- `0x40ff80` `HudUiMgr::OnViewportChanged` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4533`
- `0x410140` `HudUiMgr::TickLayoutDelay` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3440`
- `0x410d10` `HudUiMgrSensor::SetViewportRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2621`
- `0x410ed0` `HudUiMgr::DisableHud` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4314`
- `0x411710` `HudUiMgr::ReticleStaticAtexitStub` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4071`
- `0x411720` `HudUiMgr::CopyReticleProjection` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4074`
- `0x411740` `HudUiMgr::SetReticleMode` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4085`
- `0x411750` `HudUiMgr::SetNanitePanelCount` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4049`
- `0x411a20` `HudUiMgrObjective::Begin` -> `src/GameZRecoil/zHud/zhud_ui.cpp:2906`
- `0x411eb0` `HudUiMgrObjective::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3011`
- `0x4126e0` `HudUiMessage::SelectVariantDisplay` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14292`
- `0x412790` `HudUiMessage::ApplySideImageSwap` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14321`
- `0x4127d0` `HudUiMessage::ClearDisplay` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14333`
- `0x412bd0` `HudLayoutBase::SetActive` -> `src/GameZRecoil/zHud/zhud_ui.cpp:196`
- `0x412be0` `HudLayoutBase::UpdateAll` -> `src/GameZRecoil/zHud/zhud_ui.cpp:287`
- `0x412bf0` `HudLayoutBase::Enable` -> `src/GameZRecoil/zHud/zhud_ui.cpp:294`
- `0x412bf0` `HudLayoutBase::Enable` -> `src/GameZRecoil/zHud/zhud_ui.cpp:298`
- `0x412c00` `HudLayoutBase::Disable` -> `src/GameZRecoil/zHud/zhud_ui.cpp:303`
- `0x413080` `HudLayoutHW::ReleaseImages` -> `src/GameZRecoil/zHud/zhud_ui.cpp:488`
- `0x413630` `HudUiMgr::TriggerCurrentLayoutOnActivated` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3431`
- `0x413730` `HudUiMgr::DestroySensorWindow` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4281`
- `0x4137c0` `HudUiAuxOverlay::ClearTextLines` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3133`
- `0x4137f0` `HudUiAuxOverlay::UpdateTextLine` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3104`
- `0x413910` `HudUiMgr::EnableTopAndChatStacks` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4612`
- `0x413950` `HudUiMgr::DisableTopAndChatStacks` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4623`
- `0x413ec0` `HudUiMessage::LoadWeaponLayoutFromNode` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14222`
- `0x413ff0` `HudUiMessage::ReleaseImages` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14273`
- `0x414180` `HudUiLoadingCheckpoint::AdvanceAndLog` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3036`
- `0x414210` `HudUiLoadingCheckpoint::InitTable` -> `src/GameZRecoil/zHud/zhud_ui.cpp:3067`
- `0x414660` `GameNet::EndChatComposeAndSendThunk` -> `src/Battlesport/GameNet.cpp:1657`
- `0x414670` `HudUiTripletEntries::GetCount` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1418`
- `0x4146a0` `HudUiTripletEntries::CopyRange` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1427`
- `0x4146e0` `HudUiTripletEntries::FillN` -> `src/GameZRecoil/zHud/zhud_ui.cpp:1445`
- `0x414b50` `WestwoodOnlineUpgradeDownloadEventSink::CallbackNoOp` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:46`
- `0x415100` `RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.cpp:22`
- `0x415110` `RecoilStateMainMenuTransition::StaticInit` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.cpp:33`
- `0x415120` `RecoilStateMainMenuTransition::RegisterAtExit` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.cpp:43`
- `0x415130` `RecoilStateMainMenuTransition::AtExitDestructor` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.cpp:53`
- `0x415170` `RecoilStateMainMenuTransition::RecoilStateMainMenuTransition` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.cpp:9`
- `0x4151b0` `RecoilStateMainMenuTransition::~RecoilStateMainMenuTransition` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.cpp:63`
- `0x415220` `RecoilStateMainMenuTransition::OnTryBecomeCurrent` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_OnTryBecomeCurrent.cpp:59`
- `0x415370` `RecoilStateMainMenuTransition::OnResume` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_OnResume.cpp:6`
- `0x4153d0` `RecoilStateMainMenuTransition::OnDeactivate` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_OnDeactivate.cpp:74`
- `0x415630` `RecoilStateMainMenuTransition::ClearPausedAudioSnapshot` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_ClearPausedAudioSnapshot.cpp:4`
- `0x415650` `RecoilStateMainMenuTransition::QueueEnter` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_QueueEnter.cpp:4`
- `0x415670` `RecoilStateMainMenuTransition::SetDeferredVideoModeIndex` -> `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_SetDeferredVideoModeIndex.cpp:4`
- `0x4159d0` `zFMV_Action::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:158`
- `0x4159e0` `zFMV_Action::RunBlockingTimed` -> `src/GameZRecoil/zFMV/fmv_script.cpp:215`
- `0x415aa0` `zFMV_Action::~zFMV_Action` -> `src/GameZRecoil/zFMV/fmv_script.cpp:152`
- `0x417430` `HudSensorTracker::WriteMissionDataSection` -> `src/Battlesport/HudSensorTracker.cpp:1746`
- `0x417640` `HudSensorTracker::RegisterMissionSectionHandlers` -> `src/Battlesport/HudSensorTracker.cpp:1852`
- `0x417680` `HudSensorTracker::ZarMission_SaveCallback` -> `src/Battlesport/HudSensorTracker.cpp:3325`
- `0x4176b0` `HudSensorTracker::ZarMissionLate_SaveCallback` -> `src/Battlesport/HudSensorTracker.cpp:3351`
- `0x417770` `HudSensorTracker::InitMissionIdAndFlags` -> `src/Battlesport/HudSensorTracker.cpp:1870`
- `0x4177a0` `HudSensorTracker::SetMissionId` -> `src/Battlesport/HudSensorTracker.cpp:1897`
- `0x4177d0` `HudSensorTracker::SetZbdPath` -> `src/Battlesport/HudSensorTracker.cpp:1884`
- `0x417800` `HudSensorTracker::GetMissionId` -> `src/Battlesport/HudSensorTracker.cpp:1910`
- `0x417ee0` `HudSensorTracker::UnloadObjectives` -> `src/Battlesport/HudSensorTracker.cpp:2320`
- `0x417f60` `HudSensorObjectiveSlot::Reset` -> `src/Battlesport/HudSensorTracker.cpp:1733`
- `0x4186f0` `HudSensorTracker::GetObjectiveBriefingStringsAndImageRef` -> `src/Battlesport/HudSensorTracker.cpp:2715`
- `0x419470` `HudSensorTracker::SetRuntimeTimerSecAndGoalValue` -> `src/Battlesport/HudSensorTracker.cpp:2793`
- `0x419aa0` `HudUiNetGameSetupPanel::Constructor` -> `src/Battlesport/HudUiNetGameSetup.cpp:173`
- `0x41a160` `HudUiNetGameSetupPanel_CancelButton::OnActivate` -> `src/Battlesport/HudUiNetGameSetup.cpp:477`
- `0x41a190` `HudUiNumericTextInput::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14872`
- `0x41a200` `HudUiClampedIntTextInput::HudUiClampedIntTextInput` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14887`
- `0x41a290` `HudUiNumericTextInput::OnAcceptForwardToCommit` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15146`
- `0x41a2a0` `HudUiClampedIntTextInput::OnRawKeyboardChar` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15161`
- `0x41a2d0` `HudUiClampedIntTextInput::CommitAndGetValue` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15175`
- `0x41a350` `HudUiClampedIntStepButton::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15214`
- `0x41a3f0` `HudUiNumericTextInput::DestructorThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15044`
- `0x41a400` `HudUiNetGameSetupPanel::Destructor` -> `src/Battlesport/HudUiNetGameSetup.cpp:432`
- `0x41a570` `HudUiCycleSelectorWidget::ScalarDeletingDestructorThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9518`
- `0x41a590` `HudUiCheckToggleWidget::ScalarDeletingDestructorThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9182`
- `0x41a5b0` `HudUiNetGameSetupPanel_LaunchButton::OnActivate` -> `src/Battlesport/HudUiNetGameSetup.cpp:491`
- `0x41a7b0` `HudUiNetGameSetupTextInput::OnActivateFocusAndCursor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15250`
- `0x41a820` `HudUiNetGameSetupPanel_NextWorldButton::OnActivate` -> `src/Battlesport/HudUiNetGameSetup.cpp:569`
- `0x41a9c0` `HudUiNetGameSetupPanel_PrevWorldButton::OnActivate` -> `src/Battlesport/HudUiNetGameSetup.cpp:583`
- `0x41b150` `NetSessionBrowserDialog::RefreshSessionList` -> `src/Battlesport/GameNet.cpp:894`
- `0x41be70` `HudUiNetExitPanel_ExitButton::OnActivate` -> `src/Battlesport/HudUiNetExitPanel.cpp:76`
- `0x41bf10` `HudUiNetExitPanel_ResumeWidget::OnActivate` -> `src/Battlesport/HudUiNetExitPanel.cpp:84`
- `0x41bf40` `HudUiNetExitPanel_ResumeWidget::OnShowPreview` -> `src/Battlesport/HudUiNetExitPanel.cpp:92`
- `0x41bfa0` `HudUiNetExitPanel_ResumeWidget::OnHidePreview` -> `src/Battlesport/HudUiNetExitPanel.cpp:121`
- `0x41c070` `HudUiNetExitPanel::Show` -> `src/Battlesport/HudUiNetExitPanel.cpp:158`
- `0x41c080` `HudUiNetExitPanel::Tick` -> `src/Battlesport/HudUiNetExitPanel.cpp:163`
- `0x41c0a0` `HudUiNetExitPanel::DestroyGlobal` -> `src/Battlesport/HudUiNetExitPanel.cpp:169`
- `0x41c480` `HudUiZrdWidget::ScalarDeletingDestructorThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8852`
- `0x41c4a0` `HudUiNumericTextInput::ScalarDeletingDestructorThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15065`
- `0x41c4c0` `HudUiZrdWidgetEx17C::ScalarDeletingDestructorThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10509`
- `0x41cca0` `PickupTypeTable::FreeOptMeta` -> `src/Battlesport/pickup.cpp:2301`
- `0x41eb30` `Player_UnderwaterFxPass3Ui::Player_UnderwaterFxPass3Ui` -> `src/Battlesport/player.cpp:1927`
- `0x41eb90` `Player_ProjectileCameraFxPass3Ui::Player_ProjectileCameraFxPass3Ui` -> `src/Battlesport/player.cpp:1943`
- `0x420be0` `zReader::LoadMoversFromZrd` -> `src/GameZRecoil/zReader/zreader_load.cpp:1688`
- `0x421da0` `zClass_Node::PropagateExtraFlagsRecursive` -> `src/GameZRecoil/zClass/Class.c:2554`
- `0x421e20` `zReader::BuildResolvedParentDir` -> `src/GameZRecoil/zReader/zreader_load.cpp:1601`
- `0x423440` `Player_UnderwaterFxPass3Ui::ApplyBlueTint` -> `src/Battlesport/player.cpp:1959`
- `0x423450` `Player_ProjectileCameraFxPass3Ui::ApplyGreenMask` -> `src/Battlesport/player.cpp:1968`
- `0x425a20` `Player::TickLocalPlayerControls` -> `src/Battlesport/player.cpp:10016`
- `0x429f10` `zInput::BindGroupList_StaticInitAndRegisterAtExit` -> `src/GameZRecoil/zInput/zInput.cpp:736`
- `0x429f20` `zInput::BindGroupListStaticInit` -> `src/GameZRecoil/zInput/zInput.cpp:706`
- `0x429f40` `zInput::BindGroupListRegisterAtExit` -> `src/GameZRecoil/zInput/zInput.cpp:728`
- `0x429f50` `zInput::BindGroupListAtExitDestructor` -> `src/GameZRecoil/zInput/zInput.cpp:717`
- `0x429f80` `zInput::BindGroupList_Clear` -> `src/GameZRecoil/zInput/zInput.cpp:1792`
- `0x42a000` `zInput_BindGroupInfo::Destroy` -> `src/GameZRecoil/zInput/zInput.cpp:692`
- `0x42a070` `zInput::BindGroupList_AddGroup` -> `src/GameZRecoil/zInput/zInput.cpp:1812`
- `0x42a2c0` `zInput::BindGroupList_AddCommandToGroup` -> `src/GameZRecoil/zInput/zInput.cpp:1856`
- `0x42a480` `zInput::BindGroupList_GetCount` -> `src/GameZRecoil/zInput/zInput.cpp:1737`
- `0x42a4a0` `zInput::BindGroupList_GetGroupTitle` -> `src/GameZRecoil/zInput/zInput.cpp:1750`
- `0x42a4b0` `zInput::BindGroupList_GetGroupCommandCount` -> `src/GameZRecoil/zInput/zInput.cpp:1762`
- `0x42a4d0` `zInput::BindGroupList_GetGroupCommandId` -> `src/GameZRecoil/zInput/zInput.cpp:1779`
- `0x42a4e0` `zInput::BindMap_GetCommandLabel` -> `src/GameZRecoil/zInput/zInput.cpp:1891`
- `0x42a4f0` `zInput::BindMap_GetCommandHint` -> `src/GameZRecoil/zInput/zInput.cpp:1903`
- `0x42a500` `zInput::BindMap_AddDefaultBinding` -> `src/GameZRecoil/zInput/zInput.cpp:1915`
- `0x42a550` `zInput::BindMap_InitDefaultBindings` -> `src/GameZRecoil/zInput/zInput.cpp:1939`
- `0x42a9d0` `zInput_BindGroupInfoVec::Count` -> `src/GameZRecoil/zInput/zInput.cpp:679`
- `0x42db50` `zCom::QueryInterfaceFromInterfaceMap` -> `src/GameZRecoil/zCom/zCom.cpp:24`
- `0x42dc30` `zCom::ConnectionPointContainer_Advise` -> `src/GameZRecoil/zCom/zCom.cpp:90`
- `0x42dcf0` `zCom::ConnectionPointContainer_Unadvise` -> `src/GameZRecoil/zCom/zCom.cpp:126`
- `0x42dda0` `WestwoodOnlineUpgradeApiInitState::Init` -> `src/Battlesport/WestwoodOnlineUpgradeApi.cpp:220`
- `0x42de10` `RecoilApp::GetMessageMap` -> `src/Battlesport/RecoilApp.cpp:2360`
- `0x42de60` `RecoilApp::~RecoilApp` -> `src/Battlesport/RecoilApp.cpp:2351`
- `0x42df10` `RecoilApp_AttractFmvState::~RecoilApp_AttractFmvState` -> `src/Battlesport/RecoilApp.cpp:3192`
- `0x42df50` `RecoilApp_IntroFmvState::~RecoilApp_IntroFmvState` -> `src/Battlesport/RecoilApp.cpp:3194`
- `0x42df90` `RecoilApp_IState::~RecoilApp_IState is` -> `src/Battlesport/RecoilApp.cpp:3153`
- `0x42df90` `RecoilApp_IState::~RecoilApp_IState` -> `src/Battlesport/RecoilApp.h:33`
- `0x42dfa0` `RecoilApp::RecoilApp` -> `src/Battlesport/RecoilApp.cpp:2343`
- `0x42e070` `RecoilApp_MissionFmvState::~RecoilApp_MissionFmvState` -> `src/Battlesport/RecoilApp.cpp:3196`
- `0x42e110` `RecoilApp::CreateMainWnd` -> `src/Battlesport/RecoilApp.cpp:1669`
- `0x42e170` `zInput::DI_SetJoystickEnabled` -> `src/GameZRecoil/zInput/zInput.cpp:3729`
- `0x42e220` `RecoilApp::StartEngine` -> `src/Battlesport/RecoilApp.cpp:1830`
- `0x42e430` `RecoilApp::ShutdownEngine` -> `src/Battlesport/RecoilApp.cpp:1941`
- `0x42e490` `RecoilApp::LoadZbdAndStartEngine` -> `src/Battlesport/RecoilApp.cpp:1966`
- `0x42e4d0` `RecoilApp::LoadZbdAndSetupSensorTracker` -> `src/Battlesport/RecoilApp.cpp:1980`
- `0x42e520` `RecoilApp::InitInstance` -> `src/Battlesport/RecoilApp.cpp:1482`
- `0x42e930` `RecoilApp::ExitInstance` -> `src/Battlesport/RecoilApp.cpp:1459`
- `0x42e990` `RecoilApp::ActivateExistingInstance` -> `src/Battlesport/RecoilApp.cpp:1881`
- `0x42e9f0` `RecoilApp::PreTranslateMessage` -> `src/Battlesport/RecoilApp.cpp:1900`
- `0x42ea20` `RecoilApp_IntroFmvState::OnTryBecomeCurrent` -> `src/Battlesport/RecoilApp.cpp:2946`
- `0x42eac0` `RecoilApp_IntroFmvState::OnUpdateShouldQuit` -> `src/Battlesport/RecoilApp.cpp:2981`
- `0x42eb00` `RecoilApp_FmvState::OnIdleOrDispatch` -> `src/Battlesport/RecoilApp.cpp:3003`
- `0x42eb10` `RecoilApp_IntroFmvState::OnDeactivate` -> `src/Battlesport/RecoilApp.cpp:3011`
- `0x42eb20` `RecoilApp_MainMenuPrepState::OnTryBecomeCurrent` -> `src/Battlesport/RecoilApp.cpp:3016`
- `0x42eb60` `RecoilApp_MainMenuPrepState::OnUpdateShouldQuit` -> `src/Battlesport/RecoilApp.cpp:3028`
- `0x42eb70` `RecoilApp_AttractFmvState::Constructor` -> `src/Battlesport/RecoilApp.cpp:2941`
- `0x42ebf0` `RecoilApp_AttractFmvState::OnTryBecomeCurrent` -> `src/Battlesport/RecoilApp.cpp:3034`
- `0x42ec80` `RecoilApp_AttractFmvState::OnUpdateShouldQuit` -> `src/Battlesport/RecoilApp.cpp:3071`
- `0x42eca0` `RecoilApp_AttractFmvState::OnDeactivate` -> `src/Battlesport/RecoilApp.cpp:3085`
- `0x42ed30` `RecoilApp_MissionFmvState::Constructor` -> `src/Battlesport/RecoilApp.cpp:3090`
- `0x42ee40` `HudUiBackgroundContainer::SetEnabled` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6106`
- `0x42eea0` `RecoilApp_PlayState::RecoilApp_PlayState` -> `src/Battlesport/RecoilApp.cpp:2613`
- `0x42eec0` `RecoilApp_PlayState::OnWndActivate` -> `src/Battlesport/RecoilApp.cpp:2619`
- `0x42f280` `RecoilApp_PlayState::TickAndRenderFrame` -> `src/Battlesport/RecoilApp_PlayState_TickAndRenderFrame.cpp:18`
- `0x42f8a0` `RecoilApp_PlayState::OnResume` -> `src/Battlesport/RecoilApp.cpp:2874`
- `0x42f9d0` `RecoilApp_LeaveNetworkState::OnTryBecomeCurrent` -> `src/Battlesport/RecoilApp.cpp:2933`
- `0x42f9f0` `zInput_DI_InitForceFeedbackEffectSet` -> `src/GameZRecoil/zInput/zInput.cpp:1055`
- `0x42fa80` `zInput_DI_IsForceFeedbackEnabled` -> `src/GameZRecoil/zInput/zInput.cpp:781`
- `0x42faa0` `zInput_DI_RestartPrimaryFireEffect` -> `src/GameZRecoil/zInput/zInput.cpp:999`
- `0x42fac0` `zInput_DI_PlayAltFireEffect` -> `src/GameZRecoil/zInput/zInput.cpp:1015`
- `0x42fdc0` `zInput_DI_UpdateSteerAndPitchForceEffects` -> `src/GameZRecoil/zInput/zInput.cpp:1163`
- `0x42ffa0` `zInput_DI_CreateConstantForceEffectScaled` -> `src/GameZRecoil/zInput/zInput.cpp:925`
- `0x430070` `zInput_DI_CreateConstantForceEffectWithDirection` -> `src/GameZRecoil/zInput/zInput.cpp:949`
- `0x430100` `zInput_DI_CreateSineEffectScaled` -> `src/GameZRecoil/zInput/zInput.cpp:972`
- `0x4301e0` `CZRecoilFrame::CreateObject` -> `src/Battlesport/CZRecoilFrame.cpp:135`
- `0x430240` `CZRecoilFrame::GetRuntimeClass` -> `src/Battlesport/CZRecoilFrame.cpp:150`
- `0x430250` `CZRecoilFrame::Constructor` -> `src/Battlesport/CZRecoilFrame.cpp:174`
- `0x430610` `CZRecoilFrame::Destructor` -> `src/Battlesport/CZRecoilFrame.cpp:339`
- `0x430680` `CZRecoilFrame::SetMenuBarVisibility` -> `src/Battlesport/CZRecoilFrame.cpp:346`
- `0x4306e0` `CZRecoilFrame::GetMessageMap` -> `src/Battlesport/CZRecoilFrame.cpp:160`
- `0x4306f0` `CZRecoilFrame::BuildWindowTitle` -> `src/Battlesport/CZRecoilFrame.cpp:373`
- `0x430740` `CZRecoilFrame::OnMenuStartSinglePlayer` -> `src/Battlesport/CZRecoilFrame.cpp:387`
- `0x430760` `CZRecoilFrame::OnMenuOpenCampaign` -> `src/Battlesport/CZRecoilFrame.cpp:394`
- `0x430770` `CZRecoilFrame::OnOpenFileDialog` -> `src/Battlesport/CZRecoilFrame.cpp:400`
- `0x4308a0` `CZRecoilFrame::OnMenuExitGame` -> `src/Battlesport/CZRecoilFrame.cpp:554`
- `0x4308c0` `CZRecoilFrame::ConfigureModeFeatureFlags` -> `src/Battlesport/CZRecoilFrame.cpp:456`
- `0x4309b0` `CZRecoilFrame::OnMenuSetVideoMode2` -> `src/Battlesport/CZRecoilFrame.cpp:518`
- `0x4309d0` `CZRecoilFrame::OnMenuSetVideoMode3` -> `src/Battlesport/CZRecoilFrame.cpp:524`
- `0x4309f0` `CZRecoilFrame::OnMenuSetVideoMode4` -> `src/Battlesport/CZRecoilFrame.cpp:530`
- `0x430a10` `CZRecoilFrame::OnMenuSetVideoMode5` -> `src/Battlesport/CZRecoilFrame.cpp:536`
- `0x430a30` `CZRecoilFrame::OnMenuSetVideoMode6` -> `src/Battlesport/CZRecoilFrame.cpp:542`
- `0x430a50` `CZRecoilFrame::OnMenuSetVideoMode7` -> `src/Battlesport/CZRecoilFrame.cpp:548`
- `0x430a70` `CZRecoilFrame::OnMenuToggleHud` -> `src/Battlesport/CZRecoilFrame.cpp:564`
- `0x430a90` `CZRecoilFrame::OnUpdateHudCmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:569`
- `0x430ab0` `CZRecoilFrame::OnMenuToggleFullscreen` -> `src/Battlesport/CZRecoilFrame.cpp:577`
- `0x430ad0` `CZRecoilFrame::OnMenuOpenHelpDocs` -> `src/Battlesport/CZRecoilFrame.cpp:582`
- `0x431270` `CZRecoilFrame::OnMenuStartMultiplayer` -> `src/Battlesport/CZRecoilFrame.cpp:768`
- `0x431290` `CZRecoilFrame::OnMenuStartCampaignMode` -> `src/Battlesport/CZRecoilFrame.cpp:778`
- `0x4312b0` `CZRecoilFrame::OnMenuStartCampaignMode2` -> `src/Battlesport/CZRecoilFrame.cpp:788`
- `0x4312d0` `CZRecoilFrame::OnMenuStartCampaignMode3` -> `src/Battlesport/CZRecoilFrame.cpp:798`
- `0x4312f0` `CZRecoilFrame::OnMenuStartCampaignMode4` -> `src/Battlesport/CZRecoilFrame.cpp:808`
- `0x431310` `CZRecoilFrame::OnMenuStartCampaignMode5` -> `src/Battlesport/CZRecoilFrame.cpp:818`
- `0x431330` `CZRecoilFrame::OnMenuToggleArchiveBanks` -> `src/Battlesport/CZRecoilFrame.cpp:872`
- `0x431380` `CZRecoilFrame::OnMenuToggleTexturePacks` -> `src/Battlesport/CZRecoilFrame.cpp:884`
- `0x4313d0` `CZRecoilFrame::OnUpdateVideoMode2CmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:904`
- `0x431430` `CZRecoilFrame::OnUpdateVideoMode3CmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:914`
- `0x431490` `CZRecoilFrame::OnUpdateVideoMode4CmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:924`
- `0x4314f0` `CZRecoilFrame::OnUpdateVideoMode5CmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:934`
- `0x431550` `CZRecoilFrame::OnUpdateVideoMode6CmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:944`
- `0x4315b0` `CZRecoilFrame::OnUpdateVideoMode7CmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:954`
- `0x431610` `CZRecoilFrame::SetHwApiAndInitMode` -> `src/Battlesport/CZRecoilFrame.cpp:1128`
- `0x431680` `CZRecoilFrame::InitFallbackMode` -> `src/Battlesport/CZRecoilFrame.cpp:1145`
- `0x4316c0` `CZRecoilFrame::EnsureHwApiInitialized` -> `src/Battlesport/CZRecoilFrame.cpp:1154`
- `0x431730` `CZRecoilFrame::InitStartupHwApiFromOptions` -> `src/Battlesport/CZRecoilFrame.cpp:1180`
- `0x431790` `CZRecoilFrame::OnMenuSelectHwApi0` -> `src/Battlesport/CZRecoilFrame.cpp:964`
- `0x4317a0` `CZRecoilFrame::OnMenuSelectHwApi1` -> `src/Battlesport/CZRecoilFrame.cpp:969`
- `0x4317b0` `CZRecoilFrame::OnMenuSelectHwApi2` -> `src/Battlesport/CZRecoilFrame.cpp:974`
- `0x4317c0` `CZRecoilFrame::OnMenuSelectHwApi3` -> `src/Battlesport/CZRecoilFrame.cpp:979`
- `0x4317d0` `CZRecoilFrame::UpdateHwApiMenuItem` -> `src/Battlesport/CZRecoilFrame.cpp:984`
- `0x431870` `CZRecoilFrame::OnUpdateHwApi0CmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:1010`
- `0x4318b0` `CZRecoilFrame::OnUpdateHwApi1CmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:1018`
- `0x4318c0` `CZRecoilFrame::OnUpdateHwApi2CmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:1028`
- `0x4318d0` `CZRecoilFrame::OnUpdateHwApi3CmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:1038`
- `0x4318e0` `CZRecoilFrame::OnUpdateFullscreenCmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:1048`
- `0x431900` `CZRecoilFrame::OnMenuToggleCDAudio` -> `src/Battlesport/CZRecoilFrame.cpp:1059`
- `0x431920` `CZRecoilFrame::OnUpdateCDAudioCmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:1064`
- `0x431950` `CZRecoilFrame::OnMenuToggleJoystick` -> `src/Battlesport/CZRecoilFrame.cpp:1072`
- `0x431970` `CZRecoilFrame::OnUpdateJoystickCmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:1077`
- `0x431a80` `MfcCmdUI::EnableAlways` -> `src/Battlesport/CZRecoilFrame.cpp:166`
- `0x431a90` `CZRecoilFrame::OnMenuSelectDirectSound` -> `src/Battlesport/CZRecoilFrame.cpp:1085`
- `0x431aa0` `CZRecoilFrame::OnUpdateDirectSoundCmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:1090`
- `0x431ad0` `CZRecoilFrame::OnMenuSelectA3D` -> `src/Battlesport/CZRecoilFrame.cpp:1098`
- `0x431ae0` `CZRecoilFrame::OnUpdateA3DCmdUI` -> `src/Battlesport/CZRecoilFrame.cpp:1103`
- `0x431b10` `CZRecoilFrame::OnSize` -> `src/Battlesport/CZRecoilFrame.cpp:1111`
- `0x431bf0` `GameNetSpawnPointList::InitGlobals` -> `src/Battlesport/GameNet.cpp:3501`
- `0x431c20` `GameNetPlayerRowList::Reset` -> `src/Battlesport/GameNet.cpp:3511`
- `0x431dd0` `Net::InitFromZrd` -> `src/Battlesport/GameNet.cpp:620`
- `0x4320b0` `GameNet::WaitForLocalPlayerColorIndex` -> `src/Battlesport/GameNet.cpp:2624`
- `0x4320f0` `GameNet::ResetRemotePlayersAndSpawnLists` -> `src/Battlesport/GameNet.cpp:2586`
- `0x4321b0` `GameNet::UnregisterGameplayPacketHandlers` -> `src/Battlesport/GameNet.cpp:2414`
- `0x4322a0` `GameNet::ResetHudTimerPanelNetStateLongCountdown` -> `src/Battlesport/GameNet.cpp:2644`
- `0x4327e0` `GameNet::HandlePkt06_PlayerStateSnapshot` -> `src/Battlesport/GameNet.cpp:2244`
- `0x432830` `GameNet::FindPlayerRowByKey` -> `src/Battlesport/GameNet.cpp:1664`
- `0x432860` `GameNet::SpawnRemotePlayerFromPkt06_PlayerStateSnapshot` -> `src/Battlesport/GameNet.cpp:2084`
- `0x432ae0` `GameNet::ApplyPkt06_PlayerStateSnapshotToRow` -> `src/Battlesport/GameNet.cpp:1989`
- `0x433710` `GameNet::SetStatusBitsFromFlags` -> `src/Battlesport/GameNet.cpp:2663`
- `0x433730` `GameNet::GetStatusBitAllowMaps` -> `src/Battlesport/GameNet.cpp:2675`
- `0x433740` `GameNet::GetStatusBitNameTags` -> `src/Battlesport/GameNet.cpp:2684`
- `0x433a40` `HudTimerPanelNetState::ClearTailFlagsLocal` -> `src/Battlesport/GameNet.cpp:610`
- `0x433a50` `GameNetPlayerRow::ApplyPlayerColorTint` -> `src/Battlesport/GameNet.cpp:584`
- `0x434460` `GameNet::SendPkt14_HudTimerAndFlagsSync` -> `src/Battlesport/GameNet.cpp:3409`
- `0x434550` `GameNet::HostUpdateSessionDescStatusFields` -> `src/Battlesport/GameNet.cpp:3473`
- `0x4345a0` `GameNetPlayerRowList::AppendNewRow` -> `src/Battlesport/GameNet.cpp:3519`
- `0x434650` `GameNetPlayerRow::DestroyEmbeddedPanel` -> `src/Battlesport/GameNet.cpp:605`
- `0x436e00` `zTurret_Runtime::Shutdown` -> `src/GameZRecoil/zTurret/zTurret.cpp:1296`
- `0x437aa0` `zTurret_System::ResetIterationState` -> `src/GameZRecoil/zTurret/zTurret.cpp:1316`
- `0x437ab0` `zTurret_System::Shutdown` -> `src/GameZRecoil/zTurret/zTurret.cpp:1507`
- `0x437dc0` `zTurret_System::FreeAllRuntimes` -> `src/GameZRecoil/zTurret/zTurret.cpp:1480`
- `0x437e60` `zClass_Node::SetContextRecursive` -> `src/GameZRecoil/zClass/Class.c:2587`
- `0x437fe4` `zClass_Object3D_ModelRefLerpQueue::ClearGlobalState` -> `src/GameZRecoil/zClass/Object3d.c:1141`
- `0x438020` `zClass_Object3D_ModelRefLerpQueue::Add` -> `src/GameZRecoil/zClass/Object3d.c:1154`
- `0x438180` `zClass_Object3D_ModelRefLerpQueue::Reset` -> `src/GameZRecoil/zClass/Object3d.c:1299`
- `0x4381d0` `zClass_Object3D_ModelRefLerpQueue::Update` -> `src/GameZRecoil/zClass/Object3d.c:1214`
- `0x4383e0` `zUtil_SaveGameStateList_Init` -> `src/GameZRecoil/zUtil/zSaveGame.cpp:11`
- `0x438430` `zUtil_SaveGameState::FreeOwnedResources` -> `src/GameZRecoil/zUtil/zSaveGame.cpp:77`
- `0x4384e0` `zUtil_SaveGameStateList_AllocAppend` -> `src/GameZRecoil/zUtil/zSaveGame.cpp:40`
- `0x438980` `RecoilVersion::GetString` -> `src/Battlesport/RecoilVersion.cpp:5`
- `0x438b10` `PickupAirdropSpawnRef::ShutdownGlobal` -> `src/Battlesport/pickup.cpp:390`
- `0x43ce80` `NetUi::VerifyWinsock2OrPromptContinue` -> `src/Battlesport/NetUi.cpp:11`
- `0x43d130` `WestwoodOnlineUpgradeApi::CreateInstanceAndLoadConfig` -> `src/Battlesport/WestwoodOnlineUpgradeApi.cpp:390`
- `0x43d280` `WestwoodOnlineUpgradeApi::Shutdown` -> `src/Battlesport/WestwoodOnlineUpgradeApi.cpp:459`
- `0x43d2e0` `WestwoodOnlineUpgradeApi::Init` -> `src/Battlesport/WestwoodOnlineUpgradeApi.cpp:252`
- `0x43f610` `WestwoodOnlineUpgradeApiEventSink::CreateInstance` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:199`
- `0x43f830` `WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:286`
- `0x43f9d0` `WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1677`
- `0x43fa70` `WestwoodOnlineUpgradeApiEventSink::OnServerError` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:351`
- `0x43fa90` `WestwoodOnlineUpgradeApiEventSink::OnApiStatus` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:369`
- `0x43fde0` `WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:468`
- `0x43fe50` `WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:500`
- `0x43ff80` `WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:559`
- `0x4401d0` `WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:671`
- `0x4402c0` `WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:818`
- `0x4404c0` `WestwoodOnlineUpgradeApiEventSink::LaunchSelectedSession` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:910`
- `0x4407e0` `WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString1` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1119`
- `0x440a30` `WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString0` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1055`
- `0x440c80` `WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301B` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1184`
- `0x440ce0` `WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1212`
- `0x440d40` `WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301D` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1426`
- `0x440d90` `WestwoodOnlineUpgradeApiEventSink::AppendConnectStatus301E_3021` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1449`
- `0x440e10` `WestwoodOnlineUpgradeApiEventSink::AppendBrowseRecordStatus3022_3025` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1473`
- `0x440ef0` `WestwoodOnlineUpgradeApiEventSink::AppendValueStatus3026` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1510`
- `0x440f40` `WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1535`
- `0x441040` `WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordListReceived` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1589`
- `0x4411c0` `WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt0` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1239`
- `0x441200` `WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt1` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1261`
- `0x441240` `WestwoodOnlineUpgradeApiEventSink::CallbackNoOp0` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1284`
- `0x441250` `WestwoodOnlineUpgradeApiEventSink::CallbackNoOp1` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1297`
- `0x441260` `WestwoodOnlineUpgradeApiEventSink::AppendTimeStatus302A` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1309`
- `0x4412c0` `WestwoodOnlineUpgradeApiEventSink::AppendValueStatus302B_302C` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1335`
- `0x441350` `WestwoodOnlineUpgradeApiEventSink::UpdateSessionResultItemFlags` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:1359`
- `0x441480` `WestwoodOnlineUpgradeApiEventSink::OnSessionLaunchResult` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:738`
- `0x441600` `WestwoodOnlineUpgradeRefCountAndLock::Init` -> `src/Battlesport/WestwoodOnlineUpgradeRefCountAndLock.cpp:4`
- `0x441620` `WestwoodOnlineUpgradeApiEventSink::Release` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:258`
- `0x441660` `WestwoodOnlineUpgradeApiEventSink::QueryInterface` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:219`
- `0x441680` `WestwoodOnlineUpgradeApiEventSink::Destructor` -> `src/Battlesport/WestwoodOnlineUpgradeApiEventSink.cpp:276`
- `0x4422a0` `WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:221`
- `0x4422f0` `WestwoodOnlineUpgradeDownload::UnadviseAndRelease` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:245`
- `0x4425c0` `WestwoodOnlineUpgradeDownloadEventSink::CreateInstance` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:200`
- `0x442660` `WestwoodOnlineUpgradeDownloadEventSink::OnDownloadFinished` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:56`
- `0x442680` `WestwoodOnlineUpgradeDownloadEventSink::OnDownloadError` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:66`
- `0x4426b0` `WestwoodOnlineUpgradeDownloadEventSink::OnDownloadProgress` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:79`
- `0x442720` `WestwoodOnlineUpgradeDownloadEventSink::OnStateChanged` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:113`
- `0x442770` `WestwoodOnlineUpgradeDownloadEventSink::AddRef` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:130`
- `0x442790` `WestwoodOnlineUpgradeDownloadEventSink::Release` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:138`
- `0x4427d0` `WestwoodOnlineUpgradeDownloadEventSink::QueryInterface` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:161`
- `0x4427f0` `WestwoodOnlineUpgradeDownloadEventSink::Destructor` -> `src/GameZRecoil/wwonline/upgrade_download.cpp:190`
- `0x4429d0` `RecoilApp::InitMainWindow` -> `src/Battlesport/RecoilApp.cpp:1679`
- `0x442a10` `RecoilApp::TakeSkipWaitMessage` -> `src/Battlesport/RecoilApp.cpp:2490`
- `0x442a30` `RecoilApp::MarkSkipWaitMessage` -> `src/Battlesport/RecoilApp.cpp:2497`
- `0x442a50` `RecoilApp::EngineInit` -> `src/Battlesport/RecoilApp.cpp:1719`
- `0x442bc0` `RecoilApp::ShutdownSubsystems` -> `src/Battlesport/RecoilApp.cpp:1926`
- `0x442c00` `RecoilApp::GetMainWnd` -> `src/Battlesport/RecoilApp.cpp:2365`
- `0x442c10` `RecoilApp::StartEngineAndQueueStartupState` -> `src/Battlesport/RecoilApp.cpp:2452`
- `0x443140` `RecoilApp::GetCurrentState` -> `src/Battlesport/RecoilApp.cpp:2370`
- `0x443160` `RecoilApp::QueueSwitchCurrentState` -> `src/Battlesport/RecoilApp.cpp:2384`
- `0x443310` `RecoilApp::QueuePushState` -> `src/Battlesport/RecoilApp.cpp:2409`
- `0x4434b0` `RecoilApp::QueueExitCurrentState` -> `src/Battlesport/RecoilApp.cpp:2430`
- `0x443650` `RecoilApp::OnIdleOrDispatch` -> `src/Battlesport/RecoilApp.cpp:2470`
- `0x443690` `RecoilApp_StateQueue::GrowAndCenterChunkBaseList` -> `src/Battlesport/RecoilApp.cpp:2111`
- `0x443700` `RecoilApp_StateQueueBlock::InitFromCursor` -> `src/Battlesport/RecoilApp.cpp:2096`
- `0x443730` `CZGameFrame::CreateObject` -> `src/Battlesport/CZGameFrame.cpp:126`
- `0x443790` `CZGameFrame::GetBaseRuntimeClass` -> `src/Battlesport/CZGameFrame.cpp:106`
- `0x4437a0` `CZGameFrame::GetRuntimeClass` -> `src/Battlesport/CZGameFrame.cpp:146`
- `0x4437b0` `CZGameFrame::GetBaseMessageMap` -> `src/Battlesport/CZGameFrame.cpp:155`
- `0x4437c0` `CZGameFrame::GetMessageMap` -> `src/Battlesport/CZGameFrame.cpp:165`
- `0x4437d0` `CZGameFrame::Constructor` -> `src/Battlesport/CZGameFrame.cpp:190`
- `0x443830` `CZGameFrame::Destructor` -> `src/Battlesport/CZGameFrame.cpp:206`
- `0x4438a0` `CZGameFrame::IsWindowValid` -> `src/Battlesport/CZGameFrame.cpp:174`
- `0x4438c0` `CZGameFrame::BuildWindowTitle` -> `src/Battlesport/CZGameFrame.cpp:219`
- `0x4438f0` `CZGameFrame::OnClose` -> `src/Battlesport/CZGameFrame.cpp:255`
- `0x443900` `CZGameFrame::OnPaint` -> `src/Battlesport/CZGameFrame.cpp:264`
- `0x443a20` `CZGameFrame::OnSize` -> `src/Battlesport/CZGameFrame.cpp:365`
- `0x443a40` `zVid::UpdateCachedClientRectIfUpdateMaskEnabled` -> `src/GameZRecoil/zVideo/zVideo.cpp:1137`
- `0x443a50` `CZGameFrame::OnMove` -> `src/Battlesport/CZGameFrame.cpp:384`
- `0x443a60` `CZGameFrame::OnCreate` -> `src/Battlesport/CZGameFrame.cpp:231`
- `0x443ab0` `CZGameFrame::OnDestroy` -> `src/Battlesport/CZGameFrame.cpp:318`
- `0x443ae0` `CZGameFrame::OnActivate` -> `src/Battlesport/CZGameFrame.cpp:332`
- `0x443b50` `CZGameFrame::OnAppIdleDispatchMessage` -> `src/Battlesport/CZGameFrame.cpp:398`
- `0x4478c0` `zClass_Class::AllocNodeFromFreeList` -> `src/GameZRecoil/zClass/Class.c:345`
- `0x447980` `zClass_Class::DeleteNodeByType` -> `src/GameZRecoil/zClass/Class.c:2091`
- `0x447a70` `zClass_Class::FreeNodeToFreeList` -> `src/GameZRecoil/zClass/Class.c:2024`
- `0x447b60` `zClass_Class::TryFreeNode` -> `src/GameZRecoil/zClass/Class.c:2069`
- `0x447c60` `zClass_Class::gwNodeSetActive` -> `src/GameZRecoil/zClass/Class.c:921`
- `0x447d20` `zClass_Class::gwNodeSetFlag16` -> `src/GameZRecoil/zClass/Class.c:968`
- `0x447d70` `zClass_Class::gwNodeSetFlag17` -> `src/GameZRecoil/zClass/Class.c:989`
- `0x447dc0` `zClass_Class::gwNodeSetName` -> `src/GameZRecoil/zClass/Class.c:1055`
- `0x447e30` `zClass_Class::gwNodeGetName` -> `src/GameZRecoil/zClass/Class.c:1090`
- `0x447e60` `zClass_Class::gwNodeSetDisplayInstance` -> `src/GameZRecoil/zClass/Class.c:1010`
- `0x447f00` `zClass_Class::gwNodeGetUserData` -> `src/GameZRecoil/zClass/Class.c:1106`
- `0x447f30` `zClass_Class::gwNodeSetActionCallback` -> `src/GameZRecoil/zClass/Class.c:1126`
- `0x447fe0` `zClass_Class::gwNodeSetActionCallbackTail` -> `src/GameZRecoil/zClass/Class.c:1178`
- `0x448090` `zClass_Class::gwNodeSetPriority` -> `src/GameZRecoil/zClass/Class.c:1232`
- `0x448100` `zClass_Class::gwNodeSetCellPickable` -> `src/GameZRecoil/zClass/Class.c:1263`
- `0x448140` `zClass_Class::gwNodeGetCellPickable` -> `src/GameZRecoil/zClass/Class.c:1285`
- `0x448180` `zClass_Class::gwNodeGetNodeType` -> `src/GameZRecoil/zClass/Class.c:1303`
- `0x4481b0` `zClass_Class::gwNodeSetRaycastable` -> `src/GameZRecoil/zClass/Class.c:1322`
- `0x4481f0` `zClass_Class::gwNodeGetRaycastable` -> `src/GameZRecoil/zClass/Class.c:1343`
- `0x448230` `zClass_Class::gwNodeSetPickable` -> `src/GameZRecoil/zClass/Class.c:1360`
- `0x448270` `zClass_Class::gwNodeGetPickable` -> `src/GameZRecoil/zClass/Class.c:1381`
- `0x4482b0` `zClass_Class::gwNodeSetHasHitCallback` -> `src/GameZRecoil/zClass/Class.c:1398`
- `0x4482f0` `zClass_Class::gwNodeSetBypassFarClip` -> `src/GameZRecoil/zClass/Class.c:1420`
- `0x448330` `zClass_Class::gwNodeSetNodeType` -> `src/GameZRecoil/zClass/Class.c:1443`
- `0x448360` `zClass_Class::gwNodeClearVariantGate` -> `src/GameZRecoil/zClass/Class.c:1466`
- `0x4483a0` `zClass_Class::gwNodeSetVertexAlphaOverride` -> `src/GameZRecoil/zClass/Class.c:1486`
- `0x4483f0` `zClass_Class::AddChild` -> `src/GameZRecoil/zClass/Class.c:1722`
- `0x4484d0` `zClass_Class::AddChildGeneric` -> `src/GameZRecoil/zClass/Class.c:1912`
- `0x448570` `zClass_Class::RemoveChild` -> `src/GameZRecoil/zClass/Class.c:1814`
- `0x448660` `zClass_Class::RemoveChildGeneric` -> `src/GameZRecoil/zClass/Class.c:1957`
- `0x4487c0` `zClass_Class::gwNodeGetWorldBBoxCorners` -> `src/GameZRecoil/zClass/Class.c:552`
- `0x448920` `zClass_Class::gwNodeGetViewBBoxCorners` -> `src/GameZRecoil/zClass/Class.c:619`
- `0x448cc0` `zClass_Class::gwNodeUpdate` -> `src/GameZRecoil/zClass/Class.c:390`
- `0x448e90` `zClass_Class::gwNodeRecalcBBox` -> `src/GameZRecoil/zClass/Class.c:812`
- `0x4491b0` `zClass_Class::gwNodeComputeChildBBox` -> `src/GameZRecoil/zClass/Class.c:737`
- `0x449420` `zClass_Class::gwNodeUpdateDisplayInstance` -> `src/GameZRecoil/zClass/Class.c:495`
- `0x449480` `gwNode::BuildNodeToAncestorMatrix` -> `src/GameZRecoil/zClass/Class.c:2138`
- `0x449ab0` `zClass_Class::gwNodeGetRoot` -> `src/GameZRecoil/zClass/Class.c:1508`
- `0x449af0` `zClass_Class::gwNodeGetWorldChild` -> `src/GameZRecoil/zClass/Class.c:1600`
- `0x449b40` `zClass_Class::SetSingleParentFlagRecursive` -> `src/GameZRecoil/zClass/Class.c:1635`
- `0x449ba0` `zClass_Camera::SetViewDistance` -> `src/GameZRecoil/zClass/Camera.c:1033`
- `0x449be0` `zClass_Camera::gwCameraNew` -> `src/GameZRecoil/zClass/Camera.c:326`
- `0x449c90` `zClass_Camera::gwCameraAddChild` -> `src/GameZRecoil/zClass/Camera.c:359`
- `0x449cd0` `zClass_Camera::gwCameraRemoveChild` -> `src/GameZRecoil/zClass/Camera.c:390`
- `0x449d20` `zClass_Camera::gwCameraSetFlagBit0` -> `src/GameZRecoil/zClass/Camera.c:420`
- `0x449da0` `zClass_Camera::SetTargetNode` -> `src/GameZRecoil/zClass/Camera.c:447`
- `0x449db0` `zClass_Camera::SetActiveCamera` -> `src/GameZRecoil/zClass/Camera.c:453`
- `0x449dc0` `zClass_Camera::SetObjectHseTestEnabled` -> `src/GameZRecoil/zClass/Camera.c:461`
- `0x449dd0` `zClass_Camera::gwCameraSetWorld` -> `src/GameZRecoil/zClass/Camera.c:467`
- `0x449e80` `zClass_Camera::gwCameraGetWorld` -> `src/GameZRecoil/zClass/Camera.c:530`
- `0x449e90` `zClass_Camera::gwCameraSetWindow` -> `src/GameZRecoil/zClass/Camera.c:537`
- `0x449ea0` `zClass_Camera::gwCameraSetPosition` -> `src/GameZRecoil/zClass/Camera.c:570`
- `0x449f50` `zClass_Camera::ActivateChildren` -> `src/GameZRecoil/zClass/Camera.c:547`
- `0x449fb0` `zClass_Camera::gwCameraTranslate` -> `src/GameZRecoil/zClass/Camera.c:607`
- `0x44a060` `zClass_Camera::gwCameraGetPosition` -> `src/GameZRecoil/zClass/Camera.c:641`
- `0x44a0f0` `zClass_Camera::gwCameraSetTarget` -> `src/GameZRecoil/zClass/Camera.c:667`
- `0x44a1a0` `zClass_Camera::gwCameraTranslateTarget` -> `src/GameZRecoil/zClass/Camera.c:702`
- `0x44a250` `zClass_Camera::gwCameraGetTarget` -> `src/GameZRecoil/zClass/Camera.c:736`
- `0x44a2f0` `zClass_Camera::gwCameraSetNearFarClip` -> `src/GameZRecoil/zClass/Camera.c:763`
- `0x44a380` `zClass_Camera::gwCameraGetNearFarClip` -> `src/GameZRecoil/zClass/Camera.c:788`
- `0x44a410` `zClass_Camera::gwCameraSetViewport` -> `src/GameZRecoil/zClass/Camera.c:812`
- `0x44a580` `zClass_Camera::gwCameraGetViewport` -> `src/GameZRecoil/zClass/Camera.c:857`
- `0x44a610` `zClass_Camera::gwCameraSetFOV` -> `src/GameZRecoil/zClass/Camera.c:905`
- `0x44a760` `zClass_Camera::gwCameraGetFOV` -> `src/GameZRecoil/zClass/Camera.c:881`
- `0x44a7f0` `zClass_Camera::gwCameraGetClipDistance` -> `src/GameZRecoil/zClass/Camera.c:944`
- `0x44a870` `zClass_Camera::gwCameraSetClipDistance` -> `src/GameZRecoil/zClass/Camera.c:966`
- `0x44a910` `zClass_Camera::gwCameraSetHorizon` -> `src/GameZRecoil/zClass/Camera.c:989`
- `0x44a980` `zClass_Camera::gwCameraSetHorizonXZ` -> `src/GameZRecoil/zClass/Camera.c:1011`
- `0x44af60` `zClass_Sound::RenderTraverse` -> `src/GameZRecoil/zClass/Sound.c:552`
- `0x44b140` `zClass_Light::RenderTraverse` -> `src/GameZRecoil/zClass/Light.c:849`
- `0x44b300` `zClass_Object3D::RenderTraverse` -> `src/GameZRecoil/zClass/Object3d.c:376`
- `0x44b710` `zClass_Animate::RenderTraverse` -> `src/GameZRecoil/zClass/Animate.c:361`
- `0x44b8c0` `zClass_Lod::RenderTraverse` -> `src/GameZRecoil/zClass/Lod.c:113`
- `0x44bea0` `zClass_Sequence::RenderTraverse` -> `src/GameZRecoil/zClass/Seq.c:476`
- `0x44bfb0` `zClass_Switch::RenderTraverse` -> `src/GameZRecoil/zClass/Switch.c:49`
- `0x44c3c0` `zClass_Camera::BuildFrustumGridTiles` -> `src/GameZRecoil/zClass/Camera.c:1149`
- `0x44c8e0` `zClass_Camera::BuildFrustumGridTilesFromParams` -> `src/GameZRecoil/zClass/Camera.c:1302`
- `0x44d990` `zClass_Node::PropagateTransformDirtyRecursive` -> `src/GameZRecoil/zClass/Object3d.c:1344`
- `0x44d9e0` `zClass_Object3D::PropagateTransformDirty` -> `src/GameZRecoil/zClass/Object3d.c:599`
- `0x44daa0` `zClass_Object3D::gwObject3DInit` -> `src/GameZRecoil/zClass/Object3d.c:474`
- `0x44db00` `zClass_Object3D::DeleteNode` -> `src/GameZRecoil/zClass/Object3d.c:590`
- `0x44db10` `zClass_Object3D::gwObject3DAddChild` -> `src/GameZRecoil/zClass/Object3d.c:501`
- `0x44db60` `zClass_Object3D::RemoveChild` -> `src/GameZRecoil/zClass/Object3d.c:546`
- `0x44dbb0` `zClass_Object3D::gwObject3DSetVisibleFlag` -> `src/GameZRecoil/zClass/Object3d.c:659`
- `0x44dc30` `zClass_Object3D::gwObject3DSetColorAlpha` -> `src/GameZRecoil/zClass/Object3d.c:687`
- `0x44dd90` `zClass_Object3D::gwObject3DSetAlphaScale` -> `src/GameZRecoil/zClass/Object3d.c:719`
- `0x44de10` `zClass_Object3D::gwObject3DGetAlphaScale` -> `src/GameZRecoil/zClass/Object3d.c:743`
- `0x44de80` `zClass_Object3D::gwObject3DSetLitFlag` -> `src/GameZRecoil/zClass/Object3d.c:766`
- `0x44df00` `zClass_Object3D::gwObject3DSetScale` -> `src/GameZRecoil/zClass/Object3d.c:822`
- `0x44dfd0` `zClass_Object3D::gwObject3DGetScale` -> `src/GameZRecoil/zClass/Object3d.c:795`
- `0x44e030` `zClass_Object3D::gwObject3DSetRotation` -> `src/GameZRecoil/zClass/Object3d.c:886`
- `0x44e110` `zClass_Object3D::gwObject3DGetRotation` -> `src/GameZRecoil/zClass/Object3d.c:859`
- `0x44e170` `zClass_Object3D::gwObject3DTranslateRotation` -> `src/GameZRecoil/zClass/Object3d.c:926`
- `0x44e270` `zClass_Object3D::gwObject3DGetPosition` -> `src/GameZRecoil/zClass/Object3d.c:966`
- `0x44e300` `zClass_Object3D::gwObject3DSetPosition` -> `src/GameZRecoil/zClass/Object3d.c:995`
- `0x44e3d0` `zClass_Object3D::gwObject3DTranslatePosition` -> `src/GameZRecoil/zClass/Object3d.c:1034`
- `0x44e4f0` `zClass_Object3D::gwObject3DSetMatrix` -> `src/GameZRecoil/zClass/Object3d.c:1094`
- `0x44e5b0` `zClass_Object3D::gwObject3DGetMatrixPtr` -> `src/GameZRecoil/zClass/Object3d.c:1073`
- `0x44e630` `zClass_TypeList::AllocLink` -> `src/GameZRecoil/zClass/List.c:501`
- `0x44e690` `zClass_TypeList::FreeLink` -> `src/GameZRecoil/zClass/List.c:533`
- `0x44e6d0` `zClass_TypeList::FreeAll` -> `src/GameZRecoil/zClass/List.c:551`
- `0x44e700` `zClass_TypeList::ProcessPendingRemovals` -> `src/GameZRecoil/zClass/List.c:561`
- `0x44e920` `zClass::ProcessDeferredWork` -> `src/GameZRecoil/zClass/List.c:906`
- `0x44ea70` `zClass_TypeList::UpdateAllBuckets` -> `src/GameZRecoil/zClass/List.c:751`
- `0x44eaa0` `zClass_TypeList::UpdateBucket` -> `src/GameZRecoil/zClass/List.c:762`
- `0x44eb00` `gwNode::UpdateSubtree` -> `src/GameZRecoil/zClass/List.c:845`
- `0x44eb50` `gwNode::UpdateTree` -> `src/GameZRecoil/zClass/List.c:862`
- `0x44eba0` `zClass_TypeList::UpdateQueuedTrees` -> `src/GameZRecoil/zClass/List.c:781`
- `0x44ebe0` `zClass_TypeList::UpdateSequences` -> `src/GameZRecoil/zClass/List.c:799`
- `0x44ec30` `zClass_TypeList::UpdateAnimations` -> `src/GameZRecoil/zClass/List.c:821`
- `0x44ec80` `zClass_Class::gwNodeUpdateAll` -> `src/GameZRecoil/zClass/List.c:1047`
- `0x44ec90` `zClass_TypeList::CountNodes` -> `src/GameZRecoil/zClass/List.c:611`
- `0x44ecb0` `zClass_TypeList::PrintBucket` -> `src/GameZRecoil/zClass/List.c:621`
- `0x44ecf0` `zClass::FindByTypeAndName` -> `src/GameZRecoil/zClass/List.c:944`
- `0x44ed50` `zClass_TypeList::GetBucketHead` -> `src/GameZRecoil/zClass/List.c:635`
- `0x44ed60` `zClass_NodeList::Insert` -> `src/GameZRecoil/zClass/List.c:879`
- `0x44ed90` `zClass_TypeList::Insert` -> `src/GameZRecoil/zClass/List.c:672`
- `0x44ee10` `zClass_TypeList::InsertChildNodes` -> `src/GameZRecoil/zClass/List.c:711`
- `0x44eea0` `zClass_NodeList::ProcessPendingFrees` -> `src/GameZRecoil/zClass/List.c:893`
- `0x44eed0` `zClass_TypeList::MarkPendingRemoval` -> `src/GameZRecoil/zClass/List.c:641`
- `0x44f000` `zClass_List::DeleteNodeFromLists` -> `src/GameZRecoil/zClass/List.c:114`
- `0x44f120` `zClass_List::DeleteAllOfType` -> `src/GameZRecoil/zClass/List.c:449`
- `0x44f690` `zClass_List::IterateBucketFiltered` -> `src/GameZRecoil/zClass/List.c:199`
- `0x44f6f0` `zClass::FindNextByTypePrefix` -> `src/GameZRecoil/zClass/List.c:975`
- `0x44f720` `zClass::FindNextByTypePrefix_Predicate` -> `src/GameZRecoil/zClass/List.c:966`
- `0x44f740` `zClass_Class::gwNodeFindNextByName` -> `src/GameZRecoil/zClass/List.c:1062`
- `0x44f750` `zClass_Class::gwNodeFindNextByName_Predicate` -> `src/GameZRecoil/zClass/List.c:1054`
- `0x44f7a0` `zClass_Window::gwWindowNew` -> `src/GameZRecoil/zClass/Window.c:112`
- `0x44f870` `zClass::RemoveChildChecked` -> `src/GameZRecoil/zClass/List.c:1014`
- `0x44f8b0` `zClass_Window::gwWindowSetResolution` -> `src/GameZRecoil/zClass/Window.c:165`
- `0x44f930` `zClass_Window::gwWindowGetResolution` -> `src/GameZRecoil/zClass/Window.c:187`
- `0x44f9c0` `zClass_Window::gwWindowSetSize` -> `src/GameZRecoil/zClass/Window.c:211`
- `0x44fa40` `zClass_Window::gwWindowGetSize` -> `src/GameZRecoil/zClass/Window.c:233`
- `0x44fad0` `zClass_Window::gwWindowSetBuffer` -> `src/GameZRecoil/zClass/Window.c:258`
- `0x44fb40` `zClass_Window::gwWindowSetClearPolygon` -> `src/GameZRecoil/zClass/Window.c:281`
- `0x44fbd0` `zClass_Window::gwWindowAddClearPolygonVertex` -> `src/GameZRecoil/zClass/Window.c:309`
- `0x44fcf0` `zClass_Window::gwWindowCloseClearPolygon` -> `src/GameZRecoil/zClass/Window.c:362`
- `0x44fdd0` `zClass_Display::gwDisplayInit` -> `src/GameZRecoil/zClass/Display.c:133`
- `0x44fe50` `zClass_Display::RemoveChild` -> `src/GameZRecoil/zClass/Display.c:177`
- `0x44fe90` `zClass_Display::gwDisplaySetSize` -> `src/GameZRecoil/zClass/Display.c:216`
- `0x44ff10` `zClass_Display::gwDisplaySetPosition` -> `src/GameZRecoil/zClass/Display.c:243`
- `0x44ff90` `zClass_Display::gwDisplaySetBackgroundColor` -> `src/GameZRecoil/zClass/Display.c:270`
- `0x450030` `zClass_World::QueueAreaUpdate` -> `src/GameZRecoil/zClass/cls_world.c:691`
- `0x4500b0` `zClass_World::RebuildAreaBounds` -> `src/GameZRecoil/zClass/cls_world.c:722`
- `0x450240` `zClass_World::DeleteNode` -> `src/GameZRecoil/zClass/cls_world.c:664`
- `0x450530` `zClass_World::ApplyPendingFogSettings` -> `src/GameZRecoil/zClass/cls_world.c:788`
- `0x450840` `zClass_World::WorldRectToGridIndex` -> `src/GameZRecoil/zClass/cls_world.c:858`
- `0x450a70` `zClass_World::EnsureGridCellDisplayPosition` -> `src/GameZRecoil/zClass/cls_world.c:1050`
- `0x450e40` `zClass_World::FreeVirtualAreaPartitions` -> `src/GameZRecoil/zClass/cls_world.c:627`
- `0x450f60` `zClass_World::AddChildToGridCell` -> `src/GameZRecoil/zClass/cls_world.c:1163`
- `0x4510e0` `zClass_World::AddChildAtGrid` -> `src/GameZRecoil/zClass/cls_world.c:1091`
- `0x451240` `zClass_World::RemoveChildAtGrid` -> `src/GameZRecoil/zClass/cls_world.c:1246`
- `0x451410` `zClass_World::RemoveLight` -> `src/GameZRecoil/zClass/cls_world.c:1362`
- `0x451640` `zClass_World::RemoveSound` -> `src/GameZRecoil/zClass/cls_world.c:1487`
- `0x4518b0` `zClass::SetNodeArraySize` -> `src/GameZRecoil/zClass/cls_util.c:102`
- `0x4518f0` `zClass::IsInitialized` -> `src/GameZRecoil/zClass/cls_util.c:118`
- `0x451a00` `zClass::ShutdownCore` -> `src/GameZRecoil/zClass/cls_util.c:171`
- `0x451a60` `zClass_Util::DestroyNodeRecursive` -> `src/GameZRecoil/zClass/cls_util.c:200`
- `0x452770` `zClass_Class::FindSubNodeByName` -> `src/GameZRecoil/zClass/Class.c:1536`
- `0x4527f0` `zClass_Node::HasRenderableDiPredicate` -> `src/GameZRecoil/zClass/Object3d.c:1329`
- `0x452810` `zClass::AnyNodeMatchesPredicateRecursive` -> `src/GameZRecoil/zClass/List.c:992`
- `0x452860` `zClass_Node::SetMaterialFlagBit9ForFlagBit0EntriesRecursive` -> `src/GameZRecoil/zClass/Class.c:2634`
- `0x4528a0` `zClass_Node::LoadFlagBit8MaterialImagesAndTexturePack` -> `src/GameZRecoil/zClass/Class.c:2670`
- `0x4528b0` `zClass_Node::InvalidateFlagBit8MaterialImagesRecursive` -> `src/GameZRecoil/zClass/Class.c:2656`
- `0x4528e0` `zClass_Node::AssignInt32ToDiRecursive` -> `src/GameZRecoil/zClass/Class.c:2682`
- `0x452920` `zClass_Class::AddChildValidated` -> `src/GameZRecoil/zClass/Class.c:1668`
- `0x452970` `zClass_Class::RemoveChildValidated` -> `src/GameZRecoil/zClass/Class.c:1695`
- `0x4529c0` `zClass_Sound::gwSoundNew` -> `src/GameZRecoil/zClass/Sound.c:97`
- `0x452ab0` `zClass_Sound::DeleteNode` -> `src/GameZRecoil/zClass/Sound.c:154`
- `0x452b80` `zClass_Sound::RemoveChild` -> `src/GameZRecoil/zClass/Sound.c:214`
- `0x452bc0` `zClass_Sound::SetSampleSetByName` -> `src/GameZRecoil/zClass/Sound.c:251`
- `0x452c60` `zClass_Sound::gwSoundSetActive` -> `src/GameZRecoil/zClass/Sound.c:305`
- `0x452d00` `zClass_Sound::gwSoundSetPosition` -> `src/GameZRecoil/zClass/Sound.c:357`
- `0x452d60` `zClass_Sound::gwSoundGetPosition` -> `src/GameZRecoil/zClass/Sound.c:399`
- `0x452dc0` `zClass_Sound::UpdatePlayback` -> `src/GameZRecoil/zClass/Sound.c:439`
- `0x452ec0` `zClass_Sound::ComputeWorldTransform` -> `src/GameZRecoil/zClass/Sound.c:514`
- `0x452fd0` `zClass_Light::gwLightNew` -> `src/GameZRecoil/zClass/Light.c:253`
- `0x453110` `zClass_Light::DeleteNode` -> `src/GameZRecoil/zClass/Light.c:324`
- `0x4531c0` `zClass_Light::RemoveChild` -> `src/GameZRecoil/zClass/Light.c:373`
- `0x453200` `zClass_Light::gwLightSetIntensity` -> `src/GameZRecoil/zClass/Light.c:410`
- `0x453250` `zClass_Light::gwLightSetFalloff` -> `src/GameZRecoil/zClass/Light.c:435`
- `0x4532a0` `zClass_Light::gwLightSetConeAngle` -> `src/GameZRecoil/zClass/Light.c:460`
- `0x4532f0` `zClass_Light::gwLightSetPointMode` -> `src/GameZRecoil/zClass/Light.c:488`
- `0x453350` `zClass_Light::gwLightSetDirectionalMode` -> `src/GameZRecoil/zClass/Light.c:510`
- `0x4533b0` `zClass_Light::gwLightSetParam` -> `src/GameZRecoil/zClass/Light.c:532`
- `0x453400` `zClass_Light::gwLightSetRange` -> `src/GameZRecoil/zClass/Light.c:557`
- `0x453500` `zClass_Light::gwLightGetRange` -> `src/GameZRecoil/zClass/Light.c:597`
- `0x453560` `zClass_Light::gwLightSetPosition` -> `src/GameZRecoil/zClass/Light.c:623`
- `0x4535c0` `zClass_Light::gwLightSetRotation` -> `src/GameZRecoil/zClass/Light.c:654`
- `0x453620` `zClass_Light::ComputeWorldTransform` -> `src/GameZRecoil/zClass/Light.c:685`
- `0x453880` `zClass_Light::gwLightUpdate` -> `src/GameZRecoil/zClass/Light.c:731`
- `0x453a40` `zClass_Light::gwLightGetSpecularColor` -> `src/GameZRecoil/zClass/Light.c:792`
- `0x453aa0` `zClass_Light::gwLightSetSpecularColor` -> `src/GameZRecoil/zClass/Light.c:819`
- `0x453b10` `zClass_Animate::DeleteNode` -> `src/GameZRecoil/zClass/Animate.c:293`
- `0x453b40` `zClass_Animate::AddChild` -> `src/GameZRecoil/zClass/Animate.c:257`
- `0x453b80` `zClass_Animate::RemoveChild` -> `src/GameZRecoil/zClass/Animate.c:315`
- `0x453bd0` `zClass_Animate::UpdateNode` -> `src/GameZRecoil/zClass/Animate.c:197`
- `0x453c90` `zClass_Animate::AdvanceTime` -> `src/GameZRecoil/zClass/Animate.c:118`
- `0x453d20` `zClass_Animate::SampleTransform` -> `src/GameZRecoil/zClass/Animate.c:153`
- `0x453ee0` `zClass_Sequence::gwSequenceNew` -> `src/GameZRecoil/zClass/Seq.c:56`
- `0x453f40` `zClass_Sequence::gwSequenceAddChild` -> `src/GameZRecoil/zClass/Seq.c:90`
- `0x454000` `zClass_Sequence::RemoveChild` -> `src/GameZRecoil/zClass/Seq.c:326`
- `0x4540c0` `zClass_Sequence::SetActive` -> `src/GameZRecoil/zClass/Seq.c:162`
- `0x454100` `zClass_Sequence::SetRepeat` -> `src/GameZRecoil/zClass/Seq.c:202`
- `0x454140` `zClass_Sequence::SetLoop` -> `src/GameZRecoil/zClass/Seq.c:243`
- `0x454180` `zClass_Sequence::SetPause` -> `src/GameZRecoil/zClass/Seq.c:284`
- `0x4541c0` `zClass_Sequence::Update` -> `src/GameZRecoil/zClass/Seq.c:396`
- `0x4542a0` `zClass_Lod::gwLodNew` -> `src/GameZRecoil/zClass/Lod.c:294`
- `0x454310` `zClass_Lod::gwLodAddChild` -> `src/GameZRecoil/zClass/Lod.c:319`
- `0x454320` `zClass_Lod::RemoveChild` -> `src/GameZRecoil/zClass/Lod.c:337`
- `0x454330` `zClass_Lod::SetComputeOwnDistance` -> `src/GameZRecoil/zClass/Lod.c:356`
- `0x454340` `zClass_Lod::SetTargetNodeAndRange` -> `src/GameZRecoil/zClass/Lod.c:372`
- `0x454360` `zClass::ResetCurrentZbdPath` -> `src/GameZRecoil/zClass/cls_util.c:165`
- `0x454370` `GameZ_ZBD::NodePtrToIndex` -> `src/GameZRecoil/zClass/cls_zbd.c:327`
- `0x4543a0` `zClass::NodePtrToValidatedIndex` -> `src/GameZRecoil/zClass/cls_zbd.c:315`
- `0x4543d0` `GameZ_ZBD::NodeIndexToPtr` -> `src/GameZRecoil/zClass/cls_zbd.c:336`
- `0x4543f0` `GameZ_ZBD::WriteNodeRefListIndices` -> `src/GameZRecoil/zClass/cls_zbd.c:345`
- `0x454bf0` `GameZ_ZBD::ReadNodeRefListIndices` -> `src/GameZRecoil/zClass/cls_zbd.c:739`
- `0x455e40` `zDEClient::ShutdownGlobals` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2532`
- `0x457ae0` `zDEClient::ClearFeatureEntriesAndMapTree` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:2516`
- `0x457e80` `zDEClient_MapTreeState::EraseRange` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1657`
- `0x457fe0` `zDEClient_MapTreeState::EraseAndAdvance` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1605`
- `0x458510` `zDEClient_MapTreeState::DestroySubtree` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1593`
- `0x4588c0` `zDEClient_MapTreeState::IterNextNodeRef` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1454`
- `0x458970` `zDEClient_MapTreeState::IterPrevNodeRef` -> `src/GameZRecoil/zDEClient/zdec_init.cpp:1478`
- `0x45d7a0` `zEffectAnim::ResetActivationPrereqCount` -> `src/GameZRecoil/zEffect/zEffect.cpp:943`
- `0x45e0f0` `zEffect::SetConditionalEffectLevel` -> `src/GameZRecoil/zEffect/zEffect.cpp:5508`
- `0x45e100` `zEffect_Anim::Init` -> `src/GameZRecoil/zEffect/zEffect.cpp:5185`
- `0x45fd10` `zEffectAnim::ShutdownEntry` -> `src/GameZRecoil/zEffect/zEffect.cpp:2762`
- `0x45fe50` `zEffect_Anim::Shutdown` -> `src/GameZRecoil/zEffect/zEffect.cpp:5147`
- `0x45fef0` `zEffect_Anim::ShutdownIfLoaded` -> `src/GameZRecoil/zEffect/zEffect.cpp:5176`
- `0x460020` `zEffect::Init` -> `src/GameZRecoil/zEffect/zEffect.cpp:5249`
- `0x460060` `zEffect::ShutdownAll` -> `src/GameZRecoil/zEffect/zEffect.cpp:8634`
- `0x460330` `zEffect::Reset` -> `src/GameZRecoil/zEffect/zEffect.cpp:8600`
- `0x4603d0` `zEffect_Anim::ClearActivationRecords` -> `src/GameZRecoil/zEffect/zEffect.cpp:3359`
- `0x460400` `zEffect_Anim::HasActivationRecord` -> `src/GameZRecoil/zEffect/zEffect.cpp:3369`
- `0x460470` `zEffect_Anim::GetActivationRecordCount` -> `src/GameZRecoil/zEffect/zEffect.cpp:3388`
- `0x460480` `zEffect_Anim::GetActivationRecordAt` -> `src/GameZRecoil/zEffect/zEffect.cpp:3393`
- `0x460ae0` `zEffect_Anim::AllocActivationRecord` -> `src/GameZRecoil/zEffect/zEffect.cpp:5073`
- `0x461a90` `zEffect_Anim::DiscardLastActivationRecord` -> `src/GameZRecoil/zEffect/zEffect.cpp:5133`
- `0x461eb0` `zEffect_Anim::SetActivationDispatchContext` -> `src/GameZRecoil/zEffect/zEffect.cpp:5138`
- `0x4622f0` `zError::EmitDebugBuffer` -> `src/GameZRecoil/zError/zerr_old.c:37`
- `0x462310` `RecoilError::InitOutputContext` -> `src/GameZRecoil/zError/zerr_old.c:10`
- `0x462330` `zFMV_Playback::Constructor` -> `src/GameZRecoil/zFMV/fmv.h:47`
- `0x462330` `zFMV_Playback::Constructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:353`
- `0x462360` `zFMV_Playback::Destructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:366`
- `0x462370` `zFMV_Playback::OpenAndPlay` -> `src/GameZRecoil/zFMV/fmv_script.cpp:402`
- `0x4624f0` `zFMV_Playback::StopAndClose` -> `src/GameZRecoil/zFMV/fmv_script.cpp:515`
- `0x462540` `zFMV_Playback::SetDestRect` -> `src/GameZRecoil/zFMV/fmv_script.cpp:541`
- `0x462570` `zFMV_Playback::ReportMciError` -> `src/GameZRecoil/zFMV/fmv_script.cpp:374`
- `0x4625e0` `zFMV_Script::Init` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1117`
- `0x462630` `zFMV_Script::Cleanup` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1143`
- `0x462660` `zFMV_Script::Reset` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1160`
- `0x4626b0` `zFMV_Script::LoadActionsFromZrd` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1189`
- `0x462e30` `zFMV_Action::RunBlockingImmediate` -> `src/GameZRecoil/zFMV/fmv_script.cpp:204`
- `0x462e90` `zFMV_ActionPlaySound::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:260`
- `0x462ed0` `zFMV_ActionWait::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:231`
- `0x462ee0` `zFMV_ActionWait::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:241`
- `0x462f00` `zFMV_Action::FlipSurfaces` -> `src/GameZRecoil/zFMV/fmv_script.cpp:191`
- `0x462f10` `zFMV_Script::AppendAction` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1367`
- `0x462f50` `zFMV_Script::RunBlocking` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1466`
- `0x462f90` `zFMV_Script::BeginCurrentAction` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1391`
- `0x463000` `zFMV_Script::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1423`
- `0x4630a0` `zFMV_Script::BeginAtTime` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1415`
- `0x4630e0` `zFMV_Script::UpdateAtTime` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1458`
- `0x463120` `zFMV_Script::BeginNow` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1484`
- `0x463130` `zFMV_ActionImage::ConstructorWithScreenRect` -> `src/GameZRecoil/zFMV/fmv.h:99`
- `0x463130` `zFMV_ActionImage::ConstructorWithScreenRect` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1494`
- `0x4631f0` `zFMV_ActionImage::ConstructorScaled` -> `src/GameZRecoil/zFMV/fmv.h:109`
- `0x4631f0` `zFMV_ActionImage::ConstructorScaled` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1517`
- `0x4632a0` `zFMV_ActionImage::~zFMV_ActionImage` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1611`
- `0x463300` `zFMV_ActionImage::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1547`
- `0x463320` `zFMV_ActionImage::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1555`
- `0x4633a0` `zFMV_ActionImage::End` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1600`
- `0x4633c0` `zFMV_ActionFade::Constructor` -> `src/GameZRecoil/zFMV/fmv.h:138`
- `0x4633c0` `zFMV_ActionFade::Constructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1623`
- `0x463410` `zFMV_ActionFade::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1645`
- `0x463440` `zFMV_ActionFade::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1654`
- `0x463550` `zFMV_ActionFade::End` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1726`
- `0x463570` `zFMV_ActionPlayAvi::Constructor` -> `src/GameZRecoil/zFMV/fmv.h:170`
- `0x463570` `zFMV_ActionPlayAvi::Constructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1737`
- `0x463670` `zFMV_ActionPlayAvi::~zFMV_ActionPlayAvi` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1777`
- `0x4636d0` `zFMV_ActionPlayAvi::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1788`
- `0x463790` `zFMV_ActionPlayAvi::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1839`
- `0x463820` `zFMV_ActionPlayAvi::End` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1868`
- `0x463850` `zFMV_ActionBlur::Constructor` -> `src/GameZRecoil/zFMV/fmv.h:243`
- `0x463850` `zFMV_ActionBlur::Constructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1937`
- `0x463870` `zFMV_ActionBlur::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1949`
- `0x463920` `zFMV_ActionBlur::End` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1990`
- `0x463950` `zFMV_ActionBlur::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:2012`
- `0x4639e0` `zFMV_ActionBlurH::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:2063`
- `0x463a70` `zFMV_ActionBlurV::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:2114`
- `0x463b00` `zFMV_ActionPlayMci::Constructor` -> `src/GameZRecoil/zFMV/fmv.h:197`
- `0x463b00` `zFMV_ActionPlayMci::Constructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1881`
- `0x463c10` `zFMV_ActionPlayMci::~zFMV_ActionPlayMci` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1919`
- `0x463c90` `zFMV_ActionPlayMci::Update` -> `src/GameZRecoil/zFMV/fmv_script.cpp:276`
- `0x463ca0` `zFMV_ActionPlayMci::Begin` -> `src/GameZRecoil/zFMV/fmv_script.cpp:286`
- `0x463cc0` `zFMV_ActionPlayMci::End` -> `src/GameZRecoil/zFMV/fmv_script.cpp:302`
- `0x463d50` `zFMV_Stream::Init` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1033`
- `0x463dd0` `zFMV_Stream::Destructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:1063`
- `0x463ef0` `zFMV_Stream::Constructor` -> `src/GameZRecoil/zFMV/fmv_script.cpp:554`
- `0x4641a0` `zFMV_Stream::OpenAudio` -> `src/GameZRecoil/zFMV/fmv_script.cpp:727`
- `0x4643a0` `zFMV_Stream::ReadAndDecodeFrame` -> `src/GameZRecoil/zFMV/fmv_script.cpp:864`
- `0x464540` `zFMV_Stream::FillAudioBuffer` -> `src/GameZRecoil/zFMV/fmv_script.cpp:954`
- `0x46d4c0` `zImage::GetDefaultImageRefPtr` -> `src/GameZRecoil/zImage/zimg_texture.cpp:29`
- `0x46d550` `zImage::InitTextureDirectory` -> `src/GameZRecoil/zImage/zimg_texture.cpp:81`
- `0x46d5a0` `zVid_Image::ReleaseIfNotDefault` -> `src/GameZRecoil/zVideo/zVideo.cpp:5043`
- `0x46d5b0` `zVid::SetTexturePackLoadState` -> `src/GameZRecoil/zVideo/zVideo.cpp:1577`
- `0x46d5c0` `zVid::GetTexturePackLoadState` -> `src/GameZRecoil/zVideo/zVideo.cpp:1569`
- `0x46d5d0` `zVid_TexDir::Shutdown` -> `src/GameZRecoil/zVideo/zVideo.cpp:6422`
- `0x46d6b0` `zVid_TexturePack::ShutdownBuiltinPacks` -> `src/GameZRecoil/zVideo/zVideo.cpp:6391`
- `0x46d730` `zImage::ShutdownTextureDirectoryRuntime` -> `src/GameZRecoil/zVideo/zVideo.cpp:6374`
- `0x46d870` `zVid_Image::ClearZeroAlphaPixelsInPlace` -> `src/GameZRecoil/zVideo/zVideo.cpp:5477`
- `0x46d900` `zImage::TexDir_FindOrCreateByPath` -> `src/GameZRecoil/zImage/zimg_texture.cpp:604`
- `0x46d940` `zVid_TexturePack_LoadImageByName` -> `src/GameZRecoil/zVideo/zVideo.cpp:6317`
- `0x46da40` `zVid_TexturePack_EnsureDefaultImagePackLoaded` -> `src/GameZRecoil/zVideo/zVideo.cpp:6080`
- `0x46dae0` `zVid_TexturePackEntry_LoadFromFile` -> `src/GameZRecoil/zVideo/zVideo.cpp:5975`
- `0x46dd30` `zVid_TexturePack_LoadBuiltinImageByName` -> `src/GameZRecoil/zVideo/zVideo.cpp:6333`
- `0x46e250` `zImage::InvalidateLoadedVariantChain` -> `src/GameZRecoil/zImage/zimg_texture.cpp:620`
- `0x46e290` `zImage_TexDirEntryPartial::GetVariantImageAtIndex` -> `src/GameZRecoil/zImage/zimg_texture.cpp:116`
- `0x46e2c0` `zImage::SetPathExtension` -> `src/GameZRecoil/zImage/zimg_texture.cpp:461`
- `0x46e380` `zImage::TexDirSetBaseNameFromPath` -> `src/GameZRecoil/zImage/zimg_texture.cpp:509`
- `0x46e4e0` `zVid_PaletteRemap::ApplyRecipeToPaletteVariant` -> `src/GameZRecoil/zVideo/zVideo.cpp:5770`
- `0x46e680` `zVid_PaletteRemap::FindRecipeIndex` -> `src/GameZRecoil/zVideo/zVideo.cpp:5744`
- `0x46e720` `zVid_PaletteRemap_BuildPaletteVariant` -> `src/GameZRecoil/zVideo/zVideo.cpp:5824`
- `0x46e8d0` `zVid_PaletteRemap_BuildAllRecipeVariantsForPalette` -> `src/GameZRecoil/zVideo/zVideo.cpp:5917`
- `0x46e960` `zVid_PaletteRemap_FindRecipeIndexFromRgb` -> `src/GameZRecoil/zVideo/zVideo.cpp:5956`
- `0x46e9b0` `zVid_Image::ResampleSquare` -> `src/GameZRecoil/zVideo/zVideo.cpp:5683`
- `0x46eb20` `zImage_Init` -> `src/GameZRecoil/zImage/zimg_texture.cpp:271`
- `0x46eb90` `zImage::ShutdownSubsystem` -> `src/GameZRecoil/zImage/zimg_texture.cpp:641`
- `0x46ebb0` `zImage::Shutdown` -> `src/GameZRecoil/zImage/zimg_texture.cpp:633`
- `0x46ebd0` `zImage_InitMissionResources` -> `src/GameZRecoil/zImage/zimg_texture.cpp:255`
- `0x46ec00` `zVid_Image::Create` -> `src/GameZRecoil/zVideo/zVideo.cpp:5011`
- `0x46ec20` `zVid_Image::QueryBytesPerPixel` -> `src/GameZRecoil/zVideo/zVideo.cpp:5316`
- `0x46ec30` `zVid_Image::SetHeaderFlagsByte` -> `src/GameZRecoil/zVideo/zVideo.cpp:5323`
- `0x46ec40` `zVid_Image::QueryPixelDataBytes` -> `src/GameZRecoil/zVideo/zVideo.cpp:5466`
- `0x46ec60` `zVid_Image::SetFormatCode` -> `src/GameZRecoil/zVideo/zVideo.cpp:5332`
- `0x46ec70` `zVid_Image_SetPixels` -> `src/GameZRecoil/zVideo/zVideo.cpp:5381`
- `0x46ec90` `zVid_Image::SetSize` -> `src/GameZRecoil/zVideo/zVideo.cpp:5341`
- `0x46ecc0` `zVid_Image::Destroy` -> `src/GameZRecoil/zVideo/zVideo.cpp:5022`
- `0x46ecf0` `zVid_Image::ReleaseOwnedBuffers` -> `src/GameZRecoil/zVideo/zVideo.cpp:5286`
- `0x46ed70` `zVid_Image::ReadHeader` -> `src/GameZRecoil/zVideo/zVideo.cpp:5536`
- `0x46ede0` `zVid_Image::ReadData` -> `src/GameZRecoil/zVideo/zVideo.cpp:5570`
- `0x46ef70` `zVid_Image::ReadFromFile` -> `src/GameZRecoil/zVideo/zVideo.cpp:5660`
- `0x46efc0` `zImage_Font::GetByIndexOrDefault` -> `src/GameZRecoil/zImage/zimg_texture.cpp:648`
- `0x46efe0` `zImage::FontsLoadFromPath` -> `src/GameZRecoil/zImage/zimg_texture.cpp:538`
- `0x46f130` `zImage_Font::BuildGlyphRects` -> `src/GameZRecoil/zImage/zimg_texture.cpp:784`
- `0x46f210` `zImage_Font::IsImageColumnTransparent` -> `src/GameZRecoil/zImage/zimg_texture.cpp:758`
- `0x46f260` `zImage_Font::MeasureString` -> `src/GameZRecoil/zImage/zimg_texture.cpp:660`
- `0x46f300` `zInput::Keyboard_InitDevice` -> `src/GameZRecoil/zInput/zInput.cpp:2845`
- `0x46f420` `zInput::Keyboard_ShutdownDevice` -> `src/GameZRecoil/zInput/zInput.cpp:2617`
- `0x46f450` `zInput::Keyboard_ResetTransitionState` -> `src/GameZRecoil/zInput/zInput.cpp:4021`
- `0x46f690` `zInput::Keyboard_PollState` -> `src/GameZRecoil/zInput/zInput.cpp:3171`
- `0x46f970` `zInput::Keyboard_SetRawEventCallback` -> `src/GameZRecoil/zInput/zInput.cpp:3147`
- `0x46f980` `zInput::Keyboard_GetKeyTransitionState` -> `src/GameZRecoil/zInput/zInput.cpp:3101`
- `0x46f9b0` `zInput::Keyboard_RegisterKeyCallback` -> `src/GameZRecoil/zInput/zInput.cpp:3118`
- `0x46f9d0` `zInput::Keyboard_UnregisterKeyCallback` -> `src/GameZRecoil/zInput/zInput.cpp:3135`
- `0x46f9f0` `zInput::Keyboard_ClearKeyCallbackTable` -> `src/GameZRecoil/zInput/zInput.cpp:2929`
- `0x46fa10` `zInput::Keyboard_WaitForAnyKeyPress` -> `src/GameZRecoil/zInput/zInput.cpp:3223`
- `0x46fba0` `zInput::Keyboard_TranslateDikToAscii` -> `src/GameZRecoil/zInput/zInput.cpp:3032`
- `0x46fd20` `zInput::Keyboard_InitDikToAsciiTable` -> `src/GameZRecoil/zInput/zInput.cpp:2943`
- `0x470020` `zInput::Mouse_ApplyClientCursorPosToOS` -> `src/GameZRecoil/zInput/zInput.cpp:2690`
- `0x470060` `zInput::Mouse_UpdateClientRectAndCenter` -> `src/GameZRecoil/zInput/zInput.cpp:2705`
- `0x4700a0` `zInput::Mouse_SetNormalizedCursorPos` -> `src/GameZRecoil/zInput/zInput.cpp:2735`
- `0x470150` `zInput::Mouse_RecenterCursor` -> `src/GameZRecoil/zInput/zInput.cpp:2720`
- `0x470180` `zInput::Mouse_RecenterCursorX` -> `src/GameZRecoil/zInput/zInput.cpp:2729`
- `0x470190` `zInput::Mouse_IsInitialized` -> `src/GameZRecoil/zInput/zInput.cpp:2762`
- `0x4701a0` `zInput::Mouse_SetClientSizeAndCenter` -> `src/GameZRecoil/zInput/zInput.cpp:2767`
- `0x4701f0` `zInput::Mouse_InitDevice` -> `src/GameZRecoil/zInput/zInput.cpp:2803`
- `0x4702e0` `zInput::Mouse_GetButtonTransitionState` -> `src/GameZRecoil/zInput/zInput.cpp:2660`
- `0x470310` `zInput::Mouse_UpdateAcquireState` -> `src/GameZRecoil/zInput/zInput.cpp:3762`
- `0x470360` `zInput::Mouse_ShutdownDevice` -> `src/GameZRecoil/zInput/zInput.cpp:3786`
- `0x4703a0` `zInput::Mouse_GetStateSnapshotPtr` -> `src/GameZRecoil/zInput/zInput.cpp:2781`
- `0x4703b0` `zInput::Mouse_PollAndStoreState` -> `src/GameZRecoil/zInput/zInput.cpp:3802`
- `0x4703c0` `zInput::Mouse_PollState` -> `src/GameZRecoil/zInput/zInput.cpp:3833`
- `0x4704f0` `zInput::Mouse_ApplyAccumulatedDelta` -> `src/GameZRecoil/zInput/zInput.cpp:3902`
- `0x4705f0` `zInput::Mouse_GetStateSnapshot` -> `src/GameZRecoil/zInput/zInput.cpp:2788`
- `0x470610` `zInput::Mouse_ResetTransitionState` -> `src/GameZRecoil/zInput/zInput.cpp:3954`
- `0x470670` `zInput::Mouse_SetCooperativeLevelFlags` -> `src/GameZRecoil/zInput/zInput.cpp:2650`
- `0x470680` `zInput::Mouse_WaitForButtonPress` -> `src/GameZRecoil/zInput/zInput.cpp:3879`
- `0x4706c0` `zInput_BindMapContext::InitFromTemplate` -> `src/GameZRecoil/zInput/zInput.cpp:162`
- `0x4707a0` `zInput_BindMapContext::FreeAllBuffers` -> `src/GameZRecoil/zInput/zInput.cpp:214`
- `0x470820` `zInput_BindMapContext::RebuildLookupIndices` -> `src/GameZRecoil/zInput/zInput.cpp:241`
- `0x4708f0` `zInput_BindMapContext::InitCommandMap` -> `src/GameZRecoil/zInput/zInput.cpp:281`
- `0x470960` `zInput_BindMapContext::FreeNonOwnedBuffers` -> `src/GameZRecoil/zInput/zInput.cpp:313`
- `0x4709d0` `zInput_BindMapContext::ResetAllBindings` -> `src/GameZRecoil/zInput/zInput.cpp:334`
- `0x470a10` `zInput::BindMap_PackBindingCode` -> `src/GameZRecoil/zInput/zInput.cpp:1723`
- `0x470a40` `zInput_BindMapContext::GetPrimaryKeyboardKey` -> `src/GameZRecoil/zInput/zInput.cpp:352`
- `0x470a60` `zInput_BindMapContext::GetSecondaryKeyboardKey` -> `src/GameZRecoil/zInput/zInput.cpp:362`
- `0x470a80` `zInput_BindMapContext::GetJoystickButtonSlot` -> `src/GameZRecoil/zInput/zInput.cpp:372`
- `0x470aa0` `zInput_BindMapContext::GetMouseButtonSlot` -> `src/GameZRecoil/zInput/zInput.cpp:382`
- `0x470ac0` `zInput_BindMapContext::GetCommandByPrimaryKey` -> `src/GameZRecoil/zInput/zInput.cpp:391`
- `0x470ad0` `zInput_BindMapContext::GetCommandBySecondaryKey` -> `src/GameZRecoil/zInput/zInput.cpp:398`
- `0x470ae0` `zInput_BindMapContext::GetCommandByAnyKeyboardKey` -> `src/GameZRecoil/zInput/zInput.cpp:405`
- `0x470b00` `zInput_BindMapContext::GetCommandByJoystickSlot` -> `src/GameZRecoil/zInput/zInput.cpp:417`
- `0x470b10` `zInput_BindMapContext::GetCommandByMouseSlot` -> `src/GameZRecoil/zInput/zInput.cpp:425`
- `0x470b20` `zInput_BindMapContext::SetPrimaryKeyBinding` -> `src/GameZRecoil/zInput/zInput.cpp:434`
- `0x470b80` `zInput_BindMapContext::SetSecondaryKeyBinding` -> `src/GameZRecoil/zInput/zInput.cpp:452`
- `0x470bf0` `zInput_BindMapContext::SetJoystickBinding` -> `src/GameZRecoil/zInput/zInput.cpp:470`
- `0x470c60` `zInput_BindMapContext::SetMouseBinding` -> `src/GameZRecoil/zInput/zInput.cpp:488`
- `0x470cd0` `zInput_BindMapContext::SetBindingRecord` -> `src/GameZRecoil/zInput/zInput.cpp:506`
- `0x470d40` `zInput_BindMapContext::DispatchMouseButtonCallbacks` -> `src/GameZRecoil/zInput/zInput.cpp:542`
- `0x470db0` `zInput_BindMapContext::DispatchJoystickButtonCallbacks` -> `src/GameZRecoil/zInput/zInput.cpp:571`
- `0x470df0` `zInput_BindMapContext::SetCommandCallback` -> `src/GameZRecoil/zInput/zInput.cpp:589`
- `0x470e80` `zInput_BindMapContext_DispatchFromKeyboardEvent` -> `src/GameZRecoil/zInput/zInput.cpp:148`
- `0x470eb0` `zInput_BindMapContext::ReadCommandInputState` -> `src/GameZRecoil/zInput/zInput.cpp:621`
- `0x470f50` `zInput_BindMapContext::CopyCommandLabel` -> `src/GameZRecoil/zInput/zInput.cpp:655`
- `0x470f80` `zInput::BindMap_FormatKeyComboName` -> `src/GameZRecoil/zInput/zInput.cpp:2364`
- `0x471040` `zInput::BindMap_CopyJoystickButtonName` -> `src/GameZRecoil/zInput/zInput.cpp:2415`
- `0x471070` `zInput::BindMap_CopyMouseButtonName` -> `src/GameZRecoil/zInput/zInput.cpp:2438`
- `0x4710a0` `zInput::BindMapSystem_Init` -> `src/GameZRecoil/zInput/zInput.cpp:2046`
- `0x471120` `zInput::BindMap_InitDikKeyNameTable` -> `src/GameZRecoil/zInput/zInput.cpp:1565`
- `0x4715e0` `zInput::BindMap_InitJoystickButtonNameTable` -> `src/GameZRecoil/zInput/zInput.cpp:1694`
- `0x471640` `zInput::BindMap_InitMouseButtonNameTable` -> `src/GameZRecoil/zInput/zInput.cpp:1711`
- `0x471660` `zInput::BindMapSystem_Shutdown` -> `src/GameZRecoil/zInput/zInput.cpp:2152`
- `0x4716b0` `zInput::BindMap_Current_RebuildLookupIndices` -> `src/GameZRecoil/zInput/zInput.cpp:2172`
- `0x4716c0` `zInput::BindMapCurrent_ResetAllBindings` -> `src/GameZRecoil/zInput/zInput.cpp:2178`
- `0x4716d0` `zInput::BindMapCurrent_GetPrimaryKeyboardKey` -> `src/GameZRecoil/zInput/zInput.cpp:2186`
- `0x4716e0` `zInput::BindMapCurrent_GetSecondaryKeyboardKey` -> `src/GameZRecoil/zInput/zInput.cpp:2198`
- `0x4716f0` `zInput::BindMapCurrent_GetJoystickButtonSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2210`
- `0x471700` `zInput::BindMapCurrent_GetMouseButtonSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2222`
- `0x471710` `zInput::BindMapCurrent_GetCommandByPrimaryKey` -> `src/GameZRecoil/zInput/zInput.cpp:2233`
- `0x471720` `zInput::BindMapCurrent_GetCommandBySecondaryKey` -> `src/GameZRecoil/zInput/zInput.cpp:2240`
- `0x471730` `zInput::BindMapCurrent_GetCommandByJoystickSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2247`
- `0x471740` `zInput::BindMapCurrent_GetCommandByMouseSlot` -> `src/GameZRecoil/zInput/zInput.cpp:2254`
- `0x471750` `zInput::BindMapCurrent_SetPrimaryKeyBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2261`
- `0x471760` `zInput::BindMapCurrent_SetSecondaryKeyBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2272`
- `0x471770` `zInput::BindMapCurrent_SetJoystickBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2283`
- `0x471780` `zInput::BindMapCurrent_SetMouseBinding` -> `src/GameZRecoil/zInput/zInput.cpp:2294`
- `0x471790` `zInput::BindMap_Current_SetBindingRecord` -> `src/GameZRecoil/zInput/zInput.cpp:2305`
- `0x4717c0` `zInput::BindMap_Current_SetCommandCallback` -> `src/GameZRecoil/zInput/zInput.cpp:2326`
- `0x4717d0` `zInput::BindMap_Current_ReadCommandInputState` -> `src/GameZRecoil/zInput/zInput.cpp:2337`
- `0x4717e0` `zInput::BindMapCurrent_CopyCommandLabel` -> `src/GameZRecoil/zInput/zInput.cpp:2345`
- `0x471800` `zInput::BindMapCurrent_FormatKeyComboName` -> `src/GameZRecoil/zInput/zInput.cpp:2461`
- `0x471820` `zInput::BindMapCurrent_CopyJoystickButtonName` -> `src/GameZRecoil/zInput/zInput.cpp:2479`
- `0x471840` `zInput::BindMapCurrent_CopyMouseButtonName` -> `src/GameZRecoil/zInput/zInput.cpp:2497`
- `0x471860` `zInput::BindMapContext_Push` -> `src/GameZRecoil/zInput/zInput.cpp:2063`
- `0x471950` `zInput::BindMapContext_Pop` -> `src/GameZRecoil/zInput/zInput.cpp:2108`
- `0x4719e0` `zInput::GlobalStateStaticInitAndRegisterAtExit` -> `src/GameZRecoil/zInput/zInput.cpp:2558`
- `0x4719f0` `zInput::GlobalStateStaticInit` -> `src/GameZRecoil/zInput/zInput.cpp:2543`
- `0x471a00` `zInput::GlobalStateRegisterAtExit` -> `src/GameZRecoil/zInput/zInput.cpp:2553`
- `0x471a10` `zInput::GlobalStateAtExitDestructor` -> `src/GameZRecoil/zInput/zInput.cpp:2548`
- `0x471a20` `zInput_GlobalState::Destructor` -> `src/GameZRecoil/zInput/zInput.cpp:2529`
- `0x471ab0` `zInput_GlobalState::Constructor` -> `src/GameZRecoil/zInput/zInput.cpp:2516`
- `0x471ae0` `zInput::OnAppDeactivate` -> `src/GameZRecoil/zInput/zInput.cpp:4192`
- `0x471b20` `zInput::OnAppActivate` -> `src/GameZRecoil/zInput/zInput.cpp:4179`
- `0x471b50` `zInput::Init` -> `src/GameZRecoil/zInput/zInput.cpp:2564`
- `0x471c10` `zInput::Shutdown` -> `src/GameZRecoil/zInput/zInput.cpp:2632`
- `0x471c50` `zInput::ResetAllTransitionState` -> `src/GameZRecoil/zInput/zInput.cpp:4156`
- `0x471c60` `zInput::Mouse_IsUnsuspended` -> `src/GameZRecoil/zInput/zInput.cpp:3981`
- `0x471c70` `zInput::Joystick_IsUnsuspended` -> `src/GameZRecoil/zInput/zInput.cpp:3986`
- `0x471c80` `zInput_Keyboard_IsUnsuspended` -> `src/GameZRecoil/zInput/zInput.cpp:97`
- `0x471c90` `zInput::Mouse_ResumeFromSuspend` -> `src/GameZRecoil/zInput/zInput.cpp:4011`
- `0x471cb0` `zInput::Joystick_ResumeFromSuspend` -> `src/GameZRecoil/zInput/zInput.cpp:4170`
- `0x471cd0` `zInput::Keyboard_ResumeFromSuspend` -> `src/GameZRecoil/zInput/zInput.cpp:4120`
- `0x471cf0` `zInput::Mouse_Suspend` -> `src/GameZRecoil/zInput/zInput.cpp:3996`
- `0x471d00` `zInput::Joystick_Suspend` -> `src/GameZRecoil/zInput/zInput.cpp:4001`
- `0x471d10` `zInput::Keyboard_Suspend` -> `src/GameZRecoil/zInput/zInput.cpp:4006`
- `0x471d20` `zInput::Keyboard_AddRef` -> `src/GameZRecoil/zInput/zInput.cpp:3158`
- `0x471d50` `zInput::DI_AddJoystickRef` -> `src/GameZRecoil/zInput/zInput.cpp:3343`
- `0x471d80` `zInput::DI_ReleaseJoystickRef` -> `src/GameZRecoil/zInput/zInput.cpp:3355`
- `0x471da0` `zInput::Mouse_AddRef` -> `src/GameZRecoil/zInput/zInput.cpp:3331`
- `0x471dd0` `zInput::DI_GetJoystickRefCount` -> `src/GameZRecoil/zInput/zInput.cpp:3366`
- `0x471de0` `zInput::PollActiveDevices` -> `src/GameZRecoil/zInput/zInput.cpp:3812`
- `0x471e40` `zInput::DI_InitJoystickDevice` -> `src/GameZRecoil/zInput/zInput.cpp:3407`
- `0x471f60` `zInput::DI_EnumDevicesCallback_SelectFirstJoystick` -> `src/GameZRecoil/zInput/zInput.cpp:3371`
- `0x471fb0` `zInput::DI_AcquireJoystickDevice` -> `src/GameZRecoil/zInput/zInput.cpp:3395`
- `0x471fd0` `zInput::DI_ApplyAxisConfig` -> `src/GameZRecoil/zInput/zInput.cpp:3582`
- `0x4721a0` `zInput::DI_SetAxisDeadzone` -> `src/GameZRecoil/zInput/zInput.cpp:3472`
- `0x4721e0` `zInput::DI_SetAxisRange` -> `src/GameZRecoil/zInput/zInput.cpp:3496`
- `0x472230` `zInput::DI_GetAxisRange` -> `src/GameZRecoil/zInput/zInput.cpp:3523`
- `0x472280` `zInput::Joystick_ShutdownDevice` -> `src/GameZRecoil/zInput/zInput.cpp:2605`
- `0x4722b0` `zInput::DI_IsJoystickDeviceReady` -> `src/GameZRecoil/zInput/zInput.cpp:3624`
- `0x4722c0` `zInput::DI_PollJoystickState` -> `src/GameZRecoil/zInput/zInput.cpp:3639`
- `0x472390` `zInput::DI_GetCurrentState` -> `src/GameZRecoil/zInput/zInput.cpp:3630`
- `0x4723a0` `zInput::DI_GetButtonTransitionState` -> `src/GameZRecoil/zInput/zInput.cpp:3689`
- `0x4723d0` `zInput::DI_WaitForButtonPress` -> `src/GameZRecoil/zInput/zInput.cpp:3704`
- `0x472410` `zInput::DI_ResetTransitionState` -> `src/GameZRecoil/zInput/zInput.cpp:4130`
- `0x472450` `zInput_DI_CreateForceFeedbackEffect` -> `src/GameZRecoil/zInput/zInput.cpp:790`
- `0x472480` `zInput_DI_HasForceFeedback` -> `src/GameZRecoil/zInput/zInput.cpp:776`
- `0x472490` `zInput::DI_ReportError` -> `src/GameZRecoil/zInput/zInput.cpp:1361`
- `0x4727a0` `zMath_Vec3_DivScalar` -> `src/GameZRecoil/zMath/zmth_main.c:129`
- `0x472f30` `zMath::MatStackPushPtr` -> `src/GameZRecoil/zMath/zMath.cpp:493`
- `0x472f60` `zMath::MatStackPopPtr` -> `src/GameZRecoil/zMath/zMath.cpp:518`
- `0x472f90` `zMath::MatLoadCameraScratchB` -> `src/GameZRecoil/zMath/zMath.cpp:524`
- `0x472fa0` `zMath::MatLoadCameraScratchA` -> `src/GameZRecoil/zMath/zMath.cpp:529`
- `0x472fb0` `zMath_Mat_LoadProjection` -> `src/GameZRecoil/zMath/zMath.cpp:1587`
- `0x473060` `zMath_Mat_LoadView` -> `src/GameZRecoil/zMath/zMath.cpp:1667`
- `0x4731f0` `zMath_Mat_SetupCamera` -> `src/GameZRecoil/zMath/zMath.cpp:1578`
- `0x473210` `zMath::MatCopyCurrentTo` -> `src/GameZRecoil/zMath/zMath.cpp:928`
- `0x473230` `zMath_Mat_GetCurrent` -> `src/GameZRecoil/zMath/zMath.cpp:1745`
- `0x473240` `zMath_Mat_IsCurrentIdentity` -> `src/GameZRecoil/zMath/zMath.cpp:1750`
- `0x473250` `zMath::MatLoadCurrentFrom` -> `src/GameZRecoil/zMath/zMath.cpp:940`
- `0x4732f0` `zMath::MatLoadIdentity` -> `src/GameZRecoil/zMath/zMath.cpp:534`
- `0x473370` `zMath::MatMultiply` -> `src/GameZRecoil/zMath/zMath.cpp:972`
- `0x473b10` `zMath::MatRotateY` -> `src/GameZRecoil/zMath/zMath.cpp:1070`
- `0x473e60` `zMath_Camera_StageInverseRotation` -> `src/GameZRecoil/zMath/zMath.cpp:1842`
- `0x473fc0` `zMath::Vec3ArrayProjectToCachedY` -> `src/GameZRecoil/zMath/zMath.cpp:1263`
- `0x474010` `zMath::MatApplyLocalTRS` -> `src/GameZRecoil/zMath/zMath.cpp:1148`
- `0x4744f0` `zMath_Vec3Array_AddScaled` -> `src/GameZRecoil/zMath/zMath.cpp:2024`
- `0x4747d0` `zMath::MatTransformPointBatchInPlace` -> `src/GameZRecoil/zMath/zMath.cpp:1311`
- `0x474870` `zMath_Mat_TransformBBoxToCorners` -> `src/GameZRecoil/zMath/zMath.cpp:2136`
- `0x474b20` `zMath::ProjectPointBatch` -> `src/GameZRecoil/zMath/zMath.cpp:1332`
- `0x474b70` `zMath_ProjectSphereBatch` -> `src/GameZRecoil/zMath/zMath.cpp:2110`
- `0x474bc0` `zMath_UnprojectPointBatch` -> `src/GameZRecoil/zMath/zMath.cpp:1621`
- `0x474bc0` `zMath_UnprojectPointBatch` -> `src/GameZRecoil/zMath/zmth_main.c:852`
- `0x474c20` `zMath_UnprojectPointBatchZBuf` -> `src/GameZRecoil/zMath/zMath.cpp:1639`
- `0x474d10` `zMath::Vec3DirectionAnglesBetweenPoints` -> `src/GameZRecoil/zMath/zMath.cpp:1242`
- `0x474de0` `zMath_Mat_ExtractYaw` -> `src/GameZRecoil/zMath/zMath.cpp:1755`
- `0x474e10` `zMath_Mat_ExtractEulerAngles` -> `src/GameZRecoil/zMath/zMath.cpp:1769`
- `0x474ec0` `zMath_Vec3_RotateX` -> `src/GameZRecoil/zMath/zMath.cpp:1810`
- `0x4753e0` `zMath_BuildPerspectiveTextureInterpolants` -> `src/GameZRecoil/zMath/zMath.cpp:384`
- `0x4757c0` `zMath_Quat_FromEuler` -> `src/GameZRecoil/zMath/zMath.cpp:1898`
- `0x4757c0` `zMath_Quat_FromEuler` -> `src/GameZRecoil/zMath/zmth_main.c:1077`
- `0x4759d0` `zMath_Quat_MultiplyInverse` -> `src/GameZRecoil/zMath/zMath.cpp:1944`
- `0x4759d0` `zMath_Quat_MultiplyInverse` -> `src/GameZRecoil/zMath/zmth_main.c:1123`
- `0x475a80` `zMath_Quat_ToMatrix` -> `src/GameZRecoil/zMath/zMath.cpp:1964`
- `0x475a80` `zMath_Quat_ToMatrix` -> `src/GameZRecoil/zMath/zmth_main.c:1141`
- `0x475c40` `zModel_Display_Init` -> `src/GameZRecoil/zModel/zModel_Display.cpp:366`
- `0x475e60` `zModel_Display::ShutdownThunk` -> `src/GameZRecoil/zModel/zModel_Display.cpp:1049`
- `0x475e70` `zModel::Init` -> `src/GameZRecoil/zModel/zModel.cpp:1947`
- `0x475f60` `zModel_Display::Reset` -> `src/GameZRecoil/zModel/zModel_Display.cpp:1015`
- `0x475fa0` `zModel_Display::Shutdown` -> `src/GameZRecoil/zModel/zModel_Display.cpp:1030`
- `0x475ff0` `zModel::SetDisplayInstancePoolCapacity` -> `src/GameZRecoil/zModel/zModel.cpp:2004`
- `0x476020` `zModel::SetSoftwarePathActive` -> `src/GameZRecoil/zModel/zModel.cpp:2026`
- `0x476030` `zModel::SetVertexShadingEnabled` -> `src/GameZRecoil/zModel/zModel.cpp:1993`
- `0x476040` `zModel_FogTargetColorOverride_SetCurrent` -> `src/GameZRecoil/zModel/gmod_light.c:337`
- `0x476070` `zModel_RenderAlphaScale_SetCurrent` -> `src/GameZRecoil/zModel/gmod_light.c:353`
- `0x476080` `zModel_RenderVertexAlphaEnabled_SetCurrent` -> `src/GameZRecoil/zModel/gmod_light.c:364`
- `0x476090` `zModel::SetTextureWorldPerMeter` -> `src/GameZRecoil/zModel/zModel.cpp:2039`
- `0x4760b0` `zModel::SetTextureWorldBase` -> `src/GameZRecoil/zModel/zModel.cpp:2052`
- `0x4760d0` `zModel::SetDiTextureWorldPerMeter` -> `src/GameZRecoil/zModel/zModel.cpp:2065`
- `0x476120` `zClipAlt::SetSourceRect` -> `src/GameZRecoil/zGeometry/zClipAlt.cpp:46`
- `0x476170` `zModel_Fog_SetEnabled` -> `src/GameZRecoil/zModel/gmod_light.c:202`
- `0x476180` `zModel_Fog_IsEnabled` -> `src/GameZRecoil/zModel/gmod_light.c:213`
- `0x476190` `zModel_Fog_SetDistanceStart` -> `src/GameZRecoil/zModel/gmod_light.c:222`
- `0x4761d0` `zModel_Fog_GetDistanceStart` -> `src/GameZRecoil/zModel/gmod_light.c:236`
- `0x4761e0` `zModel_Fog_SetDistanceEnd` -> `src/GameZRecoil/zModel/gmod_light.c:245`
- `0x476220` `zModel_Fog_SetHeightHigh` -> `src/GameZRecoil/zModel/gmod_light.c:259`
- `0x476260` `zModel_Fog_SetHeightLow` -> `src/GameZRecoil/zModel/gmod_light.c:273`
- `0x4762a0` `zModel_Fog_SetDensity` -> `src/GameZRecoil/zModel/gmod_light.c:287`
- `0x4762b0` `zModel_Fog_SetLinearModeEnabled` -> `src/GameZRecoil/zModel/gmod_light.c:298`
- `0x4762c0` `zModel_Fog_SetColorRgb01` -> `src/GameZRecoil/zModel/gmod_light.c:309`
- `0x4762f0` `zModel_Fog_ApplyCurrentColor` -> `src/GameZRecoil/zModel/gmod_light.c:328`
- `0x476300` `zRndr::SetInverseZTolerance` -> `src/GameZRecoil/zModel/zModel_Display.cpp:227`
- `0x476320` `zTag4::Clear` -> `src/GameZRecoil/zModel/zModel_Display.cpp:1061`
- `0x476340` `zDi::SetVariantTagIfUnset` -> `src/GameZRecoil/zModel/gmod_matl.c:23`
- `0x476460` `zModel::SetBackfaceEliminationToleranceScalar` -> `src/GameZRecoil/zModel/zModel_Display.cpp:205`
- `0x476470` `zModel::GetBackfaceEliminationToleranceScalar` -> `src/GameZRecoil/zModel/zModel_Display.cpp:216`
- `0x4766a0` `zClipAlt::RemapPointXYInPlace` -> `src/GameZRecoil/zGeometry/zClipAlt.cpp:104`
- `0x476700` `zScene::TestProjectedSphereVisible` -> `src/GameZRecoil/zModel/zModel_Display.cpp:243`
- `0x476a50` `zDi::EvalBoundingSphereLightingFlags` -> `src/GameZRecoil/zModel/zModel.cpp:3033`
- `0x476cf0` `zModel::RenderNodeSoftware` -> `src/GameZRecoil/zModel/zModel.cpp:2092`
- `0x477b30` `zModel::RenderNodeHardware` -> `src/GameZRecoil/zModel/zModel.cpp:2470`
- `0x478c70` `zVideo_FrustumTestSphereClipMask` -> `src/GameZRecoil/zVideo/zVideo.cpp:992`
- `0x478fc0` `zModel_Instance_UpdateScrollingTexturesIfNeeded` -> `src/GameZRecoil/zModel/zModel.cpp:1911`
- `0x479020` `zModel_RenderPointQueueEntry` -> `src/GameZRecoil/zModel/zModel.cpp:2688`
- `0x4791c0` `zModel_Instance_UpdateScrollingTextures` -> `src/GameZRecoil/zModel/zModel.cpp:1830`
- `0x479660` `OptCatalog::ApplyDamageMaskStampOnHit` -> `src/GameZRecoil/zModel/zModel_Display.cpp:494`
- `0x479c50` `OptCatalog::SetDamageMaskSlotIndex` -> `src/GameZRecoil/zModel/zModel_Display.cpp:471`
- `0x479c60` `OptCatalog::RegisterDamageMaskSlotPtr` -> `src/GameZRecoil/zModel/zModel_Display.cpp:482`
- `0x479c80` `OptCatalog_IsDamageMaskEnabled` -> `src/GameZRecoil/zModel/zModel_Display.cpp:461`
- `0x479c90` `OptCatalog_SetDamageMaskUv` -> `src/GameZRecoil/zModel/zModel_Display.cpp:448`
- `0x479cb0` `OptCatalog_SetDamageMaskEnabled` -> `src/GameZRecoil/zModel/zModel_Display.cpp:642`
- `0x479cc0` `OptCatalog_IsDamageMaskSlotPtrRegistered` -> `src/GameZRecoil/zModel/zModel_Display.cpp:653`
- `0x479f90` `zClipAlt::SetTargetRect` -> `src/GameZRecoil/zGeometry/zClipAlt.cpp:64`
- `0x47a1d0` `zClipAlt_BuildFrustumPlanes` -> `src/GameZRecoil/zGeometry/zClipAlt.cpp:26`
- `0x47a200` `zClipRect::ClipPolyZRange_NoUV` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1515`
- `0x47a4e0` `zClipRect::ClipPolyZRange_NoUV_WithAttribs` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1603`
- `0x47aa80` `zClipRect::ClipPolyNearZ` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1286`
- `0x47af60` `zClipRect::ClipPolyNearZ_WithAttr0` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1392`
- `0x47b540` `zClipRect::ClipPoly_NoUV_Alt` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1900`
- `0x47bd30` `zClipRect::ClipPoly_NoUV_WithAttr012_Alt` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1955`
- `0x47cdc0` `zClipRect::ClipPoly_NoUV` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1911`
- `0x47d3f0` `zClipRect::ClipPoly` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1922`
- `0x47dfb0` `zClipRect::ClipPoly_NoUV_WithAttr0_Alt` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1944`
- `0x47e900` `zClipRect::ClipPolyZRange_WithAttr012` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1743`
- `0x47efd0` `zClipRect::ClipPoly_WithAttr012` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1933`
- `0x4803b0` `zClipRect::TrivialRejectPolyXY` -> `src/GameZRecoil/zGeometry/zClipRect.cpp:1966`
- `0x4804c0` `zModel::UpdateSmallPolyRejectThresholds` -> `src/GameZRecoil/zModel/gmod_scene.c:8`
- `0x4804e0` `zReader::FindGlobalStringPrefixIndex` -> `src/GameZRecoil/zReader/zreader_lookup.cpp:67`
- `0x4805b0` `zModel_MatlSlot::IndexFromPtrOrMinus1` -> `src/GameZRecoil/zModel/gmod_matl.c:49`
- `0x4805e0` `zModel_Matl::GetPoolEntry` -> `src/GameZRecoil/zModel/zModel_Display.cpp:998`
- `0x480600` `zModel_MatlBuffer::WriteGameZ` -> `src/GameZRecoil/zModel/gmod_matl.c:99`
- `0x4808c0` `zModel_MatlBuffer::ReadGameZ` -> `src/GameZRecoil/zModel/gmod_matl.c:266`
- `0x480ae0` `zModel_Matl::InitGlobals` -> `src/GameZRecoil/zModel/zModel_Display.cpp:965`
- `0x480bf0` `zModel_MatlBuffer::SetArraySize` -> `src/GameZRecoil/zModel/gmod_matl.c:66`
- `0x480c40` `zModel_Material::ResetDefaults` -> `src/GameZRecoil/zModel/gdi.c:603`
- `0x480c80` `zModel_Material::HasAuxData` -> `src/GameZRecoil/zModel/gdi.c:622`
- `0x480ca0` `zModel_Material::FindOrClone` -> `src/GameZRecoil/zModel/gdi.c:698`
- `0x480d20` `zModel_Material::CompareForReuse` -> `src/GameZRecoil/zModel/gdi.c:634`
- `0x480d80` `zModel_MatlBuffer::ReleaseAllActive` -> `src/GameZRecoil/zModel/zModel_Display.cpp:894`
- `0x480dc0` `zModel_MatlSlot::Release` -> `src/GameZRecoil/zModel/zModel_Display.cpp:835`
- `0x480ec0` `zRndr::GlobalStringTable_ReleaseDynamicEntries` -> `src/GameZRecoil/zModel/zModel_Display.cpp:671`
- `0x480f10` `zModel_MatlBuffer::Shutdown` -> `src/GameZRecoil/zModel/zModel_Display.cpp:942`
- `0x480f60` `zModel_Material_SetFlagBit9` -> `src/GameZRecoil/zModel/gdi.c:1010`
- `0x480f80` `zModel_Material::InvalidateImagesIfEligible` -> `src/GameZRecoil/zModel/gdi.c:1028`
- `0x480fd0` `zModel_MatlBuffer::ReleaseTextureSurfaces` -> `src/GameZRecoil/zModel/zModel_Display.cpp:912`
- `0x481040` `zModel_Material::SetUserTag` -> `src/GameZRecoil/zModel/gdi.c:731`
- `0x481050` `zModel_Material::SetCycleTextureCount` -> `src/GameZRecoil/zModel/gdi.c:748`
- `0x481100` `zModel_Material::AddCycleTexture` -> `src/GameZRecoil/zModel/gdi.c:791`
- `0x481140` `zModel_Material::UpdateCycleIfNeeded` -> `src/GameZRecoil/zModel/gdi.c:821`
- `0x481220` `zModel_Material::SetCycleTextureLoop` -> `src/GameZRecoil/zModel/gdi.c:860`
- `0x481260` `zModel_Material::SetCycleTextureSpeed` -> `src/GameZRecoil/zModel/gdi.c:889`
- `0x4812b0` `zModel_Material::Clone` -> `src/GameZRecoil/zModel/gdi.c:998`
- `0x4812c0` `zModel_MatlBuffer::CloneToActiveSlot` -> `src/GameZRecoil/zModel/gdi.c:924`
- `0x481420` `zModel_Material::FindByTexDirEntry` -> `src/GameZRecoil/zModel/gdi.c:673`
- `0x481460` `zRndr_GlobalStringTable::LoadDynamicEntriesFromPath` -> `src/GameZRecoil/zModel/zModel_Display.cpp:687`
- `0x481530` `zModel_Const::GetVertexMergeEpsilon` -> `src/GameZRecoil/zModel/gmod_const.c:61`
- `0x481540` `zModel_Const::SetVertexMergeEpsilon` -> `src/GameZRecoil/zModel/gmod_const.c:70`
- `0x481550` `zModel_Const::SetCoplanarTolerance` -> `src/GameZRecoil/zModel/gmod_const.c:89`
- `0x481560` `zModel_Const::SetColinearTolerance` -> `src/GameZRecoil/zModel/gmod_const.c:98`
- `0x481570` `zDi::PtrToIndexOrMinus1` -> `src/GameZRecoil/zModel/zModel_Display.cpp:742`
- `0x4815a0` `zDi::IndexToPtrOrNull` -> `src/GameZRecoil/zModel/zModel_Display.cpp:757`
- `0x4815c0` `zModel_DiPool::WriteToStream` -> `src/GameZRecoil/zModel/gmod_const.c:109`
- `0x481aa0` `zModel_DiPool::ReadEntryByIndexFromStream` -> `src/GameZRecoil/zModel/gmod_const.c:595`
- `0x481bc0` `zModel_DiPool::ReadHeaderFromStream` -> `src/GameZRecoil/zModel/gmod_const.c:346`
- `0x481c50` `zModel_DiPool::ReadEntryDynamicDataFromStream` -> `src/GameZRecoil/zModel/gmod_const.c:399`
- `0x481fa0` `zModel_DiPool::ReadFromStream` -> `src/GameZRecoil/zModel/gmod_const.c:676`
- `0x482080` `zModel_DiPool::AllocFromFreeList` -> `src/GameZRecoil/zModel/zModel_Display.cpp:774`
- `0x4820f0` `zModel_DiPool::FreeIfUnreferenced` -> `src/GameZRecoil/zModel/zModel_Display.cpp:803`
- `0x482160` `zDi::FreeContents` -> `src/GameZRecoil/zModel/gdi.c:173`
- `0x482270` `zDi::CloneToInstance` -> `src/GameZRecoil/zModel/gdi.c:259`
- `0x4826a0` `zUtil::StoreInt32` -> `src/GameZRecoil/zUtil/zutil.cpp:5`
- `0x4826b0` `zDi::SetClonedFlag` -> `src/GameZRecoil/zModel/gdi.c:245`
- `0x4826d0` `zDi::SetFlagBit0` -> `src/GameZRecoil/zModel/gdi.c:231`
- `0x4826f0` `zDi::AddRef` -> `src/GameZRecoil/zModel/gdi.c:144`
- `0x482700` `zDi::Release` -> `src/GameZRecoil/zModel/gdi.c:154`
- `0x482710` `zDi::GetRefCount` -> `src/GameZRecoil/zModel/gdi.c:164`
- `0x482720` `zModel_Const::AddOrMergeVertex` -> `src/GameZRecoil/zModel/zModel.cpp:1520`
- `0x482860` `zModel_Const::AddOrMergeVertexAndNormal` -> `src/GameZRecoil/zModel/zModel.cpp:1566`
- `0x482a10` `zModel_Const::FindOrAppendNormalIndex` -> `src/GameZRecoil/zModel/zModel.cpp:1628`
- `0x482b40` `zModel_Const::RemoveColinearVerticesInPlace` -> `src/GameZRecoil/zModel/zModel.cpp:1377`
- `0x482c60` `zModel_Const::SetNormalizedCrossFromVertexTriplet` -> `src/GameZRecoil/zModel/zModel.cpp:1338`
- `0x482db0` `zModel_Const::IsPolygonCoplanar` -> `src/GameZRecoil/zModel/zModel.cpp:1488`
- `0x482e30` `zModel_Const::ComputePolygonPlaneEquation` -> `src/GameZRecoil/zModel/zModel.cpp:1440`
- `0x482fe0` `zModel_Const::SplitPolygonChunkedByVertexLimit` -> `src/GameZRecoil/zModel/zModel.cpp:1748`
- `0x483240` `zDi::AddPolygonSplitByVertexLimit` -> `src/GameZRecoil/zModel/zModel.cpp:1123`
- `0x483510` `zModel_Const::QuantizeAndNormalizeUvPairs` -> `src/GameZRecoil/zModel/zModel.cpp:1709`
- `0x483610` `zDi::AddPolygon` -> `src/GameZRecoil/zModel/zModel.cpp:1089`
- `0x483650` `zDi::AddPolygonEx` -> `src/GameZRecoil/zModel/zModel.cpp:889`
- `0x483a60` `zDi::HasSpecialFlagsOrAuxMaterialData` -> `src/GameZRecoil/zModel/gdi.c:420`
- `0x483ad0` `zDi::RebuildBounds` -> `src/GameZRecoil/zModel/gdi.c:567`
- `0x483b80` `zDi::BuildAabb` -> `src/GameZRecoil/zModel/gdi.c:443`
- `0x483e60` `zDi::BuildOriginSymmetricAabb` -> `src/GameZRecoil/zModel/gdi.c:509`
- `0x483f80` `zDi::BuildBlendVertsFromConnectivity` -> `src/GameZRecoil/zModel/zModel.cpp:2888`
- `0x484140` `zDi::SetEntryValueForAllEntries` -> `src/GameZRecoil/zModel/zModel.cpp:2755`
- `0x484170` `zDi::SetShowBackFaceForAllEntries` -> `src/GameZRecoil/zModel/zModel.cpp:2773`
- `0x4841b0` `zDi_SetMaterialFlagBit9ForFlagBit0Entries` -> `src/GameZRecoil/zModel/gdi.c:1052`
- `0x4841f0` `zDi::InvalidateImagesForFlagBit8Materials` -> `src/GameZRecoil/zModel/gdi.c:1073`
- `0x484230` `zDi::ResetCurrentVariant` -> `src/GameZRecoil/zModel/zModel.cpp:2789`
- `0x484250` `zDi::SetCurrentVariantCycleTextureCount` -> `src/GameZRecoil/zModel/zModel.cpp:2805`
- `0x4842b0` `zDi::SetCurrentVariant` -> `src/GameZRecoil/zModel/zModel.cpp:2843`
- `0x4842f0` `zModel_Instance::SetCycleTextureLoop` -> `src/GameZRecoil/zModel/zModel.cpp:2992`
- `0x484310` `zDi::SetCurrentVariantCycleTextureSpeed` -> `src/GameZRecoil/zModel/zModel.cpp:2869`
- `0x484330` `zModel_Instance::AddCycleTexture` -> `src/GameZRecoil/zModel/zModel.cpp:3011`
- `0x484350` `zDi::SetObject3DColorModeForMaterials` -> `src/GameZRecoil/zModel/zModel.cpp:2965`
- `0x4843b0` `zDi::RebuildGeneratedUvPairsForEntry` -> `src/GameZRecoil/zModel/zModel.cpp:1229`
- `0x484860` `zModel_Const::SolveTriScalarGradient2D` -> `src/GameZRecoil/zModel/zModel.cpp:1674`
- `0x487a30` `zModel_Light_PointInPolygonInitXZ` -> `src/GameZRecoil/zModel/gmod_light.c:848`
- `0x487c50` `zModel_Light::PointInPolygonTestRadiusXZ` -> `src/GameZRecoil/zModel/gmod_light.c:1295`
- `0x487f10` `zModel_Light::SetActiveLights` -> `src/GameZRecoil/zModel/gmod_light.c:927`
- `0x488d60` `zModel_Light::BuildLightWeights` -> `src/GameZRecoil/zModel/gmod_light.c:651`
- `0x4894f0` `zModel_Light::EvalDistanceWeight` -> `src/GameZRecoil/zModel/gmod_light.c:377`
- `0x489540` `zModel_Light::EvalSphereFogFade` -> `src/GameZRecoil/zModel/gmod_light.c:398`
- `0x4896d0` `zModel_Light::BuildAttr0DepthFade` -> `src/GameZRecoil/zModel/gmod_light.c:438`
- `0x489920` `zModel_Light::EvalBatchSphereFade` -> `src/GameZRecoil/zModel/gmod_light.c:609`
- `0x489a90` `zModel_Light::BuildAttr1Falloff` -> `src/GameZRecoil/zModel/gmod_light.c:526`
- `0x489e10` `zNetwork::ShutdownSessionRuntime` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2325`
- `0x489f30` `zNetwork::ClearEnumeratedSessionList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2163`
- `0x489fa0` `zNetwork::ClearServiceProviderList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2183`
- `0x48a030` `zNetwork::ClearPlayerRecordList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2215`
- `0x48a140` `zNetworkDPlay::InitializeConnectionFromProviderInfo` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:944`
- `0x48a180` `zNetworkDPlay::SelectServiceProviderAndInitConnection` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:904`
- `0x48a220` `zNetwork_DPlay::EnumSessions` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1383`
- `0x48a2c0` `zNetworkDPlay::GetEnumeratedSessionNameByIndex` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:973`
- `0x48a2e0` `zNetworkDPlay::GetEnumeratedSessionPlayerCountsByIndex` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:992`
- `0x48a310` `zNetwork_DPlay::EnumPlayers` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1427`
- `0x48a350` `zNetworkDPlay::QueryCapsAndConfigureSendMode` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1041`
- `0x48a410` `zNetwork_DPlay::CreateSessionFromStatusFields` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1593`
- `0x48a980` `zNetwork_DPlay_DestroyCachedLocalPlayer` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:708`
- `0x48ae70` `zNetworkDPlay::ReceivePendingMessages` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1264`
- `0x48afa0` `zNetwork::GetPlayerNameByKey` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:311`
- `0x48afe0` `zNetworkDPlay::PumpIncomingMessages` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1085`
- `0x48b5e0` `zNetworkDPlay::EnumSessionCallback_AddSessionDescCache` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1012`
- `0x48b660` `zNetworkDPlay::EnumPlayerCallback_AddPlayerRecord` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1338`
- `0x48b730` `zNetwork_DPlay::CreateInterfaceAndCoInitialize` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1942`
- `0x48b7f0` `zNetwork_DPlay::CloseReleaseAndCoUninitialize` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2000`
- `0x48b820` `zNetwork_ApplyPkt01_PlayerColorAssignments` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1918`
- `0x48b860` `zNetwork::HostSendPlayerColorAssignmentsPacket` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:769`
- `0x48b940` `zNetwork::AllocFreePlayerColorIndex` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:752`
- `0x48b9e0` `zNetwork::RemovePlayerRecordByKey` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1228`
- `0x48ba60` `zNetwork_FindPlayerRecordByKey` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:731`
- `0x48bee0` `zNetworkDPlay::FreeServiceProviderInfoBuffers` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2020`
- `0x48bfa0` `zNetwork_InitMessageHandlers` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1909`
- `0x48bfb0` `zNetwork_CreateEmptyDispatchHandlerList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1893`
- `0x48bfe0` `zNetwork_RegisterDispatchHandlerListShutdown` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1885`
- `0x48bff0` `zNetwork_DestroyDispatchHandlerList` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:1859`
- `0x48c120` `zNetwork::UnregisterPacketHandler` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2089`
- `0x48c200` `zNetwork_DPlay::DispatchPacketToHandlers` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:2138`
- `0x48c250` `zNetwork_DPlay_ReportError` -> `src/GameZRecoil/zNetwork/znet_dplay.cpp:509`
- `0x48c7d0` `zUtil::ZRDR_PreallocNodePool` -> `src/GameZRecoil/zReader/zreader_load.cpp:111`
- `0x48c800` `zUtil_ZRDR_GrowFreePool` -> `src/GameZRecoil/zReader/zreader_load.cpp:103`
- `0x48c820` `zUtil_ZRDR_PushFreeNode` -> `src/GameZRecoil/zReader/zreader_load.cpp:77`
- `0x48c890` `zUtil_ZRDR_FreeNodePool` -> `src/GameZRecoil/zReader/zreader_load.cpp:829`
- `0x48c8e0` `zUtil_ZRDR_PopFreeNode` -> `src/GameZRecoil/zReader/zreader_load.cpp:127`
- `0x48c950` `zArchiveList_CreateEmpty` -> `src/GameZRecoil/zReader/zreader_load.cpp:49`
- `0x48c970` `zArchiveList_Destroy` -> `src/GameZRecoil/zReader/zreader_load.cpp:61`
- `0x48c9a0` `zArchiveList_LinkNodeBetween` -> `src/GameZRecoil/zReader/zreader_load.cpp:37`
- `0x48c9c0` `zArchiveList_PushFrontPayload` -> `src/GameZRecoil/zReader/zreader_load.cpp:167`
- `0x48ca10` `zUtil_ZRDR_AllocNodeWithPayload` -> `src/GameZRecoil/zReader/zreader_load.cpp:158`
- `0x48ca30` `zArchiveList_PushBackPayload` -> `src/GameZRecoil/zReader/zreader_load.cpp:195`
- `0x48ca70` `zArchiveList_RemovePayload` -> `src/GameZRecoil/zReader/zreader_load.cpp:223`
- `0x48cae0` `zArchiveList_FreeNode` -> `src/GameZRecoil/zReader/zreader_load.cpp:262`
- `0x48cb00` `zArchiveList_FindNodeByPayload` -> `src/GameZRecoil/zReader/zreader_load.cpp:279`
- `0x48cb30` `zArchiveList_GetAt` -> `src/GameZRecoil/zReader/zreader_load.cpp:344`
- `0x48cb70` `zArchiveList_PopFrontPayload` -> `src/GameZRecoil/zReader/zreader_load.cpp:309`
- `0x48cbd0` `zArchiveList_FindPayloadByPredicate` -> `src/GameZRecoil/zReader/zreader_load.cpp:368`
- `0x48cc20` `zArchiveList_FindPayloadByValue` -> `src/GameZRecoil/zReader/zreader_load.cpp:405`
- `0x48cc50` `zArchiveList_FindPayloadByPredicate_Thunk` -> `src/GameZRecoil/zReader/zreader_load.cpp:435`
- `0x48cc60` `zArchiveList_GetCount` -> `src/GameZRecoil/zReader/zreader_load.cpp:333`
- `0x48cc70` `zUtil::ZRDR_Init` -> `src/GameZRecoil/zReader/zreader_load.cpp:879`
- `0x48cca0` `zUtil_ZRDR_SetSearchPath` -> `src/GameZRecoil/zReader/zreader_load.cpp:845`
- `0x48cce0` `zUtil_ZRDR_AppendSearchPath` -> `src/GameZRecoil/zReader/zreader_load.cpp:862`
- `0x48cd10` `zUtil_ZRDR_Shutdown` -> `src/GameZRecoil/zReader/zreader_load.cpp:819`
- `0x48cd40` `zReader::TryResolvePath` -> `src/GameZRecoil/zReader/zreader_load.cpp:1570`
- `0x48cf80` `zReader::ReadNamedString` -> `src/GameZRecoil/zReader/zreader_lookup.cpp:108`
- `0x48cfb0` `zReader::ReadNamedFloat` -> `src/GameZRecoil/zReader/zreader_lookup.cpp:140`
- `0x48d030` `zReader::ReadNamedInt` -> `src/GameZRecoil/zReader/zreader_lookup.cpp:185`
- `0x48d210` `zArchive::MountIndexArchive` -> `src/GameZRecoil/zReader/zreader_load.cpp:1324`
- `0x48d2c0` `zUtil_ZRDR_UnloadMountedArchives` -> `src/GameZRecoil/zReader/zreader_load.cpp:790`
- `0x48d340` `zVid::Noise_InitBuffers` -> `src/GameZRecoil/zVideo/zVideo.cpp:4375`
- `0x48d3e0` `zVid::Noise_ShutdownBuffers` -> `src/GameZRecoil/zVideo/zVideo.cpp:4401`
- `0x48d420` `zVideo::Fx_SetSurfaceState` -> `src/GameZRecoil/zVideo/zVideo.cpp:2914`
- `0x48d450` `zRndr::OverlayBlendRow555_Scalar` -> `src/GameZRecoil/zRndr/zRndr.cpp:1533`
- `0x48d4b0` `zRndr::OverlayBlendRow565_Scalar` -> `src/GameZRecoil/zRndr/zRndr.cpp:1561`
- `0x48d510` `zRndr::OverlayBlendRow555_Mmx` -> `src/GameZRecoil/zRndr/zRndr.cpp:1586`
- `0x48d5f0` `zRndr::OverlayBlendRow565_Mmx` -> `src/GameZRecoil/zRndr/zRndr.cpp:1623`
- `0x48d6d0` `zRndr_OverlayRect_Submit` -> `src/GameZRecoil/zRndr/zRndr.cpp:8092`
- `0x48d7a0` `zRndr_OverlayRect_FlushSw` -> `src/GameZRecoil/zRndr/zRndr.cpp:8136`
- `0x48da60` `zVideo::FxPass3_CopySurfacePixelToScratchClipped` -> `src/GameZRecoil/zVideo/zVideo.cpp:2930`
- `0x48daf0` `zVideo::FxPass3_ApplyToCurrentSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:3169`
- `0x48e380` `zVideo::buff_BlurRegionCombined` -> `src/GameZRecoil/zVideo/zVideo.cpp:3315`
- `0x48e670` `zVideo::buff_BlurRegionVertical` -> `src/GameZRecoil/zVideo/zVideo.cpp:3460`
- `0x48e870` `zVideo::buff_BlurRegionHorizontal` -> `src/GameZRecoil/zVideo/zVideo.cpp:3567`
- `0x48ea00` `zVideo::buff_BlurRegionByMode` -> `src/GameZRecoil/zVideo/zVideo.cpp:3665`
- `0x48f500` `zVid_Image::BlitToActiveTarget` -> `src/GameZRecoil/zVideo/zVideo.cpp:5059`
- `0x48f560` `zVid_Image::BlitToFramebufferClipped` -> `src/GameZRecoil/zVideo/zVideo.cpp:5091`
- `0x48fd80` `zRndr::InitGlobals` -> `src/GameZRecoil/zRndr/zRndr.cpp:1461`
- `0x48ff80` `zRndr::SelectSpanRoutines` -> `src/GameZRecoil/zRndr/zRndr.cpp:1701`
- `0x490330` `zFloat::Set255f` -> `src/GameZRecoil/zMath/zMath.cpp:2127`
- `0x490340` `zRndr::SetFrameBufferRegion` -> `src/GameZRecoil/zRndr/zRndr.cpp:1839`
- `0x4903c0` `zRndr::SetActiveRegionSizeFromRect` -> `src/GameZRecoil/zRndr/zRndr.cpp:1867`
- `0x4903f0` `zRndr::GetActiveRegionState` -> `src/GameZRecoil/zRndr/zRndr.cpp:1819`
- `0x490430` `zRndr::SetPerspectiveTextureDeltaX` -> `src/GameZRecoil/zRndr/zRndr.cpp:1385`
- `0x490480` `zRndr::SetPerspectiveAdaptiveSpanParams` -> `src/GameZRecoil/zRndr/zRndr.cpp:1447`
- `0x4904a0` `zRndr::SetPerspectiveTextureFarZ` -> `src/GameZRecoil/zRndr/zRndr.cpp:1417`
- `0x4904d0` `zRndr::SetPerspectiveAdaptiveCorrection` -> `src/GameZRecoil/zRndr/zRndr.cpp:1430`
- `0x490520` `zRndr::SpanOcclusionInit` -> `src/GameZRecoil/zRndr/zRndr.cpp:1975`
- `0x490590` `zRndr::SpanOcclusionBuildColumnHeadTable` -> `src/GameZRecoil/zRndr/zRndr.cpp:2008`
- `0x490600` `zRndr::SpanOcclusionResetFrame` -> `src/GameZRecoil/zRndr/zRndr.cpp:2364`
- `0x490610` `zRndr::SpanOcclusionSubmitOccluderRect` -> `src/GameZRecoil/zRndr/zRndr.cpp:1932`
- `0x490710` `zRndr::SpanOcclusionAddPolygon` -> `src/GameZRecoil/zRndr/zRndr.cpp:1895`
- `0x490780` `zRndr::SpanOcclusionShutdown` -> `src/GameZRecoil/zRndr/zRndr.cpp:2375`
- `0x4907c0` `zRndr_SpanOcclusion_TestSpanDepthOrderPair` -> `src/GameZRecoil/zRndr/zRndr.cpp:6575`
- `0x490ae0` `zRndr_SpanOcclusion_InsertSpanNode_Local` -> `src/GameZRecoil/zRndr/zRndr.cpp:5630`
- `0x4912a0` `zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest` -> `src/GameZRecoil/zRndr/zRndr.cpp:5852`
- `0x491840` `zRndr_SpanOcclusion_BuildSpanList` -> `src/GameZRecoil/zRndr/zRndr.cpp:5873`
- `0x491da0` `zRndr_SpanOcclusion_BuildSpanListFast` -> `src/GameZRecoil/zRndr/zRndr.cpp:5895`
- `0x491dd0` `zRndr_SpanOcclusion_TestColumnVisibility` -> `src/GameZRecoil/zRndr/zRndr.cpp:5916`
- `0x492000` `zRndr_RasterizePolyWithSpanList` -> `src/GameZRecoil/zRndr/zRndr.cpp:7126`
- `0x4927d0` `zRndr::SpanOcclusionRasterizeOccluderPoly` -> `src/GameZRecoil/zRndr/zRndr.cpp:2045`
- `0x492f00` `zRndr_DrawFlatImmediate` -> `src/GameZRecoil/zRndr/zRndr.cpp:7377`
- `0x4936d0` `zRndr_RasterizePoly` -> `src/GameZRecoil/zRndr/zRndr.cpp:7540`
- `0x493df0` `zRndr_DrawFlatQueued` -> `src/GameZRecoil/zRndr/zRndr.cpp:8510`
- `0x494af0` `Renderer_DrawPolyTLV` -> `src/GameZRecoil/zRndr/zRndr.cpp:8961`
- `0x495850` `zRndr_DrawTexturedQueued` -> `src/GameZRecoil/zRndr/zRndr.cpp:8728`
- `0x4969d0` `zRndr_DrawTexturedQueuedAlpha` -> `src/GameZRecoil/zRndr/zRndr.cpp:9175`
- `0x497ac0` `zRndr_DrawTexturedFanTri` -> `src/GameZRecoil/zRndr/zRndr.cpp:9385`
- `0x498bd0` `zRndr_DrawImmediateLine` -> `src/GameZRecoil/zRndr/zRndr.cpp:9594`
- `0x498c00` `zRndr_DrawClippedImmediateLineStrip` -> `src/GameZRecoil/zRndr/zRndr.cpp:9616`
- `0x498c40` `zRndr_SpanOcclusion_TestPointVisibility` -> `src/GameZRecoil/zRndr/zRndr.cpp:5990`
- `0x498cb0` `zRndr::LensFlare_DrawQueuedSample16_ClippedFramebuffer` -> `src/GameZRecoil/zRndr/zRndr.cpp:5441`
- `0x498f90` `zRndr_SpanOcclusion_TestSample` -> `src/GameZRecoil/zRndr/zRndr.cpp:6021`
- `0x498fb0` `zRndr_DrawCircleOutline16_Framebuffer` -> `src/GameZRecoil/zRndr/zRndr.cpp:6109`
- `0x499020` `zRndr_DrawCircleOctants16_Framebuffer` -> `src/GameZRecoil/zRndr/zRndr.cpp:6044`
- `0x499130` `zRndr_TextureMip_SelectVariantImage` -> `src/GameZRecoil/zRndr/zRndr.cpp:8451`
- `0x4992b0` `zRndr_PlotPixel16` -> `src/GameZRecoil/zRndr/zRndr.cpp:6159`
- `0x4992d0` `zRndr_DrawLine16` -> `src/GameZRecoil/zRndr/zRndr.cpp:6173`
- `0x4993a0` `zRndr_DrawLine16_Segmented` -> `src/GameZRecoil/zRndr/zRndr.cpp:6236`
- `0x499500` `zRndr_DrawLine16_Clipped` -> `src/GameZRecoil/zRndr/zRndr.cpp:6323`
- `0x4997d0` `zRndr_FillSpan16Opaque` -> `src/GameZRecoil/zRndr/zRndr.cpp:6462`
- `0x499810` `zRndr_FillSpan555Solid` -> `src/GameZRecoil/zRndr/zRndr.cpp:6499`
- `0x4998a0` `zRndr_FillSpan565Solid` -> `src/GameZRecoil/zRndr/zRndr.cpp:6537`
- `0x499930` `zRndr_SetPaletteRemapKey` -> `src/GameZRecoil/zRndr/zRndr.cpp:10119`
- `0x499990` `zRndr_SetPaletteRemapKeyFromRgb01` -> `src/GameZRecoil/zRndr/zRndr.cpp:10145`
- `0x499a00` `zRndr_SetPaletteShadeRecipeIndex` -> `src/GameZRecoil/zRndr/zRndr.cpp:10175`
- `0x499a20` `zRndr_SubmitPolyWithSpanList` -> `src/GameZRecoil/zRndr/zRndr.cpp:7687`
- `0x499c40` `zRndr_SubmitTexturedPolyUniformAlphaOrShade` -> `src/GameZRecoil/zRndr/zRndr.cpp:7783`
- `0x499ec0` `zRndr_SubmitTexturedPolyPerVertexAlphaOrShade` -> `src/GameZRecoil/zRndr/zRndr.cpp:7923`
- `0x49a2b0` `zRndr_FlushTransparentQueue` -> `src/GameZRecoil/zRndr/zRndr.cpp:8206`
- `0x49a490` `zRndr_FlushOverwriteQueue` -> `src/GameZRecoil/zRndr/zRndr.cpp:8306`
- `0x49a830` `zRndr_LensFlare_QueueProjectedSample` -> `src/GameZRecoil/zRndr/zRndr.cpp:9649`
- `0x49a8b0` `zRndr_LensFlare_GetQueuedSampleCount` -> `src/GameZRecoil/zRndr/zRndr.cpp:9675`
- `0x49a8c0` `zRndr::LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer` -> `src/GameZRecoil/zRndr/zRndr.cpp:5514`
- `0x49a910` `zRndr::LensFlare_ResetSampleQueue` -> `src/GameZRecoil/zRndr/zRndr.cpp:5358`
- `0x49a920` `zRndr_LensFlare_DrawQueuedSamples16_AndBuildVisibleList` -> `src/GameZRecoil/zRndr/zRndr.cpp:9683`
- `0x49a9c0` `zRndr_LensFlare::BuildVisibleSampleListFromQueue` -> `src/GameZRecoil/zRndr/zRndr.cpp:9725`
- `0x49aa30` `zRndr_SpanOcclusion_FilterSampleList` -> `src/GameZRecoil/zRndr/zRndr.cpp:10051`
- `0x49aa40` `zRndr_LensFlare_SetVisibleSampleStage` -> `src/GameZRecoil/zRndr/zRndr.cpp:9757`
- `0x49aa90` `zRndr_LensFlare_DrawSampleStageClipped` -> `src/GameZRecoil/zRndr/zRndr.cpp:9779`
- `0x49afb0` `zRndr_LensFlare_DrawVisibleSample` -> `src/GameZRecoil/zRndr/zRndr.cpp:10000`
- `0x49b020` `zRndr_LensFlare_DrawVisibleSampleStages` -> `src/GameZRecoil/zRndr/zRndr.cpp:9943`
- `0x49b1a0` `zRndr_LensFlare_DrawVisibleSamples` -> `src/GameZRecoil/zRndr/zRndr.cpp:10035`
- `0x49b1e0` `zRndr::FogColor_SetRgb01Clamped` -> `src/GameZRecoil/zRndr/zRndr.cpp:5203`
- `0x49b350` `zRndr::SetFogTargetColorRgb01Clamped` -> `src/GameZRecoil/zRndr/zRndr.cpp:5253`
- `0x49b4c0` `zRndr::CommitDirectFogParamsIfChanged` -> `src/GameZRecoil/zRndr/zRndr.cpp:5303`
- `0x49b530` `zRndr::CommitFogColorParamsIfChanged` -> `src/GameZRecoil/zRndr/zRndr.cpp:5311`
- `0x49b5a0` `zRndr_FogTargetColorStaged_SetRgb01Clamped` -> `src/GameZRecoil/zRndr/zRndr.cpp:10068`
- `0x49b710` `zRndr::CommitStagedFogParamsIfChanged` -> `src/GameZRecoil/zRndr/zRndr.cpp:5319`
- `0x49b780` `zRndr::BlendPackedColor565WithFogInPlace` -> `src/GameZRecoil/zRndr/zRndr.cpp:5327`
- `0x49b7e0` `zRndr::SpanMasked16FromTex16SwitchVShift` -> `src/GameZRecoil/zRndr/zRndr.cpp:4374`
- `0x49bbf0` `zRndr::SpanMasked16FromPal8SwitchVShift` -> `src/GameZRecoil/zRndr/zRndr.cpp:4873`
- `0x49c020` `zRndr::SpanMasked16FromPal8To565` -> `src/GameZRecoil/zRndr/zRndr.cpp:2505`
- `0x49c150` `zRndr::SpanMasked16FromTex16To565` -> `src/GameZRecoil/zRndr/zRndr.cpp:2565`
- `0x49c230` `zRndr::SpanAlphaBlend565ConstAlphaFromPal8` -> `src/GameZRecoil/zRndr/zRndr.cpp:2461`
- `0x49c360` `zRndr::SpanAlphaBlend565FromTex16Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:2599`
- `0x49c560` `zRndr::SpanAlphaBlend555FromTex16Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:2685`
- `0x49c760` `zRndr::SpanAlphaBlend565ConstAlphaFromTex16` -> `src/GameZRecoil/zRndr/zRndr.cpp:3393`
- `0x49c860` `zRndr::SpanAlphaBlend555ConstAlphaFromTex16` -> `src/GameZRecoil/zRndr/zRndr.cpp:3435`
- `0x49c970` `zRndr::SpanAlphaBlend565ConstAlphaFromTex16Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:2770`
- `0x49ca90` `zRndr::SpanAlphaBlend555ConstAlphaFromTex16Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:2822`
- `0x49cbb0` `zRndr::SpanAlphaBlend565MmxFromTex16Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:2872`
- `0x49cea0` `zRndr::SpanAlphaBlend555MmxFromTex16Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:2935`
- `0x49d1a0` `zRndr::SpanAlphaBlend565FromPal8Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:2997`
- `0x49d3b0` `zRndr::SpanAlphaBlend555FromPal8Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:3079`
- `0x49d5c0` `zRndr::SpanAlphaBlend565ConstAlphaFastFromPal8` -> `src/GameZRecoil/zRndr/zRndr.cpp:3476`
- `0x49d6e0` `zRndr::SpanAlphaBlend555ConstAlphaFastFromPal8` -> `src/GameZRecoil/zRndr/zRndr.cpp:3518`
- `0x49d810` `zRndr::SpanAlphaBlend565ConstAlphaFromPal8Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:3161`
- `0x49d950` `zRndr::SpanAlphaBlend555ConstAlphaFromPal8Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:3214`
- `0x49da80` `zRndr::SpanAlphaBlend565MmxFromPal8Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:3266`
- `0x49ddb0` `zRndr::SpanAlphaBlend555MmxFromPal8Alpha8` -> `src/GameZRecoil/zRndr/zRndr.cpp:3330`
- `0x49e0e0` `zRndr::FogTarget565_SetPackedColorAndRamp` -> `src/GameZRecoil/zRndr/zRndr.cpp:2398`
- `0x49e140` `zRndr::SpanMmxSetPixelFormatMasks` -> `src/GameZRecoil/zRndr/zRndr.cpp:1660`
- `0x49e200` `zRndr::FogBlendSpan565Scalar` -> `src/GameZRecoil/zRndr/zRndr.cpp:3964`
- `0x49e300` `zRndr::FogBlendSpan555Scalar` -> `src/GameZRecoil/zRndr/zRndr.cpp:4004`
- `0x49e400` `zRndr::FogBlendSpan565Mmx` -> `src/GameZRecoil/zRndr/zRndr.cpp:4044`
- `0x49e560` `zRndr::FogBlendSpan555Mmx` -> `src/GameZRecoil/zRndr/zRndr.cpp:4137`
- `0x49e6c0` `zRndr::SpanCopy16FromTex16SwitchVShift` -> `src/GameZRecoil/zRndr/zRndr.cpp:4230`
- `0x49ea40` `zRndr::SpanMmxSetTexUvMasksAndVShift` -> `src/GameZRecoil/zRndr/zRndr.cpp:4560`
- `0x49ea80` `zRndr::SpanCopy16FromTex16` -> `src/GameZRecoil/zRndr/zRndr.cpp:4578`
- `0x49ec20` `zRndr::SpanCopy16FromTex16ExplicitVShift` -> `src/GameZRecoil/zRndr/zRndr.cpp:4646`
- `0x49edc0` `zRndr::SpanCopy16FromPal8SwitchVShift` -> `src/GameZRecoil/zRndr/zRndr.cpp:4713`
- `0x49f180` `zRndr::SpanShade16FromPal8SwitchVShift` -> `src/GameZRecoil/zRndr/zRndr.cpp:5034`
- `0x49f614` `zSnd_TickWrapper` -> `src/GameZRecoil/zSound/zsnd_system.cpp:669`
- `0x49f620` `zSnd::Tick` -> `src/GameZRecoil/zSound/zsnd_system.cpp:618`
- `0x49f6d0` `zSndSample::AcquirePlayHandleDispatch` -> `src/GameZRecoil/zSound/zsnd_play.cpp:292`
- `0x49f6f0` `zSndSample::AcquireA3dVoice` -> `src/GameZRecoil/zSound/zsnd_play.cpp:379`
- `0x49f830` `zSndSample::AcquireVoice` -> `src/GameZRecoil/zSound/zsnd_play.cpp:308`
- `0x49f960` `zSndSample::PlayA3DSimple` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1694`
- `0x49f9a0` `zSnd::GainScaleToDirectSoundAttenuation` -> `src/GameZRecoil/zSound/zsnd_play.cpp:462`
- `0x49fa00` `zSndSample_PlaySimple` -> `src/GameZRecoil/zSound/zsnd_play.cpp:579`
- `0x49fa10` `zSndSample::PlayOnActiveBackend` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1454`
- `0x49fa60` `zSndSample::PlayOnA3D` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1485`
- `0x49fbb0` `zSndSample::PlayOnDirectSound` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1560`
- `0x49fcf0` `zSndSample::PlayA3D` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1628`
- `0x49fd50` `zSndSample::PlayDirectSound` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1664`
- `0x49fda0` `zSndPlayHandle::StopIfActive` -> `src/GameZRecoil/zSound/zsnd_play.cpp:986`
- `0x49fec0` `zSndSample::StopActiveVoicesIfPlaying` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1078`
- `0x49fff0` `zSndPlayHandleSnapshot::CreateFromActiveSamples` -> `src/GameZRecoil/zSound/zsnd_play.cpp:711`
- `0x4a0300` `zSndPlayHandleSnapshotPayload::CaptureFromPlayHandle` -> `src/GameZRecoil/zSound/zsnd_play.cpp:652`
- `0x4a0500` `zSndPlayHandleSnapshot::StopAllIfPlaying` -> `src/GameZRecoil/zSound/zsnd_play.cpp:590`
- `0x4a05f0` `zSndPlayHandleSnapshot::Destroy` -> `src/GameZRecoil/zSound/zsnd_play.cpp:955`
- `0x4a0670` `zSnd::ApplyMuteStateToActiveVoices` -> `src/GameZRecoil/zSound/zsnd_play.cpp:530`
- `0x4a07a0` `zSnd::IsMuted` -> `src/GameZRecoil/zSound/zsnd_play.cpp:482`
- `0x4a07c0` `zSndPlayHandleSnapshot::NewNode` -> `src/GameZRecoil/zSound/zsnd_play.cpp:634`
- `0x4a07f0` `zSnd::SetUseArchiveBanksFlag` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:185`
- `0x4a0800` `zSnd_SetUseArchiveBanksAndRegisterAtExit` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:109`
- `0x4a0810` `zSnd_SetUseArchiveBanks` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:74`
- `0x4a0830` `zSndSampleSetRegistry_RegisterAtExit` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:101`
- `0x4a0840` `zSndSampleSetRegistry_Shutdown` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:90`
- `0x4a0860` `zSndSampleSet_InitByName` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:179`
- `0x4a0870` `zSndSampleSet_DestroyByName` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:169`
- `0x4a0880` `zSndSampleSetRegistry_DestroyAll` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:435`
- `0x4a08d0` `zSndSampleSetRegistry_GetByIndex` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:128`
- `0x4a0900` `zSndSampleSetRegistry_GetCount` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:120`
- `0x4a0920` `zSndSampleSetRegistry_FindByName` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:147`
- `0x4a0990` `zSnd::FindSampleByName` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:453`
- `0x4a09e0` `zSndSampleSet::RegistryAddEntry` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:190`
- `0x4a0c00` `zSndSampleSet::DestroyOwnedData` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:420`
- `0x4a0c40` `zSndSampleSet::Init` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:298`
- `0x4a0e40` `zSndSampleSet::Destroy` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:404`
- `0x4a0e90` `zSndSampleSet::GetSampleAt` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:213`
- `0x4a0ec0` `zSndSampleSet::FindSampleByName` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:227`
- `0x4a0fb0` `zSndSampleSet::LoadSamplesFromIndexArchive` -> `src/GameZRecoil/zSound/zsnd_sample_set.cpp:252`
- `0x4a1090` `zSnd::SetGlobalVolumeScale` -> `src/GameZRecoil/zSound/zsnd_play.cpp:494`
- `0x4a10b0` `zSnd::MulGlobalVolumeScaleAndGetPrev` -> `src/GameZRecoil/zSound/zsnd_play.cpp:508`
- `0x4a10d0` `zSnd::SetFlag10PlaybackEnabled` -> `src/GameZRecoil/zSound/zsnd_play.cpp:521`
- `0x4a10e0` `zSndPlayHandle::SetFreqScaled` -> `src/GameZRecoil/zSound/zsnd_parm.cpp:48`
- `0x4a11d0` `zSndPlayHandle::SetEnableScale` -> `src/GameZRecoil/zSound/zsnd_parm.cpp:102`
- `0x4a1240` `zSndSample::SetPlaybackEventHandler` -> `src/GameZRecoil/zSound/zsnd_parm.cpp:135`
- `0x4a1250` `zSndPlayHandle_TryEnableManaged` -> `src/GameZRecoil/zSound/zsnd_parm.cpp:150`
- `0x4a1270` `zSndPlayHandle_TryDisableManaged` -> `src/GameZRecoil/zSound/zsnd_parm.cpp:168`
- `0x4a1290` `zSnd::SetActiveBackendPreInit` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:137`
- `0x4a12b0` `zSnd::GetActiveBackend` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:149`
- `0x4a12c0` `zSnd_PreInitializeRuntimeState` -> `src/GameZRecoil/zSound/zsnd_init.cpp:627`
- `0x4a13d0` `zSndSystem::Shutdown` -> `src/GameZRecoil/zSound/zsnd_system.cpp:793`
- `0x4a1420` `zSndSystem_Init` -> `src/GameZRecoil/zSound/zsnd_init.cpp:688`
- `0x4a1510` `zSndSystem_InitLegacySetsSyntax` -> `src/GameZRecoil/zSound/zsnd_system.cpp:817`
- `0x4a1870` `zSndSystem_InitNamedSetsSyntax` -> `src/GameZRecoil/zSound/zsnd_system.cpp:679`
- `0x4a1d10` `zSndBackend_InitA3D` -> `src/GameZRecoil/zSound/zsnd_init.cpp:818`
- `0x4a1e50` `zSndBackend_InitDirectSound` -> `src/GameZRecoil/zSound/zsnd_init.cpp:755`
- `0x4a1f40` `zSndBackend::Shutdown` -> `src/GameZRecoil/zSound/zsnd_init.cpp:931`
- `0x4a2010` `zSndCdTrackList::StaticInit` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:129`
- `0x4a2020` `zSndCdTrackList::StaticConstructor` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:97`
- `0x4a2050` `zSndCdTrackList::RegisterAtExitDestructor` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:124`
- `0x4a2060` `zSndCdTrackList::StaticDestructor` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:106`
- `0x4a20d0` `zSndCd::Init` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:233`
- `0x4a2490` `zSndCd::ResetTrackState` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:385`
- `0x4a24d0` `zSndCd::Shutdown` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:641`
- `0x4a25e0` `zSndCd::PlayTrackWithMode` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:581`
- `0x4a2600` `zSndCd::ApplyPlaybackMode` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:490`
- `0x4a26b0` `zSndCd::OnMciNotify` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:595`
- `0x4a26f0` `zSndCd::Stop` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:612`
- `0x4a2750` `zSndCd::PlayTrack` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:549`
- `0x4a27d0` `zSndCd::IsStereoAuxEnabled` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:395`
- `0x4a27f0` `zSndCd::GetVolume` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:411`
- `0x4a2880` `zSndCd::SetVolume` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:454`
- `0x4a2930` `zSndCd::GetTrackCount` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:540`
- `0x4a2950` `zSnd_UpdateListenerState` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1188`
- `0x4a2a30` `zSndPlayHandle::Update3DDispatch` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1257`
- `0x4a2a70` `zSndPlayHandle::Update3D_A3D` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1402`
- `0x4a2b40` `zSndPlayHandle::Update3D` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1285`
- `0x4a2e70` `zSnd_GetSpeedOfSoundMps` -> `src/GameZRecoil/zSound/zsnd_play.cpp:1249`
- `0x4a2e80` `zSnd::SetSpeedOfSoundMps` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:193`
- `0x4a2ea0` `zSndSample::InitFromWaveData` -> `src/GameZRecoil/zSound/zsnd_create.cpp:506`
- `0x4a2ec0` `zSndSample::InitFromWaveData_A3D` -> `src/GameZRecoil/zSound/zsnd_create.cpp:231`
- `0x4a3180` `zSndSample::InitFromWaveData_DirectSound` -> `src/GameZRecoil/zSound/zsnd_create.cpp:37`
- `0x4a34e0` `zSndSample::LockBackendBuffers` -> `src/GameZRecoil/zSound/zsnd_create.cpp:390`
- `0x4a3590` `zSndSample::UnlockBackendBuffers` -> `src/GameZRecoil/zSound/zsnd_create.cpp:454`
- `0x4a3690` `zSndSample::DestroyOwnedData` -> `src/GameZRecoil/zSound/zsnd_play.cpp:232`
- `0x4a3850` `zSndSample_CreateQueuedStreamingSample` -> `src/GameZRecoil/zSound/zsnd_create.cpp:528`
- `0x4a3910` `zSndSample::Destroy` -> `src/GameZRecoil/zSound/zsnd_play.cpp:278`
- `0x4a3930` `zSndFadeLists::Init` -> `src/GameZRecoil/zSound/zsnd_system.cpp:432`
- `0x4a3940` `zSndFadeLists::InitGlobals` -> `src/GameZRecoil/zSound/zsnd_system.cpp:378`
- `0x4a39a0` `zSndFadeLists::RegisterShutdownAtExit` -> `src/GameZRecoil/zSound/zsnd_system.cpp:424`
- `0x4a39b0` `zSndFadeLists::ShutdownAtExit` -> `src/GameZRecoil/zSound/zsnd_system.cpp:389`
- `0x4a3a80` `zSndFadeDispatchList::PushBack` -> `src/GameZRecoil/zSound/zsnd_system.cpp:443`
- `0x4a3ad0` `zSndFadeEntry::UpdateAndQueueCompletion` -> `src/GameZRecoil/zSound/zsnd_system.cpp:467`
- `0x4a3c20` `zSndFadeActiveList::TickAll` -> `src/GameZRecoil/zSound/zsnd_system.cpp:528`
- `0x4a3d20` `zSndFadeLists::StopAllAndShutdown` -> `src/GameZRecoil/zSound/zsnd_system.cpp:742`
- `0x4a3e50` `zSndFadeList::DeleteNodeAndAdvanceCursor` -> `src/GameZRecoil/zSound/zsnd_system.cpp:586`
- `0x4a3e90` `zSndFadeListCursor::PopFrontCursor` -> `src/GameZRecoil/zSound/zsnd_system.cpp:601`
- `0x4a3ea0` `zSnd::ReportMciError` -> `src/GameZRecoil/zSound/zsnd_cd.cpp:204`
- `0x4a3ef0` `zSnd::ReportA3DError` -> `src/GameZRecoil/zSound/zsnd_init.cpp:55`
- `0x4a4330` `zSnd::ReportDirectSoundError` -> `src/GameZRecoil/zSound/zsnd_init.cpp:438`
- `0x4a44c0` `zSndPendingList_FindByName` -> `src/GameZRecoil/zSound/zsnd_group.cpp:221`
- `0x4a44e0` `zSndPendingList_MatchNamePredicate` -> `src/GameZRecoil/zSound/zsnd_group.cpp:206`
- `0x4a4530` `zSndGroup_QueuePendingLoadsFromConfigNode` -> `src/GameZRecoil/zSound/zsnd_group.cpp:916`
- `0x4a4590` `zSndGroup_LoadFromConfigNode` -> `src/GameZRecoil/zSound/zsnd_group.cpp:784`
- `0x4a49b0` `zSndGroup_LoadConfigBlock` -> `src/GameZRecoil/zSound/zsnd_group.cpp:667`
- `0x4a4c40` `zSndStreamMgr::UpdateActiveRequestPredicate` -> `src/GameZRecoil/zSound/zsnd_group.cpp:36`
- `0x4a4cb0` `zSndStreamRequest::StateBeginGroup` -> `src/GameZRecoil/zSound/zsnd_group.cpp:312`
- `0x4a4d10` `zSndGroup::SelectWeightedEntry` -> `src/GameZRecoil/zSound/zsnd_group.cpp:239`
- `0x4a4ea0` `zSndStreamRequest::StatePlayCurrentEntry` -> `src/GameZRecoil/zSound/zsnd_group.cpp:333`
- `0x4a4fd0` `zSndStreamRequest::StateWaitRepeatDelay` -> `src/GameZRecoil/zSound/zsnd_group.cpp:403`
- `0x4a5020` `zSndStreamRequest::StateWaitTerminationDelay` -> `src/GameZRecoil/zSound/zsnd_group.cpp:297`
- `0x4a5050` `zSndStreamMgr::RecycleFinishedRequest` -> `src/GameZRecoil/zSound/zsnd_group.cpp:640`
- `0x4a50a0` `zSndStreamMgr::Shutdown` -> `src/GameZRecoil/zSound/zsnd_group.cpp:519`
- `0x4a51e0` `zSndStreamRequest::MatchRequestPredicate` -> `src/GameZRecoil/zSound/zsnd_group.cpp:21`
- `0x4a51f0` `zSndStreamRequest::StopIfActive` -> `src/GameZRecoil/zSound/zsnd_group.cpp:173`
- `0x4a5220` `zSndStreamRequest_MatchGroupPredicate` -> `src/GameZRecoil/zSound/zsnd_group.cpp:194`
- `0x4a5230` `zSndGroup::QueueStreamRequestSimple` -> `src/GameZRecoil/zSound/zsnd_group.cpp:608`
- `0x4a5250` `zSndGroup::QueueStreamRequest` -> `src/GameZRecoil/zSound/zsnd_group.cpp:543`
- `0x4a5350` `zSndStreamMgr_EnsureInit` -> `src/GameZRecoil/zSound/zsnd_group.cpp:419`
- `0x4a53d0` `zSndGroup::QueueStreamRequestWithWorldPos` -> `src/GameZRecoil/zSound/zsnd_group.cpp:623`
- `0x4a53f0` `zSndWaveData::zSndWaveData` -> `src/GameZRecoil/zSound/zsnd_create.cpp:567`
- `0x4a5440` `zSndWaveData::Destructor` -> `src/GameZRecoil/zSound/zsnd_create.cpp:592`
- `0x4a5460` `zSndWaveData::ParseLoadedWaveFile` -> `src/GameZRecoil/zSound/zsnd_create.cpp:604`
- `0x4a5540` `zSndWaveData::LoadAndParseIfNeeded` -> `src/GameZRecoil/zSound/zsnd_create.cpp:668`
- `0x4a55c0` `zSndWaveData::Reset` -> `src/GameZRecoil/zSound/zsnd_create.cpp:745`
- `0x4a5600` `zSndWaveData::LoadAndParseFromIndexArchiveIfNeeded` -> `src/GameZRecoil/zSound/zsnd_create.cpp:708`
- `0x4a5670` `Time::Reset` -> `src/GameZRecoil/Time/Time.cpp:28`
- `0x4a5780` `RecoilApp::InitStdLogFiles` -> `src/Battlesport/RecoilApp.cpp:2014`
- `0x4a5980` `zSys::ExitProcessWithCleanup` -> `src/GameZRecoil/zSys/zSys.cpp:1126`
- `0x4a59a0` `zVid::SetCachedClientRectUpdateMask` -> `src/GameZRecoil/zVideo/zVideo.cpp:1874`
- `0x4a59b0` `zVid::QueryCachedClientRectUpdateMaskIf3dfx` -> `src/GameZRecoil/zVideo/zVideo.cpp:1120`
- `0x4a59e0` `zSys::FindFileOnDriveType` -> `src/GameZRecoil/zSys/zSys.cpp:86`
- `0x4a5ad0` `zLoc::LoadMessagesDll` -> `src/GameZRecoil/zLoc/zLoc.cpp:13`
- `0x4a5b00` `zLoc::UnloadMessagesDll` -> `src/GameZRecoil/zLoc/zLoc.cpp:34`
- `0x4a5b20` `zLoc::GetMessageId` -> `src/GameZRecoil/zLoc/zLoc.cpp:47`
- `0x4a5b40` `zLoc::ResolveMessageKeyOrFallback` -> `src/GameZRecoil/zLoc/zLoc.cpp:61`
- `0x4a5b60` `zLoc::FormatMessage` -> `src/GameZRecoil/zLoc/zLoc.cpp:76`
- `0x4a5bf0` `zLoc::GetMessageString` -> `src/GameZRecoil/zLoc/zLoc.cpp:118`
- `0x4a5c20` `zReader::FileExists` -> `src/GameZRecoil/zReader/zreader_load.cpp:1558`
- `0x4a5c40` `zReader_FileExists_Wrapper` -> `src/GameZRecoil/zReader/zreader_load.cpp:1725`
- `0x4a5ca0` `zUtil_ZRDR_CreateSearchPathList` -> `src/GameZRecoil/zReader/zreader_load.cpp:715`
- `0x4a5cc0` `zUtil_ZRDR_FreeSearchPathList` -> `src/GameZRecoil/zReader/zreader_load.cpp:749`
- `0x4a5ce0` `zUtil::ZRDR_AddSearchPaths` -> `src/GameZRecoil/zReader/zreader_load.cpp:661`
- `0x4a5da0` `zUtil_ZRDR_StrCmpPredicate` -> `src/GameZRecoil/zReader/zreader_load.cpp:448`
- `0x4a5df0` `zUtil_ZRDR_FreeScratchSearchPathList` -> `src/GameZRecoil/zReader/zreader_load.cpp:760`
- `0x4a5e10` `zUtil_ZRDR_FreePathList` -> `src/GameZRecoil/zReader/zreader_load.cpp:728`
- `0x4a5e50` `zUtil_ZRDR_ResolvePathInSearchPathList` -> `src/GameZRecoil/zReader/zreader_load.cpp:507`
- `0x4a5f20` `zUtil_ZRDR_SearchPathContainsFilePredicate` -> `src/GameZRecoil/zReader/zreader_load.cpp:464`
- `0x4a5f50` `zUtil_ZRDR_OpenFileResolved` -> `src/GameZRecoil/zReader/zreader_load.cpp:564`
- `0x4a6100` `zUtil_ZRDR_ShutdownWildcardPath` -> `src/GameZRecoil/zReader/zreader_load.cpp:769`
- `0x4a6190` `zIndexArchive::Reset` -> `src/GameZRecoil/zReader/zreader_load.cpp:931`
- `0x4a61b0` `zIndexArchive::Destroy` -> `src/GameZRecoil/zReader/zreader_load.cpp:945`
- `0x4a61d0` `zIndexArchive::Init` -> `src/GameZRecoil/zReader/zreader_load.cpp:956`
- `0x4a62b0` `zIndexArchive::CloseAndFreeRecords` -> `src/GameZRecoil/zReader/zreader_load.cpp:1024`
- `0x4a62f0` `zIndexArchive::EnsureCapacity` -> `src/GameZRecoil/zReader/zreader_load.cpp:1166`
- `0x4a6330` `zIndexArchive::FreeRecordsAndReset` -> `src/GameZRecoil/zReader/zreader_load.cpp:1043`
- `0x4a6360` `zIndexArchive::FlushIndexToTail` -> `src/GameZRecoil/zReader/zreader_load.cpp:1061`
- `0x4a63f0` `zIndexArchive::LoadIndexFromTail` -> `src/GameZRecoil/zReader/zreader_load.cpp:1100`
- `0x4a64d0` `zIndexArchive::AddFileRecord` -> `src/GameZRecoil/zReader/zreader_load.cpp:1189`
- `0x4a65d0` `zIndexArchive::FindRecordByNameCI` -> `src/GameZRecoil/zReader/zreader_load.cpp:1249`
- `0x4a6630` `zIndexArchive::OpenFileByName` -> `src/GameZRecoil/zReader/zreader_load.cpp:1266`
- `0x4a6670` `zIndexArchive::ReadFileByName` -> `src/GameZRecoil/zReader/zreader_load.cpp:1289`
- `0x4a66e0` `zVideo::GetDisplayModeBpp` -> `src/GameZRecoil/zVideo/zVideo.cpp:2628`
- `0x4a66f0` `zVideo::Init_ApplyModeIndex` -> `src/GameZRecoil/zVideo/zVideo.cpp:2803`
- `0x4a6710` `zVideo::GetSwSurfacePixels` -> `src/GameZRecoil/zVideo/zVideo.cpp:2532`
- `0x4a6720` `zVideo::GetSwSurfaceWidth` -> `src/GameZRecoil/zVideo/zVideo.cpp:2543`
- `0x4a6730` `zVideo::GetSwSurfaceHeight` -> `src/GameZRecoil/zVideo/zVideo.cpp:2554`
- `0x4a6740` `zVideo::GetSwSurfacePitch` -> `src/GameZRecoil/zVideo/zVideo.cpp:2565`
- `0x4a6750` `zVideo_dd3d::CallClearZBufferRect` -> `src/GameZRecoil/zVideo/zVideo.cpp:6474`
- `0x4a6760` `zVideo::CallClearSwSurfaceAndZBuffer` -> `src/GameZRecoil/zVideo/zVideo.cpp:2833`
- `0x4a67d0` `zVideo::Dispatch_UnlockSwSurfaceState` -> `src/GameZRecoil/zVideo/zVideo.cpp:2898`
- `0x4a67e0` `zVideo::GetSwSurfaceLockedFlag` -> `src/GameZRecoil/zVideo/zVideo.cpp:2576`
- `0x4a67f0` `zVideo::GetPrimarySurfacePixels` -> `src/GameZRecoil/zVideo/zVideo.cpp:2587`
- `0x4a6810` `zVideo::GetPrimarySurfaceHeight` -> `src/GameZRecoil/zVideo/zVideo.cpp:2604`
- `0x4a6820` `zVideo::GetPrimarySurfacePitch` -> `src/GameZRecoil/zVideo/zVideo.cpp:2616`
- `0x4a6830` `zVideo::CallClearPrimarySurfaceAndZBuffer` -> `src/GameZRecoil/zVideo/zVideo.cpp:2848`
- `0x4a6840` `zVideo::RunPostprocessOnPrimaryBuffer` -> `src/GameZRecoil/zVideo/zVideo.cpp:3864`
- `0x4a68d0` `zVideo::Dispatch_UnlockPrimarySurfaceState` -> `src/GameZRecoil/zVideo/zVideo.cpp:2906`
- `0x4a68e0` `zVideo::Dispatch_LockDisplayModeSurfaceState` -> `src/GameZRecoil/zVideo/zVideo.cpp:2882`
- `0x4a68f0` `zVideo::Dispatch_UnlockDisplayModeSurfaceState` -> `src/GameZRecoil/zVideo/zVideo.cpp:2890`
- `0x4a6930` `zVideo_dd::PrepareWindowForMode` -> `src/GameZRecoil/zVideo/zVideo.cpp:9886`
- `0x4a69c0` `zVideo_buff::ClipCoordToRange` -> `src/GameZRecoil/zVideo/zVideo.cpp:2092`
- `0x4a69e0` `zVideo_buff::BltSourceToPrimaryClipped` -> `src/GameZRecoil/zVideo/zVideo.cpp:2222`
- `0x4a6b40` `zVideo::SetRendererTypeAndActivePath` -> `src/GameZRecoil/zVideo/zVideo.cpp:2422`
- `0x4a6b60` `zVideo_dd3d::SetPendingWireframeState` -> `src/GameZRecoil/zVideo/zVideo.cpp:6486`
- `0x4a6b70` `zVideo_dd3d::SetPendingDitherEnable` -> `src/GameZRecoil/zVideo/zVideo.cpp:6500`
- `0x4a6b80` `zVideo::SetClearColorPacked16` -> `src/GameZRecoil/zVideo/zVideo.cpp:561`
- `0x4a6b90` `zVideo::PixelPack_GetRgbBits` -> `src/GameZRecoil/zVideo/zVideo.cpp:4329`
- `0x4a6bb0` `zVideo::PixelPack_GetRgbMasks` -> `src/GameZRecoil/zVideo/zVideo.cpp:4344`
- `0x4a6bd0` `zVideo::PixelPack_GetPackingParams` -> `src/GameZRecoil/zVideo/zVideo.cpp:4358`
- `0x4a6bf0` `zVideo::PixelPack_SetupFromMasks` -> `src/GameZRecoil/zVideo/zVideo.cpp:2346`
- `0x4a6ca0` `zVid_PackColor00RRGGBB` -> `src/GameZRecoil/zVideo/zVideo.cpp:529`
- `0x4a6cf0` `zVid_PackColorRGB` -> `src/GameZRecoil/zVideo/zVideo.cpp:514`
- `0x4a6d40` `zVid_PackColorRgbFloats` -> `src/GameZRecoil/zVideo/zVideo.cpp:543`
- `0x4a6db0` `zVideo::TexturePixelPack_SetupFromMasks` -> `src/GameZRecoil/zVideo/zVideo.cpp:2378`
- `0x4a6e80` `zVideo_buff_CaptureSurfaceToImage` -> `src/GameZRecoil/zVideo/zVideo.cpp:5397`
- `0x4a71c0` `zVideo::SetHalfResAdjustMode` -> `src/GameZRecoil/zVideo/zVideo.cpp:2441`
- `0x4a7200` `zVideo::GetPrimarySurfaceRectScratch` -> `src/GameZRecoil/zVideo/zVideo.cpp:2516`
- `0x4a7220` `zVideo::SetFogColorFromRgb01` -> `src/GameZRecoil/zVideo/zVideo.cpp:4257`
- `0x4a7250` `zVideo_SetPendingFogTargetColorFromRgb01` -> `src/GameZRecoil/zVideo/zVideo.cpp:574`
- `0x4a7300` `zVideo::SetFogTargetColorFromRgb01` -> `src/GameZRecoil/zVideo/zVideo.cpp:4272`
- `0x4a7330` `zVideo::CommitFogColorIfChanged` -> `src/GameZRecoil/zVideo/zVideo.cpp:4287`
- `0x4a73a0` `zVideo::CommitFogTargetColorIfChanged` -> `src/GameZRecoil/zVideo/zVideo.cpp:4308`
- `0x4a7410` `zVid::GetSelectedHwApiDescriptionOrDefault` -> `src/GameZRecoil/zVideo/zVideo.cpp:1887`
- `0x4a7430` `zVid::GetHwApiDescription` -> `src/GameZRecoil/zVideo/zVideo.cpp:1900`
- `0x4a7450` `zVid::GetHwApiDriverName` -> `src/GameZRecoil/zVideo/zVideo.cpp:1907`
- `0x4a7480` `zVid::GetAcceptedDirectDrawDeviceCount` -> `src/GameZRecoil/zVideo/zVideo.cpp:1553`
- `0x4a7490` `zVideo::SelectHwApiDeviceOrFallback` -> `src/GameZRecoil/zVideo/zVideo.cpp:4018`
- `0x4a74d0` `zVideoD3D::SceneEnter` -> `src/GameZRecoil/zVideo/zVideo.cpp:9574`
- `0x4a74f0` `zVideoD3D::SceneLeave` -> `src/GameZRecoil/zVideo/zVideo.cpp:9584`
- `0x4a7520` `zVideo::AtExitReleaseAllInterfacesAndSurfaces` -> `src/GameZRecoil/zVideo/zVideo.cpp:4242`
- `0x4a7530` `zVideo::ModuleInit` -> `src/GameZRecoil/zVideo/zVideo.cpp:4058`
- `0x4a75e0` `zVideo::ReturnSuccessStub` -> `src/GameZRecoil/zVideo/zVideo.cpp:4046`
- `0x4a75f0` `zVideo::InitVideoSystem` -> `src/GameZRecoil/zVideo/zVideo.cpp:4117`
- `0x4a7700` `zVideo::UpdateCachedClientRectScreenCoords` -> `src/GameZRecoil/zVideo/zVideo.cpp:4216`
- `0x4a7740` `zVideo::ShutdownVideoSystem` -> `src/GameZRecoil/zVideo/zVideo.cpp:4196`
- `0x4a7770` `zVideo_RestoreIconicFullscreenWindowIfNeeded` -> `src/GameZRecoil/zVideo/zVideo.cpp:1108`
- `0x4a7990` `zVideo::Init_SetSurfaceGeometryFromModeIndex` -> `src/GameZRecoil/zVideo/zVideo.cpp:2727`
- `0x4a7af0` `zVideo::SetVideoMode` -> `src/GameZRecoil/zVideo/zVideo.cpp:2812`
- `0x4a7b20` `zVideo::ExchangeClearScreenBufferEnabled` -> `src/GameZRecoil/zVideo/zVideo.cpp:2862`
- `0x4a7b30` `zVideo::GetClearScreenBufferEnabled` -> `src/GameZRecoil/zVideo/zVideo.cpp:2874`
- `0x4a7b40` `zVideo_dd::StartupEnumerateAndDefaultSelect` -> `src/GameZRecoil/zVideo/zVideo.cpp:10087`
- `0x4a7b60` `zVideo_dd::PresentDisplayModeSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:10660`
- `0x4a7d20` `zVideo_dd::OpenVideoMode` -> `src/GameZRecoil/zVideo/zVideo.cpp:9936`
- `0x4a7d40` `zVideo_dd::ShutdownVideoSystem` -> `src/GameZRecoil/zVideo/zVideo.cpp:10104`
- `0x4a7d70` `zVideo_dd::FlipToGDIIfAttached` -> `src/GameZRecoil/zVideo/zVideo.cpp:11092`
- `0x4a7d90` `zVideo_dd::BltSwToPrimaryRectDirect` -> `src/GameZRecoil/zVideo/zVideo.cpp:10617`
- `0x4a7dd0` `zVideo_dd::BltPrimaryToSwRectDirect` -> `src/GameZRecoil/zVideo/zVideo.cpp:10638`
- `0x4a7e10` `zVideo_dd::BltSwToPrimaryRect` -> `src/GameZRecoil/zVideo/zVideo.cpp:10768`
- `0x4a7fc0` `zVideo_dd::LockSurfaceState` -> `src/GameZRecoil/zVideo/zVideo.cpp:10284`
- `0x4a8030` `zVideo_dd::UnlockSurfaceState` -> `src/GameZRecoil/zVideo/zVideo.cpp:10318`
- `0x4a8060` `zVideo_dd::LockDirectDrawSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:10124`
- `0x4a80c0` `zVideo_dd::UnlockDirectDrawSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:10173`
- `0x4a8100` `zVideo_dd::LockSurface_WaitRestore` -> `src/GameZRecoil/zVideo/zVideo.cpp:10209`
- `0x4a8160` `zVideo_dd::UnlockSurface_WaitRestore` -> `src/GameZRecoil/zVideo/zVideo.cpp:10253`
- `0x4a81a0` `zVideo_dd::ZBuffer_DepthFillRect` -> `src/GameZRecoil/zVideo/zVideo.cpp:10888`
- `0x4a8220` `zVideo_dd::ClearScreenAndZBufferRect` -> `src/GameZRecoil/zVideo/zVideo.cpp:10936`
- `0x4a82f0` `zVideo_dd::ClearSwBackbufferAndZBufferRects` -> `src/GameZRecoil/zVideo/zVideo.cpp:11014`
- `0x4a83d0` `zVideo_dd::Image_LazyCreateBackingSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:10339`
- `0x4a84c0` `zVideo_dd::Image_LazyCreateVideoMemorySurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:10486`
- `0x4a8500` `zVideo_dd::Image_PopulateSurfaceFromHeapPixels` -> `src/GameZRecoil/zVideo/zVideo.cpp:10394`
- `0x4a8650` `zVideo_dd::Image_EnsureSurfaceForCurrentDevice` -> `src/GameZRecoil/zVideo/zVideo.cpp:10516`
- `0x4a8680` `zVideo_dd::Image_UploadPixelsToSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:10539`
- `0x4a86f0` `zVideo_dd::Image_ReleaseSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:10587`
- `0x4a8720` `zVideo_dd::SetDisplayMode` -> `src/GameZRecoil/zVideo/zVideo.cpp:11104`
- `0x4a8790` `zVideo_dd::SetVideoMode` -> `src/GameZRecoil/zVideo/zVideo.cpp:11148`
- `0x4a8800` `zVideo_dd::CreateDirectDraw2ForSelectedDevice` -> `src/GameZRecoil/zVideo/zVideo.cpp:9983`
- `0x4a8870` `zVideo::CommitHwApiDeviceSelection` -> `src/GameZRecoil/zVideo/zVideo.cpp:3995`
- `0x4a88b0` `zVideo_dd::CreateSurface3FromDesc` -> `src/GameZRecoil/zVideo/zVideo.cpp:11342`
- `0x4a88f0` `zVideo_dd::CreateFullscreenSurfacesForRenderer` -> `src/GameZRecoil/zVideo/zVideo.cpp:11380`
- `0x4a8920` `zVideo_dd::CreateHalfResBackbufferSurfaces` -> `src/GameZRecoil/zVideo/zVideo.cpp:11401`
- `0x4a8b20` `zVideo_dd::CreateFullscreenSoftwareSurfaces` -> `src/GameZRecoil/zVideo/zVideo.cpp:11521`
- `0x4a8dc0` `zVideo_dd::CreateFullscreenHardwareSurfaces` -> `src/GameZRecoil/zVideo/zVideo.cpp:11670`
- `0x4a8f80` `zVideo_dd::InitFullscreenSoftwarePixelPack` -> `src/GameZRecoil/zVideo/zVideo.cpp:11270`
- `0x4a9060` `zVideo_dd::VerifyFullscreenSurfaceLocks` -> `src/GameZRecoil/zVideo/zVideo.cpp:11192`
- `0x4a90e0` `zVideo_dd::RestoreDisplaySurfaces` -> `src/GameZRecoil/zVideo/zVideo.cpp:11223`
- `0x4a9160` `zVideo_dd::VerifySurfaceStateLocking` -> `src/GameZRecoil/zVideo/zVideo.cpp:11862`
- `0x4a91b0` `zVideo_dd::ReleaseAllInterfacesAndSurfaces` -> `src/GameZRecoil/zVideo/zVideo.cpp:11779`
- `0x4a9300` `zVideo_dd::TeardownVideoSubsystem` -> `src/GameZRecoil/zVideo/zVideo.cpp:11893`
- `0x4a9390` `zVideo_dd::RunDirectDrawDeviceEnumeration` -> `src/GameZRecoil/zVideo/zVideo.cpp:9955`
- `0x4a93d0` `zVideo_dd::EnumDirectDrawDeviceCallback` -> `src/GameZRecoil/zVideo/zVideo.cpp:9659`
- `0x4a95e0` `zVideo_dd::EnumerateDirect3DDevicesForRecord` -> `src/GameZRecoil/zVideo/zVideo.cpp:10027`
- `0x4a96b0` `zVideo_dd::EnumDirect3DDeviceCallback` -> `src/GameZRecoil/zVideo/zVideo.cpp:9789`
- `0x4a9890` `zVideo_dd::PaletteSetEntries` -> `src/GameZRecoil/zVideo/zVideo.cpp:11929`
- `0x4a9900` `zVideo_dd::GetAcceptedDirectDrawDeviceCountCached` -> `src/GameZRecoil/zVideo/zVideo.cpp:9601`
- `0x4a9910` `zVid::GetAcceptedHardwareRendererCount_Cached` -> `src/GameZRecoil/zVideo/zVideo.cpp:1558`
- `0x4a9920` `zVideo_dd::GetHwApiDeviceFeatureFlags` -> `src/GameZRecoil/zVideo/zVideo.cpp:11965`
- `0x4a9940` `zVid::GetSelectedD3DDeviceNameOrDefault` -> `src/GameZRecoil/zVideo/zVideo.cpp:1894`
- `0x4a9950` `zVid::QueryDeviceVideoMemoryBytes` -> `src/GameZRecoil/zVideo/zVideo.cpp:1795`
- `0x4a9a30` `zVid::QueryTextureMemoryBytes` -> `src/GameZRecoil/zVideo/zVideo.cpp:1838`
- `0x4a9ac0` `zVideo_dd3d::BeginSceneAndFlushPendingRenderStates` -> `src/GameZRecoil/zVideo/zVideo.cpp:6514`
- `0x4a9b40` `zVideo_dd3d::EndScene` -> `src/GameZRecoil/zVideo/zVideo.cpp:6560`
- `0x4a9b70` `zVideo_dd3d::PresentDisplayModeSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:6989`
- `0x4a9c20` `zVideo_dd3d::CreateDeviceState` -> `src/GameZRecoil/zVideo/zVideo.cpp:7327`
- `0x4aa0f0` `zVideo_dd3d::CreateTextureRecord` -> `src/GameZRecoil/zVideo/zVideo.cpp:7059`
- `0x4aa600` `zVideo_dd3d::UploadImageToSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:9393`
- `0x4aa6f0` `zVideo_dd3d::ConvertImagePixelsForTexture` -> `src/GameZRecoil/zVideo/zVideo.cpp:9315`
- `0x4aa8b0` `zVideo_dd3d::TextureRecord_LockUploadSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:9285`
- `0x4aa8f0` `zVideo_dd3d::TextureRecord_UnlockUploadSurface` -> `src/GameZRecoil/zVideo/zVideo.cpp:9454`
- `0x4aa900` `zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef` -> `src/GameZRecoil/zVideo/zVideo.cpp:9474`
- `0x4aa920` `zVideo_dd3d::TextureRecord_FinalizeUpload` -> `src/GameZRecoil/zVideo/zVideo.cpp:9492`
- `0x4aa980` `zVideo_dd3d::TextureRecord_Destroy` -> `src/GameZRecoil/zVideo/zVideo.cpp:9538`
- `0x4aa9d0` `zVideo_dd3d::TextureRecord_Create` -> `src/GameZRecoil/zVideo/zVideo.cpp:9270`
- `0x4aa9e0` `zVideo_dd3d::SetFogEnable` -> `src/GameZRecoil/zVideo/zVideo.cpp:7581`
- `0x4aaa30` `zVideo_dd3d::SetFogStart` -> `src/GameZRecoil/zVideo/zVideo.cpp:7611`
- `0x4aaa60` `zVideo_dd3d::SetFogEnd` -> `src/GameZRecoil/zVideo/zVideo.cpp:7624`
- `0x4aaa90` `zVideo_dd3d::ApplyFogStateFromGlobals` -> `src/GameZRecoil/zVideo/zVideo.cpp:7637`
- `0x4aab30` `zVideo_dd3d::UpdateFogColor` -> `src/GameZRecoil/zVideo/zVideo.cpp:7672`
- `0x4aab90` `zVideo_dd3d::SubmitPolyFlatColor16` -> `src/GameZRecoil/zVideo/zVideo.cpp:7711`
- `0x4aaef0` `zVideo_dd3d::SubmitPolyGouraudColor16` -> `src/GameZRecoil/zVideo/zVideo.cpp:7851`
- `0x4ab320` `zVideo_dd3d::SubmitPolyColorAttr` -> `src/GameZRecoil/zVideo/zVideo.cpp:7990`
- `0x4ab6d0` `zVideo_dd3d::SubmitPolyRenderClass` -> `src/GameZRecoil/zVideo/zVideo.cpp:8093`
- `0x4abb20` `zVideo_dd3d::SubmitPolygon` -> `src/GameZRecoil/zVideo/zVideo.cpp:8263`
- `0x4ac370` `zVideo_dd3d::SubmitPolygonLit` -> `src/GameZRecoil/zVideo/zVideo.cpp:8461`
- `0x4acbd0` `zVideo_dd3d::DrawPointColor16` -> `src/GameZRecoil/zVideo/zVideo.cpp:8656`
- `0x4accc0` `zVideo_dd3d::SetQuadBatchDepthAndRhw` -> `src/GameZRecoil/zVideo/zVideo.cpp:7685`
- `0x4acd00` `zVideo_dd3d::QueueSolidQuad` -> `src/GameZRecoil/zVideo/zVideo.cpp:8707`
- `0x4ace30` `zVideo_dd3d::FlushSortedPolys` -> `src/GameZRecoil/zVideo/zVideo.cpp:8761`
- `0x4ad120` `zVideo_dd3d::FlushQuadBatch` -> `src/GameZRecoil/zVideo/zVideo.cpp:8908`
- `0x4ad250` `zVideo_dd3d::FlushOverwritePolys` -> `src/GameZRecoil/zVideo/zVideo.cpp:8984`
- `0x4ad680` `zVideo_dd3d::FloorPowerOfTwo` -> `src/GameZRecoil/zVideo/zVideo.cpp:9245`
- `0x4ad6a0` `zVideo_dd::ReportError` -> `src/GameZRecoil/zVideo/zVideo.cpp:11973`
- `0x4b1090` `zWepInit` -> `src/GameZRecoil/zWeapon/zWeapon.cpp:40`
- `0x4b1140` `zWeapon::OnWeaponsSectionPreLoad` -> `src/GameZRecoil/zWeapon/zWeapon.cpp:85`
- `0x4b1160` `zWeapon::OnWeaponsSectionDataReady` -> `src/GameZRecoil/zWeapon/zWeapon.cpp:105`
- `0x4b1d80` `zWeapon::SetMaxTetherAltitude` -> `src/GameZRecoil/zWeapon/zWeapon.cpp:123`
- `0x4b1d90` `OptCatalog::ShutdownCore` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2032`
- `0x4b1f90` `OptCatalog::FreeTrailRuntimeStateStorage` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2096`
- `0x4b2160` `Light::InitThermalGlowPool` -> `src/GameZRecoil/zClass/Light.c:148`
- `0x4b21c0` `PlayerTimedHitStatus::ResetFields` -> `src/GameZRecoil/zWeapon/zWeapon.cpp:136`
- `0x4b21e0` `Light::DestroyThermalGlowPool` -> `src/GameZRecoil/zClass/Light.c:179`
- `0x4b2210` `HitSource::UpdateTimedStatus` -> `src/GameZRecoil/zWeapon/zWeapon.cpp:248`
- `0x4b22d0` `PlayerTimedHitStatus::ClearLightAndReset` -> `src/GameZRecoil/zWeapon/zWeapon.cpp:151`
- `0x4b2300` `PlayerTimedHitStatus::TickAndUpdateLight` -> `src/GameZRecoil/zWeapon/zWeapon.cpp:169`
- `0x4b2520` `Light::AllocFromFreeListAndAttach` -> `src/GameZRecoil/zClass/Light.c:198`
- `0x4b2570` `Light::ReturnToFreeList` -> `src/GameZRecoil/zClass/Light.c:231`
- `0x4b25a0` `zClass_Node::SetDamageHitCallback` -> `src/GameZRecoil/zClass/Class.c:2740`
- `0x4b25f0` `zClass_Node::AssignDamageHandlerRecursiveIfMissing` -> `src/GameZRecoil/zClass/Class.c:2704`
- `0x4b2630` `zClass_Node::ClearDamageHandler` -> `src/GameZRecoil/zClass/Class.c:2770`
- `0x4b2670` `zClass_Node::ClearDamageHandlerRecursive` -> `src/GameZRecoil/zClass/Class.c:2723`
- `0x4b26b0` `zClass_Node::SetDamageTimerCallback` -> `src/GameZRecoil/zClass/Class.c:2794`
- `0x4b2880` `OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback` -> `src/GameZRecoil/zWeapon/OptCatalog.c:2406`
- `0x4b2960` `zGame::Options_LoadFromRegistry` -> `src/GameZRecoil/zGame/zGame.cpp:465`
- `0x4b2bf0` `zGame::Options_SaveToRegistry` -> `src/GameZRecoil/zGame/zGame.cpp:579`
- `0x4b2e80` `zGame::Options_GetOrCreateOption` -> `src/GameZRecoil/zGame/zGame.cpp:396`
- `0x4b2f50` `zSnd::AcquireCachedDirectSound` -> `src/GameZRecoil/zSound/zsnd_init.cpp:569`
- `0x4b2fa0` `zSnd::ReleaseCachedDirectSound` -> `src/GameZRecoil/zSound/zsnd_init.cpp:600`
- `0x4b2fc0` `zSnd::CachedDirectSound_GetCaps` -> `src/GameZRecoil/zSound/zsnd_init.cpp:613`
- `0x4b2fe0` `zSys::HasCpuidSupportRuntimeOptions` -> `src/GameZRecoil/zSys/zSys.cpp:172`
- `0x4b2fe0` `zSys::HasCpuidSupportRuntimeOptions` -> `src/GameZRecoil/zSys/zSys_cpu_asm.inl:32`
- `0x4b3020` `zCpu::HasMmxSupport` -> `src/GameZRecoil/zSys/zSys.cpp:1077`
- `0x4b3050` `zSys::CheckCpuSignatureMask` -> `src/GameZRecoil/zSys/zSys.cpp:157`
- `0x4b3090` `zGame_OptionsRuntimeConfig::CopyDefault` -> `src/GameZRecoil/zGame/zGame.cpp:172`
- `0x4b30b0` `zGame_OptionsRuntimeConfig::InitFromSystem` -> `src/GameZRecoil/zGame/zGame.cpp:221`
- `0x4b3160` `zGame_OptionsRuntimeConfig::LoadCpuVendorString` -> `src/GameZRecoil/zGame/zGame.cpp:186`
- `0x4b31b0` `zSys::GetCpuClass` -> `src/GameZRecoil/zSys/zSys_cpu_get_class.inl:2`
- `0x4b31c0` `zSys::GetCpuMhz` -> `src/GameZRecoil/zSys/zSys.cpp:1092`
- `0x4b31f0` `zSnd::HasMmxMixerSupport` -> `src/GameZRecoil/zSound/zsnd_init.cpp:555`
- `0x4b3210` `zSys::ReturnZeroStub` -> `src/GameZRecoil/zSys/zSys.cpp:1107`
- `0x4b3220` `zVid::HasAcceptedHardwareRenderer` -> `src/GameZRecoil/zVideo/zVideo.cpp:1563`
- `0x4b3230` `zSys::GetTotalPhysKb` -> `src/GameZRecoil/zSys/zSys.cpp:1115`
- `0x4b3260` `zGame::Options_InitRegistryContext` -> `src/GameZRecoil/zGame/zGame.cpp:451`
- `0x4b32c0` `zGame::Options_ShutdownRegistryContext` -> `src/GameZRecoil/zGame/zGame.cpp:684`
- `0x4b3380` `zGame::Options_FindOption` -> `src/GameZRecoil/zGame/zGame.cpp:379`
- `0x4b3420` `zSys::DetectCpuClassAndFeatures` -> `src/GameZRecoil/zSys/zSys_cpu_detect.inl:2`
- `0x4b3480` `zSys::ReadCpuidFeatureFlags` -> `src/GameZRecoil/zSys/zSys.cpp:240`
- `0x4b36f0` `CpuBenchmarkResolver::ResolveCpuBenchmarkPacket` -> `src/GameZRecoil/zSys/zSys.cpp:1021`
- `0x4b37f0` `CpuBenchmarkResolver::MeasureMhzViaBsfLoop_Qpc` -> `src/GameZRecoil/zSys/zSys.cpp:383`
- `0x4b37f0` `CpuBenchmarkResolver::MeasureMhzViaBsfLoop_Qpc` -> `src/GameZRecoil/zSys/zSys.cpp:506`
- `0x4b38e0` `CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc` -> `src/GameZRecoil/zSys/zSys.cpp:571`
- `0x4b38e0` `CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc` -> `src/GameZRecoil/zSys/zSys.cpp:822`
- `0x4b3b00` `zSys::ReadCmosRtcSecondsBcd` -> `src/GameZRecoil/zSys/zSys.cpp:292`
- `0x4b3b20` `zSys::ReadTsc64` -> `src/GameZRecoil/zSys/zSys.cpp:303`
- `0x4b3b50` `CpuBenchmarkResolver::MeasureCpuMhz_CmosRtc` -> `src/GameZRecoil/zSys/zSys.cpp:948`
- `0x4b3ca0` `zSys::Sub64` -> `src/GameZRecoil/zSys/zSys.cpp:323`
- `0x4b3ce0` `HudUiWidget::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13474`
- `0x4b3d00` `HudUiWidget::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7905`
- `0x4b3d50` `HudUiWidget::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13469`
- `0x4b3da0` `HudUiWidget::ReleaseImageIfOwned` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13417`
- `0x4b3dd0` `HudUiWidget::SetPos` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13486`
- `0x4b3e30` `HudUiWidget::SetImageByPathOwned` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13449`
- `0x4b3e70` `HudUiWidget::SetImageBorrowedAndInvalidate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13430`
- `0x4b4030` `HudUiWidget::HitTest` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13331`
- `0x4b40c0` `HudUiElement::CopyConstructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4901`
- `0x4b4120` `HudUiElement::CopyFrom` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4920`
- `0x4b4180` `HudUiElement::Invalidate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4985`
- `0x4b4190` `HudUiElement::SetBltSourceAndClipRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5112`
- `0x4b41b0` `HudUiElement::SetClipRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5122`
- `0x4b41e0` `HudUiElement::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5044`
- `0x4b4280` `HudUiElement::SetTimer` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5096`
- `0x4b42c0` `HudUiElement::GetTextRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5151`
- `0x4b42f0` `HudUiTextInput::HudUiTextInput` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13790`
- `0x4b4370` `HudUiTextInput::~HudUiTextInput` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13745`
- `0x4b4390` `HudUiTextInput::AllocTextBuffer` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13767`
- `0x4b43d0` `HudUiTextInput::SetContents` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13837`
- `0x4b4410` `HudUiTextInput::GetBuffer` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13850`
- `0x4b4420` `HudUiTextInput::SetCursorPosition` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13827`
- `0x4b4460` `HudUiTextInput::DispatchKeyAction` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13940`
- `0x4b44e0` `HudUiTextInput::InsertCharAtCursor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13922`
- `0x4b4530` `HudUiTextInput::BackspaceDeleteChar` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13911`
- `0x4b4550` `HudUiTextInput::DeleteCharForward` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13888`
- `0x4b4560` `HudUiTextInput::MoveCursorLeft` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13896`
- `0x4b4570` `HudUiTextInput::MoveCursorRight` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13903`
- `0x4b4590` `HudUiTextInput::ShiftTextRight` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13855`
- `0x4b45e0` `HudUiTextInput::ShiftTextLeft` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13873`
- `0x4b4620` `HudUiSliderBorder::HudUiSliderBorder` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14658`
- `0x4b47a0` `HudUiElement::~HudUiElement` -> `src/GameZRecoil/zHud/zhud_ui.cpp:4939`
- `0x4b47a0` `HudUiElement::~HudUiElement` -> `src/GameZRecoil/zHud/zhud_ui.h:114`
- `0x4b47b0` `HudUiSliderBorder::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14741`
- `0x4b4810` `HudUiSliderBorder::SetBounds` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14764`
- `0x4b49e0` `HudUiNumericTextInput::HudUiNumericTextInput` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14846`
- `0x4b4a90` `HudUiNumericTextInput::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15052`
- `0x4b4ac0` `HudUiNumericTextInput::~HudUiNumericTextInput` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15024`
- `0x4b4b30` `HudUiNumericTextInput::RawKeyboardCallback` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15080`
- `0x4b4b50` `HudUiNumericTextInput::OnRawKeyboardChar` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15127`
- `0x4b4ba0` `HudUiNumericTextInput::SetInputActive` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15093`
- `0x4b4c50` `HudUiNumericTextInput::SetRawKeyboardCapture` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14994`
- `0x4b4c90` `HudUiNumericTextInput::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15017`
- `0x4b4ca0` `HudUiNumericTextInput::UpdateCaptureUiAndClip` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14932`
- `0x4b4e40` `HudUiNumericTextInput::AllocTextBuffer` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14901`
- `0x4b4e60` `HudUiNumericTextInput::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14914`
- `0x4b4ed0` `HudUiNumericTextInput::GetBuffer` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14908`
- `0x4b4ee0` `HudUiZrdWidget::HudUiZrdWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8334`
- `0x4b4ee0` `HudUiZrdWidget::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8371`
- `0x4b50a0` `HudUiZrdWidget::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8836`
- `0x4b50c0` `HudUiZrdWidget::~HudUiZrdWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8715`
- `0x4b52f0` `HudUiZrdWidget::DeleteChildIfPresent` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8700`
- `0x4b5310` `HudUiZrdWidget::Invalidate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8868`
- `0x4b5350` `HudUiZrdWidget::GetBoundsRectOrNull` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8886`
- `0x4b5630` `HudUiZrdWidget::ShowPreview` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9005`
- `0x4b5740` `HudUiZrdWidget::RefreshState` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8955`
- `0x4b5860` `HudUiZrdWidget::HidePreview` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9090`
- `0x4b5900` `HudUiZrdWidget::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9046`
- `0x4b59f0` `HudUiZrdWidget::LoadFromZrd` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8378`
- `0x4b6fc0` `HudUiCheckToggleWidget::HudUiCheckToggleWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9120`
- `0x4b6fc0` `HudUiCheckToggleWidget::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9130`
- `0x4b7000` `HudUiCheckToggleWidget::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9170`
- `0x4b70b0` `HudUiCheckToggleWidget::GetBoundsRectOrNull` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9194`
- `0x4b70c0` `HudUiCheckToggleWidget::RefreshState` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9199`
- `0x4b7210` `HudUiCheckToggleWidget::ShowPreview` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9253`
- `0x4b7250` `HudUiCheckToggleWidget::HidePreview` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9266`
- `0x4b7290` `HudUiCheckToggleWidget::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9280`
- `0x4b72c0` `HudUiCheckToggleWidget::SetChecked` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9419`
- `0x4b7d60` `HudUiCycleSelectorWidget::HudUiCycleSelectorWidget` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9450`
- `0x4b7d60` `HudUiCycleSelectorWidget::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9466`
- `0x4b7dc0` `HudUiCycleSelectorWidget::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9506`
- `0x4b7e60` `HudUiCycleSelectorWidget::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9612`
- `0x4b7ee0` `HudUiCycleSelectorWidget::AdvanceSelectionAndActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9531`
- `0x4b7f20` `HudUiCycleSelectorWidget::SetIndexClamped` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9553`
- `0x4b7f80` `HudUiCycleSelectorWidget::SetVisibleRange` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9588`
- `0x4b7fd0` `HudUiCycleSelectorWidget::AddTextEntry` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9632`
- `0x4b8100` `HudUiCycleSelectorWidget::ApplyFontStyleForEntry` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9827`
- `0x4b8200` `HudUiCycleSelectorWidget::AddBitmapEntry` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9897`
- `0x4b82e0` `HudUiCycleSelectorWidget::LoadFromZrd` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9932`
- `0x4b8450` `HudUiFillBitmap::HudUiFillBitmap` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10058`
- `0x4b84b0` `HudUiFillBitmap::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10109`
- `0x4b8520` `HudUiFillBitmap::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10121`
- `0x4b85c0` `HudUiFillBitmap::LoadFromZrd` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10150`
- `0x4b8650` `HudUiFillBitmap::UpdateNormalizedFromCursor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10200`
- `0x4b86b0` `HudUiFillBitmap::SetNormalizedValueAndRebuild` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10229`
- `0x4b8760` `HudUiZrdWidgetEx17C_Item::HudUiZrdWidgetEx17C_Item` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10263`
- `0x4b87a0` `HudUiZrdWidgetEx17C_Item::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10282`
- `0x4b87c0` `HudUiZrdWidgetEx17C_Item::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10277`
- `0x4b87d0` `HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10294`
- `0x4b87e0` `HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10305`
- `0x4b87f0` `HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10316`
- `0x4b8850` `HudUiZrdWidgetEx17C_Item::LoadFromZrd` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10334`
- `0x4b8a90` `HudUiZrdWidgetEx17C_Item::SetSelected` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10426`
- `0x4b8af0` `HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10449`
- `0x4b8b10` `HudUiZrdWidgetEx17C::HudUiZrdWidgetEx17C` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10454`
- `0x4b8b40` `HudUiZrdWidgetEx17C::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10493`
- `0x4b8b60` `HudUiZrdWidgetEx17C::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10472`
- `0x4b8be0` `HudUiZrdWidgetEx17C::LoadFromZrd` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10524`
- `0x4b8cf0` `HudUiZrdWidgetEx17C::SetSelectedIndex` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10584`
- `0x4b8d30` `HudCmdBindButtonBase::HudCmdBindButtonBase` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10639`
- `0x4b92a0` `HudUiListSelectorItem::HudUiListSelectorItem` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10604`
- `0x4b92a0` `HudUiListSelectorItem::HudUiListSelectorItem` -> `src/GameZRecoil/zHud/zhud_ui.h:1162`
- `0x4b9520` `HudUiListSelectorItem::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10606`
- `0x4ba070` `HudUiBackground::BindButtonsNodeToWidgetByName` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7561`
- `0x4ba0c0` `HudUiBackground::BindWidgetByName` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7588`
- `0x4ba0e0` `HudUiBackground::BindPrimitiveNodeToElement` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7602`
- `0x4ba380` `HudUiDialogController::BlitOwnedSurfaceToPrimary` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7068`
- `0x4ba3a0` `HudUiContainer::InvalidateChildren` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6099`
- `0x4ba3c0` `HudUiFillBitmap::SetNormalizedValue` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10211`
- `0x4ba3e0` `HudUiOwnedTextInput::OnAccept` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13977`
- `0x4ba400` `HudUiPanel::GetWrapRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15859`
- `0x4ba410` `HudUiListSelectorItem::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10624`
- `0x4ba470` `StdPtrVector::FreeBufferAndReset` -> `src/GameZRecoil/zHud/zhud_ui.cpp:10663`
- `0x4ba4a0` `HudFontStyle::HudFontStyle` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7884`
- `0x4ba4c0` `HudFontStyle::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7900`
- `0x4ba4d0` `HudUiPanelPtrVector::EraseRange` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8545`
- `0x4ba510` `HudUiPanelPtrVector::InsertN` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8576`
- `0x4ba740` `HudUiPanel::HudUiPanel` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15627`
- `0x4bab40` `HudUiPanel::~HudUiPanel` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15779`
- `0x4bac10` `HudUiPanel::RebuildTextRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16164`
- `0x4bb0c0` `HudUiFlashPanel::ComputeFlashBlendColor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5937`
- `0x4bb1c0` `HudUiPanel::MeasureTextPrefixRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16452`
- `0x4bb2a0` `HudUiPanel::UpdateTextBoundsFromContent` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16391`
- `0x4bb3d0` `HudUiPanel::HitTest` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15867`
- `0x4bb440` `HudUiPanel::GetLastTextPtr` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15890`
- `0x4bb460` `HudUiPanel::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15803`
- `0x4bb540` `HudUiPanel::SetTextFmt` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16051`
- `0x4bb5e0` `HudUiPanel::SetTextFmtV` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16072`
- `0x4bb680` `HudUiPanel::SetText` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16120`
- `0x4bb710` `HudUiPanel::QueryTextHeight` -> `src/GameZRecoil/zHud/zhud_ui.cpp:16503`
- `0x4bb740` `HudUiPanel::GetTextRect` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15903`
- `0x4bb790` `HudUiCompositePanel::ConstructorWithEntryCount` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5567`
- `0x4bb960` `HudUiCompositePanel::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5611`
- `0x4bb980` `HudUiCompositePanel::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5627`
- `0x4bb9f0` `HudUiCompositePanel::SetPos` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5646`
- `0x4bbaa0` `HudUiCompositePanel::SetTextFmt` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5716`
- `0x4bbac0` `HudUiCompositePanel::SetTextFmtV` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5736`
- `0x4bbb20` `HudUiCompositePanel::ScrollHistory` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5754`
- `0x4bbbe0` `HudUiCompositePanel::SetFont` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5779`
- `0x4bbca0` `HudUiCompositePanel::ResizeEntryVectorAndRelayout` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5828`
- `0x4bbe90` `HudUiCompositePanel::ReapplyEntryCount` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5674`
- `0x4bbed0` `HudUiCompositePanel::ResizeEntryCount` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5685`
- `0x4bbfa0` `HudUiCompositePanelVector::Clear` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5426`
- `0x4bbff0` `HudUiCompositePanelVector::InsertCopies` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5453`
- `0x4bc320` `HudUiCompositePanelEntry::ConstructorCopyRange` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5912`
- `0x4bc3a0` `HudUiCompositePanelEntry::AssignCopy` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5873`
- `0x4bc410` `HudUiCompositePanelEntry::ConstructorCopy` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5891`
- `0x4bc480` `HudUiCircle::HudUiCircle` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5348`
- `0x4bc480` `HudUiCircle::HudUiCircle` -> `src/GameZRecoil/zHud/zhud_ui.h:632`
- `0x4bc4c0` `HudUiCircle::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5375`
- `0x4bc4e0` `HudUiCircle::HitTestCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5412`
- `0x4bc510` `HudUiBackgroundContainer::HudUiBackgroundContainer` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6950`
- `0x4bc540` `HudUiBackgroundContainer::~HudUiBackgroundContainer` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6959`
- `0x4bc550` `HudUiBackgroundContainer::SetInputFocus` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6967`
- `0x4bc560` `HudUiBackgroundContainer::GetInputFocus` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6977`
- `0x4bc570` `HudUiBackground::Update` -> `src/GameZRecoil/zHud/zhud_ui.cpp:7776`
- `0x4bc780` `HudUiContainer::HudUiContainer` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5964`
- `0x4bc7b0` `HudUiContainer::DestructorCore` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5972`
- `0x4bc7c0` `HudUiContainer::AddChild` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5983`
- `0x4bc810` `HudUiContainer::FindChildWithPrev` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6000`
- `0x4bc860` `HudUiContainer::RemoveChild` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6031`
- `0x4bc8d0` `HudUiContainer::SetChildFlags` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6061`
- `0x4bc900` `HudUiContainer::UpdateAll` -> `src/GameZRecoil/zHud/zhud_ui.cpp:6084`
- `0x4bc930` `HudUiTransitionTextPanel::ResetFlashState` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9758`
- `0x4bc980` `HudUiTransitionTextPanel::SetFlashRate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:9784`
- `0x4bcb50` `HudUiTextLabel::HudUiTextLabel` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15417`
- `0x4bccf0` `HudUiTextLabel::SetTextFmt` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15496`
- `0x4bcd40` `HudUiPanel::SetClip` -> `src/GameZRecoil/zHud/zhud_ui.cpp:5137`
- `0x4bcd80` `HudUiTextLabel::RebuildTextBounds` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15529`
- `0x4bcdc0` `HudUiTextLabel::MeasureTextWidth` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15543`
- `0x4bcdf0` `HudUiTextLabel::UpdateTextExtents` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15614`
- `0x4bce30` `HudUiTextLabel::OnDraw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15556`
- `0x4bcea0` `HudUiTextLabel::HitTest` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15589`
- `0x4bcf20` `HudUiBar::HudUiBar` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14526`
- `0x4bcff0` `HudUiBar::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14542`
- `0x4bd020` `HudUiTopMessageStack::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17626`
- `0x4bd100` `HudUiPanel::ConstructorDefaultThunk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:15674`
- `0x4bd2a0` `HudUiTextStack4::Clear` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17561`
- `0x4bd2d0` `HudUiChatMessageStack::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17669`
- `0x4bd3d0` `HudUiTextStack4::SetTextColors` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17545`
- `0x4bd410` `HudUiTextStack4::SetXAll` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17598`
- `0x4bd440` `HudUiTextStack4::SetYDescending` -> `src/GameZRecoil/zHud/zhud_ui.cpp:17611`
- `0x4bdb60` `zVideoFxPass3Element::Draw` -> `src/GameZRecoil/zVideo/zVideo.cpp:1915`
- `0x4bdbc0` `zVideoFxPass3RootElement::ApplyPass3` -> `src/GameZRecoil/zVideo/zVideo.cpp:1952`
- `0x4bdbe0` `zVideoFxPass3Slot::Constructor` -> `src/GameZRecoil/zVideo/zVideo.cpp:1963`
- `0x4bdc40` `zVideoFxPass3Slot::ApplyPass3` -> `src/GameZRecoil/zVideo/zVideo.cpp:1998`
- `0x4bde20` `HudWeatherFx::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:177`
- `0x4be2c0` `HudWeatherFxSnow::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:375`
- `0x4be2e0` `HudWeatherFxSnow::Destructor` -> `src/Battlesport/hud.cpp:387`
- `0x4be850` `HudWeatherFxRain::ScalarDeletingDestructor` -> `src/Battlesport/hud.cpp:563`
- `0x4bed50` `zVideoFxPass3Config::SetPrimaryElementParamsLocal` -> `src/GameZRecoil/zVideo/zVideo.cpp:3720`
- `0x4bed90` `zVideoFxPass3Config::QueueElementLocal` -> `src/GameZRecoil/zVideo/zVideo.cpp:3745`
- `0x4bee20` `zVideoFxPass3Config::QueuePrimitiveRaw` -> `src/GameZRecoil/zVideo/zVideo.cpp:3693`
- `0x4bee40` `zVideoFxPass3Config::CrtInitGlobalSingleton` -> `src/GameZRecoil/zVideo/zVideo.cpp:2074`
- `0x4bee50` `zVideoFxPass3Config::ConstructGlobalSingleton` -> `src/GameZRecoil/zVideo/zVideo.cpp:2059`
- `0x4bee60` `zVideoFxPass3Config::RegisterDestroyAtExit` -> `src/GameZRecoil/zVideo/zVideo.cpp:2069`
- `0x4bee70` `zVideoFxPass3Config::DestroyGlobalSingleton` -> `src/GameZRecoil/zVideo/zVideo.cpp:2064`
- `0x4bee80` `zVideoFxPass3Config::Destructor` -> `src/GameZRecoil/zVideo/zVideo.cpp:2052`
- `0x4beee0` `zVideo::FxPass3_SetPrimaryElementParamsLocal` -> `src/GameZRecoil/zVideo/zVideo.cpp:3733`
- `0x4bef10` `zVideo::FxPass3_QueueElementLocal` -> `src/GameZRecoil/zVideo/zVideo.cpp:3776`
- `0x4bef50` `zVideo::FxPass3_QueuePrimitive` -> `src/GameZRecoil/zVideo/zVideo.cpp:3799`
- `0x4bef90` `zVideoFxPass3Config::Constructor` -> `src/GameZRecoil/zVideo/zVideo.cpp:2015`
- `0x4bf060` `HudUiMessageBoxDialog::Constructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:12991`
- `0x4bf540` `HudUiMessageBoxDialog::ScalarDeletingDestructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13148`
- `0x4bf560` `HudUiMessageBoxDialog::Destructor` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13160`
- `0x4bf630` `HudUiMessageBoxDialog::RunModal` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13180`
- `0x4bf7c0` `HudUiMessageBoxDialog::OnOk` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13268`
- `0x4bf7e0` `HudUiMessageBoxDialog::OnCancel` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13274`
- `0x4bf800` `HudUiMessageBoxOkButton::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13280`
- `0x4bf820` `HudUiMessageBoxCancelButton::OnActivate` -> `src/GameZRecoil/zHud/zhud_ui.cpp:13288`
- `0x4bf840` `HudUiPolyline::HudUiPolyline` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14580`
- `0x4bf8b0` `HudUiPolyline::SetPoint` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14635`
- `0x4bf900` `HudUiPolyline::Draw` -> `src/GameZRecoil/zHud/zhud_ui.cpp:14601`
- `0x4bfe20` `HudUiBackgroundVideoWidget::SetColorKey565` -> `src/GameZRecoil/zHud/zhud_ui.cpp:8243`
- `0x4bffe0` `zUtil_ZAR::RegisterSectionHandler` -> `src/GameZRecoil/zUtil/zZbd.cpp:544`
- `0x4c0010` `zUtil_ZAR::WriteSectionBlob` -> `src/GameZRecoil/zUtil/zZbd.cpp:564`
- `0x4c0100` `zUtil::ZBD_Init` -> `src/GameZRecoil/zUtil/zZbd.cpp:47`
- `0x4c0180` `zUtil::ZBD_DestroyGlobalManager` -> `src/GameZRecoil/zUtil/zZbd.cpp:69`
- `0x4c01b0` `zZbdManager::Destroy` -> `src/GameZRecoil/zUtil/zZbd.cpp:188`
- `0x4c0280` `zZbdManager::RegisterSectionHandler` -> `src/GameZRecoil/zUtil/zZbd.cpp:213`
- `0x4c0630` `zZbdManager::WriteSectionRecord` -> `src/GameZRecoil/zUtil/zZbd.cpp:371`

## WestwoodOnlineUpgradeConfigDialog.cpp

- `0x441a10` `WestwoodOnlineUpgradeConfigDialog::GetMessageMap` -> `src/Battlesport/WestwoodOnlineUpgradeConfigDialog.cpp:127`

## WinMain.cpp

- `0x4c81c0` `WinMain` -> `src/Battlesport/WinMain.cpp:15`

## zeff_anim.c

- `0x45d6c0` `zEffectAnim::ResetForNode` -> `src/GameZRecoil/zEffect/zEffect.cpp:1013`
- `0x45d7b0` `zEffectAnim::SetTransformRotAndVelocity` -> `src/GameZRecoil/zEffect/zEffect.cpp:1424`
- `0x45d930` `zEffectAnim::ActivateRuntime` -> `src/GameZRecoil/zEffect/zEffect.cpp:1286`
- `0x45db20` `zEffectAnim::CheckActivationPrereqs` -> `src/GameZRecoil/zEffect/zEffect.cpp:950`
- `0x45dc70` `zEffectAnim::SetTransformRotAndVelocity_Thunk` -> `src/GameZRecoil/zEffect/zEffect.cpp:1515`
- `0x45dcb0` `zEffectAnim::SetVelocity` -> `src/GameZRecoil/zEffect/zEffect.cpp:1545`
- `0x45dde0` `zEffectAnim::SetVelocity_Thunk` -> `src/GameZRecoil/zEffect/zEffect.cpp:1592`
- `0x45de00` `zEffectAnim::SetPositionRefAndVelocity` -> `src/GameZRecoil/zEffect/zEffect.cpp:1610`
- `0x45df70` `zEffectAnim::SetPositionRefAndVelocity_Thunk` -> `src/GameZRecoil/zEffect/zEffect.cpp:1665`
- `0x45df90` `zEffectAnim::SetTransformRefs` -> `src/GameZRecoil/zEffect/zEffect.cpp:1683`
- `0x45e0b0` `zEffectAnim::SetTransformRefs_Thunk` -> `src/GameZRecoil/zEffect/zEffect.cpp:1726`
- `0x45e280` `zEffectAnim::FindSoundRefIndexByName` -> `src/GameZRecoil/zEffect/zEffect.cpp:2100`
- `0x45e300` `zEffectAnim::FindLightRefIndexByName` -> `src/GameZRecoil/zEffect/zEffect.cpp:2118`
- `0x45e380` `zEffectAnim::FindOrCreateSoundRef` -> `src/GameZRecoil/zEffect/zEffect.cpp:2136`
- `0x45e4a0` `zEffectAnim::FindOrCreateLightRef` -> `src/GameZRecoil/zEffect/zEffect.cpp:2206`
- `0x45e5c0` `zEffectAnim::ResolveNodeByName` -> `src/GameZRecoil/zEffect/zEffect.cpp:2272`
- `0x45e650` `zEffectAnim::FindNodeRecursiveByName` -> `src/GameZRecoil/zEffect/zEffect.cpp:2053`
- `0x45e6d0` `zEffectAnim::EnsureCopiedRootTree` -> `src/GameZRecoil/zEffect/zEffect.cpp:2427`
- `0x45ff10` `zEffectAnim::FindEntryByName` -> `src/GameZRecoil/zEffect/zEffect.cpp:2032`

## zeff_anim_run.c

- `0x458e10` `zEffect::HandleSampleRefOffsetEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:6052`
- `0x458eb0` `zEffect::HandleEffectTemplateOffsetEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:6005`
- `0x458f70` `zEffect::HandleSoundEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:6080`
- `0x459080` `zEffect::HandleLightEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:6146`
- `0x459280` `zEffect::HandleLightAnimEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:6299`
- `0x459510` `zEffect::HandleFogEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:6403`
- `0x459580` `zEffect::HandleCameraParamsEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:6443`
- `0x4596c0` `zEffect::AnimateCameraParamsOverTime` -> `src/GameZRecoil/zEffect/zEffect.cpp:6546`
- `0x459ae0` `zEffect::HandleRotationEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:7518`
- `0x459cb0` `zEffect::HandleNodeScaleEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:7503`
- `0x459ce0` `zEffect::HandlePositionEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:7422`
- `0x459e30` `zEffect::HandleActivateEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:7400`
- `0x459e70` `zEffect::HandleNodeAnimEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:6873`
- `0x45a920` `zEffect::FindNearestPickCandidateBelowPoint` -> `src/GameZRecoil/zEffect/zEffect.cpp:6836`
- `0x45a9d0` `zEffect::AnimateNodeOverTime` -> `src/GameZRecoil/zEffect/zEffect.cpp:7211`
- `0x45ae30` `zEffect_Anim::AdvanceKeyframeSample` -> `src/GameZRecoil/zEffect/zEffect.cpp:4532`
- `0x45ae90` `zEffect_Anim::AnimateKeyframeSample` -> `src/GameZRecoil/zEffect/zEffect.cpp:4564`
- `0x45b120` `zEffect_Anim::AdvanceKeyframe` -> `src/GameZRecoil/zEffect/zEffect.cpp:4712`
- `0x45b210` `zEffect_Anim::EvaluateKeyframe` -> `src/GameZRecoil/zEffect/zEffect.cpp:4765`
- `0x45b280` `zEffect_Anim::RunKeyframes` -> `src/GameZRecoil/zEffect/zEffect.cpp:4795`
- `0x45b3b0` `zEffect::HandleAddChildEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:7602`
- `0x45b410` `zEffect::HandleRemoveChildEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:7627`
- `0x45b440` `zEffect::HandleAttachEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:7641`
- `0x45b4a0` `zEffect::HandleDetachEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:7669`
- `0x45b8b0` `zEffect::HandleTransformRefsEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8173`
- `0x45bb00` `zEffect::HandleSurfaceStopEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8031`
- `0x45bbb0` `zEffect::HandleSurfacePlayEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8048`
- `0x45bc60` `zEffect::HandleSurfaceRefEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8065`
- `0x45bf60` `zEffect::CleanupLightRefs` -> `src/GameZRecoil/zEffect/zEffect.cpp:8530`
- `0x45bfd0` `zEffect::CleanupSoundRefs` -> `src/GameZRecoil/zEffect/zEffect.cpp:8565`
- `0x45c040` `zEffectAnim::Stop` -> `src/GameZRecoil/zEffect/zEffect.cpp:1226`
- `0x45c100` `zEffect::HandleNamedAnimStopEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8337`
- `0x45c1a0` `zEffect::HandleEmitterPlayEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8351`
- `0x45c240` `zEffect::HandleEmitterStopEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8320`
- `0x45c2f0` `zEffect::HandleEmitterResetEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8272`
- `0x45c310` `zEffect::HandleEmitterLoopEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8287`
- `0x45c3c0` `zEffect::HandleConditionalChainEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8368`
- `0x45c530` `zEffect::TraceUpwardHitFromNodeOrPos` -> `src/GameZRecoil/zEffect/zEffect.cpp:5568`
- `0x45c640` `zEffect::GetConditionalRefPosDistanceSq` -> `src/GameZRecoil/zEffect/zEffect.cpp:5549`
- `0x45c6b0` `zEffect::SkipConditionalChainToEnd` -> `src/GameZRecoil/zEffect/zEffect.cpp:8459`
- `0x45c6e0` `zEffect::HandleNoOpMarkerEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8479`
- `0x45c6f0` `zEffect::HandleCallbackEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8489`
- `0x45c710` `zEffect::HandleScreenColorFxEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:7877`
- `0x45c920` `zEffect::HandleScreenOverlayFxEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:7929`
- `0x45cbc0` `zEffect::HandleTopMessageEvent` -> `src/GameZRecoil/zEffect/zEffect.cpp:8506`
- `0x45cc00` `zEffect_Anim::RunSequenceEvents` -> `src/GameZRecoil/zEffect/zEffect.cpp:4871`
- `0x45d000` `zEffect::SetAnimDebugFrameTag` -> `src/GameZRecoil/zEffect/zEffect.cpp:5541`
- `0x45d010` `zEffect_Anim::RunSequence` -> `src/GameZRecoil/zEffect/zEffect.cpp:4959`
- `0x45d240` `zEffect_Anim::CaptureNodeStates` -> `src/GameZRecoil/zEffect/zEffect.cpp:4417`
- `0x45d310` `zEffect_Anim::RestoreNodeStates` -> `src/GameZRecoil/zEffect/zEffect.cpp:4473`
- `0x45d3d0` `zEffectAnim::FinalizeStop` -> `src/GameZRecoil/zEffect/zEffect.cpp:1051`
- `0x45d4c0` `zEffectAnim::RunStopSequenceCallback` -> `src/GameZRecoil/zEffect/zEffect.cpp:1102`
- `0x45d570` `zEffectAnim::StopAndCleanup` -> `src/GameZRecoil/zEffect/zEffect.cpp:1151`
- `0x45d6b0` `zEffect_Anim::NodeActionCallback` -> `src/GameZRecoil/zEffect/zEffect.cpp:5060`
- `0x45d770` `zEffectAnim::RunStopDelayCallback` -> `src/GameZRecoil/zEffect/zEffect.cpp:1265`

## zeff_detach.c

- `0x458c10` `zEffect::UpdateBeamNodeBetweenPoints` -> `src/GameZRecoil/zEffect/zEffect.cpp:5889`
- `0x458ce0` `zEffect::UpdateBeamNodeBetweenFractions` -> `src/GameZRecoil/zEffect/zEffect.cpp:5942`
