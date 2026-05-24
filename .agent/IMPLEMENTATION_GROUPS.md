# Implementation Groups

Use this file for temporary dependency-group notes during binary-safe reimplementation. The plan remains address-based; this file only lists active multi-function/source-readiness groups and their live source blockers.

## Rules

- Create or update a group before editing when a task touches more than one function or a shared type/global/vtable.
- Keep groups scoped. Prefer one class, one source file cluster, one recursive cycle, or one call-chain frontier.
- Do not mark plan entries done from this file alone. Plan markers still require current source/build/Binary Ninja evidence.
- Keep notes concise and temporary. Move durable facts into source comments, Binary Ninja comments, tests, README, or `docs/reconstruction/` before pruning.
- Verification-only queues that no longer carry source blockers do not belong in this active working file; use `.agent/RECOIL_PLAN.md`, `python tools/recoil_status.py 0xNNNNNN`, and VC verification manifests for current verification state.
- Recompute verification scope with `python tools/recoil_status.py 0xNNNNNN` or `python tools/recoil_frontier.py 0xNNNNNN --depth 1` after source blockers clear.
- Use `python tools/recoil_groups_audit.py --summary` to find stale, completed, or overgrown groups.

## Active Group Template

```text
### Group: short descriptive name

- Anchor: 0xNNNNNN Name
- Reason: dependency closure / class cluster / recursive cycle / shared ABI layout / source file cluster
- Source blockers:
  - 0xNNNNNN Name
- Next action:
  - python tools/recoil_status.py 0xNNNNNN
```

## Active Groups

### Group: Player ZAR section registration and read callbacks

- Anchor: 0x41f5b0 Player::ZAR_RegisterSections
- Reason: source-readiness closure; registration takes function pointers to mission-save and vehicle-list read/write callbacks, so callback signatures and source behavior must be ready before the registration wrapper can be implemented.
- Source blockers:
  - 0x41f1d0 Player::ApplyMissionSaveData
  - 0x45d6b0 zEffect_Anim::NodeActionCallback
  - 0x4231b0 Player::RefreshHudFromState
  - 0x4266b0 Player::TickMasterTypeAndForceFeedback
  - 0x4528a0 zClass_Node::LoadFlagBit8MaterialImagesAndTexturePack
  - 0x41f640 Player::ZAR_ReadMissionSaveDataSection
  - 0x41f850 Player::ZAR_ReadVehicleListSection
- Next action:
  - `python tools/recoil_frontier.py 0x41f640 --depth 1`
  - `python tools/recoil_frontier.py 0x41f850 --depth 1`

### Group: Deferred P0 zRndr Span*SwitchVShift cluster

- Anchor: 0x49e6c0 zRndr::SpanCopy16FromTex16SwitchVShift (priority ranks 654-660)
- Reason: HLIL-listed P0 leaves with 257-358 HLIL lines; renderer globals/types and span dispatch tables must be stable before implementation or verification is high-cost.
- Source blockers:
  - zRndr span switch tables and texture/palette path globals (not audited in this pass)
- Next action:
  - defer until `zRndr` dependency frontier is `Source dependencies satisfied` for a chosen renderer anchor

### Group: HUD container dispatch model

- Anchor: 0x4bc780 HudUiContainer::ConstructorDefault
- Reason: shared container ftable/vtable dispatch source cleanup and VC6 verification.
- Source blockers:
  - 0x4bc780 HudUiContainer::ConstructorDefault
  - 0x4bc900 HudUiContainer::UpdateAll
  - 0x40d9d0 HudUiContainer::SetEnabled
  - 0x40fa10 HudUiStatsListElement::Update
- Next action:
  - `python tools/recoil_status.py 0x4bc780`

### Group: HUD per-frame update

- Anchor: 0x42f280 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x42f280 pending
- Next action:
  - `python tools/recoil_status.py 0x42f280`

### Group: HUD timed task active list

- Anchor: 0x42f280 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x42f280 pending
- Next action:
  - `python tools/recoil_status.py 0x42f280`

### Group: HudUiNetExitPanel callbacks

- Anchor: 0x42eed0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x42eed0 pending
- Next action:
  - `python tools/recoil_status.py 0x42eed0`

### Group: Briefing objective action queue

- Anchor: 0x42eed0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x42eed0 pending
  - 0x404400 HudUiBriefingRuntime::BuildObjectiveActionsFromIndex
  - 0x4045b0 Briefing_ActionQueue::AddHideElement
  - 0x404620 BriefingActionHideElement::Tick
  - 0x404640 Briefing_ActionQueue::AddShowElement
  - 0x4046b0 BriefingActionShowElement::Tick
  - 0x4046d0 Briefing_ActionQueue::AddFadeInElement
  - 0x404740 BriefingActionFadeInElement::Tick
  - 0x404780 Briefing_ActionQueue::AddSetPanelText
  - 0x404850 BriefingActionSetPanelText::Tick
  - 0x4048a0 Briefing_ActionQueue::AddSetWidgetImageTimed
  - 0x404960 BriefingActionSetWidgetImageTimed::Tick
  - 0x4049d0 Briefing_ActionQueue::AddPlaySampleByName
  - 0x404aa0 BriefingActionPlaySample::Tick
  - 0x404b30 Briefing::SampleEventCallback
  - 0x404b40 Briefing_ActionQueue::AddDelayUntilProgress
  - 0x404bb0 BriefingActionDelayUntilProgress::Tick
  - 0x404c80 Briefing::BuildObjectiveActionsGlobal
- Next action:
  - `python tools/recoil_status.py 0x42eed0`

### Group: Briefing thread loop

- Anchor: 0x42eed0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x42eed0 pending
  - 0x403930 HudUiBriefingRuntime::Constructor
  - 0x403d90 HudUiBriefingRuntime::ScalarDeletingDestructor
  - 0x403ed0 HudUiBriefingRuntime::Destructor
  - 0x404180 Briefing::StartForMission
  - 0x404280 Briefing::ThreadMain
  - 0x4a56d0 Time::Tick
- Next action:
  - `python tools/recoil_status.py 0x42eed0`

### Group: Briefing locator panel leaves

- Anchor: 0x403930 HudUiBriefingRuntime::Constructor
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x403930 HudUiBriefingRuntime::Constructor
  - 0x403c10 HudUiBriefingLocatorPanel::Constructor
  - 0x403c90 HudUiBriefingLocatorPanel::BlitDirtyRect
  - 0x403cb0 HudUiBriefingLocatorPanel::Update
- Next action:
  - `python tools/recoil_status.py 0x403930`

### Group: Briefing objective picture noise

- Anchor: 0x403930 HudUiBriefingRuntime::Constructor
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x403930 HudUiBriefingRuntime::Constructor
  - 0x4038a0 HudUiBriefingObjectivePicture::DrawNoiseOverlay
  - 0x48d910 zVid::DrawNoiseRect
- Next action:
  - `python tools/recoil_status.py 0x403930`

### Group: Briefing composite panel constructor dependencies

- Anchor: 0x403930 HudUiBriefingRuntime::Constructor
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x403930 HudUiBriefingRuntime::Constructor
  - 0x4bb790 HudUiCompositePanel::ConstructorWithEntryCount
  - 0x4bb9f0 HudUiCompositePanel::LayoutEntries
  - 0x4bbaa0 HudUiCompositePanel::SetTextFmt
  - 0x4bbac0 HudUiCompositePanel::SetTextFmtV
  - 0x4bbb20 HudUiCompositePanel::ScrollHistory
  - 0x4bbbe0 HudUiCompositePanel::SetFont
  - 0x4bbca0 HudUiCompositePanel::ResizeEntryVectorAndRelayout
  - 0x4bbe90 HudUiCompositePanel::ReapplyEntryCount
  - 0x4bbed0 HudUiCompositePanel::ResizeEntryCount
  - 0x4bbff0 HudUiCompositePanelVector::InsertCopies
- Next action:
  - `python tools/recoil_status.py 0x403930`

### Group: GameNet gameplay packet handler band

- Anchor: 0x4321b0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4321b0 pending
  - 0x4320f0 GameNet::ResetRemotePlayersAndSpawnLists
  - 0x4330f0 GameNet::SendPkt0E_PlayerLapProgress
  - 0x433170 GameNet::HandlePkt0E_PlayerLapProgress
  - 0x433250 GameNet::HandlePkt0D_HudTimerPanelState
  - 0x433310 GameNet::SendPkt0D_HudTimerPanelState
  - 0x4343f0 GameNet::HandlePkt13_EffectAnimActivationRecord
- Next action:
  - `python tools/recoil_status.py 0x4321b0`

### Group: HudSensorTracker mission-save queue

- Anchor: 0x418fb0 HudSensorTracker::SaveAndQueueMissionState
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x418fb0 HudSensorTracker::SaveAndQueueMissionState
  - 0x419010 HudSensorTracker::QueueMissionFmvStateForMissionId
  - 0x41f010 Player::BuildMissionSaveData
- Next action:
  - `python tools/recoil_status.py 0x418fb0`

### Group: zEffectAnim stop and activation closure

- Anchor: 0x45d7b0 zEffectAnim::SetTransformRotAndVelocity
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x45d7b0 zEffectAnim::SetTransformRotAndVelocity
  - 0x459280 zEffect::HandleLightAnimEvent
  - 0x459510 zEffect::HandleFogEvent
  - 0x459580 zEffect::HandleCameraParamsEvent
  - 0x4596c0 zEffect::AnimateCameraParamsOverTime
  - 0x459ae0 zEffect::HandleRotationEvent
  - 0x459cb0 zEffect::HandleNodeScaleEvent
  - 0x459ce0 zEffect::HandlePositionEvent
  - 0x459e30 zEffect::HandleActivateEvent
  - 0x458c10 zEffect::UpdateBeamNodeBetweenPoints
  - 0x458ce0 zEffect::UpdateBeamNodeBetweenFractions
  - 0x443c50 zClass_cls_di::SetBreakOnFirstCandidate
  - 0x45c040 zEffectAnim::Stop
  - 0x45c100 zEffect::HandleNamedAnimStopEvent
  - 0x45c1a0 zEffect::HandleEmitterPlayEvent
  - 0x45c3c0 zEffect::HandleConditionalChainEvent
  - 0x45c530 zEffect::TraceUpwardHitFromNodeOrPos
  - 0x45cbc0 zEffect::HandleTopMessageEvent
  - 0x45ae30 zEffect_Anim::AdvanceKeyframeSample
  - 0x45ae90 zEffect_Anim::AnimateKeyframeSample
  - 0x45b120 zEffect_Anim::AdvanceKeyframe
  - 0x45b210 zEffect_Anim::EvaluateKeyframe
  - 0x45b280 zEffect_Anim::RunKeyframes
  - 0x45b4a0 zEffect::HandleDetachEvent
  - 0x45c710 zEffect::HandleScreenColorFxEvent
  - 0x45c920 zEffect::HandleScreenOverlayFxEvent
  - 0x45b8b0 zEffect::HandleTransformRefsEvent
  - 0x45bc60 zEffect::HandleSurfaceRefEvent
  - 0x476480 zMath::ProjectPointAndClampToScreenClip
  - 0x4bdc00 zVideoFxPass3Slot::SetRectAndPayload
  - 0x4bed90 zVideo_FxPass3Config_QueueElementLocal
  - 0x4bed50 zVideo_FxPass3Config_SetPrimaryElementParamsLocal
  - 0x4beee0 zVideo_FxPass3_SetPrimaryElementParamsLocal
  - 0x4bef10 zVideo_FxPass3_QueueElementLocal
  - 0x4498e0 gwNode::GetWorldPosAndOrientation
  - 0x474d90 zMath_Vec3_ElevationAngleBetweenPoints
  - 0x475910 zMath_Quat_Multiply
  - 0x475b80 zMath_Quat_FromRotationVector
  - 0x45cc00 zEffect_Anim::RunSequenceEvents
  - 0x45d010 zEffect_Anim::RunSequence
  - 0x45d3d0 zEffectAnim::FinalizeStop
  - 0x45d4c0 zEffectAnim::RunStopSequenceCallback
  - 0x45d570 zEffectAnim::StopAndCleanup
  - 0x45d6b0 zEffect_Anim::NodeActionCallback
  - 0x45d770 zEffectAnim::RunStopDelayCallback
  - 0x45dcb0 zEffectAnim::SetVelocity
  - 0x45dde0 zEffectAnim::SetVelocity_Thunk
  - 0x45de00 zEffectAnim::SetPositionRefAndVelocity
  - 0x45df70 zEffectAnim::SetPositionRefAndVelocity_Thunk
  - 0x45df90 zEffectAnim::SetTransformRefs
  - 0x45e0b0 zEffectAnim::SetTransformRefs_Thunk
  - 0x45d930 zEffectAnim::ActivateRuntime
  - 0x45dc70 zEffectAnim::SetTransformRotAndVelocity_Thunk
  - 0x45a9d0 zEffect::AnimateNodeOverTime
  - 0x474580 zMath_Vec3_DirFromYaw
  - 0x45a920 zEffect::FindNearestPickCandidateBelowPoint
  - 0x459e70 zEffect::HandleNodeAnimEvent
- Next action:
  - `python tools/recoil_status.py 0x45d7b0`

### Group: OptCatalog runtime instance free-list leaves

