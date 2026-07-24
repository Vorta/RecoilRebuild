#include "Battlesport/hud.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" unsigned int g_HudUi_InvalidateMask;
extern zFMV_Playback *g_HudUiSensorWindowPlayback;

namespace {

class HudSmokeLayout : public HudLayoutBase {
public:
    HudSmokeLayout()
        : enableCount(0),
          disableCount(0),
          activatedCount(0) {
    }

    void Enable() override {
        ++enableCount;
    }

    void Disable() override {
        ++disableCount;
    }

    void OnActivated() override {
        ++activatedCount;
    }

    int enableCount;
    int disableCount;
    int activatedCount;
};

class HudSmokeBindWidget : public HudUiZrdWidget {
public:
    HudSmokeBindWidget()
        : loadedNode(nullptr),
          loadedOwner(nullptr),
          postLoadCount(0) {
    }

    int LoadFromZrd(
        zReader::Node *node,
        HudUiBackground *ownerDialog
    ) override {
        loadedNode = node;
        loadedOwner = ownerDialog;
        return 1;
    }

    void PostLoadFromZrd() override {
        ++postLoadCount;
    }

    zReader::Node *loadedNode;
    HudUiBackground *loadedOwner;
    int postLoadCount;
};

int __fastcall HudSmokeVideoSurfaceStateNoOp(zVideo_SurfaceStatePartial *) {
    return 0;
}

HudUiPanelLayoutEntry *AllocatePanelEntries(unsigned int count) {
    return static_cast<HudUiPanelLayoutEntry *>(
        ::operator new(sizeof(HudUiPanelLayoutEntry) * count)
    );
}

void ConstructPanelEntry(
    HudUiPanelLayoutEntry *entry,
    const char *text,
    int x,
    int y
) {
    new (entry) HudUiPanelLayoutEntry(text, x, y);
    entry->layoutX = x;
    entry->layoutY = y;
}

void InitializePanelSpan(
    HudUiPanelSpan &span,
    unsigned int capacity,
    unsigned int count,
    const char *const *texts,
    const int *xs,
    const int *ys
) {
    span.begin = AllocatePanelEntries(capacity);
    span.end = span.begin;
    span.cap = span.begin + capacity;
    for (unsigned int index = 0; index < count; ++index) {
        ConstructPanelEntry(
            span.end,
            texts[index],
            xs[index],
            ys[index]
        );
        ++span.end;
    }
}

bool PanelEntryMatches(
    const HudUiPanelLayoutEntry &entry,
    const char *text,
    int x,
    int y
) {
    return std::strcmp(entry.panel.textBuffer, text) == 0 &&
           entry.layoutX == x &&
           entry.layoutY == y;
}

void InitializeSingleEntryPanelSpan(
    HudUiPanelSpan &span,
    const char *text,
    int x,
    int y
) {
    const char *texts[] = {text};
    const int xs[] = {x};
    const int ys[] = {y};
    InitializePanelSpan(span, 1, 1, texts, xs, ys);
}

bool SingleEntryPanelSpanMatches(
    const HudUiPanelSpan &span,
    const char *text,
    int x,
    int y
) {
    return span.begin != nullptr &&
           span.end == span.begin + 1 &&
           PanelEntryMatches(span.begin[0], text, x, y);
}

void InitializePanelSpanVector(
    HudUiPanelSpanVec &spanVector,
    unsigned int capacity,
    unsigned int count,
    const char *const *texts,
    const int *xs,
    const int *ys
) {
    spanVector.begin = static_cast<HudUiPanelSpan *>(
        ::operator new(sizeof(HudUiPanelSpan) * capacity)
    );
    spanVector.end = spanVector.begin;
    spanVector.cap = spanVector.begin + capacity;
    for (unsigned int index = 0; index < count; ++index) {
        new (spanVector.end) HudUiPanelSpan();
        InitializeSingleEntryPanelSpan(
            *spanVector.end,
            texts[index],
            xs[index],
            ys[index]
        );
        ++spanVector.end;
    }
}

} // namespace

