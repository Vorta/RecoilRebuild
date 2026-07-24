#include "GameZRecoil/zHud/zhud_ui.h"

extern "C" unsigned int g_HudUi_InvalidateMask;

namespace {

struct FillBitmapActivationProbe : HudUiFillBitmap {
    int derivedActivationCount;

    FillBitmapActivationProbe() : derivedActivationCount(0) {
    }

    void OnActivate() {
        ++derivedActivationCount;
    }
};

bool FloatNear(float actual, float expected) {
    const float difference = actual - expected;
    return difference > -0.0001f && difference < 0.0001f;
}

} // namespace

extern "C" int zhud_fill_bitmap_update_normalized_call_contract_smoke(void) {
    __declspec(align(4)) unsigned char ownerStorage[sizeof(HudUiBackground)] = {0};
    HudUiBackground *const owner = reinterpret_cast<HudUiBackground *>(ownerStorage);
    owner->mouseState.cursorClientX = 35;

    zVidImagePartial baseImage = {0};
    baseImage.width = 100;
    zVidImagePartial fillImage = {0};
    fillImage.width = 100;
    fillImage.height = 8;

    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;

    FillBitmapActivationProbe bitmap;
    bitmap.owner = owner;
    bitmap.x = 10;
    bitmap.image = &baseImage;
    bitmap.fillImage = &fillImage;
    bitmap.flags = 0;

    bitmap.UpdateNormalizedFromCursor();

    const bool passed =
        bitmap.derivedActivationCount == 0 &&
        FloatNear(bitmap.normalizedValue, 0.25f) &&
        bitmap.fillRect.left == 0 &&
        bitmap.fillRect.top == 0 &&
        bitmap.fillRect.right == 25 &&
        bitmap.fillRect.bottom == 8 &&
        (bitmap.flags & 0x80u) != 0;

    bitmap.fillImage = 0;
    bitmap.previewImage = 0;
    bitmap.image = 0;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    return passed ? 0 : 1;
}
