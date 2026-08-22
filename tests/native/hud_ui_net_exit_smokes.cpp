#include "Battlesport/hud_ui_net_exit_panel.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdint>
#include <cstring>

namespace {
template <typename Method> std::uintptr_t MethodAddress(Method method) {
    static_assert(sizeof(method) <= sizeof(std::uintptr_t));
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(method));
    return address;
}

struct CodeFunctionPatch {
    void *target;
    unsigned char original[5];
    bool active;
};

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    DWORD oldProtect = 0;
    if (!VirtualProtect(target, sizeof(patch.original), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    patch.target = target;
    std::memcpy(patch.original, target, sizeof(patch.original));

    unsigned char *const bytes = static_cast<unsigned char *>(target);
    bytes[0] = 0xe9;
    const std::intptr_t rel = reinterpret_cast<unsigned char *>(replacement) -
                              (reinterpret_cast<unsigned char *>(target) + 5);
    *reinterpret_cast<std::int32_t *>(bytes + 1) = static_cast<std::int32_t>(rel);

    FlushInstructionCache(GetCurrentProcess(), target, sizeof(patch.original));
    DWORD ignored = 0;
    VirtualProtect(target, sizeof(patch.original), oldProtect, &ignored);
    patch.active = true;
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (!patch.active) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.target, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect)) {
        std::memcpy(patch.target, patch.original, sizeof(patch.original));
        FlushInstructionCache(GetCurrentProcess(), patch.target, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.target, sizeof(patch.original), oldProtect, &ignored);
    }
    patch.active = false;
}

std::uint32_t g_testNetExitDeleteFlags = 0;
float g_testNetExitUpdateDelta = -1.0f;
HudUiNetExitPanel *g_testNetExitUpdateThis = nullptr;
std::int32_t g_testNetExitSetEnabled = -1;
HudUiBackground *g_testNetExitSetEnabledThis = nullptr;
std::int32_t g_testNetExitQueueSwitchCount = 0;
RecoilApp_IState *g_testNetExitQueueSwitchState = nullptr;
std::int32_t g_testNetExitQueueSwitchParam = -1;
std::int32_t g_testNetExitDestructorStep = 0;
HudUiZrdWidget *g_testNetExitDestroyedWidgets[2] = {};
zVidImagePartial *g_testNetExitReleasedImage = nullptr;
std::int32_t g_testNetExitResumeStep = 0;
HudUiNetExitPanel_ResumeWidget *g_testNetExitResumeHideThis = nullptr;
HudUiNetExitPanel *g_testNetExitResumeSetEnabledThis = nullptr;
std::int32_t g_testNetExitResumeSetEnabledValue = -1;
std::int32_t g_testNetExitResumeTriggerStep = 0;
HudUiZrdWidget *g_testNetExitResumeActivateThis = nullptr;
std::int32_t g_testNetExitShowPreviewStep = 0;
std::int32_t g_testNetExitPushCount = 0;
zInput_BindMapContext *g_testNetExitPushArg = nullptr;
std::int32_t g_testNetExitMouseBindingCount = 0;
std::int32_t g_testNetExitMouseSlot = -1;
std::int32_t g_testNetExitMouseCommand = -1;
std::int32_t g_testNetExitReticleCount = 0;
std::int32_t g_testNetExitReticleMode = -1;
zVec3 *g_testNetExitReticleHitPoint = nullptr;
float g_testNetExitReticleX = -1.0f;
float g_testNetExitReticleY = -1.0f;
std::int32_t g_testNetExitSetInputFocusCount = 0;
HudUiBackgroundContainer *g_testNetExitSetInputFocusThis = nullptr;
HudUiElement *g_testNetExitSetInputFocusElement = nullptr;
std::int32_t g_testNetExitShowPreviewCount = 0;
HudUiZrdWidget *g_testNetExitShowPreviewThis = nullptr;
std::int32_t g_testNetExitHidePreviewStep = 0;
std::int32_t g_testNetExitPopCount = 0;
std::int32_t g_testNetExitHideReticleCount = 0;
std::int32_t g_testNetExitHideReticleMode = -1;
zVec3 *g_testNetExitHideReticleHitPoint = nullptr;
float g_testNetExitHideReticleX = -1.0f;
float g_testNetExitHideReticleY = -1.0f;
std::int32_t g_testNetExitGetInputFocusCount = 0;
HudUiBackgroundContainer *g_testNetExitGetInputFocusThis = nullptr;
HudUiElement *g_testNetExitGetInputFocusResult = nullptr;
std::int32_t g_testNetExitHideSetInputFocusCount = 0;
HudUiBackgroundContainer *g_testNetExitHideSetInputFocusThis = nullptr;
HudUiElement *g_testNetExitHideSetInputFocusElement = nullptr;
std::int32_t g_testNetExitHidePreviewCount = 0;
HudUiZrdWidget *g_testNetExitHidePreviewThis = nullptr;

