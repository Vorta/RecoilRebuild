#include "Battlesport/AiPropertyDlg.h"

namespace {
const int kAiPropertyDlgBehaviorComboId = 1089;
const int kAiPropertyDlgFirstPropertyLabelId = 1107;
const int kAiPropertyDlgSecondPropertyLabelId = 1108;

const char kAiPropertyDlgLabelUnused[] = "Unused";
const char kAiPropertyDlgLabelAttackRange[] = "Attack Range";
const char kAiPropertyDlgLabelMovement[] = "Movement";
const char kAiPropertyDlgLabelMinPursuitRange[] = "Min Pursuit Rng";
const char kAiPropertyDlgLabelMaxPursuitRange[] = "Max Pursuit Rng";
} // namespace

// Access shim for imported MFC42 CDialog metadata; this does not reimplement
// CDialog behavior.
class AiPropertyDlgCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *RECOIL_STDCALL GetMessageMap();
};

// Access shim for imported MFC42 protected window members; this does not
// reimplement provider behavior.
class AiPropertyDlgMfcWndAccess : public CWnd {
  public:
    void CallOnDestroy();
};

const AFX_MSGMAP *RECOIL_STDCALL AiPropertyDlgCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

void AiPropertyDlgMfcWndAccess::CallOnDestroy() {
    CWnd::OnDestroy();
}

const AFX_MSGMAP *RECOIL_STDCALL AiPropertyDlg::GetBaseMessageMapForMfc() {
    return AiPropertyDlgCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const AiPropertyDlg::messageEntries[] = {
    {WM_DESTROY, 0, 0, 0, 12, (AFX_PMSG)&AiPropertyDlg::OnDestroy},
    {WM_COMMAND,
        CBN_SELCHANGE,
        kAiPropertyDlgBehaviorComboId,
        kAiPropertyDlgBehaviorComboId,
        12,
        (AFX_PMSG)&AiPropertyDlg::OnSelChange},
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP AiPropertyDlg::messageMap = {
    &AiPropertyDlg::GetBaseMessageMapForMfc,
    &AiPropertyDlg::messageEntries[0],
};

RECOIL_NOINLINE const AFX_MSGMAP *RECOIL_THISCALL AiPropertyDlg::GetMessageMap() const {
    return &AiPropertyDlg::messageMap;
}

// Reimplements 0x41c0c0: AiPropertyDlg::OnDestroy
// (D:\Proj\Recoil\AiPropertyDlg.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL AiPropertyDlg::OnDestroy() {
    ((AiPropertyDlgMfcWndAccess *)this)->CallOnDestroy();

    const LRESULT selectedPropertyComboIndex =
        ::SendMessageA(
            m_propertyCombo.m_hWnd,
            CB_GETCURSEL,
            0,
            0
        );
    m_selectedPropertyIndex =
        ::SendMessageA(
            m_propertyCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedPropertyComboIndex,
            0
        );

    const LRESULT selectedBehaviorComboIndex =
        ::SendMessageA(
            m_behaviorCombo.m_hWnd,
            CB_GETCURSEL,
            0,
            0
        );
    m_selectedBehaviorIndex =
        ::SendMessageA(
            m_behaviorCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedBehaviorComboIndex,
            0
        );

    ::ShowCursor(FALSE);
}

// Reimplements 0x41c130: AiPropertyDlg::OnSelChange
// (D:\Proj\Recoil\AiPropertyDlg.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL AiPropertyDlg::OnSelChange() {
    const LRESULT selectedBehaviorComboIndex =
        ::SendMessageA(
            m_behaviorCombo.m_hWnd,
            CB_GETCURSEL,
            0,
            0
        );
    m_selectedBehaviorIndex =
        ::SendMessageA(
            m_behaviorCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedBehaviorComboIndex,
            0
        );
    UpdatePropertyLabels();
}

// Reimplements 0x41c170: AiPropertyDlg::UpdatePropertyLabels
// (D:\Proj\Recoil\AiPropertyDlg.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL AiPropertyDlg::UpdatePropertyLabels() {
    CString firstLabel;
    CString secondLabel;

    switch ((unsigned int)m_selectedBehaviorIndex) {
    case 0:
        firstLabel = kAiPropertyDlgLabelMinPursuitRange;
        secondLabel = kAiPropertyDlgLabelMaxPursuitRange;
        break;

    case 1:
        firstLabel = kAiPropertyDlgLabelAttackRange;
        secondLabel = kAiPropertyDlgLabelUnused;
        break;

    case 2:
        firstLabel = kAiPropertyDlgLabelAttackRange;
        secondLabel = kAiPropertyDlgLabelMovement;
        break;

    case 3:
    case 4:
    case 5:
        firstLabel = kAiPropertyDlgLabelUnused;
        secondLabel = kAiPropertyDlgLabelUnused;
        break;

    default:
        break;
    }

    ((CWnd *)this)->SetDlgItemTextA(
        kAiPropertyDlgFirstPropertyLabelId,
        firstLabel
    );
    ((CWnd *)this)->SetDlgItemTextA(
        kAiPropertyDlgSecondPropertyLabelId,
        secondLabel
    );
}