- Anchor: 0x4ae660 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4ae660 pending
  - 0x4ae4b0 OptCatalog::AllocOrReuseAttachNodeChildClone
  - 0x4ae520 OptCatalog::ClearRuntimeInstanceAsyncFxHandleCallback
  - 0x4ae530 OptCatalog::AllocOrReuseAttachNodeClone
  - 0x4aeaa0 OptCatalog::SpawnRuntimeInstanceAt
  - 0x4b2520 Light::AllocFromFreeListAndAttach
  - 0x45e0d0 zEffectAnimEntry::SetOnStateDoneCallback
  - 0x45e200 zEffect::SetWorldNode
  - 0x45e210 zEffect_Anim::SetZbdFilename
  - 0x45e270 zEffect::SetResourceNode
  - 0x45ffa0 zEffectAnim::FindNextAsyncEntry
  - 0x460010 zEffectAnim::GetRootNodeOrNull
  - 0x462280 zEffect::FindTemplateIndexByName
- Next action:
  - `python tools/recoil_status.py 0x4ae660`

### Group: Player alt-gun fire-point selection

- Anchor: 0x43aa30 Player::SelectAltGunFirePointAndSlot
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x43aa30 Player::SelectAltGunFirePointAndSlot
  - 0x43afd0 Player::ComposeAimBasisWorldMatrix
  - 0x43c190 pending
- Next action:
  - `python tools/recoil_status.py 0x43aa30`

### Group: zClass recursive teardown dependencies

- Anchor: 0x451a60 zClass_Util::DestroyNodeRecursive
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x481570 zDi::PtrToIndexOrMinus1
  - 0x4815a0 zDi::IndexToPtrOrNull
- Next action:
  - `python tools/recoil_status.py 0x481570`

### Group: HudSensorTracker mission identity fields

- Anchor: 0x4177a0 HudSensorTracker::SetMissionId
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4177a0 HudSensorTracker::SetMissionId
- Next action:
  - `python tools/recoil_status.py 0x4177a0`

### Group: HudSensorTracker map shutdown helpers

- Anchor: 0x417d40 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x417d40 pending
  - 0x415ac0 HudSensorMapNode::FreePointArray
  - 0x415d30 HudSensorMapNode::DrawOnTracker
  - 0x416480 HudSensorMapNode::DrawProjectedPath
  - 0x416660 HudSensorTracker::Init
  - 0x4167a0 HudSensorTracker::MapShutdownAndReset
  - 0x4167e0 HudSensorTracker::MapRemoveNode
  - 0x416ad0 HudSensorTracker::MapOverlayEndShow
  - 0x417130 HudSensorTracker::Update
  - 0x416ef0 HudSensorTracker::SetSaveStateMarkerMaxDistance
  - 0x417220 HudSensorTracker::SetTrackedSaveState
  - 0x4176f0 HudSensorTracker::ResetMissionState
  - 0x443c60 zClass_cls_di::SetStopAfterFirstHit
  - 0x444de0 zClass_cls_di::RaycastSelectClosestHitBetweenPoints
  - 0x444e90 zClass_cls_di::RaycastFindClosest
  - 0x4455f0 zClass_cls_di::BuildPickCandidatesForSegment
  - 0x445650 zClass_cls_di::BuildPickCandidatesForSegmentChildFallback
  - 0x445a00 zClass_cls_di::BuildPickCandidatesForSegmentRecursive
  - 0x445b20 zClass_cls_di::BuildPickCandidatesForSegmentForCamera
  - 0x445c20 zClass_cls_di::BuildPickCandidatesForSegmentForLight
  - 0x447540 zClass_cls_di::FilterPointsBBox
  - 0x485380 zClass_cls_di::BuildPickCandidatesForSegmentVsBBoxFaces
  - 0x4857f0 zClass_cls_di::BuildPickCandidateForSegmentVsPolygon
  - 0x484fc0 zClass_cls_di::AppendPickCandidatesForFace
  - 0x485d10 zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv
  - 0x472670 zMath::Vec3DeltaLengthSq
  - 0x475130 zMath_SolveLinearGradient2D
  - 0x4bd720 zMath::ClipLineSegmentToZRange
  - 0x4bd800 zMath::ClipLineSegmentPointToZ
  - 0x479f90 zClipAlt::SetTargetRect
  - 0x479c90 OptCatalog_SetDamageMaskUv
- Next action:
  - `python tools/recoil_status.py 0x417d40`

### Group: HudSensorMapNode leaf helpers

- Anchor: 0x415ab0 HudSensorMapNode::Init
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x415ab0 HudSensorMapNode::Init
  - 0x415ae0 HudSensorMapNode::SetEnabled
  - 0x415b10 HudSensorMapNode::SelectPoint
  - 0x415b40 HudSensorMapNode::InitDefaults
  - 0x415b70 HudSensorMapNode::SetColorRgb
  - 0x415bd0 HudSensorMapNode::LoadFromStream
  - 0x415c90 HudSensorMapNode::UpdateCachedBounds
- Next action:
  - `python tools/recoil_status.py 0x415ab0`

### Group: CZRecoilFrame About dialog

- Anchor: 0x430c30 CZRecoilFrame::OnMenuAbout
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x430c30 CZRecoilFrame::OnMenuAbout
  - 0x401000 CAboutDlg::CAboutDlg
- Next action:
  - `python tools/recoil_status.py 0x430c30`

### Group: RecoilApp PlayState activation

- Anchor: 0x42eed0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x42eed0 pending
  - 0x42f280 pending
  - 0x42f5e0 pending
  - 0x42f8a0 RecoilApp_PlayState::OnResume
  - 0x42f8e0 pending
  - 0x411760 HudUiMgrObjective::SetVisibleAndResetMeterFill
  - 0x411900 HudUiMgrObjective::Show
  - 0x416a30 HudSensorTracker::MapOverlayBeginShow
  - 0x416b30 HudSensorTracker::MapOverlayRefToggle
  - 0x416b80 HudSensorTracker::MapZoomIn
  - 0x416bb0 HudSensorTracker::MapZoomOut
  - 0x416be0 HudSensorTracker::UpdateMapScaleLerp
  - 0x416c90 HudSensorTracker::ProjectWorldPointsToOverlay
  - 0x416d50 HudSensorTracker::DrawTrackedSaveStateMarker
  - 0x416dd0 HudSensorTracker::DrawMarkerCross
  - 0x415f40 HudSensorTracker::DrawDiamondMarker
  - 0x416e50 HudSensorTracker::GetSaveStateRelativeVectorLen
  - 0x416f10 HudSensorTracker::DrawSaveStateMarker
  - 0x415fb0 HudRectI::ClipOrSplitSegment
  - 0x416240 HudRectI::CalcOutcode
  - 0x416290 HudRectI::IsCornerOutcode
  - 0x4162b0 HudRectI::SegmentIntersectsEdge
  - 0x416390 HudGeom2D::ClassifyPointAgainstSegment
  - 0x4bd6f0 HudLineClip::SetCurrentBoundsFromRectI
  - 0x4bd840 HudLineClip::ClipSegmentToCurrentBounds
  - 0x4bd880 HudLineClip::ClipSegmentToCurrentXBounds
  - 0x4bd9c0 HudLineClip::ClipEndpointToX
  - 0x4bd9f0 HudLineClip::ClipSegmentToCurrentYBounds
  - 0x4bdb30 HudLineClip::ClipEndpointToY
  - 0x416840 HudSensorTracker::MapInsertNodeAndGrowBounds
  - 0x4168d0 HudSensorTracker::LoadMapFromStream
  - 0x4169d0 HudSensorTracker::LoadMapFromPath
  - 0x416650 HudSensorTracker::InitNoBounds
  - 0x416790 HudSensorTracker::MapShutdownAndResetThunk
  - 0x417360 HudSensorTracker::ConstructGlobal
  - 0x417370 HudSensorTracker::RegisterGlobalOnExit
  - 0x417380 HudSensorTracker::ShutdownGlobal
  - 0x417390 HudSensorTracker::Constructor
  - 0x417260 HudSensorTracker::LoadMissionMapAndSfx
  - 0x4172c0 HudSensorTracker::SetObjectiveMarkerEnabledAndColor
  - 0x417300 HudSensorTracker::SetObjectiveMarkerColorBlink
  - 0x417ca0 HudSensorTracker::OnObjectiveCommand
  - 0x417f90 HudSensorTracker::LoadObjectivesFromPath
  - 0x418230 HudSensorTracker::LoadObjectivesFromZrd
  - 0x4184e0 HudSensorTracker::AdvanceObjectiveState
  - 0x418620 HudSensorTracker::SetObjectiveReviewVisible
  - 0x418730 HudSensorTracker::Command_ToggleObjectivePanel
  - 0x418760 HudSensorTracker::SetObjectivePanelVisible
  - 0x4188f0 HudSensorTracker::Command_ShowObjectivePickupInfo
  - 0x418940 HudSensorTracker::ShowObjectivePickupInfo
  - 0x4186f0 HudSensorTracker::GetObjectiveBriefingStringsAndImageRef
  - 0x418c30 HudSensorTracker::FindAndHighlightFirstIncompleteObjective
  - 0x418d40 HudSensorTracker::UpdateObjectiveFlow
  - 0x419380 HudSensorTracker::OnObjectiveReadSoundEvent
  - 0x4192d0 HudSensorTracker::RunStartAnimsFromZrd
  - 0x4193c0 HudSensorTracker::LoadRaceCheckpointMeta
  - 0x419470 HudSensorTracker::SetRuntimeTimerSecAndGoalValue
  - 0x419490 HudSensorTracker::Shutdown
  - 0x418c70 HudSensorTracker::ResetHudForMissionStart
  - 0x42bf40 HudUi::PlayPowerupSfx
  - 0x41ea00 Pickup::FindOptMetaImageByOptEntry
  - 0x402080 Player::AiRestorePreviousTopLevelState
  - 0x402f10 Player::AiFinalizeMode2State1ForAllPlayers
  - 0x447bc0 zClass_Class::FindNodeRecursiveByName
  - 0x479cb0 OptCatalog_SetDamageMaskEnabled
  - 0x437d40 zTurret_System::DisableTickCallback
  - 0x4a10b0 zSnd::MulGlobalVolumeScaleAndGetPrev
  - 0x4a10d0 zSnd::SetFlag10PlaybackEnabled
  - 0x4a1240 zSndSample::SetPlaybackEventHandler
  - 0x49fec0 zSndSample::StopActiveVoicesIfPlaying
  - 0x4a6ca0 zVid_PackColor00RRGGBB
- Next action:
  - `python tools/recoil_status.py 0x42eed0`

### Group: CZRecoilFrame menu shell handlers

- Anchor: 0x430740 CZRecoilFrame::OnMenuStartSinglePlayer
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x430740 CZRecoilFrame::OnMenuStartSinglePlayer
  - 0x430760 CZRecoilFrame::OnMenuOpenCampaign
  - 0x430770 CZRecoilFrame::OnOpenFileDialog
  - 0x4308a0 CZRecoilFrame::OnMenuExitGame
  - 0x4309b0 CZRecoilFrame::OnMenuSetVideoMode2
  - 0x4309d0 CZRecoilFrame::OnMenuSetVideoMode3
  - 0x4309f0 CZRecoilFrame::OnMenuSetVideoMode4
  - 0x430a30 CZRecoilFrame::OnMenuSetVideoMode6
  - 0x430a50 CZRecoilFrame::OnMenuSetVideoMode7
  - 0x430a70 CZRecoilFrame::OnMenuToggleHud
  - 0x430a90 CZRecoilFrame::OnUpdateHudCmdUI
  - 0x430ab0 CZRecoilFrame::OnMenuToggleFullscreen
  - 0x430ad0 CZRecoilFrame::OnMenuOpenHelpDocs
  - 0x431270 CZRecoilFrame::OnMenuStartMultiplayer
  - 0x431290 CZRecoilFrame::OnMenuStartCampaignMode
  - 0x4312b0 CZRecoilFrame::OnMenuStartCampaignMode2
  - 0x4312d0 CZRecoilFrame::OnMenuStartCampaignMode3
  - 0x4312f0 CZRecoilFrame::OnMenuStartCampaignMode4
  - 0x431310 CZRecoilFrame::OnMenuStartCampaignMode5
  - 0x431330 CZRecoilFrame::OnMenuToggleArchiveBanks
  - 0x431380 CZRecoilFrame::OnMenuToggleTexturePacks
  - 0x46d5b0 zVid::SetTexturePackLoadState
  - 0x4313d0 CZRecoilFrame::OnUpdateVideoMode2CmdUI
  - 0x431b10 CZRecoilFrame::OnSize
  - 0x430c30 CZRecoilFrame::OnMenuAbout
- Next action:
  - `python tools/recoil_status.py 0x430740`

### Group: zSnd fade tick list leaves

- Anchor: 0x49f620 zSnd_Tick
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x49f620 zSnd_Tick
  - 0x49f614 zSnd_TickWrapper
  - 0x4a3ad0 zSndFadeEntry::TickAndMaybeDispatch
  - 0x4a3c20 zSndFadeActiveList_TickAll
- Next action:
  - `python tools/recoil_status.py 0x49f620`

### Group: zSnd sample-set registry init

- Anchor: 0x4a0810 zSnd_SetUseArchiveBanks
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4a0810 zSnd_SetUseArchiveBanks
  - 0x4a0800 zSnd_SetUseArchiveBanksAndRegisterAtExit
  - 0x4a0830 zSndSampleSetRegistry_RegisterAtExit
  - 0x4a0840 zSndSampleSetRegistry_Shutdown
- Next action:
  - `python tools/recoil_status.py 0x4a0810`

### Group: zMath projection batches

- Anchor: 0x476cf0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476cf0 pending
  - 0x474b70 zMath_ProjectSphereBatch
  - 0x474bc0 zMath_UnprojectPointBatch
  - 0x474c20 zMath_UnprojectPointBatchZBuf
