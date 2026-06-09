#include "GameZRecoil/zHud/zhud_ui.h"
#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" unsigned int g_HudUi_InvalidateMask;
extern "C" int g_RecoilApp_QuitAfterCredits;
extern RecoilApp g_RecoilApp;

namespace {
struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

template <typename Method> void *MethodAddress(Method method) {
    union MethodToFunction {
        Method method;
        void *function;
    };

    MethodToFunction thunk{};
    thunk.method = method;
    return thunk.function;
}

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(
        patch.original,
        patch.address,
        sizeof(patch.original)
    );

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
        patch.address = nullptr;
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t relativeOffset =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(patch.address + sizeof(patch.original));
    *reinterpret_cast<std::int32_t *>(patch.address + 1) =
        static_cast<std::int32_t>(relativeOffset);

    DWORD ignored = 0;
    VirtualProtect(
        patch.address,
        sizeof(patch.original),
        oldProtect,
        &ignored
    );
    FlushInstructionCache(
        GetCurrentProcess(),
        patch.address,
        sizeof(patch.original)
    );
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        std::memcpy(
            patch.address,
            patch.original,
            sizeof(patch.original)
        );
        DWORD ignored = 0;
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            oldProtect,
            &ignored
        );
    }
    FlushInstructionCache(
        GetCurrentProcess(),
        patch.address,
        sizeof(patch.original)
    );
    patch.address = nullptr;
}

void DestroyHudCmdDialogDescriptionPanelForSmoke(
    HudCmdDialog &dialog
) {
    dialog.descriptionPanel.HudUiPanel::~HudUiPanel();
    dialog.descriptionPanel.textPick = nullptr;
    dialog.descriptionPanel.hFont = nullptr;
}

int g_optionsDialogLoadCalls;
bool g_optionsDialogLoadArgsOk;
int g_creditsPanelLoadCalls;
bool g_creditsPanelLoadArgsOk;
int g_cmdDialogLoadCalls;
bool g_cmdDialogLoadArgsOk;
int g_cmdDialogRebuildCalls;
int g_cmdDialogRebuildGroup;
int g_cmdDialogSetChildFlagsCalls;
int g_cmdDialogSetChildFlagsValue;
int g_cmdResetDefaultBindingCalls;
int g_cmdResetLookupRebuildCalls;
int g_cmdResetBaseActivateCalls;
int g_elementBlitCount;
zVidImagePartial *g_elementBlitImage;
int g_elementBlitX;
int g_elementBlitY;
int g_elementBlitFlags;
zVidRect32 g_elementBlitRect;
int g_elementBlitHasRect;
int g_tripletPanelBlitCount;
zVidImagePartial *g_tripletPanelBlitImages[4];
int g_layoutActivatedCount;
int g_widgetInvalidateRectGetCenterXCount;
int g_widgetInvalidateRectGetCenterYCount;
int g_widgetInvalidateRectInvalidateCount;
void *g_widgetInvalidateRectInvalidateThis;
int g_elementUpdateDrawCount;
int g_elementUpdateDrawBaseCount;
int g_elementUpdateInvalidateCount;
int g_circleDrawBaseCount;
int g_circlePointOpCount;
void *g_circlePointOpFrameBuffer;
int g_circlePointOpArgs[3];

struct CaptureActivatedLayout : HudLayoutBase {
    void OnActivated();
};

struct TestWidgetInvalidateRect : HudUiWidget {
    virtual int GetCenterX();
    virtual int GetCenterY();
    virtual void Invalidate();
};

struct TestElementUpdateElement : HudUiElement {
    void Draw();
    void DrawBase();
    void Invalidate();
};

struct TestCircleDrawDirtyOps : HudUiCircle {
    void DrawBase();
};

void __fastcall CaptureHudElementBlit(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
) {
    ++g_elementBlitCount;
    g_elementBlitImage = image;
    g_elementBlitX = dstX;
    g_elementBlitY = dstY;
    g_elementBlitFlags = clipFlags;
    g_elementBlitHasRect = srcRect != nullptr ? 1 : 0;
    if (srcRect != nullptr) {
        g_elementBlitRect = *srcRect;
    }
}

void __fastcall CaptureTripletPanelBlit(
    zVidImagePartial *image,
    int,
    int,
    int,
    zVidRect32 *
) {
    const int index = g_tripletPanelBlitCount;
    if (index < 4) {
        g_tripletPanelBlitImages[index] = image;
    }
    ++g_tripletPanelBlitCount;
}

void CaptureActivatedLayout::OnActivated() {
    ++g_layoutActivatedCount;
}

int TestWidgetInvalidateRect::GetCenterX() {
    ++g_widgetInvalidateRectGetCenterXCount;
    return 7;
}

int TestWidgetInvalidateRect::GetCenterY() {
    ++g_widgetInvalidateRectGetCenterYCount;
    return 11;
}

void TestWidgetInvalidateRect::Invalidate() {
    ++g_widgetInvalidateRectInvalidateCount;
    g_widgetInvalidateRectInvalidateThis = this;
}

void TestElementUpdateElement::Draw() {
    ++g_elementUpdateDrawCount;
}

void TestElementUpdateElement::DrawBase() {
    ++g_elementUpdateDrawBaseCount;
}

void TestElementUpdateElement::Invalidate() {
    ++g_elementUpdateInvalidateCount;
    HudUiElement::Invalidate();
}

void TestCircleDrawDirtyOps::DrawBase() {
    ++g_circleDrawBaseCount;
}

void __fastcall CaptureCirclePointOp(
    void *frameBuffer,
    int y,
    int x,
    int color16
) {
    ++g_circlePointOpCount;
    g_circlePointOpFrameBuffer = frameBuffer;
    g_circlePointOpArgs[0] = y;
    g_circlePointOpArgs[1] = x;
    g_circlePointOpArgs[2] = color16;
}

struct OptionsDialogLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * OptionsDialogLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_optionsDialogLoadCalls;
    g_optionsDialogLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(
            zrdPath,
            "dialog.zrd"
        ) == 0 &&
        sectionName != nullptr &&
        std::strcmp(
            sectionName,
            "OPTIONSPANEL"
        ) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *HudUiBackgroundLoadFromZrdAddress() {
    return MethodAddress(&HudUiBackground::LoadFromZrd);
}

void *OptionsDialogLoadProbeAddress() {
    return MethodAddress(&OptionsDialogLoadProbe::LoadFromZrd);
}

struct CreditsPanelLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * CreditsPanelLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_creditsPanelLoadCalls;
    g_creditsPanelLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(
            zrdPath,
            "dialog.zrd"
        ) == 0 &&
        sectionName != nullptr &&
        std::strcmp(
            sectionName,
            "CREDITSPANEL"
        ) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *CreditsPanelLoadProbeAddress() {
    return MethodAddress(&CreditsPanelLoadProbe::LoadFromZrd);
}

template <typename T> T &ZhudFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(base) + offset);
}

bool ZhudFloatNear(float actual, float expected) {
    const float delta = actual - expected;
    return delta > -0.0001f && delta < 0.0001f;
}

HudUiPanelLayoutEntry *AllocateCreditsPanelEntries(int count) {
    HudUiPanelLayoutEntry *const entries =
        static_cast<HudUiPanelLayoutEntry *>(
            ::operator new(sizeof(HudUiPanelLayoutEntry) * count)
        );
    std::memset(
        entries,
        0,
        sizeof(HudUiPanelLayoutEntry) * count
    );
    return entries;
}

void InitCreditsPanelEntry(
    HudUiPanelLayoutEntry *entry,
    const char *text,
    int x,
    int y
) {
    entry->panel.ConstructorDefault(
        text,
        x,
        y
    );
    entry->layoutX = x;
    entry->layoutY = y;
}

bool CreditsPanelEntryMatches(
    const HudUiPanelLayoutEntry &entry,
    const char *text,
    int x,
    int y
) {
    return entry.layoutX == x && entry.layoutY == y &&
           std::strcmp(entry.panel.textBuffer, text) == 0;
}

int g_PanelLayoutDestroyCount = 0;
HudUiPanel *g_PanelLayoutDestroyPanels[4] = {};

void __fastcall CountPanelDestructorThunk(HudUiPanel *panel) {
    if (g_PanelLayoutDestroyCount < 4) {
        g_PanelLayoutDestroyPanels[g_PanelLayoutDestroyCount] = panel;
    }

    ++g_PanelLayoutDestroyCount;
}

void InitSingleCreditsPanelSpan(
    HudUiPanelSpan *span,
    const char *text,
    int x,
    int y
) {
    span->allocatorProxy = 0;
    span->begin = AllocateCreditsPanelEntries(1);
    span->end = span->begin + 1;
    span->cap = span->end;
    InitCreditsPanelEntry(
        &span->begin[0],
        text,
        x,
        y
    );
}

void InitScrollingCreditsTextForDestructor(
    HudUiZrdScrollingText *text
) {
    new (text) HudUiZrdScrollingText;
    text->rows.begin =
        static_cast<HudUiPanelSpan *>(::operator new(sizeof(HudUiPanelSpan) * 2));
    text->rows.end = text->rows.begin + 2;
    text->rows.cap = text->rows.end;
    InitSingleCreditsPanelSpan(
        &text->rows.begin[0],
        "scroll a",
        100,
        200
    );
    InitSingleCreditsPanelSpan(
        &text->rows.begin[1],
        "scroll b",
        101,
        201
    );
}

int ScrollingCreditsTextDestructorFailureBits(
    const HudUiZrdScrollingText &text
) {
    int failure = 0;
    failure |= text.rows.begin == nullptr ? 0 : 1;
    failure |= text.rows.end == nullptr ? 0 : 2;
    failure |= text.rows.cap == nullptr ? 0 : 4;
    return failure;
}

void InitCreditsPanelForDestructor(
    HudUiCreditsPanel *panel
) {
    new ((HudUiBackground *)panel) HudUiBackground;
    new (&panel->backButton) HudUiCreditsBackButton;
    new (&panel->quitButton) HudUiCreditsQuitButton;
    InitScrollingCreditsTextForDestructor(&panel->creditsScreen);
}

int CreditsPanelDestructorFailureBits(
    const HudUiCreditsPanel &panel
) {
    int failure = 0;
    failure |= panel.creditsScreen.rows.begin == nullptr ? 0 : 1;
    failure |= panel.creditsScreen.rows.end == nullptr ? 0 : 2;
    failure |= panel.creditsScreen.rows.cap == nullptr ? 0 : 4;
    return failure;
}

RecoilApp_StateQueueItem *CreditsQueueItemAt(
    RecoilApp_StateQueue &queue,
    int index
) {
    if (index < 0 || index >= queue.m_itemCount || queue.m_readBlock.m_cursor == nullptr) {
        return nullptr;
    }

    return queue.m_readBlock.m_cursor[index];
}

void CleanupCreditsQueue(
    RecoilApp_StateQueue &queue
) {
    const int itemCount = queue.m_itemCount;
    for (int index = 0; index < itemCount; ++index) {
        ::operator delete(CreditsQueueItemAt(
            queue,
            index
        ));
    }

    if (queue.m_chunkBaseList != nullptr) {
        if (queue.m_readBlock.m_chunkBaseSlot != nullptr &&
            queue.m_writeBlock.m_chunkBaseSlot != nullptr) {
            for (RecoilApp_StateQueueItem ***slot = queue.m_readBlock.m_chunkBaseSlot;
                 slot <= queue.m_writeBlock.m_chunkBaseSlot;
                 ++slot) {
                ::operator delete(*slot);
            }
        }
        ::operator delete(queue.m_chunkBaseList);
    }

    std::memset(
        &queue,
        0,
        sizeof(queue)
    );
}

void InitCreditsPanelForUpdate(
    HudUiCreditsPanel *panel,
    float progress,
    float step
) {
    new ((HudUiBackground *)panel) HudUiBackground;
    new (&panel->creditsScreen) HudUiZrdScrollingText;
    panel->creditsScreen.rows.begin = nullptr;
    panel->creditsScreen.rows.end = nullptr;
    panel->creditsScreen.rows.cap = nullptr;
    panel->fadeProgress = progress;
    panel->fadeStep = step;
}

struct CreditsLeaveNetworkState : RecoilApp_IState {
    void OnEnter() {
    }
};

struct CmdDialogLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * CmdDialogLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_cmdDialogLoadCalls;
    g_cmdDialogLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(zrdPath, "dialog.zrd") == 0 &&
        sectionName != nullptr &&
        std::strcmp(sectionName, "COMMANDS_DIALOG") == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *CmdDialogLoadProbeAddress() {
    return MethodAddress(&CmdDialogLoadProbe::LoadFromZrd);
}

struct CmdDialogRebuildProbe {
    void RebuildCommandBindingListsForGroup(int groupIndex);
};

void CmdDialogRebuildProbe::RebuildCommandBindingListsForGroup(
    int groupIndex
) {
    ++g_cmdDialogRebuildCalls;
    g_cmdDialogRebuildGroup = groupIndex;
}

void *CmdDialogRebuildProbeAddress() {
    return MethodAddress(&CmdDialogRebuildProbe::RebuildCommandBindingListsForGroup);
}

struct HudUiContainerSetChildFlagsProbe {
    void SetChildFlags(int flags);
};

void HudUiContainerSetChildFlagsProbe::SetChildFlags(
    int flags
) {
    ++g_cmdDialogSetChildFlagsCalls;
    g_cmdDialogSetChildFlagsValue = flags;
}

void *HudUiContainerSetChildFlagsProbeAddress() {
    return MethodAddress(&HudUiContainerSetChildFlagsProbe::SetChildFlags);
}

int __fastcall FakeHudCmdBindGroupCount() {
    return 0;
}

void __fastcall FakeHudCmdInitDefaultBindings() {
    ++g_cmdResetDefaultBindingCalls;
}

void __fastcall FakeHudCmdRebuildLookupIndices() {
    ++g_cmdResetLookupRebuildCalls;
}

struct HudCmdZrdActivateProbe {
    void OnActivate();
};

void HudCmdZrdActivateProbe::OnActivate() {
    ++g_cmdResetBaseActivateCalls;
}

void *HudCmdZrdActivateProbeAddress() {
    return MethodAddress(&HudCmdZrdActivateProbe::OnActivate);
}

char *__fastcall FakeHudCmdCommandLabel(int commandId) {
    return commandId == 7 ? const_cast<char *>("CommandSeven")
                          : const_cast<char *>("CommandFive");
}

char *__fastcall FakeHudCmdCommandHint(int commandId) {
    return commandId == 7 ? const_cast<char *>("HintSeven")
                          : const_cast<char *>("HintFive");
}

void SetupHudCmdButton(
    HudCmdBindButtonBase &button,
    const char *firstText,
    const char *secondText,
    int firstCommandId,
    int secondCommandId
) {
    button.AddBindingEntry(
        firstText,
        firstCommandId
    );
    button.AddBindingEntry(
        secondText,
        secondCommandId
    );
}

void CleanupHudCmdButton(HudCmdBindButtonBase &button) {
    button.DestructorCore();
}

void InstallHudCmdDialogBinding(
    HudCmdBindButtonBase &button,
    const char *text
) {
    HudCmdBindingEntry *const entry =
        static_cast<HudCmdBindingEntry *>(::operator new(sizeof(HudCmdBindingEntry)));
    entry->displayText = _strdup(text);
    entry->commandId = 5;

    HudCmdBindingEntry **const slots =
        static_cast<HudCmdBindingEntry **>(::operator new(sizeof(HudCmdBindingEntry *)));
    slots[0] = entry;
    button.bindingVec.begin = slots;
    button.bindingVec.end = slots + 1;
    button.bindingVec.capacity = slots + 1;
}

void SetupHudCmdDialogButtons(HudCmdDialog &dialog) {
    SetupHudCmdButton(
        dialog.commandList,
        "CommandZero",
        "CommandSeven",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.keyAButton,
        "KeyA0",
        "KeyA1",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.keyBButton,
        "KeyB0",
        "KeyB1",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.joyButton,
        "Joy0",
        "Joy1",
        5,
        7
    );
    SetupHudCmdButton(
        dialog.mouseButton,
        "Mouse0",
        "Mouse1",
        5,
        7
    );
}