struct TestNetExitPatchOps {
    RecoilApp_IState * QueueSwitchCurrentState(RecoilApp_IState *state,
                                               std::int32_t stateParam) {
        ++g_testNetExitQueueSwitchCount;
        g_testNetExitQueueSwitchState = state;
        g_testNetExitQueueSwitchParam = stateParam;
        return state;
    }

    void NetExitUpdate(float deltaSeconds) {
        g_testNetExitUpdateThis = reinterpret_cast<HudUiNetExitPanel *>(this);
        g_testNetExitUpdateDelta = deltaSeconds;
    }

    void BackgroundSetEnabled(std::int32_t enabled) {
        ++g_testNetExitResumeStep;
        g_testNetExitSetEnabledThis = reinterpret_cast<HudUiBackground *>(this);
        g_testNetExitSetEnabled = enabled;
        g_testNetExitResumeSetEnabledThis =
            reinterpret_cast<HudUiNetExitPanel *>(this);
        g_testNetExitResumeSetEnabledValue = enabled;
    }

    void ZrdWidgetDestructorCore() {
        const int index = g_testNetExitDestructorStep;
        if (index < 2) {
            g_testNetExitDestroyedWidgets[index] =
                reinterpret_cast<HudUiZrdWidget *>(this);
        }
        ++g_testNetExitDestructorStep;
    }

    void HidePreview() {
        ++g_testNetExitResumeStep;
        g_testNetExitResumeHideThis =
            reinterpret_cast<HudUiNetExitPanel_ResumeWidget *>(this);
    }

    void ZrdWidgetOnActivate() {
        ++g_testNetExitResumeStep;
        g_testNetExitResumeActivateThis = reinterpret_cast<HudUiZrdWidget *>(this);
    }

    void BackgroundContainerSetInputFocus(HudUiElement *element) {
        ++g_testNetExitShowPreviewStep;
        ++g_testNetExitSetInputFocusCount;
        g_testNetExitSetInputFocusThis =
            reinterpret_cast<HudUiBackgroundContainer *>(this);
        g_testNetExitSetInputFocusElement = element;
    }

    void ZrdWidgetShowPreview() {
        ++g_testNetExitShowPreviewStep;
        ++g_testNetExitShowPreviewCount;
        g_testNetExitShowPreviewThis = reinterpret_cast<HudUiZrdWidget *>(this);
    }

    HudUiElement * BackgroundContainerGetInputFocus() {
        ++g_testNetExitHidePreviewStep;
        ++g_testNetExitGetInputFocusCount;
        g_testNetExitGetInputFocusThis =
            reinterpret_cast<HudUiBackgroundContainer *>(this);
        return g_testNetExitGetInputFocusResult;
    }

    void BackgroundContainerSetInputFocusForHide(HudUiElement *element) {
        ++g_testNetExitHidePreviewStep;
        ++g_testNetExitHideSetInputFocusCount;
        g_testNetExitHideSetInputFocusThis =
            reinterpret_cast<HudUiBackgroundContainer *>(this);
        g_testNetExitHideSetInputFocusElement = element;
    }