extern "C" int zhud_mgr_toggle_hud_smoke(void) {
    int *const savedAccelerationOption = g_zGame_Options_PointerCache.videoAcceleration;
    int *const savedHudTypeSw = g_zGame_Options_PointerCache.hudTypeSw;
    const int savedHwMode = g_zOpt_HwMode;
    const int savedEnabled = g_HudUiMgr.enabled;
    HudLayoutBase *const savedCurrentLayout = g_HudUiMgrCurrentLayout;
    HudUiPanel *const savedDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    HudUiPanel *const savedSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const savedLabelPanel = g_HudUiMgrObjectiveLabelTextPanel;
    HudUiTimerPanel *const savedTimerPanel = g_HudUiMgrTimerPanel;
    zFMV_Playback *const savedPlayback = g_HudUiSensorWindowPlayback;
    const int savedLayoutDelayFrames = g_HudUiMgrLayoutDelayFrames;
    const int savedObjectivePhase = g_HudUiMgrObjectivePhase;
    const int savedTargetMarkerCount = g_HudUiMgrSensorTargetMarkerCount;
    const int savedWeaponState = g_HudUiMgrWeaponState;
    const zClipAltFloatRect savedSensorSourceRect =
        g_HudUiMgrSensorBlock.sensorPiVSrcRect;
    const int savedAltClipEnabled = gAltClipPassEnabled;
    const int savedAltClipRectValid = gAltClipSourceRectValid;
    const float savedClipLeft = g_zClipAlt_SourceLeft;
    const float savedClipTop = g_zClipAlt_SourceTop;
    const float savedClipRight = g_zClipAlt_SourceRight;
    const float savedClipBottom = g_zClipAlt_SourceBottom;
    const float savedClipWidth = g_zClipAlt_SourceWidth;
    const float savedClipHeight = g_zClipAlt_SourceHeight;

    unsigned int savedSlotFlags[32][2] = {};
    for (int index = 0; index < 32; ++index) {
        savedSlotFlags[index][0] = g_HudUiMgrWeaponSlots[index].slotWidget.flags;
        savedSlotFlags[index][1] =
            g_HudUiMgrWeaponSlots[index].trackMarkerWidget.flags;
    }
    const unsigned int savedObjectiveWidgetFlags = g_HudUiMgrObjectiveWidget.flags;
    const unsigned int savedObjectiveBarFlags = g_HudUiMgrObjectiveBar.flags;
    const unsigned int savedObjectiveSensorFlags =
        g_HudUiMgrObjectiveSensorRect.flags;
    const unsigned int savedObjectiveMeterFlags = g_HudUiMgrObjectiveMeter.flags;

    int accelerationOption = 1;
    int hudTypeSw = 2;
    g_zGame_Options_PointerCache.videoAcceleration = &accelerationOption;
    g_zGame_Options_PointerCache.hudTypeSw = &hudTypeSw;
    g_zOpt_HwMode = 0;
    g_HudUiSensorWindowPlayback = nullptr;

    HudSmokeLayout layout;
    HudUiPanel descPanel("", 0, 0);
    HudUiPanel summaryPanel("", 0, 0);
    HudUiPanel labelPanel("", 0, 0);
    HudUiTimerPanel timerPanel;
    timerPanel.flags = 0;

    g_HudUiMgrCurrentLayout = &layout;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveLabelTextPanel = &labelPanel;
    g_HudUiMgrTimerPanel = &timerPanel;

    for (int index = 0; index < 32; ++index) {
        g_HudUiMgrWeaponSlots[index].slotWidget.flags = 0;
        g_HudUiMgrWeaponSlots[index].trackMarkerWidget.flags = 0;
    }
    g_HudUiMgrObjectiveWidget.flags = 0;
    g_HudUiMgrObjectiveBar.flags = 0;
    g_HudUiMgrObjectiveSensorRect.flags = 0;
    g_HudUiMgrObjectiveMeter.flags = 0;
    g_HudUiMgr.enabled = 1;
    g_HudUiMgrLayoutDelayFrames = 0;
    gAltClipPassEnabled = 1;

    const int disableResult = HudUiMgr::ToggleHud();
    bool allSlotsHidden = true;
    for (int index = 0; index < 32; ++index) {
        allSlotsHidden =
            allSlotsHidden &&
            (g_HudUiMgrWeaponSlots[index].slotWidget.flags & 0x10u) != 0 &&
            (g_HudUiMgrWeaponSlots[index].trackMarkerWidget.flags & 0x10u) != 0;
    }
    const bool disabled =
        disableResult == 1 &&
        g_HudUiMgr.enabled == 0 &&
        layout.disableCount == 1 &&
        layout.enableCount == 0 &&
        allSlotsHidden &&
        (g_HudUiMgrObjectiveWidget.flags & 0x10u) != 0 &&
        (g_HudUiMgrObjectiveBar.flags & 0x10u) != 0 &&
        (g_HudUiMgrObjectiveSensorRect.flags & 0x10u) != 0 &&
        (g_HudUiMgrObjectiveMeter.flags & 0x10u) != 0 &&
        (timerPanel.flags & 0x10u) == 0 &&
        g_HudUiMgrLayoutDelayFrames == 2 &&
        gAltClipPassEnabled == 0;

    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveWidget.flags = 0x10;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.left = 2.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.top = 3.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.right = 22.0f;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom = 33.0f;
    gAltClipPassEnabled = 0;
    gAltClipSourceRectValid = 0;

    const int enableResult = HudUiMgr::ToggleHud();
    const bool enabled =
        enableResult == 1 &&
        g_HudUiMgr.enabled == 1 &&
        layout.enableCount == 1 &&
        layout.disableCount == 1 &&
        (g_HudUiMgrObjectiveWidget.flags & 0x10u) == 0 &&
        gAltClipPassEnabled == 1 &&
        gAltClipSourceRectValid == 1 &&
        g_zClipAlt_SourceLeft == 2.0f &&
        g_zClipAlt_SourceTop == 3.0f &&
        g_zClipAlt_SourceWidth == 20.0f &&
        g_zClipAlt_SourceHeight == 30.0f;

    for (int index = 0; index < 32; ++index) {
        g_HudUiMgrWeaponSlots[index].slotWidget.flags = savedSlotFlags[index][0];
        g_HudUiMgrWeaponSlots[index].trackMarkerWidget.flags =
            savedSlotFlags[index][1];
    }
    g_HudUiMgrObjectiveWidget.flags = savedObjectiveWidgetFlags;
    g_HudUiMgrObjectiveBar.flags = savedObjectiveBarFlags;
    g_HudUiMgrObjectiveSensorRect.flags = savedObjectiveSensorFlags;
    g_HudUiMgrObjectiveMeter.flags = savedObjectiveMeterFlags;
    g_HudUiMgr.enabled = savedEnabled;
    g_HudUiMgrCurrentLayout = savedCurrentLayout;
    g_HudUiMgrObjectiveDescTextPanel = savedDescPanel;
    g_HudUiMgrObjectiveSummaryTextPanel = savedSummaryPanel;
    g_HudUiMgrObjectiveLabelTextPanel = savedLabelPanel;
    g_HudUiMgrTimerPanel = savedTimerPanel;
    g_HudUiSensorWindowPlayback = savedPlayback;
    g_HudUiMgrLayoutDelayFrames = savedLayoutDelayFrames;
    g_HudUiMgrObjectivePhase = savedObjectivePhase;
    g_HudUiMgrSensorTargetMarkerCount = savedTargetMarkerCount;
    g_HudUiMgrWeaponState = savedWeaponState;
    g_HudUiMgrSensorBlock.sensorPiVSrcRect = savedSensorSourceRect;
    gAltClipPassEnabled = savedAltClipEnabled;
    gAltClipSourceRectValid = savedAltClipRectValid;
    g_zClipAlt_SourceLeft = savedClipLeft;
    g_zClipAlt_SourceTop = savedClipTop;
    g_zClipAlt_SourceRight = savedClipRight;
    g_zClipAlt_SourceBottom = savedClipBottom;
    g_zClipAlt_SourceWidth = savedClipWidth;
    g_zClipAlt_SourceHeight = savedClipHeight;
    g_zGame_Options_PointerCache.videoAcceleration = savedAccelerationOption;
    g_zGame_Options_PointerCache.hudTypeSw = savedHudTypeSw;
    g_zOpt_HwMode = savedHwMode;

    return disabled && enabled ? 0 : 1;
}