void CleanupHudCmdDialogButtons(HudCmdDialog &dialog) {
    CleanupHudCmdButton(dialog.mouseButton);
    CleanupHudCmdButton(dialog.joyButton);
    CleanupHudCmdButton(dialog.keyBButton);
    CleanupHudCmdButton(dialog.keyAButton);
    CleanupHudCmdButton(dialog.commandList);
}

void InitHudCmdInputTables() {
    zInput::BindMap_InitDikKeyNameTable();
    zInput::BindMap_InitJoystickButtonNameTable();
    zInput::BindMap_InitMouseButtonNameTable();
}

const void *HudElementVptr(const HudUiElement &element) {
    return *reinterpret_cast<const void *const *>(&element);
}

void SetHudElementVptr(
    HudUiElement &element,
    const void *vptr
) {
    *reinterpret_cast<const void **>(&element) = vptr;
}
} // namespace

extern "C" int zhud_element_invalidate_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    HudUiElement element{};
    element.flags = 0x01;
    g_HudUi_InvalidateMask = 0x24;
    element.Invalidate();
    const bool ok = element.flags == 0x25;

    g_HudUi_InvalidateMask = oldMask;
    return ok ? 0 : 1;
}

extern "C" int zhud_element_visible_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;

    HudUiElement element{};
    element.flags = 0x10;
    element.SetVisible(1);
    const bool visible = element.flags == 0x80;

    element.SetVisible(0);
    const bool hidden = element.flags == 0x90;

    g_HudUi_InvalidateMask = oldMask;
    return visible && hidden ? 0 : 1;
}

extern "C" int zhud_circle_constructor_and_hit_test_smoke(void) {
    HudUiCircle circle{};
    HudUiCircle *const result = circle.Constructor(
        10,
        20,
        5,
        0x07e0
    );

    const bool constructed =
        result == &circle &&
        circle.x == 10 &&
        circle.y == 20 &&
        circle.radius == 5 &&
        circle.radiusSquared == 25 &&
        circle.color565 == 0x07e0;

    const bool hitCore =
        circle.HitTestCore(10, 20) == 1 &&
        circle.HitTestCore(13, 23) == 1 &&
        circle.HitTestCore(15, 20) == 0 &&
        circle.HitTestCore(16, 20) == 0;
    const bool hitWrapped =
        circle.HitTest(10, 20) == 1 &&
        circle.HitTest(16, 20) == 0;

    return constructed && hitCore && hitWrapped ? 0 : 1;
}

extern "C" int zhud_circle_draw_dirty_smoke(void) {
    void *const oldFrameBuffer = zRndr::g_frameBuffer;
    zRndr::PointOpProc const oldPointOp = zRndr::g_pfnPointOpActive;
    const int oldCircleCenterX = g_zRndr_CircleCenterX;
    const int oldCircleCenterY = g_zRndr_CircleCenterY;
    const int oldAuxArg = g_zRndr_CircleDrawAuxArg;

    TestCircleDrawDirtyOps circle{};
    circle.x = 10;
    circle.y = 20;
    circle.radius = 1;
    circle.radiusSquared = 1;
    circle.color565 = 0x07e0;

    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x87651234);
    zRndr::g_pfnPointOpActive = CaptureCirclePointOp;
    g_circleDrawBaseCount = 0;
    g_circlePointOpCount = 0;
    g_circlePointOpFrameBuffer = 0;
    g_circlePointOpArgs[0] = 0;
    g_circlePointOpArgs[1] = 0;
    g_circlePointOpArgs[2] = 0;

    circle.DrawDirty();
    const bool directOk =
        g_circleDrawBaseCount == 1 &&
        g_circlePointOpCount == 16 &&
        g_circlePointOpFrameBuffer == reinterpret_cast<void *>(0x87651234) &&
        g_zRndr_CircleCenterX == 10 &&
        g_zRndr_CircleCenterY == 20 &&
        g_zRndr_CircleDrawAuxArg == 0 &&
        g_circlePointOpArgs[2] == 0x07e0;

    g_circleDrawBaseCount = 0;
    g_circlePointOpCount = 0;
    g_circlePointOpArgs[2] = 0;

    circle.DrawDirtyForwarder();
    const bool forwarderOk =
        g_circleDrawBaseCount == 1 &&
        g_circlePointOpCount == 16 &&
        g_circlePointOpArgs[2] == 0x07e0;

    zRndr::g_frameBuffer = oldFrameBuffer;
    zRndr::g_pfnPointOpActive = oldPointOp;
    g_zRndr_CircleCenterX = oldCircleCenterX;
    g_zRndr_CircleCenterY = oldCircleCenterY;
    g_zRndr_CircleDrawAuxArg = oldAuxArg;
    if (!directOk) {
        if (g_circleDrawBaseCount != 1) {
            return 2;
        }
        if (g_circlePointOpCount != 16) {
            return 3;
        }
        if (g_circlePointOpFrameBuffer != reinterpret_cast<void *>(0x87651234)) {
            return 4;
        }
        if (g_zRndr_CircleCenterX != 10 || g_zRndr_CircleCenterY != 20) {
            return 5;
        }
        if (g_zRndr_CircleDrawAuxArg != 0) {
            return 6;
        }
        return 7;
    }
    if (!forwarderOk) {
        if (g_circleDrawBaseCount != 1) {
            return 8;
        }
        if (g_circlePointOpCount != 16) {
            return 9;
        }
        return 10;
    }
    return 0;
}

extern "C" int zhud_element_clip_and_invalidate_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;

    HudUiElement element{};
    element.Constructor(0, 0);
    element.flags = 0x01;
    g_HudUi_InvalidateMask = 0x24;
    element.Invalidate();

    HudUiRect rect{1, 2, 3, 4};
    element.SetBltSourceAndClipRect(&element, &rect);

    const bool updated =
        element.flags == 0x25 &&
        element.bltSource == &element &&
        std::memcmp(&element.clipRect, &rect, sizeof(rect)) == 0;

    element.SetClipRect(nullptr);
    const bool nullPreserved = std::memcmp(&element.clipRect, &rect, sizeof(rect)) == 0;

    g_HudUi_InvalidateMask = oldMask;
    return updated && nullPreserved ? 0 : 1;
}

extern "C" int zhud_element_constructor_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;

    HudUiElement element{};
    HudUiElement *const result = element.Constructor(17, 29);
    const bool initialized =
        result == &element &&
        element.next == nullptr &&
        element.parent == nullptr &&
        element.flags == 0 &&
        element.timer == 0.0f &&
        element.x == 17 &&
        element.y == 29 &&
        element.bltSource == nullptr &&
        element.state == 0;

    HudUiRect rect{4, 5, 6, 7};
    element.SetClipRect(&rect);
    const bool virtualsReady = std::memcmp(&element.clipRect, &rect, sizeof(rect)) == 0;

    g_HudUi_InvalidateMask = oldMask;
    return initialized && virtualsReady ? 0 : 1;
}

extern "C" int zhud_element_copy_constructor_smoke(void) {
    HudUiElement commonProbe(0, 0);
    const void *const commonVptr = HudElementVptr(commonProbe);
    const void *const preservedVptr = reinterpret_cast<const void *>(0x4444);

    HudUiElement source{};
    SetHudElementVptr(
        source,
        nullptr
    );
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
    copiedResult->GetTextRect(&copiedRect);
    const bool copiedCoords =
        copiedResult->GetX() == source.x &&
        copiedResult->GetY() == source.y;

    HudUiElement assigned{};
    SetHudElementVptr(
        assigned,
        preservedVptr
    );
    assigned.next = reinterpret_cast<HudUiElement *>(0xaaaa);
    assigned.parent = reinterpret_cast<void *>(0xbbbb);
    assigned.padding32 = 0xdddd;
    HudUiElement *const assignedResult = assigned.CopyFrom(&source);

    HudUiElement scalar{};
    SetHudElementVptr(
        scalar,
        nullptr
    );
    HudUiElement *const scalarResult = scalar.ScalarDeletingDestructor(0);

    return copiedResult == &copied && HudElementVptr(copied) == commonVptr &&
                   copied.next == nullptr && copied.parent == nullptr &&
                   copied.flags == source.flags && copied.timer == source.timer &&
                   copied.x == source.x && copied.y == source.y &&
                   copied.bltSource == source.bltSource &&
                   copied.clipRect.left == source.clipRect.left &&
                   copied.clipRect.top == source.clipRect.top &&
                   copied.clipRect.right == source.clipRect.right &&
                   copied.clipRect.bottom == source.clipRect.bottom &&
                   copiedRect.left == source.x && copiedRect.top == source.y &&
                   copiedRect.right == source.x && copiedRect.bottom == source.y &&
                   copiedCoords && copied.state == source.state &&
                   copied.padding32 == 0xcccc && assignedResult == &assigned &&
                   HudElementVptr(assigned) == preservedVptr &&
                   assigned.next == nullptr && assigned.parent == nullptr &&
                   assigned.flags == source.flags && assigned.timer == source.timer &&
                   assigned.x == source.x && assigned.y == source.y &&
                   assigned.bltSource == source.bltSource &&
                   assigned.clipRect.left == source.clipRect.left &&
                   assigned.clipRect.top == source.clipRect.top &&
                   assigned.clipRect.right == source.clipRect.right &&
                   assigned.clipRect.bottom == source.clipRect.bottom &&
                   assigned.state == source.state && assigned.padding32 == 0xdddd &&
                   scalarResult == &scalar && HudElementVptr(scalar) == commonVptr
               ? 0
               : 1;
}

extern "C" int zhud_element_scalar_deleting_destructor_smoke(void) {
    void *const noDeleteStorage = ::operator new(sizeof(HudUiElement));
    HudUiElement *const noDeleteElement =
        new (noDeleteStorage) HudUiElement(17, 29);
    HudUiElement *const noDeleteResult =
        noDeleteElement->ScalarDeletingDestructor(0);
    ::operator delete(noDeleteStorage);

    void *const deleteStorage = ::operator new(sizeof(HudUiElement));
    HudUiElement *const deleteElement =
        new (deleteStorage) HudUiElement(31, 43);
    HudUiElement *const deleteResult =
        deleteElement->ScalarDeletingDestructor(1);

    return noDeleteResult == noDeleteElement && deleteResult == deleteElement ? 0 : 1;
}

extern "C" int zhud_element_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiElement));
    HudUiElement *const element = new (storage) HudUiElement(23, 31);
    element->~HudUiElement();
    ::operator delete(storage);

    return 0;
}

extern "C" int zhud_element_draw_dispatch_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    g_zVideo_pfnBltSourceToPrimary = CaptureHudElementBlit;

    HudUiElement element(14, 27);
    element.bltSource = nullptr;
    g_elementBlitCount = 0;
    element.Draw();
    const bool nullSkipped = g_elementBlitCount == 0;

    zVidImagePartial image{};
    HudUiRect rect{2, 3, 18, 21};
    element.bltSource = &image;
    element.clipRect = rect;
    g_elementBlitCount = 0;
    element.Draw();

    const bool blitted =
        g_elementBlitCount == 1 &&
        g_elementBlitImage == &image &&
        g_elementBlitX == 14 &&
        g_elementBlitY == 27 &&
        g_elementBlitFlags == 0 &&
        g_elementBlitHasRect != 0 &&
        g_elementBlitRect.left == 2 &&
        g_elementBlitRect.top == 3 &&
        g_elementBlitRect.right == 18 &&
        g_elementBlitRect.bottom == 21;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    return nullSkipped && blitted ? 0 : 1;
}

extern "C" int zhud_element_draw_base_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    g_zVideo_pfnBltSourceToPrimary = CaptureHudElementBlit;

    HudUiElement element(14, 27);
    element.bltSource = nullptr;
    g_elementBlitCount = 0;
    element.DrawBase();
    const bool nullSkipped = g_elementBlitCount == 0;

    zVidImagePartial image{};
    HudUiRect rect{2, 3, 18, 21};
    element.bltSource = &image;
    element.clipRect = rect;
    g_elementBlitCount = 0;
    element.DrawBase();

    const bool blitted =
        g_elementBlitCount == 1 &&
        g_elementBlitImage == &image &&
        g_elementBlitX == 14 &&
        g_elementBlitY == 27 &&
        g_elementBlitFlags == 0 &&
        g_elementBlitHasRect != 0 &&
        g_elementBlitRect.left == 2 &&
        g_elementBlitRect.top == 3 &&
        g_elementBlitRect.right == 18 &&
        g_elementBlitRect.bottom == 21;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    return nullSkipped && blitted ? 0 : 1;
}

extern "C" int zhud_element_update_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;

    g_elementUpdateDrawCount = 0;
    g_elementUpdateDrawBaseCount = 0;
    g_elementUpdateInvalidateCount = 0;
    g_HudUi_InvalidateMask = 0x80;

    TestElementUpdateElement element{};
    element.flags = 0;
    element.Update(0.1f);
    const bool visibleDraw = g_elementUpdateDrawCount == 1;

    element.flags = 0x02 | 0x04;
    element.Update(0.1f);
    const bool visibleDirty =
        g_elementUpdateDrawCount == 2 &&
        (element.flags & 0x04) == 0;

    element.flags = 0x02 | 0x08;
    element.Update(0.1f);
    const bool visibleAlternateDirty =
        g_elementUpdateDrawCount == 3 &&
        (element.flags & 0x08) == 0;

    element.flags = 0x10 | 0x02 | 0x04;
    element.Update(0.1f);
    const bool hiddenDirty =
        g_elementUpdateDrawBaseCount == 1 &&
        (element.flags & 0x04) == 0;

    element.flags = 0x10 | 0x02 | 0x08;
    element.Update(0.1f);
    const bool hiddenAlternateDirty =
        g_elementUpdateDrawBaseCount == 2 &&
        (element.flags & 0x08) == 0;

    element.flags = 0x01;
    element.timer = 0.5f;
    element.Update(0.25f);
    const bool countdownActive =
        element.timer == 0.25f &&
        (element.flags & 0x10) == 0;
    element.Update(0.25f);
    const bool countdownExpired =
        element.timer == 0.0f &&
        (element.flags & 0x10) != 0 &&
        g_elementUpdateInvalidateCount == 1;

    g_HudUi_InvalidateMask = oldMask;
    return visibleDraw && visibleDirty && visibleAlternateDirty && hiddenDirty &&
                   hiddenAlternateDirty && countdownActive && countdownExpired
               ? 0
               : 1;
}

extern "C" int zhud_widget_constructor_smoke(void) {
    HudUiWidget widget{};
    widget.imageStateWord = 0xabcd1234;
    for (HudUiRectDirty &dirtyRect : widget.dirtyRects) {
        dirtyRect.framesRemaining = 7;
    }

    HudUiWidget *const result = widget.Constructor(0x55aa);

    bool dirtyFramesCleared = true;
    for (const HudUiRectDirty &dirtyRect : widget.dirtyRects) {
        dirtyFramesCleared = dirtyFramesCleared && dirtyRect.framesRemaining == 0;
    }

    const bool initialized =
        result == &widget &&
        widget.alignFlags == 0x55aa &&
        widget.image == nullptr &&
        widget.ownsImage == 0 &&
        widget.bltClipRectOrNull == nullptr &&
        (widget.imageStateWord & 0xffffu) == 0 &&
        widget.dirtyRectCount == 0 &&
        widget.state == 0 &&
        dirtyFramesCleared;

    zVidImagePartial image{};
    image.width = 11;
    image.height = 6;
    widget.x = 10;
    widget.y = 20;
    widget.image = &image;
    widget.alignFlags = 1;
    const bool virtualsReady = widget.GetCenterX() == 15 && widget.GetCenterY() == 23;

    return initialized && virtualsReady ? 0 : 1;
}

