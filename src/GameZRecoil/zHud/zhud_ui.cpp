#include "Battlesport/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"

#include "Battlesport/Briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/HudUiNetGameSetup.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zClass/cls_stubs.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zTurret/zTurret.h"
#include "GameZRecoil/zUtil/zZbd.h"

#include <cctype>
#include <cstdarg>
#include <math.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

LPCSTR __stdcall AfxRegisterWndClass(
    UINT classStyle,
    HCURSOR cursor,
    HBRUSH background,
    HICON icon
);

namespace {
const int ZOPT_GRAPHICS_PERSPECTIVE = 8;
const int ZOPT_GRAPHICS_GLOBAL_LIGHT = 0x10;
const int ZVID_HW_MODE_SOFTWARE = 0;
const float ZSND_CD_VOLUME_TO_NORMALIZED = 1.52590219e-05f;
const float ZSND_CD_NORMALIZED_TO_VOLUME = 65535.0f;
const unsigned int HUD_UI_NET_GAME_SETUP_FOCUS_TEXT_INPUT_OFFSET = 0xa94c;

/**
 * Provider boundary for 0x413eb0: NoOp::MethodStub.
 * Purpose: preserve the compiler-generated no-op method target used by HUD
 * teardown stubs without modeling it as authored HUD source.
 */
void __fastcall HudUiNoOpMethodStub(
    void *
) {}

bool IsCallableProviderAddress(
    unsigned int address
) {
    if (address == 0) {
        return false;
    }

    MEMORY_BASIC_INFORMATION info = {0};
    if (VirtualQuery((const void *)(address), &info, sizeof(info)) == 0 ||
        info.State != MEM_COMMIT) {
        return false;
    }

    const DWORD protect = info.Protect & 0xffu;
    return protect == PAGE_EXECUTE || protect == PAGE_EXECUTE_READ ||
           protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
}

struct HudReticleAttachStatePartial {
    unsigned char unknown_00[0x0c];
    zClass_NodePartial *projectileNode;
};

struct HudReticleAltGunControllerPartial {
    OptCatalogEntryDef *optCatalogEntry;
    unsigned char unknown_04[0x24];
    HudReticleAttachStatePartial *attachState;
};

struct HudReticlePlayerStatePartial {
    unsigned char unknown_000[0x58c];
    int cameraState;
    unsigned char unknown_590[0x54];
    HudReticleAltGunControllerPartial *activeAltGunController;
    unsigned char unknown_5e8[0x8e8];
    zClass_NodePartial *rootNode;
};

RECOIL_STATIC_ASSERT(offsetof(HudReticleAttachStatePartial, projectileNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudReticleAltGunControllerPartial, attachState) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, cameraState) == 0x58c);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, activeAltGunController) == 0x5e4);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, rootNode) == 0xed0);

} // namespace

struct zTimedTask {
    zTimedTask *next;
    int kind;
    int flags;
    float remainingSeconds;
    int actionArg0;
    int actionArg1;
    int actionArg2;
    int actionArg3;
    int actionArg4;
    unsigned char payload_24[0x94];
    int alphaPointCount;
    int alphaVariantIndex;
    int alpha255;
    unsigned char payload_c4[0x48];
    int rasterVertexCount;
    int rasterDrawParam;

    void RemoveFromActiveList();
    void RunImmediateAction();
    static void TickActiveList();
};

RECOIL_STATIC_ASSERT(offsetof(zTimedTask, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, kind) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, flags) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, remainingSeconds) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, actionArg0) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, actionArg4) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alphaPointCount) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alphaVariantIndex) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alpha255) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, rasterVertexCount) == 0x10c);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, rasterDrawParam) == 0x110);

zTimedTask *g_zTimedTask_ActiveHead = 0;
zTimedTask *g_zTimedTask_ActiveTail = 0;
int g_zTimedTask_ActiveCount = 0;

int g_HudCmdMouseDebounceFrames = 0;
zVidImagePartial *g_HudUiWidget_ExclusiveDrawImage = 0;
HudUiContainer g_HudUiMgr;
HudUiTransitionTextPanel g_HudUiMgrHudRootPanel;
HudUiTimerPanel *g_HudUiMgrTimerPanel = 0;
HudUiTimerPanelFloat *g_HudUiMgrTimerPanelFloat = 0;
HudUiStringMenu *g_HudUiMgrStringMenu = 0;
HudUiStatsListElement *g_HudUiMgrStatsList = 0;
zSndSample *g_HudUi_PowerupSample = 0;
unsigned char g_HudUi_PowerupSampleInitFlags = 0;
zVidImagePartial *g_HudUiMgrReticleImages[3] = {0};
zVidImagePartial *g_HudUiMgrSensorTargetMarkerImages[5] = {0};
HudUiNanitePanel g_HudUiMgrNanitePanel;
HudUiMessage g_HudUiMgrMessages[10];
int g_HudUiMgrActiveWeaponMessageIndex = 0;
int g_HudUiMgrActiveWeaponSideIndex = 0;
HudUiCounter g_HudUiMgrModeCounters[4];
HudUiSlot g_HudUiMgrWeaponSlots[32];
HudUiSlot *g_HudUiMgrSensorTrackedProgressSlot = 0;
int g_HudUiMgrSensorRoundRobinTrackIndex = 0;
HudUiRect g_HudUiMgrSensor_FxRectScratch = {0};
HudUiMeter g_HudUiMgrObjectiveMeter;
int g_HudUiMgrActiveModeCounterIndex = 0;
int g_HudUiMgrHudLayoutsInitialized = 0;
int g_HudUiMgrHudLoaded = 0;
int g_HudUiMgrLayoutDelayFrames = 0;
int g_HudUiMgrSensorTargetMarkerCount = 0;
int g_HudUiMgrWeaponState = 0;
int g_HudUiMgrHudOriginX = 0;
int g_HudUiMgrHudOriginY = 0;
HudUiNetGameSetupOverlayOwner g_HudUiNetGameSetupOverlayOwner;
int g_HudUiMgrStatsListState1 = 0;
int g_HudUiMgrStatsListState2 = 0;
int g_HudUiMgrStatsListState3 = 0;
int g_HudUiMgrStatsListState5 = 0;
HudUiMgrSensorBlock g_HudUiMgrSensorBlock = {0};
HudUiRect g_HudUiMgrSensorFxRect = {0};
int g_HudUiMgrSensorFxViewportWidth = 0;
int g_HudUiMgrSensorFxViewportHeight = 0;
HudUiRect g_HudUiMgrHudRect = {0};
HudUiRect g_HudUiMgrViewRect = {0};
float g_HudUiMgrHudRectW = 0.0f;
float g_HudUiMgrHudRectH = 0.0f;
float g_HudUiMgrReticleMapBiasX = 0.0f;
float g_HudUiMgrReticleMapBiasY = 0.0f;
float g_HudUiMgrReticleMapScaleHalfW = 0.0f;
float g_HudUiMgrReticleMapScaleHalfH = 0.0f;
int g_HudUiMgrReticleSnapRadiusSq = 0;
float g_HudUiMgrReticleProjection[3] = {0};
int g_HudUiMgrReticleWidgetHalfW = 0;
int g_HudUiMgrReticleWidgetHalfH = 0;
int g_HudUiMgrReticleProjectedX = 0;
int g_HudUiMgrReticleProjectedY = 0;
int g_HudUiMgrReticleMode = 0;
HudUiWidget g_HudUiMgrReticleWidget;
HudLayoutBase *g_HudUiMgrCurrentLayout = 0;
float g_HudUiLoadingCheckpointRawProgress[19] = {0};
float g_HudUiLoadingCheckpointProgress[19] = {0};
float g_HudUiLoadingCheckpointProgressScale = 0.0186219737f;
unsigned int g_HudUiLoadingCheckpointMaxIndex = 0;
unsigned int g_HudUiLoadingCheckpointCurrentIndex = 0;
float g_HudUiLoadingCheckpointCurrentProgress = 0.0f;
HudLayoutHW g_HudLayoutHW;
HudLayoutSW g_HudLayoutSW;
HudUiTextStack4 *g_HudUiTopMessageStack = 0;
HudUiTextStack4 *g_HudUiChatMessageStack = 0;
zFMV_Playback *g_HudUiSensorWindowPlayback = 0;
CWnd g_HudUiSensorWindow;
HudUiShieldMessageWidgetState *g_HudUiMgrShieldMessageWidget = 0;
int g_HudUiMgrObjectivePhase = 0;
int g_HudUiMgrObjectiveState = 0;
int g_HudUiMgrObjectiveChatComposeActive = 0;
float g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
float g_HudUiMgrObjectivePhaseDurationSec = 0.0f;
float g_HudUiMgrObjectiveAutoHideDelaySec = 0.0f;
int g_HudUiMgrObjectiveShowResetUnused = 0;
int g_HudUiMgrObjectiveWidgetRightX = 0;
float g_HudUiMgrObjectiveMeterFillAnimTimerSec = 0.0f;
unsigned int g_HudUiMgrObjectiveMeterFillAnimEnabled = 0;
HudUiWidget g_HudUiMgrSensorPanel;
HudUiWidget g_HudUiMgrObjectiveWidget;
HudUiObjectiveBar g_HudUiMgrObjectiveBar;
HudUiWidget g_HudUiMgrObjectiveSensorRect;
HudUiWidget g_HudUiMgrSensorOverlay;
HudUiMeter g_HudUiMgrSensorMeter;
HudUiPanel *g_HudUiMgrObjectiveSummaryTextPanel = 0;
HudUiPanel *g_HudUiMgrObjectiveDescTextPanel = 0;
HudUiPanel *g_HudUiMgrObjectiveLabelTextPanel = 0;
HudUiCounterTextPanel *g_HudUiMgrObjectiveCounterTextPanel = 0;
HudUiChatComposeTextInput g_HudUiMgrObjectiveChatComposeTextInput;
HudUiBar g_HudUiMgrTailBar;
int g_HudUi_AuxOverlayEnabled = 0;
HudCmdDialogState g_HudCmdDialogState;
CString g_HudUiTripletWndClassName("");

// Reimplements 0x40ec90: HudLayoutBase::Shutdown_Stub
void HudLayoutBase::Shutdown_Stub() {
    HudUiNoOpMethodStub(&g_HudUiMgrShieldMessageWidget->widget);
}

// Reimplements 0x40d3b0: HudLayoutBase::Destructor
void HudLayoutBase::Destructor() {
    widget0.DestructorCore();
    HudUiContainer::DestructorCore();
}

// Reimplements 0x412bd0: HudLayoutBase::SetActive
int HudLayoutBase::SetActive(
    int
) {
    return 1;
}

// Reimplements 0x412b60: HudLayoutSW::Constructor
// (D:\Proj\Battlesport\hud.cpp)
HudLayoutSW * HudLayoutSW::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;
    HudUiWidget *const childWidget = (HudUiWidget *)(&widget0);
    childWidget->Constructor(0);
    AddChild((HudUiElement *)(childWidget));
    return this;
}

// Reimplements 0x40d270: HudLayoutSW::GlobalInit
HudLayoutSW *HudLayoutSW::GlobalInit() {
    return g_HudLayoutSW.Constructor();
}

// Reimplements 0x40d280: HudLayoutSW::RegisterAtExit
void HudLayoutSW::RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x40d290: HudLayoutSW::AtExitDestructor
void HudLayoutSW::AtExitDestructor() {
    g_HudLayoutSW.GlobalDestructor();
}

// Reimplements 0x40d2a0: HudLayoutSW::GlobalDestructor
// (D:\Proj\Battlesport\hud.cpp)
void HudLayoutSW::GlobalDestructor() {
    ((HudUiWidget *)(&widget0))->DestructorCore();
    DestructorCore();
}

// Reimplements 0x412ea0: HudLayoutHW::Constructor
// (D:\Proj\Battlesport\hud.cpp)
HudLayoutHW * HudLayoutHW::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;
    HudUiWidget *const baseWidget = (HudUiWidget *)(&widget0);
    baseWidget->Constructor(0);
    AddChild((HudUiElement *)(baseWidget));

    HudUiWidget *const widget1Object = (HudUiWidget *)(&widget1);
    widget1Object->Constructor(0);
    HudUiWidget *const widget2Object = (HudUiWidget *)(&widget2);
    widget2Object->Constructor(0);
    HudUiWidget *const widget3Object = (HudUiWidget *)(&widget3);
    widget3Object->Constructor(0);

    AddChild((HudUiElement *)(widget1Object));
    AddChild((HudUiElement *)(widget3Object));
    AddChild((HudUiElement *)(widget2Object));
    return this;
}

// Reimplements 0x40d300: HudLayoutHW::GlobalInit
HudLayoutHW *HudLayoutHW::GlobalInit() {
    return g_HudLayoutHW.Constructor();
}

// Reimplements 0x40d310: HudLayoutHW::RegisterAtExit
void HudLayoutHW::RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x40d320: HudLayoutHW::AtExitDestructor
void HudLayoutHW::AtExitDestructor() {
    g_HudLayoutHW.GlobalDestructor();
}

// Reimplements 0x40d330: HudLayoutHW::GlobalDestructor
// (D:\Proj\Battlesport\hud.cpp)
void HudLayoutHW::GlobalDestructor() {
    ((HudUiWidget *)(&widget3))->DestructorCore();
    ((HudUiWidget *)(&widget2))->DestructorCore();
    ((HudUiWidget *)(&widget1))->DestructorCore();
    ((HudUiWidget *)(&widget0))->DestructorCore();
    DestructorCore();
}

// Reimplements 0x40d2f0: HudLayoutHW::CrtInitGlobalSingleton
void HudLayoutHW::CrtInitGlobalSingleton() {
    GlobalInit();
    RegisterAtExit();
}

// Reimplements 0x412be0: HudLayoutBase::UpdateAll
void HudLayoutBase::UpdateAll(
    float deltaSeconds
) {
    HudUiContainer::UpdateAll(deltaSeconds);
}

// Reimplements 0x412bf0: HudLayoutBase::Enable
void HudLayoutBase::LayoutPreUpdate() {
}

// Reimplements 0x412bf0: HudLayoutBase::Enable
void HudLayoutBase::Enable() {
    SetActive(1);
}

// Reimplements 0x412c00: HudLayoutBase::Disable
void HudLayoutBase::Disable() {
    SetActive(0);
}

void HudLayoutBase::OnActivated() {
}

// Reimplements 0x413500: HudLayoutHW::UpdateAll
// (D:\Proj\Battlesport\hud.cpp)
void HudLayoutHW::UpdateAll(
    float deltaSeconds
) {
    if (g_HudUiMgr.enabled != 0 && zOpt::GetReplicateMode() != 0 && g_HudUiMgrObjectivePhase == 0) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectScaled),
            (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw)
        );
    }

    HudUiContainer::UpdateAll(deltaSeconds);
}

// Reimplements 0x412c60: HudLayoutSW::SetActive
// (D:\Proj\Battlesport\hud.cpp)
int HudLayoutSW::SetActive(
    int active
) {
    if (zVid::GetAccelerationOption() == ZVID_HW_MODE_SOFTWARE) {
        zRndr::SpanOcclusionResetFrame();
    }

    activeRect.right = zVideo::GetPrimarySurfaceWidth();
    activeRect.bottom = layoutRect.bottom + g_HudUiMgrHudOriginY;
    HudLayout::ApplyViewportRect(&activeRect);

    if (active != 0) {
        HudUiRect outerRect;
        outerRect.top = activeRect.top + 1;
        outerRect.left = activeRect.left + 1;
        outerRect.right = activeRect.right - 1;
        outerRect.bottom = activeRect.bottom - 1;

        g_HudSensorTracker.SetBounds(
            &outerRect,
            &g_HudUiMgrSensorBlock.sensorViewportRect
        );
        g_HudUiMgr.SetChildFlags(0);
        SetChildFlags(0);
        zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);

        if (g_HudUiMgr.enabled != 0 && zVid::GetAccelerationOption() == ZVID_HW_MODE_SOFTWARE) {
            const int replicateMode = zOpt::GetReplicateMode();
            float nearClip = 0.0f;
            float farClip = 0.0f;
            zClass_Camera::gwCameraGetNearFarClip(
                g_MainCamera,
                &nearClip,
                &farClip
            );

            const float invNearClip = 1.0f / nearClip;
            zRndr::SpanOcclusionSubmitOccluderRect(
                &g_HudUiMgrSensorBlock.sensorViewportRect,
                replicateMode,
                invNearClip
            );
            zRndr::SpanOcclusionSubmitOccluderRect(
                &g_HudUiMgrShieldMessageWidget->screenRect,
                replicateMode,
                invNearClip
            );

            {
                for (int index = 0; index < 4; ++index) {
                    zRndr::SpanOcclusionSubmitOccluderRect(
                        &g_HudUiMgrModeCounters[index].clipViewportRect,
                        replicateMode,
                        invNearClip
                    );
                }
            }
        }
    }

    return 1;
}

// Reimplements 0x412c10: HudLayoutSW::LoadTypeIFromZarRoot
// (D:\Proj\Battlesport\hud.cpp)
void HudLayoutBase::LoadTypeIFromZarRoot(
    zReader::Node *parentNode
) {
    zReader::Node *const typeINode = zReader_GetNamedNode(
        parentNode,
        "TYPEI"
    );
    if (typeINode == 0) {
        return;
    }

    HudUiLayoutNode::ReadRectOffsetAndSize(
        &typeINode->value.nodes[1],
        &layoutRect,
        0,
        0,
        0
    );
    activeRect = layoutRect;
}

namespace HudLayout {
// Reimplements 0x412db0: HudLayout::ApplyViewportRect
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall ApplyViewportRect(
    HudUiRect *activeRect
) {
    const int replicateMode = zOpt::GetReplicateMode();
    const int left = activeRect->left;
    const int top = activeRect->top;

    zOpt::DisplaySection_SetPosition(
        left,
        top
    );

    int renderX = left;
    int renderY = top;
    if (replicateMode != 0) {
        renderX = (left - (left >> 31)) >> 1;
        renderY = (top - (top >> 31)) >> 1;
    }

    zOpt::RenderSection_SetPosition(
        renderX,
        renderY
    );

    int width = activeRect->right - left;
    int height = activeRect->bottom - top;
    zOpt::DisplaySection_SetSize(
        width,
        height
    );

    const float viewportWidth = (float)(width);
    const float viewportHeight = (float)(height);

    if (replicateMode != 0) {
        width = (width - (width >> 31)) >> 1;
        height = (height - (height >> 31)) >> 1;
    }

    zOpt::RenderSection_SetSize(
        width,
        height
    );

    zClass_NodePartial *const camera = g_HudSensorTracker.cameraNode;
    if (camera != 0) {
        float fovX = 0.0f;
        float fovY = 0.0f;
        zClass_Camera::gwCameraGetFOV(
            camera,
            &fovX,
            &fovY
        );
        fovY = viewportHeight / viewportWidth * fovX;
        zClass_Camera::gwCameraSetFOV(
            camera,
            fovX,
            fovY
        );
    }

    zOpt_ViewRectSection *const renderSection = zOpt::GetRenderSection();
    HudUiMgr::OnViewportChanged(
        (const HudUiRect *)(zOpt::GetDisplaySection()),
        (const HudUiRect *)(renderSection)
    );
    return 1;
}
} // namespace HudLayout

// Reimplements 0x413080: HudLayoutHW::ReleaseImages
void HudLayoutHW::ReleaseImages() {
    zVid_Image::ReleaseIfNotDefault(widget1Image320);
    zVid_Image::ReleaseIfNotDefault(widget1Image400);
    zVid_Image::ReleaseIfNotDefault(widget2Image320);
    zVid_Image::ReleaseIfNotDefault(widget2Image400);

    widget2Image400 = 0;
    widget2Image320 = 0;
    widget1Image400 = 0;
    widget1Image320 = 0;
}

// Reimplements 0x413340: HudLayoutHW::OnActivated
// (D:\Proj\Battlesport\hud.cpp)
void HudLayoutHW::OnActivated() {
    HudUi::SetInvalidateMode(zOpt::GetReplicateMode() == 0 ? 1 : 0);

    g_HudUiMgr.SetChildFlags(0x0e);
    SetChildFlags(0x0e);

    widget2.flags = (unsigned int)((unsigned char)(widget2.flags) & 0x10u);

    g_HudUiMgrObjectiveWidget.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveWidget.flags) & 0x10u);
    g_HudUiMgrObjectiveMeter.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveMeter.flags) & 0x10u);
    ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->flags =
        (unsigned int)((unsigned char)(((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))
                               ->flags) &
                       0x10u);
    g_HudUiMgrSensorOverlay.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrSensorOverlay.flags) & 0x10u);

    g_HudUiMgrStatsList->triplet->RebuildDisplay();

    {
        for (int index = 1; index < 10; ++index) {
            HudUiMessage &message = g_HudUiMgrMessages[index];
            if (message.widget.image != 0) {
                message.widget.flags =
                    (unsigned int)((unsigned char)(message.widget.flags) & 0x10u);
            }
        }
    }

    HudLayoutBase *const layout = (HudLayoutBase *)(this);
    HudUiRect outerRect;
    outerRect.left = layout->activeRect.left + 1;
    outerRect.top = layout->activeRect.top + 1;
    outerRect.right = layout->activeRect.right - 1;
    outerRect.bottom = layout->activeRect.bottom - 1;

    HudUiRect *innerRect = 0;
    if (zOpt::GetReplicateMode() == 0) {
        innerRect = &g_HudUiMgrSensorBlock.sensorViewportRect;
    }
    g_HudSensorTracker.SetBounds(
        &outerRect,
        innerRect
    );

    zVidImagePartial *widget1Image = widget1ImageDefault;
    zVidImagePartial *widget2Image = widget2ImageDefault;
    if (layout->activeRect.right == 0x320) {
        widget1Image = widget1Image320;
        widget2Image = widget2Image320;
    } else if (layout->activeRect.right == 0x400) {
        widget1Image = widget1Image400;
        widget2Image = widget2Image400;
    }

    widget2.SetImageBorrowedAndInvalidate(widget2Image);
    widget1.SetImageBorrowedAndInvalidate(widget1Image);

    if (g_HudUiMgrHudLayoutsInitialized != 0) {
        widget2.InvalidateRect(&g_HudUiMgrViewRect);
    }
}

// Reimplements 0x4132b0: HudLayoutHW::UpdateObjectiveDirtyRect
// (D:\Proj\Battlesport\hud.cpp)
void HudLayoutHW::UpdateObjectiveDirtyRect() {
    zVidImagePartial *const image = g_HudUiMgrObjectiveWidget.image;
    const int width = image != 0 ? image->width : 0;

    const int centerX = g_HudUiMgrObjectiveWidget.GetCenterX();
    const int centerY = g_HudUiMgrObjectiveWidget.GetCenterY();

    const int height = image != 0 ? image->height : 0;
    HudUiRect dirtyRect;
    dirtyRect.left = centerX + width;
    dirtyRect.top = centerY;
    dirtyRect.right = g_HudUiMgrObjectiveWidgetRightX;
    dirtyRect.bottom = centerY + height;

    widget2.InvalidateRect(&dirtyRect);
    ((HudUiElement *)(&g_HudUiMgrNanitePanel))->Invalidate();
    ((HudUiTripletPanel *)(&g_HudUiMgrNanitePanel))->Draw();
}

// Reimplements 0x4130d0: HudLayoutHW::SetActive
// (D:\Proj\Battlesport\hud.cpp)
int HudLayoutHW::SetActive(
    int active
) {
    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionResetFrame();
    }

    HudLayoutBase *const layout = (HudLayoutBase *)(this);
    layout->activeRect.right = zVideo::GetPrimarySurfaceWidth();
    layout->activeRect.bottom = layout->layoutRect.bottom + g_HudUiMgrHudOriginY;
    HudLayout::ApplyViewportRect(&layout->activeRect);

    if (active == 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))->SetBltSourceAndClipRect(
            0,
            0
        );
        ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetBltSourceAndClipRect(
            0,
            0
        );
        ((HudUiElement *)(&g_HudUiMgrNanitePanel))->SetBltSourceAndClipRect(
            0,
            0
        );

        {
            for (int index = 0; index < 10; ++index) {
                HudUiMessage &message = g_HudUiMgrMessages[index];
                message.SetBltSourceAndClipRect(
                    0,
                    0
                );
                message.panel.SetBltSourceAndClipRect(
                    0,
                    0
                );
            }
        }

        const int clearState = zVideo::ExchangeClearScreenBufferEnabled(1);
        zVideo::CallClearPrimarySurfaceAndZBuffer(0);
        zVideo::ExchangeClearScreenBufferEnabled(clearState);
        return 1;
    }

    layout->OnActivated();

    zVidImagePartial *const widget1Image = widget1.image;
    zVidImagePartial *const widget2Image = widget2.image;
    ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))
        ->SetBltSourceAndClipRect(
            widget1Image,
            0
        );
    ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetBltSourceAndClipRect(
        widget1Image,
        0
    );

    {
        for (int index = 1; index < 10; ++index) {
            HudUiMessage &message = g_HudUiMgrMessages[index];
            message.SetBltSourceAndClipRect(
                widget2Image,
                0
            );
            message.panel.SetBltSourceAndClipRect(
                widget2Image,
                0
            );
        }
    }

    ((HudUiElement *)(&g_HudUiMgrNanitePanel))->SetBltSourceAndClipRect(
        widget2Image,
        0
    );
    zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);

    if (g_HudUiMgr.enabled != 0 && zVid::GetAccelerationOption() == 0) {
        HudUiRect occluderRect;
        occluderRect.left = g_HudUiMgrSensorBlock.sensorViewportRect.left;
        occluderRect.top = g_HudUiMgrSensorBlock.sensorViewportRect.top;
        occluderRect.right = g_HudUiMgrSensorBlock.sensorViewportRect.right;
        occluderRect.bottom = g_HudUiMgrHudRect.bottom;

        float nearClip = 0.0f;
        float farClip = 0.0f;
        zClass_Camera::gwCameraGetNearFarClip(
            g_MainCamera,
            &nearClip,
            &farClip
        );
        zRndr::SpanOcclusionSubmitOccluderRect(
            &occluderRect,
            zOpt::GetReplicateMode(),
            1.0f / nearClip
        );
    }

    return 1;
}

// Reimplements 0x412f70: HudLayoutHW::LoadTypeIIFromZarRoot
// (D:\Proj\Battlesport\hud.cpp)
int HudLayoutHW::LoadTypeIIFromZarRoot(
    zReader::Node *parentNode
) {
    zReader::Node *const typeIINode = zReader_GetNamedNode(
        parentNode,
        "TYPEII"
    );
    if (typeIINode == 0) {
        return 1;
    }

    zReader::Node *const typeIIPayload = typeIINode->value.nodes;
    HudLayoutBase *const layout = (HudLayoutBase *)(this);

    HudUiLayoutNode::ReadRectOffsetAndSize(
        &typeIIPayload[1],
        &layout->layoutRect,
        0,
        0,
        0
    );
    layout->activeRect = layout->layoutRect;

    HudUiLayoutNode::ApplyImageWidget(
        &typeIIPayload[2],
        &widget1,
        0,
        0,
        0,
        0,
        0
    );
    HudUiLayoutNode::ApplyImageWidget(
        &typeIIPayload[3],
        &widget3,
        0,
        g_HudUiMgrHudOriginY,
        0,
        0,
        0
    );
    HudUiLayoutNode::ApplyImageWidget(
        &typeIIPayload[4],
        &widget2,
        0,
        g_HudUiMgrHudOriginY,
        0,
        0,
        0
    );

    zReader::Node *const imageNames = typeIIPayload[5].value.nodes;
    widget1ImageDefault = widget1.image;
    widget1Image320 = zImage::TexDir_FindOrCreateByPath(imageNames[1].value.str);
    widget1Image400 = zImage::TexDir_FindOrCreateByPath(imageNames[2].value.str);
    widget2ImageDefault = widget2.image;
    widget2Image320 = zImage::TexDir_FindOrCreateByPath(imageNames[3].value.str);
    widget2Image400 = zImage::TexDir_FindOrCreateByPath(imageNames[4].value.str);

    return 1;
}

// Reimplements 0x413540: HudLayoutHW::Enable
// (D:\Proj\Battlesport\hud.cpp)
void HudLayoutHW::Enable() {
    g_HudUiMgr.SetChildFlags(0x0e);
    SetChildFlags(0x0e);

    widget2.flags = (unsigned int)((unsigned char)(widget2.flags) & 0x10u);

    g_HudUiMgrObjectiveWidget.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveWidget.flags) & 0x10u);
    g_HudUiMgrObjectiveMeter.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveMeter.flags) & 0x10u);
    ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->flags =
        (unsigned int)((unsigned char)(((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))
                               ->flags) &
                       0x10u);
    g_HudUiMgrSensorOverlay.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrSensorOverlay.flags) & 0x10u);

    {
        for (int index = 1; index < 10; ++index) {
            HudUiMessage &message = g_HudUiMgrMessages[index];
            if (message.widget.image != 0) {
                message.widget.flags =
                    (unsigned int)((unsigned char)(message.widget.flags) & 0x10u);
            }
        }
    }

    SetEnabled(1);
}

// Reimplements 0x4135f0: HudLayoutHW::Disable
// (D:\Proj\Battlesport\hud.cpp)
void HudLayoutHW::Disable() {
    SetEnabled(0);
}

extern "C" {
unsigned int g_HudUi_InvalidateMask = 0x0c;
}

namespace {
const char kNumericTextInputAcceptedRawKeyChars[] = "0123456789.-\x1b\r\x08\x7f\x02\x06";
const char kClampedIntTextInputAcceptedRawKeyChars[] = "0123456789\x1b\r\x08\x7f\x02\x06";

#if defined(_MSC_VER) && _MSC_VER < 1200
// VC5 misparses explicit function-template calls such as FieldAt<unsigned int>(...).
// Keep the same call-site spelling for first-pass VC5 verification without changing
// modern compiler codegen.
template <typename T> class FieldAt {
  public:
    FieldAt(
        void *base,
        size_t offset
    )
        : address((T *)((unsigned char *)(base) + offset)) {}

    FieldAt(
        const void *base,
        size_t offset
    )
        : address((T *)((const unsigned char *)(base) + offset)) {}

    operator T &() {
        return *address;
    }

    T *operator&() const {
        return address;
    }

    FieldAt &operator=(
        const T &value
    ) {
        *address = value;
        return *this;
    }

    FieldAt &operator|=(
        const T &value
    ) {
        *address |= value;
        return *this;
    }

    FieldAt &operator+=(
        const T &value
    ) {
        *address += value;
        return *this;
    }

    FieldAt &operator-=(
        const T &value
    ) {
        *address -= value;
        return *this;
    }

  private:
    T *address;
};
#else
template <typename T>
T &FieldAt(
    void *base,
    size_t offset
) {
    return *(T *)((unsigned char *)(base) + offset);
}

template <typename T>
const T &FieldAt(
    const void *base,
    size_t offset
) {
    return *(const T *)((const unsigned char *)(base) + offset);
}
#endif

HudUiPanel *NewSimplePanel(
    int fontSize,
    int fontWeight
) {
    HudUiPanel *const panel = (HudUiPanel *)(::operator new(0x2a4));
    ((HudUiPanelSimple *)(panel))->Constructor(
        0,
        0,
        0
    );
    panel->SetFont(
        "Arial",
        fontSize,
        0x1f4,
        fontWeight,
        0,
        0,
        2
    );
    ((HudUiElement *)(panel))->SetVisible(0);
    return panel;
}

size_t HudUiTripletEntryCount(
    const HudUiTripletEntries &entries
) {
    return (size_t)(((HudUiTripletEntries *)(&entries))->GetCount());
}

size_t HudUiTripletEntryCapacity(
    const HudUiTripletEntries &entries
) {
    if (entries.begin == 0) {
        return 0;
    }

    return (size_t)(entries.cap - entries.begin);
}

int HudUiTripletEntrySortKey(
    const HudUiScoreboardEntry &entry
) {
    return entry.score + entry.lapCount * 1000;
}

namespace HudUiListMenuEntry {

// Reimplements 0x40d220: HudUiListMenuEntry::CompareSortKey
// (D:\Proj\Battlesport\HudUiListMenu.cpp)
int __fastcall CompareSortKey(
    const HudUiScoreboardEntry *entryA,
    const HudUiScoreboardEntry *entryB
) {
    const unsigned int keyA = (unsigned int)(HudUiTripletEntrySortKey(*entryA));
    const unsigned int keyB = (unsigned int)(HudUiTripletEntrySortKey(*entryB));
    if (keyA != keyB) {
        return keyB < keyA ? 1 : 0;
    }

    return (unsigned int)(entryB->playerKey) < (unsigned int)(entryA->playerKey) ? 1 : 0;
}

bool EntryComesBefore(
    const HudUiScoreboardEntry &lhs,
    const HudUiScoreboardEntry &rhs
) {
    return CompareSortKey(
        &lhs,
        &rhs
    ) != 0;
}

// Reimplements 0x414930: HudUiListMenuEntry::InsertPivotIntoSortedPrefix
// (D:\Proj\Battlesport\HudUiListMenu.cpp)
void InsertPivotIntoSortedPrefix(
    HudUiScoreboardEntry *slot,
    const HudUiScoreboardEntry &pivot
) {
    HudUiScoreboardEntry *insertSlot = slot;
    HudUiScoreboardEntry *previousEntry = insertSlot - 1;
    while (EntryComesBefore(
        pivot,
        *previousEntry
    )) {
        *insertSlot = *previousEntry;
        insertSlot = previousEntry;
        --previousEntry;
    }

    *insertSlot = pivot;
}

// Reimplements 0x414980: HudUiListMenuEntry::InsertionSortRange
// (D:\Proj\Battlesport\HudUiListMenu.cpp)
void __fastcall InsertionSortRange(
    HudUiScoreboardEntry *begin,
    HudUiScoreboardEntry *end,
    int
) {
    if (begin == end) {
        return;
    }

    HudUiScoreboardEntry *current = begin + 1;
    while (current != end) {
        HudUiScoreboardEntry candidate = *current;
        if (EntryComesBefore(
            candidate,
            *begin
        )) {
            HudUiScoreboardEntry *shiftCursor = current;
            while (shiftCursor != begin) {
                *shiftCursor = *(shiftCursor - 1);
                --shiftCursor;
            }

            *begin = candidate;
        } else {
            InsertPivotIntoSortedPrefix(
                current,
                candidate
            );
        }

        ++current;
    }
}

HudUiScoreboardEntry *MedianOfThree(
    HudUiScoreboardEntry *first,
    HudUiScoreboardEntry *middle,
    HudUiScoreboardEntry *last
) {
    if (EntryComesBefore(
        *first,
        *middle
    )) {
        if (EntryComesBefore(
            *middle,
            *last
        )) {
            return middle;
        }

        return EntryComesBefore(
            *first,
            *last
        ) ? last : first;
    }

    if (EntryComesBefore(
        *first,
        *last
    )) {
        return first;
    }

    return EntryComesBefore(
        *middle,
        *last
    ) ? last : middle;
}

// Reimplements 0x414710: HudUiListMenuEntry::SortRange
// (D:\Proj\Battlesport\HudUiListMenu.cpp)
void __fastcall SortRange(
    HudUiScoreboardEntry *begin,
    HudUiScoreboardEntry *end,
    int unusedFlags
) {
    InsertionSortRange(
        begin,
        end,
        unusedFlags
    );
}

} // namespace HudUiListMenuEntry

void HudUiTripletInsertionSort(
    HudUiScoreboardEntry *begin,
    HudUiScoreboardEntry *end
) {
    if (begin == 0 || begin == end) {
        return;
    }

    HudUiListMenuEntry::InsertionSortRange(
        begin,
        end,
        0
    );
}

void HudUiTripletEnsureCapacity(
    HudUiTripletEntries &entries,
    size_t neededCount
) {
    const size_t count = HudUiTripletEntryCount(entries);
    const size_t capacity = HudUiTripletEntryCapacity(entries);
    if (neededCount <= capacity) {
        return;
    }

    const size_t growth = count > 1 ? count : 1;
    size_t newCapacity = count + growth;
    if (newCapacity < neededCount) {
        newCapacity = neededCount;
    }

    HudUiScoreboardEntry *const newBegin =
        (HudUiScoreboardEntry *)(::operator new(newCapacity * sizeof(HudUiScoreboardEntry)));

    HudUiTripletEntries::CopyRange(
        entries.begin,
        entries.end,
        newBegin
    );

    ::operator delete(entries.begin);
    entries.begin = newBegin;
    entries.end = newBegin + count;
    entries.cap = newBegin + newCapacity;
}

void HudUiTripletSetPanelTextColor(
    HudUiPanel *panel,
    unsigned int color
) {
    panel->textColor0 = color;
    panel->textColor1 = color;
    panel->textDirty = 1;
}

void HudUiTripletSetPanelVisible(
    HudUiPanel *panel,
    int visible
) {
    ((HudUiElement *)(panel))->SetVisible(visible);
}

void HudUiTripletPrepareCell(
    HudUiTriplet *triplet,
    HudUiPanel *panel,
    unsigned int color
) {
    HudUiTripletSetPanelTextColor(
        panel,
        color
    );
    HudUiElement *const element = (HudUiElement *)(panel);
    element->flags = (element->flags & 0x10u) | 0x0cu;
    panel->SetFont(
        "Arial",
        triplet->fontSize,
        0x1f4,
        triplet->fontWeight,
        0,
        0,
        2
    );
}

template <typename T> T *AllocateHudObject() {
    return (T *)(::operator new(sizeof(T)));
}

template <typename T>
zZbdSectionCallback ZbdCallbackPtr(
    T callback
) {
    RECOIL_STATIC_ASSERT(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T callback;
        zZbdSectionCallback raw;
    } value = {0};
    value.callback = callback;
    return value.raw;
}

HudUiPanel *NewObjectivePanel() {
    HudUiPanelSimple *const storage = AllocateHudObject<HudUiPanelSimple>();
    if (storage == 0) {
        return 0;
    }

    storage->Constructor(
        0,
        0,
        0
    );
    return (HudUiPanel *)(storage);
}

HudUiPanel *TextStackLineAt(
    HudUiTextStack4 *stack,
    int index
) {
    return (HudUiPanel *)(&stack->lines[index][0]);
}

const char kHudUiMessageClearSpecialToken165[] = "\xa5";
const float kHudUiMessageClearSpecialTokenValue = 123456792.0f;

void HudUiSetPanelVectorVisible(
    HudUiPanelPtrVector &panels,
    int visible
) {
    for (HudUiPanel **it = panels.begin; it != panels.end; ++it) {
        (*it)->SetVisible(visible);
    }
}

zReader::Node *ZrdArrayBase(
    zReader::Node *node
) {
    if (node == 0 || node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    return node->value.nodes;
}

int ZrdArrayCount(
    zReader::Node *arrayBase
) {
    return arrayBase != 0 ? arrayBase[0].value.i32 : 0;
}

zReader::Node *ZrdArrayItem(
    zReader::Node *arrayBase,
    int index
) {
    return arrayBase != 0 ? &arrayBase[index] : 0;
}

const char *ZrdArrayString(
    zReader::Node *arrayBase,
    int index
) {
    zReader::Node *const item = ZrdArrayItem(
        arrayBase,
        index
    );
    return item != 0 && item->type == zReader::ZRDR_NODE_STRING ? item->value.str : 0;
}

int ZrdArrayInt(
    zReader::Node *arrayBase,
    int index,
    int fallback
) {
    zReader::Node *const item = ZrdArrayItem(
        arrayBase,
        index
    );
    return item != 0 && item->type == zReader::ZRDR_NODE_INT ? item->value.i32 : fallback;
}

float ZrdArrayFloat(
    zReader::Node *arrayBase,
    int index,
    float fallback
) {
    zReader::Node *const item = ZrdArrayItem(
        arrayBase,
        index
    );
    if (item == 0) {
        return fallback;
    }

    if (item->type == zReader::ZRDR_NODE_FLOAT) {
        return item->value.f32;
    }

    if (item->type == zReader::ZRDR_NODE_INT) {
        return (float)(item->value.i32);
    }

    return fallback;
}

} // namespace

// Reimplements 0x414670: HudUiTripletEntries::GetCount
int HudUiTripletEntries::GetCount() {
    if (begin == 0) {
        return 0;
    }

    return (int)(end - begin);
}

// Reimplements 0x4146a0: HudUiTripletEntries::CopyRange
HudUiScoreboardEntry *__stdcall HudUiTripletEntries::CopyRange(
    HudUiScoreboardEntry *sourceBegin,
    HudUiScoreboardEntry *sourceEnd,
    HudUiScoreboardEntry *dest
) {
    HudUiScoreboardEntry *cursor = dest;
    while (sourceBegin != sourceEnd) {
        if (cursor != 0) {
            *cursor = *sourceBegin;
        }
        ++sourceBegin;
        cursor = (HudUiScoreboardEntry *)((unsigned char *)cursor + sizeof(HudUiScoreboardEntry));
    }

    return cursor;
}

// Reimplements 0x4146e0: HudUiTripletEntries::FillN
void __stdcall HudUiTripletEntries::FillN(
    HudUiScoreboardEntry *dest,
    unsigned int count,
    const HudUiScoreboardEntry *sourceValue
) {
    HudUiScoreboardEntry *cursor = dest;
    while (count != 0) {
        if (cursor != 0) {
            *cursor = *sourceValue;
        }
        cursor = (HudUiScoreboardEntry *)((unsigned char *)cursor + sizeof(HudUiScoreboardEntry));
        --count;
    }
}

namespace HudUiLayoutNode {
// Reimplements 0x413aa0: HudUiLayoutNode::ReadRect
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall ReadRect(
    zReader::Node *node,
    HudUiRect *outRect
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    outRect->left = arrayBase[1].value.i32;
    outRect->right = arrayBase[2].value.i32;
    outRect->top = arrayBase[3].value.i32;
    outRect->bottom = arrayBase[4].value.i32;
    return 1;
}

// Reimplements 0x413ad0: HudUiLayoutNode::ReadInt3
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall ReadInt3(
    zReader::Node *node,
    int *out0,
    int *out1,
    int *out2
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    if (out0 != 0) {
        *out0 = arrayBase[1].value.i32;
    }

    if (out1 != 0) {
        *out1 = arrayBase[2].value.i32;
    }

    if (out2 != 0) {
        *out2 = arrayBase[3].value.i32;
    }

    return 1;
}

// Reimplements 0x413b10: HudUiLayoutNode::ApplyCornerTextQuad
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall ApplyCornerTextQuad(
    zReader::Node *node,
    HudUiBar *target,
    const int *offsetXY,
    HudUiRect *outRect
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    int left = arrayBase[1].value.i32;
    int top = arrayBase[2].value.i32;
    int right = arrayBase[3].value.i32;
    int bottom = arrayBase[4].value.i32;

    if (offsetXY != 0) {
        left += offsetXY[0];
        top += offsetXY[1];
        right += offsetXY[0];
        bottom += offsetXY[1];
    }

    const float leftF = (float)(left);
    const float topF = (float)(top);
    const float rightF = (float)(right);
    const float bottomF = (float)(bottom);
    target->SetPointXY(
        0,
        leftF,
        topF
    );
    target->SetPointXY(
        1,
        leftF,
        bottomF
    );
    target->SetPointXY(
        2,
        rightF,
        bottomF
    );
    target->SetPointXY(
        3,
        rightF,
        topF
    );

    if (outRect != 0) {
        outRect->left = left;
        outRect->top = top;
        outRect->right = right;
        outRect->bottom = bottom;
    }

    return 1;
}

// Reimplements 0x413c10: HudUiLayoutNode::ApplyMeterQuad
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall ApplyMeterQuad(
    zReader::Node *node,
    HudUiMeter *target,
    int xBase,
    int yBase,
    const int *offsetXY,
    HudUiRect *outRect
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    int left = arrayBase[1].value.i32;
    const int top = arrayBase[2].value.i32;
    int right = arrayBase[3].value.i32 + 1;
    const int bottom = arrayBase[4].value.i32 + 1;

    if (outRect != 0) {
        outRect->left = left;
        outRect->top = top;
        outRect->right = right;
        outRect->bottom = bottom;
    }

    right += xBase;
    int topY = top + yBase;
    int bottomY = bottom + yBase;

    if (offsetXY != 0) {
        left += offsetXY[0];
        topY += offsetXY[1];
        right += offsetXY[0];
        bottomY += offsetXY[1];
    }

    const int width = right - left;
    const int height = bottomY - topY;
    HudUiBar *const bar = (HudUiBar *)(target);
    bar->SetPointXY(
        0,
        (float)(left),
        (float)(topY)
    );
    bar->SetPointXY(
        1,
        (float)(left),
        (float)(height + topY)
    );
    bar->SetPointXY(
        2,
        (float)(width + left + 1),
        (float)(height + topY)
    );
    bar->SetPointXY(
        3,
        (float)(width + left + 1),
        (float)(topY)
    );

    target->fillPixelsMax = height;
    target->meterFlags = (unsigned int)(width);
    return 1;
}

// Reimplements 0x413a10: HudUiLayoutNode::ReadRectOffsetAndSize
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall ReadRectOffsetAndSize(
    zReader::Node *node,
    HudUiRect *outRect,
    const int *offsetXY,
    int *outWidth,
    int *outHeight
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    outRect->left = arrayBase[1].value.i32;
    outRect->top = arrayBase[2].value.i32;
    outRect->right = arrayBase[3].value.i32;
    outRect->bottom = arrayBase[4].value.i32;

    if (offsetXY != 0) {
        outRect->left += offsetXY[0];
        outRect->top += offsetXY[1];
        outRect->right += offsetXY[0];
        outRect->bottom += offsetXY[1];
    }

    if (outWidth != 0) {
        *outWidth = outRect->right - outRect->left;
    }

    if (outHeight != 0) {
        *outHeight = outRect->bottom - outRect->top;
    }

    return 1;
}

// Reimplements 0x413990: HudUiLayoutNode::ApplyTextLabel
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall ApplyTextLabel(
    zReader::Node *layoutNode,
    HudUiPanel *target,
    int baseX,
    int baseY,
    const int *offsetXY
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    const char *const text = payload[1].value.str;
    int x = payload[2].value.i32 + baseX;
    int y = payload[3].value.i32 + baseY;
    if (offsetXY != 0) {
        x += offsetXY[0];
        y += offsetXY[1];
    }

    target->SetPos(
        x,
        y
    );
    target->SetTextFmt(
        text != 0 ? text : ""
    );
    return 1;
}

// Reimplements 0x413d30: HudUiLayoutNode::ApplyImageWidget
// (D:\Proj\Battlesport\hud.cpp)
zVidImagePartial *__fastcall ApplyImageWidget(
    zReader::Node *layoutNode,
    HudUiWidget *widget,
    int baseX,
    int baseY,
    const int *anchorOrNull,
    zVidImagePartial *preloadedImageOrNull,
    HudUiRect *outRectOrNull
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    const char *const imagePath = payload[1].value.str;
    int x = payload[2].value.i32 + baseX;
    int y = payload[3].value.i32 + baseY;

    if (anchorOrNull != 0) {
        x += anchorOrNull[0];
        y += anchorOrNull[1];
    }

    unsigned short visibleState = 0;
    int centerImage = 0;
    if (payload[0].value.i32 == 6) {
        visibleState = (unsigned short)(strcmp(
            payload[4].value.str,
            "TRUE"
        ) == 0 ? 1 : 0);
        centerImage = strcmp(
            payload[5].value.str,
            "TRUE"
        ) == 0 ? 1 : 0;
    }

    zVidImagePartial *image = preloadedImageOrNull;
    if (image != 0) {
        widget->SetImageBorrowedAndInvalidate(image);
    } else {
        image = widget->SetImageByPathOwned(imagePath);
    }

    if (image == 0) {
        return 0;
    }

    if (centerImage != 0) {
        x -= (int)(image->width) / 2;
        y -= (int)(image->height) / 2;
    }

    widget->SetPos(
        x,
        y
    );
    widget->imageStateWord = (widget->imageStateWord & 0xffff0000u) | visibleState;
    widget->Invalidate();

    if (outRectOrNull != 0) {
        outRectOrNull->left = x;
        outRectOrNull->top = y;
        outRectOrNull->right = x + image->width;
        outRectOrNull->bottom = y + image->height;
    }

    return image;
}
} // namespace HudUiLayoutNode

namespace {

struct HudUiListSelectorItemArrayHeader {
    int count;
};

const HudFontStyle *HudUiZrdOwnerFontStyle(
    const HudUiBackground *owner,
    int styleIndex
) {
    const HudFontStyle *const style = &owner->fontStyles[styleIndex];
    return style->validMarker != 0 ? style : 0;
}

void ApplyHudFontStyleToPanel(
    HudUiPanel *panel,
    const HudFontStyle *style
) {
    if (style == 0) {
        return;
    }

    panel->SetFont(
        style->fontName,
        style->fontSize,
        style->fontWeight,
        0,
        0,
        0,
        2
    );
    panel->alignMode = style->alignMode;
    panel->textColor0 = style->textColor;
    panel->textColor1 = style->textColor;
    panel->textDirty = 1;
    panel->shadowEnabled = style->shadowEnabled;
    panel->shadowOffsetX = 1;
    panel->shadowOffsetY = 1;
    panel->bkMode = style->bkMode;
    panel->bkColor = style->bkColor;
}

void ApplyHudFontStyleTextOnly(
    HudUiPanel *panel,
    const HudFontStyle *style
) {
    if (style == 0) {
        return;
    }

    panel->SetFont(
        style->fontName,
        style->fontSize,
        style->fontWeight,
        0,
        0,
        0,
        2
    );
    panel->textColor0 = style->textColor;
    panel->textColor1 = style->textColor;
    panel->textDirty = 1;
    panel->shadowEnabled = style->shadowEnabled;
    panel->shadowOffsetX = 1;
    panel->shadowOffsetY = 1;
}

void DeleteHudUiListSelectorItemArray(
    HudUiListSelectorItem *items
) {
    if (items == 0) {
        return;
    }

    HudUiListSelectorItemArrayHeader *const header =
        ((HudUiListSelectorItemArrayHeader *)(items)) - 1;
    const int count = header->count;
    {
        for (int index = 0; index < count; ++index) {
            ((HudUiPanel *)(&items[index]))->Destructor();
        }
    }

    ::operator delete(header);
}

HudUiPanel *CreateHudZrdLabelPanel(
    HudUiZrdWidget *widget,
    zReader::Node *labelSpecBase,
    int originX,
    int originY
) {
    HudUiTransitionTextPanel *const transitionPanel =
        (HudUiTransitionTextPanel *)(::operator new(sizeof(HudUiTransitionTextPanel)));
    new (transitionPanel) HudUiTransitionTextPanel;

    HudUiPanel *const panel = (HudUiPanel *)(transitionPanel);
    HudUiElement *const element = (HudUiElement *)(transitionPanel);
    element->flags = (element->flags & 0x10u) | 0x02u;

    const char *const key = ZrdArrayString(
        labelSpecBase,
        1
    );
    const char *const text = key != 0 ? zLoc::ResolveMessageKeyOrFallback(key) : "";
    panel->SetTextFmt(text != 0 ? text : "");

    element->SetPos(
        originX + ZrdArrayInt(
            labelSpecBase,
            2,
            0
        ),
        originY + ZrdArrayInt(labelSpecBase, 3, 0)
    );

    const int styleIndex = ZrdArrayInt(
        labelSpecBase,
        4,
        0
    );
    ApplyHudFontStyleToPanel(
        panel,
        HudUiZrdOwnerFontStyle(widget->owner, styleIndex)
    );

    element->SetVisible(1);
    ((HudUiContainer *)(widget->owner))->AddChild(element);
    return panel;
}

void AppendHudZrdLabelPanel(
    HudUiZrdWidget *widget,
    HudUiPanelPtrVector &panels,
    zReader::Node *labelSpecBase,
    int originX,
    int originY
) {
    HudUiPanel *panel = CreateHudZrdLabelPanel(
        widget,
        labelSpecBase,
        originX,
        originY
    );
    panels.InsertN(
        panels.end,
        1,
        &panel
    );
}

HudUiPanel *CreateHudZrdTextPanel(
    HudUiZrdWidget *widget,
    zReader::Node *textNode,
    int visible
) {
    zReader::Node *const textBase = ZrdArrayBase(textNode);
    if (textBase == 0) {
        return 0;
    }

    HudUiTransitionTextPanel *const transitionPanel =
        (HudUiTransitionTextPanel *)(::operator new(sizeof(HudUiTransitionTextPanel)));
    new (transitionPanel) HudUiTransitionTextPanel;

    HudUiPanel *const panel = (HudUiPanel *)(transitionPanel);
    const char *const key = ZrdArrayString(
        textBase,
        1
    );
    const char *const text = key != 0 ? zLoc::ResolveMessageKeyOrFallback(key) : "";
    panel->SetTextFmt(text != 0 ? text : "");

    HudUiElement *const element = (HudUiElement *)(transitionPanel);
    element->SetPos(
        widget->originX + ZrdArrayInt(
            textBase,
            2,
            0
        ),
        widget->originY + ZrdArrayInt(textBase, 3, 0)
    );

    const int styleIndex = ZrdArrayInt(
        textBase,
        4,
        0
    );
    ApplyHudFontStyleTextOnly(
        panel,
        HudUiZrdOwnerFontStyle(widget->owner, styleIndex)
    );

    element->SetVisible(visible);
    ((HudUiContainer *)(widget->owner))->AddChild(element);
    return panel;
}

void LoadHudZrdLabelSection(
    HudUiZrdWidget *widget,
    zReader::Node *parentNode,
    HudUiPanelPtrVector &panels
) {
    zReader::Node *const labelNode = zReader_GetNamedNode(
        parentNode,
        "LABEL"
    );
    zReader::Node *const labelBase = ZrdArrayBase(labelNode);
    if (labelBase == 0) {
        return;
    }

    const int originX = widget->originX;
    const int originY = widget->originY;
    zReader::Node *const firstItem = ZrdArrayItem(
        labelBase,
        1
    );
    if (firstItem != 0 && firstItem->type == zReader::ZRDR_NODE_ARRAY) {
        const int count = ZrdArrayCount(labelBase);
        {
            for (int index = 1; index <= count - 1; ++index) {
                AppendHudZrdLabelPanel(
                    widget,
                    panels,
                    ZrdArrayBase(ZrdArrayItem(
                        labelBase,
                        index
                    )),
                    originX,
                    originY
                );
            }
        }
        return;
    }

    AppendHudZrdLabelPanel(
        widget,
        panels,
        labelBase,
        originX,
        originY
    );
}

void ApplyHudZrdFlashSection(
    zReader::Node *parentNode,
    HudUiPanelPtrVector &panels
) {
    zReader::Node *const flashNode = zReader_GetNamedNode(
        parentNode,
        "FLASH"
    );
    if (flashNode == 0) {
        return;
    }

    float flashRate = 0.0f;
    zReader::ReadNamedFloat(
        flashNode,
        "RATE",
        &flashRate
    );

    unsigned int flashColor = 0;
    zReader::Node *const colorNode = zReader_GetNamedNode(
        flashNode,
        "COLOR"
    );
    zReader::Node *const colorBase = ZrdArrayBase(colorNode);
    if (colorBase != 0) {
        const unsigned int red = (unsigned int)(ZrdArrayInt(
            colorBase,
            1,
            0
        )) & 0xffu;
        const unsigned int green = (unsigned int)(ZrdArrayInt(
            colorBase,
            2,
            0
        )) & 0xffu;
        const unsigned int blue = (unsigned int)(ZrdArrayInt(
            colorBase,
            3,
            0
        )) & 0xffu;
        flashColor = red | (green << 8) | (blue << 16);
    }

    if (flashRate == 0.0f) {
        return;
    }

    for (HudUiPanel **it = panels.begin; it != panels.end; ++it) {
        ((HudUiTransitionTextPanel *)(*it))->SetFlashColorAndRate(
            flashColor,
            flashRate
        );
    }
}

void LoadHudZrdBitmap(
    zReader::Node *parentNode,
    const char *sectionName,
    zVidImagePartial **outImage
) {
    zReader::Node *const bitmapNode = zReader_GetNamedNode(
        parentNode,
        sectionName
    );
    zReader::Node *const bitmapBase = ZrdArrayBase(bitmapNode);
    const char *const path = ZrdArrayString(
        bitmapBase,
        1
    );
    if (path != 0) {
        *outImage = zImage::TexDir_FindOrCreateByPath(path);
    }
}

void LoadHudZrdSound(
    zReader::Node *parentNode,
    zSndSample **outSound,
    float *outScale
) {
    zReader::Node *const soundNode = zReader_GetNamedNode(
        parentNode,
        "SOUND"
    );
    zReader::Node *const soundBase = ZrdArrayBase(soundNode);
    const char *const name = ZrdArrayString(
        soundBase,
        1
    );
    if (name == 0) {
        return;
    }

    *outScale = ZrdArrayCount(soundBase) >= 3 ? ZrdArrayFloat(
        soundBase,
        2,
        1.0f
    ) : 1.0f;
    *outSound = zSnd::FindSampleByName(name);
}

void ConfigureTextStackLine(
    HudUiTextStack4 *stack,
    HudUiPanel *panel,
    int y,
    int fontSize,
    int fontWeight,
    int fontWidth
) {
    HudUiElement *const element = (HudUiElement *)(panel);
    stack->AddChild(element);
    panel->SetFont(
        "Arial",
        fontSize,
        fontWeight,
        fontWidth,
        0,
        0,
        2
    );
    panel->SetShadow(
        1,
        -1,
        -1
    );
    panel->alignMode = 1;
    element->SetPos(
        0x140,
        y
    );
    element->SetVisible(0);
}

void DestroyTextStackLines(
    HudUiTextStack4 *stack
) {
    {
        for (int index = 0; index < 4; ++index) {
            TextStackLineAt(
                stack,
                index
            )->Destructor();
        }
    }

    stack->HudUiContainer::DestructorCore();
}
} // namespace

namespace HudUiMgrSensor {
// Reimplements 0x41ebd0: HudUiMgrSensor::TrackList_Reset
// (D:\Proj\Battlesport\HudUiMgrSensor.cpp)
void TrackList_Reset() {
    memset(
        &g_HudUiMgrSensor_TrackList,
        0,
        sizeof(g_HudUiMgrSensor_TrackList)
    );
}

// Reimplements 0x438920: HudUiMgrSensor::TrackList_Add
// (D:\Proj\Battlesport\HudUiMgrSensor.cpp)
HudUiMgrSensorTrackNode *__fastcall TrackList_Add(
    int trackKind,
    void *payload
) {
    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(malloc(sizeof(HudUiMgrSensorTrackNode)));
    trackNode->trackKind = 0;
    trackNode->payload = 0;
    trackNode->next = 0;

    if (trackNode != 0) {
        trackNode->next = 0;
        if (g_HudUiMgrSensor_TrackList.count == 0) {
            g_HudUiMgrSensor_TrackList.head = trackNode;
        } else {
            g_HudUiMgrSensor_TrackList.tail->next = trackNode;
        }

        g_HudUiMgrSensor_TrackList.tail = trackNode;
        trackNode->next = 0;
        ++g_HudUiMgrSensor_TrackList.count;
    }

    trackNode->trackKind = trackKind;
    trackNode->payload = payload;
    return trackNode;
}

// Reimplements 0x412070: HudUiMgrSensor::PlaceTrackCounterWidget
// (D:\Proj\Battlesport\HudUiMgrSensor.cpp)
int __fastcall PlaceTrackCounterWidget(
    HudUiMgrSensorTrackNode *trackNode,
    const zVec3 *worldPoint
) {
    const int targetMarkerCount = g_HudUiMgrSensorTargetMarkerCount;
    int inBounds = 0;
    if (targetMarkerCount >= 32) {
        return 0;
    }

    HudUiSlot *const slot = &g_HudUiMgrWeaponSlots[targetMarkerCount];
    g_HudUiMgrSensorTargetMarkerCount = targetMarkerCount + 1;

    const int screenEdgeCode =
        zMath::ProjectPointAndClampToScreenClip(
            worldPoint,
            (zVec3 *)(&slot->screenX)
        );

    int slotX = (int)(slot->screenX);
    int slotY = (int)(slot->screenY);
    if (zOpt::GetReplicateMode() != 0) {
        slotX = (int)(slot->screenX + slot->screenX);
        slotY = (int)(slot->screenY + slot->screenY);
    }
    slot->SetPos(
        slotX,
        slotY
    );

    switch (screenEdgeCode) {
    case 0:
        inBounds = 1;
        break;

    case 1: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[1]);

        const int halfHeight = counterWidget->image->height / 2;
        int top = slot->GetY() - halfHeight;
        if (top <= g_HudUiMgrHudRect.top + halfHeight) {
            top = g_HudUiMgrHudRect.top;
        } else if (top > g_HudUiMgrSensorBlock.sensorViewportRect.top - halfHeight) {
            top = g_HudUiMgrSensorBlock.sensorViewportRect.top - halfHeight * 2;
        }
        counterWidget->SetPos(
            0,
            top
        );
        break;
    }

    case 2: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[2]);

        const zVidImagePartial *const image = counterWidget->image;
        const int height = image->height;
        int top = slot->GetY() - height;
        if (top <= g_HudUiMgrHudRect.top + height) {
            top = g_HudUiMgrHudRect.top;
        } else if (top > g_HudUiMgrHudRect.bottom - height) {
            top = g_HudUiMgrHudRect.bottom - height * 2;
        }

        const int left = slot->GetX() + 1 - image->width;
        counterWidget->SetPos(
            left,
            top
        );
        break;
    }

    case 4: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[3]);

        const zVidImagePartial *const image = counterWidget->image;
        const int top = slot->GetY() + 1;
        const int left = slot->GetX() - image->width / 2;
        counterWidget->SetPos(
            left,
            top
        );
        break;
    }

    case 8: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[4]);

        int left = slot->GetX();
        int top = slot->GetY();
        if (left < g_HudUiMgrObjectiveWidgetRightX) {
            top = g_HudUiMgrSensorBlock.sensorViewportRect.top;
        }

        const zVidImagePartial *const image = counterWidget->image;
        top -= image->height;
        left -= image->width / 2;
        counterWidget->SetPos(
            left,
            top
        );
        break;
    }
    }

    slot->screenEdgeCode = screenEdgeCode;
    slot->trackNode = trackNode;
    return inBounds;
}

// Reimplements 0x4122c0: HudUiMgrSensor::PlaceTrackMarker
// (D:\Proj\Battlesport\HudUiMgrSensor.cpp)
int __fastcall PlaceTrackMarker(
    int markerMode,
    PlayerProgressTargetSlotRuntime *outputSlots
) {
    const int HUD_SENSOR_MARKER_MODE_NEAREST = 1;
    const int HUD_SENSOR_MARKER_MODE_ALL = 2;

    HudUiSlot *const endSlot = &g_HudUiMgrWeaponSlots[g_HudUiMgrSensorTargetMarkerCount];
    HudUiSlot *slot = &g_HudUiMgrWeaponSlots[0];
    PlayerProgressTargetSlotRuntime *const firstOutputSlot = outputSlots;
    int result = 0;
    int nearestDistSq = 0x98967f;
    g_HudUiMgrSensorTrackedProgressSlot = 0;

    while (slot < endSlot) {
        if (slot->screenEdgeCode == 0) {
            if (markerMode == HUD_SENSOR_MARKER_MODE_ALL) {
                HudUiMgrSensorTrackNode *const trackNode =
                    (HudUiMgrSensorTrackNode *)(slot->trackNode);
                if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
                    zUtil_SaveGameState *const saveState =
                        (zUtil_SaveGameState *)(trackNode->payload);
                    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
                    outputSlots->targetPos = &playerState->fxOffsetWorld;
                    outputSlots->targetVelocity = &playerState->projectileSpawnVel;
                    ++outputSlots;
                    ++result;
                } else if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
                    zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(trackNode->payload);
                    outputSlots->targetPos = &turretRuntime->firePos;
                    outputSlots->targetVelocity = 0;
                    ++outputSlots;
                    ++result;
                }
            }

            const int dx = slot->GetX() - g_HudUiMgrReticleProjectedX;
            const int dy = slot->GetY() - g_HudUiMgrReticleProjectedY;
            const int distSq = dx * dx + dy * dy;
            if (distSq < nearestDistSq) {
                g_HudUiMgrSensorTrackedProgressSlot = slot;
                nearestDistSq = distSq;
            }
        }

        ++slot;
    }

    outputSlots = firstOutputSlot;
    if (markerMode != HUD_SENSOR_MARKER_MODE_NEAREST ||
        nearestDistSq >= g_HudUiMgrReticleSnapRadiusSq ||
        g_HudUiMgrSensorTrackedProgressSlot == 0) {
        return result;
    }

    HudUiSlot *const trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    trackedProgressSlot->trackMarkerWidget.SetImageBorrowedAndInvalidate(
        g_HudUiMgrSensorTargetMarkerImages[0]
    );

    const zVidImagePartial *const image = trackedProgressSlot->trackMarkerWidget.image;
    const int markerY = ((HudUiElement *)(trackedProgressSlot))->GetY() - image->height / 2;
    const int markerX = ((HudUiElement *)(trackedProgressSlot))->GetX() - image->width / 2;
    trackedProgressSlot->trackMarkerWidget.SetPos(
        markerX,
        markerY
    );
    trackedProgressSlot->trackMarkerWidget.SetVisible(1);

    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
    if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(trackNode->payload);
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        outputSlots->targetPos = &playerState->fxOffsetWorld;
        outputSlots->targetVelocity = &playerState->projectileSpawnVel;
        return 1;
    }

    if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
        zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(trackNode->payload);
        outputSlots->targetVelocity = 0;
        outputSlots->targetPos = &turretRuntime->firePos;
    }

    return 1;
}

// Reimplements 0x439690: HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag
// (D:\Proj\Battlesport\HudUiMgrSensor.cpp)
void __fastcall UpdateMarkersAndProgressFromVariantTag(
    const zTag4Partial *requiredVariantTag
) {
    HudUiMgrSensorTrackNode *trackNode = g_HudUiMgrSensor_TrackList.head;
    zUtil_PlayerStateStorage *const localPlayerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);

    HudUiMgrSensorTrackNode *candidateTrackNodes[0x64];
    int candidateCount = 0;
    while (trackNode != 0) {
        if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
            zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(trackNode->payload);
            zUtil_PlayerStateStorage *const playerState = saveState->playerState;

            if (playerState->recentHitFlag != 0 &&
                !(g_Time_AccumulatedTimeSec < playerState->recentHitExpireTime)) {
                playerState->recentHitFlag = 0;
            }

            if (playerState->lifecycleState != 1 && playerState->lifecycleState != 4 &&
                VariantTag::TagsOverlap(
                    &playerState->variantTag,
                    requiredVariantTag
                ) != 0) {
                const float distXZ =
                    fabs(playerState->fxOffsetWorld.x - localPlayerState->worldPos.x) +
                    fabs(playerState->fxOffsetWorld.z - localPlayerState->worldPos.z);
                if (distXZ < 650.0f && candidateCount < 0x63) {
                    candidateTrackNodes[candidateCount++] = trackNode;
                }
            }
        } else {
            trackNode->trackKind = HUD_SENSOR_TRACK_KIND_TURRET;
            zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(trackNode->payload);
            if (turretRuntime->HasActiveNode() != 0 &&
                VariantTag::CurrentAllowsId(turretRuntime->turretNode->nodeType) != 0) {
                const float distXZ = fabs(turretRuntime->firePos.z - localPlayerState->worldPos.z) +
                                     fabs(turretRuntime->firePos.x - localPlayerState->worldPos.x);
                if (distXZ < 650.0f && candidateCount < 0x63) {
                    candidateTrackNodes[candidateCount++] = trackNode;
                }
            }
        }

        trackNode = trackNode->next;
    }

    if (candidateCount != 0) {
        int selectedIndex = g_HudUiMgrSensorRoundRobinTrackIndex + 1;
        g_HudUiMgrSensorRoundRobinTrackIndex = selectedIndex;
        if (selectedIndex >= candidateCount) {
            selectedIndex = 0;
            g_HudUiMgrSensorRoundRobinTrackIndex = 0;
        }

        HudUiMgrSensorTrackNode *const selectedTrackNode = candidateTrackNodes[selectedIndex];
        if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
            zUtil_SaveGameState *const saveState =
                (zUtil_SaveGameState *)(selectedTrackNode->payload);
            zUtil_PlayerStateStorage *const playerState = saveState->playerState;
            zVec3 point = playerState->fxOffsetWorld;
            point.y += 3.0f;

            const int visible =
                Player::TestScenePathBetweenCameraTargetAndPoint(
                    playerState->rootNode,
                    &point,
                    1
                );
            playerState->spawnStateInitialized = visible;
            if (visible != 0 && playerState->recentHitMarkerHandle != 0) {
                playerState->recentHitFlag = 1;
                playerState->recentHitExpireTime = g_Time_AccumulatedTimeSec + 3.0f;
            }
        } else if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
            zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(selectedTrackNode->payload);
            turretRuntime->scenePathVisible = Player::TestScenePathBetweenCameraTargetAndPoint(
                turretRuntime->turretNode,
                &turretRuntime->firePos,
                2
            );
        }

        {
            for (int index = 0; index < candidateCount; ++index) {
                HudUiMgrSensorTrackNode *const candidate = candidateTrackNodes[index];
                if (candidate->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
                    zUtil_SaveGameState *const saveState =
                        (zUtil_SaveGameState *)(candidate->payload);
                    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
                    if ((playerState->spawnStateInitialized & 1) != 0) {
                        playerState->recentHitMarkerHandle =
                            HudUiMgrSensor::PlaceTrackCounterWidget(
                                candidate,
                                &playerState->fxOffsetWorld
                            );
                    }
                } else if (candidate->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
                    zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(candidate->payload);
                    if ((turretRuntime->scenePathVisible & 1) != 0) {
                        HudUiMgrSensor::PlaceTrackCounterWidget(
                            candidate,
                            &turretRuntime->firePos
                        );
                    }
                }
            }
        }
    }

    PlayerGunFireController *const activeAltGunController =
        localPlayerState->activeAltGunController;
    const unsigned int optEntryFlags = activeAltGunController->optCatalogEntry->flags;
    if (((optEntryFlags >> 20) & 1u) != 0) {
        HudUiMgr::CopyReticleProjection(&localPlayerState->autoTurnTargetWorldPos.x);
        localPlayerState->progressTargetCount = 1;
        localPlayerState->progressTargetSlots[0].targetPos =
            &localPlayerState->autoTurnTargetWorldPos;
        localPlayerState->progressTargetSlots[0].targetVelocity = 0;
        HudUiMgrTarget::UpdateSelectedProgressMeter(0);
        return;
    }

    if (activeAltGunController->ammoOrCharge != 0.0f) {
        int markerMode = 0;
        if (((optEntryFlags >> 16) & 1u) != 0) {
            markerMode = 2;
        } else if ((optEntryFlags & 0x4000u) != 0) {
            markerMode = 1;
        }

        localPlayerState->progressTargetCount =
            HudUiMgrSensor::PlaceTrackMarker(
                markerMode,
                localPlayerState->progressTargetSlots
            );
    }

    HudUiMgrTarget::UpdateSelectedProgressMeter(0);
}

// Reimplements 0x411f10: HudUiMgrSensor::SetShieldMessageRatio
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall SetShieldMessageRatio(
    float ratio
) {
    if (ratio > 1.0f) {
        ratio = 1.0f;
    } else if (ratio < 0.0f) {
        ratio = 0.0f;
    }

    HudUiShieldMessageWidget *const shieldMessageWidget = g_HudUiMgrShieldMessageWidget;
    HudUiMeter *const meter = &shieldMessageWidget->meter;
    const unsigned char green = ratio < 0.25f ? 0 : 255;
    meter->color565 = zVid_PackColorRGB(
        255,
        green,
        0
    ) & 0xffffu;

    const int fillPixels = (int)(ceil((double)(meter->fillPixelsMax) * (double)(ratio)));
    const int top = (int)(meter->points[1].y) - fillPixels;
    meter->points[0].y = (float)(top);
    meter->points[3].y = (float)(top);
    meter->Invalidate();

    HudUiPanel *const percentTextPanel = (HudUiPanel *)(&shieldMessageWidget->percentTextPanel);
    const int percent = (int)(ceil((double)(ratio) * 100.0));
    percentTextPanel->SetTextFmt(
        "%d",
        percent
    );
    percentTextPanel->Invalidate();
}

// Reimplements 0x410d10: HudUiMgrSensor::SetViewportRect
void __fastcall SetViewportRect(
    int x,
    int y,
    int width,
    int height
) {
    const int right = x + width;
    const int bottom = y + height;

    g_HudUiMgrSensorBlock.sensorRectRaw.left = x;
    g_HudUiMgrSensorBlock.sensorRectRaw.right = right;
    g_HudUiMgrSensorBlock.sensorRectRaw.top = y;
    g_HudUiMgrSensorBlock.sensorRectRaw.bottom = bottom;

    if (zOpt::GetReplicateMode() == 0) {
        g_HudUiMgrSensorBlock.sensorRectScaled = g_HudUiMgrSensorBlock.sensorRectRaw;
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.left = (float)(x);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.top = (float)(y);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.right = (float)(right);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom = (float)(bottom);
    } else {
        const int halfX = x / 2;
        const int halfY = y / 2;
        const int halfWidth = width / 2;
        const int halfHeight = height / 2;

        g_HudUiMgrSensorBlock.sensorPiVSrcRect.left = (float)(halfX);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.top = (float)(halfY);
        g_HudUiMgrSensorBlock.sensorRectScaled.left = halfX;
        g_HudUiMgrSensorBlock.sensorRectScaled.top = halfY;
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.right =
            (float)(halfWidth) + g_HudUiMgrSensorBlock.sensorPiVSrcRect.left;
        g_HudUiMgrSensorBlock.sensorRectScaled.right = halfX + halfWidth;
        g_HudUiMgrSensorBlock.sensorRectScaled.bottom = halfY + halfHeight;
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom =
            (float)(halfHeight) + g_HudUiMgrSensorBlock.sensorPiVSrcRect.top;
    }

    g_HudUiMgrSensorBlock.sensorClampHalfW = (g_HudUiMgrSensorBlock.sensorPiVSrcRect.right -
                                                 g_HudUiMgrSensorBlock.sensorPiVSrcRect.left) /
                                             g_HudUiMgrSensorBlock.sensorParam;
    g_HudUiMgrSensorBlock.sensorClampHalfH = (g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom -
                                                 g_HudUiMgrSensorBlock.sensorPiVSrcRect.top) /
                                             g_HudUiMgrSensorBlock.sensorParam;
    zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);
}

// Reimplements 0x414300: HudUiMgrSensor::GetFxRect (D:\Proj\Battlesport\hud.cpp)
void __fastcall GetFxRect(
    HudUiRect *outRect
) {
    *outRect = g_HudUiMgrSensorFxRect;
}
} // namespace HudUiMgrSensor

namespace HudUiMgrTarget {
// Reimplements 0x4124b0: HudUiMgrTarget::UpdateSelectedProgressMeter
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall UpdateSelectedProgressMeter(
    int clearSelectedTrack
) {
    HudUiSlot *trackedProgressSlot = 0;
    if (clearSelectedTrack != 0) {
        g_HudUiMgrSensorTrackedProgressSlot = 0;
    } else {
        trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    }

    if (g_HudUiMgr.enabled == 0 || g_HudUiMgrObjectivePhase != 0 || trackedProgressSlot == 0) {
        return;
    }

    HudUiMgrSensorTrackNode *const selectedTrackNode =
        (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
    float selectedHealthCurrent = 0.0f;
    float selectedHealthMax = 1.0f;
    if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(selectedTrackNode->payload);
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        selectedHealthCurrent = playerState->statusMeterValue;
        selectedHealthMax = playerState->masterCommonData->maxHealth;
    } else if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
        zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(selectedTrackNode->payload);
        selectedHealthCurrent = turretRuntime->healthCurrent;
        selectedHealthMax = turretRuntime->healthMax;
    }

    if (selectedHealthCurrent == 0.0f) {
        g_HudUiMgrSensorMeter.SetVisible(0);
        trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    }

    if (zClipAlt::RemapPointXYInPlace(&trackedProgressSlot->screenX) == 0) {
        return;
    }

    if (zOpt::GetReplicateMode() != 0) {
        g_HudUiMgrSensorTrackedProgressSlot->screenX +=
            g_HudUiMgrSensorTrackedProgressSlot->screenX;
        g_HudUiMgrSensorTrackedProgressSlot->screenY +=
            g_HudUiMgrSensorTrackedProgressSlot->screenY;
    }

    float healthRatio = selectedHealthCurrent / selectedHealthMax;
    if (healthRatio > 1.0f) {
        healthRatio = 1.0f;
    } else if (healthRatio < 0.0f) {
        healthRatio = 0.0f;
    }

    const int fillPixels =
        (int)(ceil((double)(g_HudUiMgrSensorMeter.fillPixelsMax) * (double)(healthRatio)));
    const int top = (int)(g_HudUiMgrSensorMeter.points[1].y) - fillPixels;
    g_HudUiMgrSensorMeter.points[0].y = (float)(top);
    g_HudUiMgrSensorMeter.points[3].y = (float)(top);
    g_HudUiMgrSensorMeter.Invalidate();
    g_HudUiMgrSensorMeter.SetVisible(1);
}
} // namespace HudUiMgrTarget

namespace HudUiMgrObjective {
// Reimplements 0x412050: HudUiMgrObjective::RefreshCounterText
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall RefreshCounterText(
    int counterValue
) {
    HudUiPanel *const panel = (HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel);
    panel->SetTextFmt(
        "%d",
        counterValue
    );
    panel->UpdateTextBoundsFromContent();
}

// Reimplements 0x411760: HudUiMgrObjective::SetVisibleAndResetMeterFill
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall SetVisibleAndResetMeterFill(
    int visible
) {
    if (visible == 0) {
        g_HudUiMgrObjectiveLabelTextPanel->SetVisible(0);
        g_HudUiMgrObjectiveMeter.SetVisible(0);
        return;
    }

    g_HudUiMgrObjectiveLabelTextPanel->SetVisible(1);
    g_HudUiMgrObjectiveMeter.SetVisible(1);

    const int meterTop = (int)(g_HudUiMgrObjectiveMeter.points[1].y);
    g_HudUiMgrObjectiveMeterFillAnimTimerSec = 0.0f;
    g_HudUiMgrObjectiveMeterFillAnimEnabled = 1;
    g_HudUiMgrObjectiveMeter.points[0].y = (float)(meterTop);
    g_HudUiMgrObjectiveMeter.points[3].y = (float)(meterTop);
}

// Reimplements 0x4118b0: HudUiMgrObjective::UpdateMeterXPoints
// (D:\Proj\Battlesport\hud.cpp)
void UpdateMeterXPoints() {
    const float left = (float)(g_HudUiMgrObjectiveWidget.GetCenterX()) + 5.0f;
    const float right = left + 7.0f;
    g_HudUiMgrObjectiveMeter.points[0].x = left;
    g_HudUiMgrObjectiveMeter.points[1].x = left;
    g_HudUiMgrObjectiveMeter.points[2].x = right;
    g_HudUiMgrObjectiveMeter.points[3].x = right;
}

// Reimplements 0x4117f0: HudUiMgrObjective::TickMeterFillAnimation
// (D:\Proj\Battlesport\hud.cpp)
void TickMeterFillAnimation() {
    g_HudUiMgrObjectiveMeterFillAnimTimerSec += g_Time_UnscaledDeltaTimeSec;

    int fillPixels;
    if (g_HudUiMgrObjectiveMeterFillAnimTimerSec >= 3.0f) {
        fillPixels = (int)(ceil((double)(g_HudUiMgrObjectiveMeter.fillPixelsMax)));
        g_HudUiMgrObjectiveMeterFillAnimEnabled = 0;
    } else {
        const double fillRatio = (double)(g_HudUiMgrObjectiveMeterFillAnimTimerSec * 0.333332986f) *
                                 (double)(g_HudUiMgrObjectiveMeter.fillPixelsMax);
        fillPixels = (int)(ceil(fillRatio));
    }

    const int top = (int)(g_HudUiMgrObjectiveMeter.points[1].y) - fillPixels;
    g_HudUiMgrObjectiveMeter.points[0].y = (float)(top);
    g_HudUiMgrObjectiveMeter.points[3].y = (float)(top);
}

static void HudUiMgrObjective_UpdateWidgetRightX() {
    const zVidImagePartial *const image = g_HudUiMgrObjectiveWidget.image;
    const int width = image != 0 ? image->width : 0;
    g_HudUiMgrObjectiveWidgetRightX = g_HudUiMgrObjectiveWidget.GetCenterX() + width;
}

static void HudUiMgrObjective_SetSlidePosition(
    float slideX
) {
    g_HudUiMgrObjectiveBar.points[2].x = slideX;
    g_HudUiMgrObjectiveBar.points[3].x = slideX;
    g_HudUiMgrObjectiveBar.Invalidate();
    ((HudUiElement *)(&g_HudUiMgrObjectiveWidget))->SetX((int)(slideX)-1);
    HudUiMgrObjective::UpdateMeterXPoints();
}

static void HudUiMgrObjective_UpdateHwDirtyRectIfNeeded() {
    if (zOpt::GetHudTypeForCurrentHwMode() == 2) {
        g_HudLayoutHW.UpdateObjectiveDirtyRect();
    }
}

static void HudUiMgrObjective_DrawSensorNoise(
    float fade,
    int visibleWhenCovered
) {
    if (g_HudUiMgrObjectiveSensorRect.image == 0) {
        return;
    }

    float noise = fade + fade;
    if (noise < 1.0f) {
        zVid::DrawNoiseRect(
            (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
            (double)(noise)
        );
        return;
    }

    g_HudUiMgrObjectiveSensorRect.SetVisible(visibleWhenCovered);
    zVid::DrawNoiseRect(
        (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
        (double)(2.0f - noise)
    );
}

// Reimplements 0x411900: HudUiMgrObjective::Show
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall Show(
    zVidImagePartial *objectiveImage,
    const char *summaryFormat,
    const char *descText,
    float autoHideDelay
) {
    if (summaryFormat == 0 || descText == 0 || g_HudUiMgrObjectiveChatComposeActive != 0) {
        return 0;
    }

    g_HudUiMgrObjectiveSummaryTextPanel->SetTextFmt(summaryFormat);
    g_HudUiMgrObjectiveDescTextPanel->SetTextFmt(descText);
    g_HudUiMgrSensorOverlay.SetVisible(0);

    const int phase = g_HudUiMgrObjectivePhase;
    if (phase == 0) {
        g_HudUiMgrObjectiveSensorRect.SetImageBorrowedAndInvalidate(objectiveImage);
        zVidImagePartial *const widgetImage = g_HudUiMgrObjectiveWidget.image;
        g_HudUiMgrObjectiveState = 1;
        g_HudUiMgrObjectivePhase = 1;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
        g_HudUiMgrObjectiveShowResetUnused = 0;
        g_HudUiMgrObjectiveAutoHideDelaySec = autoHideDelay;

        const int imageWidth = widgetImage != 0 ? widgetImage->width : 0;
        g_HudUiMgrObjectiveWidgetRightX = g_HudUiMgrObjectiveWidget.GetCenterX() + imageWidth;
        g_HudUiMgrObjectiveBar.SetVisible(1);
        gAltClipPassEnabled = 0;
        return 1;
    }

    if (phase == 3) {
        g_HudUiMgrObjectivePhase = 1;
        g_HudUiMgrObjectivePhaseTimerSec =
            g_HudUiMgrObjectivePhaseDurationSec - g_HudUiMgrObjectivePhaseTimerSec;
        return 1;
    }

    g_HudUiMgrObjectiveSensorRect.SetImageBorrowedAndInvalidate(objectiveImage);
    return 0;
}

// Reimplements 0x411a20: HudUiMgrObjective::Begin
void Begin() {
    if (g_HudUiMgrObjectiveChatComposeActive != 0) {
        return;
    }

    const int phase = g_HudUiMgrObjectivePhase;
    if (phase == 2) {
        g_HudUiMgrObjectiveState = 1;
        g_HudUiMgrObjectivePhase = 3;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;

        if (g_HudUiMgrObjectiveDescTextPanel != 0) {
            ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))->SetVisible(0);
        }

        if (g_HudUiMgrObjectiveLabelTextPanel != 0) {
            ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->SetVisible(0);
        }

        g_HudUiMgrObjectiveSensorRect.SetVisible(0);
        g_HudUiMgrObjectiveAutoHideDelaySec = 0.0f;
        return;
    }

    if (phase == 1) {
        g_HudUiMgrObjectivePhase = 3;
        g_HudUiMgrObjectivePhaseTimerSec =
            g_HudUiMgrObjectivePhaseDurationSec - g_HudUiMgrObjectivePhaseTimerSec;
        g_HudUiMgrObjectiveAutoHideDelaySec = 0.0f;
    }
}

// Reimplements 0x411ac0: HudUiMgrObjective::StartHide
// (D:\Proj\Battlesport\hud.cpp)
void StartHide() {
    g_HudUiMgrObjectivePhaseTimerSec += g_Time_UnscaledDeltaTimeSec;

    if (g_HudUiMgrObjectivePhase == 1) {
        if (g_HudUiMgrObjectivePhaseTimerSec < g_HudUiMgrObjectivePhaseDurationSec) {
            const float fade =
                g_HudUiMgrObjectivePhaseTimerSec / g_HudUiMgrObjectivePhaseDurationSec;
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x +
                fade * g_HudUiMgrObjectiveBar.slideRangeX;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateWidgetRightX();
            HudUiMgrObjective_DrawSensorNoise(
                fade,
                1
            );
        } else {
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x + g_HudUiMgrObjectiveBar.slideRangeX;
            g_HudUiMgrObjectivePhase = 2;
            g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateWidgetRightX();
            g_HudUiMgrObjectiveSummaryTextPanel->SetVisible(1);
            g_HudUiMgrObjectiveDescTextPanel->SetVisible(1);
            g_HudUiMgrObjectiveSensorRect.SetVisible(1);
        }
    } else if (g_HudUiMgrObjectivePhase == 2) {
        ((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel))->Invalidate();
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))->Invalidate();
        g_HudUiMgrObjectiveBar.Invalidate();
        ((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect))->Invalidate();
    } else if (g_HudUiMgrObjectivePhase == 3) {
        if (g_HudUiMgrObjectivePhaseTimerSec < g_HudUiMgrObjectivePhaseDurationSec) {
            const float fade =
                1.0f - g_HudUiMgrObjectivePhaseTimerSec / g_HudUiMgrObjectivePhaseDurationSec;
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x +
                fade * g_HudUiMgrObjectiveBar.slideRangeX;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateHwDirtyRectIfNeeded();
            HudUiMgrObjective_UpdateWidgetRightX();
            HudUiMgrObjective_DrawSensorNoise(
                fade,
                0
            );
        } else {
            g_HudUiMgrObjectiveState = 0;
            g_HudUiMgrObjectivePhase = 0;
            g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
            ((HudUiElement *)(&g_HudUiMgrObjectiveWidget))
                ->SetX((int)(g_HudUiMgrObjectiveBar.points[1].x));
            HudUiMgrObjective::UpdateMeterXPoints();
            HudUiMgrObjective_UpdateHwDirtyRectIfNeeded();
            HudUiMgrObjective_UpdateWidgetRightX();
            g_HudUiMgrObjectiveBar.SetVisible(0);
            g_HudUiMgrSensorOverlay.SetVisible(1);
            gAltClipPassEnabled = 1;
        }
    }

    if (g_HudUiMgrObjectiveAutoHideDelaySec != 0.0f) {
        if (g_HudUiMgrObjectivePhaseTimerSec >= g_HudUiMgrObjectiveAutoHideDelaySec) {
            HudUiMgrObjective::Begin();
        }

        g_HudUiMgrObjectiveState = 1;
    }
}

// Reimplements 0x411eb0: HudUiMgrObjective::Update
void Update() {
    g_HudUiMgrObjectiveWidget.SetVisible(1);
    if (g_HudUiMgrObjectivePhase == 0) {
        return;
    }

    g_HudUiMgrObjectiveBar.SetVisible(1);
    if (g_HudUiMgrObjectivePhase != 2) {
        return;
    }

    if (g_HudUiMgrObjectiveDescTextPanel != 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))->SetVisible(1);
    }

    if (g_HudUiMgrObjectiveLabelTextPanel != 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->SetVisible(1);
    }

    g_HudUiMgrObjectiveSensorRect.SetVisible(1);
}
} // namespace HudUiMgrObjective

namespace HudUiLoadingCheckpoint {
// Reimplements 0x414180: HudUiLoadingCheckpoint::AdvanceAndLog
void __fastcall AdvanceAndLog(
    const char *messageOrNull
) {
    const unsigned int currentIndex = g_HudUiLoadingCheckpointCurrentIndex;
    const unsigned int maxIndex = g_HudUiLoadingCheckpointMaxIndex;
    if (currentIndex > maxIndex) {
        zError::ReportOld(
            0x800,
            "D:\\Proj\\Battlesport\\hud.cpp",
            0x1184,
            "Checkpoint overflow"
        );
    } else {
        g_HudUiLoadingCheckpointCurrentProgress = g_HudUiLoadingCheckpointProgress[currentIndex];
        const unsigned int nextIndex = currentIndex + 1;
        g_HudUiLoadingCheckpointCurrentIndex = nextIndex;
        if (nextIndex > maxIndex) {
            g_HudUiLoadingCheckpointCurrentIndex = maxIndex;
        }
    }

    if (messageOrNull != 0) {
        puts(messageOrNull);
        fflush(stdout);
    }

    zGame::ReturnOnlyStub();
    Briefing::SetProgressAndSleep(g_HudUiLoadingCheckpointCurrentProgress);
}

// Reimplements 0x414210: HudUiLoadingCheckpoint::InitTable
void InitTable() {
    static const float kRawProgress[] = {
        0.00100000005f,
        0.136999995f,
        0.237000003f,
        0.340000004f,
        0.899999976f,
        9.30000019f,
        12.3999996f,
        13.3999996f,
        20.0f,
        26.0f,
        26.2999992f,
        28.7000008f,
        31.5f,
        34.0f,
        36.2000008f,
        36.4000015f,
        53.2999992f,
        53.5999985f,
        53.7000008f,
    };

    g_HudUiLoadingCheckpointMaxIndex = 0x12;
    g_HudUiLoadingCheckpointCurrentIndex = 0;
    {
        for (unsigned int index = 0; index <= g_HudUiLoadingCheckpointMaxIndex; ++index) {
            g_HudUiLoadingCheckpointRawProgress[index] = kRawProgress[index];
            g_HudUiLoadingCheckpointProgress[index] =
                g_HudUiLoadingCheckpointRawProgress[index] * g_HudUiLoadingCheckpointProgressScale;
        }
    }
}
} // namespace HudUiLoadingCheckpoint

namespace HudUiAuxOverlay {
// Reimplements 0x4137f0: HudUiAuxOverlay::UpdateTextLine
void __fastcall UpdateTextLine(
    int op,
    int index,
    const char *format
) {
    HudUiPanel *const panel = (HudUiPanel *)(&g_HudUiMgrStringMenu->items[index]);

    if (op == 1) {
        panel->SetTextFmt(format);
        panel->SetVisible(1);
        return;
    }

    if (op == 0) {
        panel->SetVisible(0);
        return;
    }

    if (op == 2) {
        if (*format != '\0') {
            panel->SetTextFmt(format);
            panel->SetVisible(1);
        } else {
            panel->SetVisible(0);
        }
    }
}

// Reimplements 0x4137c0: HudUiAuxOverlay::ClearTextLines
void ClearTextLines() {
    {
        for (int index = 0; index < 23; ++index) {
            UpdateTextLine(
                2,
                index,
                ""
            );
            UpdateTextLine(
                0,
                index,
                0
            );
        }
    }
}
} // namespace HudUiAuxOverlay

namespace {
zReader::Node *HudUiZrdPayload(
    zReader::Node *node
) {
    return node != 0 && node->type == zReader::ZRDR_NODE_ARRAY ? node->value.nodes : 0;
}

const char *HudUiZrdStringAt(
    zReader::Node *payload,
    int index
) {
    return payload != 0 ? payload[index].value.str : 0;
}

int HudUiZrdIntAt(
    zReader::Node *payload,
    int index
) {
    return payload != 0 ? payload[index].value.i32 : 0;
}

void HudUiEnsureLoaderWidgetsConstructed() {
    g_HudUiMgrSensorPanel.Constructor(0);
    g_HudUiMgrSensorOverlay.Constructor(0);
    g_HudUiMgrSensorMeter.Constructor();
    g_HudUiMgrObjectiveWidget.Constructor(0);
    g_HudUiMgrObjectiveSensorRect.Constructor(0);
    g_HudUiMgrObjectiveBar.Constructor();
    g_HudUiMgrReticleWidget.Constructor(0);
    ((HudUiTripletPanel *)(&g_HudUiMgrNanitePanel))->Constructor();

    {
        int messageIndex1;
        for (messageIndex1 = 0;
            messageIndex1 < (int)(sizeof(g_HudUiMgrMessages) / sizeof(g_HudUiMgrMessages[0]));
            ++messageIndex1) {
            HudUiMessage &message = g_HudUiMgrMessages[messageIndex1];
            message.Constructor();
        }
    }

    {
        int counterIndex2;
        for (counterIndex2 = 0; counterIndex2 < (int)(sizeof(g_HudUiMgrModeCounters) /
                                                      sizeof(g_HudUiMgrModeCounters[0]));
            ++counterIndex2) {
            HudUiCounter &counter = g_HudUiMgrModeCounters[counterIndex2];
            counter.Constructor();
        }
    }

    {
        int slotIndex3;
        for (slotIndex3 = 0;
            slotIndex3 < (int)(sizeof(g_HudUiMgrWeaponSlots) / sizeof(g_HudUiMgrWeaponSlots[0]));
            ++slotIndex3) {
            HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex3];
            slot.Constructor();
        }
    }
}

void HudUiSetFontFromRect(
    HudUiPanel *panel,
    const HudUiRect &fontSpec
) {
    panel->SetFont(
        (const char *)(fontSpec.left),
        fontSpec.right,
        fontSpec.bottom,
        fontSpec.top,
        0,
        0,
        2
    );
}

void HudUiSetPanelClipWithSource(
    HudUiPanel *panel,
    void *source,
    const HudUiRect *clipRect
) {
    panel->SetClip(
        source,
        clipRect
    );
}

void HudUiApplyStatsTripletInt3(
    zReader::Node *payload,
    int nodeIndex,
    int &outX,
    int &outY,
    int *outZ = 0
) {
    HudUiLayoutNode::ReadInt3(
        &payload[nodeIndex],
        &outX,
        &outY,
        outZ
    );
}
} // namespace

namespace HudUiMgr {
// Reimplements 0x40d7e0: HudUiMgr::Constructor (D:\Proj\Battlesport\hud.cpp)
//
// The retail binary stores these members in one contiguous HudUiMgr object. The
// current source model exposes the same subobjects as recovered globals; this
// constructor preserves the observed construction order and table installs.
HudUiContainer *__fastcall Constructor(
    HudUiContainer *self
) {
    HudUiContainer *const manager = self;
    new (manager) HudUiContainer;

    ((HudUiPanel *)(&g_HudUiMgrHudRootPanel))->ConstructorDefault(
        0,
        0,
        0
    );
    g_HudUiMgrHudRootPanel.flashResetValue = 0.349999994f;
    g_HudUiMgrHudRootPanel.flashCountdown = 0.0f;
    g_HudUiMgrHudRootPanel.flashAltColor0 = 0;
    g_HudUiMgrHudRootPanel.flashEnabled = 0;
    g_HudUiMgrHudRootPanel.flashMode = 0;
    g_HudUiMgrHudRootPanel.flashDirectionSign = 1;

    g_HudUiMgrReticleWidget.Constructor(0);
    ((HudUiTripletPanel *)(&g_HudUiMgrNanitePanel))->Constructor();

    g_HudUiMgrObjectiveWidget.Constructor(0);
    g_HudUiMgrObjectiveSensorRect.Constructor(0);
    g_HudUiMgrObjectiveMeter.ConstructorEx();
    g_HudUiMgrObjectiveBar.Constructor();
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(256);

    g_HudUiMgrSensorPanel.Constructor(0);
    g_HudUiMgrSensorOverlay.Constructor(0);
    g_HudUiMgrSensorMeter.ConstructorEx();

    {
        int index;
        for (index = 0; index < 32; ++index) {
            g_HudUiMgrWeaponSlots[index].Constructor();
        }
    }

    {
        int index;
        for (index = 0; index < 4; ++index) {
            g_HudUiMgrModeCounters[index].Constructor();
        }
    }

    {
        int index;
        for (index = 0; index < 10; ++index) {
            g_HudUiMgrMessages[index].Constructor();
        }
    }

    g_HudUiMgrTailBar.Constructor();
    g_HudUiMgrTailBar.quadHeight = 0;
    g_HudUiMgrTailBar.quadLeftX = 0.0f;
    return manager;
}

// Reimplements 0x40d400: HudUiMgr::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\hud.cpp)
void StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x40d410: HudUiMgr::StaticInit (D:\Proj\Battlesport\hud.cpp)
HudUiContainer *StaticInit() {
    return Constructor(&g_HudUiMgr);
}

// Reimplements 0x40d420: HudUiMgr::RegisterAtExit (D:\Proj\Battlesport\hud.cpp)
void RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x40d430: HudUiMgr::AtExitDestructor (D:\Proj\Battlesport\hud.cpp)
void AtExitDestructor() {
    StaticDestructor(&g_HudUiMgr);
}

// Reimplements 0x40d440: HudUiMgr::StaticDestructor (D:\Proj\Battlesport\hud.cpp)
//
// The original object is contiguous; the current source model keeps the same
// embedded HUD manager subobjects as recovered globals. Keep the destruction
// order aligned with the retail static destructor.
void __fastcall StaticDestructor(
    HudUiContainer *self
) {

    {
        int index;
        for (index = 10; index > 0; --index) {
            g_HudUiMgrMessages[index - 1].Destructor();
        }
    }

    {
        int index;
        for (index = 4; index > 0; --index) {
            g_HudUiMgrModeCounters[index - 1].DestructorCore();
        }
    }

    {
        int index;
        for (index = 32; index > 0; --index) {
            g_HudUiMgrWeaponSlots[index - 1].Destructor();
        }
    }

    g_HudUiMgrSensorBlock.Destructor();

    g_HudUiMgrObjectiveChatComposeTextInput.DestructorCore();
    g_HudUiMgrObjectiveSensorRect.DestructorCore();
    g_HudUiMgrObjectiveWidget.DestructorCore();

    ((HudUiTripletPanel *)(&g_HudUiMgrNanitePanel))->DestructorCore();
    g_HudUiMgrReticleWidget.DestructorCore();
    ((HudUiPanel *)(&g_HudUiMgrHudRootPanel))->Destructor();
    self->DestructorCore();
}

// Reimplements 0x411170: HudUiMgr::ProjectPointToNormalizedClamped
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall ProjectPointToNormalizedClamped(
    const zVec3 *srcPoint,
    zVec3 *projectedPoint
) {
    if (zMath::ProjectPointAndClampToScreenClip(
        srcPoint,
        projectedPoint
    ) == 0x10) {
        return 1;
    }

    const float halfHudWidth = g_HudUiMgrHudRectW * 0.5f;
    const float halfHudHeight = g_HudUiMgrHudRectH * 0.5f;
    if (zOpt::GetReplicateMode() != 0) {
        projectedPoint->x += projectedPoint->x;
        projectedPoint->y += projectedPoint->y;
    }

    projectedPoint->x = (projectedPoint->x - halfHudWidth) / halfHudWidth;
    projectedPoint->y =
        (projectedPoint->y - (float)(g_HudUiMgrHudRect.top) - halfHudHeight) / halfHudHeight;

    if (projectedPoint->x > 1.0f) {
        projectedPoint->x = 1.0f;
    } else if (projectedPoint->x < -1.0f) {
        projectedPoint->x = -1.0f;
    }

    if (projectedPoint->y > 1.0f) {
        projectedPoint->y = 1.0f;
    } else if (projectedPoint->y < -1.0f) {
        projectedPoint->y = -1.0f;
    }

    return 0;
}

// Reimplements 0x413630: HudUiMgr::TriggerCurrentLayoutOnActivated
void TriggerCurrentLayoutOnActivated() {
    if (g_HudUiMgrCurrentLayout != 0) {
        g_HudUiMgrCurrentLayout->OnActivated();
    }
}

// Reimplements 0x410140: HudUiMgr::TickLayoutDelay
int TickLayoutDelay() {
    if (g_HudUiMgrLayoutDelayFrames == 0) {
        return 0;
    }

    --g_HudUiMgrLayoutDelayFrames;
    return 1;
}

// Reimplements 0x4143a0: HudUiMgr::IsLocalPlayerFirstInStatsList
// (D:\Proj\Battlesport\HudUi.cpp)
int IsLocalPlayerFirstInStatsList() {
    return g_HudUiMgrStatsList->triplet->IsLocalPlayerFirstEntry();
}

// Reimplements 0x410160: HudUiMgr::EnsureHudLoaded
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall EnsureHudLoaded(
    const char *entryPath
) {
    if (g_HudUiMgrHudLoaded != 0) {
        return 1;
    }

    zReader::Node *const root = zReader::LoadNodeFromPath(
        entryPath,
        0,
        0
    );
    if (root == 0) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\Battlesport\\hud.cpp",
            0x60d,
            "Failed to read %s",
            entryPath
        );
        return 0;
    }

    HudUiEnsureLoaderWidgetsConstructed();

    zImage_InitMissionResources("..\\data\\common\\images\\hud");
    g_HudLayoutSW.LoadTypeIFromZarRoot(root);
    g_HudLayoutHW.LoadTypeIIFromZarRoot(root);
    SwitchActiveDialog(&g_HudLayoutSW);

    HudUiRect objectiveSummaryFont = {0};
    HudUiRect objectiveDescriptionFont = {0};
    HudUiRect ammoFont = {0};

    zReader::Node *const fontsNode = zReader_GetNamedNode(
        root,
        "FONTS"
    );
    if (fontsNode != 0) {
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            "OBJ_SUMMARY"
        )) {
            HudUiLayoutNode::ReadRect(
                node,
                &objectiveSummaryFont
            );
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            "OBJ_DESCRIPTION"
        )) {
            HudUiLayoutNode::ReadRect(
                node,
                &objectiveDescriptionFont
            );
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            "STRINGS"
        )) {
            HudUiPanelFontParams *const fontArgs =
                (HudUiPanelFontParams *)(&g_HudUiMgrStringMenu->unknown_10[0]);
            HudUiLayoutNode::ReadRect(
                node,
                (HudUiRect *)(fontArgs)
            );
            {
                int itemIndex4;
                for (itemIndex4 = 0; itemIndex4 < (int)(sizeof(g_HudUiMgrStringMenu->items) /
                                                        sizeof(g_HudUiMgrStringMenu->items[0]));
                    ++itemIndex4) {
                    HudUiPanelSimple &item = g_HudUiMgrStringMenu->items[itemIndex4];
                    item.SetFont(
                        fontArgs->faceName,
                        fontArgs->height,
                        fontArgs->weight,
                        fontArgs->width,
                        0,
                        0,
                        2
                    );
                }
            }
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            "MESSAGES"
        )) {
            HudUiRect messagesFont = {0};
            HudUiLayoutNode::ReadRect(
                node,
                &messagesFont
            );
            if (g_HudUiTopMessageStack != 0) {
                g_HudUiTopMessageStack->SetFontAll(
                    (const char *)(messagesFont.left),
                    messagesFont.right,
                    messagesFont.bottom,
                    messagesFont.top
                );
            }
            if (g_HudUiChatMessageStack != 0) {
                g_HudUiChatMessageStack->SetFontAll(
                    (const char *)(messagesFont.left),
                    messagesFont.right,
                    messagesFont.bottom,
                    messagesFont.top
                );
            }
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            "AMMO"
        )) {
            HudUiLayoutNode::ReadRect(
                node,
                &ammoFont
            );
        }
    }

    if (zReader::Node *const naniteNode = zReader_GetNamedNode(
        root,
        "NANITE"
    )) {
        g_HudUiMgrNanitePanel.InitLayout(naniteNode);
    }

    zReader::Node *const sensorNode = zReader_GetNamedNode(
        root,
        "SENSOR"
    );
    int sensorCenterX = 0;
    int sensorCenterY = 0;
    if (zReader::Node *const sensorPayload = HudUiZrdPayload(sensorNode)) {
        HudUiLayoutNode::ApplyImageWidget(
            &sensorPayload[1],
            &g_HudUiMgrSensorPanel,
            0,
            g_HudUiMgrHudOriginY,
            0,
            0,
            &g_HudUiMgrSensorBlock.sensorViewportRect
        );

        sensorCenterX = g_HudUiMgrSensorPanel.GetCenterX();
        sensorCenterY = g_HudUiMgrSensorPanel.GetCenterY();
        g_HudUiMgrSensorBlock.sensorParam = sensorPayload[2].value.f32;

        int sensorOffsetX = 0;
        int sensorOffsetY = 0;
        int sensorWidth = 0;
        int sensorHeight = 0;
        HudUiLayoutNode::ReadInt3(
            &sensorPayload[3],
            &sensorOffsetX,
            &sensorOffsetY,
            0
        );
        HudUiLayoutNode::ReadInt3(
            &sensorPayload[4],
            &sensorWidth,
            &sensorHeight,
            0
        );

        const int sensorX = sensorCenterX + sensorOffsetX;
        const int sensorY = sensorCenterY + sensorOffsetY;
        g_HudUiMgrSensorFxRect.left = sensorX;
        g_HudUiMgrSensorFxRect.top = sensorY;
        g_HudUiMgrSensorFxRect.right = sensorX + sensorWidth;
        g_HudUiMgrSensorFxRect.bottom = sensorY + sensorHeight;
        g_HudUiMgrSensorFxViewportWidth = sensorWidth;
        g_HudUiMgrSensorFxViewportHeight = sensorHeight;
        HudUiMgrSensor::SetViewportRect(
            sensorX,
            sensorY,
            sensorWidth,
            sensorHeight
        );

        const float range = sensorPayload[5].value.f32;
        float rangeBitsValue = range * range * 0.5f;
        unsigned int rangeBits = 0;
        memcpy(
            &rangeBits,
            &rangeBitsValue,
            sizeof(rangeBits)
        );
        rangeBits = (rangeBits >> 1) + 0x1fc00000u;
        memcpy(
            &rangeBitsValue,
            &rangeBits,
            sizeof(rangeBitsValue)
        );
        g_HudUiMgrSensorBlock.sensorRangeSq = rangeBitsValue + rangeBitsValue;

        const int overlayAnchor[2] = {sensorCenterX, sensorCenterY};
        HudUiLayoutNode::ApplyImageWidget(
            &sensorPayload[6],
            &g_HudUiMgrSensorOverlay,
            0,
            0,
            overlayAnchor,
            0,
            0
        );

        HudUiRect meterRect = {0};
        HudUiLayoutNode::ApplyMeterQuad(
            &sensorPayload[7],
            &g_HudUiMgrSensorMeter,
            0,
            0,
            overlayAnchor,
            &meterRect
        );
        g_HudUiMgrSensorMeter.color565 = 0x7e0;
        ((HudUiElement *)(&g_HudUiMgrSensorMeter))
            ->SetBltSourceAndClipRect(
                g_HudUiMgrSensorPanel.image,
                &meterRect
            );

        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorOverlay));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorMeter));
    }

    if (zReader::Node *const objectivePayload =
            HudUiZrdPayload(zReader_GetNamedNode(
                root,
                "OBJECTIVE"
            ))) {
        g_HudUiMgrObjectivePhaseDurationSec = objectivePayload[1].value.f32;

        const int panelCenter[2] = {sensorCenterX != 0 ? sensorCenterX
                                                       : g_HudUiMgrSensorPanel.GetCenterX(),
            sensorCenterY != 0 ? sensorCenterY : g_HudUiMgrSensorPanel.GetCenterY()};
        HudUiLayoutNode::ApplyImageWidget(
            &objectivePayload[2],
            &g_HudUiMgrObjectiveWidget,
            0,
            0,
            panelCenter,
            0,
            0
        );

        int objectiveCenter[2] = {g_HudUiMgrObjectiveWidget.GetCenterX(),
            g_HudUiMgrObjectiveWidget.GetCenterY()};
        HudUiRect objectiveBarRect = {0};
        HudUiLayoutNode::ApplyCornerTextQuad(
            &objectivePayload[3],
            &g_HudUiMgrObjectiveBar,
            objectiveCenter,
            &objectiveBarRect
        );
        g_HudUiMgrObjectiveBar.slideRangeX =
            (float)(panelCenter[0] - objectiveBarRect.left);

        int red = 0;
        int green = 0;
        int blue = 0;
        HudUiLayoutNode::ReadInt3(
            &objectivePayload[4],
            &red,
            &green,
            &blue
        );
        g_HudUiMgrObjectiveBar.drawParam =
            zVid_PackColorRGB(
                (unsigned char)(red),
                (unsigned char)(green),
                (unsigned char)(blue)
            ) &
            0xffffu;

        int x = 0;
        int y = 0;
        HudUiLayoutNode::ReadInt3(
            &objectivePayload[5],
            &x,
            &y,
            0
        );
        ((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel))
            ->SetPos(
                objectiveCenter[0] + x,
                objectiveCenter[1] + y
            );
        HudUiLayoutNode::ReadInt3(
            &objectivePayload[6],
            &x,
            &y,
            0
        );
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))
            ->SetPos(
                objectiveCenter[0] + x,
                objectiveCenter[1] + y
            );

        HudUiRect wrapRect = {0};
        wrapRect.left = 0;
        wrapRect.top = 0;
        wrapRect.right = panelCenter[0] - x * 2 - objectiveBarRect.left;
        wrapRect.bottom = panelCenter[1] - objectiveBarRect.bottom;
        g_HudUiMgrObjectiveDescTextPanel->EnableWordWrapWithRect(&wrapRect);

        HudUiLayoutNode::ApplyMeterQuad(
            &objectivePayload[7],
            &g_HudUiMgrObjectiveMeter,
            0,
            0,
            objectiveCenter,
            &objectiveBarRect
        );
        HudUiMgrObjective::UpdateMeterXPoints();
        const int meterTop = (int)(g_HudUiMgrObjectiveMeter.points[1].y) -
                             (int)(ceil((double)(g_HudUiMgrObjectiveMeter.fillPixelsMax)));
        g_HudUiMgrObjectiveMeter.color565 = 0x1f;
        g_HudUiMgrObjectiveMeter.points[0].y = (float)(meterTop);
        g_HudUiMgrObjectiveMeter.points[3].y = (float)(meterTop);

        HudUiLayoutNode::ReadInt3(
            &objectivePayload[8],
            &x,
            &y,
            0
        );
        ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->SetPos(
            x,
            y + g_HudUiMgrHudOriginY
        );
        g_HudUiMgrObjectiveLabelTextPanel->SetTextFmt(
            "%s",
            zLoc::GetMessageString(0x906)
        );
        ((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect))
            ->SetPos(
                g_HudUiMgrSensorFxRect.left,
                g_HudUiMgrSensorFxRect.top
            );

        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveWidget));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect));
        g_HudUiMgr.AddChild(&g_HudUiMgrObjectiveBar);
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveMeter));
        g_HudUiMgrObjectiveBar.SetVisible(0);

        g_HudUiMgrObjectiveState = 0;
        g_HudUiMgrObjectivePhase = 0;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
        g_HudUiMgrObjectiveChatComposeActive = 0;
        HudUiSetFontFromRect(
            g_HudUiMgrObjectiveDescTextPanel,
            objectiveDescriptionFont
        );
        HudUiSetFontFromRect(
            g_HudUiMgrObjectiveSummaryTextPanel,
            objectiveSummaryFont
        );
    }

    if (zReader::Node *const reticlePayload =
            HudUiZrdPayload(zReader_GetNamedNode(
                root,
                "RETICULE"
            ))) {
        g_HudUiMgrReticleImages[0] =
            zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(
                reticlePayload,
                1
            ));
        g_HudUiMgrReticleImages[1] =
            zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(
                reticlePayload,
                2
            ));
        g_HudUiMgrReticleImages[2] =
            zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(
                reticlePayload,
                3
            ));
        g_HudUiMgrReticleWidget.SetImageBorrowedAndInvalidate(g_HudUiMgrReticleImages[0]);
        g_HudUiMgrReticleWidget.imageStateWord =
            (g_HudUiMgrReticleWidget.imageStateWord & 0xffff0000u) | 1u;
        ((HudUiElement *)(&g_HudUiMgrReticleWidget))->Invalidate();
        zVidImagePartial *const image = g_HudUiMgrReticleWidget.image;
        g_HudUiMgrReticleWidgetHalfW = image != 0 ? (short)(image->width) / 2 : 0;
        g_HudUiMgrReticleWidgetHalfH = image != 0 ? (short)(image->height) / 2 : 0;
        ((HudUiElement *)(&g_HudUiMgrReticleWidget))->SetVisible(0);
    }

    if (zReader::Node *const statsPayload = HudUiZrdPayload(zReader_GetNamedNode(
        root,
        "STATS"
    ))) {
        HudUiWidget *const layoutWidget = &g_HudLayoutHW.widget1;
        const int layoutCenterX = layoutWidget->GetCenterX();
        const int layoutCenterY = layoutWidget->GetCenterY();
        int x = 0;
        int y = 0;
        int z = 0;
        HudUiLayoutNode::ReadInt3(
            &statsPayload[1],
            &x,
            &y,
            0
        );
        const int counterX = (g_HudUiMgrHudOriginX / 2) + x;
        ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))
            ->SetPos(
                counterX + layoutCenterX,
                y + layoutCenterY
            );
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->alignMode = 1;
        HudUiRect counterClip = {counterX - 0x14, y, counterX + 0x14, y + 0x0a};
        HudUiSetPanelClipWithSource(
            g_HudUiMgrObjectiveCounterTextPanel,
            0,
            &counterClip
        );
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->SetTextFmt("        ");
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->UpdateTextBoundsFromContent();
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->SetTextFmt(
            "%d",
            0
        );
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->UpdateTextBoundsFromContent();

        HudUiLayoutNode::ReadInt3(
            &statsPayload[2],
            &x,
            &y,
            0
        );
        const int timerX = x + g_HudUiMgrHudOriginX;
        ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetPos(
            timerX + layoutCenterX,
            y + layoutCenterY
        );
        HudUiRect timerClip = {timerX, y, 0, 0};
        HudUiSetPanelClipWithSource(
            g_HudUiMgrTimerPanel,
            0,
            &timerClip
        );
        ((HudUiPanel *)(g_HudUiMgrTimerPanel))->SetTextFmt("00:00:00");

        HudUiTriplet *const triplet = g_HudUiMgrStatsList->triplet;
        HudUiApplyStatsTripletInt3(
            statsPayload,
            3,
            x,
            y,
            &z
        );
        triplet->baseXStart = x + layoutCenterX + g_HudUiMgrHudOriginX;
        triplet->baseYStart = y + layoutCenterY;
        triplet->rowPitchYStart = z;
        HudUiApplyStatsTripletInt3(
            statsPayload,
            4,
            x,
            y,
            &z
        );
        triplet->baseXEnd = x + layoutCenterX + g_HudUiMgrHudOriginX;
        triplet->baseYEnd = y + layoutCenterY;
        triplet->rowPitchYEnd = z;
        HudUiApplyStatsTripletInt3(
            statsPayload,
            5,
            triplet->lapsColumnOffsetXStart,
            triplet->lapsColumnOffsetXEnd
        );
        HudUiApplyStatsTripletInt3(
            statsPayload,
            6,
            triplet->killsColumnOffsetXStart,
            triplet->killsColumnOffsetXEnd
        );
        HudUiApplyStatsTripletInt3(
            statsPayload,
            7,
            triplet->fontSizeStart,
            triplet->fontSizeEnd
        );
        HudUiApplyStatsTripletInt3(
            statsPayload,
            8,
            triplet->fontWeightStart,
            triplet->fontWeightEnd
        );
        triplet->InterpolateLayout(0.0f);
        triplet->RebuildDisplay();
    }

    if (zReader::Node *const shieldNode = zReader_GetNamedNode(
        root,
        "SHIELD"
    )) {
        HudUiShieldMessageWidget::ApplyLayout(shieldNode);
    }

    if (zReader::Node *const targetPayload =
            HudUiZrdPayload(zReader_GetNamedNode(
                root,
                "TARGET"
            ))) {
        {
            for (int index = 0; index < 5; ++index) {
                zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(
                    targetPayload,
                    index + 1
                ));
            }
        }

        {
            int slotIndex5;
            for (slotIndex5 = 0; slotIndex5 < (int)(sizeof(g_HudUiMgrWeaponSlots) /
                                                    sizeof(g_HudUiMgrWeaponSlots[0]));
                ++slotIndex5) {
                HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex5];
                slot.trackMarkerWidget.imageStateWord =
                    (slot.trackMarkerWidget.imageStateWord & 0xffff0000u) | 1u;
                ((HudUiElement *)(&slot.trackMarkerWidget))->Invalidate();
            }
        }

        {
            int slotIndex6;
            for (slotIndex6 = 0; slotIndex6 < (int)(sizeof(g_HudUiMgrWeaponSlots) /
                                                    sizeof(g_HudUiMgrWeaponSlots[0]));
                ++slotIndex6) {
                HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex6];
                ((HudUiElement *)(&slot.slotWidget))->Invalidate();
                g_HudUiMgr.AddChild(&slot);
                ((HudUiElement *)(&slot.trackMarkerWidget))->SetVisible(0);
                ((HudUiElement *)(&slot.slotWidget))->SetVisible(0);
            }
        }
        g_HudUiMgrSensorTargetMarkerCount = 0;
        g_HudUiMgrWeaponState = 0;
    }

    zReader::Node *weaponPayload = HudUiZrdPayload(zReader_GetNamedNode(
        root,
        "WEAPON"
    ));
    if (weaponPayload != 0) {
        {
            for (int index = 1; index < 10; ++index) {
                g_HudUiMgrMessages[index].LoadWeaponLayoutFromNode(
                    &weaponPayload[index],
                    (const HudUiPanelFontParams *)(&ammoFont)
                );
            }
        }
    }

    zReader::Node *modesPayload = HudUiZrdPayload(zReader_GetNamedNode(
        root,
        "MODES"
    ));
    if (modesPayload != 0) {
        {
            for (int index = 0; index < 4; ++index) {
                g_HudUiMgrModeCounters[index].ApplyFromLayoutNode(&modesPayload[index + 1]);
            }
        }
    }

    SetModeCounterState(
        0,
        2
    );
    zReader::FreeLoadedTree(root);
    SetFloatTimerVisible(0);
    SetAuxOverlayVisible(0);
    g_HudUiMgrHudLoaded = 1;
    return 1;
}

// Reimplements 0x411750: HudUiMgr::SetNanitePanelCount
void __fastcall SetNanitePanelCount(
    int count
) {
    g_HudUiMgrNanitePanel.SetVisibleCount(count);
}

// Reimplements 0x40f1a0: HudUiMgr::SetModeCounterState
void __fastcall SetModeCounterState(
    int counterIndex,
    int state
) {
    if (state == 2) {
        HudUiCounter &previous = g_HudUiMgrModeCounters[g_HudUiMgrActiveModeCounterIndex];
        previous.SetImageBorrowedAndInvalidate(previous.stateImages[1]);
        g_HudUiMgrActiveModeCounterIndex = counterIndex;
    }

    HudUiCounter &counter = g_HudUiMgrModeCounters[counterIndex];
    counter.SetImageBorrowedAndInvalidate(counter.stateImages[state]);
}

// Reimplements 0x411710: HudUiMgr::ReticleStaticAtexitStub
void ReticleStaticAtexitStub() {}

// Reimplements 0x411720: HudUiMgr::CopyReticleProjection
void __fastcall CopyReticleProjection(
    float *outProjection
) {
    unsigned int *const outBits = (unsigned int *)(outProjection);
    const unsigned int *const projectionBits = (const unsigned int *)(g_HudUiMgrReticleProjection);
    outBits[0] = projectionBits[0];
    outBits[1] = projectionBits[1];
    outBits[2] = projectionBits[2];
}

// Reimplements 0x411740: HudUiMgr::SetReticleMode
void __fastcall SetReticleMode(
    int mode
) {
    g_HudUiMgrReticleMode = mode;
}

// Reimplements 0x411270: HudUiMgr::UpdateTargetReticleFromCursor (D:\Proj\Battlesport\hud.cpp)
int __fastcall UpdateTargetReticleFromCursor(
    int reticleMode,
    zVec3 *worldHitPoint,
    float normalizedX,
    float normalizedY
) {
    HudUiElement *const reticleElement = (HudUiElement *)(&g_HudUiMgrReticleWidget);

    if (reticleMode == 0) {
        reticleElement->SetVisible(0);
        return 0;
    }

    if (reticleMode == 1) {
        reticleElement->SetVisible(1);
        return 0;
    }

    if (reticleMode != 2) {
        return 0;
    }

    float screenX =
        (normalizedX + 1.0f) * g_HudUiMgrReticleMapScaleHalfW + g_HudUiMgrReticleMapBiasX;
    float screenY =
        (normalizedY + 1.0f) * g_HudUiMgrReticleMapScaleHalfH + g_HudUiMgrReticleMapBiasY;

    const int projectedX = (int)(screenX);
    const int projectedY = (int)(screenY);
    g_HudUiMgrReticleProjectedX = projectedX;
    g_HudUiMgrReticleProjectedY = projectedY;

    reticleElement->SetPos(
        projectedX - g_HudUiMgrReticleWidgetHalfW,
        projectedY - g_HudUiMgrReticleWidgetHalfH
    );

    if ((g_HudLayoutHW.reticleClipInitFlags & 1) == 0) {
        g_HudLayoutHW.reticleClipInitFlags =
            (unsigned char)(g_HudLayoutHW.reticleClipInitFlags | 1);
        atexit(&HudUiMgr::ReticleStaticAtexitStub);
    }

    RECT reticleBounds = {0};
    reticleBounds.top = g_HudUiMgrReticleWidget.GetCenterY();
    reticleBounds.bottom =
        g_HudUiMgrReticleWidget.GetCenterY() +
        (g_HudUiMgrReticleWidget.image != 0 ? g_HudUiMgrReticleWidget.image->height : 0);
    reticleBounds.left = g_HudUiMgrReticleWidget.GetCenterX();
    reticleBounds.right =
        g_HudUiMgrReticleWidget.GetCenterX() +
        (g_HudUiMgrReticleWidget.image != 0 ? g_HudUiMgrReticleWidget.image->width : 0);

    if (IntersectRect(
            (RECT *)(&g_HudLayoutHW.reticleClipRect),
            &reticleBounds,
            (const RECT *)(zOpt::GetDisplaySection())
        ) != 0) {
        g_HudLayoutHW.reticleClipRect.top -= g_HudUiMgrReticleWidget.GetCenterY();
        g_HudLayoutHW.reticleClipRect.bottom -= g_HudUiMgrReticleWidget.GetCenterY();
        g_HudLayoutHW.reticleClipRect.left -= g_HudUiMgrReticleWidget.GetCenterX();
        g_HudLayoutHW.reticleClipRect.right -= g_HudUiMgrReticleWidget.GetCenterX();

        reticleElement->SetPos(
            g_HudUiMgrReticleWidget.GetCenterX() + g_HudLayoutHW.reticleClipRect.left,
            g_HudUiMgrReticleWidget.GetCenterY() + g_HudLayoutHW.reticleClipRect.top
        );
        g_HudUiMgrReticleWidget.bltClipRectOrNull = &g_HudLayoutHW.reticleClipRect;
    }

    zProjectedPoint projectedPoint = {screenX, screenY, 0.0f};
    ScreenToWorld(&projectedPoint.x);

    HudReticlePlayerStatePartial *const playerState =
        (HudReticlePlayerStatePartial *)(g_GameStateOrMapTable->playerState);

    float nearClip = 0.0f;
    float farClip = 0.0f;
    zClass_Camera::gwCameraGetNearFarClip(
        g_MainCamera,
        &nearClip,
        &farClip
    );

    zVec3 nearPoint = {0};
    projectedPoint.reciprocalZ = 1.0f / nearClip;
    zMath_UnprojectPointBatchZBuf(
        &projectedPoint,
        &nearPoint,
        1
    );

    zVec3 farPoint = {0};
    projectedPoint.reciprocalZ = 1.0f / playerState->activeAltGunController->optCatalogEntry->range;
    zMath_UnprojectPointBatchZBuf(
        &projectedPoint,
        &farPoint,
        1
    );

    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    if (playerState->cameraState == 7) {
        zClass_Class::gwNodeSetRaycastable(
            playerState->activeAltGunController->attachState->projectileNode,
            0
        );
    }

    zClass_cls_di::SetStopAfterFirstHit(0x40000);
    PlayerProbeSampleCandidateBuffer rayData = {0};
    const int raycastResult = zClass_cls_di::RaycastSelectClosestHitBetweenPoints(
        g_Player_RuntimeDiScene,
        &nearPoint,
        &farPoint,
        &rayData
    );

    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    if (playerState->cameraState == 7) {
        zClass_Class::gwNodeSetRaycastable(
            playerState->activeAltGunController->attachState->projectileNode,
            1
        );
    }

    zVidImagePartial *reticleImage = 0;
    if (raycastResult != 0) {
        g_HudUiMgrReticleProjection[0] = farPoint.x;
        g_HudUiMgrReticleProjection[1] = farPoint.y;
        g_HudUiMgrReticleProjection[2] = farPoint.z;
        reticleImage = g_HudUiMgrReticleImages[1];
    } else {
        const zClassDiPickCandidateEntry &candidate = rayData.entries[rayData.candidateCount];
        g_HudUiMgrReticleProjection[0] = candidate.hitPos.x;
        g_HudUiMgrReticleProjection[1] = candidate.hitPos.y;
        g_HudUiMgrReticleProjection[2] = candidate.hitPos.z;

        zClass_NodeFreeListSlot *const hitSlot = (zClass_NodeFreeListSlot *)(candidate.node);
        reticleImage =
            hitSlot->damageHandler != 0 ? g_HudUiMgrReticleImages[2] : g_HudUiMgrReticleImages[0];
    }

    g_HudUiMgrReticleWidget.SetImageBorrowedAndInvalidate(reticleImage);

    worldHitPoint->x = g_HudUiMgrReticleProjection[0];
    worldHitPoint->y = g_HudUiMgrReticleProjection[1];
    worldHitPoint->z = g_HudUiMgrReticleProjection[2];

    zOpt_ViewRectSection *const renderRect = zOpt::GetRenderSection();
    const float minX = (float)(renderRect->x) + g_HudUiMgrSensorBlock.sensorClampHalfW;
    if (!(screenX >= minX)) {
        screenX = minX;
    } else {
        const float maxX =
            (float)(renderRect->rightExclusive) - g_HudUiMgrSensorBlock.sensorClampHalfW;
        if (screenX > maxX) {
            screenX = maxX;
        }
    }

    const float minY = (float)(renderRect->y) + g_HudUiMgrSensorBlock.sensorClampHalfH;
    if (!(screenY >= minY)) {
        screenY = minY;
    } else {
        const float maxY =
            (float)(renderRect->bottomExclusive) - g_HudUiMgrSensorBlock.sensorClampHalfH;
        if (screenY > maxY) {
            screenY = maxY;
        }
    }

    zClipAltFloatRect targetRect = {screenX - g_HudUiMgrSensorBlock.sensorClampHalfW,
        screenY - g_HudUiMgrSensorBlock.sensorClampHalfH,
        screenX + g_HudUiMgrSensorBlock.sensorClampHalfW,
        screenY + g_HudUiMgrSensorBlock.sensorClampHalfH};
    zClipAlt::SetTargetRect(
        &targetRect,
        zOpt::GetReplicateMode()
    );
    return 0;
}

// Reimplements 0x413730: HudUiMgr::DestroySensorWindow
void DestroySensorWindow() {
    zFMV_Playback *playback = g_HudUiSensorWindowPlayback;
    if (playback == 0) {
        return;
    }

    playback->StopAndClose();

    playback = g_HudUiSensorWindowPlayback;
    if (playback != 0) {
        playback->Destructor();
        ::operator delete(playback);
    }

    g_HudUiSensorWindowPlayback = 0;
    g_HudUiSensorWindow.CWnd::DestroyWindow();
}

// Reimplements 0x410e90: HudUiMgr::EnableHud
// (D:\Proj\Battlesport\hud.cpp)
int EnableHud() {
    const int previouslyEnabled = g_HudUiMgr.enabled;
    g_HudUiMgr.SetEnabled(1);

    g_HudUiMgrCurrentLayout->Enable();

    HudUiMgrObjective::Update();
    zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);
    gAltClipPassEnabled = 1;
    return previouslyEnabled;
}

// Reimplements 0x410ed0: HudUiMgr::DisableHud
int DisableHud() {
    const int previouslyEnabled = g_HudUiMgr.enabled;
    DestroySensorWindow();

    {
        int slotIndex;
        for (slotIndex = 0;
            slotIndex < (int)(sizeof(g_HudUiMgrWeaponSlots) / sizeof(g_HudUiMgrWeaponSlots[0]));
            ++slotIndex) {
            HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex];
            slot.trackMarkerWidget.SetVisible(0);
            slot.slotWidget.SetVisible(0);
        }
    }

    g_HudUiMgrSensorTargetMarkerCount = 0;
    g_HudUiMgrWeaponState = 0;
    g_HudUiMgr.SetEnabled(0);

    g_HudUiMgrCurrentLayout->Disable();

    g_HudUiMgrObjectiveWidget.SetVisible(0);
    g_HudUiMgrObjectiveDescTextPanel->SetVisible(0);
    g_HudUiMgrObjectiveBar.SetVisible(0);
    g_HudUiMgrObjectiveSensorRect.SetVisible(0);
    g_HudUiMgrObjectiveSummaryTextPanel->SetVisible(0);
    g_HudUiMgrObjectiveLabelTextPanel->SetVisible(0);
    g_HudUiMgrObjectiveMeter.SetVisible(0);

    gAltClipPassEnabled = 0;
    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionResetFrame();
    }

    const int hudType = zOpt::GetHudTypeForCurrentHwMode();
    if (hudType == 2) {
        g_HudUiMgrLayoutDelayFrames = hudType;
    }

    g_HudUiMgrTimerPanel->SetVisible(1);
    return previouslyEnabled;
}

// Reimplements 0x413640: HudUiMgr::ToggleHud
// (D:\Proj\Battlesport\hud.cpp)
int ToggleHud() {
    if (g_HudUiMgr.enabled != 0) {
        DisableHud();
    } else {
        EnableHud();
    }
    return 1;
}

// Reimplements 0x410fe0: HudUiMgr::UpdateFrame (D:\Proj\Battlesport\hud.cpp)
void UpdateFrame() {
    g_HudUiMgrCurrentLayout->LayoutPreUpdate();

    if (g_HudUiMgr.enabled != 0) {
        if (g_HudUiMgrObjectiveState != 0) {
            HudUiMgrObjective::StartHide();
        }
    } else {
        if (g_HudUiMgrObjectiveChatComposeActive != 0) {
            g_HudUiMgrObjectiveSummaryTextPanel->Draw();
            g_HudUiMgrObjectiveDescTextPanel->Draw();
        }

        g_HudUiMgrTimerPanel->Update(
            g_Time_UnscaledDeltaTimeSec
        );
    }

    if (g_HudUiMgrObjectiveMeterFillAnimEnabled != 0) {
        HudUiMgrObjective::TickMeterFillAnimation();
    }

    g_HudSensorTracker.Update();
    zTimedTask::TickActiveList();

    g_HudUiMgrCurrentLayout->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiMgr.UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiTopMessageStack->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiChatMessageStack->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiMgrStringMenu->UpdateAll(g_Time_UnscaledDeltaTimeSec);

    const float sampleElapsedSec =
        g_HudUiMgrTimerPanelFloat->sampleElapsedSec + g_FrameDeltaTimeSec;
    g_HudUiMgrTimerPanelFloat->sampleElapsedSec = sampleElapsedSec;

    const float sampleFrameCount =
        g_HudUiMgrTimerPanelFloat->sampleFrameCount + 1.0f;
    g_HudUiMgrTimerPanelFloat->sampleFrameCount = sampleFrameCount;
    if (sampleElapsedSec >= 1.0f) {
        g_HudUiMgrTimerPanelFloat->sampleFrameCount = 0.0f;
        g_HudUiMgrTimerPanelFloat->sampleElapsedSec = 0.0f;
        g_HudUiMgrTimerPanelFloat->displayValue =
            sampleFrameCount / sampleElapsedSec;
    }

    HudUiElement *const floatingTimerElement = (HudUiElement *)(g_HudUiMgrTimerPanelFloat);
    if ((floatingTimerElement->flags & 0x10) == 0) {
        g_HudUiMgrTimerPanelFloat->Draw();
    }

    g_HudUiMgrReticleWidget.Update(
        g_Time_UnscaledDeltaTimeSec
    );

    {
        for (int slotIndex = 0; slotIndex < 32; ++slotIndex) {
            HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex];
            slot.trackMarkerWidget.SetVisible(0);
            slot.slotWidget.SetVisible(0);
        }
    }

    g_HudUiMgrSensorTargetMarkerCount = 0;
    g_HudUiMgrWeaponState = 0;
}

// Reimplements 0x413660: HudUiMgr::SwitchActiveDialog
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall SwitchActiveDialog(
    HudLayoutBase *newDialog
) {
    const int enabled = g_HudUiMgr.enabled;
    if (enabled != 0) {
        DisableHud();
    } else {
        g_HudUiMgrLayoutDelayFrames = 2;
    }

    if (g_HudUiMgrCurrentLayout != 0) {
        g_HudUiMgrCurrentLayout->SetActive(0);
    }

    newDialog->SetActive(1);
    g_HudUiMgrCurrentLayout = newDialog;

    if (enabled != 0) {
        EnableHud();
    }
}

// Reimplements 0x413770: HudUiMgr::SetFloatTimerVisible
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall SetFloatTimerVisible(
    int visible
) {
    g_HudUiMgrTimerPanelFloat->SetVisible(visible != 0 ? 1 : 0);

    if (visible == 0) {
        TriggerCurrentLayoutOnActivated();
    }
}

// Reimplements 0x412620: HudUiMgr::HideTrackedProgressMeterIfOwnerMatches
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall HideTrackedProgressMeterIfOwnerMatches(
    void *ownerPayload
) {
    HudUiSlot *const trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    if (trackedProgressSlot == 0) {
        return;
    }

    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
    if (trackNode->payload == ownerPayload) {
        g_HudUiMgrSensorMeter.SetVisible(0);
    }
}

// Reimplements 0x4137a0: HudUiMgr::SetAuxOverlayVisible
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall SetAuxOverlayVisible(
    int visible
) {
    g_HudUiMgrStringMenu->SetEnabled(visible != 0 ? 1 : 0);
}

// Reimplements 0x4136b0: HudUiMgr::ApplyHudModeSwitch
// (D:\Proj\Battlesport\hud.cpp)
int __fastcall ApplyHudModeSwitch(
    int hudType
) {
    const int currentType = zOpt::GetHudTypeForCurrentHwMode();
    if (g_HudUiMgrHudLayoutsInitialized != 0) {
        if (hudType == 1) {
            SwitchActiveDialog((HudLayoutBase *)(&g_HudLayoutSW));
        } else if (hudType == 2) {
            SwitchActiveDialog((HudLayoutBase *)(&g_HudLayoutHW));
        }
    }

    return currentType;
}

// Reimplements 0x4089c0: HudUiMgr::ScreenToWorld
void __fastcall ScreenToWorld(
    float *pointXY
) {
    zOpt_ViewRectSection *const renderSection = *g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection *const displaySection = *g_zOpt_DisplaySectionOption;

    if (zOpt::GetReplicateMode() == 0) {
        return;
    }

    pointXY[0] *= 0.5f;
    pointXY[1] = (float)(renderSection->y) + (pointXY[1] - (float)(displaySection->y)) * 0.5f;
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(
        renderSection,
        pointXY
    );
}

// Reimplements 0x40ff80: HudUiMgr::OnViewportChanged
void __fastcall OnViewportChanged(
    const HudUiRect *hudRectOrNull,
    const HudUiRect *viewRectOrNull
) {
    if (hudRectOrNull != 0) {
        g_HudUiMgrHudRect = *hudRectOrNull;
    } else {
        hudRectOrNull = &g_HudUiMgrHudRect;
    }

    if (viewRectOrNull != 0) {
        g_HudUiMgrViewRect = *viewRectOrNull;
    } else {
        viewRectOrNull = &g_HudUiMgrViewRect;
    }

    const int viewWidth = viewRectOrNull->right - viewRectOrNull->left;
    const float viewWidthFloat = (float)(viewWidth);
    const int viewHeight = viewRectOrNull->bottom - viewRectOrNull->top;
    const float viewHeightFloat = (float)(viewHeight);

    g_HudUiMgrHudRectW = (float)(hudRectOrNull->right - hudRectOrNull->left);
    g_HudUiMgrHudRectH = (float)(hudRectOrNull->bottom - hudRectOrNull->top);
    g_HudUiMgrReticleMapBiasX = (float)(hudRectOrNull->left);
    g_HudUiMgrReticleMapBiasY = (float)(hudRectOrNull->top);

    const int snapRadius = viewWidth / 10;
    g_HudUiMgrReticleMapScaleHalfW = (g_HudUiMgrHudRectW / viewWidthFloat) * viewWidthFloat * 0.5f;
    g_HudUiMgrReticleSnapRadiusSq = snapRadius * snapRadius;
    g_HudUiMgrReticleMapScaleHalfH =
        (g_HudUiMgrHudRectH / viewHeightFloat) * viewHeightFloat * 0.5f;

    HudUiMgrSensor::SetViewportRect(
        g_HudUiMgrSensorFxRect.left,
        g_HudUiMgrSensorFxRect.top,
        g_HudUiMgrSensorFxViewportWidth,
        g_HudUiMgrSensorFxViewportHeight
    );

    if (g_HudUiMgrCurrentLayout != 0) {
        g_HudUiMgrCurrentLayout->OnActivated();
    }

    HudUiMgrObjective::Update();

    if (g_HudUiTopMessageStack != 0) {
        g_HudUiTopMessageStack->SetTextColors(
            0x0020bf40,
            0x0020bf40
        );
        g_HudUiTopMessageStack->SetXAll(zVideo::GetPrimarySurfaceWidth() / 2);
    }

    if (g_HudUiChatMessageStack != 0) {
        g_HudUiChatMessageStack->SetTextColors(
            0x0020bf40,
            0x0020bf40
        );
        g_HudUiChatMessageStack->SetXAll(zVideo::GetPrimarySurfaceWidth() / 2);
        g_HudUiChatMessageStack->SetYDescending(zVideo::GetPrimarySurfaceHeight() - 0x88);
    }
}

// Reimplements 0x40ff50: HudUiMgr::ActivateHud
void __fastcall ActivateHud(
    const HudUiRect *hudRectOrNull,
    const HudUiRect *viewRectOrNull
) {
    OnViewportChanged(
        hudRectOrNull,
        viewRectOrNull
    );
    g_HudUiMgrShieldMessageWidget->viewportResetFrame = -1;
    g_HudUiMgrShieldMessageWidget->state = 0;
    g_HudUiMgrSensorBlock.state = 1;
}

// Reimplements 0x413910: HudUiMgr::EnableTopAndChatStacks
void EnableTopAndChatStacks() {
    g_HudUiTopMessageStack->Clear();
    g_HudUiTopMessageStack->SetEnabled(1);
    g_HudUiChatMessageStack->Clear();
    g_HudUiChatMessageStack->SetEnabled(1);
}

// Reimplements 0x413950: HudUiMgr::DisableTopAndChatStacks
void DisableTopAndChatStacks() {
    g_HudUiTopMessageStack->Clear();
    g_HudUiTopMessageStack->SetEnabled(0);
    g_HudUiChatMessageStack->Clear();
    g_HudUiChatMessageStack->SetEnabled(0);
}

// Reimplements 0x40f4c0: HudUiMgr::InitHudLayouts
int __fastcall InitHudLayouts(
    const HudUiRect *displaySection,
    const HudUiRect *windowSection
) {
    if (g_HudUiMgrHudLayoutsInitialized != 0) {
        return 1;
    }

    HudUiTimerPanelFloat *const timerPanelFloat = AllocateHudObject<HudUiTimerPanelFloat>();
    g_HudUiMgrTimerPanelFloat = timerPanelFloat != 0 ? timerPanelFloat->ConstructorDefault() : 0;

    HudUiStringMenu *const stringMenu = AllocateHudObject<HudUiStringMenu>();
    if (stringMenu != 0) {
        new ((HudUiContainer *)stringMenu) HudUiContainer;

        int y = 0x5f;
        {
            int itemIndex;
            for (itemIndex = 0;
                itemIndex < (int)(sizeof(stringMenu->items) / sizeof(stringMenu->items[0]));
                ++itemIndex) {
                HudUiPanelSimple &item = stringMenu->items[itemIndex];
                item.ConstructorDefaultThunk();

                HudUiElement *const child = (HudUiElement *)(&item);
                child->SetPos(
                    5,
                    y
                );
                stringMenu->AddChild(child);
                child->SetVisible(1);
                y += 0x0f;
            }
        }

        stringMenu->SetEnabled(1);
    }
    g_HudUiMgrStringMenu = stringMenu;

    HudUiShieldMessageWidget *const shieldMessageWidget =
        AllocateHudObject<HudUiShieldMessageWidget>();
    if (shieldMessageWidget != 0) {
        shieldMessageWidget->widget.Constructor(0);
        HudUiPanel *const percentTextPanel = (HudUiPanel *)(&shieldMessageWidget->percentTextPanel);
        percentTextPanel->ConstructorDefault(
            0,
            0,
            0
        );
        percentTextPanel->SetTextColor(0x0020bf40);
        percentTextPanel->SetFont(
            "Arial",
            0x0a,
            0x1f4,
            6,
            0,
            0,
            2
        );
        percentTextPanel->SetShadow(
            1,
            -1,
            -1
        );
        shieldMessageWidget->meter.Constructor();
    }
    g_HudUiMgrShieldMessageWidget = shieldMessageWidget;

    HudUiCounterTextPanel *const counterTextPanel = AllocateHudObject<HudUiCounterTextPanel>();
    g_HudUiMgrObjectiveCounterTextPanel =
        counterTextPanel != 0 ? counterTextPanel->Constructor() : 0;

    HudUiTimerPanel *const timerPanel = AllocateHudObject<HudUiTimerPanel>();
    g_HudUiMgrTimerPanel = timerPanel != 0 ? timerPanel->ConstructorDefault() : 0;

    HudUiStatsListElement *const statsList = AllocateHudObject<HudUiStatsListElement>();
    if (statsList != 0) {
        statsList->Constructor(
            0,
            0
        );

        HudUiTriplet *const statsTriplet = AllocateHudObject<HudUiTriplet>();
        statsList->triplet = statsTriplet != 0 ? statsTriplet->Constructor() : 0;
    }
    g_HudUiMgrStatsList = statsList;

    g_HudUiMgrObjectiveSummaryTextPanel = NewObjectivePanel();
    g_HudUiMgrObjectiveDescTextPanel = NewObjectivePanel();
    g_HudUiMgrObjectiveLabelTextPanel = NewObjectivePanel();

    HudUiTopMessageStack *const topMessageStack = AllocateHudObject<HudUiTopMessageStack>();
    g_HudUiTopMessageStack = topMessageStack != 0 ? topMessageStack->Constructor() : 0;

    HudUiChatMessageStack *const chatMessageStack = AllocateHudObject<HudUiChatMessageStack>();
    g_HudUiChatMessageStack = chatMessageStack != 0 ? chatMessageStack->Constructor() : 0;

    g_HudUiMgrHudLoaded = 0;
    g_HudUiMgrLayoutDelayFrames = 0;
    g_HudUiMgrStatsListState1 = 0;
    g_HudUiMgrStatsListState2 = 0;
    g_HudUiMgrStatsListState3 = 0;
    g_HudUiMgrStatsListState5 = 0;

    ActivateHud(
        displaySection,
        windowSection
    );
    g_HudUiMgrHudOriginX = displaySection->right - 0x280;
    g_HudUiMgrHudOriginY = displaySection->bottom - 0x1e0;

    g_HudUiTripletWndClassName = AfxRegisterWndClass(
        0x83,
        0,
        0,
        0
    );

    zUtil_ZAR::RegisterSectionHandler(
        "HUDTimer",
        ZbdCallbackPtr(&HudUiTimerPanel::ZarWriteTimerDataCallback),
        ZbdCallbackPtr(&HudUiTimerPanel::ZarReadTimerData),
        0x64,
        g_HudUiMgrTimerPanel
    );

    g_HudUiMgrHudLayoutsInitialized = 1;
    if (g_HudUiMgrStatsList != 0) {
        g_HudUiMgrStatsList->SetVisible(1);
        g_HudUiMgr.AddChild(g_HudUiMgrStatsList);
    }
    return 1;
}

// Reimplements 0x40fbd0: HudUiMgr::ShutdownResources
void ShutdownResources() {
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrReticleImages[0]);
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrReticleImages[1]);
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrReticleImages[2]);

    {
        int counterIndex12;
        for (counterIndex12 = 0; counterIndex12 < (int)(sizeof(g_HudUiMgrModeCounters) /
                                                        sizeof(g_HudUiMgrModeCounters[0]));
            ++counterIndex12) {
            HudUiCounter &counter = g_HudUiMgrModeCounters[counterIndex12];
            counter.ReleaseStateImages();
        }
    }

    g_HudUiMgrNanitePanel.ShutdownItems_Stub();
    HudLayoutBase::Shutdown_Stub();

    {
        for (size_t index = 1; index < 10; ++index) {
            g_HudUiMgrMessages[index].ReleaseImages();
        }
    }

    zGame::ReturnOnlyStub();
    g_HudLayoutHW.ReleaseImages();

    if (g_HudUiMgrTimerPanelFloat != 0) {
        ((HudUiPanel *)(g_HudUiMgrTimerPanelFloat))->ScalarDeletingDestructor(1);
        g_HudUiMgrTimerPanelFloat = 0;
    }

    if (g_HudUiMgrStringMenu != 0) {
        HudUiStringMenu *const stringMenu = g_HudUiMgrStringMenu;
        stringMenu->DestructorCore();
        ::operator delete(stringMenu);
        g_HudUiMgrStringMenu = 0;
    }

    if (g_HudUiMgrShieldMessageWidget != 0) {
        HudUiShieldMessageWidget *const shieldMessageWidget = g_HudUiMgrShieldMessageWidget;
        shieldMessageWidget->Destructor();
        ::operator delete(shieldMessageWidget);
        g_HudUiMgrShieldMessageWidget = 0;
    }

    if (g_HudUiMgrObjectiveCounterTextPanel != 0) {
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->ScalarDeletingDestructor(1);
        g_HudUiMgrObjectiveCounterTextPanel = 0;
    }

    if (g_HudUiMgrTimerPanel != 0) {
        ((HudUiPanel *)(g_HudUiMgrTimerPanel))->ScalarDeletingDestructor(1);
        g_HudUiMgrTimerPanel = 0;
    }

    if (g_HudUiMgrStatsList != 0) {
        g_HudUiMgrStatsList->ScalarDeletingDestructor(1);
        g_HudUiMgrStatsList = 0;
    }

    if (g_HudUiMgrObjectiveSummaryTextPanel != 0) {
        g_HudUiMgrObjectiveSummaryTextPanel->ScalarDeletingDestructor(1);
        g_HudUiMgrObjectiveSummaryTextPanel = 0;
    }

    if (g_HudUiMgrObjectiveDescTextPanel != 0) {
        g_HudUiMgrObjectiveDescTextPanel->ScalarDeletingDestructor(1);
        g_HudUiMgrObjectiveDescTextPanel = 0;
    }

    if (g_HudUiMgrObjectiveLabelTextPanel != 0) {
        g_HudUiMgrObjectiveLabelTextPanel->ScalarDeletingDestructor(1);
        g_HudUiMgrObjectiveLabelTextPanel = 0;
    }

    if (g_HudUiTopMessageStack != 0) {
        HudUiTextStack4 *const topStack = g_HudUiTopMessageStack;
        ((HudUiTopMessageStack *)(topStack))->DestructorCore();
        ::operator delete(topStack);
        g_HudUiTopMessageStack = 0;
    }

    if (g_HudUiChatMessageStack != 0) {
        HudUiTextStack4 *const chatStack = g_HudUiChatMessageStack;
        ((HudUiChatMessageStack *)(chatStack))->DestructorCore();
        ::operator delete(chatStack);
        g_HudUiChatMessageStack = 0;
    }

    g_HudUiMgrHudLayoutsInitialized = 0;
    g_HudUiMgrHudLoaded = 0;
}
} // namespace HudUiMgr

/**
 * Reimplements 0x4b4070: HudUiElement::Constructor.
 * Original source path: D:\Proj\Battlesport\hudui_element.cpp.
 * Purpose: Initializes the common HUD element position, links, timer, invalidation state, and blit source.
 */
HudUiElement::HudUiElement(
    int initX,
    int initY
) {
    HudUiElement *const element = this;
    element->y = initY;
    parent = 0;
    next = 0;
    timer = 0.0f;
    element->x = initX;
    element->Invalidate();

    flags = 0;
    state = 0;
    HudUiElement::SetBltSourceAndClipRect(
        0,
        0
    );
}

HudUiElement * HudUiElement::Constructor(
    int initX,
    int initY
) {
    new (this) HudUiElement(
        initX,
        initY
    );
    return this;
}

// Reimplements 0x4b40c0: HudUiElement::CopyConstructor
HudUiElement * HudUiElement::CopyConstructor(
    const HudUiElement *source
) {
    next = 0;
    parent = 0;
    flags = source->flags;
    timer = source->timer;
    x = source->x;
    y = source->y;
    bltSource = source->bltSource;
    clipRect = source->clipRect;
    state = source->state;
    return this;
}

// Reimplements 0x4b4120: HudUiElement::CopyFrom
HudUiElement * HudUiElement::CopyFrom(
    const HudUiElement *source
) {
    next = 0;
    parent = 0;
    flags = source->flags;
    state = source->state;
    timer = source->timer;
    x = source->x;
    y = source->y;
    bltSource = source->bltSource;
    clipRect = source->clipRect;
    return this;
}

// Reimplements 0x404d70: HudUiElement::ScalarDeletingDestructor
HudUiElement * HudUiElement::ScalarDeletingDestructor(
    unsigned int flags
) {
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x404ca0: HudUiElement::Draw
void HudUiElement::Draw() {
    DrawBase();
}

// Reimplements 0x404cb0: HudUiElement::DrawBase
void HudUiElement::DrawBase() {
    if (bltSource != 0) {
        zVid_Image::BlitToActiveTarget(
            (zVidImagePartial *)(bltSource),
            x,
            y,
            0,
            (zVidRect32 *)(&clipRect)
        );
    }
}

/**
 * Reimplements 0x4b4180: HudUiElement::Invalidate.
 * Purpose: mark the element dirty by OR-ing the current HUD invalidation mask into its flags.
 */
void HudUiElement::Invalidate() {
    flags |= g_HudUi_InvalidateMask;
}

// Reimplements 0x404cd0: HudUiElement::SetPos
void HudUiElement::SetPos(
    int newX,
    int newY
) {
    x = newX;
    y = newY;
    Invalidate();
}

// Reimplements 0x404cf0: HudUiElement::SetX
void HudUiElement::SetX(
    int newX
) {
    x = newX;
    Invalidate();
}

// Reimplements 0x404d00: HudUiElement::SetY
void HudUiElement::SetY(
    int newY
) {
    y = newY;
    Invalidate();
}

// Reimplements 0x404d20: HudUiElement::SetVisible
void HudUiElement::SetVisible(
    int visible
) {
    if (visible != 0) {
        flags &= 0xffffffefu;
    } else {
        flags |= 0x10u;
    }

    Invalidate();
}

// Reimplements 0x4b41e0: HudUiElement::Update
void HudUiElement::Update(
    float deltaSeconds
) {
    unsigned int currentFlags = flags;

    if ((currentFlags & 0x10) == 0) {
        if ((currentFlags & 0x02) == 0) {
            Draw();
        } else if ((currentFlags & 0x04) != 0) {
            Draw();
            currentFlags = flags & ~0x04u;
            flags = currentFlags;
        } else if ((currentFlags & 0x08) != 0) {
            Draw();
            currentFlags = flags & ~0x08u;
            flags = currentFlags;
        }

        if ((flags & 0x01) != 0) {
            timer -= deltaSeconds;
            if (timer <= 0.0f) {
                SetVisible(0);
            }
        }
    } else if ((currentFlags & 0x02) != 0) {
        if ((currentFlags & 0x04) != 0) {
            DrawBase();
            flags &= ~0x04u;
        } else if ((currentFlags & 0x08) != 0) {
            DrawBase();
            flags &= ~0x08u;
        }
    }
}

// Reimplements 0x4b4280: HudUiElement::SetTimer
void HudUiElement::SetTimer(
    float duration
) {
    unsigned int durationBits = 0;
    memcpy(
        &durationBits,
        &duration,
        sizeof(durationBits)
    );
    memcpy(
        &timer,
        &durationBits,
        sizeof(timer)
    );

    if (duration >= 0.0f) {
        flags |= 0x01u;
    } else {
        flags = (flags & ~0x01u) | 0x10u;
    }
}

// Reimplements 0x4b4190: HudUiElement::SetBltSourceAndClipRect
void HudUiElement::SetBltSourceAndClipRect(
    void *bltSourceOrNull,
    const HudUiRect *rectOrNull
) {
    bltSource = bltSourceOrNull;
    SetClipRect(rectOrNull);
}

// Reimplements 0x4b41b0: HudUiElement::SetClipRect
void HudUiElement::SetClipRect(
    const HudUiRect *rect
) {
    if (rect == 0) {
        return;
    }

    clipRect = *rect;
}

// Reimplements 0x4bcd40: HudUiPanel::SetClip
void HudUiPanel::SetClip(
    void *bltSourceOrNull,
    const HudUiRect *rectOrNull
) {
    bltSource = bltSourceOrNull;
    if (rectOrNull != 0) {
        clipRect = *rectOrNull;
    }

    Invalidate();
}

// Reimplements 0x4b42c0: HudUiElement::GetRect
void HudUiElement::GetRect(
    HudUiRect *outRect
) {
    const int rectX = GetX();
    outRect->left = rectX;
    outRect->right = rectX;

    const int rectY = GetY();
    outRect->top = rectY;
    outRect->bottom = rectY;
}

// Reimplements 0x404d10: HudUiElement::HitTestTrue (D:\Proj\Battlesport\hud.cpp)
// BN returns via AL only (`mov al, 1`); ignore hit-test coordinates.
unsigned char HudUiElement::HitTestTrue(
    int px,
    int py
) {
    (void)px;
    (void)py;
    return 1;
}

// Reimplements 0x404d50: HudUiElement::GetX
int HudUiElement::GetX() {
    return x;
}

// Reimplements 0x404d60: HudUiElement::GetY
int HudUiElement::GetY() {
    return y;
}

void HudUiElement::OnHoverRepeat() {}

HudUiRect * HudUiElement::GetBoundsRectOrNull() {
    return 0;
}

void HudUiElement::OnActivate() {}

void HudUiElement::OnClearBinding() {}

void HudUiElement::ShowPreview() {}

void HudUiElement::HidePreview() {}

void HudUiElement::OnBeginCapture() {}

void HudUiElement::OnEndCapture() {}

void HudUiElement::OnPointerButtonState(
    int,
    int
) {}

void HudUiElement::OnCapturedPrimaryRelease() {}

int HudUiElement::ShouldHandleInput(
    HudUiBackground *,
    int
) {
    return 1;
}

void HudUiElement::AfterInputUpdate(
    HudUiBackground *,
    int
) {}

int HudUiElement::HitTest(
    int px,
    int py
) {
    return HitTestTrue(
        px,
        py
    );
}

void HudUiElement::EnableWordWrapWithRect(
    const HudUiRect *
) {}

void HudUiElement::GetTextRect(
    HudUiRect *outRect
) {
    GetRect(outRect);
}

void HudUiElement::SetTextFmt(
    const char *,
    ...
) {}

void HudUiElement::UpdateTextBoundsFromContent() {}

HGDIOBJ HudUiElement::GetFont() {
    return 0;
}

void HudUiElement::SetFont(
    const char *,
    int,
    int,
    int,
    int,
    int,
    int
) {}

void HudUiElement::SetFontHandle(
    HGDIOBJ
) {}

void HudUiElement::SetTextFmtV(
    const char *,
    va_list
) {}

void HudUiElement::SetText(
    const char *
) {}

void HudUiElement::RebuildTextRect() {}

void HudUiElement::RefreshState() {}

int HudUiElement::LoadFromZrd(
    zReader::Node *,
    HudUiBackground *
) {
    return 0;
}

void HudUiElement::PostLoadFromZrd() {}

int HudUiElement::OnRawKeyboardChar(
    int
) {
    return 0;
}

int HudUiElement::OnAcceptForwardToCommit() {
    return CommitAndGetValue();
}

int HudUiElement::CommitAndGetValue() {
    return 0;
}

// Reimplements 0x4bffb0: HudUiPrimitiveBindTarget::SetSegmentEndpoints (HudUiBackground.cpp)
void HudUiPrimitiveBindTarget::SetSegmentEndpoints(
    int startX,
    int startY,
    int newEndX,
    int newEndY
) {
    SetPos(
        startX,
        startY
    );
    endX = newEndX;
    endY = newEndY;
}

// Reimplements 0x4bc480: HudUiCircle::Constructor
HudUiCircle * HudUiCircle::Constructor(
    int x,
    int y,
    int circleRadius,
    unsigned int circleColor565
) {
    HudUiElement::Constructor(
        x,
        y
    );
    radius = circleRadius;
    const unsigned int radiusBits = (unsigned int)(circleRadius);
    radiusSquared = (int)(radiusBits * radiusBits);
    color565 = circleColor565;
    return this;
}

// Reimplements 0x4bc4c0: HudUiCircle::DrawDirty
void HudUiCircle::DrawDirty() {
    DrawBase();
    zRndr_DrawCircleOutline16_Framebuffer(
        x,
        y,
        radius,
        color565,
        0
    );
}

// Reimplements 0x403c80: HudUiCircle::DrawDirtyForwarder
void HudUiCircle::DrawDirtyForwarder() {
    DrawDirty();
}

// Reimplements 0x404e60: HudUiCircle::HitTest
int HudUiCircle::HitTest(
    int px,
    int py
) {
    return HitTestCore(
        px,
        py
    ) != 0 ? 1 : 0;
}

// Reimplements 0x4bc4e0: HudUiCircle::HitTestCore
unsigned char HudUiCircle::HitTestCore(
    int px,
    int py
) {
    const unsigned int dx = (unsigned int)(px) - (unsigned int)(x);
    const unsigned int dy = (unsigned int)(py) - (unsigned int)(y);
    const unsigned int distanceSquared = dx * dx + dy * dy;
    return (int)(distanceSquared) < radiusSquared ? 1 : 0;
}

// Reimplements 0x4bbfa0: HudUiCompositePanelVector::Clear
void HudUiCompositePanelVector::Clear() {
    for (HudUiCompositePanelEntry *entry = begin; entry != end; ++entry) {
        entry->panel.ScalarDeletingDestructor(0);
    }

    ::operator delete(begin);
    begin = 0;
    end = 0;
    capacityEnd = 0;
}

size_t HudUiCompositePanelVectorCount(
    const HudUiCompositePanelVector &vector
) {
    return vector.begin != 0 ? (size_t)(vector.end - vector.begin) : 0;
}

// Reimplements 0x4bbff0: HudUiCompositePanelVector::InsertCopies
void HudUiCompositePanelVector::InsertCopies(
    HudUiCompositePanelEntry *insertPos,
    unsigned int insertCount,
    const HudUiCompositePanelEntry *templateEntry
) {
    if (insertCount == 0) {
        return;
    }

    const size_t size = begin != 0 ? (size_t)(end - begin) : 0;
    const size_t positionIndex = begin != 0 && insertPos != 0 ? (size_t)(insertPos - begin) : 0;
    const size_t capacity = begin != 0 ? (size_t)(capacityEnd - begin) : 0;
    const size_t tailCount = size - positionIndex;

    if (size + insertCount <= capacity) {
        if (tailCount >= insertCount) {
            HudUiCompositePanelEntry *source = end - insertCount;
            HudUiCompositePanelEntry *dest = end;
            while (source != end) {
                dest->ConstructorCopy(source);
                ++source;
                ++dest;
            }

            source = end - insertCount;
            dest = end;
            while (source != begin + positionIndex) {
                --source;
                --dest;
                dest->AssignCopy(source);
            }

            for (unsigned int i = 0; i < insertCount; ++i) {
                begin[positionIndex + i].AssignCopy(templateEntry);
            }
        } else {
            HudUiCompositePanelEntry *dest = end;
            for (unsigned int i = 0; i < insertCount - tailCount; ++i) {
                dest->ConstructorCopy(templateEntry);
                ++dest;
            }

            for (HudUiCompositePanelEntry *source = begin + positionIndex; source != end;
                ++source, ++dest) {
                dest->ConstructorCopy(source);
            }

            for (HudUiCompositePanelEntry *entry = begin + positionIndex; entry != end; ++entry) {
                entry->AssignCopy(templateEntry);
            }
        }

        end += insertCount;
        return;
    }

    const size_t growth = insertCount < size ? size : insertCount;
    const size_t newCapacity = size + growth;
    HudUiCompositePanelEntry *const newBegin = (HudUiCompositePanelEntry *)(::operator new(
        newCapacity * sizeof(HudUiCompositePanelEntry)
    ));
    HudUiCompositePanelEntry *dest = newBegin;

    for (size_t prefixIndex = 0; prefixIndex < positionIndex; ++prefixIndex, ++dest) {
        dest->ConstructorCopy(&begin[prefixIndex]);
    }

    for (unsigned int insertIndex = 0; insertIndex < insertCount; ++insertIndex, ++dest) {
        dest->ConstructorCopy(templateEntry);
    }

    for (size_t suffixIndex = positionIndex; suffixIndex < size; ++suffixIndex, ++dest) {
        dest->ConstructorCopy(&begin[suffixIndex]);
    }

    for (HudUiCompositePanelEntry *entry = begin; entry != end; ++entry) {
        entry->panel.Destructor();
    }

    ::operator delete(begin);
    begin = newBegin;
    end = newBegin + size + insertCount;
    capacityEnd = newBegin + newCapacity;
}

// Reimplements 0x4bb790: HudUiCompositePanel::ConstructorWithEntryCount
HudUiCompositePanel * HudUiCompositePanel::ConstructorWithEntryCount(
    int entryCount
) {
    HudUiPanel::ConstructorDefault(
        0,
        0,
        0
    );

    activeEntryCount = 0;
    entryVector.allocatorStorage = 0;
    entryVector.begin = 0;
    entryVector.end = 0;
    entryVector.capacityEnd = 0;

    HudUiCompositePanelEntry templateEntry;
    entryVector.InsertCopies(
        entryVector.end,
        (unsigned int)(entryCount),
        &templateEntry
    );

    SetTextFmt("");
    LayoutEntries(
        0,
        0
    );
    ResizeEntryVectorAndRelayout(entryCount);
    SetVisible(1);
    return this;
}

// Reimplements 0x403e20: HudUiCompositePanel::Destructor
void HudUiCompositePanel::Destructor() {
    for (HudUiCompositePanelEntry *entry = entryVector.begin; entry != entryVector.end; ++entry) {
        entry->panel.Destructor();
    }

    ::operator delete(entryVector.begin);
    entryVector.begin = 0;
    entryVector.end = 0;
    entryVector.capacityEnd = 0;

    HudUiPanel::Destructor();
}

// Reimplements 0x4bb960: HudUiCompositePanel::ScalarDeletingDestructor
HudUiElement * HudUiCompositePanel::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4bb980: HudUiCompositePanel::Update
void HudUiCompositePanel::Update(
    float deltaSeconds
) {
    if ((flags & 0x10u) != 0) {
        return;
    }

    for (unsigned int index = 0;
        index < (unsigned int)(HudUiCompositePanelVectorCount(entryVector));
        ++index) {
        HudUiCompositePanelEntry *const entry = &entryVector.begin[index];
        entry->panel.TickFlash(deltaSeconds);
    }
}

// Reimplements 0x4bb9f0: HudUiCompositePanel::LayoutEntries
void HudUiCompositePanel::LayoutEntries(
    int x,
    int y
) {
    this->x = x;
    this->y = y;
    Invalidate();

    const int entryHeight = QueryTextHeight();
    int yOffset = 0;
    for (HudUiCompositePanelEntry *entry = entryVector.begin; entry != entryVector.end; ++entry) {
        entry->panel.SetPos(
            GetX(),
            GetY() + yOffset
        );
        yOffset += entryHeight;
    }
}

// Reimplements 0x4bbe90: HudUiCompositePanel::ReapplyEntryCount
void HudUiCompositePanel::ReapplyEntryCount() {
    ResizeEntryCount(
        0,
        (int)(HudUiCompositePanelVectorCount(entryVector))
    );
}

// Reimplements 0x4bbed0: HudUiCompositePanel::ResizeEntryCount
void HudUiCompositePanel::ResizeEntryCount(
    int oldCount,
    int entryCount
) {
    int activeCount = oldCount;
    if (activeCount > entryCount) {
        activeCount = entryCount;
    }
    if (activeCount < 0) {
        activeCount = 0;
    }

    const int vectorCount = (int)(HudUiCompositePanelVectorCount(entryVector));
    if (entryCount > vectorCount) {
        entryCount = vectorCount;
    }

    {
        for (int index = activeCount; index < entryCount; ++index) {
            HudUiCompositePanelEntry *const entry = &entryVector.begin[index];
            entry->panel.SetTextFmt("");
            entry->panel.SetVisible(0);
        }
    }

    activeEntryCount = activeCount;
}

// Reimplements 0x4bbaa0: HudUiCompositePanel::SetTextFmt
void HudUiCompositePanel::SetTextFmt(
    const char *format,
    ...
) {
    va_list args;
    va_start(
        args,
        format
    );
    SetTextFmtV(
        format,
        args
    );
    va_end(args);
}

// Reimplements 0x4bbac0: HudUiCompositePanel::SetTextFmtV
void HudUiCompositePanel::SetTextFmtV(
    const char *format,
    va_list args
) {
    HudUiCompositePanelEntry *const entry = &entryVector.begin[activeEntryCount];
    entry->panel.SetTextFmtV(
        format,
        args
    );
    entry->panel.SetVisible(1);
    ScrollHistory();
}

// Reimplements 0x4bbb20: HudUiCompositePanel::ScrollHistory
void HudUiCompositePanel::ScrollHistory() {
    ++activeEntryCount;

    if ((unsigned int)(activeEntryCount) >=
        (unsigned int)(HudUiCompositePanelVectorCount(entryVector))) {
        {
            for (unsigned int index = 0;
                index < (unsigned int)(HudUiCompositePanelVectorCount(entryVector)) - 1;
                ++index) {
                HudUiCompositePanelEntry *const current = &entryVector.begin[index];
                HudUiCompositePanelEntry *const next = &entryVector.begin[index + 1];
                current->panel.SetText(next->panel.GetLastTextPtr());
            }
        }
        --activeEntryCount;
    }

    Invalidate();
}

// Reimplements 0x4bbbe0: HudUiCompositePanel::SetFont
void HudUiCompositePanel::SetFont(
    const char *faceName,
    int height,
    int weight,
    int width,
    int italic,
    int charSet,
    int pitchAndFamily
) {
    for (unsigned int index = 0;
        index < (unsigned int)(HudUiCompositePanelVectorCount(entryVector));
        ++index) {
        HudUiCompositePanelEntry *const entry = &entryVector.begin[index];
        entry->panel.SetFont(
            faceName,
            height,
            weight,
            width,
            italic,
            charSet,
            pitchAndFamily
        );
    }

    HudUiPanel::SetFont(
        faceName,
        height,
        weight,
        width,
        italic,
        charSet,
        pitchAndFamily
    );

    SetPos(
        GetX(),
        GetY()
    );
}

// Reimplements 0x4bbca0: HudUiCompositePanel::ResizeEntryVectorAndRelayout
void HudUiCompositePanel::ResizeEntryVectorAndRelayout(
    int entryCount
) {
    const int oldCount = (int)(HudUiCompositePanelVectorCount(entryVector));

    if (entryCount != oldCount) {
        HudUiCompositePanelEntry templateEntry;

        if (entryCount > oldCount) {
            entryVector.InsertCopies(
                entryVector.end,
                (unsigned int)(entryCount - oldCount),
                &templateEntry
            );
        } else {
            HudUiCompositePanelEntry *const newEnd = entryVector.begin + entryCount;
            for (HudUiCompositePanelEntry *entry = newEnd; entry != entryVector.end; ++entry) {
                entry->panel.Destructor();
            }
            entryVector.end = newEnd;
        }

        ResizeEntryCount(
            oldCount,
            entryCount
        );
    } else {
        ReapplyEntryCount();
    }

    LayoutEntries(
        HudUiElement::GetX(),
        HudUiElement::GetY()
    );
}

// Reimplements 0x4bc3a0: HudUiCompositePanelEntry::AssignCopy
HudUiCompositePanelEntry * HudUiCompositePanelEntry::AssignCopy(
    const HudUiCompositePanelEntry *source
) {
    panel.ConstructorCopy(&source->panel);
    panel.flashCountdown = source->panel.flashCountdown;
    panel.flashResetValue = source->panel.flashResetValue;
    panel.flashAltColor0 = source->panel.flashAltColor0;
    panel.flashAltColor1 = source->panel.flashAltColor1;
    panel.flashEnabled = source->panel.flashEnabled;
    panel.flashMode = source->panel.flashMode;
    panel.flashDirectionSign = source->panel.flashDirectionSign;
    return this;
}

// Reimplements 0x4bc410: HudUiCompositePanelEntry::ConstructorCopy
HudUiCompositePanelEntry * HudUiCompositePanelEntry::ConstructorCopy(
    const HudUiCompositePanelEntry *source
) {
    panel.CopyConstructCore(&source->panel);
    panel.flashCountdown = source->panel.flashCountdown;
    panel.flashResetValue = source->panel.flashResetValue;
    panel.flashAltColor0 = source->panel.flashAltColor0;
    panel.flashAltColor1 = source->panel.flashAltColor1;
    panel.flashEnabled = source->panel.flashEnabled;
    panel.flashMode = source->panel.flashMode;
    panel.flashDirectionSign = source->panel.flashDirectionSign;
    return this;
}

// Reimplements 0x4bc320: HudUiCompositePanelEntry::ConstructorCopyRange
HudUiCompositePanelEntry *__fastcall HudUiCompositePanelEntry::ConstructorCopyRange(
    const HudUiCompositePanelEntry *sourceBegin,
    const HudUiCompositePanelEntry *sourceEnd,
    HudUiCompositePanelEntry *destBegin
) {
    HudUiCompositePanelEntry *dest = destBegin;
    for (const HudUiCompositePanelEntry *source = sourceBegin; source != sourceEnd;
        ++source, ++dest) {
        dest->panel.ConstructorCopy(&source->panel);
        dest->panel.flashCountdown = source->panel.flashCountdown;
        dest->panel.flashResetValue = source->panel.flashResetValue;
        dest->panel.flashAltColor0 = source->panel.flashAltColor0;
        dest->panel.flashAltColor1 = source->panel.flashAltColor1;
        dest->panel.flashEnabled = source->panel.flashEnabled;
        dest->panel.flashMode = source->panel.flashMode;
        dest->panel.flashDirectionSign = source->panel.flashDirectionSign;
    }

    return dest;
}

// Reimplements 0x4bb0c0: HudUiFlashPanel::ComputeFlashBlendColor
unsigned int __fastcall HudUiFlashPanel::ComputeFlashBlendColor(
    unsigned int color0,
    unsigned int color1,
    float blend
) {
    const double blendValue = (double)(blend);
    if (!(blendValue >= 0.001)) {
        return color0;
    }
    if (blendValue > 0.999) {
        return color1;
    }

    const double inverseBlend = 1.0 - blendValue;
    const unsigned int blue = (unsigned int)((int)((double)(color0 & 0xffu) * inverseBlend +
                                                   (double)(color1 & 0xffu) * blendValue)) &
                              0xffu;
    const unsigned int green = (unsigned int)((int)((double)((color0 >> 8) & 0xffu) * inverseBlend +
                                                    (double)((color1 >> 8) & 0xffu) * blendValue)) &
                               0xffu;
    const unsigned int red = (unsigned int)((int)((double)((color0 >> 16) & 0xffu) * inverseBlend +
                                                  (double)((color1 >> 16) & 0xffu) * blendValue)) &
                             0xffu;
    return (red << 16) | (green << 8) | blue;
}

// Reimplements 0x4bc780: HudUiContainer::HudUiContainer
HudUiContainer::HudUiContainer() {
    HudUiContainer *const container = this;
    container->SetEnabled(0);
    childHead = 0;
    childTail = 0;
}

// Reimplements 0x4bc7b0: HudUiContainer::DestructorCore
void HudUiContainer::DestructorCore() {
}

// Reimplements 0x40d9d0: HudUiContainer::SetEnabled
void HudUiContainer::SetEnabled(
    int enabledValue
) {
    enabled = enabledValue;
}

// Reimplements 0x4bc7c0: HudUiContainer::AddChild
int HudUiContainer::AddChild(
    HudUiElement *child
) {
    if (childHead != 0 && childTail != 0) {
        childTail->next = child;
        childTail = child;
    } else {
        childTail = child;
        childHead = child;
    }

    child->next = 0;
    child->parent = this;
    return 1;
}

// Reimplements 0x4bc810: HudUiContainer::FindChildWithPrev
int HudUiContainer::FindChildWithPrev(
    HudUiElement *child,
    HudUiElement **previousOut
) {
    HudUiElement *previous = childHead;
    if (previous == 0) {
        return 0;
    }

    if (child == previous) {
        *previousOut = 0;
        return 1;
    }

    while (previous != 0) {
        HudUiElement *const current = previous->next;
        if (current == child) {
            if (previousOut != 0) {
                *previousOut = previous;
            }

            return 1;
        }

        previous = current;
    }

    return 0;
}

// Reimplements 0x4bc860: HudUiContainer::RemoveChild
int HudUiContainer::RemoveChild(
    HudUiElement *child
) {
    HudUiElement *previous = child;
    if (FindChildWithPrev(
        child,
        &previous
    ) == 0) {
        return 0;
    }

    if (previous != 0) {
        previous->next = child->next;
        if (child == childTail) {
            childTail = previous;
        }
    } else {
        childHead = child->next;
        if (child == childTail) {
            childTail = child->next;
        }
    }

    child->next = 0;
    child->parent = 0;
    return 1;
}

/**
 * Reimplements 0x4bc8d0: HudUiContainer::SetChildFlags.
 *
 * Purpose: apply a shared child flag mask to every child while preserving each
 * child's hidden/disabled bit 0x10.
 *
 * Evidence: BN assembly at 0x4bc8d0 walks HudUiContainer::childHead through
 * HudUiElement::next, writes childFlags directly when bit 0x10 is clear, and
 * writes childFlags|0x10 when the existing child flags preserve that bit.
 */
void HudUiContainer::SetChildFlags(
    unsigned int childFlags
) {
    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        child->flags = childFlags | (child->flags & 0x10u);
    }
}

// Reimplements 0x4bc900: HudUiContainer::UpdateAll
void HudUiContainer::UpdateAll(
    float deltaSeconds
) {
    if (enabled == 0) {
        return;
    }

    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        child->Update(deltaSeconds);
    }
}

// Reimplements 0x4ba3a0: HudUiContainer::InvalidateChildren
void HudUiContainer::InvalidateChildren() {
    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        child->Invalidate();
    }
}

// Reimplements 0x42ee40: HudUiBackgroundContainer::SetEnabled
void HudUiBackgroundContainer::SetEnabled(
    int enabled
) {
    HudUiContainer::SetEnabled(enabled);
}

inline HudUiCreditsBackButton::HudUiCreditsBackButton() : HudUiZrdWidget() {
}

inline HudUiCreditsBackButton::~HudUiCreditsBackButton() {
    HudUiZrdWidget::DestructorCore();
}

inline HudUiCreditsQuitButton::HudUiCreditsQuitButton() : HudUiZrdWidget() {
}

inline HudUiCreditsQuitButton::~HudUiCreditsQuitButton() {
    HudUiZrdWidget::DestructorCore();
}

inline HudUiZrdScrollingText::HudUiZrdScrollingText() : HudUiZrdWidget() {
}

inline HudUiZrdScrollingText::~HudUiZrdScrollingText() {
    HudUiPanelSpan *row = rows.begin;
    while (row != rows.end) {
        HudUiPanelLayoutEntry *entry = row->begin;
        while (entry != row->end) {
            HudUiPanel::DestructorCallback(&entry->panel);
            ++entry;
        }

        ::operator delete(row->begin);
        row->begin = 0;
        row->end = 0;
        row->cap = 0;
        ++row;
    }

    ::operator delete(rows.begin);
    rows.begin = 0;
    rows.end = 0;
    rows.cap = 0;

    HudUiZrdWidget::DestructorCore();
}

// Reimplements 0x409040: HudUiCreditsPanel::HudUiCreditsPanel
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
HudUiCreditsPanel::HudUiCreditsPanel() : HudUiBackground() {
    fadeProgress = 0.0f;
    fadeStep = 0.05f;
    HudUiZrdScrollingText *const screen = &creditsScreen;

    zReader::Node *const loadedSection = HudUiBackground::LoadFromZrd(
        "dialog.zrd",
        "CREDITSPANEL",
        0
    );
    if (loadedSection != 0) {
        if (g_RecoilApp_QuitAfterCredits != 0) {
            HudUiBackground::BindWidgetByName(
                loadedSection,
                (HudUiWidget *)(&quitButton),
                "QUIT"
            );
        } else {
            HudUiBackground::BindWidgetByName(
                loadedSection,
                (HudUiWidget *)(&backButton),
                "BACK"
            );
        }

        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(screen),
            "CREDITS_SCREEN"
        );
        HudUiBackground::FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    unsigned int screenFlags = 0;
    screenFlags = (unsigned char)(screen->flags);
    screen->flags = screenFlags & 0x10u;
}

// Reimplements 0x409550: HudUiZrdScrollingText::OnActivateResetOwnerFade
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void HudUiZrdScrollingText::OnActivateResetOwnerFade() {
    ((HudUiCreditsPanel *)(owner))->fadeProgress = 0.0f;
}

void HudUiZrdScrollingText::OnActivate() {
    OnActivateResetOwnerFade();
}

// Reimplements 0x409570: HudUiZrdScrollingText::LoadFromZrd
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
int HudUiZrdScrollingText::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiZrdWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );

    zReader::Node *const rectNode = zReader_GetNamedNode(
        zrdSection,
        "RECT"
    );
    zReader::Node *const rectBase = ZrdArrayBase(rectNode);
    if (rectBase != 0) {
        zReader::Node *const topLeft = ZrdArrayBase(&rectBase[1]);
        zReader::Node *const bottomRight = ZrdArrayBase(&rectBase[2]);
        rect.left = ZrdArrayInt(
            topLeft,
            1,
            0
        ) + originX;
        rect.top = ZrdArrayInt(
            topLeft,
            2,
            0
        ) + originY;
        rect.right = ZrdArrayInt(
            bottomRight,
            1,
            0
        ) + originX;
        rect.bottom = ZrdArrayInt(
            bottomRight,
            2,
            0
        ) + originY;
    }

    zReader::Node *const scrollRateNode = zReader_GetNamedNode(
        zrdSection,
        "SCROLL_RATE"
    );
    if (scrollRateNode != 0) {
        ((HudUiCreditsPanel *)(ownerDialog))->fadeStep = scrollRateNode->value.f32;
    }

    zReader::Node *const scrollingTextNode = zReader_GetNamedNode(
        zrdSection,
        "SCROLLING_TEXT"
    );
    zReader::Node *const scrollingTextBase = ZrdArrayBase(scrollingTextNode);
    if (scrollingTextBase == 0) {
        return 1;
    }

    HudUiPanelSpan templateSpan;
    templateSpan.allocatorProxy = 0;
    templateSpan.begin = 0;
    templateSpan.end = 0;
    templateSpan.cap = 0;

    const int rowCount = ZrdArrayCount(scrollingTextBase);
    for (int rowIndex = 1; rowIndex < rowCount; ++rowIndex) {
        zReader::Node *const rowBase = ZrdArrayBase(&scrollingTextBase[rowIndex]);
        const int labelCount = ZrdArrayCount(rowBase);

        HudUiPanelLayoutEntry *const resetEnd = HudUiPanelLayoutEntry::CopyAssignRange(
            templateSpan.end,
            templateSpan.end,
            templateSpan.begin
        );
        HudUiPanelLayoutEntry::DestroyRange(
            resetEnd,
            templateSpan.end
        );
        templateSpan.end = resetEnd;

        for (int labelIndex = 1; labelIndex < labelCount; ++labelIndex) {
            zReader::Node *const labelBase = ZrdArrayBase(&rowBase[labelIndex]);
            const char *const key = ZrdArrayString(
                labelBase,
                1
            );
            const char *const text = zLoc::ResolveMessageKeyOrFallback(key);
            const int layoutX = ZrdArrayInt(
                labelBase,
                2,
                0
            );
            const int layoutY = ZrdArrayInt(
                labelBase,
                3,
                0
            );
            const int styleIndex = ZrdArrayInt(
                labelBase,
                4,
                0
            );

            HudUiPanelLayoutEntry templateEntry;
            templateEntry.panel.ConstructorDefault(
                0,
                0,
                0
            );
            templateEntry.panel.SetTextFmt(
                "%s",
                text
            );
            templateEntry.layoutX = layoutX;
            templateEntry.layoutY = layoutY;

            ApplyHudFontStyleTextOnly(
                &templateEntry.panel,
                HudUiZrdOwnerFontStyle(owner, styleIndex)
            );
            templateSpan.InsertN(
                templateSpan.end,
                1,
                &templateEntry
            );
            templateEntry.panel.Destructor();
        }

        rows.InsertN(
            rows.end,
            1,
            &templateSpan
        );
    }

    totalHeight = 0;
    HudUiPanelSpan *row = rows.begin;
    while (row != rows.end) {
        int rowHeight = 0;
        HudUiPanelLayoutEntry *entry = row->begin;
        while (entry != row->end) {
            const int entryBottom = entry->panel.QueryTextHeight() + entry->layoutY;
            if (entryBottom > rowHeight) {
                rowHeight = entryBottom;
            }

            ++entry;
        }

        entry = row->begin;
        while (entry != row->end) {
            entry->layoutY += totalHeight;
            ++entry;
        }

        totalHeight += rowHeight;
        ++row;
    }

    HudUiPanelLayoutEntry *entry = templateSpan.begin;
    while (entry != templateSpan.end) {
        entry->panel.Destructor();
        ++entry;
    }

    ::operator delete(templateSpan.begin);
    return 1;
}

// Reimplements 0x409410: HudUiZrdScrollingText::Update
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void HudUiZrdScrollingText::Update(
    float deltaSeconds
) {
    HudUiElement::Update(deltaSeconds);

    HudUiPanelSpan *row = rows.begin;
    while (row != rows.end) {
        HudUiPanelLayoutEntry *entry = row->begin;
        while (entry != row->end) {
            entry->panel.Update(deltaSeconds);
            ++entry;
        }

        ++row;
    }
}

// Reimplements 0x409470: HudUiZrdScrollingText::UpdateScrollPositions
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void HudUiZrdScrollingText::UpdateScrollPositions(
    float scrollProgress
) {
    const int left = rect.left;
    const int scrollY = (int)((float)(rect.top - totalHeight) * scrollProgress +
                              (1.0f - scrollProgress) * (float)(rect.bottom));

    HudUiPanelSpan *row = rows.begin;
    while (row != rows.end) {
        HudUiPanelLayoutEntry *entry = row->begin;
        while (entry != row->end) {
            const int y = entry->layoutY + scrollY;
            entry->panel.SetPos(
                entry->layoutX + left,
                y
            );
            if (y > rect.top && y + entry->panel.QueryTextHeight() < rect.bottom) {
                entry->panel.SetVisible(1);
            } else {
                entry->panel.SetVisible(0);
            }

            ++entry;
        }

        ++row;
    }
}

// Reimplements 0x409ef0: HudUiPanel::DestructorCallback
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void __stdcall HudUiPanel::DestructorCallback(
    HudUiPanel *panel
) {
    panel->DestructorThunk();
}

// Reimplements 0x4091e0: HudUiZrdScrollingText::Destructor
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void HudUiZrdScrollingText::Destructor() {
    this->~HudUiZrdScrollingText();
}

// Reimplements 0x409360: HudUiZrdScrollingText::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
HudUiElement * HudUiZrdScrollingText::ScalarDeletingDestructor(
    unsigned int flags
) {
    HudUiZrdScrollingText *self = this;
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(self);
    }

    return self;
}

// Reimplements 0x409380: HudUiCreditsPanel::UpdateFadeAndExit
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void HudUiCreditsPanel::UpdateFadeAndExit(
    float deltaSeconds
) {
    creditsScreen.UpdateScrollPositions(fadeProgress);
    fadeProgress += fadeStep * deltaSeconds;
    HudUiBackground::Update(deltaSeconds);

    if (fadeProgress < 1.0f) {
        return;
    }

    if (g_RecoilApp_QuitAfterCredits != 0) {
        g_RecoilApp.QueueExitCurrentState(1);
        g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_leaveNetworkState,
            0
        );
        return;
    }

    g_RecoilApp.QueueExitCurrentState(0);
}

void HudUiCreditsPanel::Update(
    float deltaSeconds
) {
    UpdateFadeAndExit(deltaSeconds);
}

// Reimplements 0x4092a0: HudUiCreditsPanel::Destructor
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void HudUiCreditsPanel::Destructor() {
    HudUiPanelSpan *row = creditsScreen.rows.begin;
    while (row != creditsScreen.rows.end) {
        row->DestroyAndFree();
        ++row;
    }

    ::operator delete(creditsScreen.rows.begin);
    creditsScreen.rows.begin = 0;
    creditsScreen.rows.end = 0;
    creditsScreen.rows.cap = 0;

    creditsScreen.HudUiZrdWidget::DestructorCore();
    quitButton.DestructorCore();
    backButton.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

// Reimplements 0x4091c0: HudUiCreditsPanel::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
HudUiCreditsPanel * HudUiCreditsPanel::ScalarDeletingDestructor(
    unsigned int flags
) {
    HudUiCreditsPanel *self = this;
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(self);
    }

    return self;
}

// Reimplements 0x40a210: HudUiPanelLayoutEntry::CopyConstruct
// (D:\Proj\Battlesport\HudUiPanel.cpp)
HudUiPanelLayoutEntry * HudUiPanelLayoutEntry::CopyConstruct(
    const HudUiPanelLayoutEntry *source
) {
    panel.CopyConstructCore(&source->panel);
    layoutX = source->layoutX;
    layoutY = source->layoutY;
    return this;
}

// Reimplements 0x40a1e0: HudUiPanelLayoutEntry::CopyAssign
// (D:\Proj\Battlesport\HudUiPanel.cpp)
HudUiPanelLayoutEntry * HudUiPanelLayoutEntry::CopyAssign(
    const HudUiPanelLayoutEntry *source
) {
    panel.ConstructorCopy(&source->panel);
    layoutX = source->layoutX;
    layoutY = source->layoutY;
    return this;
}

// Reimplements 0x40a170: HudUiPanelLayoutEntry::CopyAssignRange
// (D:\Proj\Battlesport\HudUiPanel.cpp)
HudUiPanelLayoutEntry *__fastcall HudUiPanelLayoutEntry::CopyAssignRange(
    const HudUiPanelLayoutEntry *sourceStart,
    const HudUiPanelLayoutEntry *sourceEnd,
    HudUiPanelLayoutEntry *dest
) {
    HudUiPanelLayoutEntry *out = dest;
    const HudUiPanelLayoutEntry *source = sourceStart;
    while (source != sourceEnd) {
        out->panel.ConstructorCopy(&source->panel);
        out->layoutX = source->layoutX;
        out->layoutY = source->layoutY;
        ++source;
        ++out;
    }

    return out;
}

// Reimplements 0x409b60: HudUiPanelLayoutEntry::DestroyRange
// (D:\Proj\Battlesport\HudUiPanel.cpp)
void __stdcall HudUiPanelLayoutEntry::DestroyRange(
    HudUiPanelLayoutEntry *start,
    HudUiPanelLayoutEntry *end
) {
    HudUiPanelLayoutEntry *entry = start;
    while (entry != end) {
        entry->panel.DestructorThunk();
        ++entry;
    }
}

// Reimplements 0x409910: HudUiPanelSpan::Clear
// (D:\Proj\Battlesport\HudUiPanel.cpp)
void HudUiPanelSpan::Clear() {
    HudUiPanelLayoutEntry *entry = begin;
    while (entry != end) {
        entry->panel.Destructor();
        ++entry;
    }

    ::operator delete(begin);
    begin = 0;
    end = 0;
    cap = 0;
}

// Reimplements 0x40a240: HudUiPanelSpan::CopyInit
// (D:\Proj\Battlesport\HudUiPanel.cpp)
HudUiPanelSpan * HudUiPanelSpan::CopyInit(
    const HudUiPanelSpan *source
) {
    allocatorProxy = (allocatorProxy & 0xffffff00) | (source->allocatorProxy & 0xff);

    const size_t count = source->begin != 0 ? (size_t)(source->end - source->begin) : 0;
    HudUiPanelLayoutEntry *const newBegin =
        (HudUiPanelLayoutEntry *)(::operator new(count * sizeof(HudUiPanelLayoutEntry)));
    HudUiPanelLayoutEntry *dest = newBegin;

    const HudUiPanelLayoutEntry *sourceEntry = source->begin;
    while (sourceEntry != source->end) {
        dest->panel.CopyConstructCore(&sourceEntry->panel);
        dest->layoutX = sourceEntry->layoutX;
        dest->layoutY = sourceEntry->layoutY;
        ++sourceEntry;
        ++dest;
    }

    begin = newBegin;
    end = dest;
    cap = dest;
    return this;
}

// Reimplements 0x40a300: HudUiPanelSpan::CopyFrom
// (D:\Proj\Battlesport\HudUiPanel.cpp)
HudUiPanelSpan * HudUiPanelSpan::CopyFrom(
    const HudUiPanelSpan *source
) {
    if (this == source) {
        return this;
    }

    const size_t sourceCount = source->begin != 0 ? (size_t)(source->end - source->begin) : 0;
    const size_t currentCount = begin != 0 ? (size_t)(end - begin) : 0;
    const size_t capacity = begin != 0 ? (size_t)(cap - begin) : 0;

    if (sourceCount <= currentCount) {
        HudUiPanelLayoutEntry *dest = begin;
        const HudUiPanelLayoutEntry *sourceEntry = source->begin;
        while (sourceEntry != source->end) {
            dest->CopyAssign(sourceEntry);
            ++sourceEntry;
            ++dest;
        }

        HudUiPanelLayoutEntry *oldEntry = dest;
        while (oldEntry != end) {
            oldEntry->panel.ScalarDeletingDestructor(0);
            ++oldEntry;
        }

        end = begin + sourceCount;
        return this;
    }

    if (sourceCount <= capacity) {
        HudUiPanelLayoutEntry *dest = begin;
        const HudUiPanelLayoutEntry *sourceEntry = source->begin;
        for (size_t i = 0; i < currentCount; ++i) {
            dest->CopyAssign(sourceEntry);
            ++sourceEntry;
            ++dest;
        }

        while (sourceEntry != source->end) {
            dest->CopyConstruct(sourceEntry);
            ++sourceEntry;
            ++dest;
        }

        end = begin + sourceCount;
        return this;
    }

    HudUiPanelLayoutEntry *oldEntry = begin;
    while (oldEntry != end) {
        oldEntry->panel.DestructorThunk();
        ++oldEntry;
    }

    ::operator delete(begin);

    HudUiPanelLayoutEntry *const newBegin =
        (HudUiPanelLayoutEntry *)(::operator new(sourceCount * sizeof(HudUiPanelLayoutEntry)));
    HudUiPanelLayoutEntry *dest = newBegin;
    const HudUiPanelLayoutEntry *sourceEntry = source->begin;
    while (sourceEntry != source->end) {
        dest->CopyConstruct(sourceEntry);
        ++sourceEntry;
        ++dest;
    }

    begin = newBegin;
    end = dest;
    cap = dest;
    return this;
}

// Reimplements 0x409b90: HudUiPanelSpan::InsertN
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void HudUiPanelSpan::InsertN(
    HudUiPanelLayoutEntry *insertPos,
    unsigned int count,
    const HudUiPanelLayoutEntry *templatePanel
) {
    if (count == 0) {
        return;
    }

    const size_t size = begin != 0 ? (size_t)(end - begin) : 0;
    const size_t positionIndex = begin != 0 && insertPos != 0 ? (size_t)(insertPos - begin) : 0;
    const size_t capacity = begin != 0 ? (size_t)(cap - begin) : 0;
    const size_t tailCount = size - positionIndex;

    if (size + count <= capacity) {
        if (tailCount >= count) {
            HudUiPanelLayoutEntry *source = end - count;
            HudUiPanelLayoutEntry *dest = end;
            while (source != end) {
                dest->CopyConstruct(source);
                ++source;
                ++dest;
            }

            source = end - count;
            dest = end;
            while (source != begin + positionIndex) {
                --source;
                --dest;
                dest->CopyAssign(source);
            }

            for (unsigned int i = 0; i < count; ++i) {
                begin[positionIndex + i].CopyAssign(templatePanel);
            }
        } else {
            HudUiPanelLayoutEntry *dest = end;
            for (unsigned int i = 0; i < count - tailCount; ++i) {
                dest->CopyConstruct(templatePanel);
                ++dest;
            }

            for (HudUiPanelLayoutEntry *source = begin + positionIndex; source != end;
                ++source, ++dest) {
                dest->CopyConstruct(source);
            }

            for (HudUiPanelLayoutEntry *entry = begin + positionIndex; entry != end; ++entry) {
                entry->CopyAssign(templatePanel);
            }
        }

        end += count;
        return;
    }

    const size_t growth = count < size ? size : count;
    const size_t newCapacity = size + growth;
    HudUiPanelLayoutEntry *const newBegin =
        (HudUiPanelLayoutEntry *)(::operator new(newCapacity * sizeof(HudUiPanelLayoutEntry)));
    HudUiPanelLayoutEntry *dest = newBegin;

    for (size_t prefixIndex = 0; prefixIndex < positionIndex; ++prefixIndex, ++dest) {
        dest->CopyConstruct(&begin[prefixIndex]);
    }

    for (unsigned int insertIndex = 0; insertIndex < count; ++insertIndex, ++dest) {
        dest->CopyConstruct(templatePanel);
    }

    for (size_t suffixIndex = positionIndex; suffixIndex < size; ++suffixIndex, ++dest) {
        dest->CopyConstruct(&begin[suffixIndex]);
    }

    HudUiPanelLayoutEntry *entry = begin;
    while (entry != end) {
        entry->panel.DestructorThunk();
        ++entry;
    }

    ::operator delete(begin);
    begin = newBegin;
    end = newBegin + size + count;
    cap = newBegin + newCapacity;
}

// Reimplements 0x409f00: HudUiPanelSpanVec::InsertN
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void HudUiPanelSpanVec::InsertN(
    HudUiPanelSpan *insertPos,
    unsigned int count,
    const HudUiPanelSpan *templateSpan
) {
    if (count == 0) {
        return;
    }

    const size_t size = begin != 0 ? (size_t)(end - begin) : 0;
    const size_t positionIndex = begin != 0 && insertPos != 0 ? (size_t)(insertPos - begin) : 0;
    const size_t capacity = begin != 0 ? (size_t)(cap - begin) : 0;
    const size_t tailCount = size - positionIndex;

    if (size + count <= capacity) {
        if (tailCount >= count) {
            HudUiPanelSpan *source = end - count;
            HudUiPanelSpan *dest = end;
            while (source != end) {
                dest->CopyInit(source);
                ++source;
                ++dest;
            }

            source = end - count;
            dest = end;
            while (source != begin + positionIndex) {
                --source;
                --dest;
                dest->CopyFrom(source);
            }

            for (unsigned int i = 0; i < count; ++i) {
                begin[positionIndex + i].CopyFrom(templateSpan);
            }
        } else {
            HudUiPanelSpan *dest = end;
            for (unsigned int i = 0; i < count - tailCount; ++i) {
                dest->CopyInit(templateSpan);
                ++dest;
            }

            for (HudUiPanelSpan *source = begin + positionIndex; source != end; ++source, ++dest) {
                dest->CopyInit(source);
            }

            for (HudUiPanelSpan *span = begin + positionIndex; span != end; ++span) {
                span->CopyFrom(templateSpan);
            }
        }

        end += count;
        return;
    }

    const size_t growth = count < size ? size : count;
    const size_t newCapacity = size + growth;
    HudUiPanelSpan *const newBegin =
        (HudUiPanelSpan *)(::operator new(newCapacity * sizeof(HudUiPanelSpan)));
    HudUiPanelSpan *dest = newBegin;

    for (size_t prefixIndex = 0; prefixIndex < positionIndex; ++prefixIndex, ++dest) {
        dest->CopyInit(&begin[prefixIndex]);
    }

    for (unsigned int insertIndex = 0; insertIndex < count; ++insertIndex, ++dest) {
        dest->CopyInit(templateSpan);
    }

    for (size_t suffixIndex = positionIndex; suffixIndex < size; ++suffixIndex, ++dest) {
        dest->CopyInit(&begin[suffixIndex]);
    }

    HudUiPanelSpan *span = begin;
    while (span != end) {
        span->Clear();
        ++span;
    }

    ::operator delete(begin);
    begin = newBegin;
    end = newBegin + size + count;
    cap = newBegin + newCapacity;
}

// Reimplements 0x409b20: HudUiPanelSpan::DestroyAndFree
// (D:\Proj\Battlesport\HudUiPanel.cpp)
void HudUiPanelSpan::DestroyAndFree() {
    HudUiPanelLayoutEntry *entry = begin;
    while (entry != end) {
        entry->panel.DestructorThunk();
        ++entry;
    }

    ::operator delete(begin);
    begin = 0;
    end = 0;
    cap = 0;
}

// Reimplements 0x4bc510: HudUiBackgroundContainer::HudUiBackgroundContainer
HudUiBackgroundContainer::HudUiBackgroundContainer(
    int initFlag
) : HudUiContainer() {
    captureTransitionMask = initFlag;
    inputFocusElement = 0;
}

/**
 * Reimplements 0x4bc540: HudUiBackgroundContainer::~HudUiBackgroundContainer.
 * Purpose: Restores the background-container base state and tears down the inherited container.
 */
HudUiBackgroundContainer::~HudUiBackgroundContainer() {
    HudUiContainer::DestructorCore();
}

// Reimplements 0x4bc550: HudUiBackgroundContainer::SetInputFocus
void HudUiBackgroundContainer::SetInputFocus(
    HudUiElement *element
) {
    inputFocusElement = element;
}

// Reimplements 0x4bc560: HudUiBackgroundContainer::GetInputFocus
HudUiElement * HudUiBackgroundContainer::GetInputFocus() {
    return inputFocusElement;
}

// Reimplements 0x4b9540: HudUiBackground::HudUiBackground
// (D:\Proj\Battlesport\hudui_background.cpp)
HudUiBackground::HudUiBackground()
    : HudUiBackgroundContainer(1),
      cursorWidget(0, 1) {
    primaryClipImage = 0;
    capturedCompositeImage = 0;

    {
        for (int index = 0; index < 10; ++index) {
            backgroundSounds[index].sample = 0;
            backgroundSounds[index].volume = 1.0f;
            backgroundSounds[index].playHandle = 0;
        }
    }

    int vmode = 5;
    zOptionEntryPartial *vmodeOption = zGame::Options_FindOption("VMode");
    if (vmodeOption != 0) {
        vmode = vmodeOption->payloadOrBuffer;
    }

    switch (vmode) {
    case 2:
    case 4:
        uiOriginX = 0;
        uiOriginY = -40;
        break;
    case 3:
    case 5:
        uiOriginX = 0;
        uiOriginY = 0;
        break;
    case 6:
        uiOriginX = 0;
        uiOriginY = 60;
        break;
    case 7:
        uiOriginX = 0;
        uiOriginY = 144;
        break;
    }

}

/**
 * Reimplements 0x4b9760: HudUiBackground::~HudUiBackground.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: Releases owned background clip images before compiler-generated member and base cleanup.
 */
HudUiBackground::~HudUiBackground() {
    if (primaryClipImage != 0) {
        zVid_Image::ReleaseIfNotDefault(primaryClipImage);
        primaryClipImage = 0;
    }

    if (capturedCompositeImage != 0) {
        zVid_Image::ReleaseIfNotDefault(capturedCompositeImage);
        capturedCompositeImage = 0;
    }

    cursorWidget.DestructorCore();
}

// Reimplements 0x4ba380: HudUiDialogController::BlitOwnedSurfaceToPrimary
void HudUiDialogController::BlitOwnedSurfaceToPrimary() {
    if (capturedImage != 0) {
        zVid_Image::BlitToActiveTarget(
            capturedImage,
            0,
            0,
            0,
            0
        );
    }
}

unsigned int HudUiReadPackedColor(
    zReader::Node *colorBase
) {
    if (colorBase == 0) {
        return 0;
    }

    zReader::Node *rgbBase = colorBase;
    if (colorBase[1].type == zReader::ZRDR_NODE_ARRAY) {
        rgbBase = ZrdArrayBase(&colorBase[1]);
    }

    const unsigned int red = (unsigned char)(ZrdArrayInt(
        rgbBase,
        1,
        0
    ));
    const unsigned int green = (unsigned char)(ZrdArrayInt(
        rgbBase,
        2,
        0
    ));
    const unsigned int blue = (unsigned char)(ZrdArrayInt(
        rgbBase,
        3,
        0
    ));
    return red | (green << 8) | (blue << 16);
}

// Reimplements 0x4b98d0: HudUiBackground::LoadFromZrd
// (D:\Proj\Battlesport\hudui_background.cpp)
zReader::Node * HudUiBackground::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    zReader::Node *const root = zReader::LoadNodeFromPath(
        zrdPath,
        0,
        0
    );
    loadedRoot = root;
    return LoadZrdAndSection(
        root,
        sectionName,
        capturePrimary
    );
}

// Reimplements 0x4b9900: HudUiBackground::LoadZrdAndSection
// (D:\Proj\Battlesport\hudui_background.cpp)
zReader::Node * HudUiBackground::LoadZrdAndSection(
    zReader::Node *loadedRootNode,
    const char *sectionName,
    int capturePrimary
) {
    zReader::Node *result = 0;
    zVideo::RunPostprocessOnPrimaryBuffer();

    if (capturePrimary == 0) {
        primaryClipImage = zVideo_buff_CaptureSurfaceToImage(0);
    }

    if (loadedRootNode != 0) {
        result = loadedRootNode;

        zReader::Node *const sharedImagePath =
            zReader_GetNamedNode(
                loadedRootNode,
                "SHARED_IMAGE_PATH"
            );
        zReader::Node *const sharedImagePathBase = ZrdArrayBase(sharedImagePath);
        if (sharedImagePathBase != 0) {
            zImage_InitMissionResources(ZrdArrayString(
                sharedImagePathBase,
                1
            ));
        }

        cfgRoot = zReader_GetNamedNode(
            loadedRootNode,
            sectionName
        );
    } else {
        cfgRoot = 0;
    }

    if (cfgRoot != 0) {
        zReader::Node *const imagePath = zReader_GetNamedNode(
            cfgRoot,
            "IMAGE_PATH"
        );
        zReader::Node *const imagePathBase = ZrdArrayBase(imagePath);
        if (imagePathBase != 0) {
            zImage_InitMissionResources(ZrdArrayString(
                imagePathBase,
                1
            ));
        }

        zReader::Node *const fontList = ZrdArrayBase(zReader_GetNamedNode(
            cfgRoot,
            "FONTS"
        ));
        int fontCount = ZrdArrayCount(fontList);
        if (fontCount > 20) {
            fontCount = 20;
        }

        {
            for (int index = 1; index < fontCount; ++index) {
                zReader::Node *const fontSpec = ZrdArrayBase(&fontList[index]);
                if (fontSpec == 0) {
                    continue;
                }

                const int styleIndex = ZrdArrayInt(
                    fontSpec,
                    1,
                    0
                );
                if (styleIndex < 0 || styleIndex >= 20) {
                    continue;
                }

                HudFontStyle &style = fontStyles[styleIndex];
                style.validMarker = 1;
                style.bkColor = 0;
                style.bkMode = 1;
                style.fontName = ZrdArrayString(
                    fontSpec,
                    2
                );
                style.fontSize = ZrdArrayInt(
                    fontSpec,
                    3,
                    0
                );

                zReader::Node *const colors = ZrdArrayBase(&fontSpec[4]);
                style.textColor = HudUiReadPackedColor(colors);
                if (colors != 0 && colors[1].type == zReader::ZRDR_NODE_ARRAY) {
                    style.bkColor = HudUiReadPackedColor(ZrdArrayBase(&colors[2]));
                    style.bkMode = 2;
                }

                if (ZrdArrayCount(fontSpec) >= 6) {
                    style.fontWeight = ZrdArrayInt(
                        fontSpec,
                        5,
                        style.fontWeight
                    );
                }
                if (ZrdArrayCount(fontSpec) >= 7) {
                    style.shadowEnabled = ZrdArrayInt(
                        fontSpec,
                        6,
                        style.shadowEnabled
                    );
                }
                if (ZrdArrayCount(fontSpec) >= 8) {
                    const char *const align = ZrdArrayString(
                        fontSpec,
                        7
                    );
                    if (align == 0 || strcmp(
                        align,
                        "LEFT"
                    ) == 0) {
                        style.alignMode = 0;
                    } else if (strcmp(
                        align,
                        "RIGHT"
                    ) == 0) {
                        style.alignMode = 2;
                    } else if (strcmp(
                        align,
                        "CENTER"
                    ) == 0) {
                        style.alignMode = 1;
                    }
                }
            }
        }

        zReader::Node *const imageList =
            ZrdArrayBase(zReader_GetNamedNode(
                cfgRoot,
                "BACKGROUND_IMAGES"
            ));
        int imageCount = ZrdArrayCount(imageList);
        if (imageCount > 20) {
            imageCount = 20;
        }

        {
            for (int index = 1; index < imageCount; ++index) {
                zReader::Node *const imageSpec = ZrdArrayBase(&imageList[index]);
                if (imageSpec == 0) {
                    continue;
                }

                HudUiWidget &child = backgroundImageWidgets[index - 1];
                child.SetImageByPathOwned(ZrdArrayString(
                    imageSpec,
                    1
                ));
                if (ZrdArrayCount(imageSpec) >= 4) {
                    ((HudUiElement *)(&child))
                        ->SetPos(
                            ZrdArrayInt(
                                imageSpec,
                                2,
                                0
                            ) + uiOriginX,
                            ZrdArrayInt(
                                imageSpec,
                                3,
                                0
                            ) + uiOriginY
                        );
                }

                child.flags = (child.flags & 0x10u) | 0x02u;
                ((HudUiElement *)(&child))->SetVisible(1);
                ((HudUiElement *)(&child))->Invalidate();
                AddChild((HudUiElement *)(&child));
            }
        }

        zReader::Node *const videoList =
            ZrdArrayBase(zReader_GetNamedNode(
                cfgRoot,
                "BACKGROUND_VIDEOS"
            ));
        int videoCount = ZrdArrayCount(videoList);
        if (videoCount > 10) {
            videoCount = 10;
        }

        {
            for (int index = 1; index < videoCount; ++index) {
                zReader::Node *const videoSpec = ZrdArrayBase(&videoList[index]);
                if (videoSpec == 0) {
                    continue;
                }

                HudUiBackgroundVideoWidget &child = backgroundVideoWidgets[index - 1];
                child.SetMediaPathOwnedAndRefresh(ZrdArrayString(
                    videoSpec,
                    1
                ));
                if (ZrdArrayCount(videoSpec) >= 4) {
                    child.SetPos(
                        ZrdArrayInt(
                            videoSpec,
                            2,
                            0
                        ) + uiOriginX,
                        ZrdArrayInt(
                            videoSpec,
                            3,
                            0
                        ) + uiOriginY
                    );
                }
                if (ZrdArrayCount(videoSpec) >= 5) {
                    zReader::Node *const color = ZrdArrayBase(&videoSpec[4]);
                    child.SetColorKey565((unsigned short)(zVid_PackColorRGB(
                        (unsigned char)(ZrdArrayInt(
                            color,
                            1,
                            0
                        )),
                        (unsigned char)(ZrdArrayInt(
                            color,
                            2,
                            0
                        )),
                        (unsigned char)(ZrdArrayInt(
                            color,
                            3,
                            0
                        ))
                    )));
                }

                child.SetVisible(1);
                child.Invalidate();
                child.SetBltSourceAndClipRect(
                    primaryClipImage,
                    0
                );
                child.RebuildBltRect();
                AddChild(&child);
            }
        }

        zReader::Node *const textList =
            ZrdArrayBase(zReader_GetNamedNode(
                cfgRoot,
                "BACKGROUND_TEXT"
            ));
        int textCount = ZrdArrayCount(textList);
        if (textCount > 50) {
            textCount = 50;
        }

        {
            for (int index = 1; index < textCount; ++index) {
                zReader::Node *const textSpec = ZrdArrayBase(&textList[index]);
                if (textSpec == 0) {
                    continue;
                }

                HudUiPanel *const child = (HudUiPanel *)(&backgroundTextPanels[index - 1]);
                child->SetTextFmt(
                    "%s",
                    zLoc::ResolveMessageKeyOrFallback(ZrdArrayString(
                        textSpec,
                        1
                    ))
                );
                child->SetPos(
                    ZrdArrayInt(
                        textSpec,
                        2,
                        0
                    ) + uiOriginX,
                    ZrdArrayInt(
                        textSpec,
                        3,
                        0
                    ) + uiOriginY
                );
                ApplyHudFontStyleToPanel(
                    child,
                    HudUiZrdOwnerFontStyle(this, ZrdArrayInt(
                        textSpec,
                        4,
                        0
                    ))
                );
                child->SetVisible(1);
                AddChild((HudUiElement *)(child));
            }
        }

        if (capturePrimary == 0) {
            HudUiBackgroundContainer::SetEnabled(1);
            UpdateAll(0.0f);
            capturedCompositeImage = zVideo_buff_CaptureSurfaceToImage(0);
            HudUiBackgroundContainer::SetEnabled(0);
            ((HudUiDialogController *)(this))->BlitOwnedSurfaceToPrimary();
        }

        zReader::Node *const cursorNode = zReader_GetNamedNode(
            cfgRoot,
            "CURSOR"
        );
        if (cursorNode != 0) {
            zReader::Node *const bitmapNode =
                ZrdArrayBase(zReader_GetNamedNode(
                    cursorNode,
                    "BITMAP"
                ));
            if (bitmapNode != 0) {
                cursorWidget.SetImageByPathOwnedAndRefresh(ZrdArrayString(
                    bitmapNode,
                    1
                ));
            }

            SetInputFocus((HudUiElement *)(&cursorWidget));

            zReader::Node *const centerNode =
                ZrdArrayBase(zReader_GetNamedNode(
                    cursorNode,
                    "CENTER"
                ));
            if (centerNode != 0) {
                // Original 0x4b9f69 stores CENTER's string pointer into HudUiWidget::alignFlags;
                // GetCenterX/Y only test the slot for nonzero on this cursor path.
                cursorWidget.alignFlags =
                    (unsigned int)(ZrdArrayString(centerNode, 1));
            }

            int cursorCapture = 1;
            zReader::ReadNamedInt(
                cursorNode,
                "CAPTURE",
                &cursorCapture
            );
            cursorWidget.SetImageOwnedAndRefresh(cursorCapture);
        }

        zReader::Node *const soundList =
            ZrdArrayBase(zReader_GetNamedNode(
                cfgRoot,
                "BACKGROUND_SOUNDS"
            ));
        int soundCount = ZrdArrayCount(soundList);
        if (soundCount > 10) {
            soundCount = 10;
        }

        {
            for (int index = 1; index < soundCount; ++index) {
                zReader::Node *const soundSpec = ZrdArrayBase(&soundList[index]);
                if (soundSpec == 0) {
                    continue;
                }

                HudUiBackgroundSoundEntry &entry = backgroundSounds[index - 1];
                float volume = 1.0f;
                if (ZrdArrayCount(soundSpec) >= 3) {
                    volume = ZrdArrayFloat(
                        soundSpec,
                        2,
                        1.0f
                    );
                }
                entry.sample = zSnd::FindSampleByName(ZrdArrayString(
                    soundSpec,
                    1
                ));
                entry.volume = volume;
            }
        }
    }

    zVideo::Dispatch_UnlockPrimarySurfaceState();
    return result;
}

// Reimplements 0x4b9850: HudUiBackground::SetEnabled
void HudUiBackground::SetEnabled(
    int enabled
) {
    if (enabled == 0) {
        int entryIndex14;
        for (entryIndex14 = 0;
            entryIndex14 < (int)(sizeof(backgroundSounds) / sizeof(backgroundSounds[0]));
            ++entryIndex14) {
            HudUiBackgroundSoundEntry &entry = backgroundSounds[entryIndex14];
            if (entry.playHandle != 0) {
                entry.playHandle->StopIfActive();
            }

            entry.playHandle = 0;
        }

        HudUiBackgroundContainer::SetEnabled(enabled);
        return;
    }

    {
        int entryIndex15;
        for (entryIndex15 = 0;
            entryIndex15 < (int)(sizeof(backgroundSounds) / sizeof(backgroundSounds[0]));
            ++entryIndex15) {
            HudUiBackgroundSoundEntry &entry = backgroundSounds[entryIndex15];
            if (entry.sample != 0) {
                entry.playHandle = entry.sample->PlayA3DSimple(entry.volume);
            }
        }
    }

    InvalidateChildren();
    HudUiBackgroundContainer::SetEnabled(enabled);
}

// Reimplements 0x4ba070: HudUiBackground::BindButtonsNodeToWidgetByName
unsigned char __fastcall HudUiBackground::BindButtonsNodeToWidgetByName(
    zReader::Node *parentNode,
    HudUiWidget *widget,
    const char *name
) {
    if (parentNode != 0) {
        zReader::Node *const buttonsNode = zReader_GetNamedNode(
            parentNode,
            "BUTTONS"
        );
        zReader::Node *const widgetNode = zReader_GetNamedNode(
            buttonsNode,
            name
        );
        if (widgetNode != 0) {
            widget->LoadFromZrd(
                widgetNode,
                this
            );
            widget->PostLoadFromZrd();
        }
    }

    return 0;
}

// Reimplements 0x4ba0c0: HudUiBackground::BindWidgetByName
int HudUiBackground::BindWidgetByName(
    zReader::Node *,
    HudUiWidget *widget,
    const char *name
) {
    return BindButtonsNodeToWidgetByName(
        cfgRoot,
        widget,
        name
    ) & 0xff;
}

// Reimplements 0x4ba0e0: HudUiBackground::BindPrimitiveNodeToElement
int HudUiBackground::BindPrimitiveNodeToElement(
    zReader::Node *,
    HudUiElement *element,
    const char *name
) {
    zReader::Node *const cfgRoot = this->cfgRoot;
    if (cfgRoot == 0) {
        return 0;
    }

    zReader::Node *const primitivesNode = zReader_GetNamedNode(
        cfgRoot,
        "PRIMITIVES"
    );
    if (primitivesNode == 0) {
        return 0;
    }

    zReader::Node *const primitiveNode = zReader_GetNamedNode(
        primitivesNode,
        name
    );
    if (primitiveNode == 0) {
        return 0;
    }

    ((HudUiContainer *)(this))->AddChild(element);

    zReader::Node *bitmapNode = zReader_GetNamedNode(
        primitiveNode,
        "BITMAP"
    );
    zReader::Node *bitmapBase = ZrdArrayBase(bitmapNode);
    const char *const bitmapPath = ZrdArrayString(
        bitmapBase,
        1
    );
    if (bitmapPath != 0) {
        ((HudUiWidget *)(element))->SetImageByPathOwned(bitmapPath);
    }

    zReader::Node *positionBase = ZrdArrayBase(zReader_GetNamedNode(
        primitiveNode,
        "POSITION"
    ));
    if (positionBase != 0) {
        element->SetPos(
            uiOriginX + ZrdArrayInt(
                positionBase,
                1,
                0
            ),
            uiOriginY + ZrdArrayInt(positionBase, 2, 0)
        );
    }

    zReader::Node *wordWrapBase = ZrdArrayBase(zReader_GetNamedNode(
        primitiveNode,
        "WORDWRAP"
    ));
    if (wordWrapBase != 0) {
        HudUiRect wordWrapRect = {0};
        wordWrapRect.right = ZrdArrayInt(
            wordWrapBase,
            1,
            0
        );
        wordWrapRect.bottom = ZrdArrayInt(
            wordWrapBase,
            2,
            0
        );
        element->EnableWordWrapWithRect(&wordWrapRect);
    }

    zReader::Node *fontNode = zReader_GetNamedNode(
        primitiveNode,
        "FONT"
    );
    zReader::Node *fontBase = ZrdArrayBase(fontNode);
    if (fontNode != 0) {
        const int fontIndex = fontBase != 0 ? ZrdArrayInt(
            fontBase,
            1,
            0
        ) : fontNode->value.i32;
        const HudFontStyle *const style = HudUiZrdOwnerFontStyle(
            this,
            fontIndex
        );
        if (style != 0) {
            HudUiPanel *const panel = (HudUiPanel *)(element);
            panel->alignMode = style->alignMode;
            element->SetFont(
                style->fontName,
                style->fontSize,
                style->fontWeight,
                0,
                0,
                0,
                2
            );
            panel->textColor0 = style->textColor;
            panel->textColor1 = style->textColor;
            panel->textDirty = 1;
            panel->shadowEnabled = style->shadowEnabled;
            panel->shadowOffsetX = 1;
            panel->shadowOffsetY = 1;
            panel->bkColor = style->bkColor;
            panel->bkMode = style->bkMode;
        }
    }

    zReader::Node *colorBase = ZrdArrayBase(zReader_GetNamedNode(
        primitiveNode,
        "COLOR"
    ));
    if (colorBase != 0) {
        const unsigned char red = (unsigned char)(ZrdArrayInt(
            colorBase,
            1,
            0
        ));
        const unsigned char green = (unsigned char)(ZrdArrayInt(
            colorBase,
            2,
            0
        ));
        const unsigned char blue = (unsigned char)(ZrdArrayInt(
            colorBase,
            3,
            0
        ));
        ((HudUiPrimitiveBindTarget *)(element))->color565 = zVid_PackColorRGB(
            red,
            green,
            blue
        ) & 0xffffu;
    }

    zReader::Node *relativeEndBase = ZrdArrayBase(zReader_GetNamedNode(
        primitiveNode,
        "ENDP_REL"
    ));
    if (relativeEndBase != 0) {
        const int startX = element->GetX();
        const int startY = element->GetY();
        ((HudUiPrimitiveBindTarget *)(element))
            ->SetSegmentEndpoints(
                startX,
                startY,
                startX + ZrdArrayInt(
                    relativeEndBase,
                    1,
                    0
                ),
                startY + ZrdArrayInt(relativeEndBase, 2, 0)
            );
    }

    zReader::Node *absoluteEndBase = ZrdArrayBase(zReader_GetNamedNode(
        primitiveNode,
        "ENDP_ABS"
    ));
    if (absoluteEndBase != 0) {
        ((HudUiPrimitiveBindTarget *)(element))
            ->SetSegmentEndpoints(
                element->GetX(),
                element->GetY(),
                ZrdArrayInt(
                    absoluteEndBase,
                    1,
                    0
                ),
                ZrdArrayInt(absoluteEndBase, 2, 0)
            );
    }

    HudUiRect clipRect;
    clipRect.left = element->GetX();
    clipRect.top = element->GetY();
    clipRect.right = element->GetX();
    clipRect.bottom = element->GetY();
    element->SetBltSourceAndClipRect(
        capturedCompositeImage,
        &clipRect
    );

    element->flags = (element->flags & 0x10u) | 0x02u;
    return 0;
}

// Reimplements 0x4ba350: HudUiBackground::FreeLoadedTreeRoots (HudUiBackground.cpp)
void HudUiBackground::FreeLoadedTreeRoots(
    int
) {
    zReader::Node *const root = loadedRoot;
    if (root != 0) {
        zReader::FreeLoadedTree(root);
    }

    loadedRoot = 0;
    cfgRoot = 0;
}

// Reimplements 0x4bc570: HudUiBackground::Update (D:\Proj\Battlesport\HudUi_Background.cpp)
void HudUiBackground::Update(
    float deltaSeconds
) {
    if (enabled == 0) {
        return;
    }

    memcpy(
        &mouseState,
        zInput::Mouse_GetStateSnapshotPtr(),
        sizeof(mouseState)
    );

    for (HudUiElement *widget = childHead; widget != 0; widget = widget->next) {
        const int hit = widget->HitTest(
            mouseState.cursorClientX,
            mouseState.cursorClientY
        );
        const int hovered = hit == 1 ? 1 : 0;

        if (widget->ShouldHandleInput(
            this,
            hovered
        ) != 0) {
            if ((mouseState.button2Transition & 4) != 0 && (widget->state & 2) == 2) {
                widget->state = (unsigned short)(widget->state & 0xfffd);
                widget->OnEndCapture();
            }

            if (hovered != 0) {
                if ((widget->state & 1) == 0) {
                    widget->state = (unsigned short)(widget->state | 1);
                    widget->ShowPreview();
                } else {
                    widget->OnHoverRepeat();
                }

                if ((mouseState.button1Transition & captureTransitionMask) != 0 &&
                    (widget->state & 2) == 0) {
                    widget->state = (unsigned short)(widget->state | 2);
                    widget->OnBeginCapture();
                }

                if ((mouseState.button1Transition & 4) != 0) {
                    widget->OnActivate();
                }

                if ((mouseState.button2Transition & 4) != 0) {
                    widget->OnClearBinding();
                }

                if ((mouseState.button1Transition & 3) != 0) {
                    widget->OnPointerButtonState(
                        mouseState.cursorClientX,
                        mouseState.cursorClientY
                    );
                }

                if ((mouseState.button1Transition & 4) != 0 && (widget->state & 2) == 2) {
                    widget->OnCapturedPrimaryRelease();
                }
            } else {
                if ((mouseState.button1Transition & captureTransitionMask) != 0 &&
                    (widget->state & 2) == 2) {
                    widget->state = (unsigned short)(widget->state & 0xfffd);
                    widget->OnEndCapture();
                }

                if ((mouseState.button1Transition & 3) != 0 && (widget->state & 2) == 2) {
                    widget->OnPointerButtonState(
                        mouseState.cursorClientX,
                        mouseState.cursorClientY
                    );
                }

                if ((widget->state & 1) == 1) {
                    widget->state = (unsigned short)(widget->state & 0xfffe);
                    widget->HidePreview();
                }
            }
        }

        widget->AfterInputUpdate(
            this,
            hovered
        );
    }

    HudUiElement *const focusBeforeUpdate = inputFocusElement;
    if (focusBeforeUpdate != 0) {
        focusBeforeUpdate->DrawBase();
    }

    UpdateAll(deltaSeconds);

    HudUiElement *const focusAfterUpdate = inputFocusElement;
    if (focusAfterUpdate != 0) {
        focusAfterUpdate->SetPos(
            mouseState.cursorClientX,
            mouseState.cursorClientY
        );
        focusAfterUpdate->Update(deltaSeconds);
    }
}

// Reimplements 0x4ba4a0: HudFontStyle::HudFontStyle
HudFontStyle::HudFontStyle() {
    validMarker = 0;
    fontName = 0;
    fontSize = 0;
    textColor = 0;
    shadowEnabled = 0;
    alignMode = 0;
    fontWeight = 0x1f4;
}

HudFontStyle::~HudFontStyle() {
    Destructor();
}

// Reimplements 0x4ba4c0: HudFontStyle::Destructor
void HudFontStyle::Destructor() {
    validMarker = 0;
}

// Reimplements 0x4b3d00: HudUiWidget::Constructor
HudUiWidget::HudUiWidget(
    unsigned int initAlignFlags
) : HudUiElement(0, 0) {
    alignFlags = initAlignFlags;
    image = 0;
    ownsImage = 0;
    bltClipRectOrNull = 0;
    *((unsigned short *)(&imageStateWord)) = 0;
    dirtyRectCount = 0;

    {
        int dirtyRectIndex;
        for (dirtyRectIndex = 0; dirtyRectIndex < 4; ++dirtyRectIndex) {
            dirtyRects[dirtyRectIndex].framesRemaining = 0;
        }
    }
}

HudUiWidget * HudUiWidget::Constructor(
    unsigned int initAlignFlags
) {
    new (this) HudUiWidget(initAlignFlags);
    return this;
}

// Reimplements 0x4b3e90: HudUiWidget::InvalidateRect
// (D:\Proj\Battlesport\hudui.cpp)
void HudUiWidget::InvalidateRect(
    const HudUiRect *dirtyRect
) {
    if (image == 0) {
        return;
    }

    HudUiRectDirty *slot = 0;
    {
        for (int index = 0; index < 4; ++index) {
            if (dirtyRects[index].framesRemaining == 0) {
                slot = &dirtyRects[index];
                break;
            }
        }
    }

    if (slot == 0) {
        return;
    }

    slot->srcLeft = dirtyRect->left;
    slot->srcTop = dirtyRect->top;
    slot->srcRight = dirtyRect->right;
    slot->srcBottom = dirtyRect->bottom;

    if (slot->srcLeft < x) {
        slot->srcLeft = x;
    }

    const int imageRight = image->width + x;
    if (slot->srcRight > imageRight) {
        slot->srcBottom = imageRight;
    }

    if (slot->srcTop < y) {
        slot->srcTop = y;
    }

    const int imageBottom = image->height + y;
    if (slot->srcBottom > imageBottom) {
        slot->srcBottom = imageBottom;
    }

    if (slot->srcRight <= slot->srcLeft || slot->srcBottom <= slot->srcTop) {
        return;
    }

    ++dirtyRectCount;
    slot->framesRemaining = (g_HudUi_InvalidateMask == 0x0c ? 1u : 0u) + 1u;
    slot->drawX = slot->srcLeft;
    slot->drawY = slot->srcTop;

    slot->srcLeft -= GetCenterX();
    slot->srcRight -= GetCenterX();
    slot->srcTop -= GetCenterY();
    slot->srcBottom -= GetCenterY();
    Invalidate();
}

// Reimplements 0x4bf980: HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget
// (D:\Proj\Battlesport\hudui_background.cpp)
HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget(
    const char *imagePath,
    int initCaptureEnabled
) : HudUiWidget(0) {
    captureEnabled = initCaptureEnabled;
    capturedImage = 0;
    if (imagePath != 0) {
        SetImageByPathOwnedAndRefresh(imagePath);
    }

    reservedC8 = 0;
    reservedCC = 0;
    captureSourceSelector = 1;
}

// Reimplements 0x4bfa20: HudUiBackgroundCursorWidget::DestructorCore
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundCursorWidget::DestructorCore() {
    if (capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
    }

    HudUiWidget::DestructorCore();
}

// Reimplements 0x4bfa50: HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh(
    const char *imagePath
) {
    if (HudUiWidget::SetImageByPathOwned(imagePath) != 0) {
        SetImageBorrowedAndRefresh();
    }
}

// Reimplements 0x4bfa70: HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged(
    zVidImagePartial *image
) {
    if (HudUiWidget::SetImageBorrowedAndInvalidate(image) != 0) {
        SetImageBorrowedAndRefresh();
    }
}

// Reimplements 0x4bfa90: HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh(
    int newCaptureEnabled
) {
    captureEnabled = newCaptureEnabled;
    if (newCaptureEnabled == 0 && capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
        capturedImage = 0;
        HudUiElement::SetBltSourceAndClipRect(
            0,
            0
        );
        return;
    }

    if (capturedImage == 0) {
        SetImageBorrowedAndRefresh();
    }
}

// Reimplements 0x4bfae0: HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh() {
    if (captureEnabled == 0 || image == 0) {
        return;
    }

    if (capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
    }

    capturedImage = zVid_Image::Create();
    if (capturedImage == 0) {
        return;
    }

    zVid_Image::SetSize(
        capturedImage,
        image->width,
        image->height
    );
    void *const pixels = malloc((size_t)(capturedImage->pixelCount) * sizeof(unsigned short));
    zVid_Image_SetPixels(
        capturedImage,
        pixels,
        0
    );
    capturedImage->formatFlagsPacked = (unsigned char)(capturedImage->formatFlagsPacked | 0x20u);

    const int y = HudUiElement::GetY();
    const int x = HudUiElement::GetX();
    RebuildCapturedImage(
        x,
        y
    );
}

// Reimplements 0x4bfb70: HudUiBackgroundCursorWidget::SetPos
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundCursorWidget::SetPos(
    int newX,
    int newY
) {
    HudUiWidget::SetPos(
        newX,
        newY
    );
    RebuildCapturedImage(
        x,
        y
    );
}

// Reimplements 0x4bfba0: HudUiBackgroundCursorWidget::RebuildCapturedImage
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundCursorWidget::RebuildCapturedImage(
    int originX,
    int originY
) {
    if (capturedImage == 0) {
        return;
    }

    zVidRect32 sourceRect;
    sourceRect.left = originX;
    sourceRect.top = originY;
    sourceRect.right = originX + image->width;
    sourceRect.bottom = originY + image->height;

    if (zVideo_buff::CopySurfaceRectToImage(captureSourceSelector, &sourceRect, capturedImage) !=
        0) {
        const HudUiRect clipRect = {sourceRect.left - originX,
            sourceRect.top - originY,
            sourceRect.right - originX,
            sourceRect.bottom - originY};
        SetBltSourceAndClipRect(
            capturedImage,
            &clipRect
        );
        return;
    }

    SetBltSourceAndClipRect(
        0,
        0
    );
}

// Reimplements 0x4bfc50: HudUiBackgroundCursorWidget::Draw
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundCursorWidget::Draw() {
    HudUiWidget::Draw();
}

// Reimplements 0x4bfc60: HudUiBackgroundCursorWidget::DrawBase
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundCursorWidget::DrawBase() {
    if (bltSource != 0) {
        zVid_Image::BlitToActiveTarget(
            (zVidImagePartial *)(bltSource),
            x,
            y,
            0,
            (zVidRect32 *)(&clipRect)
        );
    }
}

// Reimplements 0x4bfc80: HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget
// (D:\Proj\Battlesport\hudui_background.cpp)
HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget()
    : HudUiElement(0, 0) {
    mediaPath[0] = '\0';
    stream = 0;
    elapsedTimeSec = 0.0f;
}

HudUiBackgroundVideoWidget::~HudUiBackgroundVideoWidget() {
    zFMV_Stream *const oldStream = stream;
    if (oldStream != 0) {
        oldStream->Destructor();
        ::operator delete(oldStream);
        stream = 0;
    }
}

// Reimplements 0x4bfcd0: HudUiBackgroundVideoWidget::Destructor
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundVideoWidget::Destructor() {
    this->~HudUiBackgroundVideoWidget();
}

// Reimplements 0x4bfd40: HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh(
    const char *path
) {
    strncpy(
        mediaPath,
        path,
        0x104
    );

    struct _stat statBuffer;
    if (_stat(
        mediaPath,
        &statBuffer
    ) == -1) {
        char *const resolvedPath = zSys::FindFileOnDriveType(
            5,
            mediaPath,
            0
        );
        if (resolvedPath != 0) {
            strncpy(
                mediaPath,
                resolvedPath,
                0x104
            );
        }
    }

    if (_stat(
        mediaPath,
        &statBuffer
    ) == -1) {
        stream = 0;
        return;
    }

    zFMV_Stream *const newStream = (zFMV_Stream *)(::operator new(sizeof(zFMV_Stream)));
    stream = newStream != 0 ? newStream->Init(
        mediaPath,
        0
    ) : 0;

    RebuildBltRect();
}

// Reimplements 0x4bfe20: HudUiBackgroundVideoWidget::SetColorKey565
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundVideoWidget::SetColorKey565(
    unsigned short colorKey
) {
    if (stream != 0) {
        stream->formatFlagsPacked |= 0x02;
    }

    colorKey565 = colorKey;
}

// Reimplements 0x4bfe40: HudUiBackgroundVideoWidget::Update
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundVideoWidget::Update(
    float deltaSeconds
) {
    if ((flags & 0x10u) != 0) {
        return;
    }

    if (stream != 0) {
        const int frameTick = (int)((float)(stream->videoFramesPerSecond) * elapsedTimeSec);
        stream->ReadAndDecodeFrame((unsigned int)(frameTick % stream->videoFrameCount));
    }

    HudUiElement::Update(deltaSeconds);
    elapsedTimeSec += deltaSeconds;
}

// Reimplements 0x4bfe90: HudUiBackgroundVideoWidget::Draw
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundVideoWidget::Draw() {
    DrawBase();

    if (stream != 0) {
        zVid_Image::BlitToActiveTarget(
            (zVidImagePartial *)(stream),
            x,
            y,
            colorKey565,
            0
        );
    }
}

// Reimplements 0x4bfec0: HudUiBackgroundVideoWidget::DrawBase
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundVideoWidget::DrawBase() {
    zVidImagePartial *const bltSource = (zVidImagePartial *)(this->bltSource);
    if (bltSource != 0) {
        const int dstX = x > 0 ? x : 0;
        const int dstY = y > 0 ? y : 0;
        zVid_Image::BlitToActiveTarget(
            bltSource,
            dstX,
            dstY,
            0,
            (zVidRect32 *)(&clipRect)
        );
    }
}

// Reimplements 0x4bff00: HudUiBackgroundVideoWidget::RebuildBltRect
// (D:\Proj\Battlesport\hudui_background.cpp)
void HudUiBackgroundVideoWidget::RebuildBltRect() {
    HudUiRect rect;
    rect.left = GetX() > 0 ? GetX() : 0;
    rect.top = GetY() > 0 ? GetY() : 0;

    if (stream == 0) {
        return;
    }

    const int streamRight = rect.left + stream->width;
    const int streamBottom = rect.top + stream->height;

    zVidImagePartial *const bltSource = (zVidImagePartial *)(this->bltSource);
    if (bltSource != 0) {
        rect.right = streamRight < bltSource->width ? streamRight : bltSource->width;
        rect.bottom = streamBottom < bltSource->height ? streamBottom : bltSource->height;
    } else {
        rect.right = streamRight;
        rect.bottom = streamBottom;
    }

    SetClipRect(&rect);
}

/**
 * Reimplements 0x4b4ee0: HudUiZrdWidget::HudUiZrdWidget.
 * Purpose: initialize the ZRD widget's base widget state, image/sound slots,
 * panel vectors, enabled mode, and initial invalidation state.
 */
HudUiZrdWidget::HudUiZrdWidget() : HudUiWidget(0) {
    modeOrEnabled = 1;
    originY = 0;
    originX = 0;
    owner = 0;
    defaultImage = 0;
    disabledImage = 0;
    rolloverImage = 0;
    rolloverSound = 0;
    rolloverPlayHandle = 0;
    rolloverSoundScale = 1.0f;
    activateImage = 0;
    activateSound = 0;
    activateSoundScale = 1.0f;
    activatePlayHandle = 0;

    HudUiPanel **labelSource = labelPanels.end;
    HudUiPanel **labelDest = labelPanels.begin;
    if (labelSource != labelPanels.end) {
        do {
            *labelDest++ = *labelSource++;
        } while (labelSource != labelPanels.end);
    }
    ((StdPtrVector *)(&labelPanels))
        ->ClearNoOpDestroy(
            (int *)(labelDest),
            (int *)(labelPanels.end)
        );
    labelPanels.end = labelDest;

    HudUiPanel **rolloverSource = rolloverLabelPanels.end;
    HudUiPanel **rolloverDest = rolloverLabelPanels.begin;
    if (rolloverSource != rolloverLabelPanels.end) {
        do {
            *rolloverDest++ = *rolloverSource++;
        } while (rolloverSource != rolloverLabelPanels.end);
    }
    ((StdPtrVector *)(&rolloverLabelPanels))
        ->ClearNoOpDestroy(
            (int *)(rolloverDest),
            (int *)(rolloverLabelPanels.end)
        );
    rolloverLabelPanels.end = rolloverDest;

    HudUiPanel **activateSource = activateLabelPanels.end;
    HudUiPanel **activateDest = activateLabelPanels.begin;
    if (activateSource != activateLabelPanels.end) {
        do {
            *activateDest++ = *activateSource++;
        } while (activateSource != activateLabelPanels.end);
    }
    ((StdPtrVector *)(&activateLabelPanels))
        ->ClearNoOpDestroy(
            (int *)(activateDest),
            (int *)(activateLabelPanels.end)
        );
    activateLabelPanels.end = activateDest;

    *((unsigned short *)(&imageStateWord)) = 1;
    Invalidate();
    flags = ((unsigned char)(flags) & 0x10u) | 0x02u;
}

// Reimplements 0x4b4ee0: HudUiZrdWidget::Constructor
HudUiZrdWidget * HudUiZrdWidget::Constructor() {
    new (this) HudUiZrdWidget;
    return this;
}

// Reimplements 0x4b59f0: HudUiZrdWidget::LoadFromZrd
int HudUiZrdWidget::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    owner = ownerDialog;
    HudUiElement::SetVisible(1);
    ((HudUiContainer *)(ownerDialog))->AddChild(this);

    originX = ownerDialog->uiOriginX;
    originY = ownerDialog->uiOriginY;
    if (zrdSection == 0) {
        return 0;
    }

    zReader::Node *const positionNode = zReader_GetNamedNode(
        zrdSection,
        "POSITION"
    );
    zReader::Node *const positionBase = ZrdArrayBase(positionNode);
    if (positionBase != 0) {
        originX += ZrdArrayInt(
            positionBase,
            1,
            0
        );
        originY += ZrdArrayInt(
            positionBase,
            2,
            0
        );
    }

    int widgetX = originX;
    int widgetY = originY;
    zReader::Node *const bitmapNode = zReader_GetNamedNode(
        zrdSection,
        "BITMAP"
    );
    zReader::Node *const bitmapBase = ZrdArrayBase(bitmapNode);
    const char *const bitmapPath = ZrdArrayString(
        bitmapBase,
        1
    );
    if (bitmapPath != 0) {
        defaultImage = SetImageByPathOwned(bitmapPath);
        if (ZrdArrayCount(bitmapBase) >= 4) {
            widgetX += ZrdArrayInt(
                bitmapBase,
                2,
                0
            );
            widgetY += ZrdArrayInt(
                bitmapBase,
                3,
                0
            );
        }
    }

    HudUiElement::SetPos(
        widgetX,
        widgetY
    );

    void *const clipSource = ownerDialog->capturedCompositeImage;
    if (clipSource != 0) {
        HudUiRect *const bounds = GetBoundsRectOrNull();
        if (bounds != 0) {
            HudUiElement::SetBltSourceAndClipRect(
                clipSource,
                bounds
            );
        }
    }

    zReader::Node *const rolloverNode = zReader_GetNamedNode(
        zrdSection,
        "ROLLOVER"
    );
    if (rolloverNode != 0) {
        LoadHudZrdBitmap(
            rolloverNode,
            "BITMAP",
            &rolloverImage
        );
        LoadHudZrdSound(
            rolloverNode,
            &rolloverSound,
            &rolloverSoundScale
        );
        LoadHudZrdLabelSection(
            this,
            rolloverNode,
            rolloverLabelPanels
        );
        ApplyHudZrdFlashSection(
            rolloverNode,
            rolloverLabelPanels
        );
    }

    zReader::Node *const disableNode = zReader_GetNamedNode(
        zrdSection,
        "DISABLE"
    );
    if (disableNode != 0) {
        LoadHudZrdBitmap(
            disableNode,
            "BITMAP",
            &disabledImage
        );
        LoadHudZrdSound(
            disableNode,
            &disabledSound,
            &disabledSoundScale
        );
        LoadHudZrdLabelSection(
            this,
            disableNode,
            disabledLabelPanels
        );
    }

    zReader::Node *const activateNode = zReader_GetNamedNode(
        zrdSection,
        "ACTIVATE"
    );
    if (activateNode != 0) {
        LoadHudZrdBitmap(
            activateNode,
            "BITMAP",
            &activateImage
        );
        LoadHudZrdSound(
            activateNode,
            &activateSound,
            &activateSoundScale
        );
        LoadHudZrdLabelSection(
            this,
            activateNode,
            activateLabelPanels
        );
        ApplyHudZrdFlashSection(
            activateNode,
            activateLabelPanels
        );
    }

    LoadHudZrdLabelSection(
        this,
        zrdSection,
        labelPanels
    );
    ApplyHudZrdFlashSection(
        zrdSection,
        labelPanels
    );
    return 1;
}

/**
 * Reimplements 0x4ba4d0: HudUiPanelPtrVector::EraseRange.
 * Binary Ninja identifies the body as a VC5 std::vector-style erase helper
 * over the recovered HudUiPanelPtrVector storage: shift [last, end) over
 * first, update end, and return the original first iterator.
 */
HudUiPanel ** HudUiPanelPtrVector::EraseRange(
    HudUiPanel **first,
    HudUiPanel **last
) {
    HudUiPanel **write = first;
    HudUiPanel **read = last;
    HudUiPanel **const oldEnd = end;
    while (read != oldEnd) {
        *write = *read;
        ++write;
        ++read;
    }

    end = write;
    return first;
}

/**
 * Reimplements 0x4ba510: HudUiPanelPtrVector::InsertN.
 * Binary Ninja shows the matching VC5 std::vector-style insert helper for
 * HudUiPanel pointers, including in-place tail movement and reallocation when
 * the current capacity cannot hold the requested insertion count.
 */
void HudUiPanelPtrVector::InsertN(
    HudUiPanel **position,
    unsigned int count,
    HudUiPanel **valueSource
) {
    if (count == 0) {
        return;
    }

    const size_t size = begin != 0 ? (size_t)(end - begin) : 0;
    const size_t positionIndex = begin != 0 && position != 0 ? (size_t)(position - begin) : 0;
    const size_t capacity = begin != 0 ? (size_t)(capacityEnd - begin) : 0;

    if (size + count <= capacity) {
        HudUiPanel **read = end;
        HudUiPanel **write = end + count;
        while (read != begin + positionIndex) {
            --read;
            --write;
            *write = *read;
        }

        for (unsigned int i = 0; i < count; ++i) {
            begin[positionIndex + i] = *valueSource;
        }

        end += count;
        return;
    }

    const size_t growBy = count < size ? size : count;
    const size_t newCapacity = size + growBy;
    HudUiPanel **const newBegin =
        (HudUiPanel **)(::operator new(newCapacity * sizeof(HudUiPanel *)));
    HudUiPanel **write = newBegin;

    for (size_t prefixIndex = 0; prefixIndex < positionIndex; ++prefixIndex) {
        *write++ = begin[prefixIndex];
    }

    for (unsigned int insertIndex = 0; insertIndex < count; ++insertIndex) {
        *write++ = *valueSource;
    }

    for (size_t suffixIndex = positionIndex; suffixIndex < size; ++suffixIndex) {
        *write++ = begin[suffixIndex];
    }

    ::operator delete(begin);
    begin = newBegin;
    end = newBegin + size + count;
    capacityEnd = newBegin + newCapacity;
}

// Reimplements 0x4b52f0: HudUiZrdWidget::DeleteChildIfPresent
void *__stdcall HudUiZrdWidget::DeleteChildIfPresent(
    void *childWidgetOrNull
) {
    if (childWidgetOrNull != 0) {
        ((HudUiElement *)(childWidgetOrNull))->ScalarDeletingDestructor(1);
    }

    return 0;
}

void HudUiZrdWidget_DeletePanelVectorChildren(
    HudUiPanelPtrVector &vector
) {
    for (HudUiPanel **it = vector.begin; it != vector.end; ++it) {
        if (*it != 0) {
            (*it)->ScalarDeletingDestructor(1);
        }

        *it = 0;
    }
}

// Reimplements 0x4b50c0: HudUiZrdWidget::DestructorCore
void HudUiZrdWidget::DestructorCore() {

    for (HudUiPanel **it = labelPanels.begin; it != labelPanels.end; ++it) {
        *it = (HudUiPanel *)(DeleteChildIfPresent(*it));
    }

    HudUiZrdWidget_DeletePanelVectorChildren(rolloverLabelPanels);
    HudUiZrdWidget_DeletePanelVectorChildren(activateLabelPanels);

    labelPanels.EraseRange(
        labelPanels.begin,
        labelPanels.end
    );
    rolloverLabelPanels.EraseRange(
        rolloverLabelPanels.begin,
        rolloverLabelPanels.end
    );
    activateLabelPanels.EraseRange(
        activateLabelPanels.begin,
        activateLabelPanels.end
    );

    if (defaultImage != 0 && defaultImage != image) {
        zVid_Image::ReleaseIfNotDefault(defaultImage);
        defaultImage = 0;
    }

    if (activateImage != 0 && activateImage != image) {
        zVid_Image::ReleaseIfNotDefault(activateImage);
        activateImage = 0;
    }

    if (rolloverImage != 0 && rolloverImage != image) {
        zVid_Image::ReleaseIfNotDefault(rolloverImage);
        rolloverImage = 0;
    }

    if (disabledImage != 0 && disabledImage != image) {
        zVid_Image::ReleaseIfNotDefault(disabledImage);
        disabledImage = 0;
    }

    if (image != 0 && ownsImage == 0) {
        zVid_Image::ReleaseIfNotDefault(image);
    }

    ::operator delete(disabledLabelPanels.begin);
    disabledLabelPanels.begin = 0;
    disabledLabelPanels.end = 0;
    disabledLabelPanels.capacityEnd = 0;

    ::operator delete(activateLabelPanels.begin);
    activateLabelPanels.begin = 0;
    activateLabelPanels.end = 0;
    activateLabelPanels.capacityEnd = 0;

    ::operator delete(rolloverLabelPanels.begin);
    rolloverLabelPanels.begin = 0;
    rolloverLabelPanels.end = 0;
    rolloverLabelPanels.capacityEnd = 0;

    ::operator delete(labelPanels.begin);
    labelPanels.begin = 0;
    labelPanels.end = 0;
    labelPanels.capacityEnd = 0;

    HudUiWidget::DestructorCore();
}

// Reimplements 0x4b50a0: HudUiZrdWidget::ScalarDeletingDestructor
HudUiElement * HudUiZrdWidget::ScalarDeletingDestructor(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x41c480: HudUiZrdWidget::ScalarDeletingDestructorThunk
HudUiZrdWidget * HudUiZrdWidget::ScalarDeletingDestructorThunk(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b5310: HudUiZrdWidget::Invalidate
void HudUiZrdWidget::Invalidate() {
    HudUiElement::Invalidate();

    HudUiPanel **panel = labelPanels.begin;
    if (panel == 0) {
        return;
    }

    while (panel != labelPanels.end) {
        HudUiPanel *const label = *panel;
        label->Invalidate();
        ++panel;
    }
}

// Reimplements 0x4b5350: HudUiZrdWidget::GetBoundsRectOrNull
HudUiRect * HudUiZrdWidget::GetBoundsRectOrNull() {
    if (modeOrEnabled == 0) {
        return 0;
    }

    if (image != 0) {
        boundsRect.left = x;
        boundsRect.top = y;
        boundsRect.right = x + image->width;
        boundsRect.bottom = y + image->height;
        return &boundsRect;
    }

    HudUiPanel **panelIt = labelPanels.begin;
    if (panelIt == 0) {
        return 0;
    }

    HudUiPanel *const firstPanel = *panelIt;
    boundsRect.top = firstPanel->GetY();
    boundsRect.bottom = boundsRect.top + firstPanel->QueryTextHeight();

    while (panelIt != labelPanels.end) {
        HudUiPanel *const panel = *panelIt;
        boundsRect.bottom += panel->QueryTextHeight();

        const int alignMode = panel->alignMode;
        const int panelX = panel->GetX();
        if (panel->textDirty != 0) {
            panel->RebuildTextRect();
        }

        const int width = panel->textWidthPx;
        if (alignMode == 0) {
            boundsRect.left = firstPanel->GetX();
            const int right = panelX + width;
            if (right > boundsRect.right) {
                boundsRect.right = right;
            }
        } else if (alignMode == 1) {
            const int halfWidth = width / 2;
            const int left = panelX - halfWidth;
            if (left < boundsRect.left) {
                boundsRect.left = left;
            }

            const int right = panelX + halfWidth;
            if (right > boundsRect.right) {
                boundsRect.right = right;
            }
        } else if (alignMode == 2) {
            boundsRect.right = firstPanel->GetX();
            const int left = panelX - width;
            if (left > boundsRect.left) {
                boundsRect.left = boundsRect.left;
            } else {
                boundsRect.left = left;
            }
        }

        ++panelIt;
    }

    boundsRect.bottom -= firstPanel->QueryTextHeight();
    return &boundsRect;
}

// Reimplements 0x4b5740: HudUiZrdWidget::RefreshState
void HudUiZrdWidget::RefreshState() {
    for (HudUiPanel **rolloverIt = rolloverLabelPanels.begin; rolloverIt != rolloverLabelPanels.end;
        ++rolloverIt) {
        HudUiPanel *const panel = *rolloverIt;
        panel->SetVisible(0);
    }

    for (HudUiPanel **activateIt = activateLabelPanels.begin; activateIt != activateLabelPanels.end;
        ++activateIt) {
        HudUiPanel *const panel = *activateIt;
        panel->SetVisible(0);
    }

    if (modeOrEnabled != 0) {
        for (HudUiPanel **labelIt = labelPanels.begin; labelIt != labelPanels.end; ++labelIt) {
            HudUiPanel *const panel = *labelIt;
            panel->SetVisible(1);
        }

        for (HudUiPanel **disabledIt = disabledLabelPanels.begin;
            disabledIt != disabledLabelPanels.end;
            ++disabledIt) {
            HudUiPanel *const panel = *disabledIt;
            panel->SetVisible(0);
        }

        SetImageBorrowedAndInvalidate(defaultImage);
        Invalidate();
        return;
    }

    for (HudUiPanel **labelIt2 = labelPanels.begin; labelIt2 != labelPanels.end; ++labelIt2) {
        HudUiPanel *const panel = *labelIt2;
        panel->SetVisible(0);
    }

    for (HudUiPanel **disabledIt2 = disabledLabelPanels.begin;
        disabledIt2 != disabledLabelPanels.end;
        ++disabledIt2) {
        HudUiPanel *const panel = *disabledIt2;
        panel->SetVisible(1);
    }

    SetImageBorrowedAndInvalidate(disabledImage);
    Invalidate();
}

// Reimplements 0x4b5630: HudUiZrdWidget::ShowPreview
void HudUiZrdWidget::ShowPreview() {
    if (rolloverImage != 0) {
        if (defaultImage == 0) {
            defaultImage = image;
        }

        SetImageBorrowedAndInvalidate(rolloverImage);
    }

    if (rolloverSound != 0) {
        rolloverPlayHandle = rolloverSound->PlayA3DSimple(rolloverSoundScale);
    }

    if (rolloverLabelPanels.begin != 0) {
        HudUiSetPanelVectorVisible(
            labelPanels,
            0
        );
        HudUiSetPanelVectorVisible(
            activateLabelPanels,
            0
        );
        HudUiSetPanelVectorVisible(
            rolloverLabelPanels,
            1
        );
        return;
    }

    HudUiSetPanelVectorVisible(
        labelPanels,
        1
    );
    HudUiSetPanelVectorVisible(
        activateLabelPanels,
        0
    );
}

// Reimplements 0x4b5900: HudUiZrdWidget::OnActivate
void HudUiZrdWidget::OnActivate() {
    zInput::ResetAllTransitionState();

    if (activateImage != 0) {
        SetImageBorrowedAndInvalidate(activateImage);
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle->StopIfActive();
        rolloverPlayHandle = 0;
    }

    if (activateSound != 0) {
        activatePlayHandle = activateSound->PlayA3DSimple(activateSoundScale);
    }

    HudUiSetPanelVectorVisible(
        rolloverLabelPanels,
        0
    );

    if (activateLabelPanels.begin != 0) {
        HudUiSetPanelVectorVisible(
            activateLabelPanels,
            1
        );
        HudUiSetPanelVectorVisible(
            labelPanels,
            0
        );
        return;
    }

    HudUiSetPanelVectorVisible(
        labelPanels,
        1
    );
}

// Reimplements 0x4b5860: HudUiZrdWidget::HidePreview
void HudUiZrdWidget::HidePreview() {
    if (defaultImage != 0) {
        SetImageBorrowedAndInvalidate(defaultImage);
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle = 0;
    }

    for (HudUiPanel **rolloverIt = rolloverLabelPanels.begin; rolloverIt != rolloverLabelPanels.end;
        ++rolloverIt) {
        HudUiPanel *const panel = *rolloverIt;
        panel->SetVisible(0);
    }

    for (HudUiPanel **activateIt = activateLabelPanels.begin; activateIt != activateLabelPanels.end;
        ++activateIt) {
        HudUiPanel *const panel = *activateIt;
        panel->SetVisible(0);
    }

    for (HudUiPanel **labelIt = labelPanels.begin; labelIt != labelPanels.end; ++labelIt) {
        HudUiPanel *const panel = *labelIt;
        panel->SetVisible(1);
    }
}

// Reimplements 0x4b6fc0: HudUiCheckToggleWidget::HudUiCheckToggleWidget
HudUiCheckToggleWidget::HudUiCheckToggleWidget() : HudUiZrdWidget() {
    checked = 0;
    uncheckedImage = 0;
    checkedImage = 0;
    checkedLabelPanel = 0;
    disabledCheckedImage = 0;
    disabledCheckedFallbackImage = 0;
}

// Reimplements 0x4b6fc0: HudUiCheckToggleWidget::Constructor
HudUiCheckToggleWidget * HudUiCheckToggleWidget::Constructor() {
    new (this) HudUiCheckToggleWidget;
    return this;
}

// Reimplements 0x4b7020: HudUiCheckToggleWidget::DestructorCore
void HudUiCheckToggleWidget::DestructorCore() {
    SetImageBorrowedAndInvalidate(uncheckedImage);

    if (checkedImage != 0) {
        ::operator delete(checkedImage);
        checkedImage = 0;
    }

    if (checkedLabelPanel != 0) {
        checkedLabelPanel->ScalarDeletingDestructor(1);
        checkedLabelPanel = 0;
    }

    HudUiZrdWidget::DestructorCore();
}

// Reimplements 0x40cf30: HudUiCheckToggleWidget::DestructorCoreThunk
void HudUiCheckToggleWidget::DestructorCoreThunk() {
    DestructorCore();
}

// Reimplements 0x4b7000: HudUiCheckToggleWidget::ScalarDeletingDestructor
HudUiElement * HudUiCheckToggleWidget::ScalarDeletingDestructor(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x41a590: HudUiCheckToggleWidget::ScalarDeletingDestructorThunk
HudUiCheckToggleWidget * HudUiCheckToggleWidget::ScalarDeletingDestructorThunk(
    unsigned int flags
) {
    DestructorCoreThunk();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b70b0: HudUiCheckToggleWidget::GetBoundsRectOrNull
HudUiRect * HudUiCheckToggleWidget::GetBoundsRectOrNull() {
    return &boundsRect;
}

// Reimplements 0x4b70c0: HudUiCheckToggleWidget::RefreshState
void HudUiCheckToggleWidget::RefreshState() {
    HudUiSetPanelVectorVisible(
        rolloverLabelPanels,
        0
    );
    HudUiSetPanelVectorVisible(
        activateLabelPanels,
        0
    );

    if (modeOrEnabled != 0) {
        HudUiSetPanelVectorVisible(
            labelPanels,
            1
        );
        HudUiSetPanelVectorVisible(
            disabledLabelPanels,
            0
        );

        if (checked != 0) {
            zVidImagePartial *const image = checkedImage != 0 ? checkedImage : uncheckedImage;
            if (image != 0) {
                SetImageBorrowedAndInvalidate(image);
                Invalidate();
                return;
            }
        }

        Invalidate();
        return;
    }

    HudUiSetPanelVectorVisible(
        labelPanels,
        0
    );
    HudUiSetPanelVectorVisible(
        disabledLabelPanels,
        1
    );

    if (checked != 0) {
        zVidImagePartial *const image =
            disabledCheckedImage != 0 ? disabledCheckedImage : disabledCheckedFallbackImage;
        if (image != 0) {
            SetImageBorrowedAndInvalidate(image);
        }
    }

    Invalidate();
}

// Reimplements 0x4b7210: HudUiCheckToggleWidget::ShowPreview
void HudUiCheckToggleWidget::ShowPreview() {
    if (modeOrEnabled == 0 || checked != 0) {
        return;
    }

    if (rolloverSound != 0) {
        rolloverPlayHandle = rolloverSound->PlayA3DSimple(rolloverSoundScale);
    }

    HudUiZrdWidget::ShowPreview();
}

// Reimplements 0x4b7250: HudUiCheckToggleWidget::HidePreview
void HudUiCheckToggleWidget::HidePreview() {
    if (modeOrEnabled == 0 || checked != 0) {
        return;
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle->StopIfActive();
        rolloverPlayHandle = 0;
    }

    HudUiZrdWidget::HidePreview();
}

// Reimplements 0x4b7290: HudUiCheckToggleWidget::OnActivate
void HudUiCheckToggleWidget::OnActivate() {
    if (modeOrEnabled == 0) {
        return;
    }

    SetChecked(checked == 0 ? 1 : 0);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40caa0: HudUiCheckToggleWidget::OnActivateThunk
void HudUiCheckToggleWidget::OnActivateThunk() {
    OnActivate();
}

// Reimplements 0x4b7340: HudUiCheckToggleWidget::LoadFromZrd
int HudUiCheckToggleWidget::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiZrdWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );
    uncheckedImage = image;

    zReader::Node *const checkedNode = zReader_GetNamedNode(
        zrdSection,
        "CHECKED"
    );
    if (checkedNode != 0) {
        LoadHudZrdBitmap(
            checkedNode,
            "BITMAP",
            &checkedImage
        );
        zReader::Node *const textNode = zReader_GetNamedNode(
            checkedNode,
            "TEXT"
        );
        if (textNode != 0) {
            checkedLabelPanel = CreateHudZrdTextPanel(
                this,
                textNode,
                0
            );
        }
    }

    zReader::Node *const disabledUnselectedNode = zReader_GetNamedNode(
        zrdSection,
        "DISABLE_UNSEL"
    );
    if (disabledUnselectedNode != 0) {
        LoadHudZrdBitmap(
            disabledUnselectedNode,
            "BITMAP",
            &disabledCheckedFallbackImage
        );
        zReader::Node *const textNode = zReader_GetNamedNode(
            disabledUnselectedNode,
            "TEXT"
        );
        if (textNode != 0) {
            checkedLabelPanel = CreateHudZrdTextPanel(
                this,
                textNode,
                0
            );
        }

        LoadHudZrdLabelSection(
            this,
            zrdSection,
            disabledLabelPanels
        );
    }

    zReader::Node *const disabledSelectedNode = zReader_GetNamedNode(
        zrdSection,
        "DISABLE_SEL"
    );
    if (disabledSelectedNode != 0) {
        LoadHudZrdBitmap(
            disabledSelectedNode,
            "BITMAP",
            &disabledCheckedImage
        );
        zReader::Node *const textNode = zReader_GetNamedNode(
            disabledSelectedNode,
            "TEXT"
        );
        if (textNode != 0) {
            checkedLabelPanel = CreateHudZrdTextPanel(
                this,
                textNode,
                0
            );
        }
    }

    if (uncheckedImage != 0) {
        boundsRect.left = x;
        boundsRect.top = y;
        boundsRect.right = x + uncheckedImage->width;
        boundsRect.bottom = y + uncheckedImage->height;
    } else if (labelPanels.begin != 0) {
        HudUiPanel **panelIt = labelPanels.begin;
        HudUiPanel *const firstPanel = *panelIt;
        boundsRect.top = firstPanel->GetY();
        boundsRect.left = firstPanel->GetX();
        boundsRect.bottom = firstPanel->QueryTextHeight() + boundsRect.top;

        while (panelIt != labelPanels.end) {
            HudUiPanel *const panel = *panelIt;
            boundsRect.bottom += panel->QueryTextHeight();

            if (panel->textDirty != 0) {
                panel->RebuildTextRect();
            }

            const int right = panel->textWidthPx + boundsRect.left;
            if (right > boundsRect.right) {
                boundsRect.right = right;
            }

            ++panelIt;
        }

        boundsRect.bottom -= firstPanel->QueryTextHeight();
    }

    return 1;
}

// Reimplements 0x4b72c0: HudUiCheckToggleWidget::SetChecked
int HudUiCheckToggleWidget::SetChecked(
    int newChecked
) {
    const int previousChecked = checked;
    checked = newChecked;

    if (newChecked != 0) {
        if (checkedImage != 0) {
            SetImageBorrowedAndInvalidate(checkedImage);
        }

        if (checkedLabelPanel != 0) {
            checkedLabelPanel->SetVisible(1);
            Invalidate();
            return previousChecked;
        }
    } else {
        if (uncheckedImage != 0) {
            SetImageBorrowedAndInvalidate(uncheckedImage);
        }

        if (checkedLabelPanel != 0) {
            checkedLabelPanel->SetVisible(0);
        }
    }

    Invalidate();
    return previousChecked;
}

// Reimplements 0x4b7d60: HudUiCycleSelectorWidget::HudUiCycleSelectorWidget
HudUiCycleSelectorWidget::HudUiCycleSelectorWidget() : HudUiZrdWidget() {
    selectedIndex = 0;
    itemCount = 0;
    for (int i = 0; i < 20; ++i) {
        entriesA[i] = 0;
        entriesB[i] = 0;
    }

    firstIndex = 0;
    visibleCount = 20;
    fontStyleRef = 0;
    textOffsetY = 0;
    textOffsetX = 0;
}

// Reimplements 0x4b7d60: HudUiCycleSelectorWidget::Constructor
HudUiCycleSelectorWidget * HudUiCycleSelectorWidget::Constructor() {
    new (this) HudUiCycleSelectorWidget;
    return this;
}

// Reimplements 0x4b7de0: HudUiCycleSelectorWidget::DestructorCore
void HudUiCycleSelectorWidget::DestructorCore() {

    for (int i = 0; i < 20; ++i) {
        if (entriesA[i] != 0) {
            HudUiZrdWidget::DeleteChildIfPresent(entriesA[i]);
            entriesA[i] = 0;
        }

        if (entriesB[i] != 0) {
            HudUiZrdWidget::DeleteChildIfPresent(entriesB[i]);
            entriesB[i] = 0;
        }
    }

    HudUiZrdWidget::DestructorCore();
}

// Reimplements 0x40cf40: HudUiCycleSelectorWidget::DestructorCoreThunk
void HudUiCycleSelectorWidget::DestructorCoreThunk() {
    DestructorCore();
}

// Reimplements 0x4b7dc0: HudUiCycleSelectorWidget::ScalarDeletingDestructor
HudUiElement * HudUiCycleSelectorWidget::ScalarDeletingDestructor(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x41a570: HudUiCycleSelectorWidget::ScalarDeletingDestructorThunk
HudUiCycleSelectorWidget * HudUiCycleSelectorWidget::ScalarDeletingDestructorThunk(
    unsigned int flags
) {
    DestructorCoreThunk();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b7ee0: HudUiCycleSelectorWidget::AdvanceSelectionAndActivate
void HudUiCycleSelectorWidget::AdvanceSelectionAndActivate() {
    const int nextIndex = selectedIndex + 1;
    selectedIndex = nextIndex;

    int endIndex = visibleCount;
    if (endIndex >= itemCount) {
        endIndex = itemCount;
    }

    if (nextIndex >= endIndex) {
        selectedIndex = firstIndex;
    }

    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x4b7f20: HudUiCycleSelectorWidget::SetIndexClamped
void HudUiCycleSelectorWidget::SetIndexClamped(
    int index
) {
    if (index < firstIndex) {
        selectedIndex = firstIndex;
        return;
    }

    if (index >= itemCount) {
        selectedIndex = itemCount - 1;
        return;
    }

    if (index >= visibleCount) {
        selectedIndex = visibleCount - 1;
        return;
    }

    selectedIndex = index;
}

// Reimplements 0x4b7f80: HudUiCycleSelectorWidget::SetVisibleRange
void HudUiCycleSelectorWidget::SetVisibleRange(
    int first,
    int last
) {
    if (first >= 0 && first < itemCount) {
        firstIndex = first;
    }

    if (last >= first && last < itemCount) {
        visibleCount = last;
    }

    if (selectedIndex < first) {
        selectedIndex = first;
    }

    if (selectedIndex >= last) {
        selectedIndex = last - 1;
    }
}

// Reimplements 0x4b7e60: HudUiCycleSelectorWidget::Update
void HudUiCycleSelectorWidget::Update(
    float deltaSeconds
) {
    for (int i = 0; i < itemCount; ++i) {
        Invalidate();

        if (entriesA[i] != 0) {
            entriesA[i]->SetVisible(i == selectedIndex ? 1 : 0);
        }

        if (entriesB[i] != 0) {
            entriesB[i]->SetVisible(i == selectedIndex ? 1 : 0);
        }
    }

    HudUiElement::Update(deltaSeconds);
}

/**
 * Reimplements 0x4b7fd0: HudUiCycleSelectorWidget::AddTextEntry.
 *
 * Purpose: create a hidden transition text-panel entry, position it with the
 * selector text offset, and attach it to the owning HUD background container.
 *
 * Evidence: BN assembly at 0x4b7fd0 grows itemCount/visibleCount, allocates a
 * 0x2c0 HudUiTransitionTextPanel, stores it in entriesA[index], dispatches
 * SetTextFmt/SetPos/SetVisible, and adds it through the owner container.
 */
void HudUiCycleSelectorWidget::AddTextEntry(
    int index,
    const char *text,
    int posX,
    int posY
) {
    if (index >= itemCount) {
        int newCount = index + 1;
        if (newCount >= 20) {
            newCount = 20;
        }

        itemCount = newCount;
        if (newCount > visibleCount) {
            visibleCount = newCount;
        }
    }

    if (index > visibleCount) {
        return;
    }

    HudUiTransitionTextPanel *const transitionPanel =
        (HudUiTransitionTextPanel *)(::operator new(sizeof(HudUiTransitionTextPanel)));
    new (transitionPanel) HudUiTransitionTextPanel;

    entriesA[index] = (HudUiWidget *)(transitionPanel);
    transitionPanel->SetTextFmt(text);

    transitionPanel->SetPos(
        textOffsetX + posX,
        textOffsetY + posY
    );
    transitionPanel->SetVisible(0);
    ((HudUiContainer *)(owner))->AddChild(transitionPanel);
}

// Reimplements 0x4ba020: HudUiTransitionTextPanel::HudUiTransitionTextPanel
HudUiTransitionTextPanel::HudUiTransitionTextPanel()
    : HudUiPanel(
        0,
        0,
        0
    ) {
    flashCountdown = 0;
    flashAltColor0 = 0;
    flashEnabled = 0;
    flashMode = 0;
    flashResetValue = 0.349999994f;
    flashDirectionSign = 1;
}

HudUiTransitionTextPanel::~HudUiTransitionTextPanel() {
    HudUiPanel::Destructor();
}

// Reimplements 0x4bc9f0: HudUiTransitionTextPanel::TickFlash
void HudUiTransitionTextPanel::TickFlash(
    float deltaSeconds
) {
    const unsigned int elementFlags = flags;
    if ((elementFlags & 0x10u) != 0) {
        return;
    }

    if ((elementFlags & 1u) != 0) {
        timer -= deltaSeconds;
        if (timer <= 0.0f) {
            SetVisible(0);
        }
    }

    if (flashEnabled == 0 || (flags & 0x10u) != 0) {
        HudUiPanel::Draw();
        return;
    }

    flashCountdown -= deltaSeconds;
    switch (flashMode) {
    case 0:
        HudUiPanel::Draw();
    case 1:
        if (flashCountdown < 0.0f) {
            flashCountdown += flashResetValue;
            textDirty = 1;
            flashDirectionSign = -flashDirectionSign;
        }

        if (flashDirectionSign == 1) {
            HudUiPanel::Draw();
        }
        return;

    case 2:
    case 3:
        if (flashCountdown < 0.0f) {
            flashCountdown = flashResetValue;
            textDirty = 1;
            flashDirectionSign = -flashDirectionSign;

            const unsigned int oldTextColor0 = textColor0;
            const unsigned int oldTextColor1 = textColor1;
            textColor0 = (unsigned int)(flashAltColor0);
            textColor1 = (unsigned int)(flashAltColor1);
            flashAltColor0 = (int)(oldTextColor0);
            flashAltColor1 = (int)(oldTextColor1);
        }

        HudUiPanel::Draw();
        return;

    default:
        HudUiPanel::Draw();
        return;
    }
}

/**
 * Reimplements 0x4bc930: HudUiTransitionTextPanel::ResetFlashState.
 *
 * Purpose: enable flash state, update a positive flash rate to its half-period,
 * reset the countdown from that period, and restore forward flash direction.
 *
 * Evidence: BN assembly at 0x4bc930 writes flashEnabled, conditionally stores
 * flashRate*0.5 into flashResetValue when flashRate is positive, copies
 * flashResetValue into flashCountdown, and writes flashDirectionSign = 1.
 */
void HudUiTransitionTextPanel::ResetFlashState(
    float flashRate
) {
    flashEnabled = 1;
    if (flashRate > 0.0f) {
        flashResetValue = flashRate * 0.5f;
    }

    flashDirectionSign = 1;
    memcpy(
        &flashCountdown,
        &flashResetValue,
        sizeof(flashCountdown)
    );
}

/**
 * Reimplements 0x4bc980: HudUiTransitionTextPanel::SetFlashRate.
 *
 * Purpose: enter rate-only flashing by resetting flash state unless the panel
 * is already in rate-only flash mode.
 *
 * Evidence: BN assembly at 0x4bc980 returns when flashMode is 1; otherwise it
 * calls ResetFlashState(flashRate) and stores flashMode = 1.
 */
void HudUiTransitionTextPanel::SetFlashRate(
    float flashRate
) {
    if (flashMode == 1) {
        return;
    }

    ResetFlashState(flashRate);
    flashMode = 1;
}

// Reimplements 0x4bc9b0: HudUiTransitionTextPanel::SetFlashColorAndRate
void HudUiTransitionTextPanel::SetFlashColorAndRate(
    unsigned int flashColor,
    float flashRate
) {
    if (flashMode == 2) {
        return;
    }

    ResetFlashState(flashRate);
    flashMode = 2;
    flashAltColor0 = flashColor;
    flashAltColor1 = flashColor;
}

/**
 * Reimplements 0x4b8100: HudUiCycleSelectorWidget::ApplyFontStyleForEntry.
 *
 * Purpose: grow the selector entry range when needed, validate the owning
 * background font style, and copy that style onto the text-panel entry.
 *
 * Evidence: BN assembly at 0x4b8100 selects owner->fontStyles[styleIndex] at
 * HudUiBackground offset 0x1cec, checks validMarker, calls the entry SetFont
 * slot, and copies text color, shadow, alignment, background mode, and
 * background color fields.
 */
void HudUiCycleSelectorWidget::ApplyFontStyleForEntry(
    int index,
    int styleIndex
) {
    if (index >= itemCount) {
        int newCount = index + 1;
        if (newCount >= 20) {
            newCount = 20;
        }

        itemCount = newCount;
        if (newCount > visibleCount) {
            visibleCount = newCount;
        }
    }

    if (index > visibleCount) {
        return;
    }

    const HudFontStyle *const style = &owner->fontStyles[styleIndex];
    if (style->validMarker == 0) {
        return;
    }

    HudUiPanel *const panel = (HudUiPanel *)(entriesA[index]);
    panel->SetFont(
        style->fontName,
        style->fontSize,
        style->fontWeight,
        0,
        0,
        0,
        2
    );

    panel->textColor0 = style->textColor;
    panel->textColor1 = style->textColor;
    panel->textDirty = 1;
    panel->shadowEnabled = style->shadowEnabled;
    panel->shadowOffsetX = 1;
    panel->shadowOffsetY = 1;
    panel->alignMode = style->alignMode;
    panel->bkMode = style->bkMode;
    panel->bkColor = style->bkColor;
}

// Reimplements 0x4b8200: HudUiCycleSelectorWidget::AddBitmapEntry
void HudUiCycleSelectorWidget::AddBitmapEntry(
    int index,
    const char *imagePath,
    int posX,
    int posY
) {
    if (index > itemCount) {
        int newCount = index + 1;
        if (newCount >= 20) {
            newCount = 20;
        }

        itemCount = newCount;
        if (newCount > visibleCount) {
            visibleCount = newCount;
        }
    }

    if (index > visibleCount) {
        return;
    }

    HudUiWidget *const bitmapWidget = (HudUiWidget *)(::operator new(sizeof(HudUiWidget)));
    bitmapWidget->Constructor(0);
    entriesB[index] = bitmapWidget;
    bitmapWidget->SetImageByPathOwned(imagePath);
    bitmapWidget->SetPos(
        posX,
        posY
    );
    bitmapWidget->SetVisible(0);
    ((HudUiContainer *)(owner))->AddChild((HudUiElement *)(bitmapWidget));
}

// Reimplements 0x4b82e0: HudUiCycleSelectorWidget::LoadFromZrd
int HudUiCycleSelectorWidget::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiZrdWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );

    zReader::Node *const fontNode = zReader_GetNamedNode(
        zrdSection,
        "FONT"
    );
    if (fontNode != 0) {
        fontStyleRef = (void *)((unsigned int)(fontNode->value.u32));
    }

    zReader::Node *const textOffsetNode = zReader_GetNamedNode(
        zrdSection,
        "TEXTOFFSET"
    );
    zReader::Node *const textOffsetBase = ZrdArrayBase(textOffsetNode);
    if (textOffsetBase != 0) {
        textOffsetX = ZrdArrayInt(
            textOffsetBase,
            1,
            textOffsetX
        );
        textOffsetY = ZrdArrayInt(
            textOffsetBase,
            2,
            textOffsetY
        );
    }

    zReader::Node *const cycleNode = zReader_GetNamedNode(
        zrdSection,
        "CYCLE"
    );
    zReader::Node *const cycleBase = ZrdArrayBase(cycleNode);
    if (cycleBase == 0) {
        return 1;
    }

    int count = ZrdArrayCount(cycleBase) - 1;
    if (count >= 20) {
        count = 20;
    }

    itemCount = count;
    if (count > visibleCount) {
        visibleCount = count;
    }

    {
        for (int index = 0; index < itemCount; ++index) {
            zReader::Node *const entryNode = ZrdArrayItem(
                cycleBase,
                index + 1
            );

            zReader::Node *const textNode = zReader_GetNamedNode(
                entryNode,
                "TEXT"
            );
            zReader::Node *const textBase = ZrdArrayBase(textNode);
            if (textBase != 0) {
                const char *const key = ZrdArrayString(
                    textBase,
                    1
                );
                const char *const text = key != 0 ? zLoc::ResolveMessageKeyOrFallback(key) : "";
                AddTextEntry(
                    index,
                    text != 0 ? text : "",
                    originX + ZrdArrayInt(
                        textBase,
                        2,
                        0
                    ),
                    originY + ZrdArrayInt(textBase, 3, 0)
                );
                ApplyFontStyleForEntry(
                    index,
                    ZrdArrayInt(textBase, 4, 0)
                );
            }

            zReader::Node *const bitmapNode = zReader_GetNamedNode(
                entryNode,
                "BITMAP"
            );
            zReader::Node *const bitmapBase = ZrdArrayBase(bitmapNode);
            if (bitmapBase != 0) {
                int bitmapX = originX;
                int bitmapY = originY;
                if (ZrdArrayCount(bitmapBase) >= 4) {
                    bitmapX += ZrdArrayInt(
                        bitmapBase,
                        2,
                        0
                    );
                    bitmapY += ZrdArrayInt(
                        bitmapBase,
                        3,
                        0
                    );
                }

                AddBitmapEntry(
                    index,
                    ZrdArrayString(
                        bitmapBase,
                        1
                    ),
                    bitmapX,
                    bitmapY
                );
            }
        }
    }

    return 1;
}

// Reimplements 0x4b8450: HudUiFillBitmap::HudUiFillBitmap
HudUiFillBitmap::HudUiFillBitmap() : HudUiZrdWidget() {
    normalizedValue = 0.0f;
    previewImage = 0;
    fillImage = 0;
    previewRect.right = 0;
    previewRect.left = 0;
    previewRect.bottom = 0;
    previewRect.top = 0;
    fillRect.right = 0;
    fillRect.left = 0;
    fillRect.bottom = 0;
    fillRect.top = 0;
}

// Reimplements 0x4b84d0: HudUiFillBitmap::DestructorCore
void HudUiFillBitmap::DestructorCore() {

    if (previewImage != 0 && previewImage != image) {
        zVid_Image::ReleaseIfNotDefault(previewImage);
        previewImage = 0;
    }

    if (fillImage != 0 && fillImage != image) {
        zVid_Image::ReleaseIfNotDefault(fillImage);
        fillImage = 0;
    }

    HudUiZrdWidget::DestructorCore();
}

// Reimplements 0x40cf50: HudUiFillBitmap::DestructorCoreThunk
void HudUiFillBitmap::DestructorCoreThunk() {
    DestructorCore();
}

// Reimplements 0x4b84b0: HudUiFillBitmap::ScalarDeletingDestructor
HudUiElement * HudUiFillBitmap::ScalarDeletingDestructor(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b8520: HudUiFillBitmap::Draw
void HudUiFillBitmap::Draw() {
    if (previewImage == 0 || fillImage == 0) {
        return;
    }

    HudUiWidget::Draw();

    if (fillRect.left != fillRect.right) {
        zVid_Image::BlitToActiveTarget(
            fillImage,
            x + fillOffsetX,
            y + fillOffsetY,
            0,
            (zVidRect32 *)(&fillRect)
        );
    }

    if (previewRect.left != previewRect.right) {
        zVid_Image::BlitToActiveTarget(
            previewImage,
            x + previewOffsetX,
            y + previewOffsetY,
            0,
            (zVidRect32 *)(&previewRect)
        );
    }
}

// Reimplements 0x4b85c0: HudUiFillBitmap::LoadFromZrd
int HudUiFillBitmap::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiZrdWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );

    zReader::Node *const fillBitmapNode = zReader_GetNamedNode(
        zrdSection,
        "FILLBITMAP"
    );
    zReader::Node *const fillBitmapBase = ZrdArrayBase(fillBitmapNode);
    if (fillBitmapBase != 0) {
        fillImage = zImage::TexDir_FindOrCreateByPath(ZrdArrayString(
            fillBitmapBase,
            1
        ));
        HudUiElement::Invalidate();

        int posX = originX;
        int posY = originY;
        if (ZrdArrayCount(fillBitmapBase) >= 4) {
            posX += ZrdArrayInt(
                fillBitmapBase,
                2,
                0
            );
            posY += ZrdArrayInt(
                fillBitmapBase,
                3,
                0
            );
        }

        HudUiElement::SetPos(
            posX,
            posY
        );
        previewImage = image;
        HudUiElement::Invalidate();
    }

    SetNormalizedValueAndRebuild(0.0f);
    return 1;
}

// Reimplements 0x4b8650: HudUiFillBitmap::UpdateNormalizedFromCursor
void HudUiFillBitmap::UpdateNormalizedFromCursor() {
    const int cursorX = owner->mouseState.cursorClientX;
    const int relativeX = cursorX - GetCenterX();
    const int imageWidth = image != 0 ? image->width : 0;
    SetNormalizedValueAndRebuild((float)(relativeX) / (float)(imageWidth));
    OnActivate();
}

// Reimplements 0x4ba3c0: HudUiFillBitmap::SetNormalizedValue
void HudUiFillBitmap::SetNormalizedValue(
    float value
) {
    unsigned int valueBits = 0;
    memcpy(
        &valueBits,
        &value,
        sizeof(valueBits)
    );
    memcpy(
        &normalizedValue,
        &valueBits,
        sizeof(normalizedValue)
    );
    Invalidate();
}

// Reimplements 0x4b86b0: HudUiFillBitmap::SetNormalizedValueAndRebuild
void HudUiFillBitmap::SetNormalizedValueAndRebuild(
    float value
) {
    if (fillImage == 0) {
        return;
    }

    normalizedValue = value;
    HudUiElement::Invalidate();

    const int fillWidth = fillImage->width;
    const int fillHeight = fillImage->height;
    const int filledWidth = (int)((float)(fillWidth)*value);

    fillRect.left = 0;
    fillRect.top = 0;
    fillRect.right = filledWidth;
    fillRect.bottom = fillHeight;
    fillOffsetX = 0;
    fillOffsetY = 0;

    if (previewImage == 0) {
        return;
    }

    previewRect.left = filledWidth;
    previewRect.top = 0;
    previewRect.right = previewImage->width;
    previewRect.bottom = fillHeight;
    previewOffsetX = filledWidth;
    previewOffsetY = 0;
}

// Reimplements 0x4b8760: HudUiZrdWidgetEx17C_Item::HudUiZrdWidgetEx17C_Item
HudUiZrdWidgetEx17C_Item::HudUiZrdWidgetEx17C_Item() : HudUiZrdWidget() {
    selected = 0;
    selectedImage = 0;
    unselectedImage = 0;
    ownerSelector = 0;
    mouseRectValid = 0;
}

HudUiZrdWidgetEx17C_Item * HudUiZrdWidgetEx17C_Item::Constructor() {
    new (this) HudUiZrdWidgetEx17C_Item;
    return this;
}

// Reimplements 0x4b87c0: HudUiZrdWidgetEx17C_Item::DestructorCore
void HudUiZrdWidgetEx17C_Item::DestructorCore() {
    HudUiZrdWidget::DestructorCore();
}

// Reimplements 0x4b87a0: HudUiZrdWidgetEx17C_Item::ScalarDeletingDestructor
HudUiElement * HudUiZrdWidgetEx17C_Item::ScalarDeletingDestructor(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b87d0: HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected
void HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected() {
    if (selected == 0) {
        HudUiZrdWidget::ShowPreview();
    }
}

void HudUiZrdWidgetEx17C_Item::ShowPreview() {
    ShowPreviewIfNotSelected();
}

// Reimplements 0x4b87e0: HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected
void HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected() {
    if (selected == 0) {
        HudUiZrdWidget::HidePreview();
    }
}

void HudUiZrdWidgetEx17C_Item::HidePreview() {
    HidePreviewIfNotSelected();
}

// Reimplements 0x4b87f0: HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf
void HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf() {
    ownerSelector->SetSelectedIndex(itemIndex);
    ownerSelector->OnActivate();
    HudUiZrdWidget::OnActivate();

    {
        for (int index = 0; index < ownerSelector->optionCount; ++index) {
            HudUiZrdWidgetEx17C_Item *const option = ownerSelector->options[index];
            option->HidePreviewIfNotSelected();
        }
    }
}

void HudUiZrdWidgetEx17C_Item::OnActivate() {
    OnActivateSelectSelf();
}

// Reimplements 0x4b8850: HudUiZrdWidgetEx17C_Item::LoadFromZrd
int HudUiZrdWidgetEx17C_Item::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiZrdWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );

    unselectedImage = image;
    unselectedRolloverImage = rolloverImage;
    selectedImage = activateImage;
    selectedRolloverImage = activateImage;

    boundsRect.top = GetCenterY();
    boundsRect.left = GetCenterX();

    if (image != 0) {
        boundsRect.bottom = boundsRect.top + image->width;
        boundsRect.right = boundsRect.left + image->height;
    } else {
        boundsRect.bottom = boundsRect.top;
        boundsRect.right = boundsRect.left;
    }

    if (unselectedImage != 0) {
        boundsRect.top = y;
        boundsRect.left = x;
        boundsRect.bottom = y + unselectedImage->height;
        boundsRect.right = x + unselectedImage->width;
    } else if (labelPanels.begin != 0) {
        HudUiPanel **panelIt = labelPanels.begin;
        HudUiPanel *const firstPanel = *panelIt;
        boundsRect.top = firstPanel->GetY();
        boundsRect.left = firstPanel->GetX();
        boundsRect.bottom = firstPanel->QueryTextHeight() + boundsRect.top;

        while (panelIt != labelPanels.end) {
            HudUiPanel *const panel = *panelIt;
            boundsRect.bottom += panel->QueryTextHeight();

            if (panel->textDirty != 0) {
                panel->RebuildTextRect();
            }

            const int right = panel->textWidthPx + boundsRect.left;
            if (right > boundsRect.right) {
                boundsRect.right = right;
            }

            ++panelIt;
        }

        boundsRect.bottom -= firstPanel->QueryTextHeight();
    }

    mouseRect = boundsRect;
    mouseRectValid = 1;

    zReader::Node *const mouseRectNode = zReader_GetNamedNode(
        zrdSection,
        "MOUSERECT"
    );
    zReader::Node *const mouseRectBase = ZrdArrayBase(mouseRectNode);
    if (mouseRectBase != 0) {
        mouseRect.top += ZrdArrayInt(
            mouseRectBase,
            1,
            0
        );
        mouseRect.left += ZrdArrayInt(
            mouseRectBase,
            2,
            0
        );
        mouseRect.bottom = mouseRect.top + ZrdArrayInt(
            mouseRectBase,
            3,
            0
        );
        mouseRect.right = mouseRect.left + ZrdArrayInt(
            mouseRectBase,
            4,
            0
        );
    }

    return 1;
}

// Reimplements 0x4b8a90: HudUiZrdWidgetEx17C_Item::SetSelected
void HudUiZrdWidgetEx17C_Item::SetSelected(
    int selectedValue
) {
    selected = selectedValue;
    if (modeOrEnabled == 0) {
        return;
    }

    if (selectedValue != 0) {
        defaultImage = selectedImage;
        rolloverImage = selectedRolloverImage;
    } else {
        defaultImage = unselectedImage;
        rolloverImage = unselectedRolloverImage;
    }

    SetImageBorrowedAndInvalidate(defaultImage);
}

// Reimplements 0x4b8af0: HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds
HudUiRect * HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds() {
    return mouseRectValid != 0 ? &mouseRect : GetBoundsRectOrNull();
}

// Reimplements 0x4b8b10: HudUiZrdWidgetEx17C::HudUiZrdWidgetEx17C
HudUiZrdWidgetEx17C::HudUiZrdWidgetEx17C() : HudUiZrdWidget() {
    optionCount = 0;

    {
        int optionIndex;
        for (optionIndex = 0; optionIndex < 10; ++optionIndex) {
            options[optionIndex] = 0;
        }
    }
}

HudUiZrdWidgetEx17C * HudUiZrdWidgetEx17C::Constructor() {
    new (this) HudUiZrdWidgetEx17C;
    return this;
}

// Reimplements 0x4b8b60: HudUiZrdWidgetEx17C::DestructorCore
void HudUiZrdWidgetEx17C::DestructorCore() {

    {
        int optionIndex;
        for (optionIndex = 0; optionIndex < 10; ++optionIndex) {
            HudUiZrdWidgetEx17C_Item *option = options[optionIndex];
            if (option != 0) {
                option->ScalarDeletingDestructor(1);
                options[optionIndex] = 0;
            }
        }
    }

    HudUiZrdWidget::DestructorCore();
}

// Reimplements 0x4b8b40: HudUiZrdWidgetEx17C::ScalarDeletingDestructor
HudUiElement * HudUiZrdWidgetEx17C::ScalarDeletingDestructor(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x41c4c0: HudUiZrdWidgetEx17C::ScalarDeletingDestructorThunk
HudUiZrdWidgetEx17C * HudUiZrdWidgetEx17C::ScalarDeletingDestructorThunk(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b8be0: HudUiZrdWidgetEx17C::LoadFromZrd
int HudUiZrdWidgetEx17C::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    owner = ownerDialog;

    zReader::Node *const radioNode = zReader_GetNamedNode(
        zrdSection,
        "RADIO"
    );
    zReader::Node *const radioBase = ZrdArrayBase(radioNode);
    if (radioBase != 0) {
        optionCount = ZrdArrayCount(radioBase) - 1;
        if (optionCount >= 10) {
            optionCount = 10;
        }

        {
            for (int index = 0; index < optionCount; ++index) {
                HudUiZrdWidgetEx17C_Item *const option =
                    (HudUiZrdWidgetEx17C_Item *)(::operator new(sizeof(HudUiZrdWidgetEx17C_Item)));
                option->Constructor();
                options[index] = option;

                option->LoadFromZrd(
                    &radioBase[index + 1],
                    ownerDialog
                );
                option->ownerSelector = this;
                option->itemIndex = index;
            }
        }
    }

    SetSelectedIndex(0);
    return 1;
}

// Reimplements 0x409010: HudUiZrdWidgetEx17C::EnableChildAtIndex
void HudUiZrdWidgetEx17C::EnableChildAtIndex(
    int childIndex
) {
    if (childIndex >= optionCount) {
        return;
    }

    HudUiZrdWidgetEx17C_Item *const option = options[childIndex];
    option->selected = 1;
    option->RefreshState();
}

void HudUiZrdWidgetEx17C::SetVisible(
    int childIndex
) {
    EnableChildAtIndex(childIndex);
}

// Reimplements 0x4b8cf0: HudUiZrdWidgetEx17C::SetSelectedIndex
int HudUiZrdWidgetEx17C::SetSelectedIndex(
    int index
) {
    selectedIndex = index;
    {
        for (int optionIndex = 0; optionIndex < 10; ++optionIndex) {
            HudUiZrdWidgetEx17C_Item *const option = options[optionIndex];
            if (option != 0) {
                option->SetSelected(optionIndex == index ? 1 : 0);
            }
        }
    }

    return 1;
}

// Reimplements 0x4b92a0: HudUiListSelectorItem::HudUiListSelectorItem
// Source model lives in the inline class-body constructor in zhud_ui.h.
// Reimplements 0x4b9520: HudUiListSelectorItem::OnActivate
void HudUiListSelectorItem::OnActivate() {
    typedef void( * OnSelectedIndexChangedFn)(
        void *self,
        int selectedIndex
    );

    void *const selectionOwner = owner;
    if (selectionOwner != 0) {
        const unsigned int *const ownerSlots =
            *(const unsigned int *const *)selectionOwner;
        ((OnSelectedIndexChangedFn)(ownerSlots[33]))(
            selectionOwner,
            entryIndex
        );
    }
}

// Reimplements 0x4ba410: HudUiListSelectorItem::Draw
void HudUiListSelectorItem::Draw() {
    HudUiPanel::Draw();

    clipRect.left = GetX();
    if (textDirty != 0) {
        RebuildTextRect();
    }

    clipRect.right = GetX() + textWidthPx;
    clipRect.top = GetY();
    const int textHeight = QueryTextHeight();
    clipRect.bottom = textHeight + GetY();
}

// Reimplements 0x4b8d30: HudCmdBindButtonBase::HudCmdBindButtonBase
HudCmdBindButtonBase::HudCmdBindButtonBase() :
    HudUiCheckToggleWidget()
{
    bindingSlotTotalCount = 0;
    bindingSlotPanels = 0;
    visibleListOffsetX = 0.0f;
    visibleListOffsetY = 0.0f;
    overflowListOffsetX = 0.0f;
    overflowListOffsetY = 0.0f;
    bindingSlotSpacing = 0xf;
    selectedBindingIndex = -1;
}

// Reimplements 0x40bdf0: StdPtrVector::ClearNoOpDestroy
void StdPtrVector::ClearNoOpDestroy(
    int *begin,
    int *end
) {
    (void)begin;
    (void)end;
}

// Reimplements 0x40be60: HudCmdBindingEntry::CopyRange
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
HudCmdBindingEntry **__fastcall HudCmdBindingEntry::CopyRange(
    HudCmdBindingEntry **sourceBegin,
    HudCmdBindingEntry **sourceEnd,
    HudCmdBindingEntry **dest
) {
    if (sourceBegin != sourceEnd) {
        do {
            *dest = *sourceBegin;
            ++sourceBegin;
            ++dest;
        } while (sourceBegin != sourceEnd);
    }

    return dest;
}

/**
 * No standalone retail function; current Binary Ninja evidence shows this
 * destructor body inlined into the VC scalar-deleting destructor at 0x40bf50
 * and the static delete helper at 0x40bf20.
 * Purpose: release the owned command-binding display string.
 */
HudCmdBindingEntry::~HudCmdBindingEntry() {
    if (displayText != 0) {
        free(displayText);
        displayText = 0;
    }
}

/**
 * Reimplements 0x40bf20: HudCmdBindingEntry::DeleteAndReturnNull.
 * Binary Ninja shows a static HudCmdBindButton.cpp helper that destroys a
 * non-null binding entry, deletes its storage, and returns null.
 */
HudCmdBindingEntry *__stdcall HudCmdBindingEntry::DeleteAndReturnNull(
    HudCmdBindingEntry *entry
) {
    if (entry != 0) {
        delete entry;
    }

    return 0;
}

/**
 * Reimplements 0x40bf80: HudCmdBindButtonBase::AddBindingEntry.
 * Binary Ninja shows the HudCmdBindButton.cpp method allocating a
 * HudCmdBindingEntry, duplicating the display text, assigning the command id,
 * and appending it to the binding vector with growth when capacity is full.
 */
int HudCmdBindButtonBase::AddBindingEntry(
    const char *displayText,
    int commandId
) {
    HudCmdBindingEntry **begin = (HudCmdBindingEntry **)(bindingVec.begin);
    HudCmdBindingEntry **end = (HudCmdBindingEntry **)(bindingVec.end);
    HudCmdBindingEntry **capacity = (HudCmdBindingEntry **)(bindingVec.capacity);
    const int oldCount = begin != 0 ? (int)(end - begin) : 0;

    HudCmdBindingEntry *const entry =
        (HudCmdBindingEntry *)(::operator new(sizeof(HudCmdBindingEntry)));
    if (entry != 0) {
        new (entry) HudCmdBindingEntry;
        entry->displayText = _strdup(displayText);
        entry->commandId = commandId;
    }

    const int hasCapacity = begin != 0 && (capacity - end) >= 1;
    if (!hasCapacity) {
        const int growCount = oldCount > 1 ? oldCount : 1;
        const int newCapacityCount = oldCount + growCount;
        HudCmdBindingEntry **const newBegin = (HudCmdBindingEntry **)(::operator new(
            (unsigned int)newCapacityCount * sizeof(HudCmdBindingEntry *)
        ));

        for (int index = 0; index < oldCount; ++index) {
            newBegin[index] = begin[index];
        }
        newBegin[oldCount] = entry;

        ::operator delete(begin);
        bindingVec.begin = newBegin;
        bindingVec.end = newBegin + oldCount + 1;
        bindingVec.capacity = newBegin + newCapacityCount;
    } else {
        *end = entry;
        bindingVec.end = end + 1;
    }

    return oldCount;
}

// Reimplements 0x4b9320: HudCmdBindButtonBase::OnSelectedIndexChanged
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void HudCmdBindButtonBase::OnSelectedIndexChanged(
    int selectedIndex
) {
    SetSelectedEntry(selectedIndex);
}

// Reimplements 0x4b9330: HudCmdBindButtonBase::SetSelectedEntry
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void HudCmdBindButtonBase::SetSelectedEntry(
    int selectedIndex
) {
    HudCmdBindingEntry **const entries = (HudCmdBindingEntry **)(bindingVec.begin);
    const int entryCount =
        entries != 0 ? (int)((HudCmdBindingEntry **)(bindingVec.end) - entries) : 0;

    int slotIndex;
    for (slotIndex = 0; slotIndex < visibleBindingSlotCount; ++slotIndex) {
        HudUiListSelectorItem *const item = &bindingSlotPanels[slotIndex];
        const int entryIndex = selectedIndex + slotIndex - visibleBindingSlotCount;
        if (entryIndex >= 0 && entryIndex < entryCount) {
            item->entryIndex = entryIndex;
            item->SetTextFmt(
                "%s",
                entries[entryIndex]->displayText
            );
            item->SetVisible(1);
        } else {
            item->SetVisible(0);
            item->Draw();
        }

        item->Invalidate();
    }

    if (selectedIndex >= 0 && selectedIndex < entryCount) {
        bindPanel.entryIndex = selectedIndex;
        bindPanel.SetTextFmt(
            "%s",
            entries[selectedIndex]->displayText
        );
    }

    for (slotIndex = visibleBindingSlotCount; slotIndex < bindingSlotTotalCount; ++slotIndex) {
        HudUiListSelectorItem *const item = &bindingSlotPanels[slotIndex];
        const int entryIndex = selectedIndex + slotIndex - visibleBindingSlotCount + 1;
        if (entryIndex >= 0 && entryIndex < entryCount) {
            item->entryIndex = entryIndex;
            item->SetTextFmt(
                "%s",
                entries[entryIndex]->displayText
            );
            item->SetVisible(1);
        } else {
            item->SetVisible(0);
            item->Draw();
        }

        item->Invalidate();
    }

    selectedBindingIndex = selectedIndex;
}

// Reimplements 0x40be00: HudCmdBinding::DestroyRange
// (HudCmdDialog.cpp)
HudCmdBinding **__fastcall HudCmdBinding::DestroyRange(
    HudCmdBinding **first,
    HudCmdBinding **last,
    HudCmdBinding **dest,
    void *unusedAlloc
) {
    (void)unusedAlloc;

    while (first != last) {
        HudCmdBinding *const binding = *first;
        if (binding != 0) {
            if (binding->displayText != 0) {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *dest = 0;
        ++first;
        ++dest;
    }

    return dest;
}

// Reimplements 0x40bdc0: zUtil_StdPtrVector_Clear
void **__fastcall zUtil_StdPtrVector_Clear(
    HudCmdBindingVector *self
) {
    void **const oldEnd = (void **)(self->end);
    self->end = self->begin;
    return oldEnd;
}

// Reimplements 0x4ba470: zUtil_StdPtrVector_FreeBufferAndReset
void __fastcall zUtil_StdPtrVector_FreeBufferAndReset(
    HudCmdBindingVector *self
) {
    ::operator delete(self->begin);
    self->begin = 0;
    self->end = 0;
    self->capacity = 0;
}

// Reimplements 0x40c1d0: HudCmdBindButtonBase::ClearBindingEntries
void HudCmdBindButtonBase::ClearBindingEntries() {
    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(bindingVec.end);

    while (entry != end) {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0) {
            if (binding->displayText != 0) {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    zUtil_StdPtrVector_Clear(&bindingVec);
}

// Reimplements 0x40c280: HudCmdBindButtonBase::DestructorCore
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void HudCmdBindButtonBase::DestructorCore() {

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(bindingVec.end);
    while (entry != end) {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0) {
            if (binding->displayText != 0) {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd = (HudCmdBindingEntry **)(bindingVec.end);
    bindingVec.end =
        HudCmdBindingEntry::CopyRange(
            oldEnd,
            oldEnd,
            (HudCmdBindingEntry **)(bindingVec.begin)
        );
    ((StdPtrVector *)(&bindingVec))->ClearNoOpDestroy(
        (int *)(bindingVec.end),
        (int *)oldEnd
    );
    ::operator delete(bindingVec.begin);
    bindingVec.begin = 0;
    bindingVec.end = 0;
    bindingVec.capacity = 0;

    ((HudUiPanel *)(&bindPanel))->Destructor();
    HudUiCheckToggleWidget::DestructorCore();
}

// Reimplements 0x40a940: HudCmdCommandList::Destructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void HudCmdCommandList::Destructor() {

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(bindingVec.end);
    while (entry != end) {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0) {
            if (binding->displayText != 0) {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd = (HudCmdBindingEntry **)(bindingVec.end);
    HudCmdBindingEntry **const oldBegin = (HudCmdBindingEntry **)(bindingVec.begin);
    bindingVec.end = HudCmdBindingEntry::CopyRange(
        oldEnd,
        oldEnd,
        oldBegin
    );
    ((StdPtrVector *)(&bindingVec))
        ->ClearNoOpDestroy(
            (int *)(bindingVec.end),
            (int *)(oldEnd)
        );
    ::operator delete(bindingVec.begin);
    bindingVec.begin = 0;
    bindingVec.end = 0;
    bindingVec.capacity = 0;

    ((HudUiPanel *)(&bindPanel))->Destructor();
    DestructorCore();
}

// Reimplements 0x40b0a0: HudCmdCommandList::ScalarDeletingDestructor
HudUiElement * HudCmdCommandList::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40aa30: HudCmdKeyAButton::Destructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void HudCmdKeyAButton::Destructor() {

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(bindingVec.end);
    while (entry != end) {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0) {
            if (binding->displayText != 0) {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd = (HudCmdBindingEntry **)(bindingVec.end);
    HudCmdBindingEntry **const oldBegin = (HudCmdBindingEntry **)(bindingVec.begin);
    bindingVec.end = HudCmdBindingEntry::CopyRange(
        oldEnd,
        oldEnd,
        oldBegin
    );
    ((StdPtrVector *)(&bindingVec))
        ->ClearNoOpDestroy(
            (int *)(bindingVec.end),
            (int *)(oldEnd)
        );
    ::operator delete(bindingVec.begin);
    bindingVec.begin = 0;
    bindingVec.end = 0;
    bindingVec.capacity = 0;

    ((HudUiPanel *)(&bindPanel))->Destructor();
    DestructorCore();
}

// Reimplements 0x40b0c0: HudCmdKeyAButton::ScalarDeletingDestructor
HudUiElement * HudCmdKeyAButton::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40ab20: HudCmdKeyBButton::Destructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void HudCmdKeyBButton::Destructor() {

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(bindingVec.end);
    while (entry != end) {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0) {
            if (binding->displayText != 0) {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd = (HudCmdBindingEntry **)(bindingVec.end);
    HudCmdBindingEntry **const oldBegin = (HudCmdBindingEntry **)(bindingVec.begin);
    bindingVec.end = HudCmdBindingEntry::CopyRange(
        oldEnd,
        oldEnd,
        oldBegin
    );
    ((StdPtrVector *)(&bindingVec))
        ->ClearNoOpDestroy(
            (int *)(bindingVec.end),
            (int *)(oldEnd)
        );
    ::operator delete(bindingVec.begin);
    bindingVec.begin = 0;
    bindingVec.end = 0;
    bindingVec.capacity = 0;

    ((HudUiPanel *)(&bindPanel))->Destructor();
    DestructorCore();
}

// Reimplements 0x40b0e0: HudCmdKeyBButton::ScalarDeletingDestructor
HudUiElement * HudCmdKeyBButton::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40ac10: HudCmdJoyButton::Destructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void HudCmdJoyButton::Destructor() {

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(bindingVec.end);
    while (entry != end) {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0) {
            if (binding->displayText != 0) {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd = (HudCmdBindingEntry **)(bindingVec.end);
    HudCmdBindingEntry **const oldBegin = (HudCmdBindingEntry **)(bindingVec.begin);
    bindingVec.end = HudCmdBindingEntry::CopyRange(
        oldEnd,
        oldEnd,
        oldBegin
    );
    ((StdPtrVector *)(&bindingVec))
        ->ClearNoOpDestroy(
            (int *)(bindingVec.end),
            (int *)(oldEnd)
        );
    ::operator delete(bindingVec.begin);
    bindingVec.begin = 0;
    bindingVec.end = 0;
    bindingVec.capacity = 0;

    ((HudUiPanel *)(&bindPanel))->Destructor();
    DestructorCore();
}

// Reimplements 0x40b100: HudCmdJoyButton::ScalarDeletingDestructor
HudUiElement * HudCmdJoyButton::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40ad00: HudCmdMouseButton::Destructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void HudCmdMouseButton::Destructor() {

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(bindingVec.end);
    while (entry != end) {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0) {
            if (binding->displayText != 0) {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd = (HudCmdBindingEntry **)(bindingVec.end);
    HudCmdBindingEntry **const oldBegin = (HudCmdBindingEntry **)(bindingVec.begin);
    bindingVec.end = HudCmdBindingEntry::CopyRange(
        oldEnd,
        oldEnd,
        oldBegin
    );
    ((StdPtrVector *)(&bindingVec))
        ->ClearNoOpDestroy(
            (int *)(bindingVec.end),
            (int *)(oldEnd)
        );
    ::operator delete(bindingVec.begin);
    bindingVec.begin = 0;
    bindingVec.end = 0;
    bindingVec.capacity = 0;

    ((HudUiPanel *)(&bindPanel))->Destructor();
    DestructorCore();
}

// Reimplements 0x40b120: HudCmdMouseButton::ScalarDeletingDestructor
HudUiElement * HudCmdMouseButton::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Restores repeated inline bind-button cleanup observed in 0x40adf0; no
// standalone helper exists in the retail executable.
static void HudCmdDialog_DestroyBindButtonRange(
    HudCmdBindButtonBase *button
) {

    HudCmdBinding **const begin = (HudCmdBinding **)(button->bindingVec.begin);
    HudCmdBinding **const end = (HudCmdBinding **)(button->bindingVec.end);
    HudCmdBinding::DestroyRange(
        begin,
        end,
        begin,
        button
    );
    zUtil_StdPtrVector_Clear(&button->bindingVec);
    zUtil_StdPtrVector_FreeBufferAndReset(&button->bindingVec);
    ((HudUiPanel *)(&button->bindPanel))->Destructor();
    button->HudUiCheckToggleWidget::DestructorCore();
}

// Restores the mouse-button cleanup variant observed in 0x40adf0; no
// standalone helper exists in the retail executable.
static void HudCmdDialog_DestroyMouseButton(
    HudCmdBindButtonBase *button
) {

    button->ClearBindingEntries();
    zUtil_StdPtrVector_FreeBufferAndReset(&button->bindingVec);
    ((HudUiPanel *)(&button->bindPanel))->Destructor();
    button->HudUiCheckToggleWidget::DestructorCore();
}

// Restores repeated inline binding-vector clearing observed in 0x40b680; no
// standalone helper exists in the retail executable.
static void HudCmdDialog_ClearBindButtonEntries(
    HudCmdBindButtonBase *button
) {
    button->ClearBindingEntries();
}

// Reimplements 0x40a5b0: HudCmdDialog::Constructor
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
HudCmdDialog * HudCmdDialog::Constructor() {
    new ((HudUiBackground *)this) HudUiBackground;

    new (&resumeButton) HudCmdSimpleWidget;
    new (&resetButton) HudCmdResetButton;

    new (&commandList) HudCmdCommandList;
    new (&keyAButton) HudCmdKeyAButton;
    new (&keyBButton) HudCmdKeyBButton;
    new (&joyButton) HudCmdJoyButton;
    new (&mouseButton) HudCmdMouseButton;

    setList.HudUiCycleSelectorWidget::Constructor();
    new (&nextSetButton) HudCmdNextSetButton;
    new (&prevSetButton) HudCmdPrevSetButton;
    new (&nextCommandButton) HudCmdNextCommandButton;
    new (&prevCommandButton) HudCmdPrevCommandButton;

    new ((HudUiTransitionTextPanel *)(&promptPanel)) HudUiTransitionTextPanel;
    descriptionPanel.HudUiPanel::ConstructorDefault(
        0,
        0,
        0
    );
    zReader::Node *const loadedSection = HudUiBackground::LoadFromZrd(
        "dialog.zrd",
        "COMMANDS_DIALOG",
        0
    );
    if (loadedSection != 0) {
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&resumeButton),
            "CMD_RESUME_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&resetButton),
            "CMD_RESET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&commandList),
            "CMD_COMMAND_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&keyAButton),
            "CMD_KEYA_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&keyBButton),
            "CMD_KEYB_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&joyButton),
            "CMD_JOY_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&mouseButton),
            "CMD_MOUSE_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&setList),
            "CMD_SET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&nextSetButton),
            "CMD_NEXT_SET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&prevSetButton),
            "CMD_PREV_SET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&nextCommandButton),
            "CMD_NEXT_CMD_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiWidget *)(&prevCommandButton),
            "CMD_PREV_CMD_BTN"
        );

        HudUiBackground::BindPrimitiveNodeToElement(
            loadedSection,
            (HudUiElement *)(&promptPanel),
            "PRESS_A_KEY"
        );
        HudUiBackground::BindPrimitiveNodeToElement(
            loadedSection,
            (HudUiElement *)(&descriptionPanel),
            "CMD_DESCRIPTION"
        );
        HudUiBackground::FreeLoadedTreeRoots(0);
        promptPanel.SetFlashRate(1.0f);
    }

    promptPanel.SetVisible(0);

    const int groupCount = zInput::BindGroupList_GetCount();
    for (int groupIndex = 0; groupIndex < groupCount; ++groupIndex) {
        setList.AddTextEntry(
            groupIndex,
            zInput::BindGroupList_GetGroupTitle(groupIndex),
            setList.textOffsetX,
            setList.textOffsetY
        );
        setList.ApplyFontStyleForEntry(
            groupIndex,
            (int)((unsigned int)(setList.fontStyleRef))
        );
    }

    RebuildCommandBindingListsForGroup(0);
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    ((HudUiContainer *)(this))->SetChildFlags(0);
    return this;
}

// Reimplements 0x40adf0: HudCmdDialog::Destructor
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdDialog::Destructor() {
    descriptionPanel.HudUiPanel::Destructor();
    ((HudUiPanel *)(&promptPanel))->Destructor();
    prevCommandButton.HudUiZrdWidget::DestructorCore();
    nextCommandButton.HudUiZrdWidget::DestructorCore();
    prevSetButton.HudUiZrdWidget::DestructorCore();
    nextSetButton.HudUiZrdWidget::DestructorCore();
    setList.HudUiCycleSelectorWidget::DestructorCore();

    HudCmdDialog_DestroyMouseButton(&mouseButton);
    HudCmdDialog_DestroyBindButtonRange(&joyButton);
    HudCmdDialog_DestroyBindButtonRange(&keyBButton);
    HudCmdDialog_DestroyBindButtonRange(&keyAButton);
    HudCmdDialog_DestroyBindButtonRange(&commandList);

    resetButton.HudUiZrdWidget::DestructorCore();
    resumeButton.HudUiZrdWidget::DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

// Reimplements 0x40a920: HudCmdDialog::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
HudCmdDialog * HudCmdDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();

    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40c6e0: HudUiOptionsPanelBackButton::OnActivate
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
void HudUiOptionsPanelBackButton::OnActivate() {
    HudOptionsDialog *const ownerDialog = (HudOptionsDialog *)(owner);
    const int hudType = ownerDialog->fullHudToggle.checked != 0 ? ZOPT_HUD_TYPE_PERSPECTIVE
                                                                : ZOPT_HUD_TYPE_STANDARD;
    zOpt::SetHudTypeForCurrentHwMode(hudType);

    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

void HudUiOptionsPanel_Lighting::OnActivate() {
    SyncFromOptions();
}

void HudUiOptionsPanel_Lighting::PostLoadFromZrd() {
    InitFromOptions();
}

// Reimplements 0x40c9c0: HudUiOptionsPanel_Lighting::InitFromOptions
void HudUiOptionsPanel_Lighting::InitFromOptions() {
    SetChecked(zOpt::GetGraphicsFlagsForCurrentHwMode() & ZOPT_GRAPHICS_GLOBAL_LIGHT);
}

// Reimplements 0x40c9e0: HudUiOptionsPanel_Lighting::SyncFromOptions
void HudUiOptionsPanel_Lighting::SyncFromOptions() {
    const int flags = zOpt::GetGraphicsFlagsForCurrentHwMode();
    HudUiCheckToggleWidget::OnActivate();
    zOpt::SetGraphicsFlagsForCurrentHwMode(
        checked != 0 ? (flags | ZOPT_GRAPHICS_GLOBAL_LIGHT) : (flags & ~ZOPT_GRAPHICS_GLOBAL_LIGHT)
    );
}

void HudUiOptionsPanel_Perspective::OnActivate() {
    SyncFromOptions();
}

void HudUiOptionsPanel_Perspective::PostLoadFromZrd() {
    InitFromOptions();
}

// Reimplements 0x40ca20: HudUiOptionsPanel_Perspective::InitFromOptions
void HudUiOptionsPanel_Perspective::InitFromOptions() {
    SetChecked(zOpt::GetGraphicsFlagsForCurrentHwMode() & ZOPT_GRAPHICS_PERSPECTIVE);
}

// Reimplements 0x40ca40: HudUiOptionsPanel_Perspective::SyncFromOptions
void HudUiOptionsPanel_Perspective::SyncFromOptions() {
    const int flags = zOpt::GetGraphicsFlagsForCurrentHwMode();
    HudUiCheckToggleWidget::OnActivate();
    zOpt::SetGraphicsFlagsForCurrentHwMode(
        checked != 0 ? (flags | ZOPT_GRAPHICS_PERSPECTIVE) : (flags & ~ZOPT_GRAPHICS_PERSPECTIVE)
    );
    zRndr::SelectSpanRoutines();
}

void HudUiOptionsPanel_FullHud::PostLoadFromZrd() {
    InitFromOptions();
}

// Reimplements 0x40ca80: HudUiOptionsPanel_FullHud::InitFromOptions
void HudUiOptionsPanel_FullHud::InitFromOptions() {
    SetChecked(zOpt::GetHudTypeForCurrentHwMode() == ZOPT_HUD_TYPE_PERSPECTIVE);
}

void HudUiOptionsPanel_ObjectDetail::OnActivate() {
    SyncFromOptions();
}

void HudUiOptionsPanel_ObjectDetail::PostLoadFromZrd() {
    InitFromOptions();
}

// Reimplements 0x40cab0: HudUiOptionsPanel_ObjectDetail::InitFromOptions
void HudUiOptionsPanel_ObjectDetail::InitFromOptions() {
    SetIndexClamped(zOpt::GetObjectLODForCurrentHwMode());
}

// Reimplements 0x40cad0: HudUiOptionsPanel_ObjectDetail::SyncFromOptions
void HudUiOptionsPanel_ObjectDetail::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetObjectLODForCurrentHwMode(selectedIndex);
}

void HudUiOptionsPanel_TextureMemory::OnActivate() {
    SyncFromOptions();
}

void HudUiOptionsPanel_TextureMemory::PostLoadFromZrd() {
    InitFromOptions();
}

// Reimplements 0x40caf0: HudUiOptionsPanel_TextureMemory::InitFromOptions
void HudUiOptionsPanel_TextureMemory::InitFromOptions() {
    SetIndexClamped(zOpt::GetTextureMemoryForCurrentHwMode());
}

// Reimplements 0x40cb10: HudUiOptionsPanel_TextureMemory::SyncFromOptions
void HudUiOptionsPanel_TextureMemory::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetTextureMemoryForCurrentHwMode(selectedIndex);
}

void HudUiOptionsPanel_Effects::OnActivate() {
    SyncFromOptions();
}

void HudUiOptionsPanel_Effects::PostLoadFromZrd() {
    InitFromOptions();
}

// Reimplements 0x40cb30: HudUiOptionsPanel_Effects::InitFromOptions
void HudUiOptionsPanel_Effects::InitFromOptions() {
    int level = zOpt::GetEffectsLevelForCurrentHwMode();
    if (zVid::GetAccelerationOption() == ZVID_HW_MODE_SOFTWARE) {
        if (level == 0) {
            level = 1;
        }
        SetVisibleRange(
            1,
            3
        );
    }

    SetIndexClamped(level);
}

// Reimplements 0x40cb70: HudUiOptionsPanel_Effects::SyncFromOptions
void HudUiOptionsPanel_Effects::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetEffectsLevelForCurrentHwMode(selectedIndex);
}

void HudUiOptionsPanel_SoundActive::OnActivate() {
    SyncFromOptions();
}

void HudUiOptionsPanel_SoundActive::PostLoadFromZrd() {
    InitFromOptions();
}

// Reimplements 0x40cb90: HudUiOptionsPanel_SoundActive::InitFromOptions
void HudUiOptionsPanel_SoundActive::InitFromOptions() {
    SetChecked(zOpt::GetMuteSoundOption() == 0);
}

// Reimplements 0x40cbb0: HudUiOptionsPanel_SoundActive::SyncFromOptions
void HudUiOptionsPanel_SoundActive::SyncFromOptions() {
    HudUiCheckToggleWidget::OnActivate();
    zOpt::SetMuteSoundOption(checked == 0);
}

void HudUiOptionsPanel_SoundQuality::OnActivate() {
    SyncFromOptions();
}

void HudUiOptionsPanel_SoundQuality::PostLoadFromZrd() {
    InitFromOptions();
}

// Reimplements 0x40cbd0: HudUiOptionsPanel_SoundQuality::InitFromOptions
void HudUiOptionsPanel_SoundQuality::InitFromOptions() {
    SetIndexClamped(zOpt::GetSoundLODOption());
}

// Reimplements 0x40cbf0: HudUiOptionsPanel_SoundQuality::SyncFromOptions
void HudUiOptionsPanel_SoundQuality::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetSoundLODOption(selectedIndex);
}

void HudUiOptionsPanel_SoundVolume::PostLoadFromZrd() {
    SyncFromOptions();
}

// Reimplements 0x40cc10: HudUiOptionsPanel_SoundVolume::SyncFromOptions
void HudUiOptionsPanel_SoundVolume::SyncFromOptions() {
    SetNormalizedValue(zOpt::GetSoundVolumeOption());
}

// Reimplements 0x40cc30: HudUiOptionsPanel_SoundVolume::OnActivate
void HudUiOptionsPanel_SoundVolume::OnActivate() {
    UpdateNormalizedFromCursor();
    zOpt::SetSoundVolumeOption(normalizedValue);
    SetNormalizedValue(zOpt::GetSoundVolumeOption());
}

void HudUiOptionsPanel_MusicEnable::PostLoadFromZrd() {
    SyncFromOptions();
}

// Reimplements 0x40cc60: HudUiOptionsPanel_MusicEnable::SyncFromOptions
void HudUiOptionsPanel_MusicEnable::SyncFromOptions() {
    SetChecked(zSnd::GetCDAudioOption());
}

// Reimplements 0x40cc80: HudUiOptionsPanel_MusicEnable::OnActivate
void HudUiOptionsPanel_MusicEnable::OnActivate() {
    HudUiCheckToggleWidget::OnActivate();
    if (checked == 0) {
        zSnd::SetCDAudioOption(0);
        zSndCd::Stop();
    } else {
        zSnd::SetCDAudioOption(1);
        zSndCd::PlayTrackWithMode(
            2,
            5
        );
    }
}

void HudUiOptionsPanel_MusicVolume::PostLoadFromZrd() {
    SyncFromOptions();
}

// Reimplements 0x40ccc0: HudUiOptionsPanel_MusicVolume::SyncFromOptions
void HudUiOptionsPanel_MusicVolume::SyncFromOptions() {
    unsigned short primaryVolume = 0;
    unsigned short secondaryVolume = 0;
    zSndCd::GetVolume(
        &primaryVolume,
        &secondaryVolume
    );
    SetNormalizedValue((float)(primaryVolume)*ZSND_CD_VOLUME_TO_NORMALIZED);
}

// Reimplements 0x40cd00: HudUiOptionsPanel_MusicVolume::OnActivate
void HudUiOptionsPanel_MusicVolume::OnActivate() {
    UpdateNormalizedFromCursor();
    const unsigned short volume = (unsigned short)(normalizedValue * ZSND_CD_NORMALIZED_TO_VOLUME);
    zSndCd::SetVolume(
        volume,
        volume
    );
}

void HudUiOptionsPanel_Resolution::PostLoadFromZrd() {
    SyncFromOptions();
}

// Reimplements 0x40cd30: HudUiOptionsPanel_Resolution::SyncFromOptions
void HudUiOptionsPanel_Resolution::SyncFromOptions() {
    const int modeCase = zVid::GetVideoModeIndexFromOptions() - 2;
    if ((unsigned int)(modeCase) > 5u) {
        return;
    }

    if (zVid::GetAccelerationOption() == ZVID_HW_MODE_SOFTWARE) {
        switch (modeCase) {
        case 0:
            SetIndexClamped(3);
            SetVisibleRange(
                2,
                4
            );
            return;
        case 1:
            SetIndexClamped(1);
            SetVisibleRange(
                0,
                2
            );
            return;
        case 2:
            SetIndexClamped(2);
            SetVisibleRange(
                2,
                4
            );
            return;
        case 3:
            SetIndexClamped(0);
            SetVisibleRange(
                0,
                2
            );
            return;
        case 4:
            SetIndexClamped(4);
            SetVisibleRange(
                4,
                5
            );
            return;
        case 5:
            SetIndexClamped(5);
            SetVisibleRange(
                5,
                6
            );
            return;
        }
    }

    switch (modeCase) {
    case 0:
        SetIndexClamped(3);
        SetVisibleRange(
            3,
            4
        );
        return;
    case 1:
        SetIndexClamped(1);
        SetVisibleRange(
            1,
            2
        );
        return;
    case 2:
        SetIndexClamped(2);
        SetVisibleRange(
            2,
            3
        );
        return;
    case 3:
        SetIndexClamped(0);
        SetVisibleRange(
            0,
            1
        );
        return;
    case 4:
        SetIndexClamped(4);
        SetVisibleRange(
            4,
            5
        );
        return;
    case 5:
        SetIndexClamped(5);
        SetVisibleRange(
            5,
            6
        );
        return;
    }
}

// Reimplements 0x40ce80: HudUiOptionsPanel_Resolution::OnActivate
void HudUiOptionsPanel_Resolution::OnActivate() {
    AdvanceSelectionAndActivate();
    switch (selectedIndex) {
    case 0:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_640X480);
        return;
    case 1:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_320X240_TO_640X480);
        return;
    case 2:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_640X400);
        return;
    case 3:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_320X200_TO_640X400);
        return;
    case 4:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_800X600);
        return;
    case 5:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_1024X768);
        return;
    }
}

// Reimplements 0x40c720: HudOptionsDialog::HudOptionsDialog
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
HudOptionsDialog::HudOptionsDialog() : HudUiBackground() {
    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        "OPTIONSPANEL",
        0
    );
    if (loadedSection != 0) {
        BindWidgetByName(
            loadedSection,
            &backButton,
            "BACK"
        );
        BindWidgetByName(
            loadedSection,
            &lightingToggle,
            "LIGHTING"
        );
        BindWidgetByName(
            loadedSection,
            &perspectiveToggle,
            "PERSPECTIVE"
        );
        BindWidgetByName(
            loadedSection,
            &fullHudToggle,
            "FULLHUD"
        );
        BindWidgetByName(
            loadedSection,
            &objectDetailSelector,
            "OBJECT_DETAIL"
        );
        BindWidgetByName(
            loadedSection,
            &textureMemorySelector,
            "TEXTURE_MEMORY"
        );
        BindWidgetByName(
            loadedSection,
            &effectsSelector,
            "EFFECTS"
        );
        BindWidgetByName(
            loadedSection,
            &soundActiveToggle,
            "SOUND_ACTIVE"
        );
        BindWidgetByName(
            loadedSection,
            &soundQualitySelector,
            "SOUND_QUALITY"
        );
        BindWidgetByName(
            loadedSection,
            &soundVolumeWidget,
            "SOUND_VOLUME"
        );
        BindWidgetByName(
            loadedSection,
            &musicEnableToggle,
            "MUSIC_ENABLE"
        );
        BindWidgetByName(
            loadedSection,
            &musicVolumeWidget,
            "MUSIC_VOLUME"
        );
        BindWidgetByName(
            loadedSection,
            &resolutionSelector,
            "RESOLUTION_CYCLE"
        );
        FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }
}

// Reimplements 0x40cf60: HudOptionsDialog::DestructorCore
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
void HudOptionsDialog::DestructorCore() {
    resolutionSelector.HudUiCycleSelectorWidget::DestructorCore();
    musicVolumeWidget.HudUiFillBitmap::DestructorCore();
    musicEnableToggle.HudUiCheckToggleWidget::DestructorCore();
    soundVolumeWidget.HudUiFillBitmap::DestructorCore();
    soundQualitySelector.HudUiCycleSelectorWidget::DestructorCore();
    soundActiveToggle.HudUiCheckToggleWidget::DestructorCore();
    effectsSelector.HudUiCycleSelectorWidget::DestructorCore();
    textureMemorySelector.HudUiCycleSelectorWidget::DestructorCore();
    objectDetailSelector.HudUiCycleSelectorWidget::DestructorCore();
    fullHudToggle.HudUiCheckToggleWidget::DestructorCore();
    perspectiveToggle.HudUiCheckToggleWidget::DestructorCore();
    lightingToggle.HudUiCheckToggleWidget::DestructorCore();
    backButton.HudUiZrdWidget::DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

// Reimplements 0x40cf00: HudOptionsDialog::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
HudOptionsDialog * HudOptionsDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40bc20: HudCmdDialogState::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdDialogState::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x40bc30: HudCmdDialogState::StaticInit
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
HudCmdDialogState *HudCmdDialogState::StaticInit() {
    return new (&g_HudCmdDialogState) HudCmdDialogState;
}

// Reimplements 0x40bc40: HudCmdDialogState::RegisterAtExit
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdDialogState::RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x40bc50: HudCmdDialogState::AtExitDestructor
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdDialogState::AtExitDestructor() {
    g_HudCmdDialogState.~HudCmdDialogState();
}

// Reimplements 0x40bda0: HudCmdDialogState::QueueEnter
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdDialogState::QueueEnter() {
    g_RecoilApp.QueuePushState(
        &g_HudCmdDialogState,
        0
    );
}

// Reimplements 0x40bc60: HudCmdDialogState::HudCmdDialogState
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
HudCmdDialogState::HudCmdDialogState() : m_dialog(0) {}

// Reimplements 0x40bcf0: HudCmdDialogState::OnTryBecomeCurrent
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int HudCmdDialogState::OnTryBecomeCurrent() {
    HudCmdDialog *dialog = (HudCmdDialog *) ::operator new(sizeof(HudCmdDialog));
    if (dialog != 0) {
        dialog = dialog->Constructor();
    }
    m_dialog = dialog;

    dialog->SetEnabled(1);
    zInput::Keyboard_Suspend();
    return 1;
}

// Reimplements 0x40bd60: HudCmdDialogState::OnDeactivate
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdDialogState::OnDeactivate() {
    zInput::Keyboard_ResumeFromSuspend();

    HudCmdDialog *dialog = m_dialog;
    if (dialog == 0) {
        return;
    }

    dialog->SetEnabled(0);
    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();

    dialog = m_dialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
    zInput::BindMap_Current_RebuildLookupIndices();
}

// Reimplements 0x40bc90: HudCmdDialogState::~HudCmdDialogState
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
HudCmdDialogState::~HudCmdDialogState() {
    HudCmdDialog *const dialog = m_dialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
        m_dialog = 0;
    }
}

// Reimplements 0x40b5e0: HudCmdDialog::SelectGroupRelative
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int HudCmdDialog::SelectGroupRelative(
    int delta
) {
    int groupIndex = setList.selectedIndex + delta;
    if (groupIndex >= setList.itemCount) {
        groupIndex = 0;
    } else if (groupIndex < 0) {
        groupIndex = setList.itemCount - 1;
    }

    setList.SetIndexClamped(groupIndex);
    const int selectedIndex = setList.selectedIndex;
    RebuildCommandBindingListsForGroup(selectedIndex);
    return selectedIndex;
}

// Reimplements 0x40b630: HudCmdDialog::SelectCommandRelative
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int HudCmdDialog::SelectCommandRelative(
    int delta
) {
    int selectedIndex = delta;
    selectedIndex += commandList.selectedBindingIndex;
    if (selectedIndex >= 0) {
        HudCmdBindingEntry **const begin =
            (HudCmdBindingEntry **)(commandList.bindingVec.begin);
        int count;
        if (begin == 0) {
            count = 0;
        } else {
            count = (int)((HudCmdBindingEntry **)(commandList.bindingVec.end) - begin);
        }
        if (selectedIndex < count) {
            commandList.SetSelectedEntry(selectedIndex);
        }
    }

    const int currentIndex = commandList.selectedBindingIndex;
    OnCommandSelectionChanged(currentIndex);
    return currentIndex;
}

// Reimplements 0x40b930: HudCmdResetButton::OnActivate
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdResetButton::OnActivate() {
    HudCmdDialog *const dialog = (HudCmdDialog *)(owner);
    zInput::BindMap_InitDefaultBindings();
    zInput::BindMap_Current_RebuildLookupIndices();
    dialog->RebuildCommandBindingListsForGroup(dialog->setList.selectedIndex);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40b960: HudCmdSetListWidget::OnActivate
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdSetListWidget::OnActivate() {
    AdvanceSelectionAndActivate();
    ((HudCmdDialog *)(owner))->RebuildCommandBindingListsForGroup(selectedIndex);
}

// Reimplements 0x40ba30: HudCmdKeyAButton::OnBeginCapture
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdKeyAButton::OnBeginCapture() {
    ((HudCmdDialog *)(owner))->descriptionPanel.captureState = 1;
    zInput::ResetAllTransitionState();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40ba60: HudCmdKeyAButton::OnClearBinding
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdKeyAButton::OnClearBinding() {
    const int selectedIndex = selectedBindingIndex;
    ((HudCmdDialog *)(owner))->ApplyPrimaryKeyRebind(
        0,
        selectedIndex
    );
    SetSelectedEntry(selectedIndex);
}

// Reimplements 0x40ba90: HudCmdBindButtonBase::OnSelectionChangedRefresh
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdBindButtonBase::OnSelectionChangedRefresh(
    int selectedIndex
) {
    ((HudCmdDialog *)(owner))->OnCommandSelectionChanged(selectedIndex);
}

// Reimplements 0x40bab0: HudCmdKeyBButton::OnBeginCapture
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdKeyBButton::OnBeginCapture() {
    ((HudCmdDialog *)(owner))->descriptionPanel.captureState = 2;
    zInput::ResetAllTransitionState();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40bae0: HudCmdKeyBButton::OnClearBinding
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdKeyBButton::OnClearBinding() {
    ((HudCmdDialog *)(owner))->ApplySecondaryKeyRebind(
        0,
        selectedBindingIndex
    );
}

// Reimplements 0x40bb00: HudCmdJoyButton::OnBeginCapture
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdJoyButton::OnBeginCapture() {
    ((HudCmdDialog *)(owner))->descriptionPanel.captureState = 3;
    zInput::ResetAllTransitionState();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40bb30: HudCmdJoyButton::OnClearBinding
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdJoyButton::OnClearBinding() {
    ((HudCmdDialog *)(owner))
        ->ApplyJoystickButtonRebind(
            0,
            selectedBindingIndex
        );
}

// Reimplements 0x40bb50: HudCmdMouseButton::OnBeginCapture
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdMouseButton::OnBeginCapture() {
    if (g_HudCmdMouseDebounceFrames > 0) {
        return;
    }

    ((HudCmdDialog *)(owner))->descriptionPanel.captureState = 4;
    zInput::ResetAllTransitionState();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40bb80: HudCmdMouseButton::OnClearBinding
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdMouseButton::OnClearBinding() {
    if (g_HudCmdMouseDebounceFrames > 0) {
        return;
    }

    ((HudCmdDialog *)(owner))->ApplyMouseButtonRebind(
        0,
        selectedBindingIndex
    );
}

// Reimplements 0x40bba0: HudCmdNextSetButton::OnActivate
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdNextSetButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectGroupRelative(1);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40bbc0: HudCmdPrevSetButton::OnActivate
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdPrevSetButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectGroupRelative(-1);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40bbe0: HudCmdNextCommandButton::OnActivate
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdNextCommandButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectCommandRelative(1);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40bc00: HudCmdPrevCommandButton::OnActivate
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdPrevCommandButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectCommandRelative(-1);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40b680: HudCmdDialog::RebuildCommandBindingListsForGroup
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdDialog::RebuildCommandBindingListsForGroup(
    int groupIndex
) {
    HudCmdDialog_ClearBindButtonEntries(&commandList);
    HudCmdDialog_ClearBindButtonEntries(&keyAButton);
    HudCmdDialog_ClearBindButtonEntries(&keyBButton);
    HudCmdDialog_ClearBindButtonEntries(&joyButton);
    HudCmdDialog_ClearBindButtonEntries(&mouseButton);

    int commandIndex;
    for (commandIndex = 0; commandIndex < zInput::BindGroupList_GetGroupCommandCount(groupIndex);
        ++commandIndex) {
        const int commandId = zInput::BindGroupList_GetGroupCommandId(
            groupIndex,
            commandIndex
        );
        char labelBuffer[40];
        zInput::BindMapCurrent_CopyCommandLabel(
            commandId,
            labelBuffer,
            sizeof(labelBuffer)
        );
        if (strlen(labelBuffer) != 0) {
            commandList.AddBindingEntry(
                zInput::BindMap_GetCommandLabel(commandId),
                commandId
            );
            keyAButton.AddBindingEntry(
                zInput::BindMapCurrent_FormatKeyComboName(
                    zInput::BindMapCurrent_GetPrimaryKeyboardKey(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
            keyBButton.AddBindingEntry(
                zInput::BindMapCurrent_FormatKeyComboName(
                    zInput::BindMapCurrent_GetSecondaryKeyboardKey(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
            joyButton.AddBindingEntry(
                zInput::BindMapCurrent_CopyJoystickButtonName(
                    zInput::BindMapCurrent_GetJoystickButtonSlot(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
            mouseButton.AddBindingEntry(
                zInput::BindMapCurrent_CopyMouseButtonName(
                    zInput::BindMapCurrent_GetMouseButtonSlot(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
        }
    }

    OnCommandSelectionChanged(0);
}

// Reimplements 0x40b980: HudCmdDialog::OnCommandSelectionChanged
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdDialog::OnCommandSelectionChanged(
    int commandIndex
) {
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    commandList.SetSelectedEntry(commandIndex);
    keyAButton.SetSelectedEntry(commandIndex);
    keyBButton.SetSelectedEntry(commandIndex);
    joyButton.SetSelectedEntry(commandIndex);
    mouseButton.SetSelectedEntry(commandIndex);

    HudCmdBindingEntry **const entries = (HudCmdBindingEntry **)(commandList.bindingVec.begin);
    HudCmdBindingEntry *const selectedEntry = entries[commandList.selectedBindingIndex];
    char *const hint = zInput::BindMap_GetCommandHint(selectedEntry->commandId);
    if (hint != 0) {
        descriptionPanel.SetTextFmt(
            "%s",
            hint
        );
    } else {
        descriptionPanel.SetTextFmt("");
    }
}

// Reimplements 0x40b140: HudCmdDialog::UpdateCaptureState
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void HudCmdDialog::UpdateCaptureState(
    float deltaTime
) {
    HudUiBackground::Update(deltaTime);

    switch (descriptionPanel.captureState) {
    case 0:
        promptPanel.SetVisible(0);
        --g_HudCmdMouseDebounceFrames;
        break;

    case 1: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired keyboard key.");
        keyBButton.SetChecked(0);
        joyButton.SetChecked(0);
        mouseButton.SetChecked(0);

        const int keyCode = zInput::Keyboard_WaitForAnyKeyPress(0);
        if (keyCode != 0) {
            ApplyPrimaryKeyRebind(
                keyCode,
                keyAButton.selectedBindingIndex
            );
            keyAButton.SetChecked(0);
        }
        break;
    }

    case 2: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired keyboard key.");
        keyAButton.SetChecked(0);
        joyButton.SetChecked(0);
        mouseButton.SetChecked(0);

        const int keyCode = zInput::Keyboard_WaitForAnyKeyPress(0);
        if (keyCode != 0) {
            ApplySecondaryKeyRebind(
                keyCode,
                keyBButton.selectedBindingIndex
            );
            keyBButton.SetChecked(0);
        }
        break;
    }

    case 3: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired joystick button.");
        keyAButton.SetChecked(0);
        keyBButton.SetChecked(0);
        mouseButton.SetChecked(0);

        if (zInput::Keyboard_WaitForAnyKeyPress(0) == 1) {
            descriptionPanel.captureState = 0;
            zInput::ResetAllTransitionState();
            joyButton.SetChecked(0);
            return;
        }

        const int buttonCode = zInput::DI_WaitForButtonPress(0);
        if (buttonCode != 0) {
            ApplyJoystickButtonRebind(
                buttonCode,
                joyButton.selectedBindingIndex
            );
            joyButton.SetChecked(0);
        }
        break;
    }

    case 4: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired mouse button.");
        keyAButton.SetChecked(0);
        keyBButton.SetChecked(0);
        joyButton.SetChecked(0);

        if (zInput::Keyboard_WaitForAnyKeyPress(0) == 1) {
            descriptionPanel.captureState = 0;
            zInput::ResetAllTransitionState();
            joyButton.SetChecked(0);
            return;
        }

        const int buttonCode = zInput::Mouse_WaitForButtonPress(0);
        if (buttonCode != 0) {
            ApplyMouseButtonRebind(
                buttonCode,
                mouseButton.selectedBindingIndex
            );
            mouseButton.SetChecked(0);
            g_HudCmdMouseDebounceFrames = 10;
        }
        break;
    }

    default:
        break;
    }
}

// Reimplements 0x40b3e0: HudCmdDialog::ApplyPrimaryKeyRebind
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int HudCmdDialog::ApplyPrimaryKeyRebind(
    int keyCode,
    int commandIndex
) {
    if (keyCode != 1) {
        const int primaryCommand = zInput::BindMapCurrent_GetCommandByPrimaryKey(keyCode);
        const int groupIndex = setList.selectedIndex;
        const int commandId = zInput::BindGroupList_GetGroupCommandId(
            groupIndex,
            commandIndex
        );
        if (primaryCommand == 0 && zInput::BindMapCurrent_GetCommandBySecondaryKey(keyCode) != 0) {
            zInput::BindMapCurrent_SetSecondaryKeyBinding(
                keyCode,
                0
            );
        }

        zInput::BindMapCurrent_SetPrimaryKeyBinding(
            keyCode,
            commandId
        );
        RebuildCommandBindingListsForGroup(groupIndex);
        OnCommandSelectionChanged(commandIndex);
    }

    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

// Reimplements 0x40b460: HudCmdDialog::ApplySecondaryKeyRebind
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int HudCmdDialog::ApplySecondaryKeyRebind(
    int keyCode,
    int commandIndex
) {
    if (keyCode != 1) {
        const int secondaryCommand = zInput::BindMapCurrent_GetCommandBySecondaryKey(keyCode);
        const int groupIndex = setList.selectedIndex;
        const int commandId = zInput::BindGroupList_GetGroupCommandId(
            groupIndex,
            commandIndex
        );
        if (secondaryCommand == 0 && zInput::BindMapCurrent_GetCommandByPrimaryKey(keyCode) != 0) {
            zInput::BindMapCurrent_SetPrimaryKeyBinding(
                keyCode,
                0
            );
        }

        zInput::BindMapCurrent_SetSecondaryKeyBinding(
            keyCode,
            commandId
        );
        RebuildCommandBindingListsForGroup(groupIndex);
        OnCommandSelectionChanged(commandIndex);
    }

    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

// Reimplements 0x40b4e0: HudCmdDialog::ApplyJoystickButtonRebind
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int HudCmdDialog::ApplyJoystickButtonRebind(
    int buttonCode,
    int commandIndex
) {
    const int joystickCommand = zInput::BindMapCurrent_GetCommandByJoystickSlot(buttonCode);
    const int groupIndex = setList.selectedIndex;
    const int commandId = zInput::BindGroupList_GetGroupCommandId(
        groupIndex,
        commandIndex
    );
    if (joystickCommand == 0 && zInput::BindMapCurrent_GetCommandByJoystickSlot(buttonCode) != 0) {
        zInput::BindMapCurrent_SetJoystickBinding(
            buttonCode,
            0
        );
    }

    zInput::BindMapCurrent_SetJoystickBinding(
        buttonCode,
        commandId
    );
    RebuildCommandBindingListsForGroup(groupIndex);
    OnCommandSelectionChanged(commandIndex);
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

// Reimplements 0x40b560: HudCmdDialog::ApplyMouseButtonRebind
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int HudCmdDialog::ApplyMouseButtonRebind(
    int buttonCode,
    int commandIndex
) {
    const int mouseCommand = zInput::BindMapCurrent_GetCommandByMouseSlot(buttonCode);
    const int groupIndex = setList.selectedIndex;
    const int commandId = zInput::BindGroupList_GetGroupCommandId(
        groupIndex,
        commandIndex
    );
    if (mouseCommand == 0 && zInput::BindMapCurrent_GetCommandByMouseSlot(buttonCode) != 0) {
        zInput::BindMapCurrent_SetMouseBinding(
            buttonCode,
            0
        );
    }

    zInput::BindMapCurrent_SetMouseBinding(
        buttonCode,
        commandId
    );
    RebuildCommandBindingListsForGroup(groupIndex);
    OnCommandSelectionChanged(commandIndex);
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

// Reimplements 0x4b90e0: HudCmdBindButtonBase::RebuildBindingSlotWidgets
void HudCmdBindButtonBase::RebuildBindingSlotWidgets(
    int totalCount,
    int visibleCount
) {
    DeleteHudUiListSelectorItemArray(bindingSlotPanels);
    bindingSlotPanels = 0;

    const unsigned int allocationSize =
        sizeof(int) + (unsigned int)(totalCount) * sizeof(HudUiListSelectorItem);
    HudUiListSelectorItemArrayHeader *const header =
        (HudUiListSelectorItemArrayHeader *)(::operator new(allocationSize));
    header->count = totalCount;
    HudUiListSelectorItem *const items = (HudUiListSelectorItem *)(header + 1);
    {
        for (int index = 0; index < totalCount; ++index) {
            new (&items[index]) HudUiListSelectorItem;
        }
    }

    bindingSlotPanels = items;
    bindingSlotTotalCount = totalCount;
    visibleBindingSlotCount = visibleCount;

    {
        for (int index = 0; index < visibleBindingSlotCount; ++index) {
            const int x = (int)((float)(originX) + visibleListOffsetX);
            const int y = (int)((float)(originY +
                                        (index - visibleBindingSlotCount) * bindingSlotSpacing) +
                                visibleListOffsetY);
            bindingSlotPanels[index].SetPos(
                x,
                y
            );
        }
    }

    bindPanel.SetPos(
        originX,
        originY
    );

    {
        for (int index = visibleBindingSlotCount; index < bindingSlotTotalCount; ++index) {
            const int x = (int)((float)(originX) + overflowListOffsetX);
            const int y = (int)((float)(originY + (index - visibleBindingSlotCount + 1) *
                                                                bindingSlotSpacing) +
                                overflowListOffsetY);
            bindingSlotPanels[index].SetPos(
                x,
                y
            );
        }
    }
}

// Reimplements 0x4b8de0: HudCmdBindButtonBase::LoadFromZrd
int HudCmdBindButtonBase::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiCheckToggleWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );

    void *const clipSource = ownerDialog->capturedCompositeImage;

    zReader::Node *const selectedFontNode = zReader_GetNamedNode(
        zrdSection,
        "SELECTED_FONT"
    );
    if (selectedFontNode != 0) {
        selectedFontStyleRef = selectedFontNode->value.i32;
        ApplyHudFontStyleTextOnly(
            (HudUiPanel *)(&bindPanel),
            HudUiZrdOwnerFontStyle(owner, selectedFontStyleRef)
        );
    }

    zReader::Node *const listFontNode = zReader_GetNamedNode(
        zrdSection,
        "LIST_FONT"
    );
    if (listFontNode != 0) {
        listFontStyleRef = listFontNode->value.i32;
    }

    zReader::Node *const spacingNode = zReader_GetNamedNode(
        zrdSection,
        "SPACING"
    );
    if (spacingNode != 0) {
        bindingSlotSpacing = spacingNode->value.i32;
    }

    zReader::Node *const listOffsetNode = zReader_GetNamedNode(
        zrdSection,
        "LIST_OFFSET"
    );
    zReader::Node *const listOffsetBase = ZrdArrayBase(listOffsetNode);
    zReader::Node *const visibleOffsetBase = ZrdArrayBase(ZrdArrayItem(
        listOffsetBase,
        1
    ));
    zReader::Node *const overflowOffsetBase = ZrdArrayBase(ZrdArrayItem(
        listOffsetBase,
        2
    ));
    if (visibleOffsetBase != 0 && overflowOffsetBase != 0) {
        visibleListOffsetX = (float)(ZrdArrayInt(
            visibleOffsetBase,
            1,
            0
        ));
        visibleListOffsetY = (float)(ZrdArrayInt(
            visibleOffsetBase,
            2,
            0
        ));
        overflowListOffsetX = (float)(ZrdArrayInt(
            overflowOffsetBase,
            1,
            0
        ));
        overflowListOffsetY = (float)(ZrdArrayInt(
            overflowOffsetBase,
            2,
            0
        ));
    }

    zReader::Node *const listSizeNode = zReader_GetNamedNode(
        zrdSection,
        "LISTSIZE"
    );
    zReader::Node *const listSizeBase = ZrdArrayBase(listSizeNode);
    if (listSizeBase != 0) {
        const int visibleCount =
            ZrdArrayCount(listSizeBase) > 2 ? ZrdArrayInt(
                listSizeBase,
                2,
                0
            ) : 0;
        RebuildBindingSlotWidgets(
            ZrdArrayInt(
                listSizeBase,
                1,
                0
            ),
            visibleCount
        );

        HudUiRect clipRect = {0};
        const HudFontStyle *const listStyle =
            HudUiZrdOwnerFontStyle(
                owner,
                listFontStyleRef
            );
        {
            for (int index = 0; index < bindingSlotTotalCount; ++index) {
                HudUiListSelectorItem *const item = &bindingSlotPanels[index];
                ((HudUiContainer *)(ownerDialog))->AddChild((HudUiElement *)(item));
                item->SetVisible(1);
                item->owner = this;
                if (clipSource != 0) {
                    HudUiSetPanelClipWithSource(
                        item,
                        clipSource,
                        &clipRect
                    );
                }

                ApplyHudFontStyleTextOnly(
                    (HudUiPanel *)(item),
                    listStyle
                );
            }
        }

        ((HudUiContainer *)(ownerDialog))->AddChild((HudUiElement *)(&bindPanel));
        bindPanel.SetVisible(1);
        bindPanel.owner = this;
        if (clipSource != 0) {
            HudUiSetPanelClipWithSource(
                &bindPanel,
                clipSource,
                &clipRect
            );
        }
    }

    return 1;
}

int HudUiDialogSignedDivPow2(
    int value,
    int shift
) {
    const int signMask = value >> 31;
    return (value + (signMask & ((1 << shift) - 1))) >> shift;
}

zVidImagePartial *HudUiMessageBoxCreateSolidImage(
    int width,
    int height,
    unsigned short color565
) {
    zVidImagePartial *const image = zVid_Image::Create();
    zVid_Image::SetFormatCode(
        image,
        1
    );
    zVid_Image::SetSize(
        image,
        (short)(width),
        (short)(height)
    );

    void *const pixels = malloc(zVid_Image::QueryBytesPerPixel(image) * width * height);
    zVid_Image_SetPixels(
        image,
        pixels,
        0
    );

    unsigned short *const pixelWords = (unsigned short *)(pixels);
    for (int index = 0; index < image->pixelCount; ++index) {
        pixelWords[index] = color565;
    }

    return image;
}

// Reimplements 0x4bf060: HudUiMessageBoxDialog::Constructor
HudUiMessageBoxDialog * HudUiMessageBoxDialog::Constructor(
    const char *zrdPath,
    const char *sectionName
) {
    new ((HudUiBackground *)this) HudUiBackground;
    backdropWidget.Constructor(0);
    messagePanel.ConstructorDefault(
        0,
        0,
        0
    );
    titlePanel.ConstructorDefault(
        0,
        0,
        0
    );
    okButton.Constructor();
    cancelButton.Constructor();
    const zVidRect32 *const primaryRect = zVideo::GetPrimarySurfaceRectScratch();
    blitRect = *primaryRect;

    if (zrdPath != 0 && sectionName != 0) {
        backgroundImage = 0;
        okButtonNormalImage = 0;
        okButtonPressedImage = 0;

        zReader::Node *const loadedSection = LoadFromZrd(
            zrdPath,
            sectionName,
            0
        );
        if (loadedSection != 0) {
            BindWidgetByName(
                loadedSection,
                &okButton,
                "MB_OK"
            );
            BindWidgetByName(
                loadedSection,
                &cancelButton,
                "MB_CANCEL"
            );
            BindPrimitiveNodeToElement(
                loadedSection,
                &titlePanel,
                "TITLE"
            );
            BindPrimitiveNodeToElement(
                loadedSection,
                &messagePanel,
                "MESSAGE"
            );
        }

        FreeLoadedTreeRoots(0);
        titlePanel.SetVisible(1);
        messagePanel.SetVisible(1);
        return this;
    }

    const int centerX = HudUiDialogSignedDivPow2(
        blitRect.right,
        1
    );
    const int centerY = HudUiDialogSignedDivPow2(
        blitRect.bottom,
        1
    );
    fallbackWidth = 300;
    fallbackHeight = 200;

    backgroundImage = HudUiMessageBoxCreateSolidImage(
        fallbackWidth,
        fallbackHeight,
        (unsigned short)(zVid_PackColorRGB(
            128,
            128,
            128
        ))
    );

    const int buttonWidth = HudUiDialogSignedDivPow2(
        fallbackWidth,
        2
    );
    const int buttonHeight = HudUiDialogSignedDivPow2(
        fallbackHeight,
        2
    );
    okButtonNormalImage = HudUiMessageBoxCreateSolidImage(
        buttonWidth,
        buttonHeight,
        (unsigned short)(zVid_PackColorRGB(
            192,
            192,
            192
        ))
    );
    okButtonPressedImage = HudUiMessageBoxCreateSolidImage(
        buttonWidth,
        buttonHeight,
        (unsigned short)(zVid_PackColorRGB(
            160,
            192,
            160
        ))
    );

    backdropWidget.SetImageBorrowedAndInvalidate(backgroundImage);
    messagePanel.SetTextFmt("");
    titlePanel.SetTextFmt("");
    okButton.LoadFromZrd(
        0,
        this
    );
    okButton.defaultImage = okButton.SetImageBorrowedAndInvalidate(okButtonNormalImage);
    okButton.rolloverImage = okButtonPressedImage;

    backdropWidget.SetPos(
        centerX - 150,
        centerY - 100
    );
    titlePanel.SetPos(
        centerX - 140,
        centerY - 90
    );
    messagePanel.SetPos(
        centerX - 140,
        centerY - 70
    );
    okButton.SetPos(
        centerX - 150 + HudUiDialogSignedDivPow2(
            fallbackWidth,
            1
        ) -
            HudUiDialogSignedDivPow2(
                fallbackWidth,
                3
            ),
        centerY - 100 - HudUiDialogSignedDivPow2(
            fallbackHeight,
            2
        ) + fallbackHeight - 10
    );

    AddChild(&backdropWidget);
    AddChild(&messagePanel);
    AddChild(&titlePanel);
    AddChild(&okButton);
    messagePanel.SetVisible(1);
    titlePanel.SetVisible(1);
    okButton.SetVisible(0);
    SetChildFlags(0);
    return this;
}

// Reimplements 0x4bf540: HudUiMessageBoxDialog::ScalarDeletingDestructor
HudUiMessageBoxDialog * HudUiMessageBoxDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4bf560: HudUiMessageBoxDialog::Destructor
void HudUiMessageBoxDialog::Destructor() {
    if (backgroundImage != 0) {
        if (backgroundImage->pixels != 0) {
            free(backgroundImage->pixels);
            backgroundImage->pixels = 0;
        }

        zVid_Image::Destroy(backgroundImage);
        backgroundImage = 0;
    }

    cancelButton.DestructorCore();
    okButton.DestructorCore();
    titlePanel.Destructor();
    messagePanel.Destructor();
    backdropWidget.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

// Reimplements 0x4bf630: HudUiMessageBoxDialog::RunModal
int HudUiMessageBoxDialog::RunModal(
    const char *messageText,
    const char *titleText,
    void *modalContext,
    float timeoutSeconds
) {
    (void)modalContext;
    (void)timeoutSeconds;

    if (g_zVideo_ActiveRendererPath != 0) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            0,
            0
        );
    }

    const int previousHalfResMode = zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    zVidRect32 previousRegionRect = {0, 0, 0, 0};
    int previousBitsPerPixel = 0;
    int previousPitchBytes = 0;
    void *const previousPixels = zRndr::GetActiveRegionState(
        &previousRegionRect.right,
        &previousRegionRect.bottom,
        &previousBitsPerPixel,
        &previousPitchBytes
    );

    int dialogPitchBytes;
    int dialogBitsPerPixel;
    void *dialogPixels;
    if (g_zVideo_ActiveRendererPath == 0) {
        dialogPitchBytes = zVideo::GetSwSurfacePitch();
        dialogBitsPerPixel = zVideo::GetDisplayModeBpp();
        dialogPixels = zVideo::GetSwSurfacePixels();
    } else {
        dialogPitchBytes = zVideo::GetPrimarySurfacePitch();
        dialogBitsPerPixel = zVideo::GetDisplayModeBpp();
        dialogPixels = zVideo::GetPrimarySurfacePixels();
    }

    zRndr::SetFrameBufferRegion(
        dialogPixels,
        (zOpt_ViewRectSection *)(&blitRect),
        dialogBitsPerPixel,
        dialogPitchBytes
    );

    modalResult = 0;
    modalFrameCountdown = 100000;
    SetEnabled(1);
    messagePanel.SetTextFmt(messageText);
    titlePanel.SetTextFmt(titleText);
    okButton.SetVisible(1);

    int framesRemaining = modalFrameCountdown;
    modalFrameCountdown = framesRemaining - 1;
    while (framesRemaining > 0) {
        zInput::PollActiveDevices(0);
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();
        Update(g_FrameDeltaTimeSec);
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(
            &blitRect,
            &blitRect,
            1,
            1
        );
        framesRemaining = modalFrameCountdown;
        modalFrameCountdown = framesRemaining - 1;
    }

    ((HudUiDialogController *)(this))->BlitOwnedSurfaceToPrimary();
    SetEnabled(0);
    zVideo::SetHalfResAdjustMode(previousHalfResMode);
    HudUi::SetInvalidateMode(previousHalfResMode);
    zRndr::SetFrameBufferRegion(
        previousPixels,
        (zOpt_ViewRectSection *)(&previousRegionRect),
        previousBitsPerPixel,
        previousPitchBytes
    );
    return modalResult;
}

// Reimplements 0x4bf7c0: HudUiMessageBoxDialog::OnOk
void HudUiMessageBoxDialog::OnOk() {
    modalResult = 1;
    modalFrameCountdown = 0;
}

// Reimplements 0x4bf7e0: HudUiMessageBoxDialog::OnCancel
void HudUiMessageBoxDialog::OnCancel() {
    modalResult = 2;
    modalFrameCountdown = 0;
}

// Reimplements 0x4bf800: HudUiMessageBoxOkButton::OnActivate
void HudUiMessageBoxOkButton::OnActivate() {
    HudUiMessageBoxDialog *const dialog = (HudUiMessageBoxDialog *)(owner);
    dialog->OnOk();

    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x4bf820: HudUiMessageBoxCancelButton::OnActivate
void HudUiMessageBoxCancelButton::OnActivate() {
    HudUiMessageBoxDialog *const dialog = (HudUiMessageBoxDialog *)(owner);
    dialog->OnCancel();

    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40f2d0: HudUiWidget::HudUiWidget
HudUiWidget::HudUiWidget() {
    Constructor(0);
}

HudUiWidget::~HudUiWidget() {
    ReleaseImageIfOwned();
}

// Reimplements 0x404d90: HudUiWidget::GetCenterX
int HudUiWidget::GetCenterX() {
    if (alignFlags != 0) {
        const int width = image != 0 ? image->width : 0;
        return x + (width / 2);
    }

    return x;
}

// Reimplements 0x404dd0: HudUiWidget::GetCenterY
int HudUiWidget::GetCenterY() {
    if (alignFlags != 0) {
        const int height = image != 0 ? image->height : 0;
        return y + (height / 2);
    }

    return y;
}

// Reimplements 0x4b4030: HudUiWidget::HitTest
int HudUiWidget::HitTest(
    int px,
    int py
) {
    if ((flags & 0x10) != 0) {
        return 0;
    }

    HudUiRect *const bounds = GetBoundsRectOrNull();
    if (bounds == 0) {
        return 0;
    }

    return px >= bounds->left && px <= bounds->right && py >= bounds->top && py <= bounds->bottom
               ? 1
               : 0;
}

// Reimplements 0x404e10: HudUiWidget::RebuildBltRectFromImage
RECOIL_NO_GS void HudUiWidget::RebuildBltRectFromImage() {
    zVidImagePartial *const sourceImage = image;
    int right = x;
    int bottom = y;
    HudUiRect rect;

    rect.left = right;
    rect.top = bottom;
    right += sourceImage != 0 ? sourceImage->width : 0;
    rect.right = right;
    bottom += sourceImage != 0 ? sourceImage->height : 0;
    rect.bottom = bottom;

    SetClipRect(&rect);
}

// Reimplements 0x4b3fb0: HudUiWidget::Draw
void HudUiWidget::Draw() {
    if (image == 0) {
        return;
    }

    if (dirtyRectCount != 0) {
        int dirtyRectIndex;
        for (dirtyRectIndex = 0; dirtyRectIndex < 4; ++dirtyRectIndex) {
            HudUiRectDirty &dirtyRect = dirtyRects[dirtyRectIndex];
            if (dirtyRect.framesRemaining == 0) {
                continue;
            }

            zVid_Image::BlitToActiveTarget(
                image,
                dirtyRect.drawX,
                dirtyRect.drawY,
                0,
                (zVidRect32 *)(&dirtyRect.srcLeft)
            );

            --dirtyRect.framesRemaining;
            if (dirtyRect.framesRemaining == 0) {
                --dirtyRectCount;
            }
        }
        return;
    }

    if (g_HudUiWidget_ExclusiveDrawImage != 0 && g_HudUiWidget_ExclusiveDrawImage != image) {
        return;
    }

    DrawBase();

    zVid_Image::BlitToActiveTarget(
        image,
        x,
        y,
        0,
        (zVidRect32 *)(bltClipRectOrNull)
    );
}

/**
 * Reimplements 0x4b3da0: HudUiWidget::ReleaseImageIfOwned.
 * Purpose: release an owned widget image and clear the ownership bit.
 */
void HudUiWidget::ReleaseImageIfOwned() {
    if (image != 0 && ownsImage != 0) {
        zVid_Image::ReleaseIfNotDefault(image);
        image = 0;
    }

    ownsImage = 0;
}

// Reimplements 0x4b3e70: HudUiWidget::SetImageBorrowedAndInvalidate
zVidImagePartial * HudUiWidget::SetImageBorrowedAndInvalidate(
    zVidImagePartial *newImage
) {
    ownsImage = 0;
    image = newImage;
    Invalidate();
    return newImage;
}

/**
 * Reimplements 0x4b3e30: HudUiWidget::SetImageByPathOwned.
 * Purpose: replace an owned widget image from a texture-directory path and invalidate the widget.
 */
zVidImagePartial * HudUiWidget::SetImageByPathOwned(
    const char *imagePath
) {
    if (imagePath == 0) {
        return 0;
    }

    ReleaseImageIfOwned();
    image = zImage::TexDir_FindOrCreateByPath(imagePath);
    if (image != 0) {
        ownsImage = 1;
    }

    Invalidate();
    return image;
}

// Reimplements 0x4b3d50: HudUiWidget::DestructorCore
void HudUiWidget::DestructorCore() {
    this->~HudUiWidget();
}

// Reimplements 0x4b3ce0: HudUiWidget::ScalarDeletingDestructor
HudUiElement * HudUiWidget::ScalarDeletingDestructor(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b3dd0: HudUiWidget::SetPos
void HudUiWidget::SetPos(
    int newX,
    int newY
) {
    if (alignFlags != 0 && image != 0) {
        x = newX - (image->width / 2);
        y = newY - (image->height / 2);
    } else {
        x = newX;
        y = newY;
    }

    Invalidate();
}

// Reimplements 0x40f200: HudUiTripletPanel::Constructor
HudUiTripletPanel * HudUiTripletPanel::Constructor() {
    HudUiElement::Constructor(
        0,
        0
    );
    visibleCount = 0;

    {
        int itemIndex;
        for (itemIndex = 0; itemIndex < (int)(sizeof(items) / sizeof(items[0])); ++itemIndex) {
            HudUiWidget &item = items[itemIndex];
            new (&item) HudUiWidget;
            item.HudUiElement::SetVisible(0);
        }
    }

    g_HudUiMgr.AddChild(this);
    return this;
}

// Reimplements 0x40f2b0: HudUiTripletPanel::ScalarDeletingDestructor
HudUiElement * HudUiTripletPanel::ScalarDeletingDestructor(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40f400: HudUiTripletPanel::Draw
// (D:\Proj\Battlesport\hud.cpp)
void HudUiTripletPanel::Draw() {
    DrawBase();

    {
        for (int index = 2; index >= 0; --index) {
            HudUiWidget &item = items[index];
            if (((unsigned char)(item.flags) & 0x10u) == 0) {
                item.Draw();
            }
        }
    }
}

// Reimplements 0x40f460: HudUiTripletPanel::SetVisibleCount
void HudUiTripletPanel::SetVisibleCount(
    int count
) {
    if (visibleCount == count) {
        return;
    }

    if (count > 4) {
        count = 4;
    } else if (count < 0) {
        count = 0;
    }

    visibleCount = count;

    {
        for (int index = 0; index < 3; ++index) {
            items[index].SetVisible(count > index ? 1 : 0);
        }
    }

    Invalidate();
}

// Reimplements 0x40f2e0: HudUiNanitePanel::InitLayout
// (D:\Proj\Battlesport\hud.cpp)
void HudUiNanitePanel::InitLayout(
    zReader::Node *layoutRoot
) {
    HudUiWidget *const layoutWidget2 = &g_HudLayoutHW.widget2;
    const int baseX = g_HudUiMgrHudOriginX / 2;
    int anchor[2];
    anchor[0] = layoutWidget2->GetCenterX();
    anchor[1] = layoutWidget2->GetCenterY();

    zReader::Node *const layoutPayload = layoutRoot->value.nodes;
    HudUiRect clipRect;
    HudUiLayoutNode::ReadRectOffsetAndSize(
        &layoutPayload[1],
        &clipRect,
        0,
        0,
        0
    );

    zVidImagePartial *const sharedImage =
        HudUiLayoutNode::ApplyImageWidget(
            &layoutPayload[2],
            &items[0],
            baseX,
            0,
            anchor,
            0,
            0
        );
    HudUiLayoutNode::ApplyImageWidget(
        &layoutPayload[3],
        &items[1],
        baseX,
        0,
        anchor,
        sharedImage,
        0
    );
    HudUiLayoutNode::ApplyImageWidget(
        &layoutPayload[4],
        &items[2],
        baseX,
        0,
        anchor,
        sharedImage,
        0
    );

    HudUiWidget *const anchorItem = &items[2];
    const int y = anchorItem->GetCenterY();
    const int x = anchorItem->GetCenterX();
    SetPos(
        x,
        y
    );

    clipRect.left += baseX;
    clipRect.right += baseX;
    SetBltSourceAndClipRect(
        0,
        &clipRect
    );
}

// Reimplements 0x40f3e0: HudUiTripletPanel::ShutdownItems_Stub
void HudUiTripletPanel::ShutdownItems_Stub() {
    HudUiNoOpMethodStub(&g_HudUiMgrNanitePanel.items[0]);
    HudUiNoOpMethodStub(&g_HudUiMgrNanitePanel.items[1]);
    HudUiNoOpMethodStub(&g_HudUiMgrNanitePanel.items[2]);
}

// Reimplements 0x40d600: HudUiTripletPanel::UnwindDestructFirstItem
void HudUiTripletPanel::UnwindDestructFirstItem() {
    items[0].DestructorCore();
}

// Reimplements 0x40d610: HudUiTripletPanel::DestructorCore
void HudUiTripletPanel::DestructorCore() {
    {
        for (int index = 2; index >= 0; --index) {
            items[index].DestructorCore();
        }
    }

}

// Reimplements 0x40e910: HudUiTriplet::InterpolateLayout
// (D:\Proj\Battlesport\HudUiTriplet.cpp)
void HudUiTriplet::InterpolateLayout(
    float t
) {
    baseX = (int)((float)(baseXEnd - baseXStart) * t + baseXStart);
    baseY = (int)((float)(baseYEnd - baseYStart) * t + baseYStart);
    rowPitchY = (int)((float)(rowPitchYEnd - rowPitchYStart) * t + rowPitchYStart);
    lapsColumnOffsetX =
        (int)((float)(lapsColumnOffsetXEnd - lapsColumnOffsetXStart) * t + lapsColumnOffsetXStart);
    killsColumnOffsetX = (int)((float)(killsColumnOffsetXEnd - killsColumnOffsetXStart) * t +
                               killsColumnOffsetXStart);
    fontSize = (int)((float)(fontSizeEnd - fontSizeStart) * t + fontSizeStart);
    fontWeight = (int)((float)(fontWeightEnd - fontWeightStart) * t + fontWeightStart);
}

void HudUiTextInput::OnPrintableKey(
    int key
) {
    InsertCharAtCursor(key);
}

void HudUiTextInput::OnIgnoredKey(
    int
) {
}

void HudUiTextInput::OnAccept() {
    zGame::ReturnOnlyStub();
}

void HudUiTextInput::OnCancel() {
    zGame::ReturnOnlyStub();
}

void HudUiTextInput::OnBackspace() {
    BackspaceDeleteChar();
}

void HudUiTextInput::OnDeleteForward() {
    DeleteCharForward();
}

void HudUiTextInput::OnMoveCursorLeft() {
    MoveCursorLeft();
}

void HudUiTextInput::OnMoveCursorRight() {
    MoveCursorRight();
}

void HudUiTextInput::OnOverflow() {
    zGame::ReturnOnlyStub();
}

// Reimplements 0x4b4370: HudUiTextInput::DestructorCore
void HudUiTextInput::DestructorCore() {
    char *const ownedBuffer = buffer;
    ::operator delete(ownedBuffer);
}

// Reimplements 0x4b4ab0: HudUiTextInput::DestructorCoreThunk
void HudUiTextInput::DestructorCoreThunk() {
    DestructorCore();
}

// Reimplements 0x4b4390: HudUiTextInput::AllocTextBuffer
void HudUiTextInput::AllocTextBuffer(
    int bufferSize
) {
    char *const newBuffer = (char *)(::operator new(bufferSize));
    char *const oldBuffer = buffer;
    if (oldBuffer != 0) {
        int copyCount = capacity;
        if (bufferSize < copyCount) {
            copyCount = bufferSize;
        }

        strncpy(
            newBuffer,
            oldBuffer,
            copyCount
        );
    }

    capacity = bufferSize;
    buffer = newBuffer;
}

// Reimplements 0x4b42f0: HudUiTextInput::Constructor
HudUiTextInput * HudUiTextInput::Constructor(
    int bufferSize
) {
    cursor = 0;
    buffer = 0;
    capacity = 0;
    AllocTextBuffer(bufferSize);

    {
        for (int code = 0; code < 0x100; ++code) {
            keyActionMap[code] = isprint(code) != 0 ? (char)(0) : (char)(1);
        }
    }

    keyActionMap[0x20] = 0;
    keyActionMap[0x2e] = 0;
    keyActionMap[0x1b] = 2;
    keyActionMap[0x0d] = 3;
    keyActionMap[0x08] = 4;
    keyActionMap[0x7f] = 5;
    keyActionMap[0x02] = 6;
    keyActionMap[0x06] = 7;
    return this;
}

// Reimplements 0x4b4420: HudUiTextInput::SetCursorPosition
void HudUiTextInput::SetCursorPosition(
    int position
) {
    const int textLength = (int)(strlen(buffer));
    cursor = position < textLength ? (unsigned int)(position) : (unsigned int)(textLength);
}

// Reimplements 0x4b43d0: HudUiTextInput::SetContents
void HudUiTextInput::SetContents(
    const char *source
) {
    strncpy(
        buffer,
        source,
        capacity
    );
    buffer[capacity - 1] = '\0';
    SetCursorPosition((int)(cursor));
}

// Reimplements 0x4b4410: HudUiTextInput::GetBuffer
char * HudUiTextInput::GetBuffer() {
    return buffer;
}

// Reimplements 0x4b4590: HudUiTextInput::ShiftTextRight
int HudUiTextInput::ShiftTextRight(
    int count,
    int startPos
) {
    int index = (int)(strlen(buffer)) + count;
    if (index >= (int)(capacity)) {
        return 0;
    }

    while (index > startPos) {
        buffer[index] = buffer[index - count];
        --index;
    }

    return 1;
}

// Reimplements 0x4b45e0: HudUiTextInput::ShiftTextLeft
int HudUiTextInput::ShiftTextLeft(
    int count,
    int startPos
) {
    const int textLength = (int)(strlen(buffer));
    {
        for (int index = startPos; index < textLength; ++index) {
            buffer[index] = buffer[index + count];
        }
    }

    return 1;
}

// Reimplements 0x4b4550: HudUiTextInput::DeleteCharForward
void HudUiTextInput::DeleteCharForward() {
    ShiftTextLeft(
        1,
        (int)(cursor)
    );
}

// Reimplements 0x4b4560: HudUiTextInput::MoveCursorLeft
void HudUiTextInput::MoveCursorLeft() {
    if ((int)(cursor) > 0) {
        --cursor;
    }
}

// Reimplements 0x4b4570: HudUiTextInput::MoveCursorRight
void HudUiTextInput::MoveCursorRight() {
    const int textLength = (int)(strlen(buffer));
    if ((int)(cursor) < textLength) {
        ++cursor;
    }
}

// Reimplements 0x4b4530: HudUiTextInput::BackspaceDeleteChar
void HudUiTextInput::BackspaceDeleteChar() {
    if ((int)(cursor) > 0) {
        --cursor;
        ShiftTextLeft(
            1,
            (int)(cursor)
        );
    }
}

// Reimplements 0x4b44e0: HudUiTextInput::InsertCharAtCursor
void HudUiTextInput::InsertCharAtCursor(
    int ch
) {
    const int textLength = (int)(strlen(buffer));
    if (textLength >= (int)(capacity)-1) {
        OnOverflow();
        return;
    }

    ShiftTextRight(
        1,
        (int)(cursor)
    );
    buffer[cursor] = (char)(ch);
    ++cursor;
}

// Reimplements 0x4b4460: HudUiTextInput::DispatchKeyAction
void HudUiTextInput::DispatchKeyAction(
    int key
) {
    const int keyIndex = (signed char)(key);
    const int action = (signed char)(keyActionMap[keyIndex]);

    switch (action) {
    case 0:
        OnPrintableKey(key);
        break;
    case 1:
        OnIgnoredKey(key);
        break;
    case 2:
        OnCancel();
        break;
    case 3:
        OnAccept();
        break;
    case 4:
        OnBackspace();
        break;
    case 5:
        OnDeleteForward();
        break;
    case 6:
        OnMoveCursorLeft();
        break;
    case 7:
        OnMoveCursorRight();
        break;
    default:
        break;
    }
}

// Reimplements 0x4ba3e0: HudUiOwnedTextInput::OnAcceptNotifyOwner
void HudUiOwnedTextInput::OnAcceptNotifyOwner() {
    if (owner != 0) {
        owner->OnAcceptForwardToCommit();
    }
}

void HudUiOwnedTextInput::OnAccept() {
    OnAcceptNotifyOwner();
}

void HudUiChatComposeTextInput::OnAccept() {
    GameNet::EndChatComposeAndSendThunk();
}

// Reimplements 0x40d660: HudUiMgrObjectiveBlock::Destructor
void HudUiMgrObjectiveBlock::Destructor() {
    chatComposeTextInput.DestructorCore();
    objectiveSensorRect.DestructorCore();
    objectiveWidget.DestructorCore();
}

// Reimplements 0x40d6e0: HudUiMgrSensorBlock::Destructor
// Binary Ninja models the retail HUD manager as one contiguous object and reaches
// sensorPanel, sensorOverlay, and sensorMeter by offsets from this block. The
// current source keeps those recovered subobjects as globals in the HUD manager
// cluster, so this method applies the same teardown to the named objects.
void HudUiMgrSensorBlock::Destructor() {
    g_HudUiMgrSensorOverlay.DestructorCore();
    g_HudUiMgrSensorPanel.DestructorCore();
}

/**
 * Reimplements 0x40d780: HudUiSlot::Destructor.
 * Purpose: Tears down the marker and slot widgets before resetting the slot to the common HUD element table.
 */
void HudUiSlot::Destructor() {
    trackMarkerWidget.DestructorCore();
    slotWidget.DestructorCore();
}

/**
 * Reimplements 0x40db20: HudUiSlot::Constructor.
 * Purpose: Constructs the HUD element base and embedded slot widgets for a weapon/sensor HUD slot.
 */
HudUiSlot * HudUiSlot::Constructor() {
    HudUiElement::Constructor(
        0,
        0
    );
    slotWidget.Constructor(0);
    trackMarkerWidget.Constructor(0);
    return this;
}

/**
 * Reimplements 0x40db90: HudUiSlot::Draw.
 * Purpose: Draws the visible slot and track-marker widgets in recovered HUD slot order.
 */
void HudUiSlot::Draw() {
    if ((slotWidget.flags & 0x10) == 0) {
        slotWidget.Draw();
    }

    if ((trackMarkerWidget.flags & 0x10) == 0) {
        trackMarkerWidget.Draw();
    }
}

/**
 * Reimplements 0x40dbd0: HudUiSlot::ScalarDeletingDestructor.
 * Purpose: Runs slot destruction and conditionally releases heap storage according to the MSVC deleting-destructor flag.
 */
HudUiElement * HudUiSlot::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40fa10: HudUiStatsListElement::Update
void HudUiStatsListElement::Update(
    float deltaSeconds
) {
    triplet->UpdateAll(deltaSeconds);
}

// Reimplements 0x40fa40: HudUiStatsListElement::DestructorCore
void HudUiStatsListElement::DestructorCore() {

    HudUiTriplet *const ownedTriplet = triplet;
    if (ownedTriplet != 0) {
        ownedTriplet->DestructorCore();
        ::operator delete(ownedTriplet);
    }

    triplet = 0;
}

// Reimplements 0x40fa20: HudUiStatsListElement::ScalarDeletingDestructor
HudUiElement * HudUiStatsListElement::ScalarDeletingDestructor(
    unsigned int flags
) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40fdd0: HudUiStringMenu::DestructorCore
void HudUiStringMenu::DestructorCore() {
    {
        int itemIndex;
        for (itemIndex = 0; itemIndex < 23; ++itemIndex) {
            ((HudUiPanel *)(&items[itemIndex]))->Destructor();
        }
    }

    HudUiContainer::DestructorCore();
}

/**
 * Reimplements 0x40dac0: HudUiCounter::Constructor.
 * Purpose: Constructs the widget base and clears the three HUD counter state-image slots.
 */
HudUiCounter * HudUiCounter::Constructor() {
    HudUiWidget::Constructor(0);
    stateImages[2] = 0;
    stateImages[1] = 0;
    stateImages[0] = 0;
    return this;
}

/**
 * Reimplements 0x40f0f0: HudUiCounter::ReleaseStateImages.
 * Purpose: Releases and clears the counter's three variant images.
 */
void HudUiCounter::ReleaseStateImages() {
    zVid_Image::ReleaseIfNotDefault(stateImages[0]);
    zVid_Image::ReleaseIfNotDefault(stateImages[1]);
    zVid_Image::ReleaseIfNotDefault(stateImages[2]);

    stateImages[2] = 0;
    stateImages[1] = 0;
    stateImages[0] = 0;
}

/**
 * Reimplements 0x40f070: HudUiCounter::ApplyFromLayoutNode.
 * Purpose: Loads counter image/layout data from a ZRD array node and registers the counter with the HUD manager.
 */
int HudUiCounter::ApplyFromLayoutNode(
    zReader::Node *layoutNode
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    stateImages[0] = zImage::TexDir_FindOrCreateByPath(payload[1].value.str);
    stateImages[1] = zImage::TexDir_FindOrCreateByPath(payload[2].value.str);
    stateImages[2] = zImage::TexDir_FindOrCreateByPath(payload[3].value.str);
    layoutX = payload[4].value.i32;
    layoutY = payload[5].value.i32;

    UpdateLayoutPosition();
    SetImageBorrowedAndInvalidate(stateImages[0]);
    g_HudUiMgr.AddChild(this);
    return 1;
}

/**
 * Reimplements 0x40f130: HudUiCounter::UpdateLayoutPosition.
 * Purpose: Places the counter relative to the HUD origin and rebuilds the local clip viewport rectangle.
 */
void HudUiCounter::UpdateLayoutPosition() {
    const int localX = layoutX;
    const int localY = layoutY;
    SetPos(
        g_HudUiMgrHudOriginX + localX,
        g_HudUiMgrHudOriginY + localY
    );

    zVidImagePartial *const image = stateImages[0];
    clipViewportRect.left = localX;
    clipViewportRect.top = localY;
    clipViewportRect.right = localX + image->width;
    clipViewportRect.bottom = localY + image->height;
}

// Reimplements 0x4134e0: HudUiMessage::Draw
// (D:\Proj\Battlesport\hud.cpp)
void HudUiMessage::Draw() {
    HudUiWidget::Draw();
    panel.Draw();
}

// Reimplements 0x414070: HudUiMessage::RebuildWeaponLayout
// (D:\Proj\Battlesport\hud.cpp)
void HudUiMessage::RebuildWeaponLayout() {
    HudUiWidget *const layoutWidget2 = &g_HudLayoutHW.widget2;
    const int anchorX = layoutWidget2->GetCenterX();
    const int anchorY = layoutWidget2->GetCenterY();

    const int clipLeft = panel.layoutX + (g_HudUiMgrHudOriginX / 2);
    zVidImagePartial *const baseImage = variantImages[0];
    HudUiRect widgetClipRect;
    widgetClipRect.left = clipLeft;
    widgetClipRect.top = panel.layoutY;
    widgetClipRect.right = clipLeft + baseImage->width;
    widgetClipRect.bottom = panel.layoutY + baseImage->height;

    SetPos(
        clipLeft + anchorX,
        panel.layoutY + anchorY
    );
    SetBltSourceAndClipRect(
        0,
        &widgetClipRect
    );

    HudUiRect panelClipRect;
    panelClipRect.left = clipLeft + 3;
    panelClipRect.top = widgetClipRect.bottom;
    panelClipRect.right = widgetClipRect.right - 2;
    panelClipRect.bottom = widgetClipRect.bottom + 12;

    const int textX =
        panelClipRect.left + ((panelClipRect.right - panelClipRect.left) / 2) + anchorX;
    panel.SetPos(
        textX,
        widgetClipRect.bottom + anchorY
    );
    panel.SetClip(
        0,
        &panelClipRect
    );

    zVidImagePartial *const sideImage = sideImageSwaps[0];
    widget.SetPos(
        anchorX - sideImage->width + widgetClipRect.right - 1,
        anchorY - sideImage->height + widgetClipRect.bottom - 1
    );
}

/**
 * Reimplements 0x413ec0: HudUiMessage::LoadWeaponLayoutFromNode.
 * Purpose: Load weapon-message images/layout and register the message owner and side widget with the HUD manager.
 */
int HudUiMessage::LoadWeaponLayoutFromNode(
    zReader::Node *layoutNode,
    const HudUiPanelFontParams *fontParams
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    variantImages[0] = zImage::TexDir_FindOrCreateByPath(payload[1].value.str);
    variantImages[1] = zImage::TexDir_FindOrCreateByPath(payload[2].value.str);
    variantImages[2] = zImage::TexDir_FindOrCreateByPath(payload[3].value.str);
    variantImages[3] = zImage::TexDir_FindOrCreateByPath(payload[4].value.str);
    variantImages[4] = zImage::TexDir_FindOrCreateByPath(payload[5].value.str);
    sideImageSwaps[0] = zImage::TexDir_FindOrCreateByPath(payload[6].value.str);
    sideImageSwaps[1] = zImage::TexDir_FindOrCreateByPath(payload[7].value.str);
    panel.layoutX = payload[8].value.i32;
    panel.layoutY = payload[9].value.i32;

    RebuildWeaponLayout();

    imageStateWord = (imageStateWord & 0xffff0000u) | 1u;
    Invalidate();

    panel.centerText = 1;
    panel.textColor0 = 0x0020bf40;
    panel.textColor1 = 0x0020bf40;
    panel.textDirty = 1;
    panel.shadowOffsetX = -1;
    panel.shadowOffsetY = -1;
    panel.shadowEnabled = 1;

    panel.SetFont(
        fontParams->faceName,
        fontParams->height,
        fontParams->weight,
        fontParams->width,
        0,
        0,
        2
    );
    panel.SetTextFmt("   ");

    g_HudUiMgr.AddChild(this);
    g_HudUiMgr.AddChild(&widget);
    return 1;
}

// Reimplements 0x413ff0: HudUiMessage::ReleaseImages
void HudUiMessage::ReleaseImages() {
    zVid_Image::ReleaseIfNotDefault(variantImages[0]);
    zVid_Image::ReleaseIfNotDefault(variantImages[1]);
    zVid_Image::ReleaseIfNotDefault(variantImages[2]);
    zVid_Image::ReleaseIfNotDefault(variantImages[3]);
    zVid_Image::ReleaseIfNotDefault(variantImages[4]);
    zVid_Image::ReleaseIfNotDefault(sideImageSwaps[0]);
    zVid_Image::ReleaseIfNotDefault(sideImageSwaps[1]);

    sideImageSwaps[1] = 0;
    sideImageSwaps[0] = 0;
    variantImages[4] = 0;
    variantImages[3] = 0;
    variantImages[2] = 0;
    variantImages[1] = 0;
    variantImages[0] = 0;
}

// Reimplements 0x4126e0: HudUiMessage::SelectVariantDisplay
void __fastcall HudUiMessage::SelectVariantDisplay(
    int messageIndex,
    int variantIndex
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    message.SetImageBorrowedAndInvalidate(message.variantImages[variantIndex]);

    if (variantIndex == 0 || variantIndex == 3) {
        message.activeSideImages[0] = message.sideImageSwaps[0];
        message.widget.SetImageBorrowedAndInvalidate(message.activeSideImages[1]);
        message.panel.activeSideIndex = 0;
    }

    if (variantIndex == 5) {
        message.panel.activeSideIndex = 0;
    }

    if (variantIndex == 1 || variantIndex == 4) {
        message.activeSideImages[1] = message.sideImageSwaps[1];
        message.widget.SetImageBorrowedAndInvalidate(message.activeSideImages[0]);
        message.panel.activeSideIndex = 1;
    }

    if (variantIndex == 6) {
        message.panel.activeSideIndex = 1;
    }
}

// Reimplements 0x412790: HudUiMessage::ApplySideImageSwap
void __fastcall HudUiMessage::ApplySideImageSwap(
    int messageIndex,
    int sideIndex
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    zVidImagePartial *const image = message.sideImageSwaps[sideIndex];
    message.activeSideImages[sideIndex] = image;
    message.widget.SetImageBorrowedAndInvalidate(image);
    message.widget.flags &= 0x10u;
}

// Reimplements 0x4127d0: HudUiMessage::ClearDisplay
void __fastcall HudUiMessage::ClearDisplay(
    int messageIndex
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    message.SetImageBorrowedAndInvalidate(0);
    message.widget.SetImageBorrowedAndInvalidate(0);

    message.panel.SetText("");
    message.Invalidate();
}

// Reimplements 0x412650: HudUiMessage::SetValueIfOwnerMatches
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall HudUiMessage::SetValueIfOwnerMatches(
    int messageIndex,
    int ownerSideIndex,
    float valueOrClearToken
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    if (ownerSideIndex != message.panel.activeSideIndex) {
        return;
    }

    if (valueOrClearToken == kHudUiMessageClearSpecialTokenValue) {
        message.panel.SetText(kHudUiMessageClearSpecialToken165);
        return;
    }

    message.panel.SetTextFmt(
        "%d",
        (int)(ceil(valueOrClearToken))
    );
    message.Invalidate();
}

// Reimplements 0x412820: HudUiMessage::UpdateSelectedWeaponDisplay
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall HudUiMessage::UpdateSelectedWeaponDisplay(
    int weaponBankIndex,
    int weaponSideIndex,
    float valueOrClearToken
) {
    int messageIndexForText = weaponBankIndex;
    if (weaponBankIndex > 1) {
        SelectVariantDisplay(
            g_HudUiMgrActiveWeaponMessageIndex,
            g_HudUiMgrActiveWeaponSideIndex
        );
        g_HudUiMgrActiveWeaponMessageIndex = weaponBankIndex;
        g_HudUiMgrActiveWeaponSideIndex = weaponSideIndex;
        if (valueOrClearToken > 0.0f) {
            SelectVariantDisplay(
                weaponBankIndex,
                weaponSideIndex + 3
            );
        }
    } else if (weaponBankIndex == 1) {
        SelectVariantDisplay(
            1,
            weaponSideIndex + 3
        );
        messageIndexForText = 1;
    } else {
        g_HudUiMgrActiveWeaponMessageIndex = 0;
        g_HudUiMgrActiveWeaponSideIndex = 0;
        return;
    }

    HudUiMessage &message = g_HudUiMgrMessages[messageIndexForText];
    if (weaponSideIndex != message.panel.activeSideIndex) {
        return;
    }

    if (valueOrClearToken == kHudUiMessageClearSpecialTokenValue) {
        message.panel.SetTextFmt(kHudUiMessageClearSpecialToken165);
        return;
    }

    message.panel.SetTextFmt(
        "%d",
        (int)(ceil(valueOrClearToken))
    );
    message.Invalidate();
}

// Reimplements 0x40da00: HudUiMessage::Constructor
HudUiMessage * HudUiMessage::Constructor() {
    HudUiWidget::Constructor(0);
    panel.ConstructorDefault(
        0,
        0,
        0
    );
    widget.Constructor(0);

    memset(
        variantImages,
        0,
        0x24
    );
    panel.activeSideIndex = 0;
    return this;
}

// Reimplements 0x40d590: HudUiMessage::Destructor
void HudUiMessage::Destructor() {
    widget.DestructorCore();
    panel.Destructor();
    HudUiWidget::DestructorCore();
}

// Reimplements 0x40daa0: HudUiMessage::ScalarDeletingDestructor
HudUiElement * HudUiMessage::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40eb00: HudUiShieldMessageWidget::ApplyLayout
// (D:\Proj\Battlesport\hud.cpp)
int __stdcall HudUiShieldMessageWidget::ApplyLayout(
    zReader::Node *layoutRoot
) {
    HudUiShieldMessageWidget *const shieldMessageWidget = g_HudUiMgrShieldMessageWidget;
    zReader::Node *const layoutPayload = layoutRoot->value.nodes;

    HudUiLayoutNode::ApplyImageWidget(
        &layoutPayload[1],
        &shieldMessageWidget->widget,
        g_HudUiMgrHudOriginX,
        g_HudUiMgrHudOriginY,
        0,
        0,
        &shieldMessageWidget->screenRect
    );

    int offsetXY[2];
    offsetXY[0] = shieldMessageWidget->widget.GetCenterX();
    offsetXY[1] = shieldMessageWidget->widget.GetCenterY();

    HudUiPanel *const percentTextPanel = (HudUiPanel *)(&shieldMessageWidget->percentTextPanel);
    HudUiLayoutNode::ApplyTextLabel(
        &layoutPayload[2],
        percentTextPanel,
        0,
        0,
        offsetXY
    );

    HudUiRect clipRect;
    clipRect.left = percentTextPanel->GetX() - offsetXY[0];
    clipRect.top = percentTextPanel->GetY() - offsetXY[1];
    percentTextPanel->SetClip(
        shieldMessageWidget->widget.image,
        &clipRect
    );

    percentTextPanel->SetTextFmt("000");
    percentTextPanel->UpdateTextBoundsFromContent();

    HudUiLayoutNode::ApplyMeterQuad(
        &layoutPayload[3],
        &shieldMessageWidget->meter,
        0,
        0,
        offsetXY,
        &clipRect
    );

    shieldMessageWidget->meter.SetBltSourceAndClipRect(
        shieldMessageWidget->widget.image,
        &clipRect
    );

    g_HudUiMgr.AddChild((HudUiElement *)(&shieldMessageWidget->widget));
    g_HudUiMgr.AddChild((HudUiElement *)(percentTextPanel));
    g_HudUiMgr.AddChild((HudUiElement *)(&shieldMessageWidget->meter));
    return 1;
}

// Reimplements 0x40fe30: HudUiShieldMessageWidget::Destructor
void HudUiShieldMessageWidget::Destructor() {
    ((HudUiPanel *)(&percentTextPanel))->Destructor();
    widget.DestructorCore();
}

// Reimplements 0x4bcf20: HudUiBar::Constructor
HudUiBar * HudUiBar::Constructor() {
    HudUiElement::Constructor(
        0,
        0
    );
    drawVertexCount = 0;
    memset(
        points,
        0,
        sizeof(points)
    );
    Invalidate();
    return this;
}

// Reimplements 0x4bcff0: HudUiBar::Draw
void HudUiBar::Draw() {
    DrawBase();
    if (drawVertexCount != 0) {
        zRndr_RasterizePoly(
            (zVec3 *)(points),
            drawVertexCount,
            drawParam
        );
    }
}

// Reimplements 0x4bcf80: HudUiBar::SetPointXY
// (D:\Proj\Battlesport\hud.cpp)
void HudUiBar::SetPointXY(
    int pointIndex,
    float x,
    float y
) {
    if (pointIndex >= 0 && pointIndex < 21) {
        points[pointIndex].x = x;
        points[pointIndex].y = y;

        if (drawVertexCount < pointIndex + 1) {
            drawVertexCount = pointIndex + 1;
        }

        if (pointIndex == 0) {
            SetPos(
                (int)(x),
                (int)(y)
            );
        }
    }

    Invalidate();
}

// Reimplements 0x4bf840: HudUiPolyline::Constructor
HudUiPolyline * HudUiPolyline::Constructor() {
    HudUiElement::Constructor(
        0,
        0
    );
    pointCount = 0;
    memset(
        points,
        0,
        sizeof(points)
    );
    Invalidate();
    clipRect = 0;
    return this;
}

// Reimplements 0x4bf900: HudUiPolyline::Draw
void HudUiPolyline::Draw() {
    DrawBase();

    const int currentPointCount = pointCount;
    if (currentPointCount == 0) {
        return;
    }

    if (clipRect != 0) {
        zRndr_DrawClippedImmediateLineStrip(
            (const zRndr_LinePoint2I *)(points),
            currentPointCount - 1,
            clipRect,
            color565
        );
        return;
    }

    {
        for (int index = 0; index < currentPointCount - 1; ++index) {
            const HudUiPolylinePoint &point = points[index];
            const HudUiPolylinePoint &nextPoint = points[index + 1];
            zRndr_DrawImmediateLine(
                point.x,
                point.y,
                nextPoint.x,
                nextPoint.y,
                color565
            );
        }
    }
}

// Reimplements 0x4bf8b0: HudUiPolyline::SetPoint
void HudUiPolyline::SetPoint(
    int index,
    int pointX,
    int pointY
) {
    points[index].x = pointX;
    points[index].y = pointY;

    if (pointCount <= index) {
        pointCount = index + 1;
    }

    if (index == 0) {
        SetPos(
            pointX,
            pointY
        );
    }

    Invalidate();
}

// Reimplements 0x4b4620: HudUiSliderBorder::Constructor
HudUiSliderBorder * HudUiSliderBorder::Constructor() {
    HudUiPolyline::Constructor();
    originX = 0;
    originY = 0;
    halfWidth = 1;
    height = 10;
    blinkEnabled = 0;
    blinkPeriodSec = 0.35f;
    blinkDirSign = 1;
    blinkTimeRemainingSec = 0.0f;

    SetPoint(
        0,
        -1,
        0
    );
    SetPoint(
        1,
        halfWidth,
        0
    );
    SetPoint(
        2,
        halfWidth,
        1
    );
    SetPoint(
        3,
        0,
        1
    );
    SetPoint(
        4,
        0,
        height - 1
    );
    SetPoint(
        5,
        halfWidth,
        height - 1
    );
    SetPoint(
        6,
        halfWidth,
        height
    );
    SetPoint(
        7,
        -halfWidth,
        height
    );
    SetPoint(
        8,
        -halfWidth,
        height - 1
    );
    SetPoint(
        9,
        0,
        height - 1
    );
    SetPoint(
        10,
        0,
        1
    );
    SetPoint(
        11,
        -halfWidth,
        1
    );
    SetPoint(
        12,
        -halfWidth,
        0
    );
    return this;
}

// Reimplements 0x4b47b0: HudUiSliderBorder::Update
void HudUiSliderBorder::Update(
    float deltaSeconds
) {
    if ((flags & 0x10) != 0) {
        return;
    }

    if (blinkEnabled != 0) {
        const float nextTime = blinkTimeRemainingSec - deltaSeconds;
        blinkTimeRemainingSec = nextTime;
        if (nextTime < 0.0f) {
            blinkDirSign = -blinkDirSign;
            blinkTimeRemainingSec = blinkPeriodSec;
        }
    }

    if (blinkEnabled == 0 || blinkDirSign == 1) {
        HudUiPolyline::Draw();
    }
}

// Reimplements 0x4b4810: HudUiSliderBorder::SetBounds
void HudUiSliderBorder::SetBounds(
    int newOriginX,
    int newOriginY,
    int newHalfWidth,
    int newHeight
) {
    originX = newOriginX;
    originY = newOriginY;
    halfWidth = newHalfWidth;
    height = newHeight;

    SetPoint(
        0,
        originX - halfWidth,
        originY
    );
    SetPoint(
        1,
        originX + halfWidth,
        originY
    );
    SetPoint(
        2,
        originX + halfWidth,
        originY + 1
    );
    SetPoint(
        3,
        originX,
        originY + 1
    );
    SetPoint(
        4,
        originX,
        originY + height - 1
    );
    SetPoint(
        5,
        originX + halfWidth,
        originY + height - 1
    );
    SetPoint(
        6,
        originX + halfWidth,
        originY + height
    );
    SetPoint(
        7,
        originX - halfWidth,
        originY + height
    );
    SetPoint(
        8,
        originX - halfWidth,
        originY + height - 1
    );
    SetPoint(
        9,
        originX,
        originY + height - 1
    );
    SetPoint(
        10,
        originX,
        originY + 1
    );
    SetPoint(
        11,
        originX - halfWidth,
        originY + 1
    );
    SetPoint(
        12,
        originX - halfWidth,
        originY
    );
}

/**
 * Reimplements 0x4b49e0: HudUiNumericTextInput::BaseConstructor.
 * Purpose: Construct the ZRD widget base and owned numeric text-entry controls.
 */
HudUiNumericTextInput * HudUiNumericTextInput::BaseConstructor() {
    HudUiZrdWidget::Constructor();
    textInput.Constructor(0x100);
    textInput.owner = 0;

    sliderBorder.Constructor();
    sliderBorder.sliderVisibleWhenInputActive = 0;
    sliderBorder.rawKeyFilterEnabled = 0;
    sliderBorder.inputActive = 1;
    sliderBorder.caretHalfWidth = 0;

    sliderBorder.SetVisible(1);
    textInput.owner = this;
    SetVisible(1);
    return this;
}

// Reimplements 0x41a190: HudUiNumericTextInput::Constructor
HudUiNumericTextInput * HudUiNumericTextInput::Constructor(
    unsigned int maxDigits
) {
    BaseConstructor();
    textInput.AllocTextBuffer(maxDigits);
    Update("");
    SetInputActive(0);
    return this;
}

// Reimplements 0x41a200: HudUiClampedIntTextInput::Constructor
HudUiClampedIntTextInput * HudUiClampedIntTextInput::Constructor(
    unsigned int maxDigits
) {
    BaseConstructor();
    textInput.AllocTextBuffer(maxDigits + 1);
    Update("");
    SetInputActive(0);
    minValue = -2147483647 - 1;
    maxValue = 2147483647;
    return this;
}

// Reimplements 0x4b4e40: HudUiNumericTextInput::AllocTextBuffer
void HudUiNumericTextInput::AllocTextBuffer(
    unsigned int bufferSize
) {
    textInput.AllocTextBuffer(bufferSize);
}

// Reimplements 0x4b4ed0: HudUiNumericTextInput::GetBuffer
char * HudUiNumericTextInput::GetBuffer() {
    return textInput.GetBuffer();
}

/**
 * Reimplements 0x4b4e60: HudUiNumericTextInput::Update.
 * Purpose: Update the text-input buffer, mirror the visible label text, and invalidate the owning widget.
 */
void HudUiNumericTextInput::Update(
    const char *text
) {
    textInput.SetContents(text);
    textInput.SetCursorPosition((int)(strlen(text)));
    char *const buffer = textInput.GetBuffer();

    if (labelPanels.end != labelPanels.begin) {
        HudUiPanel *const firstPanel = labelPanels.begin[0];
        firstPanel->SetText(buffer);
    }

    Invalidate();
}

// Reimplements 0x4b4ca0: HudUiNumericTextInput::UpdateCaptureUiAndClip
RECOIL_NO_GS void HudUiNumericTextInput::UpdateCaptureUiAndClip(
    float deltaSeconds
) {
    HudUiPanel *const firstPanel = labelPanels.begin[0];
    HudUiElement *const baseElement = (HudUiElement *)(this);

    if ((flags & 0x10) != 0) {
        firstPanel->SetVisible(0);
        firstPanel->Invalidate();
        sliderBorder.SetVisible(0);
        sliderBorder.Invalidate();
        return;
    }

    if (sliderBorder.sliderVisibleWhenInputActive != 0) {
        firstPanel->SetVisible(1);
        char *const buffer = textInput.GetBuffer();

        if (labelPanels.begin != 0) {
            const ptrdiff_t panelCount = labelPanels.end - labelPanels.begin;
            if (panelCount != 0) {
                firstPanel->SetText(buffer);
            }
        }

        RECT textRect = {0};
        textRect.left = firstPanel->GetX();
        textRect.top = firstPanel->GetY();
        textRect.right = firstPanel->GetX();
        textRect.bottom = firstPanel->GetY();

        if (firstPanel->MeasureTextPrefixRect(
            (int)(textInput.cursor),
            &textRect
        ) != 0) {
            const unsigned int textColor = firstPanel->textColor0;
            const unsigned int packedColor = zVid_PackColorRGB(
                                                 (unsigned char)(textColor),
                                                 (unsigned char)(textColor >> 8),
                                                 (unsigned char)(textColor >> 16)
                                             ) &
                                             0xffffu;
            sliderBorder.color565 = (int)(packedColor);
            sliderBorder.SetBounds(
                textRect.right,
                textRect.top,
                sliderBorder.caretHalfWidth,
                textRect.bottom - textRect.top
            );
            sliderBorder.SetVisible(1);
        }

        Invalidate();
    } else {
        sliderBorder.SetVisible(0);
    }

    baseElement->Update(deltaSeconds);
    sliderBorder.Update(deltaSeconds);
}

// Reimplements 0x4b4c50: HudUiNumericTextInput::SetRawKeyboardCapture
void HudUiNumericTextInput::SetRawKeyboardCapture(
    int enable
) {
    const char enableByte = (char)(enable);
    if (enableByte == sliderBorder.sliderVisibleWhenInputActive) {
        return;
    }

    sliderBorder.sliderVisibleWhenInputActive = enableByte;
    if (enableByte != 0) {
        zInput::Keyboard_SetRawEventCallback(
            (void *)(&HudUiNumericTextInput::RawKeyboardCallback),
            this
        );
    } else {
        zInput::Keyboard_SetRawEventCallback(
            0,
            0
        );
    }
}

// Reimplements 0x4b4c90: HudUiNumericTextInput::OnActivate
void HudUiNumericTextInput::OnActivate() {
    sliderBorder.inputActive = 1;
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x4b4ac0: HudUiNumericTextInput::Destructor
void HudUiNumericTextInput::Destructor() {
    SetRawKeyboardCapture(0);
    textInput.DestructorCore();
    HudUiZrdWidget::DestructorCore();
}

// Reimplements 0x41a3f0: HudUiNumericTextInput::DestructorThunk
void HudUiNumericTextInput::DestructorThunk() {
    Destructor();
}

// Reimplements 0x4b4a90: HudUiNumericTextInput::ScalarDeletingDestructor
HudUiElement * HudUiNumericTextInput::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x41c4a0: HudUiNumericTextInput::ScalarDeletingDestructorThunk
HudUiNumericTextInput * HudUiNumericTextInput::ScalarDeletingDestructorThunk(
    unsigned int flags
) {
    DestructorThunk();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b4b30: HudUiNumericTextInput::RawKeyboardCallback
int __fastcall HudUiNumericTextInput::RawKeyboardCallback(
    int key,
    HudUiNumericTextInput *callbackCtx
) {
    if (callbackCtx == 0) {
        return 0;
    }

    return callbackCtx->OnRawKeyboardChar(key);
}

// Reimplements 0x4b4ba0: HudUiNumericTextInput::SetInputActive
int HudUiNumericTextInput::SetInputActive(
    int active
) {
    const int previousActive = sliderBorder.inputActive;
    sliderBorder.inputActive = active;

    HudUiPanel *const firstLabelPanel =
        labelPanels.end != labelPanels.begin ? labelPanels.begin[0] : 0;

    if (active != 0) {
        SetVisible(1);
        sliderBorder.SetVisible(1);
        if (firstLabelPanel != 0) {
            firstLabelPanel->SetVisible(1);
        }
    } else {
        SetVisible(0);
        if (firstLabelPanel != 0) {
            firstLabelPanel->SetVisible(0);
        }
        sliderBorder.SetVisible(0);
    }

    return previousActive;
}

// Reimplements 0x4b4b50: HudUiNumericTextInput::OnRawKeyboardChar
int HudUiNumericTextInput::OnRawKeyboardChar(
    int key
) {
    if (sliderBorder.rawKeyFilterEnabled == 0 ||
        strchr(
            kNumericTextInputAcceptedRawKeyChars,
            key
        ) != 0) {
        textInput.DispatchKeyAction(key);
    }

    return 0;
}

// Reimplements 0x41a290: HudUiNumericTextInput::OnAcceptForwardToCommit
int HudUiNumericTextInput::OnAcceptForwardToCommit() {
    return CommitAndGetValue();
}

// Reimplements 0x41a2a0: HudUiClampedIntTextInput::OnRawKeyboardDigitOnly
int HudUiClampedIntTextInput::OnRawKeyboardDigitOnly(
    int key
) {
    if (strchr(
        kClampedIntTextInputAcceptedRawKeyChars,
        key
    ) != 0) {
        textInput.DispatchKeyAction(key);
    }

    return 0;
}

int HudUiClampedIntTextInput::OnRawKeyboardChar(
    int key
) {
    return OnRawKeyboardDigitOnly(key);
}

// Reimplements 0x41a2d0: HudUiClampedIntTextInput::CommitAndGetValue
int HudUiClampedIntTextInput::CommitAndGetValue() {
    char *const text = GetBuffer();
    int value;

    if (text == 0 || *text == 0) {
        value = minValue;
    } else {
        value = atoi(text);
    }

    if (value < minValue) {
        value = minValue;
    }

    if (value > maxValue) {
        value = maxValue;
    }

    int displayValue = value;
    if (displayValue < minValue) {
        displayValue = minValue;
    }

    if (displayValue > maxValue) {
        displayValue = maxValue;
    }

    char valueText[20];
    sprintf(
        valueText,
        "%d",
        displayValue
    );
    Update(valueText);
    return value;
}

// Reimplements 0x41a350: HudUiClampedIntStepButton::OnActivate
void HudUiClampedIntStepButton::OnActivate() {
    if (targetInput != 0) {
        HudUiClampedIntTextInput *input = targetInput;
        int value = input->CommitAndGetValue() + stepDelta;

        if (value < input->minValue) {
            value = input->minValue;
        }

        if (value > input->maxValue) {
            value = input->maxValue;
        }

        char valueText[20];
        sprintf(
            valueText,
            "%d",
            value
        );
        input->Update(valueText);

        input = targetInput;
        input->Invalidate();
    }

    HudUiZrdWidget::OnActivate();
}

static HudUiNumericTextInput **HudUiNetGameSetupFocusTextInputSlot(
    void *owner
) {
    // BN shows the current network setup text input focus pointer at owner + 0xa94c.
    return (HudUiNumericTextInput **)((unsigned char *)(owner) +
                                      HUD_UI_NET_GAME_SETUP_FOCUS_TEXT_INPUT_OFFSET);
}

// Reimplements 0x41a7b0: HudUiNetGameSetupTextInput::OnActivateFocusAndCursor
void HudUiNetGameSetupTextInput::OnActivateFocusAndCursor() {
    HudUiNumericTextInput **const focusTextInputSlot =
        HudUiNetGameSetupFocusTextInputSlot(HudUiZrdWidget::owner);
    HudUiNumericTextInput *const previousFocusTextInput = *focusTextInputSlot;

    if (previousFocusTextInput != 0) {
        previousFocusTextInput->CommitAndGetValue();
        previousFocusTextInput->SetRawKeyboardCapture(0);
    }

    *focusTextInputSlot = this;
    SetRawKeyboardCapture(1);
    Update(GetBuffer());
    textInput.SetCursorPosition((int)(strlen(GetBuffer())));
    HudUiNumericTextInput::OnActivate();
}

void HudUiNetGameSetupTextInput::OnActivate() {
    OnActivateFocusAndCursor();
}

// Reimplements 0x41ab60: HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\HudUi.cpp)
void HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x41ab70: HudUiNetGameSetupOverlayOwner::StaticInit
// (D:\Proj\Battlesport\HudUi.cpp)
HudUiNetGameSetupOverlayOwner *HudUiNetGameSetupOverlayOwner::StaticInit() {
    return new (&g_HudUiNetGameSetupOverlayOwner) HudUiNetGameSetupOverlayOwner;
}

// Reimplements 0x41ab80: HudUiNetGameSetupOverlayOwner::RegisterAtExit
// (D:\Proj\Battlesport\HudUi.cpp)
void HudUiNetGameSetupOverlayOwner::RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x41ab90: HudUiNetGameSetupOverlayOwner::AtExitDestructor
// (D:\Proj\Battlesport\HudUi.cpp)
void HudUiNetGameSetupOverlayOwner::AtExitDestructor() {
    g_HudUiNetGameSetupOverlayOwner.~HudUiNetGameSetupOverlayOwner();
}

// Reimplements 0x41aba0: HudUiNetGameSetupOverlayOwner::HudUiNetGameSetupOverlayOwner
// (D:\Proj\Battlesport\HudUi.cpp)
HudUiNetGameSetupOverlayOwner::HudUiNetGameSetupOverlayOwner()
    : m_panel(0),
      m_reconfigureExistingSession(0) {}

// Reimplements 0x41abe0: HudUiNetGameSetupOverlayOwner::~HudUiNetGameSetupOverlayOwner
// (D:\Proj\Battlesport\HudUi.cpp)
HudUiNetGameSetupOverlayOwner::~HudUiNetGameSetupOverlayOwner() {
    HudUiNetGameSetupPanel *panel = m_panel;
    if (panel != 0) {
        panel->SetEnabled(0);

        panel = m_panel;
        if (panel != 0) {
            panel->ScalarDeletingDestructor(1);
        }

        m_panel = 0;
    }
}

// Reimplements 0x41ac50: HudUiNetGameSetupOverlayOwner::OnTryBecomeCurrent
// (D:\Proj\Battlesport\HudUi.cpp)
int HudUiNetGameSetupOverlayOwner::OnTryBecomeCurrent() {
    zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int bitsPerPixel = zOpt::GetDisplaySectionBitsPerPixel();
    zOpt_ViewRectSection *const activeRegionRect = zOpt::GetWindowSection();
    zRndr::SetFrameBufferRegion(
        zVideo::GetPrimarySurfacePixels(),
        activeRegionRect,
        bitsPerPixel,
        pitchBytes
    );

    zSndSampleSet_InitByName("DIALOG");

    HudUiNetGameSetupPanel *panel =
        (HudUiNetGameSetupPanel *) ::operator new(sizeof(HudUiNetGameSetupPanel));
    if (panel != 0) {
        panel = panel->Constructor(m_reconfigureExistingSession);
    }

    m_panel = panel;
    panel->SetEnabled(1);

    if (zSnd::GetCDAudioOption() != 0) {
        zSndCd::PlayTrackWithMode(
            2,
            5
        );
    }

    return 1;
}

// Reimplements 0x41ad20: HudUiNetGameSetupOverlayOwner::OnDeactivate
// (D:\Proj\Battlesport\HudUi.cpp)
void HudUiNetGameSetupOverlayOwner::OnDeactivate() {
    Sleep(1000);
    zSndSampleSet_DestroyByName("DIALOG");

    HudUiNetGameSetupPanel *panel = m_panel;
    if (panel == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    panel = m_panel;
    panel->SetEnabled(0);

    ((HudUiDialogController *)m_panel)->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    panel = m_panel;
    if (panel != 0) {
        panel->ScalarDeletingDestructor(1);
    }

    m_panel = 0;
}

// Reimplements 0x41ad80: HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag
// (D:\Proj\GameZRecoil\zHud\HudUiNetGameSetup.cpp)
void HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag(
    int reconfigureExistingSession
) {
    g_HudUiNetGameSetupOverlayOwner.m_reconfigureExistingSession = reconfigureExistingSession;
    g_RecoilApp.QueuePushState(
        &g_HudUiNetGameSetupOverlayOwner,
        0
    );
}

// Reimplements 0x40fb70: HudUiMeter::Constructor
HudUiMeter * HudUiMeter::Constructor() {
    HudUiBar::Constructor();
    fillPixelsMax = 0;
    meterFlags = 0;
    return this;
}

// Reimplements 0x40d9e0: HudUiMeter::ConstructorEx
HudUiMeter * HudUiMeter::ConstructorEx() {
    HudUiBar::Constructor();
    fillPixelsMax = 0;
    meterFlags = 0;
    return this;
}

// Reimplements 0x4bcb50: HudUiTextLabel::HudUiTextLabel
HudUiTextLabel::HudUiTextLabel(
    const char *text,
    int initX,
    int initY,
    int flags
) : HudUiElement(
        0,
        0
    ) {
    centerText = 0;
    SetTextFmt(text);
    x = initX;
    y = initY;
    ((HudUiElement *)(this))->Invalidate();
    fontHandle = flags;
    ((HudUiElement *)(this))->Invalidate();
    alignMode = 0;
}

HudUiTextLabel * HudUiTextLabel::ConstructorWithPosAndFlags(
    const char *text,
    int initX,
    int initY,
    int flags
) {
    new (this) HudUiTextLabel(
        text,
        initX,
        initY,
        flags
    );
    return this;
}

// Reimplements 0x4bcbe0: HudUiTextLabel::CopyConstructor
HudUiTextLabel * HudUiTextLabel::CopyConstructor(
    const HudUiTextLabel *source
) {
    HudUiElement::CopyConstructor(source);
    strncpy(
        textBuffer,
        source->textBuffer,
        sizeof(textBuffer)
    );
    fontHandle = source->fontHandle;
    centerText = source->centerText;
    centerBoundsLeft = source->centerBoundsLeft;
    centerBoundsRight = source->centerBoundsRight;
    alignMode = source->alignMode;
    return this;
}

// Reimplements 0x4bcc80: HudUiTextLabel::Constructor
HudUiTextLabel * HudUiTextLabel::Constructor(
    const HudUiTextLabel *source
) {
    HudUiElement::CopyFrom(source);
    strncpy(
        textBuffer,
        source->textBuffer,
        sizeof(textBuffer)
    );
    fontHandle = source->fontHandle;
    centerText = source->centerText;
    centerBoundsLeft = source->centerBoundsLeft;
    centerBoundsRight = source->centerBoundsRight;
    alignMode = source->alignMode;
    return this;
}

// Reimplements 0x4bccf0: HudUiTextLabel::SetTextFmt
void HudUiTextLabel::SetTextFmt(
    const char *format,
    ...
) {
    if (format == 0) {
        memset(
            textBuffer,
            0,
            sizeof(textBuffer)
        );
        return;
    }

    va_list args;
    va_start(
        args,
        format
    );
    vsprintf(
        textBuffer,
        format,
        args
    );
    va_end(args);

    if (centerText != 0) {
        UpdateTextExtents();
    }

    Invalidate();
}

// Reimplements 0x4bcd80: HudUiTextLabel::RebuildTextBounds
void HudUiTextLabel::RebuildTextBounds() {
    int widthPx = 0;
    int lineAdvance = 0;
    zImage_Font::MeasureString(
        textBuffer,
        fontHandle,
        &widthPx,
        &lineAdvance
    );
    clipRect.right = clipRect.left + widthPx;
    clipRect.bottom = clipRect.top + lineAdvance;
}

// Reimplements 0x4bcdc0: HudUiTextLabel::MeasureTextWidth
int HudUiTextLabel::MeasureTextWidth() {
    int widthPx = 0;
    int lineAdvance = 0;
    zImage_Font::MeasureString(
        textBuffer,
        fontHandle,
        &widthPx,
        &lineAdvance
    );
    return widthPx;
}

// Reimplements 0x4bce30: HudUiTextLabel::OnDraw
void HudUiTextLabel::OnDraw() {
    DrawBase();

    if (textBuffer[0] == '\0') {
        return;
    }

    if (alignMode != 0) {
        int xOffset = MeasureTextWidth();
        if (alignMode == 1) {
            xOffset >>= 1;
        }

        x -= xOffset;
        zImage_Font::BlitStringToActiveTarget(
            textBuffer,
            x,
            y,
            fontHandle
        );
        x += xOffset;
        return;
    }

    zImage_Font::BlitStringToActiveTarget(
        textBuffer,
        x,
        y,
        fontHandle
    );
}

// Reimplements 0x4bcea0: HudUiTextLabel::HitTest
int HudUiTextLabel::HitTest(
    int px,
    int py
) {
    if ((flags & 0x10u) != 0 || x > px || y > py) {
        return 0;
    }

    int textWidth = 0;
    int lineAdvance = 0;
    zImage_Font::MeasureString(
        textBuffer,
        fontHandle,
        &textWidth,
        &lineAdvance
    );

    if (px > x + textWidth) {
        return 0;
    }

    return py <= y + lineAdvance ? 1 : 0;
}

// Reimplements 0x4bcdf0: HudUiTextLabel::UpdateTextExtents
void HudUiTextLabel::UpdateTextExtents() {
    const int widthPx = MeasureTextWidth();
    x = centerBoundsLeft + (centerBoundsRight - widthPx - centerBoundsLeft) / 2;

    if (bltSource != 0) {
        clipRect.left = x;
        clipRect.top = y;
        RebuildTextBounds();
    }
}

// Reimplements 0x4ba740: HudUiPanel::HudUiPanel
HudUiPanel::HudUiPanel(
    const char *text,
    int initX,
    int initY
) : HudUiTextLabel(
        text,
        initX,
        initY,
        0
) {
    textPick = 0;
    textColor0 = 0x00ffffff;
    textColor1 = 0x00ffffff;
    textDirty = 1;
    hFont = GetStockObject(OEM_FIXED_FONT);
    textRect.left = 0;
    cachedText[0] = '\0';
    shadowEnabled = 0;
    textDirty = 1;
    textRect.top = 0;
    alignMode = 0;
    bkMode = TRANSPARENT;
    wrapRect.right = 0;
    textRect.bottom = 0;
    wrapRect.left = 0;
    wrapRect.bottom = 0;
    shadowOffsetY = 0;
    wrapRect.top = 0;
    wordWrapEnabled = 0;
    unknown274 = 0;
    textHeightPx = 0;
    textWidthPx = 0;
}

HudUiPanel * HudUiPanel::ConstructorDefault(
    const char *text,
    int initX,
    int initY
) {
    new (this) HudUiPanel(
        text,
        initX,
        initY
    );
    return this;
}

// Reimplements 0x4bd100: HudUiPanel::ConstructorDefaultThunk
HudUiPanel * HudUiPanel::ConstructorDefaultThunk() {
    return ConstructorDefault(
        0,
        0,
        0
    );
}

// Reimplements 0x4ba850: HudUiPanel::CopyConstructCore
HudUiPanel * HudUiPanel::CopyConstructCore(
    const HudUiPanel *source
) {
    HudUiTextLabel::CopyConstructor(source);

    textPick = 0;
    textColor0 = source->textColor0;
    textColor1 = source->textColor1;

    LOGFONTA logFont = {0};
    if (GetObjectA(
        source->hFont,
        sizeof(logFont),
        &logFont
    ) != 0) {
        hFont = CreateFontIndirectA(&logFont);
    }

    cachedTextLength = source->cachedTextLength;
    strncpy(
        cachedText,
        source->cachedText,
        0x100
    );

    textWidthPx = source->textWidthPx;
    textHeightPx = source->textHeightPx;
    shadowEnabled = source->shadowEnabled;
    bkMode = source->bkMode;
    bkColor = source->bkColor;
    textDirty = source->textDirty;
    unknown274 = source->unknown274;
    wordWrapEnabled = source->wordWrapEnabled;
    wrapRect = source->wrapRect;
    textRect = source->textRect;
    alignMode = source->alignMode;
    shadowOffsetX = source->shadowOffsetX;
    shadowOffsetY = source->shadowOffsetY;
    return this;
}

// Reimplements 0x4ba9e0: HudUiPanel::ConstructorCopy
HudUiPanel * HudUiPanel::ConstructorCopy(
    const HudUiPanel *source
) {
    HudUiTextLabel::Constructor(source);

    textPick = 0;
    textColor0 = source->textColor0;
    textColor1 = source->textColor1;

    LOGFONTA logFont = {0};
    if (GetObjectA(
        source->hFont,
        sizeof(logFont),
        &logFont
    ) != 0) {
        hFont = CreateFontIndirectA(&logFont);
    }

    cachedTextLength = source->cachedTextLength;
    strncpy(
        cachedText,
        source->cachedText,
        0x100
    );

    textWidthPx = source->textWidthPx;
    textHeightPx = source->textHeightPx;
    shadowEnabled = source->shadowEnabled;
    bkMode = source->bkMode;
    bkColor = source->bkColor;
    textDirty = 1;
    unknown274 = source->unknown274;
    wordWrapEnabled = source->wordWrapEnabled;
    wrapRect = source->wrapRect;
    textRect = source->textRect;
    alignMode = source->alignMode;
    shadowOffsetX = source->shadowOffsetX;
    shadowOffsetY = source->shadowOffsetY;
    return this;
}

/**
 * Reimplements 0x4bab40: HudUiPanel::Destructor.
 * Purpose: release the owned text image/font resources during panel teardown.
 */
void HudUiPanel::Destructor() {

    if (textPick != 0) {
        zVid_Image::Destroy(textPick);
        textPick = 0;
    }

    DeleteObject(hFont);
}

/**
 * Reimplements 0x40bef0: HudUiPanel::DestructorThunk.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: tail-call the panel destructor through the callback-compatible
 * panel method slot.
 */
void HudUiPanel::DestructorThunk() {
    Destructor();
}

/**
 * Reimplements 0x4bb460: HudUiPanel::Draw.
 * Purpose: rebuild dirty panel text, draw the panel base, and blit the rendered text image with recovered alignment behavior.
 */
void HudUiPanel::Draw() {
    if (textDirty != 0) {
        RebuildTextRect();
    }

    if (textPick == 0) {
        return;
    }

    if (textBuffer[0] == '\0') {
        DrawBase();
        return;
    }

    if (alignMode == 0) {
        DrawBase();
        zVid_Image::BlitToActiveTarget(
            textPick,
            x,
            y,
            0,
            (zVidRect32 *)(&textRect)
        );
        return;
    }

    if (textDirty != 0) {
        RebuildTextRect();
    }

    int frameWidth = clipRect.right - clipRect.left;
    int textWidth = textWidthPx;
    if (alignMode == 1) {
        frameWidth >>= 1;
        textWidth >>= 1;
    }

    x -= frameWidth;
    DrawBase();

    const int dstX = x + frameWidth - textWidth;
    x = dstX;
    zVid_Image::BlitToActiveTarget(
        textPick,
        dstX,
        y,
        0,
        (zVidRect32 *)(&textRect)
    );
    x += textWidth;
}

// Reimplements 0x4ba400: HudUiPanel::GetWrapRect
HudUiRect * HudUiPanel::GetWrapRect() {
    return &wrapRect;
}

/**
 * Reimplements 0x4bb3d0: HudUiPanel::HitTest.
 * Purpose: test a point against the current visible text bounds, rebuilding dirty text metrics first.
 */
int HudUiPanel::HitTest(
    int px,
    int py
) {
    if ((flags & 0x10u) != 0 || x > px || y > py) {
        return 0;
    }

    if (textDirty != 0) {
        RebuildTextRect();
    }

    if (px >= x + textWidthPx) {
        return 0;
    }

    return py < y + QueryTextHeight() ? 1 : 0;
}

/**
 * Reimplements 0x4bb440: HudUiPanel::GetLastTextPtr.
 * Purpose: return the cached panel text after ensuring dirty text rendering state is rebuilt.
 */
char * HudUiPanel::GetLastTextPtr() {
    if (textDirty != 0) {
        RebuildTextRect();
    }

    return cachedText;
}

/**
 * Reimplements 0x4bb740: HudUiPanel::GetTextRect.
 * Purpose: report the inherited element rectangle extended to the current rendered panel text dimensions.
 */
void HudUiPanel::GetTextRect(
    HudUiRect *outRect
) {
    HudUiElement::GetRect(outRect);

    if (textDirty != 0) {
        RebuildTextRect();
    }

    outRect->right = outRect->left + textWidthPx;
    outRect->bottom = outRect->top + QueryTextHeight();
}

// Reimplements 0x40be90: HudUiPanel::Invalidate
void HudUiPanel::Invalidate() {
    textDirty = 1;
    HudUiElement::Invalidate();
}

// Reimplements 0x40bea0: HudUiPanel::GetFont
HGDIOBJ HudUiPanel::GetFont() {
    return hFont;
}

// Reimplements 0x40beb0: HudUiPanel::SetFontHandle
void HudUiPanel::SetFontHandle(
    HGDIOBJ fontHandle
) {
    hFont = fontHandle;
}

// Reimplements 0x40bec0: HudUiPanel::EnableWordWrapWithRect
void HudUiPanel::EnableWordWrapWithRect(
    const HudUiRect *rect
) {
    wordWrapEnabled = 1;
    wrapRect = *rect;
}

// Reimplements 0x40bf00: HudUtil::FreeFieldPtr
void HudUtil::FreeFieldPtr() {
    if (fieldPtr != 0) {
        free(fieldPtr);
        fieldPtr = 0;
    }
}

// Reimplements 0x40a590: HudUiPanel::ScalarDeletingDestructor
HudUiElement * HudUiPanel::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40f9e0: HudUiPanel::SetTextColor
unsigned int HudUiPanel::SetTextColor(
    unsigned int color
) {
    const unsigned int previous = textColor0;
    textColor0 = color;
    textColor1 = color;
    textDirty = 1;
    return previous;
}

// Reimplements 0x40e010: HudUiPanel::SetTextColorsAndMarkDirty
void HudUiPanel::SetTextColorsAndMarkDirty(
    unsigned int color0,
    unsigned int color1
) {
    textColor0 = color0;
    textColor1 = color1;
    textDirty = 1;
}

// Reimplements 0x40e040: HudUiPanel::SetShadow
unsigned int HudUiPanel::SetShadow(
    unsigned int enableShadow,
    int offsetX,
    int offsetY
) {
    const unsigned int previous = shadowEnabled;
    shadowEnabled = enableShadow;
    shadowOffsetX = offsetX;
    shadowOffsetY = offsetY;
    return previous;
}

// Reimplements 0x4babb0: HudUiPanel::SetFont
void HudUiPanel::SetFont(
    const char *faceName,
    int height,
    int weight,
    int width,
    int italic,
    int charSet,
    int pitchAndFamily
) {
    DeleteObject(hFont);
    hFont = CreateFontA(
        -height,
        width,
        0,
        0,
        weight,
        italic,
        0,
        0,
        charSet,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DRAFT_QUALITY,
        pitchAndFamily,
        faceName
    );
    textDirty = 1;
}

// Reimplements 0x4bb540: HudUiPanel::SetTextFmt
void HudUiPanel::SetTextFmt(
    const char *format,
    ...
) {
    va_list args;
    va_start(
        args,
        format
    );
    SetTextFmtV(
        format,
        args
    );
    va_end(args);
}

// Reimplements 0x4bb5e0: HudUiPanel::SetTextFmtV
void HudUiPanel::SetTextFmtV(
    const char *format,
    va_list args
) {
    if (format == 0) {
        memset(
            textBuffer,
            0,
            0x100
        );
        textDirty = 1;
        return;
    }

    _vsnprintf(
        textBuffer,
        0x100,
        format,
        args
    );
    textBuffer[0xff] = '\0';

    if (strncmp(
        cachedText,
        textBuffer,
        0x100
    ) == 0) {
        return;
    }

    if (centerText != 0) {
        HudUiTextLabel::UpdateTextExtents();
    }

    Invalidate();
    textDirty = 1;
    strncpy(
        cachedText,
        textBuffer,
        0x100
    );
}

// Reimplements 0x4bb680: HudUiPanel::SetText
void HudUiPanel::SetText(
    const char *text
) {
    if (text == 0) {
        memset(
            textBuffer,
            0,
            0x100
        );
        textDirty = 1;
        return;
    }

    strncpy(
        textBuffer,
        text,
        0x100
    );

    if (strncmp(
        cachedText,
        textBuffer,
        0x100
    ) == 0) {
        return;
    }

    if (centerText != 0) {
        HudUiTextLabel::UpdateTextExtents();
    }

    Invalidate();
    textDirty = 1;
    strncpy(
        cachedText,
        textBuffer,
        0x100
    );
}

// Reimplements 0x4bac10: HudUiPanel::RebuildTextRect
void HudUiPanel::RebuildTextRect() {
    if (strlen(textBuffer) == 0) {
        memset(
            &textRect,
            0,
            sizeof(HudUiRect)
        );
        textHeightPx = 0;
        textWidthPx = 0;
        textDirty = 0;
        return;
    }

    HDC measureDc = CreateCompatibleDC(0);
    if (measureDc == 0) {
        textDirty = 0;
        return;
    }

    SelectObject(
        measureDc,
        hFont
    );

    RECT &textRectRef = *(RECT *)(&textRect);
    UINT drawFormat = DT_LEFT;
    BOOL measured = FALSE;
    if (wordWrapEnabled != 0) {
        textRectRef = *(RECT *)(&wrapRect);
        drawFormat = DT_WORDBREAK;
        measured = TRUE;
    } else {
        measured = DrawTextA(
            measureDc,
            textBuffer,
            -1,
            &textRectRef,
            DT_CALCRECT
        );
    }

    if (measured == 0) {
        GetLastError();
    } else {
        if (shadowEnabled != 0) {
            textRectRef.bottom += abs(shadowOffsetY);
            textRectRef.right += abs(shadowOffsetX);
        }

        const int textWidth = textRectRef.right - textRectRef.left;
        const int textHeight = textRectRef.bottom - textRectRef.top;
        textWidthPx = textWidth;
        textHeightPx = textHeight;

        if (textPick != 0 && (textWidth > textPick->width || textHeight > textPick->height)) {
            zVid_Image::Destroy(textPick);
            textPick = 0;
        }

        if (textPick == 0) {
            textPick = zVid_Image::Create();
            zVid_Image::SetFormatCode(
                textPick,
                3
            );
            zVid_Image::SetSize(
                textPick,
                (short)(textWidth),
                (short)(textHeight)
            );
            void *const pixels =
                malloc(zVid_Image::QueryBytesPerPixel(textPick) * textWidth * textHeight);
            zVid_Image_SetPixels(
                textPick,
                pixels,
                0
            );
            textPick->formatFlagsPacked |= 0x20;
        }

        if (textPick != 0) {
            const int clearBytes = zVid_Image::QueryBytesPerPixel(textPick) * textPick->pixelCount;
            memset(
                textPick->pixels,
                0,
                clearBytes
            );

            typedef int(__fastcall * UploadPixelsFn)(
                zVidImagePartial * image,
                HDC * outDc
            );
            typedef void(__fastcall * ReleaseSurfaceFn)(
                zVidImagePartial * image,
                HDC dc
            );

            HDC drawDc = 0;
            const unsigned int uploadAddress = g_zVideo_pfnImageUploadPixelsToSurface;
            UploadPixelsFn uploadPixels = (UploadPixelsFn)(uploadAddress);
            if (IsCallableProviderAddress(uploadAddress) && uploadPixels(
                textPick,
                &drawDc
            ) != 0) {
                RECT shadowRect = textRectRef;
                RECT mainRect = textRectRef;
                SelectObject(
                    drawDc,
                    hFont
                );

                if (shadowEnabled != 0) {
                    if (shadowOffsetX > 0) {
                        shadowRect.left += shadowOffsetX;
                    } else {
                        mainRect.left -= shadowOffsetX;
                    }

                    if (shadowOffsetY > 0) {
                        shadowRect.top += shadowOffsetY;
                    } else {
                        mainRect.top -= shadowOffsetY;
                    }

                    ::SetTextColor(
                        drawDc,
                        0x00141414
                    );
                    if (bkMode == OPAQUE) {
                        SetBkColor(
                            drawDc,
                            0x20
                        );
                    }

                    SetBkMode(
                        drawDc,
                        bkMode
                    );
                    DrawTextA(
                        drawDc,
                        textBuffer,
                        -1,
                        &shadowRect,
                        drawFormat
                    );
                }

                ::SetTextColor(
                    drawDc,
                    textColor0 == textColor1 ? textColor0 : 0x00ffffff
                );
                if (bkMode == OPAQUE) {
                    SetBkColor(
                        drawDc,
                        bkColor
                    );
                }

                SetBkMode(
                    drawDc,
                    bkMode
                );
                DrawTextA(
                    drawDc,
                    textBuffer,
                    -1,
                    &mainRect,
                    drawFormat
                );

                const unsigned int releaseAddress = g_zVideo_pfnImageReleaseSurface;
                ReleaseSurfaceFn releaseSurface = (ReleaseSurfaceFn)(releaseAddress);
                if (IsCallableProviderAddress(releaseAddress)) {
                    releaseSurface(
                        textPick,
                        drawDc
                    );
                }
            }

            TEXTMETRICA metrics = {0};
            if (GetTextMetricsA(
                measureDc,
                &metrics
            ) != 0) {
                if (textColor0 != textColor1) {
                    const unsigned short sourceWhite =
                        (unsigned short)(zVid_PackColorRGB(
                            0xff,
                            0xff,
                            0xff
                        ));
                    unsigned short *pixel = (unsigned short *)(textPick->pixels);
                    {
                        for (int row = 0; row < textPick->height; ++row) {
                            const int lineSpan = metrics.tmHeight + metrics.tmExternalLeading;
                            const int rowPhase = (shadowEnabled != 0
                                                         ? row + shadowOffsetY
                                                         : row) %
                                                 lineSpan;
                            const float blend =
                                (float)(rowPhase - metrics.tmInternalLeading) /
                                (float)(metrics.tmAscent - metrics.tmInternalLeading);
                            const unsigned int blendedColor =
                                HudUiFlashPanel::ComputeFlashBlendColor(
                                    textColor0,
                                    textColor1,
                                    blend
                                );
                            const unsigned short packedColor = (unsigned short)(zVid_PackColorRGB(
                                (unsigned char)(blendedColor & 0xffu),
                                (unsigned char)((blendedColor >> 8) & 0xffu),
                                (unsigned char)((blendedColor >> 16) & 0xffu)
                            ));

                            {
                                for (int col = 0; col < textPick->width; ++col, ++pixel) {
                                    if (*pixel == sourceWhite) {
                                        *pixel = packedColor;
                                    }
                                }
                            }
                        }
                    }
                }

                unknown274 = (int)(metrics.tmExternalLeading);
            }
        }
    }

    DeleteDC(measureDc);
    textDirty = 0;
}

/**
 * Reimplements 0x4bb2a0: HudUiPanel::UpdateTextBoundsFromContent.
 * Purpose: update the panel clip rectangle from current text contents, alignment, wrapping, and shadow state.
 */
void HudUiPanel::UpdateTextBoundsFromContent() {
    char *const panelText = textBuffer;
    const int textLength = (int)(strlen(panelText));

    if (wordWrapEnabled != 0) {
        clipRect.left = x;
        clipRect.top = y;
        clipRect.right = x + wrapRect.right;
        clipRect.bottom = y + wrapRect.bottom;
        return;
    }

    HDC hdc = CreateCompatibleDC(0);
    if (hdc == 0) {
        return;
    }

    SelectObject(
        hdc,
        hFont
    );
    SIZE textSize = {0};
    if (GetTextExtentPoint32A(
        hdc,
        panelText,
        textLength,
        &textSize
    ) != 0) {
        int left;
        if (alignMode == 1) {
            if (textDirty != 0) {
                RebuildTextRect();
            }

            left = clipRect.left + ((clipRect.right - clipRect.left) / 2) - (textWidthPx / 2);
        } else if (alignMode == 0) {
            left = clipRect.left;
        } else {
            if (textDirty != 0) {
                RebuildTextRect();
            }

            left = clipRect.right - textWidthPx;
        }

        clipRect.left = left;
        clipRect.right = left + textSize.cx;
        clipRect.bottom = clipRect.top + textSize.cy;

        if (shadowEnabled != 0) {
            clipRect.bottom += abs(shadowOffsetY);
            clipRect.right += abs(shadowOffsetX);
        }
    }

    DeleteDC(hdc);
}

// Reimplements 0x4bb1c0: HudUiPanel::MeasureTextPrefixRect
int HudUiPanel::MeasureTextPrefixRect(
    int maxChars,
    RECT *outRect
) {
    int result = 0;
    HDC hdc = CreateCompatibleDC(0);
    if (hdc == 0) {
        return 0;
    }

    SelectObject(
        hdc,
        hFont
    );
    if (maxChars > 0) {
        char *const textCopy = _strdup(textBuffer);
        if (maxChars <= (int)(strlen(textCopy))) {
            textCopy[maxChars] = '\0';
            if (DrawTextA(
                hdc,
                textCopy,
                -1,
                outRect,
                DT_CALCRECT
            ) != 0) {
                result = 1;
            }
        }

        free(textCopy);
        DeleteDC(hdc);
        return result;
    }

    if (DrawTextA(
        hdc,
        "W",
        -1,
        outRect,
        DT_CALCRECT
    ) != 0) {
        result = 1;
        outRect->right = outRect->left;
    }

    DeleteDC(hdc);
    return result;
}

/**
 * Reimplements 0x4bb710: HudUiPanel::QueryTextHeight.
 * Purpose: return the panel text height without external leading after rebuilding dirty text metrics.
 */
int HudUiPanel::QueryTextHeight() {
    if (textDirty != 0) {
        RebuildTextRect();
    }

    return textHeightPx - unknown274;
}

// Reimplements 0x40fac0: HudUiPanelSimple::Constructor
HudUiPanelSimple * HudUiPanelSimple::Constructor(
    const char *text,
    int initX,
    int initY
) {
    HudUiPanel::ConstructorDefault(
        text,
        initX,
        initY
    );
    textColor0 = 0x0020bf40;
    textColor1 = 0x0020bf40;
    textDirty = 1;
    HudUiPanel::SetFont(
        "Arial",
        0x0a,
        0x1f4,
        6,
        0,
        0,
        2
    );
    shadowOffsetX = -1;
    shadowOffsetY = -1;
    shadowEnabled = 1;
    return this;
}

// Reimplements 0x40fab0: HudUiPanelSimple::ConstructorDefaultThunk
HudUiPanelSimple * HudUiPanelSimple::ConstructorDefaultThunk() {
    return Constructor(
        0,
        0,
        0
    );
}

// Reimplements 0x40ef00: HudUiTimerPanel::SetTimeSeconds
void HudUiTimerPanel::SetTimeSeconds(
    int hours,
    int minutes,
    int seconds
) {
    if (hours >= 0 && minutes >= 0 && seconds >= 0) {
        HudUiPanel::SetTextFmt(
            "%02d:%02d:%02d",
            hours,
            minutes,
            seconds
        );
    } else {
        HudUiPanel::SetTextFmt("00:00:00");
    }

    HudUiPanel::UpdateTextBoundsFromContent();
}

// Reimplements 0x40ee60: HudUiTimerPanel::UpdateHMSFromSeconds
void HudUiTimerPanel::UpdateHMSFromSeconds(
    float seconds
) {
    elapsedSeconds = seconds;

    const int hours = (int)(floor(seconds * 0.000277777785f));
    float remaining = seconds - (float)(hours) * 3600.0f;
    const int minutes = (int)(floor(remaining * 0.0166666675f));
    remaining = (float)(remaining);
    const int displaySeconds = (int)(floor(remaining - (float)(minutes) * 60.0f));
    SetTimeSeconds(
        hours,
        minutes,
        displaySeconds
    );
}

// Reimplements 0x40eca0: HudUiTimerPanel::SetRunning
void __fastcall HudUiTimerPanel::SetRunning(
    int running
) {
    g_HudUiMgrTimerPanel->stopped = running == 0 ? 1 : 0;
}

// Reimplements 0x40ecc0: HudUiTimerPanel::SetElapsedSeconds
void __stdcall HudUiTimerPanel::SetElapsedSeconds(
    float seconds
) {
    g_HudUiMgrTimerPanel->elapsedSeconds = seconds;
}

// Reimplements 0x40ece0: HudUiTimerPanel::SetSeconds
void __stdcall HudUiTimerPanel::SetSeconds(
    float elapsedSeconds,
    float secondsStep
) {
    g_HudUiMgrTimerPanel->secondsStep = (int)(secondsStep);
    g_HudUiMgrTimerPanel->UpdateHMSFromSeconds(elapsedSeconds);
}

// Reimplements 0x40ed10: HudUiTimerPanel::GetSeconds
float HudUiTimerPanel::GetSeconds() {
    return g_HudUiMgrTimerPanel->elapsedSeconds;
}

// Reimplements 0x40ed20: HudUiTimerPanel::Update
void HudUiTimerPanel::Update(
    float deltaSeconds
) {
    if (stopped == 0) {
        const float frameDelta =
            zOpt::GetNetworkEnabled() != 0 ? g_Time_UnscaledDeltaTimeSec : g_FrameDeltaTimeSec;
        const float newElapsedSeconds =
            elapsedSeconds + (float)(secondsStep) * frameDelta;
        elapsedSeconds = newElapsedSeconds;
        UpdateHMSFromSeconds(newElapsedSeconds);
    }

    HudUiElement::Update(deltaSeconds);
}

// Reimplements 0x40fbb0: HudUiTimerPanel::ZarReadTimerData
void __stdcall HudUiTimerPanel::ZarReadTimerData(
    const float *buffer,
    int byteCount,
    HudUiTimerPanel *userData
) {
    (void)byteCount;

    userData->UpdateHMSFromSeconds(*buffer);
    HudUiMgrObjective::Begin();
}

// Reimplements 0x40fb90: HudUiTimerPanel::ZarWriteTimerDataCallback
void __fastcall HudUiTimerPanel::ZarWriteTimerDataCallback(
    zZbdSectionCallbackCtx *sectionCtx,
    HudUiTimerPanel *userData
) {
    zUtil_ZAR::WriteSectionBlob(
        sectionCtx,
        "TimerData",
        &userData->elapsedSeconds,
        sizeof(userData->elapsedSeconds)
    );
}

// Reimplements 0x40ed80: HudUiTimerPanel::ConstructorDefault
HudUiTimerPanel * HudUiTimerPanel::ConstructorDefault() {
    HudUiPanel::ConstructorDefault(
        0,
        0,
        0
    );
    textColor0 = 0x0020bf40;
    textColor1 = 0x0020bf40;
    textDirty = 1;
    HudUiPanel::SetFont(
        "Arial",
        0x0a,
        0x1f4,
        6,
        0,
        0,
        2
    );
    shadowEnabled = 1;
    shadowOffsetX = -1;
    shadowOffsetY = -1;

    secondsStep = 1;
    SetTimeSeconds(
        0,
        0,
        0
    );
    stopped = 1;
    elapsedSeconds = 0.0f;
    HudUiElement::SetVisible(1);
    g_HudUiMgr.AddChild(this);
    return this;
}

// Reimplements 0x40dbf0: HudUiCounterTextPanel::Constructor
HudUiCounterTextPanel * HudUiCounterTextPanel::Constructor() {
    HudUiPanel::ConstructorDefault(
        0,
        0,
        0
    );
    textColor0 = 0x0020bf40;
    textColor1 = 0x0020bf40;
    textDirty = 1;
    HudUiPanel::SetFont(
        "Arial",
        0x0a,
        0x1f4,
        6,
        0,
        0,
        2
    );
    shadowEnabled = 1;
    shadowOffsetX = -1;
    shadowOffsetY = -1;

    HudUiPanel::SetTextFmt(
        "%d",
        0
    );
    HudUiPanel::UpdateTextBoundsFromContent();
    HudUiElement::SetVisible(1);
    g_HudUiMgr.AddChild(this);
    return this;
}

// Reimplements 0x40dcd0: HudUiTriplet::Constructor
HudUiTriplet * HudUiTriplet::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;
    entries.rowInitFlag = 0;
    entries.begin = 0;
    entries.end = 0;
    entries.cap = 0;
    lapsColumnOffsetX = 0x23;
    killsColumnOffsetX = 0x46;
    fontSize = 8;
    fontWeight = 6;

    {
        int headerIndex;
        for (headerIndex = 0; headerIndex < 3; ++headerIndex) {
            HudUiPanel *header = NewSimplePanel(
                fontSize,
                fontWeight
            );
            headerPanels[headerIndex] = header;
            HudUiContainer::AddChild((HudUiElement *)(header));
        }
    }

    headerPanels[0]->alignMode = 2;
    headerPanels[1]->alignMode = 1;
    headerPanels[2]->alignMode = 1;
    headerPanels[0]->SetTextFmt("Player");
    headerPanels[1]->SetTextFmt("Laps");
    headerPanels[2]->SetTextFmt("Kills");

    HudUiPanel **rowCell = rowCells;
    {
        int row;
        for (row = 0; row < 8; ++row) {
            int column;
            for (column = 0; column < 3; ++column) {
                *rowCell = NewSimplePanel(
                    fontSize,
                    fontWeight
                );
                HudUiContainer::AddChild((HudUiElement *)(*rowCell));
                ++rowCell;
            }

            rowCells[row * 3 + 1]->alignMode = 1;
            rowCells[row * 3 + 2]->alignMode = 1;
        }
    }

    HudUiContainer::SetEnabled(1);
    return this;
}

// Reimplements 0x40e070: HudUiTriplet::DestructorCore
void HudUiTriplet::DestructorCore() {
    {
        int headerIndex;
        for (headerIndex = 0; headerIndex < 3; ++headerIndex) {
            HudUiPanel *header = headerPanels[headerIndex];
            if (header != 0) {
                header->Destructor();
                ::operator delete(header);
                headerPanels[headerIndex] = 0;
            }
        }
    }

    {
        int rowCellIndex;
        for (rowCellIndex = 0; rowCellIndex < 24; ++rowCellIndex) {
            HudUiPanel *rowCell = rowCells[rowCellIndex];
            if (rowCell != 0) {
                rowCell->Destructor();
                ::operator delete(rowCell);
                rowCells[rowCellIndex] = 0;
            }
        }
    }

    ::operator delete(entries.begin);
    entries.begin = 0;
    entries.end = 0;
    entries.cap = 0;

    HudUiContainer::DestructorCore();
}

// Reimplements 0x40e140: HudUiTriplet::RebuildDisplay (D:\Proj\Battlesport\HudUiTriplet.cpp)
void HudUiTriplet::RebuildDisplay() {
    HudUiTripletInsertionSort(
        entries.begin,
        entries.end
    );

    HudUiScoreboardEntry *entry = entries.begin;
    const size_t entryCount = HudUiTripletEntryCount(entries);
    size_t rowIndex = 0;
    while (entry != entries.end && rowIndex < 8) {
        HudUiPanel *const nameCell = rowCells[rowIndex * 3];
        HudUiPanel *const lapsCell = rowCells[rowIndex * 3 + 1];
        HudUiPanel *const killsCell = rowCells[rowIndex * 3 + 2];
        HudUiPanel *cells[3] = {nameCell, lapsCell, killsCell};

        {
            size_t column;
            for (column = 0; column < 3; ++column) {
                HudUiTripletPrepareCell(
                    this,
                    cells[column],
                    entry->playerColorPackedRgb
                );
                if (column != 2 || entry->lapCount >= 0) {
                    HudUiTripletSetPanelVisible(
                        cells[column],
                        1
                    );
                }
            }
        }

        const int y = baseY + (int)(rowIndex + 1) * rowPitchY;
        ((HudUiElement *)(nameCell))->SetPos(
            baseX,
            y
        );
        ((HudUiElement *)(lapsCell))->SetPos(
            baseX + lapsColumnOffsetX,
            y
        );
        ((HudUiElement *)(killsCell))->SetPos(
            baseX + killsColumnOffsetX,
            y
        );

        nameCell->SetTextFmt(
            "%s",
            entry->displayName
        );
        if (g_HudSensorTracker.raceCheckpointMode != 0) {
            lapsCell->SetTextFmt(
                "%d",
                entry->lapCount
            );
            killsCell->SetTextFmt(
                "%d",
                entry->score
            );
        } else {
            lapsCell->SetTextFmt(
                "%d",
                entry->score
            );
            HudUiTripletSetPanelVisible(
                killsCell,
                0
            );
        }

        ++entry;
        ++rowIndex;
    }

    for (; rowIndex < 8; ++rowIndex) {
        {
            size_t column;
            for (column = 0; column < 3; ++column) {
                HudUiPanel *const cell = rowCells[rowIndex * 3 + column];
                HudUiElement *const element = (HudUiElement *)(cell);
                element->flags &= 0x10u;
                HudUiTripletSetPanelVisible(
                    cell,
                    0
                );
            }
        }
    }

    if (entryCount == 0) {
        return;
    }

    ((HudUiElement *)(headerPanels[0]))->SetPos(
        baseX,
        baseY
    );
    ((HudUiElement *)(headerPanels[1]))->SetPos(
        baseX + lapsColumnOffsetX,
        baseY
    );
    ((HudUiElement *)(headerPanels[2]))->SetPos(
        baseX + killsColumnOffsetX,
        baseY
    );

    {
        int headerIndex;
        for (headerIndex = 0; headerIndex < 3; ++headerIndex) {
            headerPanels[headerIndex]->SetFont(
                "Arial",
                fontSize,
                0x1f4,
                fontWeight,
                0,
                0,
                2
            );
        }
    }

    HudUiTripletSetPanelVisible(
        headerPanels[0],
        1
    );
    if (g_HudSensorTracker.raceCheckpointMode != 0) {
        headerPanels[1]->SetTextFmt(
            "%s(%d)",
            zLoc::GetMessageString(0x113),
            g_HudSensorTracker.runtimeGoalValue
        );
        headerPanels[2]->SetTextFmt(
            "%s",
            zLoc::GetMessageString(0x114)
        );
        HudUiTripletSetPanelVisible(
            headerPanels[1],
            1
        );
        HudUiTripletSetPanelVisible(
            headerPanels[2],
            1
        );
    } else {
        headerPanels[1]->SetTextFmt(
            "%s(%d)",
            zLoc::GetMessageString(0x114),
            g_HudSensorTracker.runtimeGoalValue
        );
        HudUiTripletSetPanelVisible(
            headerPanels[1],
            1
        );
        HudUiTripletSetPanelVisible(
            headerPanels[2],
            0
        );
    }
}

// Reimplements 0x40e590: HudUiTriplet::AddEntry (D:\Proj\Battlesport\HudUiTriplet.cpp)
void HudUiTriplet::AddEntry(
    GameNetPlayerRow *entryData
) {
    HudUiScoreboardEntry sourceValue = {0};
    strncpy(
        sourceValue.displayName,
        entryData->displayName,
        0x3f
    );
    sourceValue.playerKey = entryData->playerKey;
    sourceValue.playerColorPackedRgb = entryData->playerColorPackedRgb;
    sourceValue.score = 0;
    sourceValue.lapCount = 0;

    const size_t count = HudUiTripletEntryCount(entries);
    HudUiTripletEnsureCapacity(
        entries,
        count + 1
    );
    HudUiTripletEntries::FillN(
        entries.end,
        1,
        &sourceValue
    );
    ++entries.end;
    RebuildDisplay();
}

// Reimplements 0x40e800: HudUiTriplet::UpdateEntryData (D:\Proj\Battlesport\HudUiTriplet.cpp)
void HudUiTriplet::UpdateEntryData(
    GameNetPlayerRow *entryData
) {
    HudUiScoreboardEntry *entry = entries.begin;
    for (int i = 0; entry != entries.end && i < 8; ++i) {
        if (entry->playerKey == entryData->playerKey) {
            entry->playerColorPackedRgb = entryData->playerColorPackedRgb;
            entry->score = entryData->score;
            entry->lapCount = g_HudSensorTracker.raceCheckpointMode != 0 ? entryData->lapCount : -1;
            break;
        }

        ++entry;
    }

    RebuildDisplay();
}

// Reimplements 0x40e880: HudUiTriplet::RemoveEntry (D:\Proj\Battlesport\HudUiTriplet.cpp)
void HudUiTriplet::RemoveEntry(
    GameNetPlayerRow *entryKey
) {
    HudUiScoreboardEntry *entry = entries.begin;
    for (int i = 0; entry != entries.end && i < 8; ++i) {
        if (entry->playerKey == entryKey->playerKey) {
            HudUiScoreboardEntry *const next = entry + 1;
            if (next != entries.end) {
                memmove(
                    entry,
                    next,
                    (size_t)(entries.end - next) * sizeof(HudUiScoreboardEntry)
                );
            }

            --entries.end;
            break;
        }

        ++entry;
    }

    RebuildDisplay();
}

// Reimplements 0x40ea60: HudUiTriplet::IsLocalPlayerFirstEntry
// (D:\Proj\Battlesport\HudUiTriplet.cpp)
int HudUiTriplet::IsLocalPlayerFirstEntry() {
    HudUiScoreboardEntry *const begin = entries.begin;
    int count = 0;
    if (begin != 0) {
        count = (int)(entries.end - begin);
    }

    if (count == 0) {
        return -1;
    }

    return zNetwork_GetLocalPlayerKey() == entries.begin->playerKey ? 1 : 0;
}

namespace HudScoreboard {

// Reimplements 0x40eab0: HudScoreboard::SetScaleAndRebuild
// (D:\Proj\Battlesport\HudScoreboard.cpp)
void __stdcall SetScaleAndRebuild(
    float scale
) {
    g_HudUiMgrStatsList->triplet->InterpolateLayout(scale);
    g_HudUiMgrStatsList->triplet->RebuildDisplay();
}

// Reimplements 0x40eae0: HudScoreboard::DispatchSetScale
// (D:\Proj\Battlesport\HudScoreboard.cpp)
void __stdcall DispatchSetScale(
    float deltaTime
) {
    HudUiStatsListElement *const statsList = g_HudUiMgrStatsList;
    statsList->Update(deltaTime);
}

} // namespace HudScoreboard

// Reimplements 0x4bd160: HudUiTextStack4::PushLine (D:\Proj\Battlesport\HudUiTextStack4.cpp)
HudUiPanel * HudUiTextStack4::PushLine(
    const char *message,
    float duration
) {
    SetEnabled(1);

    HudUiPanel *const firstLine = TextStackLineAt(
        this,
        0
    );
    if ((((HudUiElement *)(firstLine))->flags & 0x10u) == 0 &&
        strcmp(
            message,
            firstLine->GetLastTextPtr()
        ) != 0) {
        {
            for (int sourceIndex = 2; sourceIndex >= 0; --sourceIndex) {
                HudUiPanel *const source = TextStackLineAt(
                    this,
                    sourceIndex
                );
                HudUiPanel *const dest = TextStackLineAt(
                    this,
                    sourceIndex + 1
                );
                HudUiElement *const sourceElement = (HudUiElement *)(source);

                if ((sourceElement->flags & 0x10u) == 0) {
                    source->SetVisible(0);
                    ((HudUiElement *)(dest))->SetTimer(
                        ((HudUiElement *)(source))->timer
                    );
                    dest->SetTextFmt(source->GetLastTextPtr());
                    dest->textColor0 = source->textColor0;
                    dest->textColor1 = source->textColor1;
                    dest->textDirty = 1;
                    dest->SetVisible(1);
                }
            }
        }
    }

    ((HudUiElement *)(firstLine))->SetTimer(duration);
    firstLine->SetTextFmt(
        "%s",
        message
    );
    firstLine->SetVisible(1);
    return firstLine;
}

// Reimplements 0x4bd470: zTimedTask::RemoveFromActiveList (D:\Proj\Battlesport\hud.cpp)
void zTimedTask::RemoveFromActiveList() {
    zTimedTask *node = g_zTimedTask_ActiveHead;
    zTimedTask *previous = 0;
    if (node == 0) {
        return;
    }

    while (node != this) {
        previous = node;
        node = node->next;
        if (node == 0) {
            return;
        }
    }

    if (previous == 0) {
        g_zTimedTask_ActiveHead = g_zTimedTask_ActiveHead->next;
        --g_zTimedTask_ActiveCount;
        return;
    }

    if (node == g_zTimedTask_ActiveTail) {
        g_zTimedTask_ActiveTail = previous;
    }

    previous->next = node->next;
    --g_zTimedTask_ActiveCount;
}

// Reimplements 0x4bd4d0: zTimedTask::RunImmediateAction (D:\Proj\Battlesport\hud.cpp)
void zTimedTask::RunImmediateAction() {
    switch (kind) {
    case 1:
        if (actionArg2 != 0) {
            zVid_Image::BlitToActiveTarget(
                (zVidImagePartial *)(actionArg2),
                actionArg0,
                actionArg1,
                (unsigned short)(actionArg3),
                (zVidRect32 *)(actionArg4)
            );
        }
        break;

    case 2:
        zRndr_DrawImmediateLine(
            actionArg0,
            actionArg1,
            actionArg2,
            actionArg3,
            actionArg4
        );
        break;

    case 3:
        zRndr_RasterizePoly(
            (zVec3 *)(&actionArg0),
            rasterVertexCount,
            rasterDrawParam
        );
        break;

    case 4: {
        const char *text = (const char *)(&actionArg2) + 2;
        if (*text != '\0') {
            zImage_Font::BlitStringToActiveTarget(
                text,
                (short)(actionArg0),
                (short)(actionArg1),
                (short)(actionArg2)
            );
        }
        break;
    }

    case 5: {
        const char *text = (const char *)(actionArg3);
        if (text != 0 && *text != '\0') {
            zImage_Font::BlitStringToActiveTarget(
                text,
                (short)(actionArg0),
                (short)(actionArg1),
                (short)(actionArg2)
            );
        }
        break;
    }

    case 6:
        zRndr_SpanOcclusion_TestSample(
            actionArg0,
            actionArg1,
            actionArg2
        );
        break;

    case 7: {
        zVec3 point0;
        zVec3 point1;
        int point0Clipped;
        int point1Clipped;
        point0.x = (float)(actionArg0);
        point0.y = (float)(actionArg1);
        point1.x = (float)(actionArg2);
        point1.y = (float)(actionArg3);

        if (HudLineClip::ClipSegmentToCurrentBounds(
                &point0,
                &point1,
                &point0Clipped,
                &point1Clipped
            ) != 0) {
            zRndr_DrawImmediateLine(
                (int)(point0.x),
                (int)(point0.y),
                (int)(point1.x),
                (int)(point1.y),
                actionArg4
            );
        }
        break;
    }

    case 8:
        zRndr_DrawClippedImmediateLineStrip(
            (const zRndr_LinePoint2I *)(&actionArg0),
            alphaPointCount - 1,
            (void *)(alpha255),
            alphaVariantIndex
        );
        break;

    default:
        break;
    }
}

// Reimplements 0x4bd660: zTimedTask::TickActiveList (D:\Proj\Battlesport\hud.cpp)
void zTimedTask::TickActiveList() {
    zTimedTask *task = g_zTimedTask_ActiveHead;
    while (task != 0) {
        if ((task->flags & 0x02) == 0) {
            task->RunImmediateAction();
        } else if ((task->flags & 0x04) != 0) {
            task->RunImmediateAction();
            task->flags &= ~0x04;
        } else if ((task->flags & 0x08) != 0) {
            task->RunImmediateAction();
            task->flags &= ~0x08;
        }

        if ((task->flags & 0x01) != 0) {
            task->remainingSeconds -= g_FrameDeltaTimeSec;
            if (task->remainingSeconds <= 0.0f) {
                task->kind = 9;
                task->RemoveFromActiveList();
            }
        }

        task = task->next;
    }
}

namespace HudUi {
// Reimplements 0x4bc760: HudUi::SetInvalidateMode (D:\Proj\Battlesport\hudui.cpp)
void __fastcall SetInvalidateMode(
    int mode
) {
    g_HudUi_InvalidateMask = mode != 0 ? 0x0c : 0x04;
}

// Reimplements 0x438350: HudUi::ShowMessageBox (D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp)
int __fastcall ShowMessageBox(
    const char *messageText,
    const char *titleText,
    void *modalContext
) {
    HudUiMessageBoxDialog dialog;
    dialog.Constructor(
        "dialog.zrd",
        "MESSAGEBOX"
    );
    const int result = dialog.RunModal(
        messageText,
        titleText,
        modalContext,
        -1.0f
    );
    dialog.Destructor();
    return result;
}

// Reimplements 0x426150: HudUi::HandleHotkeyCommand (D:\Proj\Battlesport\hudui.cpp)
void __fastcall HandleHotkeyCommand(
    int commandId
) {
    switch (commandId) {
    case 9:
        Player::ToggleSteeringModeAndResetMouseLook();
        return;
    case 30:
        Player::ApplyCameraState(0);
        return;
    case 31:
        Player::ApplyCameraState(2);
        return;
    case 32:
        HudUiMgr::ToggleHud();
        return;
    case 33:
        zOpt::ToggleHudTypeForCurrentHwMode();
        return;
    case 35:
        if (zOpt::GetNetworkEnabled() == 0) {
            HudUiCallback::QueueCheatCodeState();
        }
        zInput::Keyboard_ResetTransitionState();
        return;
    case 36:
        if (g_HudUi_AuxOverlayEnabled == 0) {
            g_HudUi_AuxOverlayEnabled = 1;
            HudUiMgr::SetFloatTimerVisible(1);
            HudUiMgr::SetAuxOverlayVisible(1);
        } else {
            g_HudUi_AuxOverlayEnabled = 0;
            HudUiMgr::SetFloatTimerVisible(0);
            HudUiMgr::SetAuxOverlayVisible(0);
        }
        return;
    case 42:
        GameNet::BeginChatCompose();
        return;
    case 43:
        if (zOpt::GetThrottleMode() == 0) {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x24c),
                5.0f
            );
            zOpt::SetThrottleMode(1);
        } else {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x24d),
                5.0f
            );
            zOpt::SetThrottleMode(0);
        }
        return;
    case 44:
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiMgrObjective::Show(
                0,
                "Message",
                zLoc::GetMessageString(0x86),
                2.0f
            );
        } else if (HudUiMainMenuDialog::CanLoadGame() != 0) {
            RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_QUICKLOAD);
        } else {
            HudUiMgrObjective::Show(
                0,
                "Message",
                zLoc::GetMessageString(0x87),
                2.0f
            );
        }
        return;
    case 45:
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiMgrObjective::Show(
                0,
                "Message",
                zLoc::GetMessageString(0x85),
                2.0f
            );
        } else if (HudUiMainMenuDialog::CanSaveGame() != 0) {
            RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
                RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED
            );
        } else {
            HudUiMgrObjective::Show(
                0,
                "Message",
                zLoc::GetMessageString(0x82),
                2.0f
            );
        }
        return;
    default:
        return;
    }
}

// Reimplements 0x42bf40: HudUi::PlayPowerupSfx
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall PlayPowerupSfx(
    int shouldPlay
) {
    zSndSample *powerupSample = g_HudUi_PowerupSample;
    if ((g_HudUi_PowerupSampleInitFlags & 1) == 0) {
        g_HudUi_PowerupSampleInitFlags = (unsigned char)(g_HudUi_PowerupSampleInitFlags | 1);
        powerupSample = zSnd::FindSampleByName("snd_powerup");
        g_HudUi_PowerupSample = powerupSample;
    }

    if (shouldPlay != 0) {
        powerupSample->PlayA3DSimple(1.0f);
        return;
    }

    powerupSample->StopActiveVoicesIfPlaying();
}

// Reimplements 0x4138d0: HudUi::ShowTopMessageLine (D:\Proj\Battlesport\hud.cpp)
void __fastcall ShowTopMessageLine(
    const char *message,
    float duration
) {
    HudUiTextStack4 *const topStack = g_HudUiTopMessageStack;
    if (topStack->enabled != 0) {
        topStack->PushLine(
            message,
            duration
        );
    }
}

// Reimplements 0x4138f0: HudUi::ShowChatLine (D:\Proj\Battlesport\hud.cpp)
void __fastcall ShowChatLine(
    const char *message,
    float duration
) {
    HudUiTextStack4 *const chatStack = g_HudUiChatMessageStack;
    if (chatStack->enabled != 0) {
        chatStack->PushLine(
            message,
            duration
        );
    }
}

// Reimplements 0x4143b0: HudUi::RefreshScoreboardEntryRow (D:\Proj\Battlesport\HudUi.cpp)
void __fastcall RefreshScoreboardEntryRow(
    GameNetPlayerRow *entryData
) {
    g_HudUiMgrStatsList->triplet->UpdateEntryData(entryData);
}

// Reimplements 0x4143c0: HudUi::RemoveScoreboardEntryRow (D:\Proj\Battlesport\HudUi.cpp)
void __fastcall RemoveScoreboardEntryRow(
    GameNetPlayerRow *entryKey
) {
    g_HudUiMgrStatsList->triplet->RemoveEntry(entryKey);
}

// Reimplements 0x4bd280: HudUi::PushTopMessageLine (D:\Proj\Battlesport\hud.cpp)
void __fastcall PushTopMessageLine(
    const char *message,
    float duration
) {
    g_HudUiTopMessageStack->PushLine(
        message,
        duration
    );
}
} // namespace HudUi

// Reimplements 0x4bd3d0: HudUiTextStack4::SetTextColors
void HudUiTextStack4::SetTextColors(
    unsigned int color0,
    unsigned int color1
) {
    for (int index = 3; index >= 0; --index) {
        HudUiPanel *const panel = TextStackLineAt(
            this,
            index
        );
        panel->textColor0 = color0;
        panel->textColor1 = color1;
        panel->textDirty = 1;
    }
}

// Reimplements 0x4bd2a0: HudUiTextStack4::Clear
void HudUiTextStack4::Clear() {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAt(
            this,
            index
        );
        panel->SetTextFmt("");
        panel->SetVisible(0);
    }
}

// Reimplements 0x4bd110: HudUiTextStack4::SetFontAll
// (D:\Proj\Battlesport\HudUiTextStack4.cpp)
void HudUiTextStack4::SetFontAll(
    const char *faceName,
    int height,
    int weight,
    int width
) {
    for (int index = 3; index >= 0; --index) {
        HudUiPanel *const panel = TextStackLineAt(
            this,
            index
        );
        panel->SetFont(
            faceName,
            height,
            weight,
            width,
            0,
            0,
            2
        );
    }
}

// Reimplements 0x4bd410: HudUiTextStack4::SetXAll
void HudUiTextStack4::SetXAll(
    int newX
) {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAt(
            this,
            index
        );
        panel->SetX(newX);
    }
}

// Reimplements 0x4bd440: HudUiTextStack4::SetYDescending
void HudUiTextStack4::SetYDescending(
    int yStart
) {
    int y = yStart;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAt(
            this,
            index
        );
        panel->SetY(y);
        y -= 0x12;
    }
}

// Reimplements 0x4bd020: HudUiTopMessageStack::Constructor
HudUiTopMessageStack * HudUiTopMessageStack::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;

    {
        for (int index = 0; index < 4; ++index) {
            TextStackLineAt(
                this,
                index
            )->ConstructorDefault(
                0,
                0,
                0
            );
        }
    }

    int y = 0x1e;
    {
        for (int index = 0; index < 4; ++index) {
            ConfigureTextStackLine(
                this,
                TextStackLineAt(
                    this,
                    index
                ),
                y,
                0x0d,
                0x258,
                7
            );
            y += 0x12;
        }
    }

    return this;
}

// Reimplements 0x40fe90: HudUiTopMessageStack::DestructorCore
void HudUiTopMessageStack::DestructorCore() {
    DestroyTextStackLines(this);
}

// Reimplements 0x4bd2d0: HudUiChatMessageStack::Constructor
HudUiChatMessageStack * HudUiChatMessageStack::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;

    {
        for (int index = 0; index < 4; ++index) {
            TextStackLineAt(
                this,
                index
            )->ConstructorDefault(
                0,
                0,
                0
            );
        }
    }

    int y = 0x159;
    {
        for (int index = 0; index < 4; ++index) {
            HudUiPanel *const panel = TextStackLineAt(
                this,
                index
            );
            panel->textColor0 = 0x00996a00;
            panel->textColor1 = 0x0095c7ff;
            panel->textDirty = 1;
            ConfigureTextStackLine(
                this,
                panel,
                y,
                0x0a,
                0x1f4,
                6
            );
            y -= 0x12;
        }
    }

    return this;
}

// Reimplements 0x40fef0: HudUiChatMessageStack::DestructorCore
void HudUiChatMessageStack::DestructorCore() {
    DestroyTextStackLines(this);
}

// Reimplements 0x40f040: HudUiTimerPanelFloat::Draw
void HudUiTimerPanelFloat::Draw() {
    HudUiPanel::Invalidate();
    HudUiPanel::SetTextFmt(
        "%2.1f",
        (double)(displayValue)
    );
    HudUiPanel::Draw();
}

// Reimplements 0x40ef60: HudUiTimerPanelFloat::ConstructorDefault
HudUiTimerPanelFloat * HudUiTimerPanelFloat::ConstructorDefault() {
    HudUiPanel::ConstructorDefault(
        " ",
        3,
        0x1c
    );
    textColor0 = 0x0020bf40;
    textColor1 = 0x0020bf40;
    textDirty = 1;
    HudUiPanel::SetFont(
        "Arial",
        0x0a,
        0x1f4,
        6,
        0,
        0,
        2
    );
    shadowEnabled = 1;
    shadowOffsetX = -1;
    shadowOffsetY = -1;

    clipRect.left = x;
    clipRect.top = y;
    clipRect.right = x + 0x3c;

    sampleFrameCount = 0.0f;
    displayValue = 0.0f;
    sampleElapsedSec = 0.0f;
    clipRect.bottom = y + 0x0f;
    HudUiElement::SetVisible(0);
    return this;
}
