#pragma once

#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

// Authored Recoil dialog reconstructed over imported MFC42 CDialog and
// combo-box controls. MFC control behavior is provided by MFC42.
struct AiPropertyDlg : CDialog {
    LRESULT m_selectedPropertyIndex;
    LRESULT m_selectedBehaviorIndex;
    unsigned char m_unknown068[0x20];
    CComboBox m_behaviorCombo;
    CComboBox m_propertyCombo;

    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    static const AFX_MSGMAP *RECOIL_STDCALL GetBaseMessageMapForMfc();
    RECOIL_NOINLINE const AFX_MSGMAP *RECOIL_THISCALL GetMessageMap() const;
    RECOIL_NOINLINE void RECOIL_THISCALL OnDestroy();
    RECOIL_NOINLINE void RECOIL_THISCALL OnSelChange();
    RECOIL_NOINLINE void RECOIL_THISCALL UpdatePropertyLabels();
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
    ) == 0x88
);
RECOIL_STATIC_ASSERT(
    offsetof(
        AiPropertyDlg,
        m_propertyCombo
    ) == 0xc8
);