extern "C" int zhud_widget_invalidate_rect_smoke(void) {
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;

    g_widgetInvalidateRectGetCenterXCount = 0;
    g_widgetInvalidateRectGetCenterYCount = 0;
    g_widgetInvalidateRectInvalidateCount = 0;
    g_widgetInvalidateRectInvalidateThis = nullptr;

    zVidImagePartial image{};
    image.width = 30;
    image.height = 40;

    TestWidgetInvalidateRect widget{};
    widget.x = 10;
    widget.y = 20;
    widget.alignFlags = 0;
    widget.image = &image;
    widget.dirtyRectCount = 2;
    for (HudUiRectDirty &dirtyRect : widget.dirtyRects) {
        dirtyRect.framesRemaining = 9;
    }
    widget.dirtyRects[0].framesRemaining = 0;

    g_HudUi_InvalidateMask = 0x0c;
    const HudUiRect dirtyRect = {5, 15, 50, 80};
    widget.InvalidateRect(&dirtyRect);

    const HudUiRectDirty &slot = widget.dirtyRects[0];
    const bool clipped =
        widget.dirtyRectCount == 3 &&
        slot.framesRemaining == 2 &&
        slot.drawX == 10 &&
        slot.drawY == 20 &&
        slot.srcLeft == 3 &&
        slot.srcTop == 9 &&
        slot.srcRight == 43 &&
        slot.srcBottom == 29;
    const bool dispatched = g_widgetInvalidateRectGetCenterXCount == 2 &&
                            g_widgetInvalidateRectGetCenterYCount == 2 &&
                            g_widgetInvalidateRectInvalidateCount == 1 &&
                            g_widgetInvalidateRectInvalidateThis == &widget;

    g_HudUi_InvalidateMask = oldInvalidateMask;
    return clipped && dispatched ? 0 : 1;
}

extern "C" int zhud_objective_update_meter_xpoints_smoke(void) {
    const HudUiWidget oldWidget = g_HudUiMgrObjectiveWidget;
    const HudUiMeter oldMeter = g_HudUiMgrObjectiveMeter;

    g_HudUiMgrObjectiveWidget = {};
    g_HudUiMgrObjectiveWidget.x = 123;
    g_HudUiMgrObjectiveWidget.alignFlags = 0;
    g_HudUiMgrObjectiveMeter = {};
    g_HudUiMgrObjectiveMeter.points[0].x = -1.0f;
    g_HudUiMgrObjectiveMeter.points[1].x = -2.0f;
    g_HudUiMgrObjectiveMeter.points[2].x = -3.0f;
    g_HudUiMgrObjectiveMeter.points[3].x = -4.0f;
    g_HudUiMgrObjectiveMeter.points[0].y = 10.0f;
    g_HudUiMgrObjectiveMeter.points[3].y = 40.0f;

    HudUiMgrObjective::UpdateMeterXPoints();

    const bool leftEdge = g_HudUiMgrObjectiveMeter.points[0].x == 128.0f &&
                          g_HudUiMgrObjectiveMeter.points[1].x == 128.0f;
    const bool rightEdge = g_HudUiMgrObjectiveMeter.points[2].x == 135.0f &&
                           g_HudUiMgrObjectiveMeter.points[3].x == 135.0f;
    const bool yUnchanged = g_HudUiMgrObjectiveMeter.points[0].y == 10.0f &&
                            g_HudUiMgrObjectiveMeter.points[3].y == 40.0f;

    g_HudUiMgrObjectiveWidget = oldWidget;
    g_HudUiMgrObjectiveMeter = oldMeter;
    return leftEdge && rightEdge && yUnchanged ? 0 : 1;
}

extern "C" int zhud_slot_draw_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    zVidImagePartial *const oldExclusiveImage =
        g_HudUiWidget_ExclusiveDrawImage;
    g_zVideo_pfnBltSourceToPrimary = CaptureHudElementBlit;
    g_HudUiWidget_ExclusiveDrawImage = nullptr;

    HudUiSlot slot{};
    slot.Constructor();
    zVidImagePartial slotImage{};
    zVidImagePartial trackMarkerImage{};
    slot.slotWidget.image = &slotImage;
    slot.slotWidget.bltSource = &slotImage;
    slot.trackMarkerWidget.image = &trackMarkerImage;
    slot.trackMarkerWidget.bltSource = &trackMarkerImage;
    slot.slotWidget.dirtyRectCount = 0;
    slot.trackMarkerWidget.dirtyRectCount = 0;
    for (HudUiRectDirty &dirtyRect : slot.slotWidget.dirtyRects) {
        dirtyRect.framesRemaining = 0;
    }
    for (HudUiRectDirty &dirtyRect : slot.trackMarkerWidget.dirtyRects) {
        dirtyRect.framesRemaining = 0;
    }

    g_elementBlitCount = 0;
    slot.Draw();
    const int bothVisibleBlits = g_elementBlitCount;

    slot.slotWidget.flags = 0x10;
    slot.trackMarkerWidget.flags = 0;
    const int beforeTrackMarkerOnly = g_elementBlitCount;
    slot.Draw();
    const int trackMarkerOnlyBlits = g_elementBlitCount - beforeTrackMarkerOnly;

    slot.slotWidget.flags = 0;
    slot.trackMarkerWidget.flags = 0x10;
    const int beforeSlotOnly = g_elementBlitCount;
    slot.Draw();
    const int slotOnlyBlits = g_elementBlitCount - beforeSlotOnly;

    g_HudUiWidget_ExclusiveDrawImage = oldExclusiveImage;
    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    slot.Destructor();

    const bool bothVisible =
        bothVisibleBlits != 0 &&
        bothVisibleBlits == trackMarkerOnlyBlits + slotOnlyBlits;
    const bool onlyTrackMarkerVisible = trackMarkerOnlyBlits != 0;
    const bool onlySlotVisible = slotOnlyBlits != 0;
    return bothVisible && onlyTrackMarkerVisible && onlySlotVisible ? 0 : 1;
}

extern "C" int zhud_triplet_panel_draw_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    zVidImagePartial *const oldExclusiveImage =
        g_HudUiWidget_ExclusiveDrawImage;
    g_zVideo_pfnBltSourceToPrimary = CaptureTripletPanelBlit;
    g_HudUiWidget_ExclusiveDrawImage = nullptr;
    g_tripletPanelBlitCount = 0;
    g_tripletPanelBlitImages[0] = nullptr;
    g_tripletPanelBlitImages[1] = nullptr;
    g_tripletPanelBlitImages[2] = nullptr;
    g_tripletPanelBlitImages[3] = nullptr;

    zVidImagePartial baseImage{};
    zVidImagePartial itemImages[3]{};

    HudUiTripletPanel panel{};
    panel.bltSource = &baseImage;
    panel.items[0].flags = 0;
    panel.items[0].image = &itemImages[0];
    panel.items[0].dirtyRectCount = 0;
    panel.items[1].flags = 0x10;
    panel.items[1].image = &itemImages[1];
    panel.items[1].dirtyRectCount = 0;
    panel.items[2].flags = 0;
    panel.items[2].image = &itemImages[2];
    panel.items[2].dirtyRectCount = 0;

    panel.Draw();

    const bool drawOrder =
        g_tripletPanelBlitCount == 3 &&
        g_tripletPanelBlitImages[0] == &baseImage &&
        g_tripletPanelBlitImages[1] == &itemImages[2] &&
        g_tripletPanelBlitImages[2] == &itemImages[0] &&
        g_tripletPanelBlitImages[3] == nullptr;

    g_HudUiWidget_ExclusiveDrawImage = oldExclusiveImage;
    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    return drawOrder ? 0 : 1;
}

extern "C" int zhud_triplet_panel_set_visible_count_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    const HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;

    g_HudUi_InvalidateMask = 0x80;

    HudUiTripletPanel panel{};
    panel.visibleCount = 99;
    panel.items[0].flags = 0x10;
    panel.items[1].flags = 0x10;
    panel.items[2].flags = 0x10;
    panel.SetVisibleCount(2);

    const bool twoVisible =
        panel.visibleCount == 2 &&
        (panel.flags & 0x80) != 0 &&
        (panel.items[0].flags & 0x10) == 0 &&
        (panel.items[1].flags & 0x10) == 0 &&
        (panel.items[2].flags & 0x10) != 0;

    panel.flags = 0;
    panel.items[0].flags = 0;
    panel.items[1].flags = 0;
    panel.items[2].flags = 0;
    panel.SetVisibleCount(-3);

    const bool noneVisible =
        panel.visibleCount == 0 &&
        (panel.flags & 0x80) != 0 &&
        (panel.items[0].flags & 0x10) != 0 &&
        (panel.items[1].flags & 0x10) != 0 &&
        (panel.items[2].flags & 0x10) != 0;

    panel.flags = 0;
    panel.SetVisibleCount(0);
    const bool unchanged = panel.flags == 0;

    g_HudUiMgrNanitePanel.visibleCount = 0;
    g_HudUiMgrNanitePanel.flags = 0;
    g_HudUiMgrNanitePanel.items[0].flags = 0x10;
    g_HudUiMgrNanitePanel.items[1].flags = 0x10;
    g_HudUiMgrNanitePanel.items[2].flags = 0x10;

    HudUiMgr::SetNanitePanelCount(2);
    const bool naniteForwarded =
        g_HudUiMgrNanitePanel.visibleCount == 2 &&
        (g_HudUiMgrNanitePanel.flags & 0x80) != 0 &&
        (g_HudUiMgrNanitePanel.items[0].flags & 0x10) == 0 &&
        (g_HudUiMgrNanitePanel.items[1].flags & 0x10) == 0 &&
        (g_HudUiMgrNanitePanel.items[2].flags & 0x10) != 0;

    g_HudUiMgrNanitePanel = oldNanitePanel;
    g_HudUi_InvalidateMask = oldMask;
    return twoVisible && noneVisible && unchanged && naniteForwarded ? 0 : 1;
}

extern "C" int zhud_mgr_trigger_current_layout_on_activated_smoke(void) {
    CaptureActivatedLayout layout{};

    HudLayoutBase *const oldCurrentLayout = g_HudUiMgrCurrentLayout;
    g_layoutActivatedCount = 0;

    g_HudUiMgrCurrentLayout = nullptr;
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    const bool nullPath = g_layoutActivatedCount == 0;

    g_HudUiMgrCurrentLayout = &layout;
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    const bool activePath = g_layoutActivatedCount == 1;

    g_HudUiMgrCurrentLayout = oldCurrentLayout;
    return nullPath && activePath ? 0 : 1;
}

extern "C" int zhud_layout_hw_update_objective_dirty_rect_smoke(void) {
    const HudUiWidget oldObjectiveWidget = g_HudUiMgrObjectiveWidget;
    const int oldObjectiveRightX = g_HudUiMgrObjectiveWidgetRightX;
    const HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    zVideo_BltSourceToPrimaryProc const oldBlit =
        g_zVideo_pfnBltSourceToPrimary;
    zVidImagePartial *const oldExclusiveImage =
        g_HudUiWidget_ExclusiveDrawImage;

    zVidImagePartial objectiveImage{};
    objectiveImage.width = 12;
    objectiveImage.height = 6;
    new (&g_HudUiMgrObjectiveWidget) HudUiWidget;
    g_HudUiMgrObjectiveWidget.image = &objectiveImage;
    g_HudUiMgrObjectiveWidget.x = 31;
    g_HudUiMgrObjectiveWidget.y = 34;
    g_HudUiMgrObjectiveWidget.alignFlags = 1;
    g_HudUiMgrObjectiveWidgetRightX = 80;

    HudLayoutHW layout{};
    zVidImagePartial layoutImage{};
    layoutImage.width = 100;
    layoutImage.height = 100;
    layout.widget2.image = &layoutImage;
    layout.widget2.x = 10;
    layout.widget2.y = 20;
    layout.widget2.dirtyRectCount = 0;
    for (HudUiRectDirty &dirtyRect : layout.widget2.dirtyRects) {
        dirtyRect.framesRemaining = 0;
    }

    zVidImagePartial naniteImage{};
    new (&g_HudUiMgrNanitePanel) HudUiNanitePanel();
    g_HudUiMgrNanitePanel.items[0].image = &naniteImage;
    g_HudUiMgrNanitePanel.items[0].bltSource = nullptr;
    g_HudUiMgrNanitePanel.items[0].flags = 0;
    g_HudUiMgrNanitePanel.items[0].dirtyRectCount = 0;
    for (HudUiRectDirty &dirtyRect : g_HudUiMgrNanitePanel.items[0].dirtyRects) {
        dirtyRect.framesRemaining = 0;
    }
    g_HudUiMgrNanitePanel.items[1].flags = 0x10;
    g_HudUiMgrNanitePanel.items[2].flags = 0x10;

    g_zVideo_pfnBltSourceToPrimary = CaptureHudElementBlit;
    g_HudUiWidget_ExclusiveDrawImage = nullptr;
    g_elementBlitCount = 0;
    g_elementBlitImage = nullptr;
    g_HudUi_InvalidateMask = 0x80;

    layout.UpdateObjectiveDirtyRect();

    const HudUiRectDirty &slot = layout.widget2.dirtyRects[0];
    const bool dirtyRect =
        layout.widget2.dirtyRectCount == 1 &&
        slot.framesRemaining == 1 &&
        slot.drawX == 49 &&
        slot.drawY == 37 &&
        slot.srcLeft == 39 &&
        slot.srcTop == 17 &&
        slot.srcRight == 70 &&
        slot.srcBottom == 23;
    const bool layoutInvalidated = (layout.widget2.flags & 0x80u) != 0;
    const bool naniteInvalidated =
        (g_HudUiMgrNanitePanel.flags & 0x80u) != 0;
    const bool naniteDrawn =
        g_elementBlitCount == 1 &&
        g_elementBlitImage == &naniteImage;

    g_HudUiMgrObjectiveWidget = oldObjectiveWidget;
    g_HudUiMgrObjectiveWidgetRightX = oldObjectiveRightX;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiWidget_ExclusiveDrawImage = oldExclusiveImage;
    g_zVideo_pfnBltSourceToPrimary = oldBlit;

    return dirtyRect && layoutInvalidated && naniteInvalidated && naniteDrawn
               ? 0
               : 1;
}

extern "C" int zhud_widget_release_image_if_owned_smoke(void) {
    HudUiWidget borrowed{};
    zVidImagePartial borrowedImage{};
    borrowed.image = &borrowedImage;
    borrowed.ownsImage = 0;
    borrowed.ReleaseImageIfOwned();
    const bool borrowedOk = borrowed.image == &borrowedImage && borrowed.ownsImage == 0;

    HudUiWidget owned{};
    owned.image = &zVid_Image::g_zImage_DefaultImage;
    owned.ownsImage = 1;
    owned.ReleaseImageIfOwned();
    const bool ownedOk = owned.image == nullptr && owned.ownsImage == 0;

    HudUiWidget empty{};
    empty.ownsImage = 1;
    empty.ReleaseImageIfOwned();
    const bool emptyOk = empty.image == nullptr && empty.ownsImage == 0;

    return borrowedOk && ownedOk && emptyOk ? 0 : 1;
}

extern "C" int zhud_widget_destructor_core_smoke(void) {
    HudUiWidget *borrowed = new HudUiWidget();
    zVidImagePartial borrowedImage{};
    borrowed->image = &borrowedImage;
    borrowed->ownsImage = 0;
    borrowed->DestructorCore();
    const bool borrowedOk = borrowed->image == &borrowedImage && borrowed->ownsImage == 0;
    ::operator delete(borrowed);

    HudUiWidget *owned = new HudUiWidget();
    owned->image = &zVid_Image::g_zImage_DefaultImage;
    owned->ownsImage = 1;
    owned->DestructorCore();
    const bool ownedOk = owned->image == nullptr && owned->ownsImage == 0;
    ::operator delete(owned);

    return borrowedOk && ownedOk ? 0 : 1;
}

