#include "Battlesport/HudUiNetExitPanel.h"

#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zInput/zInput.h"

#include <string.h>

extern "C" {
HudUiNetExitPanel *g_HudUiNetExitPanel = 0;
HudUiElement *g_HudUiNetExitPanel_SavedInputFocus = 0;
}

namespace {
template <typename Method> unsigned int MethodAddress(Method method) {
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(unsigned int));
    unsigned int address = 0;
    memcpy(&address, &method, sizeof(method));
    return address;
}

template <typename Function, typename Method> Function MethodFunction(Method method) {
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(Function));
    Function function = 0;
    memcpy(&function, &method, sizeof(method));
    return function;
}

RECOIL_NOINLINE void RECOIL_CDECL HudUiWidgetPostLoadNoOp() {}

HudUiWidget_FTable MakeHudUiNetExitPanelExitWidgetFTable()
{
    HudUiWidget_FTable table = {0};
    table.slots[0] = MethodAddress(&HudUiZrdWidget::ScalarDeletingDestructor);
    table.slots[1] = MethodAddress(&HudUiWidget::Draw);
    table.slots[3] = MethodAddress(&HudUiElement::SetPos);
    table.slots[4] = MethodAddress(&HudUiElement::SetX);
    table.slots[5] = MethodAddress(&HudUiElement::SetY);
    table.slots[6] = MethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = MethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = MethodAddress(&HudUiZrdWidget::Invalidate);
    table.slots[12] = MethodAddress(&HudUiNetExitPanel_ExitButton::OnActivate);
    table.slots[15] = MethodAddress(&HudUiNetExitPanel_ResumeWidget::OnShowPreview);
    table.slots[16] = MethodAddress(&HudUiNetExitPanel_ResumeWidget::OnHidePreview);
    table.slots[24] = MethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = MethodAddress(&HudUiElement::GetX);
    table.slots[26] = MethodAddress(&HudUiElement::GetY);
    table.slots[30] = MethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = MethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = (unsigned int)(&HudUiWidgetPostLoadNoOp);
    return table;
}

const HudUiWidget_FTable g_HudUiNetExitPanel_ExitWidget_FTable =
    MakeHudUiNetExitPanelExitWidgetFTable();

HudUiWidget_FTable MakeHudUiNetExitPanelResumeWidgetFTable()
{
    HudUiWidget_FTable table = g_HudUiNetExitPanel_ExitWidget_FTable;
    table.slots[12] = MethodAddress(&HudUiNetExitPanel_ResumeWidget::OnActivate);
    return table;
}

const HudUiWidget_FTable g_HudUiNetExitPanel_ResumeWidget_FTable =
    MakeHudUiNetExitPanelResumeWidgetFTable();

HudUiNetExitPanel_FTable MakeHudUiNetExitPanelFTable()
{
    HudUiNetExitPanel_FTable table = {0};
    table.updateAll = MethodFunction<void(RECOIL_THISCALL *)(HudUiNetExitPanel *, float)>(
        &HudUiNetExitPanel::Update);
    table.setEnabled = MethodFunction<int(RECOIL_THISCALL *)(HudUiNetExitPanel *, int)>(
        &HudUiNetExitPanel::SetEnabled);
    table.scalarDeletingDtor = MethodFunction<HudUiNetExitPanel *(RECOIL_THISCALL *)(
        HudUiNetExitPanel *, unsigned int)>(
        &HudUiNetExitPanel::ScalarDeletingDestructor);
    return table;
}

const HudUiNetExitPanel_FTable g_HudUiNetExitPanel_FTable = MakeHudUiNetExitPanelFTable();
} // namespace

// Reimplements 0x41bd80: HudUiNetExitPanel::Constructor
// (D:\Proj\Battlesport\HudUi_NetExit.cpp)
HudUiNetExitPanel *RECOIL_THISCALL HudUiNetExitPanel::Constructor() {
    base.Constructor();

    resumeWidget.base.Constructor();
    resumeWidget.previewInputCaptureActive = 0;
    resumeWidget.base.base.ftable = &g_HudUiNetExitPanel_ResumeWidget_FTable;

    exitWidget.base.Constructor();
    exitWidget.previewInputCaptureActive = 0;
    exitWidget.base.base.ftable = &g_HudUiNetExitPanel_ExitWidget_FTable;

    base.base.base.vptr =
        (const HudUiContainer_FTable *)(&g_HudUiNetExitPanel_FTable);

    zReader::Node *const loadedSection = base.LoadFromZrd("dialog.zrd", "NETEXIT", 1);
    if (loadedSection != 0) {
        base.BindWidgetByName(loadedSection, &exitWidget.base.base, "EXIT");
        base.BindWidgetByName(loadedSection, &resumeWidget.base.base, "RESUME");
        base.FreeLoadedTreeRoots(
            (int)((unsigned int)(loadedSection)));
    }

    if (zInp::GetJoystickOption() == 0) {
        g_HudUiNetExitPanel_SavedInputFocus = base.base.GetInputFocus();
        base.base.SetInputFocus(0);
    }

    base.base.base.SetChildFlags(0);
    SetEnabled(0);
    return this;
}

// Reimplements 0x41beb0: HudUiNetExitPanel::Destructor
// (D:\Proj\Battlesport\HudUi_NetExit.cpp)
void RECOIL_THISCALL HudUiNetExitPanel::Destructor() {
    exitWidget.base.DestructorCore();
    resumeWidget.base.DestructorCore();
    base.Destructor();
}

