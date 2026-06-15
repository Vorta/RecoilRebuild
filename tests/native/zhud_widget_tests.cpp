#include "GameZRecoil/zHud/zhud_ui.h"
#include "Battlesport/HudUiMpExitDialog.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/hud.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" unsigned int g_HudUi_InvalidateMask;
extern "C" int g_RecoilApp_QuitAfterCredits;
extern "C" HWND g_RecoilApp_hWndMain;
extern RecoilApp g_RecoilApp;

namespace zOpt {
int GetNetworkModemEnabled();
}

namespace {
struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

template <typename Method> void *MethodAddress(Method method) {
    union MethodToFunction {
        Method method;
        void *function;
    };

    MethodToFunction thunk{};
    thunk.method = method;
    return thunk.function;
}

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(
        patch.original,
        patch.address,
        sizeof(patch.original)
    );

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
        patch.address = nullptr;
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t relativeOffset =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(patch.address + sizeof(patch.original));
    *reinterpret_cast<std::int32_t *>(patch.address + 1) =
        static_cast<std::int32_t>(relativeOffset);

    DWORD ignored = 0;
    VirtualProtect(
        patch.address,
        sizeof(patch.original),
        oldProtect,
        &ignored
    );
    FlushInstructionCache(
        GetCurrentProcess(),
        patch.address,
        sizeof(patch.original)
    );
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        std::memcpy(
            patch.address,
            patch.original,
            sizeof(patch.original)
        );
        DWORD ignored = 0;
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            oldProtect,
            &ignored
        );
    }
    FlushInstructionCache(
        GetCurrentProcess(),
        patch.address,
        sizeof(patch.original)
    );
    patch.address = nullptr;
}

int g_mpExitLocalPlayerFirst;
int g_mpExitCaptureSelector;
zVidImagePartial *g_mpExitCaptureImage;
void *g_mpExitFxPixels;
int g_mpExitFxWidth;
int g_mpExitFxHeight;
int g_mpExitFxPitch;
int g_mpExitBlurCount;
int g_mpExitBlurModes[4];
zVidRect32 *g_mpExitBlurRects[4];
float g_mpExitScale;
const char *g_mpExitLoadPath;
const char *g_mpExitLoadSection;
int g_mpExitLoadCapture;
zReader::Node *g_mpExitLoadResult;
int g_mpExitBindCount;
const char *g_mpExitBindNames[4];
HudUiWidget *g_mpExitBindWidgets[4];
int g_mpExitFreeCount;
int g_mpExitFreeArg;
int g_mpExitChildFlags;
int g_mpExitRefreshCount;
HudUiZrdWidget *g_mpExitRefreshThis;
int g_mpExitEnableTopCount;
int g_mpExitPrimaryWidth;
int g_mpExitTextStackX;
HudUiTextStack4 *g_mpExitTextStackThis;
int g_mpExitNetworkModem;
unsigned int g_mpExitLocIds[4];
int g_mpExitLocCount;
char g_mpExitMessages[4][16];
const char *g_mpExitShownMessages[4];
float g_mpExitShownDurations[4];
int g_mpExitShowCount;
int g_mpExitBackgroundEnabled;
HudUiBackground *g_mpExitBackgroundThis;

void ResetMpExitLayoutProbe() {
    g_mpExitCaptureSelector = -1;
    g_mpExitFxPixels = nullptr;
    g_mpExitFxWidth = -1;
    g_mpExitFxHeight = -1;
    g_mpExitFxPitch = -1;
    g_mpExitBlurCount = 0;
    for (int index = 0; index < 4; ++index) {
        g_mpExitBlurModes[index] = -1;
        g_mpExitBlurRects[index] = reinterpret_cast<zVidRect32 *>(1);
        g_mpExitBindNames[index] = nullptr;
        g_mpExitBindWidgets[index] = nullptr;
        g_mpExitLocIds[index] = 0;
        g_mpExitShownMessages[index] = nullptr;
        g_mpExitShownDurations[index] = -1.0f;
        std::sprintf(
            g_mpExitMessages[index],
            "msg%d",
            index
        );
    }
    g_mpExitScale = -1.0f;
    g_mpExitLoadPath = nullptr;
    g_mpExitLoadSection = nullptr;
    g_mpExitLoadCapture = -1;
    g_mpExitBindCount = 0;
    g_mpExitFreeCount = 0;
    g_mpExitFreeArg = 0;
    g_mpExitChildFlags = -1;
    g_mpExitRefreshCount = 0;
    g_mpExitRefreshThis = nullptr;
    g_mpExitEnableTopCount = 0;
    g_mpExitTextStackX = -1;
    g_mpExitTextStackThis = nullptr;
    g_mpExitLocCount = 0;
    g_mpExitShowCount = 0;
    g_mpExitBackgroundEnabled = -1;
    g_mpExitBackgroundThis = nullptr;
}

int FakeMpExitIsLocalPlayerFirstInStatsList() {
    return g_mpExitLocalPlayerFirst;
}

zVidImagePartial *__fastcall FakeMpExitCaptureSurfaceToImage(
    int selector
) {
    g_mpExitCaptureSelector = selector;
    return g_mpExitCaptureImage;
}

void __fastcall FakeMpExitFxSetSurfaceState(
    void *pixels,
    int width,
    int height,
    int pitchBytes
) {
    g_mpExitFxPixels = pixels;
    g_mpExitFxWidth = width;
    g_mpExitFxHeight = height;
    g_mpExitFxPitch = pitchBytes;
}

void __fastcall FakeMpExitBlurRegionByMode(
    zVidRect32 *rectOrNull,
    int mode
) {
    const int index = g_mpExitBlurCount;
    if (index < 4) {
        g_mpExitBlurRects[index] = rectOrNull;
        g_mpExitBlurModes[index] = mode;
    }
    ++g_mpExitBlurCount;
}

void __stdcall FakeMpExitSetScaleAndRebuild(
    float scale
) {
    g_mpExitScale = scale;
}

void FakeMpExitEnableTopAndChatStacks() {
    ++g_mpExitEnableTopCount;
}

int FakeMpExitGetPrimarySurfaceWidth() {
    return g_mpExitPrimaryWidth;
}

int FakeMpExitGetNetworkModemEnabled() {
    return g_mpExitNetworkModem;
}

char *__fastcall FakeMpExitGetMessageString(
    unsigned int messageId
) {
    const int index = g_mpExitLocCount;
    if (index < 4) {
        g_mpExitLocIds[index] = messageId;
    }
    ++g_mpExitLocCount;
    return index < 4 ? g_mpExitMessages[index] : g_mpExitMessages[3];
}

void __fastcall FakeMpExitShowTopMessageLine(
    const char *message,
    float duration
) {
    const int index = g_mpExitShowCount;
    if (index < 4) {
        g_mpExitShownMessages[index] = message;
        g_mpExitShownDurations[index] = duration;
    }
    ++g_mpExitShowCount;
}

struct MpExitDialogPatchOps {
    zReader::Node *LoadFromZrd(
        const char *path,
        const char *section,
        int capturePrimary
    ) {
        g_mpExitLoadPath = path;
        g_mpExitLoadSection = section;
        g_mpExitLoadCapture = capturePrimary;
        return g_mpExitLoadResult;
    }

    int BindWidgetByName(
        zReader::Node *,
        HudUiWidget *widget,
        const char *name
    ) {
        const int index = g_mpExitBindCount;
        if (index < 4) {
            g_mpExitBindNames[index] = name;
            g_mpExitBindWidgets[index] = widget;
        }
        ++g_mpExitBindCount;
        return 1;
    }

    void FreeLoadedTreeRoots(
        int rootArg
    ) {
        ++g_mpExitFreeCount;
        g_mpExitFreeArg = rootArg;
    }

    void SetChildFlags(
        unsigned int flags
    ) {
        g_mpExitChildFlags = static_cast<int>(flags);
    }

    void RefreshState() {
        ++g_mpExitRefreshCount;
        g_mpExitRefreshThis = reinterpret_cast<HudUiZrdWidget *>(this);
    }

    void SetXAll(
        int x
    ) {
        g_mpExitTextStackX = x;
        g_mpExitTextStackThis = reinterpret_cast<HudUiTextStack4 *>(this);
    }

    void SetEnabled(
        int enabled
    ) {
        g_mpExitBackgroundEnabled = enabled;
        g_mpExitBackgroundThis = reinterpret_cast<HudUiBackground *>(this);
    }
};

float g_mpExitClusterSetScales[4];
int g_mpExitClusterSetScaleCount;
float g_mpExitClusterDispatchScales[4];
int g_mpExitClusterDispatchCount;
int g_mpExitClusterRunPostCount;
zVidImagePartial *g_mpExitClusterBlitImage;
int g_mpExitClusterBlitDstX;
int g_mpExitClusterBlitDstY;
int g_mpExitClusterBlitClipFlags;
zVidRect32 *g_mpExitClusterBlitRect;
HudUiBackground *g_mpExitClusterUpdateThis;
float g_mpExitClusterUpdateDelta;
int g_mpExitClusterUpdateCount;
int g_mpExitClusterTopUpdateCount;
float g_mpExitClusterTopUpdateDelta;
int g_mpExitClusterUnlockCount;
int g_mpExitClusterGetWindowCount;
zOpt_ViewRectSection g_mpExitClusterWindowA;
zOpt_ViewRectSection g_mpExitClusterWindowB;
zVidRect32 *g_mpExitClusterAdjustSrc;
zVidRect32 *g_mpExitClusterAdjustDst;
int g_mpExitClusterAdjustWait;
int g_mpExitClusterAdjustBlit;
int g_mpExitClusterAdjustCount;
HudUiBackground *g_mpExitClusterSetEnabledThis;
int g_mpExitClusterSetEnabledValue;
int g_mpExitClusterUnloadUpdateCount;
float g_mpExitClusterUnloadUpdateDelta;
HudUiMpExitDialog *g_mpExitClusterUnloadUpdateThis;
HudUiTextStack4 *g_mpExitClusterClearThis;
int g_mpExitClusterClearCount;
zVidImagePartial *g_mpExitClusterReleaseImage;
int g_mpExitClusterQueueSwitchCount;
RecoilApp_IState *g_mpExitClusterSwitchStates[4];
int g_mpExitClusterSwitchParams[4];
int g_mpExitClusterQueueEnterFlag;
int g_mpExitClusterQueueEnterCount;
HudUiZrdWidget *g_mpExitClusterActivateThis[4];
int g_mpExitClusterActivateCount;
HudUiZrdWidget *g_mpExitClusterDestroyedWidgets[4];
int g_mpExitClusterWidgetDtorCount;
int g_mpExitOnEnterAccelerationMode;
HudUiMpExitDialog *g_mpExitOnEnterLoadThis;
int g_mpExitOnEnterLoadCount;

void ResetMpExitClusterProbe() {
    g_mpExitClusterSetScaleCount = 0;
    g_mpExitClusterDispatchCount = 0;
    g_mpExitClusterRunPostCount = 0;
    g_mpExitClusterBlitImage = nullptr;
    g_mpExitClusterBlitDstX = -1;
    g_mpExitClusterBlitDstY = -1;
    g_mpExitClusterBlitClipFlags = -1;
    g_mpExitClusterBlitRect = reinterpret_cast<zVidRect32 *>(1);
    g_mpExitClusterUpdateThis = nullptr;
    g_mpExitClusterUpdateDelta = -1.0f;
    g_mpExitClusterUpdateCount = 0;
    g_mpExitClusterTopUpdateCount = 0;
    g_mpExitClusterTopUpdateDelta = -1.0f;
    g_mpExitClusterUnlockCount = 0;
    g_mpExitClusterGetWindowCount = 0;
    g_mpExitClusterAdjustSrc = nullptr;
    g_mpExitClusterAdjustDst = nullptr;
    g_mpExitClusterAdjustWait = -1;
    g_mpExitClusterAdjustBlit = -1;
    g_mpExitClusterAdjustCount = 0;
    g_mpExitClusterSetEnabledThis = nullptr;
    g_mpExitClusterSetEnabledValue = -1;
    g_mpExitClusterUnloadUpdateCount = 0;
    g_mpExitClusterUnloadUpdateDelta = -1.0f;
    g_mpExitClusterUnloadUpdateThis = nullptr;
    g_mpExitClusterClearThis = nullptr;
    g_mpExitClusterClearCount = 0;
    g_mpExitClusterReleaseImage = nullptr;
    g_mpExitClusterQueueSwitchCount = 0;
    g_mpExitClusterQueueEnterFlag = -1;
    g_mpExitClusterQueueEnterCount = 0;
    g_mpExitClusterActivateCount = 0;
    g_mpExitClusterWidgetDtorCount = 0;
    for (int index = 0; index < 4; ++index) {
        g_mpExitClusterSetScales[index] = -1.0f;
        g_mpExitClusterDispatchScales[index] = -1.0f;
        g_mpExitClusterSwitchStates[index] = nullptr;
        g_mpExitClusterSwitchParams[index] = -1;
        g_mpExitClusterActivateThis[index] = nullptr;
        g_mpExitClusterDestroyedWidgets[index] = nullptr;
    }
}

void ResetMpExitOnEnterProbe() {
    g_mpExitOnEnterLoadThis = nullptr;
    g_mpExitOnEnterLoadCount = 0;
}

void __stdcall FakeMpExitClusterSetScaleAndRebuild(float scale) {
    if (g_mpExitClusterSetScaleCount < 4) {
        g_mpExitClusterSetScales[g_mpExitClusterSetScaleCount] = scale;
    }
    ++g_mpExitClusterSetScaleCount;
}

void __stdcall FakeMpExitClusterDispatchSetScale(float deltaSeconds) {
    if (g_mpExitClusterDispatchCount < 4) {
        g_mpExitClusterDispatchScales[g_mpExitClusterDispatchCount] = deltaSeconds;
    }
    ++g_mpExitClusterDispatchCount;
}

int FakeMpExitClusterRunPostprocessOnPrimaryBuffer() {
    ++g_mpExitClusterRunPostCount;
    return 1;
}

void __fastcall FakeMpExitClusterBlitToActiveTarget(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
) {
    g_mpExitClusterBlitImage = image;
    g_mpExitClusterBlitDstX = dstX;
    g_mpExitClusterBlitDstY = dstY;
    g_mpExitClusterBlitClipFlags = clipFlags;
    g_mpExitClusterBlitRect = srcRect;
}

int FakeMpExitClusterDispatchUnlockPrimarySurfaceState() {
    ++g_mpExitClusterUnlockCount;
    return 1;
}

zOpt_ViewRectSection *FakeMpExitClusterGetWindowSection() {
    ++g_mpExitClusterGetWindowCount;
    return g_mpExitClusterGetWindowCount == 1 ? &g_mpExitClusterWindowA
                                              : &g_mpExitClusterWindowB;
}

int __fastcall FakeMpExitClusterAdjustSurfacesIfEnabled(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
) {
    ++g_mpExitClusterAdjustCount;
    g_mpExitClusterAdjustSrc = srcRect;
    g_mpExitClusterAdjustDst = dstRect;
    g_mpExitClusterAdjustWait = waitForPresent;
    g_mpExitClusterAdjustBlit = blitPrimaryToSwFirst;
    return 1;
}

void FakeMpExitClusterQueueEnterWithReconfigureFlag(int flag) {
    ++g_mpExitClusterQueueEnterCount;
    g_mpExitClusterQueueEnterFlag = flag;
}

int FakeMpExitOnEnterGetAccelerationOption() {
    return g_mpExitOnEnterAccelerationMode;
}

int __fastcall FakeMpExitClusterReleaseIfNotDefault(zVidImagePartial *image) {
    g_mpExitClusterReleaseImage = image;
    return 0;
}

struct MpExitClusterPatchOps {
    void BackgroundUpdate(float deltaSeconds) {
        ++g_mpExitClusterUpdateCount;
        g_mpExitClusterUpdateThis = reinterpret_cast<HudUiBackground *>(this);
        g_mpExitClusterUpdateDelta = deltaSeconds;
    }

    void ContainerUpdateAll(float deltaSeconds) {
        ++g_mpExitClusterTopUpdateCount;
        g_mpExitClusterTopUpdateDelta = deltaSeconds;
    }

    void BackgroundSetEnabled(int enabled) {
        g_mpExitClusterSetEnabledThis = reinterpret_cast<HudUiBackground *>(this);
        g_mpExitClusterSetEnabledValue = enabled;
    }

    void MpDialogUpdate(float deltaSeconds) {
        ++g_mpExitClusterUnloadUpdateCount;
        g_mpExitClusterUnloadUpdateThis = reinterpret_cast<HudUiMpExitDialog *>(this);
        g_mpExitClusterUnloadUpdateDelta = deltaSeconds;
    }

    void TextStackClear() {
        ++g_mpExitClusterClearCount;
        g_mpExitClusterClearThis = reinterpret_cast<HudUiTextStack4 *>(this);
    }

    RecoilPtr32 QueueSwitchCurrentState(
        RecoilApp_IState *state,
        int stateParam
    ) {
        const int index = g_mpExitClusterQueueSwitchCount;
        if (index < 4) {
            g_mpExitClusterSwitchStates[index] = state;
            g_mpExitClusterSwitchParams[index] = stateParam;
        }
        ++g_mpExitClusterQueueSwitchCount;
        return 0;
    }

    void ZrdWidgetOnActivate() {
        const int index = g_mpExitClusterActivateCount;
        if (index < 4) {
            g_mpExitClusterActivateThis[index] = reinterpret_cast<HudUiZrdWidget *>(this);
        }
        ++g_mpExitClusterActivateCount;
    }

    void ZrdWidgetDestructorCore() {
        const int index = g_mpExitClusterWidgetDtorCount;
        if (index < 4) {
            g_mpExitClusterDestroyedWidgets[index] = reinterpret_cast<HudUiZrdWidget *>(this);
        }
        ++g_mpExitClusterWidgetDtorCount;
    }

    void MpExitLoadLayout() {
        ++g_mpExitOnEnterLoadCount;
        g_mpExitOnEnterLoadThis = reinterpret_cast<HudUiMpExitDialog *>(this);
    }

};

void DestroyHudCmdDialogDescriptionPanelForSmoke(
    HudCmdDialog &dialog
) {
    dialog.descriptionPanel.HudUiPanel::~HudUiPanel();
    dialog.descriptionPanel.textPick = nullptr;
    dialog.descriptionPanel.hFont = nullptr;
}

int g_optionsDialogLoadCalls;
bool g_optionsDialogLoadArgsOk;
int g_creditsPanelLoadCalls;
bool g_creditsPanelLoadArgsOk;
int g_cmdDialogLoadCalls;
bool g_cmdDialogLoadArgsOk;
int g_cmdDialogRebuildCalls;
int g_cmdDialogRebuildGroup;
int g_cmdDialogSetChildFlagsCalls;
int g_cmdDialogSetChildFlagsValue;
int g_cmdDialogApplySecondaryProbeCalls;
void *g_cmdDialogApplySecondaryProbeThis;
int g_cmdDialogApplySecondaryProbeKeyCode;
int g_cmdDialogApplySecondaryProbeCommandIndex;
int g_cmdDialogApplyJoystickProbeCalls;
void *g_cmdDialogApplyJoystickProbeThis;
int g_cmdDialogApplyJoystickProbeButtonCode;
int g_cmdDialogApplyJoystickProbeCommandIndex;
int g_cmdDialogApplyMouseProbeCalls;
void *g_cmdDialogApplyMouseProbeThis;
int g_cmdDialogApplyMouseProbeButtonCode;
int g_cmdDialogApplyMouseProbeCommandIndex;
int g_cmdResetDefaultBindingCalls;
int g_cmdResetLookupRebuildCalls;
int g_cmdResetBaseActivateCalls;
int g_elementBlitCount;
zVidImagePartial *g_elementBlitImage;
int g_elementBlitX;
int g_elementBlitY;
int g_elementBlitFlags;
zVidRect32 g_elementBlitRect;
int g_elementBlitHasRect;
int g_tripletPanelBlitCount;
zVidImagePartial *g_tripletPanelBlitImages[4];
int g_layoutActivatedCount;
int g_widgetInvalidateRectGetCenterXCount;
int g_widgetInvalidateRectGetCenterYCount;
int g_widgetInvalidateRectInvalidateCount;
void *g_widgetInvalidateRectInvalidateThis;
int g_widgetDrawBaseCount;
void *g_widgetDrawBaseThis;
int g_widgetDrawBlitCount;
zVidImagePartial *g_widgetDrawBlitImages[4];
int g_widgetDrawBlitX[4];
int g_widgetDrawBlitY[4];
int g_widgetDrawBlitFlags[4];
int g_widgetDrawBlitHasRect[4];
zVidRect32 g_widgetDrawBlitRects[4];
int g_panelDrawBaseCount;
void *g_panelDrawBaseThis;
int g_panelRebuildTextRectCount;
void *g_panelRebuildTextRectThis;
int g_elementUpdateDrawCount;
int g_elementUpdateDrawBaseCount;
int g_elementUpdateInvalidateCount;
int g_circleDrawBaseCount;
int g_circlePointOpCount;
void *g_circlePointOpFrameBuffer;
int g_circlePointOpArgs[3];

struct CaptureActivatedLayout : HudLayoutBase {
    void OnActivated();
};

struct TestWidgetInvalidateRect : HudUiWidget {
    virtual int GetCenterX();
    virtual int GetCenterY();
    virtual void Invalidate();
};

struct TestWidgetDrawOps : HudUiWidget {
    virtual void DrawBase();
};

struct TestPanelDrawOps : HudUiPanel {
    void DrawBase();
    void RebuildTextRect();
};

struct TestElementUpdateElement : HudUiElement {
    void Draw();
    void DrawBase();
    void Invalidate();
};

struct TestCircleDrawDirtyOps : HudUiCircle {
    void DrawBase();
};

void __fastcall CaptureHudElementBlit(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
) {
    ++g_elementBlitCount;
    g_elementBlitImage = image;
    g_elementBlitX = dstX;
    g_elementBlitY = dstY;
    g_elementBlitFlags = clipFlags;
    g_elementBlitHasRect = srcRect != nullptr ? 1 : 0;
    if (srcRect != nullptr) {
        g_elementBlitRect = *srcRect;
    }
}

void __fastcall CaptureTripletPanelBlit(
    zVidImagePartial *image,
    int,
    int,
    int,
    zVidRect32 *
) {
    const int index = g_tripletPanelBlitCount;
    if (index < 4) {
        g_tripletPanelBlitImages[index] = image;
    }
    ++g_tripletPanelBlitCount;
}

void __fastcall CaptureWidgetDrawBlit(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
) {
    const int index = g_widgetDrawBlitCount;
    if (index < 4) {
        g_widgetDrawBlitImages[index] = image;
        g_widgetDrawBlitX[index] = dstX;
        g_widgetDrawBlitY[index] = dstY;
        g_widgetDrawBlitFlags[index] = clipFlags;
        g_widgetDrawBlitHasRect[index] = srcRect != nullptr ? 1 : 0;
        if (srcRect != nullptr) {
            g_widgetDrawBlitRects[index] = *srcRect;
        }
    }
    ++g_widgetDrawBlitCount;
}

void CaptureActivatedLayout::OnActivated() {
    ++g_layoutActivatedCount;
}

int TestWidgetInvalidateRect::GetCenterX() {
    ++g_widgetInvalidateRectGetCenterXCount;
    return 7;
}

int TestWidgetInvalidateRect::GetCenterY() {
    ++g_widgetInvalidateRectGetCenterYCount;
    return 11;
}

void TestWidgetInvalidateRect::Invalidate() {
    ++g_widgetInvalidateRectInvalidateCount;
    g_widgetInvalidateRectInvalidateThis = this;
}

void TestWidgetDrawOps::DrawBase() {
    ++g_widgetDrawBaseCount;
    g_widgetDrawBaseThis = this;
}

void TestPanelDrawOps::DrawBase() {
    ++g_panelDrawBaseCount;
    g_panelDrawBaseThis = this;
}

void TestPanelDrawOps::RebuildTextRect() {
    ++g_panelRebuildTextRectCount;
    g_panelRebuildTextRectThis = this;
    HudUiPanel::RebuildTextRect();
}

void TestElementUpdateElement::Draw() {
    ++g_elementUpdateDrawCount;
}

void TestElementUpdateElement::DrawBase() {
    ++g_elementUpdateDrawBaseCount;
}

void TestElementUpdateElement::Invalidate() {
    ++g_elementUpdateInvalidateCount;
    HudUiElement::Invalidate();
}

void TestCircleDrawDirtyOps::DrawBase() {
    ++g_circleDrawBaseCount;
}

void __fastcall CaptureCirclePointOp(
    void *frameBuffer,
    int y,
    int x,
    int color16
) {
    ++g_circlePointOpCount;
    g_circlePointOpFrameBuffer = frameBuffer;
    g_circlePointOpArgs[0] = y;
    g_circlePointOpArgs[1] = x;
    g_circlePointOpArgs[2] = color16;
}

struct OptionsDialogLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * OptionsDialogLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_optionsDialogLoadCalls;
    g_optionsDialogLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(
            zrdPath,
            "dialog.zrd"
        ) == 0 &&
        sectionName != nullptr &&
        std::strcmp(
            sectionName,
            "OPTIONSPANEL"
        ) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *HudUiBackgroundLoadFromZrdAddress() {
    return MethodAddress(&HudUiBackground::LoadFromZrd);
}

void *OptionsDialogLoadProbeAddress() {
    return MethodAddress(&OptionsDialogLoadProbe::LoadFromZrd);
}

HudOptionsDialog *CreateOptionsDialogForSmoke(
    int &result
) {
    CodeFunctionPatch loadPatch{};
    g_optionsDialogLoadCalls = 0;
    g_optionsDialogLoadArgsOk = false;
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            OptionsDialogLoadProbeAddress(),
            loadPatch
        )) {
        result = 1;
        return nullptr;
    }

    void *const storage = ::operator new(sizeof(HudOptionsDialog));
    HudOptionsDialog *const dialog = new (storage) HudOptionsDialog;
    RestoreFunctionPatch(loadPatch);
    if (g_optionsDialogLoadCalls != 1 || !g_optionsDialogLoadArgsOk) {
        result = 2;
    }
    return dialog;
}

struct CreditsPanelLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * CreditsPanelLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_creditsPanelLoadCalls;
    g_creditsPanelLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(
            zrdPath,
            "dialog.zrd"
        ) == 0 &&
        sectionName != nullptr &&
        std::strcmp(
            sectionName,
            "CREDITSPANEL"
        ) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *CreditsPanelLoadProbeAddress() {
    return MethodAddress(&CreditsPanelLoadProbe::LoadFromZrd);
}

template <typename T> T &ZhudFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(base) + offset);
}

bool ZhudFloatNear(float actual, float expected) {
    const float delta = actual - expected;
    return delta > -0.0001f && delta < 0.0001f;
}

HudUiPanelLayoutEntry *AllocateCreditsPanelEntries(int count) {
    HudUiPanelLayoutEntry *const entries =
        static_cast<HudUiPanelLayoutEntry *>(
            ::operator new(sizeof(HudUiPanelLayoutEntry) * count)
        );
    std::memset(
        entries,
        0,
        sizeof(HudUiPanelLayoutEntry) * count
    );
    return entries;
}

void InitCreditsPanelEntry(
    HudUiPanelLayoutEntry *entry,
    const char *text,
    int x,
    int y
) {
    entry->panel.ConstructorDefault(
        text,
        x,
        y
    );
    entry->layoutX = x;
    entry->layoutY = y;
}

bool CreditsPanelEntryMatches(
    const HudUiPanelLayoutEntry &entry,
    const char *text,
    int x,
    int y
) {
    return entry.layoutX == x && entry.layoutY == y &&
           std::strcmp(entry.panel.textBuffer, text) == 0;
}

int g_PanelLayoutDestroyCount = 0;
HudUiPanel *g_PanelLayoutDestroyPanels[4] = {};

void __fastcall CountPanelDestructorThunk(HudUiPanel *panel) {
    if (g_PanelLayoutDestroyCount < 4) {
        g_PanelLayoutDestroyPanels[g_PanelLayoutDestroyCount] = panel;
    }

    ++g_PanelLayoutDestroyCount;
}

void InitSingleCreditsPanelSpan(
    HudUiPanelSpan *span,
    const char *text,
    int x,
    int y
) {
    span->allocatorProxy = 0;
    span->begin = AllocateCreditsPanelEntries(1);
    span->end = span->begin + 1;
    span->cap = span->end;
    InitCreditsPanelEntry(
        &span->begin[0],
        text,
        x,
        y
    );
}

void InitScrollingCreditsTextForDestructor(
    HudUiZrdScrollingText *text
) {
    new (text) HudUiZrdScrollingText;
    text->rows.begin =
        static_cast<HudUiPanelSpan *>(::operator new(sizeof(HudUiPanelSpan) * 2));
    text->rows.end = text->rows.begin + 2;
    text->rows.cap = text->rows.end;
    InitSingleCreditsPanelSpan(
        &text->rows.begin[0],
        "scroll a",
        100,
        200
    );
    InitSingleCreditsPanelSpan(
        &text->rows.begin[1],
        "scroll b",
        101,
        201
    );
}

int ScrollingCreditsTextDestructorFailureBits(
    const HudUiZrdScrollingText &text
) {
    int failure = 0;
    failure |= text.rows.begin == nullptr ? 0 : 1;
    failure |= text.rows.end == nullptr ? 0 : 2;
    failure |= text.rows.cap == nullptr ? 0 : 4;
    return failure;
}

void InitCreditsPanelForDestructor(
    HudUiCreditsPanel *panel
) {
    new ((HudUiBackground *)panel) HudUiBackground;
    new (&panel->backButton) HudUiCreditsBackButton;
    new (&panel->quitButton) HudUiCreditsQuitButton;
    InitScrollingCreditsTextForDestructor(&panel->creditsScreen);
}

int CreditsPanelDestructorFailureBits(
    const HudUiCreditsPanel &panel
) {
    int failure = 0;
    failure |= panel.creditsScreen.rows.begin == nullptr ? 0 : 1;
    failure |= panel.creditsScreen.rows.end == nullptr ? 0 : 2;
    failure |= panel.creditsScreen.rows.cap == nullptr ? 0 : 4;
    return failure;
}

RecoilApp_StateQueueItem *CreditsQueueItemAt(
    RecoilApp_StateQueue &queue,
    int index
) {
    if (index < 0 || index >= queue.m_itemCount || queue.m_readBlock.m_cursor == nullptr) {
        return nullptr;
    }

    return queue.m_readBlock.m_cursor[index];
}

void CleanupCreditsQueue(
    RecoilApp_StateQueue &queue
) {
    const int itemCount = queue.m_itemCount;
    for (int index = 0; index < itemCount; ++index) {
        ::operator delete(CreditsQueueItemAt(
            queue,
            index
        ));
    }

    if (queue.m_chunkBaseList != nullptr) {
        if (queue.m_readBlock.m_chunkBaseSlot != nullptr &&
            queue.m_writeBlock.m_chunkBaseSlot != nullptr) {
            for (RecoilApp_StateQueueItem ***slot = queue.m_readBlock.m_chunkBaseSlot;
                 slot <= queue.m_writeBlock.m_chunkBaseSlot;
                 ++slot) {
                ::operator delete(*slot);
            }
        }
        ::operator delete(queue.m_chunkBaseList);
    }

    std::memset(
        &queue,
        0,
        sizeof(queue)
    );
}

void InitCreditsPanelForUpdate(
    HudUiCreditsPanel *panel,
    float progress,
    float step
) {
    new ((HudUiBackground *)panel) HudUiBackground;
    new (&panel->creditsScreen) HudUiZrdScrollingText;
    panel->creditsScreen.rows.begin = nullptr;
    panel->creditsScreen.rows.end = nullptr;
    panel->creditsScreen.rows.cap = nullptr;
    panel->fadeProgress = progress;
    panel->fadeStep = step;
}

struct CreditsLeaveNetworkState : RecoilApp_IState {
    void OnEnter() {
    }
};

struct CmdDialogLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * CmdDialogLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_cmdDialogLoadCalls;
    g_cmdDialogLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(zrdPath, "dialog.zrd") == 0 &&
        sectionName != nullptr &&
        std::strcmp(sectionName, "COMMANDS_DIALOG") == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *CmdDialogLoadProbeAddress() {
    return MethodAddress(&CmdDialogLoadProbe::LoadFromZrd);
}

struct CmdDialogRebuildProbe {
    void RebuildCommandBindingListsForGroup(int groupIndex);
};

void CmdDialogRebuildProbe::RebuildCommandBindingListsForGroup(
    int groupIndex
) {
    ++g_cmdDialogRebuildCalls;
    g_cmdDialogRebuildGroup = groupIndex;
}

void *CmdDialogRebuildProbeAddress() {
    return MethodAddress(&CmdDialogRebuildProbe::RebuildCommandBindingListsForGroup);
}

struct HudUiContainerSetChildFlagsProbe {
    void SetChildFlags(int flags);
};

void HudUiContainerSetChildFlagsProbe::SetChildFlags(
    int flags
) {
    ++g_cmdDialogSetChildFlagsCalls;
    g_cmdDialogSetChildFlagsValue = flags;
}

void *HudUiContainerSetChildFlagsProbeAddress() {
    return MethodAddress(&HudUiContainerSetChildFlagsProbe::SetChildFlags);
}

int __fastcall FakeHudCmdBindGroupCount() {
    return 0;
}

void __fastcall FakeHudCmdInitDefaultBindings() {
    ++g_cmdResetDefaultBindingCalls;
}

void __fastcall FakeHudCmdRebuildLookupIndices() {
    ++g_cmdResetLookupRebuildCalls;
}

struct HudCmdZrdActivateProbe {
    void OnActivate();
};

struct HudCmdDialogApplySecondaryProbe {
    int ApplySecondaryKeyRebind(
        int keyCode,
        int commandIndex
    );
};

struct HudCmdDialogApplyJoystickProbe {
    int ApplyJoystickButtonRebind(
        int buttonCode,
        int commandIndex
    );
};

struct HudCmdDialogApplyMouseProbe {
    int ApplyMouseButtonRebind(
        int buttonCode,
        int commandIndex
    );
};

void HudCmdZrdActivateProbe::OnActivate() {
    ++g_cmdResetBaseActivateCalls;
}

int HudCmdDialogApplySecondaryProbe::ApplySecondaryKeyRebind(
    int keyCode,
    int commandIndex
) {
    ++g_cmdDialogApplySecondaryProbeCalls;
    g_cmdDialogApplySecondaryProbeThis = this;
    g_cmdDialogApplySecondaryProbeKeyCode = keyCode;
    g_cmdDialogApplySecondaryProbeCommandIndex = commandIndex;
    return 1;
}

int HudCmdDialogApplyJoystickProbe::ApplyJoystickButtonRebind(
    int buttonCode,
    int commandIndex
) {
    ++g_cmdDialogApplyJoystickProbeCalls;
    g_cmdDialogApplyJoystickProbeThis = this;
    g_cmdDialogApplyJoystickProbeButtonCode = buttonCode;
    g_cmdDialogApplyJoystickProbeCommandIndex = commandIndex;
    return 1;
}

int HudCmdDialogApplyMouseProbe::ApplyMouseButtonRebind(
    int buttonCode,
    int commandIndex
) {
    ++g_cmdDialogApplyMouseProbeCalls;
    g_cmdDialogApplyMouseProbeThis = this;
    g_cmdDialogApplyMouseProbeButtonCode = buttonCode;
    g_cmdDialogApplyMouseProbeCommandIndex = commandIndex;
    return 1;
}

void *HudCmdZrdActivateProbeAddress() {
    return MethodAddress(&HudCmdZrdActivateProbe::OnActivate);
}

void *HudCmdDialogApplySecondaryProbeAddress() {
    return MethodAddress(&HudCmdDialogApplySecondaryProbe::ApplySecondaryKeyRebind);
}

void *HudCmdDialogApplyJoystickProbeAddress() {
    return MethodAddress(&HudCmdDialogApplyJoystickProbe::ApplyJoystickButtonRebind);
}

void *HudCmdDialogApplyMouseProbeAddress() {
    return MethodAddress(&HudCmdDialogApplyMouseProbe::ApplyMouseButtonRebind);
}