- Next action:
  - `python tools/recoil_status.py 0x476cf0`

### Group: zMath renderer point transforms

- Anchor: 0x476cf0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476cf0 pending
  - 0x472fa0 zMath::MatLoadCameraScratchA
  - 0x472fb0 zMath_Mat_LoadProjection
  - 0x4731f0 zMath_Mat_SetupCamera
  - 0x4747d0 zMath::MatTransformPointBatchInPlace
- Next action:
  - `python tools/recoil_status.py 0x476cf0`

### Group: zMath camera quaternion helpers

- Anchor: 0x473060 zMath_Mat_LoadView
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x473060 zMath_Mat_LoadView
  - 0x4757c0 zMath_Quat_FromEuler
  - 0x4759d0 zMath_Quat_MultiplyInverse
  - 0x475a80 zMath_Quat_ToMatrix
- Next action:
  - `python tools/recoil_status.py 0x473060`

### Group: zMath vector scalar helpers

- Anchor: 0x487f10 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x487f10 pending
  - 0x4727a0 zMath_Vec3_DivScalar
- Next action:
  - `python tools/recoil_status.py 0x487f10`

### Group: zModel light depth fade attrs

- Anchor: 0x476cf0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476cf0 pending
  - 0x490330 zFloat::Set255f
  - 0x4896d0 zModel_Light::BuildAttr0DepthFade
  - 0x489920 zModel_Light::EvalBatchSphereFade
- Next action:
  - `python tools/recoil_status.py 0x476cf0`

### Group: zModel light per-vertex weights

- Anchor: 0x476cf0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476cf0 pending
  - 0x488d60 pending
  - 0x49b4c0 zRndr::CommitDirectFogParamsIfChanged
  - 0x49b710 zRndr::CommitStagedFogParamsIfChanged
  - 0x49b780 zRndr::BlendPackedColor565WithFogInPlace
- Next action:
  - `python tools/recoil_status.py 0x476cf0`

### Group: zModel fog sphere fade

- Anchor: 0x489540 zModel_Light::EvalSphereFogFade
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x489540 zModel_Light::EvalSphereFogFade
  - 0x473fc0 zMath::Vec3ArrayProjectToCachedY
- Next action:
  - `python tools/recoil_status.py 0x489540`

### Group: zRndr lens flare projected sample queue

- Anchor: 0x476cf0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476cf0 pending
  - 0x498cb0 zRndr::LensFlare_DrawQueuedSample16_ClippedFramebuffer
  - 0x49a830 zRndr_LensFlare_QueueProjectedSample
  - 0x49a910 zRndr::LensFlare_ResetSampleQueue
  - 0x49aa30 zRndr_SpanOcclusion_FilterSampleList
  - 0x49aa40 zRndr_LensFlare_SetVisibleSampleStage
- Next action:
  - `python tools/recoil_status.py 0x476cf0`

### Group: zModel scrolling texture UV update

- Anchor: 0x476cf0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476cf0 pending
  - 0x478fc0 zModel_Instance_UpdateScrollingTexturesIfNeeded
  - 0x4791c0 zModel_Instance_UpdateScrollingTextures
- Next action:
  - `python tools/recoil_status.py 0x476cf0`

### Group: zModel point queue entry rendering

- Anchor: 0x476cf0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476cf0 pending
  - 0x479020 zModel_RenderPointQueueEntry
- Next action:
  - `python tools/recoil_status.py 0x476cf0`

### Group: zRndr fog commit and packed blend helpers

- Anchor: 0x488d60 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x488d60 pending
  - 0x49b4c0 zRndr::CommitDirectFogParamsIfChanged
  - 0x49b530 zRndr::CommitFogColorParamsIfChanged
  - 0x49b710 zRndr::CommitStagedFogParamsIfChanged
  - 0x49b780 zRndr::BlendPackedColor565WithFogInPlace
- Next action:
  - `python tools/recoil_status.py 0x488d60`

### Group: zRndr 565 span alpha blend leaf

- Anchor: 0x476cf0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476cf0 pending
  - 0x49c020 zRndr::SpanMasked16FromPal8To565
  - 0x49c150 zRndr::SpanMasked16FromTex16To565
  - 0x49c230 zRndr::SpanAlphaBlend565ConstAlphaFromPal8
  - 0x49c360 zRndr::SpanAlphaBlend565FromTex16Alpha8
  - 0x49c560 zRndr::SpanAlphaBlend555FromTex16Alpha8
  - 0x49c760 zRndr::SpanAlphaBlend565ConstAlphaFromTex16
  - 0x49c860 zRndr::SpanAlphaBlend555ConstAlphaFromTex16
  - 0x49c970 zRndr::SpanAlphaBlend565ConstAlphaFromTex16Alpha8
  - 0x49ca90 zRndr::SpanAlphaBlend555ConstAlphaFromTex16Alpha8
  - 0x49cbb0 zRndr::SpanAlphaBlend565MmxFromTex16Alpha8
  - 0x49cea0 zRndr::SpanAlphaBlend555MmxFromTex16Alpha8
  - 0x49d1a0 zRndr::SpanAlphaBlend565FromPal8Alpha8
  - 0x49d3b0 zRndr::SpanAlphaBlend555FromPal8Alpha8
  - 0x49d5c0 zRndr::SpanAlphaBlend565ConstAlphaFastFromPal8
  - 0x49d6e0 zRndr::SpanAlphaBlend555ConstAlphaFastFromPal8
  - 0x49d810 zRndr::SpanAlphaBlend565ConstAlphaFromPal8Alpha8
  - 0x49d950 zRndr::SpanAlphaBlend555ConstAlphaFromPal8Alpha8
  - 0x49da80 zRndr::SpanAlphaBlend565MmxFromPal8Alpha8
  - 0x49ddb0 zRndr::SpanAlphaBlend555MmxFromPal8Alpha8
  - 0x49e200 zRndr::FogBlendSpan565Scalar
  - 0x49e300 zRndr::FogBlendSpan555Scalar
  - 0x49e400 zRndr::FogBlendSpan565Mmx
  - 0x49e560 zRndr::FogBlendSpan555Mmx
  - 0x49e6c0 zRndr::SpanCopy16FromTex16SwitchVShift
  - 0x49b7e0 zRndr::SpanMasked16FromTex16SwitchVShift
  - 0x49ea40 zRndr::SpanMmxSetTexUvMasksAndVShift
  - 0x49ea80 zRndr::SpanCopy16FromTex16
  - 0x49ec20 zRndr::SpanCopy16FromTex16ExplicitVShift
  - 0x49edc0 zRndr::SpanCopy16FromPal8SwitchVShift
  - 0x49bbf0 zRndr::SpanMasked16FromPal8SwitchVShift
  - 0x49f180 zRndr::SpanShade16FromPal8SwitchVShift
  - 0x490ae0 zRndr_SpanOcclusion_InsertSpanNode_Local
  - 0x4912a0 zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest
  - 0x491840 zRndr_SpanOcclusion_BuildSpanList
  - 0x491da0 zRndr_SpanOcclusion_BuildSpanListFast
  - 0x4936d0 zRndr_RasterizePoly
  - 0x499a20 zRndr_SubmitPolyWithSpanList
  - 0x498f90 zRndr_SpanOcclusion_TestSample
  - 0x498fb0 zRndr_DrawCircleOutline16_Framebuffer
  - 0x499020 zRndr_DrawCircleOctants16_Framebuffer
  - 0x4992b0 zRndr_PlotPixel16
  - 0x4992d0 zRndr_DrawLine16
  - 0x4993a0 zRndr_DrawLine16_Segmented
  - 0x499500 zRndr_DrawLine16_Clipped
  - 0x4997d0 zRndr_FillSpan16Opaque
  - 0x499810 zRndr_FillSpan555Solid
  - 0x4998a0 zRndr_FillSpan565Solid
  - 0x499930 zRndr_SetPaletteRemapKey
  - 0x499990 zRndr_SetPaletteRemapKeyFromRgb01
  - 0x499a00 zRndr_SetPaletteShadeRecipeIndex
  - 0x499ec0 zRndr_SubmitTexturedPolyPerVertexAlphaOrShade
  - 0x46e720 zVid_PaletteRemap_BuildPaletteVariant
- Next action:
  - `python tools/recoil_status.py 0x476cf0`

### Group: zModel active-light sphere contribution

- Anchor: 0x476a50 zDi::EvalBoundingSphereLightingFlags
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476a50 zDi::EvalBoundingSphereLightingFlags
  - 0x4894f0 zModel_Light::EvalDistanceWeight
  - 0x487c50 zModel_Light::PointInPolygonTestRadiusXZ
- Next action:
  - `python tools/recoil_status.py 0x476a50`

### Group: zRndr direct fog target color

- Anchor: 0x49b350 zRndr::SetFogTargetColorRgb01Clamped
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x49b350 zRndr::SetFogTargetColorRgb01Clamped
  - 0x4a7300 zVideo::SetFogTargetColorFromRgb01
- Next action:
  - `python tools/recoil_status.py 0x49b350`

### Group: zClipRect near-z clipping

- Anchor: 0x476cf0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476cf0 pending
  - 0x47a200 zClipRect::ClipPolyZRange_NoUV
  - 0x47a4e0 zClipRect::ClipPolyZRange_NoUV_WithAttribs
  - 0x47aa80 zClipRect::ClipPolyNearZ
  - 0x47af60 zClipRect::ClipPolyNearZ_WithAttr0
  - 0x47b540 zClipRect::ClipPoly_NoUV_Alt
  - 0x47bd30 zClipRect::ClipPoly_NoUV_WithAttr012_Alt
  - 0x47cdc0 zClipRect::ClipPoly_NoUV
  - 0x47d3f0 zClipRect::ClipPoly
  - 0x47dfb0 zClipRect::ClipPoly_NoUV_WithAttr0_Alt
  - 0x47e900 zClipRect::ClipPolyZRange_WithAttr012
  - 0x47efd0 zClipRect::ClipPoly_WithAttr012
  - 0x4803b0 zClipRect::TrivialRejectPolyXY
- Next action:
  - `python tools/recoil_status.py 0x476cf0`

### Group: zReader movers load dependency chain

- Anchor: 0x420be0 zReader::LoadMoversFromZrd
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x420be0 zReader::LoadMoversFromZrd
  - 0x421da0 zClass_Node::PropagateExtraFlagsRecursive
  - 0x437e60 zClass_Node::SetContextRecursive
- Next action:
  - `python tools/recoil_status.py 0x420be0`

### Group: zReader path resolution leaves

- Anchor: 0x421e20 zReader::BuildResolvedParentDir
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x421e20 zReader::BuildResolvedParentDir
  - 0x48cd40 zReader::TryResolvePath
- Next action:
  - `python tools/recoil_status.py 0x421e20`

### Group: zModel material pool accessor

- Anchor: 0x4805e0 zModel_Matl::GetPoolEntry
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4805e0 zModel_Matl::GetPoolEntry
- Next action:
  - `python tools/recoil_status.py 0x4805e0`

### Group: zDi variant tag leaf

- Anchor: 0x476340 zDi::SetVariantTagIfUnset
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x476340 zDi::SetVariantTagIfUnset
- Next action:
  - `python tools/recoil_status.py 0x476340`

### Group: player HUD counter leaf

- Anchor: 0x42a9f0 Player::AddScaledHudCounterValue
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x42a9f0 Player::AddScaledHudCounterValue
- Next action:
  - `python tools/recoil_status.py 0x42a9f0`

### Group: Player mission runtime shutdown

- Anchor: 0x41fb80 Player::ShutdownMissionRuntime
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x41fb80 Player::ShutdownMissionRuntime
  - 0x4037c0 AINetNode::Free
  - 0x403800 AINet::Free
  - 0x403830 Player::AiDiscardNegativeBranchPathNodes
  - 0x403870 AINet::FreeAll
  - 0x41fd20 Player::DestroySaveGameState
  - 0x438430 zUtil_SaveGameState::FreeOwnedResources
  - 0x438b60 Player::FreeAltWeaponTrailRuntimeStates
- Next action:
  - `python tools/recoil_status.py 0x41fb80`

### Group: save-game state list leaves

- Anchor: 0x4383e0 zUtil_SaveGameStateList_Init
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4383e0 zUtil_SaveGameStateList_Init
  - 0x4384e0 zUtil_SaveGameStateList_AllocAppend
  - 0x438430 zUtil_SaveGameState::FreeOwnedResources
- Next action:
  - `python tools/recoil_status.py 0x4383e0`

### Group: zNetwork dispatch handler list

- Anchor: 0x48bfa0 zNetwork_InitMessageHandlers
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x48bfa0 zNetwork_InitMessageHandlers
  - 0x48bfb0 zNetwork_CreateEmptyDispatchHandlerList
  - 0x48bfe0 zNetwork_RegisterDispatchHandlerListShutdown
  - 0x48bff0 zNetwork_DestroyDispatchHandlerList
- Next action:
  - `python tools/recoil_status.py 0x48bfa0`

### Group: RecoilApp load-ZBD startup

- Anchor: 0x42e490 RecoilApp::LoadZbdAndStartEngine
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x417430 HudSensorTracker::WriteMissionDataSection
  - 0x417680 HudSensorTracker::ZarMission_SaveCallback
  - 0x4176d0 HudSensorTracker::ZarMissionLate_RestoreCallback
  - 0x4176b0 HudSensorTracker::ZarMissionLate_SaveCallback
  - 0x417800 HudSensorTracker::GetMissionId
  - 0x417690 pending
- Next action:
  - `python tools/recoil_status.py 0x417430`

### Group: ZAR read-side mount helpers

