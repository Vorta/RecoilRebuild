#include "Battlesport/Mfc42Abi.h"

/**
 * Provider-boundary accessor for imported MFC42 CDialog metadata; this does
 * not reimplement CDialog behavior.
 */
class MfcThreeFloatCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

/**
 * Authored Recoil dialog reconstructed over imported MFC42 CDialog; MFC base
 * behavior is provided by MFC42.
 */
class MfcThreeFloatDialog : public CDialog {
  public:
    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];
    static const float kSpinStepPositive;
    static const float kSpinStepNegative;

    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    const AFX_MSGMAP * GetMessageMap() const;

    void OnKillFocusValue0();
    void OnKillFocusValue1();
    void OnKillFocusValue2();
    void OnDeltaposSpinValue0(
        NMHDR *notify,
        long *result
    );
    void OnDeltaposSpinValue1(
        NMHDR *notify,
        long *result
    );
    void OnDeltaposSpinValue2(
        NMHDR *notify,
        long *result
    );
    void OnMove(
        int x,
        int y
    );
    int OnCreate(
        LPCREATESTRUCT createStruct
    );

    int unknown060;
    float value0;
    float value1;
    float value2;
};

RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(
    offsetof(
        MfcThreeFloatDialog,
        value0
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        MfcThreeFloatDialog,
        value1
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        MfcThreeFloatDialog,
        value2
    ) == 0x6c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        NM_UPDOWN,
        iDelta
    ) == 0x10
);

namespace {
const unsigned int kValue0EditControlId = 0x3f1;
const unsigned int kValue1EditControlId = 0x3f2;
const unsigned int kValue2EditControlId = 0x3f3;
const unsigned int kValue0SpinControlId = 0x42d;
const unsigned int kValue1SpinControlId = 0x42e;
const unsigned int kValue2SpinControlId = 0x42f;

} // namespace

/**
 * Provider-boundary: imported MFC42 CDialog message-map accessor.
 *
 * Purpose: expose the CDialog base message map for the recovered MFC map
 * chain.
 */
const AFX_MSGMAP *__stdcall MfcThreeFloatCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Original helper evidence: no standalone retail function; used by the
 * MfcThreeFloatDialog MFC message-map data at 0x4ccb18.
 *
 * Purpose: return the provider CDialog base message map for MFC dispatch
 * chaining.
 */
const AFX_MSGMAP *__stdcall MfcThreeFloatDialog::GetBaseMessageMapForMfc() {
    return MfcThreeFloatCDialogMessageMapAccessor::GetMessageMap();
}

/**
 * Reimplements data 0x4ccb20: MfcThreeFloatDialog::messageEntries.
 *
 * Purpose: provide the terminal MFC message-map entries for the three edit
 * kill-focus handlers, three up-down delta handlers, WM_MOVE, WM_CREATE, and
 * the MFC sentinel.
 */
AFX_MSGMAP_ENTRY const MfcThreeFloatDialog::messageEntries[] = {
    {WM_COMMAND,
        EN_KILLFOCUS,
        kValue0EditControlId,
        kValue0EditControlId,
        AfxSig_vv,
        (AFX_PMSG)&MfcThreeFloatDialog::OnKillFocusValue0},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kValue1EditControlId,
        kValue1EditControlId,
        AfxSig_vv,
        (AFX_PMSG)&MfcThreeFloatDialog::OnKillFocusValue1},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kValue2EditControlId,
        kValue2EditControlId,
        AfxSig_vv,
        (AFX_PMSG)&MfcThreeFloatDialog::OnKillFocusValue2},
    {WM_NOTIFY,
        (WORD)(int)UDN_DELTAPOS,
        kValue0SpinControlId,
        kValue0SpinControlId,
        AfxSig_vNMHDRpl,
        (AFX_PMSG)(void (AFX_MSG_CALL CCmdTarget::*)(NMHDR *, LRESULT *))
            &MfcThreeFloatDialog::OnDeltaposSpinValue0},
    {WM_NOTIFY,
        (WORD)(int)UDN_DELTAPOS,
        kValue1SpinControlId,
        kValue1SpinControlId,
        AfxSig_vNMHDRpl,
        (AFX_PMSG)(void (AFX_MSG_CALL CCmdTarget::*)(NMHDR *, LRESULT *))
            &MfcThreeFloatDialog::OnDeltaposSpinValue1},
    {WM_NOTIFY,
        (WORD)(int)UDN_DELTAPOS,
        kValue2SpinControlId,
        kValue2SpinControlId,
        AfxSig_vNMHDRpl,
        (AFX_PMSG)(void (AFX_MSG_CALL CCmdTarget::*)(NMHDR *, LRESULT *))
            &MfcThreeFloatDialog::OnDeltaposSpinValue2},
    {WM_MOVE,
        0,
        0,
        0,
        AfxSig_vvii,
        (AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(int, int))
            &MfcThreeFloatDialog::OnMove},
    {WM_CREATE,
        0,
        0,
        0,
        AfxSig_is,
        (AFX_PMSG)(AFX_PMSGW)(int (AFX_MSG_CALL CWnd::*)(LPCREATESTRUCT))
            &MfcThreeFloatDialog::OnCreate},
    {0, 0, 0, 0, AfxSig_end, 0},
};

/**
 * Reimplements data 0x4ccb18: MfcThreeFloatDialog::messageMap.
 *
 * Purpose: link MfcThreeFloatDialog's message entries to the CDialog provider
 * message-map accessor used as the retail base-map callback.
 */