char *__fastcall FakeHudCmdCommandLabel(int commandId) {
    return commandId == 7 ? const_cast<char *>("CommandSeven")
                          : const_cast<char *>("CommandFive");
}

char *__fastcall FakeHudCmdCommandHint(int commandId) {
    return commandId == 7 ? const_cast<char *>("HintSeven")
                          : const_cast<char *>("HintFive");
}

void SetupHudCmdButton(
    HudCmdBindButtonBase &button,
    const char *firstText,
    const char *secondText,
    int firstCommandId,
    int secondCommandId
) {
    button.AddBindingEntry(
        firstText,
        firstCommandId
    );
    button.AddBindingEntry(
        secondText,
        secondCommandId
    );
}

void CleanupHudCmdButton(HudCmdBindButtonBase &button) {
    button.DestructorCore();
}

void InstallHudCmdDialogBinding(
    HudCmdBindButtonBase &button,
    const char *text
) {
    HudCmdBindingEntry *const entry =
        static_cast<HudCmdBindingEntry *>(::operator new(sizeof(HudCmdBindingEntry)));
    entry->displayText = _strdup(text);
    entry->commandId = 5;

    HudCmdBindingEntry **const slots =
        static_cast<HudCmdBindingEntry **>(::operator new(sizeof(HudCmdBindingEntry *)));
    slots[0] = entry;
    button.bindingVec.begin = slots;
    button.bindingVec.end = slots + 1;
    button.bindingVec.capacity = slots + 1;
}

int g_hudCmdBindButtonDestructorCoreCalls = 0;

void __fastcall CountHudCmdBindButtonDestructorCore(
    HudCmdBindButtonBase *button
) {
    (void)button;
    ++g_hudCmdBindButtonDestructorCoreCalls;
}

template <typename Button>
int RunHudCmdDerivedDestructorSmoke(
    const char *text
) {
    CodeFunctionPatch corePatch{};
    if (!PatchFunctionJump(
            MethodAddress(&HudCmdBindButtonBase::DestructorCore),
            reinterpret_cast<void *>(&CountHudCmdBindButtonDestructorCore),
            corePatch
        )) {
        return 2;
    }

    void *const storage = ::operator new(sizeof(Button));
    std::memset(storage, 0, sizeof(Button));
    Button *const button = new (storage) Button;
    InstallHudCmdDialogBinding(*button, text);

    g_hudCmdBindButtonDestructorCoreCalls = 0;
    button->Destructor();

    const bool cleared =
        button->bindingVec.begin == nullptr &&
        button->bindingVec.end == nullptr &&
        button->bindingVec.capacity == nullptr &&
        g_hudCmdBindButtonDestructorCoreCalls == 0;

    ::operator delete(storage);
    RestoreFunctionPatch(corePatch);
    return cleared ? 0 : 1;
}

void SetupHudCmdDialogButtons(HudCmdDialog &dialog) {
    SetupHudCmdButton(
        dialog.commandList,
        "CommandZero",
        "CommandSeven",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.keyAButton,
        "KeyA0",
        "KeyA1",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.keyBButton,
        "KeyB0",
        "KeyB1",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.joyButton,
        "Joy0",
        "Joy1",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.mouseButton,
        "Mouse0",
        "Mouse1",
        5,
        7
    );
}

void CleanupHudCmdDialogButtons(HudCmdDialog &dialog) {
    CleanupHudCmdButton(dialog.mouseButton);
    CleanupHudCmdButton(dialog.joyButton);
    CleanupHudCmdButton(dialog.keyBButton);
    CleanupHudCmdButton(dialog.keyAButton);
    CleanupHudCmdButton(dialog.commandList);
}

void InitHudCmdInputTables() {
    zInput::BindMap_InitDikKeyNameTable();
    zInput::BindMap_InitJoystickButtonNameTable();
    zInput::BindMap_InitMouseButtonNameTable();
}

const void *HudElementVptr(const HudUiElement &element) {
    return *reinterpret_cast<const void *const *>(&element);
}

void SetHudElementVptr(
    HudUiElement &element,
    const void *vptr
) {
    *reinterpret_cast<const void **>(&element) = vptr;
}

struct CheckToggleTestChildWidget : HudUiElement {
    unsigned int deleteFlags;

    HudUiElement * ScalarDeletingDestructor(unsigned int flags) {
        deleteFlags = flags;
        return this;
    }
};
} // namespace

extern "C" int hud_ui_mp_exit_dialog_load_layout_smoke(void) {
    CodeFunctionPatch patches[15] = {};
    int patchCount = 0;
    bool installed = true;

    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&HudUiMgr::IsLocalPlayerFirstInStatsList),
                    reinterpret_cast<void *>(&FakeMpExitIsLocalPlayerFirstInStatsList),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&zVideo_buff_CaptureSurfaceToImage),
                    reinterpret_cast<void *>(&FakeMpExitCaptureSurfaceToImage),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&zVideo::Fx_SetSurfaceState),
                    reinterpret_cast<void *>(&FakeMpExitFxSetSurfaceState),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&zVideo::buff_BlurRegionByMode),
                    reinterpret_cast<void *>(&FakeMpExitBlurRegionByMode),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&HudScoreboard::SetScaleAndRebuild),
                    reinterpret_cast<void *>(&FakeMpExitSetScaleAndRebuild),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    MethodAddress(&HudUiBackground::LoadFromZrd),
                    MethodAddress(&MpExitDialogPatchOps::LoadFromZrd),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    MethodAddress(&HudUiBackground::BindWidgetByName),
                    MethodAddress(&MpExitDialogPatchOps::BindWidgetByName),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    MethodAddress(&HudUiBackground::FreeLoadedTreeRoots),
                    MethodAddress(&MpExitDialogPatchOps::FreeLoadedTreeRoots),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    MethodAddress(&HudUiContainer::SetChildFlags),
                    MethodAddress(&MpExitDialogPatchOps::SetChildFlags),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    MethodAddress(&HudUiZrdWidget::RefreshState),
                    MethodAddress(&MpExitDialogPatchOps::RefreshState),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&HudUiMgr::EnableTopAndChatStacks),
                    reinterpret_cast<void *>(&FakeMpExitEnableTopAndChatStacks),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&zVideo::GetPrimarySurfaceWidth),
                    reinterpret_cast<void *>(&FakeMpExitGetPrimarySurfaceWidth),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    MethodAddress(&HudUiTextStack4::SetXAll),
                    MethodAddress(&MpExitDialogPatchOps::SetXAll),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&zOpt::GetNetworkModemEnabled),
                    reinterpret_cast<void *>(&FakeMpExitGetNetworkModemEnabled),
                    patches[patchCount++]
                );
    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&zLoc::GetMessageString),
                    reinterpret_cast<void *>(&FakeMpExitGetMessageString),
                    patches[patchCount++]
                );

    CodeFunctionPatch showPatch = {};
    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&HudUi::ShowTopMessageLine),
                    reinterpret_cast<void *>(&FakeMpExitShowTopMessageLine),
                    showPatch
                );

    void *dispatchTable[3] = {};
    dispatchTable[1] = MethodAddress(&MpExitDialogPatchOps::SetEnabled);
    void *newGameButtonTable[64] = {};
    for (int index = 0; index < 64; ++index) {
        newGameButtonTable[index] = MethodAddress(&MpExitDialogPatchOps::RefreshState);
    }

    zVidImagePartial image = {};
    unsigned short pixels[12] = {};
    image.width = 4;
    image.height = 3;
    image.pixels = pixels;
    g_mpExitCaptureImage = &image;

    char fakeNodeStorage = 0;
    g_mpExitLoadResult = reinterpret_cast<zReader::Node *>(&fakeNodeStorage);

    HudUiTextStack4 savedTopStack = {};
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    g_HudUiTopMessageStack = &savedTopStack;

    HudUiMpExitDialog newGameDialog = {};
    *reinterpret_cast<void **>(&newGameDialog) = dispatchTable;
    *reinterpret_cast<void **>(&newGameDialog.m_mpNewGameButton) = newGameButtonTable;
    ResetMpExitLayoutProbe();
    g_mpExitLocalPlayerFirst = 1;
    g_mpExitNetworkModem = 1;
    g_mpExitPrimaryWidth = 640;
    if (installed) {
        newGameDialog.LoadLayout();
    }

    const bool newGamePath =
        installed &&
        newGameDialog.m_mpNewGameButtonMode == 1 &&
        newGameDialog.m_capturedBackgroundImage == &image &&
        newGameDialog.m_fadeElapsedSeconds == 0.0f &&
        g_mpExitCaptureSelector == 1 &&
        g_mpExitFxPixels == pixels &&
        g_mpExitFxWidth == 4 &&
        g_mpExitFxHeight == 3 &&
        g_mpExitFxPitch == 8 &&
        g_mpExitBlurCount == 3 &&
        g_mpExitBlurRects[0] == nullptr &&
        g_mpExitBlurModes[0] == 3 &&
        g_mpExitBlurRects[1] == nullptr &&
        g_mpExitBlurModes[1] == 3 &&
        g_mpExitBlurRects[2] == nullptr &&
        g_mpExitBlurModes[2] == 3 &&
        g_mpExitScale == 0.0f &&
        std::strcmp(
            g_mpExitLoadPath,
            "dialog.zrd"
        ) == 0 &&
        std::strcmp(
            g_mpExitLoadSection,
            "MPEXIT"
        ) == 0 &&
        g_mpExitLoadCapture == 1 &&
        g_mpExitBindCount == 2 &&
        std::strcmp(
            g_mpExitBindNames[0],
            "MPNEWGAME"
        ) == 0 &&
        g_mpExitBindWidgets[0] ==
            static_cast<HudUiWidget *>(&newGameDialog.m_mpNewGameButton) &&
        std::strcmp(
            g_mpExitBindNames[1],
            "MPEXITBTN"
        ) == 0 &&
        g_mpExitBindWidgets[1] ==
            static_cast<HudUiWidget *>(&newGameDialog.m_mpExitButton) &&
        g_mpExitFreeCount == 1 &&
        g_mpExitChildFlags == 0 &&
        newGameDialog.m_mpNewGameButton.modeOrEnabled == 1 &&
        g_mpExitRefreshCount == 1 &&
        g_mpExitRefreshThis == &newGameDialog.m_mpNewGameButton &&
        g_mpExitEnableTopCount == 0 &&
        g_mpExitShowCount == 0 &&
        g_mpExitBackgroundEnabled == 1 &&
        g_mpExitBackgroundThis == static_cast<HudUiBackground *>(&newGameDialog);

    HudUiMpExitDialog messageDialog = {};
    *reinterpret_cast<void **>(&messageDialog) = dispatchTable;
    ResetMpExitLayoutProbe();
    g_mpExitLocalPlayerFirst = -1;
    g_mpExitNetworkModem = 0;
    g_mpExitPrimaryWidth = 641;
    if (installed) {
        messageDialog.LoadLayout();
    }

    const bool messagePath =
        installed &&
        messageDialog.m_mpNewGameButtonMode == -1 &&
        messageDialog.m_capturedBackgroundImage == &image &&
        g_mpExitBindCount == 1 &&
        std::strcmp(
            g_mpExitBindNames[0],
            "MPEXITBTN"
        ) == 0 &&
        g_mpExitBindWidgets[0] ==
            static_cast<HudUiWidget *>(&messageDialog.m_mpExitButton) &&
        g_mpExitRefreshCount == 0 &&
        g_mpExitEnableTopCount == 1 &&
        g_mpExitTextStackThis == &savedTopStack &&
        g_mpExitTextStackX == 320 &&
        g_mpExitLocCount == 2 &&
        g_mpExitLocIds[0] == 0x39 &&
        g_mpExitLocIds[1] == 0x40 &&
        g_mpExitShowCount == 2 &&
        g_mpExitShownMessages[0] == g_mpExitMessages[0] &&
        g_mpExitShownMessages[1] == g_mpExitMessages[1] &&
        g_mpExitShownDurations[0] == 300.0f &&
        g_mpExitShownDurations[1] == 300.0f &&
        g_mpExitBackgroundEnabled == 1 &&
        g_mpExitBackgroundThis == static_cast<HudUiBackground *>(&messageDialog);

    RestoreFunctionPatch(showPatch);
    while (patchCount > 0) {
        --patchCount;
        RestoreFunctionPatch(patches[patchCount]);
    }

    g_HudUiTopMessageStack = oldTopStack;
    return newGamePath && messagePath ? 0 : 1;
}

extern "C" int hud_ui_mp_exit_dialog_table_cluster_smoke(void) {
    CodeFunctionPatch updatePatches[8] = {};
    bool installed =
        PatchFunctionJump(
            reinterpret_cast<void *>(&HudScoreboard::SetScaleAndRebuild),
            reinterpret_cast<void *>(&FakeMpExitClusterSetScaleAndRebuild),
            updatePatches[0]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&HudScoreboard::DispatchSetScale),
            reinterpret_cast<void *>(&FakeMpExitClusterDispatchSetScale),
            updatePatches[1]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
            reinterpret_cast<void *>(&FakeMpExitClusterRunPostprocessOnPrimaryBuffer),
            updatePatches[2]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVid_Image::BlitToActiveTarget),
            reinterpret_cast<void *>(&FakeMpExitClusterBlitToActiveTarget),
            updatePatches[3]
        ) &&
        PatchFunctionJump(
            MethodAddress(&HudUiBackground::Update),
            MethodAddress(&MpExitClusterPatchOps::BackgroundUpdate),
            updatePatches[4]
        ) &&
        PatchFunctionJump(
            MethodAddress(&HudUiContainer::UpdateAll),
            MethodAddress(&MpExitClusterPatchOps::ContainerUpdateAll),
            updatePatches[5]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
            reinterpret_cast<void *>(&FakeMpExitClusterDispatchUnlockPrimarySurfaceState),
            updatePatches[6]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zOpt::GetWindowSection),
            reinterpret_cast<void *>(&FakeMpExitClusterGetWindowSection),
            updatePatches[7]
        );
    CodeFunctionPatch adjustPatch{};
    installed = installed &&
                PatchFunctionJump(
                    reinterpret_cast<void *>(&zVideo::AdjustSurfacesIfEnabled),
                    reinterpret_cast<void *>(&FakeMpExitClusterAdjustSurfacesIfEnabled),
                    adjustPatch
                );

    zVidImagePartial updateImage{};
    HudUiTextStack4 savedTopStack{};
    void *topStackDispatchTable[1] = {};
    topStackDispatchTable[0] = MethodAddress(&MpExitClusterPatchOps::ContainerUpdateAll);
    *reinterpret_cast<void **>(&savedTopStack) = topStackDispatchTable;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    g_HudUiTopMessageStack = &savedTopStack;

    HudUiMpExitDialog fadeDialog{};
    fadeDialog.m_mpNewGameButtonMode = 1;
    fadeDialog.m_fadeElapsedSeconds = 0.25f;
    fadeDialog.m_capturedBackgroundImage = &updateImage;
    ResetMpExitClusterProbe();
    if (installed) {
        fadeDialog.Update(0.5f);
    }
    const bool updateFadeOk =
        installed &&
        fadeDialog.m_fadeElapsedSeconds == 0.75f &&
        g_mpExitClusterRunPostCount == 1 &&
        g_mpExitClusterBlitImage == &updateImage &&
        g_mpExitClusterBlitDstX == 0 &&
        g_mpExitClusterBlitDstY == 0 &&
        g_mpExitClusterBlitClipFlags == 0 &&
        g_mpExitClusterBlitRect == nullptr &&
        g_mpExitClusterTopUpdateCount == 0 &&
        g_mpExitClusterUnlockCount == 1 &&
        g_mpExitClusterGetWindowCount == 2 &&
        g_mpExitClusterAdjustCount == 1 &&
        g_mpExitClusterAdjustSrc == reinterpret_cast<zVidRect32 *>(&g_mpExitClusterWindowB) &&
        g_mpExitClusterAdjustDst == reinterpret_cast<zVidRect32 *>(&g_mpExitClusterWindowA) &&
        g_mpExitClusterAdjustWait == 0 &&
        g_mpExitClusterAdjustBlit == 1;

    HudUiMpExitDialog messageDialog{};
    messageDialog.m_mpNewGameButtonMode = -1;
    messageDialog.m_capturedBackgroundImage = &updateImage;
    g_Time_UnscaledDeltaTimeSec = 0.125f;
    ResetMpExitClusterProbe();
    if (installed) {
        messageDialog.Update(0.25f);
    }
    const bool updateMessageOk =
        installed &&
        g_mpExitClusterTopUpdateCount == 1 &&
        g_mpExitClusterTopUpdateDelta == 0.125f &&
        g_mpExitClusterRunPostCount == 1;

    RestoreFunctionPatch(adjustPatch);
    for (int index = 7; index >= 0; --index) {
        RestoreFunctionPatch(updatePatches[index]);
    }

    CodeFunctionPatch unloadPatches[3] = {};
    installed =
        PatchFunctionJump(
            reinterpret_cast<void *>(&HudScoreboard::SetScaleAndRebuild),
            reinterpret_cast<void *>(&FakeMpExitClusterSetScaleAndRebuild),
            unloadPatches[0]
        ) &&
        PatchFunctionJump(
            MethodAddress(&HudUiTextStack4::Clear),
            MethodAddress(&MpExitClusterPatchOps::TextStackClear),
            unloadPatches[1]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVid_Image::ReleaseIfNotDefault),
            reinterpret_cast<void *>(&FakeMpExitClusterReleaseIfNotDefault),
            unloadPatches[2]
        );

    HudUiMpExitDialog unloadDialog{};
    void *unloadDispatchTable[4] = {};
    unloadDispatchTable[1] = MethodAddress(&MpExitClusterPatchOps::BackgroundSetEnabled);
    unloadDispatchTable[3] = MethodAddress(&MpExitClusterPatchOps::MpDialogUpdate);
    *reinterpret_cast<void **>(&unloadDialog) = unloadDispatchTable;
    zVidImagePartial unloadImage{};
    unloadDialog.m_capturedBackgroundImage = &unloadImage;
    ResetMpExitClusterProbe();
    if (installed) {
        unloadDialog.UnloadLayout();
    }
    const bool unloadOk =
        installed &&
        g_mpExitClusterSetEnabledThis == static_cast<HudUiBackground *>(&unloadDialog) &&
        g_mpExitClusterSetEnabledValue == 0 &&
        g_mpExitClusterUnloadUpdateThis == &unloadDialog &&
        g_mpExitClusterUnloadUpdateDelta == 0.0f &&
        g_mpExitClusterSetScaleCount == 1 &&
        g_mpExitClusterSetScales[0] == 0.0f &&
        g_mpExitClusterClearThis == &savedTopStack &&
        g_mpExitClusterClearCount == 1 &&
        g_mpExitClusterReleaseImage == &unloadImage &&
        unloadDialog.m_capturedBackgroundImage == nullptr;

    for (int index = 2; index >= 0; --index) {
        RestoreFunctionPatch(unloadPatches[index]);
    }

    CodeFunctionPatch activatePatches[3] = {};
    installed =
        PatchFunctionJump(
            MethodAddress(&RecoilApp::QueueSwitchCurrentState),
            MethodAddress(&MpExitClusterPatchOps::QueueSwitchCurrentState),
            activatePatches[0]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag),
            reinterpret_cast<void *>(&FakeMpExitClusterQueueEnterWithReconfigureFlag),
            activatePatches[1]
        ) &&
        PatchFunctionJump(
            MethodAddress(&HudUiZrdWidget::OnActivate),
            MethodAddress(&MpExitClusterPatchOps::ZrdWidgetOnActivate),
            activatePatches[2]
        );

    HudUiMpExitDialog_NewGameButton newGameButton{};
    HudUiMpExitDialog_ExitButton exitButton{};
    ResetMpExitClusterProbe();
    if (installed) {
        newGameButton.OnActivate();
        exitButton.OnActivate();
    }
    const bool activateOk =
        installed &&
        g_mpExitClusterQueueSwitchCount == 2 &&
        g_mpExitClusterSwitchStates[0] ==
            static_cast<RecoilApp_IState *>(&g_RecoilApp.m_introFmvState) &&
        g_mpExitClusterSwitchParams[0] == 0 &&
        g_mpExitClusterQueueEnterCount == 1 &&
        g_mpExitClusterQueueEnterFlag == 1 &&
        g_mpExitClusterSwitchStates[1] ==
            static_cast<RecoilApp_IState *>(&g_RecoilApp.m_leaveNetworkState) &&
        g_mpExitClusterSwitchParams[1] == 0;

    for (int index = 2; index >= 0; --index) {
        RestoreFunctionPatch(activatePatches[index]);
    }

    CodeFunctionPatch destructorPatch{};
    installed = PatchFunctionJump(
        MethodAddress(&HudUiZrdWidget::DestructorCore),
        MethodAddress(&MpExitClusterPatchOps::ZrdWidgetDestructorCore),
        destructorPatch
    );

    void *const destructorStorage = ::operator new(sizeof(HudUiMpExitDialog));
    std::memset(
        destructorStorage,
        0,
        sizeof(HudUiMpExitDialog)
    );
    HudUiMpExitDialog *const destructorDialog =
        new (destructorStorage) HudUiMpExitDialog;
    ResetMpExitClusterProbe();
    if (installed) {
        destructorDialog->Destructor();
    }
    const bool destructorOk =
        installed &&
        g_mpExitClusterWidgetDtorCount == 2 &&
        g_mpExitClusterDestroyedWidgets[0] == &destructorDialog->m_mpExitButton &&
        g_mpExitClusterDestroyedWidgets[1] == &destructorDialog->m_mpNewGameButton;

    RestoreFunctionPatch(destructorPatch);
    ::operator delete(destructorStorage);

    g_HudUiTopMessageStack = oldTopStack;

    return updateFadeOk && updateMessageOk && unloadOk && activateOk && destructorOk ? 0 : 1;
}

extern "C" int recoil_app_mp_exit_dialog_state_on_enter_smoke(void) {
    CodeFunctionPatch patches[2] = {};
    const bool installed =
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVid::GetAccelerationOption),
            reinterpret_cast<void *>(&FakeMpExitOnEnterGetAccelerationOption),
            patches[0]
        ) &&
        PatchFunctionJump(
            MethodAddress(&HudUiMpExitDialog::LoadLayout),
            MethodAddress(&MpExitClusterPatchOps::MpExitLoadLayout),
            patches[1]
        );

    HudUiMpExitDialog *const oldDialog = g_HudUiMpExitDialog;
    RecoilApp_MpExitDialogState state{};

    HudUiMpExitDialog dispatchProbe{};
    HudUiMpExitDialog_NewGameButton newGameButtonProbe{};
    HudUiMpExitDialog_ExitButton exitButtonProbe{};
    void *const dialogDispatch = *reinterpret_cast<void **>(&dispatchProbe);
    void *const newGameDispatch = *reinterpret_cast<void **>(&newGameButtonProbe);
    void *const exitDispatch = *reinterpret_cast<void **>(&exitButtonProbe);

    g_HudUiMpExitDialog = nullptr;
    g_mpExitOnEnterAccelerationMode = 0;
    ResetMpExitOnEnterProbe();
    if (installed) {
        state.OnEnter();
    }
    HudUiMpExitDialog *const createdDialog = g_HudUiMpExitDialog;
    const bool createdOk =
        installed &&
        createdDialog != nullptr &&
        *reinterpret_cast<void **>(createdDialog) == dialogDispatch &&
        *reinterpret_cast<void **>(&createdDialog->m_mpNewGameButton) == newGameDispatch &&
        *reinterpret_cast<void **>(&createdDialog->m_mpExitButton) == exitDispatch &&
        g_mpExitOnEnterLoadCount == 1 &&
        g_mpExitOnEnterLoadThis == createdDialog;

    HudUiMpExitDialog existingDialog{};
    g_HudUiMpExitDialog = &existingDialog;
    g_mpExitOnEnterAccelerationMode = 1;
    ResetMpExitOnEnterProbe();
    if (installed) {
        state.OnEnter();
    }
    const bool existingHardwareOk =
        installed &&
        g_HudUiMpExitDialog == &existingDialog &&
        g_mpExitOnEnterLoadCount == 0;

    g_mpExitOnEnterAccelerationMode = 0;
    ResetMpExitOnEnterProbe();
    if (installed) {
        state.OnEnter();
    }
    const bool existingSoftwareOk =
        installed &&
        g_HudUiMpExitDialog == &existingDialog &&
        g_mpExitOnEnterLoadCount == 1 &&
        g_mpExitOnEnterLoadThis == &existingDialog;

    g_HudUiMpExitDialog = oldDialog;
    if (createdDialog != nullptr) {
        createdDialog->Destructor();
        ::operator delete(createdDialog);
    }
    for (int index = 1; index >= 0; --index) {
        RestoreFunctionPatch(patches[index]);
    }

    return createdOk && existingHardwareOk && existingSoftwareOk ? 0 : 1;
}

namespace {
HudUiMpExitDialog *g_mpExitDeactivateUnloadThis;
int g_mpExitDeactivateUnloadCount;
HudUiMpExitDialog *g_mpExitDeactivateDtorThis;
unsigned int g_mpExitDeactivateDtorFlags;
int g_mpExitDeactivateDtorCount;
int g_mpExitDeactivatePopCount;
DWORD g_mpExitDeactivateSleepMs;
int g_mpExitDeactivateSoundCount;
const char *g_mpExitDeactivateSoundName;
float g_mpExitDeactivateScale;
int g_mpExitDeactivateScaleCount;

void ResetMpExitDeactivateProbe() {
    g_mpExitDeactivateUnloadThis = nullptr;
    g_mpExitDeactivateUnloadCount = 0;
    g_mpExitDeactivateDtorThis = nullptr;
    g_mpExitDeactivateDtorFlags = 0;
    g_mpExitDeactivateDtorCount = 0;
    g_mpExitDeactivatePopCount = 0;
    g_mpExitDeactivateSleepMs = 0;
    g_mpExitDeactivateSoundCount = 0;
    g_mpExitDeactivateSoundName = nullptr;
    g_mpExitDeactivateScale = -1.0f;
    g_mpExitDeactivateScaleCount = 0;
}

void FakeMpExitDeactivateBindMapContextPop() {
    ++g_mpExitDeactivatePopCount;
}

void WINAPI FakeMpExitDeactivateSleep(DWORD milliseconds) {
    g_mpExitDeactivateSleepMs = milliseconds;
}

int __fastcall FakeMpExitDeactivateSampleSetDestroyByName(
    const char *setName
) {
    ++g_mpExitDeactivateSoundCount;
    g_mpExitDeactivateSoundName = setName;
    return 1;
}

void __stdcall FakeMpExitDeactivateSetScaleAndRebuild(float scale) {
    ++g_mpExitDeactivateScaleCount;
    g_mpExitDeactivateScale = scale;
}

struct MpExitDeactivatePatchOps {
    void UnloadLayout() {
        ++g_mpExitDeactivateUnloadCount;
        g_mpExitDeactivateUnloadThis = (HudUiMpExitDialog *)this;
    }

    HudUiMpExitDialog * ScalarDeletingDtor(unsigned int flags) {
        ++g_mpExitDeactivateDtorCount;
        g_mpExitDeactivateDtorThis = (HudUiMpExitDialog *)this;
        g_mpExitDeactivateDtorFlags = flags;
        return (HudUiMpExitDialog *)this;
    }
};

struct MpExitDeactivateTable {
    unsigned int slots[3];
};
} // namespace

extern "C" int recoil_app_mp_exit_dialog_state_on_deactivate_smoke(void) {
    CodeFunctionPatch patches[5] = {};
    const bool installed =
        PatchFunctionJump(
            reinterpret_cast<void *>(MethodAddress(&HudUiMpExitDialog::UnloadLayout)),
            reinterpret_cast<void *>(MethodAddress(&MpExitDeactivatePatchOps::UnloadLayout)),
            patches[0]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMapContext_Pop),
            reinterpret_cast<void *>(&FakeMpExitDeactivateBindMapContextPop),
            patches[1]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&Sleep),
            reinterpret_cast<void *>(&FakeMpExitDeactivateSleep),
            patches[2]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zSndSampleSet_DestroyByName),
            reinterpret_cast<void *>(&FakeMpExitDeactivateSampleSetDestroyByName),
            patches[3]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&HudScoreboard::SetScaleAndRebuild),
            reinterpret_cast<void *>(&FakeMpExitDeactivateSetScaleAndRebuild),
            patches[4]
        );

    MpExitDeactivateTable table = {};
    table.slots[2] =
        (unsigned int)MethodAddress(&MpExitDeactivatePatchOps::ScalarDeletingDtor);
    HudUiMpExitDialog dialog = {};
    *reinterpret_cast<const void **>(&dialog) = &table;
    HudUiMpExitDialog *const oldDialog = g_HudUiMpExitDialog;
    g_HudUiMpExitDialog = &dialog;

    RecoilApp_MpExitDialogState state{};
    ResetMpExitDeactivateProbe();
    if (installed) {
        state.OnDeactivate();
    }

    const bool ok =
        installed &&
        g_mpExitDeactivateUnloadCount == 1 &&
        g_mpExitDeactivateUnloadThis == &dialog &&
        g_mpExitDeactivateDtorCount == 1 &&
        g_mpExitDeactivateDtorThis == &dialog &&
        g_mpExitDeactivateDtorFlags == 1 &&
        g_HudUiMpExitDialog == nullptr &&
        g_mpExitDeactivatePopCount == 1 &&
        g_mpExitDeactivateSleepMs == 1000 &&
        g_mpExitDeactivateSoundCount == 1 &&
        std::strcmp(g_mpExitDeactivateSoundName, "DIALOG") == 0 &&
        g_mpExitDeactivateScaleCount == 1 &&
        g_mpExitDeactivateScale == 0.0f;

    g_HudUiMpExitDialog = oldDialog;
    for (int index = 4; index >= 0; --index) {
        RestoreFunctionPatch(patches[index]);
    }

    return ok ? 0 : 1;
}

namespace {
int g_mpExitTryHalfResMode;
int g_mpExitTryInvalidateMode;
int g_mpExitTryPitch = 1536;
int g_mpExitTryBitsPerPixel = 16;
zOpt_ViewRectSection g_mpExitTryRect;
void *g_mpExitTryPixels = reinterpret_cast<void *>(0x12345678);
void *g_mpExitTryFramePixels;
zOpt_ViewRectSection *g_mpExitTryFrameRect;
int g_mpExitTryFrameBitsPerPixel;
int g_mpExitTryFramePitch;
int g_mpExitTrySoundCount;
const char *g_mpExitTrySoundName;
zInput_BindMapContext *g_mpExitTryPushContext;
int g_mpExitTryPushCount;
int g_mpExitTryResetCount;
int g_mpExitTryAccelerationMode;
HudUiMpExitDialog *g_mpExitTryLoadThis;
int g_mpExitTryLoadCount;

void ResetMpExitTryProbe() {
    g_mpExitTryHalfResMode = -1;
    g_mpExitTryInvalidateMode = -1;
    g_mpExitTryFramePixels = nullptr;
    g_mpExitTryFrameRect = nullptr;
    g_mpExitTryFrameBitsPerPixel = -1;
    g_mpExitTryFramePitch = -1;
    g_mpExitTrySoundCount = 0;
    g_mpExitTrySoundName = nullptr;
    g_mpExitTryPushContext = reinterpret_cast<zInput_BindMapContext *>(0x1);
    g_mpExitTryPushCount = 0;
    g_mpExitTryResetCount = 0;
    g_mpExitTryLoadThis = nullptr;
    g_mpExitTryLoadCount = 0;
}

int __fastcall FakeMpExitTrySetHalfResAdjustMode(int mode) {
    g_mpExitTryHalfResMode = mode;
    return 0;
}

void __fastcall FakeMpExitTrySetInvalidateMode(int mode) {
    g_mpExitTryInvalidateMode = mode;
}

int FakeMpExitTryGetPrimarySurfacePitch() {
    return g_mpExitTryPitch;
}

int FakeMpExitTryGetDisplaySectionBitsPerPixel() {
    return g_mpExitTryBitsPerPixel;
}

zOpt_ViewRectSection *FakeMpExitTryGetWindowSection() {
    return &g_mpExitTryRect;
}

void *FakeMpExitTryGetPrimarySurfacePixels() {
    return g_mpExitTryPixels;
}

void __fastcall FakeMpExitTrySetFrameBufferRegion(
    void *pixels,
    zOpt_ViewRectSection *activeRegionRect,
    int bitsPerPixel,
    int pitchBytes
) {
    g_mpExitTryFramePixels = pixels;
    g_mpExitTryFrameRect = activeRegionRect;
    g_mpExitTryFrameBitsPerPixel = bitsPerPixel;
    g_mpExitTryFramePitch = pitchBytes;
}

int __fastcall FakeMpExitTrySampleSetInitByName(const char *setName) {
    ++g_mpExitTrySoundCount;
    g_mpExitTrySoundName = setName;
    return 1;
}

void __fastcall FakeMpExitTryBindMapContextPush(
    zInput_BindMapContext *bindMapOrNull
) {
    ++g_mpExitTryPushCount;
    g_mpExitTryPushContext = bindMapOrNull;
}

void FakeMpExitTryBindMapCurrentResetAllBindings() {
    ++g_mpExitTryResetCount;
}

int FakeMpExitTryGetAccelerationOption() {
    return g_mpExitTryAccelerationMode;
}

struct MpExitTryPatchOps {
    void LoadLayout() {
        ++g_mpExitTryLoadCount;
        g_mpExitTryLoadThis = (HudUiMpExitDialog *)this;
    }
};
} // namespace

extern "C" int recoil_app_mp_exit_dialog_state_on_try_become_current_smoke(void) {
    CodeFunctionPatch patches[12] = {};
    const bool installed =
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::SetHalfResAdjustMode),
            reinterpret_cast<void *>(&FakeMpExitTrySetHalfResAdjustMode),
            patches[0]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&HudUi::SetInvalidateMode),
            reinterpret_cast<void *>(&FakeMpExitTrySetInvalidateMode),
            patches[1]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::GetPrimarySurfacePitch),
            reinterpret_cast<void *>(&FakeMpExitTryGetPrimarySurfacePitch),
            patches[2]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zOpt::GetDisplaySectionBitsPerPixel),
            reinterpret_cast<void *>(&FakeMpExitTryGetDisplaySectionBitsPerPixel),
            patches[3]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zOpt::GetWindowSection),
            reinterpret_cast<void *>(&FakeMpExitTryGetWindowSection),
            patches[4]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::GetPrimarySurfacePixels),
            reinterpret_cast<void *>(&FakeMpExitTryGetPrimarySurfacePixels),
            patches[5]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zRndr::SetFrameBufferRegion),
            reinterpret_cast<void *>(&FakeMpExitTrySetFrameBufferRegion),
            patches[6]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zSndSampleSet_InitByName),
            reinterpret_cast<void *>(&FakeMpExitTrySampleSetInitByName),
            patches[7]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMapContext_Push),
            reinterpret_cast<void *>(&FakeMpExitTryBindMapContextPush),
            patches[8]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMapCurrent_ResetAllBindings),
            reinterpret_cast<void *>(&FakeMpExitTryBindMapCurrentResetAllBindings),
            patches[9]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVid::GetAccelerationOption),
            reinterpret_cast<void *>(&FakeMpExitTryGetAccelerationOption),
            patches[10]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(MethodAddress(&HudUiMpExitDialog::LoadLayout)),
            reinterpret_cast<void *>(MethodAddress(&MpExitTryPatchOps::LoadLayout)),
            patches[11]
        );

    HudUiMpExitDialog dialog{};
    HudUiMpExitDialog *const oldDialog = g_HudUiMpExitDialog;
    g_HudUiMpExitDialog = &dialog;
    RecoilApp_MpExitDialogState state{};

    g_mpExitTryAccelerationMode = 1;
    ResetMpExitTryProbe();
    int resultHardware = 0;
    if (installed) {
        resultHardware = state.OnTryBecomeCurrent();
    }
    const bool hardwareOk =
        installed && resultHardware == 1 && g_mpExitTryHalfResMode == 0 &&
        g_mpExitTryInvalidateMode == 0 && g_mpExitTryFramePixels == g_mpExitTryPixels &&
        g_mpExitTryFrameRect == &g_mpExitTryRect &&
        g_mpExitTryFrameBitsPerPixel == g_mpExitTryBitsPerPixel &&
        g_mpExitTryFramePitch == g_mpExitTryPitch && g_mpExitTrySoundCount == 1 &&
        std::strcmp(g_mpExitTrySoundName, "DIALOG") == 0 &&
        g_mpExitTryPushCount == 1 && g_mpExitTryPushContext == nullptr &&
        g_mpExitTryResetCount == 1 && g_mpExitTryLoadCount == 1 &&
        g_mpExitTryLoadThis == &dialog;

    g_mpExitTryAccelerationMode = 0;
    ResetMpExitTryProbe();
    int resultSoftware = 0;
    if (installed) {
        resultSoftware = state.OnTryBecomeCurrent();
    }
    const bool softwareOk =
        installed && resultSoftware == 1 && g_mpExitTryHalfResMode == 0 &&
        g_mpExitTryInvalidateMode == 0 && g_mpExitTryFramePixels == g_mpExitTryPixels &&
        g_mpExitTryFrameRect == &g_mpExitTryRect &&
        g_mpExitTryFrameBitsPerPixel == g_mpExitTryBitsPerPixel &&
        g_mpExitTryFramePitch == g_mpExitTryPitch && g_mpExitTrySoundCount == 1 &&
        std::strcmp(g_mpExitTrySoundName, "DIALOG") == 0 &&
        g_mpExitTryPushCount == 1 && g_mpExitTryPushContext == nullptr &&
        g_mpExitTryResetCount == 1 && g_mpExitTryLoadCount == 0 &&
        g_mpExitTryLoadThis == nullptr;

    g_HudUiMpExitDialog = oldDialog;
    for (int i = 11; i >= 0; --i) {
        RestoreFunctionPatch(patches[i]);
    }

    return hardwareOk && softwareOk ? 0 : 1;
}