- Anchor: 0x48d210 zArchive::MountIndexArchive
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4a6270 zIndexArchive::OpenCreateWrite
- Next action:
  - `python tools/recoil_status.py 0x4a6270`

### Group: ZBD manager load helpers

- Anchor: 0x4c0030 zUtil::ZBD_LoadEntriesGlobal
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4c0030 zUtil::ZBD_LoadEntriesGlobal
  - 0x4c0050 zUtil::ZAR_LoadFileGlobal
  - 0x4c0070 zUtil::ZAR_RequestStopGlobal
  - 0x4c0260 zZbdSectionHandler::CompareSortOrderLessThan
  - 0x4c0370 zZbdManager::LoadEntries
  - 0x4c0400 zZbdManager::LoadZarFile
  - 0x4c0620 zZbdManager::RequestStop
  - 0x4c06a0 zZbdSectionHandler::InvokePreLoad
  - 0x4c06c0 zZbdSectionHandler::InvokeDataReady
  - 0x4c07d0 zZbdManager::SortSectionHandlers
- Next action:
  - `python tools/recoil_status.py 0x4c0030`

### Group: zSound stream request playback leaves

- Anchor: 0x4b5900 HudUiZrdWidget::OnActivate
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x49fd50 zSndSample::PlayDirectSound
  - 0x4a5220 zSndStreamRequest_MatchGroupPredicate
- Next action:
  - `python tools/recoil_status.py 0x49fd50`

### Group: ZAR write-side section records

- Anchor: 0x40fb90 HudUiTimerPanel::ZarWriteTimerDataCallback
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x40fb90 HudUiTimerPanel::ZarWriteTimerDataCallback
  - 0x4a64d0 zIndexArchive::AddFileRecord
  - 0x4c0630 zZbdManager::WriteSectionRecord
  - 0x4c0010 zUtil_ZAR::WriteSectionBlob
- Next action:
  - `python tools/recoil_status.py 0x40fb90`

### Group: HUD base element and container leaves

- Anchor: 0x40f4c0 HudUiMgr::InitHudLayouts
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x40bdc0 zUtil_StdPtrVector_Clear
  - 0x40c1d0 HudCmdBindButtonBase::ClearBindingEntries
  - 0x40dac0 HudUiCounter::Constructor
  - 0x40da00 HudUiMessage::Constructor
  - 0x40d590 HudUiMessage::Destructor
  - 0x40daa0 HudUiMessage::ScalarDeletingDestructor
  - 0x40db00 HudUiMgr::DestructMessagesArray
  - 0x40e070 HudUiTriplet::DestructorCore
  - 0x40e590 HudUiTriplet::AddEntry
  - 0x40e800 HudUiTriplet::UpdateEntryData
  - 0x40e880 HudUiTriplet::RemoveEntry
  - 0x414390 GameNet::RefreshPlayerListMenu
  - 0x4143b0 HudUi::RefreshScoreboardEntryRow
  - 0x4143c0 HudUi::RemoveScoreboardEntryRow
  - 0x40a590 HudUiPanel::ScalarDeletingDestructor
  - 0x40fab0 HudUiPanelSimple::ConstructorDefaultThunk
  - 0x40fac0 HudUiPanelSimple::Constructor
  - 0x40d9e0 HudUiMeter::ConstructorEx
  - 0x40f2d0 HudUiWidget::CtorDefaultThunk
  - 0x40d600 HudUiTripletPanel::UnwindDestructFirstItem
  - 0x40d610 HudUiTripletPanel::DestructorCore
  - 0x40d760 HudUiMgr::DestructModeCounterArray
  - 0x40f200 HudUiTripletPanel::Constructor
  - 0x40f460 HudUiTripletPanel::SetVisibleCount
  - 0x40fa10 HudUiStatsListElement::Update
  - 0x40fa20 HudUiStatsListElement::ScalarDeletingDestructor
  - 0x40fa40 HudUiStatsListElement::DestructorCore
  - 0x4b4390 HudUiTextInput::AllocTextBuffer
  - 0x4b42f0 HudUiTextInput::Constructor
  - 0x4b43d0 HudUiTextInput::SetContents
  - 0x4b4410 HudUiTextInput::GetBuffer
  - 0x4b4420 HudUiTextInput::SetCursorPosition
  - 0x4b4460 HudUiTextInput::DispatchKeyAction
  - 0x4b44e0 HudUiTextInput::InsertCharAtCursor
  - 0x4b4530 HudUiTextInput::BackspaceDeleteChar
  - 0x4b4550 HudUiTextInput::DeleteCharForward
  - 0x4b4560 HudUiTextInput::MoveCursorLeft
  - 0x4b4570 HudUiTextInput::MoveCursorRight
  - 0x4b4590 HudUiTextInput::ShiftTextRight
  - 0x4b45e0 HudUiTextInput::ShiftTextLeft
  - 0x4b4370 HudUiTextInput::DestructorCore
  - 0x40d660 HudUiMgrObjectiveBlock::Destructor
  - 0x40db20 HudUiSlot::Constructor
  - 0x40db90 HudUiSlot::Draw
  - 0x40d780 HudUiSlot::Destructor
  - 0x40dbd0 HudUiSlot::ScalarDeletingDestructor
  - 0x40eca0 HudUiTimerPanel::SetRunning
  - 0x40ecc0 HudUiTimerPanel::SetElapsedSeconds
  - 0x40ece0 HudUiTimerPanel::SetSeconds
  - 0x40ed10 HudUiTimerPanel::GetSeconds
  - 0x40ed20 HudUiTimerPanel::Update
  - 0x40ee60 HudUiTimerPanel::UpdateHMSFromSeconds
  - 0x40fbb0 HudUiTimerPanel::ZarReadTimerData
  - 0x4b49e0 HudUiNumericTextInput::BaseConstructor
  - 0x4b4a90 HudUiNumericTextInput::ScalarDeletingDestructor
  - 0x4b4ac0 HudUiNumericTextInput::Destructor
  - 0x4b4e40 HudUiNumericTextInput::AllocTextBuffer
  - 0x4b4e60 HudUiNumericTextInput::Update
  - 0x4b4ed0 HudUiNumericTextInput::GetBuffer
  - 0x4ba510 HudUiPanelPtrVector::InsertN
  - 0x4bc930 HudUiTransitionTextPanel::ResetFlashState
  - 0x4bc9b0 HudUiTransitionTextPanel::SetFlashColorAndRate
  - 0x4b6fc0 HudUiCheckToggleWidget::Constructor
  - 0x4b7000 HudUiCheckToggleWidget::ScalarDeletingDestructor
  - 0x4b7020 HudUiCheckToggleWidget::DestructorCore
  - 0x4b70b0 HudUiCheckToggleWidget::GetBoundsRectOrNull
  - 0x4b70c0 HudUiCheckToggleWidget::RefreshState
  - 0x4b7250 HudUiCheckToggleWidget::HidePreview
  - 0x4b72c0 HudUiCheckToggleWidget::SetChecked
  - 0x4b7d60 HudUiCycleSelectorWidget::Constructor
  - 0x4b7dc0 HudUiCycleSelectorWidget::ScalarDeletingDestructor
  - 0x4b7de0 HudUiCycleSelectorWidget::DestructorCore
  - 0x4b7e60 HudUiCycleSelectorWidget::Update
  - 0x4b7ee0 HudUiCycleSelectorWidget::AdvanceSelectionAndActivate
  - 0x4b7f20 HudUiCycleSelectorWidget::SetIndexClamped
  - 0x4b7f80 HudUiCycleSelectorWidget::SetVisibleRange
  - 0x4b7fd0 HudUiCycleSelectorWidget::AddTextEntry
  - 0x4b8100 HudUiCycleSelectorWidget::ApplyFontStyleForEntry
  - 0x4b8200 HudUiCycleSelectorWidget::AddBitmapEntry
  - 0x4b82e0 HudUiCycleSelectorWidget::LoadFromZrd
  - 0x4b8450 HudUiFillBitmap::Constructor
  - 0x4b84b0 HudUiFillBitmap::ScalarDeletingDestructor
  - 0x4b84d0 HudUiFillBitmap::DestructorCore
  - 0x4b8520 HudUiFillBitmap::Draw
  - 0x4b85c0 HudUiFillBitmap::LoadFromZrd
  - 0x4b8650 HudUiFillBitmap::UpdateNormalizedFromCursor
  - 0x4b86b0 HudUiFillBitmap::SetNormalizedValueAndRebuild
  - 0x4ba3c0 HudUiFillBitmap::SetNormalizedValue
  - 0x4b8760 HudUiZrdWidgetEx17C_Item::Constructor
  - 0x4b87a0 HudUiZrdWidgetEx17C_Item::ScalarDeletingDestructor
  - 0x4b87c0 HudUiZrdWidgetEx17C_Item::DestructorCore
  - 0x4b87d0 HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected
  - 0x4b87e0 HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected
  - 0x4b87f0 HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf
  - 0x4b8850 HudUiZrdWidgetEx17C_Item::LoadFromZrd
  - 0x4b8a90 HudUiZrdWidgetEx17C_Item::SetSelected
  - 0x4b8af0 HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds
  - 0x4b8b10 HudUiZrdWidgetEx17C::Constructor
  - 0x4b8b40 HudUiZrdWidgetEx17C::ScalarDeletingDestructor
  - 0x4b8b60 HudUiZrdWidgetEx17C::DestructorCore
  - 0x4b8be0 HudUiZrdWidgetEx17C::LoadFromZrd
  - 0x4b8cf0 HudUiZrdWidgetEx17C::SetSelectedIndex
  - 0x4b8d30 HudCmdBindButtonBase::Constructor
  - 0x4ba470 zUtil_StdPtrVector_FreeBufferAndReset
  - 0x4b8de0 HudCmdBindButtonBase::LoadFromZrd
  - 0x4b90e0 HudCmdBindButtonBase::RebuildBindingSlotWidgets
  - 0x4b92a0 HudUiListSelectorItem::Constructor
  - 0x4ba410 HudUiListSelectorItem::Draw
  - 0x4b4280 HudUiElement::SetTimer
  - 0x4b4120 HudUiElement::CopyFrom
  - 0x4b42c0 HudUiElement::GetRect
  - 0x4ba400 HudUiPanel::GetWrapRect
  - 0x4ba850 HudUiPanel::CopyConstructCore
  - 0x4bb3d0 HudUiPanel::HitTest
  - 0x4bb440 HudUiPanel::GetLastTextPtr
  - 0x4bb740 HudUiPanel::GetTextRect
  - 0x40be90 HudUiPanel::Invalidate
  - 0x40bea0 HudUiPanel::GetFont
  - 0x40beb0 HudUiPanel::SetFontHandle
  - 0x40bec0 HudUiPanel::EnableWordWrapWithRect
  - 0x40bf00 HudUtil::FreeFieldPtr
  - 0x4bb460 HudUiPanel::Draw
  - 0x4bb5e0 HudUiPanel::SetTextFmtV
  - 0x4bb680 HudUiPanel::SetText
  - 0x4bcc80 HudUiTextLabel::Constructor
  - 0x4ba9e0 HudUiPanel::ConstructorCopy
  - 0x404e60 HudUiCircle::HitTest
  - 0x4bc480 HudUiCircle::Constructor
  - 0x4bc4e0 HudUiCircle::HitTestCore
  - 0x4bbfa0 HudUiCompositePanelVector::Clear
  - 0x4bc320 HudUiCompositePanelEntry::ConstructorCopyRange
  - 0x4bc3a0 HudUiCompositePanelEntry::AssignCopy
  - 0x4bc410 HudUiCompositePanelEntry::ConstructorCopy
  - 0x4bb0c0 HudUiFlashPanel::ComputeFlashBlendColor
  - 0x4bc9f0 HudUiTransitionTextPanel::TickFlash
  - 0x4bf7c0 HudUiMessageBoxDialog::OnOk
  - 0x4bf7e0 HudUiMessageBoxDialog::OnCancel
  - 0x4bf800 HudUiMessageBoxOkButton::OnActivate
  - 0x4bf820 HudUiMessageBoxCancelButton::OnActivate
  - 0x4bc510 HudUiBackgroundContainer::Constructor
  - 0x4bc540 HudUiBackgroundContainer::Destructor
  - 0x4b9850 HudUiBackground::SetEnabled
  - 0x4ba4a0 HudFontStyle::Constructor
  - 0x4ba4c0 HudFontStyle::Destructor
  - 0x4b4b30 HudUiNumericTextInput::RawKeyboardCallback
  - 0x4b4b50 HudUiNumericTextInput::OnRawKeyboardChar
  - 0x4b4ba0 HudUiNumericTextInput::SetInputActive
  - 0x4b4c50 HudUiNumericTextInput::SetRawKeyboardCapture
  - 0x4b4ca0 HudUiNumericTextInput::UpdateCaptureUiAndClip
  - 0x4ba3e0 HudUiOwnedTextInput::OnAcceptNotifyOwner
  - 0x4b3ce0 HudUiWidget::ScalarDeletingDestructor
  - 0x404e10 HudUiWidget::RebuildBltRectFromImage
  - 0x4b5310 HudUiZrdWidget::Invalidate
  - 0x4b5350 HudUiZrdWidget::GetBoundsRectOrNull
  - 0x4b5740 HudUiZrdWidget::RefreshState
  - 0x4b5860 HudUiZrdWidget::HidePreview
  - 0x4b40c0 HudUiElement::CopyConstructor
  - 0x4b47a0 HudUiElement::ResetCommonFTable
  - 0x404d50 HudUiElement::GetX
  - 0x404d60 HudUiElement::GetY
  - 0x404d70 HudUiElement::ScalarDeletingDestructor
  - 0x4bcbe0 HudUiTextLabel::CopyConstructor
  - 0x4bf840 HudUiPolyline::Constructor
  - 0x4bf8b0 HudUiPolyline::SetPoint
  - 0x4bf900 HudUiPolyline::Draw
  - 0x4b4620 HudUiSliderBorder::Constructor
  - 0x4b47b0 HudUiSliderBorder::Update
  - 0x4b4810 HudUiSliderBorder::SetBounds
  - 0x4bac10 HudUiPanel::RebuildTextRect
  - 0x4bb1c0 HudUiPanel::MeasureTextPrefixRect
  - 0x4bb2a0 HudUiPanel::UpdateTextBoundsFromContent
  - 0x4bb710 HudUiPanel::QueryTextHeight
  - 0x4bc810 HudUiContainer::FindChildWithPrev
  - 0x4bc860 HudUiContainer::RemoveChild
  - 0x4bc900 HudUiContainer::UpdateAll
  - 0x4138d0 HudUi::ShowTopMessageLine
  - 0x4138f0 HudUi::ShowChatLine
  - 0x4bd160 HudUiTextStack4::PushLine
  - 0x4bd280 HudUi::PushTopMessageLine
  - 0x40f2b0 HudUiTripletPanel::ScalarDeletingDestructor
  - 0x40f130 HudUiCounter::UpdateLayoutPosition
  - 0x40d3b0 HudLayoutBase::Destructor
  - 0x412bd0 HudLayoutBase::SetActiveNoOp
  - 0x412be0 HudLayoutBase::UpdateAll
  - 0x4126e0 HudUiMessage::SelectVariantDisplay
  - 0x412790 HudUiMessage::ApplySideImageSwap
  - 0x4127d0 HudUiMessage::ClearDisplay
  - 0x411a20 HudUiMgrObjective::Begin
  - 0x410140 HudUiMgr::TickLayoutDelay
  - 0x411750 HudUiMgr::SetNanitePanelCount
  - 0x411710 HudUiMgr::ReticleStaticAtexitStub
  - 0x411720 HudUiMgr::CopyReticleProjection
  - 0x411740 HudUiMgr::SetReticleMode
  - 0x411270 HudUiMgr::UpdateTargetReticleFromCursor
  - 0x4089c0 HudUiMgr::ScreenToWorld
  - 0x408430 zOpt::ViewRectSection_ClampPointToInclusiveBounds
  - 0x471de0 zInput::PollActiveDevices
  - 0x4b7340 HudUiCheckToggleWidget::LoadFromZrd
  - 0x4b7290 HudUiCheckToggleWidget::OnActivate
  - 0x4b7210 HudUiCheckToggleWidget::ShowPreview
  - 0x4b59f0 HudUiZrdWidget::LoadFromZrd
  - 0x4ba020 HudUiTransitionTextPanel::Constructor