extern "C" int zhud_mgr_set_float_timer_visible_smoke(void) {
    HudUiTimerPanelFloat *const savedTimerPanel = g_HudUiMgrTimerPanelFloat;
    HudLayoutBase *const savedLayout = g_HudUiMgrCurrentLayout;

    HudUiTimerPanelFloat timer{};
    HudSmokeLayout layout;
    timer.flags = 0x10;
    g_HudUiMgrTimerPanelFloat = &timer;
    g_HudUiMgrCurrentLayout = &layout;

    HudUiMgr::SetFloatTimerVisible(1);
    const bool shown =
        (timer.flags & 0x10u) == 0 &&
        layout.activatedCount == 0;

    HudUiMgr::SetFloatTimerVisible(0);
    const bool hidden =
        (timer.flags & 0x10u) != 0 &&
        layout.activatedCount == 1;

    g_HudUiMgrTimerPanelFloat = savedTimerPanel;
    g_HudUiMgrCurrentLayout = savedLayout;
    return shown && hidden ? 0 : 1;
}

extern "C" int zhud_mgr_set_aux_overlay_visible_smoke(void) {
    HudUiStringMenu *const savedMenu = g_HudUiMgrStringMenu;
    HudUiStringMenu menu{};
    menu.enabled = 0;
    g_HudUiMgrStringMenu = &menu;

    HudUiMgr::SetAuxOverlayVisible(1);
    const bool shown = menu.enabled == 1;

    HudUiMgr::SetAuxOverlayVisible(0);
    const bool hidden = menu.enabled == 0;

    g_HudUiMgrStringMenu = savedMenu;
    return shown && hidden ? 0 : 1;
}