namespace {
int g_mpExitUpdatePollDispatch;
int g_mpExitUpdatePollCount;
int g_mpExitUpdateTickCount;
HudUiMpExitDialog *g_mpExitUpdateThis;
float g_mpExitUpdateDelta;
int g_mpExitUpdateCount;
int g_mpExitUpdateLocIds[2];
int g_mpExitUpdateLocCount;
int g_mpExitUpdateStep;
int g_mpExitUpdateStepError;
DWORD g_mpExitUpdateSleepMs;
UINT g_mpExitUpdateBeepType;
HWND g_mpExitUpdateMessageHwnd;
char g_mpExitUpdateMessageText[64];
char g_mpExitUpdateMessageCaption[64];
UINT g_mpExitUpdateMessageType;
int g_mpExitUpdateExitCode;
char g_mpExitUpdatePrintfCaption[64];
char g_mpExitUpdatePrintfText[64];

void ResetMpExitUpdateProbe() {
    g_mpExitUpdatePollDispatch = -1;
    g_mpExitUpdatePollCount = 0;
    g_mpExitUpdateTickCount = 0;
    g_mpExitUpdateThis = nullptr;
    g_mpExitUpdateDelta = -1.0f;
    g_mpExitUpdateCount = 0;
    g_mpExitUpdateLocCount = 0;
    g_mpExitUpdateStep = 0;
    g_mpExitUpdateStepError = 0;
    g_mpExitUpdateSleepMs = 0;
    g_mpExitUpdateBeepType = 0;
    g_mpExitUpdateMessageHwnd = 0;
    g_mpExitUpdateMessageText[0] = 0;
    g_mpExitUpdateMessageCaption[0] = 0;
    g_mpExitUpdateMessageType = 0;
    g_mpExitUpdateExitCode = -1;
    g_mpExitUpdatePrintfCaption[0] = 0;
    g_mpExitUpdatePrintfText[0] = 0;
}

void ExpectMpExitUpdateStep(int expected) {
    ++g_mpExitUpdateStep;
    if (g_mpExitUpdateStep != expected) {
        g_mpExitUpdateStepError = 1;
    }
}

void __fastcall FakeMpExitUpdatePollActiveDevices(unsigned char dispatchCallbacks) {
    g_mpExitUpdatePollDispatch = dispatchCallbacks;
    ++g_mpExitUpdatePollCount;
}

void FakeMpExitUpdateTimeTick() {
    ++g_mpExitUpdateTickCount;
}

char *__fastcall FakeMpExitUpdateGetMessageString(int messageId) {
    if (g_mpExitUpdateLocCount < 2) {
        g_mpExitUpdateLocIds[g_mpExitUpdateLocCount] = messageId;
    }
    ++g_mpExitUpdateLocCount;
    return messageId == 28 ? const_cast<char *>("Fatal caption")
                           : const_cast<char *>("Fatal text");
}

void FakeMpExitUpdateFlipToGDI() {
    ExpectMpExitUpdateStep(1);
}

int FakeMpExitUpdateSndShutdown() {
    ExpectMpExitUpdateStep(2);
    return 1;
}

int FakeMpExitUpdateNetworkShutdown() {
    ExpectMpExitUpdateStep(3);
    return 1;
}

int FakeMpExitUpdateVideoShutdown() {
    ExpectMpExitUpdateStep(4);
    return 1;
}

int FakeMpExitUpdatePrintf(const char *, const char *caption, const char *text) {
    ExpectMpExitUpdateStep(5);
    std::strncpy(
        g_mpExitUpdatePrintfCaption,
        caption,
        sizeof(g_mpExitUpdatePrintfCaption) - 1
    );
    std::strncpy(
        g_mpExitUpdatePrintfText,
        text,
        sizeof(g_mpExitUpdatePrintfText) - 1
    );
    return 0;
}

void WINAPI FakeMpExitUpdateSleep(DWORD milliseconds) {
    ExpectMpExitUpdateStep(6);
    g_mpExitUpdateSleepMs = milliseconds;
}

BOOL WINAPI FakeMpExitUpdateMessageBeep(UINT type) {
    ExpectMpExitUpdateStep(7);
    g_mpExitUpdateBeepType = type;
    return TRUE;
}

int WINAPI FakeMpExitUpdateMessageBoxA(
    HWND hwnd,
    LPCSTR text,
    LPCSTR caption,
    UINT type
) {
    ExpectMpExitUpdateStep(8);
    g_mpExitUpdateMessageHwnd = hwnd;
    std::strncpy(
        g_mpExitUpdateMessageText,
        text,
        sizeof(g_mpExitUpdateMessageText) - 1
    );
    std::strncpy(
        g_mpExitUpdateMessageCaption,
        caption,
        sizeof(g_mpExitUpdateMessageCaption) - 1
    );
    g_mpExitUpdateMessageType = type;
    return IDOK;
}

void __fastcall FakeMpExitUpdateExitProcessWithCleanup(int exitCode) {
    ExpectMpExitUpdateStep(9);
    g_mpExitUpdateExitCode = exitCode;
}

struct MpExitUpdatePatchOps {
    void UpdateAll(float deltaSeconds) {
        ++g_mpExitUpdateCount;
        g_mpExitUpdateThis = (HudUiMpExitDialog *)this;
        g_mpExitUpdateDelta = deltaSeconds;
    }
};
} // namespace

extern "C" int recoil_app_mp_exit_dialog_state_on_update_should_quit_smoke(void) {
    CodeFunctionPatch patches[12] = {};
    const bool installed =
        PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::PollActiveDevices),
            reinterpret_cast<void *>(&FakeMpExitUpdatePollActiveDevices),
            patches[0]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&Time::Tick),
            reinterpret_cast<void *>(&FakeMpExitUpdateTimeTick),
            patches[1]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zLoc::GetMessageString),
            reinterpret_cast<void *>(&FakeMpExitUpdateGetMessageString),
            patches[2]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo_dd::FlipToGDIIfAttached),
            reinterpret_cast<void *>(&FakeMpExitUpdateFlipToGDI),
            patches[3]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zSndSystem::Shutdown),
            reinterpret_cast<void *>(&FakeMpExitUpdateSndShutdown),
            patches[4]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zNetwork::ShutdownSessionRuntime),
            reinterpret_cast<void *>(&FakeMpExitUpdateNetworkShutdown),
            patches[5]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::ShutdownVideoSystem),
            reinterpret_cast<void *>(&FakeMpExitUpdateVideoShutdown),
            patches[6]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&printf),
            reinterpret_cast<void *>(&FakeMpExitUpdatePrintf),
            patches[7]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&Sleep),
            reinterpret_cast<void *>(&FakeMpExitUpdateSleep),
            patches[8]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&MessageBeep),
            reinterpret_cast<void *>(&FakeMpExitUpdateMessageBeep),
            patches[9]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&MessageBoxA),
            reinterpret_cast<void *>(&FakeMpExitUpdateMessageBoxA),
            patches[10]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zSys::ExitProcessWithCleanup),
            reinterpret_cast<void *>(&FakeMpExitUpdateExitProcessWithCleanup),
            patches[11]
        );

    alignas(HudUiMpExitDialog) unsigned char dialogStorage[sizeof(HudUiMpExitDialog)] = {};
    HudUiMpExitDialog *const dialog = reinterpret_cast<HudUiMpExitDialog *>(dialogStorage);
    void *dialogTable[1] = {};
    dialogTable[0] = MethodAddress(&MpExitUpdatePatchOps::UpdateAll);
    *reinterpret_cast<void ***>(dialog) = dialogTable;
    HudUiMpExitDialog *const oldDialog = g_HudUiMpExitDialog;
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    const HWND oldMainHwnd = g_RecoilApp_hWndMain;
    g_HudUiMpExitDialog = dialog;
    g_FrameDeltaTimeSec = 0.375f;
    g_RecoilApp_hWndMain = (HWND)0x1234;

    RecoilApp_MpExitDialogState state{};
    if (!installed) {
        g_HudUiMpExitDialog = oldDialog;
        g_FrameDeltaTimeSec = oldFrameDelta;
        g_RecoilApp_hWndMain = oldMainHwnd;
        for (int i = 11; i >= 0; --i) {
            RestoreFunctionPatch(patches[i]);
        }
        return 2;
    }

    dialog->m_fadeElapsedSeconds = 600.0f;
    ResetMpExitUpdateProbe();
    int resultNormal = 1;
    resultNormal = state.OnUpdateShouldQuit();
    const bool normalOk =
        installed && resultNormal == 0 && g_mpExitUpdatePollCount == 1 &&
        g_mpExitUpdatePollDispatch == 0 && g_mpExitUpdateTickCount == 1 &&
        g_mpExitUpdateCount == 1 && g_mpExitUpdateThis == dialog &&
        g_mpExitUpdateDelta == g_FrameDeltaTimeSec && g_mpExitUpdateStep == 0 &&
        g_mpExitUpdateLocCount == 0;

    dialog->m_fadeElapsedSeconds = 600.25f;
    ResetMpExitUpdateProbe();
    int resultFatal = 1;
    resultFatal = state.OnUpdateShouldQuit();
    const bool fatalOk =
        installed && resultFatal == 0 && g_mpExitUpdatePollCount == 1 &&
        g_mpExitUpdatePollDispatch == 0 && g_mpExitUpdateTickCount == 1 &&
        g_mpExitUpdateCount == 1 && g_mpExitUpdateLocCount == 2 &&
        g_mpExitUpdateLocIds[0] == 28 && g_mpExitUpdateLocIds[1] == 29 &&
        g_mpExitUpdateStep == 9 && g_mpExitUpdateStepError == 0 &&
        std::strcmp(g_mpExitUpdatePrintfCaption, "Fatal caption") == 0 &&
        std::strcmp(g_mpExitUpdatePrintfText, "Fatal text") == 0 &&
        g_mpExitUpdateSleepMs == 1000 && g_mpExitUpdateBeepType == MB_ICONHAND &&
        g_mpExitUpdateMessageHwnd == (HWND)0x1234 &&
        std::strcmp(g_mpExitUpdateMessageText, "Fatal text") == 0 &&
        std::strcmp(g_mpExitUpdateMessageCaption, "Fatal caption") == 0 &&
        g_mpExitUpdateMessageType == MB_ICONHAND && g_mpExitUpdateExitCode == 0;

    g_HudUiMpExitDialog = oldDialog;
    g_FrameDeltaTimeSec = oldFrameDelta;
    g_RecoilApp_hWndMain = oldMainHwnd;
    for (int i = 11; i >= 0; --i) {
        RestoreFunctionPatch(patches[i]);
    }

    if (!normalOk) {
        return 3;
    }
    if (!fatalOk) {
        return 4;
    }
    return 0;
}

extern "C" int zhud_element_invalidate_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    HudUiElement element{};
    element.flags = 0x01;
    g_HudUi_InvalidateMask = 0x24;
    element.Invalidate();
    const bool ok = element.flags == 0x25;

    g_HudUi_InvalidateMask = oldMask;
    return ok ? 0 : 1;
}

extern "C" int zhud_element_visible_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;

    HudUiElement element{};
    element.flags = 0x10;
    element.SetVisible(1);
    const bool visible = element.flags == 0x80;

    element.SetVisible(0);
    const bool hidden = element.flags == 0x90;

    g_HudUi_InvalidateMask = oldMask;
    return visible && hidden ? 0 : 1;
}

extern "C" int zhud_element_set_timer_smoke(void) {
    HudUiElement element{};
    element.flags = 0x20;

    element.SetTimer(0.5f);
    const bool active =
        element.timer == 0.5f &&
        (element.flags & 0x01u) != 0 &&
        (element.flags & 0x10u) == 0 &&
        (element.flags & 0x20u) != 0;

    element.SetTimer(-1.0f);
    const bool expired =
        element.timer == -1.0f &&
        (element.flags & 0x01u) == 0 &&
        (element.flags & 0x10u) != 0 &&
        (element.flags & 0x20u) != 0;

    return active && expired ? 0 : 1;
}

extern "C" int zhud_panel_query_text_height_smoke(void) {
    TestPanelDrawOps panel{};
    std::strcpy(
        panel.cachedText,
        "last"
    );
    std::strcpy(
        panel.textBuffer,
        "last"
    );
    panel.textHeightPx = 17;
    panel.unknown274 = 3;
    panel.textDirty = 0;

    g_panelRebuildTextRectCount = 0;
    g_panelRebuildTextRectThis = nullptr;
    const bool cached =
        panel.QueryTextHeight() == 14 &&
        panel.GetLastTextPtr() == panel.cachedText &&
        std::strcmp(
            panel.GetLastTextPtr(),
            "last"
        ) == 0 &&
        g_panelRebuildTextRectCount == 0;

    panel.flags = 0;
    panel.x = 10;
    panel.y = 20;
    panel.textWidthPx = 30;
    HudUiRect textRect{-1, -1, -1, -1};
    panel.GetTextRect(&textRect);
    const bool textRectMatch =
        textRect.left == 10 &&
        textRect.top == 20 &&
        textRect.right == 40 &&
        textRect.bottom == 34;

    const bool hitTest =
        panel.HitTest(10, 20) == 1 &&
        panel.HitTest(39, 33) == 1 &&
        panel.HitTest(40, 20) == 0 &&
        panel.HitTest(10, 34) == 0 &&
        panel.HitTest(9, 20) == 0 &&
        panel.HitTest(10, 19) == 0;

    panel.textDirty = 1;
    panel.hFont = GetStockObject(SYSTEM_FONT);
    panel.GetLastTextPtr();
    const bool dirtyRebuilt =
        g_panelRebuildTextRectCount == 1 &&
        g_panelRebuildTextRectThis == &panel &&
        panel.textDirty == 0;

    return cached && textRectMatch && hitTest && dirtyRebuilt ? 0 : 1;
}

extern "C" int zhud_circle_constructor_and_hit_test_smoke(void) {
    HudUiCircle circle{};
    HudUiCircle *const result = new (&circle) HudUiCircle(
        10,
        20,
        5,
        0x07e0
    );

    const bool constructed =
        result == &circle &&
        circle.x == 10 &&
        circle.y == 20 &&
        circle.radius == 5 &&
        circle.radiusSquared == 25 &&
        circle.color565 == 0x07e0;

    const bool hitCore =
        circle.HitTestCore(10, 20) == 1 &&
        circle.HitTestCore(13, 23) == 1 &&
        circle.HitTestCore(15, 20) == 0 &&
        circle.HitTestCore(16, 20) == 0;
    const bool hitWrapped =
        circle.HitTest(10, 20) == 1 &&
        circle.HitTest(16, 20) == 0;

    return constructed && hitCore && hitWrapped ? 0 : 1;
}

extern "C" int zhud_circle_draw_dirty_smoke(void) {
    void *const oldFrameBuffer = zRndr::g_frameBuffer;
    zRndr::PointOpProc const oldPointOp = zRndr::g_pfnPointOpActive;
    const int oldCircleCenterX = g_zRndr_CircleCenterX;
    const int oldCircleCenterY = g_zRndr_CircleCenterY;
    const int oldAuxArg = g_zRndr_CircleDrawAuxArg;

    TestCircleDrawDirtyOps circle{};
    circle.x = 10;
    circle.y = 20;
    circle.radius = 1;
    circle.radiusSquared = 1;
    circle.color565 = 0x07e0;

    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x87651234);
    zRndr::g_pfnPointOpActive = CaptureCirclePointOp;
    g_circleDrawBaseCount = 0;
    g_circlePointOpCount = 0;
    g_circlePointOpFrameBuffer = 0;
    g_circlePointOpArgs[0] = 0;
    g_circlePointOpArgs[1] = 0;
    g_circlePointOpArgs[2] = 0;

    circle.Draw();
    const bool directOk =
        g_circleDrawBaseCount == 1 &&
        g_circlePointOpCount == 16 &&
        g_circlePointOpFrameBuffer == reinterpret_cast<void *>(0x87651234) &&
        g_zRndr_CircleCenterX == 10 &&
        g_zRndr_CircleCenterY == 20 &&
        g_zRndr_CircleDrawAuxArg == 0 &&
        g_circlePointOpArgs[2] == 0x07e0;

    g_circleDrawBaseCount = 0;
    g_circlePointOpCount = 0;
    g_circlePointOpArgs[2] = 0;

    circle.DrawDirtyForwarder();
    const bool forwarderOk =
        g_circleDrawBaseCount == 1 &&
        g_circlePointOpCount == 16 &&
        g_circlePointOpArgs[2] == 0x07e0;

    zRndr::g_frameBuffer = oldFrameBuffer;
    zRndr::g_pfnPointOpActive = oldPointOp;
    g_zRndr_CircleCenterX = oldCircleCenterX;
    g_zRndr_CircleCenterY = oldCircleCenterY;
    g_zRndr_CircleDrawAuxArg = oldAuxArg;
    if (!directOk) {
        if (g_circleDrawBaseCount != 1) {
            return 2;
        }
        if (g_circlePointOpCount != 16) {
            return 3;
        }
        if (g_circlePointOpFrameBuffer != reinterpret_cast<void *>(0x87651234)) {
            return 4;
        }
        if (g_zRndr_CircleCenterX != 10 || g_zRndr_CircleCenterY != 20) {
            return 5;
        }
        if (g_zRndr_CircleDrawAuxArg != 0) {
            return 6;
        }
        return 7;
    }
    if (!forwarderOk) {
        if (g_circleDrawBaseCount != 1) {
            return 8;
        }
        if (g_circlePointOpCount != 16) {
            return 9;
        }
        return 10;
    }
    return 0;
}

extern "C" int zhud_element_clip_and_invalidate_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;

    HudUiElement element{};
    element.Constructor(0, 0);
    element.flags = 0x01;
    g_HudUi_InvalidateMask = 0x24;
    element.Invalidate();

    HudUiRect rect{1, 2, 3, 4};
    element.SetBltSourceAndClipRect(&element, &rect);

    const bool updated =
        element.flags == 0x25 &&
        element.bltSource == &element &&
        std::memcmp(&element.clipRect, &rect, sizeof(rect)) == 0;

    element.SetClipRect(nullptr);
    const bool nullPreserved = std::memcmp(&element.clipRect, &rect, sizeof(rect)) == 0;

    g_HudUi_InvalidateMask = oldMask;
    return updated && nullPreserved ? 0 : 1;
}

extern "C" int zhud_element_constructor_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;

    HudUiElement element{};
    HudUiElement *const result = element.Constructor(17, 29);
    const bool initialized =
        result == &element &&
        element.next == nullptr &&
        element.parent == nullptr &&
        element.flags == 0 &&
        element.timer == 0.0f &&
        element.x == 17 &&
        element.y == 29 &&
        element.bltSource == nullptr &&
        element.state == 0;

    HudUiRect rect{4, 5, 6, 7};
    element.SetClipRect(&rect);
    const bool virtualsReady = std::memcmp(&element.clipRect, &rect, sizeof(rect)) == 0;

    g_HudUi_InvalidateMask = oldMask;
    return initialized && virtualsReady ? 0 : 1;
}

extern "C" int zhud_element_copy_constructor_smoke(void) {
    HudUiElement commonProbe(0, 0);
    const void *const commonVptr = HudElementVptr(commonProbe);
    const void *const preservedVptr = reinterpret_cast<const void *>(0x4444);

    HudUiElement source{};
    SetHudElementVptr(
        source,
        nullptr
    );
    source.next = reinterpret_cast<HudUiElement *>(0x1111);
    source.parent = reinterpret_cast<void *>(0x2222);
    source.flags = 0x1234;
    source.timer = 2.5f;
    source.x = 17;
    source.y = 29;
    source.bltSource = reinterpret_cast<void *>(0x3333);
    source.clipRect.left = 1;
    source.clipRect.top = 2;
    source.clipRect.right = 3;
    source.clipRect.bottom = 4;
    source.state = 5;
    source.padding32 = 6;

    HudUiElement copied{};
    copied.next = reinterpret_cast<HudUiElement *>(0xaaaa);
    copied.parent = reinterpret_cast<void *>(0xbbbb);
    copied.padding32 = 0xcccc;

    HudUiRect copiedRect{-1, -1, -1, -1};
    HudUiElement *const copiedResult = copied.CopyConstructor(&source);
    copiedResult->GetTextRect(&copiedRect);
    const bool copiedCoords =
        copiedResult->GetCenterX() == source.x &&
        copiedResult->GetCenterY() == source.y;

    HudUiElement assigned{};
    SetHudElementVptr(
        assigned,
        preservedVptr
    );
    assigned.next = reinterpret_cast<HudUiElement *>(0xaaaa);
    assigned.parent = reinterpret_cast<void *>(0xbbbb);
    assigned.padding32 = 0xdddd;
    HudUiElement *const assignedResult = assigned.CopyFrom(&source);

    HudUiElement scalar{};
    SetHudElementVptr(
        scalar,
        nullptr
    );
    HudUiElement *const scalarResult = scalar.ScalarDeletingDestructor(0);

    return copiedResult == &copied && HudElementVptr(copied) == commonVptr &&
                   copied.next == nullptr && copied.parent == nullptr &&
                   copied.flags == source.flags && copied.timer == source.timer &&
                   copied.x == source.x && copied.y == source.y &&
                   copied.bltSource == source.bltSource &&
                   copied.clipRect.left == source.clipRect.left &&
                   copied.clipRect.top == source.clipRect.top &&
                   copied.clipRect.right == source.clipRect.right &&
                   copied.clipRect.bottom == source.clipRect.bottom &&
                   copiedRect.left == source.x && copiedRect.top == source.y &&
                   copiedRect.right == source.x && copiedRect.bottom == source.y &&
                   copiedCoords && copied.state == source.state &&
                   copied.padding32 == 0xcccc && assignedResult == &assigned &&
                   HudElementVptr(assigned) == preservedVptr &&
                   assigned.next == nullptr && assigned.parent == nullptr &&
                   assigned.flags == source.flags && assigned.timer == source.timer &&
                   assigned.x == source.x && assigned.y == source.y &&
                   assigned.bltSource == source.bltSource &&
                   assigned.clipRect.left == source.clipRect.left &&
                   assigned.clipRect.top == source.clipRect.top &&
                   assigned.clipRect.right == source.clipRect.right &&
                   assigned.clipRect.bottom == source.clipRect.bottom &&
                   assigned.state == source.state && assigned.padding32 == 0xdddd &&
                   scalarResult == &scalar && HudElementVptr(scalar) == commonVptr
               ? 0
               : 1;
}

extern "C" int zhud_element_scalar_deleting_destructor_smoke(void) {
    void *const noDeleteStorage = ::operator new(sizeof(HudUiElement));
    HudUiElement *const noDeleteElement =
        new (noDeleteStorage) HudUiElement(17, 29);
    HudUiElement *const noDeleteResult =
        noDeleteElement->ScalarDeletingDestructor(0);
    ::operator delete(noDeleteStorage);

    void *const deleteStorage = ::operator new(sizeof(HudUiElement));
    HudUiElement *const deleteElement =
        new (deleteStorage) HudUiElement(31, 43);
    HudUiElement *const deleteResult =
        deleteElement->ScalarDeletingDestructor(1);

    return noDeleteResult == noDeleteElement && deleteResult == deleteElement ? 0 : 1;
}

extern "C" int zhud_element_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiElement));
    HudUiElement *const element = new (storage) HudUiElement(23, 31);
    element->~HudUiElement();
    ::operator delete(storage);

    return 0;
}

extern "C" int zhud_element_draw_dispatch_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    g_zVideo_pfnBltSourceToPrimary = CaptureHudElementBlit;

    HudUiElement element(14, 27);
    element.bltSource = nullptr;
    g_elementBlitCount = 0;
    element.Draw();
    const bool nullSkipped = g_elementBlitCount == 0;

    zVidImagePartial image{};
    HudUiRect rect{2, 3, 18, 21};
    element.bltSource = &image;
    element.clipRect = rect;
    g_elementBlitCount = 0;
    element.Draw();

    const bool blitted =
        g_elementBlitCount == 1 &&
        g_elementBlitImage == &image &&
        g_elementBlitX == 14 &&
        g_elementBlitY == 27 &&
        g_elementBlitFlags == 0 &&
        g_elementBlitHasRect != 0 &&
        g_elementBlitRect.left == 2 &&
        g_elementBlitRect.top == 3 &&
        g_elementBlitRect.right == 18 &&
        g_elementBlitRect.bottom == 21;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    return nullSkipped && blitted ? 0 : 1;
}

extern "C" int zhud_element_draw_base_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    g_zVideo_pfnBltSourceToPrimary = CaptureHudElementBlit;

    HudUiElement element(14, 27);
    element.bltSource = nullptr;
    g_elementBlitCount = 0;
    element.DrawBase();
    const bool nullSkipped = g_elementBlitCount == 0;

    zVidImagePartial image{};
    HudUiRect rect{2, 3, 18, 21};
    element.bltSource = &image;
    element.clipRect = rect;
    g_elementBlitCount = 0;
    element.DrawBase();

    const bool blitted =
        g_elementBlitCount == 1 &&
        g_elementBlitImage == &image &&
        g_elementBlitX == 14 &&
        g_elementBlitY == 27 &&
        g_elementBlitFlags == 0 &&
        g_elementBlitHasRect != 0 &&
        g_elementBlitRect.left == 2 &&
        g_elementBlitRect.top == 3 &&
        g_elementBlitRect.right == 18 &&
        g_elementBlitRect.bottom == 21;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    return nullSkipped && blitted ? 0 : 1;
}

extern "C" int zhud_element_update_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;

    g_elementUpdateDrawCount = 0;
    g_elementUpdateDrawBaseCount = 0;
    g_elementUpdateInvalidateCount = 0;
    g_HudUi_InvalidateMask = 0x80;

    TestElementUpdateElement element{};
    element.flags = 0;
    element.Update(0.1f);
    const bool visibleDraw = g_elementUpdateDrawCount == 1;

    element.flags = 0x02 | 0x04;
    element.Update(0.1f);
    const bool visibleDirty =
        g_elementUpdateDrawCount == 2 &&
        (element.flags & 0x04) == 0;

    element.flags = 0x02 | 0x08;
    element.Update(0.1f);
    const bool visibleAlternateDirty =
        g_elementUpdateDrawCount == 3 &&
        (element.flags & 0x08) == 0;

    element.flags = 0x10 | 0x02 | 0x04;
    element.Update(0.1f);
    const bool hiddenDirty =
        g_elementUpdateDrawBaseCount == 1 &&
        (element.flags & 0x04) == 0;

    element.flags = 0x10 | 0x02 | 0x08;
    element.Update(0.1f);
    const bool hiddenAlternateDirty =
        g_elementUpdateDrawBaseCount == 2 &&
        (element.flags & 0x08) == 0;

    element.flags = 0x01;
    element.timer = 0.5f;
    element.Update(0.25f);
    const bool countdownActive =
        element.timer == 0.25f &&
        (element.flags & 0x10) == 0;
    element.Update(0.25f);
    const bool countdownExpired =
        element.timer == 0.0f &&
        (element.flags & 0x10) != 0 &&
        g_elementUpdateInvalidateCount == 1;

    g_HudUi_InvalidateMask = oldMask;
    return visibleDraw && visibleDirty && visibleAlternateDirty && hiddenDirty &&
                   hiddenAlternateDirty && countdownActive && countdownExpired
               ? 0
               : 1;
}

extern "C" int zhud_widget_constructor_smoke(void) {
    HudUiWidget widget{};
    widget.imageStateWord = 0xabcd1234;
    for (HudUiRectDirty &dirtyRect : widget.dirtyRects) {
        dirtyRect.framesRemaining = 7;
    }

    HudUiWidget *const result = widget.Constructor(0x55aa);

    bool dirtyFramesCleared = true;
    for (const HudUiRectDirty &dirtyRect : widget.dirtyRects) {
        dirtyFramesCleared = dirtyFramesCleared && dirtyRect.framesRemaining == 0;
    }

    const bool initialized =
        result == &widget &&
        widget.alignFlags == 0x55aa &&
        widget.image == nullptr &&
        widget.ownsImage == 0 &&
        widget.bltClipRectOrNull == nullptr &&
        (widget.imageStateWord & 0xffffu) == 0 &&
        widget.dirtyRectCount == 0 &&
        widget.state == 0 &&
        dirtyFramesCleared;

    zVidImagePartial image{};
    image.width = 11;
    image.height = 6;
    widget.x = 10;
    widget.y = 20;
    widget.image = &image;
    widget.alignFlags = 1;
    const bool virtualsReady = widget.GetCenterX() == 15 && widget.GetCenterY() == 23;

    return initialized && virtualsReady ? 0 : 1;
}

extern "C" int zhud_widget_invalidate_rect_smoke(void) {
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;

    g_widgetInvalidateRectGetCenterXCount = 0;
    g_widgetInvalidateRectGetCenterYCount = 0;
    g_widgetInvalidateRectInvalidateCount = 0;
    g_widgetInvalidateRectInvalidateThis = nullptr;

    zVidImagePartial image{};
    image.width = 30;
    image.height = 40;

    TestWidgetInvalidateRect widget{};
    widget.x = 10;
    widget.y = 20;
    widget.alignFlags = 0;
    widget.image = &image;
    widget.dirtyRectCount = 2;
    for (HudUiRectDirty &dirtyRect : widget.dirtyRects) {
        dirtyRect.framesRemaining = 9;
    }
    widget.dirtyRects[0].framesRemaining = 0;

    g_HudUi_InvalidateMask = 0x0c;
    const HudUiRect dirtyRect = {5, 15, 50, 80};
    widget.InvalidateRect(&dirtyRect);

    const HudUiRectDirty &slot = widget.dirtyRects[0];
    const bool clipped =
        widget.dirtyRectCount == 3 &&
        slot.framesRemaining == 2 &&
        slot.drawX == 10 &&
        slot.drawY == 20 &&
        slot.srcLeft == 3 &&
        slot.srcTop == 9 &&
        slot.srcRight == 43 &&
        slot.srcBottom == 29;
    const bool dispatched = g_widgetInvalidateRectGetCenterXCount == 2 &&
                            g_widgetInvalidateRectGetCenterYCount == 2 &&
                            g_widgetInvalidateRectInvalidateCount == 1 &&
                            g_widgetInvalidateRectInvalidateThis == &widget;

    g_HudUi_InvalidateMask = oldInvalidateMask;
    return clipped && dispatched ? 0 : 1;
}

extern "C" int zhud_widget_draw_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    zVidImagePartial *const oldExclusiveImage =
        g_HudUiWidget_ExclusiveDrawImage;

    TestWidgetDrawOps widget{};
    widget.x = 17;
    widget.y = 23;

    g_widgetDrawBlitCount = 0;
    g_widgetDrawBaseCount = 0;
    g_widgetDrawBaseThis = nullptr;
    g_zVideo_pfnBltSourceToPrimary = CaptureWidgetDrawBlit;
    g_HudUiWidget_ExclusiveDrawImage = nullptr;
    widget.Draw();
    const bool nullImageSkipped =
        g_widgetDrawBlitCount == 0 &&
        g_widgetDrawBaseCount == 0 &&
        g_widgetDrawBaseThis == nullptr;

    zVidImagePartial image{};
    zVidImagePartial otherImage{};
    widget.image = &image;
    widget.dirtyRectCount = 0;
    g_widgetDrawBlitCount = 0;
    g_widgetDrawBaseCount = 0;
    g_widgetDrawBaseThis = nullptr;
    g_HudUiWidget_ExclusiveDrawImage = &otherImage;
    widget.Draw();
    const bool exclusiveSkipped =
        g_widgetDrawBlitCount == 0 &&
        g_widgetDrawBaseCount == 0 &&
        g_widgetDrawBaseThis == nullptr;

    HudUiRect clip = {1, 2, 7, 9};
    widget.bltClipRectOrNull = &clip;
    g_widgetDrawBlitCount = 0;
    g_widgetDrawBaseCount = 0;
    g_widgetDrawBaseThis = nullptr;
    g_HudUiWidget_ExclusiveDrawImage = nullptr;
    widget.Draw();
    const bool wholeWidgetDrawn =
        g_widgetDrawBaseCount == 1 &&
        g_widgetDrawBaseThis == &widget &&
        g_widgetDrawBlitCount == 1 &&
        g_widgetDrawBlitImages[0] == &image &&
        g_widgetDrawBlitX[0] == 17 &&
        g_widgetDrawBlitY[0] == 23 &&
        g_widgetDrawBlitFlags[0] == 0 &&
        g_widgetDrawBlitHasRect[0] == 1 &&
        g_widgetDrawBlitRects[0].left == 1 &&
        g_widgetDrawBlitRects[0].top == 2 &&
        g_widgetDrawBlitRects[0].right == 7 &&
        g_widgetDrawBlitRects[0].bottom == 9;

    widget.dirtyRectCount = 2;
    widget.dirtyRects[0].framesRemaining = 2;
    widget.dirtyRects[0].drawX = 31;
    widget.dirtyRects[0].drawY = 41;
    widget.dirtyRects[0].srcLeft = 3;
    widget.dirtyRects[0].srcTop = 4;
    widget.dirtyRects[0].srcRight = 13;
    widget.dirtyRects[0].srcBottom = 14;
    widget.dirtyRects[1].framesRemaining = 1;
    widget.dirtyRects[1].drawX = 51;
    widget.dirtyRects[1].drawY = 61;
    widget.dirtyRects[1].srcLeft = 5;
    widget.dirtyRects[1].srcTop = 6;
    widget.dirtyRects[1].srcRight = 15;
    widget.dirtyRects[1].srcBottom = 16;
    widget.dirtyRects[2].framesRemaining = 0;
    widget.dirtyRects[3].framesRemaining = 0;

    g_widgetDrawBlitCount = 0;
    g_widgetDrawBaseCount = 0;
    g_widgetDrawBaseThis = nullptr;
    widget.Draw();
    const bool dirtyRectsDrawn =
        g_widgetDrawBaseCount == 0 &&
        g_widgetDrawBaseThis == nullptr &&
        g_widgetDrawBlitCount == 2 &&
        widget.dirtyRectCount == 1 &&
        widget.dirtyRects[0].framesRemaining == 1 &&
        widget.dirtyRects[1].framesRemaining == 0 &&
        g_widgetDrawBlitImages[0] == &image &&
        g_widgetDrawBlitX[0] == 31 &&
        g_widgetDrawBlitY[0] == 41 &&
        g_widgetDrawBlitHasRect[0] == 1 &&
        g_widgetDrawBlitRects[0].left == 3 &&
        g_widgetDrawBlitRects[0].top == 4 &&
        g_widgetDrawBlitRects[0].right == 13 &&
        g_widgetDrawBlitRects[0].bottom == 14 &&
        g_widgetDrawBlitImages[1] == &image &&
        g_widgetDrawBlitX[1] == 51 &&
        g_widgetDrawBlitY[1] == 61 &&
        g_widgetDrawBlitHasRect[1] == 1 &&
        g_widgetDrawBlitRects[1].left == 5 &&
        g_widgetDrawBlitRects[1].top == 6 &&
        g_widgetDrawBlitRects[1].right == 15 &&
        g_widgetDrawBlitRects[1].bottom == 16;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    g_HudUiWidget_ExclusiveDrawImage = oldExclusiveImage;
    return nullImageSkipped && exclusiveSkipped && wholeWidgetDrawn && dirtyRectsDrawn
               ? 0
               : 1;
}