- Next action:
  - `python tools/recoil_status.py 0x40bdc0`

### Group: RecoilApp shutdown engine wrapper

- Anchor: 0x42e430 RecoilApp::ShutdownEngine
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x48b820 zNetwork_ApplyPkt01_PlayerColorAssignments
  - 0x46f970 zInput::Keyboard_SetRawEventCallback
  - 0x471f60 zInput::DI_EnumDevicesCallback_SelectFirstJoystick
  - 0x458af0 zEffect::SetConditionalRefPos
  - 0x458e10 zEffect::HandleSampleRefOffsetEvent
  - 0x458eb0 zEffect::HandleEffectTemplateOffsetEvent
  - 0x458f70 zEffect::HandleSoundEvent
  - 0x459080 zEffect::HandleLightEvent
  - 0x45c240 zEffect::HandleEmitterStopEvent
  - 0x45c2f0 zEffect::HandleEmitterResetEvent
  - 0x45c310 zEffect::HandleEmitterLoopEvent
  - 0x45c640 zEffect::GetConditionalRefPosDistanceSq
  - 0x45c6b0 zEffect::SkipConditionalChainToEnd
  - 0x45c6e0 zEffect::HandleNoOpMarkerEvent
  - 0x45c6f0 zEffect::HandleCallbackEvent
  - 0x45bf60 zEffect::CleanupLightRefs
  - 0x45bfd0 zEffect::CleanupSoundRefs
  - 0x45d240 zEffect_Anim::CaptureNodeStates
  - 0x45d310 zEffect_Anim::RestoreNodeStates
  - 0x45d6c0 zEffectAnim::ResetForNode
  - 0x447dc0 zClass_Class::gwNodeSetName
  - 0x447e30 zClass_Class::gwNodeGetName
  - 0x4483f0 zClass_Class::AddChild
  - 0x4497b0 gwNode::GetWorldPosition
  - 0x449850 gwNode::TransformPoint
  - 0x4510e0 zClass_World::AddChildAtGrid
  - 0x451360 zClass_World::AddLight
  - 0x451590 zClass_World::AddSound
  - 0x451b20 zClass_cls_util::CopyNodeDisplayInstance
  - 0x451bd0 zClass_cls_util::CopyNodeBaseData
  - 0x451f70 zClass_cls_util::CopyCameraNode
  - 0x4520c0 zClass_cls_util::CopyLightNode_Unimplemented
  - 0x4520e0 zClass_cls_util::CopySoundNode_Unimplemented
  - 0x452100 zClass_cls_util::CopyObject3DNode
  - 0x452230 zClass_cls_util::CopyAnimateNode_Unimplemented
  - 0x452250 zClass_cls_util::CopyLodNode
  - 0x4523c0 zClass_cls_util::CopySequenceNode_Unimplemented
  - 0x4523e0 zClass_cls_util::CopySwitchNode_Stub
  - 0x452400 zClass_cls_util::CopyNodeDispatch
  - 0x452500 zClass_cls_util::CopyNodeWithCloneOptions
  - 0x452560 zClass_cls_util::CopyNode
  - 0x453b40 zClass_Animate::AddChild
  - 0x4529c0 zClass_Sound::gwSoundNew
  - 0x452bc0 zClass_Sound::SetSampleSetByName
  - 0x452d00 zClass_Sound::gwSoundSetPosition
  - 0x480c80 zModel_Material::HasAuxData
  - 0x480600 zModel_MatlBuffer::WriteGameZ
  - 0x4808c0 zModel_MatlBuffer::ReadGameZ
  - 0x4812b0 zModel_Material::Clone
  - 0x4812c0 zModel_MatlBuffer::CloneToActiveSlot
  - 0x4815c0 zModel_DiPool::WriteToStream
  - 0x481bc0 zModel_DiPool::ReadHeaderFromStream
  - 0x481c50 zModel_DiPool::ReadEntryDynamicDataFromStream
  - 0x481fa0 zModel_DiPool::ReadFromStream
  - 0x482080 zModel_DiPool::AllocFromFreeList
  - 0x482270 zDi::CloneToInstance
  - 0x4826b0 zDi::SetClonedFlag
  - 0x483a60 zDi::HasSpecialFlagsOrAuxMaterialData
  - 0x484230 zDi::ResetCurrentVariant
  - 0x45ff10 zEffectAnim::FindEntryByName
  - 0x45d7a0 zEffectAnim::ResetActivationPrereqCount
  - 0x45db20 zEffectAnim::CheckActivationPrereqs
  - 0x45e280 zEffectAnim::FindSoundRefIndexByName
  - 0x45e300 zEffectAnim::FindLightRefIndexByName
  - 0x45e380 zEffectAnim::FindOrCreateSoundRef
  - 0x45e4a0 zEffectAnim::FindOrCreateLightRef
  - 0x45e5c0 zEffectAnim::ResolveNodeByName
  - 0x45e650 zEffectAnim::FindNodeRecursiveByName
  - 0x45e6d0 zEffectAnim::EnsureCopiedRootTree
  - 0x45e730 zEffectAnim::CloneEntryForNode
  - 0x45ed80 zEffectAnim::RebindEntryToNode
  - 0x460400 zEffect_Anim::HasActivationRecord
  - 0x460470 zEffect_Anim::GetActivationRecordCount
  - 0x460480 zEffect_Anim::GetActivationRecordAt
  - 0x460ae0 zEffect_Anim::AllocActivationRecord
  - 0x461800 zEffect_Anim::GetActivationRecordPackedSize
  - 0x461970 zEffectAnim::QueueCmdType1TransformRotVelocity
  - 0x461a90 zEffect_Anim::DiscardLastActivationRecord
  - 0x461aa0 zEffectAnim::QueueCmdType2Velocity
  - 0x461ba0 zEffectAnim::QueueCmdType3PositionRefAndVelocity
  - 0x461d00 zEffectAnim::QueueCmdType4TransformRefs
  - 0x461eb0 zEffect_Anim::SetActivationDispatchContext
  - 0x461ec0 zEffect::FindNodeUserDataRecursive
  - 0x461f00 zEffect::SpawnRuntimeInstanceAt
  - 0x461f50 zEffect::ActivateRuntimeEntryAtPosition
  - 0x462050 zEffect::ComputeDistanceSqToListener
  - 0x4620d0 zEffect::AcquireRuntimeEntryByIndex
  - 0x462130 zEffect::CloneRuntimeEntryFromTemplate
  - 0x4621b0 zEffect::RuntimeNodeActionCallback
  - 0x498bd0 zRndr_DrawImmediateLine
  - 0x498c00 zRndr_DrawClippedImmediateLineStrip
  - 0x48ca70 zArchiveList_RemovePayload
  - 0x48cb00 zArchiveList_FindNodeByPayload
  - 0x48cc20 zArchiveList_FindPayloadByValue
- Next action:
  - `python tools/recoil_status.py 0x48b820`

### Group: RecoilApp display init leaf helpers

- Anchor: 0x42e330 RecoilApp::InitializeDisplay
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4a7b30 zVideo::GetClearScreenBufferEnabled
  - 0x48d450 zRndr::OverlayBlendRow555_Scalar
- Next action:
  - `python tools/recoil_status.py 0x4a7b30`

### Group: zDEClient config resource load closure