extern "C" int zhud_panel_span_clear_smoke(void) {
    HudUiPanelSpan span;
    span.allocatorProxy = 0xabcdef01;
    const char *texts[] = {"line 1", "line 2"};
    const int xs[] = {10, 30};
    const int ys[] = {20, 40};
    InitializePanelSpan(span, 2, 2, texts, xs, ys);

    span.Clear();

    return span.allocatorProxy == 0xabcdef01 &&
                   span.begin == nullptr &&
                   span.end == nullptr &&
                   span.cap == nullptr
               ? 0
               : 1;
}

extern "C" int zhud_panel_span_destroy_and_free_smoke(void) {
    HudUiPanelSpan span;
    span.allocatorProxy = 0x1234abcd;
    const char *texts[] = {"row a", "row b"};
    const int xs[] = {12, 32};
    const int ys[] = {22, 42};
    InitializePanelSpan(span, 2, 2, texts, xs, ys);

    span.DestroyAndFree();

    return span.allocatorProxy == 0x1234abcd &&
                   span.begin == nullptr &&
                   span.end == nullptr &&
                   span.cap == nullptr
               ? 0
               : 1;
}

extern "C" int zhud_panel_span_insert_n_smoke(void) {
    HudUiPanelLayoutEntry templateEntry("insert", 90, 91);
    templateEntry.layoutX = 90;
    templateEntry.layoutY = 91;

    bool growOk = false;
    {
        HudUiPanelSpan span;
        const char *texts[] = {"before", "after"};
        const int xs[] = {10, 20};
        const int ys[] = {11, 21};
        InitializePanelSpan(span, 2, 2, texts, xs, ys);
        span.InsertN(span.begin + 1, 1, &templateEntry);
        growOk =
            span.end == span.begin + 3 &&
            span.cap == span.begin + 4 &&
            PanelEntryMatches(span.begin[0], "before", 10, 11) &&
            PanelEntryMatches(span.begin[1], "insert", 90, 91) &&
            PanelEntryMatches(span.begin[2], "after", 20, 21);
    }

    bool longTailOk = false;
    {
        HudUiPanelSpan span;
        const char *texts[] = {"row 0", "row 1", "row 2"};
        const int xs[] = {30, 40, 50};
        const int ys[] = {31, 41, 51};
        InitializePanelSpan(span, 4, 3, texts, xs, ys);
        span.InsertN(span.begin + 1, 1, &templateEntry);
        longTailOk =
            span.end == span.begin + 4 &&
            span.cap == span.begin + 4 &&
            PanelEntryMatches(span.begin[0], "row 0", 30, 31) &&
            PanelEntryMatches(span.begin[1], "insert", 90, 91) &&
            PanelEntryMatches(span.begin[2], "row 1", 40, 41) &&
            PanelEntryMatches(span.begin[3], "row 2", 50, 51);
    }

    bool shortTailOk = false;
    {
        HudUiPanelSpan span;
        const char *texts[] = {"base 0", "base 1", "base 2"};
        const int xs[] = {60, 70, 80};
        const int ys[] = {61, 71, 81};
        InitializePanelSpan(span, 5, 3, texts, xs, ys);
        span.InsertN(span.begin + 2, 2, &templateEntry);
        shortTailOk =
            span.end == span.begin + 5 &&
            span.cap == span.begin + 5 &&
            PanelEntryMatches(span.begin[0], "base 0", 60, 61) &&
            PanelEntryMatches(span.begin[1], "base 1", 70, 71) &&
            PanelEntryMatches(span.begin[2], "insert", 90, 91) &&
            PanelEntryMatches(span.begin[3], "insert", 90, 91) &&
            PanelEntryMatches(span.begin[4], "base 2", 80, 81);
    }

    return growOk && longTailOk && shortTailOk ? 0 : 1;
}

extern "C" int zhud_panel_span_copy_init_smoke(void) {
    HudUiPanelSpan source;
    source.allocatorProxy = 0x12345678;
    const char *texts[] = {"copy init a", "copy init b"};
    const int xs[] = {21, 41};
    const int ys[] = {31, 51};
    InitializePanelSpan(source, 2, 2, texts, xs, ys);

    HudUiPanelSpan copied;
    HudUiPanelSpan *const result = copied.CopyInit(&source);

    return result == &copied &&
                   copied.allocatorProxy == 0x78 &&
                   copied.begin != source.begin &&
                   copied.end == copied.begin + 2 &&
                   copied.cap == copied.end &&
                   PanelEntryMatches(copied.begin[0], "copy init a", 21, 31) &&
                   PanelEntryMatches(copied.begin[1], "copy init b", 41, 51)
               ? 0
               : 1;
}