extern "C" int zhud_objective_update_meter_xpoints_smoke(void) {
    const HudUiWidget oldWidget = g_HudUiMgrObjectiveWidget;
    const HudUiMeter oldMeter = g_HudUiMgrObjectiveMeter;

    g_HudUiMgrObjectiveWidget = {};
    g_HudUiMgrObjectiveWidget.x = 123;
    g_HudUiMgrObjectiveWidget.alignFlags = 0;
    g_HudUiMgrObjectiveMeter = {};
    g_HudUiMgrObjectiveMeter.points[0].x = -1.0f;
    g_HudUiMgrObjectiveMeter.points[1].x = -2.0f;
    g_HudUiMgrObjectiveMeter.points[2].x = -3.0f;
    g_HudUiMgrObjectiveMeter.points[3].x = -4.0f;
    g_HudUiMgrObjectiveMeter.points[0].y = 10.0f;
    g_HudUiMgrObjectiveMeter.points[3].y = 40.0f;

    HudUiMgrObjective::UpdateMeterXPoints();

    const bool leftEdge = g_HudUiMgrObjectiveMeter.points[0].x == 128.0f &&
                          g_HudUiMgrObjectiveMeter.points[1].x == 128.0f;
    const bool rightEdge = g_HudUiMgrObjectiveMeter.points[2].x == 135.0f &&
                           g_HudUiMgrObjectiveMeter.points[3].x == 135.0f;
    const bool yUnchanged = g_HudUiMgrObjectiveMeter.points[0].y == 10.0f &&
                            g_HudUiMgrObjectiveMeter.points[3].y == 40.0f;

    g_HudUiMgrObjectiveWidget = oldWidget;
    g_HudUiMgrObjectiveMeter = oldMeter;
    return leftEdge && rightEdge && yUnchanged ? 0 : 1;
}

extern "C" int zhud_slot_draw_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    zVidImagePartial *const oldExclusiveImage =
        g_HudUiWidget_ExclusiveDrawImage;
    g_zVideo_pfnBltSourceToPrimary = CaptureHudElementBlit;
    g_HudUiWidget_ExclusiveDrawImage = nullptr;

    HudUiSlot slot{};
    slot.Constructor();
    zVidImagePartial slotImage{};
    zVidImagePartial trackMarkerImage{};
    slot.slotWidget.image = &slotImage;
    slot.slotWidget.bltSource = &slotImage;
    slot.trackMarkerWidget.image = &trackMarkerImage;
    slot.trackMarkerWidget.bltSource = &trackMarkerImage;
    slot.slotWidget.dirtyRectCount = 0;
    slot.trackMarkerWidget.dirtyRectCount = 0;
    for (HudUiRectDirty &dirtyRect : slot.slotWidget.dirtyRects) {
        dirtyRect.framesRemaining = 0;
    }
    for (HudUiRectDirty &dirtyRect : slot.trackMarkerWidget.dirtyRects) {
        dirtyRect.framesRemaining = 0;
    }

    g_elementBlitCount = 0;
    slot.Draw();
    const int bothVisibleBlits = g_elementBlitCount;

    slot.slotWidget.flags = 0x10;
    slot.trackMarkerWidget.flags = 0;
    const int beforeTrackMarkerOnly = g_elementBlitCount;
    slot.Draw();
    const int trackMarkerOnlyBlits = g_elementBlitCount - beforeTrackMarkerOnly;

    slot.slotWidget.flags = 0;
    slot.trackMarkerWidget.flags = 0x10;
    const int beforeSlotOnly = g_elementBlitCount;
    slot.Draw();
    const int slotOnlyBlits = g_elementBlitCount - beforeSlotOnly;

    g_HudUiWidget_ExclusiveDrawImage = oldExclusiveImage;
    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    slot.Destructor();

    const bool bothVisible =
        bothVisibleBlits != 0 &&
        bothVisibleBlits == trackMarkerOnlyBlits + slotOnlyBlits;
    const bool onlyTrackMarkerVisible = trackMarkerOnlyBlits != 0;
    const bool onlySlotVisible = slotOnlyBlits != 0;
    return bothVisible && onlyTrackMarkerVisible && onlySlotVisible ? 0 : 1;
}

extern "C" int zhud_triplet_panel_draw_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    zVidImagePartial *const oldExclusiveImage =
        g_HudUiWidget_ExclusiveDrawImage;
    g_zVideo_pfnBltSourceToPrimary = CaptureTripletPanelBlit;
    g_HudUiWidget_ExclusiveDrawImage = nullptr;
    g_tripletPanelBlitCount = 0;
    g_tripletPanelBlitImages[0] = nullptr;
    g_tripletPanelBlitImages[1] = nullptr;
    g_tripletPanelBlitImages[2] = nullptr;
    g_tripletPanelBlitImages[3] = nullptr;

    zVidImagePartial baseImage{};
    zVidImagePartial itemImages[3]{};

    HudUiTripletPanel panel{};
    panel.bltSource = &baseImage;
    panel.items[0].flags = 0;
    panel.items[0].image = &itemImages[0];
    panel.items[0].dirtyRectCount = 0;
    panel.items[1].flags = 0x10;
    panel.items[1].image = &itemImages[1];
    panel.items[1].dirtyRectCount = 0;
    panel.items[2].flags = 0;
    panel.items[2].image = &itemImages[2];
    panel.items[2].dirtyRectCount = 0;

    panel.Draw();

    const bool drawOrder =
        g_tripletPanelBlitCount == 3 &&
        g_tripletPanelBlitImages[0] == &baseImage &&
        g_tripletPanelBlitImages[1] == &itemImages[2] &&
        g_tripletPanelBlitImages[2] == &itemImages[0] &&
        g_tripletPanelBlitImages[3] == nullptr;

    g_HudUiWidget_ExclusiveDrawImage = oldExclusiveImage;
    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    return drawOrder ? 0 : 1;
}

extern "C" int zhud_triplet_panel_set_visible_count_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    const HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;

    g_HudUi_InvalidateMask = 0x80;

    HudUiTripletPanel panel{};
    panel.visibleCount = 99;
    panel.items[0].flags = 0x10;
    panel.items[1].flags = 0x10;
    panel.items[2].flags = 0x10;
    panel.SetVisibleCount(2);

    const bool twoVisible =
        panel.visibleCount == 2 &&
        (panel.flags & 0x80) != 0 &&
        (panel.items[0].flags & 0x10) == 0 &&
        (panel.items[1].flags & 0x10) == 0 &&
        (panel.items[2].flags & 0x10) != 0;

    panel.flags = 0;
    panel.items[0].flags = 0;
    panel.items[1].flags = 0;
    panel.items[2].flags = 0;
    panel.SetVisibleCount(-3);

    const bool noneVisible =
        panel.visibleCount == 0 &&
        (panel.flags & 0x80) != 0 &&
        (panel.items[0].flags & 0x10) != 0 &&
        (panel.items[1].flags & 0x10) != 0 &&
        (panel.items[2].flags & 0x10) != 0;

    panel.flags = 0;
    panel.SetVisibleCount(0);
    const bool unchanged = panel.flags == 0;

    g_HudUiMgrNanitePanel.visibleCount = 0;
    g_HudUiMgrNanitePanel.flags = 0;
    g_HudUiMgrNanitePanel.items[0].flags = 0x10;
    g_HudUiMgrNanitePanel.items[1].flags = 0x10;
    g_HudUiMgrNanitePanel.items[2].flags = 0x10;

    HudUiMgr::SetNanitePanelCount(2);
    const bool naniteForwarded =
        g_HudUiMgrNanitePanel.visibleCount == 2 &&
        (g_HudUiMgrNanitePanel.flags & 0x80) != 0 &&
        (g_HudUiMgrNanitePanel.items[0].flags & 0x10) == 0 &&
        (g_HudUiMgrNanitePanel.items[1].flags & 0x10) == 0 &&
        (g_HudUiMgrNanitePanel.items[2].flags & 0x10) != 0;

    g_HudUiMgrNanitePanel = oldNanitePanel;
    g_HudUi_InvalidateMask = oldMask;
    return twoVisible && noneVisible && unchanged && naniteForwarded ? 0 : 1;
}

extern "C" int zhud_mgr_trigger_current_layout_on_activated_smoke(void) {
    CaptureActivatedLayout layout{};

    HudLayoutBase *const oldCurrentLayout = g_HudUiMgrCurrentLayout;
    g_layoutActivatedCount = 0;

    g_HudUiMgrCurrentLayout = nullptr;
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    const bool nullPath = g_layoutActivatedCount == 0;

    g_HudUiMgrCurrentLayout = &layout;
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    const bool activePath = g_layoutActivatedCount == 1;

    g_HudUiMgrCurrentLayout = oldCurrentLayout;
    return nullPath && activePath ? 0 : 1;
}

extern "C" int zhud_layout_hw_update_objective_dirty_rect_smoke(void) {
    const HudUiWidget oldObjectiveWidget = g_HudUiMgrObjectiveWidget;
    const int oldObjectiveRightX = g_HudUiMgrObjectiveWidgetRightX;
    const HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    zVidImagePartial *const oldExclusiveImage =
        g_HudUiWidget_ExclusiveDrawImage;

    zVidImagePartial objectiveImage{};
    objectiveImage.width = 12;
    objectiveImage.height = 6;
    new (&g_HudUiMgrObjectiveWidget) HudUiWidget;
    g_HudUiMgrObjectiveWidget.image = &objectiveImage;
    g_HudUiMgrObjectiveWidget.x = 31;
    g_HudUiMgrObjectiveWidget.y = 34;
    g_HudUiMgrObjectiveWidget.alignFlags = 1;
    g_HudUiMgrObjectiveWidgetRightX = 80;

    HudLayoutHW layout{};
    zVidImagePartial layoutImage{};
    layoutImage.width = 100;
    layoutImage.height = 100;
    layout.widget2.image = &layoutImage;
    layout.widget2.x = 10;
    layout.widget2.y = 20;
    layout.widget2.dirtyRectCount = 0;
    for (HudUiRectDirty &dirtyRect : layout.widget2.dirtyRects) {
        dirtyRect.framesRemaining = 0;
    }

    zVidImagePartial naniteImage{};
    new (&g_HudUiMgrNanitePanel) HudUiNanitePanel();
    g_HudUiMgrNanitePanel.items[0].image = &naniteImage;
    g_HudUiMgrNanitePanel.items[0].bltSource = nullptr;
    g_HudUiMgrNanitePanel.items[0].flags = 0;
    g_HudUiMgrNanitePanel.items[0].dirtyRectCount = 0;
    for (HudUiRectDirty &dirtyRect : g_HudUiMgrNanitePanel.items[0].dirtyRects) {
        dirtyRect.framesRemaining = 0;
    }
    g_HudUiMgrNanitePanel.items[1].flags = 0x10;
    g_HudUiMgrNanitePanel.items[2].flags = 0x10;

    g_zVideo_pfnBltSourceToPrimary = CaptureHudElementBlit;
    g_HudUiWidget_ExclusiveDrawImage = nullptr;
    g_elementBlitCount = 0;
    g_elementBlitImage = nullptr;
    g_HudUi_InvalidateMask = 0x80;

    layout.UpdateObjectiveDirtyRect();

    const HudUiRectDirty &slot = layout.widget2.dirtyRects[0];
    const bool dirtyRect =
        layout.widget2.dirtyRectCount == 1 &&
        slot.framesRemaining == 1 &&
        slot.drawX == 49 &&
        slot.drawY == 37 &&
        slot.srcLeft == 39 &&
        slot.srcTop == 17 &&
        slot.srcRight == 70 &&
        slot.srcBottom == 23;
    const bool layoutInvalidated = (layout.widget2.flags & 0x80u) != 0;
    const bool naniteInvalidated =
        (g_HudUiMgrNanitePanel.flags & 0x80u) != 0;
    const bool naniteDrawn =
        g_elementBlitCount == 1 &&
        g_elementBlitImage == &naniteImage;

    g_HudUiMgrObjectiveWidget = oldObjectiveWidget;
    g_HudUiMgrObjectiveWidgetRightX = oldObjectiveRightX;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiWidget_ExclusiveDrawImage = oldExclusiveImage;
    g_zVideo_pfnBltSourceToPrimary = oldBlit;

    return dirtyRect && layoutInvalidated && naniteInvalidated && naniteDrawn
               ? 0
               : 1;
}

extern "C" int zhud_widget_release_image_if_owned_smoke(void) {
    HudUiWidget borrowed{};
    zVidImagePartial borrowedImage{};
    borrowed.image = &borrowedImage;
    borrowed.ownsImage = 0;
    borrowed.ReleaseImageIfOwned();
    const bool borrowedOk = borrowed.image == &borrowedImage && borrowed.ownsImage == 0;

    HudUiWidget owned{};
    owned.image = &zVid_Image::g_zImage_DefaultImage;
    owned.ownsImage = 1;
    owned.ReleaseImageIfOwned();
    const bool ownedOk = owned.image == nullptr && owned.ownsImage == 0;

    HudUiWidget empty{};
    empty.ownsImage = 1;
    empty.ReleaseImageIfOwned();
    const bool emptyOk = empty.image == nullptr && empty.ownsImage == 0;

    return borrowedOk && ownedOk && emptyOk ? 0 : 1;
}

extern "C" int zhud_widget_set_image_borrowed_and_invalidate_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;

    HudUiWidget widget{};
    zVidImagePartial oldImage{};
    widget.image = &oldImage;
    widget.ownsImage = 1;
    widget.flags = 0;

    g_HudUi_InvalidateMask = 0x80;
    zVidImagePartial *const result =
        widget.SetImageBorrowedAndInvalidate(&zVid_Image::g_zImage_DefaultImage);

    const bool setBorrowed =
        result == &zVid_Image::g_zImage_DefaultImage &&
        widget.image == &zVid_Image::g_zImage_DefaultImage &&
        widget.ownsImage == 0 &&
        (widget.flags & 0x80u) != 0;

    g_HudUi_InvalidateMask = oldMask;
    return setBorrowed ? 0 : 1;
}

extern "C" int zhud_widget_destructor_core_smoke(void) {
    HudUiWidget *borrowed = new HudUiWidget();
    zVidImagePartial borrowedImage{};
    borrowed->image = &borrowedImage;
    borrowed->ownsImage = 0;
    borrowed->DestructorCore();
    const bool borrowedOk = borrowed->image == &borrowedImage && borrowed->ownsImage == 0;
    ::operator delete(borrowed);

    HudUiWidget *owned = new HudUiWidget();
    owned->image = &zVid_Image::g_zImage_DefaultImage;
    owned->ownsImage = 1;
    owned->DestructorCore();
    const bool ownedOk = owned->image == nullptr && owned->ownsImage == 0;
    ::operator delete(owned);

    return borrowedOk && ownedOk ? 0 : 1;
}

extern "C" int zhud_fill_bitmap_core_smoke(void) {
    HudUiFillBitmap bitmap{};
    bitmap.Constructor();
    bitmap.image = &zVid_Image::g_zImage_DefaultImage;
    bitmap.ownsImage = 0;
    bitmap.previewImage = &zVid_Image::g_zImage_DefaultImage;
    bitmap.fillImage = &zVid_Image::g_zImage_DefaultImage;
    const void *const bitmapVptr = HudElementVptr(bitmap);

    bitmap.DestructorCore();
    const bool coreDestructed =
        HudElementVptr(bitmap) != bitmapVptr &&
        bitmap.previewImage == &zVid_Image::g_zImage_DefaultImage &&
        bitmap.fillImage == &zVid_Image::g_zImage_DefaultImage;

    HudUiFillBitmap scalar{};
    scalar.Constructor();
    scalar.image = &zVid_Image::g_zImage_DefaultImage;
    scalar.ownsImage = 0;
    scalar.previewImage = &zVid_Image::g_zImage_DefaultImage;
    scalar.fillImage = &zVid_Image::g_zImage_DefaultImage;
    const void *const scalarVptr = HudElementVptr(scalar);
    HudUiElement *const scalarResult = scalar.ScalarDeletingDestructor(0);
    const bool scalarDestructed =
        scalarResult == &scalar &&
        HudElementVptr(scalar) != scalarVptr &&
        scalar.previewImage == &zVid_Image::g_zImage_DefaultImage &&
        scalar.fillImage == &zVid_Image::g_zImage_DefaultImage;

    HudUiFillBitmap thunk{};
    thunk.Constructor();
    thunk.image = &zVid_Image::g_zImage_DefaultImage;
    thunk.ownsImage = 0;
    thunk.previewImage = &zVid_Image::g_zImage_DefaultImage;
    thunk.fillImage = &zVid_Image::g_zImage_DefaultImage;
    const void *const thunkVptr = HudElementVptr(thunk);
    thunk.DestructorCoreThunk();
    const bool thunkDestructed =
        HudElementVptr(thunk) != thunkVptr &&
        thunk.previewImage == &zVid_Image::g_zImage_DefaultImage &&
        thunk.fillImage == &zVid_Image::g_zImage_DefaultImage;

    return coreDestructed && scalarDestructed && thunkDestructed ? 0 : 1;
}

extern "C" int zhud_zrd_widget_ex17c_item_core_smoke(void) {
    zVidImagePartial selectedImage{};
    zVidImagePartial unselectedImage{};
    zVidImagePartial selectedRollover{};
    zVidImagePartial unselectedRollover{};

    HudUiZrdWidgetEx17C_Item item{};
    item.Constructor();
    item.modeOrEnabled = 0;
    item.defaultImage = reinterpret_cast<zVidImagePartial *>(0x3333);
    item.rolloverImage = reinterpret_cast<zVidImagePartial *>(0x4444);
    item.image = reinterpret_cast<zVidImagePartial *>(0x5555);
    item.SetSelected(1);
    const bool disabledPath =
        item.selected == 1 &&
        item.defaultImage == reinterpret_cast<zVidImagePartial *>(0x3333) &&
        item.rolloverImage == reinterpret_cast<zVidImagePartial *>(0x4444) &&
        item.image == reinterpret_cast<zVidImagePartial *>(0x5555);

    item.modeOrEnabled = 1;
    item.selectedImage = &selectedImage;
    item.unselectedImage = &unselectedImage;
    item.selectedRolloverImage = &selectedRollover;
    item.unselectedRolloverImage = &unselectedRollover;
    item.defaultImage = nullptr;
    item.rolloverImage = nullptr;
    item.image = nullptr;

    item.SetSelected(1);
    const bool selectedPath =
        item.selected == 1 &&
        item.defaultImage == &selectedImage &&
        item.rolloverImage == &selectedRollover &&
        item.image == &selectedImage;

    item.SetSelected(0);
    const bool unselectedPath =
        item.selected == 0 &&
        item.defaultImage == &unselectedImage &&
        item.rolloverImage == &unselectedRollover &&
        item.image == &unselectedImage;

    HudUiZrdWidgetEx17C selector{};
    HudUiZrdWidgetEx17C_Item firstOption{};
    HudUiZrdWidgetEx17C_Item secondOption{};
    selector.Constructor();
    firstOption.Constructor();
    secondOption.Constructor();
    firstOption.modeOrEnabled = 0;
    secondOption.modeOrEnabled = 0;
    selector.options[0] = &firstOption;
    selector.options[1] = &secondOption;

    const bool selectorPath =
        selector.SetSelectedIndex(1) == 1 &&
        selector.selectedIndex == 1 &&
        firstOption.selected == 0 &&
        secondOption.selected == 1;

    const bool passed = disabledPath && selectedPath && unselectedPath && selectorPath;

    selector.options[0] = nullptr;
    selector.options[1] = nullptr;
    item.defaultImage = nullptr;
    item.rolloverImage = nullptr;
    item.image = nullptr;
    item.selectedImage = nullptr;
    item.unselectedImage = nullptr;
    item.selectedRolloverImage = nullptr;
    item.unselectedRolloverImage = nullptr;

    return passed ? 0 : 1;
}

extern "C" int zhud_widget_set_image_by_path_owned_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;

    HudUiWidget nullPath{};
    nullPath.image = &zVid_Image::g_zImage_DefaultImage;
    nullPath.ownsImage = 1;
    g_HudUi_InvalidateMask = 0x80;
    zVidImagePartial *const nullResult = nullPath.SetImageByPathOwned(nullptr);
    const bool nullPathOk =
        nullResult == nullptr &&
        nullPath.image == &zVid_Image::g_zImage_DefaultImage &&
        nullPath.ownsImage == 1 &&
        (nullPath.flags & 0x80) == 0;

    g_zVid_BuiltinTexturePackCount = 0;
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_TexturePackLoadState = 0;
    g_zVid_TexturePackCount = 0;
    g_zVid_TexturePacks = nullptr;

    HudUiWidget missingPath{};
    missingPath.image = &zVid_Image::g_zImage_DefaultImage;
    missingPath.ownsImage = 1;
    g_HudUi_InvalidateMask = 0x80;
    zVidImagePartial *const missingResult =
        missingPath.SetImageByPathOwned("__recoil_missing_widget_image__");
    const bool missingPathOk =
        missingResult == nullptr &&
        missingPath.image == nullptr &&
        missingPath.ownsImage == 0 &&
        (missingPath.flags & 0x80) != 0;

    g_HudUi_InvalidateMask = oldMask;
    std::free(g_zVid_TexturePacks);
    g_zVid_TexturePacks = nullptr;
    return nullPathOk && missingPathOk ? 0 : 1;
}

extern "C" int zhud_background_cursor_widget_member_constructor_smoke(void) {
    HudUiBackgroundCursorWidget cursor(nullptr, 7);

    const bool constructed =
        cursor.image == nullptr &&
        cursor.alignFlags == 0 &&
        cursor.capturedImage == nullptr &&
        cursor.captureEnabled == 7 &&
        cursor.captureSourceSelector == 1 &&
        cursor.reservedC8 == 0 &&
        cursor.reservedCC == 0;

    zVidImagePartial image{};
    image.width = 4;
    image.height = 2;
    cursor.image = &image;
    cursor.alignFlags = 1;
    cursor.x = 8;
    cursor.y = 10;
    const bool virtualsReady = cursor.GetCenterX() == 10 && cursor.GetCenterY() == 11;

    return constructed && virtualsReady ? 0 : 1;
}

extern "C" int zhud_background_cursor_widget_rebuild_captured_image_smoke(void) {
    unsigned short surfacePixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    unsigned short capturedPixels[4] = {};
    zVidImagePartial sourceImage{};
    sourceImage.width = 2;
    sourceImage.height = 2;
    zVidImagePartial capturedImage{};
    capturedImage.width = 2;
    capturedImage.height = 2;
    capturedImage.pixelCount = 4;
    capturedImage.pixels = capturedPixels;

    const zVideo_SurfaceStatePartial oldSwSurfaceState = g_zVideo_SwSurfaceState;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = surfacePixels;

    HudUiBackgroundCursorWidget cursor(nullptr, 1);
    cursor.image = &sourceImage;
    cursor.capturedImage = &capturedImage;
    cursor.captureSourceSelector = 0;

    cursor.RebuildCapturedImage(1, 1);

    const bool ok =
        cursor.bltSource == &capturedImage &&
        cursor.clipRect.left == 0 &&
        cursor.clipRect.top == 0 &&
        cursor.clipRect.right == 2 &&
        cursor.clipRect.bottom == 2 &&
        capturedPixels[0] == 6 &&
        capturedPixels[1] == 7 &&
        capturedPixels[2] == 10 &&
        capturedPixels[3] == 11;
    g_zVideo_SwSurfaceState = oldSwSurfaceState;
    cursor.capturedImage = nullptr;

    return ok ? 0 : 1;
}

extern "C" int zhud_background_cursor_widget_set_image_borrowed_refresh_smoke(void) {
    unsigned short surfacePixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    zVidImagePartial sourceImage{};
    sourceImage.width = 2;
    sourceImage.height = 2;

    const zVideo_SurfaceStatePartial oldSwSurfaceState = g_zVideo_SwSurfaceState;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = surfacePixels;

    HudUiBackgroundCursorWidget cursor(nullptr, 1);
    cursor.x = 1;
    cursor.y = 1;
    cursor.image = &sourceImage;
    cursor.captureSourceSelector = 0;

    cursor.SetImageBorrowedAndRefresh();
    unsigned short *const capturedPixels =
        static_cast<unsigned short *>(cursor.capturedImage != nullptr
                                          ? cursor.capturedImage->pixels
                                          : nullptr);
    const bool ok =
        cursor.capturedImage != nullptr &&
        cursor.capturedImage->width == 2 &&
        cursor.capturedImage->height == 2 &&
        (cursor.capturedImage->formatFlagsPacked & 0x20) != 0 &&
        cursor.bltSource == cursor.capturedImage &&
        cursor.clipRect.left == 0 &&
        cursor.clipRect.top == 0 &&
        cursor.clipRect.right == 2 &&
        cursor.clipRect.bottom == 2 &&
        capturedPixels != nullptr &&
        capturedPixels[0] == 6 &&
        capturedPixels[1] == 7 &&
        capturedPixels[2] == 10 &&
        capturedPixels[3] == 11;

    if (cursor.capturedImage != nullptr) {
        zVid_Image::Destroy(cursor.capturedImage);
        cursor.capturedImage = nullptr;
    }
    g_zVideo_SwSurfaceState = oldSwSurfaceState;

    return ok ? 0 : 1;
}

extern "C" int
zhud_background_cursor_widget_set_image_by_path_owned_refresh_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char packPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "hcr", 0, packPath) == 0) {
        return 1;
    }

    const char *const imageName = "cursor_path.tex";
    zVidTexturePackHeader packHeader{};
    packHeader.fileFormat = 1;
    packHeader.recordCount = 1;

    zVidTexturePackRecord record{};
    std::strcpy(record.name, imageName);
    record.fileOffset = sizeof(packHeader) + sizeof(record);
    record.paletteIndex = -1;

    FILE *out = std::fopen(packPath, "wb");
    if (out == nullptr) {
        DeleteFileA(packPath);
        return 2;
    }

    unsigned char imageHeader[0x10] = {};
    imageHeader[0] = 1;
    *reinterpret_cast<short *>(&imageHeader[4]) = 2;
    *reinterpret_cast<short *>(&imageHeader[6]) = 2;
    const unsigned short imagePixels[4] = {0x1001, 0x1002, 0x1003, 0x1004};
    std::fwrite(&packHeader, sizeof(packHeader), 1, out);
    std::fwrite(&record, sizeof(record), 1, out);
    std::fwrite(imageHeader, 1, sizeof(imageHeader), out);
    std::fwrite(imagePixels, sizeof(imagePixels), 1, out);
    std::fclose(out);

    zVidTexturePackEntry entry{};
    std::strcpy(entry.filePath, packPath);
    zVidTexturePackEntry *const oldTexturePacks = g_zVid_TexturePacks;
    const int oldTexturePackCount = g_zVid_TexturePackCount;
    zVidTexturePackEntry *const oldBuiltinPacks = g_zVid_BuiltinTexturePacks;
    const int oldBuiltinPackCount = g_zVid_BuiltinTexturePackCount;
    const int oldTexturePackLoadState = g_zVid_TexturePackLoadState;
    const zVideo_SurfaceStatePartial oldSwSurfaceState = g_zVideo_SwSurfaceState;

    g_zVid_TexturePackLoadState = 1;
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    if (zVid_TexturePackEntry_LoadFromFile(&entry) == nullptr) {
        if (entry.fileHandle != nullptr) {
            std::fclose(entry.fileHandle);
        }
        std::free(entry.records);
        g_zVid_TexturePacks = oldTexturePacks;
        g_zVid_TexturePackCount = oldTexturePackCount;
        g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
        g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
        g_zVid_TexturePackLoadState = oldTexturePackLoadState;
        DeleteFileA(packPath);
        return 3;
    }

    g_zVid_TexturePacks = &entry;
    g_zVid_TexturePackCount = 1;

    unsigned short surfacePixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = surfacePixels;

    HudUiBackgroundCursorWidget cursor(nullptr, 1);
    cursor.x = 1;
    cursor.y = 1;
    cursor.captureSourceSelector = 0;

    cursor.SetImageByPathOwnedAndRefresh(imageName);
    unsigned short *const capturedPixels =
        static_cast<unsigned short *>(cursor.capturedImage != nullptr
                                          ? cursor.capturedImage->pixels
                                          : nullptr);
    const bool loadedAndRefreshed =
        cursor.image != nullptr &&
        cursor.image->width == 2 &&
        cursor.image->height == 2 &&
        cursor.ownsImage == 1 &&
        cursor.capturedImage != nullptr &&
        cursor.bltSource == cursor.capturedImage &&
        cursor.clipRect.left == 0 &&
        cursor.clipRect.top == 0 &&
        cursor.clipRect.right == 2 &&
        cursor.clipRect.bottom == 2 &&
        capturedPixels != nullptr &&
        capturedPixels[0] == 6 &&
        capturedPixels[1] == 7 &&
        capturedPixels[2] == 10 &&
        capturedPixels[3] == 11;

    zVidImagePartial *const capturedBeforeMissing = cursor.capturedImage;
    void *const bltSourceBeforeMissing = cursor.bltSource;
    cursor.SetImageByPathOwnedAndRefresh("__recoil_missing_cursor_path_image__");
    const bool missingPathSkippedRefresh =
        cursor.image == nullptr &&
        cursor.ownsImage == 0 &&
        cursor.capturedImage == capturedBeforeMissing &&
        cursor.bltSource == bltSourceBeforeMissing;

    if (cursor.capturedImage != nullptr) {
        zVid_Image::Destroy(cursor.capturedImage);
        cursor.capturedImage = nullptr;
    }
    cursor.ReleaseImageIfOwned();
    std::fclose(entry.fileHandle);
    std::free(entry.records);
    g_zVid_TexturePacks = oldTexturePacks;
    g_zVid_TexturePackCount = oldTexturePackCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
    g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
    g_zVid_TexturePackLoadState = oldTexturePackLoadState;
    g_zVideo_SwSurfaceState = oldSwSurfaceState;
    DeleteFileA(packPath);

    return loadedAndRefreshed && missingPathSkippedRefresh ? 0 : 1;
}

extern "C" int zhud_background_video_widget_constructor_smoke(void) {
    HudUiBackgroundVideoWidget widget{};
    const bool initialized =
        widget.x == 0 &&
        widget.y == 0 &&
        widget.stream == nullptr &&
        widget.elapsedTimeSec == 0.0f &&
        widget.mediaPath[0] == '\0';

    return initialized ? 0 : 1;
}

extern "C" int zhud_background_video_widget_destructor_smoke(void) {
    HudUiBackgroundVideoWidget *widget = new HudUiBackgroundVideoWidget();
    widget->stream = nullptr;
    widget->Destructor();
    const bool destroyed = widget->stream == nullptr;
    ::operator delete(widget);

    return destroyed ? 0 : 1;
}

extern "C" int zhud_background_constructor_smoke(void) {
    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption{};
    vmodeOption.payloadOrBuffer = 6;
    vmodeOption.name = vmodeName;
    vmodeOption.next = nullptr;

    zOptionEntryPartial *const oldOptionsHead = g_zGame_Options_OptionListHead;
    g_zGame_Options_OptionListHead = &vmodeOption;

    HudUiBackground *const backgroundPtr = new HudUiBackground;
    HudUiBackground &background = *backgroundPtr;

    bool soundsInitialized = true;
    for (int index = 0; index < 10; ++index) {
        soundsInitialized =
            soundsInitialized && background.backgroundSounds[index].sample == nullptr &&
            background.backgroundSounds[index].volume == 1.0f &&
            background.backgroundSounds[index].playHandle == nullptr;
    }

    const bool constructed =
        background.enabled == 0 &&
        background.childHead == nullptr &&
        background.childTail == nullptr &&
        background.inputFocusElement == nullptr &&
        background.captureTransitionMask == 1 &&
        background.cursorWidget.image == nullptr &&
        background.cursorWidget.alignFlags == 0 &&
        background.cursorWidget.captureEnabled == 1 &&
        background.cursorWidget.captureSourceSelector == 1 &&
        background.primaryClipImage == nullptr &&
        background.capturedCompositeImage == nullptr &&
        background.backgroundImageWidgets[0].image == nullptr &&
        background.backgroundImageWidgets[19].image == nullptr &&
        background.backgroundVideoWidgets[0].stream == nullptr &&
        background.backgroundVideoWidgets[9].stream == nullptr &&
        background.fontStyles[0].validMarker == 0 &&
        background.fontStyles[0].fontWeight == 500 &&
        background.fontStyles[19].fontWeight == 500 &&
        background.backgroundTextPanels[0].flashResetValue == 0.349999994f &&
        background.backgroundTextPanels[0].flashDirectionSign == 1 &&
        soundsInitialized &&
        background.uiOriginX == 0 &&
        background.uiOriginY == 60;

    delete backgroundPtr;
    g_zGame_Options_OptionListHead = oldOptionsHead;
    return constructed ? 0 : 1;
}

extern "C" int zhud_background_set_enabled_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldSndPreInitialized = g_zSnd_PreInitialized;

    g_HudUi_InvalidateMask = 0x80;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;
    g_elementUpdateInvalidateCount = 0;

    TestElementUpdateElement first{};
    TestElementUpdateElement second{};
    first.next = &second;
    second.next = nullptr;

    HudUiBackground background{};
    background.childHead = &first;
    background.childTail = &second;
    background.enabled = 0;

    zSndSample sample{};
    background.backgroundSounds[0].sample = &sample;
    background.backgroundSounds[0].volume = 0.5f;
    background.backgroundSounds[1].playHandle =
        reinterpret_cast<zSndPlayHandle *>(0x1234);

    background.SetEnabled(1);
    const bool enabled =
        background.enabled == 1 &&
        background.backgroundSounds[0].playHandle == nullptr &&
        background.backgroundSounds[1].playHandle ==
            reinterpret_cast<zSndPlayHandle *>(0x1234) &&
        g_elementUpdateInvalidateCount == 2 &&
        (first.flags & 0x80) != 0 &&
        (second.flags & 0x80) != 0;

    zSndPlayHandle handle{};
    background.backgroundSounds[0].playHandle = &handle;
    background.backgroundSounds[1].playHandle =
        reinterpret_cast<zSndPlayHandle *>(0x5678);

    background.SetEnabled(0);
    const bool disabled =
        background.enabled == 0 &&
        background.backgroundSounds[0].playHandle == nullptr &&
        background.backgroundSounds[1].playHandle == nullptr &&
        g_elementUpdateInvalidateCount == 2;

    g_HudUi_InvalidateMask = oldMask;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_zSnd_PreInitialized = oldSndPreInitialized;

    return enabled && disabled ? 0 : 1;
}