    void ZrdWidgetHidePreview() {
        ++g_testNetExitHidePreviewStep;
        ++g_testNetExitHidePreviewCount;
        g_testNetExitHidePreviewThis = reinterpret_cast<HudUiZrdWidget *>(this);
    }
};

void FakeNetExitTriggerCurrentLayoutOnActivated() {
    ++g_testNetExitResumeStep;
    g_testNetExitResumeTriggerStep = g_testNetExitResumeStep;
}

void __fastcall FakeNetExitBindMapContextPush(zInput_BindMapContext *bindMapOrNull) {
    ++g_testNetExitShowPreviewStep;
    ++g_testNetExitPushCount;
    g_testNetExitPushArg = bindMapOrNull;
}

void __fastcall FakeNetExitSetMouseBinding(int mouseSlot, int commandId) {
    ++g_testNetExitShowPreviewStep;
    ++g_testNetExitMouseBindingCount;
    g_testNetExitMouseSlot = mouseSlot;
    g_testNetExitMouseCommand = commandId;
}

int __fastcall FakeNetExitUpdateTargetReticleFromCursor(
    int reticleMode,
    zVec3 *worldHitPoint,
    float normalizedX,
    float normalizedY) {
    ++g_testNetExitShowPreviewStep;
    ++g_testNetExitReticleCount;
    g_testNetExitReticleMode = reticleMode;
    g_testNetExitReticleHitPoint = worldHitPoint;
    g_testNetExitReticleX = normalizedX;
    g_testNetExitReticleY = normalizedY;
    return 0;
}

void ResetNetExitShowPreviewProbe() {
    g_testNetExitShowPreviewStep = 0;
    g_testNetExitPushCount = 0;
    g_testNetExitPushArg = nullptr;
    g_testNetExitMouseBindingCount = 0;
    g_testNetExitMouseSlot = -1;
    g_testNetExitMouseCommand = -1;
    g_testNetExitReticleCount = 0;
    g_testNetExitReticleMode = -1;
    g_testNetExitReticleHitPoint = nullptr;
    g_testNetExitReticleX = -1.0f;
    g_testNetExitReticleY = -1.0f;
    g_testNetExitSetInputFocusCount = 0;
    g_testNetExitSetInputFocusThis = nullptr;
    g_testNetExitSetInputFocusElement = nullptr;
    g_testNetExitShowPreviewCount = 0;
    g_testNetExitShowPreviewThis = nullptr;
}

void FakeNetExitBindMapContextPop() {
    ++g_testNetExitHidePreviewStep;
    ++g_testNetExitPopCount;
}

int __fastcall FakeNetExitHideUpdateTargetReticleFromCursor(
    int reticleMode,
    zVec3 *worldHitPoint,
    float normalizedX,
    float normalizedY) {
    ++g_testNetExitHidePreviewStep;
    ++g_testNetExitHideReticleCount;
    g_testNetExitHideReticleMode = reticleMode;
    g_testNetExitHideReticleHitPoint = worldHitPoint;
    g_testNetExitHideReticleX = normalizedX;
    g_testNetExitHideReticleY = normalizedY;
    return 0;
}

void ResetNetExitHidePreviewProbe(HudUiElement *getInputFocusResult) {
    g_testNetExitHidePreviewStep = 0;
    g_testNetExitPopCount = 0;
    g_testNetExitHideReticleCount = 0;
    g_testNetExitHideReticleMode = -1;
    g_testNetExitHideReticleHitPoint = nullptr;
    g_testNetExitHideReticleX = -1.0f;
    g_testNetExitHideReticleY = -1.0f;
    g_testNetExitGetInputFocusCount = 0;
    g_testNetExitGetInputFocusThis = nullptr;
    g_testNetExitGetInputFocusResult = getInputFocusResult;
    g_testNetExitHideSetInputFocusCount = 0;
    g_testNetExitHideSetInputFocusThis = nullptr;
    g_testNetExitHideSetInputFocusElement = nullptr;
    g_testNetExitHidePreviewCount = 0;
    g_testNetExitHidePreviewThis = nullptr;
}