extern "C" int zhud_fill_bitmap_core_smoke(void) {
    HudUiFillBitmap bitmap{};
    bitmap.Constructor();
    bitmap.image = &zVid_Image::g_zImage_DefaultImage;
    bitmap.ownsImage = 0;
    bitmap.previewImage = &zVid_Image::g_zImage_DefaultImage;
    bitmap.fillImage = &zVid_Image::g_zImage_DefaultImage;
    const void *const bitmapVptr = HudElementVptr(bitmap);

    bitmap.DestructorCore();
    const bool coreDestructed =
        HudElementVptr(bitmap) != bitmapVptr &&
        bitmap.previewImage == &zVid_Image::g_zImage_DefaultImage &&
        bitmap.fillImage == &zVid_Image::g_zImage_DefaultImage;

    HudUiFillBitmap scalar{};
    scalar.Constructor();
    scalar.image = &zVid_Image::g_zImage_DefaultImage;
    scalar.ownsImage = 0;
    scalar.previewImage = &zVid_Image::g_zImage_DefaultImage;
    scalar.fillImage = &zVid_Image::g_zImage_DefaultImage;
    const void *const scalarVptr = HudElementVptr(scalar);
    HudUiElement *const scalarResult = scalar.ScalarDeletingDestructor(0);
    const bool scalarDestructed =
        scalarResult == &scalar &&
        HudElementVptr(scalar) != scalarVptr &&
        scalar.previewImage == &zVid_Image::g_zImage_DefaultImage &&
        scalar.fillImage == &zVid_Image::g_zImage_DefaultImage;

    HudUiFillBitmap thunk{};
    thunk.Constructor();
    thunk.image = &zVid_Image::g_zImage_DefaultImage;
    thunk.ownsImage = 0;
    thunk.previewImage = &zVid_Image::g_zImage_DefaultImage;
    thunk.fillImage = &zVid_Image::g_zImage_DefaultImage;
    const void *const thunkVptr = HudElementVptr(thunk);
    thunk.DestructorCoreThunk();
    const bool thunkDestructed =
        HudElementVptr(thunk) != thunkVptr &&
        thunk.previewImage == &zVid_Image::g_zImage_DefaultImage &&
        thunk.fillImage == &zVid_Image::g_zImage_DefaultImage;

    return coreDestructed && scalarDestructed && thunkDestructed ? 0 : 1;
}

extern "C" int zhud_widget_set_image_by_path_owned_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;

    HudUiWidget nullPath{};
    nullPath.image = &zVid_Image::g_zImage_DefaultImage;
    nullPath.ownsImage = 1;
    g_HudUi_InvalidateMask = 0x80;
    zVidImagePartial *const nullResult = nullPath.SetImageByPathOwned(nullptr);
    const bool nullPathOk =
        nullResult == nullptr &&
        nullPath.image == &zVid_Image::g_zImage_DefaultImage &&
        nullPath.ownsImage == 1 &&
        (nullPath.flags & 0x80) == 0;

    g_zVid_BuiltinTexturePackCount = 0;
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_TexturePackLoadState = 0;
    g_zVid_TexturePackCount = 0;
    g_zVid_TexturePacks = nullptr;

    HudUiWidget missingPath{};
    missingPath.image = &zVid_Image::g_zImage_DefaultImage;
    missingPath.ownsImage = 1;
    g_HudUi_InvalidateMask = 0x80;
    zVidImagePartial *const missingResult =
        missingPath.SetImageByPathOwned("__recoil_missing_widget_image__");
    const bool missingPathOk =
        missingResult == nullptr &&
        missingPath.image == nullptr &&
        missingPath.ownsImage == 0 &&
        (missingPath.flags & 0x80) != 0;

    g_HudUi_InvalidateMask = oldMask;
    std::free(g_zVid_TexturePacks);
    g_zVid_TexturePacks = nullptr;
    return nullPathOk && missingPathOk ? 0 : 1;
}

extern "C" int zhud_background_cursor_widget_member_constructor_smoke(void) {
    HudUiBackgroundCursorWidget cursor(nullptr, 7);

    const bool constructed =
        cursor.image == nullptr &&
        cursor.alignFlags == 0 &&
        cursor.capturedImage == nullptr &&
        cursor.captureEnabled == 7 &&
        cursor.captureSourceSelector == 1 &&
        cursor.reservedC8 == 0 &&
        cursor.reservedCC == 0;

    zVidImagePartial image{};
    image.width = 4;
    image.height = 2;
    cursor.image = &image;
    cursor.alignFlags = 1;
    cursor.x = 8;
    cursor.y = 10;
    const bool virtualsReady = cursor.GetCenterX() == 10 && cursor.GetCenterY() == 11;

    return constructed && virtualsReady ? 0 : 1;
}

extern "C" int zhud_background_cursor_widget_rebuild_captured_image_smoke(void) {
    unsigned short surfacePixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    unsigned short capturedPixels[4] = {};
    zVidImagePartial sourceImage{};
    sourceImage.width = 2;
    sourceImage.height = 2;
    zVidImagePartial capturedImage{};
    capturedImage.width = 2;
    capturedImage.height = 2;
    capturedImage.pixelCount = 4;
    capturedImage.pixels = capturedPixels;

    const zVideo_SurfaceStatePartial oldSwSurfaceState = g_zVideo_SwSurfaceState;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = surfacePixels;

    HudUiBackgroundCursorWidget cursor(nullptr, 1);
    cursor.image = &sourceImage;
    cursor.capturedImage = &capturedImage;
    cursor.captureSourceSelector = 0;

    cursor.RebuildCapturedImage(1, 1);

    const bool ok =
        cursor.bltSource == &capturedImage &&
        cursor.clipRect.left == 0 &&
        cursor.clipRect.top == 0 &&
        cursor.clipRect.right == 2 &&
        cursor.clipRect.bottom == 2 &&
        capturedPixels[0] == 6 &&
        capturedPixels[1] == 7 &&
        capturedPixels[2] == 10 &&
        capturedPixels[3] == 11;
    g_zVideo_SwSurfaceState = oldSwSurfaceState;
    cursor.capturedImage = nullptr;

    return ok ? 0 : 1;
}

extern "C" int zhud_background_cursor_widget_set_image_borrowed_refresh_smoke(void) {
    unsigned short surfacePixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    zVidImagePartial sourceImage{};
    sourceImage.width = 2;
    sourceImage.height = 2;

    const zVideo_SurfaceStatePartial oldSwSurfaceState = g_zVideo_SwSurfaceState;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = surfacePixels;

    HudUiBackgroundCursorWidget cursor(nullptr, 1);
    cursor.x = 1;
    cursor.y = 1;
    cursor.image = &sourceImage;
    cursor.captureSourceSelector = 0;

    cursor.SetImageBorrowedAndRefresh();
    unsigned short *const capturedPixels =
        static_cast<unsigned short *>(cursor.capturedImage != nullptr
                                          ? cursor.capturedImage->pixels
                                          : nullptr);
    const bool ok =
        cursor.capturedImage != nullptr &&
        cursor.capturedImage->width == 2 &&
        cursor.capturedImage->height == 2 &&
        (cursor.capturedImage->formatFlagsPacked & 0x20) != 0 &&
        cursor.bltSource == cursor.capturedImage &&
        cursor.clipRect.left == 0 &&
        cursor.clipRect.top == 0 &&
        cursor.clipRect.right == 2 &&
        cursor.clipRect.bottom == 2 &&
        capturedPixels != nullptr &&
        capturedPixels[0] == 6 &&
        capturedPixels[1] == 7 &&
        capturedPixels[2] == 10 &&
        capturedPixels[3] == 11;

    if (cursor.capturedImage != nullptr) {
        zVid_Image::Destroy(cursor.capturedImage);
        cursor.capturedImage = nullptr;
    }
    g_zVideo_SwSurfaceState = oldSwSurfaceState;

    return ok ? 0 : 1;
}

extern "C" int
zhud_background_cursor_widget_set_image_by_path_owned_refresh_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char packPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "hcr", 0, packPath) == 0) {
        return 1;
    }

    const char *const imageName = "cursor_path.tex";
    zVidTexturePackHeader packHeader{};
    packHeader.fileFormat = 1;
    packHeader.recordCount = 1;

    zVidTexturePackRecord record{};
    std::strcpy(record.name, imageName);
    record.fileOffset = sizeof(packHeader) + sizeof(record);
    record.paletteIndex = -1;

    FILE *out = std::fopen(packPath, "wb");
    if (out == nullptr) {
        DeleteFileA(packPath);
        return 2;
    }

    unsigned char imageHeader[0x10] = {};
    imageHeader[0] = 1;
    *reinterpret_cast<short *>(&imageHeader[4]) = 2;
    *reinterpret_cast<short *>(&imageHeader[6]) = 2;
    const unsigned short imagePixels[4] = {0x1001, 0x1002, 0x1003, 0x1004};
    std::fwrite(&packHeader, sizeof(packHeader), 1, out);
    std::fwrite(&record, sizeof(record), 1, out);
    std::fwrite(imageHeader, 1, sizeof(imageHeader), out);
    std::fwrite(imagePixels, sizeof(imagePixels), 1, out);
    std::fclose(out);

    zVidTexturePackEntry entry{};
    std::strcpy(entry.filePath, packPath);
    zVidTexturePackEntry *const oldTexturePacks = g_zVid_TexturePacks;
    const int oldTexturePackCount = g_zVid_TexturePackCount;
    zVidTexturePackEntry *const oldBuiltinPacks = g_zVid_BuiltinTexturePacks;
    const int oldBuiltinPackCount = g_zVid_BuiltinTexturePackCount;
    const int oldTexturePackLoadState = g_zVid_TexturePackLoadState;
    const zVideo_SurfaceStatePartial oldSwSurfaceState = g_zVideo_SwSurfaceState;

    g_zVid_TexturePackLoadState = 1;
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    if (zVid_TexturePackEntry_LoadFromFile(&entry) == nullptr) {
        if (entry.fileHandle != nullptr) {
            std::fclose(entry.fileHandle);
        }
        std::free(entry.records);
        g_zVid_TexturePacks = oldTexturePacks;
        g_zVid_TexturePackCount = oldTexturePackCount;
        g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
        g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
        g_zVid_TexturePackLoadState = oldTexturePackLoadState;
        DeleteFileA(packPath);
        return 3;
    }

    g_zVid_TexturePacks = &entry;
    g_zVid_TexturePackCount = 1;

    unsigned short surfacePixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = surfacePixels;

    HudUiBackgroundCursorWidget cursor(nullptr, 1);
    cursor.x = 1;
    cursor.y = 1;
    cursor.captureSourceSelector = 0;

    cursor.SetImageByPathOwnedAndRefresh(imageName);
    unsigned short *const capturedPixels =
        static_cast<unsigned short *>(cursor.capturedImage != nullptr
                                          ? cursor.capturedImage->pixels
                                          : nullptr);
    const bool loadedAndRefreshed =
        cursor.image != nullptr &&
        cursor.image->width == 2 &&
        cursor.image->height == 2 &&
        cursor.ownsImage == 1 &&
        cursor.capturedImage != nullptr &&
        cursor.bltSource == cursor.capturedImage &&
        cursor.clipRect.left == 0 &&
        cursor.clipRect.top == 0 &&
        cursor.clipRect.right == 2 &&
        cursor.clipRect.bottom == 2 &&
        capturedPixels != nullptr &&
        capturedPixels[0] == 6 &&
        capturedPixels[1] == 7 &&
        capturedPixels[2] == 10 &&
        capturedPixels[3] == 11;

    zVidImagePartial *const capturedBeforeMissing = cursor.capturedImage;
    void *const bltSourceBeforeMissing = cursor.bltSource;
    cursor.SetImageByPathOwnedAndRefresh("__recoil_missing_cursor_path_image__");
    const bool missingPathSkippedRefresh =
        cursor.image == nullptr &&
        cursor.ownsImage == 0 &&
        cursor.capturedImage == capturedBeforeMissing &&
        cursor.bltSource == bltSourceBeforeMissing;

    if (cursor.capturedImage != nullptr) {
        zVid_Image::Destroy(cursor.capturedImage);
        cursor.capturedImage = nullptr;
    }
    cursor.ReleaseImageIfOwned();
    std::fclose(entry.fileHandle);
    std::free(entry.records);
    g_zVid_TexturePacks = oldTexturePacks;
    g_zVid_TexturePackCount = oldTexturePackCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinPacks;
    g_zVid_BuiltinTexturePackCount = oldBuiltinPackCount;
    g_zVid_TexturePackLoadState = oldTexturePackLoadState;
    g_zVideo_SwSurfaceState = oldSwSurfaceState;
    DeleteFileA(packPath);

    return loadedAndRefreshed && missingPathSkippedRefresh ? 0 : 1;
}

extern "C" int zhud_background_video_widget_constructor_smoke(void) {
    HudUiBackgroundVideoWidget widget{};
    const bool initialized =
        widget.x == 0 &&
        widget.y == 0 &&
        widget.stream == nullptr &&
        widget.elapsedTimeSec == 0.0f &&
        widget.mediaPath[0] == '\0';

    return initialized ? 0 : 1;
}

extern "C" int zhud_background_video_widget_destructor_smoke(void) {
    HudUiBackgroundVideoWidget *widget = new HudUiBackgroundVideoWidget();
    widget->stream = nullptr;
    widget->Destructor();
    const bool destroyed = widget->stream == nullptr;
    ::operator delete(widget);

    return destroyed ? 0 : 1;
}

extern "C" int zhud_background_constructor_smoke(void) {
    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption{};
    vmodeOption.payloadOrBuffer = 6;
    vmodeOption.name = vmodeName;
    vmodeOption.next = nullptr;

    zOptionEntryPartial *const oldOptionsHead = g_zGame_Options_OptionListHead;
    g_zGame_Options_OptionListHead = &vmodeOption;

    HudUiBackground *const backgroundPtr = new HudUiBackground;
    HudUiBackground &background = *backgroundPtr;

    bool soundsInitialized = true;
    for (int index = 0; index < 10; ++index) {
        soundsInitialized =
            soundsInitialized && background.backgroundSounds[index].sample == nullptr &&
            background.backgroundSounds[index].volume == 1.0f &&
            background.backgroundSounds[index].playHandle == nullptr;
    }

    const bool constructed =
        background.enabled == 0 &&
        background.childHead == nullptr &&
        background.childTail == nullptr &&
        background.inputFocusElement == nullptr &&
        background.captureTransitionMask == 1 &&
        background.cursorWidget.image == nullptr &&
        background.cursorWidget.alignFlags == 0 &&
        background.cursorWidget.captureEnabled == 1 &&
        background.cursorWidget.captureSourceSelector == 1 &&
        background.primaryClipImage == nullptr &&
        background.capturedCompositeImage == nullptr &&
        background.backgroundImageWidgets[0].image == nullptr &&
        background.backgroundImageWidgets[19].image == nullptr &&
        background.backgroundVideoWidgets[0].stream == nullptr &&
        background.backgroundVideoWidgets[9].stream == nullptr &&
        background.fontStyles[0].validMarker == 0 &&
        background.fontStyles[0].fontWeight == 500 &&
        background.fontStyles[19].fontWeight == 500 &&
        background.backgroundTextPanels[0].flashResetValue == 0.349999994f &&
        background.backgroundTextPanels[0].flashDirectionSign == 1 &&
        soundsInitialized &&
        background.uiOriginX == 0 &&
        background.uiOriginY == 60;

    delete backgroundPtr;
    g_zGame_Options_OptionListHead = oldOptionsHead;
    return constructed ? 0 : 1;
}

