#include "Battlesport/AiPropertyDlg.h"

namespace {
const int kAiPropertyDlgBehaviorComboId = 1089;
const int kAiPropertyDlgFirstPropertyLabelId = 1107;
const int kAiPropertyDlgSecondPropertyLabelId = 1108;
} // namespace

/**
 * Reimplements data 0x4db604: g_AiPropertyDlg_LabelUnused.
 * Purpose: Supplies the AI property dialog label used when a behavior mode has no property.
 */
char g_AiPropertyDlg_LabelUnused[] = "Unused";

/**
 * Reimplements data 0x4db60c: g_AiPropertyDlg_LabelMovement.
 * Purpose: Supplies the AI property dialog label for movement behavior properties.
 */
char g_AiPropertyDlg_LabelMovement[] = "Movement";

/**
 * Reimplements data 0x4db618: g_AiPropertyDlg_LabelAttackRange.
 * Purpose: Supplies the AI property dialog label for attack range properties.
 */
char g_AiPropertyDlg_LabelAttackRange[] = "Attack Range";

/**
 * Reimplements data 0x4db628: g_AiPropertyDlg_LabelMaxPursuitRange.
 * Purpose: Supplies the AI property dialog label for maximum pursuit range properties.
 */
char g_AiPropertyDlg_LabelMaxPursuitRange[] = "Max Pursuit Rng";

/**
 * Reimplements data 0x4db638: g_AiPropertyDlg_LabelMinPursuitRange.
 * Purpose: Supplies the AI property dialog label for minimum pursuit range properties.
 */
char g_AiPropertyDlg_LabelMinPursuitRange[] = "Min Pursuit Rng";

// Provider-boundary accessor for imported MFC42 CDialog metadata; this does not reimplement
// CDialog behavior.
class AiPropertyDlgCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

/**
 * MFC provider-boundary accessor for imported CDialog message-map metadata.
 * Purpose: Exposes CDialog::messageMap through the callback shape expected by the derived map.
 */
const AFX_MSGMAP *__stdcall AiPropertyDlgCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * MFC provider-boundary accessor for AiPropertyDlg's base message-map callback.
 * Purpose: Returns the provider-owned CDialog base message map for MFC dispatch chaining.
 */
const AFX_MSGMAP *__stdcall AiPropertyDlg::GetBaseMessageMapForMfc() {
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

/**
 * MFC provider-boundary message-map accessor for AiPropertyDlg.
 * Purpose: Returns the authored dialog message-map table used by MFC command routing.
 */
const AFX_MSGMAP * AiPropertyDlg::GetMessageMap() const {
    return &AiPropertyDlg::messageMap;
}

/**
 * Reimplements 0x41c0c0: AiPropertyDlg::OnDestroy (Battlesport/AiPropertyDlg.cpp).
 * Purpose: Saves combo-box selections when the AI property dialog closes and hides the cursor.
 */
void AiPropertyDlg::OnDestroy() {
    CWnd::OnDestroy();

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

/**
 * Reimplements 0x41c130: AiPropertyDlg::OnSelChange (Battlesport/AiPropertyDlg.cpp).
 * Purpose: Updates the selected AI behavior and refreshes the property labels.
 */
void AiPropertyDlg::OnSelChange() {
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

/**
 * Reimplements 0x41c170: AiPropertyDlg::UpdatePropertyLabels (Battlesport/AiPropertyDlg.cpp).
 * Purpose: Chooses the two property label strings for the currently selected AI behavior.
 */
void AiPropertyDlg::UpdatePropertyLabels() {
    CString firstLabel;
    CString secondLabel;

    switch ((unsigned int)m_selectedBehaviorIndex) {
    case 0:
        firstLabel = g_AiPropertyDlg_LabelMinPursuitRange;
        secondLabel = g_AiPropertyDlg_LabelMaxPursuitRange;
        break;

    case 1:
        firstLabel = g_AiPropertyDlg_LabelAttackRange;
        secondLabel = g_AiPropertyDlg_LabelUnused;
        break;

    case 2:
        firstLabel = g_AiPropertyDlg_LabelAttackRange;
        secondLabel = g_AiPropertyDlg_LabelMovement;
        break;

    case 3:
    case 4:
    case 5:
        firstLabel = g_AiPropertyDlg_LabelUnused;
        secondLabel = g_AiPropertyDlg_LabelUnused;
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