extern "C" int zhud_text_label_constructor_and_extents_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    zImage_Font *const oldFont1 = g_zImage_FontTable[1];
    g_HudUi_InvalidateMask = 0x80;

    HudUiTextLabel label{};
    HudUiTextLabel *const result = label.ConstructorWithPosAndFlags("Speed", 12, 34, 3);

    const bool constructed =
        result == &label &&
        std::strcmp(label.textBuffer, "Speed") == 0 &&
        label.x == 12 &&
        label.y == 34 &&
        label.fontHandle == 3 &&
        label.centerText == 0 &&
        label.alignMode == 0 &&
        (label.flags & 0x80) != 0;

    HudUiTextLabel assigned{};
    HudUiTextLabel *const assignedResult = assigned.Constructor(&label);
    const bool assignedLabel =
        assignedResult == &assigned &&
        std::strcmp(assigned.textBuffer, "Speed") == 0 &&
        assigned.x == 12 &&
        assigned.y == 34 &&
        assigned.fontHandle == 3 &&
        assigned.centerText == 0 &&
        assigned.alignMode == 0;

    zVidImagePartial image{};
    image.height = 6;

    zImage_Font font{};
    font.image = &image;
    font.glyphRects['A' - 0x21].left = 2;
    font.glyphRects['A' - 0x21].right = 12;
    font.glyphRects['B' - 0x21].left = 4;
    font.glyphRects['B' - 0x21].right = 10;
    g_zImage_FontTable[1] = &font;

    label.fontHandle = 1;
    label.centerText = 1;
    label.centerBoundsLeft = 100;
    label.centerBoundsRight = 150;
    label.y = 20;
    label.bltSource = &label;
    label.flags = 0;
    label.SetTextFmt("AB");

    const bool centered =
        label.x == 117 &&
        label.clipRect.left == 117 &&
        label.clipRect.top == 20 &&
        label.clipRect.right == 133 &&
        label.clipRect.bottom == 26 &&
        (label.flags & 0x80) != 0;

    HudUiTextLabel copied{};
    copied.padding32 = 0x7777;
    copied.CopyConstructor(&label);
    const bool copiedLabel =
        copied.next == nullptr &&
        copied.parent == nullptr &&
        copied.x == label.x &&
        copied.y == label.y &&
        copied.state == label.state &&
        copied.padding32 == 0x7777 &&
        std::strcmp(copied.textBuffer, label.textBuffer) == 0 &&
        copied.fontHandle == label.fontHandle &&
        copied.centerText == label.centerText &&
        copied.centerBoundsLeft == label.centerBoundsLeft &&
        copied.centerBoundsRight == label.centerBoundsRight &&
        copied.alignMode == label.alignMode;

    label.flags = 0;
    std::memset(label.textBuffer, 'X', sizeof(label.textBuffer));
    label.SetTextFmt(nullptr);

    const bool cleared = label.textBuffer[0] == 0 &&
                         label.textBuffer[sizeof(label.textBuffer) - 1] == 0 &&
                         label.flags == 0;

    g_zImage_FontTable[1] = oldFont1;
    g_HudUi_InvalidateMask = oldMask;
    return constructed && assignedLabel && centered && copiedLabel && cleared ? 0 : 1;
}

extern "C" int zhud_panel_constructor_default_smoke(void) {
    HudUiPanel panel{};
    HudUiPanel *const returned = panel.ConstructorDefault("TXT", 3, 28);

    HudUiPanel thunkPanel{};
    HudUiPanel *const thunkReturned = thunkPanel.ConstructorDefaultThunk();

    const bool defaultCtor =
        returned == &panel &&
        std::strcmp(panel.textBuffer, "TXT") == 0 &&
        panel.x == 3 &&
        panel.y == 28 &&
        panel.textPick == nullptr &&
        panel.textColor0 == 0x00ffffff &&
        panel.textColor1 == 0x00ffffff &&
        panel.hFont != nullptr &&
        panel.cachedText[0] == '\0' &&
        panel.textWidthPx == 0 &&
        panel.textHeightPx == 0 &&
        panel.shadowEnabled == 0 &&
        panel.bkMode == TRANSPARENT &&
        panel.textDirty == 1 &&
        panel.GetWrapRect() == &panel.wrapRect &&
        panel.wrapRect.left == 0 &&
        panel.wrapRect.top == 0 &&
        panel.wrapRect.right == 0 &&
        panel.wrapRect.bottom == 0 &&
        panel.textRect.left == 0 &&
        panel.textRect.top == 0 &&
        panel.textRect.bottom == 0 &&
        panel.shadowOffsetY == 0;

    const bool thunkCtor =
        thunkReturned == &thunkPanel &&
        thunkPanel.textBuffer[0] == '\0' &&
        thunkPanel.x == 0 &&
        thunkPanel.y == 0 &&
        thunkPanel.textPick == nullptr &&
        thunkPanel.textColor0 == 0x00ffffff &&
        thunkPanel.textColor1 == 0x00ffffff &&
        thunkPanel.hFont != nullptr &&
        thunkPanel.shadowEnabled == 0 &&
        thunkPanel.bkMode == TRANSPARENT &&
        thunkPanel.textDirty == 1;

    return defaultCtor && thunkCtor ? 0 : 1;
}

extern "C" int zhud_panel_copy_construct_core_smoke(void) {
    HudUiPanel panelDispatchProbe{};
    panelDispatchProbe.ConstructorDefault(
        "",
        0,
        0
    );
    HudUiTextLabel labelDispatchProbe{};
    labelDispatchProbe.ConstructorWithPosAndFlags(
        "",
        0,
        0,
        0
    );
    void *const panelDispatch = ZhudFieldAt<void *>(
        &panelDispatchProbe,
        0
    );
    void *const textLabelDispatch = ZhudFieldAt<void *>(
        &labelDispatchProbe,
        0
    );

    alignas(HudUiPanel) unsigned char sourceStorage[sizeof(HudUiPanel)] = {};
    alignas(HudUiPanel) unsigned char copiedStorage[sizeof(HudUiPanel)] = {};
    HudUiPanel *const source = reinterpret_cast<HudUiPanel *>(sourceStorage);
    HudUiPanel *const copied = reinterpret_cast<HudUiPanel *>(copiedStorage);

    source->ConstructorDefault(
        "SRC",
        4,
        5
    );
    source->textPick = reinterpret_cast<zVidImagePartial *>(0x1111);
    source->textColor0 = 0x00010203;
    source->textColor1 = 0x00040506;
    source->cachedTextLength = 0x77;
    std::strcpy(
        source->cachedText,
        "cached"
    );
    source->textWidthPx = 11;
    source->textHeightPx = 12;
    source->shadowEnabled = 1;
    source->bkMode = 2;
    source->bkColor = 3;
    source->textDirty = 4;
    source->unknown274 = 5;
    source->wordWrapEnabled = 6;
    source->wrapRect.left = 7;
    source->wrapRect.top = 8;
    source->wrapRect.right = 9;
    source->wrapRect.bottom = 10;
    source->textRect.left = 13;
    source->textRect.top = 14;
    source->textRect.right = 15;
    source->textRect.bottom = 16;
    source->alignMode = 2;
    source->shadowOffsetX = -3;
    source->shadowOffsetY = 4;

    HudUiPanel *const copiedResult = copied->CopyConstructCore(source);
    const bool copiedOk =
        copiedResult == copied &&
        ZhudFieldAt<void *>(
            copied,
            0
        ) == panelDispatch &&
        copied->textPick == nullptr &&
        copied->textColor0 == source->textColor0 &&
        copied->textColor1 == source->textColor1 &&
        copied->hFont != nullptr &&
        copied->hFont != source->hFont &&
        copied->cachedTextLength == source->cachedTextLength &&
        std::strcmp(
            copied->cachedText,
            "cached"
        ) == 0 &&
        copied->textWidthPx == source->textWidthPx &&
        copied->textHeightPx == source->textHeightPx &&
        copied->shadowEnabled == source->shadowEnabled &&
        copied->bkMode == source->bkMode &&
        copied->bkColor == source->bkColor &&
        copied->textDirty == source->textDirty &&
        copied->unknown274 == source->unknown274 &&
        copied->wordWrapEnabled == source->wordWrapEnabled &&
        copied->wrapRect.left == source->wrapRect.left &&
        copied->textRect.right == source->textRect.right &&
        copied->alignMode == source->alignMode &&
        copied->shadowOffsetX == source->shadowOffsetX &&
        copied->shadowOffsetY == source->shadowOffsetY;

    alignas(HudUiPanel) unsigned char assignedStorage[sizeof(HudUiPanel)] = {};
    HudUiPanel *const assigned = reinterpret_cast<HudUiPanel *>(assignedStorage);
    ZhudFieldAt<void *>(
        assigned,
        0
    ) = textLabelDispatch;
    assigned->hFont = nullptr;
    HudUiPanel *const assignedResult = assigned->ConstructorCopy(source);
    const bool assignedOk =
        assignedResult == assigned &&
        ZhudFieldAt<void *>(
            assigned,
            0
        ) == textLabelDispatch &&
        assigned->textPick == nullptr &&
        assigned->textColor0 == source->textColor0 &&
        assigned->textColor1 == source->textColor1 &&
        assigned->hFont != nullptr &&
        assigned->hFont != source->hFont &&
        assigned->cachedTextLength == source->cachedTextLength &&
        std::strcmp(
            assigned->cachedText,
            "cached"
        ) == 0 &&
        assigned->textWidthPx == source->textWidthPx &&
        assigned->textHeightPx == source->textHeightPx &&
        assigned->shadowEnabled == source->shadowEnabled &&
        assigned->bkMode == source->bkMode &&
        assigned->bkColor == source->bkColor &&
        assigned->textDirty == 1 &&
        assigned->unknown274 == source->unknown274 &&
        assigned->wordWrapEnabled == source->wordWrapEnabled &&
        assigned->wrapRect.left == source->wrapRect.left &&
        assigned->textRect.right == source->textRect.right &&
        assigned->alignMode == source->alignMode &&
        assigned->shadowOffsetX == source->shadowOffsetX &&
        assigned->shadowOffsetY == source->shadowOffsetY;

    if (assigned->hFont != nullptr) {
        DeleteObject(assigned->hFont);
        assigned->hFont = nullptr;
    }
    if (copied->hFont != nullptr) {
        DeleteObject(copied->hFont);
        copied->hFont = nullptr;
    }
    if (source->hFont != nullptr) {
        DeleteObject(source->hFont);
        source->hFont = nullptr;
    }
    panelDispatchProbe.DestructorThunk();
    return copiedOk && assignedOk ? 0 : 1;
}

extern "C" int zhud_panel_draw_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;

    TestPanelDrawOps panel{};
    zVidImagePartial image{};
    zVidImagePartial baseImage{};

    panel.ConstructorDefault("TXT", 100, 20);
    panel.textPick = nullptr;
    panel.textBuffer[0] = '\0';
    panel.textDirty = 1;
    g_widgetDrawBlitCount = 0;
    g_panelDrawBaseCount = 0;
    g_panelDrawBaseThis = nullptr;
    g_panelRebuildTextRectCount = 0;
    g_panelRebuildTextRectThis = nullptr;
    g_zVideo_pfnBltSourceToPrimary = CaptureWidgetDrawBlit;
    panel.Draw();
    const bool dirtyNullTextPick =
        g_panelDrawBaseCount == 0 &&
        g_widgetDrawBlitCount == 0 &&
        panel.textDirty == 0;

    panel.textPick = &image;
    panel.bltSource = &baseImage;
    panel.textBuffer[0] = '\0';
    panel.textDirty = 0;
    g_widgetDrawBlitCount = 0;
    g_panelDrawBaseCount = 0;
    g_panelDrawBaseThis = nullptr;
    panel.Draw();
    const bool emptyTextDrawBase =
        g_widgetDrawBlitCount == 1 &&
        g_widgetDrawBlitImages[0] == &baseImage &&
        g_widgetDrawBlitX[0] == 100 &&
        g_widgetDrawBlitY[0] == 20;

    std::strcpy(
        panel.textBuffer,
        "TXT"
    );
    panel.bltSource = nullptr;
    panel.textRect.left = 1;
    panel.textRect.top = 2;
    panel.textRect.right = 9;
    panel.textRect.bottom = 10;
    panel.alignMode = 0;
    panel.x = 100;
    panel.y = 20;
    panel.textDirty = 0;
    g_widgetDrawBlitCount = 0;
    g_panelDrawBaseCount = 0;
    g_panelDrawBaseThis = nullptr;
    panel.Draw();
    const bool leftAligned =
        g_widgetDrawBlitCount == 1 &&
        g_widgetDrawBlitImages[0] == &image &&
        g_widgetDrawBlitX[0] == 100 &&
        g_widgetDrawBlitY[0] == 20 &&
        g_widgetDrawBlitFlags[0] == 0 &&
        g_widgetDrawBlitHasRect[0] == 1 &&
        g_widgetDrawBlitRects[0].left == 1 &&
        g_widgetDrawBlitRects[0].right == 9 &&
        panel.x == 100;

    panel.alignMode = 1;
    panel.clipRect.left = 10;
    panel.clipRect.right = 70;
    panel.textWidthPx = 20;
    panel.x = 100;
    g_widgetDrawBlitCount = 0;
    g_panelDrawBaseCount = 0;
    panel.Draw();
    const bool centerAligned =
        g_widgetDrawBlitCount == 1 &&
        g_widgetDrawBlitX[0] == 90 &&
        g_widgetDrawBlitY[0] == 20 &&
        panel.x == 100;

    panel.alignMode = 2;
    panel.x = 100;
    g_widgetDrawBlitCount = 0;
    g_panelDrawBaseCount = 0;
    panel.Draw();
    const bool rightAligned =
        g_widgetDrawBlitCount == 1 &&
        g_widgetDrawBlitX[0] == 80 &&
        g_widgetDrawBlitY[0] == 20 &&
        panel.x == 100;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    panel.textPick = nullptr;

    if (!dirtyNullTextPick) {
        return 10;
    }
    if (!emptyTextDrawBase) {
        return 11;
    }
    if (!leftAligned) {
        return 12;
    }
    if (!centerAligned) {
        return 13;
    }
    if (!rightAligned) {
        return 14;
    }
    return 0;
}

extern "C" int zhud_panel_layout_entry_copy_construct_smoke(void) {
    HudUiPanelLayoutEntry source{};
    InitCreditsPanelEntry(&source, "source row", 14, 24);

    HudUiPanelLayoutEntry copied{};
    HudUiPanelLayoutEntry *const result = copied.CopyConstruct(&source);

    const bool copiedValues =
        result == &copied && CreditsPanelEntryMatches(copied, "source row", 14, 24);

    copied.panel.DestructorThunk();
    source.panel.DestructorThunk();
    return copiedValues ? 0 : 1;
}

extern "C" int zhud_panel_layout_entry_copy_assign_smoke(void) {
    HudUiPanelLayoutEntry source{};
    InitCreditsPanelEntry(&source, "assign row", 16, 26);

    HudUiPanelLayoutEntry copied{};
    HudUiPanelLayoutEntry *const result = copied.CopyAssign(&source);

    const bool copiedValues =
        result == &copied && CreditsPanelEntryMatches(copied, "assign row", 16, 26);

    copied.panel.DestructorThunk();
    source.panel.DestructorThunk();
    return copiedValues ? 0 : 1;
}

extern "C" int zhud_panel_layout_entry_copy_assign_range_smoke(void) {
    HudUiPanelLayoutEntry source[2] = {};
    InitCreditsPanelEntry(&source[0], "range a", 18, 28);
    InitCreditsPanelEntry(&source[1], "range b", 38, 48);

    HudUiPanelLayoutEntry dest[2] = {};
    HudUiPanelLayoutEntry *const result =
        HudUiPanelLayoutEntry::CopyAssignRange(source, source + 2, dest);

    const bool copiedValues =
        result == dest + 2 && CreditsPanelEntryMatches(dest[0], "range a", 18, 28) &&
        CreditsPanelEntryMatches(dest[1], "range b", 38, 48);

    HudUiPanelLayoutEntry::DestroyRange(dest, dest + 2);
    source[1].panel.DestructorThunk();
    source[0].panel.DestructorThunk();
    return copiedValues ? 0 : 1;
}

extern "C" int zhud_panel_layout_entry_destroy_range_smoke(void) {
    HudUiPanelLayoutEntry entries[2] = {};
    InitCreditsPanelEntry(&entries[0], "destroy a", 11, 12);
    InitCreditsPanelEntry(&entries[1], "destroy b", 13, 14);

    CodeFunctionPatch destructorPatch{};
    g_PanelLayoutDestroyCount = 0;
    g_PanelLayoutDestroyPanels[0] = nullptr;
    g_PanelLayoutDestroyPanels[1] = nullptr;

    if (!PatchFunctionJump(
            MethodAddress(&HudUiPanel::DestructorThunk),
            reinterpret_cast<void *>(&CountPanelDestructorThunk),
            destructorPatch
        )) {
        entries[1].panel.DestructorThunk();
        entries[0].panel.DestructorThunk();
        return 1;
    }

    HudUiPanelLayoutEntry::DestroyRange(entries, entries + 2);
    RestoreFunctionPatch(destructorPatch);

    return g_PanelLayoutDestroyCount == 2 &&
                   g_PanelLayoutDestroyPanels[0] == &entries[0].panel &&
                   g_PanelLayoutDestroyPanels[1] == &entries[1].panel
               ? 0
               : 1;
}

namespace {
int g_primitiveSetPosCount = 0;
HudUiPrimitiveBindTarget *g_primitiveSetPosThis = nullptr;
int g_primitiveSetPosX = 0;
int g_primitiveSetPosY = 0;

struct TestPrimitiveBindTarget : HudUiPrimitiveBindTarget {
    void SetPos(int newX, int newY) override {
        ++g_primitiveSetPosCount;
        g_primitiveSetPosThis = this;
        g_primitiveSetPosX = newX;
        g_primitiveSetPosY = newY;
        HudUiElement::SetPos(newX, newY);
    }
};
} // namespace

extern "C" int zhud_primitive_bind_target_set_segment_endpoints_smoke(void) {
    TestPrimitiveBindTarget target{};
    target.endX = -1;
    target.endY = -1;
    g_primitiveSetPosCount = 0;
    g_primitiveSetPosThis = nullptr;
    g_primitiveSetPosX = 0;
    g_primitiveSetPosY = 0;

    target.SetSegmentEndpoints(11, 22, 33, 44);

    return g_primitiveSetPosCount == 1 &&
                   g_primitiveSetPosThis == &target &&
                   g_primitiveSetPosX == 11 &&
                   g_primitiveSetPosY == 22 &&
                   target.x == 11 &&
                   target.y == 22 &&
                   target.endX == 33 &&
                   target.endY == 44
               ? 0
               : 1;
}

extern "C" int zhud_background_bind_primitive_node_to_element_smoke(void) {
    const int oldRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int oldGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int oldRShift = g_zVideo_PixelPack.packedBase;
    const int oldGShift = g_zVideo_PixelPack.sumMinus8;
    const int oldBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;

    g_zVideo_PixelPack.rMaskShifted = 0xf8;
    g_zVideo_PixelPack.gMaskShifted = 0xfc;
    g_zVideo_PixelPack.packedBase = 8;
    g_zVideo_PixelPack.sumMinus8 = 3;
    g_zVideo_PixelPack.bShiftTo8 = 3;

    zReader::Node positionItems[3] = {};
    positionItems[0].value.i32 = 3;
    positionItems[1].type = zReader::ZRDR_NODE_INT;
    positionItems[1].value.i32 = 5;
    positionItems[2].type = zReader::ZRDR_NODE_INT;
    positionItems[2].value.i32 = 7;

    zReader::Node colorItems[4] = {};
    colorItems[0].value.i32 = 4;
    colorItems[1].type = zReader::ZRDR_NODE_INT;
    colorItems[1].value.i32 = 0x20;
    colorItems[2].type = zReader::ZRDR_NODE_INT;
    colorItems[2].value.i32 = 0x60;
    colorItems[3].type = zReader::ZRDR_NODE_INT;
    colorItems[3].value.i32 = 0x40;

    zReader::Node endRelItems[3] = {};
    endRelItems[0].value.i32 = 3;
    endRelItems[1].type = zReader::ZRDR_NODE_INT;
    endRelItems[1].value.i32 = 2;
    endRelItems[2].type = zReader::ZRDR_NODE_INT;
    endRelItems[2].value.i32 = 3;

    zReader::Node primitiveItems[7] = {};
    primitiveItems[0].value.i32 = 7;
    primitiveItems[1].type = zReader::ZRDR_NODE_STRING;
    primitiveItems[1].value.str = const_cast<char *>("POSITION");
    primitiveItems[2].type = zReader::ZRDR_NODE_ARRAY;
    primitiveItems[2].value.nodes = positionItems;
    primitiveItems[3].type = zReader::ZRDR_NODE_STRING;
    primitiveItems[3].value.str = const_cast<char *>("COLOR");
    primitiveItems[4].type = zReader::ZRDR_NODE_ARRAY;
    primitiveItems[4].value.nodes = colorItems;
    primitiveItems[5].type = zReader::ZRDR_NODE_STRING;
    primitiveItems[5].value.str = const_cast<char *>("ENDP_REL");
    primitiveItems[6].type = zReader::ZRDR_NODE_ARRAY;
    primitiveItems[6].value.nodes = endRelItems;

    zReader::Node primitivesItems[3] = {};
    primitivesItems[0].value.i32 = 3;
    primitivesItems[1].type = zReader::ZRDR_NODE_STRING;
    primitivesItems[1].value.str = const_cast<char *>("LINE");
    primitivesItems[2].type = zReader::ZRDR_NODE_ARRAY;
    primitivesItems[2].value.nodes = primitiveItems;

    zReader::Node rootItems[3] = {};
    rootItems[0].value.i32 = 3;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("PRIMITIVES");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = primitivesItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    HudUiBackground background{};
    background.cfgRoot = &root;
    background.uiOriginX = 10;
    background.uiOriginY = 20;
    background.capturedCompositeImage = reinterpret_cast<zVidImagePartial *>(0x1234);

    TestPrimitiveBindTarget target{};
    target.color565 = 0xffff;
    const int result = background.BindPrimitiveNodeToElement(nullptr, &target, "LINE");

    const bool linked =
        result == 0 &&
        background.childHead == &target &&
        background.childTail == &target &&
        target.parent == &background;
    const bool primitive =
        target.x == 15 &&
        target.y == 27 &&
        target.endX == 17 &&
        target.endY == 30 &&
        target.color565 == (zVid_PackColorRGB(0x20, 0x60, 0x40) & 0xffffu) &&
        target.bltSource == reinterpret_cast<zVidImagePartial *>(0x1234) &&
        target.clipRect.left == 15 &&
        target.clipRect.top == 27 &&
        target.clipRect.right == 15 &&
        target.clipRect.bottom == 27 &&
        (target.flags & 0x02) != 0;

    g_zVideo_PixelPack.rMaskShifted = oldRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = oldGMaskShifted;
    g_zVideo_PixelPack.packedBase = oldRShift;
    g_zVideo_PixelPack.sumMinus8 = oldGShift;
    g_zVideo_PixelPack.bShiftTo8 = oldBShiftTo8;
    background.capturedCompositeImage = nullptr;
    return linked && primitive ? 0 : 1;
}

void *g_backgroundUpdateChildElement = nullptr;
void *g_backgroundUpdateFocusElement = nullptr;
int g_backgroundUpdateHitCount = 0;
int g_backgroundUpdateShouldHandleCount = 0;
int g_backgroundUpdateShouldHandleHovered = -1;
int g_backgroundUpdateHoverEnterCount = 0;
int g_backgroundUpdateHoverRepeatCount = 0;
int g_backgroundUpdateHoverExitCount = 0;
int g_backgroundUpdateCaptureEnterCount = 0;
int g_backgroundUpdateCaptureExitCount = 0;
int g_backgroundUpdatePrimaryReleaseCount = 0;
int g_backgroundUpdateSecondaryReleaseCount = 0;
int g_backgroundUpdatePointerStateCount = 0;
int g_backgroundUpdateActivateCount = 0;
int g_backgroundUpdateAfterInputCount = 0;
int g_backgroundUpdateAfterHovered = -1;
int g_backgroundUpdateDrawBaseCount = 0;
int g_backgroundUpdateChildUpdateCount = 0;
int g_backgroundUpdateFocusUpdateCount = 0;
int g_backgroundUpdateSetPosCount = 0;
int g_backgroundUpdateSetPosX = 0;
int g_backgroundUpdateSetPosY = 0;
float g_backgroundUpdateChildDelta = 0.0f;
float g_backgroundUpdateFocusDelta = 0.0f;

struct TestBackgroundUpdateElement : HudUiElement {
    int hitResult;
    int shouldHandleResult;

    int HitTest(
        int x,
        int y
    ) {
        ++g_backgroundUpdateHitCount;
        return x == 123 && y == 456 ? hitResult : 0;
    }

    int ShouldHandleInput(
        HudUiBackground *,
        int hovered
    ) {
        ++g_backgroundUpdateShouldHandleCount;
        g_backgroundUpdateShouldHandleHovered = hovered;
        return shouldHandleResult;
    }

    void DrawBase() {
        ++g_backgroundUpdateDrawBaseCount;
    }

    void SetPos(
        int x,
        int y
    ) {
        ++g_backgroundUpdateSetPosCount;
        g_backgroundUpdateSetPosX = x;
        g_backgroundUpdateSetPosY = y;
        this->x = x;
        this->y = y;
    }

    void Update(
        float deltaSeconds
    ) {
        if (this == g_backgroundUpdateChildElement) {
            ++g_backgroundUpdateChildUpdateCount;
            g_backgroundUpdateChildDelta = deltaSeconds;
        } else if (this == g_backgroundUpdateFocusElement) {
            ++g_backgroundUpdateFocusUpdateCount;
            g_backgroundUpdateFocusDelta = deltaSeconds;
        }
    }

    void OnCapturedPrimaryRelease() {
        ++g_backgroundUpdatePrimaryReleaseCount;
    }

    void OnClearBinding() {
        ++g_backgroundUpdateSecondaryReleaseCount;
    }

    void OnHoverRepeat() {
        ++g_backgroundUpdateHoverRepeatCount;
    }

    void ShowPreview() {
        ++g_backgroundUpdateHoverEnterCount;
    }

    void HidePreview() {
        ++g_backgroundUpdateHoverExitCount;
    }

    void OnBeginCapture() {
        ++g_backgroundUpdateCaptureEnterCount;
    }

    void OnEndCapture() {
        ++g_backgroundUpdateCaptureExitCount;
    }

    void OnPointerButtonState(
        int,
        int
    ) {
        ++g_backgroundUpdatePointerStateCount;
    }

    void OnActivate() {
        ++g_backgroundUpdateActivateCount;
    }

    void AfterInputUpdate(
        HudUiBackground *,
        int hovered
    ) {
        ++g_backgroundUpdateAfterInputCount;
        g_backgroundUpdateAfterHovered = hovered;
    }
};

extern "C" int zhud_background_update_input_focus_smoke(void) {
    TestBackgroundUpdateElement child = {};
    TestBackgroundUpdateElement focus = {};
    child.hitResult = 1;
    child.shouldHandleResult = 1;

    HudUiBackground background = {};
    background.enabled = 1;
    background.childHead = &child;
    background.childTail = &child;
    background.inputFocusElement = &focus;
    background.captureTransitionMask = 4;

    g_zInput_MouseStateSnapshot = {};
    g_zInput_MouseStateSnapshot.cursorClientX = 123;
    g_zInput_MouseStateSnapshot.cursorClientY = 456;
    g_zInput_MouseStateSnapshot.button1Transition = 4;
    g_zInput_MouseStateSnapshot.button2Transition = 0;

    g_backgroundUpdateChildElement = &child;
    g_backgroundUpdateFocusElement = &focus;
    g_backgroundUpdateHitCount = 0;
    g_backgroundUpdateShouldHandleCount = 0;
    g_backgroundUpdateShouldHandleHovered = -1;
    g_backgroundUpdateHoverEnterCount = 0;
    g_backgroundUpdateHoverRepeatCount = 0;
    g_backgroundUpdateHoverExitCount = 0;
    g_backgroundUpdateCaptureEnterCount = 0;
    g_backgroundUpdateCaptureExitCount = 0;
    g_backgroundUpdatePrimaryReleaseCount = 0;
    g_backgroundUpdateSecondaryReleaseCount = 0;
    g_backgroundUpdatePointerStateCount = 0;
    g_backgroundUpdateActivateCount = 0;
    g_backgroundUpdateAfterInputCount = 0;
    g_backgroundUpdateAfterHovered = -1;
    g_backgroundUpdateDrawBaseCount = 0;
    g_backgroundUpdateChildUpdateCount = 0;
    g_backgroundUpdateFocusUpdateCount = 0;
    g_backgroundUpdateSetPosCount = 0;
    g_backgroundUpdateSetPosX = 0;
    g_backgroundUpdateSetPosY = 0;
    g_backgroundUpdateChildDelta = 0.0f;
    g_backgroundUpdateFocusDelta = 0.0f;

    background.Update(0.25f);

    const bool inputDispatched =
        g_backgroundUpdateHitCount == 1 &&
        g_backgroundUpdateShouldHandleCount == 1 &&
        g_backgroundUpdateShouldHandleHovered == 1 &&
        g_backgroundUpdateHoverEnterCount == 1 &&
        g_backgroundUpdateHoverRepeatCount == 0 &&
        g_backgroundUpdateCaptureEnterCount == 1 &&
        g_backgroundUpdatePrimaryReleaseCount == 1 &&
        g_backgroundUpdateActivateCount == 1 &&
        g_backgroundUpdateAfterInputCount == 1 &&
        g_backgroundUpdateAfterHovered == 1 &&
        g_backgroundUpdateSecondaryReleaseCount == 0 &&
        g_backgroundUpdatePointerStateCount == 0 &&
        g_backgroundUpdateHoverExitCount == 0 &&
        g_backgroundUpdateCaptureExitCount == 0 &&
        child.state == 3;

    const bool focusUpdated =
        g_backgroundUpdateDrawBaseCount == 1 &&
        g_backgroundUpdateChildUpdateCount == 1 &&
        g_backgroundUpdateChildDelta == 0.25f &&
        g_backgroundUpdateSetPosCount == 1 &&
        g_backgroundUpdateSetPosX == 123 &&
        g_backgroundUpdateSetPosY == 456 &&
        g_backgroundUpdateFocusUpdateCount == 1 &&
        g_backgroundUpdateFocusDelta == 0.25f &&
        focus.x == 123 &&
        focus.y == 456;

    g_backgroundUpdateChildElement = 0;
    g_backgroundUpdateFocusElement = 0;
    return inputDispatched && focusUpdated ? 0 : 1;
}

extern "C" int zhud_container_child_list_smoke(void) {
    HudUiContainer container{};
    HudUiElement first{};
    HudUiElement second{};
    HudUiElement third{};

    const int firstResult = container.AddChild(&first);
    const int secondResult = container.AddChild(&second);
    const int thirdResult = container.AddChild(&third);
    const bool linked =
        container.enabled == 0 &&
        container.childHead == &first &&
        container.childTail == &third &&
        first.next == &second &&
        second.next == &third &&
        third.next == nullptr &&
        first.parent == &container &&
        second.parent == &container &&
        third.parent == &container;

    HudUiElement *previous = reinterpret_cast<HudUiElement *>(0x1234);
    const bool foundHead =
        container.FindChildWithPrev(&first, &previous) == 1 && previous == nullptr;
    previous = reinterpret_cast<HudUiElement *>(0x1234);
    const bool foundSecond =
        container.FindChildWithPrev(&second, &previous) == 1 && previous == &first;
    const bool foundThirdNoPrevious = container.FindChildWithPrev(&third, nullptr) == 1;
    previous = reinterpret_cast<HudUiElement *>(0x9abc);
    const bool missing =
        container.FindChildWithPrev(reinterpret_cast<HudUiElement *>(0x5678), &previous) == 0 &&
        previous == reinterpret_cast<HudUiElement *>(0x9abc);

    HudUiContainer emptyContainer{};
    previous = reinterpret_cast<HudUiElement *>(0xdef0);
    const bool emptyMissing =
        emptyContainer.FindChildWithPrev(&first, &previous) == 0 &&
        previous == reinterpret_cast<HudUiElement *>(0xdef0);

    first.flags = 0;
    second.flags = 0x10;
    third.flags = 0x20;
    container.SetChildFlags(0x02);
    const bool flagsSet = first.flags == 0x02 && second.flags == 0x12 && third.flags == 0x02;

    const bool removedMiddle =
        container.RemoveChild(&second) == 1 &&
        first.next == &third &&
        second.next == nullptr &&
        second.parent == nullptr &&
        container.childTail == &third;

    const bool removedHead =
        container.RemoveChild(&first) == 1 &&
        container.childHead == &third &&
        first.next == nullptr &&
        first.parent == nullptr;

    const bool removedTail =
        container.RemoveChild(&third) == 1 &&
        container.childHead == nullptr &&
        container.childTail == nullptr &&
        third.next == nullptr &&
        third.parent == nullptr;

    return firstResult == 1 && secondResult == 1 && thirdResult == 1 && linked &&
                   foundHead && foundSecond && foundThirdNoPrevious && missing && emptyMissing &&
                   flagsSet && removedMiddle && removedHead && removedTail
               ? 0
               : 1;
}

extern "C" int zhud_zrd_widget_constructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiZrdWidget));
    std::memset(storage, 0xcc, sizeof(HudUiZrdWidget));

    HudUiZrdWidget *const widget = reinterpret_cast<HudUiZrdWidget *>(storage);
    HudUiZrdWidget *const result = widget->Constructor();

    const bool initialized =
        result == widget &&
        widget->originX == 0 &&
        widget->originY == 0 &&
        widget->modeOrEnabled == 1 &&
        widget->owner == nullptr &&
        widget->defaultImage == nullptr &&
        widget->disabledImage == nullptr &&
        widget->rolloverImage == nullptr &&
        widget->rolloverSound == nullptr &&
        widget->rolloverPlayHandle == nullptr &&
        widget->rolloverSoundScale == 1.0f &&
        widget->activateImage == nullptr &&
        widget->activateSound == nullptr &&
        widget->activatePlayHandle == nullptr &&
        widget->activateSoundScale == 1.0f &&
        widget->labelPanels.begin == nullptr &&
        widget->labelPanels.end == nullptr &&
        widget->labelPanels.capacityEnd == nullptr &&
        widget->rolloverLabelPanels.begin == nullptr &&
        widget->rolloverLabelPanels.end == nullptr &&
        widget->rolloverLabelPanels.capacityEnd == nullptr &&
        widget->activateLabelPanels.begin == nullptr &&
        widget->activateLabelPanels.end == nullptr &&
        widget->activateLabelPanels.capacityEnd == nullptr &&
        widget->disabledLabelPanels.begin == nullptr &&
        widget->disabledLabelPanels.end == nullptr &&
        widget->disabledLabelPanels.capacityEnd == nullptr &&
        (widget->flags & 0x02u) != 0 &&
        (widget->flags & ~0x12u) == 0 &&
        (widget->imageStateWord & 0xffffu) == 1;

    widget->DestructorCore();
    ::operator delete(storage);
    return initialized ? 0 : 1;
}

extern "C" int zhud_cycle_selector_widget_constructor_smoke(void) {
    HudUiCycleSelectorWidget widget{};
    widget.selectedIndex = 1;
    widget.itemCount = 2;
    widget.firstIndex = 3;
    widget.visibleCount = 4;
    widget.fontStyleRef = reinterpret_cast<void *>(0x1111);
    widget.textOffsetX = 5;
    widget.textOffsetY = 6;
    widget.entriesA[0] = reinterpret_cast<HudUiWidget *>(0x2222);
    widget.entriesB[19] = reinterpret_cast<HudUiWidget *>(0x3333);

    HudUiCycleSelectorWidget *const result = widget.Constructor();

    bool entriesClear = true;
    for (std::int32_t i = 0; i < 20; ++i) {
        entriesClear =
            entriesClear && widget.entriesA[i] == nullptr && widget.entriesB[i] == nullptr;
    }

    const bool constructed =
        result == &widget &&
        widget.selectedIndex == 0 && widget.itemCount == 0 && widget.firstIndex == 0 &&
        widget.visibleCount == 20 && widget.fontStyleRef == nullptr && widget.textOffsetX == 0 &&
        widget.textOffsetY == 0 && entriesClear;

    widget.firstIndex = 0;
    widget.itemCount = 3;
    widget.visibleCount = 2;
    widget.selectedIndex = 1;
    widget.AdvanceSelectionAndActivate();
    const bool advanceWrap = widget.selectedIndex == 0;

    widget.firstIndex = 1;
    widget.itemCount = 5;
    widget.visibleCount = 4;
    widget.selectedIndex = 0;
    widget.AdvanceSelectionAndActivate();
    const bool advanceInside = widget.selectedIndex == 1;

    widget.DestructorCore();
    return constructed && advanceWrap && advanceInside ? 0 : 1;
}