extern "C" int zhud_text_label_constructor_and_extents_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    zImage_Font *const oldFont1 = g_zImage_FontTable[1];
    g_HudUi_InvalidateMask = 0x80;

    HudUiTextLabel label{};
    HudUiTextLabel *const result = label.ConstructorWithPosAndFlags("Speed", 12, 34, 3);

    const bool constructed =
        result == &label &&
        std::strcmp(label.textBuffer, "Speed") == 0 &&
        label.x == 12 &&
        label.y == 34 &&
        label.fontHandle == 3 &&
        label.centerText == 0 &&
        label.alignMode == 0 &&
        (label.flags & 0x80) != 0;

    HudUiTextLabel assigned{};
    HudUiTextLabel *const assignedResult = assigned.Constructor(&label);
    const bool assignedLabel =
        assignedResult == &assigned &&
        std::strcmp(assigned.textBuffer, "Speed") == 0 &&
        assigned.x == 12 &&
        assigned.y == 34 &&
        assigned.fontHandle == 3 &&
        assigned.centerText == 0 &&
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
    label.y = 20;
    label.bltSource = &label;
    label.flags = 0;
    label.SetTextFmt("AB");

    const bool centered =
        label.x == 117 &&
        label.clipRect.left == 117 &&
        label.clipRect.top == 20 &&
        label.clipRect.right == 133 &&
        label.clipRect.bottom == 26 &&
        (label.flags & 0x80) != 0;

    HudUiTextLabel copied{};
    copied.padding32 = 0x7777;
    copied.CopyConstructor(&label);
    const bool copiedLabel =
        copied.next == nullptr &&
        copied.parent == nullptr &&
        copied.x == label.x &&
        copied.y == label.y &&
        copied.state == label.state &&
        copied.padding32 == 0x7777 &&
        std::strcmp(copied.textBuffer, label.textBuffer) == 0 &&
        copied.fontHandle == label.fontHandle &&
        copied.centerText == label.centerText &&
        copied.centerBoundsLeft == label.centerBoundsLeft &&
        copied.centerBoundsRight == label.centerBoundsRight &&
        copied.alignMode == label.alignMode;

    label.flags = 0;
    std::memset(label.textBuffer, 'X', sizeof(label.textBuffer));
    label.SetTextFmt(nullptr);

    const bool cleared = label.textBuffer[0] == 0 &&
                         label.textBuffer[sizeof(label.textBuffer) - 1] == 0 &&
                         label.flags == 0;

    g_zImage_FontTable[1] = oldFont1;
    g_HudUi_InvalidateMask = oldMask;
    return constructed && assignedLabel && centered && copiedLabel && cleared ? 0 : 1;
}

extern "C" int zhud_panel_constructor_default_smoke(void) {
    HudUiPanel panel{};
    HudUiPanel *const returned = panel.ConstructorDefault("TXT", 3, 28);

    HudUiPanel thunkPanel{};
    HudUiPanel *const thunkReturned = thunkPanel.ConstructorDefaultThunk();

    const bool defaultCtor =
        returned == &panel &&
        std::strcmp(panel.textBuffer, "TXT") == 0 &&
        panel.x == 3 &&
        panel.y == 28 &&
        panel.textPick == nullptr &&
        panel.textColor0 == 0x00ffffff &&
        panel.textColor1 == 0x00ffffff &&
        panel.hFont != nullptr &&
        panel.cachedText[0] == '\0' &&
        panel.textWidthPx == 0 &&
        panel.textHeightPx == 0 &&
        panel.shadowEnabled == 0 &&
        panel.bkMode == TRANSPARENT &&
        panel.textDirty == 1 &&
        panel.GetWrapRect() == &panel.wrapRect &&
        panel.wrapRect.left == 0 &&
        panel.wrapRect.top == 0 &&
        panel.wrapRect.right == 0 &&
        panel.wrapRect.bottom == 0 &&
        panel.textRect.left == 0 &&
        panel.textRect.top == 0 &&
        panel.textRect.bottom == 0 &&
        panel.shadowOffsetY == 0;

    const bool thunkCtor =
        thunkReturned == &thunkPanel &&
        thunkPanel.textBuffer[0] == '\0' &&
        thunkPanel.x == 0 &&
        thunkPanel.y == 0 &&
        thunkPanel.textPick == nullptr &&
        thunkPanel.textColor0 == 0x00ffffff &&
        thunkPanel.textColor1 == 0x00ffffff &&
        thunkPanel.hFont != nullptr &&
        thunkPanel.shadowEnabled == 0 &&
        thunkPanel.bkMode == TRANSPARENT &&
        thunkPanel.textDirty == 1;

    return defaultCtor && thunkCtor ? 0 : 1;
}

extern "C" int zhud_panel_layout_entry_copy_construct_smoke(void) {
    HudUiPanelLayoutEntry source{};
    InitCreditsPanelEntry(&source, "source row", 14, 24);

    HudUiPanelLayoutEntry copied{};
    HudUiPanelLayoutEntry *const result = copied.CopyConstruct(&source);

    const bool copiedValues =
        result == &copied && CreditsPanelEntryMatches(copied, "source row", 14, 24);

    copied.panel.DestructorThunk();
    source.panel.DestructorThunk();
    return copiedValues ? 0 : 1;
}

extern "C" int zhud_panel_layout_entry_copy_assign_smoke(void) {
    HudUiPanelLayoutEntry source{};
    InitCreditsPanelEntry(&source, "assign row", 16, 26);

    HudUiPanelLayoutEntry copied{};
    HudUiPanelLayoutEntry *const result = copied.CopyAssign(&source);

    const bool copiedValues =
        result == &copied && CreditsPanelEntryMatches(copied, "assign row", 16, 26);

    copied.panel.DestructorThunk();
    source.panel.DestructorThunk();
    return copiedValues ? 0 : 1;
}

extern "C" int zhud_panel_layout_entry_copy_assign_range_smoke(void) {
    HudUiPanelLayoutEntry source[2] = {};
    InitCreditsPanelEntry(&source[0], "range a", 18, 28);
    InitCreditsPanelEntry(&source[1], "range b", 38, 48);

    HudUiPanelLayoutEntry dest[2] = {};
    HudUiPanelLayoutEntry *const result =
        HudUiPanelLayoutEntry::CopyAssignRange(source, source + 2, dest);

    const bool copiedValues =
        result == dest + 2 && CreditsPanelEntryMatches(dest[0], "range a", 18, 28) &&
        CreditsPanelEntryMatches(dest[1], "range b", 38, 48);

    HudUiPanelLayoutEntry::DestroyRange(dest, dest + 2);
    source[1].panel.DestructorThunk();
    source[0].panel.DestructorThunk();
    return copiedValues ? 0 : 1;
}

extern "C" int zhud_panel_layout_entry_destroy_range_smoke(void) {
    HudUiPanelLayoutEntry entries[2] = {};
    InitCreditsPanelEntry(&entries[0], "destroy a", 11, 12);
    InitCreditsPanelEntry(&entries[1], "destroy b", 13, 14);

    CodeFunctionPatch destructorPatch{};
    g_PanelLayoutDestroyCount = 0;
    g_PanelLayoutDestroyPanels[0] = nullptr;
    g_PanelLayoutDestroyPanels[1] = nullptr;

    if (!PatchFunctionJump(
            MethodAddress(&HudUiPanel::DestructorThunk),
            reinterpret_cast<void *>(&CountPanelDestructorThunk),
            destructorPatch
        )) {
        entries[1].panel.DestructorThunk();
        entries[0].panel.DestructorThunk();
        return 1;
    }

    HudUiPanelLayoutEntry::DestroyRange(entries, entries + 2);
    RestoreFunctionPatch(destructorPatch);

    return g_PanelLayoutDestroyCount == 2 &&
                   g_PanelLayoutDestroyPanels[0] == &entries[0].panel &&
                   g_PanelLayoutDestroyPanels[1] == &entries[1].panel
               ? 0
               : 1;
}

namespace {
int g_primitiveSetPosCount = 0;
HudUiPrimitiveBindTarget *g_primitiveSetPosThis = nullptr;
int g_primitiveSetPosX = 0;
int g_primitiveSetPosY = 0;

struct TestPrimitiveBindTarget : HudUiPrimitiveBindTarget {
    void SetPos(int newX, int newY) override {
        ++g_primitiveSetPosCount;
        g_primitiveSetPosThis = this;
        g_primitiveSetPosX = newX;
        g_primitiveSetPosY = newY;
        HudUiElement::SetPos(newX, newY);
    }
};
} // namespace

extern "C" int zhud_primitive_bind_target_set_segment_endpoints_smoke(void) {
    TestPrimitiveBindTarget target{};
    target.endX = -1;
    target.endY = -1;
    g_primitiveSetPosCount = 0;
    g_primitiveSetPosThis = nullptr;
    g_primitiveSetPosX = 0;
    g_primitiveSetPosY = 0;

    target.SetSegmentEndpoints(11, 22, 33, 44);

    return g_primitiveSetPosCount == 1 &&
                   g_primitiveSetPosThis == &target &&
                   g_primitiveSetPosX == 11 &&
                   g_primitiveSetPosY == 22 &&
                   target.x == 11 &&
                   target.y == 22 &&
                   target.endX == 33 &&
                   target.endY == 44
               ? 0
               : 1;
}

extern "C" int zhud_background_bind_primitive_node_to_element_smoke(void) {
    const int oldRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int oldGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int oldRShift = g_zVideo_PixelPack.packedBase;
    const int oldGShift = g_zVideo_PixelPack.sumMinus8;
    const int oldBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;

    g_zVideo_PixelPack.rMaskShifted = 0xf8;
    g_zVideo_PixelPack.gMaskShifted = 0xfc;
    g_zVideo_PixelPack.packedBase = 8;
    g_zVideo_PixelPack.sumMinus8 = 3;
    g_zVideo_PixelPack.bShiftTo8 = 3;

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

    HudUiBackground background{};
    background.cfgRoot = &root;
    background.uiOriginX = 10;
    background.uiOriginY = 20;
    background.capturedCompositeImage = reinterpret_cast<zVidImagePartial *>(0x1234);

    TestPrimitiveBindTarget target{};
    target.color565 = 0xffff;
    const int result = background.BindPrimitiveNodeToElement(nullptr, &target, "LINE");

    const bool linked =
        result == 0 &&
        background.childHead == &target &&
        background.childTail == &target &&
        target.parent == &background;
    const bool primitive =
        target.x == 15 &&
        target.y == 27 &&
        target.endX == 17 &&
        target.endY == 30 &&
        target.color565 == (zVid_PackColorRGB(0x20, 0x60, 0x40) & 0xffffu) &&
        target.bltSource == reinterpret_cast<zVidImagePartial *>(0x1234) &&
        target.clipRect.left == 15 &&
        target.clipRect.top == 27 &&
        target.clipRect.right == 15 &&
        target.clipRect.bottom == 27 &&
        (target.flags & 0x02) != 0;

    g_zVideo_PixelPack.rMaskShifted = oldRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = oldGMaskShifted;
    g_zVideo_PixelPack.packedBase = oldRShift;
    g_zVideo_PixelPack.sumMinus8 = oldGShift;
    g_zVideo_PixelPack.bShiftTo8 = oldBShiftTo8;
    background.capturedCompositeImage = nullptr;
    return linked && primitive ? 0 : 1;
}

void *g_backgroundUpdateChildElement = nullptr;
void *g_backgroundUpdateFocusElement = nullptr;
int g_backgroundUpdateHitCount = 0;
int g_backgroundUpdateShouldHandleCount = 0;
int g_backgroundUpdateShouldHandleHovered = -1;
int g_backgroundUpdateHoverEnterCount = 0;
int g_backgroundUpdateHoverRepeatCount = 0;
int g_backgroundUpdateHoverExitCount = 0;
int g_backgroundUpdateCaptureEnterCount = 0;
int g_backgroundUpdateCaptureExitCount = 0;
int g_backgroundUpdatePrimaryReleaseCount = 0;
int g_backgroundUpdateSecondaryReleaseCount = 0;
int g_backgroundUpdatePointerStateCount = 0;
int g_backgroundUpdateActivateCount = 0;
int g_backgroundUpdateAfterInputCount = 0;
int g_backgroundUpdateAfterHovered = -1;
int g_backgroundUpdateDrawBaseCount = 0;
int g_backgroundUpdateChildUpdateCount = 0;
int g_backgroundUpdateFocusUpdateCount = 0;
int g_backgroundUpdateSetPosCount = 0;
int g_backgroundUpdateSetPosX = 0;
int g_backgroundUpdateSetPosY = 0;
float g_backgroundUpdateChildDelta = 0.0f;
float g_backgroundUpdateFocusDelta = 0.0f;

struct TestBackgroundUpdateElement : HudUiElement {
    int hitResult;
    int shouldHandleResult;

    int HitTest(
        int x,
        int y
    ) {
        ++g_backgroundUpdateHitCount;
        return x == 123 && y == 456 ? hitResult : 0;
    }

    int ShouldHandleInput(
        HudUiBackground *,
        int hovered
    ) {
        ++g_backgroundUpdateShouldHandleCount;
        g_backgroundUpdateShouldHandleHovered = hovered;
        return shouldHandleResult;
    }

    void DrawBase() {
        ++g_backgroundUpdateDrawBaseCount;
    }

    void SetPos(
        int x,
        int y
    ) {
        ++g_backgroundUpdateSetPosCount;
        g_backgroundUpdateSetPosX = x;
        g_backgroundUpdateSetPosY = y;
        this->x = x;
        this->y = y;
    }

    void Update(
        float deltaSeconds
    ) {
        if (this == g_backgroundUpdateChildElement) {
            ++g_backgroundUpdateChildUpdateCount;
            g_backgroundUpdateChildDelta = deltaSeconds;
        } else if (this == g_backgroundUpdateFocusElement) {
            ++g_backgroundUpdateFocusUpdateCount;
            g_backgroundUpdateFocusDelta = deltaSeconds;
        }
    }

    void OnCapturedPrimaryRelease() {
        ++g_backgroundUpdatePrimaryReleaseCount;
    }

    void OnClearBinding() {
        ++g_backgroundUpdateSecondaryReleaseCount;
    }

    void OnHoverRepeat() {
        ++g_backgroundUpdateHoverRepeatCount;
    }

    void ShowPreview() {
        ++g_backgroundUpdateHoverEnterCount;
    }

    void HidePreview() {
        ++g_backgroundUpdateHoverExitCount;
    }

    void OnBeginCapture() {
        ++g_backgroundUpdateCaptureEnterCount;
    }

    void OnEndCapture() {
        ++g_backgroundUpdateCaptureExitCount;
    }

    void OnPointerButtonState(
        int,
        int
    ) {
        ++g_backgroundUpdatePointerStateCount;
    }

    void OnActivate() {
        ++g_backgroundUpdateActivateCount;
    }

    void AfterInputUpdate(
        HudUiBackground *,
        int hovered
    ) {
        ++g_backgroundUpdateAfterInputCount;
        g_backgroundUpdateAfterHovered = hovered;
    }
};