- Anchor: 0x4558f0 zDEClient::LoadConfigResources
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4558f0 zDEClient::LoadConfigResources
  - 0x455ef0 zDEClient_QSand::InstanceEventMaybeRelay
  - 0x458aa0 zDEClient::SetCameraNode
  - 0x455dd0 zDEClient::LoadMaterialFromTexturePath_Local
  - 0x480c40 zModel_Material::ResetDefaults
  - 0x481050 zModel_Material::SetCycleTextureCount
  - 0x481260 zModel_Material::SetCycleTextureSpeed
  - 0x481220 zModel_Material::SetCycleTextureLoop
  - 0x46d4c0 zImage::GetDefaultImageRefPtr
  - 0x46d810 zImage::TexDir_FindOrAppendByPath
  - 0x481100 zModel_Material::AddCycleTexture
  - 0x480ca0 zModel_Material::FindOrClone
  - 0x480d20 zModel_Material::CompareForReuse
  - 0x481040 zModel_Material::SetUserTag
  - 0x46d4d0 zImage::FindTexDirEntryByName
  - 0x46de50 zImage::TexDir_LoadPendingEntries
  - 0x46df50 zVid_TexturePack_EnsureBuiltinTexturePacksLoaded
  - 0x46e3e0 zImage_TexDirEntryPartial::BuildMipChain
  - 0x479cc0 OptCatalog_IsDamageMaskSlotPtrRegistered
  - 0x4902b0 zVid_Image::CalcPow2ScratchFields
  - 0x481420 zModel_Material::FindByTexDirEntry
  - 0x457b40 zDEClient::WriteFeatureSectionsToZAR
  - 0x457750 zDEClient::ClearFeatureDisplayNodes
  - 0x457840 zDEClient::AppendFeatureEntry
  - 0x457d90 zDEClient_MapTreeState::FindOrInsertKey
  - 0x4575f0 zDEClient::SubmitFeatureGeometry
  - 0x450a00 zClass_World::GetAreaPartitionAtGrid
  - 0x481530 zModel_Const::GetVertexMergeEpsilon
  - 0x481540 zModel_Const::SetVertexMergeEpsilon
  - 0x46af00 zGeometry_ClipPatchOutput::Create
  - 0x46af20 zGeometry_ClipPatchOutput::Destroy
  - 0x46ae40 zGeometry_ClipPatchOutput::ApplyNodeDiPairs
  - 0x46af40 zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition
  - 0x46b1f0 zGeometry_Model::ClipPatch
  - 0x458ae0 zDEClient::GetCameraNode
  - 0x458ac0 zDEClient::GetFeatureGridCell
  - 0x455ea0 zDEClient_QSand::DestroyFeature
  - 0x456ad0 zDEClient_Crater::DestroyFeature
  - 0x456b00 zDEClient_Crater::InitEventTemplateDefaults
  - 0x456b20 pending
  - 0x456c50 pending
  - 0x456c80 zDEClient_Crater::InitFeatureFromEventTemplate
  - 0x4563d0 zDEClient_QSand::CreateFeatureStructFromEventTemplate
  - 0x457040 zDEClient_Crater::CreateFeatureStructFromEventTemplate
  - 0x4570e0 zDEClient_Crater::Build
  - 0x457140 zDEClient_Crater::CreateFeature
  - 0x456010 zDEClient_QSand::InitFeatureFromEventTemplate
  - 0x456450 zDEClient_QSand::Build
  - 0x4564b0 zDEClient_QSand::CreateFeature
  - 0x46a5e0 zGeometry_Vec3Array::RotateNeg90AroundX
  - 0x46a600 zGeometry_Vec3Array::RotatePos90AroundX
  - 0x46a9c0 zGeometry_Vec3Array::ComputeBoundsXY
  - 0x469e50 zGeometry_Vec3::IsNearEqualXY
  - 0x469e90 zGeometry_Vec3::SnapPointToSegmentXYIfNear
  - 0x46a130 zGeometry_Polygon::SnapPointsXYIfNear
  - 0x46a620 zGeometry_Bounds2D::OverlapsWithUnitMargin
  - 0x46a690 zGeometry_Model::FindOrCreateRandomDebugMaterial
  - 0x46a770 zGeometry_Model::AddPolygonToDi
  - 0x46a7f0 zGeometry_Model::BuildPolygonUvList
  - 0x46a8e0 zGeometry_Polygon::SolveUvAxisCoefficientsXZ
  - 0x46ab40 zGeometry_ClipPolygon::FindPointIndexXY
  - 0x46ac80 zGeometry_ClipPolygon::FindPointInsertionEdgeXYIndex
  - 0x46ab90 zGeometry_ClipPolygon::UpsertPointListXY
  - 0x464790 zGeometry_ClipPolygon::ResetWeilerStateFromContourPoints
  - 0x464810 zGeometry_Weiler::ClipPointList
  - 0x46bb90 zGeometry_Model::IsFullyInsideClipPolygonXY
  - 0x4826a0 zUtil::StoreInt32
  - 0x46aa40 zGeometry_ClipPolygon::CreateFromPointList
  - 0x46aab0 zGeometry_ClipPolygon::CopyPointsOutRotatedBack
  - 0x46ab10 zGeometry_ClipPolygon::FinalizeAndDestroy
  - 0x46b030 zGeometry_ClipPolygon::SnapPointsNearNodeModelXY
  - 0x464680 zGeometry_Weiler::Init
  - 0x464670 zGeometry_Weiler::GetInputContourAPointList
  - 0x464b30 zGeometry_WeilerClipOutput::Destroy
  - 0x464b90 zGeometry_Weiler::InitInputContourPair
  - 0x4647d0 zGeometry_Weiler::DestroyState
  - 0x4676c0 zGeometry_Weiler::EnsureContourOutput
  - 0x467710 zGeometry_Weiler::MergeContours
  - 0x464c90 zGeometry_Weiler::ClassifyInputContourPairBounds
  - 0x464ea0 zGeometry_Weiler::OutputPreclassifiedContourPairResult
  - 0x4681a0 zGeometry_Weiler::OutputContoursForClipMode
  - 0x4682c0 zGeometry_Weiler::OutputContourToPolygonSet
  - 0x468650 zGeometry_Weiler::CreateForwardSegmentPairAtPoint
  - 0x468700 zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA
  - 0x4687b0 zGeometry_Weiler::GenerateOutsideResults
  - 0x4680b0 zGeometry_Weiler::NewContour
  - 0x468a10 zGeometry_Weiler::ClassifyPointInContourPointListXY
  - 0x469430 zGeometry_Weiler::GetNextContourSegmentForTraversal
  - 0x469ae0 zGeometry_WeilerBuffer::SetCountAndAppendPtr
  - 0x469a30 zGeometry_Weiler::PreclassifyInputContourAAdjacentEdgePairs
  - 0x468470 zGeometry_Weiler::BuildPointSideTablesForContourPair
  - 0x464f70 zGeometry_Weiler::PreclassifyInputContourPair
  - 0x465ac0 zGeometry_Weiler::ClassifyContainedContour
  - 0x468580 zGeometry_Weiler::DivideContourSegmentAtPoint
  - 0x468c40 zGeometry_Weiler::Intersect2d
  - 0x468fa0 zGeometry_Weiler::ClassifyIntersect2d
  - 0x4683a0 zGeometry_Weiler::TogglePointAxesForContourSource
  - 0x469af0 zGeometry_Weiler::RestorePointTranslation
  - 0x469b60 zGeometry_Weiler::RestoreOutputZFromInputPlane
  - 0x469ca0 zGeometry_Vec3::IsBetweenEndpointsXY
  - 0x469d60 zGeometry_Weiler::SelectForwardStartPointInContourA
  - 0x469450 zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment
  - 0x469560 zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair
  - 0x46a1f0 zGeometry_Weiler::ValidateXings
  - 0x468410 zGeometry_WeilerContourSegment::UpdateBounds
  - 0x4693a0 zGeometry_WeilerContourSegmentArray::UpdateBounds
  - 0x4693c0 zGeometry_WeilerContourSegmentArray::InitFromPointList
  - 0x469960 zGeometry_Weiler::RecenterPointSetsIfOutOfRange
  - 0x467600 zGeometry_WeilerBuffer::Init
  - 0x467660 zGeometry_WeilerBuffer::GetAppendSpace
  - 0x467630 zGeometry_WeilerBuffer::Destroy
  - 0x46b550 zGeometry_ClipPolygon::ProcessNodePolygonSetXY
  - 0x46b650 zGeometry_Model::GetLinearBufferOfPolygonVertices
  - 0x46b6d0 zGeometry_Model::ProcessClipPatchNode
  - 0x46ba90 zGeometry_Model::AddPointListPolygonToDi
  - 0x46bb30 zGeometry_Model::AddIndexedPolygonToDi
  - 0x46bd50 zGeometry_TriangulateHole::TryAppendBridgeEdge
  - 0x46be20 zGeometry_Segment::IntersectsSegmentXY
  - 0x46bf30 zGeometry_TriangulateHole::CollectActiveEdgeIndicesForVertex
  - 0x46bf70 zGeometry_TriangulateHole::FindActiveEdgeState
  - 0x46bfc0 zGeometry_TriangulateHole::TryEmitTriangleFromEdgePair
  - 0x46c070 zGeometry::TriangulatePolygonWithHole
  - 0x46c390 zGeometry_TriangulateHole::CacheCombinedPlane
  - 0x46c3a0 zGeometry_Vec3Array::ComputeNewellPlane
  - 0x46c570 zGeometry_TriangulateHole::ProjectInnerRingOntoCachedPlane
  - 0x46c5b0 zGeometry_Vec3Array::ReversePoints
  - 0x46c620 zGeometry_Vec3Array::EnsurePositiveCrossZ
  - 0x46c720 zGeometry_ConvexPolygonSet::Destroy
  - 0x46c760 zGeometry_Polygon::Convexify
  - 0x46cb50 zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive
  - 0x46ced0 zGeometry_Polygon::TrySplitPointDwordOffsetsAtBestDiagonal
  - 0x483610 zDi::AddPolygon
  - 0x483650 pending
- Next action:
  - `python tools/recoil_status.py 0x4558f0`

### Group: zNetwork session runtime shutdown

- Anchor: 0x42e430 RecoilApp::ShutdownEngine
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x489f70 zNetwork_GetLocalPlayerKey
  - 0x489f80 zNetwork::IsHost
  - 0x48ba60 zNetwork_FindPlayerRecordByKey
  - 0x48b820 zNetwork_ApplyPkt01_PlayerColorAssignments
  - 0x48b980 zNetwork_GetLocalPlayerColorIndex
  - 0x48b9a0 zNetwork_GetPlayerColorIndexByKey
  - 0x48b9d0 zNetwork_GetPlayerRecordCount
  - 0x48bab0 zNetwork_ExtractStatusFieldsFromSessionDesc
  - 0x48bb20 zNetwork_ApplyStatusFieldsToSessionDesc
  - 0x48acf0 zNetwork_DPlay_SendUnreliable
  - 0x48ad30 zNetwork_DPlay_SendReliable
  - 0x48ad70 zNetwork_DPlay_SendExUnreliableTracked
  - 0x48ae10 zNetwork_DPlay_SendExReliable
  - 0x48c060 zNetwork_SendPacketUnreliable
  - 0x48c080 zNetwork_SendPacketReliable
- Next action:
  - `python tools/recoil_status.py 0x489f70`

### Group: zInput startup leaf helpers

- Anchor: 0x42e220 RecoilApp::StartEngine
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x470180 zInput::Mouse_RecenterCursorX
  - 0x470190 zInput::Mouse_IsInitialized
  - 0x4703b0 zInput::Mouse_PollAndStoreState
  - 0x4703c0 zInput::Mouse_PollState
  - 0x470680 zInput::Mouse_WaitForButtonPress
  - 0x46f690 zInput::Keyboard_PollState
  - 0x4703a0 zInput::Mouse_GetStateSnapshotPtr
  - 0x4705f0 zInput::Mouse_GetStateSnapshot
  - 0x4702e0 zInput::Mouse_GetButtonTransitionState
  - 0x46f980 zInput::Keyboard_GetKeyTransitionState
  - 0x46f9d0 zInput::Keyboard_UnregisterKeyCallback
  - 0x46fa10 zInput::Keyboard_WaitForAnyKeyPress
  - 0x404140 zInput_WaitForAnyKeyPressWithTimeoutMs
  - 0x42f9f0 zInput_DI_InitForceFeedbackEffectSet
  - 0x42fb50 zInput_DI_PlayCollisionImpactEffect
  - 0x42fc90 zInput_DI_PlayDamageHitEffect
  - 0x42fdc0 zInput_DI_UpdateSteerAndPitchForceEffects
  - 0x4707a0 zInput_BindMapContext::FreeAllBuffers
  - 0x470960 zInput_BindMapContext::FreeNonOwnedBuffers
  - 0x470a80 zInput_BindMapContext::GetJoystickButtonSlot
  - 0x470aa0 zInput_BindMapContext::GetMouseButtonSlot
  - 0x470ac0 zInput_BindMapContext::GetCommandByPrimaryKey
  - 0x470ad0 zInput_BindMapContext::GetCommandBySecondaryKey
  - 0x470ae0 zInput_BindMapContext::GetCommandByAnyKeyboardKey
  - 0x470b00 zInput_BindMapContext::GetCommandByJoystickSlot
  - 0x470b10 zInput_BindMapContext::GetCommandByMouseSlot
  - 0x470eb0 zInput_BindMapContext::ReadCommandInputState
  - 0x470f50 zInput_BindMapContext::CopyCommandLabel
  - 0x470f80 zInput::BindMap_FormatKeyComboName
  - 0x471040 zInput::BindMap_CopyJoystickButtonName
  - 0x471070 zInput::BindMap_CopyMouseButtonName
  - 0x471660 zInput::BindMapSystem_Shutdown
  - 0x4716c0 zInput::BindMapCurrent_ResetAllBindings
  - 0x4716d0 zInput::BindMapCurrent_GetPrimaryKeyboardKey
  - 0x4716e0 zInput::BindMapCurrent_GetSecondaryKeyboardKey
  - 0x4716f0 zInput::BindMapCurrent_GetJoystickButtonSlot
  - 0x471700 zInput::BindMapCurrent_GetMouseButtonSlot
  - 0x471710 zInput::BindMapCurrent_GetCommandByPrimaryKey
  - 0x471720 zInput::BindMapCurrent_GetCommandBySecondaryKey
  - 0x471730 zInput::BindMapCurrent_GetCommandByJoystickSlot
  - 0x471740 zInput::BindMapCurrent_GetCommandByMouseSlot
  - 0x471750 zInput::BindMapCurrent_SetPrimaryKeyBinding
  - 0x471760 zInput::BindMapCurrent_SetSecondaryKeyBinding
  - 0x471770 zInput::BindMapCurrent_SetJoystickBinding
  - 0x471780 zInput::BindMapCurrent_SetMouseBinding
  - 0x4717c0 zInput::BindMap_Current_SetCommandCallback
  - 0x4717d0 zInput::BindMap_Current_ReadCommandInputState
  - 0x4717e0 zInput::BindMapCurrent_CopyCommandLabel
  - 0x471800 zInput::BindMapCurrent_FormatKeyComboName
  - 0x471820 zInput::BindMapCurrent_CopyJoystickButtonName
  - 0x471840 zInput::BindMapCurrent_CopyMouseButtonName
  - 0x471860 zInput::BindMapContext_Push
  - 0x471950 zInput::BindMapContext_Pop
  - 0x471f60 zInput::DI_EnumDevicesCallback_SelectFirstJoystick
  - 0x4722c0 zInput::DI_PollJoystickState
  - 0x4723a0 zInput::DI_GetButtonTransitionState
  - 0x4723d0 zInput::DI_WaitForButtonPress
  - 0x42ffa0 zInput_DI_CreateConstantForceEffectScaled
  - 0x430070 zInput_DI_CreateConstantForceEffectWithDirection
  - 0x430100 zInput_DI_CreateSineEffectScaled
  - 0x472450 zInput_DI_CreateForceFeedbackEffect
- Next action:
  - `python tools/recoil_status.py 0x470180`

### Group: controls option accessors

- Anchor: 0x408df0 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x408df0 pending
  - 0x407e30 zOpt::SetThrottleMode
  - 0x407e50 zOpt::GetThrottleMode
  - 0x407e60 zOpt::SetSteeringMode
  - 0x407e80 zOpt::GetSteeringMode
  - 0x407e90 zOpt::SetCursorMode
  - 0x407eb0 zOpt::GetCursorMode
  - 0x407ef0 zOpt::GetCameraModePlayerState
  - 0x407f20 zOpt::GetGameDifficultyMode
  - 0x408030 zOpt::GetObjectLODForCurrentHwMode
  - 0x408060 zOpt::GetMuteSoundOption
  - 0x408090 zOpt::GetSoundVolumeOption
  - 0x4080d0 zOpt::GetSoundLODOption
  - 0x408100 zOpt::GetTextureMemoryForCurrentHwMode
  - 0x408190 zOpt_GetPlayerName
  - 0x4081f0 zOpt::GetGraphicsFlagsForCurrentHwMode
  - 0x408270 zOpt::GetNetworkModemEnabled
  - 0x408570 zOpt::RenderSection_SetTargetWindow
  - 0x4085b0 zOpt::DisplaySection_SetTargetDisplay
  - 0x408480 zOpt::CameraSection_SetActiveCamera
  - 0x407ec0 pending