void RECOIL_THISCALL HudUiNetExitPanel::Update(float deltaSeconds) {
    base.Update(deltaSeconds);
}

int RECOIL_THISCALL HudUiNetExitPanel::SetEnabled(int enabled) {
    base.SetEnabled(enabled);
    return 0;
}

// Reimplements 0x41be90: HudUiNetExitPanel_ScalarDeletingDtor
HudUiNetExitPanel *RECOIL_THISCALL
HudUiNetExitPanel::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x41be70: HudUiNetExitPanel_ExitButton::OnActivate
void RECOIL_THISCALL HudUiNetExitPanel_ExitButton::OnActivate() {
    g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_leaveNetworkState_1d0.base, 0);
}

// Reimplements 0x41bf10: HudUiNetExitPanel_ResumeWidget::OnActivate
void RECOIL_THISCALL HudUiNetExitPanel_ResumeWidget::OnActivate() {
    typedef void (RECOIL_THISCALL *HidePreviewFn)(HudUiNetExitPanel_ResumeWidget * self);
    ((HidePreviewFn)(base.base.ftable->slots[0x40 / 4]))(this);

    const HudUiNetExitPanel_FTable *const panelFtable =
        (const HudUiNetExitPanel_FTable *)(g_HudUiNetExitPanel->base.base.base.vptr);
    panelFtable->setEnabled(g_HudUiNetExitPanel, 0);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    base.OnActivate();
}

// Reimplements 0x41bf40: HudUiNetExitPanel_ResumeWidget::OnShowPreview
void RECOIL_THISCALL HudUiNetExitPanel_ResumeWidget::OnShowPreview() {
    if (previewInputCaptureActive == 0) {
        zInput::BindMapContext_Push(0);
        zInput::BindMapCurrent_SetMouseBinding(1, 0);

        if (zInp::GetJoystickOption() == 0) {
            HudUiMgr::UpdateTargetReticleFromCursor(0, 0, 0.0f, 0.0f);

            HudUiElement *const focus = g_HudUiNetExitPanel_SavedInputFocus;
            if (focus != 0) {
                ((HudUiBackgroundContainer *)(base.owner))->SetInputFocus(focus);
            }
        }

        previewInputCaptureActive = 1;
    }

    base.ShowPreview();
}

// Reimplements 0x41bfa0: HudUiNetExitPanel_ResumeWidget::OnHidePreview
void RECOIL_THISCALL HudUiNetExitPanel_ResumeWidget::OnHidePreview() {
    if (previewInputCaptureActive != 0) {
        zInput::BindMapContext_Pop();

        if (zInp::GetJoystickOption() == 0) {
            HudUiMgr::UpdateTargetReticleFromCursor(1, 0, 0.0f, 0.0f);
            HudUiBackgroundContainer *const owner =
                (HudUiBackgroundContainer *)(base.owner);
            g_HudUiNetExitPanel_SavedInputFocus = owner->GetInputFocus();
            owner->SetInputFocus(0);
        }

        previewInputCaptureActive = 0;
    }

    base.HidePreview();
}

// Reimplements 0x41c000: HudUiNetExitPanel::CreateGlobal
// (D:\Proj\Battlesport\HudUi_NetExit.cpp)
RECOIL_NOINLINE HudUiNetExitPanel *RECOIL_CDECL HudUiNetExitPanel::CreateGlobal() {
    HudUiNetExitPanel *const panel =
        (HudUiNetExitPanel *)(::operator new(sizeof(HudUiNetExitPanel)));
    if (panel == 0) {
        g_HudUiNetExitPanel = 0;
        return 0;
    }

    g_HudUiNetExitPanel = panel->Constructor();
    return g_HudUiNetExitPanel;
}

// Reimplements 0x41c070: HudUiNetExitPanel::Show
RECOIL_NOINLINE void RECOIL_CDECL HudUiNetExitPanel::Show() {
    const HudUiNetExitPanel_FTable *const ftable =
        (const HudUiNetExitPanel_FTable *)(g_HudUiNetExitPanel->base.base.base.vptr);
    ftable->setEnabled(g_HudUiNetExitPanel, 1);
}

// Reimplements 0x41c080: HudUiNetExitPanel::Tick
RECOIL_NOINLINE int RECOIL_CDECL HudUiNetExitPanel::Tick() {
    const HudUiNetExitPanel_FTable *const ftable =
        (const HudUiNetExitPanel_FTable *)(g_HudUiNetExitPanel->base.base.base.vptr);
    const unsigned int deltaBits =
        *(const unsigned int *)(&g_FrameDeltaTimeSec);
    typedef void (RECOIL_THISCALL *UpdateAllBitsFn)(HudUiNetExitPanel * self,
                                                    unsigned int deltaBits);
    ((UpdateAllBitsFn)(ftable->updateAll))(g_HudUiNetExitPanel, deltaBits);
    return 0;
}

// Reimplements 0x41c0a0: HudUiNetExitPanel::DestroyGlobal
RECOIL_NOINLINE void RECOIL_CDECL HudUiNetExitPanel::DestroyGlobal() {
    HudUiNetExitPanel *const panel = g_HudUiNetExitPanel;
    if (panel != 0) {
        const HudUiNetExitPanel_FTable *const ftable =
            (const HudUiNetExitPanel_FTable *)(panel->base.base.base.vptr);
        ftable->scalarDeletingDtor(panel, 1);
        g_HudUiNetExitPanel = 0;
    }
}