extern "C" int zhud_background_update_input_focus_smoke(void) {
    TestBackgroundUpdateElement child = {};
    TestBackgroundUpdateElement focus = {};
    child.hitResult = 1;
    child.shouldHandleResult = 1;

    HudUiBackground background = {};
    background.enabled = 1;
    background.childHead = &child;
    background.childTail = &child;
    background.inputFocusElement = &focus;
    background.captureTransitionMask = 4;

    g_zInput_MouseStateSnapshot = {};
    g_zInput_MouseStateSnapshot.cursorClientX = 123;
    g_zInput_MouseStateSnapshot.cursorClientY = 456;
    g_zInput_MouseStateSnapshot.button1Transition = 4;
    g_zInput_MouseStateSnapshot.button2Transition = 0;

    g_backgroundUpdateChildElement = &child;
    g_backgroundUpdateFocusElement = &focus;
    g_backgroundUpdateHitCount = 0;
    g_backgroundUpdateShouldHandleCount = 0;
    g_backgroundUpdateShouldHandleHovered = -1;
    g_backgroundUpdateHoverEnterCount = 0;
    g_backgroundUpdateHoverRepeatCount = 0;
    g_backgroundUpdateHoverExitCount = 0;
    g_backgroundUpdateCaptureEnterCount = 0;
    g_backgroundUpdateCaptureExitCount = 0;
    g_backgroundUpdatePrimaryReleaseCount = 0;
    g_backgroundUpdateSecondaryReleaseCount = 0;
    g_backgroundUpdatePointerStateCount = 0;
    g_backgroundUpdateActivateCount = 0;
    g_backgroundUpdateAfterInputCount = 0;
    g_backgroundUpdateAfterHovered = -1;
    g_backgroundUpdateDrawBaseCount = 0;
    g_backgroundUpdateChildUpdateCount = 0;
    g_backgroundUpdateFocusUpdateCount = 0;
    g_backgroundUpdateSetPosCount = 0;
    g_backgroundUpdateSetPosX = 0;
    g_backgroundUpdateSetPosY = 0;
    g_backgroundUpdateChildDelta = 0.0f;
    g_backgroundUpdateFocusDelta = 0.0f;

    background.Update(0.25f);

    const bool inputDispatched =
        g_backgroundUpdateHitCount == 1 &&
        g_backgroundUpdateShouldHandleCount == 1 &&
        g_backgroundUpdateShouldHandleHovered == 1 &&
        g_backgroundUpdateHoverEnterCount == 1 &&
        g_backgroundUpdateHoverRepeatCount == 0 &&
        g_backgroundUpdateCaptureEnterCount == 1 &&
        g_backgroundUpdatePrimaryReleaseCount == 1 &&
        g_backgroundUpdateActivateCount == 1 &&
        g_backgroundUpdateAfterInputCount == 1 &&
        g_backgroundUpdateAfterHovered == 1 &&
        g_backgroundUpdateSecondaryReleaseCount == 0 &&
        g_backgroundUpdatePointerStateCount == 0 &&
        g_backgroundUpdateHoverExitCount == 0 &&
        g_backgroundUpdateCaptureExitCount == 0 &&
        child.state == 3;

    const bool focusUpdated =
        g_backgroundUpdateDrawBaseCount == 1 &&
        g_backgroundUpdateChildUpdateCount == 1 &&
        g_backgroundUpdateChildDelta == 0.25f &&
        g_backgroundUpdateSetPosCount == 1 &&
        g_backgroundUpdateSetPosX == 123 &&
        g_backgroundUpdateSetPosY == 456 &&
        g_backgroundUpdateFocusUpdateCount == 1 &&
        g_backgroundUpdateFocusDelta == 0.25f &&
        focus.x == 123 &&
        focus.y == 456;

    g_backgroundUpdateChildElement = 0;
    g_backgroundUpdateFocusElement = 0;
    return inputDispatched && focusUpdated ? 0 : 1;
}

extern "C" int zhud_container_child_list_smoke(void) {
    HudUiContainer container{};
    HudUiElement first{};
    HudUiElement second{};
    HudUiElement third{};

    const int firstResult = container.AddChild(&first);
    const int secondResult = container.AddChild(&second);
    const int thirdResult = container.AddChild(&third);
    const bool linked =
        container.enabled == 0 &&
        container.childHead == &first &&
        container.childTail == &third &&
        first.next == &second &&
        second.next == &third &&
        third.next == nullptr &&
        first.parent == &container &&
        second.parent == &container &&
        third.parent == &container;

    HudUiElement *previous = reinterpret_cast<HudUiElement *>(0x1234);
    const bool foundHead =
        container.FindChildWithPrev(&first, &previous) == 1 && previous == nullptr;
    previous = reinterpret_cast<HudUiElement *>(0x1234);
    const bool foundSecond =
        container.FindChildWithPrev(&second, &previous) == 1 && previous == &first;
    const bool foundThirdNoPrevious = container.FindChildWithPrev(&third, nullptr) == 1;
    previous = reinterpret_cast<HudUiElement *>(0x9abc);
    const bool missing =
        container.FindChildWithPrev(reinterpret_cast<HudUiElement *>(0x5678), &previous) == 0 &&
        previous == reinterpret_cast<HudUiElement *>(0x9abc);

    HudUiContainer emptyContainer{};
    previous = reinterpret_cast<HudUiElement *>(0xdef0);
    const bool emptyMissing =
        emptyContainer.FindChildWithPrev(&first, &previous) == 0 &&
        previous == reinterpret_cast<HudUiElement *>(0xdef0);

    first.flags = 0;
    second.flags = 0x10;
    third.flags = 0x20;
    container.SetChildFlags(0x02);
    const bool flagsSet = first.flags == 0x02 && second.flags == 0x12 && third.flags == 0x02;

    const bool removedMiddle =
        container.RemoveChild(&second) == 1 &&
        first.next == &third &&
        second.next == nullptr &&
        second.parent == nullptr &&
        container.childTail == &third;

    const bool removedHead =
        container.RemoveChild(&first) == 1 &&
        container.childHead == &third &&
        first.next == nullptr &&
        first.parent == nullptr;

    const bool removedTail =
        container.RemoveChild(&third) == 1 &&
        container.childHead == nullptr &&
        container.childTail == nullptr &&
        third.next == nullptr &&
        third.parent == nullptr;

    return firstResult == 1 && secondResult == 1 && thirdResult == 1 && linked &&
                   foundHead && foundSecond && foundThirdNoPrevious && missing && emptyMissing &&
                   flagsSet && removedMiddle && removedHead && removedTail
               ? 0
               : 1;
}

extern "C" int zhud_zrd_widget_constructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiZrdWidget));
    std::memset(storage, 0xcc, sizeof(HudUiZrdWidget));

    HudUiZrdWidget *const widget = reinterpret_cast<HudUiZrdWidget *>(storage);
    HudUiZrdWidget *const result = widget->Constructor();

    const bool initialized =
        result == widget &&
        widget->originX == 0 &&
        widget->originY == 0 &&
        widget->modeOrEnabled == 1 &&
        widget->owner == nullptr &&
        widget->defaultImage == nullptr &&
        widget->disabledImage == nullptr &&
        widget->rolloverImage == nullptr &&
        widget->rolloverSound == nullptr &&
        widget->rolloverPlayHandle == nullptr &&
        widget->rolloverSoundScale == 1.0f &&
        widget->activateImage == nullptr &&
        widget->activateSound == nullptr &&
        widget->activatePlayHandle == nullptr &&
        widget->activateSoundScale == 1.0f &&
        widget->labelPanels.begin == nullptr &&
        widget->labelPanels.end == nullptr &&
        widget->labelPanels.capacityEnd == nullptr &&
        widget->rolloverLabelPanels.begin == nullptr &&
        widget->rolloverLabelPanels.end == nullptr &&
        widget->rolloverLabelPanels.capacityEnd == nullptr &&
        widget->activateLabelPanels.begin == nullptr &&
        widget->activateLabelPanels.end == nullptr &&
        widget->activateLabelPanels.capacityEnd == nullptr &&
        widget->disabledLabelPanels.begin == nullptr &&
        widget->disabledLabelPanels.end == nullptr &&
        widget->disabledLabelPanels.capacityEnd == nullptr &&
        (widget->flags & 0x02u) != 0 &&
        (widget->flags & ~0x12u) == 0 &&
        (widget->imageStateWord & 0xffffu) == 1;

    widget->DestructorCore();
    ::operator delete(storage);
    return initialized ? 0 : 1;
}

extern "C" int zhud_options_dialog_constructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    g_optionsDialogLoadCalls = 0;
    g_optionsDialogLoadArgsOk = false;
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            OptionsDialogLoadProbeAddress(),
            loadPatch
        )) {
        return 1;
    }

    void *const storage = ::operator new(sizeof(HudOptionsDialog));
    HudOptionsDialog *const dialog = new (storage) HudOptionsDialog;
    RestoreFunctionPatch(loadPatch);

    const bool constructed =
        dialog != nullptr &&
        g_optionsDialogLoadCalls == 1 &&
        g_optionsDialogLoadArgsOk &&
        dialog->soundVolumeWidget.normalizedValue == 0.0f &&
        dialog->musicVolumeWidget.normalizedValue == 0.0f &&
        dialog->resolutionSelector.selectedIndex == 0 &&
        dialog->backButton.owner == nullptr;

    dialog->DestructorCore();
    ::operator delete(storage);
    return constructed ? 0 : 1;
}

extern "C" int zhud_credits_panel_constructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    g_creditsPanelLoadCalls = 0;
    g_creditsPanelLoadArgsOk = false;
    const int oldQuitAfterCredits = g_RecoilApp_QuitAfterCredits;
    g_RecoilApp_QuitAfterCredits = 0;
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CreditsPanelLoadProbeAddress(),
            loadPatch
        )) {
        g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
        return 1;
    }

    void *const storage = ::operator new(sizeof(HudUiCreditsPanel));
    HudUiCreditsPanel *const panel = new (storage) HudUiCreditsPanel;
    RestoreFunctionPatch(loadPatch);

    const bool constructed =
        panel != nullptr &&
        g_creditsPanelLoadCalls == 1 &&
        g_creditsPanelLoadArgsOk &&
        panel->fadeProgress == 0.0f &&
        panel->fadeStep > 0.049f &&
        panel->fadeStep < 0.051f &&
        panel->backButton.owner == nullptr &&
        panel->quitButton.owner == nullptr &&
        panel->creditsScreen.rows.begin == nullptr &&
        panel->creditsScreen.rows.end == nullptr &&
        panel->creditsScreen.rows.cap == nullptr &&
        (panel->creditsScreen.flags & ~0x10u) == 0;

    panel->Destructor();
    ::operator delete(storage);
    g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
    return constructed ? 0 : 1;
}

extern "C" int zhud_credits_panel_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        storage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiCreditsPanel *const panel = static_cast<HudUiCreditsPanel *>(storage);
    InitCreditsPanelForDestructor(panel);
    panel->Destructor();
    const int failure = CreditsPanelDestructorFailureBits(*panel);
    ::operator delete(storage);
    return failure;
}

extern "C" int zhud_credits_panel_scalar_deleting_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        storage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiCreditsPanel *const panel = static_cast<HudUiCreditsPanel *>(storage);
    InitCreditsPanelForDestructor(panel);
    HudUiCreditsPanel *const result = panel->ScalarDeletingDestructor(0);

    int failure = result == panel ? 0 : 1;
    failure |= CreditsPanelDestructorFailureBits(*panel) << 1;
    ::operator delete(storage);
    return failure;
}

extern "C" int zhud_scrolling_text_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiZrdScrollingText));
    std::memset(
        storage,
        0,
        sizeof(HudUiZrdScrollingText)
    );
    HudUiZrdScrollingText *const text = static_cast<HudUiZrdScrollingText *>(storage);
    InitScrollingCreditsTextForDestructor(text);
    text->Destructor();
    const int failure = ScrollingCreditsTextDestructorFailureBits(*text);
    ::operator delete(storage);
    return failure;
}

extern "C" int zhud_scrolling_text_scalar_deleting_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiZrdScrollingText));
    std::memset(
        storage,
        0,
        sizeof(HudUiZrdScrollingText)
    );
    HudUiZrdScrollingText *const text = static_cast<HudUiZrdScrollingText *>(storage);
    InitScrollingCreditsTextForDestructor(text);
    HudUiElement *const result = text->ScalarDeletingDestructor(0);

    int failure = result == text ? 0 : 1;
    failure |= ScrollingCreditsTextDestructorFailureBits(*text) << 1;
    ::operator delete(storage);
    return failure;
}

extern "C" int zhud_scrolling_text_on_activate_reset_owner_fade_smoke(void) {
    unsigned char ownerStorage[sizeof(HudUiCreditsPanel)] = {};
    HudUiCreditsPanel *const owner = reinterpret_cast<HudUiCreditsPanel *>(ownerStorage);
    owner->fadeProgress = 0.75f;

    HudUiZrdScrollingText text{};
    text.owner = owner;
    text.OnActivateResetOwnerFade();

    return ZhudFloatNear(
               owner->fadeProgress,
               0.0f
           )
               ? 0
               : 1;
}

extern "C" int zhud_scrolling_text_update_scroll_positions_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiZrdScrollingText));
    std::memset(
        storage,
        0,
        sizeof(HudUiZrdScrollingText)
    );
    HudUiZrdScrollingText *const text = new (storage) HudUiZrdScrollingText;
    text->rect.left = 10;
    text->rect.top = 100;
    text->rect.bottom = 200;
    text->totalHeight = 50;

    HudUiPanelSpan row{};
    row.begin = AllocateCreditsPanelEntries(3);
    row.end = row.begin + 3;
    row.cap = row.end;
    InitCreditsPanelEntry(&row.begin[0], "first", 1, -30);
    InitCreditsPanelEntry(&row.begin[1], "second", 2, -10);
    InitCreditsPanelEntry(&row.begin[2], "third", 3, 70);
    ZhudFieldAt<int>(&row.begin[0].panel, 0x260) = 20;
    ZhudFieldAt<int>(&row.begin[1].panel, 0x260) = 20;
    ZhudFieldAt<int>(&row.begin[2].panel, 0x260) = 20;

    text->rows.begin = &row;
    text->rows.end = &row + 1;
    text->rows.cap = text->rows.end;
    text->UpdateScrollPositions(0.5f);

    const bool positions =
        row.begin[0].panel.x == 11 && row.begin[0].panel.y == 95 &&
        row.begin[1].panel.x == 12 && row.begin[1].panel.y == 115 &&
        row.begin[2].panel.x == 13 && row.begin[2].panel.y == 195;
    const bool visibility =
        (row.begin[0].panel.flags & 0x10u) != 0 &&
        (row.begin[1].panel.flags & 0x10u) == 0 &&
        (row.begin[2].panel.flags & 0x10u) != 0;

    row.DestroyAndFree();
    ::operator delete(storage);
    return positions && visibility ? 0 : 1;
}

extern "C" int zhud_credits_panel_update_fade_and_exit_smoke(void) {
    unsigned char oldApp[sizeof(g_RecoilApp)];
    std::memcpy(
        oldApp,
        &g_RecoilApp,
        sizeof(oldApp)
    );
    const int oldQuitAfterCredits = g_RecoilApp_QuitAfterCredits;
    std::memset(
        &g_RecoilApp,
        0,
        sizeof(g_RecoilApp)
    );
    g_RecoilApp.m_currentStateIndex = -1;
    g_RecoilApp_QuitAfterCredits = 0;

    void *const belowStorage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        belowStorage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiCreditsPanel *const belowPanel = static_cast<HudUiCreditsPanel *>(belowStorage);
    InitCreditsPanelForUpdate(
        belowPanel,
        0.25f,
        0.5f
    );
    belowPanel->UpdateFadeAndExit(0.5f);
    const bool belowThreshold =
        ZhudFloatNear(
            belowPanel->fadeProgress,
            0.5f
        ) &&
        g_RecoilApp.m_stateQueue.m_itemCount == 0;
    ::operator delete(belowStorage);

    void *const normalStorage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        normalStorage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiCreditsPanel *const normalPanel = static_cast<HudUiCreditsPanel *>(normalStorage);
    InitCreditsPanelForUpdate(
        normalPanel,
        1.0f,
        0.0f
    );
    normalPanel->UpdateFadeAndExit(0.25f);
    RecoilApp_StateQueueItem *const item = CreditsQueueItemAt(
        g_RecoilApp.m_stateQueue,
        0
    );
    const bool normalExit =
        g_RecoilApp.m_stateQueue.m_itemCount == 1 &&
        item != nullptr &&
        item->m_kind == RecoilApp_StateQueueKind_ExitCurrent &&
        item->m_stateObj == nullptr &&
        item->m_param == 0;
    CleanupCreditsQueue(g_RecoilApp.m_stateQueue);
    ::operator delete(normalStorage);

    g_RecoilApp_QuitAfterCredits = 1;
    CreditsLeaveNetworkState leaveState;
    *reinterpret_cast<void **>(&g_RecoilApp.m_leaveNetworkState) =
        *reinterpret_cast<void **>(&leaveState);
    void *const quitStorage = ::operator new(sizeof(HudUiCreditsPanel));
    std::memset(
        quitStorage,
        0,
        sizeof(HudUiCreditsPanel)
    );
    HudUiCreditsPanel *const quitPanel = static_cast<HudUiCreditsPanel *>(quitStorage);
    InitCreditsPanelForUpdate(
        quitPanel,
        1.0f,
        0.0f
    );
    quitPanel->UpdateFadeAndExit(0.25f);
    RecoilApp_StateQueueItem *const exitItem = CreditsQueueItemAt(
        g_RecoilApp.m_stateQueue,
        0
    );
    RecoilApp_StateQueueItem *const switchItem = CreditsQueueItemAt(
        g_RecoilApp.m_stateQueue,
        1
    );
    const bool quitExit =
        g_RecoilApp.m_stateQueue.m_itemCount == 2 &&
        exitItem != nullptr &&
        exitItem->m_kind == RecoilApp_StateQueueKind_ExitCurrent &&
        exitItem->m_param == 1 &&
        switchItem != nullptr &&
        switchItem->m_kind == RecoilApp_StateQueueKind_SwitchCurrent &&
        switchItem->m_stateObj == &g_RecoilApp.m_leaveNetworkState &&
        switchItem->m_param == 0 &&
        g_RecoilApp.m_missionShutdownMode == RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
    CleanupCreditsQueue(g_RecoilApp.m_stateQueue);
    ::operator delete(quitStorage);

    std::memcpy(
        &g_RecoilApp,
        oldApp,
        sizeof(g_RecoilApp)
    );
    g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
    return belowThreshold && normalExit && quitExit ? 0 : 1;
}