extern "C" int zhud_cycle_selector_text_entry_smoke(void) {
    HudUiBackground owner{};
    owner.fontStyles[1].validMarker = 1;
    owner.fontStyles[1].fontName = "Arial";
    owner.fontStyles[1].fontSize = 12;
    owner.fontStyles[1].fontWeight = FW_BOLD;
    owner.fontStyles[1].textColor = 0x00112233;
    owner.fontStyles[1].shadowEnabled = 1;
    owner.fontStyles[1].alignMode = 2;
    owner.fontStyles[1].bkMode = 3;
    owner.fontStyles[1].bkColor = 0x00445566;

    HudUiCycleSelectorWidget widget{};
    widget.Constructor();
    widget.owner = &owner;
    widget.itemCount = 0;
    widget.visibleCount = 1;
    widget.textOffsetX = 11;
    widget.textOffsetY = -2;

    widget.AddTextEntry(
        3,
        "Bravo",
        7,
        9
    );
    widget.ApplyFontStyleForEntry(
        3,
        1
    );

    HudUiTransitionTextPanel *const textEntry =
        reinterpret_cast<HudUiTransitionTextPanel *>(widget.entriesA[3]);
    HudUiElement *const textElement = textEntry;
    HudUiPanel *const textPanel = textEntry;

    const bool added =
        widget.itemCount == 4 &&
        widget.visibleCount == 4 &&
        textEntry != nullptr &&
        std::strcmp(textPanel->cachedText, "Bravo") == 0 &&
        textElement->x == 18 &&
        textElement->y == 7 &&
        (textElement->flags & 0x10u) != 0 &&
        textEntry->flashEnabled == 0 &&
        ZhudFloatNear(
            textEntry->flashResetValue,
            0.349999994f
        ) &&
        textEntry->flashAltColor0 == 0 &&
        textEntry->flashMode == 0 &&
        textEntry->flashDirectionSign == 1 &&
        textPanel->textColor0 == 0x00112233 &&
        textPanel->textColor1 == 0x00112233 &&
        textPanel->textDirty == 1 &&
        textPanel->shadowEnabled == 1 &&
        textPanel->shadowOffsetX == 1 &&
        textPanel->shadowOffsetY == 1 &&
        textPanel->alignMode == 2 &&
        textPanel->bkMode == 3 &&
        textPanel->bkColor == 0x00445566 &&
        owner.childHead == textElement &&
        owner.childTail == textElement &&
        textElement->parent == &owner;

    if (textEntry != nullptr) {
        textEntry->ScalarDeletingDestructor(1);
        widget.entriesA[3] = nullptr;
    }
    owner.childHead = nullptr;
    owner.childTail = nullptr;
    widget.DestructorCore();
    return added ? 0 : 1;
}

extern "C" int zhud_zrd_widget_helpers_smoke(void) {
    struct TestZrdChildWidget : HudUiElement {
        std::uint32_t deleteFlags;

        HudUiElement * ScalarDeletingDestructor(unsigned int flags) {
            deleteFlags = flags;
            return this;
        }
    };

    HudUiPanel *panels[] = {
        reinterpret_cast<HudUiPanel *>(0x1000),
        reinterpret_cast<HudUiPanel *>(0x2000),
        reinterpret_cast<HudUiPanel *>(0x3000),
        reinterpret_cast<HudUiPanel *>(0x4000),
    };
    HudUiPanelPtrVector vector{};
    vector.begin = panels;
    vector.end = panels + 4;
    vector.capacityEnd = panels + 4;

    HudUiPanel **const result = vector.EraseRange(panels + 1, panels + 3);
    const bool erased =
        result == panels + 1 &&
        vector.end == panels + 2 &&
        panels[0] == reinterpret_cast<HudUiPanel *>(0x1000) &&
        panels[1] == reinterpret_cast<HudUiPanel *>(0x4000);
    vector.begin = nullptr;
    vector.end = nullptr;
    vector.capacityEnd = nullptr;

    HudUiPanelPtrVector insertVector{};
    HudUiPanel *insertA = reinterpret_cast<HudUiPanel *>(0x1110);
    HudUiPanel *insertB = reinterpret_cast<HudUiPanel *>(0x2220);
    insertVector.InsertN(nullptr, 1, &insertA);
    insertVector.InsertN(insertVector.begin, 2, &insertB);
    const bool inserted =
        insertVector.begin != nullptr &&
        insertVector.end == insertVector.begin + 3 &&
        insertVector.capacityEnd >= insertVector.end &&
        insertVector.begin[0] == reinterpret_cast<HudUiPanel *>(0x2220) &&
        insertVector.begin[1] == reinterpret_cast<HudUiPanel *>(0x2220) &&
        insertVector.begin[2] == reinterpret_cast<HudUiPanel *>(0x1110);

    TestZrdChildWidget child{};
    child.deleteFlags = 0;
    const bool nullDelete = HudUiZrdWidget::DeleteChildIfPresent(nullptr) == nullptr;
    const bool childDelete =
        HudUiZrdWidget::DeleteChildIfPresent(&child) == nullptr && child.deleteFlags == 1;

    TestZrdChildWidget labelChild{};
    TestZrdChildWidget rolloverChild{};
    TestZrdChildWidget activateChild{};
    HudUiPanel **const labelPanels =
        static_cast<HudUiPanel **>(::operator new(sizeof(HudUiPanel *)));
    HudUiPanel **const rolloverPanels =
        static_cast<HudUiPanel **>(::operator new(sizeof(HudUiPanel *)));
    HudUiPanel **const activatePanels =
        static_cast<HudUiPanel **>(::operator new(sizeof(HudUiPanel *)));
    labelPanels[0] = reinterpret_cast<HudUiPanel *>(&labelChild);
    rolloverPanels[0] = reinterpret_cast<HudUiPanel *>(&rolloverChild);
    activatePanels[0] = reinterpret_cast<HudUiPanel *>(&activateChild);

    void *const widgetStorage = ::operator new(sizeof(HudUiZrdWidget));
    std::memset(widgetStorage, 0, sizeof(HudUiZrdWidget));
    HudUiZrdWidget *const widget = new (widgetStorage) HudUiZrdWidget;
    widget->labelPanels.begin = labelPanels;
    widget->labelPanels.end = labelPanels + 1;
    widget->labelPanels.capacityEnd = labelPanels + 1;
    widget->rolloverLabelPanels.begin = rolloverPanels;
    widget->rolloverLabelPanels.end = rolloverPanels + 1;
    widget->rolloverLabelPanels.capacityEnd = rolloverPanels + 1;
    widget->activateLabelPanels.begin = activatePanels;
    widget->activateLabelPanels.end = activatePanels + 1;
    widget->activateLabelPanels.capacityEnd = activatePanels + 1;

    widget->DestructorCore();
    const bool destructed =
        labelChild.deleteFlags == 1 &&
        rolloverChild.deleteFlags == 1 &&
        activateChild.deleteFlags == 1;
    ::operator delete(widgetStorage);

    void *const scalarStorage = ::operator new(sizeof(HudUiZrdWidget));
    std::memset(scalarStorage, 0, sizeof(HudUiZrdWidget));
    HudUiZrdWidget *const scalarWidget = new (scalarStorage) HudUiZrdWidget;
    HudUiElement *const scalarResult = scalarWidget->ScalarDeletingDestructor(0);
    const bool scalarDestroyed = scalarResult == scalarWidget;
    ::operator delete(scalarStorage);

    HudUiZrdWidget invalidateWidget{};
    void *const labelAStorage = ::operator new(sizeof(HudUiPanel));
    void *const labelBStorage = ::operator new(sizeof(HudUiPanel));
    std::memset(labelAStorage, 0, sizeof(HudUiPanel));
    std::memset(labelBStorage, 0, sizeof(HudUiPanel));
    HudUiPanel *const labelA = new (labelAStorage) HudUiPanel;
    HudUiPanel *const labelB = new (labelBStorage) HudUiPanel;
    HudUiPanel *invalidateLabels[] = {labelA, labelB};
    invalidateWidget.labelPanels.begin = invalidateLabels;
    invalidateWidget.labelPanels.end = invalidateLabels + 2;
    g_HudUi_InvalidateMask = 0x80;
    invalidateWidget.Invalidate();
    const bool invalidated =
        (invalidateWidget.flags & 0x80) != 0 &&
        (labelA->flags & 0x80) != 0 &&
        (labelB->flags & 0x80) != 0;
    g_HudUi_InvalidateMask = 0;
    invalidateWidget.labelPanels.begin = nullptr;
    invalidateWidget.labelPanels.end = nullptr;
    ::operator delete(labelAStorage);
    ::operator delete(labelBStorage);

    HudUiZrdWidget boundsWidget{};
    const bool disabledBounds = boundsWidget.GetBoundsRectOrNull() == nullptr;

    zVidImagePartial boundsImage{};
    boundsImage.width = 11;
    boundsImage.height = 13;
    boundsWidget.modeOrEnabled = 1;
    boundsWidget.x = 3;
    boundsWidget.y = 4;
    boundsWidget.image = &boundsImage;
    HudUiRect *const imageBounds = boundsWidget.GetBoundsRectOrNull();
    const bool imageBoundsOk =
        imageBounds == &boundsWidget.boundsRect &&
        imageBounds->left == 3 &&
        imageBounds->top == 4 &&
        imageBounds->right == 14 &&
        imageBounds->bottom == 17;

    void *const boundsLabelAStorage = ::operator new(sizeof(HudUiPanel));
    void *const boundsLabelBStorage = ::operator new(sizeof(HudUiPanel));
    std::memset(boundsLabelAStorage, 0, sizeof(HudUiPanel));
    std::memset(boundsLabelBStorage, 0, sizeof(HudUiPanel));
    HudUiPanel *const boundsLabelA = new (boundsLabelAStorage) HudUiPanel;
    HudUiPanel *const boundsLabelB = new (boundsLabelBStorage) HudUiPanel;
    boundsLabelA->x = 10;
    boundsLabelA->y = 20;
    boundsLabelA->textWidthPx = 10;
    boundsLabelA->textHeightPx = 5;
    boundsLabelA->unknown274 = 0;
    boundsLabelA->textDirty = 0;
    boundsLabelB->x = 12;
    boundsLabelB->y = 25;
    boundsLabelB->textWidthPx = 30;
    boundsLabelB->textHeightPx = 7;
    boundsLabelB->unknown274 = 0;
    boundsLabelB->textDirty = 0;
    HudUiPanel *boundsLabels[] = {boundsLabelA, boundsLabelB};
    boundsWidget.image = nullptr;
    boundsWidget.boundsRect.left = 0;
    boundsWidget.boundsRect.top = 0;
    boundsWidget.boundsRect.right = 0;
    boundsWidget.boundsRect.bottom = 0;
    boundsWidget.labelPanels.begin = boundsLabels;
    boundsWidget.labelPanels.end = boundsLabels + 2;
    HudUiRect *const labelBounds = boundsWidget.GetBoundsRectOrNull();
    const bool labelBoundsOk =
        labelBounds == &boundsWidget.boundsRect &&
        labelBounds->left == 10 &&
        labelBounds->top == 20 &&
        labelBounds->right == 42 &&
        labelBounds->bottom == 32;

    g_HudUi_InvalidateMask = 0;

    invalidateWidget.labelPanels.begin = nullptr;
    invalidateWidget.labelPanels.end = nullptr;
    boundsWidget.labelPanels.begin = nullptr;
    boundsWidget.labelPanels.end = nullptr;
    boundsWidget.image = nullptr;
    boundsWidget.defaultImage = nullptr;
    boundsWidget.disabledImage = nullptr;
    boundsWidget.rolloverImage = nullptr;
    boundsWidget.activateImage = nullptr;
    ::operator delete(boundsLabelAStorage);
    ::operator delete(boundsLabelBStorage);

    return erased && inserted && nullDelete && childDelete && destructed && scalarDestroyed &&
                   invalidated && disabledBounds && imageBoundsOk && labelBoundsOk
               ? 0
               : 1;
}

extern "C" int zhud_zrd_widget_load_from_zrd_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;
    g_zLoc_GetIdProc = nullptr;

    HudUiBackground owner{};
    owner.uiOriginX = 10;
    owner.uiOriginY = 20;
    owner.fontStyles[2].validMarker = 1;
    owner.fontStyles[2].fontName = "Arial";
    owner.fontStyles[2].fontSize = 10;
    owner.fontStyles[2].textColor = 0x00112233;
    owner.fontStyles[2].bkColor = 0x00040506;
    owner.fontStyles[2].bkMode = 2;
    owner.fontStyles[2].shadowEnabled = 1;
    owner.fontStyles[2].fontWeight = FW_NORMAL;
    owner.fontStyles[2].alignMode = 1;

    zReader::Node positionItems[3] = {};
    positionItems[0].value.i32 = 3;
    positionItems[1].type = zReader::ZRDR_NODE_INT;
    positionItems[1].value.i32 = 5;
    positionItems[2].type = zReader::ZRDR_NODE_INT;
    positionItems[2].value.i32 = 7;

    zReader::Node labelItems[5] = {};
    labelItems[0].value.i32 = 5;
    labelItems[1].type = zReader::ZRDR_NODE_STRING;
    labelItems[1].value.str = const_cast<char *>("HELLO");
    labelItems[2].type = zReader::ZRDR_NODE_INT;
    labelItems[2].value.i32 = 2;
    labelItems[3].type = zReader::ZRDR_NODE_INT;
    labelItems[3].value.i32 = 3;
    labelItems[4].type = zReader::ZRDR_NODE_INT;
    labelItems[4].value.i32 = 2;

    zReader::Node rateItems[2] = {};
    rateItems[0].value.i32 = 2;
    rateItems[1].type = zReader::ZRDR_NODE_FLOAT;
    rateItems[1].value.f32 = 0.5f;

    zReader::Node colorItems[4] = {};
    colorItems[0].value.i32 = 4;
    colorItems[1].type = zReader::ZRDR_NODE_INT;
    colorItems[1].value.i32 = 0x11;
    colorItems[2].type = zReader::ZRDR_NODE_INT;
    colorItems[2].value.i32 = 0x22;
    colorItems[3].type = zReader::ZRDR_NODE_INT;
    colorItems[3].value.i32 = 0x33;

    zReader::Node flashItems[5] = {};
    flashItems[0].value.i32 = 5;
    flashItems[1].type = zReader::ZRDR_NODE_STRING;
    flashItems[1].value.str = const_cast<char *>("RATE");
    flashItems[2].type = zReader::ZRDR_NODE_ARRAY;
    flashItems[2].value.nodes = rateItems;
    flashItems[3].type = zReader::ZRDR_NODE_STRING;
    flashItems[3].value.str = const_cast<char *>("COLOR");
    flashItems[4].type = zReader::ZRDR_NODE_ARRAY;
    flashItems[4].value.nodes = colorItems;

    zReader::Node rootItems[7] = {};
    rootItems[0].value.i32 = 7;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("POSITION");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = positionItems;
    rootItems[3].type = zReader::ZRDR_NODE_STRING;
    rootItems[3].value.str = const_cast<char *>("LABEL");
    rootItems[4].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[4].value.nodes = labelItems;
    rootItems[5].type = zReader::ZRDR_NODE_STRING;
    rootItems[5].value.str = const_cast<char *>("FLASH");
    rootItems[6].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[6].value.nodes = flashItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    HudUiZrdWidget widget{};
    widget.Constructor();
    const int result = widget.LoadFromZrd(
        &root,
        &owner
    );

    HudUiTransitionTextPanel *const labelPanel =
        widget.labelPanels.begin != nullptr
            ? reinterpret_cast<HudUiTransitionTextPanel *>(widget.labelPanels.begin[0])
            : nullptr;
    HudUiPanel *const panel = labelPanel;
    HudUiElement *const element = labelPanel;

    unsigned int expectedCountdownBits = 0;
    float expectedCountdown = 0.25f;
    std::memcpy(
        &expectedCountdownBits,
        &expectedCountdown,
        sizeof(expectedCountdownBits)
    );
    unsigned int actualCountdownBits = 0;
    if (labelPanel != nullptr) {
        std::memcpy(
            &actualCountdownBits,
            &labelPanel->flashCountdown,
            sizeof(actualCountdownBits)
        );
    }

    const bool loaded =
        result == 1 &&
        widget.owner == &owner &&
        widget.originX == 15 &&
        widget.originY == 27 &&
        widget.x == 15 &&
        widget.y == 27 &&
        owner.childHead == reinterpret_cast<HudUiElement *>(&widget) &&
        widget.labelPanels.end == widget.labelPanels.begin + 1 &&
        labelPanel != nullptr &&
        std::strcmp(panel->cachedText, "HELLO") == 0 &&
        element->x == 17 &&
        element->y == 30 &&
        panel->alignMode == 1 &&
        panel->textColor0 == 0x00112233 &&
        panel->bkMode == 2 &&
        panel->bkColor == 0x00040506 &&
        labelPanel->flashEnabled == 1 &&
        labelPanel->flashMode == 2 &&
        labelPanel->flashAltColor0 == 0x00332211 &&
        labelPanel->flashAltColor1 == 0x00332211 &&
        actualCountdownBits == expectedCountdownBits;

    if (panel != nullptr) {
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
        ::operator delete(panel);
    }
    ::operator delete(widget.labelPanels.begin);
    widget.labelPanels.begin = nullptr;
    widget.labelPanels.end = nullptr;
    widget.labelPanels.capacityEnd = nullptr;
    owner.childHead = nullptr;
    owner.childTail = nullptr;
    g_HudUi_InvalidateMask = oldMask;
    return loaded ? 0 : 1;
}

extern "C" int zhud_options_dialog_constructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    g_optionsDialogLoadCalls = 0;
    g_optionsDialogLoadArgsOk = false;
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            OptionsDialogLoadProbeAddress(),
            loadPatch
        )) {
        return 1;
    }

    void *const storage = ::operator new(sizeof(HudOptionsDialog));
    HudOptionsDialog *const dialog = new (storage) HudOptionsDialog;
    RestoreFunctionPatch(loadPatch);

    const bool constructed =
        dialog != nullptr &&
        g_optionsDialogLoadCalls == 1 &&
        g_optionsDialogLoadArgsOk &&
        dialog->soundVolumeWidget.normalizedValue == 0.0f &&
        dialog->musicVolumeWidget.normalizedValue == 0.0f &&
        dialog->resolutionSelector.selectedIndex == 0 &&
        dialog->backButton.owner == nullptr;

    dialog->DestructorCore();
    ::operator delete(storage);
    return constructed ? 0 : 1;
}

extern "C" int zhud_options_dialog_destructor_core_smoke(void) {
    int createResult = 0;
    HudOptionsDialog *const dialog = CreateOptionsDialogForSmoke(createResult);
    if (createResult != 0 || dialog == nullptr) {
        return 1;
    }

    dialog->DestructorCore();

    ::operator delete(dialog);
    return 0;
}

extern "C" int zhud_options_dialog_scalar_deleting_destructor_smoke(void) {
    int createResult = 0;
    HudOptionsDialog *const dialog = CreateOptionsDialogForSmoke(createResult);
    if (createResult != 0 || dialog == nullptr) {
        return 1;
    }

    HudUiBackground *const returned = dialog->ScalarDeletingDestructor(0);
    const bool noDeletePath = returned == dialog;
    ::operator delete(dialog);
    if (!noDeletePath) {
        return 2;
    }

    createResult = 0;
    HudOptionsDialog *const deletingDialog = CreateOptionsDialogForSmoke(createResult);
    if (createResult != 0 || deletingDialog == nullptr) {
        return 3;
    }

    deletingDialog->ScalarDeletingDestructor(1);
    return 0;
}

extern "C" int hud_ui_options_panel_overlay_owner_constructor_smoke(void) {
    void *const stateStorage = ::operator new(sizeof(HudUiOptionsPanelOverlayOwner));
    std::memset(
        stateStorage,
        0xcc,
        sizeof(HudUiOptionsPanelOverlayOwner)
    );

    HudUiOptionsPanelOverlayOwner *const state =
        new (stateStorage) HudUiOptionsPanelOverlayOwner;
    const bool constructed = state->m_panel == nullptr;

    state->~HudUiOptionsPanelOverlayOwner();
    ::operator delete(stateStorage);
    return constructed ? 0 : 1;
}

extern "C" int hud_ui_options_panel_overlay_owner_destructor_core_smoke(void) {
    int createResult = 0;
    HudOptionsDialog *const dialog = CreateOptionsDialogForSmoke(createResult);
    if (createResult != 0 || dialog == nullptr) {
        return 1;
    }

    void *const stateStorage = ::operator new(sizeof(HudUiOptionsPanelOverlayOwner));
    HudUiOptionsPanelOverlayOwner *const state =
        new (stateStorage) HudUiOptionsPanelOverlayOwner;
    state->m_panel = dialog;
    state->~HudUiOptionsPanelOverlayOwner();

    const bool cleared = state->m_panel == nullptr;
    ::operator delete(stateStorage);
    return cleared ? 0 : 2;
}

extern "C" int hud_ui_options_panel_overlay_owner_static_init_thunks_smoke(void) {
    HudUiOptionsPanelOverlayOwner *const staticInitReturned =
        HudUiOptionsPanelOverlayOwner::StaticInit();
    if (staticInitReturned != &g_HudUiOptionsPanelOverlayOwner ||
        g_HudUiOptionsPanelOverlayOwner.m_panel != nullptr) {
        return 1;
    }

    int createResult = 0;
    HudOptionsDialog *const dialog = CreateOptionsDialogForSmoke(createResult);
    if (createResult != 0 || dialog == nullptr) {
        return 2;
    }

    g_HudUiOptionsPanelOverlayOwner.m_panel = dialog;
    HudUiOptionsPanelOverlayOwner::AtExitDestructor();
    if (g_HudUiOptionsPanelOverlayOwner.m_panel != nullptr) {
        return 3;
    }

    HudUiOptionsPanelOverlayOwner::RegisterAtExit();
    HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit();
    return g_HudUiOptionsPanelOverlayOwner.m_panel == nullptr ? 0 : 4;
}

extern "C" int hud_ui_options_panel_overlay_owner_queue_enter_smoke(void) {
    const int oldCount = g_RecoilApp.m_stateQueue.m_itemCount;
    HudUiOptionsPanelOverlayOwner::QueueEnter();
    return g_RecoilApp.m_stateQueue.m_itemCount == oldCount + 1 ? 0 : 1;
}

extern "C" int hud_ui_options_panel_overlay_owner_on_try_become_current_smoke(void) {
    CodeFunctionPatch loadPatch{};
    g_optionsDialogLoadCalls = 0;
    g_optionsDialogLoadArgsOk = false;
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            OptionsDialogLoadProbeAddress(),
            loadPatch
        )) {
        return 1;
    }

    void *const stateStorage = ::operator new(sizeof(HudUiOptionsPanelOverlayOwner));
    HudUiOptionsPanelOverlayOwner *const state =
        new (stateStorage) HudUiOptionsPanelOverlayOwner;
    const int accepted = state->OnTryBecomeCurrent();
    HudOptionsDialog *const dialog = state->m_panel;

    const bool becameCurrent =
        accepted == 1 &&
        dialog != nullptr &&
        g_optionsDialogLoadCalls == 1 &&
        g_optionsDialogLoadArgsOk &&
        dialog->enabled == 1;

    state->~HudUiOptionsPanelOverlayOwner();
    ::operator delete(stateStorage);
    RestoreFunctionPatch(loadPatch);
    return becameCurrent ? 0 : 2;
}

extern "C" int zhud_credits_panel_constructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    g_creditsPanelLoadCalls = 0;
    g_creditsPanelLoadArgsOk = false;
    const int oldQuitAfterCredits = g_RecoilApp_QuitAfterCredits;
    g_RecoilApp_QuitAfterCredits = 0;
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CreditsPanelLoadProbeAddress(),
            loadPatch
        )) {
        g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
        return 1;
    }

    void *const storage = ::operator new(sizeof(HudUiCreditsPanel));
    HudUiCreditsPanel *const panel = new (storage) HudUiCreditsPanel;
    RestoreFunctionPatch(loadPatch);

    const bool constructed =
        panel != nullptr &&
        g_creditsPanelLoadCalls == 1 &&
        g_creditsPanelLoadArgsOk &&
        panel->fadeProgress == 0.0f &&
        panel->fadeStep > 0.049f &&
        panel->fadeStep < 0.051f &&
        panel->backButton.owner == nullptr &&
        panel->quitButton.owner == nullptr &&
        panel->creditsScreen.rows.begin == nullptr &&
        panel->creditsScreen.rows.end == nullptr &&
        panel->creditsScreen.rows.cap == nullptr &&
        (panel->creditsScreen.flags & ~0x10u) == 0;

    panel->Destructor();
    ::operator delete(storage);
    g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
    return constructed ? 0 : 1;
}

extern "C" int zhud_credits_panel_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        storage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiCreditsPanel *const panel = static_cast<HudUiCreditsPanel *>(storage);
    InitCreditsPanelForDestructor(panel);
    panel->Destructor();
    const int failure = CreditsPanelDestructorFailureBits(*panel);
    ::operator delete(storage);
    return failure;
}

extern "C" int zhud_credits_panel_scalar_deleting_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        storage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiCreditsPanel *const panel = static_cast<HudUiCreditsPanel *>(storage);
    InitCreditsPanelForDestructor(panel);
    HudUiBackground *const result = panel->HudUiCreditsPanel::ScalarDeletingDestructor(0);

    int failure = result == panel ? 0 : 1;
    failure |= CreditsPanelDestructorFailureBits(*panel) << 1;
    ::operator delete(storage);
    return failure;
}

extern "C" int zhud_scrolling_text_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiZrdScrollingText));
    std::memset(
        storage,
        0,
        sizeof(HudUiZrdScrollingText)
    );
    HudUiZrdScrollingText *const text = static_cast<HudUiZrdScrollingText *>(storage);
    InitScrollingCreditsTextForDestructor(text);
    text->Destructor();
    const int failure = ScrollingCreditsTextDestructorFailureBits(*text);
    ::operator delete(storage);
    return failure;
}

extern "C" int zhud_scrolling_text_scalar_deleting_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiZrdScrollingText));
    std::memset(
        storage,
        0,
        sizeof(HudUiZrdScrollingText)
    );
    HudUiZrdScrollingText *const text = static_cast<HudUiZrdScrollingText *>(storage);
    InitScrollingCreditsTextForDestructor(text);
    HudUiElement *const result = text->ScalarDeletingDestructor(0);

    int failure = result == text ? 0 : 1;
    failure |= ScrollingCreditsTextDestructorFailureBits(*text) << 1;
    ::operator delete(storage);
    return failure;
}

extern "C" int zhud_scrolling_text_load_from_zrd_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    unsigned int (*const oldGetIdProc)(const char *) = g_zLoc_GetIdProc;
    g_HudUi_InvalidateMask = 0x80;
    g_zLoc_GetIdProc = nullptr;

    void *const ownerStorage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        ownerStorage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiBackground *const ownerBackground =
        new (ownerStorage) HudUiBackground;
    HudUiCreditsPanel *const owner =
        reinterpret_cast<HudUiCreditsPanel *>(ownerBackground);
    owner->uiOriginX = 10;
    owner->uiOriginY = 20;
    owner->fadeStep = -1.0f;
    owner->fontStyles[1].validMarker = 1;
    owner->fontStyles[1].fontName = "Arial";
    owner->fontStyles[1].fontSize = 12;
    owner->fontStyles[1].fontWeight = FW_NORMAL;
    owner->fontStyles[1].textColor = 0x00123456;
    owner->fontStyles[1].shadowEnabled = 1;

    zReader::Node positionItems[3] = {};
    positionItems[0].value.i32 = 3;
    positionItems[1].type = zReader::ZRDR_NODE_INT;
    positionItems[1].value.i32 = 5;
    positionItems[2].type = zReader::ZRDR_NODE_INT;
    positionItems[2].value.i32 = 7;

    zReader::Node topLeftItems[3] = {};
    topLeftItems[0].value.i32 = 3;
    topLeftItems[1].type = zReader::ZRDR_NODE_INT;
    topLeftItems[1].value.i32 = 1;
    topLeftItems[2].type = zReader::ZRDR_NODE_INT;
    topLeftItems[2].value.i32 = 2;

    zReader::Node bottomRightItems[3] = {};
    bottomRightItems[0].value.i32 = 3;
    bottomRightItems[1].type = zReader::ZRDR_NODE_INT;
    bottomRightItems[1].value.i32 = 101;
    bottomRightItems[2].type = zReader::ZRDR_NODE_INT;
    bottomRightItems[2].value.i32 = 202;

    zReader::Node rectItems[3] = {};
    rectItems[0].value.i32 = 3;
    rectItems[1].type = zReader::ZRDR_NODE_ARRAY;
    rectItems[1].value.nodes = topLeftItems;
    rectItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rectItems[2].value.nodes = bottomRightItems;

    zReader::Node labelAItems[5] = {};
    labelAItems[0].value.i32 = 5;
    labelAItems[1].type = zReader::ZRDR_NODE_STRING;
    labelAItems[1].value.str = const_cast<char *>("CREDITS_A");
    labelAItems[2].type = zReader::ZRDR_NODE_INT;
    labelAItems[2].value.i32 = 3;
    labelAItems[3].type = zReader::ZRDR_NODE_INT;
    labelAItems[3].value.i32 = 4;
    labelAItems[4].type = zReader::ZRDR_NODE_INT;
    labelAItems[4].value.i32 = 1;

    zReader::Node labelBItems[5] = {};
    labelBItems[0].value.i32 = 5;
    labelBItems[1].type = zReader::ZRDR_NODE_STRING;
    labelBItems[1].value.str = const_cast<char *>("CREDITS_B");
    labelBItems[2].type = zReader::ZRDR_NODE_INT;
    labelBItems[2].value.i32 = 9;
    labelBItems[3].type = zReader::ZRDR_NODE_INT;
    labelBItems[3].value.i32 = 12;
    labelBItems[4].type = zReader::ZRDR_NODE_INT;
    labelBItems[4].value.i32 = 1;

    zReader::Node labelCItems[5] = {};
    labelCItems[0].value.i32 = 5;
    labelCItems[1].type = zReader::ZRDR_NODE_STRING;
    labelCItems[1].value.str = const_cast<char *>("CREDITS_C");
    labelCItems[2].type = zReader::ZRDR_NODE_INT;
    labelCItems[2].value.i32 = 7;
    labelCItems[3].type = zReader::ZRDR_NODE_INT;
    labelCItems[3].value.i32 = 6;
    labelCItems[4].type = zReader::ZRDR_NODE_INT;
    labelCItems[4].value.i32 = 1;

    zReader::Node rowAItems[3] = {};
    rowAItems[0].value.i32 = 3;
    rowAItems[1].type = zReader::ZRDR_NODE_ARRAY;
    rowAItems[1].value.nodes = labelAItems;
    rowAItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rowAItems[2].value.nodes = labelBItems;

    zReader::Node rowBItems[2] = {};
    rowBItems[0].value.i32 = 2;
    rowBItems[1].type = zReader::ZRDR_NODE_ARRAY;
    rowBItems[1].value.nodes = labelCItems;

    zReader::Node scrollingItems[3] = {};
    scrollingItems[0].value.i32 = 3;
    scrollingItems[1].type = zReader::ZRDR_NODE_ARRAY;
    scrollingItems[1].value.nodes = rowAItems;
    scrollingItems[2].type = zReader::ZRDR_NODE_ARRAY;
    scrollingItems[2].value.nodes = rowBItems;

    zReader::Node rootItems[9] = {};
    rootItems[0].value.i32 = 9;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("POSITION");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = positionItems;
    rootItems[3].type = zReader::ZRDR_NODE_STRING;
    rootItems[3].value.str = const_cast<char *>("RECT");
    rootItems[4].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[4].value.nodes = rectItems;
    rootItems[5].type = zReader::ZRDR_NODE_STRING;
    rootItems[5].value.str = const_cast<char *>("SCROLL_RATE");
    rootItems[6].type = zReader::ZRDR_NODE_FLOAT;
    rootItems[6].value.f32 = 0.25f;
    rootItems[7].type = zReader::ZRDR_NODE_STRING;
    rootItems[7].value.str = const_cast<char *>("SCROLLING_TEXT");
    rootItems[8].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[8].value.nodes = scrollingItems;

    zReader::Node root = {};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    void *const textStorage = ::operator new(sizeof(HudUiZrdScrollingText));
    std::memset(
        textStorage,
        0,
        sizeof(HudUiZrdScrollingText)
    );
    HudUiZrdScrollingText *const text = new (textStorage) HudUiZrdScrollingText;
    const int result = text->LoadFromZrd(
        &root,
        owner
    );

    HudUiPanelSpan *const firstRow = text->rows.begin;
    HudUiPanelSpan *const secondRow = text->rows.begin != nullptr
        ? text->rows.begin + 1
        : nullptr;

    const bool loaded =
        result == 1 &&
        text->owner == owner &&
        text->originX == 15 &&
        text->originY == 27 &&
        owner->fadeStep == 0.25f &&
        text->rect.left == 16 &&
        text->rect.top == 29 &&
        text->rect.right == 116 &&
        text->rect.bottom == 229 &&
        text->rows.begin != nullptr &&
        secondRow != nullptr &&
        text->rows.end == text->rows.begin + 2 &&
        firstRow->begin != nullptr &&
        secondRow->begin != nullptr &&
        firstRow->end == firstRow->begin + 2 &&
        secondRow->end == secondRow->begin + 1 &&
        CreditsPanelEntryMatches(
            firstRow->begin[0],
            "CREDITS_A",
            3,
            4
        ) &&
        CreditsPanelEntryMatches(
            firstRow->begin[1],
            "CREDITS_B",
            9,
            12
        ) &&
        secondRow->begin[0].layoutX == 7 &&
        secondRow->begin[0].layoutY > 6 &&
        std::strcmp(
            secondRow->begin[0].panel.textBuffer,
            "CREDITS_C"
        ) == 0 &&
        firstRow->begin[0].panel.textColor0 == 0x00123456 &&
        firstRow->begin[0].panel.textColor1 == 0x00123456 &&
        firstRow->begin[0].panel.shadowEnabled == 1 &&
        text->totalHeight > secondRow->begin[0].layoutY;

    text->Destructor();
    ::operator delete(textStorage);
    ownerBackground->~HudUiBackground();
    ::operator delete(ownerStorage);
    g_zLoc_GetIdProc = oldGetIdProc;
    g_HudUi_InvalidateMask = oldMask;
    return loaded ? 0 : 1;
}

int g_scrollingTextUpdateDispatchCount = 0;
float g_scrollingTextUpdateLastDelta = 0.0f;
void *g_scrollingTextUpdateFirstSelf = nullptr;
void *g_scrollingTextUpdateSecondSelf = nullptr;

struct TestScrollingTextUpdatePanel : HudUiPanel {
    void Update(float deltaSeconds) {
        ++g_scrollingTextUpdateDispatchCount;
        g_scrollingTextUpdateLastDelta = deltaSeconds;
        if (g_scrollingTextUpdateDispatchCount == 1) {
            g_scrollingTextUpdateFirstSelf = this;
        } else if (g_scrollingTextUpdateDispatchCount == 2) {
            g_scrollingTextUpdateSecondSelf = this;
        }
    }
};

extern "C" int zhud_scrolling_text_update_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiZrdScrollingText));
    std::memset(
        storage,
        0,
        sizeof(HudUiZrdScrollingText)
    );
    HudUiZrdScrollingText *const text = new (storage) HudUiZrdScrollingText;
    text->flags = 0x10u;

    HudUiPanelSpan row{};
    HudUiPanelLayoutEntry entries[2]{};
    TestScrollingTextUpdatePanel *const firstPanel =
        new (&entries[0].panel) TestScrollingTextUpdatePanel;
    TestScrollingTextUpdatePanel *const secondPanel =
        new (&entries[1].panel) TestScrollingTextUpdatePanel;
    row.begin = entries;
    row.end = entries + 2;
    row.cap = row.end;
    text->rows.begin = &row;
    text->rows.end = &row + 1;
    text->rows.cap = text->rows.end;

    g_scrollingTextUpdateDispatchCount = 0;
    g_scrollingTextUpdateLastDelta = 0.0f;
    g_scrollingTextUpdateFirstSelf = nullptr;
    g_scrollingTextUpdateSecondSelf = nullptr;

    text->Update(0.25f);

    const bool passed =
        g_scrollingTextUpdateDispatchCount == 2 &&
        ZhudFloatNear(
            g_scrollingTextUpdateLastDelta,
            0.25f
        ) &&
        g_scrollingTextUpdateFirstSelf == firstPanel &&
        g_scrollingTextUpdateSecondSelf == secondPanel;
    ::operator delete(storage);
    return passed ? 0 : 1;
}

