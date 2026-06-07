#include "Battlesport/Mfc42Abi.h"

// Authored Recoil dialog reconstructed over imported MFC42 CDialog; MFC base
// behavior is not reimplemented here.
class MfcThreeFloatDialog : public CDialog {
  public:
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
    void OnDeltaposSpin2(
        NMHDR *notify,
        long *result
    );
    long OnMove(unsigned int packedPos);
    void CallBaseOnOK();

    int unknown060;
    float value0;
    float value1;
    float value2;
};

struct RecoilNmUpDown {
    NMHDR hdr;
    int pos;
    int delta;
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
        RecoilNmUpDown,
        delta
    ) == 0x10
);

namespace {
const float kSpinStep = 0.25f;

// Source-faithful helper recovered from address-backed callers in this source file.
void OnKillFocusValue(
    float *value,
    MfcThreeFloatDialog *dialog
) {
    const float oldValue = *value;
    dialog->UpdateData(TRUE);
    if (*value != oldValue) {
        dialog->CallBaseOnOK();
    }
}

// Source-faithful helper recovered from address-backed callers in this source file.
void OnDeltaposSpinValue(
    float *value,
    MfcThreeFloatDialog *dialog,
    NMHDR *notify,
    long *result
) {
    RecoilNmUpDown *const upDown = (RecoilNmUpDown *)notify;
    if (upDown->delta > 0) {
        *value -= kSpinStep;
    } else {
        *value += kSpinStep;
    }

    dialog->UpdateData(FALSE);
    dialog->CallBaseOnOK();
    *result = 0;
}
} // namespace

// Source-faithful helper recovered from address-backed callers in this source file.
void MfcThreeFloatDialog::CallBaseOnOK() {
    CDialog::OnOK();
}

// Reimplements 0x406890: MfcThreeFloatDialog::OnKillFocusValue0
// (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp)
void MfcThreeFloatDialog::OnKillFocusValue0() {
    OnKillFocusValue(
        &value0,
        this
    );
}

// Reimplements 0x4068c0: MfcThreeFloatDialog::OnKillFocusValue1
// (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp)
void MfcThreeFloatDialog::OnKillFocusValue1() {
    OnKillFocusValue(
        &value1,
        this
    );
}

// Reimplements 0x4068f0: MfcThreeFloatDialog::OnKillFocusValue2
// (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp)
void MfcThreeFloatDialog::OnKillFocusValue2() {
    OnKillFocusValue(
        &value2,
        this
    );
}

// Reimplements 0x406920: MfcThreeFloatDialog::OnDeltaposSpinValue0
// (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp)
void MfcThreeFloatDialog::OnDeltaposSpinValue0(
    NMHDR *notify,
    long *result
) {
    OnDeltaposSpinValue(
        &value0,
        this,
        notify,
        result
    );
}

// Reimplements 0x406960: MfcThreeFloatDialog::OnDeltaposSpinValue1
// (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp)
void MfcThreeFloatDialog::OnDeltaposSpinValue1(
    NMHDR *notify,
    long *result
) {
    OnDeltaposSpinValue(
        &value1,
        this,
        notify,
        result
    );
}

// Reimplements 0x4069a0: MfcThreeFloatDialog::OnDeltaposSpinValue2
// (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp)
void MfcThreeFloatDialog::OnDeltaposSpinValue2(
    NMHDR *notify,
    long *result
) {
    OnDeltaposSpinValue(
        &value2,
        this,
        notify,
        result
    );
}

// Reimplements 0x4069e0: MfcThreeFloatDialog::OnDeltaposSpin2
// (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp)
void MfcThreeFloatDialog::OnDeltaposSpin2(
    NMHDR *,
    long *
) {
    Default();
}

// Reimplements 0x4069f0: MfcThreeFloatDialog::OnMove
// (D:\Proj\Battlesport\MfcThreeFloatDialog.cpp)
long MfcThreeFloatDialog::OnMove(
    unsigned int
) {
    return Default() == -1 ? -1 : 0;
}