int __fastcall FakeNetExitReleaseIfNotDefault(zVidImagePartial *image) {
    g_testNetExitReleasedImage = image;
    ++g_testNetExitDestructorStep;
    return 0;
}

int __fastcall TestVideoSurfaceStateNoOp(zVideo_SurfaceStatePartial *) {
    return 0;
}

void ResetNetExitDestructorProbe() {
    g_testNetExitDestructorStep = 0;
    g_testNetExitDestroyedWidgets[0] = nullptr;
    g_testNetExitDestroyedWidgets[1] = nullptr;
    g_testNetExitReleasedImage = nullptr;
}

bool InstallNetExitDestructorPatches(CodeFunctionPatch *patches) {
    return PatchFunctionJump(reinterpret_cast<void *>(MethodAddress(&HudUiZrdWidget::DestructorCore)),
                             reinterpret_cast<void *>(
                                 MethodAddress(&TestNetExitPatchOps::ZrdWidgetDestructorCore)),
                             patches[0]) &&
           PatchFunctionJump(reinterpret_cast<void *>(&zVid_Image::ReleaseIfNotDefault),
                             reinterpret_cast<void *>(&FakeNetExitReleaseIfNotDefault),
                             patches[1]);
}

void RestoreNetExitPatches(CodeFunctionPatch *patches, int count) {
    for (int index = count - 1; index >= 0; --index) {
        RestoreFunctionPatch(patches[index]);
    }
}
} // namespace

extern "C" int hud_ui_net_exit_destroy_global_smoke(void) {
    CodeFunctionPatch patches[2] = {};
    const bool installed = InstallNetExitDestructorPatches(patches);

    HudUiNetExitPanel *const panel = new HudUiNetExitPanel;
    HudUiZrdWidget *const expectedExitWidget = &panel->exitWidget;
    HudUiZrdWidget *const expectedResumeWidget = &panel->resumeWidget;
    zVidImagePartial clipImage = {};
    panel->primaryClipImage = &clipImage;
    ResetNetExitDestructorProbe();
    g_HudUiNetExitPanel = panel;

    if (installed) {
        HudUiNetExitPanel::DestroyGlobal();
    }
    if (!installed) {
        panel->primaryClipImage = nullptr;
        g_HudUiNetExitPanel = nullptr;
        delete panel;
    }

    RestoreNetExitPatches(patches, 2);

    return installed && g_HudUiNetExitPanel == nullptr &&
                   g_testNetExitDestructorStep == 3 &&
                   g_testNetExitDestroyedWidgets[0] == expectedExitWidget &&
                   g_testNetExitDestroyedWidgets[1] == expectedResumeWidget &&
                   g_testNetExitReleasedImage == &clipImage
               ? 0
               : 1;
}

extern "C" int hud_ui_net_exit_show_tick_smoke(void) {
    CodeFunctionPatch patch{};
    const bool installed =
        PatchFunctionJump(reinterpret_cast<void *>(MethodAddress(&HudUiContainer::UpdateAll)),
                          reinterpret_cast<void *>(
                              MethodAddress(&TestNetExitPatchOps::NetExitUpdate)),
                          patch);

    HudUiNetExitPanel panel{};
    panel.enabled = 0;
    g_testNetExitUpdateDelta = -1.0f;
    g_testNetExitUpdateThis = nullptr;
    g_testNetExitSetEnabled = -1;
    g_testNetExitSetEnabledThis = nullptr;
    g_testNetExitResumeStep = 0;
    g_FrameDeltaTimeSec = 0.25f;
    g_HudUiNetExitPanel = &panel;

    if (installed) {
        HudUiNetExitPanel::Show();
        HudUiNetExitPanel::Tick();
    }

    g_HudUiNetExitPanel = nullptr;
    RestoreFunctionPatch(patch);

    return installed && panel.enabled == 1 && g_testNetExitUpdateDelta == 0.25f &&
                   g_testNetExitUpdateThis == &panel
               ? 0
               : 1;
}

