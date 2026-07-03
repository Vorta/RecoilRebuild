#pragma once

#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

/**
 * Authored Recoil dialog reconstructed over imported MFC42 CDialog and
 * combo-box controls. MFC control behavior is provided by MFC42.
 */
struct AiPropertyDlg : CDialog {
    LRESULT m_selectedPropertyIndex;
    LRESULT m_selectedBehaviorIndex;
    CComboBox m_behaviorCombo;
    CComboBox m_propertyCombo;
    unsigned char m_unknown0e8[0x20];

    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    const AFX_MSGMAP * GetMessageMap() const;
    void OnDestroy();
    void OnSelChange();
    void UpdatePropertyLabels();
};

RECOIL_STATIC_ASSERT(sizeof(AiPropertyDlg) == 0x108);
RECOIL_STATIC_ASSERT(
    offsetof(
        AiPropertyDlg,
        m_selectedPropertyIndex
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AiPropertyDlg,
        m_selectedBehaviorIndex
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AiPropertyDlg,
        m_behaviorCombo
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AiPropertyDlg,
        m_propertyCombo
    ) == 0xa8
);