extern "C" int zhud_scrolling_text_on_activate_reset_owner_fade_smoke(void) {
    unsigned char ownerStorage[sizeof(HudUiCreditsPanel)] = {};
    HudUiCreditsPanel *const owner = reinterpret_cast<HudUiCreditsPanel *>(ownerStorage);
    owner->fadeProgress = 0.75f;

    HudUiZrdScrollingText text{};
    text.owner = owner;
    text.OnActivateResetOwnerFade();

    return ZhudFloatNear(
               owner->fadeProgress,
               0.0f
           )
               ? 0
               : 1;
}

extern "C" int zhud_scrolling_text_update_scroll_positions_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiZrdScrollingText));
    std::memset(
        storage,
        0,
        sizeof(HudUiZrdScrollingText)
    );
    HudUiZrdScrollingText *const text = new (storage) HudUiZrdScrollingText;
    text->rect.left = 10;
    text->rect.top = 100;
    text->rect.bottom = 200;
    text->totalHeight = 50;

    HudUiPanelSpan row{};
    row.begin = AllocateCreditsPanelEntries(3);
    row.end = row.begin + 3;
    row.cap = row.end;
    InitCreditsPanelEntry(&row.begin[0], "first", 1, -30);
    InitCreditsPanelEntry(&row.begin[1], "second", 2, -10);
    InitCreditsPanelEntry(&row.begin[2], "third", 3, 70);
    ZhudFieldAt<int>(&row.begin[0].panel, 0x260) = 20;
    ZhudFieldAt<int>(&row.begin[1].panel, 0x260) = 20;
    ZhudFieldAt<int>(&row.begin[2].panel, 0x260) = 20;

    text->rows.begin = &row;
    text->rows.end = &row + 1;
    text->rows.cap = text->rows.end;
    text->UpdateScrollPositions(0.5f);

    const bool positions =
        row.begin[0].panel.x == 11 && row.begin[0].panel.y == 95 &&
        row.begin[1].panel.x == 12 && row.begin[1].panel.y == 115 &&
        row.begin[2].panel.x == 13 && row.begin[2].panel.y == 195;
    const bool visibility =
        (row.begin[0].panel.flags & 0x10u) != 0 &&
        (row.begin[1].panel.flags & 0x10u) == 0 &&
        (row.begin[2].panel.flags & 0x10u) != 0;

    row.DestroyAndFree();
    ::operator delete(storage);
    return positions && visibility ? 0 : 1;
}

extern "C" int zhud_credits_panel_update_fade_and_exit_smoke(void) {
    unsigned char oldApp[sizeof(g_RecoilApp)];
    std::memcpy(
        oldApp,
        &g_RecoilApp,
        sizeof(oldApp)
    );
    const int oldQuitAfterCredits = g_RecoilApp_QuitAfterCredits;
    std::memset(
        &g_RecoilApp,
        0,
        sizeof(g_RecoilApp)
    );
    g_RecoilApp.m_currentStateIndex = -1;
    g_RecoilApp_QuitAfterCredits = 0;

    void *const belowStorage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        belowStorage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiCreditsPanel *const belowPanel = static_cast<HudUiCreditsPanel *>(belowStorage);
    InitCreditsPanelForUpdate(
        belowPanel,
        0.25f,
        0.5f
    );
    belowPanel->UpdateFadeAndExit(0.5f);
    const bool belowThreshold =
        ZhudFloatNear(
            belowPanel->fadeProgress,
            0.5f
        ) &&
        g_RecoilApp.m_stateQueue.m_itemCount == 0;
    ::operator delete(belowStorage);

    void *const normalStorage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        normalStorage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiCreditsPanel *const normalPanel = static_cast<HudUiCreditsPanel *>(normalStorage);
    InitCreditsPanelForUpdate(
        normalPanel,
        1.0f,
        0.0f
    );
    normalPanel->UpdateFadeAndExit(0.25f);
    RecoilApp_StateQueueItem *const item = CreditsQueueItemAt(
        g_RecoilApp.m_stateQueue,
        0
    );
    const bool normalExit =
        g_RecoilApp.m_stateQueue.m_itemCount == 1 &&
        item != nullptr &&
        item->m_kind == RecoilApp_StateQueueKind_ExitCurrent &&
        item->m_stateObj == nullptr &&
        item->m_param == 0;
    CleanupCreditsQueue(g_RecoilApp.m_stateQueue);
    ::operator delete(normalStorage);

    g_RecoilApp_QuitAfterCredits = 1;
    CreditsLeaveNetworkState leaveState;
    *reinterpret_cast<void **>(&g_RecoilApp.m_leaveNetworkState) =
        *reinterpret_cast<void **>(&leaveState);
    void *const quitStorage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        quitStorage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiCreditsPanel *const quitPanel = static_cast<HudUiCreditsPanel *>(quitStorage);
    InitCreditsPanelForUpdate(
        quitPanel,
        1.0f,
        0.0f
    );
    quitPanel->UpdateFadeAndExit(0.25f);
    RecoilApp_StateQueueItem *const exitItem = CreditsQueueItemAt(
        g_RecoilApp.m_stateQueue,
        0
    );
    RecoilApp_StateQueueItem *const switchItem = CreditsQueueItemAt(
        g_RecoilApp.m_stateQueue,
        1
    );
    const bool quitExit =
        g_RecoilApp.m_stateQueue.m_itemCount == 2 &&
        exitItem != nullptr &&
        exitItem->m_kind == RecoilApp_StateQueueKind_ExitCurrent &&
        exitItem->m_param == 1 &&
        switchItem != nullptr &&
        switchItem->m_kind == RecoilApp_StateQueueKind_SwitchCurrent &&
        switchItem->m_stateObj == &g_RecoilApp.m_leaveNetworkState &&
        switchItem->m_param == 0 &&
        g_RecoilApp.m_missionShutdownMode == RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
    CleanupCreditsQueue(g_RecoilApp.m_stateQueue);
    ::operator delete(quitStorage);

    std::memcpy(
        &g_RecoilApp,
        oldApp,
        sizeof(g_RecoilApp)
    );
    g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
    return belowThreshold && normalExit && quitExit ? 0 : 1;
}

extern "C" int zhud_cmd_bind_button_base_constructor_smoke(void) {
    void *const itemStorage = ::operator new(sizeof(HudUiListSelectorItem));
    void *const buttonStorage = ::operator new(sizeof(HudCmdBindButtonBase));
    std::memset(itemStorage, 0, sizeof(HudUiListSelectorItem));
    std::memset(buttonStorage, 0, sizeof(HudCmdBindButtonBase));
    HudUiListSelectorItem *const item = new (itemStorage) HudUiListSelectorItem;
    HudCmdBindButtonBase *const button = new (buttonStorage) HudCmdBindButtonBase;

    const bool listItemConstructed =
        item->textBuffer[0] == 0 &&
        item->x == 0 &&
        item->y == 0 &&
        item->textPick == nullptr &&
        item->textColor0 == 0x00ffffff &&
        item->textColor1 == 0x00ffffff &&
        item->textDirty == 1;

    const bool buttonConstructed =
        button->bindingSlotTotalCount == 0 &&
        button->bindingSlotPanels == nullptr &&
        button->bindingVec.begin == nullptr &&
        button->bindingVec.end == nullptr &&
        button->bindingVec.capacity == nullptr &&
        button->bindingSlotSpacing == 0xf &&
        button->selectedBindingIndex == -1 &&
        button->visibleListOffsetX == 0.0f &&
        button->visibleListOffsetY == 0.0f &&
        button->overflowListOffsetX == 0.0f &&
        button->overflowListOffsetY == 0.0f &&
        button->bindPanel.textBuffer[0] == 0 &&
        button->bindPanel.x == 0 &&
        button->bindPanel.y == 0 &&
        button->bindPanel.textPick == nullptr &&
        button->bindPanel.textDirty == 1;

    ::operator delete(buttonStorage);
    ::operator delete(itemStorage);
    return listItemConstructed && buttonConstructed ? 0 : 1;
}

extern "C" int zhud_check_toggle_widget_helpers_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;

    HudUiCheckToggleWidget widget{};
    widget.checked = 7;
    widget.disabledCheckedImage = reinterpret_cast<zVidImagePartial *>(0x1111);
    widget.disabledCheckedFallbackImage = reinterpret_cast<zVidImagePartial *>(0x2222);
    widget.uncheckedImage = reinterpret_cast<zVidImagePartial *>(0x3333);
    widget.checkedImage = reinterpret_cast<zVidImagePartial *>(0x4444);
    widget.checkedLabelPanel = reinterpret_cast<HudUiPanel *>(0x5555);
    widget.Constructor();

    const bool constructed =
        widget.modeOrEnabled == 1 &&
        widget.checked == 0 &&
        widget.disabledCheckedImage == nullptr &&
        widget.disabledCheckedFallbackImage == nullptr &&
        widget.uncheckedImage == nullptr &&
        widget.checkedImage == nullptr &&
        widget.checkedLabelPanel == nullptr;

    zVidImagePartial uncheckedImage{};
    zVidImagePartial checkedImage{};
    HudUiElement checkedLabel{};
    widget.uncheckedImage = &uncheckedImage;
    widget.checkedImage = &checkedImage;
    widget.checkedLabelPanel = reinterpret_cast<HudUiPanel *>(&checkedLabel);
    widget.flags = 0;
    checkedLabel.flags = 0;

    const int previousUnchecked = widget.SetChecked(1);
    const bool checkedState =
        previousUnchecked == 0 &&
        widget.checked == 1 &&
        widget.image == &checkedImage &&
        (checkedLabel.flags & 0x10) == 0 &&
        (widget.flags & 0x80) != 0;

    widget.flags = 0;
    const int previousChecked = widget.SetChecked(0);
    const bool uncheckedState =
        previousChecked == 1 &&
        widget.checked == 0 &&
        widget.image == &uncheckedImage &&
        (checkedLabel.flags & 0x10) != 0 &&
        (widget.flags & 0x80) != 0;

    HudUiElement label{};
    HudUiElement rollover{};
    HudUiElement activate{};
    HudUiElement disabled{};
    HudUiPanel *labels[] = {reinterpret_cast<HudUiPanel *>(&label)};
    HudUiPanel *rollovers[] = {reinterpret_cast<HudUiPanel *>(&rollover)};
    HudUiPanel *activates[] = {reinterpret_cast<HudUiPanel *>(&activate)};
    HudUiPanel *disabledLabels[] = {reinterpret_cast<HudUiPanel *>(&disabled)};
    zVidImagePartial disabledPrimary{};
    zVidImagePartial disabledFallback{};
    widget.labelPanels.begin = labels;
    widget.labelPanels.end = labels + 1;
    widget.rolloverLabelPanels.begin = rollovers;
    widget.rolloverLabelPanels.end = rollovers + 1;
    widget.activateLabelPanels.begin = activates;
    widget.activateLabelPanels.end = activates + 1;
    widget.disabledLabelPanels.begin = disabledLabels;
    widget.disabledLabelPanels.end = disabledLabels + 1;
    widget.disabledCheckedImage = &disabledPrimary;
    widget.disabledCheckedFallbackImage = &disabledFallback;
    widget.checked = 1;
    widget.modeOrEnabled = 1;
    widget.image = nullptr;
    widget.RefreshState();
    const bool refreshEnabled =
        widget.image == &checkedImage &&
        (label.flags & 0x10) == 0 &&
        (rollover.flags & 0x10) != 0 &&
        (activate.flags & 0x10) != 0 &&
        (disabled.flags & 0x10) != 0;

    widget.modeOrEnabled = 0;
    widget.disabledCheckedImage = nullptr;
    widget.RefreshState();
    const bool refreshDisabled =
        widget.image == &disabledFallback &&
        (label.flags & 0x10) != 0 &&
        (disabled.flags & 0x10) == 0;

    HudUiCheckToggleWidget previewWidget{};
    zVidImagePartial previewDefault{};
    zVidImagePartial previewRollover{};
    HudUiElement previewLabel{};
    HudUiElement previewRolloverLabel{};
    HudUiPanel *previewLabels[] = {
        reinterpret_cast<HudUiPanel *>(&previewLabel),
    };
    HudUiPanel *previewRolloverLabels[] = {
        reinterpret_cast<HudUiPanel *>(&previewRolloverLabel),
    };
    previewWidget.modeOrEnabled = 1;
    previewWidget.checked = 0;
    previewWidget.image = &previewDefault;
    previewWidget.rolloverImage = &previewRollover;
    previewWidget.labelPanels.begin = previewLabels;
    previewWidget.labelPanels.end = previewLabels + 1;
    previewWidget.rolloverLabelPanels.begin = previewRolloverLabels;
    previewWidget.rolloverLabelPanels.end = previewRolloverLabels + 1;
    previewWidget.ShowPreview();
    const bool previewShown =
        previewWidget.defaultImage == &previewDefault &&
        previewWidget.image == &previewRollover &&
        (previewLabel.flags & 0x10) != 0 &&
        (previewRolloverLabel.flags & 0x10) == 0;
    previewWidget.labelPanels.begin = nullptr;
    previewWidget.labelPanels.end = nullptr;
    previewWidget.rolloverLabelPanels.begin = nullptr;
    previewWidget.rolloverLabelPanels.end = nullptr;

    HudUiCheckToggleWidget checkedPreviewWidget{};
    checkedPreviewWidget.modeOrEnabled = 1;
    checkedPreviewWidget.checked = 1;
    checkedPreviewWidget.image = &previewDefault;
    checkedPreviewWidget.rolloverImage = &previewRollover;
    checkedPreviewWidget.ShowPreview();
    const bool checkedPreviewSkipped =
        checkedPreviewWidget.defaultImage == nullptr &&
        checkedPreviewWidget.image == &previewDefault;

    HudUiCheckToggleWidget disabledPreviewWidget{};
    disabledPreviewWidget.modeOrEnabled = 0;
    disabledPreviewWidget.checked = 0;
    disabledPreviewWidget.image = &previewDefault;
    disabledPreviewWidget.rolloverImage = &previewRollover;
    disabledPreviewWidget.ShowPreview();
    const bool disabledPreviewSkipped =
        disabledPreviewWidget.defaultImage == nullptr &&
        disabledPreviewWidget.image == &previewDefault;

    HudUiCheckToggleWidget activateToggle{};
    zVidImagePartial activateImage{};
    HudUiElement activateCheckedLabel{};
    activateToggle.modeOrEnabled = 1;
    activateToggle.checked = 0;
    activateToggle.checkedImage = &checkedImage;
    activateToggle.checkedLabelPanel = reinterpret_cast<HudUiPanel *>(&activateCheckedLabel);
    activateToggle.activateImage = &activateImage;
    activateToggle.OnActivate();
    const bool activated =
        activateToggle.checked == 1 &&
        activateToggle.image == &activateImage &&
        (activateCheckedLabel.flags & 0x10) == 0;
    activateToggle.checkedImage = nullptr;
    activateToggle.checkedLabelPanel = nullptr;

    HudUiCheckToggleWidget thunkActivateToggle{};
    thunkActivateToggle.modeOrEnabled = 1;
    thunkActivateToggle.checked = 0;
    thunkActivateToggle.OnActivateThunk();
    const bool thunkActivated = thunkActivateToggle.checked == 1;

    HudUiCheckToggleWidget disabledActivateToggle{};
    disabledActivateToggle.modeOrEnabled = 0;
    disabledActivateToggle.checked = 0;
    disabledActivateToggle.OnActivate();
    const bool disabledActivateSkipped = disabledActivateToggle.checked == 0;

    zSndPlayHandle rolloverHandle{};
    widget.modeOrEnabled = 1;
    widget.checked = 0;
    widget.defaultImage = &zVid_Image::g_zImage_DefaultImage;
    widget.rolloverPlayHandle = &rolloverHandle;
    widget.HidePreview();
    const bool previewHidden =
        widget.rolloverPlayHandle == nullptr &&
        widget.image == &zVid_Image::g_zImage_DefaultImage &&
        (label.flags & 0x10) == 0 &&
        (rollover.flags & 0x10) != 0 &&
        (activate.flags & 0x10) != 0;

    HudUiCheckToggleWidget checkedHideWidget{};
    checkedHideWidget.modeOrEnabled = 1;
    checkedHideWidget.checked = 1;
    checkedHideWidget.rolloverPlayHandle =
        reinterpret_cast<zSndPlayHandle *>(0x11112222);
    checkedHideWidget.HidePreview();
    const bool checkedHideSkipped =
        checkedHideWidget.rolloverPlayHandle ==
        reinterpret_cast<zSndPlayHandle *>(0x11112222);

    HudUiCheckToggleWidget disabledHideWidget{};
    disabledHideWidget.modeOrEnabled = 0;
    disabledHideWidget.checked = 0;
    disabledHideWidget.rolloverPlayHandle =
        reinterpret_cast<zSndPlayHandle *>(0x33334444);
    disabledHideWidget.HidePreview();
    const bool disabledHideSkipped =
        disabledHideWidget.rolloverPlayHandle ==
        reinterpret_cast<zSndPlayHandle *>(0x33334444);

    const bool bounds = widget.GetBoundsRectOrNull() == &widget.boundsRect;
    widget.labelPanels.begin = nullptr;
    widget.labelPanels.end = nullptr;
    widget.labelPanels.capacityEnd = nullptr;
    widget.rolloverLabelPanels.begin = nullptr;
    widget.rolloverLabelPanels.end = nullptr;
    widget.rolloverLabelPanels.capacityEnd = nullptr;
    widget.activateLabelPanels.begin = nullptr;
    widget.activateLabelPanels.end = nullptr;
    widget.activateLabelPanels.capacityEnd = nullptr;
    widget.disabledLabelPanels.begin = nullptr;
    widget.disabledLabelPanels.end = nullptr;
    widget.disabledLabelPanels.capacityEnd = nullptr;
    widget.defaultImage = nullptr;
    widget.disabledImage = nullptr;
    widget.rolloverImage = nullptr;
    widget.activateImage = nullptr;
    widget.disabledCheckedImage = nullptr;
    widget.disabledCheckedFallbackImage = nullptr;
    widget.uncheckedImage = nullptr;
    widget.checkedImage = nullptr;
    widget.checkedLabelPanel = nullptr;

    CheckToggleTestChildWidget labelChild{};
    labelChild.deleteFlags = 0;
    zVidImagePartial *const ownedCheckedImage =
        static_cast<zVidImagePartial *>(::operator new(sizeof(zVidImagePartial)));
    void *const destructStorage = ::operator new(sizeof(HudUiCheckToggleWidget));
    std::memset(destructStorage, 0, sizeof(HudUiCheckToggleWidget));
    HudUiCheckToggleWidget *const destructWidget =
        new (destructStorage) HudUiCheckToggleWidget;
    destructWidget->uncheckedImage = &zVid_Image::g_zImage_DefaultImage;
    destructWidget->checkedImage = ownedCheckedImage;
    destructWidget->checkedLabelPanel = reinterpret_cast<HudUiPanel *>(&labelChild);
    destructWidget->DestructorCore();
    const bool destructed =
        destructWidget->image == &zVid_Image::g_zImage_DefaultImage &&
        destructWidget->checkedImage == nullptr &&
        destructWidget->checkedLabelPanel == nullptr &&
        labelChild.deleteFlags == 1;
    ::operator delete(destructStorage);

    void *const scalarStorage = ::operator new(sizeof(HudUiCheckToggleWidget));
    std::memset(scalarStorage, 0, sizeof(HudUiCheckToggleWidget));
    HudUiCheckToggleWidget *const scalarWidget =
        new (scalarStorage) HudUiCheckToggleWidget;
    HudUiElement *const scalarResult = scalarWidget->ScalarDeletingDestructor(0);
    const bool scalarDeleted = scalarResult == scalarWidget;
    ::operator delete(scalarStorage);

    HudUiCheckToggleWidget *const scalarHeapWidget = new HudUiCheckToggleWidget{};
    HudUiElement *const scalarHeapResult =
        scalarHeapWidget->ScalarDeletingDestructor(1);
    const bool scalarHeapDeleted = scalarHeapResult == scalarHeapWidget;

    CheckToggleTestChildWidget thunkLabelChild{};
    thunkLabelChild.deleteFlags = 0;
    zVidImagePartial *const thunkCheckedImage =
        static_cast<zVidImagePartial *>(::operator new(sizeof(zVidImagePartial)));
    void *const thunkStorage = ::operator new(sizeof(HudUiCheckToggleWidget));
    std::memset(thunkStorage, 0, sizeof(HudUiCheckToggleWidget));
    HudUiCheckToggleWidget *const thunkWidget = new (thunkStorage) HudUiCheckToggleWidget;
    thunkWidget->uncheckedImage = &zVid_Image::g_zImage_DefaultImage;
    thunkWidget->checkedImage = thunkCheckedImage;
    thunkWidget->checkedLabelPanel = reinterpret_cast<HudUiPanel *>(&thunkLabelChild);
    HudUiCheckToggleWidget *const thunkResult =
        thunkWidget->ScalarDeletingDestructorThunk(0);
    const bool thunkDeleted =
        thunkResult == thunkWidget &&
        thunkWidget->image == &zVid_Image::g_zImage_DefaultImage &&
        thunkWidget->checkedImage == nullptr &&
        thunkWidget->checkedLabelPanel == nullptr &&
        thunkLabelChild.deleteFlags == 1;
    ::operator delete(thunkStorage);

    g_HudUi_InvalidateMask = 0;
    int failureCode = 0;
    if (!constructed) {
        failureCode = 3;
    } else if (!checkedState) {
        failureCode = 4;
    } else if (!uncheckedState) {
        failureCode = 5;
    } else if (!refreshEnabled) {
        failureCode = 6;
    } else if (!refreshDisabled) {
        failureCode = 7;
    } else if (!previewShown) {
        failureCode = 8;
    } else if (!checkedPreviewSkipped) {
        failureCode = 9;
    } else if (!disabledPreviewSkipped) {
        failureCode = 10;
    } else if (!activated) {
        failureCode = 11;
    } else if (!thunkActivated) {
        failureCode = 12;
    } else if (!disabledActivateSkipped) {
        failureCode = 13;
    } else if (!previewHidden) {
        failureCode = 14;
    } else if (!checkedHideSkipped) {
        failureCode = 15;
    } else if (!disabledHideSkipped) {
        failureCode = 16;
    } else if (!bounds) {
        failureCode = 17;
    } else if (!destructed) {
        failureCode = 18;
    } else if (!scalarDeleted) {
        failureCode = 19;
    } else if (!scalarHeapDeleted) {
        failureCode = 20;
    } else if (!thunkDeleted) {
        failureCode = 21;
    }

    ExitProcess(static_cast<UINT>(failureCode));
    return failureCode;
}

extern "C" int zhud_util_free_field_ptr_smoke(void) {
    HudUtil field{std::malloc(4)};
    field.FreeFieldPtr();
    if (field.fieldPtr != nullptr) {
        return 1;
    }

    field.FreeFieldPtr();
    return field.fieldPtr == nullptr ? 0 : 2;
}

extern "C" int zhud_cmd_binding_entry_copy_range_smoke(void) {
    HudCmdBindingEntry first;
    HudCmdBindingEntry second;
    HudCmdBindingEntry third;
    first.displayText = nullptr;
    second.displayText = nullptr;
    third.displayText = nullptr;
    first.commandId = 1;
    second.commandId = 2;
    third.commandId = 3;
    HudCmdBindingEntry *source[3] = {&first, &second, &third};
    HudCmdBindingEntry *dest[4] = {};

    dest[0] = &third;
    HudCmdBindingEntry **const emptyResult =
        HudCmdBindingEntry::CopyRange(source, source, dest);
    if (emptyResult != dest || dest[0] != &third) {
        return 1;
    }

    dest[0] = nullptr;
    HudCmdBindingEntry **const result =
        HudCmdBindingEntry::CopyRange(source, source + 3, dest);

    return result == dest + 3 && dest[0] == &first && dest[1] == &second &&
                   dest[2] == &third && dest[3] == nullptr
               ? 0
               : 2;
}

extern "C" int zhud_cmd_binding_destroy_range_smoke(void) {
    HudCmdBindingEntry *const first =
        (HudCmdBindingEntry *)(::operator new(sizeof(HudCmdBindingEntry)));
    HudCmdBindingEntry *const second =
        (HudCmdBindingEntry *)(::operator new(sizeof(HudCmdBindingEntry)));
    first->displayText = (char *)(std::malloc(6));
    first->commandId = 7;
    second->displayText = nullptr;
    second->commandId = 9;
    std::strcpy(first->displayText, "Mouse");

    HudCmdBinding *source[3] = {first, nullptr, second};
    HudCmdBinding *dest[4] = {
        (HudCmdBinding *)1,
        (HudCmdBinding *)2,
        (HudCmdBinding *)3,
        (HudCmdBinding *)4
    };

    HudCmdBinding **const result =
        HudCmdBinding::DestroyRange(source, source + 3, dest, nullptr);
    HudCmdBinding **const emptyResult =
        HudCmdBinding::DestroyRange(source, source, dest + 3, nullptr);

    return result == dest + 3 && emptyResult == dest + 3 && dest[0] == nullptr &&
                   dest[1] == nullptr && dest[2] == nullptr &&
                   dest[3] == (HudCmdBinding *)4
               ? 0
               : 1;
}

extern "C" int zhud_cmd_command_list_destructor_smoke(void) {
    return RunHudCmdDerivedDestructorSmoke<HudCmdCommandList>("Command");
}

extern "C" int zhud_cmd_key_a_button_destructor_smoke(void) {
    return RunHudCmdDerivedDestructorSmoke<HudCmdKeyAButton>("KeyA");
}

extern "C" int zhud_cmd_key_b_button_destructor_smoke(void) {
    return RunHudCmdDerivedDestructorSmoke<HudCmdKeyBButton>("KeyB");
}

extern "C" int zhud_cmd_joy_button_destructor_smoke(void) {
    return RunHudCmdDerivedDestructorSmoke<HudCmdJoyButton>("Joy");
}

extern "C" int zhud_cmd_mouse_button_destructor_smoke(void) {
    return RunHudCmdDerivedDestructorSmoke<HudCmdMouseButton>("Mouse");
}

extern "C" int zhud_cmd_dialog_on_command_selection_changed_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    SetupHudCmdDialogButtons(dialog);

    dialog.OnCommandSelectionChanged(1);

    const bool selected =
        dialog.descriptionPanel.captureState == 0 &&
        dialog.commandList.selectedBindingIndex == 1 &&
        dialog.keyAButton.selectedBindingIndex == 1 &&
        dialog.keyBButton.selectedBindingIndex == 1 &&
        dialog.joyButton.selectedBindingIndex == 1 &&
        dialog.mouseButton.selectedBindingIndex == 1;
    const bool labels =
        std::strcmp(dialog.commandList.bindPanel.textBuffer, "CommandSeven") == 0 &&
        std::strcmp(dialog.keyAButton.bindPanel.textBuffer, "KeyA1") == 0 &&
        std::strcmp(dialog.keyBButton.bindPanel.textBuffer, "KeyB1") == 0 &&
        std::strcmp(dialog.joyButton.bindPanel.textBuffer, "Joy1") == 0 &&
        std::strcmp(dialog.mouseButton.bindPanel.textBuffer, "Mouse1") == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return selected && labels ? 0 : 1;
}

extern "C" int zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    SetupHudCmdDialogButtons(dialog);
    dialog.keyAButton.owner = &dialog;

    dialog.keyAButton.OnSelectionChangedRefresh(1);

    const bool selected =
        dialog.descriptionPanel.captureState == 0 &&
        dialog.commandList.selectedBindingIndex == 1 &&
        dialog.keyAButton.selectedBindingIndex == 1 &&
        dialog.keyBButton.selectedBindingIndex == 1 &&
        dialog.joyButton.selectedBindingIndex == 1 &&
        dialog.mouseButton.selectedBindingIndex == 1 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return selected ? 0 : 1;
}

extern "C" int zhud_cmd_reset_button_on_activate_smoke(void) {
    CodeFunctionPatch defaultsPatch{};
    CodeFunctionPatch lookupPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch activatePatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_InitDefaultBindings),
            reinterpret_cast<void *>(&FakeHudCmdInitDefaultBindings),
            defaultsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_Current_RebuildLookupIndices),
            reinterpret_cast<void *>(&FakeHudCmdRebuildLookupIndices),
            lookupPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiZrdWidget::OnActivate),
            HudCmdZrdActivateProbeAddress(),
            activatePatch
        )) {
        RestoreFunctionPatch(defaultsPatch);
        RestoreFunctionPatch(lookupPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(activatePatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.resetButton.owner = &dialog;
    dialog.setList.selectedIndex = 3;
    g_cmdResetDefaultBindingCalls = 0;
    g_cmdResetLookupRebuildCalls = 0;
    g_cmdResetBaseActivateCalls = 0;
    g_cmdDialogRebuildCalls = 0;
    g_cmdDialogRebuildGroup = -1;

    dialog.resetButton.OnActivate();

    int failureCode = 0;
    if (g_cmdResetDefaultBindingCalls != 1) {
        failureCode = 3;
    } else if (g_cmdResetLookupRebuildCalls != 1) {
        failureCode = 4;
    } else if (g_cmdDialogRebuildCalls != 1) {
        failureCode = 5;
    } else if (g_cmdDialogRebuildGroup != 3) {
        failureCode = 6;
    }

    RestoreFunctionPatch(defaultsPatch);
    RestoreFunctionPatch(lookupPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(activatePatch);
    return failureCode;
}

extern "C" int zhud_cmd_set_list_widget_on_activate_smoke(void) {
    CodeFunctionPatch rebuildPatch{};
    if (!PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        )) {
        RestoreFunctionPatch(rebuildPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.setList.owner = &dialog;
    dialog.setList.selectedIndex = 1;
    dialog.setList.itemCount = 4;
    dialog.setList.firstIndex = 0;
    dialog.setList.visibleCount = 3;
    g_cmdDialogRebuildCalls = 0;
    g_cmdDialogRebuildGroup = -1;

    dialog.setList.OnActivate();

    int failureCode = 0;
    if (dialog.setList.selectedIndex != 2) {
        failureCode = 3;
    } else if (g_cmdDialogRebuildCalls != 1) {
        failureCode = 4;
    } else if (g_cmdDialogRebuildGroup != 2) {
        failureCode = 5;
    }

    dialog.setList.selectedIndex = 2;
    dialog.setList.itemCount = 4;
    dialog.setList.firstIndex = 1;
    dialog.setList.visibleCount = 3;

    dialog.setList.OnActivate();

    if (failureCode == 0 && dialog.setList.selectedIndex != 1) {
        failureCode = 6;
    } else if (failureCode == 0 && g_cmdDialogRebuildCalls != 2) {
        failureCode = 7;
    } else if (failureCode == 0 && g_cmdDialogRebuildGroup != 1) {
        failureCode = 8;
    }

    RestoreFunctionPatch(rebuildPatch);
    return failureCode;
}

extern "C" int zhud_cmd_key_a_button_on_begin_capture_smoke(void) {
    CodeFunctionPatch resetPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::ResetAllTransitionState),
            reinterpret_cast<void *>(&FakeHudCmdRebuildLookupIndices),
            resetPatch
        )) {
        RestoreFunctionPatch(resetPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.captureState = 0;
    dialog.keyAButton.owner = &dialog;
    g_cmdResetLookupRebuildCalls = 0;

    dialog.keyAButton.OnBeginCapture();

    int failureCode = 0;
    if (dialog.descriptionPanel.captureState != 1) {
        failureCode = 3;
    }

    RestoreFunctionPatch(resetPatch);
    return failureCode;
}

extern "C" int zhud_cmd_key_b_button_on_begin_capture_smoke(void) {
    CodeFunctionPatch resetPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::ResetAllTransitionState),
            reinterpret_cast<void *>(&FakeHudCmdRebuildLookupIndices),
            resetPatch
        )) {
        RestoreFunctionPatch(resetPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.captureState = 0;
    dialog.keyBButton.owner = &dialog;
    g_cmdResetLookupRebuildCalls = 0;

    dialog.keyBButton.OnBeginCapture();

    int failureCode = 0;
    if (dialog.descriptionPanel.captureState != 2) {
        failureCode = 3;
    }

    RestoreFunctionPatch(resetPatch);
    return failureCode;
}

extern "C" int zhud_cmd_joy_button_on_begin_capture_smoke(void) {
    CodeFunctionPatch resetPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::ResetAllTransitionState),
            reinterpret_cast<void *>(&FakeHudCmdRebuildLookupIndices),
            resetPatch
        )) {
        RestoreFunctionPatch(resetPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.captureState = 0;
    dialog.joyButton.owner = &dialog;
    g_cmdResetLookupRebuildCalls = 0;

    dialog.joyButton.OnBeginCapture();

    int failureCode = 0;
    if (dialog.descriptionPanel.captureState != 3) {
        failureCode = 3;
    }

    RestoreFunctionPatch(resetPatch);
    return failureCode;
}

extern "C" int zhud_cmd_mouse_button_on_begin_capture_smoke(void) {
    CodeFunctionPatch resetPatch{};
    CodeFunctionPatch activatePatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::ResetAllTransitionState),
            reinterpret_cast<void *>(&FakeHudCmdRebuildLookupIndices),
            resetPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiZrdWidget::OnActivate),
            HudCmdZrdActivateProbeAddress(),
            activatePatch
        )) {
        RestoreFunctionPatch(resetPatch);
        RestoreFunctionPatch(activatePatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.captureState = 77;

    HudCmdMouseButton button{};
    button.owner = &dialog;

    const int oldMouseDebounceFrames = g_HudCmdMouseDebounceFrames;
    g_HudUi_InvalidateMask = 0x80;

    g_HudCmdMouseDebounceFrames = 2;
    g_cmdResetLookupRebuildCalls = 0;
    g_zInput_MouseStateSnapshot.button1Transition = 7;
    g_zInput_MouseStateSnapshot.button2Transition = 8;
    g_zInput_MouseStateSnapshot.button3Transition = 9;
    button.OnBeginCapture();
    const bool debounced =
        dialog.descriptionPanel.captureState == 77 &&
        g_zInput_MouseStateSnapshot.button1Transition == 7 &&
        g_zInput_MouseStateSnapshot.button2Transition == 8 &&
        g_zInput_MouseStateSnapshot.button3Transition == 9;

    g_HudCmdMouseDebounceFrames = 0;
    button.OnBeginCapture();

    int failure = 0;
    if (!debounced) {
        failure = 9;
    } else if (dialog.descriptionPanel.captureState != 4) {
        failure = 10;
    }

    g_HudUi_InvalidateMask = 0;
    g_HudCmdMouseDebounceFrames = oldMouseDebounceFrames;
    RestoreFunctionPatch(resetPatch);
    RestoreFunctionPatch(activatePatch);
    return failure;
}

extern "C" int zhud_cmd_key_a_button_on_clear_binding_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    SetupHudCmdDialogButtons(dialog);
    dialog.keyAButton.owner = &dialog;
    dialog.keyAButton.selectedBindingIndex = 0;

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    std::memset(
        &context,
        0,
        sizeof(context)
    );
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIds.begin = commandIds;
    group.commandIds.end = commandIds + 1;
    group.commandIds.capacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    dialog.keyAButton.OnClearBinding();

    HudCmdBindingEntry **const commandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    HudCmdBindingEntry **const keyABegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyAButton.bindingVec.begin);
    HudCmdBindingEntry **const keyBBegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyBButton.bindingVec.begin);
    const bool cleared =
        dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(5) == 0 &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(5) == 0x30 &&
        dialog.commandList.bindingVec.end == commandBegin + 1 &&
        dialog.keyAButton.bindingVec.end == keyABegin + 1 &&
        dialog.keyBButton.bindingVec.end == keyBBegin + 1 &&
        commandBegin[0]->commandId == 5 &&
        keyABegin[0]->commandId == 5 &&
        keyBBegin[0]->commandId == 5 &&
        keyABegin[0]->displayText[0] == '\0' &&
        std::strcmp(keyBBegin[0]->displayText, "B") == 0 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.keyAButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return cleared ? 0 : 1;
}