extern "C" int zhud_panel_span_copy_from_smoke(void) {
    HudUiPanelSpan source;
    const char *sourceTexts[] = {"copy from a", "copy from b"};
    const int sourceXs[] = {22, 42};
    const int sourceYs[] = {32, 52};
    InitializePanelSpan(source, 2, 2, sourceTexts, sourceXs, sourceYs);

    bool shrinkOk = false;
    {
        HudUiPanelSpan shrink;
        const char *texts[] = {"old a", "old b", "old c"};
        const int xs[] = {1, 3, 5};
        const int ys[] = {2, 4, 6};
        InitializePanelSpan(shrink, 3, 3, texts, xs, ys);
        HudUiPanelLayoutEntry *const abandonedEntry = shrink.begin + 2;
        shrink.CopyFrom(&source);
        shrinkOk =
            shrink.end == shrink.begin + 2 &&
            shrink.cap == shrink.begin + 3 &&
            PanelEntryMatches(shrink.begin[0], "copy from a", 22, 32) &&
            PanelEntryMatches(shrink.begin[1], "copy from b", 42, 52);
        abandonedEntry->panel.~HudUiPanel();
    }

    bool expandOk = false;
    {
        HudUiPanelSpan expand;
        const char *texts[] = {"small"};
        const int xs[] = {7};
        const int ys[] = {8};
        InitializePanelSpan(expand, 3, 1, texts, xs, ys);
        expand.CopyFrom(&source);
        expandOk =
            expand.end == expand.begin + 2 &&
            expand.cap == expand.begin + 3 &&
            PanelEntryMatches(expand.begin[0], "copy from a", 22, 32) &&
            PanelEntryMatches(expand.begin[1], "copy from b", 42, 52);
    }

    bool reallocateOk = false;
    {
        HudUiPanelSpan reallocate;
        const char *texts[] = {"tiny"};
        const int xs[] = {9};
        const int ys[] = {10};
        InitializePanelSpan(reallocate, 1, 1, texts, xs, ys);
        reallocate.CopyFrom(&source);
        reallocateOk =
            reallocate.end == reallocate.begin + 2 &&
            reallocate.cap == reallocate.end &&
            PanelEntryMatches(reallocate.begin[0], "copy from a", 22, 32) &&
            PanelEntryMatches(reallocate.begin[1], "copy from b", 42, 52);
    }

    return shrinkOk && expandOk && reallocateOk ? 0 : 1;
}

extern "C" int zhud_panel_span_vec_insert_n_smoke(void) {
    HudUiPanelSpan templateSpan;
    InitializeSingleEntryPanelSpan(templateSpan, "span insert", 210, 211);

    bool growOk = false;
    {
        HudUiPanelSpanVec spanVector;
        const char *texts[] = {"vec before", "vec after"};
        const int xs[] = {10, 20};
        const int ys[] = {11, 21};
        InitializePanelSpanVector(spanVector, 2, 2, texts, xs, ys);
        spanVector.InsertN(spanVector.begin + 1, 1, &templateSpan);
        growOk =
            spanVector.end == spanVector.begin + 3 &&
            spanVector.cap == spanVector.begin + 4 &&
            SingleEntryPanelSpanMatches(spanVector.begin[0], "vec before", 10, 11) &&
            SingleEntryPanelSpanMatches(spanVector.begin[1], "span insert", 210, 211) &&
            SingleEntryPanelSpanMatches(spanVector.begin[2], "vec after", 20, 21);
    }

    bool longTailOk = false;
    {
        HudUiPanelSpanVec spanVector;
        const char *texts[] = {"vec 0", "vec 1", "vec 2"};
        const int xs[] = {30, 40, 50};
        const int ys[] = {31, 41, 51};
        InitializePanelSpanVector(spanVector, 4, 3, texts, xs, ys);
        spanVector.InsertN(spanVector.begin + 1, 1, &templateSpan);
        longTailOk =
            spanVector.end == spanVector.begin + 4 &&
            spanVector.cap == spanVector.begin + 4 &&
            SingleEntryPanelSpanMatches(spanVector.begin[0], "vec 0", 30, 31) &&
            SingleEntryPanelSpanMatches(spanVector.begin[1], "span insert", 210, 211) &&
            SingleEntryPanelSpanMatches(spanVector.begin[2], "vec 1", 40, 41) &&
            SingleEntryPanelSpanMatches(spanVector.begin[3], "vec 2", 50, 51);
    }

    bool shortTailOk = false;
    {
        HudUiPanelSpanVec spanVector;
        const char *texts[] = {"vec base 0", "vec base 1", "vec base 2"};
        const int xs[] = {60, 70, 80};
        const int ys[] = {61, 71, 81};
        InitializePanelSpanVector(spanVector, 5, 3, texts, xs, ys);
        spanVector.InsertN(spanVector.begin + 2, 2, &templateSpan);
        shortTailOk =
            spanVector.end == spanVector.begin + 5 &&
            spanVector.cap == spanVector.begin + 5 &&
            SingleEntryPanelSpanMatches(spanVector.begin[0], "vec base 0", 60, 61) &&
            SingleEntryPanelSpanMatches(spanVector.begin[1], "vec base 1", 70, 71) &&
            SingleEntryPanelSpanMatches(spanVector.begin[2], "span insert", 210, 211) &&
            SingleEntryPanelSpanMatches(spanVector.begin[3], "span insert", 210, 211) &&
            SingleEntryPanelSpanMatches(spanVector.begin[4], "vec base 2", 80, 81);
    }

    return growOk && longTailOk && shortTailOk ? 0 : 1;
}

