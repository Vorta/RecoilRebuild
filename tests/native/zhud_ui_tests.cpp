#include "Battlesport/GameNet.h"
#include "Battlesport/Briefing.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/HudUiNetExitPanel.h"
#include "Battlesport/hud.h"
#include "Battlesport/pickup.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" std::uint32_t g_HudUi_InvalidateMask;
extern float g_HudUiLoadingCheckpointRawProgress[19];
extern float g_HudUiLoadingCheckpointProgress[19];
extern float g_HudUiLoadingCheckpointProgressScale;
extern unsigned int g_HudUiLoadingCheckpointMaxIndex;
extern unsigned int g_HudUiLoadingCheckpointCurrentIndex;
extern float g_HudUiLoadingCheckpointCurrentProgress;
extern zSndSample *g_HudUi_PowerupSample;
extern unsigned char g_HudUi_PowerupSampleInitFlags;
extern "C" int g_Hud_MapOverlayRefCount;
extern "C" int g_HudSensorTracker_ObjectiveCommandLocked;

namespace {
template <typename T> T &TestFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<std::uint8_t *>(base) + offset);
}

std::size_t HudUiTripletEntryCountForTest(const HudUiTriplet &triplet) {
    if (triplet.entries.begin == nullptr) {
        return 0;
    }

    return static_cast<std::size_t>(triplet.entries.end - triplet.entries.begin);
}

typedef std::int32_t(__stdcall *HudUiTestBackendGetStatusFn)(void *self,
                                                             std::int32_t *status);
typedef std::int32_t(__stdcall *HudUiTestBackendSetIntFn)(void *self, std::int32_t value);
typedef std::int32_t(__stdcall *HudUiTestBackendSimpleFn)(void *self);
typedef std::int32_t(__stdcall *HudUiTestBackendPlayDirectSoundFn)(
    void *self, std::uint32_t reserved1, std::uint32_t reserved2, std::uint32_t flags);

struct HudUiTestDirectSoundBufferVTable {
    void *slots00_0c[4];
    void *GetCurrentPosition;
    void *slots14_1c[3];
    void *GetFrequency;
    HudUiTestBackendGetStatusFn GetStatus;
    void *slot28;
    void *slot2c;
    HudUiTestBackendPlayDirectSoundFn Play;
    HudUiTestBackendSetIntFn SetCurrentPosition;
    void *slot38;
    HudUiTestBackendSetIntFn SetVolume;
    HudUiTestBackendSetIntFn SetPan;
    HudUiTestBackendSetIntFn SetFrequency;
    HudUiTestBackendSimpleFn Stop;
    void *slot4c;
    void *Restore;
};

struct HudUiTestDirectSoundBuffer {
    HudUiTestDirectSoundBufferVTable *vtable;
};

int g_HudUiTestPowerupPlayCount = 0;
int g_HudUiTestPowerupStopCount = 0;
int g_HudUiTestPowerupVolumeCount = 0;
int g_HudUiTestPowerupPositionCount = 0;

std::int32_t __stdcall HudUiTestDirectSoundGetStatus(void *, std::int32_t *status) {
    *status = 0;
    return 0;
}

std::int32_t __stdcall HudUiTestDirectSoundSetInt(void *, std::int32_t) {
    return 0;
}

std::int32_t __stdcall HudUiTestDirectSoundSetVolume(void *, std::int32_t) {
    ++g_HudUiTestPowerupVolumeCount;
    return 0;
}

std::int32_t __stdcall HudUiTestDirectSoundSetCurrentPosition(void *, std::int32_t) {
    ++g_HudUiTestPowerupPositionCount;
    return 0;
}

std::int32_t __stdcall HudUiTestDirectSoundPlay(void *, std::uint32_t, std::uint32_t,
                                                std::uint32_t) {
    ++g_HudUiTestPowerupPlayCount;
    return 0;
}

std::int32_t __stdcall HudUiTestDirectSoundStop(void *) {
    ++g_HudUiTestPowerupStopCount;
    return 0;
}

void DeletePanelAllocation(HudUiPanel *panel) {
    if (panel == nullptr) {
        return;
    }

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    ::operator delete(panel);
}

HudUiPanel *TextStackLineAt(HudUiTextStack4 *stack, int index) {
    return reinterpret_cast<HudUiPanel *>(&stack->lines[index][0]);
}

void DeleteTextStackLineFonts(HudUiTextStack4 *stack) {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAt(stack, index);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }
}

void TestWriteU32(HANDLE file, std::uint32_t value) {
    DWORD written = 0;
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

void DeleteListSelectorItemArray(HudUiListSelectorItem *items) {
    if (items == nullptr) {
        return;
    }

    auto *const header = reinterpret_cast<std::uint8_t *>(items) - sizeof(std::int32_t);
    const std::int32_t count = TestFieldAt<std::int32_t>(header, 0);
    for (std::int32_t index = 0; index < count; ++index) {
        reinterpret_cast<HudUiPanel *>(&items[index])->Destructor();
    }

    ::operator delete(header);
}

void InitCompositePanelEntryForTest(HudUiCompositePanelEntry *entry, int marker) {
    reinterpret_cast<HudUiTransitionTextPanel *>(entry)->Constructor();
    entry->panel.flashCountdown = static_cast<float>(marker);
    entry->panel.flashResetValue = static_cast<float>(marker + 10);
    entry->panel.flashAltColor0 = marker;
    entry->panel.flashAltColor1 = marker + 100;
    entry->panel.flashEnabled = marker + 200;
    entry->panel.flashMode = marker + 300;
    entry->panel.flashDirectionSign = -marker;
}

int CompositePanelEntryMarkerForTest(const HudUiCompositePanelEntry &entry) {
    return entry.panel.flashAltColor0;
}

void DestroyCompositePanelEntryForTest(HudUiCompositePanelEntry *entry) {
    reinterpret_cast<HudUiPanel *>(entry)->Destructor();
}

HudUiCompositePanelEntry *AllocateCompositePanelEntriesForTest(std::size_t capacity) {
    return static_cast<HudUiCompositePanelEntry *>(
        ::operator new(sizeof(HudUiCompositePanelEntry) * capacity));
}

void DestroyCompositePanelVectorEntriesForTest(HudUiCompositePanelVector *vector) {
    if (vector->begin == nullptr) {
        return;
    }

    for (HudUiCompositePanelEntry *entry = vector->begin; entry != vector->end; ++entry) {
        DestroyCompositePanelEntryForTest(entry);
    }

    ::operator delete(vector->begin);
    vector->begin = nullptr;
    vector->end = nullptr;
    vector->capacityEnd = nullptr;
}

struct TestReticleAttachState {
    std::uint8_t unknown_00[0x0c];
    zClass_NodePartial *projectileNode;
};

struct TestReticleAltGunController {
    OptCatalogEntryDef *optCatalogEntry;
    std::uint8_t unknown_04[0x24];
    TestReticleAttachState *attachState;
};

struct TestReticlePlayerState {
    std::uint8_t unknown_000[0x58c];
    std::int32_t cameraState;
    std::uint8_t unknown_590[0x54];
    TestReticleAltGunController *activeAltGunController;
    std::uint8_t unknown_5e8[0x8e8];
    zClass_NodePartial *rootNode;
};

std::uint32_t g_testNetExitDeleteFlags = 0;
float g_testNetExitUpdateDelta = -1.0f;
std::int32_t g_testNetExitSetEnabled = -1;

struct TestNetExitPanel : HudUiNetExitPanel {
    void RECOIL_THISCALL UpdateAll(float deltaSeconds) {
        g_testNetExitUpdateDelta = deltaSeconds;
    }

    std::int32_t RECOIL_THISCALL SetEnabled(std::int32_t enabled) {
        g_testNetExitSetEnabled = enabled;
        return enabled;
    }

    HudUiNetExitPanel *RECOIL_THISCALL ScalarDeletingDestructor(std::uint32_t flags) {
        g_testNetExitDeleteFlags = flags;
        return this;
    }
};

int g_auxTextFmtCount[23] = {};
int g_auxVisibleCount[23] = {};
int g_auxLastVisible[23] = {};
const char *g_auxLastText[23] = {};

int AuxLineIndexFromPanel(void *self) {
    auto *const item = reinterpret_cast<HudUiPanelSimple *>(self);
    return static_cast<int>(item - g_HudUiMgrStringMenu->items);
}

void RECOIL_CDECL TestAuxSetTextFmt(HudUiPanel *self, const char *format, ...) {
    const int index = AuxLineIndexFromPanel(self);
    ++g_auxTextFmtCount[index];
    g_auxLastText[index] = format;
}

struct TestAuxPanel {
    void RECOIL_THISCALL SetVisible(std::int32_t visible) {
        const int index = AuxLineIndexFromPanel(this);
        ++g_auxVisibleCount[index];
        g_auxLastVisible[index] = visible;
    }
};

int g_disableVisibleCount = 0;
void *g_disableVisibleThis[96] = {};
int g_disableVisibleValue[96] = {};
int g_disableSetEnabledCount = 0;
int g_disableSetEnabledValue = -1;
void *g_disableSetEnabledThis[16] = {};
int g_disableSetEnabledValues[16] = {};
int g_disableLayoutDisableCount = 0;
int g_enableLayoutEnableCount = 0;

struct TestDisableVisibleReceiver {
    void RECOIL_THISCALL SetVisible(std::int32_t visible) {
        const int index = g_disableVisibleCount;
        if (index < 96) {
            g_disableVisibleThis[index] = this;
            g_disableVisibleValue[index] = visible;
        }

        ++g_disableVisibleCount;
    }
};

struct TestDisableContainer {
    void RECOIL_THISCALL SetEnabled(std::int32_t enabled) {
        const int index = g_disableSetEnabledCount;
        if (index < 16) {
            g_disableSetEnabledThis[index] = this;
            g_disableSetEnabledValues[index] = enabled;
        }

        ++g_disableSetEnabledCount;
        g_disableSetEnabledValue = enabled;
    }
};

struct TestDisableLayout {
    void RECOIL_THISCALL Disable() {
        ++g_disableLayoutDisableCount;
    }
};

struct TestEnableLayout {
    void RECOIL_THISCALL Enable() {
        ++g_enableLayoutEnableCount;
    }
};

int g_layoutActivatedCount = 0;

void RECOIL_FASTCALL TestLayoutOnActivated(HudLayoutBase *) {
    ++g_layoutActivatedCount;
}

int g_elementDrawCount = 0;
int g_elementBaseDrawCount = 0;

void RECOIL_FASTCALL TestElementDraw(HudUiElement *) {
    ++g_elementDrawCount;
}

void RECOIL_FASTCALL TestElementBaseDraw(HudUiElement *) {
    ++g_elementBaseDrawCount;
}

int g_elementInvalidateCount = 0;

void RECOIL_FASTCALL TestElementInvalidate(HudUiElement *element) {
    ++g_elementInvalidateCount;
    element->Invalidate();
}

int g_elementConstructorInvalidateCount = 0;
HudUiElement *g_elementConstructorInvalidateThis = nullptr;

void RECOIL_FASTCALL TestElementConstructorInvalidate(HudUiElement *element) {
    ++g_elementConstructorInvalidateCount;
    g_elementConstructorInvalidateThis = element;
    element->Invalidate();
}

int g_containerUpdateCount = 0;
float g_containerUpdateDelta[4] = {};

struct TestContainerUpdateElement {
    HudUiElement base;

    void RECOIL_THISCALL Update(float deltaSeconds) {
        const int index = g_containerUpdateCount;
        if (index < 4) {
            g_containerUpdateDelta[index] = deltaSeconds;
        }

        ++g_containerUpdateCount;
    }
};

int g_textStackSetFontCount = 0;
void *g_textStackSetFontThis[4] = {};
const char *g_textStackSetFontFace[4] = {};
int g_textStackSetFontHeight[4] = {};
int g_textStackSetFontWeight[4] = {};
int g_textStackSetFontWidth[4] = {};
int g_textStackSetFontItalic[4] = {};
int g_textStackSetFontCharSet[4] = {};
int g_textStackSetFontPitch[4] = {};

struct TestTextStackFontPanel {
    void RECOIL_THISCALL SetFont(const char *faceName, int height, int weight, int width,
                                 int italic, int charSet, int pitchAndFamily) {
        const int index = g_textStackSetFontCount;
        if (index < 4) {
            g_textStackSetFontThis[index] = this;
            g_textStackSetFontFace[index] = faceName;
            g_textStackSetFontHeight[index] = height;
            g_textStackSetFontWeight[index] = weight;
            g_textStackSetFontWidth[index] = width;
            g_textStackSetFontItalic[index] = italic;
            g_textStackSetFontCharSet[index] = charSet;
            g_textStackSetFontPitch[index] = pitchAndFamily;
        }

        ++g_textStackSetFontCount;
    }
};

int g_applyTextLabelSetPosCount = 0;
void *g_applyTextLabelSetPosThis[4] = {};
int g_applyTextLabelSetPosX[4] = {};
int g_applyTextLabelSetPosY[4] = {};
int g_applyTextLabelSetTextFmtCount = 0;
void *g_applyTextLabelSetTextFmtThis[4] = {};
const char *g_applyTextLabelSetTextFmtFormat[4] = {};

struct TestApplyTextLabelPanel {
    void RECOIL_THISCALL SetPos(int x, int y) {
        const int index = g_applyTextLabelSetPosCount;
        if (index < 4) {
            g_applyTextLabelSetPosThis[index] = this;
            g_applyTextLabelSetPosX[index] = x;
            g_applyTextLabelSetPosY[index] = y;
        }

        ++g_applyTextLabelSetPosCount;
    }
};

void RECOIL_CDECL TestApplyTextLabelSetTextFmt(HudUiPanel *self, const char *format, ...) {
    const int index = g_applyTextLabelSetTextFmtCount;
    if (index < 4) {
        g_applyTextLabelSetTextFmtThis[index] = self;
        g_applyTextLabelSetTextFmtFormat[index] = format;
    }

    ++g_applyTextLabelSetTextFmtCount;
}

void *g_shieldApplyWidgetThis = nullptr;
void *g_shieldApplyPanelThis = nullptr;
int g_shieldApplyWidgetCenterX = 0;
int g_shieldApplyWidgetCenterY = 0;
int g_shieldApplyPanelX = 0;
int g_shieldApplyPanelY = 0;
int g_shieldApplyGetXCount = 0;
int g_shieldApplyGetYCount = 0;
int g_shieldApplySetClipCount = 0;
void *g_shieldApplySetClipThis[4] = {};
void *g_shieldApplySetClipBltSource[4] = {};
HudUiRect g_shieldApplySetClipRect[4] = {};
int g_shieldApplyUpdateBoundsCount = 0;
void *g_shieldApplyUpdateBoundsThis = nullptr;
int g_shieldApplyInvalidateCount = 0;

struct TestShieldApplyLayoutOps {
    int RECOIL_THISCALL GetX() {
        ++g_shieldApplyGetXCount;
        return this == g_shieldApplyWidgetThis ? g_shieldApplyWidgetCenterX
                                               : g_shieldApplyPanelX;
    }

    int RECOIL_THISCALL GetY() {
        ++g_shieldApplyGetYCount;
        return this == g_shieldApplyWidgetThis ? g_shieldApplyWidgetCenterY
                                               : g_shieldApplyPanelY;
    }

    void RECOIL_THISCALL SetClip(void *bltSource, const HudUiRect *rect) {
        const int index = g_shieldApplySetClipCount;
        if (index < 4) {
            g_shieldApplySetClipThis[index] = this;
            g_shieldApplySetClipBltSource[index] = bltSource;
            g_shieldApplySetClipRect[index] = *rect;
        }

        ++g_shieldApplySetClipCount;
    }

    void RECOIL_THISCALL UpdateTextBoundsFromContent() {
        ++g_shieldApplyUpdateBoundsCount;
        g_shieldApplyUpdateBoundsThis = this;
    }

    void RECOIL_THISCALL Invalidate() {
        ++g_shieldApplyInvalidateCount;
    }
};

void *g_naniteInitLayoutWidget2 = nullptr;
int g_naniteInitGetCenterXCount = 0;
int g_naniteInitGetCenterYCount = 0;
int g_naniteInitPanelSetPosCount = 0;
void *g_naniteInitPanelSetPosThis = nullptr;
int g_naniteInitPanelSetPosX = 0;
int g_naniteInitPanelSetPosY = 0;
int g_naniteInitPanelSetClipCount = 0;
void *g_naniteInitPanelSetClipThis = nullptr;
void *g_naniteInitPanelBltSource = nullptr;
HudUiRect g_naniteInitPanelClip = {};

struct TestNaniteInitWidget {
    int RECOIL_THISCALL GetCenterX() {
        ++g_naniteInitGetCenterXCount;
        return this == g_naniteInitLayoutWidget2 ? 300 : 33;
    }

    int RECOIL_THISCALL GetCenterY() {
        ++g_naniteInitGetCenterYCount;
        return this == g_naniteInitLayoutWidget2 ? 400 : 44;
    }
};

int g_objectiveMeterGetCenterXCount = 0;
void *g_objectiveMeterGetCenterXThis = nullptr;
int g_objectiveMeterCenterX = 0;

struct TestObjectiveMeterWidget {
    int RECOIL_THISCALL GetCenterX() {
        ++g_objectiveMeterGetCenterXCount;
        g_objectiveMeterGetCenterXThis = this;
        return g_objectiveMeterCenterX;
    }
};

struct TestNaniteInitPanel {
    void RECOIL_THISCALL SetPos(int x, int y) {
        ++g_naniteInitPanelSetPosCount;
        g_naniteInitPanelSetPosThis = this;
        g_naniteInitPanelSetPosX = x;
        g_naniteInitPanelSetPosY = y;
    }

    void RECOIL_THISCALL SetBltSourceAndClipRect(void *bltSourceOrNull,
                                                 const HudUiRect *clipRect) {
        ++g_naniteInitPanelSetClipCount;
        g_naniteInitPanelSetClipThis = this;
        g_naniteInitPanelBltSource = bltSourceOrNull;
        g_naniteInitPanelClip = *clipRect;
    }
};

void *g_backgroundChildElement = nullptr;
void *g_backgroundFocusElement = nullptr;
int g_backgroundHitCount = 0;
int g_backgroundShouldHandleCount = 0;
int g_backgroundShouldHandleHovered = -1;
int g_backgroundHoverEnterCount = 0;
int g_backgroundHoverRepeatCount = 0;
int g_backgroundHoverExitCount = 0;
int g_backgroundCaptureEnterCount = 0;
int g_backgroundCaptureExitCount = 0;
int g_backgroundPrimaryReleaseCount = 0;
int g_backgroundSecondaryReleaseCount = 0;
int g_backgroundPointerStateCount = 0;
int g_backgroundActivateCount = 0;
int g_backgroundAfterInputCount = 0;
int g_backgroundAfterHovered = -1;
int g_backgroundDrawBaseCount = 0;
int g_backgroundChildUpdateCount = 0;
int g_backgroundFocusUpdateCount = 0;
int g_backgroundSetPosCount = 0;
int g_backgroundSetPosX = 0;
int g_backgroundSetPosY = 0;
float g_backgroundChildUpdateDelta = 0.0f;
float g_backgroundFocusUpdateDelta = 0.0f;

struct TestBackgroundInputElement {
    HudUiElement base;
    int hitResult;
    int shouldHandleResult;

    int RECOIL_THISCALL HitTest(int x, int y) {
        ++g_backgroundHitCount;
        return x == 123 && y == 456 ? hitResult : 0;
    }

    int RECOIL_THISCALL ShouldHandleInput(HudUiBackground *, int hovered) {
        ++g_backgroundShouldHandleCount;
        g_backgroundShouldHandleHovered = hovered;
        return shouldHandleResult;
    }

    void RECOIL_THISCALL DrawBase() {
        ++g_backgroundDrawBaseCount;
    }

    void RECOIL_THISCALL SetPos(int x, int y) {
        ++g_backgroundSetPosCount;
        g_backgroundSetPosX = x;
        g_backgroundSetPosY = y;
        base.x = x;
        base.y = y;
    }

    void RECOIL_THISCALL Update(float deltaSeconds) {
        if (this == g_backgroundChildElement) {
            ++g_backgroundChildUpdateCount;
            g_backgroundChildUpdateDelta = deltaSeconds;
        } else if (this == g_backgroundFocusElement) {
            ++g_backgroundFocusUpdateCount;
            g_backgroundFocusUpdateDelta = deltaSeconds;
        }
    }

    void RECOIL_THISCALL OnPrimaryButtonReleased() {
        ++g_backgroundPrimaryReleaseCount;
    }

    void RECOIL_THISCALL OnSecondaryButtonReleased() {
        ++g_backgroundSecondaryReleaseCount;
    }

    void RECOIL_THISCALL OnHoverRepeat() {
        ++g_backgroundHoverRepeatCount;
    }

    void RECOIL_THISCALL OnHoverEnter() {
        ++g_backgroundHoverEnterCount;
    }

    void RECOIL_THISCALL OnHoverExit() {
        ++g_backgroundHoverExitCount;
    }

    void RECOIL_THISCALL OnCaptureEnter() {
        ++g_backgroundCaptureEnterCount;
    }

    void RECOIL_THISCALL OnCaptureExit() {
        ++g_backgroundCaptureExitCount;
    }

    void RECOIL_THISCALL OnPointerButtonState(int, int) {
        ++g_backgroundPointerStateCount;
    }

    void RECOIL_THISCALL OnActivate() {
        ++g_backgroundActivateCount;
    }

    void RECOIL_THISCALL AfterInputUpdate(HudUiBackground *, int hovered) {
        ++g_backgroundAfterInputCount;
        g_backgroundAfterHovered = hovered;
    }
};

struct TestLayoutSetActiveElement {
    const HudLayoutBase_FTable *ftable;
    std::int32_t activeValue;

    std::int32_t RECOIL_THISCALL SetActive(std::int32_t active) {
        activeValue = active;
        return 1;
    }
};

int g_numericAcceptCount = 0;

void RECOIL_FASTCALL TestNumericAccept(HudUiNumericTextInput *) {
    ++g_numericAcceptCount;
}

int g_polylineLineCount = 0;
int g_polylineLineArgs[5] = {};

void RECOIL_FASTCALL TestPolylineRaster4(void *, int x0, int y0, int x1, int y1, int color16) {
    ++g_polylineLineCount;
    g_polylineLineArgs[0] = x0;
    g_polylineLineArgs[1] = y0;
    g_polylineLineArgs[2] = x1;
    g_polylineLineArgs[3] = y1;
    g_polylineLineArgs[4] = color16;
}

void RECOIL_FASTCALL TestPolylineRaster5(void *, const void *, int x0, int y0, int x1, int y1,
                                         int color16) {
    ++g_polylineLineCount;
    g_polylineLineArgs[0] = x0;
    g_polylineLineArgs[1] = y0;
    g_polylineLineArgs[2] = x1;
    g_polylineLineArgs[3] = y1;
    g_polylineLineArgs[4] = color16;
}

struct TestNumericLabelPanel {
    const HudUiPanel_FTable *ftable;
    const char *text;

    void RECOIL_THISCALL SetText(const char *value) {
        text = value;
    }
};

struct TestZrdChildWidget {
    const HudUiWidget_FTable *ftable;
    std::uint32_t deleteFlags;

    TestZrdChildWidget *RECOIL_THISCALL ScalarDeletingDestructor(std::uint32_t flags) {
        deleteFlags = flags;
        return this;
    }
};

struct TestBackgroundBindWidget {
    HudUiWidget base;
    zReader::Node *loadedNode;
    void *owner;
    int postLoadCount;

    int RECOIL_THISCALL LoadFromZrd(zReader::Node *node, void *ownerDialog) {
        loadedNode = node;
        owner = ownerDialog;
        return 1;
    }

    void RECOIL_THISCALL PostLoadFromZrd() {
        ++postLoadCount;
    }
};

int g_compositeEntryDestructorCount = 0;
void *g_compositeEntryDestructorThis[4] = {};
std::uint32_t g_compositeEntryDestructorFlags[4] = {};

struct TestCompositePanelEntry {
    const HudUiCommon_FTable *ftable;

    TestCompositePanelEntry *RECOIL_THISCALL ScalarDeletingDestructor(std::uint32_t flags) {
        const int index = g_compositeEntryDestructorCount;
        if (index < 4) {
            g_compositeEntryDestructorThis[index] = this;
            g_compositeEntryDestructorFlags[index] = flags;
        }

        ++g_compositeEntryDestructorCount;
        return this;
    }
};

int g_slotWidgetDrawCount = 0;

void RECOIL_FASTCALL TestSlotWidgetDraw(HudUiWidget *) {
    ++g_slotWidgetDrawCount;
}

int g_primitiveSetPosCount = 0;
void *g_primitiveSetPosThis = nullptr;
int g_primitiveSetPosX = 0;
int g_primitiveSetPosY = 0;

struct TestPrimitiveBindTarget : HudUiPrimitiveBindTarget {
    void RECOIL_THISCALL SetPos(int x, int y) {
        ++g_primitiveSetPosCount;
        g_primitiveSetPosThis = this;
        g_primitiveSetPosX = x;
        g_primitiveSetPosY = y;
        base.x = x;
        base.y = y;
    }
};

int g_barSetPointSetPosCount = 0;
void *g_barSetPointSetPosThis = nullptr;
int g_barSetPointSetPosX = 0;
int g_barSetPointSetPosY = 0;
int g_barSetPointInvalidateCount = 0;
void *g_barSetPointInvalidateThis = nullptr;

struct TestBarSetPointReceiver {
    void RECOIL_THISCALL SetPos(int x, int y) {
        ++g_barSetPointSetPosCount;
        g_barSetPointSetPosThis = this;
        g_barSetPointSetPosX = x;
        g_barSetPointSetPosY = y;
    }

    void RECOIL_THISCALL Invalidate() {
        ++g_barSetPointInvalidateCount;
        g_barSetPointInvalidateThis = this;
    }
};

template <typename Method> std::uintptr_t MethodAddress(Method method) {
    static_assert(sizeof(method) <= sizeof(std::uintptr_t));
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(method));
    return address;
}

template <typename Slot, typename Method> void AssignMethodSlot(Slot &slot, Method method) {
    static_assert(sizeof(slot) == sizeof(method));
    std::memcpy(&slot, &method, sizeof(slot));
}

int g_testBlitCount = 0;
zVidImagePartial *g_testBlitImages[8] = {};
std::int32_t g_testBlitX[8] = {};
std::int32_t g_testBlitY[8] = {};
std::int32_t g_testBlitFlags[8] = {};
zVidRect32 g_testBlitRects[8] = {};
std::int32_t g_testBlitHasRect[8] = {};

void RECOIL_FASTCALL TestBltSourceToPrimary(void *self, std::int32_t dstX, std::int32_t dstY,
                                            std::int32_t clipFlags, void *srcRect) {
    const int index = g_testBlitCount;
    if (index < 8) {
        g_testBlitImages[index] = static_cast<zVidImagePartial *>(self);
        g_testBlitX[index] = dstX;
        g_testBlitY[index] = dstY;
        g_testBlitFlags[index] = clipFlags;
        g_testBlitHasRect[index] = srcRect != nullptr ? 1 : 0;
        if (srcRect != nullptr) {
            g_testBlitRects[index] = *static_cast<zVidRect32 *>(srcRect);
        }
    }
    ++g_testBlitCount;
}

int g_tripletUpdateAllCount = 0;
float g_tripletUpdateAllDelta = 0.0f;

struct TestTripletContainerDispatch {
    void RECOIL_THISCALL UpdateAll(float deltaSeconds) {
        ++g_tripletUpdateAllCount;
        g_tripletUpdateAllDelta = deltaSeconds;
    }
};

void TestPanelSetTextFmtV(HudUiPanel *panel, const char *format, ...) {
    va_list args;
    va_start(args, format);
    panel->SetTextFmtV(format, args);
    va_end(args);
}

int RECOIL_FASTCALL TestVideoSurfaceStateNoOp(zVideo_SurfaceStatePartial *) {
    return 0;
}
} // namespace

extern "C" int hud_ui_net_exit_destroy_global_smoke(void) {
    HudUiNetExitPanel_FTable ftable{};
    ftable.updateAll =
        reinterpret_cast<decltype(ftable.updateAll)>(MethodAddress(&TestNetExitPanel::UpdateAll));
    ftable.setEnabled =
        reinterpret_cast<decltype(ftable.setEnabled)>(MethodAddress(&TestNetExitPanel::SetEnabled));
    ftable.scalarDeletingDtor = reinterpret_cast<decltype(ftable.scalarDeletingDtor)>(
        MethodAddress(&TestNetExitPanel::ScalarDeletingDestructor));

    TestNetExitPanel panel{};
    panel.base.base.base.vptr = reinterpret_cast<const HudUiContainer_FTable *>(&ftable);

    g_testNetExitDeleteFlags = 0;
    g_HudUiNetExitPanel = &panel;

    HudUiNetExitPanel::DestroyGlobal();

    return g_testNetExitDeleteFlags == 1 && g_HudUiNetExitPanel == nullptr ? 0 : 1;
}

extern "C" int hud_ui_net_exit_show_tick_smoke(void) {
    HudUiNetExitPanel_FTable ftable{};
    ftable.updateAll =
        reinterpret_cast<decltype(ftable.updateAll)>(MethodAddress(&TestNetExitPanel::UpdateAll));
    ftable.setEnabled =
        reinterpret_cast<decltype(ftable.setEnabled)>(MethodAddress(&TestNetExitPanel::SetEnabled));

    TestNetExitPanel panel{};
    panel.base.base.base.vptr = reinterpret_cast<const HudUiContainer_FTable *>(&ftable);

    g_testNetExitUpdateDelta = -1.0f;
    g_testNetExitSetEnabled = -1;
    g_FrameDeltaTimeSec = 0.25f;
    g_HudUiNetExitPanel = &panel;

    HudUiNetExitPanel::Show();
    HudUiNetExitPanel::Tick();

    g_HudUiNetExitPanel = nullptr;

    return g_testNetExitSetEnabled == 1 && g_testNetExitUpdateDelta == 0.25f ? 0 : 1;
}

extern "C" int hud_ui_net_exit_constructor_smoke(void) {
    int joystickOption = 0;
    int *const savedJoystickOption = ZOPT_INPUT_JOYSTICK;
    ZOPT_INPUT_JOYSTICK = &joystickOption;

    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption{};
    vmodeOption.payloadOrBuffer = 6;
    vmodeOption.name = vmodeName;
    zOptionEntryPartial *const savedOptionsHead = g_zGame_Options_OptionListHead;
    g_zGame_Options_OptionListHead = &vmodeOption;

    std::uint16_t pixels[4] = {};
    const int savedRendererType = g_zVideo_RendererType;
    const int savedHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial savedPrimarySurface = g_zVideo_PrimarySurfaceState;
    auto *const savedLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const savedUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(std::uint16_t) * 2;

    HudUiElement savedFocus{};
    g_HudUiNetExitPanel_SavedInputFocus = &savedFocus;

    HudUiNetExitPanel panel{};
    const bool returnedSelf = panel.Constructor() == &panel;
    const bool initialized =
        panel.resumeWidget.previewInputCaptureActive == 0 &&
        panel.exitWidget.previewInputCaptureActive == 0 &&
        panel.base.base.inputFocusElement == nullptr &&
        panel.base.base.base.enabled == 0 &&
        g_HudUiNetExitPanel_SavedInputFocus == nullptr;

    ZOPT_INPUT_JOYSTICK = savedJoystickOption;
    g_zGame_Options_OptionListHead = savedOptionsHead;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_UseHalfResBackbuffer = savedHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = savedPrimarySurface;
    g_zVideo_pfnLockSurfaceState = savedLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = savedUnlockSurfaceState;
    g_HudUiNetExitPanel_SavedInputFocus = nullptr;

    return returnedSelf && initialized ? 0 : 1;
}

extern "C" int hud_ui_net_exit_create_global_smoke(void) {
    int joystickOption = 0;
    int *const savedJoystickOption = ZOPT_INPUT_JOYSTICK;
    ZOPT_INPUT_JOYSTICK = &joystickOption;

    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption{};
    vmodeOption.payloadOrBuffer = 6;
    vmodeOption.name = vmodeName;
    zOptionEntryPartial *const savedOptionsHead = g_zGame_Options_OptionListHead;
    g_zGame_Options_OptionListHead = &vmodeOption;

    std::uint16_t pixels[4] = {};
    const int savedRendererType = g_zVideo_RendererType;
    const int savedHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial savedPrimarySurface = g_zVideo_PrimarySurfaceState;
    auto *const savedLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const savedUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(std::uint16_t) * 2;

    HudUiElement savedFocus{};
    g_HudUiNetExitPanel = nullptr;
    g_HudUiNetExitPanel_SavedInputFocus = &savedFocus;

    HudUiNetExitPanel *const panel = HudUiNetExitPanel::CreateGlobal();
    const bool created =
        panel != nullptr && g_HudUiNetExitPanel == panel &&
        panel->resumeWidget.previewInputCaptureActive == 0 &&
        panel->exitWidget.previewInputCaptureActive == 0 &&
        panel->base.base.inputFocusElement == nullptr &&
        panel->base.base.base.enabled == 0 &&
        g_HudUiNetExitPanel_SavedInputFocus == nullptr;

    g_HudUiNetExitPanel = nullptr;

    ZOPT_INPUT_JOYSTICK = savedJoystickOption;
    g_zGame_Options_OptionListHead = savedOptionsHead;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_UseHalfResBackbuffer = savedHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = savedPrimarySurface;
    g_zVideo_pfnLockSurfaceState = savedLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = savedUnlockSurfaceState;
    g_HudUiNetExitPanel_SavedInputFocus = nullptr;

    return created ? 0 : 1;
}

extern "C" int hud_ui_aux_overlay_text_lines_smoke(void) {
    HudUiPanel_FTable ftable{};
    ftable.slots[24] = MethodAddress(&TestAuxPanel::SetVisible);
    ftable.slots[29] = reinterpret_cast<std::uintptr_t>(&TestAuxSetTextFmt);

    HudUiStringMenu menu{};
    for (HudUiPanelSimple &item : menu.items) {
        *reinterpret_cast<const HudUiPanel_FTable **>(&item) = &ftable;
    }

    std::memset(g_auxTextFmtCount, 0, sizeof(g_auxTextFmtCount));
    std::memset(g_auxVisibleCount, 0, sizeof(g_auxVisibleCount));
    std::memset(g_auxLastVisible, 0, sizeof(g_auxLastVisible));
    std::memset(g_auxLastText, 0, sizeof(g_auxLastText));
    g_HudUiMgrStringMenu = &menu;

    HudUiAuxOverlay::UpdateTextLine(1, 3, "alpha");
    HudUiAuxOverlay::UpdateTextLine(2, 4, "bravo");
    HudUiAuxOverlay::UpdateTextLine(2, 5, "");

    const bool nonEmptyOps =
        g_auxTextFmtCount[3] == 1 && g_auxVisibleCount[3] == 1 && g_auxLastVisible[3] == 1 &&
        std::strcmp(g_auxLastText[3], "alpha") == 0 && g_auxTextFmtCount[4] == 1 &&
        g_auxVisibleCount[4] == 1 && g_auxLastVisible[4] == 1 &&
        std::strcmp(g_auxLastText[4], "bravo") == 0;
    const bool emptyOp =
        g_auxTextFmtCount[5] == 0 && g_auxVisibleCount[5] == 1 && g_auxLastVisible[5] == 0;

    HudUiAuxOverlay::ClearTextLines();

    bool clearOps = true;
    for (int index = 0; index < 23; ++index) {
        const int expectedVisible = 2 + (index == 3 || index == 4 || index == 5 ? 1 : 0);
        const int expectedText = index == 3 || index == 4 ? 1 : 0;
        clearOps = clearOps && g_auxVisibleCount[index] == expectedVisible &&
                   g_auxTextFmtCount[index] == expectedText && g_auxLastVisible[index] == 0;
    }

    g_HudUiMgrStringMenu = nullptr;

    return nonEmptyOps && emptyOp && clearOps ? 0 : 1;
}

extern "C" int zhud_mgr_destroy_sensor_window_null_smoke(void) {
    HudUiMgr::DestroySensorWindow();
    return 0;
}

extern "C" int zhud_mgr_disable_hud_smoke(void) {
    HudUiCommon_FTable visibleTable{};
    visibleTable.slots[24] = MethodAddress(&TestDisableVisibleReceiver::SetVisible);

    HudUiContainer_FTable containerTable{};
    AssignMethodSlot(containerTable.setEnabled, &TestDisableContainer::SetEnabled);

    HudLayoutBase_FTable layoutTable{};
    layoutTable.slots[5] = MethodAddress(&TestDisableLayout::Disable);
    HudLayoutBase layout{};
    layout.ftable = &layoutTable;

    HudUiPanel_FTable panelTable{};
    panelTable.slots[24] = MethodAddress(&TestDisableVisibleReceiver::SetVisible);
    HudUiPanel descPanel{};
    HudUiPanel summaryPanel{};
    HudUiPanel labelPanel{};
    descPanel.vtbl = &panelTable;
    summaryPanel.vtbl = &panelTable;
    labelPanel.vtbl = &panelTable;

    HudUiTimerPanel timerPanel{};
    *reinterpret_cast<const HudUiCommon_FTable **>(&timerPanel) = &visibleTable;

    for (HudUiSlot &slot : g_HudUiMgrWeaponSlots) {
        slot.slotWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
        slot.trackMarkerWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    }

    std::memset(g_disableVisibleThis, 0, sizeof(g_disableVisibleThis));
    std::memset(g_disableVisibleValue, 0, sizeof(g_disableVisibleValue));
    g_disableVisibleCount = 0;
    g_disableSetEnabledCount = 0;
    g_disableSetEnabledValue = -1;
    g_disableLayoutDisableCount = 0;

    g_HudUiMgr = {};
    g_HudUiMgr.vptr = &containerTable;
    g_HudUiMgr.enabled = 7;
    g_HudUiMgrCurrentLayout = &layout;
    g_HudUiMgrObjectiveWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveBar.ftable = reinterpret_cast<const HudUiBar_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveMeter.ftable = reinterpret_cast<const HudUiMeter_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveLabelTextPanel = &labelPanel;
    g_HudUiMgrTimerPanel = &timerPanel;
    g_HudUiMgrSensorTargetMarkerCount = 11;
    g_HudUiMgrWeaponState = 12;
    g_HudUiMgrLayoutDelayFrames = 0;
    gAltClipPassEnabled = 5;

    std::int32_t accelerationOption = 1;
    ZOPT_VIDEO_ACCELERATION = &accelerationOption;
    std::int32_t hudTypeSw = 2;
    ZOPT_HUD_TYPE_SW = &hudTypeSw;
    g_zOpt_HwMode = 0;

    const std::int32_t previous = HudUiMgr::DisableHud();

    const bool coreState = previous == 7 && g_HudUiMgrSensorTargetMarkerCount == 0 &&
                           g_HudUiMgrWeaponState == 0 && g_disableSetEnabledCount == 1 &&
                           g_disableSetEnabledValue == 0 && g_disableLayoutDisableCount == 1 &&
                           gAltClipPassEnabled == 0 && g_HudUiMgrLayoutDelayFrames == 2;

    const bool visibilityCount = g_disableVisibleCount == 72;
    const bool weaponVisibility =
        g_disableVisibleThis[0] == &g_HudUiMgrWeaponSlots[0].trackMarkerWidget &&
        g_disableVisibleThis[1] == &g_HudUiMgrWeaponSlots[0].slotWidget &&
        g_disableVisibleThis[62] == &g_HudUiMgrWeaponSlots[31].trackMarkerWidget &&
        g_disableVisibleThis[63] == &g_HudUiMgrWeaponSlots[31].slotWidget;
    const bool objectiveVisibility = g_disableVisibleThis[64] == &g_HudUiMgrObjectiveWidget &&
                                     g_disableVisibleThis[65] == &descPanel &&
                                     g_disableVisibleThis[66] == &g_HudUiMgrObjectiveBar &&
                                     g_disableVisibleThis[67] == &g_HudUiMgrObjectiveSensorRect &&
                                     g_disableVisibleThis[68] == &summaryPanel &&
                                     g_disableVisibleThis[69] == &labelPanel &&
                                     g_disableVisibleThis[70] == &g_HudUiMgrObjectiveMeter &&
                                     g_disableVisibleThis[71] == &timerPanel;

    bool visibleValues = true;
    for (int index = 0; index < 71; ++index) {
        visibleValues = visibleValues && g_disableVisibleValue[index] == 0;
    }
    visibleValues = visibleValues && g_disableVisibleValue[71] == 1;

    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgrObjectiveDescTextPanel = nullptr;
    g_HudUiMgrObjectiveSummaryTextPanel = nullptr;
    g_HudUiMgrObjectiveLabelTextPanel = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    ZOPT_VIDEO_ACCELERATION = nullptr;
    ZOPT_HUD_TYPE_SW = nullptr;

    return coreState && visibilityCount && weaponVisibility && objectiveVisibility && visibleValues
               ? 0
               : 1;
}

extern "C" int zhud_mgr_enable_hud_smoke(void) {
    HudUiCommon_FTable visibleTable{};
    visibleTable.slots[24] = MethodAddress(&TestDisableVisibleReceiver::SetVisible);

    HudUiContainer_FTable containerTable{};
    AssignMethodSlot(containerTable.setEnabled, &TestDisableContainer::SetEnabled);

    HudLayoutBase_FTable layoutTable{};
    layoutTable.slots[4] = MethodAddress(&TestEnableLayout::Enable);
    HudLayoutBase layout{};
    layout.ftable = &layoutTable;

    std::memset(g_disableVisibleThis, 0, sizeof(g_disableVisibleThis));
    std::memset(g_disableVisibleValue, 0, sizeof(g_disableVisibleValue));
    g_disableVisibleCount = 0;
    g_disableSetEnabledCount = 0;
    g_disableSetEnabledValue = -1;
    g_enableLayoutEnableCount = 0;

    g_HudUiMgr = {};
    g_HudUiMgr.vptr = &containerTable;
    g_HudUiMgr.enabled = 9;
    g_HudUiMgrCurrentLayout = &layout;
    g_HudUiMgrObjectiveWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.left = 2.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.top = 3.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.right = 22.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom = 33.0f;
    gAltClipPassEnabled = 0;
    gAltClipSourceRectValid = 0;
    g_zClipAlt_SourceLeft = 0.0f;
    g_zClipAlt_SourceTop = 0.0f;
    g_zClipAlt_SourceWidth = 0.0f;
    g_zClipAlt_SourceHeight = 0.0f;

    const std::int32_t previous = HudUiMgr::EnableHud();

    const bool dispatch = previous == 9 && g_HudUiMgr.enabled == 1 &&
                          g_disableSetEnabledCount == 0 && g_enableLayoutEnableCount == 1;
    const bool objectiveVisible = g_disableVisibleCount == 1 &&
                                  g_disableVisibleThis[0] == &g_HudUiMgrObjectiveWidget &&
                                  g_disableVisibleValue[0] == 1;
    const bool clipState = gAltClipPassEnabled == 1 && gAltClipSourceRectValid == 1 &&
                           g_zClipAlt_SourceLeft == 2.0f && g_zClipAlt_SourceTop == 3.0f &&
                           g_zClipAlt_SourceWidth == 20.0f &&
                           g_zClipAlt_SourceHeight == 30.0f;

    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgrObjectivePhase = 0;
    gAltClipPassEnabled = 0;
    gAltClipSourceRectValid = 0;

    return dispatch && objectiveVisible && clipState ? 0 : 1;
}

extern "C" int zhud_mgr_toggle_hud_smoke(void) {
    HudUiCommon_FTable visibleTable{};
    visibleTable.slots[24] = MethodAddress(&TestDisableVisibleReceiver::SetVisible);

    HudUiContainer_FTable containerTable{};
    AssignMethodSlot(containerTable.setEnabled, &TestDisableContainer::SetEnabled);

    HudLayoutBase_FTable layoutTable{};
    layoutTable.slots[4] = MethodAddress(&TestEnableLayout::Enable);
    layoutTable.slots[5] = MethodAddress(&TestDisableLayout::Disable);
    HudLayoutBase layout{};
    layout.ftable = &layoutTable;

    HudUiPanel_FTable panelTable{};
    panelTable.slots[24] = MethodAddress(&TestDisableVisibleReceiver::SetVisible);
    HudUiPanel descPanel{};
    HudUiPanel summaryPanel{};
    HudUiPanel labelPanel{};
    descPanel.vtbl = &panelTable;
    summaryPanel.vtbl = &panelTable;
    labelPanel.vtbl = &panelTable;

    HudUiTimerPanel timerPanel{};
    *reinterpret_cast<const HudUiCommon_FTable **>(&timerPanel) = &visibleTable;

    for (HudUiSlot &slot : g_HudUiMgrWeaponSlots) {
        slot.slotWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
        slot.trackMarkerWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    }

    std::memset(g_disableVisibleThis, 0, sizeof(g_disableVisibleThis));
    std::memset(g_disableVisibleValue, 0, sizeof(g_disableVisibleValue));
    g_disableVisibleCount = 0;
    g_disableSetEnabledCount = 0;
    g_disableSetEnabledValue = -1;
    g_disableLayoutDisableCount = 0;
    g_enableLayoutEnableCount = 0;

    g_HudUiMgr = {};
    g_HudUiMgr.vptr = &containerTable;
    g_HudUiMgr.enabled = 1;
    g_HudUiMgrCurrentLayout = &layout;
    g_HudUiMgrObjectiveWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveBar.ftable = reinterpret_cast<const HudUiBar_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveMeter.ftable = reinterpret_cast<const HudUiMeter_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveLabelTextPanel = &labelPanel;
    g_HudUiMgrTimerPanel = &timerPanel;
    gAltClipPassEnabled = 5;
    std::int32_t accelerationOption = 1;
    std::int32_t hudTypeSw = 2;
    ZOPT_VIDEO_ACCELERATION = &accelerationOption;
    ZOPT_HUD_TYPE_SW = &hudTypeSw;
    g_zOpt_HwMode = 0;

    const int disabledReturn = HudUiMgr::ToggleHud();
    const bool disabledPath = disabledReturn == 1 && g_disableLayoutDisableCount == 1 &&
                              g_enableLayoutEnableCount == 0 && g_disableSetEnabledCount == 1 &&
                              g_disableSetEnabledValue == 0 &&
                              g_HudUiMgrLayoutDelayFrames == 2 && gAltClipPassEnabled == 0;

    g_disableVisibleCount = 0;
    g_disableSetEnabledCount = 0;
    g_disableSetEnabledValue = -1;
    g_disableLayoutDisableCount = 0;
    g_enableLayoutEnableCount = 0;
    g_HudUiMgr = {};
    g_HudUiMgr.vptr = &containerTable;
    g_HudUiMgr.enabled = 0;
    g_HudUiMgrCurrentLayout = &layout;
    g_HudUiMgrObjectiveWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.left = 2.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.top = 3.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.right = 22.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom = 33.0f;
    gAltClipPassEnabled = 0;
    gAltClipSourceRectValid = 0;

    const int enabledReturn = HudUiMgr::ToggleHud();
    int enabledFailure = 0;
    if (enabledReturn != 1) {
        enabledFailure = 20;
    } else if (g_enableLayoutEnableCount != 1) {
        enabledFailure = 21;
    } else if (g_disableLayoutDisableCount != 0) {
        enabledFailure = 22;
    } else if (g_disableSetEnabledCount != 0) {
        enabledFailure = 23;
    } else if (g_HudUiMgr.enabled != 1) {
        enabledFailure = 25;
    } else if (gAltClipPassEnabled != 1) {
        enabledFailure = 26;
    } else if (gAltClipSourceRectValid != 1) {
        enabledFailure = 27;
    }

    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgrObjectiveDescTextPanel = nullptr;
    g_HudUiMgrObjectiveSummaryTextPanel = nullptr;
    g_HudUiMgrObjectiveLabelTextPanel = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    ZOPT_VIDEO_ACCELERATION = nullptr;
    ZOPT_HUD_TYPE_SW = nullptr;
    g_HudUiMgrObjectivePhase = 0;
    gAltClipPassEnabled = 0;
    gAltClipSourceRectValid = 0;

    if (!disabledPath) {
        return 1;
    }
    return enabledFailure == 0 ? 0 : enabledFailure;
}

extern "C" int zhud_mgr_switch_active_dialog_smoke(void) {
    HudLayoutBase_FTable disabledOldTable{};
    disabledOldTable.slots[2] = MethodAddress(&TestLayoutSetActiveElement::SetActive);
    HudLayoutBase_FTable disabledNewTable{};
    disabledNewTable.slots[2] = MethodAddress(&TestLayoutSetActiveElement::SetActive);
    TestLayoutSetActiveElement disabledOld{};
    TestLayoutSetActiveElement disabledNew{};
    disabledOld.ftable = &disabledOldTable;
    disabledNew.ftable = &disabledNewTable;

    g_HudUiMgr = {};
    g_HudUiMgr.enabled = 0;
    g_HudUiMgrLayoutDelayFrames = 0;
    g_HudUiMgrCurrentLayout = reinterpret_cast<HudLayoutBase *>(&disabledOld);
    HudUiMgr::SwitchActiveDialog(reinterpret_cast<HudLayoutBase *>(&disabledNew));
    const bool disabledPath = g_HudUiMgrLayoutDelayFrames == 2 && disabledOld.activeValue == 0 &&
                              disabledNew.activeValue == 1 &&
                              g_HudUiMgrCurrentLayout ==
                                  reinterpret_cast<HudLayoutBase *>(&disabledNew);

    HudUiCommon_FTable visibleTable{};
    visibleTable.slots[24] = MethodAddress(&TestDisableVisibleReceiver::SetVisible);

    HudUiContainer_FTable containerTable{};
    AssignMethodSlot(containerTable.setEnabled, &TestDisableContainer::SetEnabled);

    HudLayoutBase_FTable enabledOldTable{};
    enabledOldTable.slots[2] = MethodAddress(&TestLayoutSetActiveElement::SetActive);
    enabledOldTable.slots[5] = MethodAddress(&TestDisableLayout::Disable);
    HudLayoutBase_FTable enabledNewTable{};
    enabledNewTable.slots[2] = MethodAddress(&TestLayoutSetActiveElement::SetActive);
    enabledNewTable.slots[4] = MethodAddress(&TestEnableLayout::Enable);
    TestLayoutSetActiveElement enabledOld{};
    TestLayoutSetActiveElement enabledNew{};
    enabledOld.ftable = &enabledOldTable;
    enabledNew.ftable = &enabledNewTable;

    HudUiPanel_FTable panelTable{};
    panelTable.slots[24] = MethodAddress(&TestDisableVisibleReceiver::SetVisible);
    HudUiPanel descPanel{};
    HudUiPanel summaryPanel{};
    HudUiPanel labelPanel{};
    descPanel.vtbl = &panelTable;
    summaryPanel.vtbl = &panelTable;
    labelPanel.vtbl = &panelTable;

    HudUiTimerPanel timerPanel{};
    *reinterpret_cast<const HudUiCommon_FTable **>(&timerPanel) = &visibleTable;

    for (HudUiSlot &slot : g_HudUiMgrWeaponSlots) {
        slot.slotWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
        slot.trackMarkerWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    }

    std::memset(g_disableVisibleThis, 0, sizeof(g_disableVisibleThis));
    std::memset(g_disableVisibleValue, 0, sizeof(g_disableVisibleValue));
    g_disableVisibleCount = 0;
    g_disableSetEnabledCount = 0;
    g_disableSetEnabledValue = -1;
    g_disableLayoutDisableCount = 0;
    g_enableLayoutEnableCount = 0;

    g_HudUiMgr = {};
    g_HudUiMgr.vptr = &containerTable;
    g_HudUiMgr.enabled = 5;
    g_HudUiMgrCurrentLayout = reinterpret_cast<HudLayoutBase *>(&enabledOld);
    g_HudUiMgrObjectiveWidget.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveBar.ftable = reinterpret_cast<const HudUiBar_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveMeter.ftable = reinterpret_cast<const HudUiMeter_FTable *>(&visibleTable);
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveLabelTextPanel = &labelPanel;
    g_HudUiMgrTimerPanel = &timerPanel;
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.left = 4.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.top = 6.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.right = 44.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom = 66.0f;
    gAltClipPassEnabled = 5;

    std::int32_t accelerationOption = 1;
    ZOPT_VIDEO_ACCELERATION = &accelerationOption;
    std::int32_t hudTypeSw = 2;
    ZOPT_HUD_TYPE_SW = &hudTypeSw;
    g_zOpt_HwMode = 0;

    HudUiMgr::SwitchActiveDialog(reinterpret_cast<HudLayoutBase *>(&enabledNew));

    const bool enabledPath =
        g_disableSetEnabledCount == 1 && g_disableSetEnabledValue == 0 &&
        g_disableLayoutDisableCount == 1 && g_enableLayoutEnableCount == 1 &&
        enabledOld.activeValue == 0 && enabledNew.activeValue == 1 &&
        g_HudUiMgrCurrentLayout == reinterpret_cast<HudLayoutBase *>(&enabledNew) &&
        g_HudUiMgr.enabled == 1 && gAltClipPassEnabled == 1 &&
        g_zClipAlt_SourceLeft == 4.0f && g_zClipAlt_SourceTop == 6.0f &&
        g_zClipAlt_SourceWidth == 40.0f && g_zClipAlt_SourceHeight == 60.0f;

    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgrObjectiveDescTextPanel = nullptr;
    g_HudUiMgrObjectiveSummaryTextPanel = nullptr;
    g_HudUiMgrObjectiveLabelTextPanel = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    ZOPT_VIDEO_ACCELERATION = nullptr;
    ZOPT_HUD_TYPE_SW = nullptr;
    g_HudUiMgrObjectivePhase = 0;
    gAltClipPassEnabled = 0;

    return disabledPath && enabledPath ? 0 : 1;
}

extern "C" int zhud_mgr_set_float_timer_visible_smoke(void) {
    HudUiTimerPanelFloat_FTable visibleTable{};
    visibleTable.slots[24] = MethodAddress(&TestDisableVisibleReceiver::SetVisible);

    HudUiTimerPanelFloat timer{};
    *reinterpret_cast<const HudUiTimerPanelFloat_FTable **>(&timer) = &visibleTable;

    HudLayoutBase_FTable layoutTable{};
    layoutTable.OnActivated = TestLayoutOnActivated;
    HudLayoutBase layout{&layoutTable};

    std::memset(g_disableVisibleThis, 0, sizeof(g_disableVisibleThis));
    std::memset(g_disableVisibleValue, 0, sizeof(g_disableVisibleValue));
    g_disableVisibleCount = 0;
    g_layoutActivatedCount = 0;

    g_HudUiMgrTimerPanelFloat = &timer;
    g_HudUiMgrCurrentLayout = &layout;

    HudUiMgr::SetFloatTimerVisible(1);
    const bool visiblePath = g_disableVisibleCount == 1 && g_disableVisibleThis[0] == &timer &&
                             g_disableVisibleValue[0] == 1 && g_layoutActivatedCount == 0;

    HudUiMgr::SetFloatTimerVisible(0);
    const bool hiddenPath = g_disableVisibleCount == 2 && g_disableVisibleThis[1] == &timer &&
                            g_disableVisibleValue[1] == 0 && g_layoutActivatedCount == 1;

    g_HudUiMgrTimerPanelFloat = nullptr;
    g_HudUiMgrCurrentLayout = nullptr;
    return visiblePath && hiddenPath ? 0 : 1;
}

extern "C" int zhud_mgr_hide_tracked_progress_meter_if_owner_matches_smoke(void) {
    HudUiMeter_FTable visibleTable{};
    visibleTable.slots[24] = MethodAddress(&TestDisableVisibleReceiver::SetVisible);

    const HudUiMeter oldMeter = g_HudUiMgrSensorMeter;
    HudUiSlot *const oldTrackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;

    std::memset(g_disableVisibleThis, 0, sizeof(g_disableVisibleThis));
    std::memset(g_disableVisibleValue, 0, sizeof(g_disableVisibleValue));
    g_disableVisibleCount = 0;

    g_HudUiMgrSensorMeter = {};
    g_HudUiMgrSensorMeter.ftable = &visibleTable;

    int ownerPayload = 0;
    int otherPayload = 0;
    HudUiMgrSensorTrackNode trackNode{};
    trackNode.payload = &ownerPayload;
    HudUiSlot slot{};
    slot.trackNode = &trackNode;
    g_HudUiMgrSensorTrackedProgressSlot = &slot;

    HudUiMgr::HideTrackedProgressMeterIfOwnerMatches(&otherPayload);
    const bool rejectedOwner = g_disableVisibleCount == 0;

    HudUiMgr::HideTrackedProgressMeterIfOwnerMatches(&ownerPayload);
    const bool acceptedOwner = g_disableVisibleCount == 1 &&
                               g_disableVisibleThis[0] == &g_HudUiMgrSensorMeter &&
                               g_disableVisibleValue[0] == 0;

    g_HudUiMgrSensorTrackedProgressSlot = nullptr;
    HudUiMgr::HideTrackedProgressMeterIfOwnerMatches(&ownerPayload);
    const bool nullSlotIgnored = g_disableVisibleCount == 1;

    g_HudUiMgrSensorMeter = oldMeter;
    g_HudUiMgrSensorTrackedProgressSlot = oldTrackedProgressSlot;

    return rejectedOwner && acceptedOwner && nullSlotIgnored ? 0 : 1;
}

extern "C" int zhud_mgr_set_aux_overlay_visible_smoke(void) {
    HudUiContainer_FTable containerTable{};
    AssignMethodSlot(containerTable.setEnabled, &TestDisableContainer::SetEnabled);

    HudUiStringMenu menu{};
    menu.base.vptr = &containerTable;

    std::memset(g_disableSetEnabledThis, 0, sizeof(g_disableSetEnabledThis));
    std::memset(g_disableSetEnabledValues, 0, sizeof(g_disableSetEnabledValues));
    g_disableSetEnabledCount = 0;
    g_disableSetEnabledValue = -1;

    g_HudUiMgrStringMenu = &menu;

    HudUiMgr::SetAuxOverlayVisible(1);
    const bool visiblePath = g_disableSetEnabledCount == 1 &&
                             g_disableSetEnabledThis[0] == &menu.base &&
                             g_disableSetEnabledValues[0] == 1 &&
                             g_disableSetEnabledValue == 1;

    HudUiMgr::SetAuxOverlayVisible(0);
    const bool hiddenPath = g_disableSetEnabledCount == 2 &&
                            g_disableSetEnabledThis[1] == &menu.base &&
                            g_disableSetEnabledValues[1] == 0 &&
                            g_disableSetEnabledValue == 0;

    g_HudUiMgrStringMenu = nullptr;
    return visiblePath && hiddenPath ? 0 : 1;
}

extern "C" int zhud_mgr_apply_hud_mode_switch_smoke(void) {
    const HudLayoutBase_FTable *const oldSwTable = g_HudLayoutSW.ftable;
    const int oldSwEnabled = g_HudLayoutSW.enabled;
    const void *const oldHwTable = TestFieldAt<const void *>(&g_HudLayoutHW, 0);
    const int oldHwEnabled = TestFieldAt<int>(&g_HudLayoutHW, 4);
    HudLayoutBase *const oldCurrentLayout = g_HudUiMgrCurrentLayout;
    const int oldLayoutsInitialized = g_HudUiMgrHudLayoutsInitialized;
    const int oldHudEnabled = g_HudUiMgr.enabled;
    const int oldLayoutDelay = g_HudUiMgrLayoutDelayFrames;
    int *const oldHudTypeSw = ZOPT_HUD_TYPE_SW;
    int *const oldHudTypeHw = ZOPT_HUD_TYPE_HW;
    const int oldHwMode = g_zOpt_HwMode;

    HudLayoutBase_FTable swTable{};
    swTable.slots[2] = MethodAddress(&TestLayoutSetActiveElement::SetActive);
    HudLayoutBase_FTable hwTable{};
    hwTable.slots[2] = MethodAddress(&TestLayoutSetActiveElement::SetActive);

    g_HudLayoutSW.ftable = &swTable;
    g_HudLayoutSW.enabled = 0;
    TestFieldAt<const HudLayoutBase_FTable *>(&g_HudLayoutHW, 0) = &hwTable;
    TestFieldAt<int>(&g_HudLayoutHW, 4) = 0;

    std::int32_t hudTypeSw = 7;
    std::int32_t hudTypeHw = 8;
    ZOPT_HUD_TYPE_SW = &hudTypeSw;
    ZOPT_HUD_TYPE_HW = &hudTypeHw;
    g_zOpt_HwMode = 0;
    g_HudUiMgr.enabled = 0;
    g_HudUiMgrLayoutDelayFrames = 0;
    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgrHudLayoutsInitialized = 0;

    const int uninitializedReturn = HudUiMgr::ApplyHudModeSwitch(1);
    const bool uninitialized =
        uninitializedReturn == 7 && g_HudUiMgrCurrentLayout == nullptr &&
        g_HudLayoutSW.enabled == 0 && TestFieldAt<int>(&g_HudLayoutHW, 4) == 0;

    g_HudUiMgrHudLayoutsInitialized = 1;
    const int swReturn = HudUiMgr::ApplyHudModeSwitch(1);
    const bool swSwitch = swReturn == 7 && g_HudUiMgrCurrentLayout == &g_HudLayoutSW &&
                          g_HudLayoutSW.enabled == 1 &&
                          TestFieldAt<int>(&g_HudLayoutHW, 4) == 0 &&
                          g_HudUiMgrLayoutDelayFrames == 2;

    g_zOpt_HwMode = 1;
    const int hwReturn = HudUiMgr::ApplyHudModeSwitch(2);
    const bool hwSwitch =
        hwReturn == 8 &&
        g_HudUiMgrCurrentLayout == reinterpret_cast<HudLayoutBase *>(&g_HudLayoutHW) &&
        TestFieldAt<int>(&g_HudLayoutHW, 4) == 1 && g_HudLayoutSW.enabled == 0;

    g_HudUiMgrCurrentLayout = nullptr;
    g_HudLayoutSW.enabled = 0;
    TestFieldAt<int>(&g_HudLayoutHW, 4) = 0;
    const int ignoredReturn = HudUiMgr::ApplyHudModeSwitch(3);
    const bool ignored = ignoredReturn == 8 && g_HudUiMgrCurrentLayout == nullptr &&
                         g_HudLayoutSW.enabled == 0 &&
                         TestFieldAt<int>(&g_HudLayoutHW, 4) == 0;

    g_HudLayoutSW.ftable = oldSwTable;
    g_HudLayoutSW.enabled = oldSwEnabled;
    TestFieldAt<const void *>(&g_HudLayoutHW, 0) = oldHwTable;
    TestFieldAt<int>(&g_HudLayoutHW, 4) = oldHwEnabled;
    g_HudUiMgrCurrentLayout = oldCurrentLayout;
    g_HudUiMgrHudLayoutsInitialized = oldLayoutsInitialized;
    g_HudUiMgr.enabled = oldHudEnabled;
    g_HudUiMgrLayoutDelayFrames = oldLayoutDelay;
    ZOPT_HUD_TYPE_SW = oldHudTypeSw;
    ZOPT_HUD_TYPE_HW = oldHudTypeHw;
    g_zOpt_HwMode = oldHwMode;

    return uninitialized && swSwitch && hwSwitch && ignored ? 0 : 1;
}

extern "C" int zhud_mgr_ensure_hud_loaded_minimal_smoke(void) {
    zArchiveList *const oldMountedArchives = g_zArchive_MountedList;
    zArchiveList *const oldCurrentArchive = g_zArchive_Current;
    zArchiveList *const oldMissionResourcePaths = g_zImage_MissionResourcePaths;
    HudUiTimerPanelFloat *const oldFloatTimer = g_HudUiMgrTimerPanelFloat;
    HudUiStringMenu *const oldStringMenu = g_HudUiMgrStringMenu;
    HudLayoutBase *const oldCurrentLayout = g_HudUiMgrCurrentLayout;
    const HudLayoutBase oldLayoutSW = g_HudLayoutSW;
    const int oldHudLoaded = g_HudUiMgrHudLoaded;
    const int oldHudEnabled = g_HudUiMgr.enabled;
    const int oldLayoutDelay = g_HudUiMgrLayoutDelayFrames;
    const int oldActiveModeCounter = g_HudUiMgrActiveModeCounterIndex;

    const void *savedSensorPanelFTable = TestFieldAt<const void *>(&g_HudUiMgrSensorPanel, 0);
    const void *savedSensorOverlayFTable = TestFieldAt<const void *>(&g_HudUiMgrSensorOverlay, 0);
    const void *savedSensorMeterFTable = TestFieldAt<const void *>(&g_HudUiMgrSensorMeter, 0);
    const void *savedObjectiveWidgetFTable =
        TestFieldAt<const void *>(&g_HudUiMgrObjectiveWidget, 0);
    const void *savedObjectiveSensorRectFTable =
        TestFieldAt<const void *>(&g_HudUiMgrObjectiveSensorRect, 0);
    const void *savedObjectiveBarFTable = TestFieldAt<const void *>(&g_HudUiMgrObjectiveBar, 0);
    const void *savedReticleWidgetFTable = TestFieldAt<const void *>(&g_HudUiMgrReticleWidget, 0);
    const void *savedNanitePanelFTable = TestFieldAt<const void *>(&g_HudUiMgrNanitePanel, 0);
    const void *savedMessageFTables[10] = {};
    const void *savedCounterFTables[4] = {};
    const void *savedSlotFTables[32] = {};
    for (int index = 0; index < 10; ++index) {
        savedMessageFTables[index] = TestFieldAt<const void *>(&g_HudUiMgrMessages[index], 0);
    }
    for (int index = 0; index < 4; ++index) {
        savedCounterFTables[index] = TestFieldAt<const void *>(&g_HudUiMgrModeCounters[index], 0);
    }
    for (int index = 0; index < 32; ++index) {
        savedSlotFTables[index] = TestFieldAt<const void *>(&g_HudUiMgrWeaponSlots[index], 0);
    }

    g_zArchive_MountedList = nullptr;
    g_HudUiMgrHudLoaded = 1;
    const bool alreadyLoaded = HudUiMgr::EnsureHudLoaded("missing-hud.zar") == 1;

    g_HudUiMgrHudLoaded = 0;
    const bool missingRoot = HudUiMgr::EnsureHudLoaded("missing-hud.zar") == 0 &&
                             g_HudUiMgrHudLoaded == 0;

    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zhd", 0, tempPath) == 0) {
        g_zArchive_MountedList = oldMountedArchives;
        g_HudUiMgrHudLoaded = oldHudLoaded;
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        g_zArchive_MountedList = oldMountedArchives;
        g_HudUiMgrHudLoaded = oldHudLoaded;
        return 2;
    }

    TestWriteU32(file, zReader::ZRDR_NODE_ARRAY);
    TestWriteU32(file, 1);
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "hud.zar");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;

    if (g_zImage_MissionResourcePaths != nullptr) {
        g_zImage_MissionResourcePaths = nullptr;
    }

    HudUiTimerPanelFloat_FTable timerTable{};
    timerTable.slots[24] = MethodAddress(&TestDisableVisibleReceiver::SetVisible);
    HudUiTimerPanelFloat timer{};
    *reinterpret_cast<const HudUiTimerPanelFloat_FTable **>(&timer) = &timerTable;

    HudUiContainer_FTable stringMenuTable{};
    AssignMethodSlot(stringMenuTable.setEnabled, &TestDisableContainer::SetEnabled);
    HudUiStringMenu menu{};
    menu.base.vptr = &stringMenuTable;

    HudLayoutBase_FTable layoutTable{};
    layoutTable.slots[2] = MethodAddress(&HudLayoutBase::SetActiveNoOp);
    layoutTable.OnActivated = TestLayoutOnActivated;
    g_HudLayoutSW.ftable = &layoutTable;

    TestFieldAt<const void *>(&g_HudUiMgrSensorPanel, 0) = &g_HudUiWidget_FTable;
    TestFieldAt<const void *>(&g_HudUiMgrSensorOverlay, 0) = &g_HudUiWidget_FTable;
    TestFieldAt<const void *>(&g_HudUiMgrSensorMeter, 0) = &g_HudUiMeter_FTable;
    TestFieldAt<const void *>(&g_HudUiMgrObjectiveWidget, 0) = &g_HudUiWidget_FTable;
    TestFieldAt<const void *>(&g_HudUiMgrObjectiveSensorRect, 0) = &g_HudUiWidget_FTable;
    TestFieldAt<const void *>(&g_HudUiMgrObjectiveBar, 0) = &g_HudUiBar_FTable;
    TestFieldAt<const void *>(&g_HudUiMgrReticleWidget, 0) = &g_HudUiWidget_FTable;
    TestFieldAt<const void *>(&g_HudUiMgrNanitePanel, 0) = &g_HudUiTripletPanel_FTable;
    for (int index = 0; index < 10; ++index) {
        TestFieldAt<const void *>(&g_HudUiMgrMessages[index], 0) = &g_HudUiMessage_FTable;
    }
    for (int index = 0; index < 4; ++index) {
        TestFieldAt<const void *>(&g_HudUiMgrModeCounters[index], 0) = &g_HudUiCounter_FTable;
    }
    for (int index = 0; index < 32; ++index) {
        TestFieldAt<const void *>(&g_HudUiMgrWeaponSlots[index], 0) = &g_HudUiSlot_FTable;
    }

    std::memset(g_disableVisibleThis, 0, sizeof(g_disableVisibleThis));
    std::memset(g_disableVisibleValue, 0, sizeof(g_disableVisibleValue));
    std::memset(g_disableSetEnabledThis, 0, sizeof(g_disableSetEnabledThis));
    std::memset(g_disableSetEnabledValues, 0, sizeof(g_disableSetEnabledValues));
    g_disableVisibleCount = 0;
    g_disableSetEnabledCount = 0;
    g_disableSetEnabledValue = -1;
    g_layoutActivatedCount = 0;

    g_zArchive_MountedList = &list;
    g_zArchive_Current = nullptr;
    g_HudUiMgrTimerPanelFloat = &timer;
    g_HudUiMgrStringMenu = &menu;
    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgr.enabled = 0;
    g_HudUiMgrLayoutDelayFrames = 0;
    g_HudUiMgrActiveModeCounterIndex = 0;

    const int loadedResult = HudUiMgr::EnsureHudLoaded("C:\\dummy\\hud.zar");
    const bool loaded = loadedResult == 1 && g_HudUiMgrHudLoaded == 1 &&
                        g_HudUiMgrCurrentLayout == &g_HudLayoutSW &&
                        g_HudUiMgrLayoutDelayFrames == 2 &&
                        g_disableVisibleCount == 1 && g_disableVisibleThis[0] == &timer &&
                        g_disableVisibleValue[0] == 0 && g_disableSetEnabledCount == 1 &&
                        g_disableSetEnabledThis[0] == &menu.base &&
                        g_disableSetEnabledValues[0] == 0 && g_layoutActivatedCount == 1 &&
                        g_zImage_MissionResourcePaths != nullptr;

    if (g_zImage_MissionResourcePaths != nullptr) {
        zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionResourcePaths);
    }
    g_zImage_MissionResourcePaths = oldMissionResourcePaths;
    g_zArchive_MountedList = oldMountedArchives;
    g_zArchive_Current = oldCurrentArchive;
    g_HudUiMgrTimerPanelFloat = oldFloatTimer;
    g_HudUiMgrStringMenu = oldStringMenu;
    g_HudUiMgrCurrentLayout = oldCurrentLayout;
    g_HudLayoutSW = oldLayoutSW;
    g_HudUiMgrHudLoaded = oldHudLoaded;
    g_HudUiMgr.enabled = oldHudEnabled;
    g_HudUiMgrLayoutDelayFrames = oldLayoutDelay;
    g_HudUiMgrActiveModeCounterIndex = oldActiveModeCounter;
    TestFieldAt<const void *>(&g_HudUiMgrSensorPanel, 0) = savedSensorPanelFTable;
    TestFieldAt<const void *>(&g_HudUiMgrSensorOverlay, 0) = savedSensorOverlayFTable;
    TestFieldAt<const void *>(&g_HudUiMgrSensorMeter, 0) = savedSensorMeterFTable;
    TestFieldAt<const void *>(&g_HudUiMgrObjectiveWidget, 0) = savedObjectiveWidgetFTable;
    TestFieldAt<const void *>(&g_HudUiMgrObjectiveSensorRect, 0) =
        savedObjectiveSensorRectFTable;
    TestFieldAt<const void *>(&g_HudUiMgrObjectiveBar, 0) = savedObjectiveBarFTable;
    TestFieldAt<const void *>(&g_HudUiMgrReticleWidget, 0) = savedReticleWidgetFTable;
    TestFieldAt<const void *>(&g_HudUiMgrNanitePanel, 0) = savedNanitePanelFTable;
    for (int index = 0; index < 10; ++index) {
        TestFieldAt<const void *>(&g_HudUiMgrMessages[index], 0) = savedMessageFTables[index];
    }
    for (int index = 0; index < 4; ++index) {
        TestFieldAt<const void *>(&g_HudUiMgrModeCounters[index], 0) =
            savedCounterFTables[index];
    }
    for (int index = 0; index < 32; ++index) {
        TestFieldAt<const void *>(&g_HudUiMgrWeaponSlots[index], 0) = savedSlotFTables[index];
    }

    CloseHandle(file);
    DeleteFileA(tempPath);
    return alreadyLoaded && missingRoot && loaded ? 0 : 3;
}

extern "C" int zhud_element_constructor_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;

    HudUiCommon_FTable *const commonTable =
        const_cast<HudUiCommon_FTable *>(&g_HudUiCommon_FTable);
    const unsigned int originalInvalidate = commonTable->slots[8];
    DWORD oldProtect = 0;
    if (VirtualProtect(commonTable, sizeof(*commonTable), PAGE_READWRITE, &oldProtect) == 0) {
        g_HudUi_InvalidateMask = 0;
        return 1;
    }

    commonTable->slots[8] = reinterpret_cast<std::uintptr_t>(TestElementConstructorInvalidate);
    g_elementConstructorInvalidateCount = 0;
    g_elementConstructorInvalidateThis = nullptr;

    HudUiElement element{};
    element.Constructor(17, 29);

    const bool initialized = element.ftable == &g_HudUiCommon_FTable && element.next == nullptr &&
                             element.parent == nullptr && element.flags == 0 &&
                             element.timer == 0.0f && element.x == 17 && element.y == 29 &&
                             element.bltSource == nullptr && element.state == 0;
    const bool invalidated = g_elementConstructorInvalidateCount == 1 &&
                             g_elementConstructorInvalidateThis == &element;

    commonTable->slots[8] = originalInvalidate;
    DWORD ignoredProtect = 0;
    VirtualProtect(commonTable, sizeof(*commonTable), oldProtect, &ignoredProtect);

    g_HudUi_InvalidateMask = 0;
    return initialized && invalidated ? 0 : 1;
}

extern "C" int zhud_element_copy_constructor_smoke(void) {
    HudUiElement source{};
    source.ftable = nullptr;
    source.next = reinterpret_cast<HudUiElement *>(0x1111);
    source.parent = reinterpret_cast<void *>(0x2222);
    source.flags = 0x1234;
    source.timer = 2.5f;
    source.x = 17;
    source.y = 29;
    source.bltSource = reinterpret_cast<void *>(0x3333);
    source.clipRect.left = 1;
    source.clipRect.top = 2;
    source.clipRect.right = 3;
    source.clipRect.bottom = 4;
    source.state = 5;
    source.padding32 = 6;

    HudUiElement copied{};
    copied.next = reinterpret_cast<HudUiElement *>(0xaaaa);
    copied.parent = reinterpret_cast<void *>(0xbbbb);
    copied.padding32 = 0xcccc;

    HudUiRect copiedRect{-1, -1, -1, -1};
    HudUiElement *const copiedResult = copied.CopyConstructor(&source);
    copiedResult->GetRect(&copiedRect);
    const bool copiedCoords = copiedResult->GetX() == source.x && copiedResult->GetY() == source.y;

    HudUiElement assigned{};
    assigned.ftable = &g_HudUiCircle_FTable;
    assigned.next = reinterpret_cast<HudUiElement *>(0xaaaa);
    assigned.parent = reinterpret_cast<void *>(0xbbbb);
    assigned.padding32 = 0xdddd;
    HudUiElement *const assignedResult = assigned.CopyFrom(&source);

    HudUiElement scalar{};
    scalar.ftable = nullptr;
    HudUiElement *const scalarResult = scalar.ScalarDeletingDestructor(0);

    return copiedResult == &copied && copied.ftable == &g_HudUiCommon_FTable &&
                   copied.next == nullptr && copied.parent == nullptr &&
                   copied.flags == source.flags && copied.timer == source.timer &&
                   copied.x == source.x && copied.y == source.y &&
                   copied.bltSource == source.bltSource &&
                   copied.clipRect.left == source.clipRect.left &&
                   copied.clipRect.top == source.clipRect.top &&
                   copied.clipRect.right == source.clipRect.right &&
                   copied.clipRect.bottom == source.clipRect.bottom &&
                   copiedRect.left == source.x && copiedRect.top == source.y &&
                   copiedRect.right == source.x && copiedRect.bottom == source.y && copiedCoords &&
                   copied.state == source.state && copied.padding32 == 0xcccc &&
                   assignedResult == &assigned && assigned.ftable == &g_HudUiCircle_FTable &&
                   assigned.next == nullptr && assigned.parent == nullptr &&
                   assigned.flags == source.flags && assigned.timer == source.timer &&
                   assigned.x == source.x && assigned.y == source.y &&
                   assigned.bltSource == source.bltSource &&
                   assigned.clipRect.left == source.clipRect.left &&
                   assigned.clipRect.bottom == source.clipRect.bottom &&
                   assigned.state == source.state && assigned.padding32 == 0xdddd &&
                   scalarResult == &scalar && scalar.ftable == &g_HudUiCommon_FTable
               ? 0
               : 1;
}

extern "C" int zhud_element_set_timer_smoke(void) {
    HudUiElement element{};
    element.flags = 0x20;

    element.SetTimer(0.5f);
    const bool active = element.timer == 0.5f && (element.flags & 0x01u) != 0 &&
                        (element.flags & 0x10u) == 0 && (element.flags & 0x20u) != 0;

    element.SetTimer(-1.0f);
    const bool expired = element.timer == -1.0f && (element.flags & 0x01u) == 0 &&
                         (element.flags & 0x10u) != 0 && (element.flags & 0x20u) != 0;

    return active && expired ? 0 : 1;
}

extern "C" int zhud_circle_constructor_and_hit_test_smoke(void) {
    HudUiCircle circle{};
    HudUiCircle *const result = circle.Constructor(10, 20, 5, 0x07e0);

    const bool constructed = result == &circle && circle.base.ftable == &g_HudUiCircle_FTable &&
                             circle.base.x == 10 && circle.base.y == 20 && circle.radius == 5 &&
                             circle.radiusSquared == 25 && circle.color565 == 0x07e0;

    const bool hitCore = circle.HitTestCore(10, 20) == 1 && circle.HitTestCore(13, 23) == 1 &&
                         circle.HitTestCore(15, 20) == 0 && circle.HitTestCore(16, 20) == 0;
    const bool hitWrapped = circle.HitTest(10, 20) == 1 && circle.HitTest(16, 20) == 0;

    return constructed && hitCore && hitWrapped ? 0 : 1;
}

extern "C" int zhud_composite_panel_vector_clear_smoke(void) {
    HudUiCommon_FTable table{};
    table.slots[0] = MethodAddress(&TestCompositePanelEntry::ScalarDeletingDestructor);

    g_compositeEntryDestructorCount = 0;
    std::memset(g_compositeEntryDestructorThis, 0, sizeof(g_compositeEntryDestructorThis));
    std::memset(g_compositeEntryDestructorFlags, 0xff, sizeof(g_compositeEntryDestructorFlags));

    auto *entries = static_cast<HudUiCompositePanelEntry *>(
        ::operator new(sizeof(HudUiCompositePanelEntry) * 2));
    TestFieldAt<const HudUiCommon_FTable *>(&entries[0], 0) = &table;
    TestFieldAt<const HudUiCommon_FTable *>(&entries[1], 0) = &table;

    HudUiCompositePanelVector vector{};
    vector.allocatorStorage = 0x12345678;
    vector.begin = entries;
    vector.end = entries + 2;
    vector.capacityEnd = entries + 2;

    vector.Clear();

    return g_compositeEntryDestructorCount == 2 &&
                   g_compositeEntryDestructorThis[0] == &entries[0] &&
                   g_compositeEntryDestructorThis[1] == &entries[1] &&
                   g_compositeEntryDestructorFlags[0] == 0 &&
                   g_compositeEntryDestructorFlags[1] == 0 &&
                   vector.allocatorStorage == 0x12345678 && vector.begin == nullptr &&
                   vector.end == nullptr && vector.capacityEnd == nullptr
               ? 0
               : 1;
}

extern "C" int zhud_composite_panel_vector_insert_copies_smoke(void) {
    HudUiCommon_FTable destructorTable{};
    destructorTable.slots[0] = MethodAddress(&TestCompositePanelEntry::ScalarDeletingDestructor);

    HudUiCompositePanelEntry templateEntry{};
    InitCompositePanelEntryForTest(&templateEntry, 90);

    HudUiCompositePanelVector growVector{};
    growVector.begin = AllocateCompositePanelEntriesForTest(2);
    growVector.end = growVector.begin + 2;
    growVector.capacityEnd = growVector.end;
    InitCompositePanelEntryForTest(&growVector.begin[0], 1);
    InitCompositePanelEntryForTest(&growVector.begin[1], 2);
    TestFieldAt<const HudUiCommon_FTable *>(&growVector.begin[0], 0) = &destructorTable;
    TestFieldAt<const HudUiCommon_FTable *>(&growVector.begin[1], 0) = &destructorTable;
    growVector.InsertCopies(growVector.begin + 1, 2, &templateEntry);
    const bool growOk =
        growVector.end == growVector.begin + 4 && growVector.capacityEnd == growVector.begin + 4 &&
        CompositePanelEntryMarkerForTest(growVector.begin[0]) == 1 &&
        CompositePanelEntryMarkerForTest(growVector.begin[1]) == 90 &&
        CompositePanelEntryMarkerForTest(growVector.begin[2]) == 90 &&
        CompositePanelEntryMarkerForTest(growVector.begin[3]) == 2 &&
        reinterpret_cast<HudUiPanel *>(&growVector.begin[1])->vtbl ==
            &g_HudUiTransitionTextPanel_FTable;
    DestroyCompositePanelVectorEntriesForTest(&growVector);

    HudUiCompositePanelVector longTailVector{};
    longTailVector.begin = AllocateCompositePanelEntriesForTest(5);
    longTailVector.end = longTailVector.begin + 3;
    longTailVector.capacityEnd = longTailVector.begin + 5;
    InitCompositePanelEntryForTest(&longTailVector.begin[0], 10);
    InitCompositePanelEntryForTest(&longTailVector.begin[1], 11);
    InitCompositePanelEntryForTest(&longTailVector.begin[2], 12);
    longTailVector.InsertCopies(longTailVector.begin + 1, 1, &templateEntry);
    const bool longTailOk =
        longTailVector.end == longTailVector.begin + 4 &&
        longTailVector.capacityEnd == longTailVector.begin + 5 &&
        CompositePanelEntryMarkerForTest(longTailVector.begin[0]) == 10 &&
        CompositePanelEntryMarkerForTest(longTailVector.begin[1]) == 90 &&
        CompositePanelEntryMarkerForTest(longTailVector.begin[2]) == 11 &&
        CompositePanelEntryMarkerForTest(longTailVector.begin[3]) == 12;
    DestroyCompositePanelVectorEntriesForTest(&longTailVector);

    HudUiCompositePanelVector shortTailVector{};
    shortTailVector.begin = AllocateCompositePanelEntriesForTest(5);
    shortTailVector.end = shortTailVector.begin + 2;
    shortTailVector.capacityEnd = shortTailVector.begin + 5;
    InitCompositePanelEntryForTest(&shortTailVector.begin[0], 20);
    InitCompositePanelEntryForTest(&shortTailVector.begin[1], 21);
    shortTailVector.InsertCopies(shortTailVector.begin + 1, 3, &templateEntry);
    const bool shortTailOk =
        shortTailVector.end == shortTailVector.begin + 5 &&
        shortTailVector.capacityEnd == shortTailVector.begin + 5 &&
        CompositePanelEntryMarkerForTest(shortTailVector.begin[0]) == 20 &&
        CompositePanelEntryMarkerForTest(shortTailVector.begin[1]) == 90 &&
        CompositePanelEntryMarkerForTest(shortTailVector.begin[2]) == 90 &&
        CompositePanelEntryMarkerForTest(shortTailVector.begin[3]) == 90 &&
        CompositePanelEntryMarkerForTest(shortTailVector.begin[4]) == 21;
    DestroyCompositePanelVectorEntriesForTest(&shortTailVector);

    DestroyCompositePanelEntryForTest(&templateEntry);
    return growOk && longTailOk && shortTailOk ? 0 : 1;
}

extern "C" int zhud_composite_panel_constructor_with_entry_count_smoke(void) {
    g_HudUi_InvalidateMask = 1;

    HudUiCompositePanel panel{};
    HudUiCompositePanel *const result = panel.ConstructorWithEntryCount(2);
    auto *const panelAsPanel = reinterpret_cast<HudUiPanel *>(&panel);

    const bool panelInitialized =
        result == &panel && panelAsPanel->vtbl == &g_HudUiCompositePanel_FTable &&
        panel.activeEntryCount == 0 && panel.entryVector.begin != nullptr &&
        panel.entryVector.end == panel.entryVector.begin + 2 &&
        panel.entryVector.capacityEnd == panel.entryVector.begin + 2 &&
        (TestFieldAt<std::uint32_t>(&panel, 0x0c) & 0x10u) == 0;

    const bool entriesInitialized =
        reinterpret_cast<HudUiPanel *>(&panel.entryVector.begin[0])->vtbl ==
            &g_HudUiTransitionTextPanel_FTable &&
        reinterpret_cast<HudUiPanel *>(&panel.entryVector.begin[1])->vtbl ==
            &g_HudUiTransitionTextPanel_FTable &&
        TestFieldAt<char>(&panel.entryVector.begin[0], 0x34) == '\0' &&
        TestFieldAt<char>(&panel.entryVector.begin[1], 0x34) == '\0' &&
        TestFieldAt<std::int32_t>(&panel.entryVector.begin[0], 0x14) == 0 &&
        TestFieldAt<std::int32_t>(&panel.entryVector.begin[0], 0x18) == 0 &&
        TestFieldAt<std::int32_t>(&panel.entryVector.begin[1], 0x14) == 0 &&
        TestFieldAt<std::int32_t>(&panel.entryVector.begin[1], 0x18) == 0 &&
        (TestFieldAt<std::uint32_t>(&panel.entryVector.begin[0], 0x0c) & 0x10u) != 0 &&
        (TestFieldAt<std::uint32_t>(&panel.entryVector.begin[1], 0x0c) & 0x10u) != 0;

    DestroyCompositePanelVectorEntriesForTest(&panel.entryVector);
    g_HudUi_InvalidateMask = 0;
    return panelInitialized && entriesInitialized ? 0 : 1;
}

extern "C" int zhud_composite_panel_layout_entries_smoke(void) {
    g_HudUi_InvalidateMask = 1;

    HudUiCompositePanel panel{};
    auto *const panelAsPanel = reinterpret_cast<HudUiPanel *>(&panel);
    panelAsPanel->vtbl = &g_HudUiPanel_FTable;
    TestFieldAt<std::uint32_t>(panelAsPanel, 0x270) = 0;
    TestFieldAt<std::int32_t>(panelAsPanel, 0x260) = 14;
    TestFieldAt<std::int32_t>(panelAsPanel, 0x274) = 2;

    HudUiCompositePanelEntry *const entries = AllocateCompositePanelEntriesForTest(2);
    InitCompositePanelEntryForTest(&entries[0], 30);
    InitCompositePanelEntryForTest(&entries[1], 31);
    panel.entryVector.begin = entries;
    panel.entryVector.end = entries + 2;
    panel.entryVector.capacityEnd = entries + 2;

    panel.LayoutEntries(40, 50);

    const bool panelPositioned =
        TestFieldAt<std::int32_t>(&panel, 0x14) == 40 &&
        TestFieldAt<std::int32_t>(&panel, 0x18) == 50 &&
        (TestFieldAt<std::uint32_t>(&panel, 0x0c) & 1u) != 0;
    const bool entriesPositioned =
        TestFieldAt<std::int32_t>(&entries[0], 0x14) == 40 &&
        TestFieldAt<std::int32_t>(&entries[0], 0x18) == 50 &&
        TestFieldAt<std::int32_t>(&entries[1], 0x14) == 40 &&
        TestFieldAt<std::int32_t>(&entries[1], 0x18) == 62;

    DestroyCompositePanelVectorEntriesForTest(&panel.entryVector);
    g_HudUi_InvalidateMask = 0;
    int failure = 0;
    failure |= panelPositioned ? 0 : 1;
    failure |= entriesPositioned ? 0 : 2;
    return failure;
}

extern "C" int zhud_composite_panel_resize_entry_count_smoke(void) {
    g_HudUi_InvalidateMask = 1;

    HudUiCompositePanel panel{};
    HudUiCompositePanelEntry *const entries = AllocateCompositePanelEntriesForTest(3);
    for (int index = 0; index < 3; ++index) {
        InitCompositePanelEntryForTest(&entries[index], 40 + index);
        reinterpret_cast<HudUiPanel *>(&entries[index])->SetText("occupied");
        TestFieldAt<std::uint32_t>(&entries[index], 0x0c) &= ~0x10u;
    }

    panel.entryVector.begin = entries;
    panel.entryVector.end = entries + 3;
    panel.entryVector.capacityEnd = entries + 3;
    panel.activeEntryCount = 99;
    panel.ResizeEntryCount(1, 3);

    const bool clampedAndCleared =
        panel.activeEntryCount == 1 &&
        std::strcmp(&TestFieldAt<char>(&entries[0], 0x34), "occupied") == 0 &&
        TestFieldAt<char>(&entries[1], 0x34) == '\0' &&
        TestFieldAt<char>(&entries[2], 0x34) == '\0' &&
        (TestFieldAt<std::uint32_t>(&entries[1], 0x0c) & 0x10u) != 0 &&
        (TestFieldAt<std::uint32_t>(&entries[2], 0x0c) & 0x10u) != 0;

    reinterpret_cast<HudUiPanel *>(&entries[1])->SetText("again");
    reinterpret_cast<HudUiPanel *>(&entries[2])->SetText("again");
    TestFieldAt<std::uint32_t>(&entries[1], 0x0c) &= ~0x10u;
    TestFieldAt<std::uint32_t>(&entries[2], 0x0c) &= ~0x10u;
    panel.activeEntryCount = 2;
    panel.ReapplyEntryCount();
    const bool reapplied =
        panel.activeEntryCount == 0 &&
        TestFieldAt<char>(&entries[0], 0x34) == '\0' &&
        TestFieldAt<char>(&entries[1], 0x34) == '\0' &&
        TestFieldAt<char>(&entries[2], 0x34) == '\0' &&
        (TestFieldAt<std::uint32_t>(&entries[0], 0x0c) & 0x10u) != 0 &&
        (TestFieldAt<std::uint32_t>(&entries[1], 0x0c) & 0x10u) != 0 &&
        (TestFieldAt<std::uint32_t>(&entries[2], 0x0c) & 0x10u) != 0;

    DestroyCompositePanelVectorEntriesForTest(&panel.entryVector);
    g_HudUi_InvalidateMask = 0;
    return clampedAndCleared && reapplied ? 0 : 1;
}

extern "C" int zhud_composite_panel_resize_vector_relayout_smoke(void) {
    g_HudUi_InvalidateMask = 1;
    g_compositeEntryDestructorCount = 0;
    std::memset(g_compositeEntryDestructorThis, 0, sizeof(g_compositeEntryDestructorThis));
    std::memset(g_compositeEntryDestructorFlags, 0xff, sizeof(g_compositeEntryDestructorFlags));

    HudUiCommon_FTable destructorTable{};
    destructorTable.slots[0] = MethodAddress(&TestCompositePanelEntry::ScalarDeletingDestructor);

    HudUiCompositePanel panel{};
    auto *const panelAsPanel = reinterpret_cast<HudUiPanel *>(&panel);
    panelAsPanel->vtbl = &g_HudUiPanel_FTable;
    TestFieldAt<std::int32_t>(&panel, 0x14) = 10;
    TestFieldAt<std::int32_t>(&panel, 0x18) = 20;
    TestFieldAt<std::uint32_t>(panelAsPanel, 0x270) = 0;
    TestFieldAt<std::int32_t>(panelAsPanel, 0x260) = 15;
    TestFieldAt<std::int32_t>(panelAsPanel, 0x274) = 3;

    panel.ResizeEntryVectorAndRelayout(2);
    const bool grown =
        panel.entryVector.begin != nullptr && panel.entryVector.end == panel.entryVector.begin + 2 &&
        panel.entryVector.capacityEnd == panel.entryVector.begin + 2 && panel.activeEntryCount == 0 &&
        TestFieldAt<std::int32_t>(&panel.entryVector.begin[0], 0x14) == 10 &&
        TestFieldAt<std::int32_t>(&panel.entryVector.begin[0], 0x18) == 20 &&
        TestFieldAt<std::int32_t>(&panel.entryVector.begin[1], 0x14) == 10 &&
        TestFieldAt<std::int32_t>(&panel.entryVector.begin[1], 0x18) == 32 &&
        (TestFieldAt<std::uint32_t>(&panel.entryVector.begin[0], 0x0c) & 0x10u) != 0 &&
        (TestFieldAt<std::uint32_t>(&panel.entryVector.begin[1], 0x0c) & 0x10u) != 0;

    TestFieldAt<const HudUiCommon_FTable *>(&panel.entryVector.begin[1], 0) = &destructorTable;
    panel.ResizeEntryVectorAndRelayout(1);
    const bool shrunk =
        panel.entryVector.end == panel.entryVector.begin + 1 && panel.activeEntryCount == 1 &&
        g_compositeEntryDestructorCount == 1 &&
        g_compositeEntryDestructorThis[0] == &panel.entryVector.begin[1] &&
        g_compositeEntryDestructorFlags[0] == 0 &&
        TestFieldAt<std::int32_t>(&panel.entryVector.begin[0], 0x14) == 10 &&
        TestFieldAt<std::int32_t>(&panel.entryVector.begin[0], 0x18) == 20;

    reinterpret_cast<HudUiPanel *>(&panel.entryVector.begin[0])->SetText("again");
    TestFieldAt<std::uint32_t>(&panel.entryVector.begin[0], 0x0c) &= ~0x10u;
    panel.activeEntryCount = 1;
    panel.ResizeEntryVectorAndRelayout(1);
    const bool reapplied =
        panel.entryVector.end == panel.entryVector.begin + 1 && panel.activeEntryCount == 0 &&
        TestFieldAt<char>(&panel.entryVector.begin[0], 0x34) == '\0' &&
        (TestFieldAt<std::uint32_t>(&panel.entryVector.begin[0], 0x0c) & 0x10u) != 0;

    DestroyCompositePanelVectorEntriesForTest(&panel.entryVector);
    g_HudUi_InvalidateMask = 0;
    return grown && shrunk && reapplied ? 0 : 1;
}

extern "C" int zhud_composite_panel_entry_copy_smoke(void) {
    HudUiCompositePanelEntry source{};
    auto *sourcePanel = reinterpret_cast<HudUiPanel *>(&source);
    sourcePanel->ConstructorDefault("entry", 7, 9);
    TestFieldAt<std::uint32_t>(sourcePanel, 0x14c) = 0x010203;
    TestFieldAt<std::uint32_t>(sourcePanel, 0x150) = 0x040506;
    TestFieldAt<std::uint32_t>(sourcePanel, 0x270) = 4;
    source.panel.flashCountdown = 1.25f;
    source.panel.flashResetValue = 2.5f;
    source.panel.flashAltColor0 = 0x111111;
    source.panel.flashAltColor1 = 0x222222;
    source.panel.flashEnabled = 3;
    source.panel.flashMode = 4;
    source.panel.flashDirectionSign = -1;

    HudUiCompositePanelEntry assigned{};
    auto *assignedPanel = reinterpret_cast<HudUiPanel *>(&assigned);
    assignedPanel->vtbl = &g_HudUiCommon_FTable;
    HudUiCompositePanelEntry *const assignedResult = assigned.AssignCopy(&source);
    const bool assignedOk =
        assignedResult == &assigned && assignedPanel->vtbl == &g_HudUiCommon_FTable &&
        TestFieldAt<std::uint32_t>(assignedPanel, 0x14c) == 0x010203 &&
        TestFieldAt<std::uint32_t>(assignedPanel, 0x150) == 0x040506 &&
        TestFieldAt<std::uint32_t>(assignedPanel, 0x270) == 1 &&
        assigned.panel.flashCountdown == 1.25f && assigned.panel.flashResetValue == 2.5f &&
        assigned.panel.flashAltColor0 == 0x111111 && assigned.panel.flashAltColor1 == 0x222222 &&
        assigned.panel.flashEnabled == 3 && assigned.panel.flashMode == 4 &&
        assigned.panel.flashDirectionSign == -1;

    HudUiCompositePanelEntry copied{};
    auto *copiedPanel = reinterpret_cast<HudUiPanel *>(&copied);
    HudUiCompositePanelEntry *const copiedResult = copied.ConstructorCopy(&source);
    const bool copiedOk =
        copiedResult == &copied && copiedPanel->vtbl == &g_HudUiTransitionTextPanel_FTable &&
        TestFieldAt<std::uint32_t>(copiedPanel, 0x14c) == 0x010203 &&
        TestFieldAt<std::uint32_t>(copiedPanel, 0x150) == 0x040506 &&
        TestFieldAt<std::uint32_t>(copiedPanel, 0x270) == 4 &&
        copied.panel.flashCountdown == 1.25f && copied.panel.flashResetValue == 2.5f &&
        copied.panel.flashAltColor0 == 0x111111 && copied.panel.flashAltColor1 == 0x222222 &&
        copied.panel.flashEnabled == 3 && copied.panel.flashMode == 4 &&
        copied.panel.flashDirectionSign == -1;

    HudUiCompositePanelEntry rangeSource[2]{};
    HudUiCompositePanelEntry rangeDest[2]{};
    for (int index = 0; index < 2; ++index) {
        auto *panel = reinterpret_cast<HudUiPanel *>(&rangeSource[index]);
        panel->ConstructorDefault(index == 0 ? "r0" : "r1", index, index + 1);
        TestFieldAt<std::uint32_t>(panel, 0x14c) = 0x10 + index;
        rangeSource[index].panel.flashCountdown = static_cast<float>(index + 1);
        rangeSource[index].panel.flashDirectionSign = -index;
        reinterpret_cast<HudUiPanel *>(&rangeDest[index])->vtbl = &g_HudUiCommon_FTable;
    }

    HudUiCompositePanelEntry *const rangeEnd =
        HudUiCompositePanelEntry::ConstructorCopyRange(rangeSource, rangeSource + 2, rangeDest);
    const bool rangeOk =
        rangeEnd == rangeDest + 2 &&
        reinterpret_cast<HudUiPanel *>(&rangeDest[0])->vtbl == &g_HudUiCommon_FTable &&
        reinterpret_cast<HudUiPanel *>(&rangeDest[1])->vtbl == &g_HudUiCommon_FTable &&
        TestFieldAt<std::uint32_t>(&rangeDest[0], 0x14c) == 0x10 &&
        TestFieldAt<std::uint32_t>(&rangeDest[1], 0x14c) == 0x11 &&
        rangeDest[0].panel.flashCountdown == 1.0f && rangeDest[1].panel.flashCountdown == 2.0f &&
        rangeDest[0].panel.flashDirectionSign == 0 && rangeDest[1].panel.flashDirectionSign == -1;

    for (int index = 0; index < 2; ++index) {
        auto *destPanel = reinterpret_cast<HudUiPanel *>(&rangeDest[index]);
        DeleteObject(destPanel->hFont);
        destPanel->hFont = nullptr;
        auto *sourceRangePanel = reinterpret_cast<HudUiPanel *>(&rangeSource[index]);
        DeleteObject(sourceRangePanel->hFont);
        sourceRangePanel->hFont = nullptr;
    }

    DeleteObject(assignedPanel->hFont);
    assignedPanel->hFont = nullptr;
    DeleteObject(copiedPanel->hFont);
    copiedPanel->hFont = nullptr;
    DeleteObject(sourcePanel->hFont);
    sourcePanel->hFont = nullptr;
    return assignedOk && copiedOk && rangeOk ? 0 : 1;
}

extern "C" int zhud_flash_panel_compute_blend_color_smoke(void) {
    const std::uint32_t from = 0x00102030;
    const std::uint32_t to = 0x00a0c0e0;

    const bool clamps = HudUiFlashPanel::ComputeFlashBlendColor(from, to, 0.0005f) == from &&
                        HudUiFlashPanel::ComputeFlashBlendColor(from, to, 1.0f) == to;
    const bool blends =
        HudUiFlashPanel::ComputeFlashBlendColor(0x00000000, 0x00020406, 0.5f) == 0x00010203;

    return clamps && blends ? 0 : 1;
}

extern "C" int zhud_layout_shutdown_stub_smoke(void) {
    HudUiShieldMessageWidgetState shield{};
    g_HudUiMgrShieldMessageWidget = &shield;
    HudLayoutBase::Shutdown_Stub();
    g_HudUiMgrShieldMessageWidget = nullptr;

    alignas(HudUiWidget) std::uint8_t storage[0x30 + sizeof(HudUiWidget)]{};
    auto *const layout = reinterpret_cast<HudLayoutBase *>(storage);
    auto *const widget = reinterpret_cast<HudUiWidget *>(storage + 0x30);
    layout->ftable = nullptr;
    widget->ftable = &g_HudUiWidget_FTable;
    widget->image = &zVid_Image::g_zImage_DefaultImage;
    widget->ownsImage = 0;

    layout->Destructor();

    alignas(HudUiContainer) std::uint8_t updateStorage[sizeof(HudUiContainer)]{};
    auto *const updateLayout = reinterpret_cast<HudLayoutBase *>(updateStorage);
    auto *const updateContainer = reinterpret_cast<HudUiContainer *>(updateStorage);
    updateContainer->ConstructorDefault();
    updateContainer->enabled = 1;

    HudUiCommon_FTable updateTable{};
    updateTable.slots[9] = MethodAddress(&TestContainerUpdateElement::Update);
    TestContainerUpdateElement updateElement{};
    updateElement.base.ftable = &updateTable;
    updateContainer->AddChild(&updateElement.base);

    g_containerUpdateCount = 0;
    updateLayout->UpdateAll(0.75f);
    const bool layoutUpdated = g_containerUpdateCount == 1 && g_containerUpdateDelta[0] == 0.75f;

    HudLayoutBase noOp{};
    const bool noOpActive = noOp.SetActiveNoOp(0) == 1;

    HudLayoutBase_FTable setActiveTable{};
    setActiveTable.slots[1] = MethodAddress(&TestLayoutSetActiveElement::SetActive);
    TestLayoutSetActiveElement activeElement{};
    activeElement.ftable = &setActiveTable;
    reinterpret_cast<HudLayoutBase *>(&activeElement)->Enable();
    const bool enabled = activeElement.activeValue == 1;
    reinterpret_cast<HudLayoutBase *>(&activeElement)->Disable();
    const bool disabled = activeElement.activeValue == 0;

    return reinterpret_cast<HudUiContainer *>(layout)->vptr == &g_HudUiContainer_FTable &&
                   widget->ftable ==
                       reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable) &&
                   widget->image == &zVid_Image::g_zImage_DefaultImage && layoutUpdated &&
                   noOpActive && enabled && disabled
               ? 0
               : 1;
}

extern "C" int zhud_layout_hw_release_images_smoke(void) {
    HudLayoutHW layout{};
    layout.widget1Image320 = &zVid_Image::g_zImage_DefaultImage;
    layout.widget1Image400 = nullptr;
    layout.widget2Image320 = &zVid_Image::g_zImage_DefaultImage;
    layout.widget2Image400 = nullptr;
    layout.unknown_1b4[0] = 0xa5;

    layout.ReleaseImages();

    return layout.widget1Image320 == nullptr && layout.widget1Image400 == nullptr &&
                   layout.widget2Image320 == nullptr && layout.widget2Image400 == nullptr &&
                   layout.unknown_1b4[0] == 0xa5
               ? 0
               : 1;
}

extern "C" int zhud_element_clip_and_invalidate_smoke(void) {
    g_HudUi_InvalidateMask = 0x24;

    HudUiElement element{};
    element.ftable = &g_HudUiCommon_FTable;
    element.flags = 0x01;
    element.Invalidate();

    HudUiRect rect{1, 2, 3, 4};
    element.SetBltSourceAndClipRect(&element, &rect);

    const bool updated = element.flags == 0x25 && element.bltSource == &element &&
                         std::memcmp(&element.clipRect, &rect, sizeof(rect)) == 0;

    g_HudUi_InvalidateMask = 0;
    return updated ? 0 : 1;
}

extern "C" int zhud_element_visible_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;

    HudUiElement element{};
    element.ftable = nullptr;
    element.ResetCommonFTable();
    const bool reset = element.ftable == &g_HudUiCommon_FTable;

    element.flags = 0x10;
    element.SetVisible(1);
    const bool visible = element.flags == 0x80;

    element.SetVisible(0);
    const bool hidden = element.flags == 0x90;

    g_HudUi_InvalidateMask = 0;
    return reset && visible && hidden ? 0 : 1;
}

extern "C" int zhud_element_update_smoke(void) {
    g_elementDrawCount = 0;
    g_elementBaseDrawCount = 0;
    g_HudUi_InvalidateMask = 0x80;

    HudUiCommon_FTable table = {};
    table.slots[1] = reinterpret_cast<std::uintptr_t>(TestElementDraw);
    table.slots[2] = reinterpret_cast<std::uintptr_t>(TestElementBaseDraw);
    table.slots[8] = reinterpret_cast<std::uintptr_t>(TestElementInvalidate);
    table.slots[24] = MethodAddress(&HudUiElement::SetVisible);

    HudUiElement element{};
    element.ftable = &table;
    element.flags = 0;
    element.Update(0.1f);
    const bool visibleDraw = g_elementDrawCount == 1;

    element.flags = 0x02 | 0x04;
    element.Update(0.1f);
    const bool visibleDirty = g_elementDrawCount == 2 && (element.flags & 0x04) == 0;

    element.flags = 0x10 | 0x02 | 0x04;
    element.Update(0.1f);
    const bool hiddenDirty = g_elementBaseDrawCount == 1 && (element.flags & 0x04) == 0;

    element.flags = 0x01;
    element.timer = 0.5f;
    element.Update(0.25f);
    const bool countdownActive = element.timer == 0.25f && (element.flags & 0x10) == 0;
    element.Update(0.25f);
    const bool countdownExpired = element.timer == 0.0f && (element.flags & 0x10) != 0;

    g_HudUi_InvalidateMask = 0;
    return visibleDraw && visibleDirty && hiddenDirty && countdownActive && countdownExpired ? 0
                                                                                             : 1;
}

extern "C" int zhud_element_position_mutators_smoke(void) {
    g_HudUi_InvalidateMask = 0x40;

    HudUiElement element{};
    element.Constructor(1, 2);
    element.flags = 0;

    element.SetPos(3, 4);
    const bool pos = element.x == 3 && element.y == 4 && element.flags == 0x40;

    element.flags = 0;
    element.SetX(5);
    const bool xOnly = element.x == 5 && element.y == 4 && element.flags == 0x40;

    element.flags = 0;
    element.SetY(6);
    const bool yOnly = element.x == 5 && element.y == 6 && element.flags == 0x40;

    g_HudUi_InvalidateMask = 0;
    return pos && xOnly && yOnly ? 0 : 1;
}

extern "C" int zhud_primitive_bind_target_set_segment_endpoints_smoke(void) {
    HudUiCommon_FTable table{};
    table.slots[0x0c / 4] = MethodAddress(&TestPrimitiveBindTarget::SetPos);

    HudUiPrimitiveBindTarget target{};
    target.base.ftable = &table;
    target.endX = -1;
    target.endY = -1;
    g_primitiveSetPosCount = 0;
    g_primitiveSetPosThis = nullptr;
    g_primitiveSetPosX = 0;
    g_primitiveSetPosY = 0;

    target.SetSegmentEndpoints(11, 22, 33, 44);

    return g_primitiveSetPosCount == 1 && g_primitiveSetPosThis == &target &&
                   g_primitiveSetPosX == 11 && g_primitiveSetPosY == 22 &&
                   target.base.x == 11 && target.base.y == 22 && target.endX == 33 &&
                   target.endY == 44
               ? 0
               : 1;
}

extern "C" int zhud_background_bind_primitive_node_to_element_smoke(void) {
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;

    zReader::Node positionItems[3] = {};
    positionItems[0].value.i32 = 3;
    positionItems[1].type = zReader::ZRDR_NODE_INT;
    positionItems[1].value.i32 = 5;
    positionItems[2].type = zReader::ZRDR_NODE_INT;
    positionItems[2].value.i32 = 7;

    zReader::Node colorItems[4] = {};
    colorItems[0].value.i32 = 4;
    colorItems[1].type = zReader::ZRDR_NODE_INT;
    colorItems[1].value.i32 = 0x20;
    colorItems[2].type = zReader::ZRDR_NODE_INT;
    colorItems[2].value.i32 = 0x60;
    colorItems[3].type = zReader::ZRDR_NODE_INT;
    colorItems[3].value.i32 = 0x40;

    zReader::Node endRelItems[3] = {};
    endRelItems[0].value.i32 = 3;
    endRelItems[1].type = zReader::ZRDR_NODE_INT;
    endRelItems[1].value.i32 = 2;
    endRelItems[2].type = zReader::ZRDR_NODE_INT;
    endRelItems[2].value.i32 = 3;

    zReader::Node primitiveItems[7] = {};
    primitiveItems[0].value.i32 = 7;
    primitiveItems[1].type = zReader::ZRDR_NODE_STRING;
    primitiveItems[1].value.str = const_cast<char *>("POSITION");
    primitiveItems[2].type = zReader::ZRDR_NODE_ARRAY;
    primitiveItems[2].value.nodes = positionItems;
    primitiveItems[3].type = zReader::ZRDR_NODE_STRING;
    primitiveItems[3].value.str = const_cast<char *>("COLOR");
    primitiveItems[4].type = zReader::ZRDR_NODE_ARRAY;
    primitiveItems[4].value.nodes = colorItems;
    primitiveItems[5].type = zReader::ZRDR_NODE_STRING;
    primitiveItems[5].value.str = const_cast<char *>("ENDP_REL");
    primitiveItems[6].type = zReader::ZRDR_NODE_ARRAY;
    primitiveItems[6].value.nodes = endRelItems;

    zReader::Node primitivesItems[3] = {};
    primitivesItems[0].value.i32 = 3;
    primitivesItems[1].type = zReader::ZRDR_NODE_STRING;
    primitivesItems[1].value.str = const_cast<char *>("LINE");
    primitivesItems[2].type = zReader::ZRDR_NODE_ARRAY;
    primitivesItems[2].value.nodes = primitiveItems;

    zReader::Node rootItems[3] = {};
    rootItems[0].value.i32 = 3;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("PRIMITIVES");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = primitivesItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    alignas(HudUiBackground) std::uint8_t backgroundStorage[0xa94c]{};
    auto *const background = reinterpret_cast<HudUiBackground *>(backgroundStorage);
    TestFieldAt<zReader::Node *>(background, 0xa940) = &root;
    TestFieldAt<std::int32_t>(background, 0xa944) = 10;
    TestFieldAt<std::int32_t>(background, 0xa948) = 20;
    TestFieldAt<void *>(background, 0x118) = reinterpret_cast<void *>(0x1234);

    HudUiPrimitiveBindTarget target{};
    target.base.ftable = &g_HudUiCommon_FTable;
    const int result = background->BindPrimitiveNodeToElement(nullptr, &target.base, "LINE");

    const bool linked =
        result == 0 && reinterpret_cast<HudUiContainer *>(background)->childHead == &target.base &&
        reinterpret_cast<HudUiContainer *>(background)->childTail == &target.base &&
        target.base.parent == reinterpret_cast<HudUiContainer *>(background);
    const bool primitive =
        target.base.x == 15 && target.base.y == 27 && target.endX == 17 && target.endY == 30 &&
        target.color565 == (zVid_PackColorRGB(0x20, 0x60, 0x40) & 0xffffu) &&
        target.base.bltSource == reinterpret_cast<void *>(0x1234) &&
        target.base.clipRect.left == 15 && target.base.clipRect.top == 27 &&
        target.base.clipRect.right == 15 && target.base.clipRect.bottom == 27 &&
        (target.base.flags & 0x02) != 0;

    int failure = 0;
    failure |= linked ? 0 : 1;
    failure |= primitive ? 0 : 2;
    return failure;
}

extern "C" int zhud_container_child_list_smoke(void) {
    HudUiContainer container{};
    HudUiElement first{};
    HudUiElement second{};
    HudUiElement third{};

    container.ConstructorDefault();
    const std::int32_t firstResult = container.AddChild(&first);
    const std::int32_t secondResult = container.AddChild(&second);
    const std::int32_t thirdResult = container.AddChild(&third);
    const bool linked = container.vptr == &g_HudUiContainer_FTable && container.enabled == 0 &&
                        container.childHead == &first && container.childTail == &third &&
                        first.next == &second && second.next == &third && third.next == nullptr &&
                        first.parent == &container && second.parent == &container &&
                        third.parent == &container;

    HudUiElement *previous = reinterpret_cast<HudUiElement *>(0x1234);
    const bool foundSecond =
        container.FindChildWithPrev(&second, &previous) == 1 && previous == &first;
    const bool missing =
        container.FindChildWithPrev(reinterpret_cast<HudUiElement *>(0x5678), &previous) == 0;

    first.flags = 0;
    second.flags = 0x10;
    third.flags = 0x20;
    container.SetChildFlags(0x02);
    const bool flagsSet = first.flags == 0x02 && second.flags == 0x12 && third.flags == 0x02;

    const bool removedMiddle = container.RemoveChild(&second) == 1 && first.next == &third &&
                               second.next == nullptr && second.parent == nullptr &&
                               container.childTail == &third;

    const bool removedHead = container.RemoveChild(&first) == 1 && container.childHead == &third &&
                             first.next == nullptr && first.parent == nullptr;

    const bool removedTail = container.RemoveChild(&third) == 1 && container.childHead == nullptr &&
                             container.childTail == nullptr && third.next == nullptr &&
                             third.parent == nullptr;

    HudUiCommon_FTable updateTable{};
    updateTable.slots[9] = MethodAddress(&TestContainerUpdateElement::Update);
    TestContainerUpdateElement updateA{};
    TestContainerUpdateElement updateB{};
    updateA.base.ftable = &updateTable;
    updateB.base.ftable = &updateTable;
    container.AddChild(&updateA.base);
    container.AddChild(&updateB.base);

    g_containerUpdateCount = 0;
    container.enabled = 0;
    container.UpdateAll(0.25f);
    const bool disabledSkipped = g_containerUpdateCount == 0;

    container.enabled = 1;
    container.UpdateAll(0.5f);
    const bool updated = g_containerUpdateCount == 2 && g_containerUpdateDelta[0] == 0.5f &&
                         g_containerUpdateDelta[1] == 0.5f;

    return firstResult == 1 && secondResult == 1 && thirdResult == 1 && linked && foundSecond &&
                   missing && flagsSet && removedMiddle && removedHead && removedTail &&
                   disabledSkipped && updated
               ? 0
               : 1;
}

extern "C" int zhud_container_destructor_core_smoke(void) {
    HudUiContainer container{};
    container.vptr = nullptr;
    container.DestructorCore();
    return container.vptr == &g_HudUiContainer_FTable ? 0 : 1;
}

extern "C" int zhud_widget_constructor_smoke(void) {
    HudUiWidget widget{};
    widget.imageStateWord = 0xabcd1234;
    for (HudUiRectDirty &dirtyRect : widget.dirtyRects) {
        dirtyRect.framesRemaining = 7;
    }

    widget.Constructor(0x55aa);

    bool dirtyFramesCleared = true;
    for (const HudUiRectDirty &dirtyRect : widget.dirtyRects) {
        dirtyFramesCleared = dirtyFramesCleared && dirtyRect.framesRemaining == 0;
    }

    zVidImagePartial image{};
    image.width = 11;
    image.height = 6;
    widget.x = 10;
    widget.y = 20;
    widget.image = &image;
    widget.alignFlags = 0;
    const bool unalignedCenter = widget.GetCenterX() == 10 && widget.GetCenterY() == 20;

    widget.alignFlags = 1;
    const bool alignedCenter = widget.GetCenterX() == 15 && widget.GetCenterY() == 23;

    image.width = -5;
    image.height = -3;
    const bool negativeCenter = widget.GetCenterX() == 8 && widget.GetCenterY() == 19;

    image.width = 11;
    image.height = 6;
    widget.RebuildBltRectFromImage();
    const bool rebuiltImageRect = widget.clipRect.left == 10 && widget.clipRect.top == 20 &&
                                  widget.clipRect.right == 21 && widget.clipRect.bottom == 26;

    widget.image = nullptr;
    widget.RebuildBltRectFromImage();
    const bool rebuiltNullRect = widget.clipRect.left == 10 && widget.clipRect.top == 20 &&
                                 widget.clipRect.right == 10 && widget.clipRect.bottom == 20;

    widget.alignFlags = 0x55aa;

    return widget.ftable == &g_HudUiWidget_FTable && widget.alignFlags == 0x55aa &&
                   widget.ownsImage == 0 && widget.bltClipRectOrNull == nullptr &&
                   widget.imageStateWord == 0xabcd0000 && widget.dirtyRectCount == 0 &&
                   dirtyFramesCleared && unalignedCenter && alignedCenter && negativeCenter &&
                   rebuiltImageRect && rebuiltNullRect
               ? 0
               : 1;
}

extern "C" int zhud_widget_default_ctor_thunk_smoke(void) {
    HudUiWidget widget{};
    return widget.CtorDefaultThunk() == &widget && widget.ftable == &g_HudUiWidget_FTable &&
                   widget.alignFlags == 0
               ? 0
               : 1;
}

extern "C" int zhud_zrd_widget_constructor_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;

    HudUiZrdWidget widget{};
    widget.boundsRect = {1, 2, 3, 4};
    widget.disabledSound = reinterpret_cast<zSndSample *>(0x1234);
    widget.disabledSoundScale = 2.5f;
    widget.Constructor();

    const bool vectorsEmpty =
        widget.labelPanels.begin == nullptr && widget.labelPanels.end == nullptr &&
        widget.labelPanels.capacityEnd == nullptr && widget.rolloverLabelPanels.begin == nullptr &&
        widget.rolloverLabelPanels.end == nullptr &&
        widget.rolloverLabelPanels.capacityEnd == nullptr &&
        widget.activateLabelPanels.begin == nullptr && widget.activateLabelPanels.end == nullptr &&
        widget.activateLabelPanels.capacityEnd == nullptr &&
        widget.disabledLabelPanels.begin == nullptr && widget.disabledLabelPanels.end == nullptr &&
        widget.disabledLabelPanels.capacityEnd == nullptr;

    const bool ok = widget.base.ftable ==
                        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiZrdWidget_FTable) &&
                    widget.base.alignFlags == 0 && widget.base.flags == 0x02 &&
                    widget.base.imageStateWord == 1 && widget.originX == 0 && widget.originY == 0 &&
                    widget.modeOrEnabled == 1 && widget.owner == nullptr &&
                    widget.boundsRect.left == 1 && widget.boundsRect.top == 2 &&
                    widget.boundsRect.right == 3 && widget.boundsRect.bottom == 4 &&
                    widget.defaultImage == nullptr && widget.disabledImage == nullptr &&
                    widget.rolloverImage == nullptr && widget.rolloverSound == nullptr &&
                    widget.rolloverPlayHandle == nullptr && widget.rolloverSoundScale == 1.0f &&
                    widget.activateImage == nullptr && widget.activateSound == nullptr &&
                    widget.activatePlayHandle == nullptr && widget.activateSoundScale == 1.0f &&
                    widget.disabledSound == reinterpret_cast<zSndSample *>(0x1234) &&
                    widget.disabledSoundScale == 2.5f && vectorsEmpty;

    g_HudUi_InvalidateMask = 0;
    return ok ? 0 : 1;
}

extern "C" int zhud_zrd_widget_helpers_smoke(void) {
    HudUiPanel *panels[] = {
        reinterpret_cast<HudUiPanel *>(0x1000),
        reinterpret_cast<HudUiPanel *>(0x2000),
        reinterpret_cast<HudUiPanel *>(0x3000),
        reinterpret_cast<HudUiPanel *>(0x4000),
    };
    HudUiPanelPtrVector vector{};
    vector.begin = panels;
    vector.end = panels + 4;
    vector.capacityEnd = panels + 4;

    HudUiPanel **const result = vector.EraseRange(panels + 1, panels + 3);
    const bool erased = result == panels + 1 && vector.end == panels + 2 &&
                        panels[0] == reinterpret_cast<HudUiPanel *>(0x1000) &&
                        panels[1] == reinterpret_cast<HudUiPanel *>(0x4000);

    HudUiPanelPtrVector insertVector{};
    HudUiPanel *insertA = reinterpret_cast<HudUiPanel *>(0x1110);
    HudUiPanel *insertB = reinterpret_cast<HudUiPanel *>(0x2220);
    insertVector.InsertN(nullptr, 1, &insertA);
    insertVector.InsertN(insertVector.begin, 2, &insertB);
    const bool inserted = insertVector.begin != nullptr &&
                          insertVector.end == insertVector.begin + 3 &&
                          insertVector.capacityEnd >= insertVector.end &&
                          insertVector.begin[0] == reinterpret_cast<HudUiPanel *>(0x2220) &&
                          insertVector.begin[1] == reinterpret_cast<HudUiPanel *>(0x2220) &&
                          insertVector.begin[2] == reinterpret_cast<HudUiPanel *>(0x1110);
    ::operator delete(insertVector.begin);

    HudUiWidget_FTable table{};
    table.slots[0] = MethodAddress(&TestZrdChildWidget::ScalarDeletingDestructor);
    TestZrdChildWidget child{&table, 0};
    const bool nullDelete = HudUiZrdWidget::DeleteChildIfPresent(nullptr) == nullptr;
    const bool childDelete =
        HudUiZrdWidget::DeleteChildIfPresent(&child) == nullptr && child.deleteFlags == 1;

    auto *labelVec = static_cast<HudUiPanel **>(::operator new(sizeof(HudUiPanel *)));
    auto *rolloverVec = static_cast<HudUiPanel **>(::operator new(sizeof(HudUiPanel *)));
    auto *activateVec = static_cast<HudUiPanel **>(::operator new(sizeof(HudUiPanel *)));
    auto *disabledVec = static_cast<HudUiPanel **>(::operator new(sizeof(HudUiPanel *)));

    TestZrdChildWidget labelChild{&table, 0};
    TestZrdChildWidget rolloverChild{&table, 0};
    TestZrdChildWidget activateChild{&table, 0};
    *labelVec = reinterpret_cast<HudUiPanel *>(&labelChild);
    *rolloverVec = reinterpret_cast<HudUiPanel *>(&rolloverChild);
    *activateVec = reinterpret_cast<HudUiPanel *>(&activateChild);
    *disabledVec = nullptr;

    HudUiZrdWidget widget{};
    widget.base.ftable = nullptr;
    widget.base.image = &zVid_Image::g_zImage_DefaultImage;
    widget.base.ownsImage = 0;
    widget.defaultImage = widget.base.image;
    widget.activateImage = widget.base.image;
    widget.labelPanels.begin = labelVec;
    widget.labelPanels.end = labelVec + 1;
    widget.labelPanels.capacityEnd = labelVec + 1;
    widget.rolloverLabelPanels.begin = rolloverVec;
    widget.rolloverLabelPanels.end = rolloverVec + 1;
    widget.rolloverLabelPanels.capacityEnd = rolloverVec + 1;
    widget.activateLabelPanels.begin = activateVec;
    widget.activateLabelPanels.end = activateVec + 1;
    widget.activateLabelPanels.capacityEnd = activateVec + 1;
    widget.disabledLabelPanels.begin = disabledVec;
    widget.disabledLabelPanels.end = disabledVec + 1;
    widget.disabledLabelPanels.capacityEnd = disabledVec + 1;

    widget.DestructorCore();
    const bool destructed =
        widget.base.ftable == reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable) &&
        labelChild.deleteFlags == 1 && rolloverChild.deleteFlags == 1 &&
        activateChild.deleteFlags == 1 && widget.labelPanels.begin == nullptr &&
        widget.labelPanels.end == nullptr && widget.labelPanels.capacityEnd == nullptr &&
        widget.rolloverLabelPanels.begin == nullptr && widget.rolloverLabelPanels.end == nullptr &&
        widget.rolloverLabelPanels.capacityEnd == nullptr &&
        widget.activateLabelPanels.begin == nullptr && widget.activateLabelPanels.end == nullptr &&
        widget.activateLabelPanels.capacityEnd == nullptr &&
        widget.disabledLabelPanels.begin == nullptr && widget.disabledLabelPanels.end == nullptr &&
        widget.disabledLabelPanels.capacityEnd == nullptr &&
        widget.defaultImage == widget.base.image && widget.activateImage == widget.base.image;

    HudUiZrdWidget scalarWidget{};
    HudUiZrdWidget *const scalarResult = scalarWidget.ScalarDeletingDestructor(0);
    const bool scalarDeleted =
        scalarResult == &scalarWidget &&
        scalarWidget.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);

    HudUiZrdWidget invalidateWidget{};
    HudUiElement labelA{};
    HudUiElement labelB{};
    labelA.ftable = &g_HudUiCommon_FTable;
    labelB.ftable = &g_HudUiCommon_FTable;
    HudUiPanel *labelPanels[] = {
        reinterpret_cast<HudUiPanel *>(&labelA),
        reinterpret_cast<HudUiPanel *>(&labelB),
    };
    invalidateWidget.labelPanels.begin = labelPanels;
    invalidateWidget.labelPanels.end = labelPanels + 2;
    g_HudUi_InvalidateMask = 0x80;
    invalidateWidget.Invalidate();
    const bool invalidated = (invalidateWidget.base.flags & 0x80) != 0 &&
                             (labelA.flags & 0x80) != 0 && (labelB.flags & 0x80) != 0;
    g_HudUi_InvalidateMask = 0;

    HudUiZrdWidget boundsWidget{};
    const bool disabledBounds = boundsWidget.GetBoundsRectOrNull() == nullptr;

    zVidImagePartial boundsImage{};
    boundsImage.width = 11;
    boundsImage.height = 13;
    boundsWidget.modeOrEnabled = 1;
    boundsWidget.base.x = 3;
    boundsWidget.base.y = 4;
    boundsWidget.base.image = &boundsImage;
    HudUiRect *imageBounds = boundsWidget.GetBoundsRectOrNull();
    const bool imageBoundsOk = imageBounds == &boundsWidget.boundsRect && imageBounds->left == 3 &&
                               imageBounds->top == 4 && imageBounds->right == 14 &&
                               imageBounds->bottom == 17;

    alignas(HudUiPanel) std::uint8_t boundsLabelStorageA[0x2ac]{};
    alignas(HudUiPanel) std::uint8_t boundsLabelStorageB[0x2ac]{};
    auto *const boundsLabelA = reinterpret_cast<HudUiPanel *>(boundsLabelStorageA);
    auto *const boundsLabelB = reinterpret_cast<HudUiPanel *>(boundsLabelStorageB);
    boundsLabelA->vtbl = &g_HudUiPanel_FTable;
    reinterpret_cast<HudUiElement *>(boundsLabelA)->x = 10;
    reinterpret_cast<HudUiElement *>(boundsLabelA)->y = 20;
    TestFieldAt<std::int32_t>(boundsLabelA, 0x25c) = 10;
    TestFieldAt<std::int32_t>(boundsLabelA, 0x260) = 5;
    TestFieldAt<std::int32_t>(boundsLabelA, 0x274) = 0;
    TestFieldAt<std::uint32_t>(boundsLabelA, 0x270) = 0;
    boundsLabelB->vtbl = &g_HudUiPanel_FTable;
    reinterpret_cast<HudUiElement *>(boundsLabelB)->x = 12;
    reinterpret_cast<HudUiElement *>(boundsLabelB)->y = 25;
    TestFieldAt<std::int32_t>(boundsLabelB, 0x25c) = 30;
    TestFieldAt<std::int32_t>(boundsLabelB, 0x260) = 7;
    TestFieldAt<std::int32_t>(boundsLabelB, 0x274) = 0;
    TestFieldAt<std::uint32_t>(boundsLabelB, 0x270) = 0;
    HudUiPanel *boundsLabels[] = {boundsLabelA, boundsLabelB};
    boundsWidget.base.image = nullptr;
    boundsWidget.boundsRect = {};
    boundsWidget.labelPanels.begin = boundsLabels;
    boundsWidget.labelPanels.end = boundsLabels + 2;
    HudUiRect *labelBounds = boundsWidget.GetBoundsRectOrNull();
    const bool labelBoundsOk = labelBounds == &boundsWidget.boundsRect && labelBounds->left == 10 &&
                               labelBounds->top == 20 && labelBounds->right == 42 &&
                               labelBounds->bottom == 32;

    HudUiZrdWidget stateWidget{};
    stateWidget.base.ftable = &g_HudUiWidget_FTable;
    zVidImagePartial defaultImage{};
    zVidImagePartial disabledImage{};
    HudUiElement stateLabel{};
    HudUiElement stateRollover{};
    HudUiElement stateActivate{};
    HudUiElement stateDisabled{};
    stateLabel.ftable = &g_HudUiCommon_FTable;
    stateRollover.ftable = &g_HudUiCommon_FTable;
    stateActivate.ftable = &g_HudUiCommon_FTable;
    stateDisabled.ftable = &g_HudUiCommon_FTable;
    HudUiPanel *stateLabels[] = {
        reinterpret_cast<HudUiPanel *>(&stateLabel),
    };
    HudUiPanel *stateRolloverLabels[] = {
        reinterpret_cast<HudUiPanel *>(&stateRollover),
    };
    HudUiPanel *stateActivateLabels[] = {
        reinterpret_cast<HudUiPanel *>(&stateActivate),
    };
    HudUiPanel *stateDisabledLabels[] = {
        reinterpret_cast<HudUiPanel *>(&stateDisabled),
    };
    stateWidget.labelPanels.begin = stateLabels;
    stateWidget.labelPanels.end = stateLabels + 1;
    stateWidget.rolloverLabelPanels.begin = stateRolloverLabels;
    stateWidget.rolloverLabelPanels.end = stateRolloverLabels + 1;
    stateWidget.activateLabelPanels.begin = stateActivateLabels;
    stateWidget.activateLabelPanels.end = stateActivateLabels + 1;
    stateWidget.disabledLabelPanels.begin = stateDisabledLabels;
    stateWidget.disabledLabelPanels.end = stateDisabledLabels + 1;
    stateWidget.defaultImage = &defaultImage;
    stateWidget.disabledImage = &disabledImage;
    stateWidget.modeOrEnabled = 1;
    stateWidget.RefreshState();
    const bool refreshEnabled =
        stateWidget.base.image == &defaultImage && (stateLabel.flags & 0x10) == 0 &&
        (stateRollover.flags & 0x10) != 0 && (stateActivate.flags & 0x10) != 0 &&
        (stateDisabled.flags & 0x10) != 0;

    stateWidget.modeOrEnabled = 0;
    stateWidget.RefreshState();
    const bool refreshDisabled = stateWidget.base.image == &disabledImage &&
                                 (stateLabel.flags & 0x10) != 0 &&
                                 (stateDisabled.flags & 0x10) == 0;

    stateWidget.rolloverPlayHandle = reinterpret_cast<zSndPlayHandle *>(0x1111);
    stateWidget.HidePreview();
    const bool previewHidden =
        stateWidget.base.image == &defaultImage && stateWidget.rolloverPlayHandle == nullptr &&
        (stateLabel.flags & 0x10) == 0 && (stateRollover.flags & 0x10) != 0 &&
        (stateActivate.flags & 0x10) != 0;

    HudUiZrdWidget previewWidget{};
    previewWidget.base.ftable = &g_HudUiWidget_FTable;
    zVidImagePartial previewDefault{};
    zVidImagePartial previewRollover{};
    HudUiElement previewLabel{};
    HudUiElement previewRolloverLabel{};
    HudUiElement previewActivateLabel{};
    previewLabel.ftable = &g_HudUiCommon_FTable;
    previewRolloverLabel.ftable = &g_HudUiCommon_FTable;
    previewActivateLabel.ftable = &g_HudUiCommon_FTable;
    HudUiPanel *previewLabels[] = {
        reinterpret_cast<HudUiPanel *>(&previewLabel),
    };
    HudUiPanel *previewRolloverLabels[] = {
        reinterpret_cast<HudUiPanel *>(&previewRolloverLabel),
    };
    HudUiPanel *previewActivateLabels[] = {
        reinterpret_cast<HudUiPanel *>(&previewActivateLabel),
    };
    previewWidget.base.image = &previewDefault;
    previewWidget.rolloverImage = &previewRollover;
    previewWidget.labelPanels.begin = previewLabels;
    previewWidget.labelPanels.end = previewLabels + 1;
    previewWidget.rolloverLabelPanels.begin = previewRolloverLabels;
    previewWidget.rolloverLabelPanels.end = previewRolloverLabels + 1;
    previewWidget.activateLabelPanels.begin = previewActivateLabels;
    previewWidget.activateLabelPanels.end = previewActivateLabels + 1;
    g_HudUi_InvalidateMask = 0x80;
    previewWidget.ShowPreview();
    const bool previewShown =
        previewWidget.defaultImage == &previewDefault &&
        previewWidget.base.image == &previewRollover && (previewWidget.base.flags & 0x80) != 0 &&
        (previewLabel.flags & 0x10) != 0 && (previewRolloverLabel.flags & 0x10) == 0 &&
        (previewActivateLabel.flags & 0x10) != 0;

    HudUiZrdWidget activateWidget{};
    activateWidget.base.ftable = &g_HudUiWidget_FTable;
    zVidImagePartial activateImage{};
    HudUiElement activateLabel{};
    HudUiElement activateRolloverLabel{};
    HudUiElement activateActiveLabel{};
    activateLabel.ftable = &g_HudUiCommon_FTable;
    activateRolloverLabel.ftable = &g_HudUiCommon_FTable;
    activateActiveLabel.ftable = &g_HudUiCommon_FTable;
    HudUiPanel *activateLabels[] = {
        reinterpret_cast<HudUiPanel *>(&activateLabel),
    };
    HudUiPanel *activateRolloverLabels[] = {
        reinterpret_cast<HudUiPanel *>(&activateRolloverLabel),
    };
    HudUiPanel *activateActiveLabels[] = {
        reinterpret_cast<HudUiPanel *>(&activateActiveLabel),
    };
    activateWidget.activateImage = &activateImage;
    activateWidget.labelPanels.begin = activateLabels;
    activateWidget.labelPanels.end = activateLabels + 1;
    activateWidget.rolloverLabelPanels.begin = activateRolloverLabels;
    activateWidget.rolloverLabelPanels.end = activateRolloverLabels + 1;
    activateWidget.activateLabelPanels.begin = activateActiveLabels;
    activateWidget.activateLabelPanels.end = activateActiveLabels + 1;
    activateWidget.OnActivate();
    const bool activated =
        activateWidget.base.image == &activateImage && (activateWidget.base.flags & 0x80) != 0 &&
        (activateLabel.flags & 0x10) != 0 && (activateRolloverLabel.flags & 0x10) != 0 &&
        (activateActiveLabel.flags & 0x10) == 0;
    g_HudUi_InvalidateMask = 0;

    return erased && inserted && nullDelete && childDelete && destructed && scalarDeleted &&
                   invalidated && disabledBounds && imageBoundsOk && labelBoundsOk &&
                   refreshEnabled && refreshDisabled && previewHidden && previewShown && activated
               ? 0
               : 1;
}

extern "C" int zhud_zrd_widget_load_from_zrd_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;
    g_zLoc_GetIdProc = nullptr;

    alignas(HudUiContainer) std::uint8_t ownerStorage[0xa94c]{};
    auto *owner = reinterpret_cast<HudUiContainer *>(ownerStorage);
    owner->ConstructorDefault();
    TestFieldAt<std::int32_t>(ownerStorage, 0xa944) = 10;
    TestFieldAt<std::int32_t>(ownerStorage, 0xa948) = 20;

    auto *const styles = reinterpret_cast<HudFontStyle *>(ownerStorage + 0x1cec);
    styles[2].validMarker = 1;
    styles[2].fontName = "Arial";
    styles[2].fontSize = 10;
    styles[2].textColor = 0x00112233;
    styles[2].bkColor = 0x00040506;
    styles[2].bkMode = 2;
    styles[2].shadowEnabled = 1;
    styles[2].fontWeight = FW_NORMAL;
    styles[2].alignMode = 1;

    zReader::Node positionItems[3] = {};
    positionItems[0].value.i32 = 3;
    positionItems[1].type = zReader::ZRDR_NODE_INT;
    positionItems[1].value.i32 = 5;
    positionItems[2].type = zReader::ZRDR_NODE_INT;
    positionItems[2].value.i32 = 7;

    zReader::Node labelItems[5] = {};
    labelItems[0].value.i32 = 5;
    labelItems[1].type = zReader::ZRDR_NODE_STRING;
    labelItems[1].value.str = const_cast<char *>("HELLO");
    labelItems[2].type = zReader::ZRDR_NODE_INT;
    labelItems[2].value.i32 = 2;
    labelItems[3].type = zReader::ZRDR_NODE_INT;
    labelItems[3].value.i32 = 3;
    labelItems[4].type = zReader::ZRDR_NODE_INT;
    labelItems[4].value.i32 = 2;

    zReader::Node rateItems[2] = {};
    rateItems[0].value.i32 = 2;
    rateItems[1].type = zReader::ZRDR_NODE_FLOAT;
    rateItems[1].value.f32 = 0.5f;

    zReader::Node colorItems[4] = {};
    colorItems[0].value.i32 = 4;
    colorItems[1].type = zReader::ZRDR_NODE_INT;
    colorItems[1].value.i32 = 0x11;
    colorItems[2].type = zReader::ZRDR_NODE_INT;
    colorItems[2].value.i32 = 0x22;
    colorItems[3].type = zReader::ZRDR_NODE_INT;
    colorItems[3].value.i32 = 0x33;

    zReader::Node flashItems[5] = {};
    flashItems[0].value.i32 = 5;
    flashItems[1].type = zReader::ZRDR_NODE_STRING;
    flashItems[1].value.str = const_cast<char *>("RATE");
    flashItems[2].type = zReader::ZRDR_NODE_ARRAY;
    flashItems[2].value.nodes = rateItems;
    flashItems[3].type = zReader::ZRDR_NODE_STRING;
    flashItems[3].value.str = const_cast<char *>("COLOR");
    flashItems[4].type = zReader::ZRDR_NODE_ARRAY;
    flashItems[4].value.nodes = colorItems;

    zReader::Node rootItems[7] = {};
    rootItems[0].value.i32 = 7;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("POSITION");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = positionItems;
    rootItems[3].type = zReader::ZRDR_NODE_STRING;
    rootItems[3].value.str = const_cast<char *>("LABEL");
    rootItems[4].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[4].value.nodes = labelItems;
    rootItems[5].type = zReader::ZRDR_NODE_STRING;
    rootItems[5].value.str = const_cast<char *>("FLASH");
    rootItems[6].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[6].value.nodes = flashItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    HudUiZrdWidget widget{};
    widget.Constructor();
    const std::int32_t result = widget.LoadFromZrd(&root, owner);

    auto *const labelPanel =
        widget.labelPanels.begin != nullptr
            ? reinterpret_cast<HudUiTransitionTextPanel *>(widget.labelPanels.begin[0])
            : nullptr;
    auto *const panel = reinterpret_cast<HudUiPanel *>(labelPanel);
    auto *const element = reinterpret_cast<HudUiElement *>(labelPanel);

    std::int32_t flashCountdownBits = 0;
    float expectedCountdown = 0.25f;
    std::memcpy(&flashCountdownBits, &expectedCountdown, sizeof(flashCountdownBits));
    std::int32_t actualLabelFlashCountdownBits = 0;
    if (labelPanel != nullptr) {
        std::memcpy(&actualLabelFlashCountdownBits, &labelPanel->flashCountdown,
                    sizeof(actualLabelFlashCountdownBits));
    }

    const bool loaded =
        result == 1 && widget.owner == owner && widget.originX == 15 && widget.originY == 27 &&
        reinterpret_cast<HudUiElement *>(&widget.base)->x == 15 &&
        reinterpret_cast<HudUiElement *>(&widget.base)->y == 27 &&
        owner->childHead == reinterpret_cast<HudUiElement *>(&widget) &&
        widget.labelPanels.end == widget.labelPanels.begin + 1 && labelPanel != nullptr &&
        std::strcmp(&TestFieldAt<char>(panel, 0x34), "HELLO") == 0 && element->x == 17 &&
        element->y == 30 && TestFieldAt<std::uint32_t>(panel, 0x144) == 1 &&
        TestFieldAt<std::uint32_t>(panel, 0x14c) == 0x00112233 &&
        TestFieldAt<std::uint32_t>(panel, 0x268) == 2 &&
        TestFieldAt<std::uint32_t>(panel, 0x26c) == 0x00040506 && labelPanel->flashEnabled == 1 &&
        labelPanel->flashMode == 2 && labelPanel->flashAltColor0 == 0x00332211 &&
        labelPanel->flashAltColor1 == 0x00332211 &&
        actualLabelFlashCountdownBits == flashCountdownBits;

    if (panel != nullptr) {
        DeletePanelAllocation(panel);
    }
    ::operator delete(widget.labelPanels.begin);

    g_HudUi_InvalidateMask = 0;
    return loaded ? 0 : 1;
}

extern "C" int zhud_check_toggle_widget_helpers_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;

    HudUiCheckToggleWidget widget{};
    widget.checked = 7;
    widget.disabledCheckedImage = reinterpret_cast<zVidImagePartial *>(0x1111);
    widget.disabledCheckedFallbackImage = reinterpret_cast<zVidImagePartial *>(0x2222);
    widget.uncheckedImage = reinterpret_cast<zVidImagePartial *>(0x3333);
    widget.checkedImage = reinterpret_cast<zVidImagePartial *>(0x4444);
    widget.checkedLabelPanel = reinterpret_cast<HudUiPanel *>(0x5555);
    widget.Constructor();

    const bool constructed =
        widget.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCheckToggleWidget_FTable) &&
        widget.base.modeOrEnabled == 1 && widget.checked == 0 &&
        widget.disabledCheckedImage == nullptr && widget.disabledCheckedFallbackImage == nullptr &&
        widget.uncheckedImage == nullptr && widget.checkedImage == nullptr &&
        widget.checkedLabelPanel == nullptr;

    zVidImagePartial uncheckedImage{};
    zVidImagePartial checkedImage{};
    HudUiElement checkedLabel{};
    checkedLabel.ftable = &g_HudUiCommon_FTable;
    widget.uncheckedImage = &uncheckedImage;
    widget.checkedImage = &checkedImage;
    widget.checkedLabelPanel = reinterpret_cast<HudUiPanel *>(&checkedLabel);
    widget.base.base.flags = 0;
    checkedLabel.flags = 0;

    const std::int32_t previousUnchecked = widget.SetChecked(1);
    const bool checkedState =
        previousUnchecked == 0 && widget.checked == 1 && widget.base.base.image == &checkedImage &&
        (checkedLabel.flags & 0x10) == 0 && (widget.base.base.flags & 0x80) != 0;

    widget.base.base.flags = 0;
    const std::int32_t previousChecked = widget.SetChecked(0);
    const bool uncheckedState =
        previousChecked == 1 && widget.checked == 0 && widget.base.base.image == &uncheckedImage &&
        (checkedLabel.flags & 0x10) != 0 && (widget.base.base.flags & 0x80) != 0;

    HudUiElement label{};
    HudUiElement rollover{};
    HudUiElement activate{};
    HudUiElement disabled{};
    label.ftable = &g_HudUiCommon_FTable;
    rollover.ftable = &g_HudUiCommon_FTable;
    activate.ftable = &g_HudUiCommon_FTable;
    disabled.ftable = &g_HudUiCommon_FTable;
    HudUiPanel *labels[] = {reinterpret_cast<HudUiPanel *>(&label)};
    HudUiPanel *rollovers[] = {reinterpret_cast<HudUiPanel *>(&rollover)};
    HudUiPanel *activates[] = {reinterpret_cast<HudUiPanel *>(&activate)};
    HudUiPanel *disabledLabels[] = {reinterpret_cast<HudUiPanel *>(&disabled)};
    zVidImagePartial disabledPrimary{};
    zVidImagePartial disabledFallback{};
    widget.base.labelPanels.begin = labels;
    widget.base.labelPanels.end = labels + 1;
    widget.base.rolloverLabelPanels.begin = rollovers;
    widget.base.rolloverLabelPanels.end = rollovers + 1;
    widget.base.activateLabelPanels.begin = activates;
    widget.base.activateLabelPanels.end = activates + 1;
    widget.base.disabledLabelPanels.begin = disabledLabels;
    widget.base.disabledLabelPanels.end = disabledLabels + 1;
    widget.disabledCheckedImage = &disabledPrimary;
    widget.disabledCheckedFallbackImage = &disabledFallback;
    widget.checked = 1;
    widget.base.modeOrEnabled = 1;
    widget.base.base.image = nullptr;
    widget.RefreshState();
    const bool refreshEnabled = widget.base.base.image == &checkedImage &&
                                (label.flags & 0x10) == 0 && (rollover.flags & 0x10) != 0 &&
                                (activate.flags & 0x10) != 0 && (disabled.flags & 0x10) != 0;

    widget.base.modeOrEnabled = 0;
    widget.disabledCheckedImage = nullptr;
    widget.RefreshState();
    const bool refreshDisabled = widget.base.base.image == &disabledFallback &&
                                 (label.flags & 0x10) != 0 && (disabled.flags & 0x10) == 0;

    HudUiCheckToggleWidget previewWidget{};
    previewWidget.base.base.ftable = &g_HudUiWidget_FTable;
    zVidImagePartial previewDefault{};
    zVidImagePartial previewRollover{};
    HudUiElement previewLabel{};
    HudUiElement previewRolloverLabel{};
    previewLabel.ftable = &g_HudUiCommon_FTable;
    previewRolloverLabel.ftable = &g_HudUiCommon_FTable;
    HudUiPanel *previewLabels[] = {
        reinterpret_cast<HudUiPanel *>(&previewLabel),
    };
    HudUiPanel *previewRolloverLabels[] = {
        reinterpret_cast<HudUiPanel *>(&previewRolloverLabel),
    };
    previewWidget.base.modeOrEnabled = 1;
    previewWidget.checked = 0;
    previewWidget.base.base.image = &previewDefault;
    previewWidget.base.rolloverImage = &previewRollover;
    previewWidget.base.labelPanels.begin = previewLabels;
    previewWidget.base.labelPanels.end = previewLabels + 1;
    previewWidget.base.rolloverLabelPanels.begin = previewRolloverLabels;
    previewWidget.base.rolloverLabelPanels.end = previewRolloverLabels + 1;
    previewWidget.ShowPreview();
    const bool previewShown = previewWidget.base.defaultImage == &previewDefault &&
                              previewWidget.base.base.image == &previewRollover &&
                              (previewLabel.flags & 0x10) != 0 &&
                              (previewRolloverLabel.flags & 0x10) == 0;

    HudUiCheckToggleWidget checkedPreviewWidget{};
    checkedPreviewWidget.base.modeOrEnabled = 1;
    checkedPreviewWidget.checked = 1;
    checkedPreviewWidget.base.base.image = &previewDefault;
    checkedPreviewWidget.base.rolloverImage = &previewRollover;
    checkedPreviewWidget.ShowPreview();
    const bool checkedPreviewSkipped = checkedPreviewWidget.base.defaultImage == nullptr &&
                                       checkedPreviewWidget.base.base.image == &previewDefault;

    HudUiCheckToggleWidget activateToggle{};
    activateToggle.base.base.ftable = &g_HudUiWidget_FTable;
    zVidImagePartial activateImage{};
    HudUiElement activateCheckedLabel{};
    activateCheckedLabel.ftable = &g_HudUiCommon_FTable;
    activateToggle.base.modeOrEnabled = 1;
    activateToggle.checked = 0;
    activateToggle.checkedImage = &checkedImage;
    activateToggle.checkedLabelPanel = reinterpret_cast<HudUiPanel *>(&activateCheckedLabel);
    activateToggle.base.activateImage = &activateImage;
    activateToggle.OnActivate();
    const bool activated = activateToggle.checked == 1 &&
                           activateToggle.base.base.image == &activateImage &&
                           (activateCheckedLabel.flags & 0x10) == 0;

    HudUiCheckToggleWidget disabledActivateToggle{};
    disabledActivateToggle.base.modeOrEnabled = 0;
    disabledActivateToggle.checked = 0;
    disabledActivateToggle.OnActivate();
    const bool disabledActivateSkipped = disabledActivateToggle.checked == 0;

    zSndPlayHandle rolloverHandle{};
    widget.base.modeOrEnabled = 1;
    widget.checked = 0;
    widget.base.defaultImage = &zVid_Image::g_zImage_DefaultImage;
    widget.base.rolloverPlayHandle = &rolloverHandle;
    widget.HidePreview();
    const bool previewHidden = widget.base.rolloverPlayHandle == nullptr &&
                               widget.base.base.image == &zVid_Image::g_zImage_DefaultImage &&
                               (label.flags & 0x10) == 0 && (rollover.flags & 0x10) != 0 &&
                               (activate.flags & 0x10) != 0;

    const bool bounds = widget.GetBoundsRectOrNull() == &widget.base.boundsRect;

    HudUiWidget_FTable table{};
    table.slots[0] = MethodAddress(&TestZrdChildWidget::ScalarDeletingDestructor);
    TestZrdChildWidget labelChild{&table, 0};
    auto *const ownedCheckedImage =
        static_cast<zVidImagePartial *>(::operator new(sizeof(zVidImagePartial)));
    widget.base.labelPanels = {};
    widget.base.rolloverLabelPanels = {};
    widget.base.activateLabelPanels = {};
    widget.base.disabledLabelPanels = {};
    widget.base.defaultImage = nullptr;
    widget.base.disabledImage = nullptr;
    widget.base.rolloverImage = nullptr;
    widget.base.activateImage = nullptr;
    widget.uncheckedImage = &zVid_Image::g_zImage_DefaultImage;
    widget.checkedImage = ownedCheckedImage;
    widget.checkedLabelPanel = reinterpret_cast<HudUiPanel *>(&labelChild);
    widget.DestructorCore();
    const bool destructed = widget.base.base.ftable == reinterpret_cast<const HudUiWidget_FTable *>(
                                                           &g_HudUiCommon_FTable) &&
                            widget.base.base.image == &zVid_Image::g_zImage_DefaultImage &&
                            widget.checkedImage == nullptr && widget.checkedLabelPanel == nullptr &&
                            labelChild.deleteFlags == 1;

    HudUiCheckToggleWidget scalarWidget{};
    HudUiCheckToggleWidget *const scalarResult = scalarWidget.ScalarDeletingDestructor(0);
    const bool scalarDeleted =
        scalarResult == &scalarWidget &&
        scalarWidget.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);

    g_HudUi_InvalidateMask = 0;
    return constructed && checkedState && uncheckedState && refreshEnabled && refreshDisabled &&
                   previewShown && checkedPreviewSkipped && activated && disabledActivateSkipped &&
                   previewHidden && bounds && destructed && scalarDeleted
               ? 0
               : 1;
}

extern "C" int zhud_check_toggle_widget_load_from_zrd_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;
    g_zLoc_GetIdProc = nullptr;

    alignas(HudUiContainer) std::uint8_t ownerStorage[0xa94c]{};
    auto *owner = reinterpret_cast<HudUiContainer *>(ownerStorage);
    owner->ConstructorDefault();
    TestFieldAt<std::int32_t>(ownerStorage, 0xa944) = 30;
    TestFieldAt<std::int32_t>(ownerStorage, 0xa948) = 40;

    auto *const styles = reinterpret_cast<HudFontStyle *>(ownerStorage + 0x1cec);
    styles[1].validMarker = 1;
    styles[1].fontName = "Arial";
    styles[1].fontSize = 10;
    styles[1].textColor = 0x00010203;
    styles[1].bkColor = 0x00040506;
    styles[1].bkMode = 2;
    styles[1].shadowEnabled = 1;
    styles[1].fontWeight = FW_NORMAL;
    styles[1].alignMode = 2;

    zReader::Node checkedTextItems[5] = {};
    checkedTextItems[0].value.i32 = 5;
    checkedTextItems[1].type = zReader::ZRDR_NODE_STRING;
    checkedTextItems[1].value.str = const_cast<char *>("ON");
    checkedTextItems[2].type = zReader::ZRDR_NODE_INT;
    checkedTextItems[2].value.i32 = 4;
    checkedTextItems[3].type = zReader::ZRDR_NODE_INT;
    checkedTextItems[3].value.i32 = 5;
    checkedTextItems[4].type = zReader::ZRDR_NODE_INT;
    checkedTextItems[4].value.i32 = 1;

    zReader::Node checkedItems[3] = {};
    checkedItems[0].value.i32 = 3;
    checkedItems[1].type = zReader::ZRDR_NODE_STRING;
    checkedItems[1].value.str = const_cast<char *>("TEXT");
    checkedItems[2].type = zReader::ZRDR_NODE_ARRAY;
    checkedItems[2].value.nodes = checkedTextItems;

    zReader::Node disabledLabelItems[5] = {};
    disabledLabelItems[0].value.i32 = 5;
    disabledLabelItems[1].type = zReader::ZRDR_NODE_STRING;
    disabledLabelItems[1].value.str = const_cast<char *>("DISABLED");
    disabledLabelItems[2].type = zReader::ZRDR_NODE_INT;
    disabledLabelItems[2].value.i32 = 7;
    disabledLabelItems[3].type = zReader::ZRDR_NODE_INT;
    disabledLabelItems[3].value.i32 = 8;
    disabledLabelItems[4].type = zReader::ZRDR_NODE_INT;
    disabledLabelItems[4].value.i32 = 1;

    zReader::Node disabledItems[3] = {};
    disabledItems[0].value.i32 = 3;
    disabledItems[1].type = zReader::ZRDR_NODE_STRING;
    disabledItems[1].value.str = const_cast<char *>("LABEL");
    disabledItems[2].type = zReader::ZRDR_NODE_ARRAY;
    disabledItems[2].value.nodes = disabledLabelItems;

    zReader::Node rootItems[5] = {};
    rootItems[0].value.i32 = 5;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("CHECKED");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = checkedItems;
    rootItems[3].type = zReader::ZRDR_NODE_STRING;
    rootItems[3].value.str = const_cast<char *>("DISABLE_UNSEL");
    rootItems[4].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[4].value.nodes = disabledItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    HudUiCheckToggleWidget widget{};
    widget.Constructor();
    const std::int32_t result = widget.LoadFromZrd(&root, owner);

    auto *const checkedPanel = widget.checkedLabelPanel;
    auto *const checkedElement = reinterpret_cast<HudUiElement *>(checkedPanel);
    auto *const disabledPanel = widget.base.disabledLabelPanels.begin != nullptr
                                    ? widget.base.disabledLabelPanels.begin[0]
                                    : nullptr;
    auto *const disabledElement = reinterpret_cast<HudUiElement *>(disabledPanel);

    const bool loaded =
        result == 1 && widget.base.originX == 30 && widget.base.originY == 40 &&
        widget.uncheckedImage == nullptr && checkedPanel != nullptr &&
        std::strcmp(&TestFieldAt<char>(checkedPanel, 0x34), "ON") == 0 && checkedElement->x == 34 &&
        checkedElement->y == 45 && (checkedElement->flags & 0x10) != 0 &&
        TestFieldAt<std::uint32_t>(checkedPanel, 0x14c) == 0x00010203 &&
        widget.base.disabledLabelPanels.end == widget.base.disabledLabelPanels.begin + 1 &&
        disabledPanel != nullptr &&
        std::strcmp(&TestFieldAt<char>(disabledPanel, 0x34), "DISABLED") == 0 &&
        disabledElement->x == 37 && disabledElement->y == 48 &&
        (disabledElement->flags & 0x10) == 0 &&
        TestFieldAt<std::uint32_t>(disabledPanel, 0x144) == 2 &&
        TestFieldAt<std::uint32_t>(disabledPanel, 0x268) == 2;

    if (checkedPanel != nullptr) {
        DeletePanelAllocation(checkedPanel);
    }
    if (disabledPanel != nullptr) {
        DeletePanelAllocation(disabledPanel);
    }
    ::operator delete(widget.base.disabledLabelPanels.begin);

    g_HudUi_InvalidateMask = 0;
    return loaded ? 0 : 1;
}

extern "C" int zhud_cycle_selector_widget_constructor_smoke(void) {
    HudUiCycleSelectorWidget widget{};
    widget.selectedIndex = 1;
    widget.itemCount = 2;
    widget.firstIndex = 3;
    widget.visibleCount = 4;
    widget.fontStyleRef = reinterpret_cast<void *>(0x1111);
    widget.textOffsetX = 5;
    widget.textOffsetY = 6;
    widget.entriesA[0] = reinterpret_cast<HudUiWidget *>(0x2222);
    widget.entriesB[19] = reinterpret_cast<HudUiWidget *>(0x3333);

    widget.Constructor();

    bool entriesClear = true;
    for (std::int32_t i = 0; i < 20; ++i) {
        entriesClear =
            entriesClear && widget.entriesA[i] == nullptr && widget.entriesB[i] == nullptr;
    }

    const bool constructed =
        widget.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCycleSelectorWidget_FTable) &&
        widget.selectedIndex == 0 && widget.itemCount == 0 && widget.firstIndex == 0 &&
        widget.visibleCount == 20 && widget.fontStyleRef == nullptr && widget.textOffsetX == 0 &&
        widget.textOffsetY == 0 && entriesClear;
    widget.itemCount = 10;
    widget.firstIndex = 2;
    widget.visibleCount = 6;
    widget.SetIndexClamped(1);
    const bool clampLow = widget.selectedIndex == 2;
    widget.SetIndexClamped(10);
    const bool clampItemCount = widget.selectedIndex == 9;
    widget.SetIndexClamped(6);
    const bool clampVisible = widget.selectedIndex == 5;
    widget.SetIndexClamped(4);
    const bool clampInside = widget.selectedIndex == 4;

    widget.selectedIndex = 1;
    widget.SetVisibleRange(3, 8);
    const bool rangeLow =
        widget.firstIndex == 3 && widget.visibleCount == 8 && widget.selectedIndex == 3;
    widget.selectedIndex = 9;
    widget.SetVisibleRange(-1, 20);
    const bool rangeInvalid =
        widget.firstIndex == 3 && widget.visibleCount == 8 && widget.selectedIndex == 9;
    widget.firstIndex = 0;
    widget.itemCount = 3;
    widget.visibleCount = 2;
    widget.selectedIndex = 1;
    widget.AdvanceSelectionAndActivate();
    const bool advanceWrap = widget.selectedIndex == 0;
    widget.firstIndex = 1;
    widget.itemCount = 5;
    widget.visibleCount = 4;
    widget.selectedIndex = 0;
    widget.AdvanceSelectionAndActivate();
    const bool advanceInside = widget.selectedIndex == 1;
    HudUiElement updateEntryA0{};
    HudUiElement updateEntryA1{};
    HudUiElement updateEntryB0{};
    HudUiElement updateEntryB1{};
    widget.itemCount = 2;
    widget.selectedIndex = 1;
    widget.entriesA[0] = reinterpret_cast<HudUiWidget *>(&updateEntryA0);
    widget.entriesA[1] = reinterpret_cast<HudUiWidget *>(&updateEntryA1);
    widget.entriesB[0] = reinterpret_cast<HudUiWidget *>(&updateEntryB0);
    widget.entriesB[1] = reinterpret_cast<HudUiWidget *>(&updateEntryB1);
    widget.base.base.flags = 0x03;
    widget.base.base.timer = 0.5f;
    g_HudUi_InvalidateMask = 0x80;
    widget.Update(0.25f);
    g_HudUi_InvalidateMask = 0;
    const bool updated = (widget.base.base.flags & 0x80) != 0 && widget.base.base.timer == 0.25f &&
                         (updateEntryA0.flags & 0x10) != 0 && (updateEntryB0.flags & 0x10) != 0 &&
                         (updateEntryA1.flags & 0x10) == 0 && (updateEntryB1.flags & 0x10) == 0;
    for (std::int32_t i = 0; i < 20; ++i) {
        widget.entriesA[i] = nullptr;
        widget.entriesB[i] = nullptr;
    }

    alignas(HudUiContainer) std::uint8_t ownerStorage[0x1cec + (sizeof(HudFontStyle) * 2)] = {};
    auto *const owner = reinterpret_cast<HudUiContainer *>(ownerStorage);
    owner->ConstructorDefault();
    auto *const fontStyles = reinterpret_cast<HudFontStyle *>(ownerStorage + 0x1cec);
    fontStyles[1].validMarker = 1;
    fontStyles[1].fontName = "Arial";
    fontStyles[1].fontSize = 12;
    fontStyles[1].fontWeight = FW_BOLD;
    fontStyles[1].textColor = 0x00112233;
    fontStyles[1].shadowEnabled = 1;
    fontStyles[1].alignMode = 2;
    fontStyles[1].bkMode = 3;
    fontStyles[1].bkColor = 0x00445566;
    widget.base.owner = owner;
    widget.itemCount = 0;
    widget.visibleCount = 1;
    widget.textOffsetX = 11;
    widget.textOffsetY = -2;
    widget.AddTextEntry(3, "Bravo", 7, 9);
    widget.ApplyFontStyleForEntry(3, 1);
    auto *const textEntry = reinterpret_cast<HudUiTransitionTextPanel *>(widget.entriesA[3]);
    const bool textEntryAdded =
        widget.itemCount == 4 && widget.visibleCount == 4 && textEntry != nullptr &&
        TestFieldAt<const void *>(textEntry, 0x00) == &g_HudUiTransitionTextPanel_FTable &&
        std::strcmp(&TestFieldAt<char>(textEntry, 0x34), "Bravo") == 0 &&
        TestFieldAt<std::int32_t>(textEntry, 0x14) == 18 &&
        TestFieldAt<std::int32_t>(textEntry, 0x18) == 7 &&
        (TestFieldAt<std::uint32_t>(textEntry, 0x0c) & 0x10u) != 0 &&
        TestFieldAt<std::int32_t>(textEntry, 0x2a4) == 0 &&
        TestFieldAt<float>(textEntry, 0x2a8) == 0.349999994f &&
        TestFieldAt<std::int32_t>(textEntry, 0x2ac) == 0 &&
        TestFieldAt<std::int32_t>(textEntry, 0x2b4) == 0 &&
        TestFieldAt<std::int32_t>(textEntry, 0x2b8) == 0 &&
        TestFieldAt<std::int32_t>(textEntry, 0x2bc) == 1 &&
        TestFieldAt<std::uint32_t>(textEntry, 0x14c) == 0x00112233 &&
        TestFieldAt<std::uint32_t>(textEntry, 0x150) == 0x00112233 &&
        TestFieldAt<std::uint32_t>(textEntry, 0x270) == 1 &&
        TestFieldAt<std::uint32_t>(textEntry, 0x264) == 1 &&
        TestFieldAt<std::int32_t>(textEntry, 0x29c) == 1 &&
        TestFieldAt<std::int32_t>(textEntry, 0x2a0) == 1 &&
        TestFieldAt<std::uint32_t>(textEntry, 0x144) == 2 &&
        TestFieldAt<std::uint32_t>(textEntry, 0x268) == 3 &&
        TestFieldAt<std::uint32_t>(textEntry, 0x26c) == 0x00445566 &&
        owner->childHead == reinterpret_cast<HudUiElement *>(textEntry) &&
        owner->childTail == reinterpret_cast<HudUiElement *>(textEntry) &&
        reinterpret_cast<HudUiElement *>(textEntry)->parent == owner;
    if (textEntry != nullptr) {
        reinterpret_cast<HudUiPanel *>(textEntry)->Destructor();
        ::operator delete(textEntry);
        widget.entriesA[3] = nullptr;
    }
    owner->childHead = nullptr;
    owner->childTail = nullptr;

    HudUiTransitionTextPanel flashPanel{};
    flashPanel.flashMode = 0;
    flashPanel.SetFlashColorAndRate(0x00112233, 6.0f);
    std::uint32_t rawFlashCountdown = 0;
    const float flashHalfRate = 3.0f;
    std::memcpy(&rawFlashCountdown, &flashHalfRate, sizeof(rawFlashCountdown));
    std::uint32_t actualFlashCountdown = 0;
    std::memcpy(&actualFlashCountdown, &flashPanel.flashCountdown, sizeof(actualFlashCountdown));
    const bool flashSet =
        flashPanel.flashEnabled == 1 && flashPanel.flashMode == 2 &&
        flashPanel.flashResetValue == 3.0f && actualFlashCountdown == rawFlashCountdown &&
        flashPanel.flashDirectionSign == 1 && flashPanel.flashAltColor0 == 0x00112233 &&
        flashPanel.flashAltColor1 == 0x00112233;
    flashPanel.flashAltColor0 = 1;
    flashPanel.flashAltColor1 = 2;
    flashPanel.flashResetValue = 0.75f;
    flashPanel.SetFlashColorAndRate(0x00445566, 8.0f);
    const bool flashAlreadySet = flashPanel.flashMode == 2 && flashPanel.flashAltColor0 == 1 &&
                                 flashPanel.flashAltColor1 == 2 &&
                                 flashPanel.flashResetValue == 0.75f;

    flashPanel.Constructor();
    TestFieldAt<std::uint32_t>(&flashPanel, 0x14c) = 0x00010203;
    TestFieldAt<std::uint32_t>(&flashPanel, 0x150) = 0x00040506;
    flashPanel.flashCountdown = 0.25f;
    flashPanel.flashResetValue = 0.5f;
    flashPanel.flashAltColor0 = 0x00070809;
    flashPanel.flashAltColor1 = 0x000a0b0c;
    flashPanel.flashEnabled = 1;
    flashPanel.flashMode = 2;
    flashPanel.flashDirectionSign = 1;
    TestFieldAt<std::uint32_t>(&flashPanel, 0x270) = 0;
    flashPanel.TickFlash(0.5f);
    const bool flashTickSwap =
        flashPanel.flashCountdown == 0.5f && flashPanel.flashDirectionSign == -1 &&
        TestFieldAt<std::uint32_t>(&flashPanel, 0x270) == 0 &&
        TestFieldAt<std::uint32_t>(&flashPanel, 0x14c) == 0x00070809 &&
        TestFieldAt<std::uint32_t>(&flashPanel, 0x150) == 0x000a0b0c &&
        flashPanel.flashAltColor0 == 0x00010203 && flashPanel.flashAltColor1 == 0x00040506;
    reinterpret_cast<HudUiPanel *>(&flashPanel)->Destructor();

    widget.itemCount = 1;
    widget.visibleCount = 1;
    widget.AddBitmapEntry(2, nullptr, 21, 22);
    HudUiWidget *const bitmapEntry = widget.entriesB[2];
    const bool bitmapEntryAdded =
        widget.itemCount == 3 && widget.visibleCount == 3 && bitmapEntry != nullptr &&
        bitmapEntry->ftable == &g_HudUiWidget_FTable && bitmapEntry->image == nullptr &&
        bitmapEntry->ownsImage == 0 && bitmapEntry->x == 21 && bitmapEntry->y == 22 &&
        (bitmapEntry->flags & 0x10u) != 0 &&
        owner->childHead == reinterpret_cast<HudUiElement *>(bitmapEntry) &&
        owner->childTail == reinterpret_cast<HudUiElement *>(bitmapEntry) &&
        reinterpret_cast<HudUiElement *>(bitmapEntry)->parent == owner;
    if (bitmapEntry != nullptr) {
        bitmapEntry->DestructorCore();
        ::operator delete(bitmapEntry);
        widget.entriesB[2] = nullptr;
    }

    HudUiWidget_FTable table{};
    table.slots[0] = MethodAddress(&TestZrdChildWidget::ScalarDeletingDestructor);
    TestZrdChildWidget entryA{&table, 0};
    TestZrdChildWidget entryB{&table, 0};
    widget.entriesA[3] = reinterpret_cast<HudUiWidget *>(&entryA);
    widget.entriesB[7] = reinterpret_cast<HudUiWidget *>(&entryB);
    widget.base.base.image = &zVid_Image::g_zImage_DefaultImage;
    widget.base.base.ownsImage = 0;
    widget.DestructorCore();
    const bool destructed = entryA.deleteFlags == 1 && entryB.deleteFlags == 1 &&
                            widget.entriesA[3] == nullptr && widget.entriesB[7] == nullptr &&
                            widget.base.base.ftable ==
                                reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);

    HudUiCycleSelectorWidget scalarWidget{};
    HudUiCycleSelectorWidget *const scalarResult = scalarWidget.ScalarDeletingDestructor(0);
    const bool scalarDeleted =
        scalarResult == &scalarWidget &&
        scalarWidget.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);

    return constructed && clampLow && clampItemCount && clampVisible && clampInside && rangeLow &&
                   rangeInvalid && advanceWrap && advanceInside && updated && textEntryAdded &&
                   flashSet && flashAlreadySet && flashTickSwap && bitmapEntryAdded && destructed &&
                   scalarDeleted
               ? 0
               : 1;
}

extern "C" int zhud_cycle_selector_widget_load_from_zrd_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;
    g_zLoc_GetIdProc = nullptr;

    alignas(HudUiContainer) std::uint8_t ownerStorage[0xa94c] = {};
    auto *owner = reinterpret_cast<HudUiContainer *>(ownerStorage);
    owner->ConstructorDefault();
    TestFieldAt<std::int32_t>(ownerStorage, 0xa944) = 100;
    TestFieldAt<std::int32_t>(ownerStorage, 0xa948) = 200;

    auto *const styles = reinterpret_cast<HudFontStyle *>(ownerStorage + 0x1cec);
    styles[2].validMarker = 1;
    styles[2].fontName = "Arial";
    styles[2].fontSize = 9;
    styles[2].fontWeight = FW_BOLD;
    styles[2].textColor = 0x00010203;
    styles[2].shadowEnabled = 1;
    styles[2].alignMode = 2;
    styles[2].bkMode = 1;
    styles[2].bkColor = 0x00040506;

    zReader::Node textOffsetItems[3] = {};
    textOffsetItems[0].value.i32 = 3;
    textOffsetItems[1].type = zReader::ZRDR_NODE_INT;
    textOffsetItems[1].value.i32 = 11;
    textOffsetItems[2].type = zReader::ZRDR_NODE_INT;
    textOffsetItems[2].value.i32 = -3;

    zReader::Node textItems[5] = {};
    textItems[0].value.i32 = 5;
    textItems[1].type = zReader::ZRDR_NODE_STRING;
    textItems[1].value.str = const_cast<char *>("FIRST");
    textItems[2].type = zReader::ZRDR_NODE_INT;
    textItems[2].value.i32 = 4;
    textItems[3].type = zReader::ZRDR_NODE_INT;
    textItems[3].value.i32 = 5;
    textItems[4].type = zReader::ZRDR_NODE_INT;
    textItems[4].value.i32 = 2;

    zReader::Node bitmapItems[4] = {};
    bitmapItems[0].value.i32 = 4;
    bitmapItems[1].type = zReader::ZRDR_NODE_INT;
    bitmapItems[1].value.i32 = 0;
    bitmapItems[2].type = zReader::ZRDR_NODE_INT;
    bitmapItems[2].value.i32 = 8;
    bitmapItems[3].type = zReader::ZRDR_NODE_INT;
    bitmapItems[3].value.i32 = 9;

    zReader::Node entryItems[5] = {};
    entryItems[0].value.i32 = 5;
    entryItems[1].type = zReader::ZRDR_NODE_STRING;
    entryItems[1].value.str = const_cast<char *>("TEXT");
    entryItems[2].type = zReader::ZRDR_NODE_ARRAY;
    entryItems[2].value.nodes = textItems;
    entryItems[3].type = zReader::ZRDR_NODE_STRING;
    entryItems[3].value.str = const_cast<char *>("BITMAP");
    entryItems[4].type = zReader::ZRDR_NODE_ARRAY;
    entryItems[4].value.nodes = bitmapItems;

    zReader::Node secondEntryItems[1] = {};
    secondEntryItems[0].value.i32 = 1;

    zReader::Node cycleItems[3] = {};
    cycleItems[0].value.i32 = 3;
    cycleItems[1].type = zReader::ZRDR_NODE_ARRAY;
    cycleItems[1].value.nodes = entryItems;
    cycleItems[2].type = zReader::ZRDR_NODE_ARRAY;
    cycleItems[2].value.nodes = secondEntryItems;

    zReader::Node rootItems[7] = {};
    rootItems[0].value.i32 = 7;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("FONT");
    rootItems[2].type = zReader::ZRDR_NODE_INT;
    rootItems[2].value.i32 = 7;
    rootItems[3].type = zReader::ZRDR_NODE_STRING;
    rootItems[3].value.str = const_cast<char *>("TEXTOFFSET");
    rootItems[4].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[4].value.nodes = textOffsetItems;
    rootItems[5].type = zReader::ZRDR_NODE_STRING;
    rootItems[5].value.str = const_cast<char *>("CYCLE");
    rootItems[6].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[6].value.nodes = cycleItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    HudUiCycleSelectorWidget widget{};
    widget.Constructor();
    widget.visibleCount = 1;
    const std::int32_t result = widget.LoadFromZrd(&root, owner);

    auto *const textEntry = reinterpret_cast<HudUiTransitionTextPanel *>(widget.entriesA[0]);
    auto *const bitmapEntry = widget.entriesB[0];
    const bool loaded =
        result == 1 && widget.base.owner == owner &&
        widget.fontStyleRef == reinterpret_cast<void *>(7) && widget.textOffsetX == 11 &&
        widget.textOffsetY == -3 && widget.itemCount == 2 && widget.visibleCount == 2 &&
        textEntry != nullptr && std::strcmp(&TestFieldAt<char>(textEntry, 0x34), "FIRST") == 0 &&
        TestFieldAt<std::int32_t>(textEntry, 0x14) == 115 &&
        TestFieldAt<std::int32_t>(textEntry, 0x18) == 202 &&
        TestFieldAt<std::uint32_t>(textEntry, 0x14c) == 0x00010203 && bitmapEntry != nullptr &&
        bitmapEntry->x == 108 && bitmapEntry->y == 209 && (bitmapEntry->flags & 0x10u) != 0;

    if (textEntry != nullptr) {
        reinterpret_cast<HudUiPanel *>(textEntry)->Destructor();
        ::operator delete(textEntry);
    }
    if (bitmapEntry != nullptr) {
        bitmapEntry->DestructorCore();
        ::operator delete(bitmapEntry);
    }

    g_HudUi_InvalidateMask = 0;
    return loaded ? 0 : 1;
}

extern "C" int zhud_fill_bitmap_core_smoke(void) {
    HudUiFillBitmap bitmap{};
    bitmap.fillOffsetX = 0x11111111;
    bitmap.fillOffsetY = 0x22222222;
    bitmap.previewOffsetX = 0x33333333;
    bitmap.previewOffsetY = 0x44444444;
    bitmap.Constructor();

    const bool constructed =
        bitmap.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiFillBitmap_FTable) &&
        bitmap.normalizedValue == 0.0f && bitmap.fillImage == nullptr &&
        bitmap.previewImage == nullptr && bitmap.fillRect.left == 0 && bitmap.fillRect.top == 0 &&
        bitmap.fillRect.right == 0 && bitmap.fillRect.bottom == 0 && bitmap.previewRect.left == 0 &&
        bitmap.previewRect.top == 0 && bitmap.previewRect.right == 0 &&
        bitmap.previewRect.bottom == 0 && bitmap.fillOffsetX == 0x11111111 &&
        bitmap.fillOffsetY == 0x22222222 && bitmap.previewOffsetX == 0x33333333 &&
        bitmap.previewOffsetY == 0x44444444;

    zVidImagePartial fillImage{};
    fillImage.width = 101;
    fillImage.height = 7;
    zVidImagePartial previewImage{};
    previewImage.width = 120;
    previewImage.height = 9;

    g_HudUi_InvalidateMask = 0x80;
    bitmap.base.base.flags = 0;
    bitmap.fillImage = &fillImage;
    bitmap.previewImage = &previewImage;
    bitmap.SetNormalizedValueAndRebuild(0.25f);
    g_HudUi_InvalidateMask = 0;

    const bool normalized = bitmap.normalizedValue == 0.25f && bitmap.fillRect.left == 0 &&
                            bitmap.fillRect.top == 0 && bitmap.fillRect.right == 25 &&
                            bitmap.fillRect.bottom == 7 && bitmap.fillOffsetX == 0 &&
                            bitmap.fillOffsetY == 0 && bitmap.previewRect.left == 25 &&
                            bitmap.previewRect.top == 0 && bitmap.previewRect.right == 120 &&
                            bitmap.previewRect.bottom == 7 && bitmap.previewOffsetX == 25 &&
                            bitmap.previewOffsetY == 0 && (bitmap.base.base.flags & 0x80u) != 0;

    bitmap.base.base.flags = 0;
    g_HudUi_InvalidateMask = 0x80;
    bitmap.SetNormalizedValue(0.5f);
    g_HudUi_InvalidateMask = 0;
    const bool setNormalized =
        bitmap.normalizedValue == 0.5f && (bitmap.base.base.flags & 0x80u) != 0;

    zVidImagePartial baseImage{};
    bitmap.base.base.image = &baseImage;
    bitmap.base.base.x = 10;
    bitmap.base.base.y = 20;
    bitmap.base.base.dirtyRectCount = 0;
    g_testBlitCount = 0;
    g_zVideo_pfnBltSourceToPrimary = TestBltSourceToPrimary;
    bitmap.Draw();
    g_zVideo_pfnBltSourceToPrimary = nullptr;
    const bool drawn = g_testBlitCount == 3 && g_testBlitImages[0] == &baseImage &&
                       g_testBlitX[0] == 10 && g_testBlitY[0] == 20 && g_testBlitHasRect[0] == 0 &&
                       g_testBlitImages[1] == &fillImage && g_testBlitX[1] == 10 &&
                       g_testBlitY[1] == 20 && g_testBlitHasRect[1] == 1 &&
                       g_testBlitRects[1].left == 0 && g_testBlitRects[1].right == 25 &&
                       g_testBlitImages[2] == &previewImage && g_testBlitX[2] == 35 &&
                       g_testBlitY[2] == 20 && g_testBlitHasRect[2] == 1 &&
                       g_testBlitRects[2].left == 25 && g_testBlitRects[2].right == 120;

    bitmap.base.base.dirtyRectCount = 1;
    bitmap.base.base.dirtyRects[0].framesRemaining = 1;
    bitmap.base.base.dirtyRects[0].drawX = 3;
    bitmap.base.base.dirtyRects[0].drawY = 4;
    bitmap.base.base.dirtyRects[0].srcLeft = 1;
    bitmap.base.base.dirtyRects[0].srcTop = 2;
    bitmap.base.base.dirtyRects[0].srcRight = 5;
    bitmap.base.base.dirtyRects[0].srcBottom = 6;
    g_testBlitCount = 0;
    g_zVideo_pfnBltSourceToPrimary = TestBltSourceToPrimary;
    bitmap.base.base.Draw();
    g_zVideo_pfnBltSourceToPrimary = nullptr;
    const bool dirtyDrawn = g_testBlitCount == 1 && bitmap.base.base.dirtyRectCount == 0 &&
                            bitmap.base.base.dirtyRects[0].framesRemaining == 0 &&
                            g_testBlitX[0] == 3 && g_testBlitY[0] == 4 &&
                            g_testBlitHasRect[0] == 1 && g_testBlitRects[0].left == 1 &&
                            g_testBlitRects[0].bottom == 6;

    bitmap.normalizedValue = 0.75f;
    bitmap.fillImage = nullptr;
    bitmap.SetNormalizedValueAndRebuild(0.5f);
    const bool nullFillSkipped = bitmap.normalizedValue == 0.75f;

    alignas(HudUiContainer) std::uint8_t ownerStorage[0xa94c] = {};
    auto *owner = reinterpret_cast<HudUiContainer *>(ownerStorage);
    owner->ConstructorDefault();
    TestFieldAt<std::int32_t>(ownerStorage, 0xa944) = 100;
    TestFieldAt<std::int32_t>(ownerStorage, 0xa948) = 200;
    TestFieldAt<std::int32_t>(ownerStorage, 0x14) = 35;

    zVidImagePartial cursorBaseImage{};
    cursorBaseImage.width = 100;
    zVidImagePartial cursorFillImage{};
    cursorFillImage.width = 100;
    cursorFillImage.height = 6;
    HudUiFillBitmap cursorWidget{};
    cursorWidget.Constructor();
    cursorWidget.base.owner = owner;
    cursorWidget.base.base.x = 10;
    cursorWidget.base.base.image = &cursorBaseImage;
    cursorWidget.fillImage = &cursorFillImage;
    cursorWidget.UpdateNormalizedFromCursor();
    const bool cursorUpdated = cursorWidget.normalizedValue == 0.25f &&
                               cursorWidget.fillRect.right == 25 &&
                               cursorWidget.fillRect.bottom == 6;

    zReader::Node fillBitmapItems[4] = {};
    fillBitmapItems[0].value.i32 = 4;
    fillBitmapItems[1].type = zReader::ZRDR_NODE_STRING;
    fillBitmapItems[1].value.str = const_cast<char *>("missing_fill_bitmap");
    fillBitmapItems[2].type = zReader::ZRDR_NODE_INT;
    fillBitmapItems[2].value.i32 = 7;
    fillBitmapItems[3].type = zReader::ZRDR_NODE_INT;
    fillBitmapItems[3].value.i32 = 8;

    zReader::Node rootItems[3] = {};
    rootItems[0].value.i32 = 3;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("FILLBITMAP");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = fillBitmapItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    HudUiFillBitmap loadedWidget{};
    loadedWidget.Constructor();
    loadedWidget.base.base.image = &zVid_Image::g_zImage_DefaultImage;
    loadedWidget.base.base.ownsImage = 0;
    const std::int32_t loadResult = loadedWidget.LoadFromZrd(&root, owner);
    const bool loaded = loadResult == 1 && loadedWidget.base.owner == owner &&
                        loadedWidget.fillImage == nullptr &&
                        loadedWidget.previewImage == &zVid_Image::g_zImage_DefaultImage &&
                        loadedWidget.base.base.x == 107 && loadedWidget.base.base.y == 208;
    loadedWidget.ScalarDeletingDestructor(0);

    HudUiFillBitmap destructorWidget{};
    destructorWidget.Constructor();
    destructorWidget.base.base.image = &zVid_Image::g_zImage_DefaultImage;
    destructorWidget.base.base.ownsImage = 0;
    destructorWidget.previewImage = &zVid_Image::g_zImage_DefaultImage;
    destructorWidget.fillImage = &zVid_Image::g_zImage_DefaultImage;
    HudUiFillBitmap *const scalarResult = destructorWidget.ScalarDeletingDestructor(0);
    const bool destructed = scalarResult == &destructorWidget &&
                            destructorWidget.base.base.ftable ==
                                reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);

    return constructed && normalized && setNormalized && drawn && dirtyDrawn && nullFillSkipped &&
                   cursorUpdated && loaded && destructed
               ? 0
               : 1;
}

extern "C" int zhud_zrd_widget_ex17c_item_core_smoke(void) {
    HudUiZrdWidgetEx17C_Item item{};
    item.itemIndex = 7;
    item.selectedRolloverImage = reinterpret_cast<zVidImagePartial *>(0x1111);
    item.unselectedRolloverImage = reinterpret_cast<zVidImagePartial *>(0x2222);
    item.Constructor();

    const bool constructed =
        item.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiZrdWidgetEx17C_Item_FTable) &&
        item.selected == 0 && item.ownerSelector == nullptr && item.mouseRectValid == 0 &&
        item.selectedImage == nullptr && item.unselectedImage == nullptr && item.itemIndex == 7 &&
        item.selectedRolloverImage == reinterpret_cast<zVidImagePartial *>(0x1111) &&
        item.unselectedRolloverImage == reinterpret_cast<zVidImagePartial *>(0x2222);

    zVidImagePartial selectedImage{};
    zVidImagePartial unselectedImage{};
    zVidImagePartial selectedRollover{};
    zVidImagePartial unselectedRollover{};
    selectedImage.width = 11;
    selectedImage.height = 13;
    item.base.modeOrEnabled = 1;
    item.selectedImage = &selectedImage;
    item.unselectedImage = &unselectedImage;
    item.selectedRolloverImage = &selectedRollover;
    item.unselectedRolloverImage = &unselectedRollover;

    item.SetSelected(1);
    const bool selectedOn = item.selected == 1 && item.base.defaultImage == &selectedImage &&
                            item.base.rolloverImage == &selectedRollover &&
                            item.base.base.image == &selectedImage;

    item.SetSelected(0);
    const bool selectedOff = item.selected == 0 && item.base.defaultImage == &unselectedImage &&
                             item.base.rolloverImage == &unselectedRollover &&
                             item.base.base.image == &unselectedImage;

    item.base.base.x = 3;
    item.base.base.y = 4;
    item.base.base.image = &selectedImage;
    item.mouseRectValid = 0;
    HudUiRect *const bounds = item.GetMouseRectOrBounds();
    item.mouseRect = {1, 2, 5, 6};
    item.mouseRectValid = 1;
    HudUiRect *const mouseRect = item.GetMouseRectOrBounds();
    const bool boundsOk = bounds == &item.base.boundsRect && bounds->left == 3 &&
                          bounds->top == 4 && bounds->right == 14 && bounds->bottom == 17 &&
                          mouseRect == &item.mouseRect && mouseRect->right == 5;

    item.selected = 0;
    item.base.defaultImage = &unselectedImage;
    item.base.rolloverImage = &unselectedRollover;
    item.base.base.image = &unselectedImage;
    item.ShowPreviewIfNotSelected();
    const bool showPreview = item.base.base.image == &unselectedRollover;
    item.HidePreviewIfNotSelected();
    const bool hidePreview = item.base.base.image == &unselectedImage;
    item.selected = 1;
    item.base.base.image = &unselectedImage;
    item.ShowPreviewIfNotSelected();
    item.HidePreviewIfNotSelected();
    const bool selectedSkipsPreview = item.base.base.image == &unselectedImage;

    alignas(HudUiContainer) std::uint8_t loadOwnerStorage[0xa94c] = {};
    auto *loadOwner = reinterpret_cast<HudUiContainer *>(loadOwnerStorage);
    loadOwner->ConstructorDefault();
    TestFieldAt<std::int32_t>(loadOwnerStorage, 0xa944) = 10;
    TestFieldAt<std::int32_t>(loadOwnerStorage, 0xa948) = 20;

    zReader::Node mouseOverItems[5] = {};
    mouseOverItems[0].value.i32 = 5;
    mouseOverItems[1].type = zReader::ZRDR_NODE_INT;
    mouseOverItems[1].value.i32 = 2;
    mouseOverItems[2].type = zReader::ZRDR_NODE_INT;
    mouseOverItems[2].value.i32 = 3;
    mouseOverItems[3].type = zReader::ZRDR_NODE_INT;
    mouseOverItems[3].value.i32 = 4;
    mouseOverItems[4].type = zReader::ZRDR_NODE_INT;
    mouseOverItems[4].value.i32 = 5;

    zReader::Node loadRootItems[3] = {};
    loadRootItems[0].value.i32 = 3;
    loadRootItems[1].type = zReader::ZRDR_NODE_STRING;
    loadRootItems[1].value.str = const_cast<char *>("MOUSEOVER");
    loadRootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    loadRootItems[2].value.nodes = mouseOverItems;

    zReader::Node loadRoot{};
    loadRoot.type = zReader::ZRDR_NODE_ARRAY;
    loadRoot.value.nodes = loadRootItems;

    zVidImagePartial loadImage{};
    loadImage.width = 11;
    loadImage.height = 13;
    HudUiZrdWidgetEx17C_Item loadedItem{};
    loadedItem.Constructor();
    loadedItem.base.base.image = &loadImage;
    loadedItem.base.base.ownsImage = 0;
    const bool loaded = loadedItem.LoadFromZrd(&loadRoot, loadOwner) == 1 &&
                        loadedItem.unselectedImage == &loadImage &&
                        loadedItem.unselectedRolloverImage == nullptr &&
                        loadedItem.selectedImage == nullptr && loadedItem.mouseRectValid == 1 &&
                        loadedItem.mouseRect.left == 13 && loadedItem.mouseRect.top == 22 &&
                        loadedItem.mouseRect.right == 18 && loadedItem.mouseRect.bottom == 26;
    loadedItem.base.base.image = nullptr;

    item.base.defaultImage = nullptr;
    item.base.rolloverImage = nullptr;
    item.base.base.image = nullptr;
    item.selectedImage = nullptr;
    item.unselectedImage = nullptr;
    item.selectedRolloverImage = nullptr;
    item.unselectedRolloverImage = nullptr;

    HudUiZrdWidgetEx17C selector{};
    selector.optionCount = 5;
    selector.selectedIndex = 9;
    selector.options[0] = reinterpret_cast<HudUiZrdWidgetEx17C_Item *>(0x1111);
    selector.Constructor();
    bool optionsClear = true;
    for (HudUiZrdWidgetEx17C_Item *option : selector.options) {
        optionsClear = optionsClear && option == nullptr;
    }
    const bool selectorConstructed =
        selector.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiZrdWidgetEx17C_FTable) &&
        selector.optionCount == 0 && selector.selectedIndex == 9 && optionsClear;

    zReader::Node radioChild0Items[1] = {};
    radioChild0Items[0].value.i32 = 1;
    zReader::Node radioChild1Items[1] = {};
    radioChild1Items[0].value.i32 = 1;
    zReader::Node radioChild2Items[1] = {};
    radioChild2Items[0].value.i32 = 1;
    zReader::Node radioItems[4] = {};
    radioItems[0].value.i32 = 4;
    radioItems[1].type = zReader::ZRDR_NODE_ARRAY;
    radioItems[1].value.nodes = radioChild0Items;
    radioItems[2].type = zReader::ZRDR_NODE_ARRAY;
    radioItems[2].value.nodes = radioChild1Items;
    radioItems[3].type = zReader::ZRDR_NODE_ARRAY;
    radioItems[3].value.nodes = radioChild2Items;
    zReader::Node selectorRootItems[3] = {};
    selectorRootItems[0].value.i32 = 3;
    selectorRootItems[1].type = zReader::ZRDR_NODE_STRING;
    selectorRootItems[1].value.str = const_cast<char *>("RADIO");
    selectorRootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    selectorRootItems[2].value.nodes = radioItems;
    zReader::Node selectorRoot{};
    selectorRoot.type = zReader::ZRDR_NODE_ARRAY;
    selectorRoot.value.nodes = selectorRootItems;

    HudUiZrdWidgetEx17C loadedSelector{};
    loadedSelector.Constructor();
    const bool selectorLoaded =
        loadedSelector.LoadFromZrd(&selectorRoot, loadOwner) == 1 &&
        loadedSelector.base.owner == loadOwner && loadedSelector.optionCount == 3 &&
        loadedSelector.selectedIndex == 0 && loadedSelector.options[0] != nullptr &&
        loadedSelector.options[1] != nullptr && loadedSelector.options[2] != nullptr &&
        loadedSelector.options[3] == nullptr &&
        loadedSelector.options[0]->ownerSelector == &loadedSelector &&
        loadedSelector.options[1]->ownerSelector == &loadedSelector &&
        loadedSelector.options[2]->ownerSelector == &loadedSelector &&
        loadedSelector.options[0]->itemIndex == 0 && loadedSelector.options[1]->itemIndex == 1 &&
        loadedSelector.options[2]->itemIndex == 2 && loadedSelector.options[0]->selected == 1 &&
        loadedSelector.options[1]->selected == 0 && loadedSelector.options[2]->selected == 0;
    loadedSelector.DestructorCore();

    HudUiZrdWidgetEx17C_Item selectorItem0{};
    HudUiZrdWidgetEx17C_Item selectorItem1{};
    selectorItem0.Constructor();
    selectorItem1.Constructor();
    selectorItem0.base.modeOrEnabled = 0;
    selectorItem1.base.modeOrEnabled = 0;
    selector.options[0] = &selectorItem0;
    selector.options[1] = &selectorItem1;
    const bool setSelectedIndex = selector.SetSelectedIndex(1) == 1 &&
                                  selector.selectedIndex == 1 && selectorItem0.selected == 0 &&
                                  selectorItem1.selected == 1;

    zVidImagePartial selectorItem0Image{};
    zVidImagePartial selectorItem0Rollover{};
    zVidImagePartial selectorItem1Image{};
    zVidImagePartial selectorItem1Selected{};
    selector.optionCount = 2;
    selectorItem0.base.modeOrEnabled = 1;
    selectorItem0.unselectedImage = &selectorItem0Image;
    selectorItem0.unselectedRolloverImage = &selectorItem0Rollover;
    selectorItem0.base.defaultImage = &selectorItem0Image;
    selectorItem0.base.rolloverImage = &selectorItem0Rollover;
    selectorItem0.base.base.image = &selectorItem0Rollover;
    selectorItem1.base.modeOrEnabled = 1;
    selectorItem1.ownerSelector = &selector;
    selectorItem1.itemIndex = 1;
    selectorItem1.unselectedImage = &selectorItem1Image;
    selectorItem1.selectedImage = &selectorItem1Selected;
    selectorItem1.selectedRolloverImage = &selectorItem1Selected;
    selectorItem1.base.defaultImage = &selectorItem1Image;
    selectorItem1.base.base.image = &selectorItem1Image;
    selectorItem1.OnActivateSelectSelf();
    const bool activatedSelection = selector.selectedIndex == 1 && selectorItem0.selected == 0 &&
                                    selectorItem0.base.base.image == &selectorItem0Image &&
                                    selectorItem1.selected == 1 &&
                                    selectorItem1.base.base.image == &selectorItem1Selected;
    selector.options[0] = nullptr;
    selector.options[1] = nullptr;

    HudUiZrdWidgetEx17C destructorSelector{};
    destructorSelector.Constructor();
    auto *const heapItem =
        static_cast<HudUiZrdWidgetEx17C_Item *>(::operator new(sizeof(HudUiZrdWidgetEx17C_Item)));
    heapItem->Constructor();
    destructorSelector.options[2] = heapItem;
    destructorSelector.DestructorCore();
    const bool selectorDestructed =
        destructorSelector.options[2] == nullptr &&
        destructorSelector.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);

    HudUiZrdWidgetEx17C_Item scalarItem{};
    scalarItem.Constructor();
    HudUiZrdWidgetEx17C_Item *const scalarResult = scalarItem.ScalarDeletingDestructor(0);
    const bool scalarDeleted =
        scalarResult == &scalarItem &&
        scalarItem.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);

    return constructed && selectedOn && selectedOff && boundsOk && showPreview && hidePreview &&
                   selectedSkipsPreview && loaded && selectorConstructed && selectorLoaded &&
                   setSelectedIndex && activatedSelection && selectorDestructed && scalarDeleted
               ? 0
               : 1;
}

extern "C" int zhud_cmd_bind_button_base_constructor_smoke(void) {
    HudUiListSelectorItem item;
    std::memset(&item, 0xcc, sizeof(item));
    item.Constructor();
    TestFieldAt<std::int32_t>(&item, 0x14) = 7;
    TestFieldAt<std::int32_t>(&item, 0x18) = 9;
    TestFieldAt<std::int32_t>(&item, 0x25c) = 40;
    TestFieldAt<std::int32_t>(&item, 0x260) = 12;
    TestFieldAt<std::int32_t>(&item, 0x274) = 2;
    TestFieldAt<std::uint32_t>(&item, 0x270) = 0;
    item.Draw();
    const bool itemConstructed =
        TestFieldAt<const void *>(&item, 0x00) == &g_HudUiListSelectorItem_FTable &&
        item.owner == reinterpret_cast<void *>(0xcccccccc) &&
        TestFieldAt<std::int32_t>(&item, 0x20) == 7 &&
        TestFieldAt<std::int32_t>(&item, 0x24) == 9 &&
        TestFieldAt<std::int32_t>(&item, 0x28) == 47 &&
        TestFieldAt<std::int32_t>(&item, 0x2c) == 19;
    reinterpret_cast<HudUiPanel *>(&item)->Destructor();

    alignas(HudUiPanel) std::uint8_t drawPanelStorage[0x2ac] = {};
    auto *drawPanel = reinterpret_cast<HudUiPanel *>(drawPanelStorage);
    zVidImagePartial drawImage{};
    drawPanel->ConstructorDefault("A", 3, 4);
    drawPanel->textPick = &drawImage;
    TestFieldAt<zVidRect32>(drawPanel, 0x28c) = {1, 2, 5, 6};
    TestFieldAt<std::uint32_t>(drawPanel, 0x270) = 0;
    g_testBlitCount = 0;
    g_zVideo_pfnBltSourceToPrimary = &TestBltSourceToPrimary;
    drawPanel->Draw();
    const bool panelDraw = g_testBlitCount == 1 && g_testBlitImages[0] == &drawImage &&
                           g_testBlitX[0] == 3 && g_testBlitY[0] == 4 && g_testBlitFlags[0] == 0 &&
                           g_testBlitHasRect[0] != 0;
    g_zVideo_pfnBltSourceToPrimary = nullptr;
    drawPanel->textPick = nullptr;
    drawPanel->Destructor();

    HudCmdBindButtonBase button;
    std::memset(&button, 0xcc, sizeof(button));
    button.Constructor();
    const bool buttonConstructed =
        button.base.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudCmdBindButtonBase_FTable) &&
        TestFieldAt<const void *>(&button.bindPanel, 0x00) == &g_HudUiListSelectorItem_FTable &&
        button.bindingSlotTotalCount == 0 &&
        button.visibleBindingSlotCount == static_cast<std::int32_t>(0xcccccccc) &&
        button.bindingSlotPanels == nullptr && button.bindingVec.allocator == 0xcc &&
        button.bindingVec.begin == nullptr && button.bindingVec.end == nullptr &&
        button.bindingVec.capacity == nullptr && button.bindingSlotSpacing == 0x0f &&
        button.selectedBindingIndex == -1 && button.visibleListOffsetX == 0.0f &&
        button.visibleListOffsetY == 0.0f && button.overflowListOffsetX == 0.0f &&
        button.overflowListOffsetY == 0.0f &&
        button.selectedFontStyleRef == static_cast<std::int32_t>(0xcccccccc) &&
        button.listFontStyleRef == static_cast<std::int32_t>(0xcccccccc);

    auto *firstBinding =
        static_cast<HudCmdBindingEntry *>(::operator new(sizeof(HudCmdBindingEntry)));
    auto *secondBinding =
        static_cast<HudCmdBindingEntry *>(::operator new(sizeof(HudCmdBindingEntry)));
    firstBinding->displayText = static_cast<char *>(std::malloc(6));
    secondBinding->displayText = nullptr;
    std::strcpy(firstBinding->displayText, "Fire");
    auto **bindingSlots =
        static_cast<HudCmdBindingEntry **>(::operator new(2 * sizeof(HudCmdBindingEntry *)));
    bindingSlots[0] = firstBinding;
    bindingSlots[1] = secondBinding;
    button.bindingVec.begin = bindingSlots;
    button.bindingVec.end = bindingSlots + 2;
    button.bindingVec.capacity = bindingSlots + 2;
    button.ClearBindingEntries();
    const bool bindingsCleared = button.bindingVec.begin == bindingSlots &&
                                 button.bindingVec.end == bindingSlots &&
                                 button.bindingVec.capacity == bindingSlots + 2 &&
                                 bindingSlots[0] == nullptr && bindingSlots[1] == nullptr;
    ::operator delete(bindingSlots);
    button.bindingVec.begin = nullptr;
    button.bindingVec.end = nullptr;
    button.bindingVec.capacity = nullptr;

    HudCmdBindingVector rawVector{};
    rawVector.allocator = 0x7f;
    rawVector.begin = ::operator new(3 * sizeof(void *));
    rawVector.end = static_cast<void **>(rawVector.begin) + 3;
    rawVector.capacity = rawVector.end;
    void **const oldEnd = zUtil_StdPtrVector_Clear(&rawVector);
    const bool vectorCleared = oldEnd == static_cast<void **>(rawVector.begin) + 3 &&
                               rawVector.end == rawVector.begin && rawVector.capacity == oldEnd &&
                               rawVector.allocator == 0x7f;
    zUtil_StdPtrVector_FreeBufferAndReset(&rawVector);
    const bool vectorFreed = rawVector.begin == nullptr && rawVector.end == nullptr &&
                             rawVector.capacity == nullptr && rawVector.allocator == 0x7f;
    reinterpret_cast<HudUiPanel *>(&button.bindPanel)->Destructor();

    HudCmdBindButtonBase rebuildButton{};
    rebuildButton.Constructor();
    rebuildButton.base.base.originX = 10;
    rebuildButton.base.base.originY = 100;
    rebuildButton.visibleListOffsetX = 1.5f;
    rebuildButton.visibleListOffsetY = 2.5f;
    rebuildButton.overflowListOffsetX = 3.5f;
    rebuildButton.overflowListOffsetY = 4.5f;
    rebuildButton.bindingSlotSpacing = 15;
    rebuildButton.RebuildBindingSlotWidgets(3, 2);
    const bool rebuilt =
        rebuildButton.bindingSlotTotalCount == 3 && rebuildButton.visibleBindingSlotCount == 2 &&
        rebuildButton.bindingSlotPanels != nullptr &&
        TestFieldAt<std::int32_t>(rebuildButton.bindingSlotPanels, 0x14) == 11 &&
        TestFieldAt<std::int32_t>(rebuildButton.bindingSlotPanels, 0x18) == 72 &&
        TestFieldAt<std::int32_t>(&rebuildButton.bindingSlotPanels[1], 0x14) == 11 &&
        TestFieldAt<std::int32_t>(&rebuildButton.bindingSlotPanels[1], 0x18) == 87 &&
        TestFieldAt<std::int32_t>(&rebuildButton.bindingSlotPanels[2], 0x14) == 13 &&
        TestFieldAt<std::int32_t>(&rebuildButton.bindingSlotPanels[2], 0x18) == 119 &&
        TestFieldAt<std::int32_t>(&rebuildButton.bindPanel, 0x14) == 10 &&
        TestFieldAt<std::int32_t>(&rebuildButton.bindPanel, 0x18) == 100;
    DeleteListSelectorItemArray(rebuildButton.bindingSlotPanels);
    rebuildButton.bindingSlotPanels = nullptr;
    reinterpret_cast<HudUiPanel *>(&rebuildButton.bindPanel)->Destructor();

    alignas(HudUiContainer) std::uint8_t ownerStorage[0xa94c] = {};
    auto *owner = reinterpret_cast<HudUiContainer *>(ownerStorage);
    owner->ConstructorDefault();
    TestFieldAt<std::int32_t>(ownerStorage, 0xa944) = 20;
    TestFieldAt<std::int32_t>(ownerStorage, 0xa948) = 30;

    auto *const styles = reinterpret_cast<HudFontStyle *>(ownerStorage + 0x1cec);
    styles[1].validMarker = 1;
    styles[1].fontName = "Arial";
    styles[1].fontSize = 8;
    styles[1].textColor = 0x00112233;
    styles[1].shadowEnabled = 1;
    styles[1].fontWeight = 0x190;
    styles[2].validMarker = 1;
    styles[2].fontName = "Arial";
    styles[2].fontSize = 9;
    styles[2].textColor = 0x00445566;
    styles[2].shadowEnabled = 0;
    styles[2].fontWeight = 0x1f4;

    zReader::Node visiblePair[3] = {};
    visiblePair[0].value.i32 = 3;
    visiblePair[1].type = zReader::ZRDR_NODE_INT;
    visiblePair[1].value.i32 = 5;
    visiblePair[2].type = zReader::ZRDR_NODE_INT;
    visiblePair[2].value.i32 = 6;
    zReader::Node overflowPair[3] = {};
    overflowPair[0].value.i32 = 3;
    overflowPair[1].type = zReader::ZRDR_NODE_INT;
    overflowPair[1].value.i32 = 7;
    overflowPair[2].type = zReader::ZRDR_NODE_INT;
    overflowPair[2].value.i32 = 8;
    zReader::Node listOffsetItems[3] = {};
    listOffsetItems[0].value.i32 = 3;
    listOffsetItems[1].type = zReader::ZRDR_NODE_ARRAY;
    listOffsetItems[1].value.nodes = visiblePair;
    listOffsetItems[2].type = zReader::ZRDR_NODE_ARRAY;
    listOffsetItems[2].value.nodes = overflowPair;
    zReader::Node listSizeItems[3] = {};
    listSizeItems[0].value.i32 = 3;
    listSizeItems[1].type = zReader::ZRDR_NODE_INT;
    listSizeItems[1].value.i32 = 2;
    listSizeItems[2].type = zReader::ZRDR_NODE_INT;
    listSizeItems[2].value.i32 = 1;
    zReader::Node loadItems[11] = {};
    loadItems[0].value.i32 = 11;
    loadItems[1].type = zReader::ZRDR_NODE_STRING;
    loadItems[1].value.str = const_cast<char *>("SELECTED_FONT");
    loadItems[2].type = zReader::ZRDR_NODE_INT;
    loadItems[2].value.i32 = 1;
    loadItems[3].type = zReader::ZRDR_NODE_STRING;
    loadItems[3].value.str = const_cast<char *>("LIST_FONT");
    loadItems[4].type = zReader::ZRDR_NODE_INT;
    loadItems[4].value.i32 = 2;
    loadItems[5].type = zReader::ZRDR_NODE_STRING;
    loadItems[5].value.str = const_cast<char *>("SPACING");
    loadItems[6].type = zReader::ZRDR_NODE_INT;
    loadItems[6].value.i32 = 12;
    loadItems[7].type = zReader::ZRDR_NODE_STRING;
    loadItems[7].value.str = const_cast<char *>("LIST_OFFSET");
    loadItems[8].type = zReader::ZRDR_NODE_ARRAY;
    loadItems[8].value.nodes = listOffsetItems;
    loadItems[9].type = zReader::ZRDR_NODE_STRING;
    loadItems[9].value.str = const_cast<char *>("LISTSIZE");
    loadItems[10].type = zReader::ZRDR_NODE_ARRAY;
    loadItems[10].value.nodes = listSizeItems;
    zReader::Node loadRoot{};
    loadRoot.type = zReader::ZRDR_NODE_ARRAY;
    loadRoot.value.nodes = loadItems;

    HudCmdBindButtonBase loadedButton{};
    loadedButton.Constructor();
    const bool loaded =
        loadedButton.LoadFromZrd(&loadRoot, owner) == 1 && loadedButton.base.base.originX == 20 &&
        loadedButton.base.base.originY == 30 && loadedButton.selectedFontStyleRef == 1 &&
        loadedButton.listFontStyleRef == 2 && loadedButton.bindingSlotSpacing == 12 &&
        loadedButton.visibleListOffsetX == 5.0f && loadedButton.visibleListOffsetY == 6.0f &&
        loadedButton.overflowListOffsetX == 7.0f && loadedButton.overflowListOffsetY == 8.0f &&
        loadedButton.bindingSlotTotalCount == 2 && loadedButton.visibleBindingSlotCount == 1 &&
        loadedButton.bindPanel.owner == &loadedButton &&
        loadedButton.bindingSlotPanels[0].owner == &loadedButton &&
        loadedButton.bindingSlotPanels[1].owner == &loadedButton &&
        TestFieldAt<std::uint32_t>(&loadedButton.bindPanel, 0x14c) == 0x00112233 &&
        TestFieldAt<std::uint32_t>(&loadedButton.bindingSlotPanels[0], 0x14c) == 0x00445566;
    DeleteListSelectorItemArray(loadedButton.bindingSlotPanels);
    loadedButton.bindingSlotPanels = nullptr;
    reinterpret_cast<HudUiPanel *>(&loadedButton.bindPanel)->Destructor();

    const bool ok = itemConstructed && panelDraw && buttonConstructed && bindingsCleared &&
                    vectorCleared && vectorFreed && rebuilt && loaded;
    return ok ? 0 : 1;
}

extern "C" int zhud_message_box_leaf_handlers_smoke(void) {
    HudUiMessageBoxDialog dialog{};
    TestFieldAt<const HudUiMessageBoxDialog_FTable *>(&dialog, 0) = &g_HudUiMessageBoxDialog_FTable;
    dialog.modalResult = 99;
    dialog.modalFrameCountdown = 7;
    dialog.OnOk();
    const bool okResult = dialog.modalResult == 1 && dialog.modalFrameCountdown == 0;

    dialog.modalResult = 99;
    dialog.modalFrameCountdown = 7;
    dialog.OnCancel();
    const bool cancelResult = dialog.modalResult == 2 && dialog.modalFrameCountdown == 0;

    HudUiMessageBoxOkButton okButton{};
    okButton.base.Constructor();
    okButton.base.owner = &dialog;
    dialog.modalResult = 0;
    dialog.modalFrameCountdown = 5;
    okButton.OnActivate();
    const bool okActivated = dialog.modalResult == 1 && dialog.modalFrameCountdown == 0;

    HudUiMessageBoxCancelButton cancelButton{};
    cancelButton.base.Constructor();
    cancelButton.base.owner = &dialog;
    dialog.modalResult = 0;
    dialog.modalFrameCountdown = 5;
    cancelButton.OnActivate();
    const bool cancelActivated = dialog.modalResult == 2 && dialog.modalFrameCountdown == 0;

    return okResult && cancelResult && okActivated && cancelActivated ? 0 : 1;
}

extern "C" int zhud_background_container_focus_smoke(void) {
    HudUiBackgroundContainer container{};
    container.inputFocusElement = reinterpret_cast<HudUiElement *>(0x1111);
    container.captureTransitionMask = 0;
    container.base.childHead = reinterpret_cast<HudUiElement *>(0x2222);
    container.Constructor(7);

    HudUiElement element{};
    const bool constructed =
        container.base.vptr ==
            reinterpret_cast<const HudUiContainer_FTable *>(&g_HudUiBackgroundContainer_FTable) &&
        container.base.enabled == 0 && container.base.childHead == nullptr &&
        container.base.childTail == nullptr && container.inputFocusElement == nullptr &&
        container.captureTransitionMask == 7;

    container.SetInputFocus(&element);
    const bool focus =
        container.GetInputFocus() == &element && container.inputFocusElement == &element;

    container.Destructor();
    const bool destructed = container.base.vptr == &g_HudUiContainer_FTable;

    return constructed && focus && destructed ? 0 : 1;
}

extern "C" int zhud_background_update_input_focus_smoke(void) {
    HudUiCommon_FTable table{};
    table.slots[0x08 / 4] = MethodAddress(&TestBackgroundInputElement::DrawBase);
    table.slots[0x0c / 4] = MethodAddress(&TestBackgroundInputElement::SetPos);
    table.slots[0x24 / 4] = MethodAddress(&TestBackgroundInputElement::Update);
    table.slots[0x30 / 4] =
        MethodAddress(&TestBackgroundInputElement::OnPrimaryButtonReleased);
    table.slots[0x34 / 4] =
        MethodAddress(&TestBackgroundInputElement::OnSecondaryButtonReleased);
    table.slots[0x38 / 4] = MethodAddress(&TestBackgroundInputElement::OnHoverRepeat);
    table.slots[0x3c / 4] = MethodAddress(&TestBackgroundInputElement::OnHoverEnter);
    table.slots[0x40 / 4] = MethodAddress(&TestBackgroundInputElement::OnHoverExit);
    table.slots[0x44 / 4] = MethodAddress(&TestBackgroundInputElement::OnCaptureEnter);
    table.slots[0x48 / 4] = MethodAddress(&TestBackgroundInputElement::OnCaptureExit);
    table.slots[0x4c / 4] =
        MethodAddress(&TestBackgroundInputElement::OnPointerButtonState);
    table.slots[0x50 / 4] = MethodAddress(&TestBackgroundInputElement::OnActivate);
    table.slots[0x54 / 4] = MethodAddress(&TestBackgroundInputElement::ShouldHandleInput);
    table.slots[0x58 / 4] = MethodAddress(&TestBackgroundInputElement::AfterInputUpdate);
    table.slots[0x5c / 4] = MethodAddress(&TestBackgroundInputElement::HitTest);

    TestBackgroundInputElement child{};
    TestBackgroundInputElement focus{};
    child.base.ftable = &table;
    child.hitResult = 1;
    child.shouldHandleResult = 1;
    focus.base.ftable = &table;

    HudUiBackground background{};
    background.base.base.enabled = 1;
    background.base.base.childHead = &child.base;
    background.base.base.childTail = &child.base;
    background.base.inputFocusElement = &focus.base;
    background.base.captureTransitionMask = 4;

    g_zInput_MouseStateSnapshot = {};
    g_zInput_MouseStateSnapshot.cursorClientX = 123;
    g_zInput_MouseStateSnapshot.cursorClientY = 456;
    g_zInput_MouseStateSnapshot.button1Transition = 4;
    g_zInput_MouseStateSnapshot.button2Transition = 0;

    g_backgroundChildElement = &child;
    g_backgroundFocusElement = &focus;
    g_backgroundHitCount = 0;
    g_backgroundShouldHandleCount = 0;
    g_backgroundShouldHandleHovered = -1;
    g_backgroundHoverEnterCount = 0;
    g_backgroundHoverRepeatCount = 0;
    g_backgroundHoverExitCount = 0;
    g_backgroundCaptureEnterCount = 0;
    g_backgroundCaptureExitCount = 0;
    g_backgroundPrimaryReleaseCount = 0;
    g_backgroundSecondaryReleaseCount = 0;
    g_backgroundPointerStateCount = 0;
    g_backgroundActivateCount = 0;
    g_backgroundAfterInputCount = 0;
    g_backgroundAfterHovered = -1;
    g_backgroundDrawBaseCount = 0;
    g_backgroundChildUpdateCount = 0;
    g_backgroundFocusUpdateCount = 0;
    g_backgroundSetPosCount = 0;
    g_backgroundSetPosX = 0;
    g_backgroundSetPosY = 0;
    g_backgroundChildUpdateDelta = 0.0f;
    g_backgroundFocusUpdateDelta = 0.0f;

    background.Update(0.25f);

    const bool inputDispatched =
        g_backgroundHitCount == 1 && g_backgroundShouldHandleCount == 1 &&
        g_backgroundShouldHandleHovered == 1 && g_backgroundHoverEnterCount == 1 &&
        g_backgroundHoverRepeatCount == 0 && g_backgroundCaptureEnterCount == 1 &&
        g_backgroundPrimaryReleaseCount == 1 && g_backgroundActivateCount == 1 &&
        g_backgroundAfterInputCount == 1 && g_backgroundAfterHovered == 1 &&
        g_backgroundSecondaryReleaseCount == 0 && g_backgroundPointerStateCount == 0 &&
        g_backgroundHoverExitCount == 0 && g_backgroundCaptureExitCount == 0 &&
        child.base.state == 3;

    const bool focusUpdated =
        g_backgroundDrawBaseCount == 1 && g_backgroundChildUpdateCount == 1 &&
        g_backgroundChildUpdateDelta == 0.25f && g_backgroundSetPosCount == 1 &&
        g_backgroundSetPosX == 123 && g_backgroundSetPosY == 456 &&
        g_backgroundFocusUpdateCount == 1 && g_backgroundFocusUpdateDelta == 0.25f &&
        focus.base.x == 123 && focus.base.y == 456;

    g_backgroundChildElement = nullptr;
    g_backgroundFocusElement = nullptr;
    return inputDispatched && focusUpdated ? 0 : 1;
}

extern "C" int zhud_background_cursor_widget_member_constructor_smoke(void) {
    HudUiBackgroundCursorWidget cursor{};
    cursor.captureEnabled = -1;
    cursor.captureSourceSelector = -1;
    cursor.capturedImage = reinterpret_cast<zVidImagePartial *>(0x1111);
    cursor.reservedC8 = 0x2222;
    cursor.reservedCC = 0x3333;

    const HudUiBackgroundCursorWidget *const result = cursor.MemberConstructorLocal(nullptr, 7);

    const bool constructed =
        result == &cursor &&
        cursor.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiBackgroundCursorWidget_FTable) &&
        cursor.base.image == nullptr && cursor.base.alignFlags == 0 &&
        cursor.capturedImage == nullptr && cursor.captureEnabled == 7 &&
        cursor.captureSourceSelector == 1 && cursor.reservedC8 == 0 && cursor.reservedCC == 0;

    cursor.DestructorCore();
    const bool destructed =
        cursor.base.ftable == reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);

    return constructed && destructed ? 0 : 1;
}

extern "C" int zhud_background_constructor_smoke(void) {
    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption{};
    vmodeOption.payloadOrBuffer = 6;
    vmodeOption.name = vmodeName;
    vmodeOption.next = nullptr;

    zOptionEntryPartial *const oldOptionsHead = g_zGame_Options_OptionListHead;
    g_zGame_Options_OptionListHead = &vmodeOption;

    HudUiBackground background{};
    const HudUiBackground *const result = background.Constructor();

    bool soundsInitialized = true;
    for (int index = 0; index < 10; ++index) {
        soundsInitialized =
            soundsInitialized && background.backgroundSounds[index].sample == nullptr &&
            background.backgroundSounds[index].volume == 1.0f &&
            background.backgroundSounds[index].playHandle == nullptr;
    }

    const bool constructed =
        result == &background &&
        background.base.base.vptr ==
            reinterpret_cast<const HudUiContainer_FTable *>(&g_HudUiBackground_FTable) &&
        background.base.base.enabled == 0 && background.base.base.childHead == nullptr &&
        background.base.base.childTail == nullptr && background.base.inputFocusElement == nullptr &&
        background.base.captureTransitionMask == 1 &&
        background.cursorWidget.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(
                &g_HudUiBackgroundCursorWidget_MemberFTable) &&
        background.cursorWidget.base.image == nullptr &&
        background.cursorWidget.captureEnabled == 1 &&
        background.cursorWidget.captureSourceSelector == 1 &&
        background.primaryClipImage == nullptr && background.capturedCompositeImage == nullptr &&
        background.backgroundImageWidgets[0].ftable == &g_HudUiWidget_FTable &&
        background.backgroundImageWidgets[19].ftable == &g_HudUiWidget_FTable &&
        background.backgroundVideoWidgets[0].base.ftable ==
            reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiBackgroundVideoWidget_FTable) &&
        background.backgroundVideoWidgets[9].base.ftable ==
            reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiBackgroundVideoWidget_FTable) &&
        background.backgroundVideoWidgets[0].stream == nullptr &&
        background.fontStyles[0].validMarker == 0 && background.fontStyles[0].fontWeight == 500 &&
        background.fontStyles[19].fontWeight == 500 &&
        TestFieldAt<const HudUiPanel_FTable *>(&background.backgroundTextPanels[0], 0) ==
            &g_HudUiTransitionTextPanel_FTable &&
        background.backgroundTextPanels[0].flashResetValue == 0.349999994f &&
        background.backgroundTextPanels[0].flashDirectionSign == 1 &&
        background.loadedRoot == nullptr && background.cfgRoot == nullptr && soundsInitialized &&
        background.uiOriginX == 0 && background.uiOriginY == 60;

    background.Destructor();
    g_zGame_Options_OptionListHead = oldOptionsHead;
    return constructed ? 0 : 1;
}

extern "C" int zhud_background_free_loaded_tree_roots_smoke(void) {
    HudUiBackground background{};
    zReader::Node *const root =
        static_cast<zReader::Node *>(std::malloc(sizeof(zReader::Node)));
    if (root == nullptr) {
        return 1;
    }

    root->type = zReader::ZRDR_NODE_INT;
    root->value.i32 = 42;
    zReader::Node cfgRoot{};
    background.loadedRoot = root;
    background.cfgRoot = &cfgRoot;

    background.FreeLoadedTreeRoots(0x1234);
    const bool cleared = background.loadedRoot == nullptr && background.cfgRoot == nullptr;

    background.FreeLoadedTreeRoots(0);
    return cleared ? 0 : 1;
}

extern "C" int zhud_play_powerup_sfx_smoke(void) {
    zSndSample *const oldPowerupSample = g_HudUi_PowerupSample;
    const unsigned char oldPowerupInitFlags = g_HudUi_PowerupSampleInitFlags;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldSndPreInitialized = g_zSnd_PreInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;
    void *const oldGlobalVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const zSndSampleSetRegistry oldSampleRegistry = g_zSnd_SampleSetRegistry;

    HudUiTestDirectSoundBufferVTable directSoundVTable = {};
    directSoundVTable.GetStatus = &HudUiTestDirectSoundGetStatus;
    directSoundVTable.SetVolume = &HudUiTestDirectSoundSetVolume;
    directSoundVTable.SetCurrentPosition = &HudUiTestDirectSoundSetCurrentPosition;
    directSoundVTable.Play = &HudUiTestDirectSoundPlay;
    directSoundVTable.Stop = &HudUiTestDirectSoundStop;
    HudUiTestDirectSoundBuffer directSoundBuffer{&directSoundVTable};

    zSndSample sample = {};
    sample.replayFields.sampleId = "snd_powerup";
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 1.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    zSndSampleSet sampleSet = {};
    sampleSet.sampleCount = 1;
    sampleSet.samples = &sample;
    zSndSampleSet *sampleSetSlots[1] = {&sampleSet};

    float globalVolume = 1.0f;
    g_HudUi_PowerupSample = nullptr;
    g_HudUi_PowerupSampleInitFlags = 0;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_SampleSetRegistry.begin = sampleSetSlots;
    g_zSnd_SampleSetRegistry.end = sampleSetSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = sampleSetSlots + 1;

    g_HudUiTestPowerupPlayCount = 0;
    g_HudUiTestPowerupStopCount = 0;
    g_HudUiTestPowerupVolumeCount = 0;
    g_HudUiTestPowerupPositionCount = 0;

    HudUi::PlayPowerupSfx(1);
    const bool played = g_HudUi_PowerupSample == &sample &&
                        (g_HudUi_PowerupSampleInitFlags & 1) != 0 &&
                        g_HudUiTestPowerupPlayCount == 1 &&
                        g_HudUiTestPowerupVolumeCount == 1 &&
                        g_HudUiTestPowerupPositionCount == 1;

    HudUi::PlayPowerupSfx(0);
    const bool stopped = g_HudUi_PowerupSample == &sample &&
                         g_HudUiTestPowerupStopCount == 1 &&
                         g_HudUiTestPowerupPlayCount == 1;

    g_zSnd_SampleSetRegistry = oldSampleRegistry;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScalePtr;
    g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
    g_zSnd_MuteDepth = oldMuteDepth;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_PreInitialized = oldSndPreInitialized;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_HudUi_PowerupSampleInitFlags = oldPowerupInitFlags;
    g_HudUi_PowerupSample = oldPowerupSample;

    return played && stopped ? 0 : 1;
}

extern "C" int zhud_background_bind_widget_by_name_smoke(void) {
    zReader::Node buttonItems[3] = {};
    buttonItems[0].type = zReader::ZRDR_NODE_INT;
    buttonItems[0].value.i32 = 3;
    buttonItems[1].type = zReader::ZRDR_NODE_STRING;
    buttonItems[1].value.str = const_cast<char *>("TARGET");
    buttonItems[2].type = zReader::ZRDR_NODE_INT;
    buttonItems[2].value.i32 = 99;

    zReader::Node rootItems[3] = {};
    rootItems[0].type = zReader::ZRDR_NODE_INT;
    rootItems[0].value.i32 = 3;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("BUTTONS");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = buttonItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    HudUiWidget_FTable table{};
    table.slots[0x7c / 4] = MethodAddress(&TestBackgroundBindWidget::LoadFromZrd);
    table.slots[0x80 / 4] = MethodAddress(&TestBackgroundBindWidget::PostLoadFromZrd);

    HudUiBackground background{};
    TestBackgroundBindWidget widget{};
    widget.base.ftable = &table;

    const unsigned char directResult =
        background.BindButtonsNodeToWidgetByName(&root, &widget.base, "TARGET");
    const bool directBound = directResult == 0 && widget.loadedNode == &buttonItems[2] &&
                             widget.owner == &background && widget.postLoadCount == 1;

    TestBackgroundBindWidget wrapperWidget{};
    wrapperWidget.base.ftable = &table;
    background.cfgRoot = &root;
    const int wrapperResult = background.BindWidgetByName(nullptr, &wrapperWidget.base, "TARGET");
    const bool wrapperBound = wrapperResult == 0 && wrapperWidget.loadedNode == &buttonItems[2] &&
                              wrapperWidget.owner == &background &&
                              wrapperWidget.postLoadCount == 1;

    TestBackgroundBindWidget missingWidget{};
    missingWidget.base.ftable = &table;
    const unsigned char missingResult =
        background.BindButtonsNodeToWidgetByName(&root, &missingWidget.base, "MISSING");
    const bool missingSkipped = missingResult == 0 && missingWidget.loadedNode == nullptr &&
                                missingWidget.postLoadCount == 0;

    return directBound && wrapperBound && missingSkipped ? 0 : 1;
}

extern "C" int zhud_background_set_enabled_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;
    g_elementInvalidateCount = 0;

    HudUiCommon_FTable table = {};
    table.slots[8] = reinterpret_cast<std::uintptr_t>(TestElementInvalidate);

    HudUiElement first{};
    HudUiElement second{};
    first.ftable = &table;
    first.next = &second;
    second.ftable = &table;
    second.next = nullptr;

    HudUiBackground background{};
    background.base.base.childHead = &first;
    background.base.base.childTail = &second;
    background.base.base.enabled = 0;

    zSndSample sample{};
    background.backgroundSounds[0].sample = &sample;
    background.backgroundSounds[0].volume = 0.5f;
    background.backgroundSounds[1].playHandle = reinterpret_cast<zSndPlayHandle *>(0x1234);

    background.SetEnabled(1);
    const bool enabled =
        background.base.base.enabled == 1 && background.backgroundSounds[0].playHandle == nullptr &&
        background.backgroundSounds[1].playHandle == reinterpret_cast<zSndPlayHandle *>(0x1234) &&
        g_elementInvalidateCount == 2 && (first.flags & 0x80) != 0 && (second.flags & 0x80) != 0;

    zSndPlayHandle handle{};
    background.backgroundSounds[0].playHandle = &handle;
    background.backgroundSounds[1].playHandle = reinterpret_cast<zSndPlayHandle *>(0x5678);
    background.SetEnabled(0);
    const bool disabled =
        background.base.base.enabled == 0 && background.backgroundSounds[0].playHandle == nullptr &&
        background.backgroundSounds[1].playHandle == nullptr && g_elementInvalidateCount == 2;

    g_HudUi_InvalidateMask = 0;
    return enabled && disabled ? 0 : 1;
}

extern "C" int zhud_dialog_controller_blit_owned_surface_smoke(void) {
    HudUiDialogController controller{};
    g_testBlitCount = 0;
    g_zVideo_pfnBltSourceToPrimary = TestBltSourceToPrimary;
    controller.BlitOwnedSurfaceToPrimary();
    const bool nullSkipped = g_testBlitCount == 0;

    zVidImagePartial image{};
    controller.capturedImage = &image;
    controller.BlitOwnedSurfaceToPrimary();
    g_zVideo_pfnBltSourceToPrimary = nullptr;

    const bool blitted = g_testBlitCount == 1 && g_testBlitImages[0] == &image &&
                         g_testBlitX[0] == 0 && g_testBlitY[0] == 0 && g_testBlitFlags[0] == 0 &&
                         g_testBlitHasRect[0] == 0;

    return nullSkipped && blitted ? 0 : 1;
}

extern "C" int zhud_font_style_constructor_smoke(void) {
    HudFontStyle style{};
    style.validMarker = 7;
    style.fontName = "Arial";
    style.fontSize = 12;
    style.textColor = 0x112233;
    style.bkColor = 0x445566;
    style.bkMode = 2;
    style.shadowEnabled = 1;
    style.fontWeight = 700;
    style.alignMode = 3;

    const bool constructed = style.Constructor() == &style && style.validMarker == 0 &&
                             style.fontName == nullptr && style.fontSize == 0 &&
                             style.textColor == 0 && style.bkColor == 0x445566 &&
                             style.bkMode == 2 && style.shadowEnabled == 0 &&
                             style.fontWeight == 0x1f4 && style.alignMode == 0;

    const char *const destructorFontName = "Tahoma";
    style.validMarker = 1;
    style.fontName = destructorFontName;
    style.Destructor();
    const bool destructed = style.validMarker == 0 && style.fontName == destructorFontName;

    return constructed && destructed ? 0 : 1;
}

extern "C" int zhud_numeric_text_input_base_constructor_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;
    g_zInput_KbdRawEventCallback = reinterpret_cast<void *>(0x1111);
    g_zInput_KbdRawEventCallbackCtx = reinterpret_cast<void *>(0x2222);

    HudUiNumericTextInput input{};
    input.BaseConstructor();

    const bool ok =
        input.base.base.ftable ==
            reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiNumericTextInput_Base_FTable) &&
        input.base.base.flags == 0x82 &&
        input.textInput.ftable == &g_HudUiNumericTextInput_TextInputFTable &&
        input.textInput.buffer != nullptr && input.textInput.capacity == 0x100 &&
        input.owner == &input &&
        input.sliderBorder.base.base.ftable ==
            reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiSliderBorder_FTable) &&
        input.sliderBorder.sliderVisibleWhenInputActive == 0 &&
        input.sliderBorder.rawKeyFilterEnabled == 0 && input.sliderBorder.inputActive == 1 &&
        input.sliderBorder.caretHalfWidth == 0 && input.sliderBorder.base.base.flags == 0x80;

    input.SetRawKeyboardCapture(0);
    const bool rawNoChange = g_zInput_KbdRawEventCallback == reinterpret_cast<void *>(0x1111) &&
                             g_zInput_KbdRawEventCallbackCtx == reinterpret_cast<void *>(0x2222);

    input.SetRawKeyboardCapture(1);
    const bool rawEnabled =
        input.sliderBorder.sliderVisibleWhenInputActive == 1 &&
        g_zInput_KbdRawEventCallback ==
            reinterpret_cast<void *>(&HudUiNumericTextInput::RawKeyboardCallback) &&
        g_zInput_KbdRawEventCallbackCtx == &input;

    input.SetRawKeyboardCapture(0);
    const bool rawDisabled = input.sliderBorder.sliderVisibleWhenInputActive == 0 &&
                             g_zInput_KbdRawEventCallback == nullptr &&
                             g_zInput_KbdRawEventCallbackCtx == nullptr;

    HudUiNumericTextInput_Base_FTable acceptTable{};
    acceptTable.slots[34] = reinterpret_cast<std::uintptr_t>(&TestNumericAccept);
    input.base.base.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&acceptTable);
    g_numericAcceptCount = 0;
    reinterpret_cast<HudUiOwnedTextInput *>(&input.textInput)->OnAcceptNotifyOwner();
    const bool acceptedNotify = g_numericAcceptCount == 1;

    HudUiWidget_FTable baseTable{};
    baseTable.slots[8] = reinterpret_cast<std::uintptr_t>(TestElementInvalidate);
    baseTable.slots[24] = MethodAddress(&HudUiElement::SetVisible);
    baseTable.slots[33] = MethodAddress(&HudUiNumericTextInput::OnRawKeyboardChar);
    HudUiTextInput_FTable textTable{};
    textTable.slots[0] = MethodAddress(&HudUiTextInput::InsertCharAtCursor);
    input.base.base.ftable = &baseTable;
    input.textInput.ftable = &textTable;
    input.textInput.buffer[0] = 0;
    input.textInput.cursor = 0;
    input.textInput.keyActionMap['5'] = 0;
    input.textInput.keyActionMap['x'] = 0;
    input.sliderBorder.rawKeyFilterEnabled = 0;
    const bool rawNull = HudUiNumericTextInput::RawKeyboardCallback('5', nullptr) == 0;
    const bool rawDispatched = HudUiNumericTextInput::RawKeyboardCallback('5', &input) == 0 &&
                               std::strcmp(input.textInput.buffer, "5") == 0;

    input.sliderBorder.rawKeyFilterEnabled = 1;
    HudUiNumericTextInput::RawKeyboardCallback('x', &input);
    const bool rawFiltered = std::strcmp(input.textInput.buffer, "5") == 0;
    HudUiNumericTextInput::RawKeyboardCallback('5', &input);
    const bool rawAccepted = std::strcmp(input.textInput.buffer, "55") == 0;

    HudUiElement labelPanel{};
    labelPanel.ftable = &g_HudUiCommon_FTable;
    HudUiPanel *labelPanelPtr = reinterpret_cast<HudUiPanel *>(&labelPanel);
    input.base.labelPanels.begin = &labelPanelPtr;
    input.base.labelPanels.end = &labelPanelPtr + 1;
    input.sliderBorder.inputActive = 1;
    input.base.base.flags = 0x82;
    input.sliderBorder.base.base.flags = 0x80;
    labelPanel.flags = 0;
    const std::int32_t previousActive = input.SetInputActive(0);
    const bool deactivated = previousActive == 1 && input.sliderBorder.inputActive == 0 &&
                             input.base.base.flags == 0x92 &&
                             input.sliderBorder.base.base.flags == 0x90 && labelPanel.flags == 0x90;

    const std::int32_t previousInactive = input.SetInputActive(1);
    const bool activated = previousInactive == 0 && input.sliderBorder.inputActive == 1 &&
                           input.base.base.flags == 0x82 &&
                           input.sliderBorder.base.base.flags == 0x80 && labelPanel.flags == 0x80;

    char *const oldBuffer = input.textInput.buffer;
    std::strcpy(oldBuffer, "42");
    input.AllocTextBuffer(8);
    const bool bufferAllocated =
        input.GetBuffer() == input.textInput.buffer && input.textInput.buffer != oldBuffer &&
        input.textInput.capacity == 8 && std::strncmp(input.textInput.buffer, "42", 2) == 0;
    ::operator delete(oldBuffer);

    HudUiPanel_FTable labelTextTable{};
    labelTextTable.slots[35] = MethodAddress(&TestNumericLabelPanel::SetText);
    TestNumericLabelPanel textLabel{&labelTextTable, nullptr};
    HudUiPanel *textLabelPtr = reinterpret_cast<HudUiPanel *>(&textLabel);
    input.base.labelPanels.begin = &textLabelPtr;
    input.base.labelPanels.end = &textLabelPtr + 1;
    input.base.base.flags = 0;
    input.Update("123");
    const bool updated = std::strcmp(input.textInput.buffer, "123") == 0 &&
                         input.textInput.cursor == 3 && textLabel.text == input.textInput.buffer &&
                         input.base.base.flags == 0x80;

    alignas(HudUiPanel) std::uint8_t measureLabelStorage[0x2ac]{};
    auto *const measureLabel = reinterpret_cast<HudUiPanel *>(measureLabelStorage);
    measureLabel->ConstructorDefault("X", 5, 7);
    TestFieldAt<std::uint32_t>(measureLabel, 0x14c) = 0x00332211;
    HudUiPanel *measureLabelPtr = measureLabel;
    input.base.labelPanels.begin = &measureLabelPtr;
    input.base.labelPanels.end = &measureLabelPtr + 1;
    std::strcpy(input.textInput.buffer, "1234");
    input.textInput.cursor = 2;
    input.base.base.flags = 0x82;
    input.sliderBorder.base.base.flags = 0x90;
    input.sliderBorder.sliderVisibleWhenInputActive = 1;
    input.sliderBorder.caretHalfWidth = 3;
    input.sliderBorder.blinkEnabled = 1;
    input.sliderBorder.blinkDirSign = -1;
    input.sliderBorder.blinkTimeRemainingSec = 10.0f;
    input.UpdateCaptureUiAndClip(0.05f);
    const bool captureUpdated =
        std::strcmp(&TestFieldAt<char>(measureLabel, 0x34), "1234") == 0 &&
        (TestFieldAt<std::uint32_t>(measureLabel, 0x0c) & 0x10) == 0 &&
        (input.sliderBorder.base.base.flags & 0x10) == 0 && input.sliderBorder.originX > 5 &&
        input.sliderBorder.originY == 7 && input.sliderBorder.halfWidth == 3 &&
        input.sliderBorder.height > 0 &&
        input.sliderBorder.base.color565 ==
            static_cast<std::int32_t>(zVid_PackColorRGB(0x11, 0x22, 0x33) & 0xffffu);

    input.base.base.flags = 0x92;
    reinterpret_cast<HudUiElement *>(measureLabel)->SetVisible(1);
    input.sliderBorder.base.base.flags = 0x80;
    input.UpdateCaptureUiAndClip(0.05f);
    const bool captureHidden = (TestFieldAt<std::uint32_t>(measureLabel, 0x0c) & 0x10) != 0 &&
                               (input.sliderBorder.base.base.flags & 0x10) != 0;

    DeleteObject(measureLabel->hFont);
    measureLabel->hFont = nullptr;

    HudUiNumericTextInput destructInput{};
    destructInput.BaseConstructor();
    destructInput.SetRawKeyboardCapture(1);
    HudUiNumericTextInput *const destructResult = destructInput.ScalarDeletingDestructor(0);
    const bool destructed = destructResult == &destructInput &&
                            g_zInput_KbdRawEventCallback == nullptr &&
                            g_zInput_KbdRawEventCallbackCtx == nullptr &&
                            destructInput.textInput.ftable == &g_HudUiTextInput_FTable &&
                            destructInput.sliderBorder.base.base.ftable == &g_HudUiCommon_FTable &&
                            destructInput.base.base.ftable ==
                                reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);

    ::operator delete(input.textInput.buffer);
    input.textInput.buffer = nullptr;
    g_HudUi_InvalidateMask = 0;
    return ok && rawNoChange && rawEnabled && rawDisabled && rawNull && rawDispatched &&
                   rawFiltered && rawAccepted && acceptedNotify && deactivated && activated &&
                   bufferAllocated && updated && captureUpdated && captureHidden && destructed
               ? 0
               : 1;
}

extern "C" int zhud_widget_release_and_destructor_core_smoke(void) {
    HudUiWidget borrowed{};
    borrowed.image = &zVid_Image::g_zImage_DefaultImage;
    borrowed.ownsImage = 0;
    borrowed.ReleaseImageIfOwned();
    const bool borrowedKept =
        borrowed.image == &zVid_Image::g_zImage_DefaultImage && borrowed.ownsImage == 0;

    HudUiWidget owned{};
    owned.image = &zVid_Image::g_zImage_DefaultImage;
    owned.ownsImage = 1;
    owned.ReleaseImageIfOwned();
    const bool ownedCleared = owned.image == nullptr && owned.ownsImage == 0;

    HudUiWidget borrowedImage{};
    borrowedImage.ftable = &g_HudUiWidget_FTable;
    g_HudUi_InvalidateMask = 0x80;
    borrowedImage.ownsImage = 1;
    borrowedImage.image = reinterpret_cast<zVidImagePartial *>(0x1111);
    zVidImagePartial *const borrowedImageResult =
        borrowedImage.SetImageBorrowedAndInvalidate(&zVid_Image::g_zImage_DefaultImage);
    const bool borrowedImageSet = borrowedImageResult == &zVid_Image::g_zImage_DefaultImage &&
                                  borrowedImage.image == &zVid_Image::g_zImage_DefaultImage &&
                                  borrowedImage.ownsImage == 0 && (borrowedImage.flags & 0x80) != 0;
    g_HudUi_InvalidateMask = 0;

    HudUiWidget nullPathImage{};
    nullPathImage.image = &zVid_Image::g_zImage_DefaultImage;
    nullPathImage.ownsImage = 1;
    g_HudUi_InvalidateMask = 0x80;
    zVidImagePartial *const nullPathResult = nullPathImage.SetImageByPathOwned(nullptr);
    const bool nullPathKept = nullPathResult == nullptr &&
                              nullPathImage.image == &zVid_Image::g_zImage_DefaultImage &&
                              nullPathImage.ownsImage == 1 && (nullPathImage.flags & 0x80) == 0;
    g_HudUi_InvalidateMask = 0;

    HudUiWidget missingPathImage{};
    missingPathImage.ftable = &g_HudUiWidget_FTable;
    missingPathImage.image = &zVid_Image::g_zImage_DefaultImage;
    missingPathImage.ownsImage = 1;
    g_HudUi_InvalidateMask = 0x80;
    zVidImagePartial *const missingPathResult =
        missingPathImage.SetImageByPathOwned("__recoil_missing_widget_image__");
    const bool missingPathReleased =
        missingPathResult == nullptr && missingPathImage.image == nullptr &&
        missingPathImage.ownsImage == 0 && (missingPathImage.flags & 0x80) != 0;
    g_HudUi_InvalidateMask = 0;

    HudUiWidget destructed{};
    destructed.ftable = reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCounter_FTable);
    destructed.image = &zVid_Image::g_zImage_DefaultImage;
    destructed.ownsImage = 1;
    destructed.DestructorCore();
    const bool destructorRestoredBase =
        TestFieldAt<const void *>(&destructed, 0x00) == &g_HudUiCommon_FTable &&
        destructed.image == nullptr && destructed.ownsImage == 0;

    HudUiWidget scalar{};
    scalar.ftable = &g_HudUiWidget_FTable;
    scalar.image = &zVid_Image::g_zImage_DefaultImage;
    scalar.ownsImage = 1;
    HudUiWidget *const scalarResult = scalar.ScalarDeletingDestructor(0);
    const bool scalarDestructed =
        scalarResult == &scalar &&
        TestFieldAt<const void *>(&scalar, 0x00) == &g_HudUiCommon_FTable &&
        scalar.image == nullptr && scalar.ownsImage == 0;

    return borrowedKept && ownedCleared && borrowedImageSet && nullPathKept && missingPathReleased &&
                   destructorRestoredBase && scalarDestructed
               ? 0
               : 1;
}

extern "C" int zhud_counter_constructor_smoke(void) {
    HudUiCounter counter{};
    zVidImagePartial image{};
    image.width = 11;
    image.height = 6;
    const bool ok = counter.Constructor() == &counter &&
                    TestFieldAt<const void *>(&counter, 0x00) == &g_HudUiCounter_FTable &&
                    TestFieldAt<std::int32_t>(&counter, 0xbc) == 0 &&
                    TestFieldAt<std::int32_t>(&counter, 0xc0) == 0 &&
                    TestFieldAt<std::int32_t>(&counter, 0xc4) == 0;

    g_HudUi_InvalidateMask = 0x80;
    g_HudUiMgrHudOriginX = 100;
    g_HudUiMgrHudOriginY = 200;
    TestFieldAt<zVidImagePartial *>(&counter, 0xbc) = &image;
    TestFieldAt<std::int32_t>(&counter, 0xd8) = 7;
    TestFieldAt<std::int32_t>(&counter, 0xdc) = 9;
    reinterpret_cast<HudUiElement *>(&counter)->flags = 0;
    counter.UpdateLayoutPosition();
    const bool layoutPosition = TestFieldAt<std::int32_t>(&counter, 0x14) == 107 &&
                                TestFieldAt<std::int32_t>(&counter, 0x18) == 209 &&
                                TestFieldAt<std::uint32_t>(&counter, 0x0c) == 0x80 &&
                                TestFieldAt<std::int32_t>(&counter, 0xc8) == 7 &&
                                TestFieldAt<std::int32_t>(&counter, 0xcc) == 9 &&
                                TestFieldAt<std::int32_t>(&counter, 0xd0) == 18 &&
                                TestFieldAt<std::int32_t>(&counter, 0xd4) == 15;

    zVidImagePartial previousArmed{};
    zVidImagePartial selectedIdle{};
    zVidImagePartial selectedActive{};
    zVidImagePartial otherIdle{};
    TestFieldAt<const HudUiCounter_FTable *>(&g_HudUiMgrModeCounters[1], 0x00) =
        &g_HudUiCounter_FTable;
    TestFieldAt<const HudUiCounter_FTable *>(&g_HudUiMgrModeCounters[2], 0x00) =
        &g_HudUiCounter_FTable;
    TestFieldAt<const HudUiCounter_FTable *>(&g_HudUiMgrModeCounters[3], 0x00) =
        &g_HudUiCounter_FTable;
    TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[1], 0xc0) = &previousArmed;
    TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[3], 0xbc) = &selectedIdle;
    TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[3], 0xc4) = &selectedActive;
    TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[2], 0xbc) = &otherIdle;
    g_HudUiMgrActiveModeCounterIndex = 1;
    HudUiMgr::SetModeCounterState(3, 2);
    const bool modeActive =
        g_HudUiMgrActiveModeCounterIndex == 3 &&
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[1], 0x3c) == &previousArmed &&
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[3], 0x3c) == &selectedActive;

    HudUiMgr::SetModeCounterState(2, 0);
    const bool modeIdle =
        g_HudUiMgrActiveModeCounterIndex == 3 &&
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[2], 0x3c) == &otherIdle;

    g_HudUi_InvalidateMask = 0;
    g_HudUiMgrHudOriginX = 0;
    g_HudUiMgrHudOriginY = 0;
    g_HudUiMgrModeCounters[1] = {};
    g_HudUiMgrModeCounters[2] = {};
    g_HudUiMgrModeCounters[3] = {};
    g_HudUiMgrActiveModeCounterIndex = 0;
    return ok && layoutPosition && modeActive && modeIdle ? 0 : 1;
}

extern "C" int zhud_counter_update_layout_position_smoke(void) {
    const int oldHudOriginX = g_HudUiMgrHudOriginX;
    const int oldHudOriginY = g_HudUiMgrHudOriginY;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;

    HudUiCounter counter{};
    TestFieldAt<const HudUiCounter_FTable *>(&counter, 0x00) = &g_HudUiCounter_FTable;
    zVidImagePartial image{};
    image.width = 13;
    image.height = 8;
    TestFieldAt<zVidImagePartial *>(&counter, 0xbc) = &image;
    TestFieldAt<std::int32_t>(&counter, 0xd8) = -4;
    TestFieldAt<std::int32_t>(&counter, 0xdc) = 9;
    TestFieldAt<std::uint32_t>(&counter, 0x0c) = 0;
    g_HudUiMgrHudOriginX = 100;
    g_HudUiMgrHudOriginY = 200;
    g_HudUi_InvalidateMask = 0x80;

    counter.UpdateLayoutPosition();

    const bool positioned =
        TestFieldAt<std::int32_t>(&counter, 0x14) == 96 &&
        TestFieldAt<std::int32_t>(&counter, 0x18) == 209 &&
        (TestFieldAt<std::uint32_t>(&counter, 0x0c) & 0x80) != 0;
    const bool clip =
        TestFieldAt<std::int32_t>(&counter, 0xc8) == -4 &&
        TestFieldAt<std::int32_t>(&counter, 0xcc) == 9 &&
        TestFieldAt<std::int32_t>(&counter, 0xd0) == 9 &&
        TestFieldAt<std::int32_t>(&counter, 0xd4) == 17;

    g_HudUiMgrHudOriginX = oldHudOriginX;
    g_HudUiMgrHudOriginY = oldHudOriginY;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    return positioned && clip ? 0 : 1;
}

extern "C" int zhud_counter_apply_from_layout_node_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char packPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "hct", 0, packPath) == 0) {
        return 1;
    }

    const char *const names[3] = {"counter0.tex", "counter1.tex", "counter2.tex"};
    const int widths[3] = {13, 2, 3};
    const int heights[3] = {8, 2, 3};
    zVidTexturePackHeader packHeader{};
    packHeader.fileFormat = 1;
    packHeader.recordCount = 3;
    zVidTexturePackRecord records[3] = {};
    int offset = sizeof(packHeader) + sizeof(records);
    for (int index = 0; index < 3; ++index) {
        std::strcpy(records[index].name, names[index]);
        records[index].fileOffset = offset;
        records[index].paletteIndex = -1;
        offset += 0x10 + widths[index] * heights[index] * static_cast<int>(sizeof(std::uint16_t));
    }

    FILE *out = std::fopen(packPath, "wb");
    if (out == nullptr) {
        DeleteFileA(packPath);
        return 2;
    }
    std::fwrite(&packHeader, sizeof(packHeader), 1, out);
    std::fwrite(records, sizeof(records), 1, out);
    for (int index = 0; index < 3; ++index) {
        unsigned char imageHeader[0x10] = {};
        imageHeader[0] = 1;
        *reinterpret_cast<std::int16_t *>(&imageHeader[4]) =
            static_cast<std::int16_t>(widths[index]);
        *reinterpret_cast<std::int16_t *>(&imageHeader[6]) =
            static_cast<std::int16_t>(heights[index]);
        std::fwrite(imageHeader, 1, sizeof(imageHeader), out);
        const std::uint16_t pixel = static_cast<std::uint16_t>(0x2000 + index);
        for (int pixelIndex = 0; pixelIndex < widths[index] * heights[index]; ++pixelIndex) {
            std::fwrite(&pixel, sizeof(pixel), 1, out);
        }
    }
    std::fclose(out);

    zVidTexturePackEntry entry{};
    std::strcpy(entry.filePath, packPath);
    zVidTexturePackEntry *const oldTexturePacks = g_zVid_TexturePacks;
    const int oldTexturePackCount = g_zVid_TexturePackCount;
    zVidTexturePackEntry *const oldBuiltinPacks = g_zVid_BuiltinTexturePacks;
    const int oldBuiltinPackCount = g_zVid_BuiltinTexturePackCount;
    const int oldTexturePackLoadState = g_zVid_TexturePackLoadState;
    const int oldPixelPackRBits = g_zVideo_PixelPack_RBits;
    HudUiContainer oldMgr = g_HudUiMgr;
    const int oldHudOriginX = g_HudUiMgrHudOriginX;
    const int oldHudOriginY = g_HudUiMgrHudOriginY;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;

    g_zVid_TexturePackLoadState = 1;
    g_zVideo_PixelPack_RBits = 0;
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    if (zVid_TexturePackEntry_LoadFromFile(&entry) == nullptr) {
        g_zVid_TexturePacks = oldTexturePacks;
        g_zVid_TexturePackCount = oldTexturePackCount;
        g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
        g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
        g_zVid_TexturePackLoadState = oldTexturePackLoadState;
        g_zVideo_PixelPack_RBits = oldPixelPackRBits;
        DeleteFileA(packPath);
        return 3;
    }
    g_zVid_TexturePacks = &entry;
    g_zVid_TexturePackCount = 1;

    HudUiCounter counter{};
    counter.Constructor();
    g_HudUiMgr.ConstructorDefault();
    g_HudUiMgrHudOriginX = 100;
    g_HudUiMgrHudOriginY = 200;
    g_HudUi_InvalidateMask = 0x80;

    zReader::Node payload[6] = {};
    for (int index = 0; index < 3; ++index) {
        payload[index + 1].type = zReader::ZRDR_NODE_STRING;
        payload[index + 1].value.str = const_cast<char *>(names[index]);
    }
    payload[4].value.i32 = -4;
    payload[5].value.i32 = 9;
    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = payload;

    const int result = counter.ApplyFromLayoutNode(&root);
    zReader::Node scalarNode{};
    scalarNode.type = zReader::ZRDR_NODE_INT;
    const int rejectedResult = counter.ApplyFromLayoutNode(&scalarNode);

    const bool imagesLoaded =
        TestFieldAt<zVidImagePartial *>(&counter, 0xbc) != nullptr &&
        TestFieldAt<zVidImagePartial *>(&counter, 0xbc)->width == 13 &&
        TestFieldAt<zVidImagePartial *>(&counter, 0xbc)->height == 8 &&
        TestFieldAt<zVidImagePartial *>(&counter, 0xc0) != nullptr &&
        TestFieldAt<zVidImagePartial *>(&counter, 0xc4) != nullptr;
    const bool layout =
        TestFieldAt<std::int32_t>(&counter, 0xd8) == -4 &&
        TestFieldAt<std::int32_t>(&counter, 0xdc) == 9 &&
        TestFieldAt<std::int32_t>(&counter, 0x14) == 96 &&
        TestFieldAt<std::int32_t>(&counter, 0x18) == 209 &&
        TestFieldAt<std::int32_t>(&counter, 0xc8) == -4 &&
        TestFieldAt<std::int32_t>(&counter, 0xcc) == 9 &&
        TestFieldAt<std::int32_t>(&counter, 0xd0) == 9 &&
        TestFieldAt<std::int32_t>(&counter, 0xd4) == 17;
    const bool defaultImage =
        TestFieldAt<zVidImagePartial *>(&counter, 0x3c) ==
            TestFieldAt<zVidImagePartial *>(&counter, 0xbc) &&
        TestFieldAt<std::int32_t>(&counter, 0x34) == 0 &&
        (TestFieldAt<std::uint32_t>(&counter, 0x0c) & 0x80) != 0;
    const bool child =
        g_HudUiMgr.childHead == reinterpret_cast<HudUiElement *>(&counter) &&
        g_HudUiMgr.childTail == reinterpret_cast<HudUiElement *>(&counter) &&
        reinterpret_cast<HudUiElement *>(&counter)->parent == &g_HudUiMgr;

    for (int index = 0; index < 3; ++index) {
        zVidImagePartial *const image =
            TestFieldAt<zVidImagePartial *>(&counter, 0xbc + index * 4);
        if (image != nullptr) {
            zVid_Image::Destroy(image);
            TestFieldAt<zVidImagePartial *>(&counter, 0xbc + index * 4) = nullptr;
        }
    }
    std::fclose(entry.fileHandle);
    std::free(entry.records);
    g_zVid_TexturePacks = oldTexturePacks;
    g_zVid_TexturePackCount = oldTexturePackCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
    g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
    g_zVid_TexturePackLoadState = oldTexturePackLoadState;
    g_zVideo_PixelPack_RBits = oldPixelPackRBits;
    g_HudUiMgr = oldMgr;
    g_HudUiMgrHudOriginX = oldHudOriginX;
    g_HudUiMgrHudOriginY = oldHudOriginY;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    DeleteFileA(packPath);

    return result == 1 && rejectedResult == 0 && imagesLoaded && layout && defaultImage && child
               ? 0
               : 1;
}

extern "C" int zhud_counter_release_state_images_smoke(void) {
    HudUiCounter counter{};
    TestFieldAt<zVidImagePartial *>(&counter, 0xbc) = nullptr;
    TestFieldAt<zVidImagePartial *>(&counter, 0xc0) = nullptr;
    TestFieldAt<zVidImagePartial *>(&counter, 0xc4) = nullptr;

    counter.ReleaseStateImages();

    return TestFieldAt<zVidImagePartial *>(&counter, 0xbc) == nullptr &&
                   TestFieldAt<zVidImagePartial *>(&counter, 0xc0) == nullptr &&
                   TestFieldAt<zVidImagePartial *>(&counter, 0xc4) == nullptr
               ? 0
               : 1;
}

extern "C" int zhud_message_release_images_smoke(void) {
    zVidImagePartial variant0{};
    zVidImagePartial variant1{};
    zVidImagePartial side0{};
    zVidImagePartial side1{};
    HudUiMessage &globalMessage = g_HudUiMgrMessages[2];
    globalMessage = {};
    globalMessage.base.ftable = &g_HudUiWidget_FTable;
    globalMessage.widget.ftable = &g_HudUiWidget_FTable;
    TestFieldAt<zVidImagePartial *>(&globalMessage, 0xbc) = &variant0;
    TestFieldAt<zVidImagePartial *>(&globalMessage, 0xc0) = &variant1;
    TestFieldAt<zVidImagePartial *>(&globalMessage, 0xd4) = &side0;
    TestFieldAt<zVidImagePartial *>(&globalMessage, 0xd8) = &side1;
    TestFieldAt<zVidImagePartial *>(&globalMessage, 0xdc) = &side0;

    g_HudUi_InvalidateMask = 0x80;
    HudUiMessage::SelectVariantDisplay(2, 0);
    const bool variantLeft = globalMessage.base.image == &variant0 &&
                             TestFieldAt<zVidImagePartial *>(&globalMessage, 0xd0) == &side1 &&
                             globalMessage.widget.image == &side0 &&
                             TestFieldAt<std::int32_t>(&globalMessage, 0x384) == 0;

    HudUiMessage::SelectVariantDisplay(2, 1);
    const bool variantRight = globalMessage.base.image == &variant1 &&
                              TestFieldAt<zVidImagePartial *>(&globalMessage, 0xd4) == &side0 &&
                              globalMessage.widget.image == &side1 &&
                              TestFieldAt<std::int32_t>(&globalMessage, 0x384) == 1;

    globalMessage.widget.flags = 0x93;
    HudUiMessage::ApplySideImageSwap(2, 1);
    const bool sideSwap = TestFieldAt<zVidImagePartial *>(&globalMessage, 0xd4) == &side0 &&
                          globalMessage.widget.image == &side0 &&
                          globalMessage.widget.flags == 0x10;

    reinterpret_cast<HudUiPanel *>(&globalMessage.panel)->SetText("abc");
    HudUiMessage::ClearDisplay(2);
    const bool clearedDisplay =
        globalMessage.base.image == nullptr && globalMessage.widget.image == nullptr &&
        std::strcmp(&TestFieldAt<char>(&globalMessage.panel, 0x34), "") == 0 &&
        (globalMessage.base.flags & 0x80) != 0;
    g_HudUi_InvalidateMask = 0;
    globalMessage = {};

    HudUiMessage message{};
    message.variantImages[0] = nullptr;
    message.variantImages[1] = &zVid_Image::g_zImage_DefaultImage;
    message.variantImages[2] = nullptr;
    message.variantImages[3] = &zVid_Image::g_zImage_DefaultImage;
    message.variantImages[4] = nullptr;
    message.sideImageSwaps[0] = &zVid_Image::g_zImage_DefaultImage;
    message.sideImageSwaps[1] = nullptr;
    message.unknown_0d0[0] = 0x5a;

    message.ReleaseImages();

    return message.variantImages[0] == nullptr && message.variantImages[1] == nullptr &&
                   message.variantImages[2] == nullptr && message.variantImages[3] == nullptr &&
                   message.variantImages[4] == nullptr && message.sideImageSwaps[0] == nullptr &&
                   message.sideImageSwaps[1] == nullptr && message.unknown_0d0[0] == 0x5a &&
                   variantLeft && variantRight && sideSwap && clearedDisplay
               ? 0
               : 1;
}

extern "C" int zhud_message_set_value_if_owner_matches_smoke(void) {
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    HudUiMessage &message = g_HudUiMgrMessages[3];
    message = {};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&message.panel);
    panel->SetText("unchanged");
    TestFieldAt<std::int32_t>(&message.panel, 0x2a4) = 2;
    g_HudUi_InvalidateMask = 0x80;

    HudUiMessage::SetValueIfOwnerMatches(3, 1, 4.25f);
    const bool skipped = std::strcmp(&TestFieldAt<char>(&message.panel, 0x34), "unchanged") == 0 &&
                         (message.base.flags & 0x80) == 0;

    HudUiMessage::SetValueIfOwnerMatches(3, 2, 4.25f);
    const bool rounded = std::strcmp(&TestFieldAt<char>(&message.panel, 0x34), "5") == 0 &&
                         (message.base.flags & 0x80) != 0;

    message.base.flags = 0;
    HudUiMessage::SetValueIfOwnerMatches(3, 2, 123456792.0f);
    const char *const text = &TestFieldAt<char>(&message.panel, 0x34);
    const bool special = text[0] == static_cast<char>(0xa5) && text[1] == '\0' &&
                         (message.base.flags & 0x80) == 0;

    g_HudUi_InvalidateMask = oldInvalidateMask;
    message = {};
    return skipped && rounded && special ? 0 : 1;
}

extern "C" int zhud_message_update_selected_weapon_display_smoke(void) {
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    zVidImagePartial prevBase{};
    zVidImagePartial prevSwap0{};
    zVidImagePartial prevRight{};
    zVidImagePartial selectedBase{};
    zVidImagePartial selectedSwap1{};
    zVidImagePartial selectedLeft{};

    HudUiMessage &previous = g_HudUiMgrMessages[2];
    HudUiMessage &selected = g_HudUiMgrMessages[3];
    previous = {};
    selected = {};
    previous.base.ftable = &g_HudUiWidget_FTable;
    previous.widget.ftable = &g_HudUiWidget_FTable;
    selected.base.ftable = &g_HudUiWidget_FTable;
    selected.widget.ftable = &g_HudUiWidget_FTable;
    previous.variantImages[0] = &prevBase;
    previous.sideImageSwaps[0] = &prevSwap0;
    TestFieldAt<zVidImagePartial *>(&previous, 0xd4) = &prevRight;
    selected.variantImages[4] = &selectedBase;
    selected.sideImageSwaps[1] = &selectedSwap1;
    TestFieldAt<zVidImagePartial *>(&selected, 0xd0) = &selectedLeft;
    TestFieldAt<std::int32_t>(&selected.panel, 0x2a4) = 1;
    g_HudUiMgrActiveWeaponMessageIndex = 2;
    g_HudUiMgrActiveWeaponSideIndex = 0;
    g_HudUi_InvalidateMask = 0x80;

    HudUiMessage::UpdateSelectedWeaponDisplay(3, 1, 4.25f);
    if (previous.base.image != &prevBase ||
        TestFieldAt<zVidImagePartial *>(&previous, 0xd0) != &prevSwap0 ||
        previous.widget.image != &prevRight ||
        TestFieldAt<std::int32_t>(&previous.panel, 0x2a4) != 0) {
        return 1;
    }
    if (selected.base.image != &selectedBase ||
        TestFieldAt<zVidImagePartial *>(&selected, 0xd4) != &selectedSwap1 ||
        selected.widget.image != &selectedLeft ||
        TestFieldAt<std::int32_t>(&selected.panel, 0x2a4) != 1) {
        return 2;
    }
    if (g_HudUiMgrActiveWeaponMessageIndex != 3 || g_HudUiMgrActiveWeaponSideIndex != 1) {
        return 3;
    }
    if (std::strcmp(&TestFieldAt<char>(&selected.panel, 0x34), "5") != 0 ||
        (selected.base.flags & 0x80) == 0) {
        return 4;
    }

    selected.base.flags = 0;
    HudUiMessage::UpdateSelectedWeaponDisplay(3, 1, 123456792.0f);
    const char *const specialText = &TestFieldAt<char>(&selected.panel, 0x34);
    if (specialText[0] != static_cast<char>(0xa5) || specialText[1] != '\0') {
        return 5;
    }

    HudUiMessage::UpdateSelectedWeaponDisplay(0, 0, 0.0f);
    if (g_HudUiMgrActiveWeaponMessageIndex != 0 || g_HudUiMgrActiveWeaponSideIndex != 0) {
        return 6;
    }

    g_HudUi_InvalidateMask = oldInvalidateMask;
    previous = {};
    selected = {};
    return 0;
}

extern "C" int zhud_message_constructor_smoke(void) {
    HudUiMessage message{};
    message.variantImages[0] = &zVid_Image::g_zImage_DefaultImage;
    message.sideImageSwaps[1] = &zVid_Image::g_zImage_DefaultImage;
    TestFieldAt<std::int32_t>(&message.panel, 0x2a4) = 7;

    HudUiMessage *const result = message.Constructor();

    const bool ok = result == &message &&
                    TestFieldAt<const void *>(&message.base, 0x00) == &g_HudUiMessage_FTable &&
                    message.variantImages[0] == nullptr && message.variantImages[4] == nullptr &&
                    message.sideImageSwaps[0] == nullptr && message.sideImageSwaps[1] == nullptr &&
                    TestFieldAt<std::int32_t>(&message.unknown_0d0[0], 0) == 0 &&
                    TestFieldAt<std::int32_t>(&message.unknown_0d0[4], 0) == 0 &&
                    TestFieldAt<std::int32_t>(&message.panel, 0x2a4) == 0 &&
                    message.widget.ftable == &g_HudUiWidget_FTable;

    message.Destructor();
    return ok ? 0 : 1;
}

extern "C" int zhud_message_rebuild_weapon_layout_smoke(void) {
    const int oldHudOriginX = g_HudUiMgrHudOriginX;
    HudUiWidget *const layoutWidget2 = &TestFieldAt<HudUiWidget>(&g_HudLayoutHW, 0x1b4);
    const HudUiWidget_FTable *const oldLayoutWidget2FTable = layoutWidget2->ftable;

    HudUiWidget_FTable layoutWidgetTable{};
    layoutWidgetTable.slots[0x64 / 4] = MethodAddress(&TestShieldApplyLayoutOps::GetX);
    layoutWidgetTable.slots[0x68 / 4] = MethodAddress(&TestShieldApplyLayoutOps::GetY);

    HudUiWidget_FTable messageWidgetTable{};
    messageWidgetTable.slots[0x0c / 4] = MethodAddress(&TestApplyTextLabelPanel::SetPos);
    messageWidgetTable.slots[0x18 / 4] = MethodAddress(&TestShieldApplyLayoutOps::SetClip);

    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x0c / 4] = MethodAddress(&TestApplyTextLabelPanel::SetPos);
    panelTable.slots[0x18 / 4] = MethodAddress(&TestShieldApplyLayoutOps::SetClip);

    zVidImagePartial baseImage{};
    baseImage.width = 30;
    baseImage.height = 12;
    zVidImagePartial sideImage{};
    sideImage.width = 5;
    sideImage.height = 6;

    HudUiMessage message{};
    message.base.ftable = &messageWidgetTable;
    reinterpret_cast<HudUiPanel *>(&message.panel)->vtbl = &panelTable;
    message.widget.ftable = &messageWidgetTable;
    message.variantImages[0] = &baseImage;
    message.sideImageSwaps[0] = &sideImage;
    message.panel.layoutX = 7;
    message.panel.layoutY = 20;

    layoutWidget2->ftable = &layoutWidgetTable;
    g_HudUiMgrHudOriginX = 101;
    g_shieldApplyWidgetThis = layoutWidget2;
    g_shieldApplyWidgetCenterX = 300;
    g_shieldApplyWidgetCenterY = 400;
    g_shieldApplyGetXCount = 0;
    g_shieldApplyGetYCount = 0;
    g_shieldApplySetClipCount = 0;
    g_applyTextLabelSetPosCount = 0;
    for (int index = 0; index < 4; ++index) {
        g_applyTextLabelSetPosThis[index] = nullptr;
        g_applyTextLabelSetPosX[index] = 0;
        g_applyTextLabelSetPosY[index] = 0;
        g_shieldApplySetClipThis[index] = nullptr;
        g_shieldApplySetClipBltSource[index] = nullptr;
        g_shieldApplySetClipRect[index] = {};
    }

    message.RebuildWeaponLayout();

    const bool anchorsRead = g_shieldApplyGetXCount == 1 && g_shieldApplyGetYCount == 1;
    const bool baseSet =
        g_applyTextLabelSetPosCount == 3 &&
        g_applyTextLabelSetPosThis[0] == &message &&
        g_applyTextLabelSetPosX[0] == 357 &&
        g_applyTextLabelSetPosY[0] == 420 &&
        g_shieldApplySetClipCount == 2 &&
        g_shieldApplySetClipThis[0] == &message &&
        g_shieldApplySetClipBltSource[0] == nullptr &&
        g_shieldApplySetClipRect[0].left == 57 &&
        g_shieldApplySetClipRect[0].top == 20 &&
        g_shieldApplySetClipRect[0].right == 87 &&
        g_shieldApplySetClipRect[0].bottom == 32;

    const bool panelSet =
        g_applyTextLabelSetPosThis[1] == reinterpret_cast<HudUiPanel *>(&message.panel) &&
        g_applyTextLabelSetPosX[1] == 372 &&
        g_applyTextLabelSetPosY[1] == 432 &&
        g_shieldApplySetClipThis[1] == reinterpret_cast<HudUiPanel *>(&message.panel) &&
        g_shieldApplySetClipBltSource[1] == nullptr &&
        g_shieldApplySetClipRect[1].left == 60 &&
        g_shieldApplySetClipRect[1].top == 32 &&
        g_shieldApplySetClipRect[1].right == 85 &&
        g_shieldApplySetClipRect[1].bottom == 44;

    const bool sideWidgetSet =
        g_applyTextLabelSetPosThis[2] == &message.widget &&
        g_applyTextLabelSetPosX[2] == 381 &&
        g_applyTextLabelSetPosY[2] == 425;

    layoutWidget2->ftable = oldLayoutWidget2FTable;
    g_HudUiMgrHudOriginX = oldHudOriginX;
    return anchorsRead && baseSet && panelSet && sideWidgetSet ? 0 : 1;
}

extern "C" int zhud_message_load_weapon_layout_from_node_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char packPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "hwm", 0, packPath) == 0) {
        return 1;
    }

    const char *const names[7] = {
        "msg0.tex", "msg1.tex", "msg2.tex", "msg3.tex",
        "msg4.tex", "side0.tex", "side1.tex"};
    const int widths[7] = {30, 1, 1, 1, 1, 5, 2};
    const int heights[7] = {12, 1, 1, 1, 1, 6, 2};

    zVidTexturePackHeader packHeader{};
    packHeader.fileFormat = 1;
    packHeader.recordCount = 7;
    zVidTexturePackRecord records[7] = {};
    int offset = sizeof(packHeader) + sizeof(records);
    for (int index = 0; index < 7; ++index) {
        std::strcpy(records[index].name, names[index]);
        records[index].fileOffset = offset;
        records[index].paletteIndex = -1;
        offset += 0x10 + widths[index] * heights[index] * static_cast<int>(sizeof(std::uint16_t));
    }

    FILE *out = std::fopen(packPath, "wb");
    if (out == nullptr) {
        DeleteFileA(packPath);
        return 2;
    }

    std::fwrite(&packHeader, sizeof(packHeader), 1, out);
    std::fwrite(records, sizeof(records), 1, out);
    for (int index = 0; index < 7; ++index) {
        unsigned char imageHeader[0x10] = {};
        imageHeader[0] = 1;
        *reinterpret_cast<std::int16_t *>(&imageHeader[4]) =
            static_cast<std::int16_t>(widths[index]);
        *reinterpret_cast<std::int16_t *>(&imageHeader[6]) =
            static_cast<std::int16_t>(heights[index]);
        std::fwrite(imageHeader, 1, sizeof(imageHeader), out);
        const std::uint16_t pixel = static_cast<std::uint16_t>(0x1000 + index);
        for (int pixelIndex = 0; pixelIndex < widths[index] * heights[index]; ++pixelIndex) {
            std::fwrite(&pixel, sizeof(pixel), 1, out);
        }
    }
    std::fclose(out);

    zVidTexturePackEntry entry{};
    std::strcpy(entry.filePath, packPath);
    zVidTexturePackEntry *const oldTexturePacks = g_zVid_TexturePacks;
    const int oldTexturePackCount = g_zVid_TexturePackCount;
    zVidTexturePackEntry *const oldBuiltinPacks = g_zVid_BuiltinTexturePacks;
    const int oldBuiltinPackCount = g_zVid_BuiltinTexturePackCount;
    const int oldTexturePackLoadState = g_zVid_TexturePackLoadState;
    const int oldPixelPackRBits = g_zVideo_PixelPack_RBits;
    HudUiContainer oldMgr = g_HudUiMgr;
    const int oldHudOriginX = g_HudUiMgrHudOriginX;
    HudUiWidget *const layoutWidget2 = &TestFieldAt<HudUiWidget>(&g_HudLayoutHW, 0x1b4);
    const HudUiWidget_FTable *const oldLayoutWidget2FTable = layoutWidget2->ftable;

    g_zVid_TexturePackLoadState = 1;
    g_zVideo_PixelPack_RBits = 0;
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    if (zVid_TexturePackEntry_LoadFromFile(&entry) == nullptr) {
        g_zVid_TexturePacks = oldTexturePacks;
        g_zVid_TexturePackCount = oldTexturePackCount;
        g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
        g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
        g_zVid_TexturePackLoadState = oldTexturePackLoadState;
        g_zVideo_PixelPack_RBits = oldPixelPackRBits;
        DeleteFileA(packPath);
        return 3;
    }

    g_zVid_TexturePacks = &entry;
    g_zVid_TexturePackCount = 1;

    HudUiWidget_FTable layoutWidgetTable{};
    layoutWidgetTable.slots[0x64 / 4] = MethodAddress(&TestShieldApplyLayoutOps::GetX);
    layoutWidgetTable.slots[0x68 / 4] = MethodAddress(&TestShieldApplyLayoutOps::GetY);
    HudUiWidget_FTable messageWidgetTable{};
    messageWidgetTable.slots[0x0c / 4] = MethodAddress(&TestApplyTextLabelPanel::SetPos);
    messageWidgetTable.slots[0x18 / 4] = MethodAddress(&TestShieldApplyLayoutOps::SetClip);
    messageWidgetTable.slots[0x20 / 4] = MethodAddress(&TestShieldApplyLayoutOps::Invalidate);
    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x0c / 4] = MethodAddress(&TestApplyTextLabelPanel::SetPos);
    panelTable.slots[0x18 / 4] = MethodAddress(&TestShieldApplyLayoutOps::SetClip);
    panelTable.slots[0x74 / 4] = reinterpret_cast<std::uintptr_t>(&TestApplyTextLabelSetTextFmt);
    panelTable.slots[0x80 / 4] = MethodAddress(&TestTextStackFontPanel::SetFont);

    HudUiMessage message{};
    message.base.ftable = &messageWidgetTable;
    message.base.imageStateWord = 0xabcd1234;
    reinterpret_cast<HudUiPanel *>(&message.panel)->vtbl = &panelTable;
    message.widget.ftable = &messageWidgetTable;
    g_HudUiMgr.ConstructorDefault();
    g_HudUiMgrHudOriginX = 101;
    layoutWidget2->ftable = &layoutWidgetTable;
    g_shieldApplyWidgetThis = layoutWidget2;
    g_shieldApplyWidgetCenterX = 300;
    g_shieldApplyWidgetCenterY = 400;
    g_shieldApplyInvalidateCount = 0;
    g_textStackSetFontCount = 0;
    g_applyTextLabelSetTextFmtCount = 0;
    g_applyTextLabelSetPosCount = 0;
    g_shieldApplySetClipCount = 0;

    zReader::Node payload[10] = {};
    for (int index = 0; index < 7; ++index) {
        payload[index + 1].type = zReader::ZRDR_NODE_STRING;
        payload[index + 1].value.str = const_cast<char *>(names[index]);
    }
    payload[8].value.i32 = 7;
    payload[9].value.i32 = 20;
    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = payload;
    HudUiPanelFontParams fontParams = {"Arial", 11, 600, 2};

    const int result = message.LoadWeaponLayoutFromNode(&root, &fontParams);
    zReader::Node scalarNode{};
    scalarNode.type = zReader::ZRDR_NODE_INT;
    const int rejectedResult = message.LoadWeaponLayoutFromNode(&scalarNode, &fontParams);

    const auto *const panel = reinterpret_cast<const HudUiPanel *>(&message.panel);
    const bool imagesLoaded =
        message.variantImages[0] != nullptr && message.variantImages[0]->width == 30 &&
        message.variantImages[0]->height == 12 && message.variantImages[4] != nullptr &&
        message.sideImageSwaps[0] != nullptr && message.sideImageSwaps[0]->width == 5 &&
        message.sideImageSwaps[0]->height == 6 && message.sideImageSwaps[1] != nullptr;
    const bool layoutAndInvalidation =
        message.panel.layoutX == 7 && message.panel.layoutY == 20 &&
        message.base.imageStateWord == 0xabcd0001 && g_shieldApplyInvalidateCount == 1;
    const bool panelStyle =
        TestFieldAt<std::uint32_t>(&message.panel, 0x144) == 1 &&
        TestFieldAt<std::uint32_t>(&message.panel, 0x14c) == 0x0020bf40 &&
        TestFieldAt<std::uint32_t>(&message.panel, 0x150) == 0x0020bf40 &&
        TestFieldAt<std::uint32_t>(&message.panel, 0x270) == 1 &&
        TestFieldAt<std::int32_t>(&message.panel, 0x29c) == -1 &&
        TestFieldAt<std::int32_t>(&message.panel, 0x2a0) == -1 &&
        TestFieldAt<std::uint32_t>(&message.panel, 0x264) == 1;
    const bool fontAndText =
        g_textStackSetFontCount == 1 && g_textStackSetFontThis[0] == panel &&
        std::strcmp(g_textStackSetFontFace[0], "Arial") == 0 &&
        g_textStackSetFontHeight[0] == 11 && g_textStackSetFontWeight[0] == 600 &&
        g_textStackSetFontWidth[0] == 2 && g_textStackSetFontItalic[0] == 0 &&
        g_textStackSetFontCharSet[0] == 0 && g_textStackSetFontPitch[0] == 2 &&
        g_applyTextLabelSetTextFmtCount == 1 &&
        g_applyTextLabelSetTextFmtThis[0] == panel &&
        std::strcmp(g_applyTextLabelSetTextFmtFormat[0], "   ") == 0;
    const bool children =
        g_HudUiMgr.childHead == reinterpret_cast<HudUiElement *>(&message) &&
        reinterpret_cast<HudUiElement *>(&message)->next ==
            reinterpret_cast<HudUiElement *>(&message.widget) &&
        g_HudUiMgr.childTail == reinterpret_cast<HudUiElement *>(&message.widget) &&
        reinterpret_cast<HudUiElement *>(&message)->parent == &g_HudUiMgr &&
        reinterpret_cast<HudUiElement *>(&message.widget)->parent == &g_HudUiMgr;

    for (int index = 0; index < 5; ++index) {
        if (message.variantImages[index] != nullptr) {
            zVid_Image::Destroy(message.variantImages[index]);
            message.variantImages[index] = nullptr;
        }
    }
    for (int index = 0; index < 2; ++index) {
        if (message.sideImageSwaps[index] != nullptr) {
            zVid_Image::Destroy(message.sideImageSwaps[index]);
            message.sideImageSwaps[index] = nullptr;
        }
    }
    std::fclose(entry.fileHandle);
    std::free(entry.records);
    g_zVid_TexturePacks = oldTexturePacks;
    g_zVid_TexturePackCount = oldTexturePackCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
    g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
    g_zVid_TexturePackLoadState = oldTexturePackLoadState;
    g_zVideo_PixelPack_RBits = oldPixelPackRBits;
    g_HudUiMgr = oldMgr;
    g_HudUiMgrHudOriginX = oldHudOriginX;
    layoutWidget2->ftable = oldLayoutWidget2FTable;
    DeleteFileA(packPath);

    return result == 1 && rejectedResult == 0 && imagesLoaded && layoutAndInvalidation &&
                   panelStyle && fontAndText && children
               ? 0
               : 1;
}

extern "C" int zhud_message_destructors_smoke(void) {
    HudUiMessage message{};
    message.base.ftable = &g_HudUiWidget_FTable;
    message.base.image = &zVid_Image::g_zImage_DefaultImage;
    message.base.ownsImage = 1;
    message.widget.ftable = &g_HudUiWidget_FTable;
    message.widget.image = &zVid_Image::g_zImage_DefaultImage;
    message.widget.ownsImage = 1;
    auto *const panel = reinterpret_cast<HudUiPanel *>(&message.panel);
    panel->vtbl = &g_HudUiPanel_FTable;
    panel->textPick = nullptr;
    panel->hFont = nullptr;

    message.Destructor();

    const bool destructorOk =
        TestFieldAt<const void *>(&message.base, 0x00) == &g_HudUiCommon_FTable &&
        TestFieldAt<const void *>(&message.widget, 0x00) == &g_HudUiCommon_FTable &&
        panel->vtbl == &g_HudUiCommon_FTable && message.base.image == nullptr &&
        message.widget.image == nullptr && message.base.ownsImage == 0 &&
        message.widget.ownsImage == 0;

    HudUiMessage scalar{};
    scalar.base.image = &zVid_Image::g_zImage_DefaultImage;
    scalar.base.ownsImage = 1;
    scalar.widget.image = &zVid_Image::g_zImage_DefaultImage;
    scalar.widget.ownsImage = 1;
    auto *const scalarPanel = reinterpret_cast<HudUiPanel *>(&scalar.panel);
    scalarPanel->textPick = nullptr;
    scalarPanel->hFont = nullptr;
    HudUiMessage *const scalarResult = scalar.ScalarDeletingDestructor(0);

    const bool scalarOk = scalarResult == &scalar &&
                          TestFieldAt<const void *>(&scalar.base, 0x00) == &g_HudUiCommon_FTable &&
                          TestFieldAt<const void *>(&scalar.widget, 0x00) == &g_HudUiCommon_FTable;

    return destructorOk && scalarOk ? 0 : 1;
}

extern "C" int zhud_shield_message_widget_destructor_smoke(void) {
    HudUiShieldMessageWidget shield{};
    shield.widget.ftable = &g_HudUiWidget_FTable;
    shield.widget.image = &zVid_Image::g_zImage_DefaultImage;
    shield.widget.ownsImage = 1;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl = &g_HudUiPanelSimple_FTable;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->textPick = nullptr;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->hFont = nullptr;
    shield.meter.ftable = &g_HudUiMeter_FTable;

    shield.Destructor();

    return TestFieldAt<const void *>(&shield.meter, 0x00) == &g_HudUiCommon_FTable &&
                   TestFieldAt<const void *>(&shield.percentTextPanel, 0x00) ==
                       &g_HudUiCommon_FTable &&
                   TestFieldAt<const void *>(&shield.widget, 0x00) == &g_HudUiCommon_FTable &&
                   shield.widget.image == nullptr && shield.widget.ownsImage == 0
               ? 0
               : 1;
}

extern "C" int zhud_mgr_sensor_set_shield_message_ratio_smoke(void) {
    HudUiShieldMessageWidget shield{};
    g_HudUiMgrShieldMessageWidget = &shield;
    shield.meter.ftable = &g_HudUiMeter_FTable;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl =
        &g_HudUiPanelSimple_FTable;

    g_HudUi_InvalidateMask = 0x80;
    HudUiMgrSensor::SetShieldMessageRatio(0.125f);
    if (shield.meter.color565 != (zVid_PackColorRGB(255, 0, 0) & 0xffffu)) {
        return 1;
    }
    if (shield.meter.points[0].y != 97.0f || shield.meter.points[3].y != 97.0f) {
        return 2;
    }
    if ((shield.meter.flags & 0x80) == 0) {
        return 3;
    }
    if (std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "13") != 0) {
        return 4;
    }

    shield.meter.flags = 0;
    reinterpret_cast<HudUiElement *>(&shield.percentTextPanel)->flags = 0;
    HudUiMgrSensor::SetShieldMessageRatio(1.50f);
    if (shield.meter.color565 != (zVid_PackColorRGB(255, 255, 0) & 0xffffu)) {
        return 5;
    }
    if (shield.meter.points[0].y != 80.0f || shield.meter.points[3].y != 80.0f) {
        return 6;
    }
    if ((shield.meter.flags & 0x80) == 0) {
        return 7;
    }
    if (std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "100") != 0) {
        return 8;
    }

    shield.meter.flags = 0;
    HudUiMgrSensor::SetShieldMessageRatio(-0.50f);
    if (shield.meter.color565 != (zVid_PackColorRGB(255, 0, 0) & 0xffffu)) {
        return 9;
    }
    if (shield.meter.points[0].y != 100.0f || shield.meter.points[3].y != 100.0f) {
        return 10;
    }
    if (std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "0") != 0) {
        return 11;
    }

    g_HudUi_InvalidateMask = 0;
    g_HudUiMgrShieldMessageWidget = nullptr;
    return 0;
}

extern "C" int zhud_shield_message_widget_apply_layout_smoke(void) {
    HudUiContainer oldMgr = g_HudUiMgr;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    const int oldHudOriginX = g_HudUiMgrHudOriginX;
    const int oldHudOriginY = g_HudUiMgrHudOriginY;

    HudUiWidget_FTable widgetTable{};
    widgetTable.slots[0x64 / 4] = MethodAddress(&TestShieldApplyLayoutOps::GetX);
    widgetTable.slots[0x68 / 4] = MethodAddress(&TestShieldApplyLayoutOps::GetY);
    widgetTable.slots[8] = MethodAddress(&TestShieldApplyLayoutOps::Invalidate);

    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x0c / 4] = MethodAddress(&TestApplyTextLabelPanel::SetPos);
    panelTable.slots[0x18 / 4] = MethodAddress(&TestShieldApplyLayoutOps::SetClip);
    panelTable.slots[0x64 / 4] = MethodAddress(&TestShieldApplyLayoutOps::GetX);
    panelTable.slots[0x68 / 4] = MethodAddress(&TestShieldApplyLayoutOps::GetY);
    panelTable.slots[0x74 / 4] = reinterpret_cast<std::uintptr_t>(&TestApplyTextLabelSetTextFmt);
    panelTable.slots[0x78 / 4] =
        MethodAddress(&TestShieldApplyLayoutOps::UpdateTextBoundsFromContent);

    HudUiMeter_FTable meterTable{};
    meterTable.slots[3] = MethodAddress(&TestApplyTextLabelPanel::SetPos);
    meterTable.slots[0x18 / 4] = MethodAddress(&TestShieldApplyLayoutOps::SetClip);
    meterTable.slots[8] = MethodAddress(&TestShieldApplyLayoutOps::Invalidate);

    HudUiShieldMessageWidget shield{};
    zVidImagePartial image{};
    shield.widget.ftable = &widgetTable;
    shield.widget.image = &image;
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->vtbl = &panelTable;
    shield.meter.ftable = &meterTable;

    g_HudUiMgr.ConstructorDefault();
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiMgrHudOriginX = 100;
    g_HudUiMgrHudOriginY = 200;
    g_shieldApplyWidgetThis = &shield.widget;
    g_shieldApplyPanelThis = &shield.percentTextPanel;
    g_shieldApplyWidgetCenterX = 50;
    g_shieldApplyWidgetCenterY = 60;
    g_shieldApplyPanelX = 80;
    g_shieldApplyPanelY = 95;
    g_shieldApplyGetXCount = 0;
    g_shieldApplyGetYCount = 0;
    g_shieldApplySetClipCount = 0;
    g_shieldApplyUpdateBoundsCount = 0;
    g_shieldApplyUpdateBoundsThis = nullptr;
    g_shieldApplyInvalidateCount = 0;
    g_applyTextLabelSetPosCount = 0;
    g_applyTextLabelSetTextFmtCount = 0;
    for (int index = 0; index < 4; ++index) {
        g_applyTextLabelSetPosThis[index] = nullptr;
        g_applyTextLabelSetPosX[index] = 0;
        g_applyTextLabelSetPosY[index] = 0;
        g_applyTextLabelSetTextFmtThis[index] = nullptr;
        g_applyTextLabelSetTextFmtFormat[index] = nullptr;
        g_shieldApplySetClipThis[index] = nullptr;
        g_shieldApplySetClipBltSource[index] = nullptr;
        g_shieldApplySetClipRect[index] = {};
    }

    zReader::Node imageItems[4] = {};
    imageItems[1].type = zReader::ZRDR_NODE_STRING;
    imageItems[1].value.str = nullptr;

    zReader::Node textItems[4] = {};
    textItems[1].type = zReader::ZRDR_NODE_STRING;
    textItems[1].value.str = const_cast<char *>("%d%%");
    textItems[2].value.i32 = 3;
    textItems[3].value.i32 = 4;

    zReader::Node meterItems[5] = {};
    meterItems[1].value.i32 = 10;
    meterItems[2].value.i32 = 20;
    meterItems[3].value.i32 = 30;
    meterItems[4].value.i32 = 40;

    zReader::Node layoutItems[4] = {};
    layoutItems[1].type = zReader::ZRDR_NODE_ARRAY;
    layoutItems[1].value.nodes = imageItems;
    layoutItems[2].type = zReader::ZRDR_NODE_ARRAY;
    layoutItems[2].value.nodes = textItems;
    layoutItems[3].type = zReader::ZRDR_NODE_ARRAY;
    layoutItems[3].value.nodes = meterItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = layoutItems;

    const int result = HudUiShieldMessageWidget::ApplyLayout(&root);

    const auto *const panel = reinterpret_cast<const HudUiPanel *>(&shield.percentTextPanel);
    const bool textApplied =
        g_applyTextLabelSetPosCount >= 1 && g_applyTextLabelSetPosThis[0] == panel &&
        g_applyTextLabelSetPosX[0] == 53 && g_applyTextLabelSetPosY[0] == 64 &&
        g_applyTextLabelSetTextFmtCount == 2 &&
        g_applyTextLabelSetTextFmtThis[0] == panel &&
        std::strcmp(g_applyTextLabelSetTextFmtFormat[0], "%d%%") == 0 &&
        g_applyTextLabelSetTextFmtThis[1] == panel &&
        std::strcmp(g_applyTextLabelSetTextFmtFormat[1], "000") == 0 &&
        g_shieldApplyUpdateBoundsCount == 1 && g_shieldApplyUpdateBoundsThis == panel;

    const bool panelClip =
        g_shieldApplySetClipCount >= 1 && g_shieldApplySetClipThis[0] == panel &&
        g_shieldApplySetClipBltSource[0] == &image &&
        g_shieldApplySetClipRect[0].left == 30 && g_shieldApplySetClipRect[0].top == 35;

    const bool meterApplied =
        shield.meter.points[0].x == 60.0f && shield.meter.points[0].y == 80.0f &&
        shield.meter.points[1].x == 60.0f && shield.meter.points[1].y == 101.0f &&
        shield.meter.points[2].x == 82.0f && shield.meter.points[2].y == 101.0f &&
        shield.meter.points[3].x == 82.0f && shield.meter.points[3].y == 80.0f &&
        shield.meter.fillPixelsMax == 21 && shield.meter.meterFlags == 21;

    const bool meterClip =
        g_shieldApplySetClipCount >= 2 && g_shieldApplySetClipThis[1] == &shield.meter &&
        g_shieldApplySetClipBltSource[1] == &image &&
        g_shieldApplySetClipRect[1].left == 10 &&
        g_shieldApplySetClipRect[1].top == 20 &&
        g_shieldApplySetClipRect[1].right == 31 &&
        g_shieldApplySetClipRect[1].bottom == 41;

    const bool children =
        g_HudUiMgr.childHead == reinterpret_cast<HudUiElement *>(&shield.widget) &&
        reinterpret_cast<HudUiElement *>(&shield.widget)->next ==
            reinterpret_cast<HudUiElement *>(&shield.percentTextPanel) &&
        reinterpret_cast<HudUiElement *>(&shield.percentTextPanel)->next ==
            reinterpret_cast<HudUiElement *>(&shield.meter) &&
        g_HudUiMgr.childTail == reinterpret_cast<HudUiElement *>(&shield.meter) &&
        reinterpret_cast<HudUiElement *>(&shield.widget)->parent == &g_HudUiMgr &&
        reinterpret_cast<HudUiElement *>(&shield.percentTextPanel)->parent == &g_HudUiMgr &&
        reinterpret_cast<HudUiElement *>(&shield.meter)->parent == &g_HudUiMgr;

    g_HudUiMgr = oldMgr;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiMgrHudOriginX = oldHudOriginX;
    g_HudUiMgrHudOriginY = oldHudOriginY;

    return result == 1 && textApplied && panelClip && meterApplied && meterClip && children ? 0
                                                                                            : 1;
}

extern "C" int zhud_triplet_panel_constructor_smoke(void) {
    g_HudUiMgr = {};
    g_HudUiMgr.ConstructorDefault();

    HudUiTripletPanel panel{};
    const HudUiTripletPanel *const result = panel.Constructor();

    bool itemsHidden = true;
    for (const HudUiWidget &item : panel.items) {
        itemsHidden =
            itemsHidden && item.ftable == &g_HudUiWidget_FTable && (item.flags & 0x10) != 0;
    }

    const bool linked = g_HudUiMgr.childHead == reinterpret_cast<HudUiElement *>(&panel) &&
                        g_HudUiMgr.childTail == reinterpret_cast<HudUiElement *>(&panel) &&
                        panel.base.parent == &g_HudUiMgr && panel.base.next == nullptr;

    const bool ok = result == &panel &&
                    TestFieldAt<const void *>(&panel, 0x00) == &g_HudUiTripletPanel_FTable &&
                    panel.visibleCount == 0 && itemsHidden && linked;

    g_HudUiMgr = {};
    g_HudUi_InvalidateMask = 0;
    return ok ? 0 : 1;
}

extern "C" int zhud_triplet_panel_set_visible_count_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;

    HudUiTripletPanel panel{};
    panel.base.ftable = reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiTripletPanel_FTable);
    for (HudUiWidget &item : panel.items) {
        item.ftable = &g_HudUiWidget_FTable;
        reinterpret_cast<HudUiElement *>(&item)->flags = 0x10;
    }

    panel.visibleCount = 99;
    panel.SetVisibleCount(2);

    const bool twoVisible = panel.visibleCount == 2 && (panel.base.flags & 0x80) != 0 &&
                            (panel.items[0].flags & 0x10) == 0 &&
                            (panel.items[1].flags & 0x10) == 0 &&
                            (panel.items[2].flags & 0x10) != 0;

    panel.base.flags = 0;
    panel.items[0].flags = 0;
    panel.items[1].flags = 0;
    panel.items[2].flags = 0;
    panel.SetVisibleCount(-3);

    const bool noneVisible = panel.visibleCount == 0 && (panel.base.flags & 0x80) != 0 &&
                             (panel.items[0].flags & 0x10) != 0 &&
                             (panel.items[1].flags & 0x10) != 0 &&
                             (panel.items[2].flags & 0x10) != 0;

    panel.base.flags = 0;
    panel.SetVisibleCount(0);
    const bool unchanged = panel.base.flags == 0;

    g_HudUiMgrNanitePanel.visibleCount = 0;
    g_HudUiMgrNanitePanel.base.ftable =
        reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiTripletPanel_FTable);
    for (HudUiWidget &item : g_HudUiMgrNanitePanel.items) {
        item.ftable = &g_HudUiWidget_FTable;
        item.flags = 0x10;
    }

    HudUiMgr::SetNanitePanelCount(2);
    const bool naniteForwarded = g_HudUiMgrNanitePanel.visibleCount == 2 &&
                                 (g_HudUiMgrNanitePanel.items[0].flags & 0x10) == 0 &&
                                 (g_HudUiMgrNanitePanel.items[1].flags & 0x10) == 0 &&
                                 (g_HudUiMgrNanitePanel.items[2].flags & 0x10) != 0;

    g_HudUi_InvalidateMask = 0;
    g_HudUiMgrNanitePanel = {};
    return twoVisible && noneVisible && unchanged && naniteForwarded ? 0 : 1;
}

extern "C" int zhud_triplet_panel_shutdown_items_stub_smoke(void) {
    HudUiTripletPanel panel{};
    panel.visibleCount = 2;
    panel.items[0].flags = 0x12;
    panel.ShutdownItems_Stub();
    return panel.visibleCount == 2 && panel.items[0].flags == 0x12 ? 0 : 1;
}

extern "C" int zhud_triplet_panel_destructor_core_smoke(void) {
    HudUiTripletPanel panel{};
    panel.base.ftable = nullptr;
    for (HudUiWidget &item : panel.items) {
        item.ftable = &g_HudUiWidget_FTable;
        item.image = &zVid_Image::g_zImage_DefaultImage;
        item.ownsImage = 1;
    }

    panel.DestructorCore();

    bool itemsDestroyed = true;
    for (const HudUiWidget &item : panel.items) {
        itemsDestroyed = itemsDestroyed &&
                         TestFieldAt<const void *>(const_cast<HudUiWidget *>(&item), 0x00) ==
                             &g_HudUiCommon_FTable &&
                         item.image == nullptr && item.ownsImage == 0;
    }

    HudUiTripletPanel unwind{};
    unwind.items[0].image = &zVid_Image::g_zImage_DefaultImage;
    unwind.items[0].ownsImage = 1;
    unwind.items[1].image = &zVid_Image::g_zImage_DefaultImage;
    unwind.items[1].ownsImage = 1;
    unwind.UnwindDestructFirstItem();
    const bool firstOnly = unwind.items[0].image == nullptr && unwind.items[0].ownsImage == 0 &&
                           unwind.items[1].image == &zVid_Image::g_zImage_DefaultImage &&
                           unwind.items[1].ownsImage == 1;

    HudUiTripletPanel scalar{};
    for (HudUiWidget &item : scalar.items) {
        item.ftable = &g_HudUiWidget_FTable;
        item.image = &zVid_Image::g_zImage_DefaultImage;
        item.ownsImage = 1;
    }

    HudUiTripletPanel *const scalarResult = scalar.ScalarDeletingDestructor(0);
    const bool scalarDestroyed =
        scalarResult == &scalar && scalar.base.ftable == &g_HudUiCommon_FTable &&
        scalar.items[0].image == nullptr && scalar.items[1].image == nullptr &&
        scalar.items[2].image == nullptr;

    return panel.base.ftable == &g_HudUiCommon_FTable && itemsDestroyed && firstOnly &&
                   scalarDestroyed
               ? 0
               : 1;
}

extern "C" int zhud_text_input_destructor_core_smoke(void) {
    HudUiTextInput input{};
    input.ftable = nullptr;
    input.buffer = static_cast<char *>(::operator new(16));
    input.capacity = 16;
    input.cursor = 4;

    input.DestructorCore();

    return input.ftable == &g_HudUiTextInput_FTable && input.capacity == 16 && input.cursor == 4
               ? 0
               : 1;
}

extern "C" int zhud_text_input_constructor_and_alloc_smoke(void) {
    HudUiTextInput input{};
    HudUiTextInput *const result = input.Constructor(0x20);

    const bool constructorOk = result == &input && input.ftable == &g_HudUiTextInput_FTable &&
                               input.buffer != nullptr && input.capacity == 0x20 &&
                               input.cursor == 0 && input.keyActionMap[0x41] == 0 &&
                               input.keyActionMap[0x01] == 1 && input.keyActionMap[0x20] == 0 &&
                               input.keyActionMap[0x2e] == 0 && input.keyActionMap[0x1b] == 2 &&
                               input.keyActionMap[0x0d] == 3 && input.keyActionMap[0x08] == 4 &&
                               input.keyActionMap[0x7f] == 5 && input.keyActionMap[0x02] == 6 &&
                               input.keyActionMap[0x06] == 7;

    input.DestructorCore();

    HudUiTextInput resize{};
    resize.buffer = static_cast<char *>(::operator new(6));
    char *const oldBuffer = resize.buffer;
    std::memcpy(resize.buffer, "ABCDE", 6);
    resize.capacity = 6;

    resize.AllocTextBuffer(4);
    const bool allocOk = resize.buffer != nullptr && resize.buffer != oldBuffer &&
                         resize.capacity == 4 && std::memcmp(resize.buffer, "ABCD", 4) == 0;

    ::operator delete(oldBuffer);
    resize.DestructorCore();

    HudUiTextInput contents{};
    contents.Constructor(8);
    contents.cursor = 99;
    contents.SetContents("abcdefghi");
    const bool contentsOk =
        std::strcmp(contents.GetBuffer(), "abcdefg") == 0 && contents.cursor == 7;
    contents.SetCursorPosition(3);
    const bool cursorOk = contents.cursor == 3;
    contents.SetCursorPosition(99);
    const bool cursorClampOk = contents.cursor == 7;
    contents.DestructorCore();

    HudUiTextInput edit{};
    edit.Constructor(8);
    edit.SetContents("abcd");
    edit.SetCursorPosition(2);
    edit.InsertCharAtCursor('X');
    const bool insertOk = std::strcmp(edit.GetBuffer(), "abXcd") == 0 && edit.cursor == 3;
    edit.BackspaceDeleteChar();
    const bool backspaceOk = std::strcmp(edit.GetBuffer(), "abcd") == 0 && edit.cursor == 2;
    edit.DeleteCharForward();
    const bool deleteOk = std::strcmp(edit.GetBuffer(), "abd") == 0;
    edit.MoveCursorLeft();
    const bool moveLeftOk = edit.cursor == 1;
    edit.MoveCursorRight();
    edit.MoveCursorRight();
    edit.MoveCursorRight();
    const bool moveRightOk = edit.cursor == 3;
    edit.DestructorCore();

    HudUiTextInput_FTable dispatchTable{};
    dispatchTable.slots[0] = MethodAddress(&HudUiTextInput::InsertCharAtCursor);
    dispatchTable.slots[1] = MethodAddress(&HudUiTextInput::InsertCharAtCursor);
    dispatchTable.slots[2] = MethodAddress(&HudUiTextInput::MoveCursorRight);
    dispatchTable.slots[3] = MethodAddress(&HudUiTextInput::MoveCursorLeft);
    dispatchTable.slots[4] = MethodAddress(&HudUiTextInput::BackspaceDeleteChar);
    dispatchTable.slots[5] = MethodAddress(&HudUiTextInput::DeleteCharForward);
    dispatchTable.slots[6] = MethodAddress(&HudUiTextInput::MoveCursorLeft);
    dispatchTable.slots[7] = MethodAddress(&HudUiTextInput::MoveCursorRight);
    dispatchTable.slots[8] = MethodAddress(&HudUiTextInput::MoveCursorRight);

    HudUiTextInput dispatch{};
    dispatch.Constructor(8);
    dispatch.ftable = &dispatchTable;
    dispatch.buffer[0] = '\0';
    dispatch.keyActionMap['A'] = 0;
    dispatch.DispatchKeyAction('A');
    dispatch.keyActionMap['B'] = 1;
    dispatch.DispatchKeyAction('B');
    const bool keyDispatchOk = std::strcmp(dispatch.GetBuffer(), "AB") == 0 && dispatch.cursor == 2;

    dispatch.SetContents("abcd");
    dispatch.SetCursorPosition(2);
    dispatch.keyActionMap[2] = 2;
    dispatch.DispatchKeyAction(2);
    const bool cancelDispatchOk = dispatch.cursor == 1;
    dispatch.keyActionMap[3] = 3;
    dispatch.DispatchKeyAction(3);
    const bool acceptDispatchOk = dispatch.cursor == 2;
    dispatch.keyActionMap[4] = 4;
    dispatch.DispatchKeyAction(4);
    const bool backspaceDispatchOk =
        std::strcmp(dispatch.GetBuffer(), "acd") == 0 && dispatch.cursor == 1;
    dispatch.keyActionMap[5] = 5;
    dispatch.DispatchKeyAction(5);
    const bool deleteDispatchOk =
        std::strcmp(dispatch.GetBuffer(), "ad") == 0 && dispatch.cursor == 1;
    dispatch.keyActionMap[6] = 6;
    dispatch.DispatchKeyAction(6);
    const bool leftDispatchOk = dispatch.cursor == 0;
    dispatch.keyActionMap[7] = 7;
    dispatch.DispatchKeyAction(7);
    const bool rightDispatchOk = dispatch.cursor == 1;
    dispatch.keyActionMap[8] = 9;
    dispatch.DispatchKeyAction(8);
    int dispatchFailure = 0;
    dispatchFailure |= keyDispatchOk ? 0 : 0x001;
    dispatchFailure |= cancelDispatchOk ? 0 : 0x002;
    dispatchFailure |= acceptDispatchOk ? 0 : 0x004;
    dispatchFailure |= backspaceDispatchOk ? 0 : 0x008;
    dispatchFailure |= deleteDispatchOk ? 0 : 0x010;
    dispatchFailure |= leftDispatchOk ? 0 : 0x020;
    dispatchFailure |= rightDispatchOk ? 0 : 0x040;
    dispatchFailure |= dispatch.cursor == 1 ? 0 : 0x080;
    const bool dispatchOk = dispatchFailure == 0;

    dispatch.SetContents("abcdefg");
    dispatch.cursor = 1;
    dispatch.InsertCharAtCursor('Z');
    const bool fullOk = dispatch.cursor == 2;
    dispatch.DestructorCore();

    int failure = 0;
    failure |= constructorOk ? 0 : 0x001;
    failure |= allocOk ? 0 : 0x002;
    failure |= contentsOk ? 0 : 0x004;
    failure |= cursorOk ? 0 : 0x008;
    failure |= cursorClampOk ? 0 : 0x010;
    failure |= insertOk ? 0 : 0x020;
    failure |= backspaceOk ? 0 : 0x040;
    failure |= deleteOk ? 0 : 0x080;
    failure |= moveLeftOk ? 0 : 0x100;
    failure |= moveRightOk ? 0 : 0x200;
    failure |= dispatchOk ? 0 : (0x400 | (dispatchFailure << 12));
    failure |= fullOk ? 0 : 0x800;
    return failure;
}

extern "C" int zhud_mgr_objective_block_destructor_smoke(void) {
    HudUiMgrObjectiveBlock block{};
    block.chatComposeTextInput.ftable = nullptr;
    block.objectiveBar.ftable = &g_HudUiBar_FTable;
    block.objectiveMeter.ftable = &g_HudUiMeter_FTable;
    block.objectiveSensorRect.ftable = &g_HudUiWidget_FTable;
    block.objectiveSensorRect.image = &zVid_Image::g_zImage_DefaultImage;
    block.objectiveSensorRect.ownsImage = 1;
    block.objectiveWidget.ftable = &g_HudUiWidget_FTable;
    block.objectiveWidget.image = &zVid_Image::g_zImage_DefaultImage;
    block.objectiveWidget.ownsImage = 1;

    block.Destructor();

    return block.chatComposeTextInput.ftable == &g_HudUiTextInput_FTable &&
                   TestFieldAt<const void *>(&block.objectiveBar, 0x00) == &g_HudUiCommon_FTable &&
                   TestFieldAt<const void *>(&block.objectiveMeter, 0x00) ==
                       &g_HudUiCommon_FTable &&
                   block.objectiveSensorRect.image == nullptr &&
                   block.objectiveSensorRect.ownsImage == 0 &&
                   TestFieldAt<const void *>(&block.objectiveSensorRect, 0x00) ==
                       &g_HudUiCommon_FTable &&
                   block.objectiveWidget.image == nullptr && block.objectiveWidget.ownsImage == 0 &&
                   TestFieldAt<const void *>(&block.objectiveWidget, 0x00) == &g_HudUiCommon_FTable
               ? 0
               : 1;
}

extern "C" int zhud_slot_destructors_smoke(void) {
    HudUiSlot constructed{};
    HudUiSlot *const constructedResult = constructed.Constructor();
    const bool constructorOk = constructedResult == &constructed &&
                               constructed.ftable == &g_HudUiSlot_FTable &&
                               constructed.slotWidget.ftable == &g_HudUiWidget_FTable &&
                               constructed.trackMarkerWidget.ftable == &g_HudUiWidget_FTable;

    HudUiSlot slot{};
    slot.ftable = &g_HudUiSlot_FTable;
    slot.slotWidget.ftable = &g_HudUiWidget_FTable;
    slot.slotWidget.image = &zVid_Image::g_zImage_DefaultImage;
    slot.slotWidget.ownsImage = 1;
    slot.trackMarkerWidget.ftable = &g_HudUiWidget_FTable;
    slot.trackMarkerWidget.image = &zVid_Image::g_zImage_DefaultImage;
    slot.trackMarkerWidget.ownsImage = 1;

    slot.Destructor();

    const bool destroyed =
        TestFieldAt<const void *>(&slot, 0x00) == &g_HudUiCommon_FTable &&
        slot.slotWidget.image == nullptr && slot.slotWidget.ownsImage == 0 &&
        TestFieldAt<const void *>(&slot.slotWidget, 0x00) == &g_HudUiCommon_FTable &&
        slot.trackMarkerWidget.image == nullptr && slot.trackMarkerWidget.ownsImage == 0 &&
        TestFieldAt<const void *>(&slot.trackMarkerWidget, 0x00) == &g_HudUiCommon_FTable;

    HudUiSlot scalar{};
    scalar.ftable = &g_HudUiSlot_FTable;
    scalar.slotWidget.ftable = &g_HudUiWidget_FTable;
    scalar.trackMarkerWidget.ftable = &g_HudUiWidget_FTable;

    return constructorOk && destroyed && scalar.ScalarDeletingDestructor(0) == &scalar &&
                   TestFieldAt<const void *>(&scalar, 0x00) == &g_HudUiCommon_FTable
               ? 0
               : 1;
}

extern "C" int zhud_slot_draw_smoke(void) {
    HudUiWidget_FTable table{};
    table.slots[1] = reinterpret_cast<std::uintptr_t>(&TestSlotWidgetDraw);

    HudUiSlot slot{};
    slot.slotWidget.ftable = &table;
    slot.trackMarkerWidget.ftable = &table;

    g_slotWidgetDrawCount = 0;
    slot.Draw();
    const bool bothVisible = g_slotWidgetDrawCount == 2;

    slot.slotWidget.flags = 0x10;
    slot.trackMarkerWidget.flags = 0;
    slot.Draw();
    const bool onlyTrackMarkerVisible = g_slotWidgetDrawCount == 3;

    slot.slotWidget.flags = 0;
    slot.trackMarkerWidget.flags = 0x10;
    slot.Draw();
    const bool onlySlotVisible = g_slotWidgetDrawCount == 4;

    return bothVisible && onlyTrackMarkerVisible && onlySlotVisible ? 0 : 1;
}

extern "C" int zhud_stats_list_element_update_smoke(void) {
    HudUiTriplet_FTable table{};
    AssignMethodSlot(table.updateAll, &TestTripletContainerDispatch::UpdateAll);

    HudUiTriplet triplet{};
    triplet.base.vptr = reinterpret_cast<const HudUiContainer_FTable *>(&table);

    HudUiStatsListElement stats{};
    stats.triplet = &triplet;
    g_tripletUpdateAllCount = 0;
    g_tripletUpdateAllDelta = 0.0f;

    stats.Update(1.25f);

    return g_tripletUpdateAllCount == 1 && g_tripletUpdateAllDelta == 1.25f ? 0 : 1;
}

extern "C" int zhud_bar_and_meter_constructor_smoke(void) {
    g_HudUi_InvalidateMask = 0x40;

    HudUiBar bar{};
    for (HudUiBarPoint &point : bar.points) {
        point.x = 1.0f;
        point.y = 2.0f;
        point.reserved = 3;
    }
    bar.drawVertexCount = 9;
    bar.Constructor();

    bool pointsCleared = true;
    for (const HudUiBarPoint &point : bar.points) {
        pointsCleared = pointsCleared && point.x == 0.0f && point.y == 0.0f && point.reserved == 0;
    }

    HudUiMeter meter{};
    meter.fillPixelsMax = 7;
    meter.meterFlags = 8;
    meter.Constructor();

    HudUiMeter meterEx{};
    meterEx.fillPixelsMax = 7;
    meterEx.meterFlags = 8;
    meterEx.ConstructorEx();

    const bool ok = bar.ftable == &g_HudUiBar_FTable && (bar.flags & 0x40) != 0 &&
                    bar.drawVertexCount == 0 && pointsCleared &&
                    meter.ftable == &g_HudUiMeter_FTable && meter.fillPixelsMax == 0 &&
                    meter.meterFlags == 0 && meterEx.ftable == &g_HudUiMeterEx_FTable &&
                    meterEx.fillPixelsMax == 0 && meterEx.meterFlags == 0;

    g_HudUi_InvalidateMask = 0;
    return ok ? 0 : 1;
}

extern "C" int zhud_bar_set_point_xy_smoke(void) {
    HudUiBar_FTable table{};
    table.slots[3] = MethodAddress(&TestBarSetPointReceiver::SetPos);
    table.slots[8] = MethodAddress(&TestBarSetPointReceiver::Invalidate);

    HudUiBar bar{};
    bar.ftable = &table;
    bar.drawVertexCount = 2;

    g_barSetPointSetPosCount = 0;
    g_barSetPointSetPosThis = nullptr;
    g_barSetPointSetPosX = 0;
    g_barSetPointSetPosY = 0;
    g_barSetPointInvalidateCount = 0;
    g_barSetPointInvalidateThis = nullptr;

    bar.SetPointXY(0, 5.75f, -3.25f);
    const bool firstPoint = bar.points[0].x == 5.75f && bar.points[0].y == -3.25f &&
                            bar.drawVertexCount == 2 && g_barSetPointSetPosCount == 1 &&
                            g_barSetPointSetPosThis == &bar && g_barSetPointSetPosX == 5 &&
                            g_barSetPointSetPosY == -3 && g_barSetPointInvalidateCount == 1 &&
                            g_barSetPointInvalidateThis == &bar;

    bar.SetPointXY(4, 9.0f, 10.0f);
    const bool laterPoint = bar.points[4].x == 9.0f && bar.points[4].y == 10.0f &&
                            bar.drawVertexCount == 5 && g_barSetPointSetPosCount == 1 &&
                            g_barSetPointInvalidateCount == 2;

    bar.SetPointXY(21, 100.0f, 200.0f);
    bar.SetPointXY(-1, 300.0f, 400.0f);
    const bool rejected = bar.drawVertexCount == 5 && g_barSetPointSetPosCount == 1 &&
                          g_barSetPointInvalidateCount == 4;

    return firstPoint && laterPoint && rejected ? 0 : 1;
}

extern "C" int zhud_layout_node_apply_meter_quad_smoke(void) {
    zReader::Node meterItems[5] = {};
    meterItems[1].value.i32 = 10;
    meterItems[2].value.i32 = 20;
    meterItems[3].value.i32 = 30;
    meterItems[4].value.i32 = 40;

    zReader::Node meterNode{};
    meterNode.type = zReader::ZRDR_NODE_ARRAY;
    meterNode.value.nodes = meterItems;

    HudUiMeter meter{};
    meter.ftable = reinterpret_cast<const HudUiMeter_FTable *>(&g_HudUiBar_FTable);
    const int offsetXY[2] = {3, -4};
    HudUiRect outRect{};

    const int result =
        HudUiLayoutNode::ApplyMeterQuad(&meterNode, &meter, 100, 200, offsetXY, &outRect);
    const bool applied = result == 1 && outRect.left == 10 && outRect.top == 20 &&
                         outRect.right == 31 && outRect.bottom == 41 &&
                         meter.points[0].x == 13.0f && meter.points[0].y == 216.0f &&
                         meter.points[1].x == 13.0f && meter.points[1].y == 237.0f &&
                         meter.points[2].x == 135.0f && meter.points[2].y == 237.0f &&
                         meter.points[3].x == 135.0f && meter.points[3].y == 216.0f &&
                         meter.drawVertexCount == 4 && meter.fillPixelsMax == 21 &&
                         meter.meterFlags == 121;

    zReader::Node scalarNode{};
    scalarNode.type = zReader::ZRDR_NODE_INT;
    HudUiMeter rejectedMeter{};
    rejectedMeter.ftable = reinterpret_cast<const HudUiMeter_FTable *>(&g_HudUiBar_FTable);
    rejectedMeter.fillPixelsMax = 7;
    rejectedMeter.meterFlags = 8;
    HudUiRect rejectedRect = {1, 2, 3, 4};
    const int rejectedResult = HudUiLayoutNode::ApplyMeterQuad(
        &scalarNode, &rejectedMeter, 10, 20, offsetXY, &rejectedRect);

    const bool rejected = rejectedResult == 0 && rejectedMeter.drawVertexCount == 0 &&
                          rejectedMeter.fillPixelsMax == 7 && rejectedMeter.meterFlags == 8 &&
                          rejectedRect.left == 1 && rejectedRect.top == 2 &&
                          rejectedRect.right == 3 && rejectedRect.bottom == 4;

    return applied && rejected ? 0 : 1;
}

extern "C" int zhud_layout_node_apply_corner_text_quad_smoke(void) {
    zReader::Node quadItems[5] = {};
    quadItems[1].value.i32 = 10;
    quadItems[2].value.i32 = 20;
    quadItems[3].value.i32 = 30;
    quadItems[4].value.i32 = 40;

    zReader::Node quadNode{};
    quadNode.type = zReader::ZRDR_NODE_ARRAY;
    quadNode.value.nodes = quadItems;

    HudUiBar bar{};
    bar.ftable = &g_HudUiBar_FTable;
    const int offsetXY[2] = {3, -4};
    HudUiRect outRect{};

    const int result =
        HudUiLayoutNode::ApplyCornerTextQuad(&quadNode, &bar, offsetXY, &outRect);
    const bool applied = result == 1 && outRect.left == 13 && outRect.top == 16 &&
                         outRect.right == 33 && outRect.bottom == 36 &&
                         bar.points[0].x == 13.0f && bar.points[0].y == 16.0f &&
                         bar.points[1].x == 13.0f && bar.points[1].y == 36.0f &&
                         bar.points[2].x == 33.0f && bar.points[2].y == 36.0f &&
                         bar.points[3].x == 33.0f && bar.points[3].y == 16.0f &&
                         bar.drawVertexCount == 4;

    zReader::Node scalarNode{};
    scalarNode.type = zReader::ZRDR_NODE_INT;
    HudUiBar rejectedBar{};
    rejectedBar.ftable = &g_HudUiBar_FTable;
    rejectedBar.drawVertexCount = 7;
    HudUiRect rejectedRect = {1, 2, 3, 4};
    const int rejectedResult =
        HudUiLayoutNode::ApplyCornerTextQuad(&scalarNode, &rejectedBar, offsetXY, &rejectedRect);
    const bool rejected = rejectedResult == 0 && rejectedBar.drawVertexCount == 7 &&
                          rejectedRect.left == 1 && rejectedRect.top == 2 &&
                          rejectedRect.right == 3 && rejectedRect.bottom == 4;

    return applied && rejected ? 0 : 1;
}

extern "C" int zhud_polyline_and_slider_border_constructor_smoke(void) {
    g_HudUi_InvalidateMask = 0x20;

    HudUiPolyline polyline{};
    RECT clip{1, 2, 3, 4};
    polyline.color565 = 0x1234;
    polyline.clipRect = &clip;
    polyline.points[5].x = 11;
    polyline.points[5].y = 12;
    polyline.pointCount = 9;
    polyline.Constructor();

    bool pointsCleared = true;
    for (const HudUiPolylinePoint &point : polyline.points) {
        pointsCleared = pointsCleared && point.x == 0 && point.y == 0;
    }

    const bool constructed =
        polyline.base.ftable ==
            reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiPolyline_FTable) &&
        (polyline.base.flags & 0x20) != 0 && polyline.pointCount == 0 &&
        polyline.color565 == 0x1234 && polyline.clipRect == nullptr && pointsCleared;

    polyline.base.flags = 0;
    polyline.SetPoint(0, 5, 6);
    const bool firstPoint = polyline.points[0].x == 5 && polyline.points[0].y == 6 &&
                            polyline.pointCount == 1 && polyline.base.x == 5 &&
                            polyline.base.y == 6 && (polyline.base.flags & 0x20) != 0;

    polyline.base.flags = 0;
    polyline.SetPoint(3, 7, 8);
    const bool laterPoint = polyline.points[3].x == 7 && polyline.points[3].y == 8 &&
                            polyline.pointCount == 4 && polyline.base.x == 5 &&
                            polyline.base.y == 6 && (polyline.base.flags & 0x20) != 0;

    HudUiSliderBorder border{};
    border.caretHalfWidth = 0x55;
    border.inputActive = 0x66;
    border.sliderVisibleWhenInputActive = 0x12;
    border.rawKeyFilterEnabled = 0x34;
    border.Constructor();

    const HudUiPolylinePoint expected[] = {
        {-1, 0},  {1, 0},  {1, 1}, {0, 1}, {0, 9},  {1, 9},  {1, 10},
        {-1, 10}, {-1, 9}, {0, 9}, {0, 1}, {-1, 1}, {-1, 0},
    };

    bool borderPoints = true;
    for (int index = 0; index < 13; ++index) {
        borderPoints = borderPoints && border.base.points[index].x == expected[index].x &&
                       border.base.points[index].y == expected[index].y;
    }

    const bool borderConstructed =
        border.base.base.ftable ==
            reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiSliderBorder_FTable) &&
        border.base.pointCount == 13 && border.base.base.x == -1 && border.base.base.y == 0 &&
        border.originX == 0 && border.originY == 0 && border.halfWidth == 1 &&
        border.height == 10 && border.blinkEnabled == 0 && border.blinkPeriodSec == 0.35f &&
        border.blinkTimeRemainingSec == 0.0f && border.blinkDirSign == 1 &&
        border.caretHalfWidth == 0x55 && border.inputActive == 0x66 &&
        border.sliderVisibleWhenInputActive == 0x12 && border.rawKeyFilterEnabled == 0x34 &&
        borderPoints;

    border.SetBounds(10, 20, 3, 8);
    const HudUiPolylinePoint expectedBounds[] = {
        {7, 20}, {13, 20}, {13, 21}, {10, 21}, {10, 27}, {13, 27}, {13, 28},
        {7, 28}, {7, 27},  {10, 27}, {10, 21}, {7, 21},  {7, 20},
    };

    bool boundsPoints = true;
    for (int index = 0; index < 13; ++index) {
        boundsPoints = boundsPoints && border.base.points[index].x == expectedBounds[index].x &&
                       border.base.points[index].y == expectedBounds[index].y;
    }

    const bool boundsSet = border.originX == 10 && border.originY == 20 && border.halfWidth == 3 &&
                           border.height == 8 && border.base.pointCount == 13 &&
                           border.base.base.x == 7 && border.base.base.y == 20 && boundsPoints;

    g_HudUi_InvalidateMask = 0;
    return constructed && firstPoint && laterPoint && borderConstructed && boundsSet ? 0 : 1;
}

extern "C" int zhud_polyline_draw_and_slider_update_smoke(void) {
    HudUiCommon_FTable table{};
    table.slots[2] = reinterpret_cast<std::uintptr_t>(TestElementBaseDraw);

    zRndr::g_frameBuffer = &table;
    zRndr::g_pfnImmediateRaster4 = reinterpret_cast<zRndr::SpanRoutineProc>(TestPolylineRaster4);
    zRndr::g_pfnImmediateRaster5 = reinterpret_cast<zRndr::SpanRoutineProc>(TestPolylineRaster5);

    HudUiPolyline polyline{};
    polyline.base.ftable = &table;
    polyline.pointCount = 3;
    polyline.color565 = 0x77;
    polyline.points[0] = {1, 2};
    polyline.points[1] = {3, 4};
    polyline.points[2] = {5, 6};

    g_elementBaseDrawCount = 0;
    g_polylineLineCount = 0;
    polyline.Draw();
    const bool unclipped = g_elementBaseDrawCount == 1 && g_polylineLineCount == 2 &&
                           g_polylineLineArgs[0] == 3 && g_polylineLineArgs[1] == 4 &&
                           g_polylineLineArgs[2] == 5 && g_polylineLineArgs[3] == 6 &&
                           g_polylineLineArgs[4] == 0x77;

    RECT clip{0, 0, 10, 10};
    polyline.clipRect = &clip;
    g_elementBaseDrawCount = 0;
    g_polylineLineCount = 0;
    polyline.Draw();
    const bool clipped = g_elementBaseDrawCount == 1 && g_polylineLineCount == 2 &&
                         g_polylineLineArgs[0] == 3 && g_polylineLineArgs[1] == 4 &&
                         g_polylineLineArgs[2] == 5 && g_polylineLineArgs[3] == 6 &&
                         g_polylineLineArgs[4] == 0x77;

    HudUiSliderBorder border{};
    border.base.base.ftable = &table;
    border.base.pointCount = 0;
    border.base.base.flags = 0x10;
    border.blinkEnabled = 0;
    g_elementBaseDrawCount = 0;
    border.Update(0.1f);
    const bool hiddenSkipped = g_elementBaseDrawCount == 0;

    border.base.base.flags = 0;
    border.Update(0.1f);
    const bool noBlinkDraw = g_elementBaseDrawCount == 1;

    border.blinkEnabled = 1;
    border.blinkDirSign = 1;
    border.blinkPeriodSec = 0.35f;
    border.blinkTimeRemainingSec = 0.10f;
    border.Update(0.05f);
    const bool blinkStillVisible = g_elementBaseDrawCount == 2 && border.blinkDirSign == 1 &&
                                   border.blinkTimeRemainingSec > 0.049f &&
                                   border.blinkTimeRemainingSec < 0.051f;

    border.blinkTimeRemainingSec = 0.01f;
    border.Update(0.02f);
    const bool blinkFlippedHidden = g_elementBaseDrawCount == 2 && border.blinkDirSign == -1 &&
                                    border.blinkTimeRemainingSec == 0.35f;

    border.blinkTimeRemainingSec = 0.01f;
    border.Update(0.02f);
    const bool blinkFlippedVisible = g_elementBaseDrawCount == 3 && border.blinkDirSign == 1 &&
                                     border.blinkTimeRemainingSec == 0.35f;

    return unclipped && clipped && hiddenSkipped && noBlinkDraw && blinkStillVisible &&
                   blinkFlippedHidden && blinkFlippedVisible
               ? 0
               : 1;
}

extern "C" int zhud_panel_scalar_deleting_destructor_smoke(void) {
    HudUiPanel panel{};
    panel.vtbl = &g_HudUiPanel_FTable;
    panel.textPick = nullptr;
    panel.hFont = nullptr;

    HudUiPanel *const result = panel.ScalarDeletingDestructor(0);

    return result == &panel && panel.vtbl == &g_HudUiCommon_FTable ? 0 : 1;
}

extern "C" int zhud_panel_text_color_shadow_smoke(void) {
    alignas(HudUiPanel) std::uint8_t storage[0x2ac]{};
    auto *panel = reinterpret_cast<HudUiPanel *>(storage);

    TestFieldAt<std::uint32_t>(panel, 0x14c) = 0x111111;
    TestFieldAt<std::uint32_t>(panel, 0x264) = 5;

    const std::uint32_t previousColor = panel->SetTextColor(0x223344);
    panel->SetTextColorsAndMarkDirty(0x010203, 0x040506);
    const std::uint32_t previousShadow = panel->SetShadow(1, 7, -3);

    g_HudUi_InvalidateMask = 0x80;
    TestFieldAt<std::uint32_t>(panel, 0x0c) = 0;
    TestFieldAt<std::uint32_t>(panel, 0x270) = 0;
    panel->Invalidate();
    const bool invalidated = TestFieldAt<std::uint32_t>(panel, 0x0c) == 0x80 &&
                             TestFieldAt<std::uint32_t>(panel, 0x270) == 1;
    g_HudUi_InvalidateMask = 0;

    const auto fontHandle = reinterpret_cast<HGDIOBJ>(0x12345678);
    panel->SetFontHandle(fontHandle);
    const bool fontSet = panel->GetFont() == fontHandle;

    const HudUiRect wrapRect{3, 4, 50, 60};
    panel->EnableWordWrapWithRect(&wrapRect);
    const bool wordWrap =
        TestFieldAt<std::uint32_t>(panel, 0x278) == 1 &&
        std::memcmp(&TestFieldAt<HudUiRect>(panel, 0x27c), &wrapRect, sizeof(wrapRect)) == 0;

    HudUtil field{std::malloc(4)};
    field.FreeFieldPtr();
    const bool freed = field.fieldPtr == nullptr;
    field.FreeFieldPtr();
    const bool freeNull = field.fieldPtr == nullptr;

    return previousColor == 0x111111 && TestFieldAt<std::uint32_t>(panel, 0x14c) == 0x010203 &&
                   TestFieldAt<std::uint32_t>(panel, 0x150) == 0x040506 &&
                   TestFieldAt<std::uint32_t>(panel, 0x270) == 1 && previousShadow == 5 &&
                   TestFieldAt<std::uint32_t>(panel, 0x264) == 1 &&
                   TestFieldAt<std::int32_t>(panel, 0x29c) == 7 &&
                   TestFieldAt<std::int32_t>(panel, 0x2a0) == -3 && invalidated && fontSet &&
                   wordWrap && freed && freeNull
               ? 0
               : 1;
}

extern "C" int zhud_panel_constructor_default_smoke(void) {
    alignas(HudUiPanel) std::uint8_t storage[0x2ac]{};
    auto *panel = reinterpret_cast<HudUiPanel *>(storage);

    panel->ConstructorDefault("TXT", 3, 28);

    return panel->vtbl == &g_HudUiPanel_FTable &&
                   std::strcmp(&TestFieldAt<char>(panel, 0x34), "TXT") == 0 &&
                   TestFieldAt<std::int32_t>(panel, 0x14) == 3 &&
                   TestFieldAt<std::int32_t>(panel, 0x18) == 28 &&
                   TestFieldAt<void *>(panel, 0x148) == nullptr &&
                   TestFieldAt<std::uint32_t>(panel, 0x14c) == 0x00ffffff &&
                   TestFieldAt<std::uint32_t>(panel, 0x150) == 0x00ffffff &&
                   TestFieldAt<void *>(panel, 0x154) != nullptr &&
                   TestFieldAt<char>(panel, 0x15c) == 0 &&
                   TestFieldAt<std::int32_t>(panel, 0x25c) == 0 &&
                   TestFieldAt<std::int32_t>(panel, 0x260) == 0 &&
                   TestFieldAt<std::uint32_t>(panel, 0x264) == 0 &&
                   TestFieldAt<std::uint32_t>(panel, 0x268) == 1 &&
                   TestFieldAt<std::uint32_t>(panel, 0x270) == 1 &&
                   panel->GetWrapRect() == reinterpret_cast<HudUiRect *>(storage + 0x27c) &&
                   TestFieldAt<std::int32_t>(panel, 0x28c) == 0 &&
                   TestFieldAt<std::int32_t>(panel, 0x290) == 0 &&
                   TestFieldAt<std::int32_t>(panel, 0x294) == 0 &&
                   TestFieldAt<std::int32_t>(panel, 0x298) == 0
               ? 0
               : 1;
}

extern "C" int zhud_panel_copy_construct_core_smoke(void) {
    alignas(HudUiPanel) std::uint8_t sourceStorage[0x2ac]{};
    alignas(HudUiPanel) std::uint8_t copiedStorage[0x2ac]{};
    auto *source = reinterpret_cast<HudUiPanel *>(sourceStorage);
    auto *copied = reinterpret_cast<HudUiPanel *>(copiedStorage);

    source->ConstructorDefault("SRC", 4, 5);
    TestFieldAt<void *>(source, 0x148) = reinterpret_cast<void *>(0x1111);
    TestFieldAt<std::uint32_t>(source, 0x14c) = 0x010203;
    TestFieldAt<std::uint32_t>(source, 0x150) = 0x040506;
    TestFieldAt<std::uint32_t>(source, 0x158) = 0x77;
    std::strcpy(&TestFieldAt<char>(source, 0x15c), "cached");
    TestFieldAt<std::int32_t>(source, 0x25c) = 11;
    TestFieldAt<std::int32_t>(source, 0x260) = 12;
    TestFieldAt<std::uint32_t>(source, 0x264) = 1;
    TestFieldAt<std::uint32_t>(source, 0x268) = 2;
    TestFieldAt<std::uint32_t>(source, 0x26c) = 3;
    TestFieldAt<std::uint32_t>(source, 0x270) = 4;
    TestFieldAt<std::int32_t>(source, 0x274) = 5;
    TestFieldAt<std::int32_t>(source, 0x278) = 6;
    TestFieldAt<HudUiRect>(source, 0x27c) = {7, 8, 9, 10};
    TestFieldAt<HudUiRect>(source, 0x28c) = {13, 14, 15, 16};
    TestFieldAt<std::int32_t>(source, 0x144) = 2;
    TestFieldAt<std::int32_t>(source, 0x29c) = -3;
    TestFieldAt<std::int32_t>(source, 0x2a0) = 4;

    copied->CopyConstructCore(source);
    const bool copiedOk =
        copied->vtbl == &g_HudUiPanel_FTable && TestFieldAt<void *>(copied, 0x148) == nullptr &&
        TestFieldAt<std::uint32_t>(copied, 0x14c) == 0x010203 &&
        TestFieldAt<std::uint32_t>(copied, 0x150) == 0x040506 &&
        TestFieldAt<void *>(copied, 0x154) != nullptr &&
        TestFieldAt<void *>(copied, 0x154) != TestFieldAt<void *>(source, 0x154) &&
        TestFieldAt<std::uint32_t>(copied, 0x158) == 0x77 &&
        std::strcmp(&TestFieldAt<char>(copied, 0x15c), "cached") == 0 &&
        TestFieldAt<std::int32_t>(copied, 0x25c) == 11 &&
        TestFieldAt<std::int32_t>(copied, 0x260) == 12 &&
        TestFieldAt<std::uint32_t>(copied, 0x264) == 1 &&
        TestFieldAt<std::uint32_t>(copied, 0x268) == 2 &&
        TestFieldAt<std::uint32_t>(copied, 0x26c) == 3 &&
        TestFieldAt<std::uint32_t>(copied, 0x270) == 4 &&
        TestFieldAt<std::int32_t>(copied, 0x274) == 5 &&
        TestFieldAt<std::int32_t>(copied, 0x278) == 6 &&
        TestFieldAt<HudUiRect>(copied, 0x27c).left == 7 &&
        TestFieldAt<HudUiRect>(copied, 0x28c).right == 15 &&
        TestFieldAt<std::int32_t>(copied, 0x144) == 2 &&
        TestFieldAt<std::int32_t>(copied, 0x29c) == -3 &&
        TestFieldAt<std::int32_t>(copied, 0x2a0) == 4;

    alignas(HudUiPanel) std::uint8_t assignedStorage[0x2ac]{};
    auto *assigned = reinterpret_cast<HudUiPanel *>(assignedStorage);
    assigned->vtbl = &g_HudUiCommon_FTable;
    assigned->hFont = nullptr;
    HudUiPanel *const assignedResult = assigned->ConstructorCopy(source);
    const bool assignedOk =
        assignedResult == assigned && assigned->vtbl == &g_HudUiCommon_FTable &&
        TestFieldAt<void *>(assigned, 0x148) == nullptr &&
        TestFieldAt<std::uint32_t>(assigned, 0x14c) == 0x010203 &&
        TestFieldAt<std::uint32_t>(assigned, 0x150) == 0x040506 &&
        TestFieldAt<void *>(assigned, 0x154) != nullptr &&
        TestFieldAt<void *>(assigned, 0x154) != TestFieldAt<void *>(source, 0x154) &&
        TestFieldAt<std::uint32_t>(assigned, 0x158) == 0x77 &&
        std::strcmp(&TestFieldAt<char>(assigned, 0x15c), "cached") == 0 &&
        TestFieldAt<std::int32_t>(assigned, 0x25c) == 11 &&
        TestFieldAt<std::int32_t>(assigned, 0x260) == 12 &&
        TestFieldAt<std::uint32_t>(assigned, 0x264) == 1 &&
        TestFieldAt<std::uint32_t>(assigned, 0x268) == 2 &&
        TestFieldAt<std::uint32_t>(assigned, 0x26c) == 3 &&
        TestFieldAt<std::uint32_t>(assigned, 0x270) == 1 &&
        TestFieldAt<std::int32_t>(assigned, 0x274) == 5 &&
        TestFieldAt<std::int32_t>(assigned, 0x278) == 6 &&
        TestFieldAt<HudUiRect>(assigned, 0x27c).left == 7 &&
        TestFieldAt<HudUiRect>(assigned, 0x28c).right == 15 &&
        TestFieldAt<std::int32_t>(assigned, 0x144) == 2 &&
        TestFieldAt<std::int32_t>(assigned, 0x29c) == -3 &&
        TestFieldAt<std::int32_t>(assigned, 0x2a0) == 4;

    DeleteObject(assigned->hFont);
    assigned->hFont = nullptr;
    DeleteObject(copied->hFont);
    copied->hFont = nullptr;
    DeleteObject(source->hFont);
    source->hFont = nullptr;
    return copiedOk && assignedOk ? 0 : 1;
}

extern "C" int zhud_text_label_constructor_and_extents_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;

    HudUiTextLabel label{};
    HudUiTextLabel *const result = label.ConstructorWithPosAndFlags("Speed", 12, 34, 3);

    const bool constructed = result == &label &&
                             label.base.ftable == reinterpret_cast<const HudUiCommon_FTable *>(
                                                      &g_HudUiTextLabel_FTable) &&
                             std::strcmp(label.textBuffer, "Speed") == 0 && label.base.x == 12 &&
                             label.base.y == 34 && label.fontHandle == 3 && label.centerText == 0 &&
                             label.alignMode == 0 && (label.base.flags & 0x80) != 0;

    HudUiTextLabel assigned{};
    assigned.base.ftable = &g_HudUiCircle_FTable;
    HudUiTextLabel *const assignedResult = assigned.Constructor(&label);
    const bool assignedLabel =
        assignedResult == &assigned && assigned.base.ftable == &g_HudUiCircle_FTable &&
        std::strcmp(assigned.textBuffer, "Speed") == 0 && assigned.base.x == 12 &&
        assigned.base.y == 34 && assigned.fontHandle == 3 && assigned.centerText == 0 &&
        assigned.alignMode == 0;

    zVidImagePartial image{};
    image.height = 6;

    zImage_Font font{};
    font.image = &image;
    font.glyphRects['A' - 0x21].left = 2;
    font.glyphRects['A' - 0x21].right = 12;
    font.glyphRects['B' - 0x21].left = 4;
    font.glyphRects['B' - 0x21].right = 10;
    g_zImage_FontTable[1] = &font;

    label.fontHandle = 1;
    label.centerText = 1;
    label.centerBoundsLeft = 100;
    label.centerBoundsRight = 150;
    label.base.y = 20;
    label.base.bltSource = &label;
    label.base.flags = 0;
    label.SetTextFmt("AB");

    const bool centered = label.base.x == 117 && label.base.clipRect.left == 117 &&
                          label.base.clipRect.top == 20 && label.base.clipRect.right == 133 &&
                          label.base.clipRect.bottom == 26 && (label.base.flags & 0x80) != 0;

    HudUiTextLabel copied{};
    copied.base.padding32 = 0x7777;
    copied.CopyConstructor(&label);
    const bool copiedLabel =
        copied.base.ftable ==
            reinterpret_cast<const HudUiCommon_FTable *>(&g_HudUiTextLabel_FTable) &&
        copied.base.next == nullptr && copied.base.parent == nullptr &&
        copied.base.x == label.base.x && copied.base.y == label.base.y &&
        copied.base.state == label.base.state && copied.base.padding32 == 0x7777 &&
        std::strcmp(copied.textBuffer, label.textBuffer) == 0 &&
        copied.fontHandle == label.fontHandle && copied.centerText == label.centerText &&
        copied.centerBoundsLeft == label.centerBoundsLeft &&
        copied.centerBoundsRight == label.centerBoundsRight && copied.alignMode == label.alignMode;

    label.base.flags = 0;
    std::memset(label.textBuffer, 'X', sizeof(label.textBuffer));
    label.SetTextFmt(nullptr);

    const bool cleared = label.textBuffer[0] == 0 &&
                         label.textBuffer[sizeof(label.textBuffer) - 1] == 0 &&
                         label.base.flags == 0;

    g_zImage_FontTable[1] = nullptr;
    g_HudUi_InvalidateMask = 0;
    return constructed && assignedLabel && centered && copiedLabel && cleared ? 0 : 1;
}

extern "C" int zhud_panel_set_font_smoke(void) {
    alignas(HudUiPanel) std::uint8_t storage[0x2ac]{};
    auto *panel = reinterpret_cast<HudUiPanel *>(storage);

    panel->hFont = nullptr;
    panel->SetFont("Arial", 10, FW_NORMAL, 6, 0, ANSI_CHARSET, DEFAULT_PITCH);

    const bool updated = panel->hFont != nullptr && TestFieldAt<std::uint32_t>(panel, 0x270) == 1;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    return updated ? 0 : 1;
}

extern "C" int zhud_panel_set_text_fmt_smoke(void) {
    alignas(HudUiPanel) std::uint8_t storage[0x2ac]{};
    auto *panel = reinterpret_cast<HudUiPanel *>(storage);
    panel->ConstructorDefault("", 4, 5);

    panel->SetTextFmt("A%02d", 7);
    const bool firstUpdate = std::strcmp(&TestFieldAt<char>(panel, 0x34), "A07") == 0 &&
                             std::strncmp(&TestFieldAt<char>(panel, 0x15c), "A07", 3) == 0 &&
                             TestFieldAt<std::uint32_t>(panel, 0x270) == 1;

    TestFieldAt<std::uint32_t>(panel, 0x270) = 0;
    panel->SetTextFmt("A%02d", 7);
    const bool unchangedSkipped = TestFieldAt<std::uint32_t>(panel, 0x270) == 0;

    panel->SetTextFmt(nullptr);
    const bool cleared =
        TestFieldAt<char>(panel, 0x34) == 0 && TestFieldAt<std::uint32_t>(panel, 0x270) == 1;

    TestFieldAt<std::uint32_t>(panel, 0x270) = 0;
    panel->SetText("Plain");
    const bool setText = std::strcmp(&TestFieldAt<char>(panel, 0x34), "Plain") == 0 &&
                         std::strncmp(&TestFieldAt<char>(panel, 0x15c), "Plain", 5) == 0 &&
                         TestFieldAt<std::uint32_t>(panel, 0x270) == 1;

    TestFieldAt<std::uint32_t>(panel, 0x270) = 0;
    panel->SetText("Plain");
    const bool setTextUnchanged = TestFieldAt<std::uint32_t>(panel, 0x270) == 0;

    panel->SetText(nullptr);
    const bool setTextCleared =
        TestFieldAt<char>(panel, 0x34) == 0 && TestFieldAt<std::uint32_t>(panel, 0x270) == 1;

    TestFieldAt<std::uint32_t>(panel, 0x270) = 0;
    TestPanelSetTextFmtV(panel, "V%02d", 9);
    const bool fmtV = std::strcmp(&TestFieldAt<char>(panel, 0x34), "V09") == 0 &&
                      std::strncmp(&TestFieldAt<char>(panel, 0x15c), "V09", 3) == 0 &&
                      TestFieldAt<std::uint32_t>(panel, 0x270) == 1;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    return firstUpdate && unchangedSkipped && cleared && setText && setTextUnchanged &&
                   setTextCleared && fmtV
               ? 0
               : 1;
}

extern "C" int zhud_panel_measure_text_prefix_rect_smoke(void) {
    alignas(HudUiPanel) std::uint8_t storage[0x2ac]{};
    auto *panel = reinterpret_cast<HudUiPanel *>(storage);
    panel->ConstructorDefault("WWW", 0, 0);

    RECT whole{10, 20, 10, 20};
    const std::int32_t wholeResult = panel->MeasureTextPrefixRect(3, &whole);

    RECT prefix{10, 20, 10, 20};
    const std::int32_t prefixResult = panel->MeasureTextPrefixRect(1, &prefix);

    RECT empty{10, 20, 99, 20};
    const std::int32_t emptyResult = panel->MeasureTextPrefixRect(0, &empty);

    const bool measured = wholeResult == 1 && prefixResult == 1 && emptyResult == 1 &&
                          whole.right > prefix.right && prefix.right > prefix.left &&
                          empty.right == empty.left;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    return measured ? 0 : 1;
}

extern "C" int zhud_panel_query_text_height_smoke(void) {
    alignas(HudUiPanel) std::uint8_t storage[0x2ac]{};
    auto *panel = reinterpret_cast<HudUiPanel *>(storage);
    TestFieldAt<std::uint32_t>(panel, 0x270) = 0;
    TestFieldAt<std::int32_t>(panel, 0x260) = 17;
    TestFieldAt<std::int32_t>(panel, 0x274) = 3;
    std::strcpy(&TestFieldAt<char>(panel, 0x15c), "last");
    panel->vtbl = &g_HudUiPanel_FTable;
    const bool cached = panel->QueryTextHeight() == 14;
    const bool lastText = panel->GetLastTextPtr() == &TestFieldAt<char>(panel, 0x15c) &&
                          std::strcmp(panel->GetLastTextPtr(), "last") == 0;

    TestFieldAt<std::uint32_t>(panel, 0x0c) = 0;
    TestFieldAt<std::int32_t>(panel, 0x14) = 10;
    TestFieldAt<std::int32_t>(panel, 0x18) = 20;
    TestFieldAt<std::int32_t>(panel, 0x25c) = 30;
    HudUiRect textRect{-1, -1, -1, -1};
    panel->GetTextRect(&textRect);
    const bool textRectMatch =
        textRect.left == 10 && textRect.top == 20 && textRect.right == 40 && textRect.bottom == 34;
    const bool hitTest = panel->HitTest(10, 20) == 1 && panel->HitTest(39, 33) == 1 &&
                         panel->HitTest(40, 20) == 0 && panel->HitTest(10, 34) == 0 &&
                         panel->HitTest(9, 20) == 0 && panel->HitTest(10, 19) == 0;

    TestFieldAt<std::uint32_t>(panel, 0x0c) = 0x10;
    const bool hiddenMiss = panel->HitTest(10, 20) == 0;

    panel->ConstructorDefault("A", 0, 0);
    TestFieldAt<std::uint32_t>(panel, 0x270) = 1;
    TestFieldAt<std::int32_t>(panel, 0x274) = 2;
    const std::int32_t rebuiltHeight = panel->QueryTextHeight();
    const bool rebuilt = rebuiltHeight == TestFieldAt<std::int32_t>(panel, 0x260) -
                                              TestFieldAt<std::int32_t>(panel, 0x274) &&
                         rebuiltHeight > 0;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    return cached && lastText && textRectMatch && hitTest && hiddenMiss && rebuilt ? 0 : 1;
}

extern "C" int zhud_panel_simple_constructor_smoke(void) {
    HudUiPanelSimple simple{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&simple);

    simple.Constructor("ABC", 8, 9);
    const bool constructed =
        TestFieldAt<const void *>(&simple, 0x00) == &g_HudUiPanelSimple_FTable &&
        std::strcmp(&TestFieldAt<char>(&simple, 0x34), "ABC") == 0 &&
        TestFieldAt<std::int32_t>(&simple, 0x14) == 8 &&
        TestFieldAt<std::int32_t>(&simple, 0x18) == 9 &&
        TestFieldAt<std::uint32_t>(&simple, 0x14c) == 0x0020bf40 &&
        TestFieldAt<std::uint32_t>(&simple, 0x150) == 0x0020bf40 &&
        TestFieldAt<std::uint32_t>(&simple, 0x270) == 1 &&
        TestFieldAt<std::uint32_t>(&simple, 0x264) == 1 &&
        TestFieldAt<std::int32_t>(&simple, 0x29c) == -1 &&
        TestFieldAt<std::int32_t>(&simple, 0x2a0) == -1 && panel->hFont != nullptr;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;

    HudUiPanelSimple defaulted{};
    HudUiPanel *const defaultedPanel = reinterpret_cast<HudUiPanel *>(&defaulted);
    const bool defaultThunk =
        defaulted.ConstructorDefaultThunk() == &defaulted &&
        TestFieldAt<const void *>(&defaulted, 0x00) == &g_HudUiPanelSimple_FTable &&
        TestFieldAt<char>(&defaulted, 0x34) == 0 &&
        TestFieldAt<std::int32_t>(&defaulted, 0x14) == 0 &&
        TestFieldAt<std::int32_t>(&defaulted, 0x18) == 0;

    DeleteObject(defaultedPanel->hFont);
    defaultedPanel->hFont = nullptr;
    return constructed && defaultThunk ? 0 : 1;
}

extern "C" int zhud_timer_panel_set_time_smoke(void) {
    HudUiTimerPanel timer{};
    auto *panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);

    timer.SetTimeSeconds(1, 2, 3);
    const bool positive = std::strcmp(&TestFieldAt<char>(&timer, 0x34), "01:02:03") == 0;

    timer.SetTimeSeconds(-1, 2, 3);
    const bool fallback = std::strcmp(&TestFieldAt<char>(&timer, 0x34), "00:00:00") == 0;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    return positive && fallback ? 0 : 1;
}

extern "C" int zhud_timer_panel_update_hms_smoke(void) {
    HudUiTimerPanel timer{};
    auto *panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);

    timer.UpdateHMSFromSeconds(3661.9f);
    const bool positive = TestFieldAt<float>(&timer, 0x2a4) == 3661.9f &&
                          std::strcmp(&TestFieldAt<char>(&timer, 0x34), "01:01:01") == 0;

    timer.UpdateHMSFromSeconds(-1.0f);
    const bool fallback = TestFieldAt<float>(&timer, 0x2a4) == -1.0f &&
                          std::strcmp(&TestFieldAt<char>(&timer, 0x34), "00:00:00") == 0;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    return positive && fallback ? 0 : 1;
}

extern "C" int zhud_timer_panel_global_accessors_smoke(void) {
    HudUiTimerPanel timer{};
    auto *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;

    HudUiTimerPanel::SetRunning(0);
    const bool stopped = TestFieldAt<std::int32_t>(&timer, 0x2a8) == 1;
    HudUiTimerPanel::SetRunning(1);
    const bool running = TestFieldAt<std::int32_t>(&timer, 0x2a8) == 0;

    HudUiTimerPanel::SetElapsedSeconds(12.5f);
    const bool elapsed =
        TestFieldAt<float>(&timer, 0x2a4) == 12.5f && HudUiTimerPanel::GetSeconds() == 12.5f;

    HudUiTimerPanel::SetSeconds(65.0f, 3.9f);
    const bool seconds = TestFieldAt<float>(&timer, 0x2a4) == 65.0f &&
                         TestFieldAt<std::int32_t>(&timer, 0x2ac) == 3 &&
                         std::strcmp(&TestFieldAt<char>(&timer, 0x34), "00:01:05") == 0;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    return stopped && running && elapsed && seconds ? 0 : 1;
}

extern "C" int zhud_timer_panel_update_smoke(void) {
    std::int32_t networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_FrameDeltaTimeSec = 2.0f;
    g_Time_UnscaledDeltaTimeSec = 4.0f;

    HudUiCommon_FTable table = {};
    table.slots[1] = reinterpret_cast<std::uintptr_t>(TestElementDraw);

    HudUiTimerPanel timer{};
    auto *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    auto *const element = reinterpret_cast<HudUiElement *>(&timer);
    element->ftable = &table;
    element->flags = 0x02;

    TestFieldAt<float>(&timer, 0x2a4) = 10.0f;
    TestFieldAt<std::int32_t>(&timer, 0x2a8) = 0;
    TestFieldAt<std::int32_t>(&timer, 0x2ac) = 3;
    timer.Update(0.25f);
    const bool normal = TestFieldAt<float>(&timer, 0x2a4) == 16.0f &&
                        std::strcmp(&TestFieldAt<char>(&timer, 0x34), "00:00:16") == 0;

    networkEnabled = 1;
    timer.Update(0.25f);
    const bool network = TestFieldAt<float>(&timer, 0x2a4) == 28.0f &&
                         std::strcmp(&TestFieldAt<char>(&timer, 0x34), "00:00:28") == 0;

    TestFieldAt<std::int32_t>(&timer, 0x2a8) = 1;
    timer.Update(0.25f);
    const bool stopped = TestFieldAt<float>(&timer, 0x2a4) == 28.0f;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    ZOPT_NETWORK_ENABLED = nullptr;
    g_FrameDeltaTimeSec = 0.0f;
    g_Time_UnscaledDeltaTimeSec = 0.0f;
    return normal && network && stopped ? 0 : 1;
}

extern "C" int zhud_timer_panel_zar_read_smoke(void) {
    HudUiTimerPanel timer{};
    auto *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);

    g_HudUiMgrObjectiveChatComposeActive = 0;
    g_HudUiMgrObjectivePhase = 1;
    g_HudUiMgrObjectivePhaseDurationSec = 10.0f;
    g_HudUiMgrObjectivePhaseTimerSec = 2.0f;
    g_HudUiMgrObjectiveAutoHideDelaySec = 5.0f;

    const float payload = 7322.4f;
    HudUiTimerPanel::ZarReadTimerData(&payload, sizeof(payload), &timer);

    const bool timerUpdated = TestFieldAt<float>(&timer, 0x2a4) == payload &&
                              std::strcmp(&TestFieldAt<char>(&timer, 0x34), "02:02:02") == 0;
    const bool objectiveBegan = g_HudUiMgrObjectivePhase == 3 &&
                                g_HudUiMgrObjectivePhaseTimerSec == 8.0f &&
                                g_HudUiMgrObjectiveAutoHideDelaySec == 0.0f;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    return timerUpdated && objectiveBegan ? 0 : 1;
}

extern "C" int zhud_timer_panel_zar_write_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "hud", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    HudUiTimerPanel timer{};
    TestFieldAt<float>(&timer, 0x2a4) = 123.5f;

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;
    zZbdSectionHandler handler = {};
    handler.sectionName = "HUD";
    zZbdSectionCallbackCtx sectionCtx = {};
    sectionCtx.manager = &manager;
    sectionCtx.sectionHandler = &handler;

    HudUiTimerPanel::ZarWriteTimerDataCallback(&sectionCtx, &timer);

    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    float readBack = 0.0f;
    DWORD read = 0;
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool ok = manager.indexArchive.recordCount == 1 &&
                    manager.indexArchive.records != nullptr &&
                    manager.indexArchive.records[0].fileSize == sizeof(float) &&
                    std::strcmp(manager.indexArchive.records[0].name, "HUD/TimerData") == 0 &&
                    read == sizeof(readBack) && readBack == 123.5f;

    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return ok ? 0 : 1;
}

extern "C" int zhud_timer_and_counter_constructor_smoke(void) {
    g_HudUiMgr.ConstructorDefault();

    HudUiTimerPanel timer{};
    HudUiCounterTextPanel counter{};
    auto *timerPanel = reinterpret_cast<HudUiPanel *>(&timer);
    auto *counterPanel = reinterpret_cast<HudUiPanel *>(&counter);

    timer.ConstructorDefault();
    counter.Constructor();

    const bool timerOk = TestFieldAt<const void *>(&timer, 0x00) == &g_HudUiTimerPanel_FTable &&
                         std::strcmp(&TestFieldAt<char>(&timer, 0x34), "00:00:00") == 0 &&
                         TestFieldAt<float>(&timer, 0x2a4) == 0.0f &&
                         TestFieldAt<std::int32_t>(&timer, 0x2a8) == 1 &&
                         TestFieldAt<std::int32_t>(&timer, 0x2ac) == 1;

    const bool counterOk =
        TestFieldAt<const void *>(&counter, 0x00) == &g_HudUiCounterTextPanel_FTable &&
        std::strcmp(&TestFieldAt<char>(&counter, 0x34), "0") == 0;

    const bool linked = g_HudUiMgr.childHead == reinterpret_cast<HudUiElement *>(&timer) &&
                        g_HudUiMgr.childTail == reinterpret_cast<HudUiElement *>(&counter) &&
                        reinterpret_cast<HudUiElement *>(&timer)->next ==
                            reinterpret_cast<HudUiElement *>(&counter);

    DeleteObject(timerPanel->hFont);
    DeleteObject(counterPanel->hFont);
    timerPanel->hFont = nullptr;
    counterPanel->hFont = nullptr;
    return timerOk && counterOk && linked ? 0 : 1;
}

extern "C" int zhud_objective_refresh_counter_text_smoke(void) {
    g_HudUiMgr.ConstructorDefault();

    HudUiCounterTextPanel counter{};
    auto *panel = reinterpret_cast<HudUiPanel *>(&counter);
    auto *element = reinterpret_cast<HudUiElement *>(&counter);
    counter.Constructor();
    g_HudUiMgrObjectiveCounterTextPanel = &counter;

    HudUiMgrObjective::RefreshCounterText(12345);
    const bool positive = std::strcmp(&TestFieldAt<char>(&counter, 0x34), "12345") == 0 &&
                          std::strcmp(&TestFieldAt<char>(&counter, 0x15c), "12345") == 0 &&
                          TestFieldAt<std::uint32_t>(&counter, 0x270) == 1;

    HudUiMgrObjective::RefreshCounterText(-8);
    const bool negative = std::strcmp(&TestFieldAt<char>(&counter, 0x34), "-8") == 0 &&
                          std::strcmp(&TestFieldAt<char>(&counter, 0x15c), "-8") == 0 &&
                          element->clipRect.right >= element->clipRect.left &&
                          element->clipRect.bottom >= element->clipRect.top;

    g_HudUiMgrObjectiveCounterTextPanel = nullptr;
    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    return positive && negative ? 0 : 1;
}

extern "C" int zhud_objective_update_meter_xpoints_smoke(void) {
    const HudUiWidget oldWidget = g_HudUiMgrObjectiveWidget;
    const HudUiMeter oldMeter = g_HudUiMgrObjectiveMeter;

    HudUiWidget_FTable widgetTable = g_HudUiWidget_FTable;
    widgetTable.slots[0x64 / 4] = MethodAddress(&TestObjectiveMeterWidget::GetCenterX);

    g_HudUiMgrObjectiveWidget = {};
    g_HudUiMgrObjectiveWidget.ftable = &widgetTable;
    g_HudUiMgrObjectiveMeter = {};
    g_HudUiMgrObjectiveMeter.points[0].x = -1.0f;
    g_HudUiMgrObjectiveMeter.points[1].x = -2.0f;
    g_HudUiMgrObjectiveMeter.points[2].x = -3.0f;
    g_HudUiMgrObjectiveMeter.points[3].x = -4.0f;
    g_HudUiMgrObjectiveMeter.points[0].y = 10.0f;
    g_HudUiMgrObjectiveMeter.points[3].y = 40.0f;
    g_objectiveMeterGetCenterXCount = 0;
    g_objectiveMeterGetCenterXThis = nullptr;
    g_objectiveMeterCenterX = 123;

    HudUiMgrObjective::UpdateMeterXPoints();

    const bool centerRead = g_objectiveMeterGetCenterXCount == 1 &&
                            g_objectiveMeterGetCenterXThis == &g_HudUiMgrObjectiveWidget;
    const bool leftEdge = g_HudUiMgrObjectiveMeter.points[0].x == 128.0f &&
                          g_HudUiMgrObjectiveMeter.points[1].x == 128.0f;
    const bool rightEdge = g_HudUiMgrObjectiveMeter.points[2].x == 135.0f &&
                           g_HudUiMgrObjectiveMeter.points[3].x == 135.0f;
    const bool yUnchanged = g_HudUiMgrObjectiveMeter.points[0].y == 10.0f &&
                            g_HudUiMgrObjectiveMeter.points[3].y == 40.0f;

    g_HudUiMgrObjectiveWidget = oldWidget;
    g_HudUiMgrObjectiveMeter = oldMeter;
    return centerRead && leftEdge && rightEdge && yUnchanged ? 0 : 1;
}

extern "C" int zhud_layout_node_read_rect_offset_and_size_smoke(void) {
    zReader::Node rectItems[5] = {};
    rectItems[1].value.i32 = 10;
    rectItems[2].value.i32 = 20;
    rectItems[3].value.i32 = 30;
    rectItems[4].value.i32 = 40;

    zReader::Node rectNode{};
    rectNode.type = zReader::ZRDR_NODE_ARRAY;
    rectNode.value.nodes = rectItems;

    HudUiRect shifted{};
    int width = -1;
    int height = -1;
    const int offsetXY[2] = {3, -4};
    const int shiftedResult =
        HudUiLayoutNode::ReadRectOffsetAndSize(&rectNode, &shifted, offsetXY, &width, &height);
    const bool shiftedOk = shiftedResult == 1 && shifted.left == 13 && shifted.top == 16 &&
                           shifted.right == 33 && shifted.bottom == 36 && width == 20 &&
                           height == 20;

    HudUiRect unshifted{};
    width = -1;
    const int unshiftedResult =
        HudUiLayoutNode::ReadRectOffsetAndSize(&rectNode, &unshifted, nullptr, &width, nullptr);
    const bool unshiftedOk = unshiftedResult == 1 && unshifted.left == 10 &&
                             unshifted.top == 20 && unshifted.right == 30 &&
                             unshifted.bottom == 40 && width == 20;

    zReader::Node scalarNode{};
    scalarNode.type = zReader::ZRDR_NODE_INT;
    HudUiRect rejected = {1, 2, 3, 4};
    width = 77;
    height = 88;
    const int rejectedResult =
        HudUiLayoutNode::ReadRectOffsetAndSize(&scalarNode, &rejected, offsetXY, &width, &height);
    const bool rejectedOk = rejectedResult == 0 && rejected.left == 1 && rejected.top == 2 &&
                            rejected.right == 3 && rejected.bottom == 4 && width == 77 &&
                            height == 88;

    return shiftedOk && unshiftedOk && rejectedOk ? 0 : 1;
}

extern "C" int zhud_layout_node_read_rect_smoke(void) {
    zReader::Node rectItems[5] = {};
    rectItems[1].value.i32 = 4;
    rectItems[2].value.i32 = 40;
    rectItems[3].value.i32 = 8;
    rectItems[4].value.i32 = 80;

    zReader::Node rectNode{};
    rectNode.type = zReader::ZRDR_NODE_ARRAY;
    rectNode.value.nodes = rectItems;

    HudUiRect rect{};
    const int result = HudUiLayoutNode::ReadRect(&rectNode, &rect);
    const bool loaded = result == 1 && rect.left == 4 && rect.right == 40 &&
                        rect.top == 8 && rect.bottom == 80;

    zReader::Node scalarNode{};
    scalarNode.type = zReader::ZRDR_NODE_INT;
    rect = {1, 2, 3, 4};
    const int rejectedResult = HudUiLayoutNode::ReadRect(&scalarNode, &rect);
    const bool rejected = rejectedResult == 0 && rect.left == 1 && rect.top == 2 &&
                          rect.right == 3 && rect.bottom == 4;

    return loaded && rejected ? 0 : 1;
}

extern "C" int zhud_layout_node_read_int3_smoke(void) {
    zReader::Node intItems[4] = {};
    intItems[1].value.i32 = 5;
    intItems[2].value.i32 = 6;
    intItems[3].value.i32 = 7;

    zReader::Node intNode{};
    intNode.type = zReader::ZRDR_NODE_ARRAY;
    intNode.value.nodes = intItems;

    int a = 0;
    int b = 0;
    int c = 0;
    const int allResult = HudUiLayoutNode::ReadInt3(&intNode, &a, &b, &c);
    const bool allLoaded = allResult == 1 && a == 5 && b == 6 && c == 7;

    a = 10;
    b = 20;
    c = 30;
    const int partialResult = HudUiLayoutNode::ReadInt3(&intNode, nullptr, &b, nullptr);
    const bool partialLoaded = partialResult == 1 && a == 10 && b == 6 && c == 30;

    zReader::Node scalarNode{};
    scalarNode.type = zReader::ZRDR_NODE_INT;
    a = 11;
    b = 22;
    c = 33;
    const int rejectedResult = HudUiLayoutNode::ReadInt3(&scalarNode, &a, &b, &c);
    const bool rejected = rejectedResult == 0 && a == 11 && b == 22 && c == 33;

    return allLoaded && partialLoaded && rejected ? 0 : 1;
}

extern "C" int zhud_layout_node_apply_text_label_smoke(void) {
    HudUiPanel_FTable table{};
    table.slots[0x0c / 4] = MethodAddress(&TestApplyTextLabelPanel::SetPos);
    table.slots[0x74 / 4] = reinterpret_cast<std::uintptr_t>(&TestApplyTextLabelSetTextFmt);

    HudUiPanel panel{};
    panel.vtbl = &table;
    const int offsetXY[2] = {7, -4};

    g_applyTextLabelSetPosCount = 0;
    g_applyTextLabelSetTextFmtCount = 0;
    for (int index = 0; index < 4; ++index) {
        g_applyTextLabelSetPosThis[index] = nullptr;
        g_applyTextLabelSetPosX[index] = 0;
        g_applyTextLabelSetPosY[index] = 0;
        g_applyTextLabelSetTextFmtThis[index] = nullptr;
        g_applyTextLabelSetTextFmtFormat[index] = nullptr;
    }

    zReader::Node textItems[4] = {};
    textItems[1].type = zReader::ZRDR_NODE_STRING;
    textItems[1].value.str = const_cast<char *>("Shield");
    textItems[2].value.i32 = 20;
    textItems[3].value.i32 = 30;

    zReader::Node textNode{};
    textNode.type = zReader::ZRDR_NODE_ARRAY;
    textNode.value.nodes = textItems;

    const int textResult = HudUiLayoutNode::ApplyTextLabel(&textNode, &panel, 100, 200, offsetXY);
    const bool textPath =
        textResult == 1 && g_applyTextLabelSetPosCount == 1 &&
        g_applyTextLabelSetPosThis[0] == &panel && g_applyTextLabelSetPosX[0] == 127 &&
        g_applyTextLabelSetPosY[0] == 226 && g_applyTextLabelSetTextFmtCount == 1 &&
        g_applyTextLabelSetTextFmtThis[0] == &panel &&
        std::strcmp(g_applyTextLabelSetTextFmtFormat[0], "Shield") == 0;

    textItems[1].value.str = nullptr;
    textItems[2].value.i32 = 1;
    textItems[3].value.i32 = 2;
    const int nullResult = HudUiLayoutNode::ApplyTextLabel(&textNode, &panel, 10, 15, nullptr);
    const bool nullText =
        nullResult == 1 && g_applyTextLabelSetPosCount == 2 &&
        g_applyTextLabelSetPosThis[1] == &panel && g_applyTextLabelSetPosX[1] == 11 &&
        g_applyTextLabelSetPosY[1] == 17 && g_applyTextLabelSetTextFmtCount == 2 &&
        g_applyTextLabelSetTextFmtThis[1] == &panel &&
        std::strcmp(g_applyTextLabelSetTextFmtFormat[1], "") == 0;

    zReader::Node scalarNode{};
    scalarNode.type = zReader::ZRDR_NODE_INT;
    const int rejectedResult =
        HudUiLayoutNode::ApplyTextLabel(&scalarNode, &panel, 1000, 2000, offsetXY);
    const bool rejected = rejectedResult == 0 && g_applyTextLabelSetPosCount == 2 &&
                          g_applyTextLabelSetTextFmtCount == 2;

    return textPath && nullText && rejected ? 0 : 1;
}

extern "C" int zhud_layout_node_apply_image_widget_smoke(void) {
    zReader::Node imageItems[6] = {};
    imageItems[0].value.i32 = 6;
    imageItems[1].type = zReader::ZRDR_NODE_STRING;
    imageItems[1].value.str = const_cast<char *>("borrowed-image");
    imageItems[2].value.i32 = 20;
    imageItems[3].value.i32 = 30;
    imageItems[4].type = zReader::ZRDR_NODE_STRING;
    imageItems[4].value.str = const_cast<char *>("TRUE");
    imageItems[5].type = zReader::ZRDR_NODE_STRING;
    imageItems[5].value.str = const_cast<char *>("TRUE");

    zReader::Node imageNode{};
    imageNode.type = zReader::ZRDR_NODE_ARRAY;
    imageNode.value.nodes = imageItems;

    zVidImagePartial image{};
    image.width = 10;
    image.height = 6;

    HudUiWidget widget{};
    widget.ftable = &g_HudUiWidget_FTable;
    widget.ownsImage = 1;
    widget.imageStateWord = 0xabcd1234;
    const int anchor[2] = {7, 9};
    HudUiRect outRect{};
    g_HudUi_InvalidateMask = 0x80;

    zVidImagePartial *const result =
        HudUiLayoutNode::ApplyImageWidget(&imageNode, &widget, 100, 200, anchor, &image, &outRect);

    const bool centeredBorrowed =
        result == &image && widget.image == &image && widget.ownsImage == 0 &&
        widget.x == 122 && widget.y == 236 && widget.imageStateWord == 0xabcd0001 &&
        (widget.flags & 0x80) != 0 && outRect.left == 122 && outRect.top == 236 &&
        outRect.right == 132 && outRect.bottom == 242;

    zReader::Node simpleItems[4] = {};
    simpleItems[0].value.i32 = 4;
    simpleItems[1].type = zReader::ZRDR_NODE_STRING;
    simpleItems[1].value.str = const_cast<char *>("simple-borrowed-image");
    simpleItems[2].value.i32 = -3;
    simpleItems[3].value.i32 = 5;

    zReader::Node simpleNode{};
    simpleNode.type = zReader::ZRDR_NODE_ARRAY;
    simpleNode.value.nodes = simpleItems;

    zVidImagePartial simpleImage{};
    simpleImage.width = 9;
    simpleImage.height = 7;
    HudUiWidget simpleWidget{};
    simpleWidget.ftable = &g_HudUiWidget_FTable;
    simpleWidget.imageStateWord = 0x5678ffff;
    HudUiRect simpleRect{};

    zVidImagePartial *const simpleResult = HudUiLayoutNode::ApplyImageWidget(
        &simpleNode, &simpleWidget, 10, 15, nullptr, &simpleImage, &simpleRect);

    const bool defaultFlags =
        simpleResult == &simpleImage && simpleWidget.x == 7 && simpleWidget.y == 20 &&
        simpleWidget.imageStateWord == 0x56780000 && simpleRect.left == 7 &&
        simpleRect.top == 20 && simpleRect.right == 16 && simpleRect.bottom == 27;

    zReader::Node scalarNode{};
    scalarNode.type = zReader::ZRDR_NODE_INT;
    HudUiWidget rejectedWidget{};
    rejectedWidget.ftable = &g_HudUiWidget_FTable;
    rejectedWidget.x = 1;
    rejectedWidget.y = 2;
    rejectedWidget.imageStateWord = 0x11112222;
    HudUiRect rejectedRect = {3, 4, 5, 6};
    zVidImagePartial *const rejectedResult = HudUiLayoutNode::ApplyImageWidget(
        &scalarNode, &rejectedWidget, 10, 20, anchor, &image, &rejectedRect);

    const bool rejected = rejectedResult == nullptr && rejectedWidget.x == 1 &&
                          rejectedWidget.y == 2 &&
                          rejectedWidget.imageStateWord == 0x11112222 &&
                          rejectedRect.left == 3 && rejectedRect.top == 4 &&
                          rejectedRect.right == 5 && rejectedRect.bottom == 6;

    g_HudUi_InvalidateMask = 0;
    return centeredBorrowed && defaultFlags && rejected ? 0 : 1;
}

extern "C" int zhud_layout_base_load_type_i_from_zar_root_smoke(void) {
    zReader::Node rectItems[5] = {};
    rectItems[1].value.i32 = 7;
    rectItems[2].value.i32 = 11;
    rectItems[3].value.i32 = 107;
    rectItems[4].value.i32 = 211;

    zReader::Node typeIPayload[2] = {};
    typeIPayload[1].type = zReader::ZRDR_NODE_ARRAY;
    typeIPayload[1].value.nodes = rectItems;

    zReader::Node rootItems[3] = {};
    rootItems[0].value.i32 = 3;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("TYPEI");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = typeIPayload;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    HudLayoutBase layout{};
    layout.layoutRect = {1, 2, 3, 4};
    layout.activeRect = {5, 6, 7, 8};
    layout.LoadTypeIFromZarRoot(&root);

    const bool loaded = layout.layoutRect.left == 7 && layout.layoutRect.top == 11 &&
                        layout.layoutRect.right == 107 && layout.layoutRect.bottom == 211 &&
                        layout.activeRect.left == 7 && layout.activeRect.top == 11 &&
                        layout.activeRect.right == 107 && layout.activeRect.bottom == 211;

    zReader::Node missingItems[1] = {};
    missingItems[0].value.i32 = 1;
    zReader::Node missingRoot{};
    missingRoot.type = zReader::ZRDR_NODE_ARRAY;
    missingRoot.value.nodes = missingItems;

    layout.layoutRect = {10, 20, 30, 40};
    layout.activeRect = {50, 60, 70, 80};
    layout.LoadTypeIFromZarRoot(&missingRoot);

    const bool missingPreserves = layout.layoutRect.left == 10 && layout.layoutRect.top == 20 &&
                                  layout.layoutRect.right == 30 &&
                                  layout.layoutRect.bottom == 40 &&
                                  layout.activeRect.left == 50 && layout.activeRect.top == 60 &&
                                  layout.activeRect.right == 70 &&
                                  layout.activeRect.bottom == 80;

    return loaded && missingPreserves ? 0 : 1;
}

extern "C" int zhud_layout_hw_load_type_ii_from_zar_root_smoke(void) {
    zReader::Node rectItems[5] = {};
    rectItems[1].value.i32 = 12;
    rectItems[2].value.i32 = 24;
    rectItems[3].value.i32 = 320;
    rectItems[4].value.i32 = 240;

    zReader::Node widget1Items[4] = {};
    widget1Items[0].value.i32 = 4;
    widget1Items[1].type = zReader::ZRDR_NODE_STRING;
    widget1Items[1].value.str = const_cast<char *>("__recoil_missing_typeii_widget1__");
    widget1Items[2].value.i32 = 3;
    widget1Items[3].value.i32 = 5;

    zReader::Node widget3Items[4] = {};
    widget3Items[0].value.i32 = 4;
    widget3Items[1].type = zReader::ZRDR_NODE_STRING;
    widget3Items[1].value.str = const_cast<char *>("__recoil_missing_typeii_widget3__");
    widget3Items[2].value.i32 = 7;
    widget3Items[3].value.i32 = 11;

    zReader::Node widget2Items[4] = {};
    widget2Items[0].value.i32 = 4;
    widget2Items[1].type = zReader::ZRDR_NODE_STRING;
    widget2Items[1].value.str = const_cast<char *>("__recoil_missing_typeii_widget2__");
    widget2Items[2].value.i32 = 13;
    widget2Items[3].value.i32 = 17;

    zReader::Node imageNameItems[5] = {};
    imageNameItems[1].value.str = const_cast<char *>("__recoil_missing_widget1_320__");
    imageNameItems[2].value.str = const_cast<char *>("__recoil_missing_widget1_400__");
    imageNameItems[3].value.str = const_cast<char *>("__recoil_missing_widget2_320__");
    imageNameItems[4].value.str = const_cast<char *>("__recoil_missing_widget2_400__");

    zReader::Node typeIIPayload[6] = {};
    typeIIPayload[1].type = zReader::ZRDR_NODE_ARRAY;
    typeIIPayload[1].value.nodes = rectItems;
    typeIIPayload[2].type = zReader::ZRDR_NODE_ARRAY;
    typeIIPayload[2].value.nodes = widget1Items;
    typeIIPayload[3].type = zReader::ZRDR_NODE_ARRAY;
    typeIIPayload[3].value.nodes = widget3Items;
    typeIIPayload[4].type = zReader::ZRDR_NODE_ARRAY;
    typeIIPayload[4].value.nodes = widget2Items;
    typeIIPayload[5].type = zReader::ZRDR_NODE_ARRAY;
    typeIIPayload[5].value.nodes = imageNameItems;

    zReader::Node rootItems[3] = {};
    rootItems[0].value.i32 = 3;
    rootItems[1].type = zReader::ZRDR_NODE_STRING;
    rootItems[1].value.str = const_cast<char *>("TYPEII");
    rootItems[2].type = zReader::ZRDR_NODE_ARRAY;
    rootItems[2].value.nodes = typeIIPayload;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootItems;

    HudLayoutHW layout{};
    auto *layoutBase = reinterpret_cast<HudLayoutBase *>(&layout);
    HudUiWidget *const widget1 = &TestFieldAt<HudUiWidget>(&layout, 0xec);
    HudUiWidget *const widget2 = &TestFieldAt<HudUiWidget>(&layout, 0x1b4);
    HudUiWidget *const widget3 = &TestFieldAt<HudUiWidget>(&layout, 0x27c);
    widget1->ftable = &g_HudUiWidget_FTable;
    widget2->ftable = &g_HudUiWidget_FTable;
    widget3->ftable = &g_HudUiWidget_FTable;
    widget1->image = &zVid_Image::g_zImage_DefaultImage;
    widget2->image = &zVid_Image::g_zImage_DefaultImage;
    widget3->image = &zVid_Image::g_zImage_DefaultImage;

    g_HudUiMgrHudOriginY = 100;
    g_HudUi_InvalidateMask = 0x80;
    const int loadedResult = layout.LoadTypeIIFromZarRoot(&root);

    const bool rectsLoaded = loadedResult == 1 && layoutBase->layoutRect.left == 12 &&
                             layoutBase->layoutRect.top == 24 &&
                             layoutBase->layoutRect.right == 320 &&
                             layoutBase->layoutRect.bottom == 240 &&
                             layoutBase->activeRect.left == 12 &&
                             layoutBase->activeRect.top == 24 &&
                             layoutBase->activeRect.right == 320 &&
                             layoutBase->activeRect.bottom == 240;
    const bool missingImagesCleared =
        widget1->image == nullptr && widget2->image == nullptr && widget3->image == nullptr &&
        layout.widget1ImageDefault == nullptr && layout.widget1Image320 == nullptr &&
        layout.widget1Image400 == nullptr && layout.widget2ImageDefault == nullptr &&
        layout.widget2Image320 == nullptr && layout.widget2Image400 == nullptr &&
        (widget1->flags & 0x80) != 0 && (widget2->flags & 0x80) != 0 &&
        (widget3->flags & 0x80) != 0;

    zReader::Node missingItems[1] = {};
    missingItems[0].value.i32 = 1;
    zReader::Node missingRoot{};
    missingRoot.type = zReader::ZRDR_NODE_ARRAY;
    missingRoot.value.nodes = missingItems;
    layoutBase->layoutRect = {1, 2, 3, 4};
    layoutBase->activeRect = {5, 6, 7, 8};
    const int missingResult = layout.LoadTypeIIFromZarRoot(&missingRoot);
    const bool missingPreserves = missingResult == 1 && layoutBase->layoutRect.left == 1 &&
                                  layoutBase->layoutRect.top == 2 &&
                                  layoutBase->layoutRect.right == 3 &&
                                  layoutBase->layoutRect.bottom == 4 &&
                                  layoutBase->activeRect.left == 5 &&
                                  layoutBase->activeRect.top == 6 &&
                                  layoutBase->activeRect.right == 7 &&
                                  layoutBase->activeRect.bottom == 8;

    g_HudUiMgrHudOriginY = 0;
    g_HudUi_InvalidateMask = 0;
    return rectsLoaded && missingImagesCleared && missingPreserves ? 0 : 1;
}

extern "C" int zhud_timer_panel_float_constructor_smoke(void) {
    HudUiTimerPanelFloat timer{};
    auto *panel = reinterpret_cast<HudUiPanel *>(&timer);

    timer.ConstructorDefault();

    const bool updated =
        TestFieldAt<const void *>(&timer, 0x00) == &g_HudUiTimerPanelFloat_FTable &&
        TestFieldAt<std::int32_t>(&timer, 0x14) == 3 &&
        TestFieldAt<std::int32_t>(&timer, 0x18) == 0x1c &&
        TestFieldAt<std::int32_t>(&timer, 0x20) == 3 &&
        TestFieldAt<std::int32_t>(&timer, 0x24) == 0x1c &&
        TestFieldAt<std::int32_t>(&timer, 0x28) == 0x3f &&
        TestFieldAt<std::int32_t>(&timer, 0x2c) == 0x2b &&
        (TestFieldAt<std::uint32_t>(&timer, 0x0c) & 0x10) != 0 &&
        TestFieldAt<std::uint32_t>(&timer, 0x14c) == 0x0020bf40 &&
        TestFieldAt<std::uint32_t>(&timer, 0x150) == 0x0020bf40 &&
        TestFieldAt<std::uint32_t>(&timer, 0x264) == 1 &&
        TestFieldAt<std::int32_t>(&timer, 0x29c) == -1 &&
        TestFieldAt<std::int32_t>(&timer, 0x2a0) == -1 &&
        TestFieldAt<float>(&timer, 0x2a4) == 0.0f && TestFieldAt<float>(&timer, 0x2a8) == 0.0f &&
        TestFieldAt<float>(&timer, 0x2ac) == 0.0f;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    return updated ? 0 : 1;
}

extern "C" int zhud_triplet_constructor_smoke(void) {
    HudUiTriplet triplet{};
    triplet.Constructor();

    const bool headers =
        triplet.headerPanels[0] != nullptr && triplet.headerPanels[1] != nullptr &&
        triplet.headerPanels[2] != nullptr &&
        std::strcmp(&TestFieldAt<char>(triplet.headerPanels[0], 0x34), "Player") == 0 &&
        std::strcmp(&TestFieldAt<char>(triplet.headerPanels[1], 0x34), "Laps") == 0 &&
        std::strcmp(&TestFieldAt<char>(triplet.headerPanels[2], 0x34), "Kills") == 0 &&
        TestFieldAt<std::int32_t>(triplet.headerPanels[0], 0x144) == 2 &&
        TestFieldAt<std::int32_t>(triplet.headerPanels[1], 0x144) == 1 &&
        TestFieldAt<std::int32_t>(triplet.headerPanels[2], 0x144) == 1;

    bool rows = true;
    for (int i = 0; i < 24; ++i) {
        rows = rows && triplet.rowCells[i] != nullptr;
    }

    const bool linked =
        triplet.base.enabled == 1 &&
        triplet.base.childHead == reinterpret_cast<HudUiElement *>(triplet.headerPanels[0]) &&
        triplet.base.childTail == reinterpret_cast<HudUiElement *>(triplet.rowCells[23]) &&
        triplet.lapsColumnOffsetX == 0x23 && triplet.killsColumnOffsetX == 0x46 &&
        triplet.fontSize == 8 && triplet.fontWeight == 6;

    for (HudUiPanel *header : triplet.headerPanels) {
        DeletePanelAllocation(header);
    }

    for (HudUiPanel *rowCell : triplet.rowCells) {
        DeletePanelAllocation(rowCell);
    }

    return headers && rows && linked ? 0 : 1;
}

extern "C" int zhud_triplet_interpolate_layout_smoke(void) {
    HudUiTriplet triplet{};
    triplet.baseXStart = 10;
    triplet.baseXEnd = 22;
    triplet.baseYStart = 50;
    triplet.baseYEnd = 30;
    triplet.rowPitchYStart = -10;
    triplet.rowPitchYEnd = 10;
    triplet.lapsColumnOffsetXStart = 3;
    triplet.lapsColumnOffsetXEnd = 13;
    triplet.killsColumnOffsetXStart = 20;
    triplet.killsColumnOffsetXEnd = -10;
    triplet.fontSizeStart = 9;
    triplet.fontSizeEnd = 12;
    triplet.fontWeightStart = 500;
    triplet.fontWeightEnd = 299;

    triplet.InterpolateLayout(0.25f);

    const bool quarter = triplet.baseX == 13 && triplet.baseY == 45 &&
                         triplet.rowPitchY == -5 && triplet.lapsColumnOffsetX == 5 &&
                         triplet.killsColumnOffsetX == 12 && triplet.fontSize == 9 &&
                         triplet.fontWeight == 449;

    triplet.InterpolateLayout(1.25f);

    const bool beyondEnd = triplet.baseX == 25 && triplet.baseY == 25 &&
                           triplet.rowPitchY == 15 && triplet.lapsColumnOffsetX == 15 &&
                           triplet.killsColumnOffsetX == -17 && triplet.fontSize == 12 &&
                           triplet.fontWeight == 248;

    return quarter && beyondEnd ? 0 : 1;
}

extern "C" int zhud_triplet_destructor_core_smoke(void) {
    HudUiTriplet triplet{};
    triplet.Constructor();
    triplet.entries.begin =
        static_cast<HudUiScoreboardEntry *>(::operator new(sizeof(HudUiScoreboardEntry)));
    triplet.entries.end = triplet.entries.begin + 1;
    triplet.entries.cap = triplet.entries.begin + 1;

    triplet.DestructorCore();

    bool cleared = triplet.base.vptr == &g_HudUiContainer_FTable &&
                   triplet.entries.begin == nullptr && triplet.entries.end == nullptr &&
                   triplet.entries.cap == nullptr;

    for (HudUiPanel *header : triplet.headerPanels) {
        cleared = cleared && header == nullptr;
    }

    for (HudUiPanel *rowCell : triplet.rowCells) {
        cleared = cleared && rowCell == nullptr;
    }

    return cleared ? 0 : 1;
}

extern "C" int zhud_nanite_panel_init_layout_smoke(void) {
    zReader::Node clipItems[5] = {};
    clipItems[1].value.i32 = 8;
    clipItems[2].value.i32 = 12;
    clipItems[3].value.i32 = 30;
    clipItems[4].value.i32 = 40;

    zReader::Node widgetItems[4] = {};
    widgetItems[0].value.i32 = 4;
    widgetItems[1].type = zReader::ZRDR_NODE_STRING;
    widgetItems[1].value.str = nullptr;

    zReader::Node layoutPayload[5] = {};
    layoutPayload[1].type = zReader::ZRDR_NODE_ARRAY;
    layoutPayload[1].value.nodes = clipItems;
    layoutPayload[2].type = zReader::ZRDR_NODE_ARRAY;
    layoutPayload[2].value.nodes = widgetItems;
    layoutPayload[3].type = zReader::ZRDR_NODE_ARRAY;
    layoutPayload[3].value.nodes = widgetItems;
    layoutPayload[4].type = zReader::ZRDR_NODE_ARRAY;
    layoutPayload[4].value.nodes = widgetItems;

    zReader::Node root{};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = layoutPayload;

    HudUiWidget_FTable widgetTable = g_HudUiWidget_FTable;
    widgetTable.slots[0x64 / 4] = MethodAddress(&TestNaniteInitWidget::GetCenterX);
    widgetTable.slots[0x68 / 4] = MethodAddress(&TestNaniteInitWidget::GetCenterY);

    HudUiCommon_FTable panelTable{};
    panelTable.slots[3] = MethodAddress(&TestNaniteInitPanel::SetPos);
    panelTable.slots[6] = MethodAddress(&TestNaniteInitPanel::SetBltSourceAndClipRect);

    HudUiWidget *const layoutWidget2 = &TestFieldAt<HudUiWidget>(&g_HudLayoutHW, 0x1b4);
    const HudUiWidget_FTable *const oldLayoutWidget2FTable = layoutWidget2->ftable;
    const int oldHudOriginX = g_HudUiMgrHudOriginX;

    HudUiNanitePanel panel{};
    panel.base.ftable = &panelTable;
    for (int index = 0; index < 3; ++index) {
        panel.items[index].ftable = &g_HudUiWidget_FTable;
    }
    panel.items[2].ftable = &widgetTable;

    g_naniteInitLayoutWidget2 = layoutWidget2;
    g_naniteInitGetCenterXCount = 0;
    g_naniteInitGetCenterYCount = 0;
    g_naniteInitPanelSetPosCount = 0;
    g_naniteInitPanelSetClipCount = 0;
    g_naniteInitPanelSetPosThis = nullptr;
    g_naniteInitPanelSetClipThis = nullptr;
    g_naniteInitPanelBltSource = reinterpret_cast<void *>(1);
    g_naniteInitPanelClip = {};

    layoutWidget2->ftable = &widgetTable;
    g_HudUiMgrHudOriginX = 101;
    panel.InitLayout(&root);

    layoutWidget2->ftable = oldLayoutWidget2FTable;
    g_HudUiMgrHudOriginX = oldHudOriginX;

    const bool centersRead =
        g_naniteInitGetCenterXCount == 2 && g_naniteInitGetCenterYCount == 2;
    const bool positioned = g_naniteInitPanelSetPosCount == 1 &&
                            g_naniteInitPanelSetPosThis == &panel &&
                            g_naniteInitPanelSetPosX == 33 &&
                            g_naniteInitPanelSetPosY == 44;
    const bool clipped = g_naniteInitPanelSetClipCount == 1 &&
                         g_naniteInitPanelSetClipThis == &panel &&
                         g_naniteInitPanelBltSource == nullptr &&
                         g_naniteInitPanelClip.left == 58 &&
                         g_naniteInitPanelClip.top == 12 &&
                         g_naniteInitPanelClip.right == 80 &&
                         g_naniteInitPanelClip.bottom == 40;

    return centersRead && positioned && clipped ? 0 : 1;
}

extern "C" int zhud_triplet_scoreboard_entry_update_smoke(void) {
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    GameNetPlayerRow alpha{};
    alpha.playerKey = 101;
    alpha.playerColorPackedRgb = 0x00112233;
    std::strcpy(alpha.displayName, "Alpha");

    GameNetPlayerRow bravo{};
    bravo.playerKey = 202;
    bravo.playerColorPackedRgb = 0x00445566;
    std::strcpy(bravo.displayName, "Bravo");

    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 5;
    GameNet::RefreshPlayerListMenu(&alpha);
    GameNet::RefreshPlayerListMenu(&bravo);

    alpha.score = 9;
    alpha.lapCount = 3;
    alpha.playerColorPackedRgb = 0x00778899;
    HudUi::RefreshScoreboardEntryRow(&alpha);

    HudUiPanel *const row0Name = triplet.rowCells[0];
    HudUiPanel *const row0Score = triplet.rowCells[1];
    HudUiPanel *const row0Kills = triplet.rowCells[2];
    int scoreModeFail = 0;
    if (HudUiTripletEntryCountForTest(triplet) != 2) {
        scoreModeFail = 21;
    } else if (triplet.entries.begin[0].playerKey != alpha.playerKey) {
        scoreModeFail = 22;
    } else if (std::strcmp(row0Name->GetLastTextPtr(), "Alpha") != 0) {
        scoreModeFail = 23;
    } else if (std::strcmp(row0Score->GetLastTextPtr(), "9") != 0) {
        scoreModeFail = 24;
    } else if ((reinterpret_cast<HudUiElement *>(row0Kills)->flags & 0x10u) == 0) {
        scoreModeFail = 25;
    } else if (TestFieldAt<std::uint32_t>(row0Name, 0x14c) != 0x00778899) {
        scoreModeFail = 26;
    }
    const bool scoreMode = scoreModeFail == 0;

    g_HudSensorTracker.raceCheckpointMode = 1;
    alpha.score = 1;
    alpha.lapCount = 1;
    HudUi::RefreshScoreboardEntryRow(&alpha);
    bravo.score = 5;
    bravo.lapCount = 4;
    bravo.playerColorPackedRgb = 0x00010203;
    HudUi::RefreshScoreboardEntryRow(&bravo);

    int lapModeFail = 0;
    if (triplet.entries.begin[0].playerKey != bravo.playerKey) {
        lapModeFail = 31;
    } else if (std::strcmp(row0Name->GetLastTextPtr(), "Bravo") != 0) {
        lapModeFail = 32;
    } else if (std::strcmp(row0Score->GetLastTextPtr(), "4") != 0) {
        lapModeFail = 33;
    } else if (std::strcmp(row0Kills->GetLastTextPtr(), "5") != 0) {
        lapModeFail = 34;
    } else if ((reinterpret_cast<HudUiElement *>(row0Kills)->flags & 0x10u) != 0) {
        lapModeFail = 35;
    } else if (TestFieldAt<std::uint32_t>(row0Name, 0x14c) != 0x00010203) {
        lapModeFail = 36;
    }
    const bool lapMode = lapModeFail == 0;

    HudUiTriplet largeTriplet{};
    largeTriplet.Constructor();
    GameNetPlayerRow largeRows[18]{};
    for (int index = 0; index < 18; ++index) {
        GameNetPlayerRow &row = largeRows[index];
        row.playerKey = 1000 + ((index * 7) % 18);
        row.playerColorPackedRgb = 0x00010101u * static_cast<unsigned int>(index + 1);
        std::sprintf(row.displayName, "Large%02d", row.playerKey - 1000);
        largeTriplet.AddEntry(&row);
    }

    int largeSortFail = 0;
    if (HudUiTripletEntryCountForTest(largeTriplet) != 18) {
        largeSortFail = 41;
    } else {
        for (int index = 0; index < 18; ++index) {
            if (largeTriplet.entries.begin[index].playerKey != 1017 - index) {
                largeSortFail = 42;
                break;
            }
        }
    }

    if (largeSortFail == 0 &&
        std::strcmp(largeTriplet.rowCells[0]->GetLastTextPtr(), "Large17") != 0) {
        largeSortFail = 43;
    }

    const bool largeSort = largeSortFail == 0;
    largeTriplet.DestructorCore();

    HudUi::RemoveScoreboardEntryRow(&bravo);
    const bool removed = HudUiTripletEntryCountForTest(triplet) == 1 &&
                         triplet.entries.begin[0].playerKey == alpha.playerKey;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    triplet.DestructorCore();

    if (!scoreMode) {
        return scoreModeFail;
    }
    if (!lapMode) {
        return lapModeFail;
    }
    if (!largeSort) {
        return largeSortFail;
    }
    return scoreMode && lapMode && largeSort && removed ? 0 : 1;
}

extern "C" int zhud_stats_list_destructors_smoke(void) {
    auto *const triplet = static_cast<HudUiTriplet *>(::operator new(sizeof(HudUiTriplet)));
    triplet->Constructor();

    HudUiStatsListElement stats{};
    stats.triplet = triplet;
    stats.base.ftable = nullptr;
    stats.DestructorCore();

    const bool core = stats.triplet == nullptr && stats.base.ftable == &g_HudUiCommon_FTable;

    stats.base.ftable = nullptr;
    HudUiStatsListElement *const returned = stats.ScalarDeletingDestructor(0);
    const bool scalar = returned == &stats && stats.base.ftable == &g_HudUiCommon_FTable;

    return core && scalar ? 0 : 1;
}

extern "C" int zhud_string_menu_destructor_core_smoke(void) {
    HudUiStringMenu menu{};
    menu.base.ConstructorDefault();

    for (HudUiPanelSimple &item : menu.items) {
        item.ConstructorDefaultThunk();
    }

    menu.base.vptr = reinterpret_cast<const HudUiContainer_FTable *>(&g_HudUiStringMenu_FTable);
    menu.DestructorCore();

    bool panelsDestroyed = menu.base.vptr == &g_HudUiContainer_FTable;
    for (HudUiPanelSimple &item : menu.items) {
        panelsDestroyed =
            panelsDestroyed && TestFieldAt<const void *>(&item, 0x00) == &g_HudUiCommon_FTable;
    }

    return panelsDestroyed ? 0 : 1;
}

extern "C" int zhud_text_stack_constructors_smoke(void) {
    HudUiTopMessageStack top{};
    top.Constructor();

    const std::int32_t topY[4] = {0x1e, 0x30, 0x42, 0x54};
    bool topLines = top.base.vptr ==
                    reinterpret_cast<const HudUiContainer_FTable *>(&g_HudUiTopMessageStack_FTable);
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAt(&top, index);
        topLines = topLines && TestFieldAt<const void *>(panel, 0x00) == &g_HudUiPanel_FTable &&
                   TestFieldAt<std::int32_t>(panel, 0x14) == 0x140 &&
                   TestFieldAt<std::int32_t>(panel, 0x18) == topY[index] &&
                   TestFieldAt<std::int32_t>(panel, 0x144) == 1 &&
                   TestFieldAt<std::uint32_t>(panel, 0x264) == 1 &&
                   TestFieldAt<std::int32_t>(panel, 0x29c) == -1 &&
                   TestFieldAt<std::int32_t>(panel, 0x2a0) == -1 &&
                   (TestFieldAt<std::uint32_t>(panel, 0x0c) & 0x10) != 0 && panel->hFont != nullptr;
    }

    const bool topLinked =
        top.base.childHead == reinterpret_cast<HudUiElement *>(TextStackLineAt(&top, 0)) &&
        top.base.childTail == reinterpret_cast<HudUiElement *>(TextStackLineAt(&top, 3));

    HudUiChatMessageStack chat{};
    chat.Constructor();

    const std::int32_t chatY[4] = {0x159, 0x147, 0x135, 0x123};
    bool chatLines = chat.base.vptr == reinterpret_cast<const HudUiContainer_FTable *>(
                                           &g_HudUiChatMessageStack_FTable);
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAt(&chat, index);
        chatLines = chatLines && TestFieldAt<const void *>(panel, 0x00) == &g_HudUiPanel_FTable &&
                    TestFieldAt<std::int32_t>(panel, 0x14) == 0x140 &&
                    TestFieldAt<std::int32_t>(panel, 0x18) == chatY[index] &&
                    TestFieldAt<std::uint32_t>(panel, 0x14c) == 0x00996a00 &&
                    TestFieldAt<std::uint32_t>(panel, 0x150) == 0x0095c7ff &&
                    TestFieldAt<std::int32_t>(panel, 0x144) == 1 &&
                    TestFieldAt<std::uint32_t>(panel, 0x264) == 1 &&
                    TestFieldAt<std::int32_t>(panel, 0x29c) == -1 &&
                    TestFieldAt<std::int32_t>(panel, 0x2a0) == -1 &&
                    (TestFieldAt<std::uint32_t>(panel, 0x0c) & 0x10) != 0 &&
                    panel->hFont != nullptr;
    }

    const bool chatLinked =
        chat.base.childHead == reinterpret_cast<HudUiElement *>(TextStackLineAt(&chat, 0)) &&
        chat.base.childTail == reinterpret_cast<HudUiElement *>(TextStackLineAt(&chat, 3));

    g_HudUi_InvalidateMask = 0x80;
    top.SetTextColors(0x00112233, 0x00445566);
    top.SetXAll(0x155);
    top.SetYDescending(0x88);

    bool topMutators = true;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAt(&top, index);
        topMutators = topMutators && TestFieldAt<std::uint32_t>(panel, 0x14c) == 0x00112233 &&
                      TestFieldAt<std::uint32_t>(panel, 0x150) == 0x00445566 &&
                      TestFieldAt<std::uint32_t>(panel, 0x270) == 1 &&
                      TestFieldAt<std::int32_t>(panel, 0x14) == 0x155 &&
                      TestFieldAt<std::int32_t>(panel, 0x18) == 0x88 - index * 0x12 &&
                      (TestFieldAt<std::uint32_t>(panel, 0x0c) & 0x80) != 0;
    }

    DeleteTextStackLineFonts(&top);
    DeleteTextStackLineFonts(&chat);
    g_HudUi_InvalidateMask = 0;
    return topLines && topLinked && chatLines && chatLinked && topMutators ? 0 : 1;
}

extern "C" int zhud_text_stack_set_font_all_smoke(void) {
    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x80 / 4] = MethodAddress(&TestTextStackFontPanel::SetFont);

    HudUiTextStack4 stack{};
    for (int index = 0; index < 4; ++index) {
        TextStackLineAt(&stack, index)->vtbl = &panelTable;
    }

    std::memset(g_textStackSetFontThis, 0, sizeof(g_textStackSetFontThis));
    std::memset(g_textStackSetFontFace, 0, sizeof(g_textStackSetFontFace));
    std::memset(g_textStackSetFontHeight, 0, sizeof(g_textStackSetFontHeight));
    std::memset(g_textStackSetFontWeight, 0, sizeof(g_textStackSetFontWeight));
    std::memset(g_textStackSetFontWidth, 0, sizeof(g_textStackSetFontWidth));
    std::memset(g_textStackSetFontItalic, 0, sizeof(g_textStackSetFontItalic));
    std::memset(g_textStackSetFontCharSet, 0, sizeof(g_textStackSetFontCharSet));
    std::memset(g_textStackSetFontPitch, 0, sizeof(g_textStackSetFontPitch));
    g_textStackSetFontCount = 0;

    const char *const faceName = "Arial Narrow";
    stack.SetFontAll(faceName, 18, 500, 7);

    bool order = g_textStackSetFontCount == 4;
    for (int callIndex = 0; callIndex < 4; ++callIndex) {
        const int lineIndex = 3 - callIndex;
        order = order && g_textStackSetFontThis[callIndex] == TextStackLineAt(&stack, lineIndex) &&
                g_textStackSetFontFace[callIndex] == faceName &&
                g_textStackSetFontHeight[callIndex] == 18 &&
                g_textStackSetFontWeight[callIndex] == 500 &&
                g_textStackSetFontWidth[callIndex] == 7 &&
                g_textStackSetFontItalic[callIndex] == 0 &&
                g_textStackSetFontCharSet[callIndex] == 0 &&
                g_textStackSetFontPitch[callIndex] == 2;
    }

    return order ? 0 : 1;
}

extern "C" int zhud_text_stack_push_line_smoke(void) {
    HudUiTopMessageStack top{};
    top.Constructor();
    g_HudUiTopMessageStack = &top;
    HudUiChatMessageStack chat{};
    chat.Constructor();
    g_HudUiChatMessageStack = &chat;

    top.base.enabled = 0;
    HudUi::ShowTopMessageLine("ignored", 1.0f);
    HudUiPanel *line0 = TextStackLineAt(&top, 0);
    HudUiPanel *line1 = TextStackLineAt(&top, 1);
    const bool disabledShowSkipped = (TestFieldAt<std::uint32_t>(line0, 0x0c) & 0x10u) != 0;

    top.PushLine("alpha", 2.5f);
    const bool firstPush = top.base.enabled == 1 &&
                           (TestFieldAt<std::uint32_t>(line0, 0x0c) & 0x10u) == 0 &&
                           TestFieldAt<float>(line0, 0x10) == 2.5f &&
                           std::strcmp(line0->GetLastTextPtr(), "alpha") == 0;

    top.PushLine("alpha", 3.5f);
    const bool repeatedTopDoesNotShift = (TestFieldAt<std::uint32_t>(line1, 0x0c) & 0x10u) != 0 &&
                                         TestFieldAt<float>(line0, 0x10) == 3.5f;

    top.PushLine("beta", 4.5f);
    const bool shifted = std::strcmp(line0->GetLastTextPtr(), "beta") == 0 &&
                         std::strcmp(line1->GetLastTextPtr(), "alpha") == 0 &&
                         (TestFieldAt<std::uint32_t>(line1, 0x0c) & 0x10u) == 0 &&
                         TestFieldAt<float>(line1, 0x10) == 3.5f;

    HudUi::ShowTopMessageLine("gamma", 5.0f);
    const bool showPushesWhenEnabled = std::strcmp(line0->GetLastTextPtr(), "gamma") == 0 &&
                                       std::strcmp(line1->GetLastTextPtr(), "beta") == 0;

    HudUi::PushTopMessageLine("epsilon", 7.0f);
    const bool directTopPush =
        std::strcmp(line0->GetLastTextPtr(), "epsilon") == 0 &&
        std::strcmp(line1->GetLastTextPtr(), "gamma") == 0 &&
        TestFieldAt<float>(line0, 0x10) == 7.0f;

    chat.base.enabled = 0;
    HudUi::ShowChatLine("ignored", 1.0f);
    HudUiPanel *chatLine0 = TextStackLineAt(&chat, 0);
    const bool disabledChatSkipped = (TestFieldAt<std::uint32_t>(chatLine0, 0x0c) & 0x10u) != 0;

    chat.base.enabled = 1;
    HudUi::ShowChatLine("delta", 6.0f);
    const bool showChatPushesWhenEnabled = std::strcmp(chatLine0->GetLastTextPtr(), "delta") == 0 &&
                                           TestFieldAt<float>(chatLine0, 0x10) == 6.0f;

    DeleteTextStackLineFonts(&top);
    DeleteTextStackLineFonts(&chat);
    g_HudUiTopMessageStack = nullptr;
    g_HudUiChatMessageStack = nullptr;
    if (!disabledShowSkipped) {
        return 1;
    }
    if (!firstPush) {
        return 2;
    }
    if (!repeatedTopDoesNotShift) {
        return 3;
    }
    if (!shifted) {
        return 4;
    }
    if (!showPushesWhenEnabled) {
        return 5;
    }
    if (!directTopPush) {
        return 8;
    }
    if (!disabledChatSkipped) {
        return 6;
    }
    return showChatPushesWhenEnabled ? 0 : 7;
}

extern "C" int zhud_text_stack_clear_and_enable_smoke(void) {
    HudUiTopMessageStack top{};
    top.Constructor();
    HudUiChatMessageStack chat{};
    chat.Constructor();
    g_HudUiTopMessageStack = &top;
    g_HudUiChatMessageStack = &chat;

    top.PushLine("top alpha", 2.0f);
    top.PushLine("top beta", 3.0f);
    chat.base.enabled = 1;
    chat.PushLine("chat alpha", 4.0f);

    top.base.enabled = 0;
    chat.base.enabled = 0;
    HudUiMgr::EnableTopAndChatStacks();

    bool cleared = top.base.enabled == 1 && chat.base.enabled == 1;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const topLine = TextStackLineAt(&top, index);
        HudUiPanel *const chatLine = TextStackLineAt(&chat, index);
        cleared = cleared && (TestFieldAt<std::uint32_t>(topLine, 0x0c) & 0x10u) != 0 &&
                  (TestFieldAt<std::uint32_t>(chatLine, 0x0c) & 0x10u) != 0 &&
                  std::strcmp(topLine->GetLastTextPtr(), "") == 0 &&
                  std::strcmp(chatLine->GetLastTextPtr(), "") == 0;
    }

    DeleteTextStackLineFonts(&top);
    DeleteTextStackLineFonts(&chat);
    g_HudUiTopMessageStack = nullptr;
    g_HudUiChatMessageStack = nullptr;
    return cleared ? 0 : 1;
}

extern "C" int zhud_text_stack_clear_and_disable_smoke(void) {
    HudUiTopMessageStack top{};
    top.Constructor();
    HudUiChatMessageStack chat{};
    chat.Constructor();
    g_HudUiTopMessageStack = &top;
    g_HudUiChatMessageStack = &chat;

    top.PushLine("top alpha", 2.0f);
    top.PushLine("top beta", 3.0f);
    chat.base.enabled = 1;
    chat.PushLine("chat alpha", 4.0f);

    top.base.enabled = 1;
    chat.base.enabled = 1;
    HudUiMgr::DisableTopAndChatStacks();

    bool cleared = top.base.enabled == 0 && chat.base.enabled == 0;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const topLine = TextStackLineAt(&top, index);
        HudUiPanel *const chatLine = TextStackLineAt(&chat, index);
        cleared = cleared && (TestFieldAt<std::uint32_t>(topLine, 0x0c) & 0x10u) != 0 &&
                  (TestFieldAt<std::uint32_t>(chatLine, 0x0c) & 0x10u) != 0 &&
                  std::strcmp(topLine->GetLastTextPtr(), "") == 0 &&
                  std::strcmp(chatLine->GetLastTextPtr(), "") == 0;
    }

    DeleteTextStackLineFonts(&top);
    DeleteTextStackLineFonts(&chat);
    g_HudUiTopMessageStack = nullptr;
    g_HudUiChatMessageStack = nullptr;
    return cleared ? 0 : 1;
}

extern "C" int zhud_text_stack_destructor_core_smoke(void) {
    HudUiTopMessageStack top{};
    top.Constructor();
    top.DestructorCore();

    bool topDestroyed = top.base.vptr == &g_HudUiContainer_FTable;
    for (int index = 0; index < 4; ++index) {
        topDestroyed = topDestroyed && TestFieldAt<const void *>(TextStackLineAt(&top, index),
                                                                 0x00) == &g_HudUiCommon_FTable;
    }

    HudUiChatMessageStack chat{};
    chat.Constructor();
    chat.DestructorCore();

    bool chatDestroyed = chat.base.vptr == &g_HudUiContainer_FTable;
    for (int index = 0; index < 4; ++index) {
        chatDestroyed = chatDestroyed && TestFieldAt<const void *>(TextStackLineAt(&chat, index),
                                                                   0x00) == &g_HudUiCommon_FTable;
    }

    return topDestroyed && chatDestroyed ? 0 : 1;
}

extern "C" int zhud_loading_checkpoint_init_table_smoke(void) {
    const float expectedRaw[19] = {
        0.00100000005f, 0.136999995f, 0.237000003f, 0.340000004f, 0.899999976f,
        9.30000019f,    12.3999996f,  13.3999996f,  20.0f,        26.0f,
        26.2999992f,    28.7000008f,  31.5f,        34.0f,        36.2000008f,
        36.4000015f,    53.2999992f,  53.5999985f,  53.7000008f,
    };

    g_HudUiLoadingCheckpointMaxIndex = 99;
    g_HudUiLoadingCheckpointCurrentIndex = 77;
    for (int index = 0; index < 19; ++index) {
        g_HudUiLoadingCheckpointRawProgress[index] = -1.0f;
        g_HudUiLoadingCheckpointProgress[index] = -1.0f;
    }

    HudUiLoadingCheckpoint::InitTable();

    if (g_HudUiLoadingCheckpointMaxIndex != 18 ||
        g_HudUiLoadingCheckpointCurrentIndex != 0) {
        return 1;
    }

    for (int index = 0; index < 19; ++index) {
        const float expectedProgress =
            expectedRaw[index] * g_HudUiLoadingCheckpointProgressScale;
        if (g_HudUiLoadingCheckpointRawProgress[index] != expectedRaw[index] ||
            g_HudUiLoadingCheckpointProgress[index] != expectedProgress) {
            return 2;
        }
    }

    return 0;
}

extern "C" int zhud_loading_checkpoint_advance_and_log_smoke(void) {
    HudUiBriefingRuntime *const oldRuntime = g_Briefing_Runtime;
    g_Briefing_Runtime = nullptr;
    HudUiLoadingCheckpoint::InitTable();

    g_HudUiLoadingCheckpointCurrentIndex = 2;
    g_HudUiLoadingCheckpointCurrentProgress = -1.0f;
    HudUiLoadingCheckpoint::AdvanceAndLog(nullptr);
    const bool normalAdvance =
        g_HudUiLoadingCheckpointCurrentProgress == g_HudUiLoadingCheckpointProgress[2] &&
        g_HudUiLoadingCheckpointCurrentIndex == 3;

    g_HudUiLoadingCheckpointCurrentIndex = g_HudUiLoadingCheckpointMaxIndex;
    g_HudUiLoadingCheckpointCurrentProgress = -1.0f;
    HudUiLoadingCheckpoint::AdvanceAndLog(nullptr);
    const bool maxClamped =
        g_HudUiLoadingCheckpointCurrentProgress ==
            g_HudUiLoadingCheckpointProgress[g_HudUiLoadingCheckpointMaxIndex] &&
        g_HudUiLoadingCheckpointCurrentIndex == g_HudUiLoadingCheckpointMaxIndex;

    g_Briefing_Runtime = oldRuntime;
    return normalAdvance && maxClamped ? 0 : 1;
}

extern "C" int zhud_sensor_viewport_rect_smoke(void) {
    std::int32_t replicate = 0;
    ZOPT_REPLICATE = &replicate;
    g_HudUiMgrSensorBlock = {};
    g_HudUiMgrSensorBlock.sensorParam = 2.0f;

    HudUiMgrSensor::SetViewportRect(10, 20, 100, 80);
    const bool normal = g_HudUiMgrSensorBlock.sensorRectRaw.left == 10 &&
                        g_HudUiMgrSensorBlock.sensorRectRaw.top == 20 &&
                        g_HudUiMgrSensorBlock.sensorRectRaw.right == 110 &&
                        g_HudUiMgrSensorBlock.sensorRectRaw.bottom == 100 &&
                        g_HudUiMgrSensorBlock.sensorRectScaled.left == 10 &&
                        g_HudUiMgrSensorBlock.sensorRectScaled.top == 20 &&
                        g_HudUiMgrSensorBlock.sensorRectScaled.right == 110 &&
                        g_HudUiMgrSensorBlock.sensorRectScaled.bottom == 100 &&
                        g_HudUiMgrSensorBlock.sensorPiVSrcRect.left == 10.0f &&
                        g_HudUiMgrSensorBlock.sensorPiVSrcRect.top == 20.0f &&
                        g_HudUiMgrSensorBlock.sensorPiVSrcRect.right == 110.0f &&
                        g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom == 100.0f &&
                        g_HudUiMgrSensorBlock.sensorClampHalfW == 50.0f &&
                        g_HudUiMgrSensorBlock.sensorClampHalfH == 40.0f &&
                        g_zClipAlt_SourceWidth == 100.0f && g_zClipAlt_SourceHeight == 80.0f;

    replicate = 1;
    HudUiMgrSensor::SetViewportRect(11, 21, 101, 81);
    const bool replicated = g_HudUiMgrSensorBlock.sensorRectRaw.left == 11 &&
                            g_HudUiMgrSensorBlock.sensorRectRaw.top == 21 &&
                            g_HudUiMgrSensorBlock.sensorRectRaw.right == 112 &&
                            g_HudUiMgrSensorBlock.sensorRectRaw.bottom == 102 &&
                            g_HudUiMgrSensorBlock.sensorRectScaled.left == 5 &&
                            g_HudUiMgrSensorBlock.sensorRectScaled.top == 10 &&
                            g_HudUiMgrSensorBlock.sensorRectScaled.right == 55 &&
                            g_HudUiMgrSensorBlock.sensorRectScaled.bottom == 50 &&
                            g_HudUiMgrSensorBlock.sensorPiVSrcRect.left == 5.0f &&
                            g_HudUiMgrSensorBlock.sensorPiVSrcRect.top == 10.0f &&
                            g_HudUiMgrSensorBlock.sensorPiVSrcRect.right == 55.0f &&
                            g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom == 50.0f &&
                            g_HudUiMgrSensorBlock.sensorClampHalfW == 25.0f &&
                            g_HudUiMgrSensorBlock.sensorClampHalfH == 20.0f &&
                            g_zClipAlt_SourceLeft == 5.0f && g_zClipAlt_SourceTop == 10.0f;

    return normal && replicated ? 0 : 1;
}

extern "C" int zhud_objective_update_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;

    alignas(HudUiPanel) std::uint8_t descStorage[0x2a4]{};
    alignas(HudUiPanel) std::uint8_t labelStorage[0x2a4]{};
    auto *const desc = reinterpret_cast<HudUiPanel *>(descStorage);
    auto *const label = reinterpret_cast<HudUiPanel *>(labelStorage);
    desc->ConstructorDefault(nullptr, 0, 0);
    label->ConstructorDefault(nullptr, 0, 0);

    g_HudUiMgrObjectiveWidget = {};
    g_HudUiMgrObjectiveBar = {};
    g_HudUiMgrObjectiveSensorRect = {};
    g_HudUiMgrObjectiveWidget.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveBar.ftable =
        reinterpret_cast<const HudUiBar_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveMeter.ftable =
        reinterpret_cast<const HudUiMeter_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveDescTextPanel = desc;
    g_HudUiMgrObjectiveLabelTextPanel = label;
    g_HudUiMgrObjectiveWidget.flags = 0x10;
    g_HudUiMgrObjectiveBar.flags = 0x10;
    g_HudUiMgrObjectiveSensorRect.flags = 0x10;
    reinterpret_cast<HudUiElement *>(desc)->flags = 0x10;
    reinterpret_cast<HudUiElement *>(label)->flags = 0x10;

    g_HudUiMgrObjectivePhase = 0;
    HudUiMgrObjective::Update();
    const bool phase0 = (g_HudUiMgrObjectiveWidget.flags & 0x10) == 0 &&
                        (g_HudUiMgrObjectiveBar.flags & 0x10) != 0 &&
                        (reinterpret_cast<HudUiElement *>(desc)->flags & 0x10) != 0 &&
                        (reinterpret_cast<HudUiElement *>(label)->flags & 0x10) != 0 &&
                        (g_HudUiMgrObjectiveSensorRect.flags & 0x10) != 0;

    g_HudUiMgrObjectiveWidget.flags = 0x10;
    g_HudUiMgrObjectiveBar.flags = 0x10;
    g_HudUiMgrObjectiveSensorRect.flags = 0x10;
    reinterpret_cast<HudUiElement *>(desc)->flags = 0x10;
    reinterpret_cast<HudUiElement *>(label)->flags = 0x10;

    g_HudUiMgrObjectivePhase = 2;
    HudUiMgrObjective::Update();
    const bool phase2 = (g_HudUiMgrObjectiveWidget.flags & 0x10) == 0 &&
                        (g_HudUiMgrObjectiveBar.flags & 0x10) == 0 &&
                        (reinterpret_cast<HudUiElement *>(desc)->flags & 0x10) == 0 &&
                        (reinterpret_cast<HudUiElement *>(label)->flags & 0x10) == 0 &&
                        (g_HudUiMgrObjectiveSensorRect.flags & 0x10) == 0;

    reinterpret_cast<HudUiElement *>(label)->flags = 0x10;
    reinterpret_cast<HudUiElement *>(&g_HudUiMgrObjectiveMeter)->flags = 0x10;
    g_HudUiMgrObjectiveMeter.points[0].y = 4.0f;
    g_HudUiMgrObjectiveMeter.points[1].y = 23.75f;
    g_HudUiMgrObjectiveMeter.points[3].y = 7.0f;
    g_HudUiMgrObjectiveMeterFillAnimTimerSec = 9.0f;
    g_HudUiMgrObjectiveMeterFillAnimEnabled = 0;
    HudUiMgrObjective::SetVisibleAndResetMeterFill(1);
    const bool meterShow =
        (reinterpret_cast<HudUiElement *>(label)->flags & 0x10) == 0 &&
        (reinterpret_cast<HudUiElement *>(&g_HudUiMgrObjectiveMeter)->flags & 0x10) == 0 &&
        g_HudUiMgrObjectiveMeterFillAnimTimerSec == 0.0f &&
        g_HudUiMgrObjectiveMeterFillAnimEnabled == 1 &&
        g_HudUiMgrObjectiveMeter.points[0].y == 23.0f &&
        g_HudUiMgrObjectiveMeter.points[3].y == 23.0f;

    HudUiMgrObjective::SetVisibleAndResetMeterFill(0);
    const bool meterHide =
        (reinterpret_cast<HudUiElement *>(label)->flags & 0x10) != 0 &&
        (reinterpret_cast<HudUiElement *>(&g_HudUiMgrObjectiveMeter)->flags & 0x10) != 0;

    DeleteObject(desc->hFont);
    DeleteObject(label->hFont);
    desc->hFont = nullptr;
    label->hFont = nullptr;
    g_HudUi_InvalidateMask = 0;
    if (!phase0) {
        return 1;
    }
    if (!phase2) {
        return 2;
    }
    if (!meterShow) {
        return 3;
    }
    if (!meterHide) {
        return 4;
    }
    return 0;
}

extern "C" int zhud_objective_begin_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;

    alignas(HudUiPanel) std::uint8_t descStorage[0x2a4]{};
    alignas(HudUiPanel) std::uint8_t labelStorage[0x2a4]{};
    auto *const desc = reinterpret_cast<HudUiPanel *>(descStorage);
    auto *const label = reinterpret_cast<HudUiPanel *>(labelStorage);
    desc->ConstructorDefault(nullptr, 0, 0);
    label->ConstructorDefault(nullptr, 0, 0);

    g_HudUiMgrObjectiveDescTextPanel = desc;
    g_HudUiMgrObjectiveLabelTextPanel = label;
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveSensorRect.flags = 0;
    g_HudUiMgrObjectiveState = 0;
    g_HudUiMgrObjectivePhase = 2;
    g_HudUiMgrObjectivePhaseTimerSec = 9.0f;
    g_HudUiMgrObjectiveAutoHideDelaySec = 4.0f;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    HudUiMgrObjective::Begin();
    const bool phase2 = g_HudUiMgrObjectiveState == 1 && g_HudUiMgrObjectivePhase == 3 &&
                        g_HudUiMgrObjectivePhaseTimerSec == 0.0f &&
                        g_HudUiMgrObjectiveAutoHideDelaySec == 0.0f &&
                        (reinterpret_cast<HudUiElement *>(desc)->flags & 0x10) != 0 &&
                        (reinterpret_cast<HudUiElement *>(label)->flags & 0x10) != 0 &&
                        (g_HudUiMgrObjectiveSensorRect.flags & 0x10) != 0;

    g_HudUiMgrObjectivePhase = 1;
    g_HudUiMgrObjectivePhaseTimerSec = 1.25f;
    g_HudUiMgrObjectivePhaseDurationSec = 3.0f;
    g_HudUiMgrObjectiveAutoHideDelaySec = 8.0f;
    HudUiMgrObjective::Begin();
    const bool phase1 = g_HudUiMgrObjectivePhase == 3 &&
                        g_HudUiMgrObjectivePhaseTimerSec == 1.75f &&
                        g_HudUiMgrObjectiveAutoHideDelaySec == 0.0f;

    g_HudUiMgrObjectiveChatComposeActive = 1;
    g_HudUiMgrObjectivePhase = 2;
    g_HudUiMgrObjectiveState = 0;
    HudUiMgrObjective::Begin();
    const bool chatGuard = g_HudUiMgrObjectivePhase == 2 && g_HudUiMgrObjectiveState == 0;

    DeleteObject(desc->hFont);
    DeleteObject(label->hFont);
    desc->hFont = nullptr;
    label->hFont = nullptr;
    g_HudUiMgrObjectiveChatComposeActive = 0;
    g_HudUi_InvalidateMask = 0;
    return phase2 && phase1 && chatGuard ? 0 : 1;
}

extern "C" int zhud_objective_show_smoke(void) {
    g_HudUi_InvalidateMask = 0x80;

    alignas(HudUiPanel) std::uint8_t summaryStorage[0x2a4]{};
    alignas(HudUiPanel) std::uint8_t descStorage[0x2a4]{};
    auto *const summary = reinterpret_cast<HudUiPanel *>(summaryStorage);
    auto *const desc = reinterpret_cast<HudUiPanel *>(descStorage);
    summary->ConstructorDefault(nullptr, 0, 0);
    desc->ConstructorDefault(nullptr, 0, 0);

    zVidImagePartial objectiveImage = {};
    zVidImagePartial widgetImage = {};
    widgetImage.width = 24;

    g_HudUiMgrObjectiveSummaryTextPanel = summary;
    g_HudUiMgrObjectiveDescTextPanel = desc;
    g_HudUiMgrSensorOverlay.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveBar.ftable =
        reinterpret_cast<const HudUiBar_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveWidget.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveWidget.image = &widgetImage;
    g_HudUiMgrSensorOverlay.flags = 0;
    g_HudUiMgrObjectiveBar.flags = 0x10;
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveState = 0;
    g_HudUiMgrObjectivePhaseTimerSec = 9.0f;
    g_HudUiMgrObjectiveAutoHideDelaySec = 0.0f;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    const int shown = HudUiMgrObjective::Show(&objectiveImage, "Summary", "Desc", 2.5f);
    const bool showOk =
        shown == 1 && g_HudUiMgrObjectiveState == 1 && g_HudUiMgrObjectivePhase == 1 &&
        g_HudUiMgrObjectivePhaseTimerSec == 0.0f &&
        g_HudUiMgrObjectiveAutoHideDelaySec == 2.5f &&
        g_HudUiMgrObjectiveSensorRect.image == &objectiveImage &&
        (g_HudUiMgrObjectiveBar.flags & 0x10) == 0 &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Summary") == 0 &&
        std::strcmp(&TestFieldAt<char>(desc, 0x34), "Desc") == 0;

    g_HudUiMgrObjectivePhase = 3;
    g_HudUiMgrObjectivePhaseTimerSec = 0.75f;
    g_HudUiMgrObjectivePhaseDurationSec = 3.0f;
    const int reversed = HudUiMgrObjective::Show(&objectiveImage, "Again", "Text", 1.0f);
    const bool reverseOk =
        reversed == 1 && g_HudUiMgrObjectivePhase == 1 &&
        g_HudUiMgrObjectivePhaseTimerSec == 2.25f;

    g_HudUiMgrObjectiveChatComposeActive = 1;
    const int guarded = HudUiMgrObjective::Show(&objectiveImage, "No", "No", 1.0f);
    const bool guardOk = guarded == 0;

    DeleteObject(summary->hFont);
    DeleteObject(desc->hFont);
    summary->hFont = nullptr;
    desc->hFont = nullptr;
    g_HudUiMgrObjectiveChatComposeActive = 0;
    g_HudUi_InvalidateMask = 0;
    return showOk && reverseOk && guardOk ? 0 : 1;
}

extern "C" int hud_sensor_tracker_show_objective_pickup_info_smoke(void) {
    alignas(HudUiPanel) std::uint8_t summaryStorage[0x2a4]{};
    alignas(HudUiPanel) std::uint8_t descStorage[0x2a4]{};
    auto *const summary = reinterpret_cast<HudUiPanel *>(summaryStorage);
    auto *const desc = reinterpret_cast<HudUiPanel *>(descStorage);
    summary->ConstructorDefault(nullptr, 0, 0);
    desc->ConstructorDefault(nullptr, 0, 0);

    zVidImagePartial objectiveImage = {};
    zVidImagePartial widgetImage = {};
    widgetImage.width = 16;

    g_HudUiMgrObjectiveSummaryTextPanel = summary;
    g_HudUiMgrObjectiveDescTextPanel = desc;
    g_HudUiMgrSensorOverlay.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveBar.ftable =
        reinterpret_cast<const HudUiBar_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveWidget.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveWidget.image = &widgetImage;
    g_HudUiMgrObjectiveBar.flags = 0x10;
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    OptCatalogEntryDef entry = {};
    entry.description = const_cast<char *>("Pickup objective");
    entry.fireRateInterval = 0.5f;
    entry.range = 120.0f;
    entry.impactProximity = 12.0f;
    entry.damage = 4.5f;
    entry.flags = 0x00080000u | 0x00010000u | 0x00004000u | 0x00000002u;

    PickupType oldPickupType = g_PickupTypes[17];
    g_PickupTypes[17] = {};
    g_PickupTypes[17].optEntry = &entry;
    g_PickupTypes[17].optMetaImage = &objectiveImage;

    HudSensorTracker tracker = {};
    float readTime = 4.0f;
    std::memcpy(&tracker.objectiveReadTimeSecRaw, &readTime, sizeof(readTime));
    const float oldUnscaledTime = g_Time_UnscaledAccumulatedTimeSec;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    g_Time_UnscaledAccumulatedTimeSec = 3.0f;

    tracker.ShowObjectivePickupInfo(1, 1, &entry);
    float deadline = 0.0f;
    std::memcpy(&deadline, &tracker.objectiveFlowDeadlineSecRaw, sizeof(deadline));
    const bool showOk =
        tracker.objectiveUiMode == 4 && deadline == 7.0f &&
        g_HudUiMgrObjectiveSensorRect.image == &objectiveImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Pickup objective") == 0 &&
        std::strstr(&TestFieldAt<char>(desc, 0x34), "Damage Proximity") != nullptr &&
        std::strstr(&TestFieldAt<char>(desc, 0x34), "Remote") != nullptr &&
        std::strstr(&TestFieldAt<char>(desc, 0x34), "Lock On") != nullptr;

    g_HudUiMgrObjectivePhase = 1;
    tracker.ShowObjectivePickupInfo(0, 0, &entry);
    const bool hideOk = tracker.objectiveUiMode == 0 && g_HudUiMgrObjectivePhase == 3;

    TestReticleAltGunController altGun{};
    altGun.optCatalogEntry = &entry;
    TestReticlePlayerState playerState{};
    playerState.activeAltGunController = &altGun;
    zInput_GameStateOrMapTablePartial gameState{};
    gameState.playerState = reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    g_GameStateOrMapTable = &gameState;

    tracker.objectiveUiMode = 0;
    g_HudUiMgrObjectivePhase = 0;
    tracker.Command_ShowObjectivePickupInfo();
    const bool commandShowOk =
        tracker.objectiveUiMode == 3 &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Pickup objective") == 0;

    g_HudUiMgrObjectivePhase = 1;
    tracker.Command_ShowObjectivePickupInfo();
    const bool commandHideOk = tracker.objectiveUiMode == 0 && g_HudUiMgrObjectivePhase == 3;

    DeleteObject(summary->hFont);
    DeleteObject(desc->hFont);
    summary->hFont = nullptr;
    desc->hFont = nullptr;
    g_PickupTypes[17] = oldPickupType;
    g_Time_UnscaledAccumulatedTimeSec = oldUnscaledTime;
    g_GameStateOrMapTable = oldGameState;
    return showOk && hideOk && commandShowOk && commandHideOk ? 0 : 1;
}

extern "C" int hud_sensor_tracker_objective_panel_visible_smoke(void) {
    alignas(HudUiPanel) std::uint8_t summaryStorage[0x2a4]{};
    alignas(HudUiPanel) std::uint8_t descStorage[0x2a4]{};
    auto *const summary = reinterpret_cast<HudUiPanel *>(summaryStorage);
    auto *const desc = reinterpret_cast<HudUiPanel *>(descStorage);
    summary->ConstructorDefault(nullptr, 0, 0);
    desc->ConstructorDefault(nullptr, 0, 0);

    HMODULE const oldMessagesDll = g_zLoc_MessagesDllHandle;
    HudUiPanel *const oldSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    HudUiPanel *const oldLabelPanel = g_HudUiMgrObjectiveLabelTextPanel;
    const int oldHitCount = g_OptCatalog_DamageFeedbackHitCount;
    void *const oldVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const int oldFlag10 = g_zSnd_Flag10PlaybackEnabled;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldSndPreInitialized = g_zSnd_PreInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const float oldHudScale = g_HudSensorTracker.hudScale;
    const float oldUnscaledTime = g_Time_UnscaledAccumulatedTimeSec;
    HudSensorTracker const oldGlobalTracker = g_HudSensorTracker;
    const int oldObjectiveCommandLocked = g_HudSensorTracker_ObjectiveCommandLocked;
    const int oldMapOverlayRefCount = g_Hud_MapOverlayRefCount;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;

    HMODULE messagesDll = LoadLibraryA("support\\messages.dll");
    if (messagesDll == nullptr) {
        messagesDll = LoadLibraryA("..\\..\\..\\..\\support\\messages.dll");
    }
    if (messagesDll == nullptr) {
        DeleteObject(summary->hFont);
        DeleteObject(desc->hFont);
        return 1;
    }

    zVidImagePartial firstImage = {};
    zVidImagePartial currentImage = {};
    zVidImagePartial widgetImage = {};
    widgetImage.width = 16;

    g_zLoc_MessagesDllHandle = messagesDll;
    g_HudUiMgrObjectiveSummaryTextPanel = summary;
    g_HudUiMgrObjectiveDescTextPanel = desc;
    g_HudUiMgrObjectiveLabelTextPanel = summary;
    g_HudUiMgrSensorOverlay.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveBar.ftable =
        reinterpret_cast<const HudUiBar_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveWidget.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveMeter.ftable =
        reinterpret_cast<const HudUiMeter_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveWidget.image = &widgetImage;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    HudSensorTracker tracker = {};
    tracker.objectiveCount = 5;
    tracker.completedObjectiveCount = 2;
    tracker.primaryGunDispatchCount = 8;
    tracker.missionStat0 = 9;
    tracker.missionStat1 = 7;
    tracker.missionStat3 = 11;
    tracker.weaponsFoundMask = 0x15;
    tracker.objectiveMeterSeconds = 125.8f;
    tracker.currentObjectiveIndex = -1;
    tracker.objectiveSlots[0].objectiveImage = &firstImage;
    std::strcpy(tracker.objectiveSlots[0].objectiveTitle, "First objective");
    tracker.objectiveSlots[2].objectiveImage = &currentImage;
    std::strcpy(tracker.objectiveSlots[2].objectiveTitle, "Review title");
    std::strcpy(tracker.objectiveSlots[2].objectiveDesc, "Review description");
    std::strcpy(tracker.objectiveSlots[2].objectiveSummary, "Current summary");
    g_OptCatalog_DamageFeedbackHitCount = 3;

    g_HudUiMgrObjectivePhase = 1;
    tracker.objectiveUiMode = 2;
    tracker.SetObjectivePanelVisible(0);
    const bool hideOk = tracker.objectiveUiMode == 0 && g_HudUiMgrObjectivePhase == 3;

    char objectiveLine[0x80] = {};
    char statLine[0x80] = {};
    char timeLine[0x80] = {};
    if (zLoc::FormatMessage(objectiveLine, 0x40, 0x116, 2, 5, 37) == 0 ||
        zLoc::FormatMessage(statLine, 0x40, 0x117, 7, 7, 11, 0x15) == 0 ||
        zLoc::FormatMessage(timeLine, 0x40, 0x118, 2, 5) == 0) {
        g_zLoc_MessagesDllHandle = oldMessagesDll;
        g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
        g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
        g_HudUiMgrObjectiveLabelTextPanel = oldLabelPanel;
        g_OptCatalog_DamageFeedbackHitCount = oldHitCount;
        FreeLibrary(messagesDll);
        DeleteObject(summary->hFont);
        DeleteObject(desc->hFont);
        return 2;
    }

    char expectedSummary[0x400] = {};
    std::sprintf(expectedSummary, "%s\n%s\n%s", objectiveLine, statLine, timeLine);
    for (char *percent = std::strstr(expectedSummary, "%%"); percent != nullptr;
         percent = std::strstr(percent + 1, "%%")) {
        std::memmove(percent, percent + 1, std::strlen(percent));
    }

    g_HudUiMgrObjectivePhase = 0;
    tracker.SetObjectivePanelVisible(1);
    const bool firstSlotOk =
        tracker.objectiveUiMode == 2 && g_HudUiMgrObjectiveSensorRect.image == &firstImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "First objective") == 0 &&
        std::strcmp(&TestFieldAt<char>(desc, 0x34), expectedSummary) == 0;

    tracker.currentObjectiveIndex = 2;
    g_HudUiMgrObjectivePhase = 0;
    tracker.SetObjectivePanelVisible(1);
    const bool currentSlotOk =
        tracker.objectiveUiMode == 2 && g_HudUiMgrObjectiveSensorRect.image == &currentImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Current summary") == 0 &&
        std::strcmp(&TestFieldAt<char>(desc, 0x34), expectedSummary) == 0;

    tracker.objectiveReviewSfx = nullptr;
    tracker.objectiveUiMode = 0;
    g_HudUiMgrObjectivePhase = 0;
    tracker.Command_ToggleObjectivePanel();
    const bool commandShowOk = tracker.objectiveUiMode == 2;

    g_HudUiMgrObjectivePhase = 1;
    tracker.Command_ToggleObjectivePanel();
    const bool commandHideOk = tracker.objectiveUiMode == 0 && g_HudUiMgrObjectivePhase == 3;

    tracker.firstIncompleteObjectiveIndex = 2;
    tracker.objectiveFlowState = 0;
    g_HudUiMgrObjectivePhase = 0;
    tracker.SetObjectiveReviewVisible(1);
    const bool reviewShowOk =
        tracker.objectiveFlowState == 0x65 && tracker.objectiveUiMode == 1 &&
        g_HudUiMgrObjectiveSensorRect.image == &currentImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Review title") == 0 &&
        std::strcmp(&TestFieldAt<char>(desc, 0x34), "Review description") == 0;

    char completeTitle[0x100] = {};
    char *const completeTitleRaw = zLoc::GetMessageString(0x0f0f);
    if (completeTitleRaw == nullptr) {
        g_zLoc_MessagesDllHandle = oldMessagesDll;
        g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
        g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
        g_HudUiMgrObjectiveLabelTextPanel = oldLabelPanel;
        g_OptCatalog_DamageFeedbackHitCount = oldHitCount;
        g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
        g_zSnd_Flag10PlaybackEnabled = oldFlag10;
        g_HudSensorTracker.hudScale = oldHudScale;
        FreeLibrary(messagesDll);
        DeleteObject(summary->hFont);
        DeleteObject(desc->hFont);
        return 8;
    }
    std::strcpy(completeTitle, completeTitleRaw);

    tracker.firstIncompleteObjectiveIndex = tracker.objectiveCount;
    tracker.currentObjectiveIndex = 2;
    g_HudUiMgrObjectivePhase = 0;
    tracker.SetObjectiveReviewVisible(1);
    const bool reviewCompleteOk =
        tracker.objectiveUiMode == 1 && g_HudUiMgrObjectiveSensorRect.image == &currentImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Review title") == 0;

    float globalScale = 0.2f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 0;
    g_HudSensorTracker.hudScale = 0.625f;
    g_HudUiMgrObjectivePhase = 1;
    tracker.SetObjectiveReviewVisible(0);
    const bool reviewHideOk = tracker.objectiveUiMode == 0 &&
                              g_HudUiMgrObjectivePhase == 3 &&
                              globalScale == 0.625f &&
                              g_zSnd_Flag10PlaybackEnabled == 1;

    zSndSample completeSfx = {};
    zSndSample previousReadSfx = {};
    zSndSample firstReadSfx = {};
    zSndSample thirdReadSfx = {};
    tracker.objectiveCompleteSfx = &completeSfx;
    tracker.objectiveSlots[0].readSoundSample = &firstReadSfx;
    tracker.objectiveSlots[2].readSoundSample = &thirdReadSfx;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 2;
    g_HudSensorTracker.hudScale = 0.75f;

    tracker.objectiveUiMode = 0;
    tracker.objectiveFlowState = 0;
    tracker.firstIncompleteObjectiveIndex = 2;
    g_HudUiMgrObjectivePhase = 0;
    tracker.AdvanceObjectiveState();
    const bool advanceShowReviewOk =
        tracker.objectiveFlowState == 0x65 && tracker.objectiveUiMode == 1 &&
        g_HudUiMgrObjectiveSensorRect.image == &currentImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Review title") == 0;

    globalScale = 0.4f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 0;
    tracker.objectiveUiMode = 1;
    tracker.objectiveFlowState = 0;
    tracker.currentObjectiveReadSound = &previousReadSfx;
    g_HudUiMgrObjectivePhase = 1;
    tracker.AdvanceObjectiveState();
    const bool advanceHideReviewOk =
        tracker.objectiveFlowState == 0x65 && tracker.objectiveUiMode == 0 &&
        g_HudUiMgrObjectivePhase == 3 && globalScale == 0.75f &&
        g_zSnd_Flag10PlaybackEnabled == 1;

    globalScale = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 1;
    g_Time_UnscaledAccumulatedTimeSec = 10.0f;
    float readTime = 2.5f;
    std::memcpy(&tracker.objectiveReadTimeSecRaw, &readTime, sizeof(readTime));
    tracker.objectiveUiMode = 0;
    tracker.objectiveFlowState = 0x64;
    tracker.missionId = 2;
    tracker.currentObjectiveIndex = -1;
    tracker.firstIncompleteObjectiveIndex = 0;
    g_HudUiMgrObjectivePhase = 0;
    tracker.AdvanceObjectiveState();
    float deadline = 0.0f;
    std::memcpy(&deadline, &tracker.objectiveFlowDeadlineSecRaw, sizeof(deadline));
    const bool advanceSequentialOk =
        tracker.currentObjectiveReadSound == &firstReadSfx &&
        tracker.objectiveFlowState == 0x68 && deadline == 12.5f &&
        tracker.objectiveUiMode == 2 && tracker.hudScale == 1.0f &&
        globalScale == 0.600000024f && g_zSnd_Flag10PlaybackEnabled == 0;

    globalScale = 0.5f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 1;
    tracker.objectiveUiMode = 0;
    tracker.objectiveFlowState = 0x67;
    tracker.currentObjectiveIndex = 0;
    tracker.firstIncompleteObjectiveIndex = 2;
    tracker.currentObjectiveReadSound = nullptr;
    g_HudUiMgrObjectivePhase = 0;
    tracker.AdvanceObjectiveState();
    const bool advanceJumpOk =
        tracker.currentObjectiveReadSound == &thirdReadSfx &&
        tracker.objectiveFlowState == 0x69 && tracker.hudScale == 0.5f &&
        globalScale == 0.300000012f && g_zSnd_Flag10PlaybackEnabled == 0;

    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_HudSensorTracker = {};
    g_HudSensorTracker.objectiveCount = 5;
    g_HudSensorTracker.objectiveCompleteSfx = &completeSfx;
    g_HudSensorTracker.objectiveSlots[0].objectiveImage = &firstImage;
    g_HudSensorTracker.objectiveSlots[0].readSoundSample = &firstReadSfx;
    g_HudSensorTracker.objectiveSlots[2].objectiveImage = &currentImage;
    g_HudSensorTracker.objectiveSlots[2].readSoundSample = &thirdReadSfx;
    std::strcpy(g_HudSensorTracker.objectiveSlots[0].objectiveTitle, "First objective");
    std::strcpy(g_HudSensorTracker.objectiveSlots[2].objectiveTitle, "Review title");
    std::strcpy(g_HudSensorTracker.objectiveSlots[2].objectiveDesc, "Review description");
    std::strcpy(g_HudSensorTracker.objectiveSlots[2].objectiveSummary, "Current summary");

    g_HudSensorTracker_ObjectiveCommandLocked = 1;
    g_HudSensorTracker.objectiveUiMode = 0;
    g_HudSensorTracker.objectiveFlowState = 0;
    HudSensorTracker::OnObjectiveCommand(0x18);
    const bool commandLockedOk =
        g_HudSensorTracker.objectiveUiMode == 0 && g_HudSensorTracker.objectiveFlowState == 0;

    g_HudSensorTracker_ObjectiveCommandLocked = 0;
    g_HudSensorTracker.firstIncompleteObjectiveIndex = 2;
    g_HudUiMgrObjectivePhase = 0;
    HudSensorTracker::OnObjectiveCommand(0x18);
    const bool commandAdvanceOk =
        g_HudSensorTracker.objectiveUiMode == 1 &&
        g_HudSensorTracker.objectiveFlowState == 0x65;

    g_HudSensorTracker.objectiveUiMode = 0;
    g_HudSensorTracker.currentObjectiveIndex = -1;
    g_HudUiMgrObjectivePhase = 0;
    HudSensorTracker::OnObjectiveCommand(0x19);
    const bool commandPanelOk = g_HudSensorTracker.objectiveUiMode == 2;

    g_HudSensorTracker.mapScaleLerpActive = 1;
    g_HudSensorTracker.mapScaleCurrent.x = 4.0f;
    g_HudSensorTracker.mapScaleCurrent.y = 5.0f;
    g_HudSensorTracker.mapScaleCurrent.z = 6.0f;
    g_HudSensorTracker.mapLoadedFlag = 0;
    g_Hud_MapOverlayRefCount = 1;
    HudSensorTracker::OnObjectiveCommand(0x1b);
    const bool commandMapToggleOk =
        g_HudSensorTracker.mapScaleLerpActive == 0 && g_Hud_MapOverlayRefCount == 0;

    g_HudSensorTracker.mapScaleLerpActive = 1;
    g_HudSensorTracker.mapZoom = 10.0f;
    g_HudSensorTracker.mapSndClick = &completeSfx;
    HudSensorTracker::OnObjectiveCommand(0x1c);
    HudSensorTracker::OnObjectiveCommand(0x1d);
    const bool commandZoomOk = g_HudSensorTracker.mapZoom > 9.89f &&
                               g_HudSensorTracker.mapZoom < 9.91f;

    g_HudSensorTracker.hudScale = 0.875f;
    g_HudSensorTracker.firstIncompleteObjectiveIndex = 2;
    g_HudSensorTracker.objectiveUiMode = 0;
    g_HudSensorTracker.objectiveFlowState = 0;
    g_HudUiMgrObjectivePhase = 0;
    HudSensorTracker::OnObjectiveReadSoundEvent(0);
    const bool readEventShowOk =
        g_HudSensorTracker.objectiveFlowState == 0x65 &&
        g_HudSensorTracker.objectiveUiMode == 1 &&
        g_HudUiMgrObjectiveSensorRect.image == &currentImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Review title") == 0 &&
        std::strcmp(&TestFieldAt<char>(desc, 0x34), "Review description") == 0;

    globalScale = 0.25f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 0;
    g_HudUiMgrObjectivePhase = 1;
    HudSensorTracker::OnObjectiveReadSoundEvent(1);
    const bool readEventHideOk =
        g_HudSensorTracker.objectiveUiMode == 0 &&
        g_HudUiMgrObjectivePhase == 3 &&
        globalScale == 0.875f &&
        g_zSnd_Flag10PlaybackEnabled == 1;

    globalScale = 0.1f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 0;
    HudSensorTracker::OnObjectiveReadSoundEvent(2);
    const bool readEventRestoreOk =
        globalScale == 0.875f && g_zSnd_Flag10PlaybackEnabled == 1;

    globalScale = 0.2f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 0;
    HudSensorTracker::OnObjectiveReadSoundEvent(99);
    const bool readEventIgnoreOk =
        globalScale == 0.2f && g_zSnd_Flag10PlaybackEnabled == 0;

    g_zLoc_MessagesDllHandle = oldMessagesDll;
    g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_HudUiMgrObjectiveLabelTextPanel = oldLabelPanel;
    g_OptCatalog_DamageFeedbackHitCount = oldHitCount;
    g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
    g_zSnd_Flag10PlaybackEnabled = oldFlag10;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_PreInitialized = oldSndPreInitialized;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_HudSensorTracker.hudScale = oldHudScale;
    g_Time_UnscaledAccumulatedTimeSec = oldUnscaledTime;
    g_HudSensorTracker = oldGlobalTracker;
    g_HudSensorTracker_ObjectiveCommandLocked = oldObjectiveCommandLocked;
    g_Hud_MapOverlayRefCount = oldMapOverlayRefCount;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    FreeLibrary(messagesDll);
    DeleteObject(summary->hFont);
    DeleteObject(desc->hFont);
    summary->hFont = nullptr;
    desc->hFont = nullptr;

    if (!hideOk) {
        return 3;
    }
    if (!firstSlotOk) {
        return 4;
    }
    if (!currentSlotOk) {
        return 5;
    }
    if (!commandShowOk) {
        return 6;
    }
    if (!commandHideOk) {
        return 7;
    }
    if (!reviewShowOk) {
        return 9;
    }
    if (!reviewCompleteOk) {
        return 10;
    }
    if (!reviewHideOk) {
        return 11;
    }
    if (!advanceShowReviewOk) {
        return 12;
    }
    if (!advanceHideReviewOk) {
        return 13;
    }
    if (!advanceSequentialOk) {
        return 14;
    }
    if (!advanceJumpOk) {
        return 15;
    }
    if (!commandLockedOk) {
        return 16;
    }
    if (!commandAdvanceOk) {
        return 17;
    }
    if (!commandPanelOk) {
        return 18;
    }
    if (!commandMapToggleOk) {
        return 19;
    }
    if (!commandZoomOk) {
        return 20;
    }
    if (!readEventShowOk) {
        return 21;
    }
    if (!readEventHideOk) {
        return 22;
    }
    if (!readEventRestoreOk) {
        return 23;
    }
    if (!readEventIgnoreOk) {
        return 24;
    }

    return 0;
}

namespace {
const char *g_weatherTextureName = nullptr;
zVidImagePartial *g_weatherTextureImage = nullptr;
int g_weatherTextureUseAlpha = 0;
int g_weatherTextureArg3 = 0;
int g_weatherTextureArg4 = 0;
zVideo_TextureRecordPartial g_weatherTextureRecord = {};

zVideo_TextureRecordPartial *RECOIL_FASTCALL
HudWeatherFxCreateTextureRecordStub(const char *textureName, zVidImagePartial *image,
                                    int useAlpha, int arg3, int arg4) {
    g_weatherTextureName = textureName;
    g_weatherTextureImage = image;
    g_weatherTextureUseAlpha = useAlpha;
    g_weatherTextureArg3 = arg3;
    g_weatherTextureArg4 = arg4;
    return &g_weatherTextureRecord;
}
} // namespace

extern "C" int hud_weather_fx_constructor_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    zVideo_CreateTextureRecordProc const oldCreateTextureRecord = g_zVideo_pfnCreateTextureRecord;

    g_weatherTextureName = nullptr;
    g_weatherTextureImage = nullptr;
    g_weatherTextureUseAlpha = 0;
    g_weatherTextureArg3 = 0;
    g_weatherTextureArg4 = 0;
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_pfnCreateTextureRecord = HudWeatherFxCreateTextureRecordStub;

    std::srand(1);
    HudWeatherFx weather = {};
    HudWeatherFx *const result = weather.Constructor(3);

    bool particlesCopied = true;
    for (int index = 0; index < weather.particleCount; ++index) {
        const zVec3 &source = weather.particlePositions[weather.sourceBufferIndex][index];
        const zVec3 &dest = weather.particlePositions[weather.destBufferIndex][index];
        particlesCopied = particlesCopied && source.x == dest.x && source.y == dest.y &&
                          source.z == dest.z && source.z >= 0.5f && source.z <= 1.0f;
    }

    const bool quadsInvalid =
        weather.particleQuads[0].x == -1 && weather.particleQuads[0].y == -1 &&
        weather.particleQuads[0].width == -1 && weather.particleQuads[0].height == -1 &&
        weather.particleQuads[2].x == -1 && weather.particleQuads[2].y == -1 &&
        weather.particleQuads[2].width == -1 && weather.particleQuads[2].height == -1;

    const bool initialized =
        result == &weather && weather.ftable == &g_HudWeatherFx_Vtable &&
        weather.viewportRect == nullptr && weather.maxParticles == 3 &&
        weather.particleCount == 3 && weather.packedColor16 == 0x7fff &&
        weather.alphaStartScale == 1.0f && weather.alphaEndScale == 0.0500000007f &&
        weather.camera == nullptr && weather.activeParticleCount == 0 &&
        weather.sourceBufferIndex == 0 && weather.destBufferIndex == 1 &&
        weather.basisVector.x == 0.0f && weather.basisVector.y == 1.0f &&
        weather.basisVector.z == 0.0f && weather.gravity == 1.0f &&
        weather.windDirection == 0.0f && weather.windVelocity == 1.0f;

    const bool imageOk =
        weather.textureName != nullptr && std::strcmp(weather.textureName, "SnowFX") == 0 &&
        weather.softwareImage != nullptr && weather.softwareImage->formatFlagsPacked == 0x2b &&
        weather.softwareImage->width == 16 && weather.softwareImage->height == 8 &&
        weather.softwareImage->pixelCount == 128 && weather.textureRecord == &g_weatherTextureRecord &&
        g_weatherTextureName == weather.textureName &&
        g_weatherTextureImage == weather.softwareImage && g_weatherTextureUseAlpha == 2 &&
        g_weatherTextureArg3 == 1 && g_weatherTextureArg4 == 1;

    char *const alphaMap =
        weather.softwareImage != nullptr ? weather.softwareImage->alphaMap : nullptr;
    if (weather.softwareImage != nullptr) {
        zVid_Image::Destroy(weather.softwareImage);
    }
    if (alphaMap != nullptr) {
        std::free(alphaMap);
    }
    ::operator delete(weather.particleQuads);
    ::operator delete(weather.particlePositions[0]);
    ::operator delete(weather.particlePositions[1]);

    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnCreateTextureRecord = oldCreateTextureRecord;

    return initialized && quadsInvalid && particlesCopied && imageOk ? 0 : 1;
}

extern "C" int hud_weather_fx_derived_constructors_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    g_zVideo_ActiveRendererPath = 0;

    std::srand(2);
    HudWeatherFxSnow snow = {};
    HudWeatherFxSnow *const snowResult = snow.Constructor(2);
    const bool snowOk =
        snowResult == &snow && snow.ftable == &g_HudWeatherFxSnow_Vtable &&
        snow.maxParticles == 2 && snow.particleCount == 2 && snow.emitEnabled == 1 &&
        snow.emitRadius == 20.0f && snow.emitDepth == 400.0f &&
        snow.softwareImage == nullptr && snow.textureRecord == nullptr &&
        snow.particlePositions[0][0].x == snow.particlePositions[1][0].x &&
        snow.particlePositions[0][1].z == snow.particlePositions[1][1].z;

    std::srand(3);
    HudWeatherFxRain rain = {};
    HudWeatherFxRain *const rainResult = rain.Constructor(1);
    const bool rainOk =
        rainResult == &rain && rain.ftable == &g_HudWeatherFxRain_Vtable &&
        rain.maxParticles == 1 && rain.particleCount == 1 && rain.emitEnabled == 1 &&
        rain.emitRadius == 20.0f && rain.emitDepth == 400.0f &&
        rain.softwareImage == nullptr && rain.textureRecord == nullptr &&
        rain.particlePositions[0][0].y == rain.particlePositions[1][0].y;

    ::operator delete(snow.particleQuads);
    ::operator delete(snow.particlePositions[0]);
    ::operator delete(snow.particlePositions[1]);
    ::operator delete(rain.particleQuads);
    ::operator delete(rain.particlePositions[0]);
    ::operator delete(rain.particlePositions[1]);

    g_zVideo_ActiveRendererPath = oldRendererPath;
    return snowOk && rainOk ? 0 : 1;
}

extern "C" int zhud_mgr_viewport_activation_smoke(void) {
    std::int32_t *const oldReplicateOption = ZOPT_REPLICATE;
    zOpt_ViewRectSection **const oldRenderOption = g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection **const oldDisplayOption = g_zOpt_DisplaySectionOption;

    std::int32_t replicate = 0;
    ZOPT_REPLICATE = &replicate;
    g_HudUi_InvalidateMask = 0x80;
    g_layoutActivatedCount = 0;

    g_HudUiMgrSensorBlock = {};
    g_HudUiMgrSensorBlock.sensorParam = 2.0f;
    g_HudUiMgrSensorFxRect = {7, 9, 0, 0};
    g_HudUiMgrSensorFxViewportWidth = 100;
    g_HudUiMgrSensorFxViewportHeight = 80;
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveWidget.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveWidget.flags = 0x10;
    g_HudUiMgrObjectiveBar.ftable =
        reinterpret_cast<const HudUiBar_FTable *>(&g_HudUiCommon_FTable);
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiCommon_FTable);

    HudUiTopMessageStack top{};
    HudUiChatMessageStack chat{};
    top.Constructor();
    chat.Constructor();
    g_HudUiTopMessageStack = &top;
    g_HudUiChatMessageStack = &chat;
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;

    HudLayoutBase_FTable layoutTable{};
    layoutTable.OnActivated = TestLayoutOnActivated;
    HudLayoutBase layout{&layoutTable};
    g_HudUiMgrCurrentLayout = &layout;

    HudUiShieldMessageWidgetState shield{5, 6};
    g_HudUiMgrShieldMessageWidget = &shield;

    zOpt_ViewRectSection renderSection{};
    zOpt_ViewRectSection displaySection{};
    renderSection.x = 0;
    renderSection.y = 20;
    renderSection.maxXInclusive = 100;
    renderSection.maxYInclusive = 90;
    displaySection.y = 100;
    zOpt_ViewRectSection *renderSectionPtr = &renderSection;
    zOpt_ViewRectSection *displaySectionPtr = &displaySection;
    g_zOpt_RenderSectionOption = &renderSectionPtr;
    g_zOpt_DisplaySectionOption = &displaySectionPtr;

    float noReplicatePoint[2] = {80.0f, 150.0f};
    HudUiMgr::ScreenToWorld(noReplicatePoint);
    const bool screenToWorldNoReplicate =
        noReplicatePoint[0] == 80.0f && noReplicatePoint[1] == 150.0f;

    replicate = 1;
    float replicatePoint[2] = {80.0f, 150.0f};
    HudUiMgr::ScreenToWorld(replicatePoint);
    const bool screenToWorldReplicate = replicatePoint[0] == 40.0f && replicatePoint[1] == 45.0f;

    float clampedPoint[2] = {300.0f, 500.0f};
    HudUiMgr::ScreenToWorld(clampedPoint);
    const bool screenToWorldClamped = clampedPoint[0] == 100.0f && clampedPoint[1] == 90.0f;

    HudUiMgr::ReticleStaticAtexitStub();
    g_HudUiMgrReticleProjection[0] = 1.25f;
    g_HudUiMgrReticleProjection[1] = -2.5f;
    g_HudUiMgrReticleProjection[2] = 3.75f;
    float reticleProjection[3] = {};
    HudUiMgr::CopyReticleProjection(reticleProjection);
    HudUiMgr::SetReticleMode(2);
    const bool reticleUtilities = reticleProjection[0] == 1.25f && reticleProjection[1] == -2.5f &&
                                  reticleProjection[2] == 3.75f && g_HudUiMgrReticleMode == 2;

    replicate = 0;
    HudUiRect hud{10, 20, 330, 260};
    HudUiRect view{0, 0, 640, 480};
    HudUiMgr::ActivateHud(&hud, &view);

    const bool viewport =
        g_HudUiMgrHudRect.left == 10 && g_HudUiMgrHudRect.top == 20 &&
        g_HudUiMgrHudRect.right == 330 && g_HudUiMgrHudRect.bottom == 260 &&
        g_HudUiMgrViewRect.right == 640 && g_HudUiMgrViewRect.bottom == 480 &&
        g_HudUiMgrHudRectW == 320.0f && g_HudUiMgrHudRectH == 240.0f &&
        g_HudUiMgrReticleMapBiasX == 10.0f && g_HudUiMgrReticleMapBiasY == 20.0f &&
        g_HudUiMgrReticleMapScaleHalfW == 160.0f && g_HudUiMgrReticleMapScaleHalfH == 120.0f &&
        g_HudUiMgrReticleSnapRadiusSq == 4096 && g_HudUiMgrSensorBlock.sensorRectRaw.left == 7 &&
        g_HudUiMgrSensorBlock.sensorRectRaw.top == 9 &&
        g_HudUiMgrSensorBlock.sensorRectRaw.right == 107 &&
        g_HudUiMgrSensorBlock.sensorRectRaw.bottom == 89 && g_layoutActivatedCount == 1 &&
        (g_HudUiMgrObjectiveWidget.flags & 0x10) == 0 && shield.state == 0 &&
        shield.viewportResetFrame == -1 && g_HudUiMgrSensorBlock.state == 1;

    bool stacks = true;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const topLine = TextStackLineAt(&top, index);
        HudUiPanel *const chatLine = TextStackLineAt(&chat, index);
        stacks = stacks && TestFieldAt<std::int32_t>(topLine, 0x14) == 320 &&
                 TestFieldAt<std::int32_t>(chatLine, 0x14) == 320 &&
                 TestFieldAt<std::int32_t>(chatLine, 0x18) == 0x158 - index * 0x12 &&
                 TestFieldAt<std::uint32_t>(topLine, 0x14c) == 0x0020bf40 &&
                 TestFieldAt<std::uint32_t>(chatLine, 0x150) == 0x0020bf40;
    }

    DeleteTextStackLineFonts(&top);
    DeleteTextStackLineFonts(&chat);
    g_HudUiTopMessageStack = nullptr;
    g_HudUiChatMessageStack = nullptr;
    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgrShieldMessageWidget = nullptr;
    g_HudUi_InvalidateMask = 0;
    g_HudUiMgrReticleProjection[0] = 0.0f;
    g_HudUiMgrReticleProjection[1] = 0.0f;
    g_HudUiMgrReticleProjection[2] = 0.0f;
    g_HudUiMgrReticleMode = 0;
    ZOPT_REPLICATE = oldReplicateOption;
    g_zOpt_RenderSectionOption = oldRenderOption;
    g_zOpt_DisplaySectionOption = oldDisplayOption;
    return viewport && stacks && screenToWorldNoReplicate && screenToWorldReplicate &&
                   screenToWorldClamped && reticleUtilities
               ? 0
               : 1;
}

extern "C" int zhud_mgr_project_point_to_normalized_clamped_smoke(void) {
    int matrixIdentityFlags[2] = {};
    float *matrixSlots[2] = {};
    zMat4x3 baseMatrix{};
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    std::int32_t *const oldReplicateOption = ZOPT_REPLICATE;

    zMath::g_currentMatrixIdentityFlagSlot = &matrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    matrixSlots[0] = reinterpret_cast<float *>(&baseMatrix);
    zMath::g_zMath_CameraScratchB = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};

    g_zMath_ProjScaleX = 100.0f;
    g_zMath_ProjScaleY = -50.0f;
    g_zMath_ProjOffsetX = 320.0f;
    g_zMath_ProjOffsetY = 240.0f;
    gClipRect_Primary.zMin = 1.0f;
    gClipRect_Primary.xMaxAlt = 640.0f;
    g_zVideo_ProjectClipLeft = 0.0f;
    g_zVideo_ProjectClipTop = 0.0f;
    g_zVideo_ProjectClipRight = 640.0f;
    g_zVideo_ProjectClipBottom = 480.0f;
    g_HudUiMgrHudRect.top = 0;
    g_HudUiMgrHudRectW = 640.0f;
    g_HudUiMgrHudRectH = 480.0f;

    std::int32_t replicate = 0;
    ZOPT_REPLICATE = &replicate;
    zVec3 worldPoint = {0.0f, 0.0f, 10.0f};
    zVec3 projected = {};
    bool ok = HudUiMgr::ProjectPointToNormalizedClamped(&worldPoint, &projected) == 0 &&
              projected.x == 0.0f && projected.y == 0.0f;

    replicate = 1;
    worldPoint = {-16.0f, 24.0f, 10.0f};
    ok = ok && HudUiMgr::ProjectPointToNormalizedClamped(&worldPoint, &projected) == 0 &&
         projected.x == 0.0f && projected.y == 0.0f;

    replicate = 0;
    worldPoint = {-100.0f, 100.0f, 10.0f};
    ok = ok && HudUiMgr::ProjectPointToNormalizedClamped(&worldPoint, &projected) == 0 &&
         projected.x == -1.0f && projected.y == -1.0f;

    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    ZOPT_REPLICATE = oldReplicateOption;
    return ok ? 0 : 1;
}

extern "C" int zhud_mgr_update_target_reticle_smoke(void) {
    zOpt_ViewRectSection *const oldRenderSection =
        g_zOpt_RenderSectionOption != nullptr ? *g_zOpt_RenderSectionOption : nullptr;
    zOpt_ViewRectSection *const oldDisplaySection =
        g_zOpt_DisplaySectionOption != nullptr ? *g_zOpt_DisplaySectionOption : nullptr;
    zOpt_ViewRectSection **const oldRenderOption = g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection **const oldDisplayOption = g_zOpt_DisplaySectionOption;
    std::int32_t *const oldReplicateOption = ZOPT_REPLICATE;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zClass_TypeListLink *const oldTypeListHead0 = zClass_TypeList::Head(0);

    g_HudUi_InvalidateMask = 0;
    g_HudUiMgrReticleWidget = {};
    g_HudUiMgrReticleWidget.ftable = &g_HudUiWidget_FTable;

    zVec3 worldHitPoint{};
    HudUiMgr::UpdateTargetReticleFromCursor(0, &worldHitPoint, 0.0f, 0.0f);
    const bool hidden = (g_HudUiMgrReticleWidget.flags & 0x10) != 0;

    HudUiMgr::UpdateTargetReticleFromCursor(1, &worldHitPoint, 0.0f, 0.0f);
    const bool visible = (g_HudUiMgrReticleWidget.flags & 0x10) == 0;

    zVidImagePartial currentImage{};
    currentImage.width = 10;
    currentImage.height = 8;
    zVidImagePartial missImage{};
    g_HudUiMgrReticleWidget.image = &currentImage;
    g_HudUiMgrReticleWidgetHalfW = 5;
    g_HudUiMgrReticleWidgetHalfH = 4;
    g_HudUiMgrReticleMapBiasX = 10.0f;
    g_HudUiMgrReticleMapBiasY = 20.0f;
    g_HudUiMgrReticleMapScaleHalfW = 100.0f;
    g_HudUiMgrReticleMapScaleHalfH = 50.0f;
    g_HudUiMgrReticleImages[0] = nullptr;
    g_HudUiMgrReticleImages[1] = &missImage;
    g_HudUiMgrReticleImages[2] = nullptr;
    g_HudLayoutHW.reticleClipInitFlags = 0;
    g_HudLayoutHW.reticleClipRect = {};

    zOpt_ViewRectSection renderSection{};
    renderSection.x = 0;
    renderSection.y = 0;
    renderSection.rightExclusive = 100;
    renderSection.bottomExclusive = 80;
    zOpt_ViewRectSection displaySection{};
    displaySection.x = 0;
    displaySection.y = 0;
    displaySection.rightExclusive = 200;
    displaySection.bottomExclusive = 150;
    zOpt_ViewRectSection *renderSectionPtr = &renderSection;
    zOpt_ViewRectSection *displaySectionPtr = &displaySection;
    g_zOpt_RenderSectionOption = &renderSectionPtr;
    g_zOpt_DisplaySectionOption = &displaySectionPtr;
    std::int32_t replicate = 0;
    ZOPT_REPLICATE = &replicate;

    zMath::g_zMath_CameraScratchA = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    g_zMath_ProjOffsetX = 0.0f;
    g_zMath_ProjOffsetY = 0.0f;
    g_zMath_InvProjScaleX = 0.0f;
    g_zMath_InvProjScaleY = 0.0f;

    zClass_CameraDataPartial cameraData{};
    cameraData.nearClip = 2.0f;
    zClass_NodePartial cameraNode{};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    g_MainCamera = &cameraNode;

    OptCatalogEntryDef optEntry{};
    optEntry.range = 50.0f;
    zClass_NodePartial rootNode{};
    rootNode.flags = 0x10;
    zClass_NodePartial projectileNode{};
    projectileNode.flags = 0x10;
    TestReticleAttachState attachState{};
    attachState.projectileNode = &projectileNode;
    TestReticleAltGunController altGun{};
    altGun.optCatalogEntry = &optEntry;
    altGun.attachState = &attachState;
    TestReticlePlayerState playerState{};
    playerState.cameraState = 7;
    playerState.activeAltGunController = &altGun;
    playerState.rootNode = &rootNode;
    zInput_GameStateOrMapTablePartial gameState{};
    gameState.playerState = reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    g_GameStateOrMapTable = &gameState;

    zClass_WorldDataPartial worldData{};
    zClass_NodePartial runtimeScene{};
    runtimeScene.classData = &worldData;
    g_Player_RuntimeDiScene = &runtimeScene;
    zClass_TypeList::Head(0) = nullptr;
    g_HudUiMgrSensorBlock.sensorClampHalfW = 10.0f;
    g_HudUiMgrSensorBlock.sensorClampHalfH = 12.0f;

    worldHitPoint = {};
    HudUiMgr::UpdateTargetReticleFromCursor(2, &worldHitPoint, -0.5f, 0.0f);

    const bool mode2 =
        g_HudUiMgrReticleProjectedX == 60 && g_HudUiMgrReticleProjectedY == 70 &&
        g_HudUiMgrReticleWidget.x == 55 && g_HudUiMgrReticleWidget.y == 66 &&
        g_HudUiMgrReticleWidget.image == &missImage &&
        g_HudUiMgrReticleWidget.bltClipRectOrNull == &g_HudLayoutHW.reticleClipRect &&
        g_HudLayoutHW.reticleClipRect.left == 0 && g_HudLayoutHW.reticleClipRect.top == 0 &&
        g_HudLayoutHW.reticleClipRect.right == 10 && g_HudLayoutHW.reticleClipRect.bottom == 8 &&
        worldHitPoint.x == 0.0f && worldHitPoint.y == 0.0f && worldHitPoint.z == 50.0f &&
        g_HudUiMgrReticleProjection[2] == 50.0f && (rootNode.flags & 0x10) == 0 &&
        (projectileNode.flags & 0x10) != 0 && gClipRect_Alt.xMin == 50.0f &&
        gClipRect_Alt.yMin == 56.0f && gClipRect_Alt.xMax == 70.0f && gClipRect_Alt.yMax == 80.0f;

    zClass_TypeList::Head(0) = oldTypeListHead0;
    g_GameStateOrMapTable = oldGameState;
    g_MainCamera = oldMainCamera;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_zOpt_RenderSectionOption = oldRenderOption;
    g_zOpt_DisplaySectionOption = oldDisplayOption;
    if (g_zOpt_RenderSectionOption != nullptr) {
        *g_zOpt_RenderSectionOption = oldRenderSection;
    }
    if (g_zOpt_DisplaySectionOption != nullptr) {
        *g_zOpt_DisplaySectionOption = oldDisplaySection;
    }
    ZOPT_REPLICATE = oldReplicateOption;
    g_DiPickCandidateBuffer = nullptr;
    g_DiPickCandidateCursor = nullptr;
    g_HudUiMgrReticleWidget = {};
    for (zVidImagePartial *&image : g_HudUiMgrReticleImages) {
        image = nullptr;
    }

    return hidden && visible && mode2 ? 0 : 1;
}

extern "C" int zhud_mgr_init_layouts_reentry_smoke(void) {
    g_HudUiMgrLayoutDelayFrames = 2;
    const bool tickActive = HudUiMgr::TickLayoutDelay() == 1 && g_HudUiMgrLayoutDelayFrames == 1;
    g_HudUiMgrLayoutDelayFrames = 0;
    const bool tickIdle = HudUiMgr::TickLayoutDelay() == 0;

    g_HudUiMgrHudLayoutsInitialized = 1;
    const std::int32_t result = HudUiMgr::InitHudLayouts(nullptr, nullptr);
    const bool unchanged = g_HudUiMgrHudLayoutsInitialized == 1;
    g_HudUiMgrHudLayoutsInitialized = 0;
    return tickActive && tickIdle && result == 1 && unchanged ? 0 : 1;
}

extern "C" int zhud_mgr_shutdown_resources_smoke(void) {
    g_HudUiMgrTimerPanelFloat = nullptr;
    g_HudUiMgrStringMenu = nullptr;
    g_HudUiMgrShieldMessageWidget = nullptr;
    g_HudUiMgrObjectiveCounterTextPanel = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    g_HudUiMgrStatsList = nullptr;
    g_HudUiMgrObjectiveSummaryTextPanel = nullptr;
    g_HudUiMgrObjectiveDescTextPanel = nullptr;
    g_HudUiMgrObjectiveLabelTextPanel = nullptr;
    g_HudUiTopMessageStack = nullptr;
    g_HudUiChatMessageStack = nullptr;

    g_HudUiMgrReticleImages[0] = &zVid_Image::g_zImage_DefaultImage;
    g_HudUiMgrReticleImages[1] = nullptr;
    g_HudUiMgrReticleImages[2] = &zVid_Image::g_zImage_DefaultImage;
    g_HudUiMgrSensorTargetMarkerImages[0] = &zVid_Image::g_zImage_DefaultImage;
    g_HudUiMgrSensorTargetMarkerImages[1] = nullptr;
    g_HudUiMgrSensorTargetMarkerImages[2] = &zVid_Image::g_zImage_DefaultImage;
    g_HudUiMgrSensorTargetMarkerImages[3] = nullptr;
    g_HudUiMgrSensorTargetMarkerImages[4] = &zVid_Image::g_zImage_DefaultImage;

    g_HudUiMgrMessages[1].variantImages[0] = &zVid_Image::g_zImage_DefaultImage;
    g_HudUiMgrMessages[1].sideImageSwaps[1] = &zVid_Image::g_zImage_DefaultImage;
    TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[0], 0xbc) =
        &zVid_Image::g_zImage_DefaultImage;
    g_HudLayoutHW.widget1Image320 = &zVid_Image::g_zImage_DefaultImage;
    g_HudLayoutHW.widget2Image400 = &zVid_Image::g_zImage_DefaultImage;
    g_HudUiMgrHudLayoutsInitialized = 1;
    g_HudUiMgrHudLoaded = 1;

    HudUiMgr::ShutdownResources();

    const bool cleared =
        g_HudUiMgrMessages[1].variantImages[0] == nullptr &&
        g_HudUiMgrMessages[1].sideImageSwaps[1] == nullptr &&
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[0], 0xbc) == nullptr &&
        g_HudLayoutHW.widget1Image320 == nullptr && g_HudLayoutHW.widget2Image400 == nullptr &&
        g_HudUiMgrHudLayoutsInitialized == 0 && g_HudUiMgrHudLoaded == 0;

    for (zVidImagePartial *&image : g_HudUiMgrReticleImages) {
        image = nullptr;
    }

    for (zVidImagePartial *&image : g_HudUiMgrSensorTargetMarkerImages) {
        image = nullptr;
    }

    return cleared ? 0 : 1;
}

extern "C" int zhud_sensor_track_list_add_smoke(void) {
    g_HudUiMgrSensor_TrackList = {};
    g_HudUiMgrSensor_TrackList.trackListAux = 77;
    int payloadA = 11;
    int payloadB = 22;

    HudUiMgrSensorTrackNode *const nodeA =
        HudUiMgrSensor::TrackList_Add(HUD_SENSOR_TRACK_KIND_PLAYER, &payloadA);
    const bool first =
        nodeA != nullptr && nodeA->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER &&
        nodeA->payload == &payloadA && nodeA->next == nullptr &&
        g_HudUiMgrSensor_TrackList.head == nodeA && g_HudUiMgrSensor_TrackList.tail == nodeA &&
        g_HudUiMgrSensor_TrackList.count == 1 && g_HudUiMgrSensor_TrackList.trackListAux == 77;

    HudUiMgrSensorTrackNode *const nodeB =
        HudUiMgrSensor::TrackList_Add(HUD_SENSOR_TRACK_KIND_TURRET, &payloadB);
    const bool second =
        nodeB != nullptr && nodeB->trackKind == HUD_SENSOR_TRACK_KIND_TURRET &&
        nodeB->payload == &payloadB && nodeB->next == nullptr && nodeA->next == nodeB &&
        g_HudUiMgrSensor_TrackList.head == nodeA && g_HudUiMgrSensor_TrackList.tail == nodeB &&
        g_HudUiMgrSensor_TrackList.count == 2 && g_HudUiMgrSensor_TrackList.trackListAux == 77;

    std::free(nodeA);
    std::free(nodeB);
    g_HudUiMgrSensor_TrackList = {};
    return first && second ? 0 : 1;
}