extern "C" int hud_ui_net_exit_exit_button_on_activate_smoke(void) {
    CodeFunctionPatch patch{};
    const bool installed =
        PatchFunctionJump(reinterpret_cast<void *>(MethodAddress(&RecoilApp::QueueSwitchCurrentState)),
                          reinterpret_cast<void *>(
                              MethodAddress(&TestNetExitPatchOps::QueueSwitchCurrentState)),
                          patch);

    HudUiNetExitPanel_ExitButton button{};
    g_testNetExitQueueSwitchCount = 0;
    g_testNetExitQueueSwitchState = nullptr;
    g_testNetExitQueueSwitchParam = -1;
    if (installed) {
        button.OnActivate();
    }

    RestoreFunctionPatch(patch);
    return installed && g_testNetExitQueueSwitchCount == 1 &&
                   g_testNetExitQueueSwitchState == &g_RecoilApp.m_leaveNetworkState &&
                   g_testNetExitQueueSwitchParam == 0
               ? 0
               : 1;
}

extern "C" int hud_ui_net_exit_destructor_smoke(void) {
    CodeFunctionPatch patches[2] = {};
    const bool installed = InstallNetExitDestructorPatches(patches);

    HudUiNetExitPanel *const panel = new HudUiNetExitPanel;
    zVidImagePartial clipImage = {};
    panel->primaryClipImage = &clipImage;
    HudUiZrdWidget *const expectedExitWidget = &panel->exitWidget;
    HudUiZrdWidget *const expectedResumeWidget = &panel->resumeWidget;
    ResetNetExitDestructorProbe();
    if (installed) {
        delete panel;
    } else {
        panel->primaryClipImage = nullptr;
        delete panel;
    }

    RestoreNetExitPatches(patches, 2);

    return installed && g_testNetExitDestructorStep == 3 &&
                   g_testNetExitDestroyedWidgets[0] == expectedExitWidget &&
                   g_testNetExitDestroyedWidgets[1] == expectedResumeWidget &&
                   g_testNetExitReleasedImage == &clipImage
               ? 0
               : 1;
}

extern "C" int hud_ui_net_exit_scalar_deleting_destructor_smoke(void) {
    return hud_ui_net_exit_destroy_global_smoke();
}

extern "C" int hud_ui_net_exit_resume_widget_on_activate_smoke(void) {
    CodeFunctionPatch patch{};
    const bool installed =
        PatchFunctionJump(reinterpret_cast<void *>(&HudUiMgr::TriggerCurrentLayoutOnActivated),
                          reinterpret_cast<void *>(
                              &FakeNetExitTriggerCurrentLayoutOnActivated),
                          patch);

    HudUiNetExitPanel panel{};
    panel.enabled = 1;
    HudUiNetExitPanel_ResumeWidget widget{};

    HudUiNetExitPanel *const oldPanel = g_HudUiNetExitPanel;
    g_HudUiNetExitPanel = &panel;
    g_testNetExitResumeStep = 0;
    g_testNetExitResumeHideThis = nullptr;
    g_testNetExitResumeSetEnabledThis = nullptr;
    g_testNetExitResumeSetEnabledValue = -1;
    g_testNetExitResumeTriggerStep = 0;
    g_testNetExitResumeActivateThis = nullptr;
    if (installed) {
        widget.OnActivate();
    }
    g_HudUiNetExitPanel = oldPanel;

    RestoreFunctionPatch(patch);

    return installed && panel.enabled == 0 && g_testNetExitResumeStep == 1 &&
                   g_testNetExitResumeTriggerStep == 1
               ? 0
               : 1;
}

