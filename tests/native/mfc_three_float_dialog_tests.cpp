#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include <windows.h>

class CWnd {
  public:
    int UpdateData(BOOL saveAndValidate);
    long Default();
};

class CDialog : public CWnd {
  public:
    virtual void OnOK();

    unsigned char reserved004[0x5c];
};

class MfcThreeFloatDialog : public CDialog {
  public:
    void OnKillFocusValue0();
    void OnKillFocusValue1();
    void OnKillFocusValue2();
    void OnDeltaposSpinValue0(NMHDR *notify, long *result);
    void OnDeltaposSpinValue1(NMHDR *notify, long *result);
    void OnDeltaposSpinValue2(NMHDR *notify, long *result);
    void OnDeltaposSpin2(NMHDR *notify, long *result);
    long OnMove(unsigned int packedPos);

    int unknown060;
    float value0;
    float value1;
    float value2;
};

struct TestNmUpDown {
    NMHDR hdr;
    int pos;
    int delta;
};

RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(MfcThreeFloatDialog, value0) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(MfcThreeFloatDialog, value1) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(MfcThreeFloatDialog, value2) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(TestNmUpDown, delta) == 0x10);

int g_threeFloatUpdateDataCount;
int g_threeFloatUpdateDataSaveValue[8];
int g_threeFloatOnOkCount;
long g_threeFloatDefaultReturn;
int g_threeFloatDefaultCount;
float g_threeFloatUpdateReplacement[3];
int g_threeFloatUpdateReplacementIndex;

void ResetThreeFloatDialogLog()
{
    g_threeFloatUpdateDataCount = 0;
    g_threeFloatOnOkCount = 0;
    g_threeFloatDefaultCount = 0;
    g_threeFloatDefaultReturn = 0;
    g_threeFloatUpdateReplacement[0] = 0.0f;
    g_threeFloatUpdateReplacement[1] = 0.0f;
    g_threeFloatUpdateReplacement[2] = 0.0f;
    g_threeFloatUpdateReplacementIndex = -1;
    for (int index = 0; index < 8; ++index) {
        g_threeFloatUpdateDataSaveValue[index] = -1;
    }
}

int CWnd::UpdateData(BOOL saveAndValidate)
{
    MfcThreeFloatDialog *const dialog = (MfcThreeFloatDialog *)this;
    if (g_threeFloatUpdateDataCount < 8) {
        g_threeFloatUpdateDataSaveValue[g_threeFloatUpdateDataCount] = saveAndValidate;
    }
    ++g_threeFloatUpdateDataCount;

    if (saveAndValidate != 0 && g_threeFloatUpdateReplacementIndex >= 0) {
        if (g_threeFloatUpdateReplacementIndex == 0) {
            dialog->value0 = g_threeFloatUpdateReplacement[0];
        } else if (g_threeFloatUpdateReplacementIndex == 1) {
            dialog->value1 = g_threeFloatUpdateReplacement[1];
        } else {
            dialog->value2 = g_threeFloatUpdateReplacement[2];
        }
    }

    return 1;
}

void CDialog::OnOK()
{
    ++g_threeFloatOnOkCount;
}

long CWnd::Default()
{
    ++g_threeFloatDefaultCount;
    return g_threeFloatDefaultReturn;
}

extern "C" int mfc_three_float_dialog_handlers_smoke(void)
{
    MfcThreeFloatDialog dialog = {};
    dialog.value0 = 1.0f;
    dialog.value1 = 2.0f;
    dialog.value2 = 3.0f;

    ResetThreeFloatDialogLog();
    g_threeFloatUpdateReplacementIndex = 0;
    g_threeFloatUpdateReplacement[0] = 1.0f;
    dialog.OnKillFocusValue0();
    const bool killNoChange =
        g_threeFloatUpdateDataCount == 1 && g_threeFloatUpdateDataSaveValue[0] == TRUE &&
        g_threeFloatOnOkCount == 0 && dialog.value0 == 1.0f;

    ResetThreeFloatDialogLog();
    g_threeFloatUpdateReplacementIndex = 1;
    g_threeFloatUpdateReplacement[1] = 4.5f;
    dialog.OnKillFocusValue1();
    const bool killChanged =
        g_threeFloatUpdateDataCount == 1 && g_threeFloatUpdateDataSaveValue[0] == TRUE &&
        g_threeFloatOnOkCount == 1 && dialog.value1 == 4.5f;

    TestNmUpDown upDown = {};
    long result = -1;
    upDown.delta = 1;
    ResetThreeFloatDialogLog();
    dialog.OnDeltaposSpinValue0(&upDown.hdr, &result);
    const bool spinSubtract =
        dialog.value0 == 0.75f && result == 0 && g_threeFloatUpdateDataCount == 1 &&
        g_threeFloatUpdateDataSaveValue[0] == FALSE && g_threeFloatOnOkCount == 1;

    result = -1;
    upDown.delta = 2;
    ResetThreeFloatDialogLog();
    dialog.OnDeltaposSpinValue1(&upDown.hdr, &result);
    const bool spinValue1Subtract =
        dialog.value1 == 4.25f && result == 0 && g_threeFloatUpdateDataCount == 1 &&
        g_threeFloatUpdateDataSaveValue[0] == FALSE && g_threeFloatOnOkCount == 1;

    result = -1;
    upDown.delta = 0;
    ResetThreeFloatDialogLog();
    dialog.OnDeltaposSpinValue2(&upDown.hdr, &result);
    const bool spinAdd =
        dialog.value2 == 3.25f && result == 0 && g_threeFloatUpdateDataCount == 1 &&
        g_threeFloatUpdateDataSaveValue[0] == FALSE && g_threeFloatOnOkCount == 1;

    result = 5;
    ResetThreeFloatDialogLog();
    dialog.OnDeltaposSpin2(&upDown.hdr, &result);
    const bool defaultSpin = g_threeFloatDefaultCount == 1 && result == 5;

    ResetThreeFloatDialogLog();
    g_threeFloatDefaultReturn = -1;
    const bool moveDefaultMinusOne = dialog.OnMove(0x12345678) == -1 &&
                                     g_threeFloatDefaultCount == 1;

    ResetThreeFloatDialogLog();
    g_threeFloatDefaultReturn = 7;
    const bool moveDefaultOther = dialog.OnMove(0x12345678) == 0 &&
                                  g_threeFloatDefaultCount == 1;

    return killNoChange && killChanged && spinSubtract && spinValue1Subtract && spinAdd &&
                   defaultSpin && moveDefaultMinusOne && moveDefaultOther
               ? 0
               : 1;
}
