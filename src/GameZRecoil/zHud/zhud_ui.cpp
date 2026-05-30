#include "GameZRecoil/zHud/zhud_ui.h"

#include "Battlesport/Briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/zImage.h"
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
#include <math.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

LPCSTR RECOIL_STDCALL AfxRegisterWndClass(UINT classStyle, HCURSOR cursor, HBRUSH background,
                                          HICON icon);

class CWnd {
  public:
    virtual BOOL RECOIL_THISCALL DestroyWindow();

    unsigned char unknown_04[0x1c];
    HWND m_hWnd;
};

namespace {
template <typename Method> unsigned int MethodAddress(Method method) {
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(unsigned int));
    unsigned int address = 0;
    memcpy(&address, &method, sizeof(method));
    return address;
}

template <typename Slot, typename Method> void AssignMethodSlot(Slot &slot, Method method) {
    RECOIL_STATIC_ASSERT(sizeof(slot) == sizeof(method));
    memcpy(&slot, &method, sizeof(slot));
}

RECOIL_NOINLINE void RECOIL_FASTCALL HudUiCommonInvalidateThunk(HudUiElement *element) {
    element->Invalidate();
}

RECOIL_NOINLINE void RECOIL_FASTCALL HudUiNoOpMethodStub(void *) {}

HudUiCommon_FTable MakeHudUiCommonFTable() {
    HudUiCommon_FTable table = {0};
    table.slots[8] = (unsigned int)(&HudUiCommonInvalidateThunk);
    table.slots[3] = MethodAddress(&HudUiElement::SetPos);
    table.slots[4] = MethodAddress(&HudUiElement::SetX);
    table.slots[5] = MethodAddress(&HudUiElement::SetY);
    table.slots[6] = MethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = MethodAddress(&HudUiElement::SetClipRect);
    table.slots[24] = MethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = MethodAddress(&HudUiElement::GetX);
    table.slots[26] = MethodAddress(&HudUiElement::GetY);
    return table;
}

HudUiContainer_FTable MakeHudUiContainerFTable() {
    HudUiContainer_FTable table = {0};
    AssignMethodSlot(table.updateAll, &HudUiContainer::UpdateAll);
    AssignMethodSlot(table.setEnabled, &HudUiContainer::SetEnabled);
    return table;
}

template <typename FTable> FTable MakeHudUiContainerLikeFTable() {
    FTable table = {0};
    AssignMethodSlot(table.updateAll, &HudUiContainer::UpdateAll);
    AssignMethodSlot(table.setEnabled, &HudUiContainer::SetEnabled);
    return table;
}

template <typename FTable> FTable MakeHudUiFTableWithCommonInvalidate() {
    FTable table = {0};
    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 2) {
        table.slots[2] = (unsigned int)(&HudUiNoOpMethodStub);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 8) {
        table.slots[8] = (unsigned int)(&HudUiCommonInvalidateThunk);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 3) {
        table.slots[3] = MethodAddress(&HudUiElement::SetPos);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 4) {
        table.slots[4] = MethodAddress(&HudUiElement::SetX);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 5) {
        table.slots[5] = MethodAddress(&HudUiElement::SetY);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 6) {
        table.slots[6] = MethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 7) {
        table.slots[7] = MethodAddress(&HudUiElement::SetClipRect);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 24) {
        table.slots[24] = MethodAddress(&HudUiElement::SetVisible);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 25) {
        table.slots[25] = MethodAddress(&HudUiElement::GetX);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 26) {
        table.slots[26] = MethodAddress(&HudUiElement::GetY);
    }

    return table;
}

HudUiZrdWidgetEx17C_Item_FTable MakeHudUiZrdWidgetEx17CItemFTable() {
    HudUiZrdWidgetEx17C_Item_FTable table =
        MakeHudUiFTableWithCommonInvalidate<HudUiZrdWidgetEx17C_Item_FTable>();
    table.slots[30] = MethodAddress(&HudUiZrdWidget::RefreshState);
    return table;
}

HudUiZrdWidgetEx17C_FTable MakeHudUiZrdWidgetEx17CFTable() {
    HudUiZrdWidgetEx17C_FTable table =
        MakeHudUiFTableWithCommonInvalidate<HudUiZrdWidgetEx17C_FTable>();
    table.slots[24] = MethodAddress(&HudUiZrdWidgetEx17C::EnableChildAtIndex);
    table.slots[30] = MethodAddress(&HudUiZrdWidget::RefreshState);
    return table;
}

HudUiTextInput_FTable MakeHudUiTextInputFTable() {
    HudUiTextInput_FTable table = {0};
    table.slots[0] = MethodAddress(&HudUiTextInput::InsertCharAtCursor);
    table.slots[1] = MethodAddress(&HudUiTextInput::InsertCharAtCursor);
    table.slots[2] = MethodAddress(&HudUiTextInput::MoveCursorRight);
    table.slots[3] = MethodAddress(&HudUiTextInput::MoveCursorLeft);
    table.slots[4] = MethodAddress(&HudUiTextInput::BackspaceDeleteChar);
    table.slots[5] = MethodAddress(&HudUiTextInput::DeleteCharForward);
    table.slots[6] = MethodAddress(&HudUiTextInput::MoveCursorLeft);
    table.slots[7] = MethodAddress(&HudUiTextInput::MoveCursorRight);
    table.slots[8] = MethodAddress(&HudUiTextInput::MoveCursorRight);
    return table;
}

HudUiWidget_FTable MakeHudUiWidgetFTable() {
    HudUiWidget_FTable table = MakeHudUiFTableWithCommonInvalidate<HudUiWidget_FTable>();
    table.slots[25] = MethodAddress(&HudUiWidget::GetCenterX);
    table.slots[26] = MethodAddress(&HudUiWidget::GetCenterY);
    return table;
}

template <typename FTable> FTable MakeHudUiPanelFTable() {
    FTable table = MakeHudUiFTableWithCommonInvalidate<FTable>();
    RECOIL_STATIC_ASSERT((sizeof(table.slots) / sizeof(table.slots[0])) > 36);
    table.slots[27] = MethodAddress(&HudUiPanel::EnableWordWrapWithRect);
    table.slots[29] = MethodAddress(&HudUiPanel::SetTextFmt);
    table.slots[32] = MethodAddress(&HudUiPanel::SetFont);
    table.slots[34] = MethodAddress(&HudUiPanel::SetTextFmtV);
    table.slots[35] = MethodAddress(&HudUiPanel::SetText);
    table.slots[36] = MethodAddress(&HudUiPanel::RebuildTextRect);
    return table;
}

HudUiCompositePanel_FTable MakeHudUiCompositePanelFTable() {
    HudUiCompositePanel_FTable table = MakeHudUiPanelFTable<HudUiCompositePanel_FTable>();
    table.slots[3] = MethodAddress(&HudUiCompositePanel::LayoutEntries);
    table.slots[29] = MethodAddress(&HudUiCompositePanel::SetTextFmt);
    table.slots[32] = MethodAddress(&HudUiCompositePanel::SetFont);
    table.slots[34] = MethodAddress(&HudUiCompositePanel::SetTextFmtV);
    table.slots[37] = MethodAddress(&HudUiCompositePanel::ScrollHistory);
    return table;
}

HudUiTripletPanel_FTable MakeHudUiTripletPanelFTable() {
    HudUiTripletPanel_FTable table = MakeHudUiFTableWithCommonInvalidate<HudUiTripletPanel_FTable>();
    table.slots[1] = MethodAddress(&HudUiTripletPanel::Draw);
    table.slots[2] = (unsigned int)(&HudUiNoOpMethodStub);
    return table;
}

HudUiMessageBoxDialog_FTable MakeHudUiMessageBoxDialogFTable() {
    HudUiMessageBoxDialog_FTable table = {0};
    table.slots[0] = MethodAddress(&HudUiBackground::Update);
    table.slots[1] = MethodAddress(&HudUiBackground::SetEnabled);
    table.slots[2] = MethodAddress(&HudUiMessageBoxDialog::ScalarDeletingDestructor);
    table.slots[3] = MethodAddress(&HudUiMessageBoxDialog::OnOk);
    table.slots[4] = MethodAddress(&HudUiMessageBoxDialog::OnCancel);
    return table;
}

HudUiZrdWidget_FTable MakeHudUiMessageBoxButtonFTable(unsigned int activateSlot) {
    HudUiZrdWidget_FTable table = MakeHudUiFTableWithCommonInvalidate<HudUiZrdWidget_FTable>();
    table.slots[0] = MethodAddress(&HudUiZrdWidget::ScalarDeletingDestructor);
    table.slots[12] = activateSlot;
    table.slots[15] = MethodAddress(&HudUiZrdWidget::ShowPreview);
    table.slots[16] = MethodAddress(&HudUiZrdWidget::HidePreview);
    table.slots[30] = MethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = MethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = (unsigned int)(&HudUiNoOpMethodStub);
    return table;
}

HudUiZrdWidget_FTable MakeHudUiCreditsButtonFTable(unsigned int activateSlot)
{
    return MakeHudUiMessageBoxButtonFTable(activateSlot);
}

HudUiCreditsPanel_FTable MakeHudUiCreditsPanelFTable()
{
    HudUiCreditsPanel_FTable table = {0};
    table.primarySlots[0] = MethodAddress(&HudUiCreditsPanel::UpdateFadeAndExit);
    table.primarySlots[1] = MethodAddress(&HudUiBackground::SetEnabled);
    table.primarySlots[2] = MethodAddress(&HudUiCreditsPanel::ScalarDeletingDestructor);

    table.SecondaryAction = MakeHudUiFTableWithCommonInvalidate<HudUiZrdWidget_FTable>();
    table.SecondaryAction.slots[0] = MethodAddress(&HudUiZrdScrollingText::ScalarDeletingDestructor);
    table.SecondaryAction.slots[9] = MethodAddress(&HudUiZrdScrollingText::Update);
    table.SecondaryAction.slots[12] = MethodAddress(&HudUiZrdScrollingText::OnActivateResetOwnerFade);
    table.SecondaryAction.slots[30] = MethodAddress(&HudUiZrdWidget::RefreshState);
    table.SecondaryAction.slots[31] = MethodAddress(&HudUiZrdScrollingText::LoadFromZrd);
    table.SecondaryAction.slots[32] = (unsigned int)(&HudUiNoOpMethodStub);
    return table;
}

HudUiBackgroundCursorWidget_FTable MakeHudUiBackgroundCursorWidgetFTable() {
    HudUiBackgroundCursorWidget_FTable table =
        MakeHudUiFTableWithCommonInvalidate<HudUiBackgroundCursorWidget_FTable>();
    table.slots[1] = MethodAddress(&HudUiBackgroundCursorWidget::Draw);
    table.slots[2] = MethodAddress(&HudUiBackgroundCursorWidget::DrawBase);
    table.slots[3] = MethodAddress(&HudUiBackgroundCursorWidget::SetPos);
    table.slots[30] =
        MethodAddress(&HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged);
    table.slots[31] = MethodAddress(&HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh);
    return table;
}

HudUiBackgroundContainer_FTable MakeHudUiBackgroundFTable() {
    HudUiBackgroundContainer_FTable table = {0};
    AssignMethodSlot(table.updateAll, &HudUiBackground::Update);
    AssignMethodSlot(table.setEnabled, &HudUiBackground::SetEnabled);
    return table;
}

HudUiBackgroundVideoWidget_FTable MakeHudUiBackgroundVideoWidgetFTable() {
    HudUiBackgroundVideoWidget_FTable table =
        MakeHudUiFTableWithCommonInvalidate<HudUiBackgroundVideoWidget_FTable>();
    table.slots[1] = MethodAddress(&HudUiBackgroundVideoWidget::Draw);
    table.slots[2] = MethodAddress(&HudUiBackgroundVideoWidget::DrawBase);
    table.slots[9] = MethodAddress(&HudUiBackgroundVideoWidget::Update);
    table.slots[0x74 / 4] = MethodAddress(&HudUiBackgroundVideoWidget::RebuildBltRect);
    return table;
}

HudUiSliderBorder_FTable MakeHudUiSliderBorderFTable() {
    HudUiSliderBorder_FTable table =
        MakeHudUiFTableWithCommonInvalidate<HudUiSliderBorder_FTable>();
    table.slots[9] = MethodAddress(&HudUiSliderBorder::Update);
    return table;
}

bool IsCallableProviderAddress(unsigned int address) {
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

    RECOIL_NOINLINE void RECOIL_THISCALL RemoveFromActiveList();
    RECOIL_NOINLINE void RECOIL_THISCALL RunImmediateAction();
    RECOIL_NOINLINE static void RECOIL_CDECL TickActiveList();
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

const HudUiCommon_FTable g_HudUiCommon_FTable = MakeHudUiCommonFTable();
const HudUiCommon_FTable g_HudUiCircle_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiCommon_FTable>();
const HudUiContainer_FTable g_HudUiContainer_FTable = MakeHudUiContainerFTable();
const HudUiBackgroundContainer_FTable g_HudUiBackgroundContainer_FTable = {0};
const HudUiBackgroundContainer_FTable g_HudUiBackground_FTable = MakeHudUiBackgroundFTable();
const HudUiWidget_FTable g_HudUiWidget_FTable = MakeHudUiWidgetFTable();
const HudUiBackgroundCursorWidget_FTable g_HudUiBackgroundCursorWidget_FTable =
    MakeHudUiBackgroundCursorWidgetFTable();
const HudUiBackgroundCursorWidget_FTable g_HudUiBackgroundCursorWidget_MemberFTable =
    MakeHudUiBackgroundCursorWidgetFTable();
const HudUiBackgroundVideoWidget_FTable g_HudUiBackgroundVideoWidget_FTable =
    MakeHudUiBackgroundVideoWidgetFTable();
const HudUiZrdWidget_FTable g_HudUiZrdWidget_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiZrdWidget_FTable>();
const HudUiZrdWidget_FTable g_HudUiCreditsButtonQueueExitOnly_Vtbl =
    MakeHudUiCreditsButtonFTable(MethodAddress(&HudUiZrdWidget::OnActivateQueueExitCurrentState));
const HudUiZrdWidget_FTable g_HudUiCreditsButtonQueueExitAndLeaveNetwork_Vtbl =
    MakeHudUiCreditsButtonFTable(MethodAddress(&HudUiCreditsQuitButton::OnActivate));
const HudUiCreditsPanel_FTable g_HudUiCreditsPanel_FTable = MakeHudUiCreditsPanelFTable();
const HudUiCheckToggleWidget_FTable g_HudUiCheckToggleWidget_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiCheckToggleWidget_FTable>();
const HudUiCycleSelectorWidget_FTable g_HudUiCycleSelectorWidget_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiCycleSelectorWidget_FTable>();
const HudUiFillBitmap_FTable g_HudUiFillBitmap_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiFillBitmap_FTable>();
const HudUiZrdWidgetEx17C_Item_FTable g_HudUiZrdWidgetEx17C_Item_FTable =
    MakeHudUiZrdWidgetEx17CItemFTable();
const HudUiZrdWidgetEx17C_FTable g_HudUiZrdWidgetEx17C_FTable =
    MakeHudUiZrdWidgetEx17CFTable();
const HudCmdBindButtonBase_FTable g_HudCmdBindButtonBase_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudCmdBindButtonBase_FTable>();
// Recovered global 0x4e5e00: HUD command-dialog mouse capture debounce frames.
int g_HudCmdMouseDebounceFrames = 0;
const HudUiMessageBoxDialog_FTable g_HudUiMessageBoxDialog_FTable =
    MakeHudUiMessageBoxDialogFTable();
const HudUiZrdWidget_FTable g_HudUiMessageBoxOkButton_Vtbl =
    MakeHudUiMessageBoxButtonFTable(MethodAddress(&HudUiMessageBoxOkButton::OnActivate));
const HudUiZrdWidget_FTable g_HudUiMessageBoxCancelButton_Vtbl =
    MakeHudUiMessageBoxButtonFTable(MethodAddress(&HudUiMessageBoxCancelButton::OnActivate));
const HudUiCounter_FTable g_HudUiCounter_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiCounter_FTable>();
const HudUiMessage_FTable g_HudUiMessage_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiMessage_FTable>();
const HudUiPanel_FTable g_HudUiPanel_FTable = MakeHudUiPanelFTable<HudUiPanel_FTable>();
const HudUiPanel_FTable g_HudUiTransitionTextPanel_FTable =
    MakeHudUiPanelFTable<HudUiPanel_FTable>();
const HudUiCompositePanel_FTable g_HudUiCompositePanel_FTable =
    MakeHudUiCompositePanelFTable();
const HudUiListSelectorItem_FTable g_HudUiListSelectorItem_FTable =
    MakeHudUiPanelFTable<HudUiListSelectorItem_FTable>();
const HudUiTextLabel_FTable g_HudUiTextLabel_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiTextLabel_FTable>();
const HudUiPanelSimple_FTable g_HudUiPanelSimple_FTable =
    MakeHudUiPanelFTable<HudUiPanelSimple_FTable>();
const HudUiBar_FTable g_HudUiBar_FTable = MakeHudUiFTableWithCommonInvalidate<HudUiBar_FTable>();
const HudUiPolyline_FTable g_HudUiPolyline_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiPolyline_FTable>();
const HudUiSliderBorder_FTable g_HudUiSliderBorder_FTable = MakeHudUiSliderBorderFTable();
const HudUiMeter_FTable g_HudUiMeter_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiMeter_FTable>();
const HudUiMeter_FTable g_HudUiMeterEx_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiMeter_FTable>();
const HudUiTimerPanelFloat_FTable g_HudUiTimerPanelFloat_FTable =
    MakeHudUiPanelFTable<HudUiTimerPanelFloat_FTable>();
const HudUiTimerPanel_FTable g_HudUiTimerPanel_FTable =
    MakeHudUiPanelFTable<HudUiTimerPanel_FTable>();
const HudUiCounterTextPanel_FTable g_HudUiCounterTextPanel_FTable =
    MakeHudUiPanelFTable<HudUiCounterTextPanel_FTable>();
const HudUiTriplet_FTable g_HudUiTriplet_FTable =
    MakeHudUiContainerLikeFTable<HudUiTriplet_FTable>();
const HudUiTextStack4_FTable g_HudUiTopMessageStack_FTable =
    MakeHudUiContainerLikeFTable<HudUiTextStack4_FTable>();
const HudUiTextStack4_FTable g_HudUiChatMessageStack_FTable =
    MakeHudUiContainerLikeFTable<HudUiTextStack4_FTable>();
const HudUiStringMenu_FTable g_HudUiStringMenu_FTable =
    MakeHudUiContainerLikeFTable<HudUiStringMenu_FTable>();
const HudUiStatsListElement_FTable g_HudUiStatsListElement_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiStatsListElement_FTable>();
const HudUiTripletPanel_FTable g_HudUiTripletPanel_FTable = MakeHudUiTripletPanelFTable();
const HudUiTextInput_FTable g_HudUiTextInput_FTable =
    MakeHudUiTextInputFTable();
const HudUiTextInput_FTable g_HudUiNumericTextInput_TextInputFTable =
    MakeHudUiTextInputFTable();
const HudUiNumericTextInput_Base_FTable g_HudUiNumericTextInput_Base_FTable =
    MakeHudUiFTableWithCommonInvalidate<HudUiNumericTextInput_Base_FTable>();
const HudUiSlot_FTable g_HudUiSlot_FTable = MakeHudUiFTableWithCommonInvalidate<HudUiSlot_FTable>();
zVidImagePartial *g_HudUiWidget_ExclusiveDrawImage = 0;
HudUiContainer g_HudUiMgr = {0};
HudUiTimerPanel *g_HudUiMgrTimerPanel = 0;
HudUiTimerPanelFloat *g_HudUiMgrTimerPanelFloat = 0;
HudUiStringMenu *g_HudUiMgrStringMenu = 0;
HudUiStatsListElement *g_HudUiMgrStatsList = 0;
zSndSample *g_HudUi_PowerupSample = 0;
unsigned char g_HudUi_PowerupSampleInitFlags = 0;
zVidImagePartial *g_HudUiMgrReticleImages[3] = {0};
zVidImagePartial *g_HudUiMgrSensorTargetMarkerImages[5] = {0};
HudUiNanitePanel g_HudUiMgrNanitePanel;
HudUiMessage g_HudUiMgrMessages[10] = {0};
int g_HudUiMgrActiveWeaponMessageIndex = 0;
int g_HudUiMgrActiveWeaponSideIndex = 0;
HudUiCounter g_HudUiMgrModeCounters[4] = {0};
HudUiSlot g_HudUiMgrWeaponSlots[32] = {0};
HudUiSlot *g_HudUiMgrSensorTrackedProgressSlot = 0;
int g_HudUiMgrSensorRoundRobinTrackIndex = 0;
HudUiRect g_HudUiMgrSensor_FxRectScratch = {0};
HudUiMeter g_HudUiMgrObjectiveMeter = {0};
int g_HudUiMgrActiveModeCounterIndex = 0;
int g_HudUiMgrHudLayoutsInitialized = 0;
int g_HudUiMgrHudLoaded = 0;
int g_HudUiMgrLayoutDelayFrames = 0;
int g_HudUiMgrSensorTargetMarkerCount = 0;
int g_HudUiMgrWeaponState = 0;
int g_HudUiMgrHudOriginX = 0;
int g_HudUiMgrHudOriginY = 0;
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
HudUiWidget g_HudUiMgrReticleWidget = {0};
HudLayoutBase *g_HudUiMgrCurrentLayout = 0;
float g_HudUiLoadingCheckpointRawProgress[19] = {0};
float g_HudUiLoadingCheckpointProgress[19] = {0};
float g_HudUiLoadingCheckpointProgressScale = 0.0186219737f;
unsigned int g_HudUiLoadingCheckpointMaxIndex = 0;
unsigned int g_HudUiLoadingCheckpointCurrentIndex = 0;
float g_HudUiLoadingCheckpointCurrentProgress = 0.0f;
HudLayoutHW g_HudLayoutHW = {0};
HudLayoutBase g_HudLayoutSW = {0};
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
HudUiWidget g_HudUiMgrSensorPanel = {0};
HudUiWidget g_HudUiMgrObjectiveWidget = {0};
HudUiBar g_HudUiMgrObjectiveBar = {0};
HudUiWidget g_HudUiMgrObjectiveSensorRect = {0};
HudUiWidget g_HudUiMgrSensorOverlay = {0};
HudUiMeter g_HudUiMgrSensorMeter = {0};
HudUiPanel *g_HudUiMgrObjectiveSummaryTextPanel = 0;
HudUiPanel *g_HudUiMgrObjectiveDescTextPanel = 0;
HudUiPanel *g_HudUiMgrObjectiveLabelTextPanel = 0;
HudUiCounterTextPanel *g_HudUiMgrObjectiveCounterTextPanel = 0;
HudUiTextInput g_HudUiMgrObjectiveChatComposeTextInput = {0};
int g_HudUi_AuxOverlayEnabled = 0;
CString g_HudUiTripletWndClassName("");

// Reimplements 0x40ec90: HudLayoutBase::Shutdown_Stub
void RECOIL_CDECL HudLayoutBase::Shutdown_Stub() {
    HudUiNoOpMethodStub(&g_HudUiMgrShieldMessageWidget->widget);
}

// Reimplements 0x40d3b0: HudLayoutBase::Destructor
void RECOIL_THISCALL HudLayoutBase::Destructor() {
    ((HudUiWidget *)((unsigned char *)(this) + 0x30))->DestructorCore();
    ((HudUiContainer *)(this))->DestructorCore();
}

// Reimplements 0x412bd0: HudLayoutBase::SetActiveNoOp
int RECOIL_THISCALL HudLayoutBase::SetActiveNoOp(int) {
    return 1;
}

// Reimplements 0x412be0: HudLayoutBase::UpdateAll
void RECOIL_THISCALL HudLayoutBase::UpdateAll(float deltaSeconds) {
    ((HudUiContainer *)(this))->UpdateAll(deltaSeconds);
}

// Reimplements 0x412bf0: HudLayoutBase::Enable
void RECOIL_THISCALL HudLayoutBase::Enable() {
    typedef int (RECOIL_THISCALL HudLayoutBase::*SetActiveMethod)(int active);
    union {
        unsigned int slot;
        SetActiveMethod method;
    } dispatch = {ftable->slots[1]};
    (this->*dispatch.method)(1);
}

// Reimplements 0x412c00: HudLayoutBase::Disable
void RECOIL_THISCALL HudLayoutBase::Disable() {
    typedef int (RECOIL_THISCALL HudLayoutBase::*SetActiveMethod)(int active);
    union {
        unsigned int slot;
        SetActiveMethod method;
    } dispatch = {ftable->slots[1]};
    (this->*dispatch.method)(0);
}

// Reimplements 0x412c10: HudLayoutSW::LoadTypeIFromZarRoot
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudLayoutBase::LoadTypeIFromZarRoot(zReader::Node *parentNode) {
    zReader::Node *const typeINode = zReader_GetNamedNode(parentNode, "TYPEI");
    if (typeINode == 0) {
        return;
    }

    HudUiLayoutNode::ReadRectOffsetAndSize(&typeINode->value.nodes[1], &layoutRect, 0,
                                           0, 0);
    activeRect = layoutRect;
}

namespace HudLayout {
// Reimplements 0x412db0: HudLayout::ApplyViewportRect
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ApplyViewportRect(HudUiRect *activeRect) {
    const int replicateMode = zOpt::GetReplicateMode();
    const int left = activeRect->left;
    const int top = activeRect->top;

    zOpt::DisplaySection_SetPosition(left, top);

    int renderX = left;
    int renderY = top;
    if (replicateMode != 0) {
        renderX = (left - (left >> 31)) >> 1;
        renderY = (top - (top >> 31)) >> 1;
    }

    zOpt::RenderSection_SetPosition(renderX, renderY);

    int width = activeRect->right - left;
    int height = activeRect->bottom - top;
    zOpt::DisplaySection_SetSize(width, height);

    const float viewportWidth = (float)(width);
    const float viewportHeight = (float)(height);

    if (replicateMode != 0) {
        width = (width - (width >> 31)) >> 1;
        height = (height - (height >> 31)) >> 1;
    }

    zOpt::RenderSection_SetSize(width, height);

    zClass_NodePartial *const camera = g_HudSensorTracker.cameraNode;
    if (camera != 0) {
        float fovX = 0.0f;
        float fovY = 0.0f;
        zClass_Camera::gwCameraGetFOV(camera, &fovX, &fovY);
        fovY = viewportHeight / viewportWidth * fovX;
        zClass_Camera::gwCameraSetFOV(camera, fovX, fovY);
    }

    zOpt_ViewRectSection *const renderSection = zOpt::GetRenderSection();
    HudUiMgr::OnViewportChanged((const HudUiRect *)(zOpt::GetDisplaySection()),
                                (const HudUiRect *)(renderSection));
    return 1;
}
} // namespace HudLayout

// Reimplements 0x413080: HudLayoutHW::ReleaseImages
void RECOIL_THISCALL HudLayoutHW::ReleaseImages() {
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
void RECOIL_THISCALL HudLayoutHW::OnActivated() {
    HudUi::SetInvalidateMode(zOpt::GetReplicateMode() == 0 ? 1 : 0);

    g_HudUiMgr.SetChildFlags(0x0e);
    ((HudUiContainer *)(this))->SetChildFlags(0x0e);

    unsigned char *const bytes = (unsigned char *)(this);
    *(unsigned int *)(bytes + 0x288) =
        (unsigned int)(bytes[0x288] & 0x10u);

    g_HudUiMgrObjectiveWidget.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveWidget.flags) &
                                   0x10u);
    g_HudUiMgrObjectiveMeter.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveMeter.flags) &
                                   0x10u);
    ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->flags =
        (unsigned int)((unsigned char)(
                                       ((HudUiElement *)(
                                           g_HudUiMgrObjectiveLabelTextPanel))->flags) &
                                   0x10u);
    g_HudUiMgrSensorOverlay.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrSensorOverlay.flags) &
                                   0x10u);

    g_HudUiMgrStatsList->triplet->RebuildDisplay();

    {
    for (int index = 1; index < 10; ++index) {
        HudUiMessage &message = g_HudUiMgrMessages[index];
        if (message.widget.image != 0) {
            message.widget.flags =
                (unsigned int)((unsigned char)(message.widget.flags) &
                                           0x10u);
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
    g_HudSensorTracker.SetBounds(&outerRect, innerRect);

    HudUiWidget *const widget1 = (HudUiWidget *)(bytes + 0xec);
    HudUiWidget *const widget2 = (HudUiWidget *)(bytes + 0x1b4);

    zVidImagePartial *widget1Image = widget1ImageDefault;
    zVidImagePartial *widget2Image = widget2ImageDefault;
    if (layout->activeRect.right == 0x320) {
        widget1Image = widget1Image320;
        widget2Image = widget2Image320;
    } else if (layout->activeRect.right == 0x400) {
        widget1Image = widget1Image400;
        widget2Image = widget2Image400;
    }

    widget2->SetImageBorrowedAndInvalidate(widget2Image);
    widget1->SetImageBorrowedAndInvalidate(widget1Image);

    if (g_HudUiMgrHudLayoutsInitialized != 0) {
        widget2->InvalidateRect(&g_HudUiMgrViewRect);
    }
}

// Reimplements 0x4132b0: HudLayoutHW::UpdateObjectiveDirtyRect
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_THISCALL HudLayoutHW::UpdateObjectiveDirtyRect() {
    zVidImagePartial *const image = g_HudUiMgrObjectiveWidget.image;
    const int width = image != 0 ? image->width : 0;

    typedef int (RECOIL_THISCALL *GetCoordFn)(HudUiWidget * self);
    const int centerX =
        ((GetCoordFn)(g_HudUiMgrObjectiveWidget.ftable->slots[25]))(
            &g_HudUiMgrObjectiveWidget);
    const int centerY =
        ((GetCoordFn)(g_HudUiMgrObjectiveWidget.ftable->slots[26]))(
            &g_HudUiMgrObjectiveWidget);

    const int height = image != 0 ? image->height : 0;
    HudUiRect dirtyRect;
    dirtyRect.left = centerX + width;
    dirtyRect.top = centerY;
    dirtyRect.right = g_HudUiMgrObjectiveWidgetRightX;
    dirtyRect.bottom = centerY + height;

    HudUiWidget *const widget2 = (HudUiWidget *)((unsigned char *)(this) +
                                                          0x1b4);
    widget2->InvalidateRect(&dirtyRect);
    ((HudUiElement *)(&g_HudUiMgrNanitePanel))->Invalidate();
    ((HudUiTripletPanel *)(&g_HudUiMgrNanitePanel))->Draw();
}

// Reimplements 0x4130d0: HudLayoutHW::SetActive
// (D:\Proj\Battlesport\hud.cpp)
int RECOIL_THISCALL HudLayoutHW::SetActive(int active) {
    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionResetFrame();
    }

    HudLayoutBase *const layout = (HudLayoutBase *)(this);
    layout->activeRect.right = zVideo::GetPrimarySurfaceWidth();
    layout->activeRect.bottom = layout->layoutRect.bottom + g_HudUiMgrHudOriginY;
    HudLayout::ApplyViewportRect(&layout->activeRect);

    unsigned char *const bytes = (unsigned char *)(this);
    HudUiWidget *const widget1 = (HudUiWidget *)(bytes + 0xec);
    HudUiWidget *const widget2 = (HudUiWidget *)(bytes + 0x1b4);

    if (active == 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))->SetBltSourceAndClipRect(0, 0);
        ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetBltSourceAndClipRect(0, 0);
        ((HudUiElement *)(&g_HudUiMgrNanitePanel))->SetBltSourceAndClipRect(0, 0);

        {
        for (int index = 0; index < 10; ++index) {
            HudUiMessage &message = g_HudUiMgrMessages[index];
            ((HudUiElement *)(&message.base))->SetBltSourceAndClipRect(0, 0);
            ((HudUiElement *)(&message.panel))->SetBltSourceAndClipRect(0, 0);
        }
        }

        const int clearState = zVideo::ExchangeClearScreenBufferEnabled(1);
        zVideo::CallClearPrimarySurfaceAndZBuffer(0);
        zVideo::ExchangeClearScreenBufferEnabled(clearState);
        return 1;
    }

    layout->ftable->OnActivated(layout);

    zVidImagePartial *const widget1Image = widget1->image;
    zVidImagePartial *const widget2Image = widget2->image;
    ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))->SetBltSourceAndClipRect(widget1Image, 0);
    ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetBltSourceAndClipRect(widget1Image, 0);

    {
    for (int index = 1; index < 10; ++index) {
        HudUiMessage &message = g_HudUiMgrMessages[index];
        ((HudUiElement *)(&message.base))->SetBltSourceAndClipRect(widget2Image, 0);
        ((HudUiElement *)(&message.panel))->SetBltSourceAndClipRect(widget2Image, 0);
    }
    }

    ((HudUiElement *)(&g_HudUiMgrNanitePanel))->SetBltSourceAndClipRect(widget2Image, 0);
    zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);

    if (g_HudUiMgr.enabled != 0 && zVid::GetAccelerationOption() == 0) {
        HudUiRect occluderRect;
        occluderRect.left = g_HudUiMgrSensorBlock.sensorViewportRect.left;
        occluderRect.top = g_HudUiMgrSensorBlock.sensorViewportRect.top;
        occluderRect.right = g_HudUiMgrSensorBlock.sensorViewportRect.right;
        occluderRect.bottom = g_HudUiMgrHudRect.bottom;

        float nearClip = 0.0f;
        float farClip = 0.0f;
        zClass_Camera::gwCameraGetNearFarClip(g_MainCamera, &nearClip, &farClip);
        zRndr::SpanOcclusionSubmitOccluderRect(&occluderRect, zOpt::GetReplicateMode(),
                                               1.0f / nearClip);
    }

    return 1;
}

// Reimplements 0x412f70: HudLayoutHW::LoadTypeIIFromZarRoot
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
HudLayoutHW::LoadTypeIIFromZarRoot(zReader::Node *parentNode) {
    zReader::Node *const typeIINode = zReader_GetNamedNode(parentNode, "TYPEII");
    if (typeIINode == 0) {
        return 1;
    }

    zReader::Node *const typeIIPayload = typeIINode->value.nodes;
    unsigned char *const bytes = (unsigned char *)(this);
    HudLayoutBase *const layout = (HudLayoutBase *)(this);
    HudUiWidget *const widget1 = (HudUiWidget *)(bytes + 0xec);
    HudUiWidget *const widget2 = (HudUiWidget *)(bytes + 0x1b4);
    HudUiWidget *const widget3 = (HudUiWidget *)(bytes + 0x27c);

    HudUiLayoutNode::ReadRectOffsetAndSize(&typeIIPayload[1], &layout->layoutRect, 0,
                                           0, 0);
    layout->activeRect = layout->layoutRect;

    HudUiLayoutNode::ApplyImageWidget(&typeIIPayload[2], widget1, 0, 0, 0, 0,
                                      0);
    HudUiLayoutNode::ApplyImageWidget(&typeIIPayload[3], widget3, 0, g_HudUiMgrHudOriginY,
                                      0, 0, 0);
    HudUiLayoutNode::ApplyImageWidget(&typeIIPayload[4], widget2, 0, g_HudUiMgrHudOriginY,
                                      0, 0, 0);

    zReader::Node *const imageNames = typeIIPayload[5].value.nodes;
    widget1ImageDefault = widget1->image;
    widget1Image320 = zImage::TexDir_FindOrCreateByPath(imageNames[1].value.str);
    widget1Image400 = zImage::TexDir_FindOrCreateByPath(imageNames[2].value.str);
    widget2ImageDefault = widget2->image;
    widget2Image320 = zImage::TexDir_FindOrCreateByPath(imageNames[3].value.str);
    widget2Image400 = zImage::TexDir_FindOrCreateByPath(imageNames[4].value.str);

    return 1;
}

// Reimplements 0x413540: HudLayoutHW::Enable
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_THISCALL HudLayoutHW::Enable() {
    g_HudUiMgr.SetChildFlags(0x0e);
    ((HudUiContainer *)(this))->SetChildFlags(0x0e);

    unsigned char *const bytes = (unsigned char *)(this);
    *(unsigned int *)(bytes + 0x288) =
        (unsigned int)(bytes[0x288] & 0x10u);

    g_HudUiMgrObjectiveWidget.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveWidget.flags) &
                                   0x10u);
    g_HudUiMgrObjectiveMeter.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveMeter.flags) &
                                   0x10u);
    ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->flags =
        (unsigned int)((unsigned char)(
                                       ((HudUiElement *)(
                                           g_HudUiMgrObjectiveLabelTextPanel))->flags) &
                                   0x10u);
    g_HudUiMgrSensorOverlay.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrSensorOverlay.flags) &
                                   0x10u);

    {
    for (int index = 1; index < 10; ++index) {
        HudUiMessage &message = g_HudUiMgrMessages[index];
        if (message.widget.image != 0) {
            message.widget.flags =
                (unsigned int)((unsigned char)(message.widget.flags) &
                                           0x10u);
        }
    }
    }

    typedef void (RECOIL_THISCALL *SetEnabledFn)(HudLayoutHW * self, int enabled);
    ((SetEnabledFn)(((HudLayoutBase *)(this))->ftable->slots[1]))(this,
                                                                                              1);
}

// Reimplements 0x4135f0: HudLayoutHW::Disable
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_THISCALL HudLayoutHW::Disable() {
    typedef void (RECOIL_THISCALL *SetEnabledFn)(HudLayoutHW * self, int enabled);
    ((SetEnabledFn)(((HudLayoutBase *)(this))->ftable->slots[1]))(this,
                                                                                              0);
}

extern "C" {
unsigned int g_HudUi_InvalidateMask = 0;
}

namespace {
const char kNumericTextInputAcceptedRawKeyChars[] = "0123456789.-\x1b\r\x08\x7f\x02\x06";

#if defined(_MSC_VER) && _MSC_VER < 1200
// VC5 misparses explicit function-template calls such as FieldAt<unsigned int>(...).
// Keep the same call-site spelling for first-pass VC5 verification without changing
// VC6 or modern compiler codegen.
template <typename T> class FieldAt {
  public:
    FieldAt(void *base, size_t offset)
        : address((T *)((unsigned char *)(base) + offset)) {}

    FieldAt(const void *base, size_t offset)
        : address((T *)((const unsigned char *)(base) + offset)) {}

    operator T &() {
        return *address;
    }

    T *operator&() const { return address; }

    FieldAt &operator=(const T &value) {
        *address = value;
        return *this;
    }

    FieldAt &operator|=(const T &value) {
        *address |= value;
        return *this;
    }

    FieldAt &operator+=(const T &value) {
        *address += value;
        return *this;
    }

    FieldAt &operator-=(const T &value) {
        *address -= value;
        return *this;
    }

  private:
    T *address;
};
#else
template <typename T> T &FieldAt(void *base, size_t offset) {
    return *(T *)((unsigned char *)(base) + offset);
}

template <typename T> const T &FieldAt(const void *base, size_t offset) {
    return *(const T *)((const unsigned char *)(base) + offset);
}
#endif

HudUiPanel *NewSimplePanel(int fontSize, int fontWeight) {
    HudUiPanel *const panel = (HudUiPanel *)(::operator new(0x2a4));
    ((HudUiPanelSimple *)(panel))->Constructor(0, 0, 0);
    panel->SetFont("Arial", fontSize, 0x1f4, fontWeight, 0, 0, 2);
    ((HudUiElement *)(panel))->SetVisible(0);
    return panel;
}

size_t HudUiTripletEntryCount(const HudUiTripletEntries &entries) {
    return (size_t)(((HudUiTripletEntries *)(&entries))->GetCount());
}

size_t HudUiTripletEntryCapacity(const HudUiTripletEntries &entries) {
    if (entries.begin == 0) {
        return 0;
    }

    return (size_t)(entries.cap - entries.begin);
}

int HudUiTripletEntrySortKey(const HudUiScoreboardEntry &entry) {
    return entry.score + entry.lapCount * 1000;
}

namespace HudUiListMenuEntry {

// Reimplements 0x40d220: HudUiListMenuEntry::CompareSortKey
// (D:\Proj\Battlesport\HudUiListMenu.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
CompareSortKey(const HudUiScoreboardEntry *entryA, const HudUiScoreboardEntry *entryB) {
    const unsigned int keyA = (unsigned int)(HudUiTripletEntrySortKey(*entryA));
    const unsigned int keyB = (unsigned int)(HudUiTripletEntrySortKey(*entryB));
    if (keyA != keyB) {
        return keyB < keyA ? 1 : 0;
    }

    return (unsigned int)(entryB->playerKey) <
                   (unsigned int)(entryA->playerKey)
               ? 1
               : 0;
}

bool EntryComesBefore(const HudUiScoreboardEntry &lhs, const HudUiScoreboardEntry &rhs) {
    return CompareSortKey(&lhs, &rhs) != 0;
}

// Reimplements 0x414930: HudUiListMenuEntry::InsertPivotIntoSortedPrefix
// (D:\Proj\Battlesport\HudUiListMenu.cpp)
RECOIL_NOINLINE void InsertPivotIntoSortedPrefix(HudUiScoreboardEntry *slot,
                                                 const HudUiScoreboardEntry &pivot) {
    HudUiScoreboardEntry *insertSlot = slot;
    HudUiScoreboardEntry *previousEntry = insertSlot - 1;
    while (EntryComesBefore(pivot, *previousEntry)) {
        *insertSlot = *previousEntry;
        insertSlot = previousEntry;
        --previousEntry;
    }

    *insertSlot = pivot;
}

// Reimplements 0x414980: HudUiListMenuEntry::InsertionSortRange
// (D:\Proj\Battlesport\HudUiListMenu.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
InsertionSortRange(HudUiScoreboardEntry *begin, HudUiScoreboardEntry *end, int) {
    if (begin == end) {
        return;
    }

    HudUiScoreboardEntry *current = begin + 1;
    while (current != end) {
        HudUiScoreboardEntry candidate = *current;
        if (EntryComesBefore(candidate, *begin)) {
            HudUiScoreboardEntry *shiftCursor = current;
            while (shiftCursor != begin) {
                *shiftCursor = *(shiftCursor - 1);
                --shiftCursor;
            }

            *begin = candidate;
        } else {
            InsertPivotIntoSortedPrefix(current, candidate);
        }

        ++current;
    }
}

HudUiScoreboardEntry *MedianOfThree(HudUiScoreboardEntry *first, HudUiScoreboardEntry *middle,
                                    HudUiScoreboardEntry *last) {
    if (EntryComesBefore(*first, *middle)) {
        if (EntryComesBefore(*middle, *last)) {
            return middle;
        }

        return EntryComesBefore(*first, *last) ? last : first;
    }

    if (EntryComesBefore(*first, *last)) {
        return first;
    }

    return EntryComesBefore(*middle, *last) ? last : middle;
}

// Reimplements 0x414710: HudUiListMenuEntry::SortRange
// (D:\Proj\Battlesport\HudUiListMenu.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SortRange(HudUiScoreboardEntry *begin, HudUiScoreboardEntry *end, int unusedFlags) {
    InsertionSortRange(begin, end, unusedFlags);
}

} // namespace HudUiListMenuEntry

void HudUiTripletInsertionSort(HudUiScoreboardEntry *begin, HudUiScoreboardEntry *end) {
    if (begin == 0 || begin == end) {
        return;
    }

    HudUiListMenuEntry::InsertionSortRange(begin, end, 0);
}

void HudUiTripletEnsureCapacity(HudUiTripletEntries &entries, size_t neededCount) {
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

    HudUiScoreboardEntry *const newBegin = (HudUiScoreboardEntry *)(
        ::operator new(newCapacity * sizeof(HudUiScoreboardEntry)));

    HudUiTripletEntries::CopyRange(entries.begin, entries.end, newBegin);

    ::operator delete(entries.begin);
    entries.begin = newBegin;
    entries.end = newBegin + count;
    entries.cap = newBegin + newCapacity;
}

void HudUiTripletSetPanelTextColor(HudUiPanel *panel, unsigned int color) {
    FieldAt<unsigned int>(panel, 0x14c) = color;
    FieldAt<unsigned int>(panel, 0x150) = color;
    FieldAt<unsigned int>(panel, 0x270) = 1;
}

void HudUiTripletSetPanelVisible(HudUiPanel *panel, int visible) {
    ((HudUiElement *)(panel))->SetVisible(visible);
}

void HudUiTripletPrepareCell(HudUiTriplet *triplet, HudUiPanel *panel, unsigned int color) {
    HudUiTripletSetPanelTextColor(panel, color);
    HudUiElement *const element = (HudUiElement *)(panel);
    element->flags = (element->flags & 0x10u) | 0x0cu;
    panel->SetFont("Arial", triplet->fontSize, 0x1f4, triplet->fontWeight, 0, 0, 2);
}

template <typename T> T *AllocateHudObject() {
    return (T *)(::operator new(sizeof(T)));
}

template <typename T> zZbdSectionCallback ZbdCallbackPtr(T callback) {
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

    storage->Constructor(0, 0, 0);
    return (HudUiPanel *)(storage);
}

HudUiPanel *TextStackLineAt(HudUiTextStack4 *stack, int index) {
    return (HudUiPanel *)(&stack->lines[index][0]);
}

void HudUiVirtualSetTextFmtEmpty(HudUiPanel *panel) {
    typedef void (RECOIL_CDECL *SetTextFmtFn)(HudUiPanel * self, const char *format, ...);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(panel);
    ((SetTextFmtFn)(ftable->slots[0x74 / 4]))(panel, "");
}

void HudUiPanelVirtualSetTextFmtRequired(HudUiPanel *panel, const char *text) {
    typedef void (RECOIL_CDECL *SetTextFmtFn)(HudUiPanel * self, const char *format, ...);

    const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(panel);
    ((SetTextFmtFn)(ftable->slots[0x74 / 4]))(panel, text);
}

const char kHudUiMessageClearSpecialToken165[] = "\xa5";
const float kHudUiMessageClearSpecialTokenValue = 123456792.0f;

void HudUiVirtualSetContainerEnabled(HudUiContainer *container, int enabled) {
    (container->*(container->vptr->setEnabled))(enabled);
}

void HudUiVirtualInvalidate(void *element);

void HudUiVirtualSetVisibleRequired(void *element, int visible) {
    typedef void (RECOIL_THISCALL *SetVisibleFn)(void *self, int visible);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    ((SetVisibleFn)(ftable->slots[24]))(element, visible);
}

void HudUiVirtualSetPosRequired(void *element, int x, int y) {
    typedef void (RECOIL_THISCALL *SetPosFn)(void *self, int x, int y);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    ((SetPosFn)(ftable->slots[3]))(element, x, y);
}

void HudUiVirtualSetVisible(void *element, int visible) {
    typedef void (RECOIL_THISCALL *SetVisibleFn)(void *self, int visible);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    if (ftable != 0 && ftable->slots[24] != 0) {
        ((SetVisibleFn)(ftable->slots[24]))(element, visible);
        return;
    }

    HudUiElement *const base = (HudUiElement *)(element);
    if (visible != 0) {
        base->flags &= 0xffffffefu;
    } else {
        base->flags |= 0x10;
    }

    HudUiVirtualInvalidate(base);
}

void HudUiVirtualInvalidate(void *element) {
    typedef void (RECOIL_THISCALL *InvalidateFn)(void *self);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    if (ftable != 0 && ftable->slots[8] != 0) {
        ((InvalidateFn)(ftable->slots[8]))(element);
        return;
    }

    ((HudUiElement *)(element))->Invalidate();
}

void HudUiVirtualInvalidateRequired(void *element) {
    typedef void (RECOIL_THISCALL *InvalidateFn)(void *self);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    ((InvalidateFn)(ftable->slots[8]))(element);
}

void HudUiVirtualDrawRequired(void *element) {
    typedef void (RECOIL_THISCALL *DrawFn)(void *self);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    ((DrawFn)(ftable->slots[2]))(element);
}

static void HudUiPanelVirtualSetTextFmtStringRequired(void *panel, const char *text)
{
    typedef void (RECOIL_CDECL *SetTextFmtFn)(void *self, const char *format, ...);

    const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(panel);
    ((SetTextFmtFn)(ftable->slots[0x74 / 4]))(panel, "%s", text);
}

static void HudUiPanelVirtualSetTextFmtEmptyRequired(void *panel)
{
    typedef void (RECOIL_CDECL *SetTextFmtFn)(void *self, const char *format, ...);

    const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(panel);
    ((SetTextFmtFn)(ftable->slots[0x74 / 4]))(panel, "");
}

void HudUiVirtualSetClipRect(void *element, const HudUiRect *rect) {
    typedef void (RECOIL_THISCALL *SetClipRectFn)(void *self, const HudUiRect *rect);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    if (ftable != 0 && ftable->slots[7] != 0) {
        ((SetClipRectFn)(ftable->slots[7]))(element, rect);
        return;
    }

    ((HudUiElement *)(element))->SetClipRect(rect);
}

int HudUiVirtualGetX(void *element) {
    typedef int (RECOIL_THISCALL *GetCoordFn)(void *self);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    if (ftable != 0 && ftable->slots[25] != 0) {
        return ((GetCoordFn)(ftable->slots[25]))(element);
    }

    return ((HudUiElement *)(element))->x;
}

int HudUiVirtualGetXRequired(void *element) {
    typedef int (RECOIL_THISCALL *GetCoordFn)(void *self);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    return ((GetCoordFn)(ftable->slots[25]))(element);
}

int HudUiVirtualGetY(void *element) {
    typedef int (RECOIL_THISCALL *GetCoordFn)(void *self);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    if (ftable != 0 && ftable->slots[26] != 0) {
        return ((GetCoordFn)(ftable->slots[26]))(element);
    }

    return ((HudUiElement *)(element))->y;
}

int HudUiVirtualGetYRequired(void *element) {
    typedef int (RECOIL_THISCALL *GetCoordFn)(void *self);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(element);
    return ((GetCoordFn)(ftable->slots[26]))(element);
}

void HudUiVirtualSetText(void *panel, const char *text) {
    typedef void (RECOIL_THISCALL *SetTextFn)(void *self, const char *text);

    const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(panel);
    if (ftable != 0 && ftable->slots[35] != 0) {
        ((SetTextFn)(ftable->slots[35]))(panel, text);
        return;
    }

    char *const textBuffer = &FieldAt<char>(panel, 0x34);
    strncpy(textBuffer, text, 0x100);
    textBuffer[0xff] = '\0';
}

void HudUiVirtualSetTextRequired(void *panel, const char *text) {
    typedef void (RECOIL_THISCALL *SetTextFn)(void *self, const char *text);

    const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(panel);
    ((SetTextFn)(ftable->slots[35]))(panel, text);
}

void HudUiPanelVirtualRebuildTextRect(HudUiPanel *panel) {
    typedef void (RECOIL_THISCALL *RebuildFn)(HudUiPanel * self);

    const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(panel);
    if (ftable != 0 && ftable->slots[36] != 0) {
        ((RebuildFn)(ftable->slots[36]))(panel);
        return;
    }

    panel->UpdateTextBoundsFromContent();
}

void HudUiPanelVirtualRebuildTextRectRequired(HudUiPanel *panel) {
    typedef void (RECOIL_THISCALL *RebuildFn)(HudUiPanel * self);

    const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(panel);
    ((RebuildFn)(ftable->slots[36]))(panel);
}

void HudUiPanelVirtualSetFontRequired(HudUiPanel *panel, const char *faceName, int height,
                                      int weight, int width, int italic,
                                      int charSet, int pitchAndFamily) {
    typedef void (RECOIL_THISCALL *SetFontFn)(HudUiPanel * self, const char *faceName, int height, int weight,
        int width, int italic, int charSet, int pitchAndFamily);

    const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(panel);
    ((SetFontFn)(ftable->slots[32]))(panel, faceName, height, weight, width, italic,
                                                   charSet, pitchAndFamily);
}

void HudUiPanelVirtualDrawBase(HudUiPanel *panel) {
    typedef void (RECOIL_THISCALL *DrawFn)(HudUiPanel * self);

    const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(panel);
    if (ftable != 0 && ftable->slots[2] != 0) {
        ((DrawFn)(ftable->slots[2]))(panel);
    }
}

int HudUiPanelTextWidth(HudUiPanel *panel) {
    if (FieldAt<unsigned int>(panel, 0x270) != 0) {
        HudUiPanelVirtualRebuildTextRect(panel);
    }

    return FieldAt<int>(panel, 0x25c);
}

void HudUiSetPanelVectorVisible(HudUiPanelPtrVector &panels, int visible) {
    for (HudUiPanel **it = panels.begin; it != panels.end; ++it) {
        HudUiVirtualSetVisibleRequired(*it, visible);
    }
}

zReader::Node *ZrdArrayBase(zReader::Node *node) {
    if (node == 0 || node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    return node->value.nodes;
}

int ZrdArrayCount(zReader::Node *arrayBase) {
    return arrayBase != 0 ? arrayBase[0].value.i32 : 0;
}

zReader::Node *ZrdArrayItem(zReader::Node *arrayBase, int index) {
    return arrayBase != 0 ? &arrayBase[index] : 0;
}

const char *ZrdArrayString(zReader::Node *arrayBase, int index) {
    zReader::Node *const item = ZrdArrayItem(arrayBase, index);
    return item != 0 && item->type == zReader::ZRDR_NODE_STRING ? item->value.str : 0;
}

int ZrdArrayInt(zReader::Node *arrayBase, int index, int fallback) {
    zReader::Node *const item = ZrdArrayItem(arrayBase, index);
    return item != 0 && item->type == zReader::ZRDR_NODE_INT ? item->value.i32 : fallback;
}

float ZrdArrayFloat(zReader::Node *arrayBase, int index, float fallback) {
    zReader::Node *const item = ZrdArrayItem(arrayBase, index);
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
RECOIL_NOINLINE int RECOIL_THISCALL HudUiTripletEntries::GetCount() {
    if (begin == 0) {
        return 0;
    }

    return (int)(end - begin);
}

// Reimplements 0x4146a0: HudUiTripletEntries::CopyRange
RECOIL_NOINLINE HudUiScoreboardEntry *RECOIL_STDCALL
HudUiTripletEntries::CopyRange(HudUiScoreboardEntry *sourceBegin,
                               HudUiScoreboardEntry *sourceEnd,
                               HudUiScoreboardEntry *dest) {
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
RECOIL_NOINLINE void RECOIL_STDCALL
HudUiTripletEntries::FillN(HudUiScoreboardEntry *dest, unsigned int count,
                           const HudUiScoreboardEntry *sourceValue) {
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
RECOIL_NOINLINE int RECOIL_FASTCALL ReadRect(zReader::Node *node, HudUiRect *outRect) {
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
RECOIL_NOINLINE int RECOIL_FASTCALL
ReadInt3(zReader::Node *node, int *out0, int *out1, int *out2) {
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
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyCornerTextQuad(zReader::Node *node, HudUiBar *target, const int *offsetXY,
                    HudUiRect *outRect) {
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
    target->SetPointXY(0, leftF, topF);
    target->SetPointXY(1, leftF, bottomF);
    target->SetPointXY(2, rightF, bottomF);
    target->SetPointXY(3, rightF, topF);

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
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyMeterQuad(zReader::Node *node, HudUiMeter *target, int xBase, int yBase,
               const int *offsetXY, HudUiRect *outRect) {
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
    bar->SetPointXY(0, (float)(left), (float)(topY));
    bar->SetPointXY(1, (float)(left), (float)(height + topY));
    bar->SetPointXY(2, (float)(width + left + 1), (float)(height + topY));
    bar->SetPointXY(3, (float)(width + left + 1), (float)(topY));

    target->fillPixelsMax = height;
    target->meterFlags = (unsigned int)(width);
    return 1;
}

// Reimplements 0x413a10: HudUiLayoutNode::ReadRectOffsetAndSize
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ReadRectOffsetAndSize(zReader::Node *node, HudUiRect *outRect, const int *offsetXY,
                      int *outWidth, int *outHeight) {
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
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyTextLabel(zReader::Node *layoutNode, HudUiPanel *target, int baseX,
               int baseY, const int *offsetXY) {
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

    const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(target);
    typedef void (RECOIL_THISCALL *SetPosFn)(HudUiPanel * self, int x, int y);
    typedef void (RECOIL_CDECL *SetTextFmtFn)(HudUiPanel * self, const char *format, ...);
    ((SetPosFn)(ftable->slots[0x0c / 4]))(target, x, y);
    ((SetTextFmtFn)(ftable->slots[0x74 / 4]))(target, text != 0 ? text : "");
    return 1;
}

// Reimplements 0x413d30: HudUiLayoutNode::ApplyImageWidget
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
ApplyImageWidget(zReader::Node *layoutNode, HudUiWidget *widget, int baseX,
                 int baseY, const int *anchorOrNull,
                 zVidImagePartial *preloadedImageOrNull, HudUiRect *outRectOrNull) {
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
        visibleState =
            (unsigned short)(strcmp(payload[4].value.str, "TRUE") == 0 ? 1 : 0);
        centerImage = strcmp(payload[5].value.str, "TRUE") == 0 ? 1 : 0;
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

    typedef void (RECOIL_THISCALL *SetPosFn)(HudUiWidget * self, int x, int y);
    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiWidget * self);
    ((SetPosFn)(widget->ftable->slots[3]))(widget, x, y);
    widget->imageStateWord = (widget->imageStateWord & 0xffff0000u) | visibleState;
    ((InvalidateFn)(widget->ftable->slots[8]))(widget);

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

template <typename T> T &OwnerField(void *owner, size_t offset) {
#if defined(_MSC_VER) && _MSC_VER < 1200
    return *&FieldAt<T>(owner, offset);
#else
    return FieldAt<T>(owner, offset);
#endif
}

const HudFontStyle *HudUiZrdOwnerFontStyle(void *owner, int styleIndex) {
    const HudFontStyle *const stylesBase = (const HudFontStyle *)((const unsigned char *)(owner) + 0x1cec);
    const HudFontStyle *const style = &stylesBase[styleIndex];
    return style->validMarker != 0 ? style : 0;
}

void ApplyHudFontStyleToPanel(HudUiPanel *panel, const HudFontStyle *style) {
    if (style == 0) {
        return;
    }

    panel->SetFont(style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);
    FieldAt<unsigned int>(panel, 0x144) = style->alignMode;
    FieldAt<unsigned int>(panel, 0x14c) = style->textColor;
    FieldAt<unsigned int>(panel, 0x150) = style->textColor;
    FieldAt<unsigned int>(panel, 0x270) = 1;
    FieldAt<unsigned int>(panel, 0x264) = style->shadowEnabled;
    FieldAt<int>(panel, 0x29c) = 1;
    FieldAt<int>(panel, 0x2a0) = 1;
    FieldAt<unsigned int>(panel, 0x268) = style->bkMode;
    FieldAt<unsigned int>(panel, 0x26c) = style->bkColor;
}

void ApplyHudFontStyleTextOnly(HudUiPanel *panel, const HudFontStyle *style) {
    if (style == 0) {
        return;
    }

    panel->SetFont(style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);
    FieldAt<unsigned int>(panel, 0x14c) = style->textColor;
    FieldAt<unsigned int>(panel, 0x150) = style->textColor;
    FieldAt<unsigned int>(panel, 0x270) = 1;
    FieldAt<unsigned int>(panel, 0x264) = style->shadowEnabled;
    FieldAt<int>(panel, 0x29c) = 1;
    FieldAt<int>(panel, 0x2a0) = 1;
}

void DeleteHudUiListSelectorItemArray(HudUiListSelectorItem *items) {
    if (items == 0) {
        return;
    }

    unsigned char *const header = (unsigned char *)(items) - sizeof(int);
    const int count = FieldAt<int>(header, 0);
    {
    for (int index = 0; index < count; ++index) {
        ((HudUiPanel *)(&items[index]))->Destructor();
    }
    }

    ::operator delete(header);
}

HudUiPanel *CreateHudZrdLabelPanel(HudUiZrdWidget *widget, zReader::Node *labelSpecBase,
                                   int originX, int originY) {
    HudUiTransitionTextPanel *const transitionPanel = (HudUiTransitionTextPanel *)(::operator new(sizeof(HudUiTransitionTextPanel)));
    transitionPanel->Constructor();

    HudUiPanel *const panel = (HudUiPanel *)(transitionPanel);
    HudUiElement *const element = (HudUiElement *)(transitionPanel);
    element->flags = (element->flags & 0x10u) | 0x02u;

    const char *const key = ZrdArrayString(labelSpecBase, 1);
    const char *const text = key != 0 ? zLoc::ResolveMessageKeyOrFallback(key) : "";
    panel->SetTextFmt(text != 0 ? text : "");

    element->SetPos(originX + ZrdArrayInt(labelSpecBase, 2, 0),
                    originY + ZrdArrayInt(labelSpecBase, 3, 0));

    const int styleIndex = ZrdArrayInt(labelSpecBase, 4, 0);
    ApplyHudFontStyleToPanel(panel, HudUiZrdOwnerFontStyle(widget->owner, styleIndex));

    element->SetVisible(1);
    ((HudUiContainer *)(widget->owner))->AddChild(element);
    return panel;
}

void AppendHudZrdLabelPanel(HudUiZrdWidget *widget, HudUiPanelPtrVector &panels,
                            zReader::Node *labelSpecBase, int originX,
                            int originY) {
    HudUiPanel *panel = CreateHudZrdLabelPanel(widget, labelSpecBase, originX, originY);
    panels.InsertN(panels.end, 1, &panel);
}

HudUiPanel *CreateHudZrdTextPanel(HudUiZrdWidget *widget, zReader::Node *textNode,
                                  int visible) {
    zReader::Node *const textBase = ZrdArrayBase(textNode);
    if (textBase == 0) {
        return 0;
    }

    HudUiTransitionTextPanel *const transitionPanel = (HudUiTransitionTextPanel *)(::operator new(sizeof(HudUiTransitionTextPanel)));
    transitionPanel->Constructor();

    HudUiPanel *const panel = (HudUiPanel *)(transitionPanel);
    const char *const key = ZrdArrayString(textBase, 1);
    const char *const text = key != 0 ? zLoc::ResolveMessageKeyOrFallback(key) : "";
    panel->SetTextFmt(text != 0 ? text : "");

    HudUiElement *const element = (HudUiElement *)(transitionPanel);
    element->SetPos(widget->originX + ZrdArrayInt(textBase, 2, 0),
                    widget->originY + ZrdArrayInt(textBase, 3, 0));

    const int styleIndex = ZrdArrayInt(textBase, 4, 0);
    ApplyHudFontStyleTextOnly(panel, HudUiZrdOwnerFontStyle(widget->owner, styleIndex));

    element->SetVisible(visible);
    ((HudUiContainer *)(widget->owner))->AddChild(element);
    return panel;
}

void LoadHudZrdLabelSection(HudUiZrdWidget *widget, zReader::Node *parentNode,
                            HudUiPanelPtrVector &panels) {
    zReader::Node *const labelNode = zReader_GetNamedNode(parentNode, "LABEL");
    zReader::Node *const labelBase = ZrdArrayBase(labelNode);
    if (labelBase == 0) {
        return;
    }

    const int originX = widget->originX;
    const int originY = widget->originY;
    zReader::Node *const firstItem = ZrdArrayItem(labelBase, 1);
    if (firstItem != 0 && firstItem->type == zReader::ZRDR_NODE_ARRAY) {
        const int count = ZrdArrayCount(labelBase);
        {
        for (int index = 1; index <= count - 1; ++index) {
            AppendHudZrdLabelPanel(widget, panels, ZrdArrayBase(ZrdArrayItem(labelBase, index)),
                                   originX, originY);
        }
        }
        return;
    }

    AppendHudZrdLabelPanel(widget, panels, labelBase, originX, originY);
}

void ApplyHudZrdFlashSection(zReader::Node *parentNode, HudUiPanelPtrVector &panels) {
    zReader::Node *const flashNode = zReader_GetNamedNode(parentNode, "FLASH");
    if (flashNode == 0) {
        return;
    }

    float flashRate = 0.0f;
    zReader::ReadNamedFloat(flashNode, "RATE", &flashRate);

    unsigned int flashColor = 0;
    zReader::Node *const colorNode = zReader_GetNamedNode(flashNode, "COLOR");
    zReader::Node *const colorBase = ZrdArrayBase(colorNode);
    if (colorBase != 0) {
        const unsigned int red = (unsigned int)(ZrdArrayInt(colorBase, 1, 0)) & 0xffu;
        const unsigned int green =
            (unsigned int)(ZrdArrayInt(colorBase, 2, 0)) & 0xffu;
        const unsigned int blue = (unsigned int)(ZrdArrayInt(colorBase, 3, 0)) & 0xffu;
        flashColor = red | (green << 8) | (blue << 16);
    }

    if (flashRate == 0.0f) {
        return;
    }

    for (HudUiPanel **it = panels.begin; it != panels.end; ++it) {
        ((HudUiTransitionTextPanel *)(*it))->SetFlashColorAndRate(flashColor,
                                                                                flashRate);
    }
}

void LoadHudZrdBitmap(zReader::Node *parentNode, const char *sectionName,
                      zVidImagePartial **outImage) {
    zReader::Node *const bitmapNode = zReader_GetNamedNode(parentNode, sectionName);
    zReader::Node *const bitmapBase = ZrdArrayBase(bitmapNode);
    const char *const path = ZrdArrayString(bitmapBase, 1);
    if (path != 0) {
        *outImage = zImage::TexDir_FindOrCreateByPath(path);
    }
}

void LoadHudZrdSound(zReader::Node *parentNode, zSndSample **outSound, float *outScale) {
    zReader::Node *const soundNode = zReader_GetNamedNode(parentNode, "SOUND");
    zReader::Node *const soundBase = ZrdArrayBase(soundNode);
    const char *const name = ZrdArrayString(soundBase, 1);
    if (name == 0) {
        return;
    }

    *outScale = ZrdArrayCount(soundBase) >= 3 ? ZrdArrayFloat(soundBase, 2, 1.0f) : 1.0f;
    *outSound = zSnd::FindSampleByName(name);
}

void HudUiVirtualUpdateSlider(HudUiSliderBorder *slider, float deltaSeconds) {
    typedef void (RECOIL_THISCALL *UpdateFn)(HudUiSliderBorder * self, float deltaSeconds);

    const HudUiCommon_FTable *const ftable = slider->base.base.ftable;
    if (ftable != 0 && ftable->slots[9] != 0) {
        ((UpdateFn)(ftable->slots[9]))(slider, deltaSeconds);
        return;
    }

    slider->Update(deltaSeconds);
}

void HudUiVirtualUpdateSliderRequired(HudUiSliderBorder *slider, float deltaSeconds) {
    typedef void (RECOIL_THISCALL *UpdateFn)(HudUiSliderBorder * self, float deltaSeconds);

    const HudUiCommon_FTable *const ftable = slider->base.base.ftable;
    ((UpdateFn)(ftable->slots[9]))(slider, deltaSeconds);
}

void ConfigureTextStackLine(HudUiTextStack4 *stack, HudUiPanel *panel, int y,
                            int fontSize, int fontWeight,
                            int fontWidth) {
    HudUiElement *const element = (HudUiElement *)(panel);
    stack->base.AddChild(element);
    panel->SetFont("Arial", fontSize, fontWeight, fontWidth, 0, 0, 2);
    panel->SetShadow(1, -1, -1);
    FieldAt<int>(panel, 0x144) = 1;
    element->SetPos(0x140, y);
    element->SetVisible(0);
}

void DestroyTextStackLines(HudUiTextStack4 *stack) {
    {
    for (int index = 0; index < 4; ++index) {
        TextStackLineAt(stack, index)->Destructor();
    }
    }

    stack->base.DestructorCore();
}
} // namespace

namespace HudUiMgrSensor {
// Reimplements 0x438920: HudUiMgrSensor::TrackList_Add
// (D:\Proj\Battlesport\HudUiMgrSensor.cpp)
RECOIL_NOINLINE HudUiMgrSensorTrackNode *RECOIL_FASTCALL TrackList_Add(int trackKind,
                                                                        void *payload) {
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
RECOIL_NOINLINE int RECOIL_FASTCALL
PlaceTrackCounterWidget(HudUiMgrSensorTrackNode *trackNode, const zVec3 *worldPoint) {
    const int targetMarkerCount = g_HudUiMgrSensorTargetMarkerCount;
    int inBounds = 0;
    if (targetMarkerCount >= 32) {
        return 0;
    }

    HudUiSlot *const slot = &g_HudUiMgrWeaponSlots[targetMarkerCount];
    g_HudUiMgrSensorTargetMarkerCount = targetMarkerCount + 1;

    const int screenEdgeCode = zMath::ProjectPointAndClampToScreenClip(
        worldPoint, (zVec3 *)(&slot->screenX));

    int slotX = (int)(slot->screenX);
    int slotY = (int)(slot->screenY);
    if (zOpt::GetReplicateMode() != 0) {
        slotX = (int)(slot->screenX + slot->screenX);
        slotY = (int)(slot->screenY + slot->screenY);
    }
    HudUiVirtualSetPosRequired(slot, slotX, slotY);

    switch (screenEdgeCode) {
    case 0:
        inBounds = 1;
        break;

    case 1: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        HudUiVirtualSetVisibleRequired(counterWidget, 1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[1]);

        const int halfHeight = counterWidget->image->height / 2;
        int top = ((HudUiElement *)(slot))->GetY() - halfHeight;
        if (top <= g_HudUiMgrHudRect.top + halfHeight) {
            top = g_HudUiMgrHudRect.top;
        } else if (top > g_HudUiMgrSensorBlock.sensorViewportRect.top - halfHeight) {
            top = g_HudUiMgrSensorBlock.sensorViewportRect.top - halfHeight * 2;
        }
        HudUiVirtualSetPosRequired(counterWidget, 0, top);
        break;
    }

    case 2: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        HudUiVirtualSetVisibleRequired(counterWidget, 1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[2]);

        const zVidImagePartial *const image = counterWidget->image;
        const int height = image->height;
        int top = ((HudUiElement *)(slot))->GetY() - height;
        if (top <= g_HudUiMgrHudRect.top + height) {
            top = g_HudUiMgrHudRect.top;
        } else if (top > g_HudUiMgrHudRect.bottom - height) {
            top = g_HudUiMgrHudRect.bottom - height * 2;
        }

        const int left =
            ((HudUiElement *)(slot))->GetX() + 1 - image->width;
        HudUiVirtualSetPosRequired(counterWidget, left, top);
        break;
    }

    case 4: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        HudUiVirtualSetVisibleRequired(counterWidget, 1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[3]);

        const zVidImagePartial *const image = counterWidget->image;
        const int top = ((HudUiElement *)(slot))->GetY() + 1;
        const int left =
            ((HudUiElement *)(slot))->GetX() - image->width / 2;
        HudUiVirtualSetPosRequired(counterWidget, left, top);
        break;
    }

    case 8: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        HudUiVirtualSetVisibleRequired(counterWidget, 1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[4]);

        int left = ((HudUiElement *)(slot))->GetX();
        int top = ((HudUiElement *)(slot))->GetY();
        if (left < g_HudUiMgrObjectiveWidgetRightX) {
            top = g_HudUiMgrSensorBlock.sensorViewportRect.top;
        }

        const zVidImagePartial *const image = counterWidget->image;
        top -= image->height;
        left -= image->width / 2;
        HudUiVirtualSetPosRequired(counterWidget, left, top);
        break;
    }
    }

    slot->screenEdgeCode = screenEdgeCode;
    slot->trackNode = trackNode;
    return inBounds;
}

// Reimplements 0x4122c0: HudUiMgrSensor::PlaceTrackMarker
// (D:\Proj\Battlesport\HudUiMgrSensor.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
PlaceTrackMarker(int markerMode, PlayerProgressTargetSlotRuntime *outputSlots) {
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
                HudUiMgrSensorTrackNode *const trackNode = (HudUiMgrSensorTrackNode *)(slot->trackNode);
                if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
                    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(trackNode->payload);
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

            const int dx =
                ((HudUiElement *)(slot))->GetX() - g_HudUiMgrReticleProjectedX;
            const int dy =
                ((HudUiElement *)(slot))->GetY() - g_HudUiMgrReticleProjectedY;
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
        g_HudUiMgrSensorTargetMarkerImages[0]);

    const zVidImagePartial *const image = trackedProgressSlot->trackMarkerWidget.image;
    const int markerY =
        ((HudUiElement *)(trackedProgressSlot))->GetY() - image->height / 2;
    const int markerX =
        ((HudUiElement *)(trackedProgressSlot))->GetX() - image->width / 2;
    HudUiVirtualSetPosRequired(&trackedProgressSlot->trackMarkerWidget, markerX, markerY);
    HudUiVirtualSetVisibleRequired(&trackedProgressSlot->trackMarkerWidget, 1);

    HudUiMgrSensorTrackNode *const trackNode = (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
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
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMarkersAndProgressFromVariantTag(const zTag4Partial *requiredVariantTag) {
    HudUiMgrSensorTrackNode *trackNode = g_HudUiMgrSensor_TrackList.head;
    zUtil_PlayerStateStorage *const localPlayerState = (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);

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
                VariantTag::TagsOverlap(&playerState->variantTag, requiredVariantTag) != 0) {
                const float distXZ = fabs(playerState->fxOffsetWorld.x -
                                               localPlayerState->worldPos.x) +
                                     fabs(playerState->fxOffsetWorld.z -
                                               localPlayerState->worldPos.z);
                if (distXZ < 650.0f && candidateCount < 0x63) {
                    candidateTrackNodes[candidateCount++] = trackNode;
                }
            }
        } else {
            trackNode->trackKind = HUD_SENSOR_TRACK_KIND_TURRET;
            zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(trackNode->payload);
            if (turretRuntime->HasActiveNode() != 0 &&
                VariantTag::CurrentAllowsId(turretRuntime->turretNode->nodeType) != 0) {
                const float distXZ =
                    fabs(turretRuntime->firePos.z - localPlayerState->worldPos.z) +
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
            zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(selectedTrackNode->payload);
            zUtil_PlayerStateStorage *const playerState = saveState->playerState;
            zVec3 point = playerState->fxOffsetWorld;
            point.y += 3.0f;

            const int visible = Player::TestScenePathBetweenCameraTargetAndPoint(
                playerState->rootNode, &point, 1);
            playerState->spawnStateInitialized = visible;
            if (visible != 0 && playerState->recentHitMarkerHandle != 0) {
                playerState->recentHitFlag = 1;
                playerState->recentHitExpireTime = g_Time_AccumulatedTimeSec + 3.0f;
            }
        } else if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
            zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(selectedTrackNode->payload);
            turretRuntime->scenePathVisible = Player::TestScenePathBetweenCameraTargetAndPoint(
                turretRuntime->turretNode, &turretRuntime->firePos, 2);
        }

        {
        for (int index = 0; index < candidateCount; ++index) {
            HudUiMgrSensorTrackNode *const candidate = candidateTrackNodes[index];
            if (candidate->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
                zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(candidate->payload);
                zUtil_PlayerStateStorage *const playerState = saveState->playerState;
                if ((playerState->spawnStateInitialized & 1) != 0) {
                    playerState->recentHitMarkerHandle =
                        HudUiMgrSensor::PlaceTrackCounterWidget(candidate,
                                                                &playerState->fxOffsetWorld);
                }
            } else if (candidate->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
                zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(candidate->payload);
                if ((turretRuntime->scenePathVisible & 1) != 0) {
                    HudUiMgrSensor::PlaceTrackCounterWidget(candidate, &turretRuntime->firePos);
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
            HudUiMgrSensor::PlaceTrackMarker(markerMode, localPlayerState->progressTargetSlots);
    }

    HudUiMgrTarget::UpdateSelectedProgressMeter(0);
}

// Reimplements 0x411f10: HudUiMgrSensor::SetShieldMessageRatio
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_FASTCALL SetShieldMessageRatio(float ratio) {
    typedef void (RECOIL_CDECL *SetTextFmtFn)(void *self, const char *format, ...);

    if (ratio > 1.0f) {
        ratio = 1.0f;
    } else if (ratio < 0.0f) {
        ratio = 0.0f;
    }

    HudUiShieldMessageWidget *const shieldMessageWidget = g_HudUiMgrShieldMessageWidget;
    HudUiMeter *const meter = &shieldMessageWidget->meter;
    const unsigned char green = ratio < 0.25f ? 0 : 255;
    meter->color565 = zVid_PackColorRGB(255, green, 0) & 0xffffu;

    const int fillPixels =
        (int)(ceil((double)(meter->fillPixelsMax) *
                              (double)(ratio)));
    const int top = (int)(meter->points[1].y) - fillPixels;
    meter->points[0].y = (float)(top);
    meter->points[3].y = (float)(top);
    HudUiVirtualInvalidate(meter);

    HudUiPanel *const percentTextPanel =
        (HudUiPanel *)(&shieldMessageWidget->percentTextPanel);
    const HudUiCommon_FTable *const panelFTable =
        *(const HudUiCommon_FTable *const *)(percentTextPanel);
    const int percent = (int)(ceil((double)(ratio) * 100.0));
    ((SetTextFmtFn)(panelFTable->slots[0x74 / 4]))(percentTextPanel, "%d", percent);
    HudUiVirtualInvalidate(percentTextPanel);
}

// Reimplements 0x410d10: HudUiMgrSensor::SetViewportRect
void RECOIL_FASTCALL SetViewportRect(int x, int y, int width,
                                     int height) {
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
RECOIL_NOINLINE void RECOIL_FASTCALL GetFxRect(HudUiRect *outRect) {
    *outRect = g_HudUiMgrSensorFxRect;
}
} // namespace HudUiMgrSensor

namespace HudUiMgrTarget {
// Reimplements 0x4124b0: HudUiMgrTarget::UpdateSelectedProgressMeter
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateSelectedProgressMeter(
    int clearSelectedTrack) {
    HudUiSlot *trackedProgressSlot = 0;
    if (clearSelectedTrack != 0) {
        g_HudUiMgrSensorTrackedProgressSlot = 0;
    } else {
        trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    }

    if (g_HudUiMgr.enabled == 0 || g_HudUiMgrObjectivePhase != 0 ||
        trackedProgressSlot == 0) {
        return;
    }

    HudUiMgrSensorTrackNode *const selectedTrackNode = (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
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
        HudUiVirtualSetVisibleRequired(&g_HudUiMgrSensorMeter, 0);
        trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    }

    if (zClipAlt::RemapPointXYInPlace(&trackedProgressSlot->screenX) == 0) {
        return;
    }

    if (zOpt::GetReplicateMode() != 0) {
        g_HudUiMgrSensorTrackedProgressSlot->screenX += g_HudUiMgrSensorTrackedProgressSlot->screenX;
        g_HudUiMgrSensorTrackedProgressSlot->screenY += g_HudUiMgrSensorTrackedProgressSlot->screenY;
    }

    float healthRatio = selectedHealthCurrent / selectedHealthMax;
    if (healthRatio > 1.0f) {
        healthRatio = 1.0f;
    } else if (healthRatio < 0.0f) {
        healthRatio = 0.0f;
    }

    const int fillPixels =
        (int)(ceil((double)(g_HudUiMgrSensorMeter.fillPixelsMax) *
                                            (double)(healthRatio)));
    const int top = (int)(g_HudUiMgrSensorMeter.points[1].y) -
                             fillPixels;
    g_HudUiMgrSensorMeter.points[0].y = (float)(top);
    g_HudUiMgrSensorMeter.points[3].y = (float)(top);
    HudUiVirtualInvalidate(&g_HudUiMgrSensorMeter);
    HudUiVirtualSetVisibleRequired(&g_HudUiMgrSensorMeter, 1);
}
} // namespace HudUiMgrTarget

namespace HudUiMgrObjective {
// Reimplements 0x412050: HudUiMgrObjective::RefreshCounterText
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RefreshCounterText(int counterValue) {
    HudUiPanel *const panel = (HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel);
    panel->SetTextFmt("%d", counterValue);
    panel->UpdateTextBoundsFromContent();
}

// Reimplements 0x411760: HudUiMgrObjective::SetVisibleAndResetMeterFill
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_FASTCALL SetVisibleAndResetMeterFill(int visible) {
    if (visible == 0) {
        HudUiVirtualSetVisibleRequired(g_HudUiMgrObjectiveLabelTextPanel, 0);
        HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveMeter, 0);
        return;
    }

    HudUiVirtualSetVisibleRequired(g_HudUiMgrObjectiveLabelTextPanel, 1);
    HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveMeter, 1);

    const int meterTop = (int)(g_HudUiMgrObjectiveMeter.points[1].y);
    g_HudUiMgrObjectiveMeterFillAnimTimerSec = 0.0f;
    g_HudUiMgrObjectiveMeterFillAnimEnabled = 1;
    g_HudUiMgrObjectiveMeter.points[0].y = (float)(meterTop);
    g_HudUiMgrObjectiveMeter.points[3].y = (float)(meterTop);
}

// Reimplements 0x4118b0: HudUiMgrObjective::UpdateMeterXPoints
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_CDECL UpdateMeterXPoints() {
    typedef int (RECOIL_THISCALL *GetCenterXFn)(HudUiWidget * self);

    const HudUiWidget_FTable *const ftable = g_HudUiMgrObjectiveWidget.ftable;
    const float left =
        (float)(((GetCenterXFn)(ftable->slots[0x64 / 4]))(
            &g_HudUiMgrObjectiveWidget)) +
        5.0f;
    const float right = left + 7.0f;
    g_HudUiMgrObjectiveMeter.points[0].x = left;
    g_HudUiMgrObjectiveMeter.points[1].x = left;
    g_HudUiMgrObjectiveMeter.points[2].x = right;
    g_HudUiMgrObjectiveMeter.points[3].x = right;
}

// Reimplements 0x4117f0: HudUiMgrObjective::TickMeterFillAnimation
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_CDECL TickMeterFillAnimation() {
    g_HudUiMgrObjectiveMeterFillAnimTimerSec += g_Time_UnscaledDeltaTimeSec;

    int fillPixels;
    if (g_HudUiMgrObjectiveMeterFillAnimTimerSec >= 3.0f) {
        fillPixels =
            (int)(ceil((double)(
                g_HudUiMgrObjectiveMeter.fillPixelsMax)));
        g_HudUiMgrObjectiveMeterFillAnimEnabled = 0;
    } else {
        const double fillRatio =
            (double)(g_HudUiMgrObjectiveMeterFillAnimTimerSec * 0.333332986f) *
            (double)(g_HudUiMgrObjectiveMeter.fillPixelsMax);
        fillPixels = (int)(ceil(fillRatio));
    }

    const int top =
        (int)(g_HudUiMgrObjectiveMeter.points[1].y) - fillPixels;
    g_HudUiMgrObjectiveMeter.points[0].y = (float)(top);
    g_HudUiMgrObjectiveMeter.points[3].y = (float)(top);
}

static void HudUiMgrObjective_UpdateWidgetRightX() {
    const zVidImagePartial *const image = g_HudUiMgrObjectiveWidget.image;
    const int width = image != 0 ? image->width : 0;
    g_HudUiMgrObjectiveWidgetRightX = g_HudUiMgrObjectiveWidget.GetCenterX() + width;
}

static void HudUiMgrObjective_SetSlidePosition(float slideX) {
    g_HudUiMgrObjectiveBar.points[2].x = slideX;
    g_HudUiMgrObjectiveBar.points[3].x = slideX;
    ((HudUiElement *)(&g_HudUiMgrObjectiveBar))->Invalidate();
    ((HudUiElement *)(&g_HudUiMgrObjectiveWidget))->SetX((int)(slideX) - 1);
    HudUiMgrObjective::UpdateMeterXPoints();
}

static void HudUiMgrObjective_UpdateHwDirtyRectIfNeeded() {
    if (zOpt::GetHudTypeForCurrentHwMode() == 2) {
        g_HudLayoutHW.UpdateObjectiveDirtyRect();
    }
}

static void HudUiMgrObjective_DrawSensorNoise(float fade, int visibleWhenCovered) {
    if (g_HudUiMgrObjectiveSensorRect.image == 0) {
        return;
    }

    float noise = fade + fade;
    if (noise < 1.0f) {
        zVid::DrawNoiseRect((zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
                            (double)(noise));
        return;
    }

    HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveSensorRect, visibleWhenCovered);
    zVid::DrawNoiseRect((zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
                        (double)(2.0f - noise));
}

// Reimplements 0x411900: HudUiMgrObjective::Show
// (D:\Proj\Battlesport\hud.cpp)
int RECOIL_FASTCALL Show(zVidImagePartial *objectiveImage, const char *summaryFormat,
                                  const char *descText, float autoHideDelay) {
    if (summaryFormat == 0 || descText == 0 ||
        g_HudUiMgrObjectiveChatComposeActive != 0) {
        return 0;
    }

    HudUiPanelVirtualSetTextFmtRequired(g_HudUiMgrObjectiveSummaryTextPanel, summaryFormat);
    HudUiPanelVirtualSetTextFmtRequired(g_HudUiMgrObjectiveDescTextPanel, descText);
    HudUiVirtualSetVisibleRequired(&g_HudUiMgrSensorOverlay, 0);

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
        HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveBar, 1);
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
void RECOIL_CDECL Begin() {
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

        HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveSensorRect, 0);
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
RECOIL_NOINLINE void RECOIL_CDECL StartHide() {
    g_HudUiMgrObjectivePhaseTimerSec += g_Time_UnscaledDeltaTimeSec;

    if (g_HudUiMgrObjectivePhase == 1) {
        if (g_HudUiMgrObjectivePhaseTimerSec < g_HudUiMgrObjectivePhaseDurationSec) {
            const float fade =
                g_HudUiMgrObjectivePhaseTimerSec / g_HudUiMgrObjectivePhaseDurationSec;
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x +
                fade * ((HudUiObjectiveBar *)(&g_HudUiMgrObjectiveBar))->slideRangeX;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateWidgetRightX();
            HudUiMgrObjective_DrawSensorNoise(fade, 1);
        } else {
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x +
                ((HudUiObjectiveBar *)(&g_HudUiMgrObjectiveBar))->slideRangeX;
            g_HudUiMgrObjectivePhase = 2;
            g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateWidgetRightX();
            HudUiVirtualSetVisibleRequired(g_HudUiMgrObjectiveSummaryTextPanel, 1);
            HudUiVirtualSetVisibleRequired(g_HudUiMgrObjectiveDescTextPanel, 1);
            HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveSensorRect, 1);
        }
    } else if (g_HudUiMgrObjectivePhase == 2) {
        ((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel))->Invalidate();
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))->Invalidate();
        ((HudUiElement *)(&g_HudUiMgrObjectiveBar))->Invalidate();
        ((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect))->Invalidate();
    } else if (g_HudUiMgrObjectivePhase == 3) {
        if (g_HudUiMgrObjectivePhaseTimerSec < g_HudUiMgrObjectivePhaseDurationSec) {
            const float fade =
                1.0f - g_HudUiMgrObjectivePhaseTimerSec / g_HudUiMgrObjectivePhaseDurationSec;
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x +
                fade * ((HudUiObjectiveBar *)(&g_HudUiMgrObjectiveBar))->slideRangeX;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateHwDirtyRectIfNeeded();
            HudUiMgrObjective_UpdateWidgetRightX();
            HudUiMgrObjective_DrawSensorNoise(fade, 0);
        } else {
            g_HudUiMgrObjectiveState = 0;
            g_HudUiMgrObjectivePhase = 0;
            g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
            ((HudUiElement *)(&g_HudUiMgrObjectiveWidget))->SetX((int)(g_HudUiMgrObjectiveBar.points[1].x));
            HudUiMgrObjective::UpdateMeterXPoints();
            HudUiMgrObjective_UpdateHwDirtyRectIfNeeded();
            HudUiMgrObjective_UpdateWidgetRightX();
            HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveBar, 0);
            HudUiVirtualSetVisibleRequired(&g_HudUiMgrSensorOverlay, 1);
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
void RECOIL_CDECL Update() {
    HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveWidget, 1);
    if (g_HudUiMgrObjectivePhase == 0) {
        return;
    }

    ((HudUiElement *)(&g_HudUiMgrObjectiveBar))->SetVisible(1);
    if (g_HudUiMgrObjectivePhase != 2) {
        return;
    }

    if (g_HudUiMgrObjectiveDescTextPanel != 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))->SetVisible(1);
    }

    if (g_HudUiMgrObjectiveLabelTextPanel != 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->SetVisible(1);
    }

    HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveSensorRect, 1);
}
} // namespace HudUiMgrObjective

namespace HudUiLoadingCheckpoint {
// Reimplements 0x414180: HudUiLoadingCheckpoint::AdvanceAndLog
void RECOIL_FASTCALL AdvanceAndLog(const char *messageOrNull) {
    const unsigned int currentIndex = g_HudUiLoadingCheckpointCurrentIndex;
    const unsigned int maxIndex = g_HudUiLoadingCheckpointMaxIndex;
    if (currentIndex > maxIndex) {
        zError::ReportOld(0x800, "D:\\Proj\\Battlesport\\hud.cpp", 0x1184, "Checkpoint overflow");
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
void RECOIL_CDECL InitTable() {
    static const float kRawProgress[] = {
        0.00100000005f, 0.136999995f, 0.237000003f, 0.340000004f, 0.899999976f,
        9.30000019f,    12.3999996f,  13.3999996f,  20.0f,        26.0f,
        26.2999992f,    28.7000008f,  31.5f,        34.0f,        36.2000008f,
        36.4000015f,    53.2999992f,  53.5999985f,  53.7000008f,
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
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateTextLine(int op, int index,
                                                    const char *format) {
    HudUiPanel *const panel = (HudUiPanel *)(&g_HudUiMgrStringMenu->items[index]);

    if (op == 1) {
        HudUiPanelVirtualSetTextFmtRequired(panel, format);
        HudUiVirtualSetVisibleRequired(panel, 1);
        return;
    }

    if (op == 0) {
        HudUiVirtualSetVisibleRequired(panel, 0);
        return;
    }

    if (op == 2) {
        if (*format != '\0') {
            HudUiPanelVirtualSetTextFmtRequired(panel, format);
            HudUiVirtualSetVisibleRequired(panel, 1);
        } else {
            HudUiVirtualSetVisibleRequired(panel, 0);
        }
    }
}

// Reimplements 0x4137c0: HudUiAuxOverlay::ClearTextLines
RECOIL_NOINLINE void RECOIL_CDECL ClearTextLines() {
    {
    for (int index = 0; index < 23; ++index) {
        UpdateTextLine(2, index, "");
        UpdateTextLine(0, index, 0);
    }
    }
}
} // namespace HudUiAuxOverlay

namespace {
zReader::Node *HudUiZrdPayload(zReader::Node *node) {
    return node != 0 && node->type == zReader::ZRDR_NODE_ARRAY ? node->value.nodes : 0;
}

const char *HudUiZrdStringAt(zReader::Node *payload, int index) {
    return payload != 0 ? payload[index].value.str : 0;
}

int HudUiZrdIntAt(zReader::Node *payload, int index) {
    return payload != 0 ? payload[index].value.i32 : 0;
}

void HudUiEnsureLoaderWidgetsConstructed() {
    if (g_HudUiMgrSensorPanel.ftable == 0) {
        g_HudUiMgrSensorPanel.Constructor(0);
    }
    if (g_HudUiMgrSensorOverlay.ftable == 0) {
        g_HudUiMgrSensorOverlay.Constructor(0);
    }
    if (g_HudUiMgrSensorMeter.ftable == 0) {
        g_HudUiMgrSensorMeter.Constructor();
    }
    if (g_HudUiMgrObjectiveWidget.ftable == 0) {
        g_HudUiMgrObjectiveWidget.Constructor(0);
    }
    if (g_HudUiMgrObjectiveSensorRect.ftable == 0) {
        g_HudUiMgrObjectiveSensorRect.Constructor(0);
    }
    if (((HudUiElement *)(&g_HudUiMgrObjectiveBar))->ftable == 0) {
        g_HudUiMgrObjectiveBar.Constructor();
    }
    if (g_HudUiMgrReticleWidget.ftable == 0) {
        g_HudUiMgrReticleWidget.Constructor(0);
    }
    if (g_HudUiMgrNanitePanel.base.ftable == 0) {
        ((HudUiTripletPanel *)(&g_HudUiMgrNanitePanel))->Constructor();
    }

    {
        int messageIndex1;
        for (messageIndex1 = 0;
             messageIndex1 < (int)(sizeof(g_HudUiMgrMessages) / sizeof(g_HudUiMgrMessages[0]));
             ++messageIndex1) {
            HudUiMessage &message = g_HudUiMgrMessages[messageIndex1];
            if (message.base.ftable == 0) {
                message.Constructor();
            }
        }
    }

    {
        int counterIndex2;
        for (counterIndex2 = 0;
             counterIndex2 <
                 (int)(sizeof(g_HudUiMgrModeCounters) / sizeof(g_HudUiMgrModeCounters[0]));
             ++counterIndex2) {
            HudUiCounter &counter = g_HudUiMgrModeCounters[counterIndex2];
            if (((HudUiWidget *)(&counter))->ftable == 0) {
                counter.Constructor();
            }
        }
    }

    {
        int slotIndex3;
        for (slotIndex3 = 0;
             slotIndex3 < (int)(sizeof(g_HudUiMgrWeaponSlots) / sizeof(g_HudUiMgrWeaponSlots[0]));
             ++slotIndex3) {
            HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex3];
            if (slot.ftable == 0) {
                slot.Constructor();
            }
        }
    }
}

void HudUiSetFontFromRect(HudUiPanel *panel, const HudUiRect &fontSpec) {
    panel->SetFont((const char *)(fontSpec.left), fontSpec.right, fontSpec.bottom,
                   fontSpec.top, 0, 0, 2);
}

void HudUiSetPanelClipWithSource(void *panel, void *source, const HudUiRect *clipRect) {
    typedef void (RECOIL_THISCALL *SetClipFn)(void *self, void *source, const HudUiRect *clipRect);

    const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(panel);
    ((SetClipFn)(ftable->slots[6]))(panel, source, clipRect);
}

void HudUiApplyStatsTripletInt3(zReader::Node *payload, int nodeIndex,
                                int &outX, int &outY,
                                int *outZ = 0) {
    HudUiLayoutNode::ReadInt3(&payload[nodeIndex], &outX, &outY, outZ);
}
} // namespace

namespace HudUiMgr {
// Reimplements 0x411170: HudUiMgr::ProjectPointToNormalizedClamped
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ProjectPointToNormalizedClamped(
    const zVec3 *srcPoint, zVec3 *projectedPoint) {
    if (zMath::ProjectPointAndClampToScreenClip(srcPoint, projectedPoint) == 0x10) {
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
        (projectedPoint->y - (float)(g_HudUiMgrHudRect.top) - halfHudHeight) /
        halfHudHeight;

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
void RECOIL_CDECL TriggerCurrentLayoutOnActivated() {
    if (g_HudUiMgrCurrentLayout != 0) {
        g_HudUiMgrCurrentLayout->ftable->OnActivated(g_HudUiMgrCurrentLayout);
    }
}

// Reimplements 0x410140: HudUiMgr::TickLayoutDelay
int RECOIL_CDECL TickLayoutDelay() {
    if (g_HudUiMgrLayoutDelayFrames == 0) {
        return 0;
    }

    --g_HudUiMgrLayoutDelayFrames;
    return 1;
}

// Reimplements 0x410160: HudUiMgr::EnsureHudLoaded
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL EnsureHudLoaded(const char *entryPath) {
    if (g_HudUiMgrHudLoaded != 0) {
        return 1;
    }

    zReader::Node *const root = zReader::LoadNodeFromPath(entryPath, 0, 0);
    if (root == 0) {
        zError::ReportOld(0x200, "D:\\Proj\\Battlesport\\hud.cpp", 0x60d,
                          "Failed to read %s", entryPath);
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

    zReader::Node *const fontsNode = zReader_GetNamedNode(root, "FONTS");
    if (fontsNode != 0) {
        if (zReader::Node *const node = zReader_GetNamedNode(fontsNode, "OBJ_SUMMARY")) {
            HudUiLayoutNode::ReadRect(node, &objectiveSummaryFont);
        }
        if (zReader::Node *const node = zReader_GetNamedNode(fontsNode, "OBJ_DESCRIPTION")) {
            HudUiLayoutNode::ReadRect(node, &objectiveDescriptionFont);
        }
        if (zReader::Node *const node = zReader_GetNamedNode(fontsNode, "STRINGS")) {
            HudUiPanelFontParams *const fontArgs =
                (HudUiPanelFontParams *)(&g_HudUiMgrStringMenu->unknown_10[0]);
            HudUiLayoutNode::ReadRect(node, (HudUiRect *)(fontArgs));
            {
                int itemIndex4;
                for (itemIndex4 = 0;
                     itemIndex4 < (int)(sizeof(g_HudUiMgrStringMenu->items) /
                                         sizeof(g_HudUiMgrStringMenu->items[0]));
                     ++itemIndex4) {
                    HudUiPanelSimple &item = g_HudUiMgrStringMenu->items[itemIndex4];
                    HudUiPanelVirtualSetFontRequired((HudUiPanel *)(&item),
                                                     fontArgs->faceName, fontArgs->height,
                                                     fontArgs->weight, fontArgs->width, 0, 0, 2);
                }
            }
        }
        if (zReader::Node *const node = zReader_GetNamedNode(fontsNode, "MESSAGES")) {
            HudUiRect messagesFont = {0};
            HudUiLayoutNode::ReadRect(node, &messagesFont);
            if (g_HudUiTopMessageStack != 0) {
                g_HudUiTopMessageStack->SetFontAll((const char *)(messagesFont.left),
                                                   messagesFont.right, messagesFont.bottom,
                                                   messagesFont.top);
            }
            if (g_HudUiChatMessageStack != 0) {
                g_HudUiChatMessageStack->SetFontAll(
                    (const char *)(messagesFont.left), messagesFont.right,
                    messagesFont.bottom, messagesFont.top);
            }
        }
        if (zReader::Node *const node = zReader_GetNamedNode(fontsNode, "AMMO")) {
            HudUiLayoutNode::ReadRect(node, &ammoFont);
        }
    }

    if (zReader::Node *const naniteNode = zReader_GetNamedNode(root, "NANITE")) {
        g_HudUiMgrNanitePanel.InitLayout(naniteNode);
    }

    zReader::Node *const sensorNode = zReader_GetNamedNode(root, "SENSOR");
    int sensorCenterX = 0;
    int sensorCenterY = 0;
    if (zReader::Node *const sensorPayload = HudUiZrdPayload(sensorNode)) {
        HudUiLayoutNode::ApplyImageWidget(&sensorPayload[1], &g_HudUiMgrSensorPanel, 0,
                                          g_HudUiMgrHudOriginY, 0, 0,
                                          &g_HudUiMgrSensorBlock.sensorViewportRect);

        sensorCenterX = g_HudUiMgrSensorPanel.GetCenterX();
        sensorCenterY = g_HudUiMgrSensorPanel.GetCenterY();
        g_HudUiMgrSensorBlock.sensorParam = sensorPayload[2].value.f32;

        int sensorOffsetX = 0;
        int sensorOffsetY = 0;
        int sensorWidth = 0;
        int sensorHeight = 0;
        HudUiLayoutNode::ReadInt3(&sensorPayload[3], &sensorOffsetX, &sensorOffsetY, 0);
        HudUiLayoutNode::ReadInt3(&sensorPayload[4], &sensorWidth, &sensorHeight, 0);

        const int sensorX = sensorCenterX + sensorOffsetX;
        const int sensorY = sensorCenterY + sensorOffsetY;
        g_HudUiMgrSensorFxRect.left = sensorX;
        g_HudUiMgrSensorFxRect.top = sensorY;
        g_HudUiMgrSensorFxRect.right = sensorX + sensorWidth;
        g_HudUiMgrSensorFxRect.bottom = sensorY + sensorHeight;
        g_HudUiMgrSensorFxViewportWidth = sensorWidth;
        g_HudUiMgrSensorFxViewportHeight = sensorHeight;
        HudUiMgrSensor::SetViewportRect(sensorX, sensorY, sensorWidth, sensorHeight);

        const float range = sensorPayload[5].value.f32;
        float rangeBitsValue = range * range * 0.5f;
        unsigned int rangeBits = 0;
        memcpy(&rangeBits, &rangeBitsValue, sizeof(rangeBits));
        rangeBits = (rangeBits >> 1) + 0x1fc00000u;
        memcpy(&rangeBitsValue, &rangeBits, sizeof(rangeBitsValue));
        g_HudUiMgrSensorBlock.sensorRangeSq = rangeBitsValue + rangeBitsValue;

        const int overlayAnchor[2] = {sensorCenterX, sensorCenterY};
        HudUiLayoutNode::ApplyImageWidget(&sensorPayload[6], &g_HudUiMgrSensorOverlay, 0, 0,
                                          overlayAnchor, 0, 0);

        HudUiRect meterRect = {0};
        HudUiLayoutNode::ApplyMeterQuad(&sensorPayload[7], &g_HudUiMgrSensorMeter, 0, 0,
                                        overlayAnchor, &meterRect);
        g_HudUiMgrSensorMeter.color565 = 0x7e0;
        ((HudUiElement *)(&g_HudUiMgrSensorMeter))->SetBltSourceAndClipRect(g_HudUiMgrSensorPanel.image, &meterRect);

        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorOverlay));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorMeter));
    }

    if (zReader::Node *const objectivePayload =
            HudUiZrdPayload(zReader_GetNamedNode(root, "OBJECTIVE"))) {
        g_HudUiMgrObjectivePhaseDurationSec = objectivePayload[1].value.f32;

        const int panelCenter[2] = {sensorCenterX != 0 ? sensorCenterX
                                                               : g_HudUiMgrSensorPanel.GetCenterX(),
                                            sensorCenterY != 0 ? sensorCenterY
                                                               : g_HudUiMgrSensorPanel.GetCenterY()};
        HudUiLayoutNode::ApplyImageWidget(&objectivePayload[2], &g_HudUiMgrObjectiveWidget, 0, 0,
                                          panelCenter, 0, 0);

        int objectiveCenter[2] = {g_HudUiMgrObjectiveWidget.GetCenterX(),
                                          g_HudUiMgrObjectiveWidget.GetCenterY()};
        HudUiRect objectiveBarRect = {0};
        HudUiLayoutNode::ApplyCornerTextQuad(&objectivePayload[3], &g_HudUiMgrObjectiveBar,
                                             objectiveCenter, &objectiveBarRect);
        FieldAt<float>(&g_HudUiMgrObjectiveBar, 0x138) =
            (float)(panelCenter[0] - objectiveBarRect.left);

        int red = 0;
        int green = 0;
        int blue = 0;
        HudUiLayoutNode::ReadInt3(&objectivePayload[4], &red, &green, &blue);
        g_HudUiMgrObjectiveBar.drawParam = zVid_PackColorRGB(
            (unsigned char)(red), (unsigned char)(green),
            (unsigned char)(blue)) &
                                           0xffffu;

        int x = 0;
        int y = 0;
        HudUiLayoutNode::ReadInt3(&objectivePayload[5], &x, &y, 0);
        ((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel))->SetPos(objectiveCenter[0] + x, objectiveCenter[1] + y);
        HudUiLayoutNode::ReadInt3(&objectivePayload[6], &x, &y, 0);
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))->SetPos(objectiveCenter[0] + x, objectiveCenter[1] + y);

        HudUiRect wrapRect = {0};
        wrapRect.left = 0;
        wrapRect.top = 0;
        wrapRect.right = panelCenter[0] - x * 2 - objectiveBarRect.left;
        wrapRect.bottom = panelCenter[1] - objectiveBarRect.bottom;
        g_HudUiMgrObjectiveDescTextPanel->EnableWordWrapWithRect(&wrapRect);

        HudUiLayoutNode::ApplyMeterQuad(&objectivePayload[7], &g_HudUiMgrObjectiveMeter, 0, 0,
                                        objectiveCenter, &objectiveBarRect);
        HudUiMgrObjective::UpdateMeterXPoints();
        const int meterTop =
            (int)(g_HudUiMgrObjectiveMeter.points[1].y) -
            (int)(ceil((double)(
                g_HudUiMgrObjectiveMeter.fillPixelsMax)));
        g_HudUiMgrObjectiveMeter.color565 = 0x1f;
        g_HudUiMgrObjectiveMeter.points[0].y = (float)(meterTop);
        g_HudUiMgrObjectiveMeter.points[3].y = (float)(meterTop);

        HudUiLayoutNode::ReadInt3(&objectivePayload[8], &x, &y, 0);
        ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->SetPos(x, y + g_HudUiMgrHudOriginY);
        g_HudUiMgrObjectiveLabelTextPanel->SetTextFmt("%s", zLoc::GetMessageString(0x906));
        ((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect))->SetPos(g_HudUiMgrSensorFxRect.left, g_HudUiMgrSensorFxRect.top);

        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveWidget));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveBar));
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveMeter));
        ((HudUiElement *)(&g_HudUiMgrObjectiveBar))->SetVisible(0);

        g_HudUiMgrObjectiveState = 0;
        g_HudUiMgrObjectivePhase = 0;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
        g_HudUiMgrObjectiveChatComposeActive = 0;
        HudUiSetFontFromRect(g_HudUiMgrObjectiveDescTextPanel, objectiveDescriptionFont);
        HudUiSetFontFromRect(g_HudUiMgrObjectiveSummaryTextPanel, objectiveSummaryFont);
    }

    if (zReader::Node *const reticlePayload =
            HudUiZrdPayload(zReader_GetNamedNode(root, "RETICULE"))) {
        g_HudUiMgrReticleImages[0] =
            zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(reticlePayload, 1));
        g_HudUiMgrReticleImages[1] =
            zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(reticlePayload, 2));
        g_HudUiMgrReticleImages[2] =
            zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(reticlePayload, 3));
        g_HudUiMgrReticleWidget.SetImageBorrowedAndInvalidate(g_HudUiMgrReticleImages[0]);
        g_HudUiMgrReticleWidget.imageStateWord =
            (g_HudUiMgrReticleWidget.imageStateWord & 0xffff0000u) | 1u;
        ((HudUiElement *)(&g_HudUiMgrReticleWidget))->Invalidate();
        zVidImagePartial *const image = g_HudUiMgrReticleWidget.image;
        g_HudUiMgrReticleWidgetHalfW = image != 0 ? (short)(image->width) / 2
                                                        : 0;
        g_HudUiMgrReticleWidgetHalfH =
            image != 0 ? (short)(image->height) / 2 : 0;
        ((HudUiElement *)(&g_HudUiMgrReticleWidget))->SetVisible(0);
    }

    if (zReader::Node *const statsPayload = HudUiZrdPayload(zReader_GetNamedNode(root, "STATS"))) {
        HudUiWidget *const layoutWidget = &FieldAt<HudUiWidget>(&g_HudLayoutHW, 0xec);
        const int layoutCenterX = layoutWidget->GetCenterX();
        const int layoutCenterY = layoutWidget->GetCenterY();
        int x = 0;
        int y = 0;
        int z = 0;
        HudUiLayoutNode::ReadInt3(&statsPayload[1], &x, &y, 0);
        const int counterX = (g_HudUiMgrHudOriginX / 2) + x;
        ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))->SetPos(counterX + layoutCenterX, y + layoutCenterY);
        FieldAt<int>(g_HudUiMgrObjectiveCounterTextPanel, 0x144) = 1;
        HudUiRect counterClip = {counterX - 0x14, y, counterX + 0x14, y + 0x0a};
        HudUiSetPanelClipWithSource(g_HudUiMgrObjectiveCounterTextPanel, 0, &counterClip);
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->SetTextFmt("        ");
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->UpdateTextBoundsFromContent();
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->SetTextFmt("%d", 0);
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->UpdateTextBoundsFromContent();

        HudUiLayoutNode::ReadInt3(&statsPayload[2], &x, &y, 0);
        const int timerX = x + g_HudUiMgrHudOriginX;
        ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetPos(timerX + layoutCenterX, y + layoutCenterY);
        HudUiRect timerClip = {timerX, y, 0, 0};
        HudUiSetPanelClipWithSource(g_HudUiMgrTimerPanel, 0, &timerClip);
        ((HudUiPanel *)(g_HudUiMgrTimerPanel))->SetTextFmt("00:00:00");

        HudUiTriplet *const triplet = g_HudUiMgrStatsList->triplet;
        HudUiApplyStatsTripletInt3(statsPayload, 3, x, y, &z);
        triplet->baseXStart = x + layoutCenterX + g_HudUiMgrHudOriginX;
        triplet->baseYStart = y + layoutCenterY;
        triplet->rowPitchYStart = z;
        HudUiApplyStatsTripletInt3(statsPayload, 4, x, y, &z);
        triplet->baseXEnd = x + layoutCenterX + g_HudUiMgrHudOriginX;
        triplet->baseYEnd = y + layoutCenterY;
        triplet->rowPitchYEnd = z;
        HudUiApplyStatsTripletInt3(statsPayload, 5, triplet->lapsColumnOffsetXStart,
                                   triplet->lapsColumnOffsetXEnd);
        HudUiApplyStatsTripletInt3(statsPayload, 6, triplet->killsColumnOffsetXStart,
                                   triplet->killsColumnOffsetXEnd);
        HudUiApplyStatsTripletInt3(statsPayload, 7, triplet->fontSizeStart,
                                   triplet->fontSizeEnd);
        HudUiApplyStatsTripletInt3(statsPayload, 8, triplet->fontWeightStart,
                                   triplet->fontWeightEnd);
        triplet->InterpolateLayout(0.0f);
        triplet->RebuildDisplay();
    }

    if (zReader::Node *const shieldNode = zReader_GetNamedNode(root, "SHIELD")) {
        HudUiShieldMessageWidget::ApplyLayout(shieldNode);
    }

    if (zReader::Node *const targetPayload = HudUiZrdPayload(zReader_GetNamedNode(root, "TARGET"))) {
        {
        for (int index = 0; index < 5; ++index) {
            zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(targetPayload, index + 1));
        }
        }

        {
            int slotIndex5;
            for (slotIndex5 = 0;
                 slotIndex5 < (int)(sizeof(g_HudUiMgrWeaponSlots) /
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
            for (slotIndex6 = 0;
                 slotIndex6 < (int)(sizeof(g_HudUiMgrWeaponSlots) /
                                     sizeof(g_HudUiMgrWeaponSlots[0]));
                 ++slotIndex6) {
                HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex6];
            ((HudUiElement *)(&slot.slotWidget))->Invalidate();
            g_HudUiMgr.AddChild((HudUiElement *)(&slot));
            ((HudUiElement *)(&slot.trackMarkerWidget))->SetVisible(0);
            ((HudUiElement *)(&slot.slotWidget))->SetVisible(0);
            }
        }
        g_HudUiMgrSensorTargetMarkerCount = 0;
        g_HudUiMgrWeaponState = 0;
    }

    zReader::Node *weaponPayload = HudUiZrdPayload(zReader_GetNamedNode(root, "WEAPON"));
    if (weaponPayload != 0) {
        {
        for (int index = 1; index < 10; ++index) {
            g_HudUiMgrMessages[index].LoadWeaponLayoutFromNode(
                &weaponPayload[index], (const HudUiPanelFontParams *)(&ammoFont));
        }
        }
    }

    zReader::Node *modesPayload = HudUiZrdPayload(zReader_GetNamedNode(root, "MODES"));
    if (modesPayload != 0) {
        {
        for (int index = 0; index < 4; ++index) {
            g_HudUiMgrModeCounters[index].ApplyFromLayoutNode(&modesPayload[index + 1]);
        }
        }
    }

    SetModeCounterState(0, 2);
    zReader::FreeLoadedTree(root);
    SetFloatTimerVisible(0);
    SetAuxOverlayVisible(0);
    g_HudUiMgrHudLoaded = 1;
    return 1;
}

// Reimplements 0x411750: HudUiMgr::SetNanitePanelCount
void RECOIL_FASTCALL SetNanitePanelCount(int count) {
    g_HudUiMgrNanitePanel.SetVisibleCount(count);
}

// Reimplements 0x40f1a0: HudUiMgr::SetModeCounterState
void RECOIL_FASTCALL SetModeCounterState(int counterIndex, int state) {
    if (state == 2) {
        HudUiCounter &previous = g_HudUiMgrModeCounters[g_HudUiMgrActiveModeCounterIndex];
        ((HudUiWidget *)(&previous))->SetImageBorrowedAndInvalidate(
            FieldAt<zVidImagePartial *>(&previous, 0xc0));
        g_HudUiMgrActiveModeCounterIndex = counterIndex;
    }

    HudUiCounter &counter = g_HudUiMgrModeCounters[counterIndex];
    ((HudUiWidget *)(&counter))->SetImageBorrowedAndInvalidate(
        FieldAt<zVidImagePartial *>(&counter, 0xbc + state * 4));
}

// Reimplements 0x411710: HudUiMgr::ReticleStaticAtexitStub
void RECOIL_CDECL ReticleStaticAtexitStub() {}

// Reimplements 0x411720: HudUiMgr::CopyReticleProjection
void RECOIL_FASTCALL CopyReticleProjection(float *outProjection) {
    unsigned int *const outBits = (unsigned int *)(outProjection);
    const unsigned int *const projectionBits = (const unsigned int *)(g_HudUiMgrReticleProjection);
    outBits[0] = projectionBits[0];
    outBits[1] = projectionBits[1];
    outBits[2] = projectionBits[2];
}

// Reimplements 0x411740: HudUiMgr::SetReticleMode
void RECOIL_FASTCALL SetReticleMode(int mode) {
    g_HudUiMgrReticleMode = mode;
}

// Reimplements 0x411270: HudUiMgr::UpdateTargetReticleFromCursor (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL UpdateTargetReticleFromCursor(int reticleMode,
                                                                           zVec3 *worldHitPoint,
                                                                           float normalizedX,
                                                                           float normalizedY) {
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

    reticleElement->SetPos(projectedX - g_HudUiMgrReticleWidgetHalfW,
                           projectedY - g_HudUiMgrReticleWidgetHalfH);

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

    if (IntersectRect((RECT *)(&g_HudLayoutHW.reticleClipRect), &reticleBounds,
                      (const RECT *)(zOpt::GetDisplaySection())) != 0) {
        g_HudLayoutHW.reticleClipRect.top -= g_HudUiMgrReticleWidget.GetCenterY();
        g_HudLayoutHW.reticleClipRect.bottom -= g_HudUiMgrReticleWidget.GetCenterY();
        g_HudLayoutHW.reticleClipRect.left -= g_HudUiMgrReticleWidget.GetCenterX();
        g_HudLayoutHW.reticleClipRect.right -= g_HudUiMgrReticleWidget.GetCenterX();

        reticleElement->SetPos(
            g_HudUiMgrReticleWidget.GetCenterX() + g_HudLayoutHW.reticleClipRect.left,
            g_HudUiMgrReticleWidget.GetCenterY() + g_HudLayoutHW.reticleClipRect.top);
        g_HudUiMgrReticleWidget.bltClipRectOrNull = &g_HudLayoutHW.reticleClipRect;
    }

    zProjectedPoint projectedPoint = {screenX, screenY, 0.0f};
    ScreenToWorld(&projectedPoint.x);

    HudReticlePlayerStatePartial *const playerState = (HudReticlePlayerStatePartial *)(g_GameStateOrMapTable->playerState);

    float nearClip = 0.0f;
    float farClip = 0.0f;
    zClass_Camera::gwCameraGetNearFarClip(g_MainCamera, &nearClip, &farClip);

    zVec3 nearPoint = {0};
    projectedPoint.reciprocalZ = 1.0f / nearClip;
    zMath_UnprojectPointBatchZBuf(&projectedPoint, &nearPoint, 1);

    zVec3 farPoint = {0};
    projectedPoint.reciprocalZ = 1.0f / playerState->activeAltGunController->optCatalogEntry->range;
    zMath_UnprojectPointBatchZBuf(&projectedPoint, &farPoint, 1);

    zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 0);
    if (playerState->cameraState == 7) {
        zClass_Class::gwNodeSetRaycastable(
            playerState->activeAltGunController->attachState->projectileNode, 0);
    }

    zClass_cls_di::SetStopAfterFirstHit(0x40000);
    PlayerProbeSampleCandidateBuffer rayData = {0};
    const int raycastResult = zClass_cls_di::RaycastSelectClosestHitBetweenPoints(
        g_Player_RuntimeDiScene, &nearPoint, &farPoint, &rayData);

    zClass_Class::gwNodeSetRaycastable(playerState->rootNode, 0);
    if (playerState->cameraState == 7) {
        zClass_Class::gwNodeSetRaycastable(
            playerState->activeAltGunController->attachState->projectileNode, 1);
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

        zClass_NodeFreeListSlot *const hitSlot =
            (zClass_NodeFreeListSlot *)(candidate.node);
        reticleImage = hitSlot->damageHandler != 0 ? g_HudUiMgrReticleImages[2]
                                                         : g_HudUiMgrReticleImages[0];
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
        const float maxY = (float)(renderRect->bottomExclusive) -
                           g_HudUiMgrSensorBlock.sensorClampHalfH;
        if (screenY > maxY) {
            screenY = maxY;
        }
    }

    zClipAltFloatRect targetRect = {screenX - g_HudUiMgrSensorBlock.sensorClampHalfW,
                                 screenY - g_HudUiMgrSensorBlock.sensorClampHalfH,
                                 screenX + g_HudUiMgrSensorBlock.sensorClampHalfW,
                                 screenY + g_HudUiMgrSensorBlock.sensorClampHalfH};
    zClipAlt::SetTargetRect(&targetRect, zOpt::GetReplicateMode());
    return 0;
}

// Reimplements 0x413730: HudUiMgr::DestroySensorWindow
RECOIL_NOINLINE void RECOIL_CDECL DestroySensorWindow() {
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
RECOIL_NOINLINE int RECOIL_CDECL EnableHud() {
    const int previouslyEnabled = g_HudUiMgr.enabled;
    g_HudUiMgr.SetEnabled(1);

    typedef void (RECOIL_THISCALL *LayoutEnableFn)(HudLayoutBase * self);
    ((LayoutEnableFn)(g_HudUiMgrCurrentLayout->ftable->slots[4]))(
        g_HudUiMgrCurrentLayout);

    HudUiMgrObjective::Update();
    zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);
    gAltClipPassEnabled = 1;
    return previouslyEnabled;
}

// Reimplements 0x410ed0: HudUiMgr::DisableHud
RECOIL_NOINLINE int RECOIL_CDECL DisableHud() {
    const int previouslyEnabled = g_HudUiMgr.enabled;
    DestroySensorWindow();

    {
        int slotIndex;
        for (slotIndex = 0;
             slotIndex < (int)(sizeof(g_HudUiMgrWeaponSlots) / sizeof(g_HudUiMgrWeaponSlots[0]));
             ++slotIndex) {
            HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex];
        HudUiVirtualSetVisibleRequired(&slot.trackMarkerWidget, 0);
        HudUiVirtualSetVisibleRequired(&slot.slotWidget, 0);
    }
    }

    g_HudUiMgrSensorTargetMarkerCount = 0;
    g_HudUiMgrWeaponState = 0;
    HudUiVirtualSetContainerEnabled(&g_HudUiMgr, 0);

    typedef void (RECOIL_THISCALL *LayoutDisableFn)(HudLayoutBase * self);
    ((LayoutDisableFn)(g_HudUiMgrCurrentLayout->ftable->slots[5]))(
        g_HudUiMgrCurrentLayout);

    HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveWidget, 0);
    HudUiVirtualSetVisibleRequired(g_HudUiMgrObjectiveDescTextPanel, 0);
    HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveBar, 0);
    HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveSensorRect, 0);
    HudUiVirtualSetVisibleRequired(g_HudUiMgrObjectiveSummaryTextPanel, 0);
    HudUiVirtualSetVisibleRequired(g_HudUiMgrObjectiveLabelTextPanel, 0);
    HudUiVirtualSetVisibleRequired(&g_HudUiMgrObjectiveMeter, 0);

    gAltClipPassEnabled = 0;
    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionResetFrame();
    }

    const int hudType = zOpt::GetHudTypeForCurrentHwMode();
    if (hudType == 2) {
        g_HudUiMgrLayoutDelayFrames = hudType;
    }

    HudUiVirtualSetVisibleRequired(g_HudUiMgrTimerPanel, 1);
    return previouslyEnabled;
}

// Reimplements 0x413640: HudUiMgr::ToggleHud
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_CDECL ToggleHud() {
    if (g_HudUiMgr.enabled != 0) {
        DisableHud();
    } else {
        EnableHud();
    }
    return 1;
}

// Reimplements 0x410fe0: HudUiMgr::UpdateFrame (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_CDECL UpdateFrame() {
    typedef void (RECOIL_THISCALL *NoArgFn)(void *self);
    typedef void (RECOIL_THISCALL *UpdateFn)(void *self, float deltaSeconds);
    typedef void (RECOIL_THISCALL *SetVisibleFn)(void *self, int visible);

    ((NoArgFn)(g_HudUiMgrCurrentLayout->ftable->slots[3]))(
        g_HudUiMgrCurrentLayout);

    if (g_HudUiMgr.enabled != 0) {
        if (g_HudUiMgrObjectiveState != 0) {
            HudUiMgrObjective::StartHide();
        }
    } else {
        if (g_HudUiMgrObjectiveChatComposeActive != 0) {
            const HudUiPanel_FTable * summaryFTable = (const HudUiPanel_FTable *)(g_HudUiMgrObjectiveSummaryTextPanel->vtbl);
            const HudUiPanel_FTable * descFTable = (const HudUiPanel_FTable *)(g_HudUiMgrObjectiveDescTextPanel->vtbl);
            ((NoArgFn)(summaryFTable->slots[1]))(
                g_HudUiMgrObjectiveSummaryTextPanel);
            ((NoArgFn)(descFTable->slots[1]))(g_HudUiMgrObjectiveDescTextPanel);
        }

        ((UpdateFn)(
            ((HudUiCommon_FTable **)(g_HudUiMgrTimerPanel))[0]->slots[9]))(
            g_HudUiMgrTimerPanel, g_Time_UnscaledDeltaTimeSec);
    }

    if (g_HudUiMgrObjectiveMeterFillAnimEnabled != 0) {
        HudUiMgrObjective::TickMeterFillAnimation();
    }

    g_HudSensorTracker.Update();
    zTimedTask::TickActiveList();

    ((UpdateFn)(g_HudUiMgrCurrentLayout->ftable->slots[0]))(
        g_HudUiMgrCurrentLayout, g_Time_UnscaledDeltaTimeSec);
    g_HudUiMgr.UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiTopMessageStack->base.UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiChatMessageStack->base.UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiMgrStringMenu->base.UpdateAll(g_Time_UnscaledDeltaTimeSec);

    const float sampleElapsedSec =
        FieldAt<float>(g_HudUiMgrTimerPanelFloat, 0x2a4) + g_FrameDeltaTimeSec;
    FieldAt<float>(g_HudUiMgrTimerPanelFloat, 0x2a4) = sampleElapsedSec;

    const float sampleFrameCount = FieldAt<float>(g_HudUiMgrTimerPanelFloat, 0x2ac) + 1.0f;
    FieldAt<float>(g_HudUiMgrTimerPanelFloat, 0x2ac) = sampleFrameCount;
    if (sampleElapsedSec >= 1.0f) {
        FieldAt<float>(g_HudUiMgrTimerPanelFloat, 0x2ac) = 0.0f;
        FieldAt<float>(g_HudUiMgrTimerPanelFloat, 0x2a4) = 0.0f;
        FieldAt<float>(g_HudUiMgrTimerPanelFloat, 0x2a8) =
            sampleFrameCount / sampleElapsedSec;
    }

    HudUiElement *const floatingTimerElement =
        (HudUiElement *)(g_HudUiMgrTimerPanelFloat);
    if ((floatingTimerElement->flags & 0x10) == 0) {
        ((UpdateFn)(floatingTimerElement->ftable->slots[9]))(
            floatingTimerElement, g_Time_UnscaledDeltaTimeSec);
    }

    ((UpdateFn)(g_HudUiMgrReticleWidget.ftable->slots[9]))(
        &g_HudUiMgrReticleWidget, g_Time_UnscaledDeltaTimeSec);

    {
    for (int slotIndex = 0; slotIndex < 32; ++slotIndex) {
        HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex];
        HudUiVirtualSetVisibleRequired(&slot.trackMarkerWidget, 0);
        HudUiVirtualSetVisibleRequired(&slot.slotWidget, 0);
    }
    }

    g_HudUiMgrSensorTargetMarkerCount = 0;
    g_HudUiMgrWeaponState = 0;
}

// Reimplements 0x413660: HudUiMgr::SwitchActiveDialog
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SwitchActiveDialog(HudLayoutBase *newDialog) {
    const int enabled = g_HudUiMgr.enabled;
    if (enabled != 0) {
        DisableHud();
    } else {
        g_HudUiMgrLayoutDelayFrames = 2;
    }

    typedef int (RECOIL_THISCALL *LayoutSetActiveFn)(HudLayoutBase * self, int active);

    if (g_HudUiMgrCurrentLayout != 0) {
        ((LayoutSetActiveFn)(g_HudUiMgrCurrentLayout->ftable->slots[2]))(
            g_HudUiMgrCurrentLayout, 0);
    }

    ((LayoutSetActiveFn)(newDialog->ftable->slots[2]))(newDialog, 1);
    g_HudUiMgrCurrentLayout = newDialog;

    if (enabled != 0) {
        EnableHud();
    }
}

// Reimplements 0x413770: HudUiMgr::SetFloatTimerVisible
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetFloatTimerVisible(int visible) {
    HudUiVirtualSetVisibleRequired(g_HudUiMgrTimerPanelFloat, visible != 0 ? 1 : 0);

    if (visible == 0) {
        TriggerCurrentLayoutOnActivated();
    }
}

// Reimplements 0x412620: HudUiMgr::HideTrackedProgressMeterIfOwnerMatches
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL HideTrackedProgressMeterIfOwnerMatches(void *ownerPayload) {
    HudUiSlot *const trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    if (trackedProgressSlot == 0) {
        return;
    }

    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
    if (trackNode->payload == ownerPayload) {
        HudUiVirtualSetVisibleRequired(&g_HudUiMgrSensorMeter, 0);
    }
}

// Reimplements 0x4137a0: HudUiMgr::SetAuxOverlayVisible
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetAuxOverlayVisible(int visible) {
    HudUiVirtualSetContainerEnabled(&g_HudUiMgrStringMenu->base, visible != 0 ? 1 : 0);
}

// Reimplements 0x4136b0: HudUiMgr::ApplyHudModeSwitch
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ApplyHudModeSwitch(int hudType) {
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
void RECOIL_FASTCALL ScreenToWorld(float *pointXY) {
    zOpt_ViewRectSection *const renderSection = *g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection *const displaySection = *g_zOpt_DisplaySectionOption;

    if (zOpt::GetReplicateMode() == 0) {
        return;
    }

    pointXY[0] *= 0.5f;
    pointXY[1] = (float)(renderSection->y) +
                 (pointXY[1] - (float)(displaySection->y)) * 0.5f;
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(renderSection, pointXY);
}

// Reimplements 0x40ff80: HudUiMgr::OnViewportChanged
void RECOIL_FASTCALL OnViewportChanged(const HudUiRect *hudRectOrNull,
                                       const HudUiRect *viewRectOrNull) {
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

    HudUiMgrSensor::SetViewportRect(g_HudUiMgrSensorFxRect.left, g_HudUiMgrSensorFxRect.top,
                                    g_HudUiMgrSensorFxViewportWidth,
                                    g_HudUiMgrSensorFxViewportHeight);

    if (g_HudUiMgrCurrentLayout != 0) {
        g_HudUiMgrCurrentLayout->ftable->OnActivated(g_HudUiMgrCurrentLayout);
    }

    HudUiMgrObjective::Update();

    if (g_HudUiTopMessageStack != 0) {
        g_HudUiTopMessageStack->SetTextColors(0x0020bf40, 0x0020bf40);
        g_HudUiTopMessageStack->SetXAll(zVideo::GetPrimarySurfaceWidth() / 2);
    }

    if (g_HudUiChatMessageStack != 0) {
        g_HudUiChatMessageStack->SetTextColors(0x0020bf40, 0x0020bf40);
        g_HudUiChatMessageStack->SetXAll(zVideo::GetPrimarySurfaceWidth() / 2);
        g_HudUiChatMessageStack->SetYDescending(zVideo::GetPrimarySurfaceHeight() - 0x88);
    }
}

// Reimplements 0x40ff50: HudUiMgr::ActivateHud
void RECOIL_FASTCALL ActivateHud(const HudUiRect *hudRectOrNull, const HudUiRect *viewRectOrNull) {
    OnViewportChanged(hudRectOrNull, viewRectOrNull);
    g_HudUiMgrShieldMessageWidget->viewportResetFrame = -1;
    g_HudUiMgrShieldMessageWidget->state = 0;
    g_HudUiMgrSensorBlock.state = 1;
}

// Reimplements 0x413910: HudUiMgr::EnableTopAndChatStacks
void RECOIL_CDECL EnableTopAndChatStacks() {
    g_HudUiTopMessageStack->Clear();
    HudUiVirtualSetContainerEnabled(&g_HudUiTopMessageStack->base, 1);
    g_HudUiChatMessageStack->Clear();
    HudUiVirtualSetContainerEnabled(&g_HudUiChatMessageStack->base, 1);
}

// Reimplements 0x413950: HudUiMgr::DisableTopAndChatStacks
void RECOIL_CDECL DisableTopAndChatStacks() {
    g_HudUiTopMessageStack->Clear();
    HudUiVirtualSetContainerEnabled(&g_HudUiTopMessageStack->base, 0);
    g_HudUiChatMessageStack->Clear();
    HudUiVirtualSetContainerEnabled(&g_HudUiChatMessageStack->base, 0);
}

// Reimplements 0x40f4c0: HudUiMgr::InitHudLayouts
int RECOIL_FASTCALL InitHudLayouts(const HudUiRect *displaySection,
                                            const HudUiRect *windowSection) {
    if (g_HudUiMgrHudLayoutsInitialized != 0) {
        return 1;
    }

    HudUiTimerPanelFloat *const timerPanelFloat = AllocateHudObject<HudUiTimerPanelFloat>();
    g_HudUiMgrTimerPanelFloat =
        timerPanelFloat != 0 ? timerPanelFloat->ConstructorDefault() : 0;

    HudUiStringMenu *const stringMenu = AllocateHudObject<HudUiStringMenu>();
    if (stringMenu != 0) {
        stringMenu->base.ConstructorDefault();

        stringMenu->base.vptr =
            (const HudUiContainer_FTable *)(&g_HudUiStringMenu_FTable);

        int y = 0x5f;
        {
            int itemIndex;
            for (itemIndex = 0;
                 itemIndex < (int)(sizeof(stringMenu->items) / sizeof(stringMenu->items[0]));
                 ++itemIndex) {
                HudUiPanelSimple &item = stringMenu->items[itemIndex];
                item.ConstructorDefaultThunk();

                HudUiElement *const child = (HudUiElement *)(&item);
                child->SetPos(5, y);
                stringMenu->base.AddChild(child);
                child->SetVisible(1);
                y += 0x0f;
            }
        }

        stringMenu->base.SetEnabled(1);
    }
    g_HudUiMgrStringMenu = stringMenu;

    HudUiShieldMessageWidget *const shieldMessageWidget =
        AllocateHudObject<HudUiShieldMessageWidget>();
    if (shieldMessageWidget != 0) {
        shieldMessageWidget->widget.Constructor(0);
        HudUiPanel *const percentTextPanel = (HudUiPanel *)(&shieldMessageWidget->percentTextPanel);
        percentTextPanel->ConstructorDefault(0, 0, 0);
        percentTextPanel->vtbl = &g_HudUiPanelSimple_FTable;
        percentTextPanel->SetTextColor(0x0020bf40);
        percentTextPanel->SetFont("Arial", 0x0a, 0x1f4, 6, 0, 0, 2);
        percentTextPanel->SetShadow(1, -1, -1);
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
        statsList->base.Constructor(0, 0);
        statsList->base.ftable =
            (const HudUiCommon_FTable *)(&g_HudUiStatsListElement_FTable);

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
    g_HudUiChatMessageStack =
        chatMessageStack != 0 ? chatMessageStack->Constructor() : 0;

    g_HudUiMgrHudLoaded = 0;
    g_HudUiMgrLayoutDelayFrames = 0;
    g_HudUiMgrStatsListState1 = 0;
    g_HudUiMgrStatsListState2 = 0;
    g_HudUiMgrStatsListState3 = 0;
    g_HudUiMgrStatsListState5 = 0;

    ActivateHud(displaySection, windowSection);
    g_HudUiMgrHudOriginX = displaySection->right - 0x280;
    g_HudUiMgrHudOriginY = displaySection->bottom - 0x1e0;

    g_HudUiTripletWndClassName = AfxRegisterWndClass(0x83, 0, 0, 0);

    zUtil_ZAR::RegisterSectionHandler(
        "HUDTimer", ZbdCallbackPtr(&HudUiTimerPanel::ZarWriteTimerDataCallback),
        ZbdCallbackPtr(&HudUiTimerPanel::ZarReadTimerData), 0x64, g_HudUiMgrTimerPanel);

    g_HudUiMgrHudLayoutsInitialized = 1;
    if (g_HudUiMgrStatsList != 0) {
        g_HudUiMgrStatsList->base.SetVisible(1);
        g_HudUiMgr.AddChild(&g_HudUiMgrStatsList->base);
    }
    return 1;
}

// Reimplements 0x40fbd0: HudUiMgr::ShutdownResources
void RECOIL_CDECL ShutdownResources() {
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrReticleImages[0]);
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrReticleImages[1]);
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrReticleImages[2]);

    {
        int counterIndex12;
        for (counterIndex12 = 0;
             counterIndex12 <
             (int)(sizeof(g_HudUiMgrModeCounters) / sizeof(g_HudUiMgrModeCounters[0]));
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

// Reimplements 0x4b4070: HudUiElement::Constructor
HudUiElement *RECOIL_THISCALL HudUiElement::Constructor(int initX, int initY) {
    y = initY;
    ftable = &g_HudUiCommon_FTable;
    parent = 0;
    next = 0;
    timer = 0.0f;

    typedef void (RECOIL_FASTCALL *InvalidateFn)(HudUiElement *);
    x = initX;
    ((InvalidateFn)(((const unsigned int *)ftable)[8]))(this);

    flags = 0;
    state = 0;
    SetBltSourceAndClipRect(0, 0);
    return this;
}

// Reimplements 0x4b40c0: HudUiElement::CopyConstructor
RECOIL_NOINLINE HudUiElement *RECOIL_THISCALL
HudUiElement::CopyConstructor(const HudUiElement *source) {
    ftable = &g_HudUiCommon_FTable;
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
RECOIL_NOINLINE HudUiElement *RECOIL_THISCALL HudUiElement::CopyFrom(const HudUiElement *source) {
    next = 0;
    parent = 0;
    flags = source->flags;
    state = source->state;
    FieldAt<unsigned int>(this, 0x10) = FieldAt<const unsigned int>(source, 0x10);
    x = source->x;
    y = source->y;
    bltSource = source->bltSource;
    clipRect = source->clipRect;
    return this;
}

// Reimplements 0x404d70: HudUiElement::ScalarDeletingDestructor
HudUiElement *RECOIL_THISCALL HudUiElement::ScalarDeletingDestructor(unsigned int flags) {
    ftable = &g_HudUiCommon_FTable;
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b4180: HudUiElement::Invalidate
RECOIL_NOINLINE void RECOIL_THISCALL HudUiElement::Invalidate() {
    flags |= g_HudUi_InvalidateMask;
}

// Reimplements 0x404cd0: HudUiElement::SetPos
void RECOIL_THISCALL HudUiElement::SetPos(int newX, int newY) {
    typedef void (RECOIL_FASTCALL *InvalidateFn)(HudUiElement * self);

    x = newX;
    y = newY;
    ((InvalidateFn)(ftable->slots[8]))(this);
}

// Reimplements 0x404cf0: HudUiElement::SetX
void RECOIL_THISCALL HudUiElement::SetX(int newX) {
    typedef void (RECOIL_FASTCALL *InvalidateFn)(HudUiElement * self);

    x = newX;
    ((InvalidateFn)(ftable->slots[8]))(this);
}

// Reimplements 0x404d00: HudUiElement::SetY
void RECOIL_THISCALL HudUiElement::SetY(int newY) {
    typedef void (RECOIL_FASTCALL *InvalidateFn)(HudUiElement * self);

    y = newY;
    ((InvalidateFn)(ftable->slots[8]))(this);
}

// Reimplements 0x404d20: HudUiElement::SetVisible
// BN patches only the low flag byte, then indirect-calls Invalidate through ftable slot 8.
RECOIL_NOINLINE void RECOIL_THISCALL HudUiElement::SetVisible(int visible) {
    typedef void (RECOIL_FASTCALL *InvalidateFn)(HudUiElement * self);

    if (visible != 0) {
        flags &= 0xffffffefu;
    } else {
        flags |= 0x10u;
    }

    volatile const HudUiCommon_FTable *const dispatchTable = ftable;
    ((InvalidateFn)(dispatchTable->slots[8]))(this);
}

// Reimplements 0x4b47a0: HudUiElement::ResetCommonFTable
void RECOIL_THISCALL HudUiElement::ResetCommonFTable() {
    ftable = &g_HudUiCommon_FTable;
}

// Reimplements 0x4b41e0: HudUiElement::Update
void RECOIL_THISCALL HudUiElement::Update(float deltaSeconds) {
    unsigned int currentFlags = flags;
    typedef void (RECOIL_FASTCALL *DispatchFn)(HudUiElement * self);
    typedef void (RECOIL_THISCALL *SetVisibleFn)(HudUiElement * self, int visible);

    if ((currentFlags & 0x10) == 0) {
        if ((currentFlags & 0x02) == 0) {
            ((DispatchFn)(ftable->slots[1]))(this);
        } else if ((currentFlags & 0x04) != 0) {
            ((DispatchFn)(ftable->slots[1]))(this);
            currentFlags = flags & ~0x04u;
            flags = currentFlags;
        } else if ((currentFlags & 0x08) != 0) {
            ((DispatchFn)(ftable->slots[1]))(this);
            currentFlags = flags & ~0x08u;
            flags = currentFlags;
        }

        if ((flags & 0x01) != 0) {
            timer -= deltaSeconds;
            if (timer <= 0.0f) {
                ((SetVisibleFn)(ftable->slots[24]))(this, 0);
            }
        }
    } else if ((currentFlags & 0x02) != 0) {
        if ((currentFlags & 0x04) != 0) {
            ((DispatchFn)(ftable->slots[2]))(this);
            flags &= ~0x04u;
        } else if ((currentFlags & 0x08) != 0) {
            ((DispatchFn)(ftable->slots[2]))(this);
            flags &= ~0x08u;
        }
    }
}

// Reimplements 0x4b4280: HudUiElement::SetTimer
void RECOIL_THISCALL HudUiElement::SetTimer(float duration) {
    unsigned int durationBits = 0;
    memcpy(&durationBits, &duration, sizeof(durationBits));
    FieldAt<unsigned int>(this, 0x10) = durationBits;

    if (duration >= 0.0f) {
        flags |= 0x01u;
    } else {
        flags = (flags & ~0x01u) | 0x10u;
    }
}

// Reimplements 0x4b4190: HudUiElement::SetBltSourceAndClipRect
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiElement::SetBltSourceAndClipRect(void *bltSourceOrNull, const HudUiRect *rectOrNull) {
    struct SetClipRectFTable {
        unsigned int slots[7];
        void (HudUiElement::*SetClipRect)(const HudUiRect *rect);
    };

    bltSource = bltSourceOrNull;
    const SetClipRectFTable *const dispatch = (const SetClipRectFTable *)ftable;
    (this->*dispatch->SetClipRect)(rectOrNull);
}

// Reimplements 0x4b41b0: HudUiElement::SetClipRect
void RECOIL_THISCALL HudUiElement::SetClipRect(const HudUiRect *rect) {
    if (rect == 0) {
        return;
    }

    clipRect = *rect;
}

// Reimplements 0x4b42c0: HudUiElement::GetRect
RECOIL_NOINLINE void RECOIL_THISCALL HudUiElement::GetRect(HudUiRect *outRect) {
    typedef int (RECOIL_THISCALL *GetCoordFn)(HudUiElement * self);

    const int rectX = ((GetCoordFn)(ftable->slots[25]))(this);
    outRect->left = rectX;
    outRect->right = rectX;

    const int rectY = ((GetCoordFn)(ftable->slots[26]))(this);
    outRect->top = rectY;
    outRect->bottom = rectY;
}

// Reimplements 0x404d10: HudUiElement::HitTestTrue (D:\Proj\Battlesport\hud.cpp)
// BN returns via AL only (`mov al, 1`); ignore hit-test coordinates.
unsigned char RECOIL_THISCALL HudUiElement::HitTestTrue(int px, int py) {
    (void)px;
    (void)py;
    return 1;
}

// Reimplements 0x404d50: HudUiElement::GetX
int RECOIL_THISCALL HudUiElement::GetX() {
    return x;
}

// Reimplements 0x404d60: HudUiElement::GetY
int RECOIL_THISCALL HudUiElement::GetY() {
    return y;
}

// Reimplements 0x4bffb0: HudUiPrimitiveBindTarget::SetSegmentEndpoints (HudUiBackground.cpp)
void RECOIL_THISCALL HudUiPrimitiveBindTarget::SetSegmentEndpoints(int startX,
                                                                   int startY,
                                                                   int newEndX,
                                                                   int newEndY) {
    typedef void (RECOIL_THISCALL *SetPosFn)(HudUiPrimitiveBindTarget * self, int x, int y);

    ((SetPosFn)(base.ftable->slots[0x0c / 4]))(this, startX, startY);
    endX = newEndX;
    endY = newEndY;
}

// Reimplements 0x4bc480: HudUiCircle::Constructor
HudUiCircle *RECOIL_THISCALL HudUiCircle::Constructor(int x, int y,
                                                      int circleRadius,
                                                      unsigned int circleColor565) {
    base.Constructor(x, y);
    base.ftable = &g_HudUiCircle_FTable;
    radius = circleRadius;
    const unsigned int radiusBits = (unsigned int)(circleRadius);
    radiusSquared = (int)(radiusBits * radiusBits);
    color565 = circleColor565;
    return this;
}

// Reimplements 0x404e60: HudUiCircle::HitTest
int RECOIL_THISCALL HudUiCircle::HitTest(int px, int py) {
    return HitTestCore(px, py) != 0 ? 1 : 0;
}

// Reimplements 0x4bc4e0: HudUiCircle::HitTestCore
RECOIL_NOINLINE int RECOIL_THISCALL HudUiCircle::HitTestCore(int px,
                                                                      int py) {
    const unsigned int dx = (unsigned int)(px) - (unsigned int)(base.x);
    const unsigned int dy = (unsigned int)(py) - (unsigned int)(base.y);
    const unsigned int distanceSquared = dx * dx + dy * dy;
    return (int)(distanceSquared) < radiusSquared ? 1 : 0;
}

// Reimplements 0x4bbfa0: HudUiCompositePanelVector::Clear
void RECOIL_THISCALL HudUiCompositePanelVector::Clear() {
    typedef HudUiCompositePanelEntry * (RECOIL_THISCALL *ScalarDeletingDestructorFn)(HudUiCompositePanelEntry * self, unsigned int flags);

    for (HudUiCompositePanelEntry *entry = begin; entry != end; ++entry) {
        const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(entry);
        ((ScalarDeletingDestructorFn)(ftable->slots[0]))(entry, 0);
    }

    ::operator delete(begin);
    begin = 0;
    end = 0;
    capacityEnd = 0;
}

size_t HudUiCompositePanelVectorCount(const HudUiCompositePanelVector &vector) {
    return vector.begin != 0 ? (size_t)(vector.end - vector.begin) : 0;
}

// Reimplements 0x4bbff0: HudUiCompositePanelVector::InsertCopies
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiCompositePanelVector::InsertCopies(HudUiCompositePanelEntry *insertPos,
                                        unsigned int insertCount,
                                        const HudUiCompositePanelEntry *templateEntry) {
    if (insertCount == 0) {
        return;
    }

    const size_t size = begin != 0 ? (size_t)(end - begin) : 0;
    const size_t positionIndex =
        begin != 0 && insertPos != 0 ? (size_t)(insertPos - begin) : 0;
    const size_t capacity =
        begin != 0 ? (size_t)(capacityEnd - begin) : 0;
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
    HudUiCompositePanelEntry *const newBegin = (HudUiCompositePanelEntry *)(
        ::operator new(newCapacity * sizeof(HudUiCompositePanelEntry)));
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

    typedef HudUiCompositePanelEntry * (RECOIL_THISCALL *ScalarDeletingDestructorFn)(HudUiCompositePanelEntry * self, unsigned int flags);
    for (HudUiCompositePanelEntry *entry = begin; entry != end; ++entry) {
        const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(entry);
        ((ScalarDeletingDestructorFn)(ftable->slots[0]))(entry, 0);
    }

    ::operator delete(begin);
    begin = newBegin;
    end = newBegin + size + insertCount;
    capacityEnd = newBegin + newCapacity;
}

// Reimplements 0x4bb790: HudUiCompositePanel::ConstructorWithEntryCount
RECOIL_NOINLINE HudUiCompositePanel *RECOIL_THISCALL
HudUiCompositePanel::ConstructorWithEntryCount(int entryCount) {
    HudUiPanel *const panel = (HudUiPanel *)(this);
    panel->ConstructorDefault(0, 0, 0);

    activeEntryCount = 0;
    entryVector.allocatorStorage = 0;
    entryVector.begin = 0;
    entryVector.end = 0;
    entryVector.capacityEnd = 0;
    *(const HudUiCompositePanel_FTable **)(this) = &g_HudUiCompositePanel_FTable;

    HudUiCompositePanelEntry templateEntry;
    ((HudUiTransitionTextPanel *)(&templateEntry))->Constructor();
    entryVector.InsertCopies(entryVector.end, (unsigned int)(entryCount),
                             &templateEntry);
    ((HudUiPanel *)(&templateEntry))->Destructor();

    SetTextFmt("");
    LayoutEntries(0, 0);
    ResizeEntryVectorAndRelayout(entryCount);
    HudUiVirtualSetVisibleRequired(this, 1);
    return this;
}

// Reimplements 0x4bb9f0: HudUiCompositePanel::LayoutEntries
RECOIL_NOINLINE void RECOIL_THISCALL HudUiCompositePanel::LayoutEntries(int x,
                                                                        int y) {
    typedef void (RECOIL_THISCALL *SetPosFn)(HudUiCompositePanelEntry * self, int x, int y);

    HudUiPanel *const panel = (HudUiPanel *)(this);
    HudUiElement *const element = (HudUiElement *)(this);
    element->x = x;
    element->y = y;
    element->Invalidate();

    const int entryHeight = panel->QueryTextHeight();
    int yOffset = 0;
    for (HudUiCompositePanelEntry *entry = entryVector.begin; entry != entryVector.end; ++entry) {
        const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(entry);
        ((SetPosFn)(ftable->slots[0x0c / 4]))(
            entry, element->GetX(), element->GetY() + yOffset);
        yOffset += entryHeight;
    }
}

// Reimplements 0x4bbe90: HudUiCompositePanel::ReapplyEntryCount
RECOIL_NOINLINE void RECOIL_THISCALL HudUiCompositePanel::ReapplyEntryCount() {
    ResizeEntryCount(0, (int)(HudUiCompositePanelVectorCount(entryVector)));
}

// Reimplements 0x4bbed0: HudUiCompositePanel::ResizeEntryCount
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiCompositePanel::ResizeEntryCount(int oldCount, int entryCount) {
    typedef void (RECOIL_CDECL *SetTextFmtFn)(HudUiCompositePanelEntry * self,
                                             const char *format, ...);

    int activeCount = oldCount;
    if (activeCount > entryCount) {
        activeCount = entryCount;
    }
    if (activeCount < 0) {
        activeCount = 0;
    }

    const int vectorCount =
        (int)(HudUiCompositePanelVectorCount(entryVector));
    if (entryCount > vectorCount) {
        entryCount = vectorCount;
    }

    {
    for (int index = activeCount; index < entryCount; ++index) {
        HudUiCompositePanelEntry *const entry = &entryVector.begin[index];
        const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(entry);
        ((SetTextFmtFn)(ftable->slots[0x74 / 4]))(entry, "");
        HudUiVirtualSetVisibleRequired(entry, 0);
    }
    }

    activeEntryCount = activeCount;
}

// Reimplements 0x4bbaa0: HudUiCompositePanel::SetTextFmt
void HudUiCompositePanel::SetTextFmt(const char *format, ...) {
    va_list args;
    va_start(args, format);
    SetTextFmtV(format, args);
    va_end(args);
}

// Reimplements 0x4bbac0: HudUiCompositePanel::SetTextFmtV
RECOIL_NOINLINE void RECOIL_THISCALL HudUiCompositePanel::SetTextFmtV(const char *format,
                                                                      va_list args) {
    if (entryVector.begin == 0) {
        return;
    }

    HudUiCompositePanelEntry *const entry = &entryVector.begin[activeEntryCount];
    ((HudUiPanel *)(entry))->SetTextFmtV(format, args);
    HudUiVirtualSetVisibleRequired(entry, 1);
    ScrollHistory();
}

// Reimplements 0x4bbb20: HudUiCompositePanel::ScrollHistory
RECOIL_NOINLINE void RECOIL_THISCALL HudUiCompositePanel::ScrollHistory() {
    const int vectorCount =
        (int)(HudUiCompositePanelVectorCount(entryVector));
    ++activeEntryCount;

    if (activeEntryCount >= vectorCount) {
        {
        for (int index = 0; index < vectorCount - 1; ++index) {
            HudUiCompositePanelEntry *const current = &entryVector.begin[index];
            HudUiCompositePanelEntry *const next = &entryVector.begin[index + 1];
            ((HudUiPanel *)(current))->SetText(
                ((HudUiPanel *)(next))->GetLastTextPtr());
        }
        }
        --activeEntryCount;
    }

    ((HudUiElement *)(this))->Invalidate();
}

// Reimplements 0x4bbbe0: HudUiCompositePanel::SetFont
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiCompositePanel::SetFont(const char *faceName, int height, int weight,
                             int width, int italic, int charSet,
                             int pitchAndFamily) {
    for (HudUiCompositePanelEntry *entry = entryVector.begin; entry != entryVector.end; ++entry) {
        ((HudUiPanel *)(entry))->SetFont(faceName, height, weight, width, italic,
                                                       charSet, pitchAndFamily);
    }

    HudUiPanel *const panel = (HudUiPanel *)(this);
    panel->SetFont(faceName, height, weight, width, italic, charSet, pitchAndFamily);

    HudUiElement *const element = (HudUiElement *)(this);
    LayoutEntries(element->GetX(), element->GetY());
}

// Reimplements 0x4bbca0: HudUiCompositePanel::ResizeEntryVectorAndRelayout
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiCompositePanel::ResizeEntryVectorAndRelayout(int entryCount) {
    const int oldCount =
        (int)(HudUiCompositePanelVectorCount(entryVector));

    if (entryCount != oldCount) {
        HudUiCompositePanelEntry templateEntry;
        ((HudUiTransitionTextPanel *)(&templateEntry))->Constructor();

        if (entryCount > oldCount) {
            entryVector.InsertCopies(entryVector.end, (unsigned int)(entryCount - oldCount),
                                     &templateEntry);
        } else {
            HudUiCompositePanelEntry *const newEnd = entryVector.begin + entryCount;
            typedef HudUiCompositePanelEntry * (RECOIL_THISCALL *ScalarDeletingDestructorFn)(HudUiCompositePanelEntry * self, unsigned int flags);
            for (HudUiCompositePanelEntry *entry = newEnd; entry != entryVector.end; ++entry) {
                const HudUiCommon_FTable *const ftable = *(const HudUiCommon_FTable *const *)(entry);
                ((ScalarDeletingDestructorFn)(ftable->slots[0]))(entry, 0);
            }
            entryVector.end = newEnd;
        }

        ((HudUiPanel *)(&templateEntry))->Destructor();
        ResizeEntryCount(oldCount, entryCount);
    } else {
        ReapplyEntryCount();
    }

    HudUiElement *const element = (HudUiElement *)(this);
    LayoutEntries(element->GetX(), element->GetY());
}

// Reimplements 0x4bc3a0: HudUiCompositePanelEntry::AssignCopy
HudUiCompositePanelEntry *RECOIL_THISCALL
HudUiCompositePanelEntry::AssignCopy(const HudUiCompositePanelEntry *source) {
    ((HudUiPanel *)(this))->ConstructorCopy(
        (const HudUiPanel *)(source));
    FieldAt<unsigned int>(this, 0x2a4) = FieldAt<const unsigned int>(source, 0x2a4);
    FieldAt<unsigned int>(this, 0x2a8) = FieldAt<const unsigned int>(source, 0x2a8);
    FieldAt<unsigned int>(this, 0x2ac) = FieldAt<const unsigned int>(source, 0x2ac);
    FieldAt<unsigned int>(this, 0x2b0) = FieldAt<const unsigned int>(source, 0x2b0);
    FieldAt<unsigned int>(this, 0x2b4) = FieldAt<const unsigned int>(source, 0x2b4);
    FieldAt<unsigned int>(this, 0x2b8) = FieldAt<const unsigned int>(source, 0x2b8);
    FieldAt<unsigned int>(this, 0x2bc) = FieldAt<const unsigned int>(source, 0x2bc);
    return this;
}

// Reimplements 0x4bc410: HudUiCompositePanelEntry::ConstructorCopy
HudUiCompositePanelEntry *RECOIL_THISCALL
HudUiCompositePanelEntry::ConstructorCopy(const HudUiCompositePanelEntry *source) {
    ((HudUiPanel *)(this))->CopyConstructCore(
        (const HudUiPanel *)(source));
    FieldAt<unsigned int>(this, 0x2a4) = FieldAt<const unsigned int>(source, 0x2a4);
    FieldAt<unsigned int>(this, 0x2a8) = FieldAt<const unsigned int>(source, 0x2a8);
    FieldAt<unsigned int>(this, 0x2ac) = FieldAt<const unsigned int>(source, 0x2ac);
    FieldAt<unsigned int>(this, 0x2b0) = FieldAt<const unsigned int>(source, 0x2b0);
    FieldAt<unsigned int>(this, 0x2b4) = FieldAt<const unsigned int>(source, 0x2b4);
    FieldAt<unsigned int>(this, 0x2b8) = FieldAt<const unsigned int>(source, 0x2b8);
    FieldAt<unsigned int>(this, 0x2bc) = FieldAt<const unsigned int>(source, 0x2bc);
    FieldAt<const HudUiPanel_FTable *>(this, 0x00) = &g_HudUiTransitionTextPanel_FTable;
    return this;
}

// Reimplements 0x4bc320: HudUiCompositePanelEntry::ConstructorCopyRange
HudUiCompositePanelEntry *RECOIL_FASTCALL HudUiCompositePanelEntry::ConstructorCopyRange(
    const HudUiCompositePanelEntry *sourceBegin, const HudUiCompositePanelEntry *sourceEnd,
    HudUiCompositePanelEntry *destBegin) {
    HudUiCompositePanelEntry *dest = destBegin;
    for (const HudUiCompositePanelEntry *source = sourceBegin; source != sourceEnd;
         ++source, ++dest) {
        ((HudUiPanel *)(dest))->ConstructorCopy((const HudUiPanel *)(source));
        FieldAt<unsigned int>(dest, 0x2a4) = FieldAt<const unsigned int>(source, 0x2a4);
        FieldAt<unsigned int>(dest, 0x2a8) = FieldAt<const unsigned int>(source, 0x2a8);
        FieldAt<unsigned int>(dest, 0x2ac) = FieldAt<const unsigned int>(source, 0x2ac);
        FieldAt<unsigned int>(dest, 0x2b0) = FieldAt<const unsigned int>(source, 0x2b0);
        FieldAt<unsigned int>(dest, 0x2b4) = FieldAt<const unsigned int>(source, 0x2b4);
        FieldAt<unsigned int>(dest, 0x2b8) = FieldAt<const unsigned int>(source, 0x2b8);
        FieldAt<unsigned int>(dest, 0x2bc) = FieldAt<const unsigned int>(source, 0x2bc);
    }

    return dest;
}

// Reimplements 0x4bb0c0: HudUiFlashPanel::ComputeFlashBlendColor
unsigned int RECOIL_FASTCALL HudUiFlashPanel::ComputeFlashBlendColor(unsigned int color0,
                                                                      unsigned int color1,
                                                                      float blend) {
    const double blendValue = (double)(blend);
    if (!(blendValue >= 0.001)) {
        return color0;
    }
    if (blendValue > 0.999) {
        return color1;
    }

    const double inverseBlend = 1.0 - blendValue;
    const unsigned int blue =
        (unsigned int)(
            (int)((double)(color0 & 0xffu) * inverseBlend +
                             (double)(color1 & 0xffu) * blendValue)) &
        0xffu;
    const unsigned int green =
        (unsigned int)(
            (int)((double)((color0 >> 8) & 0xffu) * inverseBlend +
                             (double)((color1 >> 8) & 0xffu) * blendValue)) &
        0xffu;
    const unsigned int red =
        (unsigned int)(
            (int)((double)((color0 >> 16) & 0xffu) * inverseBlend +
                             (double)((color1 >> 16) & 0xffu) * blendValue)) &
        0xffu;
    return (red << 16) | (green << 8) | blue;
}

// Reimplements 0x4bc780: HudUiContainer::ConstructorDefault
RECOIL_NOINLINE HudUiContainer *RECOIL_THISCALL HudUiContainer::ConstructorDefault() {
    vptr = &g_HudUiContainer_FTable;
    (this->*(vptr->setEnabled))(0);
    childHead = 0;
    childTail = 0;
    return this;
}

// Reimplements 0x4bc7b0: HudUiContainer::DestructorCore
void RECOIL_THISCALL HudUiContainer::DestructorCore() {
    vptr = &g_HudUiContainer_FTable;
}

// Reimplements 0x40d9d0: HudUiContainer::SetEnabled
RECOIL_NOINLINE void RECOIL_THISCALL HudUiContainer::SetEnabled(int enabledValue) {
    enabled = enabledValue;
}

// Reimplements 0x4bc7c0: HudUiContainer::AddChild
RECOIL_NOINLINE int RECOIL_THISCALL HudUiContainer::AddChild(HudUiElement *child) {
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
RECOIL_NOINLINE int RECOIL_THISCALL
HudUiContainer::FindChildWithPrev(HudUiElement *child, HudUiElement **previousOut) {
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
int RECOIL_THISCALL HudUiContainer::RemoveChild(HudUiElement *child) {
    HudUiElement *previous = child;
    if (FindChildWithPrev(child, &previous) == 0) {
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

// Reimplements 0x4bc8d0: HudUiContainer::SetChildFlags
void RECOIL_THISCALL HudUiContainer::SetChildFlags(unsigned int childFlags) {
    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        child->flags = childFlags | (child->flags & 0x10u);
    }
}

// Reimplements 0x4bc900: HudUiContainer::UpdateAll
RECOIL_NOINLINE void RECOIL_THISCALL HudUiContainer::UpdateAll(float deltaSeconds) {
    if (enabled == 0) {
        return;
    }

    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        typedef void (RECOIL_THISCALL *UpdateFn)(HudUiElement *, float);
        ((UpdateFn)(child->ftable->slots[9]))(child, deltaSeconds);
    }
}

struct HudUiElementInvalidateDispatch {
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2();
    virtual void Slot3();
    virtual void Slot4();
    virtual void Slot5();
    virtual void Slot6();
    virtual void Slot7();
    virtual void Invalidate();
};

// Reimplements 0x4ba3a0: HudUiContainer::InvalidateChildren
RECOIL_NOINLINE void RECOIL_THISCALL HudUiContainer::InvalidateChildren() {
    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        ((HudUiElementInvalidateDispatch *)child)->Invalidate();
    }
}

// Reimplements 0x42ee40: HudUiBackgroundContainer::SetEnabled
void RECOIL_THISCALL HudUiBackgroundContainer::SetEnabled(int enabled) {
    base.enabled = enabled;
}

// Reimplements 0x409040: HudUiCreditsPanel::Constructor
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
HudUiCreditsPanel *RECOIL_THISCALL HudUiCreditsPanel::Constructor()
{
    base.Constructor();

    backButton.Constructor();
    backButton.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiCreditsButtonQueueExitOnly_Vtbl);

    quitButton.Constructor();
    quitButton.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiCreditsButtonQueueExitAndLeaveNetwork_Vtbl);

    creditsScreen.base.Constructor();
#if defined(_MSC_VER) && _MSC_VER < 1200
    char allocatorProxy;
#else
    char allocatorProxy = 0;
#endif
    creditsScreen.rows.allocatorProxy = allocatorProxy;
    creditsScreen.rows.begin = 0;
    creditsScreen.rows.end = 0;
    creditsScreen.rows.cap = 0;
    creditsScreen.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiCreditsPanel_FTable.SecondaryAction);

    base.base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiCreditsPanel_FTable);
    fadeProgress = 0.0f;
    fadeStep = 0.05f;

    zReader::Node *const loadedSection = base.LoadFromZrd("dialog.zrd", "CREDITSPANEL", 0);
    if (loadedSection != 0)
    {
        HudUiWidget *button = (HudUiWidget *)(&backButton);
        const char *buttonName = "BACK";
        if (g_RecoilApp_QuitAfterCredits != 0)
        {
            button = (HudUiWidget *)(&quitButton);
            buttonName = "QUIT";
        }

        base.BindWidgetByName(loadedSection, button, buttonName);
        base.BindWidgetByName(loadedSection, (HudUiWidget *)(&creditsScreen),
                              "CREDITS_SCREEN");
        base.FreeLoadedTreeRoots(0);
    }

    creditsScreen.base.base.flags =
        (unsigned int)((unsigned char)(creditsScreen.base.base.flags) & 0x10u);
    return this;
}

// Reimplements 0x409550: HudUiZrdScrollingText::OnActivateResetOwnerFade
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp) owner is HudUiCreditsPanel*; fadeProgress at +0xad58.
void RECOIL_THISCALL HudUiZrdScrollingText::OnActivateResetOwnerFade() {
    float *const ownerFadeProgress =
        (float *)((unsigned char *)(base.owner) + 0xad58u);
    *ownerFadeProgress = 0.0f;
}

// Reimplements 0x409570: HudUiZrdScrollingText::LoadFromZrd
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
int RECOIL_THISCALL HudUiZrdScrollingText::LoadFromZrd(zReader::Node *zrdSection,
                                                       void *ownerDialog)
{
    base.LoadFromZrd(zrdSection, ownerDialog);

    zReader::Node *const rectNode = zReader_GetNamedNode(zrdSection, "RECT");
    zReader::Node *const rectBase = ZrdArrayBase(rectNode);
    if (rectBase != 0)
    {
        zReader::Node *const topLeft = ZrdArrayBase(&rectBase[1]);
        zReader::Node *const bottomRight = ZrdArrayBase(&rectBase[2]);
        rect.left = ZrdArrayInt(topLeft, 1, 0) + base.originX;
        rect.top = ZrdArrayInt(topLeft, 2, 0) + base.originY;
        rect.right = ZrdArrayInt(bottomRight, 1, 0) + base.originX;
        rect.bottom = ZrdArrayInt(bottomRight, 2, 0) + base.originY;
    }

    zReader::Node *const scrollRateNode = zReader_GetNamedNode(zrdSection, "SCROLL_RATE");
    if (scrollRateNode != 0)
    {
        OwnerField<float>(ownerDialog, 0xa94c) = scrollRateNode->value.f32;
    }

    zReader::Node *const scrollingTextNode =
        zReader_GetNamedNode(zrdSection, "SCROLLING_TEXT");
    zReader::Node *const scrollingTextBase = ZrdArrayBase(scrollingTextNode);
    if (scrollingTextBase == 0)
    {
        return 1;
    }

    HudUiPanelSpan templateSpan;
    templateSpan.allocatorProxy = 0;
    templateSpan.begin = 0;
    templateSpan.end = 0;
    templateSpan.cap = 0;

    const int rowCount = ZrdArrayCount(scrollingTextBase);
    for (int rowIndex = 1; rowIndex < rowCount; ++rowIndex)
    {
        zReader::Node *const rowBase = ZrdArrayBase(&scrollingTextBase[rowIndex]);
        const int labelCount = ZrdArrayCount(rowBase);

        HudUiPanelLayoutEntry *const resetEnd =
            HudUiPanelLayoutEntry::CopyAssignRange(templateSpan.end, templateSpan.end,
                                                   templateSpan.begin);
        HudUiPanelLayoutEntry::DestroyRange(resetEnd, templateSpan.end);
        templateSpan.end = resetEnd;

        for (int labelIndex = 1; labelIndex < labelCount; ++labelIndex)
        {
            zReader::Node *const labelBase = ZrdArrayBase(&rowBase[labelIndex]);
            const char *const key = ZrdArrayString(labelBase, 1);
            const char *const text = zLoc::ResolveMessageKeyOrFallback(key);
            const int layoutX = ZrdArrayInt(labelBase, 2, 0);
            const int layoutY = ZrdArrayInt(labelBase, 3, 0);
            const int styleIndex = ZrdArrayInt(labelBase, 4, 0);

            HudUiPanelLayoutEntry templateEntry;
            templateEntry.panel.ConstructorDefault(0, 0, 0);
            templateEntry.panel.SetTextFmt("%s", text);
            templateEntry.layoutX = layoutX;
            templateEntry.layoutY = layoutY;

            ApplyHudFontStyleTextOnly(&templateEntry.panel,
                                      HudUiZrdOwnerFontStyle(base.owner, styleIndex));
            templateSpan.InsertN(templateSpan.end, 1, &templateEntry);
            templateEntry.panel.Destructor();
        }

        rows.InsertN(rows.end, 1, &templateSpan);
    }

    totalHeight = 0;
    HudUiPanelSpan *row = rows.begin;
    while (row != rows.end)
    {
        int rowHeight = 0;
        HudUiPanelLayoutEntry *entry = row->begin;
        while (entry != row->end)
        {
            const int entryBottom = entry->panel.QueryTextHeight() + entry->layoutY;
            if (entryBottom > rowHeight)
            {
                rowHeight = entryBottom;
            }

            ++entry;
        }

        entry = row->begin;
        while (entry != row->end)
        {
            entry->layoutY += totalHeight;
            ++entry;
        }

        totalHeight += rowHeight;
        ++row;
    }

    HudUiPanelLayoutEntry *entry = templateSpan.begin;
    while (entry != templateSpan.end)
    {
        entry->panel.Destructor();
        ++entry;
    }

    ::operator delete(templateSpan.begin);
    return 1;
}

// Reimplements 0x409410: HudUiZrdScrollingText::Update
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void RECOIL_THISCALL HudUiZrdScrollingText::Update(float deltaSeconds)
{
    ((HudUiElement *)(this))->Update(deltaSeconds);

    HudUiPanelSpan *row = rows.begin;
    while (row != rows.end)
    {
        HudUiPanelLayoutEntry *entry = row->begin;
        while (entry != row->end)
        {
            typedef void (RECOIL_THISCALL *UpdateFn)(HudUiPanel * self, float deltaSeconds);
            const HudUiPanel_FTable *const ftable =
                *(const HudUiPanel_FTable *const *)(&entry->panel);
            ((UpdateFn)(ftable->slots[9]))(&entry->panel, deltaSeconds);
            ++entry;
        }

        ++row;
    }
}

// Reimplements 0x409470: HudUiZrdScrollingText::UpdateScrollPositions
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void RECOIL_THISCALL HudUiZrdScrollingText::UpdateScrollPositions(float scrollProgress)
{
    const int left = rect.left;
    const int scrollY = (int)((float)(rect.top - totalHeight) * scrollProgress +
                              (1.0f - scrollProgress) * (float)(rect.bottom));

    HudUiPanelSpan *row = rows.begin;
    while (row != rows.end)
    {
        HudUiPanelLayoutEntry *entry = row->begin;
        while (entry != row->end)
        {
            const int y = entry->layoutY + scrollY;
            HudUiVirtualSetPosRequired(&entry->panel, entry->layoutX + left, y);
            if (y > rect.top && y + entry->panel.QueryTextHeight() < rect.bottom)
            {
                HudUiVirtualSetVisibleRequired(&entry->panel, 1);
            }
            else
            {
                HudUiVirtualSetVisibleRequired(&entry->panel, 0);
            }

            ++entry;
        }

        ++row;
    }
}

// Reimplements 0x409ef0: HudUiPanel::DestructorCallback
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void RECOIL_STDCALL HudUiPanel::DestructorCallback(HudUiPanel *panel)
{
    panel->DestructorThunk();
}

// Reimplements 0x4091e0: HudUiZrdScrollingText::Destructor
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void RECOIL_THISCALL HudUiZrdScrollingText::Destructor()
{
    HudUiPanelSpan *row = rows.begin;
    while (row != rows.end)
    {
        HudUiPanelLayoutEntry *entry = row->begin;
        while (entry != row->end)
        {
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

    base.DestructorCore();
}

// Reimplements 0x409360: HudUiZrdScrollingText::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
HudUiZrdScrollingText *RECOIL_THISCALL
HudUiZrdScrollingText::ScalarDeletingDestructor(unsigned int flags)
{
    HudUiZrdScrollingText *self = this;
    Destructor();
    if ((flags & 1) != 0)
    {
        ::operator delete(self);
    }

    return self;
}

// Reimplements 0x409380: HudUiCreditsPanel::UpdateFadeAndExit
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void RECOIL_THISCALL HudUiCreditsPanel::UpdateFadeAndExit(float deltaSeconds)
{
    creditsScreen.UpdateScrollPositions(fadeProgress);
    fadeProgress += fadeStep * deltaSeconds;
    base.Update(deltaSeconds);

    if (fadeProgress < 1.0f)
    {
        return;
    }

    if (g_RecoilApp_QuitAfterCredits != 0)
    {
        g_RecoilApp.QueueExitCurrentState(1);
        g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_leaveNetworkState_1d0.base, 0);
        return;
    }

    g_RecoilApp.QueueExitCurrentState(0);
}

// Reimplements 0x4092a0: HudUiCreditsPanel::Destructor
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void RECOIL_THISCALL HudUiCreditsPanel::Destructor()
{
    HudUiPanelSpan *row = creditsScreen.rows.begin;
    while (row != creditsScreen.rows.end)
    {
        row->DestroyAndFree();
        ++row;
    }

    ::operator delete(creditsScreen.rows.begin);
    creditsScreen.rows.begin = 0;
    creditsScreen.rows.end = 0;
    creditsScreen.rows.cap = 0;

    creditsScreen.base.DestructorCore();
    quitButton.DestructorCore();
    backButton.DestructorCore();
    base.Destructor();
}

// Reimplements 0x4091c0: HudUiCreditsPanel::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
HudUiCreditsPanel *RECOIL_THISCALL
HudUiCreditsPanel::ScalarDeletingDestructor(unsigned int flags)
{
    HudUiCreditsPanel *self = this;
    Destructor();
    if ((flags & 1) != 0)
    {
        ::operator delete(self);
    }

    return self;
}

// Reimplements 0x40a210: HudUiPanelLayoutEntry::CopyConstruct
// (D:\Proj\Battlesport\HudUiPanel.cpp)
HudUiPanelLayoutEntry *RECOIL_THISCALL
HudUiPanelLayoutEntry::CopyConstruct(const HudUiPanelLayoutEntry *source)
{
    panel.CopyConstructCore(&source->panel);
    layoutX = source->layoutX;
    layoutY = source->layoutY;
    return this;
}

// Reimplements 0x40a1e0: HudUiPanelLayoutEntry::CopyAssign
// (D:\Proj\Battlesport\HudUiPanel.cpp)
HudUiPanelLayoutEntry *RECOIL_THISCALL
HudUiPanelLayoutEntry::CopyAssign(const HudUiPanelLayoutEntry *source)
{
    panel.ConstructorCopy(&source->panel);
    layoutX = source->layoutX;
    layoutY = source->layoutY;
    return this;
}

// Reimplements 0x40a170: HudUiPanelLayoutEntry::CopyAssignRange
// (D:\Proj\Battlesport\HudUiPanel.cpp)
HudUiPanelLayoutEntry *RECOIL_FASTCALL
HudUiPanelLayoutEntry::CopyAssignRange(const HudUiPanelLayoutEntry *sourceStart,
                                       const HudUiPanelLayoutEntry *sourceEnd,
                                       HudUiPanelLayoutEntry *dest)
{
    HudUiPanelLayoutEntry *out = dest;
    const HudUiPanelLayoutEntry *source = sourceStart;
    while (source != sourceEnd)
    {
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
void RECOIL_STDCALL HudUiPanelLayoutEntry::DestroyRange(HudUiPanelLayoutEntry *start,
                                                        HudUiPanelLayoutEntry *end)
{
    HudUiPanelLayoutEntry *entry = start;
    while (entry != end)
    {
        entry->panel.DestructorThunk();
        ++entry;
    }
}

// Reimplements 0x409910: HudUiPanelSpan::Clear
// (D:\Proj\Battlesport\HudUiPanel.cpp)
void RECOIL_THISCALL HudUiPanelSpan::Clear()
{
    HudUiPanelLayoutEntry *entry = begin;
    while (entry != end)
    {
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
HudUiPanelSpan *RECOIL_THISCALL HudUiPanelSpan::CopyInit(const HudUiPanelSpan *source)
{
    allocatorProxy = (allocatorProxy & 0xffffff00) | (source->allocatorProxy & 0xff);

    const size_t count = source->begin != 0 ? (size_t)(source->end - source->begin) : 0;
    HudUiPanelLayoutEntry *const newBegin =
        (HudUiPanelLayoutEntry *)(::operator new(count * sizeof(HudUiPanelLayoutEntry)));
    HudUiPanelLayoutEntry *dest = newBegin;

    const HudUiPanelLayoutEntry *sourceEntry = source->begin;
    while (sourceEntry != source->end)
    {
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
HudUiPanelSpan *RECOIL_THISCALL HudUiPanelSpan::CopyFrom(const HudUiPanelSpan *source)
{
    if (this == source)
    {
        return this;
    }

    const size_t sourceCount =
        source->begin != 0 ? (size_t)(source->end - source->begin) : 0;
    const size_t currentCount = begin != 0 ? (size_t)(end - begin) : 0;
    const size_t capacity = begin != 0 ? (size_t)(cap - begin) : 0;

    if (sourceCount <= currentCount)
    {
        HudUiPanelLayoutEntry *dest = begin;
        const HudUiPanelLayoutEntry *sourceEntry = source->begin;
        while (sourceEntry != source->end)
        {
            dest->CopyAssign(sourceEntry);
            ++sourceEntry;
            ++dest;
        }

        HudUiPanelLayoutEntry *oldEntry = dest;
        while (oldEntry != end)
        {
            oldEntry->panel.ScalarDeletingDestructor(0);
            ++oldEntry;
        }

        end = begin + sourceCount;
        return this;
    }

    if (sourceCount <= capacity)
    {
        HudUiPanelLayoutEntry *dest = begin;
        const HudUiPanelLayoutEntry *sourceEntry = source->begin;
        for (size_t i = 0; i < currentCount; ++i)
        {
            dest->CopyAssign(sourceEntry);
            ++sourceEntry;
            ++dest;
        }

        while (sourceEntry != source->end)
        {
            dest->CopyConstruct(sourceEntry);
            ++sourceEntry;
            ++dest;
        }

        end = begin + sourceCount;
        return this;
    }

    HudUiPanelLayoutEntry *oldEntry = begin;
    while (oldEntry != end)
    {
        oldEntry->panel.DestructorThunk();
        ++oldEntry;
    }

    ::operator delete(begin);

    HudUiPanelLayoutEntry *const newBegin =
        (HudUiPanelLayoutEntry *)(::operator new(sourceCount * sizeof(HudUiPanelLayoutEntry)));
    HudUiPanelLayoutEntry *dest = newBegin;
    const HudUiPanelLayoutEntry *sourceEntry = source->begin;
    while (sourceEntry != source->end)
    {
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
void RECOIL_THISCALL HudUiPanelSpan::InsertN(HudUiPanelLayoutEntry *insertPos,
                                             unsigned int count,
                                             const HudUiPanelLayoutEntry *templatePanel)
{
    if (count == 0)
    {
        return;
    }

    const size_t size = begin != 0 ? (size_t)(end - begin) : 0;
    const size_t positionIndex =
        begin != 0 && insertPos != 0 ? (size_t)(insertPos - begin) : 0;
    const size_t capacity = begin != 0 ? (size_t)(cap - begin) : 0;
    const size_t tailCount = size - positionIndex;

    if (size + count <= capacity)
    {
        if (tailCount >= count)
        {
            HudUiPanelLayoutEntry *source = end - count;
            HudUiPanelLayoutEntry *dest = end;
            while (source != end)
            {
                dest->CopyConstruct(source);
                ++source;
                ++dest;
            }

            source = end - count;
            dest = end;
            while (source != begin + positionIndex)
            {
                --source;
                --dest;
                dest->CopyAssign(source);
            }

            for (unsigned int i = 0; i < count; ++i)
            {
                begin[positionIndex + i].CopyAssign(templatePanel);
            }
        }
        else
        {
            HudUiPanelLayoutEntry *dest = end;
            for (unsigned int i = 0; i < count - tailCount; ++i)
            {
                dest->CopyConstruct(templatePanel);
                ++dest;
            }

            for (HudUiPanelLayoutEntry *source = begin + positionIndex; source != end;
                 ++source, ++dest)
            {
                dest->CopyConstruct(source);
            }

            for (HudUiPanelLayoutEntry *entry = begin + positionIndex; entry != end; ++entry)
            {
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

    for (size_t prefixIndex = 0; prefixIndex < positionIndex; ++prefixIndex, ++dest)
    {
        dest->CopyConstruct(&begin[prefixIndex]);
    }

    for (unsigned int insertIndex = 0; insertIndex < count; ++insertIndex, ++dest)
    {
        dest->CopyConstruct(templatePanel);
    }

    for (size_t suffixIndex = positionIndex; suffixIndex < size; ++suffixIndex, ++dest)
    {
        dest->CopyConstruct(&begin[suffixIndex]);
    }

    HudUiPanelLayoutEntry *entry = begin;
    while (entry != end)
    {
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
void RECOIL_THISCALL HudUiPanelSpanVec::InsertN(HudUiPanelSpan *insertPos,
                                                unsigned int count,
                                                const HudUiPanelSpan *templateSpan)
{
    if (count == 0)
    {
        return;
    }

    const size_t size = begin != 0 ? (size_t)(end - begin) : 0;
    const size_t positionIndex =
        begin != 0 && insertPos != 0 ? (size_t)(insertPos - begin) : 0;
    const size_t capacity = begin != 0 ? (size_t)(cap - begin) : 0;
    const size_t tailCount = size - positionIndex;

    if (size + count <= capacity)
    {
        if (tailCount >= count)
        {
            HudUiPanelSpan *source = end - count;
            HudUiPanelSpan *dest = end;
            while (source != end)
            {
                dest->CopyInit(source);
                ++source;
                ++dest;
            }

            source = end - count;
            dest = end;
            while (source != begin + positionIndex)
            {
                --source;
                --dest;
                dest->CopyFrom(source);
            }

            for (unsigned int i = 0; i < count; ++i)
            {
                begin[positionIndex + i].CopyFrom(templateSpan);
            }
        }
        else
        {
            HudUiPanelSpan *dest = end;
            for (unsigned int i = 0; i < count - tailCount; ++i)
            {
                dest->CopyInit(templateSpan);
                ++dest;
            }

            for (HudUiPanelSpan *source = begin + positionIndex; source != end; ++source, ++dest)
            {
                dest->CopyInit(source);
            }

            for (HudUiPanelSpan *span = begin + positionIndex; span != end; ++span)
            {
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

    for (size_t prefixIndex = 0; prefixIndex < positionIndex; ++prefixIndex, ++dest)
    {
        dest->CopyInit(&begin[prefixIndex]);
    }

    for (unsigned int insertIndex = 0; insertIndex < count; ++insertIndex, ++dest)
    {
        dest->CopyInit(templateSpan);
    }

    for (size_t suffixIndex = positionIndex; suffixIndex < size; ++suffixIndex, ++dest)
    {
        dest->CopyInit(&begin[suffixIndex]);
    }

    HudUiPanelSpan *span = begin;
    while (span != end)
    {
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
void RECOIL_THISCALL HudUiPanelSpan::DestroyAndFree()
{
    HudUiPanelLayoutEntry *entry = begin;
    while (entry != end)
    {
        entry->panel.DestructorThunk();
        ++entry;
    }

    ::operator delete(begin);
    begin = 0;
    end = 0;
    cap = 0;
}

// Reimplements 0x4bc510: HudUiBackgroundContainer::Constructor
HudUiBackgroundContainer *RECOIL_THISCALL
HudUiBackgroundContainer::Constructor(int initFlag) {
    base.ConstructorDefault();
    base.vptr = (const HudUiContainer_FTable *)(&g_HudUiBackgroundContainer_FTable);
    captureTransitionMask = initFlag;
    inputFocusElement = 0;
    return this;
}

// Reimplements 0x4bc540: HudUiBackgroundContainer::Destructor
void RECOIL_THISCALL HudUiBackgroundContainer::Destructor() {
    base.vptr = (const HudUiContainer_FTable *)(&g_HudUiBackgroundContainer_FTable);
    base.DestructorCore();
}

// Reimplements 0x4bc550: HudUiBackgroundContainer::SetInputFocus
void RECOIL_THISCALL HudUiBackgroundContainer::SetInputFocus(HudUiElement *element) {
    inputFocusElement = element;
}

// Reimplements 0x4bc560: HudUiBackgroundContainer::GetInputFocus
HudUiElement *RECOIL_THISCALL HudUiBackgroundContainer::GetInputFocus() {
    return inputFocusElement;
}

// Reimplements 0x4b9540: HudUiBackground::Constructor
// (D:\Proj\Battlesport\hudui_background.cpp)
HudUiBackground *RECOIL_THISCALL HudUiBackground::Constructor() {
    base.Constructor(1);

    cursorWidget.MemberConstructorLocal(0, 1);
    cursorWidget.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiBackgroundCursorWidget_MemberFTable);

    {
    for (int index = 0; index < 20; ++index) {
        backgroundImageWidgets[index].CtorDefaultThunk();
    }
    }

    {
    for (int index = 0; index < 10; ++index) {
        backgroundVideoWidgets[index].Constructor();
    }
    }

    {
    for (int index = 0; index < 20; ++index) {
        fontStyles[index].Constructor();
    }
    }

    {
    for (int index = 0; index < 50; ++index) {
        backgroundTextPanels[index].Constructor();
    }
    }

    base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiBackground_FTable);
    primaryClipImage = 0;
    capturedCompositeImage = 0;

    {
    for (int index = 0; index < 10; ++index) {
        backgroundSounds[index].sample = 0;
        backgroundSounds[index].volume = 1.0f;
        backgroundSounds[index].playHandle = 0;
    }
    }

    int vmodeDefault = 5;
    zOptionEntryPartial *vmodeOption = zGame::Options_FindOption("VMode");
    int vmode = vmodeOption != 0 ? vmodeOption->payloadOrBuffer : vmodeDefault;

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

    return this;
}

// Reimplements 0x4b9760: HudUiBackground::Destructor
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackground::Destructor() {
    base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiBackground_FTable);

    if (primaryClipImage != 0) {
        zVid_Image::ReleaseIfNotDefault(primaryClipImage);
        primaryClipImage = 0;
    }

    if (capturedCompositeImage != 0) {
        zVid_Image::ReleaseIfNotDefault(capturedCompositeImage);
        capturedCompositeImage = 0;
    }

    {
    for (int index = 50; index > 0; --index) {
        ((HudUiPanel *)(&backgroundTextPanels[index - 1]))->Destructor();
    }
    }

    {
    for (int index = 20; index > 0; --index) {
        fontStyles[index - 1].Destructor();
    }
    }

    {
    for (int index = 10; index > 0; --index) {
        backgroundVideoWidgets[index - 1].Destructor();
    }
    }

    {
    for (int index = 20; index > 0; --index) {
        backgroundImageWidgets[index - 1].DestructorCore();
    }
    }

    cursorWidget.DestructorCore();
    base.Destructor();
}

// Reimplements 0x4ba380: HudUiDialogController::BlitOwnedSurfaceToPrimary
void RECOIL_THISCALL HudUiDialogController::BlitOwnedSurfaceToPrimary() {
    if (capturedImage != 0) {
        zVid_Image::BlitToActiveTarget(capturedImage, 0, 0, 0, 0);
    }
}

unsigned int HudUiReadPackedColor(zReader::Node *colorBase) {
    if (colorBase == 0) {
        return 0;
    }

    zReader::Node *rgbBase = colorBase;
    if (colorBase[1].type == zReader::ZRDR_NODE_ARRAY) {
        rgbBase = ZrdArrayBase(&colorBase[1]);
    }

    const unsigned int red = (unsigned char)(ZrdArrayInt(rgbBase, 1, 0));
    const unsigned int green = (unsigned char)(ZrdArrayInt(rgbBase, 2, 0));
    const unsigned int blue = (unsigned char)(ZrdArrayInt(rgbBase, 3, 0));
    return red | (green << 8) | (blue << 16);
}

// Reimplements 0x4b98d0: HudUiBackground::LoadFromZrd
// (D:\Proj\Battlesport\hudui_background.cpp)
zReader::Node *RECOIL_THISCALL HudUiBackground::LoadFromZrd(const char *zrdPath,
                                                            const char *sectionName,
                                                            int capturePrimary) {
    zReader::Node *const root = zReader::LoadNodeFromPath(zrdPath, 0, 0);
    loadedRoot = root;
    return LoadZrdAndSection(root, sectionName, capturePrimary);
}

// Reimplements 0x4b9900: HudUiBackground::LoadZrdAndSection
// (D:\Proj\Battlesport\hudui_background.cpp)
zReader::Node *RECOIL_THISCALL HudUiBackground::LoadZrdAndSection(
    zReader::Node *loadedRootNode, const char *sectionName, int capturePrimary) {
    zReader::Node *result = 0;
    zVideo::RunPostprocessOnPrimaryBuffer();

    if (capturePrimary == 0) {
        primaryClipImage = zVideo_buff_CaptureSurfaceToImage(0);
    }

    if (loadedRootNode != 0) {
        result = loadedRootNode;

        zReader::Node *const sharedImagePath = zReader_GetNamedNode(loadedRootNode,
                                                                    "SHARED_IMAGE_PATH");
        zReader::Node *const sharedImagePathBase = ZrdArrayBase(sharedImagePath);
        if (sharedImagePathBase != 0) {
            zImage_InitMissionResources(ZrdArrayString(sharedImagePathBase, 1));
        }

        cfgRoot = zReader_GetNamedNode(loadedRootNode, sectionName);
    } else {
        cfgRoot = 0;
    }

    if (cfgRoot != 0) {
        zReader::Node *const imagePath = zReader_GetNamedNode(cfgRoot, "IMAGE_PATH");
        zReader::Node *const imagePathBase = ZrdArrayBase(imagePath);
        if (imagePathBase != 0) {
            zImage_InitMissionResources(ZrdArrayString(imagePathBase, 1));
        }

        zReader::Node *const fontList = ZrdArrayBase(zReader_GetNamedNode(cfgRoot, "FONTS"));
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

            const int styleIndex = ZrdArrayInt(fontSpec, 1, 0);
            if (styleIndex < 0 || styleIndex >= 20) {
                continue;
            }

            HudFontStyle &style = fontStyles[styleIndex];
            style.validMarker = 1;
            style.bkColor = 0;
            style.bkMode = 1;
            style.fontName = ZrdArrayString(fontSpec, 2);
            style.fontSize = ZrdArrayInt(fontSpec, 3, 0);

            zReader::Node *const colors = ZrdArrayBase(&fontSpec[4]);
            style.textColor = HudUiReadPackedColor(colors);
            if (colors != 0 && colors[1].type == zReader::ZRDR_NODE_ARRAY) {
                style.bkColor = HudUiReadPackedColor(ZrdArrayBase(&colors[2]));
                style.bkMode = 2;
            }

            if (ZrdArrayCount(fontSpec) >= 6) {
                style.fontWeight = ZrdArrayInt(fontSpec, 5, style.fontWeight);
            }
            if (ZrdArrayCount(fontSpec) >= 7) {
                style.shadowEnabled = ZrdArrayInt(fontSpec, 6, style.shadowEnabled);
            }
            if (ZrdArrayCount(fontSpec) >= 8) {
                const char *const align = ZrdArrayString(fontSpec, 7);
                if (align == 0 || strcmp(align, "LEFT") == 0) {
                    style.alignMode = 0;
                } else if (strcmp(align, "RIGHT") == 0) {
                    style.alignMode = 2;
                } else if (strcmp(align, "CENTER") == 0) {
                    style.alignMode = 1;
                }
            }
        }
        }

        zReader::Node *const imageList = ZrdArrayBase(zReader_GetNamedNode(cfgRoot,
                                                                           "BACKGROUND_IMAGES"));
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
            child.SetImageByPathOwned(ZrdArrayString(imageSpec, 1));
            if (ZrdArrayCount(imageSpec) >= 4) {
                ((HudUiElement *)(&child))->SetPos(
                    ZrdArrayInt(imageSpec, 2, 0) + uiOriginX,
                    ZrdArrayInt(imageSpec, 3, 0) + uiOriginY);
            }

            child.flags = (child.flags & 0x10u) | 0x02u;
            ((HudUiElement *)(&child))->SetVisible(1);
            ((HudUiElement *)(&child))->Invalidate();
            base.base.AddChild((HudUiElement *)(&child));
        }
        }

        zReader::Node *const videoList = ZrdArrayBase(zReader_GetNamedNode(cfgRoot,
                                                                           "BACKGROUND_VIDEOS"));
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
            child.SetMediaPathOwnedAndRefresh(ZrdArrayString(videoSpec, 1));
            if (ZrdArrayCount(videoSpec) >= 4) {
                child.base.SetPos(ZrdArrayInt(videoSpec, 2, 0) + uiOriginX,
                                  ZrdArrayInt(videoSpec, 3, 0) + uiOriginY);
            }
            if (ZrdArrayCount(videoSpec) >= 5) {
                zReader::Node *const color = ZrdArrayBase(&videoSpec[4]);
                child.SetColorKey565((unsigned short)(
                    zVid_PackColorRGB((unsigned char)(ZrdArrayInt(color, 1, 0)),
                                      (unsigned char)(ZrdArrayInt(color, 2, 0)),
                                      (unsigned char)(ZrdArrayInt(color, 3, 0)))));
            }

            child.base.SetVisible(1);
            child.base.Invalidate();
            child.base.SetBltSourceAndClipRect(primaryClipImage, 0);
            child.RebuildBltRect();
            base.base.AddChild(&child.base);
        }
        }

        zReader::Node *const textList = ZrdArrayBase(zReader_GetNamedNode(cfgRoot,
                                                                          "BACKGROUND_TEXT"));
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
            child->SetTextFmt("%s",
                              zLoc::ResolveMessageKeyOrFallback(ZrdArrayString(textSpec, 1)));
            HudUiVirtualSetPosRequired(child, ZrdArrayInt(textSpec, 2, 0) + uiOriginX,
                                       ZrdArrayInt(textSpec, 3, 0) + uiOriginY);
            ApplyHudFontStyleToPanel(child, HudUiZrdOwnerFontStyle(this,
                                                                   ZrdArrayInt(textSpec, 4, 0)));
            HudUiVirtualSetVisibleRequired(child, 1);
            base.base.AddChild((HudUiElement *)(child));
        }
        }

        if (capturePrimary == 0) {
            base.base.SetEnabled(1);
            base.base.UpdateAll(0.0f);
            capturedCompositeImage = zVideo_buff_CaptureSurfaceToImage(0);
            base.base.SetEnabled(0);
            ((HudUiDialogController *)(this))->BlitOwnedSurfaceToPrimary();
        }

        zReader::Node *const cursorNode = zReader_GetNamedNode(cfgRoot, "CURSOR");
        if (cursorNode != 0) {
            zReader::Node *const bitmapNode = ZrdArrayBase(zReader_GetNamedNode(cursorNode,
                                                                                "BITMAP"));
            if (bitmapNode != 0) {
                cursorWidget.SetImageByPathOwnedAndRefresh(ZrdArrayString(bitmapNode, 1));
            }

            base.SetInputFocus((HudUiElement *)(&cursorWidget));

            zReader::Node *const centerNode = ZrdArrayBase(zReader_GetNamedNode(cursorNode,
                                                                                "CENTER"));
            if (centerNode != 0) {
                // Original 0x4b9f69 stores CENTER's string pointer into HudUiWidget::alignFlags;
                // GetCenterX/Y only test the slot for nonzero on this cursor path.
                FieldAt<unsigned int>(&cursorWidget, offsetof(HudUiWidget, alignFlags)) =
                    (unsigned int)(ZrdArrayString(centerNode, 1));
            }

            int cursorCapture = 1;
            zReader::ReadNamedInt(cursorNode, "CAPTURE", &cursorCapture);
            cursorWidget.SetImageOwnedAndRefresh(cursorCapture);
        }

        zReader::Node *const soundList = ZrdArrayBase(zReader_GetNamedNode(cfgRoot,
                                                                           "BACKGROUND_SOUNDS"));
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
                volume = ZrdArrayFloat(soundSpec, 2, 1.0f);
            }
            entry.sample = zSnd::FindSampleByName(ZrdArrayString(soundSpec, 1));
            entry.volume = volume;
        }
    }

    }

    zVideo::Dispatch_UnlockPrimarySurfaceState();
    return result;
}

// Reimplements 0x4b9850: HudUiBackground::SetEnabled
void RECOIL_THISCALL HudUiBackground::SetEnabled(int enabled) {
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

        base.base.SetEnabled(enabled);
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

    base.base.InvalidateChildren();
    base.base.SetEnabled(enabled);
}

// Reimplements 0x4ba070: HudUiBackground::BindButtonsNodeToWidgetByName
RECOIL_NOINLINE unsigned char RECOIL_FASTCALL
HudUiBackground::BindButtonsNodeToWidgetByName(zReader::Node *parentNode, HudUiWidget *widget,
                                              const char *name) {
    if (parentNode != 0) {
        zReader::Node *const buttonsNode = zReader_GetNamedNode(parentNode, "BUTTONS");
        zReader::Node *const widgetNode = zReader_GetNamedNode(buttonsNode, name);
        if (widgetNode != 0) {
            typedef int (HudUiWidget::*LoadFromZrdFn)(zReader::Node *zrdSection,
                                                      void *ownerDialog);
            typedef void (HudUiWidget::*PostLoadFromZrdFn)();

            (widget->*((LoadFromZrdFn *)(&widget->ftable->slots[0x7c / 4]))[0])(
                widgetNode, this);
            (widget->*((PostLoadFromZrdFn *)(&widget->ftable->slots[0x80 / 4]))[0])();
        }
    }

    return 0;
}

// Reimplements 0x4ba0c0: HudUiBackground::BindWidgetByName
RECOIL_NOINLINE int RECOIL_THISCALL
HudUiBackground::BindWidgetByName(zReader::Node *, HudUiWidget *widget, const char *name) {
    return BindButtonsNodeToWidgetByName(cfgRoot, widget, name) & 0xff;
}

// Reimplements 0x4ba0e0: HudUiBackground::BindPrimitiveNodeToElement
RECOIL_NOINLINE int RECOIL_THISCALL
HudUiBackground::BindPrimitiveNodeToElement(zReader::Node *, HudUiElement *element,
                                            const char *name) {
    typedef void (RECOIL_THISCALL *SetPosFn)(HudUiElement * self, int x, int y);
    typedef void (RECOIL_THISCALL *EnableWordWrapFn)(HudUiElement * self, const HudUiRect *rect);
    typedef void (RECOIL_THISCALL *SetFontFn)(HudUiElement * self, const char *faceName,
                                             int height, int weight,
                                             int width, int italic,
                                             int charSet, int pitchAndFamily);
    typedef void (RECOIL_THISCALL *SetClipFn)(HudUiElement * self, void *bltSource, const HudUiRect *rect);
    typedef int (RECOIL_THISCALL *GetCoordFn)(HudUiElement * self);

    zReader::Node *const cfgRoot = FieldAt<zReader::Node *>(this, 0xa940);
    if (cfgRoot == 0) {
        return 0;
    }

    zReader::Node *const primitivesNode = zReader_GetNamedNode(cfgRoot, "PRIMITIVES");
    if (primitivesNode == 0) {
        return 0;
    }

    zReader::Node *const primitiveNode = zReader_GetNamedNode(primitivesNode, name);
    if (primitiveNode == 0) {
        return 0;
    }

    ((HudUiContainer *)(this))->AddChild(element);

    zReader::Node *bitmapNode = zReader_GetNamedNode(primitiveNode, "BITMAP");
    zReader::Node *bitmapBase = ZrdArrayBase(bitmapNode);
    const char *const bitmapPath = ZrdArrayString(bitmapBase, 1);
    if (bitmapPath != 0) {
        ((HudUiWidget *)(element))->SetImageByPathOwned(bitmapPath);
    }

    zReader::Node *positionBase = ZrdArrayBase(zReader_GetNamedNode(primitiveNode, "POSITION"));
    if (positionBase != 0) {
        ((SetPosFn)(element->ftable->slots[0x0c / 4]))(
            element, FieldAt<int>(this, 0xa944) + ZrdArrayInt(positionBase, 1, 0),
            FieldAt<int>(this, 0xa948) + ZrdArrayInt(positionBase, 2, 0));
    }

    zReader::Node *wordWrapBase = ZrdArrayBase(zReader_GetNamedNode(primitiveNode, "WORDWRAP"));
    if (wordWrapBase != 0) {
        HudUiRect wordWrapRect = {0};
        wordWrapRect.right = ZrdArrayInt(wordWrapBase, 1, 0);
        wordWrapRect.bottom = ZrdArrayInt(wordWrapBase, 2, 0);
        ((EnableWordWrapFn)(element->ftable->slots[0x6c / 4]))(element,
                                                                             &wordWrapRect);
    }

    zReader::Node *fontNode = zReader_GetNamedNode(primitiveNode, "FONT");
    zReader::Node *fontBase = ZrdArrayBase(fontNode);
    if (fontNode != 0) {
        const int fontIndex =
            fontBase != 0 ? ZrdArrayInt(fontBase, 1, 0) : fontNode->value.i32;
        const HudFontStyle *const style = HudUiZrdOwnerFontStyle(this, fontIndex);
        if (style != 0) {
            FieldAt<unsigned int>(element, 0x144) = style->alignMode;
            ((SetFontFn)(element->ftable->slots[0x80 / 4]))(
                element, style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);
            FieldAt<unsigned int>(element, 0x14c) = style->textColor;
            FieldAt<unsigned int>(element, 0x150) = style->textColor;
            FieldAt<unsigned int>(element, 0x270) = 1;
            FieldAt<unsigned int>(element, 0x264) = style->shadowEnabled;
            FieldAt<int>(element, 0x29c) = 1;
            FieldAt<int>(element, 0x2a0) = 1;
            FieldAt<unsigned int>(element, 0x26c) = style->bkColor;
            FieldAt<unsigned int>(element, 0x268) = style->bkMode;
        }
    }

    zReader::Node *colorBase = ZrdArrayBase(zReader_GetNamedNode(primitiveNode, "COLOR"));
    if (colorBase != 0) {
        const unsigned char red = (unsigned char)(ZrdArrayInt(colorBase, 1, 0));
        const unsigned char green = (unsigned char)(ZrdArrayInt(colorBase, 2, 0));
        const unsigned char blue = (unsigned char)(ZrdArrayInt(colorBase, 3, 0));
        FieldAt<unsigned int>(element, 0x3c) = zVid_PackColorRGB(red, green, blue) & 0xffffu;
    }

    zReader::Node *relativeEndBase = ZrdArrayBase(zReader_GetNamedNode(primitiveNode, "ENDP_REL"));
    if (relativeEndBase != 0) {
        const int startX =
            ((GetCoordFn)(element->ftable->slots[0x64 / 4]))(element);
        const int startY =
            ((GetCoordFn)(element->ftable->slots[0x68 / 4]))(element);
        ((HudUiPrimitiveBindTarget *)(element))->SetSegmentEndpoints(
            startX, startY, startX + ZrdArrayInt(relativeEndBase, 1, 0),
            startY + ZrdArrayInt(relativeEndBase, 2, 0));
    }

    zReader::Node *absoluteEndBase = ZrdArrayBase(zReader_GetNamedNode(primitiveNode, "ENDP_ABS"));
    if (absoluteEndBase != 0) {
        ((HudUiPrimitiveBindTarget *)(element))->SetSegmentEndpoints(
            ((GetCoordFn)(element->ftable->slots[0x64 / 4]))(element),
            ((GetCoordFn)(element->ftable->slots[0x68 / 4]))(element),
            ZrdArrayInt(absoluteEndBase, 1, 0), ZrdArrayInt(absoluteEndBase, 2, 0));
    }

    HudUiRect clipRect;
    clipRect.left = ((GetCoordFn)(element->ftable->slots[0x64 / 4]))(element);
    clipRect.top = ((GetCoordFn)(element->ftable->slots[0x68 / 4]))(element);
    clipRect.right = ((GetCoordFn)(element->ftable->slots[0x64 / 4]))(element);
    clipRect.bottom = ((GetCoordFn)(element->ftable->slots[0x68 / 4]))(element);
    ((SetClipFn)(element->ftable->slots[0x18 / 4]))(
        element, FieldAt<void *>(this, 0x118), &clipRect);

    element->flags = (element->flags & 0x10u) | 0x02u;
    return 0;
}

// Reimplements 0x4ba350: HudUiBackground::FreeLoadedTreeRoots (HudUiBackground.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiBackground::FreeLoadedTreeRoots(int) {
    zReader::Node *const root = loadedRoot;
    if (root != 0) {
        zReader::FreeLoadedTree(root);
    }

    loadedRoot = 0;
    cfgRoot = 0;
}

// Reimplements 0x4bc570: HudUiBackground::Update (D:\Proj\Battlesport\HudUi_Background.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiBackground::Update(float deltaSeconds) {
    typedef void (RECOIL_THISCALL *NoArgFn)(HudUiElement * self);
    typedef void (RECOIL_THISCALL *SetPosFn)(HudUiElement * self, int x, int y);
    typedef void (RECOIL_THISCALL *UpdateFn)(HudUiElement * self, float deltaSeconds);
    typedef void (RECOIL_THISCALL *PointerFn)(HudUiElement * self, int x,
                                             int y);
    typedef int (RECOIL_THISCALL *HitTestFn)(HudUiElement * self, int x,
                                                     int y);
    typedef int (RECOIL_THISCALL *ShouldHandleInputFn)(HudUiElement * self,
                                                               HudUiBackground * background,
                                                               int hovered);
    typedef void (RECOIL_THISCALL *AfterInputUpdateFn)(HudUiElement * self,
                                                      HudUiBackground * background,
                                                      int hovered);

    if (base.base.enabled == 0) {
        return;
    }

#if defined(_MSC_VER) && _MSC_VER < 1200
    zInput::MouseStateSnapshot &mouseState =
        *&FieldAt<zInput::MouseStateSnapshot>(this, 0x14);
#else
    zInput::MouseStateSnapshot &mouseState = FieldAt<zInput::MouseStateSnapshot>(this, 0x14);
#endif
    memcpy(&mouseState, zInput::Mouse_GetStateSnapshotPtr(), sizeof(mouseState));

    for (HudUiElement *widget = base.base.childHead; widget != 0; widget = widget->next) {
        const int hit =
            ((HitTestFn)(widget->ftable->slots[0x5c / 4]))(
                widget, mouseState.cursorClientX, mouseState.cursorClientY);
        const int hovered = hit == 1 ? 1 : 0;

        if (((ShouldHandleInputFn)(widget->ftable->slots[0x54 / 4]))(
                widget, this, hovered) != 0) {
            if ((mouseState.button2Transition & 4) != 0 && (widget->state & 2) == 2) {
                widget->state = (unsigned short)(widget->state & 0xfffd);
                ((NoArgFn)(widget->ftable->slots[0x48 / 4]))(widget);
            }

            if (hovered != 0) {
                if ((widget->state & 1) == 0) {
                    widget->state = (unsigned short)(widget->state | 1);
                    ((NoArgFn)(widget->ftable->slots[0x3c / 4]))(widget);
                } else {
                    ((NoArgFn)(widget->ftable->slots[0x38 / 4]))(widget);
                }

                if ((mouseState.button1Transition & base.captureTransitionMask) != 0 &&
                    (widget->state & 2) == 0) {
                    widget->state = (unsigned short)(widget->state | 2);
                    ((NoArgFn)(widget->ftable->slots[0x44 / 4]))(widget);
                }

                if ((mouseState.button1Transition & 4) != 0) {
                    ((NoArgFn)(widget->ftable->slots[0x30 / 4]))(widget);
                }

                if ((mouseState.button2Transition & 4) != 0) {
                    ((NoArgFn)(widget->ftable->slots[0x34 / 4]))(widget);
                }

                if ((mouseState.button1Transition & 3) != 0) {
                    ((PointerFn)(widget->ftable->slots[0x4c / 4]))(
                        widget, mouseState.cursorClientX, mouseState.cursorClientY);
                }

                if ((mouseState.button1Transition & 4) != 0 && (widget->state & 2) == 2) {
                    ((NoArgFn)(widget->ftable->slots[0x50 / 4]))(widget);
                }
            } else {
                if ((mouseState.button1Transition & base.captureTransitionMask) != 0 &&
                    (widget->state & 2) == 2) {
                    widget->state = (unsigned short)(widget->state & 0xfffd);
                    ((NoArgFn)(widget->ftable->slots[0x48 / 4]))(widget);
                }

                if ((mouseState.button1Transition & 3) != 0 && (widget->state & 2) == 2) {
                    ((PointerFn)(widget->ftable->slots[0x4c / 4]))(
                        widget, mouseState.cursorClientX, mouseState.cursorClientY);
                }

                if ((widget->state & 1) == 1) {
                    widget->state = (unsigned short)(widget->state & 0xfffe);
                    ((NoArgFn)(widget->ftable->slots[0x40 / 4]))(widget);
                }
            }
        }

        ((AfterInputUpdateFn)(widget->ftable->slots[0x58 / 4]))(
            widget, this, hovered);
    }

    HudUiElement *const focusBeforeUpdate = base.inputFocusElement;
    if (focusBeforeUpdate != 0) {
        ((NoArgFn)(focusBeforeUpdate->ftable->slots[0x08 / 4]))(focusBeforeUpdate);
    }

    base.base.UpdateAll(deltaSeconds);

    HudUiElement *const focusAfterUpdate = base.inputFocusElement;
    if (focusAfterUpdate != 0) {
        ((SetPosFn)(focusAfterUpdate->ftable->slots[0x0c / 4]))(
            focusAfterUpdate, mouseState.cursorClientX, mouseState.cursorClientY);
        ((UpdateFn)(focusAfterUpdate->ftable->slots[0x24 / 4]))(focusAfterUpdate,
                                                                               deltaSeconds);
    }
}

// Reimplements 0x4ba4a0: HudFontStyle::Constructor
HudFontStyle *RECOIL_THISCALL HudFontStyle::Constructor() {
    validMarker = 0;
    fontName = 0;
    fontSize = 0;
    textColor = 0;
    shadowEnabled = 0;
    alignMode = 0;
    fontWeight = 0x1f4;
    return this;
}

// Reimplements 0x4ba4c0: HudFontStyle::Destructor
void RECOIL_THISCALL HudFontStyle::Destructor() {
    validMarker = 0;
}

// Reimplements 0x4b3d00: HudUiWidget::Constructor
HudUiWidget *RECOIL_THISCALL HudUiWidget::Constructor(unsigned int initAlignFlags) {
    ((HudUiElement *)(this))->Constructor(0, 0);
    ftable = &g_HudUiWidget_FTable;
    alignFlags = initAlignFlags;
    image = 0;
    ownsImage = 0;
    bltClipRectOrNull = 0;
    *(unsigned short *)((unsigned char *)(this) + 0x40) = 0;
    dirtyRectCount = 0;

    {
        int dirtyRectIndex;
        for (dirtyRectIndex = 0; dirtyRectIndex < 4; ++dirtyRectIndex) {
            dirtyRects[dirtyRectIndex].framesRemaining = 0;
        }
    }

    return this;
}

// Reimplements 0x4b3e90: HudUiWidget::InvalidateRect
// (D:\Proj\Battlesport\hudui.cpp)
void RECOIL_THISCALL HudUiWidget::InvalidateRect(const HudUiRect *dirtyRect) {
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

    typedef int (RECOIL_THISCALL *GetCoordFn)(HudUiWidget * self);
    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiWidget * self);
    slot->srcLeft -= ((GetCoordFn)(ftable->slots[25]))(this);
    slot->srcRight -= ((GetCoordFn)(ftable->slots[25]))(this);
    slot->srcTop -= ((GetCoordFn)(ftable->slots[26]))(this);
    slot->srcBottom -= ((GetCoordFn)(ftable->slots[26]))(this);
    ((InvalidateFn)(ftable->slots[8]))(this);
}

// Reimplements 0x4bf980: HudUiBackgroundCursorWidget::MemberConstructorLocal
// (D:\Proj\Battlesport\hudui_background.cpp)
HudUiBackgroundCursorWidget *RECOIL_THISCALL
HudUiBackgroundCursorWidget::MemberConstructorLocal(const char *imagePath,
                                                    int initCaptureEnabled) {
    base.Constructor(0);
    captureEnabled = initCaptureEnabled;
    base.ftable = (const HudUiWidget_FTable *)(&g_HudUiBackgroundCursorWidget_FTable);
    capturedImage = 0;
    if (imagePath != 0) {
        SetImageByPathOwnedAndRefresh(imagePath);
    }

    reservedC8 = 0;
    reservedCC = 0;
    captureSourceSelector = 1;
    return this;
}

// Reimplements 0x4bfa20: HudUiBackgroundCursorWidget::DestructorCore
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundCursorWidget::DestructorCore() {
    base.ftable = (const HudUiWidget_FTable *)(&g_HudUiBackgroundCursorWidget_FTable);
    if (capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
    }

    base.DestructorCore();
}

// Reimplements 0x4bfa50: HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL
HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh(const char *imagePath) {
    if (base.SetImageByPathOwned(imagePath) != 0) {
        SetImageBorrowedAndRefresh();
    }
}

// Reimplements 0x4bfa70: HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL
HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged(zVidImagePartial *image) {
    if (base.SetImageBorrowedAndInvalidate(image) != 0) {
        SetImageBorrowedAndRefresh();
    }
}

// Reimplements 0x4bfa90: HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL
HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh(int newCaptureEnabled) {
    typedef void (RECOIL_THISCALL *SetBltSourceFn)(HudUiBackgroundCursorWidget * self,
                                                   zVidImagePartial *image,
                                                   const HudUiRect *clipRect);

    captureEnabled = newCaptureEnabled;
    if (newCaptureEnabled == 0 && capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
        capturedImage = 0;
        ((SetBltSourceFn)(base.ftable->slots[6]))(this, 0, 0);
        return;
    }

    if (capturedImage == 0) {
        SetImageBorrowedAndRefresh();
    }
}

// Reimplements 0x4bfae0: HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh() {
    if (captureEnabled == 0 || base.image == 0) {
        return;
    }

    if (capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
    }

    capturedImage = zVid_Image::Create();
    if (capturedImage == 0) {
        return;
    }

    zVid_Image::SetSize(capturedImage, base.image->width, base.image->height);
    void *const pixels =
        malloc((size_t)(capturedImage->pixelCount) * sizeof(unsigned short));
    zVid_Image_SetPixels(capturedImage, pixels, 0);
    capturedImage->formatFlagsPacked =
        (unsigned char)(capturedImage->formatFlagsPacked | 0x20u);

    typedef int (RECOIL_THISCALL *GetCoordFn)(HudUiBackgroundCursorWidget * self);
    const int y = ((GetCoordFn)(base.ftable->slots[26]))(this);
    const int x = ((GetCoordFn)(base.ftable->slots[25]))(this);
    RebuildCapturedImage(x, y);
}

// Reimplements 0x4bfb70: HudUiBackgroundCursorWidget::SetPos
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundCursorWidget::SetPos(int newX, int newY) {
    ((HudUiElement *)(&base))->SetPos(newX, newY);
    RebuildCapturedImage(base.x, base.y);
}

// Reimplements 0x4bfba0: HudUiBackgroundCursorWidget::RebuildCapturedImage
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundCursorWidget::RebuildCapturedImage(int originX,
                                                                       int originY) {
    typedef void (RECOIL_THISCALL *SetBltSourceFn)(HudUiBackgroundCursorWidget * self,
                                                   zVidImagePartial *image,
                                                   const HudUiRect *clipRect);

    if (capturedImage == 0) {
        return;
    }

    zVidRect32 sourceRect;
    sourceRect.left = originX;
    sourceRect.top = originY;
    sourceRect.right = originX + base.image->width;
    sourceRect.bottom = originY + base.image->height;

    if (zVideo_buff::CopySurfaceRectToImage(captureSourceSelector, &sourceRect, capturedImage) !=
        0) {
        const HudUiRect clipRect = {sourceRect.left - originX, sourceRect.top - originY,
                                 sourceRect.right - originX, sourceRect.bottom - originY};
        ((SetBltSourceFn)(base.ftable->slots[6]))(this, capturedImage, &clipRect);
        return;
    }

    ((SetBltSourceFn)(base.ftable->slots[6]))(this, 0, 0);
}

// Reimplements 0x4bfc50: HudUiBackgroundCursorWidget::Draw
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundCursorWidget::Draw() {
    base.Draw();
}

// Reimplements 0x4bfc60: HudUiBackgroundCursorWidget::DrawBase
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundCursorWidget::DrawBase() {
    if (base.bltSource != 0) {
        zVid_Image::BlitToActiveTarget((zVidImagePartial *)(base.bltSource), base.x,
                                       base.y, 0,
                                       (zVidRect32 *)(&base.clipRect));
    }
}

// Reimplements 0x4bfc80: HudUiBackgroundVideoWidget::Constructor
// (D:\Proj\Battlesport\hudui_background.cpp)
HudUiBackgroundVideoWidget *RECOIL_THISCALL HudUiBackgroundVideoWidget::Constructor() {
    base.Constructor(0, 0);
    base.ftable = (const HudUiCommon_FTable *)(&g_HudUiBackgroundVideoWidget_FTable);
    mediaPath[0] = '\0';
    stream = 0;
    elapsedTimeSec = 0.0f;
    return this;
}

// Reimplements 0x4bfcd0: HudUiBackgroundVideoWidget::Destructor
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundVideoWidget::Destructor() {
    base.ftable = (const HudUiCommon_FTable *)(&g_HudUiBackgroundVideoWidget_FTable);
    zFMV_Stream *const oldStream = stream;
    if (oldStream != 0) {
        oldStream->Destructor();
        ::operator delete(oldStream);
        stream = 0;
    }

    base.ftable = &g_HudUiCommon_FTable;
}

// Reimplements 0x4bfd40: HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL
HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh(const char *path) {
    strncpy(mediaPath, path, 0x104);

    struct _stat statBuffer;
    if (_stat(mediaPath, &statBuffer) == -1) {
        char *const resolvedPath = zSys::FindFileOnDriveType(5, mediaPath, 0);
        if (resolvedPath != 0) {
            strncpy(mediaPath, resolvedPath, 0x104);
        }
    }

    if (_stat(mediaPath, &statBuffer) == -1) {
        stream = 0;
        return;
    }

    zFMV_Stream *const newStream = (zFMV_Stream *)(::operator new(0x1e4));
    stream = newStream != 0 ? newStream->Init(mediaPath, 0) : 0;

    typedef void (RECOIL_THISCALL *RebuildBltRectFn)(HudUiBackgroundVideoWidget * self);
    const HudUiBackgroundVideoWidget_FTable *const ftable = (const HudUiBackgroundVideoWidget_FTable *)(base.ftable);
    ((RebuildBltRectFn)(ftable->slots[0x74 / 4]))(this);
}

// Reimplements 0x4bfe20: HudUiBackgroundVideoWidget::SetColorKey565
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundVideoWidget::SetColorKey565(unsigned short colorKey) {
    if (stream != 0) {
        FieldAt<unsigned char>(stream, 0x09) |= 0x02;
    }

    colorKey565 = colorKey;
}

// Reimplements 0x4bfe40: HudUiBackgroundVideoWidget::Update
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundVideoWidget::Update(float deltaSeconds) {
    if ((base.flags & 0x10u) != 0) {
        return;
    }

    if (stream != 0) {
        const int frameTick =
            (int)((float)(FieldAt<int>(stream, 0xec)) *
                                      elapsedTimeSec);
        stream->ReadAndDecodeFrame((unsigned int)(
            frameTick % FieldAt<int>(stream, 0x4c)));
    }

    base.Update(deltaSeconds);
    elapsedTimeSec += deltaSeconds;
}

// Reimplements 0x4bfe90: HudUiBackgroundVideoWidget::Draw
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundVideoWidget::Draw() {
    typedef void (RECOIL_THISCALL *DrawBaseFn)(HudUiBackgroundVideoWidget * self);
    const HudUiBackgroundVideoWidget_FTable *const ftable = (const HudUiBackgroundVideoWidget_FTable *)(base.ftable);
    ((DrawBaseFn)(ftable->slots[2]))(this);

    if (stream != 0) {
        zVid_Image::BlitToActiveTarget((zVidImagePartial *)(stream), base.x, base.y,
                                       colorKey565, 0);
    }
}

// Reimplements 0x4bfec0: HudUiBackgroundVideoWidget::DrawBase
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundVideoWidget::DrawBase() {
    zVidImagePartial *const bltSource = (zVidImagePartial *)(base.bltSource);
    if (bltSource != 0) {
        const int dstX = base.x > 0 ? base.x : 0;
        const int dstY = base.y > 0 ? base.y : 0;
        zVid_Image::BlitToActiveTarget(bltSource, dstX, dstY, 0,
                                       (zVidRect32 *)(&base.clipRect));
    }
}

// Reimplements 0x4bff00: HudUiBackgroundVideoWidget::RebuildBltRect
// (D:\Proj\Battlesport\hudui_background.cpp)
void RECOIL_THISCALL HudUiBackgroundVideoWidget::RebuildBltRect() {
    HudUiRect rect;
    rect.left = HudUiVirtualGetXRequired(&base) > 0 ? HudUiVirtualGetXRequired(&base) : 0;
    rect.top = HudUiVirtualGetYRequired(&base) > 0 ? HudUiVirtualGetYRequired(&base) : 0;

    if (stream == 0) {
        return;
    }

    const int streamRight = rect.left + FieldAt<short>(stream, 0x04);
    const int streamBottom = rect.top + FieldAt<short>(stream, 0x06);

    zVidImagePartial *const bltSource = (zVidImagePartial *)(base.bltSource);
    if (bltSource != 0) {
        rect.right = streamRight < bltSource->width ? streamRight : bltSource->width;
        rect.bottom = streamBottom < bltSource->height ? streamBottom : bltSource->height;
    } else {
        rect.right = streamRight;
        rect.bottom = streamBottom;
    }

    typedef void (RECOIL_THISCALL *SetClipRectFn)(HudUiBackgroundVideoWidget * self,
                                                  const HudUiRect *rect);
    const HudUiBackgroundVideoWidget_FTable *const ftable = (const HudUiBackgroundVideoWidget_FTable *)(base.ftable);
    ((SetClipRectFn)(ftable->slots[7]))(this, &rect);
}

// Reimplements 0x4b4ee0: HudUiZrdWidget::Constructor
HudUiZrdWidget *RECOIL_THISCALL HudUiZrdWidget::Constructor() {
    base.Constructor(0);
    // VC5 copies an uninitialized STL allocator proxy byte into each vector.
#if defined(_MSC_VER) && _MSC_VER < 1200
    char allocatorProxy;
#else
    char allocatorProxy = 0;
#endif
    labelPanels.allocatorProxy = allocatorProxy;
    labelPanels.begin = 0;
    labelPanels.end = 0;
    labelPanels.capacityEnd = 0;
    rolloverLabelPanels.allocatorProxy = allocatorProxy;
    rolloverLabelPanels.begin = 0;
    rolloverLabelPanels.end = 0;
    rolloverLabelPanels.capacityEnd = 0;
    activateLabelPanels.begin = 0;
    activateLabelPanels.allocatorProxy = allocatorProxy;
    activateLabelPanels.end = 0;
    activateLabelPanels.capacityEnd = 0;
    disabledLabelPanels.begin = 0;
    disabledLabelPanels.allocatorProxy = allocatorProxy;
    disabledLabelPanels.end = 0;
    disabledLabelPanels.capacityEnd = 0;

    base.ftable = (const HudUiWidget_FTable *)(&g_HudUiZrdWidget_FTable);
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
    ((StdPtrVector *)(&labelPanels))->ClearNoOpDestroy((int *)(labelDest),
                                                       (int *)(labelPanels.end));
    labelPanels.end = labelDest;

    HudUiPanel **rolloverSource = rolloverLabelPanels.end;
    HudUiPanel **rolloverDest = rolloverLabelPanels.begin;
    if (rolloverSource != rolloverLabelPanels.end) {
        do {
            *rolloverDest++ = *rolloverSource++;
        } while (rolloverSource != rolloverLabelPanels.end);
    }
    ((StdPtrVector *)(&rolloverLabelPanels))
        ->ClearNoOpDestroy((int *)(rolloverDest), (int *)(rolloverLabelPanels.end));
    rolloverLabelPanels.end = rolloverDest;

    HudUiPanel **activateSource = activateLabelPanels.end;
    HudUiPanel **activateDest = activateLabelPanels.begin;
    if (activateSource != activateLabelPanels.end) {
        do {
            *activateDest++ = *activateSource++;
        } while (activateSource != activateLabelPanels.end);
    }
    ((StdPtrVector *)(&activateLabelPanels))
        ->ClearNoOpDestroy((int *)(activateDest), (int *)(activateLabelPanels.end));
    activateLabelPanels.end = activateDest;

    *((unsigned short *)(&base.imageStateWord)) = 1;
    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiZrdWidget * self);
    ((InvalidateFn)(base.ftable->slots[8]))(this);
    base.flags = ((unsigned char)(base.flags) & 0x10u) | 0x02u;
    return this;
}

// Reimplements 0x4b59f0: HudUiZrdWidget::LoadFromZrd
int RECOIL_THISCALL HudUiZrdWidget::LoadFromZrd(zReader::Node *zrdSection,
                                                         void *ownerDialog) {
    owner = ownerDialog;
    ((HudUiElement *)(&base))->SetVisible(1);
    ((HudUiContainer *)(ownerDialog))->AddChild((HudUiElement *)(this));

    originX = OwnerField<int>(ownerDialog, 0xa944);
    originY = OwnerField<int>(ownerDialog, 0xa948);
    if (zrdSection == 0) {
        return 0;
    }

    zReader::Node *const positionNode = zReader_GetNamedNode(zrdSection, "POSITION");
    zReader::Node *const positionBase = ZrdArrayBase(positionNode);
    if (positionBase != 0) {
        originX += ZrdArrayInt(positionBase, 1, 0);
        originY += ZrdArrayInt(positionBase, 2, 0);
    }

    int widgetX = originX;
    int widgetY = originY;
    zReader::Node *const bitmapNode = zReader_GetNamedNode(zrdSection, "BITMAP");
    zReader::Node *const bitmapBase = ZrdArrayBase(bitmapNode);
    const char *const bitmapPath = ZrdArrayString(bitmapBase, 1);
    if (bitmapPath != 0) {
        defaultImage = base.SetImageByPathOwned(bitmapPath);
        if (ZrdArrayCount(bitmapBase) >= 4) {
            widgetX += ZrdArrayInt(bitmapBase, 2, 0);
            widgetY += ZrdArrayInt(bitmapBase, 3, 0);
        }
    }

    ((HudUiElement *)(&base))->SetPos(widgetX, widgetY);

    void *const clipSource = OwnerField<void *>(ownerDialog, 0x118);
    if (clipSource != 0) {
        HudUiRect *const bounds = GetBoundsRectOrNull();
        if (bounds != 0) {
            ((HudUiElement *)(&base))->SetBltSourceAndClipRect(clipSource, bounds);
        }
    }

    zReader::Node *const rolloverNode = zReader_GetNamedNode(zrdSection, "ROLLOVER");
    if (rolloverNode != 0) {
        LoadHudZrdBitmap(rolloverNode, "BITMAP", &rolloverImage);
        LoadHudZrdSound(rolloverNode, &rolloverSound, &rolloverSoundScale);
        LoadHudZrdLabelSection(this, rolloverNode, rolloverLabelPanels);
        ApplyHudZrdFlashSection(rolloverNode, rolloverLabelPanels);
    }

    zReader::Node *const disableNode = zReader_GetNamedNode(zrdSection, "DISABLE");
    if (disableNode != 0) {
        LoadHudZrdBitmap(disableNode, "BITMAP", &disabledImage);
        LoadHudZrdSound(disableNode, &disabledSound, &disabledSoundScale);
        LoadHudZrdLabelSection(this, disableNode, disabledLabelPanels);
    }

    zReader::Node *const activateNode = zReader_GetNamedNode(zrdSection, "ACTIVATE");
    if (activateNode != 0) {
        LoadHudZrdBitmap(activateNode, "BITMAP", &activateImage);
        LoadHudZrdSound(activateNode, &activateSound, &activateSoundScale);
        LoadHudZrdLabelSection(this, activateNode, activateLabelPanels);
        ApplyHudZrdFlashSection(activateNode, activateLabelPanels);
    }

    LoadHudZrdLabelSection(this, zrdSection, labelPanels);
    ApplyHudZrdFlashSection(zrdSection, labelPanels);
    return 1;
}

// Reimplements 0x4ba4d0: HudUiPanelPtrVector::EraseRange
HudUiPanel **RECOIL_THISCALL HudUiPanelPtrVector::EraseRange(HudUiPanel **first,
                                                             HudUiPanel **last) {
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

// Reimplements 0x4ba510: HudUiPanelPtrVector::InsertN
void RECOIL_THISCALL HudUiPanelPtrVector::InsertN(HudUiPanel **position, unsigned int count,
                                                  HudUiPanel **valueSource) {
    if (count == 0) {
        return;
    }

    const size_t size = begin != 0 ? (size_t)(end - begin) : 0;
    const size_t positionIndex =
        begin != 0 && position != 0 ? (size_t)(position - begin) : 0;
    const size_t capacity =
        begin != 0 ? (size_t)(capacityEnd - begin) : 0;

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

struct HudUiScalarDeletingDestructorDispatch {
    // Models slot-0 scalar-deleting destructor dispatch so VC emits ecx=this.
    virtual void *ScalarDeletingDestructor(unsigned int flags);
};

// Reimplements 0x4b52f0: HudUiZrdWidget::DeleteChildIfPresent
void *RECOIL_STDCALL HudUiZrdWidget::DeleteChildIfPresent(void *childWidgetOrNull) {
    if (childWidgetOrNull != 0) {
        ((HudUiScalarDeletingDestructorDispatch *)(childWidgetOrNull))
            ->ScalarDeletingDestructor(1);
    }

    return 0;
}

void HudUiZrdWidget_DeletePanelVectorChildren(HudUiPanelPtrVector &vector) {
    for (HudUiPanel **it = vector.begin; it != vector.end; ++it) {
        if (*it != 0) {
            const HudUiWidget_FTable *const ftable =
                *(const HudUiWidget_FTable *const *)(*it);
            typedef void *(RECOIL_THISCALL *ScalarDeletingDtorFn)(void *self,
                                                                  unsigned int flags);
            ((ScalarDeletingDtorFn)(ftable->slots[0]))(*it, 1);
        }

        *it = 0;
    }
}

// Reimplements 0x4b50c0: HudUiZrdWidget::DestructorCore
void RECOIL_THISCALL HudUiZrdWidget::DestructorCore() {
    base.ftable = (const HudUiWidget_FTable *)(&g_HudUiZrdWidget_FTable);

    for (HudUiPanel **it = labelPanels.begin; it != labelPanels.end; ++it) {
        *it = (HudUiPanel *)(DeleteChildIfPresent(*it));
    }

    HudUiZrdWidget_DeletePanelVectorChildren(rolloverLabelPanels);
    HudUiZrdWidget_DeletePanelVectorChildren(activateLabelPanels);

    labelPanels.EraseRange(labelPanels.begin, labelPanels.end);
    rolloverLabelPanels.EraseRange(rolloverLabelPanels.begin, rolloverLabelPanels.end);
    activateLabelPanels.EraseRange(activateLabelPanels.begin, activateLabelPanels.end);

    if (defaultImage != 0 && defaultImage != base.image) {
        zVid_Image::ReleaseIfNotDefault(defaultImage);
        defaultImage = 0;
    }

    if (activateImage != 0 && activateImage != base.image) {
        zVid_Image::ReleaseIfNotDefault(activateImage);
        activateImage = 0;
    }

    if (rolloverImage != 0 && rolloverImage != base.image) {
        zVid_Image::ReleaseIfNotDefault(rolloverImage);
        rolloverImage = 0;
    }

    if (disabledImage != 0 && disabledImage != base.image) {
        zVid_Image::ReleaseIfNotDefault(disabledImage);
        disabledImage = 0;
    }

    if (base.image != 0 && base.ownsImage == 0) {
        zVid_Image::ReleaseIfNotDefault(base.image);
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

    base.DestructorCore();
}

// Reimplements 0x4b50a0: HudUiZrdWidget::ScalarDeletingDestructor
HudUiZrdWidget *RECOIL_THISCALL HudUiZrdWidget::ScalarDeletingDestructor(unsigned int flags) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b5310: HudUiZrdWidget::Invalidate
void RECOIL_THISCALL HudUiZrdWidget::Invalidate() {
    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiPanel * self);

    ((HudUiElement *)(&base))->Invalidate();

    HudUiPanel **panel = labelPanels.begin;
    if (panel == 0) {
        return;
    }

    while (panel != labelPanels.end) {
        HudUiPanel *const label = *panel;
        const HudUiPanel_FTable *const panelFTable = (const HudUiPanel_FTable *)(label->vtbl);
        ((InvalidateFn)(panelFTable->slots[8]))(label);
        ++panel;
    }
}

// Reimplements 0x4b5350: HudUiZrdWidget::GetBoundsRectOrNull
HudUiRect *RECOIL_THISCALL HudUiZrdWidget::GetBoundsRectOrNull() {
    if (modeOrEnabled == 0) {
        return 0;
    }

    if (base.image != 0) {
        boundsRect.left = base.x;
        boundsRect.top = base.y;
        boundsRect.right = base.x + base.image->width;
        boundsRect.bottom = base.y + base.image->height;
        return &boundsRect;
    }

    HudUiPanel **panelIt = labelPanels.begin;
    if (panelIt == 0) {
        return 0;
    }

    HudUiPanel *const firstPanel = *panelIt;
    HudUiElement *const firstElement = (HudUiElement *)(firstPanel);
    boundsRect.top = HudUiVirtualGetYRequired(firstElement);
    boundsRect.bottom = boundsRect.top + firstPanel->QueryTextHeight();

    while (panelIt != labelPanels.end) {
        HudUiPanel *const panel = *panelIt;
        HudUiElement *const panelElement = (HudUiElement *)(panel);
        boundsRect.bottom += panel->QueryTextHeight();

        const int alignMode = FieldAt<int>(panel, 0x144);
        const int panelX = HudUiVirtualGetXRequired(panelElement);
        const int width = HudUiPanelTextWidth(panel);
        if (alignMode == 0) {
            boundsRect.left = HudUiVirtualGetXRequired(firstElement);
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
            boundsRect.right = HudUiVirtualGetXRequired(firstElement);
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
void RECOIL_THISCALL HudUiZrdWidget::RefreshState() {
    typedef void (RECOIL_THISCALL *SetVisibleFn)(HudUiPanel * self, int visible);
    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiZrdWidget * self);

    for (HudUiPanel **rolloverIt = rolloverLabelPanels.begin; rolloverIt != rolloverLabelPanels.end;
         ++rolloverIt) {
        HudUiPanel *const panel = *rolloverIt;
        const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
        ((SetVisibleFn)(ftable->slots[24]))(panel, 0);
    }

    for (HudUiPanel **activateIt = activateLabelPanels.begin; activateIt != activateLabelPanels.end;
         ++activateIt) {
        HudUiPanel *const panel = *activateIt;
        const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
        ((SetVisibleFn)(ftable->slots[24]))(panel, 0);
    }

    if (modeOrEnabled != 0) {
        for (HudUiPanel **labelIt = labelPanels.begin; labelIt != labelPanels.end; ++labelIt) {
            HudUiPanel *const panel = *labelIt;
            const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
            ((SetVisibleFn)(ftable->slots[24]))(panel, 1);
        }

        for (HudUiPanel **disabledIt = disabledLabelPanels.begin;
             disabledIt != disabledLabelPanels.end; ++disabledIt) {
            HudUiPanel *const panel = *disabledIt;
            const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
            ((SetVisibleFn)(ftable->slots[24]))(panel, 0);
        }

        base.SetImageBorrowedAndInvalidate(defaultImage);
        ((InvalidateFn)(base.ftable->slots[8]))(this);
        return;
    }

    for (HudUiPanel **labelIt2 = labelPanels.begin; labelIt2 != labelPanels.end; ++labelIt2) {
        HudUiPanel *const panel = *labelIt2;
        const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
        ((SetVisibleFn)(ftable->slots[24]))(panel, 0);
    }

    for (HudUiPanel **disabledIt2 = disabledLabelPanels.begin;
         disabledIt2 != disabledLabelPanels.end; ++disabledIt2) {
        HudUiPanel *const panel = *disabledIt2;
        const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
        ((SetVisibleFn)(ftable->slots[24]))(panel, 1);
    }

    base.SetImageBorrowedAndInvalidate(disabledImage);
    ((InvalidateFn)(base.ftable->slots[8]))(this);
}

// Reimplements 0x4b5630: HudUiZrdWidget::ShowPreview
void RECOIL_THISCALL HudUiZrdWidget::ShowPreview() {
    if (rolloverImage != 0) {
        if (defaultImage == 0) {
            defaultImage = base.image;
        }

        base.SetImageBorrowedAndInvalidate(rolloverImage);
    }

    if (rolloverSound != 0) {
        rolloverPlayHandle = rolloverSound->PlayA3DSimple(rolloverSoundScale);
    }

    if (rolloverLabelPanels.begin != 0) {
        HudUiSetPanelVectorVisible(labelPanels, 0);
        HudUiSetPanelVectorVisible(activateLabelPanels, 0);
        HudUiSetPanelVectorVisible(rolloverLabelPanels, 1);
        return;
    }

    HudUiSetPanelVectorVisible(labelPanels, 1);
    HudUiSetPanelVectorVisible(activateLabelPanels, 0);
}

// Reimplements 0x4b5900: HudUiZrdWidget::OnActivate
void RECOIL_THISCALL HudUiZrdWidget::OnActivate() {
    zInput::ResetAllTransitionState();

    if (activateImage != 0) {
        base.SetImageBorrowedAndInvalidate(activateImage);
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle->StopIfActive();
        rolloverPlayHandle = 0;
    }

    if (activateSound != 0) {
        activatePlayHandle = activateSound->PlayA3DSimple(activateSoundScale);
    }

    HudUiSetPanelVectorVisible(rolloverLabelPanels, 0);

    if (activateLabelPanels.begin != 0) {
        HudUiSetPanelVectorVisible(activateLabelPanels, 1);
        HudUiSetPanelVectorVisible(labelPanels, 0);
        return;
    }

    HudUiSetPanelVectorVisible(labelPanels, 1);
}

// Reimplements 0x4b5860: HudUiZrdWidget::HidePreview
void RECOIL_THISCALL HudUiZrdWidget::HidePreview() {
    typedef void (RECOIL_THISCALL *SetVisibleFn)(HudUiPanel * self, int visible);

    if (defaultImage != 0) {
        base.SetImageBorrowedAndInvalidate(defaultImage);
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle = 0;
    }

    for (HudUiPanel **rolloverIt = rolloverLabelPanels.begin; rolloverIt != rolloverLabelPanels.end;
         ++rolloverIt) {
        HudUiPanel *const panel = *rolloverIt;
        const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
        ((SetVisibleFn)(ftable->slots[24]))(panel, 0);
    }

    for (HudUiPanel **activateIt = activateLabelPanels.begin; activateIt != activateLabelPanels.end;
         ++activateIt) {
        HudUiPanel *const panel = *activateIt;
        const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
        ((SetVisibleFn)(ftable->slots[24]))(panel, 0);
    }

    for (HudUiPanel **labelIt = labelPanels.begin; labelIt != labelPanels.end; ++labelIt) {
        HudUiPanel *const panel = *labelIt;
        const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
        ((SetVisibleFn)(ftable->slots[24]))(panel, 1);
    }
}

// Reimplements 0x4b6fc0: HudUiCheckToggleWidget::Constructor
RECOIL_NOINLINE HudUiCheckToggleWidget *RECOIL_THISCALL HudUiCheckToggleWidget::Constructor() {
    base.Constructor();
    base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiCheckToggleWidget_FTable);
    checked = 0;
    uncheckedImage = 0;
    checkedImage = 0;
    checkedLabelPanel = 0;
    disabledCheckedImage = 0;
    disabledCheckedFallbackImage = 0;
    return this;
}

// Reimplements 0x4b7020: HudUiCheckToggleWidget::DestructorCore
RECOIL_NOINLINE void RECOIL_THISCALL HudUiCheckToggleWidget::DestructorCore() {
    base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiCheckToggleWidget_FTable);
    base.base.SetImageBorrowedAndInvalidate(uncheckedImage);

    if (checkedImage != 0) {
        ::operator delete(checkedImage);
        checkedImage = 0;
    }

    if (checkedLabelPanel != 0) {
        ((HudUiScalarDeletingDestructorDispatch *)(checkedLabelPanel))
            ->ScalarDeletingDestructor(1);
        checkedLabelPanel = 0;
    }

    base.DestructorCore();
}

// Reimplements 0x4b7000: HudUiCheckToggleWidget::ScalarDeletingDestructor
HudUiCheckToggleWidget *RECOIL_THISCALL
HudUiCheckToggleWidget::ScalarDeletingDestructor(unsigned int flags) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b70b0: HudUiCheckToggleWidget::GetBoundsRectOrNull
HudUiRect *RECOIL_THISCALL HudUiCheckToggleWidget::GetBoundsRectOrNull() {
    return &base.boundsRect;
}

// Reimplements 0x4b70c0: HudUiCheckToggleWidget::RefreshState
void RECOIL_THISCALL HudUiCheckToggleWidget::RefreshState() {
    HudUiSetPanelVectorVisible(base.rolloverLabelPanels, 0);
    HudUiSetPanelVectorVisible(base.activateLabelPanels, 0);

    if (base.modeOrEnabled != 0) {
        HudUiSetPanelVectorVisible(base.labelPanels, 1);
        HudUiSetPanelVectorVisible(base.disabledLabelPanels, 0);

        if (checked != 0) {
            zVidImagePartial *const image = checkedImage != 0 ? checkedImage : uncheckedImage;
            if (image != 0) {
                base.base.SetImageBorrowedAndInvalidate(image);
                HudUiVirtualInvalidateRequired(this);
                return;
            }
        }

        HudUiVirtualInvalidateRequired(this);
        return;
    }

    HudUiSetPanelVectorVisible(base.labelPanels, 0);
    HudUiSetPanelVectorVisible(base.disabledLabelPanels, 1);

    if (checked != 0) {
        zVidImagePartial *const image =
            disabledCheckedImage != 0 ? disabledCheckedImage : disabledCheckedFallbackImage;
        if (image != 0) {
            base.base.SetImageBorrowedAndInvalidate(image);
        }
    }

    HudUiVirtualInvalidateRequired(this);
}

// Reimplements 0x4b7210: HudUiCheckToggleWidget::ShowPreview
void RECOIL_THISCALL HudUiCheckToggleWidget::ShowPreview() {
    if (base.modeOrEnabled == 0 || checked != 0) {
        return;
    }

    if (base.rolloverSound != 0) {
        base.rolloverPlayHandle = base.rolloverSound->PlayA3DSimple(base.rolloverSoundScale);
    }

    base.ShowPreview();
}

// Reimplements 0x4b7250: HudUiCheckToggleWidget::HidePreview
void RECOIL_THISCALL HudUiCheckToggleWidget::HidePreview() {
    if (base.modeOrEnabled == 0 || checked != 0) {
        return;
    }

    if (base.rolloverPlayHandle != 0) {
        base.rolloverPlayHandle->StopIfActive();
        base.rolloverPlayHandle = 0;
    }

    base.HidePreview();
}

// Reimplements 0x4b7290: HudUiCheckToggleWidget::OnActivate
void RECOIL_THISCALL HudUiCheckToggleWidget::OnActivate() {
    if (base.modeOrEnabled == 0) {
        return;
    }

    SetChecked(checked == 0 ? 1 : 0);
    base.OnActivate();
}

// Reimplements 0x4b7340: HudUiCheckToggleWidget::LoadFromZrd
int RECOIL_THISCALL HudUiCheckToggleWidget::LoadFromZrd(zReader::Node *zrdSection,
                                                                 void *ownerDialog) {
    base.LoadFromZrd(zrdSection, ownerDialog);
    uncheckedImage = base.base.image;

    zReader::Node *const checkedNode = zReader_GetNamedNode(zrdSection, "CHECKED");
    if (checkedNode != 0) {
        LoadHudZrdBitmap(checkedNode, "BITMAP", &checkedImage);
        zReader::Node *const textNode = zReader_GetNamedNode(checkedNode, "TEXT");
        if (textNode != 0) {
            checkedLabelPanel = CreateHudZrdTextPanel(&base, textNode, 0);
        }
    }

    zReader::Node *const disabledUnselectedNode = zReader_GetNamedNode(zrdSection, "DISABLE_UNSEL");
    if (disabledUnselectedNode != 0) {
        LoadHudZrdBitmap(disabledUnselectedNode, "BITMAP", &disabledCheckedFallbackImage);
        zReader::Node *const textNode = zReader_GetNamedNode(disabledUnselectedNode, "TEXT");
        if (textNode != 0) {
            checkedLabelPanel = CreateHudZrdTextPanel(&base, textNode, 0);
        }

        LoadHudZrdLabelSection(&base, disabledUnselectedNode, base.disabledLabelPanels);
    }

    zReader::Node *const disabledSelectedNode = zReader_GetNamedNode(zrdSection, "DISABLE_SEL");
    if (disabledSelectedNode != 0) {
        LoadHudZrdBitmap(disabledSelectedNode, "BITMAP", &disabledCheckedImage);
        zReader::Node *const textNode = zReader_GetNamedNode(disabledSelectedNode, "TEXT");
        if (textNode != 0) {
            checkedLabelPanel = CreateHudZrdTextPanel(&base, textNode, 0);
        }
    }

    if (uncheckedImage != 0) {
        base.boundsRect.left = base.base.x;
        base.boundsRect.top = base.base.y;
        base.boundsRect.right = base.base.x + uncheckedImage->width;
        base.boundsRect.bottom = base.base.y + uncheckedImage->height;
    } else if (base.labelPanels.begin != 0) {
        base.GetBoundsRectOrNull();
    }

    return 1;
}

// Reimplements 0x4b72c0: HudUiCheckToggleWidget::SetChecked
RECOIL_NOINLINE int RECOIL_THISCALL
HudUiCheckToggleWidget::SetChecked(int newChecked) {
    const int previousChecked = checked;
    checked = newChecked;

    if (newChecked != 0) {
        if (checkedImage != 0) {
            base.base.SetImageBorrowedAndInvalidate(checkedImage);
        }

        if (checkedLabelPanel != 0) {
            HudUiVirtualSetVisibleRequired(checkedLabelPanel, 1);
            HudUiVirtualInvalidateRequired(this);
            return previousChecked;
        }
    } else {
        if (uncheckedImage != 0) {
            base.base.SetImageBorrowedAndInvalidate(uncheckedImage);
        }

        if (checkedLabelPanel != 0) {
            HudUiVirtualSetVisibleRequired(checkedLabelPanel, 0);
        }
    }

    HudUiVirtualInvalidateRequired(this);
    return previousChecked;
}

// Reimplements 0x4b7d60: HudUiCycleSelectorWidget::Constructor
HudUiCycleSelectorWidget *RECOIL_THISCALL HudUiCycleSelectorWidget::Constructor() {
    base.Constructor();
    base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiCycleSelectorWidget_FTable);
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
    return this;
}

// Reimplements 0x4b7de0: HudUiCycleSelectorWidget::DestructorCore
void RECOIL_THISCALL HudUiCycleSelectorWidget::DestructorCore() {
    base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiCycleSelectorWidget_FTable);

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

    base.DestructorCore();
}

// Reimplements 0x4b7dc0: HudUiCycleSelectorWidget::ScalarDeletingDestructor
HudUiCycleSelectorWidget *RECOIL_THISCALL
HudUiCycleSelectorWidget::ScalarDeletingDestructor(unsigned int flags) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b7ee0: HudUiCycleSelectorWidget::AdvanceSelectionAndActivate
void RECOIL_THISCALL HudUiCycleSelectorWidget::AdvanceSelectionAndActivate() {
    const int nextIndex = selectedIndex + 1;
    selectedIndex = nextIndex;

    int endIndex = visibleCount;
    if (endIndex >= itemCount) {
        endIndex = itemCount;
    }

    if (nextIndex >= endIndex) {
        selectedIndex = firstIndex;
    }

    base.OnActivate();
}

// Reimplements 0x4b7f20: HudUiCycleSelectorWidget::SetIndexClamped
void RECOIL_THISCALL HudUiCycleSelectorWidget::SetIndexClamped(int index) {
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
void RECOIL_THISCALL HudUiCycleSelectorWidget::SetVisibleRange(int first,
                                                               int last) {
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
void RECOIL_THISCALL HudUiCycleSelectorWidget::Update(float deltaSeconds) {
    for (int i = 0; i < itemCount; ++i) {
        HudUiVirtualInvalidate(this);

        if (entriesA[i] != 0) {
            HudUiVirtualSetVisible(entriesA[i], i == selectedIndex ? 1 : 0);
        }

        if (entriesB[i] != 0) {
            HudUiVirtualSetVisible(entriesB[i], i == selectedIndex ? 1 : 0);
        }
    }

    ((HudUiElement *)(this))->Update(deltaSeconds);
}

// Reimplements 0x4b7fd0: HudUiCycleSelectorWidget::AddTextEntry
void RECOIL_THISCALL HudUiCycleSelectorWidget::AddTextEntry(int index, const char *text,
                                                            int posX, int posY) {
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

    HudUiTransitionTextPanel *const transitionPanel = (HudUiTransitionTextPanel *)(::operator new(sizeof(HudUiTransitionTextPanel)));
    HudUiPanel *const panel = (HudUiPanel *)(transitionPanel);
    panel->ConstructorDefault(0, 0, 0);
    *(const HudUiPanel_FTable **)(transitionPanel) =
        &g_HudUiTransitionTextPanel_FTable;
    transitionPanel->flashResetValue = 0.349999994f;
    transitionPanel->flashCountdown = 0;
    transitionPanel->flashAltColor0 = 0;
    transitionPanel->flashEnabled = 0;
    transitionPanel->flashMode = 0;
    transitionPanel->flashDirectionSign = 1;

    entriesA[index] = (HudUiWidget *)(transitionPanel);
    HudUiPanelVirtualSetTextFmtRequired(panel, text);

    HudUiElement *const element = (HudUiElement *)(transitionPanel);
    HudUiVirtualSetPosRequired(element, textOffsetX + posX, textOffsetY + posY);
    HudUiVirtualSetVisibleRequired(element, 0);
    ((HudUiContainer *)(base.owner))->AddChild(element);
}

// Reimplements 0x4ba020: HudUiTransitionTextPanel::Constructor
HudUiTransitionTextPanel *RECOIL_THISCALL HudUiTransitionTextPanel::Constructor() {
    HudUiPanel *const panel = (HudUiPanel *)(this);
    panel->ConstructorDefault(0, 0, 0);
    *(const HudUiPanel_FTable **)(this) = &g_HudUiTransitionTextPanel_FTable;
    flashCountdown = 0;
    flashAltColor0 = 0;
    flashEnabled = 0;
    flashMode = 0;
    flashResetValue = 0.349999994f;
    flashDirectionSign = 1;
    return this;
}

// Reimplements 0x4bc9f0: HudUiTransitionTextPanel::TickFlash
void RECOIL_THISCALL HudUiTransitionTextPanel::TickFlash(float deltaSeconds) {
    HudUiElement *const element = (HudUiElement *)(this);
    HudUiPanel *const panel = (HudUiPanel *)(this);

    const unsigned int elementFlags = element->flags;
    if ((elementFlags & 0x10u) != 0) {
        return;
    }

    if ((elementFlags & 1u) != 0) {
        element->timer -= deltaSeconds;
        if (element->timer <= 0.0f) {
            element->SetVisible(0);
        }
    }

    if (flashEnabled == 0 || (element->flags & 0x10u) != 0) {
        panel->Draw();
        return;
    }

    flashCountdown -= deltaSeconds;
    switch (flashMode) {
    case 0:
        panel->Draw();
    case 1:
        if (flashCountdown < 0.0f) {
            flashCountdown += flashResetValue;
            FieldAt<unsigned int>(this, 0x270) = 1;
            flashDirectionSign = -flashDirectionSign;
        }

        if (flashDirectionSign == 1) {
            panel->Draw();
        }
        return;

    case 2:
    case 3:
        if (flashCountdown < 0.0f) {
            flashCountdown = flashResetValue;
            FieldAt<unsigned int>(this, 0x270) = 1;
            flashDirectionSign = -flashDirectionSign;

            const unsigned int textColor0 = FieldAt<unsigned int>(this, 0x14c);
            const unsigned int textColor1 = FieldAt<unsigned int>(this, 0x150);
            FieldAt<unsigned int>(this, 0x14c) = (unsigned int)(flashAltColor0);
            FieldAt<unsigned int>(this, 0x150) = (unsigned int)(flashAltColor1);
            flashAltColor0 = (int)(textColor0);
            flashAltColor1 = (int)(textColor1);
        }

        panel->Draw();
        return;

    default:
        panel->Draw();
        return;
    }
}

// Reimplements 0x4bc930: HudUiTransitionTextPanel::ResetFlashState
void RECOIL_THISCALL HudUiTransitionTextPanel::ResetFlashState(float flashRate) {
    flashEnabled = 1;
    if (flashRate > 0.0f) {
        flashResetValue = flashRate * 0.5f;
    }

    flashDirectionSign = 1;
    memcpy(&flashCountdown, &flashResetValue, sizeof(flashCountdown));
}

// Reimplements 0x4bc9b0: HudUiTransitionTextPanel::SetFlashColorAndRate
void RECOIL_THISCALL HudUiTransitionTextPanel::SetFlashColorAndRate(unsigned int flashColor,
                                                                    float flashRate) {
    if (flashMode == 2) {
        return;
    }

    ResetFlashState(flashRate);
    flashMode = 2;
    flashAltColor0 = flashColor;
    flashAltColor1 = flashColor;
}

// Reimplements 0x4b8100: HudUiCycleSelectorWidget::ApplyFontStyleForEntry
void RECOIL_THISCALL HudUiCycleSelectorWidget::ApplyFontStyleForEntry(int index,
                                                                      int styleIndex) {
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

    const HudFontStyle *const stylesBase = (const HudFontStyle *)(
        (const unsigned char *)(base.owner) + 0x1cec);
    const HudFontStyle *const style = &stylesBase[styleIndex];
    if (style->validMarker == 0) {
        return;
    }

    HudUiPanel *const panel = (HudUiPanel *)(entriesA[index]);
    HudUiPanelVirtualSetFontRequired(panel, style->fontName, style->fontSize, style->fontWeight, 0,
                                     0, 0, 2);

    FieldAt<unsigned int>(panel, 0x14c) = style->textColor;
    FieldAt<unsigned int>(panel, 0x150) = style->textColor;
    FieldAt<unsigned int>(panel, 0x270) = 1;
    FieldAt<unsigned int>(panel, 0x264) = style->shadowEnabled;
    FieldAt<int>(panel, 0x29c) = 1;
    FieldAt<int>(panel, 0x2a0) = 1;
    FieldAt<unsigned int>(panel, 0x144) = style->alignMode;
    FieldAt<unsigned int>(panel, 0x268) = style->bkMode;
    FieldAt<unsigned int>(panel, 0x26c) = style->bkColor;
}

// Reimplements 0x4b8200: HudUiCycleSelectorWidget::AddBitmapEntry
void RECOIL_THISCALL HudUiCycleSelectorWidget::AddBitmapEntry(int index,
                                                              const char *imagePath,
                                                              int posX,
                                                              int posY) {
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
    HudUiVirtualSetPosRequired(bitmapWidget, posX, posY);
    HudUiVirtualSetVisibleRequired(bitmapWidget, 0);
    ((HudUiContainer *)(base.owner))->AddChild((HudUiElement *)(bitmapWidget));
}

// Reimplements 0x4b82e0: HudUiCycleSelectorWidget::LoadFromZrd
int RECOIL_THISCALL HudUiCycleSelectorWidget::LoadFromZrd(zReader::Node *zrdSection,
                                                                   void *ownerDialog) {
    base.LoadFromZrd(zrdSection, ownerDialog);

    zReader::Node *const fontNode = zReader_GetNamedNode(zrdSection, "FONT");
    if (fontNode != 0) {
        fontStyleRef = (void *)((unsigned int)(fontNode->value.u32));
    }

    zReader::Node *const textOffsetNode = zReader_GetNamedNode(zrdSection, "TEXTOFFSET");
    zReader::Node *const textOffsetBase = ZrdArrayBase(textOffsetNode);
    if (textOffsetBase != 0) {
        textOffsetX = ZrdArrayInt(textOffsetBase, 1, textOffsetX);
        textOffsetY = ZrdArrayInt(textOffsetBase, 2, textOffsetY);
    }

    zReader::Node *const cycleNode = zReader_GetNamedNode(zrdSection, "CYCLE");
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
        zReader::Node *const entryNode = ZrdArrayItem(cycleBase, index + 1);

        zReader::Node *const textNode = zReader_GetNamedNode(entryNode, "TEXT");
        zReader::Node *const textBase = ZrdArrayBase(textNode);
        if (textBase != 0) {
            const char *const key = ZrdArrayString(textBase, 1);
            const char *const text = key != 0 ? zLoc::ResolveMessageKeyOrFallback(key) : "";
            AddTextEntry(index, text != 0 ? text : "",
                         base.originX + ZrdArrayInt(textBase, 2, 0),
                         base.originY + ZrdArrayInt(textBase, 3, 0));
            ApplyFontStyleForEntry(index, ZrdArrayInt(textBase, 4, 0));
        }

        zReader::Node *const bitmapNode = zReader_GetNamedNode(entryNode, "BITMAP");
        zReader::Node *const bitmapBase = ZrdArrayBase(bitmapNode);
        if (bitmapBase != 0) {
            int bitmapX = base.originX;
            int bitmapY = base.originY;
            if (ZrdArrayCount(bitmapBase) >= 4) {
                bitmapX += ZrdArrayInt(bitmapBase, 2, 0);
                bitmapY += ZrdArrayInt(bitmapBase, 3, 0);
            }

            AddBitmapEntry(index, ZrdArrayString(bitmapBase, 1), bitmapX, bitmapY);
        }
    }
    }

    return 1;
}

// Reimplements 0x4b8450: HudUiFillBitmap::Constructor
HudUiFillBitmap *RECOIL_THISCALL HudUiFillBitmap::Constructor() {
    base.Constructor();
    base.base.ftable = (const HudUiWidget_FTable *)(&g_HudUiFillBitmap_FTable);
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
    return this;
}

// Reimplements 0x4b84d0: HudUiFillBitmap::DestructorCore
void RECOIL_THISCALL HudUiFillBitmap::DestructorCore() {
    base.base.ftable = (const HudUiWidget_FTable *)(&g_HudUiFillBitmap_FTable);

    if (previewImage != 0 && previewImage != base.base.image) {
        zVid_Image::ReleaseIfNotDefault(previewImage);
        previewImage = 0;
    }

    if (fillImage != 0 && fillImage != base.base.image) {
        zVid_Image::ReleaseIfNotDefault(fillImage);
        fillImage = 0;
    }

    base.DestructorCore();
}

// Reimplements 0x4b84b0: HudUiFillBitmap::ScalarDeletingDestructor
HudUiFillBitmap *RECOIL_THISCALL HudUiFillBitmap::ScalarDeletingDestructor(unsigned int flags) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b8520: HudUiFillBitmap::Draw
void RECOIL_THISCALL HudUiFillBitmap::Draw() {
    if (previewImage == 0 || fillImage == 0) {
        return;
    }

    base.base.Draw();

    if (fillRect.left != fillRect.right) {
        zVid_Image::BlitToActiveTarget(fillImage, base.base.x + fillOffsetX,
                                       base.base.y + fillOffsetY, 0,
                                       (zVidRect32 *)(&fillRect));
    }

    if (previewRect.left != previewRect.right) {
        zVid_Image::BlitToActiveTarget(previewImage, base.base.x + previewOffsetX,
                                       base.base.y + previewOffsetY, 0,
                                       (zVidRect32 *)(&previewRect));
    }
}

// Reimplements 0x4b85c0: HudUiFillBitmap::LoadFromZrd
int RECOIL_THISCALL HudUiFillBitmap::LoadFromZrd(zReader::Node *zrdSection,
                                                          void *ownerDialog) {
    base.LoadFromZrd(zrdSection, ownerDialog);

    zReader::Node *const fillBitmapNode = zReader_GetNamedNode(zrdSection, "FILLBITMAP");
    zReader::Node *const fillBitmapBase = ZrdArrayBase(fillBitmapNode);
    if (fillBitmapBase != 0) {
        fillImage = zImage::TexDir_FindOrCreateByPath(ZrdArrayString(fillBitmapBase, 1));
        ((HudUiElement *)(this))->Invalidate();

        int posX = base.originX;
        int posY = base.originY;
        if (ZrdArrayCount(fillBitmapBase) >= 4) {
            posX += ZrdArrayInt(fillBitmapBase, 2, 0);
            posY += ZrdArrayInt(fillBitmapBase, 3, 0);
        }

        ((HudUiElement *)(this))->SetPos(posX, posY);
        previewImage = base.base.image;
        ((HudUiElement *)(this))->Invalidate();
    }

    SetNormalizedValueAndRebuild(0.0f);
    return 1;
}

// Reimplements 0x4b8650: HudUiFillBitmap::UpdateNormalizedFromCursor
void RECOIL_THISCALL HudUiFillBitmap::UpdateNormalizedFromCursor() {
    const int cursorX = OwnerField<int>(base.owner, 0x14);
    const int relativeX = cursorX - HudUiVirtualGetX(this);
    const int imageWidth = base.base.image != 0 ? base.base.image->width : 0;
    SetNormalizedValueAndRebuild((float)(relativeX) / (float)(imageWidth));
    base.OnActivate();
}

// Reimplements 0x4ba3c0: HudUiFillBitmap::SetNormalizedValue
void RECOIL_THISCALL HudUiFillBitmap::SetNormalizedValue(float value) {
    unsigned int valueBits = 0;
    memcpy(&valueBits, &value, sizeof(valueBits));
    FieldAt<unsigned int>(this, 0x14c) = valueBits;
    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiElement * self);
    HudUiElement *const element = (HudUiElement *)(this);
    ((InvalidateFn)(element->ftable->slots[8]))(element);
}

// Reimplements 0x4b86b0: HudUiFillBitmap::SetNormalizedValueAndRebuild
void RECOIL_THISCALL HudUiFillBitmap::SetNormalizedValueAndRebuild(float value) {
    if (fillImage == 0) {
        return;
    }

    normalizedValue = value;
    ((HudUiElement *)(this))->Invalidate();

    const int fillWidth = fillImage->width;
    const int fillHeight = fillImage->height;
    const int filledWidth =
        (int)((float)(fillWidth) * value);

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

// Reimplements 0x4b8760: HudUiZrdWidgetEx17C_Item::Constructor
HudUiZrdWidgetEx17C_Item *RECOIL_THISCALL HudUiZrdWidgetEx17C_Item::Constructor() {
    base.Constructor();
    base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiZrdWidgetEx17C_Item_FTable);
    selected = 0;
    ownerSelector = 0;
    mouseRectValid = 0;
    selectedImage = 0;
    unselectedImage = 0;
    return this;
}

// Reimplements 0x4b87c0: HudUiZrdWidgetEx17C_Item::DestructorCore
void RECOIL_THISCALL HudUiZrdWidgetEx17C_Item::DestructorCore() {
    base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiZrdWidgetEx17C_Item_FTable);
    base.DestructorCore();
}

// Reimplements 0x4b87a0: HudUiZrdWidgetEx17C_Item::ScalarDeletingDestructor
HudUiZrdWidgetEx17C_Item *RECOIL_THISCALL
HudUiZrdWidgetEx17C_Item::ScalarDeletingDestructor(unsigned int flags) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b87d0: HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected
void RECOIL_THISCALL HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected() {
    if (selected == 0) {
        base.ShowPreview();
    }
}

// Reimplements 0x4b87e0: HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected
void RECOIL_THISCALL HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected() {
    if (selected == 0) {
        base.HidePreview();
    }
}

// Reimplements 0x4b87f0: HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf
void RECOIL_THISCALL HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf() {
    ownerSelector->SetSelectedIndex(itemIndex);
    ownerSelector->base.OnActivate();
    base.OnActivate();

    {
    for (int index = 0; index < ownerSelector->optionCount; ++index) {
        ownerSelector->options[index]->HidePreviewIfNotSelected();
    }
    }
}

// Reimplements 0x4b8850: HudUiZrdWidgetEx17C_Item::LoadFromZrd
int RECOIL_THISCALL HudUiZrdWidgetEx17C_Item::LoadFromZrd(zReader::Node *zrdSection,
                                                                   void *ownerDialog) {
    base.LoadFromZrd(zrdSection, ownerDialog);

    unselectedImage = base.base.image;
    unselectedRolloverImage = base.rolloverImage;
    selectedImage = base.activateImage;
    selectedRolloverImage = base.activateImage;

    HudUiRect *const bounds = base.GetBoundsRectOrNull();
    if (bounds != 0) {
        mouseRect = *bounds;
    } else {
        memset(&mouseRect, 0, sizeof(mouseRect));
    }
    mouseRectValid = 1;

    zReader::Node *const mouseOverNode = zReader_GetNamedNode(zrdSection, "MOUSEOVER");
    zReader::Node *const mouseOverBase = ZrdArrayBase(mouseOverNode);
    if (mouseOverBase != 0) {
        mouseRect.top += ZrdArrayInt(mouseOverBase, 1, 0);
        mouseRect.left += ZrdArrayInt(mouseOverBase, 2, 0);
        mouseRect.bottom = mouseRect.top + ZrdArrayInt(mouseOverBase, 3, 0);
        mouseRect.right = mouseRect.left + ZrdArrayInt(mouseOverBase, 4, 0);
    }

    return 1;
}

// Reimplements 0x4b8a90: HudUiZrdWidgetEx17C_Item::SetSelected
void RECOIL_THISCALL HudUiZrdWidgetEx17C_Item::SetSelected(int selectedValue) {
    selected = selectedValue;
    if (base.modeOrEnabled == 0) {
        return;
    }

    if (selectedValue != 0) {
        base.defaultImage = selectedImage;
        base.rolloverImage = selectedRolloverImage;
    } else {
        base.defaultImage = unselectedImage;
        base.rolloverImage = unselectedRolloverImage;
    }

    base.base.SetImageBorrowedAndInvalidate(base.defaultImage);
}

// Reimplements 0x4b8af0: HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds
HudUiRect *RECOIL_THISCALL HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds() {
    return mouseRectValid != 0 ? &mouseRect : base.GetBoundsRectOrNull();
}

// Reimplements 0x4b8b10: HudUiZrdWidgetEx17C::Constructor
HudUiZrdWidgetEx17C *RECOIL_THISCALL HudUiZrdWidgetEx17C::Constructor() {
    base.Constructor();
    base.base.ftable = (const HudUiWidget_FTable *)(&g_HudUiZrdWidgetEx17C_FTable);
    optionCount = 0;

    {
        int optionIndex;
        for (optionIndex = 0; optionIndex < 10; ++optionIndex) {
            options[optionIndex] = 0;
        }
    }

    return this;
}

// Reimplements 0x4b8b60: HudUiZrdWidgetEx17C::DestructorCore
void RECOIL_THISCALL HudUiZrdWidgetEx17C::DestructorCore() {
    base.base.ftable = (const HudUiWidget_FTable *)(&g_HudUiZrdWidgetEx17C_FTable);

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

    base.DestructorCore();
}

// Reimplements 0x4b8b40: HudUiZrdWidgetEx17C::ScalarDeletingDestructor
HudUiZrdWidgetEx17C *RECOIL_THISCALL
HudUiZrdWidgetEx17C::ScalarDeletingDestructor(unsigned int flags) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b8be0: HudUiZrdWidgetEx17C::LoadFromZrd
int RECOIL_THISCALL HudUiZrdWidgetEx17C::LoadFromZrd(zReader::Node *zrdSection,
                                                              void *ownerDialog) {
    base.owner = ownerDialog;

    zReader::Node *const radioNode = zReader_GetNamedNode(zrdSection, "RADIO");
    zReader::Node *const radioBase = ZrdArrayBase(radioNode);
    if (radioBase != 0) {
        optionCount = ZrdArrayCount(radioBase) - 1;
        if (optionCount >= 10) {
            optionCount = 10;
        }

        {
        for (int index = 0; index < optionCount; ++index) {
            HudUiZrdWidgetEx17C_Item *const option = (HudUiZrdWidgetEx17C_Item *)(
                ::operator new(sizeof(HudUiZrdWidgetEx17C_Item)));
            option->Constructor();
            options[index] = option;

            option->LoadFromZrd(&radioBase[index + 1], ownerDialog);
            option->ownerSelector = this;
            option->itemIndex = index;
        }
        }
    }

    SetSelectedIndex(0);
    return 1;
}

// Reimplements 0x409010: HudUiZrdWidgetEx17C::EnableChildAtIndex
void RECOIL_THISCALL HudUiZrdWidgetEx17C::EnableChildAtIndex(int childIndex) {
    typedef void (RECOIL_THISCALL *RefreshStateFn)(HudUiZrdWidgetEx17C_Item * self);

    if (childIndex >= optionCount) {
        return;
    }

    HudUiZrdWidgetEx17C_Item *const option = options[childIndex];
    option->selected = 1;
    ((RefreshStateFn)(option->base.base.ftable->slots[30]))(option);
}

// Reimplements 0x4b8cf0: HudUiZrdWidgetEx17C::SetSelectedIndex
int RECOIL_THISCALL HudUiZrdWidgetEx17C::SetSelectedIndex(int index) {
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

// Reimplements 0x4b92a0: HudUiListSelectorItem::Constructor
HudUiListSelectorItem *RECOIL_THISCALL HudUiListSelectorItem::Constructor() {
    ((HudUiPanel *)(this))->ConstructorDefault(0, 0, 0);
    ((HudUiPanel *)(this))->vtbl = &g_HudUiListSelectorItem_FTable;
    return this;
}

// Reimplements 0x4ba410: HudUiListSelectorItem::Draw
void RECOIL_THISCALL HudUiListSelectorItem::Draw() {
    HudUiPanel *const panel = (HudUiPanel *)(this);
    panel->Draw();

    FieldAt<int>(panel, 0x20) = HudUiVirtualGetXRequired(panel);
    if (FieldAt<unsigned int>(panel, 0x270) != 0) {
        HudUiPanelVirtualRebuildTextRectRequired(panel);
    }

    FieldAt<int>(panel, 0x28) =
        HudUiVirtualGetXRequired(panel) + FieldAt<int>(panel, 0x25c);
    FieldAt<int>(panel, 0x24) = HudUiVirtualGetYRequired(panel);
    const int textHeight = panel->QueryTextHeight();
    FieldAt<int>(panel, 0x2c) = textHeight + HudUiVirtualGetYRequired(panel);
}

// Reimplements 0x4b8d30: HudCmdBindButtonBase::Constructor
HudCmdBindButtonBase *RECOIL_THISCALL HudCmdBindButtonBase::Constructor() {
    base.Constructor();
    bindPanel.Constructor();

    bindingVec.begin = 0;
    bindingVec.end = 0;
    bindingVec.capacity = 0;
    bindingSlotTotalCount = 0;
    bindingSlotPanels = 0;
    visibleListOffsetX = 0.0f;
    visibleListOffsetY = 0.0f;
    overflowListOffsetX = 0.0f;
    overflowListOffsetY = 0.0f;
    base.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudCmdBindButtonBase_FTable);
    bindingSlotSpacing = 0xf;
    selectedBindingIndex = -1;
    return this;
}

// Reimplements 0x40bdf0: StdPtrVector::ClearNoOpDestroy
void RECOIL_THISCALL StdPtrVector::ClearNoOpDestroy(int *begin, int *end) {
    (void)begin;
    (void)end;
}

// Reimplements 0x40be60: HudCmdBindingEntry::CopyRange
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
HudCmdBindingEntry **RECOIL_FASTCALL
HudCmdBindingEntry::CopyRange(HudCmdBindingEntry **sourceBegin,
                              HudCmdBindingEntry **sourceEnd,
                              HudCmdBindingEntry **dest)
{
    if (sourceBegin != sourceEnd)
    {
        do
        {
            *dest = *sourceBegin;
            ++sourceBegin;
            ++dest;
        } while (sourceBegin != sourceEnd);
    }

    return dest;
}

// Reimplements 0x40bf50: HudCmdBindingEntry::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
HudCmdBindingEntry *RECOIL_THISCALL
HudCmdBindingEntry::ScalarDeletingDestructor(unsigned int flags)
{
    if (displayText != 0)
    {
        free(displayText);
        displayText = 0;
    }

    if ((flags & 1u) != 0)
    {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40bf20: HudCmdBindingEntry::DeleteAndReturnNull
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
HudCmdBindingEntry *RECOIL_STDCALL
HudCmdBindingEntry::DeleteAndReturnNull(HudCmdBindingEntry *entry)
{
    if (entry != 0)
    {
        if (entry->displayText != 0)
        {
            free(entry->displayText);
            entry->displayText = 0;
        }

        ::operator delete(entry);
    }

    return 0;
}

// Reimplements 0x40bf80: HudCmdBindButtonBase::AddBindingEntry
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
int RECOIL_THISCALL
HudCmdBindButtonBase::AddBindingEntry(const char *displayText, int commandId)
{
    HudCmdBindingEntry **begin = (HudCmdBindingEntry **)(bindingVec.begin);
    HudCmdBindingEntry **end = (HudCmdBindingEntry **)(bindingVec.end);
    HudCmdBindingEntry **capacity = (HudCmdBindingEntry **)(bindingVec.capacity);
    const int oldCount = begin != 0 ? (int)(end - begin) : 0;

    HudCmdBindingEntry *const entry =
        (HudCmdBindingEntry *)(::operator new(sizeof(HudCmdBindingEntry)));
    if (entry != 0)
    {
        entry->displayText = _strdup(displayText);
        entry->commandId = commandId;
    }

    const int hasCapacity = begin != 0 && (capacity - end) >= 1;
    if (!hasCapacity)
    {
        const int growCount = oldCount > 1 ? oldCount : 1;
        const int newCapacityCount = oldCount + growCount;
        HudCmdBindingEntry **const newBegin =
            (HudCmdBindingEntry **)(::operator new((unsigned int)newCapacityCount *
                                                   sizeof(HudCmdBindingEntry *)));

        for (int index = 0; index < oldCount; ++index)
        {
            newBegin[index] = begin[index];
        }
        newBegin[oldCount] = entry;

        ::operator delete(begin);
        bindingVec.begin = newBegin;
        bindingVec.end = newBegin + oldCount + 1;
        bindingVec.capacity = newBegin + newCapacityCount;
    }
    else
    {
        *end = entry;
        bindingVec.end = end + 1;
    }

    return oldCount;
}

// Reimplements 0x4b9330: HudCmdBindButtonBase::SetSelectedEntry
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void RECOIL_THISCALL
HudCmdBindButtonBase::SetSelectedEntry(int selectedIndex)
{
    HudCmdBindingEntry **const entries = (HudCmdBindingEntry **)(bindingVec.begin);
    const int entryCount =
        entries != 0 ? (int)((HudCmdBindingEntry **)(bindingVec.end) - entries) : 0;

    int slotIndex;
    for (slotIndex = 0; slotIndex < visibleBindingSlotCount; ++slotIndex)
    {
        HudUiListSelectorItem *const item = &bindingSlotPanels[slotIndex];
        const int entryIndex = selectedIndex + slotIndex - visibleBindingSlotCount;
        if (entryIndex >= 0 && entryIndex < entryCount)
        {
            FieldAt<int>(item, 0x2a4) = entryIndex;
            HudUiPanelVirtualSetTextFmtStringRequired(item,
                                                      entries[entryIndex]->displayText);
            HudUiVirtualSetVisibleRequired(item, 1);
        }
        else
        {
            HudUiVirtualSetVisibleRequired(item, 0);
            HudUiVirtualDrawRequired(item);
        }

        HudUiVirtualInvalidateRequired(item);
    }

    if (selectedIndex >= 0 && selectedIndex < entryCount)
    {
        FieldAt<int>(&bindPanel, 0x2a4) = selectedIndex;
        HudUiPanelVirtualSetTextFmtStringRequired(&bindPanel,
                                                  entries[selectedIndex]->displayText);
    }

    for (slotIndex = visibleBindingSlotCount; slotIndex < bindingSlotTotalCount; ++slotIndex)
    {
        HudUiListSelectorItem *const item = &bindingSlotPanels[slotIndex];
        const int entryIndex = selectedIndex + slotIndex - visibleBindingSlotCount + 1;
        if (entryIndex >= 0 && entryIndex < entryCount)
        {
            FieldAt<int>(item, 0x2a4) = entryIndex;
            HudUiPanelVirtualSetTextFmtStringRequired(item,
                                                      entries[entryIndex]->displayText);
            HudUiVirtualSetVisibleRequired(item, 1);
        }
        else
        {
            HudUiVirtualSetVisibleRequired(item, 0);
            HudUiVirtualDrawRequired(item);
        }

        HudUiVirtualInvalidateRequired(item);
    }

    selectedBindingIndex = selectedIndex;
}

// Reimplements 0x40be00: HudCmdBinding::DestroyRange
// (HudCmdDialog.cpp)
HudCmdBinding **RECOIL_FASTCALL
HudCmdBinding::DestroyRange(HudCmdBinding **first, HudCmdBinding **last,
                            HudCmdBinding **dest, void *unusedAlloc)
{
    (void)unusedAlloc;

    while (first != last)
    {
        HudCmdBinding *const binding = *first;
        if (binding != 0)
        {
            if (binding->displayText != 0)
            {
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
RECOIL_NOINLINE void **RECOIL_FASTCALL zUtil_StdPtrVector_Clear(HudCmdBindingVector *self) {
    void **const oldEnd = (void **)(self->end);
    self->end = self->begin;
    return oldEnd;
}

// Reimplements 0x4ba470: zUtil_StdPtrVector_FreeBufferAndReset
RECOIL_NOINLINE void RECOIL_FASTCALL
zUtil_StdPtrVector_FreeBufferAndReset(HudCmdBindingVector *self) {
    ::operator delete(self->begin);
    self->begin = 0;
    self->end = 0;
    self->capacity = 0;
}

// Reimplements 0x40c1d0: HudCmdBindButtonBase::ClearBindingEntries
void RECOIL_THISCALL HudCmdBindButtonBase::ClearBindingEntries() {
    HudCmdBindingEntry ** entry = (HudCmdBindingEntry **)(bindingVec.begin);
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

// Reimplements 0x40a940: HudCmdCommandList::Destructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void RECOIL_THISCALL HudCmdCommandList::Destructor()
{
    base.base.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudCmdBindButtonBase_FTable);

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(base.bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(base.bindingVec.end);
    while (entry != end)
    {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0)
        {
            if (binding->displayText != 0)
            {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd =
        (HudCmdBindingEntry **)(base.bindingVec.end);
    HudCmdBindingEntry **const oldBegin =
        (HudCmdBindingEntry **)(base.bindingVec.begin);
    base.bindingVec.end =
        HudCmdBindingEntry::CopyRange(oldEnd, oldEnd, oldBegin);
    ((StdPtrVector *)(&base.bindingVec))
        ->ClearNoOpDestroy((int *)(base.bindingVec.end), (int *)(oldEnd));
    ::operator delete(base.bindingVec.begin);
    base.bindingVec.begin = 0;
    base.bindingVec.end = 0;
    base.bindingVec.capacity = 0;

    ((HudUiPanel *)(&base.bindPanel))->Destructor();
    base.base.DestructorCore();
}

// Reimplements 0x40b0a0: HudCmdCommandList::ScalarDeletingDestructor
HudCmdCommandList *RECOIL_THISCALL
HudCmdCommandList::ScalarDeletingDestructor(unsigned int flags)
{
    Destructor();
    if ((flags & 1u) != 0)
    {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40aa30: HudCmdKeyAButton::Destructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void RECOIL_THISCALL HudCmdKeyAButton::Destructor()
{
    base.base.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudCmdBindButtonBase_FTable);

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(base.bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(base.bindingVec.end);
    while (entry != end)
    {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0)
        {
            if (binding->displayText != 0)
            {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd =
        (HudCmdBindingEntry **)(base.bindingVec.end);
    HudCmdBindingEntry **const oldBegin =
        (HudCmdBindingEntry **)(base.bindingVec.begin);
    base.bindingVec.end =
        HudCmdBindingEntry::CopyRange(oldEnd, oldEnd, oldBegin);
    ((StdPtrVector *)(&base.bindingVec))
        ->ClearNoOpDestroy((int *)(base.bindingVec.end), (int *)(oldEnd));
    ::operator delete(base.bindingVec.begin);
    base.bindingVec.begin = 0;
    base.bindingVec.end = 0;
    base.bindingVec.capacity = 0;

    ((HudUiPanel *)(&base.bindPanel))->Destructor();
    base.base.DestructorCore();
}

// Reimplements 0x40ab20: HudCmdKeyBButton::Destructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void RECOIL_THISCALL HudCmdKeyBButton::Destructor()
{
    base.base.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudCmdBindButtonBase_FTable);

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(base.bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(base.bindingVec.end);
    while (entry != end)
    {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0)
        {
            if (binding->displayText != 0)
            {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd =
        (HudCmdBindingEntry **)(base.bindingVec.end);
    HudCmdBindingEntry **const oldBegin =
        (HudCmdBindingEntry **)(base.bindingVec.begin);
    base.bindingVec.end =
        HudCmdBindingEntry::CopyRange(oldEnd, oldEnd, oldBegin);
    ((StdPtrVector *)(&base.bindingVec))
        ->ClearNoOpDestroy((int *)(base.bindingVec.end), (int *)(oldEnd));
    ::operator delete(base.bindingVec.begin);
    base.bindingVec.begin = 0;
    base.bindingVec.end = 0;
    base.bindingVec.capacity = 0;

    ((HudUiPanel *)(&base.bindPanel))->Destructor();
    base.base.DestructorCore();
}

// Reimplements 0x40ac10: HudCmdJoyButton::Destructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void RECOIL_THISCALL HudCmdJoyButton::Destructor()
{
    base.base.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudCmdBindButtonBase_FTable);

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(base.bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(base.bindingVec.end);
    while (entry != end)
    {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0)
        {
            if (binding->displayText != 0)
            {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd =
        (HudCmdBindingEntry **)(base.bindingVec.end);
    HudCmdBindingEntry **const oldBegin =
        (HudCmdBindingEntry **)(base.bindingVec.begin);
    base.bindingVec.end =
        HudCmdBindingEntry::CopyRange(oldEnd, oldEnd, oldBegin);
    ((StdPtrVector *)(&base.bindingVec))
        ->ClearNoOpDestroy((int *)(base.bindingVec.end), (int *)(oldEnd));
    ::operator delete(base.bindingVec.begin);
    base.bindingVec.begin = 0;
    base.bindingVec.end = 0;
    base.bindingVec.capacity = 0;

    ((HudUiPanel *)(&base.bindPanel))->Destructor();
    base.base.DestructorCore();
}

// Reimplements 0x40ad00: HudCmdMouseButton::Destructor
// (D:\Proj\Battlesport\HudCmdBindButton.cpp)
void RECOIL_THISCALL HudCmdMouseButton::Destructor()
{
    base.base.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudCmdBindButtonBase_FTable);

    HudCmdBindingEntry **entry = (HudCmdBindingEntry **)(base.bindingVec.begin);
    HudCmdBindingEntry **const end = (HudCmdBindingEntry **)(base.bindingVec.end);
    while (entry != end)
    {
        HudCmdBindingEntry *const binding = *entry;
        if (binding != 0)
        {
            if (binding->displayText != 0)
            {
                free(binding->displayText);
                binding->displayText = 0;
            }

            ::operator delete(binding);
        }

        *entry = 0;
        ++entry;
    }

    HudCmdBindingEntry **const oldEnd =
        (HudCmdBindingEntry **)(base.bindingVec.end);
    HudCmdBindingEntry **const oldBegin =
        (HudCmdBindingEntry **)(base.bindingVec.begin);
    base.bindingVec.end =
        HudCmdBindingEntry::CopyRange(oldEnd, oldEnd, oldBegin);
    ((StdPtrVector *)(&base.bindingVec))
        ->ClearNoOpDestroy((int *)(base.bindingVec.end), (int *)(oldEnd));
    ::operator delete(base.bindingVec.begin);
    base.bindingVec.begin = 0;
    base.bindingVec.end = 0;
    base.bindingVec.capacity = 0;

    ((HudUiPanel *)(&base.bindPanel))->Destructor();
    base.base.DestructorCore();
}

// Restores repeated inline bind-button cleanup observed in 0x40adf0; no
// standalone helper exists in the retail executable.
static void HudCmdDialog_DestroyBindButtonRange(HudCmdBindButtonBase *button)
{
    button->base.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudCmdBindButtonBase_FTable);

    HudCmdBinding **const begin = (HudCmdBinding **)(button->bindingVec.begin);
    HudCmdBinding **const end = (HudCmdBinding **)(button->bindingVec.end);
    HudCmdBinding::DestroyRange(begin, end, begin, button);
    zUtil_StdPtrVector_Clear(&button->bindingVec);
    zUtil_StdPtrVector_FreeBufferAndReset(&button->bindingVec);
    ((HudUiPanel *)(&button->bindPanel))->Destructor();
    button->base.DestructorCore();
}

// Restores the mouse-button cleanup variant observed in 0x40adf0; no
// standalone helper exists in the retail executable.
static void HudCmdDialog_DestroyMouseButton(HudCmdBindButtonBase *button)
{
    button->base.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudCmdBindButtonBase_FTable);

    button->ClearBindingEntries();
    zUtil_StdPtrVector_FreeBufferAndReset(&button->bindingVec);
    ((HudUiPanel *)(&button->bindPanel))->Destructor();
    button->base.DestructorCore();
}

// Restores repeated inline binding-vector clearing observed in 0x40b680; no
// standalone helper exists in the retail executable.
static void HudCmdDialog_ClearBindButtonEntries(HudCmdBindButtonBase *button)
{
    button->ClearBindingEntries();
}

// Reimplements 0x40adf0: HudCmdDialog::Destructor
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL HudCmdDialog::Destructor()
{
    descriptionPanel.base.Destructor();
    ((HudUiPanel *)(&promptPanel))->Destructor();
    prevCommandButton.base.DestructorCore();
    nextCommandButton.base.DestructorCore();
    prevSetButton.base.DestructorCore();
    nextSetButton.base.DestructorCore();
    setList.base.DestructorCore();

    HudCmdDialog_DestroyMouseButton(&mouseButton.base);
    HudCmdDialog_DestroyBindButtonRange(&joyButton.base);
    HudCmdDialog_DestroyBindButtonRange(&keyBButton.base);
    HudCmdDialog_DestroyBindButtonRange(&keyAButton.base);
    HudCmdDialog_DestroyBindButtonRange(&commandList.base);

    resetButton.base.DestructorCore();
    resumeButton.base.DestructorCore();
    base.Destructor();
}

// Reimplements 0x40b5e0: HudCmdDialog::SelectGroupRelative
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int RECOIL_THISCALL HudCmdDialog::SelectGroupRelative(int delta)
{
    int groupIndex = setList.base.selectedIndex + delta;
    if (groupIndex >= setList.base.itemCount)
    {
        groupIndex = 0;
    }
    else if (groupIndex < 0)
    {
        groupIndex = setList.base.itemCount - 1;
    }

    setList.base.SetIndexClamped(groupIndex);
    const int selectedIndex = setList.base.selectedIndex;
    RebuildCommandBindingListsForGroup(selectedIndex);
    return selectedIndex;
}

// Reimplements 0x40b630: HudCmdDialog::SelectCommandRelative
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int RECOIL_THISCALL HudCmdDialog::SelectCommandRelative(int delta)
{
    int selectedIndex = delta;
    selectedIndex += commandList.base.selectedBindingIndex;
    if (selectedIndex >= 0)
    {
        HudCmdBindingEntry **const begin =
            (HudCmdBindingEntry **)(commandList.base.bindingVec.begin);
        int count;
        if (begin == 0)
        {
            count = 0;
        }
        else
        {
            count = (int)((HudCmdBindingEntry **)(commandList.base.bindingVec.end) - begin);
        }
        if (selectedIndex < count)
        {
            commandList.base.SetSelectedEntry(selectedIndex);
        }
    }

    const int currentIndex = commandList.base.selectedBindingIndex;
    OnCommandSelectionChanged(currentIndex);
    return currentIndex;
}

// Reimplements 0x40b930: HudCmdResetButton::OnActivate
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL HudCmdResetButton::OnActivate()
{
    HudCmdDialog *const dialog = (HudCmdDialog *)(base.owner);
    zInput::BindMap_InitDefaultBindings();
    zInput::BindMap_Current_RebuildLookupIndices();
    dialog->RebuildCommandBindingListsForGroup(dialog->setList.base.selectedIndex);
    base.OnActivate();
}

// Reimplements 0x40b960: HudCmdSetListWidget::OnActivate
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL HudCmdSetListWidget::OnActivate()
{
    base.AdvanceSelectionAndActivate();
    ((HudCmdDialog *)(base.base.owner))->RebuildCommandBindingListsForGroup(base.selectedIndex);
}

// Reimplements 0x40ba30: HudCmdKeyAButton::OnBeginCapture
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL HudCmdKeyAButton::OnBeginCapture()
{
    ((HudCmdDialog *)(base.base.base.owner))->descriptionPanel.captureState = 1;
    zInput::ResetAllTransitionState();
    base.base.OnActivate();
}

// Reimplements 0x40ba60: HudCmdKeyAButton::OnClearBinding
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL HudCmdKeyAButton::OnClearBinding()
{
    const int selectedIndex = base.selectedBindingIndex;
    ((HudCmdDialog *)(base.base.base.owner))->ApplyPrimaryKeyRebind(0, selectedIndex);
    base.SetSelectedEntry(selectedIndex);
}

// Reimplements 0x40ba90: HudCmdBindButtonBase::OnSelectionChangedRefresh
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL
HudCmdBindButtonBase::OnSelectionChangedRefresh(int selectedIndex)
{
    ((HudCmdDialog *)(base.base.owner))->OnCommandSelectionChanged(selectedIndex);
}

// Reimplements 0x40bab0: HudCmdKeyBButton::OnBeginCapture
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL HudCmdKeyBButton::OnBeginCapture()
{
    ((HudCmdDialog *)(base.base.base.owner))->descriptionPanel.captureState = 2;
    zInput::ResetAllTransitionState();
    base.base.OnActivate();
}

// Reimplements 0x40bae0: HudCmdKeyBButton::OnClearBinding
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL HudCmdKeyBButton::OnClearBinding()
{
    ((HudCmdDialog *)(base.base.base.owner))->ApplySecondaryKeyRebind(
        0, base.selectedBindingIndex);
}

// Reimplements 0x40b680: HudCmdDialog::RebuildCommandBindingListsForGroup
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL HudCmdDialog::RebuildCommandBindingListsForGroup(int groupIndex)
{
    HudCmdDialog_ClearBindButtonEntries(&commandList.base);
    HudCmdDialog_ClearBindButtonEntries(&keyAButton.base);
    HudCmdDialog_ClearBindButtonEntries(&keyBButton.base);
    HudCmdDialog_ClearBindButtonEntries(&joyButton.base);
    HudCmdDialog_ClearBindButtonEntries(&mouseButton.base);

    int commandIndex;
    for (commandIndex = 0;
         commandIndex < zInput::BindGroupList_GetGroupCommandCount(groupIndex);
         ++commandIndex)
    {
        const int commandId =
            zInput::BindGroupList_GetGroupCommandId(groupIndex, commandIndex);
        char labelBuffer[40];
        zInput::BindMapCurrent_CopyCommandLabel(commandId, labelBuffer,
                                                sizeof(labelBuffer));
        if (strlen(labelBuffer) != 0)
        {
            commandList.base.AddBindingEntry(zInput::BindMap_GetCommandLabel(commandId),
                                             commandId);
            keyAButton.base.AddBindingEntry(
                zInput::BindMapCurrent_FormatKeyComboName(
                    zInput::BindMapCurrent_GetPrimaryKeyboardKey(commandId),
                    labelBuffer, sizeof(labelBuffer)),
                commandId);
            keyBButton.base.AddBindingEntry(
                zInput::BindMapCurrent_FormatKeyComboName(
                    zInput::BindMapCurrent_GetSecondaryKeyboardKey(commandId),
                    labelBuffer, sizeof(labelBuffer)),
                commandId);
            joyButton.base.AddBindingEntry(
                zInput::BindMapCurrent_CopyJoystickButtonName(
                    zInput::BindMapCurrent_GetJoystickButtonSlot(commandId),
                    labelBuffer, sizeof(labelBuffer)),
                commandId);
            mouseButton.base.AddBindingEntry(
                zInput::BindMapCurrent_CopyMouseButtonName(
                    zInput::BindMapCurrent_GetMouseButtonSlot(commandId),
                    labelBuffer, sizeof(labelBuffer)),
                commandId);
        }
    }

    OnCommandSelectionChanged(0);
}

// Reimplements 0x40b980: HudCmdDialog::OnCommandSelectionChanged
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL HudCmdDialog::OnCommandSelectionChanged(int commandIndex)
{
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    commandList.base.SetSelectedEntry(commandIndex);
    keyAButton.base.SetSelectedEntry(commandIndex);
    keyBButton.base.SetSelectedEntry(commandIndex);
    joyButton.base.SetSelectedEntry(commandIndex);
    mouseButton.base.SetSelectedEntry(commandIndex);

    HudCmdBindingEntry **const entries =
        (HudCmdBindingEntry **)(commandList.base.bindingVec.begin);
    HudCmdBindingEntry *const selectedEntry =
        entries[commandList.base.selectedBindingIndex];
    char *const hint = zInput::BindMap_GetCommandHint(selectedEntry->commandId);
    if (hint != 0)
    {
        HudUiPanelVirtualSetTextFmtStringRequired(&descriptionPanel, hint);
    }
    else
    {
        HudUiPanelVirtualSetTextFmtEmptyRequired(&descriptionPanel);
    }
}

// Reimplements 0x40b140: HudCmdDialog::UpdateCaptureState
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
void RECOIL_THISCALL HudCmdDialog::UpdateCaptureState(float deltaTime)
{
    base.Update(deltaTime);

    switch (descriptionPanel.captureState)
    {
    case 0:
        HudUiVirtualSetVisible(&promptPanel, 0);
        --g_HudCmdMouseDebounceFrames;
        break;

    case 1:
    {
        HudUiVirtualSetVisible(&promptPanel, 1);
        HudUiPanelVirtualSetTextFmtRequired((HudUiPanel *)(&promptPanel), "Press desired keyboard key.");
        keyBButton.base.base.SetChecked(0);
        joyButton.base.base.SetChecked(0);
        mouseButton.base.base.SetChecked(0);

        const int keyCode = zInput::Keyboard_WaitForAnyKeyPress(0);
        if (keyCode != 0)
        {
            ApplyPrimaryKeyRebind(keyCode, keyAButton.base.selectedBindingIndex);
            keyAButton.base.base.SetChecked(0);
        }
        break;
    }

    case 2:
    {
        HudUiVirtualSetVisible(&promptPanel, 1);
        HudUiPanelVirtualSetTextFmtRequired((HudUiPanel *)(&promptPanel), "Press desired keyboard key.");
        keyAButton.base.base.SetChecked(0);
        joyButton.base.base.SetChecked(0);
        mouseButton.base.base.SetChecked(0);

        const int keyCode = zInput::Keyboard_WaitForAnyKeyPress(0);
        if (keyCode != 0)
        {
            ApplySecondaryKeyRebind(keyCode, keyBButton.base.selectedBindingIndex);
            keyBButton.base.base.SetChecked(0);
        }
        break;
    }

    case 3:
    {
        HudUiVirtualSetVisible(&promptPanel, 1);
        HudUiPanelVirtualSetTextFmtRequired((HudUiPanel *)(&promptPanel), "Press desired joystick button.");
        keyAButton.base.base.SetChecked(0);
        keyBButton.base.base.SetChecked(0);
        mouseButton.base.base.SetChecked(0);

        if (zInput::Keyboard_WaitForAnyKeyPress(0) == 1)
        {
            descriptionPanel.captureState = 0;
            zInput::ResetAllTransitionState();
            joyButton.base.base.SetChecked(0);
            return;
        }

        const int buttonCode = zInput::DI_WaitForButtonPress(0);
        if (buttonCode != 0)
        {
            ApplyJoystickButtonRebind(buttonCode, joyButton.base.selectedBindingIndex);
            joyButton.base.base.SetChecked(0);
        }
        break;
    }

    case 4:
    {
        HudUiVirtualSetVisible(&promptPanel, 1);
        HudUiPanelVirtualSetTextFmtRequired((HudUiPanel *)(&promptPanel), "Press desired mouse button.");
        keyAButton.base.base.SetChecked(0);
        keyBButton.base.base.SetChecked(0);
        joyButton.base.base.SetChecked(0);

        if (zInput::Keyboard_WaitForAnyKeyPress(0) == 1)
        {
            descriptionPanel.captureState = 0;
            zInput::ResetAllTransitionState();
            joyButton.base.base.SetChecked(0);
            return;
        }

        const int buttonCode = zInput::Mouse_WaitForButtonPress(0);
        if (buttonCode != 0)
        {
            ApplyMouseButtonRebind(buttonCode, mouseButton.base.selectedBindingIndex);
            mouseButton.base.base.SetChecked(0);
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
int RECOIL_THISCALL HudCmdDialog::ApplyPrimaryKeyRebind(int keyCode, int commandIndex)
{
    if (keyCode != 1)
    {
        const int primaryCommand =
            zInput::BindMapCurrent_GetCommandByPrimaryKey(keyCode);
        const int groupIndex = setList.base.selectedIndex;
        const int commandId =
            zInput::BindGroupList_GetGroupCommandId(groupIndex, commandIndex);
        if (primaryCommand == 0 &&
            zInput::BindMapCurrent_GetCommandBySecondaryKey(keyCode) != 0)
        {
            zInput::BindMapCurrent_SetSecondaryKeyBinding(keyCode, 0);
        }

        zInput::BindMapCurrent_SetPrimaryKeyBinding(keyCode, commandId);
        RebuildCommandBindingListsForGroup(groupIndex);
        OnCommandSelectionChanged(commandIndex);
    }

    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

// Reimplements 0x40b460: HudCmdDialog::ApplySecondaryKeyRebind
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int RECOIL_THISCALL HudCmdDialog::ApplySecondaryKeyRebind(int keyCode, int commandIndex)
{
    if (keyCode != 1)
    {
        const int secondaryCommand =
            zInput::BindMapCurrent_GetCommandBySecondaryKey(keyCode);
        const int groupIndex = setList.base.selectedIndex;
        const int commandId =
            zInput::BindGroupList_GetGroupCommandId(groupIndex, commandIndex);
        if (secondaryCommand == 0 &&
            zInput::BindMapCurrent_GetCommandByPrimaryKey(keyCode) != 0)
        {
            zInput::BindMapCurrent_SetPrimaryKeyBinding(keyCode, 0);
        }

        zInput::BindMapCurrent_SetSecondaryKeyBinding(keyCode, commandId);
        RebuildCommandBindingListsForGroup(groupIndex);
        OnCommandSelectionChanged(commandIndex);
    }

    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

// Reimplements 0x40b4e0: HudCmdDialog::ApplyJoystickButtonRebind
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int RECOIL_THISCALL HudCmdDialog::ApplyJoystickButtonRebind(int buttonCode, int commandIndex)
{
    const int joystickCommand =
        zInput::BindMapCurrent_GetCommandByJoystickSlot(buttonCode);
    const int groupIndex = setList.base.selectedIndex;
    const int commandId =
        zInput::BindGroupList_GetGroupCommandId(groupIndex, commandIndex);
    if (joystickCommand == 0 &&
        zInput::BindMapCurrent_GetCommandByJoystickSlot(buttonCode) != 0)
    {
        zInput::BindMapCurrent_SetJoystickBinding(buttonCode, 0);
    }

    zInput::BindMapCurrent_SetJoystickBinding(buttonCode, commandId);
    RebuildCommandBindingListsForGroup(groupIndex);
    OnCommandSelectionChanged(commandIndex);
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

// Reimplements 0x40b560: HudCmdDialog::ApplyMouseButtonRebind
// (D:\Proj\Battlesport\HudCmdDialog.cpp)
int RECOIL_THISCALL HudCmdDialog::ApplyMouseButtonRebind(int buttonCode, int commandIndex)
{
    const int mouseCommand =
        zInput::BindMapCurrent_GetCommandByMouseSlot(buttonCode);
    const int groupIndex = setList.base.selectedIndex;
    const int commandId =
        zInput::BindGroupList_GetGroupCommandId(groupIndex, commandIndex);
    if (mouseCommand == 0 &&
        zInput::BindMapCurrent_GetCommandByMouseSlot(buttonCode) != 0)
    {
        zInput::BindMapCurrent_SetMouseBinding(buttonCode, 0);
    }

    zInput::BindMapCurrent_SetMouseBinding(buttonCode, commandId);
    RebuildCommandBindingListsForGroup(groupIndex);
    OnCommandSelectionChanged(commandIndex);
    descriptionPanel.captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

// Reimplements 0x4b90e0: HudCmdBindButtonBase::RebuildBindingSlotWidgets
void RECOIL_THISCALL HudCmdBindButtonBase::RebuildBindingSlotWidgets(int totalCount,
                                                                     int visibleCount) {
    DeleteHudUiListSelectorItemArray(bindingSlotPanels);
    bindingSlotPanels = 0;

    const unsigned int allocationSize =
        sizeof(int) +
        (unsigned int)(totalCount) * sizeof(HudUiListSelectorItem);
    unsigned char *const header = (unsigned char *)(::operator new(allocationSize));
    FieldAt<int>(header, 0) = totalCount;
    HudUiListSelectorItem *const items = (HudUiListSelectorItem *)(header + sizeof(int));
    {
    for (int index = 0; index < totalCount; ++index) {
        items[index].Constructor();
    }
    }

    bindingSlotPanels = items;
    bindingSlotTotalCount = totalCount;
    visibleBindingSlotCount = visibleCount;

    {
    for (int index = 0; index < visibleBindingSlotCount; ++index) {
        const int x =
            (int)((float)(base.base.originX) + visibleListOffsetX);
        const int y = (int)(
            (float)(base.base.originY +
                               (index - visibleBindingSlotCount) * bindingSlotSpacing) +
            visibleListOffsetY);
        ((HudUiElement *)(&bindingSlotPanels[index]))->SetPos(x, y);
    }
    }

    ((HudUiElement *)(&bindPanel))->SetPos(base.base.originX, base.base.originY);

    {
    for (int index = visibleBindingSlotCount; index < bindingSlotTotalCount; ++index) {
        const int x =
            (int)((float)(base.base.originX) + overflowListOffsetX);
        const int y = (int)(
            (float)(base.base.originY +
                               (index - visibleBindingSlotCount + 1) * bindingSlotSpacing) +
            overflowListOffsetY);
        ((HudUiElement *)(&bindingSlotPanels[index]))->SetPos(x, y);
    }
    }
}

// Reimplements 0x4b8de0: HudCmdBindButtonBase::LoadFromZrd
int RECOIL_THISCALL HudCmdBindButtonBase::LoadFromZrd(zReader::Node *zrdSection,
                                                      void *ownerDialog) {
    base.LoadFromZrd(zrdSection, ownerDialog);

    void *const clipSource = OwnerField<void *>(ownerDialog, 0x118);

    zReader::Node *const selectedFontNode = zReader_GetNamedNode(zrdSection, "SELECTED_FONT");
    if (selectedFontNode != 0) {
        selectedFontStyleRef = selectedFontNode->value.i32;
        ApplyHudFontStyleTextOnly((HudUiPanel *)(&bindPanel),
                                  HudUiZrdOwnerFontStyle(base.base.owner, selectedFontStyleRef));
    }

    zReader::Node *const listFontNode = zReader_GetNamedNode(zrdSection, "LIST_FONT");
    if (listFontNode != 0) {
        listFontStyleRef = listFontNode->value.i32;
    }

    zReader::Node *const spacingNode = zReader_GetNamedNode(zrdSection, "SPACING");
    if (spacingNode != 0) {
        bindingSlotSpacing = spacingNode->value.i32;
    }

    zReader::Node *const listOffsetNode = zReader_GetNamedNode(zrdSection, "LIST_OFFSET");
    zReader::Node *const listOffsetBase = ZrdArrayBase(listOffsetNode);
    zReader::Node *const visibleOffsetBase = ZrdArrayBase(ZrdArrayItem(listOffsetBase, 1));
    zReader::Node *const overflowOffsetBase = ZrdArrayBase(ZrdArrayItem(listOffsetBase, 2));
    if (visibleOffsetBase != 0 && overflowOffsetBase != 0) {
        visibleListOffsetX = (float)(ZrdArrayInt(visibleOffsetBase, 1, 0));
        visibleListOffsetY = (float)(ZrdArrayInt(visibleOffsetBase, 2, 0));
        overflowListOffsetX = (float)(ZrdArrayInt(overflowOffsetBase, 1, 0));
        overflowListOffsetY = (float)(ZrdArrayInt(overflowOffsetBase, 2, 0));
    }

    zReader::Node *const listSizeNode = zReader_GetNamedNode(zrdSection, "LISTSIZE");
    zReader::Node *const listSizeBase = ZrdArrayBase(listSizeNode);
    if (listSizeBase != 0) {
        const int visibleCount =
            ZrdArrayCount(listSizeBase) > 2 ? ZrdArrayInt(listSizeBase, 2, 0) : 0;
        RebuildBindingSlotWidgets(ZrdArrayInt(listSizeBase, 1, 0), visibleCount);

        HudUiRect clipRect = {0};
        const HudFontStyle *const listStyle =
            HudUiZrdOwnerFontStyle(base.base.owner, listFontStyleRef);
        {
        for (int index = 0; index < bindingSlotTotalCount; ++index) {
            HudUiListSelectorItem *const item = &bindingSlotPanels[index];
            ((HudUiContainer *)(ownerDialog))->AddChild((HudUiElement *)(item));
            ((HudUiElement *)(item))->SetVisible(1);
            item->owner = this;
            if (clipSource != 0) {
                ((HudUiElement *)(item))->SetBltSourceAndClipRect(clipSource,
                                                                                &clipRect);
            }

            ApplyHudFontStyleTextOnly((HudUiPanel *)(item), listStyle);
        }
        }

        ((HudUiContainer *)(ownerDialog))->AddChild((HudUiElement *)(&bindPanel));
        ((HudUiElement *)(&bindPanel))->SetVisible(1);
        bindPanel.owner = this;
        if (clipSource != 0) {
            ((HudUiElement *)(&bindPanel))->SetBltSourceAndClipRect(clipSource, &clipRect);
        }
    }

    return 1;
}

int HudUiDialogSignedDivPow2(int value, int shift) {
    const int signMask = value >> 31;
    return (value + (signMask & ((1 << shift) - 1))) >> shift;
}

zVidImagePartial *HudUiMessageBoxCreateSolidImage(int width, int height,
                                                  unsigned short color565) {
    zVidImagePartial *const image = zVid_Image::Create();
    zVid_Image::SetFormatCode(image, 1);
    zVid_Image::SetSize(image, (short)(width), (short)(height));

    void *const pixels = malloc(zVid_Image::QueryBytesPerPixel(image) * width * height);
    zVid_Image_SetPixels(image, pixels, 0);

    unsigned short *const pixelWords = (unsigned short *)(pixels);
    for (int index = 0; index < image->pixelCount; ++index) {
        pixelWords[index] = color565;
    }

    return image;
}

// Reimplements 0x4bf060: HudUiMessageBoxDialog::Constructor
HudUiMessageBoxDialog *RECOIL_THISCALL
HudUiMessageBoxDialog::Constructor(const char *zrdPath, const char *sectionName) {
    HudUiBackground::Constructor();
    backdropWidget.Constructor(0);
    messagePanel.ConstructorDefault(0, 0, 0);
    titlePanel.ConstructorDefault(0, 0, 0);
    okButton.base.Constructor();
    okButton.base.base.ftable = (const HudUiWidget_FTable *)(&g_HudUiMessageBoxOkButton_Vtbl);
    cancelButton.base.Constructor();
    cancelButton.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiMessageBoxCancelButton_Vtbl);
    base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiMessageBoxDialog_FTable);

    const zVidRect32 *const primaryRect = zVideo::GetPrimarySurfaceRectScratch();
    blitRect = *primaryRect;

    if (zrdPath != 0 && sectionName != 0) {
        backgroundImage = 0;
        okButtonNormalImage = 0;
        okButtonPressedImage = 0;

        zReader::Node *const loadedSection = LoadFromZrd(zrdPath, sectionName, 0);
        if (loadedSection != 0) {
            BindWidgetByName(loadedSection, &okButton.base.base, "MB_OK");
            BindWidgetByName(loadedSection, &cancelButton.base.base, "MB_CANCEL");
            BindPrimitiveNodeToElement(loadedSection, (HudUiElement *)(&titlePanel), "TITLE");
            BindPrimitiveNodeToElement(loadedSection, (HudUiElement *)(&messagePanel), "MESSAGE");
        }

        FreeLoadedTreeRoots(0);
        ((HudUiElement *)(&titlePanel))->SetVisible(1);
        ((HudUiElement *)(&messagePanel))->SetVisible(1);
        return this;
    }

    const int centerX = HudUiDialogSignedDivPow2(blitRect.right, 1);
    const int centerY = HudUiDialogSignedDivPow2(blitRect.bottom, 1);
    fallbackWidth = 300;
    fallbackHeight = 200;

    backgroundImage = HudUiMessageBoxCreateSolidImage(
        fallbackWidth, fallbackHeight,
        (unsigned short)(zVid_PackColorRGB(128, 128, 128)));

    const int buttonWidth = HudUiDialogSignedDivPow2(fallbackWidth, 2);
    const int buttonHeight = HudUiDialogSignedDivPow2(fallbackHeight, 2);
    okButtonNormalImage = HudUiMessageBoxCreateSolidImage(
        buttonWidth, buttonHeight,
        (unsigned short)(zVid_PackColorRGB(192, 192, 192)));
    okButtonPressedImage = HudUiMessageBoxCreateSolidImage(
        buttonWidth, buttonHeight,
        (unsigned short)(zVid_PackColorRGB(160, 192, 160)));

    backdropWidget.SetImageBorrowedAndInvalidate(backgroundImage);
    messagePanel.SetTextFmt("");
    titlePanel.SetTextFmt("");
    okButton.base.LoadFromZrd(0, this);
    okButton.base.defaultImage =
        okButton.base.base.SetImageBorrowedAndInvalidate(okButtonNormalImage);
    okButton.base.rolloverImage = okButtonPressedImage;

    ((HudUiElement *)(&backdropWidget))->SetPos(centerX - 150, centerY - 100);
    ((HudUiElement *)(&titlePanel))->SetPos(centerX - 140, centerY - 90);
    ((HudUiElement *)(&messagePanel))->SetPos(centerX - 140, centerY - 70);
    ((HudUiElement *)(&okButton.base))
        ->SetPos(centerX - 150 + HudUiDialogSignedDivPow2(fallbackWidth, 1) -
                     HudUiDialogSignedDivPow2(fallbackWidth, 3),
                 centerY - 100 - HudUiDialogSignedDivPow2(fallbackHeight, 2) +
                     fallbackHeight - 10);

    ((HudUiContainer *)(this))->AddChild((HudUiElement *)(&backdropWidget));
    ((HudUiContainer *)(this))->AddChild((HudUiElement *)(&messagePanel));
    ((HudUiContainer *)(this))->AddChild((HudUiElement *)(&titlePanel));
    ((HudUiContainer *)(this))->AddChild((HudUiElement *)(&okButton.base));
    ((HudUiElement *)(&messagePanel))->SetVisible(1);
    ((HudUiElement *)(&titlePanel))->SetVisible(1);
    ((HudUiElement *)(&okButton.base))->SetVisible(0);
    ((HudUiContainer *)(this))->SetChildFlags(0);
    return this;
}

// Reimplements 0x4bf540: HudUiMessageBoxDialog::ScalarDeletingDestructor
HudUiMessageBoxDialog *
RECOIL_THISCALL HudUiMessageBoxDialog::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4bf560: HudUiMessageBoxDialog::Destructor
void RECOIL_THISCALL HudUiMessageBoxDialog::Destructor() {
    base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiMessageBoxDialog_FTable);

    if (backgroundImage != 0) {
        if (backgroundImage->pixels != 0) {
            free(backgroundImage->pixels);
            backgroundImage->pixels = 0;
        }

        zVid_Image::Destroy(backgroundImage);
        backgroundImage = 0;
    }

    cancelButton.base.DestructorCore();
    okButton.base.DestructorCore();
    titlePanel.Destructor();
    messagePanel.Destructor();
    backdropWidget.DestructorCore();
    HudUiBackground::Destructor();
}

// Reimplements 0x4bf630: HudUiMessageBoxDialog::RunModal
int RECOIL_THISCALL HudUiMessageBoxDialog::RunModal(const char *messageText,
                                                    const char *titleText,
                                                    void *modalContext,
                                                    float timeoutSeconds) {
    (void)modalContext;
    (void)timeoutSeconds;

    if (g_zVideo_ActiveRendererPath != 0) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(0, 0);
    }

    const int previousHalfResMode =
        zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    zVidRect32 previousRegionRect = {0, 0, 0, 0};
    int previousBitsPerPixel = 0;
    int previousPitchBytes = 0;
    void *const previousPixels =
        zRndr::GetActiveRegionState(&previousRegionRect.right, &previousRegionRect.bottom,
                                    &previousBitsPerPixel, &previousPitchBytes);

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

    zRndr::SetFrameBufferRegion(dialogPixels, (zOpt_ViewRectSection *)(&blitRect),
                                dialogBitsPerPixel, dialogPitchBytes);

    const unsigned int *const dialogDispatch = (const unsigned int *)(base.base.vptr);
    typedef void (RECOIL_THISCALL *DialogSetEnabledFn)(HudUiMessageBoxDialog * self,
                                                       int enabled);
    typedef void (RECOIL_THISCALL *DialogUpdateFn)(HudUiMessageBoxDialog * self,
                                                   float deltaSeconds);

    modalResult = 0;
    modalFrameCountdown = 100000;
    ((DialogSetEnabledFn)(dialogDispatch[1]))(this, 1);
    messagePanel.SetTextFmt(messageText);
    titlePanel.SetTextFmt(titleText);
    ((HudUiElement *)(&okButton.base))->SetVisible(1);

    int framesRemaining = modalFrameCountdown;
    modalFrameCountdown = framesRemaining - 1;
    while (framesRemaining > 0) {
        zInput::PollActiveDevices(0);
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();
        ((DialogUpdateFn)(dialogDispatch[0]))(this, g_FrameDeltaTimeSec);
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(&blitRect, &blitRect, 1, 1);
        framesRemaining = modalFrameCountdown;
        modalFrameCountdown = framesRemaining - 1;
    }

    ((HudUiDialogController *)(this))->BlitOwnedSurfaceToPrimary();
    ((DialogSetEnabledFn)(dialogDispatch[1]))(this, 0);
    zVideo::SetHalfResAdjustMode(previousHalfResMode);
    HudUi::SetInvalidateMode(previousHalfResMode);
    zRndr::SetFrameBufferRegion(previousPixels, (zOpt_ViewRectSection *)(&previousRegionRect),
                                previousBitsPerPixel, previousPitchBytes);
    return modalResult;
}

// Reimplements 0x4bf7c0: HudUiMessageBoxDialog::OnOk
void RECOIL_THISCALL HudUiMessageBoxDialog::OnOk() {
    modalResult = 1;
    modalFrameCountdown = 0;
}

// Reimplements 0x4bf7e0: HudUiMessageBoxDialog::OnCancel
void RECOIL_THISCALL HudUiMessageBoxDialog::OnCancel() {
    modalResult = 2;
    modalFrameCountdown = 0;
}

// Reimplements 0x4bf800: HudUiMessageBoxOkButton::OnActivate
void RECOIL_THISCALL HudUiMessageBoxOkButton::OnActivate() {
    HudUiMessageBoxDialog *const dialog = (HudUiMessageBoxDialog *)(base.owner);
    const HudUiMessageBoxDialog_FTable *const ftable = *(const HudUiMessageBoxDialog_FTable *const *)(dialog);
    typedef void (RECOIL_THISCALL *OnOkFn)(HudUiMessageBoxDialog *);
    ((OnOkFn)(ftable->slots[3]))(dialog);

    base.OnActivate();
}

// Reimplements 0x4bf820: HudUiMessageBoxCancelButton::OnActivate
void RECOIL_THISCALL HudUiMessageBoxCancelButton::OnActivate() {
    HudUiMessageBoxDialog *const dialog = (HudUiMessageBoxDialog *)(base.owner);
    const HudUiMessageBoxDialog_FTable *const ftable = *(const HudUiMessageBoxDialog_FTable *const *)(dialog);
    typedef void (RECOIL_THISCALL *OnCancelFn)(HudUiMessageBoxDialog *);
    ((OnCancelFn)(ftable->slots[4]))(dialog);

    base.OnActivate();
}

// Reimplements 0x40f2d0: HudUiWidget::CtorDefaultThunk
HudUiWidget *RECOIL_THISCALL HudUiWidget::CtorDefaultThunk() {
    return Constructor(0);
}

// Reimplements 0x404d90: HudUiWidget::GetCenterX
int RECOIL_THISCALL HudUiWidget::GetCenterX() {
    if (alignFlags != 0) {
        const int width = image != 0 ? image->width : 0;
        return x + (width / 2);
    }

    return x;
}

// Reimplements 0x404dd0: HudUiWidget::GetCenterY
int RECOIL_THISCALL HudUiWidget::GetCenterY() {
    if (alignFlags != 0) {
        const int height = image != 0 ? image->height : 0;
        return y + (height / 2);
    }

    return y;
}

RECOIL_NO_GS void RECOIL_THISCALL HudUiWidget::RebuildBltRectFromImage() {
    typedef void (RECOIL_THISCALL *SetClipRectFn)(HudUiWidget * self, const HudUiRect *rect);

    const int width = image != 0 ? image->width : 0;
    const int height = image != 0 ? image->height : 0;
    const HudUiRect rect = {x, y, x + width, y + height};
    ((SetClipRectFn)(ftable->slots[7]))(this, &rect);
}

// Reimplements 0x4b3fb0: HudUiWidget::Draw
void RECOIL_THISCALL HudUiWidget::Draw() {
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

            zVid_Image::BlitToActiveTarget(image, dirtyRect.drawX, dirtyRect.drawY, 0,
                                           (zVidRect32 *)(&dirtyRect.srcLeft));

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

    typedef void (RECOIL_THISCALL *DrawBaseFn)(HudUiWidget * self);
    ((DrawBaseFn)(ftable->slots[2]))(this);

    zVid_Image::BlitToActiveTarget(image, x, y, 0,
                                   (zVidRect32 *)(bltClipRectOrNull));
}

// Reimplements 0x4b3da0: HudUiWidget::ReleaseImageIfOwned
RECOIL_NOINLINE void RECOIL_THISCALL HudUiWidget::ReleaseImageIfOwned() {
    if (image != 0 && ownsImage != 0) {
        zVid_Image::ReleaseIfNotDefault(image);
        image = 0;
    }

    ownsImage = 0;
}

// Reimplements 0x4b3e70: HudUiWidget::SetImageBorrowedAndInvalidate
RECOIL_NOINLINE zVidImagePartial *RECOIL_THISCALL
HudUiWidget::SetImageBorrowedAndInvalidate(zVidImagePartial *newImage) {
    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiWidget * self);

    ownsImage = 0;
    image = newImage;
    ((InvalidateFn)(ftable->slots[8]))(this);
    return newImage;
}

// Reimplements 0x4b3e30: HudUiWidget::SetImageByPathOwned
RECOIL_NOINLINE zVidImagePartial *RECOIL_THISCALL
HudUiWidget::SetImageByPathOwned(const char *imagePath) {
    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiWidget * self);

    if (imagePath == 0) {
        return 0;
    }

    ReleaseImageIfOwned();
    image = zImage::TexDir_FindOrCreateByPath(imagePath);
    if (image != 0) {
        ownsImage = 1;
    }

    ((InvalidateFn)(ftable->slots[8]))(this);
    return image;
}

// Reimplements 0x4b3d50: HudUiWidget::DestructorCore
RECOIL_NOINLINE void RECOIL_THISCALL HudUiWidget::DestructorCore() {
    ReleaseImageIfOwned();
    ftable = (const HudUiWidget_FTable *)(&g_HudUiCommon_FTable);
}

// Reimplements 0x4b3ce0: HudUiWidget::ScalarDeletingDestructor
HudUiWidget *RECOIL_THISCALL HudUiWidget::ScalarDeletingDestructor(unsigned int flags) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40f200: HudUiTripletPanel::Constructor
HudUiTripletPanel *RECOIL_THISCALL HudUiTripletPanel::Constructor() {
    base.Constructor(0, 0);
    base.ftable = (const HudUiCommon_FTable *)(&g_HudUiTripletPanel_FTable);
    visibleCount = 0;

    {
        int itemIndex;
        for (itemIndex = 0; itemIndex < (int)(sizeof(items) / sizeof(items[0])); ++itemIndex) {
            HudUiWidget &item = items[itemIndex];
            item.CtorDefaultThunk();
            ((HudUiElement *)(&item))->SetVisible(0);
        }
    }

    g_HudUiMgr.AddChild((HudUiElement *)(this));
    return this;
}

// Reimplements 0x40f2b0: HudUiTripletPanel::ScalarDeletingDestructor
HudUiTripletPanel *RECOIL_THISCALL
HudUiTripletPanel::ScalarDeletingDestructor(unsigned int flags) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40f400: HudUiTripletPanel::Draw
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_THISCALL HudUiTripletPanel::Draw() {
    typedef void (RECOIL_THISCALL *DrawBaseFn)(HudUiTripletPanel * self);
    typedef void (RECOIL_THISCALL *DrawFn)(HudUiWidget * self);

    ((DrawBaseFn)(base.ftable->slots[2]))(this);

    {
    for (int index = 2; index >= 0; --index) {
        HudUiWidget &item = items[index];
        if (((unsigned char)(item.flags) & 0x10u) == 0) {
            ((DrawFn)(item.ftable->slots[1]))(&item);
        }
    }
    }
}

// Reimplements 0x40f460: HudUiTripletPanel::SetVisibleCount
RECOIL_NOINLINE void RECOIL_THISCALL HudUiTripletPanel::SetVisibleCount(int count) {
    typedef void (RECOIL_THISCALL *SetVisibleFn)(HudUiElement * self, int visible);
    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiElement * self);

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
        HudUiElement *const item = (HudUiElement *)(&items[index]);
        ((SetVisibleFn)(item->ftable->slots[24]))(item, count > index ? 1 : 0);
    }
    }

    ((InvalidateFn)(base.ftable->slots[8]))(&base);
}

// Reimplements 0x40f2e0: HudUiNanitePanel::InitLayout
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiNanitePanel::InitLayout(zReader::Node *layoutRoot) {
    typedef int (RECOIL_THISCALL *GetCenterFn)(HudUiWidget * self);
    typedef void (RECOIL_THISCALL *SetPosFn)(HudUiNanitePanel * self, int x, int y);
    typedef void (RECOIL_THISCALL *SetBltSourceAndClipRectFn)(HudUiNanitePanel * self, void *bltSourceOrNull,
                                const HudUiRect *clipRect);

    HudUiWidget *const layoutWidget2 = (HudUiWidget *)((unsigned char *)(&g_HudLayoutHW) + 0x1b4);
    const int baseX = g_HudUiMgrHudOriginX / 2;
    const HudUiWidget_FTable *const layoutWidget2FTable = layoutWidget2->ftable;
    int anchor[2];
    anchor[0] =
        ((GetCenterFn)(layoutWidget2FTable->slots[0x64 / 4]))(layoutWidget2);
    anchor[1] =
        ((GetCenterFn)(layoutWidget2FTable->slots[0x68 / 4]))(layoutWidget2);

    zReader::Node *const layoutPayload = layoutRoot->value.nodes;
    HudUiRect clipRect;
    HudUiLayoutNode::ReadRectOffsetAndSize(&layoutPayload[1], &clipRect, 0, 0,
                                           0);

    zVidImagePartial *const sharedImage =
        HudUiLayoutNode::ApplyImageWidget(&layoutPayload[2], &items[0], baseX, 0, anchor, 0,
                                          0);
    HudUiLayoutNode::ApplyImageWidget(&layoutPayload[3], &items[1], baseX, 0, anchor, sharedImage,
                                      0);
    HudUiLayoutNode::ApplyImageWidget(&layoutPayload[4], &items[2], baseX, 0, anchor, sharedImage,
                                      0);

    HudUiWidget *const anchorItem = &items[2];
    const HudUiWidget_FTable *const anchorFTable = anchorItem->ftable;
    const int y = ((GetCenterFn)(anchorFTable->slots[0x68 / 4]))(
        anchorItem);
    const int x = ((GetCenterFn)(anchorFTable->slots[0x64 / 4]))(
        anchorItem);
    ((SetPosFn)(base.ftable->slots[3]))(this, x, y);

    clipRect.left += baseX;
    clipRect.right += baseX;
    ((SetBltSourceAndClipRectFn)(base.ftable->slots[6]))(this, 0, &clipRect);
}

// Reimplements 0x40f3e0: HudUiTripletPanel::ShutdownItems_Stub
void RECOIL_THISCALL HudUiTripletPanel::ShutdownItems_Stub() {
    HudUiNoOpMethodStub(&g_HudUiMgrNanitePanel.items[0]);
    HudUiNoOpMethodStub(&g_HudUiMgrNanitePanel.items[1]);
    HudUiNoOpMethodStub(&g_HudUiMgrNanitePanel.items[2]);
}

// Reimplements 0x40d600: HudUiTripletPanel::UnwindDestructFirstItem
void RECOIL_THISCALL HudUiTripletPanel::UnwindDestructFirstItem() {
    items[0].DestructorCore();
}

// Reimplements 0x40d610: HudUiTripletPanel::DestructorCore
RECOIL_NOINLINE void RECOIL_THISCALL HudUiTripletPanel::DestructorCore() {
    {
    for (int index = 2; index >= 0; --index) {
        items[index].DestructorCore();
    }
    }

    base.ftable = &g_HudUiCommon_FTable;
}

// Reimplements 0x40e910: HudUiTriplet::InterpolateLayout
// (D:\Proj\Battlesport\HudUiTriplet.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiTriplet::InterpolateLayout(float t) {
    baseX =
        (int)((float)(baseXEnd - baseXStart) * t + baseXStart);
    baseY =
        (int)((float)(baseYEnd - baseYStart) * t + baseYStart);
    rowPitchY = (int)(
        (float)(rowPitchYEnd - rowPitchYStart) * t + rowPitchYStart);
    lapsColumnOffsetX = (int)(
        (float)(lapsColumnOffsetXEnd - lapsColumnOffsetXStart) * t +
        lapsColumnOffsetXStart);
    killsColumnOffsetX = (int)(
        (float)(killsColumnOffsetXEnd - killsColumnOffsetXStart) * t +
        killsColumnOffsetXStart);
    fontSize = (int)(
        (float)(fontSizeEnd - fontSizeStart) * t + fontSizeStart);
    fontWeight = (int)(
        (float)(fontWeightEnd - fontWeightStart) * t + fontWeightStart);
}

// Reimplements 0x4b4370: HudUiTextInput::DestructorCore
void RECOIL_THISCALL HudUiTextInput::DestructorCore() {
    char *const ownedBuffer = buffer;
    ftable = &g_HudUiTextInput_FTable;
    ::operator delete(ownedBuffer);
}

// Reimplements 0x4b4390: HudUiTextInput::AllocTextBuffer
void RECOIL_THISCALL HudUiTextInput::AllocTextBuffer(unsigned int bufferSize) {
    char *const newBuffer = (char *)(::operator new(bufferSize));
    char *const oldBuffer = buffer;
    if (oldBuffer != 0) {
        unsigned int copyCount = capacity;
        if (bufferSize < copyCount) {
            copyCount = bufferSize;
        }

        strncpy(newBuffer, oldBuffer, copyCount);
    }

    capacity = bufferSize;
    buffer = newBuffer;
}

// Reimplements 0x4b42f0: HudUiTextInput::Constructor
HudUiTextInput *RECOIL_THISCALL HudUiTextInput::Constructor(unsigned int bufferSize) {
    ftable = &g_HudUiTextInput_FTable;
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
void RECOIL_THISCALL HudUiTextInput::SetCursorPosition(int position) {
    const int textLength = (int)(strlen(buffer));
    cursor = position < textLength ? (unsigned int)(position)
                                   : (unsigned int)(textLength);
}

// Reimplements 0x4b43d0: HudUiTextInput::SetContents
void RECOIL_THISCALL HudUiTextInput::SetContents(const char *source) {
    strncpy(buffer, source, capacity);
    buffer[capacity - 1] = '\0';
    SetCursorPosition((int)(cursor));
}

// Reimplements 0x4b4410: HudUiTextInput::GetBuffer
RECOIL_NOINLINE char *RECOIL_THISCALL HudUiTextInput::GetBuffer() {
    return buffer;
}

// Reimplements 0x4b4590: HudUiTextInput::ShiftTextRight
int RECOIL_THISCALL HudUiTextInput::ShiftTextRight(int count,
                                                            int startPos) {
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
int RECOIL_THISCALL HudUiTextInput::ShiftTextLeft(int count,
                                                           int startPos) {
    const int textLength = (int)(strlen(buffer));
    {
    for (int index = startPos; index < textLength; ++index) {
        buffer[index] = buffer[index + count];
    }
    }

    return 1;
}

// Reimplements 0x4b4550: HudUiTextInput::DeleteCharForward
void RECOIL_THISCALL HudUiTextInput::DeleteCharForward() {
    ShiftTextLeft(1, (int)(cursor));
}

// Reimplements 0x4b4560: HudUiTextInput::MoveCursorLeft
void RECOIL_THISCALL HudUiTextInput::MoveCursorLeft() {
    if ((int)(cursor) > 0) {
        --cursor;
    }
}

// Reimplements 0x4b4570: HudUiTextInput::MoveCursorRight
void RECOIL_THISCALL HudUiTextInput::MoveCursorRight() {
    const int textLength = (int)(strlen(buffer));
    if ((int)(cursor) < textLength) {
        ++cursor;
    }
}

// Reimplements 0x4b4530: HudUiTextInput::BackspaceDeleteChar
void RECOIL_THISCALL HudUiTextInput::BackspaceDeleteChar() {
    if ((int)(cursor) > 0) {
        --cursor;
        ShiftTextLeft(1, (int)(cursor));
    }
}

// Reimplements 0x4b44e0: HudUiTextInput::InsertCharAtCursor
void RECOIL_THISCALL HudUiTextInput::InsertCharAtCursor(int ch) {
    const int textLength = (int)(strlen(buffer));
    if (textLength >= (int)(capacity) - 1) {
        typedef void (RECOIL_THISCALL *NoArgFn)(HudUiTextInput * self);
        ((NoArgFn)(ftable->slots[8]))(this);
        return;
    }

    ShiftTextRight(1, (int)(cursor));
    buffer[cursor] = (char)(ch);
    ++cursor;
}

// Reimplements 0x4b4460: HudUiTextInput::DispatchKeyAction
RECOIL_NOINLINE void RECOIL_THISCALL HudUiTextInput::DispatchKeyAction(int key) {
    const int keyIndex = (signed char)(key);
    const int action = (signed char)(keyActionMap[keyIndex]);

    typedef void (RECOIL_THISCALL *KeyFn)(HudUiTextInput * self, int key);
    typedef void (RECOIL_THISCALL *NoArgFn)(HudUiTextInput * self);

    switch (action) {
    case 0:
        ((KeyFn)(ftable->slots[0]))(this, key);
        break;
    case 1:
        ((KeyFn)(ftable->slots[1]))(this, key);
        break;
    case 2:
        ((NoArgFn)(ftable->slots[3]))(this);
        break;
    case 3:
        ((NoArgFn)(ftable->slots[2]))(this);
        break;
    case 4:
        ((NoArgFn)(ftable->slots[4]))(this);
        break;
    case 5:
        ((NoArgFn)(ftable->slots[5]))(this);
        break;
    case 6:
        ((NoArgFn)(ftable->slots[6]))(this);
        break;
    case 7:
        ((NoArgFn)(ftable->slots[7]))(this);
        break;
    default:
        break;
    }
}

// Reimplements 0x4ba3e0: HudUiOwnedTextInput::OnAcceptNotifyOwner
void RECOIL_THISCALL HudUiOwnedTextInput::OnAcceptNotifyOwner() {
    typedef void (RECOIL_THISCALL *AcceptFn)(void *self);

    zGame::ReturnOnlyStub();

    const unsigned int *const ownerFTable = *(const unsigned int *const *)(owner);
    ((AcceptFn)(ownerFTable[34]))(owner);
}

// Reimplements 0x40d660: HudUiMgrObjectiveBlock::Destructor
void RECOIL_THISCALL HudUiMgrObjectiveBlock::Destructor() {
    chatComposeTextInput.DestructorCore();
    objectiveBar.ftable = (const HudUiBar_FTable *)(&g_HudUiCommon_FTable);
    objectiveMeter.ftable = (const HudUiMeter_FTable *)(&g_HudUiCommon_FTable);
    objectiveSensorRect.DestructorCore();
    objectiveWidget.DestructorCore();
}

// Reimplements 0x40d780: HudUiSlot::Destructor
void RECOIL_THISCALL HudUiSlot::Destructor() {
    trackMarkerWidget.DestructorCore();
    slotWidget.DestructorCore();
    ftable = (const HudUiSlot_FTable *)(&g_HudUiCommon_FTable);
}

// Reimplements 0x40db20: HudUiSlot::Constructor
HudUiSlot *RECOIL_THISCALL HudUiSlot::Constructor() {
    ((HudUiElement *)(this))->Constructor(0, 0);
    slotWidget.Constructor(0);
    trackMarkerWidget.Constructor(0);
    ftable = &g_HudUiSlot_FTable;
    return this;
}

// Reimplements 0x40db90: HudUiSlot::Draw
void RECOIL_THISCALL HudUiSlot::Draw() {
    typedef void (RECOIL_FASTCALL *DispatchFn)(HudUiWidget * self);

    if ((slotWidget.flags & 0x10) == 0) {
        ((DispatchFn)(slotWidget.ftable->slots[1]))(&slotWidget);
    }

    if ((trackMarkerWidget.flags & 0x10) == 0) {
        ((DispatchFn)(trackMarkerWidget.ftable->slots[1]))(&trackMarkerWidget);
    }
}

// Reimplements 0x40dbd0: HudUiSlot::ScalarDeletingDestructor
HudUiSlot *RECOIL_THISCALL HudUiSlot::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40fa10: HudUiStatsListElement::Update
void RECOIL_THISCALL HudUiStatsListElement::Update(float deltaSeconds) {
    (triplet->base.*(triplet->base.vptr->updateAll))(deltaSeconds);
}

// Reimplements 0x40fa40: HudUiStatsListElement::DestructorCore
void RECOIL_THISCALL HudUiStatsListElement::DestructorCore() {
    base.ftable = (const HudUiCommon_FTable *)(&g_HudUiStatsListElement_FTable);

    HudUiTriplet *const ownedTriplet = triplet;
    if (ownedTriplet != 0) {
        ownedTriplet->DestructorCore();
        ::operator delete(ownedTriplet);
    }

    triplet = 0;
    base.ftable = &g_HudUiCommon_FTable;
}

// Reimplements 0x40fa20: HudUiStatsListElement::ScalarDeletingDestructor
HudUiStatsListElement *RECOIL_THISCALL
HudUiStatsListElement::ScalarDeletingDestructor(unsigned int flags) {
    DestructorCore();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40fdd0: HudUiStringMenu::DestructorCore
void RECOIL_THISCALL HudUiStringMenu::DestructorCore() {
    {
        int itemIndex;
        for (itemIndex = 0; itemIndex < 23; ++itemIndex) {
            ((HudUiPanel *)(&items[itemIndex]))->Destructor();
        }
    }

    base.DestructorCore();
}

// Reimplements 0x40dac0: HudUiCounter::Constructor
HudUiCounter *RECOIL_THISCALL HudUiCounter::Constructor() {
    ((HudUiWidget *)(this))->Constructor(0);
    FieldAt<const void *>(this, 0x00) = &g_HudUiCounter_FTable;
    FieldAt<int>(this, 0xc4) = 0;
    FieldAt<int>(this, 0xc0) = 0;
    FieldAt<int>(this, 0xbc) = 0;
    return this;
}

// Reimplements 0x40f0f0: HudUiCounter::ReleaseStateImages
void RECOIL_THISCALL HudUiCounter::ReleaseStateImages() {
    zVid_Image::ReleaseIfNotDefault(FieldAt<zVidImagePartial *>(this, 0xbc));
    zVid_Image::ReleaseIfNotDefault(FieldAt<zVidImagePartial *>(this, 0xc0));
    zVid_Image::ReleaseIfNotDefault(FieldAt<zVidImagePartial *>(this, 0xc4));

    FieldAt<zVidImagePartial *>(this, 0xc4) = 0;
    FieldAt<zVidImagePartial *>(this, 0xc0) = 0;
    FieldAt<zVidImagePartial *>(this, 0xbc) = 0;
}

// Reimplements 0x40f070: HudUiCounter::ApplyFromLayoutNode
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
HudUiCounter::ApplyFromLayoutNode(zReader::Node *layoutNode) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    FieldAt<zVidImagePartial *>(this, 0xbc) =
        zImage::TexDir_FindOrCreateByPath(payload[1].value.str);
    FieldAt<zVidImagePartial *>(this, 0xc0) =
        zImage::TexDir_FindOrCreateByPath(payload[2].value.str);
    FieldAt<zVidImagePartial *>(this, 0xc4) =
        zImage::TexDir_FindOrCreateByPath(payload[3].value.str);
    FieldAt<int>(this, 0xd8) = payload[4].value.i32;
    FieldAt<int>(this, 0xdc) = payload[5].value.i32;

    UpdateLayoutPosition();
    ((HudUiWidget *)(this))->SetImageBorrowedAndInvalidate(
        FieldAt<zVidImagePartial *>(this, 0xbc));
    g_HudUiMgr.AddChild((HudUiElement *)(this));
    return 1;
}

// Reimplements 0x40f130: HudUiCounter::UpdateLayoutPosition
void RECOIL_THISCALL HudUiCounter::UpdateLayoutPosition() {
    typedef void (RECOIL_THISCALL *SetPosFn)(HudUiCounter * self, int x, int y);

    const int localX = FieldAt<int>(this, 0xd8);
    const int localY = FieldAt<int>(this, 0xdc);
    const HudUiCounter_FTable *const ftable = FieldAt<const HudUiCounter_FTable *>(this, 0x00);
    ((SetPosFn)(ftable->slots[3]))(this, g_HudUiMgrHudOriginX + localX,
                                                 g_HudUiMgrHudOriginY + localY);

    zVidImagePartial *const image = FieldAt<zVidImagePartial *>(this, 0xbc);
    FieldAt<int>(this, 0xc8) = localX;
    FieldAt<int>(this, 0xcc) = localY;
    FieldAt<int>(this, 0xd0) = localX + image->width;
    FieldAt<int>(this, 0xd4) = localY + image->height;
}

// Reimplements 0x414070: HudUiMessage::RebuildWeaponLayout
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiMessage::RebuildWeaponLayout() {
    typedef int (RECOIL_THISCALL *GetCoordFn)(void *self);
    typedef void (RECOIL_THISCALL *SetPosFn)(void *self, int x, int y);
    typedef void (RECOIL_THISCALL *SetClipFn)(void *self, void *bltSourceOrNull,
                                             const HudUiRect *rect);

    HudUiWidget *const layoutWidget2 = (HudUiWidget *)((unsigned char *)(&g_HudLayoutHW) + 0x1b4);
    const HudUiWidget_FTable *const layoutWidget2FTable = layoutWidget2->ftable;
    const int anchorX =
        ((GetCoordFn)(layoutWidget2FTable->slots[0x64 / 4]))(layoutWidget2);
    const int anchorY =
        ((GetCoordFn)(layoutWidget2FTable->slots[0x68 / 4]))(layoutWidget2);

    const int clipLeft = panel.layoutX + (g_HudUiMgrHudOriginX / 2);
    zVidImagePartial *const baseImage = variantImages[0];
    HudUiRect widgetClipRect;
    widgetClipRect.left = clipLeft;
    widgetClipRect.top = panel.layoutY;
    widgetClipRect.right = clipLeft + baseImage->width;
    widgetClipRect.bottom = panel.layoutY + baseImage->height;

    const HudUiWidget_FTable *const baseFTable = base.ftable;
    ((SetPosFn)(baseFTable->slots[0x0c / 4]))(
        this, clipLeft + anchorX, panel.layoutY + anchorY);
    ((SetClipFn)(baseFTable->slots[0x18 / 4]))(this, 0, &widgetClipRect);

    HudUiRect panelClipRect;
    panelClipRect.left = clipLeft + 3;
    panelClipRect.top = widgetClipRect.bottom;
    panelClipRect.right = widgetClipRect.right - 2;
    panelClipRect.bottom = widgetClipRect.bottom + 12;

    HudUiPanel *const textPanel = (HudUiPanel *)(&panel);
    const HudUiPanel_FTable *const panelFTable = *(const HudUiPanel_FTable *const *)(textPanel);
    const int textX =
        panelClipRect.left + ((panelClipRect.right - panelClipRect.left) / 2) + anchorX;
    ((SetPosFn)(panelFTable->slots[0x0c / 4]))(
        textPanel, textX, widgetClipRect.bottom + anchorY);
    ((SetClipFn)(panelFTable->slots[0x18 / 4]))(textPanel, 0,
                                                              &panelClipRect);

    zVidImagePartial *const sideImage = sideImageSwaps[0];
    const HudUiWidget_FTable *const widgetFTable = widget.ftable;
    ((SetPosFn)(widgetFTable->slots[0x0c / 4]))(
        &widget, anchorX - sideImage->width + widgetClipRect.right - 1,
        anchorY - sideImage->height + widgetClipRect.bottom - 1);
}

// Reimplements 0x413ec0: HudUiMessage::LoadWeaponLayoutFromNode
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
HudUiMessage::LoadWeaponLayoutFromNode(zReader::Node *layoutNode,
                                       const HudUiPanelFontParams *fontParams) {
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

    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiMessage * self);
    const HudUiWidget_FTable *const baseFTable = base.ftable;
    base.imageStateWord = (base.imageStateWord & 0xffff0000u) | 1u;
    ((InvalidateFn)(baseFTable->slots[0x20 / 4]))(this);

    HudUiPanel *const textPanel = (HudUiPanel *)(&panel);
    FieldAt<unsigned int>(textPanel, 0x144) = 1;
    FieldAt<unsigned int>(textPanel, 0x14c) = 0x0020bf40;
    FieldAt<unsigned int>(textPanel, 0x150) = 0x0020bf40;
    FieldAt<unsigned int>(textPanel, 0x270) = 1;
    FieldAt<int>(textPanel, 0x29c) = -1;
    FieldAt<int>(textPanel, 0x2a0) = -1;
    FieldAt<unsigned int>(textPanel, 0x264) = 1;

    const HudUiPanel_FTable *const panelFTable = *(const HudUiPanel_FTable *const *)(textPanel);
    typedef void (RECOIL_THISCALL *SetFontFn)(HudUiPanel * self, const char *faceName,
                                             int height, int weight,
                                             int width, int italic,
                                             int charSet, int pitchAndFamily);
    typedef void (RECOIL_CDECL *SetTextFmtFn)(HudUiPanel * self, const char *format, ...);
    ((SetFontFn)(panelFTable->slots[0x80 / 4]))(
        textPanel, fontParams->faceName, fontParams->height, fontParams->weight,
        fontParams->width, 0, 0, 2);
    ((SetTextFmtFn)(panelFTable->slots[0x74 / 4]))(textPanel, "   ");

    g_HudUiMgr.AddChild((HudUiElement *)(this));
    g_HudUiMgr.AddChild((HudUiElement *)(&widget));
    return 1;
}

// Reimplements 0x413ff0: HudUiMessage::ReleaseImages
void RECOIL_THISCALL HudUiMessage::ReleaseImages() {
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
void RECOIL_FASTCALL HudUiMessage::SelectVariantDisplay(int messageIndex,
                                                        int variantIndex) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    message.base.SetImageBorrowedAndInvalidate(
        FieldAt<zVidImagePartial *>(&message, 0xbc + variantIndex * 4));

    if (variantIndex == 0 || variantIndex == 3) {
        FieldAt<zVidImagePartial *>(&message, 0xd0) = FieldAt<zVidImagePartial *>(&message, 0xd8);
        message.widget.SetImageBorrowedAndInvalidate(FieldAt<zVidImagePartial *>(&message, 0xd4));
        FieldAt<int>(&message, 0x384) = 0;
    }

    if (variantIndex == 5) {
        FieldAt<int>(&message, 0x384) = 0;
    }

    if (variantIndex == 1 || variantIndex == 4) {
        FieldAt<zVidImagePartial *>(&message, 0xd4) = FieldAt<zVidImagePartial *>(&message, 0xdc);
        message.widget.SetImageBorrowedAndInvalidate(FieldAt<zVidImagePartial *>(&message, 0xd0));
        FieldAt<int>(&message, 0x384) = 1;
    }

    if (variantIndex == 6) {
        FieldAt<int>(&message, 0x384) = 1;
    }
}

// Reimplements 0x412790: HudUiMessage::ApplySideImageSwap
void RECOIL_FASTCALL HudUiMessage::ApplySideImageSwap(int messageIndex,
                                                      int sideIndex) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    zVidImagePartial *const image = FieldAt<zVidImagePartial *>(&message, 0xd8 + sideIndex * 4);
    FieldAt<zVidImagePartial *>(&message, 0xd0 + sideIndex * 4) = image;
    message.widget.SetImageBorrowedAndInvalidate(image);
    message.widget.flags &= 0x10u;
}

// Reimplements 0x4127d0: HudUiMessage::ClearDisplay
void RECOIL_FASTCALL HudUiMessage::ClearDisplay(int messageIndex) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    message.base.SetImageBorrowedAndInvalidate(0);
    message.widget.SetImageBorrowedAndInvalidate(0);

    ((HudUiPanel *)(&message.panel))->SetText("");
    ((HudUiElement *)(&message.base))->Invalidate();
}

// Reimplements 0x412650: HudUiMessage::SetValueIfOwnerMatches
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
HudUiMessage::SetValueIfOwnerMatches(int messageIndex, int ownerSideIndex,
                                     float valueOrClearToken) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    if (ownerSideIndex != FieldAt<int>(&message.panel, 0x2a4)) {
        return;
    }

    HudUiPanel *const panel = (HudUiPanel *)(&message.panel);
    if (valueOrClearToken == kHudUiMessageClearSpecialTokenValue) {
        panel->SetText(kHudUiMessageClearSpecialToken165);
        return;
    }

    panel->SetTextFmt("%d", (int)(ceil(valueOrClearToken)));
    ((HudUiElement *)(&message.base))->Invalidate();
}

// Reimplements 0x412820: HudUiMessage::UpdateSelectedWeaponDisplay
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
HudUiMessage::UpdateSelectedWeaponDisplay(int weaponBankIndex, int weaponSideIndex,
                                          float valueOrClearToken) {
    int messageIndexForText = weaponBankIndex;
    if (weaponBankIndex > 1) {
        SelectVariantDisplay(g_HudUiMgrActiveWeaponMessageIndex,
                             g_HudUiMgrActiveWeaponSideIndex);
        g_HudUiMgrActiveWeaponMessageIndex = weaponBankIndex;
        g_HudUiMgrActiveWeaponSideIndex = weaponSideIndex;
        if (valueOrClearToken > 0.0f) {
            SelectVariantDisplay(weaponBankIndex, weaponSideIndex + 3);
        }
    } else if (weaponBankIndex == 1) {
        SelectVariantDisplay(1, weaponSideIndex + 3);
        messageIndexForText = 1;
    } else {
        g_HudUiMgrActiveWeaponMessageIndex = 0;
        g_HudUiMgrActiveWeaponSideIndex = 0;
        return;
    }

    HudUiMessage &message = g_HudUiMgrMessages[messageIndexForText];
    if (weaponSideIndex != FieldAt<int>(&message.panel, 0x2a4)) {
        return;
    }

    HudUiPanel *const panel = (HudUiPanel *)(&message.panel);
    if (valueOrClearToken == kHudUiMessageClearSpecialTokenValue) {
        panel->SetTextFmt(kHudUiMessageClearSpecialToken165);
        return;
    }

    panel->SetTextFmt("%d", (int)(ceil(valueOrClearToken)));
    ((HudUiElement *)(&message.base))->Invalidate();
}

// Reimplements 0x40da00: HudUiMessage::Constructor
HudUiMessage *RECOIL_THISCALL HudUiMessage::Constructor() {
    base.Constructor(0);
    ((HudUiPanel *)(&panel))->ConstructorDefault(0, 0, 0);
    widget.Constructor(0);

    memset(variantImages, 0, 0x24);
    FieldAt<int>(&panel, 0x2a4) = 0;
    base.ftable = (const HudUiWidget_FTable *)(&g_HudUiMessage_FTable);
    return this;
}

// Reimplements 0x40d590: HudUiMessage::Destructor
void RECOIL_THISCALL HudUiMessage::Destructor() {
    widget.DestructorCore();
    ((HudUiPanel *)(&panel))->Destructor();
    base.DestructorCore();
}

// Reimplements 0x40daa0: HudUiMessage::ScalarDeletingDestructor
HudUiMessage *RECOIL_THISCALL HudUiMessage::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40eb00: HudUiShieldMessageWidget::ApplyLayout
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
HudUiShieldMessageWidget::ApplyLayout(zReader::Node *layoutRoot) {
    typedef int (RECOIL_THISCALL *GetCoordFn)(void *self);
    typedef void (RECOIL_THISCALL *SetClipFn)(void *self, void *bltSourceOrNull,
                                             const HudUiRect *rect);
    typedef void (RECOIL_CDECL *SetTextFmtFn)(void *self, const char *format, ...);
    typedef void (RECOIL_THISCALL *UpdateBoundsFn)(void *self);

    HudUiShieldMessageWidget *const shieldMessageWidget = g_HudUiMgrShieldMessageWidget;
    zReader::Node *const layoutPayload = layoutRoot->value.nodes;

    HudUiLayoutNode::ApplyImageWidget(&layoutPayload[1], &shieldMessageWidget->widget,
                                      g_HudUiMgrHudOriginX, g_HudUiMgrHudOriginY, 0,
                                      0, &shieldMessageWidget->screenRect);

    int offsetXY[2];
    const HudUiWidget_FTable *const widgetFTable = shieldMessageWidget->widget.ftable;
    offsetXY[0] =
        ((GetCoordFn)(widgetFTable->slots[0x64 / 4]))(&shieldMessageWidget->widget);
    offsetXY[1] =
        ((GetCoordFn)(widgetFTable->slots[0x68 / 4]))(&shieldMessageWidget->widget);

    HudUiPanel *const percentTextPanel = (HudUiPanel *)(&shieldMessageWidget->percentTextPanel);
    HudUiLayoutNode::ApplyTextLabel(&layoutPayload[2], percentTextPanel, 0, 0, offsetXY);

    HudUiRect clipRect;
    const HudUiCommon_FTable *const panelFTable = *(const HudUiCommon_FTable *const *)(percentTextPanel);
    clipRect.left =
        ((GetCoordFn)(panelFTable->slots[0x64 / 4]))(percentTextPanel) -
        offsetXY[0];
    clipRect.top =
        ((GetCoordFn)(panelFTable->slots[0x68 / 4]))(percentTextPanel) -
        offsetXY[1];
    ((SetClipFn)(panelFTable->slots[0x18 / 4]))(
        percentTextPanel, shieldMessageWidget->widget.image, &clipRect);

    ((SetTextFmtFn)(panelFTable->slots[0x74 / 4]))(percentTextPanel, "000");
    ((UpdateBoundsFn)(panelFTable->slots[0x78 / 4]))(percentTextPanel);

    HudUiLayoutNode::ApplyMeterQuad(&layoutPayload[3], &shieldMessageWidget->meter, 0, 0,
                                    offsetXY, &clipRect);

    const HudUiMeter_FTable *const meterFTable = shieldMessageWidget->meter.ftable;
    ((SetClipFn)(meterFTable->slots[0x18 / 4]))(
        &shieldMessageWidget->meter, shieldMessageWidget->widget.image, &clipRect);

    g_HudUiMgr.AddChild((HudUiElement *)(&shieldMessageWidget->widget));
    g_HudUiMgr.AddChild((HudUiElement *)(percentTextPanel));
    g_HudUiMgr.AddChild((HudUiElement *)(&shieldMessageWidget->meter));
    return 1;
}

// Reimplements 0x40fe30: HudUiShieldMessageWidget::Destructor
void RECOIL_THISCALL HudUiShieldMessageWidget::Destructor() {
    meter.ftable = (const HudUiMeter_FTable *)(&g_HudUiCommon_FTable);
    ((HudUiPanel *)(&percentTextPanel))->Destructor();
    widget.DestructorCore();
}

// Reimplements 0x4bcf20: HudUiBar::Constructor
HudUiBar *RECOIL_THISCALL HudUiBar::Constructor() {
    ((HudUiElement *)(this))->Constructor(0, 0);
    drawVertexCount = 0;
    ftable = &g_HudUiBar_FTable;
    memset(points, 0, sizeof(points));
    ((HudUiElement *)(this))->Invalidate();
    return this;
}

// Reimplements 0x4bcf80: HudUiBar::SetPointXY
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiBar::SetPointXY(int pointIndex, float x,
                                                          float y) {
    if (pointIndex >= 0 && pointIndex < 21) {
        points[pointIndex].x = x;
        points[pointIndex].y = y;

        if (drawVertexCount < pointIndex + 1) {
            drawVertexCount = pointIndex + 1;
        }

        if (pointIndex == 0) {
            typedef void (RECOIL_THISCALL *SetPosFn)(HudUiBar * self, int x, int y);

            ((SetPosFn)(ftable->slots[3]))(this, (int)(x),
                                                         (int)(y));
        }
    }

    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiBar * self);
    ((InvalidateFn)(ftable->slots[8]))(this);
}

// Reimplements 0x4bf840: HudUiPolyline::Constructor
RECOIL_NOINLINE HudUiPolyline *RECOIL_THISCALL HudUiPolyline::Constructor() {
    base.Constructor(0, 0);
    base.ftable = (const HudUiCommon_FTable *)(&g_HudUiPolyline_FTable);
    pointCount = 0;
    memset(points, 0, sizeof(points));
    base.Invalidate();
    clipRect = 0;
    return this;
}

// Reimplements 0x4bf900: HudUiPolyline::Draw
RECOIL_NOINLINE void RECOIL_THISCALL HudUiPolyline::Draw() {
    typedef void (RECOIL_FASTCALL *DrawBaseFn)(HudUiPolyline * self);
    ((DrawBaseFn)(base.ftable->slots[2]))(this);

    const int currentPointCount = pointCount;
    if (currentPointCount == 0) {
        return;
    }

    if (clipRect != 0) {
        zRndr_DrawClippedImmediateLineStrip((const zRndr_LinePoint2I *)(points),
                                            currentPointCount - 1, clipRect, color565);
        return;
    }

    {
    for (int index = 0; index < currentPointCount - 1; ++index) {
        const HudUiPolylinePoint &point = points[index];
        const HudUiPolylinePoint &nextPoint = points[index + 1];
        zRndr_DrawImmediateLine(point.x, point.y, nextPoint.x, nextPoint.y, color565);
    }
    }
}

// Reimplements 0x4bf8b0: HudUiPolyline::SetPoint
RECOIL_NOINLINE void RECOIL_THISCALL HudUiPolyline::SetPoint(int index,
                                                             int pointX,
                                                             int pointY) {
    typedef void (RECOIL_THISCALL *SetPosFn)(HudUiPolyline * self, int x, int y);
    typedef void (RECOIL_THISCALL *InvalidateFn)(HudUiPolyline * self);

    points[index].x = pointX;
    points[index].y = pointY;

    if (pointCount <= index) {
        pointCount = index + 1;
    }

    if (index == 0) {
        ((SetPosFn)(base.ftable->slots[3]))(this, pointX, pointY);
    }

    ((InvalidateFn)(base.ftable->slots[8]))(this);
}

// Reimplements 0x4b4620: HudUiSliderBorder::Constructor
HudUiSliderBorder *RECOIL_THISCALL HudUiSliderBorder::Constructor() {
    base.Constructor();
    base.base.ftable = (const HudUiCommon_FTable *)(&g_HudUiSliderBorder_FTable);
    originX = 0;
    originY = 0;
    halfWidth = 1;
    height = 10;
    blinkEnabled = 0;
    blinkPeriodSec = 0.35f;
    blinkDirSign = 1;
    blinkTimeRemainingSec = 0.0f;

    base.SetPoint(0, -1, 0);
    base.SetPoint(1, halfWidth, 0);
    base.SetPoint(2, halfWidth, 1);
    base.SetPoint(3, 0, 1);
    base.SetPoint(4, 0, height - 1);
    base.SetPoint(5, halfWidth, height - 1);
    base.SetPoint(6, halfWidth, height);
    base.SetPoint(7, -halfWidth, height);
    base.SetPoint(8, -halfWidth, height - 1);
    base.SetPoint(9, 0, height - 1);
    base.SetPoint(10, 0, 1);
    base.SetPoint(11, -halfWidth, 1);
    base.SetPoint(12, -halfWidth, 0);
    return this;
}

// Reimplements 0x4b47b0: HudUiSliderBorder::Update
void RECOIL_THISCALL HudUiSliderBorder::Update(float deltaSeconds) {
    if ((base.base.flags & 0x10) != 0) {
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
        base.Draw();
    }
}

// Reimplements 0x4b4810: HudUiSliderBorder::SetBounds
void RECOIL_THISCALL HudUiSliderBorder::SetBounds(int newOriginX, int newOriginY,
                                                  int newHalfWidth,
                                                  int newHeight) {
    originX = newOriginX;
    originY = newOriginY;
    halfWidth = newHalfWidth;
    height = newHeight;

    base.SetPoint(0, originX - halfWidth, originY);
    base.SetPoint(1, originX + halfWidth, originY);
    base.SetPoint(2, originX + halfWidth, originY + 1);
    base.SetPoint(3, originX, originY + 1);
    base.SetPoint(4, originX, originY + height - 1);
    base.SetPoint(5, originX + halfWidth, originY + height - 1);
    base.SetPoint(6, originX + halfWidth, originY + height);
    base.SetPoint(7, originX - halfWidth, originY + height);
    base.SetPoint(8, originX - halfWidth, originY + height - 1);
    base.SetPoint(9, originX, originY + height - 1);
    base.SetPoint(10, originX, originY + 1);
    base.SetPoint(11, originX - halfWidth, originY + 1);
    base.SetPoint(12, originX - halfWidth, originY);
}

// Reimplements 0x4b49e0: HudUiNumericTextInput::BaseConstructor
HudUiNumericTextInput *RECOIL_THISCALL HudUiNumericTextInput::BaseConstructor() {
    base.Constructor();
    textInput.Constructor(0x100);
    owner = 0;
    textInput.ftable = &g_HudUiNumericTextInput_TextInputFTable;

    sliderBorder.Constructor();
    sliderBorder.sliderVisibleWhenInputActive = 0;
    sliderBorder.rawKeyFilterEnabled = 0;
    base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNumericTextInput_Base_FTable);
    sliderBorder.inputActive = 1;
    sliderBorder.caretHalfWidth = 0;

    sliderBorder.base.base.SetVisible(1);
    owner = this;
    ((HudUiElement *)(&base.base))->SetVisible(1);
    return this;
}

// Reimplements 0x4b4e40: HudUiNumericTextInput::AllocTextBuffer
void RECOIL_THISCALL HudUiNumericTextInput::AllocTextBuffer(unsigned int bufferSize) {
    textInput.AllocTextBuffer(bufferSize);
}

// Reimplements 0x4b4ed0: HudUiNumericTextInput::GetBuffer
char *RECOIL_THISCALL HudUiNumericTextInput::GetBuffer() {
    return textInput.GetBuffer();
}

// Reimplements 0x4b4e60: HudUiNumericTextInput::Update
void RECOIL_THISCALL HudUiNumericTextInput::Update(const char *text) {
    textInput.SetContents(text);
    textInput.SetCursorPosition((int)(strlen(text)));
    char *const buffer = textInput.GetBuffer();

    if (base.labelPanels.begin != 0 && base.labelPanels.end != base.labelPanels.begin) {
        void *const firstPanel = base.labelPanels.begin[0];
        typedef void (RECOIL_THISCALL *SetTextFn)(void *panel, const char *text);
        const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(firstPanel);
        ((SetTextFn)(ftable->slots[35]))(firstPanel, buffer);
    }

    ((HudUiElement *)(&base.base))->Invalidate();
}

// Reimplements 0x4b4ca0: HudUiNumericTextInput::UpdateCaptureUiAndClip
RECOIL_NO_GS void RECOIL_THISCALL
HudUiNumericTextInput::UpdateCaptureUiAndClip(float deltaSeconds) {
    HudUiPanel *const firstPanel = base.labelPanels.begin[0];
    HudUiElement *const firstElement = (HudUiElement *)(firstPanel);
    HudUiElement *const baseElement = (HudUiElement *)(&base.base);
    HudUiElement *const sliderElement = (HudUiElement *)(&sliderBorder);

    if ((base.base.flags & 0x10) != 0) {
        HudUiVirtualSetVisibleRequired(firstElement, 0);
        HudUiVirtualInvalidateRequired(firstElement);
        HudUiVirtualSetVisibleRequired(sliderElement, 0);
        HudUiVirtualInvalidateRequired(sliderElement);
        return;
    }

    if (sliderBorder.sliderVisibleWhenInputActive != 0) {
        HudUiVirtualSetVisibleRequired(firstElement, 1);
        char *const buffer = textInput.GetBuffer();

        if (base.labelPanels.begin != 0) {
            const ptrdiff_t panelCount = base.labelPanels.end - base.labelPanels.begin;
            if (panelCount != 0) {
                HudUiVirtualSetTextRequired(firstPanel, buffer);
            }
        }

        RECT textRect = {0};
        textRect.left = HudUiVirtualGetXRequired(firstElement);
        textRect.top = HudUiVirtualGetYRequired(firstElement);
        textRect.right = HudUiVirtualGetXRequired(firstElement);
        textRect.bottom = HudUiVirtualGetYRequired(firstElement);

        if (firstPanel->MeasureTextPrefixRect((int)(textInput.cursor),
                                              &textRect) != 0) {
            const unsigned int textColor = FieldAt<unsigned int>(firstPanel, 0x14c);
            const unsigned int packedColor =
                zVid_PackColorRGB((unsigned char)(textColor),
                                  (unsigned char)(textColor >> 8),
                                  (unsigned char)(textColor >> 16)) &
                0xffffu;
            sliderBorder.base.color565 = (int)(packedColor);
            sliderBorder.SetBounds(textRect.right, textRect.top, sliderBorder.caretHalfWidth,
                                   textRect.bottom - textRect.top);
            HudUiVirtualSetVisibleRequired(sliderElement, 1);
        }

        HudUiVirtualInvalidateRequired(baseElement);
    } else {
        HudUiVirtualSetVisibleRequired(sliderElement, 0);
    }

    baseElement->Update(deltaSeconds);
    HudUiVirtualUpdateSliderRequired(&sliderBorder, deltaSeconds);
}

// Reimplements 0x4b4c50: HudUiNumericTextInput::SetRawKeyboardCapture
void RECOIL_THISCALL HudUiNumericTextInput::SetRawKeyboardCapture(int enable) {
    const char enableByte = (char)(enable);
    if (enableByte == sliderBorder.sliderVisibleWhenInputActive) {
        return;
    }

    sliderBorder.sliderVisibleWhenInputActive = enableByte;
    if (enableByte != 0) {
        zInput::Keyboard_SetRawEventCallback(
            (void *)(&HudUiNumericTextInput::RawKeyboardCallback), this);
    } else {
        zInput::Keyboard_SetRawEventCallback(0, 0);
    }
}

// Reimplements 0x4b4c90: HudUiNumericTextInput::OnActivate
void RECOIL_THISCALL HudUiNumericTextInput::OnActivate() {
    sliderBorder.inputActive = 1;
    base.OnActivate();
}

// Reimplements 0x4b4ac0: HudUiNumericTextInput::Destructor
void RECOIL_THISCALL HudUiNumericTextInput::Destructor() {
    base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNumericTextInput_Base_FTable);
    SetRawKeyboardCapture(0);
    sliderBorder.base.base.ftable = &g_HudUiCommon_FTable;
    textInput.DestructorCore();
    base.DestructorCore();
}

// Reimplements 0x4b4a90: HudUiNumericTextInput::ScalarDeletingDestructor
HudUiNumericTextInput *RECOIL_THISCALL
HudUiNumericTextInput::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4b4b30: HudUiNumericTextInput::RawKeyboardCallback
int RECOIL_FASTCALL
HudUiNumericTextInput::RawKeyboardCallback(int key, HudUiNumericTextInput *callbackCtx) {
    if (callbackCtx == 0) {
        return 0;
    }

    typedef int (RECOIL_THISCALL *RawKeyboardFn)(HudUiNumericTextInput * self, int key);
    return ((RawKeyboardFn)(callbackCtx->base.base.ftable->slots[33]))(callbackCtx,
                                                                                     key);
}

// Reimplements 0x4b4ba0: HudUiNumericTextInput::SetInputActive
int RECOIL_THISCALL HudUiNumericTextInput::SetInputActive(int active) {
    const int previousActive = sliderBorder.inputActive;
    sliderBorder.inputActive = active;

    HudUiElement *firstLabelPanel = 0;
    if (base.labelPanels.begin != 0 && base.labelPanels.end != base.labelPanels.begin) {
        firstLabelPanel = (HudUiElement *)(base.labelPanels.begin[0]);
    }

    if (active != 0) {
        HudUiVirtualSetVisibleRequired(&base.base, 1);
        HudUiVirtualSetVisibleRequired(&sliderBorder, 1);
        if (firstLabelPanel != 0) {
            HudUiVirtualSetVisibleRequired(firstLabelPanel, 1);
        }
    } else {
        HudUiVirtualSetVisibleRequired(&base.base, 0);
        if (firstLabelPanel != 0) {
            HudUiVirtualSetVisibleRequired(firstLabelPanel, 0);
        }
        HudUiVirtualSetVisibleRequired(&sliderBorder, 0);
    }

    return previousActive;
}

// Reimplements 0x4b4b50: HudUiNumericTextInput::OnRawKeyboardChar
int RECOIL_THISCALL HudUiNumericTextInput::OnRawKeyboardChar(int key) {
    if (sliderBorder.rawKeyFilterEnabled == 0 ||
        strchr(kNumericTextInputAcceptedRawKeyChars, key) != 0) {
        textInput.DispatchKeyAction(key);
    }

    return 0;
}

// Reimplements 0x40fb70: HudUiMeter::Constructor
HudUiMeter *RECOIL_THISCALL HudUiMeter::Constructor() {
    ((HudUiBar *)(this))->Constructor();
    ftable = &g_HudUiMeter_FTable;
    fillPixelsMax = 0;
    meterFlags = 0;
    return this;
}

// Reimplements 0x40d9e0: HudUiMeter::ConstructorEx
HudUiMeter *RECOIL_THISCALL HudUiMeter::ConstructorEx() {
    ((HudUiBar *)(this))->Constructor();
    ftable = &g_HudUiMeterEx_FTable;
    fillPixelsMax = 0;
    meterFlags = 0;
    return this;
}

// Reimplements 0x4bcb50: HudUiTextLabel::ConstructorWithPosAndFlags
RECOIL_NOINLINE HudUiTextLabel *RECOIL_THISCALL HudUiTextLabel::ConstructorWithPosAndFlags(
    const char *text, int initX, int initY, int flags) {
    base.Constructor(0, 0);
    base.ftable = (const HudUiCommon_FTable *)(&g_HudUiTextLabel_FTable);
    centerText = 0;
    SetTextFmt(text);
    base.x = initX;
    base.y = initY;
    HudUiVirtualInvalidateRequired(&base);
    fontHandle = flags;
    HudUiVirtualInvalidateRequired(&base);
    alignMode = 0;
    return this;
}

// Reimplements 0x4bcbe0: HudUiTextLabel::CopyConstructor
HudUiTextLabel *RECOIL_THISCALL HudUiTextLabel::CopyConstructor(const HudUiTextLabel *source) {
    base.CopyConstructor(&source->base);
    base.ftable = (const HudUiCommon_FTable *)(&g_HudUiTextLabel_FTable);
    strncpy(textBuffer, source->textBuffer, sizeof(textBuffer));
    fontHandle = source->fontHandle;
    centerText = source->centerText;
    centerBoundsLeft = source->centerBoundsLeft;
    centerBoundsRight = source->centerBoundsRight;
    alignMode = source->alignMode;
    return this;
}

// Reimplements 0x4bcc80: HudUiTextLabel::Constructor
HudUiTextLabel *RECOIL_THISCALL HudUiTextLabel::Constructor(const HudUiTextLabel *source) {
    base.CopyFrom(&source->base);
    strncpy(textBuffer, source->textBuffer, sizeof(textBuffer));
    fontHandle = source->fontHandle;
    centerText = source->centerText;
    centerBoundsLeft = source->centerBoundsLeft;
    centerBoundsRight = source->centerBoundsRight;
    alignMode = source->alignMode;
    return this;
}

// Reimplements 0x4bccf0: HudUiTextLabel::SetTextFmt
void HudUiTextLabel::SetTextFmt(const char *format, ...) {
    if (format == 0) {
        memset(textBuffer, 0, sizeof(textBuffer));
        return;
    }

    va_list args;
    va_start(args, format);
    vsprintf(textBuffer, format, args);
    va_end(args);

    if (centerText != 0) {
        UpdateTextExtents();
    }

    HudUiVirtualInvalidateRequired(&base);
}

// Reimplements 0x4bcd80: HudUiTextLabel::RebuildTextBounds
RECOIL_NOINLINE void RECOIL_THISCALL HudUiTextLabel::RebuildTextBounds() {
    int widthPx = 0;
    int lineAdvance = 0;
    zImage_Font::MeasureString(textBuffer, fontHandle, &widthPx, &lineAdvance);
    base.clipRect.right = base.clipRect.left + widthPx;
    base.clipRect.bottom = base.clipRect.top + lineAdvance;
}

// Reimplements 0x4bcdc0: HudUiTextLabel::MeasureTextWidth
RECOIL_NOINLINE int RECOIL_THISCALL HudUiTextLabel::MeasureTextWidth() {
    int widthPx = 0;
    int lineAdvance = 0;
    zImage_Font::MeasureString(textBuffer, fontHandle, &widthPx, &lineAdvance);
    return widthPx;
}

// Reimplements 0x4bcdf0: HudUiTextLabel::UpdateTextExtents
void RECOIL_THISCALL HudUiTextLabel::UpdateTextExtents() {
    const int widthPx = MeasureTextWidth();
    base.x = centerBoundsLeft + (centerBoundsRight - widthPx - centerBoundsLeft) / 2;

    if (base.bltSource != 0) {
        base.clipRect.left = base.x;
        base.clipRect.top = base.y;
        RebuildTextBounds();
    }
}

// Reimplements 0x4ba740: HudUiPanel::ConstructorDefault
HudUiPanel *RECOIL_THISCALL HudUiPanel::ConstructorDefault(const char *text, int initX,
                                                           int initY) {
    ((HudUiTextLabel *)(this))->ConstructorWithPosAndFlags(text, initX, initY, 0);

    vtbl = &g_HudUiPanel_FTable;
    textPick = 0;
    FieldAt<unsigned int>(this, 0x14c) = 0x00ffffff;
    FieldAt<unsigned int>(this, 0x150) = 0x00ffffff;
    FieldAt<unsigned int>(this, 0x270) = 1;
    hFont = GetStockObject(OEM_FIXED_FONT);
    FieldAt<int>(this, 0x28c) = 0;
    FieldAt<char>(this, 0x15c) = 0;
    FieldAt<unsigned int>(this, 0x264) = 0;
    FieldAt<unsigned int>(this, 0x270) = 1;
    FieldAt<int>(this, 0x290) = 0;
    FieldAt<int>(this, 0x144) = 0;
    FieldAt<unsigned int>(this, 0x268) = 1;
    FieldAt<int>(this, 0x284) = 0;
    FieldAt<int>(this, 0x294) = 0;
    FieldAt<int>(this, 0x27c) = 0;
    FieldAt<int>(this, 0x288) = 0;
    FieldAt<int>(this, 0x298) = 0;
    FieldAt<int>(this, 0x280) = 0;
    FieldAt<int>(this, 0x278) = 0;
    FieldAt<int>(this, 0x274) = 0;
    FieldAt<int>(this, 0x260) = 0;
    FieldAt<int>(this, 0x25c) = 0;
    return this;
}

// Reimplements 0x4ba850: HudUiPanel::CopyConstructCore
RECOIL_NOINLINE HudUiPanel *RECOIL_THISCALL
HudUiPanel::CopyConstructCore(const HudUiPanel *source) {
    ((HudUiTextLabel *)(this))->CopyConstructor(
        (const HudUiTextLabel *)(source));

    vtbl = &g_HudUiPanel_FTable;
    textPick = 0;
    FieldAt<unsigned int>(this, 0x14c) = FieldAt<unsigned int>(source, 0x14c);
    FieldAt<unsigned int>(this, 0x150) = FieldAt<unsigned int>(source, 0x150);

    LOGFONTA logFont = {0};
    if (GetObjectA(source->hFont, sizeof(logFont), &logFont) != 0) {
        hFont = CreateFontIndirectA(&logFont);
    }

    FieldAt<unsigned int>(this, 0x158) = FieldAt<unsigned int>(source, 0x158);
    strncpy(&FieldAt<char>(this, 0x15c), &FieldAt<char>(source, 0x15c), 0x100);

    FieldAt<int>(this, 0x25c) = FieldAt<int>(source, 0x25c);
    FieldAt<int>(this, 0x260) = FieldAt<int>(source, 0x260);
    FieldAt<unsigned int>(this, 0x264) = FieldAt<unsigned int>(source, 0x264);
    FieldAt<unsigned int>(this, 0x268) = FieldAt<unsigned int>(source, 0x268);
    FieldAt<unsigned int>(this, 0x26c) = FieldAt<unsigned int>(source, 0x26c);
    FieldAt<unsigned int>(this, 0x270) = FieldAt<unsigned int>(source, 0x270);
    FieldAt<int>(this, 0x274) = FieldAt<int>(source, 0x274);
    FieldAt<int>(this, 0x278) = FieldAt<int>(source, 0x278);
    FieldAt<HudUiRect>(this, 0x27c) = FieldAt<HudUiRect>(source, 0x27c);
    FieldAt<HudUiRect>(this, 0x28c) = FieldAt<HudUiRect>(source, 0x28c);
    FieldAt<int>(this, 0x144) = FieldAt<int>(source, 0x144);
    FieldAt<int>(this, 0x29c) = FieldAt<int>(source, 0x29c);
    FieldAt<int>(this, 0x2a0) = FieldAt<int>(source, 0x2a0);
    return this;
}

// Reimplements 0x4ba9e0: HudUiPanel::ConstructorCopy
RECOIL_NOINLINE HudUiPanel *RECOIL_THISCALL HudUiPanel::ConstructorCopy(const HudUiPanel *source) {
    ((HudUiTextLabel *)(this))->Constructor(
        (const HudUiTextLabel *)(source));

    textPick = 0;
    FieldAt<unsigned int>(this, 0x14c) = FieldAt<unsigned int>(source, 0x14c);
    FieldAt<unsigned int>(this, 0x150) = FieldAt<unsigned int>(source, 0x150);

    LOGFONTA logFont = {0};
    if (GetObjectA(source->hFont, sizeof(logFont), &logFont) != 0) {
        hFont = CreateFontIndirectA(&logFont);
    }

    FieldAt<unsigned int>(this, 0x158) = FieldAt<unsigned int>(source, 0x158);
    strncpy(&FieldAt<char>(this, 0x15c), &FieldAt<char>(source, 0x15c), 0x100);

    FieldAt<int>(this, 0x25c) = FieldAt<int>(source, 0x25c);
    FieldAt<int>(this, 0x260) = FieldAt<int>(source, 0x260);
    FieldAt<unsigned int>(this, 0x264) = FieldAt<unsigned int>(source, 0x264);
    FieldAt<unsigned int>(this, 0x268) = FieldAt<unsigned int>(source, 0x268);
    FieldAt<unsigned int>(this, 0x26c) = FieldAt<unsigned int>(source, 0x26c);
    FieldAt<unsigned int>(this, 0x270) = 1;
    FieldAt<int>(this, 0x274) = FieldAt<int>(source, 0x274);
    FieldAt<int>(this, 0x278) = FieldAt<int>(source, 0x278);
    FieldAt<HudUiRect>(this, 0x27c) = FieldAt<HudUiRect>(source, 0x27c);
    FieldAt<HudUiRect>(this, 0x28c) = FieldAt<HudUiRect>(source, 0x28c);
    FieldAt<int>(this, 0x144) = FieldAt<int>(source, 0x144);
    FieldAt<int>(this, 0x29c) = FieldAt<int>(source, 0x29c);
    FieldAt<int>(this, 0x2a0) = FieldAt<int>(source, 0x2a0);
    return this;
}

// Reimplements 0x4bab40: HudUiPanel::Destructor
void RECOIL_THISCALL HudUiPanel::Destructor() {
    vtbl = &g_HudUiPanel_FTable;

    if (textPick != 0) {
        zVid_Image::Destroy(textPick);
        textPick = 0;
    }

    DeleteObject(hFont);
    vtbl = &g_HudUiCommon_FTable;
}

// Reimplements 0x40bef0: HudUiPanel::DestructorThunk
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiPanel::DestructorThunk()
{
    Destructor();
}

// Reimplements 0x4bb460: HudUiPanel::Draw
void RECOIL_THISCALL HudUiPanel::Draw() {
    if (FieldAt<unsigned int>(this, 0x270) != 0) {
        typedef void (RECOIL_THISCALL *RebuildFn)(HudUiPanel * self);
        const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(this);
        ((RebuildFn)(ftable->slots[36]))(this);
    }

    if (textPick == 0) {
        return;
    }

    if (FieldAt<char>(this, 0x34) == '\0') {
        HudUiPanelVirtualDrawBase(this);
        return;
    }

    const int alignMode = FieldAt<int>(this, 0x144);
    if (alignMode == 0) {
        HudUiPanelVirtualDrawBase(this);
        zVid_Image::BlitToActiveTarget(textPick, FieldAt<int>(this, 0x14),
                                       FieldAt<int>(this, 0x18), 0,
                                       &FieldAt<zVidRect32>(this, 0x28c));
        return;
    }

    if (FieldAt<unsigned int>(this, 0x270) != 0) {
        typedef void (RECOIL_THISCALL *RebuildFn)(HudUiPanel * self);
        const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(this);
        ((RebuildFn)(ftable->slots[36]))(this);
    }

    int frameWidth = FieldAt<int>(this, 0x28) - FieldAt<int>(this, 0x20);
    int textWidth = FieldAt<int>(this, 0x25c);
    if (alignMode == 1) {
        frameWidth >>= 1;
        textWidth >>= 1;
    }

    FieldAt<int>(this, 0x14) -= frameWidth;
    HudUiPanelVirtualDrawBase(this);

    const int dstX = FieldAt<int>(this, 0x14) + frameWidth - textWidth;
    FieldAt<int>(this, 0x14) = dstX;
    zVid_Image::BlitToActiveTarget(textPick, dstX, FieldAt<int>(this, 0x18), 0,
                                   &FieldAt<zVidRect32>(this, 0x28c));
    FieldAt<int>(this, 0x14) += textWidth;
}

// Reimplements 0x4ba400: HudUiPanel::GetWrapRect
HudUiRect *RECOIL_THISCALL HudUiPanel::GetWrapRect() {
    return &FieldAt<HudUiRect>(this, 0x27c);
}

// Reimplements 0x4bb3d0: HudUiPanel::HitTest
int RECOIL_THISCALL HudUiPanel::HitTest(int px, int py) {
    if ((FieldAt<unsigned int>(this, 0x0c) & 0x10u) != 0 ||
        FieldAt<int>(this, 0x14) > px || FieldAt<int>(this, 0x18) > py) {
        return 0;
    }

    if (FieldAt<unsigned int>(this, 0x270) != 0) {
        typedef void (RECOIL_THISCALL *RebuildFn)(HudUiPanel * self);
        const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(this);
        ((RebuildFn)(ftable->slots[36]))(this);
    }

    if (px >= FieldAt<int>(this, 0x14) + FieldAt<int>(this, 0x25c)) {
        return 0;
    }

    return py < FieldAt<int>(this, 0x18) + QueryTextHeight() ? 1 : 0;
}

// Reimplements 0x4bb440: HudUiPanel::GetLastTextPtr
char *RECOIL_THISCALL HudUiPanel::GetLastTextPtr() {
    if (FieldAt<unsigned int>(this, 0x270) != 0) {
        typedef void (RECOIL_THISCALL *RebuildFn)(HudUiPanel * self);
        const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(this);
        ((RebuildFn)(ftable->slots[36]))(this);
    }

    return &FieldAt<char>(this, 0x15c);
}

// Reimplements 0x4bb740: HudUiPanel::GetTextRect
void RECOIL_THISCALL HudUiPanel::GetTextRect(HudUiRect *outRect) {
    ((HudUiElement *)(this))->GetRect(outRect);

    if (FieldAt<unsigned int>(this, 0x270) != 0) {
        typedef void (RECOIL_THISCALL *RebuildFn)(HudUiPanel * self);
        const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(this);
        ((RebuildFn)(ftable->slots[36]))(this);
    }

    outRect->right = outRect->left + FieldAt<int>(this, 0x25c);
    outRect->bottom = outRect->top + QueryTextHeight();
}

// Reimplements 0x40be90: HudUiPanel::Invalidate
void RECOIL_THISCALL HudUiPanel::Invalidate() {
    FieldAt<unsigned int>(this, 0x270) = 1;
    ((HudUiElement *)(this))->Invalidate();
}

// Reimplements 0x40bea0: HudUiPanel::GetFont
HGDIOBJ RECOIL_THISCALL HudUiPanel::GetFont() {
    return hFont;
}

// Reimplements 0x40beb0: HudUiPanel::SetFontHandle
void RECOIL_THISCALL HudUiPanel::SetFontHandle(HGDIOBJ fontHandle) {
    hFont = fontHandle;
}

// Reimplements 0x40bec0: HudUiPanel::EnableWordWrapWithRect
void RECOIL_THISCALL HudUiPanel::EnableWordWrapWithRect(const HudUiRect *rect) {
    FieldAt<unsigned int>(this, 0x278) = 1;
    FieldAt<HudUiRect>(this, 0x27c) = *rect;
}

// Reimplements 0x40bf00: HudUtil::FreeFieldPtr
void RECOIL_THISCALL HudUtil::FreeFieldPtr() {
    if (fieldPtr != 0) {
        free(fieldPtr);
        fieldPtr = 0;
    }
}

// Reimplements 0x40a590: HudUiPanel::ScalarDeletingDestructor
HudUiPanel *RECOIL_THISCALL HudUiPanel::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40f9e0: HudUiPanel::SetTextColor
unsigned int RECOIL_THISCALL HudUiPanel::SetTextColor(unsigned int color) {
    const unsigned int previous = FieldAt<unsigned int>(this, 0x14c);
    FieldAt<unsigned int>(this, 0x14c) = color;
    FieldAt<unsigned int>(this, 0x150) = color;
    FieldAt<unsigned int>(this, 0x270) = 1;
    return previous;
}

// Reimplements 0x40e010: HudUiPanel::SetTextColorsAndMarkDirty
void RECOIL_THISCALL HudUiPanel::SetTextColorsAndMarkDirty(unsigned int color0,
                                                           unsigned int color1) {
    FieldAt<unsigned int>(this, 0x14c) = color0;
    FieldAt<unsigned int>(this, 0x150) = color1;
    FieldAt<unsigned int>(this, 0x270) = 1;
}

// Reimplements 0x40e040: HudUiPanel::SetShadow
unsigned int RECOIL_THISCALL HudUiPanel::SetShadow(unsigned int shadowEnabled,
                                                    int shadowOffsetX,
                                                    int shadowOffsetY) {
    const unsigned int previous = FieldAt<unsigned int>(this, 0x264);
    FieldAt<unsigned int>(this, 0x264) = shadowEnabled;
    FieldAt<int>(this, 0x29c) = shadowOffsetX;
    FieldAt<int>(this, 0x2a0) = shadowOffsetY;
    return previous;
}

// Reimplements 0x4babb0: HudUiPanel::SetFont
void RECOIL_THISCALL HudUiPanel::SetFont(const char *faceName, int height,
                                         int weight, int width,
                                         int italic, int charSet,
                                         int pitchAndFamily) {
    DeleteObject(hFont);
    hFont = CreateFontA(-height, width, 0, 0, weight, italic, 0, 0, charSet, OUT_TT_PRECIS,
                        CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, pitchAndFamily, faceName);
    FieldAt<unsigned int>(this, 0x270) = 1;
}

// Reimplements 0x4bb540: HudUiPanel::SetTextFmt
void HudUiPanel::SetTextFmt(const char *format, ...) {
    va_list args;
    va_start(args, format);
    SetTextFmtV(format, args);
    va_end(args);
}

// Reimplements 0x4bb5e0: HudUiPanel::SetTextFmtV
void RECOIL_THISCALL HudUiPanel::SetTextFmtV(const char *format, va_list args) {
    char *const textBuffer = &FieldAt<char>(this, 0x34);
    if (format == 0) {
        memset(textBuffer, 0, 0x100);
        FieldAt<unsigned int>(this, 0x270) = 1;
        return;
    }

    _vsnprintf(textBuffer, 0x100, format, args);
    textBuffer[0xff] = '\0';

    char *const cached = &FieldAt<char>(this, 0x15c);
    if (strncmp(cached, textBuffer, 0x100) == 0) {
        return;
    }

    if (FieldAt<unsigned int>(this, 0x138) != 0) {
        ((HudUiTextLabel *)(this))->UpdateTextExtents();
    }

    HudUiVirtualInvalidate(this);
    FieldAt<unsigned int>(this, 0x270) = 1;
    strncpy(cached, textBuffer, 0x100);
}

// Reimplements 0x4bb680: HudUiPanel::SetText
void RECOIL_THISCALL HudUiPanel::SetText(const char *text) {
    char *const textBuffer = &FieldAt<char>(this, 0x34);
    if (text == 0) {
        memset(textBuffer, 0, 0x100);
        FieldAt<unsigned int>(this, 0x270) = 1;
        return;
    }

    strncpy(textBuffer, text, 0x100);

    char *const cached = &FieldAt<char>(this, 0x15c);
    if (strncmp(cached, textBuffer, 0x100) == 0) {
        return;
    }

    if (FieldAt<unsigned int>(this, 0x138) != 0) {
        ((HudUiTextLabel *)(this))->UpdateTextExtents();
    }

    HudUiVirtualInvalidate(this);
    FieldAt<unsigned int>(this, 0x270) = 1;
    strncpy(cached, textBuffer, 0x100);
}

// Reimplements 0x4bac10: HudUiPanel::RebuildTextRect
void RECOIL_THISCALL HudUiPanel::RebuildTextRect() {
    char *const textBuffer = &FieldAt<char>(this, 0x34);
    if (strlen(textBuffer) == 0) {
        memset(&FieldAt<HudUiRect>(this, 0x28c), 0, sizeof(HudUiRect));
        FieldAt<int>(this, 0x260) = 0;
        FieldAt<int>(this, 0x25c) = 0;
        FieldAt<unsigned int>(this, 0x270) = 0;
        return;
    }

    HDC measureDc = CreateCompatibleDC(0);
    if (measureDc == 0) {
        FieldAt<unsigned int>(this, 0x270) = 0;
        return;
    }

    SelectObject(measureDc, hFont);

#if defined(_MSC_VER) && _MSC_VER < 1200
    RECT &textRect = *&FieldAt<RECT>(this, 0x28c);
#else
    RECT &textRect = FieldAt<RECT>(this, 0x28c);
#endif
    UINT drawFormat = DT_LEFT;
    BOOL measured = FALSE;
    if (FieldAt<unsigned int>(this, 0x278) != 0) {
        textRect = FieldAt<RECT>(this, 0x27c);
        drawFormat = DT_WORDBREAK;
        measured = TRUE;
    } else {
        measured = DrawTextA(measureDc, textBuffer, -1, &textRect, DT_CALCRECT);
    }

    if (measured == 0) {
        GetLastError();
    } else {
        if (FieldAt<unsigned int>(this, 0x264) != 0) {
            textRect.bottom += abs(FieldAt<int>(this, 0x2a0));
            textRect.right += abs(FieldAt<int>(this, 0x29c));
        }

        const int textWidth = textRect.right - textRect.left;
        const int textHeight = textRect.bottom - textRect.top;
        FieldAt<int>(this, 0x25c) = textWidth;
        FieldAt<int>(this, 0x260) = textHeight;

        if (textPick != 0 && (textWidth > textPick->width || textHeight > textPick->height)) {
            zVid_Image::Destroy(textPick);
            textPick = 0;
        }

        if (textPick == 0) {
            textPick = zVid_Image::Create();
            zVid_Image::SetFormatCode(textPick, 3);
            zVid_Image::SetSize(textPick, (short)(textWidth),
                                (short)(textHeight));
            void *const pixels =
                malloc(zVid_Image::QueryBytesPerPixel(textPick) * textWidth * textHeight);
            zVid_Image_SetPixels(textPick, pixels, 0);
            textPick->formatFlagsPacked |= 0x20;
        }

        if (textPick != 0) {
            const int clearBytes =
                zVid_Image::QueryBytesPerPixel(textPick) * textPick->pixelCount;
            memset(textPick->pixels, 0, clearBytes);

            typedef int (RECOIL_FASTCALL *UploadPixelsFn)(zVidImagePartial * image, HDC * outDc);
            typedef void (RECOIL_FASTCALL *ReleaseSurfaceFn)(zVidImagePartial * image, HDC dc);

            HDC drawDc = 0;
            const unsigned int uploadAddress = g_zVideo_pfnImageUploadPixelsToSurface;
            UploadPixelsFn uploadPixels = (UploadPixelsFn)(uploadAddress);
            if (IsCallableProviderAddress(uploadAddress) && uploadPixels(textPick, &drawDc) != 0) {
                RECT shadowRect = textRect;
                RECT mainRect = textRect;
                SelectObject(drawDc, hFont);

                if (FieldAt<unsigned int>(this, 0x264) != 0) {
                    if (FieldAt<int>(this, 0x29c) > 0) {
                        shadowRect.left += FieldAt<int>(this, 0x29c);
                    } else {
                        mainRect.left -= FieldAt<int>(this, 0x29c);
                    }

                    if (FieldAt<int>(this, 0x2a0) > 0) {
                        shadowRect.top += FieldAt<int>(this, 0x2a0);
                    } else {
                        mainRect.top -= FieldAt<int>(this, 0x2a0);
                    }

                    ::SetTextColor(drawDc, 0x00141414);
                    if (FieldAt<int>(this, 0x268) == OPAQUE) {
                        SetBkColor(drawDc, 0x20);
                    }

                    SetBkMode(drawDc, FieldAt<int>(this, 0x268));
                    DrawTextA(drawDc, textBuffer, -1, &shadowRect, drawFormat);
                }

                const unsigned int textColor0 = FieldAt<unsigned int>(this, 0x14c);
                const unsigned int textColor1 = FieldAt<unsigned int>(this, 0x150);
                ::SetTextColor(drawDc, textColor0 == textColor1 ? textColor0 : 0x00ffffff);
                if (FieldAt<int>(this, 0x268) == OPAQUE) {
                    SetBkColor(drawDc, FieldAt<COLORREF>(this, 0x26c));
                }

                SetBkMode(drawDc, FieldAt<int>(this, 0x268));
                DrawTextA(drawDc, textBuffer, -1, &mainRect, drawFormat);

                const unsigned int releaseAddress = g_zVideo_pfnImageReleaseSurface;
                ReleaseSurfaceFn releaseSurface =
                    (ReleaseSurfaceFn)(releaseAddress);
                if (IsCallableProviderAddress(releaseAddress)) {
                    releaseSurface(textPick, drawDc);
                }
            }

            TEXTMETRICA metrics = {0};
            if (GetTextMetricsA(measureDc, &metrics) != 0) {
                if (FieldAt<unsigned int>(this, 0x14c) != FieldAt<unsigned int>(this, 0x150)) {
                    const unsigned short sourceWhite =
                        (unsigned short)(zVid_PackColorRGB(0xff, 0xff, 0xff));
                    unsigned short *pixel = (unsigned short *)(textPick->pixels);
                    {
                    for (int row = 0; row < textPick->height; ++row) {
                        const int lineSpan = metrics.tmHeight + metrics.tmExternalLeading;
                        const int rowPhase =
                            (FieldAt<unsigned int>(this, 0x264) != 0
                                 ? row + FieldAt<int>(this, 0x2a0)
                                 : row) %
                            lineSpan;
                        const float blend =
                            (float)(rowPhase - metrics.tmInternalLeading) /
                            (float)(metrics.tmAscent - metrics.tmInternalLeading);
                        const unsigned int blendedColor = HudUiFlashPanel::ComputeFlashBlendColor(
                            FieldAt<unsigned int>(this, 0x14c),
                            FieldAt<unsigned int>(this, 0x150), blend);
                        const unsigned short packedColor =
                            (unsigned short)(zVid_PackColorRGB(
                            (unsigned char)(blendedColor & 0xffu),
                            (unsigned char)((blendedColor >> 8) & 0xffu),
                            (unsigned char)((blendedColor >> 16) & 0xffu)));

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

                FieldAt<int>(this, 0x274) = (int)(metrics.tmExternalLeading);
            }
        }
    }

    DeleteDC(measureDc);
    FieldAt<unsigned int>(this, 0x270) = 0;
}

// Reimplements 0x4bb2a0: HudUiPanel::UpdateTextBoundsFromContent
void RECOIL_THISCALL HudUiPanel::UpdateTextBoundsFromContent() {
    HudUiElement *const element = (HudUiElement *)(this);
    char *const textBuffer = &FieldAt<char>(this, 0x34);
    const int textLength = (int)(strlen(textBuffer));

    if (FieldAt<unsigned int>(this, 0x278) != 0) {
        element->clipRect.left = element->x;
        element->clipRect.top = element->y;
        element->clipRect.right = element->x + FieldAt<int>(this, 0x284);
        element->clipRect.bottom = element->y + FieldAt<int>(this, 0x288);
        return;
    }

    HDC hdc = CreateCompatibleDC(0);
    if (hdc == 0) {
        return;
    }

    SelectObject(hdc, hFont);
    SIZE textSize = {0};
    if (GetTextExtentPoint32A(hdc, textBuffer, textLength, &textSize) != 0) {
        int left;
        const int alignMode = FieldAt<int>(this, 0x144);
        if (alignMode == 1) {
            if (FieldAt<unsigned int>(this, 0x270) != 0) {
                typedef void (RECOIL_THISCALL *RebuildFn)(HudUiPanel * self);
                const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(this);
                ((RebuildFn)(ftable->slots[36]))(this);
            }

            const int textWidth = FieldAt<int>(this, 0x25c);
            left = element->clipRect.left +
                   ((element->clipRect.right - element->clipRect.left) / 2) - (textWidth / 2);
        } else if (alignMode == 0) {
            left = element->clipRect.left;
        } else {
            if (FieldAt<unsigned int>(this, 0x270) != 0) {
                typedef void (RECOIL_THISCALL *RebuildFn)(HudUiPanel * self);
                const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(this);
                ((RebuildFn)(ftable->slots[36]))(this);
            }

            left = element->clipRect.right - FieldAt<int>(this, 0x25c);
        }

        element->clipRect.left = left;
        element->clipRect.right = left + textSize.cx;
        element->clipRect.bottom = element->clipRect.top + textSize.cy;

        if (FieldAt<unsigned int>(this, 0x264) != 0) {
            element->clipRect.bottom += abs(FieldAt<int>(this, 0x2a0));
            element->clipRect.right += abs(FieldAt<int>(this, 0x29c));
        }
    }

    DeleteDC(hdc);
}

// Reimplements 0x4bb1c0: HudUiPanel::MeasureTextPrefixRect
RECOIL_NOINLINE int RECOIL_THISCALL
HudUiPanel::MeasureTextPrefixRect(int maxChars, RECT *outRect) {
    int result = 0;
    HDC hdc = CreateCompatibleDC(0);
    if (hdc == 0) {
        return 0;
    }

    SelectObject(hdc, hFont);
    if (maxChars > 0) {
        char *const textCopy = _strdup(&FieldAt<char>(this, 0x34));
        if (maxChars <= (int)(strlen(textCopy))) {
            textCopy[maxChars] = '\0';
            if (DrawTextA(hdc, textCopy, -1, outRect, DT_CALCRECT) != 0) {
                result = 1;
            }
        }

        free(textCopy);
        DeleteDC(hdc);
        return result;
    }

    if (DrawTextA(hdc, "W", -1, outRect, DT_CALCRECT) != 0) {
        result = 1;
        outRect->right = outRect->left;
    }

    DeleteDC(hdc);
    return result;
}

// Reimplements 0x4bb710: HudUiPanel::QueryTextHeight
RECOIL_NOINLINE int RECOIL_THISCALL HudUiPanel::QueryTextHeight() {
    if (FieldAt<unsigned int>(this, 0x270) != 0) {
        typedef void (RECOIL_THISCALL *RebuildFn)(HudUiPanel * self);
        const HudUiPanel_FTable *const ftable = *(const HudUiPanel_FTable *const *)(this);
        ((RebuildFn)(ftable->slots[36]))(this);
    }

    return FieldAt<int>(this, 0x260) - FieldAt<int>(this, 0x274);
}

// Reimplements 0x40fac0: HudUiPanelSimple::Constructor
HudUiPanelSimple *RECOIL_THISCALL HudUiPanelSimple::Constructor(const char *text,
                                                                int initX,
                                                                int initY) {
    HudUiPanel *const panel = (HudUiPanel *)(this);
    panel->ConstructorDefault(text, initX, initY);
    FieldAt<const void *>(this, 0x00) = &g_HudUiPanelSimple_FTable;
    FieldAt<unsigned int>(this, 0x14c) = 0x0020bf40;
    FieldAt<unsigned int>(this, 0x150) = 0x0020bf40;
    FieldAt<unsigned int>(this, 0x270) = 1;
    panel->SetFont("Arial", 0x0a, 0x1f4, 6, 0, 0, 2);
    FieldAt<int>(this, 0x29c) = -1;
    FieldAt<int>(this, 0x2a0) = -1;
    FieldAt<unsigned int>(this, 0x264) = 1;
    return this;
}

// Reimplements 0x40fab0: HudUiPanelSimple::ConstructorDefaultThunk
HudUiPanelSimple *RECOIL_THISCALL HudUiPanelSimple::ConstructorDefaultThunk() {
    return Constructor(0, 0, 0);
}

// Reimplements 0x40ef00: HudUiTimerPanel::SetTimeSeconds
void RECOIL_THISCALL HudUiTimerPanel::SetTimeSeconds(int hours, int minutes,
                                                     int seconds) {
    HudUiPanel *const panel = (HudUiPanel *)(this);
    if (hours >= 0 && minutes >= 0 && seconds >= 0) {
        panel->SetTextFmt("%02d:%02d:%02d", hours, minutes, seconds);
    } else {
        panel->SetTextFmt("00:00:00");
    }

    panel->UpdateTextBoundsFromContent();
}

// Reimplements 0x40ee60: HudUiTimerPanel::UpdateHMSFromSeconds
void RECOIL_THISCALL HudUiTimerPanel::UpdateHMSFromSeconds(float seconds) {
    FieldAt<float>(this, 0x2a4) = seconds;

    const int hours = (int)(floor(seconds * 0.000277777785f));
    float remaining = seconds - (float)(hours) * 3600.0f;
    const int minutes = (int)(floor(remaining * 0.0166666675f));
    remaining = (float)(remaining);
    const int displaySeconds =
        (int)(floor(remaining - (float)(minutes) * 60.0f));
    SetTimeSeconds(hours, minutes, displaySeconds);
}

// Reimplements 0x40eca0: HudUiTimerPanel::SetRunning
void RECOIL_FASTCALL HudUiTimerPanel::SetRunning(int running) {
    FieldAt<int>(g_HudUiMgrTimerPanel, 0x2a8) = running == 0 ? 1 : 0;
}

// Reimplements 0x40ecc0: HudUiTimerPanel::SetElapsedSeconds
void RECOIL_STDCALL HudUiTimerPanel::SetElapsedSeconds(float seconds) {
    FieldAt<float>(g_HudUiMgrTimerPanel, 0x2a4) = seconds;
}

// Reimplements 0x40ece0: HudUiTimerPanel::SetSeconds
void RECOIL_STDCALL HudUiTimerPanel::SetSeconds(float elapsedSeconds, float secondsStep) {
    FieldAt<int>(g_HudUiMgrTimerPanel, 0x2ac) = (int)(secondsStep);
    g_HudUiMgrTimerPanel->UpdateHMSFromSeconds(elapsedSeconds);
}

// Reimplements 0x40ed10: HudUiTimerPanel::GetSeconds
float RECOIL_CDECL HudUiTimerPanel::GetSeconds() {
    return FieldAt<float>(g_HudUiMgrTimerPanel, 0x2a4);
}

// Reimplements 0x40ed20: HudUiTimerPanel::Update
void RECOIL_THISCALL HudUiTimerPanel::Update(float deltaSeconds) {
    if (FieldAt<int>(this, 0x2a8) == 0) {
        const float frameDelta =
            zOpt::GetNetworkEnabled() != 0 ? g_Time_UnscaledDeltaTimeSec : g_FrameDeltaTimeSec;
        const float elapsedSeconds =
            FieldAt<float>(this, 0x2a4) +
            (float)(FieldAt<int>(this, 0x2ac)) * frameDelta;
        FieldAt<float>(this, 0x2a4) = elapsedSeconds;
        UpdateHMSFromSeconds(elapsedSeconds);
    }

    ((HudUiElement *)(this))->Update(deltaSeconds);
}

// Reimplements 0x40fbb0: HudUiTimerPanel::ZarReadTimerData
void RECOIL_STDCALL HudUiTimerPanel::ZarReadTimerData(const float *buffer, int byteCount,
                                                      HudUiTimerPanel *userData) {
    (void)byteCount;

    userData->UpdateHMSFromSeconds(*buffer);
    HudUiMgrObjective::Begin();
}

// Reimplements 0x40fb90: HudUiTimerPanel::ZarWriteTimerDataCallback
void RECOIL_FASTCALL HudUiTimerPanel::ZarWriteTimerDataCallback(zZbdSectionCallbackCtx *sectionCtx,
                                                                HudUiTimerPanel *userData) {
    const float elapsedSeconds = FieldAt<float>(userData, 0x2a4);
    zUtil_ZAR::WriteSectionBlob(sectionCtx, "TimerData", &elapsedSeconds, sizeof(elapsedSeconds));
}

// Reimplements 0x40ed80: HudUiTimerPanel::ConstructorDefault
HudUiTimerPanel *RECOIL_THISCALL HudUiTimerPanel::ConstructorDefault() {
    HudUiPanel *const panel = (HudUiPanel *)(this);
    HudUiElement *const element = (HudUiElement *)(this);

    panel->ConstructorDefault(0, 0, 0);
    FieldAt<const void *>(this, 0x00) = &g_HudUiPanelSimple_FTable;
    FieldAt<unsigned int>(this, 0x14c) = 0x0020bf40;
    FieldAt<unsigned int>(this, 0x150) = 0x0020bf40;
    FieldAt<unsigned int>(this, 0x270) = 1;
    panel->SetFont("Arial", 0x0a, 0x1f4, 6, 0, 0, 2);
    FieldAt<unsigned int>(this, 0x264) = 1;
    FieldAt<int>(this, 0x29c) = -1;
    FieldAt<int>(this, 0x2a0) = -1;

    FieldAt<const void *>(this, 0x00) = &g_HudUiTimerPanel_FTable;
    FieldAt<int>(this, 0x2ac) = 1;
    SetTimeSeconds(0, 0, 0);
    FieldAt<int>(this, 0x2a8) = 1;
    FieldAt<float>(this, 0x2a4) = 0.0f;
    element->SetVisible(1);
    g_HudUiMgr.AddChild(element);
    return this;
}

// Reimplements 0x40dbf0: HudUiCounterTextPanel::Constructor
HudUiCounterTextPanel *RECOIL_THISCALL HudUiCounterTextPanel::Constructor() {
    HudUiPanel *const panel = (HudUiPanel *)(this);
    HudUiElement *const element = (HudUiElement *)(this);

    panel->ConstructorDefault(0, 0, 0);
    FieldAt<const void *>(this, 0x00) = &g_HudUiPanelSimple_FTable;
    FieldAt<unsigned int>(this, 0x14c) = 0x0020bf40;
    FieldAt<unsigned int>(this, 0x150) = 0x0020bf40;
    FieldAt<unsigned int>(this, 0x270) = 1;
    panel->SetFont("Arial", 0x0a, 0x1f4, 6, 0, 0, 2);
    FieldAt<unsigned int>(this, 0x264) = 1;
    FieldAt<int>(this, 0x29c) = -1;
    FieldAt<int>(this, 0x2a0) = -1;

    FieldAt<const void *>(this, 0x00) = &g_HudUiCounterTextPanel_FTable;
    panel->SetTextFmt("%d", 0);
    panel->UpdateTextBoundsFromContent();
    element->SetVisible(1);
    g_HudUiMgr.AddChild(element);
    return this;
}

// Reimplements 0x40dcd0: HudUiTriplet::Constructor
HudUiTriplet *RECOIL_THISCALL HudUiTriplet::Constructor() {
    base.ConstructorDefault();
    entries.rowInitFlag = 0;
    entries.begin = 0;
    entries.end = 0;
    entries.cap = 0;
    base.vptr = (const HudUiContainer_FTable *)(&g_HudUiTriplet_FTable);
    lapsColumnOffsetX = 0x23;
    killsColumnOffsetX = 0x46;
    fontSize = 8;
    fontWeight = 6;

    {
        int headerIndex;
        for (headerIndex = 0; headerIndex < 3; ++headerIndex) {
            HudUiPanel *header = NewSimplePanel(fontSize, fontWeight);
            headerPanels[headerIndex] = header;
            base.AddChild((HudUiElement *)(header));
        }
    }

    FieldAt<int>(headerPanels[0], 0x144) = 2;
    FieldAt<int>(headerPanels[1], 0x144) = 1;
    FieldAt<int>(headerPanels[2], 0x144) = 1;
    headerPanels[0]->SetTextFmt("Player");
    headerPanels[1]->SetTextFmt("Laps");
    headerPanels[2]->SetTextFmt("Kills");

    HudUiPanel **rowCell = rowCells;
    {
        int row;
        for (row = 0; row < 8; ++row) {
            int column;
            for (column = 0; column < 3; ++column) {
                *rowCell = NewSimplePanel(fontSize, fontWeight);
                base.AddChild((HudUiElement *)(*rowCell));
                ++rowCell;
            }

            FieldAt<int>(rowCells[row * 3 + 1], 0x144) = 1;
            FieldAt<int>(rowCells[row * 3 + 2], 0x144) = 1;
        }
    }

    base.SetEnabled(1);
    return this;
}

// Reimplements 0x40e070: HudUiTriplet::DestructorCore
void RECOIL_THISCALL HudUiTriplet::DestructorCore() {
    base.vptr = (const HudUiContainer_FTable *)(&g_HudUiTriplet_FTable);

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

    base.DestructorCore();
}

// Reimplements 0x40e140: HudUiTriplet::RebuildDisplay (D:\Proj\Battlesport\HudUiTriplet.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiTriplet::RebuildDisplay() {
    HudUiTripletInsertionSort(entries.begin, entries.end);

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
                HudUiTripletPrepareCell(this, cells[column], entry->playerColorPackedRgb);
                if (column != 2 || entry->lapCount >= 0) {
                    HudUiTripletSetPanelVisible(cells[column], 1);
                }
            }
        }

        const int y = baseY + (int)(rowIndex + 1) * rowPitchY;
        ((HudUiElement *)(nameCell))->SetPos(baseX, y);
        ((HudUiElement *)(lapsCell))->SetPos(baseX + lapsColumnOffsetX, y);
        ((HudUiElement *)(killsCell))->SetPos(baseX + killsColumnOffsetX, y);

        nameCell->SetTextFmt("%s", entry->displayName);
        if (g_HudSensorTracker.raceCheckpointMode != 0) {
            lapsCell->SetTextFmt("%d", entry->lapCount);
            killsCell->SetTextFmt("%d", entry->score);
        } else {
            lapsCell->SetTextFmt("%d", entry->score);
            HudUiTripletSetPanelVisible(killsCell, 0);
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
                HudUiTripletSetPanelVisible(cell, 0);
            }
        }
    }

    if (entryCount == 0) {
        return;
    }

    ((HudUiElement *)(headerPanels[0]))->SetPos(baseX, baseY);
    ((HudUiElement *)(headerPanels[1]))->SetPos(baseX + lapsColumnOffsetX, baseY);
    ((HudUiElement *)(headerPanels[2]))->SetPos(baseX + killsColumnOffsetX, baseY);

    {
        int headerIndex;
        for (headerIndex = 0; headerIndex < 3; ++headerIndex) {
            headerPanels[headerIndex]->SetFont("Arial", fontSize, 0x1f4, fontWeight, 0, 0, 2);
        }
    }

    HudUiTripletSetPanelVisible(headerPanels[0], 1);
    if (g_HudSensorTracker.raceCheckpointMode != 0) {
        headerPanels[1]->SetTextFmt("%s(%d)", zLoc::GetMessageString(0x113),
                                    g_HudSensorTracker.runtimeGoalValue);
        headerPanels[2]->SetTextFmt("%s", zLoc::GetMessageString(0x114));
        HudUiTripletSetPanelVisible(headerPanels[1], 1);
        HudUiTripletSetPanelVisible(headerPanels[2], 1);
    } else {
        headerPanels[1]->SetTextFmt("%s(%d)", zLoc::GetMessageString(0x114),
                                    g_HudSensorTracker.runtimeGoalValue);
        HudUiTripletSetPanelVisible(headerPanels[1], 1);
        HudUiTripletSetPanelVisible(headerPanels[2], 0);
    }
}

// Reimplements 0x40e590: HudUiTriplet::AddEntry (D:\Proj\Battlesport\HudUiTriplet.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiTriplet::AddEntry(GameNetPlayerRow *entryData) {
    HudUiScoreboardEntry sourceValue = {0};
    strncpy(sourceValue.displayName, entryData->displayName, 0x3f);
    sourceValue.playerKey = entryData->playerKey;
    sourceValue.playerColorPackedRgb = entryData->playerColorPackedRgb;
    sourceValue.score = 0;
    sourceValue.lapCount = 0;

    const size_t count = HudUiTripletEntryCount(entries);
    HudUiTripletEnsureCapacity(entries, count + 1);
    HudUiTripletEntries::FillN(entries.end, 1, &sourceValue);
    ++entries.end;
    RebuildDisplay();
}

// Reimplements 0x40e800: HudUiTriplet::UpdateEntryData (D:\Proj\Battlesport\HudUiTriplet.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiTriplet::UpdateEntryData(GameNetPlayerRow *entryData) {
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
RECOIL_NOINLINE void RECOIL_THISCALL HudUiTriplet::RemoveEntry(GameNetPlayerRow *entryKey) {
    HudUiScoreboardEntry *entry = entries.begin;
    for (int i = 0; entry != entries.end && i < 8; ++i) {
        if (entry->playerKey == entryKey->playerKey) {
            HudUiScoreboardEntry *const next = entry + 1;
            if (next != entries.end) {
                memmove(entry, next,
                             (size_t)(entries.end - next) *
                                 sizeof(HudUiScoreboardEntry));
            }

            --entries.end;
            break;
        }

        ++entry;
    }

    RebuildDisplay();
}

// Reimplements 0x4bd160: HudUiTextStack4::PushLine (D:\Proj\Battlesport\HudUiTextStack4.cpp)
HudUiPanel *RECOIL_THISCALL HudUiTextStack4::PushLine(const char *message, float duration) {
    HudUiVirtualSetContainerEnabled(&base, 1);

    HudUiPanel *const firstLine = TextStackLineAt(this, 0);
    if ((((HudUiElement *)(firstLine))->flags & 0x10u) == 0 &&
        strcmp(message, firstLine->GetLastTextPtr()) != 0) {
        {
        for (int sourceIndex = 2; sourceIndex >= 0; --sourceIndex) {
            HudUiPanel *const source = TextStackLineAt(this, sourceIndex);
            HudUiPanel *const dest = TextStackLineAt(this, sourceIndex + 1);
            HudUiElement *const sourceElement = (HudUiElement *)(source);

            if ((sourceElement->flags & 0x10u) == 0) {
                HudUiVirtualSetVisibleRequired(source, 0);
                ((HudUiElement *)(dest))->SetTimer(FieldAt<float>(source, 0x10));
                HudUiPanelVirtualSetTextFmtRequired(dest, source->GetLastTextPtr());
                FieldAt<unsigned int>(dest, 0x14c) = FieldAt<unsigned int>(source, 0x14c);
                FieldAt<unsigned int>(dest, 0x150) = FieldAt<unsigned int>(source, 0x150);
                FieldAt<unsigned int>(dest, 0x270) = 1;
                HudUiVirtualSetVisibleRequired(dest, 1);
            }
        }
        }
    }

    ((HudUiElement *)(firstLine))->SetTimer(duration);
    firstLine->SetTextFmt("%s", message);
    HudUiVirtualSetVisibleRequired(firstLine, 1);
    return firstLine;
}

// Reimplements 0x4bd470: zTimedTask::RemoveFromActiveList (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zTimedTask::RemoveFromActiveList() {
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
RECOIL_NOINLINE void RECOIL_THISCALL zTimedTask::RunImmediateAction() {
    switch (kind) {
    case 1:
        if (actionArg2 != 0) {
            zVid_Image::BlitToActiveTarget((zVidImagePartial *)(actionArg2),
                                           actionArg0, actionArg1,
                                           (unsigned short)(actionArg3),
                                           (zVidRect32 *)(actionArg4));
        }
        break;

    case 2:
        zRndr_DrawImmediateLine(actionArg0, actionArg1, actionArg2, actionArg3, actionArg4);
        break;

    case 3:
        zRndr_RasterizePoly((zVec3 *)(&actionArg0), rasterVertexCount,
                            rasterDrawParam);
        break;

    case 4: {
        const char *text = (const char *)(&actionArg2) + 2;
        if (*text != '\0') {
            zImage_Font::BlitStringToActiveTarget(text, (short)(actionArg0),
                                                  (short)(actionArg1),
                                                  (short)(actionArg2));
        }
        break;
    }

    case 5: {
        const char *text = (const char *)(actionArg3);
        if (text != 0 && *text != '\0') {
            zImage_Font::BlitStringToActiveTarget(text, (short)(actionArg0),
                                                  (short)(actionArg1),
                                                  (short)(actionArg2));
        }
        break;
    }

    case 6:
        zRndr_SpanOcclusion_TestSample(actionArg0, actionArg1, actionArg2);
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

        if (HudLineClip::ClipSegmentToCurrentBounds(&point0, &point1, &point0Clipped,
                                                    &point1Clipped) != 0) {
            zRndr_DrawImmediateLine((int)(point0.x),
                                    (int)(point0.y),
                                    (int)(point1.x),
                                    (int)(point1.y), actionArg4);
        }
        break;
    }

    case 8:
        zRndr_DrawClippedImmediateLineStrip(
            (const zRndr_LinePoint2I *)(&actionArg0), alphaPointCount - 1,
            (void *)(alpha255), alphaVariantIndex);
        break;

    default:
        break;
    }
}

// Reimplements 0x4bd660: zTimedTask::TickActiveList (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_CDECL zTimedTask::TickActiveList() {
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
RECOIL_NOINLINE void RECOIL_FASTCALL SetInvalidateMode(int mode) {
    g_HudUi_InvalidateMask = mode != 0 ? 0x0c : 0x04;
}

// Reimplements 0x438350: HudUi::ShowMessageBox (D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ShowMessageBox(const char *messageText,
                                                   const char *titleText,
                                                   void *modalContext) {
    HudUiMessageBoxDialog dialog;
    dialog.Constructor("dialog.zrd", "MESSAGEBOX");
    const int result = dialog.RunModal(messageText, titleText, modalContext, -1.0f);
    dialog.Destructor();
    return result;
}

// Reimplements 0x426150: HudUi::HandleHotkeyCommand (D:\Proj\Battlesport\hudui.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL HandleHotkeyCommand(int commandId) {
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
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x24c), 5.0f);
            zOpt::SetThrottleMode(1);
        } else {
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x24d), 5.0f);
            zOpt::SetThrottleMode(0);
        }
        return;
    case 44:
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiMgrObjective::Show(0, "Message", zLoc::GetMessageString(0x86), 2.0f);
        } else if (HudUiMainMenuDialog::CanLoadGame() != 0) {
            RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_QUICKLOAD);
        } else {
            HudUiMgrObjective::Show(0, "Message", zLoc::GetMessageString(0x87), 2.0f);
        }
        return;
    case 45:
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiMgrObjective::Show(0, "Message", zLoc::GetMessageString(0x85), 2.0f);
        } else if (HudUiMainMenuDialog::CanSaveGame() != 0) {
            RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
                RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED);
        } else {
            HudUiMgrObjective::Show(0, "Message", zLoc::GetMessageString(0x82), 2.0f);
        }
        return;
    default:
        return;
    }
}

// Reimplements 0x42bf40: HudUi::PlayPowerupSfx
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL PlayPowerupSfx(int shouldPlay) {
    zSndSample *powerupSample = g_HudUi_PowerupSample;
    if ((g_HudUi_PowerupSampleInitFlags & 1) == 0) {
        g_HudUi_PowerupSampleInitFlags =
            (unsigned char)(g_HudUi_PowerupSampleInitFlags | 1);
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
void RECOIL_FASTCALL ShowTopMessageLine(const char *message, float duration) {
    HudUiTextStack4 *const topStack = g_HudUiTopMessageStack;
    if (topStack->base.enabled != 0) {
        topStack->PushLine(message, duration);
    }
}

// Reimplements 0x4138f0: HudUi::ShowChatLine (D:\Proj\Battlesport\hud.cpp)
void RECOIL_FASTCALL ShowChatLine(const char *message, float duration) {
    HudUiTextStack4 *const chatStack = g_HudUiChatMessageStack;
    if (chatStack->base.enabled != 0) {
        chatStack->PushLine(message, duration);
    }
}

// Reimplements 0x4143b0: HudUi::RefreshScoreboardEntryRow (D:\Proj\Battlesport\HudUi.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RefreshScoreboardEntryRow(GameNetPlayerRow *entryData) {
    g_HudUiMgrStatsList->triplet->UpdateEntryData(entryData);
}

// Reimplements 0x4143c0: HudUi::RemoveScoreboardEntryRow (D:\Proj\Battlesport\HudUi.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RemoveScoreboardEntryRow(GameNetPlayerRow *entryKey) {
    g_HudUiMgrStatsList->triplet->RemoveEntry(entryKey);
}

// Reimplements 0x4bd280: HudUi::PushTopMessageLine (D:\Proj\Battlesport\hud.cpp)
void RECOIL_FASTCALL PushTopMessageLine(const char *message, float duration) {
    g_HudUiTopMessageStack->PushLine(message, duration);
}
} // namespace HudUi

// Reimplements 0x4bd3d0: HudUiTextStack4::SetTextColors
void RECOIL_THISCALL HudUiTextStack4::SetTextColors(unsigned int color0, unsigned int color1) {
    {
    for (int index = 3; index >= 0; --index) {
        HudUiPanel *const panel = TextStackLineAt(this, index);
        FieldAt<unsigned int>(panel, 0x14c) = color0;
        FieldAt<unsigned int>(panel, 0x150) = color1;
        FieldAt<unsigned int>(panel, 0x270) = 1;
    }
    }
}

// Reimplements 0x4bd2a0: HudUiTextStack4::Clear
void RECOIL_THISCALL HudUiTextStack4::Clear() {
    HudUiPanel *panel = TextStackLineAt(this, 0);
    {
    for (int count = 4; count != 0; --count) {
        HudUiVirtualSetTextFmtEmpty(panel);
        HudUiVirtualSetVisibleRequired(panel, 0);
        panel = (HudUiPanel *)((unsigned char *)(panel) + 0x2a4);
    }
    }
}

// Reimplements 0x4bd110: HudUiTextStack4::SetFontAll
// (D:\Proj\Battlesport\HudUiTextStack4.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiTextStack4::SetFontAll(const char *faceName, int height, int weight,
                            int width) {
    typedef void (RECOIL_THISCALL *SetFontFn)(HudUiPanel * self, const char *faceName,
                                              int height, int weight,
                                              int width, int italic,
                                              int charSet,
                                              int pitchAndFamily);

    {
    for (int index = 3; index >= 0; --index) {
        HudUiPanel *const panel = TextStackLineAt(this, index);
        const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
        ((SetFontFn)(ftable->slots[0x80 / 4]))(panel, faceName, height, weight,
                                                             width, 0, 0, 2);
    }
    }
}

// Reimplements 0x4bd410: HudUiTextStack4::SetXAll
void RECOIL_THISCALL HudUiTextStack4::SetXAll(int newX) {
    typedef void (RECOIL_THISCALL *SetXFn)(HudUiPanel * self, int x);

    {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAt(this, index);
        const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
        ((SetXFn)(ftable->slots[4]))(panel, newX);
    }
    }
}

// Reimplements 0x4bd440: HudUiTextStack4::SetYDescending
void RECOIL_THISCALL HudUiTextStack4::SetYDescending(int yStart) {
    typedef void (RECOIL_THISCALL *SetYFn)(HudUiPanel * self, int y);

    int y = yStart;
    {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAt(this, index);
        const HudUiPanel_FTable *const ftable = (const HudUiPanel_FTable *)(panel->vtbl);
        ((SetYFn)(ftable->slots[5]))(panel, y);
        y -= 0x12;
    }
    }
}

// Reimplements 0x4bd020: HudUiTopMessageStack::Constructor
HudUiTopMessageStack *RECOIL_THISCALL HudUiTopMessageStack::Constructor() {
    base.ConstructorDefault();

    {
    for (int index = 0; index < 4; ++index) {
        TextStackLineAt(this, index)->ConstructorDefault(0, 0, 0);
    }
    }

    base.vptr = (const HudUiContainer_FTable *)(&g_HudUiTopMessageStack_FTable);

    int y = 0x1e;
    {
    for (int index = 0; index < 4; ++index) {
        ConfigureTextStackLine(this, TextStackLineAt(this, index), y, 0x0d, 0x258, 7);
        y += 0x12;
    }
    }

    return this;
}

// Reimplements 0x40fe90: HudUiTopMessageStack::DestructorCore
void RECOIL_THISCALL HudUiTopMessageStack::DestructorCore() {
    DestroyTextStackLines(this);
}

// Reimplements 0x4bd2d0: HudUiChatMessageStack::Constructor
HudUiChatMessageStack *RECOIL_THISCALL HudUiChatMessageStack::Constructor() {
    base.ConstructorDefault();

    {
    for (int index = 0; index < 4; ++index) {
        TextStackLineAt(this, index)->ConstructorDefault(0, 0, 0);
    }
    }

    base.vptr = (const HudUiContainer_FTable *)(&g_HudUiChatMessageStack_FTable);

    int y = 0x159;
    {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAt(this, index);
        FieldAt<unsigned int>(panel, 0x14c) = 0x00996a00;
        FieldAt<unsigned int>(panel, 0x150) = 0x0095c7ff;
        FieldAt<unsigned int>(panel, 0x270) = 1;
        ConfigureTextStackLine(this, panel, y, 0x0a, 0x1f4, 6);
        y -= 0x12;
    }
    }

    return this;
}

// Reimplements 0x40fef0: HudUiChatMessageStack::DestructorCore
void RECOIL_THISCALL HudUiChatMessageStack::DestructorCore() {
    DestroyTextStackLines(this);
}

// Reimplements 0x40ef60: HudUiTimerPanelFloat::ConstructorDefault
HudUiTimerPanelFloat *RECOIL_THISCALL HudUiTimerPanelFloat::ConstructorDefault() {
    HudUiPanel *const panel = (HudUiPanel *)(this);
    HudUiElement *const element = (HudUiElement *)(this);

    panel->ConstructorDefault(" ", 3, 0x1c);
    FieldAt<const void *>(this, 0x00) = &g_HudUiPanelSimple_FTable;
    FieldAt<unsigned int>(this, 0x14c) = 0x0020bf40;
    FieldAt<unsigned int>(this, 0x150) = 0x0020bf40;
    FieldAt<unsigned int>(this, 0x270) = 1;
    panel->SetFont("Arial", 0x0a, 0x1f4, 6, 0, 0, 2);
    FieldAt<unsigned int>(this, 0x264) = 1;
    FieldAt<int>(this, 0x29c) = -1;
    FieldAt<int>(this, 0x2a0) = -1;

    const int x = element->x;
    const int y = element->y;
    element->clipRect.left = x;
    element->clipRect.top = y;
    element->clipRect.right = x + 0x3c;

    FieldAt<const void *>(this, 0x00) = &g_HudUiTimerPanelFloat_FTable;
    FieldAt<float>(this, 0x2ac) = 0.0f;
    FieldAt<float>(this, 0x2a8) = 0.0f;
    FieldAt<float>(this, 0x2a4) = 0.0f;
    element->clipRect.bottom = y + 0x0f;
    element->SetVisible(0);
    return this;
}