extern "C" int hud_ui_net_exit_resume_widget_on_show_preview_smoke(void) {
    CodeFunctionPatch patches[4] = {};
    const bool installed =
        PatchFunctionJump(reinterpret_cast<void *>(&zInput::BindMapContext_Push),
                          reinterpret_cast<void *>(&FakeNetExitBindMapContextPush),
                          patches[0]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zInput::BindMapCurrent_SetMouseBinding),
                          reinterpret_cast<void *>(&FakeNetExitSetMouseBinding),
                          patches[1]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&HudUiMgr::UpdateTargetReticleFromCursor),
                          reinterpret_cast<void *>(&FakeNetExitUpdateTargetReticleFromCursor),
                          patches[2]) &&
        PatchFunctionJump(reinterpret_cast<void *>(
                              MethodAddress(&HudUiBackgroundContainer::SetInputFocus)),
                          reinterpret_cast<void *>(MethodAddress(
                              &TestNetExitPatchOps::BackgroundContainerSetInputFocus)),
                          patches[3]);

    int joystickOption = 0;
    int *const oldJoystickOption = g_zGame_Options_PointerCache.inputJoystick;
    g_zGame_Options_PointerCache.inputJoystick = &joystickOption;

    HudUiBackgroundContainer owner(0);
    HudUiElement savedFocus{};
    HudUiNetExitPanel_ResumeWidget widget{};
    widget.owner = reinterpret_cast<HudUiBackground *>(&owner);
    widget.previewInputCaptureActive = 0;
    g_HudUiNetExitPanel_SavedInputFocus = &savedFocus;

    ResetNetExitShowPreviewProbe();
    if (installed) {
        widget.OnShowPreview();
    }
    const bool firstActivationOk =
        installed && widget.previewInputCaptureActive == 1 &&
        g_testNetExitShowPreviewStep == 4 &&
        g_testNetExitPushCount == 1 &&
        g_testNetExitPushArg == nullptr &&
        g_testNetExitMouseBindingCount == 1 &&
        g_testNetExitMouseSlot == 1 &&
        g_testNetExitMouseCommand == 0 &&
        g_testNetExitReticleCount == 1 &&
        g_testNetExitReticleMode == 0 &&
        g_testNetExitReticleHitPoint == nullptr &&
        g_testNetExitReticleX == 0.0f &&
        g_testNetExitReticleY == 0.0f &&
        g_testNetExitSetInputFocusCount == 1 &&
        g_testNetExitSetInputFocusThis == &owner &&
        g_testNetExitSetInputFocusElement == &savedFocus;

    if (installed) {
        widget.OnShowPreview();
    }
    const bool alreadyActiveOk =
        firstActivationOk && widget.previewInputCaptureActive == 1 &&
        g_testNetExitShowPreviewStep == 4 &&
        g_testNetExitPushCount == 1 &&
        g_testNetExitMouseBindingCount == 1 &&
        g_testNetExitReticleCount == 1 &&
        g_testNetExitSetInputFocusCount == 1;

    HudUiNetExitPanel_ResumeWidget joystickWidget{};
    joystickWidget.owner = reinterpret_cast<HudUiBackground *>(&owner);
    joystickWidget.previewInputCaptureActive = 0;
    joystickOption = 1;
    ResetNetExitShowPreviewProbe();
    if (installed) {
        joystickWidget.OnShowPreview();
    }
    const bool joystickPathOk =
        installed && joystickWidget.previewInputCaptureActive == 1 &&
        g_testNetExitShowPreviewStep == 2 &&
        g_testNetExitPushCount == 1 &&
        g_testNetExitMouseBindingCount == 1 &&
        g_testNetExitReticleCount == 0 &&
        g_testNetExitSetInputFocusCount == 0;

    g_zGame_Options_PointerCache.inputJoystick = oldJoystickOption;
    g_HudUiNetExitPanel_SavedInputFocus = nullptr;
    RestoreNetExitPatches(patches, 4);

    return alreadyActiveOk && joystickPathOk ? 0 : 1;
}