extern "C" int zhud_panel_destructor_callback_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiPanel));
    HudUiPanel *const panel = new (storage) HudUiPanel("callback", 0, 0);
    panel->textPick = zVid_Image::Create();

    HudUiPanel::DestructorCallback(panel);
    const bool destroyed = panel->textPick == nullptr;

    ::operator delete(storage);
    return destroyed ? 0 : 1;
}

extern "C" int zhud_cycle_selector_widget_load_from_zrd_smoke(void) {
    const unsigned int savedInvalidateMask = g_HudUi_InvalidateMask;
    unsigned int (*const savedGetIdProc)(const char *) = g_zLoc_GetIdProc;
    g_HudUi_InvalidateMask = 0x80;
    g_zLoc_GetIdProc = nullptr;

    HudUiBackground owner;
    owner.uiOriginX = 100;
    owner.uiOriginY = 200;
    owner.fontStyles[2].validMarker = 1;
    owner.fontStyles[2].fontName = "Arial";
    owner.fontStyles[2].fontSize = 9;
    owner.fontStyles[2].fontWeight = FW_BOLD;
    owner.fontStyles[2].textColor = 0x00010203;
    owner.fontStyles[2].shadowEnabled = 1;
    owner.fontStyles[2].alignMode = 2;
    owner.fontStyles[2].bkMode = 1;
    owner.fontStyles[2].bkColor = 0x00040506;

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
    bitmapItems[1].type = zReader::ZRDR_NODE_STRING;
    bitmapItems[1].value.str =
        const_cast<char *>("__missing_cycle_selector_bitmap__.tex");
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

    bool loaded = false;
    {
        HudUiCycleSelectorWidget widget;
        widget.visibleCount = 1;
        const int result = widget.LoadFromZrd(&root, &owner);

        HudUiTransitionTextPanel *const textEntry =
            reinterpret_cast<HudUiTransitionTextPanel *>(widget.entriesA[0]);
        HudUiWidget *const bitmapEntry = widget.entriesB[0];
        loaded =
            result == 1 &&
            widget.owner == &owner &&
            widget.fontStyleRef == reinterpret_cast<void *>(7) &&
            widget.textOffsetX == 11 &&
            widget.textOffsetY == -3 &&
            widget.itemCount == 2 &&
            widget.visibleCount == 2 &&
            textEntry != nullptr &&
            std::strcmp(textEntry->textBuffer, "FIRST") == 0 &&
            textEntry->x == 115 &&
            textEntry->y == 202 &&
            textEntry->textColor0 == 0x00010203 &&
            bitmapEntry != nullptr &&
            bitmapEntry->x == 108 &&
            bitmapEntry->y == 209 &&
            (bitmapEntry->flags & 0x10u) != 0;
    }

    g_HudUi_InvalidateMask = savedInvalidateMask;
    g_zLoc_GetIdProc = savedGetIdProc;
    return loaded ? 0 : 1;
}

extern "C" int zhud_message_box_leaf_handlers_smoke(void) {
    HudUiMessageBoxDialog dialog{};

    dialog.modalResult = 99;
    dialog.modalFrameCountdown = 7;
    dialog.OnOk();
    const bool okResult =
        dialog.modalResult == 1 &&
        dialog.modalFrameCountdown == 0;

    dialog.modalResult = 99;
    dialog.modalFrameCountdown = 7;
    dialog.OnCancel();
    const bool cancelResult =
        dialog.modalResult == 2 &&
        dialog.modalFrameCountdown == 0;

    dialog.okButton.owner = &dialog;
    dialog.modalResult = 0;
    dialog.modalFrameCountdown = 5;
    dialog.okButton.OnActivate();
    const bool okActivated =
        dialog.modalResult == 1 &&
        dialog.modalFrameCountdown == 0;

    dialog.cancelButton.owner = &dialog;
    dialog.modalResult = 0;
    dialog.modalFrameCountdown = 5;
    dialog.cancelButton.OnActivate();
    const bool cancelActivated =
        dialog.modalResult == 2 &&
        dialog.modalFrameCountdown == 0;

    return okResult && cancelResult && okActivated && cancelActivated ? 0 : 1;
}