extern "C" int zhud_cmd_key_b_button_on_clear_binding_smoke(void) {
    CodeFunctionPatch rebindPatch{};
    if (!PatchFunctionJump(
            MethodAddress(&HudCmdDialog::ApplySecondaryKeyRebind),
            HudCmdDialogApplySecondaryProbeAddress(),
            rebindPatch
        )) {
        RestoreFunctionPatch(rebindPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    HudCmdKeyBButton button{};
    button.owner = &dialog;
    button.selectedBindingIndex = 7;
    g_cmdDialogApplySecondaryProbeCalls = 0;
    g_cmdDialogApplySecondaryProbeThis = 0;
    g_cmdDialogApplySecondaryProbeKeyCode = -1;
    g_cmdDialogApplySecondaryProbeCommandIndex = -1;

    button.OnClearBinding();

    const bool forwarded =
        g_cmdDialogApplySecondaryProbeCalls == 1 &&
        g_cmdDialogApplySecondaryProbeThis == &dialog &&
        g_cmdDialogApplySecondaryProbeKeyCode == 0 &&
        g_cmdDialogApplySecondaryProbeCommandIndex == 7;

    RestoreFunctionPatch(rebindPatch);
    return forwarded ? 0 : 1;
}

extern "C" int zhud_cmd_joy_button_on_clear_binding_smoke(void) {
    CodeFunctionPatch rebindPatch{};
    if (!PatchFunctionJump(
            MethodAddress(&HudCmdDialog::ApplyJoystickButtonRebind),
            HudCmdDialogApplyJoystickProbeAddress(),
            rebindPatch
        )) {
        RestoreFunctionPatch(rebindPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    HudCmdJoyButton button{};
    button.owner = &dialog;
    button.selectedBindingIndex = 7;
    g_cmdDialogApplyJoystickProbeCalls = 0;
    g_cmdDialogApplyJoystickProbeThis = 0;
    g_cmdDialogApplyJoystickProbeButtonCode = -1;
    g_cmdDialogApplyJoystickProbeCommandIndex = -1;

    button.OnClearBinding();

    const bool forwarded =
        g_cmdDialogApplyJoystickProbeCalls == 1 &&
        g_cmdDialogApplyJoystickProbeThis == &dialog &&
        g_cmdDialogApplyJoystickProbeButtonCode == 0 &&
        g_cmdDialogApplyJoystickProbeCommandIndex == 7;

    RestoreFunctionPatch(rebindPatch);
    return forwarded ? 0 : 1;
}

extern "C" int zhud_cmd_mouse_button_on_clear_binding_smoke(void) {
    CodeFunctionPatch rebindPatch{};
    if (!PatchFunctionJump(
            MethodAddress(&HudCmdDialog::ApplyMouseButtonRebind),
            HudCmdDialogApplyMouseProbeAddress(),
            rebindPatch
        )) {
        RestoreFunctionPatch(rebindPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    HudCmdMouseButton button{};
    button.owner = &dialog;
    button.selectedBindingIndex = 7;

    const int oldMouseDebounceFrames = g_HudCmdMouseDebounceFrames;
    g_HudCmdMouseDebounceFrames = 2;
    g_cmdDialogApplyMouseProbeCalls = 0;
    g_cmdDialogApplyMouseProbeThis = 0;
    g_cmdDialogApplyMouseProbeButtonCode = -1;
    g_cmdDialogApplyMouseProbeCommandIndex = -1;

    button.OnClearBinding();
    const bool debounced =
        g_cmdDialogApplyMouseProbeCalls == 0 &&
        g_cmdDialogApplyMouseProbeThis == 0 &&
        g_cmdDialogApplyMouseProbeButtonCode == -1 &&
        g_cmdDialogApplyMouseProbeCommandIndex == -1;

    g_HudCmdMouseDebounceFrames = 0;
    button.OnClearBinding();

    const bool forwarded =
        g_cmdDialogApplyMouseProbeCalls == 1 &&
        g_cmdDialogApplyMouseProbeThis == &dialog &&
        g_cmdDialogApplyMouseProbeButtonCode == 0 &&
        g_cmdDialogApplyMouseProbeCommandIndex == 7;

    g_HudCmdMouseDebounceFrames = oldMouseDebounceFrames;
    RestoreFunctionPatch(rebindPatch);
    return debounced && forwarded ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_rebuild_command_binding_lists_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    std::memset(
        &context,
        0,
        sizeof(context)
    );
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(
        5,
        "CmdFiveCurrent",
        0x1e,
        0x30,
        2,
        1
    );
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIds.begin = commandIds;
    group.commandIds.end = commandIds + 1;
    group.commandIds.capacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    dialog.RebuildCommandBindingListsForGroup(0);

    HudCmdBindingEntry **const commandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    HudCmdBindingEntry **const keyABegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyAButton.bindingVec.begin);
    HudCmdBindingEntry **const keyBBegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyBButton.bindingVec.begin);
    HudCmdBindingEntry **const joyBegin =
        static_cast<HudCmdBindingEntry **>(dialog.joyButton.bindingVec.begin);
    HudCmdBindingEntry **const mouseBegin =
        static_cast<HudCmdBindingEntry **>(dialog.mouseButton.bindingVec.begin);
    const bool rebuilt =
        dialog.commandList.bindingVec.end == commandBegin + 1 &&
        dialog.keyAButton.bindingVec.end == keyABegin + 1 &&
        dialog.keyBButton.bindingVec.end == keyBBegin + 1 &&
        dialog.joyButton.bindingVec.end == joyBegin + 1 &&
        dialog.mouseButton.bindingVec.end == mouseBegin + 1 &&
        commandBegin[0]->commandId == 5 &&
        keyABegin[0]->commandId == 5 &&
        keyBBegin[0]->commandId == 5 &&
        joyBegin[0]->commandId == 5 &&
        mouseBegin[0]->commandId == 5 &&
        std::strcmp(commandBegin[0]->displayText, "CommandFive") == 0 &&
        std::strcmp(keyABegin[0]->displayText, "A") == 0 &&
        std::strcmp(keyBBegin[0]->displayText, "B") == 0 &&
        std::strcmp(joyBegin[0]->displayText, "Button 2") == 0 &&
        std::strcmp(mouseBegin[0]->displayText, "Left") == 0 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.keyAButton.selectedBindingIndex == 0 &&
        dialog.keyBButton.selectedBindingIndex == 0 &&
        dialog.joyButton.selectedBindingIndex == 0 &&
        dialog.mouseButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return rebuilt ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_apply_primary_key_rebind_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    std::memset(
        &context,
        0,
        sizeof(context)
    );
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandThreeLabel[0x50] = {};
    char *labels[16] = {};
    labels[3] = commandThreeLabel;
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(3, "SecondaryHolder", 0, 0x42, 0, 0);
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIds.begin = commandIds;
    group.commandIds.end = commandIds + 1;
    group.commandIds.capacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    const int ignoredResult = dialog.ApplyPrimaryKeyRebind(1, 0);
    HudCmdBindingEntry **const oldCommandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool ignored =
        ignoredResult == 1 && dialog.descriptionPanel.captureState == 0 &&
        dialog.commandList.bindingVec.end == oldCommandBegin + 2 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(5) == 0x1e &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(3) == 0x42;

    dialog.descriptionPanel.captureState = 77;
    const int reboundResult = dialog.ApplyPrimaryKeyRebind(0x42, 0);

    HudCmdBindingEntry **const commandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    HudCmdBindingEntry **const keyABegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyAButton.bindingVec.begin);
    HudCmdBindingEntry **const keyBBegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyBButton.bindingVec.begin);
    const bool rebound =
        reboundResult == 1 && dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(5) == 0x42 &&
        zInput::BindMapCurrent_GetCommandByPrimaryKey(0x42) == 5 &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(3) == 0 &&
        zInput::BindMapCurrent_GetCommandBySecondaryKey(0x42) == 0 &&
        dialog.commandList.bindingVec.end == commandBegin + 1 &&
        dialog.keyAButton.bindingVec.end == keyABegin + 1 &&
        dialog.keyBButton.bindingVec.end == keyBBegin + 1 &&
        commandBegin[0]->commandId == 5 &&
        keyABegin[0]->commandId == 5 &&
        keyBBegin[0]->commandId == 5 &&
        std::strcmp(keyABegin[0]->displayText, "F8") == 0 &&
        keyBBegin[0]->displayText[0] == '\0' &&
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.keyAButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return ignored && rebound ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_apply_secondary_key_rebind_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    std::memset(
        &context,
        0,
        sizeof(context)
    );
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandThreeLabel[0x50] = {};
    char *labels[16] = {};
    labels[3] = commandThreeLabel;
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(3, "PrimaryHolder", 0x42, 0, 0, 0);
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIds.begin = commandIds;
    group.commandIds.end = commandIds + 1;
    group.commandIds.capacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    const int ignoredResult = dialog.ApplySecondaryKeyRebind(1, 0);
    HudCmdBindingEntry **const oldCommandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool ignored =
        ignoredResult == 1 && dialog.descriptionPanel.captureState == 0 &&
        dialog.commandList.bindingVec.end == oldCommandBegin + 2 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(3) == 0x42 &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(5) == 0x30;

    dialog.descriptionPanel.captureState = 77;
    const int reboundResult = dialog.ApplySecondaryKeyRebind(0x42, 0);

    HudCmdBindingEntry **const commandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    HudCmdBindingEntry **const keyABegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyAButton.bindingVec.begin);
    HudCmdBindingEntry **const keyBBegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyBButton.bindingVec.begin);
    const bool rebound =
        reboundResult == 1 && dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(3) == 0 &&
        zInput::BindMapCurrent_GetCommandByPrimaryKey(0x42) == 0 &&
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(5) == 0x42 &&
        zInput::BindMapCurrent_GetCommandBySecondaryKey(0x42) == 5 &&
        dialog.commandList.bindingVec.end == commandBegin + 1 &&
        dialog.keyAButton.bindingVec.end == keyABegin + 1 &&
        dialog.keyBButton.bindingVec.end == keyBBegin + 1 &&
        commandBegin[0]->commandId == 5 &&
        keyABegin[0]->commandId == 5 &&
        keyBBegin[0]->commandId == 5 &&
        std::strcmp(keyABegin[0]->displayText, "A") == 0 &&
        std::strcmp(keyBBegin[0]->displayText, "F8") == 0 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.keyBButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return ignored && rebound ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_apply_joystick_button_rebind_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    std::memset(
        &context,
        0,
        sizeof(context)
    );
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandThreeLabel[0x50] = {};
    char *labels[16] = {};
    labels[3] = commandThreeLabel;
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(3, "JoystickHolder", 0, 0, 4, 0);
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIds.begin = commandIds;
    group.commandIds.end = commandIds + 1;
    group.commandIds.capacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    const int reboundResult = dialog.ApplyJoystickButtonRebind(4, 0);

    HudCmdBindingEntry **const commandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    HudCmdBindingEntry **const joyBegin =
        static_cast<HudCmdBindingEntry **>(dialog.joyButton.bindingVec.begin);
    const bool rebound =
        reboundResult == 1 && dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetJoystickButtonSlot(3) == 0 &&
        zInput::BindMapCurrent_GetCommandByJoystickSlot(4) == 5 &&
        zInput::BindMapCurrent_GetJoystickButtonSlot(5) == 4 &&
        dialog.commandList.bindingVec.end == commandBegin + 1 &&
        dialog.joyButton.bindingVec.end == joyBegin + 1 &&
        commandBegin[0]->commandId == 5 &&
        joyBegin[0]->commandId == 5 &&
        std::strcmp(joyBegin[0]->displayText, "Button 4") == 0 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.joyButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return rebound ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_apply_mouse_button_rebind_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    dialog.setList.selectedIndex = 0;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    std::memset(
        &context,
        0,
        sizeof(context)
    );
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandThreeLabel[0x50] = {};
    char *labels[16] = {};
    labels[3] = commandThreeLabel;
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(3, "MouseHolder", 0, 0, 0, 2);
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 4, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIds.begin = commandIds;
    group.commandIds.end = commandIds + 1;
    group.commandIds.capacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    const int reboundResult = dialog.ApplyMouseButtonRebind(2, 0);

    HudCmdBindingEntry **const commandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    HudCmdBindingEntry **const mouseBegin =
        static_cast<HudCmdBindingEntry **>(dialog.mouseButton.bindingVec.begin);
    const bool rebound =
        reboundResult == 1 && dialog.descriptionPanel.captureState == 0 &&
        zInput::BindMapCurrent_GetMouseButtonSlot(3) == 0 &&
        zInput::BindMapCurrent_GetCommandByMouseSlot(2) == 5 &&
        zInput::BindMapCurrent_GetMouseButtonSlot(5) == 2 &&
        dialog.commandList.bindingVec.end == commandBegin + 1 &&
        dialog.mouseButton.bindingVec.end == mouseBegin + 1 &&
        commandBegin[0]->commandId == 5 &&
        mouseBegin[0]->commandId == 5 &&
        std::strcmp(mouseBegin[0]->displayText, "Right") == 0 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.mouseButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return rebound ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_update_capture_state_idle_smoke(void) {
    HudCmdDialog dialog{};
    dialog.enabled = 0;
    dialog.descriptionPanel.captureState = 0;
    g_HudCmdMouseDebounceFrames = 3;

    dialog.UpdateCaptureState(0.016f);

    const bool idleUpdated =
        g_HudCmdMouseDebounceFrames == 2 &&
        (dialog.promptPanel.flags & 0x10u) != 0;

    g_HudCmdMouseDebounceFrames = 0;
    return idleUpdated ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_select_group_relative_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.setList.selectedIndex = 1;
    dialog.setList.itemCount = 3;
    dialog.setList.firstIndex = 0;
    dialog.setList.visibleCount = 3;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    std::memset(
        &context,
        0,
        sizeof(context)
    );
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandSevenLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    labels[7] = commandSevenLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    context.SetBindingRecord(7, "CmdSevenCurrent", 0x20, 0x31, 3, 3);
    g_zInput_BindMap_Current = &context;

    int groupZeroCommandIds[] = {5};
    int groupOneCommandIds[] = {5};
    int groupTwoCommandIds[] = {7};
    zInput_BindGroupInfo groupsStorage[3] = {};
    groupsStorage[0].commandIds.begin = groupZeroCommandIds;
    groupsStorage[0].commandIds.end = groupZeroCommandIds + 1;
    groupsStorage[0].commandIds.capacity = groupZeroCommandIds + 1;
    groupsStorage[1].commandIds.begin = groupOneCommandIds;
    groupsStorage[1].commandIds.end = groupOneCommandIds + 1;
    groupsStorage[1].commandIds.capacity = groupOneCommandIds + 1;
    groupsStorage[2].commandIds.begin = groupTwoCommandIds;
    groupsStorage[2].commandIds.end = groupTwoCommandIds + 1;
    groupsStorage[2].commandIds.capacity = groupTwoCommandIds + 1;
    zInput_BindGroupInfo *groups[] = {&groupsStorage[0], &groupsStorage[1], &groupsStorage[2]};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 3;
    g_zInput_BindGroupInfoList.capacity = groups + 3;

    const int forwardResult = dialog.SelectGroupRelative(1);
    HudCmdBindingEntry **const forwardBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool forward =
        forwardResult == 2 &&
        dialog.setList.selectedIndex == 2 &&
        dialog.commandList.bindingVec.end == forwardBegin + 1 &&
        forwardBegin[0]->commandId == 7;

    const int wrapForwardResult = dialog.SelectGroupRelative(1);
    HudCmdBindingEntry **const wrapForwardBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool wrapForward =
        wrapForwardResult == 0 &&
        dialog.setList.selectedIndex == 0 &&
        dialog.commandList.bindingVec.end == wrapForwardBegin + 1 &&
        wrapForwardBegin[0]->commandId == 5;

    const int wrapBackwardResult = dialog.SelectGroupRelative(-1);
    HudCmdBindingEntry **const wrapBackwardBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool wrapBackward =
        wrapBackwardResult == 2 &&
        dialog.setList.selectedIndex == 2 &&
        dialog.commandList.bindingVec.end == wrapBackwardBegin + 1 &&
        wrapBackwardBegin[0]->commandId == 7;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return forward && wrapForward && wrapBackward ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_select_command_relative_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    SetupHudCmdDialogButtons(dialog);
    dialog.commandList.selectedBindingIndex = 0;

    const int forwardResult = dialog.SelectCommandRelative(1);
    const bool forward =
        forwardResult == 1 &&
        dialog.commandList.selectedBindingIndex == 1 &&
        dialog.keyAButton.selectedBindingIndex == 1 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    const int outOfRangeForward = dialog.SelectCommandRelative(1);
    const bool clampedForward =
        outOfRangeForward == 1 &&
        dialog.commandList.selectedBindingIndex == 1;

    const int backwardResult = dialog.SelectCommandRelative(-1);
    const bool backward =
        backwardResult == 0 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    const int outOfRangeBackward = dialog.SelectCommandRelative(-1);
    const bool clampedBackward =
        outOfRangeBackward == 0 &&
        dialog.commandList.selectedBindingIndex == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return forward && clampedForward && backward && clampedBackward ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_callback_navigation_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog setDialog{};
    setDialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    setDialog.setList.selectedIndex = 0;
    setDialog.setList.itemCount = 3;
    setDialog.setList.firstIndex = 0;
    setDialog.setList.visibleCount = 3;
    setDialog.nextSetButton.owner = &setDialog;
    setDialog.prevSetButton.owner = &setDialog;
    SetupHudCmdDialogButtons(setDialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    std::memset(
        &context,
        0,
        sizeof(context)
    );
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandSevenLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    labels[7] = commandSevenLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    context.SetBindingRecord(7, "CmdSevenCurrent", 0x20, 0x31, 3, 3);
    g_zInput_BindMap_Current = &context;

    int groupZeroCommandIds[] = {5};
    int groupOneCommandIds[] = {7};
    int groupTwoCommandIds[] = {5};
    zInput_BindGroupInfo groupsStorage[3] = {};
    groupsStorage[0].commandIds.begin = groupZeroCommandIds;
    groupsStorage[0].commandIds.end = groupZeroCommandIds + 1;
    groupsStorage[0].commandIds.capacity = groupZeroCommandIds + 1;
    groupsStorage[1].commandIds.begin = groupOneCommandIds;
    groupsStorage[1].commandIds.end = groupOneCommandIds + 1;
    groupsStorage[1].commandIds.capacity = groupOneCommandIds + 1;
    groupsStorage[2].commandIds.begin = groupTwoCommandIds;
    groupsStorage[2].commandIds.end = groupTwoCommandIds + 1;
    groupsStorage[2].commandIds.capacity = groupTwoCommandIds + 1;
    zInput_BindGroupInfo *groups[] = {&groupsStorage[0], &groupsStorage[1], &groupsStorage[2]};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 3;
    g_zInput_BindGroupInfoList.capacity = groups + 3;

    setDialog.nextSetButton.OnActivate();
    HudCmdBindingEntry **const nextSetBegin =
        static_cast<HudCmdBindingEntry **>(setDialog.commandList.bindingVec.begin);
    const bool nextSet =
        setDialog.setList.selectedIndex == 1 &&
        setDialog.commandList.bindingVec.end == nextSetBegin + 1 &&
        nextSetBegin[0]->commandId == 7;

    setDialog.prevSetButton.OnActivate();
    HudCmdBindingEntry **const prevSetBegin =
        static_cast<HudCmdBindingEntry **>(setDialog.commandList.bindingVec.begin);
    const bool prevSet =
        setDialog.setList.selectedIndex == 0 &&
        setDialog.commandList.bindingVec.end == prevSetBegin + 1 &&
        prevSetBegin[0]->commandId == 5;

    CleanupHudCmdDialogButtons(setDialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(setDialog);

    HudCmdDialog commandDialog{};
    commandDialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    commandDialog.nextCommandButton.owner = &commandDialog;
    commandDialog.prevCommandButton.owner = &commandDialog;
    SetupHudCmdDialogButtons(commandDialog);
    commandDialog.commandList.selectedBindingIndex = 0;

    commandDialog.nextCommandButton.OnActivate();
    const bool nextCommand =
        commandDialog.commandList.selectedBindingIndex == 1 &&
        commandDialog.keyAButton.selectedBindingIndex == 1 &&
        std::strcmp(commandDialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    commandDialog.prevCommandButton.OnActivate();
    const bool prevCommand =
        commandDialog.commandList.selectedBindingIndex == 0 &&
        commandDialog.keyAButton.selectedBindingIndex == 0 &&
        std::strcmp(commandDialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(commandDialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(commandDialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return nextSet && prevSet && nextCommand && prevCommand ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_constructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch setChildFlagsPatch{};
    CodeFunctionPatch groupCountPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CmdDialogLoadProbeAddress(),
            loadPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiContainer::SetChildFlags),
            HudUiContainerSetChildFlagsProbeAddress(),
            setChildFlagsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindGroupList_GetCount),
            reinterpret_cast<void *>(&FakeHudCmdBindGroupCount),
            groupCountPatch
        )) {
        RestoreFunctionPatch(loadPatch);
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(setChildFlagsPatch);
        RestoreFunctionPatch(groupCountPatch);
        return 2;
    }

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    std::memset(
        &context,
        0,
        sizeof(context)
    );
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.title = const_cast<char *>("GroupOne");
    group.commandIds.begin = commandIds;
    group.commandIds.end = commandIds + 1;
    group.commandIds.capacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    g_cmdDialogLoadCalls = 0;
    g_cmdDialogLoadArgsOk = false;
    g_cmdDialogRebuildCalls = 0;
    g_cmdDialogRebuildGroup = -1;
    g_cmdDialogSetChildFlagsCalls = 0;
    g_cmdDialogSetChildFlagsValue = -1;
    void *const storage = ::operator new(sizeof(HudCmdDialog));
    std::memset(storage, 0, sizeof(HudCmdDialog));
    HudCmdDialog *const dialog = static_cast<HudCmdDialog *>(storage);
    HudCmdDialog *const result = dialog->Constructor();

    int failureCode = 0;
    if (result != dialog) {
        failureCode = 3;
    } else if (g_cmdDialogLoadCalls != 1 || !g_cmdDialogLoadArgsOk) {
        failureCode = 4;
    } else if (dialog->descriptionPanel.captureState != 0) {
        failureCode = 5;
    } else if (dialog->setList.itemCount != 0) {
        failureCode = 6;
    } else if (g_cmdDialogRebuildCalls != 1 || g_cmdDialogRebuildGroup != 0) {
        failureCode = 7;
    } else if (g_cmdDialogSetChildFlagsCalls != 1 || g_cmdDialogSetChildFlagsValue != 0) {
        failureCode = 8;
    } else if (dialog->commandList.bindingVec.begin != nullptr ||
               dialog->keyAButton.bindingVec.begin != nullptr ||
               dialog->mouseButton.bindingVec.begin != nullptr) {
        failureCode = 9;
    }

    CleanupHudCmdDialogButtons(*dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(*dialog);
    ::operator delete(storage);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(loadPatch);
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(setChildFlagsPatch);
    RestoreFunctionPatch(groupCountPatch);
    return failureCode;
}

extern "C" int zhud_cmd_dialog_destructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch setChildFlagsPatch{};
    CodeFunctionPatch groupCountPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CmdDialogLoadProbeAddress(),
            loadPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiContainer::SetChildFlags),
            HudUiContainerSetChildFlagsProbeAddress(),
            setChildFlagsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindGroupList_GetCount),
            reinterpret_cast<void *>(&FakeHudCmdBindGroupCount),
            groupCountPatch
        )) {
        RestoreFunctionPatch(loadPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(setChildFlagsPatch);
        RestoreFunctionPatch(groupCountPatch);
        return 2;
    }

    HudCmdDialog *const dialog =
        static_cast<HudCmdDialog *>(::operator new(sizeof(HudCmdDialog)));
    std::memset(dialog, 0, sizeof(HudCmdDialog));
    dialog->Constructor();

    InstallHudCmdDialogBinding(dialog->commandList, "Command");
    InstallHudCmdDialogBinding(dialog->keyAButton, "KeyA");
    InstallHudCmdDialogBinding(dialog->keyBButton, "KeyB");
    InstallHudCmdDialogBinding(dialog->joyButton, "Joy");
    InstallHudCmdDialogBinding(dialog->mouseButton, "Mouse");

    dialog->Destructor();

    const bool bindingsCleared =
        dialog->commandList.bindingVec.begin == nullptr &&
        dialog->commandList.bindingVec.end == nullptr &&
        dialog->commandList.bindingVec.capacity == nullptr &&
        dialog->keyAButton.bindingVec.begin == nullptr &&
        dialog->keyAButton.bindingVec.end == nullptr &&
        dialog->keyAButton.bindingVec.capacity == nullptr &&
        dialog->keyBButton.bindingVec.begin == nullptr &&
        dialog->keyBButton.bindingVec.end == nullptr &&
        dialog->keyBButton.bindingVec.capacity == nullptr &&
        dialog->joyButton.bindingVec.begin == nullptr &&
        dialog->joyButton.bindingVec.end == nullptr &&
        dialog->joyButton.bindingVec.capacity == nullptr &&
        dialog->mouseButton.bindingVec.begin == nullptr &&
        dialog->mouseButton.bindingVec.end == nullptr &&
        dialog->mouseButton.bindingVec.capacity == nullptr;

    ::operator delete(dialog);
    RestoreFunctionPatch(loadPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(setChildFlagsPatch);
    RestoreFunctionPatch(groupCountPatch);
    return bindingsCleared ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_scalar_deleting_destructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch setChildFlagsPatch{};
    CodeFunctionPatch groupCountPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CmdDialogLoadProbeAddress(),
            loadPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiContainer::SetChildFlags),
            HudUiContainerSetChildFlagsProbeAddress(),
            setChildFlagsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindGroupList_GetCount),
            reinterpret_cast<void *>(&FakeHudCmdBindGroupCount),
            groupCountPatch
        )) {
        RestoreFunctionPatch(loadPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(setChildFlagsPatch);
        RestoreFunctionPatch(groupCountPatch);
        return 2;
    }

    HudCmdDialog *const dialog =
        static_cast<HudCmdDialog *>(::operator new(sizeof(HudCmdDialog)));
    std::memset(dialog, 0, sizeof(HudCmdDialog));
    dialog->Constructor();
    InstallHudCmdDialogBinding(dialog->commandList, "Command");

    HudUiBackground *const returned = dialog->HudCmdDialog::ScalarDeletingDestructor(0);
    const bool noDeletePath =
        returned == dialog &&
        dialog->commandList.bindingVec.begin == nullptr &&
        dialog->commandList.bindingVec.end == nullptr &&
        dialog->commandList.bindingVec.capacity == nullptr;
    ::operator delete(dialog);

    HudCmdDialog *const deletingDialog =
        static_cast<HudCmdDialog *>(::operator new(sizeof(HudCmdDialog)));
    std::memset(deletingDialog, 0, sizeof(HudCmdDialog));
    deletingDialog->Constructor();
    InstallHudCmdDialogBinding(deletingDialog->commandList, "DeleteCommand");
    deletingDialog->HudCmdDialog::ScalarDeletingDestructor(1);

    RestoreFunctionPatch(loadPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(setChildFlagsPatch);
    RestoreFunctionPatch(groupCountPatch);
    return noDeletePath ? 0 : 1;
}

extern "C" int zhud_text_input_constructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiTextInput));
    std::memset(storage, 0, sizeof(HudUiTextInput));
    HudUiTextInput *const input = static_cast<HudUiTextInput *>(storage);
    HudUiTextInput *const result = input->Constructor(8);

    const bool constructed =
        result == input &&
        input->buffer != nullptr &&
        input->capacity == 8 &&
        input->cursor == 0 &&
        input->keyActionMap[0x20] == 0 &&
        input->keyActionMap[0x2e] == 0 &&
        input->keyActionMap[0x1b] == 2 &&
        input->keyActionMap[0x0d] == 3 &&
        input->keyActionMap[0x08] == 4 &&
        input->keyActionMap[0x7f] == 5 &&
        input->keyActionMap[0x02] == 6 &&
        input->keyActionMap[0x06] == 7 &&
        input->keyActionMap['A'] == 0 &&
        input->keyActionMap[1] == 1;

    input->SetContents("abcdef");
    const bool contents =
        std::strcmp(input->GetBuffer(), "abcdef") == 0 &&
        input->cursor == 0;

    input->DestructorCore();
    ::operator delete(storage);
    return constructed && contents ? 0 : 1;
}

extern "C" int zhud_text_input_destructor_core_smoke(void) {
    HudUiTextInput baseProbe{};
    baseProbe.buffer = nullptr;
    void *const baseVptr = *reinterpret_cast<void **>(&baseProbe);

    HudUiTextInput input{};
    *reinterpret_cast<void **>(&input) = nullptr;
    input.buffer = static_cast<char *>(::operator new(16));
    input.capacity = 16;
    input.cursor = 4;

    input.DestructorCore();
    const bool destructed =
        *reinterpret_cast<void **>(&input) == baseVptr &&
        input.capacity == 16 &&
        input.cursor == 4;
    input.buffer = nullptr;

    HudUiTextInput directInput{};
    *reinterpret_cast<void **>(&directInput) = nullptr;
    directInput.buffer = static_cast<char *>(::operator new(8));
    directInput.capacity = 8;
    directInput.cursor = 2;

    directInput.HudUiTextInput::~HudUiTextInput();
    const bool directDestructed =
        *reinterpret_cast<void **>(&directInput) == baseVptr &&
        directInput.capacity == 8 &&
        directInput.cursor == 2;
    directInput.buffer = nullptr;

    return destructed && directDestructed ? 0 : 1;
}

extern "C" int zhud_text_input_constructor_and_alloc_smoke(void) {
    return zhud_text_input_constructor_smoke();
}

extern "C" int zhud_polyline_and_slider_border_constructor_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x20;

    HudUiPolyline polyline{};
    RECT clip{1, 2, 3, 4};
    polyline.color565 = 0x1234;
    polyline.clipRect = &clip;
    polyline.points[5].x = 11;
    polyline.points[5].y = 12;
    polyline.pointCount = 9;
    polyline.Constructor();

    bool pointsCleared = true;
    for (const HudUiPolylinePoint &point : polyline.points) {
        pointsCleared = pointsCleared && point.x == 0 && point.y == 0;
    }

    const bool constructed =
        (polyline.flags & 0x20) != 0 &&
        polyline.pointCount == 0 &&
        polyline.color565 == 0x1234 &&
        polyline.clipRect == nullptr &&
        pointsCleared;

    polyline.flags = 0;
    polyline.SetPoint(0, 5, 6);
    const bool firstPoint =
        polyline.points[0].x == 5 &&
        polyline.points[0].y == 6 &&
        polyline.pointCount == 1 &&
        polyline.x == 5 &&
        polyline.y == 6 &&
        (polyline.flags & 0x20) != 0;

    polyline.flags = 0;
    polyline.SetPoint(3, 7, 8);
    const bool laterPoint =
        polyline.points[3].x == 7 &&
        polyline.points[3].y == 8 &&
        polyline.pointCount == 4 &&
        polyline.x == 5 &&
        polyline.y == 6 &&
        (polyline.flags & 0x20) != 0;

    HudUiSliderBorder border{};
    border.caretHalfWidth = 0x55;
    border.inputActive = 0x66;
    border.sliderVisibleWhenInputActive = 0x12;
    border.rawKeyFilterEnabled = 0x34;
    border.Constructor();

    const HudUiPolylinePoint expected[] = {
        {-1, 0},  {1, 0},  {1, 1}, {0, 1}, {0, 9},  {1, 9},  {1, 10},
        {-1, 10}, {-1, 9}, {0, 9}, {0, 1}, {-1, 1}, {-1, 0},
    };

    bool borderPoints = true;
    for (int index = 0; index < 13; ++index) {
        borderPoints = borderPoints &&
                       border.points[index].x == expected[index].x &&
                       border.points[index].y == expected[index].y;
    }

    const bool borderConstructed =
        border.pointCount == 13 &&
        border.x == -1 &&
        border.y == 0 &&
        border.originX == 0 &&
        border.originY == 0 &&
        border.halfWidth == 1 &&
        border.height == 10 &&
        border.blinkEnabled == 0 &&
        border.blinkPeriodSec == 0.35f &&
        border.blinkTimeRemainingSec == 0.0f &&
        border.blinkDirSign == 1 &&
        border.caretHalfWidth == 0x55 &&
        border.inputActive == 0x66 &&
        border.sliderVisibleWhenInputActive == 0x12 &&
        border.rawKeyFilterEnabled == 0x34 &&
        borderPoints;

    border.SetBounds(10, 20, 3, 8);
    const HudUiPolylinePoint expectedBounds[] = {
        {7, 20}, {13, 20}, {13, 21}, {10, 21}, {10, 27}, {13, 27}, {13, 28},
        {7, 28}, {7, 27},  {10, 27}, {10, 21}, {7, 21},  {7, 20},
    };

    bool boundsPoints = true;
    for (int index = 0; index < 13; ++index) {
        boundsPoints = boundsPoints &&
                       border.points[index].x == expectedBounds[index].x &&
                       border.points[index].y == expectedBounds[index].y;
    }

    const bool boundsSet =
        border.originX == 10 &&
        border.originY == 20 &&
        border.halfWidth == 3 &&
        border.height == 8 &&
        border.pointCount == 13 &&
        border.x == 7 &&
        border.y == 20 &&
        boundsPoints;

    g_HudUi_InvalidateMask = oldMask;
    return constructed && firstPoint && laterPoint && borderConstructed && boundsSet ? 0 : 1;
}

extern "C" int zhud_numeric_text_input_base_constructor_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;

    void *const storage = ::operator new(sizeof(HudUiNumericTextInput));
    std::memset(storage, 0, sizeof(HudUiNumericTextInput));
    HudUiNumericTextInput *const input = static_cast<HudUiNumericTextInput *>(storage);
    HudUiNumericTextInput *const result = input->BaseConstructor();

    const bool constructed =
        result == input &&
        input->modeOrEnabled == 1 &&
        input->textInput.buffer != nullptr &&
        input->textInput.capacity == 0x100 &&
        input->textInput.cursor == 0 &&
        input->textInput.owner == input &&
        input->sliderBorder.sliderVisibleWhenInputActive == 0 &&
        input->sliderBorder.rawKeyFilterEnabled == 0 &&
        input->sliderBorder.inputActive == 1 &&
        input->sliderBorder.caretHalfWidth == 0 &&
        (input->sliderBorder.flags & 0x10u) == 0 &&
        (input->flags & 0x10u) == 0;

    input->Update("42");
    const bool updated =
        std::strcmp(input->textInput.buffer, "42") == 0 &&
        input->textInput.cursor == 2 &&
        (input->flags & 0x80u) != 0;

    const int previousActive = input->SetInputActive(0);
    const bool disabled =
        previousActive == 1 &&
        input->sliderBorder.inputActive == 0 &&
        (input->flags & 0x10u) != 0 &&
        (input->sliderBorder.flags & 0x10u) != 0;

    const int previousInactive = input->SetInputActive(1);
    const bool enabled =
        previousInactive == 0 &&
        input->sliderBorder.inputActive == 1 &&
        (input->flags & 0x10u) == 0 &&
        (input->sliderBorder.flags & 0x10u) == 0;

    input->Destructor();
    ::operator delete(storage);
    g_HudUi_InvalidateMask = oldMask;
    return constructed && updated && disabled && enabled ? 0 : 1;
}

extern "C" int zhud_std_ptr_vector_clear_no_op_destroy_smoke(void) {
    int values[3] = {1, 2, 3};
    StdPtrVector vector{};
    vector.ClearNoOpDestroy(values, values + 3);
    vector.ClearNoOpDestroy(values + 1, values + 1);

    return values[0] == 1 && values[1] == 2 && values[2] == 3 ? 0 : 1;
}