extern "C" int hud_ui_net_exit_resume_widget_on_hide_preview_smoke(void) {
    CodeFunctionPatch patches[4] = {};
    const bool installed =
        PatchFunctionJump(reinterpret_cast<void *>(&zInput::BindMapContext_Pop),
                          reinterpret_cast<void *>(&FakeNetExitBindMapContextPop),
                          patches[0]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&HudUiMgr::UpdateTargetReticleFromCursor),
                          reinterpret_cast<void *>(
                              &FakeNetExitHideUpdateTargetReticleFromCursor),
                          patches[1]) &&
        PatchFunctionJump(reinterpret_cast<void *>(
                              MethodAddress(&HudUiBackgroundContainer::GetInputFocus)),
                          reinterpret_cast<void *>(MethodAddress(
                              &TestNetExitPatchOps::BackgroundContainerGetInputFocus)),
                          patches[2]) &&
        PatchFunctionJump(reinterpret_cast<void *>(
                              MethodAddress(&HudUiBackgroundContainer::SetInputFocus)),
                          reinterpret_cast<void *>(MethodAddress(
                              &TestNetExitPatchOps::BackgroundContainerSetInputFocusForHide)),
                          patches[3]);

    int joystickOption = 0;
    int *const oldJoystickOption = g_zGame_Options_PointerCache.inputJoystick;
    g_zGame_Options_PointerCache.inputJoystick = &joystickOption;

    HudUiBackgroundContainer owner(0);
    HudUiElement currentFocus{};
    HudUiElement staleSavedFocus{};
    HudUiNetExitPanel_ResumeWidget widget{};
    widget.owner = reinterpret_cast<HudUiBackground *>(&owner);
    widget.previewInputCaptureActive = 1;
    g_HudUiNetExitPanel_SavedInputFocus = &staleSavedFocus;

    ResetNetExitHidePreviewProbe(&currentFocus);
    if (installed) {
        widget.OnHidePreview();
    }
    const bool firstHideOk =
        installed && widget.previewInputCaptureActive == 0 &&
        g_testNetExitHidePreviewStep == 4 &&
        g_testNetExitPopCount == 1 &&
        g_testNetExitHideReticleCount == 1 &&
        g_testNetExitHideReticleMode == 1 &&
        g_testNetExitHideReticleHitPoint == nullptr &&
        g_testNetExitHideReticleX == 0.0f &&
        g_testNetExitHideReticleY == 0.0f &&
        g_testNetExitGetInputFocusCount == 1 &&
        g_testNetExitGetInputFocusThis == &owner &&
        g_HudUiNetExitPanel_SavedInputFocus == &currentFocus &&
        g_testNetExitHideSetInputFocusCount == 1 &&
        g_testNetExitHideSetInputFocusThis == &owner &&
        g_testNetExitHideSetInputFocusElement == nullptr;

    if (installed) {
        widget.OnHidePreview();
    }
    const bool alreadyInactiveOk =
        firstHideOk && widget.previewInputCaptureActive == 0 &&
        g_testNetExitHidePreviewStep == 4 &&
        g_testNetExitPopCount == 1 &&
        g_testNetExitHideReticleCount == 1 &&
        g_testNetExitGetInputFocusCount == 1 &&
        g_testNetExitHideSetInputFocusCount == 1;

    HudUiNetExitPanel_ResumeWidget joystickWidget{};
    joystickWidget.owner = reinterpret_cast<HudUiBackground *>(&owner);
    joystickWidget.previewInputCaptureActive = 1;
    joystickOption = 1;
    g_HudUiNetExitPanel_SavedInputFocus = &staleSavedFocus;
    ResetNetExitHidePreviewProbe(&currentFocus);
    if (installed) {
        joystickWidget.OnHidePreview();
    }
    const bool joystickPathOk =
        installed && joystickWidget.previewInputCaptureActive == 0 &&
        g_testNetExitHidePreviewStep == 1 &&
        g_testNetExitPopCount == 1 &&
        g_testNetExitHideReticleCount == 0 &&
        g_testNetExitGetInputFocusCount == 0 &&
        g_testNetExitHideSetInputFocusCount == 0 &&
        g_HudUiNetExitPanel_SavedInputFocus == &staleSavedFocus;

    g_zGame_Options_PointerCache.inputJoystick = oldJoystickOption;
    g_HudUiNetExitPanel_SavedInputFocus = nullptr;
    RestoreNetExitPatches(patches, 4);

    return alreadyInactiveOk && joystickPathOk ? 0 : 1;
}