- Next action:
  - `python tools/recoil_status.py 0x408df0`

### Group: MFC frame shell cluster

- Anchor: 0x42e110 RecoilApp::CreateMainWnd
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x430240 CZRecoilFrame::GetRuntimeClass
  - 0x430610 CZRecoilFrame::Destructor
  - 0x430680 CZRecoilFrame::SetMenuBarVisibility
  - 0x4306e0 CZRecoilFrame::GetMessageMap
  - 0x4308a0 CZRecoilFrame::OnMenuExitGame
  - 0x4309b0 CZRecoilFrame::OnMenuSetVideoMode2
  - 0x4309d0 CZRecoilFrame::OnMenuSetVideoMode3
  - 0x4309f0 CZRecoilFrame::OnMenuSetVideoMode4
  - 0x430a30 CZRecoilFrame::OnMenuSetVideoMode6
  - 0x430a50 CZRecoilFrame::OnMenuSetVideoMode7
  - 0x430a70 CZRecoilFrame::OnMenuToggleHud
  - 0x430a90 CZRecoilFrame::OnUpdateHudCmdUI
  - 0x430ab0 CZRecoilFrame::OnMenuToggleFullscreen
  - 0x4313d0 CZRecoilFrame::OnUpdateVideoMode2CmdUI
  - 0x431430 CZRecoilFrame::OnUpdateVideoMode3CmdUI
  - 0x431490 CZRecoilFrame::OnUpdateVideoMode4CmdUI
  - 0x4314f0 CZRecoilFrame::OnUpdateVideoMode5CmdUI
  - 0x431550 CZRecoilFrame::OnUpdateVideoMode6CmdUI
  - 0x4315b0 CZRecoilFrame::OnUpdateVideoMode7CmdUI
  - 0x4316c0 CZRecoilFrame::EnsureHwApiInitialized
  - 0x431790 CZRecoilFrame::OnMenuSelectHwApi0
  - 0x4317a0 CZRecoilFrame::OnMenuSelectHwApi1
  - 0x4317b0 CZRecoilFrame::OnMenuSelectHwApi2
  - 0x4317c0 CZRecoilFrame::OnMenuSelectHwApi3
  - 0x4317d0 CZRecoilFrame::UpdateHwApiMenuItem
  - 0x431870 CZRecoilFrame::OnUpdateHwApi0CmdUI
  - 0x4318b0 CZRecoilFrame::OnUpdateHwApi1CmdUI
  - 0x4318c0 CZRecoilFrame::OnUpdateHwApi2CmdUI
  - 0x4318d0 CZRecoilFrame::OnUpdateHwApi3CmdUI
  - 0x4318e0 CZRecoilFrame::OnUpdateFullscreenCmdUI
  - 0x431900 CZRecoilFrame::OnMenuToggleCDAudio
  - 0x431920 CZRecoilFrame::OnUpdateCDAudioCmdUI
  - 0x431950 CZRecoilFrame::OnMenuToggleJoystick
  - 0x431970 CZRecoilFrame::OnUpdateJoystickCmdUI
  - 0x431a80 MfcCmdUI::EnableAlways
  - 0x431a90 CZRecoilFrame::OnMenuSelectDirectSound
  - 0x431aa0 CZRecoilFrame::OnUpdateDirectSoundCmdUI
  - 0x431ad0 CZRecoilFrame::OnMenuSelectA3D
  - 0x431ae0 CZRecoilFrame::OnUpdateA3DCmdUI
  - 0x431b10 CZRecoilFrame::OnSize
  - 0x4437a0 CZGameFrame::GetRuntimeClass
  - 0x4437b0 CZGameFrame::GetBaseMessageMap
  - 0x4437c0 CZGameFrame::GetMessageMap
  - 0x443830 CZGameFrame::Destructor
  - 0x4438a0 CZGameFrame::IsWindowValid
  - 0x4438c0 CZGameFrame::BuildWindowTitle
  - 0x4438f0 CZGameFrame::OnClose
  - 0x443a20 CZGameFrame::OnSize
  - 0x443a60 CZGameFrame::OnCreate
  - 0x443ab0 CZGameFrame::OnDestroy
  - 0x443ae0 CZGameFrame::OnActivate
  - 0x443b50 CZGameFrame::OnAppIdleDispatchMessage
  - 0x471c60 zInput::Mouse_IsUnsuspended
  - 0x471c70 zInput::Joystick_IsUnsuspended
  - 0x471c80 zInput_Keyboard_IsUnsuspended
  - 0x471cf0 zInput::Mouse_Suspend
  - 0x471d00 zInput::Joystick_Suspend
  - 0x471d10 zInput::Keyboard_Suspend
  - 0x471ae0 zInput::OnAppDeactivate
  - 0x471b20 zInput::OnAppActivate
  - 0x471c90 zInput::Mouse_ResumeFromSuspend
  - 0x471cd0 zInput::Keyboard_ResumeFromSuspend
  - 0x471cb0 zInput::Joystick_ResumeFromSuspend
  - 0x4a7770 zVideo_RestoreIconicFullscreenWindowIfNeeded
  - 0x48a980 zNetwork_DPlay_DestroyCachedLocalPlayer
  - 0x48c250 zNetwork_DPlay_ReportError
  - 0x443a40 zVid_UpdateCachedClientRectIfUpdateMaskEnabled
  - 0x4a59b0 zVid_QueryCachedClientRectUpdateMaskIf3dfx
  - 0x42a480 zInput::BindGroupList_GetCount
  - 0x42a4a0 zInput::BindGroupList_GetGroupTitle
  - 0x42a4b0 zInput::BindGroupList_GetGroupCommandCount
  - 0x42a4d0 zInput::BindGroupList_GetGroupCommandId
  - 0x42a4e0 zInput::BindMap_GetCommandLabel
  - 0x42a4f0 zInput::BindMap_GetCommandHint
  - 0x46fba0 zInput::Keyboard_TranslateDikToAscii
  - 0x46fd20 zInput::Keyboard_InitDikToAsciiTable
  - 0x42fa80 zInput_DI_IsForceFeedbackEnabled
  - 0x472480 zInput_DI_HasForceFeedback
  - 0x4700a0 zInput::Mouse_SetNormalizedCursorPos
  - 0x472390 zInput::DI_GetCurrentState
  - 0x470d40 zInput_BindMapContext::DispatchMouseButtonCallbacks
  - 0x470db0 zInput_BindMapContext::DispatchJoystickButtonCallbacks
  - 0x470e80 zInput_BindMapContext_DispatchFromKeyboardEvent
  - 0x42faa0 zInput_DI_RestartPrimaryFireEffect
  - 0x42fac0 zInput_DI_PlayAltFireEffect
- Next action:
  - `python tools/recoil_status.py 0x430240`

### Group: RecoilApp startup construction cluster

- Anchor: 0x42de20 RecoilApp::StaticInitAndRegisterAtExit
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4429b0 RecoilApp::MfcOleModuleScalarDeletingDestructor
  - 0x42eec0 RecoilApp_PlayState::OnWndActivate
  - 0x4a9940 zVid::GetSelectedD3DDeviceNameOrDefault
  - 0x404c50 Briefing::SetProgressAndSleep
  - 0x414180 HudUiLoadingCheckpoint::AdvanceAndLog
  - 0x414210 HudUiLoadingCheckpoint::InitTable
  - 0x4bd2a0 HudUiTextStack4::Clear
  - 0x413910 HudUiMgr::EnableTopAndChatStacks
  - 0x413950 HudUiMgr::DisableTopAndChatStacks
  - 0x443650 RecoilApp::OnIdleOrDispatch
  - 0x462370 zFMV_Playback::OpenAndPlay
  - 0x4159d0 zFMV_Action::NoOpUpdate
  - 0x4159e0 zFMV_Action::RunBlockingTimed
  - 0x462e30 zFMV_Action::RunBlockingImmediate
  - 0x462e90 zFMV_ActionPlaySound::Begin
  - 0x463ca0 zFMV_ActionPlayMci::Begin
  - 0x462ed0 zFMV_ActionWait::Begin
  - 0x462ee0 zFMV_ActionWait::Update
  - 0x462f00 zFMV_Action::FlipSurfaces
  - 0x462f90 zFMV_Script::BeginCurrentAction
  - 0x463000 zFMV_Script::Update
  - 0x4630a0 zFMV_Script::BeginAtTime
  - 0x4630e0 zFMV_Script::UpdateAtTime
  - 0x463120 zFMV_Script::BeginNow
  - 0x463cc0 zFMV_ActionPlayMci::End
  - 0x463dd0 zFMV_Stream::Destructor
  - 0x4a3910 zSndSample::Destroy
- Next action:
  - `python tools/recoil_status.py 0x4429b0`

### Group: GameNet list globals cluster

