#include "Battlesport/RecoilApp.h"

#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/HudUiNetExitPanel.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

namespace {
int HalfTruncTowardZero(int value)
{
    return value / 2;
}

zOpt_ViewRectSection *ViewRectFromPtr(RecoilPtr32 ptr)
{
    return (zOpt_ViewRectSection *)(static_cast<unsigned int>(ptr));
}
} // namespace

// Reimplements 0x42f280: RecoilApp_PlayState::TickAndRenderFrame (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
RecoilApp_PlayState::TickAndRenderFrame(int shouldPresent)
{
    Time::Tick();

    if (g_Player_ActiveDebugScriptAsyncEntry != 0 &&
        zInput::Keyboard_WaitForAnyKeyPress(0) != 0) {
        zEffectAnimEntry *const entry = g_Player_ActiveDebugScriptAsyncEntry;
        g_Player_ActiveDebugScriptAsyncEntry = 0;
        zEffect_Anim::NodeActionCallback(entry, 0);
    }

    zInput::PollActiveDevices(1);

    pRenderSection = (RecoilPtr32)(zOpt::GetRenderSection());
    pDisplaySection = (RecoilPtr32)(zOpt::GetDisplaySection());
    pWindowSection = (RecoilPtr32)(zOpt::GetWindowSection());
    zOpt_ViewRectSection *const renderSection = ViewRectFromPtr(pRenderSection);
    zOpt_ViewRectSection *const displaySection = ViewRectFromPtr(pDisplaySection);
    zOpt_ViewRectSection *const windowSection = ViewRectFromPtr(pWindowSection);
    zClass_TypeList::UpdateAllBuckets();

    if (g_RecoilApp_QuitAfterCredits != 0) {
        return 1;
    }

    zSnd_Tick(0);

    if (g_Player_HorizonNodeFollowCameraEnabled != 0 && g_Player_HorizonNode != 0) {
        zVec3 cameraPosition = {0};
        gwNode::GetWorldPosition(g_MainCamera, &cameraPosition);
        zClass_Object3D::gwObject3DSetPosition(g_Player_HorizonNode, cameraPosition.x,
                                               cameraPosition.y, cameraPosition.z);
    }

    const int oldClearState = zVideo::GetClearScreenBufferEnabled();
    const int layoutDelay = HudUiMgr::TickLayoutDelay();
    const int savedClearState =
        zVideo::ExchangeClearScreenBufferEnabled(layoutDelay | oldClearState);
    zOpt_ViewRectSection *const clearRect = layoutDelay != 0 ? windowSection : renderSection;

    if (zVid::GetAccelerationOption() != 0) {
        zVideo::CallClearSwSurfaceAndZBuffer((zVidRect32 *)(clearRect),
                                             (zVidRect32 *)(windowSection));
    } else {
        zVideo::CallClearPrimarySurfaceAndZBuffer((zVidRect32 *)(clearRect));
    }
    zVideo::ExchangeClearScreenBufferEnabled(savedClearState);

    void *pixels;
    int pitchBytes;
    if (zOpt::GetReplicateMode() != 0) {
        pitchBytes = zVideo::GetSwSurfacePitch();
        pixels = zVideo::GetSwSurfacePixels();
    } else {
        pitchBytes = zVideo::GetPrimarySurfacePitch();
        pixels = zVideo::GetPrimarySurfacePixels();
    }

    zRndr::SetFrameBufferRegion(pixels, renderSection, zOpt::GetDisplaySectionBitsPerPixel(),
                                pitchBytes);
    zClass_List::RenderActiveCameras();
    zVideo::FxPass3_SetInputRectByIndex(0, (HudUiRect *)(renderSection));

    HudUiMgrSensor::GetFxRect(&g_HudUiMgrSensor_FxRectScratch);
    int fxTop = g_HudUiMgrSensor_FxRectScratch.top;
    int fxBottom = g_HudUiMgrSensor_FxRectScratch.bottom;
    if (zOpt::GetReplicateMode() != 0) {
        fxTop = HalfTruncTowardZero(fxTop);
        fxBottom = HalfTruncTowardZero(fxBottom);
        g_HudUiMgrSensor_FxRectScratch.left =
            HalfTruncTowardZero(g_HudUiMgrSensor_FxRectScratch.left);
        g_HudUiMgrSensor_FxRectScratch.right =
            HalfTruncTowardZero(g_HudUiMgrSensor_FxRectScratch.right);
        g_HudUiMgrSensor_FxRectScratch.top = fxTop;
        g_HudUiMgrSensor_FxRectScratch.bottom = fxBottom;
    }

    HudUiRect *fxRectOrNull = 0;
    if (fxBottom > renderSection->bottomExclusive) {
        if (fxTop < renderSection->bottomExclusive) {
            g_HudUiMgrSensor_FxRectScratch.top = renderSection->bottomExclusive;
        }
        fxRectOrNull = &g_HudUiMgrSensor_FxRectScratch;
    }
    zVideo::FxPass3_SetInputRectByIndex(1, fxRectOrNull);

    const int quitTransition = zInput::Keyboard_GetKeyTransitionState(1) & 3;

    if (zVid::GetAccelerationOption() != 0) {
        zRndr::LensFlare_ResetSampleQueue();
        g_HudSensorTracker.UpdateObjectiveFlow();
        HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag(&g_Variant_CurrentTag);
        zVideo::RunPostprocessOnSwBuffer();
        zVideo::FxPass3_UpdateLocal(g_FrameDeltaTimeSec);

        if (quitTransition != 0) {
            zVideo::Dispatch_UnlockSwSurfaceState();
            return 1;
        }

        zRndr::SetActiveRegionSizeFromRect((HudUiRect *)(windowSection));
        HudUiMgr::UpdateFrame();
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Tick();
        }
        zVideo::Dispatch_UnlockSwSurfaceState();
    } else if (zOpt::GetReplicateMode() != 0) {
        zVideo::RunPostprocessOnSwBuffer();
        zVideo::FxPass3_UpdateLocal(g_FrameDeltaTimeSec);
        zVideo::Dispatch_UnlockSwSurfaceState();

        if (shouldPresent != 0) {
            g_zVideo_pfnBltSwToPrimaryRectDirect((zVidRect32 *)(renderSection),
                                                 (zVidRect32 *)(displaySection));
        }

        zVideo::RunPostprocessOnPrimaryBuffer();
        if (quitTransition != 0) {
            zVideo::Dispatch_UnlockPrimarySurfaceState();
            zVideo::AdjustSurfacesIfEnabled(0, 0, 0, 1);
            return 1;
        }

        g_HudSensorTracker.UpdateObjectiveFlow();
        zRndr::SetActiveRegionSizeFromRect((HudUiRect *)(windowSection));
        zRndr::LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer(0, 2.0f);
        HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag(&g_Variant_CurrentTag);
        HudUiMgr::UpdateFrame();
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Tick();
        }
        zVideo::Dispatch_UnlockPrimarySurfaceState();
    } else {
        zVideo::RunPostprocessOnPrimaryBuffer();
        zVideo::FxPass3_UpdateLocal(g_FrameDeltaTimeSec);

        if (quitTransition != 0) {
            zVideo::Dispatch_UnlockPrimarySurfaceState();
            zVideo::AdjustSurfacesIfEnabled(0, 0, 0, 1);
            return 1;
        }

        g_HudSensorTracker.UpdateObjectiveFlow();
        zRndr::SetActiveRegionSizeFromRect((HudUiRect *)(windowSection));
        zRndr::LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer(0, 1.0f);
        HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag(&g_Variant_CurrentTag);
        HudUiMgr::UpdateFrame();
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Tick();
        }
        zVideo::Dispatch_UnlockPrimarySurfaceState();
    }

    if (shouldPresent != 0) {
        zVideo::AdjustSurfacesIfEnabled((zVidRect32 *)(windowSection),
                                        (zVidRect32 *)(windowSection), 0, 0);
    }

    return 0;
}