extern "C" int hud_ui_net_exit_constructor_smoke(void) {
    int joystickOption = 0;
    int *const savedJoystickOption = g_zGame_Options_PointerCache.inputJoystick;
    g_zGame_Options_PointerCache.inputJoystick = &joystickOption;

    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption{};
    vmodeOption.payloadOrBuffer = 6;
    vmodeOption.name = vmodeName;
    zOptionEntryPartial *const savedOptionsHead = g_zGame_Options_OptionListHead;
    g_zGame_Options_OptionListHead = &vmodeOption;

    std::uint16_t pixels[4] = {};
    const int savedRendererType = g_zVideo_RendererType;
    const int savedHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial savedPrimarySurface = g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const savedLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const savedUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(std::uint16_t) * 2;

    HudUiElement savedFocus{};
    g_HudUiNetExitPanel_SavedInputFocus = &savedFocus;

    HudUiNetExitPanel panel;
    const bool initialized =
        panel.resumeWidget.previewInputCaptureActive == 0 &&
        panel.exitWidget.previewInputCaptureActive == 0 &&
        panel.inputFocusElement == nullptr &&
        panel.enabled == 0 &&
        g_HudUiNetExitPanel_SavedInputFocus == nullptr;

    g_zGame_Options_PointerCache.inputJoystick = savedJoystickOption;
    g_zGame_Options_OptionListHead = savedOptionsHead;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_UseHalfResBackbuffer = savedHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = savedPrimarySurface;
    g_zVideo_pfnLockSurfaceState = savedLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = savedUnlockSurfaceState;
    g_HudUiNetExitPanel_SavedInputFocus = nullptr;

    return initialized ? 0 : 1;
}

extern "C" int hud_ui_net_exit_create_global_smoke(void) {
    int joystickOption = 0;
    int *const savedJoystickOption = g_zGame_Options_PointerCache.inputJoystick;
    g_zGame_Options_PointerCache.inputJoystick = &joystickOption;

    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption{};
    vmodeOption.payloadOrBuffer = 6;
    vmodeOption.name = vmodeName;
    zOptionEntryPartial *const savedOptionsHead = g_zGame_Options_OptionListHead;
    g_zGame_Options_OptionListHead = &vmodeOption;

    std::uint16_t pixels[4] = {};
    const int savedRendererType = g_zVideo_RendererType;
    const int savedHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial savedPrimarySurface = g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const savedLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const savedUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(std::uint16_t) * 2;

    HudUiElement savedFocus{};
    g_HudUiNetExitPanel = nullptr;
    g_HudUiNetExitPanel_SavedInputFocus = &savedFocus;

    HudUiNetExitPanel *const panel = HudUiNetExitPanel::CreateGlobal();
    const bool created =
        panel != nullptr && g_HudUiNetExitPanel == panel &&
        panel->resumeWidget.previewInputCaptureActive == 0 &&
        panel->exitWidget.previewInputCaptureActive == 0 &&
        panel->inputFocusElement == nullptr &&
        panel->enabled == 0 &&
        g_HudUiNetExitPanel_SavedInputFocus == nullptr;

    HudUiNetExitPanel::DestroyGlobal();

    g_zGame_Options_PointerCache.inputJoystick = savedJoystickOption;
    g_zGame_Options_OptionListHead = savedOptionsHead;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_UseHalfResBackbuffer = savedHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = savedPrimarySurface;
    g_zVideo_pfnLockSurfaceState = savedLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = savedUnlockSurfaceState;
    g_HudUiNetExitPanel_SavedInputFocus = nullptr;

    return created ? 0 : 1;
}