extern "C" int zhud_message_box_scalar_deleting_destructor_smoke(void) {
    HudUiMessageBoxDialog *const dialog = new HudUiMessageBoxDialog{};
    dialog->backgroundImage = nullptr;
    dialog->okButtonNormalImage = nullptr;
    dialog->okButtonPressedImage = nullptr;

    HudUiBackground *const polymorphicOwner = dialog;
    delete polymorphicOwner;
    return 0;
}

extern "C" int zhud_background_container_focus_smoke(void) {
    HudUiBackgroundContainer container(7);
    HudUiElement focus{};

    const bool constructed =
        container.enabled == 0 &&
        container.childHead == nullptr &&
        container.childTail == nullptr &&
        container.inputFocusElement == nullptr &&
        container.captureTransitionMask == 7;

    container.SetInputFocus(&focus);
    const bool focused =
        container.GetInputFocus() == &focus &&
        container.inputFocusElement == &focus;

    container.SetEnabled(1);
    const bool enabled = container.enabled == 1;
    container.SetEnabled(0);
    const bool disabled = container.enabled == 0;

    return constructed && focused && enabled && disabled ? 0 : 1;
}

extern "C" int zhud_background_free_loaded_tree_roots_smoke(void) {
    HudUiBackground background;
    zReader::Node *const root =
        static_cast<zReader::Node *>(std::malloc(sizeof(zReader::Node)));
    if (root == nullptr) {
        return 1;
    }

    std::memset(root, 0, sizeof(*root));
    root->type = zReader::ZRDR_NODE_INT;
    root->value.i32 = 42;
    zReader::Node cfgRoot{};
    background.loadedRoot = root;
    background.cfgRoot = &cfgRoot;

    background.FreeLoadedTreeRoots(0x1234);
    const bool cleared =
        background.loadedRoot == nullptr &&
        background.cfgRoot == nullptr;

    background.FreeLoadedTreeRoots(0);
    return cleared ? 0 : 1;
}

extern "C" int zhud_background_video_widget_set_color_key_smoke(void) {
    HudUiBackgroundVideoWidget widget;
    zFMV_Stream stream{};

    stream.formatFlagsPacked = 0x40;
    widget.stream = &stream;
    widget.SetColorKey565(0x07e0);
    const bool streamPresent =
        stream.formatFlagsPacked == 0x42 &&
        widget.colorKey565 == 0x07e0;

    widget.stream = nullptr;
    stream.formatFlagsPacked = 0x10;
    widget.SetColorKey565(0x001f);
    const bool streamAbsent =
        stream.formatFlagsPacked == 0x10 &&
        widget.colorKey565 == 0x001f;

    return streamPresent && streamAbsent ? 0 : 1;
}

extern "C" int zhud_background_video_widget_set_media_path_missing_smoke(void) {
    HudUiBackgroundVideoWidget widget;
    const char *const missingPath = "__missing_hud_video_widget__.avi";
    std::remove(missingPath);

    widget.stream = reinterpret_cast<zFMV_Stream *>(0x11111111);
    std::memset(widget.mediaPath, 0x7f, sizeof(widget.mediaPath));
    widget.mediaPath[sizeof(widget.mediaPath) - 1] = '\0';

    widget.SetMediaPathOwnedAndRefresh(missingPath);

    return widget.stream == nullptr &&
                   std::strcmp(widget.mediaPath, missingPath) == 0
               ? 0
               : 1;
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

    HudUiBackground background;
    HudSmokeBindWidget directWidget;
    const unsigned char directResult =
        background.BindButtonsNodeToWidgetByName(
            &root,
            &directWidget,
            "TARGET"
        );
    const bool directBound =
        directResult == 0 &&
        directWidget.loadedNode == &buttonItems[2] &&
        directWidget.loadedOwner == &background &&
        directWidget.postLoadCount == 1;

    HudSmokeBindWidget wrapperWidget;
    background.cfgRoot = &root;
    const int wrapperResult =
        background.BindWidgetByName(
            nullptr,
            &wrapperWidget,
            "TARGET"
        );
    const bool wrapperBound =
        wrapperResult == 0 &&
        wrapperWidget.loadedNode == &buttonItems[2] &&
        wrapperWidget.loadedOwner == &background &&
        wrapperWidget.postLoadCount == 1;

    HudSmokeBindWidget missingWidget;
    const unsigned char missingResult =
        background.BindButtonsNodeToWidgetByName(
            &root,
            &missingWidget,
            "MISSING"
        );
    const bool missingSkipped =
        missingResult == 0 &&
        missingWidget.loadedNode == nullptr &&
        missingWidget.postLoadCount == 0;

    return directBound && wrapperBound && missingSkipped ? 0 : 1;
}