extern "C" int zhud_cmd_bind_button_base_constructor_smoke(void) {
    void *const itemStorage = ::operator new(sizeof(HudUiListSelectorItem));
    void *const buttonStorage = ::operator new(sizeof(HudCmdBindButtonBase));
    std::memset(itemStorage, 0, sizeof(HudUiListSelectorItem));
    std::memset(buttonStorage, 0, sizeof(HudCmdBindButtonBase));
    HudUiListSelectorItem *const item = new (itemStorage) HudUiListSelectorItem;
    HudCmdBindButtonBase *const button = new (buttonStorage) HudCmdBindButtonBase;

    const bool listItemConstructed =
        item->textBuffer[0] == 0 &&
        item->x == 0 &&
        item->y == 0 &&
        item->textPick == nullptr &&
        item->textColor0 == 0x00ffffff &&
        item->textColor1 == 0x00ffffff &&
        item->textDirty == 1;

    const bool buttonConstructed =
        button->bindingSlotTotalCount == 0 &&
        button->bindingSlotPanels == nullptr &&
        button->bindingVec.begin == nullptr &&
        button->bindingVec.end == nullptr &&
        button->bindingVec.capacity == nullptr &&
        button->bindingSlotSpacing == 0xf &&
        button->selectedBindingIndex == -1 &&
        button->visibleListOffsetX == 0.0f &&
        button->visibleListOffsetY == 0.0f &&
        button->overflowListOffsetX == 0.0f &&
        button->overflowListOffsetY == 0.0f &&
        button->bindPanel.textBuffer[0] == 0 &&
        button->bindPanel.x == 0 &&
        button->bindPanel.y == 0 &&
        button->bindPanel.textPick == nullptr &&
        button->bindPanel.textDirty == 1;

    ::operator delete(buttonStorage);
    ::operator delete(itemStorage);
    return listItemConstructed && buttonConstructed ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_on_command_selection_changed_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    SetupHudCmdDialogButtons(dialog);

    dialog.OnCommandSelectionChanged(1);

    const bool selected =
        dialog.descriptionPanel.captureState == 0 &&
        dialog.commandList.selectedBindingIndex == 1 &&
        dialog.keyAButton.selectedBindingIndex == 1 &&
        dialog.keyBButton.selectedBindingIndex == 1 &&
        dialog.joyButton.selectedBindingIndex == 1 &&
        dialog.mouseButton.selectedBindingIndex == 1;
    const bool labels =
        std::strcmp(dialog.commandList.bindPanel.textBuffer, "CommandSeven") == 0 &&
        std::strcmp(dialog.keyAButton.bindPanel.textBuffer, "KeyA1") == 0 &&
        std::strcmp(dialog.keyBButton.bindPanel.textBuffer, "KeyB1") == 0 &&
        std::strcmp(dialog.joyButton.bindPanel.textBuffer, "Joy1") == 0 &&
        std::strcmp(dialog.mouseButton.bindPanel.textBuffer, "Mouse1") == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return selected && labels ? 0 : 1;
}

extern "C" int zhud_cmd_bind_button_base_on_selection_changed_refresh_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    SetupHudCmdDialogButtons(dialog);
    dialog.keyAButton.owner = &dialog;

    dialog.keyAButton.OnSelectionChangedRefresh(1);

    const bool selected =
        dialog.descriptionPanel.captureState == 0 &&
        dialog.commandList.selectedBindingIndex == 1 &&
        dialog.keyAButton.selectedBindingIndex == 1 &&
        dialog.keyBButton.selectedBindingIndex == 1 &&
        dialog.joyButton.selectedBindingIndex == 1 &&
        dialog.mouseButton.selectedBindingIndex == 1 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return selected ? 0 : 1;
}

extern "C" int zhud_cmd_reset_button_on_activate_smoke(void) {
    CodeFunctionPatch defaultsPatch{};
    CodeFunctionPatch lookupPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch activatePatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_InitDefaultBindings),
            reinterpret_cast<void *>(&FakeHudCmdInitDefaultBindings),
            defaultsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_Current_RebuildLookupIndices),
            reinterpret_cast<void *>(&FakeHudCmdRebuildLookupIndices),
            lookupPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiZrdWidget::OnActivate),
            HudCmdZrdActivateProbeAddress(),
            activatePatch
        )) {
        RestoreFunctionPatch(defaultsPatch);
        RestoreFunctionPatch(lookupPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(activatePatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.resetButton.owner = &dialog;
    dialog.setList.selectedIndex = 3;
    g_cmdResetDefaultBindingCalls = 0;
    g_cmdResetLookupRebuildCalls = 0;
    g_cmdResetBaseActivateCalls = 0;
    g_cmdDialogRebuildCalls = 0;
    g_cmdDialogRebuildGroup = -1;

    dialog.resetButton.OnActivate();

    int failureCode = 0;
    if (g_cmdResetDefaultBindingCalls != 1) {
        failureCode = 3;
    } else if (g_cmdResetLookupRebuildCalls != 1) {
        failureCode = 4;
    } else if (g_cmdDialogRebuildCalls != 1) {
        failureCode = 5;
    } else if (g_cmdDialogRebuildGroup != 3) {
        failureCode = 6;
    }

    RestoreFunctionPatch(defaultsPatch);
    RestoreFunctionPatch(lookupPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(activatePatch);
    return failureCode;
}

extern "C" int zhud_cmd_key_a_button_on_begin_capture_smoke(void) {
    CodeFunctionPatch resetPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::ResetAllTransitionState),
            reinterpret_cast<void *>(&FakeHudCmdRebuildLookupIndices),
            resetPatch
        )) {
        RestoreFunctionPatch(resetPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.captureState = 0;
    dialog.keyAButton.owner = &dialog;
    g_cmdResetLookupRebuildCalls = 0;

    dialog.keyAButton.OnBeginCapture();

    int failureCode = 0;
    if (dialog.descriptionPanel.captureState != 1) {
        failureCode = 3;
    }

    RestoreFunctionPatch(resetPatch);
    return failureCode;
}

extern "C" int zhud_cmd_dialog_rebuild_command_binding_lists_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.descriptionPanel.captureState = 77;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(
        5,
        "CmdFiveCurrent",
        0x1e,
        0x30,
        2,
        1
    );
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.commandIds.begin = commandIds;
    group.commandIds.end = commandIds + 1;
    group.commandIds.capacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    dialog.RebuildCommandBindingListsForGroup(0);

    HudCmdBindingEntry **const commandBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    HudCmdBindingEntry **const keyABegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyAButton.bindingVec.begin);
    HudCmdBindingEntry **const keyBBegin =
        static_cast<HudCmdBindingEntry **>(dialog.keyBButton.bindingVec.begin);
    HudCmdBindingEntry **const joyBegin =
        static_cast<HudCmdBindingEntry **>(dialog.joyButton.bindingVec.begin);
    HudCmdBindingEntry **const mouseBegin =
        static_cast<HudCmdBindingEntry **>(dialog.mouseButton.bindingVec.begin);
    const bool rebuilt =
        dialog.commandList.bindingVec.end == commandBegin + 1 &&
        dialog.keyAButton.bindingVec.end == keyABegin + 1 &&
        dialog.keyBButton.bindingVec.end == keyBBegin + 1 &&
        dialog.joyButton.bindingVec.end == joyBegin + 1 &&
        dialog.mouseButton.bindingVec.end == mouseBegin + 1 &&
        commandBegin[0]->commandId == 5 &&
        keyABegin[0]->commandId == 5 &&
        keyBBegin[0]->commandId == 5 &&
        joyBegin[0]->commandId == 5 &&
        mouseBegin[0]->commandId == 5 &&
        std::strcmp(commandBegin[0]->displayText, "CommandFive") == 0 &&
        std::strcmp(keyABegin[0]->displayText, "A") == 0 &&
        std::strcmp(keyBBegin[0]->displayText, "B") == 0 &&
        std::strcmp(joyBegin[0]->displayText, "Button 2") == 0 &&
        std::strcmp(mouseBegin[0]->displayText, "Left") == 0 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        dialog.keyAButton.selectedBindingIndex == 0 &&
        dialog.keyBButton.selectedBindingIndex == 0 &&
        dialog.joyButton.selectedBindingIndex == 0 &&
        dialog.mouseButton.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return rebuilt ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_select_group_relative_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    dialog.setList.selectedIndex = 1;
    dialog.setList.itemCount = 3;
    dialog.setList.firstIndex = 0;
    dialog.setList.visibleCount = 3;
    SetupHudCmdDialogButtons(dialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandSevenLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    labels[7] = commandSevenLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    context.SetBindingRecord(7, "CmdSevenCurrent", 0x20, 0x31, 3, 3);
    g_zInput_BindMap_Current = &context;

    int groupZeroCommandIds[] = {5};
    int groupOneCommandIds[] = {5};
    int groupTwoCommandIds[] = {7};
    zInput_BindGroupInfo groupsStorage[3] = {};
    groupsStorage[0].commandIds.begin = groupZeroCommandIds;
    groupsStorage[0].commandIds.end = groupZeroCommandIds + 1;
    groupsStorage[0].commandIds.capacity = groupZeroCommandIds + 1;
    groupsStorage[1].commandIds.begin = groupOneCommandIds;
    groupsStorage[1].commandIds.end = groupOneCommandIds + 1;
    groupsStorage[1].commandIds.capacity = groupOneCommandIds + 1;
    groupsStorage[2].commandIds.begin = groupTwoCommandIds;
    groupsStorage[2].commandIds.end = groupTwoCommandIds + 1;
    groupsStorage[2].commandIds.capacity = groupTwoCommandIds + 1;
    zInput_BindGroupInfo *groups[] = {&groupsStorage[0], &groupsStorage[1], &groupsStorage[2]};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 3;
    g_zInput_BindGroupInfoList.capacity = groups + 3;

    const int forwardResult = dialog.SelectGroupRelative(1);
    HudCmdBindingEntry **const forwardBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool forward =
        forwardResult == 2 &&
        dialog.setList.selectedIndex == 2 &&
        dialog.commandList.bindingVec.end == forwardBegin + 1 &&
        forwardBegin[0]->commandId == 7;

    const int wrapForwardResult = dialog.SelectGroupRelative(1);
    HudCmdBindingEntry **const wrapForwardBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool wrapForward =
        wrapForwardResult == 0 &&
        dialog.setList.selectedIndex == 0 &&
        dialog.commandList.bindingVec.end == wrapForwardBegin + 1 &&
        wrapForwardBegin[0]->commandId == 5;

    const int wrapBackwardResult = dialog.SelectGroupRelative(-1);
    HudCmdBindingEntry **const wrapBackwardBegin =
        static_cast<HudCmdBindingEntry **>(dialog.commandList.bindingVec.begin);
    const bool wrapBackward =
        wrapBackwardResult == 2 &&
        dialog.setList.selectedIndex == 2 &&
        dialog.commandList.bindingVec.end == wrapBackwardBegin + 1 &&
        wrapBackwardBegin[0]->commandId == 7;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return forward && wrapForward && wrapBackward ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_select_command_relative_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog dialog{};
    dialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    SetupHudCmdDialogButtons(dialog);
    dialog.commandList.selectedBindingIndex = 0;

    const int forwardResult = dialog.SelectCommandRelative(1);
    const bool forward =
        forwardResult == 1 &&
        dialog.commandList.selectedBindingIndex == 1 &&
        dialog.keyAButton.selectedBindingIndex == 1 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    const int outOfRangeForward = dialog.SelectCommandRelative(1);
    const bool clampedForward =
        outOfRangeForward == 1 &&
        dialog.commandList.selectedBindingIndex == 1;

    const int backwardResult = dialog.SelectCommandRelative(-1);
    const bool backward =
        backwardResult == 0 &&
        dialog.commandList.selectedBindingIndex == 0 &&
        std::strcmp(dialog.descriptionPanel.textBuffer, "HintFive") == 0;

    const int outOfRangeBackward = dialog.SelectCommandRelative(-1);
    const bool clampedBackward =
        outOfRangeBackward == 0 &&
        dialog.commandList.selectedBindingIndex == 0;

    CleanupHudCmdDialogButtons(dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(dialog);
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return forward && clampedForward && backward && clampedBackward ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_callback_navigation_smoke(void) {
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        )) {
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        return 2;
    }

    HudCmdDialog setDialog{};
    setDialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    setDialog.setList.selectedIndex = 0;
    setDialog.setList.itemCount = 3;
    setDialog.setList.firstIndex = 0;
    setDialog.setList.visibleCount = 3;
    setDialog.nextSetButton.owner = &setDialog;
    setDialog.prevSetButton.owner = &setDialog;
    SetupHudCmdDialogButtons(setDialog);

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char commandSevenLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    labels[7] = commandSevenLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    context.SetBindingRecord(7, "CmdSevenCurrent", 0x20, 0x31, 3, 3);
    g_zInput_BindMap_Current = &context;

    int groupZeroCommandIds[] = {5};
    int groupOneCommandIds[] = {7};
    int groupTwoCommandIds[] = {5};
    zInput_BindGroupInfo groupsStorage[3] = {};
    groupsStorage[0].commandIds.begin = groupZeroCommandIds;
    groupsStorage[0].commandIds.end = groupZeroCommandIds + 1;
    groupsStorage[0].commandIds.capacity = groupZeroCommandIds + 1;
    groupsStorage[1].commandIds.begin = groupOneCommandIds;
    groupsStorage[1].commandIds.end = groupOneCommandIds + 1;
    groupsStorage[1].commandIds.capacity = groupOneCommandIds + 1;
    groupsStorage[2].commandIds.begin = groupTwoCommandIds;
    groupsStorage[2].commandIds.end = groupTwoCommandIds + 1;
    groupsStorage[2].commandIds.capacity = groupTwoCommandIds + 1;
    zInput_BindGroupInfo *groups[] = {&groupsStorage[0], &groupsStorage[1], &groupsStorage[2]};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 3;
    g_zInput_BindGroupInfoList.capacity = groups + 3;

    setDialog.nextSetButton.OnActivate();
    HudCmdBindingEntry **const nextSetBegin =
        static_cast<HudCmdBindingEntry **>(setDialog.commandList.bindingVec.begin);
    const bool nextSet =
        setDialog.setList.selectedIndex == 1 &&
        setDialog.commandList.bindingVec.end == nextSetBegin + 1 &&
        nextSetBegin[0]->commandId == 7;

    setDialog.prevSetButton.OnActivate();
    HudCmdBindingEntry **const prevSetBegin =
        static_cast<HudCmdBindingEntry **>(setDialog.commandList.bindingVec.begin);
    const bool prevSet =
        setDialog.setList.selectedIndex == 0 &&
        setDialog.commandList.bindingVec.end == prevSetBegin + 1 &&
        prevSetBegin[0]->commandId == 5;

    CleanupHudCmdDialogButtons(setDialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(setDialog);

    HudCmdDialog commandDialog{};
    commandDialog.descriptionPanel.ConstructorDefault("stale", 0, 0);
    commandDialog.nextCommandButton.owner = &commandDialog;
    commandDialog.prevCommandButton.owner = &commandDialog;
    SetupHudCmdDialogButtons(commandDialog);
    commandDialog.commandList.selectedBindingIndex = 0;

    commandDialog.nextCommandButton.OnActivate();
    const bool nextCommand =
        commandDialog.commandList.selectedBindingIndex == 1 &&
        commandDialog.keyAButton.selectedBindingIndex == 1 &&
        std::strcmp(commandDialog.descriptionPanel.textBuffer, "HintSeven") == 0;

    commandDialog.prevCommandButton.OnActivate();
    const bool prevCommand =
        commandDialog.commandList.selectedBindingIndex == 0 &&
        commandDialog.keyAButton.selectedBindingIndex == 0 &&
        std::strcmp(commandDialog.descriptionPanel.textBuffer, "HintFive") == 0;

    CleanupHudCmdDialogButtons(commandDialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(commandDialog);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    return nextSet && prevSet && nextCommand && prevCommand ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_constructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch labelPatch{};
    CodeFunctionPatch hintPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch setChildFlagsPatch{};
    CodeFunctionPatch groupCountPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CmdDialogLoadProbeAddress(),
            loadPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandLabel),
            reinterpret_cast<void *>(&FakeHudCmdCommandLabel),
            labelPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindMap_GetCommandHint),
            reinterpret_cast<void *>(&FakeHudCmdCommandHint),
            hintPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiContainer::SetChildFlags),
            HudUiContainerSetChildFlagsProbeAddress(),
            setChildFlagsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindGroupList_GetCount),
            reinterpret_cast<void *>(&FakeHudCmdBindGroupCount),
            groupCountPatch
        )) {
        RestoreFunctionPatch(loadPatch);
        RestoreFunctionPatch(labelPatch);
        RestoreFunctionPatch(hintPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(setChildFlagsPatch);
        RestoreFunctionPatch(groupCountPatch);
        return 2;
    }

    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    zInput_BindGroupInfoList oldGroups = g_zInput_BindGroupInfoList;
    InitHudCmdInputTables();

    zInput_BindMapContext context{};
    int packedBindings[16] = {};
    zInputCommandCallbackFn callbacks[16] = {};
    char commandFiveLabel[0x50] = {};
    char *labels[16] = {};
    labels[5] = commandFiveLabel;
    context.m_commandCount = 16;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    context.SetBindingRecord(5, "CmdFiveCurrent", 0x1e, 0x30, 2, 1);
    g_zInput_BindMap_Current = &context;

    int commandIds[] = {5};
    zInput_BindGroupInfo group{};
    group.title = const_cast<char *>("GroupOne");
    group.commandIds.begin = commandIds;
    group.commandIds.end = commandIds + 1;
    group.commandIds.capacity = commandIds + 1;
    zInput_BindGroupInfo *groups[] = {&group};
    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    g_cmdDialogLoadCalls = 0;
    g_cmdDialogLoadArgsOk = false;
    g_cmdDialogRebuildCalls = 0;
    g_cmdDialogRebuildGroup = -1;
    g_cmdDialogSetChildFlagsCalls = 0;
    g_cmdDialogSetChildFlagsValue = -1;
    void *const storage = ::operator new(sizeof(HudCmdDialog));
    std::memset(storage, 0, sizeof(HudCmdDialog));
    HudCmdDialog *const dialog = static_cast<HudCmdDialog *>(storage);
    HudCmdDialog *const result = dialog->Constructor();

    int failureCode = 0;
    if (result != dialog) {
        failureCode = 3;
    } else if (g_cmdDialogLoadCalls != 1 || !g_cmdDialogLoadArgsOk) {
        failureCode = 4;
    } else if (dialog->descriptionPanel.captureState != 0) {
        failureCode = 5;
    } else if (dialog->setList.itemCount != 0) {
        failureCode = 6;
    } else if (g_cmdDialogRebuildCalls != 1 || g_cmdDialogRebuildGroup != 0) {
        failureCode = 7;
    } else if (g_cmdDialogSetChildFlagsCalls != 1 || g_cmdDialogSetChildFlagsValue != 0) {
        failureCode = 8;
    } else if (dialog->commandList.bindingVec.begin != nullptr ||
               dialog->keyAButton.bindingVec.begin != nullptr ||
               dialog->mouseButton.bindingVec.begin != nullptr) {
        failureCode = 9;
    }

    CleanupHudCmdDialogButtons(*dialog);
    DestroyHudCmdDialogDescriptionPanelForSmoke(*dialog);
    ::operator delete(storage);
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_BindGroupInfoList = oldGroups;
    RestoreFunctionPatch(loadPatch);
    RestoreFunctionPatch(labelPatch);
    RestoreFunctionPatch(hintPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(setChildFlagsPatch);
    RestoreFunctionPatch(groupCountPatch);
    return failureCode;
}

extern "C" int zhud_cmd_dialog_destructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch setChildFlagsPatch{};
    CodeFunctionPatch groupCountPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CmdDialogLoadProbeAddress(),
            loadPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiContainer::SetChildFlags),
            HudUiContainerSetChildFlagsProbeAddress(),
            setChildFlagsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindGroupList_GetCount),
            reinterpret_cast<void *>(&FakeHudCmdBindGroupCount),
            groupCountPatch
        )) {
        RestoreFunctionPatch(loadPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(setChildFlagsPatch);
        RestoreFunctionPatch(groupCountPatch);
        return 2;
    }

    HudCmdDialog *const dialog =
        static_cast<HudCmdDialog *>(::operator new(sizeof(HudCmdDialog)));
    std::memset(dialog, 0, sizeof(HudCmdDialog));
    dialog->Constructor();

    InstallHudCmdDialogBinding(dialog->commandList, "Command");
    InstallHudCmdDialogBinding(dialog->keyAButton, "KeyA");
    InstallHudCmdDialogBinding(dialog->keyBButton, "KeyB");
    InstallHudCmdDialogBinding(dialog->joyButton, "Joy");
    InstallHudCmdDialogBinding(dialog->mouseButton, "Mouse");

    dialog->Destructor();

    const bool bindingsCleared =
        dialog->commandList.bindingVec.begin == nullptr &&
        dialog->commandList.bindingVec.end == nullptr &&
        dialog->commandList.bindingVec.capacity == nullptr &&
        dialog->keyAButton.bindingVec.begin == nullptr &&
        dialog->keyAButton.bindingVec.end == nullptr &&
        dialog->keyAButton.bindingVec.capacity == nullptr &&
        dialog->keyBButton.bindingVec.begin == nullptr &&
        dialog->keyBButton.bindingVec.end == nullptr &&
        dialog->keyBButton.bindingVec.capacity == nullptr &&
        dialog->joyButton.bindingVec.begin == nullptr &&
        dialog->joyButton.bindingVec.end == nullptr &&
        dialog->joyButton.bindingVec.capacity == nullptr &&
        dialog->mouseButton.bindingVec.begin == nullptr &&
        dialog->mouseButton.bindingVec.end == nullptr &&
        dialog->mouseButton.bindingVec.capacity == nullptr;

    ::operator delete(dialog);
    RestoreFunctionPatch(loadPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(setChildFlagsPatch);
    RestoreFunctionPatch(groupCountPatch);
    return bindingsCleared ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_scalar_deleting_destructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch rebuildPatch{};
    CodeFunctionPatch setChildFlagsPatch{};
    CodeFunctionPatch groupCountPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            CmdDialogLoadProbeAddress(),
            loadPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudCmdDialog::RebuildCommandBindingListsForGroup),
            CmdDialogRebuildProbeAddress(),
            rebuildPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiContainer::SetChildFlags),
            HudUiContainerSetChildFlagsProbeAddress(),
            setChildFlagsPatch
        ) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zInput::BindGroupList_GetCount),
            reinterpret_cast<void *>(&FakeHudCmdBindGroupCount),
            groupCountPatch
        )) {
        RestoreFunctionPatch(loadPatch);
        RestoreFunctionPatch(rebuildPatch);
        RestoreFunctionPatch(setChildFlagsPatch);
        RestoreFunctionPatch(groupCountPatch);
        return 2;
    }

    HudCmdDialog *const dialog =
        static_cast<HudCmdDialog *>(::operator new(sizeof(HudCmdDialog)));
    std::memset(dialog, 0, sizeof(HudCmdDialog));
    dialog->Constructor();
    InstallHudCmdDialogBinding(dialog->commandList, "Command");

    HudCmdDialog *const returned = dialog->ScalarDeletingDestructor(0);
    const bool noDeletePath =
        returned == dialog &&
        dialog->commandList.bindingVec.begin == nullptr &&
        dialog->commandList.bindingVec.end == nullptr &&
        dialog->commandList.bindingVec.capacity == nullptr;
    ::operator delete(dialog);

    HudCmdDialog *const deletingDialog =
        static_cast<HudCmdDialog *>(::operator new(sizeof(HudCmdDialog)));
    std::memset(deletingDialog, 0, sizeof(HudCmdDialog));
    deletingDialog->Constructor();
    InstallHudCmdDialogBinding(deletingDialog->commandList, "DeleteCommand");
    deletingDialog->ScalarDeletingDestructor(1);

    RestoreFunctionPatch(loadPatch);
    RestoreFunctionPatch(rebuildPatch);
    RestoreFunctionPatch(setChildFlagsPatch);
    RestoreFunctionPatch(groupCountPatch);
    return noDeletePath ? 0 : 1;
}