const AFX_MSGMAP MfcThreeFloatDialog::messageMap = {
    &MfcThreeFloatDialog::GetBaseMessageMapForMfc,
    &MfcThreeFloatDialog::messageEntries[0],
};

/**
 * Reimplements data 0x4ccbf8: MfcThreeFloatDialog::kSpinStepPositive.
 *
 * Purpose: provide the recovered positive spin delta used when the up-down
 * control reports a non-positive delta.
 */
const float MfcThreeFloatDialog::kSpinStepPositive = 0.25f;

/**
 * Reimplements data 0x4ccbfc: MfcThreeFloatDialog::kSpinStepNegative.
 *
 * Purpose: provide the recovered negative spin delta used when the up-down
 * control reports a positive delta.
 */
const float MfcThreeFloatDialog::kSpinStepNegative = -0.25f;

/**
 * Original helper evidence: no standalone retail function; used by the MFC
 * message-map vtable override for this dialog's owner.
 *
 * Purpose: return the authored dialog message-map table used by MFC command,
 * notification, and window-message dispatch.
 */
const AFX_MSGMAP * MfcThreeFloatDialog::GetMessageMap() const {
    return &MfcThreeFloatDialog::messageMap;
}

/**
 * Reimplements 0x406890: MfcThreeFloatDialog::OnKillFocusValue0
 * (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp).
 *
 * Purpose: commit edited value0 through MFC data exchange and accept the
 * dialog only when the value changed.
 */
void MfcThreeFloatDialog::OnKillFocusValue0() {
    const float oldValue = value0;
    UpdateData(TRUE);
    if (value0 != oldValue) {
        CDialog::OnOK();
    }
}

/**
 * Reimplements 0x4068c0: MfcThreeFloatDialog::OnKillFocusValue1
 * (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp).
 *
 * Purpose: commit edited value1 through MFC data exchange and accept the
 * dialog only when the value changed.
 */
void MfcThreeFloatDialog::OnKillFocusValue1() {
    const float oldValue = value1;
    UpdateData(TRUE);
    if (value1 != oldValue) {
        CDialog::OnOK();
    }
}

/**
 * Reimplements 0x4068f0: MfcThreeFloatDialog::OnKillFocusValue2
 * (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp).
 *
 * Purpose: commit edited value2 through MFC data exchange and accept the
 * dialog only when the value changed.
 */
void MfcThreeFloatDialog::OnKillFocusValue2() {
    const float oldValue = value2;
    UpdateData(TRUE);
    if (value2 != oldValue) {
        CDialog::OnOK();
    }
}

/**
 * Reimplements 0x406920: MfcThreeFloatDialog::OnDeltaposSpinValue0
 * (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp).
 *
 * Purpose: adjust value0 by the recovered 0.25 spin step, refresh dialog data,
 * accept the value, and clear the notify result.
 */
void MfcThreeFloatDialog::OnDeltaposSpinValue0(
    NMHDR *notify,
    long *result
) {
    NM_UPDOWN *const upDown = (NM_UPDOWN *)notify;
    if (upDown->iDelta > 0) {
        value0 -= kSpinStepPositive;
    } else {
        value0 -= kSpinStepNegative;
    }

    UpdateData(FALSE);
    CDialog::OnOK();
    *result = 0;
}

/**
 * Reimplements 0x406960: MfcThreeFloatDialog::OnDeltaposSpinValue1
 * (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp).
 *
 * Purpose: adjust value1 by the recovered 0.25 spin step, refresh dialog data,
 * accept the value, and clear the notify result.
 */
void MfcThreeFloatDialog::OnDeltaposSpinValue1(
    NMHDR *notify,
    long *result
) {
    NM_UPDOWN *const upDown = (NM_UPDOWN *)notify;
    if (upDown->iDelta > 0) {
        value1 -= kSpinStepPositive;
    } else {
        value1 -= kSpinStepNegative;
    }

    UpdateData(FALSE);
    CDialog::OnOK();
    *result = 0;
}

/**
 * Reimplements 0x4069a0: MfcThreeFloatDialog::OnDeltaposSpinValue2
 * (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp).
 *
 * Purpose: adjust value2 by the recovered 0.25 spin step, refresh dialog data,
 * accept the value, and clear the notify result.
 */
void MfcThreeFloatDialog::OnDeltaposSpinValue2(
    NMHDR *notify,
    long *result
) {
    NM_UPDOWN *const upDown = (NM_UPDOWN *)notify;
    if (upDown->iDelta > 0) {
        value2 -= kSpinStepPositive;
    } else {
        value2 -= kSpinStepNegative;
    }

    UpdateData(FALSE);
    CDialog::OnOK();
    *result = 0;
}

/**
 * Reimplements 0x4069e0: MfcThreeFloatDialog::OnMove
 * (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp).
 *
 * Purpose: dispatch default MFC move handling for the dialog.
 */
void MfcThreeFloatDialog::OnMove(
    int,
    int
) {
    Default();
}

/**
 * Reimplements 0x4069f0: MfcThreeFloatDialog::OnCreate
 * (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp).
 *
 * Purpose: preserve the dialog creation result rule from MFC default handling,
 * returning -1 only when the provider default handler returns -1.
 */
int MfcThreeFloatDialog::OnCreate(
    LPCREATESTRUCT
) {
    return Default() == -1 ? -1 : 0;
}