extern "C" int zhud_background_load_from_zrd_missing_path_smoke(void) {
    const char *const missingPath = "__missing_hud_background_load__.zrd";
    std::remove(missingPath);

    const int savedRendererType = g_zVideo_RendererType;
    const int savedHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial savedPrimarySurface =
        g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const savedLockSurfaceState =
        g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const savedUnlockSurfaceState =
        g_zVideo_pfnUnlockSurfaceState;

    std::uint16_t pixels[4] = {};
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = HudSmokeVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = HudSmokeVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(std::uint16_t) * 2;

    zReader::Node existingCfg = {};
    HudUiBackground background;
    background.loadedRoot = reinterpret_cast<zReader::Node *>(0x11111111);
    background.cfgRoot = &existingCfg;
    zReader::Node *const result =
        background.LoadFromZrd(missingPath, "SECTION", 1);
    const bool missing =
        result == nullptr &&
        background.loadedRoot == nullptr &&
        background.cfgRoot == &existingCfg;

    g_zVideo_RendererType = savedRendererType;
    g_zVideo_UseHalfResBackbuffer = savedHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = savedPrimarySurface;
    g_zVideo_pfnLockSurfaceState = savedLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = savedUnlockSurfaceState;

    return missing ? 0 : 1;
}

extern "C" int zhud_font_style_constructor_smoke(void) {
    HudFontStyle style;
    const bool constructed =
        style.validMarker == 0 &&
        style.fontName == nullptr &&
        style.fontSize == 0 &&
        style.textColor == 0 &&
        style.shadowEnabled == 0 &&
        style.fontWeight == 0x1f4 &&
        style.alignMode == 0;

    const char *const fontName = "Tahoma";
    style.validMarker = 1;
    style.fontName = fontName;
    style.Destructor();
    const bool destructed =
        style.validMarker == 0 &&
        style.fontName == fontName;

    return constructed && destructed ? 0 : 1;
}

extern "C" int zhud_panel_destructor_thunk_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiPanel));
    HudUiPanel *const panel = new (storage) HudUiPanel("thunk", 0, 0);
    panel->textPick = zVid_Image::Create();

    panel->DestructorThunk();
    const bool destroyed = panel->textPick == nullptr;

    ::operator delete(storage);
    return destroyed ? 0 : 1;
}

extern "C" int zhud_panel_text_color_shadow_smoke(void) {
    const unsigned int savedInvalidateMask = g_HudUi_InvalidateMask;
    HudUiPanel panel("", 0, 0);
    panel.textColor0 = 0x111111;
    panel.shadowEnabled = 5;

    const unsigned int previousColor = panel.SetTextColor(0x223344);
    const bool colorSet =
        previousColor == 0x111111 &&
        panel.textColor0 == 0x223344 &&
        panel.textColor1 == 0x223344 &&
        panel.textDirty == 1;

    panel.textDirty = 0;
    panel.SetTextColorsAndMarkDirty(0x010203, 0x040506);
    const unsigned int previousShadow = panel.SetShadow(1, 7, -3);
    const bool shadowSet =
        previousShadow == 5 &&
        panel.shadowEnabled == 1 &&
        panel.shadowOffsetX == 7 &&
        panel.shadowOffsetY == -3;

    g_HudUi_InvalidateMask = 0x80;
    panel.flags = 0;
    panel.textDirty = 0;
    panel.Invalidate();
    const bool invalidated =
        (panel.flags & 0x80u) != 0 &&
        panel.textDirty == 1;

    const HudUiRect wrapRect{3, 4, 50, 60};
    panel.EnableWordWrapWithRect(&wrapRect);
    const bool wrapped =
        panel.wordWrapEnabled == 1 &&
        std::memcmp(&panel.wrapRect, &wrapRect, sizeof(wrapRect)) == 0;

    g_HudUi_InvalidateMask = savedInvalidateMask;
    return colorSet &&
                   panel.textColor0 == 0x010203 &&
                   panel.textColor1 == 0x040506 &&
                   panel.textDirty == 1 &&
                   shadowSet &&
                   invalidated &&
                   wrapped
               ? 0
               : 1;
}

extern "C" int zhud_panel_measure_text_prefix_rect_smoke(void) {
    HudUiPanel panel("WWW", 0, 0);

    RECT whole{10, 20, 10, 20};
    const int wholeResult = panel.MeasureTextPrefixRect(3, &whole);

    RECT prefix{10, 20, 10, 20};
    const int prefixResult = panel.MeasureTextPrefixRect(1, &prefix);

    RECT tooLong{10, 20, 77, 20};
    const int tooLongResult = panel.MeasureTextPrefixRect(4, &tooLong);

    RECT empty{10, 20, 99, 20};
    const int emptyResult = panel.MeasureTextPrefixRect(0, &empty);

    return wholeResult == 1 &&
                   prefixResult == 1 &&
                   tooLongResult == 0 &&
                   emptyResult == 1 &&
                   whole.right > prefix.right &&
                   prefix.right > prefix.left &&
                   tooLong.right == 77 &&
                   empty.right == empty.left
               ? 0
               : 1;
}