extern "C" int zhud_text_input_constructor_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiTextInput));
    std::memset(storage, 0, sizeof(HudUiTextInput));
    HudUiTextInput *const input = static_cast<HudUiTextInput *>(storage);
    HudUiTextInput *const result = input->Constructor(8);

    const bool constructed =
        result == input &&
        input->buffer != nullptr &&
        input->capacity == 8 &&
        input->cursor == 0 &&
        input->keyActionMap[0x20] == 0 &&
        input->keyActionMap[0x2e] == 0 &&
        input->keyActionMap[0x1b] == 2 &&
        input->keyActionMap[0x0d] == 3 &&
        input->keyActionMap[0x08] == 4 &&
        input->keyActionMap[0x7f] == 5 &&
        input->keyActionMap[0x02] == 6 &&
        input->keyActionMap[0x06] == 7 &&
        input->keyActionMap['A'] == 0 &&
        input->keyActionMap[1] == 1;

    input->SetContents("abcdef");
    const bool contents =
        std::strcmp(input->GetBuffer(), "abcdef") == 0 &&
        input->cursor == 0;

    input->DestructorCore();
    ::operator delete(storage);
    return constructed && contents ? 0 : 1;
}

extern "C" int zhud_text_input_constructor_and_alloc_smoke(void) {
    return zhud_text_input_constructor_smoke();
}

extern "C" int zhud_polyline_and_slider_border_constructor_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
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
        (polyline.flags & 0x20) != 0 &&
        polyline.pointCount == 0 &&
        polyline.color565 == 0x1234 &&
        polyline.clipRect == nullptr &&
        pointsCleared;

    polyline.flags = 0;
    polyline.SetPoint(0, 5, 6);
    const bool firstPoint =
        polyline.points[0].x == 5 &&
        polyline.points[0].y == 6 &&
        polyline.pointCount == 1 &&
        polyline.x == 5 &&
        polyline.y == 6 &&
        (polyline.flags & 0x20) != 0;

    polyline.flags = 0;
    polyline.SetPoint(3, 7, 8);
    const bool laterPoint =
        polyline.points[3].x == 7 &&
        polyline.points[3].y == 8 &&
        polyline.pointCount == 4 &&
        polyline.x == 5 &&
        polyline.y == 6 &&
        (polyline.flags & 0x20) != 0;

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
        borderPoints = borderPoints &&
                       border.points[index].x == expected[index].x &&
                       border.points[index].y == expected[index].y;
    }

    const bool borderConstructed =
        border.pointCount == 13 &&
        border.x == -1 &&
        border.y == 0 &&
        border.originX == 0 &&
        border.originY == 0 &&
        border.halfWidth == 1 &&
        border.height == 10 &&
        border.blinkEnabled == 0 &&
        border.blinkPeriodSec == 0.35f &&
        border.blinkTimeRemainingSec == 0.0f &&
        border.blinkDirSign == 1 &&
        border.caretHalfWidth == 0x55 &&
        border.inputActive == 0x66 &&
        border.sliderVisibleWhenInputActive == 0x12 &&
        border.rawKeyFilterEnabled == 0x34 &&
        borderPoints;

    border.SetBounds(10, 20, 3, 8);
    const HudUiPolylinePoint expectedBounds[] = {
        {7, 20}, {13, 20}, {13, 21}, {10, 21}, {10, 27}, {13, 27}, {13, 28},
        {7, 28}, {7, 27},  {10, 27}, {10, 21}, {7, 21},  {7, 20},
    };

    bool boundsPoints = true;
    for (int index = 0; index < 13; ++index) {
        boundsPoints = boundsPoints &&
                       border.points[index].x == expectedBounds[index].x &&
                       border.points[index].y == expectedBounds[index].y;
    }

    const bool boundsSet =
        border.originX == 10 &&
        border.originY == 20 &&
        border.halfWidth == 3 &&
        border.height == 8 &&
        border.pointCount == 13 &&
        border.x == 7 &&
        border.y == 20 &&
        boundsPoints;

    g_HudUi_InvalidateMask = oldMask;
    return constructed && firstPoint && laterPoint && borderConstructed && boundsSet ? 0 : 1;
}

extern "C" int zhud_numeric_text_input_base_constructor_smoke(void) {
    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;

    void *const storage = ::operator new(sizeof(HudUiNumericTextInput));
    std::memset(storage, 0, sizeof(HudUiNumericTextInput));
    HudUiNumericTextInput *const input = static_cast<HudUiNumericTextInput *>(storage);
    HudUiNumericTextInput *const result = input->BaseConstructor();

    const bool constructed =
        result == input &&
        input->modeOrEnabled == 1 &&
        input->textInput.buffer != nullptr &&
        input->textInput.capacity == 0x100 &&
        input->textInput.cursor == 0 &&
        input->textInput.owner == input &&
        input->sliderBorder.sliderVisibleWhenInputActive == 0 &&
        input->sliderBorder.rawKeyFilterEnabled == 0 &&
        input->sliderBorder.inputActive == 1 &&
        input->sliderBorder.caretHalfWidth == 0 &&
        (input->sliderBorder.flags & 0x10u) == 0 &&
        (input->flags & 0x10u) == 0;

    input->Update("42");
    const bool updated =
        std::strcmp(input->textInput.buffer, "42") == 0 &&
        input->textInput.cursor == 2 &&
        (input->flags & 0x80u) != 0;

    const int previousActive = input->SetInputActive(0);
    const bool disabled =
        previousActive == 1 &&
        input->sliderBorder.inputActive == 0 &&
        (input->flags & 0x10u) != 0 &&
        (input->sliderBorder.flags & 0x10u) != 0;

    const int previousInactive = input->SetInputActive(1);
    const bool enabled =
        previousInactive == 0 &&
        input->sliderBorder.inputActive == 1 &&
        (input->flags & 0x10u) == 0 &&
        (input->sliderBorder.flags & 0x10u) == 0;

    input->Destructor();
    ::operator delete(storage);
    g_HudUi_InvalidateMask = oldMask;
    return constructed && updated && disabled && enabled ? 0 : 1;
}

extern "C" int zhud_std_ptr_vector_clear_no_op_destroy_smoke(void) {
    int values[3] = {1, 2, 3};
    StdPtrVector vector{};
    vector.ClearNoOpDestroy(values, values + 3);
    vector.ClearNoOpDestroy(values + 1, values + 1);

    return values[0] == 1 && values[1] == 2 && values[2] == 3 ? 0 : 1;
}