- Anchor: 0x431bf0 GameNetSpawnPointList::InitGlobals
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x431bf0 GameNetSpawnPointList::InitGlobals
  - 0x431c20 GameNetPlayerRowList::Reset
  - 0x4322a0 GameNet::ResetHudTimerPanelNetStateLongCountdown
  - 0x414330 GameNet::ShowPlayerKillMessage
  - 0x414390 GameNet::RefreshPlayerListMenu
  - 0x4143b0 HudUi::RefreshScoreboardEntryRow
  - 0x4143c0 HudUi::RemoveScoreboardEntryRow
  - 0x432e70 GameNet::ReassignPlayerColorsAndRefreshRows
  - 0x4ae450 OptCatalog::FindEntryById
  - 0x433000 GameNet::SendPkt08_PlayerKillEvent
  - 0x433060 GameNet::HandlePkt08_PlayerKillEvent
  - 0x432830 GameNet::FindPlayerRowByKey
  - 0x433200 GameNet::AreAllPlayersAtLapTarget
  - 0x433390 GameNet::SendPkt0C_HudTimerStatusBits
  - 0x433410 GameNet::HandlePkt0C_HudTimerStatusBits
  - 0x4334f0 GameNet::SendPkt09_PlayerScoreboardSnapshot
  - 0x4335b0 GameNet::HandlePkt09_PlayerScoreboardSnapshot
  - 0x4337e0 GameNet::HandlePkt0B_ChatMessage
  - 0x433710 GameNet::SetStatusBitsFromFlags
  - 0x433730 GameNet::GetStatusBitAllowMaps
  - 0x433740 GameNet::GetStatusBitNameTags
  - 0x434550 GameNet::HostUpdateSessionDescStatusFields
  - 0x433a50 GameNetPlayerRow::ApplyPlayerColorTint
  - 0x433a40 HudTimerPanelNetState::ClearTailFlagsLocal
  - 0x434650 GameNetPlayerRow::DestroyEmbeddedPanel
  - 0x447d20 zClass_Class::gwNodeSetFlag16
  - 0x447d70 zClass_Class::gwNodeSetFlag17
  - 0x448090 zClass_Class::gwNodeSetPriority
  - 0x44f7a0 zClass_Window::gwWindowNew
  - 0x44f930 zClass_Window::gwWindowGetResolution
  - 0x44fa40 zClass_Window::gwWindowGetSize
  - 0x44fad0 zClass_Window::gwWindowSetBuffer
  - 0x44fb40 zClass_Window::gwWindowSetClearPolygon
  - 0x44fbd0 zClass_Window::gwWindowAddClearPolygonVertex
  - 0x44fcf0 zClass_Window::gwWindowCloseClearPolygon
  - 0x44fdd0 zClass_Display::gwDisplayInit
  - 0x44ff90 zClass_Display::gwDisplaySetBackgroundColor
  - 0x452fd0 zClass_Light::gwLightNew
  - 0x453200 zClass_Light::gwLightSetIntensity
  - 0x453250 zClass_Light::gwLightSetFalloff
  - 0x4532a0 zClass_Light::gwLightSetConeAngle
  - 0x4532f0 zClass_Light::gwLightSetPointMode
  - 0x453350 zClass_Light::gwLightSetDirectionalMode
  - 0x4533b0 zClass_Light::gwLightSetParam
  - 0x453400 zClass_Light::gwLightSetRange
  - 0x453500 zClass_Light::gwLightGetRange
  - 0x453560 zClass_Light::gwLightSetPosition
  - 0x4535c0 zClass_Light::gwLightSetRotation
  - 0x453a40 zClass_Light::gwLightGetSpecularColor
  - 0x453aa0 zClass_Light::gwLightSetSpecularColor
  - 0x474d10 zMath::Vec3DirectionAnglesBetweenPoints
  - 0x49b5a0 zRndr_FogTargetColorStaged_SetRgb01Clamped
  - 0x4a6b80 zVideo_SetClearColorPacked16
  - 0x4a7250 zVideo_SetPendingFogTargetColorFromRgb01
  - 0x449ba0 zClass_Camera::SetViewDistance
  - 0x450ae0 zClass_World::SetPendingFogState
  - 0x450af0 zClass_World::SetPendingFogColorRgb01
  - 0x450b20 zClass_World::SetPendingFogAltitudeRange
  - 0x450b40 zClass_World::SetPendingFogRange
  - 0x450b60 zClass_World::SetPendingFogDensity
  - 0x453620 zClass_Light::ComputeWorldTransform
  - 0x449be0 zClass_Camera::gwCameraNew
  - 0x449c90 zClass_Camera::gwCameraAddChild
  - 0x449d20 zClass_Camera::gwCameraSetFlagBit0
  - 0x449da0 zClass_Camera::SetTargetNode
  - 0x449db0 zClass_Camera::SetActiveCamera
  - 0x449dc0 zClass_Camera::SetObjectHseTestEnabled
  - 0x449dd0 zClass_Camera::gwCameraSetWorld
  - 0x449e90 zClass_Camera::gwCameraSetWindow
  - 0x449ea0 zClass_Camera::gwCameraSetPosition
  - 0x449f50 zClass_Camera::ActivateChildren
  - 0x449fb0 zClass_Camera::gwCameraTranslate
  - 0x44a060 zClass_Camera::gwCameraGetPosition
  - 0x44a0f0 zClass_Camera::gwCameraSetTarget
  - 0x44a1a0 zClass_Camera::gwCameraTranslateTarget
  - 0x44a250 zClass_Camera::gwCameraGetTarget
  - 0x44a2f0 zClass_Camera::gwCameraSetNearFarClip
  - 0x44a410 zClass_Camera::gwCameraSetViewport
  - 0x44a580 zClass_Camera::gwCameraGetViewport
  - 0x44a7f0 zClass_Camera::gwCameraGetClipDistance
  - 0x44a910 zClass_Camera::gwCameraSetHorizon
  - 0x44a980 zClass_Camera::gwCameraSetHorizonXZ
  - 0x452770 zClass_Class::FindSubNodeByName
  - 0x452860 zClass_Node::SetMaterialFlagBit9ForFlagBit0EntriesRecursive
  - 0x4528b0 zClass_Node::InvalidateFlagBit8MaterialImagesRecursive
  - 0x4528e0 zClass_Node::AssignInt32ToDiRecursive
  - 0x452920 zClass_Class::AddChildValidated
  - 0x4542a0 zClass_Lod::gwLodNew
  - 0x454310 zClass_Lod::gwLodAddChild
  - 0x454330 zClass_Lod::SetComputeOwnDistance
  - 0x454340 zClass_Lod::SetTargetNodeAndRange
  - 0x46e250 zImage::InvalidateLoadedVariantChain
  - 0x480f60 zModel_Material_SetFlagBit9
  - 0x480f80 zModel_Material::InvalidateImagesIfEligible
  - 0x4826d0 zDi::SetFlagBit0
  - 0x4841b0 zDi_SetMaterialFlagBit9ForFlagBit0Entries
  - 0x4841f0 zDi::InvalidateImagesForFlagBit8Materials
  - 0x4b25a0 zClass_Node::SetDamageHitCallback
  - 0x4b25f0 zClass_Node::AssignDamageHandlerRecursiveIfMissing
  - 0x4b26b0 zClass_Node::SetDamageTimerCallback
  - 0x4b2880 OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback
  - 0x448100 zClass_Class::gwNodeSetCellPickable
  - 0x448140 zClass_Class::gwNodeGetCellPickable
  - 0x448180 zClass_Class::gwNodeGetNodeType
  - 0x4481b0 zClass_Class::gwNodeSetRaycastable
  - 0x4481f0 zClass_Class::gwNodeGetRaycastable
  - 0x448230 zClass_Class::gwNodeSetPickable
  - 0x448270 zClass_Class::gwNodeGetPickable
  - 0x4482f0 zClass_Class::gwNodeSetBypassFarClip
  - 0x448330 zClass_Class::gwNodeSetNodeType
  - 0x448360 zClass_Class::gwNodeClearVariantGate
  - 0x4483a0 zClass_Class::gwNodeSetVertexAlphaOverride
  - 0x4484d0 zClass_Class::AddChildGeneric
  - 0x449ab0 zClass_Class::gwNodeGetRoot
  - 0x449af0 zClass_Class::gwNodeGetWorldChild
  - 0x44db10 zClass_Object3D::gwObject3DAddChild
  - 0x44dbb0 zClass_Object3D::gwObject3DSetVisibleFlag
  - 0x44dc30 zClass_Object3D::gwObject3DSetColorAlpha
  - 0x44dd90 zClass_Object3D::gwObject3DSetAlphaScale
  - 0x44de10 zClass_Object3D::gwObject3DGetAlphaScale
  - 0x44de80 zClass_Object3D::gwObject3DSetLitFlag
  - 0x44df00 zClass_Object3D::gwObject3DSetScale
  - 0x44dfd0 zClass_Object3D::gwObject3DGetScale
  - 0x44e030 zClass_Object3D::gwObject3DSetRotation
  - 0x44e110 zClass_Object3D::gwObject3DGetRotation
  - 0x44e170 zClass_Object3D::gwObject3DTranslateRotation
  - 0x44e270 zClass_Object3D::gwObject3DGetPosition
  - 0x44e300 zClass_Object3D::gwObject3DSetPosition
  - 0x44e3d0 zClass_Object3D::gwObject3DTranslatePosition
  - 0x44e4f0 zClass_Object3D::gwObject3DSetMatrix
  - 0x44e5b0 zClass_Object3D::gwObject3DGetMatrixPtr
  - 0x44ea70 zClass_TypeList::UpdateAllBuckets
  - 0x44eaa0 zClass_TypeList::UpdateBucket
  - 0x44ebe0 zClass_TypeList::UpdateSequences
  - 0x44ec30 zClass_TypeList::UpdateAnimations
  - 0x44ec80 zClass_Class::gwNodeUpdateAll
  - 0x44ecb0 zClass_TypeList::PrintBucket
  - 0x453bd0 zClass_Animate::UpdateNode
  - 0x453c90 zClass_Animate::AdvanceTime
  - 0x453d20 zClass_Animate::SampleTransform
  - 0x453ee0 zClass_Sequence::gwSequenceNew
  - 0x453f40 zClass_Sequence::gwSequenceAddChild
  - 0x4540c0 zClass_Sequence::SetActive
  - 0x454100 zClass_Sequence::SetRepeat
  - 0x454140 zClass_Sequence::SetLoop
  - 0x454180 zClass_Sequence::SetPause
  - 0x4541c0 zClass_Sequence::Update
  - 0x44f690 zClass_List::IterateBucketFiltered
  - 0x44f6f0 zClass::FindNextByTypePrefix
  - 0x44f720 zClass::FindNextByTypePrefix_Predicate
  - 0x44f740 zClass_Class::gwNodeFindNextByName
  - 0x44f750 zClass_Class::gwNodeFindNextByName_Predicate
  - 0x4518b0 zClass::SetNodeArraySize
  - 0x4527f0 zClass_Node::HasRenderableDiPredicate
  - 0x452810 zClass::AnyNodeMatchesPredicateRecursive
  - 0x454370 GameZ_ZBD::NodePtrToIndex
  - 0x4543a0 zClass::NodePtrToValidatedIndex
  - 0x4543d0 GameZ_ZBD::NodeIndexToPtr
  - 0x4543f0 GameZ_ZBD::WriteNodeRefListIndices
  - 0x4544b0 GameZ_ZBD::WriteSingleNodeClassData
  - 0x454890 GameZ_ZBD::WriteNodeTable
  - 0x454a50 GameZ::WriteZBDFile
  - 0x454bf0 GameZ_ZBD::ReadNodeRefListIndices
  - 0x454c60 GameZ_ZBD::ReadSingleNodeClassData
  - 0x455350 GameZ_ZBD::ReadNodeTable
  - 0x455520 GameZ::ReadZBDFile
  - 0x4556a0 GameZ::OpenAndReadZBDHeader
  - 0x455730 GameZ_ZBD::ReloadDisplayInstancesFromCurrentPath_Local
  - 0x4557a0 GameZ_ZBD::ReloadDisplayInstancesRecursive_Local
  - 0x46d310 zImage::TexDirEntryToIndex
  - 0x46d340 zImage::TexIndexToDirEntry
  - 0x46d360 zImage::WriteTextureDirectory
  - 0x46d420 zImage::ReadTextureDirectory
  - 0x4808c0 zModel_MatlBuffer::ReadGameZ
  - 0x481aa0 zModel_DiPool::ReadEntryByIndexFromStream
  - 0x481bc0 zModel_DiPool::ReadHeaderFromStream
  - 0x481c50 zModel_DiPool::ReadEntryDynamicDataFromStream
  - 0x481fa0 zModel_DiPool::ReadFromStream
- Next action:
  - `python tools/recoil_status.py 0x431bf0`

### Group: Pickup shutdown lists

- Anchor: 0x41ccd0 Pickup::Shutdown
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x41ccd0 Pickup::Shutdown
  - 0x41cc10 PickupSpawnList::Primary_Init
  - 0x41cc40 PickupSpawnList::NetCopy_Init
  - 0x41cc70 PickupRespawnQueue::Init
  - 0x41d8a0 PickupSpawnList::RemoveAndFreeNode
  - 0x41e240 PickupSpawnList::Clear
  - 0x41e270 PickupRespawnQueue::ClearAndFree
- Next action:
  - `python tools/recoil_status.py 0x41ccd0`

### Group: Pickup packet leaf helpers

- Anchor: 0x433f40 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x433f40 pending
  - 0x41ceb0 zClass_Node::ClearPickupFlagsRecursive
  - 0x41db40 PickupType::GetByIndex_Pure
  - 0x41e1c0 PickupType::GetByIndex
  - 0x41e930 Pickup::FindSpawnByPickupId
  - 0x41e950 Pickup::GetSpawnDefFromNode
  - 0x41e960 Pickup::SetNextPickupId
  - 0x41e970 Pickup::GetNextPickupId
- Next action:
  - `python tools/recoil_status.py 0x433f40`

### Group: GameNet packet 07 OptCatalog leaf

- Anchor: 0x434190 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x434190 pending
  - 0x4ae4a0 OptCatalog::SetPendingSpawnTargetOverrides
- Next action:
  - `python tools/recoil_status.py 0x434190`

### Group: Player alt-gun controller lookup

- Anchor: 0x43c9c0 Player::FindAltGunFireControllerForWeaponId
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x43c9c0 Player::FindAltGunFireControllerForWeaponId
- Next action:
  - `python tools/recoil_status.py 0x43c9c0`

### Group: OptCatalog warning samples

- Anchor: 0x4b0600 OptCatalog::PlayTriggerInactiveWarning
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4b0600 OptCatalog::PlayTriggerInactiveWarning
  - 0x4b0620 OptCatalog::PlayWeaponInactiveWarning
  - 0x4b0640 OptCatalog::PlayNoAmmoWarning
- Next action:
  - `python tools/recoil_status.py 0x4b0600`

### Group: RecoilApp fatal-error shutdown

- Anchor: 0x430c90 RecoilApp::FatalErrorAndExit
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x430c90 RecoilApp::FatalErrorAndExit
  - 0x404bd0 Briefing::StopAndShutdownThread
- Next action:
  - `python tools/recoil_status.py 0x430c90`

### Group: zVideo DD3D polygon queues

- Anchor: 0x4aab90 zVideo_dd3d::SubmitPolyFlatColor16
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x4aab90 zVideo_dd3d::SubmitPolyFlatColor16
  - 0x4aaef0 zVideo_dd3d::SubmitPolyGouraudColor16
  - 0x4ab320 zVideo_dd3d::SubmitPolyColorAttr
  - 0x4ab6d0 zVideo_dd3d::SubmitPolyRenderClass
  - 0x4abb20 zVideo_dd3d::SubmitPolygon
  - 0x4ac370 zVideo_dd3d::SubmitPolygonLit
  - 0x4acbd0 zVideo_dd3d::DrawPointColor16
  - 0x4acd00 zVideo_dd3d::QueueSolidQuad
  - 0x4ace30 zVideo_dd3d::FlushSortedPolys
  - 0x4ad120 zVideo_dd3d::FlushQuadBatch
  - 0x4ad250 zVideo_dd3d::FlushOverwritePolys
- Next action:
  - `python tools/recoil_status.py 0x4aab90`

### Group: zEffectAnim ZBD load closure

- Anchor: 0x45efb0 zEffect_Anim::LoadZbd
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x45efb0 zEffect_Anim::LoadZbd
  - 0x458b50 zEffect::TickResetDelayOnTimer
  - 0x458bb0 zEffect::TickResetDelayOnHit
  - 0x45fb30 zEffect_Anim::LoadAndInstantiate
  - 0x460070 zEffect::InitFromPath
  - 0x460490 zEffect_Anim::SaveActivationRecords
  - 0x4606d0 zEffect_Anim::LoadActivationRecords
  - 0x460bc0 zEffect_Anim::SaveRunningAnimRecord
  - 0x460f80 zEffect_Anim::SaveRunningAnimRecords
  - 0x461040 zEffect_Anim::LoadRunningAnimRecords
  - 0x461430 zEffect_Anim::SaveAnimRecords
  - 0x461670 zEffect_Anim::LoadAnimRecords
  - 0x461840 zEffect_Anim::ResetFromActivationRecord
  - 0x461870 zEffect_Anim::ProcessActivationRecord
- Next action:
  - `python tools/recoil_status.py 0x45efb0`

### Group: RecoilApp play-state frame tick

- Anchor: 0x42f280 pending
- Reason: active dependency/source-readiness group retained after pruning temporary detail.
- Source blockers:
  - 0x42f280 pending
  - 0x44f630 pending
- Next action:
  - `python tools/recoil_status.py 0x42f280`

## Completed Groups

No completed groups are currently retained. Use `.agent/RECOIL_PLAN.md`, VC
verification manifests, and `python tools/recoil_status.py 0xNNNNNN` for
verification-only binary-safe debt.
